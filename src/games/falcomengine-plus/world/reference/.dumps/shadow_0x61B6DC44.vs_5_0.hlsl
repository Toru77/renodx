// ---- Created with 3Dmigoto v1.4.1 on Tue Oct  6 16:15:31 2026



// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : POSITION0,
  out float4 o0 : SV_Position0,
  out float2 o1 : TEXCOORD0)
{
  o0.xy = v0.xy;
  o0.zw = float2(0,1);
  o1.xy = v0.zw;
  return;
}