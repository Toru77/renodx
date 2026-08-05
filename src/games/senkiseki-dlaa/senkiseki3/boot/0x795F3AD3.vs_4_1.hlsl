// ---- Created with 3Dmigoto v1.4.1 on Wed Aug  5 18:08:12 2026

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
  float GameMaterialID : packoffset(c48.y) = {0};
  float4 GameMaterialDiffuse : packoffset(c49) = {1,1,1,1};
  float4 GameMaterialEmission : packoffset(c50) = {0,0,0,0};
  float GameMaterialMonotone : packoffset(c51) = {0};
  float4 GameMaterialTexcoord : packoffset(c52) = {0,0,1,1};
  float4 GameDitherParams : packoffset(c53) = {0,0,0,0};
  float4 UVaMUvColor : packoffset(c54) = {1,1,1,1};
  float4 UVaProjTexcoord : packoffset(c55) = {0,0,1,1};
  float4 UVaMUvTexcoord : packoffset(c56) = {0,0,1,1};
  float4 UVaMUv2Texcoord : packoffset(c57) = {0,0,1,1};
  float4 UVaDuDvTexcoord : packoffset(c58) = {0,0,1,1};
  float AlphaThreshold : packoffset(c59) = {0.5};
  float Shininess : packoffset(c59.y) = {0.5};
  float SpecularPower : packoffset(c59.z) = {50};
  float BloomIntensity : packoffset(c59.w) = {1};
  float4 PointLightParams : packoffset(c60) = {0,0,0,0};
  float4 PointLightColor : packoffset(c61) = {0,0,0,0};
}



// 3Dmigoto declarations
#define cmp -


void main(
  float3 v0 : POSITION0,
  float3 v1 : NORMAL0,
  float2 v2 : TEXCOORD0,
  float4 v3 : COLOR0,
  float2 v4 : TEXCOORD3,
  out float4 o0 : SV_POSITION0,
  out float4 o1 : COLOR0,
  out float4 o2 : COLOR1,
  out float2 o3 : TEXCOORD0,
  out float2 p3 : TEXCOORD2,
  out float4 o4 : TEXCOORD1,
  out float3 o5 : TEXCOORD4)
{
  float4 r0,r1;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.w = 1;
  r1.xyz = v0.xyz;
  r1.w = 1;
  r0.x = dot(r1.xyzw, World._m00_m10_m20_m30);
  r0.y = dot(r1.xyzw, World._m01_m11_m21_m31);
  r0.z = dot(r1.xyzw, World._m02_m12_m22_m32);
  o0.x = dot(r0.xyzw, scene.ViewProjection._m00_m10_m20_m30);
  o0.y = dot(r0.xyzw, scene.ViewProjection._m01_m11_m21_m31);
  o0.z = dot(r0.xyzw, scene.ViewProjection._m02_m12_m22_m32);
  o0.w = dot(r0.xyzw, scene.ViewProjection._m03_m13_m23_m33);
  // DLAA: rasterization-level camera jitter (SV_Position sub-pixel shift).
  o0.x += DLAA_JITTER_X * o0.w;
  o0.y += DLAA_JITTER_Y * o0.w;
  r0.w = dot(r0.xyzw, scene.View._m02_m12_m22_m32);
  o4.w = -r0.w;
  o1.xyzw = min(float4(1,1,1,1), v3.xyzw);
  r1.xyz = scene.EyePosition.xyz + -r0.xyz;
  o4.xyz = r0.xyz;
  r0.x = dot(r1.xyz, r1.xyz);
  r0.x = rsqrt(r0.x);
  r0.xyz = r1.xyz * r0.xxx;
  r1.x = dot(v1.xyz, World._m00_m10_m20);
  r1.y = dot(v1.xyz, World._m01_m11_m21);
  r1.z = dot(v1.xyz, World._m02_m12_m22);
  r0.w = dot(r1.xyz, r1.xyz);
  r0.w = rsqrt(r0.w);
  r1.xyz = r1.xyz * r0.www;
  r0.x = dot(r1.xyz, r0.xyz);
  o5.xyz = r1.xyz;
  r0.x = max(0, r0.x);
  r0.x = 1 + -r0.x;
  r0.x = max(0, r0.x);
  r0.x = log2(r0.x);
  r0.y = max(1, PointLightColor.w);
  r0.x = r0.y * r0.x;
  r0.x = exp2(r0.x);
  o2.xyz = PointLightColor.xyz * r0.xxx;
  o2.w = 0;
  o3.xy = v2.xy * GameMaterialTexcoord.zw + GameMaterialTexcoord.xy;
  p3.xy = v4.xy * UVaMUvTexcoord.zw + UVaMUvTexcoord.xy;
  return;
}
