// ---- Created with 3Dmigoto v1.4.1 on Thu Aug  6 13:26:28 2026

#include "../../shared.h"

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


  struct
  {
    float4x4 m_split0Transform;
    float4x4 m_split1Transform;
    float4 m_splitDistances;
  } LightShadow0 : packoffset(c51);

  float PerMaterialMainLightClampFactor : packoffset(c60) = {1.5};
  float3 LightDirForChar : packoffset(c60.y);
  float GameMaterialID : packoffset(c61) = {0};
  float4 GameMaterialDiffuse : packoffset(c62) = {1,1,1,1};
  float4 GameMaterialEmission : packoffset(c63) = {0,0,0,0};
  float GameMaterialMonotone : packoffset(c64) = {0};
  float4 GameMaterialTexcoord : packoffset(c65) = {0,0,1,1};
  float4 UVaMUvColor : packoffset(c66) = {1,1,1,1};
  float4 UVaProjTexcoord : packoffset(c67) = {0,0,1,1};
  float4 UVaMUvTexcoord : packoffset(c68) = {0,0,1,1};
  float4 UVaMUv2Texcoord : packoffset(c69) = {0,0,1,1};
  float4 UVaDuDvTexcoord : packoffset(c70) = {0,0,1,1};
  float3 ShadowColorShift : packoffset(c71) = {0.100000001,0.0199999996,0.0199999996};
  float Shininess : packoffset(c71.w) = {0.5};
  float SpecularPower : packoffset(c72) = {50};
  float3 SpecularColor : packoffset(c72.y) = {1,1,1};
  float3 RimLitColor : packoffset(c73) = {1,1,1};
  float RimLitIntensity : packoffset(c73.w) = {4};
  float RimLitPower : packoffset(c74) = {2};
  float RimLightClampFactor : packoffset(c74.y) = {2};
  float ShadowReceiveOffset : packoffset(c74.z) = {0.600000024};
  float BloomIntensity : packoffset(c74.w) = {0.699999988};
  float4 GameEdgeParameters : packoffset(c75) = {1,1,1,0.00300000003};
  float4 OutlineColorFactor : packoffset(c76) = {1,1,1,1};
  float4 PointLightParams : packoffset(c77) = {0,0,0,0};
  float4 PointLightColor : packoffset(c78) = {0,0,0,0};
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
  float4 v5 : TEXCOORD9,
  float4 v6 : TEXCOORD10,
  float4 v7 : TEXCOORD5,  // DLAA: prevClip from replaced char VS
  out float4 o0 : SV_TARGET0,
  out float4 o1 : SV_TARGET1,
  out float4 o2 : SV_TARGET2,
  out float4 o3 : SV_TARGET3)  // DLAA: appended per-object motion target
{
  float4 r0,r1;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.xyzw = DiffuseMapSampler.Sample(DiffuseMapSamplerSampler_s, v3.xy).xyzw;
  r0.xyzw = v1.xyzw * r0.xyzw;
  r0.xyz = r0.xyz * GameMaterialDiffuse.xyz + GameMaterialEmission.xyz;
  r0.w = GameMaterialDiffuse.w * r0.w;
  o0.w = r0.w;
  r1.xyz = scene.FogColor.xyz + -r0.xyz;
  r0.xyz = v2.www * r1.xyz + r0.xyz;
  r0.w = dot(r0.xyz, float3(0.298999995,0.587000012,0.114));
  r1.xyz = r0.www * scene.MonotoneMul.xyz + scene.MonotoneAdd.xyz;
  r1.xyz = r1.xyz + -r0.xyz;
  o0.xyz = GameMaterialMonotone * r1.xyz + r0.xyz;
  r0.x = dot(v5.xyz, v5.xyz);
  r0.x = rsqrt(r0.x);
  r0.xyz = v5.xyz * r0.xxx;
  o1.xyz = r0.xyz * float3(0.5,0.5,0.5) + float3(0.5,0.5,0.5);
  o1.w = 0;
  r0.x = v6.z / v6.w;
  r0.y = 256 * r0.x;
  r1.x = trunc(r0.y);
  r0.x = r0.x * 256 + -r1.x;
  r0.y = 256 * r0.x;
  r1.y = trunc(r0.y);
  r1.z = r0.x * 256 + -r1.y;
  o2.xyz = float3(0.00390625,0.00390625,1) * r1.xyz;
  o2.w = 1;

  // DLAA: per-object motion (prevClip from replaced char VS TEXCOORD5).
  // Encodes prevNDC (0..1, y-up) into the appended 16-bit RTV's xy with w=1 as
  // the valid flag; o0/o1/o2 (the game's MRTs) are left exactly as the original.
  if (DLAA_PER_OBJECT_MOTION > 0.5f) {
    float2 prevNDC = v7.xy / max(v7.w, 0.001);
    o3 = float4(prevNDC.x * 0.5 + 0.5, prevNDC.y * 0.5 + 0.5, 0, 1.0);
  } else {
    o3 = float4(0, 0, 0, 0);
  }
  return;
}