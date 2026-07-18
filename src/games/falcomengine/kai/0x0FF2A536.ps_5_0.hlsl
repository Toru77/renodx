// ---- Created with 3Dmigoto v1.3.16 on Sun May 10 17:42:41 2026

SamplerState smplColor_s : register(s0);
SamplerState smplDepth_s : register(s1);
Texture2D<float4> texColor : register(t0);
Texture2D<float4> texDepth : register(t1);


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_Position0,
  float2 v1 : TEXCOORD0,
  out float4 o0 : SV_Target0,
  out float oDepth : SV_Depth)
{
  float4 r0;
  uint4 bitmask, uiDest;
  float4 fDest;

  o0.xyzw = texColor.SampleLevel(smplColor_s, v1.xy, 0).xyzw;
  r0.x = texDepth.SampleLevel(smplDepth_s, v1.xy, 0).x;
  
  oDepth = r0.x;
  return;
}