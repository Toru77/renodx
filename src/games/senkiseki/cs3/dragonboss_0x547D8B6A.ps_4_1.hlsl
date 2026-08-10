// ---- Created with 3Dmigoto v1.3.16 on Sun Aug 31 21:33:37 2025
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
  float GlobalTexcoordFactor : packoffset(c48);

  struct
  {
    float3 m_direction;
    float3 m_colorIntensity;
  } Light0 : packoffset(c49);

  float GameMaterialID : packoffset(c50.w) = {0};
  float4 GameMaterialDiffuse : packoffset(c51) = {1,1,1,1};
  float4 GameMaterialEmission : packoffset(c52) = {0,0,0,0};
  float GameMaterialMonotone : packoffset(c53) = {0};
  float4 GameMaterialTexcoord : packoffset(c54) = {0,0,1,1};
  float4 UVaMUvColor : packoffset(c55) = {1,1,1,1};
  float4 UVaProjTexcoord : packoffset(c56) = {0,0,1,1};
  float4 UVaMUvTexcoord : packoffset(c57) = {0,0,1,1};
  float4 UVaMUv2Texcoord : packoffset(c58) = {0,0,1,1};
  float4 UVaDuDvTexcoord : packoffset(c59) = {0,0,1,1};
  float AlphaThreshold : packoffset(c60) = {0.5};
  float3 RimLitColor : packoffset(c60.y) = {1,1,1};
  float RimLitIntensity : packoffset(c61) = {4};
  float RimLitPower : packoffset(c61.y) = {2};
  float RimLightClampFactor : packoffset(c61.z) = {2};
  float BloomIntensity : packoffset(c61.w) = {0.699999988};
  float4 GameEdgeParameters : packoffset(c62) = {1,1,1,0.00300000003};
  float4 PointLightParams : packoffset(c63) = {0,0,0,0};
  float4 PointLightColor : packoffset(c64) = {0,0,0,0};
}

SamplerState DiffuseMapSamplerSampler_s : register(s0);
SamplerState DiffuseMap2SamplerSampler_s : register(s1);
SamplerState DiffuseMap3SamplerSampler_s : register(s2);
Texture2D<float4> DiffuseMapSampler : register(t0);
Texture2D<float4> DiffuseMap2Sampler : register(t1);
Texture2D<float4> DiffuseMap3Sampler : register(t2);


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
  float2 v6 : TEXCOORD7,
  out float4 o0 : SV_TARGET0)
{
  float4 r0,r1;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.xyzw = DiffuseMapSampler.Sample(DiffuseMapSamplerSampler_s, v3.xy).xyzw;
  r1.x = r0.w * v1.w + -0.00400000019;
  r1.x = cmp(r1.x < 0);
  if (r1.x != 0) discard;
  r1.xyzw = DiffuseMap2Sampler.Sample(DiffuseMap2SamplerSampler_s, w3.xy).xyzw;
  r1.w = UVaMUvColor.w * r1.w;
  r1.xyz = r1.xyz * UVaMUvColor.xyz + -r0.xyz;
  r1.w = v1.w * r1.w;
  r0.xyz = r1.www * r1.xyz + r0.xyz;
  r0.w = v1.w * r0.w;
  r1.xyzw = DiffuseMap3Sampler.Sample(DiffuseMap3SamplerSampler_s, v6.xy).xyzw;
  r1.w = v1.w * r1.w;
  r0.xyz = r1.xyz * r1.www + r0.xyz;
  r1.x = max(Light0.m_colorIntensity.x, Light0.m_colorIntensity.y);
  r1.y = max(0.00100000005, Light0.m_colorIntensity.z);
  r1.x = max(r1.x, r1.y);
  r1.xyz = Light0.m_colorIntensity.xyz / r1.xxx;
  // r1.xyz = min(float3(1.5,1.5,1.5), r1.xyz);
  r1.xyz = v1.xyz * r1.xyz;
  r0.xyz = r1.xyz * r0.xyz;
  r0.xyz = r0.xyz * GameMaterialDiffuse.xyz + GameMaterialEmission.xyz;
  r0.xyz = v2.xyz + r0.xyz;
  // r1.x = dot(r0.xyz, float3(0.298999995,0.587000012,0.114));
  r1.x = calculateLuminanceSRGB(r0.rgb);
  r1.xyz = r1.xxx * scene.MonotoneMul.xyz + scene.MonotoneAdd.xyz;
  r1.xyz = r1.xyz + -r0.xyz;
  o0.xyz = GameMaterialMonotone * r1.xyz + r0.xyz;
  r0.xyz = scene.EyePosition.xyz + -v4.xyz;
  r1.x = dot(r0.xyz, r0.xyz);
  r1.x = rsqrt(r1.x);
  r0.xyz = r1.xxx * r0.xyz;
  r1.x = dot(v5.xyz, v5.xyz);
  r1.x = rsqrt(r1.x);
  r1.xyz = v5.xyz * r1.xxx;
  r0.x = dot(r1.xyz, r0.xyz);
  r0.y = cmp(r0.x < 0);
  r0.x = r0.y ? -r0.x : r0.x;
  r0.x = 1 + -abs(r0.x);
  r0.x = max(0, r0.x);
  r0.x = log2(r0.x);
  r0.x = RimLitPower * r0.x;
  r0.x = exp2(r0.x);
  r0.x = -r0.x * RimLitIntensity + 1;
  r0.x = r0.w * r0.x;
  r0.x = GameMaterialDiffuse.w * r0.x;
  o0.w = r0.x;

  o0.w = max(0.f, o0.w);
  return;
}