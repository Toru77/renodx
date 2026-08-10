// ---- Created with 3Dmigoto v1.3.16 on Fri Sep 05 02:28:43 2025
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


  struct
  {
    float4x4 m_split0Transform;
    float4x4 m_split1Transform;
    float4 m_splitDistances;
  } LightShadow0 : packoffset(c55);

  float3 LightDirForChar : packoffset(c64);
  float GameMaterialID : packoffset(c64.w) = {0};
  float4 GameMaterialDiffuse : packoffset(c65) = {1,1,1,1};
  float4 GameMaterialEmission : packoffset(c66) = {0,0,0,0};
  float GameMaterialMonotone : packoffset(c67) = {0};
  float4 GameMaterialTexcoord : packoffset(c68) = {0,0,1,1};
  float4 UVaMUvColor : packoffset(c69) = {1,1,1,1};
  float4 UVaProjTexcoord : packoffset(c70) = {0,0,1,1};
  float4 UVaMUvTexcoord : packoffset(c71) = {0,0,1,1};
  float4 UVaMUv2Texcoord : packoffset(c72) = {0,0,1,1};
  float4 UVaDuDvTexcoord : packoffset(c73) = {0,0,1,1};
  float AlphaThreshold : packoffset(c74) = {0.5};
  float SphereMapIntensity : packoffset(c74.y) = {1};
  float BloomIntensity : packoffset(c74.z) = {0.699999988};
  float4 GameEdgeParameters : packoffset(c75) = {1,1,1,0.00300000003};
  float4 PointLightParams : packoffset(c76) = {0,0,0,0};
  float4 PointLightColor : packoffset(c77) = {0,0,0,0};
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
  float4 r0,r1,r2;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.xyzw = DiffuseMapSampler.Sample(DiffuseMapSamplerSampler_s, v3.xy).xyzw;
  r1.x = r0.w * v1.w + -0.00400000019;
  r1.x = cmp(r1.x < 0);
  if (r1.x != 0) discard;
  r1.x = dot(v5.xyz, v5.xyz);
  r1.x = rsqrt(r1.x);
  r1.xyz = v5.xyz * r1.xxx;
  r2.x = dot(r1.xyz, scene.View._m00_m10_m20);
  r2.y = dot(r1.xyz, scene.View._m01_m11_m21);
  r1.x = dot(LightDirForChar.xyz, r1.xyz);
  r1.x = r1.x * 0.5 + 0.5;
  r1.x = r1.x * r1.x;
  r1.x = 0.550000012 * r1.x;
  r1.x = min(1.5, r1.x);
  r1.xyz = v1.xyz * r1.xxx;
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
  // r0.w = dot(r0.xyz, float3(0.298999995,0.587000012,0.114));
  r0.w = calculateLuminanceSRGB(r0.rgb);
  r1.xyz = r0.www * scene.MonotoneMul.xyz + scene.MonotoneAdd.xyz;
  r1.xyz = r1.xyz + -r0.xyz;
  o0.xyz = GameMaterialMonotone * r1.xyz + r0.xyz;

  o0.w = max(o0.w, 0.f);
  return;
}