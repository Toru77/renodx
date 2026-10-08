// ---- Created with 3Dmigoto v1.4.1 on Tue Oct  6 16:15:31 2026

struct InstanceParam
{
    float4x4 world;                // Offset:    0
    float4 color;                  // Offset:   64
    float4 uv;                     // Offset:   80
    float4 param;                  // Offset:   96
    uint boneAddress;              // Offset:  112
    float3 param2;                 // Offset:  116
};

cbuffer cb_instance : register(b1)
{
  int instanceOffset_g : packoffset(c0);
  int maxBoneCount_g : packoffset(c0.y);
}

StructuredBuffer<InstanceParam> instances_g : register(t15);


// 3Dmigoto declarations
#define cmp -


void main(
  float3 v0 : POSITION0,
  float2 v1 : TEXCOORD0,
  uint v2 : SV_InstanceID0,
  out float4 o0 : SV_Position0,
  out float4 o1 : TEXCOORD0,
  out float4 o2 : TEXCOORD1,
  out float4 o3 : TEXCOORD2,
  out int o4 : TEXCOORD3)
{
// Needs manual fix for instruction:
// unknown dcl_: dcl_input_sgv v2.x, instance_id
  float4 r0,r1,r2;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.xyz = v0.xyz;
  r0.w = 1;
  r1.x = (int)v2.x + instanceOffset_g;
  r2.x = instances_g[r1.x].world._m00;
  r2.y = instances_g[r1.x].world._m10;
  r2.z = instances_g[r1.x].world._m20;
  r2.w = instances_g[r1.x].world._m30;
  o0.x = dot(r0.xyzw, r2.xyzw);
  r2.x = instances_g[r1.x].world._m01;
  r2.y = instances_g[r1.x].world._m11;
  r2.z = instances_g[r1.x].world._m21;
  r2.w = instances_g[r1.x].world._m31;
  o0.y = dot(r0.xyzw, r2.xyzw);
  r2.x = instances_g[r1.x].world._m02;
  r2.y = instances_g[r1.x].world._m12;
  r2.z = instances_g[r1.x].world._m22;
  r2.w = instances_g[r1.x].world._m32;
  o0.z = dot(r0.xyzw, r2.xyzw);
  r2.x = instances_g[r1.x].world._m03;
  r2.y = instances_g[r1.x].world._m13;
  r2.z = instances_g[r1.x].world._m23;
  r2.w = instances_g[r1.x].world._m33;
  o0.w = dot(r0.xyzw, r2.xyzw);
  r0.xy = v1.xy * float2(1,-1) + float2(0,1);
  r2.x = instances_g[r1.x].uv.x;
  r2.y = instances_g[r1.x].uv.y;
  r2.z = instances_g[r1.x].uv.z;
  r2.w = instances_g[r1.x].uv.w;
  o1.xy = r0.xy * r2.zw + r2.xy;
  r0.x = instances_g[r1.x].boneAddress;
  r0.y = instances_g[r1.x].param2.x;
  r0.z = instances_g[r1.x].param2.y;
  r0.w = instances_g[r1.x].param2.z;
  r0.x = (int)r0.x & 256;
  o3.xyz = r0.yzw;
  o1.z = r0.x ? 1 : 0;
  o2.x = instances_g[r1.x].color.x;
  o2.y = instances_g[r1.x].color.y;
  o2.z = instances_g[r1.x].color.z;
  o2.w = instances_g[r1.x].color.w;
  r0.x = instances_g[r1.x].param.x;
  o4.x = (int)r0.x;
  return;
}