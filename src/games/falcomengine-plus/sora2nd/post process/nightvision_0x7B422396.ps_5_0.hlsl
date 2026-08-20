// ---- Created with 3Dmigoto v1.4.1 on Thu Aug 20 19:36:19 2026

cbuffer cb_nvg : register(b2)
{
  float aspectRatio_g : packoffset(c0);
  float fade_g : packoffset(c0.y);
  float time_g : packoffset(c0.z);
  float intensity_g : packoffset(c0.w);
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

  r0.xy = v1.xy * float2(2,2) + float2(-1,-1);
  r1.x = aspectRatio_g * r0.x;
  r1.y = r0.y;
  r0.y = dot(r1.xy, r1.xy);
  r0.y = sqrt(r0.y);
  r0.y = r0.y * 0.400000006 + -1;
  r0.y = max(0, -r0.y);
  r0.z = r0.y * -2 + 3;
  r0.y = r0.y * r0.y;
  r0.y = r0.z * r0.y;
  r0.y = r0.y * 0.800000012 + 0.200000003;
  r0.x = sin(r0.x);
  r0.x = 43758.5469 * r0.x;
  r0.x = frac(r0.x);
  r0.x = r0.x + r1.y;
  r0.x = time_g * r0.x;
  r0.x = sin(r0.x);
  r0.x = 43758.5469 * r0.x;
  r0.z = sin(time_g);
  r0.z = 43758.5469 * r0.z;
  r0.xz = frac(r0.xz);
  r0.w = sin(r0.z);
  r1.y = -r0.z * 0.0500000007 + 1.5;
  r2.xyzw = colorTexture.SampleLevel(samLinear_s, v1.xy, 0).xyzw;
  r3.xyz = r0.www * float3(0.00999999978,0.00999999978,0.00999999978) + r2.xyz;
  r0.xzw = r0.xxx * float3(0.25,0.25,0.25) + r3.xyz;
  r0.xyz = r0.xzw * r0.yyy;
  r0.xyz = fade_g * r0.xyz;
  r0.x = dot(r0.xyz, float3(0.212599993,0.715200007,0.0722000003));
  r1.xz = float2(0.400000006,0.800000012);
  r0.xyz = r0.xxx * r1.xyz + -r2.xyz;
  o0.xyz = intensity_g * r0.xyz + r2.xyz;
  o0.w = r2.w;
  return;
}