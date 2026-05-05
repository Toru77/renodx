#ifndef SRC_GAMES_SORA_VANILLAPLUS_REFERENCE_ENV_SSS_HLSL_
#define SRC_GAMES_SORA_VANILLAPLUS_REFERENCE_ENV_SSS_HLSL_

#include "rendering.hlsl"

static const float ENV_SSS_FAR_DEPTH_VALUE = 0.0;

static float3 DecodeNormalFromMrt(uint2 packed)
{
  float2 enc = (float2)packed * float2(3.05180438e-05, 3.05180438e-05) + float2(-1, -1);
  float angle = 3.14159274 * enc.x;
  float sn, cs;
  sincos(angle, sn, cs);
  float xy_len = sqrt(max(1.0 - enc.y * enc.y, 0.0));
  return float3(cs * xy_len, sn * xy_len, enc.y);
}

static float ComputeEnvSunShadow(float2 uv, float3 normalWS)
{
  int sampleCount = max((int)round(shader_injection_data.env_sss_sample_count), 1);
  int hardShadowSamples = max(sampleCount / 8, 1);
  int fadeOutSamples = max(sampleCount / 3, 1);
  if ((hardShadowSamples + fadeOutSamples) > sampleCount) {
    fadeOutSamples = max(sampleCount - hardShadowSamples, 0);
  }

  float surfaceThickness = max(shader_injection_data.env_sss_surface_thickness, 1e-5);
  float shadowContrast = max(shader_injection_data.env_sss_contrast, 0.0);
  bool useJitter = shader_injection_data.env_sss_jitter_enabled >= 0.5;

  float startDepth = depthTexture.SampleLevel(samPoint_s, resolutionScaling_g.xy * uv, 0).x;
  if (startDepth <= ENV_SSS_FAR_DEPTH_VALUE) {
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

  float envHeightMask = 1.0;
  if (shader_injection_data.env_sss_height_enabled >= 0.5) {
    float searchPixels = max(shader_injection_data.env_sss_height_max, 1.0);
    float2 groundUV = uv + float2(0.0, searchPixels * invVPSize_g.y);
    groundUV = saturate(groundUV);

    float groundDepth = depthTexture.SampleLevel(samPoint_s, resolutionScaling_g.xy * groundUV, 0).x;

    float4 groundClipPos = float4(groundUV * float2(2, -2) + float2(-1, 1), groundDepth, 1.0);
    float4 groundViewPosH;
    groundViewPosH.x = dot(groundClipPos, projInv_g._m00_m10_m20_m30);
    groundViewPosH.y = dot(groundClipPos, projInv_g._m01_m11_m21_m31);
    groundViewPosH.z = dot(groundClipPos, projInv_g._m02_m12_m22_m32);
    groundViewPosH.w = dot(groundClipPos, projInv_g._m03_m13_m23_m33);
    float3 groundViewPos = groundViewPosH.xyz / max(abs(groundViewPosH.w), 1e-6);

    float4 groundViewPos4 = float4(groundViewPos, 1.0);
    float3 groundWorldPos;
    groundWorldPos.x = dot(groundViewPos4, viewInv_g._m00_m10_m20_m30);
    groundWorldPos.y = dot(groundViewPos4, viewInv_g._m01_m11_m21_m31);
    groundWorldPos.z = dot(groundViewPos4, viewInv_g._m02_m12_m22_m32);

    float heightAboveGround = abs(worldPos.y - groundWorldPos.y);

    float heightThreshold = max(shader_injection_data.env_sss_height_min, 0.0);
    float heightFade = max(shader_injection_data.env_sss_height_fade, 1e-5);
    envHeightMask = 1.0 - smoothstep(heightThreshold, heightThreshold + heightFade, heightAboveGround);

    if (envHeightMask <= 1e-4) {
      return 1.0;
    }
  }

  float lightDirLenSq = dot(lightDirection_g.xyz, lightDirection_g.xyz);
  if (lightDirLenSq <= 1e-8) {
    return 1.0;
  }
  float3 pixelToLightWS = -lightDirection_g.xyz * rsqrt(lightDirLenSq);

  float verticalReject = shader_injection_data.env_sss_vertical_reject;
  if (verticalReject > 0.0) {
    float uprightness = abs(normalDirWS.y);
    float verticalMask = smoothstep(verticalReject - 0.1, verticalReject + 0.1, uprightness);
    if (verticalMask <= 1e-4) {
      return 1.0;
    }
    envHeightMask *= verticalMask;
  }

  float ndotl = saturate(abs(dot(normalDirWS, pixelToLightWS)));
  if (ndotl <= 0.001) {
    return 1.0;
  }

  float jitter = 0.0;
  if (useJitter) {
    float2 pixelCoord = floor(uv * vpSize_g.xy);
    uint frameIndex = (uint)max(sceneTime_g * 60.0, 0.0);
    jitter = renodx::rendering::InterleavedGradientNoiseTemporal(pixelCoord, frameIndex);
  }

  float3 viewForwardWS = float3(viewInv_g._m20, viewInv_g._m21, viewInv_g._m22);
  viewForwardWS = viewForwardWS * rsqrt(max(dot(viewForwardWS, viewForwardWS), 1e-8));
  float normalViewFactor = saturate(abs(dot(normalDirWS, viewForwardWS)));
  float surfaceStartBias = normalViewFactor * -0.00249999994 + 0.00749999983;
  float rayStepLength = normalViewFactor * 0.00150000001 + 0.000500000024;
  float3 rayStepWS = pixelToLightWS * rayStepLength;
  float3 rayStartWS = worldPos + normalDirWS * surfaceStartBias;

  float depthThickness = max(abs(ENV_SSS_FAR_DEPTH_VALUE - startDepth) * surfaceThickness, 1e-5);
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
    if (depthDelta > 0) {
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

  float maxDarkening = saturate(shader_injection_data.env_sss_max_darkening);
  result = max(result, 1.0 - maxDarkening);

  float validSampleFade = saturate((float)validSamples / (float)sampleCount);
  validSampleFade = validSampleFade * validSampleFade;
  float directionalStrength = ndotl * validSampleFade * envHeightMask;
  return lerp(1.0, result, directionalStrength);
}

static void ApplyEnvSSS(inout float3 color, float2 uv, uint2 mrt0_xy_raw, bool is_character_pixel)
{
  int debugMode = clamp((int)round(shader_injection_data.debug_show_env_sss), 0, 4);
  float3 envNormal = DecodeNormalFromMrt(mrt0_xy_raw);
  float rawShadow = ComputeEnvSunShadow(uv, envNormal);

  if (debugMode == 2) {
    color = is_character_pixel ? float3(1.0, 0.25, 0.25) : float3(0.2, 0.9, 0.2);
    return;
  }

  if (debugMode == 3) {
    color = envNormal * 0.5 + 0.5;
    return;
  }

  if (debugMode == 4) {
    color = rawShadow.xxx;
    return;
  }

  float brightness_reject = shader_injection_data.env_sss_bright_reject_threshold;
  float brightness_fade = max(shader_injection_data.env_sss_bright_reject_fade, 1e-5);
  float pixel_luma = dot(color, float3(0.2126, 0.7152, 0.0722));
  float bright_mask = smoothstep(brightness_reject, brightness_reject + brightness_fade, pixel_luma);
  float env_shadow = lerp(rawShadow, 1.0, bright_mask);

  if (debugMode == 1) {
    if (is_character_pixel) {
      return;
    }
    color = float3(0.5, 0.5, 0.5) * env_shadow * max(shader_injection_data.env_sss_strength, 0.0);
    return;
  }

  if (shader_injection_data.env_sss_enabled >= 0.5 && !is_character_pixel) {
    float strength = saturate(shader_injection_data.env_sss_strength);
    color = lerp(color, color * rawShadow, strength);
  }
}

#endif
