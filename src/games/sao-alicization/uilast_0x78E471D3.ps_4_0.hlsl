// ---- Created with 3Dmigoto v1.4.1 on Mon Mar 23 01:12:20 2026

cbuffer _Globals : register(b0)
{
  float4 textColor : packoffset(c0) = {1,1,1,1};
  float4x4 World : packoffset(c1);
  float CameraAspectRatio : packoffset(c5);
  float alphaThreshold : packoffset(c5.y) = {0.5};
  float4 outlineColor : packoffset(c6) = {0,0,0,1};
  float4 outlineValues : packoffset(c7) = {0.469999999,0.5,0.620000005,0.629999995};
  float4 shadowColor : packoffset(c8) = {0,0,0,1};
  float2 shadowUVOffset : packoffset(c9) = {-0.00249999994,-0.00249999994};
  float4 glowColor : packoffset(c10) = {1,0,0,1};
  float2 glowValues : packoffset(c11) = {0.170000002,0.5};
  float2 softEdges : packoffset(c11.z) = {0.5,0.50999999};
}

SamplerState LinearClampSampler_s : register(s0);
Texture2D<float4> BitmapFontTexture : register(t0);


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_POSITION0,
  float2 v1 : TEXCOORD0,
  out float4 o0 : SV_TARGET0)
{
  float4 r0;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.xyzw = BitmapFontTexture.Sample(LinearClampSampler_s, v1.xy).xyzw;
  r0.x = textColor.w * r0.x;
  r0.y = cmp(0.200000003 >= r0.x);
  if (r0.y != 0) discard;
  o0.xyz = textColor.xyz;
  o0.w = r0.x;
  return;
}