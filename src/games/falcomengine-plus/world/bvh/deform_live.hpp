#pragma once

// Deforming meshes in the BVH (path 1, round S2: characters and water).
//
// Every frame, each camera-view draw of a deforming vertex shader whose
// world-position output the stream-out probe confirmed (deform_probe.hpp) is
// re-issued once more with a position-only stream-output geometry shader:
// the shader's own world-space triangles (three float3 per triangle, in draw
// order) land in a GPU arena. Nothing is read back on the per-frame path.
//
// Identity. A draw identity is its VS pipeline, buffers, offsets and index
// range (DeformDrawKey). The same identity emits the same triangles in the
// same order every frame, only their positions move. The first time an
// identity is captured its triangles are copied to a staging ring and read
// two presents later; its BLAS topology is built once on the CPU from that
// pose (bvh_build.hpp, the static builder) and uploaded with a refit order
// (nodes grouped by depth, deepest first).
//
// Every present, before the trace, world_bvh_refit recomputes the bounds of
// every BLAS node of the identities captured this frame from this frame's
// triangles (one thread group per object, one depth level at a time), and the
// trace walks those objects beside the static TLAS (world_bvh_trace.hlsli,
// dynamic objects). Identities not captured this frame are not traced.
//
// Draw direct, indexed triangle lists only (characters, water); indirect
// deforming draws (wind foliage, billboards) change size every frame and are
// alpha-tested: they belong to the alpha-tested geometry path.
//
// Lock rule (bvh_pool.hpp): captures are reserved under g_deform_live.mutex
// and recorded after it is released; no graphics call happens under it.

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <mutex>
#include <numeric>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <include/reshade.hpp>

#include "../../../../utils/log.hpp"
#include "../debug/pool_stage.hpp"
#include "../world_state.hpp"
#include "bvh_build.hpp"
#include "bvh_resources.hpp"
#include "deform_probe.hpp"
#include "gpu_timer.hpp"

namespace falcom_world::bvh {

inline constexpr uint64_t kDynamicArenaBytes = 24ull * 1024ull * 1024ull;  // captured float3 vertices per frame
inline constexpr uint32_t kDynamicVertexBytes = 12u;
inline constexpr uint32_t kDynamicMaxObjects = 256u;
inline constexpr uint32_t kDynamicTopologySlots = 3u;  // read back two presents later
inline constexpr uint64_t kDynamicTopologySlotBytes = 4ull * 1024ull * 1024ull;
inline constexpr uint32_t kDynamicMaxDepth = 64u;      // trace stack size and refit levels
inline constexpr uint32_t kDynamicRetireFrames = 600u;
inline constexpr uint64_t kDynamicResetGarbageBytes = 8ull * 1024ull * 1024ull;
inline constexpr uint64_t kDynamicArenaMinBytes = 256ull * 1024ull;  // first size of a BLAS arena
inline constexpr uint32_t kDynamicRefitGroupSize = 64u;  // world_bvh_refit numthreads
inline constexpr uint32_t kDynamicInstanceFlag = 0x80000000u;  // trace hit.instance of a dynamic object
inline constexpr uint32_t kDynamicMeshMarker = 0xFFFFFFFEu;    // trace hit.mesh of a dynamic object

// Must match DynamicObjectGPU in world_bvh_types.hlsli.
struct DynamicObjectGPU {
  uint32_t node_offset = 0u;    // first node in the dynamic node arena
  uint32_t leaf_offset = 0u;    // first leaf in the dynamic leaf arena
  uint32_t node_count = 0u;
  uint32_t vertex_base = 0u;    // first captured vertex (float3) of this frame
  uint32_t triangle_count = 0u;
  uint32_t refit_offset = 0u;   // node order (deepest level first) in the refit arena
  uint32_t level_offset = 0u;   // level_count + 1 level starts in the refit arena (relative to refit_offset)
  uint32_t level_count = 0u;
};
static_assert(sizeof(DynamicObjectGPU) == 32u, "DynamicObjectGPU layout must match world_bvh_types.hlsli");

enum class DynamicSkip : uint8_t {
  None = 0,
  NotConfirmed,        // the probe has not confirmed this shader's world position (yet)
  LightView,
  NotIndexed,
  Topology,
  Deferred,
  Duplicate,           // the same identity was already captured this frame
  ArenaFull,
  NoResources,
  ShaderCreateFailed,
  GameState,           // the game had a GS/HS/DS or SO target bound
  Count,
};

inline const char* DynamicSkipName(DynamicSkip skip) {
  switch (skip) {
    case DynamicSkip::NotConfirmed:       return "not_confirmed";
    case DynamicSkip::LightView:          return "light_view";
    case DynamicSkip::NotIndexed:         return "not_indexed";
    case DynamicSkip::Topology:           return "topology";
    case DynamicSkip::Deferred:           return "deferred";
    case DynamicSkip::Duplicate:          return "duplicate";
    case DynamicSkip::ArenaFull:          return "arena_full";
    case DynamicSkip::NoResources:        return "no_resources";
    case DynamicSkip::ShaderCreateFailed: return "shader_create_failed";
    case DynamicSkip::GameState:          return "game_state";
    default:                              return "none";
  }
}

// Draw identity of a captured deforming draw.
inline uint64_t DeformDrawKey(const DrawRecord& draw) {
  uint64_t key = 1469598103934665603ull;
  key = PoolMix(key, draw.vs_pipeline);
  key = PoolMix(key, draw.vb.handle);
  key = PoolMix(key, draw.vb_offset);
  key = PoolMix(key, draw.vb_stride);
  key = PoolMix(key, draw.ib.handle);
  key = PoolMix(key, draw.ib_offset);
  key = PoolMix(key, draw.index_size);
  key = PoolMix(key, draw.first_index);
  key = PoolMix(key, draw.index_count);
  key = PoolMix(key, static_cast<uint64_t>(static_cast<int64_t>(draw.vertex_offset)));
  key = PoolMix(key, draw.instance_count == 0u ? 1u : draw.instance_count);
  return PoolMix(key, draw.first_instance);
}

// The probe's confirmed world-position output of a VS pipeline: matched at
// least once and never two different outputs.
struct DeformConfirmed {
  bool confirmed = false;
  uint32_t hash = 0u;
  DeformSoElement element;
};

inline DeformConfirmed LookupDeformConfirmed(uint64_t pipeline) {
  DeformConfirmed result;
  std::lock_guard<std::mutex> lock(g_deform.mutex);
  const auto it = g_deform.shaders.find(pipeline);
  if (it == g_deform.shaders.end()) return result;
  const DeformShader& shader = it->second;
  if (shader.matched == 0u || shader.inconsistent || shader.chosen < 0
      || static_cast<size_t>(shader.chosen) >= shader.layout.elements.size()) {
    return result;
  }
  result.confirmed = true;
  result.hash = shader.hash;
  result.element = shader.layout.elements[static_cast<size_t>(shader.chosen)];
  return result;
}

// Position-only stream-output layout: xyz of the confirmed element.
inline DeformSoLayout DynamicCaptureLayout(const DeformSoElement& element) {
  DeformSoLayout layout;
  DeformSoElement position = element;
  position.start = 0u;
  position.count = 3u;
  position.offset = 0u;
  position.position = false;
  position.candidate = true;
  layout.elements.push_back(position);
  layout.stride = kDynamicVertexBytes;
  layout.position = -1;
  layout.candidates = 1u;
  return layout;
}

// Node order for the refit: every node of a tree grouped by depth, deepest
// level first. `level_starts` has level_count + 1 entries into `order`.
// Returns false when the tree is deeper than kDynamicMaxDepth.
inline bool BuildRefitOrder(const std::vector<BVHNodeGPU>& nodes, std::vector<uint32_t>* order,
                            std::vector<uint32_t>* level_starts) {
  order->clear();
  level_starts->clear();
  if (nodes.empty()) return true;
  std::vector<uint32_t> depth(nodes.size(), 0u);
  uint32_t max_depth = 0u;
  // Parents precede their children (bvh_build.hpp), so one forward pass sets every depth.
  for (uint32_t i = 0; i < nodes.size(); ++i) {
    const BVHNodeGPU& node = nodes[i];
    if ((node.child_or_leaf & kBvhLeafFlag) != 0u) continue;
    if (node.child_or_leaf >= nodes.size() || node.sibling_or_right >= nodes.size()
        || node.child_or_leaf <= i || node.sibling_or_right <= i) {
      return false;
    }
    depth[node.child_or_leaf] = depth[i] + 1u;
    depth[node.sibling_or_right] = depth[i] + 1u;
    max_depth = (std::max)(max_depth, depth[i] + 1u);
  }
  if (max_depth + 1u > kDynamicMaxDepth) return false;
  const uint32_t levels = max_depth + 1u;
  std::vector<uint32_t> counts(levels, 0u);
  for (const uint32_t d : depth) counts[max_depth - d] += 1u;
  level_starts->resize(levels + 1u, 0u);
  for (uint32_t l = 0; l < levels; ++l) (*level_starts)[l + 1u] = (*level_starts)[l] + counts[l];
  order->resize(nodes.size());
  std::vector<uint32_t> fill(level_starts->begin(), level_starts->end() - 1);
  for (uint32_t i = 0; i < nodes.size(); ++i) (*order)[fill[max_depth - depth[i]]++] = i;
  return true;
}

// CPU reference of world_bvh_refit for one object (the GPU shader mirrors it):
// recomputes node bounds from captured triangles (three float3 per triangle).
inline void RefitDynamicNodes(std::vector<BVHNodeGPU>* nodes, const std::vector<BVHLeafGPU>& leaves,
                              const std::vector<uint32_t>& order, const std::vector<uint32_t>& level_starts,
                              const float* vertices, uint32_t triangle_count) {
  for (size_t level = 0; level + 1u < level_starts.size(); ++level) {
    for (uint32_t e = level_starts[level]; e < level_starts[level + 1u]; ++e) {
      BVHNodeGPU& node = (*nodes)[order[e]];
      float lo[3];
      float hi[3];
      if ((node.child_or_leaf & kBvhLeafFlag) != 0u) {
        const uint32_t prim = leaves[node.child_or_leaf & ~kBvhLeafFlag].prim;
        if (prim >= triangle_count) continue;
        const float* a = vertices + static_cast<size_t>(prim) * 9u;
        for (int k = 0; k < 3; ++k) {
          lo[k] = (std::min)({a[k], a[3 + k], a[6 + k]});
          hi[k] = (std::max)({a[k], a[3 + k], a[6 + k]});
        }
      } else {
        const BVHNodeGPU& left = (*nodes)[node.child_or_leaf];
        const BVHNodeGPU& right = (*nodes)[node.sibling_or_right];
        for (int k = 0; k < 3; ++k) {
          lo[k] = (std::min)(left.bounds_min[k], right.bounds_min[k]);
          hi[k] = (std::max)(left.bounds_max[k], right.bounds_max[k]);
        }
      }
      for (int k = 0; k < 3; ++k) {
        node.bounds_min[k] = lo[k];
        node.bounds_max[k] = hi[k];
      }
    }
  }
}

// ---------------------------------------------------------------------------
// State.

// GPU buffer with an SRV and (optionally) a UAV; grows by copy on the GPU.
struct DynamicArena {
  reshade::api::resource buffer = {0u};
  reshade::api::resource_view srv = {0u};
  reshade::api::resource_view uav = {0u};
  uint32_t stride = 0u;
  bool writable = false;   // UAV for the refit
  uint64_t capacity = 0u;  // elements
  uint64_t used = 0u;      // elements
};

struct DynamicIdentity {
  uint64_t vs_pipeline = 0u;
  uint32_t vs_hash = 0u;
  uint32_t triangles = 0u;
  uint32_t first_frame = 0u;
  uint32_t last_frame = 0u;
  uint32_t captures = 0u;
  bool topology_pending = false;  // a readback is in flight
  bool resident = false;          // BLAS uploaded
  bool failed = false;            // no usable BLAS (reason kept)
  std::string failure;
  uint32_t node_offset = 0u;
  uint32_t node_count = 0u;
  uint32_t leaf_offset = 0u;
  uint32_t refit_offset = 0u;
  uint32_t level_offset = 0u;
  uint32_t level_count = 0u;
  uint64_t gpu_bytes = 0u;  // arena bytes (garbage once retired)
  float bbox_min[3] = {};   // at the topology capture
  float bbox_max[3] = {};
  uint64_t vb = 0u;  // draw components at its first capture (diagnostic)
  uint64_t ib = 0u;
  uint64_t vb_offset = 0u;
  uint64_t ib_offset = 0u;
  uint32_t index_count = 0u;
  uint32_t instance_count = 0u;
  uint32_t first_instance = 0u;
};

struct DynamicCapture {
  uint64_t key = 0u;
  uint64_t vs_pipeline = 0u;
  uint32_t vs_hash = 0u;
  uint32_t vertex_base = 0u;  // float3 vertices into the arena
  uint32_t triangles = 0u;
  uint64_t vb = 0u;  // draw components at the capture (diagnostic)
  uint64_t ib = 0u;
  uint64_t vb_offset = 0u;
  uint64_t ib_offset = 0u;
  uint32_t index_count = 0u;
  uint32_t instance_count = 0u;
  uint32_t first_instance = 0u;
};

struct DynamicCaptureShader {
  uint32_t hash = 0u;
  uint64_t so_shader = 0u;
  bool failed = false;
  int32_t hr = 0;
};

struct DynamicTopologyRequest {
  uint64_t key = 0u;
  uint64_t staging_offset = 0u;
  uint32_t triangles = 0u;
};

struct DynamicTopologySlot {
  reshade::api::resource staging = {0u};
  uint64_t used = 0u;
  std::vector<DynamicTopologyRequest> requests;
};

// The object list of the last present (for the trace and the inspect tool).
struct DynamicObjectInfo {
  uint64_t key = 0u;
  uint32_t vs_hash = 0u;
  uint32_t triangles = 0u;
  uint32_t captures = 0u;
  uint32_t first_frame = 0u;
  float bbox_min[3] = {};  // topology pose
  float bbox_max[3] = {};
};

struct DeformLiveStats {
  uint32_t frame_captures = 0u;   // last present
  uint32_t frame_objects = 0u;    // traced at the last present
  uint32_t frame_pending = 0u;    // captured, BLAS not built yet
  uint64_t frame_vertices = 0u;
  uint64_t captures = 0u;
  uint32_t identities = 0u;
  uint32_t resident = 0u;
  uint32_t failed = 0u;
  uint32_t topology_reads = 0u;
  uint32_t topology_mismatch = 0u;  // read size did not match the identity
  uint32_t blas_built = 0u;
  uint32_t retired = 0u;
  uint32_t recreated = 0u;  // a key seen again after it was retired
  uint32_t resets = 0u;
  uint32_t resource_failures = 0u;
  uint32_t map_failures = 0u;
  uint32_t object_cap_drops = 0u;
  uint64_t garbage_bytes = 0u;
  uint64_t used_bytes = 0u;
  std::array<uint64_t, static_cast<size_t>(DynamicSkip::Count)> skips = {};
  std::string last_failure;
};

struct DeformLiveState {
  std::atomic_bool enabled{true};
  std::mutex mutex;
  reshade::api::device* device = nullptr;
  // Per-frame capture arena (vertices are written by stream output).
  reshade::api::resource arena = {0u};
  reshade::api::resource_view arena_srv = {0u};
  uint64_t arena_used = 0u;  // bytes, this frame
  std::vector<DynamicCapture> captures;  // this frame
  std::unordered_set<uint64_t> frame_keys;
  uint32_t capture_frame = 0u;
  std::unordered_map<uint64_t, DynamicCaptureShader> shaders;  // by VS pipeline
  std::vector<uint64_t> retired_so_shaders;
  std::unordered_map<uint64_t, DynamicIdentity> identities;   // by DeformDrawKey
  std::unordered_set<uint64_t> retired_keys;  // identities retired since (recreated count)
  // Topology readback ring.
  std::array<DynamicTopologySlot, kDynamicTopologySlots> topology;
  uint32_t topology_write = 0u;
  // BLAS store.
  DynamicArena nodes;
  DynamicArena leaves;
  DynamicArena refit;
  // Objects of the last present.
  reshade::api::resource objects_buffer = {0u};
  reshade::api::resource_view objects_srv = {0u};
  uint32_t object_count = 0u;
  std::vector<DynamicObjectInfo> object_info;
  // Refit pass.
  reshade::api::pipeline_layout refit_layout = {0u};
  reshade::api::descriptor_table refit_srv_table = {0u};
  reshade::api::descriptor_table refit_uav_table = {0u};
  reshade::api::pipeline refit_pipeline = {0u};
  bool refit_failed = false;
  GpuTimer refit_timer;
  std::atomic_uint64_t cpu_us{0u};  // CPU time of OnDeformCaptureDraw and UpdateDeformLive (PoolCpuTimer)
  std::atomic_uint64_t cpu_frame_us{0u};  // cpu_us of the last frame (moved by UpdateDeformLive)
  uint32_t log_lines = 0u;          // identity and store reset lines written (kPoolLifecycleLogLines)
  DeformLiveStats stats;
};

inline DeformLiveState g_deform_live;

// What the trace binds (bvh_trace.hpp): null views and count 0 when off.
struct DynamicTraceInputs {
  reshade::api::resource_view vertices = {0u};
  reshade::api::resource_view objects = {0u};
  reshade::api::resource_view nodes = {0u};
  reshade::api::resource_view leaves = {0u};
  uint32_t count = 0u;
};

inline DynamicTraceInputs GetDynamicTraceInputs(reshade::api::device* device) {
  DynamicTraceInputs inputs;
  std::lock_guard<std::mutex> lock(g_deform_live.mutex);
  if (!g_deform_live.enabled.load(std::memory_order_relaxed) || g_deform_live.device != device) return inputs;
  if (g_deform_live.object_count == 0u) return inputs;
  inputs.vertices = g_deform_live.arena_srv;
  inputs.objects = g_deform_live.objects_srv;
  inputs.nodes = g_deform_live.nodes.srv;
  inputs.leaves = g_deform_live.leaves.srv;
  inputs.count = g_deform_live.object_count;
  return inputs;
}

// ---------------------------------------------------------------------------
// Draw time.

inline void CountDynamicSkip(DynamicSkip skip) { g_deform_live.stats.skips[static_cast<size_t>(skip)] += 1u; }

// Draw event, after the probe. Captures the draw's world-space triangles.
inline void OnDeformCaptureDraw(
    reshade::api::device* device, reshade::api::command_list* cmd_list, const DrawRecord& draw) {
  const PoolCpuTimer cpu_timer{&g_deform_live.cpu_us};
  if (!g_deform_live.enabled.load(std::memory_order_relaxed)) return;
  if (device == nullptr || cmd_list == nullptr) return;
  const contract::ShaderTraits traits = contract::LookupVertexTraits(draw.vs_pipeline);
  if (!contract::IsDeformingClass(static_cast<contract::VsClass>(traits.cls))) return;
  PoolStageScope stage("deform capture: reserve");
  const auto count = [](DynamicSkip skip) {
    std::lock_guard<std::mutex> lock(g_deform_live.mutex);
    CountDynamicSkip(skip);
  };
  if ((traits.flags & contract::kTraitCameraView) == 0u) return count(DynamicSkip::LightView);
  if (draw.method != 1u || !draw.has_index_buffer) return count(DynamicSkip::NotIndexed);
  if ((draw.topology != reshade::api::primitive_topology::undefined
       && draw.topology != reshade::api::primitive_topology::triangle_list)
      || draw.index_count < 3u) {
    return count(DynamicSkip::Topology);
  }
  if (IsPoolDeferredList(cmd_list)) return count(DynamicSkip::Deferred);
  const DeformConfirmed confirmed = LookupDeformConfirmed(draw.vs_pipeline);
  if (!confirmed.confirmed) return count(DynamicSkip::NotConfirmed);
  const std::shared_ptr<const contract::ShaderCode> code = contract::LookupDeformingCode(draw.vs_pipeline);
  if (code == nullptr || code->hash != confirmed.hash) return count(DynamicSkip::NotConfirmed);

  const uint64_t key = DeformDrawKey(draw);
  const uint32_t instances = draw.instance_count == 0u ? 1u : draw.instance_count;
  const uint64_t triangles = static_cast<uint64_t>(draw.index_count / 3u) * instances;
  const uint64_t bytes = triangles * 3u * kDynamicVertexBytes;

  bool create = false;
  uint64_t so_shader = 0u;
  reshade::api::resource arena = {0u};
  uint64_t offset = 0u;
  {
    std::lock_guard<std::mutex> lock(g_deform_live.mutex);
    if (g_deform_live.capture_frame != draw.frame) {
      // A frame whose present did not run (or the first): start over.
      g_deform_live.capture_frame = draw.frame;
      g_deform_live.captures.clear();
      g_deform_live.frame_keys.clear();
      g_deform_live.arena_used = 0u;
    }
    DynamicCaptureShader& shader = g_deform_live.shaders[draw.vs_pipeline];
    if (shader.hash != confirmed.hash) {
      if (shader.so_shader != 0u) g_deform_live.retired_so_shaders.push_back(shader.so_shader);
      shader = DynamicCaptureShader{};
      shader.hash = confirmed.hash;
    }
    if (shader.failed) return CountDynamicSkip(DynamicSkip::ShaderCreateFailed);
    if (g_deform_live.device != device || g_deform_live.arena.handle == 0u) return CountDynamicSkip(DynamicSkip::NoResources);
    if (g_deform_live.frame_keys.count(key) != 0u) return CountDynamicSkip(DynamicSkip::Duplicate);
    if (shader.so_shader == 0u) {
      create = true;
    } else {
      if (g_deform_live.captures.size() >= kDynamicMaxObjects || g_deform_live.arena_used + bytes > kDynamicArenaBytes) {
        return CountDynamicSkip(DynamicSkip::ArenaFull);
      }
      so_shader = shader.so_shader;
      arena = g_deform_live.arena;
      offset = g_deform_live.arena_used;
      g_deform_live.arena_used += bytes;  // reserved; released only by the next frame
    }
  }

  if (create) {
    stage.Set("deform capture: create shader");
    int32_t hr = 0;
    const uint64_t created = g_deform.backend.create_so_shader != nullptr
                                 ? g_deform.backend.create_so_shader(device, *code, DynamicCaptureLayout(confirmed.element), &hr)
                                 : 0u;
    std::lock_guard<std::mutex> lock(g_deform_live.mutex);
    DynamicCaptureShader& shader = g_deform_live.shaders[draw.vs_pipeline];
    shader.hr = hr;
    if (created == 0u) {
      shader.failed = true;
      g_deform_live.stats.last_failure = "capture shader creation failed";
      CountDynamicSkip(DynamicSkip::ShaderCreateFailed);
    } else if (shader.so_shader != 0u || shader.hash != confirmed.hash) {
      g_deform_live.retired_so_shaders.push_back(created);
    } else {
      shader.so_shader = created;
    }
    return;  // captured from the next draw of this shader on
  }

  stage.Set("deform capture: issue");
  const DeformSkip state =
      g_deform.backend.check_game_state != nullptr ? g_deform.backend.check_game_state(cmd_list) : DeformSkip::None;
  if (state == DeformSkip::None) {
    using reshade::api::pipeline_stage;
    const auto stages = pipeline_stage::geometry_shader | pipeline_stage::stream_output;
    const uint64_t no_offset = 0u;
    const reshade::api::resource no_buffer = {0u};
    cmd_list->bind_pipeline(stages, reshade::api::pipeline{so_shader});
    cmd_list->bind_stream_output_buffers(0u, 1u, &arena, &offset, nullptr, nullptr, nullptr);
    cmd_list->draw_indexed(draw.index_count, instances, draw.first_index, draw.vertex_offset, draw.first_instance);
    cmd_list->bind_stream_output_buffers(0u, 1u, &no_buffer, &no_offset, nullptr, nullptr, nullptr);
    cmd_list->bind_pipeline(stages, reshade::api::pipeline{0u});
  }
  std::lock_guard<std::mutex> lock(g_deform_live.mutex);
  if (state != DeformSkip::None) return CountDynamicSkip(DynamicSkip::GameState);
  if (g_deform_live.capture_frame != draw.frame) return;  // the frame was reset meanwhile
  DynamicCapture capture;
  capture.key = key;
  capture.vs_pipeline = draw.vs_pipeline;
  capture.vs_hash = confirmed.hash;
  capture.vertex_base = static_cast<uint32_t>(offset / kDynamicVertexBytes);
  capture.triangles = static_cast<uint32_t>(triangles);
  capture.vb = draw.vb.handle;
  capture.ib = draw.ib.handle;
  capture.vb_offset = draw.vb_offset;
  capture.ib_offset = draw.ib_offset;
  capture.index_count = draw.index_count;
  capture.instance_count = instances;
  capture.first_instance = draw.first_instance;
  g_deform_live.captures.push_back(capture);
  g_deform_live.frame_keys.insert(key);
  g_deform_live.stats.captures += 1u;
}

// ---------------------------------------------------------------------------
// Resources.

inline void DestroyDynamicArena(reshade::api::device* device, DynamicArena* arena) {
  if (arena->uav.handle != 0u) device->destroy_resource_view(arena->uav);
  DestroyBuffer(device, &arena->srv, &arena->buffer);
  arena->uav = {0u};
  arena->capacity = 0u;
  arena->used = 0u;
}

// Structured buffer with an SRV (and a UAV when writable).
inline bool CreateDynamicBuffer(reshade::api::device* device, uint64_t elements, uint32_t stride, bool writable,
                                DynamicArena* out) {
  using reshade::api::resource_usage;
  reshade::api::resource_desc desc = {};
  desc.type = reshade::api::resource_type::buffer;
  desc.buffer.size = elements * stride;
  desc.buffer.stride = stride;
  desc.heap = reshade::api::memory_heap::gpu_only;
  desc.usage = resource_usage::shader_resource | resource_usage::copy_source | resource_usage::copy_dest;
  if (writable) desc.usage |= resource_usage::unordered_access;
  reshade::api::resource buffer = {0u};
  if (!device->create_resource(desc, nullptr, resource_usage::shader_resource, &buffer)) return false;
  const reshade::api::resource_view_desc view_desc(reshade::api::resource_view_type::buffer,
                                                   reshade::api::format::unknown, 0, elements);
  reshade::api::resource_view srv = {0u};
  reshade::api::resource_view uav = {0u};
  if (!device->create_resource_view(buffer, resource_usage::shader_resource, view_desc, &srv)
      || (writable && !device->create_resource_view(buffer, resource_usage::unordered_access, view_desc, &uav))) {
    if (srv.handle != 0u) device->destroy_resource_view(srv);
    device->destroy_resource(buffer);
    return false;
  }
  out->buffer = buffer;
  out->srv = srv;
  out->uav = uav;
  out->capacity = elements;
  return true;
}

inline bool EnsureDynamicArena(reshade::api::device* device, reshade::api::command_list* cmd_list, DynamicArena* arena,
                               uint64_t needed) {
  if (needed <= arena->capacity) return true;
  const uint64_t min_elements = (kDynamicArenaMinBytes + arena->stride - 1u) / arena->stride;
  const uint64_t capacity = (std::max)({needed, arena->capacity * 2u, min_elements});
  DynamicArena grown;
  grown.stride = arena->stride;
  grown.writable = arena->writable;
  if (!CreateDynamicBuffer(device, capacity, arena->stride, arena->writable, &grown)) return false;
  if (arena->used != 0u) cmd_list->copy_buffer_region(arena->buffer, 0u, grown.buffer, 0u, arena->used * arena->stride);
  grown.used = arena->used;
  DestroyDynamicArena(device, arena);
  *arena = grown;
  return true;
}

inline void DestroyDynamicResources(reshade::api::device* device, DeformLiveState& state) {
  if (state.arena_srv.handle != 0u) device->destroy_resource_view(state.arena_srv);
  if (state.arena.handle != 0u) device->destroy_resource(state.arena);
  state.arena_srv = {0u};
  state.arena = {0u};
  for (DynamicTopologySlot& slot : state.topology) {
    if (slot.staging.handle != 0u) device->destroy_resource(slot.staging);
    slot = DynamicTopologySlot{};
  }
  DestroyDynamicArena(device, &state.nodes);
  DestroyDynamicArena(device, &state.leaves);
  DestroyDynamicArena(device, &state.refit);
  DestroyBuffer(device, &state.objects_srv, &state.objects_buffer);
  if (state.refit_pipeline.handle != 0u) device->destroy_pipeline(state.refit_pipeline);
  if (state.refit_srv_table.handle != 0u) device->free_descriptor_table(state.refit_srv_table);
  if (state.refit_uav_table.handle != 0u) device->free_descriptor_table(state.refit_uav_table);
  if (state.refit_layout.handle != 0u) device->destroy_pipeline_layout(state.refit_layout);
  state.refit_pipeline = {0u};
  state.refit_srv_table = {0u};
  state.refit_uav_table = {0u};
  state.refit_layout = {0u};
  state.refit_failed = false;
  DestroyGpuTimer(&state.refit_timer);
}

// Creates the per-device buffers (arena, topology staging, objects). The BLAS
// arenas grow on demand.
inline bool CreateDynamicResources(reshade::api::device* device, DeformLiveState& state) {
  using reshade::api::memory_heap;
  using reshade::api::resource_usage;
  state.nodes.stride = sizeof(BVHNodeGPU);
  state.nodes.writable = true;
  state.leaves.stride = sizeof(BVHLeafGPU);
  state.refit.stride = sizeof(uint32_t);
  bool ok = device->create_resource(
      reshade::api::resource_desc(kDynamicArenaBytes, memory_heap::gpu_only,
                                  resource_usage::stream_output | resource_usage::shader_resource | resource_usage::copy_source),
      nullptr, resource_usage::shader_resource, &state.arena);
  ok = ok && device->create_resource_view(
                 state.arena, resource_usage::shader_resource,
                 reshade::api::resource_view_desc(reshade::api::resource_view_type::buffer, reshade::api::format::r32_float,
                                                  0, kDynamicArenaBytes / sizeof(float)),
                 &state.arena_srv);
  for (DynamicTopologySlot& slot : state.topology) {
    ok = ok && device->create_resource(
                   reshade::api::resource_desc(kDynamicTopologySlotBytes, memory_heap::gpu_to_cpu, resource_usage::copy_dest),
                   nullptr, resource_usage::copy_dest, &slot.staging);
  }
  DynamicArena objects;
  ok = ok && CreateDynamicBuffer(device, kDynamicMaxObjects, sizeof(DynamicObjectGPU), false, &objects);
  if (ok) {
    state.objects_buffer = objects.buffer;
    state.objects_srv = objects.srv;
  }
  if (!ok) DestroyDynamicResources(device, state);
  return ok;
}

inline bool EnsureDynamicRefitPipeline(reshade::api::device* device, DeformLiveState& state) {
  if (state.refit_pipeline.handle != 0u) return true;
  if (state.refit_failed) return false;
#if defined(__world_bvh_refit_EMBED_FILE)
  using DR = reshade::api::descriptor_range;
  using DS = reshade::api::shader_stage;
  using DT = reshade::api::descriptor_type;
  using P = reshade::api::pipeline_layout_param;
  DR srv_range = {0, 0, 0, 4, DS::all_compute, 1, DT::shader_resource_view};
  DR uav_range = {0, 0, 0, 1, DS::all_compute, 1, DT::unordered_access_view};
  reshade::api::constant_range push_range = {};
  push_range.binding = 0;
  push_range.dx_register_index = 13;
  push_range.dx_register_space = 0;
  push_range.count = 4;
  push_range.visibility = DS::all_compute;
  P params[3] = {};
  params[0].type = reshade::api::pipeline_layout_param_type::descriptor_table;
  params[0].descriptor_table.count = 1;
  params[0].descriptor_table.ranges = &srv_range;
  params[1].type = reshade::api::pipeline_layout_param_type::descriptor_table;
  params[1].descriptor_table.count = 1;
  params[1].descriptor_table.ranges = &uav_range;
  params[2].type = reshade::api::pipeline_layout_param_type::push_constants;
  params[2].push_constants = push_range;
  reshade::api::shader_desc shader = {};
  shader.code = __world_bvh_refit.data();
  shader.code_size = __world_bvh_refit.size();
  shader.entry_point = "main";
  const reshade::api::pipeline_subobject subobject = {reshade::api::pipeline_subobject_type::compute_shader, 1, &shader};
  const bool ok = device->create_pipeline_layout(3, params, &state.refit_layout)
                  && device->allocate_descriptor_table(state.refit_layout, 0, &state.refit_srv_table)
                  && device->allocate_descriptor_table(state.refit_layout, 1, &state.refit_uav_table)
                  && device->create_pipeline(state.refit_layout, 1, &subobject, &state.refit_pipeline);
  if (ok) return true;
  state.refit_failed = true;
  state.stats.last_failure = "refit pipeline creation failed";
  return false;
#else
  (void)device;
  state.refit_failed = true;
  state.stats.last_failure = "refit shader not embedded";
  return false;
#endif
}

// Records the refit dispatch for the objects uploaded this present, then
// unbinds the node UAV so the trace can read the nodes as an SRV.
inline void DispatchDynamicRefit(reshade::api::device* device, reshade::api::command_list* cmd_list,
                                 DeformLiveState& state, uint32_t object_count) {
  if (object_count == 0u || !EnsureDynamicRefitPipeline(device, state)) return;
  reshade::api::resource_view srvs[4] = {state.arena_srv, state.objects_srv, state.leaves.srv, state.refit.srv};
  reshade::api::descriptor_table_update srv_update = {
      state.refit_srv_table, 0, 0, 4, reshade::api::descriptor_type::shader_resource_view, srvs};
  device->update_descriptor_tables(1, &srv_update);
  reshade::api::resource_view uav = state.nodes.uav;
  reshade::api::descriptor_table_update uav_update = {
      state.refit_uav_table, 0, 0, 1, reshade::api::descriptor_type::unordered_access_view, &uav};
  device->update_descriptor_tables(1, &uav_update);
  cmd_list->bind_pipeline(reshade::api::pipeline_stage::all_compute, state.refit_pipeline);
  const reshade::api::descriptor_table tables[2] = {state.refit_srv_table, state.refit_uav_table};
  cmd_list->bind_descriptor_tables(reshade::api::shader_stage::all_compute, state.refit_layout, 0, 2, tables);
  const uint32_t constants[4] = {object_count, 0u, 0u, 0u};
  cmd_list->push_constants(reshade::api::shader_stage::all_compute, state.refit_layout, 2, 0, 4, constants);
  BeginGpuTimer(device, cmd_list, &state.refit_timer);
  cmd_list->dispatch(object_count, 1u, 1u);
  EndGpuTimer(cmd_list, &state.refit_timer);
  reshade::api::resource_view no_uav = {0u};
  reshade::api::descriptor_table_update clear = {
      state.refit_uav_table, 0, 0, 1, reshade::api::descriptor_type::unordered_access_view, &no_uav};
  device->update_descriptor_tables(1, &clear);
  cmd_list->bind_descriptor_tables(reshade::api::shader_stage::all_compute, state.refit_layout, 1, 1,
                                   &state.refit_uav_table);
}

// ---------------------------------------------------------------------------
// Present.

inline void ReleaseDeformLive(reshade::api::device* device) {
  std::vector<uint64_t> shaders;
  DeformLiveState snapshot_holder;  // resources moved out under the lock
  reshade::api::device* owner = nullptr;
  {
    std::lock_guard<std::mutex> lock(g_deform_live.mutex);
    if (device != nullptr && g_deform_live.device != device) return;
    owner = g_deform_live.device;
    for (auto& [pipeline, shader] : g_deform_live.shaders) {
      (void)pipeline;
      if (shader.so_shader != 0u) shaders.push_back(shader.so_shader);
    }
    shaders.insert(shaders.end(), g_deform_live.retired_so_shaders.begin(), g_deform_live.retired_so_shaders.end());
    g_deform_live.retired_so_shaders.clear();
    g_deform_live.shaders.clear();
    snapshot_holder.arena = g_deform_live.arena;
    snapshot_holder.arena_srv = g_deform_live.arena_srv;
    snapshot_holder.topology = g_deform_live.topology;
    snapshot_holder.nodes = g_deform_live.nodes;
    snapshot_holder.leaves = g_deform_live.leaves;
    snapshot_holder.refit = g_deform_live.refit;
    snapshot_holder.objects_buffer = g_deform_live.objects_buffer;
    snapshot_holder.objects_srv = g_deform_live.objects_srv;
    snapshot_holder.refit_layout = g_deform_live.refit_layout;
    snapshot_holder.refit_srv_table = g_deform_live.refit_srv_table;
    snapshot_holder.refit_uav_table = g_deform_live.refit_uav_table;
    snapshot_holder.refit_pipeline = g_deform_live.refit_pipeline;
    g_deform_live.arena = {0u};
    g_deform_live.arena_srv = {0u};
    g_deform_live.arena_used = 0u;
    g_deform_live.topology = {};
    g_deform_live.nodes = DynamicArena{};
    g_deform_live.leaves = DynamicArena{};
    g_deform_live.refit = DynamicArena{};
    g_deform_live.objects_buffer = {0u};
    g_deform_live.objects_srv = {0u};
    g_deform_live.refit_layout = {0u};
    g_deform_live.refit_srv_table = {0u};
    g_deform_live.refit_uav_table = {0u};
    g_deform_live.refit_pipeline = {0u};
    g_deform_live.refit_failed = false;
    snapshot_holder.refit_timer = g_deform_live.refit_timer;  // released below, outside the lock
    g_deform_live.refit_timer = GpuTimer{};
    g_deform_live.identities.clear();
    g_deform_live.retired_keys.clear();
    g_deform_live.captures.clear();
    g_deform_live.frame_keys.clear();
    g_deform_live.object_count = 0u;
    g_deform_live.object_info.clear();
    g_deform_live.device = nullptr;
  }
  if (owner != nullptr) DestroyDynamicResources(owner, snapshot_holder);
  if (g_deform.backend.destroy_so_shader != nullptr) {
    for (const uint64_t shader : shaders) g_deform.backend.destroy_so_shader(shader);
  }
}

inline void OnDestroyDeviceDeformLive(reshade::api::device* device) { ReleaseDeformLive(device); }

inline void OnDestroyPipelineDeformLive(reshade::api::device* device, reshade::api::pipeline pipeline) {
  (void)device;
  std::lock_guard<std::mutex> lock(g_deform_live.mutex);
  const auto it = g_deform_live.shaders.find(pipeline.handle);
  if (it == g_deform_live.shaders.end()) return;
  if (it->second.so_shader != 0u) g_deform_live.retired_so_shaders.push_back(it->second.so_shader);
  g_deform_live.shaders.erase(it);
}

// One identity's BLAS as built on the CPU, ready to append.
struct DynamicBuiltBlas {
  uint64_t key = 0u;
  uint32_t triangles = 0u;
  MeshBlas blas;
  std::vector<uint32_t> order;
  std::vector<uint32_t> level_starts;
  const char* error = nullptr;
  float bbox_min[3] = {1e30f, 1e30f, 1e30f};
  float bbox_max[3] = {-1e30f, -1e30f, -1e30f};
};

// Builds a BLAS from `triangles` captured triangles (three float3 each).
inline void BuildDynamicBlas(const float* vertices, uint32_t triangles, DynamicBuiltBlas* out) {
  std::vector<std::array<float, 3>> positions(static_cast<size_t>(triangles) * 3u);
  for (size_t v = 0; v < positions.size(); ++v) {
    for (int k = 0; k < 3; ++k) {
      positions[v][k] = vertices[v * 3u + k];
      if (std::isfinite(positions[v][k])) {
        out->bbox_min[k] = (std::min)(out->bbox_min[k], positions[v][k]);
        out->bbox_max[k] = (std::max)(out->bbox_max[k], positions[v][k]);
      }
    }
  }
  std::vector<uint32_t> indices(positions.size());
  std::iota(indices.begin(), indices.end(), 0u);
  out->error = BuildMeshBlas(positions, indices, &out->blas);
  if (out->error == nullptr && out->blas.skipped_triangles != 0u) out->error = "non-finite triangles in the topology pose";
  if (out->error == nullptr && !BuildRefitOrder(out->blas.nodes, &out->order, &out->level_starts)) {
    out->error = "tree deeper than the trace stack";
  }
}

// Reads the topology slot written two presents ago and uploads the BLAS of
// each identity it holds.
inline void ResolveDynamicTopology(reshade::api::device* device, reshade::api::command_list* cmd_list, uint32_t slot_index,
                                   uint32_t frame) {
  std::vector<DynamicTopologyRequest> requests;
  reshade::api::resource staging = {0u};
  uint64_t used = 0u;
  {
    std::lock_guard<std::mutex> lock(g_deform_live.mutex);
    DynamicTopologySlot& slot = g_deform_live.topology[slot_index];
    requests.swap(slot.requests);
    staging = slot.staging;
    used = slot.used;
    slot.used = 0u;
  }
  if (requests.empty()) return;
  std::vector<DynamicBuiltBlas> built(requests.size());
  void* mapped = nullptr;
  const bool mapped_ok = used != 0u && device->map_buffer_region(staging, 0u, used, reshade::api::map_access::read_only, &mapped)
                         && mapped != nullptr;
  if (mapped_ok) {
    const auto* bytes = static_cast<const uint8_t*>(mapped);
    for (size_t i = 0; i < requests.size(); ++i) {
      built[i].key = requests[i].key;
      built[i].triangles = requests[i].triangles;
      BuildDynamicBlas(reinterpret_cast<const float*>(bytes + requests[i].staging_offset), requests[i].triangles, &built[i]);
    }
    device->unmap_buffer_region(staging);
  }

  // A: which results to upload (under the lock).
  std::vector<size_t> uploads;
  {
    std::lock_guard<std::mutex> lock(g_deform_live.mutex);
    DeformLiveState& state = g_deform_live;
    if (!mapped_ok) {
      state.stats.map_failures += 1u;
      for (const DynamicTopologyRequest& request : requests) {
        const auto it = state.identities.find(request.key);
        if (it != state.identities.end()) it->second.topology_pending = false;  // read again
      }
      return;
    }
    for (size_t i = 0; i < built.size(); ++i) {
      DynamicBuiltBlas& result = built[i];
      const auto it = state.identities.find(result.key);
      if (it == state.identities.end()) continue;  // retired meanwhile
      DynamicIdentity& identity = it->second;
      identity.topology_pending = false;
      state.stats.topology_reads += 1u;
      if (identity.triangles != result.triangles) {
        state.stats.topology_mismatch += 1u;
        continue;
      }
      if (result.error != nullptr) {
        identity.failed = true;
        identity.failure = result.error;
        state.stats.last_failure = result.error;
        continue;
      }
      uploads.push_back(i);
    }
  }

  // B: append to the BLAS arenas. Graphics calls, so not under the lock; the
  // arenas are only touched on the present thread.
  struct Placed {
    size_t index;
    uint32_t node_offset;
    uint32_t leaf_offset;
    uint32_t refit_offset;
    uint64_t bytes;
  };
  std::vector<Placed> placed;
  std::string error;
  uint32_t failures = 0u;
  DeformLiveState& state = g_deform_live;
  for (const size_t i : uploads) {
    const DynamicBuiltBlas& result = built[i];
    // Refit data: the order, then the level starts (relative to the order).
    std::vector<uint32_t> refit = result.order;
    refit.insert(refit.end(), result.level_starts.begin(), result.level_starts.end());
    const uint64_t node_offset = state.nodes.used;
    const uint64_t leaf_offset = state.leaves.used;
    const uint64_t refit_offset = state.refit.used;
    const bool ok =
        EnsureDynamicArena(device, cmd_list, &state.nodes, node_offset + result.blas.nodes.size())
        && EnsureDynamicArena(device, cmd_list, &state.leaves, leaf_offset + result.blas.leaves.size())
        && EnsureDynamicArena(device, cmd_list, &state.refit, refit_offset + refit.size())
        && WriteBufferRange(device, cmd_list, state.nodes.buffer, state.nodes.stride, node_offset * state.nodes.stride,
                            result.blas.nodes.data(), result.blas.nodes.size() * sizeof(BVHNodeGPU), &error)
        && WriteBufferRange(device, cmd_list, state.leaves.buffer, state.leaves.stride, leaf_offset * state.leaves.stride,
                            result.blas.leaves.data(), result.blas.leaves.size() * sizeof(BVHLeafGPU), &error)
        && WriteBufferRange(device, cmd_list, state.refit.buffer, state.refit.stride, refit_offset * state.refit.stride,
                            refit.data(), refit.size() * sizeof(uint32_t), &error);
    if (!ok) {
      failures += 1u;
      continue;  // stays without BLAS; read again on a later capture
    }
    state.nodes.used = node_offset + result.blas.nodes.size();
    state.leaves.used = leaf_offset + result.blas.leaves.size();
    state.refit.used = refit_offset + refit.size();
    placed.push_back({i, static_cast<uint32_t>(node_offset), static_cast<uint32_t>(leaf_offset),
                      static_cast<uint32_t>(refit_offset),
                      result.blas.nodes.size() * sizeof(BVHNodeGPU) + result.blas.leaves.size() * sizeof(BVHLeafGPU)
                          + refit.size() * sizeof(uint32_t)});
  }

  // C: publish.
  std::lock_guard<std::mutex> lock(g_deform_live.mutex);
  if (failures != 0u) {
    state.stats.resource_failures += failures;
    state.stats.last_failure = error.empty() ? "BLAS arena growth failed" : error;
  }
  for (const Placed& entry : placed) {
    const DynamicBuiltBlas& result = built[entry.index];
    const auto it = state.identities.find(result.key);
    if (it == state.identities.end()) continue;
    DynamicIdentity& identity = it->second;
    std::memcpy(identity.bbox_min, result.bbox_min, sizeof(identity.bbox_min));
    std::memcpy(identity.bbox_max, result.bbox_max, sizeof(identity.bbox_max));
    identity.resident = true;
    identity.node_offset = entry.node_offset;
    identity.node_count = static_cast<uint32_t>(result.blas.nodes.size());
    identity.leaf_offset = entry.leaf_offset;
    identity.refit_offset = entry.refit_offset;
    identity.level_offset = static_cast<uint32_t>(result.order.size());
    identity.level_count = static_cast<uint32_t>(result.level_starts.size() - 1u);
    identity.gpu_bytes = entry.bytes;
    state.stats.blas_built += 1u;
  }
  (void)frame;
}

// Present, before the trace (OnWorldPresentBvh): turns this frame's captures
// into traced objects and refits their BLAS on the GPU.
inline void UpdateDeformLive(reshade::api::device* device, reshade::api::command_queue* queue) {
  const PoolCpuTimer cpu_timer{&g_deform_live.cpu_us};
  g_deform_live.cpu_frame_us = g_deform_live.cpu_us.exchange(0u);
  if (device == nullptr || queue == nullptr) return;
  if (!g_deform_live.enabled.load(std::memory_order_relaxed)) {
    if (g_deform_live.device != nullptr) ReleaseDeformLive(nullptr);
    return;
  }
  auto* cmd_list = queue->get_immediate_command_list();
  if (cmd_list == nullptr) return;
  PoolStageScope stage("present: deform live");
  const uint32_t frame = g_state.frame.load();

  std::vector<uint64_t> retired;
  bool need_resources = false;
  {
    std::lock_guard<std::mutex> lock(g_deform_live.mutex);
    retired.swap(g_deform_live.retired_so_shaders);
    need_resources = g_deform_live.device != device;
  }
  if (g_deform.backend.destroy_so_shader != nullptr) {
    for (const uint64_t shader : retired) g_deform.backend.destroy_so_shader(shader);
  }
  if (need_resources) {
    ReleaseDeformLive(nullptr);
    DeformLiveState created;
    const bool ok = CreateDynamicResources(device, created);
    std::lock_guard<std::mutex> lock(g_deform_live.mutex);
    if (!ok) {
      g_deform_live.stats.resource_failures += 1u;
      g_deform_live.stats.last_failure = "capture resources could not be created";
      return;
    }
    g_deform_live.arena = created.arena;
    g_deform_live.arena_srv = created.arena_srv;
    g_deform_live.topology = created.topology;
    g_deform_live.nodes = created.nodes;
    g_deform_live.leaves = created.leaves;
    g_deform_live.refit = created.refit;
    g_deform_live.objects_buffer = created.objects_buffer;
    g_deform_live.objects_srv = created.objects_srv;
    g_deform_live.device = device;
    g_deform_live.captures.clear();
    g_deform_live.frame_keys.clear();
    g_deform_live.arena_used = 0u;
    return;
  }

  // 1. BLAS of identities captured two presents ago.
  stage.Set("present: deform live (topology)");
  uint32_t resolve = 0u;
  {
    std::lock_guard<std::mutex> lock(g_deform_live.mutex);
    g_deform_live.topology_write = (g_deform_live.topology_write + 1u) % kDynamicTopologySlots;
    resolve = g_deform_live.topology_write;
  }
  ResolveDynamicTopology(device, cmd_list, resolve, frame);

  // 2. This frame's captures: objects with a BLAS, topology reads for new identities.
  stage.Set("present: deform live (objects)");
  std::vector<DynamicObjectGPU> objects;
  std::vector<DynamicObjectInfo> info;
  struct TopologyCopy {
    uint64_t source_offset;
    uint64_t dest_offset;
    uint64_t size;
  };
  std::vector<TopologyCopy> copies;
  reshade::api::resource arena = {0u};
  reshade::api::resource staging = {0u};
  bool reset = false;
  std::vector<std::string> identity_lines;
  {
    std::lock_guard<std::mutex> lock(g_deform_live.mutex);
    DeformLiveState& state = g_deform_live;
    arena = state.arena;
    DynamicTopologySlot& slot = state.topology[state.topology_write];
    staging = slot.staging;
    const bool this_frame = state.capture_frame == frame;
    if (this_frame) {
      for (const DynamicCapture& capture : state.captures) {
        DynamicIdentity& identity = state.identities[capture.key];
        if (identity.captures == 0u) {
          if (state.retired_keys.erase(capture.key) != 0u) state.stats.recreated += 1u;
          identity.vs_pipeline = capture.vs_pipeline;
          identity.vs_hash = capture.vs_hash;
          identity.triangles = capture.triangles;
          identity.first_frame = frame;
          identity.vb = capture.vb;
          identity.ib = capture.ib;
          identity.vb_offset = capture.vb_offset;
          identity.ib_offset = capture.ib_offset;
          identity.index_count = capture.index_count;
          identity.instance_count = capture.instance_count;
          identity.first_instance = capture.first_instance;
          if (state.log_lines < kPoolLifecycleLogLines) {
            state.log_lines += 1u;
            identity_lines.push_back(renodx::utils::log::BuildString(
                "falcom_world::deform: frame ", frame, ": new identity ", renodx::utils::log::AsHex(capture.key),
                " VS ", renodx::utils::log::AsHex(capture.vs_hash), ", ", capture.triangles, " triangles, pipeline ",
                renodx::utils::log::AsHex(capture.vs_pipeline), ", vb ", renodx::utils::log::AsHex(capture.vb),
                " offset ", capture.vb_offset, ", ib ", renodx::utils::log::AsHex(capture.ib), " offset ",
                capture.ib_offset, ", ", capture.index_count, " indices, ", capture.instance_count,
                " instances from ", capture.first_instance));
          }
        }
        identity.captures += 1u;
        identity.last_frame = frame;
        if (identity.resident) {
          if (objects.size() >= kDynamicMaxObjects) {
            state.stats.object_cap_drops += 1u;
            continue;
          }
          DynamicObjectGPU object;
          object.node_offset = identity.node_offset;
          object.leaf_offset = identity.leaf_offset;
          object.node_count = identity.node_count;
          object.vertex_base = capture.vertex_base;
          object.triangle_count = identity.triangles;
          object.refit_offset = identity.refit_offset;
          object.level_offset = identity.level_offset;
          object.level_count = identity.level_count;
          objects.push_back(object);
          DynamicObjectInfo entry;
          entry.key = capture.key;
          entry.vs_hash = identity.vs_hash;
          entry.triangles = identity.triangles;
          entry.captures = identity.captures;
          entry.first_frame = identity.first_frame;
          std::memcpy(entry.bbox_min, identity.bbox_min, sizeof(entry.bbox_min));
          std::memcpy(entry.bbox_max, identity.bbox_max, sizeof(entry.bbox_max));
          info.push_back(entry);
          continue;
        }
        if (identity.failed || identity.topology_pending) continue;
        const uint64_t size = static_cast<uint64_t>(capture.triangles) * 3u * kDynamicVertexBytes;
        const uint64_t dest = (slot.used + 15u) & ~uint64_t{15};
        if (dest + size > kDynamicTopologySlotBytes) continue;  // next frame
        copies.push_back({static_cast<uint64_t>(capture.vertex_base) * kDynamicVertexBytes, dest, size});
        slot.requests.push_back({capture.key, dest, capture.triangles});
        slot.used = dest + size;
        identity.topology_pending = true;
      }
    }
    state.stats.frame_captures = this_frame ? static_cast<uint32_t>(state.captures.size()) : 0u;
    state.stats.frame_vertices = this_frame ? state.arena_used / kDynamicVertexBytes : 0u;
    state.stats.frame_objects = static_cast<uint32_t>(objects.size());
    state.stats.frame_pending = state.stats.frame_captures - static_cast<uint32_t>(objects.size());
    // Retire identities not captured for a while; reset the BLAS store when
    // its garbage dominates.
    for (auto it = state.identities.begin(); it != state.identities.end();) {
      if (frame - it->second.last_frame > kDynamicRetireFrames && !it->second.topology_pending) {
        state.stats.garbage_bytes += it->second.gpu_bytes;
        state.stats.retired += 1u;
        state.retired_keys.insert(it->first);
        it = state.identities.erase(it);
      } else {
        ++it;
      }
    }
    const uint64_t used_bytes = state.nodes.used * state.nodes.stride + state.leaves.used * state.leaves.stride
                                + state.refit.used * state.refit.stride;
    state.stats.used_bytes = used_bytes;
    if (state.stats.garbage_bytes > kDynamicResetGarbageBytes && state.stats.garbage_bytes * 2u > used_bytes) {
      reset = true;
      for (auto& [key, identity] : state.identities) {
        (void)key;
        identity.resident = false;  // rebuilt from a new topology read (in flight reads stay valid)
      }
      state.nodes.used = 0u;
      state.leaves.used = 0u;
      state.refit.used = 0u;
      state.stats.garbage_bytes = 0u;
      state.stats.resets += 1u;
      if (state.log_lines < kPoolLifecycleLogLines) {
        state.log_lines += 1u;
        identity_lines.push_back(renodx::utils::log::BuildString("falcom_world::deform: frame ", frame,
                                                                 ": BLAS store reset, ", state.identities.size(),
                                                                 " identities rebuilt"));
      }
      objects.clear();
      info.clear();
    }
    uint32_t resident = 0u;
    uint32_t failed = 0u;
    for (const auto& [key, identity] : state.identities) {
      (void)key;
      resident += identity.resident ? 1u : 0u;
      failed += identity.failed ? 1u : 0u;
    }
    state.stats.identities = static_cast<uint32_t>(state.identities.size());
    state.stats.resident = resident;
    state.stats.failed = failed;
    // The next frame captures from the start of the arena again.
    state.captures.clear();
    state.frame_keys.clear();
    state.arena_used = 0u;
    state.capture_frame = frame + 1u;
  }
  for (const std::string& line : identity_lines) renodx::utils::log::i(line);
  (void)reset;
  for (const TopologyCopy& copy : copies) cmd_list->copy_buffer_region(arena, copy.source_offset, staging, copy.dest_offset, copy.size);

  // 3. Objects and refit.
  stage.Set("present: deform live (refit)");
  uint32_t object_count = 0u;
  if (!objects.empty()) {
    reshade::api::resource objects_buffer = {0u};
    {
      std::lock_guard<std::mutex> lock(g_deform_live.mutex);
      objects_buffer = g_deform_live.objects_buffer;
    }
    std::string error;
    if (WriteBufferRange(device, cmd_list, objects_buffer, sizeof(DynamicObjectGPU), 0u, objects.data(),
                         objects.size() * sizeof(DynamicObjectGPU), &error)) {
      object_count = static_cast<uint32_t>(objects.size());
    }
  }
  if (object_count != 0u) DispatchDynamicRefit(device, cmd_list, g_deform_live, object_count);
  std::lock_guard<std::mutex> lock(g_deform_live.mutex);
  g_deform_live.object_count = g_deform_live.refit_pipeline.handle != 0u ? object_count : 0u;
  g_deform_live.object_info = std::move(info);
}

// Inspect lines for a hit on a dynamic object (trace hit.instance with kDynamicInstanceFlag).
inline void DescribeDynamicInspect(BvhInspect& inspect, const char* compare) {
  inspect.lines.clear();
  const std::string compare_text = compare != nullptr ? std::string("  [depth compare: ") + compare + "]" : "";
  char line[256] = {};
  std::snprintf(line, sizeof(line), "Pixel (%u, %u): hit at %.2f m, point (%.2f, %.2f, %.2f).", inspect.x, inspect.y,
                inspect.t, inspect.position[0], inspect.position[1], inspect.position[2]);
  inspect.lines.push_back(line + compare_text);
  const uint32_t index = inspect.instance & ~kDynamicInstanceFlag;
  std::lock_guard<std::mutex> lock(g_deform_live.mutex);
  if (index >= g_deform_live.object_info.size()) {
    inspect.lines.push_back("Deforming object no longer in the object list (a newer frame replaced it).");
    return;
  }
  const DynamicObjectInfo& object = g_deform_live.object_info[index];
  std::snprintf(line, sizeof(line),
                "Deforming object %u (stream output): VS 0x%08X, triangle %u of %u, captured in %u frames since frame %u.",
                index, object.vs_hash, inspect.prim, object.triangles, object.captures, object.first_frame);
  inspect.lines.push_back(line);
  std::snprintf(line, sizeof(line), "Identity 0x%016llX; topology pose bounds (%.2f, %.2f, %.2f) .. (%.2f, %.2f, %.2f).",
                static_cast<unsigned long long>(object.key), object.bbox_min[0], object.bbox_min[1], object.bbox_min[2],
                object.bbox_max[0], object.bbox_max[1], object.bbox_max[2]);
  inspect.lines.push_back(line);
}

}  // namespace falcom_world::bvh
