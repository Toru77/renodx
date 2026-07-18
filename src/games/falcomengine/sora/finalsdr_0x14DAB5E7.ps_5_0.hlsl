// ---- Created with 3Dmigoto v1.3.16 on Fri Jan 02 23:13:35 2026
#include "../common.hlsl"
cbuffer Constants : register(b0)
{
  float gamma : packoffset(c0);
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

  r0.xyz = tex.SampleLevel(smpl_s, v1.xy, 0).xyz;
  // r0.xyz = saturate(r0.xyz);
  // r0.xyz = log2(r0.xyz);
  // r0.xyz = gamma * r0.xyz;
  // o0.xyz = exp2(r0.xyz);
  // ignore gamma 
  r0.rgb = renodx::color::srgb::DecodeSafe(r0.rgb);

  // Swapchain Pass

  renodx::draw::Config config = renodx::draw::BuildConfig();

  if (RENODX_GAMMA_CORRECTION == renodx::draw::GAMMA_CORRECTION_GAMMA_2_2) {
    // r0.rgb = GammaCorrectHuePreserving(r0.rgb, 2.2f);
    r0.rgb = renodx::color::correct::GammaSafe(r0.rgb, false, 2.2f);
  } else if (RENODX_GAMMA_CORRECTION == renodx::draw::GAMMA_CORRECTION_GAMMA_2_4) {
    // r0.rgb = GammaCorrectHuePreserving(r0.rgb, 2.4f);
    r0.rgb = renodx::color::correct::GammaSafe(r0.rgb, false, 2.4f);
  } else if (RENODX_GAMMA_CORRECTION == 3.f) {
    // r0.rgb = GammaCorrectHuePreserving(r0.rgb, 2.3f);

    // float gamma_value = gamma * 2.20000005;
    float gamma_value = 2.3f;
    r0.rgb = renodx::color::correct::GammaSafe(r0.rgb, false, gamma_value);
  }

  r0.rgb = ToneMapLMS(r0.rgb);

  float3 color = r0.rgb;

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

  if (shader_injection.hdr_format == 1.f) {
    o0.rgb *= RENODX_DIFFUSE_WHITE_NITS / 80.f;
  } else {
    o0.rgb = renodx::color::bt2020::from::BT709(o0.rgb);
    o0.rgb = renodx::color::pq::EncodeSafe(o0.rgb, RENODX_DIFFUSE_WHITE_NITS);
  }

  o0.w = 1;
  return;
}