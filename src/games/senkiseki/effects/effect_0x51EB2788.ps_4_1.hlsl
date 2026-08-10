// ---- Created with 3Dmigoto v1.3.16 on Wed Jul 09 03:34:35 2025
#include "../shared.h"
#include "../cs4/common.hlsl"
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
  float GameMaterialID : packoffset(c61.y) = {0};
  float4 GameMaterialDiffuse : packoffset(c62) = {1,1,1,1};
  float4 GameMaterialEmission : packoffset(c63) = {0,0,0,0};
  float GameMaterialMonotone : packoffset(c64) = {0};
  float4 GameMaterialTexcoord : packoffset(c65) = {0,0,1,1};
  float4 GameDitherParams : packoffset(c66) = {0,0,0,0};
  float4 UVaMUvColor : packoffset(c67) = {1,1,1,1};
  float4 UVaProjTexcoord : packoffset(c68) = {0,0,1,1};
  float4 UVaMUvTexcoord : packoffset(c69) = {0,0,1,1};
  float4 UVaMUv2Texcoord : packoffset(c70) = {0,0,1,1};
  float4 UVaDuDvTexcoord : packoffset(c71) = {0,0,1,1};
  float2 WindyGrassDirection : packoffset(c72) = {0,0};
  float WindyGrassSpeed : packoffset(c72.z) = {0.100000001};
  float WindyGrassHomogenity : packoffset(c72.w) = {2};
  float WindyGrassScale : packoffset(c73) = {1};
  float BloomIntensity : packoffset(c73.y) = {1};
  float MaskEps : packoffset(c73.z);
  float4 PointLightParams : packoffset(c74) = {0,2,1,1};
  float4 PointLightColor : packoffset(c75) = {1,0,0,0};
}

SamplerState PointWrapSamplerState_s : register(s0);
SamplerState DiffuseMapSamplerSampler_s : register(s1);
Texture2D<float4> DitherNoiseTexture : register(t0);
Texture2D<float4> DiffuseMapSampler : register(t1);


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_POSITION0,
  float4 v1 : COLOR0,
  float4 v2 : COLOR1,
  float4 v3 : TEXCOORD0,
  float4 v4 : TEXCOORD1,
  float4 v5 : TEXCOORD4,
  float4 v6 : TEXCOORD9,
  out float4 o0 : SV_TARGET0)
{
  float4 r0,r1,r2;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.xy = float2(0.25,0.25) * v0.xy;
  r0.x = DitherNoiseTexture.SampleLevel(PointWrapSamplerState_s, r0.xy, 0).x;
  r0.x = v6.x + -r0.x;
  r0.x = cmp(r0.x < 0);
  if (r0.x != 0) discard;
  r0.xyz = -scene.UserClipPlane2.xyz + v4.xyz;
  r0.w = dot(r0.xyz, r0.xyz);
  r0.w = rsqrt(r0.w);
  r0.xyz = r0.xyz * r0.www;
  r0.x = dot(scene.UserClipPlane.xyz, r0.xyz);
  r0.x = cmp(r0.x < 0);
  if (r0.x != 0) discard;
  r0.xyz = scene.EyePosition.xyz + -v4.xyz;
  r0.w = dot(r0.xyz, r0.xyz);
  r0.w = rsqrt(r0.w);
  r0.xyz = r0.xyz * r0.www;
  r0.w = dot(v5.xyz, v5.xyz);
  r0.w = rsqrt(r0.w);
  r1.xyz = v5.xyz * r0.www;
  r0.x = saturate(dot(r1.xyz, r0.xyz));
  // r0.x = (dot(r1.xyz, r0.xyz));
  r0.x = 1 + -r0.x;
  // r0.x = log2(r0.x);
  // r0.x = PointLightColor.x * r0.x;
  // r0.x = exp2(r0.x);
  r0.x = renodx::math::SafePow(r0.x, PointLightColor.x);
  r0.x = -1 + r0.x;
  r0.x = PointLightColor.y * r0.x + 1;
  r0.xyz = GameMaterialEmission.xyz * r0.xxx;
  r1.xyz = max(float3(1,1,1), scene.GlobalAmbientColor.xyz);
  // r1.xyz = min(float3(1.5,1.5,1.5), r1.xyz);
  r1.xyz = v1.xyz * r1.xyz;
  r2.xyz = DiffuseMapSampler.Sample(DiffuseMapSamplerSampler_s, v3.xy).xyz;
  r1.xyz = r2.xyz * r1.xyz;
  r0.xyz = r1.xyz * GameMaterialDiffuse.xyz + r0.xyz;
  // r0.w = dot(r0.xyz, float3(0.298999995,0.587000012,0.114));
  r0.w = calculateLuminanceSRGB(r0.xyz);
  r1.xyz = r0.www * scene.MonotoneMul.xyz + scene.MonotoneAdd.xyz;
  r1.xyz = r1.xyz + -r0.xyz;
  r0.xyz = GameMaterialMonotone * r1.xyz + r0.xyz;
  r1.xyz = BloomIntensity * r0.xyz;
  o0.xyz = r0.xyz;
  // r0.x = dot(r1.xyz, float3(0.298999995,0.587000012,0.114));
  r0.x = calculateLuminanceSRGB(r1.xyz);
  r0.x = -scene.MiscParameters2.z + r0.x;
  r0.x = max(0, r0.x);
  r0.x = 0.5 * r0.x;
  r0.x = min(1, r0.x);
  o0.w = PointLightParams.z * r0.x;

  o0.w = max(o0.w, 0.f);
  return;
}