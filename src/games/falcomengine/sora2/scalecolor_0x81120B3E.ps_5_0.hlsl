// ---- Created with 3Dmigoto v1.3.16 on Fri Aug 21 00:15:18 2026

SamplerState samPoint_s : register(s1);
Texture2D<float4> colorTexture : register(t0);
Texture2D<float2> param_data_g : register(t4);


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_Position0,
  float4 v1 : TEXCOORD0,
  out float4 o0 : SV_Target0)
{
  float4 r0,r1;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.x = param_data_g.Load(float4(0,0,0,0)).x;
  r1.xyzw = colorTexture.SampleLevel(samPoint_s, v1.xy, 0).xyzw;
  o0.xyz = r1.xyz * r0.xxx;
  o0.w = r1.w;
  return;
}