// ---- Created with 3Dmigoto v1.3.16 on Fri Jun 06 16:40:11 2025
#include "./shared.h"
#include "./cs4/common.hlsl"
SamplerState PointClampSampler_s : register(s0);
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

  r0.xyz = ColorBuffer.SampleLevel(PointClampSampler_s, v1.xy, 0).xyz;

  // BT.601 luma
  o0.w = calculateLuminanceSRGB(r0.xyz);
  o0.xyz = r0.xyz;
  return;
}