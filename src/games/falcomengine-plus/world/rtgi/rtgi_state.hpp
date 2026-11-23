#pragma once

#include <algorithm>
#include <cstdint>

#include "../rtao/rtao_state.hpp"

namespace falcom_world::rtgi {

// Effective GI ray length. The camera is at least built_range + margin from every region face (bvh_pool.hpp
// kPoolRegionRayMargin), so a pixel inside the effective Fade End keeps built_range + margin - fade_end_eff to the
// nearest face: rays of at most that length never reach the shader's region guard.
inline float RtgiEffectiveRayMax(float requested, float built_range, float fade_end_eff, float margin) {
  return std::min(requested, built_range + margin - fade_end_eff);
}

}  // namespace falcom_world::rtgi
