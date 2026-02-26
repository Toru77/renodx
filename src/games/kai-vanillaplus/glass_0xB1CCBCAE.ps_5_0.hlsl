// ---- Created with 3Dmigoto v1.4.1 on Thu Feb 19 20:15:24 2026

struct LightParam
{
    float3 pos;
    float radius;
    float3 color;                  // Offset:   16
    float radiusInv;
    float3 charaColor;             // Offset:   32
    float attenuation;
    float3 vec;                    // Offset:   48
    float spotAngleInv;
    float attenuationAngle;        // Offset:   64
    float specularIntensity;
    float specularGlossiness;      // Offset:   72
    float scatterAnisotropy;
    float3 scatterColor;           // Offset:   80
    float scatterDensity;
    float translucency;            // Offset:   96
    int shadowmapIndex;
    float userParams[2];           // Offset:  104
};

struct LightIndexData
{
    int pointLightIndices[63];
    uint pointLightCount;          // Offset:  252
    int spotLightIndices[63];
    uint spotLightCount;           // Offset:  508
    int lightProbeIndices[14];
    uint lightProbeCount;          // Offset:  568
    float tileDepthInv;
};

struct InstanceParam
{
    float4x4 world;
    float4 color;
    float4 uv;                     // Offset:   80
    float4 param;
    uint boneAddress;              // Offset:  112
    float3 param2;
    float4x4 prevWorld;            // Offset:  128
};

cbuffer cb_tex_swizzle : register(b10)
{
  uint swizzle_flags_g : packoffset(c0);
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
  float ldotvXZ_g : packoffset(c25.w);
  float3 lightDirection_g : packoffset(c26);
  float gameTime_g : packoffset(c26.w);
  float3 sceneShadowColor_g : packoffset(c27);
  float chrLightIntensity_g : packoffset(c27.w);
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
  float disableMapObjNearFade_g : packoffset(c32.w);
  float4x4 ditherMtx_g : packoffset(c33);
  float4 lightProbe_g[9] : packoffset(c37);
  float3 chrLightDir_g : packoffset(c46);
  float windForce_g : packoffset(c46.w);
  float4 mapColor_g : packoffset(c47);
  float4 clipPlane_g : packoffset(c48);
  float2 resolutionScaling_g : packoffset(c49);
  float2 shadowSplitDistance_g : packoffset(c49.z);
  float4x4 shadowMtx_g[3] : packoffset(c50);
  float shadowFadeNear_g : packoffset(c62);
  float shadowFadeRangeInv_g : packoffset(c62.y);
  float2 invShadowSize_g : packoffset(c62.z);
  float4 frustumPlanes_g[6] : packoffset(c63);
  float4x4 prevView_g : packoffset(c69);
  float4x4 prevViewInv_g : packoffset(c73);
  float4x4 prevProj_g : packoffset(c77);
  float4x4 prevProjInv_g : packoffset(c81);
  float4x4 prevViewProj_g : packoffset(c85);
  float4x4 prevViewProjInv_g : packoffset(c89);
  float2 motionJitterOffset_g : packoffset(c93);
  float2 curJitterOffset_g : packoffset(c93.z);
  float prevSceneTime_g : packoffset(c94);
  uint enableMotionVectors_g : packoffset(c94.y);
  float prevWindWaveTime_g : packoffset(c94.z);
  float padding : packoffset(c94.w);
}

cbuffer cb_post_sky : register(b6)
{
  float3 incomingLight_g : packoffset(c0);
  uint isEnableSky_g : packoffset(c0.w);
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
  float cloudCoverage_g : packoffset(c9);
  float cloudThickness_g : packoffset(c9.y);
  uint cloudRaySteps_g : packoffset(c9.z);
  float cloudLightIntensity_g : packoffset(c9.w);
  float cloudDistance_g : packoffset(c10);
  float cloudFadeRangeInv_g : packoffset(c10.y);
  float cloudHeight_g : packoffset(c10.z);
  float cloudScale_g : packoffset(c10.w);
  float3 cloudColor_g : packoffset(c11);
}

cbuffer cb_volume_fog : register(b7)
{
  float volumeIntensity_g : packoffset(c0);
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
  float temporalRatio_g : packoffset(c3.w);
  float2 prevScaling_g : packoffset(c4);
  float2 prevUVClamp_g : packoffset(c4.z);
  float volumeNearFadeInv_g : packoffset(c5);
  float densityScale_g : packoffset(c5.y);
}

cbuffer cb_local : register(b5)
{
  float2 uvScroll0_g : packoffset(c0);
  float2 uvScroll1_g : packoffset(c0.z);
  float2 uvScroll2_g : packoffset(c1);
  float emissive_g : packoffset(c1.z);
  float materialFogIntensity_g : packoffset(c1.w);
  float3 diffuseMapColor0_g : packoffset(c2);
  float opacity_g : packoffset(c2.w);
  float translucency_g : packoffset(c3);
  float ssaoIntensity_g : packoffset(c3.y);
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
  float metalness_g : packoffset(c8);
  float roughness_g : packoffset(c8.y);
  float cryRoughness_g : packoffset(c8.z);
  float cryFresnel_g : packoffset(c8.w);
  float cryRefractionIndex_g : packoffset(c9);
  float cryBrightnessPower_g : packoffset(c9.y);
  float cryBrightness_g : packoffset(c9.z);
  float shadowCastOffset_g : packoffset(c9.w);
  float volumeFogInvalidity_g : packoffset(c10);
  uint materialID_g : packoffset(c10.y);
}

SamplerState Smpl0_s : register(s0);
SamplerState Smpl6_s : register(s6);
SamplerState SmplCube_s : register(s14);
SamplerState SmplLinearClamp_s : register(s15);
SamplerComparisonState SmplShadow_s : register(s13);
Texture2D<float4> Tex0 : register(t0);
Texture2D<float4> Tex6 : register(t6);
StructuredBuffer<LightParam> dynamicLights_g : register(t11);
StructuredBuffer<LightIndexData> lightIndices_g : register(t12);
StructuredBuffer<float4x4> spotShadowMatrices_g : register(t14);
StructuredBuffer<InstanceParam> instances_g : register(t15);
Texture2DArray<float4> shadowMaps : register(t16);
TextureCube<float4> texEnvMap_g : register(t17);
Texture2DArray<float4> spotShadowMaps : register(t18);
Texture3D<float4> atmosphereInscatterLUT : register(t19);
Texture3D<float4> atmosphereExtinctionLUT : register(t20);
Texture3D<float4> volumeFogTexture_g : register(t26);

#include "./kai-vanillaplus.h"

// 3Dmigoto declarations
#define cmp -

void main(
  float4 v0 : SV_Position0,
  float3 v1 : NORMAL0,
  nointerpolation uint4 v2 : TEXCOORD0,
  float4 v3 : TEXCOORD1,
  float4 v4 : TEXCOORD2,
  float4 v5 : TEXCOORD5,
  float4 v6 : TEXCOORD7,
  uint v7 : SV_IsFrontFace0,
  out float4 o0 : SV_Target0,
  out uint4 o1 : SV_Target1,
  out uint4 o2 : SV_Target2,
  out float2 o3 : SV_Target3)
{
  const float4 icb[] = { { 1.000000, 0, 0, 0},
                         { 0, 1.000000, 0, 0},
                         { 0, 0, 1.000000, 0},
                         { 0, 0, 0, 1.000000},
                         { -0.840520, -0.073954, 0, 0},
                         { -0.326235, -0.405830, 0, 0},
                         { -0.698464, 0.457259, 0, 0},
                         { -0.203356, 0.620585, 0, 0},
                         { 0.963450, -0.194353, 0, 0},
                         { 0.473434, -0.480026, 0, 0},
                         { 0.519454, 0.767034, 0, 0},
                         { 0.185461, -0.894523, 0, 0},
                         { 0.507351, 0.064963, 0, 0},
                         { -0.321932, 0.595435, 0, 0} };
  float4 r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12,r13,r14,r15,r16;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.x = dot(v3.xyz, clipPlane_g.xyz);
  r0.x = -clipPlane_g.w + r0.x;
  r0.x = cmp(r0.x < 0);
  if (r0.x != 0) discard;
  r0.xyz = v3.xyz;
  r0.w = 1;
  r1.x = dot(r0.xyzw, view_g._m00_m10_m20_m30);
  r1.y = dot(r0.xyzw, view_g._m01_m11_m21_m31);
  r0.x = dot(r0.xyzw, view_g._m02_m12_m22_m32);
  r0.zw = ddy_coarse(r1.xy);
  r0.y = ddy_coarse(r0.x);
  r2.yw = ddx_coarse(r1.yx);
  r2.z = ddx_coarse(r0.x);
  r1.xyz = r2.yzw * r0.yzw;
  r0.xyz = r0.wyz * r2.zwy + -r1.xyz;
  r0.w = dot(r0.xyz, r0.xyz);
  r0.w = rsqrt(r0.w);
  r0.yzw = r0.xyz * r0.www;
  r1.x = instances_g[v2.x].color.x;
  r1.y = instances_g[v2.x].color.y;
  r1.z = instances_g[v2.x].color.z;
  r1.w = instances_g[v2.x].color.w;
  r1.xyzw = v5.xyzw * r1.xyzw;
  r1.w = opacity_g * r1.w;
  r2.xy = v6.xy / v6.ww;
  r2.xy = r2.xy * float2(0.5,-0.5) + float2(0.5,0.5);
  r2.xy = r2.xy * vpSize_g.xy + -v0.xy;
  o3.xy = -motionJitterOffset_g.xy + r2.xy;
  r2.x = viewInv_g._m30;
  r2.y = viewInv_g._m31;
  r2.z = viewInv_g._m32;
  r3.xyz = -v3.xyz + r2.xyz;
  r2.w = dot(r3.xyz, r3.xyz);
  r3.w = sqrt(r2.w);
  r2.w = rsqrt(r2.w);
  r4.xyz = r3.xyz * r2.www;
  r5.x = instances_g[v2.x].param.x;
  r5.y = instances_g[v2.x].param.y;
  r3.w = -r5.x + r3.w;
  r3.w = r3.w * r5.y;
  r3.w = min(1, r3.w);
  r3.w = max(disableMapObjNearFade_g, r3.w);
  r4.w = dot(v1.xyz, v1.xyz);
  r4.w = rsqrt(r4.w);
  r5.yzw = v1.xyz * r4.www;
  r6.xy = v4.xy * float2(1,-1) + float2(0,1);
  r7.xyzw = Tex0.Sample(Smpl0_s, r6.xy).xyzw;
  r6.zw = int2(1,64) & swizzle_flags_g;
  r8.x = r7.x;
  r8.w = 1;
  r7.xyzw = r6.zzzz ? r8.xxxw : r7.xyzw;
  r7.xyz = diffuseMapColor0_g.xyz * r7.xyz;
  r8.xyzw = Tex6.Sample(Smpl6_s, r6.xy).xyzw;
  r9.x = r8.x;
  r9.w = 1;
  r6.xyzw = r6.wwww ? r9.xxxw : r8.xyzw;
  r1.xyzw = r7.xyzw * r1.xyzw;
  r4.w = emissive_g * r6.w;
  r6.w = dot(view_g._m02_m12_m22_m32, v3.xyzw);
  r7.xyz = lightProbe_g[1].xyz * r5.yyy + lightProbe_g[0].xyz;
  r7.xyz = lightProbe_g[2].xyz * r5.zzz + r7.xyz;
  r7.xyz = lightProbe_g[3].xyz * r5.www + r7.xyz;
  r8.xyz = lightProbe_g[4].xyz * r5.www;
  r7.xyz = r8.xyz * r5.yyy + r7.xyz;
  r8.xyz = lightProbe_g[5].xyz * r5.zzz;
  r7.xyz = r8.xyz * r5.www + r7.xyz;
  r8.xyz = lightProbe_g[6].xyz * r5.zzz;
  r7.xyz = r8.xyz * r5.yyy + r7.xyz;
  r8.xy = r5.wz * r5.wz;
  r7.w = r8.x * 3 + -1;
  r7.xyz = lightProbe_g[7].xyz * r7.www + r7.xyz;
  r7.w = r5.y * r5.y + -r8.y;
  r7.xyz = lightProbe_g[8].xyz * r7.www + r7.xyz;
  r8.xyz = sceneShadowColor_g.xyz + shadowColor_g.xyz;
  r8.xyz = min(float3(1,1,1), r8.xyz);
  r7.w = dot(r5.yzw, -lightDirection_g.xyz);
  r8.w = dot(r5.yzw, r4.xyz);
  r2.xyz = v3.xyz + -r2.xyz;
  r2.x = dot(r2.xyz, r2.xyz);
  r2.x = sqrt(r2.x);
  r2.y = shadowSplitDistance_g.y + -5;
  r2.y = cmp(r2.y < r2.x);
  if (r2.y != 0) {
    r9.x = dot(v3.xyzw, shadowMtx_g[2]._m00_m10_m20_m30);
    r9.y = dot(v3.xyzw, shadowMtx_g[2]._m01_m11_m21_m31);
    r9.z = dot(v3.xyzw, shadowMtx_g[2]._m02_m12_m22_m32);
    r2.y = dot(v3.xyzw, shadowMtx_g[2]._m03_m13_m23_m33);
    r9.xyz = r9.xyz / r2.yyy;
    r2.yz = cmp(r9.xy < float2(0,0));
    r10.xy = cmp(float2(1,1) < r9.xy);
    r2.y = (int)r2.y | (int)r10.x;
    r2.y = (int)r2.z | (int)r2.y;
    r2.y = (int)r10.y | (int)r2.y;
    if (r2.y != 0) {
      r2.y = 1;
    } else {
      r10.z = 2;
      r2.yz = float2(0,0);
      while (true) {
        r9.w = cmp((int)r2.z >= 10);
        if (r9.w != 0) break;
        r10.xy = saturate(icb[r2.z+4].xy * invShadowSize_g.xy + r9.xy);
        r9.w = shadowMaps.SampleCmpLevelZero(SmplShadow_s, r10.xyz, r9.z).x;
        r2.y = r9.w * 0.100000001 + r2.y;
        r2.z = (int)r2.z + 1;
      }
    }
    r2.z = cmp(r2.x < shadowSplitDistance_g.y);
    if (r2.z != 0) {
      r9.x = dot(v3.xyzw, shadowMtx_g[1]._m00_m10_m20_m30);
      r9.y = dot(v3.xyzw, shadowMtx_g[1]._m01_m11_m21_m31);
      r9.z = dot(v3.xyzw, shadowMtx_g[1]._m02_m12_m22_m32);
      r2.z = dot(v3.xyzw, shadowMtx_g[1]._m03_m13_m23_m33);
      r9.xyz = r9.xyz / r2.zzz;
      r10.z = 1;
      r2.z = 0;
      r9.w = 0;
      while (true) {
        r10.w = cmp((int)r9.w >= 10);
        if (r10.w != 0) break;
        r11.xy = icb[r9.w+4].xy * invShadowSize_g.xy;
        r10.xy = saturate(r11.xy * float2(1.125,1.125) + r9.xy);
        r10.x = shadowMaps.SampleCmpLevelZero(SmplShadow_s, r10.xyz, r9.z).x;
        r2.z = r10.x * 0.100000001 + r2.z;
        r9.w = (int)r9.w + 1;
      }
      r9.x = shadowSplitDistance_g.y + -r2.x;
      r9.x = 0.200000003 * r9.x;
      r2.z = r2.z + -r2.y;
      r2.y = r9.x * r2.z + r2.y;
    }
    r2.z = -shadowFadeNear_g + r2.x;
    r2.z = saturate(shadowFadeRangeInv_g * r2.z);
    r9.x = 1 + -r2.y;
    r2.y = r2.z * r9.x + r2.y;
  } else {
    r2.z = cmp(r2.x < shadowSplitDistance_g.x);
    r9.xyz = r2.zzz ? float3(0,0,0) : float3(1,4,1);
    r10.x = dot(v3.xyzw, shadowMtx_g[r9.y/4]._m00_m10_m20_m30);
    r10.y = dot(v3.xyzw, shadowMtx_g[r9.y/4]._m01_m11_m21_m31);
    r10.z = dot(v3.xyzw, shadowMtx_g[r9.y/4]._m02_m12_m22_m32);
    r9.w = dot(v3.xyzw, shadowMtx_g[r9.y/4]._m03_m13_m23_m33);
    r10.xyz = r10.xyz / r9.www;
    r9.w = dot(float2(1.25,1.125), icb[r9.x+0].xy);
    r2.y = 0;
    r10.w = 0;
    while (true) {
      r11.x = cmp((int)r10.w >= 10);
      if (r11.x != 0) break;
      r11.xy = icb[r10.w+4].xy * invShadowSize_g.xy;
      r9.xy = saturate(r11.xy * r9.ww + r10.xy);
      r9.x = shadowMaps.SampleCmpLevelZero(SmplShadow_s, r9.xyz, r10.z).x;
      r2.y = r9.x * 0.100000001 + r2.y;
      r10.w = (int)r10.w + 1;
    }
    r9.x = shadowSplitDistance_g.x + -5;
    r9.x = cmp(r9.x < r2.x);
    r2.z = r2.z ? r9.x : 0;
    if (r2.z != 0) {
      r9.x = dot(v3.xyzw, shadowMtx_g[1]._m00_m10_m20_m30);
      r9.y = dot(v3.xyzw, shadowMtx_g[1]._m01_m11_m21_m31);
      r9.z = dot(v3.xyzw, shadowMtx_g[1]._m02_m12_m22_m32);
      r2.z = dot(v3.xyzw, shadowMtx_g[1]._m03_m13_m23_m33);
      r9.xyz = r9.xyz / r2.zzz;
      r10.z = 1;
      r2.z = 0;
      r9.w = 0;
      while (true) {
        r10.w = cmp((int)r9.w >= 10);
        if (r10.w != 0) break;
        r11.xy = icb[r9.w+4].xy * invShadowSize_g.xy;
        r10.xy = saturate(r11.xy * float2(1.125,1.125) + r9.xy);
        r10.x = shadowMaps.SampleCmpLevelZero(SmplShadow_s, r10.xyz, r9.z).x;
        r2.z = r10.x * 0.100000001 + r2.z;
        r9.w = (int)r9.w + 1;
      }
      r2.x = shadowSplitDistance_g.x + -r2.x;
      r2.x = 0.200000003 * r2.x;
      r9.x = r2.y + -r2.z;
      r2.y = r2.x * r9.x + r2.z;
    }
  }
  r2.x = -1 + r2.y;
  r2.x = specularShadowFadeRatio_g * r2.x + 1;
  r2.z = 1 + -r7.w;
  r2.z = translucency_g * r2.z + r7.w;
  r2.z = r2.z * 0.5 + 0.5;
  r9.x = r2.z * r2.z;
  r2.z = -r2.z * r9.x + 1;
  r2.y = r2.z * r2.z + r2.y;
  r2.y = min(1, r2.y);
  r9.xyz = r3.xyz * r2.www + -lightDirection_g.xyz;
  r2.z = dot(r9.xyz, r9.xyz);
  r2.z = rsqrt(r2.z);
  r9.xyz = r9.xyz * r2.zzz;
  r2.z = lightSpecularGlossiness_g * specularGlossiness0_g;
  r9.x = saturate(dot(r9.xyz, r5.yzw));
  r2.z = max(0.00100000005, r2.z);
  r9.x = log2(r9.x);
  r2.z = r9.x * r2.z;
  r2.z = exp2(r2.z);
  r2.x = r2.z * r2.x;
  r2.x = lightSpecularIntensity_g * r2.x;
  r9.xyz = specularColor_g.xyz * r2.xxx;
  r2.xz = r7.ww * float2(0.5,-0.5) + float2(0.5,0.5);
  r2.z = max(r2.x, r2.z);
  r2.z = r2.z + -r2.x;
  r2.x = translucency_g * r2.z + r2.x;
  r10.xy = metalness_g * r6.xy;
  r2.z = r8.w + r8.w;
  r4.xyz = r5.yzw * -r2.zzz + r4.xyz;
  
  // -- FIXED GetDimensions & APPLIED CUBEMAP MODS -- 
  texEnvMap_g.GetDimensions(0, uiDest.x, uiDest.y, uiDest.w);
  r2.z = uiDest.w;
  r4.xyz = float3(1,-1,-1) * r4.xyz;
  r2.z = (int)r2.z + -1;
  r2.z = (uint)r2.z;
  r2.z = r10.y * r2.z;
  
  // Modify glass cubemap
  r2.z += 0.0; // Adds X mip levels of blur
  r4.xyz = texEnvMap_g.SampleLevel(SmplCube_s, r4.xyz, r2.z).xyz;
  r4.xyz *= lerp(1.0, 0.25, saturate(sss_injection_data.cubemap_improvements_enabled)); // Reduces the reflection intensity by X
  r2.z = cmp(0 < fresnel0_g);
  r6.x = 1 + -abs(r8.w);
  r6.x = max(0, r6.x);
  r7.w = log2(r6.x);
  r7.w = fresnel0_g * r7.w;
  r7.w = exp2(r7.w);
  r2.z = r2.z ? r7.w : 1;
  r7.w = r10.x * r2.z;
  r11.xyz = r1.xyz * r4.xyz + -r1.xyz;
  r1.xyz = r7.www * r11.xyz + r1.xyz;
  r7.w = dot(r4.xyz, float3(0.298999995,0.587000012,0.114));
  r8.w = r10.y * -9 + 10;
  r7.w = log2(r7.w);
  r7.w = r8.w * r7.w;
  r7.w = exp2(r7.w);
  r8.w = 1 + -r7.w;
  r7.w = r10.x * r8.w + r7.w;
  r4.xyz = r7.www * r4.xyz;
  r4.xyz = r4.xyz * r2.zzz;
  r6.y = -r6.y * roughness_g + 1;
  r4.xyz = r6.yyy * r4.xyz;
  r4.xyz = r9.xyz * lightColor_g.xyz + r4.xyz;
  r2.x = r2.x * r2.y;
  r9.xyz = float3(1,1,1) + -r8.xyz;
  r8.xyz = r2.xxx * r9.xyz + r8.xyz;
  r7.xyz = r8.xyz * lightColor_g.xyz + r7.xyz;
  r2.x = min(1, r4.w);
  r8.xyz = float3(1,1,1) + -r7.xyz;
  r7.xyz = r2.xxx * r8.xyz + r7.xyz;
  r2.x = rimIntensity_g * r6.x;
  r2.x = log2(r2.x);
  r2.x = rimLightPower_g * r2.x;
  r2.x = exp2(r2.x);
  r2.x = min(1, r2.x);
  r4.xyz = rimLightColor_g.xyz * r2.xxx + r4.xyz;
  r2.xy = lightTileSizeInv_g.xy * v0.xy;
  r2.xy = (uint2)r2.xy;
  r2.y = (uint)r2.y << 5;
  r6.x = (int)r2.x + (int)r2.y;
  r6.x = lightIndices_g[(uint)r6.x].tileDepthInv;
  r6.x = -r6.w * r6.x;
  r6.x = min(7, r6.x);
  r6.x = max(0, r6.x);
  r6.x = (uint)r6.x;
  r2.y = mad((int)r6.x, 576, (int)r2.y);
  r2.x = (int)r2.x + (int)r2.y;
  r2.x = min(4607, (uint)r2.x);
  r2.y = lightIndices_g[(uint)r2.x].pointLightCount;
  r2.y = min(63, (uint)r2.y);
  r8.xyz = float3(0,0,0);
  r9.xyz = float3(0,0,0);
  r6.x = 0;
  while (true) {
    r6.y = cmp((uint)r6.x >= (uint)r2.y);
    if (r6.y != 0) break;
    
    // -- FIXED DYNAMIC STRUCTURED BUFFER READ (POINT LIGHTS) --
    r6.y = lightIndices_g[(uint)r2.x].pointLightIndices[(uint)r6.x];
    
    r10.x = dynamicLights_g[(uint)r6.y].pos.x;
    r10.y = dynamicLights_g[(uint)r6.y].pos.y;
    r10.z = dynamicLights_g[(uint)r6.y].pos.z;
    r10.xyz = -v3.xyz + r10.xyz;
    r7.w = dot(r10.xyz, r10.xyz);
    r8.w = sqrt(r7.w);
    r9.w = dynamicLights_g[(uint)r6.y].radiusInv;
    r8.w = r9.w * r8.w;
    r9.w = dynamicLights_g[(uint)r6.y].attenuation;
    r8.w = log2(abs(r8.w));
    r8.w = r9.w * r8.w;
    r8.w = exp2(r8.w);
    r8.w = 1 + -r8.w;
    r8.w = max(0, r8.w);
    r9.w = cmp(0 < r8.w);
    if (r9.w != 0) {
      r7.w = rsqrt(r7.w);
      r10.xyz = r10.xyz * r7.www;
      r7.w = dynamicLights_g[(uint)r6.y].translucency;
      r9.w = dot(r10.xyz, r5.yzw);
      r7.w = max(r9.w, r7.w);
      r7.w = r8.w * r7.w;
      r11.x = dynamicLights_g[(uint)r6.y].color.x;
      r11.y = dynamicLights_g[(uint)r6.y].color.y;
      r11.z = dynamicLights_g[(uint)r6.y].color.z;
      r9.xyz = r11.xyz * r7.www + r9.xyz;
      r10.xyz = r3.xyz * r2.www + r10.xyz;
      r8.w = dot(r10.xyz, r10.xyz);
      r8.w = rsqrt(r8.w);
      r10.xyz = r10.xyz * r8.www;
      r12.x = dynamicLights_g[(uint)r6.y].specularIntensity;
      r12.y = dynamicLights_g[(uint)r6.y].specularGlossiness;
      r6.y = specularGlossiness0_g * r12.y;
      r8.w = saturate(dot(r10.xyz, r5.yzw));
      r6.y = max(0.00100000005, r6.y);
      r8.w = log2(r8.w);
      r6.y = r8.w * r6.y;
      r6.y = exp2(r6.y);
      r10.xyz = r11.xyz * r6.yyy;
      r10.xyz = r10.xyz * r7.www;
      r8.xyz = r10.xyz * r12.xxx + r8.xyz;
    }
    r6.x = (int)r6.x + 1;
  }
  r7.xyz = r9.xyz * dynamicLightIntensity_g + r7.xyz;
  r2.y = lightIndices_g[(uint)r2.x].spotLightCount;
  r2.y = min(63, (uint)r2.y);
  r9.xyz = r8.xyz;
  r10.xyz = float3(0,0,0);
  r6.x = 0;
  while (true) {
    r6.y = cmp((uint)r6.x >= (uint)r2.y);
    if (r6.y != 0) break;
    
    // -- FIXED DYNAMIC STRUCTURED BUFFER READ (SPOT LIGHTS) --
    r6.y = lightIndices_g[(uint)r2.x].spotLightIndices[(uint)r6.x];
    
    r11.x = dynamicLights_g[(uint)r6.y].pos.x;
    r11.y = dynamicLights_g[(uint)r6.y].pos.y;
    r11.z = dynamicLights_g[(uint)r6.y].pos.z;
    r11.xyz = -v3.xyz + r11.xyz;
    r7.w = dot(r11.xyz, r11.xyz);
    r8.w = rsqrt(r7.w);
    r11.xyz = r11.xyz * r8.www;
    r12.x = dynamicLights_g[(uint)r6.y].vec.x;
    r12.y = dynamicLights_g[(uint)r6.y].vec.y;
    r12.z = dynamicLights_g[(uint)r6.y].vec.z;
    r12.w = dynamicLights_g[(uint)r6.y].spotAngleInv;
    r8.w = dot(r11.xyz, r12.xyz);
    r8.w = max(0, r8.w);
    r8.w = 1 + -r8.w;
    r8.w = r8.w * r12.w;
    r9.w = dynamicLights_g[(uint)r6.y].attenuationAngle;
    r8.w = log2(r8.w);
    r8.w = r9.w * r8.w;
    r8.w = exp2(r8.w);
    r8.w = 1 + -r8.w;
    r8.w = max(0, r8.w);
    r9.w = cmp(0 < r8.w);
    if (r9.w != 0) {
      r7.w = sqrt(r7.w);
      r9.w = dynamicLights_g[(uint)r6.y].radiusInv;
      r7.w = r9.w * r7.w;
      r9.w = dynamicLights_g[(uint)r6.y].attenuation;
      r7.w = log2(abs(r7.w));
      r7.w = r9.w * r7.w;
      r7.w = exp2(r7.w);
      r7.w = 1 + -r7.w;
      r7.w = max(0, r7.w);
      r7.w = r8.w * r7.w;
      r8.w = cmp(0 < r7.w);
      if (r8.w != 0) {
        r12.x = dynamicLights_g[(uint)r6.y].translucency;
        r12.y = dynamicLights_g[(uint)r6.y].shadowmapIndex;
        r8.w = cmp((int)r12.y != -1);
        
        // -- FIXED SPOT SHADOW MAP BUG (r12.w slice overwrite) --
        if (r8.w != 0) {
          r13.xyzw = spotShadowMatrices_g[(uint)r12.y]._m00_m10_m20_m30;
          r14.xyzw = spotShadowMatrices_g[(uint)r12.y]._m01_m11_m21_m31;
          r15.xyzw = spotShadowMatrices_g[(uint)r12.y]._m02_m12_m22_m32;
          r16.xyzw = spotShadowMatrices_g[(uint)r12.y]._m03_m13_m23_m33;
          r13.x = dot(v3.xyzw, r13.xyzw);
          r13.y = dot(v3.xyzw, r14.xyzw);
          r13.z = dot(v3.xyzw, r15.xyzw);
          r8.w = dot(v3.xyzw, r16.xyzw);
          r13.xyz = r13.xyz / r8.www;
          r13.w = (uint)r12.y; // Correctly mapped array slice
          
          r8.w = spotShadowMaps.SampleCmpLevelZero(SmplShadow_s, r13.xyw, r13.z).x;
          
          r14.xyz = float3(0.00244140625,0,0) + r13.xyw;
          r9.w = spotShadowMaps.SampleCmpLevelZero(SmplShadow_s, r14.xyz, r13.z).x;
          r9.w = 0.200000003 * r9.w;
          r8.w = r8.w * 0.200000003 + r9.w;
          
          r14.xyz = float3(-0.00244140625,0,0) + r13.xyw;
          r9.w = spotShadowMaps.SampleCmpLevelZero(SmplShadow_s, r14.xyz, r13.z).x;
          r8.w = r9.w * 0.200000003 + r8.w;
          
          r14.xyz = float3(0,0.00244140625,0) + r13.xyw;
          r9.w = spotShadowMaps.SampleCmpLevelZero(SmplShadow_s, r14.xyz, r13.z).x;
          r8.w = r9.w * 0.200000003 + r8.w;
          
          r14.xyz = float3(0,-0.00244140625,0) + r13.xyw;
          r9.w = spotShadowMaps.SampleCmpLevelZero(SmplShadow_s, r14.xyz, r13.z).x;
          r8.w = r9.w * 0.200000003 + r8.w;
          
          r7.w = r8.w * r7.w;
        }
        // -------------------------------------------------------

        r8.w = dot(r11.xyz, r5.yzw);
        r8.w = max(r12.x, r8.w);
        r7.w = r8.w * r7.w;
        r12.x = dynamicLights_g[(uint)r6.y].color.x;
        r12.y = dynamicLights_g[(uint)r6.y].color.y;
        r12.z = dynamicLights_g[(uint)r6.y].color.z;
        r10.xyz = r12.xyz * r7.www + r10.xyz;
        r11.xyz = r3.xyz * r2.www + r11.xyz;
        r8.w = dot(r11.xyz, r11.xyz);
        r8.w = rsqrt(r8.w);
        r11.xyz = r11.xyz * r8.www;
        r13.x = dynamicLights_g[(uint)r6.y].specularIntensity;
        r13.y = dynamicLights_g[(uint)r6.y].specularGlossiness;
        r6.y = specularGlossiness0_g * r13.y;
        r8.w = saturate(dot(r11.xyz, r5.yzw));
        r6.y = max(0.00100000005, r6.y);
        r8.w = log2(r8.w);
        r6.y = r8.w * r6.y;
        r6.y = exp2(r6.y);
        r11.xyz = r12.xyz * r6.yyy;
        r11.xyz = r11.xyz * r7.www;
        r9.xyz = r11.xyz * r13.xxx + r9.xyz;
      }
    }
    r6.x = (int)r6.x + 1;
  }
  r2.xyw = r10.xyz * dynamicLightIntensity_g + r7.xyz;
  r3.xyz = r9.xyz * dynamicLightIntensity_g + r4.xyz;
  r4.xyz = r3.xyz * r6.zzz;
  r1.xyz = r1.xyz * r2.xyw + r4.xyz;
  r2.x = dot(r3.xyz, float3(0.298999995,0.587000012,0.114));
  r2.x = min(1, r2.x);
  r2.y = r1.w * r2.z;
  r2.w = instances_g[v2.x].color.w;
  r1.w = -r1.w * r2.z + r2.w;
  r1.w = r2.x * r1.w + r2.y;
  r2.x = max(1, r4.w);
  r2.yzw = r2.xxx * r1.xyz;
  r1.w = r1.w * r3.w;
  r3.x = -fogNearDistance_g + -r6.w;
  r3.x = saturate(fogFadeRangeInv_g * r3.x);
  r3.y = -fogHeight_g + v3.y;
  r3.y = saturate(fogHeightRangeInv_g * r3.y);
  r3.x = r3.x * r3.y;
  r3.y = fogIntensity_g * r3.x;
  r3.y = materialFogIntensity_g * r3.y;
  r1.xyz = -r1.xyz * r2.xxx + fogColor_g.xyz;
  r1.xyz = r3.yyy * r1.xyz + r2.yzw;
  r2.x = -r6.w / skyLutCameraFarClip_g;
  r4.xy = invVPSize_g.xy * v0.xy;
  r2.x = -skyLutNearOverFarClip_g + r2.x;
  r2.y = -skyLutNearOverFarClip_g + 1;
  r4.z = r2.x / r2.y;
  r2.xyz = atmosphereInscatterLUT.SampleLevel(SmplLinearClamp_s, r4.xyz, 0).xyz;
  r3.yzw = atmosphereExtinctionLUT.SampleLevel(SmplLinearClamp_s, r4.xyz, 0).xyz;
  r1.xyz = r1.xyz * r3.yzw + r2.xyz;
  r2.x = -r6.w / volumeCameraFarClip_g;
  r4.xy = resolutionScaling_g.xy * r4.xy;
  r2.x = r2.x * volumeCameraFarOverMaxFar_g + -volumeNearOverFarClip_g;
  r2.y = -volumeNearOverFarClip_g + 1;
  r4.z = r2.x / r2.y;
  r2.xyzw = volumeFogTexture_g.SampleLevel(SmplLinearClamp_s, r4.xyz, 0).xyzw;
  r2.xyz = r1.xyz * r2.www + r2.xyz;
  r2.xyz = r2.xyz + -r1.xyz;
  r2.xyz = combineAlpha_g * r2.xyz + r1.xyz;
  r1.xyz = -r2.xyz + r1.xyz;
  r1.xyz = volumeFogInvalidity_g * r1.xyz + r2.xyz;
  o0.xyz = mapColor_g.xyz * r1.xyz;
  r1.x = -r3.x * fogIntensity_g + 1;
  r1.x = ssaoIntensity_g * r1.x;
  r1.y = min(abs(r5.z), abs(r5.y));
  r1.z = max(abs(r5.z), abs(r5.y));
  r1.z = 1 / r1.z;
  r1.xy = r1.xy * r1.wz;
  r1.z = r1.y * r1.y;
  r2.x = r1.z * 0.0208350997 + -0.0851330012;
  r2.x = r1.z * r2.x + 0.180141002;
  r2.x = r1.z * r2.x + -0.330299497;
  r1.z = r1.z * r2.x + 0.999866009;
  r2.x = r1.y * r1.z;
  r2.y = cmp(abs(r5.y) < abs(r5.z));
  r2.x = r2.x * -2 + 1.57079637;
  r2.x = r2.y ? r2.x : 0;
  r1.y = r1.y * r1.z + r2.x;
  r1.z = cmp(r5.y < -r5.y);
  r1.z = r1.z ? -3.141593 : 0;
  r1.y = r1.y + r1.z;
  r1.z = min(r5.z, r5.y);
  r2.x = max(r5.z, r5.y);
  r1.z = cmp(r1.z < -r1.z);
  r2.x = cmp(r2.x >= -r2.x);
  r1.z = r1.z ? r2.x : 0;
  r1.y = r1.z ? -r1.y : r1.y;
  r5.x = 0.318309873 * r1.y;
  r1.yz = float2(1,1) + r5.xw;
  r2.x = min(abs(r0.z), abs(r0.y));
  r2.y = max(abs(r0.z), abs(r0.y));
  r2.y = 1 / r2.y;
  r2.x = r2.x * r2.y;
  r2.y = r2.x * r2.x;
  r2.z = r2.y * 0.0208350997 + -0.0851330012;
  r2.z = r2.y * r2.z + 0.180141002;
  r2.z = r2.y * r2.z + -0.330299497;
  r2.y = r2.y * r2.z + 0.999866009;
  r2.z = r2.x * r2.y;
  r2.w = cmp(abs(r0.y) < abs(r0.z));
  r2.z = r2.z * -2 + 1.57079637;
  r2.z = r2.w ? r2.z : 0;
  r2.x = r2.x * r2.y + r2.z;
  r2.y = cmp(r0.y < -r0.y);
  r2.y = r2.y ? -3.141593 : 0;
  r2.x = r2.x + r2.y;
  r2.y = min(r0.z, r0.y);
  r0.y = max(r0.z, r0.y);
  r0.z = cmp(r2.y < -r2.y);
  r0.y = cmp(r0.y >= -r0.y);
  r0.y = r0.y ? r0.z : 0;
  r0.y = r0.y ? -r2.x : r2.x;
  r0.x = 0.318309873 * r0.y;
  r0.xy = float2(1,1) + r0.xw;
  r0.zw = float2(32767.5,32767.5) * r1.yz;
  r0.zw = (uint2)r0.zw;
  o1.xy = min(uint2(65535,65535), (uint2)r0.zw);
  r0.xy = float2(127.5,127.5) * r0.xy;
  r0.xy = (uint2)r0.xy;
  r0.xy = min(uint2(255,255), (uint2)r0.xy);
  o1.w = mad((int)r0.y, 256, (int)r0.x);
  r0.x = 255 * r1.x;
  r0.x = (uint)r0.x;
  r0.x = min(255, (uint)r0.x);
  o2.y = mad((int)r0.x, 256, (int)r0.x);
  o0.w = r1.w;
  o1.z = 0;
  o2.xw = float2(0,0);
  o2.z = r0.x;
  return;
}
