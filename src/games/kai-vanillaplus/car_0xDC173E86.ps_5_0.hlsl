// ---- Created with 3Dmigoto v1.4.1 on Mon Mar  2 01:29:58 2026

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
    float2 reserve;                // Offset:  132
    uint flag;                     // Offset:  140
};

struct OutlineShapeParam
{
    float4x4 mtx;                  // Offset:    0
    float4 color;                  // Offset:   64
    float2 gradation_size;         // Offset:   80
    float gradation_sharpness;     // Offset:   88
    uint type;                     // Offset:   92
    float radius;                  // Offset:   96
    float height_base;             // Offset:  100
    float height_width;            // Offset:  104
    float height_gradation_width;  // Offset:  108
    float fan_angle;               // Offset:  112
    float pad[3];                  // Offset:  116
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
    float userParams[2];           // Offset:  104
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

struct LightProbeParam
{
    float4 pos;                    // Offset:    0
    float radius;                  // Offset:   16
    float radiusInv;               // Offset:   20
    float attenuation;             // Offset:   24
    float intensity;               // Offset:   28
    float4 sh[9];                  // Offset:   32
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

cbuffer cb_deferred : register(b4)
{
  float3 mapAOColor_g : packoffset(c0);
  uint outlineShapeCount_g : packoffset(c0.w);
  float4 outlineShapeMaskUVParam_g : packoffset(c1);
  float ssgiShadowRatio_g : packoffset(c2);
}

SamplerState samPoint_s : register(s0);
SamplerState samLinear_s : register(s1);
SamplerState SmplCube_s : register(s14);
SamplerState SmplLinearClamp_s : register(s15);
SamplerComparisonState SmplShadow_s : register(s13);
Texture2D<float4> colorTexture : register(t0);
Texture2D<uint4> mrtTexture0 : register(t1);
Texture2D<uint4> mrtTexture1 : register(t2);
Texture2D<float4> depthTexture : register(t3);
Texture2D<float4> ssaoTexture : register(t4);
StructuredBuffer<DeferredParam> deferredParams_g : register(t5);
StructuredBuffer<OutlineShapeParam> outlineShapes_g : register(t6);
Texture2D<float4> outlineShapeMask : register(t7);
Texture2D<float4> ssgiTexture : register(t9);
StructuredBuffer<LightParam> dynamicLights_g : register(t11);
StructuredBuffer<LightIndexData> lightIndices_g : register(t12);
StructuredBuffer<LightProbeParam> localLightProbes_g : register(t13);
StructuredBuffer<float4x4> spotShadowMatrices_g : register(t14);
Texture2DArray<float4> shadowMaps : register(t16);
TextureCube<float4> texEnvMap_g : register(t17);
Texture2DArray<float4> spotShadowMaps : register(t18);
Texture3D<float4> atmosphereInscatterLUT : register(t19);
Texture3D<float4> atmosphereExtinctionLUT : register(t20);
Texture2D<float4> texMirror_g : register(t21);
Texture2D<float4> texSSRMap_g : register(t24);

#include "./kai-vanillaplus.h"


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_Position0,
  float4 v1 : TEXCOORD0,
  out float4 o0 : SV_Target0)
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
  float4 r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20;
  uint4 bitmask, uiDest;
  float4 fDest;
  float car_mode = (sss_injection_data.car_mode >= 0.5) ? 1.0 : 0.0;
  float car_diffuse_scale = lerp(1.0, max(sss_injection_data.car_diffuse_scale, 0.0), car_mode);
  float car_specular_scale = lerp(1.0, max(sss_injection_data.car_specular_scale, 0.0), car_mode);
  float car_reflection_scale = lerp(1.0, max(sss_injection_data.car_reflection_scale, 0.0), car_mode);
  float car_local_light_scale = lerp(1.0, max(sss_injection_data.car_local_light_scale, 0.0), car_mode);
  float car_ambient_scale = lerp(1.0, max(sss_injection_data.car_ambient_scale, 0.0), car_mode);
  float car_rim_scale = lerp(1.0, max(sss_injection_data.car_rim_scale, 0.0), car_mode);
  float car_shadow_scale = lerp(1.0, max(sss_injection_data.car_shadow_scale, 0.0), car_mode);
  float car_ssr_scale = lerp(1.0, max(sss_injection_data.car_ssr_scale, 0.0), car_mode);
  float car_cubemap_mip_scale = lerp(1.0, max(sss_injection_data.car_cubemap_mip_scale, 0.0), car_mode);
  float car_cubemap_brightness = lerp(1.0, max(sss_injection_data.car_cubemap_brightness, 0.0), car_mode);

  r0.xyz = colorTexture.SampleLevel(samPoint_s, v1.xy, 0).xyz;
  mrtTexture0.GetDimensions(0, fDest.x, fDest.y, fDest.z);
  r1.xy = fDest.xy;
  r1.xy = v1.xy * r1.xy;
  r1.xy = (int2)r1.xy;
  r1.zw = float2(0,0);
  r1.xyz = mrtTexture0.Load(r1.xyz).xyz;
  mrtTexture1.GetDimensions(0, fDest.x, fDest.y, fDest.z);
  r2.xy = fDest.xy;
  r2.xy = v1.xy * r2.xy;
  r2.xy = (int2)r2.xy;
  r2.zw = float2(0,0);
  r2.xyzw = mrtTexture1.Load(r2.xyz).xyzw;
  r0.w = ssaoTexture.SampleLevel(samLinear_s, v1.xy, 0).x;
  r3.z = depthTexture.SampleLevel(samPoint_s, v1.xy, 0).x;
  r1.w = (uint)r1.z >> 8;
  r1.xy = (uint2)r1.xy;
  r4.zw = r1.xy * float2(3.05180438e-05,3.05180438e-05) + float2(-1,-1);
  r1.x = 3.14159274 * r4.z;
  sincos(r1.x, r1.x, r5.x);
  r1.y = r4.w * r4.w;
  r4.z = -r4.w * r4.w + 1;
  r4.z = sqrt(r4.z);
  r4.x = r5.x * r4.z;
  r4.y = r4.z * r1.x;
  r1.xzw = (int3)r1.wzw & int3(2,255,16);
  r4.z = cmp((int)r1.x != 0);
  r5.x = ~(int)r4.z;
  if (r5.x != 0) discard;
  r1.z = (uint)r1.z;
  r5.x = 0.00392156886 * r1.z;
  r6.xyw = (uint3)r2.yzw >> int3(8,8,8);
  r6.z = r2.y;
  r5.yz = (int2)r6.zx & int2(255,255);
  r5.yz = (uint2)r5.yz;
  r5.yz = float2(0.00392156886,0.00392156886) * r5.yz;
  r6.xz = r2.zw;
  r6.xyzw = (int4)r6.xyzw & int4(255,255,255,255);
  r6.xyzw = (uint4)r6.xyzw;
  r7.xyzw = float4(0.00392156886,0.00392156886,0.00392156886,0.00392156886) * r6.yxzw;
  r2.x = min(0x00004e1f, (uint)r2.x);
  r8.x = deferredParams_g[r2.x].shadowColor.x;
  r8.y = deferredParams_g[r2.x].shadowColor.y;
  r8.z = deferredParams_g[r2.x].shadowColor.z;
  r8.w = deferredParams_g[r2.x].emissive;
  r9.x = deferredParams_g[r2.x].specularColor.x;
  r9.y = deferredParams_g[r2.x].specularColor.y;
  r9.z = deferredParams_g[r2.x].specularColor.z;
  r9.w = deferredParams_g[r2.x].rimLightPower;
  r10.x = deferredParams_g[r2.x].rimLightColor.x;
  r10.y = deferredParams_g[r2.x].rimLightColor.y;
  r10.z = deferredParams_g[r2.x].rimLightColor.z;
  r10.w = deferredParams_g[r2.x].rimIntensity;
  r11.x = deferredParams_g[r2.x].fresnels.x;
  r11.y = deferredParams_g[r2.x].fresnels.y;
  r11.z = deferredParams_g[r2.x].fresnels.z;
  r12.x = deferredParams_g[r2.x].specularGlossinesses.x;
  r12.y = deferredParams_g[r2.x].specularGlossinesses.y;
  r12.z = deferredParams_g[r2.x].specularGlossinesses.z;
  r12.w = deferredParams_g[r2.x].dynamicLightIntensity;
  r13.x = deferredParams_g[r2.x].materialFogIntensity;
  r13.y = deferredParams_g[r2.x].metalness;
  r13.z = deferredParams_g[r2.x].roughness;
  r13.w = deferredParams_g[r2.x].cryRefractionIndex;
  r2.y = deferredParams_g[r2.x].flag;
  r11.w = r12.x;
  r14.xz = r11.yz;
  r14.yw = r12.yz;
  r2.zw = r14.xy + -r11.xw;
  r2.zw = r7.zz * r2.zw + r11.xw;
  r6.xz = r14.zw + -r2.zw;
  r2.zw = r7.ww * r6.xz + r2.zw;
  r3.xy = v1.zw * float2(2,-2) + float2(-1,1);
  r3.w = 1;
  r11.x = dot(r3.xyzw, viewProjInv_g._m00_m10_m20_m30);
  r11.y = dot(r3.xyzw, viewProjInv_g._m01_m11_m21_m31);
  r11.z = dot(r3.xyzw, viewProjInv_g._m02_m12_m22_m32);
  r11.w = dot(r3.xyzw, viewProjInv_g._m03_m13_m23_m33);
  r3.xyzw = r11.xyzw / r11.wwww;
  r5.w = dot(view_g._m02_m12_m22_m32, r3.xyzw);
  r6.xz = lightTileSizeInv_g.xy * v0.xy;
  r6.xz = (uint2)r6.xz;
  r6.z = (uint)r6.z << 5;
  r6.w = (int)r6.x + (int)r6.z;
  r6.w = lightIndices_g[r6.w].tileDepthInv;
  r6.w = r6.w * -r5.w;
  r6.w = min(7, r6.w);
  r6.w = max(0, r6.w);
  r6.w = (uint)r6.w;
  r6.z = mad((int)r6.w, 576, (int)r6.z);
  r6.x = (int)r6.x + (int)r6.z;
  r6.x = min(4607, (uint)r6.x);
  r6.z = lightIndices_g[r6.x].lightProbeCount;
  r6.z = min(14, (uint)r6.z);
  r1.y = r1.y * 3 + -1;
  r6.w = r4.y * r4.y;
  r6.w = r4.x * r4.x + -r6.w;
  r11.xyz = float3(0,0,0);
  r14.w = 0;
  r7.z = 0;
  while (true) {
    r7.w = cmp((uint)r7.z >= (uint)r6.z);
    if (r7.w != 0) break;
    r7.w = (uint)r7.z << 2;
    r7.w = (int)r7.w + 512;
    r7.w = lightIndices_g[r6.x].lightProbeIndices[(int)r7.z];
    r12.x = localLightProbes_g[r7.w].pos.x;
    r12.y = localLightProbes_g[r7.w].pos.y;
    r12.z = localLightProbes_g[r7.w].pos.z;
    r12.xyz = r12.xyz + -r3.xyz;
    r11.w = dot(r12.xyz, r12.xyz);
    r11.w = sqrt(r11.w);
    r12.x = localLightProbes_g[r7.w].radiusInv;
    r12.y = localLightProbes_g[r7.w].attenuation;
    r12.z = localLightProbes_g[r7.w].intensity;
    r11.w = r12.x * r11.w;
    r11.w = log2(abs(r11.w));
    r11.w = r12.y * r11.w;
    r11.w = exp2(r11.w);
    r11.w = 1 + -r11.w;
    r11.w = max(0, r11.w);
    r12.x = r11.w * r12.z;
    r12.y = cmp(0 >= r12.x);
    if (r12.y != 0) {
      r12.y = (int)r7.z + 1;
      r7.z = r12.y;
      continue;
    }
    r15.x = localLightProbes_g[r7.w].sh[0].x;
    r15.y = localLightProbes_g[r7.w].sh[0].y;
    r15.z = localLightProbes_g[r7.w].sh[0].z;
    r16.x = localLightProbes_g[r7.w].sh[1].x;
    r16.y = localLightProbes_g[r7.w].sh[1].y;
    r16.z = localLightProbes_g[r7.w].sh[1].z;
    r15.xyz = r16.xyz * r4.xxx + r15.xyz;
    r16.x = localLightProbes_g[r7.w].sh[2].x;
    r16.y = localLightProbes_g[r7.w].sh[2].y;
    r16.z = localLightProbes_g[r7.w].sh[2].z;
    r15.xyz = r16.xyz * r4.yyy + r15.xyz;
    r16.x = localLightProbes_g[r7.w].sh[3].x;
    r16.y = localLightProbes_g[r7.w].sh[3].y;
    r16.z = localLightProbes_g[r7.w].sh[3].z;
    r15.xyz = r16.xyz * r4.www + r15.xyz;
    r16.x = localLightProbes_g[r7.w].sh[4].x;
    r16.y = localLightProbes_g[r7.w].sh[4].y;
    r16.z = localLightProbes_g[r7.w].sh[4].z;
    r16.xyz = r16.xyz * r4.www;
    r15.xyz = r16.xyz * r4.xxx + r15.xyz;
    r16.x = localLightProbes_g[r7.w].sh[5].x;
    r16.y = localLightProbes_g[r7.w].sh[5].y;
    r16.z = localLightProbes_g[r7.w].sh[5].z;
    r16.xyz = r16.xyz * r4.yyy;
    r15.xyz = r16.xyz * r4.www + r15.xyz;
    r16.x = localLightProbes_g[r7.w].sh[6].x;
    r16.y = localLightProbes_g[r7.w].sh[6].y;
    r16.z = localLightProbes_g[r7.w].sh[6].z;
    r16.xyz = r16.xyz * r4.yyy;
    r15.xyz = r16.xyz * r4.xxx + r15.xyz;
    r16.x = localLightProbes_g[r7.w].sh[7].x;
    r16.y = localLightProbes_g[r7.w].sh[7].y;
    r16.z = localLightProbes_g[r7.w].sh[7].z;
    r15.xyz = r16.xyz * r1.yyy + r15.xyz;
    r16.x = localLightProbes_g[r7.w].sh[8].x;
    r16.y = localLightProbes_g[r7.w].sh[8].y;
    r16.z = localLightProbes_g[r7.w].sh[8].z;
    r15.xyz = r16.xyz * r6.www + r15.xyz;
    r11.xyz = r15.xyz * r12.xxx + r11.xyz;
    r14.w = r11.w * r12.z + r14.w;
    r7.z = (int)r7.z + 1;
  }
  r6.z = cmp(r14.w == 0.000000);
  r6.z = r6.z ? 1.000000 : 0;
  r6.z = r14.w + r6.z;
  r11.xyz = r11.xyz / r6.zzz;
  r14.w = saturate(r14.w);
  r6.z = 1 + -r14.w;
  r14.xyz = r14.www * r11.xyz;
  r11.xyzw = max(float4(0,0,0,0), r14.xyzw);
  r7.z = cmp(0 < r6.z);
  r12.xyz = lightProbe_g[1].xyz * r4.xxx + lightProbe_g[0].xyz;
  r12.xyz = lightProbe_g[2].xyz * r4.yyy + r12.xyz;
  r12.xyz = lightProbe_g[3].xyz * r4.www + r12.xyz;
  r14.xyz = lightProbe_g[4].xyz * r4.www;
  r12.xyz = r14.xyz * r4.xxx + r12.xyz;
  r14.xyz = lightProbe_g[5].xyz * r4.yyy;
  r12.xyz = r14.xyz * r4.www + r12.xyz;
  r14.xyz = lightProbe_g[6].xyz * r4.yyy;
  r12.xyz = r14.xyz * r4.xxx + r12.xyz;
  r12.xyz = lightProbe_g[7].xyz * r1.yyy + r12.xyz;
  r12.xyz = lightProbe_g[8].xyz * r6.www + r12.xyz;
  r12.xyz = r12.xyz * r6.zzz;
  r12.xyz = (r7.z != 0) ? r12.xyz : 0;
  r12.xyz = max(float3(0,0,0), r12.xyz);
  r12.xyz = r12.xyz + r11.xyz;
  r14.xyzw = ssgiTexture.SampleLevel(samLinear_s, v1.zw, 0).xyzw;
  r15.x = viewInv_g._m30;
  r15.y = viewInv_g._m31;
  r15.z = viewInv_g._m32;
  r16.xyz = r15.xyz + -r3.xyz;
  r1.y = dot(r16.xyz, r16.xyz);
  r1.y = rsqrt(r1.y);
  r17.xyz = r16.xyz * r1.yyy;
  r5.x = r8.w * r5.x;
  r6.z = dot(r4.xyw, r17.xyz);
  r18.xyzw = (int4)r2.yyyy & int4(1,2,12,4);
  if (r18.x != 0) {
    r15.xyz = -r15.xyz + r3.xyz;
    r6.w = dot(r15.xyz, r15.xyz);
    r6.w = sqrt(r6.w);
    r7.z = shadowSplitDistance_g.y + -5;
    r7.z = cmp(r7.z < r6.w);
    if (r7.z != 0) {
      r15.x = dot(r3.xyzw, shadowMtx_g[2]._m00_m10_m20_m30);
      r15.y = dot(r3.xyzw, shadowMtx_g[2]._m01_m11_m21_m31);
      r15.z = dot(r3.xyzw, shadowMtx_g[2]._m02_m12_m22_m32);
      r7.z = dot(r3.xyzw, shadowMtx_g[2]._m03_m13_m23_m33);
      r15.xyz = r15.xyz / r7.zzz;
      r7.zw = cmp(r15.xy < float2(0,0));
      r19.xy = cmp(float2(1,1) < r15.xy);
      r7.z = (int)r7.z | (int)r19.x;
      r7.z = (int)r7.w | (int)r7.z;
      r7.z = (int)r19.y | (int)r7.z;
      if (r7.z != 0) {
        r7.z = 1;
      } else {
        r19.z = 2;
        r7.zw = float2(0,0);
        while (true) {
          r8.w = cmp((int)r7.w >= 10);
          if (r8.w != 0) break;
          r19.xy = saturate(icb[r7.w+4].xy * invShadowSize_g.xy + r15.xy);
          r8.w = shadowMaps.SampleCmpLevelZero(SmplShadow_s, r19.xyz, r15.z).x;
          r7.z = r8.w * 0.100000001 + r7.z;
          r7.w = (int)r7.w + 1;
        }
      }
      r7.w = cmp(r6.w < shadowSplitDistance_g.y);
      if (r7.w != 0) {
        r15.x = dot(r3.xyzw, shadowMtx_g[1]._m00_m10_m20_m30);
        r15.y = dot(r3.xyzw, shadowMtx_g[1]._m01_m11_m21_m31);
        r15.z = dot(r3.xyzw, shadowMtx_g[1]._m02_m12_m22_m32);
        r7.w = dot(r3.xyzw, shadowMtx_g[1]._m03_m13_m23_m33);
        r15.xyz = r15.xyz / r7.www;
        r19.z = 1;
        r7.w = 0;
        r8.w = 0;
        while (true) {
          r15.w = cmp((int)r8.w >= 10);
          if (r15.w != 0) break;
          r20.xy = icb[r8.w+4].xy * invShadowSize_g.xy;
          r19.xy = saturate(r20.xy * float2(1.125,1.125) + r15.xy);
          r15.w = shadowMaps.SampleCmpLevelZero(SmplShadow_s, r19.xyz, r15.z).x;
          r7.w = r15.w * 0.100000001 + r7.w;
          r8.w = (int)r8.w + 1;
        }
        r8.w = shadowSplitDistance_g.y + -r6.w;
        r8.w = 0.200000003 * r8.w;
        r7.w = r7.w + -r7.z;
        r7.z = r8.w * r7.w + r7.z;
      }
      r7.w = -shadowFadeNear_g + r6.w;
      r7.w = saturate(shadowFadeRangeInv_g * r7.w);
      r8.w = 1 + -r7.z;
      r7.z = r7.w * r8.w + r7.z;
    } else {
      r7.w = cmp(r6.w < shadowSplitDistance_g.x);
      r15.xyz = (r7.w != 0) ? float3(0,0,0) : float3(1,4,1);
      r19.x = dot(r3.xyzw, shadowMtx_g[r15.y/4]._m00_m10_m20_m30);
      r19.y = dot(r3.xyzw, shadowMtx_g[r15.y/4]._m01_m11_m21_m31);
      r19.z = dot(r3.xyzw, shadowMtx_g[r15.y/4]._m02_m12_m22_m32);
      r8.w = dot(r3.xyzw, shadowMtx_g[r15.y/4]._m03_m13_m23_m33);
      r19.xyz = r19.xyz / r8.www;
      r8.w = dot(float2(1.25,1.125), icb[r15.x+0].xy);
      r7.z = 0;
      r15.w = 0;
      while (true) {
        r16.w = cmp((int)r15.w >= 10);
        if (r16.w != 0) break;
        r20.xy = icb[r15.w+4].xy * invShadowSize_g.xy;
        r15.xy = saturate(r20.xy * r8.ww + r19.xy);
        r15.x = shadowMaps.SampleCmpLevelZero(SmplShadow_s, r15.xyz, r19.z).x;
        r7.z = r15.x * 0.100000001 + r7.z;
        r15.w = (int)r15.w + 1;
      }
      r8.w = shadowSplitDistance_g.x + -5;
      r8.w = cmp(r8.w < r6.w);
      r7.w = r7.w ? r8.w : 0;
      if (r7.w != 0) {
        r15.x = dot(r3.xyzw, shadowMtx_g[1]._m00_m10_m20_m30);
        r15.y = dot(r3.xyzw, shadowMtx_g[1]._m01_m11_m21_m31);
        r15.z = dot(r3.xyzw, shadowMtx_g[1]._m02_m12_m22_m32);
        r7.w = dot(r3.xyzw, shadowMtx_g[1]._m03_m13_m23_m33);
        r15.xyz = r15.xyz / r7.www;
        r19.z = 1;
        r7.w = 0;
        r8.w = 0;
        while (true) {
          r15.w = cmp((int)r8.w >= 10);
          if (r15.w != 0) break;
          r20.xy = icb[r8.w+4].xy * invShadowSize_g.xy;
          r19.xy = saturate(r20.xy * float2(1.125,1.125) + r15.xy);
          r15.w = shadowMaps.SampleCmpLevelZero(SmplShadow_s, r19.xyz, r15.z).x;
          r7.w = r15.w * 0.100000001 + r7.w;
          r8.w = (int)r8.w + 1;
        }
        r6.w = shadowSplitDistance_g.x + -r6.w;
        r6.w = 0.200000003 * r6.w;
        r8.w = r7.z + -r7.w;
        r7.z = r6.w * r8.w + r7.w;
      }
    }
  } else {
    r7.z = 1;
  }
  r7.z = 1 + -saturate(r7.z);
  r7.z = r7.z * car_shadow_scale;
  r7.z = 1 + -r7.z;
  r7.z = saturate(r7.z);
  r15.xyz = r16.xyz * r1.yyy + -lightDirection_g.xyz;
  r6.w = dot(r15.xyz, r15.xyz);
  r6.w = rsqrt(r6.w);
  r15.xyz = r15.xyz * r6.www;
  r6.w = lightSpecularGlossiness_g * r2.w;
  r7.w = saturate(dot(r15.xyz, r4.xyw));
  r6.w = max(0.00100000005, r6.w);
  r7.w = log2(r7.w);
  r6.w = r7.w * r6.w;
  r6.w = exp2(r6.w);
  r6.w = r6.w * r7.z;
  r6.w = lightSpecularIntensity_g * r6.w;
  r6.w = r18.y ? r6.w : 0;
  r9.xyz = r6.www * r9.xyz;
  r9.xyz = lightColor_g.xyz * r9.xyz;
  r9.xyz = car_specular_scale * r9.xyz;
  r6.w = cmp((int)r18.z != 0);
  r4.z = r4.z ? r6.w : 0;
  if (r4.z != 0) {
    r15.xy = resolutionScaling_g.xy * v1.zw;
    r15.xyzw = texSSRMap_g.SampleLevel(SmplLinearClamp_s, r15.xy, 0).xyzw;
    r15.w = saturate(r15.w * car_ssr_scale);
  } else {
    r15.xyzw = float4(0,0,0,0);
  }
  if (r18.w != 0) {
    r18.xz = r13.yz * r5.yz;
    r18.x = r18.x * car_reflection_scale;
    r4.z = (int)r2.y & 32;
    if (r4.z != 0) {
      r19.y = resolutionScaling_g.y + -v1.y;
      r19.x = v1.x;
      r19.xyz = texMirror_g.SampleLevel(SmplLinearClamp_s, r19.xy, 0).xyz;
    } else {
      r5.y = r6.z + r6.z;
      r20.xyz = r4.xyw * -r5.yyy + r17.xyz;
      texEnvMap_g.GetDimensions(0, uiDest.x, uiDest.y, uiDest.z);
      r5.y = uiDest.z;
      r20.xyz = float3(1,-1,-1) * r20.xyz;
      r5.y = (int)r5.y + -1;
      r5.y = (uint)r5.y;
      r5.y = r18.z * r5.y;
      r5.y = car_cubemap_mip_scale * r5.y;
      r19.xyz = texEnvMap_g.SampleLevel(SmplCube_s, r20.xyz, r5.y).xyz;
      r19.xyz = car_cubemap_brightness * r19.xyz;
    }
    r20.xyz = -r19.xyz + r15.xyz;
    r20.xyz = r15.www * r20.xyz + r19.xyz;
    r19.xyz = (r1.x != 0) ? r20.xyz : r19.xyz;
    r5.y = cmp(0 < r2.z);
    r6.w = 1 + -abs(r6.z);
    r6.w = max(0, r6.w);
    r6.w = log2(r6.w);
    r2.z = r6.w * r2.z;
    r2.z = exp2(r2.z);
    r2.z = r5.y ? r2.z : 1;
    r5.y = r18.x * r2.z;
    r20.xyz = r0.xyz * r19.xyz + -r0.xyz;
    r0.xyz = r5.yyy * r20.xyz + r0.xyz;
    r5.y = dot(r19.xyz, float3(0.298999995,0.587000012,0.114));
    r6.w = r18.z * -9 + 10;
    r5.y = log2(r5.y);
    r5.y = r6.w * r5.y;
    r5.y = exp2(r5.y);
    r6.w = 1 + -r5.y;
    r5.y = r18.x * r6.w + r5.y;
    r18.xzw = r19.xyz * r5.yyy;
    r18.xzw = r18.xzw * r2.zzz;
    r2.z = -r5.z * r13.z + 1;
    r18.xzw = r18.xzw * r2.zzz;
    r2.z = 1 + -r15.w;
    r18.xzw = r18.xzw * r2.zzz + r9.xyz;
    r9.xyz = (r4.z != 0) ? r9.xyz : r18.xzw;
  } else {
    r2.z = (int)r2.y & 8;
    if (r2.z != 0) {
      r18.x = deferredParams_g[r2.x].cryFresnel;
      r18.z = deferredParams_g[r2.x].cryBrightness;
      r18.w = deferredParams_g[r2.x].cryBrightnessPower;
      r18.x = r18.x * car_reflection_scale;
      r2.x = r6.z + r6.z;
      r19.xyz = r4.xyw * -r2.xxx + r17.xyz;
      r2.x = 1 / r13.w;
      r2.z = dot(-r17.xyz, r4.xyw);
      r4.z = r2.x * r2.x;
      r5.y = -r2.z * r2.z + 1;
      r4.z = -r4.z * r5.y + 1;
      r5.y = sqrt(r4.z);
      r2.z = r2.x * r2.z + r5.y;
      r4.z = cmp(r4.z >= 0);
      r20.xyz = r2.zzz * r4.xyw;
      r17.xyz = r2.xxx * -r17.xyz + -r20.xyz;
      r17.xyz = (r4.z != 0) ? r17.xyz : 0;
      r2.x = r13.z * r5.z;
      texEnvMap_g.GetDimensions(0, uiDest.x, uiDest.y, uiDest.z);
      r2.z = uiDest.z;
      r19.xyz = float3(1,-1,-1) * r19.xyz;
      r2.z = (int)r2.z + -1;
      r2.z = (uint)r2.z;
      r2.x = r2.x * r2.z;
      r2.x = car_cubemap_mip_scale * r2.x;
      r19.xyz = texEnvMap_g.SampleLevel(SmplCube_s, r19.xyz, r2.x).xyz;
      r19.xyz = car_cubemap_brightness * r19.xyz;
      r15.xyz = -r19.xyz + r15.xyz;
      r15.xyz = r15.www * r15.xyz + r19.xyz;
      r15.xyz = (r1.x != 0) ? r15.xyz : r19.xyz;
      r17.xyz = texEnvMap_g.SampleLevel(SmplCube_s, r17.xyz, r2.x).xyz;
      r17.xyz = car_cubemap_brightness * r17.xyz;
      r1.x = cmp(0 < r18.x);
      r2.x = 1 + -abs(r6.z);
      r2.x = max(0, r2.x);
      r2.x = log2(r2.x);
      r2.x = r18.x * r2.x;
      r2.x = exp2(r2.x);
      r1.x = r1.x ? r2.x : 1;
      r19.xyz = r0.xyz * r15.xyz + -r0.xyz;
      r19.xyz = r1.xxx * r19.xyz + r0.xyz;
      r2.x = -r6.y * 0.00392156886 + 1;
      r7.x = r1.x * r2.x + r7.x;
      r2.x = r18.z * abs(r6.z);
      r2.x = log2(r2.x);
      r2.x = r18.w * r2.x;
      r2.x = exp2(r2.x);
      r15.xyz = r7.xxx * r15.xyz;
      r15.xyz = r15.xyz * r1.xxx;
      r1.x = -r5.z * r13.z + 1;
      r9.xyz = r15.xyz * r1.xxx + r9.xyz;
      r13.yzw = r2.xxx * r17.xyz;
      r0.xyz = r19.xyz * r13.yzw;
    }
  }
  r1.x = ssgiShadowRatio_g * r14.w;
  r13.yzw = sceneShadowColor_g.xyz * r14.xyz + -sceneShadowColor_g.xyz;
  r13.yzw = r1.xxx * r13.yzw + sceneShadowColor_g.xyz;
  r8.xyz = r13.yzw + r8.xyz;
  r8.xyz = min(float3(1,1,1), r8.xyz);
  r13.yzw = r14.xyz + -r12.xyz;
  r12.xyz = r14.www * r13.yzw + r12.xyz;
  r13.yzw = float3(1,1,1) + -r8.xyz;
  r11.xyz = r13.yzw * r11.xyz;
  r8.xyz = r11.www * r11.xyz + r8.xyz;
  r1.x = r7.x * r7.z;
  r7.xzw = float3(1,1,1) + -r8.xyz;
  r7.xzw = r1.xxx * r7.xzw + r8.xyz;
  r12.xyz = car_ambient_scale * r12.xyz;
  r7.xzw = r7.xzw * lightColor_g.xyz + r12.xyz;
  r7.xzw = car_diffuse_scale * r7.xzw;
  r1.x = min(1, r5.x);
  r5.xyz = float3(1,1,1) + -r7.xzw;
  r5.xyz = r1.xxx * r5.xyz + r7.xzw;
  r1.x = 1 + -abs(r6.z);
  r1.x = max(0, r1.x);
  r1.x = r1.x * r10.w;
  r1.x = log2(r1.x);
  r1.x = r9.w * r1.x;
  r1.x = exp2(r1.x);
  r1.x = min(1, r1.x);
  r10.xyz = car_rim_scale * r10.xyz;
  r6.yzw = r10.xyz * r1.xxx + r9.xyz;
  if (r18.y != 0) {
    r1.x = lightIndices_g[r6.x].pointLightCount;
    r1.x = min(63, (uint)r1.x);
    r7.xzw = float3(0,0,0);
    r8.xyz = float3(0,0,0);
    r2.x = 0;
    while (true) {
      r2.z = cmp((uint)r2.x >= (uint)r1.x);
      if (r2.z != 0) break;
      r2.z = (uint)r2.x << 2;
      r2.z = lightIndices_g[r6.x].pointLightIndices[(int)r2.x];
      r9.x = dynamicLights_g[r2.z].pos.x;
      r9.y = dynamicLights_g[r2.z].pos.y;
      r9.z = dynamicLights_g[r2.z].pos.z;
      r9.xyz = r9.xyz + -r3.xyz;
      r4.z = dot(r9.xyz, r9.xyz);
      r8.w = sqrt(r4.z);
      r9.w = dynamicLights_g[r2.z].radiusInv;
      r8.w = r9.w * r8.w;
      r9.w = dynamicLights_g[r2.z].attenuation;
      r8.w = log2(abs(r8.w));
      r8.w = r9.w * r8.w;
      r8.w = exp2(r8.w);
      r8.w = 1 + -r8.w;
      r8.w = max(0, r8.w);
      r9.w = cmp(0 < r8.w);
      if (r9.w != 0) {
        r4.z = rsqrt(r4.z);
        r9.xyz = r9.xyz * r4.zzz;
        r4.z = dynamicLights_g[r2.z].translucency;
        r9.w = dot(r9.xyz, r4.xyw);
        r4.z = max(r9.w, r4.z);
        r4.z = r8.w * r4.z;
        r10.x = dynamicLights_g[r2.z].color.x;
        r10.y = dynamicLights_g[r2.z].color.y;
        r10.z = dynamicLights_g[r2.z].color.z;
        r8.xyz = r10.xyz * r4.zzz + r8.xyz;
        r9.xyz = r16.xyz * r1.yyy + r9.xyz;
        r8.w = dot(r9.xyz, r9.xyz);
        r8.w = rsqrt(r8.w);
        r9.xyz = r9.xyz * r8.www;
        r11.x = dynamicLights_g[r2.z].specularIntensity;
        r11.y = dynamicLights_g[r2.z].specularGlossiness;
        r2.z = r11.y * r2.w;
        r8.w = saturate(dot(r9.xyz, r4.xyw));
        r2.z = max(0.00100000005, r2.z);
        r8.w = log2(r8.w);
        r2.z = r8.w * r2.z;
        r2.z = exp2(r2.z);
        r9.xyz = r10.xyz * r2.zzz;
        r9.xyz = r9.xyz * r4.zzz;
        r7.xzw = r9.xyz * r11.xxx + r7.xzw;
      }
      r2.x = (int)r2.x + 1;
    }
    r8.xyz = r8.xyz * r12.www + r5.xyz;
    r1.x = lightIndices_g[r6.x].spotLightCount;
    r1.x = min(63, (uint)r1.x);
    r9.xyz = r7.xzw;
    r10.xyz = float3(0,0,0);
    r2.x = 0;
    while (true) {
      r2.z = cmp((uint)r2.x >= (uint)r1.x);
      if (r2.z != 0) break;
      r2.z = (uint)r2.x << 2;
      r2.z = (int)r2.z + 256;
      r2.z = lightIndices_g[r6.x].spotLightIndices[(int)r2.x];
      r11.x = dynamicLights_g[r2.z].pos.x;
      r11.y = dynamicLights_g[r2.z].pos.y;
      r11.z = dynamicLights_g[r2.z].pos.z;
      r11.xyz = r11.xyz + -r3.xyz;
      r4.z = dot(r11.xyz, r11.xyz);
      r8.w = rsqrt(r4.z);
      r11.xyz = r11.xyz * r8.www;
      r15.x = dynamicLights_g[r2.z].vec.x;
      r15.y = dynamicLights_g[r2.z].vec.y;
      r15.z = dynamicLights_g[r2.z].vec.z;
      r15.w = dynamicLights_g[r2.z].spotAngleInv;
      r8.w = dot(r11.xyz, r15.xyz);
      r8.w = max(0, r8.w);
      r8.w = 1 + -r8.w;
      r8.w = r8.w * r15.w;
      r9.w = dynamicLights_g[r2.z].attenuationAngle;
      r8.w = log2(r8.w);
      r8.w = r9.w * r8.w;
      r8.w = exp2(r8.w);
      r8.w = 1 + -r8.w;
      r8.w = max(0, r8.w);
      r9.w = cmp(0 < r8.w);
      if (r9.w != 0) {
        r4.z = sqrt(r4.z);
        r9.w = dynamicLights_g[r2.z].radiusInv;
        r4.z = r9.w * r4.z;
        r9.w = dynamicLights_g[r2.z].attenuation;
        r4.z = log2(abs(r4.z));
        r4.z = r9.w * r4.z;
        r4.z = exp2(r4.z);
        r4.z = 1 + -r4.z;
        r4.z = max(0, r4.z);
        r4.z = r8.w * r4.z;
        r8.w = cmp(0 < r4.z);
        if (r8.w != 0) {
          r12.x = dynamicLights_g[r2.z].translucency;
          r12.y = dynamicLights_g[r2.z].shadowmapIndex;
          r8.w = cmp((int)r12.y != -1);
          if (r8.w != 0) {
            r15.xyzw = spotShadowMatrices_g[r12.y]._m00_m10_m20_m30;
            r17.xyzw = spotShadowMatrices_g[r12.y]._m01_m11_m21_m31;
            r18.xyzw = spotShadowMatrices_g[r12.y]._m02_m12_m22_m32;
            r19.xyzw = spotShadowMatrices_g[r12.y]._m03_m13_m23_m33;
            r15.x = dot(r3.xyzw, r15.xyzw);
            r15.y = dot(r3.xyzw, r17.xyzw);
            r15.z = dot(r3.xyzw, r18.xyzw);
            r8.w = dot(r3.xyzw, r19.xyzw);
            r15.xyz = r15.xyz / r8.www;
            r15.w = (uint)r12.y;
            r8.w = spotShadowMaps.SampleCmpLevelZero(SmplShadow_s, r15.xyw, r15.z).x;
            r13.yzw = float3(0.00244140625,0,0) + r15.xyw;
            r9.w = spotShadowMaps.SampleCmpLevelZero(SmplShadow_s, r13.yzw, r15.z).x;
            r9.w = 0.200000003 * r9.w;
            r8.w = r8.w * 0.200000003 + r9.w;
            r13.yzw = float3(-0.00244140625,0,0) + r15.xyw;
            r9.w = spotShadowMaps.SampleCmpLevelZero(SmplShadow_s, r13.yzw, r15.z).x;
            r8.w = r9.w * 0.200000003 + r8.w;
            r13.yzw = float3(0,0.00244140625,0) + r15.xyw;
            r9.w = spotShadowMaps.SampleCmpLevelZero(SmplShadow_s, r13.yzw, r15.z).x;
            r8.w = r9.w * 0.200000003 + r8.w;
            r13.yzw = float3(0,-0.00244140625,0) + r15.xyw;
            r9.w = spotShadowMaps.SampleCmpLevelZero(SmplShadow_s, r13.yzw, r15.z).x;
            r8.w = r9.w * 0.200000003 + r8.w;
            r4.z = r8.w * r4.z;
          }
          r8.w = dot(r11.xyz, r4.xyw);
          r8.w = max(r12.x, r8.w);
          r4.z = r8.w * r4.z;
          r12.x = dynamicLights_g[r2.z].color.x;
          r12.y = dynamicLights_g[r2.z].color.y;
          r12.z = dynamicLights_g[r2.z].color.z;
          r10.xyz = r12.xyz * r4.zzz + r10.xyz;
          r11.xyz = r16.xyz * r1.yyy + r11.xyz;
          r8.w = dot(r11.xyz, r11.xyz);
          r8.w = rsqrt(r8.w);
          r11.xyz = r11.xyz * r8.www;
          r13.y = dynamicLights_g[r2.z].specularIntensity;
          r13.z = dynamicLights_g[r2.z].specularGlossiness;
          r2.z = r13.z * r2.w;
          r8.w = saturate(dot(r11.xyz, r4.xyw));
          r2.z = max(0.00100000005, r2.z);
          r8.w = log2(r8.w);
          r2.z = r8.w * r2.z;
          r2.z = exp2(r2.z);
          r11.xyz = r12.xyz * r2.zzz;
          r11.xyz = r11.xyz * r4.zzz;
          r9.xyz = r11.xyz * r13.yyy + r9.xyz;
        }
      }
      r2.x = (int)r2.x + 1;
    }
    r2.xzw = r10.xyz * r12.www + r8.xyz;
    r6.yzw = r9.xyz * r12.www + r6.yzw;
  } else {
    r1.x = lightIndices_g[r6.x].pointLightCount;
    r1.x = min(63, (uint)r1.x);
    r8.xyzw = float4(0,0,0,0);
    while (true) {
      r1.y = cmp((uint)r8.w >= (uint)r1.x);
      if (r1.y != 0) break;
      r1.y = (uint)r8.w << 2;
      r1.y = lightIndices_g[r6.x].pointLightIndices[(int)r8.w];
      r7.x = dynamicLights_g[r1.y].pos.x;
      r7.z = dynamicLights_g[r1.y].pos.y;
      r7.w = dynamicLights_g[r1.y].pos.z;
      r7.xzw = r7.xzw + -r3.xyz;
      r4.z = dot(r7.xzw, r7.xzw);
      r9.x = sqrt(r4.z);
      r9.y = dynamicLights_g[r1.y].radiusInv;
      r9.x = r9.x * r9.y;
      r9.y = dynamicLights_g[r1.y].attenuation;
      r9.x = log2(abs(r9.x));
      r9.x = r9.y * r9.x;
      r9.x = exp2(r9.x);
      r9.x = 1 + -r9.x;
      r9.x = max(0, r9.x);
      r9.y = cmp(0 < r9.x);
      if (r9.y != 0) {
        r9.y = dynamicLights_g[r1.y].translucency;
        r4.z = rsqrt(r4.z);
        r7.xzw = r7.xzw * r4.zzz;
        r4.z = dot(r7.xzw, r4.xyw);
        r4.z = max(r9.y, r4.z);
        r7.x = dynamicLights_g[r1.y].color.x;
        r7.z = dynamicLights_g[r1.y].color.y;
        r7.w = dynamicLights_g[r1.y].color.z;
        r7.xzw = r7.xzw * r9.xxx;
        r8.xyz = r7.xzw * r4.zzz + r8.xyz;
      }
      r8.w = (int)r8.w + 1;
    }
    r5.xyz = r8.xyz * r12.www + r5.xyz;
    r1.x = lightIndices_g[r6.x].spotLightCount;
    r1.x = min(63, (uint)r1.x);
    r8.xyzw = float4(0,0,0,0);
    while (true) {
      r1.y = cmp((uint)r8.w >= (uint)r1.x);
      if (r1.y != 0) break;
      r1.y = (uint)r8.w << 2;
      r1.y = (int)r1.y + 256;
      r1.y = lightIndices_g[r6.x].spotLightIndices[(int)r8.w];
      r7.x = dynamicLights_g[r1.y].pos.x;
      r7.z = dynamicLights_g[r1.y].pos.y;
      r7.w = dynamicLights_g[r1.y].pos.z;
      r7.xzw = r7.xzw + -r3.xyz;
      r4.z = dot(r7.xzw, r7.xzw);
      r9.x = rsqrt(r4.z);
      r7.xzw = r9.xxx * r7.xzw;
      r9.x = dynamicLights_g[r1.y].vec.x;
      r9.y = dynamicLights_g[r1.y].vec.y;
      r9.z = dynamicLights_g[r1.y].vec.z;
      r9.w = dynamicLights_g[r1.y].spotAngleInv;
      r9.x = dot(r7.xzw, r9.xyz);
      r9.x = max(0, r9.x);
      r9.x = 1 + -r9.x;
      r9.x = r9.x * r9.w;
      r9.y = dynamicLights_g[r1.y].attenuationAngle;
      r9.x = log2(r9.x);
      r9.x = r9.y * r9.x;
      r9.x = exp2(r9.x);
      r9.x = 1 + -r9.x;
      r9.x = max(0, r9.x);
      r9.y = cmp(0 < r9.x);
      if (r9.y != 0) {
        r4.z = sqrt(r4.z);
        r9.y = dynamicLights_g[r1.y].radiusInv;
        r4.z = r9.y * r4.z;
        r9.y = dynamicLights_g[r1.y].attenuation;
        r4.z = log2(abs(r4.z));
        r4.z = r9.y * r4.z;
        r4.z = exp2(r4.z);
        r4.z = 1 + -r4.z;
        r4.z = max(0, r4.z);
        r4.z = r9.x * r4.z;
        r9.x = cmp(0 < r4.z);
        if (r9.x != 0) {
          r9.x = dynamicLights_g[r1.y].translucency;
          r9.y = dynamicLights_g[r1.y].shadowmapIndex;
          r9.z = cmp((int)r9.y != -1);
          if (r9.z != 0) {
            r10.xyzw = spotShadowMatrices_g[r9.y]._m00_m10_m20_m30;
            r11.xyzw = spotShadowMatrices_g[r9.y]._m01_m11_m21_m31;
            r15.xyzw = spotShadowMatrices_g[r9.y]._m02_m12_m22_m32;
            r16.xyzw = spotShadowMatrices_g[r9.y]._m03_m13_m23_m33;
            r10.x = dot(r3.xyzw, r10.xyzw);
            r10.y = dot(r3.xyzw, r11.xyzw);
            r10.z = dot(r3.xyzw, r15.xyzw);
            r9.z = dot(r3.xyzw, r16.xyzw);
            r10.xyz = r10.xyz / r9.zzz;
            r10.w = (uint)r9.y;
            r9.y = spotShadowMaps.SampleCmpLevelZero(SmplShadow_s, r10.xyw, r10.z).x;
            r11.xyz = float3(0.00244140625,0,0) + r10.xyw;
            r9.z = spotShadowMaps.SampleCmpLevelZero(SmplShadow_s, r11.xyz, r10.z).x;
            r9.z = 0.200000003 * r9.z;
            r9.y = r9.y * 0.200000003 + r9.z;
            r11.xyz = float3(-0.00244140625,0,0) + r10.xyw;
            r9.z = spotShadowMaps.SampleCmpLevelZero(SmplShadow_s, r11.xyz, r10.z).x;
            r9.y = r9.z * 0.200000003 + r9.y;
            r11.xyz = float3(0,0.00244140625,0) + r10.xyw;
            r9.z = spotShadowMaps.SampleCmpLevelZero(SmplShadow_s, r11.xyz, r10.z).x;
            r9.y = r9.z * 0.200000003 + r9.y;
            r10.xyw = float3(0,-0.00244140625,0) + r10.xyw;
            r9.z = spotShadowMaps.SampleCmpLevelZero(SmplShadow_s, r10.xyw, r10.z).x;
            r9.y = r9.z * 0.200000003 + r9.y;
            r4.z = r9.y * r4.z;
          }
          r7.x = dot(r7.xzw, r4.xyw);
          r7.x = max(r9.x, r7.x);
          r9.x = dynamicLights_g[r1.y].color.x;
          r9.y = dynamicLights_g[r1.y].color.y;
          r9.z = dynamicLights_g[r1.y].color.z;
          r9.xyz = r9.xyz * r4.zzz;
          r8.xyz = r9.xyz * r7.xxx + r8.xyz;
        }
      }
      r8.w = (int)r8.w + 1;
    }
    r2.xzw = r8.xyz * r12.www + r5.xyz;
  }
  r2.xzw = lerp(float3(1,1,1), r2.xzw, car_local_light_scale);
  r6.yzw = car_local_light_scale * r6.yzw;
  r4.xyz = r6.yzw * r7.yyy;
  r0.xyz = r0.xyz * r2.xzw + r4.xyz;
  r2.xzw = min(float3(1,1,1), r14.xyz);
  r2.xzw = -mapAOColor_g.xyz + r2.xzw;
  r2.xzw = r14.www * r2.xzw + mapAOColor_g.xyz;
  r0.w = 1 + -r0.w;
  r2.xzw = r0.xyz * r2.xzw + -r0.xyz;
  r0.xyz = r0.www * r2.xzw + r0.xyz;
  r0.w = -fogNearDistance_g + -r5.w;
  r0.w = saturate(fogFadeRangeInv_g * r0.w);
  r1.x = -fogHeight_g + r3.y;
  r1.x = saturate(fogHeightRangeInv_g * r1.x);
  r0.w = r1.x * r0.w;
  r0.w = fogIntensity_g * r0.w;
  r1.x = -r1.z * 0.00392156886 + 1;
  r1.x = r1.w ? r1.x : 1;
  r0.w = r0.w * r13.x;
  r0.w = r0.w * r1.x;
  r1.xyz = fogColor_g.xyz + -r0.xyz;
  r0.xyz = r0.www * r1.xyz + r0.xyz;
  r0.w = (int)r2.y & 64;
  r0.w = cmp((int)r0.w == 0);
  r1.x = cmp(0 != isEnableSky_g);
  r0.w = r0.w ? r1.x : 0;
  if (r0.w != 0) {
    r0.w = -r5.w / skyLutCameraFarClip_g;
    r1.xy = invVPSize_g.xy * v0.xy;
    r0.w = -skyLutNearOverFarClip_g + r0.w;
    r1.w = -skyLutNearOverFarClip_g + 1;
    r1.z = r0.w / r1.w;
    r2.xyz = atmosphereInscatterLUT.SampleLevel(samLinear_s, r1.xyz, 0).xyz;
    r1.xyz = atmosphereExtinctionLUT.SampleLevel(samLinear_s, r1.xyz, 0).xyz;
    r0.xyz = r0.xyz * r1.xyz + r2.xyz;
  }
  if (outlineShapeCount_g != 0) {
    r1.xy = -outlineShapeMaskUVParam_g.xy + r3.xz;
    r1.xy = outlineShapeMaskUVParam_g.zw * r1.xy;
    r1.zw = cmp(r1.xy >= float2(0,0));
    r0.w = r1.w ? r1.z : 0;
    r1.zw = cmp(float2(1,1) >= r1.xy);
    r1.z = r1.w ? r1.z : 0;
    r0.w = r1.z ? r0.w : 0;
    if (r0.w != 0) {
      r0.w = outlineShapeMask.SampleLevel(samLinear_s, r1.xy, 0).x;
    } else {
      r0.w = 1;
    }
    r3.w = 1;
    r1.xyz = r0.xyz;
    r1.w = 0;
    while (true) {
      r2.x = cmp((uint)r1.w >= outlineShapeCount_g);
      if (r2.x != 0) break;
      r2.x = outlineShapes_g[r1.w].radius;
      r2.y = outlineShapes_g[r1.w].height_base;
      r2.z = outlineShapes_g[r1.w].height_width;
      r2.w = outlineShapes_g[r1.w].height_gradation_width;
      r2.y = r3.y + -r2.y;
      r4.x = cmp(r2.z >= abs(r2.y));
      if (r4.x != 0) {
        r4.x = outlineShapes_g[r1.w].mtx._m00;
        r4.y = outlineShapes_g[r1.w].mtx._m10;
        r4.z = outlineShapes_g[r1.w].mtx._m20;
        r4.w = outlineShapes_g[r1.w].mtx._m30;
        r5.x = outlineShapes_g[r1.w].mtx._m02;
        r5.y = outlineShapes_g[r1.w].mtx._m32;
        r5.z = outlineShapes_g[r1.w].mtx._m12;
        r5.w = outlineShapes_g[r1.w].mtx._m22;
        r6.x = outlineShapes_g[r1.w].color.x;
        r6.y = outlineShapes_g[r1.w].color.y;
        r6.z = outlineShapes_g[r1.w].color.z;
        r6.w = outlineShapes_g[r1.w].color.w;
        r7.x = outlineShapes_g[r1.w].gradation_size.x;
        r7.y = outlineShapes_g[r1.w].gradation_size.y;
        r7.z = outlineShapes_g[r1.w].gradation_sharpness;
        r7.w = outlineShapes_g[r1.w].type;
        r8.x = abs(r2.y) + r2.w;
        r8.x = cmp(r2.z < r8.x);
        r2.y = r2.z + -abs(r2.y);
        r2.y = r2.y / r2.w;
        r2.y = r8.x ? r2.y : 1;
        if (r7.w == 0) {
          r5.x = r4.w;
          r2.zw = -r5.xy + r3.xz;
          r2.z = dot(r2.zw, r2.zw);
          r2.z = sqrt(r2.z);
          r2.w = cmp(r2.x < r2.z);
          r8.x = -r7.x + r2.x;
          r8.y = cmp(r2.z >= r8.x);
          r8.z = ~(int)r8.y;
          r2.z = -r8.x + r2.z;
          r2.z = r2.z / r7.x;
          r2.z = r8.y ? r2.z : 0;
          r2.z = r2.w ? 0 : r2.z;
          r2.w = (int)r2.w | (int)r8.z;
        } else {
          r8.x = cmp((int)r7.w == 1);
          if (r8.x != 0) {
            r8.x = outlineShapes_g[r1.w].fan_angle;
            r5.xz = r4.wz;
            r8.yz = -r5.xy + r3.xz;
            r8.w = dot(r8.yz, r8.yz);
            r9.x = sqrt(r8.w);
            r9.y = cmp(r2.x < r9.x);
            r2.x = -r7.x + r2.x;
            r8.w = rsqrt(r8.w);
            r8.yz = r8.yz * r8.ww;
            r8.y = dot(r5.zw, r8.yz);
            r8.z = 1 + -abs(r8.y);
            r8.z = sqrt(r8.z);
            r8.w = abs(r8.y) * -0.0187292993 + 0.0742610022;
            r8.w = r8.w * abs(r8.y) + -0.212114394;
            r8.w = r8.w * abs(r8.y) + 1.57072878;
            r9.z = r8.w * r8.z;
            r9.z = r9.z * -2 + 3.14159274;
            r8.y = cmp(r8.y < -r8.y);
            r8.y = r8.y ? r9.z : 0;
            r8.y = r8.w * r8.z + r8.y;
            r8.z = cmp(r8.x >= r8.y);
            r8.w = ~(int)r8.z;
            r9.z = 6.28318548 * r9.x;
            r8.xy = r9.zz * r8.xy;
            r8.y = 0.159154937 * r8.y;
            r8.x = r8.x * 0.159154937 + -r8.y;
            r8.x = r8.x / r7.x;
            r8.x = min(1, r8.x);
            r8.x = 1 + -r8.x;
            r2.x = r9.x + -r2.x;
            r2.x = r2.x / r7.x;
            r2.x = min(1, r2.x);
            r2.x = max(r8.x, r2.x);
            r2.x = r8.z ? r2.x : 0;
            r2.z = r9.y ? 0 : r2.x;
            r2.w = (int)r9.y | (int)r8.w;
          } else {
            r2.x = cmp((int)r7.w == 2);
            r4.x = dot(r3.xyzw, r4.xyzw);
            r4.y = dot(r3.xwyz, r5.xyzw);
            r4.zw = cmp(r4.xy < float2(0.5,0.5));
            r4.z = r4.w ? r4.z : 0;
            r5.xy = cmp(float2(-0.5,-0.5) < r4.xy);
            r4.w = r5.y ? r5.x : 0;
            r4.z = r4.w ? r4.z : 0;
            r5.xy = float2(0.5,0.5) + -r7.xy;
            r5.zw = cmp(abs(r4.xy) < r5.xy);
            r4.w = r5.w ? r5.z : 0;
            r4.xy = -r5.xy + abs(r4.xy);
            r4.xy = r4.xy / r7.xy;
            r4.x = max(r4.x, r4.y);
            r4.x = r4.w ? 0 : r4.x;
            r4.x = r4.z ? r4.x : 0;
            r4.y = r4.z ? r4.w : -1;
            r2.zw = (r2.x != 0) ? r4.xy : 0;
          }
        }
        r2.x = log2(r2.z);
        r2.x = r7.z * r2.x;
        r2.x = exp2(r2.x);
        r2.x = r2.x * r6.w;
        r2.x = r2.x * r0.w;
        r6.w = r2.x * r2.y;
        r2.xyzw = (r2.w != 0) ? float4(0,0,0,0) : r6.xyzw;
      } else {
        r2.xyzw = float4(0,0,0,0);
      }
      r2.xyz = r2.xyz + -r1.xyz;
      r1.xyz = r2.www * r2.xyz + r1.xyz;
      r1.w = (int)r1.w + 1;
    }
    r0.xyz = r1.xyz;
  }
  o0.xyz = r0.xyz;
  o0.w = 1;
  return;
}
