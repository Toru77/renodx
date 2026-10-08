// ---- Created with 3Dmigoto v1.4.1 on Tue Oct  6 04:55:37 2026

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
  float3 shadowColor_g : packoffset(c3);
  float glowShadowFadeRatio_g : packoffset(c3.w);
  float3 rimLightColor_g : packoffset(c4);
  float rimLightPower_g : packoffset(c4.w);
  float3 specularColor_g : packoffset(c5);
  float specularShadowFadeRatio_g : packoffset(c5.w);
  float rimIntensity_g : packoffset(c6);
  float dynamicLightIntensity_g : packoffset(c6.y);
  float fresnel0_g : packoffset(c6.z);
  float specularGlossiness0_g : packoffset(c6.w);
  float alphaTestThreshold_g : packoffset(c7);
  float shakeScale_g : packoffset(c7.y);
  float shakeSpeed_g : packoffset(c7.z);
  float shakeFlexibility_g : packoffset(c7.w);
  float shakeFreq_g : packoffset(c8);
  float shakeWindScale_g : packoffset(c8.y);
  float shadowCastOffset_g : packoffset(c8.z);
  float volumeFogInvalidity_g : packoffset(c8.w);
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
  out float4 o1 : NORMAL0,
  out float4 o2 : TEXCOORD0,
  out float4 o3 : TEXCOORD1,
  out uint4 o4 : TEXCOORD6,
  out float4 o5 : TEXCOORD7,
  out float4 o6 : TEXCOORD8)
{
// Needs manual fix for instruction:
// unknown dcl_: dcl_input_sgv v3.x, instance_id
  float4 r0,r1,r2,r3,r4,r5,r6;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.x = max(0, v2.y);
  r0.x = log2(r0.x);
  r0.x = shakeFlexibility_g * r0.x;
  r0.x = exp2(r0.x);
  r0.y = (int)v3.x + instanceOffset_g;
  r1.x = instances_g[r0.y].world._m00;
  r1.y = instances_g[r0.y].world._m10;
  r1.z = instances_g[r0.y].world._m20;
  r1.w = instances_g[r0.y].world._m30;
  r2.xyz = v0.xyz;
  r2.w = 1;
  r3.x = dot(r2.xyzw, r1.xyzw);
  o1.x = dot(v1.xyz, r1.xyz);
  r1.x = instances_g[r0.y].world._m01;
  r1.y = instances_g[r0.y].world._m11;
  r1.z = instances_g[r0.y].world._m21;
  r1.w = instances_g[r0.y].world._m31;
  r3.y = dot(r2.xyzw, r1.xyzw);
  o1.y = dot(v1.xyz, r1.xyz);
  r1.x = instances_g[r0.y].world._m02;
  r1.y = instances_g[r0.y].world._m12;
  r1.z = instances_g[r0.y].world._m22;
  r1.w = instances_g[r0.y].world._m32;
  r3.z = dot(r2.xyzw, r1.xyzw);
  o1.z = dot(v1.xyz, r1.xyz);
  r1.xyz = windWaveFrequency_g * r3.xyz;
  r0.z = dot(r1.xyz, -windDirection_g.xyz);
  r0.z = windWaveTime_g + r0.z;
  r0.z = sin(r0.z);
  r0.z = r0.z * 0.5 + 0.5;
  r0.z = log2(r0.z);
  r0.z = 7 * r0.z;
  r0.z = exp2(r0.z);
  r0.z = 1 + -r0.z;
  r0.w = gameTime_g * shakeSpeed_g;
  r1.xyz = r3.xyz * shakeFreq_g + r0.www;
  r1.xyz = r0.zzz * windForce_g + r1.xyz;
  r0.z = windForce_g * r0.z;
  r1.xyz = sin(r1.xyz);
  r1.xyz = windDirection_g.xyz * r1.xyz;
  r1.xyz = shakeScale_g * r1.xyz;
  r4.xyz = windDirection_g.xyz * shakeWindScale_g;
  r5.xyz = r4.xyz * r0.zzz;
  r1.xyz = r1.xyz * r0.zzz + r5.xyz;
  r1.xyz = float3(0,-0.00980000012,0) + r1.xyz;
  r5.x = instances_g[r0.y].param2.x;
  r5.y = instances_g[r0.y].param2.y;
  r5.z = instances_g[r0.y].param2.z;
  r1.xyz = r5.xyz + r1.xyz;
  r1.xyz = r1.xyz * r0.xxx + r3.xyz;
  r1.w = 1;
  r3.x = dot(r1.xyzw, viewProj_g._m00_m10_m20_m30);
  r3.y = dot(r1.xyzw, viewProj_g._m01_m11_m21_m31);
  r3.z = dot(r1.xyzw, viewProj_g._m02_m12_m22_m32);
  r3.w = dot(r1.xyzw, viewProj_g._m03_m13_m23_m33);
  o2.xyzw = r1.xyzw;
  o0.xyzw = r3.xyzw;
  o5.xyzw = r3.xyzw;
  o1.w = 0;
  r1.xy = v2.xy;
  r1.zw = float2(0,0);
  o3.xyzw = uvScroll0_g.xyzw + r1.xyzw;
  o4.x = r0.y;
  r1.x = instances_g[r0.y].prevWorld._m00;
  r1.y = instances_g[r0.y].prevWorld._m10;
  r1.z = instances_g[r0.y].prevWorld._m20;
  r1.w = instances_g[r0.y].prevWorld._m30;
  r1.x = dot(r2.xyzw, r1.xyzw);
  r3.x = instances_g[r0.y].prevWorld._m01;
  r3.y = instances_g[r0.y].prevWorld._m11;
  r3.z = instances_g[r0.y].prevWorld._m21;
  r3.w = instances_g[r0.y].prevWorld._m31;
  r6.x = instances_g[r0.y].prevWorld._m02;
  r6.y = instances_g[r0.y].prevWorld._m12;
  r6.z = instances_g[r0.y].prevWorld._m22;
  r6.w = instances_g[r0.y].prevWorld._m32;
  r1.z = dot(r2.xyzw, r6.xyzw);
  r1.y = dot(r2.xyzw, r3.xyzw);
  r0.yzw = r1.xyz * shakeFreq_g + r0.www;
  r2.xyz = windWaveFrequency_g * r1.xyz;
  r1.w = dot(r2.xyz, -windDirection_g.xyz);
  r1.w = windWaveTime_g + r1.w;
  r1.w = sin(r1.w);
  r1.w = r1.w * 0.5 + 0.5;
  r1.w = log2(r1.w);
  r1.w = 7 * r1.w;
  r1.w = exp2(r1.w);
  r1.w = 1 + -r1.w;
  r0.yzw = r1.www * windForce_g + r0.yzw;
  r1.w = windForce_g * r1.w;
  r0.yzw = sin(r0.yzw);
  r0.yzw = windDirection_g.xyz * r0.yzw;
  r0.yzw = shakeScale_g * r0.yzw;
  r2.xyz = r4.xyz * r1.www;
  r0.yzw = r0.yzw * r1.www + r2.xyz;
  r5.w = 0;
  r0.yzw = r5.wyw + r0.yzw;
  r5.y = -0.00980000012;
  r0.yzw = r5.xyz + r0.yzw;
  r0.xyz = r0.yzw * r0.xxx + r1.xyz;
  r0.w = 1;
  o6.x = dot(r0.xyzw, prevViewProj_g._m00_m10_m20_m30);
  o6.y = dot(r0.xyzw, prevViewProj_g._m01_m11_m21_m31);
  o6.z = dot(r0.xyzw, prevViewProj_g._m02_m12_m22_m32);
  o6.w = dot(r0.xyzw, prevViewProj_g._m03_m13_m23_m33);
  return;
}