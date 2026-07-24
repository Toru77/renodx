// ---- Created with 3Dmigoto v1.4.1 on Fri Jul 24 03:45:03 2026

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
    float reserve[3];              // Offset:  128
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

struct RoundShadowParam
{
    float3 pos;                    // Offset:    0
    float radius;                  // Offset:   12
    float gradation_size;          // Offset:   16
    float inner_radius;            // Offset:   20
    float alpha;                   // Offset:   24
    float height_width;            // Offset:   28
    uint type;                     // Offset:   32
    uint3 pad;                     // Offset:   36
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

cbuffer SceneConstantData : register(b0)
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
  float fogChrIntensity_g : packoffset(c26.w);
  float3 chrLightDir_g : packoffset(c27);
  float padding_g : packoffset(c27.w);
  float3 sceneShadowColor_g : packoffset(c28);
  float disableMapObjNearFade_g : packoffset(c28.w);
  float3 windDirection_g : packoffset(c29);
  float windForce_g : packoffset(c29.w);
  float windWaveTime_g : packoffset(c30);
  float windWaveFrequency_g : packoffset(c30.y);
  float lightTileWidthInv_g : packoffset(c30.z);
  float lightTileHeightInv_g : packoffset(c30.w);
  float3 fogColor_g : packoffset(c31);
  float fogIntensity_g : packoffset(c31.w);
  float fogHeight_g : packoffset(c32);
  float fogHeightRangeInv_g : packoffset(c32.y);
  float fogNearDistance_g : packoffset(c32.z);
  float fogFadeRangeInv_g : packoffset(c32.w);
  uint localLightProbeCount_g : packoffset(c33);
  float lightSpecularGlossiness_g : packoffset(c33.y);
  float lightSpecularIntensity_g : packoffset(c33.z);
  float lightTileDepthInv_g : packoffset(c33.w);
  float4x4 ditherMtx_g : packoffset(c34);
  float4 lightProbe_g[9] : packoffset(c38);
  float2 resolutionScaling_g : packoffset(c47);
  float sceneTime_g : packoffset(c47.z);
  float gameTime_g : packoffset(c47.w);
  float4 mapColor_g : packoffset(c48);
  float4 clipPlane_g : packoffset(c49);
  float3 debugData_g : packoffset(c50);
  uint debugFlag_g : packoffset(c50.w);
  float4x4 shadowMtx_g[3] : packoffset(c51);
  float2 shadowSplitDistance_g : packoffset(c63);
  float shadowFadeNear_g : packoffset(c63.z);
  float shadowFadeRangeInv_g : packoffset(c63.w);
  float2 invShadowSize_g : packoffset(c64);
  float2 cameraNearFar_g : packoffset(c64.z);
  float4 frustumPlanes_g[6] : packoffset(c65);
  float4 chrSilhouetteColor_g : packoffset(c71);
  float4x4 prevViewProj_g : packoffset(c72);
  float2 jitterDiff_g : packoffset(c76);
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

cbuffer DeferredConstantData : register(b3)
{
  uint outlineShapeCount_g : packoffset(c0);
  uint roundShadowCount_g : packoffset(c0.y);
  float2 pad : packoffset(c0.z);
}

SamplerState samPoint_s : register(s0);
SamplerState samLinear_s : register(s1);
SamplerState SmplCube_s : register(s14);
SamplerState SmplLinearClamp_s : register(s15);
SamplerComparisonState SmplShadow_s : register(s13);
Texture2D<float4> colorTexture : register(t0);
Texture2D<uint4> mrtTexture0 : register(t1);
Texture2D<uint4> mrtTexture1 : register(t2);
Texture2D<uint2> mrtTexture2 : register(t3);
Texture2D<float4> depthTexture : register(t4);
StructuredBuffer<DeferredParam> deferredParams_g : register(t6);
StructuredBuffer<OutlineShapeParam> outlineShapes_g : register(t7);
StructuredBuffer<RoundShadowParam> roundShadows_g : register(t8);
Texture2D<float4> outlinePrepareTexture : register(t10);
StructuredBuffer<LightParam> dynamicLights_g : register(t11);
StructuredBuffer<LightIndexData> lightIndices_g : register(t13);
StructuredBuffer<float4x4> spotShadowMatrices_g : register(t15);
Texture2DArray<float4> shadowMaps : register(t16);
TextureCube<float4> texEnvMap_g : register(t17);
Texture2DArray<float4> spotShadowMaps : register(t24);
Texture2D<float4> texMirror_g : register(t26);
Texture2D<uint4> gtvbaoTexture : register(t22);  // GTVBAO AO (r32_uint, packed 0-255)
Texture2D<float4> vbgiTexture : register(t23);   // VBGI indirect diffuse (R16G16B16A16)

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
  const float4 icb[] = { { 1.000000, 0, 0, 0},
                              { 0, 1.000000, 0, 0},
                              { 0, 0, 1.000000, 0},
                              { 0, 0, 0, 1.000000} };
  float4 r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20;
  uint4 bitmask;
  float4 fDest;

  mrtTexture0.GetDimensions(0, fDest.x, fDest.y, fDest.z);
  r0.xy = fDest.xy;
  r0.xy = v1.xy * r0.xy;
  r0.xy = (int2)r0.xy;
  r0.zw = float2(0,0);
  r0.xyzw = mrtTexture0.Load(r0.xyz).xyzw;
  mrtTexture1.GetDimensions(0, fDest.x, fDest.y, fDest.z);
  r1.xy = fDest.xy;
  r1.xy = v1.xy * r1.xy;
  r1.xy = (int2)r1.xy;
  r1.zw = float2(0,0);
  r1.xyzw = mrtTexture1.Load(r1.xyz).xyzw;
  mrtTexture2.GetDimensions(0, fDest.x, fDest.y, fDest.z);
  r2.xy = fDest.xy;
  r2.xy = v1.xy * r2.xy;
  r2.xy = (int2)r2.xy;
  r2.zw = float2(0,0);
  r2.xy = mrtTexture2.Load(r2.xyz).xy;
  r3.xyzw = colorTexture.SampleLevel(samPoint_s, v1.xy, 0).xyzw;

  // ── GTVBAO: sample AO once and reuse ──
  bool GTVBAO_bound = shader_injection_data.gtvbao_dedicated_bound > 0.5f;
  float GTVBAO_ao = 1.0f;
  if (GTVBAO_bound) {
    uint gtvbao_w, gtvbao_h;
    gtvbaoTexture.GetDimensions(gtvbao_w, gtvbao_h);
    uint2 gtvbao_texel = uint2(saturate(v1.xy) * float2(gtvbao_w, gtvbao_h));
    uint4 GTVBAO_raw = gtvbaoTexture.Load(int3(gtvbao_texel, 0));
    GTVBAO_ao = float(GTVBAO_raw.x) / 255.0f;
  }

  // ── VBGI: sample GI once and reuse ──
  float3 cachedVBGI = float3(0, 0, 0);
  if (shader_injection_data.gtvbao_vbgi_bound > 0.5f) {
    float3 giRaw = vbgiTexture.SampleLevel(samLinear_s, v1.xy, 0).rgb;
    float giLuma = dot(giRaw, float3(0.299f, 0.587f, 0.114f));
    float3 giColor = lerp(giLuma.xxx, giRaw, shader_injection_data.vbgi_saturation);
    giColor *= shader_injection_data.vbgi_intensity;
    if (shader_injection_data.vbgi_max_clamp > 0.0f) {
      giColor = min(giColor, shader_injection_data.vbgi_max_clamp);
    }
    cachedVBGI = giColor;
  }

  r2.zw = (int2)r0.ww & int2(1,4);
  r2.z = cmp((int)r2.z != 0);
  r2.w = cmp((int)r2.w == 0);
  r2.z = r2.w ? r2.z : 0;
  if (r2.z != 0) {
    r2.z = (uint)r2.x;
    r2.w = cmp(0 < r2.z);
    if (r2.w != 0) {
      r4.xy = outlinePrepareTexture.SampleLevel(samPoint_s, v1.xy, 0).xy;
      r5.x = r4.x;
      r5.yw = float2(1,1);
      r2.w = dot(projInv_g._m22_m32, r5.xy);
      r4.x = dot(projInv_g._m23_m33, r5.xy);
      r2.w = r2.w / r4.x;
      r6.xyzw = -fadeBeginDistance_g.xyzw + -r2.wwww;
      r6.xyzw = saturate(fadeRangeInv_g.xyzw * r6.xyzw);
      r7.xy = maxThickness_g;
      r7.z = densityByDepthDiffNear_g;
      r8.x = 1 + -r7.x;
      r8.y = depthThresholdFar_g + -r7.y;
      r8.z = densityByDepthDiffFar_g + -r7.z;
      r4.xzw = r6.yzw * r8.xyz + r7.xyz;
      r6.yz = offsetsAndWeights[0].xy * r4.xx + v1.xy;
      r6.yz = min(uvClamp_g.xy, r6.yz);
      r5.xz = outlinePrepareTexture.SampleLevel(samLinear_s, r6.yz, 0).yx;
      r5.y = dot(projInv_g._m22_m32, r5.zw);
      r5.z = dot(projInv_g._m23_m33, r5.zw);
      r5.y = r5.y / r5.z;
      r5.z = cmp(r5.x < 0);
      r5.y = -r5.y + r2.w;
      r6.yz = cmp(r5.yy >= float2(0,0.300000012));
      r6.yz = r6.yz ? float2(1,1) : 0;
      r5.w = cmp(r5.y >= r4.z);
      r7.x = r5.w ? 1.000000 : 0;
      r5.x = -r5.x + r4.y;
      r5.w = cmp(abs(r5.x) >= 0.0299999993);
      r5.w = r5.w ? 1.000000 : 0;
      r7.y = r6.y * r5.w + r7.x;
      r7.z = abs(r5.y);
      r7.w = r6.y * abs(r5.x) + r7.z;
      r5.xy = offsetsAndWeights[1].xy * r4.xx + v1.xy;
      r5.xy = min(uvClamp_g.xy, r5.xy);
      r5.xy = outlinePrepareTexture.SampleLevel(samLinear_s, r5.xy, 0).xy;
      r8.x = r5.x;
      r8.yw = float2(1,1);
      r5.x = dot(projInv_g._m22_m32, r8.xy);
      r5.w = dot(projInv_g._m23_m33, r8.xy);
      r5.x = r5.x / r5.w;
      r5.w = cmp(r5.y < 0);
      r5.z = (int)r5.z | (int)r5.w;
      r5.x = -r5.x + r2.w;
      r6.yw = cmp(r5.xx >= float2(0,0.300000012));
      r6.yw = r6.yw ? float2(1,1) : 0;
      r5.w = cmp(r5.x >= r4.z);
      r9.x = r5.w ? 1.000000 : 0;
      r5.y = -r5.y + r4.y;
      r5.w = cmp(abs(r5.y) >= 0.0299999993);
      r5.w = r5.w ? 1.000000 : 0;
      r9.y = r6.y * r5.w + r9.x;
      r9.xy = offsetsAndWeights[1].zz * r9.xy;
      r7.xy = r7.xy * offsetsAndWeights[0].zz + r9.xy;
      r9.z = abs(r5.x);
      r9.w = r6.y * abs(r5.y) + r9.z;
      r5.xy = max(r9.zw, r7.zw);
      r5.w = offsetsAndWeights[1].z * r6.w;
      r5.w = r6.z * offsetsAndWeights[0].z + r5.w;
      r6.yz = offsetsAndWeights[2].xy * r4.xx + v1.xy;
      r6.yz = min(uvClamp_g.xy, r6.yz);
      r8.xz = outlinePrepareTexture.SampleLevel(samLinear_s, r6.yz, 0).yx;
      r6.y = dot(projInv_g._m22_m32, r8.zw);
      r6.z = dot(projInv_g._m23_m33, r8.zw);
      r6.y = r6.y / r6.z;
      r6.z = cmp(r8.x < 0);
      r5.z = (int)r5.z | (int)r6.z;
      r6.y = -r6.y + r2.w;
      r6.zw = cmp(r6.yy >= float2(0,0.300000012));
      r6.zw = r6.zw ? float2(1,1) : 0;
      r7.z = cmp(r6.y >= r4.z);
      r9.x = r7.z ? 1.000000 : 0;
      r7.z = -r8.x + r4.y;
      r7.w = cmp(abs(r7.z) >= 0.0299999993);
      r7.w = r7.w ? 1.000000 : 0;
      r9.y = r6.z * r7.w + r9.x;
      r7.xy = r9.xy * offsetsAndWeights[2].zz + r7.xy;
      r8.z = abs(r6.y);
      r8.w = r6.z * abs(r7.z) + r8.z;
      r5.xy = max(r8.zw, r5.xy);
      r5.w = r6.w * offsetsAndWeights[2].z + r5.w;
      r6.yz = offsetsAndWeights[3].xy * r4.xx + v1.xy;
      r6.yz = min(uvClamp_g.xy, r6.yz);
      r6.yz = outlinePrepareTexture.SampleLevel(samLinear_s, r6.yz, 0).xy;
      r8.x = r6.y;
      r8.yw = float2(1,1);
      r6.y = dot(projInv_g._m22_m32, r8.xy);
      r6.w = dot(projInv_g._m23_m33, r8.xy);
      r6.y = r6.y / r6.w;
      r6.w = cmp(r6.z < 0);
      r5.z = (int)r5.z | (int)r6.w;
      r6.y = -r6.y + r2.w;
      r7.zw = cmp(r6.yy >= float2(0,0.300000012));
      r7.zw = r7.zw ? float2(1,1) : 0;
      r6.w = cmp(r6.y >= r4.z);
      r9.x = r6.w ? 1.000000 : 0;
      r6.z = -r6.z + r4.y;
      r6.w = cmp(abs(r6.z) >= 0.0299999993);
      r6.w = r6.w ? 1.000000 : 0;
      r9.y = r7.z * r6.w + r9.x;
      r7.xy = r9.xy * offsetsAndWeights[3].zz + r7.xy;
      r9.z = abs(r6.y);
      r9.w = r7.z * abs(r6.z) + r9.z;
      r5.xy = max(r9.zw, r5.xy);
      r5.w = r7.w * offsetsAndWeights[3].z + r5.w;
      r6.yz = offsetsAndWeights[4].xy * r4.xx + v1.xy;
      r6.yz = min(uvClamp_g.xy, r6.yz);
      r8.xz = outlinePrepareTexture.SampleLevel(samLinear_s, r6.yz, 0).yx;
      r6.y = dot(projInv_g._m22_m32, r8.zw);
      r6.z = dot(projInv_g._m23_m33, r8.zw);
      r6.y = r6.y / r6.z;
      r6.z = cmp(r8.x < 0);
      r5.z = (int)r5.z | (int)r6.z;
      r6.y = -r6.y + r2.w;
      r6.zw = cmp(r6.yy >= float2(0,0.300000012));
      r6.zw = r6.zw ? float2(1,1) : 0;
      r7.z = cmp(r6.y >= r4.z);
      r9.x = r7.z ? 1.000000 : 0;
      r7.z = -r8.x + r4.y;
      r7.w = cmp(abs(r7.z) >= 0.0299999993);
      r7.w = r7.w ? 1.000000 : 0;
      r9.y = r6.z * r7.w + r9.x;
      r7.xy = r9.xy * offsetsAndWeights[4].zz + r7.xy;
      r8.z = abs(r6.y);
      r8.w = r6.z * abs(r7.z) + r8.z;
      r5.xy = max(r8.zw, r5.xy);
      r5.w = r6.w * offsetsAndWeights[4].z + r5.w;
      r6.yz = offsetsAndWeights[5].xy * r4.xx + v1.xy;
      r6.yz = min(uvClamp_g.xy, r6.yz);
      r6.yz = outlinePrepareTexture.SampleLevel(samLinear_s, r6.yz, 0).xy;
      r8.x = r6.y;
      r8.yw = float2(1,1);
      r6.y = dot(projInv_g._m22_m32, r8.xy);
      r6.w = dot(projInv_g._m23_m33, r8.xy);
      r6.y = r6.y / r6.w;
      r6.w = cmp(r6.z < 0);
      r5.z = (int)r5.z | (int)r6.w;
      r6.y = -r6.y + r2.w;
      r7.zw = cmp(r6.yy >= float2(0,0.300000012));
      r7.zw = r7.zw ? float2(1,1) : 0;
      r6.w = cmp(r6.y >= r4.z);
      r9.x = r6.w ? 1.000000 : 0;
      r6.z = -r6.z + r4.y;
      r6.w = cmp(abs(r6.z) >= 0.0299999993);
      r6.w = r6.w ? 1.000000 : 0;
      r9.y = r7.z * r6.w + r9.x;
      r7.xy = r9.xy * offsetsAndWeights[5].zz + r7.xy;
      r9.z = abs(r6.y);
      r9.w = r7.z * abs(r6.z) + r9.z;
      r5.xy = max(r9.zw, r5.xy);
      r5.w = r7.w * offsetsAndWeights[5].z + r5.w;
      r6.yz = offsetsAndWeights[6].xy * r4.xx + v1.xy;
      r6.yz = min(uvClamp_g.xy, r6.yz);
      r8.xz = outlinePrepareTexture.SampleLevel(samLinear_s, r6.yz, 0).yx;
      r6.y = dot(projInv_g._m22_m32, r8.zw);
      r6.z = dot(projInv_g._m23_m33, r8.zw);
      r6.y = r6.y / r6.z;
      r6.z = cmp(r8.x < 0);
      r5.z = (int)r5.z | (int)r6.z;
      r6.y = -r6.y + r2.w;
      r6.zw = cmp(r6.yy >= float2(0,0.300000012));
      r6.zw = r6.zw ? float2(1,1) : 0;
      r7.z = cmp(r6.y >= r4.z);
      r9.x = r7.z ? 1.000000 : 0;
      r7.z = -r8.x + r4.y;
      r7.w = cmp(abs(r7.z) >= 0.0299999993);
      r7.w = r7.w ? 1.000000 : 0;
      r9.y = r6.z * r7.w + r9.x;
      r7.xy = r9.xy * offsetsAndWeights[6].zz + r7.xy;
      r8.z = abs(r6.y);
      r8.w = r6.z * abs(r7.z) + r8.z;
      r5.xy = max(r8.zw, r5.xy);
      r5.w = r6.w * offsetsAndWeights[6].z + r5.w;
      r6.yz = offsetsAndWeights[7].xy * r4.xx + v1.xy;
      r6.yz = min(uvClamp_g.xy, r6.yz);
      r6.yz = outlinePrepareTexture.SampleLevel(samLinear_s, r6.yz, 0).xy;
      r8.x = r6.y;
      r8.y = 1;
      r4.x = dot(projInv_g._m22_m32, r8.xy);
      r6.y = dot(projInv_g._m23_m33, r8.xy);
      r4.x = r4.x / r6.y;
      r6.y = cmp(r6.z < 0);
      r5.z = (int)r5.z | (int)r6.y;
      r2.w = -r4.x + r2.w;
      r6.yw = cmp(r2.ww >= float2(0,0.300000012));
      r6.yw = r6.yw ? float2(1,1) : 0;
      r4.x = cmp(r2.w >= r4.z);
      r8.x = r4.x ? 1.000000 : 0;
      r4.x = -r6.z + r4.y;
      r4.y = cmp(abs(r4.x) >= 0.0299999993);
      r4.y = r4.y ? 1.000000 : 0;
      r8.y = r6.y * r4.y + r8.x;
      r7.xy = r8.xy * offsetsAndWeights[7].zz + r7.xy;
      r8.z = abs(r2.w);
      r8.w = r6.y * abs(r4.x) + r8.z;
      r7.zw = max(r8.zw, r5.xy);
      r2.w = r6.w * offsetsAndWeights[7].z + r5.w;
      r4.xy = r5.zz ? r7.xz : r7.yw;
      r4.y = min(r4.y, r4.w);
      r4.y = r4.y / r4.w;
      r4.y = log2(r4.y);
      r4.y = 1.25 * r4.y;
      r4.y = exp2(r4.y);
      r4.x = r4.x * r4.x;
      r4.x = density_g * r4.x;
      r4.x = min(1, r4.x);
      r4.x = r4.x * r4.y;
      r4.y = 1 + -r6.x;
      r4.x = r4.x * r4.y;
      r2.z = -r2.z * 1.52590219e-05 + 1;
      r4.yzw = r3.xyz * r2.zzz + -r3.xyz;
      r4.yzw = r4.xxx * r4.yzw + r3.xyz;
      r2.z = cmp(0 < r2.w);
      r5.yw = (uint2)r1.zw >> int2(4,4);
      r5.xz = r1.zw;
      r5.xyzw = (int4)r5.xyzw & int4(15,15,15,15);
      r5.xyzw = (uint4)r5.xyzw;
      r5.xyzw = float4(0.0666666701,0.0666666701,0.0666666701,0.0666666701) * r5.xyzw;
      r5.xyz = r5.xyz * r5.www;
      r2.w = r5.w * r4.x;
      r5.xyz = r5.xyz * float3(10,10,10) + -r4.yzw;
      r5.xyz = r2.www * r5.xyz + r4.yzw;
      r4.xyz = r2.zzz ? r5.xyz : r4.yzw;
    } else {
      r4.xyz = r3.xyz;
    }
    r4.w = r3.w;
    o0.xyzw = r4.xyzw;
    o1.xyzw = r1.xyzw;
    o2.xy = r2.xy;
    return;
  } else {
    r2.z = (int)r0.w & 8;
    if (r2.z == 0) {
      o0.xyzw = r3.xyzw;
      o1.xyzw = r1.xyzw;
      o2.xy = r2.xy;
      return;
    }
  }
  r4.z = depthTexture.SampleLevel(samPoint_s, v1.xy, 0).x;
  r0.xyz = (uint3)r0.xyz;
  r0.xyz = r0.xyz * float3(0.00392156886,0.00392156886,0.00392156886) + float3(-0.5,-0.5,-0.5);
  r5.xyz = r0.xyz + r0.xyz;
  r1.xyzw = (uint4)r1.xyzw;
  r6.xyzw = float4(0.00392156886,0.00392156886,0.00392156886,0.00392156886) * r1.wxyz;
  r1.yz = (uint2)r2.yy >> int2(5,10);
  r1.x = r2.y;
  r1.xyz = (int3)r1.xyz & int3(31,31,31);
  r1.xyz = (uint3)r1.xyz;
  r1.xyz = float3(0.0322580636,0.0322580636,0.0322580636) * r1.xyz;
  r2.x = min(0x00004e1f, (uint)r2.x);
  r7.x = deferredParams_g[r2.x].shadowColor.x;
  r7.y = deferredParams_g[r2.x].shadowColor.y;
  r7.z = deferredParams_g[r2.x].shadowColor.z;
  r7.w = deferredParams_g[r2.x].emissive;
  r8.x = deferredParams_g[r2.x].specularColor.x;
  r8.y = deferredParams_g[r2.x].specularColor.y;
  r8.z = deferredParams_g[r2.x].specularColor.z;
  r8.w = deferredParams_g[r2.x].rimLightPower;
  r9.x = deferredParams_g[r2.x].rimLightColor.x;
  r9.y = deferredParams_g[r2.x].rimLightColor.y;
  r9.z = deferredParams_g[r2.x].rimLightColor.z;
  r9.w = deferredParams_g[r2.x].rimIntensity;
  r10.x = deferredParams_g[r2.x].fresnels.x;
  r10.y = deferredParams_g[r2.x].fresnels.y;
  r10.z = deferredParams_g[r2.x].fresnels.z;
  r11.x = deferredParams_g[r2.x].specularGlossinesses.x;
  r11.y = deferredParams_g[r2.x].specularGlossinesses.y;
  r11.z = deferredParams_g[r2.x].specularGlossinesses.z;
  r11.w = deferredParams_g[r2.x].dynamicLightIntensity;
  r12.x = deferredParams_g[r2.x].materialFogIntensity;
  r12.y = deferredParams_g[r2.x].metalness;
  r12.z = deferredParams_g[r2.x].roughness;
  r12.w = deferredParams_g[r2.x].cryRefractionIndex;
  r13.x = deferredParams_g[r2.x].cryFresnel;
  r13.y = deferredParams_g[r2.x].cryBrightness;
  r13.z = deferredParams_g[r2.x].cryBrightnessPower;
  r13.w = deferredParams_g[r2.x].glowIntensity;
  r2.y = deferredParams_g[r2.x].glowLumThreshold;
  r2.z = deferredParams_g[r2.x].glowShadowFadeRatio;
  r2.w = deferredParams_g[r2.x].ssaoIntensity;
  r2.x = deferredParams_g[r2.x].flag;
  r10.w = r11.x;
  r14.xz = r10.yz;
  r14.yw = r11.yz;
  r10.yz = r14.xy + -r10.xw;
  r10.xy = r1.xx * r10.yz + r10.xw;
  r10.zw = r14.zw + -r10.xy;
  r1.xy = r1.yy * r10.zw + r10.xy;
  r10.xyz = lightProbe_g[1].xyz * r5.xxx + lightProbe_g[0].xyz;
  r10.xyz = lightProbe_g[2].xyz * r5.yyy + r10.xyz;
  r10.xyz = lightProbe_g[3].xyz * r5.zzz + r10.xyz;
  r11.xyz = lightProbe_g[4].xyz * r5.zzz;
  r10.xyz = r11.xyz * r5.xxx + r10.xyz;
  r11.xyz = lightProbe_g[5].xyz * r5.yyy;
  r10.xyz = r11.xyz * r5.zzz + r10.xyz;
  r11.xyz = lightProbe_g[6].xyz * r5.yyy;
  r10.xyz = r11.xyz * r5.xxx + r10.xyz;
  r3.w = r5.z * r0.z;
  r3.w = r3.w * 6 + -1;
  r10.xyz = lightProbe_g[7].xyz * r3.www + r10.xyz;
  r3.w = r5.y * r5.y;
  r3.w = r5.x * r5.x + -r3.w;
  r10.xyz = lightProbe_g[8].xyz * r3.www + r10.xyz;
  r4.xy = v1.zw * float2(2,-2) + float2(-1,1);
  r4.w = 1;
  r14.x = dot(r4.xyzw, viewProjInv_g._m00_m10_m20_m30);
  r14.y = dot(r4.xyzw, viewProjInv_g._m01_m11_m21_m31);
  r14.z = dot(r4.xyzw, viewProjInv_g._m02_m12_m22_m32);
  r14.w = dot(r4.xyzw, viewProjInv_g._m03_m13_m23_m33);
  r4.xyzw = r14.xyzw / r14.wwww;
  r3.w = dot(view_g._m02_m12_m22_m32, r4.xyzw);
  r11.x = viewInv_g._m30;
  r11.y = viewInv_g._m31;
  r11.z = viewInv_g._m32;
  r14.xyz = r11.xyz + -r4.xyz;
  r5.w = dot(r14.xyz, r14.xyz);
  r5.w = rsqrt(r5.w);
  r15.xyz = r14.xyz * r5.www;
  r7.xyz = sceneShadowColor_g.xyz + r7.xyz;
  r7.xyz = min(float3(1,1,1), r7.xyz);
  r7.w = r7.w * r1.z;
  r10.w = dot(r5.xyz, r15.xyz);
  r16.xyzw = (int4)r2.xxxx & int4(1,2,4,16);
  if (r16.x != 0) {
    r17.xyz = float3(0.340000004,0.340000004,0.340000004) * r0.xyz;
    r17.w = 0;
    r17.xyzw = r17.xyzw + r4.xyzw;
    r0.xyz = r17.xyz + -r11.xyz;
    r0.x = dot(r0.xyz, r0.xyz);
    r0.x = sqrt(r0.x);
    r0.y = shadowSplitDistance_g.y + -5;
    r0.y = cmp(r0.y < r0.x);
    if (r0.y != 0) {
      r11.x = dot(r17.xyzw, shadowMtx_g[2]._m00_m10_m20_m30);
      r11.y = dot(r17.xyzw, shadowMtx_g[2]._m01_m11_m21_m31);
      r11.z = dot(r17.xyzw, shadowMtx_g[2]._m02_m12_m22_m32);
      r0.y = dot(r17.xyzw, shadowMtx_g[2]._m03_m13_m23_m33);
      r11.xyz = r11.xyz / r0.yyy;
      r0.yz = cmp(r11.xy < float2(0,0));
      r18.xy = cmp(float2(1,1) < r11.xy);
      r0.y = (int)r0.y | (int)r18.x;
      r0.y = (int)r0.z | (int)r0.y;
      r0.y = (int)r18.y | (int)r0.y;
      if (r0.y != 0) {
        r0.y = 1;
      } else {
        r18.xy = saturate(r11.xy);
        r18.z = 2;
        r0.z = shadowMaps.SampleCmpLevelZero(SmplShadow_s, r18.xyz, r11.z).x;
        r18.xz = invShadowSize_g.xx * float2(3,-3);
        r18.yw = float2(0,0);
        r18.xyzw = r18.zwxy + r11.xyxy;
        r19.xy = saturate(r18.zw);
        r19.z = 2;
        r14.w = shadowMaps.SampleCmpLevelZero(SmplShadow_s, r19.xyz, r11.z).x;
        r0.z = r14.w + r0.z;
        r18.xy = saturate(r18.xy);
        r18.z = 2;
        r14.w = shadowMaps.SampleCmpLevelZero(SmplShadow_s, r18.xyz, r11.z).x;
        r0.z = r14.w + r0.z;
        r18.xz = float2(0,0);
        r18.yw = invShadowSize_g.yy * float2(3,-3);
        r18.xyzw = r18.zwxy + r11.xyxy;
        r19.xy = saturate(r18.zw);
        r19.z = 2;
        r11.x = shadowMaps.SampleCmpLevelZero(SmplShadow_s, r19.xyz, r11.z).x;
        r0.z = r11.x + r0.z;
        r18.xy = saturate(r18.xy);
        r18.z = 2;
        r11.x = shadowMaps.SampleCmpLevelZero(SmplShadow_s, r18.xyz, r11.z).x;
        r0.z = r11.x + r0.z;
        r0.y = 0.200000003 * r0.z;
      }
      r0.z = cmp(r0.x < shadowSplitDistance_g.y);
      if (r0.z != 0) {
        r11.x = dot(r17.xyzw, shadowMtx_g[1]._m00_m10_m20_m30);
        r11.y = dot(r17.xyzw, shadowMtx_g[1]._m01_m11_m21_m31);
        r11.z = dot(r17.xyzw, shadowMtx_g[1]._m02_m12_m22_m32);
        r0.z = dot(r17.xyzw, shadowMtx_g[1]._m03_m13_m23_m33);
        r11.xyz = r11.xyz / r0.zzz;
        r18.xy = saturate(r11.xy);
        r18.z = 1;
        r0.z = shadowMaps.SampleCmpLevelZero(SmplShadow_s, r18.xyz, r11.z).x;
        r18.xz = invShadowSize_g.xx * float2(2,-2);
        r18.yw = float2(0,0);
        r18.xyzw = r18.zwxy + r11.xyxy;
        r19.xy = saturate(r18.zw);
        r19.z = 1;
        r14.w = shadowMaps.SampleCmpLevelZero(SmplShadow_s, r19.xyz, r11.z).x;
        r0.z = r14.w + r0.z;
        r18.xy = saturate(r18.xy);
        r18.z = 1;
        r14.w = shadowMaps.SampleCmpLevelZero(SmplShadow_s, r18.xyz, r11.z).x;
        r0.z = r14.w + r0.z;
        r18.xz = float2(0,0);
        r18.yw = invShadowSize_g.yy * float2(2,-2);
        r18.xyzw = r18.zwxy + r11.xyxy;
        r19.xy = saturate(r18.zw);
        r19.z = 1;
        r11.x = shadowMaps.SampleCmpLevelZero(SmplShadow_s, r19.xyz, r11.z).x;
        r0.z = r11.x + r0.z;
        r18.xy = saturate(r18.xy);
        r18.z = 1;
        r11.x = shadowMaps.SampleCmpLevelZero(SmplShadow_s, r18.xyz, r11.z).x;
        r0.z = r11.x + r0.z;
        r11.x = shadowSplitDistance_g.y + -r0.x;
        r11.x = 0.200000003 * r11.x;
        r0.z = r0.z * 0.200000003 + -r0.y;
        r0.y = r11.x * r0.z + r0.y;
      }
      r0.z = -shadowFadeNear_g + r0.x;
      r0.z = saturate(shadowFadeRangeInv_g * r0.z);
      r11.x = 1 + -r0.y;
      r0.y = r0.z * r11.x + r0.y;
    } else {
      r0.z = cmp(r0.x < shadowSplitDistance_g.x);
      r11.xyz = r0.zzz ? float3(0,0,0) : float3(1,4,1);
      r18.x = dot(r17.xyzw, shadowMtx_g[r11.y/4]._m00_m10_m20_m30);
      r18.y = dot(r17.xyzw, shadowMtx_g[r11.y/4]._m01_m11_m21_m31);
      r18.z = dot(r17.xyzw, shadowMtx_g[r11.y/4]._m02_m12_m22_m32);
      r14.w = dot(r17.xyzw, shadowMtx_g[r11.y/4]._m03_m13_m23_m33);
      r18.xyz = r18.xyz / r14.www;
      r14.w = dot(float2(1.25,2), icb[r11.x+0].xy);
      r19.xy = invShadowSize_g.xy * r14.ww;
      r11.xy = saturate(r18.xy);
      r14.w = shadowMaps.SampleCmpLevelZero(SmplShadow_s, r11.xyz, r18.z).x;
      r19.z = 0;
      r20.xyzw = r19.xzzy + r18.xyxy;
      r11.xy = saturate(r20.xy);
      r15.w = shadowMaps.SampleCmpLevelZero(SmplShadow_s, r11.xyz, r18.z).x;
      r14.w = r15.w + r14.w;
      r19.w = -r19.x;
      r11.xy = saturate(r19.wz + r18.xy);
      r15.w = shadowMaps.SampleCmpLevelZero(SmplShadow_s, r11.xyz, r18.z).x;
      r14.w = r15.w + r14.w;
      r11.xy = saturate(r20.zw);
      r15.w = shadowMaps.SampleCmpLevelZero(SmplShadow_s, r11.xyz, r18.z).x;
      r14.w = r15.w + r14.w;
      r11.xy = saturate(r19.zy * float2(1,-1) + r18.xy);
      r11.x = shadowMaps.SampleCmpLevelZero(SmplShadow_s, r11.xyz, r18.z).x;
      r11.x = r14.w + r11.x;
      r0.y = 0.200000003 * r11.x;
      r11.y = shadowSplitDistance_g.x + -5;
      r11.y = cmp(r11.y < r0.x);
      r0.z = r0.z ? r11.y : 0;
      if (r0.z != 0) {
        r18.x = dot(r17.xyzw, shadowMtx_g[1]._m00_m10_m20_m30);
        r18.y = dot(r17.xyzw, shadowMtx_g[1]._m01_m11_m21_m31);
        r18.z = dot(r17.xyzw, shadowMtx_g[1]._m02_m12_m22_m32);
        r0.z = dot(r17.xyzw, shadowMtx_g[1]._m03_m13_m23_m33);
        r17.xyz = r18.xyz / r0.zzz;
        r18.xy = saturate(r17.xy);
        r18.z = 1;
        r0.z = shadowMaps.SampleCmpLevelZero(SmplShadow_s, r18.xyz, r17.z).x;
        r18.xz = invShadowSize_g.xx * float2(2,-2);
        r18.yw = float2(0,0);
        r18.xyzw = r18.zwxy + r17.xyxy;
        r19.xy = saturate(r18.zw);
        r19.z = 1;
        r11.y = shadowMaps.SampleCmpLevelZero(SmplShadow_s, r19.xyz, r17.z).x;
        r0.z = r11.y + r0.z;
        r18.xy = saturate(r18.xy);
        r18.z = 1;
        r11.y = shadowMaps.SampleCmpLevelZero(SmplShadow_s, r18.xyz, r17.z).x;
        r0.z = r11.y + r0.z;
        r18.xz = float2(0,0);
        r18.yw = invShadowSize_g.yy * float2(2,-2);
        r18.xyzw = r18.zwxy + r17.xyxy;
        r19.xy = saturate(r18.zw);
        r19.z = 1;
        r11.y = shadowMaps.SampleCmpLevelZero(SmplShadow_s, r19.xyz, r17.z).x;
        r0.z = r11.y + r0.z;
        r18.xy = saturate(r18.xy);
        r18.z = 1;
        r11.y = shadowMaps.SampleCmpLevelZero(SmplShadow_s, r18.xyz, r17.z).x;
        r0.z = r11.y + r0.z;
        r0.x = shadowSplitDistance_g.x + -r0.x;
        r0.xz = float2(0.200000003,0.200000003) * r0.xz;
        r11.x = r11.x * 0.200000003 + -r0.z;
        r0.y = r0.x * r11.x + r0.z;
      }
    }
  } else {
    r0.y = 1;
  }
  r11.xyz = r14.xyz * r5.www + -lightDirection_g.xyz;
  r0.x = dot(r11.xyz, r11.xyz);
  r0.x = rsqrt(r0.x);
  r11.xyz = r11.xyz * r0.xxx;
  r0.x = lightSpecularGlossiness_g * r1.y;
  r0.z = saturate(dot(r11.xyz, r5.xyz));
  r0.x = max(0.00100000005, r0.x);
  r0.z = log2(r0.z);
  r0.x = r0.x * r0.z;
  r0.x = exp2(r0.x);
  r0.x = r0.x * r0.y;
  r0.x = lightSpecularIntensity_g * r0.x;
  r0.x = r16.y ? r0.x : 0;
  r8.xyz = r0.xxx * r8.xyz;
  r8.xyz = lightColor_g.xyz * r8.xyz;
  if (r16.z != 0) {
    r0.x = r12.y * r6.y;
    r0.z = cmp(0 < r1.x);
    r6.y = 1 + -abs(r10.w);
    r6.y = max(0.00999999978, r6.y);
    r6.y = log2(r6.y);
    r1.x = r6.y * r1.x;
    r1.x = exp2(r1.x);
    r0.z = r0.z ? r1.x : 1;
    r1.x = (int)r2.x & 32;
    if (r1.x != 0) {
      r11.y = resolutionScaling_g.y + -v1.y;
      r11.x = v1.x;
      r11.xyz = texMirror_g.SampleLevel(SmplLinearClamp_s, r11.xy, 0).xyz;
      r1.x = r0.x * r0.z;
      r11.xyz = r3.xyz * r11.xyz + -r3.xyz;
      r11.xyz = r1.xxx * r11.xyz + r3.xyz;
    } else {
      r1.x = r12.z * r6.z;
      r6.y = r10.w + r10.w;
      r17.xyz = r5.xyz * -r6.yyy + r15.xyz;
      uint envMapLevels;
      texEnvMap_g.GetDimensions(0, envMapLevels, envMapLevels, envMapLevels);
      r6.y = envMapLevels;
      r17.xyz = float3(1,-1,-1) * r17.xyz;
      r6.y = (int)r6.y + -1;
      r6.y = (uint)r6.y;
      r6.y = r6.y * r1.x;
      r17.xyz = texEnvMap_g.SampleLevel(SmplCube_s, r17.xyz, r6.y).xyz;
      r6.y = r0.x * r0.z;
      r18.xyz = r3.xyz * r17.xyz + -r3.xyz;
      r11.xyz = r6.yyy * r18.xyz + r3.xyz;
      r6.y = dot(r17.xyz, float3(0.298999995,0.587000012,0.114));
      r1.x = r1.x * -9 + 10;
      r6.y = log2(r6.y);
      r1.x = r6.y * r1.x;
      r1.x = exp2(r1.x);
      r6.y = 1 + -r1.x;
      r0.x = r0.x * r6.y + r1.x;
      r17.xyz = r17.xyz * r0.xxx;
      r17.xyz = r17.xyz * r0.zzz;
      r0.x = -r6.z * r12.z + 1;
      r8.xyz = r17.xyz * r0.xxx + r8.xyz;
    }
  } else {
    r0.x = (int)r2.x & 8;
    if (r0.x != 0) {
      r0.x = r10.w + r10.w;
      r17.xyz = r5.xyz * -r0.xxx + r15.xyz;
      r0.x = 1 / r12.w;
      r0.z = dot(-r15.xyz, r5.xyz);
      r1.x = r0.x * r0.x;
      r2.x = -r0.z * r0.z + 1;
      r1.x = -r1.x * r2.x + 1;
      r2.x = sqrt(r1.x);
      r0.z = r0.x * r0.z + r2.x;
      r1.x = cmp(r1.x >= 0);
      r18.xyz = r0.zzz * r5.xyz;
      r15.xyz = r0.xxx * -r15.xyz + -r18.xyz;
      r15.xyz = r1.xxx ? r15.xyz : 0;
      r0.x = r12.z * r6.z;
      uint envMapLevels2;
      texEnvMap_g.GetDimensions(0, envMapLevels2, envMapLevels2, envMapLevels2);
      r0.z = envMapLevels2;
      r17.xyz = float3(1,-1,-1) * r17.xyz;
      r0.z = (int)r0.z + -1;
      r0.z = (uint)r0.z;
      r0.x = r0.x * r0.z;
      r17.xyz = texEnvMap_g.SampleLevel(SmplCube_s, r17.xyz, r0.x).xyz;
      r15.xyz = texEnvMap_g.SampleLevel(SmplCube_s, r15.xyz, r0.x).xyz;
      r0.x = cmp(0 < r13.x);
      r0.z = 1 + -abs(r10.w);
      r0.z = max(0, r0.z);
      r0.z = log2(r0.z);
      r0.z = r13.x * r0.z;
      r0.z = exp2(r0.z);
      r0.x = r0.x ? r0.z : 1;
      r18.xyz = r3.xyz * r17.xyz + -r3.xyz;
      r18.xyz = r0.xxx * r18.xyz + r3.xyz;
      r0.z = -r1.w * 0.00392156886 + 1;
      r6.x = r0.x * r0.z + r6.x;
      r0.z = abs(r10.w) * r13.y;
      r0.z = log2(r0.z);
      r0.z = r13.z * r0.z;
      r0.z = exp2(r0.z);
      r13.xyz = r6.xxx * r17.xyz;
      r13.xyz = r13.xyz * r0.xxx;
      r0.x = -r6.z * r12.z + 1;
      r8.xyz = r13.xyz * r0.xxx + r8.xyz;
      r12.yzw = r0.zzz * r15.xyz;
      r11.xyz = r18.xyz * r12.yzw;
    } else {
      r11.xyz = r3.xyz;
    }
  }
  r0.x = r6.x * r0.y;
  r6.xyz = float3(1,1,1) + -r7.xyz;
  r6.xyz = r0.xxx * r6.xyz + r7.xyz;
  r6.xyz = r6.xyz * lightColor_g.xyz + r10.xyz;
  r0.x = min(1, r7.w);
  r7.xyz = float3(1,1,1) + -r6.xyz;
  r6.xyz = r0.xxx * r7.xyz + r6.xyz;
  r0.x = 1 + -abs(r10.w);
  r0.x = max(0, r0.x);
  r0.x = r0.x * r9.w;
  r0.x = log2(r0.x);
  r0.x = r8.w * r0.x;
  r0.x = exp2(r0.x);
  r0.x = min(1, r0.x);
  r7.xyz = r9.xyz * r0.xxx + r8.xyz;
  if (r16.y != 0) {
    r0.xz = lightTileWidthInv_g * v0.xy;
    r1.x = lightTileDepthInv_g * -r3.w;
    r1.x = min(7, r1.x);
    r1.x = max(0, r1.x);
    r0.xz = (uint2)r0.xz;
    r1.x = (uint)r1.x;
    r0.z = (uint)r0.z << 5;
    r0.z = mad((int)r1.x, 576, (int)r0.z);
    r0.x = (int)r0.x + (int)r0.z;
    r0.x = min(4607, (uint)r0.x);
    r0.z = lightIndices_g[r0.x].pointLightCount;
    r0.z = min(63, (uint)r0.z);
    r8.xyz = float3(0,0,0);
    r9.xyz = float3(0,0,0);
    r1.x = 0;
    while (true) {
      r1.w = cmp((uint)r1.x >= (uint)r0.z);
      if (r1.w != 0) break;
      r1.w = (uint)r1.x << 2;
      r1.w = lightIndices_g[r0.x].pointLightIndices[r1.x];
      r10.x = dynamicLights_g[r1.w].pos.x;
      r10.y = dynamicLights_g[r1.w].pos.y;
      r10.z = dynamicLights_g[r1.w].pos.z;
      r10.xyz = r10.xyz + -r4.xyz;
      r2.x = dot(r10.xyz, r10.xyz);
      r7.w = sqrt(r2.x);
      r8.w = dynamicLights_g[r1.w].radiusInv;
      r7.w = r8.w * r7.w;
      r8.w = dynamicLights_g[r1.w].attenuation;
      r7.w = log2(abs(r7.w));
      r7.w = r8.w * r7.w;
      r7.w = exp2(r7.w);
      r7.w = 1 + -r7.w;
      r7.w = max(0, r7.w);
      r8.w = cmp(0 < r7.w);
      if (r8.w != 0) {
        r2.x = rsqrt(r2.x);
        r10.xyz = r10.xyz * r2.xxx;
        r2.x = dynamicLights_g[r1.w].translucency;
        r8.w = dot(r10.xyz, r5.xyz);
        r2.x = max(r8.w, r2.x);
        r2.x = r7.w * r2.x;
        r12.y = dynamicLights_g[r1.w].color.x;
        r12.z = dynamicLights_g[r1.w].color.y;
        r12.w = dynamicLights_g[r1.w].color.z;
        r9.xyz = r12.yzw * r2.xxx + r9.xyz;
        r10.xyz = r14.xyz * r5.www + r10.xyz;
        r7.w = dot(r10.xyz, r10.xyz);
        r7.w = rsqrt(r7.w);
        r10.xyz = r10.xyz * r7.www;
        r13.x = dynamicLights_g[r1.w].specularIntensity;
        r13.y = dynamicLights_g[r1.w].specularGlossiness;
        r1.w = r13.y * r1.y;
        r7.w = saturate(dot(r10.xyz, r5.xyz));
        r1.w = max(0.00100000005, r1.w);
        r7.w = log2(r7.w);
        r1.w = r7.w * r1.w;
        r1.w = exp2(r1.w);
        r10.xyz = r12.yzw * r1.www;
        r10.xyz = r10.xyz * r2.xxx;
        r8.xyz = r10.xyz * r13.xxx + r8.xyz;
      }
      r1.x = (int)r1.x + 1;
    }
    r9.xyz = r9.xyz * r11.www + r6.xyz;
    r0.z = lightIndices_g[r0.x].spotLightCount;
    r0.z = min(63, (uint)r0.z);
    r10.xyz = r8.xyz;
    r12.yzw = float3(0,0,0);
    r1.x = 0;
    while (true) {
      r1.w = cmp((uint)r1.x >= (uint)r0.z);
      if (r1.w != 0) break;
      r1.w = (uint)r1.x << 2;
      r1.w = (int)r1.w + 256;
      r1.w = lightIndices_g[r0.x].spotLightIndices[r1.x];
      r13.x = dynamicLights_g[r1.w].pos.x;
      r13.y = dynamicLights_g[r1.w].pos.y;
      r13.z = dynamicLights_g[r1.w].pos.z;
      r13.xyz = r13.xyz + -r4.xyz;
      r2.x = dot(r13.xyz, r13.xyz);
      r7.w = rsqrt(r2.x);
      r13.xyz = r13.xyz * r7.www;
      r15.x = dynamicLights_g[r1.w].vec.x;
      r15.y = dynamicLights_g[r1.w].vec.y;
      r15.z = dynamicLights_g[r1.w].vec.z;
      r15.w = dynamicLights_g[r1.w].spotAngleInv;
      r7.w = dot(r13.xyz, r15.xyz);
      r7.w = max(0, r7.w);
      r7.w = 1 + -r7.w;
      r7.w = r7.w * r15.w;
      r8.w = dynamicLights_g[r1.w].attenuationAngle;
      r7.w = log2(r7.w);
      r7.w = r8.w * r7.w;
      r7.w = exp2(r7.w);
      r7.w = 1 + -r7.w;
      r7.w = max(0, r7.w);
      r8.w = cmp(0 < r7.w);
      if (r8.w != 0) {
        r2.x = sqrt(r2.x);
        r8.w = dynamicLights_g[r1.w].radiusInv;
        r2.x = r8.w * r2.x;
        r8.w = dynamicLights_g[r1.w].attenuation;
        r2.x = log2(abs(r2.x));
        r2.x = r8.w * r2.x;
        r2.x = exp2(r2.x);
        r2.x = 1 + -r2.x;
        r2.x = max(0, r2.x);
        r2.x = r7.w * r2.x;
        r7.w = cmp(0 < r2.x);
        if (r7.w != 0) {
          r15.x = dynamicLights_g[r1.w].translucency;
          r15.y = dynamicLights_g[r1.w].shadowmapIndex;
          r7.w = cmp((int)r15.y != -1);
          if (r7.w != 0) {
            r17.xyzw = spotShadowMatrices_g[r15.y]._m00_m10_m20_m30;
            r18.xyzw = spotShadowMatrices_g[r15.y]._m01_m11_m21_m31;
            r19.xyzw = spotShadowMatrices_g[r15.y]._m02_m12_m22_m32;
            r20.xyzw = spotShadowMatrices_g[r15.y]._m03_m13_m23_m33;
            r16.x = dot(r4.xyzw, r17.xyzw);
            r16.y = dot(r4.xyzw, r18.xyzw);
            r16.z = dot(r4.xyzw, r19.xyzw);
            r7.w = dot(r4.xyzw, r20.xyzw);
            r17.xyz = r16.xyz / r7.www;
            r17.w = (int)r15.y;
            r7.w = spotShadowMaps.SampleCmpLevelZero(SmplShadow_s, r17.xyw, r17.z).x;
            r2.x = r7.w * r2.x;
          }
          r7.w = dot(r13.xyz, r5.xyz);
          r7.w = max(r15.x, r7.w);
          r2.x = r7.w * r2.x;
          r15.x = dynamicLights_g[r1.w].color.x;
          r15.y = dynamicLights_g[r1.w].color.y;
          r15.z = dynamicLights_g[r1.w].color.z;
          r12.yzw = r15.xyz * r2.xxx + r12.yzw;
          r13.xyz = r14.xyz * r5.www + r13.xyz;
          r7.w = dot(r13.xyz, r13.xyz);
          r7.w = rsqrt(r7.w);
          r13.xyz = r13.xyz * r7.www;
          r16.x = dynamicLights_g[r1.w].specularIntensity;
          r16.y = dynamicLights_g[r1.w].specularGlossiness;
          r1.w = r16.y * r1.y;
          r7.w = saturate(dot(r13.xyz, r5.xyz));
          r1.w = max(0.00100000005, r1.w);
          r7.w = log2(r7.w);
          r1.w = r7.w * r1.w;
          r1.w = exp2(r1.w);
          r13.xyz = r15.xyz * r1.www;
          r13.xyz = r13.xyz * r2.xxx;
          r10.xyz = r13.xyz * r16.xxx + r10.xyz;
        }
      }
      r1.x = (int)r1.x + 1;
    }
    r1.xyw = r12.yzw * r11.www + r9.xyz;
    r7.xyz = r10.xyz * r11.www + r7.xyz;
  } else {
    r0.xz = lightTileWidthInv_g * v0.xy;
    r2.x = lightTileDepthInv_g * -r3.w;
    r2.x = min(7, r2.x);
    r2.x = max(0, r2.x);
    r0.xz = (uint2)r0.xz;
    r2.x = (uint)r2.x;
    r0.z = (uint)r0.z << 5;
    r0.z = mad((int)r2.x, 576, (int)r0.z);
    r0.x = (int)r0.x + (int)r0.z;
    r0.x = min(4607, (uint)r0.x);
    r0.z = lightIndices_g[r0.x].pointLightCount;
    r0.z = min(63, (uint)r0.z);
    r8.xyzw = float4(0,0,0,0);
    while (true) {
      r2.x = cmp((uint)r8.w >= (uint)r0.z);
      if (r2.x != 0) break;
      r2.x = (uint)r8.w << 2;
      r2.x = lightIndices_g[r0.x].pointLightIndices[r8.w];
      r9.x = dynamicLights_g[r2.x].pos.x;
      r9.y = dynamicLights_g[r2.x].pos.y;
      r9.z = dynamicLights_g[r2.x].pos.z;
      r9.xyz = r9.xyz + -r4.xyz;
      r5.w = dot(r9.xyz, r9.xyz);
      r7.w = sqrt(r5.w);
      r9.w = dynamicLights_g[r2.x].radiusInv;
      r7.w = r9.w * r7.w;
      r9.w = dynamicLights_g[r2.x].attenuation;
      r7.w = log2(abs(r7.w));
      r7.w = r9.w * r7.w;
      r7.w = exp2(r7.w);
      r7.w = 1 + -r7.w;
      r7.w = max(0, r7.w);
      r9.w = cmp(0 < r7.w);
      if (r9.w != 0) {
        r9.w = dynamicLights_g[r2.x].translucency;
        r5.w = rsqrt(r5.w);
        r9.xyz = r9.xyz * r5.www;
        r5.w = dot(r9.xyz, r5.xyz);
        r5.w = max(r9.w, r5.w);
        r9.x = dynamicLights_g[r2.x].color.x;
        r9.y = dynamicLights_g[r2.x].color.y;
        r9.z = dynamicLights_g[r2.x].color.z;
        r9.xyz = r9.xyz * r7.www;
        r8.xyz = r9.xyz * r5.www + r8.xyz;
      }
      r8.w = (int)r8.w + 1;
    }
    r6.xyz = r8.xyz * r11.www + r6.xyz;
    r0.z = lightIndices_g[r0.x].spotLightCount;
    r0.z = min(63, (uint)r0.z);
    r8.xyzw = float4(0,0,0,0);
    while (true) {
      r2.x = cmp((uint)r8.w >= (uint)r0.z);
      if (r2.x != 0) break;
      r2.x = (uint)r8.w << 2;
      r2.x = (int)r2.x + 256;
      r2.x = lightIndices_g[r0.x].spotLightIndices[r8.w];
      r9.x = dynamicLights_g[r2.x].pos.x;
      r9.y = dynamicLights_g[r2.x].pos.y;
      r9.z = dynamicLights_g[r2.x].pos.z;
      r9.xyz = r9.xyz + -r4.xyz;
      r5.w = dot(r9.xyz, r9.xyz);
      r7.w = rsqrt(r5.w);
      r9.xyz = r9.xyz * r7.www;
      r10.x = dynamicLights_g[r2.x].vec.x;
      r10.y = dynamicLights_g[r2.x].vec.y;
      r10.z = dynamicLights_g[r2.x].vec.z;
      r10.w = dynamicLights_g[r2.x].spotAngleInv;
      r7.w = dot(r9.xyz, r10.xyz);
      r7.w = max(0, r7.w);
      r7.w = 1 + -r7.w;
      r7.w = r7.w * r10.w;
      r9.w = dynamicLights_g[r2.x].attenuationAngle;
      r7.w = log2(r7.w);
      r7.w = r9.w * r7.w;
      r7.w = exp2(r7.w);
      r7.w = 1 + -r7.w;
      r7.w = max(0, r7.w);
      r9.w = cmp(0 < r7.w);
      if (r9.w != 0) {
        r5.w = sqrt(r5.w);
        r9.w = dynamicLights_g[r2.x].radiusInv;
        r5.w = r9.w * r5.w;
        r9.w = dynamicLights_g[r2.x].attenuation;
        r5.w = log2(abs(r5.w));
        r5.w = r9.w * r5.w;
        r5.w = exp2(r5.w);
        r5.w = 1 + -r5.w;
        r5.w = max(0, r5.w);
        r5.w = r7.w * r5.w;
        r7.w = cmp(0 < r5.w);
        if (r7.w != 0) {
          r10.x = dynamicLights_g[r2.x].translucency;
          r10.y = dynamicLights_g[r2.x].shadowmapIndex;
          r7.w = cmp((int)r10.y != -1);
          if (r7.w != 0) {
            r14.xyzw = spotShadowMatrices_g[r10.y]._m00_m10_m20_m30;
            r15.xyzw = spotShadowMatrices_g[r10.y]._m01_m11_m21_m31;
            r17.xyzw = spotShadowMatrices_g[r10.y]._m02_m12_m22_m32;
            r18.xyzw = spotShadowMatrices_g[r10.y]._m03_m13_m23_m33;
            r13.x = dot(r4.xyzw, r14.xyzw);
            r13.y = dot(r4.xyzw, r15.xyzw);
            r13.z = dot(r4.xyzw, r17.xyzw);
            r7.w = dot(r4.xyzw, r18.xyzw);
            r14.xyz = r13.xyz / r7.www;
            r14.w = (int)r10.y;
            r7.w = spotShadowMaps.SampleCmpLevelZero(SmplShadow_s, r14.xyw, r14.z).x;
            r5.w = r7.w * r5.w;
          }
          r7.w = dot(r9.xyz, r5.xyz);
          r7.w = max(r10.x, r7.w);
          r9.x = dynamicLights_g[r2.x].color.x;
          r9.y = dynamicLights_g[r2.x].color.y;
          r9.z = dynamicLights_g[r2.x].color.z;
          r9.xyz = r9.xyz * r5.www;
          r8.xyz = r9.xyz * r7.www + r8.xyz;
        }
      }
      r8.w = (int)r8.w + 1;
    }
    r1.xyw = r8.xyz * r11.www + r6.xyz;
  }
  r5.xyz = r7.xyz * r6.www;
  r1.xyw = r11.xyz * r1.xyw + r5.xyz;
  r0.x = cmp(0 < r2.y);
  r0.z = dot(r3.xyz, float3(0.298999995,0.587000012,0.114));
  r0.z = r0.z + -r2.y;
  r0.z = max(0, r0.z);
  r0.z = r0.z / r2.y;
  r0.z = min(1, r0.z);
  r0.x = r0.x ? r0.z : 1;
  r0.z = r13.w * r1.z;
  r0.x = r0.z * r0.x;
  r0.z = -1 + r0.y;
  r0.z = r2.z * r0.z + 1;
  r0.x = r0.x * r0.z;
  r0.z = -fogNearDistance_g + -r3.w;
  r0.z = saturate(fogFadeRangeInv_g * r0.z);
  r1.z = -fogHeight_g + r4.y;
  r1.z = saturate(fogHeightRangeInv_g * r1.z);
  r0.z = r1.z * r0.z;
  r1.z = fogIntensity_g * r0.z;
  r1.z = r1.z * r12.x;
  r2.xyz = fogColor_g.xyz + -r1.xyw;
  r1.xyz = r1.zzz * r2.xyz + r1.xyw;
  r0.w = (int)r0.w & 128;
  r2.xyz = mapColor_g.xyz * r1.xyz;
  r1.xyz = r0.www ? r1.xyz : r2.xyz;
  if (outlineShapeCount_g != 0) {
    r4.w = 1;
    r2.xyz = r1.xyz;
    r0.w = 0;
    while (true) {
      r1.w = cmp((uint)r0.w >= outlineShapeCount_g);
      if (r1.w != 0) break;
      r3.x = outlineShapes_g[r0.w].gradation_sharpness;
      r3.y = outlineShapes_g[r0.w].height_base;
      r3.z = outlineShapes_g[r0.w].height_width;
      r3.w = outlineShapes_g[r0.w].height_gradation_width;
      r1.w = r4.y + -r3.y;
      r3.y = cmp(r3.z >= abs(r1.w));
      if (r3.y != 0) {
        r5.x = outlineShapes_g[r0.w].mtx._m00;
        r5.y = outlineShapes_g[r0.w].mtx._m10;
        r5.z = outlineShapes_g[r0.w].mtx._m20;
        r5.w = outlineShapes_g[r0.w].mtx._m30;
        r6.x = outlineShapes_g[r0.w].mtx._m02;
        r6.y = outlineShapes_g[r0.w].mtx._m32;
        r6.z = outlineShapes_g[r0.w].mtx._m12;
        r6.w = outlineShapes_g[r0.w].mtx._m22;
        r7.x = outlineShapes_g[r0.w].color.x;
        r7.y = outlineShapes_g[r0.w].color.y;
        r7.z = outlineShapes_g[r0.w].color.z;
        r7.w = outlineShapes_g[r0.w].color.w;
        r8.x = outlineShapes_g[r0.w].type;
        r8.y = outlineShapes_g[r0.w].radius;
        r8.z = outlineShapes_g[r0.w].gradation_size.x;
        r8.w = outlineShapes_g[r0.w].gradation_size.y;
        r3.y = abs(r1.w) + r3.w;
        r3.y = cmp(r3.z < r3.y);
        r1.w = r3.z + -abs(r1.w);
        r1.w = r1.w / r3.w;
        r1.w = r3.y ? r1.w : 1;
        if (r8.x == 0) {
          r6.x = r5.w;
          r3.yz = -r6.xy + r4.xz;
          r3.y = dot(r3.yz, r3.yz);
          r3.y = sqrt(r3.y);
          r3.z = cmp(r8.y < r3.y);
          r3.w = r8.y + -r8.z;
          r9.x = cmp(r3.y >= r3.w);
          r9.y = ~(int)r9.x;
          r3.y = r3.y + -r3.w;
          r3.y = r3.y / r8.z;
          r3.y = r9.x ? r3.y : 0;
          r3.y = r3.z ? 0 : r3.y;
          r3.z = (int)r3.z | (int)r9.y;
        } else {
          r3.w = cmp((int)r8.x == 1);
          if (r3.w != 0) {
            r3.w = outlineShapes_g[r0.w].fan_angle;
            r6.xz = r5.wz;
            r9.xy = -r6.xy + r4.xz;
            r9.z = dot(r9.xy, r9.xy);
            r9.w = sqrt(r9.z);
            r10.x = cmp(r8.y < r9.w);
            r8.y = r8.y + -r8.z;
            r9.z = rsqrt(r9.z);
            r9.xy = r9.xy * r9.zz;
            r9.x = dot(r6.zw, r9.xy);
            r9.y = 1 + -abs(r9.x);
            r9.y = sqrt(r9.y);
            r9.z = abs(r9.x) * -0.0187292993 + 0.0742610022;
            r9.z = r9.z * abs(r9.x) + -0.212114394;
            r9.z = r9.z * abs(r9.x) + 1.57072878;
            r10.y = r9.z * r9.y;
            r10.y = r10.y * -2 + 3.14159274;
            r9.x = cmp(r9.x < -r9.x);
            r9.x = r9.x ? r10.y : 0;
            r9.x = r9.z * r9.y + r9.x;
            r9.y = cmp(r3.w >= r9.x);
            r9.z = ~(int)r9.y;
            r10.y = 6.28318548 * r9.w;
            r9.x = r10.y * r9.x;
            r9.x = 0.159154937 * r9.x;
            r3.w = r10.y * r3.w;
            r3.w = r3.w * 0.159154937 + -r9.x;
            r3.w = r3.w / r8.z;
            r3.w = min(1, r3.w);
            r3.w = 1 + -r3.w;
            r8.y = r9.w + -r8.y;
            r8.y = r8.y / r8.z;
            r8.y = min(1, r8.y);
            r3.w = max(r8.y, r3.w);
            r3.w = r9.y ? r3.w : 0;
            r3.y = r10.x ? 0 : r3.w;
            r3.z = (int)r10.x | (int)r9.z;
          } else {
            r3.w = cmp((int)r8.x == 2);
            r5.x = dot(r4.xyzw, r5.xyzw);
            r5.y = dot(r4.xwyz, r6.xyzw);
            r5.zw = cmp(r5.xy < float2(0.5,0.5));
            r5.z = r5.w ? r5.z : 0;
            r6.xy = cmp(float2(-0.5,-0.5) < r5.xy);
            r5.w = r6.y ? r6.x : 0;
            r5.z = r5.w ? r5.z : 0;
            r6.xy = float2(0.5,0.5) + -r8.zw;
            r6.zw = cmp(abs(r5.xy) < r6.xy);
            r5.w = r6.w ? r6.z : 0;
            r5.xy = -r6.xy + abs(r5.xy);
            r5.xy = r5.xy / r8.zw;
            r5.x = max(r5.x, r5.y);
            r5.x = r5.w ? 0 : r5.x;
            r5.x = r5.z ? r5.x : 0;
            r5.y = r5.z ? r5.w : -1;
            r3.yz = r3.ww ? r5.xy : 0;
          }
        }
        r3.y = log2(r3.y);
        r3.x = r3.x * r3.y;
        r3.x = exp2(r3.x);
        r3.x = r7.w * r3.x;
        r1.w = r3.x * r1.w;
        r3.xyw = r7.xyz + -r2.xyz;
        r3.xyw = r1.www * r3.xyw + r2.xyz;
        r2.xyz = r3.zzz ? r2.xyz : r3.xyw;
      }
      r0.w = (int)r0.w + 1;
    }
    r1.xyz = r2.xyz;
  }
  if (roundShadowCount_g != 0) {
    r2.xyz = r1.xyz;
    r0.w = r0.y;
    r1.w = 0;
    while (true) {
      r3.x = cmp((uint)r1.w >= roundShadowCount_g);
      if (r3.x != 0) break;
      r3.x = roundShadows_g[r1.w].pos.x;
      r3.y = roundShadows_g[r1.w].pos.y;
      r3.z = roundShadows_g[r1.w].pos.z;
      r3.w = roundShadows_g[r1.w].radius;
      r5.x = roundShadows_g[r1.w].gradation_size;
      r5.y = roundShadows_g[r1.w].inner_radius;
      r5.z = roundShadows_g[r1.w].alpha;
      r5.w = roundShadows_g[r1.w].height_width;
      r4.w = roundShadows_g[r1.w].type;
      r4.w = cmp((int)r4.w == 1);
      if (r4.w != 0) {
        r6.x = cmp(r0.w != 0.000000);
        r6.yzw = r4.yxz + -r3.yxz;
        r7.x = cmp(r5.w >= abs(r6.y));
        r6.z = dot(r6.zw, r6.zw);
        r6.z = sqrt(r6.z);
        r6.w = cmp(r3.w >= r6.z);
        r6.y = -abs(r6.y) * 2 + 1;
        r6.y = max(0, r6.y);
        r6.z = r6.z + -r5.y;
        r6.z = r6.z / r5.x;
        r6.z = saturate(1 + -r6.z);
        r6.z = r6.z * r5.z;
        r6.y = r6.z * r6.y;
        r6.y = r6.y * r0.w;
        r7.yzw = r6.yyy * -r2.xyz + r2.xyz;
        r6.yzw = r6.www ? r7.yzw : r2.xyz;
        r6.yzw = r7.xxx ? r6.yzw : r2.xyz;
        r2.xyz = r6.xxx ? r6.yzw : r2.xyz;
      }
      if (r4.w == 0) {
        r4.w = cmp(r0.w != 1.000000);
        r3.xyz = r4.yxz + -r3.yxz;
        r5.w = cmp(r5.w >= abs(r3.x));
        r3.y = dot(r3.yz, r3.yz);
        r3.y = sqrt(r3.y);
        r3.z = cmp(r3.w >= r3.y);
        r3.x = -abs(r3.x) * 2 + 1;
        r3.x = max(0, r3.x);
        r3.y = r3.y + -r5.y;
        r3.y = r3.y / r5.x;
        r3.y = saturate(1 + -r3.y);
        r3.y = r5.z * r3.y;
        r3.x = r3.y * r3.x;
        r3.y = 1 + -r0.w;
        r3.x = r3.x * r3.y;
        r3.xyw = r3.xxx * -r2.xyz + r2.xyz;
        r3.xyz = r3.zzz ? r3.xyw : r2.xyz;
        r3.xyz = r5.www ? r3.xyz : r2.xyz;
        r2.xyz = r4.www ? r3.xyz : r2.xyz;
      }
      r1.w = (int)r1.w + 1;
    }
    r1.xyz = r2.xyz;
  }
  r0.y = -r0.z * fogIntensity_g + 1;
  r0.y = saturate(r2.w * r0.y);
  r0.x = saturate(0.100000001 * r0.x);
  r2.y = r16.w ? r0.x : 0;
  r2.xzw = float3(255,255,255);
  r2.xyzw = float4(0,255,0,0) * r2.xyzw;
  o1.xyzw = (uint4)r2.xyzw;
  r0.x = 31 * r0.y;
  r0.x = (uint)r0.x;
  r0.y = mad((int)r0.x, 32, (int)r0.x);
  o2.x = mad((int)r0.x, 1024, (int)r0.y);
  // ── GTVBAO Debug View ──
  if (shader_injection_data.gtvbao_debug_view > 0.5f) {
    int gtvbao_debug_mode = (int)shader_injection_data.gtvbao_debug_view;
    float hdr_scale = 0.3f;
    if (gtvbao_debug_mode == 1) {
      o0.rgb = float3(GTVBAO_ao * hdr_scale, 0.0f, 0.0f);
      o0.a = 1.0f; o1.xyzw = r2.xyzw; o2.xy = r3.xy; return;
    }
    if (gtvbao_debug_mode == 4) {
      o0.rgb = float3(GTVBAO_ao, GTVBAO_ao, GTVBAO_ao) * hdr_scale;
      o0.a = 1.0f; o1.xyzw = r2.xyzw; o2.xy = r3.xy; return;
    }
  }

  // ── Apply GTVBAO: modulate final lit color ──
  if (GTVBAO_bound) {
    r1.xyz *= GTVBAO_ao;
  }

  // ── Apply VBGI: add indirect diffuse ──
  if (shader_injection_data.gtvbao_vbgi_bound > 0.5f) {
    if (shader_injection_data.gtvbao_vbgi_debug > 0.5f) {
      r1.xyz = cachedVBGI;
    } else {
      r1.xyz += cachedVBGI;
    }
  }

  o0.xyz = r1.xyz;
  o0.w = 1;
  o2.y = 0;
  return;
}