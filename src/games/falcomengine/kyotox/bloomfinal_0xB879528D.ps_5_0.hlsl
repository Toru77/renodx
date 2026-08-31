#include "../common.hlsl"

// ---- Created with 3Dmigoto v1.4.1 on Sat Jul 18 23:45:29 2026

cbuffer SceneConstantData : register(b0)
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
  float fogChrIntensity_g : packoffset(c26.w);
  float3 chrLightDir_g : packoffset(c27);
  float padding_g : packoffset(c27.w);
  float3 sceneShadowColor_g : packoffset(c28);
  float disableMapObjNearFade_g : packoffset(c28.w);
  float3 windDirection_g : packoffset(c29);
  float windForce_g : packoffset(c29.w);
  float windWaveTime_g : packoffset(c30);
  float windWaveFrequency_g : packoffset(c30.y);
  float lightTileWidthInv_g : packoffset(c30.z);
  float lightTileHeightInv_g : packoffset(c30.w);
  float3 fogColor_g : packoffset(c31);
  float fogIntensity_g : packoffset(c31.w);
  float fogHeight_g : packoffset(c32);
  float fogHeightRangeInv_g : packoffset(c32.y);
  float fogNearDistance_g : packoffset(c32.z);
  float fogFadeRangeInv_g : packoffset(c32.w);
  uint localLightProbeCount_g : packoffset(c33);
  float lightSpecularGlossiness_g : packoffset(c33.y);
  float lightSpecularIntensity_g : packoffset(c33.z);
  float lightTileDepthInv_g : packoffset(c33.w);
  float4x4 ditherMtx_g : packoffset(c34);
  float4 lightProbe_g[9] : packoffset(c38);
  float2 resolutionScaling_g : packoffset(c47);
  float sceneTime_g : packoffset(c47.z);
  float gameTime_g : packoffset(c47.w);
  float4 mapColor_g : packoffset(c48);
  float4 clipPlane_g : packoffset(c49);
  float3 debugData_g : packoffset(c50);
  uint debugFlag_g : packoffset(c50.w);
  float4x4 shadowMtx_g[3] : packoffset(c51);
  float2 shadowSplitDistance_g : packoffset(c63);
  float shadowFadeNear_g : packoffset(c63.z);
  float shadowFadeRangeInv_g : packoffset(c63.w);
  float2 invShadowSize_g : packoffset(c64);
  float2 cameraNearFar_g : packoffset(c64.z);
  float4 frustumPlanes_g[6] : packoffset(c65);
  float4 chrSilhouetteColor_g : packoffset(c71);
  float4x4 prevViewProj_g : packoffset(c72);
  float2 jitterDiff_g : packoffset(c76);
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
  float pad : packoffset(c2.w);
  float2 uv_clamp_g : packoffset(c3);
  float2 pad2 : packoffset(c3.z);
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

  r0.xy = blurCenter_g.xy + float2(-0.5,-0.5);
  r0.x = dot(r0.xy, r0.xy);
  r0.x = sqrt(r0.x);
  r0.x = min(1, r0.x);
  r0.x = log2(r0.x);
  r0.x = centricSharpness_g * r0.x;
  r0.x = exp2(r0.x);
  r0.x = 1 + -r0.x;
  r0.yz = min(uv_clamp_g.xy, v1.xy);
  r1.xyzw = colorTexture.SampleLevel(samLinear_s, r0.yz, 0).xyzw;
  r0.y = depthTexture.SampleLevel(samLinear_s, r0.yz, 0).x;
  //r0.w = dot(r1.xyz, float3(0.298999995,0.587000012,0.114));
  r0.w = calculateLuminanceSRGB(r1.rgb);
  r0.y = -brightnessThreshold_g + r0.y;
  r0.w = max(0, r0.w);
  r1.xyz = r1.xyz * r0.www;
  o0.w = r1.w;
  r1.xyz = r1.xyz * r0.xxx;
  r0.z = 1;
  r0.x = dot(projInv_g._m22_m32, r0.yz);
  r0.y = dot(projInv_g._m23_m33, r0.yz);
  r0.x = r0.x / r0.y;
  r0.x = cmp(-r0.x < zThreshold_g);
  r0.x = r0.x ? 0 : 1;
  r0.xyz = r1.xyz * r0.xxx;
  r0.w = cmp(isFlip_g < 0);
  o0.xyz = r0.www ? float3(0,0,0) : r0.xyz;
  return;
}