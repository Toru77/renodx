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

cbuffer cb_volume_fog : register(b7)
{
  float volumeLightBrightness_g : packoffset(c0);
  float volumeDensity_g : packoffset(c0.y);
  float volumeCameraFarOverMaxFar_g : packoffset(c0.z);
  float volumeCameraFarClip_g : packoffset(c0.w);
  float volumeNearOverFarClip_g : packoffset(c1);
  float volumeNearDistance_g : packoffset(c1.y);
  float volumeFarDistance_g : packoffset(c1.z);
  uint volumeShapeCount_g : packoffset(c1.w);
  float4 volumeColor_g : packoffset(c2);
  float2 voulumeLightTileSizeInv_g : packoffset(c3);
  float combineAlpha_g : packoffset(c3.z);
  float temporalRatio_g : packoffset(c3.w);
  float2 prevScaling_g : packoffset(c4);
  float2 prevUVClamp_g : packoffset(c4.z);
  float volumeNearFadeInv_g : packoffset(c5);
  float volumeNearFadeValue_g : packoffset(c5.y);
  float densityScale_g : packoffset(c5.w);
}

SamplerState samLinear_s : register(s0);
SamplerState samPoint_s : register(s1);
SamplerState samPointWrap_s : register(s2);
Texture2D<float4> colorTexture : register(t0);
Texture2D<float4> depthTexture : register(t1);
Texture3D<float4> volumeScatter : register(t2);
Texture3D<float2> g_isfastNoiseTexture : register(t3);


// Includes for improved volumetric haze AA
#include "../../reference/rendering.hlsl"
#include "../../shared.h"

// 3Dmigoto declarations
#define cmp -

// ── Interleaved Gradient Noise (IGN) — IS-FAST fallback ──
static float IGN(float2 p) {
  return frac(52.9829189 * frac(0.06711056 * p.x + 0.00583715 * p.y));
}

// ── Spatio-Temporal Blue Noise (IS-FAST) with IGN fallback ──
static float2 SpatioTemporalNoise_ISFAST(uint2 p, uint t) {
  if (shader_injection_data.volfog_isfast_texture_loaded > 0.5f) {
    float3 uvw = float3(
      (float)(p.x % 128u) / 128.0,
      (float)(p.y % 128u) / 128.0,
      (float)((t + 0u) % 32u) / 32.0);
    uvw.xy *= shader_injection_data.volfog_isfast_spatial_scale;
    float2 s = g_isfastNoiseTexture.SampleLevel(samPoint_s, uvw, 0);
    return s;
  } else {
    static const float R2_A1 = 0.7548776662466927;
    static const float R2_A2 = 0.5698402909980532;
    float b1 = IGN(float2(p) * shader_injection_data.volfog_isfast_spatial_scale);
    float b2 = IGN(float2(p) * shader_injection_data.volfog_isfast_spatial_scale + float2(47, 17));
    return float2(frac(b1 + R2_A1 * (float)t),
                  frac(b2 + R2_A2 * (float)t));
  }
}


void main(
  float4 v0 : SV_Position0,
  float4 v1 : TEXCOORD0,
  out float4 o0 : SV_Target0)
{
  float4 r0,r1;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.x = depthTexture.SampleLevel(samPoint_s, v1.xy, 0).x;
  r0.y = 1;
  r0.z = dot(projInv_g._m22_m32, r0.xy);
  r0.x = dot(projInv_g._m23_m33, r0.xy);
  r0.x = r0.z / r0.x;
  r0.x = -r0.x / volumeCameraFarClip_g;
  r0.x = r0.x * volumeCameraFarOverMaxFar_g + -volumeNearOverFarClip_g;
  r0.y = -volumeNearOverFarClip_g + 1;
  r0.z = r0.x / r0.y;
  r0.xy = v1.xy;
  // Sample the volume: vanilla trilinear or improved tricubic haze AA
  {
    float3 uvw = r0.xyz;
    uint volW, volH, volD;
    volumeScatter.GetDimensions(volW, volH, volD);
    float3 volSize = float3((float)volW, (float)volH, (float)volD);

    // ── IS-FAST temporal jitter ──
    if (shader_injection_data.volfog_isfast_enabled > 0.5f) {
      float2 pixelCoord = floor(v0.xy);
      float jitterSpeed = max(shader_injection_data.volfog_jitter_speed, 1.0f);
      uint frameIndex = (uint)max(sceneTime_g * jitterSpeed, 0.0f);
      float2 jitter2 = SpatioTemporalNoise_ISFAST((uint2)pixelCoord, frameIndex);

      float3 texelSize = 1.0f / max(volSize, float3(1.0f, 1.0f, 1.0f));
      float2 halfTexelXY = 0.5f * texelSize.xy;
      float halfSlice = 0.5f * texelSize.z;
      float jitterZ = frac(jitter2.x + jitter2.y * 0.5f);
      float jitterStrength = max(shader_injection_data.volfog_jitter_amount, 0.0f);

      uvw.xy = clamp(uvw.xy + (jitter2 - 0.5f) * texelSize.xy * jitterStrength, halfTexelXY, 1.0f - halfTexelXY);
      uvw.z = clamp(uvw.z + (jitterZ - 0.5f) * texelSize.z * jitterStrength, halfSlice, 1.0f - halfSlice);
    }

    float4 volSample;
    if (shader_injection_data.volfog_haze_aa_mode > 0.5) {
      volSample = renodx::rendering::SampleTricubicBSpline(volumeScatter, samLinear_s, uvw, volSize);
    } else {
      volSample = volumeScatter.SampleLevel(samLinear_s, uvw, 0);
    }
    r0.xyzw = volSample.xyzw;
  }
  r1.xyzw = colorTexture.SampleLevel(samPoint_s, v1.xy, 0).xyzw;
  r0.xyz = r1.xyz * r0.www + r0.xyz;
  r0.xyz = r0.xyz + -r1.xyz;
  o0.xyz = combineAlpha_g * r0.xyz + r1.xyz;
  o0.w = r1.w;
  return;
}