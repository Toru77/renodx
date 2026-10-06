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
// Indirect draws keep their index and instance counts in a GPU buffer, so at
// the draw the pool copies b1, the 20-byte args and a window of elements from
// base; resolve reads the counts from the copied args and uses the first
// instance_count elements of the window. The window adapts per draw identity:
// what the draw used last time plus a margin, a small window after empty
// args, doubled after a truncation, at most kPoolIndirectWindow.
//
// An instance is admitted once the same (mesh, world matrix) pair was read in
// two different frames, which keeps moving objects out. Its matrix must be
// plausible: finite, axes between kPoolMinScale and kPoolMaxScale and not
// parallel, translation within +-kPoolMaxPosition (objects parked far away
// are placeholders or horizon pieces).
//
// Meshes are read without waiting for the GPU, in two copies made at draws
// that bind the mesh's buffers (bound buffers are alive): first the draw's
// index range, then, once those indices are read two presents later, the
// vertex range they cover. Both go to a per-slot mesh staging buffer of the
// same ring and are decoded at present outside the lock. Copies are taken
// only at draws on the immediate context: a deferred command list may run
// after its slot is read, and a mesh has no check like b1 that would catch
// stale bytes. Meshes are merged
// by content and retired with their instances when their vertex or index
// buffer is destroyed, so a map change empties the pool instead of leaving
// the previous map's geometry behind.
//
// Diagnostic switches (BVH panel): "Capture meshes" stops new mesh copies
// (instances are still observed; new meshes wait), "Scan indirect draws"
// ignores indirect draws, and "Log mesh captures" writes ReShade.log lines
// for each mesh copy and read and when tracked buffers are released. Nothing
// is logged unless that last switch is on. Each pool entry
// point also records its stage per thread (debug/pool_stage.hpp) for the
// crash log (debug/crash_log.hpp, "Log crashes").
//
// Lock rule: no graphics API call while g_pool.mutex is held. A D3D11 call
// can run deferred object destruction on the calling thread, which raises
// destroy_resource, whose handler (OnDestroyResourcePool) takes this mutex
// again (MSVC std::mutex then throws, and the uncaught exception aborts the
// game). The draw-time copies are therefore reserved under the lock and
// recorded after it is released. Verified in game 2026-10-06: recording them
// under the lock crashed (ucrtbase 0xC0000409); after the lock, no crash.

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

#include "../../../../utils/log.hpp"
#include "../../../../utils/path.hpp"
#include "../../../../utils/scene.hpp"
#include "../capture/buffer_readback.hpp"
#include "../capture/cb_tracking.hpp"
#include "../capture/cb_value_tracker.hpp"
#include "../contract/shader_registry.hpp"
#include "../debug/pool_stage.hpp"
#include "../world_state.hpp"
#include "pool_transform.hpp"

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
inline constexpr uint64_t kPoolIndirectArgsBytes = 32u;     // 20-byte indexed args, padded to 16
inline constexpr uint32_t kPoolIndirectArgsCopyBytes = 20u;
inline constexpr uint32_t kPoolIndirectWindow = 512u;       // most instances copied per indirect draw
inline constexpr uint32_t kPoolIndirectEmptyWindow = 16u;   // window after the args came back empty
inline constexpr uint32_t kPoolIndirectWindowStep = 16u;    // windows are rounded up to this
inline constexpr uint32_t kPoolMaxIndirectSubDraws = 64u;
inline constexpr uint32_t kPoolMaxRejectSamples = 2u;       // per family, for the dump
inline constexpr size_t kPoolMaxNearIndex = 500000u;
inline constexpr uint64_t kPoolMeshSlotBytes = 4ull * 1024ull * 1024ull;  // mesh copies per slot (frame)
inline constexpr uint32_t kPoolMeshCopiesPerFrame = 64u;
inline constexpr uint32_t kPoolMeshCopiesPerDraw = 4u;
inline constexpr float kPoolMinScale = 0.001f;     // shorter axes: collapsed or hidden instances
inline constexpr float kPoolMaxScale = 10000.f;
inline constexpr float kPoolMaxPosition = 50000.f;  // farther translations: parked or horizon pieces
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
  DrawState,         // rigid VS, but not an opaque depth-writing indexed triangle list (see PoolDrawState)
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

// First failing condition of the pool draw-state gate, in check order.
enum class PoolDrawState : uint8_t {
  Ok = 0,
  NotIndexed,     // Draw / DrawInstanced(Indirect)
  NoBuffers,      // no vertex or index buffer bound
  NoDepthTarget,  // no depth-stencil view bound
  IndexCount,     // fewer than 3 indices, or not a multiple of 3
  Blend,          // blending on render target 0 (transparent, decals, clouds)
  NoDepthTest,
  NoDepthWrite,
  Topology,       // not a triangle list
  Count,
};

inline const char* PoolDrawStateName(PoolDrawState state) {
  switch (state) {
    case PoolDrawState::NotIndexed:    return "not_indexed";
    case PoolDrawState::NoBuffers:     return "no_buffers";
    case PoolDrawState::NoDepthTarget: return "no_depth_target";
    case PoolDrawState::IndexCount:    return "index_count";
    case PoolDrawState::Blend:         return "blend";
    case PoolDrawState::NoDepthTest:   return "no_depth_test";
    case PoolDrawState::NoDepthWrite:  return "no_depth_write";
    case PoolDrawState::Topology:      return "topology";
    default:                           return "ok";
  }
}

// Why a world matrix failed the plausibility check (first failing test).
enum class PoolMatrixReject : uint8_t {
  None = 0,
  NonFinite,
  ZeroScale,   // an axis shorter than 1e-6: collapsed / hidden instance
  FarAway,     // translation beyond +-kPoolMaxPosition: parked placeholder or horizon piece
  SmallScale,  // an axis shorter than kPoolMinScale
  LargeScale,  // an axis kPoolMaxScale or longer
  Skewed,      // two axes nearly parallel
  Count,
};

inline const char* PoolMatrixRejectName(PoolMatrixReject reject) {
  switch (reject) {
    case PoolMatrixReject::NonFinite:  return "non_finite";
    case PoolMatrixReject::ZeroScale:  return "zero_scale";
    case PoolMatrixReject::FarAway:    return "far_away";
    case PoolMatrixReject::SmallScale: return "small_scale";
    case PoolMatrixReject::LargeScale: return "large_scale";
    case PoolMatrixReject::Skewed:     return "skewed";
    default:                           return "none";
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

// Positions and triangles of one decoded mesh (vertices remapped in first-use order).
struct PoolDecodedMesh {
  std::vector<std::array<float, 3>> positions;
  std::vector<std::array<uint32_t, 3>> triangles;
  std::array<float, 3> bbox_min = {0.f, 0.f, 0.f};
  std::array<float, 3> bbox_max = {0.f, 0.f, 0.f};
};

// What the next copy of a queued mesh reads.
enum class PoolMeshPhase : uint8_t {
  Indices = 0,  // the draw's index range
  Vertices,     // the vertex range those indices cover
};

inline const char* PoolMeshPhaseName(PoolMeshPhase phase) {
  return phase == PoolMeshPhase::Indices ? "indices" : "vertices";
}

// One queued mesh: the draw that first showed the geometry, its position
// layout and, after the index read, the vertices it uses.
struct PoolMeshRequest {
  uint64_t mesh_key = 0u;
  uint64_t serial = 0u;      // copies in flight carry it; a re-queued key gets a new one
  uint64_t buffer_key = 0u;  // the VB/IB bindings a draw must have to serve its copies
  uint32_t vs_hash = 0u;
  uint32_t last_frame = 0u;  // queued, last sighting, or last copy issued or read
  DrawRecord draw;
  int32_t pos_offset = 0;
  reshade::api::format pos_format = reshade::api::format::unknown;
  PoolMeshPhase phase = PoolMeshPhase::Indices;
  bool in_flight = false;
  std::vector<uint32_t> indices;  // absolute vertex indices, after the index read
  uint32_t min_vertex = 0u;
  uint32_t max_vertex = 0u;
};

// One mesh copy in a staging slot.
struct PoolMeshCopy {
  uint64_t mesh_key = 0u;
  uint64_t serial = 0u;
  PoolMeshPhase phase = PoolMeshPhase::Indices;
  uint64_t staging_offset = 0u;
  uint64_t skip = 0u;    // bytes before the wanted range (the source is copied 4-byte aligned)
  uint64_t size = 0u;    // wanted bytes
  uint64_t copied = 0u;  // bytes copied
  uint64_t source = 0u;  // VB or IB handle, for the log
  uint64_t source_offset = 0u;
};

struct PoolPendingCopy {
  uint64_t cb_offset = 0u;
  uint64_t slice_offset = 0u;
  uint32_t count = 0u;  // instances copied (the window for an indirect draw)
  int32_t cpu_base = 0;
  uint32_t frame = 0u;
  uint32_t vs_hash = 0u;
  uint64_t mesh_key = 0u;  // 0 for an indirect draw until its args are read
  // Indirect draws only: the draw record in PoolStagingSlot::indirect_draws
  // (its counts are filled from the args at resolve), the args location and
  // the draw identity whose next window the args decide.
  uint32_t indirect_index = UINT32_MAX;
  uint64_t args_offset = 0u;
  uint64_t schedule_key = 0u;
};

struct PoolStagingSlot {
  reshade::api::resource buffer = {0u};
  uint64_t used = 0u;
  bool resolving = false;
  std::vector<PoolPendingCopy> copies;
  std::vector<DrawRecord> indirect_draws;
  // Mesh copies of the same frame, in their own buffer.
  reshade::api::resource mesh_buffer = {0u};
  uint64_t mesh_used = 0u;
  std::vector<PoolMeshCopy> mesh_copies;
};

struct PoolMatrixSample {
  uint8_t reason = 0u;  // PoolMatrixReject (None for near-miss samples)
  float matrix[kPoolWorldFloats] = {};
};

// First sighting of a (mesh, rounded placement); a later sighting with a
// different exact matrix is a near miss (see ObservePoolInstance).
struct PoolNearEntry {
  uint64_t matrix_hash = 0u;
  float matrix[kPoolWorldFloats] = {};
};

struct PoolSchedule {
  uint32_t copy_frame = 0u;
  uint32_t next_frame = 0u;
  uint32_t indirect_window = 0u;  // indirect draws: next window (0 = not known yet)
};

struct PoolFamilyStats {
  uint8_t vs_class = 0u;
  uint64_t draws = 0u;
  uint64_t indirect_draws = 0u;
  uint64_t copied = 0u;
  uint64_t instances_seen = 0u;
  uint32_t admitted = 0u;
  uint32_t base_mismatch = 0u;
  uint32_t rejected_matrix = 0u;
  uint32_t near_misses = 0u;
  float near_miss_max_delta = 0.f;  // largest element difference among near misses
  std::array<uint64_t, static_cast<size_t>(PoolSkip::Count)> skips = {};
  std::array<uint64_t, static_cast<size_t>(PoolDrawState::Count)> draw_state = {};
  std::array<uint32_t, static_cast<size_t>(PoolMatrixReject::Count)> matrix_rejects = {};
  std::vector<PoolMatrixSample> reject_samples;
  std::vector<PoolMatrixSample> near_miss_samples;  // pairs: first sighting, then the near miss
};

struct PoolStats {
  // Draw admission (counted per scanned draw).
  std::array<uint64_t, static_cast<size_t>(contract::VsClass::Count)> draws_by_vs_class = {};
  std::array<uint64_t, static_cast<size_t>(PoolSkip::Count)> skips = {};
  std::array<uint64_t, static_cast<size_t>(PoolDrawState::Count)> draw_state = {};
  uint64_t copied_draws = 0u;
  // Scanned draws recorded on a command list other than the immediate one
  // (deferred contexts); direct and indirect.
  uint64_t draws_on_deferred = 0u;
  uint64_t indirect_on_deferred = 0u;
  // Indirect draws (sub-draws seen while scanning; also counted above).
  std::array<uint64_t, static_cast<size_t>(contract::VsClass::Count)> indirect_by_vs_class = {};
  uint64_t indirect_copied = 0u;
  uint64_t indirect_resolved = 0u;
  uint64_t indirect_empty = 0u;      // args with zero indices or instances
  uint64_t indirect_truncated = 0u;  // instance_count larger than the copied window
  uint64_t indirect_dead = 0u;       // VB/IB released before the args were read
  uint64_t indirect_window_instances = 0u;  // instances copied for indirect draws (sum of windows)
  // Read back.
  uint64_t base_verified = 0u;
  uint64_t base_mismatch = 0u;
  int32_t last_mismatch_cpu = 0;
  int32_t last_mismatch_gpu = 0;
  uint32_t map_failures = 0u;
  uint32_t staging_failures = 0u;
  uint64_t instances_seen = 0u;
  uint64_t rejected_matrix = 0u;
  std::array<uint64_t, static_cast<size_t>(PoolMatrixReject::Count)> matrix_rejects = {};
  uint64_t near_misses = 0u;
  uint64_t moving_instances = 0u;  // world != prevWorld at the draw (diagnostic)
  uint32_t rejected_bounds = 0u;
  uint32_t dedup_instances = 0u;
  uint32_t instance_cap_drops = 0u;
  // Meshes.
  uint64_t mesh_index_copies = 0u;   // index ranges copied
  uint64_t mesh_vertex_copies = 0u;  // vertex ranges copied
  uint64_t mesh_copy_bytes = 0u;
  uint32_t mesh_budget_full = 0u;    // copies postponed: the frame's mesh staging was full
  uint32_t mesh_dropped = 0u;        // copies read after their mesh was released or re-queued
  uint32_t mesh_expired = 0u;        // requests not drawn for kPoolPruneAge frames
  uint64_t mesh_deferred_skips = 0u; // draws on a deferred context that could have served a copy
  uint32_t mesh_failures = 0u;
  uint32_t mesh_dedup = 0u;
  uint32_t mesh_cap_drops = 0u;
  uint32_t meshes_retired = 0u;
  uint32_t instances_retired = 0u;
  uint32_t resource_invalidations = 0u;
  std::string last_mesh_error;
  // Sizes (refreshed by UpdatePoolStats).
  size_t queued = 0u;
  size_t mesh_queue = 0u;      // waiting for a copy
  size_t mesh_in_flight = 0u;  // copy issued, not read yet
  size_t meshes = 0u;
  size_t observed = 0u;
  size_t admitted = 0u;
  size_t region = 0u;
  uint64_t scan_first_frame = 0u;
  uint64_t scan_last_frame = 0u;
};

struct PoolState {
  std::atomic_bool scan_active{false};
  // Diagnostic switches (see the header comment).
  std::atomic_bool capture_meshes{true};
  std::atomic_bool scan_indirect{true};
  std::atomic_bool log_captures{false};
  // The immediate command list (set at present), to tell deferred-context draws apart.
  std::atomic_uint64_t immediate_cmd_list{0u};
  float region_size = 512.f;
  std::mutex mutex;

  uint32_t logged_invalidations = 0u;  // stats.resource_invalidations at the last present

  // Staging ring. Draws of the frame in flight write `write_slot`.
  reshade::api::device* staging_device = nullptr;
  std::array<PoolStagingSlot, kPoolStagingSlots> slots;
  uint32_t write_slot = 0u;

  // Meshes.
  std::unordered_map<uint64_t, PoolMeshRequest> mesh_requests;       // by mesh key
  std::unordered_map<uint64_t, std::vector<uint64_t>> mesh_waiting;  // buffer key -> keys waiting for a copy
  uint64_t next_mesh_serial = 1u;
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
  std::unordered_map<uint64_t, PoolNearEntry> near_index;  // (mesh, rounded placement) -> first sighting

  // VB/IB handles referenced by indirect copies not read back yet, and the
  // ones among them released meanwhile (their mesh must not be captured).
  std::unordered_map<uint64_t, uint32_t> indirect_refs;
  std::unordered_set<uint64_t> indirect_dead;

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
// length in (0.05, 50) and no two axes nearly parallel. Returns the first
// failing test.
inline PoolMatrixReject CheckPoolWorld(const float* matrix) {
  for (uint32_t i = 0; i < kPoolWorldFloats; ++i) {
    if (!std::isfinite(matrix[i])) return PoolMatrixReject::NonFinite;
  }
  const float rows[3][3] = {
      {matrix[0], matrix[1], matrix[2]},
      {matrix[4], matrix[5], matrix[6]},
      {matrix[8], matrix[9], matrix[10]},
  };
  float lengths[3] = {};
  for (int r = 0; r < 3; ++r) {
    lengths[r] = std::sqrt(rows[r][0] * rows[r][0] + rows[r][1] * rows[r][1] + rows[r][2] * rows[r][2]);
    if (!(lengths[r] >= 1e-6f)) return PoolMatrixReject::ZeroScale;
  }
  // Parked placeholders and horizon pieces sit 100 km out (seen at -100000 on
  // one axis and on a 100 km ring); real scene objects are far inside.
  for (const uint32_t element : {3u, 7u, 11u}) {
    if (!(std::fabs(matrix[element]) <= kPoolMaxPosition)) return PoolMatrixReject::FarAway;
  }
  for (int r = 0; r < 3; ++r) {
    if (!(lengths[r] >= kPoolMinScale)) return PoolMatrixReject::SmallScale;
    if (!(lengths[r] < kPoolMaxScale)) return PoolMatrixReject::LargeScale;
  }
  for (int a = 0; a < 3; ++a) {
    for (int b = a + 1; b < 3; ++b) {
      const float dot = (rows[a][0] * rows[b][0] + rows[a][1] * rows[b][1] + rows[a][2] * rows[b][2])
                        / (lengths[a] * lengths[b]);
      if (!(std::fabs(dot) < 0.995f)) return PoolMatrixReject::Skewed;
    }
  }
  return PoolMatrixReject::None;
}

inline bool PoolWorldPlausible(const float* matrix) {
  return CheckPoolWorld(matrix) == PoolMatrixReject::None;
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
  size_t in_flight = 0u;
  for (const auto& [key, request] : g_pool.mesh_requests) {
    (void)key;
    if (request.in_flight) in_flight += 1u;
  }
  g_pool.stats.mesh_in_flight = in_flight;
  g_pool.stats.mesh_queue = g_pool.mesh_requests.size() - in_flight;
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
// objects outside the camera view. An indirect draw's index count is only
// known once its args are read back, so it is checked there.
inline PoolDrawState CheckPoolDrawState(const DrawRecord& draw, bool check_index_count) {
  if (draw.method != 1u || !draw.has_index_buffer) return PoolDrawState::NotIndexed;
  if (draw.vb.handle == 0u || draw.ib.handle == 0u) return PoolDrawState::NoBuffers;
  if (draw.dsv.handle == 0u) return PoolDrawState::NoDepthTarget;
  if (check_index_count && (draw.index_count < 3u || (draw.index_count % 3u) != 0u)) {
    return PoolDrawState::IndexCount;
  }
  if (draw.blend_enable) return PoolDrawState::Blend;
  if (!draw.depth_enable) return PoolDrawState::NoDepthTest;
  if (!draw.depth_write) return PoolDrawState::NoDepthWrite;
  if (draw.topology != reshade::api::primitive_topology::undefined
      && draw.topology != reshade::api::primitive_topology::triangle_list) {
    return PoolDrawState::Topology;
  }
  return PoolDrawState::Ok;
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

// Rate-limit identity of an indirect draw: its buffers and args location
// (the counts themselves are not known at the draw).
inline uint64_t PoolIndirectScheduleKey(const DrawRecord& draw, reshade::api::resource args_buffer, uint64_t args_offset) {
  uint64_t key = PoolMix(1469598103934665603ull, draw.vb.handle);
  key = PoolMix(key, draw.vb_offset);
  key = PoolMix(key, draw.ib.handle);
  key = PoolMix(key, draw.ib_offset);
  key = PoolMix(key, args_buffer.handle);
  return PoolMix(key, args_offset);
}

// (mesh, placement rounded to 1 cm and 1e-3 of the basis): two sightings with
// the same near key but different exact matrices are almost the same
// placement, which the exact-match stability rule would never admit.
inline uint64_t PoolNearKey(uint64_t mesh_key, const float* world) {
  uint64_t key = PoolMix(1469598103934665603ull, mesh_key);
  for (int row = 0; row < 3; ++row) {
    for (int column = 0; column < 3; ++column) {
      key = PoolMix(key, static_cast<uint64_t>(std::llround(static_cast<double>(world[row * 4 + column]) * 1000.0)));
    }
    key = PoolMix(key, static_cast<uint64_t>(std::llround(static_cast<double>(world[row * 4 + 3]) * 100.0)));
  }
  return key;
}

// ---------------------------------------------------------------------------
// Staging ring.

// Staging buffers are created and destroyed outside g_pool.mutex: resource
// events raised by those calls must never re-enter a held pool lock.

// Caller holds g_pool.mutex. An indirect copy keeps its VB/IB handles until
// its args are read; a release in between marks them dead so no mesh is
// captured from a destroyed buffer.
inline void AddPoolIndirectRefs(const DrawRecord& draw) {
  g_pool.indirect_refs[draw.vb.handle] += 1u;
  if (draw.ib.handle != draw.vb.handle) g_pool.indirect_refs[draw.ib.handle] += 1u;
}

// Caller holds g_pool.mutex. Returns true when one of the buffers was
// released after the draw.
inline bool ReleasePoolIndirectRefs(const DrawRecord& draw) {
  bool dead = false;
  const uint64_t handles[2] = {draw.vb.handle, draw.ib.handle};
  for (uint32_t i = 0; i < 2u; ++i) {
    const uint64_t handle = handles[i];
    if (i == 1u && handle == handles[0]) break;
    if (g_pool.indirect_dead.count(handle) != 0u) dead = true;
    const auto it = g_pool.indirect_refs.find(handle);
    if (it == g_pool.indirect_refs.end()) continue;
    if (it->second > 1u) {
      it->second -= 1u;
    } else {
      g_pool.indirect_refs.erase(it);
      g_pool.indirect_dead.erase(handle);
    }
  }
  return dead;
}

// Caller holds g_pool.mutex. Detaches the ring and returns its buffers.
// Meshes whose copies are dropped with it go back to waiting.
inline std::vector<reshade::api::resource> DetachPoolStaging(reshade::api::device** out_device) {
  std::vector<reshade::api::resource> buffers;
  for (auto& slot : g_pool.slots) {
    if (slot.buffer.handle != 0u) buffers.push_back(slot.buffer);
    if (slot.mesh_buffer.handle != 0u) buffers.push_back(slot.mesh_buffer);
    for (const DrawRecord& draw : slot.indirect_draws) ReleasePoolIndirectRefs(draw);
    for (const PoolMeshCopy& copy : slot.mesh_copies) {
      const auto request = g_pool.mesh_requests.find(copy.mesh_key);
      if (request == g_pool.mesh_requests.end() || request->second.serial != copy.serial) continue;
      request->second.in_flight = false;
      g_pool.mesh_waiting[request->second.buffer_key].push_back(copy.mesh_key);
    }
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
  // Per slot: the instance buffer, then the mesh buffer.
  std::vector<reshade::api::resource> created;
  for (uint32_t i = 0; i < 2u * kPoolStagingSlots; ++i) {
    const reshade::api::resource_desc desc(
        (i % 2u) == 0u ? kPoolStagingSlotBytes : kPoolMeshSlotBytes, reshade::api::memory_heap::gpu_to_cpu,
        reshade::api::resource_usage::copy_dest);
    reshade::api::resource buffer = {0u};
    if (!device->create_resource(desc, nullptr, reshade::api::resource_usage::copy_dest, &buffer)) break;
    created.push_back(buffer);
  }
  if (created.size() != 2u * kPoolStagingSlots) {
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
    for (uint32_t i = 0; i < kPoolStagingSlots; ++i) {
      g_pool.slots[i].buffer = created[2u * i];
      g_pool.slots[i].mesh_buffer = created[2u * i + 1u];
    }
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
  if (observed.count == 0u) {
    // Diagnostic only: a new exact matrix next to an earlier one for the same
    // mesh. Counts objects whose matrix jitters instead of staying identical.
    const auto [near_it, inserted] = g_pool.near_index.try_emplace(PoolNearKey(mesh_key, world));
    if (inserted) {
      near_it->second.matrix_hash = key.matrix_hash;
      std::memcpy(near_it->second.matrix, world, sizeof(float) * kPoolWorldFloats);
    } else if (near_it->second.matrix_hash != key.matrix_hash) {
      PoolFamilyStats& family = g_pool.families[vs_hash];
      g_pool.stats.near_misses += 1u;
      family.near_misses += 1u;
      for (uint32_t i = 0; i < kPoolWorldFloats; ++i) {
        family.near_miss_max_delta = (std::max)(family.near_miss_max_delta, std::fabs(world[i] - near_it->second.matrix[i]));
      }
      if (family.near_miss_samples.size() < 2u * kPoolMaxRejectSamples) {
        PoolMatrixSample first;
        std::memcpy(first.matrix, near_it->second.matrix, sizeof(float) * kPoolWorldFloats);
        PoolMatrixSample second;
        std::memcpy(second.matrix, world, sizeof(float) * kPoolWorldFloats);
        family.near_miss_samples.push_back(first);
        family.near_miss_samples.push_back(second);
      }
    }
    if (g_pool.near_index.size() > kPoolMaxNearIndex) g_pool.near_index.clear();
  }
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

struct PoolDrawGate {
  contract::VsClass vs_class = contract::VsClass::Unclassified;
  PoolSkip skip = PoolSkip::None;
  PoolDrawState state = PoolDrawState::Ok;
};

// Shader classes first, then draw state, then the pixel shader.
inline PoolDrawGate GatePoolDraw(const DrawRecord& draw, bool check_index_count) {
  PoolDrawGate gate;
  gate.vs_class = contract::LookupVertexClass(draw.vs_pipeline);
  if (gate.vs_class != contract::VsClass::Rigid) {
    gate.skip = PoolSkip::NotRigid;
    return gate;
  }
  gate.state = CheckPoolDrawState(draw, check_index_count);
  if (gate.state != PoolDrawState::Ok) {
    gate.skip = PoolSkip::DrawState;
    return gate;
  }
  const contract::PsClass ps_class = contract::LookupPixelClass(draw.ps_pipeline);
  if (ps_class == contract::PsClass::AlphaTested) {
    gate.skip = PoolSkip::AlphaTested;
  } else if (ps_class != contract::PsClass::Opaque) {
    gate.skip = PoolSkip::PixelUnknown;
  }
  return gate;
}

// The draw's instance source: t15, b1 and the elements available from the
// CPU-mirrored base to the end of t15. D3D11 SV_InstanceID starts at 0
// whatever StartInstanceLocation is (that only offsets per-instance vertex
// streams), so a draw reads [instanceOffset_g, + instance_count).
struct PoolSlice {
  reshade::api::resource instance_buffer = {0u};
  reshade::api::resource instance_cb = {0u};
  int32_t base = 0;
  uint64_t cb_bytes = 0u;
  uint64_t available = 0u;  // elements from base to the end of t15
};

inline PoolSkip ResolvePoolSlice(
    reshade::api::device* device,
    reshade::api::command_list* cmd_list,
    const WorldCommandListData* cl_data,
    PoolSlice* slice) {
  slice->instance_buffer = cl_data->vs_srv[kPoolInstanceSlot];
  slice->instance_cb = cl_data->vs_cb[kPoolInstanceCbSlot];
  if (slice->instance_buffer.handle == 0u) return PoolSkip::NoInstanceSrv;
  if (slice->instance_cb.handle == 0u || !ReadTrackedCbInt(cmd_list, slice->instance_cb, &slice->base)) {
    return PoolSkip::NoCpuBase;
  }
  const auto cb_desc = device->get_resource_desc(slice->instance_cb);
  slice->cb_bytes = cb_desc.type == reshade::api::resource_type::buffer
                        ? (std::min)(cb_desc.buffer.size, kPoolCbCopyBytes)
                        : 0u;
  if (slice->cb_bytes < sizeof(int32_t)) return PoolSkip::NoCpuBase;
  const auto buffer_desc = device->get_resource_desc(slice->instance_buffer);
  const uint64_t elements = buffer_desc.type == reshade::api::resource_type::buffer
                                ? buffer_desc.buffer.size / kPoolInstanceStride
                                : 0u;
  if (slice->base < 0 || static_cast<uint64_t>(slice->base) >= elements) return PoolSkip::SliceRange;
  slice->available = elements - static_cast<uint64_t>(slice->base);
  return PoolSkip::None;
}

// Caller holds g_pool.mutex.
inline void CountPoolSkip(PoolFamilyStats& family, PoolSkip skip, PoolDrawState state) {
  g_pool.stats.skips[static_cast<size_t>(skip)] += 1u;
  family.skips[static_cast<size_t>(skip)] += 1u;
  if (skip == PoolSkip::DrawState) {
    g_pool.stats.draw_state[static_cast<size_t>(state)] += 1u;
    family.draw_state[static_cast<size_t>(state)] += 1u;
  }
}

// Caller holds g_pool.mutex. Applies the per-identity cooldown and checks the
// write slot; on None, `*out_schedule` is the identity to stamp after copying.
inline PoolSkip ReservePoolCopy(
    reshade::api::device* device,
    uint64_t schedule_key,
    uint32_t frame,
    uint64_t bytes,
    PoolSchedule** out_schedule) {
  PoolSchedule& schedule = g_pool.schedule[schedule_key];
  const PoolStagingSlot& slot = g_pool.slots[g_pool.write_slot];
  if (schedule.next_frame != 0u && schedule.copy_frame != frame && frame < schedule.next_frame) {
    return PoolSkip::Cooldown;
  }
  if (g_pool.staging_device != device || slot.buffer.handle == 0u || slot.resolving) return PoolSkip::NoStaging;
  if (slot.used + bytes > kPoolStagingSlotBytes) return PoolSkip::BudgetFull;
  *out_schedule = &schedule;
  return PoolSkip::None;
}

// The VB/IB bindings of a draw: any draw that has them bound can serve the
// mesh copies of every queued mesh drawn from them.
inline uint64_t PoolBufferKey(const DrawRecord& draw) {
  uint64_t key = 1469598103934665603ull;
  key = PoolMix(key, draw.vb.handle);
  key = PoolMix(key, draw.vb_offset);
  key = PoolMix(key, draw.vb_stride);
  key = PoolMix(key, draw.ib.handle);
  key = PoolMix(key, draw.ib_offset);
  return PoolMix(key, draw.index_size);
}

// Caller holds g_pool.mutex.
inline void AddPoolMeshWaiting(const PoolMeshRequest& request) {
  g_pool.mesh_waiting[request.buffer_key].push_back(request.mesh_key);
}

// Caller holds g_pool.mutex.
inline void RemovePoolMeshWaiting(uint64_t buffer_key, uint64_t mesh_key) {
  const auto it = g_pool.mesh_waiting.find(buffer_key);
  if (it == g_pool.mesh_waiting.end()) return;
  auto& keys = it->second;
  keys.erase(std::remove(keys.begin(), keys.end(), mesh_key), keys.end());
  if (keys.empty()) g_pool.mesh_waiting.erase(it);
}

// Caller holds g_pool.mutex. Records a mesh failure; the key stays queued
// (not retried) until its buffers are released.
inline void RecordPoolMeshFailure(uint64_t mesh_key, const char* reason) {
  g_pool.failed_meshes.insert(mesh_key);
  g_pool.stats.mesh_failures += 1u;
  g_pool.stats.last_mesh_error = reason;
}

// Caller holds g_pool.mutex, and has already taken the key out of
// mesh_waiting (or it is in flight).
inline void FailPoolMesh(std::unordered_map<uint64_t, PoolMeshRequest>::iterator request, const char* reason) {
  RecordPoolMeshFailure(request->first, reason);
  g_pool.mesh_requests.erase(request);
}

// Position layout of a draw's vertex stream 0, from its input layout.
// Returns nullptr when usable, else why not.
inline const char* ResolvePoolMeshLayout(
    const DrawRecord& draw, int32_t* pos_offset, reshade::api::format* pos_format) {
  namespace scene = renodx::utils::scene;
  if (draw.method != 1u || !draw.has_index_buffer || draw.vb.handle == 0u || draw.ib.handle == 0u) {
    return "not an indexed draw";
  }
  if (draw.index_size != 2u && draw.index_size != 4u) return "unsupported index size";
  if (draw.index_count < 3u || (draw.index_count % 3u) != 0u) return "index count is not a triangle list";
  if (draw.vb_stride == 0u) return "vertex stride is zero";
  scene::InputLayoutInfo layout_info;
  bool found = false;
  if (scene::shared.data != nullptr && draw.input_layout.handle != 0u) {
    scene::shared.data->input_layouts.if_contains(draw.input_layout.handle, [&](const auto& pair) {
      layout_info = pair.second;
      found = true;
    });
  }
  if (!found) return "no input layout";
  scene::MeshLayout layout;
  if (!scene::BuildMeshLayout(layout_info, draw.vb_stride, draw.index_size, &layout)) return "input layout not decodable";
  if (layout.topology != reshade::api::primitive_topology::undefined
      && layout.topology != reshade::api::primitive_topology::triangle_list) {
    return "not a triangle list";
  }
  const scene::FormatInfo* info = scene::FindFormatInfo(layout.pos_format);
  if (info == nullptr) return "unsupported position format";
  if (layout.pos_off < 0 || static_cast<uint64_t>(layout.pos_off) + info->byte_size > draw.vb_stride) {
    return "position outside the vertex";
  }
  *pos_offset = layout.pos_off;
  *pos_format = layout.pos_format;
  return nullptr;
}

// Caller holds g_pool.mutex. Queues the mesh a draw shows, once per key until
// its buffers are released.
// Caller holds g_pool.mutex. Maps a VB/IB handle to a draw key once (a key
// queued again after it expired is already mapped).
inline void AddPoolResourceKey(uint64_t handle, uint64_t mesh_key) {
  auto& keys = g_pool.keys_by_resource[handle];
  if (std::find(keys.begin(), keys.end(), mesh_key) == keys.end()) keys.push_back(mesh_key);
}

inline void QueuePoolMesh(uint64_t mesh_key, uint32_t vs_hash, const DrawRecord& draw) {
  if (!g_pool.mesh_queued.insert(mesh_key).second) {
    // Seen again: a waiting mesh is still drawn, so it does not expire.
    const auto request = g_pool.mesh_requests.find(mesh_key);
    if (request != g_pool.mesh_requests.end()) request->second.last_frame = g_state.frame.load();
    return;
  }
  if (g_pool.mesh_by_key.count(mesh_key) != 0u || g_pool.failed_meshes.count(mesh_key) != 0u) return;
  AddPoolResourceKey(draw.vb.handle, mesh_key);
  if (draw.ib.handle != draw.vb.handle) AddPoolResourceKey(draw.ib.handle, mesh_key);

  PoolMeshRequest request;
  request.mesh_key = mesh_key;
  request.vs_hash = vs_hash;
  request.last_frame = g_state.frame.load();
  request.draw = draw;
  const char* error = ResolvePoolMeshLayout(draw, &request.pos_offset, &request.pos_format);
  if (error != nullptr) {
    RecordPoolMeshFailure(mesh_key, error);
    return;
  }
  request.serial = g_pool.next_mesh_serial++;
  request.buffer_key = PoolBufferKey(draw);
  AddPoolMeshWaiting(request);
  g_pool.mesh_requests.emplace(mesh_key, std::move(request));
}

// One draw-time copy, reserved under g_pool.mutex and recorded after it.
// Trivial on purpose: arrays of it are not cleared, only [0, count) is used.
struct PoolCopyCommand {
  reshade::api::resource source;
  uint64_t source_offset;
  reshade::api::resource dest;
  uint64_t dest_offset;
  uint64_t size;
};

inline void IssuePoolCopies(reshade::api::command_list* cmd_list, const PoolCopyCommand* commands, uint32_t count) {
  for (uint32_t i = 0; i < count; ++i) {
    const PoolCopyCommand& command = commands[i];
    cmd_list->copy_buffer_region(command.source, command.source_offset, command.dest, command.dest_offset, command.size);
  }
}

// Caller holds g_pool.mutex. Reserves copies for meshes waiting on the
// buffers this draw has bound. A bound buffer is alive, so copying from it at
// the draw is safe (the instance copies rely on the same). Draws on a
// deferred context do not serve copies (see the header comment). Writes up
// to `capacity` commands plus the matching copies (for the log); returns how
// many.
inline uint32_t ReservePoolMeshCopies(
    reshade::api::device* device,
    const DrawRecord& draw,
    bool deferred,
    uint32_t frame,
    PoolCopyCommand* commands,
    PoolMeshCopy* copies,
    uint32_t capacity) {
  if (capacity == 0u || g_pool.mesh_waiting.empty()) return 0u;
  if (!g_pool.capture_meshes.load(std::memory_order_relaxed)) return 0u;
  if (draw.method != 1u || !draw.has_index_buffer || draw.vb.handle == 0u || draw.ib.handle == 0u) return 0u;
  if (draw.vb_size == 0u || draw.ib_size == 0u) return 0u;
  const auto waiting = g_pool.mesh_waiting.find(PoolBufferKey(draw));
  if (waiting == g_pool.mesh_waiting.end()) return 0u;
  if (deferred) {
    g_pool.stats.mesh_deferred_skips += 1u;
    return 0u;
  }
  PoolStagingSlot& slot = g_pool.slots[g_pool.write_slot];
  if (g_pool.staging_device != device || slot.mesh_buffer.handle == 0u || slot.resolving) return 0u;

  std::vector<uint64_t>& keys = waiting->second;
  uint32_t count = 0u;
  size_t index = 0u;
  while (index < keys.size() && count < capacity && slot.mesh_copies.size() < kPoolMeshCopiesPerFrame) {
    const auto request_it = g_pool.mesh_requests.find(keys[index]);
    if (request_it == g_pool.mesh_requests.end() || request_it->second.in_flight) {
      keys.erase(keys.begin() + static_cast<std::ptrdiff_t>(index));
      continue;
    }
    PoolMeshRequest& request = request_it->second;
    const DrawRecord& queued = request.draw;
    const bool indices = request.phase == PoolMeshPhase::Indices;
    const reshade::api::resource source = indices ? draw.ib : draw.vb;
    const uint64_t buffer_size = indices ? draw.ib_size : draw.vb_size;
    const uint64_t begin =
        indices ? queued.ib_offset + static_cast<uint64_t>(queued.first_index) * queued.index_size
                : queued.vb_offset + static_cast<uint64_t>(request.min_vertex) * queued.vb_stride;
    const uint64_t end =
        indices ? begin + static_cast<uint64_t>(queued.index_count) * queued.index_size
                : queued.vb_offset + (static_cast<uint64_t>(request.max_vertex) + 1u) * queued.vb_stride;
    const uint64_t aligned_begin = begin & ~uint64_t{3};
    const uint64_t aligned_end = (std::min)((end + 3u) & ~uint64_t{3}, buffer_size);
    const char* error = nullptr;
    if (end <= begin || end > buffer_size) {
      error = indices ? "index range outside the index buffer" : "vertex range outside the vertex buffer";
    } else if (aligned_end - aligned_begin > kPoolMeshSlotBytes) {
      error = indices ? "index range larger than the mesh staging" : "vertex range larger than the mesh staging";
    }
    if (error != nullptr) {
      keys.erase(keys.begin() + static_cast<std::ptrdiff_t>(index));
      FailPoolMesh(request_it, error);
      continue;
    }
    const uint64_t offset = (slot.mesh_used + 15u) & ~uint64_t{15};
    if (offset + (aligned_end - aligned_begin) > kPoolMeshSlotBytes) {
      g_pool.stats.mesh_budget_full += 1u;
      break;
    }
    PoolMeshCopy copy;
    copy.mesh_key = request.mesh_key;
    copy.serial = request.serial;
    copy.phase = request.phase;
    copy.staging_offset = offset;
    copy.skip = begin - aligned_begin;
    copy.size = end - begin;
    copy.copied = aligned_end - aligned_begin;
    copy.source = source.handle;
    copy.source_offset = aligned_begin;
    slot.mesh_used = offset + copy.copied;
    slot.mesh_copies.push_back(copy);
    commands[count] = {source, aligned_begin, slot.mesh_buffer, offset, copy.copied};
    copies[count] = copy;
    ++count;
    request.in_flight = true;
    request.last_frame = frame;
    if (indices) {
      g_pool.stats.mesh_index_copies += 1u;
    } else {
      g_pool.stats.mesh_vertex_copies += 1u;
    }
    g_pool.stats.mesh_copy_bytes += copy.copied;
    keys.erase(keys.begin() + static_cast<std::ptrdiff_t>(index));
  }
  if (keys.empty()) g_pool.mesh_waiting.erase(waiting);
  return count;
}

inline void LogPoolMeshCopies(const PoolMeshCopy* copies, uint32_t count, uint32_t frame) {
  if (count == 0u || !g_pool.log_captures.load(std::memory_order_relaxed)) return;
  namespace log_utils = renodx::utils::log;
  for (uint32_t i = 0; i < count; ++i) {
    const PoolMeshCopy& copy = copies[i];
    log_utils::i("falcom_world::pool: mesh ", log_utils::AsHex(copy.mesh_key), " ", PoolMeshPhaseName(copy.phase),
                 " copy issued: frame ", frame, " | source ", log_utils::AsPtr(copy.source), " offset ",
                 copy.source_offset, " bytes ", copy.copied);
  }
}

// True for a command list other than the immediate one (a deferred context).
// Unknown until the first present has recorded the immediate list.
inline bool IsPoolDeferredList(const reshade::api::command_list* cmd_list) {
  const uint64_t immediate = g_pool.immediate_cmd_list.load(std::memory_order_relaxed);
  return immediate != 0u && reinterpret_cast<uint64_t>(cmd_list) != immediate;
}

inline void OnPoolScanDraw(
    reshade::api::device* device,
    reshade::api::command_list* cmd_list,
    const DrawRecord& draw,
    WorldCommandListData* cl_data) {
  if (!g_pool.scan_active.load(std::memory_order_relaxed)) return;
  if (device == nullptr || cmd_list == nullptr || cl_data == nullptr) return;
  PoolStageScope stage("direct draw: gate");
  const bool deferred = IsPoolDeferredList(cmd_list);

  const PoolDrawGate gate = GatePoolDraw(draw, true);
  PoolSkip skip = gate.skip;
  PoolSlice slice;
  const uint32_t count = draw.instance_count == 0u ? 1u : draw.instance_count;
  stage.Set("direct draw: slice");
  if (skip == PoolSkip::None) skip = ResolvePoolSlice(device, cmd_list, cl_data, &slice);
  if (skip == PoolSkip::None) {
    if (count > kPoolMaxInstancesPerDraw) {
      skip = PoolSkip::TooManyInstances;
    } else if (count > slice.available) {
      skip = PoolSkip::SliceRange;
    }
  }

  const uint32_t frame = g_state.frame.load();
  const uint64_t mesh_key = skip == PoolSkip::None ? PoolMeshKey(draw) : 0u;

  stage.Set("direct draw: reserve");
  std::array<PoolCopyCommand, 2u + kPoolMeshCopiesPerDraw> commands;
  std::array<PoolMeshCopy, kPoolMeshCopiesPerDraw> mesh_copies;
  uint32_t command_count = 0u;
  uint32_t mesh_count = 0u;
  {
    std::lock_guard<std::mutex> lock(g_pool.mutex);
    g_pool.stats.draws_by_vs_class[static_cast<size_t>(gate.vs_class)] += 1u;
    if (deferred) g_pool.stats.draws_on_deferred += 1u;
    PoolFamilyStats& family = g_pool.families[draw.vs_hash];
    family.vs_class = static_cast<uint8_t>(gate.vs_class);
    family.draws += 1u;

    if (skip == PoolSkip::None) {
      const uint64_t slice_bytes = static_cast<uint64_t>(count) * kPoolInstanceStride;
      PoolSchedule* schedule = nullptr;
      skip = ReservePoolCopy(
          device, PoolScheduleKey(mesh_key, draw.first_instance, count), frame, kPoolCbCopyBytes + slice_bytes,
          &schedule);
      if (skip == PoolSkip::None) {
        PoolStagingSlot& slot = g_pool.slots[g_pool.write_slot];
        PoolPendingCopy copy;
        copy.cb_offset = slot.used;
        copy.slice_offset = slot.used + kPoolCbCopyBytes;
        copy.count = count;
        copy.cpu_base = slice.base;
        copy.frame = frame;
        copy.vs_hash = draw.vs_hash;
        copy.mesh_key = mesh_key;
        // The b1 copy is recorded before this draw, so it holds exactly the
        // offset the draw reads; resolve compares it to `base`.
        commands[command_count++] = {slice.instance_cb, 0u, slot.buffer, copy.cb_offset, slice.cb_bytes};
        commands[command_count++] = {slice.instance_buffer, static_cast<uint64_t>(slice.base) * kPoolInstanceStride,
                                     slot.buffer, copy.slice_offset, slice_bytes};
        slot.used += kPoolCbCopyBytes + slice_bytes;
        slot.copies.push_back(copy);
        schedule->copy_frame = frame;
        schedule->next_frame = frame + kPoolRecaptureFrames;
        g_pool.stats.copied_draws += 1u;
        family.copied += 1u;
        QueuePoolMesh(mesh_key, draw.vs_hash, draw);
      }
    }
    if (skip != PoolSkip::None) CountPoolSkip(family, skip, gate.state);
    mesh_count = ReservePoolMeshCopies(device, draw, deferred, frame, commands.data() + command_count,
                                       mesh_copies.data(), kPoolMeshCopiesPerDraw);
    command_count += mesh_count;
  }
  stage.Set("direct draw: copy");
  IssuePoolCopies(cmd_list, commands.data(), command_count);
  LogPoolMeshCopies(mesh_copies.data(), mesh_count, frame);
}

// DrawIndexedInstancedIndirect and friends. `draw` carries the bindings at
// the draw; its counts come from the args at resolve.
inline void OnPoolScanIndirectDraw(
    reshade::api::device* device,
    reshade::api::command_list* cmd_list,
    const DrawRecord& draw,
    WorldCommandListData* cl_data,
    reshade::api::resource args_buffer,
    uint64_t args_offset,
    uint32_t draw_count,
    uint32_t stride) {
  if (!g_pool.scan_active.load(std::memory_order_relaxed)) return;
  if (!g_pool.scan_indirect.load(std::memory_order_relaxed)) return;
  if (device == nullptr || cmd_list == nullptr || cl_data == nullptr || args_buffer.handle == 0u) return;
  PoolStageScope stage("indirect draw: gate");
  const bool deferred = IsPoolDeferredList(cmd_list);

  const PoolDrawGate gate = GatePoolDraw(draw, false);
  PoolSkip skip = gate.skip;
  PoolSlice slice;
  stage.Set("indirect draw: slice");
  if (skip == PoolSkip::None) skip = ResolvePoolSlice(device, cmd_list, cl_data, &slice);
  const uint32_t max_window =
      static_cast<uint32_t>((std::min)(static_cast<uint64_t>(kPoolIndirectWindow), slice.available));
  if (skip == PoolSkip::None && max_window == 0u) skip = PoolSkip::SliceRange;
  uint64_t args_size = 0u;
  stage.Set("indirect draw: args size");
  if (skip == PoolSkip::None) {
    const auto args_desc = device->get_resource_desc(args_buffer);
    args_size = args_desc.type == reshade::api::resource_type::buffer ? args_desc.buffer.size : 0u;
  }

  const uint32_t frame = g_state.frame.load();
  const uint32_t sub_draws = (std::min)(draw_count == 0u ? 1u : draw_count, kPoolMaxIndirectSubDraws);

  stage.Set("indirect draw: reserve");
  std::array<PoolCopyCommand, 3u * kPoolMaxIndirectSubDraws + kPoolMeshCopiesPerDraw> commands;
  std::array<PoolMeshCopy, kPoolMeshCopiesPerDraw> mesh_copies;
  uint32_t command_count = 0u;
  uint32_t mesh_count = 0u;
  {
    std::lock_guard<std::mutex> lock(g_pool.mutex);
    PoolFamilyStats& family = g_pool.families[draw.vs_hash];
    family.vs_class = static_cast<uint8_t>(gate.vs_class);
    for (uint32_t i = 0; i < sub_draws; ++i) {
      g_pool.stats.draws_by_vs_class[static_cast<size_t>(gate.vs_class)] += 1u;
      g_pool.stats.indirect_by_vs_class[static_cast<size_t>(gate.vs_class)] += 1u;
      if (deferred) {
        g_pool.stats.draws_on_deferred += 1u;
        g_pool.stats.indirect_on_deferred += 1u;
      }
      family.draws += 1u;
      family.indirect_draws += 1u;

      PoolSkip sub_skip = skip;
      const uint64_t sub_args_offset = args_offset + static_cast<uint64_t>(i) * stride;
      if (sub_skip == PoolSkip::None && sub_args_offset + kPoolIndirectArgsCopyBytes > args_size) {
        sub_skip = PoolSkip::SliceRange;
      }
      // The window this identity needs: decided by its last args (see
      // NextPoolIndirectWindow), the full window until they were read once.
      const uint64_t schedule_key = PoolIndirectScheduleKey(draw, args_buffer, sub_args_offset);
      uint32_t window = max_window;
      const auto known = g_pool.schedule.find(schedule_key);
      if (known != g_pool.schedule.end() && known->second.indirect_window != 0u) {
        window = (std::min)(known->second.indirect_window, max_window);
      }
      const uint64_t bytes =
          kPoolCbCopyBytes + kPoolIndirectArgsBytes + static_cast<uint64_t>(window) * kPoolInstanceStride;
      PoolSchedule* schedule = nullptr;
      if (sub_skip == PoolSkip::None) {
        sub_skip = ReservePoolCopy(device, schedule_key, frame, bytes, &schedule);
      }
      if (sub_skip != PoolSkip::None) {
        CountPoolSkip(family, sub_skip, gate.state);
        continue;
      }

      PoolStagingSlot& slot = g_pool.slots[g_pool.write_slot];
      PoolPendingCopy copy;
      copy.cb_offset = slot.used;
      copy.args_offset = slot.used + kPoolCbCopyBytes;
      copy.slice_offset = slot.used + kPoolCbCopyBytes + kPoolIndirectArgsBytes;
      copy.count = window;
      copy.cpu_base = slice.base;
      copy.frame = frame;
      copy.vs_hash = draw.vs_hash;
      copy.indirect_index = static_cast<uint32_t>(slot.indirect_draws.size());
      copy.schedule_key = schedule_key;
      PoolCopyCommand* sub_commands = commands.data() + command_count;
      sub_commands[0] = {slice.instance_cb, 0u, slot.buffer, copy.cb_offset, slice.cb_bytes};
      sub_commands[1] = {args_buffer, sub_args_offset, slot.buffer, copy.args_offset, kPoolIndirectArgsCopyBytes};
      sub_commands[2] = {slice.instance_buffer, static_cast<uint64_t>(slice.base) * kPoolInstanceStride, slot.buffer,
                         copy.slice_offset, static_cast<uint64_t>(window) * kPoolInstanceStride};
      command_count += 3u;
      slot.used += bytes;
      slot.copies.push_back(copy);
      slot.indirect_draws.push_back(draw);
      AddPoolIndirectRefs(draw);
      schedule->copy_frame = frame;
      schedule->next_frame = frame + kPoolRecaptureFrames;
      g_pool.stats.copied_draws += 1u;
      g_pool.stats.indirect_copied += 1u;
      g_pool.stats.indirect_window_instances += window;
      family.copied += 1u;
    }
    mesh_count = ReservePoolMeshCopies(device, draw, deferred, frame, commands.data() + command_count,
                                       mesh_copies.data(), kPoolMeshCopiesPerDraw);
    command_count += mesh_count;
  }
  stage.Set("indirect draw: copy");
  IssuePoolCopies(cmd_list, commands.data(), command_count);
  LogPoolMeshCopies(mesh_copies.data(), mesh_count, frame);
}

// ---------------------------------------------------------------------------
// Resource invalidation and retirement.

// Caller holds g_pool.mutex. Unmaps one draw key from its mesh; the mesh is
// retired (with its instances) once no draw key refers to it any more.
inline void InvalidatePoolMeshKey(uint64_t mesh_key) {
  g_pool.mesh_queued.erase(mesh_key);
  g_pool.failed_meshes.erase(mesh_key);
  g_pool.invalidated_keys.insert(mesh_key);
  // A queued mesh is forgotten; a copy of it still in flight is dropped when
  // read (no request with its serial is left).
  const auto request = g_pool.mesh_requests.find(mesh_key);
  if (request != g_pool.mesh_requests.end()) {
    if (!request->second.in_flight) RemovePoolMeshWaiting(request->second.buffer_key, mesh_key);
    g_pool.mesh_requests.erase(request);
  }
  const auto it = g_pool.mesh_by_key.find(mesh_key);
  if (it == g_pool.mesh_by_key.end()) return;
  WorldMesh& mesh = g_pool.meshes[it->second];
  if (mesh.live_keys != 0u) mesh.live_keys -= 1u;
  if (mesh.live_keys == 0u) g_pool.retire_pending = true;
  g_pool.mesh_by_key.erase(it);
}

inline void OnDestroyResourcePool(reshade::api::device* device, reshade::api::resource resource) {
  (void)device;
  const PoolStageScope stage("buffer release");
  std::lock_guard<std::mutex> lock(g_pool.mutex);
  if (!g_pool.indirect_refs.empty() && g_pool.indirect_refs.count(resource.handle) != 0u) {
    g_pool.indirect_dead.insert(resource.handle);
  }
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

inline uint64_t PoolMeshSignature(const PoolDecodedMesh& mesh) {
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

// Caller holds g_pool.mutex. Adds one decoded mesh, or maps the key to an
// existing mesh with the same content, and admits the instances waiting for
// it. Returns what happened, for the log.
inline const char* ApplyPoolMesh(uint64_t mesh_key, uint32_t vs_hash, PoolDecodedMesh& mesh) {
  if (g_pool.mesh_by_key.count(mesh_key) != 0u) return "already captured";

  const char* outcome = "added";
  uint32_t mesh_id = 0u;
  const uint64_t signature = PoolMeshSignature(mesh);
  const auto signature_it = g_pool.mesh_by_signature.find(signature);
  if (signature_it != g_pool.mesh_by_signature.end()) {
    mesh_id = signature_it->second;
    g_pool.stats.mesh_dedup += 1u;
    outcome = "merged (same content)";
  } else {
    if (g_pool.meshes.size() >= kPoolMaxMeshes) {
      g_pool.stats.mesh_cap_drops += 1u;
      g_pool.failed_meshes.insert(mesh_key);
      return "dropped (mesh cap)";
    }
    WorldMesh world_mesh;
    world_mesh.mesh_key = mesh_key;
    world_mesh.mesh_id = static_cast<uint32_t>(g_pool.meshes.size());
    world_mesh.source_vs_hash = vs_hash;
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
  g_pool.mesh_by_key[mesh_key] = mesh_id;
  AdmitPendingInstancesForMesh(mesh_key, mesh_id);
  return outcome;
}

// Index read: the draw's indices with the base vertex applied, and the
// vertex range they cover. Returns nullptr on success, else why the mesh
// cannot be captured.
inline const char* DecodePoolMeshIndices(
    const uint8_t* bytes,
    uint64_t size,
    uint32_t index_size,
    uint32_t index_count,
    int32_t vertex_offset,
    uint32_t stride,
    std::vector<uint32_t>* indices,
    uint32_t* min_vertex,
    uint32_t* max_vertex) {
  if (index_size != 2u && index_size != 4u) return "unsupported index size";
  if (size < static_cast<uint64_t>(index_count) * index_size) return "index copy shorter than the draw";
  indices->clear();
  indices->reserve(index_count);
  int64_t lowest = INT64_MAX;
  int64_t highest = -1;
  for (uint32_t i = 0; i < index_count; ++i) {
    uint32_t raw = 0u;
    if (index_size == 2u) {
      uint16_t value = 0u;
      std::memcpy(&value, bytes + static_cast<size_t>(i) * 2u, sizeof(value));
      raw = value;
    } else {
      std::memcpy(&raw, bytes + static_cast<size_t>(i) * 4u, sizeof(raw));
    }
    const int64_t absolute = static_cast<int64_t>(raw) + vertex_offset;
    if (absolute < 0 || absolute > static_cast<int64_t>(UINT32_MAX - 1u)) return "index outside the vertex buffer";
    indices->push_back(static_cast<uint32_t>(absolute));
    lowest = (std::min)(lowest, absolute);
    highest = (std::max)(highest, absolute);
  }
  if (highest < 0) return "no indices";
  if (static_cast<uint64_t>(highest - lowest + 1) * stride > kPoolMeshSlotBytes) {
    return "vertex range larger than the mesh staging";
  }
  *min_vertex = static_cast<uint32_t>(lowest);
  *max_vertex = static_cast<uint32_t>(highest);
  return nullptr;
}

// Vertex read: `bytes` starts at vertex min_vertex. Positions of the vertices
// the indices use, remapped in first-use order, and the triangles.
inline const char* DecodePoolMeshVertices(
    const uint8_t* bytes,
    uint64_t size,
    uint32_t stride,
    int32_t pos_offset,
    reshade::api::format pos_format,
    const std::vector<uint32_t>& indices,
    uint32_t min_vertex,
    uint32_t max_vertex,
    PoolDecodedMesh* mesh) {
  namespace scene = renodx::utils::scene;
  const scene::FormatInfo* info = scene::FindFormatInfo(pos_format);
  if (info == nullptr) return "unsupported position format";
  if (max_vertex < min_vertex || pos_offset < 0) return "bad vertex range";
  const uint64_t vertex_count = static_cast<uint64_t>(max_vertex) - min_vertex + 1u;
  if (size < (vertex_count - 1u) * stride + static_cast<uint64_t>(pos_offset) + info->byte_size) {
    return "vertex copy shorter than the range";
  }
  std::vector<uint32_t> remap(static_cast<size_t>(vertex_count), UINT32_MAX);
  mesh->positions.clear();
  mesh->triangles.clear();
  mesh->triangles.reserve(indices.size() / 3u);
  for (size_t i = 0; i + 2u < indices.size(); i += 3u) {
    std::array<uint32_t, 3> triangle = {0u, 0u, 0u};
    for (size_t k = 0; k < 3u; ++k) {
      const uint32_t absolute = indices[i + k];
      if (absolute < min_vertex || absolute > max_vertex) return "index outside the vertex range";
      uint32_t& fresh = remap[absolute - min_vertex];
      if (fresh == UINT32_MAX) {
        float position[4] = {};
        const uint8_t* vertex = bytes + static_cast<uint64_t>(absolute - min_vertex) * stride + pos_offset;
        if (!scene::DecodeAttribute(vertex, *info, position) || !std::isfinite(position[0])
            || !std::isfinite(position[1]) || !std::isfinite(position[2])) {
          return "vertex position not decodable";
        }
        fresh = static_cast<uint32_t>(mesh->positions.size());
        mesh->positions.push_back({position[0], position[1], position[2]});
      }
      triangle[k] = fresh;
    }
    mesh->triangles.push_back(triangle);
  }
  if (mesh->positions.empty()) return "no vertices decoded";
  mesh->bbox_min = mesh->positions[0];
  mesh->bbox_max = mesh->positions[0];
  for (const auto& position : mesh->positions) {
    for (int k = 0; k < 3; ++k) {
      mesh->bbox_min[k] = (std::min)(mesh->bbox_min[k], position[k]);
      mesh->bbox_max[k] = (std::max)(mesh->bbox_max[k], position[k]);
    }
  }
  if (mesh->bbox_min == mesh->bbox_max) return "decoded bounding box is degenerate";
  return nullptr;
}

// Writes the switch states to ReShade.log while "Log mesh captures" is on, so
// a log shows which pool stages were running before a crash.
inline void LogPoolSwitches() {
  if (!g_pool.log_captures.load(std::memory_order_relaxed)) return;
  const auto on = [](const std::atomic_bool& value) { return value.load(std::memory_order_relaxed) ? "on" : "off"; };
  renodx::utils::log::i("falcom_world::pool: frame ", g_state.frame.load(), " switches: pool scan ", on(g_pool.scan_active),
                        ", capture meshes ", on(g_pool.capture_meshes), ", scan indirect draws ", on(g_pool.scan_indirect));
}

struct PoolResolvedCopy {
  bool verified = false;
  int32_t gpu_base = 0;
  size_t first_world = 0u;  // index into the parsed world list
  uint32_t count = 0u;      // elements parsed
  uint32_t args[5] = {};    // indirect: index_count, instance_count, first_index, vertex_offset, first_instance
};

// One mesh copy being read: what decoding needs from its request (taken when
// the slot is read, so decoding runs without the lock) and the result.
struct PoolMeshJob {
  PoolMeshCopy copy;
  bool queued = false;  // a request with this serial existed when the slot was read
  uint32_t index_size = 0u;
  uint32_t index_count = 0u;
  int32_t vertex_offset = 0;
  uint32_t stride = 0u;
  int32_t pos_offset = 0;
  reshade::api::format pos_format = reshade::api::format::unknown;
  std::vector<uint32_t> indices;  // index read: output; vertex read: moved from the request
  uint32_t min_vertex = 0u;
  uint32_t max_vertex = 0u;
  const char* error = nullptr;
  PoolDecodedMesh mesh;
};

// Caller holds g_pool.mutex.
inline void CountPoolMatrixReject(PoolFamilyStats& family, PoolMatrixReject reject, const float* world) {
  g_pool.stats.rejected_matrix += 1u;
  g_pool.stats.matrix_rejects[static_cast<size_t>(reject)] += 1u;
  family.rejected_matrix += 1u;
  family.matrix_rejects[static_cast<size_t>(reject)] += 1u;
  if (family.reject_samples.size() < kPoolMaxRejectSamples) {
    PoolMatrixSample sample;
    sample.reason = static_cast<uint8_t>(reject);
    std::memcpy(sample.matrix, world, sizeof(float) * kPoolWorldFloats);
    family.reject_samples.push_back(sample);
  }
}

// Window for the next copy of an indirect draw: what it drew last time plus
// a margin, a small window when it drew nothing, at least double the last
// window when that one was too small; rounded up, at most kPoolIndirectWindow.
inline uint32_t NextPoolIndirectWindow(uint32_t instance_count, uint32_t window) {
  uint32_t next = kPoolIndirectEmptyWindow;
  if (instance_count != 0u) {
    next = instance_count + instance_count / 4u + kPoolIndirectWindowStep;
    if (instance_count > window) next = (std::max)(next, window * 2u);
  }
  next = (std::min)(next, kPoolIndirectWindow);
  return (next + kPoolIndirectWindowStep - 1u) / kPoolIndirectWindowStep * kPoolIndirectWindowStep;
}

// Reads one staging slot that the GPU finished two presents ago and applies
// its copies. Mapping and mesh decoding happen without holding g_pool.mutex
// so recording threads are not blocked; the slot is flagged `resolving`
// meanwhile.
inline void ResolvePoolSlot(reshade::api::device* device, uint32_t slot_index) {
  PoolStageScope stage("present: resolve (take)");
  std::vector<PoolPendingCopy> copies;
  std::vector<DrawRecord> indirect_draws;
  std::vector<PoolMeshJob> jobs;
  reshade::api::resource buffer = {0u};
  reshade::api::resource mesh_buffer = {0u};
  uint64_t used = 0u;
  uint64_t mesh_used = 0u;
  {
    std::lock_guard<std::mutex> lock(g_pool.mutex);
    PoolStagingSlot& slot = g_pool.slots[slot_index];
    if (slot.copies.empty() && slot.mesh_copies.empty()) {
      slot.used = 0u;
      slot.mesh_used = 0u;
      slot.indirect_draws.clear();
      return;
    }
    copies.swap(slot.copies);
    indirect_draws.swap(slot.indirect_draws);
    jobs.reserve(slot.mesh_copies.size());
    for (const PoolMeshCopy& copy : slot.mesh_copies) {
      PoolMeshJob job;
      job.copy = copy;
      const auto request = g_pool.mesh_requests.find(copy.mesh_key);
      if (request != g_pool.mesh_requests.end() && request->second.serial == copy.serial) {
        PoolMeshRequest& queued = request->second;
        job.queued = true;
        job.index_size = queued.draw.index_size;
        job.index_count = queued.draw.index_count;
        job.vertex_offset = queued.draw.vertex_offset;
        job.stride = queued.draw.vb_stride;
        job.pos_offset = queued.pos_offset;
        job.pos_format = queued.pos_format;
        job.min_vertex = queued.min_vertex;
        job.max_vertex = queued.max_vertex;
        if (copy.phase == PoolMeshPhase::Vertices) job.indices = std::move(queued.indices);
      }
      jobs.push_back(std::move(job));
    }
    slot.mesh_copies.clear();
    buffer = slot.buffer;
    used = slot.used;
    mesh_buffer = slot.mesh_buffer;
    mesh_used = slot.mesh_used;
    slot.resolving = true;
  }
  const auto is_indirect = [&indirect_draws](const PoolPendingCopy& copy) {
    return copy.indirect_index != UINT32_MAX && copy.indirect_index < indirect_draws.size();
  };

  stage.Set("present: resolve (map)");
  std::vector<PoolResolvedCopy> resolved(copies.size());
  std::vector<float> worlds;  // kPoolWorldFloats per instance of verified copies
  std::vector<uint8_t> moving;
  void* mapped = nullptr;
  const bool mapped_ok = copies.empty()
                         || (buffer.handle != 0u && used != 0u
                             && device->map_buffer_region(buffer, 0u, used, reshade::api::map_access::read_only, &mapped)
                             && mapped != nullptr);
  if (mapped_ok && !copies.empty()) {
    const auto* bytes = static_cast<const uint8_t*>(mapped);
    for (size_t i = 0; i < copies.size(); ++i) {
      const PoolPendingCopy& copy = copies[i];
      PoolResolvedCopy& result = resolved[i];
      std::memcpy(&result.gpu_base, bytes + copy.cb_offset, sizeof(int32_t));
      result.verified = result.gpu_base == copy.cpu_base;
      if (!result.verified) continue;
      result.count = copy.count;
      if (is_indirect(copy)) {
        std::memcpy(result.args, bytes + copy.args_offset, sizeof(result.args));
        result.count = (std::min)(result.args[1], copy.count);
      }
      result.first_world = worlds.size() / kPoolWorldFloats;
      for (uint32_t element = 0; element < result.count; ++element) {
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

  // Mesh copies are decoded straight from the mapped staging.
  stage.Set("present: resolve (decode meshes)");
  void* mesh_mapped = nullptr;
  const bool meshes_ok = jobs.empty()
                         || (mesh_buffer.handle != 0u && mesh_used != 0u
                             && device->map_buffer_region(mesh_buffer, 0u, mesh_used, reshade::api::map_access::read_only,
                                                          &mesh_mapped)
                             && mesh_mapped != nullptr);
  if (meshes_ok && !jobs.empty()) {
    const auto* bytes = static_cast<const uint8_t*>(mesh_mapped);
    for (PoolMeshJob& job : jobs) {
      if (!job.queued) continue;
      const uint8_t* data = bytes + job.copy.staging_offset + job.copy.skip;
      if (job.copy.phase == PoolMeshPhase::Indices) {
        job.error = DecodePoolMeshIndices(data, job.copy.size, job.index_size, job.index_count, job.vertex_offset,
                                          job.stride, &job.indices, &job.min_vertex, &job.max_vertex);
      } else {
        job.error = DecodePoolMeshVertices(data, job.copy.size, job.stride, job.pos_offset, job.pos_format, job.indices,
                                           job.min_vertex, job.max_vertex, &job.mesh);
      }
    }
    device->unmap_buffer_region(mesh_buffer);
  }

  stage.Set("present: resolve (apply)");
  const bool log = g_pool.log_captures.load(std::memory_order_relaxed);
  const uint32_t frame = g_state.frame.load();
  std::vector<std::string> log_lines;
  {
    std::lock_guard<std::mutex> lock(g_pool.mutex);
    PoolStagingSlot& slot = g_pool.slots[slot_index];
    slot.resolving = false;
    slot.used = 0u;
    slot.mesh_used = 0u;

    // Meshes first, so instances read in this slot can admit right away.
    if (!meshes_ok) g_pool.stats.map_failures += 1u;
    for (PoolMeshJob& job : jobs) {
      const auto request = g_pool.mesh_requests.find(job.copy.mesh_key);
      const bool current = job.queued && request != g_pool.mesh_requests.end()
                           && request->second.serial == job.copy.serial;
      const char* outcome = nullptr;
      if (!current) {
        g_pool.stats.mesh_dropped += 1u;
        outcome = "dropped (mesh released or re-queued)";
      } else if (!meshes_ok) {
        // Staging not readable: back in line for another copy.
        PoolMeshRequest& queued = request->second;
        if (job.copy.phase == PoolMeshPhase::Vertices) queued.indices = std::move(job.indices);
        queued.in_flight = false;
        AddPoolMeshWaiting(queued);
        outcome = "staging not readable, copy again";
      } else if (job.error != nullptr) {
        FailPoolMesh(request, job.error);
        outcome = job.error;
      } else if (job.copy.phase == PoolMeshPhase::Indices) {
        PoolMeshRequest& queued = request->second;
        queued.indices = std::move(job.indices);
        queued.min_vertex = job.min_vertex;
        queued.max_vertex = job.max_vertex;
        queued.phase = PoolMeshPhase::Vertices;
        queued.in_flight = false;
        queued.last_frame = frame;
        AddPoolMeshWaiting(queued);
        outcome = "read, vertex copy next";
      } else {
        const PoolMeshRequest& queued = request->second;
        outcome = ApplyPoolMesh(queued.mesh_key, queued.vs_hash, job.mesh);
        g_pool.mesh_requests.erase(request);
      }
      if (log) {
        namespace log_utils = renodx::utils::log;
        std::string line = log_utils::BuildString(
            "falcom_world::pool: mesh ", log_utils::AsHex(job.copy.mesh_key), " ", PoolMeshPhaseName(job.copy.phase),
            " read: frame ", frame, " | ", outcome);
        if (current && job.error == nullptr && meshes_ok) {
          if (job.copy.phase == PoolMeshPhase::Indices) {
            line += log_utils::BuildString(" | ", job.index_count, " indices, vertices ", job.min_vertex, "..",
                                           job.max_vertex);
          } else {
            line += log_utils::BuildString(" | ", job.mesh.triangles.size(), " triangles");
          }
        }
        log_lines.push_back(std::move(line));
      }
    }

    if (!mapped_ok) {
      g_pool.stats.map_failures += 1u;
      for (const DrawRecord& draw : indirect_draws) ReleasePoolIndirectRefs(draw);
      copies.clear();
    }
    for (size_t i = 0; i < copies.size(); ++i) {
      const PoolPendingCopy& copy = copies[i];
      const PoolResolvedCopy& result = resolved[i];
      PoolFamilyStats& family = g_pool.families[copy.vs_hash];
      const bool indirect = is_indirect(copy);
      const bool buffers_dead = indirect && ReleasePoolIndirectRefs(indirect_draws[copy.indirect_index]);
      if (!result.verified) {
        g_pool.stats.base_mismatch += 1u;
        g_pool.stats.last_mismatch_cpu = copy.cpu_base;
        g_pool.stats.last_mismatch_gpu = result.gpu_base;
        family.base_mismatch += 1u;
        continue;
      }
      g_pool.stats.base_verified += 1u;

      uint64_t mesh_key = copy.mesh_key;
      if (indirect) {
        g_pool.stats.indirect_resolved += 1u;
        const uint32_t index_count = result.args[0];
        const uint32_t instance_count = result.args[1];
        // The args decide the next window of this draw identity.
        const auto schedule = g_pool.schedule.find(copy.schedule_key);
        if (schedule != g_pool.schedule.end()) {
          schedule->second.indirect_window = NextPoolIndirectWindow(index_count == 0u ? 0u : instance_count, copy.count);
        }
        if (index_count == 0u || instance_count == 0u) {
          g_pool.stats.indirect_empty += 1u;
          continue;
        }
        if (index_count < 3u || (index_count % 3u) != 0u) {
          g_pool.stats.draw_state[static_cast<size_t>(PoolDrawState::IndexCount)] += 1u;
          family.draw_state[static_cast<size_t>(PoolDrawState::IndexCount)] += 1u;
          continue;
        }
        if (instance_count > copy.count) g_pool.stats.indirect_truncated += 1u;
        if (buffers_dead) {
          g_pool.stats.indirect_dead += 1u;
          continue;
        }
        DrawRecord record = indirect_draws[copy.indirect_index];
        record.index_count = index_count;
        record.instance_count = instance_count;
        record.first_index = result.args[2];
        record.vertex_offset = static_cast<int32_t>(result.args[3]);
        record.first_instance = result.args[4];
        mesh_key = PoolMeshKey(record);
        QueuePoolMesh(mesh_key, copy.vs_hash, record);
      }

      for (uint32_t element = 0; element < result.count; ++element) {
        const size_t index = result.first_world + element;
        const float* world = worlds.data() + index * kPoolWorldFloats;
        g_pool.stats.instances_seen += 1u;
        family.instances_seen += 1u;
        if (moving[index] != 0u) g_pool.stats.moving_instances += 1u;
        const PoolMatrixReject reject = CheckPoolWorld(world);
        if (reject != PoolMatrixReject::None) {
          CountPoolMatrixReject(family, reject, world);
          continue;
        }
        ObservePoolInstance(mesh_key, copy.vs_hash, world, copy.frame);
      }
    }
  }
  for (const std::string& line : log_lines) renodx::utils::log::i(line);
}

// Caller holds g_pool.mutex.
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
  // Meshes not drawn for a while (QueuePoolMesh refreshes last_frame at each
  // sighting): forget them and their indices; the next sighting queues them
  // again.
  for (auto it = g_pool.mesh_requests.begin(); it != g_pool.mesh_requests.end();) {
    if (frame > it->second.last_frame + kPoolPruneAge) {
      if (!it->second.in_flight) RemovePoolMeshWaiting(it->second.buffer_key, it->first);
      g_pool.mesh_queued.erase(it->first);
      g_pool.stats.mesh_expired += 1u;
      it = g_pool.mesh_requests.erase(it);
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
  g_pool.immediate_cmd_list.store(reinterpret_cast<uint64_t>(queue->get_immediate_command_list()),
                                  std::memory_order_relaxed);

  PoolStageScope stage("present: staging");
  if (scanning) EnsurePoolStaging(device);

  uint32_t resolve_slot = UINT32_MAX;
  stage.Set("present: compact");
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
  stage.Set("present: resolve");
  if (resolve_slot != UINT32_MAX) ResolvePoolSlot(device, resolve_slot);

  stage.Set("present: prune");
  uint32_t released = 0u;
  size_t mesh_waiting = 0u;
  {
    std::lock_guard<std::mutex> lock(g_pool.mutex);
    if ((frame % 60u) == 0u) PrunePool(frame);
    UpdatePoolStats();
    const uint32_t invalidations = g_pool.stats.resource_invalidations;
    released = invalidations >= g_pool.logged_invalidations ? invalidations - g_pool.logged_invalidations : invalidations;
    g_pool.logged_invalidations = invalidations;
    mesh_waiting = g_pool.stats.mesh_queue;
  }
  if (released != 0u && g_pool.log_captures.load(std::memory_order_relaxed)) {
    renodx::utils::log::i("falcom_world::pool: frame ", frame, ": ", released,
                          " tracked vertex/index buffers released, ", mesh_waiting, " meshes waiting");
  }
}

inline void ResetWorldPool() {
  std::lock_guard<std::mutex> lock(g_pool.mutex);
  for (auto& slot : g_pool.slots) {
    // Copies already recorded into a slot still land there; only forget them.
    if (!slot.resolving) {
      for (const DrawRecord& draw : slot.indirect_draws) ReleasePoolIndirectRefs(draw);
      slot.copies.clear();
      slot.indirect_draws.clear();
      slot.used = 0u;
      slot.mesh_copies.clear();
      slot.mesh_used = 0u;
    }
  }
  g_pool.mesh_requests.clear();
  g_pool.mesh_waiting.clear();
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
  g_pool.near_index.clear();
  g_pool.schedule.clear();
  g_pool.families.clear();
  g_pool.revision += 1u;
  g_pool.stats = {};
  g_pool.logged_invalidations = 0u;
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
  out << "  \"schema\": 3,\n";
  out << "  \"generated_frame\": " << g_state.frame.load() << ",\n";
  out << "  \"camera_valid\": " << (camera.valid ? "true" : "false") << ",\n";
  out << "  \"camera_position\": [" << camera.position[0] << ", " << camera.position[1] << ", " << camera.position[2] << "],\n";
  out << "  \"switches\": {\"capture_meshes\": " << (g_pool.capture_meshes.load() ? "true" : "false")
      << ", \"scan_indirect\": " << (g_pool.scan_indirect.load() ? "true" : "false") << "},\n";
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
  out << "}, \"draw_state\": {";
  for (size_t i = 1; i < stats.draw_state.size(); ++i) {
    if (i != 1u) out << ", ";
    out << "\"" << PoolDrawStateName(static_cast<PoolDrawState>(i)) << "\": " << stats.draw_state[i];
  }
  out << "}, \"indirect_by_vs_class\": {";
  for (size_t i = 0; i < stats.indirect_by_vs_class.size(); ++i) {
    if (i != 0u) out << ", ";
    out << "\"" << contract::VsClassName(static_cast<contract::VsClass>(i)) << "\": " << stats.indirect_by_vs_class[i];
  }
  out << "}, \"matrix_rejects\": {";
  for (size_t i = 1; i < stats.matrix_rejects.size(); ++i) {
    if (i != 1u) out << ", ";
    out << "\"" << PoolMatrixRejectName(static_cast<PoolMatrixReject>(i)) << "\": " << stats.matrix_rejects[i];
  }
  out << "}"
      << ", \"copied_draws\": " << stats.copied_draws
      << ", \"draws_on_deferred\": " << stats.draws_on_deferred
      << ", \"indirect_on_deferred\": " << stats.indirect_on_deferred
      << ", \"indirect_copied\": " << stats.indirect_copied
      << ", \"indirect_resolved\": " << stats.indirect_resolved
      << ", \"indirect_empty\": " << stats.indirect_empty
      << ", \"indirect_truncated\": " << stats.indirect_truncated
      << ", \"indirect_dead\": " << stats.indirect_dead
      << ", \"indirect_window_instances\": " << stats.indirect_window_instances
      << ", \"near_misses\": " << stats.near_misses
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
      << ", \"mesh_in_flight\": " << stats.mesh_in_flight
      << ", \"mesh_index_copies\": " << stats.mesh_index_copies
      << ", \"mesh_vertex_copies\": " << stats.mesh_vertex_copies
      << ", \"mesh_copy_bytes\": " << stats.mesh_copy_bytes
      << ", \"mesh_budget_full\": " << stats.mesh_budget_full
      << ", \"mesh_dropped\": " << stats.mesh_dropped
      << ", \"mesh_expired\": " << stats.mesh_expired
      << ", \"mesh_deferred_skips\": " << stats.mesh_deferred_skips
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
        << ", \"indirect_draws\": " << family.indirect_draws
        << ", \"copied\": " << family.copied
        << ", \"instances_seen\": " << family.instances_seen
        << ", \"admitted\": " << family.admitted
        << ", \"region\": " << entry.in_region
        << ", \"meshes\": " << entry.meshes
        << ", \"base_mismatch\": " << family.base_mismatch
        << ", \"rejected_matrix\": " << family.rejected_matrix
        << ", \"near_misses\": " << family.near_misses
        << ", \"near_miss_max_delta\": " << family.near_miss_max_delta
        << ", \"skips\": {";
    bool first_skip = true;
    for (size_t i = 1; i < family.skips.size(); ++i) {
      if (family.skips[i] == 0u) continue;
      if (!first_skip) out << ", ";
      first_skip = false;
      out << "\"" << PoolSkipName(static_cast<PoolSkip>(i)) << "\": " << family.skips[i];
    }
    out << "}, \"draw_state\": {";
    first_skip = true;
    for (size_t i = 1; i < family.draw_state.size(); ++i) {
      if (family.draw_state[i] == 0u) continue;
      if (!first_skip) out << ", ";
      first_skip = false;
      out << "\"" << PoolDrawStateName(static_cast<PoolDrawState>(i)) << "\": " << family.draw_state[i];
    }
    out << "}, \"matrix_rejects\": {";
    first_skip = true;
    for (size_t i = 1; i < family.matrix_rejects.size(); ++i) {
      if (family.matrix_rejects[i] == 0u) continue;
      if (!first_skip) out << ", ";
      first_skip = false;
      out << "\"" << PoolMatrixRejectName(static_cast<PoolMatrixReject>(i)) << "\": " << family.matrix_rejects[i];
    }
    out << "}";
    const auto write_samples = [&out](const char* name, const std::vector<PoolMatrixSample>& samples, bool with_reason) {
      if (samples.empty()) return;
      out << ", \"" << name << "\": [";
      for (size_t s = 0; s < samples.size(); ++s) {
        if (s != 0u) out << ", ";
        out << "{";
        if (with_reason) {
          out << "\"reason\": \"" << PoolMatrixRejectName(static_cast<PoolMatrixReject>(samples[s].reason)) << "\", ";
        }
        out << "\"world\": [";
        for (uint32_t k = 0; k < kPoolWorldFloats; ++k) {
          if (k != 0u) out << ", ";
          out << samples[s].matrix[k];
        }
        out << "]}";
      }
      out << "]";
    };
    write_samples("reject_samples", family.reject_samples, true);
    write_samples("near_miss_samples", family.near_miss_samples, false);
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

  // One OBJ object per vertex-shader family ("vs_0x........", the shader that
  // drew the instance when it was admitted), so a family can be shown or
  // hidden on its own in Blender.
  std::vector<const WorldInstance*> ordered;
  ordered.reserve(instances.size());
  for (const auto& instance : instances) {
    if (!PoolInstanceInRegion(instance, region)) continue;
    if (instance.mesh_id >= meshes.size()) continue;
    ordered.push_back(&instance);
  }
  std::stable_sort(ordered.begin(), ordered.end(), [](const WorldInstance* a, const WorldInstance* b) {
    return a->source_vs_hash < b->source_vs_hash;
  });

  std::ostringstream out;
  out << "# falcomengine-plus world pool (region instances, world space, one object per vertex shader)\n";
  size_t vertex_base = 1u;
  size_t written = 0u;
  bool have_family = false;
  uint32_t family = 0u;
  for (const WorldInstance* entry : ordered) {
    const WorldInstance& instance = *entry;
    const WorldMesh& mesh = meshes[instance.mesh_id];
    if (written + mesh.positions.size() > max_vertices) break;
    if (!have_family || instance.source_vs_hash != family) {
      have_family = true;
      family = instance.source_vs_hash;
      out << "o vs_" << PoolHashText(family) << "\n";
    }
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
