#include "../common.hlsl"

// ---- Created with 3Dmigoto v1.4.1 on Sun Jul 19 00:52:27 2026

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
  nointerpolation float4 v4 : TEXCOORD3,
  nointerpolation float2 v5 : TEXCOORD4,
  nointerpolation int w5 : TEXCOORD5,
  out float4 o0 : SV_Target0)
{
  const float4 icb[] = { { 1.000000, 0, 0, 0},
                              { 0, 1.000000, 0, 0},
                              { 0, 0, 1.000000, 0},
                              { 0, 0, 0, 1.000000},
                              { 0.363636, 0, 0, 0},
                              { 0.112370, 0.345838, 0, 0},
                              { -0.294188, 0.213740, 0, 0},
                              { -0.294188, -0.213740, 0, 0},
                              { 0.112370, -0.345838, 0, 0},
                              { 0.727273, 0, 0, 0},
                              { 0.588376, 0.427480, 0, 0},
                              { 0.224740, 0.691678, 0, 0},
                              { -0.224740, 0.691678, 0, 0},
                              { -0.588376, 0.427480, 0, 0},
                              { -0.727273, 0, 0, 0},
                              { -0.588376, -0.427480, 0, 0},
                              { -0.224739, -0.691678, 0, 0},
                              { 0.224740, -0.691678, 0, 0},
                              { 0.588376, -0.427480, 0, 0},
                              { 1.000000, 0, 0, 0},
                              { 0.913545, 0.406737, 0, 0},
                              { 0.669131, 0.743145, 0, 0},
                              { 0.309017, 0.951057, 0, 0},
                              { -0.104529, 0.994522, 0, 0},
                              { -0.500000, 0.866025, 0, 0},
                              { -0.809017, 0.587785, 0, 0},
                              { -0.978148, 0.207912, 0, 0},
                              { -0.978148, -0.207912, 0, 0},
                              { -0.809017, -0.587785, 0, 0},
                              { -0.500000, -0.866025, 0, 0},
                              { -0.104528, -0.994522, 0, 0},
                              { 0.309017, -0.951056, 0, 0},
                              { 0.669131, -0.743145, 0, 0},
                              { 0.913546, -0.406736, 0, 0} };
  float4 r0,r1,r2,r3,r4,r5;
  int sampleIndex;
  int channelIndex;
  uint4 bitmask, uiDest;
  float4 fDest;

  colorTexture.GetDimensions(0, fDest.x, fDest.y, fDest.z);
  r0.xy = fDest.xy;
  r0.xy = float2(1,1) / r0.xy;
  r0.xy = v5.xx * r0.xy;
  r0.zw = max(v4.xy, v1.xy);
  r0.zw = min(v4.zw, r0.zw);
  r1.xyzw = colorTexture.SampleLevel(samLinear_s, r0.zw, 0).xyzw;
  r0.z = cmp((int)w5.x >= 0);
  if (r0.z != 0) {
    channelIndex = w5;
    r1.w = dot(r1.xyzw, icb[channelIndex+0].xyzw);
    r1.xyz = float3(1,1,1);
  }
  r2.xyz = float3(1,1,1);
  r3.xyzw = r1.xyzw;
  r0.w = 1;
  r4.x = 0;
  while (true) {
    r4.y = cmp((uint)r4.x >= 30);
    if (r4.y != 0) break;
    sampleIndex = (int)r4.x;
    r4.yz = icb[sampleIndex+4].xy * r0.xy + v1.xy;
    r4.yz = max(v4.xy, r4.yz);
    r4.yz = min(v4.zw, r4.yz);
    r5.xyzw = colorTexture.SampleLevel(samLinear_s, r4.yz, 0).xyzw;
    if (r0.z != 0) {
      channelIndex = w5;
      r2.w = dot(r5.xyzw, icb[channelIndex+0].xyzw);
      r5.xyzw = r2.xyzw;
    }
    r2.w = r5.w + -r1.w;
    r2.w = max(0, r2.w);
    r4.y = dot(icb[sampleIndex+4].xy, icb[sampleIndex+4].xy);
    r4.y = sqrt(r4.y);
    r4.y = r4.y * r2.w;
    r3.xyzw = r5.xyzw * r2.wwww + r3.xyzw;
    r0.w = r4.y * r4.y + r0.w;
    r4.x = (int)r4.x + 1;
  }
  r0.xyzw = r3.xyzw / r0.wwww;
  r2.x = renodx::color::y::from::BT709(renodx::color::srgb::DecodeSafe(r0.xyz));
  r1.x = v2.w * r0.w;
  r0.xyz = r0.xyz * v2.xyz + v3.xyz;
  r0.xyz = min(float3(1,1,1), r0.xyz);
  r0.w = r0.w * v2.w + -1;
  r0.w = v5.y * r0.w + 1;
  r0.xyz = r0.xyz * r0.www;
  o0.rgb = renodx::color::srgb::DecodeSafe(r0.rgb);
  o0.xyz = uiBrightness * r0.xyz;
  o0.rgb = processUI(o0.rgb, false);
  o0.w = r1.x;
  return;
}