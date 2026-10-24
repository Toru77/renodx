#pragma once

// RT sun cascades (R2): C++ mirror of include/rt_sun_range.hlsli (RtSunBoundary, RtSunWeight) and the effective
// count rule the draw hooks read (rt_sun_cascades, set once per present in ApplyBvhInputs). Pure functions;
// test_rtsun.cpp checks them.

#include <algorithm>
#include <cstdint>

#include "../world_state.hpp"

namespace falcom_world::rtsun {

inline float RtSunBoundary(uint32_t rt, int count, const float (&splits)[3], float fade_near, float fade_range_inv) {
  if (rt == 0u || (count != 3 && count != 4) || rt > static_cast<uint32_t>(count)) return 0.f;
  if (rt < static_cast<uint32_t>(count)) return splits[rt - 1u];
  return fade_near + 1.f / std::max(fade_range_inv, 1e-6f);
}

inline float RtSunWeight(float dist, uint32_t rt, int count, const float (&splits)[3], float fade_near, float fade_range_inv) {
  const float boundary = RtSunBoundary(rt, count, splits, fade_near, fade_range_inv);
  if (boundary == 0.f) return 0.f;
  const float band_inv = rt < static_cast<uint32_t>(count) ? 0.2f : fade_range_inv;
  return std::clamp((boundary - dist) * band_inv, 0.f, 1.f);
}

// Cascades RT may own: the requested count, capped by the camera's cascade count (3 or 4 only), and stepped down
// while the outer boundary lies beyond built_range (metres from the camera; 0 or less = unlimited).
inline int RtSunEffectiveCascades(int requested, const CameraSnapshot& camera, float built_range) {
  if (!camera.valid || !camera.has_sun) return 0;
  const int count = camera.shadow_cascade_count;
  if (count != 3 && count != 4) return 0;
  int n = std::clamp(requested, 0, count);
  while (n > 0 && built_range > 0.f
         && RtSunBoundary(static_cast<uint32_t>(n), count, camera.shadow_split_distance, camera.shadow_fade_near,
                          camera.shadow_fade_range_inv) > built_range) {
    --n;
  }
  return n;
}

}  // namespace falcom_world::rtsun
