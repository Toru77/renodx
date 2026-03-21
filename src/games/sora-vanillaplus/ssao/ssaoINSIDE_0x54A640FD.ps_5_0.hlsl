// ---- Created with 3Dmigoto v1.4.1 on Sun Mar 22 00:32:12 2026

SamplerState samPoint_s : register(s0);
Texture2D<float4> depthTexture : register(t0);


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_Position0,
  float4 v1 : TEXCOORD0,
  out float oDepth : SV_Depth)
{
  float4 r0;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.xyzw = depthTexture.Gather(samPoint_s, v1.xy).xyzw;
  r0.y = max(r0.y, r0.z);
  r0.y = max(r0.y, r0.w);
  oDepth = max(r0.x, r0.y);
  return;
}