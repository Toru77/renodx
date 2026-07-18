// ---- Created with 3Dmigoto v1.3.16 on Mon Sep 29 00:42:31 2025
#include "../common.hlsl"
cbuffer cb_local : register(b2)
{
  float exposureBias_g : packoffset(c0);
  float exposureMin_g : packoffset(c0.y);
  float exposureMax_g : packoffset(c0.z);
  float lumAdvance_g : packoffset(c0.w);
  float intensity_g : packoffset(c1);
}

SamplerState samPoint_s : register(s0);
Texture2D<float4> colorTexture : register(t0);
Texture2D<float4> lumTexture : register(t1);
Texture2D<uint4> mrtTexture : register(t2);


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_Position0,
  float4 v1 : TEXCOORD0,
  out float4 o0 : SV_Target0)
{
  float4 r0,r1;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.xyzw = colorTexture.SampleLevel(samPoint_s, v1.xy, 0).xyzw;
  mrtTexture.GetDimensions(0, fDest.x, fDest.y, fDest.z);
  r1.xy = fDest.xy;
  r1.xy = v1.xy * r1.xy;
  r1.xy = (int2)r1.xy;
  r1.zw = float2(0,0);
  r1.x = mrtTexture.Load(r1.xyz).z;
  r1.x = (uint)r1.x >> 24;
  r1.x = (int)r1.x & 16;
  if (r1.x != 0) {
    o0.xyzw = r0.xyzw;
    return;
  }
  r1.x = lumTexture.SampleLevel(samPoint_s, float2(0.5,0.5), 0).x;
  r1.x = max(9.99999975e-005, r1.x);
  r1.x = exposureBias_g / r1.x;
  r1.x = min(exposureMax_g, r1.x);
  r1.x = max(exposureMin_g, r1.x);

  // r0.rgb = renodx::color::srgb::DecodeSafe(r0.rgb);

  // r1.xyz = r0.xyz * r1.xxx + -r0.xyz;
  // o0.xyz = intensity_g * r1.xyz + r0.xyz;

  float3 exposure = r1.x * r0.rgb;
  float3 org = renodx::color::srgb::DecodeSafe(r0.rgb);
  exposure = renodx::color::srgb::DecodeSafe(exposure);
  o0.rgb = lerp(org, exposure, intensity_g);

  o0.rgb = renodx::color::srgb::EncodeSafe(o0.rgb);
  o0.w = r0.w;
  return;
}