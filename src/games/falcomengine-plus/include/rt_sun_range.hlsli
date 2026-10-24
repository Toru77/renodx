#ifndef SRC_GAMES_FALCOMENGINE_PLUS_INCLUDE_RT_SUN_RANGE_HLSLI_
#define SRC_GAMES_FALCOMENGINE_PLUS_INCLUDE_RT_SUN_RANGE_HLSLI_

// RT sun cascades (R2): the sun cascade slices RT owns, and the blend at their outer edge.
// Pure functions. The C++ mirror is world/rtsun/rtsun_range.hpp; keep the two in step.
//
// rt = shader_injection_data.rt_sun_cascades (0..4), count = shadowmapCascadeCount_g,
// splits = shadowSplitDistance_g, fadeNear = shadowFadeNear_g, fadeRangeInv = shadowFadeRangeInv_g.
// Cascade i ends at splits[i-1]; the last cascade ends at fadeNear + 1 / fadeRangeInv.

float RtSunBoundary(uint rt, int count, float3 splits, float fadeNear, float fadeRangeInv)
{
  if (rt == 0u || (count != 3 && count != 4) || rt > (uint)count) return 0.0;
  if (rt < (uint)count) return splits[rt - 1u];
  return fadeNear + rcp(max(fadeRangeInv, 1e-6));
}

// 1 inside the RT range, 0 at its outer edge. The inner boundaries blend over 0.2 inverse (5 m),
// the last one over fadeRangeInv.
// RtSunWeight >= 1 is an early-out where the skipped slices would give lit. Fractional weights come from the
// vanilla cross-fade over the cleared slice, so the resolve needs no blend.
float RtSunWeight(float dist, uint rt, int count, float3 splits, float fadeNear, float fadeRangeInv)
{
  const float boundary = RtSunBoundary(rt, count, splits, fadeNear, fadeRangeInv);
  if (boundary == 0.0) return 0.0;
  const float bandInv = rt < (uint)count ? 0.2 : fadeRangeInv;
  return saturate((boundary - dist) * bandInv);
}

#endif
