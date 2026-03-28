// ---- Created with 3Dmigoto v1.4.1 on Sat Mar 28 19:41:21 2026

cbuffer _Globals : register(b0)
{
  float4 AmbColParam : packoffset(c0);
  float4 LuminasParam : packoffset(c1);
  float4 ToneMapParam : packoffset(c2);
  float4 ToneScalerParam : packoffset(c3);
  float4 ToneSaoParam : packoffset(c4);
  float4 GradingParam : packoffset(c5);
  float4x4 ScrnViewProjection : packoffset(c6);
  float4x4 invScrnViewProjection : packoffset(c10);
}

SamplerState PointClampSampler_s : register(s0);
Texture2D<float> Exposure0Buffer : register(t0);
Texture2D<float> Exposure1Buffer : register(t1);


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_POSITION0,
  float2 v1 : TEXCOORD0,
  out float o0 : SV_TARGET0)
{
  const float kExposureHardMax = 0.0f;
  float4 r0,r1;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.x = Exposure1Buffer.SampleLevel(PointClampSampler_s, float2(0.5,0.5), 0).x;
  r0.x = LuminasParam.w + -r0.x;
  r1.x = Exposure0Buffer.SampleLevel(PointClampSampler_s, float2(0.5,0.5), 0).x;
  r0.x = -r1.x + r0.x;
  r0.x = LuminasParam.z * r0.x + r1.x;
  o0.x = LuminasParam.x * r0.x + LuminasParam.y;
  o0.x = min(o0.x, kExposureHardMax);
  return;
}
