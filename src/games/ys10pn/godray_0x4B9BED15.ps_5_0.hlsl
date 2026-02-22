// ---- Created with 3Dmigoto v1.4.1 on Sun Feb 22 20:40:13 2026

cbuffer cb_godray : register(b2)
{
  float2 base_resolution_g : packoffset(c0);
  float2 mrt_resolution_g : packoffset(c0.z);
  float2 blur_center_g : packoffset(c1);
  float2 blur_scale_g : packoffset(c1.z);
  float z_threshold_g : packoffset(c2);
  float luminance_threshold_g : packoffset(c2.y);
  float centric_sharpness_g : packoffset(c2.z);
  float is_flip_g : packoffset(c2.w);
  float blend_intensity_g : packoffset(c3);
}

SamplerState samLinear_s : register(s0);
SamplerState samPoint_s : register(s1);
Texture2D<float4> colorTexture : register(t0);
Texture2D<float4> blurTexture : register(t1);


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_Position0,
  float4 v1 : TEXCOORD0,
  out float4 o0 : SV_Target0)
{
  float4 r0,r1,r2;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.xyz = blurTexture.SampleLevel(samLinear_s, v1.xy, 0).xyz;
  r1.xyz = blend_intensity_g * r0.xyz;
  r2.xyz = colorTexture.SampleLevel(samPoint_s, v1.zw, 0).xyz;
  r1.xyz = r2.xyz * r1.xyz;
  r0.xyz = r0.xyz * blend_intensity_g + r2.xyz;
  o0.xyz = -r1.xyz + r0.xyz;
  o0.w = 1;
  return;
}
