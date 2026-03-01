// ---- Created with 3Dmigoto v1.4.1 on Sun Mar  1 14:03:28 2026
#include "./shared.h"

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

struct InstanceParam
{
    float4x4 world;                // Offset:    0
    float4 color;                  // Offset:   64
    float4 uv;                     // Offset:   80
    float4 param;                  // Offset:   96
    uint boneAddress;              // Offset:  112
    float3 param2;                 // Offset:  116
    float4x4 prevWorld;            // Offset:  128
};

cbuffer cb_tex_swizzle : register(b10)
{
  uint swizzle_flags_g : packoffset(c0);
}

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

cbuffer cb_chara_material : register(b5)
{
  float2 uvScroll0_g : packoffset(c0);
  float2 uvScroll1_g : packoffset(c0.z);
  float2 uvScroll2_g : packoffset(c1);
  float opacity_g : packoffset(c1.z);
  float materialFogIntensity_g : packoffset(c1.w);
  float ssaoIntensity_g : packoffset(c2);
  float4 _align_16_a : packoffset(c3);
  float3 shadowColor_g : packoffset(c4);
  float dynamicLightIntensity_g : packoffset(c4.w);
  float lightPitch_g : packoffset(c5);
  float4 _align_16_b : packoffset(c6);
  float3 rimLightColor_g : packoffset(c7);
  float rimLightPower_g : packoffset(c7.w);
  float rimIntensity_g : packoffset(c8);
  float cameraSpecularIntensity_g : packoffset(c8.y);
  float fresnel0_g : packoffset(c8.z);
  float specularGlossiness0_g : packoffset(c8.w);
  float fresnel1_g : packoffset(c9);
  float specularGlossiness1_g : packoffset(c9.y);
  float4 _align_16_c : packoffset(c10);
  float3 shadowColor1_g : packoffset(c11);
  float3 shadowColor2_g : packoffset(c12);
  float shadowGradSharpness_g : packoffset(c12.w);
  float shadowGradBorder_g : packoffset(c13);
  float shadowOffsetDistance_g : packoffset(c13.y);
  float shadowBias_g : packoffset(c13.z);
  float4 _align_16_d : packoffset(c14);
  float3 highlightEdgeColor_g : packoffset(c15);
  float3 highlightEdgeShadowColor_g : packoffset(c16);
  float highlightEdgeShadeFade_g : packoffset(c16.w);
  float highlightEdgeSensitivityLow_g : packoffset(c17);
  float highlightEdgeSensitivityHigh_g : packoffset(c17.y);
  float highlightEdgeWeight_g : packoffset(c17.z);
  float highlightEdgeFarWeight_g : packoffset(c17.w);
  float highlightEdgeWeightDecayRatio_g : packoffset(c18);
  float highlightEdgeFarClipDistance_g : packoffset(c18.y);
  float highlightEdgeFarClipFadeRange_g : packoffset(c18.z);
  float highlightEdgeNormalLevel_g : packoffset(c18.w);
  float highlightEdgeNormalSensitivity_g : packoffset(c19);
  float highlightEdgeNormalFrontFade_g : packoffset(c19.y);
  float highlightEdgeDynamicLightColorIntensity_g : packoffset(c19.z);
  float highlightEdgeFogIntensity_g : packoffset(c19.w);
  float4 _align_for_noseSideLightColor_g : packoffset(c20);
  float3 noseSideLightColor_g : packoffset(c21);
  float noseSideLightPower_g : packoffset(c21.w);
  float noseSideLightIntensity_g : packoffset(c22);
  uint noseSideLightSide_g : packoffset(c22.y);
  uint materialID_g : packoffset(c22.z);
}

SamplerState Smpl0_s : register(s0);
SamplerState Smpl1_s : register(s1);
SamplerState Smpl5_s : register(s5);
SamplerState Smpl7_s : register(s7);
SamplerState SmplLinearClamp_s : register(s15);
SamplerComparisonState SmplShadow_s : register(s13);
Texture2D<float4> Tex0 : register(t0);
Texture2D<float4> Tex1 : register(t1);
Texture2D<float4> Tex5 : register(t5);
Texture2D<float4> Tex7 : register(t7);
StructuredBuffer<LightParam> dynamicLights_g : register(t11);
StructuredBuffer<SceneParam> scene_param_g : register(t12);
StructuredBuffer<LightProbeParam> localLightProbes_g : register(t13);
StructuredBuffer<LightIndexData> lightIndices_g : register(t15);
Texture2DArray<float4> shadowMap : register(t16);
Texture2D<float4> farShadowMap : register(t21);
StructuredBuffer<InstanceParam> instances_g : register(t23);


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_Position0,
  float3 v1 : NORMAL0,
  float w1 : TEXCOORD5,
  float3 v2 : TANGENT0,
  float3 v3 : BINORMAL0,
  float4 v4 : TEXCOORD0,
  float4 v5 : TEXCOORD1,
  float4 v6 : TEXCOORD2,
  float4 v7 : TEXCOORD4,
  nointerpolation uint4 v8 : TEXCOORD6,
  float4 v9 : TEXCOORD8,
  uint v10 : SV_IsFrontFace0,
  out float4 o0 : SV_Target0,
  out uint2 o1 : SV_Target1,
  out uint2 o2 : SV_Target2,
  out float2 o3 : SV_Target3)
{
  float4 r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12,r13,r14,r15,r16,r17;
  uint4 bitmask, uiDest;
  float4 fDest;

  // Skip all fragments for this shader.
  discard;

  if (enableMotionVectors_g != 0) {
    r0.xy = v9.xy / v9.ww;
    r0.xy = r0.xy * float2(0.5,-0.5) + float2(0.5,0.5);
    r0.zw = float2(1,1) / invViewportSize_g.xy;
    r0.xy = r0.xy * r0.zw + -v0.xy;
    r0.z = scene_param_g[scene_slot_index_g].motionJitterOffset_g.x;
    r0.w = scene_param_g[scene_slot_index_g].motionJitterOffset_g.y;
    o3.xy = r0.xy + -r0.zw;
  }
  r0.x = instances_g[v8.x].param.x;
  r0.y = instances_g[v8.x].param.y;
  r0.z = instances_g[v8.x].param.z;
  r0.w = instances_g[v8.x].param.w;
  r1.x = instances_g[v8.x].param2.x;
  r2.x = scene_param_g[scene_slot_index_g].view_inv_g._m30;
  r2.y = scene_param_g[scene_slot_index_g].view_inv_g._m31;
  r2.z = scene_param_g[scene_slot_index_g].view_inv_g._m32;
  r1.yzw = -v6.xyz + r2.xyz;
  r2.x = dot(r1.yzw, r1.yzw);
  r2.y = sqrt(r2.x);
  r0.x = r2.y + -r0.x;
  r0.x = saturate(r0.x * r0.y);
  r2.zw = cmp(float2(0,0) >= r0.yw);
  r2.zw = r2.zw ? float2(1,1) : 0;
  r0.x = r2.z + r0.x;
  r0.y = cmp(disableNearCameraAlpha_g >= 1);
  r0.y = r0.y ? 1.000000 : 0;
  r0.x = r0.x + r0.y;
  r0.y = -r2.y + r0.z;
  r0.y = saturate(r0.y * r0.w);
  r0.y = r0.y + r2.w;
  r0.z = cmp(disableFarCameraAlpha_g >= 1);
  r0.z = r0.z ? 1.000000 : 0;
  r0.y = r0.y + r0.z;
  r0.xy = min(float2(1,1), r0.xy);
  r0.x = r0.x * r0.y;
  r0.y = cmp(r0.x < 1);
  r0.y = r0.y ? 6 : 4;
  r0.z = rsqrt(r2.x);
  r1.yzw = r1.yzw * r0.zzz;
  r0.z = dot(v1.xyz, v1.xyz);
  r0.z = rsqrt(r0.z);
  r2.xyz = v1.zxy * r0.zzz;
  r0.z = dot(v2.xyz, v2.xyz);
  r0.z = rsqrt(r0.z);
  r3.xyz = v2.xyz * r0.zzz;
  r4.xyzw = v4.xyzw * float4(1,-1,1,-1) + float4(0,1,0,1);
  r5.xyzw = Tex0.Sample(Smpl0_s, r4.xy).xyzw;
  r6.xyz = int3(1,32,2) & swizzle_flags_g;
  r7.x = r5.x;
  r7.w = 1;
  r5.xyzw = r6.xxxx ? r7.xxxw : r5.xyzw;
  r0.zw = Tex5.Sample(Smpl5_s, r4.xy).xz;
  r0.zw = r6.yy ? r0.zz : r0.zw;
  r4.xyzw = Tex1.Sample(Smpl1_s, r4.zw).xyzw;
  r7.xyzw = r5.xyzw * r4.xyzw + -r5.xyzw;
  r4.xyzw = r4.wwww * r7.xyzw + r5.xyzw;
  r5.x = r4.x;
  r5.w = 1;
  r4.xyzw = r6.zzzz ? r5.xxxw : r4.xyzw;
  r4.xyzw = v7.xyzw * r4.xyzw;
  sincos(lightPitch_g, r5.x, r6.x);
  r6.xz = chrLightDir_g.xz * abs(r6.xx);
  r6.y = -1 * r5.x;
  r2.w = dot(r6.xyz, r6.xyz);
  r2.w = rsqrt(r2.w);
  r5.xyz = r6.xyz * r2.www;
  sincos(w1.x, r6.x, r7.x);
  r7.y = r7.x;
  r7.z = r6.x;
  r8.x = dot(r5.xz, r7.yz);
  r7.x = -r6.x;
  r8.z = dot(r5.xz, r7.xy);
  r8.y = r5.y;
  r2.w = dot(r2.yzx, r8.xyz);
  r3.w = dot(r1.wyz, r2.xyz);
  r5.x = scene_param_g[scene_slot_index_g].view_g._m00;
  r5.y = scene_param_g[scene_slot_index_g].view_g._m10;
  r5.z = scene_param_g[scene_slot_index_g].view_g._m20;
  r6.x = scene_param_g[scene_slot_index_g].view_g._m01;
  r6.y = scene_param_g[scene_slot_index_g].view_g._m11;
  r6.z = scene_param_g[scene_slot_index_g].view_g._m21;
  r7.x = scene_param_g[scene_slot_index_g].view_g._m02;
  r7.y = scene_param_g[scene_slot_index_g].view_g._m12;
  r7.z = scene_param_g[scene_slot_index_g].view_g._m22;
  r8.x = scene_param_g[scene_slot_index_g].view_g._m03;
  r8.y = scene_param_g[scene_slot_index_g].view_g._m13;
  r8.z = scene_param_g[scene_slot_index_g].view_g._m23;
  r5.x = dot(r2.yzx, r5.xyz);
  r5.y = dot(r2.yzx, r6.xyz);
  r5.z = dot(r2.yzx, r7.xyz);
  r5.w = dot(r2.yzx, r8.xyz);
  r5.w = dot(r5.xyzw, r5.xyzw);
  r5.w = rsqrt(r5.w);
  r5.xyz = r5.xyz * r5.www;
  r6.x = scene_param_g[scene_slot_index_g].view_g._m00;
  r6.y = scene_param_g[scene_slot_index_g].view_g._m10;
  r6.z = scene_param_g[scene_slot_index_g].view_g._m20;
  r6.w = scene_param_g[scene_slot_index_g].view_g._m30;
  r7.x = scene_param_g[scene_slot_index_g].view_g._m01;
  r7.y = scene_param_g[scene_slot_index_g].view_g._m11;
  r7.z = scene_param_g[scene_slot_index_g].view_g._m21;
  r7.w = scene_param_g[scene_slot_index_g].view_g._m31;
  r8.x = scene_param_g[scene_slot_index_g].view_g._m02;
  r8.y = scene_param_g[scene_slot_index_g].view_g._m12;
  r8.z = scene_param_g[scene_slot_index_g].view_g._m22;
  r8.w = scene_param_g[scene_slot_index_g].view_g._m32;
  r6.x = dot(v6.xyzw, r6.xyzw);
  r6.y = dot(v6.xyzw, r7.xyzw);
  r6.z = dot(v6.xyzw, r8.xyzw);
  r5.w = lightTileSizeInv_g.z * -r6.z;
  r5.w = max(0, r5.w);
  r5.w = (uint)r5.w;
  r5.w = min(63, (uint)r5.w);
  r7.xy = lightTileSizeInv_g.xy * v0.xy;
  r7.xy = (uint2)r7.xy;
  r7.xy = min(uint2(15,8), (uint2)r7.xy);
  r6.w = (uint)r7.y << 4;
  r5.w = mad((int)r5.w, 144, (int)r6.w);
  r5.w = (int)r5.w + (int)r7.x;
  r6.w = lightIndices_g[r5.w].pointLightCount;
  r7.xyzw = float4(0,0,0,0);
  while (true) {
    r9.x = cmp((uint)r7.w >= (uint)r6.w);
    if (r9.x != 0) break;
    r9.x = lightIndices_g[(uint)r5.w].pointLightIndices[(uint)r7.w];
    r9.y = dynamicLights_g[r9.x].pos.x;
    r9.z = dynamicLights_g[r9.x].pos.y;
    r9.w = dynamicLights_g[r9.x].pos.z;
    r9.yzw = -v6.xyz + r9.yzw;
    r9.y = dot(r9.yzw, r9.yzw);
    r9.y = sqrt(r9.y);
    r10.x = dynamicLights_g[r9.x].radiusInv;
    r10.y = dynamicLights_g[r9.x].charaColor.x;
    r10.z = dynamicLights_g[r9.x].charaColor.y;
    r10.w = dynamicLights_g[r9.x].charaColor.z;
    r9.y = r10.x * r9.y;
    r9.x = dynamicLights_g[r9.x].attenuation;
    r9.y = log2(r9.y);
    r9.x = r9.x * r9.y;
    r9.x = exp2(r9.x);
    r9.x = 1 + -r9.x;
    r9.x = max(0, r9.x);
    r7.xyz = r10.yzw * r9.xxx + r7.xyz;
    r7.w = (int)r7.w + 1;
  }
  r6.w = lightIndices_g[r5.w].spotLightCount;
  r9.xyz = float3(0,0,0);
  r7.w = 0;
  while (true) {
    r9.w = cmp((uint)r7.w >= (uint)r6.w);
    if (r9.w != 0) break;
    r9.w = lightIndices_g[(uint)r5.w].spotLightIndices[(uint)r7.w];
    r10.x = dynamicLights_g[r9.w].pos.x;
    r10.y = dynamicLights_g[r9.w].pos.y;
    r10.z = dynamicLights_g[r9.w].pos.z;
    r10.xyz = -v6.xyz + r10.xyz;
    r10.w = dot(r10.xyz, r10.xyz);
    r10.w = sqrt(r10.w);
    r11.x = dynamicLights_g[r9.w].radiusInv;
    r11.y = dynamicLights_g[r9.w].charaColor.x;
    r11.z = dynamicLights_g[r9.w].charaColor.y;
    r11.w = dynamicLights_g[r9.w].charaColor.z;
    r11.x = r11.x * r10.w;
    r12.x = dynamicLights_g[r9.w].attenuation;
    r11.x = log2(r11.x);
    r11.x = r12.x * r11.x;
    r11.x = exp2(r11.x);
    r11.x = 1 + -r11.x;
    r12.x = cmp(r11.x >= 1.52587891e-05);
    if (r12.x != 0) {
      r10.xyz = r10.xyz / r10.www;
      r12.x = dynamicLights_g[r9.w].vec.x;
      r12.y = dynamicLights_g[r9.w].vec.y;
      r12.z = dynamicLights_g[r9.w].vec.z;
      r12.w = dynamicLights_g[r9.w].spotAngleInv;
      r10.x = dot(r10.xyz, r12.xyz);
      r10.x = max(0, r10.x);
      r10.x = 1 + -r10.x;
      r10.x = r10.x * r12.w;
      r9.w = dynamicLights_g[r9.w].attenuationAngle;
      r10.x = log2(abs(r10.x));
      r9.w = r10.x * r9.w;
      r9.w = exp2(r9.w);
      r9.w = 1 + -r9.w;
      r9.w = max(0, r9.w);
      r9.w = r11.x * r9.w;
    } else {
      r9.w = 0;
    }
    r9.xyz = r11.yzw * r9.www + r9.xyz;
    r7.w = (int)r7.w + 1;
  }
  r7.xyz = r9.xyz + r7.xyz;
  r9.xyz = min(float3(1,1,1), r7.xyz);
  r6.w = dot(r9.xyz, float3(0.298911989,0.586610973,0.114477001));
  r9.xyz = float3(0.800000012,0.800000012,0.800000012) * r7.xyz;
  r9.xyz = max(lightColor_g.xyz, r9.xyz);
  r9.xyz = min(float3(1.10000002,1.10000002,1.10000002), r9.xyz);
  r10.xyz = lightDirection_g.xyz * shadowOffsetDistance_g;
  r10.w = 0;
  r10.xyzw = v6.xyzw + -r10.xyzw;
  r7.w = dot(r8.xyzw, r10.xyzw);
  r8.x = cmp(-r7.w < farShadowStartDistance_g);
  if (r8.x != 0) {
    r8.x = cmp(shadow_split_distance_g.x >= -r7.w);
    r11.zw = r8.xx ? float2(0,0) : float2(1,5.60519386e-45);
    r12.x = dot(r10.xyzw, shadow_matrices_g[r11.w/4]._m00_m10_m20_m30);
    r12.y = dot(r10.xyzw, shadow_matrices_g[r11.w/4]._m01_m11_m21_m31);
    r12.z = dot(r10.xyzw, shadow_matrices_g[r11.w/4]._m02_m12_m22_m32);
    r8.y = dot(r10.xyzw, shadow_matrices_g[r11.w/4]._m03_m13_m23_m33);
    r12.xyz = r12.xyz / r8.yyy;
    switch (shadowSamplingMode_g) {
      case 1 :      r11.xy = saturate(r12.xy);
      r8.w = shadowMap.SampleCmpLevelZero(SmplShadow_s, r11.xyz, r12.z).x;
      r13.xw = invShadowSize_g.xy;
      r13.yz = float2(0,0);
      r13.xyzw = r13.xyzw + r12.xyxy;
      r14.xy = saturate(r13.xy);
      r14.z = r11.z;
      r9.w = shadowMap.SampleCmpLevelZero(SmplShadow_s, r14.xyz, r12.z).x;
      r9.w = 0.125 * r9.w;
      r8.w = r8.w * 0.166666672 + r9.w;
      r15.xyz = -invShadowSize_g.xyx;
      r15.w = 0;
      r15.xyzw = r15.zwxy + r12.xyxy;
      r14.xy = saturate(r15.xy);
      r9.w = shadowMap.SampleCmpLevelZero(SmplShadow_s, r14.xyz, r12.z).x;
      r8.w = r9.w * 0.125 + r8.w;
      r14.xy = saturate(r13.zw);
      r9.w = shadowMap.SampleCmpLevelZero(SmplShadow_s, r14.xyz, r12.z).x;
      r8.w = r9.w * 0.125 + r8.w;
      r13.x = 0;
      r13.y = -invShadowSize_g.y;
      r14.xy = saturate(r13.xy + r12.xy);
      r9.w = shadowMap.SampleCmpLevelZero(SmplShadow_s, r14.xyz, r12.z).x;
      r8.w = r9.w * 0.125 + r8.w;
      r14.xy = saturate(invShadowSize_g.xy + r12.xy);
      r9.w = shadowMap.SampleCmpLevelZero(SmplShadow_s, r14.xyz, r12.z).x;
      r8.w = r9.w * 0.0833333358 + r8.w;
      r13.xyzw = invShadowSize_g.xyxy * float4(-1,1,1,-1) + r12.xyxy;
      r14.xy = saturate(r13.xy);
      r9.w = shadowMap.SampleCmpLevelZero(SmplShadow_s, r14.xyz, r12.z).x;
      r8.w = r9.w * 0.0833333358 + r8.w;
      r14.xy = saturate(r13.zw);
      r9.w = shadowMap.SampleCmpLevelZero(SmplShadow_s, r14.xyz, r12.z).x;
      r8.w = r9.w * 0.0833333358 + r8.w;
      r14.xy = saturate(r15.zw);
      r9.w = shadowMap.SampleCmpLevelZero(SmplShadow_s, r14.xyz, r12.z).x;
      r8.y = r9.w * 0.0833333358 + r8.w;
      r8.z = -1;
      break;
      case 2 :      r8.w = shadow_split_distance_g.x / shadowDistance_g;
      r8.w = 0.00124999997 * r8.w;
      r8.w = r8.x ? 0.00124999997 : r8.w;
      r9.w = dot(v0.xy, float2(0.0671105608,0.00583714992));
      r9.w = frac(r9.w);
      r9.w = 52.9829178 * r9.w;
      r9.w = frac(r9.w);
      r9.w = 6.28318548 * r9.w;
      r11.w = 0;
      r13.x = 0;
      while (true) {
        r13.y = cmp((int)r13.x >= 16);
        if (r13.y != 0) break;
        r13.y = (int)r13.x;
        r13.z = 0.5 + r13.y;
        r13.z = sqrt(r13.z);
        r13.z = 0.25 * r13.z;
        r13.y = r13.y * 2.4000001 + r9.w;
        sincos(r13.y, r14.x, r15.x);
        r15.x = r15.x * r13.z;
        r15.y = r14.x * r13.z;
        r11.xy = r15.xy * r8.ww + r12.xy;
        r13.y = shadowMap.SampleCmpLevelZero(SmplShadow_s, r11.xyz, r12.z).x;
        r11.w = r13.y + r11.w;
        r13.x = (int)r13.x + 1;
      }
      r8.y = 0.0625 * r11.w;
      r8.z = -1;
      break;
      case 3 :      r8.w = shadow_split_distance_g.x / shadowDistance_g;
      r8.w = 0.00124999997 * r8.w;
      r8.x = r8.x ? 0.00124999997 : r8.w;
      r8.w = -6 + r12.z;
      r8.w = r8.x * r8.w;
      r8.w = r8.w / r12.z;
      r12.w = r11.z;
      r13.y = shadowMap.SampleLevel(SmplLinearClamp_s, r12.xyw, 0).x;
      r9.w = cmp(r13.y < r12.z);
      r13.x = 1;
      r13.xy = r9.ww ? r13.xy : 0;
      r9.w = dot(v0.xy, float2(0.0671105608,0.00583714992));
      r9.w = frac(r9.w);
      r9.w = 52.9829178 * r9.w;
      r9.w = frac(r9.w);
      r9.w = 6.28318548 * r9.w;
      r14.z = r12.w;
      r13.zw = r13.xy;
      r11.w = 0;
      while (true) {
        r14.w = cmp((int)r11.w >= 15);
        if (r14.w != 0) break;
        r14.w = (int)r11.w;
        r15.x = 0.5 + r14.w;
        r15.x = sqrt(r15.x);
        r15.x = 0.258198887 * r15.x;
        r14.w = r14.w * 2.4000001 + r9.w;
        sincos(r14.w, r16.x, r17.x);
        r17.x = r17.x * r15.x;
        r17.y = r16.x * r15.x;
        r14.xy = r17.xy * r8.ww + r12.xy;
        r14.x = shadowMap.SampleLevel(SmplLinearClamp_s, r14.xyz, 0).x;
        r14.y = cmp(r14.x < r12.z);
        r15.y = r14.x + r13.w;
        r15.x = 1 + r13.z;
        r13.zw = r14.yy ? r15.xy : r13.zw;
        r11.w = (int)r11.w + 1;
      }
      r8.w = cmp(r13.z >= 1);
      if (r8.w != 0) {
        r8.w = r13.w / r13.z;
        r8.w = r12.z + -r8.w;
        r8.w = min(0.0500000007, r8.w);
        r8.x = r8.x * r8.w;
        r8.x = 60 * r8.x;
        shadowMap.GetDimensions(0, fDest.x, fDest.y, fDest.z, fDest.w);
        r13.xy = fDest.xy;
        r13.xy = float2(0.333330005,0.333330005) / r13.xy;
        r8.xw = max(r13.xy, r8.xx);
        r13.z = r12.w;
        r11.w = 0;
        r12.w = 0;
        while (true) {
          r13.w = cmp((int)r12.w >= 16);
          if (r13.w != 0) break;
          r13.w = (int)r12.w;
          r14.x = 0.5 + r13.w;
          r14.x = sqrt(r14.x);
          r14.x = 0.25 * r14.x;
          r13.w = r13.w * 2.4000001 + r9.w;
          sincos(r13.w, r15.x, r16.x);
          r16.x = r16.x * r14.x;
          r16.y = r15.x * r14.x;
          r13.xy = r16.xy * r8.xw + r12.xy;
          r13.x = shadowMap.SampleCmpLevelZero(SmplShadow_s, r13.xyz, r12.z).x;
          r11.w = r13.x + r11.w;
          r12.w = (int)r12.w + 1;
        }
        r8.y = 0.0625 * r11.w;
      } else {
        r8.y = 1;
      }
      r8.z = -1;
      break;
      default :
      r8.z = 0;
      break;
    }
    if (r8.z == 0) {
      r11.xy = saturate(r12.xy);
      r8.y = shadowMap.SampleCmpLevelZero(SmplShadow_s, r11.xyz, r12.z).x;
    }
    r8.x = cmp(shadowDistance_g < farShadowStartDistance_g);
    r8.z = -shadowFadeNear_g + -r7.w;
    r8.z = saturate(shadowFadeRangeInv_g * r8.z);
    r8.w = 1 + -r8.y;
    r8.z = r8.z * r8.w + r8.y;
    r8.x = r8.x ? r8.z : r8.y;
  } else {
    r8.y = cmp(-r7.w < farShadowEndDistance_g);
    if (r8.y != 0) {
      r11.x = dot(r10.xyzw, farShadowMtx_g._m00_m10_m20_m30);
      r11.y = dot(r10.xyzw, farShadowMtx_g._m01_m11_m21_m31);
      r11.z = dot(r10.xyzw, farShadowMtx_g._m02_m12_m22_m32);
      r8.y = dot(r10.xyzw, farShadowMtx_g._m03_m13_m23_m33);
      r8.yzw = r11.xyz / r8.yyy;
      r11.xy = cmp(r8.yz < float2(0,0));
      r11.zw = cmp(float2(1,1) < r8.yz);
      r9.w = (int)r11.z | (int)r11.x;
      r9.w = (int)r11.y | (int)r9.w;
      r9.w = (int)r11.w | (int)r9.w;
      if (r9.w != 0) {
        r8.x = 1;
      } else {
        r11.xy = saturate(r8.yz);
        r9.w = farShadowMap.SampleCmpLevelZero(SmplShadow_s, r11.xy, r8.w).x;
        r11.xw = invFarShadowSize_g.xy;
        r11.yz = float2(0,0);
        r11.xyzw = saturate(r11.xyzw + r8.yzyz);
        r11.x = farShadowMap.SampleCmpLevelZero(SmplShadow_s, r11.xy, r8.w).x;
        r11.x = 0.125 * r11.x;
        r9.w = r9.w * 0.166666672 + r11.x;
        r12.xyz = -invFarShadowSize_g.xyx;
        r12.w = 0;
        r12.xyzw = saturate(r12.zwxy + r8.yzyz);
        r11.x = farShadowMap.SampleCmpLevelZero(SmplShadow_s, r12.xy, r8.w).x;
        r9.w = r11.x * 0.125 + r9.w;
        r11.x = farShadowMap.SampleCmpLevelZero(SmplShadow_s, r11.zw, r8.w).x;
        r9.w = r11.x * 0.125 + r9.w;
        r11.x = 0;
        r11.y = -invFarShadowSize_g.y;
        r11.xy = saturate(r11.xy + r8.yz);
        r11.x = farShadowMap.SampleCmpLevelZero(SmplShadow_s, r11.xy, r8.w).x;
        r9.w = r11.x * 0.125 + r9.w;
        r11.xy = saturate(invFarShadowSize_g.xy + r8.yz);
        r11.x = farShadowMap.SampleCmpLevelZero(SmplShadow_s, r11.xy, r8.w).x;
        r9.w = r11.x * 0.0833333358 + r9.w;
        r11.xyzw = saturate(invFarShadowSize_g.xyxy * float4(-1,1,1,-1) + r8.yzyz);
        r8.y = farShadowMap.SampleCmpLevelZero(SmplShadow_s, r11.xy, r8.w).x;
        r8.y = r8.y * 0.0833333358 + r9.w;
        r8.z = farShadowMap.SampleCmpLevelZero(SmplShadow_s, r11.zw, r8.w).x;
        r8.y = r8.z * 0.0833333358 + r8.y;
        r8.z = farShadowMap.SampleCmpLevelZero(SmplShadow_s, r12.zw, r8.w).x;
        r8.x = r8.z * 0.0833333358 + r8.y;
      }
      r8.y = cmp(-r7.w < shadowDistance_g);
      if (r8.y != 0) {
        r8.y = cmp(shadow_split_distance_g.x >= -r7.w);
        r11.zw = r8.yy ? float2(0,0) : float2(1,5.60519386e-45);
        r12.x = dot(r10.xyzw, shadow_matrices_g[r11.w/4]._m00_m10_m20_m30);
        r12.y = dot(r10.xyzw, shadow_matrices_g[r11.w/4]._m01_m11_m21_m31);
        r12.z = dot(r10.xyzw, shadow_matrices_g[r11.w/4]._m02_m12_m22_m32);
        r8.z = dot(r10.xyzw, shadow_matrices_g[r11.w/4]._m03_m13_m23_m33);
        r10.xyz = r12.xyz / r8.zzz;
        switch (shadowSamplingMode_g) {
          case 1 :          r11.xy = saturate(r10.xy);
          r9.w = shadowMap.SampleCmpLevelZero(SmplShadow_s, r11.xyz, r10.z).x;
          r12.xw = invShadowSize_g.xy;
          r12.yz = float2(0,0);
          r12.xyzw = r12.xyzw + r10.xyxy;
          r13.xy = saturate(r12.xy);
          r13.z = r11.z;
          r11.w = shadowMap.SampleCmpLevelZero(SmplShadow_s, r13.xyz, r10.z).x;
          r11.w = 0.125 * r11.w;
          r9.w = r9.w * 0.166666672 + r11.w;
          r14.xyz = -invShadowSize_g.xyx;
          r14.w = 0;
          r14.xyzw = r14.zwxy + r10.xyxy;
          r13.xy = saturate(r14.xy);
          r11.w = shadowMap.SampleCmpLevelZero(SmplShadow_s, r13.xyz, r10.z).x;
          r9.w = r11.w * 0.125 + r9.w;
          r13.xy = saturate(r12.zw);
          r11.w = shadowMap.SampleCmpLevelZero(SmplShadow_s, r13.xyz, r10.z).x;
          r9.w = r11.w * 0.125 + r9.w;
          r12.x = 0;
          r12.y = -invShadowSize_g.y;
          r13.xy = saturate(r12.xy + r10.xy);
          r11.w = shadowMap.SampleCmpLevelZero(SmplShadow_s, r13.xyz, r10.z).x;
          r9.w = r11.w * 0.125 + r9.w;
          r13.xy = saturate(invShadowSize_g.xy + r10.xy);
          r11.w = shadowMap.SampleCmpLevelZero(SmplShadow_s, r13.xyz, r10.z).x;
          r9.w = r11.w * 0.0833333358 + r9.w;
          r12.xyzw = invShadowSize_g.xyxy * float4(-1,1,1,-1) + r10.xyxy;
          r13.xy = saturate(r12.xy);
          r11.w = shadowMap.SampleCmpLevelZero(SmplShadow_s, r13.xyz, r10.z).x;
          r9.w = r11.w * 0.0833333358 + r9.w;
          r13.xy = saturate(r12.zw);
          r11.w = shadowMap.SampleCmpLevelZero(SmplShadow_s, r13.xyz, r10.z).x;
          r9.w = r11.w * 0.0833333358 + r9.w;
          r13.xy = saturate(r14.zw);
          r11.w = shadowMap.SampleCmpLevelZero(SmplShadow_s, r13.xyz, r10.z).x;
          r8.z = r11.w * 0.0833333358 + r9.w;
          r8.w = -1;
          break;
          case 2 :          r9.w = shadow_split_distance_g.x / shadowDistance_g;
          r9.w = 0.00124999997 * r9.w;
          r9.w = r8.y ? 0.00124999997 : r9.w;
          r11.w = dot(v0.xy, float2(0.0671105608,0.00583714992));
          r11.w = frac(r11.w);
          r11.w = 52.9829178 * r11.w;
          r11.w = frac(r11.w);
          r11.w = 6.28318548 * r11.w;
          r12.xy = float2(0,0);
          while (true) {
            r12.z = cmp((int)r12.y >= 16);
            if (r12.z != 0) break;
            r12.z = (int)r12.y;
            r12.w = 0.5 + r12.z;
            r12.w = sqrt(r12.w);
            r12.w = 0.25 * r12.w;
            r12.z = r12.z * 2.4000001 + r11.w;
            sincos(r12.z, r13.x, r14.x);
            r14.x = r14.x * r12.w;
            r14.y = r13.x * r12.w;
            r11.xy = r14.xy * r9.ww + r10.xy;
            r12.z = shadowMap.SampleCmpLevelZero(SmplShadow_s, r11.xyz, r10.z).x;
            r12.x = r12.x + r12.z;
            r12.y = (int)r12.y + 1;
          }
          r8.z = 0.0625 * r12.x;
          r8.w = -1;
          break;
          case 3 :          r9.w = shadow_split_distance_g.x / shadowDistance_g;
          r9.w = 0.00124999997 * r9.w;
          r8.y = r8.y ? 0.00124999997 : r9.w;
          r9.w = -6 + r10.z;
          r9.w = r9.w * r8.y;
          r9.w = r9.w / r10.z;
          r10.w = r11.z;
          r12.y = shadowMap.SampleLevel(SmplLinearClamp_s, r10.xyw, 0).x;
          r11.w = cmp(r12.y < r10.z);
          r12.x = 1;
          r12.xy = r11.ww ? r12.xy : 0;
          r11.w = dot(v0.xy, float2(0.0671105608,0.00583714992));
          r11.w = frac(r11.w);
          r11.w = 52.9829178 * r11.w;
          r11.w = frac(r11.w);
          r11.w = 6.28318548 * r11.w;
          r13.z = r10.w;
          r12.zw = r12.xy;
          r13.w = 0;
          while (true) {
            r14.x = cmp((int)r13.w >= 15);
            if (r14.x != 0) break;
            r14.x = (int)r13.w;
            r14.y = 0.5 + r14.x;
            r14.y = sqrt(r14.y);
            r14.y = 0.258198887 * r14.y;
            r14.x = r14.x * 2.4000001 + r11.w;
            sincos(r14.x, r14.x, r15.x);
            r15.x = r15.x * r14.y;
            r15.y = r14.y * r14.x;
            r13.xy = r15.xy * r9.ww + r10.xy;
            r13.x = shadowMap.SampleLevel(SmplLinearClamp_s, r13.xyz, 0).x;
            r13.y = cmp(r13.x < r10.z);
            r14.y = r13.x + r12.w;
            r14.x = 1 + r12.z;
            r12.zw = r13.yy ? r14.xy : r12.zw;
            r13.w = (int)r13.w + 1;
          }
          r9.w = cmp(r12.z >= 1);
          if (r9.w != 0) {
            r9.w = r12.w / r12.z;
            r9.w = r10.z + -r9.w;
            r9.w = min(0.0500000007, r9.w);
            r8.y = r9.w * r8.y;
            r8.y = 60 * r8.y;
            shadowMap.GetDimensions(0, fDest.x, fDest.y, fDest.z, fDest.w);
            r12.xy = fDest.xy;
            r12.xy = float2(0.333330005,0.333330005) / r12.xy;
            r12.xy = max(r12.xy, r8.yy);
            r13.z = r10.w;
            r8.y = 0;
            r9.w = 0;
            while (true) {
              r10.w = cmp((int)r9.w >= 16);
              if (r10.w != 0) break;
              r10.w = (int)r9.w;
              r12.z = 0.5 + r10.w;
              r12.z = sqrt(r12.z);
              r12.z = 0.25 * r12.z;
              r10.w = r10.w * 2.4000001 + r11.w;
              sincos(r10.w, r14.x, r15.x);
              r15.x = r15.x * r12.z;
              r15.y = r14.x * r12.z;
              r13.xy = r15.xy * r12.xy + r10.xy;
              r10.w = shadowMap.SampleCmpLevelZero(SmplShadow_s, r13.xyz, r10.z).x;
              r8.y = r10.w + r8.y;
              r9.w = (int)r9.w + 1;
            }
            r8.z = 0.0625 * r8.y;
          } else {
            r8.z = 1;
          }
          r8.w = -1;
          break;
          default :
          r8.w = 0;
          break;
        }
        if (r8.w == 0) {
          r11.xy = saturate(r10.xy);
          r8.z = shadowMap.SampleCmpLevelZero(SmplShadow_s, r11.xyz, r10.z).x;
        }
        r7.w = -farShadowStartDistance_g + -r7.w;
        r8.y = shadowDistance_g + -farShadowStartDistance_g;
        r7.w = saturate(r7.w / r8.y);
        r8.y = r8.x + -r8.z;
        r8.x = r7.w * r8.y + r8.z;
      }
    } else {
      r8.x = 1;
    }
  }
  r6.w = r8.x + r6.w;
  r6.w = min(1, r6.w);
  r8.z = r6.w * 0.600000024 + 0.400000006;
  r10.x = r2.w * 0.5 + 0.5;
  r2.w = saturate(-shadowGradBorder_g + -r2.w);
  r2.w = 1 + -r2.w;
  r2.w = log2(r2.w);
  r2.w = shadowGradSharpness_g * r2.w;
  r2.w = exp2(r2.w);
  r11.xyz = -shadowColor2_g.xyz + shadowColor1_g.xyz;
  r11.xyz = r2.www * r11.xyz + shadowColor2_g.xyz;
  r10.y = 0.5;
  r2.w = Tex7.SampleLevel(Smpl7_s, r10.xy, 0).x;
  r0.z = r2.w + -r0.z;
  r0.z = 1 + r0.z;
  r0.z = min(1, r0.z);
  r8.y = r0.z * r8.z;
  r10.xyz = float3(1,1,1) + -r11.xyz;
  r10.xyz = r8.yyy * r10.xyz + r11.xyz;
  r11.xyz = r10.xyz * r9.xyz;
  r0.z = 1 + -abs(r3.w);
  r2.w = max(0, r0.z);
  r3.w = max(0, rimIntensity_g);
  r3.w = r3.w * r2.w;
  r3.w = log2(r3.w);
  r3.w = rimLightPower_g * r3.w;
  r3.w = exp2(r3.w);
  r3.w = min(1, r3.w);
  r12.xyz = rimLightColor_g.xyz * r3.www;
  r12.xyz = r12.xyz * r0.www;
  if (2 == 0) r13.x = 0; else if (2+12 < 32) {   r13.x = (uint)r1.x << (32-(2 + 12)); r13.x = (uint)r13.x >> (32-2);  } else r13.x = (uint)r1.x >> 12;
  if (4 == 0) r13.y = 0; else if (4+20 < 32) {   r13.y = (uint)r1.x << (32-(4 + 20)); r13.y = (uint)r13.y >> (32-4);  } else r13.y = (uint)r1.x >> 20;
  if (4 == 0) r13.z = 0; else if (4+24 < 32) {   r13.z = (uint)r1.x << (32-(4 + 24)); r13.z = (uint)r13.z >> (32-4);  } else r13.z = (uint)r1.x >> 24;
  if (4 == 0) r13.w = 0; else if (4+28 < 32) {   r13.w = (uint)r1.x << (32-(4 + 28)); r13.w = (uint)r13.w >> (32-4);  } else r13.w = (uint)r1.x >> 28;
  r1.x = (int)r13.x & noseSideLightSide_g;
  r1.y = dot(r3.xyz, r1.yzw);
  r1.y = r1.y * r1.y;
  r0.z = r1.y * r0.z;
  r0.z = noseSideLightIntensity_g * r0.z;
  r0.z = max(1.17549435e-38, r0.z);
  r0.z = log2(r0.z);
  r0.z = noseSideLightPower_g * r0.z;
  r0.z = exp2(r0.z);
  r0.z = min(1, r0.z);
  r1.yzw = noseSideLightColor_g.xyz * r0.zzz;
  r1.yzw = r1.yzw * r0.www + r12.xyz;
  r1.xyz = r1.xxx ? r1.yzw : r12.xyz;
  r0.z = lightIndices_g[r5.w].lightProbeCount;
  r3.xy = r2.xz * r2.xz;
  r0.w = r3.x * 3 + -1;
  r1.w = r2.y * r2.y + -r3.y;
  r3.xyzw = float4(0,0,0,0);
  r7.w = 0;
  while (true) {
    r8.w = cmp((uint)r7.w >= (uint)r0.z);
    if (r8.w != 0) break;
    r8.w = lightIndices_g[(uint)r5.w].lightProbeIndices[(uint)r7.w];
    r12.x = localLightProbes_g[r8.w].pos.x;
    r12.y = localLightProbes_g[r8.w].pos.y;
    r12.z = localLightProbes_g[r8.w].pos.z;
    r12.xyz = -v6.xyz + r12.xyz;
    r9.w = dot(r12.xyz, r12.xyz);
    r9.w = sqrt(r9.w);
    r12.x = localLightProbes_g[r8.w].radiusInv;
    r12.y = localLightProbes_g[r8.w].attenuation;
    r9.w = r12.x * r9.w;
    r9.w = log2(abs(r9.w));
    r9.w = r12.y * r9.w;
    r9.w = exp2(r9.w);
    r9.w = 1 + -r9.w;
    r10.w = cmp(r9.w >= 1.1920929e-07);
    if (r10.w != 0) {
      r12.x = localLightProbes_g[r8.w].sh[0].x;
      r12.y = localLightProbes_g[r8.w].sh[0].y;
      r12.z = localLightProbes_g[r8.w].sh[0].z;
      r14.x = localLightProbes_g[r8.w].sh[1].x;
      r14.y = localLightProbes_g[r8.w].sh[1].y;
      r14.z = localLightProbes_g[r8.w].sh[1].z;
      r12.xyz = r14.xyz * r2.yyy + r12.xyz;
      r14.x = localLightProbes_g[r8.w].sh[2].x;
      r14.y = localLightProbes_g[r8.w].sh[2].y;
      r14.z = localLightProbes_g[r8.w].sh[2].z;
      r12.xyz = r14.xyz * r2.zzz + r12.xyz;
      r14.x = localLightProbes_g[r8.w].sh[3].x;
      r14.y = localLightProbes_g[r8.w].sh[3].y;
      r14.z = localLightProbes_g[r8.w].sh[3].z;
      r12.xyz = r14.xyz * r2.xxx + r12.xyz;
      r14.x = localLightProbes_g[r8.w].sh[4].x;
      r14.y = localLightProbes_g[r8.w].sh[4].y;
      r14.z = localLightProbes_g[r8.w].sh[4].z;
      r14.xyz = r14.xyz * r2.xxx;
      r12.xyz = r14.xyz * r2.yyy + r12.xyz;
      r14.x = localLightProbes_g[r8.w].sh[5].x;
      r14.y = localLightProbes_g[r8.w].sh[5].y;
      r14.z = localLightProbes_g[r8.w].sh[5].z;
      r14.xyz = r14.xyz * r2.zzz;
      r12.xyz = r14.xyz * r2.xxx + r12.xyz;
      r14.x = localLightProbes_g[r8.w].sh[6].x;
      r14.y = localLightProbes_g[r8.w].sh[6].y;
      r14.z = localLightProbes_g[r8.w].sh[6].z;
      r14.xyz = r14.xyz * r2.zzz;
      r12.xyz = r14.xyz * r2.yyy + r12.xyz;
      r14.x = localLightProbes_g[r8.w].sh[7].x;
      r14.y = localLightProbes_g[r8.w].sh[7].y;
      r14.z = localLightProbes_g[r8.w].sh[7].z;
      r12.xyz = r14.xyz * r0.www + r12.xyz;
      r14.x = localLightProbes_g[r8.w].sh[8].x;
      r14.y = localLightProbes_g[r8.w].sh[8].y;
      r14.z = localLightProbes_g[r8.w].sh[8].z;
      r12.xyz = r14.xyz * r1.www + r12.xyz;
      r3.xyz = r12.xyz * r9.www + r3.xyz;
      r3.w = r9.w + r3.w;
    }
    r7.w = (int)r7.w + 1;
  }
  r0.z = cmp(0 < r3.w);
  r12.xyz = r3.xyz / r3.www;
  r3.xyz = r0.zzz ? r12.xyz : r3.xyz;
  r0.z = cmp(r3.w < 1);
  r12.xyz = lightProbe_g[1].xyz * r2.yyy + lightProbe_g[0].xyz;
  r12.xyz = lightProbe_g[2].xyz * r2.zzz + r12.xyz;
  r12.xyz = lightProbe_g[3].xyz * r2.xxx + r12.xyz;
  r14.xyz = lightProbe_g[4].xyz * r2.xxx;
  r12.xyz = r14.xyz * r2.yyy + r12.xyz;
  r14.xyz = lightProbe_g[5].xyz * r2.zzz;
  r12.xyz = r14.xyz * r2.xxx + r12.xyz;
  r14.xyz = lightProbe_g[6].xyz * r2.zzz;
  r2.xyz = r14.xyz * r2.yyy + r12.xyz;
  r2.xyz = lightProbe_g[7].xyz * r0.www + r2.xyz;
  r2.xyz = lightProbe_g[8].xyz * r1.www + r2.xyz;
  r0.w = 1 + -r3.w;
  r2.xyz = r2.xyz + -r3.xyz;
  r2.xyz = r0.www * r2.xyz + r3.xyz;
  r2.xyz = r0.zzz ? r2.xyz : r3.xyz;
  r2.xyz = max(float3(0,0,0), r2.xyz);
  r3.xyz = r9.xyz * r10.xyz + r2.xyz;
  r2.xyz = r11.xyz * r2.xyz;
  r2.xyz = min(float3(1,1,1), r2.xyz);
  r2.xyz = r3.xyz + -r2.xyz;
  r1.xyz = r4.xyz * r2.xyz + r1.xyz;
  r2.xyz = -chara_shadow_mul_color_g.xyz + float3(1,1,1);
  r2.xyz = r6.www * r2.xyz + chara_shadow_mul_color_g.xyz;
  r3.xyz = (uint3)r13.yzw;
  r3.xyz = float3(0.0666666701,0.0666666701,0.0666666701) * r3.xyz;
  r0.z = 1.70000005 * r2.w;
  r0.z = log2(r0.z);
  r0.z = 15 * r0.z;
  r0.z = exp2(r0.z);
  r0.z = min(1, r0.z);
  r3.xyz = r0.zzz * r3.xyz;
  r1.xyz = r1.xyz * r2.xyz + r3.xyz;
  r0.z = dot(r6.xyz, r6.xyz);
  r0.z = sqrt(r0.z);
  r0.z = -twoLayeredFogStartDistance_g + r0.z;
  r0.z = saturate(twoLayeredFogDistanceRangeInv_g * r0.z);
  r2.xy = twoLayeredFogHeightFar_g.xy + -twoLayeredFogHeightNear_g.xy;
  r2.xy = r0.zz * r2.xy + twoLayeredFogHeightNear_g.xy;
  if (twoLayeredFogMode_g != 0) {
    r0.w = scene_param_g[scene_slot_index_g].look_at_g.y;
    r0.w = v6.y + -r0.w;
  } else {
    r0.w = v6.y;
  }
  r0.w = r0.w + -r2.x;
  r1.w = r2.y + -r2.x;
  r0.w = saturate(r0.w / r1.w);
  r2.xy = twoLayeredFogDistanceCoefInv_g.y + -twoLayeredFogDistanceCoefInv_g.x;
  r2.xy = r0.ww * r2.xy + twoLayeredFogDistanceCoefInv_g.x;
  r0.z = log2(r0.z);
  r0.z = r2.x * r0.z;
  r0.z = exp2(r0.z);
  r0.z = min(1, r0.z);
  r2.xzw = twoLayeredFogColorLowerFar_g.xyz + -twoLayeredFogColorLowerNear_g.xyz;
  r2.xzw = r0.zzz * r2.xzw + twoLayeredFogColorLowerNear_g.xyz;
  r3.xyz = twoLayeredFogColorUpperFar_g.xyz + -twoLayeredFogColorUpperNear_g.xyz;
  r3.xyz = r0.zzz * r3.xyz + twoLayeredFogColorUpperNear_g.xyz;
  r3.xyz = r3.xyz + -r2.xzw;
  r2.xzw = r0.www * r3.xyz + r2.xzw;
  r3.xy = twoLayeredFogMaxIntensity_g.xy + -twoLayeredFogMinIntensity_g.xy;
  r3.xy = r0.zz * r3.xy + twoLayeredFogMinIntensity_g.xy;
  r0.z = r3.y + -r3.x;
  r0.z = r0.w * r0.z + r3.x;
  r0.w = dot(r1.xyz, float3(0.298999995,0.587000012,0.114));
  r3.xyz = r2.xzw * r0.zzz;
  r1.w = dot(r3.xyz, float3(0.298999995,0.587000012,0.114));
  r0.w = -r1.w * 0.5 + r0.w;
  r0.w = max(0, r0.w);
  r3.xyz = r1.xyz * r0.www;
  r4.xyz = r3.xyz * r2.yyy;
  r3.xyz = r3.xyz * r2.yyy + r2.xzw;
  r2.xyz = r4.xyz * r2.xzw;
  r2.xyz = min(float3(1,1,1), r2.xyz);
  r2.xyz = r3.xyz + -r2.xyz;
  r8.x = materialFogIntensity_g * r0.z;
  r2.xyz = r2.xyz + -r1.xyz;
  o0.xyz = r8.xxx * r2.xyz + r1.xyz;
  o0.w = r4.w * r0.x;
  r1.x = materialID_g << 8;
  r1.y = (uint)r0.y << 20;
  r0.xy = (int2)r1.xy & int2(0xfff00,0x7f00000);
  o1.x = (int)r0.y + (int)r0.x;
  r8.xyz = saturate(r8.xyz);
  r0.xyz = float3(255,255,255) * r8.xyz;
  r0.xyz = (uint3)r0.xyz;
  r0.x = mad((int)r0.y, 256, (int)r0.x);
  o1.y = mad((int)r0.z, 0x00010000, (int)r0.x);
  r0.xyz = saturate(r5.xyz * float3(0.5,0.5,0.5) + float3(0.5,0.5,0.5));
  r0.xyz = float3(255,255,255) * r0.xyz;
  r0.xyz = (uint3)r0.xyz;
  r0.x = mad((int)r0.y, 256, (int)r0.x);
  o2.x = mad((int)r0.z, 0x00010000, (int)r0.x);
  r7.xyz = saturate(r7.xyz);
  r0.xyz = float3(255,255,255) * r7.xyz;
  r0.xyz = (uint3)r0.xyz;
  //o0.xyz = renodx::tonemap::renodrt::BT709(o0.xyz);
  //o0.xyz = renodx::draw::ToneMapPass(o0.xyz);
  //o0.xyz = RENODX_DIFFUSE_WHITE_NITS / 1000.f;
  r0.x = mad((int)r0.y, 256, (int)r0.x);
  o2.y = mad((int)r0.z, 0x00010000, (int)r0.x);
  return;
}
