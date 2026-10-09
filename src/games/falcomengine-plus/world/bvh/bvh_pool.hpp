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
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <limits>
#include <mutex>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../../../../utils/log.hpp"
#include "../../../../utils/path.hpp"
#include "../../../../utils/scene.hpp"
#include "../capture/buffer_readback.hpp"
#include "alpha_atlas.hpp"
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
// A follow copy (moving key, cooldown bypassed) may fill up to 1/share of a slot.
inline constexpr uint64_t kPoolFollowSlotShare = 2u;
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
inline constexpr uint32_t kPoolPoisonWord = 0xFFC0DEADu;  // poison_staging: what a read leaves in the mesh staging
inline constexpr uint64_t kPoolResidueCycles = 2u;        // poison_staging: cycles of heads kept per slot
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
inline constexpr size_t kPoolMismatchMaxMeshes = 64u;  // mesh mismatch histories kept (diagnostic)
inline constexpr size_t kPoolDumpMaxInstances = 20000u;  // instances[] in world_pool.json
inline constexpr size_t kPoolDumpMaxAlphaKeys = 256u;   // alpha_keys[] in world_pool.json
inline constexpr uint32_t kPoolMeshRetryRounds = 3u;      // unstable meshes captured again this many times
inline constexpr uint32_t kPoolMeshRetryFrames = 180u;    // frames before each new round
inline constexpr uint32_t kPoolLifecycleLogLines = 200u;  // orphan, moving and identity log lines per session (each its own)
inline constexpr uint32_t kPoolMeshFailLogLines = 64u;    // mesh failure log lines per pool reset
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
inline constexpr uint32_t kPoolFollowRelinkFrames = 8u;     // unseen frames a moving instance may be relinked across
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
  WindOpaque,        // wind vertex shader with an opaque pixel shader: not alpha foliage, refused while alpha_foliage is on
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
    case PoolSkip::WindOpaque:       return "wind_opaque";
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
inline constexpr uint8_t kPoolPassAlpha = 16u;       // PS alpha-tests (cutout material), admitted only with alpha_foliage on
inline constexpr uint8_t kPoolPassWind = 32u;        // wind VS admitted as its rest pose with an opaque PS (alpha_wind_opaque on)

struct PoolSighting {
  uint8_t pass = 0u;     // kPoolPass* bits
  uint32_t ps_hash = 0u;
};

// Alpha material of a draw key (its first sighting): view (the SRV handle bound to t0, not the
// resource; 0 when unreadable), threshold, UV scroll and swizzle from the material cbs.
struct PoolAlphaMaterial {
  uint64_t view = 0u;
  uint64_t resource = 0u;  // the t0 resource (diagnostics only; decisions use view)
  float threshold = 0.f;
  float scroll[2] = {};
  uint32_t swizzle = 0u;
};

// Which parts of a material differ from the first one (conflict_fields). Conflict is decided on
// the first four bits; kPoolAlphaDiffResource marks a view whose resource differs too.
inline constexpr uint8_t kPoolAlphaDiffView = 1u;
inline constexpr uint8_t kPoolAlphaDiffSwizzle = 2u;
inline constexpr uint8_t kPoolAlphaDiffThreshold = 4u;
inline constexpr uint8_t kPoolAlphaDiffScroll = 8u;
inline constexpr uint8_t kPoolAlphaDiffResource = 16u;

// UV layout verdict of a draw's vertex input (ClassifyPoolUvLayout), for diagnostics.
enum class PoolUvVerdict : uint8_t {
  NotEvaluated,
  Ok,
  NoTexcoord,
  TexcoordOtherSlot,
  TexcoordUnsupportedFormat,
  TexcoordBeyondStride,
};

inline constexpr uint32_t kPoolLayoutElementsMax = 16u;
inline constexpr uint32_t kPoolVbSlotsMax = 4u;

struct PoolLayoutElement {
  char semantic[12] = {};
  uint32_t index = 0u;
  uint32_t slot = 0u;
  uint32_t offset = 0u;
  uint32_t format = 0u;
};

struct PoolVertexBufferSlot {
  uint32_t slot = 0u;
  uint64_t handle = 0u;
  uint32_t stride = 0u;
  uint64_t offset = 0u;
};

// The vertex buffer slots bound at a direct draw (up to kPoolVbSlotsMax, in slot order).
struct PoolVertexBuffers {
  uint32_t count = 0u;
  std::array<PoolVertexBufferSlot, kPoolVbSlotsMax> slots = {};
};

// TEXCOORD0 bound in a vertex buffer slot other than 0: its own stream, copied with the
// vertex range. buffer.handle 0 = no such stream.
struct PoolUvStream {
  reshade::api::resource buffer = {0u};
  uint64_t offset = 0u;          // the slot's byte offset
  uint32_t stride = 0u;          // the slot's stride
  uint32_t element_offset = 0u;  // TEXCOORD0 offset within the stride
  reshade::api::format format = reshade::api::format::unknown;
  uint64_t size = 0u;            // buffer size (bounds of the copy)
};

// Vertex input of a draw key's mesh: the UV verdict and the first elements, for diagnostics.
struct PoolAlphaLayout {
  bool valid = false;  // ClassifyPoolUvLayout ran
  PoolUvVerdict verdict = PoolUvVerdict::NotEvaluated;
  bool uv_exists = false;
  uint32_t uv_slot = 0u;
  uint32_t uv_offset = 0u;
  uint32_t uv_format = 0u;
  uint32_t vertex_stride = 0u;
  uint32_t element_total = 0u;
  std::array<PoolLayoutElement, kPoolLayoutElementsMax> elements = {};
  bool vbs_known = false;  // false for indirect draws (their slots are not read)
  PoolVertexBuffers vbs;
};

// Material held by a draw key or a mesh. conflict_fields and conflict_incoming describe the
// first conflict; the material held stays the first readable one.
// The material part (what a mesh keeps for every mesh, ON or OFF).
struct PoolAlphaMaterialState {
  PoolAlphaMaterial material;
  bool conflict = false;
  uint8_t conflict_fields = 0u;
  PoolAlphaMaterial conflict_incoming;
};

// A draw key's state: its material and its vertex input layout (ON only; kept out of meshes).
struct PoolAlphaState : PoolAlphaMaterialState {
  PoolAlphaLayout layout;
};

// A copy of an alpha-tested draw's source texture (mip 0), made at draw time and blitted into the atlas
// once its mesh is resident. One copy per source texture: the draw keys that sample it are its refcount
// (keys). It is freed when no key is left (their invalidation) or alpha_foliage goes off; a blitted copy
// freed this way is copied again on demand.
struct PoolAlphaSource {
  uint64_t texture = 0u;                // source texture handle (alpha_source_by_texture)
  bool texture_dead = false;            // the game destroyed the texture: the copy is kept until its keys go
  uint64_t hold_until = 0u;             // keyless copy (no key yet): kept until this frame, 0 = none
  reshade::api::resource proxy = {0u};  // empty while its copy is being made
  reshade::api::format format = reshade::api::format::unknown;  // the format of the view the game bound
  uint64_t bytes = 0u;                  // mip 0 bytes (kAlphaSourceBytesMax)
  bool blitted = false;                 // the atlas has been filled from it at least once
  std::unordered_set<uint64_t> keys;    // draw keys that use this texture
};
// The GPU stage's numbers as the pool last saw them (written under g_pool.mutex by SyncLiveAlpha at the start of
// each present, so they lag one present; the trace fields come from the last readback). Read by the dump and the panel.
struct PoolAlphaGpuStats {
  uint32_t slices_used = 0u;
  uint64_t blits = 0u;             // slices filled since the atlas was created
  uint32_t blits_frame = 0u;       // slices filled at the last present
  uint32_t proxies = 0u;           // source copies live
  uint64_t proxy_bytes = 0u;       // bytes of the live source copies
  uint64_t cap_refused = 0u;       // admissions without a slice (atlas full)
  uint32_t tlas_instances = 0u;    // alpha-tested instances in the TLAS
  uint32_t waiting = 0u;           // alpha-tested instances left out of the TLAS (no slice yet)
  uint64_t tests = 0u;             // trace: triangle hits tested against the atlas (last readback)
  uint64_t cut = 0u;               // trace: of those, cut
  float blit_ms = -1.f;            // GPU ms of the blits (-1: not measured)
  float trace_ms = -1.f;           // GPU ms of the last trace dispatch (-1: not measured)
};

// The atlas maps, for the dump only (the panel copies PoolAlphaGpuStats every UI frame and must not copy them). Copied
// under the pool lock by SyncLiveAlpha at each store change, with alpha_foliage on.
struct PoolAlphaDumpMaps {
  uint64_t store_version = ~uint64_t{0};  // data->store_version the maps were copied at
  std::unordered_map<uint64_t, uint32_t> slice_of_uid;  // copy of AlphaGpu::slot_of_uid
  std::unordered_set<uint64_t> indirect_uids;           // copy of AlphaGpu::indirect_uids
  std::unordered_set<uint64_t> resident_uids;           // mesh uids in the live store (slot_by_uid)
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
  std::vector<std::array<float, 2>> uvs;  // per position; empty when the layout has no UV
  std::vector<uint32_t> indices;  // flat, three per triangle
  uint64_t signature = 0u;        // content hash (positions + indices + UVs)
  uint32_t live_keys = 0u;        // draw keys still mapped to this mesh
  // Provenance (diagnostic): the draw whose copies produced the mesh.
  uint32_t capture_frame = 0u;
  uint64_t source_vb = 0u;
  uint64_t source_ib = 0u;
  bool from_indirect = false;
  uint32_t writes_after_capture = 0u;  // game writes to its VB/IB seen since (the mesh may be stale)
  uint32_t captures = 0u;              // captures it took
  bool verified = false;               // the last two captures decoded to the same mesh
  bool admitted_by_retry = false;      // admitted after a retry round (kPoolMeshRetryRounds)
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
  // Alpha-tested material (MarkPoolMeshAlpha): no instance is admitted (AdmitPoolInstance refuses it).
  bool alpha = false;
  PoolAlphaMaterialState alpha_state;  // material and conflict of its alpha keys (AbsorbPoolAlphaMaterial)
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
  // Follow mode (FollowPoolMovingInstance).
  bool dynamic = false;  // admitted for a moving mesh (its pose follows the object)
  uint32_t last_follow_frame = 0u;
  uint32_t follows = 0u;  // pose updates
  uint64_t id = 0u;       // PoolState::next_instance_id, never reused
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
  std::vector<std::array<float, 2>> uvs;  // per position, when the layout has a decodable UV
  std::vector<std::array<uint32_t, 3>> triangles;
  std::array<float, 3> bbox_min = {0.f, 0.f, 0.f};
  std::array<float, 3> bbox_max = {0.f, 0.f, 0.f};
};

// What the next copy of a queued mesh reads.
enum class PoolMeshPhase : uint8_t {
  Indices = 0,  // the draw's index range
  Vertices,     // the vertex range those indices cover
  Uvs,          // the same vertex range of the TEXCOORD0 stream (PoolUvStream)
};

inline const char* PoolMeshPhaseName(PoolMeshPhase phase) {
  return phase == PoolMeshPhase::Indices ? "indices" : phase == PoolMeshPhase::Vertices ? "vertices" : "uvs";
}

// An unstable mesh waiting for its next capture round.
struct PoolMeshRetry {
  uint32_t next_frame = 0u;
  uint32_t rounds = 0u;  // rounds given so far
};

// poison_staging: what a mesh copy's first bytes were when it was read.
struct PoolStagingDiag {
  bool poisoned = false;         // filled in (the switch was on for this read)
  uint32_t slot = 0u;
  uint32_t ordinal = 0u;         // the copy's place in its slot's read list
  uint64_t staging_offset = 0u;
  uint64_t copied = 0u;
  uint32_t begin_mod16 = 0u;     // wanted bytes start at (staging offset + skip) % 16
  uint32_t sentinel_words = 0u;  // poison words in [staging_offset, + copied)
  uint32_t sentinel_head = 0u;   // poison words among the first head_words
  uint32_t head_words = 0u;      // words of the first 64 B that the copy covers
  uint32_t head_bytes = 0u;      // first raw bytes kept (48 at most)
  uint8_t head[48] = {};
  uint64_t head_hash = 0u;       // FNV of head
  const char* head_class = "";   // ClassifyPoolStagingHead
};

// poison_staging: a head read from a slot (per slot, the last kPoolResidueCycles cycles).
struct PoolResidue {
  uint64_t offset = 0u;
  uint64_t head_hash = 0u;
  uint64_t mesh_key = 0u;
  uint64_t cycle = 0u;
};

// One complete capture of a queued mesh (diagnostic: mesh_failed and mesh_mismatches history).
struct PoolCaptureRecord {
  uint32_t frame = 0u;
  uint32_t indices_frame = 0u;
  uint32_t vertices_frame = 0u;
  bool indices_served_indirect = false;
  bool vertices_served_indirect = false;
  uint64_t indices_source_offset = 0u;
  uint64_t vertices_source_offset = 0u;
  uint32_t min_vertex = 0u;
  uint32_t max_vertex = 0u;
  uint64_t indices_raw_hash = 0u;
  uint64_t vertices_raw_hash = 0u;
  uint64_t signature = 0u;
  uint64_t index_signature = 0u;
  bool written = false;
  uint32_t vertices = 0u;
  uint32_t triangles = 0u;
  int64_t first_diff_triangle = -1;  // -1: no difference from the previous capture
  size_t differing_triangles = 0u;
  float before[9] = {};  // corners of the first differing triangle, previous capture
  float after[9] = {};   // and this capture
  PoolStagingDiag staging;
};

// A mesh whose captures disagreed (world_pool.json mesh_mismatches): its draw and capture history.
struct PoolMeshMismatch {
  uint32_t draw_frame = 0u;
  uint32_t draw_serial = 0u;
  std::vector<PoolCaptureRecord> history;
};

// A failed mesh and what its draw showed (world_pool.json mesh_failed).
struct PoolMeshFailure {
  PoolStagingDiag staging;  // the last read of the mesh (poison_staging)
  const char* reason = "";
  uint32_t vs_hash = 0u;
  uint32_t frame = 0u;
  uint32_t captures = 0u;
  uint32_t index_count = 0u;
  uint32_t first_index = 0u;
  int32_t base_vertex = 0;
  uint32_t vertex_stride = 0u;
  uint32_t min_vertex = 0u;
  uint32_t max_vertex = 0u;
  uint64_t vb_size = 0u;
  uint64_t ib_size = 0u;
  bool from_indirect = false;
  int32_t pos_offset = 0;
  uint32_t vb_usage = 0u;
  uint32_t vb_flags = 0u;
  uint32_t ib_usage = 0u;
  uint32_t ib_flags = 0u;
  bool written = false;
  bool logged = false;  // its log line was written (present)
  std::vector<PoolCaptureRecord> history;  // its captures (diagnostic)
  PoolMeshPhase phase = PoolMeshPhase::Indices;
  uint32_t index_size = 0u;
  uint32_t instance_count = 0u;
  uint64_t vb_handle = 0u;
  uint64_t ib_handle = 0u;
  uint64_t vb_offset = 0u;
  uint64_t ib_offset = 0u;
  uint64_t buffer_key = 0u;
  uint32_t raw_index_min = 0u;  // raw index values, before the base vertex
  uint32_t raw_index_max = 0u;
  uint32_t first_raw[8] = {};
  uint64_t raw_hash = 0u;  // FNV of the first 4 KiB of the bytes read
};

// One queued mesh: the draw that first showed the geometry, its position
// layout and, after the index read, the vertices it uses.
struct PoolMeshRequest {
  uint64_t mesh_key = 0u;
  uint64_t serial = 0u;      // copies in flight carry it; a re-queued key gets a new one
  uint64_t buffer_key = 0u;  // the VB/IB bindings a draw must have to serve its copies
  uint32_t vs_hash = 0u;
  uint32_t last_frame = 0u;  // queued, last sighting, or last copy issued or read
  PoolStagingDiag staging;   // the last read (poison_staging)
  DrawRecord draw;
  int32_t pos_offset = 0;
  reshade::api::format pos_format = reshade::api::format::unknown;
  int32_t uv_offset = -1;  // -1: no decodable UV
  reshade::api::format uv_format = reshade::api::format::unknown;
  PoolUvStream uv;                       // TEXCOORD0 in another slot (buffer 0: none)
  std::vector<uint8_t> vertex_bytes;     // the vertex read, held until the uv read decodes it
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
  uint64_t previous_indices_source_offset = 0u;  // the previous capture's copies (mismatch diagnostics)
  uint64_t previous_vertices_source_offset = 0u;
  uint32_t indices_frame = 0u;
  uint32_t vertices_frame = 0u;
  uint64_t indices_raw_hash = 0u;  // FNV of the bytes read (diagnostic)
  uint64_t vertices_raw_hash = 0u;
  uint32_t raw_index_min = 0u;  // raw index values, before the base vertex
  uint32_t raw_index_max = 0u;
  uint32_t first_raw[8] = {};
  std::vector<PoolCaptureRecord> records;  // one per complete capture, at most kPoolMeshMaxCaptures
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
  bool follow = false;      // moving key in follow mode: the copy bypassed the cooldown
  PoolAlphaMaterial alpha;  // indirect alpha draws: material read at the draw, flagged at resolve
  uint32_t alpha_index = UINT32_MAX;  // indirect alpha draws only: PoolStagingSlot::indirect_alpha entry
};

// Normal copies keep the per-identity cooldown. Follow and Trace bypass it.
enum class PoolCopyKind : uint8_t { Normal, Follow, Trace };

// Indirect alpha draws only (ON): the slot bindings and TEXCOORD0 stream read at the draw, kept out of
// PoolPendingCopy so every copy stays small with alpha_foliage off.
struct PoolIndirectAlpha {
  PoolUvStream uv;
  PoolVertexBuffers vbs;
  bool vbs_known = false;
  uint64_t source = 0u;  // alpha source id of the keyless copy (CapturePoolAlphaSource), 0 = none
};

struct PoolStagingSlot {
  reshade::api::resource buffer = {0u};
  uint64_t used = 0u;
  uint64_t follow_used = 0u;  // bytes of Follow copies in `used`
  bool resolving = false;
  std::vector<PoolPendingCopy> copies;
  std::vector<DrawRecord> indirect_draws;
  std::vector<PoolIndirectAlpha> indirect_alpha;  // parallel to the alpha copies' alpha_index
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
  bool follow = false;            // moving key in follow mode: copies bypass the cooldown
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
  uint32_t retired_orphans = 0u;    // instances of its key retired as orphans (RetirePoolOrphans)
  uint64_t blocked = 0u;            // admissions refused since
  uint64_t follows = 0u;            // pose updates of its instances (follow mode)
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
  // Alpha-tested foliage.
  uint64_t alpha_draws = 0u;           // alpha-tested draws seen (material read or not)
  uint64_t alpha_cb_unavailable = 0u;  // material not readable (unbound, offset, untracked): fails closed
  uint64_t alpha_conflicts = 0u;       // keys whose draws read differing materials (first readable one kept)
  uint64_t alpha_meshes_conflicted = 0u;  // meshes that took a conflicting key (refused until the conflict clears)
  uint64_t alpha_conflict_refused = 0u;   // admissions refused: mesh conflict
  uint64_t alpha_refused_off = 0u;     // admissions refused: alpha_foliage off
  uint64_t alpha_no_uv = 0u;           // admissions refused: mesh has no UVs
  uint64_t alpha_uv_stream_mismatch = 0u;  // a queued key seen with another TEXCOORD0 buffer (its request keeps the first)
  uint64_t alpha_removed = 0u;         // instances removed by the alpha flag or the switch
  uint64_t alpha_uv_requeues = 0u;     // meshes captured without UVs and queued again for them (once per key)
  uint64_t alpha_source_copies = 0u;   // source textures copied for alpha draws (CapturePoolAlphaSource)
  uint64_t alpha_source_refused = 0u;  // alpha draws whose source was not copied (texture kind, caps, deferred, failure)
  uint64_t alpha_source_refused_bytes = 0u;   // of those: the byte cap (kAlphaSourceBytesMax) was reached
  uint64_t alpha_source_refused_format = 0u;  // of those: the bound view's format is unknown or typeless
  uint64_t alpha_source_refused_view = 0u;  // of those: the bound view does not belong to the bound t0 resource
  uint64_t alpha_source_refused_cap = 0u;       // of those: the copy cap (per frame, sources) or the byte cap was reached
  uint64_t alpha_source_refused_deferred = 0u;  // of those: drawn on a deferred context (no copy there)
  uint64_t alpha_source_refused_type = 0u;      // of those: texture kind, view format, or the bound view is not the t0 texture
  uint64_t alpha_source_refused_failed = 0u;    // of those: the copy could not be made (the keys are refused)
  uint64_t alpha_indirect_copies = 0u;   // keyless copies made for indirect alpha draws (mesh_key 0)
  uint64_t alpha_orphans_attached = 0u;  // keyless copies attached to their key at resolve
  uint64_t alpha_orphans_expired = 0u;   // keyless copies freed when their hold ran out (no resolve)
  uint64_t alpha_orphan_missed = 0u;     // resolves whose keyless copy was already gone
  uint64_t alpha_indirect_source_changed = 0u;  // resolves of a key already attached to another copy (the first is kept)
  uint64_t wind_refused_off = 0u;  // wind rest-pose admissions refused: alpha_wind_opaque (with alpha_foliage) off
  uint64_t wind_rest_draws = 0u;  // wind rest-pose draws gated in (alpha_wind_opaque with alpha_foliage on)
  uint64_t follow_hits = 0u;             // moving instances moved to their new pose
  uint64_t follow_admits = 0u;           // moving poses admitted without the stable count
  uint64_t follow_misses_skipped = 0u;   // moving sightings with no copy to follow (not admitted)
  uint64_t follow_rejected_bounds = 0u;  // pose updates refused (bounds)
  uint64_t follow_budget_skips = 0u;     // follow copies refused by the slot share
  uint64_t max_slot_used = 0u;           // largest staging slot fill at resolve (bytes)
  uint64_t max_follow_used = 0u;         // largest follow part of a staging slot at resolve (bytes)
  uint64_t orphans = 0u;                 // dynamic instances not seen for kPoolMovingHoldFrames (pending retirement)
  uint64_t orphans_retired = 0u;         // orphans removed by RetirePoolOrphans
  uint64_t follow_relinks = 0u;          // moving instances relinked across a gap (FollowPoolMovingInstance)
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
  uint32_t mesh_retries = 0u;             // unstable meshes given another round (kPoolMeshRetryRounds)
  uint32_t meshes_admitted_by_retry = 0u;  // admitted after a retry round
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
  size_t mesh_retry = 0u;      // failed captures waiting for their next round
  size_t mesh_in_flight = 0u;  // copy issued, not read yet
  size_t meshes = 0u;
  size_t observed = 0u;
  size_t admitted = 0u;
  size_t region = 0u;
  size_t dynamic_live_keys = 0u;  // draw keys seen moving, still mapped
  size_t dynamic_meshes = 0u;     // meshes flagged dynamic now
  size_t following = 0u;          // dynamic instances (following their object) now
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

// Adds the CPU time of its scope to total_us (steady_clock, microseconds).
struct PoolCpuTimer {
  std::atomic_uint64_t* total_us;
  std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
  ~PoolCpuTimer() {
    total_us->fetch_add(static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(
                                                  std::chrono::steady_clock::now() - start).count()),
                        std::memory_order_relaxed);
  }
};

// The last inspect result (middle-click in a trace view), written to world_pool.json.
struct PoolInspectRecord {
  bool set = false;
  uint32_t frame = 0u;
  bool hit = false;
  uint64_t instance_id = 0u;  // PoolState instance id of the hit (0: none, or a moving instance)
  uint64_t mesh_uid = 0u;     // WorldMesh::uid of the hit (0: none, or a moving instance)
  std::vector<std::string> lines;
};

struct PoolState {
  std::atomic_bool scan_active{false};
  bool scan_continuous = false;  // the scan has been on every frame since scan_since
  uint32_t scan_since = 0u;
  // Diagnostic switches (see the header comment).
  std::atomic_bool capture_meshes{true};
  std::atomic_bool scan_indirect{true};
  std::atomic_bool log_captures{false};
  std::atomic_bool verify_meshes{true};   // admit a mesh only after two identical captures
  std::atomic_bool retry_unstable{false};  // diagnostic: unstable captures get kPoolMeshRetryRounds more rounds
  std::atomic_bool legacy_scale{false};   // instance scale limits of round 6 (0.05 .. 50)
  std::atomic_bool exclude_moving{false};  // meshes seen moving in a camera view stay out of the static pool
  std::atomic_bool alpha_foliage{true};   // alpha-tested foliage (rigid, wind rest pose); session only, on by default
  std::atomic_bool alpha_indirect_source{true};  // indirect alpha draws get a source copy (alpha_foliage on only); on by default
  std::atomic_bool alpha_wind_opaque{true};      // wind with an opaque pixel shader admitted as rest pose (alpha_foliage on only)
  std::atomic_bool follow_moving{true};    // moving meshes keep their instances, which follow the pose
  PoolInspectRecord inspect;               // last inspect result (bvh_debug.hpp), written by DumpWorldPool
  std::atomic_bool poison_staging{false};  // diagnostic: mesh staging cpu-visible, poisoned after each read
  bool staging_poisoned = false;           // how the staging ring was created (EnsurePoolStaging)
  std::array<std::vector<PoolResidue>, kPoolStagingSlots> residue;  // ClassifyPoolStagingHead
  std::array<uint64_t, kPoolStagingSlots> residue_cycle{};          // resolves with mesh copies, per slot
  uint32_t mismatch_lines_logged = 0u;
  uint32_t mesh_fail_lines_logged = 0u;  // since the last pool reset
  uint32_t orphan_lines_logged = 0u;     // per session
  uint32_t lifecycle_lines_logged = 0u;  // per session
  std::vector<std::string> pending_lines;  // flagged-moving lines, logged by the present block
  // CPU time of the pool hooks and of DrainPoolScan (PoolCpuTimer): accumulated during a frame,
  // moved to the *_frame_us values (last frame) by DrainPoolScan.
  std::atomic_uint64_t cpu_hook_us{0u};
  std::atomic_uint64_t cpu_drain_us{0u};
  std::atomic_uint64_t cpu_hook_frame_us{0u};
  std::atomic_uint64_t cpu_drain_frame_us{0u};
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
  uint64_t next_instance_id = 1u;  // WorldInstance::id, never reused (also not by ResetWorldPool)
  std::unordered_set<uint64_t> mesh_queued;   // keys queued or captured since last invalidation
  std::unordered_map<uint64_t, PoolMeshFailure> failed_meshes;
  std::unordered_map<uint64_t, PoolMeshMismatch> mismatch_meshes;  // capped at kPoolMismatchMaxMeshes
  struct {
    std::atomic_uint64_t update{0u};
    std::atomic_uint64_t update_cmd{0u};
    std::atomic_uint64_t map{0u};
    std::atomic_uint64_t copy_region{0u};
    std::atomic_uint64_t copy_resource{0u};
    std::atomic_uint64_t tracked{0u};
  } write_events;  // hook calls seen (relaxed; diagnostic)
  std::unordered_map<uint64_t, PoolMeshRetry> mesh_retry;  // unstable meshes waiting for their next round
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
  std::unordered_map<uint64_t, PoolAlphaState> alpha_keys;  // by draw key; flags its mesh alpha-tested
  bool alpha_off_applied = false;  // the switch-off removal ran (re-armed by switching on)
  bool alpha_indirect_off_applied = false;  // the indirect-source switch-off detach ran (re-armed by switching on)
  std::unordered_set<uint64_t> alpha_uv_requeued;  // keys whose mesh was requeued for its UVs
  std::set<std::pair<uint64_t, uint64_t>> pixel_unknown_logged;  // (vs, ps) pipelines logged as PixelUnknown once
  uint64_t alpha_held_bytes_max = uint64_t{64} << 20;  // vertex bytes held by requests in the Uvs phase (ON): refused above
  std::unordered_map<uint64_t, PoolAlphaSource> alpha_sources;  // by source id; no proxy while its copy is made
  std::unordered_map<uint64_t, uint64_t> alpha_source_by_texture;  // live source texture handle -> source id
  uint64_t alpha_next_source = 1u;  // source id, never reused
  std::unordered_map<uint64_t, uint64_t> alpha_key_source;  // draw key -> its source id (alpha_sources)
  bool alpha_capture_logged = false;  // the first capture after alpha_foliage was switched on is logged
  bool alpha_sync_logged = false;     // the first SyncLiveAlpha after alpha_foliage was switched on is logged
  std::unordered_set<uint64_t> alpha_source_done;  // draw keys needing no further copy (refused)
  std::unordered_set<uint64_t> alpha_indirect_keys;  // keys of indirect alpha draws attached to their source (resolve)
  PoolAlphaDumpMaps alpha_maps;  // dump-only copies of the atlas maps (SyncLiveAlpha)
  std::unordered_set<uint64_t> wind_keys;  // draw keys admitted as wind rest poses (cleared when alpha_wind_opaque goes off)
  bool wind_off_applied = false;  // the wind switch-off removal ran (re-armed by switching on)
  std::unordered_set<uint64_t> alpha_format_logged;  // texture handles whose refused format was logged
  std::atomic_bool live_on{true};  // the live BVH is enabled (UpdateLiveBvh): no source copies while it is off
  std::vector<reshade::api::resource> alpha_dead_proxies;  // proxies to free at the next present (never in a destroy event)
  uint32_t alpha_copy_frame = 0u;
  uint32_t alpha_copies_frame = 0u;  // source copies made in alpha_copy_frame
  PoolAlphaGpuStats alpha_gpu;
  uint64_t dynamic_revision = 0u;  // bumped when a moving instance changes pose (not a change of the set)
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
inline uint64_t PoolAdmittedKey(uint32_t mesh_id, uint64_t matrix_hash) {
  return PoolMix(PoolMix(1469598103934665603ull, mesh_id), matrix_hash);
}

// Caller holds g_pool.mutex. The last frame an instance was seen: admitted,
// followed, or sighted at its (mesh, matrix) observation.
inline uint32_t PoolInstanceLastSeen(const WorldInstance& instance) {
  uint32_t seen = (std::max)(instance.admit_frame, instance.last_follow_frame);
  const auto observed = g_pool.observations.find(InstanceKey{instance.mesh_key, MatrixHash(instance.matrix, kPoolWorldFloats)});
  if (observed != g_pool.observations.end()) {
    seen = (std::max)({seen, observed->second.last_frame, observed->second.last_camera_frame, observed->second.last_light_frame});
  }
  return seen;
}

// Caller holds g_pool.mutex. The newest last-seen frame of the dynamic instances, by mesh key.
inline std::unordered_map<uint64_t, uint32_t> PoolNewestSeenByKey() {
  std::unordered_map<uint64_t, uint32_t> newest;
  for (const auto& instance : g_pool.instances) {
    if (!instance.dynamic) continue;
    uint32_t& seen = newest[instance.mesh_key];
    seen = (std::max)(seen, PoolInstanceLastSeen(instance));
  }
  return newest;
}

// A dynamic instance not seen for kPoolMovingHoldFrames is an orphan (pending
// retirement) when a fresher dynamic instance of its mesh key exists (the
// pose it was superseded by), or when it was not seen for kPoolPruneAge frames.
inline bool PoolInstanceIsOrphan(const WorldInstance& instance, uint32_t frame,
                                 const std::unordered_map<uint64_t, uint32_t>& newest) {
  if (!instance.dynamic) return false;
  const uint32_t seen = PoolInstanceLastSeen(instance);
  if (frame <= seen + kPoolMovingHoldFrames) return false;
  if (frame > seen + kPoolPruneAge) return true;
  const auto it = newest.find(instance.mesh_key);
  return it != newest.end() && it->second > seen;
}

// Caller holds g_pool.mutex. Removes the instances `remove` selects (order kept),
// erases their admitted keys, and bumps the revision once if any were removed.
template <typename Remove>
inline uint32_t RemovePoolInstances(Remove remove) {
  uint32_t removed = 0u;
  size_t write = 0u;
  for (size_t read = 0u; read < g_pool.instances.size(); ++read) {
    WorldInstance& instance = g_pool.instances[read];
    if (remove(instance)) {
      g_pool.admitted_keys.erase(PoolAdmittedKey(instance.mesh_id, MatrixHash(instance.matrix, kPoolWorldFloats)));
      removed += 1u;
      continue;
    }
    if (write != read) g_pool.instances[write] = std::move(instance);
    write += 1u;
  }
  g_pool.instances.resize(write);
  if (removed != 0u) g_pool.revision += 1u;
  return removed;
}

// Caller holds g_pool.mutex. Retires the orphans: their exact-key observation no
// longer counts as admitted, and the instance is removed. No graphics calls.
inline void RetirePoolOrphans(uint32_t frame, const std::unordered_map<uint64_t, uint32_t>& newest, std::vector<std::string>* lines) {
  uint64_t first_key = 0u;
  uint32_t first_unseen = 0u;
  const uint32_t removed = RemovePoolInstances([&](const WorldInstance& instance) {
    if (!PoolInstanceIsOrphan(instance, frame, newest)) return false;
    if (first_unseen == 0u) {
      first_key = instance.mesh_key;
      first_unseen = frame - PoolInstanceLastSeen(instance);
    }
    const auto observed = g_pool.observations.find(InstanceKey{instance.mesh_key, MatrixHash(instance.matrix, kPoolWorldFloats)});
    if (observed != g_pool.observations.end()) observed->second.admitted = false;
    const auto dynamic = g_pool.dynamic_keys.find(instance.mesh_key);
    if (dynamic != g_pool.dynamic_keys.end()) dynamic->second.retired_orphans += 1u;
    return true;
  });
  g_pool.stats.orphans_retired += removed;
  if (removed != 0u && g_pool.orphan_lines_logged < kPoolLifecycleLogLines) {
    g_pool.orphan_lines_logged += 1u;
    lines->push_back(renodx::utils::log::BuildString("falcom_world::pool: frame ", frame, ": retired ", removed,
                                                     " orphan instances, first mesh ", renodx::utils::log::AsHex(first_key),
                                                     " unseen ", first_unseen, " frames"));
  }
}

inline void UpdatePoolStats(const std::unordered_map<uint64_t, uint32_t>& newest) {
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
  g_pool.stats.mesh_retry = g_pool.mesh_retry.size();
  g_pool.stats.meshes = g_pool.meshes.size();
  g_pool.stats.observed = g_pool.observations.size();
  g_pool.stats.admitted = g_pool.instances.size();
  const PoolRegion region = CurrentPoolRegion();
  size_t in_region = 0u;
  uint64_t orphans = 0u;
  const uint32_t frame = g_state.frame.load();
  size_t following = 0u;
  for (const auto& instance : g_pool.instances) {
    if (PoolInstanceInRegion(instance, region)) in_region += 1u;
    if (PoolInstanceIsOrphan(instance, frame, newest)) orphans += 1u;
    if (instance.dynamic) following += 1u;
  }
  g_pool.stats.orphans = orphans;
  g_pool.stats.following = following;
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
  const bool poison = g_pool.poison_staging.load(std::memory_order_relaxed);
  {
    std::lock_guard<std::mutex> lock(g_pool.mutex);
    if (g_pool.staging_device == device && g_pool.staging_poisoned == poison) return;
  }
  // Per slot: the instance buffer, then the mesh buffer.
  std::vector<reshade::api::resource> created;
  for (uint32_t i = 0; i < 2u * kPoolStagingSlots; ++i) {
    const reshade::api::resource_desc desc(
        (i % 2u) == 0u ? kPoolStagingSlotBytes : kPoolMeshSlotBytes,
        (i % 2u) == 0u || !poison ? reshade::api::memory_heap::gpu_to_cpu : reshade::api::memory_heap::cpu_only,
        reshade::api::resource_usage::copy_dest);
    reshade::api::resource buffer = {0u};
    if (!device->create_resource(desc, nullptr, reshade::api::resource_usage::copy_dest, &buffer)) break;
    // Poisoned mesh staging starts as the poison word, not zero: a copy that never writes its range shows it.
    void* fill = nullptr;
    if (poison && (i % 2u) == 1u && device->map_buffer_region(buffer, 0u, kPoolMeshSlotBytes,
                                                              reshade::api::map_access::write_only, &fill)
        && fill != nullptr) {
      std::fill_n(static_cast<uint32_t*>(fill), kPoolMeshSlotBytes / sizeof(uint32_t), kPoolPoisonWord);
      device->unmap_buffer_region(buffer);
    }
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
    g_pool.staging_poisoned = poison;
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
inline bool AdmitPoolInstance(ObservedInstance& observed, uint64_t mesh_key, uint32_t mesh_id, bool follow = false) {
  if (mesh_id >= g_pool.meshes.size()) return false;
  if (g_pool.wind_keys.count(mesh_key) != 0u
      && !(g_pool.alpha_foliage.load(std::memory_order_relaxed) && g_pool.alpha_wind_opaque.load(std::memory_order_relaxed))) {
    g_pool.stats.wind_refused_off += 1u;
    return false;
  }
  if (g_pool.meshes[mesh_id].alpha) {
    // Alpha-tested foliage: refused with alpha_foliage off, in a conflict, or without UVs. Otherwise admitted;
    // the TLAS takes it once its material has an atlas slice (alpha_waiting, alpha_live.hpp).
    if (!g_pool.alpha_foliage.load(std::memory_order_relaxed)) {
      g_pool.stats.alpha_refused_off += 1u;
      return false;
    }
    if (g_pool.meshes[mesh_id].alpha_state.conflict) {
      g_pool.stats.alpha_conflict_refused += 1u;
      return false;
    }
    if (g_pool.meshes[mesh_id].uvs.empty()) {
      g_pool.stats.alpha_no_uv += 1u;
      return false;
    }
  }
  if (g_pool.meshes[mesh_id].dynamic && g_pool.exclude_moving.load(std::memory_order_relaxed)) {
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
  instance.id = g_pool.next_instance_id++;
  instance.dynamic = follow;
  std::memcpy(instance.matrix, observed.matrix, sizeof(float) * kPoolWorldFloats);
  ComputeMatrixInverse(instance.matrix, instance.inverse_world);
  instance.first_frame = observed.first_frame;
  instance.admit_frame = g_state.frame.load();
  instance.last_follow_frame = instance.admit_frame;
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

// Caller holds g_pool.mutex. Follow mode: moving objects stay in the pool.
inline bool PoolFollowMode() {
  return g_pool.follow_moving.load(std::memory_order_relaxed) && !g_pool.exclude_moving.load(std::memory_order_relaxed);
}

// Caller holds g_pool.mutex. Clears the admitted flag of the observations that map to a mesh.
inline void ClearPoolMeshAdmitted(uint32_t mesh_id) {
  for (auto& [key, observed] : g_pool.observations) {
    if (!observed.admitted) continue;
    const auto mesh_it = g_pool.mesh_by_key.find(key.mesh_key);
    if (mesh_it != g_pool.mesh_by_key.end() && mesh_it->second == mesh_id) observed.admitted = false;
  }
}

// Caller holds g_pool.mutex. Flags a mesh dynamic and removes its admitted
// instances (their observations no longer count as admitted; AdmitPoolInstance
// refuses them while the flag is set). Returns the instances removed.
inline uint32_t MarkPoolMeshDynamic(uint32_t mesh_id) {
  if (mesh_id >= g_pool.meshes.size() || g_pool.meshes[mesh_id].dynamic) return 0u;
  g_pool.meshes[mesh_id].dynamic = true;
  g_pool.stats.dynamic_meshes_marked += 1u;
  const uint32_t removed = RemovePoolInstances([mesh_id](const WorldInstance& instance) { return instance.mesh_id == mesh_id; });
  ClearPoolMeshAdmitted(mesh_id);
  g_pool.stats.dynamic_retired += removed;
  if (removed != 0u && g_pool.lifecycle_lines_logged < kPoolLifecycleLogLines) {
    g_pool.lifecycle_lines_logged += 1u;
    g_pool.pending_lines.push_back(renodx::utils::log::BuildString("falcom_world::pool: frame ", g_state.frame.load(), ": mesh ",
                                                                   renodx::utils::log::AsHex(g_pool.meshes[mesh_id].mesh_key),
                                                                   " flagged moving, removed ", removed, " instances, follow mode ",
                                                                   PoolFollowMode() ? "on" : "off"));
  }
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
  const bool on = g_pool.exclude_moving.load(std::memory_order_relaxed) || g_pool.follow_moving.load(std::memory_order_relaxed);
  if (on == g_pool.dynamic_applied) return;
  g_pool.dynamic_applied = on;
  if (on) {
    for (auto& [key, entry] : g_pool.dynamic_keys) ApplyPoolDynamicKey(key, entry);
  } else {
    for (WorldMesh& mesh : g_pool.meshes) mesh.dynamic = false;
  }
}

// Caller holds g_pool.mutex. Removes the admitted instances of an alpha-tested mesh and
// clears their observations' admitted flag (AdmitPoolInstance refuses them while off).
inline uint32_t RetirePoolMeshAlpha(uint32_t mesh_id) {
  const uint32_t removed = RemovePoolInstances([mesh_id](const WorldInstance& instance) { return instance.mesh_id == mesh_id; });
  ClearPoolMeshAdmitted(mesh_id);
  g_pool.stats.alpha_removed += removed;
  return removed;
}

// Which parts of an incoming material differ from the first one (kPoolAlphaDiff* bits).
inline uint8_t PoolAlphaDiff(const PoolAlphaMaterial& first, const PoolAlphaMaterial& incoming) {
  uint8_t fields = 0u;
  if (first.view != incoming.view) {
    fields |= kPoolAlphaDiffView;
    if (first.resource != incoming.resource) fields |= kPoolAlphaDiffResource;
  }
  if (first.swizzle != incoming.swizzle) fields |= kPoolAlphaDiffSwizzle;
  if (std::memcmp(&first.threshold, &incoming.threshold, sizeof(float)) != 0) fields |= kPoolAlphaDiffThreshold;
  if (std::memcmp(first.scroll, incoming.scroll, sizeof(first.scroll)) != 0) fields |= kPoolAlphaDiffScroll;
  return fields;
}

// Caller holds g_pool.mutex. Records conflict fields; the first conflict keeps its incoming
// material. Returns true when a conflict is newly set.
inline bool SetPoolAlphaConflict(PoolAlphaMaterialState* held, uint8_t fields, const PoolAlphaMaterial& incoming) {
  if (fields == 0u) return false;
  const bool newly = held->conflict_fields == 0u;
  if (newly) held->conflict_incoming = incoming;
  held->conflict_fields |= fields;
  held->conflict = true;
  return newly;
}

// Caller holds g_pool.mutex. Folds an incoming material into a key's or a mesh's state. An
// unreadable material (texture 0) never conflicts; an empty state takes the first readable one;
// a differing one sets conflict (bitwise float compare: a NaN threshold equals itself).
// Returns true when conflict is newly set.
inline bool AbsorbPoolAlphaMaterial(PoolAlphaMaterialState* held, const PoolAlphaMaterial& incoming) {
  if (incoming.view == 0u) return false;
  if (held->material.view == 0u) {
    held->material = incoming;
    return false;
  }
  const uint8_t fields = PoolAlphaDiff(held->material, incoming);
  return (fields & 15u) != 0u && SetPoolAlphaConflict(held, fields, incoming);
}

// Caller holds g_pool.mutex. Folds a key's state into a mesh's state: its material, its conflict
// fields (the first conflict and its material are kept) and its layout when the mesh has none.
// Returns true when conflict is newly set.
inline bool FoldPoolAlphaState(PoolAlphaMaterialState* held, const PoolAlphaMaterialState& key_state) {
  const bool newly_material = AbsorbPoolAlphaMaterial(held, key_state.material);
  const bool newly_fields = SetPoolAlphaConflict(held, key_state.conflict_fields, key_state.conflict_incoming);
  return newly_material || newly_fields;
}

// Caller holds g_pool.mutex. Flags a mesh alpha-tested (MarkPoolMeshDynamic's pattern) and folds
// its key's state into the mesh. Removes its instances when it is newly flagged or newly conflicted.
inline uint32_t MarkPoolMeshAlpha(uint32_t mesh_id, const PoolAlphaState& key_state) {
  if (mesh_id >= g_pool.meshes.size()) return 0u;
  WorldMesh& mesh = g_pool.meshes[mesh_id];
  const bool newly_flagged = !mesh.alpha;
  mesh.alpha = true;
  const bool newly_conflicted = FoldPoolAlphaState(&mesh.alpha_state, key_state);
  if (newly_conflicted) g_pool.stats.alpha_meshes_conflicted += 1u;
  if (!newly_flagged && !newly_conflicted) return 0u;
  return RetirePoolMeshAlpha(mesh_id);
}

// Caller holds g_pool.mutex. With alpha_foliage off, removes the instances of every
// alpha-tested mesh once per switch-off.
inline void ApplyPoolAlphaSwitch() {
  if (g_pool.alpha_foliage.load(std::memory_order_relaxed)) {
    g_pool.alpha_off_applied = false;
    return;
  }
  if (g_pool.alpha_off_applied) return;
  g_pool.alpha_off_applied = true;
  for (uint32_t mesh_id = 0u; mesh_id < g_pool.meshes.size(); ++mesh_id) {
    if (g_pool.meshes[mesh_id].alpha) RetirePoolMeshAlpha(mesh_id);
  }
}

void RequeuePoolMeshKey(uint64_t mesh_key);

// Caller holds g_pool.mutex. Folds a draw's material into its key (a key whose draws read
// differing materials is conflicted) and into the key's mesh when the mesh exists. A key
// flagged before its mesh is captured is flagged at the capture (ApplyPoolMesh). With
// alpha_foliage on, a mesh captured without UVs (captured before its key was flagged) is
// requeued once for its UVs (alpha_uv_requeued).
inline void NotePoolAlphaMaterial(uint64_t mesh_key, const PoolAlphaMaterial& material, const PoolVertexBuffers* vbs = nullptr) {
  PoolAlphaState& key_state = g_pool.alpha_keys[mesh_key];
  if (vbs != nullptr && !key_state.layout.vbs_known) {
    key_state.layout.vbs = *vbs;
    key_state.layout.vbs_known = true;
  }
  if (AbsorbPoolAlphaMaterial(&key_state, material)) g_pool.stats.alpha_conflicts += 1u;
  const auto mesh_it = g_pool.mesh_by_key.find(mesh_key);
  if (mesh_it == g_pool.mesh_by_key.end()) return;
  MarkPoolMeshAlpha(mesh_it->second, key_state);
  if (g_pool.alpha_foliage.load(std::memory_order_relaxed) && material.view != 0u
      && g_pool.meshes[mesh_it->second].uvs.empty() && g_pool.alpha_uv_requeued.insert(mesh_key).second) {
    RequeuePoolMeshKey(mesh_key);
    g_pool.stats.alpha_uv_requeues += 1u;
  }
}

// Reads the alpha material a pixel shader uses (t0 texture, b5 threshold at threshold_offset
// and scroll, b10 swizzle) for a draw whose pixel shader carries kTraitAlphaMaterial. False when
// any part is not readable; the caller then flags the key with an empty material
// (fails closed). Counts the draw in the alpha stats.
inline bool ReadPoolAlphaDraw(reshade::api::command_list* cmd_list, const WorldCommandListData& cl_data, uint32_t threshold_offset,
                              PoolAlphaMaterial* out) {
  const reshade::api::resource material = cl_data.ps_cb[contract::kAlphaMaterialSlot];
  const reshade::api::resource swizzle = cl_data.ps_cb[contract::kAlphaSwizzleSlot];
  out->view = cl_data.ps_srv_view[contract::kAlphaTexSlot].handle;
  out->resource = cl_data.ps_srv[contract::kAlphaTexSlot].handle;
  bool readable = out->view != 0u && material.handle != 0u && cl_data.ps_cb_offset[contract::kAlphaMaterialSlot] == 0u;
  std::array<uint8_t, kTrackedCbBytes> bytes = {};
  readable = readable && ReadTrackedCbBytes(cmd_list, material, bytes.data(), threshold_offset + sizeof(float));
  if (readable) {
    std::memcpy(&out->threshold, bytes.data() + threshold_offset, sizeof(float));
    std::memcpy(out->scroll, bytes.data() + contract::kAlphaUvScrollOffset, sizeof(out->scroll));
    if (swizzle.handle != 0u) {
      readable = cl_data.ps_cb_offset[contract::kAlphaSwizzleSlot] == 0u
                 && ReadTrackedCbBytes(cmd_list, swizzle, &out->swizzle, sizeof(uint32_t));
      out->swizzle &= 1u;  // the shader reads bit 0 only (other bits must not make a conflict)
    }
  }
  std::lock_guard<std::mutex> lock(g_pool.mutex);
  g_pool.stats.alpha_draws += 1u;
  if (!readable) {
    g_pool.stats.alpha_cb_unavailable += 1u;
    *out = {};
  }
  return readable;
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

// Caller holds g_pool.mutex. Follow mode, a camera sighting of a mesh flagged
// dynamic: the instance at prevWorld moves to world (its pose follows the
// object). Without one, a moving pose is admitted directly when a copy of its
// key may follow it; otherwise it is not admitted.
// Translation distance of two 4x3 matrices (row-major, translation in [3], [7], [11]).
inline float PoolTranslationDistance(const float* a, const float* b) {
  const float dx = a[3] - b[3], dy = a[7] - b[7], dz = a[11] - b[11];
  return std::sqrt(dx * dx + dy * dy + dz * dz);
}

inline void FollowPoolMovingInstance(uint64_t mesh_key, uint32_t vs_hash, const float* world, const float* prev_world,
                                     bool copy_follow, uint32_t frame, PoolSightingVerdict verdict) {
  if (!PoolFollowMode() || verdict == PoolSightingVerdict::Unknown) return;
  const auto mesh_it = g_pool.mesh_by_key.find(mesh_key);
  if (mesh_it == g_pool.mesh_by_key.end() || mesh_it->second >= g_pool.meshes.size()) return;
  const uint32_t mesh_id = mesh_it->second;
  if (!g_pool.meshes[mesh_id].dynamic) return;
  if (std::memcmp(world, prev_world, sizeof(float) * kPoolWorldFloats) != 0) {
    // One scan picks the target: the instance at prevWorld, else (Moving only) the
    // nearest relink candidate of this key that was followed within the window.
    WorldInstance* target = nullptr;
    WorldInstance* relink = nullptr;
    float relink_distance = 3.0e38f;
    const float step = PoolTranslationDistance(world, prev_world);
    const bool world_finite = std::isfinite(world[3]) && std::isfinite(world[7]) && std::isfinite(world[11]);
    for (WorldInstance& instance : g_pool.instances) {
      if (instance.mesh_id != mesh_id) continue;
      if (std::memcmp(instance.matrix, prev_world, sizeof(float) * kPoolWorldFloats) == 0) {
        target = &instance;
        break;
      }
      if (!world_finite || verdict != PoolSightingVerdict::Moving || !instance.dynamic || instance.mesh_key != mesh_key) continue;
      if (instance.last_follow_frame >= frame || frame - instance.last_follow_frame > kPoolFollowRelinkFrames) continue;
      const float gap = static_cast<float>(frame - instance.last_follow_frame);
      const float limit = (gap + 1.f) * (std::max)(step, 0.02f) * 1.5f + 0.1f;
      const float distance = PoolTranslationDistance(instance.matrix, world);
      if (distance <= limit && distance < relink_distance) {
        relink = &instance;
        relink_distance = distance;
      }
    }
    WorldInstance* picked = target != nullptr ? target : relink;
    if (picked != nullptr) {
      float bounds_min[3] = {};
      float bounds_max[3] = {};
      if (!ComputePoolInstanceBounds(g_pool.meshes[mesh_id], world, bounds_min, bounds_max)) {
        g_pool.stats.follow_rejected_bounds += 1u;
        return;
      }
      WorldInstance& instance = *picked;
      const uint64_t old_hash = MatrixHash(instance.matrix, kPoolWorldFloats);
      const uint64_t new_hash = MatrixHash(world, kPoolWorldFloats);
      g_pool.admitted_keys.erase(PoolAdmittedKey(mesh_id, old_hash));
      g_pool.admitted_keys.insert(PoolAdmittedKey(mesh_id, new_hash));
      std::memcpy(instance.matrix, world, sizeof(float) * kPoolWorldFloats);
      ComputeMatrixInverse(instance.matrix, instance.inverse_world);
      std::memcpy(instance.bounds_min, bounds_min, sizeof(bounds_min));
      std::memcpy(instance.bounds_max, bounds_max, sizeof(bounds_max));
      if (!instance.dynamic) g_pool.revision += 1u;  // a new dynamic instance is re-partitioned by a rebuild
      instance.dynamic = true;
      instance.last_follow_frame = frame;
      instance.follows += 1u;
      const auto dynamic = g_pool.dynamic_keys.find(mesh_key);
      if (dynamic != g_pool.dynamic_keys.end()) dynamic->second.follows += 1u;
      // The observation of the old pose continues at the new one. If the new pose
      // already has an observation, insert() hands the node back and the old counts
      // are dropped (observation re-key semantics are unchanged).
      auto node = g_pool.observations.extract(InstanceKey{mesh_key, old_hash});
      if (!node.empty()) {
        node.key() = InstanceKey{mesh_key, new_hash};
        ObservedInstance& observed = node.mapped();
        std::memcpy(observed.matrix, world, sizeof(float) * kPoolWorldFloats);
        observed.admitted = true;
        observed.first_frame = frame;
        g_pool.observations.insert(std::move(node));
      }
      if (target == nullptr) g_pool.stats.follow_relinks += 1u;
      g_pool.dynamic_revision += 1u;
      g_pool.stats.follow_hits += 1u;
      return;
    }
  }
  if (verdict != PoolSightingVerdict::Moving) return;
  if (!copy_follow) {
    g_pool.stats.follow_misses_skipped += 1u;
    return;
  }
  const uint64_t matrix_hash = MatrixHash(world, kPoolWorldFloats);
  if (g_pool.admitted_keys.count(PoolAdmittedKey(mesh_id, matrix_hash)) != 0u) return;
  ObservedInstance& observed = g_pool.observations[InstanceKey{mesh_key, matrix_hash}];
  if (observed.rejected) return;
  if (observed.count == 0u) observed.first_frame = frame;
  observed.vs_hash = vs_hash;
  std::memcpy(observed.matrix, world, sizeof(float) * kPoolWorldFloats);
  if (AdmitPoolInstance(observed, mesh_key, mesh_id, true)) g_pool.stats.follow_admits += 1u;
}

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

inline std::string PoolHashText(uint32_t hash);

struct PoolDrawGate {
  contract::VsClass vs_class = contract::VsClass::Unclassified;
  PoolSkip skip = PoolSkip::None;
  PoolDrawState state = PoolDrawState::Ok;
  uint8_t pass = 0u;  // kPoolPass* bits, set when the draw passes the gate
  bool alpha_material = false;  // alpha-tested pixel shader on a rigid or wind vertex shader (recorded even when skipped)
  bool wind_rest = false;  // wind VS with an opaque PS admitted as its rest pose (alpha_wind_opaque with alpha_foliage on)
  uint32_t alpha_threshold_offset = 0u;  // the pixel shader's threshold offset in b5 (alpha_material only)
  const char* unknown_reason = nullptr;  // PixelUnknown only: logged once per pipeline pair (NotePoolPixelUnknown)
};

// Shader classes first, then draw state, then the pixel shader.
inline PoolDrawGate GatePoolDraw(const DrawRecord& draw, bool check_index_count) {
  PoolDrawGate gate;
  const contract::ShaderTraits vs_traits = contract::LookupVertexTraits(draw.vs_pipeline);
  gate.vs_class = static_cast<contract::VsClass>(vs_traits.cls);
  // Wind foliage is captured as its rest pose (instance world, no sway) only while alpha_foliage is on.
  const bool alpha_on = g_pool.alpha_foliage.load(std::memory_order_relaxed);
  const bool wind_admit = alpha_on && g_pool.alpha_wind_opaque.load(std::memory_order_relaxed);
  if (gate.vs_class != contract::VsClass::Rigid && (gate.vs_class != contract::VsClass::Wind || !alpha_on)) {
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
  // The material is read only for a pixel shader the classifier gave the material layout (kTraitAlphaMaterial).
  const bool alpha_class = ps_class == contract::PsClass::AlphaTested;
  const bool alpha = alpha_class && alpha_on && (ps_traits.flags & contract::kTraitAlphaMaterial) != 0u;
  gate.alpha_material = alpha;
  if (alpha) gate.alpha_threshold_offset = static_cast<uint32_t>(ps_traits.alpha_offset);
  if (alpha_class && !alpha_on) {
    gate.skip = PoolSkip::AlphaTested;
  } else if (alpha_class && !alpha) {
    gate.skip = PoolSkip::PixelUnknown;
  } else if (!alpha_class && ps_class != contract::PsClass::Opaque) {
    gate.skip = PoolSkip::PixelUnknown;
  } else if (gate.vs_class == contract::VsClass::Wind && !alpha) {
    if (wind_admit) {
      gate.wind_rest = true;
    } else {
      gate.skip = PoolSkip::WindOpaque;
    }
  }
  if (gate.skip == PoolSkip::PixelUnknown) {
    gate.unknown_reason = alpha_class ? contract::AlphaMaterialReasonName(static_cast<contract::AlphaMaterialReason>(ps_traits.alpha_reason))
                                      : contract::PsClassName(ps_class);
  }
  if (gate.skip == PoolSkip::None) {
    gate.pass = static_cast<uint8_t>(((vs_traits.flags & contract::kTraitCameraView) != 0u ? kPoolPassCamera : 0u)
                                     | ((vs_traits.flags & contract::kTraitLightView) != 0u ? kPoolPassLight : 0u)
                                     | ((vs_traits.flags & contract::kTraitVisibilityLayout) != 0u ? kPoolPassVisibility : 0u)
                                     | ((ps_traits.flags & contract::kTraitNearFade) != 0u ? kPoolPassNearFadePs : 0u)
                                     | (alpha ? kPoolPassAlpha : 0u) | (gate.wind_rest ? kPoolPassWind : 0u));
  }
  return gate;
}

// Caller holds g_pool.mutex. Logs a PixelUnknown pipeline pair once per session.
inline void NotePoolPixelUnknown(const DrawRecord& draw, const char* reason) {
  if (!g_pool.pixel_unknown_logged.insert({draw.vs_pipeline, draw.ps_pipeline}).second) return;
  renodx::utils::log::i("[world-bvh] pixel_unknown: vs=", PoolHashText(draw.vs_hash), " ps=", PoolHashText(draw.ps_hash),
                        " reason=", reason);
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

// Caller holds g_pool.mutex. Normal copies apply the per-identity cooldown; all
// kinds respect the slot; Follow is also limited to its share of the slot. On
// None, `*out_schedule` is the identity to stamp after copying.
inline PoolSkip ReservePoolCopy(
    reshade::api::device* device,
    uint64_t schedule_key,
    uint32_t frame,
    uint64_t bytes,
    PoolSchedule** out_schedule,
    PoolCopyKind kind) {
  PoolSchedule& schedule = g_pool.schedule[schedule_key];
  const PoolStagingSlot& slot = g_pool.slots[g_pool.write_slot];
  if (kind == PoolCopyKind::Normal && schedule.next_frame != 0u && schedule.copy_frame != frame && frame < schedule.next_frame) {
    return PoolSkip::Cooldown;
  }
  if (g_pool.staging_device != device || slot.buffer.handle == 0u || slot.resolving) return PoolSkip::NoStaging;
  if (slot.used + bytes > kPoolStagingSlotBytes) return PoolSkip::BudgetFull;
  if (kind == PoolCopyKind::Follow && slot.follow_used + bytes > kPoolStagingSlotBytes / kPoolFollowSlotShare) {
    g_pool.stats.follow_budget_skips += 1u;
    return PoolSkip::BudgetFull;
  }
  *out_schedule = &schedule;
  return PoolSkip::None;
}

// The VB/IB bindings of a draw (and its TEXCOORD0 stream, when it has one): any draw that has them
// bound can serve the mesh copies of every queued mesh drawn from them.
inline uint64_t PoolBufferKey(const DrawRecord& draw, const PoolUvStream& uv) {
  uint64_t key = 1469598103934665603ull;
  key = PoolMix(key, draw.vb.handle);
  key = PoolMix(key, draw.vb_offset);
  key = PoolMix(key, draw.vb_stride);
  key = PoolMix(key, draw.ib.handle);
  key = PoolMix(key, draw.ib_offset);
  key = PoolMix(key, draw.index_size);
  if (uv.buffer.handle == 0u) return key;
  key = PoolMix(key, uv.buffer.handle);
  key = PoolMix(key, uv.offset);
  key = PoolMix(key, uv.stride);
  return PoolMix(key, uv.element_offset);
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

// Caller holds g_pool.mutex. Unmaps one draw key from its mesh and drops its pending capture;
// the mesh is retired (with its instances) once no draw key refers to it any more.
inline void UnmapPoolMeshKey(uint64_t mesh_key) {
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

// Caller holds g_pool.mutex. Captures a draw key again: its mesh is unmapped and the key is
// queued on its next draw. Keeps the key's alpha material, observations and resource keys.
inline void RequeuePoolMeshKey(uint64_t mesh_key) {
  g_pool.mesh_queued.erase(mesh_key);
  g_pool.failed_meshes.erase(mesh_key);
  g_pool.mesh_retry.erase(mesh_key);
  g_pool.alpha_source_done.erase(mesh_key);  // the new capture takes its own source copy
  UnmapPoolMeshKey(mesh_key);
}

// Caller holds g_pool.mutex. Records a mesh failure; the key stays queued
// (not retried) until its buffers are released.
inline void RecordPoolMeshFailure(const PoolMeshRequest& request, const char* reason) {
  PoolMeshFailure& failure = g_pool.failed_meshes[request.mesh_key];
  failure.reason = reason;
  failure.vs_hash = request.vs_hash;
  failure.frame = g_state.frame.load();
  failure.captures = request.captures;
  failure.index_count = request.draw.index_count;
  failure.first_index = request.draw.first_index;
  failure.base_vertex = request.draw.vertex_offset;
  failure.vertex_stride = request.draw.vb_stride;
  failure.min_vertex = request.min_vertex;
  failure.max_vertex = request.max_vertex;
  failure.vb_size = request.draw.vb_size;
  failure.ib_size = request.draw.ib_size;
  failure.from_indirect = request.from_indirect;
  failure.pos_offset = request.pos_offset;
  failure.vb_usage = request.draw.vb_usage;
  failure.vb_flags = request.draw.vb_flags;
  failure.ib_usage = request.draw.ib_usage;
  failure.ib_flags = request.draw.ib_flags;
  failure.written = request.written;
  failure.staging = request.staging;
  failure.logged = false;
  failure.history = request.records;
  failure.phase = request.phase;
  failure.index_size = request.draw.index_size;
  failure.instance_count = request.draw.instance_count;
  failure.vb_handle = request.draw.vb.handle;
  failure.ib_handle = request.draw.ib.handle;
  failure.vb_offset = request.draw.vb_offset;
  failure.ib_offset = request.draw.ib_offset;
  failure.buffer_key = request.buffer_key;
  failure.raw_index_min = request.raw_index_min;
  failure.raw_index_max = request.raw_index_max;
  std::memcpy(failure.first_raw, request.first_raw, sizeof(failure.first_raw));
  failure.raw_hash = request.phase != PoolMeshPhase::Indices ? request.vertices_raw_hash : request.indices_raw_hash;
  g_pool.stats.mesh_failures += 1u;
  g_pool.stats.last_mesh_error = reason;
}

// Caller holds g_pool.mutex, and has already taken the key out of
// mesh_waiting (or it is in flight).
inline void FailPoolMesh(std::unordered_map<uint64_t, PoolMeshRequest>::iterator request, const char* reason) {
  RecordPoolMeshFailure(request->second, reason);
  g_pool.mesh_requests.erase(request);
}

// The input layout a draw's vertex input was created with. False when not seen.
inline bool FindPoolInputLayout(const DrawRecord& draw, renodx::utils::scene::InputLayoutInfo* out) {
  namespace scene = renodx::utils::scene;
  if (scene::shared.data == nullptr || draw.input_layout.handle == 0u) return false;
  bool found = false;
  scene::shared.data->input_layouts.if_contains(draw.input_layout.handle, [&](const auto& pair) {
    *out = pair.second;
    found = true;
  });
  return found;
}

// UV layout of a draw key's vertex input: the first TEXCOORD0 in any slot (semantic case-insensitive,
// as BuildMeshLayout). The verdict says why a mesh cannot take its UVs from this layout.
// The first TEXCOORD0 in any slot (semantic case-insensitive, as BuildMeshLayout), or null.
inline const renodx::utils::scene::InputElementCopy* FindPoolTexcoord0(const renodx::utils::scene::InputLayoutInfo& info) {
  for (const auto& element : info.elements) {
    std::string semantic = element.semantic.c_str();
    for (auto& c : semantic) c = static_cast<char>(::toupper(static_cast<unsigned char>(c)));
    if (semantic == "TEXCOORD" && element.semantic_index == 0u) return &element;
  }
  return nullptr;
}

inline void ClassifyPoolUvLayout(const renodx::utils::scene::InputLayoutInfo& info, uint32_t stride, PoolAlphaLayout* out) {
  namespace scene = renodx::utils::scene;
  out->valid = true;
  out->vertex_stride = stride;
  out->element_total = static_cast<uint32_t>(info.elements.size());
  for (size_t i = 0; i < info.elements.size(); ++i) {
    const scene::InputElementCopy& element = info.elements[i];
    if (i < kPoolLayoutElementsMax) {
      PoolLayoutElement& dst = out->elements[i];
      std::snprintf(dst.semantic, sizeof(dst.semantic), "%s", element.semantic.c_str());
      dst.index = element.semantic_index;
      dst.slot = element.buffer_binding;
      dst.offset = element.offset;
      dst.format = static_cast<uint32_t>(element.format);
    }
  }
  const scene::InputElementCopy* uv = FindPoolTexcoord0(info);
  out->uv_exists = uv != nullptr;
  if (uv == nullptr) {
    out->verdict = PoolUvVerdict::NoTexcoord;
    return;
  }
  out->uv_slot = uv->buffer_binding;
  out->uv_offset = uv->offset;
  out->uv_format = static_cast<uint32_t>(uv->format);
  const scene::FormatInfo* uv_info = scene::FindFormatInfo(uv->format);
  if (uv->buffer_binding != 0u) {
    out->verdict = PoolUvVerdict::TexcoordOtherSlot;
  } else if (uv_info == nullptr) {
    out->verdict = PoolUvVerdict::TexcoordUnsupportedFormat;
  } else if (static_cast<uint64_t>(uv->offset) + uv_info->byte_size > stride) {
    out->verdict = PoolUvVerdict::TexcoordBeyondStride;
  } else {
    out->verdict = PoolUvVerdict::Ok;
  }
}

// Position layout of a draw's vertex stream 0, from its input layout.
// Returns nullptr when usable, else why not.
inline const char* ResolvePoolMeshLayout(
    const DrawRecord& draw, int32_t* pos_offset, reshade::api::format* pos_format, int32_t* uv_offset, reshade::api::format* uv_format) {
  namespace scene = renodx::utils::scene;
  if (draw.method != 1u || !draw.has_index_buffer || draw.vb.handle == 0u || draw.ib.handle == 0u) {
    return "not an indexed draw";
  }
  if (draw.index_size != 2u && draw.index_size != 4u) return "unsupported index size";
  if (draw.index_count < 3u || (draw.index_count % 3u) != 0u) return "index count is not a triangle list";
  if (draw.vb_stride == 0u) return "vertex stride is zero";
  scene::InputLayoutInfo layout_info;
  if (!FindPoolInputLayout(draw, &layout_info)) return "no input layout";
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
  // A UV the vertex does not fully hold is left out (its meshes are not alpha-testable).
  const scene::FormatInfo* uv_info = layout.uv_off >= 0 ? scene::FindFormatInfo(layout.uv_format) : nullptr;
  *uv_offset = -1;
  *uv_format = reshade::api::format::unknown;
  if (uv_info != nullptr && static_cast<uint64_t>(layout.uv_off) + uv_info->byte_size <= draw.vb_stride) {
    *uv_offset = layout.uv_off;
    *uv_format = layout.uv_format;
  }
  return nullptr;
}

// The vertex buffer slots bound at a draw (the scene's command-list state). False when that state is not
// available (the slots are not known; an indirect draw's are read at its draw).
inline bool CollectPoolVertexBuffers(reshade::api::command_list* cmd_list, PoolVertexBuffers* out) {
  const auto* scene_cl = renodx::utils::data::Get<renodx::utils::scene::SceneCommandListData>(cmd_list);
  if (scene_cl == nullptr) return false;
  for (uint32_t slot = 0u; slot < scene_cl->vertex_buffers.size() && slot < kPoolVbSlotsMax; ++slot) {
    const auto& binding = scene_cl->vertex_buffers[slot];
    out->slots[out->count++] = {slot, binding.handle.handle, binding.stride, binding.offset};
  }
  return true;
}

// The TEXCOORD0 stream of a draw when it is bound in a slot other than 0. Empty (no UVs) when the slot,
// the element or the buffer does not fit. Reads the buffer size: call it before the pool lock.
inline void ResolvePoolUvStream(reshade::api::device* device, const DrawRecord& draw, const PoolVertexBuffers& vbs, PoolUvStream* out) {
  namespace scene = renodx::utils::scene;
  *out = {};
  scene::InputLayoutInfo layout_info;
  if (!FindPoolInputLayout(draw, &layout_info)) return;
  const scene::InputElementCopy* uv = FindPoolTexcoord0(layout_info);
  if (uv == nullptr || uv->buffer_binding == 0u) return;
  const scene::FormatInfo* info = scene::FindFormatInfo(uv->format);
  for (uint32_t i = 0u; i < vbs.count; ++i) {
    const PoolVertexBufferSlot& slot = vbs.slots[i];
    if (slot.slot != uv->buffer_binding) continue;
    if (info == nullptr || slot.handle == 0u || slot.stride == 0u || uv->offset + info->byte_size > slot.stride) return;
    out->buffer = {slot.handle};
    out->offset = slot.offset;
    out->stride = slot.stride;
    out->element_offset = uv->offset;
    out->format = uv->format;
    out->size = device->get_resource_desc(out->buffer).buffer.size;
    return;
  }
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

inline void QueuePoolMesh(uint64_t mesh_key, uint32_t vs_hash, const DrawRecord& draw, bool from_indirect, const PoolUvStream& uv) {
  const auto alpha_key = g_pool.alpha_keys.find(mesh_key);
  if (alpha_key != g_pool.alpha_keys.end() && !alpha_key->second.layout.valid) {
    renodx::utils::scene::InputLayoutInfo layout_info;
    if (FindPoolInputLayout(draw, &layout_info)) ClassifyPoolUvLayout(layout_info, draw.vb_stride, &alpha_key->second.layout);
  }
  const auto retry = g_pool.mesh_retry.find(mesh_key);
  if (retry != g_pool.mesh_retry.end() && g_state.frame.load() < retry->second.next_frame) return;
  if (!g_pool.mesh_queued.insert(mesh_key).second) {
    // Seen again: a waiting mesh is still drawn, so it does not expire.
    const auto request = g_pool.mesh_requests.find(mesh_key);
    if (request != g_pool.mesh_requests.end()) {
      request->second.last_frame = g_state.frame.load();
      if (request->second.uv.buffer.handle != uv.buffer.handle) g_pool.stats.alpha_uv_stream_mismatch += 1u;
    }
    return;
  }
  if (g_pool.mesh_by_key.count(mesh_key) != 0u || g_pool.failed_meshes.count(mesh_key) != 0u) return;
  AddPoolResourceKey(draw.vb.handle, mesh_key);
  if (draw.ib.handle != draw.vb.handle) AddPoolResourceKey(draw.ib.handle, mesh_key);
  if (uv.buffer.handle != 0u && uv.buffer.handle != draw.vb.handle && uv.buffer.handle != draw.ib.handle) {
    AddPoolResourceKey(uv.buffer.handle, mesh_key);  // destroying the UV buffer drops the request
  }

  PoolMeshRequest request;
  request.mesh_key = mesh_key;
  request.vs_hash = vs_hash;
  request.last_frame = g_state.frame.load();
  request.draw = draw;
  request.from_indirect = from_indirect;
  const char* error = ResolvePoolMeshLayout(draw, &request.pos_offset, &request.pos_format, &request.uv_offset, &request.uv_format);
  if (error != nullptr) {
    RecordPoolMeshFailure(request, error);
    return;
  }
  request.serial = g_pool.next_mesh_serial++;
  request.uv = uv;
  request.buffer_key = PoolBufferKey(draw, uv);
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
    const PoolUvStream& uv,
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
  const auto waiting = g_pool.mesh_waiting.find(PoolBufferKey(draw, uv));
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
    const bool uvs = request.phase == PoolMeshPhase::Uvs;
    if (uvs && uv.buffer.handle == 0u) {  // no stream for this draw: never copy from a null source
      keys.erase(keys.begin() + static_cast<std::ptrdiff_t>(index));
      continue;
    }
    const reshade::api::resource source = indices ? draw.ib : uvs ? uv.buffer : draw.vb;
    const uint64_t buffer_size = indices ? draw.ib_size : uvs ? uv.size : draw.vb_size;
    // The UV stream is read over the same vertex range, at its own offset and stride.
    const uint64_t vertex_base = uvs ? request.uv.offset : queued.vb_offset;
    const uint64_t vertex_stride = uvs ? request.uv.stride : queued.vb_stride;
    const uint64_t begin =
        indices ? queued.ib_offset + static_cast<uint64_t>(queued.first_index) * queued.index_size
                : vertex_base + static_cast<uint64_t>(request.min_vertex) * vertex_stride;
    const uint64_t end =
        indices ? begin + static_cast<uint64_t>(queued.index_count) * queued.index_size
                : vertex_base + (static_cast<uint64_t>(request.max_vertex) + 1u) * vertex_stride;
    const uint64_t aligned_begin = begin & ~uint64_t{3};
    const uint64_t aligned_end = (std::min)((end + 3u) & ~uint64_t{3}, buffer_size);
    const char* error = nullptr;
    if (end <= begin || end > buffer_size) {
      error = indices ? "index range outside the index buffer"
              : uvs   ? "uv range outside the uv buffer"
                      : "vertex range outside the vertex buffer";
    } else if (aligned_end - aligned_begin > kPoolMeshSlotBytes) {
      error = indices ? "index range larger than the mesh staging"
              : uvs   ? "uv range larger than the mesh staging"
                      : "vertex range larger than the mesh staging";
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
    } else if (!uvs) {
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

// Caller holds g_pool.mutex. Removes the texture -> source link of source `id` (a no-op when the handle already
// names another copy).
inline void UnlinkPoolAlphaTexture(uint64_t id, const PoolAlphaSource& source) {
  const auto link = g_pool.alpha_source_by_texture.find(source.texture);
  if (link != g_pool.alpha_source_by_texture.end() && link->second == id) g_pool.alpha_source_by_texture.erase(link);
}

// Caller holds g_pool.mutex. The indirect source switch OFF, once per switch (re-armed by switching on): detaches the
// indirect keys from their copies, drops the holds, and moves the copies left without a key to the dead list (freed at
// the next present, outside the lock). Nothing here touches the device.
inline void ApplyPoolIndirectAlphaSwitch() {
  if (g_pool.alpha_indirect_source.load(std::memory_order_relaxed)) {
    g_pool.alpha_indirect_off_applied = false;
    return;
  }
  if (g_pool.alpha_indirect_off_applied) return;
  g_pool.alpha_indirect_off_applied = true;
  for (const uint64_t key : g_pool.alpha_indirect_keys) {
    const auto key_source = g_pool.alpha_key_source.find(key);
    if (key_source == g_pool.alpha_key_source.end()) continue;
    const auto source = g_pool.alpha_sources.find(key_source->second);
    if (source != g_pool.alpha_sources.end()) source->second.keys.erase(key);
    g_pool.alpha_key_source.erase(key_source);
  }
  g_pool.alpha_indirect_keys.clear();
  for (auto it = g_pool.alpha_sources.begin(); it != g_pool.alpha_sources.end();) {
    it->second.hold_until = 0u;
    if (!it->second.keys.empty()) {
      ++it;
      continue;
    }
    if (it->second.proxy.handle != 0u) g_pool.alpha_dead_proxies.push_back(it->second.proxy);
    UnlinkPoolAlphaTexture(it->first, it->second);
    it = g_pool.alpha_sources.erase(it);
  }
}

// Caller holds g_pool.mutex. Moves every source copy to the dead list (freed at a present).
inline void DrainPoolAlphaSources() {
  for (const auto& source : g_pool.alpha_sources) {
    if (source.second.proxy.handle != 0u) g_pool.alpha_dead_proxies.push_back(source.second.proxy);
  }
  g_pool.alpha_sources.clear();
  g_pool.alpha_source_by_texture.clear();
  g_pool.alpha_key_source.clear();
  g_pool.alpha_indirect_keys.clear();
  g_pool.alpha_source_done.clear();
}

// Takes the proxies to destroy: the dead ones and, with `all` (alpha_foliage off, device destroyed), every
// live source copy. The caller destroys them outside the lock.
inline std::vector<reshade::api::resource> TakePoolAlphaProxies(bool all) {
  std::lock_guard<std::mutex> lock(g_pool.mutex);
  if (all) DrainPoolAlphaSources();
  std::vector<reshade::api::resource> proxies;
  proxies.swap(g_pool.alpha_dead_proxies);
  return proxies;
}

// The alpha switches as one mode byte (0 = alpha_foliage off): bit 0 alpha_foliage, bit 1 indirect source, bit 2 wind opaque.
inline uint8_t AlphaMode() {
  if (!g_pool.alpha_foliage.load(std::memory_order_relaxed)) return 0u;
  return static_cast<uint8_t>(1u | (g_pool.alpha_indirect_source.load(std::memory_order_relaxed) ? 2u : 0u)
                              | (g_pool.alpha_wind_opaque.load(std::memory_order_relaxed) ? 4u : 0u));
}

// Caller holds g_pool.mutex. The wind rest-pose switch OFF (AlphaMode bit 2 clear), once per switch, re-armed by switching
// on: retires the instances of the wind rest-pose keys. The keys stay in wind_keys, so AdmitPoolInstance keeps refusing them
// while off (also a copy recorded while on and resolved after the switch); InvalidatePoolMeshKey and ResetWorldPool forget them.
inline void ApplyPoolWindSwitch() {
  if ((AlphaMode() & 4u) != 0u) {
    g_pool.wind_off_applied = false;
    return;
  }
  if (g_pool.wind_off_applied) return;
  g_pool.wind_off_applied = true;
  for (const uint64_t key : g_pool.wind_keys) {
    const auto mesh_it = g_pool.mesh_by_key.find(key);
    if (mesh_it != g_pool.mesh_by_key.end()) RetirePoolMeshAlpha(mesh_it->second);
  }
}

// Caller holds g_pool.mutex. Bytes of the live source copies (the byte cap, the dump and the panel).
inline uint64_t PoolAlphaProxyBytes() {
  uint64_t bytes = 0u;
  for (const auto& source : g_pool.alpha_sources) bytes += source.second.bytes;
  return bytes;
}

// Copies the source texture of an alpha-tested draw (mip 0 of a single-layer, non-multisampled 2D texture) into an
// owned proxy, once per source texture: draw keys that sample the same texture share the copy (PoolAlphaSource::keys).
// Called only for a draw that is queued (skip None), outside g_pool.mutex. The proxy has the format of the view the
// game bound (not the texture's). At most kAlphaCopiesPerFrame copies a frame, kAlphaSourcesMax live and
// kAlphaSourceBytesMax bytes; other sources are refused and counted. No copy while the live BVH is off. Graphics calls
// run outside g_pool.mutex. The texture is registered with its draw keys, so destroying it invalidates them (the proxy
// is freed at the next present). mesh_key 0 is an indirect draw whose args are not read yet: no key is recorded, the
// copy is held for kAlphaOrphanFrames presents, and its id is returned for AttachPoolAlphaSource at resolve. The keyed
// path returns 0.
inline uint64_t CapturePoolAlphaSource(
    reshade::api::device* device, reshade::api::command_list* cmd_list, uint64_t mesh_key, uint64_t view, bool immediate) {
  const bool keyless = mesh_key == 0u;
  {
    std::lock_guard<std::mutex> lock(g_pool.mutex);
    const bool enabled = g_pool.alpha_foliage.load(std::memory_order_relaxed);
    if (!enabled) g_pool.alpha_capture_logged = false;
    if (!enabled || !g_pool.live_on.load(std::memory_order_relaxed)
        || (!keyless && (g_pool.alpha_source_done.count(mesh_key) != 0u || g_pool.alpha_key_source.count(mesh_key) != 0u))) {
      return 0u;
    }
    if (!g_pool.alpha_capture_logged) {
      g_pool.alpha_capture_logged = true;
      renodx::utils::log::i("[world-bvh] alpha stage: first source capture since alpha foliage was switched on (draw key ",
                            mesh_key, ")");
    }
  }
  if (!immediate) {
    std::lock_guard<std::mutex> lock(g_pool.mutex);
    g_pool.stats.alpha_source_refused += 1u;
    g_pool.stats.alpha_source_refused_deferred += 1u;
    return 0u;
  }
  const reshade::api::resource texture = device->get_resource_from_view({view});
  const reshade::api::resource_desc desc =
      texture.handle != 0u ? device->get_resource_desc(texture) : reshade::api::resource_desc{};
  if (desc.type != reshade::api::resource_type::texture_2d || desc.texture.depth_or_layers != 1u || desc.texture.samples != 1u) {
    std::lock_guard<std::mutex> lock(g_pool.mutex);
    if (!keyless) g_pool.alpha_source_done.insert(mesh_key);
    g_pool.stats.alpha_source_refused += 1u;
    g_pool.stats.alpha_source_refused_type += 1u;
    return 0u;
  }
  const reshade::api::format format = device->get_resource_view_desc({view}).format;
  if (format == reshade::api::format::unknown || reshade::api::format_is_typeless(format)) {
    bool first = false;
    {
      std::lock_guard<std::mutex> lock(g_pool.mutex);
      if (!keyless) g_pool.alpha_source_done.insert(mesh_key);
      g_pool.stats.alpha_source_refused += 1u;
      g_pool.stats.alpha_source_refused_type += 1u;
      g_pool.stats.alpha_source_refused_format += 1u;
      first = g_pool.alpha_format_logged.insert(texture.handle).second;
    }
    if (first) {
      renodx::utils::log::w("[world-bvh] alpha source: bound view format unknown or typeless (texture ", texture.handle,
                            "), draw key not copied");
    }
    return 0u;
  }
  const uint64_t bytes = reshade::api::format_slice_pitch(
      format, reshade::api::format_row_pitch(format, desc.texture.width), desc.texture.height);
  uint64_t source_id = 0u;
  {
    std::lock_guard<std::mutex> lock(g_pool.mutex);
    const auto link = g_pool.alpha_source_by_texture.find(texture.handle);
    if (link != g_pool.alpha_source_by_texture.end()) {  // the texture has its copy: this key shares it
      const auto shared_it = g_pool.alpha_sources.find(link->second);
      if (shared_it == g_pool.alpha_sources.end()) return 0u;
      PoolAlphaSource& shared = shared_it->second;
      if (keyless) return link->second;  // no refresh of hold_until: an orphan expires kAlphaOrphanFrames after its copy
      shared.keys.insert(mesh_key);
      shared.hold_until = 0u;  // a key uses the copy: it is no orphan any more
      g_pool.alpha_key_source[mesh_key] = link->second;
      AddPoolResourceKey(texture.handle, mesh_key);
      return 0u;
    }
    const uint32_t frame = g_state.frame.load();
    if (g_pool.alpha_copy_frame != frame) {
      g_pool.alpha_copy_frame = frame;
      g_pool.alpha_copies_frame = 0u;
    }
    size_t keyless_sources = 0u;  // keyless copies cap, so they cannot starve the keyed sources
    if (keyless) {
      for (const auto& entry : g_pool.alpha_sources) keyless_sources += entry.second.keys.empty() ? 1u : 0u;
    }
    if (g_pool.alpha_copies_frame >= kAlphaCopiesPerFrame || g_pool.alpha_sources.size() >= kAlphaSourcesMax
        || keyless_sources >= kAlphaKeylessSourcesMax) {
      g_pool.stats.alpha_source_refused += 1u;
      g_pool.stats.alpha_source_refused_cap += 1u;
      return 0u;
    }
    if (PoolAlphaProxyBytes() + bytes > kAlphaSourceBytesMax) {
      g_pool.stats.alpha_source_refused += 1u;
      g_pool.stats.alpha_source_refused_cap += 1u;
      g_pool.stats.alpha_source_refused_bytes += 1u;
      return 0u;
    }
    g_pool.alpha_copies_frame += 1u;
    source_id = g_pool.alpha_next_source++;
    PoolAlphaSource& source = g_pool.alpha_sources[source_id];  // reserved while its copy is made
    source.texture = texture.handle;
    source.bytes = bytes;
    if (keyless) {
      source.hold_until = frame + kAlphaOrphanFrames;
    } else {
      source.keys.insert(mesh_key);
      g_pool.alpha_key_source[mesh_key] = source_id;
    }
    g_pool.alpha_source_by_texture[texture.handle] = source_id;
  }
  const reshade::api::resource_desc proxy_desc(
      reshade::api::resource_type::texture_2d, desc.texture.width, desc.texture.height, 1, 1, format, 1,
      reshade::api::memory_heap::gpu_only, reshade::api::resource_usage::shader_resource | reshade::api::resource_usage::copy_dest);
  reshade::api::resource proxy = {0u};
  const bool made = device->create_resource(proxy_desc, nullptr, reshade::api::resource_usage::copy_dest, &proxy);
  if (made) cmd_list->copy_texture_region(texture, 0u, nullptr, proxy, 0u, nullptr);
  bool kept = false;
  {
    std::lock_guard<std::mutex> lock(g_pool.mutex);
    const auto it = g_pool.alpha_sources.find(source_id);
    if (made && it != g_pool.alpha_sources.end()) {
      it->second.proxy = proxy;
      it->second.format = format;
      for (const uint64_t key : it->second.keys) AddPoolResourceKey(texture.handle, key);
      g_pool.stats.alpha_source_copies += 1u;
      if (keyless) g_pool.stats.alpha_indirect_copies += 1u;
      kept = true;
    } else {
      if (it != g_pool.alpha_sources.end()) {  // the copy failed: its keys are refused
        for (const uint64_t key : it->second.keys) {
          g_pool.alpha_key_source.erase(key);
          g_pool.alpha_source_done.insert(key);
        }
        UnlinkPoolAlphaTexture(source_id, it->second);
        g_pool.alpha_sources.erase(it);
      }
      g_pool.stats.alpha_source_refused += 1u;
      g_pool.stats.alpha_source_refused_failed += 1u;
    }
  }
  if (made && !kept) device->destroy_resource(proxy);
  return keyless && kept ? source_id : 0u;
}

// Caller holds g_pool.mutex (resolve). Attaches the keyless copy of an indirect alpha draw to the mesh key its args
// resolved to (alpha_foliage and the indirect switch on). The copy then belongs to its key (hold cleared) and is freed
// with it. A key that already has a source keeps it. A missing copy is counted (alpha_orphan_missed).
inline void AttachPoolAlphaSource(uint64_t mesh_key, uint64_t source_id) {
  const auto source = g_pool.alpha_sources.find(source_id);
  if (source == g_pool.alpha_sources.end()) {
    g_pool.stats.alpha_orphan_missed += 1u;
    return;
  }
  const auto attached = g_pool.alpha_key_source.find(mesh_key);
  if (attached != g_pool.alpha_key_source.end()) {  // the key keeps its first copy; a copy of another texture is counted
    if (attached->second != source_id) g_pool.stats.alpha_indirect_source_changed += 1u;
    return;
  }
  source->second.keys.insert(mesh_key);
  source->second.hold_until = 0u;
  g_pool.alpha_key_source[mesh_key] = source_id;
  g_pool.alpha_indirect_keys.insert(mesh_key);
  g_pool.stats.alpha_orphans_attached += 1u;
  if (!source->second.texture_dead) AddPoolResourceKey(source->second.texture, mesh_key);
}

inline void OnPoolScanDraw(
    reshade::api::device* device,
    reshade::api::command_list* cmd_list,
    const DrawRecord& draw,
    WorldCommandListData* cl_data) {
  if (!g_pool.scan_active.load(std::memory_order_relaxed)) return;
  const PoolCpuTimer cpu_timer{&g_pool.cpu_hook_us};
  if (device == nullptr || cmd_list == nullptr || cl_data == nullptr) return;
  PoolStageScope stage("direct draw: gate");
  const bool deferred = IsPoolDeferredList(cmd_list);

  const PoolDrawGate gate = GatePoolDraw(draw, true);
  if (gate.wind_rest) {
    std::lock_guard<std::mutex> lock(g_pool.mutex);
    g_pool.stats.wind_rest_draws += 1u;
  }
  PoolAlphaMaterial alpha_material;
  if (gate.alpha_material) ReadPoolAlphaDraw(cmd_list, *cl_data, gate.alpha_threshold_offset, &alpha_material);
  PoolVertexBuffers vbs;
  const bool vbs_known = gate.alpha_material && CollectPoolVertexBuffers(cmd_list, &vbs);
  PoolUvStream uv;
  if (vbs_known) ResolvePoolUvStream(device, draw, vbs, &uv);
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
  bool queued = false;  // this draw is queued for a copy (ReservePoolCopy None, QueuePoolMesh ran)
  {
    std::lock_guard<std::mutex> lock(g_pool.mutex);
    if (gate.alpha_material) NotePoolAlphaMaterial(PoolMeshKey(draw), alpha_material, vbs_known ? &vbs : nullptr);
    g_pool.stats.draws_by_vs_class[static_cast<size_t>(gate.vs_class)] += 1u;
    if (deferred) g_pool.stats.draws_on_deferred += 1u;
    PoolFamilyStats& family = g_pool.families[draw.vs_hash];
    family.vs_class = static_cast<uint8_t>(gate.vs_class);
    family.draws += 1u;

    if (skip == PoolSkip::None) {
      const uint64_t schedule_key = PoolScheduleKey(mesh_key, draw.first_instance, count);
      const auto known_schedule = g_pool.schedule.find(schedule_key);
      const bool follow = (gate.pass & kPoolPassCamera) != 0u && known_schedule != g_pool.schedule.end()
                          && known_schedule->second.follow;
      const uint64_t copy_bytes = kPoolCbCopyBytes + static_cast<uint64_t>(count) * kPoolInstanceStride;
      bool traced = false;  // a key in trace state: its copy may bypass the cooldown
      if ((gate.pass & kPoolPassCamera) != 0u) {
        for (const auto& key : g_pool.prev_trace.keys) {
          if (key.state == PoolPrevTrace::KeyState::Active && key.mesh_key == mesh_key) traced = true;
        }
      }
      PoolSchedule* schedule = nullptr;
      skip = ReservePoolCopy(device, schedule_key, frame, copy_bytes, &schedule,
                             follow ? PoolCopyKind::Follow : PoolCopyKind::Normal);
      const bool trace_only = traced && skip == PoolSkip::Cooldown;
      uint32_t copy_count = count;
      if (trace_only) {
        copy_count = (std::min)(count, kPoolTraceMaxElements);
        skip = ReservePoolCopy(device, schedule_key, frame, kPoolCbCopyBytes + static_cast<uint64_t>(copy_count) * kPoolInstanceStride,
                               &schedule, PoolCopyKind::Trace);
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
        if ((gate.pass & kPoolPassWind) != 0u) g_pool.wind_keys.insert(mesh_key);
        copy.schedule_key = schedule_key;
        copy.follow = follow;
        copy.trace_only = trace_only;
        // The b1 copy is recorded before this draw, so it holds exactly the
        // offset the draw reads; resolve compares it to `base`.
        commands[command_count++] = {slice.instance_cb, 0u, slot.buffer, copy.cb_offset, slice.cb_bytes};
        commands[command_count++] = {slice.instance_buffer, static_cast<uint64_t>(slice.base) * kPoolInstanceStride,
                                     slot.buffer, copy.slice_offset, slice_bytes};
        slot.used += kPoolCbCopyBytes + slice_bytes;
        if (follow) slot.follow_used += kPoolCbCopyBytes + slice_bytes;
        slot.copies.push_back(copy);
        if (!trace_only) {
          schedule->copy_frame = frame;
          schedule->next_frame = frame + kPoolRecaptureFrames;
          g_pool.stats.copied_draws += 1u;
          family.copied += 1u;
          QueuePoolMesh(mesh_key, draw.vs_hash, draw, false, uv);
          queued = true;
        }
      }
    }
    if (skip != PoolSkip::None && !trace_declined) CountPoolSkip(family, skip, gate.state);
    if (gate.skip == PoolSkip::PixelUnknown) NotePoolPixelUnknown(draw, gate.unknown_reason);
    mesh_count = ReservePoolMeshCopies(device, draw, uv, false, deferred, frame, commands.data() + command_count,
                                       mesh_copies.data(), kPoolMeshCopiesPerDraw);
    command_count += mesh_count;
  }
  if (queued && gate.alpha_material && alpha_material.view != 0u) {
    const reshade::api::resource source = device->get_resource_from_view({alpha_material.view});
    if (source.handle != cl_data->ps_srv[contract::kAlphaTexSlot].handle) {
      std::lock_guard<std::mutex> lock(g_pool.mutex);
      g_pool.alpha_source_done.insert(mesh_key);
      g_pool.stats.alpha_source_refused += 1u;
      g_pool.stats.alpha_source_refused_type += 1u;
      g_pool.stats.alpha_source_refused_view += 1u;
    } else {
      CapturePoolAlphaSource(device, cmd_list, mesh_key, alpha_material.view, !deferred);
    }
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
  const PoolCpuTimer cpu_timer{&g_pool.cpu_hook_us};
  if (!g_pool.scan_indirect.load(std::memory_order_relaxed)) return;
  if (device == nullptr || cmd_list == nullptr || cl_data == nullptr || args_buffer.handle == 0u) return;
  PoolStageScope stage("indirect draw: gate");
  const bool deferred = IsPoolDeferredList(cmd_list);

  const PoolDrawGate gate = GatePoolDraw(draw, false);
  if (gate.wind_rest) {
    std::lock_guard<std::mutex> lock(g_pool.mutex);
    g_pool.stats.wind_rest_draws += 1u;
  }
  PoolAlphaMaterial alpha_material;
  if (gate.alpha_material) ReadPoolAlphaDraw(cmd_list, *cl_data, gate.alpha_threshold_offset, &alpha_material);
  PoolVertexBuffers vbs;
  const bool vbs_known = gate.alpha_material && CollectPoolVertexBuffers(cmd_list, &vbs);
  PoolUvStream uv;
  if (vbs_known) ResolvePoolUvStream(device, draw, vbs, &uv);
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
  std::array<std::pair<uint32_t, uint32_t>, kPoolMaxIndirectSubDraws> alpha_refs;  // (slot, indirect_alpha entry) per reserved alpha copy
  uint32_t alpha_ref_count = 0u;
  uint32_t command_count = 0u;
  uint32_t mesh_count = 0u;
  {
    std::lock_guard<std::mutex> lock(g_pool.mutex);
    PoolFamilyStats& family = g_pool.families[draw.vs_hash];
    family.vs_class = static_cast<uint8_t>(gate.vs_class);
    if (gate.skip == PoolSkip::PixelUnknown) NotePoolPixelUnknown(draw, gate.unknown_reason);
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
      const bool follow = (gate.pass & kPoolPassCamera) != 0u && known != g_pool.schedule.end() && known->second.follow;
      if (learned) window = (std::min)(known->second.indirect_window, max_window);
      const uint64_t bytes =
          kPoolCbCopyBytes + kPoolIndirectArgsBytes + static_cast<uint64_t>(window) * kPoolInstanceStride;
      PoolSchedule* schedule = nullptr;
      if (sub_skip == PoolSkip::None) {
        sub_skip = ReservePoolCopy(device, schedule_key, frame, bytes, &schedule,
                                   follow ? PoolCopyKind::Follow : PoolCopyKind::Normal);
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
      copy.alpha = alpha_material;
      if (gate.alpha_material) {
        copy.alpha_index = static_cast<uint32_t>(slot.indirect_alpha.size());
        slot.indirect_alpha.push_back({uv, vbs, vbs_known});
        alpha_refs[alpha_ref_count++] = {g_pool.write_slot, copy.alpha_index};
      }
      copy.indirect_index = static_cast<uint32_t>(slot.indirect_draws.size());
      copy.schedule_key = schedule_key;
      copy.follow = follow;
      PoolCopyCommand* sub_commands = commands.data() + command_count;
      sub_commands[0] = {slice.instance_cb, 0u, slot.buffer, copy.cb_offset, slice.cb_bytes};
      sub_commands[1] = {args_buffer, sub_args_offset, slot.buffer, copy.args_offset, kPoolIndirectArgsCopyBytes};
      sub_commands[2] = {slice.instance_buffer, static_cast<uint64_t>(slice.base) * kPoolInstanceStride, slot.buffer,
                         copy.slice_offset, static_cast<uint64_t>(window) * kPoolInstanceStride};
      command_count += 3u;
      slot.used += bytes;
      if (follow) slot.follow_used += bytes;
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
    mesh_count = ReservePoolMeshCopies(device, draw, uv, true, deferred, frame, commands.data() + command_count,
                                       mesh_copies.data(), kPoolMeshCopiesPerDraw);
    command_count += mesh_count;
  }
  if (alpha_ref_count != 0u && (AlphaMode() & 2u) != 0u && alpha_material.view != 0u) {
    // The same view guard as the direct path: the bound view must be the t0 resource.
    const reshade::api::resource source = device->get_resource_from_view({alpha_material.view});
    uint64_t source_id = 0u;
    if (source.handle != cl_data->ps_srv[contract::kAlphaTexSlot].handle) {
      std::lock_guard<std::mutex> lock(g_pool.mutex);
      g_pool.stats.alpha_source_refused += 1u;
      g_pool.stats.alpha_source_refused_type += 1u;
      g_pool.stats.alpha_source_refused_view += 1u;
    } else {
      source_id = CapturePoolAlphaSource(device, cmd_list, 0u, alpha_material.view, !deferred);
    }
    if (source_id != 0u) {
      std::lock_guard<std::mutex> lock(g_pool.mutex);
      for (uint32_t i = 0u; i < alpha_ref_count; ++i) {
        PoolStagingSlot& alpha_slot = g_pool.slots[alpha_refs[i].first];
        if (alpha_refs[i].second < alpha_slot.indirect_alpha.size()) alpha_slot.indirect_alpha[alpha_refs[i].second].source = source_id;
      }
    }
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
  g_pool.mesh_retry.erase(mesh_key);
  g_pool.alpha_keys.erase(mesh_key);
  g_pool.alpha_uv_requeued.erase(mesh_key);
  g_pool.alpha_source_done.erase(mesh_key);
  g_pool.alpha_indirect_keys.erase(mesh_key);
  g_pool.wind_keys.erase(mesh_key);
  const auto key_source = g_pool.alpha_key_source.find(mesh_key);
  if (key_source != g_pool.alpha_key_source.end()) {
    const auto source = g_pool.alpha_sources.find(key_source->second);
    if (source != g_pool.alpha_sources.end()) {
      source->second.keys.erase(mesh_key);
      // No key uses the copy any more; a keyless copy (indirect draw) is held until its hold runs out.
      if (source->second.keys.empty()
          && (source->second.hold_until == 0u || g_state.frame.load() > source->second.hold_until)) {
        if (source->second.proxy.handle != 0u) g_pool.alpha_dead_proxies.push_back(source->second.proxy);
        UnlinkPoolAlphaTexture(key_source->second, source->second);
        g_pool.alpha_sources.erase(source);
      }
    }
    g_pool.alpha_key_source.erase(key_source);
  }
  g_pool.invalidated_keys.insert(mesh_key);
  // A queued mesh is forgotten; a copy of it still in flight is dropped when
  // read (no request with its serial is left).
  UnmapPoolMeshKey(mesh_key);
}

// A game write to a VB/IB that a captured or queued mesh comes from
// (diagnostic only): the mesh may no longer match the buffer's contents.
// Other buffers cost a bloom-filter test and no lock. Called from the game's
// own calls; the pool never makes graphics calls under its lock, so this
// cannot re-enter it.
inline void NotePoolBufferWrite(uint64_t handle) {
  if (handle == 0u || !PoolBloomTest(handle)) return;
  g_pool.write_events.tracked.fetch_add(1u, std::memory_order_relaxed);
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
  g_pool.write_events.update.fetch_add(1u, std::memory_order_relaxed);
  NotePoolBufferWrite(dest.handle);
  return false;
}

inline bool OnUpdateBufferRegionCommandPool(
    reshade::api::command_list*, const void*, reshade::api::resource dest, uint64_t, uint64_t) {
  g_pool.write_events.update_cmd.fetch_add(1u, std::memory_order_relaxed);
  NotePoolBufferWrite(dest.handle);
  return false;
}

inline void OnMapBufferRegionPool(
    reshade::api::device*, reshade::api::resource resource, uint64_t, uint64_t, reshade::api::map_access access, void**) {
  g_pool.write_events.map.fetch_add(1u, std::memory_order_relaxed);
  if (access != reshade::api::map_access::read_only) NotePoolBufferWrite(resource.handle);
}

inline bool OnCopyBufferRegionPool(
    reshade::api::command_list*, reshade::api::resource, uint64_t, reshade::api::resource dest, uint64_t, uint64_t) {
  g_pool.write_events.copy_region.fetch_add(1u, std::memory_order_relaxed);
  NotePoolBufferWrite(dest.handle);
  return false;
}

inline bool OnCopyResourcePool(reshade::api::command_list*, reshade::api::resource, reshade::api::resource dest) {
  g_pool.write_events.copy_resource.fetch_add(1u, std::memory_order_relaxed);
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
  if (!g_pool.alpha_sources.empty()) {  // the destroyed texture's copy is no longer linked to it
    const auto link = g_pool.alpha_source_by_texture.find(resource.handle);
    if (link != g_pool.alpha_source_by_texture.end()) {
      const auto source_it = g_pool.alpha_sources.find(link->second);
      if (source_it != g_pool.alpha_sources.end()) {
        source_it->second.texture_dead = true;
        UnlinkPoolAlphaTexture(link->second, source_it->second);
      } else {
        g_pool.alpha_source_by_texture.erase(link);  // a stale link (no copy): nothing to free, and no throw in a destroy event
      }
    }
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
  // An alpha mesh keeps its flag and conflict only while an alpha key still maps to it.
  std::vector<bool> alpha_held(g_pool.meshes.size(), false);
  for (const auto& alpha_key : g_pool.alpha_keys) {
    const auto mesh_it = g_pool.mesh_by_key.find(alpha_key.first);
    if (mesh_it != g_pool.mesh_by_key.end() && mesh_it->second < alpha_held.size()) alpha_held[mesh_it->second] = true;
  }
  for (WorldMesh& mesh : g_pool.meshes) {
    if (mesh.alpha && !alpha_held[mesh.mesh_id]) {
      mesh.alpha = false;
      mesh.alpha_state = {};
    }
  }
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
  for (const auto& uv : mesh.uvs) {
    for (const float value : uv) {
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
      g_pool.failed_meshes[mesh_key] = PoolMeshFailure{.reason = "dropped (mesh cap)", .vs_hash = vs_hash,
                                                       .frame = g_state.frame.load(), .logged = true};
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
      world_mesh.admitted_by_retry = g_pool.mesh_retry.count(mesh_key) != 0u;
      world_mesh.source_vb_usage = origin->draw.vb_usage;
      world_mesh.source_vb_flags = origin->draw.vb_flags;
      world_mesh.source_ib_usage = origin->draw.ib_usage;
      world_mesh.source_ib_flags = origin->draw.ib_flags;
    }
    std::memcpy(world_mesh.bbox_min, mesh.bbox_min.data(), sizeof(float) * 3u);
    std::memcpy(world_mesh.bbox_max, mesh.bbox_max.data(), sizeof(float) * 3u);
    world_mesh.positions = std::move(mesh.positions);
    world_mesh.uvs = std::move(mesh.uvs);
    world_mesh.indices.reserve(mesh.triangles.size() * 3u);
    for (const auto& triangle : mesh.triangles) {
      world_mesh.indices.push_back(triangle[0]);
      world_mesh.indices.push_back(triangle[1]);
      world_mesh.indices.push_back(triangle[2]);
    }
    mesh_id = world_mesh.mesh_id;
    g_pool.meshes.push_back(std::move(world_mesh));
    if (g_pool.meshes.back().admitted_by_retry) g_pool.stats.meshes_admitted_by_retry += 1u;
    g_pool.mesh_by_signature.emplace(signature, mesh_id);
    g_pool.revision += 1u;
  }
  g_pool.meshes[mesh_id].live_keys += 1u;
  g_pool.mesh_by_key[mesh_key] = mesh_id;
  const auto dynamic = g_pool.dynamic_keys.find(mesh_key);
  if (dynamic != g_pool.dynamic_keys.end()) ApplyPoolDynamicKey(mesh_key, dynamic->second);  // seen moving before its capture
  const auto alpha_key = g_pool.alpha_keys.find(mesh_key);
  if (alpha_key != g_pool.alpha_keys.end()) MarkPoolMeshAlpha(mesh_id, alpha_key->second);  // alpha before its capture
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
    uint32_t* max_vertex,
    uint32_t* raw_min,
    uint32_t* raw_max,
    uint32_t* first_raw) {
  if (index_size != 2u && index_size != 4u) return "unsupported index size";
  if (size < static_cast<uint64_t>(index_count) * index_size) return "index copy shorter than the draw";
  indices->clear();
  indices->reserve(index_count);
  int64_t lowest = INT64_MAX;
  int64_t highest = -1;
  uint32_t raw_lowest = UINT32_MAX;
  uint32_t raw_highest = 0u;
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
    if (i < 8u) first_raw[i] = raw;
    raw_lowest = (std::min)(raw_lowest, raw);
    raw_highest = (std::max)(raw_highest, raw);
    if (absolute < 0 || absolute > static_cast<int64_t>(UINT32_MAX - 1u)) return "index outside the vertex buffer";
    indices->push_back(static_cast<uint32_t>(absolute));
    lowest = (std::min)(lowest, absolute);
    highest = (std::max)(highest, absolute);
  }
  if (highest < 0) return "no indices";
  *min_vertex = static_cast<uint32_t>(lowest);
  *max_vertex = static_cast<uint32_t>(highest);
  *raw_min = raw_lowest;
  *raw_max = raw_highest;
  if (static_cast<uint64_t>(highest - lowest + 1) * stride > kPoolMeshSlotBytes) {
    return "vertex range larger than the mesh staging";
  }
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
    int32_t uv_offset,
    reshade::api::format uv_format,
    const std::vector<uint32_t>& indices,
    uint32_t min_vertex,
    uint32_t max_vertex,
    PoolDecodedMesh* mesh,
    const uint8_t* uv_bytes = nullptr,  // a TEXCOORD0 stream of its own (uv_offset is its offset): null = in `bytes`
    uint64_t uv_size = 0u,
    uint32_t uv_stride = 0u) {
  namespace scene = renodx::utils::scene;
  if (uv_bytes == nullptr) {
    uv_bytes = bytes;
    uv_size = size;
    uv_stride = stride;
  }
  const scene::FormatInfo* info = scene::FindFormatInfo(pos_format);
  if (info == nullptr) return "unsupported position format";
  if (max_vertex < min_vertex || pos_offset < 0) return "bad vertex range";
  const uint64_t vertex_count = static_cast<uint64_t>(max_vertex) - min_vertex + 1u;
  if (size < (vertex_count - 1u) * stride + static_cast<uint64_t>(pos_offset) + info->byte_size) {
    return "vertex copy shorter than the range";
  }
  std::vector<uint32_t> remap(static_cast<size_t>(vertex_count), UINT32_MAX);
  mesh->positions.clear();
  mesh->uvs.clear();
  const scene::FormatInfo* uv_info = uv_offset >= 0 ? scene::FindFormatInfo(uv_format) : nullptr;
  const uint64_t uv_end = uv_info != nullptr ? static_cast<uint64_t>(uv_offset) + uv_info->byte_size : 0u;
  if (uv_end > uv_stride || uv_size < (vertex_count - 1u) * uv_stride + uv_end) uv_info = nullptr;  // UVs the copy does not hold
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
        if (uv_info != nullptr) {
          float uv[4] = {};
          const uint8_t* uv_vertex = uv_bytes + static_cast<uint64_t>(absolute - min_vertex) * uv_stride + uv_offset;
          if (scene::DecodeAttribute(uv_vertex, *uv_info, uv) && std::isfinite(uv[0]) && std::isfinite(uv[1])) {
            mesh->uvs.push_back({uv[0], uv[1]});
          }
        }
      }
      triangle[k] = fresh;
    }
    mesh->triangles.push_back(triangle);
  }
  if (mesh->positions.empty()) return "no vertices decoded";
  if (mesh->uvs.size() != mesh->positions.size()) mesh->uvs.clear();
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
                        on(g_pool.legacy_scale), ", keep moving objects out ", on(g_pool.exclude_moving),
                        ", follow moving objects ", on(g_pool.follow_moving), ", retry unstable meshes ", on(g_pool.retry_unstable), ", alpha foliage ", on(g_pool.alpha_foliage),
                        ", alpha indirect source ", on(g_pool.alpha_indirect_source), ", alpha wind opaque ", on(g_pool.alpha_wind_opaque));
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

// Caller holds g_pool.mutex. The first triangle whose corners moved between two captures, and how many did.
inline void ComparePoolMeshes(
    const PoolDecodedMesh& previous, const PoolDecodedMesh& current, size_t* first, size_t* differing) {
  const size_t triangles = (std::min)(previous.triangles.size(), current.triangles.size());
  for (size_t t = 0; t < triangles; ++t) {
    for (int k = 0; k < 3; ++k) {
      // UVs are compared only when both captures have them: a capture without UVs is not a difference.
      const bool uv_differs = !previous.uvs.empty() && !current.uvs.empty()
                              && previous.uvs[previous.triangles[t][k]] != current.uvs[current.triangles[t][k]];
      if (previous.positions[previous.triangles[t][k]] != current.positions[current.triangles[t][k]] || uv_differs) {
        if (*first == SIZE_MAX) *first = t;
        *differing += 1u;
        break;
      }
    }
  }
}

// One line for ReShade.log: where two captures of a mesh differ.
inline std::string DescribePoolMeshMismatch(
    const PoolMeshRequest& request, const PoolDecodedMesh& current, uint64_t index_signature, uint32_t frame,
    size_t first, size_t differing) {
  namespace log_utils = renodx::utils::log;
  const PoolDecodedMesh& previous = request.previous;
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
      " | previous capture: indices from offset ", request.previous_indices_source_offset, ", vertices from offset ",
      request.previous_vertices_source_offset, " | vb ", log_utils::AsPtr(draw.vb.handle), " ", PoolBufferText(draw.vb_usage, draw.vb_flags), " stride ",
      draw.vb_stride, " offset ", draw.vb_offset, " | ib ", log_utils::AsPtr(draw.ib.handle), " ",
      PoolBufferText(draw.ib_usage, draw.ib_flags), " first index ", draw.first_index, " count ", draw.index_count,
      " base vertex ", draw.vertex_offset, " | written ", request.written, ", tracked buffer writes ", g_pool.stats.tracked_buffer_writes, ", requests written ", g_pool.stats.requests_written);
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
  if (request.staging.poisoned) {
    line += log_utils::BuildString(" | staging: slot ", request.staging.slot, " copy ", request.staging.ordinal,
                                   " at offset ", request.staging.staging_offset, " copied ", request.staging.copied,
                                   " (begin mod 16 ", request.staging.begin_mod16, "), poison words ",
                                   request.staging.sentinel_words, " (head ", request.staging.sentinel_head, " of ",
                                   request.staging.head_words, "), head ", request.staging.head_class);
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
  size_t first = SIZE_MAX;
  size_t differing = 0u;
  if (queued.has_previous) ComparePoolMeshes(queued.previous, mesh, &first, &differing);
  if (queued.records.size() < kPoolMeshMaxCaptures) {
    PoolCaptureRecord& record = queued.records.emplace_back();
    record.frame = frame;
    record.indices_frame = queued.indices_frame;
    record.vertices_frame = queued.vertices_frame;
    record.indices_served_indirect = queued.indices_served_indirect;
    record.vertices_served_indirect = queued.vertices_served_indirect;
    record.indices_source_offset = queued.indices_source_offset;
    record.vertices_source_offset = queued.vertices_source_offset;
    record.min_vertex = queued.min_vertex;
    record.max_vertex = queued.max_vertex;
    record.indices_raw_hash = queued.indices_raw_hash;
    record.vertices_raw_hash = queued.vertices_raw_hash;
    record.signature = signature;
    record.index_signature = index_signature;
    record.written = queued.written;
    record.staging = queued.staging;
    record.vertices = static_cast<uint32_t>(mesh.positions.size());
    record.triangles = static_cast<uint32_t>(mesh.triangles.size());
    record.first_diff_triangle = first == SIZE_MAX ? -1 : static_cast<int64_t>(first);
    record.differing_triangles = differing;
    if (first != SIZE_MAX) {
      for (int k = 0; k < 3; ++k) {
        for (int c = 0; c < 3; ++c) {
          record.before[k * 3 + c] = queued.previous.positions[queued.previous.triangles[first][k]][c];
          record.after[k * 3 + c] = mesh.positions[mesh.triangles[first][k]][c];
        }
      }
    }
  }
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
    if (g_pool.mismatch_meshes.size() < kPoolMismatchMaxMeshes || g_pool.mismatch_meshes.count(queued.mesh_key) != 0u) {
      PoolMeshMismatch& entry = g_pool.mismatch_meshes[queued.mesh_key];
      entry.draw_frame = queued.draw.frame;
      entry.draw_serial = queued.draw.serial;
      entry.history = queued.records;
    }
    if (g_pool.mismatch_lines_logged < kPoolMismatchLogLines && warning_lines != nullptr) {
      g_pool.mismatch_lines_logged += 1u;
      warning_lines->push_back(DescribePoolMeshMismatch(queued, mesh, index_signature, frame, first, differing));
    }
  }
  if (queued.captures >= kPoolMeshMaxCaptures && g_pool.retry_unstable.load(std::memory_order_relaxed)) {
    PoolMeshRetry& retry = g_pool.mesh_retry[queued.mesh_key];
    if (retry.rounds < kPoolMeshRetryRounds) {
      // Another round: the request leaves the queue until next_frame.
      retry.rounds += 1u;
      retry.next_frame = frame + kPoolMeshRetryFrames;
      g_pool.stats.mesh_retries += 1u;
      g_pool.mesh_queued.erase(queued.mesh_key);
      g_pool.mesh_requests.erase(request);
      return "unstable: captured again after 180 frames";
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
  queued.previous_indices_source_offset = queued.indices_source_offset;
  queued.previous_vertices_source_offset = queued.vertices_source_offset;
  queued.has_previous = true;
  queued.phase = PoolMeshPhase::Indices;
  queued.indices.clear();
  queued.vertex_bytes.clear();
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
  int32_t uv_offset = -1;
  reshade::api::format uv_format = reshade::api::format::unknown;
  PoolUvStream uv;                    // uv read: the stream (buffer 0: none)
  std::vector<uint8_t> vertex_bytes;  // uv read: the vertex read, moved from the request
  std::vector<uint32_t> indices;  // index read: output; vertex read: moved from the request
  uint32_t min_vertex = 0u;
  uint32_t max_vertex = 0u;
  uint32_t raw_index_min = 0u;  // raw index values, before the base vertex (index read)
  uint32_t raw_index_max = 0u;
  uint32_t first_raw[8] = {};
  uint64_t raw_hash = 0u;  // FNV of the first 4 KiB of the bytes read
  PoolStagingDiag staging;
  const char* error = nullptr;
  PoolDecodedMesh mesh;
};

// Caller holds g_pool.mutex. Says where a copy's head bytes came from (poison_staging):
// the poison word (the copy did not write them), or a head read from this slot in an
// earlier cycle at the same or another offset, or in this cycle. Then remembers the head.
inline void ClassifyPoolStagingHead(PoolStagingDiag& diag, uint64_t mesh_key, uint64_t cycle) {
  std::vector<PoolResidue>& table = g_pool.residue[diag.slot];
  if (diag.head_words != 0u && diag.sentinel_head == diag.head_words) {
    diag.head_class = "sentinel";
  } else {
    // 5: this mesh's own head (expected: the mesh is copied again); 4: another mesh's head at the same
    // offset in the previous cycle; 3: another offset, previous cycle; 2: same cycle. Other meshes' heads are
    // only called stale when the mesh itself is not the match.
    uint32_t best = 0u;
    for (const PoolResidue& entry : table) {
      if (entry.head_hash != diag.head_hash) continue;
      uint32_t rank = 0u;
      if (entry.mesh_key == mesh_key) rank = 5u;
      else if (entry.cycle + 1u == cycle) rank = entry.offset == diag.staging_offset ? 4u : 3u;
      else if (entry.cycle == cycle) rank = 2u;
      best = (std::max)(best, rank);
    }
    diag.head_class = best == 5u ? "same mesh (expected)" : best == 4u ? "same offset previous cycle"
                      : best == 3u ? "other offset previous cycle" : best == 2u ? "same cycle" : "none";
  }
  table.push_back(PoolResidue{.offset = diag.staging_offset, .head_hash = diag.head_hash, .mesh_key = mesh_key, .cycle = cycle});
  table.erase(std::remove_if(table.begin(), table.end(),
                             [cycle](const PoolResidue& entry) { return entry.cycle + kPoolResidueCycles <= cycle; }),
              table.end());
}

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
  std::vector<PoolIndirectAlpha> indirect_alpha;
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
      slot.follow_used = 0u;
      slot.mesh_used = 0u;
      slot.indirect_draws.clear();
      slot.indirect_alpha.clear();
      return;
    }
    copies.swap(slot.copies);
    indirect_draws.swap(slot.indirect_draws);
    indirect_alpha.swap(slot.indirect_alpha);
    jobs.reserve(slot.mesh_copies.size());
    for (const PoolMeshCopy& copy : slot.mesh_copies) {
      PoolMeshJob job;
      job.copy = copy;
      job.staging.slot = slot_index;
      job.staging.ordinal = static_cast<uint32_t>(jobs.size());
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
        job.uv_offset = g_pool.alpha_keys.count(queued.mesh_key) != 0u ? queued.uv_offset : -1;
        job.uv_format = queued.uv_format;
        job.uv = queued.uv;
        job.min_vertex = queued.min_vertex;
        job.max_vertex = queued.max_vertex;
        if (copy.phase != PoolMeshPhase::Indices) job.indices = std::move(queued.indices);
        if (copy.phase == PoolMeshPhase::Uvs) job.vertex_bytes = std::move(queued.vertex_bytes);
      }
      jobs.push_back(std::move(job));
    }
    slot.mesh_copies.clear();
    buffer = slot.buffer;
    used = slot.used;
    g_pool.stats.max_slot_used = (std::max)(g_pool.stats.max_slot_used, slot.used);
    g_pool.stats.max_follow_used = (std::max)(g_pool.stats.max_follow_used, slot.follow_used);
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
  const bool poison = g_pool.poison_staging.load(std::memory_order_relaxed);
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
      job.raw_hash = 1469598103934665603ull;
      for (uint64_t b = 0u; b < (std::min)(job.copy.size, uint64_t{4096}); ++b) {
        job.raw_hash = (job.raw_hash ^ data[b]) * 1099511628211ull;
      }
      if (poison) {
        const uint8_t* raw = bytes + job.copy.staging_offset;
        PoolStagingDiag& diag = job.staging;
        diag.poisoned = true;
        diag.staging_offset = job.copy.staging_offset;
        diag.copied = job.copy.copied;
        diag.begin_mod16 = static_cast<uint32_t>((job.copy.staging_offset + job.copy.skip) % 16u);
        diag.head_bytes = static_cast<uint32_t>((std::min)(job.copy.copied, uint64_t{48}));
        std::memcpy(diag.head, raw, diag.head_bytes);
        diag.head_hash = 1469598103934665603ull;
        for (uint32_t b = 0u; b < diag.head_bytes; ++b) diag.head_hash = (diag.head_hash ^ diag.head[b]) * 1099511628211ull;
        diag.head_words = static_cast<uint32_t>((std::min)(job.copy.copied, uint64_t{64}) / 4u);
        for (uint64_t w = 0u; w < job.copy.copied / 4u; ++w) {
          uint32_t word = 0u;
          std::memcpy(&word, raw + 4u * w, sizeof(word));
          if (word != kPoolPoisonWord) continue;
          diag.sentinel_words += 1u;
          if (w < diag.head_words) diag.sentinel_head += 1u;
        }
      }
      if (job.copy.phase == PoolMeshPhase::Indices) {
        job.error = DecodePoolMeshIndices(data, job.copy.size, job.index_size, job.index_count, job.vertex_offset,
                                          job.stride, &job.indices, &job.min_vertex, &job.max_vertex,
                                          &job.raw_index_min, &job.raw_index_max, job.first_raw);
      } else if (job.copy.phase == PoolMeshPhase::Vertices && job.uv.buffer.handle != 0u) {
        // The UVs are a stream of their own: the vertex bytes wait for the uv read.
        job.vertex_bytes.assign(data, data + job.copy.size);
      } else if (job.copy.phase == PoolMeshPhase::Uvs) {
        job.error = DecodePoolMeshVertices(job.vertex_bytes.data(), job.vertex_bytes.size(), job.stride, job.pos_offset, job.pos_format,
                                           static_cast<int32_t>(job.uv.element_offset), job.uv.format, job.indices, job.min_vertex,
                                           job.max_vertex, &job.mesh, data, job.copy.size, job.uv.stride);
      } else {
        job.error = DecodePoolMeshVertices(data, job.copy.size, job.stride, job.pos_offset, job.pos_format, job.uv_offset,
                                           job.uv_format, job.indices, job.min_vertex, job.max_vertex, &job.mesh);
      }
    }
    device->unmap_buffer_region(mesh_buffer);
    void* poison_mapped = nullptr;
    if (poison && device->map_buffer_region(mesh_buffer, 0u, kPoolMeshSlotBytes, reshade::api::map_access::write_only,
                                            &poison_mapped)
        && poison_mapped != nullptr) {
      std::fill_n(static_cast<uint32_t*>(poison_mapped), kPoolMeshSlotBytes / sizeof(uint32_t), kPoolPoisonWord);
      device->unmap_buffer_region(mesh_buffer);
    }
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
    const uint64_t cycle = ++g_pool.residue_cycle[slot_index];
    slot.used = 0u;
    slot.follow_used = 0u;
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
      if (job.staging.poisoned) ClassifyPoolStagingHead(job.staging, job.copy.mesh_key, cycle);
      if (current && meshes_ok) {
        PoolMeshRequest& read = request->second;
        read.staging = job.staging;
        if (job.copy.phase == PoolMeshPhase::Indices) {
          read.min_vertex = job.min_vertex;
          read.max_vertex = job.max_vertex;
          read.raw_index_min = job.raw_index_min;
          read.raw_index_max = job.raw_index_max;
          std::memcpy(read.first_raw, job.first_raw, sizeof(read.first_raw));
          read.indices_raw_hash = job.raw_hash;
        } else if (job.copy.phase == PoolMeshPhase::Vertices) {  // the uv read keeps the vertex hash
          read.vertices_raw_hash = job.raw_hash;
        }
      }
      if (!current) {
        g_pool.stats.mesh_dropped += 1u;
        outcome = "dropped (mesh released or re-queued)";
      } else if (!meshes_ok) {
        // Staging not readable: back in line for another copy.
        PoolMeshRequest& queued = request->second;
        if (job.copy.phase != PoolMeshPhase::Indices) queued.indices = std::move(job.indices);
        if (job.copy.phase == PoolMeshPhase::Uvs) queued.vertex_bytes = std::move(job.vertex_bytes);
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
      } else if (job.copy.phase == PoolMeshPhase::Vertices && job.uv.buffer.handle != 0u) {
        uint64_t held = job.vertex_bytes.size();  // vertex bytes held by the requests in the Uvs phase
        for (const auto& entry : g_pool.mesh_requests) held += entry.second.vertex_bytes.size();
        if (held > g_pool.alpha_held_bytes_max) {
          outcome = "held vertex bytes over the cap";
          FailPoolMesh(request, outcome);
        } else {
          PoolMeshRequest& queued = request->second;
          queued.indices = std::move(job.indices);
          queued.vertex_bytes = std::move(job.vertex_bytes);
          queued.phase = PoolMeshPhase::Uvs;
          queued.in_flight = false;
          queued.last_frame = frame;
          AddPoolMeshWaiting(queued);
          outcome = "read, uv copy next";
        }
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
        const PoolIndirectAlpha* alpha_side = copy.alpha_index != UINT32_MAX ? &indirect_alpha[copy.alpha_index] : nullptr;
        if ((copy.pass & kPoolPassAlpha) != 0u) {
          NotePoolAlphaMaterial(mesh_key, copy.alpha, alpha_side != nullptr && alpha_side->vbs_known ? &alpha_side->vbs : nullptr);
        }
        if ((copy.pass & kPoolPassWind) != 0u) g_pool.wind_keys.insert(mesh_key);
        QueuePoolMesh(mesh_key, copy.vs_hash, record, true, alpha_side != nullptr ? alpha_side->uv : PoolUvStream{});
        if (alpha_side != nullptr && alpha_side->source != 0u && (AlphaMode() & 2u) != 0u) {
          AttachPoolAlphaSource(mesh_key, alpha_side->source);
        }
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
        if ((copy.pass & kPoolPassCamera) != 0u) {
          FollowPoolMovingInstance(mesh_key, copy.vs_hash, world, prev_world, copy.follow, copy.frame, verdict);
        }
        ObservePoolInstance(mesh_key, copy.vs_hash, world, copy.frame, &sighting, &visibility[index]);
      }
      const auto schedule = g_pool.schedule.find(copy.schedule_key);
      if (schedule != g_pool.schedule.end()) {
        const auto dynamic = g_pool.dynamic_keys.find(mesh_key);
        schedule->second.follow = (copy.pass & kPoolPassCamera) != 0u && PoolFollowMode()
                                  && dynamic != g_pool.dynamic_keys.end() && dynamic->second.moving_now;
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
  const PoolCpuTimer cpu_timer{&g_pool.cpu_drain_us};
  g_pool.cpu_hook_frame_us = g_pool.cpu_hook_us.exchange(0u);
  g_pool.cpu_drain_frame_us = g_pool.cpu_drain_us.exchange(0u);
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
    ApplyPoolAlphaSwitch();
    ApplyPoolIndirectAlphaSwitch();
    ApplyPoolWindSwitch();
    if (scanning) {
      if (g_pool.stats.scan_first_frame == 0u) g_pool.stats.scan_first_frame = frame;
      g_pool.stats.scan_last_frame = frame;
      if (!g_pool.scan_continuous) g_pool.scan_since = frame;
      g_pool.scan_continuous = true;
    } else {
      g_pool.scan_continuous = false;
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
  std::string summary;
  std::vector<std::string> failure_lines;
  {
    std::lock_guard<std::mutex> lock(g_pool.mutex);
    if ((frame % 60u) == 0u) {
      PrunePool(frame);
      if (scanning && (frame % 600u) == 0u && g_pool.log_captures.load(std::memory_order_relaxed)) {
        const PoolStats& s = g_pool.stats;
        summary = renodx::utils::log::BuildString(
            "falcom_world::pool: frame ", frame, " summary: meshes ", s.meshes, " ok, ", s.mesh_queue, " waiting, ",
            s.mesh_failures, " failed (", s.mesh_unstable, " unstable, ", s.mesh_retries, " retries), verified ",
            s.mesh_verified, ", capture mismatches ", s.mesh_capture_mismatches, "; draws copied ", s.copied_draws,
            ", b1 mismatch ", s.base_mismatch, ", deferred ", s.draws_on_deferred, ", indirect copied ", s.indirect_copied,
            "; instances seen ", s.instances_seen, " observed ", s.observed, " admitted ", s.admitted, " in region ",
            s.region, " following ", s.following, ", rejects matrix ", s.rejected_matrix, " bounds ", s.rejected_bounds,
            " duplicates ", s.dedup_instances, ", near misses ", s.near_misses, ", moving at draw ", s.moving_instances,
            "; moving keys ", s.dynamic_moving_keys, " of ", s.dynamic_live_keys, ", meshes flagged ", s.dynamic_meshes,
            ", removed ", s.dynamic_retired, ", refused ", s.dynamic_blocked, "; orphans ", s.orphans, " retired ",
            s.orphans_retired, ", follow pose updates ", s.follow_hits, " relinks ", s.follow_relinks, " admits ",
            s.follow_admits, "; map failures ", s.map_failures, " staging failures ", s.staging_failures);
      }
    }
    const auto newest = PoolNewestSeenByKey();
    if ((frame % 10u) == 0u && g_pool.scan_continuous && frame >= g_pool.scan_since + kPoolMovingHoldFrames) {
      RetirePoolOrphans(frame, newest, &failure_lines);
    }
    failure_lines.insert(failure_lines.end(), g_pool.pending_lines.begin(), g_pool.pending_lines.end());
    g_pool.pending_lines.clear();
    UpdatePoolStats(newest);
    const uint32_t invalidations = g_pool.stats.resource_invalidations;
    released = invalidations >= g_pool.logged_invalidations ? invalidations - g_pool.logged_invalidations : invalidations;
    g_pool.logged_invalidations = invalidations;
    mesh_waiting = g_pool.stats.mesh_queue;
    for (auto& [mesh_key, failure] : g_pool.failed_meshes) {
      if (g_pool.mesh_fail_lines_logged >= kPoolMeshFailLogLines) break;
      if (failure.logged) continue;
      failure.logged = true;
      g_pool.mesh_fail_lines_logged += 1u;
      failure_lines.push_back(renodx::utils::log::BuildString(
          "falcom_world::pool: mesh ", renodx::utils::log::AsHex(mesh_key), " failed: VS ",
          renodx::utils::log::AsHex(failure.vs_hash), ": ", failure.reason, " | captures ", failure.captures,
          ", VB usage ", failure.vb_usage, " flags ", failure.vb_flags, ", IB usage ", failure.ib_usage, " flags ",
          failure.ib_flags, " | written ", failure.written, ", vb size ", failure.vb_size, ", ib size ",
          failure.ib_size, ", vertex range ", failure.min_vertex, "..", failure.max_vertex, ", pos offset ",
          failure.pos_offset));
    }

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
  if (!summary.empty()) renodx::utils::log::i(summary);
  for (const std::string& line : failure_lines) renodx::utils::log::w(line);
  if (trace_done) WritePoolPrevTrace(trace_out);
  if (released != 0u && g_pool.log_captures.load(std::memory_order_relaxed)) {
    renodx::utils::log::i("falcom_world::pool: frame ", frame, ": ", released,
                          " tracked vertex/index buffers released, ", mesh_waiting, " meshes waiting");
  }
}

inline void ResetWorldPool() {
  std::lock_guard<std::mutex> lock(g_pool.mutex);
  g_pool.wind_keys.clear();
  g_pool.alpha_maps = PoolAlphaDumpMaps{};
  for (auto& slot : g_pool.slots) {
    // Copies already recorded into a slot still land there; only forget them.
    if (!slot.resolving) {
      for (const DrawRecord& draw : slot.indirect_draws) ReleasePoolIndirectRefs(draw);
      slot.copies.clear();
      slot.indirect_draws.clear();
      slot.indirect_alpha.clear();
      slot.used = 0u;
      slot.follow_used = 0u;
      slot.mesh_copies.clear();
      slot.mesh_used = 0u;
    }
  }
  g_pool.mesh_requests.clear();
  g_pool.mesh_waiting.clear();
  g_pool.mesh_queued.clear();
  g_pool.mesh_retry.clear();
  g_pool.failed_meshes.clear();
  g_pool.mismatch_meshes.clear();
  g_pool.pending_lines.clear();
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
  g_pool.alpha_keys.clear();
  g_pool.alpha_uv_requeued.clear();
  DrainPoolAlphaSources();
  g_pool.alpha_off_applied = false;
  g_pool.prev_trace = {};
  g_pool.logged_invalidations = 0u;
  g_pool.mismatch_lines_logged = 0u;
  g_pool.mesh_fail_lines_logged = 0u;
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
        << ", \"retired_instances\": " << entry.retired_instances << ", \"retired_orphans\": " << entry.retired_orphans
        << ", \"blocked\": " << entry.blocked << ", \"follows\": " << entry.follows
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

// poison_staging diagnostics of one read (world_pool.json "staging").
inline void WritePoolStagingDiag(std::ostream& out, const PoolStagingDiag& diag) {
  static constexpr char kHex[] = "0123456789abcdef";
  out << "{\"slot\": " << diag.slot << ", \"ordinal\": " << diag.ordinal << ", \"staging_offset\": " << diag.staging_offset
      << ", \"copied\": " << diag.copied << ", \"begin_mod16\": " << diag.begin_mod16
      << ", \"sentinel_words\": " << diag.sentinel_words << ", \"sentinel_head\": " << diag.sentinel_head
      << ", \"head_words\": " << diag.head_words << ", \"head_hash\": \"" << renodx::utils::log::AsHex(diag.head_hash)
      << "\", \"head_class\": \"" << diag.head_class << "\", \"head\": \"";
  for (uint32_t k = 0; k < diag.head_bytes; ++k) out << kHex[diag.head[k] >> 4] << kHex[diag.head[k] & 15u];
  out << "\"}";
}

// world_pool.json: the captures of a mesh, oldest first.
inline void WritePoolCaptureRecords(std::ostream& out, const std::vector<PoolCaptureRecord>& records) {
  out << "[";
  for (size_t i = 0; i < records.size(); ++i) {
    const PoolCaptureRecord& record = records[i];
    if (i != 0u) out << ", ";
    out << "{\"frame\": " << record.frame
        << ", \"indices_frame\": " << record.indices_frame
        << ", \"vertices_frame\": " << record.vertices_frame
        << ", \"indices_served_indirect\": " << (record.indices_served_indirect ? "true" : "false")
        << ", \"vertices_served_indirect\": " << (record.vertices_served_indirect ? "true" : "false")
        << ", \"indices_source_offset\": " << record.indices_source_offset
        << ", \"vertices_source_offset\": " << record.vertices_source_offset
        << ", \"min_vertex\": " << record.min_vertex << ", \"max_vertex\": " << record.max_vertex
        << ", \"indices_raw_hash\": \"" << renodx::utils::log::AsHex(record.indices_raw_hash) << "\""
        << ", \"vertices_raw_hash\": \"" << renodx::utils::log::AsHex(record.vertices_raw_hash) << "\""
        << ", \"signature\": \"" << renodx::utils::log::AsHex(record.signature) << "\""
        << ", \"index_signature\": \"" << renodx::utils::log::AsHex(record.index_signature) << "\""
        << ", \"written\": " << (record.written ? "true" : "false")
        << ", \"vertices\": " << record.vertices << ", \"triangles\": " << record.triangles
        << ", \"first_diff_triangle\": " << record.first_diff_triangle
        << ", \"differing_triangles\": " << record.differing_triangles << ", \"before\": [";
    for (size_t k = 0; k < 9u; ++k) out << (k != 0u ? ", " : "") << record.before[k];
    out << "], \"after\": [";
    for (size_t k = 0; k < 9u; ++k) out << (k != 0u ? ", " : "") << record.after[k];
    out << "]";
    if (record.staging.poisoned) {
      out << ", \"staging\": ";
      WritePoolStagingDiag(out, record.staging);
    }
    out << "}";
  }
  out << "]";
}

// Quotes and backslashes escaped, control characters dropped (JSON string body).
inline std::string PoolJsonEscape(const std::string& text) {
  std::string escaped;
  for (const char c : text) {
    if (c == '"' || c == '\\') escaped.push_back('\\');
    if (static_cast<unsigned char>(c) >= 0x20u) escaped.push_back(c);
  }
  return escaped;
}

struct PoolAlphaKeyDump {
  uint64_t key = 0u;
  int64_t mesh_id = -1;  // -1: no mesh yet
  PoolAlphaState state;
};

inline const char* PoolUvVerdictName(PoolUvVerdict verdict) {
  switch (verdict) {
    case PoolUvVerdict::NotEvaluated:              return "not_evaluated";
    case PoolUvVerdict::Ok:                        return "ok";
    case PoolUvVerdict::NoTexcoord:                return "no_texcoord";
    case PoolUvVerdict::TexcoordOtherSlot:         return "texcoord_other_slot";
    case PoolUvVerdict::TexcoordUnsupportedFormat: return "texcoord_unsupported_format";
    case PoolUvVerdict::TexcoordBeyondStride:      return "texcoord_beyond_stride";
  }
  return "unknown";
}

// Dump order: the flagged entries (alpha or wind rest pose) first, the rest after; each group keeps its order.
inline std::vector<size_t> PoolDumpOrder(const std::vector<uint8_t>& first) {
  std::vector<size_t> order(first.size());
  for (size_t i = 0; i < order.size(); ++i) order[i] = i;
  std::stable_partition(order.begin(), order.end(), [&first](size_t i) { return first[i] != 0u; });
  return order;
}

template <typename T>
inline std::vector<T> PoolReorder(const std::vector<T>& values, const std::vector<size_t>& order) {
  std::vector<T> out;
  out.reserve(order.size());
  for (const size_t i : order) out.push_back(values[i]);
  return out;
}

inline void WritePoolAlphaMaterial(std::ostream& out, const PoolAlphaMaterial& material) {
  out << "{\"view\": \"" << renodx::utils::log::AsHex(material.view) << "\", \"resource\": \""
      << renodx::utils::log::AsHex(material.resource) << "\", \"threshold\": " << PoolJsonFloat{material.threshold}
      << ", \"scroll\": [" << PoolJsonFloat{material.scroll[0]} << ", " << PoolJsonFloat{material.scroll[1]}
      << "], \"swizzle\": " << material.swizzle << "}";
}

// The alpha evidence of a key or a mesh, as fields of its object (leading comma each).
inline void WritePoolAlphaEvidence(std::ostream& out, const PoolAlphaMaterialState& state, const PoolAlphaLayout& layout) {
  const uint32_t element_count = (std::min)(layout.element_total, kPoolLayoutElementsMax);
  out << ", \"uv_verdict\": \"" << PoolUvVerdictName(layout.verdict) << "\""
      << ", \"uv\": {\"exists\": " << (layout.uv_exists ? "true" : "false") << ", \"slot\": " << layout.uv_slot
      << ", \"offset\": " << layout.uv_offset << ", \"format\": " << layout.uv_format << "}"
      << ", \"vertex_stride\": " << layout.vertex_stride << ", \"layout_element_total\": " << layout.element_total
      << ", \"layout_elements\": [";
  for (uint32_t i = 0; i < element_count; ++i) {
    const PoolLayoutElement& element = layout.elements[i];
    out << (i != 0u ? ", " : "") << "{\"semantic\": \"" << element.semantic << "\", \"index\": " << element.index
        << ", \"slot\": " << element.slot << ", \"offset\": " << element.offset << ", \"format\": " << element.format << "}";
  }
  out << "], \"vbs_known\": " << (layout.vbs_known ? "true" : "false") << ", \"vertex_buffer_slots\": [";
  for (uint32_t i = 0; i < layout.vbs.count; ++i) {
    const PoolVertexBufferSlot& slot = layout.vbs.slots[i];
    out << (i != 0u ? ", " : "") << "{\"slot\": " << slot.slot << ", \"handle\": \"" << renodx::utils::log::AsHex(slot.handle)
        << "\", \"stride\": " << slot.stride << ", \"offset\": " << slot.offset << "}";
  }
  out << "], \"conflict_fields\": " << static_cast<uint32_t>(state.conflict_fields) << ", \"conflict_first\": ";
  WritePoolAlphaMaterial(out, state.material);
  out << ", \"conflict_incoming\": ";
  WritePoolAlphaMaterial(out, state.conflict_incoming);
}

inline void DumpWorldPool() {
  std::vector<WorldMesh> meshes;
  std::vector<WorldInstance> instances;
  std::vector<PoolCameraVisibility> visibility;  // per instance
  std::unordered_map<uint32_t, PoolFamilyStats> families;
  PoolStats stats;
  uint64_t alpha_key_count = 0u;
  uint64_t alpha_held_vertex_bytes = 0u;  // vertex bytes held by requests in the Uvs phase
  PoolAlphaGpuStats alpha_gpu;
  std::vector<PoolAlphaKeyDump> alpha_keys;
  std::vector<std::pair<uint64_t, int64_t>> wind_keys_dump;  // wind rest-pose keys with their mesh ids (-1: no mesh)
  PoolAlphaDumpMaps alpha_maps;
  size_t wind_alpha_overlap = 0u;  // wind rest-pose keys that are also alpha keys
  std::unordered_set<uint64_t> sourced_uids;                 // mesh uids with a source copy
  std::vector<uint8_t> instance_alpha, instance_wind;        // per instance (the instance order of the dump)
  PoolMotionDetail motion_detail;
  std::vector<PoolDynamicDumpEntry> dynamic;
  std::unordered_map<uint64_t, PoolMeshFailure> failed_meshes;
  std::unordered_map<uint64_t, PoolMeshMismatch> mismatch_meshes;
  std::vector<uint32_t> last_seen;  // per instance (PoolInstanceLastSeen)
  std::unordered_map<uint64_t, PoolMeshRetry> mesh_retry;
  PoolRegion region;
  PoolInspectRecord inspect;
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
    UpdatePoolStats(PoolNewestSeenByKey());
    meshes = g_pool.meshes;
    instances = g_pool.instances;
    visibility.reserve(instances.size());
    for (const WorldInstance& instance : instances) {
      visibility.push_back(GetPoolCameraVisibility(instance));
      last_seen.push_back(PoolInstanceLastSeen(instance));
      instance_alpha.push_back(instance.mesh_id < g_pool.meshes.size() && g_pool.meshes[instance.mesh_id].alpha ? 1u : 0u);
      instance_wind.push_back(g_pool.wind_keys.count(instance.mesh_key) != 0u ? 1u : 0u);
    }
    families = g_pool.families;
    stats = g_pool.stats;
    alpha_key_count = g_pool.alpha_keys.size();
    for (const auto& entry : g_pool.mesh_requests) alpha_held_vertex_bytes += entry.second.vertex_bytes.size();
    alpha_gpu = g_pool.alpha_gpu;
    alpha_maps = g_pool.alpha_maps;
    for (const WorldMesh& mesh : g_pool.meshes) {
      if (g_pool.alpha_key_source.count(mesh.mesh_key) != 0u) sourced_uids.insert(mesh.uid);
    }
    for (const uint64_t key : g_pool.wind_keys) {
      const auto mesh_it = g_pool.mesh_by_key.find(key);
      wind_keys_dump.emplace_back(key, mesh_it == g_pool.mesh_by_key.end() ? int64_t{-1} : static_cast<int64_t>(mesh_it->second));
      if (g_pool.alpha_keys.count(key) != 0u) wind_alpha_overlap += 1u;
    }
    for (const auto& [key, state] : g_pool.alpha_keys) {
      const auto mesh_it = g_pool.mesh_by_key.find(key);
      alpha_keys.push_back({key, mesh_it == g_pool.mesh_by_key.end() ? -1 : static_cast<int64_t>(mesh_it->second), state});
    }
    failed_meshes = g_pool.failed_meshes;
    mismatch_meshes = g_pool.mismatch_meshes;
    mesh_retry = g_pool.mesh_retry;
    motion_detail = g_pool.motion_detail;
    dynamic = SnapshotPoolDynamic();
    region = CurrentPoolRegion();
    inspect = g_pool.inspect;
  }

  // The instance dump lists the alpha and wind rest-pose instances first (visibility, last_seen and flags move together).
  std::vector<uint8_t> instance_first(instances.size());
  for (size_t i = 0; i < instance_first.size(); ++i) instance_first[i] = instance_alpha[i] | instance_wind[i];
  const std::vector<size_t> instance_order = PoolDumpOrder(instance_first);
  instances = PoolReorder(instances, instance_order);
  visibility = PoolReorder(visibility, instance_order);
  last_seen = PoolReorder(last_seen, instance_order);
  instance_alpha = PoolReorder(instance_alpha, instance_order);
  instance_wind = PoolReorder(instance_wind, instance_order);

  std::sort(alpha_keys.begin(), alpha_keys.end(), [](const PoolAlphaKeyDump& a, const PoolAlphaKeyDump& b) { return a.key < b.key; });
  size_t diag_ok = 0u, diag_no_texcoord = 0u, diag_other_slot = 0u, diag_conflict_view = 0u, diag_conflict_threshold = 0u;
  size_t diag_conflict_swizzle = 0u, diag_conflict_scroll = 0u, diag_conflict_any = 0u;
  for (const PoolAlphaKeyDump& entry : alpha_keys) {
    const PoolUvVerdict verdict = entry.state.layout.verdict;
    diag_ok += verdict == PoolUvVerdict::Ok ? 1u : 0u;
    diag_no_texcoord += verdict == PoolUvVerdict::NoTexcoord ? 1u : 0u;
    diag_other_slot += verdict == PoolUvVerdict::TexcoordOtherSlot ? 1u : 0u;
    diag_conflict_view += (entry.state.conflict_fields & kPoolAlphaDiffView) != 0u ? 1u : 0u;
    diag_conflict_threshold += (entry.state.conflict_fields & kPoolAlphaDiffThreshold) != 0u ? 1u : 0u;
    diag_conflict_swizzle += (entry.state.conflict_fields & kPoolAlphaDiffSwizzle) != 0u ? 1u : 0u;
    diag_conflict_scroll += (entry.state.conflict_fields & kPoolAlphaDiffScroll) != 0u ? 1u : 0u;
    diag_conflict_any += entry.state.conflict ? 1u : 0u;
  }
  renodx::utils::log::i("[world-bvh] alpha diag: keys=", alpha_keys.size(), " ok=", diag_ok, " no_texcoord=", diag_no_texcoord,
                        " other_slot=", diag_other_slot, " conflict_view=", diag_conflict_view,
                        " conflict_threshold=", diag_conflict_threshold, " conflict_swizzle=", diag_conflict_swizzle,
                        " conflict_scroll=", diag_conflict_scroll, " conflict_any=", diag_conflict_any);

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

  const uint32_t frame = g_state.frame.load();
  std::ostringstream out;
  out << "{\n";
  out << "  \"schema\": 15,\n";
  out << "  \"generated_frame\": " << frame << ",\n";
  out << "  \"files\": {\"summary\": \"world_pool.json\", \"meshes\": \"world_pool_meshes.json\", \"instances\": \"world_pool_instances.json\"},\n";
  out << "  \"meshes_total\": " << meshes.size() << ", \"instances_total\": " << instances.size()
      << ", \"instances_written\": " << (std::min)(instances.size(), kPoolDumpMaxInstances) << ",\n";
  out << "  \"camera_valid\": " << (camera.valid ? "true" : "false") << ",\n";
  out << "  \"alpha\": {\"alpha_foliage\": " << (g_pool.alpha_foliage.load() ? "true" : "false") << ", \"draws\": " << stats.alpha_draws
      << ", \"cb_unavailable\": " << stats.alpha_cb_unavailable << ", \"conflicts\": " << stats.alpha_conflicts
      << ", \"meshes_conflicted\": " << stats.alpha_meshes_conflicted
      << ", \"refused_off\": " << stats.alpha_refused_off << ", \"conflict_refused\": " << stats.alpha_conflict_refused
      << ", \"no_uv\": " << stats.alpha_no_uv << ", \"uv_stream_mismatch\": " << stats.alpha_uv_stream_mismatch
      << ", \"held_vertex_bytes\": " << alpha_held_vertex_bytes << ", \"held_vertex_bytes_max\": " << g_pool.alpha_held_bytes_max
      << ", \"removed\": " << stats.alpha_removed
      << ", \"keys\": " << alpha_key_count
      << ", \"billboard_draws\": " << stats.draws_by_vs_class[static_cast<size_t>(contract::VsClass::Billboard)]
      << ", \"wind_opaque_skips\": " << stats.skips[static_cast<size_t>(PoolSkip::WindOpaque)] << "},\n";
  out << "  \"alpha_gpu\": {\"slices_used\": " << alpha_gpu.slices_used << ", \"slices_total\": " << kAlphaAtlasSlices
      << ", \"blits\": " << alpha_gpu.blits << ", \"blits_frame\": " << alpha_gpu.blits_frame
      << ", \"proxies\": " << alpha_gpu.proxies << ", \"proxy_bytes\": " << alpha_gpu.proxy_bytes
      << ", \"copies\": " << stats.alpha_source_copies << ", \"source_refused\": " << stats.alpha_source_refused
      << ", \"source_refused_bytes\": " << stats.alpha_source_refused_bytes
      << ", \"source_refused_format\": " << stats.alpha_source_refused_format
      << ", \"source_refused_view\": " << stats.alpha_source_refused_view
      << ", \"source_refused_cap\": " << stats.alpha_source_refused_cap
      << ", \"source_refused_deferred\": " << stats.alpha_source_refused_deferred
      << ", \"source_refused_type\": " << stats.alpha_source_refused_type
      << ", \"source_refused_failed\": " << stats.alpha_source_refused_failed
      << ", \"uv_requeues\": " << stats.alpha_uv_requeues
      << ", \"cap_refused\": " << alpha_gpu.cap_refused << ", \"tlas_alpha_instances\": " << alpha_gpu.tlas_instances
      << ", \"alpha_waiting\": " << alpha_gpu.waiting << ", \"alpha_tests\": " << alpha_gpu.tests
      << ", \"alpha_cut\": " << alpha_gpu.cut << ", \"blit_ms\": " << PoolJsonFloat{alpha_gpu.blit_ms}
      << ", \"trace_ms\": " << PoolJsonFloat{alpha_gpu.trace_ms}
      << ", \"indirect_copies\": " << stats.alpha_indirect_copies << ", \"orphans_attached\": " << stats.alpha_orphans_attached
      << ", \"orphans_expired\": " << stats.alpha_orphans_expired << ", \"orphan_missed\": " << stats.alpha_orphan_missed
      << ", \"indirect_source_changed\": " << stats.alpha_indirect_source_changed
      << ", \"wind_refused_off\": " << stats.wind_refused_off << ", \"wind_rest_draws\": " << stats.wind_rest_draws << "},\n";
  out << "  \"alpha_keys_total\": " << alpha_keys.size() << ", \"alpha_keys\": [";
  for (size_t i = 0; i < alpha_keys.size() && i < kPoolDumpMaxAlphaKeys; ++i) {
    const PoolAlphaKeyDump& entry = alpha_keys[i];
    out << (i != 0u ? "," : "") << "\n    {\"key\": \"" << renodx::utils::log::AsHex(entry.key) << "\", \"mesh_id\": " << entry.mesh_id;
    WritePoolAlphaEvidence(out, entry.state, entry.state.layout);
    out << "}";
  }
  out << "\n  ],\n";
  out << "  \"wind_keys_total\": " << wind_keys_dump.size() << ", \"wind_keys_and_alpha_keys\": " << wind_alpha_overlap
      << ", \"wind_keys\": [";
  for (size_t i = 0; i < wind_keys_dump.size() && i < kPoolDumpMaxAlphaKeys; ++i) {
    out << (i != 0u ? "," : "") << "\n    {\"key\": \"" << renodx::utils::log::AsHex(wind_keys_dump[i].first)
        << "\", \"mesh_id\": " << wind_keys_dump[i].second << "}";
  }
  out << "\n  ],\n";
  out << "  \"camera_position\": [" << camera.position[0] << ", " << camera.position[1] << ", " << camera.position[2] << "],\n";
  out << "  \"scene\": {\"fade_constants\": " << (scene_fade ? "true" : "false") << ", \"near_fade_floor\": "
      << PoolJsonFloat{near_fade_floor} << ", \"map_alpha\": " << PoolJsonFloat{map_alpha} << "},\n";
  out << "  \"switches\": {\"capture_meshes\": " << (g_pool.capture_meshes.load() ? "true" : "false")
      << ", \"scan_indirect\": " << (g_pool.scan_indirect.load() ? "true" : "false")
      << ", \"verify_meshes\": " << (g_pool.verify_meshes.load() ? "true" : "false")
      << ", \"legacy_scale\": " << (g_pool.legacy_scale.load() ? "true" : "false")
      << ", \"exclude_moving\": " << (g_pool.exclude_moving.load() ? "true" : "false")
      << ", \"follow_moving\": " << (g_pool.follow_moving.load() ? "true" : "false")
      << ", \"retry_unstable\": " << (g_pool.retry_unstable.load() ? "true" : "false")
      << ", \"alpha_indirect_source\": " << (g_pool.alpha_indirect_source.load() ? "true" : "false")
      << ", \"alpha_wind_opaque\": " << (g_pool.alpha_wind_opaque.load() ? "true" : "false") << "},\n";
  out << "  \"follow\": {\"hits\": " << g_pool.stats.follow_hits << ", \"admits\": " << g_pool.stats.follow_admits
      << ", \"misses_skipped\": " << g_pool.stats.follow_misses_skipped
      << ", \"rejected_bounds\": " << g_pool.stats.follow_rejected_bounds
      << ", \"budget_skips\": " << g_pool.stats.follow_budget_skips << ", \"orphans\": " << g_pool.stats.orphans
      << ", \"relinks\": " << g_pool.stats.follow_relinks << ", \"orphans_retired\": " << g_pool.stats.orphans_retired
      << ", \"max_slot_used\": " << g_pool.stats.max_slot_used << ", \"max_follow_used\": " << g_pool.stats.max_follow_used << "},\n";
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
      << ", \"mesh_retries\": " << stats.mesh_retries
      << ", \"meshes_admitted_by_retry\": " << stats.meshes_admitted_by_retry
      << ", \"tracked_buffer_writes\": " << stats.tracked_buffer_writes
      << ", \"write_events\": {\"update\": " << g_pool.write_events.update.load(std::memory_order_relaxed)
      << ", \"update_cmd\": " << g_pool.write_events.update_cmd.load(std::memory_order_relaxed)
      << ", \"map\": " << g_pool.write_events.map.load(std::memory_order_relaxed)
      << ", \"copy_region\": " << g_pool.write_events.copy_region.load(std::memory_order_relaxed)
      << ", \"copy_resource\": " << g_pool.write_events.copy_resource.load(std::memory_order_relaxed)
      << ", \"tracked\": " << g_pool.write_events.tracked.load(std::memory_order_relaxed) << "}"
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
  out << "  \"last_mesh_error\": \"" << PoolJsonEscape(stats.last_mesh_error) << "\",\n";

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
        << ", \"near_fade\": " << ((registry_entries.pixel[i].flags & contract::kTraitNearFade) != 0u ? "true" : "false");
    if (registry_entries.pixel[i].cls == static_cast<uint8_t>(contract::PsClass::AlphaTested)) {
      out << ", \"alpha_reason\": \""
          << contract::AlphaMaterialReasonName(static_cast<contract::AlphaMaterialReason>(registry_entries.pixel[i].alpha_reason))
          << "\", \"alpha_threshold_offset\": " << registry_entries.pixel[i].alpha_offset;
    }
    out << "}";
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

  std::ostringstream meshes_out;
  meshes_out << "{\n  \"schema\": 15,\n  \"generated_frame\": " << frame << ",\n  \"meshes\": [";
  first = true;
  for (const auto& mesh : meshes) {
    if (!first) meshes_out << ",";
    first = false;
    const auto slice_it = alpha_maps.slice_of_uid.find(mesh.uid);
    const bool resident = alpha_maps.resident_uids.count(mesh.uid) != 0u;
    meshes_out << "\n    {\"mesh_id\": " << mesh.mesh_id
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
        << ", \"alpha\": " << (mesh.alpha ? "true" : "false") << ", \"uvs\": " << mesh.uvs.size()
        << ", \"alpha_conflict\": " << (mesh.alpha_state.conflict ? "true" : "false")
        << ", \"camera_draws\": " << mesh.camera_draws
        << ", \"near_fade_draws\": " << mesh.camera_near_fade_draws
        << ", \"light_draws\": " << mesh.light_draws
        << ", \"camera_ps\": \"" << PoolHashText(mesh.camera_ps_hash) << "\""
        << ", \"bbox_min\": [" << mesh.bbox_min[0] << ", " << mesh.bbox_min[1] << ", " << mesh.bbox_min[2] << "]"
        << ", \"bbox_max\": [" << mesh.bbox_max[0] << ", " << mesh.bbox_max[1] << ", " << mesh.bbox_max[2] << "]"
        << ", \"dynamic\": " << (mesh.dynamic ? "true" : "false")
        << ", \"key\": \"" << renodx::utils::log::AsHex(mesh.mesh_key) << "\""
        << ", \"source_vb\": \"" << renodx::utils::log::AsHex(mesh.source_vb) << "\""
        << ", \"source_ib\": \"" << renodx::utils::log::AsHex(mesh.source_ib) << "\""
        << ", \"signature\": \"" << renodx::utils::log::AsHex(mesh.signature) << "\""
        << ", \"has_source\": " << (sourced_uids.count(mesh.uid) != 0u ? "true" : "false")
        << ", \"slice\": " << (slice_it != alpha_maps.slice_of_uid.end() ? static_cast<int64_t>(slice_it->second) : int64_t{-1})
        << ", \"resident\": " << (resident ? "true" : "false")
        << ", \"tlas_waiting\": " << (mesh.alpha && resident && slice_it == alpha_maps.slice_of_uid.end() ? "true" : "false")
        << ", \"indirect_sourced\": " << (alpha_maps.indirect_uids.count(mesh.uid) != 0u ? "true" : "false");
    if (mesh.alpha) {
      // The mesh's layout is its first key's layout (the layout lives with the keys, not the mesh).
      PoolAlphaLayout mesh_layout;
      for (const PoolAlphaKeyDump& entry : alpha_keys) {
        if (entry.mesh_id != static_cast<int64_t>(mesh.mesh_id) || !entry.state.layout.valid) continue;
        mesh_layout = entry.state.layout;
        break;
      }
      WritePoolAlphaEvidence(meshes_out, mesh.alpha_state, mesh_layout);
    }
    meshes_out << "}";
  }
  meshes_out << "\n  ]\n}\n";

  std::ostringstream instances_out;
  instances_out << "{\n  \"schema\": 15,\n  \"generated_frame\": " << frame << ",\n  \"instances\": [";
  first = true;
  for (size_t i = 0; i < instances.size() && i < kPoolDumpMaxInstances; ++i) {
    const WorldInstance& instance = instances[i];
    const PoolCameraVisibility& seen = visibility[i];
    const float* m = instance.matrix;
    float scale[3] = {};
    for (int k = 0; k < 3; ++k) scale[k] = std::sqrt(m[k] * m[k] + m[4 + k] * m[4 + k] + m[8 + k] * m[8 + k]);
    const uint64_t uid = instance.mesh_id < meshes.size() ? meshes[instance.mesh_id].uid : 0u;
    const auto slice_it = alpha_maps.slice_of_uid.find(uid);
    if (!first) instances_out << ",";
    first = false;
    instances_out << "\n    {\"id\": " << instance.id << ", \"mesh_id\": " << instance.mesh_id
        << ", \"mesh_key\": \"" << renodx::utils::log::AsHex(instance.mesh_key) << "\""
        << ", \"vs_hash\": \"" << PoolHashText(instance.source_vs_hash) << "\""
        << ", \"admit_frame\": " << instance.admit_frame
        << ", \"last_seen\": " << last_seen[i]
        << ", \"dynamic\": " << (instance.dynamic ? "true" : "false")
        << ", \"last_follow_frame\": " << instance.last_follow_frame
        << ", \"origin\": [" << instance.matrix[3] << ", " << instance.matrix[7] << ", " << instance.matrix[11] << "]"
        << ", \"bounds_min\": [" << instance.bounds_min[0] << ", " << instance.bounds_min[1] << ", " << instance.bounds_min[2] << "]"
        << ", \"bounds_max\": [" << instance.bounds_max[0] << ", " << instance.bounds_max[1] << ", " << instance.bounds_max[2] << "]"
        << ", \"mesh_uid\": " << uid
        << ", \"alpha\": " << (instance_alpha[i] ? "true" : "false")
        << ", \"wind_rest\": " << (instance_wind[i] ? "true" : "false")
        << ", \"slice\": " << (slice_it != alpha_maps.slice_of_uid.end() ? static_cast<int64_t>(slice_it->second) : int64_t{-1})
        << ", \"camera_seen\": " << (seen.camera_seen ? "true" : "false")
        << ", \"light_seen\": " << (seen.light_seen ? "true" : "false")
        << ", \"near_fade\": " << (seen.near_fade ? "true" : "false")
        << ", \"scale\": [" << scale[0] << ", " << scale[1] << ", " << scale[2] << "]}";
  }
  instances_out << "\n  ]\n}\n";

  out << "  \"mesh_failed\": [";
  first = true;
  for (const auto& [key, failure] : failed_meshes) {
    if (!first) out << ",";
    first = false;
    out << "\n    {\"key\": \"" << renodx::utils::log::AsHex(key) << "\", \"reason\": \"" << failure.reason << "\""
        << ", \"vs_hash\": \"" << PoolHashText(failure.vs_hash) << "\""
        << ", \"frame\": " << failure.frame
        << ", \"captures\": " << failure.captures
        << ", \"index_count\": " << failure.index_count
        << ", \"first_index\": " << failure.first_index
        << ", \"base_vertex\": " << failure.base_vertex
        << ", \"vertex_stride\": " << failure.vertex_stride
        << ", \"min_vertex\": " << failure.min_vertex
        << ", \"max_vertex\": " << failure.max_vertex
        << ", \"vb_size\": " << failure.vb_size
        << ", \"ib_size\": " << failure.ib_size
        << ", \"from_indirect\": " << (failure.from_indirect ? "true" : "false")
        << ", \"pos_offset\": " << failure.pos_offset
        << ", \"vb_usage\": " << failure.vb_usage << ", \"vb_flags\": " << failure.vb_flags
        << ", \"ib_usage\": " << failure.ib_usage << ", \"ib_flags\": " << failure.ib_flags
        << ", \"written\": " << (failure.written ? "true" : "false")
        << ", \"phase\": \"" << PoolMeshPhaseName(failure.phase) << "\""
        << ", \"index_size\": " << failure.index_size
        << ", \"instance_count\": " << failure.instance_count
        << ", \"vb_handle\": \"" << renodx::utils::log::AsHex(failure.vb_handle) << "\""
        << ", \"ib_handle\": \"" << renodx::utils::log::AsHex(failure.ib_handle) << "\""
        << ", \"vb_offset\": " << failure.vb_offset << ", \"ib_offset\": " << failure.ib_offset
        << ", \"buffer_key\": \"" << renodx::utils::log::AsHex(failure.buffer_key) << "\""
        << ", \"raw_index_min\": " << failure.raw_index_min << ", \"raw_index_max\": " << failure.raw_index_max
        << ", \"first_raw\": [";
    for (size_t k = 0; k < 8u; ++k) out << (k != 0u ? ", " : "") << failure.first_raw[k];
    out << "], \"raw_hash\": \"" << renodx::utils::log::AsHex(failure.raw_hash) << "\""
        << ", \"history\": ";
    WritePoolCaptureRecords(out, failure.history);
    if (failure.staging.poisoned) {
      out << ", \"staging\": ";
      WritePoolStagingDiag(out, failure.staging);
    }
    out << "}";
  }
  out << "\n  ],\n";

  out << "  \"mesh_mismatches\": [";
  first = true;
  for (const auto& [key, mismatch] : mismatch_meshes) {
    if (!first) out << ",";
    first = false;
    const auto failed = failed_meshes.find(key);
    bool admitted = false;
    for (const WorldMesh& mesh : meshes) admitted = admitted || mesh.mesh_key == key;
    out << "\n    {\"key\": \"" << renodx::utils::log::AsHex(key) << "\", \"draw_frame\": " << mismatch.draw_frame
        << ", \"draw_serial\": " << mismatch.draw_serial << ", \"outcome\": \"";
    if (failed != failed_meshes.end()) {
      out << failed->second.reason;
    } else if (admitted) {
      out << "admitted after " << mismatch.history.size() << " captures";
    } else {
      out << "pending";
    }
    out << "\", \"history\": ";
    WritePoolCaptureRecords(out, mismatch.history);
    out << "}";
  }
  out << "\n  ],\n";


  out << "  \"mesh_retrying\": [";
  first = true;
  for (const auto& [key, retry] : mesh_retry) {
    if (!first) out << ",";
    first = false;
    out << "\n    {\"key\": \"" << renodx::utils::log::AsHex(key) << "\", \"rounds\": " << retry.rounds
        << ", \"next_frame\": " << retry.next_frame << "}";
  }
  out << "\n  ],\n";
  out << "  \"inspect\": {\"set\": " << (inspect.set ? "true" : "false") << ", \"frame\": " << inspect.frame
      << ", \"hit\": " << (inspect.hit ? "true" : "false") << ", \"instance_id\": " << inspect.instance_id
      << ", \"mesh_uid\": " << inspect.mesh_uid << ", \"lines\": [";
  for (size_t i = 0; i < inspect.lines.size(); ++i) {
    out << (i != 0u ? ", " : "") << "\"" << PoolJsonEscape(inspect.lines[i]) << "\"";
  }
  out << "]}\n}\n";

  // Detail files first, the summary last: a summary read finds its detail files written.
  std::string meshes_text = meshes_out.str();
  std::string instances_text = instances_out.str();
  std::string text = out.str();
  renodx::utils::path::WriteTextFile(PoolOutputDir() / "world_pool_meshes.json", meshes_text);
  renodx::utils::path::WriteTextFile(PoolOutputDir() / "world_pool_instances.json", instances_text);
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
