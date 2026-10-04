// ---- Created with 3Dmigoto v1.4.1 on Sun Oct  4 09:58:41 2026

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

cbuffer cb_local : register(b5)
{
  float2 uvScroll0_g : packoffset(c0);
  float2 uvScroll1_g : packoffset(c0.z);
  float2 uvScroll2_g : packoffset(c1);
  float emissive_g : packoffset(c1.z);
  float materialFogIntensity_g : packoffset(c1.w);
  float opacity_g : packoffset(c2);
  float translucency_g : packoffset(c2.y);
  float ssaoIntensity_g : packoffset(c2.z);
  uint materialID_g : packoffset(c2.w);
  float3 diffuseMapColor0_g : packoffset(c3);
  float _pad0 : packoffset(c3.w);
  float3 diffuseMapColor1_g : packoffset(c4);
  float _pad1 : packoffset(c4.w);
  float3 diffuseMapColor2_g : packoffset(c5);
  float _pad2 : packoffset(c5.w);
  float3 shadowColor_g : packoffset(c6);
  float glowShadowFadeRatio_g : packoffset(c6.w);
  float3 rimLightColor_g : packoffset(c7);
  float rimLightPower_g : packoffset(c7.w);
  float3 specularColor_g : packoffset(c8);
  float specularShadowFadeRatio_g : packoffset(c8.w);
  float rimIntensity_g : packoffset(c9);
  float dynamicLightIntensity_g : packoffset(c9.y);
  float fresnel0_g : packoffset(c9.z);
  float specularGlossiness0_g : packoffset(c9.w);
  float fresnel1_g : packoffset(c10);
  float specularGlossiness1_g : packoffset(c10.y);
  float fresnel2_g : packoffset(c10.z);
  float specularGlossiness2_g : packoffset(c10.w);
  float shadowCastOffset_g : packoffset(c11);
  float volumeFogInvalidity_g : packoffset(c11.y);
}

StructuredBuffer<InstanceParam> instances_g : register(t15);


// 3Dmigoto declarations
#define cmp -


void main(
  float3 v0 : POSITION0,
  float3 v1 : NORMAL0,
  float3 v2 : TANGENT0,
  float4 v3 : TEXCOORD0,
  float4 v4 : TEXCOORD1,
  float2 v5 : TEXCOORD2,
  float4 v6 : COLOR0,
  float4 v7 : COLOR1,
  uint v8 : SV_InstanceID0,
  out float4 o0 : SV_Position0,
  out float4 o1 : NORMAL0,
  out float4 o2 : TANGENT0,
  out float4 o3 : TEXCOORD0,
  out float4 o4 : TEXCOORD1,
  out float4 o5 : TEXCOORD2,
  out float4 o6 : TEXCOORD3,
  out float4 o7 : TEXCOORD4,
  out uint4 o8 : TEXCOORD6,
  out float4 o9 : TEXCOORD7,
  out float4 o10 : TEXCOORD8)
{
// Needs manual fix for instruction:
// unknown dcl_: dcl_input_sgv v8.x, instance_id
  float4 r0,r1,r2,r3,r4,r5,r6;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.w = 1;
  r1.xyz = v0.xyz;
  r1.w = 1;
  r2.x = (int)v8.x + instanceOffset_g;
  r3.x = instances_g[r2.x].world._m10;
  r3.y = instances_g[r2.x].world._m00;
  r3.z = instances_g[r2.x].world._m20;
  r3.w = instances_g[r2.x].world._m30;
  r0.x = dot(r1.yxzw, r3.xyzw);
  r4.x = instances_g[r2.x].world._m01;
  r4.y = instances_g[r2.x].world._m21;
  r4.z = instances_g[r2.x].world._m11;
  r4.w = instances_g[r2.x].world._m31;
  r0.y = dot(r1.xzyw, r4.xyzw);
  r5.x = instances_g[r2.x].world._m02;
  r5.y = instances_g[r2.x].world._m12;
  r5.z = instances_g[r2.x].world._m22;
  r5.w = instances_g[r2.x].world._m32;
  r0.z = dot(r1.xyzw, r5.xyzw);
  r6.x = dot(r0.xyzw, viewProj_g._m00_m10_m20_m30);
  r6.y = dot(r0.xyzw, viewProj_g._m01_m11_m21_m31);
  r6.z = dot(r0.xyzw, viewProj_g._m02_m12_m22_m32);
  r6.w = dot(r0.xyzw, viewProj_g._m03_m13_m23_m33);
  o3.xyz = r0.xyz;
  o0.xyzw = r6.xyzw;
  o9.xyzw = r6.xyzw;
  r0.x = dot(v1.yxz, r3.xyz);
  r0.y = dot(v1.xzy, r4.xyz);
  r0.z = dot(v1.xyz, r5.xyz);
  r6.x = dot(v2.yxz, r3.xyz);
  r6.y = dot(v2.xzy, r4.xyz);
  r6.z = dot(v2.xyz, r5.xyz);
  r2.yzw = r6.yzx * r0.zxy;
  r2.yzw = r0.yzx * r6.zxy + -r2.yzw;
  o1.xyz = r0.xyz;
  o1.w = r2.y;
  r0.x = cmp(v3.x < 0);
  r0.x = r0.x ? -1 : 1;
  o2.xyz = r6.xyz * r0.xxx;
  o2.w = r2.z;
  o3.w = r2.w;
  r0.x = r3.y;
  r0.y = r4.x;
  r0.z = r5.x;
  r0.x = dot(r0.xyz, r0.xyz);
  r4.x = r3.z;
  r3.y = r4.z;
  r3.z = r5.y;
  r4.z = r5.z;
  r0.y = dot(r4.xyz, r4.xyz);
  r0.z = dot(r3.xyz, r3.xyz);
  r0.xyz = sqrt(r0.xyz);
  r0.x = max(r0.x, r0.z);
  r0.x = max(r0.x, r0.y);
  r3.xy = v3.xy * r0.xx;
  r3.zw = v4.xy * r0.xx;
  o5.xy = v5.xy * r0.xx + uvScroll2_g.xy;
  o4.xyzw = uvScroll0_g.xyzw + r3.xyzw;
  o6.xyzw = v6.xyzw;
  r0.xyz = v7.xyz;
  r0.w = 1;
  r3.x = instances_g[r2.x].color.x;
  r3.y = instances_g[r2.x].color.y;
  r3.z = instances_g[r2.x].color.z;
  r3.w = instances_g[r2.x].color.w;
  o7.xyzw = r3.xyzw * r0.xyzw;
  o8.x = r2.x;
  r0.x = instances_g[r2.x].prevWorld._m00;
  r0.y = instances_g[r2.x].prevWorld._m10;
  r0.z = instances_g[r2.x].prevWorld._m20;
  r0.w = instances_g[r2.x].prevWorld._m30;
  r0.x = dot(r1.xyzw, r0.xyzw);
  r3.x = instances_g[r2.x].prevWorld._m01;
  r3.y = instances_g[r2.x].prevWorld._m11;
  r3.z = instances_g[r2.x].prevWorld._m21;
  r3.w = instances_g[r2.x].prevWorld._m31;
  r2.x = instances_g[r2.x].prevWorld._m02;
  r2.y = instances_g[r2.x].prevWorld._m12;
  r2.z = instances_g[r2.x].prevWorld._m22;
  r2.w = instances_g[r2.x].prevWorld._m32;
  r0.z = dot(r1.xyzw, r2.xyzw);
  r0.y = dot(r1.xyzw, r3.xyzw);
  r0.w = 1;
  o10.x = dot(r0.xyzw, prevViewProj_g._m00_m10_m20_m30);
  o10.y = dot(r0.xyzw, prevViewProj_g._m01_m11_m21_m31);
  o10.z = dot(r0.xyzw, prevViewProj_g._m02_m12_m22_m32);
  o10.w = dot(r0.xyzw, prevViewProj_g._m03_m13_m23_m33);
  return;
}