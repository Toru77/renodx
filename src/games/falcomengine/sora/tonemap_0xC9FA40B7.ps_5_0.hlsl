// ---- Created with 3Dmigoto v1.3.16 on Thu Aug 21 00:51:03 2025
#include "../common.hlsl"
SamplerState smpl_s : register(s0);
Texture2D<float4> tex : register(t0);


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_Position0,
  float2 v1 : TEXCOORD0,
  out float4 o0 : SV_Target0)
{
  float4 r0;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.xyz = tex.SampleLevel(smpl_s, v1.xy, 0).xyz;
  o0.xyz = r0.xyz;

  // if (RENODX_TONE_MAP_TYPE > 0) {
    
  //   bool decoding = true;
  //   o0.rgb = processAndToneMap(o0.rgb, decoding);
  // }

  o0.w = 1;
  return;
}