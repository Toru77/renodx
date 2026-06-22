// ---- Created with 3Dmigoto v1.4.1 on Sun Mar 22 00:32:12 2026

cbuffer cb_scene : register(b0)
{
  float4x4 view_g : packoffset(c0);
  float4x4 viewInv_g : packoffset(c4);
  float4x4 proj_g : packoffset(c8);
  float4x4 projInv_g : packoffset(c12);
  float4x4 viewProj_g : packoffset(c16);
  float4x4 viewProjInv_g : packoffset(c20);
  float2 vpSize_g : packoffset(c24);
  float2 invVPSize_g : packoffset(c24.z);
  float3 lightColor_g : packoffset(c25);
  float disableMapObjNearFade_g : packoffset(c25.w);
  float3 lightDirection_g : packoffset(c26);
  float gameTime_g : packoffset(c26.w);
  float3 sceneShadowColor_g : packoffset(c27);
  int shadowmapCascadeCount_g : packoffset(c27.w);
  float3 windDirection_g : packoffset(c28);
  float sceneTime_g : packoffset(c28.w);
  float2 lightTileSizeInv_g : packoffset(c29);
  float fogNearDistance_g : packoffset(c29.z);
  float fogFadeRangeInv_g : packoffset(c29.w);
  float3 fogColor_g : packoffset(c30);
  float fogIntensity_g : packoffset(c30.w);
  float fogHeight_g : packoffset(c31);
  float fogHeightRangeInv_g : packoffset(c31.y);
  float windWaveTime_g : packoffset(c31.z);
  float windWaveFrequency_g : packoffset(c31.w);
  uint localLightProbeCount_g : packoffset(c32);
  float lightSpecularGlossiness_g : packoffset(c32.y);
  float lightSpecularIntensity_g : packoffset(c32.z);
  float localShadowResolutionInv_g : packoffset(c32.w);
  float4x4 ditherMtx_g : packoffset(c33);
  float4 lightProbe_g[9] : packoffset(c37);
  float3 chrLightDir_g : packoffset(c46);
  float windForce_g : packoffset(c46.w);
  float4 mapColor_g : packoffset(c47);
  float4 clipPlane_g : packoffset(c48);
  float2 resolutionScaling_g : packoffset(c49);
  float2 invShadowSize_g : packoffset(c49.z);
  float3 chrShadowColor_g : packoffset(c50);
  float shadowFadeNear_g : packoffset(c50.w);
  float4 frustumPlanes_g[6] : packoffset(c51);
  float3 shadowSplitDistance_g : packoffset(c57);
  float shadowFadeRangeInv_g : packoffset(c57.w);
  float4x4 shadowMtx_g[4] : packoffset(c58);
  float4x4 prevViewProj_g : packoffset(c74);
  float2 jitterDiff_g : packoffset(c78);
}

SamplerState samPoint_s : register(s0);
SamplerComparisonState SmplShadow_s : register(s13);
Texture2D<float4> depthTexture : register(t0);
Texture2DArray<float4> shadowMaps : register(t16);

#include "../shared.h"

// ── IS-FAST noise texture (bound globally by addon) ──
Texture3D<float2> g_isfastNoiseTexture : register(t3);

// 3Dmigoto declarations
#define cmp -

// ── Interleaved Gradient Noise (IGN) — IS-FAST fallback ──
static float IGN(float2 p) {
  return frac(52.9829189 * frac(0.06711056 * p.x + 0.00583715 * p.y));
}

// ── Spatio-Temporal Blue Noise (IS-FAST) with IGN fallback ──
static float2 SpatioTemporalNoise_ISFAST(uint2 p, uint t) {
  if (shader_injection_data.shadow_isfast_texture_loaded > 0.5f) {
    float3 uvw = float3(
      (float)(p.x % 128u) / 128.0,
      (float)(p.y % 128u) / 128.0,
      (float)((t + (uint)shader_injection_data.shadow_isfast_seed_offset) % 32u) / 32.0);
    uvw.xy *= shader_injection_data.shadow_isfast_spatial_scale;
    uvw.z *= shader_injection_data.shadow_isfast_temporal_speed;
    float2 s = g_isfastNoiseTexture.SampleLevel(samPoint_s, uvw, 0);
    return s;
  } else {
    static const float R2_A1 = 0.7548776662466927;
    static const float R2_A2 = 0.5698402909980532;
    float b1 = IGN(float2(p) * shader_injection_data.shadow_isfast_spatial_scale + shader_injection_data.shadow_isfast_seed_offset);
    float b2 = IGN(float2(p) * shader_injection_data.shadow_isfast_spatial_scale + float2(47, 17) + shader_injection_data.shadow_isfast_seed_offset);
    return float2(frac(b1 + R2_A1 * (float)t * shader_injection_data.shadow_isfast_temporal_speed),
                  frac(b2 + R2_A2 * (float)t * shader_injection_data.shadow_isfast_temporal_speed));
  }
}

// ── PCSS Poisson disk sample offset (√-distributed, golden-angle) ──
static float2 ISFAST_PoissonDisk(float sample_index, float sample_count, float jitter_angle, float radius_scale) {
  float r = sqrt((sample_index + 0.5) / sample_count) * radius_scale;
  float a = sample_index * 2.399963f + jitter_angle;
  float sin_a, cos_a;
  sincos(a, sin_a, cos_a);
  return float2(cos_a, sin_a) * r;
}

#define PCSS_SAMPLE_COUNT 32u
#define PCSS_BLOCKER_COUNT 31u

// ── PCSS: three-step (blocker search → penumbra → variable-radius PCF) ──
float PCSS_Shadow(
    float2 shadow_uv,
    float receiver_z,
    float slice,
    float split_distance,
    float jitter_angle)
{
  // Derive cascade world size from shadow projection matrix
  // UV radius = world_softness / cascadeWorldSize → constant world-space softness
  float2 cascadeWorldSize;
  cascadeWorldSize.x = 2.0 / max(abs(shadowMtx_g[int(slice)]._m00), 0.0001);
  cascadeWorldSize.y = 2.0 / max(abs(shadowMtx_g[int(slice)]._m11), 0.0001);
  float2 base_radius = shader_injection_data.shadow_pcss_search_radius / cascadeWorldSize;
  float2 search_radius = base_radius;

  // —— PCSS Experimental Fixes ——
  // Fix B: Clamp cascade world size (prevents Ultra's large cascades from collapsing filter)
  if (shader_injection_data.shadow_pcss_fix_clamp_cascade > 0.01f) {
    cascadeWorldSize = min(cascadeWorldSize, shader_injection_data.shadow_pcss_fix_clamp_cascade);
    base_radius = shader_injection_data.shadow_pcss_search_radius / cascadeWorldSize;
  }
  // Fix A: Texel-based radius (consistent across all quality levels)
  if (shader_injection_data.shadow_pcss_fix_texel_radius > 0.5f) {
    base_radius = shader_injection_data.shadow_pcss_search_radius * invShadowSize_g * 50.0;
  }
  search_radius = base_radius;

  float blocker_radius_scale = rsqrt((float)PCSS_BLOCKER_COUNT);
  float filter_radius_scale = rsqrt((float)PCSS_SAMPLE_COUNT);

  // Step 1: Blocker search
  float blocker_count = 0;
  float blocker_depth_sum = 0;
  for (uint i = 0u; i < PCSS_BLOCKER_COUNT; i++) {
    float2 offset = ISFAST_PoissonDisk((float)i, (float)PCSS_BLOCKER_COUNT, jitter_angle, blocker_radius_scale);
    float2 sample_uv = saturate(shadow_uv + offset * search_radius);
    float blocker_z = shadowMaps.SampleLevel(samPoint_s, float3(sample_uv, slice), 0).x;
    if (blocker_z < receiver_z) {
      blocker_depth_sum += blocker_z;
      blocker_count += 1;
    }
  }

  // If no blockers, fully lit
  if (blocker_count < 1) return 1.0;

  // Step 2: Penumbra estimate
  float avg_blocker = blocker_depth_sum / blocker_count;
  float depth_diff = receiver_z - avg_blocker;
  float penumbra = min(shader_injection_data.shadow_pcss_depth_cap, depth_diff)
                   * shader_injection_data.shadow_penumbra_scale
                   + shader_injection_data.shadow_base_softness;
  // filter_radius = penumbra × base_radius × user width multiplier
  float2 filter_radius = penumbra * base_radius * shader_injection_data.shadow_pcss_filter_width;
  filter_radius = max(invShadowSize_g, filter_radius);  // at least 1 texel
  // Fix C: Enforce minimum filter radius in texels
  if (shader_injection_data.shadow_pcss_fix_min_radius > 0.01f) {
    float2 min_radius_uv = shader_injection_data.shadow_pcss_fix_min_radius * invShadowSize_g;
    filter_radius = max(filter_radius, min_radius_uv);
  }

  // Step 3: Variable-radius PCF
  float shadow = 0;
  for (uint j = 0u; j < PCSS_SAMPLE_COUNT; j++) {
    float2 offset = ISFAST_PoissonDisk((float)j, (float)PCSS_SAMPLE_COUNT, jitter_angle, filter_radius_scale);
    float2 sample_uv = saturate(shadow_uv + offset * filter_radius);
    shadow += shadowMaps.SampleCmpLevelZero(SmplShadow_s, float3(sample_uv, slice), receiver_z).x;
  }
  return shadow / (float)PCSS_SAMPLE_COUNT;
}


void main(
  float4 v0 : SV_Position0,
  float4 v1 : TEXCOORD0,
  out float4 o0 : SV_Target0)
{
  const float4 icb[] = { { 1.000000, 0, 0, 0},
                              { 0, 1.000000, 0, 0},
                              { 0, 0, 1.000000, 0},
                              { 0, 0, 0, 1.000000},
                              { -0.840520, -0.073954, 0, 0},
                              { -0.326235, -0.405830, 0, 0},
                              { -0.698464, 0.457259, 0, 0},
                              { -0.203356, 0.620585, 0, 0},
                              { 0.963450, -0.194353, 0, 0},
                              { 0.473434, -0.480026, 0, 0},
                              { 0.519454, 0.767034, 0, 0},
                              { 0.185461, -0.894523, 0, 0},
                              { 0.507351, 0.064963, 0, 0},
                              { -0.321932, 0.595435, 0, 0} };
  float4 r0,r1,r2,r3,r4;
  uint4 bitmask, uiDest;
  float4 fDest;

  // ── PCSS jitter setup ──
  float pcss_jitter_angle = 0;
  bool pcss_active = shader_injection_data.shadow_filter_method > 1.5f;
  if (pcss_active && shader_injection_data.shadow_pcss_jitter_enabled > 0.5f) {
    uint2 jitter_pixel = uint2(floor(v0.xy));
    float jitter_phase_static = IGN(float2(jitter_pixel));
    uint jitter_frame = (uint)(sceneTime_g * shader_injection_data.shadow_pcss_jitter_speed);
    float2 jitter_noise;
    if (shader_injection_data.shadow_isfast_enabled > 0.5f) {
      jitter_noise = SpatioTemporalNoise_ISFAST(jitter_pixel, jitter_frame);
    } else {
      static const float R2_A1 = 0.7548776662466927;
      float b = IGN(float2(jitter_pixel) + float2(47, 17));
      jitter_noise = float2(frac(b + R2_A1 * (float)jitter_frame), jitter_phase_static);
    }
    float jitter_phase = lerp(jitter_phase_static, jitter_noise.x, shader_injection_data.shadow_pcss_jitter_amount);
    pcss_jitter_angle = 6.28318548 * jitter_phase;
  }

  // Fix D: Auto-scale cascade blend with split distance
  float pcss_blend = shader_injection_data.shadow_pcss_cascade_blend;
  if (shader_injection_data.shadow_pcss_fix_auto_blend > 0.5f) {
    pcss_blend *= shadowSplitDistance_g.y / 200.0;
  }

  r0.z = depthTexture.SampleLevel(samPoint_s, v1.xy, 0).x;
  r0.xy = v1.zw * float2(2,-2) + float2(-1,1);
  r0.w = 1;
  r1.x = dot(r0.xyzw, viewProjInv_g._m00_m10_m20_m30);
  r1.y = dot(r0.xyzw, viewProjInv_g._m01_m11_m21_m31);
  r1.z = dot(r0.xyzw, viewProjInv_g._m02_m12_m22_m32);
  r1.w = dot(r0.xyzw, viewProjInv_g._m03_m13_m23_m33);
  r0.xyzw = r1.xyzw / r1.wwww;
  r1.x = -viewInv_g._m30;
  r1.y = -viewInv_g._m31;
  r1.z = -viewInv_g._m32;
  r1.xyz = r1.xyz + r0.xyz;
  r1.x = dot(r1.xyz, r1.xyz);
  r1.x = sqrt(r1.x);
  r1.y = cmp(4 == shadowmapCascadeCount_g);
  r1.z = shadowSplitDistance_g.z + -5;
  r1.z = cmp(r1.z < r1.x);
  r1.y = r1.z ? r1.y : 0;
  if (r1.y != 0) {
    r2.x = dot(r0.xyzw, shadowMtx_g[3]._m00_m10_m20_m30);
    r2.y = dot(r0.xyzw, shadowMtx_g[3]._m01_m11_m21_m31);
    r2.z = dot(r0.xyzw, shadowMtx_g[3]._m02_m12_m22_m32);
    r1.y = dot(r0.xyzw, shadowMtx_g[3]._m03_m13_m23_m33);
    r1.yzw = r2.xyz / r1.yyy;
    r2.xy = cmp(r1.yz < float2(0,0));
    r2.zw = cmp(float2(1,1) < r1.yz);
    r2.x = (int)r2.z | (int)r2.x;
    r2.x = (int)r2.y | (int)r2.x;
    r2.x = (int)r2.w | (int)r2.x;
    if (r2.x != 0) {
      r2.x = 1;
    } else {
      r3.z = 3;
      if (shader_injection_data.shadow_filter_method > 1.5f) {
        r2.x = PCSS_Shadow(r1.yz, r1.w, 3, shadowSplitDistance_g.z, pcss_jitter_angle);
      } else if (shader_injection_data.shadow_filter_method > 0.5f) {
      r2.yz = float2(0,0);
      while (true) {
        r2.w = cmp((int)r2.z >= 10);
        if (r2.w != 0) break;
        r3.xy = saturate(icb[r2.z+4].xy * invShadowSize_g.xy + r1.yz);
        r2.w = shadowMaps.SampleCmpLevelZero(SmplShadow_s, r3.xyz, r1.w).x;
        r2.y = r2.y + r2.w;
        r2.z = (int)r2.z + 1;
      }
      r2.x = 0.100000001 * r2.y;
      } else {
        r3.xy = saturate(r1.yz);
        r2.x = shadowMaps.SampleCmpLevelZero(SmplShadow_s, r3.xyz, r1.w).x;
      }
    }
    r1.y = cmp(r1.x < shadowSplitDistance_g.z);
    if (r1.y != 0) {
      r3.x = dot(r0.xyzw, shadowMtx_g[2]._m00_m10_m20_m30);
      r3.y = dot(r0.xyzw, shadowMtx_g[2]._m01_m11_m21_m31);
      r3.z = dot(r0.xyzw, shadowMtx_g[2]._m02_m12_m22_m32);
      r1.y = dot(r0.xyzw, shadowMtx_g[2]._m03_m13_m23_m33);
      r1.yzw = r3.xyz / r1.yyy;
      r3.z = 2;
      if (shader_injection_data.shadow_filter_method > 1.5f) {
        r2.w = PCSS_Shadow(r1.yz, r1.w, 2, shadowSplitDistance_g.z, pcss_jitter_angle);
        r1.y = shadowSplitDistance_g.z + -r1.x;
        r1.y = pcss_blend * r1.y;
        r1.z = r2.w + -r2.x;
        r2.x = r1.y * r1.z + r2.x;
      } else if (shader_injection_data.shadow_filter_method > 0.5f) {
      r2.yz = float2(0,0);
      while (true) {
        r2.w = cmp((int)r2.z >= 10);
        if (r2.w != 0) break;
        r3.xy = saturate(icb[r2.z+4].xy * invShadowSize_g.xy + r1.yz);
        r2.w = shadowMaps.SampleCmpLevelZero(SmplShadow_s, r3.xyz, r1.w).x;
        r2.y = r2.y + r2.w;
        r2.z = (int)r2.z + 1;
      }
      r1.y = shadowSplitDistance_g.z + -r1.x;
      r1.y = pcss_blend * r1.y;
      r1.z = r2.y * 0.100000001 + -r2.x;
      r2.x = r1.y * r1.z + r2.x;
      } else {
        r3.xy = saturate(r1.yz);
        r2.w = shadowMaps.SampleCmpLevelZero(SmplShadow_s, r3.xyz, r1.w).x;
        r1.y = shadowSplitDistance_g.z + -r1.x;
        r1.y = pcss_blend * r1.y;
        r1.z = r2.w + -r2.x;
        r2.x = r1.y * r1.z + r2.x;
      }
    }
  } else {
    r1.y = shadowSplitDistance_g.y + -5;
    r1.y = cmp(r1.y < r1.x);
    if (r1.y != 0) {
      r3.x = dot(r0.xyzw, shadowMtx_g[2]._m00_m10_m20_m30);
      r3.y = dot(r0.xyzw, shadowMtx_g[2]._m01_m11_m21_m31);
      r3.z = dot(r0.xyzw, shadowMtx_g[2]._m02_m12_m22_m32);
      r1.y = dot(r0.xyzw, shadowMtx_g[2]._m03_m13_m23_m33);
      r1.yzw = r3.xyz / r1.yyy;
      r2.yz = cmp(r1.yz < float2(0,0));
      r3.xy = cmp(float2(1,1) < r1.yz);
      r2.y = (int)r2.y | (int)r3.x;
      r2.y = (int)r2.z | (int)r2.y;
      r2.y = (int)r3.y | (int)r2.y;
      if (r2.y != 0) {
        r2.x = 1;
      } else {
        r3.z = 2;
        if (shader_injection_data.shadow_filter_method > 1.5f) {
          r2.x = PCSS_Shadow(r1.yz, r1.w, 2, shadowSplitDistance_g.y, pcss_jitter_angle);
        } else if (shader_injection_data.shadow_filter_method > 0.5f) {
        r2.yz = float2(0,0);
        while (true) {
          r2.w = cmp((int)r2.z >= 10);
          if (r2.w != 0) break;
          r3.xy = saturate(icb[r2.z+4].xy * invShadowSize_g.xy + r1.yz);
          r2.w = shadowMaps.SampleCmpLevelZero(SmplShadow_s, r3.xyz, r1.w).x;
          r2.y = r2.y + r2.w;
          r2.z = (int)r2.z + 1;
        }
        r2.x = 0.100000001 * r2.y;
        } else {
          r3.xy = saturate(r1.yz);
          r2.x = shadowMaps.SampleCmpLevelZero(SmplShadow_s, r3.xyz, r1.w).x;
        }
      }
      r1.y = cmp(r1.x < shadowSplitDistance_g.y);
      if (r1.y != 0) {
        r3.x = dot(r0.xyzw, shadowMtx_g[1]._m00_m10_m20_m30);
        r3.y = dot(r0.xyzw, shadowMtx_g[1]._m01_m11_m21_m31);
        r3.z = dot(r0.xyzw, shadowMtx_g[1]._m02_m12_m22_m32);
        r1.y = dot(r0.xyzw, shadowMtx_g[1]._m03_m13_m23_m33);
        r1.yzw = r3.xyz / r1.yyy;
        r2.yz = invShadowSize_g.xy * float2(1.125,1.125);
        r3.z = 1;
        if (shader_injection_data.shadow_filter_method > 1.5f) {
          r2.w = PCSS_Shadow(r1.yz, r1.w, 1, shadowSplitDistance_g.y, pcss_jitter_angle);
          r1.y = shadowSplitDistance_g.y + -r1.x;
          r1.y = pcss_blend * r1.y;
          r1.z = r2.w + -r2.x;
          r2.x = r1.y * r1.z + r2.x;
        } else if (shader_injection_data.shadow_filter_method > 0.5f) {
        r2.w = 0;
        r3.w = 0;
        while (true) {
          r4.x = cmp((int)r3.w >= 10);
          if (r4.x != 0) break;
          r3.xy = saturate(icb[r3.w+4].xy * r2.yz + r1.yz);
          r3.x = shadowMaps.SampleCmpLevelZero(SmplShadow_s, r3.xyz, r1.w).x;
          r2.w = r3.x + r2.w;
          r3.w = (int)r3.w + 1;
        }
        r1.y = shadowSplitDistance_g.y + -r1.x;
        r1.y = pcss_blend * r1.y;
        r1.z = r2.w * 0.100000001 + -r2.x;
        r2.x = r1.y * r1.z + r2.x;
        } else {
          r3.xy = saturate(r1.yz);
          r2.w = shadowMaps.SampleCmpLevelZero(SmplShadow_s, r3.xyz, r1.w).x;
          r1.y = shadowSplitDistance_g.y + -r1.x;
          r1.y = pcss_blend * r1.y;
          r1.z = r2.w + -r2.x;
          r2.x = r1.y * r1.z + r2.x;
        }
      }
    } else {
      r1.y = cmp(r1.x < shadowSplitDistance_g.x);
      r2.yzw = r1.yyy ? float3(0,0,0) : float3(1,4,1);
      r3.x = dot(r0.xyzw, shadowMtx_g[r2.z/4]._m00_m10_m20_m30);
      r3.y = dot(r0.xyzw, shadowMtx_g[r2.z/4]._m01_m11_m21_m31);
      r3.z = dot(r0.xyzw, shadowMtx_g[r2.z/4]._m02_m12_m22_m32);
      r1.z = dot(r0.xyzw, shadowMtx_g[r2.z/4]._m03_m13_m23_m33);
      r3.xyz = r3.xyz / r1.zzz;
      if (shader_injection_data.shadow_filter_method > 1.5f) {
        r2.x = PCSS_Shadow(r3.xy, r3.z, r2.w, shadowSplitDistance_g.x, pcss_jitter_angle);
      } else if (shader_injection_data.shadow_filter_method > 0.5f) {
      r1.z = dot(float2(1.25,1.125), icb[r2.y+0].xy);
      r1.zw = invShadowSize_g.xy * r1.zz;
      r3.w = 0;
      r4.x = 0;
      while (true) {
        r4.y = cmp((int)r4.x >= 10);
        if (r4.y != 0) break;
        r2.yz = saturate(icb[r4.x+4].xy * r1.zw + r3.xy);
        r2.y = shadowMaps.SampleCmpLevelZero(SmplShadow_s, r2.yzw, r3.z).x;
        r3.w = r3.w + r2.y;
        r4.x = (int)r4.x + 1;
      }
      r2.x = 0.100000001 * r3.w;
      } else {
        r2.yz = saturate(r3.xy);
        r2.x = shadowMaps.SampleCmpLevelZero(SmplShadow_s, r2.yzw, r3.z).x;
      }
      r1.z = shadowSplitDistance_g.x + -5;
      r1.z = cmp(r1.z < r1.x);
      r1.y = r1.z ? r1.y : 0;
      if (r1.y != 0) {
        r3.x = dot(r0.xyzw, shadowMtx_g[1]._m00_m10_m20_m30);
        r3.y = dot(r0.xyzw, shadowMtx_g[1]._m01_m11_m21_m31);
        r3.z = dot(r0.xyzw, shadowMtx_g[1]._m02_m12_m22_m32);
        r0.x = dot(r0.xyzw, shadowMtx_g[1]._m03_m13_m23_m33);
        r0.xyz = r3.xyz / r0.xxx;
        r1.yz = invShadowSize_g.xy * float2(1.125,1.125);
        r3.z = 1;
        if (shader_injection_data.shadow_filter_method > 1.5f) {
          r0.w = PCSS_Shadow(r0.xy, r0.z, 1, shadowSplitDistance_g.x, pcss_jitter_angle);
          r0.y = shadowSplitDistance_g.x + -r1.x;
          r0.y = pcss_blend * r0.y;
          r0.z = r0.w + -r2.x;
          r2.x = r0.y * r0.z + r2.x;
        } else if (shader_injection_data.shadow_filter_method > 0.5f) {
        r0.w = 0;
        r1.w = 0;
        while (true) {
          r2.y = cmp((int)r1.w >= 10);
          if (r2.y != 0) break;
          r3.xy = saturate(icb[r1.w+4].xy * r1.yz + r0.xy);
          r2.y = shadowMaps.SampleCmpLevelZero(SmplShadow_s, r3.xyz, r0.z).x;
          r0.w = r2.y + r0.w;
          r1.w = (int)r1.w + 1;
        }
        r0.y = shadowSplitDistance_g.x + -r1.x;
        r0.xy = float2(0.100000001,pcss_blend) * r0.wy;
        r0.z = r3.w * 0.100000001 + -r0.x;
        r2.x = r0.y * r0.z + r0.x;
        } else {
          r3.xy = saturate(r0.xy);
          r0.w = shadowMaps.SampleCmpLevelZero(SmplShadow_s, r3.xyz, r0.z).x;
          r0.y = shadowSplitDistance_g.x + -r1.x;
          r0.y = pcss_blend * r0.y;
          r0.z = r0.w + -r2.x;
          r2.x = r0.y * r0.z + r2.x;
        }
      }
    }
  }
  r0.x = -shadowFadeNear_g + r1.x;
  r0.x = saturate(shadowFadeRangeInv_g * r0.x);
  r0.y = 1 + -r2.x;
  o0.xyzw = r0.xxxx * r0.yyyy + r2.xxxx;
  return;
}