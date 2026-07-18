// ---- Created with 3Dmigoto v1.3.16 on Mon Sep 29 00:42:31 2025
#include "../common.hlsl"
cbuffer cb_local : register(b2)
{
  float2 blurCenter_g : packoffset(c0);
  float2 blurScale_g : packoffset(c0.z);
  float zThreshold_g : packoffset(c1);
  float isFlip_g : packoffset(c1.y);
  float brightnessThreshold_g : packoffset(c1.z);
  float centricSharpness_g : packoffset(c1.w);
  float3 godrayColor_g : packoffset(c2);
}

SamplerState samLinear_s : register(s0);
Texture2D<float4> colorTexture : register(t0);


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_Position0,
  float4 v1 : TEXCOORD0,
  out float4 o0 : SV_Target0)
{
  float4 r0,r1,r2,r3;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.xy = blurCenter_g.xy + -v1.xy;
  r0.xy = isFlip_g * r0.xy;
  r0.z = dot(r0.xy, r0.xy);
  r0.w = rsqrt(r0.z);
  r0.z = sqrt(r0.z);
  r0.z = min(1, r0.z);
  r0.xy = r0.xy * r0.ww;
  r0.xy = blurScale_g.xy * r0.xy;
  r1.xy = r0.xy * r0.zz + v1.xy;
  r0.xy = r0.xy * r0.zz;
  r1.xyzw = colorTexture.SampleLevel(samLinear_s, r1.xy, 0).xyzw;
  r2.xyzw = colorTexture.SampleLevel(samLinear_s, v1.xy, 0).xyzw;

  r2.xyzw = r2.xyzw * float4(0.189999998,0.189999998,0.189999998,0.189999998) + float4(0,0,0,1);
  r1.xyzw = r1.xyzw * float4(0.170000002,0.170000002,0.170000002,0.170000002) + r2.xyzw;
  r0.zw = r0.xy * float2(2,2) + v1.xy;
  r2.xyzw = colorTexture.SampleLevel(samLinear_s, r0.zw, 0).xyzw;

  r1.xyzw = r2.xyzw * float4(0.150000006,0.150000006,0.150000006,0.150000006) + r1.xyzw;
  r2.xyzw = r0.xyxy * float4(3,3,4,4) + v1.xyxy;
  r3.xyzw = colorTexture.SampleLevel(samLinear_s, r2.xy, 0).xyzw;
  r2.xyzw = colorTexture.SampleLevel(samLinear_s, r2.zw, 0).xyzw;

  r1.xyzw = r3.xyzw * float4(0.129999995,0.129999995,0.129999995,0.129999995) + r1.xyzw;
  r1.xyzw = r2.xyzw * float4(0.109999999,0.109999999,0.109999999,0.109999999) + r1.xyzw;
  r2.xyzw = r0.xyxy * float4(5,5,6,6) + v1.xyxy;
  r3.xyzw = colorTexture.SampleLevel(samLinear_s, r2.xy, 0).xyzw;
  r2.xyzw = colorTexture.SampleLevel(samLinear_s, r2.zw, 0).xyzw;

  r1.xyzw = r3.xyzw * float4(0.0900000036,0.0900000036,0.0900000036,0.0900000036) + r1.xyzw;
  r1.xyzw = r2.xyzw * float4(0.0700000003,0.0700000003,0.0700000003,0.0700000003) + r1.xyzw;
  r2.xyzw = r0.xyxy * float4(7,7,8,8) + v1.xyxy;
  r0.xy = r0.xy * float2(9,9) + v1.xy;
  r0.xyzw = colorTexture.SampleLevel(samLinear_s, r0.xy, 0).xyzw;
  r3.xyzw = colorTexture.SampleLevel(samLinear_s, r2.xy, 0).xyzw;
  r2.xyzw = colorTexture.SampleLevel(samLinear_s, r2.zw, 0).xyzw;

  r1.xyzw = r3.xyzw * float4(0.0500000007,0.0500000007,0.0500000007,0.0500000007) + r1.xyzw;
  r1.xyzw = r2.xyzw * float4(0.0299999993,0.0299999993,0.0299999993,0.0299999993) + r1.xyzw;
  o0.xyzw = r0.xyzw * float4(0.00999999978,0.00999999978,0.00999999978,0.00999999978) + r1.xyzw;

  return;
}