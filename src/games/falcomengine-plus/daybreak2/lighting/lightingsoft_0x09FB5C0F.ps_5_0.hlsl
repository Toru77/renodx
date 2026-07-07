// ---- Created with 3Dmigoto v1.4.1 on Mon Jul  6 22:45:56 2026

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
    float ssrThickness;            // Offset:  132
    float reserve;                 // Offset:  136
    uint flag;                     // Offset:  140
};

struct OutlineShapeParam
{
    float4x4 mtx;                  // Offset:    0
    float4 color;                  // Offset:   64
    uint type;                     // Offset:   80
    float radius;                  // Offset:   84
    float2 gradation_size;         // Offset:   88
    float gradation_sharpness;     // Offset:   96
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
  float2 curJitterOffset_g : packoffset(c88.z);
}

cbuffer cb_local : register(b2)
{
  float fadeRangeInv_g : packoffset(c0);
  float density_g : packoffset(c0.y);
  float4 offsetsAndWeights[8] : packoffset(c1);
}

cbuffer cb_deferred : register(b3)
{
  uint outlineShapeCount_g : packoffset(c0);
  uint evaluateAllShadowSamples_g : packoffset(c0.y);
  float4 outlineShapeMaskUVParam_g : packoffset(c1);
}

#include "../../shared.h"

SamplerState samPoint_s : register(s0);
SamplerState samLinear_s : register(s1);
SamplerState SmplCube_s : register(s14);
SamplerState SmplLinearClamp_s : register(s15);
SamplerComparisonState SmplShadow_s : register(s13);
Texture2D<float4> colorTexture : register(t0);
Texture2D<uint4> mrtTexture : register(t1);
Texture2D<float> depthTexture : register(t2);
StructuredBuffer<DeferredParam> deferredParams_g : register(t3);
StructuredBuffer<OutlineShapeParam> outlineShapes_g : register(t4);
Texture2D<float4> outlineShapeMask : register(t5);
Texture2D<float4> texSSRMap_g : register(t14);
Texture2D<float4> texMirror_g : register(t15);
Texture2D<float4> shadowMap : register(t16);
TextureCube<float4> texEnvMap_g : register(t17);
StructuredBuffer<LightParam> dynamicLights_g : register(t18);
StructuredBuffer<LightIndexData> lightIndices_g : register(t19);
Texture2D<float4> farShadowMap : register(t21);
Texture2DArray<float4> spotShadowMaps : register(t24);
StructuredBuffer<float4x4> spotShadowMatrices_g : register(t25);


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_Position0,
  float4 v1 : TEXCOORD0,
  out float4 o0 : SV_Target0,
  out uint4 o1 : SV_Target1)
{
  float4 r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22;

  float cubemap_mode = shader_injection_data.cubemap_improvements_enabled;
  float cubemap_improved_factor = saturate(cubemap_mode);
  float cubemap_lighting_mip_boost = clamp(shader_injection_data.cubemap_lighting_mip_boost, 0.5, 4.0);

  r0.xyzw = colorTexture.SampleLevel(samPoint_s, v1.xy, 0).xyzw;
  {
    uint mrtW, mrtH, mrtMips;
    mrtTexture.GetDimensions(0, mrtW, mrtH, mrtMips);
    r1.xy = float2(mrtW, mrtH);
  }
  r1.zw = v1.xy * r1.xy;
  r2.xy = (int2)r1.zw;
  r2.zw = float2(0,0);
  r2.xyzw = mrtTexture.Load(r2.xyz).wxyz;
  r3.z = (uint)r2.w >> 24;
  if (1 == 0) r1.z = 0; else if (1+24 < 32) {   r1.z = (uint)r2.w << (32-(1 + 24)); r1.z = (uint)r1.z >> (32-1);  } else r1.z = (uint)r2.w >> 24;
  r1.z = cmp((int)r1.z != 0);
  r1.w = (int)r3.z & 4;
  r1.w = cmp((int)r1.w == 0);
  r1.z = r1.w ? r1.z : 0;
  if (r1.z != 0) {
    r4.z = depthTexture.SampleLevel(samPoint_s, v1.xy, 0).x;
    r5.y = (uint)r2.y >> 16;
    r5.x = r2.y;
    r1.zw = (int2)r5.xy & int2(0xffff,0xffff);
    r1.zw = (uint2)r1.zw;
    r5.xy = float2(1.52590219e-05,1.52590219e-05) * r1.zw;
    r1.z = (int)r2.z & 0x0000ffff;
    r1.z = (uint)r1.z;
    r5.z = 1.52590219e-05 * r1.z;
    r5.xyz = float3(-0.5,-0.5,-0.5) + r5.xyz;
    r5.xyz = r5.xyz + r5.xyz;
    r4.xy = v1.xy * float2(2,-2) + float2(-1,1);
    r4.w = 1;
    r1.z = dot(r4.xyzw, projInv_g._m02_m12_m22_m32);
    r1.w = dot(r4.xyzw, projInv_g._m03_m13_m23_m33);
    r1.z = r1.z / r1.w;
    r1.w = saturate(-0.5 * r1.z);
    r1.w = r1.w * 0.00899999961 + 0.00100000005;
    r4.xy = offsetsAndWeights[0].xy + v1.xy;
    r4.zw = r4.xy * r1.xy;
    r6.xy = (int2)r4.zw;
    r6.zw = float2(0,0);
    r3.w = mrtTexture.Load(r6.xyz).z;
    r3.w = (uint)r3.w >> 24;
    r3.w = (int)r3.w & 4;
    if (r3.w == 0) {
      r6.z = depthTexture.SampleLevel(samLinear_s, r4.xy, 0).x;
      r6.xy = r4.xy * float2(2,-2) + float2(-1,1);
      r6.w = 1;
      r3.w = dot(r6.xyzw, projInv_g._m02_m12_m22_m32);
      r4.x = dot(r6.xyzw, projInv_g._m03_m13_m23_m33);
      r3.w = r3.w / r4.x;
      r3.w = r3.w + -r1.z;
      r3.w = cmp(abs(r3.w) >= r1.w);
      r3.w = r3.w ? 1.000000 : 0;
      r3.w = offsetsAndWeights[0].z * r3.w;
      r4.xy = offsetsAndWeights[1].xy + v1.xy;
      r4.zw = r4.xy * r1.xy;
      r6.xy = (int2)r4.zw;
      r6.zw = float2(0,0);
      r4.z = mrtTexture.Load(r6.xyz).z;
      r4.z = (uint)r4.z >> 24;
      r4.z = (int)r4.z & 4;
      r4.w = cmp((int)r4.z != 0);
      if (r4.w == 0) {
        r6.z = depthTexture.SampleLevel(samLinear_s, r4.xy, 0).x;
        r6.xy = r4.xy * float2(2,-2) + float2(-1,1);
        r6.w = 1;
        r4.x = dot(r6.xyzw, projInv_g._m02_m12_m22_m32);
        r4.y = dot(r6.xyzw, projInv_g._m03_m13_m23_m33);
        r4.x = r4.x / r4.y;
        r4.x = r4.x + -r1.z;
        r4.x = cmp(abs(r4.x) >= r1.w);
        r4.x = r4.x ? 1.000000 : 0;
        r3.w = r4.x * offsetsAndWeights[1].z + r3.w;
      }
      r4.z = r4.w;
    } else {
      r3.w = 0;
      r4.zw = float2(-1, -1);
    }
    r4.xy = ~(int2)r4.wz;
    r4.x = r4.x ? r4.y : 0;
    if (r4.x != 0) {
      r4.xy = offsetsAndWeights[2].xy + v1.xy;
      r6.xy = r4.xy * r1.xy;
      r6.xy = (int2)r6.xy;
      r6.zw = float2(0,0);
      r5.w = mrtTexture.Load(r6.xyz).z;
      r5.w = (uint)r5.w >> 24;
      r5.w = (int)r5.w & 4;
      r4.w = cmp((int)r5.w != 0);
      if (r4.w == 0) {
        r6.z = depthTexture.SampleLevel(samLinear_s, r4.xy, 0).x;
        r6.xy = r4.xy * float2(2,-2) + float2(-1,1);
        r6.w = 1;
        r4.x = dot(r6.xyzw, projInv_g._m02_m12_m22_m32);
        r4.y = dot(r6.xyzw, projInv_g._m03_m13_m23_m33);
        r4.x = r4.x / r4.y;
        r4.x = r4.x + -r1.z;
        r4.x = cmp(abs(r4.x) >= r1.w);
        r4.x = r4.x ? 1.000000 : 0;
        r3.w = r4.x * offsetsAndWeights[2].z + r3.w;
      }
      r4.z = r4.w;
    } else {
      r4.w = -1;
    }
    r4.xy = ~(int2)r4.wz;
    r4.x = r4.x ? r4.y : 0;
    if (r4.x != 0) {
      r4.xy = offsetsAndWeights[3].xy + v1.xy;
      r6.xy = r4.xy * r1.xy;
      r6.xy = (int2)r6.xy;
      r6.zw = float2(0,0);
      r5.w = mrtTexture.Load(r6.xyz).z;
      r5.w = (uint)r5.w >> 24;
      r5.w = (int)r5.w & 4;
      r4.w = cmp((int)r5.w != 0);
      if (r4.w == 0) {
        r6.z = depthTexture.SampleLevel(samLinear_s, r4.xy, 0).x;
        r6.xy = r4.xy * float2(2,-2) + float2(-1,1);
        r6.w = 1;
        r4.x = dot(r6.xyzw, projInv_g._m02_m12_m22_m32);
        r4.y = dot(r6.xyzw, projInv_g._m03_m13_m23_m33);
        r4.x = r4.x / r4.y;
        r4.x = r4.x + -r1.z;
        r4.x = cmp(abs(r4.x) >= r1.w);
        r4.x = r4.x ? 1.000000 : 0;
        r3.w = r4.x * offsetsAndWeights[3].z + r3.w;
      }
      r4.z = r4.w;
    } else {
      r4.w = -1;
    }
    r4.xy = ~(int2)r4.wz;
    r4.x = r4.x ? r4.y : 0;
    if (r4.x != 0) {
      r4.xy = offsetsAndWeights[4].xy + v1.xy;
      r6.xy = r4.xy * r1.xy;
      r6.xy = (int2)r6.xy;
      r6.zw = float2(0,0);
      r5.w = mrtTexture.Load(r6.xyz).z;
      r5.w = (uint)r5.w >> 24;
      r5.w = (int)r5.w & 4;
      r4.w = cmp((int)r5.w != 0);
      if (r4.w == 0) {
        r6.z = depthTexture.SampleLevel(samLinear_s, r4.xy, 0).x;
        r6.xy = r4.xy * float2(2,-2) + float2(-1,1);
        r6.w = 1;
        r4.x = dot(r6.xyzw, projInv_g._m02_m12_m22_m32);
        r4.y = dot(r6.xyzw, projInv_g._m03_m13_m23_m33);
        r4.x = r4.x / r4.y;
        r4.x = r4.x + -r1.z;
        r4.x = cmp(abs(r4.x) >= r1.w);
        r4.x = r4.x ? 1.000000 : 0;
        r3.w = r4.x * offsetsAndWeights[4].z + r3.w;
      }
      r4.z = r4.w;
    } else {
      r4.w = -1;
    }
    r4.xy = ~(int2)r4.wz;
    r4.x = r4.x ? r4.y : 0;
    if (r4.x != 0) {
      r4.xy = offsetsAndWeights[5].xy + v1.xy;
      r6.xy = r4.xy * r1.xy;
      r6.xy = (int2)r6.xy;
      r6.zw = float2(0,0);
      r5.w = mrtTexture.Load(r6.xyz).z;
      r5.w = (uint)r5.w >> 24;
      r5.w = (int)r5.w & 4;
      r4.w = cmp((int)r5.w != 0);
      if (r4.w == 0) {
        r6.z = depthTexture.SampleLevel(samLinear_s, r4.xy, 0).x;
        r6.xy = r4.xy * float2(2,-2) + float2(-1,1);
        r6.w = 1;
        r4.x = dot(r6.xyzw, projInv_g._m02_m12_m22_m32);
        r4.y = dot(r6.xyzw, projInv_g._m03_m13_m23_m33);
        r4.x = r4.x / r4.y;
        r4.x = r4.x + -r1.z;
        r4.x = cmp(abs(r4.x) >= r1.w);
        r4.x = r4.x ? 1.000000 : 0;
        r3.w = r4.x * offsetsAndWeights[5].z + r3.w;
      }
      r4.z = r4.w;
    } else {
      r4.w = -1;
    }
    r4.xy = ~(int2)r4.wz;
    r4.x = r4.x ? r4.y : 0;
    if (r4.x != 0) {
      r4.xy = offsetsAndWeights[6].xy + v1.xy;
      r6.xy = r4.xy * r1.xy;
      r6.xy = (int2)r6.xy;
      r6.zw = float2(0,0);
      r5.w = mrtTexture.Load(r6.xyz).z;
      r5.w = (uint)r5.w >> 24;
      r5.w = (int)r5.w & 4;
      r4.w = cmp((int)r5.w != 0);
      if (r4.w == 0) {
        r6.z = depthTexture.SampleLevel(samLinear_s, r4.xy, 0).x;
        r6.xy = r4.xy * float2(2,-2) + float2(-1,1);
        r6.w = 1;
        r4.x = dot(r6.xyzw, projInv_g._m02_m12_m22_m32);
        r4.y = dot(r6.xyzw, projInv_g._m03_m13_m23_m33);
        r4.x = r4.x / r4.y;
        r4.x = r4.x + -r1.z;
        r4.x = cmp(abs(r4.x) >= r1.w);
        r4.x = r4.x ? 1.000000 : 0;
        r3.w = r4.x * offsetsAndWeights[6].z + r3.w;
      }
      r4.z = r4.w;
    } else {
      r4.w = -1;
    }
    r4.xy = ~(int2)r4.wz;
    r4.x = r4.x ? r4.y : 0;
    if (r4.x != 0) {
      r4.xy = offsetsAndWeights[7].xy + v1.xy;
      r1.xy = r4.xy * r1.xy;
      r6.xy = (int2)r1.xy;
      r6.zw = float2(0,0);
      r1.x = mrtTexture.Load(r6.xyz).z;
      r1.x = (uint)r1.x >> 24;
      r1.x = (int)r1.x & 4;
      r4.z = cmp((int)r1.x != 0);
      if (r4.z == 0) {
        r6.z = depthTexture.SampleLevel(samLinear_s, r4.xy, 0).x;
        r6.xy = r4.xy * float2(2,-2) + float2(-1,1);
        r6.w = 1;
        r1.x = dot(r6.xyzw, projInv_g._m02_m12_m22_m32);
        r1.y = dot(r6.xyzw, projInv_g._m03_m13_m23_m33);
        r1.x = r1.x / r1.y;
        r1.x = r1.x + -r1.z;
        r1.x = cmp(abs(r1.x) >= r1.w);
        r1.x = r1.x ? 1.000000 : 0;
        r3.w = r1.x * offsetsAndWeights[7].z + r3.w;
      }
    }
    if (4 == 0) r6.x = 0; else if (4+16 < 32) {     r6.x = (uint)r2.y << (32-(4 + 16)); r6.x = (uint)r6.x >> (32-4);    } else r6.x = (uint)r2.y >> 16;
    if (4 == 0) r6.y = 0; else if (4+20 < 32) {     r6.y = (uint)r2.z << (32-(4 + 20)); r6.y = (uint)r6.y >> (32-4);    } else r6.y = (uint)r2.z >> 20;
    if (4 == 0) r6.z = 0; else if (4+24 < 32) {     r6.z = (uint)r2.w << (32-(4 + 24)); r6.z = (uint)r6.z >> (32-4);    } else r6.z = (uint)r2.w >> 24;
    if (4 == 0) r6.w = 0; else if (4+28 < 32) {     r6.w = (uint)r2.x << (32-(4 + 28)); r6.w = (uint)r6.w >> (32-4);    } else r6.w = (uint)r2.x >> 28;
    r6.xyzw = (uint4)r6.xyzw;
    r1.x = 0.0666666701 * r6.w;
    r1.y = cmp(0 < r6.w);
    r1.w = r3.w * r3.w;
    r1.x = r1.w * r1.x;
    r1.x = density_g * r1.x;
    r1.z = fadeRangeInv_g * -r1.z;
    r1.xz = min(float2(1,1), r1.xz);
    r1.z = 1 + -r1.z;
    r1.x = r1.x * r1.z;
    r4.x = dot(lightDirection_g.xyz, view_g._m00_m10_m20);
    r4.y = dot(lightDirection_g.xyz, view_g._m01_m11_m21);
    r1.z = dot(r4.xy, r4.xy);
    r1.z = rsqrt(r1.z);
    r1.zw = r4.xy * r1.zz;
    r7.x = dot(r5.xyz, view_g._m00_m10_m20);
    r7.y = dot(r5.xyz, view_g._m01_m11_m21);
    r7.z = dot(r5.xyz, view_g._m02_m12_m22);
    r3.w = dot(r7.xyz, r7.xyz);
    r3.w = rsqrt(r3.w);
    r4.xy = r7.xy * r3.ww;
    r1.z = dot(r4.xy, r1.zw);
    r1.z = r1.z * 0.5 + 0.5;
    r1.z = log2(abs(r1.z));
    r1.z = 0.400000006 * r1.z;
    r1.z = exp2(r1.z);
    r1.x = r1.x * r1.z;
    r4.xyw = r6.xyz * float3(0.0666666701,0.0666666701,0.0666666701) + -r0.xyz;
    r1.xzw = r1.xxx * r4.xyw + r0.xyz;
    r1.xyz = r1.yyy ? r1.xzw : r0.xyz;
    o0.xyz = r4.zzz ? r0.xyz : r1.xyz;
    o0.w = r0.w;
    o1.xyzw = r2.yzwx;
    return;
  } else {
    r1.x = (int)r3.z & 8;
    if (r1.x == 0) {
      o0.xyzw = r0.xyzw;
      o1.xyzw = r2.yzwx;
      return;
    }
  }
  r1.z = depthTexture.SampleLevel(samPoint_s, v1.xy, 0).x;
  r4.xyzw = (uint4)r2.zwwy >> int4(16,8,16,16);
  r5.x = r2.z;
  r5.y = r4.x;
  r5.xy = (int2)r5.xy & int2(0xffff,0xffff);
  r6.xz = r2.yy;
  r6.yw = r4.ww;
  r6.xyzw = (int4)r6.xyzw & int4(0xffff,0xffff,0xffff,0xffff);
  r6.xyzw = (uint4)r6.xyzw;
  r6.xyzw = float4(1.52590219e-05,1.52590219e-05,1,1) * r6.xyzw;
  r0.w = (uint)r5.x;
  r7.xy = float2(1.52590219e-05,1) * r0.ww;
  r7.zw = r6.xy;
  r5.xzw = float3(-0.5,-0.5,-0.5) + r7.zwx;
  r7.xzw = r5.xzw + r5.xzw;
  r4.x = r2.w;
  r4.xyz = (int3)r4.xyz & int3(255,255,255);
  r4.xyz = (uint3)r4.xyz;
  r4.xyz = float3(0.00392156886,0.00392156886,0.00392156886) * r4.xyz;
  r2.yzw = (uint3)r2.xxx >> int3(8,16,24);
  r2.xyzw = (int4)r2.xyzw & int4(255,255,255,255);
  r2.xyzw = (uint4)r2.xyzw;
  r8.xyzw = float4(0.00392156886,0.00392156886,0.00392156886,0.00392156886) * r2.wxyz;
  r0.w = min(0x00004e1f, (uint)r5.y);
  r9.x = deferredParams_g[r0.w].shadowColor.x;
  r9.y = deferredParams_g[r0.w].shadowColor.y;
  r9.z = deferredParams_g[r0.w].shadowColor.z;
  r9.w = deferredParams_g[r0.w].emissive;
  r10.x = deferredParams_g[r0.w].specularColor.x;
  r10.y = deferredParams_g[r0.w].specularColor.y;
  r10.z = deferredParams_g[r0.w].specularColor.z;
  r10.w = deferredParams_g[r0.w].rimLightPower;
  r11.x = deferredParams_g[r0.w].rimLightColor.x;
  r11.y = deferredParams_g[r0.w].rimLightColor.y;
  r11.z = deferredParams_g[r0.w].rimLightColor.z;
  r11.w = deferredParams_g[r0.w].rimIntensity;
  r12.x = deferredParams_g[r0.w].fresnels.x;
  r12.y = deferredParams_g[r0.w].fresnels.y;
  r12.z = deferredParams_g[r0.w].fresnels.z;
  r13.x = deferredParams_g[r0.w].specularGlossinesses.x;
  r13.y = deferredParams_g[r0.w].specularGlossinesses.y;
  r13.z = deferredParams_g[r0.w].specularGlossinesses.z;
  r13.w = deferredParams_g[r0.w].dynamicLightIntensity;
  r14.x = deferredParams_g[r0.w].materialFogIntensity;
  r14.y = deferredParams_g[r0.w].metalness;
  r14.z = deferredParams_g[r0.w].roughness;
  r14.w = deferredParams_g[r0.w].cryRefractionIndex;
  r15.x = deferredParams_g[r0.w].cryFresnel;
  r15.y = deferredParams_g[r0.w].cryBrightness;
  r15.z = deferredParams_g[r0.w].cryBrightnessPower;
  r15.w = deferredParams_g[r0.w].glowIntensity;
  r2.x = deferredParams_g[r0.w].glowLumThreshold;
  r2.y = deferredParams_g[r0.w].glowShadowFadeRatio;
  r2.z = deferredParams_g[r0.w].ssaoIntensity;
  r5.x = deferredParams_g[r0.w].ssrDistance;
  r5.y = deferredParams_g[r0.w].ssrThickness;
  r0.w = deferredParams_g[r0.w].flag;
  r12.w = r13.x;
  r16.xz = r12.yz;
  r16.yw = r13.yz;
  r6.xy = r16.xy + -r12.xw;
  r6.xy = r8.yy * r6.xy + r12.xw;
  r12.xy = r16.zw + -r6.xy;
  r6.xy = r8.zz * r12.xy + r6.xy;
  r12.xyz = lightProbe_g[1].xyz * r7.xxx + lightProbe_g[0].xyz;
  r12.xyz = lightProbe_g[2].xyz * r7.zzz + r12.xyz;
  r12.xyz = lightProbe_g[3].xyz * r7.www + r12.xyz;
  r13.xyz = lightProbe_g[4].xyz * r7.www;
  r12.xyz = r13.xyz * r7.xxx + r12.xyz;
  r13.xyz = lightProbe_g[5].xyz * r7.zzz;
  r12.xyz = r13.xyz * r7.www + r12.xyz;
  r13.xyz = lightProbe_g[6].xyz * r7.zzz;
  r12.xyz = r13.xyz * r7.xxx + r12.xyz;
  r3.w = r7.w * r5.w;
  r3.w = r3.w * 6 + -1;
  r12.xyz = lightProbe_g[7].xyz * r3.www + r12.xyz;
  r3.w = r7.z * r7.z;
  r3.w = r7.x * r7.x + -r3.w;
  r12.xyz = lightProbe_g[8].xyz * r3.www + r12.xyz;
  r1.xy = v1.zw * float2(2,-2) + float2(-1,1);
  r1.w = 1;
  r16.x = dot(r1.xyzw, viewProjInv_g._m00_m10_m20_m30);
  r16.y = dot(r1.xyzw, viewProjInv_g._m01_m11_m21_m31);
  r16.z = dot(r1.xyzw, viewProjInv_g._m02_m12_m22_m32);
  r16.w = dot(r1.xyzw, viewProjInv_g._m03_m13_m23_m33);
  r1.xyzw = r16.xyzw / r16.wwww;
  r3.w = dot(view_g._m02_m12_m22_m32, r1.xyzw);
  r13.x = viewInv_g._m30;
  r13.y = viewInv_g._m31;
  r13.z = viewInv_g._m32;
  r16.xyz = r13.xyz + -r1.xyz;
  r4.w = dot(r16.xyz, r16.xyz);
  r4.w = rsqrt(r4.w);
  r17.xyz = r16.xyz * r4.www;
  r9.xyz = sceneShadowColor_g.xyz + r9.xyz;
  r9.xyz = min(float3(1,1,1), r9.xyz);
  r5.z = r9.w * r8.w;
  r5.w = dot(r7.xzw, r17.xyz);
  r18.xyzw = (int4)r0.wwww & int4(1,2,4,16);
  if (r18.x != 0) {
    r13.xyz = -r13.xyz + r1.xyz;
    r8.y = dot(r13.xyz, r13.xyz);
    r8.y = sqrt(r8.y);
    r8.z = 30 / shadowDistance_g;
    r9.w = shadowDistance_g + -5;
    r9.w = cmp(r9.w < r8.y);
    if (r9.w != 0) {
      r13.x = dot(r1.xyzw, farShadowMtx_g._m00_m10_m20_m30);
      r13.y = dot(r1.xyzw, farShadowMtx_g._m01_m11_m21_m31);
      r13.z = dot(r1.xyzw, farShadowMtx_g._m02_m12_m22_m32);
      r9.w = dot(r1.xyzw, farShadowMtx_g._m03_m13_m23_m33);
      r13.xyz = r13.xyz / r9.www;
      r19.xy = cmp(r13.xy < float2(0,0));
      r19.zw = cmp(float2(1,1) < r13.xy);
      r9.w = (int)r19.z | (int)r19.x;
      r9.w = (int)r19.y | (int)r9.w;
      r9.w = (int)r19.w | (int)r9.w;
      if (r9.w != 0) {
        r9.w = 1;
      } else {
        r13.xy = saturate(r13.xy);
        r9.w = farShadowMap.SampleCmpLevelZero(SmplShadow_s, r13.xy, r13.z).x;
      }
      r12.w = cmp(r8.y < shadowDistance_g);
      if (r12.w != 0) {
        r13.x = dot(r1.xyzw, shadowMtx_g[1]._m00_m10_m20_m30);
        r13.y = dot(r1.xyzw, shadowMtx_g[1]._m01_m11_m21_m31);
        r13.z = dot(r1.xyzw, shadowMtx_g[1]._m02_m12_m22_m32);
        r12.w = dot(r1.xyzw, shadowMtx_g[1]._m03_m13_m23_m33);
        r13.xyz = r13.xyz / r12.www;
        r19.xy = float2(0.000500000024,0.000250000012) * r8.zz;
        r12.w = cmp(0.5 < r13.y);
        r20.x = invShadowSize_g.x + 0.5;
        r20.yz = float2(1,0);
        r20.w = -invShadowSize_g.x + 0.5;
        r19.zw = r12.ww ? r20.xy : r20.zw;
        r12.w = dot(v0.xy, float2(0.0671105608,0.00583714992));
        r12.w = frac(r12.w);
        r12.w = 52.9829178 * r12.w;
        r12.w = frac(r12.w);
        r12.w = 6.28318548 * r12.w;
        r16.w = 0;
        r17.w = 0;
        while (true) {
          r18.x = cmp((int)r17.w >= 16);
          if (r18.x != 0) break;
          r18.x = (int)r17.w;
          r20.x = 0.5 + r18.x;
          r20.x = sqrt(r20.x);
          r20.x = 0.25 * r20.x;
          r18.x = r18.x * 2.4000001 + r12.w;
          sincos(r18.x, r18.x, r21.x);
          r21.x = r21.x * r20.x;
          r21.y = r20.x * r18.x;
          r20.xy = r21.xy * r19.xy + r13.xy;
          r18.x = max(r20.y, r19.z);
          r20.z = min(r18.x, r19.w);
          r18.x = shadowMap.SampleCmpLevelZero(SmplShadow_s, r20.xz, r13.z).x;
          r16.w = r18.x + r16.w;
          r17.w = (int)r17.w + 1;
        }
        r12.w = shadowDistance_g + -r8.y;
        r12.w = 0.200000003 * r12.w;
        r13.x = r16.w * 0.0625 + -r9.w;
        r9.w = r12.w * r13.x + r9.w;
      }
    } else {
      r12.w = cmp(r8.y < shadowSplitDistance_g);
      r19.xyzw = r12.wwww ? shadowMtx_g[0]._m00_m10_m20_m30 : shadowMtx_g[1]._m00_m10_m20_m30;
      r20.xyzw = r12.wwww ? shadowMtx_g[0]._m01_m11_m21_m31 : shadowMtx_g[1]._m01_m11_m21_m31;
      r21.xyzw = r12.wwww ? shadowMtx_g[0]._m02_m12_m22_m32 : shadowMtx_g[1]._m02_m12_m22_m32;
      r22.xyzw = r12.wwww ? shadowMtx_g[0]._m03_m13_m23_m33 : shadowMtx_g[1]._m03_m13_m23_m33;
      r13.x = dot(r1.xyzw, r19.xyzw);
      r13.y = dot(r1.xyzw, r20.xyzw);
      r13.z = dot(r1.xyzw, r21.xyzw);
      r16.w = dot(r1.xyzw, r22.xyzw);
      r13.xyz = r13.xyz / r16.www;
      r16.w = r12.w ? shadowZeroCascadeUVMult_g : 1;
      r16.w = r16.w * r8.z;
      r19.xy = float2(0.000500000024,0.000250000012) * r16.ww;
      r16.w = cmp(0.5 < r13.y);
      r20.x = invShadowSize_g.x + 0.5;
      r20.yz = float2(1,0);
      r20.w = -invShadowSize_g.x + 0.5;
      r19.zw = r16.ww ? r20.xy : r20.zw;
      r16.w = dot(v0.xy, float2(0.0671105608,0.00583714992));
      r16.w = frac(r16.w);
      r16.w = 52.9829178 * r16.w;
      r16.w = frac(r16.w);
      r16.w = 6.28318548 * r16.w;
      r17.w = 0;
      r18.x = 0;
      while (true) {
        r20.x = cmp((int)r18.x >= 16);
        if (r20.x != 0) break;
        r20.x = (int)r18.x;
        r20.y = 0.5 + r20.x;
        r20.y = sqrt(r20.y);
        r20.y = 0.25 * r20.y;
        r20.x = r20.x * 2.4000001 + r16.w;
        sincos(r20.x, r20.x, r21.x);
        r21.x = r21.x * r20.y;
        r21.y = r20.y * r20.x;
        r20.xy = r21.xy * r19.xy + r13.xy;
        r20.y = max(r20.y, r19.z);
        r20.z = min(r20.y, r19.w);
        r20.x = shadowMap.SampleCmpLevelZero(SmplShadow_s, r20.xz, r13.z).x;
        r17.w = r20.x + r17.w;
        r18.x = (int)r18.x + 1;
      }
      r9.w = 0.0625 * r17.w;
      r13.x = shadowSplitDistance_g + -1;
      r13.x = cmp(r13.x < r8.y);
      r12.w = r12.w ? r13.x : 0;
      if (r12.w != 0) {
        r13.x = dot(r1.xyzw, shadowMtx_g[1]._m00_m10_m20_m30);
        r13.y = dot(r1.xyzw, shadowMtx_g[1]._m01_m11_m21_m31);
        r13.z = dot(r1.xyzw, shadowMtx_g[1]._m02_m12_m22_m32);
        r12.w = dot(r1.xyzw, shadowMtx_g[1]._m03_m13_m23_m33);
        r13.xyz = r13.xyz / r12.www;
        r19.xy = float2(0.000500000024,0.000250000012) * r8.zz;
        r8.z = cmp(0.5 < r13.y);
        r20.x = invShadowSize_g.x + 0.5;
        r20.yz = float2(1,0);
        r20.w = -invShadowSize_g.x + 0.5;
        r19.zw = r8.zz ? r20.xy : r20.zw;
        r8.z = dot(v0.xy, float2(0.0671105608,0.00583714992));
        r8.z = frac(r8.z);
        r8.z = 52.9829178 * r8.z;
        r8.z = frac(r8.z);
        r8.z = 6.28318548 * r8.z;
        r12.w = 0;
        r16.w = 0;
        while (true) {
          r18.x = cmp((int)r16.w >= 16);
          if (r18.x != 0) break;
          r18.x = (int)r16.w;
          r20.x = 0.5 + r18.x;
          r20.x = sqrt(r20.x);
          r20.x = 0.25 * r20.x;
          r18.x = r18.x * 2.4000001 + r8.z;
          sincos(r18.x, r18.x, r21.x);
          r21.x = r21.x * r20.x;
          r21.y = r20.x * r18.x;
          r20.xy = r21.xy * r19.xy + r13.xy;
          r18.x = max(r20.y, r19.z);
          r20.z = min(r18.x, r19.w);
          r18.x = shadowMap.SampleCmpLevelZero(SmplShadow_s, r20.xz, r13.z).x;
          r12.w = r18.x + r12.w;
          r16.w = (int)r16.w + 1;
        }
        r8.z = 0.0625 * r12.w;
        r8.y = shadowSplitDistance_g + -r8.y;
        r12.w = r17.w * 0.0625 + -r8.z;
        r9.w = r8.y * r12.w + r8.z;
      }
    }
  } else {
    r9.w = 1;
  }
  r13.xyz = r16.xyz * r4.www + -lightDirection_g.xyz;
  r8.y = dot(r13.xyz, r13.xyz);
  r8.y = rsqrt(r8.y);
  r13.xyz = r13.xyz * r8.yyy;
  r8.y = lightSpecularGlossiness_g * r6.y;
  r8.z = saturate(dot(r13.xyz, r7.xzw));
  r8.y = max(0.00100000005, r8.y);
  r8.z = log2(r8.z);
  r8.y = r8.y * r8.z;
  r8.y = exp2(r8.y);
  r8.y = r8.y * r9.w;
  r8.y = lightSpecularIntensity_g * r8.y;
  r8.y = r18.y ? r8.y : 0;
  r10.xyz = r8.yyy * r10.xyz;
  r10.xyz = lightColor_g.xyz * r10.xyz;
  if (r18.z != 0) {
    r8.yz = r14.yz * r4.xy;
    r4.x = (int)r0.w & 32;
    if (r4.x != 0) {
      r13.xy = v1.xy * float2(1,-1) + float2(0,1);
      r13.xyz = texMirror_g.SampleLevel(SmplLinearClamp_s, r13.xy, 0).xyz;
    } else {
      r12.w = r5.w + r5.w;
      r19.xyz = r7.xzw * -r12.www + r17.xyz;
      {
        uint cubeDim;
        texEnvMap_g.GetDimensions(0, cubeDim, cubeDim, cubeDim);
        r12.w = (float)(cubeDim - 1);
      }
      r19.xyz = float3(1, -1, -1) * r19.xyz;
      r12.w = r8.z * r12.w;
      r13.xyz = texEnvMap_g.SampleLevel(SmplCube_s, r19.xyz, r12.w * lerp(1.0, cubemap_lighting_mip_boost, cubemap_improved_factor)).xyz;
    }
    r12.w = (int)r3.z & 2;
    if (r12.w != 0) {
      r19.xyzw = texSSRMap_g.SampleLevel(SmplLinearClamp_s, v1.xy, 0).xyzw;
      r19.xyz = r19.xyz + -r13.xyz;
      r13.xyz = r19.www * r19.xyz + r13.xyz;
    }
    r12.w = cmp(0 < r6.x);
    r14.y = 1 + -abs(r5.w);
    r14.y = max(0, r14.y);
    r14.y = log2(r14.y);
    r6.x = r14.y * r6.x;
    r6.x = exp2(r6.x);
    r6.x = r12.w ? r6.x : 1;
    r12.w = r8.y * r6.x;
    r19.xyz = r0.xyz * r13.xyz + -r0.xyz;
    r19.xyz = r12.www * r19.xyz + r0.xyz;
    r12.w = dot(r13.xyz, float3(0.298999995,0.587000012,0.114));
    r8.z = r8.z * -9 + 10;
    r12.w = log2(r12.w);
    r8.z = r12.w * r8.z;
    r8.z = exp2(r8.z);
    r12.w = 1 + -r8.z;
    r8.y = r8.y * r12.w + r8.z;
    r13.xyz = r13.xyz * r8.yyy;
    r13.xyz = r13.xyz * r6.xxx;
    r6.x = -r4.y * r14.z + 1;
    if (cubemap_improved_factor >= 0.5 && r4.x == 0) {
      float skylight_lum = max(0, dot(r13.xyz, float3(0.2126, 0.7152, 0.0722)));
      float skylight_factor = smoothstep(0.0, 0.25, skylight_lum);
      skylight_factor *= lerp(0.5, 1.0, saturate(r14.z));
      skylight_factor *= lerp(0.4, 1.0, saturate(r6.x));
      skylight_factor = lerp(0.3, 1.0, skylight_factor);
      r13.xyz = max(float3(0, 0, 0), r13.xyz * skylight_factor);
    }
    r13.xyz = r13.xyz * r6.xxx + r10.xyz;
    r10.xyz = r4.xxx ? r10.xyz : r13.xyz;
  } else {
    r0.w = (int)r0.w & 8;
    if (r0.w != 0) {
      r0.w = r5.w + r5.w;
      r13.xyz = r7.xzw * -r0.www + r17.xyz;
      r0.w = 1 / r14.w;
      r4.x = dot(-r17.xyz, r7.xzw);
      r6.x = r0.w * r0.w;
      r8.y = -r4.x * r4.x + 1;
      r6.x = -r6.x * r8.y + 1;
      r8.y = sqrt(r6.x);
      r4.x = r0.w * r4.x + r8.y;
      r6.x = cmp(r6.x >= 0);
      r20.xyz = r4.xxx * r7.xzw;
      r17.xyz = r0.www * -r17.xyz + -r20.xyz;
      r17.xyz = r6.xxx ? r17.xyz : 0;
      r0.w = r14.z * r4.y;
      {
        uint cubeDim;
        texEnvMap_g.GetDimensions(0, cubeDim, cubeDim, cubeDim);
        r4.x = (float)(cubeDim - 1);
      }
      r13.xyz = float3(1, -1, -1) * r13.xyz;
      r0.w = r4.x * r0.w;
      r13.xyz = texEnvMap_g.SampleLevel(SmplCube_s, r13.xyz, r0.w * lerp(1.0, cubemap_lighting_mip_boost, cubemap_improved_factor)).xyz;
      r4.x = (int)r3.z & 2;
      if (r4.x != 0) {
        r20.xyzw = texSSRMap_g.SampleLevel(SmplLinearClamp_s, v1.xy, 0).xyzw;
        r20.xyz = r20.xyz + -r13.xyz;
        r13.xyz = r20.www * r20.xyz + r13.xyz;
      }
      r17.xyz = texEnvMap_g.SampleLevel(SmplCube_s, r17.xyz, r0.w * lerp(1.0, cubemap_lighting_mip_boost, cubemap_improved_factor)).xyz;
      r0.w = cmp(0 < r15.x);
      r4.x = 1 + -abs(r5.w);
      r4.x = max(0, r4.x);
      r4.x = log2(r4.x);
      r4.x = r15.x * r4.x;
      r4.x = exp2(r4.x);
      r0.w = r0.w ? r4.x : 1;
      r20.xyz = r0.xyz * r13.xyz + -r0.xyz;
      r20.xyz = r0.www * r20.xyz + r0.xyz;
      r2.w = -r2.w * 0.00392156886 + 1;
      r8.x = r0.w * r2.w + r8.x;
      r2.w = abs(r5.w) * r15.y;
      r2.w = log2(r2.w);
      r2.w = r15.z * r2.w;
      r2.w = exp2(r2.w);
      r13.xyz = r8.xxx * r13.xyz;
      r13.xyz = r13.xyz * r0.www;
      r0.w = -r4.y * r14.z + 1;
      r10.xyz = r13.xyz * r0.www + r10.xyz;
      r13.xyz = r2.www * r17.xyz;
      r19.xyz = r20.xyz * r13.xyz;
    } else {
      r19.xyz = r0.xyz;
    }
  }
  r0.w = r8.x * r9.w;
  r8.xyz = float3(1,1,1) + -r9.xyz;
  r8.xyz = r0.www * r8.xyz + r9.xyz;
  r8.xyz = r8.xyz * lightColor_g.xyz + r12.xyz;
  r0.w = min(1, r5.z);
  r9.xyz = float3(1,1,1) + -r8.xyz;
  r8.xyz = r0.www * r9.xyz + r8.xyz;
  r0.w = 1 + -abs(r5.w);
  r0.w = max(0, r0.w);
  r0.w = r0.w * r11.w;
  r0.w = log2(r0.w);
  r0.w = r10.w * r0.w;
  r0.w = exp2(r0.w);
  r0.w = min(1, r0.w);
  r9.xyz = r11.xyz * r0.www + r10.xyz;
  if (r18.y != 0) {
    r4.xy = v0.xy * float2(lightTileWidthInv_g, lightTileHeightInv_g);
    r0.w = lightTileDepthInv_g * -r3.w;
    r0.w = min(7, r0.w);
    r0.w = max(0, r0.w);
    r4.xy = (uint2)r4.xy;
    r0.w = (uint)r0.w;
    r2.w = (uint)r4.y << 5;
    r0.w = mad((int)r0.w, 576, (int)r2.w);
    r0.w = (int)r4.x + (int)r0.w;
    r0.w = min(4607, (uint)r0.w);
    r2.w = lightIndices_g[r0.w].pointLightCount;
    r2.w = min(63, (uint)r2.w);
    r10.xyz = float3(0,0,0);
    r11.xyz = float3(0,0,0);
    r4.x = 0;
    while (true) {
      r4.y = cmp((uint)r4.x >= (uint)r2.w);
      if (r4.y != 0) break;
      r4.y = lightIndices_g[r0.w].pointLightIndices[(int)r4.x];
      r12.x = dynamicLights_g[r4.y].pos.x;
      r12.y = dynamicLights_g[r4.y].pos.y;
      r12.z = dynamicLights_g[r4.y].pos.z;
      r12.xyz = r12.xyz + -r1.xyz;
      r5.z = dot(r12.xyz, r12.xyz);
      r5.w = sqrt(r5.z);
      r6.x = dynamicLights_g[r4.y].radiusInv;
      r5.w = r6.x * r5.w;
      r6.x = dynamicLights_g[r4.y].attenuation;
      r5.w = log2(abs(r5.w));
      r5.w = r6.x * r5.w;
      r5.w = exp2(r5.w);
      r5.w = 1 + -r5.w;
      r5.w = max(0, r5.w);
      r6.x = cmp(0 < r5.w);
      if (r6.x != 0) {
        r5.z = rsqrt(r5.z);
        r12.xyz = r12.xyz * r5.zzz;
        r5.z = dynamicLights_g[r4.y].translucency;
        r6.x = dot(r12.xyz, r7.xzw);
        r5.z = max(r6.x, r5.z);
        r5.z = r5.w * r5.z;
        r13.x = dynamicLights_g[r4.y].color.x;
        r13.y = dynamicLights_g[r4.y].color.y;
        r13.z = dynamicLights_g[r4.y].color.z;
        r11.xyz = r13.xyz * r5.zzz + r11.xyz;
        r12.xyz = r16.xyz * r4.www + r12.xyz;
        r5.w = dot(r12.xyz, r12.xyz);
        r5.w = rsqrt(r5.w);
        r12.xyz = r12.xyz * r5.www;
        r14.y = dynamicLights_g[r4.y].specularIntensity;
        r14.z = dynamicLights_g[r4.y].specularGlossiness;
        r4.y = r14.z * r6.y;
        r5.w = saturate(dot(r12.xyz, r7.xzw));
        r4.y = max(0.00100000005, r4.y);
        r5.w = log2(r5.w);
        r4.y = r5.w * r4.y;
        r4.y = exp2(r4.y);
        r12.xyz = r13.xyz * r4.yyy;
        r12.xyz = r12.xyz * r5.zzz;
        r10.xyz = r12.xyz * r14.yyy + r10.xyz;
      }
      r4.x = (int)r4.x + 1;
    }
    r11.xyz = r11.xyz * r13.www + r8.xyz;
    r2.w = lightIndices_g[r0.w].spotLightCount;
    r2.w = min(63, (uint)r2.w);
    r12.xyz = r10.xyz;
    r13.xyz = float3(0,0,0);
    r4.x = 0;
    while (true) {
      r4.y = cmp((uint)r4.x >= (uint)r2.w);
      if (r4.y != 0) break;
      r4.y = lightIndices_g[r0.w].spotLightIndices[(int)r4.x];
      r14.y = dynamicLights_g[r4.y].pos.x;
      r14.z = dynamicLights_g[r4.y].pos.y;
      r14.w = dynamicLights_g[r4.y].pos.z;
      r14.yzw = r14.yzw + -r1.xyz;
      r5.z = dot(r14.yzw, r14.yzw);
      r5.w = rsqrt(r5.z);
      r14.yzw = r14.yzw * r5.www;
      r17.x = dynamicLights_g[r4.y].vec.x;
      r17.y = dynamicLights_g[r4.y].vec.y;
      r17.z = dynamicLights_g[r4.y].vec.z;
      r17.w = dynamicLights_g[r4.y].spotAngleInv;
      r5.w = dot(r14.yzw, r17.xyz);
      r5.w = max(0, r5.w);
      r5.w = 1 + -r5.w;
      r5.w = r5.w * r17.w;
      r6.x = dynamicLights_g[r4.y].attenuationAngle;
      r5.w = log2(r5.w);
      r5.w = r6.x * r5.w;
      r5.w = exp2(r5.w);
      r5.w = 1 + -r5.w;
      r5.w = max(0, r5.w);
      r6.x = cmp(0 < r5.w);
      if (r6.x != 0) {
        r5.z = sqrt(r5.z);
        r6.x = dynamicLights_g[r4.y].radiusInv;
        r5.z = r6.x * r5.z;
        r6.x = dynamicLights_g[r4.y].attenuation;
        r5.z = log2(abs(r5.z));
        r5.z = r6.x * r5.z;
        r5.z = exp2(r5.z);
        r5.z = 1 + -r5.z;
        r5.z = max(0, r5.z);
        r5.z = r5.w * r5.z;
        r5.w = cmp(0 < r5.z);
        if (r5.w != 0) {
          r15.x = dynamicLights_g[r4.y].translucency;
          r15.y = dynamicLights_g[r4.y].shadowmapIndex;
          r5.w = cmp((int)r15.y != -1);
          if (r5.w != 0) {
            r17.xyzw = spotShadowMatrices_g[r15.y]._m00_m10_m20_m30;
            r20.xyzw = spotShadowMatrices_g[r15.y]._m01_m11_m21_m31;
            r21.xyzw = spotShadowMatrices_g[r15.y]._m02_m12_m22_m32;
            r22.xyzw = spotShadowMatrices_g[r15.y]._m03_m13_m23_m33;
            r17.x = dot(r1.xyzw, r17.xyzw);
            r17.y = dot(r1.xyzw, r20.xyzw);
            r17.z = dot(r1.xyzw, r21.xyzw);
            r5.w = dot(r1.xyzw, r22.xyzw);
            r17.xyz = r17.xyz / r5.www;
            r17.w = (int)r15.y;
            r5.w = spotShadowMaps.SampleCmpLevelZero(SmplShadow_s, r17.xyw, r17.z).x;
            r5.z = r5.z * r5.w;
          }
          r5.w = dot(r14.yzw, r7.xzw);
          r5.w = max(r15.x, r5.w);
          r5.z = r5.z * r5.w;
          r15.x = dynamicLights_g[r4.y].color.x;
          r15.y = dynamicLights_g[r4.y].color.y;
          r15.z = dynamicLights_g[r4.y].color.z;
          r13.xyz = r15.xyz * r5.zzz + r13.xyz;
          r14.yzw = r16.xyz * r4.www + r14.yzw;
          r5.w = dot(r14.yzw, r14.yzw);
          r5.w = rsqrt(r5.w);
          r14.yzw = r14.yzw * r5.www;
          r17.x = dynamicLights_g[r4.y].specularIntensity;
          r17.y = dynamicLights_g[r4.y].specularGlossiness;
          r4.y = r17.y * r6.y;
          r5.w = saturate(dot(r14.yzw, r7.xzw));
          r4.y = max(0.00100000005, r4.y);
          r5.w = log2(r5.w);
          r4.y = r5.w * r4.y;
          r4.y = exp2(r4.y);
          r14.yzw = r15.xyz * r4.yyy;
          r14.yzw = r14.yzw * r5.zzz;
          r12.xyz = r14.yzw * r17.xxx + r12.xyz;
        }
      }
      r4.x = (int)r4.x + 1;
    }
    r4.xyw = r13.xyz * r13.www + r11.xyz;
    r9.xyz = r12.xyz * r13.www + r9.xyz;
  } else {
    r5.zw = v0.xy * float2(lightTileWidthInv_g, lightTileHeightInv_g);
    r0.w = lightTileDepthInv_g * -r3.w;
    r0.w = min(7, r0.w);
    r0.w = max(0, r0.w);
    r5.zw = (uint2)r5.zw;
    r0.w = (uint)r0.w;
    r2.w = (uint)r5.w << 5;
    r0.w = mad((int)r0.w, 576, (int)r2.w);
    r0.w = (int)r5.z + (int)r0.w;
    r0.w = min(4607, (uint)r0.w);
    r2.w = lightIndices_g[r0.w].pointLightCount;
    r2.w = min(63, (uint)r2.w);
    r10.xyzw = float4(0,0,0,0);
    while (true) {
      r5.z = cmp((uint)r10.w >= (uint)r2.w);
      if (r5.z != 0) break;
      r5.z = lightIndices_g[r0.w].pointLightIndices[(int)r10.w];
      r11.x = dynamicLights_g[r5.z].pos.x;
      r11.y = dynamicLights_g[r5.z].pos.y;
      r11.z = dynamicLights_g[r5.z].pos.z;
      r11.xyz = r11.xyz + -r1.xyz;
      r5.w = dot(r11.xyz, r11.xyz);
      r6.x = sqrt(r5.w);
      r6.y = dynamicLights_g[r5.z].radiusInv;
      r6.x = r6.x * r6.y;
      r6.y = dynamicLights_g[r5.z].attenuation;
      r6.x = log2(abs(r6.x));
      r6.x = r6.y * r6.x;
      r6.x = exp2(r6.x);
      r6.x = 1 + -r6.x;
      r6.x = max(0, r6.x);
      r6.y = cmp(0 < r6.x);
      if (r6.y != 0) {
        r6.y = dynamicLights_g[r5.z].translucency;
        r5.w = rsqrt(r5.w);
        r11.xyz = r11.xyz * r5.www;
        r5.w = dot(r11.xyz, r7.xzw);
        r5.w = max(r6.y, r5.w);
        r11.x = dynamicLights_g[r5.z].color.x;
        r11.y = dynamicLights_g[r5.z].color.y;
        r11.z = dynamicLights_g[r5.z].color.z;
        r11.xyz = r11.xyz * r6.xxx;
        r10.xyz = r11.xyz * r5.www + r10.xyz;
      }
      r10.w = (int)r10.w + 1;
    }
    r8.xyz = r10.xyz * r13.www + r8.xyz;
    r2.w = lightIndices_g[r0.w].spotLightCount;
    r2.w = min(63, (uint)r2.w);
    r10.xyzw = float4(0,0,0,0);
    while (true) {
      r5.z = cmp((uint)r10.w >= (uint)r2.w);
      if (r5.z != 0) break;
      r5.z = lightIndices_g[r0.w].spotLightIndices[(int)r10.w];
      r11.x = dynamicLights_g[r5.z].pos.x;
      r11.y = dynamicLights_g[r5.z].pos.y;
      r11.z = dynamicLights_g[r5.z].pos.z;
      r11.xyz = r11.xyz + -r1.xyz;
      r5.w = dot(r11.xyz, r11.xyz);
      r6.x = rsqrt(r5.w);
      r11.xyz = r11.xyz * r6.xxx;
      r12.x = dynamicLights_g[r5.z].vec.x;
      r12.y = dynamicLights_g[r5.z].vec.y;
      r12.z = dynamicLights_g[r5.z].vec.z;
      r12.w = dynamicLights_g[r5.z].spotAngleInv;
      r6.x = dot(r11.xyz, r12.xyz);
      r6.x = max(0, r6.x);
      r6.x = 1 + -r6.x;
      r6.x = r6.x * r12.w;
      r6.y = dynamicLights_g[r5.z].attenuationAngle;
      r6.x = log2(r6.x);
      r6.x = r6.y * r6.x;
      r6.x = exp2(r6.x);
      r6.x = 1 + -r6.x;
      r6.x = max(0, r6.x);
      r6.y = cmp(0 < r6.x);
      if (r6.y != 0) {
        r5.w = sqrt(r5.w);
        r6.y = dynamicLights_g[r5.z].radiusInv;
        r5.w = r6.y * r5.w;
        r6.y = dynamicLights_g[r5.z].attenuation;
        r5.w = log2(abs(r5.w));
        r5.w = r6.y * r5.w;
        r5.w = exp2(r5.w);
        r5.w = 1 + -r5.w;
        r5.w = max(0, r5.w);
        r5.w = r6.x * r5.w;
        r6.x = cmp(0 < r5.w);
        if (r6.x != 0) {
          r6.x = dynamicLights_g[r5.z].translucency;
          r6.y = dynamicLights_g[r5.z].shadowmapIndex;
          r11.w = cmp((int)r6.y != -1);
          if (r11.w != 0) {
            r12.xyzw = spotShadowMatrices_g[r6.y]._m00_m10_m20_m30;
            r16.xyzw = spotShadowMatrices_g[r6.y]._m01_m11_m21_m31;
            r17.xyzw = spotShadowMatrices_g[r6.y]._m02_m12_m22_m32;
            r20.xyzw = spotShadowMatrices_g[r6.y]._m03_m13_m23_m33;
            r12.x = dot(r1.xyzw, r12.xyzw);
            r12.y = dot(r1.xyzw, r16.xyzw);
            r12.z = dot(r1.xyzw, r17.xyzw);
            r11.w = dot(r1.xyzw, r20.xyzw);
            r12.xyz = r12.xyz / r11.www;
            r12.w = (int)r6.y;
            r6.y = spotShadowMaps.SampleCmpLevelZero(SmplShadow_s, r12.xyw, r12.z).x;
            r5.w = r6.y * r5.w;
          }
          r6.y = dot(r11.xyz, r7.xzw);
          r6.x = max(r6.x, r6.y);
          r11.x = dynamicLights_g[r5.z].color.x;
          r11.y = dynamicLights_g[r5.z].color.y;
          r11.z = dynamicLights_g[r5.z].color.z;
          r11.xyz = r11.xyz * r5.www;
          r10.xyz = r11.xyz * r6.xxx + r10.xyz;
        }
      }
      r10.w = (int)r10.w + 1;
    }
    r4.xyw = r10.xyz * r13.www + r8.xyz;
  }
  r7.xzw = r9.xyz * r4.zzz;
  r4.xyz = r19.xyz * r4.xyw + r7.xzw;
  r0.w = cmp(0 < r2.x);
  r0.x = dot(r0.xyz, float3(0.298999995,0.587000012,0.114));
  r0.x = r0.x + -r2.x;
  r0.x = max(0, r0.x);
  r0.x = r0.x / r2.x;
  r0.x = min(1, r0.x);
  r0.x = r0.w ? r0.x : 1;
  r0.y = r15.w * r8.w;
  r0.x = r0.y * r0.x;
  r0.y = -1 + r9.w;
  r0.y = r2.y * r0.y + 1;
  r0.x = r0.x * r0.y;
  r0.y = -fogNearDistance_g + -r3.w;
  r0.y = saturate(fogFadeRangeInv_g * r0.y);
  r0.z = -fogHeight_g + r1.y;
  r0.z = saturate(fogHeightRangeInv_g * r0.z);
  r0.y = r0.y * r0.z;
  r0.z = fogIntensity_g * r0.y;
  r0.z = r0.z * r14.x;
  r2.xyw = fogColor_g.xyz + -r4.xyz;
  r2.xyw = r0.zzz * r2.xyw + r4.xyz;
  if (outlineShapeCount_g != 0) {
    r0.zw = -outlineShapeMaskUVParam_g.xy + r1.xz;
    r0.zw = outlineShapeMaskUVParam_g.zw * r0.zw;
    r4.xy = cmp(r0.zw >= float2(0,0));
    r3.w = r4.y ? r4.x : 0;
    r4.xy = cmp(float2(1,1) >= r0.zw);
    r4.x = r4.y ? r4.x : 0;
    r3.w = r4.x ? r3.w : 0;
    if (r3.w != 0) {
      r0.z = outlineShapeMask.SampleLevel(samLinear_s, r0.zw, 0).x;
    } else {
      r0.z = 1;
    }
    r1.w = 1;
    r4.xyz = r2.xyw;
    r0.w = 0;
    while (true) {
      r3.w = cmp((uint)r0.w >= outlineShapeCount_g);
      if (r3.w != 0) break;
      r8.x = outlineShapes_g[r0.w].gradation_sharpness;
      r8.y = outlineShapes_g[r0.w].height_base;
      r8.z = outlineShapes_g[r0.w].height_width;
      r8.w = outlineShapes_g[r0.w].height_gradation_width;
      r3.w = -r8.y + r1.y;
      r4.w = cmp(r8.z >= abs(r3.w));
      if (r4.w != 0) {
        r9.x = outlineShapes_g[r0.w].mtx._m00;
        r9.y = outlineShapes_g[r0.w].mtx._m10;
        r9.z = outlineShapes_g[r0.w].mtx._m20;
        r9.w = outlineShapes_g[r0.w].mtx._m30;
        r10.x = outlineShapes_g[r0.w].mtx._m02;
        r10.y = outlineShapes_g[r0.w].mtx._m32;
        r10.z = outlineShapes_g[r0.w].mtx._m12;
        r10.w = outlineShapes_g[r0.w].mtx._m22;
        r11.x = outlineShapes_g[r0.w].color.x;
        r11.y = outlineShapes_g[r0.w].color.y;
        r11.z = outlineShapes_g[r0.w].color.z;
        r11.w = outlineShapes_g[r0.w].color.w;
        r12.x = outlineShapes_g[r0.w].type;
        r12.y = outlineShapes_g[r0.w].radius;
        r12.z = outlineShapes_g[r0.w].gradation_size.x;
        r12.w = outlineShapes_g[r0.w].gradation_size.y;
        r4.w = abs(r3.w) + r8.w;
        r4.w = cmp(r8.z < r4.w);
        r3.w = r8.z + -abs(r3.w);
        r3.w = r3.w / r8.w;
        r3.w = r4.w ? r3.w : 1;
        if (r12.x == 0) {
          r10.x = r9.w;
          r5.zw = -r10.xy + r1.xz;
          r4.w = dot(r5.zw, r5.zw);
          r4.w = sqrt(r4.w);
          r5.z = cmp(r12.y < r4.w);
          r5.w = r12.y + -r12.z;
          r6.x = cmp(r4.w >= r5.w);
          r6.y = ~(int)r6.x;
          r4.w = -r5.w + r4.w;
          r4.w = r4.w / r12.z;
          r4.w = r6.x ? r4.w : 0;
          r4.w = r5.z ? 0 : r4.w;
          r5.z = (int)r5.z | (int)r6.y;
        } else {
          r5.w = cmp((int)r12.x == 1);
          if (r5.w != 0) {
            r5.w = outlineShapes_g[r0.w].fan_angle;
            r10.xz = r9.wz;
            r6.xy = -r10.xy + r1.xz;
            r7.x = dot(r6.xy, r6.xy);
            r7.z = sqrt(r7.x);
            r7.w = cmp(r12.y < r7.z);
            r8.y = r12.y + -r12.z;
            r7.x = rsqrt(r7.x);
            r6.xy = r7.xx * r6.xy;
            r6.x = dot(r10.zw, r6.xy);
            r6.y = 1 + -abs(r6.x);
            r6.y = sqrt(r6.y);
            r7.x = abs(r6.x) * -0.0187292993 + 0.0742610022;
            r7.x = r7.x * abs(r6.x) + -0.212114394;
            r7.x = r7.x * abs(r6.x) + 1.57072878;
            r8.z = r7.x * r6.y;
            r8.z = r8.z * -2 + 3.14159274;
            r6.x = cmp(r6.x < -r6.x);
            r6.x = r6.x ? r8.z : 0;
            r6.x = r7.x * r6.y + r6.x;
            r6.y = cmp(r5.w >= r6.x);
            r7.x = ~(int)r6.y;
            r8.z = 6.28318548 * r7.z;
            r6.x = r8.z * r6.x;
            r6.x = 0.159154937 * r6.x;
            r5.w = r8.z * r5.w;
            r5.w = r5.w * 0.159154937 + -r6.x;
            r5.w = r5.w / r12.z;
            r5.w = min(1, r5.w);
            r5.w = 1 + -r5.w;
            r6.x = -r8.y + r7.z;
            r6.x = r6.x / r12.z;
            r6.x = min(1, r6.x);
            r5.w = max(r6.x, r5.w);
            r5.w = r6.y ? r5.w : 0;
            r4.w = r7.w ? 0 : r5.w;
            r5.z = (int)r7.w | (int)r7.x;
          } else {
            r5.w = cmp((int)r12.x == 2);
            r6.x = dot(r1.xyzw, r9.xyzw);
            r6.y = dot(r1.xwyz, r10.xyzw);
            r7.xz = cmp(r6.xy < float2(0.5,0.5));
            r7.x = r7.z ? r7.x : 0;
            r7.zw = cmp(float2(-0.5,-0.5) < r6.xy);
            r7.z = r7.w ? r7.z : 0;
            r7.x = r7.z ? r7.x : 0;
            r7.zw = float2(0.5,0.5) + -r12.zw;
            r8.yz = cmp(abs(r6.xy) < r7.zw);
            r8.y = r8.z ? r8.y : 0;
            r6.xy = -r7.zw + abs(r6.xy);
            r6.xy = r6.xy / r12.zw;
            r6.x = max(r6.x, r6.y);
            r6.x = r8.y ? 0 : r6.x;
            r6.x = r7.x ? r6.x : 0;
            r6.y = r7.x ? r8.y : -1;
            r4.w = r5.w ? r6.x : 0;
            r5.z = r5.w ? r6.y : 0;
          }
        }
        r4.w = log2(r4.w);
        r4.w = r8.x * r4.w;
        r4.w = exp2(r4.w);
        r4.w = r11.w * r4.w;
        r4.w = r4.w * r0.z;
        r3.w = r4.w * r3.w;
        r7.xzw = r11.xyz + -r4.xyz;
        r7.xzw = r3.www * r7.xzw + r4.xyz;
        r4.xyz = r5.zzz ? r4.xyz : r7.xzw;
      }
      r0.w = (int)r0.w + 1;
    }
    r2.xyw = r4.xyz;
  }
  r0.y = -r0.y * fogIntensity_g + 1;
  r0.y = r2.z * r0.y;
  {
    uint u6z = (uint)r6.z;
    uint u6w = (uint)r6.w;
    o1.x = u6w * 0x00010000u + u6z;
  }
  {
    float tz = 0.255000025f * r5.x;
    float tw = 2.54999995f * r5.y;
    uint u0_z = (uint)tz;
    uint u0_w = (uint)tw;
    uint u1_x = (uint)r7.y;
    u0_w = u0_w << 24;
    u0_z = (u1_x & 0x0000FFFFu) | ((u0_z & 0x0000FFFFu) << 16);
    o1.y = u0_w | u0_z;
  }
  r0.y = 255 * r0.y;
  {
    uint u0_y = (uint)r0.y;
    uint u3_z = (uint)r3.z;
    uint py = u0_y << 8;
    uint pz = u0_y << 16;
    uint pw = u3_z << 24;
    py = py | u0_y;
    py = pz | py;
    o1.z = pw | py;
  }
  r0.x = saturate(0.100000001f * r0.x);
  r0.x = 65535 * r0.x;
  {
    uint u0_x = (uint)r0.x;
    u0_x = u0_x << 16;
    o1.w = r18.w ? u0_x : 0u;
  }
  o0.xyz = r2.xyw;
  o0.w = 1;
  return;
}