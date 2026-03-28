// ---- Created with 3Dmigoto v1.4.1 on Sat Mar 28 19:41:21 2026

SamplerState LinearClampSampler_s : register(s0);
Texture2D<float4> ColorBuffer : register(t0);


// 3Dmigoto declarations
#define cmp -
#include "./common.hlsl"
#include "./shared.h"


void main(
  float4 v0 : SV_POSITION0,
  float2 v1 : TEXCOORD0,
  out float4 o0 : SV_TARGET0)
{
  float4 r0;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.xyzw = ColorBuffer.Sample(LinearClampSampler_s, v1.xy).xyzw;
  o0.w = dot(r0.xyz, float3(0.298999995,0.587000012,0.114));
  o0.xyz = ApplyToneMapPass(r0.xyz);
  o0.xyz = RenderIntermediatePass(o0.xyz);
  return;
}
