// ---- Created with 3Dmigoto v1.3.16 on Thu Aug 21 16:18:31 2025
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
}

cbuffer cb_godray : register(b2)
{
  float2 blurCenter_g : packoffset(c0);
  float2 blurScale_g : packoffset(c0.z);
  float zThreshold_g : packoffset(c1);
  float isFlip_g : packoffset(c1.y);
  float brightnessThreshold_g : packoffset(c1.z);
  float centricSharpness_g : packoffset(c1.w);
  float3 godrayColor_g : packoffset(c2);
  float4 uvClamp_g : packoffset(c3);
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

  r0.xy = blurCenter_g.xy + -v1.xy;
  r0.x = dot(r0.xy, r0.xy);
  r0.x = sqrt(r0.x);
  r0.x = min(1, r0.x);
  r0.x = 1 + -r0.x;
  r0.x = log2(r0.x);
  r0.x = centricSharpness_g * r0.x;
  r0.x = exp2(r0.x);
  // r0.x = renodx::math::SafePow(r0.x, centricSharpness_g);
  r0.yz = blurCenter_g.xy + float2(-0.5,-0.5);
  r0.y = dot(r0.yz, r0.yz);
  r0.y = sqrt(r0.y);
  r0.y = min(1, r0.y);
  r0.y = 1 + -r0.y;
  r0.x = r0.y * r0.x;
  r1.xyzw = colorTexture.SampleLevel(samLinear_s, v1.xy, 0).xyzw;
  // r1.rgb = srgbDecode(r1.xyz);
  // r0.y = dot(r1.xyz, float3(0.298999995,0.587000012,0.114));
  r0.y = calculateLuminanceSRGB(r1.rgb);
  r0.y = -brightnessThreshold_g + r0.y;
  r0.y = max(0, r0.y);
  r0.yzw = r1.xyz * r0.yyy;
  o0.w = r1.w;
  r0.xyz = r0.yzw * r0.xxx;
  r1.x = depthTexture.SampleLevel(samLinear_s, v1.xy, 0).x;
  r1.y = 1;
  r0.w = dot(projInv_g._m22_m32, r1.xy);
  r1.x = dot(projInv_g._m23_m33, r1.xy);
  r0.w = r0.w / r1.x;
  r0.w = cmp(-r0.w < zThreshold_g);
  r0.w = r0.w ? 0 : 1;
  r0.xyz = r0.xyz * r0.www;
  r0.w = cmp(isFlip_g < 0);
  o0.xyz = r0.www ? float3(0,0,0) : r0.xyz;

  // o0.rgb = srgbEncode(o0.rgb);
  return;
}