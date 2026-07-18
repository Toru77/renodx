// ---- Created with 3Dmigoto v1.3.16 on Wed Sep 17 21:30:55 2025
#include "../common.hlsl"
SamplerState samLinear_s : register(s0);
SamplerState samPoint_s : register(s1);
Texture2D<float4> colorTexture : register(t0);
Texture2D<float4> blurTexture : register(t1);


// 3Dmigoto declarations
#define cmp -

float3 vanillaBlend(float3 blur, float3 color) {
  float3 r0;
  r0.xyz = blur * 5.f * color; // b * c 
  r0.xyz = min(float3(1,1,1), r0.xyz); 
  r0.xyz = blur * 5.f - r0.xyz; // b - b * c 
  r0.xyz = max(float3(0,0,0), r0.xyz);
  return color + r0; // c + b - b * c
}


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
  // r1.xyz = float3(5,5,5) * r0.xyz; // 5 * blur
  r2.xyzw = colorTexture.SampleLevel(samPoint_s, v1.zw, 0).xyzw;
  float3 color = r2.rgb;
  // float3 color = r2.rgb;
  // r1.xyz = r2.xyz * r1.xyz; // 5 * blur * color
  // r1.xyz = min(float3(1,1,1), r1.xyz); 
  // r0.xyz = r0.xyz * float3(5,5,5) + -r1.xyz;
  // r0.xyz = max(float3(0,0,0), r0.xyz);
  // o0.xyz = r2.xyz + r0.xyz;

  float3 sdr = vanillaBlend(blur, color);

  float3 hdr = color + blur * shader_injection.bloom_strength;

  if (shader_injection.bloom == 0.f) {
    o0.rgb = sdr;
  } else {

    hdr = renodx::color::srgb::DecodeSafe(hdr);
    sdr = renodx::color::srgb::DecodeSafe(sdr);

    hdr = lerp(hdr, CorrectHueAndPurityMBFullStrength(hdr, sdr), shader_injection.bloom_hue_correction);

    hdr = renodx::color::srgb::EncodeSafe(hdr);

    o0.rgb = hdr;
  }

  if (RENODX_TONE_MAP_TYPE > 0.f) {
    o0.rgb = renodx::color::srgb::DecodeSafe(o0.rgb);

    o0.rgb = ToneMapLMSHueShift(o0.rgb);

    float3 color = o0.rgb;

    [branch]
    if (RENODX_GAMMA_CORRECTION == renodx::draw::GAMMA_CORRECTION_GAMMA_2_2) {
      color = renodx::color::correct::GammaSafe(color, false, 2.2f);
    } else if (RENODX_GAMMA_CORRECTION == renodx::draw::GAMMA_CORRECTION_GAMMA_2_4) {
      color = renodx::color::correct::GammaSafe(color, false, 2.4f);
    } else if (RENODX_GAMMA_CORRECTION == 3.f) {
      color = renodx::color::correct::GammaSafe(color, false, 2.2f);
    }

    // This is RenderIntermediatePass, simply brightness scaling and srgb encoding
    color *= RENODX_DIFFUSE_WHITE_NITS / RENODX_GRAPHICS_WHITE_NITS;

    [branch]
    if (RENODX_GAMMA_CORRECTION == renodx::draw::GAMMA_CORRECTION_GAMMA_2_2) {
      color = renodx::color::correct::GammaSafe(color, true, 2.2f);
    } else if (RENODX_GAMMA_CORRECTION == renodx::draw::GAMMA_CORRECTION_GAMMA_2_4) {
      color = renodx::color::correct::GammaSafe(color, true, 2.4f);
    } else if (RENODX_GAMMA_CORRECTION == 3.f) {
      color = renodx::color::correct::GammaSafe(color, true, 2.2f);
    }

    o0.rgb = color;

    o0.rgb = renodx::color::srgb::EncodeSafe(o0.rgb);
  }
  
  

  o0.w = r2.w;
  return;
}