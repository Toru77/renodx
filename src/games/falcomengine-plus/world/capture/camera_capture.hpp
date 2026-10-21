#pragma once

// Camera snapshot from the lighting pass constants (cb0 of the lighting pixel
// shader, cb_scene): view, inverse view, projection, view-projection and its
// inverse, plus the previous view-projection and the near-fade scalars when
// the buffer is large enough. The pool region, the GPU trace and the depth
// comparison read g_state.camera.

#include <cstring>
#include <vector>

#include "../contract/shader_contract.hpp"
#include "../world_state.hpp"

namespace falcom_world {

// The lighting pixel shader (0xCA3D8596, decompiled) declares cb_scene with
// these offsets, like every map-object shader (contract::UsesContractNearFade).
inline constexpr size_t kSceneNearFadeFloorFloat = contract::kSceneNearFadeFloorOffset / sizeof(float);
inline constexpr size_t kSceneMapAlphaFloat = contract::kSceneMapColorOffset / sizeof(float) + 3u;
// Float indices of cb_scene registers (register x 4 floats): lightColor_g c25, lightDirection_g c26, sceneShadowColor_g c27,
// shadowmapCascadeCount_g c27.w (int bits; on Kai c27.w is chrLightIntensity_g), shadowSplitDistance_g c57.
inline constexpr size_t kSceneLightColorFloat = 25u * 4u;
inline constexpr size_t kSceneLightDirectionFloat = 26u * 4u;
inline constexpr size_t kSceneShadowColorFloat = 27u * 4u;
inline constexpr size_t kSceneCascadeCountFloat = 27u * 4u + 3u;
inline constexpr size_t kSceneShadowSplitFloat = 57u * 4u;

inline void CaptureCameraFromBytes(const std::vector<uint8_t>& bytes, uint32_t frame, uint8_t source = 1u) {
  const size_t float_count = bytes.size() / sizeof(float);
  if (float_count < 96u) return;

  const auto* data = reinterpret_cast<const float*>(bytes.data());
  auto& camera = g_state.camera;
  std::memcpy(camera.view, data + 0u, sizeof(float) * 16u);
  std::memcpy(camera.view_inv, data + 16u, sizeof(float) * 16u);
  std::memcpy(camera.proj, data + 32u, sizeof(float) * 16u);
  std::memcpy(camera.view_proj, data + 64u, sizeof(float) * 16u);
  std::memcpy(camera.view_proj_inv, data + 80u, sizeof(float) * 16u);
  camera.has_prev = false;
  if (float_count >= 316u) {
    std::memcpy(camera.prev_view_proj, data + 300u, sizeof(float) * 16u);
    camera.has_prev = true;
  }
  camera.has_fade = false;
  if (float_count > kSceneMapAlphaFloat) {
    camera.near_fade_floor = data[kSceneNearFadeFloorFloat];
    camera.map_alpha = data[kSceneMapAlphaFloat];
    camera.has_fade = true;
  }
  camera.has_sun = float_count > kSceneShadowSplitFloat + 2u;
  if (camera.has_sun) {
    std::memcpy(camera.light_color, data + kSceneLightColorFloat, sizeof(float) * 3u);
    std::memcpy(camera.light_direction, data + kSceneLightDirectionFloat, sizeof(float) * 3u);
    std::memcpy(camera.scene_shadow_color, data + kSceneShadowColorFloat, sizeof(float) * 3u);
    std::memcpy(&camera.shadow_cascade_count, data + kSceneCascadeCountFloat, sizeof(int32_t));
    std::memcpy(camera.shadow_split_distance, data + kSceneShadowSplitFloat, sizeof(float) * 3u);
  }
  camera.valid = true;
  camera.frame = frame;
  camera.source = source;
}

}  // namespace falcom_world
