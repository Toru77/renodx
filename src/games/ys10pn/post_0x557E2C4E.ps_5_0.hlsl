// ---- Created with 3Dmigoto v1.4.1 on Sun Feb 22 21:41:12 2026
#include "common.hlsl"
SamplerState samLinear_s : register(s0);
SamplerState samPoint_s : register(s1);
Texture2D<float4> colorTexture : register(t0);
Texture2D<float4> blurTexture : register(t1);


// 3Dmigoto declarations
#define cmp -

float GetBloomScale() {
  // Default slider 50 -> 0.5, which matches vanilla 5x blur strength.
  return max(0.f, shader_injection.bloom_strength) * 10.f;
}

float3 BlendSdrBloom(float3 color, float3 blur, float bloom_scale) {
  float3 scaled_blur = blur * bloom_scale;
  float3 screened = min(float3(1, 1, 1), color * scaled_blur);
  float3 bloom = max(float3(0, 0, 0), scaled_blur - screened);
  return color + bloom;
}

void main(
  float4 v0 : SV_Position0,
  float4 v1 : TEXCOORD0,
  out float4 o0 : SV_Target0)
{
  float3 blur = blurTexture.SampleLevel(samLinear_s, v1.xy, 0).xyz;
  float4 scene = colorTexture.SampleLevel(samPoint_s, v1.zw, 0).xyzw;
  float bloom_scale = GetBloomScale();

  if (shader_injection.bloom == 0.f) {
    // SDR path: original Falcom-style blend.
    o0.xyz = BlendSdrBloom(scene.xyz, blur, bloom_scale);
  } else {
    // HDR path: additive blend in scene space.
    o0.xyz = scene.xyz + blur * bloom_scale;
  }
  o0.rgb = ApplyPostToneMap(o0.rgb, true);
  o0.w = scene.w;
  return;
}
