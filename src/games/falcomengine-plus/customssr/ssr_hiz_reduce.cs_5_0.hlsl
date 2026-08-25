///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// falcomengine-plus — Custom SSR (Sora 2nd) — Pass 1b: Hi-Z MIN reduction
//
// Reduces one level of the SSR depth hierarchy per dispatch:
//   t0 = scratch texture holding level L-1 data in the region
//        [0 .. SSR_scratch_width) x [0 .. SSR_scratch_height)
//   u0 = SSR Hi-Z chain mip L
//
// Reduction is a strict MIN of the 2x2 children — a conservative nearest-surface
// bound, required by the SSSR-style hierarchical traversal. This is deliberately
// different from GTVBAO's weighted-average prefilter, which is not a valid
// bound for ray intersection.
//
// Dispatched once per level with ceil(dstW/8) x ceil(dstH/8) x 1 groups.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "ssr_common.hlsl"

Texture2D<float>   g_srcMip : register(t0);
RWTexture2D<float> g_dstMip : register(u0);

float SSR_FetchClamped(int2 pos, int2 valid_size)
{
  if (pos.x < 0 || pos.y < 0 || pos.x >= valid_size.x || pos.y >= valid_size.y)
    return SSR_FLT_MAX;
  return g_srcMip.Load(int3(pos, 0));
}

[numthreads(8, 8, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
  uint dst_width, dst_height;
  g_dstMip.GetDimensions(dst_width, dst_height);
  if (dispatchThreadID.x >= dst_width || dispatchThreadID.y >= dst_height) return;

  int2 valid_size = int2(max(SSR_scratch_width, 1.0), max(SSR_scratch_height, 1.0));
  int2 src = int2(dispatchThreadID.xy) * 2;

  float d00 = SSR_FetchClamped(src + int2(0, 0), valid_size);
  float d10 = SSR_FetchClamped(src + int2(1, 0), valid_size);
  float d01 = SSR_FetchClamped(src + int2(0, 1), valid_size);
  float d11 = SSR_FetchClamped(src + int2(1, 1), valid_size);

  g_dstMip[dispatchThreadID.xy] = min(min(d00, d10), min(d01, d11));
}
