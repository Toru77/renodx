// ---- Created with 3Dmigoto v1.3.16 on Fri Feb 27 15:08:12 2026

cbuffer _Globals : register(b0)
{
  float4 old_imodelview_x : packoffset(c0);
  float4 old_imodelview_y : packoffset(c1);
  float4 old_imodelview_z : packoffset(c2);
  float4 modelviewprojection_x : packoffset(c3);
  float4 modelviewprojection_y : packoffset(c4);
  float4 modelviewprojection_w : packoffset(c5);
  float4 motion_blur_scale : packoffset(c6);
  float4 far_blur_range : packoffset(c7);
  float4 near_blur_range : packoffset(c8);
}

cbuffer shader_common_parameters : register(b1)
{
  float4 s_viewport : packoffset(c0);
  float4 s_depth_range : packoffset(c1);
  float4 s_solid_color : packoffset(c2);
  float4 s_ambient_color : packoffset(c3);
}

cbuffer shader_material_textures_parameters : register(b2)
{
  float4 s_material_textures[16] : packoffset(c0);
}

SamplerState s_sampler_0_s : register(s0);
SamplerState s_sampler_6_s : register(s6);
SamplerState s_sampler_7_s : register(s7);
SamplerState s_sampler_8_s : register(s8);
SamplerState s_sampler_9_s : register(s9);
SamplerState s_sampler_10_s : register(s10);
SamplerState s_sampler_14_s : register(s14);
SamplerState s_sampler_15_s : register(s15);
Texture2D<float4> s_texture_0 : register(t0);
Texture2DMS<float4,2> s_texture_1 : register(t1);
Texture2D<float4> s_texture_6 : register(t6);
Texture2D<float4> s_texture_7 : register(t7);
Texture2D<float4> s_texture_8 : register(t8);
Texture2D<float4> s_texture_9 : register(t9);
Texture2D<float4> s_texture_10 : register(t10);
Texture3D<float4> s_texture_14 : register(t14);
Texture2D<float4> s_texture_15 : register(t15);


// 3Dmigoto declarations
#define cmp -
#include "./common.hlsl"

void main(
  float4 v0 : SV_POSITION0,
  float4 v1 : TEXCOORD0,
  float4 v2 : TEXCOORD1,
  float4 v3 : TEXCOORD2,
  out float4 o0 : SV_TARGET0)
{
  float4 r0,r1,r2,r3;
  uint4 bitmask, uiDest;
  float4 fDest;

  //lens flare stuff
  r0.xy = v1.xy * s_material_textures[9].xy + float2(-0.5,-0.5);
  r0.xy = frac(r0.xy);
  r1.xyzw = r0.xxyy * r0.xxyy;
  r0.zw = r1.yw * r0.xy;
  r2.xy = r1.xz * float2(3,3) + -r0.zw;
  r2.xy = -r0.xy * float2(3,3) + r2.xy;
  r2.zw = float2(6,6) * r1.yw;
  r2.zw = r0.zw * float2(3,3) + -r2.zw;
  r2.xyzw = float4(1,1,4,4) + r2.xyzw;
  r2.xy = r2.xy + r2.zw;
  r2.xy = r2.zw / r2.xy;
  r2.xy = -r2.xy + r0.xy;
  r2.xy = float2(1,1) + r2.xy;
  r2.xy = -r2.xy;
  r3.xy = float2(3,3) * r0.zw;
  r1.xz = r1.xz * float2(3,3) + -r3.xy;
  r1.xz = r0.xy * float2(3,3) + r1.xz;
  r1.xy = r1.yw * r0.xy + r1.xz;
  r1.xy = float2(1,1) + r1.xy;
  r0.zw = r0.zw / r1.xy;
  r1.xy = float2(0.166666672,0.166666672) * r1.xy;
  r0.zw = float2(1,1) + r0.zw;
  r2.zw = r0.zw + -r0.xy;
  r0.xyzw = r2.xyzw * s_material_textures[9].zwzw + v1.xyxy;
  r2.xyz = s_texture_9.Sample(s_sampler_9_s, r0.zw).xyz;
  r3.xyz = s_texture_9.Sample(s_sampler_9_s, r0.zy).xyz;

  r2.xyz = -r3.xyz + r2.xyz;
  r2.xyz = r1.yyy * r2.xyz + r3.xyz;
  r3.xyz = s_texture_9.Sample(s_sampler_9_s, r0.xw).xyz;
  r0.xyz = s_texture_9.Sample(s_sampler_9_s, r0.xy).xyz;

  r3.xyz = r3.xyz + -r0.xyz;
  r0.xyz = r1.yyy * r3.xyz + r0.xyz;
  r1.yzw = r2.xyz + -r0.xyz;
  r0.xyz = r1.xxx * r1.yzw + r0.xyz;

  //lens dirt tex
  r1.xyz = s_texture_10.Sample(s_sampler_10_s, v1.zw).xyz; 
  r0.xyz = r1.xyz * r0.xyz;

  r1.xy = s_viewport.xy * v1.xy;
  r1.xy = (int2)r1.xy;
  r1.zw = float2(0,0);
  r1.xyz = s_texture_1.Load(r1.xy, 0).xyz; //MS data
  
  r0.w = 1.9921875 * r1.x;
  r0.w = floor(r0.w);
  r1.x = -r0.w * 0.501960814 + r1.x;
  r0.w = dot(r1.xyz, float3(1.9921875,0.00778198242,3.03983688e-005));
  r0.w = 1 + -r0.w;
  r1.x = r0.w * r0.w + -far_blur_range.x;
  r0.w = -r0.w * r0.w + near_blur_range.x;
  r0.w = near_blur_range.y * r0.w;
  r1.x = far_blur_range.y * r1.x;
  r1.x = saturate(far_blur_range.w * r1.x);
  r1.x = log2(r1.x);
  r1.x = far_blur_range.z * r1.x;
  r1.x = exp2(r1.x);
  r2.xyzw = s_texture_6.Sample(s_sampler_6_s, v1.xy).xyzw; //dof

  r0.w = max(r2.w, r0.w);
  r0.w = saturate(near_blur_range.w * r0.w);
  r0.w = log2(r0.w);
  r0.w = near_blur_range.z * r0.w;
  r0.w = exp2(r0.w);
  r1.y = r1.x + r0.w;
  r0.w = -r1.x * r0.w + r1.y;
  r1.xyz = s_texture_0.Sample(s_sampler_0_s, v1.xy).xyz; //color

  r2.xyz = r2.xyz + -r1.xyz;
  r1.xyz = r0.www * r2.xyz + r1.xyz; //color or dof blended

  r2.xyz = s_texture_8.Sample(s_sampler_8_s, v1.xy).xyz; //bloom streaks
  r0.w = s_texture_7.Sample(s_sampler_7_s, float2(0.5,0.5)).x; //some mask?
  r1.xyz = r1.xyz * r0.www + r2.xyz * shader_injection.custom_bloom;
  r0.xyz = r0.xyz * shader_injection.custom_lens * r0.www + r1.xyz;

  r0.xyz = T_ResolveLutAndGamma(r0.xyz, s_texture_14, s_sampler_14_s, v1.xy);

  r0.xyz = T_RenderIntermediatePass(r0.xyz);

  o0.xyz = r0.xyz;
  o0.w = 1;
  return;
}