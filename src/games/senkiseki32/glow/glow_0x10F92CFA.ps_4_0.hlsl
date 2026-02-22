// 0x10F92CFA - glow: gaussian blur (5-tap weighted)

cbuffer _Globals : register(b0)
{
  float4 GaussianBlurBufferSize : packoffset(c0);
  float4 GaussianOutputScale : packoffset(c1);
  float4 UvScaleBias : packoffset(c2);
  float LuminanceThreshold : packoffset(c3);
  float LuminanceScale : packoffset(c3.y);
}

SamplerState LinearClampSampler_s : register(s0);
Texture2D<float4> GlowBuffer : register(t0);


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_POSITION0,
  float4 v1 : TEXCOORD0,
  float4 v2 : TEXCOORD1,
  float4 v3 : TEXCOORD2,
  out float4 o0 : SV_TARGET0)
{
  float4 r0,r1;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.xyzw = GlowBuffer.Sample(LinearClampSampler_s, v2.xy).xyzw;
  r0.xyzw = float4(0.100000001,0.100000001,0.100000001,0.100000001) * r0.xyzw;
  r1.xyzw = GlowBuffer.Sample(LinearClampSampler_s, v1.xy).xyzw;
  r0.xyzw = r1.xyzw * float4(0.400000006,0.400000006,0.400000006,0.400000006) + r0.xyzw;
  r1.xyzw = GlowBuffer.Sample(LinearClampSampler_s, v2.zw).xyzw;
  r0.xyzw = r1.xyzw * float4(0.200000003,0.200000003,0.200000003,0.200000003) + r0.xyzw;
  r1.xyzw = GlowBuffer.Sample(LinearClampSampler_s, v3.xy).xyzw;
  r0.xyzw = r1.xyzw * float4(0.200000003,0.200000003,0.200000003,0.200000003) + r0.xyzw;
  r1.xyzw = GlowBuffer.Sample(LinearClampSampler_s, v3.zw).xyzw;
  r0.xyzw = r1.xyzw * float4(0.100000001,0.100000001,0.100000001,0.100000001) + r0.xyzw;
  o0.xyzw = GaussianOutputScale.xyzw * r0.xyzw;
  return;
}