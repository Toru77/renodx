// ---- Created with 3Dmigoto v1.4.1 on Fri Apr 10 00:37:11 2026

cbuffer cbDof : register(b2)
{
  float4 g_vDofInfo0 : packoffset(c0);
  float4 g_vDofInfo1 : packoffset(c1);
  float4 g_vDofInfo2 : packoffset(c2);
  float4 g_vD2Z_Z2D : packoffset(c3);
  float4 g_vTexelSize : packoffset(c4);
  float4 g_vAnamorphicInfo : packoffset(c5);
  float4 g_vSampleInfo : packoffset(c6);
  float4 g_vHeatSimmerInfo : packoffset(c7);
  float4 g_vMaxUV : packoffset(c8);
}

SamplerState sampleLinear_s : register(s7);
Texture2D<float4> g_t4MainMap : register(t0);


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_Position0,
  float2 v1 : TEXCOORD0,
  float2 w1 : TEXCOORD1,
  out float4 o0 : SV_Target0)
{ 
  float4 r0,r1,r2;
  uint4 bitmask, uiDest;
  float4 fDest;
  

  return;
}