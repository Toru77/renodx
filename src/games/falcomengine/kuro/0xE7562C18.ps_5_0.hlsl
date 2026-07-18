// ---- Created with 3Dmigoto v1.3.16 on Tue Sep 23 01:06:23 2025

cbuffer cb_local : register(b2)
{
  float lum_g : packoffset(c0);
  float intensity_g : packoffset(c0.y);
}

SamplerState samLinear_s : register(s0);
Texture2D<float4> colorTexture : register(t0);
Texture2D<uint4> mrtTexture : register(t1);


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

  mrtTexture.GetDimensions(0, fDest.x, fDest.y, fDest.z);
  r0.xy = fDest.xy;
  r0.xy = v1.xy * r0.xy;
  r0.xy = (int2)r0.xy;
  r0.zw = float2(0,0);
  r0.x = mrtTexture.Load(r0.xyz).w;
  r0.x = (uint)r0.x >> 16;
  r0.x = (uint)r0.x;
  r0.y = 0.000152590219 * r0.x;
  r0.x = cmp(0 >= r0.x);
  r1.xyzw = colorTexture.SampleLevel(samLinear_s, v1.xy, 0).xyzw;
  r2.xyz = intensity_g * r1.xyz;
  r0.xzw = r0.xxx ? r2.xyz : r1.xyz;
  o0.w = r1.w;
  r1.xyz = r0.xzw * r0.yyy;
  r0.xyz = -lum_g + r0.xzw;
  o0.xyz = max(r1.xyz, r0.xyz);
  return;
}