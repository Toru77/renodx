#define RT12_TLAS_REGISTER t29
#define RT12_DYNAMIC_VERTICES_REGISTER t30

#include "world_rt12_trace.hlsli"
#include "world_rt_lamps.hlsli"
#include "world_rtxdi_surface.hlsli"
#include "../rtxdi/rtxdi_di_layout.h"
#include <Rtxdi/DI/ReSTIRDIParameters.h>
#include <Rtxdi/Utils/RandomSamplerState.hlsli>
#include "../../reference/brdf.hlsli"

StructuredBuffer<LightParam> g_rtxdi_lights : register(t17);
StructuredBuffer<uint> g_rtxdi_light_types : register(t18);
Texture2D<float4> g_rtxdi_color : register(t19);
Texture2D<uint2> g_rtxdi_mrt2 : register(t20);
StructuredBuffer<GameDeferredParam> g_rtxdi_deferred : register(t21);
Texture2D<float> g_rtxdi_depth : register(t31);
Texture2D<uint4> g_rtxdi_normal : register(t32);
Texture2D<uint4> g_rtxdi_scene : register(t33);
Texture2D<float4> g_rtxdi_motion : register(t34);
Texture2D<float> g_rtxdi_prev_depth : register(t35);
Texture2D<uint4> g_rtxdi_prev_normal : register(t36);
Texture2D<uint4> g_rtxdi_prev_scene : register(t37);
Texture2D<float4> g_rtxdi_prev_color : register(t38);
Texture2D<uint4> g_rtxdi_prev_mrt2 : register(t39);
StructuredBuffer<GameDeferredParam> g_rtxdi_prev_deferred : register(t40);
StructuredBuffer<LightParam> g_rtxdi_prev_lights : register(t41);
StructuredBuffer<uint> g_rtxdi_prev_light_types : register(t42);
StructuredBuffer<LightIndexData> g_rtxdi_tiles : register(t43);
StructuredBuffer<LightIndexData> g_rtxdi_prev_tiles : register(t44);
RWStructuredBuffer<RtxdiDiRawResult> g_rtxdi_raw : register(u1);

cbuffer RtxdiDiParams : register(b0) {
  uint2 g_dimensions;
  uint g_scene_resources_valid;
  uint g_coverage_valid;
  uint g_light_count;
  float g_environment_bias;
  float g_character_bias;
  float g_endpoint_clearance;
  float3 g_region_min;
  float g_region_size;
  uint g_deferred_count;
  uint g_prev_deferred_count;
  uint g_prev_light_count;
  uint g_tile_count;
  uint g_prev_tile_count;
  float g_local_light_size;
  uint g_noise_mode;
  uint g_noise_slice;
  float g_character_self_softening;
};

cbuffer RtxdiN1FramePrefix : register(b1) {
  uint2 g_frame_dimensions;
  uint g_history_valid;
  uint g_frame_flags;
};

#include "world_rtxdi_diag.hlsli"
#include "world_rtxdi_light_bridge.hlsli"

Texture2D<uint4> g_rtxdi_mrt1 : register(t45);
RWTexture2D<float4> g_rtxdi_n1_diffuse : register(u3);
RWTexture2D<float4> g_rtxdi_n1_specular : register(u4);
RWTexture2D<uint> g_rtxdi_n1_status : register(u5);

cbuffer RtxdiN1ExtraParams : register(b12) {
  uint g_n1_config_version;
  float g_n1_hammon_enabled;
  float g_n1_diffuse_strength;
  float g_n1_ggx_enabled;
  float g_n1_specular_strength;
  float g_n1_specular_peak;
  float g_n1_roughness_min;
  float g_n1_roughness_max;
  float g_n1_env_keep;
  float g_n1_char_keep;
};

uint RtxdiN1Status(uint status, bool known, bool character) {
  return status | (known ? RTXDI_N1_STATUS_KNOWN_BIT : 0u)
         | (character ? RTXDI_N1_STATUS_CHARACTER_BIT : 0u);
}

bool RtxdiN1FiniteHalf4(float4 value) {
  return all(isfinite(value)) && all(value >= 0.0) && all(value <= 65504.0);
}

[numthreads(8, 8, 1)]
void main(uint3 dispatch_id : SV_DispatchThreadID) {
  if (any(dispatch_id.xy >= g_dimensions)) return;
  const uint2 pixel = dispatch_id.xy;
  const uint pixel_index = pixel.y * g_dimensions.x + pixel.x;
  const bool config_valid = g_n1_config_version == RTXDI_N1_CONFIG_VERSION
                            && all(isfinite(float4(g_n1_hammon_enabled, g_n1_diffuse_strength,
                                                  g_n1_ggx_enabled, g_n1_specular_strength)))
                            && all(isfinite(float2(g_n1_specular_peak, g_n1_roughness_min)))
                            && isfinite(g_n1_roughness_max)
                            && g_n1_hammon_enabled >= 0.0 && g_n1_hammon_enabled <= 1.0
                            && g_n1_ggx_enabled >= 0.0 && g_n1_ggx_enabled <= 1.0
                            && g_n1_diffuse_strength >= 0.0 && g_n1_diffuse_strength <= 2.0
                            && g_n1_specular_strength >= 0.0 && g_n1_specular_strength <= 2.0
                            && g_n1_specular_peak >= 0.0 && g_n1_specular_peak <= 16.0
                            && g_n1_roughness_min >= 0.0 && g_n1_roughness_min <= 0.8
                            && g_n1_roughness_max >= 0.2 && g_n1_roughness_max <= 1.0
                            && g_n1_roughness_max >= g_n1_roughness_min
                            && all(isfinite(float2(g_n1_env_keep, g_n1_char_keep)))
                            && g_n1_env_keep >= 0.0 && g_n1_env_keep <= 1.0
                            && g_n1_char_keep >= 0.0 && g_n1_char_keep <= 1.0;
  RtxdiDiagAdd(RTXDI_DIAG_N1_PIXELS, 1u);
  RtxdiDiagAdd(RTXDI_DIAG_N1_CONFIG_INVALID, config_valid ? 0u : 1u);
  float4 diffuse = 0.0;
  float4 specular = 0.0;
  uint status = RTXDI_N1_STATUS_INVALID;
  RAB_Surface surface = RAB_GetGBufferSurface(int2(pixel), false);
  const bool valid_surface = RAB_IsSurfaceValid(surface);
  const bool character = valid_surface && ((surface.flags & RTXDI_SURFACE_FLAG_CHARACTER) != 0u);

  if (valid_surface && config_valid) {
    const RtxdiDiRawResult raw = g_rtxdi_raw[pixel_index];
    if (raw.status == RTXDI_DI_STATUS_EMPTY) {
      status = RTXDI_N1_STATUS_EMPTY;
    } else if (raw.status == RTXDI_DI_STATUS_UNKNOWN) {
      status = RTXDI_N1_STATUS_UNKNOWN;
    } else if ((raw.status == RTXDI_DI_STATUS_VISIBLE || raw.status == RTXDI_DI_STATUS_BLOCKED)
               && isfinite(raw.inverse_pdf) && raw.inverse_pdf > 0.0
               && raw.selected_light_index != 0xffffffffu && g_scene_resources_valid != 0u) {
      const RAB_LightInfo light = RAB_LoadLightInfo(raw.selected_light_index, false);
      if (light.valid != 0u && light.type == raw.selected_light_type && surface.tile_valid != 0u
          && RtxdiTileContainsLight(surface.tile_cell, raw.selected_light_index, raw.selected_light_type, false)) {
        const bool blocked = raw.status == RTXDI_DI_STATUS_BLOCKED;
        const float blocked_keep = character ? g_n1_char_keep : g_n1_env_keep;
        if (blocked && blocked_keep <= 0.0) {
          status = RTXDI_N1_STATUS_BLOCKED;
        } else {
          if (blocked) status = RTXDI_N1_STATUS_BLOCKED;
          const RAB_LightSample sample = RAB_SamplePolymorphicLight(light, surface, float2(0.5, 0.5));
          const float3 to_light = sample.position - surface.world_position;
          const float attenuation = LampDistanceAttenuation(light.lamp, to_light);
          const float spot = light.type == RTGI_LAMP_TYPE_SPOT ? LampSpotCone(light.lamp, sample.direction) : 1.0;
          const float lamp_attenuation = attenuation * spot;
          const float3 lamp_color = max(character ? light.lamp.charaColor : light.lamp.color, 0.0);
          const float ndotl_raw = dot(surface.shading_normal, sample.direction);
          const float ndotl = saturate(ndotl_raw);
          const float lit = max(max(0.0, ndotl_raw), sample.translucency);
          const float3 half_sum = surface.view_direction + sample.direction;
          const float half_length_sq = dot(half_sum, half_sum);
          const float3 half_vector = (half_length_sq > 1e-12) ? half_sum * rsqrt(half_length_sq) : 0.0;
          const float ndoth = saturate(dot(surface.shading_normal, half_vector));
          const float vdoth = saturate(dot(surface.view_direction, half_vector));
          float3 diffuse_rgb = lamp_color * (lamp_attenuation * lit);
          if (g_n1_hammon_enabled > 0.5 && !character) {
            const GameDeferredParam diffuse_deferred = g_rtxdi_deferred[g_rtxdi_mrt2.Load(int3(pixel, 0)).x];
            const float roughness = clamp(diffuse_deferred.tail[0].z, g_n1_roughness_min, g_n1_roughness_max);
            diffuse_rgb *= lerp(float3(1.0, 1.0, 1.0),
                                HammonDiffuseCorrection(saturate(ndotl_raw), saturate(dot(surface.shading_normal, surface.view_direction)),
                                                        ndoth, vdoth, roughness),
                                g_n1_diffuse_strength);
          }
          float3 specular_rgb = 0.0;
          bool specular_evaluated = false;
          if (!character && surface.has_local_specular != 0u) {
            specular_evaluated = true;
            const uint material_index = g_rtxdi_mrt2.Load(int3(pixel, 0)).x;
            const GameDeferredParam deferred = g_rtxdi_deferred[material_index];
            if (!all(isfinite(float4(deferred.head[1].xyz, deferred.tail[0].z)))) {
              g_rtxdi_n1_diffuse[pixel] = diffuse;
              g_rtxdi_n1_specular[pixel] = specular;
              const uint early_status = blocked ? RTXDI_N1_STATUS_BLOCKED : RTXDI_N1_STATUS_INVALID;
              g_rtxdi_n1_status[pixel] = RtxdiN1Status(early_status, blocked, character);
              RtxdiDiagAdd(RTXDI_DIAG_N1_STATUS_BASE + early_status, 1u);
              return;
            }
            const float gloss = max(0.001, sample.specular_glossiness * surface.material_glossiness);
            const float blinn = pow(ndoth, gloss);
            const float mrt1_spec_scale = (g_rtxdi_mrt1.Load(int3(pixel, 0)).z & 255u) * (1.0 / 255.0);
            const float3 blinn_specular = lamp_color * (sample.specular_intensity * (lamp_attenuation * lit * blinn)
                                                        * mrt1_spec_scale);
            float3 ggx_specular = blinn_specular;
            if (g_n1_ggx_enabled > 0.5) {
              const float brdf_nv = saturate(dot(surface.shading_normal, surface.view_direction));
              const float roughness = clamp(deferred.tail[0].z, g_n1_roughness_min, g_n1_roughness_max);
              const float roughness_light = clamp(min(roughness, pow(2.0 / (gloss + 2.0), 0.25)), 0.08, 1.0);
              float3 ggx = GGX_Specular(ndoth, brdf_nv, ndotl, vdoth, roughness_light, deferred.head[1].xyz);
              ggx *= MultiScatterCompensation(brdf_nv, ndotl, roughness_light, deferred.head[1].xyz);
              const float3 clamped_ggx = SoftClampSpecular(ggx * (lamp_attenuation * ndotl), g_n1_specular_peak);
              ggx_specular = lamp_color * (sample.specular_intensity * clamped_ggx * mrt1_spec_scale);
            }
            specular_rgb = lerp(blinn_specular, ggx_specular, g_n1_specular_strength);
          }
          const float lobe_scale = raw.inverse_pdf * (blocked ? blocked_keep : 1.0);
          diffuse_rgb *= lobe_scale;
          specular_rgb *= lobe_scale;
          const bool positive_sample = sample.valid != 0u && isfinite(sample.distance) && sample.distance > 0.0
                                       && all(isfinite(float4(lamp_color, lamp_attenuation)))
                                       && lamp_attenuation > 0.0;
          const float diffuse_hit_t = !blocked && positive_sample && lit > 0.0 && any(lamp_color > 0.0) ? sample.distance : 0.0;
          const float specular_hit_t = !blocked && positive_sample && specular_evaluated && lit > 0.0 && any(lamp_color > 0.0)
                                           ? sample.distance
                                           : 0.0;
          const float4 diffuse_candidate = float4(diffuse_rgb, diffuse_hit_t);
          const float4 specular_candidate = float4(specular_rgb, specular_hit_t);
          if (sample.valid != 0u && isfinite(diffuse_hit_t) && isfinite(specular_hit_t)
              && (blocked || (!positive_sample) || (diffuse_hit_t > 0.0 && diffuse_hit_t <= 65504.0))
              && (blocked || (!specular_evaluated || !positive_sample) || (specular_hit_t > 0.0 && specular_hit_t <= 65504.0))
              && RtxdiN1FiniteHalf4(diffuse_candidate) && RtxdiN1FiniteHalf4(specular_candidate)) {
            diffuse = diffuse_candidate;
            specular = specular_candidate;
            status = blocked ? RTXDI_N1_STATUS_BLOCKED : RTXDI_N1_STATUS_VISIBLE;
          }
        }
      } else {
        RtxdiDiagAdd(RTXDI_DIAG_N1_MEMBERSHIP_REJECT, 1u);
      }
    }
  }

  const bool known = status == RTXDI_N1_STATUS_VISIBLE || status == RTXDI_N1_STATUS_BLOCKED;
  g_rtxdi_n1_diffuse[pixel] = diffuse;
  g_rtxdi_n1_specular[pixel] = specular;
  g_rtxdi_n1_status[pixel] = RtxdiN1Status(status, known, character);
  [unroll]
  for (uint n1_status = RTXDI_N1_STATUS_INVALID; n1_status <= RTXDI_N1_STATUS_EMPTY; ++n1_status)
    RtxdiDiagAdd(RTXDI_DIAG_N1_STATUS_BASE + n1_status, status == n1_status);
}
