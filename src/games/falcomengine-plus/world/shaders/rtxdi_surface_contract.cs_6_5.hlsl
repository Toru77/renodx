#include "world_rtxdi_surface.hlsli"

Texture2D<float> g_depth : register(t0);
Texture2D<uint4> g_mrt0 : register(t1);
Texture2D<uint4> g_scene_rows : register(t2);
RWStructuredBuffer<RtxdiSurfaceResult> g_result : register(u0);

cbuffer cb_contract : register(b0) {
  uint4 g_pixel_size;
};

[numthreads(1, 1, 1)]
void main(uint3 id : SV_DispatchThreadID) {
  uint4 scene_rows[RTXDI_SURFACE_SCENE_ROW_COUNT];
  [unroll] for (uint row = 0u; row < RTXDI_SURFACE_SCENE_ROW_COUNT; ++row) {
    scene_rows[row] = g_scene_rows.Load(int3(row, 0, 0));
  }
  const uint2 pixel = id.xy;
  const float depth = g_depth.Load(int3(pixel, 0));
  const uint4 mrt0 = g_mrt0.Load(int3(pixel, 0));
  g_result[0] = RtxdiReconstructSurface(depth, mrt0, pixel, g_pixel_size.xy, scene_rows);
}
