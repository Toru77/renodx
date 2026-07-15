#ifndef SRC_GAMES_FALCOMENGINE_PLUS_REFERENCE_LOCAL_SSS_HLSL_
#define SRC_GAMES_FALCOMENGINE_PLUS_REFERENCE_LOCAL_SSS_HLSL_

#include "rendering.hlsl"

static const float LOCAL_SSS_FAR_DEPTH_VALUE = 0.0;

// ── Local Screen Space Shadow (Bend_SSS for point/spot lights) ──
// Traces from worldPos toward lightPos, comparing depth along the ray.
// Returns visibility 0–1 (1 = fully lit, 0 = fully shadowed).
static float ComputeLocalShadow(
    float2 uv, float3 worldPos, float3 normalWS,
    float3 lightPos, float lightRadius)
{
  int sampleCount = max((int)round(shader_injection_data.local_sss_sample_count), 1);
  int hardShadowSamples = shader_injection_data.local_sss_hard_shadow_samples > 0.5f
      ? (int)shader_injection_data.local_sss_hard_shadow_samples
      : max(sampleCount / 8, 1);
  int fadeOutSamples = shader_injection_data.local_sss_fade_out_samples > 0.5f
      ? (int)shader_injection_data.local_sss_fade_out_samples
      : max(sampleCount / 3, 1);
  if ((hardShadowSamples + fadeOutSamples) > sampleCount) {
    fadeOutSamples = max(sampleCount - hardShadowSamples, 0);
  }

  float surfaceThickness = max(shader_injection_data.local_sss_surface_thickness, 1e-5);
  float shadowContrast = max(shader_injection_data.local_sss_contrast, 0.0);

  // ── Light direction and distance (cheap, no texture read) ──
  float3 toLightWS = lightPos - worldPos;
  float lightDist = length(toLightWS);
  float lightDistInv = 1.0 / max(lightDist, 1e-5);
  float3 pixelToLightWS = toLightWS * lightDistInv;
  float distNorm = lightDist / max(lightRadius, 1e-5);

  // ── Light distance fade (skip SSS for far lights) ──
  float lightFadeStart = shader_injection_data.local_sss_light_fade_start;
  float lightFadeEnd = max(shader_injection_data.local_sss_light_fade_end, lightFadeStart + 1e-5);
  float lightFade = 1.0 - smoothstep(lightFadeStart, lightFadeEnd, distNorm);
  if (lightFade <= 1e-4) return 1.0;

  // ── Normal validity ──
  float normalLenSq = dot(normalWS, normalWS);
  if (normalLenSq <= 1e-8) return 1.0;
  float3 normalDirWS = normalWS * rsqrt(normalLenSq);

  // ── NdotL facing check ──
  float ndotl = saturate(abs(dot(normalDirWS, pixelToLightWS)));
  if (ndotl <= 0.001) return 1.0;

  // ── Depth validity (texture read — deferred until after cheap early-outs) ──
  float startDepth = depthTexture.SampleLevel(samPoint_s, uv, 0).x;
  if (startDepth <= LOCAL_SSS_FAR_DEPTH_VALUE) return 1.0;

  // ── Occluder depth scale ──
  float occluderDepthScale = max(shader_injection_data.local_sss_occluder_depth_scale, 0.0);

  // ── Surface start bias ──
  float3 viewForwardWS = float3(viewInv_g._m20, viewInv_g._m21, viewInv_g._m22);
  viewForwardWS = viewForwardWS * rsqrt(max(dot(viewForwardWS, viewForwardWS), 1e-8));
  float normalViewFactor = saturate(abs(dot(normalDirWS, viewForwardWS)));
  float surfaceStartBias = normalViewFactor * -0.00249999994 + 0.00749999983;

  // ── Ray step: scale with distance — further lights get coarser steps ──
  float rayStepLength = (normalViewFactor * 0.00150000001 + 0.000500000024)
      * min(1.0, lightRadius * lightDistInv);
  float3 rayStepWS = pixelToLightWS * rayStepLength;
  float3 rayStartWS = worldPos + normalDirWS * surfaceStartBias;

  // ── Occluder depth bias ──
  float depthThickness = max(abs(LOCAL_SSS_FAR_DEPTH_VALUE - startDepth) * surfaceThickness, 1e-5);
  if (occluderDepthScale > 0.001) {
    depthThickness = max(depthThickness, occluderDepthScale * rayStepLength);
  }

  float hardShadow = 1.0;
  float4 shadowValue = 1.0;
  int validSamples = 0;

  // ── Max ray distance: capped at light distance (squared to avoid sqrt per sample) ──
  float maxRayDistSq = lightDist * lightDist * 0.98;

  [loop]
  for (int i = 0; i < sampleCount; ++i) {
    float sampleIndex = (float)(i + 1);
    float3 samplePosWS = rayStartWS + rayStepWS * sampleIndex;

    // Early out if we've passed the light (squared distance, no sqrt)
    float3 sampleDelta = samplePosWS - worldPos;
    if (dot(sampleDelta, sampleDelta) > maxRayDistSq) {
      validSamples = max(validSamples, 1);
      break;
    }

    float4 samplePosWS4 = float4(samplePosWS, 1.0);

    float4 sampleClip;
    sampleClip.x = dot(samplePosWS4, viewProj_g._m00_m10_m20_m30);
    sampleClip.y = dot(samplePosWS4, viewProj_g._m01_m11_m21_m31);
    sampleClip.z = dot(samplePosWS4, viewProj_g._m02_m12_m22_m32);
    sampleClip.w = dot(samplePosWS4, viewProj_g._m03_m13_m23_m33);
    if (abs(sampleClip.w) <= 1e-6) break;

    float3 sampleNdc = sampleClip.xyz / sampleClip.www;
    float2 sampleUV = sampleNdc.xy * float2(0.5, -0.5) + float2(0.5, 0.5);
    if (sampleUV.x <= invVPSize_g.x || sampleUV.y <= invVPSize_g.y ||
        sampleUV.x >= (1.0 - invVPSize_g.x) || sampleUV.y >= (1.0 - invVPSize_g.y)) {
      break;
    }

    validSamples++;

    float sampleDepth = depthTexture.SampleLevel(samPoint_s, sampleUV, 0).x;
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

  float validSampleFade = saturate((float)validSamples / (float)sampleCount);
  validSampleFade = validSampleFade * validSampleFade;
  float directionalStrength = ndotl * validSampleFade * lightFade;
  return lerp(1.0, result, directionalStrength);
}

#endif
