// ── Modified unskinned scene vertex shader for DLAA ──
// Based on: scene_unskinned_0x09394015.vs_4_1.hlsl
// Additions: camera jitter and per-object motion (see character VS for full comments)
//
// SPDX-License-Identifier: MIT

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
  float4 PointLightParams : packoffset(c53) = {0,0,0,0};
  float4 PointLightColor : packoffset(c54) = {0,0,0,0};

  float4x4 prevViewProjection : packoffset(c74);
}


void main(
  float3 v0 : POSITION0,
  float3 v1 : NORMAL0,
  float2 v2 : TEXCOORD0,
  out float4 o0 : SV_POSITION0,
  out float4 o1 : COLOR0,
  out float4 o2 : COLOR1,
  out float4 o3 : TEXCOORD0,
  out float4 o4 : TEXCOORD1,
  out float4 o5 : TEXCOORD4,
  out float4 o6 : TEXCOORD10,
  out float4 o7 : TEXCOORD5)
{
  float4 r0,r1,r2;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.w = 1;
  r1.xyz = v0.xyz;
  r1.w = 1;
  r0.x = dot(r1.xyzw, World._m00_m10_m20_m30);
  r0.z = dot(r1.xyzw, World._m02_m12_m22_m32);
  r0.y = dot(r1.xyzw, World._m01_m11_m21_m31);
  r1.x = dot(r0.xyzw, scene.ViewProjection._m00_m10_m20_m30);
  r1.y = dot(r0.xyzw, scene.ViewProjection._m01_m11_m21_m31);
  r1.z = dot(r0.xyzw, scene.ViewProjection._m02_m12_m22_m32);
  r1.w = dot(r0.xyzw, scene.ViewProjection._m03_m13_m23_m33);

  // DLAA: Jitter
  float4 jitteredPos = r1;
  if (DLAA_JITTER_ENABLED > 0.5f) {
    jitteredPos.x += DLAA_JITTER_X * jitteredPos.w;
    jitteredPos.y += DLAA_JITTER_Y * jitteredPos.w;
  }
  o0.xyzw = jitteredPos;

  // DLAA: Per-object motion
  if (DLAA_PER_OBJECT_MOTION > 0.5f) {
    o7.x = dot(r0.xyzw, prevViewProjection._m00_m10_m20_m30);
    o7.y = dot(r0.xyzw, prevViewProjection._m01_m11_m21_m31);
    o7.z = dot(r0.xyzw, prevViewProjection._m02_m12_m22_m32);
    o7.w = dot(r0.xyzw, prevViewProjection._m03_m13_m23_m33);
  } else {
    o7 = float4(0,0,0,0);
  }

  o6.xyzw = r1.xyzw;
  o1.xyzw = float4(1,1,1,1);
  r1.xyz = scene.EyePosition.xyz + -r0.xyz;
  o4.xyz = r0.xyz;
  r0.x = -scene.MiscParameters3.x + r0.y;
  r0.w = dot(r0.xyzw, scene.View._m02_m12_m22_m32);
  r0.x = saturate(scene.MiscParameters3.y * r0.x);
  r0.y = dot(r1.xyz, r1.xyz);
  r0.y = rsqrt(r0.y);
  r1.xyz = r1.xyz * r0.yyy;
  r2.x = dot(v1.xyz, World._m00_m10_m20);
  r2.y = dot(v1.xyz, World._m01_m11_m21);
  r2.z = dot(v1.xyz, World._m02_m12_m22);
  r0.y = dot(r2.xyz, r2.xyz);
  r0.y = rsqrt(r0.y);
  r2.xyz = r2.xyz * r0.yyy;
  r0.y = dot(r2.xyz, r1.xyz);
  o5.xyz = r2.xyz;
  r0.y = max(0, r0.y);
  r0.y = 1 + -r0.y;
  r0.y = max(0, r0.y);
  r0.y = log2(r0.y);
  r0.z = max(1, PointLightColor.w);
  r0.y = r0.z * r0.y;
  r0.y = exp2(r0.y);
  o2.xyz = PointLightColor.xyz * r0.yyy;
  r0.y = -scene.FogRangeParameters.x + -r0.w;
  o4.w = -r0.w;
  r0.y = saturate(scene.FogRangeParameters.z * r0.y);
  r0.z = scene.MiscParameters3.z + r0.y;
  r0.z = min(1, r0.z);
  r0.x = r0.x * r0.z + r0.y;
  r0.x = min(1, r0.x);
  o2.w = scene.FogRangeParameters.w * r0.x;
  o3.xy = v2.xy * GameMaterialTexcoord.zw + GameMaterialTexcoord.xy;
  return;
}
