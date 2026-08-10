// ---- Created with 3Dmigoto v1.3.16 on Fri Jun 06 22:18:00 2025

SamplerState LinearClampSamplerState_s : register(s0);
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

  r0.xyz = ColorBuffer.SampleLevel(LinearClampSamplerState_s, v1.xy, 0).xyz;
  o0.xyz = r0.xyz;
  o0.w = 1;

  o0 = max(o0, 0.f);
  return;
}