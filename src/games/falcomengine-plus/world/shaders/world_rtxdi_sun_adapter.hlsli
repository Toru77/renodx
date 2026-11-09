#ifndef FALCOM_WORLD_RTXDI_SUN_ADAPTER_HLSLI_
#define FALCOM_WORLD_RTXDI_SUN_ADAPTER_HLSLI_

#include "../rtxdi/rtxdi_di_layout.h"
#include <Rtxdi/Utils/RandomSamplerState.hlsli>

struct RAB_Surface {
  float3 world_position;
  float3 shading_normal;
  float3 view_direction;
  float linear_view_depth;
  uint flags;
};

struct RAB_LightInfo {
  float3 direction;
  float3 radiance;
  uint valid;
};

struct RAB_LightSample {
  float3 direction;
  float3 radiance;
  float distance;
  uint light_index;
  uint valid;
};

bool RAB_IsSurfaceValid(RAB_Surface surface) { return (surface.flags & RTXDI_SURFACE_FLAG_VALID) != 0u; }

RAB_LightInfo RAB_LoadLightInfo(uint light_index, bool previous) {
  RAB_LightInfo info = (RAB_LightInfo)0;
  if (light_index != 0u || previous || g_scene_resources_valid == 0u) return info;
  const float3 light_radiance = asfloat(g_rtxdi_scene.Load(int3(RTXDI_SURFACE_SUN_RESERVED_ROW - 1u, 0, 0)).xyz);
  const float3 light_direction = -asfloat(g_rtxdi_scene.Load(int3(RTXDI_SURFACE_SUN_RESERVED_ROW, 0, 0)).xyz);
  const float direction_length_squared = dot(light_direction, light_direction);
  const float radiance_luma = dot(max(light_radiance, 0.0), float3(0.2126, 0.7152, 0.0722));
  if (!all(isfinite(light_direction)) || !isfinite(direction_length_squared) || direction_length_squared <= 1e-12
      || !all(isfinite(light_radiance)) || !isfinite(radiance_luma) || radiance_luma <= 0.0) return info;
  info.direction = light_direction * rsqrt(direction_length_squared);
  info.radiance = max(light_radiance, 0.0);
  info.valid = 1u;
  return info;
}

RAB_LightSample RAB_SamplePolymorphicLight(RAB_LightInfo info, RAB_Surface surface, float2 uv) {
  RAB_LightSample sample = (RAB_LightSample)0;
  if (info.valid == 0u || !RAB_IsSurfaceValid(surface) || !all(isfinite(info.direction))
      || !all(isfinite(info.radiance))) return sample;
  sample.direction = info.direction;
  sample.radiance = info.radiance;
  sample.light_index = 0u;
  sample.valid = 1u;
  return sample;
}

float RAB_GetLightSampleTargetPdfForSurface(RAB_LightSample sample, RAB_Surface surface) {
  if (sample.valid == 0u || !RAB_IsSurfaceValid(surface) || !all(isfinite(sample.radiance))) return 0.0;
  const float target = dot(max(sample.radiance, 0.0), float3(0.2126, 0.7152, 0.0722));
  return isfinite(target) && target > 0.0 ? target : 0.0;
}

float RAB_LightSampleSolidAnglePdf(RAB_LightSample sample) { return 0.0; }
bool RAB_IsAnalyticLightSample(RAB_LightSample sample) { return true; }
void RAB_GetLightDirDistance(RAB_Surface surface, RAB_LightSample sample, out float3 direction, out float distance) {
  direction = sample.direction;
  distance = 0.0;
}
float RAB_SurfaceEvaluateBrdfPdf(RAB_Surface surface, float3 direction) { return 0.0; }
bool RAB_SurfaceImportanceSampleBrdf(RAB_Surface surface, inout RTXDI_RandomSamplerState rng, out float3 direction) {
  direction = 0.0;
  return false;
}

RAB_LightSample RAB_EmptyLightSample() { return (RAB_LightSample)0; }
float3 RAB_GetSurfaceWorldPos(RAB_Surface surface) { return surface.world_position; }
bool RAB_TraceRayForLocalLight(float3 origin, float3 direction, float t_min, float t_max, out uint light_index,
                               out float2 uv) {
  light_index = 0xffffffffu;
  uv = 0.0;
  return false;
}
float RAB_EvaluateLocalLightSourcePdf(uint light_index) { return 0.0; }
float2 RAB_GetEnvironmentMapRandXYFromDir(float3 direction) { return 0.0; }
float RAB_EvaluateEnvironmentMapSamplingPdf(float3 direction) { return 0.0; }
bool RAB_GetConservativeVisibility(RAB_Surface surface, RAB_LightSample sample) { return false; }
RAB_LightInfo RAB_LoadCompactLightInfo(uint ris_buffer_ptr) { return (RAB_LightInfo)0; }

StructuredBuffer<uint2> g_rtxdi_sun_dummy_ris : register(t60);
#define RTXDI_RIS_BUFFER g_rtxdi_sun_dummy_ris

#endif
