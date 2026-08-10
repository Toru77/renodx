#include "../shared.h"
#include "../macleod_boynton.hlsli"
#include "../lms_matrix.hlsl"

#define FLT16_MAX 65504.f
#define FLT_MIN   asfloat(0x00800000)  // 1.175494351e-38f
#define FLT_MAX   asfloat(0x7F7FFFFF)  // 3.402823466e+38f


float3 decodeColor(float3 color, bool srgb = true) {
  if (srgb) {
    return renodx::color::srgb::DecodeSafe(color);
  } else {
    return renodx::color::gamma::DecodeSafe(color, 2.2f);
  }

}


float3 encodeColor(float3 color, bool srgb = true) {
  if (srgb) {
    return renodx::color::srgb::EncodeSafe(color);
  } else {
    return renodx::color::gamma::EncodeSafe(color, 2.2f);
  }

}

float3 processColorBuffer(float3 color) {

  color = max(color, 0.f);

  return color;
}

float4 processColorBuffer(float4 color) {

  color.w = saturate(color.w);

  return color;
}

float3 processBloomBuffer(float3 color) {

  color = max(color, 0.f);


  return color;
}

float4 processBloomBuffer(float4 color) {
  // color.rgb = processColorBuffer(color.rgb);
  color.w = saturate(color.w);  

  return color;
}

float3 postProcessBloomBuffer(float3 color) {
  
  return color;
}


float3 ToneMap(float3 color) {
  float3 originalColor = color;

  if (RENODX_TONE_MAP_TYPE == 0.f) {
    return saturate(color);

  } else {
    float peak_ratio = RENODX_PEAK_WHITE_NITS / RENODX_DIFFUSE_WHITE_NITS;

    if (RENODX_GAMMA_CORRECTION == renodx::draw::GAMMA_CORRECTION_GAMMA_2_2) {
      peak_ratio = renodx::color::correct::Gamma(peak_ratio, true, 2.2f);

    } else if (RENODX_GAMMA_CORRECTION == renodx::draw::GAMMA_CORRECTION_GAMMA_2_4) {
      peak_ratio = renodx::color::correct::Gamma(peak_ratio, true, 2.4f);
    }

    if (RENODX_TONE_MAP_TYPE == 1.f) {
      float contrast = shader_injection.tone_map_contrast / shader_injection.tone_map_saturation;

      color = renodx::tonemap::psychov::psychotm_test22(
          color,
          peak_ratio,                             // peak
          1.0f,                                   // exposure
          1.0f,                                   // highlights
          1.0f,                                   // shadows
          contrast,                               // contrast
          1.0f,                                   // purity_scale
          1.0f,                                   // bleaching_intensity
          100.f,                                  // clip_point
          0.5f,                                   // hue_restore
          1.0f,                                   // adaptation_contrast
          1,                                      // naka rushton
          shader_injection.tone_map_saturation);  // cone_response_exponent

    } else {
      float3 untonemapped_graded_dechroma = LMS_Vibrancy(color, shader_injection.tone_map_saturation, shader_injection.tone_map_contrast);
      // float3 untonemapped_graded_dechroma = color;

      if (shader_injection.tone_map_hue_correction_method == 0.f) {
        color = renodx::tonemap::ReinhardScalableExtended(
            color,
            100.f,
            peak_ratio,
            0,
            0.18f,
            0.18f);

      } else if (shader_injection.tone_map_hue_correction_method == 1.f) {
        color = renodx::tonemap::HermiteSplinePerChannelRolloff(
            color,
            peak_ratio,
            100.f);

      } else {
        color = renodx::tonemap::neutwo::PerChannel(
            color,
            peak_ratio,
            max(100.f, peak_ratio));
      }

      // renodx::draw::Config config = renodx::draw::BuildConfig();
      // config.tone_map_hue_correction = 0.f;
      // config.tone_map_hue_shift = 0.f;
      // config.tone_map_type = 3.f;  // RenoDRT otherwise its ACES
      // config.tone_map_per_channel = 0.f;
      // config.reno_drt_tone_map_method = shader_injection.renodrt_tone_map_type;
      // config.peak_white_nits = RENODX_PEAK_WHITE_NITS;
      // config.diffuse_white_nits = RENODX_DIFFUSE_WHITE_NITS;
      // config.graphics_white_nits = RENODX_GRAPHICS_WHITE_NITS;

      // float3 bt709_tonemapped_per_channel = renodx::draw::ToneMapPass(untonemapped_graded_dechroma, config);

      // config = renodx::draw::BuildConfig();
      // config.tone_map_hue_correction = 0.f;
      // config.tone_map_hue_shift = 0.f;
      // config.tone_map_type = 3.f;  // RenoDRT otherwise its ACES
      // config.tone_map_per_channel = 0.f;
      // config.reno_drt_tone_map_method = shader_injection.renodrt_tone_map_type;

      // float3 bt709_tonemapped = renodx::draw::ToneMapPass(untonemapped_graded_dechroma, config);

      // bt709_tonemapped = CorrectHueAndPurityMBFullStrength(bt709_tonemapped, bt709_tonemapped_per_channel);

    }

    

    return color;
  }
  // else if (shader_injection.tone_map_type == 1.f) {
  //   color = LMS_ToneMap_Stockman(color, 1.f,
  //                                1.f);
  //   color = FrostbiteToneMap(color);

  //   return color;
  // } else if (shader_injection.tone_map_type == 2.f) {
  //   color = UserColorGrading(
  //       color,
  //       RENODX_TONE_MAP_EXPOSURE,    // exposure
  //       RENODX_TONE_MAP_HIGHLIGHTS,  // highlights
  //       RENODX_TONE_MAP_SHADOWS,     // shadows
  //       RENODX_TONE_MAP_CONTRAST,    // contrast
  //       1.f,                         // saturation, we'll do this post-tonemap
  //       0.f);                        // dechroma, post tonemapping
  //                                    // hue correction, Post tonemapping

  //   color = LMS_ToneMap_Stockman(color, 1.f,
  //                                1.f);
  //   color = DICEToneMap(color);

  //   color = UserColorGrading(
  //       color,
  //       1.f,                         // exposure
  //       1.f,                         // highlights
  //       1.f,                         // shadows
  //       1.f,                         // contrast
  //       RENODX_TONE_MAP_SATURATION,  // saturation
  //       RENODX_TONE_MAP_BLOWOUT      // dechroma, we don't need it
  //   );

  //   return color;

  // }
  // else if (shader_injection.tone_map_type == 3.f) {
  //   color = UserColorGrading(
  //       color,
  //       RENODX_TONE_MAP_EXPOSURE,    // exposure
  //       RENODX_TONE_MAP_HIGHLIGHTS,  // highlights
  //       RENODX_TONE_MAP_SHADOWS,     // shadows
  //       RENODX_TONE_MAP_CONTRAST,    // contrast
  //       1.f,                         // saturation, we'll do this post-tonemap
  //       0.f);                        // dechroma, post tonemapping
  //                                    // hue correction, Post tonemapping

  //   color = LMS_ToneMap_Stockman(color, 1.f,
  //                                1.f);

  //   float peak = RENODX_PEAK_WHITE_NITS / RENODX_DIFFUSE_WHITE_NITS;

  //   color = renodx::color::bt709::clamp::BT2020(color);

  //   float3 lum_color = renodx::tonemap::HermiteSplineLuminanceRolloff(color, peak);
  //   // float3 perch_color = renodx::tonemap::HermiteSplinePerChannelRolloff(color, peak);

  //   // if (RENODX_TONE_MAP_HUE_CORRECTION > 0.f)
  //   //   color = renodx::color::correct::Chrominance(lum_color, perch_color, RENODX_TONE_MAP_HUE_CORRECTION);
  //   // else
  //   color = lum_color;

  //   color = UserColorGrading(
  //       color,
  //       1.f,                         // exposure
  //       1.f,                         // highlights
  //       1.f,                         // shadows
  //       1.f,                         // contrast
  //       RENODX_TONE_MAP_SATURATION,  // saturation
  //       RENODX_TONE_MAP_BLOWOUT      // dechroma, we don't need it
  //   );

  //   return color;
  // } else if (shader_injection.tone_map_type == 4.f) {
  //   color = UserColorGrading(
  //       color,
  //       RENODX_TONE_MAP_EXPOSURE,    // exposure
  //       RENODX_TONE_MAP_HIGHLIGHTS,  // highlights
  //       RENODX_TONE_MAP_SHADOWS,     // shadows
  //       RENODX_TONE_MAP_CONTRAST,    // contrast
  //       1.f,                         // saturation, we'll do this post-tonemap
  //       0.f);                        // dechroma, post tonemapping
  //                                    // hue correction, Post tonemapping

  //   color = LMS_ToneMap_Stockman(color, 1.f,
  //                                1.f);

  //   // bool pq = (RENODX_TONE_MAP_WORKING_COLOR_SPACE > 0.f);
  //   bool pq = false;
  //   float shoulder_start = 0.33333330f;  // borrow from DICE
  //   float peak_nits = RENODX_PEAK_WHITE_NITS / RENODX_DIFFUSE_WHITE_NITS;
  //   float scene_nits = 4000.f / 203.f;
  //   color = MassEffectDisplayMap(color, shoulder_start, peak_nits, scene_nits);

  //   color = renodx::color::grade::UserColorGrading(
  //       color,
  //       1.f,                         // exposure
  //       1.f,                         // highlights
  //       1.f,                         // shadows
  //       1.f,                         // contrast
  //       RENODX_TONE_MAP_SATURATION,  // saturation
  //       RENODX_TONE_MAP_BLOWOUT,     // dechroma, we don't need it
  //       0.f,                         // Hue Correction Strength
  //       color);                      // Hue Correction Type

  //   return color;
  // } else if (shader_injection.tone_map_type == 5.f) {
  //   color = UserColorGrading(
  //       color,
  //       RENODX_TONE_MAP_EXPOSURE,    // exposure
  //       RENODX_TONE_MAP_HIGHLIGHTS,  // highlights
  //       RENODX_TONE_MAP_SHADOWS,     // shadows
  //       RENODX_TONE_MAP_CONTRAST,    // contrast
  //       1.f,                         // saturation, we'll do this post-tonemap
  //       0.f);                        // dechroma, post tonemapping
  //                                    // hue correction, Post tonemapping

  //   color = LMS_ToneMap_Stockman(color, 1.f,
  //                                1.f);

  //   float peak_nits = RENODX_PEAK_WHITE_NITS / RENODX_DIFFUSE_WHITE_NITS;
  //   color = renodx::tonemap::neutwo::MaxChannel(color, peak_nits);

  //   color = renodx::color::grade::UserColorGrading(
  //       color,
  //       1.f,                         // exposure
  //       1.f,                         // highlights
  //       1.f,                         // shadows
  //       1.f,                         // contrast
  //       RENODX_TONE_MAP_SATURATION,  // saturation
  //       RENODX_TONE_MAP_BLOWOUT,     // dechroma, we don't need it
  //       0.f,                         // Hue Correction Strength
  //       color);                      // Hue Correction Type

  //   return color;
  // } else {
  //   return color;
  // }
}

float3 GammaCorrectHuePreserving(float3 incorrect_color, float gamma = 2.2f) {
  float3 ch = renodx::color::correct::GammaSafe(incorrect_color, false, gamma);

  // return ch;
  const float y_in = renodx::color::y::from::BT709(incorrect_color);
  const float y_out = max(0, renodx::color::correct::Gamma(y_in, false, gamma));

  float3 lum = incorrect_color * (y_in > 0 ? y_out / y_in : 0.f);

  // use chrominance from channel gamma correction and apply hue shifting from per channel tonemap
  // float3 result = renodx::color::correct::ChrominanceICtCp(lum, ch);
  // float3 result = renodx::color::correct::Chrominance(lum, ch);
  float3 result = CorrectPurityMBBT709WithBT2020(lum, ch);

  return result;
}

float3 processAndToneMap(float3 color) {

  color = ToneMap(color);
  [branch]
  if (RENODX_GAMMA_CORRECTION == renodx::draw::GAMMA_CORRECTION_GAMMA_2_2) {
    color = GammaCorrectHuePreserving(color, 2.2f);
    // color = renodx::color::correct::GammaSafe(color, false, 2.2f);
  } else if (RENODX_GAMMA_CORRECTION == renodx::draw::GAMMA_CORRECTION_GAMMA_2_4) {
    // color = renodx::color::correct::GammaSafe(color, false, 2.4f);
    color = GammaCorrectHuePreserving(color, 2.4f);
  } else if (RENODX_GAMMA_CORRECTION == 3.f) {
    // color = renodx::color::correct::GammaSafe(color, false, 2.3f);
    color = GammaCorrectHuePreserving(color, 2.3f);
  }

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





float calculateLuminanceSRGB(float3 color) {

  return renodx::color::y::from::BT709(renodx::color::srgb::DecodeSafe(color));

}


float calculateLuminance(float3 color) {

  return renodx::color::y::from::BT709(color);

}

float safePow(float x, float y)  {

  return renodx::math::SafePow(x, y);
}

float3 safePow(float3 x, float y)  {

  return renodx::math::SafePow(x, y);
}


float3 srgbDecode(float3 color) {

  if (RENODX_TONE_MAP_TYPE == 0 || shader_injection.bloom_processing_space == 0.f) {
    return color;
  }

  return renodx::color::srgb::DecodeSafe(color);
}

float3 srgbEncode(float3 color) {

  if (RENODX_TONE_MAP_TYPE == 0 || shader_injection.bloom_processing_space == 0.f) {
    return color;
  }

  return renodx::color::srgb::EncodeSafe(color);
}


float3 hdrScreenBlend(float3 base, float3 blend, float strength = 1.0f) {

  blend *= strength; 

  blend *= shader_injection.bloom_strength;

  base = max(0.f, base);
  blend = max(0.f, blend);

  float3 addition = renodx::math::SafeDivision(blend, (1.f + base), 0.f);
  // blending like this better. 
  float3 output = base + addition;


  return output;
  
}


float3 hdrBlend(float3 base, float3 blend, float strength = 1.0f) {

  return base + strength * blend / ( 1.f + base);
}

float3 fadingBlend(float3 base, float3 fadeColor, float strength) {

  fadeColor = max(0.f, fadeColor);

  return lerp(base, fadeColor, saturate(strength));
}

float3 filterBlend(float3 base, float3 filter, bool decode = true)  {

  if (decode)
    filter = decodeColor(filter);

  float3 add = base + filter;
  float3 blend = hdrScreenBlend(base, filter, 1.0f);

  return (add + blend) * 0.5f;
}

float3 lightToneMap(float3 color) {

  color = max(0.f, color);
  color = renodx::tonemap::dice::BT709(color, RENODX_PEAK_WHITE_NITS / RENODX_DIFFUSE_WHITE_NITS, 0.5f);
  return color;
}