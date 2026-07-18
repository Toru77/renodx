// ---- Created with 3Dmigoto v1.3.16 on Mon Sep 29 00:42:31 2025
#include "../common.hlsl"
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
  float shadowSplitDistance_g : packoffset(c26.w);
  float4x4 shadowMtx_g[2] : packoffset(c27);
  float2 invShadowSize_g : packoffset(c35);
  float shadowFadeNear_g : packoffset(c35.z);
  float shadowFadeRangeInv_g : packoffset(c35.w);
  float3 sceneShadowColor_g : packoffset(c36);
  float gameTime_g : packoffset(c36.w);
  float3 windDirection_g : packoffset(c37);
  uint collisionCount_g : packoffset(c37.w);
  float lightTileWidthInv_g : packoffset(c38);
  float lightTileHeightInv_g : packoffset(c38.y);
  float fogNearDistance_g : packoffset(c38.z);
  float fogFadeRangeInv_g : packoffset(c38.w);
  float3 fogColor_g : packoffset(c39);
  float fogIntensity_g : packoffset(c39.w);
  float fogHeight_g : packoffset(c40);
  float fogHeightRangeInv_g : packoffset(c40.y);
  float windWaveTime_g : packoffset(c40.z);
  float windWaveFrequency_g : packoffset(c40.w);
  uint localLightProbeCount_g : packoffset(c41);
  float lightSpecularGlossiness_g : packoffset(c41.y);
  float lightSpecularIntensity_g : packoffset(c41.z);
  float lightTileDepthInv_g : packoffset(c41.w);
  float4x4 ditherMtx_g : packoffset(c42);
  float4 lightProbe_g[9] : packoffset(c46);
  float4x4 farShadowMtx_g : packoffset(c55);
  float3 chrLightDir_g : packoffset(c59);
  float shadowDistance_g : packoffset(c59.w);
  float resolutionScaling_g : packoffset(c60);
  float sceneTime_g : packoffset(c60.y);
  float windForce_g : packoffset(c60.z);
  float disableMapObjNearFade_g : packoffset(c60.w);
  float4 mapColor_g : packoffset(c61);
  float4 clipPlane_g : packoffset(c62);
  float shadowZeroCascadeUVMult_g : packoffset(c63);
  float prevSceneTime_g : packoffset(c63.y);
  float prevWindWaveTime_g : packoffset(c63.z);
  uint enableMotionVectors_g : packoffset(c63.w);
  float4x4 prevView_g : packoffset(c64);
  float4x4 prevViewInv_g : packoffset(c68);
  float4x4 prevProj_g : packoffset(c72);
  float4x4 prevProjInv_g : packoffset(c76);
  float4x4 prevViewProj_g : packoffset(c80);
  float4x4 prevViewProjInv_g : packoffset(c84);
  float2 motionJitterOffset_g : packoffset(c88);
  float2 curJitterOffset_g : packoffset(c88.z);
}

cbuffer cb_local : register(b2)
{
  float2 blurCenter_g : packoffset(c0);
  float2 blurScale_g : packoffset(c0.z);
  float zThreshold_g : packoffset(c1);
  float isFlip_g : packoffset(c1.y);
  float brightnessThreshold_g : packoffset(c1.z);
  float centricSharpness_g : packoffset(c1.w);
  float3 godrayColor_g : packoffset(c2);
}

SamplerState samLinear_s : register(s0);
Texture2D<float4> colorTexture : register(t0);
Texture2D<float4> depthTexture : register(t1);


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_Position0,
  float4 v1 : TEXCOORD0,
  out float4 o0 : SV_Target0)
{
  float4 r0,r1;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.x = depthTexture.SampleLevel(samLinear_s, v1.xy, 0).x;
  r0.x = saturate(r0.x);
  r0.y = 1;
  r0.z = dot(projInv_g._m22_m32, r0.xy);
  r0.x = dot(projInv_g._m23_m33, r0.xy);
  r0.x = r0.z / r0.x;
  r0.x = cmp(-r0.x < zThreshold_g);
  r0.x = r0.x ? 0 : 1;
  r0.yz = blurCenter_g.xy + float2(-0.5,-0.5);
  r0.y = dot(r0.yz, r0.yz);
  r0.y = sqrt(r0.y);
  r0.y = min(1, r0.y);
  r0.y = log2(r0.y);
  r0.y = centricSharpness_g * r0.y;
  r0.y = exp2(r0.y);
  r0.y = 1 + -r0.y;
  r1.xyzw = colorTexture.SampleLevel(samLinear_s, v1.xy, 0).xyzw;
  // r0.z = dot(r1.xyz, float3(0.298999995,0.587000012,0.114));
  r0.z = calculateLuminanceSRGB(r1.rgb);
  r0.z = -brightnessThreshold_g + r0.z;
  r0.z = max(0, r0.z);
  r1.xyz = r1.xyz * r0.zzz;
  o0.w = r1.w;
  r0.yzw = r1.xyz * r0.yyy;
  r0.xyz = r0.yzw * r0.xxx;
  r0.w = cmp(isFlip_g < 0);
  o0.xyz = r0.www ? float3(0,0,0) : r0.xyz;
  // this is necessary
  o0.rgb = saturate(o0.rgb);
  return;
}