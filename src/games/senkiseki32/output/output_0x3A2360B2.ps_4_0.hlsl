// ---- Created with 3Dmigoto v1.4.1 on Thu Feb 12 23:46:52 2026
#include "../shared.h"

SamplerState LinearClampSampler_s : register(s0);
Texture2D<float4> ColorBuffer : register(t0);


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_POSITION0,
  float2 v1 : TEXCOORD0,
  out float4 o0 : SV_TARGET0)
{
  float4 r0;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.xyzw = ColorBuffer.SampleLevel(LinearClampSampler_s, v1.xy, 0).xyzw;
  o0.xyz = saturate(r0.xyz);
  o0.w = 1;
  return;
}