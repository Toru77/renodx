// ---- Created with 3Dmigoto v1.3.16 on Fri Jun 06 18:49:34 2025

cbuffer _Globals : register(b0)
{
  float4 Color : packoffset(c0);
  float4 DepthValue : packoffset(c1);
}



// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_POSITION0,
  out float4 o0 : SV_TARGET0)
{
  o0.xyzw = Color.xyzw;

  return;
}