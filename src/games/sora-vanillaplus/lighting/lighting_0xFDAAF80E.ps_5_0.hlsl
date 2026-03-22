// ---- Created with 3Dmigoto v1.4.1 on Sun Mar 22 00:32:12 2026

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
    uint flag;                     // Offset:  132
    float2 reserve;                // Offset:  136
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
  uint localLightProbeCount_g : packoffset(c32);
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
  float4x4 prevViewProj_g : packoffset(c74);
  float2 jitterDiff_g : packoffset(c78);
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

cbuffer cb_deferred : register(b4)
{
  uint outlineShapeCount_g : packoffset(c0);
  float2 shadowUVClamp_g : packoffset(c0.z);
  float4 outlineShapeMaskUVParam_g : packoffset(c1);
  float3 mapAOColor_g : packoffset(c2);
  float mapAOIntensity_g : packoffset(c2.w);
  float3 shadowEdgeColor_g : packoffset(c3);
  float shadowEdgeSharpness_g : packoffset(c3.w);
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
Texture2D<float4> ssaoTexture : register(t5);
StructuredBuffer<DeferredParam> deferredParams_g : register(t6);
StructuredBuffer<OutlineShapeParam> outlineShapes_g : register(t7);
Texture2D<float4> outlineShapeMask : register(t8);
Texture2D<float4> shadowTexture : register(t9);
Texture2D<float4> outlinePrepareTexture : register(t10);
StructuredBuffer<LightParam> dynamicLights_g : register(t11);
StructuredBuffer<LightIndexData> lightIndices_g : register(t12);
StructuredBuffer<float4x4> spotShadowMatrices_g : register(t14);
TextureCube<float4> texEnvMap_g : register(t17);
Texture2DArray<float4> spotShadowMaps : register(t18);
Texture3D<float4> atmosphereInscatterLUT : register(t19);
Texture3D<float4> atmosphereExtinctionLUT : register(t20);
Texture2D<float4> texMirror_g : register(t21);
Texture2D<float4> texSSRMap_g : register(t24);


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_Position0,
  float4 v1 : TEXCOORD0,
  out float4 o0 : SV_Target0,
  out uint4 o1 : SV_Target1,
  out uint2 o2 : SV_Target2)
{
  float4 r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21;
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
  r4.xyz = ssaoTexture.SampleLevel(samLinear_s, v1.xy, 0).xyz;
  r1.w = (int)r1.z & 8;
  if (r1.w == 0) {
    r5.yz = (uint2)r3.xx >> int2(5,10);
    r5.x = r3.x;
    r5.xyz = (int3)r5.xyz & int3(31,31,31);
    r5.xyz = (uint3)r5.xyz;
    r6.x = 0.0322580636;
    r6.yz = r0.yz;
    r5.xyz = r6.xyz * r5.xyz;
    r1.w = (int)r1.z & 1;
    if (r1.w != 0) {
      r6.x = r0.x;
      r6.yz = float2(0.0322580636,0.0322580636);
      r7.xyz = r6.xyz * r5.xyz;
      r1.w = r4.y * r4.z;
      r4.yzw = -r6.xyz * r5.xyz + r0.xyz;
      r6.xyz = r1.www * r4.yzw + r7.xyz;
      r1.w = (int)r1.z & 4;
      if (r1.w == 0) {
        r3.zw = outlinePrepareTexture.SampleLevel(samPoint_s, v1.xy, 0).xy;
        r7.x = r3.z;
        r7.yw = float2(1,1);
        r1.w = dot(projInv_g._m22_m32, r7.xy);
        r3.z = dot(projInv_g._m23_m33, r7.xy);
        r1.w = r1.w / r3.z;
        r8.xyzw = -fadeBeginDistance_g.xyzw + -r1.wwww;
        r8.xyzw = saturate(fadeRangeInv_g.xyzw * r8.xyzw);
        r9.xy = maxThickness_g;
        r9.z = densityByDepthDiffNear_g;
        r10.x = 1 + -r9.x;
        r10.y = depthThresholdFar_g + -r9.y;
        r10.z = densityByDepthDiffFar_g + -r9.z;
        r4.yzw = r8.yzw * r10.xyz + r9.xyz;
        r5.yz = offsetsAndWeights[0].xy * r4.yy + v1.xy;
        r5.yz = min(uvClamp_g.xy, r5.yz);
        r7.xz = outlinePrepareTexture.SampleLevel(samLinear_s, r5.yz, 0).yx;
        r3.z = dot(projInv_g._m22_m32, r7.zw);
        r5.y = dot(projInv_g._m23_m33, r7.zw);
        r3.z = r3.z / r5.y;
        r5.y = cmp(r7.x < 0);
        r3.z = -r3.z + r1.w;
        r5.z = cmp(r3.z >= 0);
        r5.z = r5.z ? 1.000000 : 0;
        r5.w = cmp(r3.z >= r4.z);
        r9.x = r5.w ? 1.000000 : 0;
        r5.w = -r7.x + r3.w;
        r7.x = cmp(abs(r5.w) >= 0.0299999993);
        r7.x = r7.x ? 1.000000 : 0;
        r9.y = r5.z * r7.x + r9.x;
        r7.z = abs(r3.z);
        r7.w = r5.z * abs(r5.w) + r7.z;
        r5.zw = offsetsAndWeights[1].xy * r4.yy + v1.xy;
        r5.zw = min(uvClamp_g.xy, r5.zw);
        r5.zw = outlinePrepareTexture.SampleLevel(samLinear_s, r5.zw, 0).xy;
        r10.x = r5.z;
        r10.yw = float2(1,1);
        r3.z = dot(projInv_g._m22_m32, r10.xy);
        r5.z = dot(projInv_g._m23_m33, r10.xy);
        r3.z = r3.z / r5.z;
        r5.z = cmp(r5.w < 0);
        r5.y = (int)r5.y | (int)r5.z;
        r3.z = -r3.z + r1.w;
        r5.z = cmp(r3.z >= 0);
        r5.z = r5.z ? 1.000000 : 0;
        r7.x = cmp(r3.z >= r4.z);
        r7.x = r7.x ? 1.000000 : 0;
        r5.w = -r5.w + r3.w;
        r8.y = cmp(abs(r5.w) >= 0.0299999993);
        r8.y = r8.y ? 1.000000 : 0;
        r7.y = r5.z * r8.y + r7.x;
        r7.xy = offsetsAndWeights[1].zz * r7.xy;
        r7.xy = r9.xy * offsetsAndWeights[0].zz + r7.xy;
        r8.z = abs(r3.z);
        r8.w = r5.z * abs(r5.w) + r8.z;
        r5.zw = max(r8.zw, r7.zw);
        r7.zw = offsetsAndWeights[2].xy * r4.yy + v1.xy;
        r7.zw = min(uvClamp_g.xy, r7.zw);
        r10.xz = outlinePrepareTexture.SampleLevel(samLinear_s, r7.zw, 0).yx;
        r3.z = dot(projInv_g._m22_m32, r10.zw);
        r7.z = dot(projInv_g._m23_m33, r10.zw);
        r3.z = r3.z / r7.z;
        r7.z = cmp(r10.x < 0);
        r5.y = (int)r5.y | (int)r7.z;
        r3.z = -r3.z + r1.w;
        r7.z = cmp(r3.z >= 0);
        r7.z = r7.z ? 1.000000 : 0;
        r7.w = cmp(r3.z >= r4.z);
        r9.x = r7.w ? 1.000000 : 0;
        r7.w = -r10.x + r3.w;
        r8.y = cmp(abs(r7.w) >= 0.0299999993);
        r8.y = r8.y ? 1.000000 : 0;
        r9.y = r7.z * r8.y + r9.x;
        r7.xy = r9.xy * offsetsAndWeights[2].zz + r7.xy;
        r8.z = abs(r3.z);
        r8.w = r7.z * abs(r7.w) + r8.z;
        r5.zw = max(r8.zw, r5.zw);
        r7.zw = offsetsAndWeights[3].xy * r4.yy + v1.xy;
        r7.zw = min(uvClamp_g.xy, r7.zw);
        r7.zw = outlinePrepareTexture.SampleLevel(samLinear_s, r7.zw, 0).xy;
        r9.x = r7.z;
        r9.yw = float2(1,1);
        r3.z = dot(projInv_g._m22_m32, r9.xy);
        r7.z = dot(projInv_g._m23_m33, r9.xy);
        r3.z = r3.z / r7.z;
        r7.z = cmp(r7.w < 0);
        r5.y = (int)r5.y | (int)r7.z;
        r3.z = -r3.z + r1.w;
        r7.z = cmp(r3.z >= 0);
        r7.z = r7.z ? 1.000000 : 0;
        r8.y = cmp(r3.z >= r4.z);
        r10.x = r8.y ? 1.000000 : 0;
        r7.w = -r7.w + r3.w;
        r8.y = cmp(abs(r7.w) >= 0.0299999993);
        r8.y = r8.y ? 1.000000 : 0;
        r10.y = r7.z * r8.y + r10.x;
        r7.xy = r10.xy * offsetsAndWeights[3].zz + r7.xy;
        r8.z = abs(r3.z);
        r8.w = r7.z * abs(r7.w) + r8.z;
        r5.zw = max(r8.zw, r5.zw);
        r7.zw = offsetsAndWeights[4].xy * r4.yy + v1.xy;
        r7.zw = min(uvClamp_g.xy, r7.zw);
        r9.xz = outlinePrepareTexture.SampleLevel(samLinear_s, r7.zw, 0).yx;
        r3.z = dot(projInv_g._m22_m32, r9.zw);
        r7.z = dot(projInv_g._m23_m33, r9.zw);
        r3.z = r3.z / r7.z;
        r7.z = cmp(r9.x < 0);
        r5.y = (int)r5.y | (int)r7.z;
        r3.z = -r3.z + r1.w;
        r7.z = cmp(r3.z >= 0);
        r7.z = r7.z ? 1.000000 : 0;
        r7.w = cmp(r3.z >= r4.z);
        r10.x = r7.w ? 1.000000 : 0;
        r7.w = -r9.x + r3.w;
        r8.y = cmp(abs(r7.w) >= 0.0299999993);
        r8.y = r8.y ? 1.000000 : 0;
        r10.y = r7.z * r8.y + r10.x;
        r7.xy = r10.xy * offsetsAndWeights[4].zz + r7.xy;
        r8.z = abs(r3.z);
        r8.w = r7.z * abs(r7.w) + r8.z;
        r5.zw = max(r8.zw, r5.zw);
        r7.zw = offsetsAndWeights[5].xy * r4.yy + v1.xy;
        r7.zw = min(uvClamp_g.xy, r7.zw);
        r7.zw = outlinePrepareTexture.SampleLevel(samLinear_s, r7.zw, 0).xy;
        r9.x = r7.z;
        r9.yw = float2(1,1);
        r3.z = dot(projInv_g._m22_m32, r9.xy);
        r7.z = dot(projInv_g._m23_m33, r9.xy);
        r3.z = r3.z / r7.z;
        r7.z = cmp(r7.w < 0);
        r5.y = (int)r5.y | (int)r7.z;
        r3.z = -r3.z + r1.w;
        r7.z = cmp(r3.z >= 0);
        r7.z = r7.z ? 1.000000 : 0;
        r8.y = cmp(r3.z >= r4.z);
        r10.x = r8.y ? 1.000000 : 0;
        r7.w = -r7.w + r3.w;
        r8.y = cmp(abs(r7.w) >= 0.0299999993);
        r8.y = r8.y ? 1.000000 : 0;
        r10.y = r7.z * r8.y + r10.x;
        r7.xy = r10.xy * offsetsAndWeights[5].zz + r7.xy;
        r8.z = abs(r3.z);
        r8.w = r7.z * abs(r7.w) + r8.z;
        r5.zw = max(r8.zw, r5.zw);
        r7.zw = offsetsAndWeights[6].xy * r4.yy + v1.xy;
        r7.zw = min(uvClamp_g.xy, r7.zw);
        r9.xz = outlinePrepareTexture.SampleLevel(samLinear_s, r7.zw, 0).yx;
        r3.z = dot(projInv_g._m22_m32, r9.zw);
        r7.z = dot(projInv_g._m23_m33, r9.zw);
        r3.z = r3.z / r7.z;
        r7.z = cmp(r9.x < 0);
        r5.y = (int)r5.y | (int)r7.z;
        r3.z = -r3.z + r1.w;
        r7.z = cmp(r3.z >= 0);
        r7.z = r7.z ? 1.000000 : 0;
        r7.w = cmp(r3.z >= r4.z);
        r10.x = r7.w ? 1.000000 : 0;
        r7.w = -r9.x + r3.w;
        r8.y = cmp(abs(r7.w) >= 0.0299999993);
        r8.y = r8.y ? 1.000000 : 0;
        r10.y = r7.z * r8.y + r10.x;
        r7.xy = r10.xy * offsetsAndWeights[6].zz + r7.xy;
        r8.z = abs(r3.z);
        r8.w = r7.z * abs(r7.w) + r8.z;
        r5.zw = max(r8.zw, r5.zw);
        r7.zw = offsetsAndWeights[7].xy * r4.yy + v1.xy;
        r7.zw = min(uvClamp_g.xy, r7.zw);
        r7.zw = outlinePrepareTexture.SampleLevel(samLinear_s, r7.zw, 0).xy;
        r9.x = r7.z;
        r9.y = 1;
        r3.z = dot(projInv_g._m22_m32, r9.xy);
        r4.y = dot(projInv_g._m23_m33, r9.xy);
        r3.z = r3.z / r4.y;
        r4.y = cmp(r7.w < 0);
        r4.y = (int)r5.y | (int)r4.y;
        r1.w = -r3.z + r1.w;
        r3.z = cmp(r1.w >= 0);
        r3.z = r3.z ? 1.000000 : 0;
        r4.z = cmp(r1.w >= r4.z);
        r9.x = r4.z ? 1.000000 : 0;
        r3.w = -r7.w + r3.w;
        r4.z = cmp(abs(r3.w) >= 0.0299999993);
        r4.z = r4.z ? 1.000000 : 0;
        r9.y = r3.z * r4.z + r9.x;
        r7.xy = r9.xy * offsetsAndWeights[7].zz + r7.xy;
        r8.z = abs(r1.w);
        r8.w = r3.z * abs(r3.w) + r8.z;
        r7.zw = max(r8.zw, r5.zw);
        r3.zw = r4.yy ? r7.xz : r7.yw;
        r7.yw = (uint2)r2.zw >> int2(4,4);
        r7.xz = r2.zw;
        r7.xyzw = (int4)r7.xyzw & int4(15,15,15,15);
        r7.xyzw = (uint4)r7.xyzw;
        r6.w = 0.0666666701;
        r9.xyzw = r7.xyzw * r6.xyzw;
        r1.w = cmp(0 < r7.w);
        r3.w = min(r3.w, r4.w);
        r3.w = r3.w / r4.w;
        r3.w = log2(r3.w);
        r3.w = 1.25 * r3.w;
        r3.w = exp2(r3.w);
        r3.z = r3.z * r3.z;
        r3.z = r3.z * r9.w;
        r3.z = density_g * r3.z;
        r3.z = min(1, r3.z);
        r3.z = r3.z * r3.w;
        r3.w = 1 + -r8.x;
        r3.z = r3.z * r3.w;
        r4.yzw = r9.xyz * float3(0.0666666701,0.0666666701,0.0666666701) + -r6.xyz;
        r4.yzw = r3.zzz * r4.yzw + r6.xyz;
        r6.xyz = r1.www ? r4.yzw : r6.xyz;
      }
    } else {
      r1.w = 1 + -r4.x;
      r1.w = r5.x * r1.w;
      r4.yzw = r0.xyz * mapAOColor_g.xyz + -r0.xyz;
      r6.xyz = r1.www * r4.yzw + r0.xyz;
    }
    r6.w = r0.w;
    o0.xyzw = r6.xyzw;
    o1.xyzw = r2.xyzw;
    o2.xy = r3.xy;
    return;
  }
  r5.z = depthTexture.SampleLevel(samPoint_s, v1.xy, 0).x;
  r5.xy = v1.zw * float2(2,-2) + float2(-1,1);
  r5.w = 1;
  r6.x = dot(r5.xyzw, viewProjInv_g._m00_m10_m20_m30);
  r6.y = dot(r5.xyzw, viewProjInv_g._m01_m11_m21_m31);
  r6.z = dot(r5.xyzw, viewProjInv_g._m02_m12_m22_m32);
  r6.w = dot(r5.xyzw, viewProjInv_g._m03_m13_m23_m33);
  r5.xyzw = r6.xyzw / r6.wwww;
  r3.zw = min(shadowUVClamp_g.xy, v1.xy);
  r0.w = shadowTexture.SampleLevel(samLinear_s, r3.zw, 0).x;
  r1.xy = (uint2)r1.xy;
  r6.zw = r1.xy * float2(3.05180438e-05,3.05180438e-05) + float2(-1,-1);
  r1.x = 3.14159274 * r6.z;
  sincos(r1.x, r1.x, r7.x);
  r1.y = r6.w * r6.w;
  r1.w = -r6.w * r6.w + 1;
  r1.w = sqrt(r1.w);
  r6.x = r7.x * r1.w;
  r6.y = r1.x * r1.w;
  r2.xyzw = (uint4)r2.xyzw;
  r7.xyzw = float4(0.00392156886,0.00392156886,0.00392156886,0.00392156886) * r2.wxyz;
  r2.yz = (uint2)r3.yy >> int2(5,10);
  r2.x = r3.y;
  r2.xyz = (int3)r2.xyz & int3(31,31,31);
  r2.xyz = (uint3)r2.xyz;
  r3.yzw = float3(0.0322580636,0.0322580636,0.0322580636) * r2.xyz;
  r1.x = min(0x00004e1f, (uint)r3.x);
  r8.x = deferredParams_g[r1.x].shadowColor.x;
  r8.y = deferredParams_g[r1.x].shadowColor.y;
  r8.z = deferredParams_g[r1.x].shadowColor.z;
  r8.w = deferredParams_g[r1.x].emissive;
  r9.x = deferredParams_g[r1.x].specularColor.x;
  r9.y = deferredParams_g[r1.x].specularColor.y;
  r9.z = deferredParams_g[r1.x].specularColor.z;
  r9.w = deferredParams_g[r1.x].rimLightPower;
  r10.x = deferredParams_g[r1.x].rimLightColor.x;
  r10.y = deferredParams_g[r1.x].rimLightColor.y;
  r10.z = deferredParams_g[r1.x].rimLightColor.z;
  r10.w = deferredParams_g[r1.x].rimIntensity;
  r11.x = deferredParams_g[r1.x].fresnels.x;
  r11.y = deferredParams_g[r1.x].fresnels.y;
  r11.z = deferredParams_g[r1.x].fresnels.z;
  r12.x = deferredParams_g[r1.x].specularGlossinesses.x;
  r12.y = deferredParams_g[r1.x].specularGlossinesses.y;
  r12.z = deferredParams_g[r1.x].specularGlossinesses.z;
  r12.w = deferredParams_g[r1.x].dynamicLightIntensity;
  r13.x = deferredParams_g[r1.x].materialFogIntensity;
  r13.y = deferredParams_g[r1.x].metalness;
  r13.z = deferredParams_g[r1.x].roughness;
  r13.w = deferredParams_g[r1.x].cryRefractionIndex;
  r14.x = deferredParams_g[r1.x].cryFresnel;
  r14.y = deferredParams_g[r1.x].cryBrightness;
  r14.z = deferredParams_g[r1.x].cryBrightnessPower;
  r14.w = deferredParams_g[r1.x].glowIntensity;
  r4.y = deferredParams_g[r1.x].glowLumThreshold;
  r4.z = deferredParams_g[r1.x].glowShadowFadeRatio;
  r4.w = deferredParams_g[r1.x].ssaoIntensity;
  r1.x = deferredParams_g[r1.x].ssrDistance;
  r1.w = deferredParams_g[r1.x].flag;
  r11.w = r12.x;
  r15.xz = r11.yz;
  r15.yw = r12.yz;
  r2.xy = r15.xy + -r11.xw;
  r2.xy = r3.yy * r2.xy + r11.xw;
  r3.xy = r15.zw + -r2.xy;
  r2.xy = r3.zz * r3.xy + r2.xy;
  r3.x = dot(view_g._m02_m12_m22_m32, r5.xyzw);
  r3.yz = lightTileSizeInv_g.xy * v0.xy;
  r3.yz = (uint2)r3.yz;
  r3.z = (uint)r3.z << 5;
  r6.z = (int)r3.y + (int)r3.z;
  r6.z = lightIndices_g[r6.z].tileDepthInv;
  r6.z = r6.z * -r3.x;
  r6.z = min(7, r6.z);
  r6.z = max(0, r6.z);
  r6.z = (uint)r6.z;
  r3.z = mad((int)r6.z, 576, (int)r3.z);
  r3.y = (int)r3.y + (int)r3.z;
  r3.y = min(4607, (uint)r3.y);
  r11.xyz = lightProbe_g[1].xyz * r6.xxx + lightProbe_g[0].xyz;
  r11.xyz = lightProbe_g[2].xyz * r6.yyy + r11.xyz;
  r11.xyz = lightProbe_g[3].xyz * r6.www + r11.xyz;
  r12.xyz = lightProbe_g[4].xyz * r6.www;
  r11.xyz = r12.xyz * r6.xxx + r11.xyz;
  r12.xyz = lightProbe_g[5].xyz * r6.yyy;
  r11.xyz = r12.xyz * r6.www + r11.xyz;
  r12.xyz = lightProbe_g[6].xyz * r6.yyy;
  r11.xyz = r12.xyz * r6.xxx + r11.xyz;
  r1.y = r1.y * 3 + -1;
  r11.xyz = lightProbe_g[7].xyz * r1.yyy + r11.xyz;
  r1.y = r6.y * r6.y;
  r1.y = r6.x * r6.x + -r1.y;
  r11.xyz = lightProbe_g[8].xyz * r1.yyy + r11.xyz;
  r11.xyz = max(float3(0,0,0), r11.xyz);
  r12.x = viewInv_g._m30 + -r5.x;
  r12.y = viewInv_g._m31 + -r5.y;
  r12.z = viewInv_g._m32 + -r5.z;
  r1.y = dot(r12.xyz, r12.xyz);
  r1.y = rsqrt(r1.y);
  r15.xyz = r12.xyz * r1.yyy;
  r8.xyz = sceneShadowColor_g.xyz + r8.xyz;
  r8.xyz = min(float3(1,1,1), r8.xyz);
  r3.z = r8.w * r3.w;
  r6.z = dot(r6.xyw, r15.xyz);
  r16.xyzw = (int4)r1.wwww & int4(1,2,4,16);
  r0.w = r16.x ? 1.0f : r0.w;
  r17.xyz = r12.xyz * r1.yyy + -lightDirection_g.xyz;
  r8.w = dot(r17.xyz, r17.xyz);
  r8.w = rsqrt(r8.w);
  r17.xyz = r17.xyz * r8.www;
  r8.w = lightSpecularGlossiness_g * r2.y;
  r11.w = saturate(dot(r17.xyz, r6.xyw));
  r8.w = max(0.00100000005, r8.w);
  r11.w = log2(r11.w);
  r8.w = r11.w * r8.w;
  r8.w = exp2(r8.w);
  r8.w = r8.w * r0.w;
  r8.w = lightSpecularIntensity_g * r8.w;
  r8.w = r16.y ? r8.w : 0;
  r9.xyz = r8.www * r9.xyz;
  r9.xyz = lightColor_g.xyz * r9.xyz;
  if (r16.z != 0) {
    r16.xz = r13.yz * r7.yz;
    r7.y = (int)r1.w & 32;
    if (r7.y != 0) {
      r17.y = resolutionScaling_g.y + -v1.y;
      r17.x = v1.x;
      r17.xyz = texMirror_g.SampleLevel(SmplLinearClamp_s, r17.xy, 0).xyz;
    } else {
      r8.w = r6.z + r6.z;
      r18.xyz = r6.xyw * -r8.www + r15.xyz;
      texEnvMap_g.GetDimensions(0, width, height, num_levels);
      r18.xyz = float3(1,-1,-1) * r18.xyz;
      r8.w = (float)(num_levels - 1);
      r8.w = r16.z * r8.w;
      r17.xyz = texEnvMap_g.SampleLevel(SmplCube_s, r18.xyz, r8.w).xyz;
    }
    r8.w = (int)r1.z & 2;
    if (r8.w != 0) {
      r18.xy = resolutionScaling_g.xy * v1.zw;
      r18.xyzw = texSSRMap_g.SampleLevel(SmplLinearClamp_s, r18.xy, 0).xyzw;
      r18.xyz = r18.xyz + -r17.xyz;
      r17.xyz = r18.www * r18.xyz + r17.xyz;
    }
    r8.w = cmp(0 < r2.x);
    r11.w = 1 + -abs(r6.z);
    r11.w = max(0, r11.w);
    r11.w = log2(r11.w);
    r2.x = r11.w * r2.x;
    r2.x = exp2(r2.x);
    r2.x = r8.w ? r2.x : 1;
    r8.w = r16.x * r2.x;
    r18.xyz = r0.xyz * r17.xyz + -r0.xyz;
    r18.xyz = r8.www * r18.xyz + r0.xyz;
    r8.w = dot(r17.xyz, float3(0.298999995,0.587000012,0.114));
    r11.w = r16.z * -9 + 10;
    r8.w = log2(r8.w);
    r8.w = r11.w * r8.w;
    r8.w = exp2(r8.w);
    r11.w = 1 + -r8.w;
    r8.w = r16.x * r11.w + r8.w;
    r17.xyz = r17.xyz * r8.www;
    r17.xyz = r17.xyz * r2.xxx;
    r2.x = -r7.z * r13.z + 1;
    r17.xyz = r17.xyz * r2.xxx + r9.xyz;
    r9.xyz = r7.yyy ? r9.xyz : r17.xyz;
  } else {
    r1.w = (int)r1.w & 8;
    if (r1.w != 0) {
      r1.w = r6.z + r6.z;
      r17.xyz = r6.xyw * -r1.www + r15.xyz;
      r1.w = 1 / r13.w;
      r2.x = dot(-r15.xyz, r6.xyw);
      r7.y = r1.w * r1.w;
      r8.w = -r2.x * r2.x + 1;
      r7.y = -r7.y * r8.w + 1;
      r8.w = sqrt(r7.y);
      r2.x = r1.w * r2.x + r8.w;
      r7.y = cmp(r7.y >= 0);
      r19.xyz = r2.xxx * r6.xyw;
      r15.xyz = r1.www * -r15.xyz + -r19.xyz;
      r15.xyz = r7.yyy ? r15.xyz : 0;
      r1.w = r13.z * r7.z;
      texEnvMap_g.GetDimensions(0, width, height, num_levels);
      r17.xyz = float3(1,-1,-1) * r17.xyz;
      r2.x = (float)(num_levels - 1);
      r1.w = r2.x * r1.w;
      r17.xyz = texEnvMap_g.SampleLevel(SmplCube_s, r17.xyz, r1.w).xyz;
      r2.x = (int)r1.z & 2;
      if (r2.x != 0) {
        r13.yw = resolutionScaling_g.xy * v1.zw;
        r19.xyzw = texSSRMap_g.SampleLevel(SmplLinearClamp_s, r13.yw, 0).xyzw;
        r19.xyz = r19.xyz + -r17.xyz;
        r17.xyz = r19.www * r19.xyz + r17.xyz;
      }
      r15.xyz = texEnvMap_g.SampleLevel(SmplCube_s, r15.xyz, r1.w).xyz;
      r1.w = cmp(0 < r14.x);
      r2.x = 1 + -abs(r6.z);
      r2.x = max(0, r2.x);
      r2.x = log2(r2.x);
      r2.x = r14.x * r2.x;
      r2.x = exp2(r2.x);
      r1.w = r1.w ? r2.x : 1;
      r19.xyz = r0.xyz * r17.xyz + -r0.xyz;
      r19.xyz = r1.www * r19.xyz + r0.xyz;
      r2.x = -r2.w * 0.00392156886 + 1;
      r7.x = r1.w * r2.x + r7.x;
      r2.x = abs(r6.z) * r14.y;
      r2.x = log2(r2.x);
      r2.x = r14.z * r2.x;
      r2.x = exp2(r2.x);
      r14.xyz = r7.xxx * r17.xyz;
      r14.xyz = r14.xyz * r1.www;
      r1.w = -r7.z * r13.z + 1;
      r9.xyz = r14.xyz * r1.www + r9.xyz;
      r13.yzw = r2.xxx * r15.xyz;
      r18.xyz = r19.xyz * r13.yzw;
    } else {
      r18.xyz = r0.xyz;
    }
  }
  r1.w = r7.x * r0.w;
  r13.yzw = float3(1,1,1) + -r8.xyz;
  r8.xyz = r1.www * r13.yzw + r8.xyz;
  r1.w = log2(r0.w);
  r1.w = shadowEdgeSharpness_g * r1.w;
  r1.w = exp2(r1.w);
  r13.yzw = -shadowEdgeColor_g.xyz + r8.xyz;
  r13.yzw = r0.www * r13.yzw + shadowEdgeColor_g.xyz;
  r1.w = r1.w * r7.x;
  r7.xyz = r13.yzw + -r8.xyz;
  r7.xyz = r1.www * r7.xyz + r8.xyz;
  r7.xyz = r7.xyz * lightColor_g.xyz + r11.xyz;
  r1.w = min(1, r3.z);
  r8.xyz = float3(1,1,1) + -r7.xyz;
  r7.xyz = r1.www * r8.xyz + r7.xyz;
  r1.w = 1 + -abs(r6.z);
  r1.w = max(0, r1.w);
  r1.w = r1.w * r10.w;
  r1.w = log2(r1.w);
  r1.w = r9.w * r1.w;
  r1.w = exp2(r1.w);
  r1.w = min(1, r1.w);
  r8.xyz = r10.xyz * r1.www + r9.xyz;
  if (r16.y != 0) {
    r1.w = lightIndices_g[r3.y].pointLightCount;
    r1.w = min(63, (uint)r1.w);
    r9.xyz = float3(0,0,0);
    r10.xyz = float3(0,0,0);
    r2.x = 0;
    while (true) {
      r2.w = cmp((uint)r2.x >= (uint)r1.w);
      if (r2.w != 0) break;
      r2.w = lightIndices_g[r3.y].pointLightIndices[r2.x];
      r11.x = dynamicLights_g[r2.w].pos.x;
      r11.y = dynamicLights_g[r2.w].pos.y;
      r11.z = dynamicLights_g[r2.w].pos.z;
      r11.xyz = r11.xyz + -r5.xyz;
      r3.z = dot(r11.xyz, r11.xyz);
      r6.z = sqrt(r3.z);
      r8.w = dynamicLights_g[r2.w].radiusInv;
      r6.z = r8.w * r6.z;
      r8.w = dynamicLights_g[r2.w].attenuation;
      r6.z = log2(abs(r6.z));
      r6.z = r8.w * r6.z;
      r6.z = exp2(r6.z);
      r6.z = 1 + -r6.z;
      r6.z = max(0, r6.z);
      r8.w = cmp(0 < r6.z);
      if (r8.w != 0) {
        r3.z = rsqrt(r3.z);
        r11.xyz = r11.xyz * r3.zzz;
        r3.z = dynamicLights_g[r2.w].translucency;
        r8.w = dot(r11.xyz, r6.xyw);
        r3.z = max(r8.w, r3.z);
        r3.z = r6.z * r3.z;
        r13.y = dynamicLights_g[r2.w].color.x;
        r13.z = dynamicLights_g[r2.w].color.y;
        r13.w = dynamicLights_g[r2.w].color.z;
        r10.xyz = r13.yzw * r3.zzz + r10.xyz;
        r11.xyz = r12.xyz * r1.yyy + r11.xyz;
        r6.z = dot(r11.xyz, r11.xyz);
        r6.z = rsqrt(r6.z);
        r11.xyz = r11.xyz * r6.zzz;
        r14.x = dynamicLights_g[r2.w].specularIntensity;
        r14.y = dynamicLights_g[r2.w].specularGlossiness;
        r2.w = r14.y * r2.y;
        r6.z = saturate(dot(r11.xyz, r6.xyw));
        r2.w = max(0.00100000005, r2.w);
        r6.z = log2(r6.z);
        r2.w = r6.z * r2.w;
        r2.w = exp2(r2.w);
        r11.xyz = r13.yzw * r2.www;
        r11.xyz = r11.xyz * r3.zzz;
        r9.xyz = r11.xyz * r14.xxx + r9.xyz;
      }
      r2.x = (int)r2.x + 1;
    }
    r10.xyz = r10.xyz * r12.www + r7.xyz;
    r1.w = lightIndices_g[r3.y].spotLightCount;
    r1.w = min(63, (uint)r1.w);
    r11.yw = localShadowResolutionInv_g * float2(1.25,-1.25);
    r11.xz = float2(0,0);
    r13.yzw = r9.xyz;
    r14.xyz = float3(0,0,0);
    r2.x = 0;
    while (true) {
      r2.w = cmp((uint)r2.x >= (uint)r1.w);
      if (r2.w != 0) break;
      r2.w = lightIndices_g[r3.y].spotLightIndices[r2.x];
      r15.x = dynamicLights_g[r2.w].pos.x;
      r15.y = dynamicLights_g[r2.w].pos.y;
      r15.z = dynamicLights_g[r2.w].pos.z;
      r15.xyz = r15.xyz + -r5.xyz;
      r3.z = dot(r15.xyz, r15.xyz);
      r6.z = rsqrt(r3.z);
      r15.xyz = r15.xyz * r6.zzz;
      r17.x = dynamicLights_g[r2.w].vec.x;
      r17.y = dynamicLights_g[r2.w].vec.y;
      r17.z = dynamicLights_g[r2.w].vec.z;
      r17.w = dynamicLights_g[r2.w].spotAngleInv;
      r6.z = dot(r15.xyz, r17.xyz);
      r6.z = max(0, r6.z);
      r6.z = 1 + -r6.z;
      r6.z = r6.z * r17.w;
      r8.w = dynamicLights_g[r2.w].attenuationAngle;
      r6.z = log2(r6.z);
      r6.z = r8.w * r6.z;
      r6.z = exp2(r6.z);
      r6.z = 1 + -r6.z;
      r6.z = max(0, r6.z);
      r8.w = cmp(0 < r6.z);
      if (r8.w != 0) {
        r3.z = sqrt(r3.z);
        r8.w = dynamicLights_g[r2.w].radiusInv;
        r3.z = r8.w * r3.z;
        r8.w = dynamicLights_g[r2.w].attenuation;
        r3.z = log2(abs(r3.z));
        r3.z = r8.w * r3.z;
        r3.z = exp2(r3.z);
        r3.z = 1 + -r3.z;
        r3.z = max(0, r3.z);
        r3.z = r6.z * r3.z;
        r6.z = cmp(0 < r3.z);
        if (r6.z != 0) {
          r16.x = dynamicLights_g[r2.w].translucency;
          r16.y = dynamicLights_g[r2.w].shadowmapIndex;
          r6.z = cmp((int)r16.y != -1);
          if (r6.z != 0) {
            r17.xyzw = spotShadowMatrices_g[r16.y]._m00_m10_m20_m30;
            r19.xyzw = spotShadowMatrices_g[r16.y]._m01_m11_m21_m31;
            r20.xyzw = spotShadowMatrices_g[r16.y]._m02_m12_m22_m32;
            r21.xyzw = spotShadowMatrices_g[r16.y]._m03_m13_m23_m33;
            r17.x = dot(r5.xyzw, r17.xyzw);
            r17.y = dot(r5.xyzw, r19.xyzw);
            r17.z = dot(r5.xyzw, r20.xyzw);
            r6.z = dot(r5.xyzw, r21.xyzw);
            r17.xyz = r17.xyz / r6.zzz;
            r17.w = (uint)r16.y;
            r6.z = spotShadowMaps.SampleCmpLevelZero(SmplShadow_s, r17.xyw, r17.z).x;
            r19.xy = r17.xy + r11.yx;
            r19.z = r17.w;
            r8.w = spotShadowMaps.SampleCmpLevelZero(SmplShadow_s, r19.xyz, r17.z).x;
            r8.w = 0.200000003 * r8.w;
            r6.z = r6.z * 0.200000003 + r8.w;
            r19.xy = r17.xy + r11.wz;
            r8.w = spotShadowMaps.SampleCmpLevelZero(SmplShadow_s, r19.xyz, r17.z).x;
            r6.z = r8.w * 0.200000003 + r6.z;
            r19.xy = r17.xy + r11.xy;
            r8.w = spotShadowMaps.SampleCmpLevelZero(SmplShadow_s, r19.xyz, r17.z).x;
            r6.z = r8.w * 0.200000003 + r6.z;
            r19.xy = r17.xy + r11.zw;
            r8.w = spotShadowMaps.SampleCmpLevelZero(SmplShadow_s, r19.xyz, r17.z).x;
            r6.z = r8.w * 0.200000003 + r6.z;
            r3.z = r6.z * r3.z;
          }
          r6.z = dot(r15.xyz, r6.xyw);
          r6.z = max(r16.x, r6.z);
          r3.z = r6.z * r3.z;
          r16.x = dynamicLights_g[r2.w].color.x;
          r16.y = dynamicLights_g[r2.w].color.y;
          r16.z = dynamicLights_g[r2.w].color.z;
          r14.xyz = r16.xyz * r3.zzz + r14.xyz;
          r15.xyz = r12.xyz * r1.yyy + r15.xyz;
          r6.z = dot(r15.xyz, r15.xyz);
          r6.z = rsqrt(r6.z);
          r15.xyz = r15.xyz * r6.zzz;
          r17.x = dynamicLights_g[r2.w].specularIntensity;
          r17.y = dynamicLights_g[r2.w].specularGlossiness;
          r2.w = r17.y * r2.y;
          r6.z = saturate(dot(r15.xyz, r6.xyw));
          r2.w = max(0.00100000005, r2.w);
          r6.z = log2(r6.z);
          r2.w = r6.z * r2.w;
          r2.w = exp2(r2.w);
          r15.xyz = r16.xyz * r2.www;
          r15.xyz = r15.xyz * r3.zzz;
          r13.yzw = r15.xyz * r17.xxx + r13.yzw;
        }
      }
      r2.x = (int)r2.x + 1;
    }
    r2.xyw = r14.xyz * r12.www + r10.xyz;
    r8.xyz = r13.yzw * r12.www + r8.xyz;
  } else {
    r1.y = lightIndices_g[r3.y].pointLightCount;
    r1.y = min(63, (uint)r1.y);
    r9.xyzw = float4(0,0,0,0);
    while (true) {
      r1.w = cmp((uint)r9.w >= (uint)r1.y);
      if (r1.w != 0) break;
      r1.w = lightIndices_g[r3.y].pointLightIndices[r9.w];
      r10.x = dynamicLights_g[r1.w].pos.x;
      r10.y = dynamicLights_g[r1.w].pos.y;
      r10.z = dynamicLights_g[r1.w].pos.z;
      r10.xyz = r10.xyz + -r5.xyz;
      r3.z = dot(r10.xyz, r10.xyz);
      r6.z = sqrt(r3.z);
      r8.w = dynamicLights_g[r1.w].radiusInv;
      r6.z = r8.w * r6.z;
      r8.w = dynamicLights_g[r1.w].attenuation;
      r6.z = log2(abs(r6.z));
      r6.z = r8.w * r6.z;
      r6.z = exp2(r6.z);
      r6.z = 1 + -r6.z;
      r6.z = max(0, r6.z);
      r8.w = cmp(0 < r6.z);
      if (r8.w != 0) {
        r8.w = dynamicLights_g[r1.w].translucency;
        r3.z = rsqrt(r3.z);
        r10.xyz = r10.xyz * r3.zzz;
        r3.z = dot(r10.xyz, r6.xyw);
        r3.z = max(r8.w, r3.z);
        r10.x = dynamicLights_g[r1.w].color.x;
        r10.y = dynamicLights_g[r1.w].color.y;
        r10.z = dynamicLights_g[r1.w].color.z;
        r10.xyz = r10.xyz * r6.zzz;
        r9.xyz = r10.xyz * r3.zzz + r9.xyz;
      }
      r9.w = (int)r9.w + 1;
    }
    r7.xyz = r9.xyz * r12.www + r7.xyz;
    r1.y = lightIndices_g[r3.y].spotLightCount;
    r1.y = min(63, (uint)r1.y);
    r9.yw = localShadowResolutionInv_g * float2(1.25,-1.25);
    r9.xz = float2(0,0);
    r10.xyzw = float4(0,0,0,0);
    while (true) {
      r1.w = cmp((uint)r10.w >= (uint)r1.y);
      if (r1.w != 0) break;
      r1.w = lightIndices_g[r3.y].spotLightIndices[r10.w];
      r11.x = dynamicLights_g[r1.w].pos.x;
      r11.y = dynamicLights_g[r1.w].pos.y;
      r11.z = dynamicLights_g[r1.w].pos.z;
      r11.xyz = r11.xyz + -r5.xyz;
      r3.z = dot(r11.xyz, r11.xyz);
      r6.z = rsqrt(r3.z);
      r11.xyz = r11.xyz * r6.zzz;
      r15.x = dynamicLights_g[r1.w].vec.x;
      r15.y = dynamicLights_g[r1.w].vec.y;
      r15.z = dynamicLights_g[r1.w].vec.z;
      r15.w = dynamicLights_g[r1.w].spotAngleInv;
      r6.z = dot(r11.xyz, r15.xyz);
      r6.z = max(0, r6.z);
      r6.z = 1 + -r6.z;
      r6.z = r6.z * r15.w;
      r8.w = dynamicLights_g[r1.w].attenuationAngle;
      r6.z = log2(r6.z);
      r6.z = r8.w * r6.z;
      r6.z = exp2(r6.z);
      r6.z = 1 + -r6.z;
      r6.z = max(0, r6.z);
      r8.w = cmp(0 < r6.z);
      if (r8.w != 0) {
        r3.z = sqrt(r3.z);
        r8.w = dynamicLights_g[r1.w].radiusInv;
        r3.z = r8.w * r3.z;
        r8.w = dynamicLights_g[r1.w].attenuation;
        r3.z = log2(abs(r3.z));
        r3.z = r8.w * r3.z;
        r3.z = exp2(r3.z);
        r3.z = 1 + -r3.z;
        r3.z = max(0, r3.z);
        r3.z = r6.z * r3.z;
        r6.z = cmp(0 < r3.z);
        if (r6.z != 0) {
          r12.x = dynamicLights_g[r1.w].translucency;
          r12.y = dynamicLights_g[r1.w].shadowmapIndex;
          r6.z = cmp((int)r12.y != -1);
          if (r6.z != 0) {
            r15.xyzw = spotShadowMatrices_g[r12.y]._m00_m10_m20_m30;
            r17.xyzw = spotShadowMatrices_g[r12.y]._m01_m11_m21_m31;
            r19.xyzw = spotShadowMatrices_g[r12.y]._m02_m12_m22_m32;
            r20.xyzw = spotShadowMatrices_g[r12.y]._m03_m13_m23_m33;
            r14.x = dot(r5.xyzw, r15.xyzw);
            r14.y = dot(r5.xyzw, r17.xyzw);
            r14.z = dot(r5.xyzw, r19.xyzw);
            r6.z = dot(r5.xyzw, r20.xyzw);
            r15.xyz = r14.xyz / r6.zzz;
            r15.w = (uint)r12.y;
            r6.z = spotShadowMaps.SampleCmpLevelZero(SmplShadow_s, r15.xyw, r15.z).x;
            r14.xy = r15.xy + r9.yx;
            r14.z = r15.w;
            r8.w = spotShadowMaps.SampleCmpLevelZero(SmplShadow_s, r14.xyz, r15.z).x;
            r8.w = 0.200000003 * r8.w;
            r6.z = r6.z * 0.200000003 + r8.w;
            r14.xy = r15.xy + r9.wz;
            r8.w = spotShadowMaps.SampleCmpLevelZero(SmplShadow_s, r14.xyz, r15.z).x;
            r6.z = r8.w * 0.200000003 + r6.z;
            r14.xy = r15.xy + r9.xy;
            r8.w = spotShadowMaps.SampleCmpLevelZero(SmplShadow_s, r14.xyz, r15.z).x;
            r6.z = r8.w * 0.200000003 + r6.z;
            r14.xy = r15.xy + r9.zw;
            r8.w = spotShadowMaps.SampleCmpLevelZero(SmplShadow_s, r14.xyz, r15.z).x;
            r6.z = r8.w * 0.200000003 + r6.z;
            r3.z = r6.z * r3.z;
          }
          r6.z = dot(r11.xyz, r6.xyw);
          r6.z = max(r12.x, r6.z);
          r11.x = dynamicLights_g[r1.w].color.x;
          r11.y = dynamicLights_g[r1.w].color.y;
          r11.z = dynamicLights_g[r1.w].color.z;
          r11.xyz = r11.xyz * r3.zzz;
          r10.xyz = r11.xyz * r6.zzz + r10.xyz;
        }
      }
      r10.w = (int)r10.w + 1;
    }
    r2.xyw = r10.xyz * r12.www + r7.xyz;
  }
  r6.xyz = r8.xyz * r7.www;
  r2.xyw = r18.xyz * r2.xyw + r6.xyz;
  r1.y = cmp(0 < r4.y);
  r0.x = dot(r0.xyz, float3(0.298999995,0.587000012,0.114));
  r0.x = r0.x + -r4.y;
  r0.x = max(0, r0.x);
  r0.x = r0.x / r4.y;
  r0.x = min(1, r0.x);
  r0.x = r1.y ? r0.x : 1;
  r0.y = r14.w * r3.w;
  r0.x = r0.y * r0.x;
  r0.y = -1 + r0.w;
  r0.y = r4.z * r0.y + 1;
  r0.x = r0.x * r0.y;
  r0.y = 1 + -r4.x;
  r0.y = r4.w * r0.y;
  r0.y = mapAOIntensity_g * r0.y;
  r3.yzw = r2.xyw * mapAOColor_g.xyz + -r2.xyw;
  r0.yzw = r0.yyy * r3.yzw + r2.xyw;
  r1.y = -fogNearDistance_g + -r3.x;
  r1.y = saturate(fogFadeRangeInv_g * r1.y);
  r1.w = -fogHeight_g + r5.y;
  r1.w = saturate(fogHeightRangeInv_g * r1.w);
  r1.y = r1.y * r1.w;
  r1.y = fogIntensity_g * r1.y;
  r1.z = (int)r1.z & 16;
  r1.w = -r2.z * 0.0322580636 + 1;
  r1.z = r1.z ? r1.w : 1;
  r1.y = r1.y * r13.x;
  r1.y = r1.y * r1.z;
  r2.xyz = fogColor_g.xyz + -r0.yzw;
  r0.yzw = r1.yyy * r2.xyz + r0.yzw;
  r1.y = -r3.x / skyLutCameraFarClip_g;
  r2.xy = invVPSize_g.xy * v0.xy;
  r1.y = -skyLutNearOverFarClip_g + r1.y;
  r1.z = -skyLutNearOverFarClip_g + 1;
  r2.z = r1.y / r1.z;
  r1.yzw = atmosphereInscatterLUT.SampleLevel(samLinear_s, r2.xyz, 0).xyz;
  r2.xyz = atmosphereExtinctionLUT.SampleLevel(samLinear_s, r2.xyz, 0).xyz;
  r0.yzw = r0.yzw * r2.xyz + r1.yzw;
  if (outlineShapeCount_g != 0) {
    r1.yz = -outlineShapeMaskUVParam_g.xy + r5.xz;
    r1.yz = outlineShapeMaskUVParam_g.zw * r1.yz;
    r2.xy = cmp(r1.yz >= float2(0,0));
    r1.w = r2.y ? r2.x : 0;
    r2.xy = cmp(float2(1,1) >= r1.yz);
    r2.x = r2.y ? r2.x : 0;
    r1.w = r2.x ? r1.w : 0;
    if (r1.w != 0) {
      r1.y = outlineShapeMask.SampleLevel(samLinear_s, r1.yz, 0).x;
    } else {
      r1.y = 1;
    }
    r5.w = 1;
    r2.xyz = r0.yzw;
    r1.z = 0;
    while (true) {
      r1.w = cmp((uint)r1.z >= outlineShapeCount_g);
      if (r1.w != 0) break;
      r3.x = outlineShapes_g[r1.z].gradation_sharpness;
      r3.y = outlineShapes_g[r1.z].height_base;
      r3.z = outlineShapes_g[r1.z].height_width;
      r3.w = outlineShapes_g[r1.z].height_gradation_width;
      r1.w = r5.y + -r3.y;
      r2.w = cmp(r3.z >= abs(r1.w));
      if (r2.w != 0) {
        r4.x = outlineShapes_g[r1.z].mtx._m00;
        r4.y = outlineShapes_g[r1.z].mtx._m10;
        r4.z = outlineShapes_g[r1.z].mtx._m20;
        r4.w = outlineShapes_g[r1.z].mtx._m30;
        r6.x = outlineShapes_g[r1.z].mtx._m02;
        r6.y = outlineShapes_g[r1.z].mtx._m32;
        r6.z = outlineShapes_g[r1.z].mtx._m12;
        r6.w = outlineShapes_g[r1.z].mtx._m22;
        r7.x = outlineShapes_g[r1.z].color.x;
        r7.y = outlineShapes_g[r1.z].color.y;
        r7.z = outlineShapes_g[r1.z].color.z;
        r7.w = outlineShapes_g[r1.z].color.w;
        r8.x = outlineShapes_g[r1.z].type;
        r8.y = outlineShapes_g[r1.z].radius;
        r8.z = outlineShapes_g[r1.z].gradation_size.x;
        r8.w = outlineShapes_g[r1.z].gradation_size.y;
        r2.w = abs(r1.w) + r3.w;
        r2.w = cmp(r3.z < r2.w);
        r1.w = r3.z + -abs(r1.w);
        r1.w = r1.w / r3.w;
        r1.w = log2(r1.w);
        r1.w = 0.25 * r1.w;
        r1.w = exp2(r1.w);
        r1.w = r2.w ? r1.w : 1;
        if (r8.x == 0) {
          r6.x = r4.w;
          r3.yz = -r6.xy + r5.xz;
          r2.w = dot(r3.yz, r3.yz);
          r2.w = sqrt(r2.w);
          r3.y = cmp(r8.y < r2.w);
          r3.z = r8.y + -r8.z;
          r3.w = cmp(r2.w >= r3.z);
          r9.x = ~(int)r3.w;
          r2.w = -r3.z + r2.w;
          r2.w = r2.w / r8.z;
          r2.w = r3.w ? r2.w : 0;
          r2.w = r3.y ? 0 : r2.w;
          r3.y = (int)r3.y | (int)r9.x;
        } else {
          r3.z = cmp((int)r8.x == 1);
          if (r3.z != 0) {
            r3.z = outlineShapes_g[r1.z].fan_angle;
            r6.xz = r4.wz;
            r9.xy = -r6.xy + r5.xz;
            r3.w = dot(r9.xy, r9.xy);
            r9.z = sqrt(r3.w);
            r9.w = cmp(r8.y < r9.z);
            r8.y = r8.y + -r8.z;
            r3.w = rsqrt(r3.w);
            r9.xy = r9.xy * r3.ww;
            r3.w = dot(r6.zw, r9.xy);
            r9.x = 1 + -abs(r3.w);
            r9.x = sqrt(r9.x);
            r9.y = abs(r3.w) * -0.0187292993 + 0.0742610022;
            r9.y = r9.y * abs(r3.w) + -0.212114394;
            r9.y = r9.y * abs(r3.w) + 1.57072878;
            r10.x = r9.y * r9.x;
            r10.x = r10.x * -2 + 3.14159274;
            r3.w = cmp(r3.w < -r3.w);
            r3.w = r3.w ? r10.x : 0;
            r3.w = r9.y * r9.x + r3.w;
            r9.x = cmp(r3.z >= r3.w);
            r9.y = ~(int)r9.x;
            r10.x = 6.28318548 * r9.z;
            r3.zw = r10.xx * r3.zw;
            r3.w = 0.159154937 * r3.w;
            r3.z = r3.z * 0.159154937 + -r3.w;
            r3.z = r3.z / r8.z;
            r3.z = min(1, r3.z);
            r3.z = 1 + -r3.z;
            r3.w = r9.z + -r8.y;
            r3.w = r3.w / r8.z;
            r3.w = min(1, r3.w);
            r3.z = max(r3.z, r3.w);
            r3.z = r9.x ? r3.z : 0;
            r2.w = r9.w ? 0 : r3.z;
            r3.y = (int)r9.w | (int)r9.y;
          } else {
            r3.z = cmp((int)r8.x == 2);
            r4.x = dot(r5.xyzw, r4.xyzw);
            r4.y = dot(r5.xwyz, r6.xyzw);
            r4.zw = cmp(r4.xy < float2(0.5,0.5));
            r3.w = r4.w ? r4.z : 0;
            r4.zw = cmp(float2(-0.5,-0.5) < r4.xy);
            r4.z = r4.w ? r4.z : 0;
            r3.w = r4.z ? r3.w : 0;
            r4.zw = float2(0.5,0.5) + -r8.zw;
            r6.xy = cmp(abs(r4.xy) < r4.zw);
            r6.x = r6.y ? r6.x : 0;
            r4.xy = abs(r4.xy) + -r4.zw;
            r4.xy = r4.xy / r8.zw;
            r4.x = max(r4.x, r4.y);
            r4.x = r6.x ? 0 : r4.x;
            r4.x = (int)r3.w & (int)r4.x;
            r3.w = r3.w ? r6.x : -1;
            r2.w = r3.z ? r4.x : 0;
            r3.y = r3.z ? r3.w : 0;
          }
        }
        r2.w = log2(r2.w);
        r2.w = r3.x * r2.w;
        r2.w = exp2(r2.w);
        r2.w = r7.w * r2.w;
        r2.w = r2.w * r1.y;
        r1.w = r2.w * r1.w;
        r3.xzw = r7.xyz + -r2.xyz;
        r3.xzw = r1.www * r3.xzw + r2.xyz;
        r2.xyz = r3.yyy ? r2.xyz : r3.xzw;
      }
      r1.z = (int)r1.z + 1;
    }
    r0.yzw = r2.xyz;
  }
  r0.x = saturate(0.100000001 * r0.x);
  r2.y = r16.w ? r0.x : 0;
  r2.xzw = float3(255,255,255);
  r2.xyzw = float4(0,255,0,0) * r2.xyzw;
  o1.xyzw = (uint4)r2.xyzw;
  r0.x = 65.5350037 * r1.x;
  r0.x = (uint)r0.x;
  o2.y = min(0x0000ffff, (uint)r0.x);
  o0.xyz = r0.yzw;
  o0.w = 1;
  o2.x = 0;
  return;
}