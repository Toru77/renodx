// ---- Created with 3Dmigoto v1.4.1 on Thu Mar 12 14:27:31 2026

Texture2D<uint4> mrtTexture : register(t0);


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

  mrtTexture.GetDimensions(0, fDest.x, fDest.y, fDest.z);
  r0.xy = fDest.xy;
  r0.zw = float2(0.25,0.25) / r0.xy;
  r0.zw = v1.xy + r0.zw;
  r0.xy = r0.zw * r0.xy;
  r0.xy = (int2)r0.xy;
  r0.zw = float2(0,0);
  r0.xyz = mrtTexture.Load(r0.xyz).xyz;
  if (8 == 0) r0.z = 0; else if (8+8 < 32) {   r0.z = (uint)r0.z << (32-(8 + 8)); r0.z = (uint)r0.z >> (32-8);  } else r0.z = (uint)r0.z >> 8;
  r0.xy = (uint2)r0.xy;
  r0.xy = r0.xy * float2(3.05180438e-05,3.05180438e-05) + float2(-1,-1);
  o0.w = (uint)r0.z;
  r0.x = 3.14159274 * r0.x;
  sincos(r0.x, r0.x, r1.x);
  r0.z = -r0.y * r0.y + 1;
  o0.z = r0.y;
  r0.y = sqrt(r0.z);
  o0.x = r1.x * r0.y;
  o0.y = r0.x * r0.y;
  return;
}