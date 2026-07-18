// ---- Created with 3Dmigoto v1.3.16 on Sat Aug 23 09:40:57 2025
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

cbuffer cb_glow : register(b2)
{
  float4 uv_clamp0_g : packoffset(c0);
  float4 uv_clamp1_g : packoffset(c1);
  float2 uv_clamp2_g : packoffset(c2);
  float2 intensityLum_g : packoffset(c2.z);
  float2 chrIntensityLum_g : packoffset(c3);
  float atmosphereFadeBegin_g : packoffset(c3.z);
  float atmosphereFadeRangeInv_g : packoffset(c3.w);
  float atmosphereIntensity_g : packoffset(c4);
}

SamplerState samLinear_s : register(s0);
Texture2D<float4> colorTexture : register(t0);
Texture2D<uint4> mrtTexture0 : register(t1);
Texture2D<uint4> mrtTexture1 : register(t2);
Texture2D<float4> depthTexture : register(t3);


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_Position0,
  float4 v1 : TEXCOORD0,
  out float4 o0 : SV_Target0)
{
  float4 r0,r1,r2;
  uint4 bitmask, uiDest;
  float4 fDest;

  mrtTexture1.GetDimensions(0, fDest.x, fDest.y, fDest.z);
  r0.xy = fDest.xy;
  r0.zw = float2(0.25,0.25) / r0.xy;
  r0.zw = v1.xy + r0.zw;
  r0.xy = r0.zw * r0.xy;
  r0.xy = (int2)r0.xy;
  r0.zw = float2(0,0);
  r0.x = mrtTexture1.Load(r0.xyz).y;
  r0.x = (uint)r0.x;
  r0.y = 0.0392156877 * r0.x;
  r0.x = cmp(0 >= r0.x);
  mrtTexture0.GetDimensions(0, fDest.x, fDest.y, fDest.z);
  r0.zw = fDest.xy;
  r1.xy = float2(0.25,0.25) / r0.zw;
  r1.xy = v1.xy + r1.xy;
  r0.zw = r1.xy * r0.zw;
  r1.xy = (int2)r0.zw;
  r1.zw = float2(0,0);
  r0.z = mrtTexture0.Load(r1.xyz).w;
  r0.z = (int)r0.z & 1;
  r0.zw = r0.zz ? chrIntensityLum_g.xy : intensityLum_g.xy;
  r1.xyzw = colorTexture.SampleLevel(samLinear_s, v1.xy, 0).xyzw;
  // r1.rgb = srgbDecode(r1.rgb);
  r2.xyz = r1.xyz * r0.zzz;
  r2.xyz = r0.xxx ? r2.xyz : r1.xyz;
  r0.xyz = r2.xyz * r0.yyy;
  r2.xyz = r2.xyz + -r0.www;
  r0.xyz = max(r2.xyz, r0.xyz);
  r2.x = depthTexture.SampleLevel(samLinear_s, v1.xy, 0).x;
  r2.y = 1;
  r0.w = dot(projInv_g._m22_m32, r2.xy);
  r2.x = dot(projInv_g._m23_m33, r2.xy);
  r0.w = r0.w / r2.x;
  r0.w = -atmosphereFadeBegin_g + -r0.w;
  r0.w = saturate(atmosphereFadeRangeInv_g * r0.w);
  r1.xyz = r1.xyz * r0.www;
  o0.w = r1.w;
  r1.xyz = atmosphereIntensity_g * r1.xyz;
  o0.xyz = max(r1.xyz, r0.xyz);

  // o0.rgb = srgbEncode(o0.rgb);
  return;
}