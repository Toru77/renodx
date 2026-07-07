// ---- Created with 3Dmigoto v1.4.1 on Mon Jul  6 22:19:58 2026

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
    float userParams[2];           // Offset:  104
};

struct LightIndexData
{
    int pointLightIndices[63];     // Offset:    0
    uint pointLightCount;          // Offset:  252
    int spotLightIndices[63];      // Offset:  256
    uint spotLightCount;           // Offset:  508
    int lightProbeIndices[15];     // Offset:  512
    uint lightProbeCount;          // Offset:  572
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
  float ldotvXZ_g : packoffset(c25.w);
  float3 lightDirection_g : packoffset(c26);
  float shadowSplitDistance_g : packoffset(c26.w);
  float4x4 shadowMtx_g[2] : packoffset(c27);
  float2 invShadowSize_g : packoffset(c35);
  float shadowFadeNear_g : packoffset(c35.z);
  float shadowFadeRangeInv_g : packoffset(c35.w);
  float3 sceneShadowColor_g : packoffset(c36);
  float gameTime_g : packoffset(c36.w);
  float3 windDirection_g : packoffset(c37);
  uint collisionCount_g : packoffset(c37.w);
  float lightTileWidthInv_g : packoffset(c38);
  float lightTileHeightInv_g : packoffset(c38.y);
  float fogNearDistance_g : packoffset(c38.z);
  float fogFadeRangeInv_g : packoffset(c38.w);
  float3 fogColor_g : packoffset(c39);
  float fogIntensity_g : packoffset(c39.w);
  float fogHeight_g : packoffset(c40);
  float fogHeightRangeInv_g : packoffset(c40.y);
  float windWaveTime_g : packoffset(c40.z);
  float windWaveFrequency_g : packoffset(c40.w);
  uint localLightProbeCount_g : packoffset(c41);
  float lightSpecularGlossiness_g : packoffset(c41.y);
  float lightSpecularIntensity_g : packoffset(c41.z);
  float lightTileDepthInv_g : packoffset(c41.w);
  float4x4 ditherMtx_g : packoffset(c42);
  float4 lightProbe_g[9] : packoffset(c46);
  float4x4 farShadowMtx_g : packoffset(c55);
  float3 chrLightDir_g : packoffset(c59);
  float shadowDistance_g : packoffset(c59.w);
  float resolutionScaling_g : packoffset(c60);
  float sceneTime_g : packoffset(c60.y);
  float windForce_g : packoffset(c60.z);
  float disableMapObjNearFade_g : packoffset(c60.w);
  float4 mapColor_g : packoffset(c61);
  float4 clipPlane_g : packoffset(c62);
  float shadowZeroCascadeUVMult_g : packoffset(c63);
  float prevSceneTime_g : packoffset(c63.y);
  float prevWindWaveTime_g : packoffset(c63.z);
  uint enableMotionVectors_g : packoffset(c63.w);
  float4x4 prevView_g : packoffset(c64);
  float4x4 prevViewInv_g : packoffset(c68);
  float4x4 prevProj_g : packoffset(c72);
  float4x4 prevProjInv_g : packoffset(c76);
  float4x4 prevViewProj_g : packoffset(c80);
  float4x4 prevViewProjInv_g : packoffset(c84);
  float2 motionJitterOffset_g : packoffset(c88);
}

cbuffer ConstantBuffer : register(b6)
{
  float3 incomingLight_g : packoffset(c0);
  float3 scatteringR_g : packoffset(c1);
  float3 scatteringM_g : packoffset(c2);
  float3 extinctionR_g : packoffset(c3);
  float3 extinctionM_g : packoffset(c4);
  float3 densityScaleHeight_g : packoffset(c5);
  float3 sunDirection_g : packoffset(c6);
  float mieG_g : packoffset(c7);
  float distanceScale_g : packoffset(c7.y);
  float planetRadius_g : packoffset(c7.z);
  float atmosphereHeight_g : packoffset(c7.w);
  float sunIntensity_g : packoffset(c8);
  float volumeNearOverFarClip_g : packoffset(c8.y);
  float volumeCameraFarClip_g : packoffset(c8.z);
  float cloudCoverage_g : packoffset(c9);
  float cloudThickness_g : packoffset(c9.y);
  uint cloudRaySteps_g : packoffset(c9.z);
  float cloudLightIntensity_g : packoffset(c9.w);
  float cloudDistance_g : packoffset(c10);
  float cloudFadeRangeInv_g : packoffset(c10.y);
  float cloudHeight_g : packoffset(c10.z);
  float3 cloudColor_g : packoffset(c11);
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
  float metalness_g : packoffset(c7);
  float roughness_g : packoffset(c7.y);
  uint materialID_g : packoffset(c7.z);
}

#include "../../shared.h"

SamplerState Smpl0_s : register(s0);
SamplerState Smpl6_s : register(s6);
SamplerState SmplCube_s : register(s14);
SamplerState SmplLinearClamp_s : register(s15);
SamplerComparisonState SmplShadow_s : register(s13);
Texture2D<float4> Tex0 : register(t0);
Texture2D<float4> Tex6 : register(t6);
Texture2D<float4> shadowMap : register(t16);
TextureCube<float4> texEnvMap_g : register(t17);
StructuredBuffer<LightParam> dynamicLights_g : register(t18);
StructuredBuffer<LightIndexData> lightIndices_g : register(t19);
Texture3D<float4> atmosphereInscatterLUT : register(t22);
Texture3D<float4> atmosphereExtinctionLUT : register(t23);
Texture2DArray<float4> spotShadowMaps : register(t24);
StructuredBuffer<float4x4> spotShadowMatrices_g : register(t25);


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_Position0,
  float4 v1 : NORMAL0,
  float4 v2 : TANGENT0,
  float4 v3 : BINORMAL0,
  float4 v4 : TEXCOORD0,
  float4 v5 : TEXCOORD1,
  float4 v6 : TEXCOORD2,
  linear centroid float4 v7 : TEXCOORD3,
  float4 v8 : TEXCOORD4,
  float4 v9 : TEXCOORD5,
  out float4 o0 : SV_Target0,
  out float4 o1 : SV_Target1,
  out float2 o2 : SV_Target2)
{
  const float4 icb[] = { { -0.840520, -0.073954, 0, 0},
                              { -0.326235, -0.405830, 0, 0},
                              { -0.698464, 0.457259, 0, 0},
                              { -0.203356, 0.620585, 0, 0},
                              { 0.963450, -0.194353, 0, 0},
                              { 0.473434, -0.480026, 0, 0},
                              { 0.519454, 0.767034, 0, 0},
                              { 0.185461, -0.894523, 0, 0},
                              { 0.507351, 0.064963, 0, 0},
                              { -0.321932, 0.595435, 0, 0} };
  float4 r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12,r13,r14;

  r0.xy = v9.xy / v9.ww;
  r0.xy = r0.xy * float2(0.5,-0.5) + float2(0.5,0.5);
  r0.xy = r0.xy * vpSize_g.xy + -v0.xy;
  o2.xy = -motionJitterOffset_g.xy + r0.xy;
  r0.x = viewInv_g._m30 + -v8.x;
  r0.y = viewInv_g._m31 + -v8.y;
  r0.z = viewInv_g._m32 + -v8.z;
  r0.w = dot(r0.xyz, r0.xyz);
  r1.x = sqrt(r0.w);
  r0.w = rsqrt(r0.w);
  r1.yzw = r0.xyz * r0.www;
  r1.x = -v1.w + r1.x;
  r1.x = v2.w * r1.x;
  r1.x = min(1, r1.x);
  r1.x = max(disableMapObjNearFade_g, r1.x);
  r2.x = dot(v1.xyz, v1.xyz);
  r2.x = rsqrt(r2.x);
  r2.xyz = v1.xyz * r2.xxx;
  r3.xy = v4.xy * float2(1,-1) + float2(0,1);
  r4.xyzw = Tex0.Sample(Smpl0_s, r3.xy).xyzw;
  r3.xyzw = Tex6.Sample(Smpl6_s, r3.xy).xyzw;
  r4.xyzw = v7.xyzw * r4.xyzw;
  // ── Cubemap improvement: dehaze base glass texture ──
  {
    float cbEn = saturate(shader_injection_data.cubemap_improvements_enabled);
    float3 origTex = r4.xyz;
    float3 dehazed = saturate((r4.xyz - 0.5f) * 1.35f + 0.5f);
    float dehLum = dot(dehazed, float3(0.298999995f, 0.587000012f, 0.114f));
    dehazed = lerp(dehLum.xxx, dehazed, 0.1f);
    dehazed = pow(max(dehazed, 1e-4f), 2);
    r4.xyz = lerp(origTex, dehazed, cbEn);
  }
  r2.w = emissive_g * r3.w;
  r3.w = dot(view_g._m02_m12_m22_m32, v8.xyzw);
  r5.xyz = lightProbe_g[1].xyz * r2.xxx + lightProbe_g[0].xyz;
  r5.xyz = lightProbe_g[2].xyz * r2.yyy + r5.xyz;
  r5.xyz = lightProbe_g[3].xyz * r2.zzz + r5.xyz;
  r6.xyz = lightProbe_g[4].xyz * r2.zzz;
  r5.xyz = r6.xyz * r2.xxx + r5.xyz;
  r6.xyz = lightProbe_g[5].xyz * r2.yyy;
  r5.xyz = r6.xyz * r2.zzz + r5.xyz;
  r6.xyz = lightProbe_g[6].xyz * r2.yyy;
  r5.xyz = r6.xyz * r2.xxx + r5.xyz;
  r6.xy = r2.zy * r2.zy;
  r5.w = r6.x * 3 + -1;
  r5.xyz = lightProbe_g[7].xyz * r5.www + r5.xyz;
  r5.w = r2.x * r2.x + -r6.y;
  r5.xyz = lightProbe_g[8].xyz * r5.www + r5.xyz;
  r6.xyz = sceneShadowColor_g.xyz + shadowColor_g.xyz;
  r6.xyz = min(float3(1,1,1), r6.xyz);
  r5.w = dot(r2.xyz, -lightDirection_g.xyz);
  r6.w = dot(r2.xyz, r1.yzw);
  r7.x = cmp(shadowDistance_g >= -r3.w);
  if (r7.x != 0) {
    r7.x = cmp(-r3.w < shadowSplitDistance_g);
    r8.xyzw = r7.xxxx ? shadowMtx_g[0]._m00_m10_m20_m30 : shadowMtx_g[1]._m00_m10_m20_m30;
    r9.xyzw = r7.xxxx ? shadowMtx_g[0]._m01_m11_m21_m31 : shadowMtx_g[1]._m01_m11_m21_m31;
    r10.xyzw = r7.xxxx ? shadowMtx_g[0]._m02_m12_m22_m32 : shadowMtx_g[1]._m02_m12_m22_m32;
    r7.xyzw = r7.xxxx ? shadowMtx_g[0]._m03_m13_m23_m33 : shadowMtx_g[1]._m03_m13_m23_m33;
    r8.x = dot(v8.xyzw, r8.xyzw);
    r8.y = dot(v8.xyzw, r9.xyzw);
    r8.z = dot(v8.xyzw, r10.xyzw);
    r7.x = dot(v8.xyzw, r7.xyzw);
    r7.xyz = r8.xyz / r7.xxx;
    r7.w = 0;
    r8.x = 0;
    while (true) {
      r8.y = cmp((int)r8.x >= 10);
      if (r8.y != 0) break;
      r8.yz = icb[r8.x+0].xy * invShadowSize_g.xy;
      r8.yz = saturate(r8.yz * float2(1.5,1.5) + r7.xy);
      r8.y = shadowMap.SampleCmpLevelZero(SmplShadow_s, r8.yz, r7.z).x;
      r7.w = r8.y * 0.100000001 + r7.w;
      r8.x = (int)r8.x + 1;
    }
    r7.x = -shadowFadeNear_g + -r3.w;
    r7.x = saturate(shadowFadeRangeInv_g * r7.x);
    r7.y = 1 + -r7.w;
    r7.x = r7.x * r7.y + r7.w;
  } else {
    r7.x = 1;
  }
  r7.y = -1 + r7.x;
  r7.y = specularShadowFadeRatio_g * r7.y + 1;
  r7.z = 1 + -r5.w;
  r7.z = translucency_g * r7.z + r5.w;
  r7.z = r7.z * 0.5 + 0.5;
  r7.w = r7.z * r7.z;
  r7.z = -r7.z * r7.w + 1;
  r7.x = r7.z * r7.z + r7.x;
  r7.x = min(1, r7.x);
  r8.xyz = r0.xyz * r0.www + -lightDirection_g.xyz;
  r7.z = dot(r8.xyz, r8.xyz);
  r7.z = rsqrt(r7.z);
  r8.xyz = r8.xyz * r7.zzz;
  r7.z = lightSpecularGlossiness_g * specularGlossiness0_g;
  r7.w = saturate(dot(r8.xyz, r2.xyz));
  r7.z = max(0.00100000005, r7.z);
  r7.w = log2(r7.w);
  r7.z = r7.z * r7.w;
  r7.z = exp2(r7.z);
  r7.y = r7.z * r7.y;
  r7.y = lightSpecularIntensity_g * r7.y;
  r7.yzw = specularColor_g.xyz * r7.yyy;
  r8.xy = r5.ww * float2(0.5,-0.5) + float2(0.5,0.5);
  r5.w = max(r8.x, r8.y);
  r5.w = r5.w + -r8.x;
  r5.w = translucency_g * r5.w + r8.x;
  r8.xy = r3.xy * float2(metalness_g, roughness_g);
  r3.x = r6.w + r6.w;
  r1.yzw = r2.xyz * -r3.xxx + r1.yzw;
  {
    uint cubeDim;
    texEnvMap_g.GetDimensions(0, cubeDim, cubeDim, cubeDim);
    r3.x = (float)(cubeDim - 1);
  }
  r1.yzw = float3(r1.y, -r1.z, -r1.w);
  r3.x = r8.y * r3.x;
  r1.yzw = texEnvMap_g.SampleLevel(SmplCube_s, r1.yzw, r3.x).xyz;
  r1.yzw *= lerp(1.0, 0.25, saturate(shader_injection_data.cubemap_improvements_enabled));
  r3.x = cmp(0 < fresnel0_g);
  r6.w = 1 + -abs(r6.w);
  r6.w = max(0, r6.w);
  r8.z = log2(r6.w);
  r8.z = fresnel0_g * r8.z;
  r8.z = exp2(r8.z);
  r3.x = r3.x ? r8.z : 1;
  r8.z = r8.x * r3.x;
  r9.xyz = r4.xyz * r1.yzw + -r4.xyz;
  r4.xyz = r8.zzz * r9.xyz + r4.xyz;
  r8.z = dot(r1.yzw, float3(0.298999995,0.587000012,0.114));
  r8.y = r8.y * -9 + 10;
  r8.z = log2(r8.z);
  r8.y = r8.y * r8.z;
  r8.y = exp2(r8.y);
  r8.z = 1 + -r8.y;
  r8.x = r8.x * r8.z + r8.y;
  r1.yzw = r5.www * r1.yzw;
  r1.yzw = r1.yzw * r8.xxx;
  r1.yzw = r1.yzw * r3.xxx;
  r3.y = -r3.y * roughness_g + 1;
  r1.yzw = r3.yyy * r1.yzw;
  r1.yzw = r7.yzw * lightColor_g.xyz + r1.yzw;
  r3.y = r5.w * r7.x;
  r7.xyz = float3(1,1,1) + -r6.xyz;
  r6.xyz = r3.yyy * r7.xyz + r6.xyz;
  r5.xyz = r6.xyz * lightColor_g.xyz + r5.xyz;
  r3.y = min(1, r2.w);
  r6.xyz = float3(1,1,1) + -r5.xyz;
  r5.xyz = r3.yyy * r6.xyz + r5.xyz;
  r3.y = rimIntensity_g * r6.w;
  r3.y = log2(r3.y);
  r3.y = rimLightPower_g * r3.y;
  r3.y = exp2(r3.y);
  r3.y = min(1, r3.y);
  r1.yzw = rimLightColor_g.xyz * r3.yyy + r1.yzw;
  r6.xy = v0.xy * float2(lightTileWidthInv_g, lightTileHeightInv_g);
  r3.y = lightTileDepthInv_g * -r3.w;
  r3.y = min(7, r3.y);
  r3.y = max(0, r3.y);
  r6.xy = (uint2)r6.xy;
  r3.y = (uint)r3.y;
  r5.w = (uint)r6.y << 5;
  r3.y = mad((int)r3.y, 576, (int)r5.w);
  r3.y = (int)r6.x + (int)r3.y;
  r3.y = min(4607, (uint)r3.y);
  r5.w = lightIndices_g[r3.y].pointLightCount;
  r5.w = min(63, (uint)r5.w);
  r7.xyz = float3(0,0,0);
  r6.xyzw = float4(0,0,0,0);
  while (true) {
    r7.w = cmp((uint)r6.w >= (uint)r5.w);
    if (r7.w != 0) break;
    r7.w = lightIndices_g[r3.y].pointLightIndices[(int)r6.w];
    r8.x = dynamicLights_g[r7.w].pos.x;
    r8.y = dynamicLights_g[r7.w].pos.y;
    r8.z = dynamicLights_g[r7.w].pos.z;
    r8.xyz = -v8.xyz + r8.xyz;
    r8.w = dot(r8.xyz, r8.xyz);
    r9.x = sqrt(r8.w);
    r9.y = dynamicLights_g[r7.w].radiusInv;
    r9.x = r9.x * r9.y;
    r9.y = dynamicLights_g[r7.w].attenuation;
    r9.x = log2(abs(r9.x));
    r9.x = r9.y * r9.x;
    r9.x = exp2(r9.x);
    r9.x = 1 + -r9.x;
    r9.x = max(0, r9.x);
    r9.y = cmp(0 < r9.x);
    if (r9.y != 0) {
      r8.w = rsqrt(r8.w);
      r8.xyz = r8.xyz * r8.www;
      r8.w = dynamicLights_g[r7.w].translucency;
      r9.y = dot(r8.xyz, r2.xyz);
      r8.w = max(r9.y, r8.w);
      r8.w = r9.x * r8.w;
      r9.x = dynamicLights_g[r7.w].color.x;
      r9.y = dynamicLights_g[r7.w].color.y;
      r9.z = dynamicLights_g[r7.w].color.z;
      r7.xyz = r9.xyz * r8.www + r7.xyz;
      r8.xyz = r0.xyz * r0.www + r8.xyz;
      r9.w = dot(r8.xyz, r8.xyz);
      r9.w = rsqrt(r9.w);
      r8.xyz = r9.www * r8.xyz;
      r10.x = dynamicLights_g[r7.w].specularIntensity;
      r10.y = dynamicLights_g[r7.w].specularGlossiness;
      r7.w = specularGlossiness0_g * r10.y;
      r8.x = saturate(dot(r8.xyz, r2.xyz));
      r7.w = max(0.00100000005, r7.w);
      r8.x = log2(r8.x);
      r7.w = r8.x * r7.w;
      r7.w = exp2(r7.w);
      r8.xyz = r9.xyz * r7.www;
      r8.xyz = r8.xyz * r8.www;
      r6.xyz = r8.xyz * r10.xxx + r6.xyz;
    }
    r6.w = (int)r6.w + 1;
  }
  r5.xyz = r7.xyz * dynamicLightIntensity_g + r5.xyz;
  r5.w = lightIndices_g[r3.y].spotLightCount;
  r5.w = min(63, (uint)r5.w);
  r7.xyz = r6.xyz;
  r8.xyz = float3(0,0,0);
  r6.w = 0;
  while (true) {
    r7.w = cmp((uint)r6.w >= (uint)r5.w);
    if (r7.w != 0) break;
    r7.w = lightIndices_g[r3.y].spotLightIndices[(int)r6.w];
    r9.x = dynamicLights_g[r7.w].pos.x;
    r9.y = dynamicLights_g[r7.w].pos.y;
    r9.z = dynamicLights_g[r7.w].pos.z;
    r9.xyz = -v8.xyz + r9.xyz;
    r8.w = dot(r9.xyz, r9.xyz);
    r9.w = rsqrt(r8.w);
    r9.xyz = r9.xyz * r9.www;
    r10.x = dynamicLights_g[r7.w].vec.x;
    r10.y = dynamicLights_g[r7.w].vec.y;
    r10.z = dynamicLights_g[r7.w].vec.z;
    r10.w = dynamicLights_g[r7.w].spotAngleInv;
    r9.w = dot(r9.xyz, r10.xyz);
    r9.w = max(0, r9.w);
    r9.w = 1 + -r9.w;
    r9.w = r9.w * r10.w;
    r10.x = dynamicLights_g[r7.w].attenuationAngle;
    r9.w = log2(r9.w);
    r9.w = r10.x * r9.w;
    r9.w = exp2(r9.w);
    r9.w = 1 + -r9.w;
    r9.w = max(0, r9.w);
    r10.x = cmp(0 < r9.w);
    if (r10.x != 0) {
      r8.w = sqrt(r8.w);
      r10.x = dynamicLights_g[r7.w].radiusInv;
      r8.w = r10.x * r8.w;
      r10.x = dynamicLights_g[r7.w].attenuation;
      r8.w = log2(abs(r8.w));
      r8.w = r10.x * r8.w;
      r8.w = exp2(r8.w);
      r8.w = 1 + -r8.w;
      r8.w = max(0, r8.w);
      r8.w = r9.w * r8.w;
      r9.w = cmp(0 < r8.w);
      if (r9.w != 0) {
        r10.x = dynamicLights_g[r7.w].translucency;
        r10.y = dynamicLights_g[r7.w].shadowmapIndex;
        r9.w = cmp((int)r10.y != -1);
        if (r9.w != 0) {
          r11.xyzw = spotShadowMatrices_g[r10.y]._m00_m10_m20_m30;
          r12.xyzw = spotShadowMatrices_g[r10.y]._m01_m11_m21_m31;
          r13.xyzw = spotShadowMatrices_g[r10.y]._m02_m12_m22_m32;
          r14.xyzw = spotShadowMatrices_g[r10.y]._m03_m13_m23_m33;
          r11.x = dot(v8.xyzw, r11.xyzw);
          r11.y = dot(v8.xyzw, r12.xyzw);
          r11.z = dot(v8.xyzw, r13.xyzw);
          r9.w = dot(v8.xyzw, r14.xyzw);
          r11.xyz = r11.xyz / r9.www;
          r11.w = (int)r10.y;
          r9.w = spotShadowMaps.SampleCmpLevelZero(SmplShadow_s, r11.xyw, r11.z).x;
          r8.w = r9.w * r8.w;
        }
        r9.w = dot(r9.xyz, r2.xyz);
        r9.w = max(r10.x, r9.w);
        r8.w = r9.w * r8.w;
        r10.x = dynamicLights_g[r7.w].color.x;
        r10.y = dynamicLights_g[r7.w].color.y;
        r10.z = dynamicLights_g[r7.w].color.z;
        r8.xyz = r10.xyz * r8.www + r8.xyz;
        r9.xyz = r0.xyz * r0.www + r9.xyz;
        r9.w = dot(r9.xyz, r9.xyz);
        r9.w = rsqrt(r9.w);
        r9.xyz = r9.xyz * r9.www;
        r11.x = dynamicLights_g[r7.w].specularIntensity;
        r11.y = dynamicLights_g[r7.w].specularGlossiness;
        r7.w = specularGlossiness0_g * r11.y;
        r9.x = saturate(dot(r9.xyz, r2.xyz));
        r7.w = max(0.00100000005, r7.w);
        r9.x = log2(r9.x);
        r7.w = r9.x * r7.w;
        r7.w = exp2(r7.w);
        r9.xyz = r10.xyz * r7.www;
        r9.xyz = r9.xyz * r8.www;
        r7.xyz = r9.xyz * r11.xxx + r7.xyz;
      }
    }
    r6.w = (int)r6.w + 1;
  }
  r0.xyz = r8.xyz * dynamicLightIntensity_g + r5.xyz;
  r1.yzw = r7.xyz * dynamicLightIntensity_g + r1.yzw;
  r5.xyz = r1.yzw * r3.zzz;
  r0.xyz = r4.xyz * r0.xyz + r5.xyz;
  r0.w = dot(r1.yzw, float3(0.298999995,0.587000012,0.114));
  r0.w = min(1, r0.w);
  r1.y = r4.w * r3.x;
  r1.z = -r4.w * r3.x + v5.w;
  r0.w = r0.w * r1.z + r1.y;
  r1.y = max(1, r2.w);
  r3.xyz = r1.yyy * r0.xyz;
  r0.w = r0.w * r1.x;
  r1.x = materialFogIntensity_g * v5.z;
  r0.xyz = -r0.xyz * r1.yyy + fogColor_g.xyz;
  r0.xyz = r1.xxx * r0.xyz + r3.xyz;
  r1.x = -r3.w / volumeCameraFarClip_g;
  r3.xy = invVPSize_g.xy * v0.xy;
  r1.x = -volumeNearOverFarClip_g + r1.x;
  r1.y = -volumeNearOverFarClip_g + 1;
  r3.z = r1.x / r1.y;
  r1.xyz = atmosphereInscatterLUT.SampleLevel(SmplLinearClamp_s, r3.xyz, 0).xyz;
  r3.xyz = atmosphereExtinctionLUT.SampleLevel(SmplLinearClamp_s, r3.xyz, 0).xyz;
  r1.xyz = r3.xyz + r1.xyz;
  r0.xyz = r1.xyz * r0.xyz;
  o0.xyz = mapColor_g.xyz * r0.xyz;
  r0.x = 1 + -v5.z;
  r0.x = ssaoIntensity_g * r0.x;
  r0.x = r0.x * r0.w;
  r1.xyz = r2.xyz * float3(0.5,0.5,0.5) + float3(0.5,0.5,0.5);
  r1.xy = saturate(r1.xy);
  r0.yz = float2(65535,65535) * r1.xy;
  r0.yz = (uint2)r0.yz;
  o1.x = mad((int)r0.z, 0x00010000, (int)r0.y);
  r0.y = 65535 * r1.z;
  o1.y = (uint)r0.y;
  r0.x = 255 * r0.x;
  r0.x = (uint)r0.x;
  r0.yz = (uint2)r0.xx << int2(8,16);
  r0.x = (int)r0.y | (int)r0.x;
  o1.z = (int)r0.z | (int)r0.x;
  o0.w = r0.w;
  o1.w = 0;
  return;
}