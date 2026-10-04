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
  float shakeScale_g : packoffset(c7);
  float shakeSpeed_g : packoffset(c7.y);
  float shakeFlexibility_g : packoffset(c7.z);
  float shakeFreq_g : packoffset(c7.w);
  float shakeWindScale_g : packoffset(c8);
  float shadowCastOffset_g : packoffset(c8.y);
  float volumeFogInvalidity_g : packoffset(c8.z);
}

StructuredBuffer<InstanceParam> instances_g : register(t15);


// 3Dmigoto declarations
#define cmp -


void main(
  float3 v0 : POSITION0,
  float3 v1 : NORMAL0,
  float2 v2 : TEXCOORD0,
  float4 v3 : COLOR1,
  uint v4 : SV_InstanceID0,
  out float4 o0 : SV_Position0,
  out float4 o1 : NORMAL0,
  out float4 o2 : TEXCOORD0,
  out float4 o3 : TEXCOORD1,
  out float4 o4 : TEXCOORD4,
  out uint4 o5 : TEXCOORD6,
  out float4 o6 : TEXCOORD7,
  out float4 o7 : TEXCOORD8)
{
// Needs manual fix for instruction:
// unknown dcl_: dcl_input_sgv v4.x, instance_id
  float4 r0,r1,r2,r3,r4,r5,r6;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.x = max(0, v2.y);
  r0.x = log2(r0.x);
  r0.x = shakeFlexibility_g * r0.x;
  r0.x = exp2(r0.x);
  r1.xyz = v0.xyz;
  r1.w = 1;
  r0.y = (int)v4.x + instanceOffset_g;
  r2.x = instances_g[r0.y].world._m00;
  r2.y = instances_g[r0.y].world._m10;
  r2.z = instances_g[r0.y].world._m20;
  r2.w = instances_g[r0.y].world._m30;
  r3.x = dot(r1.xyzw, r2.xyzw);
  o1.x = dot(v1.xyz, r2.xyz);
  r2.x = instances_g[r0.y].world._m01;
  r2.y = instances_g[r0.y].world._m11;
  r2.z = instances_g[r0.y].world._m21;
  r2.w = instances_g[r0.y].world._m31;
  r3.y = dot(r1.xyzw, r2.xyzw);
  o1.y = dot(v1.xyz, r2.xyz);
  r2.x = instances_g[r0.y].world._m02;
  r2.y = instances_g[r0.y].world._m12;
  r2.z = instances_g[r0.y].world._m22;
  r2.w = instances_g[r0.y].world._m32;
  r3.z = dot(r1.xyzw, r2.xyzw);
  o1.z = dot(v1.xyz, r2.xyz);
  r2.xyz = windWaveFrequency_g * r3.xyz;
  r0.z = dot(r2.xyz, -windDirection_g.xyz);
  r0.z = windWaveTime_g + r0.z;
  r0.z = sin(r0.z);
  r0.z = r0.z * 0.5 + 0.5;
  r0.z = log2(r0.z);
  r0.z = 7 * r0.z;
  r0.z = exp2(r0.z);
  r0.z = 1 + -r0.z;
  r0.w = gameTime_g * shakeSpeed_g;
  r2.xyz = r3.xyz * shakeFreq_g + r0.www;
  r2.xyz = r0.zzz * windForce_g + r2.xyz;
  r0.z = windForce_g * r0.z;
  r2.xyz = sin(r2.xyz);
  r2.xyz = windDirection_g.xyz * r2.xyz;
  r2.xyz = shakeScale_g * r2.xyz;
  r4.xyz = windDirection_g.xyz * shakeWindScale_g;
  r5.xyz = r4.xyz * r0.zzz;
  r2.xyz = r2.xyz * r0.zzz + r5.xyz;
  r2.xyz = float3(0,-0.00980000012,0) + r2.xyz;
  r5.x = instances_g[r0.y].param2.x;
  r5.y = instances_g[r0.y].param2.y;
  r5.z = instances_g[r0.y].param2.z;
  r2.xyz = r5.xyz + r2.xyz;
  r2.xyz = r2.xyz * r0.xxx + r3.xyz;
  r2.w = 1;
  r3.x = dot(r2.xyzw, viewProj_g._m00_m10_m20_m30);
  r3.y = dot(r2.xyzw, viewProj_g._m01_m11_m21_m31);
  r3.z = dot(r2.xyzw, viewProj_g._m02_m12_m22_m32);
  r3.w = dot(r2.xyzw, viewProj_g._m03_m13_m23_m33);
  o2.xyzw = r2.xyzw;
  o0.xyzw = r3.xyzw;
  o6.xyzw = r3.xyzw;
  o1.w = 0;
  r2.xy = v2.xy;
  r2.zw = float2(0,0);
  o3.xyzw = uvScroll0_g.xyzw + r2.xyzw;
  r2.xyz = v3.xyz;
  r2.w = 1;
  r3.x = instances_g[r0.y].color.x;
  r3.y = instances_g[r0.y].color.y;
  r3.z = instances_g[r0.y].color.z;
  r3.w = instances_g[r0.y].color.w;
  o4.xyzw = r3.xyzw * r2.xyzw;
  o5.x = r0.y;
  r2.x = instances_g[r0.y].prevWorld._m00;
  r2.y = instances_g[r0.y].prevWorld._m10;
  r2.z = instances_g[r0.y].prevWorld._m20;
  r2.w = instances_g[r0.y].prevWorld._m30;
  r2.x = dot(r1.xyzw, r2.xyzw);
  r3.x = instances_g[r0.y].prevWorld._m01;
  r3.y = instances_g[r0.y].prevWorld._m11;
  r3.z = instances_g[r0.y].prevWorld._m21;
  r3.w = instances_g[r0.y].prevWorld._m31;
  r6.x = instances_g[r0.y].prevWorld._m02;
  r6.y = instances_g[r0.y].prevWorld._m12;
  r6.z = instances_g[r0.y].prevWorld._m22;
  r6.w = instances_g[r0.y].prevWorld._m32;
  r2.z = dot(r1.xyzw, r6.xyzw);
  r2.y = dot(r1.xyzw, r3.xyzw);
  r0.yzw = r2.xyz * shakeFreq_g + r0.www;
  r1.xyz = windWaveFrequency_g * r2.xyz;
  r1.x = dot(r1.xyz, -windDirection_g.xyz);
  r1.x = windWaveTime_g + r1.x;
  r1.x = sin(r1.x);
  r1.x = r1.x * 0.5 + 0.5;
  r1.x = log2(r1.x);
  r1.x = 7 * r1.x;
  r1.x = exp2(r1.x);
  r1.x = 1 + -r1.x;
  r0.yzw = r1.xxx * windForce_g + r0.yzw;
  r1.x = windForce_g * r1.x;
  r0.yzw = sin(r0.yzw);
  r0.yzw = windDirection_g.xyz * r0.yzw;
  r0.yzw = shakeScale_g * r0.yzw;
  r1.yzw = r4.xyz * r1.xxx;
  r0.yzw = r0.yzw * r1.xxx + r1.yzw;
  r5.w = 0;
  r0.yzw = r5.wyw + r0.yzw;
  r5.y = -0.00980000012;
  r0.yzw = r5.xyz + r0.yzw;
  r0.xyz = r0.yzw * r0.xxx + r2.xyz;
  r0.w = 1;
  o7.x = dot(r0.xyzw, prevViewProj_g._m00_m10_m20_m30);
  o7.y = dot(r0.xyzw, prevViewProj_g._m01_m11_m21_m31);
  o7.z = dot(r0.xyzw, prevViewProj_g._m02_m12_m22_m32);
  o7.w = dot(r0.xyzw, prevViewProj_g._m03_m13_m23_m33);
  return;
}