#define PT_COMMON_PASS 0
#define SHARC_UPDATE 0
// This software contains source code provided by NVIDIA Corporation.
#include "world_pt_common.hlsli"
#include "world_pt_bridge.hlsli"
#include "../rtxdi/rtxdi_di_layout.h"
#include "../rtxdi/rtxdi_debug_layout.h"
#include <Rtxdi/Utils/RandomSamplerPerPassSeeds.hlsli>

#include <Rtxdi/PT/Decorrelation.hlsli>

float3 PtInvalidPixelGuard(float3 value) {
  if (any(isnan(value)) || any(isinf(value))) return 0.0;
  return max(value, 0.0);
}

// d = 2 NEE samples carry their light data in the INF radiance marker; the reflected direction is resolved from it.
SplitBrdf PtCalculateDirectLighting(RAB_Surface surface, RTXDI_PTReservoir reservoir, out float3 light_radiance,
                                    out bool sun_sample, out float3 light_direction) {
  const float3 n = RAB_GetSurfaceNormal(surface);
  const float3 v = RAB_GetSurfaceViewDir(surface);
  float3 l = normalize(reservoir.translatedWorldPosition - RAB_GetSurfaceWorldPos(surface));
  light_radiance = 0.0;
  sun_sample = false;
  if (RTXDI_ConnectsToNeeLight(reservoir) && reservoir.rcVertexLength == 2u) {
    const RTXDI_SampledLightData data = RTXDI_GetSampledLightData(reservoir);
    const uint light_index = data.lightData & RTXDI_DIReservoir_LightIndexMask;
    const RAB_LightInfo info = RAB_LoadLightInfo(light_index, false);
    if (info.index != RTXDI_LIGHT_TABLE_INVALID_INDEX) {
      const RAB_LightSample sample = RAB_SamplePolymorphicLight(info, surface, asfloat(data.uvData));
      l = normalize(sample.position - surface.worldPos);
      light_radiance = sample.radiance;
      sun_sample = sample.sun;
    }
  } else if (reservoir.rcVertexLength > 2u) {
    RTXDI_RandomSamplerState replay_rng = RTXDI_GetRngForShading(reservoir);
    RTXDI_BrdfRaySampleProperties props = RTXDI_DefaultBrdfRaySampleProperties();
    RAB_SurfaceImportanceSampleBsdf(surface, replay_rng, l, props);
  }
  light_direction = l;
  return EvaluateBrdfPT(surface, n, v, l);
}

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID) {
  if (any(id.xy >= g_size)) return;
#if !defined(PT_WATER_SURFACE)
  if (g_sparse_enabled != 0u && g_sparse_levels < 256u && !PtSparseSelected(id.xy)) {
    g_pt_diffuse[id.xy] = float4(0, 0, 0, 1);
    const bool character = (RAB_GetGBufferSurface(id.xy, false).flags & 2u) != 0u;
    g_pt_specular[id.xy] = (g_refl_flags & 32u) != 0u && g_refl_samples != 0u && !character
        ? float4(0, 0, 0, 1e-3) : float4(0, 0, 0, 0);
    return;
  }
#endif
  const RAB_Surface surface = RAB_GetGBufferSurface(id.xy, false);
  if (!RAB_IsSurfaceValid(surface)) {
#if defined(PT_WATER_SURFACE)
    g_pt_specular[id.xy] = float4(0, 0, 0, 0);
#else
    g_pt_diffuse[id.xy] = 0;
    g_pt_specular[id.xy] = float4(0, 0, 0, 0);
#endif
    return;
  }
  const uint2 reservoir_position = id.xy;
  RTXDI_PTReservoir reservoir = RTXDI_LoadPTReservoir(g_restir.restir.reservoirBuffer, reservoir_position,
                                                       g_restir.restir.bufferIndices.finalShadingInputBufferIndex);
  RTXDI_PTApplyDecorrelation(id.xy, reservoir_position, reservoir, g_restir.restir.decorrelation,
                             g_restir.restir.bufferIndices, g_restir.restir.reservoirBuffer, g_frame);

  float3 light_radiance = 0.0;
  bool sun_sample = false;
  float3 light_direction = 0.0;
  const SplitBrdf brdf = PtCalculateDirectLighting(surface, reservoir, light_radiance, sun_sample, light_direction);
  const float3 ucw = reservoir.weightSum * reservoir.targetFunction;
  const float3 total = brdf.demodulatedDiffuse * surface.material.diffuseAlbedo + brdf.specular;
  float3 diffuse = brdf.demodulatedDiffuse * ucw / max(total, 1e-8);
  float3 specular = brdf.specular * ucw / max(total, 1e-8);
  const float3 raw_diffuse = diffuse;  // demodulated, before intensity: Raw Diffuse debug source

  const bool character = (surface.flags & 2u) != 0u;
  const bool direct = RTXDI_ConnectsToNeeLight(reservoir) && reservoir.rcVertexLength == 2u;
  uint debug_status_word = RTXDI_N1_STATUS_INVALID;
  uint4 sun_detail_word = uint4(0u, 0u, 0u, 0u);
  uint debug_sun_word = RTXDI_SUN_STATUS_UNKNOWN;
  if (direct) {
    debug_status_word = RTXDI_N1_STATUS_VISIBLE;
    if (sun_sample) {
      const float3 L = normalize(light_direction);
      const float3 C = g_pt_sun_dir;
      const float3 up = abs(C.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
      const float3 T = normalize(cross(up, C));
      const float3 B = cross(C, T);
      const float bias = character ? g_restir.sun_char_normal_bias : g_restir.sun_normal_bias;
      const float bias_sign = dot(surface.normal, L) >= 0.0 ? 1.0 : -1.0;
      float3 origin = surface.worldPos + surface.normal * (bias * bias_sign);
      if (character) {
        origin += Rt12SelfSofteningOffset(surface.normal * bias_sign, L - C * dot(L, C), g_restir.sun_char_self_softening);
      }
      float2 wind_u;
      if (g_restir.isfast_light_jitter != 0u) {
        wind_u = asfloat(g_pt_noise.Load(int3((id.xy.x + 64u) % 128u, (id.xy.y + 64u) % 128u + 128u * (g_frame % 32u), 0)).xy);
      } else {
        RTXDI_RandomSamplerState wind_rng = RTXDI_InitRandomSampler(id.xy, g_frame, 7u);
        wind_u = float2(RTXDI_GetNextRandom(wind_rng), RTXDI_GetNextRandom(wind_rng));
      }
      float coverage_end = 0.0;
      bool complete_segment = false;
      float transmittance = 1.0;
      float3 transmitted_tint = 1.0;
      Rt12ShadowResult shadow = (Rt12ShadowResult)0;
      shadow.status = RTXDI_SUN_STATUS_UNKNOWN;
      shadow.reason = RTXDI_SUN_REASON_NONE;
      if (g_restir.coverage_valid != 0u
          && !Rt12ShadowCoverageEnd(origin, L, g_restir.sun_max_distance, g_restir.coverage_min, g_restir.coverage_size,
                                    coverage_end)) {
        // Coverage miss: keep the reservoir contribution (T = 1) and report UNKNOWN (world_rtxdi_sun_visibility.cs_6_5.hlsl:188-201).
        shadow.reason = RTXDI_SUN_REASON_COVERAGE_MISS;
      } else {
        if (g_restir.coverage_valid == 0u) coverage_end = g_restir.sun_max_distance;
        else complete_segment = coverage_end >= g_restir.sun_max_distance;
        shadow = Rt12TraceSunShadow(origin, L, coverage_end, complete_segment, character,
            g_restir.sun_casters_only != 0u ? INSTANCE_SHADOW_CASTER : 0u, g_restir.sun_max_distance,
            g_restir.foliage_transmission, g_restir.wind_transmission, g_restir.wind_softening,
            float3(g_restir.foliage_tint.x, g_restir.foliage_tint.y, g_restir.foliage_tint.z), wind_u, T, B,
            transmittance, transmitted_tint);
      }
      sun_detail_word = uint4(shadow.status, shadow.reason, asuint(shadow.distance), shadow.flags);
      debug_sun_word = shadow.status == RTXDI_SUN_STATUS_VISIBLE ? Rt12SunVisibilityWord(shadow.status, shadow.flags)
                                                                 : RTXDI_SUN_STATUS_UNKNOWN;
      if (transmittance < 1.0) PtCount(32u);
      const float darkening = character ? g_restir.sun_char_darkening : g_restir.sun_darkening;
      diffuse *= lerp(1.0, transmittance, darkening);
      specular *= lerp(1.0, transmittance, darkening);
      debug_status_word = shadow.status == RTXDI_SUN_STATUS_VISIBLE ? RTXDI_N1_STATUS_VISIBLE : RTXDI_N1_STATUS_BLOCKED;
    }
  } else {
    debug_status_word = RTXDI_N1_STATUS_EMPTY;
  }
  if (character) debug_status_word |= RTXDI_N1_STATUS_CHARACTER_BIT;
  if (direct) debug_status_word |= RTXDI_N1_STATUS_KNOWN_BIT;

  // Indicator split of the same estimator: intensity scales the indirect part only, and only while direct is on.
  if (reservoir.pathLength > 2u && g_restir.direct_enabled != 0u) {
    const float scale = character ? g_restir.pt_char_intensity : g_restir.pt_intensity;
    diffuse *= scale;
    specular *= scale;
  }
  const float3 raw_specular = PtInvalidPixelGuard(specular);
  diffuse = PtInvalidPixelGuard(diffuse);
  specular = PtInvalidPixelGuard(DemodulateSpecular(surface.material.specularF0, specular));

#if defined(PT_WATER_SURFACE)
  if (g_sharc_enabled != 0u && reservoir.rcVertexLength == reservoir.pathLength && !isinf(reservoir.radiance.x)) PtCount(26u);
  float3 rgb = g_refl_sample_clamp > 0.0 ? min(specular, g_refl_sample_clamp) : specular;
  rgb = rgb * g_refl_intensity;
  if ((g_refl_flags & 1u) != 0u) rgb = min(rgb, 1.0);
  const float hit_t = max(length(reservoir.translatedWorldPosition - surface.worldPos), 1e-3);
  g_pt_specular[id.xy] = float4(PtInvalidPixelGuard(rgb), isfinite(hit_t) ? hit_t : 1e-3);
#else
  diffuse = PtClampLuminance(diffuse, g_gi_clamp);
  const float sparse_scale = (g_refl_flags & 32u) != 0u && g_sparse_enabled != 0u && g_sparse_levels < 256u
      ? 256.0 / float(g_sparse_levels) : 1.0;
  g_pt_diffuse[id.xy] = float4(PtInvalidPixelGuard(diffuse * sparse_scale), 1.0);
  if (g_sharc_enabled != 0u && reservoir.rcVertexLength == reservoir.pathLength && !isinf(reservoir.radiance.x)) PtCount(26u);
  if (g_refl_samples == 0u) {
    g_pt_specular[id.xy] = float4(0, 0, 0, 0);
  } else if (character) {
    // Characters consume total specular radiance directly, without demodulation (A2.4 step 10). With direct off the
    // previous zero output is kept byte-for-byte.
    g_pt_specular[id.xy] = g_restir.direct_enabled != 0u ? float4(raw_specular, 1.0) : float4(0, 0, 0, 0);
  } else if (direct && reservoir.pathLength == 2u) {
    g_pt_specular[id.xy] = float4(specular, 1.0);
  } else {
    float3 rgb = g_refl_sample_clamp > 0.0 ? min(specular, g_refl_sample_clamp) : specular;
    rgb = rgb * g_refl_intensity * sparse_scale;
    if ((g_refl_flags & 1u) != 0u) rgb = min(rgb, 1.0);
    const float hit_t = length(reservoir.translatedWorldPosition - surface.worldPos);
    g_pt_specular[id.xy] = float4(PtInvalidPixelGuard(rgb), isfinite(hit_t) ? max(hit_t, 1e-3) : 1e-3);
  }
  if (g_restir.debug_view != RTXDI_DEBUG_VIEW_OFF) {
    const uint view = g_restir.debug_view;
    if (view == RTXDI_DEBUG_VIEW_RAW_DIFFUSE) {
      g_pt_debug_value[id.xy] = float4(raw_diffuse, 1.0);
    } else if (view == RTXDI_DEBUG_VIEW_DISTANCE_VALIDITY || view == RTXDI_DEBUG_VIEW_DISTANCE_MAGNITUDE) {
      float rc_distance = length(reservoir.translatedWorldPosition - surface.worldPos);
      uint distance_flags = 0u;
      if (isfinite(rc_distance) && rc_distance > 0.0) {
        distance_flags = 1u | (character ? 2u : 0u);
        if (rc_distance >= 65504.0) {
          distance_flags |= 4u;
          rc_distance = 65504.0;
        }
      }
      g_pt_debug_value[id.xy] = float4(rc_distance, (float)distance_flags, 0.0, 1.0);
    }
    g_pt_debug_status[id.xy] = debug_status_word;
    if (view == RTXDI_DEBUG_VIEW_SELECTED_LIGHT) {
      const RTXDI_SampledLightData debug_data = RTXDI_GetSampledLightData(reservoir);
      const uint debug_index = debug_data.lightData & RTXDI_DIReservoir_LightIndexMask;
      uint debug_type = 0u;
      if (direct && debug_index < g_lamp_count) {
        debug_type = g_pt_light_table[debug_index].type == LAMP_TYPE_SPOT ? RTXDI_DI_LIGHT_TYPE_SPOT
                                                                          : RTXDI_DI_LIGHT_TYPE_POINT;
      }
      g_pt_sun_detail[id.xy] = uint4(debug_index, debug_type,
                                     direct && debug_type != 0u ? RTXDI_DI_STATUS_VISIBLE : RTXDI_DI_STATUS_EMPTY,
                                     asuint(reservoir.weightSum));
    } else if (view == RTXDI_DEBUG_VIEW_SUN_VISIBILITY && g_restir.sun_detail_view == RTXDI_SUN_DETAIL_STATUS) {
      g_pt_sun_detail[id.xy] = uint4(debug_sun_word, 0u, 0u, 0u);
    } else {
      g_pt_sun_detail[id.xy] = sun_detail_word;
    }
  }
#endif
}
