// ---- Created with 3Dmigoto v1.3.16 on Mon Sep 01 23:53:15 2025
#include "../common.hlsl"
SamplerState sam0_s : register(s0);
Texture2D<float4> colorTexture : register(t0);


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

  r0.xyz = colorTexture.SampleLevel(sam0_s, v1.xy, 0).xyz;
  o0.w = calculateLuminanceSRGB(r0.rgb);

  o0.rgb = r0.rgb;

  
  
  return;
}