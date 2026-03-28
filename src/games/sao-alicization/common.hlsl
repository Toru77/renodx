#include "./shared.h"

float3 T_ResolveLutAndGamma(float3 x, Texture2D<float4> ColorGradingLUT, SamplerState LinearClampSampler_s, float2 texcoord) {
  //color Untonemapped (source of HDR luminance)
  float3 colorU = x;

  //Return: redunant black color, prevents div 0
  if (max(max(colorU.r, colorU.g), colorU.b) <= 0.f) return 0.f;

  // colorU blowout
  // if (false) //debug
  {
    //custom perchannel blownout color from near SDR tonemap 
    const float sdrMax = 1.65f; //(arbitrary, just if it look nice!)
    const float sdrStart = 1.0f;
    float3 colorUBlow = renodx::tonemap::ReinhardPiecewise(colorU, sdrMax, sdrStart); //or whatever tonemap

    //strength
    const float hueStrength = shader_injection.custom_pblow_chue;
    const float chrominanceStrength = shader_injection.custom_pblow_csat;

    #if 1 //variant: customizable
      //rgb -> perceptual/uniform color space (e.g. OkLAB, ICtCp, JzAzBz, etc.)
      //"Ok" (does an Ok job, bruh), "l" (lightness), "ab" (hue components)
      float3 colorUBlowUcs = renodx::color::oklab::from::BT709(colorUBlow);
      float3 colorUUcs = renodx::color::oklab::from::BT709(colorU);

      //Hue correct
      float colorUUcsChrominance = length(colorUUcs.yz); //backup chromiance/saturation (length of vector) before hue shift
      colorUUcs.yz = lerp(colorUUcs.yz, colorUBlowUcs.yz, hueStrength); //lerp hue towards blown out version
      colorUUcs.yz *= renodx::math::SafeDivision(colorUUcsChrominance, length(colorUUcs.yz), 1); //restore chrominance

      //Chrominance correct
      float targetChrominance = length(colorUBlowUcs.yz); //target chrominance
      targetChrominance = lerp(colorUUcsChrominance, targetChrominance, chrominanceStrength); //apply strength
      colorUUcs.yz *= renodx::math::SafeDivision(targetChrominance, colorUUcsChrominance, 1); //set to blowout chrominance

      // perceptual -> rgb
      colorU = renodx::color::bt709::from::OkLab(colorUUcs);

    #elif 1 //variant: 100%

      float3 colorUBlowUcs = renodx::color::oklab::from::BT709(colorUBlow);
      float3 colorUUcs = renodx::color::oklab::from::BT709(colorU);
      colorUUcs.yz = colorUBlowUcs.yz; //fully take hue & chrominace from blowout
      colorU = renodx::color::bt709::from::OkLab(colorUUcs);

    #elif 1 //variant: lazy (slightly wasteful doing both seperately)

       renodx::color::correct::HueOKLab(colorU, colorUBlow, hueStrength);
       renodx::color::correct::ChrominanceOKLab(colorU, colorUBlow, chrominanceStrength);

    #endif
  }

  //color Neutral (will be used to sample LUT)
  float3 colorN = colorU; {
    //max channel tonemap (chrominance/hue preserving)
     float m = max(max(colorN.r, colorN.g), colorN.b); //max
     //float m = renodx::color::y::from::BT709(m); //luma

    //rolloff to 1
    float m1 = m; {
      #if 0 //variant: overshoot to allow more range coverage
        m1 = renodx::tonemap::Reinhard(m1, 2.5);
      #elif 0 //variant: with white clip to better fill LUT range
        m1 = renodx::tonemap::ReinhardExtended(m1, 10, 1);
      #elif 1 // variant: piecewise to maintain midgray
        m1 = renodx::tonemap::ReinhardPiecewiseExtended(m1, 5, 1, 0.18);
      #endif
      m1 = min(m1, 1); //clamp just in case
    }

    //scale
    colorN *= (m1 / m); //scale from m to m1, preserving chroma.
    // colorN = renodx::color::correct::Luminance(colorN, m, m1); //(same as above)
  }

  //LUT
   x = colorN; { //use colorN to sample LUT!
    x = renodx::color::gamma::Encode(x);  // to gamma
    x = x * (31.0 / 32.0) + 1.0 / 64.0; //pad
    x = ColorGradingLUT.Sample(LinearClampSampler_s, texcoord).xyz;
    x = renodx::color::gamma::Decode(x); //back to linear
     //x = renodx::color::srgb::Decode(x); //back to linear (alt. fix for parity with SDR sRGB)
  }

  // //debug compare
   ////if (texcoord.x < 1/2.) return colorN;
   ////else return x;

  //UpgradeToneMap() (Superimpose HDR luma onto SDR graded chroma, influenced by SDR luma delta)
  x = renodx::tonemap::UpgradeToneMap(colorU, colorN, x, RENODX_COLOR_GRADE_STRENGTH); //all linear

  //fix for parity with SDR sRGB
  //x = renodx::color::correct::Gamma(x, true);

  //ToneMapPass() (user color grading, gamma correction, HDR tonemap)
  renodx::draw::Config config = renodx::draw::BuildConfig(); {
    config.reno_drt_white_clip = 3000 / 203.;
    config.reno_drt_tone_map_method = renodx::tonemap::renodrt::config::tone_map_method::HERMITE_SPLINE;
    config.tone_map_per_channel = 0;
    config.tone_map_hue_correction = 0;
    config.tone_map_hue_shift = 0;
  }
  x = renodx::draw::ToneMapPass(x, config); //linear

  return x; //out linear
}

float3 T_RenderIntermediatePass(float3 x) {
  x = renodx::color::srgb::Decode(x);
  x = renodx::draw::RenderIntermediatePass(x);

  return x; //out intermediate encoding / scaling
}