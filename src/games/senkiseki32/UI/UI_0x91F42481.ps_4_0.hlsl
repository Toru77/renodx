// ---- Created with 3Dmigoto v1.4.1 on Thu Feb 12 12:18:38 2026
#include "../shared.h"

cbuffer _Globals : register(b0)
{
  float3 scene_EyePosition : packoffset(c0);
  float4x4 scene_View : packoffset(c1);
  float4x4 scene_ViewProjection : packoffset(c5);
  float3 scene_GlobalAmbientColor : packoffset(c9);
  float scene_GlobalTexcoordFactor : packoffset(c9.w);
  float3 scene_FakeRimLightDir : packoffset(c10);
  float4 scene_MiscParameters2 : packoffset(c11);
  float scene_AdditionalShadowOffset : packoffset(c12);
  float4 scene_cameraNearFarParameters : packoffset(c13);
  float4x4 WorldViewProjection : packoffset(c14);
  float4x4 World : packoffset(c18);
  float3 lookPosition : packoffset(c22) = {0,0,0};
  float4 inputSpecular : packoffset(c23) = {0,0,0,0};
  float2 inputShift : packoffset(c24) = {0,0};
  float2 inputHeight : packoffset(c24.z) = {3,8};
  float inputColorShiftHeight : packoffset(c25) = {50};
}

SamplerState LinearSamplerState_s : register(s0);
Texture2D<float4> MinimapTextureSampler : register(t0);


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_POSITION0,
  float4 v1 : COLOR0,
  float2 v2 : TEXCOORD0,
  out float4 o0 : SV_TARGET0)
{
  float4 r0;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.xyzw = MinimapTextureSampler.SampleLevel(LinearSamplerState_s, v2.xy, 0).xyzw;
  r0.w = v1.w * r0.w;
  o0.xyz = r0.xyz * v1.xyz + inputSpecular.xyz;
  o0.w = r0.w;
  o0 = saturate(o0);
  return;
}