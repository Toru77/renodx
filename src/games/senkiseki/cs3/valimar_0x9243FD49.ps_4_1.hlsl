// ---- Created with 3Dmigoto v1.3.16 on Thu Jul 03 18:18:04 2025
#include "../shared.h"
#include "../cs4/common.hlsl"
cbuffer _Globals : register(b0)
{
  uint4 DuranteSettings : packoffset(c0);

  struct
  {
    float3 EyePosition;
    float4x4 View;
    float4x4 Projection;
    float4x4 ViewProjection;
    float4x4 ViewInverse;
    float4x4 ProjectionInverse;
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
    float4 light1_attenuation;
    float3 light2_position;
    float3 light2_colorIntensity;
    float4 light2_attenuation;
  } scene : packoffset(c1);

  bool PhyreContextSwitches : packoffset(c43);
  bool PhyreMaterialSwitches : packoffset(c43.y);
  float4x4 World : packoffset(c44);
  float4x4 WorldViewProjection : packoffset(c48);
  float GlobalTexcoordFactor : packoffset(c52);
  float3 LightDirForChar : packoffset(c52.y);
  float GameMaterialID : packoffset(c53) = {0};
  float4 GameMaterialDiffuse : packoffset(c54) = {1,1,1,1};
  float4 GameMaterialEmission : packoffset(c55) = {0,0,0,0};
  float GameMaterialMonotone : packoffset(c56) = {0};
  float4 GameMaterialTexcoord : packoffset(c57) = {0,0,1,1};
  float4 UVaMUvColor : packoffset(c58) = {1,1,1,1};
  float4 UVaProjTexcoord : packoffset(c59) = {0,0,1,1};
  float4 UVaMUvTexcoord : packoffset(c60) = {0,0,1,1};
  float4 UVaMUv2Texcoord : packoffset(c61) = {0,0,1,1};
  float4 UVaDuDvTexcoord : packoffset(c62) = {0,0,1,1};
  float ShadowReceiveOffset : packoffset(c63) = {0.600000024};
  float SphereMapIntensity : packoffset(c63.y) = {1};
  float BloomIntensity : packoffset(c63.z) = {0.699999988};
  float GlareIntensity : packoffset(c63.w) = {1};
  float4 GameEdgeParameters : packoffset(c64) = {1,1,1,0.00300000003};
  float4 PointLightParams : packoffset(c65) = {0,0,0,0};
  float4 PointLightColor : packoffset(c66) = {0,0,0,0};
}

SamplerState LinearClampSamplerState_s : register(s0);
SamplerState DiffuseMapSamplerSampler_s : register(s1);
SamplerState DiffuseMap2SamplerSampler_s : register(s2);
SamplerState DiffuseMap3SamplerSampler_s : register(s3);
Texture2D<float4> DiffuseMapSampler : register(t0);
Texture2D<float4> DiffuseMap2Sampler : register(t1);
Texture2D<float4> DiffuseMap3Sampler : register(t2);
Texture2D<float4> SphereMapSampler : register(t3);


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_POSITION0,
  float4 v1 : COLOR0,
  float4 v2 : COLOR1,
  float2 v3 : TEXCOORD0,
  float2 w3 : TEXCOORD2,
  float4 v4 : TEXCOORD1,
  float4 v5 : TEXCOORD4,
  float4 v6 : TEXCOORD7,
  float4 v7 : TEXCOORD10,
  out float4 o0 : SV_TARGET0,
  out float4 o1 : SV_TARGET1,
  out float4 o2 : SV_TARGET2)
{
  float4 r0,r1,r2;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.xyz = DiffuseMapSampler.Sample(DiffuseMapSamplerSampler_s, v3.xy).xyz;
  r1.xyzw = DiffuseMap2Sampler.Sample(DiffuseMap2SamplerSampler_s, w3.xy).xyzw;
  r1.xyzw = UVaMUvColor.xyzw * r1.xyzw;
  r0.w = v1.w * r1.w;
  r0.xyz = r1.xyz * r0.www + r0.xyz;
  r1.xyzw = DiffuseMap3Sampler.Sample(DiffuseMap3SamplerSampler_s, v6.xy).xyzw;
  r0.w = v1.w * r1.w;
  r0.xyz = r1.xyz * r0.www + r0.xyz;
  r0.w = dot(v5.xyz, v5.xyz);
  r0.w = rsqrt(r0.w);
  r1.xyz = v5.xyz * r0.www;
  r2.x = dot(r1.xyz, scene.View._m00_m10_m20);
  r2.y = dot(r1.xyz, scene.View._m01_m11_m21);
  o1.xyz = r1.xyz * float3(0.5,0.5,0.5) + float3(0.5,0.5,0.5);
  r1.xy = r2.xy * float2(0.5,0.5) + float2(0.5,0.5);
  r1.xyz = SphereMapSampler.Sample(LinearClampSamplerState_s, r1.xy).xyz;
  r1.xyz = SphereMapIntensity * r1.xyz;
  r0.xyz = r1.xyz * v1.xyz + r0.xyz;
  r1.xyz = max(float3(1,1,1), scene.GlobalAmbientColor.xyz);
  // r1.xyz = min(float3(1.5,1.5,1.5), r1.xyz);
  r1.xyz = v1.xyz * r1.xyz;
  r0.xyz = r1.xyz * r0.xyz;
  r0.xyz = r0.xyz * GameMaterialDiffuse.xyz + GameMaterialEmission.xyz;
  r0.xyz = v2.xyz + r0.xyz;
  r1.xyz = scene.FogColor.xyz + -r0.xyz;
  r0.xyz = v2.www * r1.xyz + r0.xyz;
  // r0.w = dot(r0.xyz, float3(0.298999995,0.587000012,0.114));
  r0.w = calculateLuminanceSRGB(r0.xyz);
  r1.xyz = r0.www * scene.MonotoneMul.xyz + scene.MonotoneAdd.xyz;
  r1.xyz = r1.xyz + -r0.xyz;
  o0.xyz = GameMaterialMonotone * r1.xyz + r0.xyz;
  o0.w = min(1, GlareIntensity);
  o1.w = 0.75;
  r0.x = v7.z / v7.w;
  r0.y = 256 * r0.x;
  r1.x = trunc(r0.y);
  r0.x = r0.x * 256 + -r1.x;
  r0.y = 256 * r0.x;
  r1.y = trunc(r0.y);
  r1.z = r0.x * 256 + -r1.y;
  o2.xyz = float3(0.00390625,0.00390625,1) * r1.xyz;
  o2.w = 1;
  return;
}