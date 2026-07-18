// ---- Created with 3Dmigoto v1.3.16 on Tue Sep 02 00:24:04 2025
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
  uint pointLightCount_g : packoffset(c41.w);
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
}

SamplerState samPoint_s : register(s0);
SamplerState samLinear_s : register(s1);
Texture2D<float4> colorTexture : register(t0);
Texture2D<float4> refractionTexture : register(t1);
Texture2D<uint4> mrtTexture : register(t2);


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

  mrtTexture.GetDimensions(0, fDest.x, fDest.y, fDest.z);
  r0.xy = fDest.xy;
  r0.xy = v1.xy * r0.xy;
  r0.xy = (int2)r0.xy;
  r0.zw = float2(0,0);
  r0.xyz = mrtTexture.Load(r0.xyz).xyw;
  r1.xyzw = colorTexture.SampleLevel(samPoint_s, v1.xy, 0).xyzw;
  o0 = r1;
  return;
  r0.z = (int)r0.z & 255;
  r0.z = (uint)r0.z;
  r0.w = cmp(0 >= r0.z);
  if (r0.w != 0) {
    o0.xyzw = r1.xyzw;
    return;
  }
  r0.z = 0.00392156886 * r0.z;
  r2.y = (uint)r0.x >> 16;
  r2.x = r0.x;
  r0.xw = (int2)r2.xy & int2(0xffff,0xffff);
  r0.xw = (uint2)r0.xw;
  r2.xy = float2(1.52590219e-005,1.52590219e-005) * r0.xw;
  r0.x = (int)r0.y & 0x0000ffff;
  r0.x = (uint)r0.x;
  r2.z = 1.52590219e-005 * r0.x;
  r0.xyw = float3(-0.5,-0.5,-0.5) + r2.xyz;
  r0.xyw = r0.xyw + r0.xyw;
  r2.x = -view_g._m20;
  r2.y = -view_g._m21;
  r2.z = -view_g._m22;
  r2.w = dot(r2.xyz, r0.xyw);
  r2.w = r2.w + r2.w;
  r0.xyw = r0.xyw * -r2.www + r2.xyz;
  r2.xy = view_g._m01_m11 * r0.yy;
  r0.xy = view_g._m00_m10 * r0.xx + r2.xy;
  r0.xy = view_g._m02_m12 * r0.ww + r0.xy;
  r0.xy = r0.xy * float2(0.00999999978,0.00999999978) + v1.xy;
  r0.xyw = refractionTexture.SampleLevel(samLinear_s, r0.xy, 0).xyz;
  r0.xyw = r0.xyw + -r1.xyz;
  o0.xyz = r0.zzz * r0.xyw + r1.xyz;
  o0.w = r1.w;

  return;
}