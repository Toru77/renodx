// ---- Created with 3Dmigoto v1.3.16 on Wed Sep 17 21:30:55 2025
#include "common.hlsl"
SamplerState samLinear_s : register(s0);
SamplerState samPoint_s : register(s1);
Texture2D<float4> colorTexture : register(t0);
Texture2D<float4> blurTexture : register(t1);


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_Position0,
  float4 v1 : TEXCOORD0,
  out float4 o0 : SV_Target0)
{
  float4 r0,r1,r2;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.xyz = blurTexture.SampleLevel(samLinear_s, v1.xy, 0).xyz;
  float3 blur = r0.rgb;
  r1.xyz = float3(5,5,5) * r0.xyz; // 5 * blur
  r2.xyzw = colorTexture.SampleLevel(samPoint_s, v1.zw, 0).xyzw;
  float3 color = r2.rgb;
  r1.xyz = r2.xyz * r1.xyz; // 5 * blur * color
  r1.xyz = min(float3(1,1,1), r1.xyz); 
  r0.xyz = r0.xyz * float3(5,5,5) + -r1.xyz;
  r0.xyz = max(float3(0,0,0), r0.xyz);
  o0.xyz = r2.xyz + r0.xyz;

  // o0.rgb = r2.rgb;
  o0.rgb = processAndToneMap(o0.rgb, true);
  o0.w = r2.w;
  return;
}