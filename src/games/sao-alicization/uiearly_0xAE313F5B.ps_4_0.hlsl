// ---- Created with 3Dmigoto v1.4.1 on Mon Mar 23 01:12:20 2026

cbuffer _Globals : register(b0)
{
  float4x4 WorldView : packoffset(c0);
  float4x4 WorldViewProjection : packoffset(c4);
  float4 MaterialColor : packoffset(c8) = {1,1,1,1};
  float AlphaThreshold : packoffset(c9) = {0.00100000005};
}

SamplerState TextureSamplerSampler_s : register(s0);
Texture2D<float4> TextureSampler : register(t0);


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_POSITION0,
  float4 v1 : COLOR0,
  float2 v2 : TEXCOORD0,
  out float4 o0 : SV_TARGET0)
{
  float4 r0,r1;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.xyzw = TextureSampler.Sample(TextureSamplerSampler_s, v2.xy).xyzw;
  r1.x = v1.w * r0.w + -AlphaThreshold;
  r0.xyzw = v1.xyzw * r0.xyzw;
  o0.xyzw = r0.xyzw;
  r0.x = cmp(r1.x < 0);
  if (r0.x != 0) discard;
  return;
}