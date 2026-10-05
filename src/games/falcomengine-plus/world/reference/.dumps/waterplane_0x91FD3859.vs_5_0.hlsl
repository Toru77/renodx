// ---- Created with 3Dmigoto v1.4.1 on Mon Oct  5 14:59:05 2026

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

cbuffer cb_water : register(b5)
{
  float2 uvScroll0_g : packoffset(c0);
  float2 uvScroll1_g : packoffset(c0.z);
  float2 uvScroll2_g : packoffset(c1);
  float rimIntensity_g : packoffset(c1.z);
  float rimLightPower_g : packoffset(c1.w);
  float3 rimLightColor_g : packoffset(c2);
  float alphaTestThreshold_g : packoffset(c2.w);
  float3 shadowColor_g : packoffset(c3);
  float specularGlossiness_g : packoffset(c3.w);
  float3 specularColor_g : packoffset(c4);
  float glowIntensity_g : packoffset(c4.w);
  float refrectIntensity_g : packoffset(c5);
  float fresnel_g : packoffset(c5.y);
  float waveFreq_g : packoffset(c5.z);
  float waveScale_g : packoffset(c5.w);
  float2 waveVelocity_g : packoffset(c6);
  float waterColorDepth_g : packoffset(c6.z);
  float waterColorIntensity_g : packoffset(c6.w);
  float3 waterColor_g : packoffset(c7);
  float waterEdgeFadeWidth_g : packoffset(c7.w);
  float waterEdgeWhiteWidth_g : packoffset(c8);
  float waterEdgeWhiteFactor_g : packoffset(c8.y);
  float waterEdgeWhiteDistScaleMax_g : packoffset(c8.z);
  float waterRefractionScale_g : packoffset(c8.w);
  float waveVerticalNoiseSpeed_g : packoffset(c9);
  float glowShadowFadeRatio_g : packoffset(c9.y);
  float waveUVScale_g : packoffset(c9.z);
  float ssrRayDistance_g : packoffset(c9.w);
  float waveAttnDistance_g : packoffset(c10);
  float waveAttnLimit_g : packoffset(c10.y);
  float rimAttnDistance_g : packoffset(c10.z);
  float rimAttnLimit_g : packoffset(c10.w);
  float3 fakeSpecularColor_g : packoffset(c11);
  float fakeSpecularGlossiness_g : packoffset(c11.w);
  float fakeSpecularDir_g : packoffset(c12);
  float fakeSpecularPitch_g : packoffset(c12.y);
  float fakeSpecularShadowRatio_g : packoffset(c12.z);
  float2 waveVertexVelocity_g : packoffset(c13);
  float2 waveVertexFreqXY_g : packoffset(c13.z);
  float waveVertexScale_g : packoffset(c14);
  float specularPitchAdjust_g : packoffset(c14.y);
  float specularShadowRatio_g : packoffset(c14.z);
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
  uint v7 : SV_InstanceID0,
  out float4 o0 : SV_Position0,
  out float3 o1 : NORMAL0,
  out float3 o2 : TANGENT0,
  out float3 o3 : BINORMAL0,
  out float4 o4 : TEXCOORD0,
  out float4 o5 : TEXCOORD1,
  out float4 o6 : TEXCOORD2,
  out float4 o7 : TEXCOORD3,
  out float4 o8 : TEXCOORD4,
  out float4 o9 : TEXCOORD6,
  out float4 o10 : TEXCOORD7)
{
// Needs manual fix for instruction:
// unknown dcl_: dcl_input_sgv v7.x, instance_id
  float4 r0,r1,r2,r3,r4,r5,r6,r7,r8;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.x = (int)v7.x + instanceOffset_g;
  r1.x = instances_g[r0.x].world._m00;
  r1.y = instances_g[r0.x].world._m10;
  r1.z = instances_g[r0.x].world._m20;
  r1.w = instances_g[r0.x].world._m30;
  r2.z = dot(v1.xyz, r1.xyz);
  r3.x = instances_g[r0.x].world._m01;
  r3.y = instances_g[r0.x].world._m11;
  r3.z = instances_g[r0.x].world._m21;
  r3.w = instances_g[r0.x].world._m31;
  r2.x = dot(v1.xyz, r3.xyz);
  r4.x = instances_g[r0.x].world._m02;
  r4.y = instances_g[r0.x].world._m12;
  r4.z = instances_g[r0.x].world._m22;
  r4.w = instances_g[r0.x].world._m32;
  r2.y = dot(v1.xyz, r4.xyz);
  r0.y = dot(r2.xyz, r2.xyz);
  r0.y = max(0.00100000005, r0.y);
  r0.y = rsqrt(r0.y);
  r0.yzw = r2.xyz * r0.yyy;
  r2.x = dot(v2.xyz, r1.xyz);
  r2.y = dot(v2.xyz, r3.xyz);
  r2.z = dot(v2.xyz, r4.xyz);
  r5.xyz = r2.zxy * r0.wyz;
  r5.xyz = r0.zwy * r2.xyz + -r5.xyz;
  r6.xyz = v0.xyz;
  r6.w = 1;
  r1.z = dot(r6.xyzw, r1.xyzw);
  r1.x = dot(r6.xyzw, r3.xyzw);
  r1.y = dot(r6.xyzw, r4.xyzw);
  r3.y = dot(r1.xyz, r5.xyz);
  r3.zw = gameTime_g * waveVertexVelocity_g.xy;
  r3.x = dot(r1.zxy, r2.xyz);
  r3.xy = r3.xy * waveVertexFreqXY_g.xy + -r3.zw;
  r3.xy = float2(0.159154937,0.159154937) * r3.xy;
  r4.xy = cmp(r3.xy >= -r3.xy);
  r3.xy = frac(abs(r3.xy));
  r3.xy = r4.xy ? r3.xy : -r3.xy;
  r3.xy = float2(6.28318548,6.28318548) * r3.xy;
  r1.w = sin(r3.x);
  r2.w = cos(r3.y);
  r4.xyz = r1.www * r0.wyz;
  r4.xyz = r4.xyz * r2.www;
  r1.w = waveVertexScale_g * v6.w;
  r4.xyz = r4.xyz * r1.www + r1.zxy;
  r4.w = 1;
  r7.x = dot(r4.xyzw, viewProj_g._m00_m10_m20_m30);
  r7.y = dot(r4.xyzw, viewProj_g._m01_m11_m21_m31);
  r7.z = dot(r4.xyzw, viewProj_g._m02_m12_m22_m32);
  r7.w = dot(r4.xyzw, viewProj_g._m03_m13_m23_m33);
  o4.xyzw = r4.xyzw;
  o0.xyzw = r7.xyzw;
  o9.xyzw = r7.xyzw;
  r7.xyz = r2.zxy * float3(0.00499999989,0.00499999989,0.00499999989) + r1.yzx;
  r1.xyz = r5.xyz * float3(0.00499999989,0.00499999989,0.00499999989) + r1.xyz;
  r3.x = dot(r7.yzx, r2.xyz);
  r3.y = dot(r7.zxy, r5.xyz);
  r3.xy = r3.xy * waveVertexFreqXY_g.xy + -r3.zw;
  r3.xy = float2(0.159154937,0.159154937) * r3.xy;
  r8.xy = cmp(r3.xy >= -r3.xy);
  r3.xy = frac(abs(r3.xy));
  r3.xy = r8.xy ? r3.xy : -r3.xy;
  r3.xy = float2(6.28318548,6.28318548) * r3.xy;
  r2.w = sin(r3.x);
  r3.x = cos(r3.y);
  r8.xyz = r2.www * r0.zwy;
  r8.xyz = r8.xyz * r3.xxx;
  r7.xyz = r8.xyz * r1.www + r7.xyz;
  r7.xyz = -r7.xyz + r4.zxy;
  r3.x = dot(r1.zxy, r2.xyz);
  r3.y = dot(r1.xyz, r5.xyz);
  r3.xy = r3.xy * waveVertexFreqXY_g.xy + -r3.zw;
  r3.xy = float2(0.159154937,0.159154937) * r3.xy;
  r8.xy = cmp(r3.xy >= -r3.xy);
  r3.xy = frac(abs(r3.xy));
  r3.xy = r8.xy ? r3.xy : -r3.xy;
  r3.xy = float2(6.28318548,6.28318548) * r3.xy;
  r2.w = sin(r3.x);
  r3.x = cos(r3.y);
  r8.xyz = r2.www * r0.yzw;
  r8.xyz = r8.xyz * r3.xxx;
  r1.xyz = r8.xyz * r1.www + r1.xyz;
  r1.xyz = r4.yzx + -r1.xyz;
  r4.xyz = r7.xyz * r1.xyz;
  r1.xyz = r7.zxy * r1.yzx + -r4.xyz;
  r2.w = dot(r1.xyz, r1.xyz);
  r2.w = rsqrt(r2.w);
  r1.xyz = r2.www * r1.xyz;
  o1.xyz = r1.xyz;
  r4.xyz = r1.zxy * r5.xyz;
  r4.xyz = r1.yzx * r5.yzx + -r4.xyz;
  o2.xyz = r4.xyz;
  r7.xyz = r4.yzx * r1.zxy;
  o3.xyz = r1.yzx * r4.zxy + -r7.xyz;
  o5.xy = v3.xy;
  o5.z = instances_g[r0.x].color.w;
  r4.xy = v3.xy;
  r4.zw = v4.xy;
  o6.xyzw = uvScroll0_g.xyzw + r4.xyzw;
  o7.xy = uvScroll2_g.xy + v5.xy;
  o8.xyzw = v6.xyzw;
  r4.x = instances_g[r0.x].prevWorld._m00;
  r4.y = instances_g[r0.x].prevWorld._m10;
  r4.z = instances_g[r0.x].prevWorld._m20;
  r4.w = instances_g[r0.x].prevWorld._m30;
  r1.x = dot(r6.xyzw, r4.xyzw);
  r4.x = instances_g[r0.x].prevWorld._m01;
  r4.y = instances_g[r0.x].prevWorld._m11;
  r4.z = instances_g[r0.x].prevWorld._m21;
  r4.w = instances_g[r0.x].prevWorld._m31;
  r7.x = instances_g[r0.x].prevWorld._m02;
  r7.y = instances_g[r0.x].prevWorld._m12;
  r7.z = instances_g[r0.x].prevWorld._m22;
  r7.w = instances_g[r0.x].prevWorld._m32;
  r1.z = dot(r6.xyzw, r7.xyzw);
  r1.y = dot(r6.xyzw, r4.xyzw);
  r2.x = dot(r1.xyz, r2.xyz);
  r2.y = dot(r1.yzx, r5.xyz);
  r2.xy = r2.xy * waveVertexFreqXY_g.xy + -r3.zw;
  r2.xy = float2(0.159154937,0.159154937) * r2.xy;
  r2.zw = cmp(r2.xy >= -r2.xy);
  r2.xy = frac(abs(r2.xy));
  r2.xy = r2.zw ? r2.xy : -r2.xy;
  r2.xy = float2(6.28318548,6.28318548) * r2.xy;
  r0.x = sin(r2.x);
  r2.x = cos(r2.y);
  r0.xyz = r0.wyz * r0.xxx;
  r0.xyz = r0.xyz * r2.xxx;
  r0.xyz = r0.xyz * r1.www + r1.xyz;
  r0.w = 1;
  o10.x = dot(r0.xyzw, prevViewProj_g._m00_m10_m20_m30);
  o10.y = dot(r0.xyzw, prevViewProj_g._m01_m11_m21_m31);
  o10.z = dot(r0.xyzw, prevViewProj_g._m02_m12_m22_m32);
  o10.w = dot(r0.xyzw, prevViewProj_g._m03_m13_m23_m33);
  return;
}