// ---- Created with 3Dmigoto v1.3.16 on Mon Sep 01 23:38:26 2025
#include "../common.hlsl"
cbuffer Constants : register(b0)
{
  float gamma : packoffset(c0);
  uint hdr_enabled : packoffset(c0.y);
  float hdr_peak_brightness : packoffset(c0.z);
}

SamplerState smpl_s : register(s0);
Texture2D<float4> tex : register(t0);

// Final Shader for Kuro and YSX


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_Position0,
  float2 v1 : TEXCOORD0,
  out float4 o0 : SV_Target0)
{
  float4 r0;
  uint4 bitmask, uiDest;
  float4 fDest;

  if (hdr_enabled != 0) {
    r0.xyz = tex.SampleLevel(smpl_s, v1.xy, 0).xyz;

    if (RENODX_TONE_MAP_TYPE > 0) {

      r0.rgb = renodx::color::srgb::DecodeSafe(r0.rgb);

      r0.rgb = ToneMapAndSwapchainPass(r0.rgb);

      o0.rgb = r0.rgb;

      o0.w = 1;
    }
    else {
      // r0.xyz = log2(r0.xyz);
      // r0.xyz = float3(2.29999995,2.29999995,2.29999995) * r0.xyz;
      // r0.xyz = exp2(r0.xyz);
      r0.rgb = renodx::math::SafePow(r0.rgb, 2.3f);
      // r0.xyz = hdr_peak_brightness * r0.xyz;
      // r0.xyz = max(float3(0,0,0), r0.xyz);
      // o0.xyz = min(float3(200,200,200), r0.xyz);
      if (shader_injection.hdr_format == 1.f) {
        // o0.rgb *= RENODX_DIFFUSE_WHITE_NITS / 80.f;
        r0.xyz = hdr_peak_brightness * r0.xyz;
        r0.xyz = max(float3(0, 0, 0), r0.xyz);
        o0.xyz = min(float3(200, 200, 200), r0.xyz);
      } else {
        float diffuse_white = hdr_peak_brightness * 80.f;
        o0.rgb = r0.rgb;
        o0.rgb = renodx::color::bt2020::from::BT709(o0.rgb);
        o0.rgb = renodx::color::pq::EncodeSafe(o0.rgb, diffuse_white);
      }

      o0.w = 1;
      return;
    }
  } else {
    r0.xyz = tex.SampleLevel(smpl_s, v1.xy, 0).xyz;
    // r0.xyz = saturate(r0.xyz);
    // r0.xyz = log2(r0.xyz);
    // r0.xyz = gamma * r0.xyz;
    // o0.xyz = exp2(r0.xyz);
    // o0.w = 1;
    // return;

    r0.rgb = renodx::math::SafePow(r0.rgb, gamma);

    if (RENODX_TONE_MAP_TYPE > 0) {
      r0.rgb = renodx::color::srgb::DecodeSafe(r0.rgb);

      r0.rgb = ToneMapAndSwapchainPass(r0.rgb); 

      o0.rgb = r0.rgb;

      o0.w = 1;
    }

    o0.w = 1;
    return;
  }
  return;
}