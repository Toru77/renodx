// ---- Created with 3Dmigoto v1.3.16 on Sat Jun 07 01:12:54 2025
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
  float3 ShadowColorShift : packoffset(c74.y) = {0.100000001,0.0199999996,0.0199999996};
  float3 RimLitColor : packoffset(c75) = {1,1,1};
  float RimLitIntensity : packoffset(c75.w) = {4};
  float RimLitPower : packoffset(c76) = {2};
  float RimLightClampFactor : packoffset(c76.y) = {2};
  float2 WindyGrassDirection : packoffset(c76.z) = {0,0};
  float WindyGrassSpeed : packoffset(c77) = {0.100000001};
  float WindyGrassHomogenity : packoffset(c77.y) = {2};
  float WindyGrassScale : packoffset(c77.z) = {1};
  float BloomIntensity : packoffset(c77.w) = {1};
  float MaskEps : packoffset(c78);
  float4 PointLightParams : packoffset(c79) = {0,2,1,1};
  float4 PointLightColor : packoffset(c80) = {1,0,0,0};
}

SamplerState DiffuseMapSamplerSampler_s : register(s0);
SamplerState NormalMapSamplerSampler_s : register(s1);
Texture2D<float4> DiffuseMapSampler : register(t0);
Texture2D<float4> NormalMapSampler : register(t1);


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_POSITION0,
  float4 v1 : COLOR0,
  float4 v2 : COLOR1,
  float4 v3 : TEXCOORD0,
  float4 v4 : TEXCOORD1,
  float4 v5 : TEXCOORD4,
  float3 v6 : TEXCOORD6,
  out float4 o0 : SV_TARGET0)
{
  float4 r0,r1,r2,r3,r4;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.xyzw = DiffuseMapSampler.Sample(DiffuseMapSamplerSampler_s, v3.xy).xyzw;
  r1.x = r0.w * v1.w + -0.00400000019;
  r1.x = cmp(r1.x < 0);
  if (r1.x != 0) discard;
  r1.x = dot(v6.xyz, v6.xyz);
  r1.x = rsqrt(r1.x);
  r1.xyz = v6.xyz * r1.xxx;
  r1.w = dot(v5.xyz, v5.xyz);
  r1.w = rsqrt(r1.w);
  r2.xyz = v5.xyz * r1.www;
  r3.xyz = r2.zxy * r1.yzx;
  r3.xyz = r2.yzx * r1.zxy + -r3.xyz;
  r4.xyz = NormalMapSampler.Sample(NormalMapSamplerSampler_s, v3.xy).xyz;
  r4.xyz = r4.xyz * float3(2,2,2) + float3(-1,-1,-1);
  r3.xyz = r4.yyy * r3.xyz;
  r1.w = cmp(v3.x < 0);
  r1.w = r1.w ? -1 : 1;
  r1.w = r4.x * r1.w;
  r1.xyz = r1.www * r1.xyz + r3.xyz;
  r1.xyz = r4.zzz * r2.xyz + r1.xyz;
  r1.w = dot(r1.xyz, r1.xyz);
  r1.w = rsqrt(r1.w);
  r1.xyz = r1.xyz * r1.www;
  r2.xyz = scene.EyePosition.xyz + -v4.xyz;
  r1.w = dot(r2.xyz, r2.xyz);
  r1.w = rsqrt(r1.w);
  r2.xyz = r2.xyz * r1.www;
  r1.x = dot(r1.xyz, r2.xyz);
  r1.y = cmp(r1.x < 0);
  r1.x = r1.y ? -r1.x : r1.x;
  r1.x = 1 + -abs(r1.x);
  r1.x = max(0, r1.x);
  float l = r1.x;
  // r1.x = log2(r1.x);
  // r1.y = RimLitPower * r1.x;
  // r1.x = PointLightColor.x * r1.x;
  // r1.x = exp2(r1.x);
  r1.x = renodx::math::SafePow(l, PointLightColor.x);
  r1.x = -1 + r1.x;
  r1.x = PointLightColor.y * r1.x + 1;
  // r1.y = exp2(r1.y);
  r1.y = renodx::math::SafePow(l, RimLitPower);
  r1.y = -r1.y * RimLitIntensity + 1;
  r0.w = v1.w * r0.w;
  r2.w = r0.w * r1.y;
  r0.w = max(Light0.m_colorIntensity.x, Light0.m_colorIntensity.y);
  r1.y = max(0.00100000005, Light0.m_colorIntensity.z);
  r0.w = max(r1.y, r0.w);
  r1.yzw = Light0.m_colorIntensity.xyz / r0.www;
  // r1.yzw = min(float3(1.5,1.5,1.5), r1.yzw);
  r3.xyz = min(float3(1,1,1), r1.yzw);

  r3.xyz = float3(1,1,1) + -r3.xyz;
  r3.xyz = ShadowColorShift.xyz * r3.xyz;
  r4.xyz = Light0.m_colorIntensity.xyz + Light0.m_colorIntensity.xyz;
  r4.xyz = max(float3(1,1,1), r4.xyz);
  r1.yzw = r3.xyz * r4.xyz + r1.yzw;
  r1.yzw = v1.xyz * r1.yzw;
  r2.xyz = r1.yzw * r0.xyz;
  r0.xyzw = GameMaterialDiffuse.xyzw * r2.xyzw;
  r0.xyz = GameMaterialEmission.xyz * r1.xxx + r0.xyz;

  o0.w = r0.w;
  // r0.w = dot(r0.xyz, float3(0.298999995, 0.587000012, 0.114));
  r0.w = calculateLuminanceSRGB(r0.xyz);
  r1.xyz = r0.www * scene.MonotoneMul.xyz + scene.MonotoneAdd.xyz;
  r1.xyz = r1.xyz + -r0.xyz;
  o0.xyz = GameMaterialMonotone * r1.xyz + r0.xyz;

  o0.w = max(o0.w, 0); 
  return;
}