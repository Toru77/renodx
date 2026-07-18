// ---- Created with 3Dmigoto v1.3.16 on Mon Sep 29 00:42:31 2025
#include "../common.hlsl"
SamplerState samLinear_s : register(s0);
SamplerState samPoint_s : register(s1);
Texture2D<float4> colorTexture : register(t0);
Texture2D<float4> blurTexture1 : register(t1);
Texture2D<float4> blurTexture2 : register(t2);
Texture2D<float4> blurTexture3 : register(t3);
Texture2D<float4> blurTexture4 : register(t4);
Texture2D<float4> blurTexture5 : register(t5);


// 3Dmigoto declarations
#define cmp -

float3 vanillaSdrBlend(float3 base, float3 blend) {
  float3 oneMinusBase = max(1 - base, 0.f);

  return blend * oneMinusBase + base;
}

float4 blendBloomSrgb(SamplerState samLinear_s, float4 v1) {
  float4 output;

  float4 gameScene = colorTexture.SampleLevel(samPoint_s, v1.xy, 0).xyzw;
  output.w = gameScene.w;

  float3 blur1 = blurTexture1.SampleLevel(samLinear_s, v1.zw, 0).xyz;
  float3 blur2 = blurTexture2.SampleLevel(samLinear_s, v1.zw, 0).xyz;
  float3 blur3 = blurTexture3.SampleLevel(samLinear_s, v1.zw, 0).xyz;
  float3 blur4 = blurTexture4.SampleLevel(samLinear_s, v1.zw, 0).xyz;
  float3 blur5 = blurTexture5.SampleLevel(samLinear_s, v1.zw, 0).xyz;

  float3 blended = gameScene.rgb;

  blended = vanillaSdrBlend(blended, blur1);
  blended = vanillaSdrBlend(blended, blur2);
  blended = vanillaSdrBlend(blended, blur3);
  blended = vanillaSdrBlend(blended, blur4);
  blended = vanillaSdrBlend(blended, blur5);

  output.rgb = blended;

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

  // r0.xyz = blurTexture1.SampleLevel(samLinear_s, v1.zw, 0).xyz;
  // r1.xyzw = colorTexture.SampleLevel(samPoint_s, v1.xy, 0).xyzw;

  // if (shader_injection.bloom == 0.f) {
  //   r2.xyz = float3(1,1,1) + -r1.xyz;
  //   r2.xyz = max(float3(0,0,0), r2.xyz);
  //   r0.xyz = r0.xyz * r2.xyz + r1.xyz;
  //   o0.w = r1.w;
  //   r1.xyz = float3(1,1,1) + -r0.xyz;
  //   r1.xyz = max(float3(0,0,0), r1.xyz);
  //   r2.xyz = blurTexture2.SampleLevel(samLinear_s, v1.zw, 0).xyz;
  //   r0.xyz = r2.xyz * r1.xyz + r0.xyz;
  //   r1.xyz = float3(1,1,1) + -r0.xyz;
  //   r1.xyz = max(float3(0,0,0), r1.xyz);
  //   r2.xyz = blurTexture3.SampleLevel(samLinear_s, v1.zw, 0).xyz;
  //   r0.xyz = r2.xyz * r1.xyz + r0.xyz;
  //   r1.xyz = float3(1,1,1) + -r0.xyz;
  //   r1.xyz = max(float3(0,0,0), r1.xyz);
  //   r2.xyz = blurTexture4.SampleLevel(samLinear_s, v1.zw, 0).xyz;
  //   r0.xyz = r2.xyz * r1.xyz + r0.xyz;
  //   r1.xyz = float3(1,1,1) + -r0.xyz;
  //   r1.xyz = max(float3(0,0,0), r1.xyz);
  //   r2.xyz = blurTexture5.SampleLevel(samLinear_s, v1.zw, 0).xyz;
  //   o0.xyz = r2.xyz * r1.xyz + r0.xyz;
  // } else {

  float4 gameScene = colorTexture.SampleLevel(samPoint_s, v1.xy, 0).xyzw;
  o0.w = gameScene.w;

  float3 blur1 = blurTexture1.SampleLevel(samLinear_s, v1.zw, 0).xyz;
  float3 blur2 = blurTexture2.SampleLevel(samLinear_s, v1.zw, 0).xyz;
  float3 blur3 = blurTexture3.SampleLevel(samLinear_s, v1.zw, 0).xyz;
  float3 blur4 = blurTexture4.SampleLevel(samLinear_s, v1.zw, 0).xyz;
  float3 blur5 = blurTexture5.SampleLevel(samLinear_s, v1.zw, 0).xyz;

  float3 blended = gameScene.rgb;

  blended = vanillaSdrBlend(blended, blur1);
  blended = vanillaSdrBlend(blended, blur2);
  blended = vanillaSdrBlend(blended, blur3);
  blended = vanillaSdrBlend(blended, blur4);
  blended = vanillaSdrBlend(blended, blur5);

  float3 sdr = blended;

  if (shader_injection.bloom == 0.f || RENODX_TONE_MAP_TYPE == 0.f) {
    o0.rgb = sdr;
  } else {
    float3 hdr = (gameScene.rgb);

    hdr = renodx::color::srgb::DecodeSafe(hdr);
    blur1 = renodx::color::srgb::DecodeSafe(blur1);
    blur2 = renodx::color::srgb::DecodeSafe(blur2);
    blur3 = renodx::color::srgb::DecodeSafe(blur3);
    blur4 = renodx::color::srgb::DecodeSafe(blur4);
    blur5 = renodx::color::srgb::DecodeSafe(blur5);

    hdr = hdrScreenBlend(hdr, (blur1), 1.f);
    hdr = hdrScreenBlend(hdr, (blur2), 1.f);
    hdr = hdrScreenBlend(hdr, (blur3), 1.f);
    hdr = hdrScreenBlend(hdr, (blur4), 1.f);
    hdr = hdrScreenBlend(hdr, (blur5), 1.f);

    sdr = renodx::color::srgb::DecodeSafe(sdr);

    o0.rgb = hdr;
    o0.rgb = renodx::color::srgb::EncodeSafe(o0.rgb);
  }

  // }
  return;
}