

#include "../common.hlsl"

float3 LogMap(float3 c) { return log(1.0 + c); }
float3 LogMapInv(float3 l) { return exp(l) - 1.0; }


// ---- Created with 3Dmigoto v1.3.16 on Thu Aug 21 11:48:23 2025

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
}

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


float3 one_minus(float3 x)  {

  return 1 - x;
}

float3 vanillaSdrBlend(float3 base, float3 blend) {

  float3 oneMinusBase = max(1 - base, 0.f);

  // blend * ( 1 - base) + base
  // = blend + base - blend * base

  return blend * oneMinusBase + base;
}

float4 blendBloomSrgb(SamplerState samLinear_s, float4 v1) {

  float4 r0, r1, r2, r3;
  float4 output;

  float4 gameScene;

  r0.xyzw = min(uv_clamp0_g.xyzw, v1.xyxy);
  float3 blur1 = blurTexture1.SampleLevel(samLinear_s, r0.xy, 0).xyz;
  float3 blur2 = blurTexture2.SampleLevel(samLinear_s, r0.zw, 0).xyz;
  gameScene.xyzw = colorTexture.SampleLevel(samPoint_s, v1.xy, 0).xyzw;
  output.w = gameScene.w;

  output.rgb = vanillaSdrBlend(gameScene.rgb, blur1);
  output.rgb = vanillaSdrBlend(output.rgb, blur2);

  r2.xyzw = min(uv_clamp1_g.xyzw, v1.xyxy);
  float3 blur3 = blurTexture3.SampleLevel(samLinear_s, r2.xy, 0).xyz;
  float3 blur4 = blurTexture4.SampleLevel(samLinear_s, r2.zw, 0).xyz;

  output.rgb = vanillaSdrBlend(output.rgb, blur3);
  output.rgb = vanillaSdrBlend(output.rgb, blur4);
  
  r2.xy = min(uv_clamp2_g.xy, v1.xy);
  float3 blur5 = blurTexture5.SampleLevel(samLinear_s, r2.xy, 0).xyz;
  output.rgb = vanillaSdrBlend(output.rgb, blur5);
  
  return output;
}


float4 blendBloomSrgbLogSpace(SamplerState samLinear_s, float4 v1) {

  float4 r0, r1, r2, r3;
  float4 output;

  float4 gameScene;

  r0.xyzw = min(uv_clamp0_g.xyzw, v1.xyxy);
  r1.xyz = blurTexture1.SampleLevel(samLinear_s, r0.xy, 0).xyz;
  r1.rgb = LogMap(r1.rgb);
  r0.xyz = blurTexture2.SampleLevel(samLinear_s, r0.zw, 0).xyz;
  r0.rgb = LogMap(r0.rgb);
  gameScene.xyzw = colorTexture.SampleLevel(samPoint_s, v1.xy, 0).xyzw;
  gameScene.rgb = LogMap(gameScene.rgb);
  output.w = gameScene.w;

  r3.xyz = float3(1,1,1) + -gameScene.xyz;
  r3.xyz = max(float3(0,0,0), r3.xyz);
  r1.xyz = r1.xyz * r3.xyz + gameScene.xyz;
  r2.xyz = float3(1,1,1) + -r1.xyz;
  r2.xyz = max(float3(0,0,0), r2.xyz);

  r0.xyz = r0.xyz * r2.xyz + r1.xyz;
  r1.xyz = float3(1,1,1) + -r0.xyz;
  r1.xyz = max(float3(0,0,0), r1.xyz);
  r2.xyzw = min(uv_clamp1_g.xyzw, v1.xyxy);
  r3.xyz = blurTexture3.SampleLevel(samLinear_s, r2.xy, 0).xyz;
  r3.rgb = LogMap(r3.rgb);
  float3 blur3 = r3.xyz;
  r2.xyz = blurTexture4.SampleLevel(samLinear_s, r2.zw, 0).xyz;
  r2.rgb = LogMap(r2.rgb);
  float3 blur4 = r2.xyz;
  
  r0.xyz = blur3 * r1.xyz + r0.xyz;
  r1.xyz = float3(1,1,1) + -r0.xyz;
  r1.xyz = max(float3(0,0,0), r1.xyz);
  r0.xyz = blur4 * r1.xyz + r0.xyz;
  r1.xyz = float3(1,1,1) + -r0.xyz;
  r1.xyz = max(float3(0,0,0), r1.xyz);
  r2.xy = min(uv_clamp2_g.xy, v1.xy);
  float3 blur5 = blurTexture5.SampleLevel(samLinear_s, r2.xy, 0).xyz;
  blur5 = LogMap(blur5.rgb);
  output.xyz = blur5 * r1.xyz + r0.xyz;

  output.rgb = LogMapInv(output.rgb);
  
  return output;
}

void main(
    float4 v0 : SV_Position0,
    float4 v1 : TEXCOORD0,
    out float4 o0 : SV_Target0)
{
    float4 sdr = blendBloomSrgb(samLinear_s, v1);
    float scale = 0.f;
    if (RENODX_TONE_MAP_TYPE == 0 || shader_injection.bloom == 0.f)  {
      o0 = sdr;
      // o0 = blendBloomSrgbLogSpace(samLinear_s, v1); 
      return;
    }

  
    float4 gameScene = colorTexture.SampleLevel(samPoint_s, v1.xy, 0);

    float3 hdr = gameScene.rgb;
    hdr = renodx::color::srgb::DecodeSafe(hdr);
    o0.w = gameScene.w; 
    
    float4 clampedUV0 = min(uv_clamp0_g, v1.xyxy);
    float3 blur1 = blurTexture1.SampleLevel(samLinear_s, clampedUV0.xy, 0).xyz;
    blur1 = renodx::color::srgb::DecodeSafe(blur1);
    float3 blur2 = blurTexture2.SampleLevel(samLinear_s, clampedUV0.zw, 0).xyz;
    blur2 = renodx::color::srgb::DecodeSafe(blur2);

    hdr = hdrScreenBlend(hdr, blur1, scale);
    hdr = hdrScreenBlend(hdr, blur2, scale);
    
    float4 clampedUV1 = min(uv_clamp1_g, v1.xyxy);
    float3 blur3 = blurTexture3.SampleLevel(samLinear_s, clampedUV1.xy, 0).xyz;
    float3 blur4 = blurTexture4.SampleLevel(samLinear_s, clampedUV1.zw, 0).xyz;
    blur3 = renodx::color::srgb::DecodeSafe(blur3);
    blur4 = renodx::color::srgb::DecodeSafe(blur4);
    
    hdr = hdrScreenBlend(hdr, blur3, scale);
    hdr = hdrScreenBlend(hdr, blur4, scale);
    
    float2 clampedUV2 = min(uv_clamp2_g.xy, v1.xy);
    float3 blur5 = blurTexture5.SampleLevel(samLinear_s, clampedUV2, 0).xyz;
    blur5 = renodx::color::srgb::DecodeSafe(blur5);

    hdr = hdrScreenBlend(hdr, blur5, scale);

    float3 sdr_color = renodx::color::srgb::DecodeSafe(sdr.rgb);
    // float3 sat = saturate(renodx::color::srgb::DecodeSafe(sdr.rgb));

    // hue and chrominance correction if desaturation is desired
    // hdr = HueAndChrominanceOKLab(hdr, sdr_color, sdr_color, shader_injection.bloom_hue_correction, shader_injection.bloom_hue_correction);
    // hdr = CorrectPurityMBBT709WithBT2020(hdr, sdr_color, shader_injection.bloom_hue_correction);
    // hdr = CorrectHueMBGated(hdr, sdr_color, shader_injection.bloom_hue_correction, 0.18f, 1.f);

    float strength = shader_injection.bloom_hue_correction;
    // hdr = CorrectHueAndPurityMBGated(hdr, sdr_color, strength, 0.18f, 0.5f, strength);
    hdr = lerp(hdr, CorrectHueAndPurityMBFullStrength(hdr, sdr_color), saturate(strength));

    o0.rgb = hdr.rgb;

    o0.rgb = renodx::color::srgb::EncodeSafe(o0.rgb);
  
}

