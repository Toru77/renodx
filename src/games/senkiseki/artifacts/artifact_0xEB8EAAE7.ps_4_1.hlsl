// ---- Created with 3Dmigoto v1.3.16 on Thu Jul 03 17:38:59 2025
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
  float Shininess : packoffset(c61.y) = {0.5};
  float SpecularPower : packoffset(c61.z) = {50};
  float3 RimLitColor : packoffset(c62) = {1,1,1};
  float RimLitIntensity : packoffset(c62.w) = {4};
  float RimLitPower : packoffset(c63) = {2};
  float RimLightClampFactor : packoffset(c63.y) = {2};
  float SphereMapIntensity : packoffset(c63.z) = {1};
  float CubeMapIntensity : packoffset(c63.w) = {1};
  float CubeMapFresnel : packoffset(c64) = {0};
  float BloomIntensity : packoffset(c64.y) = {1};
  float4 PointLightParams : packoffset(c65) = {0,0,0,0};
  float4 PointLightColor : packoffset(c66) = {0,0,0,0};
}

SamplerState LinearClampSamplerState_s : register(s0);
SamplerState DiffuseMapSamplerSampler_s : register(s1);
Texture2D<float4> DiffuseMapSampler : register(t0);
Texture2D<float4> SphereMapSampler : register(t1);


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
  float4 r0,r1,r2,r3,r4;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.xyzw = DiffuseMapSampler.Sample(DiffuseMapSamplerSampler_s, v3.xy).xyzw;
  r1.x = r0.w * v1.w + -0.00400000019;
  r1.x = cmp(r1.x < 0);
  if (r1.x != 0) discard;
  r1.x = max(Light0.m_colorIntensity.x, Light0.m_colorIntensity.y);
  r1.y = max(0.00100000005, Light0.m_colorIntensity.z);
  r1.x = max(r1.x, r1.y);
  r1.xyz = Light0.m_colorIntensity.xyz / r1.xxx;
  r2.xyz = scene.EyePosition.xyz + -v4.xyz;
  r1.w = dot(r2.xyz, r2.xyz);
  r1.w = rsqrt(r1.w);
  r3.xyz = r2.xyz * r1.www + Light0.m_direction.xyz;
  r2.xyz = r2.xyz * r1.www;
  r1.w = dot(r3.xyz, r3.xyz);
  r1.w = rsqrt(r1.w);
  r3.xyz = r3.xyz * r1.www;
  r1.w = dot(v5.xyz, v5.xyz);
  r1.w = rsqrt(r1.w);
  r4.xyz = v5.xyz * r1.www;
  r1.w = saturate(dot(r4.xyz, r3.xyz));
  // r1.w = (dot(r4.xyz, r3.xyz));
  // r1.w = log2(r1.w);
  // r1.w = SpecularPower * r1.w;
  // r1.w = exp2(r1.w);
  r1.w = renodx::math::SafePow(r1.w, SpecularPower);
  r1.xyzw = min(float4(1.5,1.5,1.5,1), r1.xyzw);
  r1.w = Shininess * r1.w;
  r1.xyz = Light0.m_colorIntensity.xyz * r1.www + r1.xyz;
  r1.w = saturate(dot(r4.xyz, r2.xyz));
  r1.w = 1 + -r1.w;
  // r1.w = log2(r1.w);
  // r1.w = RimLitPower * r1.w;
  // r1.w = exp2(r1.w);
  r1.w = renodx::math::SafePow(r1.w, RimLitPower);
  r1.w = RimLitIntensity * r1.w;
  r2.xyz = RimLitColor.xyz * r1.www;
  r1.xyz = r2.xyz * Light0.m_colorIntensity.xyz + r1.xyz;
  r1.xyz = min(RimLightClampFactor, r1.xyz);
  r1.xyz = max(scene.GlobalAmbientColor.xyz, r1.xyz);
  r1.xyz = v1.xyz * r1.xyz;
  r2.x = dot(r4.xyz, scene.View._m00_m10_m20);
  r2.y = dot(r4.xyz, scene.View._m01_m11_m21);
  r2.xy = r2.xy * float2(0.5,0.5) + float2(0.5,0.5);
  r2.xyz = SphereMapSampler.Sample(LinearClampSamplerState_s, r2.xy).xyz;
  r2.xyz = SphereMapIntensity * r2.xyz;
  r0.xyz = r2.xyz * v1.xyz + r0.xyz;
  r0.w = v1.w * r0.w;
  r0.w = GameMaterialDiffuse.w * r0.w;
  o0.w = r0.w;
  r0.xyz = r0.xyz * r1.xyz;
  r0.xyz = r0.xyz * GameMaterialDiffuse.xyz + GameMaterialEmission.xyz;
  r0.xyz = v2.xyz + r0.xyz;
  r1.xyz = scene.FogColor.xyz + -r0.xyz;
  r0.xyz = v2.www * r1.xyz + r0.xyz;
  r0.w = calculateLuminanceSRGB(r0.rgb);
  r1.xyz = r0.www * scene.MonotoneMul.xyz + scene.MonotoneAdd.xyz;
  r1.xyz = r1.xyz + -r0.xyz;
  o0.xyz = GameMaterialMonotone * r1.xyz + r0.xyz;

  o0.rgb = clamp(o0.rgb, 0.f, shader_injection.safe_clamp);;
  return;
}