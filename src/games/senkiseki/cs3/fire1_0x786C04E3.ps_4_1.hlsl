// ---- Created with 3Dmigoto v1.3.16 on Wed Sep 03 04:44:34 2025
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
  float4 GameDitherParams : packoffset(c55) = {0,0,0,0};
  float4 UVaMUvColor : packoffset(c56) = {1,1,1,1};
  float4 UVaProjTexcoord : packoffset(c57) = {0,0,1,1};
  float4 UVaMUvTexcoord : packoffset(c58) = {0,0,1,1};
  float4 UVaMUv2Texcoord : packoffset(c59) = {0,0,1,1};
  float4 UVaDuDvTexcoord : packoffset(c60) = {0,0,1,1};
  float AlphaThreshold : packoffset(c61) = {0.5};
  float2 DuDvMapImageSize : packoffset(c61.y) = {256,256};
  float2 DuDvScale : packoffset(c62) = {4,4};
  float BloomIntensity : packoffset(c62.z) = {1};
  float4 PointLightParams : packoffset(c63) = {0,0,0,0};
  float4 PointLightColor : packoffset(c64) = {0,0,0,0};
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
  float4 r0,r1;
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
  r0.xy = r0.xy * r0.zw;
  r0.zw = v0.xy / scene.ViewportWidthHeight.xy;
  r0.xy = r0.zw * v0.ww + r0.xy;
  r0.xy = r0.xy / v0.ww;
  r0.xyz = RefractionTexture.Sample(LinearClampSamplerState_s, r0.xy).xyz;
  r1.xyz = r1.xyz + -r0.xyz;
  r0.w = v1.w * r1.w;
  r0.xyz = r0.www * r1.xyz + r0.xyz;
  r0.w = GameMaterialDiffuse.w * r0.w;
  o0.w = r0.w;
  r0.w = max(Light0.m_colorIntensity.x, Light0.m_colorIntensity.y);
  r1.x = max(0.00100000005, Light0.m_colorIntensity.z);
  r0.w = max(r1.x, r0.w);
  r1.xyz = Light0.m_colorIntensity.xyz / r0.www;
  // // r1.xyz = min(float3(1.5,1.5,1.5), r1.xyz);
  r1.xyz = v1.xyz * r1.xyz;
  r0.xyz = r1.xyz * r0.xyz;
  r0.xyz = r0.xyz * GameMaterialDiffuse.xyz + GameMaterialEmission.xyz;
  r0.xyz = v2.xyz + r0.xyz;
  r0.xyz = v2.www * -r0.xyz + r0.xyz;
  // r0.w = dot(r0.xyz, float3(0.298999995,0.587000012,0.114));
  r0.w = calculateLuminanceSRGB(r0.rgb);
  r1.xyz = r0.www * scene.MonotoneMul.xyz + scene.MonotoneAdd.xyz;
  r1.xyz = r1.xyz + -r0.xyz;
  o0.xyz = GameMaterialMonotone * r1.xyz + r0.xyz;

  o0.w = max(o0.w, 0.f);
  return;
}