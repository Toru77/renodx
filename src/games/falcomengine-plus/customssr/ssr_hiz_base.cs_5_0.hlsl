///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// falcomengine-plus — Custom SSR (Sora 2nd) — Pass 1a: Hi-Z base level
//
// Converts the captured hardware depth buffer to positive linearized
// view-space Z and writes it into:
//   u0 = SSR Hi-Z chain mip 0 (full resolution, exact per-pixel depth)
//   u1 = scratch texture      (pre-seeded copy of mip 0 so the first reduce
//                              dispatch needs no copy)
//
// Dispatched with ceil(w/8) x ceil(h/8) x 1 groups.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "ssr_common.hlsl"

Texture2D<float>   g_srcHardwareDepth : register(t0);  // game's depthTexture (captured t4)
RWTexture2D<float> g_outHizMip0       : register(u0);
RWTexture2D<float> g_outScratch       : register(u1);

[numthreads(8, 8, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
  uint width, height;
  g_outHizMip0.GetDimensions(width, height);
  if (dispatchThreadID.x >= width || dispatchThreadID.y >= height) return;

  float hw_depth = g_srcHardwareDepth.Load(int3(dispatchThreadID.xy, 0));
  float view_z = SSR_LinearizeDepth(hw_depth);

  g_outHizMip0[dispatchThreadID.xy] = view_z;
  g_outScratch[dispatchThreadID.xy] = view_z;
}
