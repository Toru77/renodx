// ---- Created with 3Dmigoto v1.3.16 on Tue Jul 01 21:25:05 2025
#include "../shared.h"
#include "./common.hlsl"
cbuffer _Globals : register(b0)
{

  struct
  {
    float3 EyePosition;
    float3 EyeDirection;
    float4x4 View;
    float4x4 Projection;
    float4x4 ViewProjection;
    float4x4 ViewInverse;
    float4x4 ProjectionInverse;
    float4x4 ViewProjectionPrev;
    float2 cameraNearFar;
    float cameraNearTimesFar;
    float cameraFarMinusNear;
    float cameraFarMinusNearInv;
    float2 ViewportWidthHeight;
    float2 screenWidthHeightInv;
    float3 GlobalAmbientColor;
    float Time;
    float3 FakeRimLightDir;
    float3 FogColor;
    float4 FogRangeParameters;
    float3 MiscParameters1;
    float4 MiscParameters2;
    float3 MonotoneMul;
    float3 MonotoneAdd;
    float3 UserClipPlane2;
    float4 UserClipPlane;
    float4 MiscParameters3;
    float AdditionalShadowOffset;
    float AlphaTestDirection;
    float4 MiscParameters4;
    float3 MiscParameters5;
    float4 MiscParameters6;
    float3 MiscParameters7;
    float3 light2_colorIntensity;
    float4 light2_attenuation;
    uint4 DuranteSettings;
  } scene : packoffset(c0);

  bool PhyreContextSwitches : packoffset(c48);
  bool PhyreMaterialSwitches : packoffset(c48.y);
  float4x4 World : packoffset(c49);
  float4x4 WorldViewProjection : packoffset(c53);
  float4x4 WorldViewProjectionPrev : packoffset(c57);
  float GlobalTexcoordFactor : packoffset(c61);

  struct
  {
    float3 m_direction;
    float3 m_colorIntensity;
  } Light0 : packoffset(c62);

  float GameMaterialID : packoffset(c63.w) = {0};
  float4 GameMaterialDiffuse : packoffset(c64) = {1,1,1,1};
  float4 GameMaterialEmission : packoffset(c65) = {0,0,0,0};
  float GameMaterialMonotone : packoffset(c66) = {0};
  float4 GameMaterialTexcoord : packoffset(c67) = {0,0,1,1};
  float4 GameDitherParams : packoffset(c68) = {0,0,0,0};
  float4 UVaMUvColor : packoffset(c69) = {1,1,1,1};
  float4 UVaProjTexcoord : packoffset(c70) = {0,0,1,1};
  float4 UVaMUvTexcoord : packoffset(c71) = {0,0,1,1};
  float4 UVaMUv2Texcoord : packoffset(c72) = {0,0,1,1};
  float4 UVaDuDvTexcoord : packoffset(c73) = {0,0,1,1};
  float AlphaThreshold : packoffset(c74) = {0.5};
  float3 RimLitColor : packoffset(c74.y) = {1,1,1};
  float RimLitIntensity : packoffset(c75) = {4};
  float RimLitPower : packoffset(c75.y) = {2};
  float RimLightClampFactor : packoffset(c75.z) = {2};
  float2 DuDvMapImageSize : packoffset(c76) = {256,256};
  float2 DuDvScale : packoffset(c76.z) = {4,4};
  float BloomIntensity : packoffset(c77) = {1};
  float MaskEps : packoffset(c77.y);
  float4 PointLightParams : packoffset(c78) = {0,2,1,1};
  float4 PointLightColor : packoffset(c79) = {1,0,0,0};
}

SamplerState LinearClampSamplerState_s : register(s0);
SamplerState DiffuseMapSamplerSampler_s : register(s1);
SamplerState DuDvMapSamplerSampler_s : register(s2);
Texture2D<float4> DiffuseMapSampler : register(t0);
Texture2D<float4> DuDvMapSampler : register(t1);
Texture2D<float4> RefractionTexture : register(t2);


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_POSITION0,
  float4 v1 : COLOR0,
  float4 v2 : COLOR1,
  float2 v3 : TEXCOORD0,
  float2 w3 : TEXCOORD5,
  float4 v4 : TEXCOORD1,
  float3 v5 : TEXCOORD4,
  out float4 o0 : SV_TARGET0)
{
  float4 r0,r1,r2,r3;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.xy = DuDvMapSampler.Sample(DuDvMapSamplerSampler_s, w3.xy).xy;
  r0.xy = r0.xy * float2(2,2) + float2(-1,-1);
  r0.zw = DuDvScale.xy / DuDvMapImageSize.xy;
  r1.xy = r0.xy * r0.zw + v3.xy;
  r0.xy = r0.xy * r0.zw;
  r1.xyzw = DiffuseMapSampler.Sample(DiffuseMapSamplerSampler_s, r1.xy).xyzw;
  r0.z = r1.w * v1.w + -0.00400000019;
  r0.z = cmp(r0.z < 0);
  if (r0.z != 0) discard;
  r0.zw = scene.ViewportWidthHeight.xy / DuDvMapImageSize.xy;
  r2.xy = v0.xy / scene.ViewportWidthHeight.xy;
  r2.zw = v0.ww * r2.xy;
  r0.xy = r0.xy * r0.zw + r2.zw;
  r0.zw = r2.xy * v0.ww + -r0.xy;
  r0.xy = scene.AdditionalShadowOffset * r0.zw + r0.xy;
  r0.xy = r0.xy / v0.ww;
  r0.xyz = RefractionTexture.Sample(LinearClampSamplerState_s, r0.xy).xyz;
  r1.xyz = r1.xyz + -r0.xyz;
  r0.w = v1.w * r1.w;
  r2.xyz = scene.EyePosition.xyz + -v4.xyz;
  r1.w = dot(r2.xyz, r2.xyz);
  r1.w = rsqrt(r1.w);
  r2.xyz = r2.xyz * r1.www;
  r1.w = dot(v5.xyz, v5.xyz);
  r1.w = rsqrt(r1.w);
  r3.xyz = v5.xyz * r1.www;
  r1.w = saturate(dot(r3.xyz, r2.xyz));
  // r1.w = (dot(r3.xyz, r2.xyz));
  r1.w = 1 + -r1.w;

  float l = r1.w;
  // r1.w = log2(r1.w);
  // r2.x = RimLitPower * r1.w;
  // r1.w = PointLightColor.x * r1.w;
  // r1.w = exp2(r1.w);
  r1.w = renodx::math::SafePow(l, PointLightColor.x);
  r1.w = -1 + r1.w;
  r1.w = PointLightColor.y * r1.w + 1;
  // r2.x = exp2(r2.x);
  r2.x = renodx::math::SafePow(l, RimLitPower);
  r2.x = -r2.x * RimLitIntensity + 1;
  r2.w = r2.x * r0.w;
  r0.xyz = r2.www * r1.xyz + r0.xyz;
  r0.w = max(Light0.m_colorIntensity.x, Light0.m_colorIntensity.y);
  r1.x = max(0.00100000005, Light0.m_colorIntensity.z);
  r0.w = max(r1.x, r0.w);
  r1.xyz = Light0.m_colorIntensity.xyz / r0.www;
  // r1.xyz = min(float3(1.5,1.5,1.5), r1.xyz);
  r1.xyz = v1.xyz * r1.xyz;
  r2.xyz = r1.xyz * r0.xyz;
  r0.xyzw = GameMaterialDiffuse.xyzw * r2.xyzw;
  r0.xyz = GameMaterialEmission.xyz * r1.www + r0.xyz;
  o0.w = r0.w;
  // r0.w = dot(r0.xyz, float3(0.298999995,0.587000012,0.114));
  r0.w = calculateLuminanceSRGB(r0.xyz);
  r1.xyz = r0.www * scene.MonotoneMul.xyz + scene.MonotoneAdd.xyz;
  r1.xyz = r1.xyz + -r0.xyz;
  o0.xyz = GameMaterialMonotone * r1.xyz + r0.xyz;

  o0.w = max(o0.w, 0.f);
  return;
}