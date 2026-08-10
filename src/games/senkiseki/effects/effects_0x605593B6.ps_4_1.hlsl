// ---- Created with 3Dmigoto v1.3.16 on Mon Jul 14 23:43:52 2025
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
  float AlphaThreshold : packoffset(c72) = {0.5};
  float BloomIntensity : packoffset(c72.y) = {1};
  float MaskEps : packoffset(c72.z);
  float4 PointLightParams : packoffset(c73) = {0,2,1,1};
  float4 PointLightColor : packoffset(c74) = {1,0,0,0};
}

SamplerState DiffuseMapSamplerSampler_s : register(s0);
Texture2D<float4> DiffuseMapSampler : register(t0);


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_POSITION0,
  float4 v1 : COLOR0,
  float4 v2 : COLOR1,
  float4 v3 : TEXCOORD0,
  float4 v4 : TEXCOORD1,
  float3 v5 : TEXCOORD4,
  out float4 o0 : SV_TARGET0)
{
  float4 r0,r1,r2;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.xyzw = DiffuseMapSampler.Sample(DiffuseMapSamplerSampler_s, v3.xy).xyzw;
  r1.x = r0.w * v1.w + -0.00400000019;
  r1.x = cmp(r1.x < 0);
  if (r1.x != 0) discard;
  r1.xyz = max(float3(1,1,1), scene.GlobalAmbientColor.xyz);
  // r1.xyz = min(float3(1.5,1.5,1.5), r1.xyz);
  r1.xyz = v1.xyz * r1.xyz;
  r1.xyz = r1.xyz * r0.xyz;
  r1.w = v1.w * r0.w;
  r0.xyzw = GameMaterialDiffuse.xyzw * r1.xyzw;
  r1.xyz = scene.EyePosition.xyz + -v4.xyz;
  r1.w = dot(r1.xyz, r1.xyz);
  r1.w = rsqrt(r1.w);
  r1.xyz = r1.xyz * r1.www;
  r1.w = dot(v5.xyz, v5.xyz);
  r1.w = rsqrt(r1.w);
  r2.xyz = v5.xyz * r1.www;
  r1.x = dot(r2.xyz, r1.xyz);
  r1.x = 1 + -abs(r1.x);
  r1.x = max(0, r1.x);
  // r1.x = log2(r1.x);
  // r1.x = PointLightColor.x * r1.x;
  // r1.x = exp2(r1.x);
  r1.x = renodx::math::SafePow(r1.x, PointLightColor.x);
  r1.x = -1 + r1.x;
  r1.x = PointLightColor.y * r1.x + 1;
  r0.xyz = GameMaterialEmission.xyz * r1.xxx + r0.xyz;
  o0.w = r0.w;
  r0.w = calculateLuminanceSRGB(r0.rgb);
  r1.xyz = r0.www * scene.MonotoneMul.xyz + scene.MonotoneAdd.xyz;
  r1.xyz = r1.xyz + -r0.xyz;
  o0.xyz = GameMaterialMonotone * r1.xyz + r0.xyz;
  return;
}