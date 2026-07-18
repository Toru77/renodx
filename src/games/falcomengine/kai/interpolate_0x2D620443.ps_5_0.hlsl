// ---- Created with 3Dmigoto v1.3.16 on Sun Sep 14 17:39:23 2025
#include "../common.hlsl"
cbuffer cb_local : register(b2)
{
  float3 mulColor_g : packoffset(c0);
  float alpha_g : packoffset(c0.w);
  float3 addColor_g : packoffset(c1);
  float intensity_g : packoffset(c1.w);
}

SamplerState samPoint_s : register(s0);
Texture2D<float4> colorTexture : register(t0);


// 3Dmigoto declarations
#define cmp -

void main(
    float4 v0: SV_Position0,
    float4 v1: TEXCOORD0,
    out float4 o0: SV_Target0)
{
  float4 r0, r1, r2;
  uint4 bitmask, uiDest;
  float4 fDest;
  
  // note: this shader is also used in "memorized" scenes, not just menu

  r0.xyz = colorTexture.SampleLevel(samPoint_s, v1.xy, 0).xyz;

  float3 sdr = renodx::tonemap::neutwo::BT709(srgbDecode(r0.rgb));

  if (RENODX_TONE_MAP_TYPE == 0.f) {
    r0.xyz = saturate(r0.xyz);
    r0.w = dot(r0.xyz, float3(0.298999995, 0.587000012, 0.114));
  } else {

    r0.w = calculateLuminanceSRGB(r0.rgb);
  }
  r1.xyz = r0.www * mulColor_g.xyz + addColor_g.xyz;
  // r1.xyz = r1.xyz + -r0.xyz;
  // o0.xyz = intensity_g * r1.xyz + r0.xyz;

  o0.rgb = lerp(r0.xyz, r1.xyz, intensity_g);

  sdr = lerp(sdr, r1.rgb, intensity_g);

  // // Lazy scene tonemap.
  // //
  // // Output stays linear-light, sRGB-encoded with no display gamma applied so
  // // the existing UI compositing path keeps working unchanged. final/finalkai
  // // checks scene_already_tonemapped and skips its own ToneMapLMS this frame
  // // (the addon flips that flag in interpolate's on_drawn callback).
  // //
  // // When godray runs after interpolate, godray will operate on a tonemapped
  // // base — bloom math is slightly off in that path but acceptable.
  // if (RENODX_TONE_MAP_TYPE > 0.f) {
  //   o0.rgb = renodx::color::srgb::DecodeSafe(o0.rgb);

  //   // r0.rgb = o0.rgb;
  //   // float HDRGamutRatio = 0.5f;

  //   // r1.xyz = float3(0.329299986, 0.919499993, 0.0879999995) * r0.yyy;
  //   // r1.xyz = r0.xxx * float3(0.627399981, 0.0691, 0.0164000001) + r1.xyz;
  //   // r1.xyz = r0.zzz * float3(0.0432999991, 0.0114000002, 0.895600021) + r1.xyz;
  //   // r2.xyz = -r1.xyz + r0.xyz;
  //   // r0.xyz = HDRGamutRatio * r2.xyz + r1.xyz;

  //   // o0.rgb = SE_Saturation(r0.rgb);
    

  //   o0.rgb = ToneMapLMSHueShift(o0.rgb);

  //   float3 color = o0.rgb;

  //   [branch]
  //   if (RENODX_GAMMA_CORRECTION == renodx::draw::GAMMA_CORRECTION_GAMMA_2_2) {
  //     color = renodx::color::correct::GammaSafe(color, false, 2.2f);
  //   } else if (RENODX_GAMMA_CORRECTION == renodx::draw::GAMMA_CORRECTION_GAMMA_2_4) {
  //     color = renodx::color::correct::GammaSafe(color, false, 2.4f);
  //   } else if (RENODX_GAMMA_CORRECTION == 3.f) {
  //     color = renodx::color::correct::GammaSafe(color, false, 2.2f);
  //   }

  //   // This is RenderIntermediatePass, simply brightness scaling and srgb encoding
  //   color *= RENODX_DIFFUSE_WHITE_NITS / RENODX_GRAPHICS_WHITE_NITS;

  //   [branch]
  //   if (RENODX_GAMMA_CORRECTION == renodx::draw::GAMMA_CORRECTION_GAMMA_2_2) {
  //     color = renodx::color::correct::GammaSafe(color, true, 2.2f);
  //   } else if (RENODX_GAMMA_CORRECTION == renodx::draw::GAMMA_CORRECTION_GAMMA_2_4) {
  //     color = renodx::color::correct::GammaSafe(color, true, 2.4f);
  //   } else if (RENODX_GAMMA_CORRECTION == 3.f) {
  //     color = renodx::color::correct::GammaSafe(color, true, 2.2f);
  //   }

  //   o0.rgb = color;

  //   o0.rgb = renodx::color::srgb::EncodeSafe(o0.rgb);
  // }

  o0.w = alpha_g;
  return;
}