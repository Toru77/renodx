#define PT_COMMON_PASS 0
#define SHARC_UPDATE 0
#include "world_pt_common.hlsli"
// Material cache (M3): g_mat_mode 1 zeroes every entry; 2 ages every unlocked entry and evicts it at age 15 (A22 M1);
// 0 writes every pixel whose cell is missing and refreshes existing cells from one pixel per rate x rate block (A22.1).
[numthreads(128, 1, 1)]
void main(uint3 id : SV_DispatchThreadID) {
  if (g_mat_mode == 1u) {
    if (id.x < g_mat_capacity) {
      const uint base = id.x * 4u;
      g_pt_material_keys[id.x] = 0;
      g_pt_material_values[base] = 0; g_pt_material_values[base + 1u] = 0; g_pt_material_values[base + 2u] = 0; g_pt_material_values[base + 3u] = 0;
    }
    return;
  }
  if (g_mat_mode == 2u) {
    // Value word 3 bits 27-30 hold the entry's age in eviction steps; the write pass stores age 0.
    // SharcCommon.h:134 sets HASH_GRID_LIMIT_EMPTY_SLOTS = 2: an eviction hole can shadow a later entry; the re-insert fills the hole and the stale copy ages out.
    if (id.x < g_mat_capacity && g_pt_material_keys[id.x] != 0) {
      const uint base = id.x * 4u;
      const uint w = g_pt_material_values[base + 3u];
      if ((w & 0x80000000u) != 0u) return;
      const uint age = ((w >> 27) & 15u) + 1u;
      if (age == 15u) {
        g_pt_material_keys[id.x] = 0;
        g_pt_material_values[base] = 0; g_pt_material_values[base + 1u] = 0; g_pt_material_values[base + 2u] = 0; g_pt_material_values[base + 3u] = 0;
      } else {
        g_pt_material_values[base + 3u] = (w & ~(15u << 27)) | (age << 27);
      }
    }
    return;
  }
  const uint2 pixel = uint2(id.x % g_size.x, id.x / g_size.x);
  if (pixel.y >= g_size.y) return;
  const uint rate = max(g_mat_rate, 1u);
  const uint phase = g_frame % (rate * rate);
  const bool refresh = all(pixel % rate == uint2(phase % rate, phase / rate));
  RtxdiSurfaceResult surface;
  if (!PtLoadSurface(pixel, surface)) return;
  const float3 cache_position = PtCachePosition(surface.world_position, surface.shading_normal);
  const float3 key_normal = PtMaterialKeyNormal(surface.shading_normal);
  uint64_t key;
  if (!refresh) {
    const uint found = HashGridFindEntry(PtMaterialData(), cache_position, key_normal, PtMaterialGrid(), key);
    if (found != HASH_GRID_INVALID_CACHE_INDEX && ((PtReadMaterial(found).w >> 24) & 1u) != 0u) return;
  }
  const PtMaterial sample = PtGBufferMaterial(pixel);
  uint index;
  if (!HashGridInsertEntry(PtMaterialData(), cache_position, key_normal, PtMaterialGrid(), key, index)) return;
  const uint base = index * 4u;
  uint previous;
  InterlockedOr(g_pt_material_values[base + 3u], 0x80000000u, previous);
  if ((previous & 0x80000000u) != 0u) {
    PtCount(3u);
    return;
  }
  DeviceMemoryBarrier();
  const uint4 old = uint4(g_pt_material_values[base], g_pt_material_values[base + 1u],
                          g_pt_material_values[base + 2u], previous);
  PtMaterial m = sample;
  if (((old.w >> 24) & 1u) != 0u) {
    const PtMaterial o = PtUnpackMaterial(old);
    m.albedo = lerp(o.albedo, sample.albedo, 0.25);
    m.roughness = lerp(o.roughness, sample.roughness, 0.25);
    m.f0 = lerp(o.f0, sample.f0, 0.25);
    m.spec_scale = lerp(o.spec_scale, sample.spec_scale, 0.25);
  }
  const uint4 packed = PtPackMaterial(m);
  g_pt_material_values[base] = packed.x;
  g_pt_material_values[base + 1u] = packed.y;
  g_pt_material_values[base + 2u] = packed.z;
  DeviceMemoryBarrier();
  uint ignored;
  InterlockedExchange(g_pt_material_values[base + 3u], packed.w, ignored);
  PtCount(4u);
}
