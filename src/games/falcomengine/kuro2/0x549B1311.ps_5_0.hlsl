// ---- Created with 3Dmigoto v1.3.16 on Mon Sep 29 00:42:31 2025

cbuffer cb_local : register(b2)
{
  float exposureBias_g : packoffset(c0);
  float exposureMin_g : packoffset(c0.y);
  float exposureMax_g : packoffset(c0.z);
  float lumAdvance_g : packoffset(c0.w);
  float intensity_g : packoffset(c1);
}

SamplerState samPoint_s : register(s0);
Texture2D<float4> srcTexture : register(t0);
Texture2D<float4> curTexture : register(t1);


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_Position0,
  float4 v1 : TEXCOORD0,
  out float4 o0 : SV_Target0)
{
  float4 r0;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.x = curTexture.Sample(samPoint_s, float2(0.5,0.5), int2(0, 0)).x;
  r0.y = srcTexture.Sample(samPoint_s, float2(0.5,0.5), int2(0, 0)).x;
  r0.x = r0.x + -r0.y;
  o0.x = r0.x * lumAdvance_g + r0.y;
  o0.yzw = float3(0,0,1);
  return;
}