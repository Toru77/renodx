// ---- Created with 3Dmigoto v1.4.1 on Sun Apr  5 23:53:15 2026
Texture2D<float4> t0 : register(t0);

cbuffer cb0 : register(b0)
{
  float4 cb0[3];
}




// 3Dmigoto declarations
#define cmp -
#include "./common.hlsl"


void main(
  float4 v0 : SV_POSITION0,
  float2 v1 : TEXCOORD0,
  out float oDepth : SV_Depth)
{
  float4 r0;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.xy = v0.xy * cb0[1].xy + cb0[1].zw;
  r0.xy = max(float2(0,0), r0.xy);
  r0.zw = float2(-1,-1) + cb0[2].zw;
  r0.xy = min(r0.xy, r0.zw);
  r0.xy = (int2)r0.xy;
  r0.zw = float2(0,0);
  r0.x = t0.Load(r0.xyz).x;
  oDepth = r0.x;
  //r0.xyz = ApplyToneMapPass(r0.xyz);
  //r0.xyz = RenderIntermediatePass(r0.xyz);
  return;
}