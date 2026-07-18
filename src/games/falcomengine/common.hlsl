
#include "./shared.h"
#include "lms_matrix.hlsl"
#include "./macleod_boynton.hlsli"

float3 srgbDecode(float3 color) {
  if (RENODX_TONE_MAP_TYPE == 0 || shader_injection.bloom == 0.f) {
    return color;
  }

  return renodx::color::srgb::DecodeSafe(color);
}

float3 srgbEncode(float3 color) {
  if (RENODX_TONE_MAP_TYPE == 0 || shader_injection.bloom == 0.f) {
    return color;
  }

  return renodx::color::srgb::EncodeSafe(color);
}

float calculateLuminanceSRGB(float3 color) {
  if (RENODX_TONE_MAP_TYPE == 0 || shader_injection.bloom == 0.f) {
    return renodx::color::y::from::NTSC1953(color);
  }

  return renodx::color::y::from::BT709(renodx::color::srgb::DecodeSafe(color));
}

float3 compress(float3 color) {
  return saturate(color);

}

float3 addBloom(float3 base, float3 blend) {
  float3 addition = renodx::math::SafeDivision(blend, (1.f + base), 0.f);

  return base + addition;
}

float3 hdrScreenBlend(float3 base, float3 blend, float scale = 0.f) {
  blend = max(0.f, blend);

  blend *= shader_injection.bloom_strength;
  // blend *= 8.f;

  float3 bloom_texture = blend;

  float mid_gray_bloomed = (0.18 + renodx::color::y::from::BT709(bloom_texture)) / 0.18;

  float scene_luminance = renodx::color::y::from::BT709(base) * mid_gray_bloomed;
  float bloom_blend = saturate(smoothstep(0.f, 0.18f, scene_luminance));

  float3 bloom_scaled = lerp(float3(0.f, 0.f, 0.f), bloom_texture, bloom_blend);  // = bloom_blend
  bloom_texture = lerp(bloom_texture, bloom_scaled, 0.5f);

  blend = bloom_texture;

  return addBloom(base, blend);
}

float3 hdrScreenBlend3(float3 base, float3 blend, float scale = 0.f) {
  blend = max(0.f, blend);

  base = renodx::color::srgb::EncodeSafe(base);
  blend = renodx::color::srgb::EncodeSafe(blend);

  float3 bloom = base * blend;

  float3 sdr = blend * (max(1 - base, 0.f)) + base;

  bloom = renodx::color::srgb::DecodeSafe(bloom); 
  sdr = renodx::color::srgb::DecodeSafe(sdr);

  float strength = shader_injection.bloom_hue_correction;
  // bloom = CorrectHueMB(bloom, sdr, saturate(strength));

  // bloom = lerp(bloom, CorrectHueAndPurityMBFullStrength(bloom, sdr), saturate(strength));

  return bloom;
  
  // return addBloom(base, blend);
}


float3 LMS_Processing(float3 color) {
  color = LMS_Vibrancy(color, shader_injection.tone_map_lms_vibrancy, shader_injection.tone_map_lms_contrast);
  color = lerp(color, CastleDechroma_CVVDPStyle_NakaRushton(color), shader_injection.tone_map_lms_dechroma);

  color = renodx::color::bt709::clamp::BT2020(color);
  return color;
}


float3 GammaCorrectHuePreserving(float3 incorrect_color, float gamma = 2.2f) {
  float3 ch = renodx::color::correct::GammaSafe(incorrect_color, false, gamma);

  const float y_in = renodx::color::y::from::BT709(incorrect_color);
  const float y_out = max(0, renodx::color::correct::Gamma(y_in, false, gamma));

  float3 lum = incorrect_color * (y_in > 0 ? y_out / y_in : 0.f);

  // use chrominance from channel gamma correction and apply hue shifting from per channel tonemap
  float3 result = CorrectPurityMBBT709WithBT2020(lum, ch);

  return result;
}



float3 ToneMapLMS(float3 untonemapped) {
  if (RENODX_TONE_MAP_TYPE == 0.f) {
    return untonemapped;
  }

  renodx::draw::Config config = renodx::draw::BuildConfig();
  float3 untonemapped_graded = untonemapped;

  // untonemapped_graded = LMS_Vibrancy(untonemapped_graded, shader_injection.tone_map_lms_vibrancy, shader_injection.tone_map_lms_contrast);

  // naka rushton
  float3 untonemapped_graded_dechroma = CastleDechroma_CVVDPStyle_NakaRushton(untonemapped_graded);
  untonemapped_graded_dechroma = lerp(untonemapped_graded, untonemapped_graded_dechroma, shader_injection.tone_map_lms_dechroma);

  float3 bt709_tonemapped;
  float peak_ratio = RENODX_PEAK_WHITE_NITS / RENODX_DIFFUSE_WHITE_NITS;

  if (RENODX_GAMMA_CORRECTION == renodx::draw::GAMMA_CORRECTION_GAMMA_2_2) {
    peak_ratio = renodx::color::correct::Gamma(peak_ratio, true, 2.2f);

  } else if (RENODX_GAMMA_CORRECTION == renodx::draw::GAMMA_CORRECTION_GAMMA_2_4) {
    peak_ratio = renodx::color::correct::Gamma(peak_ratio, true, 2.4f);
  }

  if (RENODX_TONE_MAP_TYPE == 1.f) {
    float contrast_ratio = min(peak_ratio, 4.f);

  //   bt709_tonemapped = renodx::tonemap::psycho::psychotm_test11(
  //       untonemapped_graded_dechroma,
  //       peak_ratio,  // peak
  //       1.0f,        // exposure
  //       1.0f,        // highlights
  //       1.0f,        // shadows
  //       1.0f,        // contrast
  //       1.0f,        // purity_scale
  //       1.0f,        // bleaching_intensity
  //       100.f,       // clip_point
  //       0.5f,        // hue_restore
  //       1.0f,        // adaptation_contrast
  //       1,           // naka rushton
  //       // 1.0f + 0.025 * (contrast_ratio - 1.0f));  // cone_response_exponent
  //       1.0f);  // cone_response_exponent
  // }

    float contrast = shader_injection.tone_map_lms_contrast / shader_injection.tone_map_lms_vibrancy;

    bt709_tonemapped = renodx::tonemap::psychov::psychotm_test17(
        untonemapped_graded_dechroma,
        peak_ratio,                               // peak
        1.0f,                                     // exposure
        1.0f,                                     // highlights
        1.0f,                                     // shadows
        contrast,                                 // contrast
        1.0f,                                     // purity_scale
        1.0f,                                     // bleaching_intensity
        100.f,                                    // clip_point
        0.5f,                                     // hue_restore
        1.0f,                                     // adaptation_contrast
        1,                                        // naka rushton
        shader_injection.tone_map_lms_vibrancy);  // cone_response_exponent
  } else {
    untonemapped_graded_dechroma = LMS_Vibrancy(untonemapped_graded_dechroma, shader_injection.tone_map_lms_vibrancy, shader_injection.tone_map_lms_contrast);

    renodx::draw::Config config = renodx::draw::BuildConfig();
    config.tone_map_hue_correction = 0.f;
    config.tone_map_hue_shift = 0.f;
    config.tone_map_type = 3.f;  // RenoDRT otherwise its ACES
    config.tone_map_per_channel = 1.f;
    config.reno_drt_tone_map_method = shader_injection.renodrt_tone_map_type;

    float3 bt709_tonemapped_per_channel = renodx::draw::ToneMapPass(untonemapped_graded_dechroma, config);

    config = renodx::draw::BuildConfig();
    config.tone_map_hue_correction = 0.f;
    config.tone_map_hue_shift = 0.f;
    config.tone_map_type = 3.f;  // RenoDRT otherwise its ACES
    config.tone_map_per_channel = 0.f;
    config.reno_drt_tone_map_method = shader_injection.renodrt_tone_map_type;

    bt709_tonemapped = renodx::draw::ToneMapPass(untonemapped_graded_dechroma, config);

    bt709_tonemapped = CorrectHueAndPurityMBFullStrength(bt709_tonemapped, bt709_tonemapped_per_channel);

    // renodx::tonemap::renodrt::config::tone_map_method
  }

  return bt709_tonemapped;
}

float3 ToneMapLMSHueShift(float3 untonemapped) {
  if (RENODX_TONE_MAP_TYPE == 0.f) {
    return untonemapped;
  }

  renodx::draw::Config config = renodx::draw::BuildConfig();
  float3 untonemapped_graded = untonemapped;

  // untonemapped_graded = LMS_Vibrancy(untonemapped_graded, shader_injection.tone_map_lms_vibrancy, shader_injection.tone_map_lms_contrast);

  // naka rushton
  float3 untonemapped_graded_dechroma = CastleDechroma_CVVDPStyle_NakaRushton(untonemapped_graded);
  untonemapped_graded_dechroma = lerp(untonemapped_graded, untonemapped_graded_dechroma, shader_injection.tone_map_lms_dechroma);

  float3 bt709_tonemapped;
  float peak_ratio = RENODX_PEAK_WHITE_NITS / RENODX_DIFFUSE_WHITE_NITS;

  if (RENODX_GAMMA_CORRECTION == renodx::draw::GAMMA_CORRECTION_GAMMA_2_2) {
    peak_ratio = renodx::color::correct::Gamma(peak_ratio, true, 2.2f);

  } else if (RENODX_GAMMA_CORRECTION == renodx::draw::GAMMA_CORRECTION_GAMMA_2_4) {
    peak_ratio = renodx::color::correct::Gamma(peak_ratio, true, 2.4f);
  }

  if (RENODX_TONE_MAP_TYPE == 1.f) {
    float contrast_ratio = min(peak_ratio, 4.f);

    untonemapped_graded_dechroma = LMS_Vibrancy(untonemapped_graded_dechroma, shader_injection.tone_map_lms_vibrancy, shader_injection.tone_map_lms_contrast);

    float3 sdr = renodx::tonemap::neutwo::BT709(untonemapped_graded_dechroma);

    renodx::draw::Config config = renodx::draw::BuildConfig();
    config.tone_map_hue_correction = 0.f;
    config.tone_map_hue_shift = 0.f;
    config.tone_map_type = 3.f;  // RenoDRT otherwise its ACES
    config.tone_map_per_channel = 2.f;
    config.reno_drt_tone_map_method = renodx::tonemap::renodrt::config::tone_map_method::NEUTWO;

    float3 bt709_tonemapped_max_channel = renodx::draw::ToneMapPass(untonemapped_graded_dechroma, config);

    float contrast = shader_injection.tone_map_lms_contrast / shader_injection.tone_map_lms_vibrancy;
    float cone_response = 1.0f;

    bt709_tonemapped = renodx::tonemap::psychov::psychotm_test17(
        untonemapped_graded_dechroma,
        peak_ratio,                               // peak
        1.0f,                                     // exposure
        1.0f,                                     // highlights
        1.0f,                                     // shadows
        1.f,                                 // contrast
        1.0f,                                     // purity_scale
        1.0f,                                     // bleaching_intensity
        100.f,                                    // clip_point
        1.0f,                                     // hue_restore
        1.0f,                                     // adaptation_contrast
        1,                                        // naka rushton
        1.f);  // cone_response_exponent

    float strength = shader_injection.bloom_hue_correction;

    // bt709_tonemapped = CorrectHueAndPurityMBGated(bt709_tonemapped, bt709_tonemapped_max_channel, strength, 0.18f, 1.f, strength);
    // bt709_tonemapped = CorrectHueAndPurityMBFullStrength(bt709_tonemapped, bt709_tonemapped_max_channel);

  } else {
    untonemapped_graded_dechroma = LMS_Vibrancy(untonemapped_graded_dechroma, shader_injection.tone_map_lms_vibrancy, shader_injection.tone_map_lms_contrast);

    renodx::draw::Config config = renodx::draw::BuildConfig();
    config.tone_map_hue_correction = 0.f;
    config.tone_map_hue_shift = 0.f;
    config.tone_map_type = 3.f;  // RenoDRT otherwise its ACES
    config.tone_map_per_channel = 2.f;
    config.reno_drt_tone_map_method = shader_injection.renodrt_tone_map_type;

    float3 bt709_tonemapped_max_channel = renodx::draw::ToneMapPass(untonemapped_graded_dechroma, config);

    // config = renodx::draw::BuildConfig();
    // config.tone_map_hue_correction = 0.f;
    // config.tone_map_hue_shift = 0.f;
    // config.tone_map_type = 3.f;  // RenoDRT otherwise its ACES
    // config.tone_map_per_channel = 0.f;
    // config.reno_drt_tone_map_method = shader_injection.renodrt_tone_map_type;

    // bt709_tonemapped = renodx::draw::ToneMapPass(untonemapped_graded_dechroma, config);

    bt709_tonemapped = bt709_tonemapped_max_channel;

    // renodx::tonemap::renodrt::config::tone_map_method
  }

  return bt709_tonemapped;
}


float3 processAndToneMap(float3 color, bool decoding = true) {
  if (decoding) {
    color = renodx::color::srgb::DecodeSafe(color);
  }

  color = ToneMapLMS(color);
  // color = correctHue(color, sdr_color);
  color = renodx::color::bt709::clamp::BT2020(color);

  [branch]
  if (RENODX_GAMMA_CORRECTION == renodx::draw::GAMMA_CORRECTION_GAMMA_2_2) {
    // color = renodx::color::correct::GammaSafe(color, false, 2.2f);
    color = GammaCorrectHuePreserving(color, 2.2f);
  } else if (RENODX_GAMMA_CORRECTION == renodx::draw::GAMMA_CORRECTION_GAMMA_2_4) {
    // color = renodx::color::correct::GammaSafe(color, false, 2.4f);
    color = GammaCorrectHuePreserving(color, 2.4f);
  } else if (RENODX_GAMMA_CORRECTION == 3.f) {
    color = GammaCorrectHuePreserving(color, 2.3f); 
    // color = renodx::color::correct::GammaSafe(color, false, 2.3f);
  }

  // This is RenderIntermediatePass, simply brightness scaling and srgb encoding
  color *= RENODX_DIFFUSE_WHITE_NITS / RENODX_GRAPHICS_WHITE_NITS;

  [branch]
  if (RENODX_GAMMA_CORRECTION == renodx::draw::GAMMA_CORRECTION_GAMMA_2_2) {
    color = renodx::color::correct::GammaSafe(color, true, 2.2f);
  } else if (RENODX_GAMMA_CORRECTION == renodx::draw::GAMMA_CORRECTION_GAMMA_2_4) {
    color = renodx::color::correct::GammaSafe(color, true, 2.4f);
  } else if (RENODX_GAMMA_CORRECTION == 3.f) {
    color = renodx::color::correct::GammaSafe(color, true, 2.3f);
  }

  color = renodx::color::srgb::EncodeSafe(color);
  return color;
}

float3 processUI(float3 color, bool decoding = true) {
  if (decoding) {
    color = renodx::color::srgb::DecodeSafe(color);
  }

  [branch]
  if (RENODX_GAMMA_CORRECTION == renodx::draw::GAMMA_CORRECTION_GAMMA_2_2) {
    color = renodx::color::correct::GammaSafe(color, false, 2.2f);
  } else if (RENODX_GAMMA_CORRECTION == renodx::draw::GAMMA_CORRECTION_GAMMA_2_4) {
    color = renodx::color::correct::GammaSafe(color, false, 2.4f);
  } else if (RENODX_GAMMA_CORRECTION == 3.f) {
    color = renodx::color::correct::GammaSafe(color, false, 2.3f);
  }

  // This is RenderIntermediatePass, simply brightness scaling and srgb encoding
  // color *= RENODX_DIFFUSE_WHITE_NITS / RENODX_GRAPHICS_WHITE_NITS;
  color *= RENODX_GRAPHICS_WHITE_NITS / RENODX_DIFFUSE_WHITE_NITS;
   
  [branch]
  if (RENODX_GAMMA_CORRECTION == renodx::draw::GAMMA_CORRECTION_GAMMA_2_2) {
    color = renodx::color::correct::GammaSafe(color, true, 2.2f);
  } else if (RENODX_GAMMA_CORRECTION == renodx::draw::GAMMA_CORRECTION_GAMMA_2_4) {
    color = renodx::color::correct::GammaSafe(color, true, 2.4f);
  } else if (RENODX_GAMMA_CORRECTION == 3.f) {
    color = renodx::color::correct::GammaSafe(color, true, 2.3f);
  }

  color = renodx::color::srgb::EncodeSafe(color); 
  return color;
}

float3 ToneMapAndSwapchainPass(float3 r0) {
  // Swapchain Pass

  float3 o0;
  float3 color = r0.rgb;
  renodx::draw::Config config = renodx::draw::BuildConfig();

  // if (RENODX_GAMMA_CORRECTION == renodx::draw::GAMMA_CORRECTION_GAMMA_2_2) {
  //   r0.rgb = GammaCorrectHuePreserving(r0.rgb, 2.2f);
  // } else if (RENODX_GAMMA_CORRECTION == renodx::draw::GAMMA_CORRECTION_GAMMA_2_4) {
  //   r0.rgb = GammaCorrectHuePreserving(r0.rgb, 2.4f);
  // } else if (RENODX_GAMMA_CORRECTION == 3.f) {
  //   r0.rgb = GammaCorrectHuePreserving(r0.rgb, 2.3f);
  // }

  // Lazy tonemap: skip if HUD trigger already applied it this frame.
  if (RENODX_SCENE_ALREADY_TONEMAPPED == 0.f) {
    color = ToneMapLMS(color);
  }

  [branch]
  if (RENODX_GAMMA_CORRECTION == renodx::draw::GAMMA_CORRECTION_GAMMA_2_2) {
    color = renodx::color::correct::GammaSafe(color, false, 2.2f);
  } else if (RENODX_GAMMA_CORRECTION == renodx::draw::GAMMA_CORRECTION_GAMMA_2_4) {
    color = renodx::color::correct::GammaSafe(color, false, 2.4f);
  } else if (RENODX_GAMMA_CORRECTION == 3.f) {
    color = renodx::color::correct::GammaSafe(color, false, 2.3f);
  }

  

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

  return o0.rgb;
}

float3 SE_Saturation(float3 color) {
  float4 r1;
  float4 r0;

  r0.rgb = color;

  r0.w = 0.587700009 * r0.y;
  r0.w = r0.x * 1.66050005 + -r0.w;
  r1.x = -r0.z * 0.072800003 + r0.w;
  r0.w = 0.100599997 * r0.y;
  r0.w = r0.x * -0.0182000007 + -r0.w;
  r1.z = r0.z * 1.11870003 + r0.w;
  r0.x = dot(r0.xy, float2(-0.124600001, 1.13300002));
  r1.y = -r0.z * 0.0083999997 + r0.x;

  return r1.rgb;
}
