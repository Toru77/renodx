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
  out uint o5 : TEXCOORD6,
  out float4 o6 : TEXCOORD7,
  out float4 o7 : TEXCOORD8)
{
// Needs manual fix for instruction:
// unknown dcl_: dcl_input_sgv v4.x, instance_id
  float4 r0,r1,r2,r3,r4,r5,r6,r7;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.x = max(0, v2.y);
  r0.x = log2(r0.x);
  r0.x = shakeFlexibility_g * r0.x;
  r0.x = exp2(r0.x);
  r0.yzw = windWaveTime_g * windDirection_g.xyz;
  r1.xyz = v0.xyz;
  r1.w = 1;
  r2.x = (int)v4.x + instanceOffset_g;
  r3.x = instances_g[r2.x].world._m00;
  r3.y = instances_g[r2.x].world._m10;
  r3.z = instances_g[r2.x].world._m20;
  r3.w = instances_g[r2.x].world._m30;
  r4.x = dot(r1.xyzw, r3.xyzw);
  o1.x = dot(v1.xyz, r3.xyz);
  r3.x = instances_g[r2.x].world._m01;
  r3.y = instances_g[r2.x].world._m11;
  r3.z = instances_g[r2.x].world._m21;
  r3.w = instances_g[r2.x].world._m31;
  r4.y = dot(r1.xyzw, r3.xyzw);
  o1.y = dot(v1.xyz, r3.xyz);
  r3.x = instances_g[r2.x].world._m02;
  r3.y = instances_g[r2.x].world._m12;
  r3.z = instances_g[r2.x].world._m22;
  r3.w = instances_g[r2.x].world._m32;
  r4.z = dot(r1.xyzw, r3.xyzw);
  o1.z = dot(v1.xyz, r3.xyz);
  r2.yzw = r4.xyz * windWaveFrequency_g + -r0.yzw;
  r2.y = dot(r2.yzw, r2.yzw);
  r2.y = sqrt(r2.y);
  r2.y = sin(r2.y);
  r2.y = r2.y * 0.5 + 0.5;
  r2.y = log2(r2.y);
  r2.y = 7 * r2.y;
  r2.y = exp2(r2.y);
  r2.y = 1 + -r2.y;
  r2.z = gameTime_g * shakeSpeed_g;
  r3.xyz = r4.xyz * shakeFreq_g + r2.zzz;
  r3.xyz = r2.yyy * windForce_g + r3.xyz;
  r2.y = windForce_g * r2.y;
  r3.xyz = sin(r3.xyz);
  r3.xyz = windDirection_g.xyz * r3.xyz;
  r3.xyz = shakeScale_g * r3.xyz;
  r5.xyz = windDirection_g.xyz * shakeWindScale_g;
  r6.xyz = r5.xyz * r2.yyy;
  r3.xyz = r3.xyz * r2.yyy + r6.xyz;
  r3.xyz = float3(0,-0.00980000012,0) + r3.xyz;
  r6.x = instances_g[r2.x].param2.x;
  r6.y = instances_g[r2.x].param2.y;
  r6.z = instances_g[r2.x].param2.z;
  r3.xyz = r6.xyz + r3.xyz;
  r3.xyz = r3.xyz * r0.xxx + r4.xyz;
  r3.w = 1;
  r4.x = dot(r3.xyzw, viewProj_g._m00_m10_m20_m30);
  r4.y = dot(r3.xyzw, viewProj_g._m01_m11_m21_m31);
  r4.z = dot(r3.xyzw, viewProj_g._m02_m12_m22_m32);
  r4.w = dot(r3.xyzw, viewProj_g._m03_m13_m23_m33);
  o2.xyzw = r3.xyzw;
  o0.xyzw = r4.xyzw;
  o6.xyzw = r4.xyzw;
  o1.w = 0;
  r3.xy = v2.xy;
  r3.zw = float2(0,0);
 o3.xyzw = float4(uvScroll0_g.xy, uvScroll1_g.xy) + r3.xyzw;
  r3.xyz = v3.xyz;
  r3.w = 1;
  r4.x = instances_g[r2.x].color.x;
  r4.y = instances_g[r2.x].color.y;
  r4.z = instances_g[r2.x].color.z;
  r4.w = instances_g[r2.x].color.w;
  o4.xyzw = r4.xyzw * r3.xyzw;
  o5.x = r2.x;
  r3.x = instances_g[r2.x].prevWorld._m00;
  r3.y = instances_g[r2.x].prevWorld._m10;
  r3.z = instances_g[r2.x].prevWorld._m20;
  r3.w = instances_g[r2.x].prevWorld._m30;
  r3.x = dot(r1.xyzw, r3.xyzw);
  r4.x = instances_g[r2.x].prevWorld._m01;
  r4.y = instances_g[r2.x].prevWorld._m11;
  r4.z = instances_g[r2.x].prevWorld._m21;
  r4.w = instances_g[r2.x].prevWorld._m31;
  r3.y = dot(r1.xyzw, r4.xyzw);
  r4.x = instances_g[r2.x].prevWorld._m02;
  r4.y = instances_g[r2.x].prevWorld._m12;
  r4.z = instances_g[r2.x].prevWorld._m22;
  r4.w = instances_g[r2.x].prevWorld._m32;
  r7.x = instances_g[r2.x].prevWorld._m03;
  r7.y = instances_g[r2.x].prevWorld._m13;
  r7.z = instances_g[r2.x].prevWorld._m23;
  r7.w = instances_g[r2.x].prevWorld._m33;
  r7.w = dot(r1.xyzw, r7.xyzw);
  r3.z = dot(r1.xyzw, r4.xyzw);
  r1.xyz = r3.xyz * shakeFreq_g + r2.zzz;
  r0.yzw = r3.xyz * windWaveFrequency_g + -r0.yzw;
  r0.y = dot(r0.yzw, r0.yzw);
  r0.y = sqrt(r0.y);
  r0.y = sin(r0.y);
  r0.y = r0.y * 0.5 + 0.5;
  r0.y = log2(r0.y);
  r0.y = 7 * r0.y;
  r0.y = exp2(r0.y);
  r0.y = 1 + -r0.y;
  r1.xyz = r0.yyy * windForce_g + r1.xyz;
  r0.y = windForce_g * r0.y;
  r1.xyz = sin(r1.xyz);
  r1.xyz = windDirection_g.xyz * r1.xyz;
  r1.xyz = shakeScale_g * r1.xyz;
  r2.xyz = r5.xyz * r0.yyy;
  r0.yzw = r1.xyz * r0.yyy + r2.xyz;
  r6.w = 0;
  r0.yzw = r6.wwy + r0.yzw;
  r6.y = -0.00980000012;
  r0.yzw = r6.xxy + r0.yzw;
  r7.xyz = r0.yzw * r0.xxx + r3.xyz;
  o7.x = dot(r7.xyzw, prevViewProj_g._m00_m10_m20_m30);
  o7.y = dot(r7.xyzw, prevViewProj_g._m01_m11_m21_m31);
  o7.z = dot(r7.xyzw, prevViewProj_g._m02_m12_m22_m32);
  o7.w = dot(r7.xyzw, prevViewProj_g._m03_m13_m23_m33);
  return;
}