// ---- Created with 3Dmigoto v1.4.1 on Fri Mar 27 18:30:53 2026

cbuffer _Globals : register(b0)
{
  float4 AmbColParam : packoffset(c0);
  float4 LuminasParam : packoffset(c1);
  float4 ToneMapParam : packoffset(c2);
  float4 ToneScalerParam : packoffset(c3);
  float4 ToneSaoParam : packoffset(c4);
  float4 GradingParam : packoffset(c5);
  float4x4 ScrnViewProjection : packoffset(c6);
  float4x4 invScrnViewProjection : packoffset(c10);
}

SamplerState PointClampSampler_s : register(s0);
Texture2D<float> Exposure0Buffer : register(t0);
Texture2D<float> Exposure1Buffer : register(t1);

#include "./shared.h"


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_POSITION0,
  float2 v1 : TEXCOORD0,
  out float o0 : SV_TARGET0)
{
  float4 r0,r1;
  uint4 bitmask, uiDest;
  float4 fDest;

  // Override game's exposure with RenoDX-injected exposure value.
  // The shader_injection constant buffer provides `tone_map_exposure`.
  float userExposure = 0.0f;
  r0.x = userExposure;
  o0.x = LuminasParam.x * r0.x + LuminasParam.y;
  return;
}