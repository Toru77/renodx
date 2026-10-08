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
// stale bytes. While "Verify mesh captures" is on (default) a mesh is
// captured again from scratch after its first capture and enters the pool
// only when two captures in a row decode to the same mesh; after
// kPoolMeshMaxCaptures without that it is rejected as unstable (its buffer
// contents change between frames). Meshes are merged
// by content and retired with their instances when their vertex or index
// buffer is destroyed, so a map change empties the pool instead of leaving
// the previous map's geometry behind.
//
// Diagnostic switches (BVH panel): "Capture meshes" stops new mesh copies
// (instances are still observed; new meshes wait), "Scan indirect draws"
// ignores indirect draws, "Legacy instance scale limits" applies the axis
// limits from before the draw-time capture rework (kPoolLegacyMinScale ..
// kPoolLegacyMaxScale), and "Log mesh captures" writes ReShade.log lines
// for each mesh copy and read and when tracked buffers are released. Nothing
// is logged unless that last switch is on, except the first
// kPoolMismatchLogLines capture mismatches per pool reset (warnings). Each pool entry
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
#include <limits>
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
#include "camera_fade.hpp"
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
inline constexpr uint32_t kPoolMassRetireMeshes = 32u;  // one compaction retiring this many: a map change (diagnostic)
// Mesh capture verification: a mesh is admitted once two captures in a row
// decode to the same mesh; after this many captures without that it is
// rejected as unstable (its buffers change between frames).
inline constexpr uint32_t kPoolMeshMaxCaptures = 4u;
inline constexpr uint32_t kPoolMismatchLogLines = 32u;  // capture mismatches written to ReShade.log per pool reset
// Instance scale limits before 2026-10-06 round 7 (A/B switch "legacy scale limits").
inline constexpr float kPoolLegacyMinScale = 0.05f;
inline constexpr float kPoolLegacyMaxScale = 50.f;
inline constexpr uint32_t kPoolTrackedBloomWords = 1024u;  // 65536-bit filter of VB/IB handles meshes come from
inline constexpr size_t kPoolMaxInstances = 2000000u;
// Instance motion probe (path 2 discovery, diagnostic only): world vs
// prevWorld of every instance read back. Buckets of the largest per-element
// difference in float ULPs: 0, 1, 2-3, 4-15, 16-255, 256-65535, more (also
// sign change or non-finite).
inline constexpr size_t kPoolMotionUlpBuckets = 7u;
// Buckets of the translation difference in meters: 0, <=1e-6, <=1e-5, <=1e-4,
// <=1e-3, <=1e-2, <=1e-1, <=1, more (also non-finite).
inline constexpr size_t kPoolMotionMeterBuckets = 9u;
inline constexpr float kPoolMotionMovedMeters = 1e-3f;  // translation difference counted as moved
inline constexpr float kPoolMotionMovedBasis = 1e-4f;   // basis element difference counted as moved (rotation)
inline constexpr uint32_t kPoolMotionUlpNoise = 3u;     // largest difference still called float noise
inline constexpr size_t kPoolMotionMaxSamples = 24u;    // per sample list
inline constexpr uint32_t kPoolMotionSamplesPerFamily = 2u;
inline constexpr size_t kPoolMotionMaxMeshKeys = 4096u;
// Moving rigid objects (path 2, P2a). The game fills prevWorld (last frame's
// world, for motion vectors) in camera passes only; shadow passes leave it
// zero. M1 measured camera sightings of unmoved objects bit-exact (except
// near-zero basis elements, ~3e-8). A camera sighting moves when prevWorld
// differs beyond these limits; its mesh becomes dynamic (PoolDynamicMesh).
inline constexpr float kPoolMovingMeters = 1e-5f;  // translation; also at least 4 ULP of the coordinate
inline constexpr float kPoolMovingBasis = 1e-5f;   // basis element, times the largest basis element (>= 1)
inline constexpr uint32_t kPoolMovingHoldFrames = 2u * kPoolRecaptureFrames;
// prevWorld trace (diagnostic, read-only): three draw keys, kPoolTraceFrames
// camera copies each (see PoolPrevTrace).
inline constexpr uint32_t kPoolTraceFrames = 120u;
inline constexpr uint32_t kPoolTraceMaxElements = 4u;
inline constexpr size_t kPoolTraceKeys = 3u;
inline constexpr uint32_t kPoolTraceWaitFrames = 1800u;
inline constexpr uint32_t kPoolTraceActiveFrames = 600u;
inline constexpr float kPoolTraceFarMeters = 1.f;
inline constexpr float kPoolTraceFarBasis = 1.5f;
inline constexpr float kPoolTraceSwayMeters = 0.1f;
inline constexpr float kPoolTraceSwayBasis = 0.1f;
inline constexpr uint32_t kPoolTraceStillSightings = 4u;
inline constexpr size_t kPoolTraceMaxRecords = 1500u;
inline constexpr size_t kPoolMaxDynamicKeys = 4096u;
inline constexpr size_t kPoolMotionRuleSamples = 16u;
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

// Camera-visibility inputs of one instance, read from its InstanceParam
// (contract::kColorOffset / kParamOffset; formula in shader_contract.hpp).
struct PoolVisibility {
  bool valid = false;   // the drawing VS declares the contract layout
  float color[4] = {};  // w: opacity
  float param[4] = {};  // x: near-fade start (m), y: near-fade 1/range, z: dither flip when > 0
};

// Draw facts a copy carries to its resolve (PoolPendingCopy::pass). The view
// comes from the vertex shader (contract::VsView): a render target is bound in
// shadow passes too, so it says nothing about the pass.
inline constexpr uint8_t kPoolPassCamera = 1u;      // VS projects with the scene camera
inline constexpr uint8_t kPoolPassVisibility = 2u;  // the VS instance element has the contract visibility layout
inline constexpr uint8_t kPoolPassNearFadePs = 4u;  // the PS applies the map-object near fade
inline constexpr uint8_t kPoolPassLight = 8u;       // VS projects with the light (shadow maps)

struct PoolSighting {
  uint8_t pass = 0u;     // kPoolPass* bits
  uint32_t ps_hash = 0u;
};

struct WorldMesh {
  uint64_t mesh_key = 0u;  // first draw key that produced this mesh
  uint64_t uid = 0u;       // never reused; the live BVH store keys GPU meshes by it
  uint32_t mesh_id = 0u;
  uint32_t source_vs_hash = 0u;
  uint32_t triangle_count = 0u;
  float bbox_min[3] = {};
  float bbox_max[3] = {};
  std::vector<std::array<float, 3>> positions;
  std::vector<uint32_t> indices;  // flat, three per triangle
  uint64_t signature = 0u;        // content hash (positions + indices)
  uint32_t live_keys = 0u;        // draw keys still mapped to this mesh
  // Provenance (diagnostic): the draw whose copies produced the mesh.
  uint32_t capture_frame = 0u;
  uint64_t source_vb = 0u;
  uint64_t source_ib = 0u;
  bool from_indirect = false;
  uint32_t writes_after_capture = 0u;  // game writes to its VB/IB seen since (the mesh may be stale)
  uint32_t captures = 0u;              // captures it took
  bool verified = false;               // the last two captures decoded to the same mesh
  uint32_t source_vb_usage = 0u;       // reshade::api::resource_usage / resource_flags bits of its VB and IB
  uint32_t source_vb_flags = 0u;
  uint32_t source_ib_usage = 0u;
  uint32_t source_ib_flags = 0u;
  // Sampled draws of it per view; how many camera draws used a pixel shader
  // that near-fades (GetPoolCameraVisibility).
  uint32_t camera_draws = 0u;
  uint32_t camera_near_fade_draws = 0u;
  uint32_t light_draws = 0u;
  uint32_t camera_ps_hash = 0u;  // latest camera-draw pixel shader (display)
  // A draw key of this mesh was seen moving in a camera view (PoolDynamicMesh)
  // while "keep moving objects out" was on: no instance of it is admitted.
  bool dynamic = false;
};

struct WorldInstance {
  uint64_t mesh_key = 0u;  // draw key it was admitted through; retired with it
  uint32_t mesh_id = 0u;
  uint32_t source_vs_hash = 0u;
  uint8_t kind = static_cast<uint8_t>(CandidateKind::Inst4x3Row);
  uint8_t source = static_cast<uint8_t>(InstanceSource::CanonicalInstance);
  uint32_t matrix_floats = kPoolWorldFloats;
  float matrix[16] = {};
  float inverse_world[16] = {};  // inverse of the affine matrix (ComputeMatrixInverse), at admission
  float bounds_min[3] = {};
  float bounds_max[3] = {};
  // Provenance (diagnostic).
  uint32_t first_frame = 0u;  // first sighting of this (mesh, matrix)
  uint32_t admit_frame = 0u;
  float admit_camera[3] = {};
  bool admit_camera_valid = false;
  PoolVisibility visibility;  // camera-visibility inputs at admission
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
  uint32_t count = 0u;       // frames it was seen in (kept counting after admission, diagnostic)
  uint32_t first_frame = 0u;
  uint32_t last_frame = 0u;
  uint32_t vs_hash = 0u;
  bool admitted = false;
  bool rejected = false;
  float matrix[kPoolWorldFloats] = {};
  // Per view (kept after admission): frames seen, and the latest inputs.
  uint32_t camera_frames = 0u;  // drawn by a camera VS
  uint32_t light_frames = 0u;   // drawn by a light VS (shadow maps)
  uint32_t last_camera_frame = 0u;
  uint32_t last_light_frame = 0u;
  uint32_t camera_sightings = 0u;            // camera draws that read it
  uint32_t camera_near_fade_sightings = 0u;  // ... with a pixel shader that near-fades
  uint32_t camera_ps_hash = 0u;              // latest camera-draw pixel shader (display)
  PoolVisibility camera_visibility;
  PoolVisibility light_visibility;
  // Admitted as a duplicate of an instance reached through another draw key
  // (same mesh content and matrix): that key, whose observation also receives
  // this one's sightings. 0 otherwise.
  uint64_t merged_mesh_key = 0u;
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
  bool from_indirect = false;
  bool written = false;  // its VB/IB was written by the game while queued (diagnostic)
  std::vector<uint32_t> indices;  // absolute vertex indices, after the index read
  uint32_t min_vertex = 0u;
  uint32_t max_vertex = 0u;
  // Verification: the previous complete capture, applied once the next one
  // decodes to the same mesh.
  uint32_t captures = 0u;
  bool verified = false;  // set when the latest capture matched the previous one
  bool has_previous = false;
  uint64_t previous_signature = 0u;
  uint64_t previous_index_signature = 0u;  // absolute indices of the previous capture
  uint32_t previous_frame = 0u;
  uint32_t previous_min_vertex = 0u;
  uint32_t previous_max_vertex = 0u;
  PoolDecodedMesh previous;
  // The draws that served this capture's copies (diagnostic): indirect or not,
  // and the copy source offsets.
  bool indices_served_indirect = false;
  bool vertices_served_indirect = false;
  uint64_t indices_source_offset = 0u;
  uint64_t vertices_source_offset = 0u;
  uint32_t indices_frame = 0u;
  uint32_t vertices_frame = 0u;
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
  uint32_t ps_hash = 0u;
  uint8_t pass = 0u;       // kPoolPass* bits of the draw
  uint64_t mesh_key = 0u;  // 0 for an indirect draw until its args are read
  // Indirect draws only: the draw record in PoolStagingSlot::indirect_draws
  // (its counts are filled from the args at resolve), the args location and
  // the draw identity whose next window the args decide.
  uint32_t indirect_index = UINT32_MAX;
  uint64_t args_offset = 0u;
  uint64_t schedule_key = 0u;
  bool trace_only = false;  // prevWorld trace copy: not counted, not queued, not stamped
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

// One instance sighting of the motion probe (see PoolMotionStats).
struct PoolMotionSample {
  uint32_t vs_hash = 0u;
  uint64_t mesh_key = 0u;
  uint32_t frame = 0u;
  uint8_t pass = 0u;          // kPoolPass* bits of the draw
  bool indirect = false;
  uint32_t draw_instances = 0u;  // instances the draw read back
  uint32_t element = 0u;         // this instance's element in the draw
  uint32_t draw_moved = 0u;      // elements of the draw counted as moved
  bool repeat = false;           // the same (mesh, world) was seen in an earlier frame
  uint32_t observed_frames = 0u; // frames that (mesh, world) was seen in before
  bool admitted = false;         // that (mesh, world) was already an instance
  uint32_t ulps = 0u;            // largest element difference, world vs prevWorld
  float meters = 0.f;            // translation difference
  float basis = 0.f;             // largest basis element difference
  float world[kPoolWorldFloats] = {};
  float prev_world[kPoolWorldFloats] = {};
};

// What prevWorld (InstanceParam @48, the matrix the game's VS projects with
// prevViewProj_g for motion vectors) holds relative to world. A "repeat"
// sighting is one whose exact (mesh, world) the pool saw in an earlier frame:
// the object has not moved since, so if prevWorld is last frame's world it
// differs from world by float noise at most. A "first" sighting is a matrix
// not seen before (a new object, or one that moved).
struct PoolMotionStats {
  uint64_t repeat = 0u;
  uint64_t first = 0u;
  uint64_t exact = 0u;  // world and prevWorld bitwise equal
  uint64_t nonfinite = 0u;  // prevWorld has a non-finite element
  std::array<uint64_t, kPoolMotionUlpBuckets> repeat_ulps = {};
  std::array<uint64_t, kPoolMotionUlpBuckets> first_ulps = {};
  std::array<uint64_t, kPoolMotionMeterBuckets> repeat_meters = {};
  std::array<uint64_t, kPoolMotionMeterBuckets> first_meters = {};
  uint32_t max_repeat_ulps = 0u;
  float max_repeat_meters = 0.f;
  // Moved: translation difference > kPoolMotionMovedMeters or a basis element
  // difference > kPoolMotionMovedBasis.
  uint64_t moved = 0u;
  uint64_t moved_repeat = 0u;  // moved although the same world was seen before
  uint64_t moved_indirect = 0u;
  uint64_t moved_camera = 0u;  // drawn by a camera VS
  uint64_t moved_light = 0u;   // drawn by a light VS
  // Copies (draws read back) with at least one moved element, and how many
  // of them had every element moved.
  uint64_t moving_draws = 0u;
  uint64_t moving_draws_all = 0u;
  uint64_t moving_draws_indirect = 0u;
  uint64_t moving_draw_instances = 0u;  // elements of those draws
  uint32_t moved_meshes = 0u;  // distinct mesh keys seen moving (PoolMotionDetail)
  // Per view (M1 found prevWorld zero in shadow passes).
  uint64_t camera_repeat = 0u;
  uint64_t camera_repeat_exact = 0u;
  uint64_t camera_zero_prev = 0u;  // camera sightings with prevWorld all zero
  uint64_t light_repeat = 0u;
  uint64_t light_zero_prev = 0u;
  // The P2a rule (PoolSightingMoves) on camera sightings: how many it calls
  // prevWorld-differs, and how many of those had a (mesh, world) already seen
  // in an earlier frame (stale: not counted as moving).
  uint64_t rule_moving = 0u;
  uint64_t rule_stale = 0u;
};

// Sample lists and keys of the motion probe: kept out of PoolStats, which the
// panel copies every frame. Reset with the stats.
struct PoolMotionDetail {
  std::vector<PoolMotionSample> moved_samples;   // first sightings that moved
  std::vector<PoolMotionSample> repeat_samples;  // repeat sightings beyond float noise
  std::unordered_set<uint64_t> moved_mesh_keys;  // meshes seen moving (capped)
  bool moved_mesh_keys_full = false;
  std::vector<PoolMotionSample> rule_stale_samples;  // rule_stale cases
};

// A draw key seen moving in a camera view (path 2, P2a), with its evidence.
struct PoolDynamicMesh {
  uint32_t vs_hash = 0u;
  uint32_t first_frame = 0u;
  uint32_t last_frame = 0u;  // last moving sighting
  uint64_t moving_sightings = 0u;
  uint64_t indirect_sightings = 0u;
  float max_meters = 0.f;  // translation difference world - prevWorld
  float max_basis = 0.f;
  uint32_t retired_instances = 0u;  // admitted instances of its mesh removed when it became dynamic
  uint64_t blocked = 0u;            // admissions refused since
  bool moving_now = true;           // moving evidence not yet released (see NotePoolCameraMotion)
  uint32_t released = 0u;           // times released by a still sighting
  float world[kPoolWorldFloats] = {};       // the sighting with the largest translation difference
  float prev_world[kPoolWorldFloats] = {};
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
  uint32_t mesh_mismatches = 0u;    // capture mismatches of meshes first queued by this VS
  uint32_t mesh_unstable = 0u;
  // Motion probe (PoolMotionStats).
  uint64_t motion_repeat = 0u;
  uint64_t motion_first = 0u;
  uint64_t motion_moved = 0u;
  uint64_t motion_moved_indirect = 0u;
  uint32_t motion_max_repeat_ulps = 0u;
  float motion_max_moved_meters = 0.f;
  uint32_t motion_moved_samples = 0u;
  uint32_t motion_repeat_samples = 0u;
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
  uint64_t indirect_first_copies = 0u;      // copied with the full window: identity not read before
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
  PoolMotionStats motion;
  // Moving rigid objects (P2a).
  uint32_t dynamic_keys = 0u;          // draw keys seen moving
  uint32_t dynamic_cap_drops = 0u;     // moving keys not recorded (kPoolMaxDynamicKeys)
  uint32_t dynamic_meshes_marked = 0u; // meshes flagged dynamic (counts re-flags after a switch change)
  uint32_t dynamic_retired = 0u;       // admitted instances removed with them
  uint64_t dynamic_blocked = 0u;       // admissions refused
  uint32_t dynamic_released = 0u;      // meshes unflagged when their moving keys were released
  size_t dynamic_moving_keys = 0u;     // draw keys moving now (refreshed by UpdatePoolStats)
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
  // Game writes to VB/IB that captured or queued meshes come from (diagnostic:
  // a written buffer may no longer hold the captured geometry).
  uint64_t tracked_buffer_writes = 0u;
  uint32_t requests_written = 0u;
  size_t meshes_written = 0u;  // refreshed by UpdatePoolStats
  // Compactions that retired at least kPoolMassRetireMeshes meshes (map changes).
  uint32_t mass_retirements = 0u;
  // Mesh capture verification.
  uint32_t mesh_verified = 0u;            // admitted after two identical captures
  uint32_t mesh_capture_mismatches = 0u;  // a capture that differed from the previous one
  uint32_t mesh_unstable = 0u;            // rejected: kPoolMeshMaxCaptures without two identical in a row
  uint32_t last_mass_retire_frame = 0u;
  uint32_t last_mass_retire_meshes = 0u;
  // Changes of what GetPoolCameraVisibility decides for admitted instances
  // (each one asks the live BVH for a TLAS rebuild).
  uint64_t visibility_changes = 0u;
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
  size_t dynamic_live_keys = 0u;  // draw keys seen moving, still mapped
  size_t dynamic_meshes = 0u;     // meshes flagged dynamic now
  uint64_t scan_first_frame = 0u;
  uint64_t scan_last_frame = 0u;
};

struct PoolMotion {
  bool finite = true;    // prevWorld finite (world was checked plausible)
  bool exact = false;    // bitwise equal
  bool moved = false;    // beyond kPoolMotionMovedMeters / kPoolMotionMovedBasis
  uint32_t ulps = 0u;    // largest element difference
  float meters = 0.f;    // translation difference
  float basis = 0.f;     // largest basis element difference
};

// The prevWorld trace (diagnostic only): keys[0] = A (far flip), keys[1] = B
// (slow sway), keys[2] = C (static, admitted). A key in Active state records
// one camera copy per frame (world and prevWorld of each element, up to
// kPoolTraceMaxElements), trace-only copies included; nothing else reads them.
struct PoolPrevTrace {
  enum class KeyState : uint8_t { Waiting, Active, Done, GaveUp };
  struct Key {
    KeyState state = KeyState::Waiting;
    uint64_t mesh_key = 0u;
    uint32_t vs_hash = 0u;
    uint32_t selected_frame = 0u;
    uint32_t frames_recorded = 0u;
    uint32_t last_frame = 0u;   // frame of the last recorded copy
    uint32_t last_count = 0u;   // its instance count
    float last_world[kPoolWorldFloats] = {};  // its element 0
    uint32_t extra_camera_draws = 0u;         // further camera copies of the key in a frame
    uint32_t missed = 0u;                     // frames without a copy since the first
    // Evidence the key was selected on.
    float max_meters = 0.f;
    float max_basis = 0.f;
    uint64_t moving_sightings = 0u;
    uint64_t camera_sightings = 0u;
  };
  struct Record {
    uint8_t key = 0u;  // index into keys
    uint32_t frame = 0u;
    uint32_t element = 0u;
    uint32_t draw_instances = 0u;
    uint32_t camera_draws = 0u;  // camera copies of the key this frame recorded (always 1; more are extra_camera_draws)
    int32_t cpu_base = 0;
    bool trace_only = false;
    float world[kPoolWorldFloats] = {};
    float prev_world[kPoolWorldFloats] = {};
    bool prev_filled = false;
    PoolMotion prev_vs_world;
    bool has_last = false;  // element 0, the key's previous copy was the previous frame
    PoolMotion prev_vs_last;  // prev_world against the previous world
    float world_step_meters = 0.f;
  };
  uint32_t armed_frame = 0u;  // 0 = not armed
  bool written = false;
  std::vector<Record> records;
  std::array<Key, kPoolTraceKeys> keys = {};
};

struct PoolState {
  std::atomic_bool scan_active{false};
  // Diagnostic switches (see the header comment).
  std::atomic_bool capture_meshes{true};
  std::atomic_bool scan_indirect{true};
  std::atomic_bool log_captures{false};
  std::atomic_bool verify_meshes{true};   // admit a mesh only after two identical captures
  std::atomic_bool legacy_scale{false};   // instance scale limits of round 6 (0.05 .. 50)
  std::atomic_bool exclude_moving{true};  // meshes seen moving in a camera view stay out of the static pool
  uint32_t mismatch_lines_logged = 0u;
  // The immediate command list (set at present), to tell deferred-context draws apart.
  std::atomic_uint64_t immediate_cmd_list{0u};
  float region_size = 512.f;
  std::mutex mutex;

  uint32_t logged_invalidations = 0u;  // stats.resource_invalidations at the last present

  // Camera at the slot being resolved, stamped on instances admitted there.
  float resolve_camera[3] = {};
  bool resolve_camera_valid = false;
  // Bloom filter of VB/IB handles in keys_by_resource, so the write events
  // only take the lock for buffers a mesh may come from. Bits are never
  // cleared (a stale bit only costs a lock and a lookup).
  std::array<std::atomic_uint64_t, kPoolTrackedBloomWords> tracked_bloom{};

  // Staging ring. Draws of the frame in flight write `write_slot`.
  reshade::api::device* staging_device = nullptr;
  std::array<PoolStagingSlot, kPoolStagingSlots> slots;
  uint32_t write_slot = 0u;

  // Meshes.
  std::unordered_map<uint64_t, PoolMeshRequest> mesh_requests;       // by mesh key
  std::unordered_map<uint64_t, std::vector<uint64_t>> mesh_waiting;  // buffer key -> keys waiting for a copy
  uint64_t next_mesh_serial = 1u;
  uint64_t next_mesh_uid = 1u;  // WorldMesh::uid, never reused (also not by ResetWorldPool)
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
  uint64_t visibility_revision = 0u;  // bumped when an admitted instance's camera visibility changes
  PoolStats stats;
  PoolMotionDetail motion_detail;
  // Moving rigid objects (P2a): draw keys seen moving. `dynamic_applied` is
  // the exclude_moving value the mesh flags follow (re-applied under the lock
  // when the switch changes).
  std::unordered_map<uint64_t, PoolDynamicMesh> dynamic_keys;
  bool dynamic_applied = false;
  PoolPrevTrace prev_trace;
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
// length in [kPoolMinScale, kPoolMaxScale) ([kPoolLegacyMinScale,
// kPoolLegacyMaxScale) while the legacy-scale switch is on) and no two axes
// nearly parallel. Returns the first failing test.
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
  const bool legacy = g_pool.legacy_scale.load(std::memory_order_relaxed);
  const float min_scale = legacy ? kPoolLegacyMinScale : kPoolMinScale;
  const float max_scale = legacy ? kPoolLegacyMaxScale : kPoolMaxScale;
  for (int r = 0; r < 3; ++r) {
    if (!(lengths[r] >= min_scale)) return PoolMatrixReject::SmallScale;
    if (!(lengths[r] < max_scale)) return PoolMatrixReject::LargeScale;
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

// An axis length outside the legacy limits (diagnostic: instances the legacy
// switch would reject).
inline bool PoolOutsideLegacyScale(const float* matrix) {
  for (const uint32_t row : {0u, 4u, 8u}) {
    const float length = std::sqrt(matrix[row] * matrix[row] + matrix[row + 1u] * matrix[row + 1u]
                                   + matrix[row + 2u] * matrix[row + 2u]);
    if (!(length >= kPoolLegacyMinScale) || !(length < kPoolLegacyMaxScale)) return true;
  }
  return false;
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
  for (const auto& slot : g_pool.slots) {
    for (const auto& copy : slot.copies) {
      if (!copy.trace_only) queued += 1u;
    }
  }
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
  size_t written = 0u;
  size_t dynamic_meshes = 0u;
  for (const auto& mesh : g_pool.meshes) {
    if (mesh.writes_after_capture != 0u) written += 1u;
    if (mesh.dynamic) dynamic_meshes += 1u;
  }
  g_pool.stats.meshes_written = written;
  size_t moving_keys = 0u;
  for (const auto& item : g_pool.dynamic_keys) {
    if (item.second.moving_now) moving_keys += 1u;
  }
  g_pool.stats.dynamic_live_keys = g_pool.dynamic_keys.size();
  g_pool.stats.dynamic_moving_keys = moving_keys;
  g_pool.stats.dynamic_meshes = dynamic_meshes;
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
  if (g_pool.meshes[mesh_id].dynamic) {
    // Seen moving in a camera view (P2a): a pose of it would stay behind as a
    // ghost when it moves again.
    g_pool.stats.dynamic_blocked += 1u;
    const auto dynamic = g_pool.dynamic_keys.find(mesh_key);
    if (dynamic != g_pool.dynamic_keys.end()) dynamic->second.blocked += 1u;
    return false;
  }
  const uint64_t admitted_key = PoolAdmittedKey(mesh_id, MatrixHash(observed.matrix, kPoolWorldFloats));
  if (g_pool.admitted_keys.count(admitted_key) != 0u) {
    // Same mesh content at the same place, reached through another draw key
    // (another pass or a duplicate VB): one instance is enough. Its sightings
    // are forwarded to that instance's observation (rare: one scan here).
    observed.admitted = true;
    g_pool.stats.dedup_instances += 1u;
    const uint64_t matrix_hash = MatrixHash(observed.matrix, kPoolWorldFloats);
    for (const WorldInstance& existing : g_pool.instances) {
      if (existing.mesh_id == mesh_id && existing.mesh_key != mesh_key
          && MatrixHash(existing.matrix, kPoolWorldFloats) == matrix_hash) {
        observed.merged_mesh_key = existing.mesh_key;
        break;
      }
    }
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
  ComputeMatrixInverse(instance.matrix, instance.inverse_world);
  instance.first_frame = observed.first_frame;
  instance.admit_frame = g_state.frame.load();
  instance.admit_camera_valid = g_pool.resolve_camera_valid;
  std::memcpy(instance.admit_camera, g_pool.resolve_camera, sizeof(instance.admit_camera));
  instance.visibility = observed.camera_visibility.valid ? observed.camera_visibility : observed.light_visibility;
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

// Caller holds g_pool.mutex. Per-view bookkeeping of one sighting (a VS of
// neither view records nothing here).
inline void NotePoolSighting(ObservedInstance& observed, const PoolSighting& sighting, const PoolVisibility* visibility,
                             uint32_t frame) {
  const bool has_inputs = visibility != nullptr && visibility->valid;
  if ((sighting.pass & kPoolPassCamera) != 0u) {
    if (observed.camera_frames == 0u || observed.last_camera_frame != frame) {
      observed.camera_frames += 1u;
      observed.last_camera_frame = frame;
    }
    observed.camera_sightings += 1u;
    if ((sighting.pass & kPoolPassNearFadePs) != 0u) observed.camera_near_fade_sightings += 1u;
    observed.camera_ps_hash = sighting.ps_hash;
    if (has_inputs) observed.camera_visibility = *visibility;
  } else if ((sighting.pass & kPoolPassLight) != 0u) {
    if (observed.light_frames == 0u || observed.last_light_frame != frame) {
      observed.light_frames += 1u;
      observed.last_light_frame = frame;
    }
    if (has_inputs) observed.light_visibility = *visibility;
  }
}

// More than half of the camera draws used a near-fading pixel shader.
inline bool PoolMostlyNearFade(uint32_t near_fade, uint32_t total) {
  return total != 0u && near_fade * 2u > total;
}

// What GetPoolCameraVisibility uses from one observation, compared before and
// after a sighting: a change asks for a TLAS rebuild.
struct PoolViewDecision {
  bool camera_seen = false;
  bool light_seen = false;
  bool near_fade = false;
  float inputs[2] = {};  // near-fade start and 1/range (bitwise compared: NaN-safe)
};

inline bool SamePoolView(const PoolViewDecision& a, const PoolViewDecision& b) {
  return a.camera_seen == b.camera_seen && a.light_seen == b.light_seen && a.near_fade == b.near_fade
         && std::memcmp(a.inputs, b.inputs, sizeof(a.inputs)) == 0;
}

inline PoolViewDecision PoolObservationView(const ObservedInstance& observed) {
  PoolViewDecision decision;
  decision.camera_seen = observed.camera_sightings != 0u;
  decision.light_seen = observed.light_frames != 0u;
  decision.near_fade = PoolMostlyNearFade(observed.camera_near_fade_sightings, observed.camera_sightings);
  const PoolVisibility& inputs = observed.camera_visibility.valid ? observed.camera_visibility : observed.light_visibility;
  if (inputs.valid) {
    decision.inputs[0] = inputs.param[0];
    decision.inputs[1] = inputs.param[1];
  }
  return decision;
}

// Caller holds g_pool.mutex.
inline void NotePoolVisibilityChange() {
  g_pool.visibility_revision += 1u;
  g_pool.stats.visibility_changes += 1u;
}

// Caller holds g_pool.mutex. NotePoolSighting, plus a visibility change when
// it alters what an admitted instance's TLAS entry would get.
inline void NotePoolSightingTracked(ObservedInstance& observed, const PoolSighting& sighting, const PoolVisibility* visibility,
                                    uint32_t frame) {
  const PoolViewDecision before = PoolObservationView(observed);
  NotePoolSighting(observed, sighting, visibility, frame);
  if (observed.admitted && !SamePoolView(before, PoolObservationView(observed))) NotePoolVisibilityChange();
}

// Caller holds g_pool.mutex. The view one sampled draw of a mesh ran in
// (diagnostic: the camera visibility of instances is decided per instance).
inline void NotePoolMeshPass(uint64_t mesh_key, const PoolSighting& sighting) {
  const auto it = g_pool.mesh_by_key.find(mesh_key);
  if (it == g_pool.mesh_by_key.end() || it->second >= g_pool.meshes.size()) return;
  WorldMesh& mesh = g_pool.meshes[it->second];
  if ((sighting.pass & kPoolPassCamera) != 0u) {
    mesh.camera_draws += 1u;
    if ((sighting.pass & kPoolPassNearFadePs) != 0u) mesh.camera_near_fade_draws += 1u;
    mesh.camera_ps_hash = sighting.ps_hash;
  } else if ((sighting.pass & kPoolPassLight) != 0u) {
    mesh.light_draws += 1u;
  }
}

// Caller holds g_pool.mutex. `sighting` and `visibility` (both optional) are
// what the draw tells about the pass and the instance's camera visibility.
inline void ObservePoolInstance(uint64_t mesh_key, uint32_t vs_hash, const float* world, uint32_t frame,
                                const PoolSighting* sighting = nullptr, const PoolVisibility* visibility = nullptr) {
  const InstanceKey key{mesh_key, MatrixHash(world, kPoolWorldFloats)};
  ObservedInstance& observed = g_pool.observations[key];
  if (observed.rejected) return;
  if (sighting != nullptr) {
    NotePoolSightingTracked(observed, *sighting, visibility, frame);
    if (observed.merged_mesh_key != 0u) {
      const auto merged = g_pool.observations.find(InstanceKey{observed.merged_mesh_key, key.matrix_hash});
      if (merged != g_pool.observations.end()) NotePoolSightingTracked(merged->second, *sighting, visibility, frame);
    }
  }
  if (observed.admitted) {
    // Diagnostic only: how often and how recently an admitted instance is seen.
    if (observed.last_frame != frame) {
      observed.count += 1u;
      observed.last_frame = frame;
    }
    return;
  }
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
  if (observed.count == 0u) observed.first_frame = frame;
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
// Instance motion probe (diagnostic only; see PoolMotionStats).

// Distance between two floats in ULPs (+0 and -0 are equal). UINT32_MAX when
// either is non-finite; finite distances are capped below that.
inline uint32_t PoolUlpDistance(float a, float b) {
  if (!std::isfinite(a) || !std::isfinite(b)) return UINT32_MAX;
  const auto ordered = [](float value) {
    int32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    return bits < 0 ? -static_cast<int64_t>(bits & 0x7FFFFFFF) : static_cast<int64_t>(bits);
  };
  const int64_t difference = ordered(a) - ordered(b);
  const uint64_t distance = static_cast<uint64_t>(difference < 0 ? -difference : difference);
  return distance >= UINT32_MAX ? UINT32_MAX - 1u : static_cast<uint32_t>(distance);
}

inline PoolMotion MeasurePoolMotion(const float* world, const float* prev_world) {
  PoolMotion motion;
  motion.exact = std::memcmp(world, prev_world, sizeof(float) * kPoolWorldFloats) == 0;
  for (uint32_t i = 0; i < kPoolWorldFloats; ++i) {
    if (!std::isfinite(prev_world[i])) motion.finite = false;
    motion.ulps = (std::max)(motion.ulps, PoolUlpDistance(world[i], prev_world[i]));
  }
  if (!motion.finite) {
    motion.meters = std::numeric_limits<float>::quiet_NaN();
    motion.basis = std::numeric_limits<float>::quiet_NaN();
    return motion;
  }
  float translation[3] = {};
  float prev_translation[3] = {};
  PoolMatrixTranslation(world, translation);
  PoolMatrixTranslation(prev_world, prev_translation);
  const float dx = translation[0] - prev_translation[0];
  const float dy = translation[1] - prev_translation[1];
  const float dz = translation[2] - prev_translation[2];
  motion.meters = std::sqrt(dx * dx + dy * dy + dz * dz);
  for (uint32_t i = 0; i < kPoolWorldFloats; ++i) {
    if (i == 3u || i == 7u || i == 11u) continue;  // translation (PoolMatrixTranslation)
    motion.basis = (std::max)(motion.basis, std::fabs(world[i] - prev_world[i]));
  }
  motion.moved = motion.meters > kPoolMotionMovedMeters || motion.basis > kPoolMotionMovedBasis;
  return motion;
}

inline size_t PoolMotionUlpBucket(uint32_t ulps) {
  if (ulps == 0u) return 0u;
  if (ulps == 1u) return 1u;
  if (ulps < 4u) return 2u;
  if (ulps < 16u) return 3u;
  if (ulps < 256u) return 4u;
  if (ulps < 65536u) return 5u;
  return 6u;
}

inline size_t PoolMotionMeterBucket(float meters) {
  if (!(meters >= 0.f)) return kPoolMotionMeterBuckets - 1u;  // non-finite
  if (meters == 0.f) return 0u;
  constexpr float kLimits[] = {1e-6f, 1e-5f, 1e-4f, 1e-3f, 1e-2f, 1e-1f, 1.f};
  for (size_t i = 0; i < sizeof(kLimits) / sizeof(kLimits[0]); ++i) {
    if (meters <= kLimits[i]) return i + 1u;
  }
  return kPoolMotionMeterBuckets - 1u;
}

inline const char* PoolMotionUlpBucketName(size_t bucket) {
  constexpr const char* kNames[kPoolMotionUlpBuckets] = {"0", "1", "2-3", "4-15", "16-255", "256-65535", "more"};
  return bucket < kPoolMotionUlpBuckets ? kNames[bucket] : "?";
}

inline const char* PoolMotionMeterBucketName(size_t bucket) {
  constexpr const char* kNames[kPoolMotionMeterBuckets] = {"0",     "<=1e-6", "<=1e-5", "<=1e-4", "<=1e-3",
                                                           "<=0.01", "<=0.1", "<=1",    "more"};
  return bucket < kPoolMotionMeterBuckets ? kNames[bucket] : "?";
}

// ---------------------------------------------------------------------------
// Moving rigid objects (path 2, P2a).

// The game left prevWorld unfilled (all zero: shadow passes).
inline bool PoolPrevWorldFilled(const float* prev_world) {
  bool identity = true;
  for (uint32_t i = 0; i < kPoolWorldFloats; ++i) {
    const float expected = (i == 0u || i == 5u || i == 10u) ? 1.f : 0.f;
    if (prev_world[i] != expected) identity = false;
  }
  if (identity) return false;
  for (uint32_t i = 0; i < kPoolWorldFloats; ++i) {
    if (prev_world[i] != 0.f) return true;
  }
  return false;
}

// The P2a rule for one camera-view sighting: prevWorld filled and finite, and
// a translation component off by more than max(kPoolMovingMeters, 4 ULP of
// the coordinate) or a basis element off by more than kPoolMovingBasis times
// the largest basis element (at least 1).
inline bool PoolSightingMoves(const float* world, const float* prev_world) {
  if (!PoolPrevWorldFilled(prev_world)) return false;
  for (uint32_t i = 0; i < kPoolWorldFloats; ++i) {
    if (!std::isfinite(prev_world[i]) || !std::isfinite(world[i])) return false;
  }
  float scale = 1.f;
  for (uint32_t i = 0; i < kPoolWorldFloats; ++i) {
    if (i == 3u || i == 7u || i == 11u) continue;
    scale = (std::max)(scale, std::fabs(world[i]));
  }
  for (uint32_t i = 0; i < kPoolWorldFloats; ++i) {
    const float difference = std::fabs(world[i] - prev_world[i]);
    if (i == 3u || i == 7u || i == 11u) {
      const float magnitude = (std::max)(std::fabs(world[i]), std::fabs(prev_world[i]));
      if (difference > (std::max)(kPoolMovingMeters, magnitude * 4.76837158e-7f)) return true;  // 2^-21
    } else if (difference > kPoolMovingBasis * scale) {
      return true;
    }
  }
  return false;
}

// Caller holds g_pool.mutex. Flags a mesh dynamic and removes its admitted
// instances (their observations no longer count as admitted; AdmitPoolInstance
// refuses them while the flag is set). Returns the instances removed.
inline uint32_t MarkPoolMeshDynamic(uint32_t mesh_id) {
  if (mesh_id >= g_pool.meshes.size() || g_pool.meshes[mesh_id].dynamic) return 0u;
  g_pool.meshes[mesh_id].dynamic = true;
  g_pool.stats.dynamic_meshes_marked += 1u;
  uint32_t removed = 0u;
  size_t write = 0u;
  for (size_t read = 0u; read < g_pool.instances.size(); ++read) {
    WorldInstance& instance = g_pool.instances[read];
    if (instance.mesh_id == mesh_id) {
      g_pool.admitted_keys.erase(PoolAdmittedKey(mesh_id, MatrixHash(instance.matrix, kPoolWorldFloats)));
      removed += 1u;
      continue;
    }
    if (write != read) g_pool.instances[write] = std::move(instance);
    write += 1u;
  }
  g_pool.instances.resize(write);
  for (auto& [key, observed] : g_pool.observations) {
    if (!observed.admitted) continue;
    const auto mesh_it = g_pool.mesh_by_key.find(key.mesh_key);
    if (mesh_it != g_pool.mesh_by_key.end() && mesh_it->second == mesh_id) observed.admitted = false;
  }
  if (removed != 0u) g_pool.revision += 1u;
  g_pool.stats.dynamic_retired += removed;
  return removed;
}

// Caller holds g_pool.mutex. Marks the mesh of a draw key seen moving, when
// moving objects are kept out and the key's mesh is captured.
inline void ApplyPoolDynamicKey(uint64_t mesh_key, PoolDynamicMesh& entry) {
  if (!g_pool.dynamic_applied || !entry.moving_now) return;
  const auto mesh_it = g_pool.mesh_by_key.find(mesh_key);
  if (mesh_it == g_pool.mesh_by_key.end()) return;
  entry.retired_instances += MarkPoolMeshDynamic(mesh_it->second);
}

// Caller holds g_pool.mutex. Makes the mesh flags follow the switch: on flags
// the meshes of every key seen moving (removing their instances); off clears
// the flags (their observations admit again at their next sighting).
inline void ApplyPoolDynamicSwitch() {
  const bool on = g_pool.exclude_moving.load(std::memory_order_relaxed);
  if (on == g_pool.dynamic_applied) return;
  g_pool.dynamic_applied = on;
  if (on) {
    for (auto& [key, entry] : g_pool.dynamic_keys) ApplyPoolDynamicKey(key, entry);
  } else {
    for (WorldMesh& mesh : g_pool.meshes) mesh.dynamic = false;
  }
}

// Verdict of one camera sighting (NotePoolMotion). Unknown: no evidence either way.
enum class PoolSightingVerdict : uint8_t { Unknown, Still, Moving };

// Caller holds g_pool.mutex. Acts on a camera sighting's verdict: a moving
// sighting marks its key moving; a still one releases a moving key once
// kPoolMovingHoldFrames passed since its last moving sighting, and unflags
// its mesh when no other moving key maps to it.
inline void NotePoolCameraMotion(uint64_t mesh_key, uint32_t vs_hash, const float* world, const float* prev_world,
                                 bool indirect, uint32_t frame, PoolSightingVerdict verdict) {
  auto it = g_pool.dynamic_keys.find(mesh_key);
  if (verdict == PoolSightingVerdict::Still) {
    if (it == g_pool.dynamic_keys.end()) return;
    PoolDynamicMesh& entry = it->second;
    if (!entry.moving_now || frame < entry.last_frame + kPoolMovingHoldFrames) return;
    entry.moving_now = false;
    entry.released += 1u;
    const auto mesh_it = g_pool.mesh_by_key.find(mesh_key);
    if (g_pool.dynamic_applied && mesh_it != g_pool.mesh_by_key.end() && mesh_it->second < g_pool.meshes.size()
        && g_pool.meshes[mesh_it->second].dynamic) {
      bool shared = false;
      for (const auto& [other_key, other] : g_pool.dynamic_keys) {
        if (!other.moving_now || other_key == mesh_key) continue;
        const auto other_mesh = g_pool.mesh_by_key.find(other_key);
        if (other_mesh != g_pool.mesh_by_key.end() && other_mesh->second == mesh_it->second) shared = true;
      }
      if (!shared) {
        g_pool.meshes[mesh_it->second].dynamic = false;
        g_pool.stats.dynamic_released += 1u;
      }
    }
    return;
  }
  if (it == g_pool.dynamic_keys.end()) {
    if (g_pool.dynamic_keys.size() >= kPoolMaxDynamicKeys) {
      g_pool.stats.dynamic_cap_drops += 1u;
      return;
    }
    it = g_pool.dynamic_keys.emplace(mesh_key, PoolDynamicMesh{}).first;
    it->second.vs_hash = vs_hash;
    it->second.first_frame = frame;
    g_pool.stats.dynamic_keys += 1u;
  }
  PoolDynamicMesh& entry = it->second;
  entry.moving_now = true;
  entry.last_frame = (std::max)(entry.last_frame, frame);
  entry.moving_sightings += 1u;
  if (indirect) entry.indirect_sightings += 1u;
  float translation[3] = {};
  float prev_translation[3] = {};
  PoolMatrixTranslation(world, translation);
  PoolMatrixTranslation(prev_world, prev_translation);
  const float dx = translation[0] - prev_translation[0];
  const float dy = translation[1] - prev_translation[1];
  const float dz = translation[2] - prev_translation[2];
  const float meters = std::sqrt(dx * dx + dy * dy + dz * dz);
  float basis = 0.f;
  for (uint32_t i = 0; i < kPoolWorldFloats; ++i) {
    if (i == 3u || i == 7u || i == 11u) continue;
    basis = (std::max)(basis, std::fabs(world[i] - prev_world[i]));
  }
  if (entry.moving_sightings == 1u || meters > entry.max_meters) {
    std::memcpy(entry.world, world, sizeof(entry.world));
    std::memcpy(entry.prev_world, prev_world, sizeof(entry.prev_world));
  }
  entry.max_meters = (std::max)(entry.max_meters, meters);
  entry.max_basis = (std::max)(entry.max_basis, basis);
  ApplyPoolDynamicKey(mesh_key, entry);
}

// The draw one sighting came from (motion samples).
struct PoolMotionDraw {
  uint8_t pass = 0u;
  bool indirect = false;
  uint32_t instances = 0u;  // elements read back
  uint32_t moved = 0u;      // of them counted as moved
};

// Caller holds g_pool.mutex. Before ObservePoolInstance of the same sighting
// (whether its (mesh, world) was seen in an earlier frame is read here).
inline PoolSightingVerdict NotePoolMotion(PoolFamilyStats& family, uint64_t mesh_key, uint32_t vs_hash,
                                          const float* world, const float* prev_world, const PoolMotion& motion,
                                          uint32_t frame, const PoolMotionDraw& draw, uint32_t element) {
  PoolMotionStats& stats = g_pool.stats.motion;
  const auto observed = g_pool.observations.find(InstanceKey{mesh_key, MatrixHash(world, kPoolWorldFloats)});
  const bool known = observed != g_pool.observations.end() && observed->second.count != 0u;
  const bool repeat = known && observed->second.first_frame < frame;  // first seen in an earlier frame
  const bool camera = (draw.pass & kPoolPassCamera) != 0u;
  const bool prev_differs = camera && PoolSightingMoves(world, prev_world);
  const bool stale = prev_differs && repeat;
  PoolSightingVerdict verdict = PoolSightingVerdict::Unknown;
  if (prev_differs && !repeat) {
    verdict = PoolSightingVerdict::Moving;
  } else if (camera && motion.finite && PoolPrevWorldFilled(prev_world)) {  // unchanged, or stale prev (repeat)
    verdict = PoolSightingVerdict::Still;
  }
  const size_t ulp_bucket = PoolMotionUlpBucket(motion.ulps);
  const size_t meter_bucket = PoolMotionMeterBucket(motion.meters);
  if (motion.exact) stats.exact += 1u;
  if (!motion.finite) stats.nonfinite += 1u;
  const bool zero_prev = !PoolPrevWorldFilled(prev_world);
  if (camera) {
    if (repeat) {
      stats.camera_repeat += 1u;
      if (motion.exact) stats.camera_repeat_exact += 1u;
    }
    if (zero_prev) stats.camera_zero_prev += 1u;
    if (prev_differs) {
      stats.rule_moving += 1u;
      if (stale) stats.rule_stale += 1u;
    }
  } else if ((draw.pass & kPoolPassLight) != 0u) {
    if (repeat) stats.light_repeat += 1u;
    if (zero_prev) stats.light_zero_prev += 1u;
  }
  if (repeat) {
    stats.repeat += 1u;
    family.motion_repeat += 1u;
    stats.repeat_ulps[ulp_bucket] += 1u;
    stats.repeat_meters[meter_bucket] += 1u;
    stats.max_repeat_ulps = (std::max)(stats.max_repeat_ulps, motion.ulps);
    family.motion_max_repeat_ulps = (std::max)(family.motion_max_repeat_ulps, motion.ulps);
    if (motion.finite) stats.max_repeat_meters = (std::max)(stats.max_repeat_meters, motion.meters);
  } else {
    stats.first += 1u;
    family.motion_first += 1u;
    stats.first_ulps[ulp_bucket] += 1u;
    stats.first_meters[meter_bucket] += 1u;
  }
  if (motion.moved) {
    stats.moved += 1u;
    family.motion_moved += 1u;
    if (repeat) stats.moved_repeat += 1u;
    if (draw.indirect) {
      stats.moved_indirect += 1u;
      family.motion_moved_indirect += 1u;
    }
    if ((draw.pass & kPoolPassCamera) != 0u) {
      stats.moved_camera += 1u;
    } else if ((draw.pass & kPoolPassLight) != 0u) {
      stats.moved_light += 1u;
    }
    family.motion_max_moved_meters = (std::max)(family.motion_max_moved_meters, motion.meters);
    PoolMotionDetail& detail = g_pool.motion_detail;
    if (detail.moved_mesh_keys.size() < kPoolMotionMaxMeshKeys) {
      if (detail.moved_mesh_keys.insert(mesh_key).second) stats.moved_meshes += 1u;
    } else if (detail.moved_mesh_keys.count(mesh_key) == 0u) {
      detail.moved_mesh_keys_full = true;
    }
  }
  uint32_t no_family_cap = 0u;
  std::vector<PoolMotionSample>* samples = nullptr;
  uint32_t* family_samples = nullptr;
  if (stale && g_pool.motion_detail.rule_stale_samples.size() < kPoolMotionRuleSamples) {
    samples = &g_pool.motion_detail.rule_stale_samples;  // every case until the cap
    family_samples = &no_family_cap;
  } else if (motion.moved && !repeat) {
    samples = &g_pool.motion_detail.moved_samples;
    family_samples = &family.motion_moved_samples;
  } else if (repeat && motion.ulps > kPoolMotionUlpNoise) {
    samples = &g_pool.motion_detail.repeat_samples;
    family_samples = &family.motion_repeat_samples;
  }
  if (samples == nullptr || samples->size() >= kPoolMotionMaxSamples
      || *family_samples >= kPoolMotionSamplesPerFamily) {
    return verdict;
  }
  *family_samples += 1u;
  PoolMotionSample& sample = samples->emplace_back();
  sample.vs_hash = vs_hash;
  sample.mesh_key = mesh_key;
  sample.frame = frame;
  sample.pass = draw.pass;
  sample.indirect = draw.indirect;
  sample.draw_instances = draw.instances;
  sample.element = element;
  sample.draw_moved = draw.moved;
  sample.repeat = repeat;
  sample.observed_frames = known ? observed->second.count : 0u;
  sample.admitted = known && observed->second.admitted;
  sample.ulps = motion.ulps;
  sample.meters = motion.meters;
  sample.basis = motion.basis;
  std::memcpy(sample.world, world, sizeof(float) * kPoolWorldFloats);
  std::memcpy(sample.prev_world, prev_world, sizeof(float) * kPoolWorldFloats);
  return verdict;
}

// What decides how the game camera sees an admitted instance (see
// shader_contract.hpp): whether any camera vertex shader drew it (an instance
// only ever drawn through light views is a shadow-only caster for the game),
// whether a light view drew it (a shadow caster), its fade inputs (the latest
// from a camera draw, else from a light draw, else those at admission) and
// whether the near fade applies (most of its camera draws used a pixel shader
// that near-fades). Sightings through another draw key of the same object
// are forwarded at observation (ObservePoolInstance). Caller holds g_pool.mutex.
struct PoolCameraVisibility {
  PoolVisibility inputs;
  const char* source = "none";  // where `inputs` came from: "camera draw", "light draw", "admission", "none"
  bool camera_seen = false;     // drawn by a camera VS
  bool light_seen = false;      // drawn by a light VS
  bool near_fade = false;       // most camera draws near-fade it
  uint32_t camera_ps_hash = 0u; // latest camera-draw pixel shader
  bool observed = false;        // its observation still exists
  uint32_t camera_frames = 0u;  // of the observation
  uint32_t light_frames = 0u;
  uint32_t camera_sightings = 0u;
  uint32_t camera_near_fade_sightings = 0u;
};

inline PoolCameraVisibility GetPoolCameraVisibility(const WorldInstance& instance) {
  PoolCameraVisibility result;
  const auto it = g_pool.observations.find(InstanceKey{instance.mesh_key, MatrixHash(instance.matrix, kPoolWorldFloats)});
  if (it != g_pool.observations.end()) {
    const ObservedInstance& observed = it->second;
    result.observed = true;
    result.camera_frames = observed.camera_frames;
    result.light_frames = observed.light_frames;
    result.camera_sightings = observed.camera_sightings;
    result.camera_near_fade_sightings = observed.camera_near_fade_sightings;
    result.camera_seen = observed.camera_sightings != 0u;
    result.light_seen = observed.light_frames != 0u;
    result.near_fade = PoolMostlyNearFade(observed.camera_near_fade_sightings, observed.camera_sightings);
    result.camera_ps_hash = observed.camera_ps_hash;
    if (observed.camera_visibility.valid) {
      result.inputs = observed.camera_visibility;
      result.source = "camera draw";
    } else if (observed.light_visibility.valid) {
      result.inputs = observed.light_visibility;
      result.source = "light draw";
    }
  }
  if (!result.inputs.valid && instance.visibility.valid) {
    result.inputs = instance.visibility;
    result.source = "admission";
  }
  return result;
}

// ---------------------------------------------------------------------------
// Draw-time scan.

struct PoolDrawGate {
  contract::VsClass vs_class = contract::VsClass::Unclassified;
  PoolSkip skip = PoolSkip::None;
  PoolDrawState state = PoolDrawState::Ok;
  uint8_t pass = 0u;  // kPoolPass* bits, set when the draw passes the gate
};

// Shader classes first, then draw state, then the pixel shader.
inline PoolDrawGate GatePoolDraw(const DrawRecord& draw, bool check_index_count) {
  PoolDrawGate gate;
  const contract::ShaderTraits vs_traits = contract::LookupVertexTraits(draw.vs_pipeline);
  gate.vs_class = static_cast<contract::VsClass>(vs_traits.cls);
  if (gate.vs_class != contract::VsClass::Rigid) {
    gate.skip = PoolSkip::NotRigid;
    return gate;
  }
  gate.state = CheckPoolDrawState(draw, check_index_count);
  if (gate.state != PoolDrawState::Ok) {
    gate.skip = PoolSkip::DrawState;
    return gate;
  }
  const contract::ShaderTraits ps_traits = contract::LookupPixelTraits(draw.ps_pipeline);
  const auto ps_class = static_cast<contract::PsClass>(ps_traits.cls);
  if (ps_class == contract::PsClass::AlphaTested) {
    gate.skip = PoolSkip::AlphaTested;
  } else if (ps_class != contract::PsClass::Opaque) {
    gate.skip = PoolSkip::PixelUnknown;
  }
  if (gate.skip == PoolSkip::None) {
    gate.pass = static_cast<uint8_t>(((vs_traits.flags & contract::kTraitCameraView) != 0u ? kPoolPassCamera : 0u)
                                     | ((vs_traits.flags & contract::kTraitLightView) != 0u ? kPoolPassLight : 0u)
                                     | ((vs_traits.flags & contract::kTraitVisibilityLayout) != 0u ? kPoolPassVisibility : 0u)
                                     | ((ps_traits.flags & contract::kTraitNearFade) != 0u ? kPoolPassNearFadePs : 0u));
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
    PoolSchedule** out_schedule,
    bool bypass_cooldown) {
  PoolSchedule& schedule = g_pool.schedule[schedule_key];
  const PoolStagingSlot& slot = g_pool.slots[g_pool.write_slot];
  if (!bypass_cooldown && schedule.next_frame != 0u && schedule.copy_frame != frame && frame < schedule.next_frame) {
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
inline uint32_t PoolBloomBit(uint64_t handle) {
  return static_cast<uint32_t>(((handle >> 4u) * 0x9E3779B97F4A7C15ull) >> 48u);  // 16 bits
}

inline bool PoolBloomTest(uint64_t handle) {
  const uint32_t bit = PoolBloomBit(handle);
  return (g_pool.tracked_bloom[bit >> 6u].load(std::memory_order_relaxed) & (1ull << (bit & 63u))) != 0u;
}

inline void AddPoolResourceKey(uint64_t handle, uint64_t mesh_key) {
  const uint32_t bit = PoolBloomBit(handle);
  g_pool.tracked_bloom[bit >> 6u].fetch_or(1ull << (bit & 63u), std::memory_order_relaxed);
  auto& keys = g_pool.keys_by_resource[handle];
  if (std::find(keys.begin(), keys.end(), mesh_key) == keys.end()) keys.push_back(mesh_key);
}

inline void QueuePoolMesh(uint64_t mesh_key, uint32_t vs_hash, const DrawRecord& draw, bool from_indirect) {
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
  request.from_indirect = from_indirect;
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
    bool indirect_draw,
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
      request.indices_served_indirect = indirect_draw;
      request.indices_source_offset = begin;
      request.indices_frame = frame;
    } else {
      g_pool.stats.mesh_vertex_copies += 1u;
      request.vertices_served_indirect = indirect_draw;
      request.vertices_source_offset = begin;
      request.vertices_frame = frame;
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
  bool trace_declined = false;
  {
    std::lock_guard<std::mutex> lock(g_pool.mutex);
    g_pool.stats.draws_by_vs_class[static_cast<size_t>(gate.vs_class)] += 1u;
    if (deferred) g_pool.stats.draws_on_deferred += 1u;
    PoolFamilyStats& family = g_pool.families[draw.vs_hash];
    family.vs_class = static_cast<uint8_t>(gate.vs_class);
    family.draws += 1u;

    if (skip == PoolSkip::None) {
      const uint64_t schedule_key = PoolScheduleKey(mesh_key, draw.first_instance, count);
      const uint64_t copy_bytes = kPoolCbCopyBytes + static_cast<uint64_t>(count) * kPoolInstanceStride;
      bool traced = false;  // a key in trace state: its copy may bypass the cooldown
      if ((gate.pass & kPoolPassCamera) != 0u) {
        for (const auto& key : g_pool.prev_trace.keys) {
          if (key.state == PoolPrevTrace::KeyState::Active && key.mesh_key == mesh_key) traced = true;
        }
      }
      PoolSchedule* schedule = nullptr;
      skip = ReservePoolCopy(device, schedule_key, frame, copy_bytes, &schedule, false);
      const bool trace_only = traced && skip == PoolSkip::Cooldown;
      uint32_t copy_count = count;
      if (trace_only) {
        copy_count = (std::min)(count, kPoolTraceMaxElements);
        skip = ReservePoolCopy(device, schedule_key, frame, kPoolCbCopyBytes + static_cast<uint64_t>(copy_count) * kPoolInstanceStride,
                               &schedule, true);
        trace_declined = skip != PoolSkip::None;  // not a normal skip: not counted
      }
      if (skip == PoolSkip::None) {
        const uint64_t slice_bytes = static_cast<uint64_t>(copy_count) * kPoolInstanceStride;
        PoolStagingSlot& slot = g_pool.slots[g_pool.write_slot];
        PoolPendingCopy copy;
        copy.cb_offset = slot.used;
        copy.slice_offset = slot.used + kPoolCbCopyBytes;
        copy.count = copy_count;
        copy.cpu_base = slice.base;
        copy.frame = frame;
        copy.vs_hash = draw.vs_hash;
        copy.ps_hash = draw.ps_hash;
        copy.pass = gate.pass;
        copy.mesh_key = mesh_key;
        copy.trace_only = trace_only;
        // The b1 copy is recorded before this draw, so it holds exactly the
        // offset the draw reads; resolve compares it to `base`.
        commands[command_count++] = {slice.instance_cb, 0u, slot.buffer, copy.cb_offset, slice.cb_bytes};
        commands[command_count++] = {slice.instance_buffer, static_cast<uint64_t>(slice.base) * kPoolInstanceStride,
                                     slot.buffer, copy.slice_offset, slice_bytes};
        slot.used += kPoolCbCopyBytes + slice_bytes;
        slot.copies.push_back(copy);
        if (!trace_only) {
          schedule->copy_frame = frame;
          schedule->next_frame = frame + kPoolRecaptureFrames;
          g_pool.stats.copied_draws += 1u;
          family.copied += 1u;
          QueuePoolMesh(mesh_key, draw.vs_hash, draw, false);
        }
      }
    }
    if (skip != PoolSkip::None && !trace_declined) CountPoolSkip(family, skip, gate.state);
    mesh_count = ReservePoolMeshCopies(device, draw, false, deferred, frame, commands.data() + command_count,
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
      const bool learned = known != g_pool.schedule.end() && known->second.indirect_window != 0u;
      if (learned) window = (std::min)(known->second.indirect_window, max_window);
      const uint64_t bytes =
          kPoolCbCopyBytes + kPoolIndirectArgsBytes + static_cast<uint64_t>(window) * kPoolInstanceStride;
      PoolSchedule* schedule = nullptr;
      if (sub_skip == PoolSkip::None) {
        sub_skip = ReservePoolCopy(device, schedule_key, frame, bytes, &schedule, false);
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
      copy.ps_hash = draw.ps_hash;
      copy.pass = gate.pass;
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
      if (!learned) g_pool.stats.indirect_first_copies += 1u;
      family.copied += 1u;
    }
    mesh_count = ReservePoolMeshCopies(device, draw, true, deferred, frame, commands.data() + command_count,
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

// A game write to a VB/IB that a captured or queued mesh comes from
// (diagnostic only): the mesh may no longer match the buffer's contents.
// Other buffers cost a bloom-filter test and no lock. Called from the game's
// own calls; the pool never makes graphics calls under its lock, so this
// cannot re-enter it.
inline void NotePoolBufferWrite(uint64_t handle) {
  if (handle == 0u || !PoolBloomTest(handle)) return;
  std::lock_guard<std::mutex> lock(g_pool.mutex);
  const auto it = g_pool.keys_by_resource.find(handle);
  if (it == g_pool.keys_by_resource.end()) return;
  g_pool.stats.tracked_buffer_writes += 1u;
  for (const uint64_t mesh_key : it->second) {
    const auto mesh = g_pool.mesh_by_key.find(mesh_key);
    if (mesh != g_pool.mesh_by_key.end() && mesh->second < g_pool.meshes.size()) {
      uint32_t& writes = g_pool.meshes[mesh->second].writes_after_capture;
      if (writes != UINT32_MAX) writes += 1u;
    }
    const auto request = g_pool.mesh_requests.find(mesh_key);
    if (request != g_pool.mesh_requests.end() && !request->second.written) {
      request->second.written = true;
      g_pool.stats.requests_written += 1u;
    }
  }
}

inline bool OnUpdateBufferRegionPool(
    reshade::api::device*, const void*, reshade::api::resource dest, uint64_t, uint64_t) {
  NotePoolBufferWrite(dest.handle);
  return false;
}

inline bool OnUpdateBufferRegionCommandPool(
    reshade::api::command_list*, const void*, reshade::api::resource dest, uint64_t, uint64_t) {
  NotePoolBufferWrite(dest.handle);
  return false;
}

inline void OnMapBufferRegionPool(
    reshade::api::device*, reshade::api::resource resource, uint64_t, uint64_t, reshade::api::map_access access, void**) {
  if (access != reshade::api::map_access::read_only) NotePoolBufferWrite(resource.handle);
}

inline bool OnCopyBufferRegionPool(
    reshade::api::command_list*, reshade::api::resource, uint64_t, reshade::api::resource dest, uint64_t, uint64_t) {
  NotePoolBufferWrite(dest.handle);
  return false;
}

inline bool OnCopyResourcePool(reshade::api::command_list*, reshade::api::resource, reshade::api::resource dest) {
  NotePoolBufferWrite(dest.handle);
  return false;
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
  uint32_t retired_now = 0u;
  for (auto& mesh : g_pool.meshes) {
    if (mesh.live_keys == 0u) {
      g_pool.stats.meshes_retired += 1u;
      retired_now += 1u;
      continue;
    }
    remap[mesh.mesh_id] = static_cast<uint32_t>(meshes.size());
    mesh.mesh_id = static_cast<uint32_t>(meshes.size());
    meshes.push_back(std::move(mesh));
  }
  g_pool.meshes = std::move(meshes);
  if (retired_now >= kPoolMassRetireMeshes) {
    g_pool.stats.mass_retirements += 1u;
    g_pool.stats.last_mass_retire_frame = g_state.frame.load();
    g_pool.stats.last_mass_retire_meshes = retired_now;
  }

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

  for (const uint64_t key : invalidated) g_pool.dynamic_keys.erase(key);
  if (g_pool.dynamic_applied) {
    // A mesh stays flagged only while a moving key still maps to it (mesh_by_key is already remapped).
    std::vector<bool> held(g_pool.meshes.size(), false);
    for (const auto& [key, entry] : g_pool.dynamic_keys) {
      const auto mesh_it = g_pool.mesh_by_key.find(key);
      if (entry.moving_now && mesh_it != g_pool.mesh_by_key.end() && mesh_it->second < held.size()) held[mesh_it->second] = true;
    }
    for (WorldMesh& mesh : g_pool.meshes) {
      if (mesh.dynamic && !held[mesh.mesh_id]) mesh.dynamic = false;
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
inline const char* ApplyPoolMesh(
    uint64_t mesh_key, uint32_t vs_hash, PoolDecodedMesh& mesh, const PoolMeshRequest* origin = nullptr) {
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
    world_mesh.uid = g_pool.next_mesh_uid++;
    world_mesh.mesh_id = static_cast<uint32_t>(g_pool.meshes.size());
    world_mesh.source_vs_hash = vs_hash;
    world_mesh.signature = signature;
    world_mesh.triangle_count = static_cast<uint32_t>(mesh.triangles.size());
    world_mesh.capture_frame = g_state.frame.load();
    if (origin != nullptr) {
      world_mesh.source_vb = origin->draw.vb.handle;
      world_mesh.source_ib = origin->draw.ib.handle;
      world_mesh.from_indirect = origin->from_indirect;
      world_mesh.writes_after_capture = origin->written ? 1u : 0u;
      world_mesh.captures = origin->captures;
      world_mesh.verified = origin->verified;
      world_mesh.source_vb_usage = origin->draw.vb_usage;
      world_mesh.source_vb_flags = origin->draw.vb_flags;
      world_mesh.source_ib_usage = origin->draw.ib_usage;
      world_mesh.source_ib_flags = origin->draw.ib_flags;
    }
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
  const auto dynamic = g_pool.dynamic_keys.find(mesh_key);
  if (dynamic != g_pool.dynamic_keys.end()) ApplyPoolDynamicKey(mesh_key, dynamic->second);  // seen moving before its capture
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
                        ", capture meshes ", on(g_pool.capture_meshes), ", scan indirect draws ", on(g_pool.scan_indirect),
                        ", verify mesh captures ", on(g_pool.verify_meshes), ", legacy scale limits ",
                        on(g_pool.legacy_scale), ", keep moving objects out ", on(g_pool.exclude_moving));
}

// Short text for how a VB/IB was created (reshade::api resource_usage /
// resource_flags bits): heap kind plus the GPU-writable bind flags.
inline std::string PoolBufferText(uint32_t usage, uint32_t flags) {
  using reshade::api::resource_flags;
  using reshade::api::resource_usage;
  std::string text = (flags & static_cast<uint32_t>(resource_flags::immutable)) != 0u ? "immutable"
                     : (flags & static_cast<uint32_t>(resource_flags::dynamic)) != 0u ? "dynamic"
                                                                                       : "default";
  if ((usage & static_cast<uint32_t>(resource_usage::unordered_access)) != 0u) text += "+uav";
  if ((usage & static_cast<uint32_t>(resource_usage::stream_output)) != 0u) text += "+stream_out";
  if ((usage & static_cast<uint32_t>(resource_usage::shader_resource)) != 0u) text += "+srv";
  return text;
}

// One line for ReShade.log: where two captures of a mesh differ.
inline std::string DescribePoolMeshMismatch(
    const PoolMeshRequest& request, const PoolDecodedMesh& current, uint64_t index_signature, uint32_t frame) {
  namespace log_utils = renodx::utils::log;
  const PoolDecodedMesh& previous = request.previous;
  size_t first = SIZE_MAX;
  const size_t triangles = (std::min)(previous.triangles.size(), current.triangles.size());
  for (size_t t = 0; t < triangles && first == SIZE_MAX; ++t) {
    for (int k = 0; k < 3; ++k) {
      if (previous.positions[previous.triangles[t][k]] != current.positions[current.triangles[t][k]]) {
        first = t;
        break;
      }
    }
  }
  size_t differing = 0u;
  for (size_t t = 0; t < triangles; ++t) {
    for (int k = 0; k < 3; ++k) {
      if (previous.positions[previous.triangles[t][k]] != current.positions[current.triangles[t][k]]) {
        differing += 1u;
        break;
      }
    }
  }
  const DrawRecord& draw = request.draw;
  std::string line = log_utils::BuildString(
      "falcom_world::pool: mesh ", log_utils::AsHex(request.mesh_key), " vs ", log_utils::AsHex(request.vs_hash),
      " capture ", request.captures, " differs from the previous one (frames ", request.previous_frame, " -> ", frame,
      ") | triangles ", previous.triangles.size(), " -> ", current.triangles.size(), ", vertices ",
      previous.positions.size(), " -> ", current.positions.size(), ", differing triangles ", differing,
      ", first at ", first == SIZE_MAX ? -1 : static_cast<int64_t>(first), ", indices ",
      request.previous_index_signature == index_signature ? "same" : "differ", ", vertex range ",
      request.previous_min_vertex, "..", request.previous_max_vertex, " -> ", request.min_vertex, "..",
      request.max_vertex, " | queued by ", request.from_indirect ? "indirect" : "direct", " draw; indices copied at frame ",
      request.indices_frame, " by ", request.indices_served_indirect ? "indirect" : "direct", " draw from offset ",
      request.indices_source_offset, " (mod 16: ", request.indices_source_offset % 16u, "), vertices at frame ",
      request.vertices_frame, " by ", request.vertices_served_indirect ? "indirect" : "direct", " draw from offset ",
      request.vertices_source_offset, " (mod 16: ", request.vertices_source_offset % 16u, ")",
      " | vb ", log_utils::AsPtr(draw.vb.handle), " ", PoolBufferText(draw.vb_usage, draw.vb_flags), " stride ",
      draw.vb_stride, " offset ", draw.vb_offset, " | ib ", log_utils::AsPtr(draw.ib.handle), " ",
      PoolBufferText(draw.ib_usage, draw.ib_flags), " first index ", draw.first_index, " count ", draw.index_count,
      " base vertex ", draw.vertex_offset);
  if (first != SIZE_MAX) {
    const auto& a = previous.triangles[first];
    const auto& b = current.triangles[first];
    line += log_utils::BuildString(
        " | triangle ", first, " was (", previous.positions[a[0]][0], ", ", previous.positions[a[0]][1], ", ",
        previous.positions[a[0]][2], ") (", previous.positions[a[1]][0], ", ", previous.positions[a[1]][1], ", ",
        previous.positions[a[1]][2], ") (", previous.positions[a[2]][0], ", ", previous.positions[a[2]][1], ", ",
        previous.positions[a[2]][2], "), now (", current.positions[b[0]][0], ", ", current.positions[b[0]][1], ", ",
        current.positions[b[0]][2], ") (", current.positions[b[1]][0], ", ", current.positions[b[1]][1], ", ",
        current.positions[b[1]][2], ") (", current.positions[b[2]][0], ", ", current.positions[b[2]][1], ", ",
        current.positions[b[2]][2], ")");
  }
  return line;
}

// Content hash of a capture's absolute indices (mismatch diagnostics).
inline uint64_t PoolIndexSignature(const std::vector<uint32_t>& indices) {
  uint64_t hash = 1469598103934665603ull;
  for (const uint32_t index : indices) hash = PoolMix(hash, index);
  return PoolMix(hash, indices.size());
}

// Caller holds g_pool.mutex. A complete capture of a queued mesh: applied
// when verification is off or it matches the previous capture; otherwise it
// becomes the previous capture and the mesh is captured again, up to
// kPoolMeshMaxCaptures. Returns the outcome for the log.
inline const char* ApplyOrVerifyPoolMesh(
    std::unordered_map<uint64_t, PoolMeshRequest>::iterator request,
    PoolDecodedMesh& mesh,
    uint64_t index_signature,
    uint32_t frame,
    std::vector<std::string>* warning_lines) {
  PoolMeshRequest& queued = request->second;
  queued.captures += 1u;
  const uint64_t signature = PoolMeshSignature(mesh);
  const bool verify = g_pool.verify_meshes.load(std::memory_order_relaxed);
  if (!verify || (queued.has_previous && queued.previous_signature == signature)) {
    if (verify) {
      g_pool.stats.mesh_verified += 1u;
      queued.verified = true;
    }
    const char* outcome = ApplyPoolMesh(queued.mesh_key, queued.vs_hash, mesh, &queued);
    g_pool.mesh_requests.erase(request);
    return outcome;
  }
  const bool mismatch = queued.has_previous;
  if (mismatch) {
    g_pool.stats.mesh_capture_mismatches += 1u;
    g_pool.families[queued.vs_hash].mesh_mismatches += 1u;
    if (g_pool.mismatch_lines_logged < kPoolMismatchLogLines && warning_lines != nullptr) {
      g_pool.mismatch_lines_logged += 1u;
      warning_lines->push_back(DescribePoolMeshMismatch(queued, mesh, index_signature, frame));
    }
  }
  if (queued.captures >= kPoolMeshMaxCaptures) {
    g_pool.stats.mesh_unstable += 1u;
    g_pool.families[queued.vs_hash].mesh_unstable += 1u;
    FailPoolMesh(request, "unstable: no two captures in a row decode to the same mesh");
    return "unstable: no two captures in a row decode to the same mesh";
  }
  queued.previous = std::move(mesh);
  queued.previous_signature = signature;
  queued.previous_index_signature = index_signature;
  queued.previous_frame = frame;
  queued.previous_min_vertex = queued.min_vertex;
  queued.previous_max_vertex = queued.max_vertex;
  queued.has_previous = true;
  queued.phase = PoolMeshPhase::Indices;
  queued.indices.clear();
  queued.in_flight = false;
  queued.last_frame = frame;
  AddPoolMeshWaiting(queued);
  return mismatch ? "differs from the previous capture, capturing again" : "captured, verifying with a second capture";
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
  std::vector<float> worlds;       // kPoolWorldFloats per instance of verified copies
  std::vector<float> prev_worlds;  // their prevWorld, like worlds (motion probe)
  std::vector<uint8_t> moving;
  std::vector<PoolVisibility> visibility;  // per instance, like worlds
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
        prev_worlds.resize(offset + kPoolWorldFloats);
        std::memcpy(prev_worlds.data() + offset, instance + kPoolPrevWorldOffset, sizeof(float) * kPoolWorldFloats);
        moving.push_back(std::memcmp(instance, instance + kPoolPrevWorldOffset, sizeof(float) * kPoolWorldFloats) != 0
                             ? 1u
                             : 0u);
        PoolVisibility& inputs = visibility.emplace_back();
        if ((copy.pass & kPoolPassVisibility) != 0u) {
          inputs.valid = true;
          std::memcpy(inputs.color, instance + contract::kColorOffset, sizeof(inputs.color));
          std::memcpy(inputs.param, instance + contract::kParamOffset, sizeof(inputs.param));
        }
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
  const PoolCameraInfo camera = GetPoolCameraInfo();  // takes g_state.mutex: before the pool lock
  const bool log = g_pool.log_captures.load(std::memory_order_relaxed);
  const uint32_t frame = g_state.frame.load();
  std::vector<std::string> log_lines;
  std::vector<std::string> warning_lines;
  {
    std::lock_guard<std::mutex> lock(g_pool.mutex);
    PoolStagingSlot& slot = g_pool.slots[slot_index];
    slot.resolving = false;
    slot.used = 0u;
    slot.mesh_used = 0u;

    g_pool.resolve_camera_valid = camera.valid;
    std::memcpy(g_pool.resolve_camera, camera.position, sizeof(g_pool.resolve_camera));

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
        outcome = ApplyOrVerifyPoolMesh(request, job.mesh, PoolIndexSignature(job.indices), frame, &warning_lines);
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
    std::vector<PoolMatrixReject> rejects;  // per element of one copy
    std::vector<PoolMotion> motions;
    for (size_t i = 0; i < copies.size(); ++i) {
      const PoolPendingCopy& copy = copies[i];
      const PoolResolvedCopy& result = resolved[i];
      PoolFamilyStats& family = g_pool.families[copy.vs_hash];
      const bool indirect = is_indirect(copy);
      const bool buffers_dead = indirect && ReleasePoolIndirectRefs(indirect_draws[copy.indirect_index]);
      if (!result.verified) {
        if (!copy.trace_only) {
          g_pool.stats.base_mismatch += 1u;
          g_pool.stats.last_mismatch_cpu = copy.cpu_base;
          g_pool.stats.last_mismatch_gpu = result.gpu_base;
          family.base_mismatch += 1u;
        }
        continue;
      }
      if (!indirect && (copy.pass & kPoolPassCamera) != 0u) {
        // prevWorld trace: one record per element (first kPoolTraceMaxElements)
        // and frame of each traced key; a second camera copy in a frame only counts.
        for (size_t k = 0; k < g_pool.prev_trace.keys.size(); ++k) {
          PoolPrevTrace::Key& key = g_pool.prev_trace.keys[k];
          if (key.state != PoolPrevTrace::KeyState::Active || key.mesh_key != copy.mesh_key) continue;
          if (key.frames_recorded != 0u && copy.frame == key.last_frame) {
            key.extra_camera_draws += 1u;
            break;
          }
          if (key.frames_recorded != 0u && copy.frame > key.last_frame + 1u) {
            key.missed += copy.frame - key.last_frame - 1u;
          }
          const uint32_t recorded = (std::min)(result.count, kPoolTraceMaxElements);
          for (uint32_t element = 0; element < recorded; ++element) {
            if (g_pool.prev_trace.records.size() >= kPoolTraceMaxRecords) break;
            const size_t index = result.first_world + element;
            const float* world = worlds.data() + index * kPoolWorldFloats;
            const float* prev = prev_worlds.data() + index * kPoolWorldFloats;
            PoolPrevTrace::Record record;
            record.key = static_cast<uint8_t>(k);
            record.frame = copy.frame;
            record.element = element;
            record.draw_instances = copy.count;
            record.camera_draws = 1u;
            record.cpu_base = copy.cpu_base;
            record.trace_only = copy.trace_only;
            std::memcpy(record.world, world, sizeof(record.world));
            std::memcpy(record.prev_world, prev, sizeof(record.prev_world));
            record.prev_filled = PoolPrevWorldFilled(prev);
            record.prev_vs_world = MeasurePoolMotion(world, prev);
            if (element == 0u && key.last_count != 0u && key.last_frame + 1u == copy.frame) {
              record.has_last = true;
              record.prev_vs_last = MeasurePoolMotion(key.last_world, prev);
              record.world_step_meters = MeasurePoolMotion(key.last_world, world).meters;
            }
            g_pool.prev_trace.records.push_back(record);
          }
          if (result.count != 0u) {
            std::memcpy(key.last_world, worlds.data() + result.first_world * kPoolWorldFloats, sizeof(key.last_world));
          }
          key.last_count = result.count;
          key.last_frame = copy.frame;
          key.frames_recorded += 1u;
          if (key.frames_recorded == kPoolTraceFrames) key.state = PoolPrevTrace::KeyState::Done;
          break;
        }
      }
      if (copy.trace_only) continue;
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
        QueuePoolMesh(mesh_key, copy.vs_hash, record, true);
      }

      const PoolSighting sighting{copy.pass, copy.ps_hash};
      NotePoolMeshPass(mesh_key, sighting);
      // Motion probe: measure the copy's plausible elements first, so each
      // sample knows how many elements of its draw moved.
      rejects.assign(result.count, PoolMatrixReject::None);
      motions.assign(result.count, PoolMotion{});
      PoolMotionDraw motion_draw;
      motion_draw.pass = copy.pass;
      motion_draw.indirect = indirect;
      for (uint32_t element = 0; element < result.count; ++element) {
        const size_t index = result.first_world + element;
        rejects[element] = CheckPoolWorld(worlds.data() + index * kPoolWorldFloats);
        if (rejects[element] != PoolMatrixReject::None) continue;
        motions[element] = MeasurePoolMotion(worlds.data() + index * kPoolWorldFloats,
                                             prev_worlds.data() + index * kPoolWorldFloats);
        motion_draw.instances += 1u;
        if (motions[element].moved) motion_draw.moved += 1u;
      }
      if (motion_draw.moved != 0u) {
        PoolMotionStats& motion_stats = g_pool.stats.motion;
        motion_stats.moving_draws += 1u;
        if (motion_draw.moved == motion_draw.instances) motion_stats.moving_draws_all += 1u;
        if (indirect) motion_stats.moving_draws_indirect += 1u;
        motion_stats.moving_draw_instances += motion_draw.instances;
      }
      for (uint32_t element = 0; element < result.count; ++element) {
        const size_t index = result.first_world + element;
        const float* world = worlds.data() + index * kPoolWorldFloats;
        g_pool.stats.instances_seen += 1u;
        family.instances_seen += 1u;
        if (moving[index] != 0u) g_pool.stats.moving_instances += 1u;
        const PoolMatrixReject reject = rejects[element];
        if (reject != PoolMatrixReject::None) {
          CountPoolMatrixReject(family, reject, world);
          continue;
        }
        const float* prev_world = prev_worlds.data() + index * kPoolWorldFloats;
        const PoolSightingVerdict verdict = NotePoolMotion(family, mesh_key, copy.vs_hash, world, prev_world,
                                                           motions[element], copy.frame, motion_draw, element);
        // Before the observation: a moving sighting flags its mesh first, so
        // this pose is not admitted.
        if (verdict != PoolSightingVerdict::Unknown) {
          NotePoolCameraMotion(mesh_key, copy.vs_hash, world, prev_world, indirect, copy.frame, verdict);
        }
        ObservePoolInstance(mesh_key, copy.vs_hash, world, copy.frame, &sighting, &visibility[index]);
      }
    }
  }
  for (const std::string& line : log_lines) renodx::utils::log::i(line);
  for (const std::string& line : warning_lines) renodx::utils::log::w(line);
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

inline void WritePoolPrevTrace(const PoolPrevTrace& trace);

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
    ApplyPoolDynamicSwitch();
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
  bool trace_done = false;
  PoolPrevTrace trace_out;
  {
    std::lock_guard<std::mutex> lock(g_pool.mutex);
    if ((frame % 60u) == 0u) PrunePool(frame);
    UpdatePoolStats();
    const uint32_t invalidations = g_pool.stats.resource_invalidations;
    released = invalidations >= g_pool.logged_invalidations ? invalidations - g_pool.logged_invalidations : invalidations;
    g_pool.logged_invalidations = invalidations;
    mesh_waiting = g_pool.stats.mesh_queue;

    // prevWorld trace: pick the keys while some are waiting (every 30 frames),
    // give up on the ones that waited or ran too long, and write the file once
    // no key is waiting or active.
    PoolPrevTrace& trace = g_pool.prev_trace;
    if (trace.armed_frame != 0u && !trace.written) {
      bool waiting = false;
      for (auto& key : trace.keys) {
        if (key.state == PoolPrevTrace::KeyState::Waiting && frame > trace.armed_frame + kPoolTraceWaitFrames) {
          key.state = PoolPrevTrace::KeyState::GaveUp;
        }
        if (key.state == PoolPrevTrace::KeyState::Active && key.frames_recorded < kPoolTraceFrames
            && frame > key.selected_frame + kPoolTraceActiveFrames) {
          key.state = PoolPrevTrace::KeyState::GaveUp;
        }
        if (key.state == PoolPrevTrace::KeyState::Waiting) waiting = true;
      }
      if (waiting && frame % 30u == 0u) {
        // A: a far flip. B: a slow sway (moving a few times, only small steps).
        // Both come from the moving draw keys; the families table decides
        // whether a key's draws are indirect.
        const PoolDynamicMesh* far_entry = nullptr;
        const PoolDynamicMesh* sway = nullptr;
        uint64_t far_key = 0u;
        uint64_t sway_key = 0u;
        float far_score = 0.f;
        for (const auto& [mesh_key, entry] : g_pool.dynamic_keys) {
          if (!entry.moving_now || entry.indirect_sightings != 0u) continue;
          const auto family = g_pool.families.find(entry.vs_hash);
          if (family != g_pool.families.end() && family->second.indirect_draws != 0u) continue;
          if (frame <= entry.last_frame + kPoolMovingHoldFrames
              && (entry.max_meters >= kPoolTraceFarMeters || entry.max_basis >= kPoolTraceFarBasis)) {
            const float score = (std::max)(entry.max_meters, entry.max_basis);
            if (far_entry == nullptr || score > far_score || (score == far_score && mesh_key < far_key)) {
              far_entry = &entry;
              far_key = mesh_key;
              far_score = score;
            }
          } else if (frame <= entry.last_frame + kPoolMovingHoldFrames && entry.moving_sightings >= 3u && entry.max_meters < kPoolTraceSwayMeters
                     && entry.max_basis < kPoolTraceSwayBasis) {
            if (sway == nullptr || entry.moving_sightings > sway->moving_sightings
                || (entry.moving_sightings == sway->moving_sightings && mesh_key < sway_key)) {
              sway = &entry;
              sway_key = mesh_key;
            }
          }
        }
        if (far_entry != nullptr && trace.keys[0].state == PoolPrevTrace::KeyState::Waiting) {
          PoolPrevTrace::Key& key = trace.keys[0];
          key.state = PoolPrevTrace::KeyState::Active;
          key.mesh_key = far_key;
          key.vs_hash = far_entry->vs_hash;
          key.selected_frame = frame;
          key.max_meters = far_entry->max_meters;
          key.max_basis = far_entry->max_basis;
          key.moving_sightings = far_entry->moving_sightings;
          key.camera_sightings = far_entry->moving_sightings;
        }
        if (sway != nullptr && trace.keys[1].state == PoolPrevTrace::KeyState::Waiting) {
          PoolPrevTrace::Key& key = trace.keys[1];
          key.state = PoolPrevTrace::KeyState::Active;
          key.mesh_key = sway_key;
          key.vs_hash = sway->vs_hash;
          key.selected_frame = frame;
          key.max_meters = sway->max_meters;
          key.max_basis = sway->max_basis;
          key.moving_sightings = sway->moving_sightings;
          key.camera_sightings = sway->moving_sightings;
        }
        // C: an admitted instance seen in a camera view recently, never moving.
        if (trace.keys[2].state == PoolPrevTrace::KeyState::Waiting) {
          const ObservedInstance* still = nullptr;
          uint64_t still_key = 0u;
          for (const auto& [instance_key, observed] : g_pool.observations) {
            if (!observed.admitted || observed.camera_sightings < kPoolTraceStillSightings) continue;
            if (frame > observed.last_camera_frame + 60u) continue;
            if (g_pool.dynamic_keys.count(instance_key.mesh_key) != 0u) continue;
            const auto family = g_pool.families.find(observed.vs_hash);
            if (family != g_pool.families.end() && family->second.indirect_draws != 0u) continue;
            if (still == nullptr || instance_key.mesh_key < still_key) {
              still = &observed;
              still_key = instance_key.mesh_key;
            }
          }
          if (still != nullptr) {
            PoolPrevTrace::Key& key = trace.keys[2];
            key.state = PoolPrevTrace::KeyState::Active;
            key.mesh_key = still_key;
            key.vs_hash = still->vs_hash;
            key.selected_frame = frame;
            key.camera_sightings = still->camera_sightings;
          }
        }
      }
      bool open = false;
      for (const auto& key : trace.keys) {
        if (key.state == PoolPrevTrace::KeyState::Waiting || key.state == PoolPrevTrace::KeyState::Active) open = true;
      }
      if (!open) {
        trace.written = true;
        trace_done = true;
        trace_out = trace;
      }
    }
  }
  if (trace_done) WritePoolPrevTrace(trace_out);
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
  g_pool.motion_detail = {};
  g_pool.dynamic_keys.clear();
  g_pool.dynamic_applied = false;  // re-applied at the next present
  g_pool.prev_trace = {};
  g_pool.logged_invalidations = 0u;
  g_pool.mismatch_lines_logged = 0u;
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

// A float for the dump: game-supplied values may be NaN or infinite, which
// JSON cannot hold.
struct PoolJsonFloat {
  float value = 0.f;
};

inline std::ostream& operator<<(std::ostream& out, PoolJsonFloat number) {
  if (std::isfinite(number.value)) return out << number.value;
  return out << "null";
}

// Motion probe section of the dump (see PoolMotionStats). Also lists the
// meshes seen moving with how many instances of them were admitted: an object
// that moves and stops is admitted at every place it stops (ghosts).
inline void WritePoolMotion(std::ostringstream& out, const PoolMotionStats& motion, const PoolMotionDetail& detail,
                            const std::vector<WorldInstance>& instances) {
  std::unordered_map<uint64_t, uint32_t> admitted_by_key;
  std::unordered_map<uint64_t, uint32_t> vs_by_key;
  uint32_t admitted_of_moved = 0u;
  for (const WorldInstance& instance : instances) {
    if (detail.moved_mesh_keys.count(instance.mesh_key) == 0u) continue;
    admitted_of_moved += 1u;
    admitted_by_key[instance.mesh_key] += 1u;
    vs_by_key[instance.mesh_key] = instance.source_vs_hash;
  }
  const auto write_histogram = [&out](const char* name, const auto& buckets, auto bucket_name) {
    out << "\"" << name << "\": {";
    for (size_t i = 0; i < buckets.size(); ++i) {
      if (i != 0u) out << ", ";
      out << "\"" << bucket_name(i) << "\": " << buckets[i];
    }
    out << "}";
  };
  out << "  \"motion\": {\"repeat\": " << motion.repeat << ", \"first\": " << motion.first
      << ", \"exact\": " << motion.exact << ", \"nonfinite_prev\": " << motion.nonfinite
      << ", \"max_repeat_ulps\": " << motion.max_repeat_ulps
      << ", \"max_repeat_meters\": " << PoolJsonFloat{motion.max_repeat_meters} << ",\n    ";
  write_histogram("repeat_ulps", motion.repeat_ulps, PoolMotionUlpBucketName);
  out << ",\n    ";
  write_histogram("first_ulps", motion.first_ulps, PoolMotionUlpBucketName);
  out << ",\n    ";
  write_histogram("repeat_meters", motion.repeat_meters, PoolMotionMeterBucketName);
  out << ",\n    ";
  write_histogram("first_meters", motion.first_meters, PoolMotionMeterBucketName);
  out << ",\n    \"moved\": " << motion.moved << ", \"moved_repeat\": " << motion.moved_repeat
      << ", \"moved_indirect\": " << motion.moved_indirect << ", \"moved_camera\": " << motion.moved_camera
      << ", \"moved_light\": " << motion.moved_light << ", \"moving_draws\": " << motion.moving_draws
      << ", \"moving_draws_all\": " << motion.moving_draws_all
      << ", \"moving_draws_indirect\": " << motion.moving_draws_indirect
      << ", \"moving_draw_instances\": " << motion.moving_draw_instances
      << ", \"moved_meshes\": " << motion.moved_meshes
      << ", \"moved_meshes_capped\": " << (detail.moved_mesh_keys_full ? "true" : "false")
      << ", \"camera_repeat\": " << motion.camera_repeat << ", \"camera_repeat_exact\": " << motion.camera_repeat_exact
      << ", \"camera_zero_prev\": " << motion.camera_zero_prev << ", \"light_repeat\": " << motion.light_repeat
      << ", \"light_zero_prev\": " << motion.light_zero_prev << ", \"rule_moving\": " << motion.rule_moving
      << ", \"rule_stale\": " << motion.rule_stale
      << ", \"admitted_of_moved_meshes\": " << admitted_of_moved << ",\n    \"moved_mesh_admitted\": [";
  std::vector<std::pair<uint64_t, uint32_t>> by_count(admitted_by_key.begin(), admitted_by_key.end());
  std::sort(by_count.begin(), by_count.end(), [](const auto& a, const auto& b) {
    return a.second != b.second ? a.second > b.second : a.first < b.first;
  });
  for (size_t i = 0; i < by_count.size() && i < kPoolMotionMaxSamples; ++i) {
    if (i != 0u) out << ", ";
    out << "{\"mesh_key\": \"" << renodx::utils::log::AsHex(by_count[i].first) << "\", \"vs_hash\": \""
        << PoolHashText(vs_by_key[by_count[i].first]) << "\", \"admitted\": " << by_count[i].second << "}";
  }
  out << "]";
  const auto write_samples = [&out](const char* name, const std::vector<PoolMotionSample>& samples) {
    out << ",\n    \"" << name << "\": [";
    for (size_t i = 0; i < samples.size(); ++i) {
      const PoolMotionSample& sample = samples[i];
      out << (i != 0u ? ",\n      " : "\n      ") << "{\"vs_hash\": \"" << PoolHashText(sample.vs_hash)
          << "\", \"mesh_key\": \"" << renodx::utils::log::AsHex(sample.mesh_key) << "\", \"frame\": " << sample.frame
          << ", \"view\": \""
          << ((sample.pass & kPoolPassCamera) != 0u ? "camera" : (sample.pass & kPoolPassLight) != 0u ? "light" : "other")
          << "\", \"indirect\": " << (sample.indirect ? "true" : "false")
          << ", \"draw_instances\": " << sample.draw_instances << ", \"element\": " << sample.element
          << ", \"draw_moved\": " << sample.draw_moved << ", \"repeat\": " << (sample.repeat ? "true" : "false")
          << ", \"observed_frames\": " << sample.observed_frames
          << ", \"admitted\": " << (sample.admitted ? "true" : "false") << ", \"ulps\": " << sample.ulps
          << ", \"meters\": " << PoolJsonFloat{sample.meters} << ", \"basis\": " << PoolJsonFloat{sample.basis}
          << ", \"world\": [";
      for (uint32_t k = 0; k < kPoolWorldFloats; ++k) out << (k != 0u ? ", " : "") << PoolJsonFloat{sample.world[k]};
      out << "], \"prev_world\": [";
      for (uint32_t k = 0; k < kPoolWorldFloats; ++k) out << (k != 0u ? ", " : "") << PoolJsonFloat{sample.prev_world[k]};
      out << "]}";
    }
    out << "]";
  };
  write_samples("moved_samples", detail.moved_samples);
  write_samples("repeat_samples", detail.repeat_samples);
  write_samples("rule_stale_samples", detail.rule_stale_samples);
  out << "},\n";
}

// One draw key seen moving, for the dump (with its mesh, when captured).
struct PoolDynamicDumpEntry {
  uint64_t mesh_key = 0u;
  PoolDynamicMesh entry;
  bool has_mesh = false;
  uint32_t mesh_id = 0u;
  uint64_t mesh_uid = 0u;
  bool mesh_dynamic = false;
  uint32_t triangles = 0u;
  float bbox_min[3] = {};
  float bbox_max[3] = {};
};

// Caller holds g_pool.mutex.
inline std::vector<PoolDynamicDumpEntry> SnapshotPoolDynamic() {
  std::vector<PoolDynamicDumpEntry> entries;
  entries.reserve(g_pool.dynamic_keys.size());
  for (const auto& [key, entry] : g_pool.dynamic_keys) {
    PoolDynamicDumpEntry& out = entries.emplace_back();
    out.mesh_key = key;
    out.entry = entry;
    const auto mesh_it = g_pool.mesh_by_key.find(key);
    if (mesh_it != g_pool.mesh_by_key.end() && mesh_it->second < g_pool.meshes.size()) {
      const WorldMesh& mesh = g_pool.meshes[mesh_it->second];
      out.has_mesh = true;
      out.mesh_id = mesh.mesh_id;
      out.mesh_uid = mesh.uid;
      out.mesh_dynamic = mesh.dynamic;
      out.triangles = mesh.triangle_count;
      std::memcpy(out.bbox_min, mesh.bbox_min, sizeof(out.bbox_min));
      std::memcpy(out.bbox_max, mesh.bbox_max, sizeof(out.bbox_max));
    }
  }
  std::sort(entries.begin(), entries.end(), [](const PoolDynamicDumpEntry& a, const PoolDynamicDumpEntry& b) {
    return a.entry.moving_sightings != b.entry.moving_sightings ? a.entry.moving_sightings > b.entry.moving_sightings
                                                                : a.mesh_key < b.mesh_key;
  });
  return entries;
}

inline void WritePoolDynamic(std::ostringstream& out, const std::vector<PoolDynamicDumpEntry>& entries,
                             const PoolStats& stats) {
  out << "  \"dynamic\": {\"keys\": " << entries.size() << ", \"keys_seen\": " << stats.dynamic_keys
      << ", \"moving_keys\": " << stats.dynamic_moving_keys << ", \"released\": " << stats.dynamic_released
      << ", \"meshes_flagged\": " << stats.dynamic_meshes << ", \"marked\": " << stats.dynamic_meshes_marked
      << ", \"retired\": " << stats.dynamic_retired << ", \"blocked\": " << stats.dynamic_blocked
      << ", \"cap_drops\": " << stats.dynamic_cap_drops << ", \"entries\": [";
  for (size_t i = 0; i < entries.size(); ++i) {
    const PoolDynamicDumpEntry& item = entries[i];
    const PoolDynamicMesh& entry = item.entry;
    out << (i != 0u ? ",\n    " : "\n    ") << "{\"mesh_key\": \"" << renodx::utils::log::AsHex(item.mesh_key)
        << "\", \"vs_hash\": \"" << PoolHashText(entry.vs_hash) << "\", \"first_frame\": " << entry.first_frame
        << ", \"last_frame\": " << entry.last_frame << ", \"moving_sightings\": " << entry.moving_sightings
        << ", \"indirect_sightings\": " << entry.indirect_sightings
        << ", \"max_meters\": " << PoolJsonFloat{entry.max_meters} << ", \"max_basis\": " << PoolJsonFloat{entry.max_basis}
        << ", \"retired_instances\": " << entry.retired_instances << ", \"blocked\": " << entry.blocked
        << ", \"moving_now\": " << (entry.moving_now ? "true" : "false") << ", \"released\": " << entry.released;
    if (item.has_mesh) {
      out << ", \"mesh_id\": " << item.mesh_id << ", \"mesh_uid\": " << item.mesh_uid
          << ", \"mesh_flagged\": " << (item.mesh_dynamic ? "true" : "false") << ", \"triangles\": " << item.triangles
          << ", \"bbox_min\": [" << PoolJsonFloat{item.bbox_min[0]} << ", " << PoolJsonFloat{item.bbox_min[1]} << ", "
          << PoolJsonFloat{item.bbox_min[2]} << "], \"bbox_max\": [" << PoolJsonFloat{item.bbox_max[0]} << ", "
          << PoolJsonFloat{item.bbox_max[1]} << ", " << PoolJsonFloat{item.bbox_max[2]} << "]";
    } else {
      out << ", \"mesh_id\": null";
    }
    out << ", \"world\": [";
    for (uint32_t k = 0; k < kPoolWorldFloats; ++k) out << (k != 0u ? ", " : "") << PoolJsonFloat{entry.world[k]};
    out << "], \"prev_world\": [";
    for (uint32_t k = 0; k < kPoolWorldFloats; ++k) out << (k != 0u ? ", " : "") << PoolJsonFloat{entry.prev_world[k]};
    out << "]}";
  }
  out << "]},\n";
}

// Camera visibility of the admitted instances, for the dump (see
// GetPoolCameraVisibility and camera_fade.hpp).
inline void WritePoolVisibilitySummary(
    std::ostringstream& out,
    const std::vector<WorldInstance>& instances,
    const std::vector<PoolCameraVisibility>& visibility,
    const std::vector<WorldMesh>& meshes,
    const PoolCameraInfo& camera,
    const PoolRegion& region,
    float near_fade_floor,
    uint64_t visibility_changes) {
  uint32_t camera_visible = 0u;
  uint32_t shadow_only = 0u;
  uint32_t shadow_only_in_region = 0u;
  uint32_t no_view = 0u;  // neither view recorded (observation gone, or a VS of another view)
  std::unordered_map<uint32_t, uint32_t> shadow_only_by_vs;
  uint32_t with_inputs = 0u;
  uint32_t from_camera = 0u;
  uint32_t from_light = 0u;
  uint32_t from_admission = 0u;
  uint32_t near_fade = 0u;
  uint32_t start_buckets[6] = {};  // <=0, 0-2, 2-5, 5-10, 10-20, >20 m
  uint32_t inv_range_zero = 0u;
  uint32_t unusable = 0u;
  float inv_range_min = 1e30f;
  float inv_range_max = -1e30f;
  uint32_t opacity_below_half = 0u;
  uint32_t flipped = 0u;
  uint32_t changed = 0u;
  uint32_t hidden_at_camera = 0u;
  struct Sample {
    size_t index = 0u;
    float distance = 0.f;
    float fade = 1.f;
    const char* reason = "";
  };
  std::vector<Sample> samples;
  const auto closest_distance = [&camera](const WorldInstance& instance) {
    float sum = 0.f;
    for (int k = 0; k < 3; ++k) {
      const float d = (std::max)((std::max)(instance.bounds_min[k] - camera.position[k], 0.f),
                                 camera.position[k] - instance.bounds_max[k]);
      sum += d * d;
    }
    return std::sqrt(sum);
  };
  for (size_t i = 0; i < instances.size() && i < visibility.size(); ++i) {
    const WorldInstance& instance = instances[i];
    const PoolCameraVisibility& entry = visibility[i];
    const bool in_region = camera.valid && PoolInstanceInRegion(instance, region);
    if (entry.camera_seen) {
      camera_visible += 1u;
    } else if (entry.light_seen) {
      shadow_only += 1u;
      shadow_only_by_vs[instance.source_vs_hash] += 1u;
      if (in_region) shadow_only_in_region += 1u;
    } else {
      no_view += 1u;
    }
    if (!entry.camera_seen && in_region) {
      samples.push_back({i, closest_distance(instance), 1.f, entry.light_seen ? "shadow-only" : "no view"});
    }
    if (!entry.inputs.valid) continue;
    with_inputs += 1u;
    if (std::strcmp(entry.source, "camera draw") == 0) {
      from_camera += 1u;
    } else if (std::strcmp(entry.source, "light draw") == 0) {
      from_light += 1u;
    } else {
      from_admission += 1u;
    }
    const float* param = entry.inputs.param;
    if (entry.inputs.color[3] < kCameraFadeShown) opacity_below_half += 1u;
    if (param[2] > 0.f) flipped += 1u;
    if (instance.visibility.valid
        && (instance.visibility.param[0] != param[0] || instance.visibility.param[1] != param[1]
            || instance.visibility.param[2] != param[2] || instance.visibility.color[3] != entry.inputs.color[3])) {
      changed += 1u;
    }
    if (!entry.camera_seen || !entry.near_fade) continue;
    if (!CameraFadeInputsUsable(param[0], param[1])) {
      unusable += 1u;
      continue;
    }
    near_fade += 1u;
    const float start = param[0];
    const int bucket = start <= 0.f ? 0 : start <= 2.f ? 1 : start <= 5.f ? 2 : start <= 10.f ? 3 : start <= 20.f ? 4 : 5;
    start_buckets[bucket] += 1u;
    if (param[1] == 0.f) inv_range_zero += 1u;
    inv_range_min = (std::min)(inv_range_min, param[1]);
    inv_range_max = (std::max)(inv_range_max, param[1]);
    if (!in_region) continue;
    const float distance = closest_distance(instance);
    const float fade = CameraNearFade(distance, start, param[1], near_fade_floor);
    if (fade < kCameraFadeShown) hidden_at_camera += 1u;
    if (fade < 1.f) samples.push_back({i, distance, fade, "near fade"});
  }
  std::sort(samples.begin(), samples.end(), [](const Sample& a, const Sample& b) { return a.distance < b.distance; });
  if (samples.size() > 24u) samples.resize(24u);
  std::vector<std::pair<uint32_t, uint32_t>> by_vs(shadow_only_by_vs.begin(), shadow_only_by_vs.end());
  std::sort(by_vs.begin(), by_vs.end(), [](const auto& a, const auto& b) { return a.second != b.second ? a.second > b.second : a.first < b.first; });
  if (by_vs.size() > 12u) by_vs.resize(12u);

  out << "  \"visibility\": {\"instances\": " << instances.size()
      << ", \"camera_visible\": " << camera_visible
      << ", \"shadow_only\": " << shadow_only
      << ", \"shadow_only_in_region\": " << shadow_only_in_region
      << ", \"no_view\": " << no_view
      << ", \"shadow_only_by_vs\": {";
  for (size_t v = 0; v < by_vs.size(); ++v) {
    if (v != 0u) out << ", ";
    out << "\"" << PoolHashText(by_vs[v].first) << "\": " << by_vs[v].second;
  }
  out << "}"
      << ", \"with_inputs\": " << with_inputs
      << ", \"inputs_from\": {\"camera_draw\": " << from_camera << ", \"light_draw\": " << from_light
      << ", \"admission\": " << from_admission << "}"
      << ", \"near_fade\": " << near_fade
      << ", \"near_fade_start_m\": {\"<=0\": " << start_buckets[0] << ", \"0-2\": " << start_buckets[1]
      << ", \"2-5\": " << start_buckets[2] << ", \"5-10\": " << start_buckets[3] << ", \"10-20\": " << start_buckets[4]
      << ", \">20\": " << start_buckets[5] << "}"
      << ", \"near_fade_inv_range\": {\"min\": " << PoolJsonFloat{inv_range_min <= inv_range_max ? inv_range_min : 0.f}
      << ", \"max\": " << PoolJsonFloat{inv_range_min <= inv_range_max ? inv_range_max : 0.f} << ", \"zero\": " << inv_range_zero
      << ", \"not_finite\": " << unusable << "}"
      << ", \"opacity_below_half\": " << opacity_below_half
      << ", \"dither_flip\": " << flipped
      << ", \"changed_since_admission\": " << changed
      << ", \"visibility_changes\": " << visibility_changes
      << ", \"hidden_at_camera\": " << hidden_at_camera
      << ", \"samples\": [";
  for (size_t s = 0; s < samples.size(); ++s) {
    const WorldInstance& instance = instances[samples[s].index];
    const PoolCameraVisibility& entry = visibility[samples[s].index];
    const WorldMesh* mesh = instance.mesh_id < meshes.size() ? &meshes[instance.mesh_id] : nullptr;
    if (s != 0u) out << ",";
    out << "\n    {\"reason\": \"" << samples[s].reason << "\""
        << ", \"mesh_uid\": " << (mesh != nullptr ? mesh->uid : 0u)
        << ", \"vs_hash\": \"" << PoolHashText(instance.source_vs_hash) << "\""
        << ", \"position\": [" << instance.matrix[3] << ", " << instance.matrix[7] << ", " << instance.matrix[11] << "]"
        << ", \"bounds_min\": [" << instance.bounds_min[0] << ", " << instance.bounds_min[1] << ", " << instance.bounds_min[2] << "]"
        << ", \"bounds_max\": [" << instance.bounds_max[0] << ", " << instance.bounds_max[1] << ", " << instance.bounds_max[2] << "]"
        << ", \"triangles\": " << (mesh != nullptr ? mesh->triangle_count : 0u)
        << ", \"closest_distance\": " << PoolJsonFloat{samples[s].distance}
        << ", \"fade_at_closest\": " << PoolJsonFloat{samples[s].fade}
        << ", \"start\": " << PoolJsonFloat{entry.inputs.param[0]}
        << ", \"inv_range\": " << PoolJsonFloat{entry.inputs.param[1]}
        << ", \"opacity\": " << PoolJsonFloat{entry.inputs.color[3]}
        << ", \"inputs\": \"" << entry.source << "\""
        << ", \"camera_ps\": \"" << PoolHashText(entry.camera_ps_hash) << "\""
        << ", \"camera_frames\": " << entry.camera_frames
        << ", \"light_frames\": " << entry.light_frames
        << ", \"mesh_camera_draws\": " << (mesh != nullptr ? mesh->camera_draws : 0u)
        << ", \"mesh_light_draws\": " << (mesh != nullptr ? mesh->light_draws : 0u) << "}";
  }
  out << "\n  ]},\n";
}

inline void DumpWorldPool() {
  std::vector<WorldMesh> meshes;
  std::vector<WorldInstance> instances;
  std::vector<PoolCameraVisibility> visibility;  // per instance
  std::unordered_map<uint32_t, PoolFamilyStats> families;
  PoolStats stats;
  PoolMotionDetail motion_detail;
  std::vector<PoolDynamicDumpEntry> dynamic;
  PoolRegion region;
  const PoolCameraInfo camera = GetPoolCameraInfo();
  bool scene_fade = false;
  float near_fade_floor = 0.f;
  float map_alpha = 1.f;
  {
    std::lock_guard<std::mutex> lock(g_state.mutex);
    scene_fade = g_state.camera.has_fade;
    near_fade_floor = g_state.camera.near_fade_floor;
    map_alpha = g_state.camera.map_alpha;
  }
  const contract::RegistryCounts registry_counts = contract::SnapshotRegistryCounts();
  const contract::RegistryEntries registry_entries = contract::SnapshotRegistryEntries();
  {
    std::lock_guard<std::mutex> lock(g_pool.mutex);
    UpdatePoolStats();
    meshes = g_pool.meshes;
    instances = g_pool.instances;
    visibility.reserve(instances.size());
    for (const WorldInstance& instance : instances) visibility.push_back(GetPoolCameraVisibility(instance));
    families = g_pool.families;
    stats = g_pool.stats;
    motion_detail = g_pool.motion_detail;
    dynamic = SnapshotPoolDynamic();
    region = CurrentPoolRegion();
  }

  struct FamilyBounds {
    uint32_t meshes = 0u;
    uint32_t in_region = 0u;
    uint32_t outside_legacy_scale = 0u;
    float bounds_min[3] = {1e30f, 1e30f, 1e30f};
    float bounds_max[3] = {-1e30f, -1e30f, -1e30f};
  };
  std::unordered_map<uint32_t, FamilyBounds> bounds;
  for (const auto& mesh : meshes) bounds[mesh.source_vs_hash].meshes += 1u;
  uint32_t outside_legacy_scale = 0u;
  for (const auto& instance : instances) {
    auto& entry = bounds[instance.source_vs_hash];
    if (PoolInstanceInRegion(instance, region)) entry.in_region += 1u;
    if (PoolOutsideLegacyScale(instance.matrix)) {
      entry.outside_legacy_scale += 1u;
      outside_legacy_scale += 1u;
    }
    for (int i = 0; i < 3; ++i) {
      entry.bounds_min[i] = (std::min)(entry.bounds_min[i], instance.bounds_min[i]);
      entry.bounds_max[i] = (std::max)(entry.bounds_max[i], instance.bounds_max[i]);
    }
  }

  std::ostringstream out;
  out << "{\n";
  out << "  \"schema\": 9,\n";
  out << "  \"generated_frame\": " << g_state.frame.load() << ",\n";
  out << "  \"camera_valid\": " << (camera.valid ? "true" : "false") << ",\n";
  out << "  \"camera_position\": [" << camera.position[0] << ", " << camera.position[1] << ", " << camera.position[2] << "],\n";
  out << "  \"scene\": {\"fade_constants\": " << (scene_fade ? "true" : "false") << ", \"near_fade_floor\": "
      << PoolJsonFloat{near_fade_floor} << ", \"map_alpha\": " << PoolJsonFloat{map_alpha} << "},\n";
  out << "  \"switches\": {\"capture_meshes\": " << (g_pool.capture_meshes.load() ? "true" : "false")
      << ", \"scan_indirect\": " << (g_pool.scan_indirect.load() ? "true" : "false")
      << ", \"verify_meshes\": " << (g_pool.verify_meshes.load() ? "true" : "false")
      << ", \"legacy_scale\": " << (g_pool.legacy_scale.load() ? "true" : "false")
      << ", \"exclude_moving\": " << (g_pool.exclude_moving.load() ? "true" : "false") << "},\n";
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
      << ", \"indirect_first_copies\": " << stats.indirect_first_copies
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
      << ", \"mesh_verified\": " << stats.mesh_verified
      << ", \"mesh_capture_mismatches\": " << stats.mesh_capture_mismatches
      << ", \"mesh_unstable\": " << stats.mesh_unstable
      << ", \"tracked_buffer_writes\": " << stats.tracked_buffer_writes
      << ", \"requests_written\": " << stats.requests_written
      << ", \"meshes_written\": " << stats.meshes_written
      << ", \"mass_retirements\": " << stats.mass_retirements
      << ", \"last_mass_retire_frame\": " << stats.last_mass_retire_frame
      << ", \"last_mass_retire_meshes\": " << stats.last_mass_retire_meshes
      << ", \"meshes_retired\": " << stats.meshes_retired
      << ", \"instances_retired\": " << stats.instances_retired
      << ", \"resource_invalidations\": " << stats.resource_invalidations
      << ", \"observed_instances\": " << stats.observed
      << ", \"admitted_instances\": " << stats.admitted
      << ", \"admitted_outside_legacy_scale\": " << outside_legacy_scale
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
        << contract::VsClassName(static_cast<contract::VsClass>(registry_entries.vertex[i].cls)) << "\""
        << ", \"view\": \""
        << ((registry_entries.vertex[i].flags & contract::kTraitCameraView) != 0u  ? "camera"
            : (registry_entries.vertex[i].flags & contract::kTraitLightView) != 0u ? "light"
                                                                                    : "other")
        << "\", \"visibility_layout\": "
        << ((registry_entries.vertex[i].flags & contract::kTraitVisibilityLayout) != 0u ? "true" : "false") << "}";
  }
  out << "\n  ],\n";
  out << "  \"pixel_shaders\": [";
  for (size_t i = 0; i < registry_entries.pixel.size(); ++i) {
    if (i != 0u) out << ",";
    out << "\n    {\"hash\": \"" << PoolHashText(registry_entries.pixel[i].hash) << "\", \"class\": \""
        << contract::PsClassName(static_cast<contract::PsClass>(registry_entries.pixel[i].cls)) << "\""
        << ", \"near_fade\": " << ((registry_entries.pixel[i].flags & contract::kTraitNearFade) != 0u ? "true" : "false")
        << "}";
  }
  out << "\n  ],\n";

  WritePoolMotion(out, stats.motion, motion_detail, instances);
  WritePoolDynamic(out, dynamic, stats);

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
        << ", \"near_miss_max_delta\": " << PoolJsonFloat{family.near_miss_max_delta}
        << ", \"mesh_mismatches\": " << family.mesh_mismatches
        << ", \"mesh_unstable\": " << family.mesh_unstable
        << ", \"outside_legacy_scale\": " << entry.outside_legacy_scale
        << ", \"motion\": {\"repeat\": " << family.motion_repeat
        << ", \"first\": " << family.motion_first
        << ", \"max_repeat_ulps\": " << family.motion_max_repeat_ulps
        << ", \"moved\": " << family.motion_moved
        << ", \"moved_indirect\": " << family.motion_moved_indirect
        << ", \"max_moved_meters\": " << PoolJsonFloat{family.motion_max_moved_meters} << "}"
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

  // Like the trace's assumption in its inspect lines: floor 0 until the scene
  // constants are captured ("fade_constants" says which).
  WritePoolVisibilitySummary(out, instances, visibility, meshes, camera, region, scene_fade ? near_fade_floor : 0.f,
                             stats.visibility_changes);

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
        << ", \"uid\": " << mesh.uid
        << ", \"capture_frame\": " << mesh.capture_frame
        << ", \"from_indirect\": " << (mesh.from_indirect ? "true" : "false")
        << ", \"writes_after_capture\": " << mesh.writes_after_capture
        << ", \"captures\": " << mesh.captures
        << ", \"verified\": " << (mesh.verified ? "true" : "false")
        << ", \"vb\": \"" << PoolBufferText(mesh.source_vb_usage, mesh.source_vb_flags) << "\""
        << ", \"ib\": \"" << PoolBufferText(mesh.source_ib_usage, mesh.source_ib_flags) << "\""
        << ", \"camera_draws\": " << mesh.camera_draws
        << ", \"near_fade_draws\": " << mesh.camera_near_fade_draws
        << ", \"light_draws\": " << mesh.light_draws
        << ", \"camera_ps\": \"" << PoolHashText(mesh.camera_ps_hash) << "\""
        << ", \"bbox_min\": [" << mesh.bbox_min[0] << ", " << mesh.bbox_min[1] << ", " << mesh.bbox_min[2] << "]"
        << ", \"bbox_max\": [" << mesh.bbox_max[0] << ", " << mesh.bbox_max[1] << ", " << mesh.bbox_max[2] << "]}";
  }
  out << "\n  ]\n}\n";

  std::string text = out.str();
  renodx::utils::path::WriteTextFile(PoolOutputDir() / "world_pool.json", text);
}

// The prevWorld trace (see PoolPrevTrace): each key's state and evidence, the
// record summary, then every record. Written once, when no key is open.
inline void WritePoolPrevTrace(const PoolPrevTrace& trace) {
  constexpr const char* kStates[] = {"waiting", "active", "done", "gave_up"};
  constexpr char kRoles[] = "ABC";
  size_t compared = 0u;
  size_t exact_last = 0u;
  size_t within_noise_last = 0u;
  size_t mismatch_last = 0u;
  size_t unfilled = 0u;
  size_t exact_world = 0u;
  float max_mismatch_meters = 0.f;
  for (const auto& record : trace.records) {
    if (!record.prev_filled) unfilled += 1u;
    if (record.prev_vs_world.exact) exact_world += 1u;
    if (!record.has_last) continue;
    compared += 1u;
    if (record.prev_vs_last.exact) exact_last += 1u;
    if (record.prev_vs_last.ulps <= kPoolMotionUlpNoise) {
      within_noise_last += 1u;
    } else {
      mismatch_last += 1u;
      max_mismatch_meters = (std::max)(max_mismatch_meters, record.prev_vs_last.meters);
    }
  }

  std::ostringstream out;
  out << "{\n  \"schema\": 1,\n  \"armed_frame\": " << trace.armed_frame << ",\n";
  out << "  \"notes\": \"keys A (far flip), B (slow sway), C (static). One record per camera copy of a key and element "
         "(element 0 only for prev_equals_last). prev_equals_last: prevWorld equals the previous frame's world of the "
         "same key.\",\n";
  out << "  \"keys\": [";
  for (size_t k = 0; k < trace.keys.size(); ++k) {
    const PoolPrevTrace::Key& key = trace.keys[k];
    out << (k != 0u ? ", " : "") << "{\"role\": \"" << kRoles[k] << "\", \"state\": \""
        << kStates[static_cast<size_t>(key.state)] << "\", \"mesh_key\": " << key.mesh_key << ", \"vs_hash\": \""
        << PoolHashText(key.vs_hash) << "\", \"selected\": " << key.selected_frame << ", \"frames\": "
        << key.frames_recorded << ", \"missed\": " << key.missed << ", \"extra\": " << key.extra_camera_draws
        << ", \"evidence\": {\"max_meters\": " << PoolJsonFloat{key.max_meters}
        << ", \"max_basis\": " << PoolJsonFloat{key.max_basis} << ", \"moving_sightings\": " << key.moving_sightings
        << ", \"camera_sightings\": " << key.camera_sightings << "}}";
  }
  out << "],\n";
  out << "  \"summary\": {\"records\": " << trace.records.size() << ", \"compared\": " << compared
      << ", \"prev_exact_last\": " << exact_last << ", \"prev_within_3ulp_last\": " << within_noise_last
      << ", \"prev_mismatch_last\": " << mismatch_last << ", \"max_mismatch_meters\": "
      << PoolJsonFloat{max_mismatch_meters} << ", \"prev_unfilled\": " << unfilled << ", \"prev_exact_world\": "
      << exact_world << "},\n";
  out << "  \"records\": [";
  for (size_t i = 0; i < trace.records.size(); ++i) {
    const PoolPrevTrace::Record& record = trace.records[i];
    out << (i != 0u ? ",\n    " : "\n    ") << "{\"role\": \"" << kRoles[record.key] << "\", \"frame\": " << record.frame
        << ", \"element\": " << record.element << ", \"draw_instances\": " << record.draw_instances
        << ", \"camera_draws\": " << record.camera_draws << ", \"cpu_base\": " << record.cpu_base
        << ", \"trace_only\": " << (record.trace_only ? "true" : "false") << ", \"world\": [";
    for (uint32_t k = 0; k < kPoolWorldFloats; ++k) out << (k != 0u ? ", " : "") << PoolJsonFloat{record.world[k]};
    out << "], \"prev_world\": [";
    for (uint32_t k = 0; k < kPoolWorldFloats; ++k) out << (k != 0u ? ", " : "") << PoolJsonFloat{record.prev_world[k]};
    out << "], \"prev_filled\": " << (record.prev_filled ? "true" : "false")
        << ", \"prev_equals_world\": " << (record.prev_vs_world.exact ? "true" : "false")
        << ", \"has_last\": " << (record.has_last ? "true" : "false")
        << ", \"prev_equals_last\": " << (record.prev_vs_last.exact ? "true" : "false")
        << ", \"prev_last_ulps\": " << record.prev_vs_last.ulps
        << ", \"prev_last_meters\": " << PoolJsonFloat{record.prev_vs_last.meters}
        << ", \"world_step_meters\": " << PoolJsonFloat{record.world_step_meters} << "}";
  }
  out << "\n  ]\n}\n";

  std::string text = out.str();
  renodx::utils::path::WriteTextFile(PoolOutputDir() / "world_prev_trace.json", text);
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
