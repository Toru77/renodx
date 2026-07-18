// ---- Created with 3Dmigoto v1.3.16 on Mon Sep 29 00:42:31 2025

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
  r0.xy = mrtTexture.Load(r0.xyz).zw;
  r0.y = (uint)r0.y >> 16;
  if (1 == 0) r0.x = 0; else if (1+24 < 32) {   r0.x = (uint)r0.x << (32-(1 + 24)); r0.x = (uint)r0.x >> (32-1);  } else r0.x = (uint)r0.x >> 24;
  r0.xz = r0.xx ? float2(1,1.10000002) : intensity_g;
  r0.y = (uint)r0.y;
  r0.w = 0.000152590219 * r0.y;
  r0.y = cmp(0 >= r0.y);
  r1.xyzw = colorTexture.SampleLevel(samLinear_s, v1.xy, 0).xyzw;
  r2.xyz = r1.xyz * r0.xxx;
  r1.xyz = r0.yyy ? r2.xyz : r1.xyz;
  o0.w = r1.w;
  r0.xyw = r1.xyz * r0.www;
  r1.xyz = r1.xyz + -r0.zzz;
  o0.xyz = max(r1.xyz, r0.xyw);
  return;
}