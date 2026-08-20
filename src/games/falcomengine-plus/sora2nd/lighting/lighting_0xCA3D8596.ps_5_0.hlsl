// ---- Created with 3Dmigoto v1.4.1 on Thu Aug 20 07:13:29 2026

struct DeferredParam
{
    float3 shadowColor;            // Offset:    0
    float emissive;                // Offset:   12
    float3 specularColor;          // Offset:   16
    float rimLightPower;           // Offset:   28
    float3 rimLightColor;          // Offset:   32
    float rimIntensity;            // Offset:   44
    float3 fresnels;               // Offset:   48
    float specularShadowFadeRatio; // Offset:   60
    float3 specularGlossinesses;   // Offset:   64
    float dynamicLightIntensity;   // Offset:   76
    float materialFogIntensity;    // Offset:   80
    float metalness;               // Offset:   84
    float roughness;               // Offset:   88
    float cryRefractionIndex;      // Offset:   92
    float cryFresnel;              // Offset:   96
    float cryBrightness;           // Offset:  100
    float cryBrightnessPower;      // Offset:  104
    float glowIntensity;           // Offset:  108
    float glowLumThreshold;        // Offset:  112
    float glowShadowFadeRatio;     // Offset:  116
    float ssaoIntensity;           // Offset:  120
    float translucency;            // Offset:  124
    float ssrDistance;             // Offset:  128
    float volumeFogIntensity;      // Offset:  132
    uint flag;                     // Offset:  136
    float reserve;                 // Offset:  140
};

struct LightParam
{
    float3 pos;                    // Offset:    0
    float radius;                  // Offset:   12
    float3 color;                  // Offset:   16
    float radiusInv;               // Offset:   28
    float3 charaColor;             // Offset:   32
    float attenuation;             // Offset:   44
    float3 vec;                    // Offset:   48
    float spotAngleInv;            // Offset:   60
    float attenuationAngle;        // Offset:   64
    float specularIntensity;       // Offset:   68
    float specularGlossiness;      // Offset:   72
    float scatterAnisotropy;       // Offset:   76
    float3 scatterColor;           // Offset:   80
    float scatterDensity;          // Offset:   92
    float translucency;            // Offset:   96
    int shadowmapIndex;            // Offset:  100
    float scatterOffset;           // Offset:  104
    float userParam;               // Offset:  108
};

struct LightIndexData
{
    int pointLightIndices[63];     // Offset:    0
    uint pointLightCount;          // Offset:  252
    int spotLightIndices[63];      // Offset:  256
    uint spotLightCount;           // Offset:  508
    int lightProbeIndices[14];     // Offset:  512
    uint lightProbeCount;          // Offset:  568
    float tileDepthInv;            // Offset:  572
};

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

cbuffer cb_local : register(b2)
{
  float4 fadeBeginDistance_g : packoffset(c0);
  float4 fadeRangeInv_g : packoffset(c1);
  float density_g : packoffset(c2);
  float maxThickness_g : packoffset(c2.y);
  float depthThresholdNear_g : packoffset(c2.z);
  float depthThresholdFar_g : packoffset(c2.w);
  float densityByDepthDiffNear_g : packoffset(c3);
  float densityByDepthDiffFar_g : packoffset(c3.y);
  float2 uvClamp_g : packoffset(c3.z);
  float4 offsetsAndWeights[8] : packoffset(c4);
}

cbuffer cb_post_sky : register(b6)
{
  float3 incomingLight_g : packoffset(c0);
  float3 scatteringR_g : packoffset(c1);
  float3 scatteringM_g : packoffset(c2);
  float3 extinctionR_g : packoffset(c3);
  float3 extinctionM_g : packoffset(c4);
  float3 densityScaleHeight_g : packoffset(c5);
  float skyHorizonBottomLimit_g : packoffset(c5.w);
  float3 sunDirection_g : packoffset(c6);
  float skyHorizonTopLimit_g : packoffset(c6.w);
  float mieG_g : packoffset(c7);
  float distanceScale_g : packoffset(c7.y);
  float planetRadius_g : packoffset(c7.z);
  float atmosphereHeight_g : packoffset(c7.w);
  float sunIntensity_g : packoffset(c8);
  float skyLutNearOverFarClip_g : packoffset(c8.y);
  float skyLutCameraFarClip_g : packoffset(c8.z);
  float skyBrightness_g : packoffset(c8.w);
}

cbuffer cb_volume_fog : register(b7)
{
  float volumeLightBrightness_g : packoffset(c0);
  float volumeDensity_g : packoffset(c0.y);
  float volumeCameraFarOverMaxFar_g : packoffset(c0.z);
  float volumeCameraFarClip_g : packoffset(c0.w);
  float volumeNearOverFarClip_g : packoffset(c1);
  float volumeNearDistance_g : packoffset(c1.y);
  float volumeFarDistance_g : packoffset(c1.z);
  uint volumeShapeCount_g : packoffset(c1.w);
  float4 volumeColor_g : packoffset(c2);
  float2 voulumeLightTileSizeInv_g : packoffset(c3);
  float combineAlpha_g : packoffset(c3.z);
  float temporalRatioMax_g : packoffset(c3.w);
  float2 prevScaling_g : packoffset(c4);
  float2 prevUVClamp_g : packoffset(c4.z);
  float volumeNearFadeInv_g : packoffset(c5);
  float volumeNearFadeValue_g : packoffset(c5.y);
  float temporalRatioMin_g : packoffset(c5.z);
  float densityScale_g : packoffset(c5.w);
}

cbuffer cb_deferred : register(b4)
{
  float2 shadowUVClamp_g : packoffset(c0);
  float3 mapAOColor_g : packoffset(c1);
  float mapAOIntensity_g : packoffset(c1.w);
  float3 shadowEdgeColor_g : packoffset(c2);
  float shadowEdgeSharpness_g : packoffset(c2.w);
}

SamplerState samPoint_s : register(s0);
SamplerState samLinear_s : register(s1);
SamplerState SmplLinearWrap_s : register(s11);
SamplerState SmplCube_s : register(s14);
SamplerState SmplLinearClamp_s : register(s15);
SamplerComparisonState SmplShadow_s : register(s13);
Texture2D<float4> colorTexture : register(t0);
Texture2D<uint4> mrtTexture0 : register(t1);
Texture2D<uint4> mrtTexture1 : register(t2);
Texture2D<uint2> mrtTexture2 : register(t3);
Texture2D<float4> depthTexture : register(t4);
Texture2D<float4> ssaoTexture : register(t5);
StructuredBuffer<DeferredParam> deferredParams_g : register(t6);
Texture2D<float4> shadowTexture : register(t7);
Texture2D<float4> outlinePrepareTexture : register(t8);
StructuredBuffer<LightParam> dynamicLights_g : register(t11);
StructuredBuffer<LightIndexData> lightIndices_g : register(t12);
StructuredBuffer<float4x4> spotShadowMatrices_g : register(t14);
TextureCube<float4> texEnvMap_g : register(t17);
Texture2DArray<float4> spotShadowMaps : register(t18);
Texture3D<float4> atmosphereInscatterLUT : register(t19);
Texture3D<float4> atmosphereExtinctionLUT : register(t20);
Texture2D<float4> texMirror_g : register(t21);
Texture2D<float4> texSSRMap_g : register(t24);
Texture3D<float4> volumeFogTexture_g : register(t26);
Texture2D<float4> texCloudShadow : register(t27);

#include "../../shared.h"

// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_Position0,
  float4 v1 : TEXCOORD0,
  out float4 o0 : SV_Target0,
  out uint4 o1 : SV_Target1,
  out uint2 o2 : SV_Target2)
{
  float4 r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25;
  uint4 bitmask, uiDest;
  float4 fDest;
  uint width, height, num_levels;

  r0.xyzw = colorTexture.SampleLevel(samPoint_s, v1.xy, 0).xyzw;
  mrtTexture0.GetDimensions(0, fDest.x, fDest.y, fDest.z);
  r1.xy = fDest.xy;
  r1.xy = v1.xy * r1.xy;
  r1.xy = (int2)r1.xy;
  r1.zw = float2(0,0);
  r1.xyz = mrtTexture0.Load(r1.xyz).xyw;
  mrtTexture1.GetDimensions(0, fDest.x, fDest.y, fDest.z);
  r2.xy = fDest.xy;
  r2.xy = v1.xy * r2.xy;
  r2.xy = (int2)r2.xy;
  r2.zw = float2(0,0);
  r2.xyzw = mrtTexture1.Load(r2.xyz).xyzw;
  mrtTexture2.GetDimensions(0, fDest.x, fDest.y, fDest.z);
  r3.xy = fDest.xy;
  r3.xy = v1.xy * r3.xy;
  r3.xy = (int2)r3.xy;
  r3.zw = float2(0,0);
  r3.xy = mrtTexture2.Load(r3.xyz).xy;
  r4.z = depthTexture.SampleLevel(samPoint_s, v1.xy, 0).x;
  r5.xyz = ssaoTexture.SampleLevel(samLinear_s, v1.xy, 0).xyz;
  r4.xy = v1.zw * float2(2,-2) + float2(-1,1);
  r4.w = 1;
  r6.x = dot(r4.xyzw, viewProjInv_g._m00_m10_m20_m30);
  r6.y = dot(r4.xyzw, viewProjInv_g._m01_m11_m21_m31);
  r6.z = dot(r4.xyzw, viewProjInv_g._m02_m12_m22_m32);
  r6.w = dot(r4.xyzw, viewProjInv_g._m03_m13_m23_m33);
  r4.xyzw = r6.xyzw / r6.wwww;
  r1.w = dot(view_g._m02_m12_m22_m32, r4.xyzw);
  r6.xy = invVPSize_g.xy * v0.xy;
  r3.z = -r1.w / volumeCameraFarClip_g;
  r7.xy = resolutionScaling_g.xy * r6.xy;
  r3.z = r3.z * volumeCameraFarOverMaxFar_g + -volumeNearOverFarClip_g;
  r3.w = -volumeNearOverFarClip_g + 1;
  r7.z = r3.z / r3.w;
  r7.xyzw = volumeFogTexture_g.SampleLevel(SmplLinearClamp_s, r7.xyz, 0).xyzw;
  r3.z = (int)r1.z & 8;
  if (r3.z == 0) {
    r8.yz = (uint2)r3.xx >> int2(5,10);
    r8.x = r3.x;
    r8.xyz = (int3)r8.xyz & int3(31,31,31);
    r8.xyz = (uint3)r8.xyz;
    r9.x = 0.0322580636;
    r9.yz = r0.yz;
    r8.xyz = r9.xyz * r8.xyz;
    r3.zw = (int2)r1.zz & int2(1,64);
    if (r3.z != 0) {
      r9.x = r0.x;
      r9.yz = float2(0.0322580636,0.0322580636);
      r10.xyz = r9.xyz * r8.xyz;
      r3.z = r5.y * r5.z;
      r5.yzw = -r9.xyz * r8.xyz + r0.xyz;
      r9.xyz = r3.zzz * r5.yzw + r10.xyz;
      r3.z = (int)r1.z & 4;
      if (r3.z == 0) {
        r5.yz = outlinePrepareTexture.SampleLevel(samLinear_s, v1.xy, 0).xy;
        r10.x = r5.y;
        r10.yw = float2(1,1);
        r3.z = dot(projInv_g._m22_m32, r10.xy);
        r5.y = dot(projInv_g._m23_m33, r10.xy);
        r3.z = r3.z / r5.y;
        r11.xyzw = -fadeBeginDistance_g.xyzw + -r3.zzzz;
        r11.xyzw = saturate(fadeRangeInv_g.xyzw * r11.xyzw);
r12.xy = float2(maxThickness_g, depthThresholdNear_g);
    r12.z = densityByDepthDiffNear_g;
        r13.x = 1 + -r12.x;
        r13.y = depthThresholdFar_g + -r12.y;
        r13.z = densityByDepthDiffFar_g + -r12.z;
        r8.yzw = r11.yzw * r13.xyz + r12.xyz;
        r5.yw = offsetsAndWeights[0].xy * r8.yy + v1.xy;
        r5.yw = min(uvClamp_g.xy, r5.yw);
        r6.w = 0.5 * r8.y;
        r6.w = max(1, r6.w);
        r10.xy = offsetsAndWeights[0].xy * r6.ww + v1.xy;
        r10.xy = min(uvClamp_g.xy, r10.xy);
        r10.z = outlinePrepareTexture.SampleLevel(samLinear_s, r5.yw, 0).x;
        r5.y = outlinePrepareTexture.SampleLevel(samLinear_s, r10.xy, 0).y;
        r5.w = dot(projInv_g._m22_m32, r10.zw);
        r10.x = dot(projInv_g._m23_m33, r10.zw);
        r5.w = r5.w / r10.x;
        r10.x = cmp(r5.y < 0);
        r5.w = -r5.w + r3.z;
        r10.y = cmp(r5.w >= 0);
        r10.z = cmp(r5.w >= r8.z);
        r12.x = r10.z ? 1.000000 : 0;
        r5.y = r5.z + -r5.y;
        r10.z = cmp(abs(r5.y) >= 0.0299999993);
        r10.yz = r10.yz ? float2(1,1) : 0;
        r12.y = r10.y * r10.z + r12.x;
        r10.z = abs(r5.w);
        r10.w = r10.y * abs(r5.y) + r10.z;
        r5.yw = offsetsAndWeights[1].xy * r8.yy + v1.xy;
        r5.yw = min(uvClamp_g.xy, r5.yw);
        r11.yz = offsetsAndWeights[1].xy * r6.ww + v1.xy;
        r11.yz = min(uvClamp_g.xy, r11.yz);
        r13.x = outlinePrepareTexture.SampleLevel(samLinear_s, r5.yw, 0).x;
        r5.y = outlinePrepareTexture.SampleLevel(samLinear_s, r11.yz, 0).y;
        r13.yw = float2(1,1);
        r5.w = dot(projInv_g._m22_m32, r13.xy);
        r10.y = dot(projInv_g._m23_m33, r13.xy);
        r5.w = r5.w / r10.y;
        r10.y = cmp(r5.y < 0);
        r10.x = (int)r10.x | (int)r10.y;
        r5.w = -r5.w + r3.z;
        r10.y = cmp(r5.w >= 0);
        r10.y = r10.y ? 1.000000 : 0;
        r11.y = cmp(r5.w >= r8.z);
        r13.x = r11.y ? 1.000000 : 0;
        r5.y = r5.z + -r5.y;
        r11.y = cmp(abs(r5.y) >= 0.0299999993);
        r11.y = r11.y ? 1.000000 : 0;
        r13.y = r10.y * r11.y + r13.x;
        r11.yz = offsetsAndWeights[1].zz * r13.xy;
        r11.yz = r12.xy * offsetsAndWeights[0].zz + r11.yz;
        r12.z = abs(r5.w);
        r12.w = r10.y * abs(r5.y) + r12.z;
        r5.yw = max(r12.zw, r10.zw);
        r10.yz = offsetsAndWeights[2].xy * r8.yy + v1.xy;
        r10.yz = min(uvClamp_g.xy, r10.yz);
        r12.xy = offsetsAndWeights[2].xy * r6.ww + v1.xy;
        r12.xy = min(uvClamp_g.xy, r12.xy);
        r13.z = outlinePrepareTexture.SampleLevel(samLinear_s, r10.yz, 0).x;
        r10.y = outlinePrepareTexture.SampleLevel(samLinear_s, r12.xy, 0).y;
        r10.z = dot(projInv_g._m22_m32, r13.zw);
        r10.w = dot(projInv_g._m23_m33, r13.zw);
        r10.z = r10.z / r10.w;
        r10.w = cmp(r10.y < 0);
        r10.x = (int)r10.x | (int)r10.w;
        r10.z = -r10.z + r3.z;
        r10.w = cmp(r10.z >= 0);
        r10.w = r10.w ? 1.000000 : 0;
        r11.w = cmp(r10.z >= r8.z);
        r12.x = r11.w ? 1.000000 : 0;
        r10.y = -r10.y + r5.z;
        r11.w = cmp(abs(r10.y) >= 0.0299999993);
        r11.w = r11.w ? 1.000000 : 0;
        r12.y = r10.w * r11.w + r12.x;
        r11.yz = r12.xy * offsetsAndWeights[2].zz + r11.yz;
        r12.z = abs(r10.z);
        r12.w = r10.w * abs(r10.y) + r12.z;
        r5.yw = max(r12.zw, r5.yw);
        r10.yz = offsetsAndWeights[3].xy * r8.yy + v1.xy;
        r10.yz = min(uvClamp_g.xy, r10.yz);
        r12.xy = offsetsAndWeights[3].xy * r6.ww + v1.xy;
        r12.xy = min(uvClamp_g.xy, r12.xy);
        r13.x = outlinePrepareTexture.SampleLevel(samLinear_s, r10.yz, 0).x;
        r10.y = outlinePrepareTexture.SampleLevel(samLinear_s, r12.xy, 0).y;
        r13.yw = float2(1,1);
        r10.z = dot(projInv_g._m22_m32, r13.xy);
        r10.w = dot(projInv_g._m23_m33, r13.xy);
        r10.z = r10.z / r10.w;
        r10.w = cmp(r10.y < 0);
        r10.x = (int)r10.x | (int)r10.w;
        r10.z = -r10.z + r3.z;
        r10.w = cmp(r10.z >= 0);
        r10.w = r10.w ? 1.000000 : 0;
        r11.w = cmp(r10.z >= r8.z);
        r12.x = r11.w ? 1.000000 : 0;
        r10.y = -r10.y + r5.z;
        r11.w = cmp(abs(r10.y) >= 0.0299999993);
        r11.w = r11.w ? 1.000000 : 0;
        r12.y = r10.w * r11.w + r12.x;
        r11.yz = r12.xy * offsetsAndWeights[3].zz + r11.yz;
        r12.z = abs(r10.z);
        r12.w = r10.w * abs(r10.y) + r12.z;
        r5.yw = max(r12.zw, r5.yw);
        r10.yz = offsetsAndWeights[4].xy * r8.yy + v1.xy;
        r10.yz = min(uvClamp_g.xy, r10.yz);
        r12.xy = offsetsAndWeights[4].xy * r6.ww + v1.xy;
        r12.xy = min(uvClamp_g.xy, r12.xy);
        r13.z = outlinePrepareTexture.SampleLevel(samLinear_s, r10.yz, 0).x;
        r10.y = outlinePrepareTexture.SampleLevel(samLinear_s, r12.xy, 0).y;
        r10.z = dot(projInv_g._m22_m32, r13.zw);
        r10.w = dot(projInv_g._m23_m33, r13.zw);
        r10.z = r10.z / r10.w;
        r10.w = cmp(r10.y < 0);
        r10.x = (int)r10.x | (int)r10.w;
        r10.z = -r10.z + r3.z;
        r10.w = cmp(r10.z >= 0);
        r10.w = r10.w ? 1.000000 : 0;
        r11.w = cmp(r10.z >= r8.z);
        r12.x = r11.w ? 1.000000 : 0;
        r10.y = -r10.y + r5.z;
        r11.w = cmp(abs(r10.y) >= 0.0299999993);
        r11.w = r11.w ? 1.000000 : 0;
        r12.y = r10.w * r11.w + r12.x;
        r11.yz = r12.xy * offsetsAndWeights[4].zz + r11.yz;
        r12.z = abs(r10.z);
        r12.w = r10.w * abs(r10.y) + r12.z;
        r5.yw = max(r12.zw, r5.yw);
        r10.yz = offsetsAndWeights[5].xy * r8.yy + v1.xy;
        r10.yz = min(uvClamp_g.xy, r10.yz);
        r12.xy = offsetsAndWeights[5].xy * r6.ww + v1.xy;
        r12.xy = min(uvClamp_g.xy, r12.xy);
        r13.x = outlinePrepareTexture.SampleLevel(samLinear_s, r10.yz, 0).x;
        r10.y = outlinePrepareTexture.SampleLevel(samLinear_s, r12.xy, 0).y;
        r13.yw = float2(1,1);
        r10.z = dot(projInv_g._m22_m32, r13.xy);
        r10.w = dot(projInv_g._m23_m33, r13.xy);
        r10.z = r10.z / r10.w;
        r10.w = cmp(r10.y < 0);
        r10.x = (int)r10.x | (int)r10.w;
        r10.z = -r10.z + r3.z;
        r10.w = cmp(r10.z >= 0);
        r10.w = r10.w ? 1.000000 : 0;
        r11.w = cmp(r10.z >= r8.z);
        r12.x = r11.w ? 1.000000 : 0;
        r10.y = -r10.y + r5.z;
        r11.w = cmp(abs(r10.y) >= 0.0299999993);
        r11.w = r11.w ? 1.000000 : 0;
        r12.y = r10.w * r11.w + r12.x;
        r11.yz = r12.xy * offsetsAndWeights[5].zz + r11.yz;
        r12.z = abs(r10.z);
        r12.w = r10.w * abs(r10.y) + r12.z;
        r5.yw = max(r12.zw, r5.yw);
        r10.yz = offsetsAndWeights[6].xy * r8.yy + v1.xy;
        r10.yz = min(uvClamp_g.xy, r10.yz);
        r12.xy = offsetsAndWeights[6].xy * r6.ww + v1.xy;
        r12.xy = min(uvClamp_g.xy, r12.xy);
        r13.z = outlinePrepareTexture.SampleLevel(samLinear_s, r10.yz, 0).x;
        r10.y = outlinePrepareTexture.SampleLevel(samLinear_s, r12.xy, 0).y;
        r10.z = dot(projInv_g._m22_m32, r13.zw);
        r10.w = dot(projInv_g._m23_m33, r13.zw);
        r10.z = r10.z / r10.w;
        r10.w = cmp(r10.y < 0);
        r10.x = (int)r10.x | (int)r10.w;
        r10.z = -r10.z + r3.z;
        r10.w = cmp(r10.z >= 0);
        r10.w = r10.w ? 1.000000 : 0;
        r11.w = cmp(r10.z >= r8.z);
        r12.x = r11.w ? 1.000000 : 0;
        r10.y = -r10.y + r5.z;
        r11.w = cmp(abs(r10.y) >= 0.0299999993);
        r11.w = r11.w ? 1.000000 : 0;
        r12.y = r10.w * r11.w + r12.x;
        r11.yz = r12.xy * offsetsAndWeights[6].zz + r11.yz;
        r12.z = abs(r10.z);
        r12.w = r10.w * abs(r10.y) + r12.z;
        r5.yw = max(r12.zw, r5.yw);
        r10.yz = offsetsAndWeights[7].xy * r8.yy + v1.xy;
        r10.yz = min(uvClamp_g.xy, r10.yz);
        r12.xy = offsetsAndWeights[7].xy * r6.ww + v1.xy;
        r12.xy = min(uvClamp_g.xy, r12.xy);
        r10.y = outlinePrepareTexture.SampleLevel(samLinear_s, r10.yz, 0).x;
        r6.w = outlinePrepareTexture.SampleLevel(samLinear_s, r12.xy, 0).y;
        r10.z = 1;
        r8.y = dot(projInv_g._m22_m32, r10.yz);
        r10.y = dot(projInv_g._m23_m33, r10.yz);
        r8.y = r8.y / r10.y;
        r10.y = cmp(r6.w < 0);
        r10.x = (int)r10.x | (int)r10.y;
        r3.z = -r8.y + r3.z;
        r8.y = cmp(r3.z >= 0);
        r8.y = r8.y ? 1.000000 : 0;
        r8.z = cmp(r3.z >= r8.z);
        r12.x = r8.z ? 1.000000 : 0;
        r5.z = -r6.w + r5.z;
        r6.w = cmp(abs(r5.z) >= 0.0299999993);
        r6.w = r6.w ? 1.000000 : 0;
        r12.y = r8.y * r6.w + r12.x;
        r12.xy = r12.xy * offsetsAndWeights[7].zz + r11.yz;
        r10.z = abs(r3.z);
        r10.w = r8.y * abs(r5.z) + r10.z;
        r12.zw = max(r10.zw, r5.yw);
        r5.yz = r10.xx ? r12.xz : r12.yw;
        r10.yw = (uint2)r2.zw >> int2(4,4);
        r10.xz = r2.zw;
        r10.xyzw = (int4)r10.xyzw & int4(15,15,15,15);
        r10.xyzw = (uint4)r10.xyzw;
        r9.w = 0.0666666701;
        r12.xyzw = r10.xyzw * r9.xyzw;
        r3.z = cmp(0 < r10.w);
        r5.z = min(r5.z, r8.w);
        r5.z = r5.z / r8.w;
        r5.z = log2(r5.z);
        r5.z = 1.25 * r5.z;
        r5.z = exp2(r5.z);
        r5.y = r5.y * r5.y;
        r5.y = r5.y * r12.w;
        r5.y = density_g * r5.y;
        r5.y = min(1, r5.y);
        r5.y = r5.y * r5.z;
        r5.z = 1 + -r11.x;
        r5.y = r5.y * r5.z;
        r8.yzw = r12.xyz * float3(0.0666666701,0.0666666701,0.0666666701) + -r9.xyz;
        r5.yzw = r5.yyy * r8.yzw + r9.xyz;
        r9.xyz = r3.zzz ? r5.yzw : r9.xyz;
      }
    } else {
      r3.z = 1 + -r5.x;
      r3.z = r8.x * r3.z;
      r5.yzw = r0.xyz * mapAOColor_g.xyz + -r0.xyz;
      r9.xyz = r3.zzz * r5.yzw + r0.xyz;
    }
    r5.yzw = r9.xyz * r7.www + r7.xyz;
    r5.yzw = r5.yzw + -r9.xyz;
    r5.yzw = combineAlpha_g * r5.yzw + r9.xyz;
    o0.xyz = r3.www ? r9.xyz : r5.yzw;
    o0.w = r0.w;
    o1.xyzw = r2.xyzw;
    o2.xy = r3.xy;
    return;
  }
  r3.zw = r4.xz * cloudShadowScale_g + cloudShadowOffset_g.xy;
  r3.zw = texCloudShadow.SampleLevel(SmplLinearWrap_s, r3.zw, 0).xw;
  r0.w = -1 + r3.z;
  r0.w = r3.w * r0.w + 1;
  r3.zw = min(shadowUVClamp_g.xy, v1.xy);
  r3.z = shadowTexture.SampleLevel(samLinear_s, r3.zw, 0).x;
  r3.w = min(r3.z, r0.w);
  r1.xy = (uint2)r1.xy;
  r8.zw = r1.xy * float2(3.05180438e-05,3.05180438e-05) + float2(-1,-1);
  r1.x = 3.14159274 * r8.z;
  sincos(r1.x, r1.x, r9.x);
  r1.y = r8.w * r8.w;
  r5.y = -r8.w * r8.w + 1;
  r5.y = sqrt(r5.y);
  r8.x = r9.x * r5.y;
  r8.y = r5.y * r1.x;
  r2.xyzw = (uint4)r2.xyzw;
  r9.xyzw = float4(0.00392156886,0.00392156886,0.00392156886,0.00392156886) * r2.wxyz;
  r2.yz = (uint2)r3.yy >> int2(5,10);
  r2.x = r3.y;
  r2.xyz = (int3)r2.xyz & int3(31,31,31);
  r2.xyz = (uint3)r2.xyz;
  r5.yzw = float3(0.0322580636,0.0322580636,0.0322580636) * r2.xyz;
  r1.x = min(0x00004e1f, (uint)r3.x);
  r10.x = deferredParams_g[r1.x].shadowColor.x;
  r10.y = deferredParams_g[r1.x].shadowColor.y;
  r10.z = deferredParams_g[r1.x].shadowColor.z;
  r10.w = deferredParams_g[r1.x].emissive;
  r11.x = deferredParams_g[r1.x].specularColor.x;
  r11.y = deferredParams_g[r1.x].specularColor.y;
  r11.z = deferredParams_g[r1.x].specularColor.z;
  r11.w = deferredParams_g[r1.x].rimLightPower;
  r12.x = deferredParams_g[r1.x].rimLightColor.x;
  r12.y = deferredParams_g[r1.x].rimLightColor.y;
  r12.z = deferredParams_g[r1.x].rimLightColor.z;
  r12.w = deferredParams_g[r1.x].rimIntensity;
  r13.x = deferredParams_g[r1.x].fresnels.x;
  r13.y = deferredParams_g[r1.x].fresnels.y;
  r13.z = deferredParams_g[r1.x].fresnels.z;
  r14.x = deferredParams_g[r1.x].specularGlossinesses.x;
  r14.y = deferredParams_g[r1.x].specularGlossinesses.y;
  r14.z = deferredParams_g[r1.x].specularGlossinesses.z;
  r14.w = deferredParams_g[r1.x].dynamicLightIntensity;
  r15.x = deferredParams_g[r1.x].materialFogIntensity;
  r15.y = deferredParams_g[r1.x].metalness;
  r15.z = deferredParams_g[r1.x].roughness;
  r15.w = deferredParams_g[r1.x].cryRefractionIndex;
  r16.x = deferredParams_g[r1.x].cryFresnel;
  r16.y = deferredParams_g[r1.x].cryBrightness;
  r16.z = deferredParams_g[r1.x].cryBrightnessPower;
  r16.w = deferredParams_g[r1.x].glowIntensity;
  r17.x = deferredParams_g[r1.x].glowLumThreshold;
  r17.y = deferredParams_g[r1.x].glowShadowFadeRatio;
  r17.z = deferredParams_g[r1.x].ssaoIntensity;
  r18.x = deferredParams_g[r1.x].ssrDistance;
  r18.y = deferredParams_g[r1.x].volumeFogIntensity;
  r18.z = deferredParams_g[r1.x].flag;
  r13.w = r14.x;
  r19.xz = r13.yz;
  r19.yw = r14.yz;
  r2.xy = r19.xy + -r13.xw;
  r2.xy = r5.yy * r2.xy + r13.xw;
  r3.xy = r19.zw + -r2.xy;
  r2.xy = r5.zz * r3.xy + r2.xy;
  r3.xy = lightTileSizeInv_g.xy * v0.xy;
  r3.xy = (uint2)r3.xy;
  r1.x = (uint)r3.y << 5;
  r3.y = (int)r3.x + (int)r1.x;
  r3.y = lightIndices_g[r3.y].tileDepthInv;
  r3.y = r3.y * -r1.w;
  r3.y = min(7, r3.y);
  r3.y = max(0, r3.y);
  r3.y = (uint)r3.y;
  r1.x = mad((int)r3.y, 576, (int)r1.x);
  r1.x = (int)r3.x + (int)r1.x;
  r1.x = min(4607, (uint)r1.x);
  r13.xyz = lightProbe_g[1].xyz * r8.xxx + lightProbe_g[0].xyz;
  r13.xyz = lightProbe_g[2].xyz * r8.yyy + r13.xyz;
  r13.xyz = lightProbe_g[3].xyz * r8.www + r13.xyz;
  r14.xyz = lightProbe_g[4].xyz * r8.www;
  r13.xyz = r14.xyz * r8.xxx + r13.xyz;
  r14.xyz = lightProbe_g[5].xyz * r8.yyy;
  r13.xyz = r14.xyz * r8.www + r13.xyz;
  r14.xyz = lightProbe_g[6].xyz * r8.yyy;
  r13.xyz = r14.xyz * r8.xxx + r13.xyz;
  r1.y = r1.y * 3 + -1;
  r13.xyz = lightProbe_g[7].xyz * r1.yyy + r13.xyz;
  r1.y = r8.y * r8.y;
  r1.y = r8.x * r8.x + -r1.y;
  r13.xyz = lightProbe_g[8].xyz * r1.yyy + r13.xyz;
  r13.xyz = max(float3(0,0,0), r13.xyz);
  r14.x = viewInv_g._m30 + -r4.x;
  r14.y = viewInv_g._m31 + -r4.y;
  r14.z = viewInv_g._m32 + -r4.z;
  r1.y = dot(r14.xyz, r14.xyz);
  r1.y = rsqrt(r1.y);
  r19.xyz = r14.xyz * r1.yyy;
  r10.xyz = sceneShadowColor_g.xyz + r10.xyz;
  r10.xyz = min(float3(1,1,1), r10.xyz);
  r3.x = r10.w * r5.w;
  r3.y = dot(r8.xyw, r19.xyz);
  r20.xyzw = (int4)r18.zzzz & int4(1,2,4,16);
  r3.w = r20.x ? r3.w : 1;
  r21.xyz = r14.xyz * r1.yyy + -lightDirection_g.xyz;
  r5.y = dot(r21.xyz, r21.xyz);
  r5.y = rsqrt(r5.y);
  r21.xyz = r21.xyz * r5.yyy;
  r5.y = lightSpecularGlossiness_g * r2.y;
  r5.z = saturate(dot(r21.xyz, r8.xyw));
  r5.y = max(0.00100000005, r5.y);
  r5.z = log2(r5.z);
  r5.y = r5.y * r5.z;
  r5.y = exp2(r5.y);
  r5.y = r5.y * r3.w;
  r5.y = lightSpecularIntensity_g * r5.y;
  r5.y = r20.y ? r5.y : 0;
  r11.xyz = r5.yyy * r11.xyz;
  r11.xyz = lightColor_g.xyz * r11.xyz;
  if (r20.z != 0) {
    r5.yz = r15.yz * r9.yz;
    r6.w = (int)r18.z & 32;
    if (r6.w != 0) {
      r21.y = resolutionScaling_g.y + -v1.y;
      r21.x = v1.x;
      r21.xyz = texMirror_g.SampleLevel(SmplLinearClamp_s, r21.xy, 0).xyz;
    } else {
      r8.z = r3.y + r3.y;
      r22.xyz = r8.xyw * -r8.zzz + r19.xyz;
      texEnvMap_g.GetDimensions(0, width, height, num_levels);
      r22.xyz = float3(1,-1,-1) * r22.xyz;
      r8.z = (float)(num_levels - 1);
      r8.z = r8.z * r5.z;
      r21.xyz = texEnvMap_g.SampleLevel(SmplCube_s, r22.xyz, r8.z).xyz;
    }
    r8.z = (int)r1.z & 2;
    if (r8.z != 0) {
      r20.xz = resolutionScaling_g.xy * v1.zw;
      r22.xyzw = texSSRMap_g.SampleLevel(SmplLinearClamp_s, r20.xz, 0).xyzw;
      r22.xyz = r22.xyz + -r21.xyz;
      r21.xyz = r22.www * r22.xyz + r21.xyz;
    }
    r8.z = cmp(0 < r2.x);
    r9.y = 1 + -abs(r3.y);
    r9.y = max(0, r9.y);
    r9.y = log2(r9.y);
    r2.x = r9.y * r2.x;
    r2.x = exp2(r2.x);
    r2.x = r8.z ? r2.x : 1;
    r8.z = r5.y * r2.x;
    r22.xyz = r0.xyz * r21.xyz + -r0.xyz;
    r22.xyz = r8.zzz * r22.xyz + r0.xyz;
    r8.z = dot(r21.xyz, float3(0.298999995,0.587000012,0.114));
    r5.z = r5.z * -9 + 10;
    r8.z = log2(r8.z);
    r5.z = r8.z * r5.z;
    r5.z = exp2(r5.z);
    r8.z = 1 + -r5.z;
    r5.y = r5.y * r8.z + r5.z;
    r21.xyz = r21.xyz * r5.yyy;
    r21.xyz = r21.xyz * r2.xxx;
    r2.x = -r9.z * r15.z + 1;
    r21.xyz = r21.xyz * r2.xxx + r11.xyz;
    r11.xyz = r6.www ? r11.xyz : r21.xyz;
  } else {
    r2.x = (int)r18.z & 8;
    if (r2.x != 0) {
      r2.x = r3.y + r3.y;
      r21.xyz = r8.xyw * -r2.xxx + r19.xyz;
      r2.x = 1 / r15.w;
      r5.y = dot(-r19.xyz, r8.xyw);
      r5.z = r2.x * r2.x;
      r6.w = -r5.y * r5.y + 1;
      r5.z = -r5.z * r6.w + 1;
      r6.w = sqrt(r5.z);
      r5.y = r2.x * r5.y + r6.w;
      r5.z = cmp(r5.z >= 0);
      r23.xyz = r5.yyy * r8.xyw;
      r19.xyz = r2.xxx * -r19.xyz + -r23.xyz;
      r19.xyz = r5.zzz ? r19.xyz : 0;
      r2.x = r15.z * r9.z;
      texEnvMap_g.GetDimensions(0, width, height, num_levels);
      r21.xyz = float3(1,-1,-1) * r21.xyz;
      r5.y = (float)(num_levels - 1);
      r2.x = r5.y * r2.x;
      r21.xyz = texEnvMap_g.SampleLevel(SmplCube_s, r21.xyz, r2.x).xyz;
      r5.y = (int)r1.z & 2;
      if (r5.y != 0) {
        r5.yz = resolutionScaling_g.xy * v1.zw;
        r23.xyzw = texSSRMap_g.SampleLevel(SmplLinearClamp_s, r5.yz, 0).xyzw;
        r23.xyz = r23.xyz + -r21.xyz;
        r21.xyz = r23.www * r23.xyz + r21.xyz;
      }
      r19.xyz = texEnvMap_g.SampleLevel(SmplCube_s, r19.xyz, r2.x).xyz;
      r2.x = cmp(0 < r16.x);
      r5.y = 1 + -abs(r3.y);
      r5.y = max(0, r5.y);
      r5.y = log2(r5.y);
      r5.y = r16.x * r5.y;
      r5.y = exp2(r5.y);
      r2.x = r2.x ? r5.y : 1;
      r23.xyz = r0.xyz * r21.xyz + -r0.xyz;
      r23.xyz = r2.xxx * r23.xyz + r0.xyz;
      r2.w = -r2.w * 0.00392156886 + 1;
      r9.x = r2.x * r2.w + r9.x;
      r2.w = abs(r3.y) * r16.y;
      r2.w = log2(r2.w);
      r2.w = r16.z * r2.w;
      r2.w = exp2(r2.w);
      r16.xyz = r9.xxx * r21.xyz;
      r16.xyz = r16.xyz * r2.xxx;
      r2.x = -r9.z * r15.z + 1;
      r11.xyz = r16.xyz * r2.xxx + r11.xyz;
      r15.yzw = r2.www * r19.xyz;
      r22.xyz = r23.xyz * r15.yzw;
    } else {
      r22.xyz = r0.xyz;
    }
  }
  r2.x = r9.x * r3.w;
  r15.yzw = float3(1,1,1) + -r10.xyz;
  r10.xyz = r2.xxx * r15.yzw + r10.xyz;
  r2.x = log2(r3.z);
  r2.x = shadowEdgeSharpness_g * r2.x;
  r2.x = exp2(r2.x);
  if (shader_injection_data.shadow_edge_tint > 1.5f) {
    // Improved: vibrancy boost in penumbra region
    float penumbra = 1.0f - 2.0f * abs(r3.z - 0.5f);
    penumbra = saturate(penumbra / shader_injection_data.shadow_penumbra_detection);

    // Vibrancy source: colorTexture (r0.xyz) — has full scene color.
    float3 surfaceColor = r0.xyz;
    float luma = dot(surfaceColor, float3(0.333f, 0.333f, 0.333f));
    float maxC = max(surfaceColor.x, max(surfaceColor.y, surfaceColor.z));
    float vibrance = shader_injection_data.shadow_penumbra_vibrance;
    float3 surfaceVibrancy;
    if (vibrance <= 1.0f) {
      surfaceVibrancy = luma + vibrance * (surfaceColor - luma);
    } else {
      float vibranceFactor = 1.0f + (vibrance - 1.0f) * (1.0f - maxC);
      surfaceVibrancy = luma + vibranceFactor * (surfaceColor - luma);
    }
    surfaceVibrancy = saturate(surfaceVibrancy);

    // Vibrancy-adjusted Falcom shadowEdgeColor
    float edgeLuma = dot(shadowEdgeColor_g.xyz, float3(0.333f, 0.333f, 0.333f));
    float edgeMaxC = max(shadowEdgeColor_g.x, max(shadowEdgeColor_g.y, shadowEdgeColor_g.z));
    float edgeVibrance = shader_injection_data.shadow_penumbra_edge_vibrance;
    float3 edgeVibrancy;
    if (edgeVibrance <= 1.0f) {
      edgeVibrancy = edgeLuma + edgeVibrance * (shadowEdgeColor_g.xyz - edgeLuma);
    } else {
      float edgeFactor = 1.0f + (edgeVibrance - 1.0f) * (1.0f - edgeMaxC);
      edgeVibrancy = edgeLuma + edgeFactor * (shadowEdgeColor_g.xyz - edgeLuma);
    }
    edgeVibrancy = saturate(edgeVibrancy);

    // Vibrancy-adjusted lightColor (sun color)
    float lightLuma = dot(lightColor_g.xyz, float3(0.333f, 0.333f, 0.333f));
    float lightMaxC = max(lightColor_g.x, max(lightColor_g.y, lightColor_g.z));
    float lightSaturation = shader_injection_data.shadow_penumbra_lightcolor_saturation;
    float3 lightVibrancy;
    if (lightSaturation <= 1.0f) {
      lightVibrancy = lightLuma + lightSaturation * (lightColor_g.xyz - lightLuma);
    } else {
      float lightFactor = 1.0f + (lightSaturation - 1.0f) * (1.0f - lightMaxC);
      lightVibrancy = lightLuma + lightFactor * (lightColor_g.xyz - lightLuma);
    }
    lightVibrancy = saturate(lightVibrancy);

    // Blend between surface vibrancy and Falcom edge vibrancy
    float3 tintColor = lerp(surfaceVibrancy, edgeVibrancy, shader_injection_data.shadow_penumbra_falcom_blend);

    // Blend toward lightColor vibrancy
    tintColor = lerp(tintColor, lightVibrancy, shader_injection_data.shadow_penumbra_lightcolor_blend);

    // Apply brightness
    tintColor *= shader_injection_data.shadow_penumbra_color_brightness;
    tintColor = saturate(tintColor);

    // Blend vibrancy color onto the shadow-edge-processed r10.xyz
    float strength = shader_injection_data.shadow_penumbra_color_strength * penumbra;
    float3 finalColor = lerp(r10.xyz, tintColor, strength);

    // Debug views — replace output and return early
    // o1/o2 semantics differ in Sora 2nd (glow mask / SSR distance) — approximated.
    static const float kDebugHdrScale = 0.05;
    int debugMode = (int)shader_injection_data.shadow_penumbra_debug_view;
    if (debugMode == 1) {
      o0.rgb = float3(penumbra, penumbra, penumbra) * kDebugHdrScale;
      o0.a = 1.0; o1.xyzw = uint4(0, 255 * saturate(0.1 * r16.w), 0, 0); o2.x = 0; o2.y = min(0xffff, (uint)(65.535 * r18.x)); return;
    }
    if (debugMode == 2) {
      o0.rgb = tintColor * kDebugHdrScale;
      o0.a = 1.0; o1.xyzw = uint4(0, 255 * saturate(0.1 * r16.w), 0, 0); o2.x = 0; o2.y = min(0xffff, (uint)(65.535 * r18.x)); return;
    }
    if (debugMode == 3) {
      o0.rgb = finalColor * kDebugHdrScale;
      o0.a = 1.0; o1.xyzw = uint4(0, 255 * saturate(0.1 * r16.w), 0, 0); o2.x = 0; o2.y = min(0xffff, (uint)(65.535 * r18.x)); return;
    }
    if (debugMode == 4) {
      // Sun color (lightColor_g from cb_scene c25) applied uniformly
      o0.rgb = lightColor_g.xyz * kDebugHdrScale;
      o0.a = 1.0; o1.xyzw = uint4(0, 255 * saturate(0.1 * r16.w), 0, 0); o2.x = 0; o2.y = min(0xffff, (uint)(65.535 * r18.x)); return;
    }

    r15.yzw = finalColor;
  } else if (shader_injection_data.shadow_edge_tint > 0.5f) {
    r15.yzw = shadowEdgeColor_g.xyz * r0.www;
    r16.xyz = -shadowEdgeColor_g.xyz * r0.www + r10.xyz;
    r15.yzw = r3.zzz * r16.xyz + r15.yzw;
  } else {
    r15.yzw = r10.xyz;
  }
  r0.w = r2.x * r9.x;
  r9.xyz = r15.yzw + -r10.xyz;
  r9.xyz = r0.www * r9.xyz + r10.xyz;
  r9.xyz = r9.xyz * lightColor_g.xyz + r13.xyz;
  r0.w = min(1, r3.x);
  r10.xyz = float3(1,1,1) + -r9.xyz;
  r9.xyz = r0.www * r10.xyz + r9.xyz;
  r0.w = 1 + -abs(r3.y);
  r0.w = max(0, r0.w);
  r0.w = r0.w * r12.w;
  r0.w = log2(r0.w);
  r0.w = r11.w * r0.w;
  r0.w = exp2(r0.w);
  r0.w = min(1, r0.w);
  r3.xyz = r12.xyz * r0.www + r11.xyz;
  if (r20.y != 0) {
    r0.w = lightIndices_g[r1.x].pointLightCount;
    r0.w = min(63, (uint)r0.w);
    r10.xyz = float3(0,0,0);
    r11.xyz = float3(0,0,0);
    r2.x = 0;
    while (true) {
      r2.w = cmp((uint)r2.x >= (uint)r0.w);
      if (r2.w != 0) break;
      r2.w = lightIndices_g[r1.x].pointLightIndices[r2.x];
      r12.x = dynamicLights_g[r2.w].pos.x;
      r12.y = dynamicLights_g[r2.w].pos.y;
      r12.z = dynamicLights_g[r2.w].pos.z;
      r12.xyz = r12.xyz + -r4.xyz;
      r5.y = dot(r12.xyz, r12.xyz);
      r5.z = sqrt(r5.y);
      r6.w = dynamicLights_g[r2.w].radiusInv;
      r5.z = r6.w * r5.z;
      r6.w = dynamicLights_g[r2.w].attenuation;
      r5.z = log2(abs(r5.z));
      r5.z = r6.w * r5.z;
      r5.z = exp2(r5.z);
      r5.z = 1 + -r5.z;
      r5.z = max(0, r5.z);
      r6.w = cmp(0 < r5.z);
      if (r6.w != 0) {
        r5.y = rsqrt(r5.y);
        r12.xyz = r12.xyz * r5.yyy;
        r5.y = dynamicLights_g[r2.w].translucency;
        r6.w = dot(r12.xyz, r8.xyw);
        r5.y = max(r6.w, r5.y);
        r5.y = r5.z * r5.y;
        r13.x = dynamicLights_g[r2.w].color.x;
        r13.y = dynamicLights_g[r2.w].color.y;
        r13.z = dynamicLights_g[r2.w].color.z;
        r11.xyz = r13.xyz * r5.yyy + r11.xyz;
        r12.xyz = r14.xyz * r1.yyy + r12.xyz;
        r5.z = dot(r12.xyz, r12.xyz);
        r5.z = rsqrt(r5.z);
        r12.xyz = r12.xyz * r5.zzz;
        r15.y = dynamicLights_g[r2.w].specularIntensity;
        r15.z = dynamicLights_g[r2.w].specularGlossiness;
        r2.w = r15.z * r2.y;
        r5.z = saturate(dot(r12.xyz, r8.xyw));
        r2.w = max(0.00100000005, r2.w);
        r5.z = log2(r5.z);
        r2.w = r5.z * r2.w;
        r2.w = exp2(r2.w);
        r12.xyz = r13.xyz * r2.www;
        r12.xyz = r12.xyz * r5.yyy;
        r10.xyz = r12.xyz * r15.yyy + r10.xyz;
      }
      r2.x = (int)r2.x + 1;
    }
    r11.xyz = r11.xyz * r14.www + r9.xyz;
    r0.w = lightIndices_g[r1.x].spotLightCount;
    r0.w = min(63, (uint)r0.w);
    r12.yw = localShadowResolutionInv_g * float2(1.25,-1.25);
    r12.xz = float2(0,0);
    r13.xyz = r10.xyz;
    r15.yzw = float3(0,0,0);
    r2.x = 0;
    while (true) {
      r2.w = cmp((uint)r2.x >= (uint)r0.w);
      if (r2.w != 0) break;
      r2.w = lightIndices_g[r1.x].spotLightIndices[r2.x];
      r16.x = dynamicLights_g[r2.w].pos.x;
      r16.y = dynamicLights_g[r2.w].pos.y;
      r16.z = dynamicLights_g[r2.w].pos.z;
      r16.xyz = r16.xyz + -r4.xyz;
      r5.y = dot(r16.xyz, r16.xyz);
      r5.z = rsqrt(r5.y);
      r16.xyz = r16.xyz * r5.zzz;
      r19.x = dynamicLights_g[r2.w].vec.x;
      r19.y = dynamicLights_g[r2.w].vec.y;
      r19.z = dynamicLights_g[r2.w].vec.z;
      r19.w = dynamicLights_g[r2.w].spotAngleInv;
      r5.z = dot(r16.xyz, r19.xyz);
      r5.z = max(0, r5.z);
      r5.z = 1 + -r5.z;
      r5.z = r5.z * r19.w;
      r6.w = dynamicLights_g[r2.w].attenuationAngle;
      r5.z = log2(r5.z);
      r5.z = r6.w * r5.z;
      r5.z = exp2(r5.z);
      r5.z = 1 + -r5.z;
      r5.z = max(0, r5.z);
      r6.w = cmp(0 < r5.z);
      if (r6.w != 0) {
        r5.y = sqrt(r5.y);
        r6.w = dynamicLights_g[r2.w].radiusInv;
        r5.y = r6.w * r5.y;
        r6.w = dynamicLights_g[r2.w].attenuation;
        r5.y = log2(abs(r5.y));
        r5.y = r6.w * r5.y;
        r5.y = exp2(r5.y);
        r5.y = 1 + -r5.y;
        r5.y = max(0, r5.y);
        r5.y = r5.z * r5.y;
        r5.z = cmp(0 < r5.y);
        if (r5.z != 0) {
          r19.x = dynamicLights_g[r2.w].translucency;
          r19.y = dynamicLights_g[r2.w].shadowmapIndex;
          r5.z = cmp((int)r19.y != -1);
          if (r5.z != 0) {
            r21.xyzw = spotShadowMatrices_g[r19.y]._m00_m10_m20_m30;
            r23.xyzw = spotShadowMatrices_g[r19.y]._m01_m11_m21_m31;
            r24.xyzw = spotShadowMatrices_g[r19.y]._m02_m12_m22_m32;
            r25.xyzw = spotShadowMatrices_g[r19.y]._m03_m13_m23_m33;
            r20.x = dot(r4.xyzw, r21.xyzw);
            r20.y = dot(r4.xyzw, r23.xyzw);
            r20.z = dot(r4.xyzw, r24.xyzw);
            r5.z = dot(r4.xyzw, r25.xyzw);
            r21.xyz = r20.xyz / r5.zzz;
            r21.w = (uint)r19.y;
            r5.z = spotShadowMaps.SampleCmpLevelZero(SmplShadow_s, r21.xyw, r21.z).x;
            r20.xy = r21.xy + r12.yx;
            r20.z = r21.w;
            r6.w = spotShadowMaps.SampleCmpLevelZero(SmplShadow_s, r20.xyz, r21.z).x;
            r6.w = 0.200000003 * r6.w;
            r5.z = r5.z * 0.200000003 + r6.w;
            r20.xy = r21.xy + r12.wz;
            r6.w = spotShadowMaps.SampleCmpLevelZero(SmplShadow_s, r20.xyz, r21.z).x;
            r5.z = r6.w * 0.200000003 + r5.z;
            r20.xy = r21.xy + r12.xy;
            r6.w = spotShadowMaps.SampleCmpLevelZero(SmplShadow_s, r20.xyz, r21.z).x;
            r5.z = r6.w * 0.200000003 + r5.z;
            r20.xy = r21.xy + r12.zw;
            r6.w = spotShadowMaps.SampleCmpLevelZero(SmplShadow_s, r20.xyz, r21.z).x;
            r5.z = r6.w * 0.200000003 + r5.z;
            r5.y = r5.y * r5.z;
          }
          r5.z = dot(r16.xyz, r8.xyw);
          r5.z = max(r19.x, r5.z);
          r5.y = r5.y * r5.z;
          r19.x = dynamicLights_g[r2.w].color.x;
          r19.y = dynamicLights_g[r2.w].color.y;
          r19.z = dynamicLights_g[r2.w].color.z;
          r15.yzw = r19.xyz * r5.yyy + r15.yzw;
          r16.xyz = r14.xyz * r1.yyy + r16.xyz;
          r5.z = dot(r16.xyz, r16.xyz);
          r5.z = rsqrt(r5.z);
          r16.xyz = r16.xyz * r5.zzz;
          r20.x = dynamicLights_g[r2.w].specularIntensity;
          r20.y = dynamicLights_g[r2.w].specularGlossiness;
          r2.w = r20.y * r2.y;
          r5.z = saturate(dot(r16.xyz, r8.xyw));
          r2.w = max(0.00100000005, r2.w);
          r5.z = log2(r5.z);
          r2.w = r5.z * r2.w;
          r2.w = exp2(r2.w);
          r16.xyz = r19.xyz * r2.www;
          r16.xyz = r16.xyz * r5.yyy;
          r13.xyz = r16.xyz * r20.xxx + r13.xyz;
        }
      }
      r2.x = (int)r2.x + 1;
    }
    r2.xyw = r15.yzw * r14.www + r11.xyz;
    r3.xyz = r13.xyz * r14.www + r3.xyz;
  } else {
    r0.w = lightIndices_g[r1.x].pointLightCount;
    r0.w = min(63, (uint)r0.w);
    r10.xyzw = float4(0,0,0,0);
    while (true) {
      r1.y = cmp((uint)r10.w >= (uint)r0.w);
      if (r1.y != 0) break;
      r1.y = lightIndices_g[r1.x].pointLightIndices[r10.w];
      r11.x = dynamicLights_g[r1.y].pos.x;
      r11.y = dynamicLights_g[r1.y].pos.y;
      r11.z = dynamicLights_g[r1.y].pos.z;
      r11.xyz = r11.xyz + -r4.xyz;
      r5.y = dot(r11.xyz, r11.xyz);
      r5.z = sqrt(r5.y);
      r6.w = dynamicLights_g[r1.y].radiusInv;
      r5.z = r6.w * r5.z;
      r6.w = dynamicLights_g[r1.y].attenuation;
      r5.z = log2(abs(r5.z));
      r5.z = r6.w * r5.z;
      r5.z = exp2(r5.z);
      r5.z = 1 + -r5.z;
      r5.z = max(0, r5.z);
      r6.w = cmp(0 < r5.z);
      if (r6.w != 0) {
        r6.w = dynamicLights_g[r1.y].translucency;
        r5.y = rsqrt(r5.y);
        r11.xyz = r11.xyz * r5.yyy;
        r5.y = dot(r11.xyz, r8.xyw);
        r5.y = max(r6.w, r5.y);
        r11.x = dynamicLights_g[r1.y].color.x;
        r11.y = dynamicLights_g[r1.y].color.y;
        r11.z = dynamicLights_g[r1.y].color.z;
        r11.xyz = r11.xyz * r5.zzz;
        r10.xyz = r11.xyz * r5.yyy + r10.xyz;
      }
      r10.w = (int)r10.w + 1;
    }
    r9.xyz = r10.xyz * r14.www + r9.xyz;
    r0.w = lightIndices_g[r1.x].spotLightCount;
    r0.w = min(63, (uint)r0.w);
    r10.yw = localShadowResolutionInv_g * float2(1.25,-1.25);
    r10.xz = float2(0,0);
    r11.xyzw = float4(0,0,0,0);
    while (true) {
      r1.y = cmp((uint)r11.w >= (uint)r0.w);
      if (r1.y != 0) break;
      r1.y = lightIndices_g[r1.x].spotLightIndices[r11.w];
      r12.x = dynamicLights_g[r1.y].pos.x;
      r12.y = dynamicLights_g[r1.y].pos.y;
      r12.z = dynamicLights_g[r1.y].pos.z;
      r12.xyz = r12.xyz + -r4.xyz;
      r5.y = dot(r12.xyz, r12.xyz);
      r5.z = rsqrt(r5.y);
      r12.xyz = r12.xyz * r5.zzz;
      r13.x = dynamicLights_g[r1.y].vec.x;
      r13.y = dynamicLights_g[r1.y].vec.y;
      r13.z = dynamicLights_g[r1.y].vec.z;
      r13.w = dynamicLights_g[r1.y].spotAngleInv;
      r5.z = dot(r12.xyz, r13.xyz);
      r5.z = max(0, r5.z);
      r5.z = 1 + -r5.z;
      r5.z = r5.z * r13.w;
      r6.w = dynamicLights_g[r1.y].attenuationAngle;
      r5.z = log2(r5.z);
      r5.z = r6.w * r5.z;
      r5.z = exp2(r5.z);
      r5.z = 1 + -r5.z;
      r5.z = max(0, r5.z);
      r6.w = cmp(0 < r5.z);
      if (r6.w != 0) {
        r5.y = sqrt(r5.y);
        r6.w = dynamicLights_g[r1.y].radiusInv;
        r5.y = r6.w * r5.y;
        r6.w = dynamicLights_g[r1.y].attenuation;
        r5.y = log2(abs(r5.y));
        r5.y = r6.w * r5.y;
        r5.y = exp2(r5.y);
        r5.y = 1 + -r5.y;
        r5.y = max(0, r5.y);
        r5.y = r5.z * r5.y;
        r5.z = cmp(0 < r5.y);
        if (r5.z != 0) {
          r13.x = dynamicLights_g[r1.y].translucency;
          r13.y = dynamicLights_g[r1.y].shadowmapIndex;
          r5.z = cmp((int)r13.y != -1);
          if (r5.z != 0) {
            r19.xyzw = spotShadowMatrices_g[r13.y]._m00_m10_m20_m30;
            r21.xyzw = spotShadowMatrices_g[r13.y]._m01_m11_m21_m31;
            r23.xyzw = spotShadowMatrices_g[r13.y]._m02_m12_m22_m32;
            r24.xyzw = spotShadowMatrices_g[r13.y]._m03_m13_m23_m33;
            r14.x = dot(r4.xyzw, r19.xyzw);
            r14.y = dot(r4.xyzw, r21.xyzw);
            r14.z = dot(r4.xyzw, r23.xyzw);
            r5.z = dot(r4.xyzw, r24.xyzw);
            r19.xyz = r14.xyz / r5.zzz;
            r19.w = (uint)r13.y;
            r5.z = spotShadowMaps.SampleCmpLevelZero(SmplShadow_s, r19.xyw, r19.z).x;
            r14.xy = r19.xy + r10.yx;
            r14.z = r19.w;
            r6.w = spotShadowMaps.SampleCmpLevelZero(SmplShadow_s, r14.xyz, r19.z).x;
            r6.w = 0.200000003 * r6.w;
            r5.z = r5.z * 0.200000003 + r6.w;
            r14.xy = r19.xy + r10.wz;
            r6.w = spotShadowMaps.SampleCmpLevelZero(SmplShadow_s, r14.xyz, r19.z).x;
            r5.z = r6.w * 0.200000003 + r5.z;
            r14.xy = r19.xy + r10.xy;
            r6.w = spotShadowMaps.SampleCmpLevelZero(SmplShadow_s, r14.xyz, r19.z).x;
            r5.z = r6.w * 0.200000003 + r5.z;
            r14.xy = r19.xy + r10.zw;
            r6.w = spotShadowMaps.SampleCmpLevelZero(SmplShadow_s, r14.xyz, r19.z).x;
            r5.z = r6.w * 0.200000003 + r5.z;
            r5.y = r5.y * r5.z;
          }
          r5.z = dot(r12.xyz, r8.xyw);
          r5.z = max(r13.x, r5.z);
          r12.x = dynamicLights_g[r1.y].color.x;
          r12.y = dynamicLights_g[r1.y].color.y;
          r12.z = dynamicLights_g[r1.y].color.z;
          r12.xyz = r12.xyz * r5.yyy;
          r11.xyz = r12.xyz * r5.zzz + r11.xyz;
        }
      }
      r11.w = (int)r11.w + 1;
    }
    r2.xyw = r11.xyz * r14.www + r9.xyz;
  }
  r3.xyz = r3.xyz * r9.www;
  r8.xyz = r22.xyz * r2.xyw + r3.xyz;
  r0.w = cmp(0 < r17.x);
  r1.x = dot(r0.xyz, float3(0.298999995,0.587000012,0.114));
  r1.x = r1.x + -r17.x;
  r1.x = max(0, r1.x);
  r1.x = r1.x / r17.x;
  r1.x = min(1, r1.x);
  r0.w = r0.w ? r1.x : 1;
  r1.x = r16.w * r5.w;
  r0.w = r1.x * r0.w;
  r1.x = -1 + r3.w;
  r1.x = r17.y * r1.x + 1;
  r3.w = r1.x * r0.w;
  r3.xyz = r0.xyz * r3.www + r8.xyz;
  r8.w = 0;
  r0.xyzw = r20.wwww ? r3.xyzw : r8.xyzw;
  r1.x = 1 + -r5.x;
  r1.x = r17.z * r1.x;
  r1.x = mapAOIntensity_g * r1.x;
  r2.xyw = r0.xyz * mapAOColor_g.xyz + -r0.xyz;
  r0.xyz = r1.xxx * r2.xyw + r0.xyz;
  r1.x = -fogNearDistance_g + -r1.w;
  r1.x = saturate(fogFadeRangeInv_g * r1.x);
  r1.x = 1 + -r1.x;
  r1.x = log2(r1.x);
  r1.x = fogExp_g * r1.x;
  r1.x = exp2(r1.x);
  r1.x = 1 + -r1.x;
  r1.y = -fogHeight_g + r4.y;
  r1.y = saturate(fogHeightRangeInv_g * r1.y);
  r1.x = r1.x * r1.y;
  r1.x = fogIntensity_g * r1.x;
  r1.y = (int)r1.z & 16;
  r2.x = -r2.z * 0.0322580636 + 1;
  r1.z = 1 + -r2.x;
  r2.y = r18.y * r1.z + r2.x;
  r18.w = 1;
  r1.yz = r1.yy ? r2.xy : r18.wy;
  r1.x = r1.x * r15.x;
  r1.x = r1.x * r1.y;
  r2.xyz = fogColor_g.xyz + -r0.xyz;
  r0.xyz = r1.xxx * r2.xyz + r0.xyz;
  r1.x = -r1.w / skyLutCameraFarClip_g;
  r1.x = -skyLutNearOverFarClip_g + r1.x;
  r1.y = -skyLutNearOverFarClip_g + 1;
  r6.z = r1.x / r1.y;
  r1.xyw = atmosphereInscatterLUT.SampleLevel(samLinear_s, r6.xyz, 0).xyz;
  r2.xyz = atmosphereExtinctionLUT.SampleLevel(samLinear_s, r6.xyz, 0).xyz;
  r0.xyz = r0.xyz * r2.xyz + r1.xyw;
  r1.xyw = r0.xyz * r7.www + r7.xyz;
  r1.z = combineAlpha_g * r1.z;
  r1.xyw = r1.xyw + -r0.xyz;
  o0.xyz = r1.zzz * r1.xyw + r0.xyz;
  r0.y = saturate(0.100000001 * r0.w);
  r0.xzw = float3(255,255,255);
  r0.xyzw = float4(0,255,0,0) * r0.xyzw;
  o1.xyzw = (uint4)r0.xyzw;
  r0.x = 65.5350037 * r18.x;
  r0.x = (uint)r0.x;
  o2.y = min(0x0000ffff, (uint)r0.x);
  o0.w = 1;
  o2.x = 0;
  return;
}