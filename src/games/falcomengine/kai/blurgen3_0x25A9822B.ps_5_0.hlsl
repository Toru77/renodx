// ---- Created with 3Dmigoto v1.3.16 on Fri Dec 19 23:27:48 2025
#include "../common.hlsl"
cbuffer cb_local : register(b2)
{
  float4 offsetsAndWeights[7] : packoffset(c0);
  float2 uv_clamp : packoffset(c7);
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

  // r0.xy = offsetsAndWeights[1].xy + v1.xy;
  // r0.xy = min(uv_clamp.xy, r0.xy);
  // r0.xyzw = colorTexture.SampleLevel(samLinear_s, r0.xy, 0).xyzw;
  // r0.xyzw = offsetsAndWeights[1].zzzz * r0.xyzw;
  // r1.xy = offsetsAndWeights[0].xy + v1.xy;
  // r1.xy = min(uv_clamp.xy, r1.xy);
  // r1.xyzw = colorTexture.SampleLevel(samLinear_s, r1.xy, 0).xyzw;
  // r0.xyzw = r1.xyzw * offsetsAndWeights[0].zzzz + r0.xyzw;
  // r1.xy = offsetsAndWeights[2].xy + v1.xy;
  // r1.xy = min(uv_clamp.xy, r1.xy);
  // r1.xyzw = colorTexture.SampleLevel(samLinear_s, r1.xy, 0).xyzw;
  // r0.xyzw = r1.xyzw * offsetsAndWeights[2].zzzz + r0.xyzw;
  // r1.xy = offsetsAndWeights[3].xy + v1.xy;
  // r1.xy = min(uv_clamp.xy, r1.xy);
  // r1.xyzw = colorTexture.SampleLevel(samLinear_s, r1.xy, 0).xyzw;
  // r0.xyzw = r1.xyzw * offsetsAndWeights[3].zzzz + r0.xyzw;
  // r1.xy = offsetsAndWeights[4].xy + v1.xy;
  // r1.xy = min(uv_clamp.xy, r1.xy);
  // r1.xyzw = colorTexture.SampleLevel(samLinear_s, r1.xy, 0).xyzw;
  // r0.xyzw = r1.xyzw * offsetsAndWeights[4].zzzz + r0.xyzw;
  // r1.xy = offsetsAndWeights[5].xy + v1.xy;
  // r1.xy = min(uv_clamp.xy, r1.xy);
  // r1.xyzw = colorTexture.SampleLevel(samLinear_s, r1.xy, 0).xyzw;
  // r0.xyzw = r1.xyzw * offsetsAndWeights[5].zzzz + r0.xyzw;
  // r1.xy = offsetsAndWeights[6].xy + v1.xy;
  // r1.xy = min(uv_clamp.xy, r1.xy);
  // r1.xyzw = colorTexture.SampleLevel(samLinear_s, r1.xy, 0).xyzw;
  // o0.xyzw = r1.xyzw * offsetsAndWeights[6].zzzz + r0.xyzw;

  float4 acc = 0;
  float wsum = 0;

  [unroll]
  for (int i = 0; i < 7; ++i) {
    float2 uv = v1.xy + offsetsAndWeights[i].xy;  // fixed offsets from center
    uv.xy = min(uv_clamp.xy, uv.xy);

    float w = offsetsAndWeights[i].z;
    float4 c = colorTexture.SampleLevel(samLinear_s, uv, 0);

    if (RENODX_TONE_MAP_TYPE == 0 || shader_injection.bloom == 0.f) {
      c = saturate(c);
    } else {
      c = saturate(c);
    }

    acc += c * w;
    wsum += w;
  }

  // If weights are pre-normalized, accW≈1 and this is a no-op; otherwise it keeps brightness consistent.
  float4 outRGB = (wsum > 0.0) ? (acc / wsum) : 0.0;
  // float4 outRGB = acc;

  if (RENODX_TONE_MAP_TYPE == 0 || shader_injection.bloom == 0.f) {
    o0 = saturate(outRGB);
  } else {
    o0 = saturate(outRGB);
  }
  return;
}