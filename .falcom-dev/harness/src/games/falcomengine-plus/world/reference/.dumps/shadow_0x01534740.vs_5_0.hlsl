// ---- Created with 3Dmigoto v1.4.1 on Mon Oct  5 13:56:17 2026

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
  float3 shadowColor_g : packoffset(c4);
  float glowShadowFadeRatio_g : packoffset(c4.w);
  float3 rimLightColor_g : packoffset(c5);
  float rimLightPower_g : packoffset(c5.w);
  float3 specularColor_g : packoffset(c6);
  float specularShadowFadeRatio_g : packoffset(c6.w);
  float rimIntensity_g : packoffset(c7);
  float dynamicLightIntensity_g : packoffset(c7.y);
  float fresnel0_g : packoffset(c7.z);
  float specularGlossiness0_g : packoffset(c7.w);
  float fresnel1_g : packoffset(c8);
  float specularGlossiness1_g : packoffset(c8.y);
  float fresnel2_g : packoffset(c8.z);
  float specularGlossiness2_g : packoffset(c8.w);
  float planarMap1Density_g : packoffset(c9);
  float planarMap1FadeExp_g : packoffset(c9.y);
  float planarMap1FadeLevel_g : packoffset(c9.z);
  float planarMap2Density_g : packoffset(c9.w);
  float planarMap2FadeExp_g : packoffset(c10);
  float planarMap2FadeLevel_g : packoffset(c10.y);
  float shadowCastOffset_g : packoffset(c10.z);
  float volumeFogInvalidity_g : packoffset(c10.w);
}

cbuffer cb_shadow : register(b2)
{
  float4x4 shadowViewProj_g : packoffset(c0);
  float shadowAlphaTestEnable_g : packoffset(c4);
}

StructuredBuffer<InstanceParam> instances_g : register(t15);


// 3Dmigoto declarations
#define cmp -


void main(
  float3 v0 : POSITION0,
  uint v1 : SV_InstanceID0,
  out float4 o0 : SV_Position0,
  out float2 o1 : TEXCOORD3)
{
// Needs manual fix for instruction:
// unknown dcl_: dcl_input_sgv v1.x, instance_id
  float4 r0,r1,r2,r3;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.x = 0.00999999978 + shadowCastOffset_g;
  r1.xyz = v0.xyz;
  r1.w = 1;
  r0.y = (int)v1.x + instanceOffset_g;
  r2.x = instances_g[r0.y].world._m00;
  r2.y = instances_g[r0.y].world._m10;
  r2.z = instances_g[r0.y].world._m20;
  r2.w = instances_g[r0.y].world._m30;
  r2.x = dot(r1.xyzw, r2.xyzw);
  r3.x = instances_g[r0.y].world._m01;
  r3.y = instances_g[r0.y].world._m11;
  r3.z = instances_g[r0.y].world._m21;
  r3.w = instances_g[r0.y].world._m31;
  r2.y = dot(r1.xyzw, r3.xyzw);
  r3.x = instances_g[r0.y].world._m02;
  r3.y = instances_g[r0.y].world._m12;
  r3.z = instances_g[r0.y].world._m22;
  r3.w = instances_g[r0.y].world._m32;
  r2.z = dot(r1.xyzw, r3.xyzw);
  r1.xyz = lightDirection_g.xyz * r0.xxx + r2.xyz;
  r1.w = 1;
  o0.x = dot(r1.xyzw, shadowViewProj_g._m00_m10_m20_m30);
  o0.y = dot(r1.xyzw, shadowViewProj_g._m01_m11_m21_m31);
  o0.z = dot(r1.xyzw, shadowViewProj_g._m02_m12_m22_m32);
  o0.w = dot(r1.xyzw, shadowViewProj_g._m03_m13_m23_m33);
  o1.x = instances_g[r0.y].param.z;
  o1.y = instances_g[r0.y].color.w;
  return;
}