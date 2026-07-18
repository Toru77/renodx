// ---- Created with 3Dmigoto v1.3.16 on Fri Sep 19 06:24:48 2025
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

  r2.rgb = 1 - r1.rgb;
  r2.rgb = max(0.f, r2.rgb);
  float3 sdr = r0.xyz * r2.xyz + r1.xyz;
  
  float3 color = r1.rgb;
  float3 atomosphere = r0.rgb;

  if (shader_injection.bloom == 0.f) {
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
  o0.rgb = renodx::color::correct::Chrominance(color, sdr);
  o0.rgb = renodx::color::srgb::EncodeSafe(o0.rgb);
  o0.w = r1.w;
  return;
}