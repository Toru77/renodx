// ---- Created with 3Dmigoto v1.3.16 on Tue Aug 26 00:09:30 2025
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

  struct
  {
    float3 m_direction;
    float3 m_colorIntensity;
  } Light0 : packoffset(c53);

  float GameMaterialID : packoffset(c54.w) = {0};
  float4 GameMaterialDiffuse : packoffset(c55) = {1,1,1,1};
  float4 GameMaterialEmission : packoffset(c56) = {0,0,0,0};
  float GameMaterialMonotone : packoffset(c57) = {0};
  float4 GameMaterialTexcoord : packoffset(c58) = {0,0,1,1};
  float4 GameDitherParams : packoffset(c59) = {0,0,0,0};
  float4 UVaMUvColor : packoffset(c60) = {1,1,1,1};
  float4 UVaProjTexcoord : packoffset(c61) = {0,0,1,1};
  float4 UVaMUvTexcoord : packoffset(c62) = {0,0,1,1};
  float4 UVaMUv2Texcoord : packoffset(c63) = {0,0,1,1};
  float4 UVaDuDvTexcoord : packoffset(c64) = {0,0,1,1};
  float BloomIntensity : packoffset(c65) = {1};
  float GlareIntensity : packoffset(c65.y) = {1};
  float4 PointLightParams : packoffset(c66) = {0,0,0,0};
  float4 PointLightColor : packoffset(c67) = {0,0,0,0};
}

SamplerState DiffuseMapSamplerSampler_s : register(s0);
SamplerState GlareMapSamplerSampler_s : register(s1);
Texture2D<float4> DiffuseMapSampler : register(t0);
Texture2D<float4> GlareMapSampler : register(t1);


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_POSITION0,
  float4 v1 : COLOR0,
  float4 v2 : COLOR1,
  float4 v3 : TEXCOORD0,
  float4 v4 : TEXCOORD1,
  float4 v5 : TEXCOORD4,
  float4 v6 : TEXCOORD10,
  out float4 o0 : SV_TARGET0,
  out float4 o1 : SV_TARGET1,
  out float4 o2 : SV_TARGET2)
{
  float4 r0,r1;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.x = max(Light0.m_colorIntensity.x, Light0.m_colorIntensity.y);
  r0.y = max(0.00100000005, Light0.m_colorIntensity.z);
  r0.x = max(r0.x, r0.y);
  r0.xyz = Light0.m_colorIntensity.xyz / r0.xxx;
  // // r0.xyz = min(float3(1.5,1.5,1.5), r0.xyz);
  r0.xyz = max(scene.GlobalAmbientColor.xyz, r0.xyz);
  r0.xyz = v1.xyz * r0.xyz;
  r1.xyz = DiffuseMapSampler.Sample(DiffuseMapSamplerSampler_s, v3.xy).xyz;
  r0.xyz = r1.xyz * r0.xyz;
  r0.xyz = r0.xyz * GameMaterialDiffuse.xyz + GameMaterialEmission.xyz;
  r0.xyz = v2.xyz + r0.xyz;
  // r0.w = dot(r0.xyz, float3(0.298999995,0.587000012,0.114));
  r0.w = calculateLuminanceSRGB(r0.rgb);
  r1.xyz = r0.www * scene.MonotoneMul.xyz + scene.MonotoneAdd.xyz;
  // r1.xyz = r1.xyz + -r0.xyz;
  // r0.xyz = GameMaterialMonotone * r1.xyz + r0.xyz;
  r0.rgb = lerp(r0.rgb, r1.rgb, GameMaterialMonotone);
  r1.xyz = BloomIntensity * r0.xyz;
  o0.xyz = r0.xyz;
  // r0.x = dot(r1.xyz, float3(0.298999995,0.587000012,0.114));
  r0.x = calculateLuminanceSRGB(r1.rgb);
  r0.x = -scene.MiscParameters2.z + r0.x;
  r0.x = max(0, r0.x);
  r0.x = 0.5 * r0.x;
  r0.x = min(1, r0.x);
  r0.y = GlareMapSampler.Sample(GlareMapSamplerSampler_s, v3.xy).x;
  r0.z = GlareIntensity * r0.y;
  r0.z = min(1, r0.z);
  r0.z = r0.z + -r0.x;
  o0.w = r0.y * r0.z + r0.x;
  r0.x = dot(v5.xyz, v5.xyz);
  r0.x = rsqrt(r0.x);
  r0.xyz = v5.xyz * r0.xxx;
  o1.xyz = r0.xyz * float3(0.5,0.5,0.5) + float3(0.5,0.5,0.5);
  o1.w = 0.75;
  r0.x = v6.z / v6.w;
  r0.y = 256 * r0.x;
  r1.x = trunc(r0.y);
  r0.x = r0.x * 256 + -r1.x;
  r0.y = 256 * r0.x;
  r1.y = trunc(r0.y);
  r1.z = r0.x * 256 + -r1.y;
  o2.xyz = float3(0.00390625,0.00390625,1) * r1.xyz;
  o2.w = 0;
  return;
}