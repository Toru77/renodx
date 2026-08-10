// ---- Created with 3Dmigoto v1.3.16 on Tue Aug 26 19:14:01 2025
#include "../cs4/common.hlsl"
Texture2D<float4> t0 : register(t0);

SamplerState s1_s : register(s1);

SamplerState s0_s : register(s0);

cbuffer cb0 : register(b0)
{
  float4 cb0[8];
}




// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_Position0,
  float2 v1 : TEXCOORD0,
  out float4 o0 : SV_TARGET0)
{
  float4 r0,r1,r2,r3,r4;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.xy = -cb0[7].xy + v0.xy;
  r1.xz = float2(0,-0);
  r1.yw = cb0[1].ww;
  r0.zw = r0.xy * cb0[1].zw + r1.xy;
  r0.zw = t0.Sample(s0_s, r0.zw).xy;
  r2.xy = cb0[1].zw * r0.xy;
  r0.xy = r0.xy * cb0[1].zw + -r1.xy;
  r0.xy = t0.Sample(s0_s, r0.xy).xy;
  r2.zw = t0.Sample(s0_s, r2.xy).xy;
  r3.x = -r2.w + r0.w;
  r0.w = -r3.x + r0.w;
  r3.y = cb0[5].w * -r2.w;
  r0.w = r0.w * cb0[5].w + r3.y;
  r0.w = -r0.w * r0.w + -0.0799999982;
  r0.w = exp2(r0.w);
  r0.z = r0.w * r0.z + r2.z;
  r0.w = 1 + r0.w;
  r2.z = -r2.w + r0.y;
  r4.xyzw = float4(2,2,3.5,3.5) * r1.xyxy + r2.xyxy;
  r1.xyzw = float4(2,-2,3.5,-3.5) * r1.zwzw + r2.xyxy;
  r2.xy = t0.Sample(s0_s, r4.xy).xy;
  r3.zw = t0.Sample(s1_s, r4.zw).xy;
  r2.y = -r3.x * 2 + r2.y;
  r2.w = -r3.x * 3 + r3.w;
  r2.w = r2.w * cb0[5].w + r3.y;
  r2.w = -r2.w * r2.w + -0.719999969;
  r2.w = exp2(r2.w);
  r2.y = r2.y * cb0[5].w + r3.y;
  r2.y = -r2.y * r2.y + -0.319999993;
  r2.y = exp2(r2.y);
  r0.z = r2.y * r2.x + r0.z;
  r0.w = r2.y + r0.w;
  r0.w = r0.w + r2.w;
  r0.z = r2.w * r3.z + r0.z;
  r0.y = -r2.z + r0.y;
  r0.y = r0.y * cb0[5].w + r3.y;
  r0.y = -r0.y * r0.y + -0.0799999982;
  r0.y = exp2(r0.y);
  r0.x = r0.y * r0.x + r0.z;
  r0.y = r0.w + r0.y;
  r0.zw = t0.Sample(s0_s, r1.xy).xy;
  r1.xy = t0.Sample(s1_s, r1.zw).xy;
  r0.w = -r2.z * 2 + r0.w;
  r1.y = -r2.z * 3 + r1.y;
  r1.y = r1.y * cb0[5].w + r3.y;
  r0.w = r0.w * cb0[5].w + r3.y;
  r0.w = -r0.w * r0.w + -0.319999993;
  r0.w = exp2(r0.w);
  r1.y = -r1.y * r1.y + -0.719999969;
  r1.y = exp2(r1.y);
  r0.x = r0.w * r0.z + r0.x;
  r0.y = r0.y + r0.w;
  r0.y = r0.y + r1.y;
  r0.x = r1.y * r1.x + r0.x;
  r0.x = saturate(r0.x / r0.y);
  r0.x = log2(r0.x);
  r0.x = cb0[4].z * r0.x;
  o0.xyzw = exp2(r0.xxxx);
  // o0 = renodx::math::SafePow(r0.x, cb0[4].z);
  // o0.y = renodx::math::SafePow(r0.x, cb0[4].z);
  // o0.z = renodx::math::SafePow(r0.x, cb0[4].z);
  // o0.w = renodx::math::SafePow(r0.x, cb0[4].z);
  return;
}