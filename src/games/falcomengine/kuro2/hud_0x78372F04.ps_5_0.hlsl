// ---- Created with 3Dmigoto v1.3.16 on Mon Apr 20 22:12:59 2026

cbuffer cb_local : register(b0)
{
  float4x4 viewProj_g : packoffset(c0);
  float lookPosY_g : packoffset(c4);
  float fadeHeight_g : packoffset(c4.y);
  float fadeRangeInv_g : packoffset(c4.z);
  float darkRangeInv_g : packoffset(c4.w);
}

SamplerState Smpl0_s : register(s0);
Texture2D<float4> Tex0 : register(t0);


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_Position0,
  float3 v1 : NORMAL0,
  float4 v2 : TEXCOORD0,
  float4 v3 : TEXCOORD1,
  float4 v4 : TEXCOORD2,
  out float4 o0 : SV_Target0)
{
  float4 r0,r1;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.x = -lookPosY_g + v3.y;
  r1.xyz = float3(-0,-0,-0);
  r1.w = -fadeHeight_g;
  r0.xyzw = r1.xyzw + abs(r0.xxxx);
  r0.xyzw = -r0.xyzw * darkRangeInv_g + float4(1,1,1,1);
  r0.xyzw = max(float4(0.5,0.5,0.5,0), r0.xyzw);
  r0.xyzw = min(float4(1,1,1,1), r0.xyzw);
  r0.xyzw = v4.xyzw * r0.xyzw;
  r1.xy = v2.xy * float2(1,-1) + float2(0,1);
  r1.xyzw = Tex0.Sample(Smpl0_s, r1.xy).xyzw;
  o0.xyzw = r1.xyzw * r0.xyzw;
  return;
}