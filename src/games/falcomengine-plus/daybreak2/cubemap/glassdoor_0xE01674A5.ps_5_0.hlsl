// ---- Created with 3Dmigoto v1.4.1 on Mon Jul  6 23:20:31 2026

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
SamplerState SmplCube_s : register(s14);
SamplerState SmplLinearClamp_s : register(s15);
SamplerComparisonState SmplShadow_s : register(s13);
Texture2D<float4> Tex0 : register(t0);
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
  float4 r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12,r13;

  r0.x = dot(v8.xyz, clipPlane_g.xyz);
  r0.x = -clipPlane_g.w + r0.x;
  r0.x = cmp(r0.x < 0);
  if (r0.x != 0) discard;
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
  r3.xyzw = Tex0.Sample(Smpl0_s, r3.xy).xyzw;
  r3.xyzw = v7.xyzw * r3.xyzw;
  r2.w = dot(view_g._m02_m12_m22_m32, v8.xyzw);
  r4.xyz = lightProbe_g[1].xyz * r2.xxx + lightProbe_g[0].xyz;
  r4.xyz = lightProbe_g[2].xyz * r2.yyy + r4.xyz;
  r4.xyz = lightProbe_g[3].xyz * r2.zzz + r4.xyz;
  r5.xyz = lightProbe_g[4].xyz * r2.zzz;
  r4.xyz = r5.xyz * r2.xxx + r4.xyz;
  r5.xyz = lightProbe_g[5].xyz * r2.yyy;
  r4.xyz = r5.xyz * r2.zzz + r4.xyz;
  r5.xyz = lightProbe_g[6].xyz * r2.yyy;
  r4.xyz = r5.xyz * r2.xxx + r4.xyz;
  r5.xy = r2.zy * r2.zy;
  r4.w = r5.x * 3 + -1;
  r4.xyz = lightProbe_g[7].xyz * r4.www + r4.xyz;
  r4.w = r2.x * r2.x + -r5.y;
  r4.xyz = lightProbe_g[8].xyz * r4.www + r4.xyz;
  r5.xyz = sceneShadowColor_g.xyz + shadowColor_g.xyz;
  r5.xyz = min(float3(1,1,1), r5.xyz);
  r4.w = dot(r2.xyz, -lightDirection_g.xyz);
  r5.w = dot(r2.xyz, r1.yzw);
  r6.x = cmp(shadowDistance_g >= -r2.w);
  if (r6.x != 0) {
    r6.x = cmp(-r2.w < shadowSplitDistance_g);
    r7.xyzw = r6.xxxx ? shadowMtx_g[0]._m00_m10_m20_m30 : shadowMtx_g[1]._m00_m10_m20_m30;
    r8.xyzw = r6.xxxx ? shadowMtx_g[0]._m01_m11_m21_m31 : shadowMtx_g[1]._m01_m11_m21_m31;
    r9.xyzw = r6.xxxx ? shadowMtx_g[0]._m02_m12_m22_m32 : shadowMtx_g[1]._m02_m12_m22_m32;
    r6.xyzw = r6.xxxx ? shadowMtx_g[0]._m03_m13_m23_m33 : shadowMtx_g[1]._m03_m13_m23_m33;
    r7.x = dot(v8.xyzw, r7.xyzw);
    r7.y = dot(v8.xyzw, r8.xyzw);
    r7.z = dot(v8.xyzw, r9.xyzw);
    r6.x = dot(v8.xyzw, r6.xyzw);
    r6.xyz = r7.xyz / r6.xxx;
    r6.w = 0;
    r7.x = 0;
    while (true) {
      r7.y = cmp((int)r7.x >= 10);
      if (r7.y != 0) break;
      r7.yz = icb[r7.x+0].xy * invShadowSize_g.xy;
      r7.yz = saturate(r7.yz * float2(1.5,1.5) + r6.xy);
      r7.y = shadowMap.SampleCmpLevelZero(SmplShadow_s, r7.yz, r6.z).x;
      r6.w = r7.y * 0.100000001 + r6.w;
      r7.x = (int)r7.x + 1;
    }
    r6.x = -shadowFadeNear_g + -r2.w;
    r6.x = saturate(shadowFadeRangeInv_g * r6.x);
    r6.y = 1 + -r6.w;
    r6.x = r6.x * r6.y + r6.w;
  } else {
    r6.x = 1;
  }
  r6.y = -1 + r6.x;
  r6.y = specularShadowFadeRatio_g * r6.y + 1;
  r6.z = 1 + -r4.w;
  r6.z = translucency_g * r6.z + r4.w;
  r6.z = r6.z * 0.5 + 0.5;
  r6.w = r6.z * r6.z;
  r6.z = -r6.z * r6.w + 1;
  r6.x = r6.z * r6.z + r6.x;
  r6.x = min(1, r6.x);
  r7.xyz = r0.xyz * r0.www + -lightDirection_g.xyz;
  r6.z = dot(r7.xyz, r7.xyz);
  r6.z = rsqrt(r6.z);
  r7.xyz = r7.xyz * r6.zzz;
  r6.z = lightSpecularGlossiness_g * specularGlossiness0_g;
  r6.w = saturate(dot(r7.xyz, r2.xyz));
  r6.z = max(0.00100000005, r6.z);
  r6.w = log2(r6.w);
  r6.z = r6.z * r6.w;
  r6.z = exp2(r6.z);
  r6.y = r6.z * r6.y;
  r6.y = lightSpecularIntensity_g * r6.y;
  r6.yzw = specularColor_g.xyz * r6.yyy;
  r7.xy = r4.ww * float2(0.5,-0.5) + float2(0.5,0.5);
  r4.w = max(r7.x, r7.y);
  r4.w = r4.w + -r7.x;
  r4.w = translucency_g * r4.w + r7.x;
  r7.x = r5.w + r5.w;
  r1.yzw = r2.xyz * -r7.xxx + r1.yzw;
  {
    uint cubeDim, mipCount;
    texEnvMap_g.GetDimensions(0, cubeDim, cubeDim, mipCount);
    r7.x = (float)(cubeDim - 1);
  }
  r1.yzw = float3(r1.y, -r1.z, -r1.w);
  r7.x = roughness_g * r7.x;
  r1.yzw = texEnvMap_g.SampleLevel(SmplCube_s, r1.yzw, r7.x).xyz;
  // ── Cubemap intensity modulation ──
  r1.yzw *= lerp(1.0f, 0.25f, saturate(shader_injection_data.cubemap_improvements_enabled));
  r7.x = cmp(0 < fresnel0_g);
  r5.w = 1 + -abs(r5.w);
  r5.w = max(0, r5.w);
  r7.y = log2(r5.w);
  r7.y = fresnel0_g * r7.y;
  r7.y = exp2(r7.y);
  r7.x = r7.x ? r7.y : 1;
  r7.y = metalness_g * r7.x;
  r8.xyz = r3.xyz * r1.yzw + -r3.xyz;
  r3.xyz = r7.yyy * r8.xyz + r3.xyz;
  r7.y = dot(r1.yzw, float3(0.298999995,0.587000012,0.114));
  r7.z = roughness_g * -9 + 10;
  r7.y = log2(r7.y);
  r7.y = r7.z * r7.y;
  r7.y = exp2(r7.y);
  r7.z = 1 + -r7.y;
  r7.y = metalness_g * r7.z + r7.y;
  r1.yzw = r4.www * r1.yzw;
  r1.yzw = r1.yzw * r7.yyy;
  r1.yzw = r1.yzw * r7.xxx;
  r7.y = 1 + -roughness_g;
  r1.yzw = r7.yyy * r1.yzw;
  r1.yzw = r6.yzw * lightColor_g.xyz + r1.yzw;
  r4.w = r4.w * r6.x;
  r6.xyz = float3(1,1,1) + -r5.xyz;
  r5.xyz = r4.www * r6.xyz + r5.xyz;
  r4.xyz = r5.xyz * lightColor_g.xyz + r4.xyz;
  r4.w = min(1, emissive_g);
  r5.xyz = float3(1,1,1) + -r4.xyz;
  r4.xyz = r4.www * r5.xyz + r4.xyz;
  r4.w = rimIntensity_g * r5.w;
  r4.w = log2(r4.w);
  r4.w = rimLightPower_g * r4.w;
  r4.w = exp2(r4.w);
  r4.w = min(1, r4.w);
  r1.yzw = rimLightColor_g.xyz * r4.www + r1.yzw;
  r5.xy = v0.xy * float2(lightTileWidthInv_g, lightTileHeightInv_g);
  r4.w = lightTileDepthInv_g * -r2.w;
  r4.w = min(7, r4.w);
  r4.w = max(0, r4.w);
  r5.xy = (uint2)r5.xy;
  r4.w = (uint)r4.w;
  r5.y = (uint)r5.y << 5;
  r4.w = mad((int)r4.w, 576, (int)r5.y);
  r4.w = (int)r5.x + (int)r4.w;
  r4.w = min(4607, (uint)r4.w);
  r5.x = lightIndices_g[r4.w].pointLightCount;
  r5.x = min(63, (uint)r5.x);
  r5.yzw = float3(0,0,0);
  r6.xyzw = float4(0,0,0,0);
  while (true) {
    r7.y = cmp((uint)r6.w >= (uint)r5.x);
    if (r7.y != 0) break;
    r7.y = lightIndices_g[r4.w].pointLightIndices[(int)r6.w];
    r8.x = dynamicLights_g[r7.y].pos.x;
    r8.y = dynamicLights_g[r7.y].pos.y;
    r8.z = dynamicLights_g[r7.y].pos.z;
    r8.xyz = -v8.xyz + r8.xyz;
    r7.z = dot(r8.xyz, r8.xyz);
    r7.w = sqrt(r7.z);
    r8.w = dynamicLights_g[r7.y].radiusInv;
    r7.w = r8.w * r7.w;
    r8.w = dynamicLights_g[r7.y].attenuation;
    r7.w = log2(abs(r7.w));
    r7.w = r8.w * r7.w;
    r7.w = exp2(r7.w);
    r7.w = 1 + -r7.w;
    r7.w = max(0, r7.w);
    r8.w = cmp(0 < r7.w);
    if (r8.w != 0) {
      r7.z = rsqrt(r7.z);
      r8.xyz = r8.xyz * r7.zzz;
      r7.z = dynamicLights_g[r7.y].translucency;
      r8.w = dot(r8.xyz, r2.xyz);
      r7.z = max(r8.w, r7.z);
      r7.z = r7.w * r7.z;
      r9.x = dynamicLights_g[r7.y].color.x;
      r9.y = dynamicLights_g[r7.y].color.y;
      r9.z = dynamicLights_g[r7.y].color.z;
      r6.xyz = r9.xyz * r7.zzz + r6.xyz;
      r8.xyz = r0.xyz * r0.www + r8.xyz;
      r7.w = dot(r8.xyz, r8.xyz);
      r7.w = rsqrt(r7.w);
      r8.xyz = r8.xyz * r7.www;
      r7.y = dynamicLights_g[r7.y].specularIntensity;
      r7.w = dynamicLights_g[r7.y].specularGlossiness;
      r7.w = specularGlossiness0_g * r7.w;
      r8.x = saturate(dot(r8.xyz, r2.xyz));
      r7.w = max(0.00100000005, r7.w);
      r8.x = log2(r8.x);
      r7.w = r8.x * r7.w;
      r7.w = exp2(r7.w);
      r8.xyz = r9.xyz * r7.www;
      r8.xyz = r8.xyz * r7.zzz;
      r5.yzw = r8.xyz * r7.yyy + r5.yzw;
    }
    r6.w = (int)r6.w + 1;
  }
  r4.xyz = r6.xyz * dynamicLightIntensity_g + r4.xyz;
  r5.x = lightIndices_g[r4.w].spotLightCount;
  r5.x = min(63, (uint)r5.x);
  r6.xyz = r5.yzw;
  r7.yzw = float3(0,0,0);
  r6.w = 0;
  while (true) {
    r8.x = cmp((uint)r6.w >= (uint)r5.x);
    if (r8.x != 0) break;
    r8.x = lightIndices_g[r4.w].spotLightIndices[(int)r6.w];
    r8.y = dynamicLights_g[r8.x].pos.x;
    r8.z = dynamicLights_g[r8.x].pos.y;
    r8.w = dynamicLights_g[r8.x].pos.z;
    r8.yzw = -v8.xyz + r8.yzw;
    r9.x = dot(r8.yzw, r8.yzw);
    r9.y = rsqrt(r9.x);
    r8.yzw = r9.yyy * r8.yzw;
    r10.x = dynamicLights_g[r8.x].vec.x;
    r10.y = dynamicLights_g[r8.x].vec.y;
    r10.z = dynamicLights_g[r8.x].vec.z;
    r10.w = dynamicLights_g[r8.x].spotAngleInv;
    r9.y = dot(r8.yzw, r10.xyz);
    r9.y = max(0, r9.y);
    r9.y = 1 + -r9.y;
    r9.y = r9.y * r10.w;
    r9.z = dynamicLights_g[r8.x].attenuationAngle;
    r9.y = log2(r9.y);
    r9.y = r9.z * r9.y;
    r9.y = exp2(r9.y);
    r9.y = 1 + -r9.y;
    r9.y = max(0, r9.y);
    r9.z = cmp(0 < r9.y);
    if (r9.z != 0) {
      r9.x = sqrt(r9.x);
      r9.z = dynamicLights_g[r8.x].radiusInv;
      r9.x = r9.x * r9.z;
      r9.z = dynamicLights_g[r8.x].attenuation;
      r9.x = log2(abs(r9.x));
      r9.x = r9.z * r9.x;
      r9.x = exp2(r9.x);
      r9.x = 1 + -r9.x;
      r9.x = max(0, r9.x);
      r9.x = r9.y * r9.x;
      r9.y = cmp(0 < r9.x);
      if (r9.y != 0) {
        r9.y = dynamicLights_g[r8.x].translucency;
        r9.z = dynamicLights_g[r8.x].shadowmapIndex;
        r9.w = cmp((int)r9.z != -1);
        if (r9.w != 0) {
          r10.xyzw = spotShadowMatrices_g[r9.z]._m00_m10_m20_m30;
          r11.xyzw = spotShadowMatrices_g[r9.z]._m01_m11_m21_m31;
          r12.xyzw = spotShadowMatrices_g[r9.z]._m02_m12_m22_m32;
          r13.xyzw = spotShadowMatrices_g[r9.z]._m03_m13_m23_m33;
          r10.x = dot(v8.xyzw, r10.xyzw);
          r10.y = dot(v8.xyzw, r11.xyzw);
          r10.z = dot(v8.xyzw, r12.xyzw);
          r9.w = dot(v8.xyzw, r13.xyzw);
          r10.xyz = r10.xyz / r9.www;
          r10.w = (int)r9.z;
          r9.z = spotShadowMaps.SampleCmpLevelZero(SmplShadow_s, r10.xyw, r10.z).x;
          r9.x = r9.x * r9.z;
        }
        r9.z = dot(r8.yzw, r2.xyz);
        r9.y = max(r9.y, r9.z);
        r9.x = r9.x * r9.y;
        r9.y = dynamicLights_g[r8.x].color.x;
        r9.z = dynamicLights_g[r8.x].color.y;
        r9.w = dynamicLights_g[r8.x].color.z;
        r7.yzw = r9.yzw * r9.xxx + r7.yzw;
        r8.yzw = r0.xyz * r0.www + r8.yzw;
        r10.x = dot(r8.yzw, r8.yzw);
        r10.x = rsqrt(r10.x);
        r8.yzw = r10.xxx * r8.yzw;
        r10.x = dynamicLights_g[r8.x].specularIntensity;
        r10.y = dynamicLights_g[r8.x].specularGlossiness;
        r8.x = specularGlossiness0_g * r10.y;
        r8.y = saturate(dot(r8.yzw, r2.xyz));
        r8.x = max(0.00100000005, r8.x);
        r8.y = log2(r8.y);
        r8.x = r8.x * r8.y;
        r8.x = exp2(r8.x);
        r8.xyz = r9.yzw * r8.xxx;
        r8.xyz = r8.xyz * r9.xxx;
        r6.xyz = r8.xyz * r10.xxx + r6.xyz;
      }
    }
    r6.w = (int)r6.w + 1;
  }
  r0.xyz = r7.yzw * dynamicLightIntensity_g + r4.xyz;
  r1.yzw = r6.xyz * dynamicLightIntensity_g + r1.yzw;
  r0.xyz = r3.xyz * r0.xyz + r1.yzw;
  r0.w = dot(r1.yzw, float3(0.298999995,0.587000012,0.114));
  r0.w = min(1, r0.w);
  r1.y = r7.x * r3.w;
  r1.z = -r3.w * r7.x + v5.w;
  r0.w = r0.w * r1.z + r1.y;
  r1.y = max(1, emissive_g);
  r3.xyz = r1.yyy * r0.xyz;
  r0.w = r0.w * r1.x;
  r1.x = materialFogIntensity_g * v5.z;
  r0.xyz = -r0.xyz * r1.yyy + fogColor_g.xyz;
  r0.xyz = r1.xxx * r0.xyz + r3.xyz;
  r1.x = -r2.w / volumeCameraFarClip_g;
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
  r0.yz = (uint2)r0.xx << int2(0,8);
  r0.x = (int)r0.y | (int)r0.x;
  o1.z = (int)r0.z | (int)r0.x;
  o0.w = r0.w;
  o1.w = 0;
  return;
}