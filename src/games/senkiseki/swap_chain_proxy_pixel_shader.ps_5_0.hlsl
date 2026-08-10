#include "./shared.h"
// #include "./common.hlsl"

Texture2D t0 : register(t0);
SamplerState s0 : register(s0);
float4 main(float4 vpos: SV_POSITION, float2 uv: TEXCOORD0)
    : SV_TARGET {
  // return SwapChainPass(t0.Sample(s0, uv));

  float4 r0 = t0.Sample(s0, uv);

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
    r0.rgb = renodx::color::correct::GammaSafe(r0.rgb, false, 2.3f);
  }

  float4 o0 = r0;
  float3 color = o0.rgb;

  [branch]
  if (config.swap_chain_custom_color_space == renodx::draw::COLOR_SPACE_CUSTOM_BT709D93) {
    color = renodx::color::bt709::from::BT709D93(color);
  } else if (config.swap_chain_custom_color_space == renodx::draw::COLOR_SPACE_CUSTOM_NTSCU) {
    color = renodx::color::bt709::from::BT601NTSCU(color);
  } else if (config.swap_chain_custom_color_space == renodx::draw::COLOR_SPACE_CUSTOM_NTSCJ) {
    color = renodx::color::bt709::from::ARIBTRB9(color);
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


  if (shader_injection.hdr_format == 0.f) {
    color = renodx::color::bt2020::from::BT709(color);
    color = renodx::color::pq::EncodeSafe(color, shader_injection.graphics_white_nits);
  }
  else {
    color *= shader_injection.graphics_white_nits / 80.f;
  }

  o0.rgb = color;

 
  o0.w = r0.w;

  return o0;
}
