#include "../common.hlsl"

// ---- Created with 3Dmigoto v1.4.1 on Sun Jul 19 00:27:22 2026

cbuffer cb_ui : register(b4)
{
  float uiBrightness : packoffset(c0);
  float3 unused : packoffset(c0.y);
}

SamplerState samLinear_s : register(s0);
Texture2D<float4> colorTexture : register(t0);
Texture2D<float4> depthTexture : register(t1);


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_Position0,
  float4 v1 : TEXCOORD0,
  float4 v2 : TEXCOORD1,
  float4 v3 : TEXCOORD2,
  float4 v4 : TEXCOORD3,
  nointerpolation int4 v5 : TEXCOORD4,
  float4 v6 : TEXCOORD5,
  float4 v7 : TEXCOORD6,
  float4 v8 : TEXCOORD7,
  out float4 o0 : SV_Target0)
{
  const float4 icb[] = { { 1.000000, 0, 0, 0},
                              { 0, 1.000000, 0, 0},
                              { 0, 0, 1.000000, 0},
                              { 0, 0, 0, 1.000000} };
  float4 r0,r1,r2;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.xyzw = colorTexture.SampleLevel(samLinear_s, v1.xy, 0).xyzw;
  r1.x = cmp((int)v5.x >= 0);
  if (r1.x != 0) {
    r1.x = v5.x;
    r0.w = dot(r0.xyzw, icb[r1.x+0].xyzw);
    r0.xyz = float3(1,1,1);
  }
  r2.w = calculateLuminanceSRGB(r0.xyz);
  r0.w = v2.w * r0.w;
  r0.xyz = r0.xyz * v2.xyz + v3.xyz;
  r0.xyz = min(float3(1,1,1), r0.xyz);
  r1.xyz = v4.xyz / v4.www;
  r2.xy = r1.xy * float2(0.5,0.5) + float2(0.5,0.5);
  r1.w = 1;
  r1.x = dot(v6.zw, r1.zw);
  r1.y = dot(v7.zw, r1.zw);
  r1.x = r1.x / r1.y;
  r1.x = -v8.x + -r1.x;
  r2.z = 1 + -r2.y;
  r1.yz = v8.zw * r2.xz;
  r1.y = depthTexture.SampleLevel(samLinear_s, r1.yz, 0).x;
  r1.z = 1;
  r1.w = dot(v6.zw, r1.yz);
  r1.y = dot(v7.zw, r1.yz);
  r1.y = r1.w / r1.y;
  r1.x = -r1.y + -r1.x;
  r1.x = max(0, r1.x);
  r1.x = v8.y * r1.x;
  r1.x = min(1, r1.x);
  r1.y = r1.x * r0.w;
  r0.w = r0.w * r1.x + -1;
  r0.w = v1.z * r0.w + 1;
  r0.xyz = r0.xyz * r0.www;
  o0.rgb = renodx::color::srgb::DecodeSafe(r0.rgb);
  o0.xyz = uiBrightness * o0.xyz;
  o0.rgb = processUI(o0.rgb, false);
  o0.w = r1.y;
  return;
}