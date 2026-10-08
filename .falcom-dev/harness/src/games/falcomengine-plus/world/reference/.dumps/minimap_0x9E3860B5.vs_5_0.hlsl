// ---- Created with 3Dmigoto v1.4.1 on Tue Oct  6 16:15:31 2026

struct InstanceParam
{
    float4x3 world;                // Offset:    0
    float4x3 prevWorld;            // Offset:   48
    float4 color;                  // Offset:   96
    float4 uv;                     // Offset:  112
    float4 param;                  // Offset:  128
    uint boneAddress;              // Offset:  144
    float3 param2;                 // Offset:  148
};

cbuffer cb_instance : register(b1)
{
  int instanceOffset_g : packoffset(c0);
  int maxBoneCount_g : packoffset(c0.y);
}

cbuffer cb_minimap : register(b0)
{
  float4x4 viewProj_g : packoffset(c0);
  float lookPosY_g : packoffset(c4);
  float fadeHeight_g : packoffset(c4.y);
  float fadeRangeInv_g : packoffset(c4.z);
  float darkRangeInv_g : packoffset(c4.w);
  float4x4 prevViewProj_g : packoffset(c5);
  float2 jitterDiff_g : packoffset(c9);
  float2 vpSize_g : packoffset(c9.z);
}

StructuredBuffer<InstanceParam> instances_g : register(t15);


// 3Dmigoto declarations
#define cmp -


void main(
  float3 v0 : POSITION0,
  float3 v1 : NORMAL0,
  float2 v2 : TEXCOORD0,
  uint v3 : SV_InstanceID0,
  out float4 o0 : SV_Position0,
  out float3 o1 : NORMAL0,
  out float4 o2 : TEXCOORD0,
  out float4 o3 : TEXCOORD1,
  out float4 o4 : TEXCOORD2,
  out float4 o5 : TEXCOORD3,
  out float4 o6 : TEXCOORD4)
{
// Needs manual fix for instruction:
// unknown dcl_: dcl_input_sgv v3.x, instance_id
  float4 r0,r1,r2,r3;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.xyz = v0.xyz;
  r0.w = 1;
  r1.x = (int)v3.x + instanceOffset_g;
  r2.x = instances_g[r1.x].world._m00;
  r2.y = instances_g[r1.x].world._m10;
  r2.z = instances_g[r1.x].world._m20;
  r2.w = instances_g[r1.x].world._m30;
  r3.x = dot(r0.xyzw, r2.xyzw);
  o1.x = dot(v1.xyz, r2.xyz);
  r2.x = instances_g[r1.x].world._m01;
  r2.y = instances_g[r1.x].world._m11;
  r2.z = instances_g[r1.x].world._m21;
  r2.w = instances_g[r1.x].world._m31;
  r3.y = dot(r0.xyzw, r2.xyzw);
  o1.y = dot(v1.xyz, r2.xyz);
  r2.x = instances_g[r1.x].world._m02;
  r2.y = instances_g[r1.x].world._m12;
  r2.z = instances_g[r1.x].world._m22;
  r2.w = instances_g[r1.x].world._m32;
  o4.x = instances_g[r1.x].color.x;
  o4.y = instances_g[r1.x].color.y;
  o4.z = instances_g[r1.x].color.z;
  o4.w = instances_g[r1.x].color.w;
  r3.z = dot(r0.xyzw, r2.xyzw);
  o1.z = dot(v1.xyz, r2.xyz);
  r3.w = 1;
  r0.x = dot(r3.xyzw, viewProj_g._m00_m10_m20_m30);
  r0.y = dot(r3.xyzw, viewProj_g._m01_m11_m21_m31);
  r0.z = dot(r3.xyzw, viewProj_g._m02_m12_m22_m32);
  r0.w = dot(r3.xyzw, viewProj_g._m03_m13_m23_m33);
  o0.xyzw = r0.xyzw;
  o5.xyzw = r0.xyzw;
  o2.xy = v2.xy;
  o3.xyzw = r3.xyzw;
  o6.x = dot(r3.xyzw, prevViewProj_g._m00_m10_m20_m30);
  o6.y = dot(r3.xyzw, prevViewProj_g._m01_m11_m21_m31);
  o6.z = dot(r3.xyzw, prevViewProj_g._m02_m12_m22_m32);
  o6.w = dot(r3.xyzw, prevViewProj_g._m03_m13_m23_m33);
  return;
}