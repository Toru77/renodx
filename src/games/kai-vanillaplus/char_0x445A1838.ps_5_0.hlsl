// ---- Created with 3Dmigoto v1.4.1 on Sat Feb 21 18:39:08 2026

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
  float ldotvXZ_g : packoffset(c25.w);
  float3 lightDirection_g : packoffset(c26);
  float gameTime_g : packoffset(c26.w);
  float3 sceneShadowColor_g : packoffset(c27);
  float chrLightIntensity_g : packoffset(c27.w);
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
  float disableMapObjNearFade_g : packoffset(c32.w);
  float4x4 ditherMtx_g : packoffset(c33);
  float4 lightProbe_g[9] : packoffset(c37);
  float3 chrLightDir_g : packoffset(c46);
  float windForce_g : packoffset(c46.w);
  float4 mapColor_g : packoffset(c47);
  float4 clipPlane_g : packoffset(c48);
  float2 resolutionScaling_g : packoffset(c49);
  float2 shadowSplitDistance_g : packoffset(c49.z);
  float4x4 shadowMtx_g[3] : packoffset(c50);
  float shadowFadeNear_g : packoffset(c62);
  float shadowFadeRangeInv_g : packoffset(c62.y);
  float2 invShadowSize_g : packoffset(c62.z);
  float4 frustumPlanes_g[6] : packoffset(c63);
  float4x4 prevView_g : packoffset(c69);
  float4x4 prevViewInv_g : packoffset(c73);
  float4x4 prevProj_g : packoffset(c77);
  float4x4 prevProjInv_g : packoffset(c81);
  float4x4 prevViewProj_g : packoffset(c85);
  float4x4 prevViewProjInv_g : packoffset(c89);
  float2 motionJitterOffset_g : packoffset(c93);
  float2 curJitterOffset_g : packoffset(c93.z);
  float prevSceneTime_g : packoffset(c94);
  uint enableMotionVectors_g : packoffset(c94.y);
  float prevWindWaveTime_g : packoffset(c94.z);
  float padding : packoffset(c94.w);
}

cbuffer cb_local : register(b2)
{
  int2 textureSize_g : packoffset(c0);
  float2 aoSize_g : packoffset(c0.z);
  float charaAOStrength_g : packoffset(c1);
  float charaAOCutOff_g : packoffset(c1.y);
  float mapAORadius_g : packoffset(c1.z);
  float mapAOFadeBeginDistance_g : packoffset(c1.w);
  float mapAOFadeRangeInv_g : packoffset(c2);
  float mapAOLimit_g : packoffset(c2.y);
  float mapAOBias_g : packoffset(c2.z);
  float mapAOIntensity_g : packoffset(c2.w);
  float2 texelSize_g : packoffset(c3);
  float2 prevAoScaling_g : packoffset(c3.z);
  float4 sphere_g[16] : packoffset(c4);
}

cbuffer cb_local2 : register(b3)
{
  float3 rayMarchShadowDir_g : packoffset(c0);
}

SamplerState samPoint_s : register(s0);
SamplerState samLinear_s : register(s1);
Texture2D<float4> depthTexture : register(t0);
Texture2D<uint4> mrtTexture0 : register(t2);
Texture2D<float4> prevTexture : register(t3);

#include "./kai-vanillaplus.h"

// 3Dmigoto declarations
#define cmp -

static const float CHAR_SHADOW_FAR_DEPTH_VALUE = 0.0;

static float ComputeCharacterSunShadow(float2 uv, float3 normalWS)
{
  int sampleCount = max((int)round(sss_injection_data.char_shadow_sample_count), 1);
  int hardShadowSamples = clamp((int)round(sss_injection_data.char_shadow_hard_shadow_samples), 0, sampleCount);
  int fadeOutSamples = clamp((int)round(sss_injection_data.char_shadow_fade_out_samples), 0, sampleCount);
  if ((hardShadowSamples + fadeOutSamples) > sampleCount) {
    fadeOutSamples = max(sampleCount - hardShadowSamples, 0);
  }

  float surfaceThickness = max(sss_injection_data.char_shadow_surface_thickness, 1e-5);
  float shadowContrast = max(sss_injection_data.char_shadow_contrast, 0.0);
  float fadeStart = min(sss_injection_data.char_shadow_light_screen_fade_start, sss_injection_data.char_shadow_light_screen_fade_end);
  float fadeEnd = max(sss_injection_data.char_shadow_light_screen_fade_start, sss_injection_data.char_shadow_light_screen_fade_end);
  float minOccluderDepthScale = max(sss_injection_data.char_shadow_min_occluder_depth_scale, 0.0);
  bool useJitter = sss_injection_data.char_shadow_jitter_enabled >= 0.5;

  float startDepth = depthTexture.SampleLevel(samPoint_s, resolutionScaling_g.xy * uv, 0).x;
  if (startDepth <= CHAR_SHADOW_FAR_DEPTH_VALUE) {
    return 1.0;
  }

  float normalLenSq = dot(normalWS, normalWS);
  if (normalLenSq <= 1e-8) {
    return 1.0;
  }
  float3 normalDirWS = normalWS * rsqrt(normalLenSq);

  float4 clipPos = float4(uv * float2(2, -2) + float2(-1, 1), startDepth, 1.0);
  float4 viewPosH;
  viewPosH.x = dot(clipPos, projInv_g._m00_m10_m20_m30);
  viewPosH.y = dot(clipPos, projInv_g._m01_m11_m21_m31);
  viewPosH.z = dot(clipPos, projInv_g._m02_m12_m22_m32);
  viewPosH.w = dot(clipPos, projInv_g._m03_m13_m23_m33);
  if (abs(viewPosH.w) <= 1e-6) {
    return 1.0;
  }
  float3 viewPos = viewPosH.xyz / viewPosH.www;

  float4 viewPos4 = float4(viewPos, 1.0);
  float3 worldPos;
  worldPos.x = dot(viewPos4, viewInv_g._m00_m10_m20_m30);
  worldPos.y = dot(viewPos4, viewInv_g._m01_m11_m21_m31);
  worldPos.z = dot(viewPos4, viewInv_g._m02_m12_m22_m32);

  float lightDirLenSq = dot(lightDirection_g.xyz, lightDirection_g.xyz);
  if (lightDirLenSq <= 1e-8) {
    return 1.0;
  }
  float3 pixelToLightWS = -lightDirection_g.xyz * rsqrt(lightDirLenSq);
  float ndotl = saturate(abs(dot(normalDirWS, pixelToLightWS)));
  if (ndotl <= 0.001) {
    return 1.0;
  }

  float2 lightViewXY;
  lightViewXY.x = dot(lightDirection_g.xyz, view_g._m00_m10_m20);
  lightViewXY.y = dot(lightDirection_g.xyz, view_g._m01_m11_m21);
  float lightLenSq = dot(lightViewXY, lightViewXY);
  if (lightLenSq <= 1e-6) {
    return 1.0;
  }

  float lightScreenLen = sqrt(lightLenSq);
  float lightScreenFade = saturate((lightScreenLen - fadeStart) / max(fadeEnd - fadeStart, 1e-5));
  if (lightScreenFade <= 0.001) {
    return 1.0;
  }

  float jitter = 0.0;
  if (useJitter) {
    jitter = dot(uv * vpSize_g.xy, float2(0.0671105608, 0.00583714992));
    jitter = frac(52.9829178 * frac(jitter));
  }

  float3 viewForwardWS = float3(viewInv_g._m20, viewInv_g._m21, viewInv_g._m22);
  viewForwardWS = viewForwardWS * rsqrt(max(dot(viewForwardWS, viewForwardWS), 1e-8));
  float normalViewFactor = saturate(abs(dot(normalDirWS, viewForwardWS)));
  float surfaceStartBias = normalViewFactor * -0.00249999994 + 0.00749999983;
  float rayStepLength = normalViewFactor * 0.00150000001 + 0.000500000024;
  float3 rayStepWS = pixelToLightWS * rayStepLength;
  float3 rayStartWS = worldPos + normalDirWS * surfaceStartBias;

  float depthThickness = max(abs(CHAR_SHADOW_FAR_DEPTH_VALUE - startDepth) * surfaceThickness, 1e-5);
  float hardShadow = 1.0;
  float4 shadowValue = 1.0;
  int validSamples = 0;

  [loop]
  for (int i = 0; i < sampleCount; ++i) {
    float sampleIndex = (float)(i + 1) + jitter;
    float3 samplePosWS = rayStartWS + rayStepWS * sampleIndex;
    float4 samplePosWS4 = float4(samplePosWS, 1.0);

    float4 sampleClip;
    sampleClip.x = dot(samplePosWS4, viewProj_g._m00_m10_m20_m30);
    sampleClip.y = dot(samplePosWS4, viewProj_g._m01_m11_m21_m31);
    sampleClip.z = dot(samplePosWS4, viewProj_g._m02_m12_m22_m32);
    sampleClip.w = dot(samplePosWS4, viewProj_g._m03_m13_m23_m33);
    if (abs(sampleClip.w) <= 1e-6) {
      break;
    }

    float3 sampleNdc = sampleClip.xyz / sampleClip.www;
    float2 sampleUV = sampleNdc.xy * float2(0.5, -0.5) + float2(0.5, 0.5);
    if (sampleUV.x <= invVPSize_g.x || sampleUV.y <= invVPSize_g.y ||
        sampleUV.x >= (1.0 - invVPSize_g.x) || sampleUV.y >= (1.0 - invVPSize_g.y)) {
      break;
    }

    validSamples++;

    float sampleDepth = depthTexture.SampleLevel(samPoint_s, resolutionScaling_g.xy * sampleUV, 0).x;
    float shadowSample = 1.0;
    float depthDelta = sampleDepth - sampleNdc.z;
    if (depthDelta > (depthThickness * minOccluderDepthScale)) {
      float depthMatch = abs(depthDelta / depthThickness - 1.0);
      shadowSample = saturate(depthMatch * shadowContrast + (1.0 - shadowContrast));
    }

    if (i < hardShadowSamples) {
      hardShadow = min(hardShadow, shadowSample);
    } else {
      if (i >= (sampleCount - fadeOutSamples)) {
        float fadeOut = (float)(i + 1 - (sampleCount - fadeOutSamples)) / (float)(fadeOutSamples + 1) * 0.75;
        shadowSample = saturate(shadowSample + fadeOut);
      }
      int bucket = i & 3;
      if (bucket == 0) shadowValue.x = min(shadowValue.x, shadowSample);
      if (bucket == 1) shadowValue.y = min(shadowValue.y, shadowSample);
      if (bucket == 2) shadowValue.z = min(shadowValue.z, shadowSample);
      if (bucket == 3) shadowValue.w = min(shadowValue.w, shadowSample);
    }
  }

  float result = dot(shadowValue, 0.25);
  result = min(result, hardShadow);
  float validSampleFade = saturate((float)validSamples / (float)sampleCount);
  validSampleFade = validSampleFade * validSampleFade;
  float directionalStrength = lightScreenFade * ndotl * validSampleFade;
  return lerp(1.0, result, directionalStrength);
}


void main(
  float4 v0 : SV_Position0,
  float4 v1 : TEXCOORD0,
  out float4 o0 : SV_Target0)
{
  float4 r0,r1,r2,r3,r4,r5;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.z = depthTexture.SampleLevel(samPoint_s, v1.xy, 0).x;
  mrtTexture0.GetDimensions(0, fDest.x, fDest.y, fDest.z);
  r1.xy = fDest.xy;
  r1.xy = v1.xy * r1.xy + float2(-0.5,-0.5);
  r1.xy = max(float2(0,0), r1.xy);
  r1.xy = (int2)r1.xy;
  r1.zw = float2(0,0);
  r1.xyz = mrtTexture0.Load(r1.xyz).xyz;
  if (1 == 0) r1.z = 0; else if (1+8 < 32) {   r1.z = (uint)r1.z << (32-(1 + 8)); r1.z = (uint)r1.z >> (32-1);  } else r1.z = (uint)r1.z >> 8;
  if (r1.z != 0) {
    r0.xy = v1.zw * float2(2,-2) + float2(-1,1);
    r0.w = 1;
    r2.x = dot(r0.xyzw, projInv_g._m00_m10_m20_m30);
    r2.y = dot(r0.xyzw, projInv_g._m01_m11_m21_m31);
    r2.z = dot(r0.xyzw, projInv_g._m02_m12_m22_m32);
    r1.z = dot(r0.xyzw, projInv_g._m03_m13_m23_m33);
    r2.xyz = r2.xyz / r1.zzz;
    r1.xy = (uint2)r1.xy;
    r1.zw = r1.xy * float2(3.05180438e-05,3.05180438e-05) + float2(-1,-1);
    r1.z = 3.14159274 * r1.z;
    sincos(r1.z, r3.x, r4.x);
    r1.z = -r1.w * r1.w + 1;
    r1.z = sqrt(r1.z);
    r1.x = r4.x * r1.z;
    r1.y = r3.x * r1.z;
    r1.z = dot(v1.zw, float2(12.9898005,78.2330017));
    r1.z = sin(r1.z);
    r1.z = 43758.5469 * r1.z;
    r1.z = frac(r1.z);
    r3.xyz = float3(5.39830017,5.44269991,6.93709993) * r1.zzz;
    r3.xyz = frac(r3.xyz);
    r4.xyz = float3(21.5351009,14.3136997,15.3219004) + r3.xyz;
    r1.z = dot(r3.yzx, r4.xyz);
    r3.xyz = r3.xyz + r1.zzz;
    r3.xyz = r3.xxy * r3.zyz;
    r3.xyz = float3(95.4337006,97.5970001,93.8365021) * r3.xyz;
    r3.xyz = frac(r3.xyz);
    r3.xyz = r3.xyz * float3(2,2,2) + float3(-1,-1,-1);
    r1.z = dot(r3.xyz, r3.xyz);
    r1.z = rsqrt(r1.z);
    r3.xyz = r3.xyz * r1.zzz;
    r1.z = charaAOStrength_g + -charaAOCutOff_g;
    r1.z = 1 / r1.z;
    r4.y = 1;
    r3.w = 0;
    r4.z = 0;
    while (true) {
      r4.w = cmp((int)r4.z >= 10);
      if (r4.w != 0) break;
      r4.w = dot(sphere_g[r4.z].xyz, r3.xyz);
      r4.w = r4.w + r4.w;
      r5.xyz = r3.xyz * -r4.www + sphere_g[r4.z].xyz;
      r4.w = dot(r5.xyz, r1.xyw);
      r5.z = cmp(0 < r4.w);
      r4.w = cmp(r4.w < 0);
      r4.w = (int)-r5.z + (int)r4.w;
      r4.w = (int)r4.w;
      r5.xy = r4.ww * r5.xy;
      r5.xy = aoSize_g.xy * r5.xy;
      r5.xy = r5.xy * float2(1,-1) + v1.zw;
      r5.xy = resolutionScaling_g.xy * r5.xy;
      r4.x = depthTexture.SampleLevel(samPoint_s, r5.xy, 0).x;
      r4.w = dot(projInv_g._m22_m32, r4.xy);
      r4.x = dot(projInv_g._m23_m33, r4.xy);
      r4.x = r4.w / r4.x;
      r4.x = r4.x + -r2.z;
      r4.w = cmp(r4.x >= charaAOCutOff_g);
      r4.w = r4.w ? 1.000000 : 0;
      r4.x = -charaAOCutOff_g + r4.x;
      r4.x = saturate(r4.x * r1.z);
      r5.x = r4.x * -2 + 3;
      r4.x = r4.x * r4.x;
      r4.x = -r5.x * r4.x + 1;
      r3.w = r4.w * r4.x + r3.w;
      r4.z = (int)r4.z + 1;
    }
    r1.z = 0.100000001 * r3.w;
    int charShadowMode = clamp((int)round(sss_injection_data.char_shadow_mode), 0, 2);
    if (charShadowMode == 1) {
      r2.w = 1;
      r3.x = dot(r2.xyzw, viewInv_g._m00_m10_m20_m30);
      r3.y = dot(r2.xyzw, viewInv_g._m01_m11_m21_m31);
      r3.z = dot(r2.xyzw, viewInv_g._m02_m12_m22_m32);
      r2.x = viewInv_g._m20;
      r2.y = viewInv_g._m21;
      r2.z = viewInv_g._m22;
      r2.x = dot(r1.xyw, r2.xyz);
      r2.xy = abs(r2.xx) * float2(-0.00249999994,0.00150000001) + float2(0.00749999983,0.000500000024);
      r1.xyw = r1.xyw * r2.xxx + r3.xyz;
      r2.xyz = rayMarchShadowDir_g.xyz * r2.yyy;
      r3.w = 1;
      r2.w = 0;
      r4.x = 0;
      while (true) {
        r4.y = cmp((int)r4.x >= 10);
        if (r4.y != 0) break;
        r4.y = (int)r4.x;
        r3.xyz = r2.xyz * r4.yyy + r1.xyw;
        r5.x = dot(r3.xyzw, viewProj_g._m00_m10_m20_m30);
        r5.y = dot(r3.xyzw, viewProj_g._m01_m11_m21_m31);
        r5.z = dot(r3.xyzw, viewProj_g._m02_m12_m22_m32);
        r3.x = dot(r3.xyzw, viewProj_g._m03_m13_m23_m33);
        r3.xyz = r5.xyz / r3.xxx;
        r5.xy = r3.xy * float2(0.5,0.5) + float2(0.5,0.5);
        r5.z = 1 + -r5.y;
        r3.xy = resolutionScaling_g.xy * r5.xz;
        r3.x = depthTexture.SampleLevel(samPoint_s, r3.xy, 0).x;
        r3.x = r3.x + -r3.z;
        r3.x = saturate(400000 * r3.x);
        r2.w = r3.x * 0.25 + r2.w;
        r4.x = (int)r4.x + 1;
      }
      r1.x = 1 + -r2.w;
      r1.x = max(0, r1.x);
    } else if (charShadowMode == 2) {
      r1.x = ComputeCharacterSunShadow(v1.zw, r1.xyw);
    } else {
      r1.x = 1;
    }

    if (charShadowMode >= 1) {
      float charShadowStrength = saturate(sss_injection_data.char_shadow_strength);
      r1.x = lerp(1.0, r1.x, charShadowStrength);
    }

    r1.yw = float2(0.5,0.5) + -v1.zw;
    r1.y = max(abs(r1.y), abs(r1.w));
    r1.y = -0.449999988 + r1.y;
    r1.y = max(0, r1.y);
    r1.y = 20 * r1.y;
    r1.w = 1 + -r1.x;
    r2.y = r1.y * r1.w + r1.x;
  } else {
    r1.z = 0;
    r2.y = 1;
  }
  r2.x = 1 + -r1.z;
  r0.xy = v1.zw * float2(2,-2) + float2(-1,1);
  r0.w = 1;
  r1.x = dot(r0.xyzw, viewProjInv_g._m00_m10_m20_m30);
  r1.y = dot(r0.xyzw, viewProjInv_g._m01_m11_m21_m31);
  r1.z = dot(r0.xyzw, viewProjInv_g._m02_m12_m22_m32);
  r1.w = dot(r0.xyzw, viewProjInv_g._m03_m13_m23_m33);
  r0.xyzw = r1.xyzw / r1.wwww;
  r1.x = dot(r0.xyzw, prevViewProj_g._m00_m10_m20_m30);
  r1.y = dot(r0.xyzw, prevViewProj_g._m01_m11_m21_m31);
  r0.x = dot(r0.xyzw, prevViewProj_g._m03_m13_m23_m33);
  r0.xy = r1.xy / r0.xx;
  r0.xy = r0.xy * float2(0.5,0.5) + float2(0.5,0.5);
  r0.z = 1 + -r0.y;
  r0.xy = prevAoScaling_g.xy * r0.xz;
  r0.xy = resolutionScaling_g.xy * r0.xy;
  r0.xyz = prevTexture.SampleLevel(samLinear_s, r0.xy, 0).xyz;
  r2.z = 1;
  r1.xyz = r2.zxy + -r0.xyz;
  o0.xyz = r1.xyz * float3(0.25,0.25,1) + r0.xyz;
  o0.w = 1;
  return;
}
