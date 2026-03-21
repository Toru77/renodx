// ---- Created with 3Dmigoto v1.4.1 on Sun Mar 22 00:32:12 2026

cbuffer cb_scene : register(b1)
{
  int4 indices_g[9] : packoffset(c0);
  float4x4 grid_proj_inv_g[9] : packoffset(c9);
  float4x4 cascade_proj_g : packoffset(c45);
}

SamplerState smpl_s : register(s0);
Texture2DArray<float4> tex : register(t0);


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_Position0,
  float2 v1 : TEXCOORD0,
  out float oDepth : SV_Depth)
{
  float4 r0,r1;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.xy = floor(v1.xy);
  r0.xy = (int2)r0.xy;
  r0.x = mad((int)r0.y, 3, (int)r0.x);
  r0.x = max(0, (int)r0.x);
  r0.x = min(8, (int)r0.x);
  r1.z = (int)indices_g[r0.x].x;
  r0.x = (uint)indices_g[r0.x].x << 2;
  r1.xy = frac(v1.xy);
  r1.x = tex.SampleLevel(smpl_s, r1.xyz, 0).x;
  r1.yw = float2(1,1);
  r1.z = dot(grid_proj_inv_g[r0.x/4]._m22_m32, r1.xy);
  oDepth = dot(cascade_proj_g._m22_m32, r1.zw);
  return;
}