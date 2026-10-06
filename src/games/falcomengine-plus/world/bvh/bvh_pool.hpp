#pragma once

// Phase 1: persistent CPU world pool for the two-level world BVH.
//
// Admission comes from the shader contract, not from research verdicts or
// hash lists. contract/shader_registry.hpp classifies every vertex and pixel
// shader once from its own bytecode; a draw is admitted when its vertex
// shader is Rigid (t15 InstanceParam stride 160 indexed by SV_InstanceID +
// b1 instanceOffset_g, world float4x3 at byte 0, no other inputs, scene
// camera) and its pixel shader does not alpha-test.
//
// Per admitted draw, at draw time on the game's command list:
//   base  = instanceOffset_g from the CPU mirror of b1 (cb_value_tracker.hpp)
//   copy  = b1 (16 bytes) + exactly the elements the draw reads,
//           [base, base + instance_count) of t15 (SV_InstanceID + base),
//           into one slot of a 3-slot staging ring
// The slot is read back two presents later (no stall), and a copy is only
// used when the GPU-side b1 equals the CPU base it was sliced with.
//
// An instance is admitted once the same (mesh, world matrix) pair was read in
// two different frames, which keeps moving objects out. Meshes are captured a
// few per frame, merged by content, and retired with their instances when
// their vertex or index buffer is destroyed, so a map change empties the pool
// instead of leaving the previous map's geometry behind.

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../../../../utils/path.hpp"
#include "../../../../utils/scene.hpp"
#include "../capture/buffer_readback.hpp"
#include "../capture/cb_tracking.hpp"
#include "../capture/cb_value_tracker.hpp"
#include "../capture/mesh_capture.hpp"
#include "../contract/shader_registry.hpp"
#include "../research/transform_candidates.hpp"
#include "../world_state.hpp"

namespace falcom_world::bvh {

enum class InstanceSource : uint8_t {
  CanonicalInstance = 0u,
  ConstantBuffer = 1u,
};

inline constexpr uint32_t kPoolInstanceSlot = contract::kInstanceSlot;
inline constexpr uint32_t kPoolInstanceCbSlot = contract::kInstanceCbSlot;
inline constexpr uint32_t kPoolInstanceStride = contract::kInstanceStride;
inline constexpr uint32_t kPoolWorldFloats = contract::kWorldFloats;
inline constexpr uint32_t kPoolPrevWorldOffset = contract::kPrevWorldOffset;

inline constexpr uint32_t kPoolStableObservations = 2u;     // frames with the same (mesh, world)
inline constexpr uint32_t kPoolRecaptureFrames = 30u;       // per draw identity
inline constexpr uint32_t kPoolStagingSlots = 3u;           // read back two presents later
inline constexpr uint64_t kPoolStagingSlotBytes = 8ull * 1024ull * 1024ull;
inline constexpr uint64_t kPoolCbCopyBytes = 16u;           // b1 copy, keeps slices 16-byte aligned
inline constexpr uint32_t kPoolMaxInstancesPerDraw = 4096u;
inline constexpr uint32_t kPoolMeshCapturesPerFrame = 2u;
inline constexpr uint32_t kPoolPruneAge = 600u;
inline constexpr size_t kPoolMaxMeshes = 8192u;
inline constexpr size_t kPoolMaxInstances = 2000000u;
inline constexpr float kPoolRegionMinSize = 64.f;
inline constexpr float kPoolRegionMaxSize = 512.f;

// Why a scanned draw did not produce an instance copy. Non-rigid vertex
// shaders are counted per class in PoolStats::draws_by_vs_class instead.
enum class PoolSkip : uint8_t {
  None = 0,
  NotRigid,
  DrawState,         // rigid VS, but not an opaque depth-writing indexed triangle list
  AlphaTested,       // pixel shader cuts holes (alphaTestThreshold_g)
  PixelUnknown,      // pixel shader unclassified or unparsable
  NoInstanceSrv,     // nothing bound at t15
  NoCpuBase,         // b1 not bound or its value never seen on the CPU
  SliceRange,        // [base, base + count) outside the t15 buffer
  TooManyInstances,  // more than kPoolMaxInstancesPerDraw
  Cooldown,          // same draw identity copied less than kPoolRecaptureFrames ago
  NoStaging,         // staging ring not ready (first frame, or slot being read)
  BudgetFull,        // staging slot full this frame
  Count,
};

inline const char* PoolSkipName(PoolSkip skip) {
  switch (skip) {
    case PoolSkip::NotRigid:         return "not_rigid";
    case PoolSkip::DrawState:        return "draw_state";
    case PoolSkip::AlphaTested:      return "alpha_tested";
    case PoolSkip::PixelUnknown:     return "pixel_unknown";
    case PoolSkip::NoInstanceSrv:    return "no_instance_srv";
    case PoolSkip::NoCpuBase:        return "no_cpu_base";
    case PoolSkip::SliceRange:       return "slice_range";
    case PoolSkip::TooManyInstances: return "too_many_instances";
    case PoolSkip::Cooldown:         return "cooldown";
    case PoolSkip::NoStaging:        return "no_staging";
    case PoolSkip::BudgetFull:       return "budget_full";
    default:                         return "none";
  }
}

struct WorldMesh {
  uint64_t mesh_key = 0u;  // first draw key that produced this mesh
  uint32_t mesh_id = 0u;
  uint32_t source_vs_hash = 0u;
  uint32_t triangle_count = 0u;
  float bbox_min[3] = {};
  float bbox_max[3] = {};
  std::vector<std::array<float, 3>> positions;
  std::vector<uint32_t> indices;  // flat, three per triangle
  uint64_t signature = 0u;        // content hash (positions + indices)
  uint32_t live_keys = 0u;        // draw keys still mapped to this mesh
};

struct WorldInstance {
  uint64_t mesh_key = 0u;  // draw key it was admitted through; retired with it
  uint32_t mesh_id = 0u;
  uint32_t source_vs_hash = 0u;
  uint8_t kind = static_cast<uint8_t>(CandidateKind::Inst4x3Row);
  uint8_t source = static_cast<uint8_t>(InstanceSource::CanonicalInstance);
  uint32_t matrix_floats = kPoolWorldFloats;
  float matrix[16] = {};
  float bounds_min[3] = {};
  float bounds_max[3] = {};
};

struct InstanceKey {
  uint64_t mesh_key = 0u;
  uint64_t matrix_hash = 0u;
  bool operator==(const InstanceKey& other) const {
    return mesh_key == other.mesh_key && matrix_hash == other.matrix_hash;
  }
};

struct InstanceKeyHash {
  size_t operator()(const InstanceKey& key) const {
    return static_cast<size_t>(key.mesh_key * 1099511628211ull ^ key.matrix_hash);
  }
};

struct ObservedInstance {
  uint32_t count = 0u;
  uint32_t last_frame = 0u;
  uint32_t vs_hash = 0u;
  bool admitted = false;
  bool rejected = false;
  float matrix[kPoolWorldFloats] = {};
};

// One queued mesh capture: the draw that first showed the geometry.
struct PoolMeshRequest {
  uint64_t mesh_key = 0u;
  uint32_t vs_hash = 0u;
  DrawRecord draw;
};

struct PoolPendingCopy {
  uint64_t cb_offset = 0u;
  uint64_t slice_offset = 0u;
  uint32_t count = 0u;
  int32_t cpu_base = 0;
  uint32_t frame = 0u;
  uint32_t vs_hash = 0u;
  uint64_t mesh_key = 0u;
};

struct PoolStagingSlot {
  reshade::api::resource buffer = {0u};
  uint64_t used = 0u;
  bool resolving = false;
  std::vector<PoolPendingCopy> copies;
};

struct PoolSchedule {
  uint32_t copy_frame = 0u;
  uint32_t next_frame = 0u;
};

struct PoolFamilyStats {
  uint8_t vs_class = 0u;
  uint64_t draws = 0u;
  uint64_t copied = 0u;
  uint64_t instances_seen = 0u;
  uint32_t admitted = 0u;
  uint32_t base_mismatch = 0u;
  uint32_t rejected_matrix = 0u;
  std::array<uint64_t, static_cast<size_t>(PoolSkip::Count)> skips = {};
};

struct PoolStats {
  // Draw admission (counted per scanned draw).
  std::array<uint64_t, static_cast<size_t>(contract::VsClass::Count)> draws_by_vs_class = {};
  std::array<uint64_t, static_cast<size_t>(PoolSkip::Count)> skips = {};
  uint64_t copied_draws = 0u;
  uint64_t skipped_indirect = 0u;
  // Read back.
  uint64_t base_verified = 0u;
  uint64_t base_mismatch = 0u;
  int32_t last_mismatch_cpu = 0;
  int32_t last_mismatch_gpu = 0;
  uint32_t map_failures = 0u;
  uint32_t staging_failures = 0u;
  uint64_t instances_seen = 0u;
  uint64_t rejected_matrix = 0u;
  uint64_t moving_instances = 0u;  // world != prevWorld at the draw (diagnostic)
  uint32_t rejected_bounds = 0u;
  uint32_t dedup_instances = 0u;
  uint32_t instance_cap_drops = 0u;
  // Meshes.
  uint32_t mesh_failures = 0u;
  uint32_t mesh_dedup = 0u;
  uint32_t mesh_cap_drops = 0u;
  uint32_t meshes_retired = 0u;
  uint32_t instances_retired = 0u;
  uint32_t resource_invalidations = 0u;
  std::string last_mesh_error;
  // Sizes (refreshed by UpdatePoolStats).
  size_t queued = 0u;
  size_t mesh_queue = 0u;
  size_t meshes = 0u;
  size_t observed = 0u;
  size_t admitted = 0u;
  size_t region = 0u;
  uint64_t scan_first_frame = 0u;
  uint64_t scan_last_frame = 0u;
};

struct PoolState {
  std::atomic_bool scan_active{false};
  float region_size = 512.f;
  std::mutex mutex;

  // Staging ring. Draws of the frame in flight write `write_slot`.
  reshade::api::device* staging_device = nullptr;
  std::array<PoolStagingSlot, kPoolStagingSlots> slots;
  uint32_t write_slot = 0u;

  // Meshes.
  std::vector<PoolMeshRequest> mesh_queue;
  std::unordered_set<uint64_t> mesh_queued;   // keys queued or captured since last invalidation
  std::unordered_set<uint64_t> failed_meshes;
  std::unordered_map<uint64_t, uint32_t> mesh_by_key;
  std::unordered_map<uint64_t, uint32_t> mesh_by_signature;
  std::unordered_map<uint64_t, std::vector<uint64_t>> keys_by_resource;  // VB/IB handle -> draw keys
  std::unordered_set<uint64_t> invalidated_keys;  // pending observation cleanup
  bool retire_pending = false;
  std::vector<WorldMesh> meshes;

  // Instances.
  std::unordered_map<InstanceKey, ObservedInstance, InstanceKeyHash> observations;
  std::unordered_set<uint64_t> admitted_keys;  // (mesh_id, matrix) already in `instances`
  std::vector<WorldInstance> instances;

  std::unordered_map<uint64_t, PoolSchedule> schedule;
  std::unordered_map<uint32_t, PoolFamilyStats> families;  // by VS hash, display only
  uint64_t revision = 0u;
  PoolStats stats;
};

inline PoolState g_pool;

inline uint64_t PoolMix(uint64_t hash, uint64_t value) {
  hash ^= value;
  hash *= 1099511628211ull;
  return hash;
}

// ---------------------------------------------------------------------------
// Research helpers (Auto Research still records layouts with these).

inline bool IsPoolStructuredKind(uint8_t kind) {
  return kind == static_cast<uint8_t>(CandidateKind::Inst4x3Row)
         || kind == static_cast<uint8_t>(CandidateKind::Inst4x3Col)
         || kind == static_cast<uint8_t>(CandidateKind::Inst4x4Row)
         || kind == static_cast<uint8_t>(CandidateKind::Inst4x4Col);
}

// Chooses the instance layout research should record: the engine's canonical
// path (slot 15, stride 160) first, row kinds before col, then any other
// instance layout. Returns false when the family has no instance layout.
inline bool SelectPoolInstanceLayout(
    const std::vector<PassingLayout>& layouts,
    PassingLayout* out_layout) {
  if (out_layout == nullptr) return false;
  const PassingLayout* canonical_col = nullptr;
  const PassingLayout* fallback = nullptr;
  for (const auto& layout : layouts) {
    if (!IsPoolStructuredKind(static_cast<uint8_t>(layout.kind))) continue;
    const bool canonical_slot = layout.slot == kPoolInstanceSlot && layout.stride == kPoolInstanceStride;
    if (canonical_slot) {
      const uint8_t kind_value = static_cast<uint8_t>(layout.kind);
      const bool row_kind = kind_value == static_cast<uint8_t>(CandidateKind::Inst4x3Row)
                            || kind_value == static_cast<uint8_t>(CandidateKind::Inst4x4Row);
      if (row_kind) {
        *out_layout = layout;
        return true;
      }
      if (canonical_col == nullptr) canonical_col = &layout;
    } else if (fallback == nullptr) {
      fallback = &layout;
    }
  }
  if (canonical_col != nullptr) {
    *out_layout = *canonical_col;
    return true;
  }
  if (fallback != nullptr) {
    *out_layout = *fallback;
    return true;
  }
  return false;
}

// ---------------------------------------------------------------------------
// Transforms and bounds. Pool instances are always Inst4x3Row: the 12 floats
// of InstanceParam.world as stored, world.x = dot(float4(p, 1), m[0..3]).

inline void TransformPoolPoint(uint8_t kind, const float* matrix, const float* point, float* out) {
  switch (static_cast<CandidateKind>(kind)) {
    case CandidateKind::Inst4x3Row:
      TransformInst4x3RowDot(matrix, point[0], point[1], point[2], out);
      break;
    case CandidateKind::Inst4x3Col:
      TransformInst4x3Col(matrix, point[0], point[1], point[2], out);
      break;
    case CandidateKind::Inst4x4Row: {
      float out4[4] = {};
      TransformRowDot4(matrix, point[0], point[1], point[2], 1.f, out4);
      out[0] = out4[0];
      out[1] = out4[1];
      out[2] = out4[2];
      break;
    }
    case CandidateKind::Inst4x4Col: {
      float out4[4] = {};
      TransformCol(matrix, point[0], point[1], point[2], 1.f, out4);
      out[0] = out4[0];
      out[1] = out4[1];
      out[2] = out4[2];
      break;
    }
    default:
      out[0] = point[0];
      out[1] = point[1];
      out[2] = point[2];
      break;
  }
}

inline void PoolMatrixTranslation(const float* matrix, float* out) {
  out[0] = matrix[3];
  out[1] = matrix[7];
  out[2] = matrix[11];
}

// The linear part must be a plausible rigid/scaled basis: finite, per-axis
// length in (0.05, 50) and no two axes nearly parallel.
inline bool PoolWorldPlausible(const float* matrix) {
  for (uint32_t i = 0; i < kPoolWorldFloats; ++i) {
    if (!std::isfinite(matrix[i])) return false;
  }
  const float rows[3][3] = {
      {matrix[0], matrix[1], matrix[2]},
      {matrix[4], matrix[5], matrix[6]},
      {matrix[8], matrix[9], matrix[10]},
  };
  float lengths[3] = {};
  for (int r = 0; r < 3; ++r) {
    lengths[r] = std::sqrt(rows[r][0] * rows[r][0] + rows[r][1] * rows[r][1] + rows[r][2] * rows[r][2]);
    if (!(lengths[r] > 0.05f && lengths[r] < 50.f)) return false;
  }
  for (int a = 0; a < 3; ++a) {
    for (int b = a + 1; b < 3; ++b) {
      const float dot = (rows[a][0] * rows[b][0] + rows[a][1] * rows[b][1] + rows[a][2] * rows[b][2])
                        / (lengths[a] * lengths[b]);
      if (!(std::fabs(dot) < 0.995f)) return false;
    }
  }
  return true;
}

inline bool ComputePoolInstanceBounds(const WorldMesh& mesh, const float* matrix, float* out_min, float* out_max) {
  const float corners[8][3] = {
      {mesh.bbox_min[0], mesh.bbox_min[1], mesh.bbox_min[2]},
      {mesh.bbox_max[0], mesh.bbox_min[1], mesh.bbox_min[2]},
      {mesh.bbox_min[0], mesh.bbox_max[1], mesh.bbox_min[2]},
      {mesh.bbox_max[0], mesh.bbox_max[1], mesh.bbox_min[2]},
      {mesh.bbox_min[0], mesh.bbox_min[1], mesh.bbox_max[2]},
      {mesh.bbox_max[0], mesh.bbox_min[1], mesh.bbox_max[2]},
      {mesh.bbox_min[0], mesh.bbox_max[1], mesh.bbox_max[2]},
      {mesh.bbox_max[0], mesh.bbox_max[1], mesh.bbox_max[2]},
  };
  out_min[0] = out_min[1] = out_min[2] = 1e30f;
  out_max[0] = out_max[1] = out_max[2] = -1e30f;
  for (const auto& corner : corners) {
    float world[3] = {};
    TransformInst4x3RowDot(matrix, corner[0], corner[1], corner[2], world);
    for (int i = 0; i < 3; ++i) {
      out_min[i] = (std::min)(out_min[i], world[i]);
      out_max[i] = (std::max)(out_max[i], world[i]);
    }
  }
  float extent = 0.f;
  for (int i = 0; i < 3; ++i) {
    if (!std::isfinite(out_min[i]) || !std::isfinite(out_max[i])) return false;
    extent = (std::max)(extent, out_max[i] - out_min[i]);
  }
  return extent > 1e-6f && extent < 1e6f;
}

// ---------------------------------------------------------------------------
// Camera and region.

struct PoolCameraInfo {
  bool valid = false;
  float row[3] = {};
  float legacy[3] = {};
  float position[3] = {};
};

inline PoolCameraInfo GetPoolCameraInfo() {
  PoolCameraInfo info;
  std::lock_guard<std::mutex> lock(g_state.mutex);
  if (!g_state.camera.valid) return info;
  const float* inv = g_state.camera.view_inv;
  info.row[0] = inv[3];
  info.row[1] = inv[7];
  info.row[2] = inv[11];
  info.legacy[0] = inv[12];
  info.legacy[1] = inv[13];
  info.legacy[2] = inv[14];
  const auto usable = [](const float* value) {
    return std::isfinite(value[0]) && std::isfinite(value[1]) && std::isfinite(value[2])
           && (value[0] != 0.f || value[1] != 0.f || value[2] != 0.f);
  };
  const float* selected = nullptr;
  if (usable(info.row)) {
    selected = info.row;
  } else if (usable(info.legacy)) {
    selected = info.legacy;
  }
  if (selected == nullptr) return info;
  info.position[0] = selected[0];
  info.position[1] = selected[1];
  info.position[2] = selected[2];
  info.valid = true;
  return info;
}

inline bool PoolCameraPosition(float* out) {
  if (out == nullptr) return false;
  const PoolCameraInfo info = GetPoolCameraInfo();
  if (!info.valid) return false;
  out[0] = info.position[0];
  out[1] = info.position[1];
  out[2] = info.position[2];
  return true;
}

struct PoolRegion {
  float size = 128.f;
  float min[3] = {};
  float max[3] = {};
};

inline PoolRegion CurrentPoolRegion() {
  PoolRegion region;
  region.size = g_pool.region_size;
  float camera[3] = {};
  if (!PoolCameraPosition(camera)) {
    camera[0] = camera[1] = camera[2] = 0.f;
  }
  // Snap the region center to half-size cells offset by a quarter: the camera
  // is then always at least size/4 away from every boundary (instead of being
  // able to sit right on one), while rebuilds still only happen when the
  // camera crosses a half-size cell.
  const float half = region.size * 0.5f;
  const float quarter = region.size * 0.25f;
  for (int i = 0; i < 3; ++i) {
    const float center = std::floor(camera[i] / half) * half + quarter;
    region.min[i] = center - half;
    region.max[i] = center + half;
  }
  return region;
}

inline bool PoolInstanceInRegion(const WorldInstance& instance, const PoolRegion& region) {
  for (int i = 0; i < 3; ++i) {
    if (instance.bounds_max[i] < region.min[i] || instance.bounds_min[i] > region.max[i]) return false;
  }
  return true;
}

// Caller holds g_pool.mutex.
inline void UpdatePoolStats() {
  size_t queued = 0u;
  for (const auto& slot : g_pool.slots) queued += slot.copies.size();
  g_pool.stats.queued = queued;
  g_pool.stats.mesh_queue = g_pool.mesh_queue.size();
  g_pool.stats.meshes = g_pool.meshes.size();
  g_pool.stats.observed = g_pool.observations.size();
  g_pool.stats.admitted = g_pool.instances.size();
  const PoolRegion region = CurrentPoolRegion();
  size_t in_region = 0u;
  for (const auto& instance : g_pool.instances) {
    if (PoolInstanceInRegion(instance, region)) in_region += 1u;
  }
  g_pool.stats.region = in_region;
}

// ---------------------------------------------------------------------------
// Draw gate and identities.

// Opaque, depth-writing, indexed triangle-list geometry. No render target or
// pixel shader is required: shadow casters are the same objects and also see
// objects outside the camera view.
inline bool IsPoolGeometryDraw(const DrawRecord& draw) {
  if (draw.method != 1u || !draw.has_index_buffer) return false;
  if (draw.vb.handle == 0u || draw.ib.handle == 0u || draw.dsv.handle == 0u) return false;
  if (draw.index_count < 3u || (draw.index_count % 3u) != 0u) return false;
  if (draw.blend_enable || !draw.depth_enable || !draw.depth_write) return false;
  return draw.topology == reshade::api::primitive_topology::undefined
         || draw.topology == reshade::api::primitive_topology::triangle_list;
}

// Geometry identity of an indexed draw: the same VB/IB range is one mesh
// whichever pass or input layout draws it, so shadow and G-buffer draws of an
// object share it. Content dedup at capture merges the rest.
inline uint64_t PoolMeshKey(const DrawRecord& draw) {
  uint64_t key = 1469598103934665603ull;
  key = PoolMix(key, draw.vb.handle);
  key = PoolMix(key, draw.vb_offset);
  key = PoolMix(key, draw.vb_stride);
  key = PoolMix(key, draw.ib.handle);
  key = PoolMix(key, draw.ib_offset);
  key = PoolMix(key, draw.index_size);
  key = PoolMix(key, draw.first_index);
  key = PoolMix(key, draw.index_count);
  key = PoolMix(key, static_cast<uint64_t>(static_cast<int64_t>(draw.vertex_offset)));
  return key;
}

// Rate-limit identity. Draws sharing it within one frame are all copied (two
// separate objects can share mesh and instance range); the identity is then
// skipped for kPoolRecaptureFrames.
inline uint64_t PoolScheduleKey(uint64_t mesh_key, uint32_t first_instance, uint32_t count) {
  uint64_t key = PoolMix(1469598103934665603ull, mesh_key);
  key = PoolMix(key, first_instance);
  return PoolMix(key, count);
}

inline uint64_t PoolAdmittedKey(uint32_t mesh_id, uint64_t matrix_hash) {
  return PoolMix(PoolMix(1469598103934665603ull, mesh_id), matrix_hash);
}

// ---------------------------------------------------------------------------
// Staging ring.

// Staging buffers are created and destroyed outside g_pool.mutex: resource
// events raised by those calls must never re-enter a held pool lock.

// Caller holds g_pool.mutex. Detaches the ring and returns its buffers.
inline std::vector<reshade::api::resource> DetachPoolStaging(reshade::api::device** out_device) {
  std::vector<reshade::api::resource> buffers;
  for (auto& slot : g_pool.slots) {
    if (slot.buffer.handle != 0u) buffers.push_back(slot.buffer);
    slot = {};
  }
  *out_device = g_pool.staging_device;
  g_pool.staging_device = nullptr;
  g_pool.write_slot = 0u;
  return buffers;
}

inline void DestroyPoolStagingBuffers(reshade::api::device* device, const std::vector<reshade::api::resource>& buffers) {
  if (device == nullptr) return;
  for (const auto buffer : buffers) device->destroy_resource(buffer);
}

// Slots have a fixed size: copies already recorded into a slot must never see
// it reallocated.
inline void EnsurePoolStaging(reshade::api::device* device) {
  {
    std::lock_guard<std::mutex> lock(g_pool.mutex);
    if (g_pool.staging_device == device) return;
  }
  std::vector<reshade::api::resource> created;
  for (uint32_t i = 0; i < kPoolStagingSlots; ++i) {
    const reshade::api::resource_desc desc(
        kPoolStagingSlotBytes, reshade::api::memory_heap::gpu_to_cpu, reshade::api::resource_usage::copy_dest);
    reshade::api::resource buffer = {0u};
    if (!device->create_resource(desc, nullptr, reshade::api::resource_usage::copy_dest, &buffer)) break;
    created.push_back(buffer);
  }
  if (created.size() != kPoolStagingSlots) {
    DestroyPoolStagingBuffers(device, created);
    std::lock_guard<std::mutex> lock(g_pool.mutex);
    g_pool.stats.staging_failures += 1u;
    return;
  }
  reshade::api::device* previous_device = nullptr;
  std::vector<reshade::api::resource> previous;
  {
    std::lock_guard<std::mutex> lock(g_pool.mutex);
    previous = DetachPoolStaging(&previous_device);
    for (uint32_t i = 0; i < kPoolStagingSlots; ++i) g_pool.slots[i].buffer = created[i];
    g_pool.staging_device = device;
  }
  DestroyPoolStagingBuffers(previous_device, previous);
}

inline void OnDestroyDevicePool(reshade::api::device* device) {
  reshade::api::device* staging_device = nullptr;
  std::vector<reshade::api::resource> buffers;
  {
    std::lock_guard<std::mutex> lock(g_pool.mutex);
    if (device != nullptr && g_pool.staging_device != device) return;
    buffers = DetachPoolStaging(&staging_device);
  }
  DestroyPoolStagingBuffers(staging_device, buffers);
}

// ---------------------------------------------------------------------------
// Admission.

// Caller holds g_pool.mutex.
inline bool AdmitPoolInstance(ObservedInstance& observed, uint64_t mesh_key, uint32_t mesh_id) {
  if (mesh_id >= g_pool.meshes.size()) return false;
  const uint64_t admitted_key = PoolAdmittedKey(mesh_id, MatrixHash(observed.matrix, kPoolWorldFloats));
  if (g_pool.admitted_keys.count(admitted_key) != 0u) {
    // Same mesh content at the same place, reached through another draw key
    // (another pass or a duplicate VB): one instance is enough.
    observed.admitted = true;
    g_pool.stats.dedup_instances += 1u;
    return false;
  }
  if (g_pool.instances.size() >= kPoolMaxInstances) {
    g_pool.stats.instance_cap_drops += 1u;
    return false;
  }
  WorldInstance instance;
  if (!ComputePoolInstanceBounds(g_pool.meshes[mesh_id], observed.matrix, instance.bounds_min, instance.bounds_max)) {
    observed.rejected = true;
    g_pool.stats.rejected_bounds += 1u;
    return false;
  }
  observed.admitted = true;
  instance.mesh_key = mesh_key;
  instance.mesh_id = mesh_id;
  instance.source_vs_hash = observed.vs_hash;
  std::memcpy(instance.matrix, observed.matrix, sizeof(float) * kPoolWorldFloats);
  g_pool.instances.push_back(instance);
  g_pool.admitted_keys.insert(admitted_key);
  g_pool.families[observed.vs_hash].admitted += 1u;
  g_pool.revision += 1u;
  return true;
}

// Caller holds g_pool.mutex.
inline void AdmitPendingInstancesForMesh(uint64_t mesh_key, uint32_t mesh_id) {
  for (auto& [key, observed] : g_pool.observations) {
    if (key.mesh_key != mesh_key || observed.admitted || observed.rejected) continue;
    if (observed.count < kPoolStableObservations) continue;
    AdmitPoolInstance(observed, mesh_key, mesh_id);
  }
}

// Caller holds g_pool.mutex.
inline void ObservePoolInstance(uint64_t mesh_key, uint32_t vs_hash, const float* world, uint32_t frame) {
  const InstanceKey key{mesh_key, MatrixHash(world, kPoolWorldFloats)};
  ObservedInstance& observed = g_pool.observations[key];
  if (observed.rejected || observed.admitted) return;
  if (observed.count == 0u || observed.last_frame != frame) {
    observed.count += 1u;
    observed.last_frame = frame;
  }
  observed.vs_hash = vs_hash;
  std::memcpy(observed.matrix, world, sizeof(float) * kPoolWorldFloats);
  if (observed.count < kPoolStableObservations) return;
  const auto mesh_it = g_pool.mesh_by_key.find(mesh_key);
  if (mesh_it != g_pool.mesh_by_key.end()) AdmitPoolInstance(observed, mesh_key, mesh_it->second);
}

// ---------------------------------------------------------------------------
// Draw-time scan.

// Indirect draws carry their counts in GPU memory, so the instance slice is
// unknown when the draw is recorded. None were seen in Sora 2nd; count them
// so a gap is visible if that changes.
inline void CountPoolIndirectDraw() {
  if (!g_pool.scan_active.load(std::memory_order_relaxed)) return;
  std::lock_guard<std::mutex> lock(g_pool.mutex);
  g_pool.stats.skipped_indirect += 1u;
}

inline void OnPoolScanDraw(
    reshade::api::device* device,
    reshade::api::command_list* cmd_list,
    const DrawRecord& draw,
    WorldCommandListData* cl_data) {
  if (!g_pool.scan_active.load(std::memory_order_relaxed)) return;
  if (device == nullptr || cmd_list == nullptr || cl_data == nullptr) return;

  const contract::VsClass vs_class = contract::LookupVertexClass(draw.vs_pipeline);
  PoolSkip skip = PoolSkip::None;
  if (vs_class != contract::VsClass::Rigid) {
    skip = PoolSkip::NotRigid;
  } else if (!IsPoolGeometryDraw(draw)) {
    skip = PoolSkip::DrawState;
  } else {
    const contract::PsClass ps_class = contract::LookupPixelClass(draw.ps_pipeline);
    if (ps_class == contract::PsClass::AlphaTested) {
      skip = PoolSkip::AlphaTested;
    } else if (ps_class != contract::PsClass::Opaque) {
      skip = PoolSkip::PixelUnknown;
    }
  }

  reshade::api::resource instance_buffer = {0u};
  reshade::api::resource instance_cb = {0u};
  int32_t base = 0;
  uint32_t count = 0u;
  uint64_t slice_offset = 0u;
  uint64_t slice_bytes = 0u;
  uint64_t cb_bytes = 0u;
  if (skip == PoolSkip::None) {
    instance_buffer = cl_data->vs_srv[kPoolInstanceSlot];
    instance_cb = cl_data->vs_cb[kPoolInstanceCbSlot];
    count = draw.instance_count == 0u ? 1u : draw.instance_count;
    if (instance_buffer.handle == 0u) {
      skip = PoolSkip::NoInstanceSrv;
    } else if (instance_cb.handle == 0u || !ReadTrackedCbInt(cmd_list, instance_cb, &base)) {
      skip = PoolSkip::NoCpuBase;
    } else if (count > kPoolMaxInstancesPerDraw) {
      skip = PoolSkip::TooManyInstances;
    } else {
      const auto buffer_desc = device->get_resource_desc(instance_buffer);
      const auto cb_desc = device->get_resource_desc(instance_cb);
      // D3D11 SV_InstanceID starts at 0 whatever StartInstanceLocation is
      // (that only offsets per-instance vertex streams), so the elements
      // read are [instanceOffset_g, + count).
      const int64_t first = static_cast<int64_t>(base);
      const uint64_t buffer_size =
          buffer_desc.type == reshade::api::resource_type::buffer ? buffer_desc.buffer.size : 0u;
      cb_bytes = cb_desc.type == reshade::api::resource_type::buffer
                     ? (std::min)(cb_desc.buffer.size, kPoolCbCopyBytes)
                     : 0u;
      slice_offset = first < 0 ? 0u : static_cast<uint64_t>(first) * kPoolInstanceStride;
      slice_bytes = static_cast<uint64_t>(count) * kPoolInstanceStride;
      if (cb_bytes < sizeof(int32_t)) {
        skip = PoolSkip::NoCpuBase;
      } else if (first < 0 || slice_offset + slice_bytes > buffer_size) {
        skip = PoolSkip::SliceRange;
      }
    }
  }

  const uint32_t frame = g_state.frame.load();
  const uint64_t mesh_key = skip == PoolSkip::None ? PoolMeshKey(draw) : 0u;

  std::lock_guard<std::mutex> lock(g_pool.mutex);
  g_pool.stats.draws_by_vs_class[static_cast<size_t>(vs_class)] += 1u;
  PoolFamilyStats& family = g_pool.families[draw.vs_hash];
  family.vs_class = static_cast<uint8_t>(vs_class);
  family.draws += 1u;

  if (skip == PoolSkip::None) {
    PoolSchedule& schedule = g_pool.schedule[PoolScheduleKey(mesh_key, draw.first_instance, count)];
    PoolStagingSlot& slot = g_pool.slots[g_pool.write_slot];
    if (schedule.next_frame != 0u && schedule.copy_frame != frame && frame < schedule.next_frame) {
      skip = PoolSkip::Cooldown;
    } else if (g_pool.staging_device != device || slot.buffer.handle == 0u || slot.resolving) {
      skip = PoolSkip::NoStaging;
    } else if (slot.used + kPoolCbCopyBytes + slice_bytes > kPoolStagingSlotBytes) {
      skip = PoolSkip::BudgetFull;
    } else {
      PoolPendingCopy copy;
      copy.cb_offset = slot.used;
      copy.slice_offset = slot.used + kPoolCbCopyBytes;
      copy.count = count;
      copy.cpu_base = base;
      copy.frame = frame;
      copy.vs_hash = draw.vs_hash;
      copy.mesh_key = mesh_key;
      // The b1 copy runs at this point of the command stream, so it holds
      // exactly the offset this draw reads; resolve compares it to `base`.
      cmd_list->copy_buffer_region(instance_cb, 0u, slot.buffer, copy.cb_offset, cb_bytes);
      cmd_list->copy_buffer_region(instance_buffer, slice_offset, slot.buffer, copy.slice_offset, slice_bytes);
      slot.used += kPoolCbCopyBytes + slice_bytes;
      slot.copies.push_back(copy);
      schedule.copy_frame = frame;
      schedule.next_frame = frame + kPoolRecaptureFrames;
      g_pool.stats.copied_draws += 1u;
      family.copied += 1u;

      if (g_pool.mesh_queued.insert(mesh_key).second) {
        if (g_pool.mesh_by_key.count(mesh_key) == 0u && g_pool.failed_meshes.count(mesh_key) == 0u) {
          g_pool.mesh_queue.push_back({mesh_key, draw.vs_hash, draw});
          g_pool.keys_by_resource[draw.vb.handle].push_back(mesh_key);
          if (draw.ib.handle != draw.vb.handle) g_pool.keys_by_resource[draw.ib.handle].push_back(mesh_key);
        }
      }
      return;
    }
  }
  g_pool.stats.skips[static_cast<size_t>(skip)] += 1u;
  family.skips[static_cast<size_t>(skip)] += 1u;
}

// ---------------------------------------------------------------------------
// Resource invalidation and retirement.

// Caller holds g_pool.mutex. Unmaps one draw key from its mesh; the mesh is
// retired (with its instances) once no draw key refers to it any more.
inline void InvalidatePoolMeshKey(uint64_t mesh_key) {
  g_pool.mesh_queued.erase(mesh_key);
  g_pool.failed_meshes.erase(mesh_key);
  g_pool.invalidated_keys.insert(mesh_key);
  g_pool.mesh_queue.erase(
      std::remove_if(g_pool.mesh_queue.begin(), g_pool.mesh_queue.end(),
                     [mesh_key](const PoolMeshRequest& request) { return request.mesh_key == mesh_key; }),
      g_pool.mesh_queue.end());
  const auto it = g_pool.mesh_by_key.find(mesh_key);
  if (it == g_pool.mesh_by_key.end()) return;
  WorldMesh& mesh = g_pool.meshes[it->second];
  if (mesh.live_keys != 0u) mesh.live_keys -= 1u;
  if (mesh.live_keys == 0u) g_pool.retire_pending = true;
  g_pool.mesh_by_key.erase(it);
}

inline void OnDestroyResourcePool(reshade::api::device* device, reshade::api::resource resource) {
  (void)device;
  std::lock_guard<std::mutex> lock(g_pool.mutex);
  if (g_pool.keys_by_resource.empty()) return;
  const auto it = g_pool.keys_by_resource.find(resource.handle);
  if (it == g_pool.keys_by_resource.end()) return;
  for (const uint64_t mesh_key : it->second) InvalidatePoolMeshKey(mesh_key);
  g_pool.keys_by_resource.erase(it);
  g_pool.stats.resource_invalidations += 1u;
}

// Caller holds g_pool.mutex. Removes instances admitted through invalidated
// draw keys and meshes no key refers to any more, renumbers mesh ids, and
// forgets observations of invalidated keys (a reused buffer handle must start
// from zero observations). Observations whose instance was removed through
// another key (content dedup) are re-armed so they can admit again.
inline void CompactPool() {
  if (g_pool.invalidated_keys.empty() && !g_pool.retire_pending) return;
  const std::unordered_set<uint64_t>& invalidated = g_pool.invalidated_keys;

  for (auto it = g_pool.observations.begin(); it != g_pool.observations.end();) {
    if (invalidated.count(it->first.mesh_key) != 0u) {
      it = g_pool.observations.erase(it);
    } else {
      ++it;
    }
  }

  std::vector<uint32_t> remap(g_pool.meshes.size(), UINT32_MAX);
  std::vector<WorldMesh> meshes;
  meshes.reserve(g_pool.meshes.size());
  for (auto& mesh : g_pool.meshes) {
    if (mesh.live_keys == 0u) {
      g_pool.stats.meshes_retired += 1u;
      continue;
    }
    remap[mesh.mesh_id] = static_cast<uint32_t>(meshes.size());
    mesh.mesh_id = static_cast<uint32_t>(meshes.size());
    meshes.push_back(std::move(mesh));
  }
  g_pool.meshes = std::move(meshes);

  std::vector<WorldInstance> instances;
  instances.reserve(g_pool.instances.size());
  g_pool.admitted_keys.clear();
  for (auto& instance : g_pool.instances) {
    const uint32_t mesh_id = instance.mesh_id < remap.size() ? remap[instance.mesh_id] : UINT32_MAX;
    if (mesh_id == UINT32_MAX || invalidated.count(instance.mesh_key) != 0u) {
      g_pool.stats.instances_retired += 1u;
      continue;
    }
    instance.mesh_id = mesh_id;
    g_pool.admitted_keys.insert(PoolAdmittedKey(mesh_id, MatrixHash(instance.matrix, kPoolWorldFloats)));
    instances.push_back(instance);
  }
  g_pool.instances = std::move(instances);

  for (auto& [key, mesh_id] : g_pool.mesh_by_key) {
    (void)key;
    mesh_id = remap[mesh_id];
  }
  g_pool.mesh_by_signature.clear();
  for (const auto& mesh : g_pool.meshes) g_pool.mesh_by_signature.emplace(mesh.signature, mesh.mesh_id);

  for (auto& [key, observed] : g_pool.observations) {
    if (!observed.admitted) continue;
    const auto mesh_it = g_pool.mesh_by_key.find(key.mesh_key);
    if (mesh_it == g_pool.mesh_by_key.end()
        || g_pool.admitted_keys.count(PoolAdmittedKey(mesh_it->second, MatrixHash(observed.matrix, kPoolWorldFloats))) == 0u) {
      observed.admitted = false;
    }
  }

  g_pool.invalidated_keys.clear();
  g_pool.retire_pending = false;
  g_pool.revision += 1u;
}

// ---------------------------------------------------------------------------
// Present-time work.

inline uint64_t PoolMeshSignature(const renodx::utils::scene::CapturedMesh& mesh) {
  uint64_t hash = 1469598103934665603ull;
  for (const auto& position : mesh.positions) {
    for (const float value : position) {
      uint32_t bits = 0u;
      std::memcpy(&bits, &value, sizeof(bits));
      hash = PoolMix(hash, bits);
    }
  }
  for (const auto& triangle : mesh.triangles) {
    hash = PoolMix(hash, triangle[0]);
    hash = PoolMix(hash, triangle[1]);
    hash = PoolMix(hash, triangle[2]);
  }
  return PoolMix(hash, mesh.positions.size());
}

inline void CaptureOnePoolMesh(reshade::api::device* device, reshade::api::command_queue* queue) {
  PoolMeshRequest request;
  {
    std::lock_guard<std::mutex> lock(g_pool.mutex);
    if (g_pool.mesh_queue.empty()) return;
    request = std::move(g_pool.mesh_queue.back());
    g_pool.mesh_queue.pop_back();
  }
  renodx::utils::scene::CapturedMesh mesh;
  std::string error;
  const bool captured =
      renodx::utils::scene::CaptureDrawIndexed(device, queue, ToSceneRecord(request.draw), &mesh, &error)
      && !mesh.positions.empty() && !mesh.triangles.empty();

  std::lock_guard<std::mutex> lock(g_pool.mutex);
  // Invalidated while the capture ran: the buffers are gone, drop the result.
  if (g_pool.mesh_queued.count(request.mesh_key) == 0u) return;
  if (!captured) {
    g_pool.failed_meshes.insert(request.mesh_key);
    g_pool.stats.mesh_failures += 1u;
    g_pool.stats.last_mesh_error = error.empty() ? std::string("empty mesh") : error;
    return;
  }
  if (g_pool.mesh_by_key.count(request.mesh_key) != 0u) return;

  uint32_t mesh_id = 0u;
  const uint64_t signature = PoolMeshSignature(mesh);
  const auto signature_it = g_pool.mesh_by_signature.find(signature);
  if (signature_it != g_pool.mesh_by_signature.end()) {
    mesh_id = signature_it->second;
    g_pool.stats.mesh_dedup += 1u;
  } else {
    if (g_pool.meshes.size() >= kPoolMaxMeshes) {
      g_pool.stats.mesh_cap_drops += 1u;
      g_pool.failed_meshes.insert(request.mesh_key);
      return;
    }
    WorldMesh world_mesh;
    world_mesh.mesh_key = request.mesh_key;
    world_mesh.mesh_id = static_cast<uint32_t>(g_pool.meshes.size());
    world_mesh.source_vs_hash = request.vs_hash;
    world_mesh.signature = signature;
    world_mesh.triangle_count = static_cast<uint32_t>(mesh.triangles.size());
    std::memcpy(world_mesh.bbox_min, mesh.bbox_min.data(), sizeof(float) * 3u);
    std::memcpy(world_mesh.bbox_max, mesh.bbox_max.data(), sizeof(float) * 3u);
    world_mesh.positions = std::move(mesh.positions);
    world_mesh.indices.reserve(mesh.triangles.size() * 3u);
    for (const auto& triangle : mesh.triangles) {
      world_mesh.indices.push_back(triangle[0]);
      world_mesh.indices.push_back(triangle[1]);
      world_mesh.indices.push_back(triangle[2]);
    }
    mesh_id = world_mesh.mesh_id;
    g_pool.meshes.push_back(std::move(world_mesh));
    g_pool.mesh_by_signature.emplace(signature, mesh_id);
    g_pool.revision += 1u;
  }
  g_pool.meshes[mesh_id].live_keys += 1u;
  g_pool.mesh_by_key[request.mesh_key] = mesh_id;
  AdmitPendingInstancesForMesh(request.mesh_key, mesh_id);
}

struct PoolResolvedCopy {
  bool verified = false;
  int32_t gpu_base = 0;
  size_t first_world = 0u;  // index into the parsed world list
};

// Reads one staging slot that the GPU finished two presents ago and applies
// its copies. The map happens without holding g_pool.mutex so recording
// threads are not blocked; the slot is flagged `resolving` meanwhile.
inline void ResolvePoolSlot(reshade::api::device* device, uint32_t slot_index) {
  std::vector<PoolPendingCopy> copies;
  reshade::api::resource buffer = {0u};
  uint64_t used = 0u;
  {
    std::lock_guard<std::mutex> lock(g_pool.mutex);
    PoolStagingSlot& slot = g_pool.slots[slot_index];
    if (slot.copies.empty()) {
      slot.used = 0u;
      return;
    }
    copies.swap(slot.copies);
    buffer = slot.buffer;
    used = slot.used;
    slot.resolving = true;
  }

  std::vector<PoolResolvedCopy> resolved(copies.size());
  std::vector<float> worlds;  // kPoolWorldFloats per instance of verified copies
  std::vector<uint8_t> moving;
  void* mapped = nullptr;
  const bool mapped_ok = buffer.handle != 0u && used != 0u
                         && device->map_buffer_region(buffer, 0u, used, reshade::api::map_access::read_only, &mapped)
                         && mapped != nullptr;
  if (mapped_ok) {
    const auto* bytes = static_cast<const uint8_t*>(mapped);
    for (size_t i = 0; i < copies.size(); ++i) {
      const PoolPendingCopy& copy = copies[i];
      std::memcpy(&resolved[i].gpu_base, bytes + copy.cb_offset, sizeof(int32_t));
      resolved[i].verified = resolved[i].gpu_base == copy.cpu_base;
      if (!resolved[i].verified) continue;
      resolved[i].first_world = worlds.size() / kPoolWorldFloats;
      for (uint32_t element = 0; element < copy.count; ++element) {
        const uint8_t* instance = bytes + copy.slice_offset + static_cast<uint64_t>(element) * kPoolInstanceStride;
        const size_t offset = worlds.size();
        worlds.resize(offset + kPoolWorldFloats);
        std::memcpy(worlds.data() + offset, instance, sizeof(float) * kPoolWorldFloats);
        moving.push_back(std::memcmp(instance, instance + kPoolPrevWorldOffset, sizeof(float) * kPoolWorldFloats) != 0
                             ? 1u
                             : 0u);
      }
    }
    device->unmap_buffer_region(buffer);
  }

  std::lock_guard<std::mutex> lock(g_pool.mutex);
  PoolStagingSlot& slot = g_pool.slots[slot_index];
  slot.resolving = false;
  slot.used = 0u;
  if (!mapped_ok) {
    g_pool.stats.map_failures += 1u;
    return;
  }
  for (size_t i = 0; i < copies.size(); ++i) {
    const PoolPendingCopy& copy = copies[i];
    PoolFamilyStats& family = g_pool.families[copy.vs_hash];
    if (!resolved[i].verified) {
      g_pool.stats.base_mismatch += 1u;
      g_pool.stats.last_mismatch_cpu = copy.cpu_base;
      g_pool.stats.last_mismatch_gpu = resolved[i].gpu_base;
      family.base_mismatch += 1u;
      continue;
    }
    g_pool.stats.base_verified += 1u;
    for (uint32_t element = 0; element < copy.count; ++element) {
      const size_t index = resolved[i].first_world + element;
      const float* world = worlds.data() + index * kPoolWorldFloats;
      g_pool.stats.instances_seen += 1u;
      family.instances_seen += 1u;
      if (moving[index] != 0u) g_pool.stats.moving_instances += 1u;
      if (!PoolWorldPlausible(world)) {
        g_pool.stats.rejected_matrix += 1u;
        family.rejected_matrix += 1u;
        continue;
      }
      ObservePoolInstance(copy.mesh_key, copy.vs_hash, world, copy.frame);
    }
  }
}

inline void PrunePool(uint32_t frame) {
  for (auto it = g_pool.observations.begin(); it != g_pool.observations.end();) {
    if (!it->second.admitted && frame > it->second.last_frame + kPoolPruneAge) {
      it = g_pool.observations.erase(it);
    } else {
      ++it;
    }
  }
  for (auto it = g_pool.schedule.begin(); it != g_pool.schedule.end();) {
    if (frame > it->second.next_frame + kPoolPruneAge) {
      it = g_pool.schedule.erase(it);
    } else {
      ++it;
    }
  }
}

// Called once per present, after the frame's draws were recorded. Retires
// invalidated meshes even while the scan is off, so the uploaded pool never
// keeps geometry whose buffers are gone.
inline void DrainPoolScan(reshade::api::device* device, reshade::api::command_queue* queue) {
  if (device == nullptr || queue == nullptr) return;
  const bool scanning = g_pool.scan_active.load(std::memory_order_relaxed);
  const uint32_t frame = g_state.frame.load();

  if (scanning) EnsurePoolStaging(device);

  uint32_t resolve_slot = UINT32_MAX;
  {
    std::lock_guard<std::mutex> lock(g_pool.mutex);
    CompactPool();
    if (scanning) {
      if (g_pool.stats.scan_first_frame == 0u) g_pool.stats.scan_first_frame = frame;
      g_pool.stats.scan_last_frame = frame;
    }
    if (g_pool.staging_device == device) {
      // The slot the next frame writes was filled three frames ago; its
      // copies have had two presents to complete.
      g_pool.write_slot = (g_pool.write_slot + 1u) % kPoolStagingSlots;
      resolve_slot = g_pool.write_slot;
    }
  }
  if (resolve_slot != UINT32_MAX) ResolvePoolSlot(device, resolve_slot);

  for (uint32_t i = 0; i < kPoolMeshCapturesPerFrame; ++i) CaptureOnePoolMesh(device, queue);

  std::lock_guard<std::mutex> lock(g_pool.mutex);
  if ((frame % 60u) == 0u) PrunePool(frame);
  UpdatePoolStats();
}

inline void ResetWorldPool() {
  std::lock_guard<std::mutex> lock(g_pool.mutex);
  for (auto& slot : g_pool.slots) {
    // Copies already recorded into a slot still land there; only forget them.
    if (!slot.resolving) {
      slot.copies.clear();
      slot.used = 0u;
    }
  }
  g_pool.mesh_queue.clear();
  g_pool.mesh_queued.clear();
  g_pool.failed_meshes.clear();
  g_pool.mesh_by_key.clear();
  g_pool.mesh_by_signature.clear();
  g_pool.keys_by_resource.clear();
  g_pool.invalidated_keys.clear();
  g_pool.retire_pending = false;
  g_pool.meshes.clear();
  g_pool.observations.clear();
  g_pool.admitted_keys.clear();
  g_pool.instances.clear();
  g_pool.schedule.clear();
  g_pool.families.clear();
  g_pool.revision += 1u;
  g_pool.stats = {};
}

// ---------------------------------------------------------------------------
// Dumps.

inline std::filesystem::path PoolOutputDir() {
  auto path = renodx::utils::path::GetOutputPath() / "falcomengine-plus" / "world";
  std::error_code ec;
  std::filesystem::create_directories(path, ec);
  return path;
}

inline std::string PoolHashText(uint32_t hash) {
  char text[16] = {};
  std::snprintf(text, sizeof(text), "0x%08X", hash);
  return text;
}

inline void DumpWorldPool() {
  std::vector<WorldMesh> meshes;
  std::vector<WorldInstance> instances;
  std::unordered_map<uint32_t, PoolFamilyStats> families;
  PoolStats stats;
  PoolRegion region;
  const PoolCameraInfo camera = GetPoolCameraInfo();
  const contract::RegistryCounts registry_counts = contract::SnapshotRegistryCounts();
  const contract::RegistryEntries registry_entries = contract::SnapshotRegistryEntries();
  {
    std::lock_guard<std::mutex> lock(g_pool.mutex);
    UpdatePoolStats();
    meshes = g_pool.meshes;
    instances = g_pool.instances;
    families = g_pool.families;
    stats = g_pool.stats;
    region = CurrentPoolRegion();
  }

  struct FamilyBounds {
    uint32_t meshes = 0u;
    uint32_t in_region = 0u;
    float bounds_min[3] = {1e30f, 1e30f, 1e30f};
    float bounds_max[3] = {-1e30f, -1e30f, -1e30f};
  };
  std::unordered_map<uint32_t, FamilyBounds> bounds;
  for (const auto& mesh : meshes) bounds[mesh.source_vs_hash].meshes += 1u;
  for (const auto& instance : instances) {
    auto& entry = bounds[instance.source_vs_hash];
    if (PoolInstanceInRegion(instance, region)) entry.in_region += 1u;
    for (int i = 0; i < 3; ++i) {
      entry.bounds_min[i] = (std::min)(entry.bounds_min[i], instance.bounds_min[i]);
      entry.bounds_max[i] = (std::max)(entry.bounds_max[i], instance.bounds_max[i]);
    }
  }

  std::ostringstream out;
  out << "{\n";
  out << "  \"schema\": 2,\n";
  out << "  \"generated_frame\": " << g_state.frame.load() << ",\n";
  out << "  \"camera_valid\": " << (camera.valid ? "true" : "false") << ",\n";
  out << "  \"camera_position\": [" << camera.position[0] << ", " << camera.position[1] << ", " << camera.position[2] << "],\n";
  out << "  \"region\": {\"size\": " << region.size
      << ", \"min\": [" << region.min[0] << ", " << region.min[1] << ", " << region.min[2]
      << "], \"max\": [" << region.max[0] << ", " << region.max[1] << ", " << region.max[2] << "]},\n";

  out << "  \"classifier\": {\"vertex\": {";
  for (size_t i = 0; i < registry_counts.vertex.size(); ++i) {
    if (i != 0u) out << ", ";
    out << "\"" << contract::VsClassName(static_cast<contract::VsClass>(i)) << "\": " << registry_counts.vertex[i];
  }
  out << "}, \"pixel\": {";
  for (size_t i = 0; i < registry_counts.pixel.size(); ++i) {
    if (i != 0u) out << ", ";
    out << "\"" << contract::PsClassName(static_cast<contract::PsClass>(i)) << "\": " << registry_counts.pixel[i];
  }
  out << "}},\n";

  out << "  \"stats\": {";
  out << "\"draws_by_vs_class\": {";
  for (size_t i = 0; i < stats.draws_by_vs_class.size(); ++i) {
    if (i != 0u) out << ", ";
    out << "\"" << contract::VsClassName(static_cast<contract::VsClass>(i)) << "\": " << stats.draws_by_vs_class[i];
  }
  out << "}, \"skips\": {";
  for (size_t i = 1; i < stats.skips.size(); ++i) {
    if (i != 1u) out << ", ";
    out << "\"" << PoolSkipName(static_cast<PoolSkip>(i)) << "\": " << stats.skips[i];
  }
  out << "}"
      << ", \"copied_draws\": " << stats.copied_draws
      << ", \"skipped_indirect\": " << stats.skipped_indirect
      << ", \"base_verified\": " << stats.base_verified
      << ", \"base_mismatch\": " << stats.base_mismatch
      << ", \"last_mismatch_cpu\": " << stats.last_mismatch_cpu
      << ", \"last_mismatch_gpu\": " << stats.last_mismatch_gpu
      << ", \"map_failures\": " << stats.map_failures
      << ", \"staging_failures\": " << stats.staging_failures
      << ", \"instances_seen\": " << stats.instances_seen
      << ", \"rejected_matrix\": " << stats.rejected_matrix
      << ", \"rejected_bounds\": " << stats.rejected_bounds
      << ", \"moving_instances\": " << stats.moving_instances
      << ", \"dedup_instances\": " << stats.dedup_instances
      << ", \"instance_cap_drops\": " << stats.instance_cap_drops
      << ", \"meshes\": " << stats.meshes
      << ", \"mesh_queue\": " << stats.mesh_queue
      << ", \"mesh_failures\": " << stats.mesh_failures
      << ", \"mesh_dedup\": " << stats.mesh_dedup
      << ", \"mesh_cap_drops\": " << stats.mesh_cap_drops
      << ", \"meshes_retired\": " << stats.meshes_retired
      << ", \"instances_retired\": " << stats.instances_retired
      << ", \"resource_invalidations\": " << stats.resource_invalidations
      << ", \"observed_instances\": " << stats.observed
      << ", \"admitted_instances\": " << stats.admitted
      << ", \"region_instances\": " << stats.region
      << ", \"scan_first_frame\": " << stats.scan_first_frame
      << ", \"scan_last_frame\": " << stats.scan_last_frame
      << ", \"indirect_dropped_calls\": " << IndirectDroppedCalls() << "},\n";
  {
    std::string escaped;
    for (const char c : stats.last_mesh_error) {
      if (c == '"' || c == '\\') escaped.push_back('\\');
      if (static_cast<unsigned char>(c) >= 0x20u) escaped.push_back(c);
    }
    out << "  \"last_mesh_error\": \"" << escaped << "\",\n";
  }

  out << "  \"vertex_shaders\": [";
  for (size_t i = 0; i < registry_entries.vertex.size(); ++i) {
    if (i != 0u) out << ",";
    out << "\n    {\"hash\": \"" << PoolHashText(registry_entries.vertex[i].hash) << "\", \"class\": \""
        << contract::VsClassName(static_cast<contract::VsClass>(registry_entries.vertex[i].cls)) << "\"}";
  }
  out << "\n  ],\n";
  out << "  \"pixel_shaders\": [";
  for (size_t i = 0; i < registry_entries.pixel.size(); ++i) {
    if (i != 0u) out << ",";
    out << "\n    {\"hash\": \"" << PoolHashText(registry_entries.pixel[i].hash) << "\", \"class\": \""
        << contract::PsClassName(static_cast<contract::PsClass>(registry_entries.pixel[i].cls)) << "\"}";
  }
  out << "\n  ],\n";

  out << "  \"families\": [";
  bool first = true;
  for (const auto& [vs_hash, family] : families) {
    if (!first) out << ",";
    first = false;
    const FamilyBounds& entry = bounds[vs_hash];
    out << "\n    {\"vs_hash\": \"" << PoolHashText(vs_hash) << "\""
        << ", \"class\": \"" << contract::VsClassName(static_cast<contract::VsClass>(family.vs_class)) << "\""
        << ", \"draws\": " << family.draws
        << ", \"copied\": " << family.copied
        << ", \"instances_seen\": " << family.instances_seen
        << ", \"admitted\": " << family.admitted
        << ", \"region\": " << entry.in_region
        << ", \"meshes\": " << entry.meshes
        << ", \"base_mismatch\": " << family.base_mismatch
        << ", \"rejected_matrix\": " << family.rejected_matrix
        << ", \"skips\": {";
    bool first_skip = true;
    for (size_t i = 1; i < family.skips.size(); ++i) {
      if (family.skips[i] == 0u) continue;
      if (!first_skip) out << ", ";
      first_skip = false;
      out << "\"" << PoolSkipName(static_cast<PoolSkip>(i)) << "\": " << family.skips[i];
    }
    out << "}";
    if (family.admitted != 0u) {
      out << ", \"bounds_min\": [" << entry.bounds_min[0] << ", " << entry.bounds_min[1] << ", " << entry.bounds_min[2] << "]"
          << ", \"bounds_max\": [" << entry.bounds_max[0] << ", " << entry.bounds_max[1] << ", " << entry.bounds_max[2] << "]";
    }
    out << "}";
  }
  out << "\n  ],\n";

  out << "  \"meshes\": [";
  first = true;
  for (const auto& mesh : meshes) {
    if (!first) out << ",";
    first = false;
    out << "\n    {\"mesh_id\": " << mesh.mesh_id
        << ", \"vs_hash\": \"" << PoolHashText(mesh.source_vs_hash) << "\""
        << ", \"vertices\": " << mesh.positions.size()
        << ", \"triangles\": " << mesh.triangle_count
        << ", \"live_keys\": " << mesh.live_keys
        << ", \"bbox_min\": [" << mesh.bbox_min[0] << ", " << mesh.bbox_min[1] << ", " << mesh.bbox_min[2] << "]"
        << ", \"bbox_max\": [" << mesh.bbox_max[0] << ", " << mesh.bbox_max[1] << ", " << mesh.bbox_max[2] << "]}";
  }
  out << "\n  ]\n}\n";

  std::string text = out.str();
  renodx::utils::path::WriteTextFile(PoolOutputDir() / "world_pool.json", text);
}

inline void DumpWorldPoolObj(size_t max_vertices = 2000000u) {
  std::vector<WorldMesh> meshes;
  std::vector<WorldInstance> instances;
  PoolRegion region;
  {
    std::lock_guard<std::mutex> lock(g_pool.mutex);
    meshes = g_pool.meshes;
    instances = g_pool.instances;
    region = CurrentPoolRegion();
  }

  std::ostringstream out;
  out << "# falcomengine-plus world pool (region instances, world space)\n";
  size_t vertex_base = 1u;
  size_t written = 0u;
  for (const auto& instance : instances) {
    if (!PoolInstanceInRegion(instance, region)) continue;
    if (instance.mesh_id >= meshes.size()) continue;
    const WorldMesh& mesh = meshes[instance.mesh_id];
    if (written + mesh.positions.size() > max_vertices) break;
    for (const auto& position : mesh.positions) {
      float world[3] = {};
      TransformPoolPoint(instance.kind, instance.matrix, position.data(), world);
      out << "v " << world[0] << " " << world[1] << " " << world[2] << "\n";
    }
    for (size_t i = 0u; i + 2u < mesh.indices.size(); i += 3u) {
      out << "f " << (vertex_base + mesh.indices[i])
          << " " << (vertex_base + mesh.indices[i + 1u])
          << " " << (vertex_base + mesh.indices[i + 2u]) << "\n";
    }
    vertex_base += mesh.positions.size();
    written += mesh.positions.size();
  }
  std::string text = out.str();
  renodx::utils::path::WriteTextFile(PoolOutputDir() / "world_pool_region.obj", text);
}

}  // namespace falcom_world::bvh
