#pragma once

// World module state: draw census, camera and depth capture. DevKit-only,
// Sora 2nd only. Everything here is inert unless falcom_world::Use()
// activated the module.
//
// Model hierarchy (do not collapse it):
//   VS family -> draw -> mesh (VB/IB) -> instance record -> per-instance world.
// A VS hash is a geometry/material class, never a unique mesh.

#include <array>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <include/reshade.hpp>

#include "../../../utils/scene.hpp"
#include "../../../utils/settings.hpp"

namespace falcom_world {

inline constexpr uint32_t kCbSlotCapacity = 16u;
inline constexpr uint32_t kSrvSlotCapacity = 32u;
inline constexpr uint32_t kCensusRingSize = 4096u;
inline constexpr uint32_t kLightingHashSora2nd = 0xCA3D8596u;
inline constexpr uint32_t kLightingDepthRegisterSora2nd = 4u;

// Instance transform layout (the pool stores Inst4x3Row; see bvh/pool_transform.hpp).
enum class CandidateKind : uint8_t {
  Cb4x4Row = 0,
  Cb4x4Col,
  Inst4x3Row,
  Inst4x3Col,
  Inst4x4Row,
  Inst4x4Col,
  Count,
};

enum class FamilyLabel : uint8_t {
  Unknown = 0,
  Terrain,
  Buildings,
  Props,
  Glass,
  Character,
  Foliage,
  Transparent,
  Other,
  Count,
};

enum class LabelSource : uint8_t {
  Unknown = 0,
  Hint,
  Manual,
};

struct DrawRecord {
  uint32_t frame = 0u;
  uint32_t serial = 0u;
  uint8_t method = 0u;
  uint32_t vertex_count = 0u;
  uint32_t index_count = 0u;
  uint32_t instance_count = 0u;
  uint32_t first_vertex = 0u;
  uint32_t first_index = 0u;
  int32_t vertex_offset = 0;
  uint32_t first_instance = 0u;
  uint32_t vs_hash = 0u;
  uint32_t ps_hash = 0u;
  // Bound shader objects (pipeline handles); the shader registry classifies
  // them from their bytecode, which is what the pool admits on.
  uint64_t vs_pipeline = 0u;
  uint64_t ps_pipeline = 0u;
  reshade::api::resource vb = {0u};
  uint64_t vb_offset = 0u;
  uint32_t vb_stride = 0u;
  uint64_t vb_size = 0u;
  reshade::api::resource ib = {0u};
  uint64_t ib_offset = 0u;
  uint32_t index_size = 0u;
  uint64_t ib_size = 0u;
  bool has_index_buffer = false;
  reshade::api::pipeline input_layout = {0u};
  reshade::api::primitive_topology topology = reshade::api::primitive_topology::undefined;
  reshade::api::resource_view rtv0 = {0u};
  reshade::api::resource_view dsv = {0u};
  reshade::api::viewport viewport = {};
  bool has_viewport = false;
  bool blend_enable = false;
  bool depth_enable = false;
  bool depth_write = false;
  uint32_t cull_mode = 0u;
  bool has_skin_inputs = false;
  bool is_candidate = false;
  uint32_t vs_cb_mask = 0u;
  uint32_t ps_cb_mask = 0u;
  uint32_t vs_srv_mask = 0u;
  reshade::api::resource vs_cb0 = {0u};
  reshade::api::resource ps_cb0 = {0u};
  reshade::api::resource vs_srv0 = {0u};
};

struct CameraSnapshot {
  bool valid = false;
  uint32_t frame = 0u;
  uint8_t source = 0u;  // 0 = unknown, 1 = cache, 2 = gpu snapshot
  float view[16] = {};
  float view_inv[16] = {};
  float proj[16] = {};
  float view_proj[16] = {};
  float view_proj_inv[16] = {};
  float prev_view_proj[16] = {};
  bool has_prev = false;
  // cb_scene scalars of the map-object near fade (shader_contract.hpp), when
  // the captured constants reach them.
  bool has_fade = false;
  float near_fade_floor = 0.f;  // disableMapObjNearFade_g
  float map_alpha = 1.f;        // mapColor_g.w
};

struct FamilyStats {
  uint32_t vs_hash = 0u;
  uint64_t draws = 0u;
  uint64_t triangles = 0u;
  uint64_t indices = 0u;
  uint64_t instances = 0u;
  uint32_t depth_write_draws = 0u;
  uint32_t blend_draws = 0u;
  uint32_t candidate_draws = 0u;
  uint32_t skin_draws = 0u;
  std::unordered_set<uint64_t> meshes;
  std::unordered_set<uint32_t> ps_hashes;
  bool candidate = false;
  uint8_t label = 0u;
  uint8_t label_source = 0u;
  bool label_prefilled = false;
};

struct DepthSource {
  bool valid = false;
  uint32_t frame = UINT32_MAX;
  reshade::api::resource_view view = {0u};
  reshade::api::resource resource = {0u};
  uint32_t width = 0u;
  uint32_t height = 0u;
  // Lighting pass viewport: the depth texels the frame actually covers
  // (smaller than the texture when the game renders at a reduced resolution).
  float viewport_x = 0.f;
  float viewport_y = 0.f;
  float viewport_width = 0.f;
  float viewport_height = 0.f;
};

struct State {
  bool supported = false;

  float setting_enabled = 0.f;

  std::atomic<uint32_t> frame{0u};
  std::atomic<uint32_t> next_serial{0u};
  // Set by the BVH module while Pool Scan or a GPU debug view needs draws
  // observed (pool capture, live camera) without the research census.
  std::atomic_bool pool_capture_requested{false};
  uint32_t draw_counter = 0u;

  std::mutex mutex;
  std::vector<DrawRecord> ring;
  uint32_t ring_head = 0u;
  std::unordered_map<uint32_t, FamilyStats> families;
  std::unordered_map<uint64_t, uint64_t> resource_sizes;

  CameraSnapshot camera;
  DepthSource depth_source;

  std::string status;
};

inline State g_state;

inline bool CensusEnabled() { return g_state.setting_enabled > 0.5f; }

// Draw observation runs when the census or the BVH module needs it.
inline bool CaptureActive() {
  return CensusEnabled() || g_state.pool_capture_requested.load(std::memory_order_relaxed);
}

inline const char* FamilyLabelName(uint8_t label) {
  switch (static_cast<FamilyLabel>(label)) {
    case FamilyLabel::Terrain: return "Terrain";
    case FamilyLabel::Buildings: return "Buildings";
    case FamilyLabel::Props: return "Props";
    case FamilyLabel::Glass: return "Glass/Windows";
    case FamilyLabel::Character: return "Character";
    case FamilyLabel::Foliage: return "Foliage";
    case FamilyLabel::Transparent: return "Transparent";
    case FamilyLabel::Other: return "Other";
    default: return "Unknown";
  }
}

inline const char* LabelSourceName(uint8_t source) {
  switch (static_cast<LabelSource>(source)) {
    case LabelSource::Hint: return "hint";
    case LabelSource::Manual: return "manual";
    default: return "unknown";
  }
}

}  // namespace falcom_world
