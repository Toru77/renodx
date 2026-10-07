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

cbuffer cb_scene : register(b0)
{
  float4x4 view_g : packoffset(c0);
  float4x4 viewInv_g : packoffset(c4);
  float4x4 proj_g : packoffset(c8);
  float4x4 projInv_g : packoffset(c12);
  float4x4 viewProj_g : packoffset(c16);
  float4x4 viewProjInv_g : packoffset(c20);
  float2 vpSize_g : packoffset(c24);
  float2 invVPSize_g : packoffset(c24.z);
  float3 lightColor_g : packoffset(c25);
  float disableMapObjNearFade_g : packoffset(c25.w);
  float3 lightDirection_g : packoffset(c26);
  float gameTime_g : packoffset(c26.w);
  float3 sceneShadowColor_g : packoffset(c27);
  int shadowmapCascadeCount_g : packoffset(c27.w);
  float3 windDirection_g : packoffset(c28);
  float sceneTime_g : packoffset(c28.w);
  float2 lightTileSizeInv_g : packoffset(c29);
  float fogNearDistance_g : packoffset(c29.z);
  float fogFadeRangeInv_g : packoffset(c29.w);
  float3 fogColor_g : packoffset(c30);
  float fogIntensity_g : packoffset(c30.w);
  float fogHeight_g : packoffset(c31);
  float fogHeightRangeInv_g : packoffset(c31.y);
  float windWaveTime_g : packoffset(c31.z);
  float windWaveFrequency_g : packoffset(c31.w);
  float fogExp_g : packoffset(c32);
  float lightSpecularGlossiness_g : packoffset(c32.y);
  float lightSpecularIntensity_g : packoffset(c32.z);
  float localShadowResolutionInv_g : packoffset(c32.w);
  float4x4 ditherMtx_g : packoffset(c33);
  float4 lightProbe_g[9] : packoffset(c37);
  float3 chrLightDir_g : packoffset(c46);
  float windForce_g : packoffset(c46.w);
  float4 mapColor_g : packoffset(c47);
  float4 clipPlane_g : packoffset(c48);
  float2 resolutionScaling_g : packoffset(c49);
  float2 invShadowSize_g : packoffset(c49.z);
  float3 chrShadowColor_g : packoffset(c50);
  float shadowFadeNear_g : packoffset(c50.w);
  float4 frustumPlanes_g[6] : packoffset(c51);
  float3 shadowSplitDistance_g : packoffset(c57);
  float shadowFadeRangeInv_g : packoffset(c57.w);
  float4x4 shadowMtx_g[4] : packoffset(c58);
  float2 cloudShadowOffset_g : packoffset(c74);
  float cloudShadowScale_g : packoffset(c74.z);
  float4x4 prevViewProj_g : packoffset(c75);
  float2 jitterDiff_g : packoffset(c79);
  float4 shadowBlurRadius_g : packoffset(c80);
}

StructuredBuffer<uint> instanceIndices_g : register(t2);
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
  out uint o3 : TEXCOORD2)
{
// Needs manual fix for instruction:
// unknown dcl_: dcl_input_sgv v2.x, instance_id
  float4 r0,r1,r2,r3;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.x = (int)v2.x + instanceOffset_g;
  r0.x = instanceIndices_g[r0.x].x;
  r0.y = instances_g[r0.x].param.z;
  r0.y = cmp(r0.y < 0);
  r1.x = instances_g[r0.x].world._m00;
  r1.y = instances_g[r0.x].world._m10;
  r1.z = instances_g[r0.x].world._m20;
  r1.w = instances_g[r0.x].world._m30;
  r2.xyz = v0.xyz;
  r2.w = 1;
  r1.x = dot(r2.xyzw, r1.xyzw);
  r3.x = instances_g[r0.x].world._m01;
  r3.y = instances_g[r0.x].world._m11;
  r3.z = instances_g[r0.x].world._m21;
  r3.w = instances_g[r0.x].world._m31;
  r1.y = dot(r2.xyzw, r3.xyzw);
  r3.x = instances_g[r0.x].world._m02;
  r3.y = instances_g[r0.x].world._m12;
  r3.z = instances_g[r0.x].world._m22;
  r3.w = instances_g[r0.x].world._m32;
  o3.x = r0.x;
  r1.z = dot(r2.xyzw, r3.xyzw);
  r1.w = 1;
  r2.x = dot(r1.xyzw, viewProj_g._m00_m10_m20_m30);
  r2.y = dot(r1.xyzw, viewProj_g._m01_m11_m21_m31);
  r2.z = dot(r1.xyzw, viewProj_g._m02_m12_m22_m32);
  r2.w = dot(r1.xyzw, viewProj_g._m03_m13_m23_m33);
  o2.xyzw = r1.xyzw;
  o0.xyzw = r0.yyyy ? float4(0,0,0,0) : r2.xyzw;
  o1.xy = v1.xy;
  return;
}