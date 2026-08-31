// ---- Created with 3Dmigoto v1.3.16 on Fri Aug 21 00:40:15 2026

SamplerState smpl_s : register(s0);
Texture2D<float4> tex : register(t0);


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_Position0,
  float2 v1 : TEXCOORD0,
  out float4 o0 : SV_Target0)
{
  o0.xyzw = tex.SampleLevel(smpl_s, v1.xy, 0).xyzw;

  return;
}