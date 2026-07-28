// ---- Created with 3Dmigoto v1.4.1 on Tue Jul 28 01:02:54 2026

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
  o0.w = dot(r0.xyz, float3(0.298999995,0.587000012,0.114));
  o0.xyz = r0.xyz;
  return;
}