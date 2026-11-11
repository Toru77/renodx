#define SHARC_UPDATE 0
#include "world_pt_common.hlsli"
// Material cache (M3): g_mat_mode 1 zeroes every entry; 0 writes one pixel per rate x rate cell (phase rotates per frame).
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
  const uint rate = max(g_mat_rate, 1u);
  const uint cells_x = (g_size.x + rate - 1u) / rate;
  const uint phase = g_frame % (rate * rate);
  const uint2 pixel = uint2(id.x % cells_x, id.x / cells_x) * rate + uint2(phase % rate, phase / rate);
  if (any(pixel >= g_size)) return;
  RtxdiSurfaceResult surface;
  if (!PtLoadSurface(pixel, surface)) return;
  const PtMaterial sample = PtGBufferMaterial(pixel);
  uint64_t key;
  uint index;
  if (!HashGridInsertEntry(PtMaterialData(), PtCachePosition(surface.world_position, surface.shading_normal),
                           PtMaterialKeyNormal(surface.shading_normal), PtMaterialGrid(), key, index)) return;
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
