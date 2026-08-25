///////////////////////////////////////////////////////////////////////////////////////
// falcomengine-plus — Custom SSR (Sora 2nd) — Pass R3b: radiance reduction
//
// Reduces one level of the radiance pyramid per dispatch:
//   t0 = single-mip SRV view of pyramid mip (L-1), bound by the caller
//   u0 = pyramid mip L
//
// Reduction is a 2x2 BOX AVERAGE of the four children — the filtered radiance
// construction used by Frostbite-style filtered importance sampling.
// Deliberately different from the Hi-Z MIN reduce: here the goal is a valid
// mean radiance per footprint, not a conservative intersection bound.
//
// Edge handling: coordinates replicated at borders (no darkening).
// Dispatched once per level with ceil(dstW/8) x ceil(dstH/8) x 1 groups.
///////////////////////////////////////////////////////////////////////////////////////

#include "ssr_common.hlsl"

Texture2D<float4>   g_srcMip : register(t0);   // single-mip view of previous level
RWTexture2D<float4> g_dstMip : register(u0);

float4 SSR_FetchClampedMip(int2 pos, int2 valid_size)
{
  pos = clamp(pos, int2(0, 0), valid_size - int2(1, 1));
  return g_srcMip.Load(int3(pos, 0));
}

[numthreads(8, 8, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
  uint dst_width, dst_height;
  g_dstMip.GetDimensions(dst_width, dst_height);
  if (dispatchThreadID.x >= dst_width || dispatchThreadID.y >= dst_height) return;

  // SSR_scratch_width/height are pushed as the PREVIOUS level's dimensions
  // (same convention as the Hi-Z reduce loop).
  const int2 valid_size = int2(max(SSR_scratch_width, 1.0),
                               max(SSR_scratch_height, 1.0));
  const int2 src = int2(dispatchThreadID.xy) * 2;

  const float4 s00 = SSR_FetchClampedMip(src + int2(0, 0), valid_size);
  const float4 s10 = SSR_FetchClampedMip(src + int2(1, 0), valid_size);
  const float4 s01 = SSR_FetchClampedMip(src + int2(0, 1), valid_size);
  const float4 s11 = SSR_FetchClampedMip(src + int2(1, 1), valid_size);

  g_dstMip[dispatchThreadID.xy] = (s00 + s10 + s01 + s11) * 0.25f;
}
