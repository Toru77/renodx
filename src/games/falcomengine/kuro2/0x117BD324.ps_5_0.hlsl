// ---- Created with 3Dmigoto v1.3.16 on Mon Sep 29 00:42:31 2025

cbuffer cb_scene : register(b2)
{
  float4x4 view_proj_[2] : packoffset(c0);
  float4x4 view_ : packoffset(c8);
  float4x4 view_inv_ : packoffset(c12);
  float4x4 proj_inv_ : packoffset(c16);
  float4x4 rain_mask_matrix_ : packoffset(c20);
  float2 inv_vp_size_ : packoffset(c24);
  float2 pad : packoffset(c24.z);
}

SamplerState colorSampler_s : register(s0);
SamplerState depthSampler_s : register(s5);
Texture2D<float4> colorMap : register(t0);
Texture2D<float4> depthTexture : register(t5);


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_Position0,
  float4 v1 : TEXCOORD0,
  float4 v2 : TEXCOORD1,
  float4 v3 : COLOR0,
  float4 v4 : COLOR1,
  float4 v5 : TEXCOORD4,
  float4 v6 : TEXCOORD5,
  float4 v7 : TEXCOORD6,
  out float4 o0 : SV_Target0)
{
  float4 r0,r1,r2,r3,r4,r5;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.xyzw = v2.xyzw;
  r1.xyz = v0.xyz;
  r2.xy = v1.xy;
  r2.zw = v1.wz;
  r3.xy = v5.xy;
  r4.w = v4.w;
  r4.yz = v5.zw;
  r1.w = v6.w;
  r2.zw = r2.zw;
  r2.z = r2.z;
  r2.z = 255.000015 * r2.z;
  r2.z = (uint)r2.z;
  r2.z = r2.z;
  r2.w = r2.w;
  r2.w = 255.000015 * r2.w;
  r2.w = (uint)r2.w;
  r3.z = 8;
  r2.w = (uint)r2.w << (int)r3.z;
  r2.z = (int)r2.w | (int)r2.z;
  r2.z = r2.z;
  r2.z = r2.z;
  r2.xy = r2.xy;
  r3.xy = r3.xy;
  r2.xy = r2.xy;
  r2.z = r2.z;
  r2.w = (int)r2.z & 4096;
  r2.w = cmp(0 != (int)r2.w);
  r3.xy = r3.xy;
  r3.z = r2.x;
  r3.z = r3.z;
  r3.w = cmp(r3.z < 0.5);
  r5.x = -r3.z;
  r5.x = 1 + r5.x;
  r3.z = r3.w ? r3.z : r5.x;
  r3.w = cmp(0 < r3.x);
  r3.z = r3.z / r3.x;
  r3.z = min(1, r3.z);
  r3.z = r3.w ? r3.z : 1;
  r3.z = log2(r3.z);
  r3.z = r3.y * r3.z;
  r3.z = exp2(r3.z);
  r3.z = max(0, r3.z);
  r3.z = min(1, r3.z);
  r2.w = r2.w ? r3.z : 1;
  r3.z = (int)r2.z & 8192;
  r3.z = cmp(0 != (int)r3.z);
  r3.w = r2.y;
  r3.w = r3.w;
  r5.x = cmp(r3.w < 0.5);
  r5.y = -r3.w;
  r5.y = 1 + r5.y;
  r3.w = r5.x ? r3.w : r5.y;
  r5.x = cmp(0 < r3.x);
  r3.x = r3.w / r3.x;
  r3.x = min(1, r3.x);
  r3.x = r5.x ? r3.x : 1;
  r3.x = log2(r3.x);
  r3.x = r3.y * r3.x;
  r3.x = exp2(r3.x);
  r3.x = max(0, r3.x);
  r3.x = min(1, r3.x);
  r3.x = r3.z ? r3.x : 1;
  r2.w = r3.x * r2.w;
  r2.w = r2.w;
  r1.w = r1.w;
  r1.w = r1.w;
  r1.w = r1.w;
  r3.x = cmp(0 < r1.w);
  if (r3.x != 0) {
    r1.xyz = r1.xyz;
    r1.w = r1.w;
    r3.z = r1.z;
    r3.xyw = int3(0,0,0);
    r3.z = r3.z;
    r1.z = dot(proj_inv_._m02_m12_m22_m32, r3.xyzw);
    r3.x = dot(proj_inv_._m03_m13_m23_m33, r3.xyzw);
    r1.z = r1.z / r3.x;
    r1.z = -r1.z;
    r1.xy = r1.xy;
    r1.xy = inv_vp_size_.xy * r1.xy;
    r3.x = 0;
    r3.z = depthTexture.SampleLevel(depthSampler_s, r1.xy, r3.x).x;
    r3.z = r3.z;
    r3.xyw = int3(0,0,0);
    r3.z = r3.z;
    r1.x = dot(proj_inv_._m02_m12_m22_m32, r3.xyzw);
    r1.y = dot(proj_inv_._m03_m13_m23_m33, r3.xyzw);
    r1.x = r1.x / r1.y;
    r1.x = -r1.x;
    r1.y = -r1.z;
    r1.x = r1.x + r1.y;
    r1.x = r1.x * r1.w;
    r1.x = max(0, r1.x);
    r1.x = min(1, r1.x);
    r2.w = r2.w * r1.x;
  }
  r2.xy = r2.xy;
  r0.xyzw = r0.xyzw;
  r2.z = r2.z;
  r2.xy = r2.xy;
  r1.x = 1;
  r1.x = (int)r1.x & (int)r2.z;
  r1.x = cmp(0 != (int)r1.x);
  if (r1.x != 0) {
    r2.xy = r2.xy;
    r1.xy = float2(2,2) * r2.xy;
    r1.zw = float2(-1,-1);
    r1.xy = r1.xy + r1.zw;
    r1.y = -r1.y;
    r1.z = 3.14159274;
    r1.w = 0.159154937;
    r3.x = r1.x * r1.x;
    r3.y = r1.y * r1.y;
    r3.x = r3.x + r3.y;
    r3.x = sqrt(r3.x);
    r3.x = -r3.x;
    r2.y = 1 + r3.x;
    r3.x = -r1.y;
    r3.x = max(r3.x, r1.y);
    r3.y = -r1.x;
    r3.z = max(r3.y, r1.x);
    r3.w = min(r3.x, r3.z);
    r5.x = max(r3.x, r3.z);
    r5.x = 1 / r5.x;
    r3.w = r5.x * r3.w;
    r5.x = r3.w * r3.w;
    r5.y = 0.0208350997 * r5.x;
    r5.y = -0.0851330012 + r5.y;
    r5.y = r5.x * r5.y;
    r5.y = 0.180141002 + r5.y;
    r5.y = r5.x * r5.y;
    r5.y = -0.330299497 + r5.y;
    r5.x = r5.x * r5.y;
    r5.x = 0.999866009 + r5.x;
    r3.w = r5.x * r3.w;
    r3.x = cmp(r3.z < r3.x);
    r3.z = -2 * r3.w;
    r3.z = 1.57079637 + r3.z;
    r3.x = r3.x ? r3.z : 0;
    r3.x = r3.x + r3.w;
    r3.y = cmp(r1.x < r3.y);
    r3.y = r3.y ? -3.141593 : 0;
    r3.x = r3.x + r3.y;
    r3.y = min(r1.y, r1.x);
    r1.x = max(r1.y, r1.x);
    r1.y = -r3.y;
    r1.y = cmp(r3.y < r1.y);
    r3.y = -r1.x;
    r1.x = cmp(r1.x >= r3.y);
    r1.x = r1.x ? r1.y : 0;
    r1.y = -r3.x;
    r1.x = r1.x ? r1.y : r3.x;
    r1.x = r1.x + r1.z;
    r2.x = r1.x * r1.w;
    r2.x = r2.x;
    r2.y = r2.y;
    r2.xy = r2.xy;
  }
  r0.zw = r2.yx * r0.wz;
  r0.xy = r0.wz + r0.xy;
  r0.z = 8;
  r0.z = (int)r0.z & (int)r2.z;
  r0.z = cmp(0 != (int)r0.z);
  if (r0.z != 0) {
    r0.xy = r0.yx;
  }
  r0.z = 64;
  r0.z = (int)r0.z & (int)r2.z;
  r0.z = cmp(0 != (int)r0.z);
  if (r0.z != 0) {
    r0.z = -r0.x;
    r0.x = 1 + r0.z;
  }
  r0.z = 512;
  r0.z = (int)r0.z & (int)r2.z;
  r0.z = cmp(0 != (int)r0.z);
  if (r0.z != 0) {
    r0.z = -r0.y;
    r0.y = 1 + r0.z;
  }
  r0.x = r0.x;
  r0.y = r0.y;
  r0.xy = r0.xy;
  r0.z = (int)r2.z & 0x00008000;
  r0.z = cmp(0 != (int)r0.z);
  r4.x = r0.z ? 1 : 0;
  r4.yz = r4.yz;
  r4.w = r4.w;
  r4.yz = r4.yz;
  r4.w = r4.w;
  r4.yzw = r4.yzw;
  r4.x = r4.x;
  r4.yzw = r4.yzw;
  r0.xy = r0.xy;
  r2.w = r2.w;
  r0.xyzw = colorMap.Sample(colorSampler_s, r0.xy).xyzw;
  r1.w = v3.w * r2.w;
  r1.xyz = v3.xyz;
  r0.xyzw = r1.xyzw * r0.xyzw;
  r0.xyz = r0.xyz;
  r1.xyw = v4.yzx + r0.yzx;
  r4.xyzw = r4.xyzw;
  r1.xyw = r1.xyw;
  r0.x = cmp(0 < r4.x);
  if (r0.x != 0) {
    r1.xyw = r1.xyw;
    r2.xyzw = float4(0.666666687,-1,0,-0.333333343);
    r0.x = cmp(r1.x < r1.y);
    r3.xy = r1.yx;
    r3.zw = r2.yx;
    r2.xy = r3.yx;
    r2.xyzw = r0.xxxx ? r3.xyzw : r2.xyzw;
    r0.x = cmp(r1.w < r2.x);
    r1.xyz = r2.xyw;
    r2.xyw = r1.wyx;
    r2.xyzw = r0.xxxx ? r1.yzxw : r2.yzxw;
    r0.x = min(r2.w, r2.x);
    r0.x = -r0.x;
    r0.x = r2.z + r0.x;
    r0.y = 1.00000001e-010;
    r0.z = -r2.x;
    r0.z = r2.w + r0.z;
    r1.z = 6 * r0.x;
    r1.z = r1.z + r0.y;
    r0.z = r0.z / r1.z;
    r0.z = r2.y + r0.z;
    r1.z = -r0.z;
    r2.x = max(r1.z, r0.z);
    r0.y = r2.z + r0.y;
    r2.y = r0.x / r0.y;
    r2.z = r2.z;
    r2.xyz = r2.xyz;
    r0.x = r2.x + r4.y;
    r0.y = cmp(1 < r0.x);
    r0.z = -1;
    r0.z = r0.x + r0.z;
    r0.x = r0.y ? r0.z : r0.x;
    r0.yz = r2.yz * r4.zw;
    r0.x = r0.x;
    r0.yz = r0.yz;
    r2.xyzw = float4(1,0.666666687,0.333333343,3);
    r3.xyz = r2.xyz + r0.xxx;
    r3.xyz = frac(r3.xyz);
    r3.xyz = float3(6,6,6) * r3.xyz;
    r2.yzw = -r2.www;
    r2.yzw = r3.xyz + r2.yzw;
    r3.xyz = -r2.yzw;
    r2.yzw = max(r3.xyz, r2.yzw);
    r3.xyz = -r2.xxx;
    r2.yzw = r3.xyz + r2.yzw;
    r2.yzw = max(float3(0,0,0), r2.yzw);
    r2.yzw = min(float3(1,1,1), r2.yzw);
    r3.xyz = -r2.xxx;
    r2.yzw = r3.xyz + r2.yzw;
    r2.yzw = r2.yzw * r0.yyy;
    r2.xyz = r2.xxx + r2.yzw;
    r1.xyw = r2.yzx * r0.zzz;
    r1.xyw = r1.xyw;
  } else {
    r1.xyw = r1.xyw;
  }
  r0.w = r0.w;
  o0.xyz = r1.wxy;
  o0.w = r0.w;
  return;
}