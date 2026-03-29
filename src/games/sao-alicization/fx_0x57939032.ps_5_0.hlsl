// ---- Created with 3Dmigoto v1.4.1 on Sun Mar 29 17:15:11 2026

cbuffer CBUber : register(b0)
{
  float4 AlphaTestParam : packoffset(c0);
  float4 ColorScale : packoffset(c1);
  float4 SpecularColorAndPow : packoffset(c2);
  float4 FogColor : packoffset(c3);
  float4 FogArea : packoffset(c4);
  float4 SoftThreshold : packoffset(c5);
  float4x4 InverseProjMatrix : packoffset(c6);
  float4 DistDMapMidValue : packoffset(c10);
}

SamplerState BaseSampler_s : register(s0);
SamplerState SubSampler_s : register(s1);
Texture2D<float4> BaseTexture : register(t0);
Texture2D<float4> SubTexture : register(t1);


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_POSITION0,
  float4 v1 : COLOR0,
  float2 v2 : TEXCOORD0,
  float2 w2 : TEXCOORD1,
  float4 v3 : TEXCOORD2,
  float4 v4 : TEXCOORD3,
  out float4 o0 : SV_TARGET0)
{
  float4 r0,r1;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.xyz = SubTexture.Sample(SubSampler_s, w2.xy).xyw;
  r0.xy = -DistDMapMidValue.xy * float2(1.0/255.0, 1.0/255.0) + r0.xy;
  r0.xyz = v4.xyw * r0.xyz;
  r0.xy = r0.xy * r0.zz + v2.xy;
  r1.xyzw = BaseTexture.Sample(BaseSampler_s, r0.xy).xyzw;
  r1.xyzw = v3.xyzw * r1.xyzw;
  r1.xyzw = v1.xyzw * r1.xyzw;
  r1.w = r1.w * r0.z;
  o0.xyzw = ColorScale.xyzw * r1.xyzw;
  return;
}