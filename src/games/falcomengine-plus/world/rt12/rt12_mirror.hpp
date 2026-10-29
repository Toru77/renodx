#pragma once

// M3 DXR scene mirror: the live store's hooks queue data here on the present thread; rt12_backend.hpp drains it into the
// D3D12 scene. Std-only (compiled by the harness). Every hook returns at once while `active` is false.

#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <utility>
#include <vector>

namespace falcom_world::rt12 {

// Arena element sizes of the live store (bvh_live.hpp): vertices float4, indices uint, UVs float2.
inline constexpr uint32_t kRt12ArenaVertices = 0u;
inline constexpr uint32_t kRt12ArenaIndices = 1u;
inline constexpr uint32_t kRt12ArenaUvs = 2u;
inline constexpr uint32_t kRt12ArenaStride[3] = {16u, 4u, 8u};
inline constexpr uint32_t kRt12AlphaSlices = 256u;  // must equal kAlphaAtlasSlices (static_assert in rt12_backend.hpp)

struct Rt12MirrorAppend {
  uint32_t arena = 0u;          // kRt12Arena*
  uint64_t first_element = 0u;  // element offset in the live store arena (and in the D3D12 mirror)
  std::vector<uint8_t> bytes;
};

struct Rt12MirrorSlot {
  uint32_t slot = 0u;  // live store slot index (WorldInstanceGPU header.x)
  uint32_t vertex_offset = 0u;
  uint32_t vertex_count = 0u;
  uint32_t index_offset = 0u;
  uint32_t index_count = 0u;
};

// Filled by the live store hooks on the present thread; drained by rt12::OnPresent. Only data moves under the mutex
// (lock rule: no graphics call while it is held).
struct Rt12Mirror {
  std::mutex mutex;
  bool active = false;  // set by rt12 while the D3D12 scene exists; every hook returns at once when false
  bool reset = false;   // DestroyLiveStore ran: everything queued before it was dropped
  std::vector<Rt12MirrorAppend> appends;
  std::vector<Rt12MirrorSlot> new_slots;
  std::vector<uint32_t> retired_slots;
  // Alpha materials: a persistent CPU copy of every slice, kept whether or not `active` is set (the D3D11 slices
  // outlive store resets and may exist before the DXR switch goes on); rt12 uploads all of it when the version moves.
  std::array<std::array<uint8_t, 32>, kRt12AlphaSlices> materials = {};
  uint64_t materials_version = 0u;
  uint64_t sanitized_triangles = 0u;  // total since the scene was created
};
inline Rt12Mirror g_rt12_mirror;

// Triangles of one mesh whose indices leave the mesh or whose vertices are not finite (the compute BLAS skips them,
// bvh_build.hpp BuildMeshBlas) become (0, 0, 0): a degenerate triangle DXR never hits. `vertices` holds the mesh's
// float4 positions (x y z 1). Returns how many triangles were changed. A trailing partial triangle is left as it is.
inline uint32_t SanitizeRt12Indices(const float* vertices, uint32_t vertex_count, uint32_t* indices, uint32_t index_count) {
  uint32_t changed = 0u;
  for (uint32_t t = 0u; t + 3u <= index_count; t += 3u) {
    uint32_t* tri = indices + t;
    bool valid = true;
    for (uint32_t k = 0u; k < 3u && valid; ++k) {
      if (tri[k] >= vertex_count) {
        valid = false;
        continue;
      }
      const float* v = vertices + static_cast<size_t>(tri[k]) * 4u;
      valid = std::isfinite(v[0]) && std::isfinite(v[1]) && std::isfinite(v[2]);
    }
    if (!valid) {
      tri[0] = 0u;
      tri[1] = 0u;
      tri[2] = 0u;
      changed += 1u;
    }
  }
  return changed;
}

inline constexpr uint32_t kRt12MaskOpaqueVisible = 1u;   // RT12_MASK_OPAQUE_VISIBLE
inline constexpr uint32_t kRt12MaskFoliageVisible = 2u;  // RT12_MASK_FOLIAGE_VISIBLE
inline constexpr uint32_t kRt12MaskShadowOnly = 4u;      // RT12_MASK_SHADOW_ONLY
inline constexpr uint32_t kRt12MaskDynamic = 8u;         // RT12_MASK_DYNAMIC: characters (deforming meshes)
inline constexpr uint32_t kRt12DynamicInstanceId = 0xFFFFFFu;  // InstanceID of a character instance (not in the instance table)
inline constexpr uint32_t kRt12CarrierWidth = 4096u;     // carrier texels per row; vertex v at (v % width, v / width)
inline constexpr uint32_t kRt12CarrierHeight = 512u;     // 4096 * 512 vertices = the 24 MB deform arena of float3
inline constexpr uint32_t kRt12DynamicRebuildFrames = 30u;  // full BLAS rebuild cadence (owner decision)
inline constexpr uint32_t kRt12RecordTexels = 12u;          // WorldInstanceGPU = 192 bytes = 12 RGBA32_UINT texels
inline constexpr uint32_t kRt12RecordCarrierHeight = 96u;   // 4096 * 96 texels = 32768 records (kPoolDynamicInstances)
inline constexpr uint32_t kRt12GcRecordCarrierHeight = 216u;  // 4096 * 216 texels = 73728 records (G4 GPU records)

// End (exclusive) of the vertices `count` characters use: max(vertex_base + 3 * triangles), 0 for none.
inline uint64_t Rt12DynamicVertexEnd(const uint32_t* vertex_base, const uint32_t* triangles, size_t count) {
  uint64_t end = 0u;
  for (size_t i = 0u; i < count; ++i) {
    const uint64_t character_end = static_cast<uint64_t>(vertex_base[i]) + 3u * static_cast<uint64_t>(triangles[i]);
    if (character_end > end) end = character_end;
  }
  return end;
}

// Whether a character's BLAS is built from scratch this frame (else refit in place): no BLAS yet, a different
// triangle count than the BLAS was built with, or kRt12DynamicRebuildFrames or more frames since its last build.
inline bool Rt12DynamicNeedsBuild(bool has_blas, uint32_t blas_triangles, uint32_t triangles, uint32_t frames_since_build) {
  return !has_blas || blas_triangles != triangles || frames_since_build >= kRt12DynamicRebuildFrames;
}

// D3D12 instance fields of a live TLAS entry: Transform rows 0..2 = WorldInstanceGPU world floats 0..11 (row-major),
// mask from the visibility flags (visibility_z as stored), flags from the material (header_z as stored).
struct Rt12InstanceFields {
  float transform[12] = {};
  uint32_t mask = 0u;  // kRt12Mask*
  bool opaque = true;  // false: FORCE_NON_OPAQUE (alpha-tested material), true: FORCE_OPAQUE
};

// bvh::kInstance* (bvh_resources.hpp:48-52)
inline Rt12InstanceFields MakeRt12InstanceFields(const float* world16, float visibility_z, float header_z) {
  Rt12InstanceFields fields;
  const uint32_t flags = static_cast<uint32_t>(visibility_z + 0.5f);
  if ((flags & 2u) == 0u) {
    fields.mask = kRt12MaskShadowOnly;
  } else if ((flags & (8u | 16u)) != 0u) {
    fields.mask = kRt12MaskFoliageVisible;
  } else {
    fields.mask = kRt12MaskOpaqueVisible;
  }
  fields.opaque = static_cast<uint32_t>(header_z + 0.5f) == 0u;
  for (uint32_t i = 0u; i < 12u; ++i) {
    fields.transform[i] = world16[i];
  }
  return fields;
}

// The 16 words of the D3D12_RAYTRACING_INSTANCE_DESC that world_rt12_records.cs_6_5.hlsl writes for one P2b record
// (its CPU transcription, tested against MakeRt12InstanceFields and the D3D12 struct in the harness):
// words 0..11 = record floats 4..15 (world rows 0..2, row-major, as raw bits); word 12 = instance_id | mask << 24;
// word 13 = flags << 24 (0x1 TRIANGLE_CULL_DISABLE | 0x4 FORCE_OPAQUE or 0x8 FORCE_NON_OPAQUE);
// words 14..15 = the BLAS address (low, high): blas_address when the record is valid (header.w = record float 3 >= 0.5),
// else (0, 0). The caller passes blas_address = 0 for a slot out of range or without a D3D12 BLAS (the shader's
// g_blas_addresses rule). mask follows MakeRt12InstanceFields (visibility.z = record float 46, header.z = record
// float 2; record float 0 = header.x slot), forced to 0 when the record is invalid or the written address is (0, 0).
inline void Rt12RecordDescWords(const float* record48, uint32_t instance_id, uint64_t blas_address, uint32_t out16[16]) {
  const bool valid = record48[3] >= 0.5f;
  const uint64_t address = valid ? blas_address : 0u;
  const Rt12InstanceFields fields = MakeRt12InstanceFields(record48 + 4, record48[46], record48[2]);
  const uint32_t mask = (valid && address != 0u) ? fields.mask : 0u;
  std::memcpy(out16, fields.transform, sizeof(fields.transform));
  out16[12] = instance_id | (mask << 24);
  out16[13] = (0x1u | (fields.opaque ? 0x4u : 0x8u)) << 24;
  out16[14] = static_cast<uint32_t>(address);
  out16[15] = static_cast<uint32_t>(address >> 32);
}

// G4: one GPU-cache mesh as WorldMeshGPU words (header = vertex offset, count, index offset, count; bbox_min.w = UV offset
// + 1, 0 without UV). The DXR trace reads header.z and bbox_min.w for alpha candidates only. False when an index or UV
// element offset is not exact in a float (2^24): the slot's alpha slice is then withheld.
inline bool Rt12GcMeshWords(uint64_t v_off, uint32_t vertex_count, uint64_t i_off, uint32_t index_count, bool has_uv,
                            uint64_t u_off, float out16[16]) {
  for (uint32_t k = 0u; k < 16u; ++k) out16[k] = 0.f;
  out16[0] = static_cast<float>(v_off);
  out16[1] = static_cast<float>(vertex_count);
  out16[2] = static_cast<float>(i_off);
  out16[3] = static_cast<float>(index_count);
  if (has_uv) out16[7] = static_cast<float>(u_off + 1u);
  constexpr uint64_t kExact = 1ull << 24u;
  return i_off + index_count < kExact && (!has_uv || u_off + vertex_count < kExact);
}

inline void MirrorStoreReset() {
  std::lock_guard lock(g_rt12_mirror.mutex);
  if (!g_rt12_mirror.active) return;
  g_rt12_mirror.reset = true;
  g_rt12_mirror.appends.clear();
  g_rt12_mirror.new_slots.clear();
  g_rt12_mirror.retired_slots.clear();
}

inline void MirrorRetireSlot(uint32_t slot) {
  std::lock_guard lock(g_rt12_mirror.mutex);
  if (!g_rt12_mirror.active) return;
  g_rt12_mirror.retired_slots.push_back(slot);
}

inline void MirrorMaterial(uint32_t slice, const void* bytes32) {
  std::lock_guard lock(g_rt12_mirror.mutex);
  if (slice < kRt12AlphaSlices) {
    std::memcpy(g_rt12_mirror.materials[slice].data(), bytes32, 32u);
    g_rt12_mirror.materials_version += 1u;
  }
}

inline void MirrorMaterialsCleared() {
  std::lock_guard lock(g_rt12_mirror.mutex);
  g_rt12_mirror.materials = {};
  g_rt12_mirror.materials_version += 1u;
}

inline void MirrorUpload(uint64_t vertex_first, const std::vector<float>& vertex_batch, uint64_t index_first,
                         const std::vector<uint32_t>& index_batch, uint64_t uv_first,
                         const std::vector<std::array<float, 2>>& uv_batch, const std::vector<Rt12MirrorSlot>& slots) {
  std::lock_guard lock(g_rt12_mirror.mutex);
  if (!g_rt12_mirror.active) return;
  std::vector<uint32_t> indices = index_batch;
  uint64_t sanitized = 0u;
  for (const Rt12MirrorSlot& slot : slots) {
    sanitized += SanitizeRt12Indices(vertex_batch.data() + (slot.vertex_offset - vertex_first) * 4u, slot.vertex_count,
                                     indices.data() + (slot.index_offset - index_first), slot.index_count);
  }
  g_rt12_mirror.sanitized_triangles += sanitized;

  const auto* vertex_bytes = reinterpret_cast<const uint8_t*>(vertex_batch.data());
  g_rt12_mirror.appends.push_back({kRt12ArenaVertices, vertex_first,
                                   std::vector<uint8_t>(vertex_bytes, vertex_bytes + vertex_batch.size() * sizeof(float))});
  const auto* index_bytes = reinterpret_cast<const uint8_t*>(indices.data());
  g_rt12_mirror.appends.push_back({kRt12ArenaIndices, index_first,
                                   std::vector<uint8_t>(index_bytes, index_bytes + indices.size() * sizeof(uint32_t))});
  if (!uv_batch.empty()) {
    const auto* uv_bytes = reinterpret_cast<const uint8_t*>(uv_batch.data());
    g_rt12_mirror.appends.push_back({kRt12ArenaUvs, uv_first,
                                     std::vector<uint8_t>(uv_bytes, uv_bytes + uv_batch.size() * sizeof(std::array<float, 2>))});
  }
  g_rt12_mirror.new_slots.insert(g_rt12_mirror.new_slots.end(), slots.begin(), slots.end());
}

enum class Rt12RtgiReason : uint32_t {
  Tracing,
  DxrOff,
  NotCreated,
  Failed,
  Stuck,
  ContextMismatch,
  SceneMissing,
  TargetFailed,
  SubmitFailed,
};
inline constexpr uint32_t kRt12RtgiReasonCount = 9u;

inline const char* Rt12RtgiReasonName(Rt12RtgiReason reason) {
  switch (reason) {
    case Rt12RtgiReason::Tracing: return "tracing";
    case Rt12RtgiReason::DxrOff: return "dxr_off";
    case Rt12RtgiReason::NotCreated: return "not_created";
    case Rt12RtgiReason::Failed: return "failed";
    case Rt12RtgiReason::Stuck: return "stuck";
    case Rt12RtgiReason::ContextMismatch: return "context_mismatch";
    case Rt12RtgiReason::SceneMissing: return "scene_missing";
    case Rt12RtgiReason::TargetFailed: return "target_failed";
    case Rt12RtgiReason::SubmitFailed: return "submit_failed";
  }
  return "unknown";
}

struct Rt12RtgiPanel {
  Rt12RtgiReason reason = Rt12RtgiReason::DxrOff;
  uint64_t frames[kRt12RtgiReasonCount] = {};
  uint64_t waits = 0u;
  float wait_ms_max = 0.f;
  uint32_t characters = 0u;
  uint32_t instances = 0u;
  float gpu_ms = 0.f;
  double gpu_ms_sum = 0.0;
  uint64_t gpu_ms_n = 0u;
  float trace_ms = 0.f;
  double trace_ms_sum = 0.0;
  uint64_t trace_ms_n = 0u;
};
inline std::mutex g_rt12_rtgi_panel_mutex;
inline Rt12RtgiPanel g_rt12_rtgi_panel;

inline bool Rt12RtgiReadsSunInputs(float sun_contribution, int debug_mode) {
  return sun_contribution != 1.f || debug_mode == 9;
}

inline bool Rt12RtgiReadsMaterialInputs(float dynamic_light_contribution) {
  return dynamic_light_contribution != 1.f;
}

inline constexpr uint32_t kRt12RtgiDescriptors = 25u;
inline constexpr uint32_t kRt12RtgiTimestamps = 3u;
inline constexpr uint32_t kRt12RtgiSceneRows = 96u;
inline constexpr uint32_t kRt12RtgiLampTexels = 7u;
inline constexpr uint32_t kRt12RtgiDeferredTexels = 9u;
inline constexpr uint32_t kRt12RtgiDeferredMax = 0x4e20u;
inline constexpr uint32_t kRt12RtgiDeferredRows = 44u;
inline constexpr uint32_t kRt12RtgiCarrierWidth = 4096u;

static_assert(kRt12RtgiLampTexels * 16u == 112u);
static_assert(kRt12RtgiDeferredTexels * 16u == 144u);
static_assert(kRt12RtgiSceneRows * 16u % 256u == 0u);
static_assert(7168u * 16u % 256u == 0u);
static_assert(1024u * 4u % 256u == 0u);
static_assert(kRt12RtgiCarrierWidth * 16u % 256u == 0u);
static_assert(kRt12RtgiDeferredRows * kRt12RtgiCarrierWidth >= kRt12RtgiDeferredMax * 9u);

}  // namespace falcom_world::rt12
