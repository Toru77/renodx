// ---- Created with 3Dmigoto v1.3.16 on Fri Aug 21 00:15:18 2026
#include "../common.hlsl"

SamplerState samLinear_s : register(s0);
SamplerState samPoint_s : register(s1);
Texture2D<float4> colorTexture : register(t0);
Texture2D<float4> glowTexture : register(t1);


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

  r0.xyz = glowTexture.SampleLevel(samLinear_s, v1.xy, 0).xyz;
  float3 glow = r0.rgb;

  r1.xyzw = colorTexture.SampleLevel(samPoint_s, v1.xy, 0).xyzw;
  float3 color = r1.rgb;
  r2.xyz = float3(1,1,1) + -r1.xyz;
  r2.xyz = max(float3(0,0,0), r2.xyz);
  float3 sdr = r0.xyz * r2.xyz + r1.xyz;

  if (shader_injection.bloom == 0.f || RENODX_TONE_MAP_TYPE == 0.f) {
    o0.rgb = sdr;
  } else {
    color = renodx::color::srgb::DecodeSafe(color);
    glow = renodx::color::srgb::DecodeSafe(glow);

    float3 hdr = hdrScreenBlend(color, glow * 0.5);
    sdr = renodx::color::srgb::DecodeSafe(sdr);

    float strength = shader_injection.bloom_hue_correction;
    hdr = lerp(hdr, CorrectHueAndPurityMBFullStrength(hdr, sdr), saturate(strength));

    o0.rgb = renodx::color::srgb::EncodeSafe(hdr);
  }


  o0.w = r1.w;
  return;
}