// ---- Created with 3Dmigoto v1.4.1 on Thu Mar 12 14:27:31 2026
Texture2D<float4> t0 : register(t0);

SamplerState s0_s : register(s0);

cbuffer cb1 : register(b1)
{
  float4 cb1[2];
}

cbuffer cb0 : register(b0)
{
  float4 cb0[2];
}




// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_Position0,
  float2 v1 : TEXCOORD0,
  out float o0 : SV_Target0,
  out float o1 : SV_Target1,
  out float o2 : SV_Target2,
  out float o3 : SV_Target3,
  out float o4 : SV_Target4,
  out float o5 : SV_Target5,
  out float o6 : SV_Target6,
  out float o7 : SV_Target7)
{
  float4 r0,r1;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.xy = floor(v0.xy);
  r0.xy = r0.xy * float2(4,4) + cb1[1].xy;
  r0.xy = float2(0.5,0.5) + r0.xy;
  r0.xy = cb0[1].zw * r0.xy;
  r1.xyzw = t0.Gather(s0_s, r0.xy).xyzw;
  r0.xyzw = t0.Gather(s0_s, r0.xy, int2(2, 0)).xyzw;
  o0.x = r1.w;
  o1.x = r1.z;
  o2.x = r0.w;
  o3.x = r0.z;
  o4.x = r1.x;
  o5.x = r1.y;
  o6.x = r0.x;
  o7.x = r0.y;
  return;
}