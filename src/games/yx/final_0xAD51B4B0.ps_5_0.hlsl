// ---- Created with 3Dmigoto v1.3.16 on Mon Sep 01 23:38:26 2025
#include "./common.hlsl"
cbuffer Constants : register(b0)
{
  float gamma : packoffset(c0);
  uint hdr_enabled : packoffset(c0.y);
  float hdr_peak_brightness : packoffset(c0.z);
}

SamplerState smpl_s : register(s0);
Texture2D<float4> tex : register(t0);


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_Position0,
  float2 v1 : TEXCOORD0,
  out float4 o0 : SV_Target0)
{
  float4 r0;
  uint4 bitmask, uiDest;
  float4 fDest;

  if (hdr_enabled != 0) {
    r0.xyz = tex.SampleLevel(smpl_s, v1.xy, 0).xyz;

    if (RENODX_TONE_MAP_TYPE > 0) {

      r0.rgb = renodx::color::srgb::DecodeSafe(r0.rgb);

      o0 = r0;

      renodx::draw::Config config = renodx::draw::BuildConfig();

      if (config.gamma_correction == renodx::draw::GAMMA_CORRECTION_GAMMA_2_2) {
          o0.rgb = GammaCorrectHuePreserving(o0.rgb, 2.2f);
      } else if (config.gamma_correction == renodx::draw::GAMMA_CORRECTION_GAMMA_2_4) {
          o0.rgb = GammaCorrectHuePreserving(o0.rgb, 2.4f);
      } else if (config.gamma_correction == 3.f) {
          o0.rgb = GammaCorrectHuePreserving(o0.rgb, 2.3f);
      } 

      float3 color = o0.rgb;

      // color = expandGamut(color, shader_injection.inverse_tonemap_extra_hdr_saturation);
      
      [branch]
      if (config.swap_chain_custom_color_space == renodx::draw::COLOR_SPACE_CUSTOM_BT709D93) {
        color = renodx::color::convert::ColorSpaces(color, config.swap_chain_decoding_color_space, renodx::color::convert::COLOR_SPACE_BT709);
        color = renodx::color::bt709::from::BT709D93(color);
        config.swap_chain_decoding_color_space = renodx::color::convert::COLOR_SPACE_BT709;
      } else if (config.swap_chain_custom_color_space == renodx::draw::COLOR_SPACE_CUSTOM_NTSCU) {
        color = renodx::color::convert::ColorSpaces(color, config.swap_chain_decoding_color_space, renodx::color::convert::COLOR_SPACE_BT709);
        color = renodx::color::bt709::from::BT601NTSCU(color);
        config.swap_chain_decoding_color_space = renodx::color::convert::COLOR_SPACE_BT709;
      } else if (config.swap_chain_custom_color_space == renodx::draw::COLOR_SPACE_CUSTOM_NTSCJ) {
        color = renodx::color::convert::ColorSpaces(color, config.swap_chain_decoding_color_space, renodx::color::convert::COLOR_SPACE_BT709);
        color = renodx::color::bt709::from::ARIBTRB9(color);
        config.swap_chain_decoding_color_space = renodx::color::convert::COLOR_SPACE_BT709;
      }
      o0.rgb = color;
      
      o0.rgb = renodx::color::bt709::clamp::AP1(o0.rgb);

      o0.rgb *= RENODX_GRAPHICS_WHITE_NITS / 80.f;

      o0.w = 1;
    }
    else {

      r0.xyz = log2(r0.xyz);
      r0.xyz = float3(2.29999995,2.29999995,2.29999995) * r0.xyz;
      r0.xyz = exp2(r0.xyz);
      r0.xyz = hdr_peak_brightness * r0.xyz;
      r0.xyz = max(float3(0,0,0), r0.xyz);
      o0.xyz = min(float3(200,200,200), r0.xyz);
      o0.w = 1;
      return;
    }
  } else {
    r0.xyz = tex.SampleLevel(smpl_s, v1.xy, 0).xyz;
    r0.xyz = saturate(r0.xyz);
    r0.xyz = log2(r0.xyz);
    r0.xyz = gamma * r0.xyz;
    o0.xyz = exp2(r0.xyz);
    o0.w = 1;
    return;
  }
  return;
}