// ---- Created with 3Dmigoto v1.3.16 on Fri Jun 06 17:15:12 2025
#include "shared.h"
#include "cs4/common.hlsl"
SamplerState LinearClampSamplerState_s : register(s0);
Texture2D<float4> ColorBuffer : register(t0);


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_POSITION0,
  float2 v1 : TEXCOORD0,
  out float4 o0 : SV_TARGET0)
{
  o0.xyzw = ColorBuffer.SampleLevel(LinearClampSamplerState_s, v1.xy, 0).xyzw;
  o0.rgb = clamp(o0.rgb, 0.f, shader_injection.safe_clamp);

  // o0.rgb = renodx::tonemap::ReinhardPiecewiseExtended(o0.rgb, 100.f, shader_injection.safe_clamp);
  // o0.rgb = renodx::tonemap::HermiteSplinePerChannelRolloff(o0.rgb, shader_injection.safe_clamp, 100.f);
  return;
}