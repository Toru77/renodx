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
  float rimIntensity_g : packoffset(c1.z);
  float rimLightPower_g : packoffset(c1.w);
  float3 rimLightColor_g : packoffset(c2);
  float alphaTestThreshold_g : packoffset(c2.w);
  float3 toonColor_g : packoffset(c3);
  float3 toonEdgeColor_g : packoffset(c4);
  float toonEdgeIntensity_g : packoffset(c4.w);
  float3 specularColor_g : packoffset(c5);
  float anisoRoughness1_g : packoffset(c5.w);
  float3 gradColor1_g : packoffset(c6);
  float anisoRoughness2_g : packoffset(c6.w);
  float3 gradColor2_g : packoffset(c7);
  float gradSharpness_g : packoffset(c7.w);
  float glowIntensity_g : packoffset(c8);
  float ssaoIntensity_g : packoffset(c8.y);
  float shadowBias_g : packoffset(c8.z);
  float specularHeight_g : packoffset(c8.w);
  float2 dudvScrollSpeed_g : packoffset(c9);
  float shadowOffsetDistance_g : packoffset(c9.z);
  float dudvScale_g : packoffset(c9.w);
}

StructuredBuffer<float4x3> bones_g : register(t0);
StructuredBuffer<InstanceParam> instances_g : register(t15);


// 3Dmigoto declarations
#define cmp -


void main(
  float3 v0 : POSITION0,
  float3 v1 : NORMAL0,
  float3 v2 : TANGENT0,
  float2 v3 : TEXCOORD0,
  float4 v4 : BLENDWEIGHTS0,
  uint4 v5 : BLENDINDICES0,
  uint v6 : SV_InstanceID0,
  out float4 o0 : SV_Position0,
  out float3 o1 : NORMAL0,
  out float3 o2 : TANGENT0,
  out uint4 o3 : TEXCOORD0,
  out float4 o4 : TEXCOORD1,
  out float4 o5 : TEXCOORD2,
  out float4 o6 : TEXCOORD6,
  out float4 o7 : TEXCOORD7)
{
// Needs manual fix for instruction:
// unknown dcl_: dcl_input_sgv v6.x, instance_id
  float4 r0,r1,r2,r3,r4,r5,r6,r7;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.x = (int)v6.x + instanceOffset_g;
  r0.y = instances_g[r0.x].boneAddress;
  r1.xyzw = (int4)r0.yyyy + (int4)v5.xyzw;
  r0.y = (int)r0.y + maxBoneCount_g;
  r2.xyzw = (int4)r0.yyyy + (int4)v5.xyzw;
  r3.xyzw = bones_g[r1.y]._m00_m10_m20_m30;
  r4.xyz = v0.xyz;
  r4.w = 1;
  r3.x = dot(r4.xyzw, r3.xyzw);
  r5.xyzw = bones_g[r1.y]._m01_m11_m21_m31;
  r3.y = dot(r4.xyzw, r5.xyzw);
  r5.xyzw = bones_g[r1.y]._m02_m12_m22_m32;
  r3.z = dot(r4.xyzw, r5.xyzw);
  r0.yzw = v4.yyy * r3.xyz;
  r3.xyzw = bones_g[r1.x]._m00_m10_m20_m30;
  r3.x = dot(r4.xyzw, r3.xyzw);
  r5.xyzw = bones_g[r1.x]._m01_m11_m21_m31;
  r3.y = dot(r4.xyzw, r5.xyzw);
  r5.xyzw = bones_g[r1.x]._m02_m12_m22_m32;
  r3.z = dot(r4.xyzw, r5.xyzw);
  r0.yzw = r3.xyz * v4.xxx + r0.yzw;
  r3.xyzw = bones_g[r1.z]._m00_m10_m20_m30;
  r3.x = dot(r4.xyzw, r3.xyzw);
  r5.xyzw = bones_g[r1.z]._m01_m11_m21_m31;
  r3.y = dot(r4.xyzw, r5.xyzw);
  r5.xyzw = bones_g[r1.z]._m02_m12_m22_m32;
  r3.z = dot(r4.xyzw, r5.xyzw);
  r0.yzw = r3.xyz * v4.zzz + r0.yzw;
  r3.xyzw = bones_g[r1.w]._m00_m10_m20_m30;
  r3.x = dot(r4.xyzw, r3.xyzw);
  r5.xyzw = bones_g[r1.w]._m01_m11_m21_m31;
  r3.y = dot(r4.xyzw, r5.xyzw);
  r5.xyzw = bones_g[r1.w]._m02_m12_m22_m32;
  r3.z = dot(r4.xyzw, r5.xyzw);
  r3.xyz = r3.xyz * v4.www + r0.yzw;
  r3.w = 1;
  r5.x = instances_g[r0.x].world._m00;
  r5.y = instances_g[r0.x].world._m10;
  r5.z = instances_g[r0.x].world._m20;
  r5.w = instances_g[r0.x].world._m30;
  r5.x = dot(r3.xyzw, r5.xyzw);
  r6.x = instances_g[r0.x].world._m01;
  r6.y = instances_g[r0.x].world._m11;
  r6.z = instances_g[r0.x].world._m21;
  r6.w = instances_g[r0.x].world._m31;
  r5.y = dot(r3.xyzw, r6.xyzw);
  r6.x = instances_g[r0.x].world._m02;
  r6.y = instances_g[r0.x].world._m12;
  r6.z = instances_g[r0.x].world._m22;
  r6.w = instances_g[r0.x].world._m32;
  r5.z = dot(r3.xyzw, r6.xyzw);
  r5.w = 1;
  r3.x = dot(r5.xyzw, viewProj_g._m00_m10_m20_m30);
  r3.y = dot(r5.xyzw, viewProj_g._m01_m11_m21_m31);
  r3.z = dot(r5.xyzw, viewProj_g._m02_m12_m22_m32);
  r3.w = dot(r5.xyzw, viewProj_g._m03_m13_m23_m33);
  o4.xyzw = r5.xyzw;
  o0.xyzw = r3.xyzw;
  o6.xyzw = r3.xyzw;
  r0.yzw = bones_g[r1.y]._m01_m11_m21;
  r0.yzw = v4.yyy * r0.yzw;
  r3.xyz = bones_g[r1.x]._m01_m11_m21;
  r0.yzw = r3.xyz * v4.xxx + r0.yzw;
  r3.xyz = bones_g[r1.z]._m01_m11_m21;
  r0.yzw = r3.xyz * v4.zzz + r0.yzw;
  r3.xyz = bones_g[r1.w]._m01_m11_m21;
  r0.yzw = r3.xyz * v4.www + r0.yzw;
  r3.x = instances_g[r0.x].world._m00;
  r3.y = instances_g[r0.x].world._m10;
  r3.z = instances_g[r0.x].world._m20;
  r5.xyz = r3.yyy * r0.yzw;
  r6.xyz = bones_g[r1.y]._m00_m10_m20;
  r6.xyz = v4.yyy * r6.xyz;
  r7.xyz = bones_g[r1.x]._m00_m10_m20;
  r6.xyz = r7.xyz * v4.xxx + r6.xyz;
  r7.xyz = bones_g[r1.z]._m00_m10_m20;
  r6.xyz = r7.xyz * v4.zzz + r6.xyz;
  r7.xyz = bones_g[r1.w]._m00_m10_m20;
  r6.xyz = r7.xyz * v4.www + r6.xyz;
  r3.xyw = r6.xyz * r3.xxx + r5.xyz;
  r5.xyz = bones_g[r1.y]._m02_m12_m22;
  r5.xyz = v4.yyy * r5.xyz;
  r7.xyz = bones_g[r1.x]._m02_m12_m22;
  r5.xyz = r7.xyz * v4.xxx + r5.xyz;
  r1.xyz = bones_g[r1.z]._m02_m12_m22;
  r7.xyz = bones_g[r1.w]._m02_m12_m22;
  r1.xyz = r1.xyz * v4.zzz + r5.xyz;
  r1.xyz = r7.xyz * v4.www + r1.xyz;
  r3.xyz = r1.xyz * r3.zzz + r3.xyw;
  o1.x = dot(v1.xyz, r3.xyz);
  r3.x = dot(v2.xyz, r3.xyz);
  r5.x = instances_g[r0.x].world._m01;
  r5.y = instances_g[r0.x].world._m11;
  r5.z = instances_g[r0.x].world._m21;
  r7.xyz = r5.yyy * r0.yzw;
  r5.xyw = r6.xyz * r5.xxx + r7.xyz;
  r5.xyz = r1.xyz * r5.zzz + r5.xyw;
  o1.y = dot(v1.xyz, r5.xyz);
  r3.y = dot(v2.xyz, r5.xyz);
  r5.x = instances_g[r0.x].world._m02;
  r5.y = instances_g[r0.x].world._m12;
  r5.z = instances_g[r0.x].world._m22;
  r0.yzw = r5.yyy * r0.yzw;
  r0.yzw = r6.xyz * r5.xxx + r0.yzw;
  r0.yzw = r1.xyz * r5.zzz + r0.yzw;
  o1.z = dot(v1.xyz, r0.yzw);
  r3.z = dot(v2.xyz, r0.yzw);
  r0.y = cmp(v3.x < 0);
  r0.y = r0.y ? -1 : 1;
  o2.xyz = r3.xyz * r0.yyy;
  o3.x = r0.x;
  r1.xy = v3.xy;
  r1.zw = float2(0,0);
  o5.xyzw = uvScroll0_g.xyzw + r1.xyzw;
  r1.xyzw = bones_g[r2.y]._m00_m10_m20_m30;
  r1.x = dot(r4.xyzw, r1.xyzw);
  r3.xyzw = bones_g[r2.y]._m01_m11_m21_m31;
  r1.y = dot(r4.xyzw, r3.xyzw);
  r3.xyzw = bones_g[r2.y]._m02_m12_m22_m32;
  r1.z = dot(r4.xyzw, r3.xyzw);
  r0.yzw = v4.yyy * r1.xyz;
  r1.xyzw = bones_g[r2.x]._m00_m10_m20_m30;
  r1.x = dot(r4.xyzw, r1.xyzw);
  r3.xyzw = bones_g[r2.x]._m01_m11_m21_m31;
  r1.y = dot(r4.xyzw, r3.xyzw);
  r3.xyzw = bones_g[r2.x]._m02_m12_m22_m32;
  r1.z = dot(r4.xyzw, r3.xyzw);
  r0.yzw = r1.xyz * v4.xxx + r0.yzw;
  r1.xyzw = bones_g[r2.z]._m00_m10_m20_m30;
  r1.x = dot(r4.xyzw, r1.xyzw);
  r3.xyzw = bones_g[r2.z]._m01_m11_m21_m31;
  r1.y = dot(r4.xyzw, r3.xyzw);
  r3.xyzw = bones_g[r2.z]._m02_m12_m22_m32;
  r1.z = dot(r4.xyzw, r3.xyzw);
  r0.yzw = r1.xyz * v4.zzz + r0.yzw;
  r1.xyzw = bones_g[r2.w]._m00_m10_m20_m30;
  r1.x = dot(r4.xyzw, r1.xyzw);
  r3.xyzw = bones_g[r2.w]._m01_m11_m21_m31;
  r2.xyzw = bones_g[r2.w]._m02_m12_m22_m32;
  r1.z = dot(r4.xyzw, r2.xyzw);
  r1.y = dot(r4.xyzw, r3.xyzw);
  r1.xyz = r1.xyz * v4.www + r0.yzw;
  r2.x = instances_g[r0.x].prevWorld._m00;
  r2.y = instances_g[r0.x].prevWorld._m10;
  r2.z = instances_g[r0.x].prevWorld._m20;
  r2.w = instances_g[r0.x].prevWorld._m30;
  r1.w = 1;
  r2.x = dot(r1.xyzw, r2.xyzw);
  r3.x = instances_g[r0.x].prevWorld._m01;
  r3.y = instances_g[r0.x].prevWorld._m11;
  r3.z = instances_g[r0.x].prevWorld._m21;
  r3.w = instances_g[r0.x].prevWorld._m31;
  r0.x = instances_g[r0.x].prevWorld._m02;
  r0.y = instances_g[r0.x].prevWorld._m12;
  r0.z = instances_g[r0.x].prevWorld._m22;
  r0.w = instances_g[r0.x].prevWorld._m32;
  r2.z = dot(r1.xyzw, r0.xyzw);
  r2.y = dot(r1.xyzw, r3.xyzw);
  r2.w = 1;
  o7.x = dot(r2.xyzw, prevViewProj_g._m00_m10_m20_m30);
  o7.y = dot(r2.xyzw, prevViewProj_g._m01_m11_m21_m31);
  o7.z = dot(r2.xyzw, prevViewProj_g._m02_m12_m22_m32);
  o7.w = dot(r2.xyzw, prevViewProj_g._m03_m13_m23_m33);
  return;
}