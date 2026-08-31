// ---- Created with 3Dmigoto v1.3.16 on Fri Aug 21 00:15:18 2026
#include "../common.hlsl"

cbuffer cb_glow : register(b2)
{
  float4 uv_clamp0_g : packoffset(c0);
  float4 uv_clamp1_g : packoffset(c1);
  float2 uv_clamp2_g : packoffset(c2);
  float2 intensityLum_g : packoffset(c2.z);
  float2 chrIntensityLum_g : packoffset(c3);
  float atmosphereFadeBegin_g : packoffset(c3.z);
  float atmosphereFadeRangeInv_g : packoffset(c3.w);
  float atmosphereIntensity_g : packoffset(c4);
  float atmosphereVolumetricRatio_g : packoffset(c4.y);
}

SamplerState samLinear_s : register(s0);
Texture2D<float4> blurTexture1 : register(t0);
Texture2D<float4> blurTexture2 : register(t1);
Texture2D<float4> blurTexture3 : register(t2);
Texture2D<float4> blurTexture4 : register(t3);
Texture2D<float4> blurTexture5 : register(t4);


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_Position0,
  float4 v1 : TEXCOORD0,
  out float4 o0 : SV_Target0)
{
  float4 r0,r1,r2,r3;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.xyzw = min(uv_clamp0_g.xyzw, v1.xyxy);
  r1.xyz = blurTexture2.SampleLevel(samLinear_s, r0.zw, 0).xyz;

  float3 blur2 = r1.rgb;

  r0.xyzw = blurTexture1.SampleLevel(samLinear_s, r0.xy, 0).xyzw;

  float3 blur1 = r0.rgb;

  r2.xyz = float3(1,1,1) + -r0.xyz;
  r2.xyz = max(float3(0,0,0), r2.xyz);
  r0.xyz = r1.xyz * r2.xyz + r0.xyz;
  
  r1.xyz = float3(1,1,1) + -r0.xyz;
  r1.xyz = max(float3(0,0,0), r1.xyz);
  r2.xyzw = min(uv_clamp1_g.xyzw, v1.xyxy);
  r3.xyz = blurTexture3.SampleLevel(samLinear_s, r2.xy, 0).xyz;
  float3 blur3 = r3.rgb;
  r2.xyz = blurTexture4.SampleLevel(samLinear_s, r2.zw, 0).xyz;
  float3 blur4 = r2.rgb;
  r0.xyz = r3.xyz * r1.xyz + r0.xyz;
  r1.xyz = float3(1,1,1) + -r0.xyz;
  r1.xyz = max(float3(0,0,0), r1.xyz);
  r0.xyz = r2.xyz * r1.xyz + r0.xyz;
  r1.xyz = float3(1,1,1) + -r0.xyz;
  r1.xyz = max(float3(0,0,0), r1.xyz);
  r2.xy = min(uv_clamp2_g.xy, v1.xy);
  r2.xyz = blurTexture5.SampleLevel(samLinear_s, r2.xy, 0).xyz;
  float3 blur5 = r2.rgb;


  float3 sdr = r2.xyz * r1.xyz + r0.xyz;

  if (shader_injection.bloom == 0.f || RENODX_TONE_MAP_TYPE == 0.f) {
    o0.rgb = sdr;
  } else {
    blur1 = renodx::color::srgb::DecodeSafe(blur1);
    blur2 = renodx::color::srgb::DecodeSafe(blur2);
    blur3 = renodx::color::srgb::DecodeSafe(blur3);
    blur4 = renodx::color::srgb::DecodeSafe(blur4);
    blur5 = renodx::color::srgb::DecodeSafe(blur5);

    float3 hdr = blur1;

    hdr = addBloom(hdr, blur2);
    hdr = addBloom(hdr, blur3);
    hdr = addBloom(hdr, blur4);
    hdr = addBloom(hdr, blur5);

    float strength = shader_injection.bloom_hue_correction;
    sdr = renodx::color::srgb::DecodeSafe(sdr);
    hdr = lerp(hdr, CorrectHueAndPurityMBFullStrength(hdr, sdr), saturate(strength));

    o0.rgb = renodx::color::srgb::EncodeSafe(hdr);
  }

  o0.w = r0.w;
  return;
}