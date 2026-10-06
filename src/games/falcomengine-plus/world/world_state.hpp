#pragma once

// Phase 0 world-transform research state. DevKit-only, Sora 2nd only.
// Everything here is inert unless falcom_world::Use() activated the module.
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
inline constexpr uint32_t kMaxCbBytes = 64u * 1024u;
inline constexpr uint32_t kMaxCandidates = 64u;
inline constexpr uint32_t kMaxCandidateInstances = 64u;
inline constexpr uint32_t kMaxCbSnapshots = 32u;
inline constexpr uint32_t kMaxSrvSnapshots = 8u;
inline constexpr uint64_t kMaxSrvReadBytes = 4ull * 1024ull * 1024ull;
inline constexpr uint32_t kLightingHashSora2nd = 0xCA3D8596u;
inline constexpr uint32_t kLightingDepthRegisterSora2nd = 4u;

enum class CandidateKind : uint8_t {
  Cb4x4Row = 0,
  Cb4x4Col,
  Inst4x3Row,
  Inst4x3Col,
  Inst4x4Row,
  Inst4x4Col,
  Count,
};

enum class CandidateSource : uint8_t {
  Generic = 0,
  HintDiagnostic,
  PreferredSignature,
};

inline const char* CandidateSourceName(uint8_t source) {
  switch (static_cast<CandidateSource>(source)) {
    case CandidateSource::HintDiagnostic: return "hint_diagnostic";
    case CandidateSource::PreferredSignature: return "preferred_signature";
    default: return "generic";
  }
}

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
  int32_t instance_offset_g = 0;
  bool instance_offset_found = false;
};

struct CbSnapshot {
  uint8_t stage = 0u;  // 1 = vertex, 2 = pixel
  uint32_t slot = 0u;
  std::vector<uint8_t> bytes;
  bool gpu_snapshot = false;
  uint64_t resource_size = 0u;
  uint64_t snapshot_size = 0u;
};

struct SrvBufferSnapshot {
  uint8_t stage = 0u;
  uint32_t slot = 0u;
  reshade::api::resource resource = {0u};
  uint32_t stride = 0u;
  uint64_t size = 0u;
  uint64_t read_offset = 0u;
  uint64_t read_size = 0u;
  std::vector<uint8_t> bytes;
  bool truncated = false;
  bool valid = false;
};

struct CapturedDraw {
  DrawRecord draw;
  std::vector<CbSnapshot> cbs;
  std::vector<SrvBufferSnapshot> srv_buffers;
  bool mesh_valid = false;
  renodx::utils::scene::CapturedMesh mesh;
  std::string mesh_status;
  uint32_t rtv_w = 0u;
  uint32_t rtv_h = 0u;
  int32_t instance_offset_g = 0;
  bool instance_offset_found = false;
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
};

struct TransformCandidate {
  CandidateKind kind = CandidateKind::Cb4x4Row;
  CandidateSource source = CandidateSource::Generic;
  bool row_dot = false;  // audited row-dot convention (hint diagnostics only)
  uint8_t stage = 0u;
  uint32_t slot = 0u;
  uint32_t matrix_offset = 0u;  // bytes in a CB, or within the element for structured
  uint32_t stride = 0u;         // structured buffer stride (0 for CB)
  uint32_t base_offset = 0u;    // instanceOffset candidate
  uint32_t element_count = 0u;  // instances covered by this candidate
  uint32_t instances_ok = 0u;
  bool has_prev = false;
  float set_score = 0.f;
  float mean_inside = 0.f;
  float bbox_area = 0.f;
  float ndc_z_valid_ratio = 0.f;
  bool projection_valid = false;
  std::vector<float> matrices;       // 12 (4x3) or 16 (4x4) per instance
  std::vector<float> prev_matrices;  // 12 per instance when has_prev
  bool valid = false;
};

struct CandidateSummary {
  CandidateKind kind = CandidateKind::Cb4x4Row;
  CandidateSource source = CandidateSource::Generic;
  uint8_t stage = 0u;
  uint32_t slot = 0u;
  uint32_t matrix_offset = 0u;
  uint32_t stride = 0u;
  uint32_t base_offset = 0u;
  uint32_t element_count = 0u;
  uint32_t instances_ok = 0u;
  bool has_prev = false;
  float set_score = 0.f;
  float mean_inside = 0.f;
  float ndc_z_valid_ratio = 0.f;
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
  bool verified = false;
  bool transform_found = false;
  uint8_t label = 0u;
  uint8_t label_source = 0u;
  bool label_prefilled = false;
  // Confirmed transform source, recorded when the family is marked verified.
  uint8_t transform_kind = 0u;
  uint32_t transform_slot = 0u;
  uint32_t transform_offset = 0u;
  uint32_t transform_stride = 0u;
  uint32_t transform_base = 0u;
  bool transform_has_prev = false;
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
  float setting_overlay_mode = 0.f;
  float setting_overlay_stride = 8.f;
  float setting_overlay_show_prev = 0.f;

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

  bool arm_active = false;
  uint32_t arm_vs_hash = 0u;
  uint32_t arm_serial = 0u;

  bool mesh_capture_pending = false;
  CapturedDraw captured;
  CameraSnapshot camera;
  DepthSource depth_source;

  std::vector<TransformCandidate> candidates;
  uint32_t selected_family = 0u;
  int selected_candidate = 0;

  std::string status;
};

inline State g_state;

inline bool CensusEnabled() { return g_state.setting_enabled > 0.5f; }

// Draw observation runs when the census or the BVH module needs it.
inline bool CaptureActive() {
  return CensusEnabled() || g_state.pool_capture_requested.load(std::memory_order_relaxed);
}

inline uint32_t OverlayMode() {
  const int mode = static_cast<int>(g_state.setting_overlay_mode + 0.5f);
  return static_cast<uint32_t>(mode < 0 ? 0 : mode);
}

inline uint32_t OverlayStride() {
  const int stride = static_cast<int>(g_state.setting_overlay_stride + 0.5f);
  return static_cast<uint32_t>(stride < 1 ? 1 : (stride > 64 ? 64 : stride));
}

inline bool OverlayShowPrev() { return g_state.setting_overlay_show_prev > 0.5f; }

inline bool IsStructuredKind(CandidateKind kind) {
  return kind == CandidateKind::Inst4x3Row || kind == CandidateKind::Inst4x3Col
         || kind == CandidateKind::Inst4x4Row || kind == CandidateKind::Inst4x4Col;
}

inline bool IsRowVectorKind(CandidateKind kind) {
  switch (kind) {
    case CandidateKind::Cb4x4Row:
    case CandidateKind::Inst4x3Row:
    case CandidateKind::Inst4x4Row:
      return true;
    default:
      return false;
  }
}

inline uint32_t KindMatrixFloats(CandidateKind kind) {
  switch (kind) {
    case CandidateKind::Inst4x3Row:
    case CandidateKind::Inst4x3Col:
      return 12u;
    default:
      return 16u;
  }
}

inline const char* CandidateKindName(CandidateKind kind) {
  switch (kind) {
    case CandidateKind::Cb4x4Row: return "CB 4x4 (row)";
    case CandidateKind::Cb4x4Col: return "CB 4x4 (col)";
    case CandidateKind::Inst4x3Row: return "Instance 4x3 (row)";
    case CandidateKind::Inst4x3Col: return "Instance 4x3 (col)";
    case CandidateKind::Inst4x4Row: return "Instance 4x4 (row)";
    case CandidateKind::Inst4x4Col: return "Instance 4x4 (col)";
    default: return "?";
  }
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
