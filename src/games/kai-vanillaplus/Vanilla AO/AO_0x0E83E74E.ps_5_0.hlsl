// ---- Created with 3Dmigoto v1.4.1 on Thu Mar 12 14:27:31 2026
Texture2D<float4> t0 : register(t0);

cbuffer cb0 : register(b0)
{
  float4 cb0[8];
}




// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_Position0,
  float2 v1 : TEXCOORD0,
  out float o0 : SV_TARGET0)
{
  float4 r0;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.xy = cb0[7].xy + v0.xy;
  r0.xy = (int2)r0.xy;
  r0.zw = float2(0,0);
  r0.x = t0.Load(r0.xyz).x;
  r0.x = saturate(cb0[6].z * r0.x + cb0[6].w);
  r0.x = r0.x * cb0[6].x + cb0[6].y;
  o0.x = 1 / r0.x;
  return;
}