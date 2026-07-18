// ---- Created with 3Dmigoto v1.3.16 on Sun Aug 24 02:48:15 2025
#include "../common.hlsl"

cbuffer cb_local : register(b2)
{
  float4 offsetsAndWeights[15] : packoffset(c0);
  float2 uv_clamp : packoffset(c15);
}

SamplerState samLinear_s : register(s0);
Texture2D<float4> colorTexture : register(t0);


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

  r0.xy = offsetsAndWeights[1].xy + v1.xy;
  r0.xy = min(uv_clamp.xy, r0.xy);
  r0.xyzw = colorTexture.SampleLevel(samLinear_s, r0.xy, 0).xyzw;
  // r0.rgb = srgbDecode(r0.rgb);
  r0.xyzw = offsetsAndWeights[1].zzzz * r0.xyzw;
  r1.xy = offsetsAndWeights[0].xy + v1.xy;
  r1.xy = min(uv_clamp.xy, r1.xy);
  r1.xyzw = colorTexture.SampleLevel(samLinear_s, r1.xy, 0).xyzw;
  // r1.rgb = srgbDecode(r1.rgb);
  r0.xyzw = r1.xyzw * offsetsAndWeights[0].zzzz + r0.xyzw;
  r1.xy = offsetsAndWeights[2].xy + v1.xy;
  r1.xy = min(uv_clamp.xy, r1.xy);
  r1.xyzw = colorTexture.SampleLevel(samLinear_s, r1.xy, 0).xyzw;
  // r1.rgb = srgbDecode(r1.rgb);
  r0.xyzw = r1.xyzw * offsetsAndWeights[2].zzzz + r0.xyzw;
  r1.xy = offsetsAndWeights[3].xy + v1.xy;
  r1.xy = min(uv_clamp.xy, r1.xy);
  r1.xyzw = colorTexture.SampleLevel(samLinear_s, r1.xy, 0).xyzw;
  // r1.rgb = srgbDecode(r1.rgb);
  r0.xyzw = r1.xyzw * offsetsAndWeights[3].zzzz + r0.xyzw;
  r1.xy = offsetsAndWeights[4].xy + v1.xy;
  r1.xy = min(uv_clamp.xy, r1.xy);
  r1.xyzw = colorTexture.SampleLevel(samLinear_s, r1.xy, 0).xyzw;
  // r1.rgb = srgbDecode(r1.rgb);
  r0.xyzw = r1.xyzw * offsetsAndWeights[4].zzzz + r0.xyzw;
  r1.xy = offsetsAndWeights[5].xy + v1.xy;
  r1.xy = min(uv_clamp.xy, r1.xy);
  r1.xyzw = colorTexture.SampleLevel(samLinear_s, r1.xy, 0).xyzw;
  // r1.rgb = srgbDecode(r1.rgb);
  r0.xyzw = r1.xyzw * offsetsAndWeights[5].zzzz + r0.xyzw;
  r1.xy = offsetsAndWeights[6].xy + v1.xy;
  r1.xy = min(uv_clamp.xy, r1.xy);
  r1.xyzw = colorTexture.SampleLevel(samLinear_s, r1.xy, 0).xyzw;
  // r1.rgb = srgbDecode(r1.rgb);
  r0.xyzw = r1.xyzw * offsetsAndWeights[6].zzzz + r0.xyzw;
  r1.xy = offsetsAndWeights[7].xy + v1.xy;
  r1.xy = min(uv_clamp.xy, r1.xy);
  r1.xyzw = colorTexture.SampleLevel(samLinear_s, r1.xy, 0).xyzw;
  // r1.rgb = srgbDecode(r1.rgb);
  r0.xyzw = r1.xyzw * offsetsAndWeights[7].zzzz + r0.xyzw;
  r1.xy = offsetsAndWeights[8].xy + v1.xy;
  r1.xy = min(uv_clamp.xy, r1.xy);
  r1.xyzw = colorTexture.SampleLevel(samLinear_s, r1.xy, 0).xyzw;
  // r1.rgb = srgbDecode(r1.rgb);
  r0.xyzw = r1.xyzw * offsetsAndWeights[8].zzzz + r0.xyzw;
  r1.xy = offsetsAndWeights[9].xy + v1.xy;
  r1.xy = min(uv_clamp.xy, r1.xy);
  r1.xyzw = colorTexture.SampleLevel(samLinear_s, r1.xy, 0).xyzw;
  // r1.rgb = srgbDecode(r1.rgb);
  r0.xyzw = r1.xyzw * offsetsAndWeights[9].zzzz + r0.xyzw;
  r1.xy = offsetsAndWeights[10].xy + v1.xy;
  r1.xy = min(uv_clamp.xy, r1.xy);
  r1.xyzw = colorTexture.SampleLevel(samLinear_s, r1.xy, 0).xyzw;
  // r1.rgb = srgbDecode(r1.rgb);
  r0.xyzw = r1.xyzw * offsetsAndWeights[10].zzzz + r0.xyzw;
  r1.xy = offsetsAndWeights[11].xy + v1.xy;
  r1.xy = min(uv_clamp.xy, r1.xy);
  r1.xyzw = colorTexture.SampleLevel(samLinear_s, r1.xy, 0).xyzw;
  // r1.rgb = srgbDecode(r1.rgb);
  r0.xyzw = r1.xyzw * offsetsAndWeights[11].zzzz + r0.xyzw;
  r1.xy = offsetsAndWeights[12].xy + v1.xy;
  r1.xy = min(uv_clamp.xy, r1.xy);
  r1.xyzw = colorTexture.SampleLevel(samLinear_s, r1.xy, 0).xyzw;
  // r1.rgb = srgbDecode(r1.rgb);
  r0.xyzw = r1.xyzw * offsetsAndWeights[12].zzzz + r0.xyzw;
  r1.xy = offsetsAndWeights[13].xy + v1.xy;
  r1.xy = min(uv_clamp.xy, r1.xy);
  r1.xyzw = colorTexture.SampleLevel(samLinear_s, r1.xy, 0).xyzw;
  // r1.rgb = srgbDecode(r1.rgb);
  r0.xyzw = r1.xyzw * offsetsAndWeights[13].zzzz + r0.xyzw;
  r1.xy = offsetsAndWeights[14].xy + v1.xy;
  r1.xy = min(uv_clamp.xy, r1.xy);
  r1.xyzw = colorTexture.SampleLevel(samLinear_s, r1.xy, 0).xyzw;
  // r1.rgb = srgbDecode(r1.rgb);
  o0.xyzw = r1.xyzw * offsetsAndWeights[14].zzzz + r0.xyzw;

  // o0.rgb = srgbEncode(o0.rgb);
  return;
}