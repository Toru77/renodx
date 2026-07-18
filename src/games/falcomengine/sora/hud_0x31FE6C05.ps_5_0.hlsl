// ---- Created with 3Dmigoto v1.3.16 on Sat Mar 14 15:37:53 2026
#include "../common.hlsl"
cbuffer cb_ui : register(b4)
{
  float uiBrightness : packoffset(c0);
  float3 unused : packoffset(c0.y);
}

SamplerState samLinear_s : register(s0);
Texture2D<float4> colorTexture : register(t0);


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_Position0,
  float4 v1 : TEXCOORD0,
  float4 v2 : TEXCOORD1,
  float4 v3 : TEXCOORD2,
  nointerpolation int v4 : TEXCOORD3,
  out float4 o0 : SV_Target0)
{
  const float4 icb[] = { { 1.000000, 0, 0, 0},
                              { 0, 1.000000, 0, 0},
                              { 0, 0, 1.000000, 0},
                              { 0, 0, 0, 1.000000} };
  float4 r0,r1;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.xyzw = colorTexture.SampleLevel(samLinear_s, v1.xy, 0).xyzw;
  r1.x = cmp((int)v4.x >= 0);
  if (r1.x != 0) {
    r1.x = v4.x;
    r0.w = dot(r0.xyzw, icb[r1.x+0].xyzw);
    r0.xyz = float3(1,1,1);
  }
  r1.x = v2.w * r0.w;
  r0.xyz = r0.xyz * v2.xyz + v3.xyz;
  r0.xyz = min(float3(1,1,1), r0.xyz);
  r0.w = r0.w * v2.w + -1;
  r0.w = v1.z * r0.w + 1;
  r0.xyz = r0.xyz * r0.www;

  o0.rgb = renodx::color::srgb::DecodeSafe(r0.rgb);
  o0.xyz = uiBrightness * o0.xyz;

  o0.rgb = processUI(o0.rgb, false);
  o0.w = r1.x;
  return;
}