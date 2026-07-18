// ---- Created with 3Dmigoto v1.3.16 on Mon Sep 29 00:42:31 2025
#include "../common.hlsl"
SamplerState samLinear_s : register(s0);
SamplerState samPoint_s : register(s1);
Texture2D<float4> colorTexture : register(t0);
Texture2D<float4> atomosphereTexture : register(t1);


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_Position0,
  float4 v1 : TEXCOORD0,
  out float4 o0 : SV_Target0)
{
  float4 r0,r1,r2;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.xyz = atomosphereTexture.SampleLevel(samLinear_s, v1.xy, 0).xyz;
  r1.xyzw = colorTexture.SampleLevel(samPoint_s, v1.xy, 0).xyzw;
  float3 color = r1.rgb;
  float3 atomosphere = r0.rgb;

  r2.xyz = float3(1, 1, 1) + -r1.xyz;
  r2.xyz = max(float3(0, 0, 0), r2.xyz);
  float3 sdr = r0.xyz * r2.xyz + r1.xyz;

  if ( shader_injection.bloom == 0.f || RENODX_TONE_MAP_TYPE == 0.f) {
    o0.rgb = sdr;
    o0.w = r1.w;
    return;
  }

  color = renodx::color::srgb::DecodeSafe(color);
  atomosphere = renodx::color::srgb::DecodeSafe(atomosphere);

  // color = hdrScreenBlend(color, atomosphere);
  color = addBloom(color, atomosphere);
  sdr = renodx::color::srgb::DecodeSafe(sdr);

  // saturation correction
  o0.rgb = color;
  o0.rgb = renodx::color::srgb::EncodeSafe(o0.rgb);
  o0.w = r1.w;

  // if (shader_injection.bloom_space == 0) {
  //   color = srgbDecode(color);
  //   atomosphere = srgbDecode(atomosphere);

  //   float3 addition = renodx::math::SafeDivision(atomosphere, (1 + color), 0.f);

  //   color += addition;

  //   o0.rgb = color;
  //   o0.rgb = srgbEncode(o0.rgb);
  // } else {
    // r2.xyz = float3(1,1,1) + -r1.xyz;
    // r2.xyz = max(float3(0,0,0), r2.xyz);
    // o0.xyz = r0.xyz * r2.xyz + r1.xyz;
  //   o0.w = r1.w;
  // }
  return;
}