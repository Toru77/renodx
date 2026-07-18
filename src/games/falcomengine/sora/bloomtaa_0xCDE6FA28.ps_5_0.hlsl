// ---- Created with 3Dmigoto v1.3.16 on Thu Aug 21 00:35:31 2025
#include "../common.hlsl"
Texture2D<uint4> t4 : register(t4);

Texture2D<float4> t3 : register(t3);

Texture2D<float4> t2 : register(t2);

Texture2D<float4> t1 : register(t1);

Texture2D<float4> t0 : register(t0);

SamplerState s1_s : register(s1);

SamplerState s0_s : register(s0);

cbuffer cb2 : register(b2)
{
  float4 cb2[7];
}




// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_Position0,
  float4 v1 : TEXCOORD0,
  out float4 o0 : SV_Target0)
{
  float4 r0,r1,r2,r3,r4,r5;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.xyzw = -cb2[4].zwxy + v1.xyxy;
  r1.xyzw = t0.SampleLevel(s1_s, r0.zw, 0).xyzw;
  // r1.xyzw = saturate(r1.xyzw);
  r0.xy = saturate(r0.xy);
  r2.z = t1.SampleLevel(s0_s, r0.xy, 0).x;
  r3.xyzw = saturate(cb2[4].zwzw * float4(1,-1,-1,1) + v1.xyxy);
  r4.z = t1.SampleLevel(s0_s, r3.xy, 0).x;
  r3.z = t1.SampleLevel(s0_s, r3.zw, 0).x;
  r0.xy = saturate(cb2[4].zw + v1.xy);
  r0.y = t1.SampleLevel(s0_s, r0.xy, 0).x;
  r5.z = t1.SampleLevel(s0_s, v1.xy, 0).x;
  r2.w = cmp(r2.z >= r5.z);
  r2.w = r2.w ? 1.000000 : 0;
  r5.xy = float2(0,0);
  r2.xy = float2(-1,-1);
  r2.xyz = r2.xyz + -r5.yyz;
  r2.xyz = r2.www * r2.xyz + r5.xyz;
  r2.w = cmp(r4.z >= r2.z);
  r2.w = r2.w ? 1.000000 : 0;
  r4.xy = float2(1,-1);
  r4.xyz = r4.xyz + -r2.yyz;
  r2.xyz = r2.www * r4.xyz + r2.xyz;
  r2.w = cmp(r3.z >= r2.z);
  r2.w = r2.w ? 1.000000 : 0;
  r3.xy = float2(-1,1);
  r3.xyz = r3.xyz + -r2.xyz;
  r2.xyz = r2.www * r3.xyz + r2.xyz;
  r2.w = cmp(r0.y >= r2.z);
  r2.w = r2.w ? 1.000000 : 0;
  r0.x = 1;
  r3.xyz = r0.xxy + -r2.xyz;
  r2.xyz = r2.www * r3.xyz + r2.xyz;
  r0.xy = r2.xy * cb2[4].zw + v1.xy;
  t4.GetDimensions(0, fDest.x, fDest.y, fDest.z);
  r2.xy = fDest.xy;
  r2.xy = r2.xy * r0.xy;
  r3.xy = (int2)r2.xy;
  r3.zw = float2(0,0);
  r2.x = t4.Load(r3.xyz).w;
  r2.x = (int)r2.x & 8;
  r2.x = cmp((int)r2.x == 0);
  r2.y = cmp(0 < r2.z);
  r2.x = r2.y ? r2.x : 0;
  if (r2.x != 0) {
    o0.xyzw = r1.xyzw;
    return;
  }
  r0.xy = t3.SampleLevel(s1_s, r0.xy, 0).xy;
  r2.xy = v1.xy * cb2[6].xy + -r0.xy;
  r2.xyzw = t2.SampleLevel(s1_s, r2.xy, 0).xyzw;
  r3.xy = saturate(-cb2[4].zw * float2(0.5,0.5) + r0.zw);
  r3.xyz = t0.SampleLevel(s1_s, r3.xy, 0).xyz;
  // r3.xyz = saturate(r3.xyz);
  r0.zw = saturate(cb2[4].zw * float2(0.5,0.5) + r0.zw);
  r4.xyz = t0.SampleLevel(s1_s, r0.zw, 0).xyz;
  // r4.xyz = saturate(r4.xyz);
  r5.xyz = r4.xyz + r3.xyz;
  r0.x = dot(r0.xy, r0.xy);
  r0.x = sqrt(r0.x);
  r0.yzw = r5.xyz * float3(4,4,4) + -r1.xyz;
  r0.yzw = float3(0.142857,0.142857,0.142857) * r0.yzw;
  r3.w = 100 * r0.x;
  r3.w = min(1, r3.w);
  r3.w = r3.w * -3.75 + 4;
  // r0.y = dot(r0.yzw, float3(0.298999995,0.587000012,0.114));
  // r0.z = dot(r1.xyz, float3(0.298999995,0.587000012,0.114));
  r0.y = renodx::color::y::from::BT709(renodx::color::srgb::DecodeSafe(r0.yzw));
  r0.z = renodx::color::y::from::BT709(renodx::color::srgb::DecodeSafe(r1.xyz));
  r0.y = r0.y + -r0.z;
  r5.xyz = min(r4.xyz, r3.xyz);
  r5.xyz = -r3.www * abs(r0.yyy) + r5.xyz;
  r3.xyz = max(r4.xyz, r3.xyz);
  r0.yzw = r3.www * abs(r0.yyy) + r3.xyz;
  r3.xyz = r0.yzw + r5.xyz;
  r4.xyz = float3(0.5,0.5,0.5) * r3.xyz;
  r0.yzw = r0.yzw + -r5.xyz;
  r0.yzw = float3(0.5,0.5,0.5) * r0.yzw;
  r3.xyz = -r3.xyz * float3(0.5,0.5,0.5) + r2.xyz;
  r5.xyz = float3(9.99999975e-005,9.99999975e-005,9.99999975e-005) + r3.xyz;
  r0.yzw = r0.yzw / r5.xyz;
  r0.z = min(abs(r0.z), abs(r0.w));
  r0.y = min(abs(r0.y), r0.z);
  r0.y = min(1, r0.y);
  r2.xyz = r3.xyz * r0.yyy + r4.xyz;
  r0.x = cb2[5].z * -r0.x;
  r0.x = 1.44269502 * r0.x;
  r0.x = exp2(r0.x);
  r0.y = cb2[5].x + -cb2[5].y;
  r0.x = r0.x * r0.y + cb2[5].y;
  r2.xyzw = r2.xyzw + -r1.xyzw;
  o0.xyzw = r0.xxxx * r2.xyzw + r1.xyzw;
  return;
}