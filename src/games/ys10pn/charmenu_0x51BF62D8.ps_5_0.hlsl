// ---- Created with 3Dmigoto v1.4.1 on Sat Feb 28 21:15:33 2026
#include "./shared.h"

struct ShaderDeferredParam
{
    float3 highlightEdgeColor;     // Offset:    0
    float highlightEdgeSensitivityLow;// Offset:   12
    float3 highlightEdgeShadowColor;// Offset:   16
    float highlightEdgeSensitivityHigh;// Offset:   28
    float highlightEdgeShadeFade;  // Offset:   32
    float highlightEdgeWeight;     // Offset:   36
    float highlightEdgeFarWeight;  // Offset:   40
    float highlightEdgeFarClipDistance;// Offset:   44
    float highlightEdgeFarClipFadeRange;// Offset:   48
    float highlightEdgeNormalLevel;// Offset:   52
    float highlightEdgeNormalSensitivity;// Offset:   56
    float highlightEdgeNormalFrontFade;// Offset:   60
    float highlightEdgeDynamicLightColorIntensity;// Offset:   64
    float highlightEdgeFogIntensity;// Offset:   68
    float outlineLevel;            // Offset:   72
    float outlineWeight;           // Offset:   76
    float3 outlineColor;           // Offset:   80
    uint outlineMode;              // Offset:   92
    uint switch_flag;              // Offset:   96
    float light_probe_exposure_correct;// Offset:  100
    float ssaoIntensity;           // Offset:  104
    float ghostTransparency;       // Offset:  108
    float materialFogIntensity;    // Offset:  112
    float metalness;               // Offset:  116
    float roughness;               // Offset:  120
    float fresnel0;                // Offset:  124
    float fresnel1;                // Offset:  128
    float fresnel2;                // Offset:  132
    float specularGlossiness0;     // Offset:  136
    float specularGlossiness1;     // Offset:  140
    float specularGlossiness2;     // Offset:  144
    float glowIntensity;           // Offset:  148
    float glowLumThreshold;        // Offset:  152
    float glowShadowFadeRatio;     // Offset:  156
    float3 shadowColor;            // Offset:  160
    float dynamicLightIntensity;   // Offset:  172
    float3 specularColor;          // Offset:  176
    float specularShadowFadeRatio; // Offset:  188
    float3 rimLightColor;          // Offset:  192
    float rimLightPower;           // Offset:  204
    float rimIntensity;            // Offset:  208
    float rimIntensityLight;       // Offset:  212
    float translucency;            // Offset:  216
    float emissive;                // Offset:  220
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

struct SceneParam
{
    float4x4 view_g;               // Offset:    0
    float4x4 view_inv_g;           // Offset:   64
    float4x4 proj_g;               // Offset:  128
    float4x4 proj_inv_g;           // Offset:  192
    float4x4 view_proj_g;          // Offset:  256
    float4x4 view_proj_inv_g;      // Offset:  320
    float4 look_at_g;              // Offset:  384
    float4 unprojs_g;              // Offset:  400
    float4x4 prevView_g;           // Offset:  416
    float4x4 prevViewInv_g;        // Offset:  480
    float4x4 prevProj_g;           // Offset:  544
    float4x4 prevProjInv_g;        // Offset:  608
    float4x4 prevViewProj_g;       // Offset:  672
    float4x4 prevViewProjInv_g;    // Offset:  736
    float2 motionJitterOffset_g;   // Offset:  800
    float2 curJitterOffset_g;      // Offset:  808
};

struct LightProbeParam
{
    float4 pos;                    // Offset:    0
    float radius;                  // Offset:   16
    float radiusInv;               // Offset:   20
    float attenuation;             // Offset:   24
    float padding;                 // Offset:   28
    float4 sh[9];                  // Offset:   32
};

struct LightIndexData
{
    uint pointLightIndices[31];    // Offset:    0
    uint pointLightCount;          // Offset:  124
    uint spotLightIndices[31];     // Offset:  128
    uint spotLightCount;           // Offset:  252
    uint lightProbeIndices[63];    // Offset:  256
    uint lightProbeCount;          // Offset:  508
};

cbuffer cb_ysx_scene : register(b0)
{
  float3 lightColor_g : packoffset(c0);
  float deltaTime_g : packoffset(c0.w);
  float3 lightDirection_g : packoffset(c1);
  uint isManaSensing_g : packoffset(c1.w);
  float4x4 farShadowMtx_g : packoffset(c2);
  float2 invFarShadowSize_g : packoffset(c6);
  float2 invShadowSize_g : packoffset(c6.z);
  float3 sceneShadowColor_g : packoffset(c7);
  float shadowFadeNear_g : packoffset(c7.w);
  float3 chrLightDir_g : packoffset(c8);
  float shadowFadeRangeInv_g : packoffset(c8.w);
  float shadowDistance_g : packoffset(c9);
  float farShadowStartDistance_g : packoffset(c9.y);
  float farShadowEndDistance_g : packoffset(c9.z);
  float sceneWaterTime_g : packoffset(c9.w);
  float4x4 ditherMtx_g : packoffset(c10);
  uint padding_c14_x : packoffset(c14);
  float sceneTime_g : packoffset(c14.y);
  float lightSpecularGlossiness_g : packoffset(c14.z);
  float lightSpecularIntensity_g : packoffset(c14.w);
  float2 resolutionScaling_g : packoffset(c15);
  float disableNearCameraAlpha_g : packoffset(c15.z);
  float sceneDeltaTime_g : packoffset(c15.w);
  float3 twoLayeredFogColorLowerNear_g : packoffset(c16);
  uint twoLayeredFogMode_g : packoffset(c16.w);
  float3 twoLayeredFogColorLowerFar_g : packoffset(c17);
  float twoLayeredFogStartDistance_g : packoffset(c17.w);
  float3 twoLayeredFogColorUpperNear_g : packoffset(c18);
  float twoLayeredFogDistanceRangeInv_g : packoffset(c18.w);
  float3 twoLayeredFogColorUpperFar_g : packoffset(c19);
  uint padding_c19_w : packoffset(c19.w);
  float2 twoLayeredFogHeightNear_g : packoffset(c20);
  float2 twoLayeredFogHeightFar_g : packoffset(c20.z);
  float2 twoLayeredFogMinIntensity_g : packoffset(c21);
  float2 twoLayeredFogMaxIntensity_g : packoffset(c21.z);
  float2 twoLayeredFogBlend_g : packoffset(c22);
  float2 twoLayeredFogDistanceCoefInv_g : packoffset(c22.z);
  float3 windDirection_g : packoffset(c23);
  float windWaveTime_g : packoffset(c23.w);
  float windWaveFrequency_g : packoffset(c24);
  float windForce_g : packoffset(c24.y);
  float seaWaveLengthScale_g : packoffset(c24.z);
  float seaWaveHeight_g : packoffset(c24.w);
  float3 seaWaveDirection_g : packoffset(c25);
  float seaWaveSpeed_g : packoffset(c25.w);
  float disableFarCameraAlpha_g : packoffset(c26);
  uint localLightProbeCount_g : packoffset(c26.y);
  float2 invTargetSize_g : packoffset(c26.z);
  float4 lightProbe_g[9] : packoffset(c27);
  float3 lightTileSizeInv_g : packoffset(c36);
  uint padding_c36_w : packoffset(c36.w);
  float4x4 waterCausticsProj_g : packoffset(c37);
  float2 invResolutionScaling_g : packoffset(c41);
  float2 resolutionUVClamp_g : packoffset(c41.z);
  float3 chara_shadow_mul_color_g : packoffset(c42);
  uint padding_c42_w : packoffset(c42.w);
  float4x4 shadow_matrices_g[3] : packoffset(c43);
  float3 shadow_split_distance_g : packoffset(c55);
  uint padding_c55_w : packoffset(c55.w);
  float2 invViewportSize_g : packoffset(c56);
  uint shadowSamplingMode_g : packoffset(c56.z);
  uint padding_c56_w : packoffset(c56.w);
  float prevSceneTime_g : packoffset(c57);
  float prevWindWaveTime_g : packoffset(c57.y);
  uint enableMotionVectors_g : packoffset(c57.z);
  uint padding_c57_w : packoffset(c57.w);
  uint padding_c58_x : packoffset(c58);
  uint padding_c58_y : packoffset(c58.y);
  uint padding_c58_z : packoffset(c58.z);
  uint padding_c58_w : packoffset(c58.w);
  uint padding_c59_x : packoffset(c59);
  uint dbgWaterCausticsOff_g : packoffset(c59.y);
  uint padding_c59_z : packoffset(c59.z);
  uint padding_c59_w : packoffset(c59.w);
}

cbuffer cb_ysx_scene_slot : register(b6)
{
  uint scene_slot_index_g : packoffset(c0);
}

SamplerState SmplCube_s : register(s14);
SamplerState SmplLinearClamp_s : register(s15);
SamplerComparisonState SmplShadow_s : register(s13);
Texture2DMS<float4> color_texture_g : register(t0);
Texture2DMS<uint4> mrt_texture0_g : register(t1);
Texture2DMS<uint4> mrt_texture1_g : register(t2);
Texture2DMS<float> depth_texture_g : register(t3);
StructuredBuffer<ShaderDeferredParam> deferred_param_g : register(t10);
StructuredBuffer<LightParam> dynamicLights_g : register(t11);
StructuredBuffer<SceneParam> scene_param_g : register(t12);
StructuredBuffer<LightProbeParam> localLightProbes_g : register(t13);
StructuredBuffer<LightIndexData> lightIndices_g : register(t15);
Texture2DArray<float4> shadowMap : register(t16);
TextureCube<float4> texEnvMap_g : register(t17);
Texture2D<float4> farShadowMap : register(t21);


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_Position0,
  float4 v1 : TEXCOORD0,
  uint v2 : SV_SampleIndex0,
  out float4 o0 : SV_Target0,
  out uint2 o1 : SV_Target1,
  out uint2 o2 : SV_Target2)
{
  float4 r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,r26,r27,r28;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.xy = (int2)v0.xy;
  r0.zw = float2(0,0);
  r1.xy = mrt_texture0_g.Load(r0.xy, v2.x).yx;
  r2.xw = mrt_texture1_g.Load(r0.xy, v2.x).xy;
  r3.xyzw = color_texture_g.Load(r0.xy, v2.x).xyzw;
  r4.xyz = (uint3)r1.yyy >> int3(8,20,27);
  r5.x = (int)r4.y & 16;
  if (r5.x != 0) {
    r4.w = r1.y;
    r5.xyzw = (int4)r4.wxyy & int4(255,4095,32,111);
    r4.x = (uint)r5.x;
    r4.x = 0.00392156886 * r4.x;
    r1.yzw = (uint3)r1.xxx >> int3(8,16,24);
    r6.xyzw = (int4)r1.xyzw & int4(255,255,255,255);
    r6.xyzw = (uint4)r6.xyzw;
    r7.xyzw = float4(0.00392156886,0.00392156886,0.00392156886,0.00392156886) * r6.xyzw;
    r8.xyw = (uint3)r2.xxx >> int3(24,28,12);
    r8.z = r2.x;
    r8.xyzw = (int4)r8.xyzw & int4(15,15,4095,4095);
    r8.xyzw = (uint4)r8.xyzw;
    r1.zw = float2(0.0666666701,0.0666666701) * r8.xy;
    r8.xy = r8.zw * float2(0.000488400517,0.000488400517) + float2(-1,-1);
    r2.xyz = (uint3)r2.www >> int3(12,16,24);
    r9.xyzw = (int4)r2.xyzw & int4(15,255,255,4095);
    r9.xyzw = (uint4)r9.xyzw;
    r6.xyw = float3(0.00392156886,0.00392156886,0.0666666701) * r9.yzx;
    r8.z = r9.w * 0.000488400517 + -1;
    r9.x = scene_param_g[scene_slot_index_g].view_g._m03;
    r9.y = scene_param_g[scene_slot_index_g].view_g._m13;
    r9.z = scene_param_g[scene_slot_index_g].view_g._m23;
    r10.x = scene_param_g[scene_slot_index_g].view_g._m00;
    r10.y = scene_param_g[scene_slot_index_g].view_g._m10;
    r10.z = scene_param_g[scene_slot_index_g].view_g._m20;
    r10.w = scene_param_g[scene_slot_index_g].view_g._m30;
    r11.x = scene_param_g[scene_slot_index_g].view_g._m01;
    r11.y = scene_param_g[scene_slot_index_g].view_g._m11;
    r11.z = scene_param_g[scene_slot_index_g].view_g._m21;
    r11.w = scene_param_g[scene_slot_index_g].view_g._m31;
    r12.x = scene_param_g[scene_slot_index_g].view_g._m02;
    r12.y = scene_param_g[scene_slot_index_g].view_g._m12;
    r12.z = scene_param_g[scene_slot_index_g].view_g._m22;
    r12.w = scene_param_g[scene_slot_index_g].view_g._m32;
    r13.x = scene_param_g[scene_slot_index_g].view_inv_g._m30;
    r13.y = scene_param_g[scene_slot_index_g].view_inv_g._m31;
    r13.z = scene_param_g[scene_slot_index_g].view_inv_g._m32;
    r2.y = deferred_param_g[r5.y].switch_flag;
    r14.x = deferred_param_g[r5.y].materialFogIntensity;
    r14.y = deferred_param_g[r5.y].metalness;
    r14.z = deferred_param_g[r5.y].roughness;
    r14.w = deferred_param_g[r5.y].fresnel0;
    r15.x = deferred_param_g[r5.y].fresnel1;
    r15.y = deferred_param_g[r5.y].fresnel2;
    r15.z = deferred_param_g[r5.y].specularGlossiness0;
    r15.w = deferred_param_g[r5.y].specularGlossiness1;
    r16.x = deferred_param_g[r5.y].specularGlossiness2;
    r16.y = deferred_param_g[r5.y].glowIntensity;
    r16.z = deferred_param_g[r5.y].glowLumThreshold;
    r16.w = deferred_param_g[r5.y].glowShadowFadeRatio;
    r17.x = deferred_param_g[r5.y].shadowColor.x;
    r17.y = deferred_param_g[r5.y].shadowColor.y;
    r17.z = deferred_param_g[r5.y].shadowColor.z;
    r17.w = deferred_param_g[r5.y].dynamicLightIntensity;
    r18.x = deferred_param_g[r5.y].specularColor.x;
    r18.y = deferred_param_g[r5.y].specularColor.y;
    r18.z = deferred_param_g[r5.y].specularColor.z;
    r18.w = deferred_param_g[r5.y].specularShadowFadeRatio;
    r19.x = deferred_param_g[r5.y].rimLightColor.x;
    r19.y = deferred_param_g[r5.y].rimLightColor.y;
    r19.z = deferred_param_g[r5.y].rimLightColor.z;
    r19.w = deferred_param_g[r5.y].rimLightPower;
    r20.x = deferred_param_g[r5.y].rimIntensity;
    r20.y = deferred_param_g[r5.y].rimIntensityLight;
    r20.z = deferred_param_g[r5.y].translucency;
    r20.w = deferred_param_g[r5.y].emissive;
    r0.z = depth_texture_g.Load(r0.xy, v2.x).x;
    r21.x = scene_param_g[scene_slot_index_g].view_proj_inv_g._m00;
    r21.y = scene_param_g[scene_slot_index_g].view_proj_inv_g._m10;
    r21.z = scene_param_g[scene_slot_index_g].view_proj_inv_g._m20;
    r21.w = scene_param_g[scene_slot_index_g].view_proj_inv_g._m30;
    r22.x = scene_param_g[scene_slot_index_g].view_proj_inv_g._m01;
    r22.y = scene_param_g[scene_slot_index_g].view_proj_inv_g._m11;
    r22.z = scene_param_g[scene_slot_index_g].view_proj_inv_g._m21;
    r22.w = scene_param_g[scene_slot_index_g].view_proj_inv_g._m31;
    r23.x = scene_param_g[scene_slot_index_g].view_proj_inv_g._m02;
    r23.y = scene_param_g[scene_slot_index_g].view_proj_inv_g._m12;
    r23.z = scene_param_g[scene_slot_index_g].view_proj_inv_g._m22;
    r23.w = scene_param_g[scene_slot_index_g].view_proj_inv_g._m32;
    r24.x = scene_param_g[scene_slot_index_g].view_proj_inv_g._m03;
    r24.y = scene_param_g[scene_slot_index_g].view_proj_inv_g._m13;
    r24.z = scene_param_g[scene_slot_index_g].view_proj_inv_g._m23;
    r24.w = scene_param_g[scene_slot_index_g].view_proj_inv_g._m33;
    r0.xy = v1.xy * float2(2,-2) + float2(-1,1);
    r0.w = 1;
    r21.x = dot(r0.xyzw, r21.xyzw);
    r21.y = dot(r0.xyzw, r22.xyzw);
    r21.z = dot(r0.xyzw, r23.xyzw);
    r21.w = dot(r0.xyzw, r24.xyzw);
    r0.xyzw = r21.yxzw / r21.wwww;
    r21.z = dot(r0.yxzw, r12.xyzw);
    r22.x = dot(r8.xyz, r10.xyz);
    r22.y = dot(r8.xyz, r11.xyz);
    r22.z = dot(r8.xyz, r12.xyz);
    r22.w = dot(r8.xyz, r9.xyz);
    r2.z = dot(r22.xyzw, r22.xyzw);
    r2.z = rsqrt(r2.z);
    r9.xyz = r22.xyz * r2.zzz;
    r2.z = lightTileSizeInv_g.z * -r21.z;
    r2.z = max(0, r2.z);
    r2.z = (uint)r2.z;
    r2.z = min(63, (uint)r2.z);
    r4.yw = lightTileSizeInv_g.xy * v0.xy;
    r4.yw = (uint2)r4.yw;
    r4.yw = min(uint2(0,0), (uint2)r4.yw);
    r4.w = (uint)r4.w << 4;
    r2.z = mad((int)r2.z, 144, (int)r4.w);
    r2.z = (int)r2.z + (int)r4.y;
    r12.xyz = r13.xyz + -r0.yxz;
    r4.y = dot(r12.xyz, r12.xyz);
    r4.y = rsqrt(r4.y);
    r13.xyz = r12.xyz * r4.yyy;
    r4.w = dot(r8.xyz, r13.xyz);
    r5.x = dot(r8.xyz, -lightDirection_g.xyz);
    r5.x = r5.z ? -r5.x : r5.x;
    r5.z = r20.w * r7.w;
    r17.xyz = sceneShadowColor_g.xyz + r17.xyz;
    r17.xyz = min(float3(1,1,1), r17.xyz);
    r22.xyzw = (int4)r2.yyyy & int4(8,16,512,64);
    if (r22.x != 0) {
      r8.w = cmp(-r21.z < farShadowStartDistance_g);
      if (r8.w != 0) {
        r8.w = -shadow_split_distance_g.x + -r21.z;
        r9.w = cmp(r8.w >= 0);
        r23.zw = r9.ww ? float2(1,0) : 0;
        r24.x = dot(r0.yxzw, shadow_matrices_g[r23.w/4]._m00_m10_m20_m30);
        r24.y = dot(r0.yxzw, shadow_matrices_g[r23.w/4]._m01_m11_m21_m31);
        r24.z = dot(r0.yxzw, shadow_matrices_g[r23.w/4]._m02_m12_m22_m32);
        r12.w = dot(r0.yxzw, shadow_matrices_g[r23.w/4]._m03_m13_m23_m33);
        r24.xyz = r24.xyz / r12.www;
        switch (shadowSamplingMode_g) {
          case 1 :          r23.xy = saturate(r24.xy);
          r20.w = shadowMap.SampleCmpLevelZero(SmplShadow_s, r23.xyz, r24.z).x;
          r25.xyzw = saturate(invShadowSize_g.xyxy * float4(0.818182468,0,0.252832502,0.778138518) + r24.xyxy);
          r26.xy = r25.xy;
          r26.z = r23.z;
          r21.w = shadowMap.SampleCmpLevelZero(SmplShadow_s, r26.xyz, r24.z).x;
          r20.w = r21.w + r20.w;
          r26.xy = r25.zw;
          r21.w = shadowMap.SampleCmpLevelZero(SmplShadow_s, r26.xyz, r24.z).x;
          r20.w = r21.w + r20.w;
          r25.xyzw = saturate(invShadowSize_g.xyxy * float4(-0.661922991,0.48091498,-0.661922991,-0.48091498) + r24.xyxy);
          r26.xy = r25.xy;
          r21.w = shadowMap.SampleCmpLevelZero(SmplShadow_s, r26.xyz, r24.z).x;
          r20.w = r21.w + r20.w;
          r26.xy = r25.zw;
          r21.w = shadowMap.SampleCmpLevelZero(SmplShadow_s, r26.xyz, r24.z).x;
          r20.w = r21.w + r20.w;
          r25.xyzw = saturate(invShadowSize_g.xyxy * float4(0.252832502,-0.778136969,1.5,0) + r24.xyxy);
          r26.xy = r25.xy;
          r21.w = shadowMap.SampleCmpLevelZero(SmplShadow_s, r26.xyz, r24.z).x;
          r20.w = r21.w + r20.w;
          r26.xy = r25.zw;
          r21.w = shadowMap.SampleCmpLevelZero(SmplShadow_s, r26.xyz, r24.z).x;
          r20.w = r21.w + r20.w;
          r25.xyzw = saturate(invShadowSize_g.xyxy * float4(1.21352553,0.881677508,0.463525504,1.42658556) + r24.xyxy);
          r26.xy = r25.xy;
          r21.w = shadowMap.SampleCmpLevelZero(SmplShadow_s, r26.xyz, r24.z).x;
          r20.w = r21.w + r20.w;
          r26.xy = r25.zw;
          r21.w = shadowMap.SampleCmpLevelZero(SmplShadow_s, r26.xyz, r24.z).x;
          r20.w = r21.w + r20.w;
          r25.xyzw = saturate(invShadowSize_g.xyxy * float4(-0.463525504,1.42658401,-1.21352553,0.881677508) + r24.xyxy);
          r26.xy = r25.xy;
          r21.w = shadowMap.SampleCmpLevelZero(SmplShadow_s, r26.xyz, r24.z).x;
          r20.w = r21.w + r20.w;
          r26.xy = r25.zw;
          r21.w = shadowMap.SampleCmpLevelZero(SmplShadow_s, r26.xyz, r24.z).x;
          r20.w = r21.w + r20.w;
          r25.xyzw = saturate(invShadowSize_g.xyxy * float4(-1.5,0,-1.21352553,-0.881677508) + r24.xyxy);
          r26.xy = r25.xy;
          r21.w = shadowMap.SampleCmpLevelZero(SmplShadow_s, r26.xyz, r24.z).x;
          r20.w = r21.w + r20.w;
          r26.xy = r25.zw;
          r21.w = shadowMap.SampleCmpLevelZero(SmplShadow_s, r26.xyz, r24.z).x;
          r20.w = r21.w + r20.w;
          r25.xyzw = saturate(invShadowSize_g.xyxy * float4(-0.463525504,-1.42658556,0.463525504,-1.42658401) + r24.xyxy);
          r26.xy = r25.xy;
          r21.w = shadowMap.SampleCmpLevelZero(SmplShadow_s, r26.xyz, r24.z).x;
          r20.w = r21.w + r20.w;
          r26.xy = r25.zw;
          r21.w = shadowMap.SampleCmpLevelZero(SmplShadow_s, r26.xyz, r24.z).x;
          r20.w = r21.w + r20.w;
          r26.xy = saturate(invShadowSize_g.xy * float2(1.21352553,-0.881677508) + r24.xy);
          r21.w = shadowMap.SampleCmpLevelZero(SmplShadow_s, r26.xyz, r24.z).x;
          r20.w = r21.w + r20.w;
          r12.w = 0.0625 * r20.w;
          r13.w = -1;
          break;
          case 2 :          r20.w = shadow_split_distance_g.x / shadowDistance_g;
          r20.w = 0.00124999997 * r20.w;
          r20.w = r9.w ? r20.w : 0.00124999997;
          r21.w = dot(v0.xy, float2(0.0671105608,0.00583714992));
          r21.w = frac(r21.w);
          r21.w = 52.9829178 * r21.w;
          r21.w = frac(r21.w);
          r21.w = 6.28318548 * r21.w;
          r22.x = 0;
          r23.w = 0;
          while (true) {
            r25.x = cmp((int)r23.w >= 16);
            if (r25.x != 0) break;
            r25.x = (int)r23.w;
            r25.y = 0.5 + r25.x;
            r25.y = sqrt(r25.y);
            r25.y = 0.25 * r25.y;
            r25.x = r25.x * 2.4000001 + r21.w;
            sincos(r25.x, r25.x, r26.x);
            r26.x = r26.x * r25.y;
            r26.y = r25.y * r25.x;
            r23.xy = r26.xy * r20.ww + r24.xy;
            r25.x = shadowMap.SampleCmpLevelZero(SmplShadow_s, r23.xyz, r24.z).x;
            r22.x = r25.x + r22.x;
            r23.w = (int)r23.w + 1;
          }
          r12.w = 0.0625 * r22.x;
          r13.w = -1;
          break;
          case 3 :          r20.w = shadow_split_distance_g.x / shadowDistance_g;
          r20.w = 0.00124999997 * r20.w;
          r9.w = r9.w ? r20.w : 0.00124999997;
          r20.w = -6 + r24.z;
          r20.w = r20.w * r9.w;
          r20.w = r20.w / r24.z;
          r24.w = r23.z;
          r25.y = shadowMap.SampleLevel(SmplLinearClamp_s, r24.xyw, 0).x;
          r21.w = cmp(r25.y < r24.z);
          r25.x = 1;
          r25.xy = r21.ww ? r25.xy : 0;
          r21.w = dot(v0.xy, float2(0.0671105608,0.00583714992));
          r21.w = frac(r21.w);
          r21.w = 52.9829178 * r21.w;
          r21.w = frac(r21.w);
          r21.w = 6.28318548 * r21.w;
          r26.z = r24.w;
          r25.zw = r25.xy;
          r22.x = 0;
          while (true) {
            r23.w = cmp((int)r22.x >= 15);
            if (r23.w != 0) break;
            r23.w = (int)r22.x;
            r26.w = 0.5 + r23.w;
            r26.w = sqrt(r26.w);
            r26.w = 0.258198887 * r26.w;
            r23.w = r23.w * 2.4000001 + r21.w;
            sincos(r23.w, r27.x, r28.x);
            r28.x = r28.x * r26.w;
            r28.y = r27.x * r26.w;
            r26.xy = r28.xy * r20.ww + r24.xy;
            r23.w = shadowMap.SampleLevel(SmplLinearClamp_s, r26.xyz, 0).x;
            r26.x = cmp(r23.w < r24.z);
            r27.y = r25.w + r23.w;
            r27.x = 1 + r25.z;
            r25.zw = r26.xx ? r27.xy : r25.zw;
            r22.x = (int)r22.x + 1;
          }
          r20.w = cmp(r25.z >= 1);
          if (r20.w != 0) {
            r20.w = r25.w / r25.z;
            r20.w = r24.z + -r20.w;
            r20.w = min(0.0500000007, r20.w);
            r9.w = r20.w * r9.w;
            r9.w = 60 * r9.w;
            shadowMap.GetDimensions(0, fDest.x, fDest.y, fDest.z, fDest.w);
            r25.xy = fDest.xy;
            r25.xy = float2(0.333330005,0.333330005) / r25.xy;
            r25.xy = max(r25.xy, r9.ww);
            r26.z = r24.w;
            r9.w = 0;
            r20.w = 0;
            while (true) {
              r22.x = cmp((int)r20.w >= 16);
              if (r22.x != 0) break;
              r22.x = (int)r20.w;
              r23.w = 0.5 + r22.x;
              r23.w = sqrt(r23.w);
              r23.w = 0.25 * r23.w;
              r22.x = r22.x * 2.4000001 + r21.w;
              sincos(r22.x, r22.x, r27.x);
              r27.x = r27.x * r23.w;
              r27.y = r23.w * r22.x;
              r26.xy = r27.xy * r25.xy + r24.xy;
              r22.x = shadowMap.SampleCmpLevelZero(SmplShadow_s, r26.xyz, r24.z).x;
              r9.w = r22.x + r9.w;
              r20.w = (int)r20.w + 1;
            }
            r12.w = 0.0625 * r9.w;
          } else {
            r12.w = 1;
          }
          r13.w = -1;
          break;
          default :
          r13.w = 0;
          break;
        }
        if (r13.w == 0) {
          r23.xy = saturate(r24.xy);
          r12.w = shadowMap.SampleCmpLevelZero(SmplShadow_s, r23.xyz, r24.z).x;
        }
        r9.w = cmp(r8.w < 0);
        if (r9.w != 0) {
          r8.w = 1 + r8.w;
          r9.w = cmp(0 < r8.w);
          if (r9.w != 0) {
            r23.x = dot(r0.yxzw, shadow_matrices_g[1]._m00_m10_m20_m30);
            r23.y = dot(r0.yxzw, shadow_matrices_g[1]._m01_m11_m21_m31);
            r23.z = dot(r0.yxzw, shadow_matrices_g[1]._m02_m12_m22_m32);
            r9.w = dot(r0.yxzw, shadow_matrices_g[1]._m03_m13_m23_m33);
            r23.xyz = r23.xyz / r9.www;
            switch (shadowSamplingMode_g) {
              case 1 :              r24.xy = saturate(r23.xy);
              r24.z = 1;
              r20.w = shadowMap.SampleCmpLevelZero(SmplShadow_s, r24.xyz, r23.z).x;
              r24.xyzw = saturate(invShadowSize_g.xyxy * float4(0.252832502,0.778138518,0.818182468,0) + r23.xyxy);
              r25.xy = r24.zw;
              r25.z = 1;
              r21.w = shadowMap.SampleCmpLevelZero(SmplShadow_s, r25.xyz, r23.z).x;
              r20.w = r21.w + r20.w;
              r24.z = 1;
              r21.w = shadowMap.SampleCmpLevelZero(SmplShadow_s, r24.xyz, r23.z).x;
              r20.w = r21.w + r20.w;
              r24.xyzw = saturate(invShadowSize_g.xyxy * float4(-0.661922991,-0.48091498,-0.661922991,0.48091498) + r23.xyxy);
              r25.xy = r24.zw;
              r25.z = 1;
              r21.w = shadowMap.SampleCmpLevelZero(SmplShadow_s, r25.xyz, r23.z).x;
              r20.w = r21.w + r20.w;
              r24.z = 1;
              r21.w = shadowMap.SampleCmpLevelZero(SmplShadow_s, r24.xyz, r23.z).x;
              r20.w = r21.w + r20.w;
              r24.xyzw = saturate(invShadowSize_g.xyxy * float4(1.5,0,0.252832502,-0.778136969) + r23.xyxy);
              r25.xy = r24.zw;
              r25.z = 1;
              r21.w = shadowMap.SampleCmpLevelZero(SmplShadow_s, r25.xyz, r23.z).x;
              r20.w = r21.w + r20.w;
              r24.z = 1;
              r21.w = shadowMap.SampleCmpLevelZero(SmplShadow_s, r24.xyz, r23.z).x;
              r20.w = r21.w + r20.w;
              r24.xyzw = saturate(invShadowSize_g.xyxy * float4(0.463525504,1.42658556,1.21352553,0.881677508) + r23.xyxy);
              r25.xy = r24.zw;
              r25.z = 1;
              r21.w = shadowMap.SampleCmpLevelZero(SmplShadow_s, r25.xyz, r23.z).x;
              r20.w = r21.w + r20.w;
              r24.z = 1;
              r21.w = shadowMap.SampleCmpLevelZero(SmplShadow_s, r24.xyz, r23.z).x;
              r20.w = r21.w + r20.w;
              r24.xyzw = saturate(invShadowSize_g.xyxy * float4(-1.21352553,0.881677508,-0.463525504,1.42658401) + r23.xyxy);
              r25.xy = r24.zw;
              r25.z = 1;
              r21.w = shadowMap.SampleCmpLevelZero(SmplShadow_s, r25.xyz, r23.z).x;
              r20.w = r21.w + r20.w;
              r24.z = 1;
              r21.w = shadowMap.SampleCmpLevelZero(SmplShadow_s, r24.xyz, r23.z).x;
              r20.w = r21.w + r20.w;
              r24.xyzw = saturate(invShadowSize_g.xyxy * float4(-1.21352553,-0.881677508,-1.5,0) + r23.xyxy);
              r25.xy = r24.zw;
              r25.z = 1;
              r21.w = shadowMap.SampleCmpLevelZero(SmplShadow_s, r25.xyz, r23.z).x;
              r20.w = r21.w + r20.w;
              r24.z = 1;
              r21.w = shadowMap.SampleCmpLevelZero(SmplShadow_s, r24.xyz, r23.z).x;
              r20.w = r21.w + r20.w;
              r24.xyzw = saturate(invShadowSize_g.xyxy * float4(0.463525504,-1.42658401,-0.463525504,-1.42658556) + r23.xyxy);
              r25.xy = r24.zw;
              r25.z = 1;
              r21.w = shadowMap.SampleCmpLevelZero(SmplShadow_s, r25.xyz, r23.z).x;
              r20.w = r21.w + r20.w;
              r24.z = 1;
              r21.w = shadowMap.SampleCmpLevelZero(SmplShadow_s, r24.xyz, r23.z).x;
              r20.w = r21.w + r20.w;
              r24.xy = saturate(invShadowSize_g.xy * float2(1.21352553,-0.881677508) + r23.xy);
              r24.z = 1;
              r21.w = shadowMap.SampleCmpLevelZero(SmplShadow_s, r24.xyz, r23.z).x;
              r20.w = r21.w + r20.w;
              r9.w = 0.0625 * r20.w;
              r13.w = -1;
              break;
              case 2 :              r20.w = shadow_split_distance_g.x / shadowDistance_g;
              r20.w = 0.00124999997 * r20.w;
              r21.w = dot(v0.xy, float2(0.0671105608,0.00583714992));
              r21.w = frac(r21.w);
              r21.w = 52.9829178 * r21.w;
              r21.w = frac(r21.w);
              r21.w = 6.28318548 * r21.w;
              r24.z = 1;
              r22.x = 0;
              r24.w = 0;
              while (true) {
                r25.x = cmp((int)r24.w >= 16);
                if (r25.x != 0) break;
                r25.x = (int)r24.w;
                r25.y = 0.5 + r25.x;
                r25.y = sqrt(r25.y);
                r25.y = 0.25 * r25.y;
                r25.x = r25.x * 2.4000001 + r21.w;
                sincos(r25.x, r25.x, r26.x);
                r26.x = r26.x * r25.y;
                r26.y = r25.y * r25.x;
                r24.xy = r26.xy * r20.ww + r23.xy;
                r24.x = shadowMap.SampleCmpLevelZero(SmplShadow_s, r24.xyz, r23.z).x;
                r22.x = r24.x + r22.x;
                r24.w = (int)r24.w + 1;
              }
              r9.w = 0.0625 * r22.x;
              r13.w = -1;
              break;
              case 3 :              r20.w = shadow_split_distance_g.x / shadowDistance_g;
              r20.w = 0.00124999997 * r20.w;
              r21.w = -6 + r23.z;
              r21.w = r21.w * r20.w;
              r21.w = r21.w / r23.z;
              r23.w = 1;
              r24.y = shadowMap.SampleLevel(SmplLinearClamp_s, r23.xyw, 0).x;
              r22.x = cmp(r24.y < r23.z);
              r24.x = 1;
              r24.xy = r22.xx ? r24.xy : 0;
              r22.x = dot(v0.xy, float2(0.0671105608,0.00583714992));
              r22.x = frac(r22.x);
              r22.x = 52.9829178 * r22.x;
              r22.x = frac(r22.x);
              r22.x = 6.28318548 * r22.x;
              r25.z = 1;
              r24.zw = r24.xy;
              r23.w = 0;
              while (true) {
                r25.w = cmp((int)r23.w >= 15);
                if (r25.w != 0) break;
                r25.w = (int)r23.w;
                r26.x = 0.5 + r25.w;
                r26.x = sqrt(r26.x);
                r26.x = 0.258198887 * r26.x;
                r25.w = r25.w * 2.4000001 + r22.x;
                sincos(r25.w, r27.x, r28.x);
                r28.x = r28.x * r26.x;
                r28.y = r27.x * r26.x;
                r25.xy = r28.xy * r21.ww + r23.xy;
                r25.x = shadowMap.SampleLevel(SmplLinearClamp_s, r25.xyz, 0).x;
                r25.y = cmp(r25.x < r23.z);
                r26.y = r25.x + r24.w;
                r26.x = 1 + r24.z;
                r24.zw = r25.yy ? r26.xy : r24.zw;
                r23.w = (int)r23.w + 1;
              }
              r21.w = cmp(r24.z >= 1);
              if (r21.w != 0) {
                r21.w = r24.w / r24.z;
                r21.w = r23.z + -r21.w;
                r21.w = min(0.0500000007, r21.w);
                r20.w = r21.w * r20.w;
                r20.w = 60 * r20.w;
                shadowMap.GetDimensions(0, fDest.x, fDest.y, fDest.z, fDest.w);
                r24.xy = fDest.xy;
                r24.xy = float2(0.333330005,0.333330005) / r24.xy;
                r24.xy = max(r24.xy, r20.ww);
                r25.z = 1;
                r20.w = 0;
                r21.w = 0;
                while (true) {
                  r23.w = cmp((int)r21.w >= 16);
                  if (r23.w != 0) break;
                  r23.w = (int)r21.w;
                  r24.z = 0.5 + r23.w;
                  r24.z = sqrt(r24.z);
                  r24.z = 0.25 * r24.z;
                  r23.w = r23.w * 2.4000001 + r22.x;
                  sincos(r23.w, r26.x, r27.x);
                  r27.x = r27.x * r24.z;
                  r27.y = r26.x * r24.z;
                  r25.xy = r27.xy * r24.xy + r23.xy;
                  r23.w = shadowMap.SampleCmpLevelZero(SmplShadow_s, r25.xyz, r23.z).x;
                  r20.w = r23.w + r20.w;
                  r21.w = (int)r21.w + 1;
                }
                r9.w = 0.0625 * r20.w;
              } else {
                r9.w = 1;
              }
              r13.w = -1;
              break;
              default :
              r13.w = 0;
              break;
            }
            if (r13.w == 0) {
              r24.xy = saturate(r23.xy);
              r24.z = 1;
              r9.w = shadowMap.SampleCmpLevelZero(SmplShadow_s, r24.xyz, r23.z).x;
            }
            r9.w = r9.w + -r12.w;
            r12.w = r8.w * r9.w + r12.w;
          }
        }
        r8.w = cmp(shadowDistance_g < farShadowStartDistance_g);
        r9.w = -shadowFadeNear_g + -r21.z;
        r9.w = saturate(shadowFadeRangeInv_g * r9.w);
        r13.w = 1 + -r12.w;
        r9.w = r9.w * r13.w + r12.w;
        r8.w = r8.w ? r9.w : r12.w;
      } else {
        r9.w = cmp(-r21.z < farShadowEndDistance_g);
        if (r9.w != 0) {
          r23.x = dot(r0.yxzw, farShadowMtx_g._m00_m10_m20_m30);
          r23.y = dot(r0.yxzw, farShadowMtx_g._m01_m11_m21_m31);
          r23.z = dot(r0.yxzw, farShadowMtx_g._m02_m12_m22_m32);
          r9.w = dot(r0.yxzw, farShadowMtx_g._m03_m13_m23_m33);
          r23.xyz = r23.xyz / r9.www;
          r24.xy = cmp(r23.xy < float2(0,0));
          r24.zw = cmp(float2(1,1) < r23.xy);
          r9.w = (int)r24.z | (int)r24.x;
          r9.w = (int)r24.y | (int)r9.w;
          r9.w = (int)r24.w | (int)r9.w;
          if (r9.w != 0) {
            r8.w = 1;
          } else {
            r24.xy = saturate(r23.xy);
            r9.w = farShadowMap.SampleCmpLevelZero(SmplShadow_s, r24.xy, r23.z).x;
            r24.xyzw = saturate(invFarShadowSize_g.xyxy * float4(0.818182468,0,0.252832502,0.778138518) + r23.xyxy);
            r12.w = farShadowMap.SampleCmpLevelZero(SmplShadow_s, r24.xy, r23.z).x;
            r9.w = r12.w + r9.w;
            r12.w = farShadowMap.SampleCmpLevelZero(SmplShadow_s, r24.zw, r23.z).x;
            r9.w = r12.w + r9.w;
            r24.xyzw = saturate(invFarShadowSize_g.xyxy * float4(-0.661922991,0.48091498,-0.661922991,-0.48091498) + r23.xyxy);
            r12.w = farShadowMap.SampleCmpLevelZero(SmplShadow_s, r24.xy, r23.z).x;
            r9.w = r12.w + r9.w;
            r12.w = farShadowMap.SampleCmpLevelZero(SmplShadow_s, r24.zw, r23.z).x;
            r9.w = r12.w + r9.w;
            r24.xyzw = saturate(invFarShadowSize_g.xyxy * float4(0.252832502,-0.778136969,1.5,0) + r23.xyxy);
            r12.w = farShadowMap.SampleCmpLevelZero(SmplShadow_s, r24.xy, r23.z).x;
            r9.w = r12.w + r9.w;
            r12.w = farShadowMap.SampleCmpLevelZero(SmplShadow_s, r24.zw, r23.z).x;
            r9.w = r12.w + r9.w;
            r24.xyzw = saturate(invFarShadowSize_g.xyxy * float4(1.21352553,0.881677508,0.463525504,1.42658556) + r23.xyxy);
            r12.w = farShadowMap.SampleCmpLevelZero(SmplShadow_s, r24.xy, r23.z).x;
            r9.w = r12.w + r9.w;
            r12.w = farShadowMap.SampleCmpLevelZero(SmplShadow_s, r24.zw, r23.z).x;
            r9.w = r12.w + r9.w;
            r24.xyzw = saturate(invFarShadowSize_g.xyxy * float4(-0.463525504,1.42658401,-1.21352553,0.881677508) + r23.xyxy);
            r12.w = farShadowMap.SampleCmpLevelZero(SmplShadow_s, r24.xy, r23.z).x;
            r9.w = r12.w + r9.w;
            r12.w = farShadowMap.SampleCmpLevelZero(SmplShadow_s, r24.zw, r23.z).x;
            r9.w = r12.w + r9.w;
            r24.xyzw = saturate(invFarShadowSize_g.xyxy * float4(-1.5,0,-1.21352553,-0.881677508) + r23.xyxy);
            r12.w = farShadowMap.SampleCmpLevelZero(SmplShadow_s, r24.xy, r23.z).x;
            r9.w = r12.w + r9.w;
            r12.w = farShadowMap.SampleCmpLevelZero(SmplShadow_s, r24.zw, r23.z).x;
            r9.w = r12.w + r9.w;
            r24.xyzw = saturate(invFarShadowSize_g.xyxy * float4(-0.463525504,-1.42658556,0.463525504,-1.42658401) + r23.xyxy);
            r12.w = farShadowMap.SampleCmpLevelZero(SmplShadow_s, r24.xy, r23.z).x;
            r9.w = r12.w + r9.w;
            r12.w = farShadowMap.SampleCmpLevelZero(SmplShadow_s, r24.zw, r23.z).x;
            r9.w = r12.w + r9.w;
            r23.xy = saturate(invFarShadowSize_g.xy * float2(1.21352553,-0.881677508) + r23.xy);
            r12.w = farShadowMap.SampleCmpLevelZero(SmplShadow_s, r23.xy, r23.z).x;
            r9.w = r12.w + r9.w;
            r8.w = 0.0625 * r9.w;
          }
          r9.w = cmp(-r21.z < shadowDistance_g);
          if (r9.w != 0) {
            r9.w = cmp(shadow_split_distance_g.x >= -r21.z);
            r23.zw = r9.ww ? float2(0,0) : float2(1,5.60519386e-45);
            r24.x = dot(r0.yxzw, shadow_matrices_g[r23.w/4]._m00_m10_m20_m30);
            r24.y = dot(r0.yxzw, shadow_matrices_g[r23.w/4]._m01_m11_m21_m31);
            r24.z = dot(r0.yxzw, shadow_matrices_g[r23.w/4]._m02_m12_m22_m32);
            r12.w = dot(r0.yxzw, shadow_matrices_g[r23.w/4]._m03_m13_m23_m33);
            r24.xyz = r24.xyz / r12.www;
            switch (shadowSamplingMode_g) {
              case 1 :              r23.xy = saturate(r24.xy);
              r20.w = shadowMap.SampleCmpLevelZero(SmplShadow_s, r23.xyz, r24.z).x;
              r25.xyzw = saturate(invShadowSize_g.xyxy * float4(0.818182468,0,0.252832502,0.778138518) + r24.xyxy);
              r26.xy = r25.xy;
              r26.z = r23.z;
              r21.w = shadowMap.SampleCmpLevelZero(SmplShadow_s, r26.xyz, r24.z).x;
              r20.w = r21.w + r20.w;
              r26.xy = r25.zw;
              r21.w = shadowMap.SampleCmpLevelZero(SmplShadow_s, r26.xyz, r24.z).x;
              r20.w = r21.w + r20.w;
              r25.xyzw = saturate(invShadowSize_g.xyxy * float4(-0.661922991,0.48091498,-0.661922991,-0.48091498) + r24.xyxy);
              r26.xy = r25.xy;
              r21.w = shadowMap.SampleCmpLevelZero(SmplShadow_s, r26.xyz, r24.z).x;
              r20.w = r21.w + r20.w;
              r26.xy = r25.zw;
              r21.w = shadowMap.SampleCmpLevelZero(SmplShadow_s, r26.xyz, r24.z).x;
              r20.w = r21.w + r20.w;
              r25.xyzw = saturate(invShadowSize_g.xyxy * float4(0.252832502,-0.778136969,1.5,0) + r24.xyxy);
              r26.xy = r25.xy;
              r21.w = shadowMap.SampleCmpLevelZero(SmplShadow_s, r26.xyz, r24.z).x;
              r20.w = r21.w + r20.w;
              r26.xy = r25.zw;
              r21.w = shadowMap.SampleCmpLevelZero(SmplShadow_s, r26.xyz, r24.z).x;
              r20.w = r21.w + r20.w;
              r25.xyzw = saturate(invShadowSize_g.xyxy * float4(1.21352553,0.881677508,0.463525504,1.42658556) + r24.xyxy);
              r26.xy = r25.xy;
              r21.w = shadowMap.SampleCmpLevelZero(SmplShadow_s, r26.xyz, r24.z).x;
              r20.w = r21.w + r20.w;
              r26.xy = r25.zw;
              r21.w = shadowMap.SampleCmpLevelZero(SmplShadow_s, r26.xyz, r24.z).x;
              r20.w = r21.w + r20.w;
              r25.xyzw = saturate(invShadowSize_g.xyxy * float4(-0.463525504,1.42658401,-1.21352553,0.881677508) + r24.xyxy);
              r26.xy = r25.xy;
              r21.w = shadowMap.SampleCmpLevelZero(SmplShadow_s, r26.xyz, r24.z).x;
              r20.w = r21.w + r20.w;
              r26.xy = r25.zw;
              r21.w = shadowMap.SampleCmpLevelZero(SmplShadow_s, r26.xyz, r24.z).x;
              r20.w = r21.w + r20.w;
              r25.xyzw = saturate(invShadowSize_g.xyxy * float4(-1.5,0,-1.21352553,-0.881677508) + r24.xyxy);
              r26.xy = r25.xy;
              r21.w = shadowMap.SampleCmpLevelZero(SmplShadow_s, r26.xyz, r24.z).x;
              r20.w = r21.w + r20.w;
              r26.xy = r25.zw;
              r21.w = shadowMap.SampleCmpLevelZero(SmplShadow_s, r26.xyz, r24.z).x;
              r20.w = r21.w + r20.w;
              r25.xyzw = saturate(invShadowSize_g.xyxy * float4(-0.463525504,-1.42658556,0.463525504,-1.42658401) + r24.xyxy);
              r26.xy = r25.xy;
              r21.w = shadowMap.SampleCmpLevelZero(SmplShadow_s, r26.xyz, r24.z).x;
              r20.w = r21.w + r20.w;
              r26.xy = r25.zw;
              r21.w = shadowMap.SampleCmpLevelZero(SmplShadow_s, r26.xyz, r24.z).x;
              r20.w = r21.w + r20.w;
              r26.xy = saturate(invShadowSize_g.xy * float2(1.21352553,-0.881677508) + r24.xy);
              r21.w = shadowMap.SampleCmpLevelZero(SmplShadow_s, r26.xyz, r24.z).x;
              r20.w = r21.w + r20.w;
              r12.w = 0.0625 * r20.w;
              r13.w = -1;
              break;
              case 2 :              r20.w = shadow_split_distance_g.x / shadowDistance_g;
              r20.w = 0.00124999997 * r20.w;
              r20.w = r9.w ? 0.00124999997 : r20.w;
              r21.w = dot(v0.xy, float2(0.0671105608,0.00583714992));
              r21.w = frac(r21.w);
              r21.w = 52.9829178 * r21.w;
              r21.w = frac(r21.w);
              r21.w = 6.28318548 * r21.w;
              r22.x = 0;
              r23.w = 0;
              while (true) {
                r25.x = cmp((int)r23.w >= 16);
                if (r25.x != 0) break;
                r25.x = (int)r23.w;
                r25.y = 0.5 + r25.x;
                r25.y = sqrt(r25.y);
                r25.y = 0.25 * r25.y;
                r25.x = r25.x * 2.4000001 + r21.w;
                sincos(r25.x, r25.x, r26.x);
                r26.x = r26.x * r25.y;
                r26.y = r25.y * r25.x;
                r23.xy = r26.xy * r20.ww + r24.xy;
                r25.x = shadowMap.SampleCmpLevelZero(SmplShadow_s, r23.xyz, r24.z).x;
                r22.x = r25.x + r22.x;
                r23.w = (int)r23.w + 1;
              }
              r12.w = 0.0625 * r22.x;
              r13.w = -1;
              break;
              case 3 :              r20.w = shadow_split_distance_g.x / shadowDistance_g;
              r20.w = 0.00124999997 * r20.w;
              r9.w = r9.w ? 0.00124999997 : r20.w;
              r20.w = -6 + r24.z;
              r20.w = r20.w * r9.w;
              r20.w = r20.w / r24.z;
              r24.w = r23.z;
              r25.y = shadowMap.SampleLevel(SmplLinearClamp_s, r24.xyw, 0).x;
              r21.w = cmp(r25.y < r24.z);
              r25.x = 1;
              r25.xy = r21.ww ? r25.xy : 0;
              r21.w = dot(v0.xy, float2(0.0671105608,0.00583714992));
              r21.w = frac(r21.w);
              r21.w = 52.9829178 * r21.w;
              r21.w = frac(r21.w);
              r21.w = 6.28318548 * r21.w;
              r26.z = r24.w;
              r25.zw = r25.xy;
              r22.x = 0;
              while (true) {
                r23.w = cmp((int)r22.x >= 15);
                if (r23.w != 0) break;
                r23.w = (int)r22.x;
                r26.w = 0.5 + r23.w;
                r26.w = sqrt(r26.w);
                r26.w = 0.258198887 * r26.w;
                r23.w = r23.w * 2.4000001 + r21.w;
                sincos(r23.w, r27.x, r28.x);
                r28.x = r28.x * r26.w;
                r28.y = r27.x * r26.w;
                r26.xy = r28.xy * r20.ww + r24.xy;
                r23.w = shadowMap.SampleLevel(SmplLinearClamp_s, r26.xyz, 0).x;
                r26.x = cmp(r23.w < r24.z);
                r27.y = r25.w + r23.w;
                r27.x = 1 + r25.z;
                r25.zw = r26.xx ? r27.xy : r25.zw;
                r22.x = (int)r22.x + 1;
              }
              r20.w = cmp(r25.z >= 1);
              if (r20.w != 0) {
                r20.w = r25.w / r25.z;
                r20.w = r24.z + -r20.w;
                r20.w = min(0.0500000007, r20.w);
                r9.w = r20.w * r9.w;
                r9.w = 60 * r9.w;
                shadowMap.GetDimensions(0, fDest.x, fDest.y, fDest.z, fDest.w);
                r25.xy = fDest.xy;
                r25.xy = float2(0.333330005,0.333330005) / r25.xy;
                r25.xy = max(r25.xy, r9.ww);
                r26.z = r24.w;
                r9.w = 0;
                r20.w = 0;
                while (true) {
                  r22.x = cmp((int)r20.w >= 16);
                  if (r22.x != 0) break;
                  r22.x = (int)r20.w;
                  r23.w = 0.5 + r22.x;
                  r23.w = sqrt(r23.w);
                  r23.w = 0.25 * r23.w;
                  r22.x = r22.x * 2.4000001 + r21.w;
                  sincos(r22.x, r22.x, r27.x);
                  r27.x = r27.x * r23.w;
                  r27.y = r23.w * r22.x;
                  r26.xy = r27.xy * r25.xy + r24.xy;
                  r22.x = shadowMap.SampleCmpLevelZero(SmplShadow_s, r26.xyz, r24.z).x;
                  r9.w = r22.x + r9.w;
                  r20.w = (int)r20.w + 1;
                }
                r12.w = 0.0625 * r9.w;
              } else {
                r12.w = 1;
              }
              r13.w = -1;
              break;
              default :
              r13.w = 0;
              break;
            }
            if (r13.w == 0) {
              r23.xy = saturate(r24.xy);
              r12.w = shadowMap.SampleCmpLevelZero(SmplShadow_s, r23.xyz, r24.z).x;
            }
            r9.w = -farShadowStartDistance_g + -r21.z;
            r13.w = shadowDistance_g + -farShadowStartDistance_g;
            r9.w = saturate(r9.w / r13.w);
            r13.w = -r12.w + r8.w;
            r8.w = r9.w * r13.w + r12.w;
          }
        } else {
          r8.w = 1;
        }
      }
      r9.w = 1 + -r5.x;
      r9.w = r20.z * r9.w + r5.x;
      r9.w = r9.w * 0.5 + 0.5;
      r12.w = r9.w * r9.w;
      r9.w = -r9.w * r12.w + 1;
      r8.w = r9.w * r9.w + r8.w;
      r23.z = min(1, r8.w);
    } else {
      r23.z = 1;
    }
    r24.x = r14.w;
    r24.yz = r15.zy;
    r15.xy = -r24.xy + r15.xw;
    r15.xy = r6.xx * r15.xy + r24.xy;
    r24.w = r16.x;
    r15.zw = r24.zw + -r15.xy;
    r15.xy = r6.yy * r15.zw + r15.xy;
    r8.w = cmp((int)r22.y != 0);
    r6.z = cmp(r6.z >= 3.0398367e-05);
    r8.w = r6.z ? r8.w : 0;
    r24.xyz = r12.xyz * r4.yyy + -lightDirection_g.xyz;
    r9.w = dot(r24.xyz, r24.xyz);
    r9.w = rsqrt(r9.w);
    r24.xyz = r24.xyz * r9.www;
    r9.w = -1 + r23.z;
    r12.w = r18.w * r9.w + 1;
    r13.w = lightSpecularGlossiness_g * r15.y;
    r14.w = saturate(dot(r24.xyz, r8.xyz));
    r14.w = log2(r14.w);
    r13.w = r14.w * r13.w;
    r13.w = exp2(r13.w);
    r12.w = r13.w * r12.w;
    r12.w = lightSpecularIntensity_g * r12.w;
    r18.xyz = r12.www * r18.xyz;
    r18.xyz = lightColor_g.xyz * r18.xyz;
    r18.xyz = r8.www ? r18.xyz : 0;
    r15.zw = r5.xx * float2(0.5,-0.5) + float2(0.5,0.5);
    r5.x = max(r15.z, r15.w);
    r5.x = r5.x + -r15.z;
    r5.x = r20.z * r5.x + r15.z;
    r4.x = r23.z * r4.x;
    r12.w = r5.x * r23.z;
    r23.y = r22.z ? r4.x : r12.w;
    r22.xyz = float3(1,1,1) + -r17.xyz;
    r17.xyz = r23.yyy * r22.xyz + r17.xyz;
    r22.xyz = lightColor_g.xyz * r17.xyz;
    r4.x = lightIndices_g[r2.z].lightProbeCount;
    r15.zw = r8.zy * r8.zy;
    r12.w = r15.z * 3 + -1;
    r13.w = r8.x * r8.x + -r15.w;
    r24.xyz = float3(0,0,0);
    r14.w = 0;
    r15.z = 0;
    while (true) {
      r15.w = cmp((uint)r15.z >= (uint)r4.x);
      if (r15.w != 0) break;
      r15.w = (uint)r15.z << 2;
      r15.w = (int)r15.w + 256;
      r15.w = lightIndices_g[(uint)r2.z].lightProbeIndices[(uint)r15.z];
      r25.x = localLightProbes_g[(uint)r15.w].pos.x;
      r25.y = localLightProbes_g[(uint)r15.w].pos.y;
      r25.z = localLightProbes_g[(uint)r15.w].pos.z;
      r25.xyz = r25.xyz + -r0.yxz;
      r16.x = dot(r25.xyz, r25.xyz);
      r16.x = sqrt(r16.x);
      r20.z = localLightProbes_g[(uint)r15.w].radiusInv;
      r20.w = localLightProbes_g[(uint)r15.w].attenuation;
      r16.x = r20.z * r16.x;
      r16.x = log2(abs(r16.x));
      r16.x = r20.w * r16.x;
      r16.x = exp2(r16.x);
      r16.x = 1 + -r16.x;
      r18.w = cmp(r16.x >= 1.1920929e-07);
      if (r18.w != 0) {
        r25.x = localLightProbes_g[(uint)r15.w].sh[0].x;
        r25.y = localLightProbes_g[(uint)r15.w].sh[0].y;
        r25.z = localLightProbes_g[(uint)r15.w].sh[0].z;
        r26.x = localLightProbes_g[(uint)r15.w].sh[1].x;
        r26.y = localLightProbes_g[(uint)r15.w].sh[1].y;
        r26.z = localLightProbes_g[(uint)r15.w].sh[1].z;
        r25.xyz = r26.xyz * r8.xxx + r25.xyz;
        r26.x = localLightProbes_g[(uint)r15.w].sh[2].x;
        r26.y = localLightProbes_g[(uint)r15.w].sh[2].y;
        r26.z = localLightProbes_g[(uint)r15.w].sh[2].z;
        r25.xyz = r26.xyz * r8.yyy + r25.xyz;
        r26.x = localLightProbes_g[(uint)r15.w].sh[3].x;
        r26.y = localLightProbes_g[(uint)r15.w].sh[3].y;
        r26.z = localLightProbes_g[(uint)r15.w].sh[3].z;
        r25.xyz = r26.xyz * r8.zzz + r25.xyz;
        r26.x = localLightProbes_g[(uint)r15.w].sh[4].x;
        r26.y = localLightProbes_g[(uint)r15.w].sh[4].y;
        r26.z = localLightProbes_g[(uint)r15.w].sh[4].z;
        r26.xyz = r26.xyz * r8.zzz;
        r25.xyz = r26.xyz * r8.xxx + r25.xyz;
        r26.x = localLightProbes_g[(uint)r15.w].sh[5].x;
        r26.y = localLightProbes_g[(uint)r15.w].sh[5].y;
        r26.z = localLightProbes_g[(uint)r15.w].sh[5].z;
        r26.xyz = r26.xyz * r8.yyy;
        r25.xyz = r26.xyz * r8.zzz + r25.xyz;
        r26.x = localLightProbes_g[(uint)r15.w].sh[6].x;
        r26.y = localLightProbes_g[(uint)r15.w].sh[6].y;
        r26.z = localLightProbes_g[(uint)r15.w].sh[6].z;
        r26.xyz = r26.xyz * r8.yyy;
        r25.xyz = r26.xyz * r8.xxx + r25.xyz;
        r26.x = localLightProbes_g[(uint)r15.w].sh[7].x;
        r26.y = localLightProbes_g[(uint)r15.w].sh[7].y;
        r26.z = localLightProbes_g[(uint)r15.w].sh[7].z;
        r25.xyz = r26.xyz * r12.www + r25.xyz;
        r26.x = localLightProbes_g[(uint)r15.w].sh[8].x;
        r26.y = localLightProbes_g[(uint)r15.w].sh[8].y;
        r26.z = localLightProbes_g[(uint)r15.w].sh[8].z;
        r25.xyz = r26.xyz * r13.www + r25.xyz;
        r24.xyz = r25.xyz * r16.xxx + r24.xyz;
        r14.w = r16.x + r14.w;
      }
      r15.z = (int)r15.z + 1;
    }
    r4.x = cmp(0 < r14.w);
    r25.xyz = r24.xyz / r14.www;
    r24.xyz = r4.xxx ? r25.xyz : r24.xyz;
    r4.x = cmp(r14.w < 1);
    r25.xyz = lightProbe_g[1].xyz * r8.xxx + lightProbe_g[0].xyz;
    r25.xyz = lightProbe_g[2].xyz * r8.yyy + r25.xyz;
    r25.xyz = lightProbe_g[3].xyz * r8.zzz + r25.xyz;
    r26.xyz = lightProbe_g[4].xyz * r8.zzz;
    r25.xyz = r26.xyz * r8.xxx + r25.xyz;
    r26.xyz = lightProbe_g[5].xyz * r8.yyy;
    r25.xyz = r26.xyz * r8.zzz + r25.xyz;
    r26.xyz = lightProbe_g[6].xyz * r8.yyy;
    r25.xyz = r26.xyz * r8.xxx + r25.xyz;
    r25.xyz = lightProbe_g[7].xyz * r12.www + r25.xyz;
    r25.xyz = lightProbe_g[8].xyz * r13.www + r25.xyz;
    r12.w = 1 + -r14.w;
    r25.xyz = r25.xyz + -r24.xyz;
    r25.xyz = r12.www * r25.xyz + r24.xyz;
    r24.xyz = r4.xxx ? r25.xyz : r24.xyz;
    r24.xyz = max(float3(0,0,0), r24.xyz);
    r17.xyz = r17.xyz * lightColor_g.xyz + r24.xyz;
    r22.xyz = r24.xyz * r22.xyz;
    r22.xyz = min(float3(1,1,1), r22.xyz);
    r17.xyz = -r22.xyz + r17.xyz;
    r4.x = min(1, r5.z);
    r22.xyz = float3(1,1,1) + -r17.xyz;
    r17.xyz = r4.xxx * r22.xyz + r17.xyz;
    r15.zw = r22.ww ? float2(1.40129846e-45,4.20389539e-45) : float2(0,2.80259693e-45);
    r4.x = r8.w ? r15.w : r15.z;
    r8.w = lightIndices_g[r2.z].pointLightCount;
    r12.w = ~(int)r4.x;
    r12.w = (int)r12.w & 1;
    r4.x = (int)r4.x & 2;
    r22.xyz = float3(0,0,0);
    r24.xyz = float3(0,0,0);
    r25.xyz = float3(0,0,0);
    r13.w = 0;
    while (true) {
      r14.w = cmp((uint)r13.w >= (uint)r8.w);
      if (r14.w != 0) break;
      r14.w = (uint)r13.w << 2;
      r14.w = lightIndices_g[(uint)r2.z].pointLightIndices[(uint)r13.w];
      r26.x = dynamicLights_g[(uint)r14.w].pos.x;
      r26.y = dynamicLights_g[(uint)r14.w].pos.y;
      r26.z = dynamicLights_g[(uint)r14.w].pos.z;
      r26.xyz = r26.xyz + -r0.yxz;
      r15.z = dot(r26.xyz, r26.xyz);
      r15.w = sqrt(r15.z);
      r16.x = dynamicLights_g[(uint)r14.w].radiusInv;
      r15.w = r16.x * r15.w;
      r16.x = dynamicLights_g[(uint)r14.w].attenuation;
      r15.w = log2(r15.w);
      r15.w = r16.x * r15.w;
      r15.w = exp2(r15.w);
      r15.w = 1 + -r15.w;
      r16.x = cmp(r15.w >= 1.52587891e-05);
      if (r16.x != 0) {
        r15.z = rsqrt(r15.z);
        r26.xyz = r26.xyz * r15.zzz;
        r27.x = dynamicLights_g[(uint)r14.w].color.x;
        r27.y = dynamicLights_g[(uint)r14.w].color.y;
        r27.z = dynamicLights_g[(uint)r14.w].color.z;
        r28.xyz = r27.xyz * r15.www;
        r24.xyz = r27.xyz * r15.www + r24.xyz;
        if (r12.w != 0) {
          r15.z = dynamicLights_g[(uint)r14.w].translucency;
          r15.w = dot(r26.xyz, r8.xyz);
          r15.z = max(r15.z, r15.w);
          r28.xyz = r28.xyz * r15.zzz;
        }
        r25.xyz = r28.xyz + r25.xyz;
        if (r4.x != 0) {
          r26.xyz = r12.xyz * r4.yyy + r26.xyz;
          r15.z = dot(r26.xyz, r26.xyz);
          r15.z = rsqrt(r15.z);
          r26.xyz = r26.xyz * r15.zzz;
          r15.z = dynamicLights_g[(uint)r14.w].specularIntensity;
          r15.w = dynamicLights_g[(uint)r14.w].specularGlossiness;
          r14.w = r15.w * r15.y;
          r15.w = saturate(dot(r26.xyz, r8.xyz));
          r15.w = log2(r15.w);
          r14.w = r15.w * r14.w;
          r14.w = exp2(r14.w);
          r26.xyz = r28.xyz * r14.www;
          r22.xyz = r26.xyz * r15.zzz + r22.xyz;
        }
      }
      r13.w = (int)r13.w + 1;
    }
    r12.xyz = r25.xyz + r17.xyz;
    r15.yzw = r22.xyz * r17.www + r18.xyz;
    r17.xyzw = (int4)r2.yyyy & int4(256,1024,32,128);
    r2.y = cmp((int)r17.x != 0);
    r2.y = r6.z ? r2.y : 0;
    r2.z = 1 + -abs(r4.w);
    r2.z = max(0, r2.z);
    r4.x = r2.z * r20.x;
    r4.x = log2(r4.x);
    r4.x = r19.w * r4.x;
    r4.x = exp2(r4.x);
    r4.x = min(1, r4.x);
    r4.y = dot(r25.xyz, float3(0.298911989,0.586610973,0.114477001));
    r4.y = min(1, r4.y);
    r8.w = 1 + -r20.y;
    r4.y = r4.y * r8.w + r20.y;
    r4.y = r4.x * r4.y;
    r4.x = r17.y ? r4.y : r4.x;
    r18.xyz = r19.xyz * r4.xxx + r15.yzw;
    r15.yzw = r2.yyy ? r18.xyz : r15.yzw;
    if (r17.z != 0) {
      r4.xy = r14.yz * r7.xy;
      r2.y = r4.w + r4.w;
      r8.xyz = r8.xyz * -r2.yyy + r13.xyz;
      texEnvMap_g.GetDimensions(0, uiDest.x, uiDest.y, uiDest.z);
      r2.y = uiDest.z;
      r8.xyz = float3(1,-1,-1) * r8.xyz;
      r2.y = (int)r2.y + -1;
      r2.y = (uint)r2.y;
      r2.y = r4.y * r2.y;
      r8.xyz = texEnvMap_g.SampleLevel(SmplCube_s, r8.xyz, r2.y).xyz;
      r2.y = cmp(0 < r15.x);
      r4.w = log2(r2.z);
      r4.w = r15.x * r4.w;
      r4.w = exp2(r4.w);
      r2.y = r2.y ? r4.w : 1;
      r4.w = r4.x * r2.y;
      r13.xyz = r3.xyz * r8.xyz + -r3.xyz;
      r13.xyz = r4.www * r13.xyz + r3.xyz;
      r4.w = dot(r8.xyz, float3(0.298999995,0.587000012,0.114));
      r4.y = r4.y * -9 + 10;
      r4.w = log2(r4.w);
      r4.y = r4.y * r4.w;
      r4.y = exp2(r4.y);
      r4.w = 1 + -r4.y;
      r4.x = r4.x * r4.w + r4.y;
      r8.xyz = r8.xyz * r5.xxx;
      r4.xyw = r8.xyz * r4.xxx;
      r4.xyw = r4.xyw * r2.yyy;
      r2.y = -r7.y * r14.z + 1;
      r4.xyw = r4.xyw * r2.yyy + r15.yzw;
      r15.yzw = r6.zzz ? r4.xyw : r15.yzw;
    } else {
      r13.xyz = r3.xyz;
    }
    r2.y = cmp(r16.z >= 1.1920929e-07);
    r4.x = dot(r3.xyz, float3(0.298999995,0.587000012,0.114));
    r4.x = r4.x + -r16.z;
    r4.x = max(0, r4.x);
    r4.x = r4.x / r16.z;
    r4.x = min(1, r4.x);
    r2.y = r2.y ? r4.x : 1;
    r4.x = r16.y * r7.w;
    r4.x = r4.x * r3.w;
    r2.y = r4.x * r2.y;
    r4.x = r16.w * r9.w + 1;
    r2.y = r4.x * r2.y;
    r4.xyw = r15.yzw * r7.zzz;
    r4.xyw = r13.xyz * r12.xyz + r4.xyw;
    r2.z = 1.70000005 * r2.z;
    r2.z = log2(r2.z);
    r2.z = 15 * r2.z;
    r2.z = exp2(r2.z);
    r2.z = min(1, r2.z);
    r6.xy = r1.zw;
    r4.xyw = r2.zzz * r6.xyw + r4.xyw;
    r1.z = max(1, r5.z);
    r3.xyz = r4.xyw * r1.zzz;
    r1.w = cmp(r14.x >= 1.1920929e-07);
    if (r1.w != 0) {
      r21.x = dot(r0.yxzw, r10.xyzw);
      r21.y = dot(r0.yxzw, r11.xyzw);
      r0.y = dot(r21.xyz, r21.xyz);
      r0.y = sqrt(r0.y);
      r0.y = -twoLayeredFogStartDistance_g + r0.y;
      r0.y = saturate(twoLayeredFogDistanceRangeInv_g * r0.y);
      r0.zw = twoLayeredFogHeightFar_g.xy + -twoLayeredFogHeightNear_g.xy;
      r0.zw = r0.yy * r0.zw + twoLayeredFogHeightNear_g.xy;
      if (twoLayeredFogMode_g != 0) {
        r1.w = scene_param_g[scene_slot_index_g].look_at_g.y;
        r0.x = -r1.w + r0.x;
      }
      r0.x = r0.x + -r0.z;
      r0.z = r0.w + -r0.z;
      r0.x = saturate(r0.x / r0.z);
      r0.zw = twoLayeredFogDistanceCoefInv_g.y + -twoLayeredFogDistanceCoefInv_g.x;
      r0.zw = r0.xx * r0.zw + twoLayeredFogDistanceCoefInv_g.x;
      r0.y = log2(r0.y);
      r0.y = r0.z * r0.y;
      r0.y = exp2(r0.y);
      r0.y = min(1, r0.y);
      r6.xyz = twoLayeredFogColorLowerFar_g.xyz + -twoLayeredFogColorLowerNear_g.xyz;
      r6.xyz = r0.yyy * r6.xyz + twoLayeredFogColorLowerNear_g.xyz;
      r7.xyz = twoLayeredFogColorUpperFar_g.xyz + -twoLayeredFogColorUpperNear_g.xyz;
      r7.xyz = r0.yyy * r7.xyz + twoLayeredFogColorUpperNear_g.xyz;
      r7.xyz = r7.xyz + -r6.xyz;
      r6.xyz = r0.xxx * r7.xyz + r6.xyz;
      r5.xz = twoLayeredFogMaxIntensity_g.xy + -twoLayeredFogMinIntensity_g.xy;
      r0.yz = r0.yy * r5.xz + twoLayeredFogMinIntensity_g.xy;
      r0.z = r0.z + -r0.y;
      r0.x = r0.x * r0.z + r0.y;
      r0.y = dot(r3.xyz, float3(0.298999995,0.587000012,0.114));
      r7.xyz = r6.xyz * r0.xxx;
      r0.z = dot(r7.xyz, float3(0.298999995,0.587000012,0.114));
      r0.y = -r0.z * 0.5 + r0.y;
      r0.y = max(0, r0.y);
      r7.xyz = r3.xyz * r0.yyy;
      r8.xyz = r7.xyz * r0.www;
      r0.yzw = r7.xyz * r0.www + r6.xyz;
      r6.xyz = r8.xyz * r6.xyz;
      r6.xyz = min(float3(1,1,1), r6.xyz);
      r0.yzw = -r6.xyz + r0.yzw;
      r23.x = r0.x * r14.x;
      r0.xyz = -r4.xyw * r1.zzz + r0.yzw;
      r3.xyz = r23.xxx * r0.xyz + r3.xyz;
    } else {
      r23.x = 0;
    }
    r0.x = saturate(0.100000001 * r2.y);
    r0.x = 255 * r0.x;
    r0.x = (uint)r0.x;
    r0.x = r17.w ? r0.x : 0;
    r0.x = mad((int)r5.y, 256, (int)r0.x);
    r0.x = mad((int)r5.w, 0x00100000, (int)r0.x);
    r1.y = mad((int)r4.z, 0x08000000, (int)r0.x);
    r23.xyz = saturate(r23.xyz);
    r0.xyz = float3(255,255,255) * r23.xyz;
    r0.xyz = (uint3)r0.xyz;
    r0.x = mad((int)r0.y, 256, (int)r0.x);
    r1.x = mad((int)r0.z, 0x00010000, (int)r0.x);
    r0.xyz = saturate(r9.xyz * float3(0.5,0.5,0.5) + float3(0.5,0.5,0.5));
    r0.xyz = float3(255,255,255) * r0.xyz;
    r0.xyz = (uint3)r0.xyz;
    r0.x = mad((int)r0.y, 256, (int)r0.x);
    r2.x = mad((int)r0.z, 0x00010000, (int)r0.x);
    r24.xyz = saturate(r24.xyz);
    r0.xyz = float3(255,255,255) * r24.xyz;
    r0.xyz = (uint3)r0.xyz;
    r0.x = mad((int)r0.y, 256, (int)r0.x);
    r2.w = mad((int)r0.z, 0x00010000, (int)r0.x);
  }
  o0.xyzw = r3.xyzw;
  o0.xyz = renodx::tonemap::renodrt::BT709(o0.xyz);
  //o0.xyz = renodx::color::srgb::DecodeSafe(o0.xyz);
  o0.xyz = renodx::draw::ToneMapPass(o0.xyz); 
  //o0.xyz = renodx::draw::RenderIntermediatePass(o0.xyz);
  o1.xy = r1.yx;
  o2.xy = r2.xw;
  return;
}
