// ---- Created with 3Dmigoto v1.4.1 on Sun Jul 12 19:35:27 2026

struct InstanceParam
{
    float4x4 world;                // Offset:    0
    float4x4 prevWorld;            // Offset:   64
    float4 color;                  // Offset:  128
    float4 uv;                     // Offset:  144
    float4 param;                  // Offset:  160
    uint boneAddress;              // Offset:  176
    float3 param2;                 // Offset:  180
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
  uint localLightProbeCount_g : packoffset(c32);
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
  float4x4 prevViewProj_g : packoffset(c74);
  float2 jitterDiff_g : packoffset(c78);
}

StructuredBuffer<InstanceParam> instances_g : register(t15);


// 3Dmigoto declarations
#define cmp -


void main(
  float3 v0 : POSITION0,
  float3 v1 : NORMAL0,
  float4 v2 : COLOR1,
  uint v3 : SV_InstanceID0,
  out float4 o0 : SV_Position0,
  out float4 o1 : NORMAL0,
  out float4 o2 : TEXCOORD0,
  out float4 o3 : TEXCOORD4,
  out uint4 o4 : TEXCOORD6,
  out float4 o5 : TEXCOORD7,
  out float4 o6 : TEXCOORD8)
{
// Needs manual fix for instruction:
// unknown dcl_: dcl_input_sgv v3.x, instance_id
  float4 r0,r1,r2,r3;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.w = 1;
  r1.x = (int)v3.x + instanceOffset_g;
  r2.x = instances_g[r1.x].world._m00;
  r2.y = instances_g[r1.x].world._m10;
  r2.z = instances_g[r1.x].world._m20;
  r2.w = instances_g[r1.x].world._m30;
  r3.xyz = v0.xyz;
  r3.w = 1;
  r0.x = dot(r3.xyzw, r2.xyzw);
  o1.x = dot(v1.xyz, r2.xyz);
  r2.x = instances_g[r1.x].world._m01;
  r2.y = instances_g[r1.x].world._m11;
  r2.z = instances_g[r1.x].world._m21;
  r2.w = instances_g[r1.x].world._m31;
  r0.y = dot(r3.xyzw, r2.xyzw);
  o1.y = dot(v1.xyz, r2.xyz);
  r2.x = instances_g[r1.x].world._m02;
  r2.y = instances_g[r1.x].world._m12;
  r2.z = instances_g[r1.x].world._m22;
  r2.w = instances_g[r1.x].world._m32;
  r0.z = dot(r3.xyzw, r2.xyzw);
  o1.z = dot(v1.xyz, r2.xyz);
  r2.x = dot(r0.xyzw, viewProj_g._m00_m10_m20_m30);
  r2.y = dot(r0.xyzw, viewProj_g._m01_m11_m21_m31);
  r2.z = dot(r0.xyzw, viewProj_g._m02_m12_m22_m32);
  r2.w = dot(r0.xyzw, viewProj_g._m03_m13_m23_m33);
  o2.xyzw = r0.xyzw;
  o0.xyzw = r2.xyzw;
  o5.xyzw = r2.xyzw;
  o1.w = 0;
  r0.xyz = v2.xyz;
  r0.w = 1;
  r2.x = instances_g[r1.x].color.x;
  r2.y = instances_g[r1.x].color.y;
  r2.z = instances_g[r1.x].color.z;
  r2.w = instances_g[r1.x].color.w;
  o3.xyzw = r2.xyzw * r0.xyzw;
  o4.x = r1.x;
  r0.x = instances_g[r1.x].prevWorld._m00;
  r0.y = instances_g[r1.x].prevWorld._m10;
  r0.z = instances_g[r1.x].prevWorld._m20;
  r0.w = instances_g[r1.x].prevWorld._m30;
  r0.x = dot(r3.xyzw, r0.xyzw);
  r2.x = instances_g[r1.x].prevWorld._m01;
  r2.y = instances_g[r1.x].prevWorld._m11;
  r2.z = instances_g[r1.x].prevWorld._m21;
  r2.w = instances_g[r1.x].prevWorld._m31;
  r0.y = dot(r3.xyzw, r2.xyzw);
  r2.x = instances_g[r1.x].prevWorld._m02;
  r2.y = instances_g[r1.x].prevWorld._m12;
  r2.z = instances_g[r1.x].prevWorld._m22;
  r2.w = instances_g[r1.x].prevWorld._m32;
  r1.x = instances_g[r1.x].prevWorld._m03;
  r1.y = instances_g[r1.x].prevWorld._m13;
  r1.z = instances_g[r1.x].prevWorld._m23;
  r1.w = instances_g[r1.x].prevWorld._m33;
  r0.w = dot(r3.xyzw, r1.xyzw);
  r0.z = dot(r3.xyzw, r2.xyzw);
  o6.x = dot(r0.xyzw, prevViewProj_g._m00_m10_m20_m30);
  o6.y = dot(r0.xyzw, prevViewProj_g._m01_m11_m21_m31);
  o6.z = dot(r0.xyzw, prevViewProj_g._m02_m12_m22_m32);
  o6.w = dot(r0.xyzw, prevViewProj_g._m03_m13_m23_m33);
  return;
}