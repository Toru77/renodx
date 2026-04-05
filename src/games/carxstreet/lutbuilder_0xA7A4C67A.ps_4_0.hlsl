// ---- Created with 3Dmigoto v1.4.1 on Thu Mar  5 20:31:02 2026
cbuffer cb0 : register(b0)
{
  float4 cb0[18];
}




// 3Dmigoto declarations
#define cmp -
#include "./common.hlsl"


void main(
  float4 v0 : SV_POSITION0,
  float2 v1 : TEXCOORD0,
  out float4 o0 : SV_Target0)
{
  float4 r0,r1,r2,r3,r4,r5,r6,r7,r8;
  uint4 bitmask, uiDest;
  float4 fDest;
  // Extract LUT 3D coordinates from the 2D UV map
  r0.x = cb0[0].x * v1.x;
  r0.x = floor(r0.x);
  r1.x = v1.x * cb0[0].x + -r0.x;
  r0.x = cb0[0].z * r0.x;
  r0.z = cb0[0].w * r0.x;
  r1.y = v1.y;
  r0.xy = -cb0[0].zz + r1.xy;
  r1.x = cb0[0].w;
  r1.z = 2;
  
  // Convert coordinates to linear RGB space
  r0.xyz = r0.xyz * r1.xxz + float3(-0.386036009, -0.386036009, -0.386036009);
  r0.xyz = float3(13.6054821, 13.6054821, 13.6054821) * r0.xyz;
  r0.xyz = exp2(r0.xyz);
  r0.xyz = float3(-0.0479959995, -0.0479959995, -0.0479959995) + r0.xyz;
  r0.xyz = float3(0.179999992, 0.179999992, 0.179999992) * r0.xyz;

  // Output the clean linear color directly (Identity pass-through)
  // This bypasses all SDR clamping & custom grading matrices, preserving native HDR
  o0.xyz = max(float3(0, 0, 0), r0.xyz);
  o0.w = 1;
  return;
}