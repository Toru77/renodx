// ---- Created with 3Dmigoto v1.4.1 on Sat Jun 27 13:46:39 2026

SamplerState smpl_s : register(s0);
Texture2D<float4> tex : register(t0);


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_Position0,
  float2 v1 : TEXCOORD0,
  out float oDepth : SV_Depth)
{
  float4 r0;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.x = tex.SampleLevel(smpl_s, v1.xy, 0).x;
  oDepth = r0.x;
  return;
}