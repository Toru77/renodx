// ---- Created with 3Dmigoto v1.4.1 on Mon Sep 21 14:57:50 2026

SamplerState samPoint_s : register(s0);
Texture2D<float4> depthTexture : register(t0);
Texture2D<uint4> mrtTexture0 : register(t1);
Texture2D<uint2> mrtTexture2 : register(t2);


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_Position0,
  float4 v1 : TEXCOORD0,
  out float2 o0 : SV_Target0)
{
  float4 r0,r1;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.y = depthTexture.SampleLevel(samPoint_s, v1.xy, 0).x;
  mrtTexture0.GetDimensions(0, fDest.x, fDest.y, fDest.z);
  r0.zw = fDest.xy;
  r0.zw = v1.xy * r0.zw;
  r1.xy = (int2)r0.zw;
  r1.zw = float2(0,0);
  r0.z = mrtTexture0.Load(r1.xyz).w;
  r0.z = (int)r0.z & 1;
  if (r0.z != 0) {
    mrtTexture2.GetDimensions(0, fDest.x, fDest.y, fDest.z);
    r0.zw = fDest.xy;
    r0.zw = v1.xy * r0.zw;
    r1.xy = (int2)r0.zw;
    r1.zw = float2(0,0);
    r0.z = mrtTexture2.Load(r1.xyz).y;
    r0.z = (int)r0.z & 255;
    r0.z = (uint)r0.z;
    r0.x = 0.00392156886 * r0.z;
  } else {
    r0.x = -100;
  }
  o0.xy = r0.yx;
  return;
}