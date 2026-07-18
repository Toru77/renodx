#include "../common.hlsl"

// ---- Created with 3Dmigoto v1.4.1 on Sat Jul 18 23:45:29 2026

cbuffer cb_local : register(b2)
{
  float2 blurCenter_g : packoffset(c0);
  float2 blurScale_g : packoffset(c0.z);
  float zThreshold_g : packoffset(c1);
  float isFlip_g : packoffset(c1.y);
  float brightnessThreshold_g : packoffset(c1.z);
  float centricSharpness_g : packoffset(c1.w);
  float3 godrayColor_g : packoffset(c2);
  float pad : packoffset(c2.w);
  float2 uv_clamp_g : packoffset(c3);
  float2 pad2 : packoffset(c3.z);
}

SamplerState samPoint_s : register(s0);
SamplerState samLinear_s : register(s1);
Texture2D<float4> colorTexture : register(t0);
Texture2D<float4> godrayTexture : register(t1);


// 3Dmigoto declarations
#define cmp -


float4 blendGodRaySrgb(Texture2D<float4> godrayTexture, Texture2D<float4> colorTexture, float4 v1)  {

  float4 r0,r1,r2, output;
  r0.xyz = godrayTexture.SampleLevel(samLinear_s, v1.xy, 0).xyz;
  // r0.rgb = saturate(r0.rgb);
  r0.xyz = godrayColor_g.xyz * r0.xyz;
  r1.xyzw = colorTexture.SampleLevel(samPoint_s, v1.xy, 0).xyzw;
  r2.xyz = float3(1, 1, 1) + (r1.xyz);
  r2.rgb = max(0.f, r2.rgb);
  output.xyz = r0.xyz * r2.xyz + r1.xyz;
  output.w = r1.w;

  return output;
}

void main(
  float4 v0 : SV_Position0,
  float4 v1 : TEXCOORD0,
  out float4 o0 : SV_Target0)
{
  float4 r0,r1,r2;
  uint4 bitmask, uiDest;
  float4 fDest;

  float4 vanilla = blendGodRaySrgb(godrayTexture, colorTexture, v1);
  o0 = vanilla;
  return;
}