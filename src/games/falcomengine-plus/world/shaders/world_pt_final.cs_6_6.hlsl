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

#if !defined(PT_WATER_SURFACE)
// A19: final-pass PT views write every pixel while active, so skipped pixels show no stale data.
void PtWriteFinalDebug(uint2 pixel, float4 value, uint status, uint4 word) {
  const uint view = g_restir.debug_view;
  if (view != PT_DEBUG_VIEW_RAW && view != PT_DEBUG_VIEW_RESTIR && view != PT_DEBUG_VIEW_FINAL_DIFFUSE
      && view != PT_DEBUG_VIEW_FINAL_SPECULAR) return;
  g_pt_debug_value[pixel] = value;
  g_pt_debug_status[pixel] = status;
  g_pt_sun_detail[pixel] = word;
}
#endif

// d = 2 NEE samples carry their light data in the INF radiance marker; the reflected direction is resolved from it.
SplitBrdf PtCalculateDirectLighting(RAB_Surface surface, RTXDI_PTReservoir reservoir, out float3 light_radiance,
                                    out bool sun_sample, out float3 light_direction, out uint direct_light_index) {
  const float3 n = RAB_GetSurfaceNormal(surface);
  const float3 v = RAB_GetSurfaceViewDir(surface);
  float3 l = normalize(reservoir.translatedWorldPosition - RAB_GetSurfaceWorldPos(surface));
  light_radiance = 0.0;
  sun_sample = false;
  direct_light_index = g_lamp_count;
  bool is_delta = false;
  if (RTXDI_ConnectsToNeeLight(reservoir) && reservoir.rcVertexLength == 2u) {
    const RTXDI_SampledLightData data = RTXDI_GetSampledLightData(reservoir);
    const uint light_index = data.lightData & RTXDI_DIReservoir_LightIndexMask;
    const RAB_LightInfo info = RAB_LoadLightInfo(light_index, false);
    if (info.index != RTXDI_LIGHT_TABLE_INVALID_INDEX) {
      const RAB_LightSample sample = RAB_SamplePolymorphicLight(info, surface, asfloat(data.uvData));
      l = normalize(sample.position - surface.worldPos);
      light_radiance = sample.radiance;
      sun_sample = sample.sun;
      direct_light_index = light_index;
    }
  } else if (reservoir.rcVertexLength > 2u) {
    RTXDI_RandomSamplerState replay_rng = RTXDI_GetRngForShading(reservoir);
    RTXDI_BrdfRaySampleProperties props = RTXDI_DefaultBrdfRaySampleProperties();
    RAB_SurfaceImportanceSampleBsdf(surface, replay_rng, l, props);
    is_delta = props.IsDelta();
  } else {
    is_delta = surface.material.roughness < kMinRoughness && dot(l, reflect(-v, n)) > 0.99999;
  }
  light_direction = l;
  return EvaluateBrdfPT(surface, n, v, l, is_delta);
}

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID) {
  if (any(id.xy >= g_size)) return;
#if !defined(PT_WATER_SURFACE)
  if (g_sparse_enabled != 0u && g_sparse_levels < 256u && !PtSparseSelected(id.xy)) {
    g_pt_diffuse[id.xy] = float4(0, 0, 0, 1);
    const bool character = (RAB_GetGBufferSurface(id.xy, false).flags & 2u) != 0u;
    g_pt_specular[id.xy] = (g_refl_flags & 32u) != 0u && g_refl_enabled != 0u && !character
        ? float4(0, 0, 0, 1e-3) : float4(0, 0, 0, 0);
    g_pt_sun_visibility[id.xy] = float4(1.0, 1.0, 1.0, -1.0);  // unknown: keep the game's CSM result
    PtWriteFinalDebug(id.xy, 0.0, 0u, 0u);
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
    g_pt_sun_visibility[id.xy] = float4(1.0, 1.0, 1.0, -1.0);  // unknown: keep the game's CSM result
    PtWriteFinalDebug(id.xy, 0.0, 0u, 0u);
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
  uint direct_light_index = g_lamp_count;
  SplitBrdf brdf =
      PtCalculateDirectLighting(surface, reservoir, light_radiance, sun_sample, light_direction, direct_light_index);
  const bool is_direct = isinf(reservoir.radiance.x) && reservoir.rcVertexLength == 2u;
  // A18.2: reflective pixels continue diffuse-only in the reservoir (flag 16u), so an indirect sample is a diffuse estimate;
  // its specular share is carried by the dedicated reflection rays.
  if ((surface.flags & 16u) != 0u && !is_direct) brdf.specular = 0.0;
  float3 ucw = reservoir.weightSum * reservoir.targetFunction;
#if !defined(PT_WATER_SURFACE)
  if (g_restir.resampling_none != 0u) ucw = g_pt_initial_color[id.xy].rgb;
#endif
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
        shadow = Rt12TraceSunShadow(origin, L, coverage_end, complete_segment, character, true,
            g_restir.sun_casters_only != 0u ? INSTANCE_SHADOW_CASTER : 0u, g_restir.sun_max_distance,
            g_restir.foliage_transmission, g_restir.wind_transmission, g_restir.wind_softening,
            float3(g_restir.foliage_tint.x, g_restir.foliage_tint.y, g_restir.foliage_tint.z), wind_u, T, B,
            transmittance, transmitted_tint);
      }
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

#if !defined(PT_WATER_SURFACE)
  // A14 F2: one sun ray for the game's own sun shading, which every pixel keeps (Direct off, or its toon share under
  // Direct, A24b). rgb carries the foliage tint, a carries RT visibility; a < 0 means unknown and keeps the game's CSM.
  float4 game_sun_visibility = float4(1.0, 1.0, 1.0, -1.0);
  if (g_restir.game_sun_shadows != 0u) {
    const RAB_LightInfo sun_info = RAB_LoadLightInfo(g_lamp_count, false);
    if (sun_info.index != RTXDI_LIGHT_TABLE_INVALID_INDEX) {
      float2 sun_uv;
      if (g_restir.isfast_light_jitter != 0u) {
        sun_uv = asfloat(g_pt_noise.Load(int3(id.xy.x % 128u, id.xy.y % 128u + 128u * (g_frame % 32u), 0)).xy);
      } else {
        RTXDI_RandomSamplerState sun_rng = RTXDI_InitRandomSampler(id.xy, g_frame, 9u);
        sun_uv = float2(RTXDI_GetNextRandom(sun_rng), RTXDI_GetNextRandom(sun_rng));
      }
      const RAB_LightSample game_sun = RAB_SamplePolymorphicLight(sun_info, surface, sun_uv);
      const float3 L = normalize(game_sun.position - surface.worldPos);
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
      bool coverage_ok = true;
      // A27: a character under a roof gets the game's uniform key light. The overhead ray is environment-only, so neither
      // the character's own proxy nor the CSM (unreliable past its last cascade) decides indoor shading.
      bool covered = false;
      if (g_restir.char_interior_sun_gate != 0u && character) {
        const float3 up_dir = float3(0.0, 1.0, 0.0);
        float up_end = g_restir.sun_max_distance;
        const bool up_ok = g_restir.coverage_valid == 0u
            || Rt12ShadowCoverageEnd(origin, up_dir, g_restir.sun_max_distance, g_restir.coverage_min, g_restir.coverage_size, up_end);
        if (up_ok) {
          float up_transmittance = 1.0;
          float3 up_tint = 1.0;
          covered = Rt12TraceSunShadow(origin, up_dir, up_end, up_end >= g_restir.sun_max_distance, false, false, 0u,
              g_restir.sun_max_distance, 1.0, 0.0, 0.0, float3(1.0, 1.0, 1.0), wind_u, T, B, up_transmittance, up_tint).status
              == RTXDI_SUN_STATUS_BLOCKED;
        }
      }
      if (g_restir.coverage_valid != 0u) {
        coverage_ok = Rt12ShadowCoverageEnd(origin, L, g_restir.sun_max_distance, g_restir.coverage_min,
                                            g_restir.coverage_size, coverage_end);
      } else {
        coverage_end = g_restir.sun_max_distance;
      }
      if (covered) {
        game_sun_visibility = float4(1.0, 1.0, 1.0, 1.0);
      } else if (coverage_ok) {
        complete_segment = coverage_end >= g_restir.sun_max_distance;
        // A26b: characters first trace against the environment only, so an indoor self-hit cannot darken the face.
        Rt12ShadowResult shadow = Rt12TraceSunShadow(origin, L, coverage_end, complete_segment, character, !character,
            g_restir.sun_casters_only != 0u ? INSTANCE_SHADOW_CASTER : 0u, g_restir.sun_max_distance,
            g_restir.foliage_transmission, g_restir.wind_transmission, g_restir.wind_softening,
            float3(g_restir.foliage_tint.x, g_restir.foliage_tint.y, g_restir.foliage_tint.z), wind_u, T, B,
            transmittance, transmitted_tint);
        const bool environment_visible = shadow.status == RTXDI_SUN_STATUS_VISIBLE;
        if (character && environment_visible) {
          shadow = Rt12TraceSunShadow(origin, L, coverage_end, complete_segment, character, true,
              g_restir.sun_casters_only != 0u ? INSTANCE_SHADOW_CASTER : 0u, g_restir.sun_max_distance,
              g_restir.foliage_transmission, g_restir.wind_transmission, g_restir.wind_softening,
              float3(g_restir.foliage_tint.x, g_restir.foliage_tint.y, g_restir.foliage_tint.z), wind_u, T, B,
              transmittance, transmitted_tint);
        }
        sun_detail_word = uint4(shadow.status, shadow.reason, asuint(shadow.distance), shadow.flags);
        debug_sun_word = Rt12SunVisibilityWord(shadow.status, shadow.flags);
        if (!character || environment_visible) {
          if (shadow.status == RTXDI_SUN_STATUS_VISIBLE || shadow.status == RTXDI_SUN_STATUS_BLOCKED) {
            const uint layers = (shadow.flags >> RTXDI_SUN_LAYER_SHIFT) & RTXDI_SUN_LAYER_MASK;
            const float darkening = character ? g_restir.sun_char_darkening : g_restir.sun_darkening;
            float visibility = shadow.status == RTXDI_SUN_STATUS_VISIBLE ? transmittance : 0.0;
            if (character && g_restir.char_terminator_fade > 0.0) {
              const float ndl = dot(surface.normal, L);
              visibility = lerp(1.0, visibility, saturate(ndl / g_restir.char_terminator_fade));
            }
            game_sun_visibility = float4(pow(saturate(float3(g_restir.foliage_tint.x, g_restir.foliage_tint.y,
                                                             g_restir.foliage_tint.z)), float(layers)),
                                         lerp(1.0, visibility, darkening));
          }
        } else {
          game_sun_visibility.a = shadow.status == RTXDI_SUN_STATUS_BLOCKED ? -2.0 : -1.0;
        }
      }
    }
  }
  g_pt_sun_visibility[id.xy] = game_sun_visibility;
#endif

  const bool pt_direct = g_restir.direct_enabled != 0u;
  const float lamp_toon = character ? g_restir.char_lamp_toon : g_restir.lamp_toon;
#if !defined(PT_WATER_SURFACE)
  float3 game_lamp = 0.0;
  if (pt_direct && lamp_toon > 0.0) {
    // The game's lamp response over the pixel's tile list with RT visibility (A24a). The estimate is bounded by the
    // unshadowed tile sum, so it is added unclamped. Environment and characters share it.
    RTXDI_RandomSamplerState lamp_rng = RTXDI_InitRandomSampler(id.xy, g_frame, 13u);
    game_lamp = PtGameLamp(surface, lamp_rng);
  }
#endif

  // Indicator split of the same estimator: intensity scales the indirect part only, and only while direct is on.
  // Direct light is only the d = 2 primary-NEE sample (A10); first-bounce SHARC and sky paths are indirect.
  if (!is_direct && pt_direct) {
    const float scale = character ? g_restir.pt_char_intensity : g_restir.pt_intensity;
    diffuse *= scale;
    specular *= scale;
  }
  if (is_direct) {
    diffuse = PtClampLuminance(diffuse, g_restir.direct_clamp);
    specular = PtClampLuminance(specular, g_restir.direct_clamp);
  }
  const float3 raw_specular = PtInvalidPixelGuard(specular);
  diffuse = PtInvalidPixelGuard(diffuse);
  specular = PtInvalidPixelGuard(DemodulateSpecular(surface.material.specularF0, specular));

#if defined(PT_WATER_SURFACE)
  if (RTXDI_IsValidPTReservoir(reservoir) && g_sharc_enabled != 0u && reservoir.rcVertexLength == reservoir.pathLength
      && !isinf(reservoir.radiance.x)) PtCount(26u);
  // Water is a pure mirror (F0 1): the output is the incoming radiance, without demodulation (A18).
  float3 rgb = g_refl_sample_clamp > 0.0 ? min(raw_specular, g_refl_sample_clamp) : raw_specular;
  rgb = rgb * g_refl_intensity;
  if ((g_refl_flags & 1u) != 0u) rgb = min(rgb, 1.0);
  const float hit_t = max(length(reservoir.translatedWorldPosition - surface.worldPos), 1e-3);
  g_pt_specular[id.xy] = float4(PtInvalidPixelGuard(rgb), isfinite(hit_t) ? hit_t : 1e-3);
#else
  if (!is_direct && g_restir.gi_clamp_per_bounce == 0u) diffuse = PtClampLuminance(diffuse, g_gi_clamp);
  // Environment game lamps are demodulated like the game's lamp sum, which the lighting shader multiplies by albedo.
  if (!character) diffuse += lamp_toon * game_lamp;
  const float sparse_scale = (g_refl_flags & 32u) != 0u && g_sparse_enabled != 0u && g_sparse_levels < 256u
      ? 256.0 / float(g_sparse_levels) : 1.0;
  g_pt_diffuse[id.xy] = float4(PtInvalidPixelGuard(diffuse * sparse_scale), 1.0);
  if (RTXDI_IsValidPTReservoir(reservoir) && g_sharc_enabled != 0u && reservoir.rcVertexLength == reservoir.pathLength
      && !isinf(reservoir.radiance.x)) PtCount(26u);
  if (character) {
    // Characters consume total specular radiance directly, without demodulation (A2.4 step 10). Indirect samples use
    // the reflection clamp; direct samples were clamped before demodulation. The game lamp estimator adds the character
    // lamps whatever the Reflections setting, which only gates the specular (A22.2).
    const float3 clamped = (is_direct || g_refl_sample_clamp <= 0.0)
        ? raw_specular : min(raw_specular, g_refl_sample_clamp);
    const float3 char_spec = g_refl_enabled != 0u ? clamped : 0.0;
    g_pt_specular[id.xy] = pt_direct
        ? float4(char_spec + surface.material.diffuseAlbedo * game_lamp * lamp_toon, 1.0)
        : float4(0, 0, 0, 0);
  } else if (g_refl_enabled == 0u) {
    g_pt_specular[id.xy] = float4(0, 0, 0, 0);
  } else if ((surface.flags & 16u) != 0u) {
    // A18 design A: reflective primaries trace their reflection here (base 22b0badd world_pt_evaluate.hlsli:47-104); the
    // reservoir carries their diffuse continuation and direct light only. Output is incoming radiance (no F0 division)
    // plus the clamped, demodulated direct highlight, as the game applies its own Fresnel to t48.
    const float alpha = max(square(surface.material.roughness), 1e-4);
    const float3 ve = normalize(WorldToTangent(surface, surface.viewDir));
    RTXDI_RandomSamplerState refl_rng = RTXDI_InitRandomSampler(id.xy, g_frame, 11u);
    WorldTraceCounters refl_counters;
    const WorldCameraView bounce_view = {true, 1.0, 0u, 0u};
    float3 sum = 0.0;
    float distance = 0.0;
    for (uint sample = 0u; sample < g_samples; ++sample) {
      const float2 u = float2(RTXDI_GetNextRandom(refl_rng), RTXDI_GetNextRandom(refl_rng));
      float3 dir = reflect(-surface.viewDir, TangentToWorld(surface, normalize(ImportanceSampleGGX_VNDF(u, sqrt(alpha), ve, 1.0))));
      if (dot(dir, surface.normal) <= 0.0) dir = reflect(-surface.viewDir, surface.normal);
      g_rt12_dynamic_mask = g_dynamic_on != 0u ? RT12_MASK_DYNAMIC : 0u;
      const WorldTraceHit hit = TraceWorldRayDxr(surface.worldPos + surface.normal * g_env_bias, dir, 1e-3, g_ray_max,
                                                 false, bounce_view, refl_counters);
      if (hit.prim == 0xFFFFFFFFu) {
        sum += PtSky(dir) * sparse_scale;
        distance += g_ray_max;
        continue;
      }
      const RAB_Surface hit_surface = PtHitSurface(hit, dir);
      float3 lit = 0.0;
      if (!PtSharcTerminal(hit_surface.worldPos, hit_surface.normal, dir, hit.t, alpha, true,
                           hit_surface.material.diffuseAlbedo, lit)) {
        RTXDI_SampledLightData nee_data;
        float nee_pdf, scatter_pdf;
        RAB_LightSample nee_sample;
        PtSampleNee(hit_surface, refl_rng, lit, nee_data, nee_pdf, scatter_pdf, nee_sample, true);
        lit += hit_surface.material.diffuseAlbedo * PtSky(hit_surface.normal);
      }
      lit *= sparse_scale;
      sum += g_refl_sample_clamp > 0.0 ? min(lit, g_refl_sample_clamp) : lit;
      distance += hit.t;
    }
    float3 rgb = PtInvalidPixelGuard(sum / float(max(g_samples, 1u))) * g_refl_intensity;
    if (direct && reservoir.pathLength == 2u) {
      // A19: demodulate by the Fresnel the lobe evaluated (Schlick at v.h), not F0; the game applies its own Fresnel.
      const float3 f = Schlick_Fresnel(surface.material.specularF0,
                                       saturate(dot(surface.viewDir, normalize(light_direction + surface.viewDir))));
      float3 hl = raw_specular / max(f, 1e-4);
      hl = g_refl_sample_clamp > 0.0 ? min(hl, g_refl_sample_clamp) : hl;
      rgb += PtInvalidPixelGuard(hl) * g_refl_intensity;
    }
    if ((g_refl_flags & 1u) != 0u) rgb = min(rgb, 1.0);
    g_pt_specular[id.xy] = float4(PtInvalidPixelGuard(rgb), max(distance / float(max(g_samples, 1u)), 1e-3));
  } else if (direct && reservoir.pathLength == 2u) {
    float3 rgb = specular;
    if ((g_refl_flags & 1u) != 0u) rgb = min(rgb, 1.0);
    g_pt_specular[id.xy] = float4(PtInvalidPixelGuard(rgb), 1.0);
  } else {
    float3 rgb = g_refl_sample_clamp > 0.0 ? min(specular, g_refl_sample_clamp) : specular;
    rgb = rgb * g_refl_intensity * sparse_scale;
    if ((g_refl_flags & 1u) != 0u) rgb = min(rgb, 1.0);
    const float hit_t = length(reservoir.translatedWorldPosition - surface.worldPos);
    g_pt_specular[id.xy] = float4(PtInvalidPixelGuard(rgb), isfinite(hit_t) ? max(hit_t, 1e-3) : 1e-3);
  }
  const uint view = g_restir.debug_view;
  if (view == PT_DEBUG_VIEW_RAW || view == PT_DEBUG_VIEW_RESTIR || view == PT_DEBUG_VIEW_FINAL_DIFFUSE
      || view == PT_DEBUG_VIEW_FINAL_SPECULAR) {
    // A18 final-pass PT views. PT SHARC and PT Material belong to the initial pass, so u14-u16 stay untouched for them.
    const uint path_word = reservoir.pathLength | (reservoir.rcVertexLength << 8u)
                           | ((RTXDI_ConnectsToNeeLight(reservoir) ? 1u : 0u) << 16u);
    float4 debug_value = float4(reservoir.weightSum * reservoir.targetFunction, 1.0);
    uint debug_status = path_word;
    uint4 debug_word = 0u;
    if (view == PT_DEBUG_VIEW_RESTIR) {
      float channel = reservoir.weightSum;
      switch (g_restir.debug_channel) {
        case PT_DEBUG_CHANNEL_M: channel = reservoir.M; break;
        case PT_DEBUG_CHANNEL_AGE: channel = float(reservoir.age); break;
        case PT_DEBUG_CHANNEL_PATH_LENGTH: channel = float(reservoir.pathLength); break;
        case PT_DEBUG_CHANNEL_RC_LENGTH: channel = float(reservoir.rcVertexLength); break;
        case PT_DEBUG_CHANNEL_INITIAL_VS_REUSED: channel = reservoir.age == 0u ? 0.0 : 1.0; break;
        default: break;
      }
      debug_value = float4(channel, float(g_restir.debug_channel), 0.0, 1.0);
      debug_word = uint4(reservoir.randomSeed, reservoir.randomIndex, reservoir.pathLength, reservoir.rcVertexLength);
    } else if (view == PT_DEBUG_VIEW_FINAL_DIFFUSE) {
      // A21: what the lighting shader adds, the demodulated diffuse times this pixel's own albedo.
      debug_value = float4(g_pt_diffuse[id.xy].rgb * surface.material.diffuseAlbedo, 1.0);
      debug_status = 0u;
    } else if (view == PT_DEBUG_VIEW_FINAL_SPECULAR) {
      debug_value = g_pt_specular[id.xy];
      debug_status = 0u;
    }
    PtWriteFinalDebug(id.xy, debug_value, debug_status, debug_word);
  } else if (view != RTXDI_DEBUG_VIEW_OFF && view <= RTXDI_DEBUG_VIEW_SUN_VISIBILITY) {
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
