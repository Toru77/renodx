#pragma once

#include <cstddef>
#include <cstdint>

namespace falcom_world::game_lights {

// Sora 2nd lighting 0xCA3D8596: t11 dynamicLights_g (LightParam) and t12 lightIndices_g (LightIndexData). Kept for the
// RTXDI and Kai hero-light readers.
inline constexpr uint32_t kLightingLightsRegister = 11u;
inline constexpr uint32_t kLightingLightIndicesRegister = 12u;
// Sora 2nd lighting 0xCA3D8596 (lighting lines 192, 193, 198, 199): t2 mrtTexture1, t3 mrtTexture2,
// t6 deferredParams_g, t7 shadowTexture. Kept for the surviving lighting consumers.
inline constexpr uint32_t kLightingMrt1Register = 2u;
inline constexpr uint32_t kLightingMrt2Register = 3u;
inline constexpr uint32_t kLightingDeferredRegister = 6u;
inline constexpr uint32_t kLightingShadowRegister = 7u;
inline constexpr uint32_t kLightParamBytes = 112u;
inline constexpr uint32_t kLightIndexDataBytes = 576u;

// Mirrors LightParam in lighting_0xCA3D8596.ps_5_0.hlsl (offsets 0 to 108).
struct LightParamCpu {
  float pos[3];
  float radius;
  float color[3];
  float radius_inv;
  float chara_color[3];
  float attenuation;
  float vec[3];
  float spot_angle_inv;
  float attenuation_angle;
  float specular_intensity;
  float specular_glossiness;
  float scatter_anisotropy;
  float scatter_color[3];
  float scatter_density;
  float translucency;
  int32_t shadowmap_index;
  float scatter_offset;
  float user_param;
};
static_assert(sizeof(LightParamCpu) == kLightParamBytes, "LightParam must be 112 bytes");
static_assert(offsetof(LightParamCpu, spot_angle_inv) == 60, "LightParam spotAngleInv must be at offset 60");
static_assert(offsetof(LightParamCpu, attenuation_angle) == 64, "LightParam attenuationAngle must be at offset 64");

// Mirrors LightIndexData in lighting_0xCA3D8596.ps_5_0.hlsl (offsets 0 to 572).
struct LightIndexDataCpu {
  int32_t point_indices[63];
  uint32_t point_count;
  int32_t spot_indices[63];
  uint32_t spot_count;
  int32_t probe_indices[14];
  uint32_t probe_count;
  float tile_depth_inv;
};
static_assert(sizeof(LightIndexDataCpu) == kLightIndexDataBytes, "LightIndexData must be 576 bytes");

}  // namespace falcom_world::game_lights
