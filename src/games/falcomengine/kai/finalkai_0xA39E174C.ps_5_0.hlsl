// ---- Created with 3Dmigoto v1.3.16 on Fri Dec 19 22:54:35 2025
#include "../common.hlsl"
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

      renodx::draw::Config config = renodx::draw::BuildConfig();

      if (RENODX_SCENE_ALREADY_TONEMAPPED == 0.f) {
        r0.rgb = ToneMapLMSHueShift(r0.rgb); // LMS or hue-shift? 
      }

      if (RENODX_GAMMA_CORRECTION == renodx::draw::GAMMA_CORRECTION_GAMMA_2_2) {
        r0.rgb = renodx::color::correct::GammaSafe(r0.rgb, false, 2.2f);
      } else if (RENODX_GAMMA_CORRECTION == renodx::draw::GAMMA_CORRECTION_GAMMA_2_4) {
        r0.rgb = renodx::color::correct::GammaSafe(r0.rgb, false, 2.4f);
      } else if (RENODX_GAMMA_CORRECTION == 3.f) {
        float gamma_value = 2.20000005;
        r0.rgb = renodx::color::correct::GammaSafe(r0.rgb, false, gamma_value);
      }

      o0 = r0;
      float3 color = o0.rgb;

      // color = SE_Saturation(color);

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

      // Gamut Compression
      color = renodx::color::bt2020::from::BT709(color);
      float grayscale = renodx::color::convert::Luminance(color, renodx::color::convert::COLOR_SPACE_BT2020);
      const float MID_GRAY_LINEAR = 1 / (pow(10, 0.75));                          // ~0.18f
      const float MID_GRAY_PERCENT = 0.5f;                                        // 50%
      const float MID_GRAY_GAMMA = log(MID_GRAY_LINEAR) / log(MID_GRAY_PERCENT);  // ~2.49f
      float encode_gamma = MID_GRAY_GAMMA;
      float3 encoded = renodx::color::gamma::EncodeSafe(color, encode_gamma);
      float encoded_gray = renodx::color::gamma::Encode(grayscale, encode_gamma);
      float3 compressed = renodx::color::correct::GamutCompress(encoded, encoded_gray);
      color = renodx::color::gamma::DecodeSafe(compressed, encode_gamma);
      color = max(0.f, color);
      color = renodx::color::bt709::from::BT2020(color);

      o0.rgb = color;

      float scale_value = RENODX_DIFFUSE_WHITE_NITS;

      if (RENODX_SCENE_ALREADY_TONEMAPPED) {
        scale_value = RENODX_GRAPHICS_WHITE_NITS;
      }

      if (shader_injection.hdr_format == 1.f) {
        o0.rgb *= scale_value / 80.f;
      } else {
        o0.rgb = renodx::color::bt2020::from::BT709(o0.rgb);
        o0.rgb = renodx::color::pq::EncodeSafe(o0.rgb, scale_value);
      }

      o0.w = 1;
    }
    else {
      r0.w = gamma * 2.20000005;
      r0.rgb = renodx::math::SafePow(r0.rgb, r0.w);
      // r0.xyz = log2(r0.xyz);
      // r0.xyz = r0.www * r0.xyz;
      // r0.xyz = exp2(r0.xyz);]

      if (shader_injection.hdr_format == 1.f) {
        // o0.rgb *= RENODX_DIFFUSE_WHITE_NITS / 80.f;
        r0.xyz = hdr_peak_brightness * r0.xyz;
        r0.xyz = max(float3(0, 0, 0), r0.xyz);
        o0.xyz = min(float3(200, 200, 200), r0.xyz);
      } else {

        float diffuse_white = hdr_peak_brightness * 80.f;
        o0.rgb = r0.rgb;
        o0.rgb = renodx::color::bt2020::from::BT709(o0.rgb);
        o0.rgb = renodx::color::pq::EncodeSafe(o0.rgb, diffuse_white);
      }

      
      o0.w = 1;
    }
    return;
  } else {
    r0.xyz = tex.SampleLevel(smpl_s, v1.xy, 0).xyz;
    // r0.xyz = saturate(r0.xyz);
    // r0.xyz = log2(r0.xyz);
    // r0.xyz = gamma * r0.xyz;
    // o0.xyz = exp2(r0.xyz);
    r0.rgb = renodx::math::SafePow(r0.rgb, gamma);

    if (RENODX_TONE_MAP_TYPE > 0) {
      r0.rgb = renodx::color::srgb::DecodeSafe(r0.rgb);

      renodx::draw::Config config = renodx::draw::BuildConfig();

      if (RENODX_SCENE_ALREADY_TONEMAPPED == 0.f) {
        r0.rgb = ToneMapLMS(r0.rgb);
      }

      if (RENODX_GAMMA_CORRECTION == renodx::draw::GAMMA_CORRECTION_GAMMA_2_2) {
        r0.rgb = renodx::color::correct::GammaSafe(r0.rgb, false, 2.2f);
      } else if (RENODX_GAMMA_CORRECTION == renodx::draw::GAMMA_CORRECTION_GAMMA_2_4) {
        r0.rgb = renodx::color::correct::GammaSafe(r0.rgb, false, 2.4f);
      } else if (RENODX_GAMMA_CORRECTION == 3.f) {
        r0.rgb = renodx::color::correct::GammaSafe(r0.rgb, false, 2.2f);
      }

      o0 = r0;
      float3 color = o0.rgb;

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

      // Gamut Compression
      color = renodx::color::bt2020::from::BT709(color);
      float grayscale = renodx::color::convert::Luminance(color, renodx::color::convert::COLOR_SPACE_BT2020);
      const float MID_GRAY_LINEAR = 1 / (pow(10, 0.75));                          // ~0.18f
      const float MID_GRAY_PERCENT = 0.5f;                                        // 50%
      const float MID_GRAY_GAMMA = log(MID_GRAY_LINEAR) / log(MID_GRAY_PERCENT);  // ~2.49f
      float encode_gamma = MID_GRAY_GAMMA;
      float3 encoded = renodx::color::gamma::EncodeSafe(color, encode_gamma);
      float encoded_gray = renodx::color::gamma::Encode(grayscale, encode_gamma);
      float3 compressed = renodx::color::correct::GamutCompress(encoded, encoded_gray);
      color = renodx::color::gamma::DecodeSafe(compressed, encode_gamma);
      color = max(0.f, color);
      color = renodx::color::bt709::from::BT2020(color);

      o0.rgb = color;

      float scale_value = RENODX_DIFFUSE_WHITE_NITS;

      if (RENODX_SCENE_ALREADY_TONEMAPPED) {
        scale_value = RENODX_GRAPHICS_WHITE_NITS;
      }

      if (shader_injection.hdr_format == 1.f) {
        o0.rgb *= scale_value / 80.f;
      } else {
        o0.rgb = renodx::color::bt2020::from::BT709(o0.rgb);
        o0.rgb = renodx::color::pq::EncodeSafe(o0.rgb, scale_value);
      }

      o0.w = 1;
    }

    o0.w = 1;
    return;
  }
  return;
}