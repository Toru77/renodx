// ---- Created with 3Dmigoto v1.4.1 on Sun Apr  5 23:26:23 2026
Texture2D<float4> t6 : register(t6);

Texture2D<float4> t5 : register(t5);

Texture2D<float4> t4 : register(t4);

Texture2D<float4> t3 : register(t3);

Texture2D<float4> t2 : register(t2);

Texture2D<float4> t1 : register(t1);

Texture2D<float4> t0 : register(t0);

SamplerState s6_s : register(s6);

SamplerState s5_s : register(s5);

SamplerState s4_s : register(s4);

SamplerState s3_s : register(s3);

SamplerState s2_s : register(s2);

SamplerState s1_s : register(s1);

SamplerState s0_s : register(s0);

cbuffer cb0 : register(b0)
{
  float4 cb0[6];
}




// 3Dmigoto declarations
#define cmp -
#include "./common.hlsl"

void main(
  float4 v0 : SV_POSITION0,
  float2 v1 : TEXCOORD0,
  out float4 o0 : SV_Target0)
{
  float4 r0,r1,r2,r3;
  uint4 bitmask, uiDest;
  float4 fDest;
  r0.xyzw = t5.Sample(s0_s, v1.xy).xyzw;
  r0.zw = v1.xy + r0.xy;
  r1.xy = saturate(r0.zw);
  r1.xyzw = t5.Sample(s0_s, r1.xy).xyzw;
  r1.xy = cmp(r1.xy == float2(0,0));
  r1.x = (int)r1.y | (int)r1.x;
  r1.yz = cmp(float2(1,1) < r0.zw);
  r0.zw = cmp(r0.zw < float2(0,0));
  r0.z = (int)r0.w | (int)r0.z;
  r0.w = (int)r1.z | (int)r1.y;
  r0.w = (int)r0.w | (int)r1.x;
  r0.z = (int)r0.z | (int)r0.w;
  r0.xy = r0.zz ? float2(0,0) : r0.xy;
  r0.xy = v1.xy + r0.xy;
  r1.xyzw = t3.Sample(s4_s, r0.xy).xyzw;
  r1.xyz = cb0[5].zxy * r1.zxy;
  r2.xyzw = t1.Sample(s2_s, r0.xy).xyzw;
  r1.xyz = r1.xyz * cb0[3].xxx + r2.zxy;
  r3.xyzw = t2.Sample(s3_s, r0.xy).xyzw;
  r0.xyzw = t6.Sample(s6_s, r0.xy).xyzw;
  r1.xyz = r1.xyz * r3.www + r3.zxy;
  r1.w = -r3.w + r2.w;
  r1.w = 1 + r1.w;
  o0.w = min(1, r1.w);
  r0.xyz = r1.xyz * r0.www + r0.zxy;
  r0.xyz = min(float3(65504,65504,65504), r0.xyz);
  r1.xy = v1.xy * float2(2,2) + float2(-1,-1);
  r1.xy = cb0[4].xy * r1.xy;
  r0.w = dot(r1.xy, r1.xy);
  r0.w = 1 + r0.w;
  r0.w = 1 / r0.w;
  r0.w = r0.w * r0.w;
  r0.xyz = r0.xyz * r0.www;
  r1.xyzw = t0.Sample(s1_s, float2(0,0)).xyzw;
  r0.xyz = r1.zxy * r0.xyz;
  r0.xyz = cb0[3].zzz * r0.xyz;
  r0.xyz = r0.xyz * float3(5.55555582,5.55555582,5.55555582) + float3(0.0479959995,0.0479959995,0.0479959995);
  r0.xyz = max(float3(0,0,0), r0.xyz);
  float3 colorSDRNeutral = r0.xyz;
  float3 colorHDR = r0.xyz;
  // Test: output HDR color directly for inspection
  //o0.xyz = colorHDR;
  //o0.w = 1;
  //return;
  r0.xyz = log2(r0.xyz);
  r0.xyz = saturate(r0.xyz * float3(0.0734997839,0.0734997839,0.0734997839) + float3(0.386036009,0.386036009,0.386036009));
  r1.xw = float2(31,0.96875) * r0.xz;
  r0.w = floor(r1.x);
  r1.yz = r0.yz * float2(0.0302734375,0.96875) + float2(0.00048828125,0.015625);
  r0.x = r0.x * 31 + -r0.w;
  r1.x = r0.w * 0.03125 + r1.y;
  r2.xyzw = t4.SampleLevel(s5_s, r1.xz, 0).xyzw;
  r0.yz = float2(0.03125,0.015625) + r1.xw;
  r1.xyzw = t4.SampleLevel(s5_s, r0.yz, 0).xyzw;
  r0.yzw = r1.xyz + -r2.xyz;
  o0.xyz = r0.xxx * r0.yzw + r2.xyz;
  float3 colorSDRGraded = o0.xyz;
  o0.xyz = BuildTonemapPayload(colorHDR, colorSDRNeutral, colorSDRGraded);
  o0.xyz = ApplyToneMapPass(o0.xyz);
  o0.xyz = RenderIntermediatePass(o0.xyz);
  return;
}