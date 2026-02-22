// ---- Created with 3Dmigoto v1.3.16 on Wed Sep 17 21:28:19 2025

cbuffer cb_ysx_scene : register(b0)
{
  float3 lightColor_g : packoffset(c0);
  float deltaTime_g : packoffset(c0.w);
  float3 lightDirection_g : packoffset(c1);
  uint isManaSensing_g : packoffset(c1.w);
  float4x4 farShadowMtx_g : packoffset(c2);
  float2 invFarShadowSize_g : packoffset(c6);
  float2 invShadowSize_g : packoffset(c6.z);
  float3 sceneShadowColor_g : packoffset(c7);
  float shadowFadeNear_g : packoffset(c7.w);
  float3 chrLightDir_g : packoffset(c8);
  float shadowFadeRangeInv_g : packoffset(c8.w);
  float shadowDistance_g : packoffset(c9);
  float farShadowStartDistance_g : packoffset(c9.y);
  float farShadowEndDistance_g : packoffset(c9.z);
  uint shadowSamplingMode_g : packoffset(c9.w);
  float4x4 ditherMtx_g : packoffset(c10);
  float sceneTime_g : packoffset(c14.y);
  float lightSpecularGlossiness_g : packoffset(c14.z);
  float lightSpecularIntensity_g : packoffset(c14.w);
  float2 resolutionScaling_g : packoffset(c15);
  float disableNearCameraAlpha_g : packoffset(c15.z);
  float sceneDeltaTime_g : packoffset(c15.w);
  float3 twoLayeredFogColorLowerNear_g : packoffset(c16);
  uint twoLayeredFogMode_g : packoffset(c16.w);
  float3 twoLayeredFogColorLowerFar_g : packoffset(c17);
  float twoLayeredFogStartDistance_g : packoffset(c17.w);
  float3 twoLayeredFogColorUpperNear_g : packoffset(c18);
  float twoLayeredFogDistanceRangeInv_g : packoffset(c18.w);
  float3 twoLayeredFogColorUpperFar_g : packoffset(c19);
  float2 twoLayeredFogHeightNear_g : packoffset(c20);
  float2 twoLayeredFogHeightFar_g : packoffset(c20.z);
  float2 twoLayeredFogMinIntensity_g : packoffset(c21);
  float2 twoLayeredFogMaxIntensity_g : packoffset(c21.z);
  float2 twoLayeredFogBlend_g : packoffset(c22);
  float2 twoLayeredFogDistanceCoefInv_g : packoffset(c22.z);
  float3 windDirection_g : packoffset(c23);
  float windWaveTime_g : packoffset(c23.w);
  float windWaveFrequency_g : packoffset(c24);
  float windForce_g : packoffset(c24.y);
  float seaWaveLengthScale_g : packoffset(c24.z);
  float seaWaveHeight_g : packoffset(c24.w);
  float3 seaWaveDirection_g : packoffset(c25);
  float seaWaveSpeed_g : packoffset(c25.w);
  float disableFarCameraAlpha_g : packoffset(c26);
  uint localLightProbeCount_g : packoffset(c26.y);
  float2 invVPSize_g : packoffset(c26.z);
  float4 lightProbe_g[9] : packoffset(c27);
  float3 lightTileSizeInv_g : packoffset(c36);
  float4x4 waterCausticsProj_g : packoffset(c37);
  float2 invResolutionScaling_g : packoffset(c41);
  float2 resolutionUVClamp_g : packoffset(c41.z);
  float3 chara_shadow_mul_color_g : packoffset(c42);
  float4x4 shadow_matrices_g[3] : packoffset(c43);
  float3 shadow_split_distance_g : packoffset(c55);
  uint dbgWaterCausticsOff_g : packoffset(c57.y);
}

SamplerState samDefault_s : register(s0);
Texture2D<float4> colorMap : register(t0);


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_Position0,
  float4 v1 : TEXCOORD0,
  out float4 o0 : SV_Target0)
{
  float4 r0;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.xy = min(resolutionUVClamp_g.xy, v1.xy);
  o0.xyzw = colorMap.SampleLevel(samDefault_s, r0.xy, 0).xyzw;
  return;
}