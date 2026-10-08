#pragma once

// The game camera's near fade of map objects, on the CPU.
//
// Map-object pixel shaders (contract::UsesContractNearFade; formula in
// shader_contract.hpp) draw a pixel at distance d from the camera when
//
//   max(floor, min(1, (d - start) * inv_range)) * map_alpha * opacity >= dither
//
// with start = InstanceParam.param.x, inv_range = param.y, opacity = color.w
// (times the material's opacity_g), floor = cb_scene.disableMapObjNearFade_g
// and map_alpha = cb_scene.mapColor_g.w. The BVH trace counts a surface as
// shown when the near-fade factor reaches kCameraFadeShown (half of the 4x4
// dither pattern). Opacity and map alpha are reported, not applied: opacity_g
// is per material and unknown here, and map alpha dims everything during map
// transitions.
//
// world_bvh_trace.hlsli CameraFadeInterval mirrors CameraFadeInterval below;
// the two must stay identical.

#include <algorithm>
#include <cmath>

namespace falcom_world::bvh {

inline constexpr float kCameraFadeShown = 0.5f;
inline constexpr float kCameraFadeNever = 3.0e38f;  // interval bound meaning "no distance"

// Near-fade factor at `distance` (0 = hidden, 1 = fully drawn).
inline float CameraNearFade(float distance, float start, float inv_range, float floor_value) {
  const float fade = (std::min)(1.f, (distance - start) * inv_range);
  return (std::max)(floor_value, fade);
}

// Distances [*lo, *hi] at which CameraNearFade >= kCameraFadeShown. Returns
// false when no distance qualifies (then *lo = kCameraFadeNever, *hi = -1).
inline bool CameraFadeInterval(float start, float inv_range, float floor_value, float* lo, float* hi) {
  *lo = 0.f;
  *hi = kCameraFadeNever;
  if (floor_value >= kCameraFadeShown) return true;
  if (inv_range > 0.f) {
    *lo = start + kCameraFadeShown / inv_range;
    return true;
  }
  if (inv_range < 0.f) {
    *hi = start + kCameraFadeShown / inv_range;
    if (*hi >= 0.f) return true;
  }
  *lo = kCameraFadeNever;
  *hi = -1.f;
  return false;
}

// Near-fade inputs the trace can use: finite start and 1/range.
inline bool CameraFadeInputsUsable(float start, float inv_range) {
  return std::isfinite(start) && std::isfinite(inv_range);
}

}  // namespace falcom_world::bvh
