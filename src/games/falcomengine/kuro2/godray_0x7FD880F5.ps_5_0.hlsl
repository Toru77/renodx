// ---- Created with 3Dmigoto v1.3.16 on Mon Sep 29 00:42:31 2025
#include "../common.hlsl"
cbuffer cb_local : register(b2)
{
  float2 blurCenter_g : packoffset(c0);
  float2 blurScale_g : packoffset(c0.z);
  float zThreshold_g : packoffset(c1);
  float isFlip_g : packoffset(c1.y);
  float brightnessThreshold_g : packoffset(c1.z);
  float centricSharpness_g : packoffset(c1.w);
  float3 godrayColor_g : packoffset(c2);
}

SamplerState samPoint_s : register(s0);
SamplerState samLinear_s : register(s1);
Texture2D<float4> colorTexture : register(t0);
Texture2D<float4> godrayTexture : register(t1);




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

  r0.xyz = godrayTexture.SampleLevel(samLinear_s, v1.xy, 0).xyz;
  r1.xyzw = colorTexture.SampleLevel(samPoint_s, v1.xy, 0).xyzw;

  if (shader_injection.bloom == 0.f && RENODX_TONE_MAP_TYPE == 0.f ) {
    r0.rgb = saturate(r0.rgb);
    // r1.rgb = saturate(r1.rgb);
    r0.xyz = godrayColor_g.xyz * r0.xyz;
    r2.xyz = float3(1,1,1) + -r1.xyz;
    r2.xyz = max(float3(0,0,0), r2.xyz);
    o0.xyz = r0.xyz * r2.xyz + r1.xyz;
  } else {
    float3 godray = godrayColor_g.xyz * r0.xyz;

    float3 color = r1.xyz;

    o0.rgb = addBloom(srgbDecode(color), srgbDecode(godray));

    o0.rgb = srgbEncode(o0.rgb);
    o0.w = r1.w;
  }
  o0.w = r1.w;
  return;
}