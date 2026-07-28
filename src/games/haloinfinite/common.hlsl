#include "./shared.h"

float3 NeutwoStockmanSharpeLMS(float3 color_input, float peak, float clip = 100.f)
{
  float3 color_bt2020 = renodx::color::bt2020::from::BT709(color_input);
  float3 color_lms = renodx::color::lms::from::BT2020(color_bt2020);

  float3 peak_lms = renodx::color::lms::from::BT2020(peak.xxx);
  float3 clip_lms = renodx::color::lms::from::BT2020(clip.xxx);

  color_lms = renodx::tonemap::neutwo::PerChannel(max(color_lms, 0.f), peak_lms, clip_lms);
  color_bt2020 = renodx::color::bt2020::from::LMS(color_lms);

  return max(renodx::color::bt709::from::BT2020(color_bt2020), 0.f);
}

float3 ReinhardStockmanSharpeLMS(float3 sceneColor, float peak)
{
  float3 color_bt2020 = renodx::color::bt2020::from::BT709(sceneColor);
  float3 color_lms = renodx::color::lms::from::BT2020(color_bt2020);

  float3 peak_lms = renodx::color::lms::from::BT2020(peak.xxx);

  color_lms = renodx::tonemap::Reinhard(max(color_lms, 0.f), peak_lms);
  color_bt2020 = renodx::color::bt2020::from::LMS(color_lms);

  return max(renodx::color::bt709::from::BT2020(color_bt2020), 0.f);
}

float HDRBoost(float color, float power = 0.20f, float normalization_point = 0.02f) {
  const float smoothing = power * 2.f;

  float boosted = max(color, lerp(color, normalization_point * pow(color / normalization_point, 1.f + power), renodx::tonemap::Reinhard(color, smoothing)));
  // float highlight_compression_scale = saturate(pow(saturate((color - highlight_compression_start) / (highlight_compression_peak - highlight_compression_start)), highlight_compression_curve));
  // float smoothed = lerp(boosted, color, highlight_compression_scale);
  return boosted;
  // return smoothed;
}

float3 HDRBoost(float3 color, float power = 0.20f, float normalization_point = 0.02f) {
  return float3(
      HDRBoost(color.r, power, normalization_point),
      HDRBoost(color.g, power, normalization_point),
      HDRBoost(color.b, power, normalization_point)
  );
}

float Highlights(float x, float highlights, float mid_gray, float highlights_version) {
  float value;
  [branch]
  if (highlights_version == 2.f) {
    [branch]
    if (highlights > 1.f) {
      float bias = 0.10f;
      float scaled = (highlights * highlights - highlights);
      float extra = bias * scaled * scaled * x * x * x * (x - mid_gray);
      value = ((mid_gray * x) + extra) / mid_gray;
    } else {
      value = x;
    }
  } else if (highlights_version == 1.f) {
    float scaled = x / mid_gray;
    float highlighted = lerp(scaled, pow(scaled, highlights), saturate(scaled));
    float rescaled = highlighted * mid_gray;
    value = rescaled;
  } else {
    // Version 3 (Default)
    [branch]
    if (highlights > 1.f) {
      value = max(x, lerp(x, mid_gray * pow(x / mid_gray, highlights), x));
    } else if (highlights < 1.f) {
      // value = x * x / lerp(x, mid_gray * pow(x / mid_gray, 2.f - highlights), x);
      value = min(x, x / (1.f + mid_gray * pow(x / mid_gray, 2.f - highlights) - x));
    } else {
      value = x;
      // 0
    }
  }
  return value;
}