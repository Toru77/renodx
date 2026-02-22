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
Texture2D<float4> colorTexture : register(t0);


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

  r0.xyzw = blur_center_g.xyxy + -v1.xyxy;
  r0.xyzw = is_flip_g * r0.xyzw;
  r1.x = dot(r0.zw, r0.zw);
  r1.y = rsqrt(r1.x);
  r1.x = sqrt(r1.x);
  r1.x = min(1, r1.x);
  r0.xyzw = r1.yyyy * r0.xyzw;
  r0.xyzw = blur_scale_g.xyxy * r0.xyzw;
  r1.yz = r0.zw * r1.xx + v1.xy;
  r0.xyzw = r0.xyzw * r1.xxxx;
  r1.xyz = colorTexture.SampleLevel(samLinear_s, r1.yz, 0).xyz;
  r1.xyz = float3(0.280000001,0.280000001,0.280000001) * r1.xyz;
  r2.xyz = colorTexture.SampleLevel(samLinear_s, v1.xy, 0).xyz;
  r1.xyz = r2.xyz * float3(0.360000014,0.360000014,0.360000014) + r1.xyz;
  r2.xy = r0.zw * float2(2,2) + v1.xy;
  r0.xyzw = r0.xyzw * float4(4,4,8,8) + v1.xyxy;
  r2.xyz = colorTexture.SampleLevel(samLinear_s, r2.xy, 0).xyz;
  r1.xyz = r2.xyz * float3(0.200000003,0.200000003,0.200000003) + r1.xyz;
  r2.xyz = colorTexture.SampleLevel(samLinear_s, r0.xy, 0).xyz;
  r0.xyz = colorTexture.SampleLevel(samLinear_s, r0.zw, 0).xyz;
  r1.xyz = r2.xyz * float3(0.119999997,0.119999997,0.119999997) + r1.xyz;
  o0.xyz = r0.xyz * float3(0.0399999991,0.0399999991,0.0399999991) + r1.xyz;
  o0.w = 1;
  return;
}