#include "./shared.h"

float3 Tonemap(float3 colorHDR, float3 colorSDRNeutral, float3 colorSDRGraded) {
  //tonemap
  //colorSDRNeutral = renodx::color::srgb::DecodeSafe(colorSDRNeutral);
  colorHDR = renodx::tonemap::UpgradeToneMap(colorHDR, colorSDRNeutral, colorSDRGraded, RENODX_COLOR_GRADE_STRENGTH);
  colorHDR = renodx::draw::ToneMapPass(colorHDR);
  return colorHDR;
}

float3 RenderIntermediatePass(float3 x) {
  x = renodx::color::srgb::DecodeSafe(x);
  x = renodx::draw::RenderIntermediatePass(x);

  return x;
}