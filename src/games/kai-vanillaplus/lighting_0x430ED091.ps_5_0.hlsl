// Fixed by Gemini for DLS_0x430ED091.ps_5_0
// Based on assembly reconstruction

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

cbuffer cb_local : register(b2)
{
  float fadeRangeInv_g : packoffset(c0);
  float density_g : packoffset(c0.y);
  float4 offsetsAndWeights[8] : packoffset(c1);
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
SamplerState SmplMirror_s : register(s12);
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
Texture2D<float4> cloudsTexture : register(t8);
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

#include "./kai-vanillaplus.h"

// 3Dmigoto declarations
#define cmp -

// Bend-style screen-space directional shadowing adapted for this deferred PS.
// Uses only resources already bound by this shader.
//
// Screen Space Shadow tuning (in-game):
// - Increase sample count for longer shadow reach and less noise; decrease for lower cost and shorter reach.
// - Increase hard samples for stronger contact shadows; decrease to reduce near-contact aliasing/shimmer.
// - Increase fade-out samples to soften the far-end cutoff; decrease for a sharper tail and slightly darker far shadows.
// - Increase surface thickness if thin casters fail to shadow; decrease if shadows feel too thick or bleed onto neighbors.
// - Increase shadow contrast for darker/crisper transitions; decrease for softer/lighter transitions.
// Reversed-Z far depth value from this game's depth setup; keep unless projection/depth setup changes.
static const float SSS_FAR_DEPTH_VALUE = 0.0;

static float ComputeScreenSpaceShadowBend(float2 uv, float3 normalWS)
{
  if (sss_injection_data.sss_enabled < 0.5) {
    return 1.0;
  }

  int sampleCount = max((int)round(sss_injection_data.sss_sample_count), 1);
  int hardShadowSamples = clamp((int)round(sss_injection_data.sss_hard_shadow_samples), 0, sampleCount);
  int fadeOutSamples = clamp((int)round(sss_injection_data.sss_fade_out_samples), 0, sampleCount);
  if ((hardShadowSamples + fadeOutSamples) > sampleCount) {
    fadeOutSamples = max(sampleCount - hardShadowSamples, 0);
  }

  float surfaceThickness = max(sss_injection_data.sss_surface_thickness, 1e-5);
  float shadowContrast = max(sss_injection_data.sss_shadow_contrast, 0.0);
  float fadeStart = min(sss_injection_data.sss_light_screen_fade_start, sss_injection_data.sss_light_screen_fade_end);
  float fadeEnd = max(sss_injection_data.sss_light_screen_fade_start, sss_injection_data.sss_light_screen_fade_end);
  float minOccluderDepthScale = max(sss_injection_data.sss_min_occluder_depth_scale, 0.0);
  bool useJitter = sss_injection_data.sss_jitter_enabled >= 0.5;

  float startDepth = depthTexture.SampleLevel(samPoint_s, uv, 0).x;
  if (startDepth <= SSS_FAR_DEPTH_VALUE) {
    return 1.0;
  }

  float3 lightToPixel = normalize(lightDirection_g.xyz);
  float3 pixelToLight = -lightToPixel;
  float ndotl = saturate(dot(normalWS, pixelToLight));
  if (ndotl <= 0.001) {
    return 1.0;
  }

  float2 lightViewXY;
  lightViewXY.x = dot(lightDirection_g.xyz, view_g._m00_m10_m20);
  lightViewXY.y = dot(lightDirection_g.xyz, view_g._m01_m11_m21);
  float lightLenSq = dot(lightViewXY, lightViewXY);
  if (lightLenSq <= 1e-6) {
    return 1.0;
  }

  float lightScreenLen = sqrt(lightLenSq);
  float lightScreenFade = saturate((lightScreenLen - fadeStart) /
                                   max(fadeEnd - fadeStart, 1e-5));
  if (lightScreenFade <= 0.001) {
    return 1.0;
  }

  // March toward the light source (opposite projected light direction).
  float2 stepUV = (-lightViewXY * rsqrt(lightLenSq)) * invVPSize_g.xy;

  // Small per-pixel jitter to reduce regular banding.
  float jitter = dot(uv * vpSize_g.xy, float2(0.0671105608, 0.00583714992));
  if (useJitter) {
    jitter += (sceneTime_g * 77.0);
  }
  jitter = frac(jitter);
  jitter = frac(52.9829178 * jitter);
  float2 sampleUV = uv + stepUV * (2.0 + jitter);

  float depthThickness = max(abs(SSS_FAR_DEPTH_VALUE - startDepth) * surfaceThickness, 1e-5);
  float2 receiverDepthSlope = float2(ddx(startDepth), ddy(startDepth));
  float hardShadow = 1.0;
  float4 shadowValue = 1.0;
  int validSamples = 0;

  [loop]
  for (int i = 0; i < sampleCount; ++i) {
    sampleUV += stepUV;

    if (sampleUV.x <= invVPSize_g.x || sampleUV.y <= invVPSize_g.y ||
        sampleUV.x >= (1.0 - invVPSize_g.x) || sampleUV.y >= (1.0 - invVPSize_g.y)) {
      break;
    }

    validSamples++;

    float sampleDepth = depthTexture.SampleLevel(samPoint_s, sampleUV, 0).x;
    float shadowSample = 1.0;

    // Compensate for receiver-plane slope so camera pitch doesn't inflate false occluders.
    float2 sampleOffsetPx = (sampleUV - uv) * vpSize_g.xy;
    float expectedDepth = startDepth + dot(receiverDepthSlope, sampleOffsetPx);

    // Reversed Z: larger depth values are closer to camera.
    float depthDelta = sampleDepth - expectedDepth;
    if (depthDelta > (depthThickness * minOccluderDepthScale)) {
      // Center matching around one "thickness" offset (prevents self-shadow from near-zero deltas).
      float depthMatch = abs(depthDelta / depthThickness - 1.0);
      shadowSample = saturate(depthMatch * shadowContrast + (1.0 - shadowContrast));
    }

    if (i < hardShadowSamples) {
      hardShadow = min(hardShadow, shadowSample);
    } else {
      if (i >= (sampleCount - fadeOutSamples)) {
        float fadeOut = (float)(i + 1 - (sampleCount - fadeOutSamples)) / (float)(fadeOutSamples + 1) * 0.75;
        shadowSample = saturate(shadowSample + fadeOut);
      }
      int bucket = i & 3;
      if (bucket == 0) shadowValue.x = min(shadowValue.x, shadowSample);
      if (bucket == 1) shadowValue.y = min(shadowValue.y, shadowSample);
      if (bucket == 2) shadowValue.z = min(shadowValue.z, shadowSample);
      if (bucket == 3) shadowValue.w = min(shadowValue.w, shadowSample);
    }
  }

  float result = dot(shadowValue, 0.25);
  result = min(result, hardShadow);
  float validSampleFade = saturate((float)validSamples / (float)sampleCount);
  validSampleFade = validSampleFade * validSampleFade;
  return lerp(1.0, result, ndotl * lightScreenFade * validSampleFade);
}

void main(
  float4 v0 : SV_Position0,
  float4 v1 : TEXCOORD0,
  out float4 o0 : SV_Target0,
  out uint4 o1 : SV_Target1)
{
  // Define registers used in assembly
  float4 r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.xyz = colorTexture.SampleLevel(samPoint_s, v1.xy, 0).xyz;
  mrtTexture1.GetDimensions(0, fDest.x, fDest.y, fDest.z);
  r1.xy = fDest.xy;
  r1.xy = v1.xy * r1.xy;
  r1.xy = (int2)r1.xy;
  r1.zw = float2(0,0);
  r1.xyzw = mrtTexture1.Load(r1.xyz).xyzw;
  r2.z = depthTexture.SampleLevel(samPoint_s, v1.xy, 0).x;
  r0.w = cmp(0 >= r2.z);
  if (r0.w != 0) {
    r3.xyzw = cloudsTexture.SampleLevel(samLinear_s, v1.xy, 0).xyzw;
    r3.xyz = r3.xyz + -r0.xyz;
    r3.xyz = r3.www * r3.xyz + r0.xyz;
    o0.xyz = skyBrightness_g * r3.xyz;
    o0.w = 1;
    o1.xyzw = r1.xyzw;
    return;
  }
  mrtTexture0.GetDimensions(0, fDest.x, fDest.y, fDest.z);
  r3.xy = fDest.xy;
  r3.xy = v1.xy * r3.xy;
  r3.xy = (int2)r3.xy;
  r3.zw = float2(0,0);
  r3.xyz = mrtTexture0.Load(r3.xyz).xyz;
  r4.xyz = ssaoTexture.SampleLevel(samLinear_s, v1.xy, 0).xyz;
  r0.w = (uint)r3.z >> 8;
  r3.xy = (uint2)r3.xy;
  r5.zw = r3.xy * float2(3.05180438e-05,3.05180438e-05) + float2(-1,-1);
  r3.x = 3.14159274 * r5.z;
  sincos(r3.x, r3.x, r6.x);
  r3.y = r5.w * r5.w;
  r3.w = -r5.w * r5.w + 1;
  r3.w = sqrt(r3.w);
  r5.x = r6.x * r3.w;
  r5.y = r3.x * r3.w;
  r3.x = (int)r0.w & 8;
  if (r3.x == 0) {
    r6.y = (uint)r1.y >> 8;
    r6.x = r1.y;
    r3.xw = (int2)r6.xy & int2(255,255);
    r3.xw = (uint2)r3.xw;
    r6.x = 0.00392156886;
    r6.y = r0.y;
    r6.xy = r6.xy * r3.xw;
    if (1 == 0) r3.x = 0; else if (1+8 < 32) {     r3.x = (uint)r3.z << (32-(1 + 8));
    r3.x = (uint)r3.x >> (32-1);    } else r3.x = (uint)r3.z >> 8;
    if (r3.x != 0) {
      r3.x = (int)r1.z & 255;
      r3.x = (uint)r3.x;
      r6.z = r3.x * r0.z;
      r7.x = r0.x;
      r7.yz = float2(0.00392156886,0.00392156886);
      r8.xyz = r7.xyz * r6.xyz;
      r3.x = r4.y * r4.z;
      r4.yzw = -r7.xyz * r6.xyz + r0.xyz;
      r4.yzw = r3.xxx * r4.yzw + r8.xyz;
      r3.x = (int)r0.w & 4;
      if (r3.x == 0) {
        r2.xy = v1.xy * float2(2,-2) + float2(-1,1);
        r2.w = 1;
        r3.x = dot(r2.xyzw, projInv_g._m02_m12_m22_m32);
        r3.w = dot(r2.xyzw, projInv_g._m03_m13_m23_m33);
        r3.x = r3.x / r3.w;
        r3.w = saturate(-0.5 * r3.x);
        r3.w = r3.w * 0.00899999961 + 0.00100000005;
        r6.yz = offsetsAndWeights[0].xy + v1.xy;
        r7.z = depthTexture.SampleLevel(samLinear_s, r6.yz, 0).x;
        r7.xy = r6.yz * float2(2,-2) + float2(-1,1);
        r7.w = 1;
        r5.z = dot(r7.xyzw, projInv_g._m02_m12_m22_m32);
        r6.y = dot(r7.xyzw, projInv_g._m03_m13_m23_m33);
        r5.z = r5.z / r6.y;
        r5.z = -r5.z + r3.x;
        r5.z = cmp(r5.z >= r3.w);
        r5.z = r5.z ? 1.000000 : 0;
        r6.yz = offsetsAndWeights[1].xy + v1.xy;
        r7.z = depthTexture.SampleLevel(samLinear_s, r6.yz, 0).x;
        r7.xy = r6.yz * float2(2,-2) + float2(-1,1);
        r7.w = 1;
        r6.y = dot(r7.xyzw, projInv_g._m02_m12_m22_m32);
        r6.z = dot(r7.xyzw, projInv_g._m03_m13_m23_m33);
        r6.y = r6.y / r6.z;
        r6.y = -r6.y + r3.x;
        r6.y = cmp(r6.y >= r3.w);
        r6.y = r6.y ? 1.000000 : 0;
        r6.y = offsetsAndWeights[1].z * r6.y;
        r5.z = r5.z * offsetsAndWeights[0].z + r6.y;
        r6.yz = offsetsAndWeights[2].xy + v1.xy;
        r7.z = depthTexture.SampleLevel(samLinear_s, r6.yz, 0).x;
        r7.xy = r6.yz * float2(2,-2) + float2(-1,1);
        r7.w = 1;
        r6.y = dot(r7.xyzw, projInv_g._m02_m12_m22_m32);
        r6.z = dot(r7.xyzw, projInv_g._m03_m13_m23_m33);
        r6.y = r6.y / r6.z;
        r6.y = -r6.y + r3.x;
        r6.y = cmp(r6.y >= r3.w);
        r6.y = r6.y ? 1.000000 : 0;
        r5.z = r6.y * offsetsAndWeights[2].z + r5.z;
        r6.yz = offsetsAndWeights[3].xy + v1.xy;
        r7.z = depthTexture.SampleLevel(samLinear_s, r6.yz, 0).x;
        r7.xy = r6.yz * float2(2,-2) + float2(-1,1);
        r7.w = 1;
        r6.y = dot(r7.xyzw, projInv_g._m02_m12_m22_m32);
        r6.z = dot(r7.xyzw, projInv_g._m03_m13_m23_m33);
        r6.y = r6.y / r6.z;
        r6.y = -r6.y + r3.x;
        r6.y = cmp(r6.y >= r3.w);
        r6.y = r6.y ? 1.000000 : 0;
        r5.z = r6.y * offsetsAndWeights[3].z + r5.z;
        r6.yz = offsetsAndWeights[4].xy + v1.xy;
        r7.z = depthTexture.SampleLevel(samLinear_s, r6.yz, 0).x;
        r7.xy = r6.yz * float2(2,-2) + float2(-1,1);
        r7.w = 1;
        r6.y = dot(r7.xyzw, projInv_g._m02_m12_m22_m32);
        r6.z = dot(r7.xyzw, projInv_g._m03_m13_m23_m33);
        r6.y = r6.y / r6.z;
        r6.y = -r6.y + r3.x;
        r6.y = cmp(r6.y >= r3.w);
        r6.y = r6.y ? 1.000000 : 0;
        r5.z = r6.y * offsetsAndWeights[4].z + r5.z;
        r6.yz = offsetsAndWeights[5].xy + v1.xy;
        r7.z = depthTexture.SampleLevel(samLinear_s, r6.yz, 0).x;
        r7.xy = r6.yz * float2(2,-2) + float2(-1,1);
        r7.w = 1;
        r6.y = dot(r7.xyzw, projInv_g._m02_m12_m22_m32);
        r6.z = dot(r7.xyzw, projInv_g._m03_m13_m23_m33);
        r6.y = r6.y / r6.z;
        r6.y = -r6.y + r3.x;
        r6.y = cmp(r6.y >= r3.w);
        r6.y = r6.y ? 1.000000 : 0;
        r5.z = r6.y * offsetsAndWeights[5].z + r5.z;
        r6.yz = offsetsAndWeights[6].xy + v1.xy;
        r7.z = depthTexture.SampleLevel(samLinear_s, r6.yz, 0).x;
        r7.xy = r6.yz * float2(2,-2) + float2(-1,1);
        r7.w = 1;
        r6.y = dot(r7.xyzw, projInv_g._m02_m12_m22_m32);
        r6.z = dot(r7.xyzw, projInv_g._m03_m13_m23_m33);
        r6.y = r6.y / r6.z;
        r6.y = -r6.y + r3.x;
        r6.y = cmp(r6.y >= r3.w);
        r6.y = r6.y ? 1.000000 : 0;
        r5.z = r6.y * offsetsAndWeights[6].z + r5.z;
        r6.yz = offsetsAndWeights[7].xy + v1.xy;
        r7.z = depthTexture.SampleLevel(samLinear_s, r6.yz, 0).x;
        r7.xy = r6.yz * float2(2,-2) + float2(-1,1);
        r7.w = 1;
        r6.y = dot(r7.xyzw, projInv_g._m02_m12_m22_m32);
        r6.z = dot(r7.xyzw, projInv_g._m03_m13_m23_m33);
        r6.y = r6.y / r6.z;
        r6.y = -r6.y + r3.x;
        r3.w = cmp(r6.y >= r3.w);
        r3.w = r3.w ? 1.000000 : 0;
        r3.w = r3.w * offsetsAndWeights[7].z + r5.z;
        r7.yzw = (uint3)r1.xxx >> int3(4,8,12);
        r7.x = r1.x;
        r7.xyzw = (int4)r7.xyzw & int4(15,15,15,15);
        r7.xyzw = (uint4)r7.xyzw;
        r5.z = 0.0666666701 * r7.w;
        r6.y = cmp(0 < r7.w);
        r3.w = r3.w * r3.w;
        r3.w = r3.w * r5.z;
        r3.w = density_g * r3.w;
        r3.x = fadeRangeInv_g * -r3.x;
        r3.xw = min(float2(1,1), r3.xw);
        r3.x = 1 + -r3.x;
        r3.x = r3.w * r3.x;
        r8.x = dot(lightDirection_g.xyz, view_g._m00_m10_m20);
        r8.y = dot(lightDirection_g.xyz, view_g._m01_m11_m21);
        r3.w = dot(r8.xy, r8.xy);
        r3.w = rsqrt(r3.w);
        r6.zw = r8.xy * r3.ww;
        r8.x = dot(r5.xyw, view_g._m00_m10_m20);
        r8.y = dot(r5.xyw, view_g._m01_m11_m21);
        r8.z = dot(r5.xyw, view_g._m02_m12_m22);
        r3.w = dot(r8.xyz, r8.xyz);
        r3.w = rsqrt(r3.w);
        r8.xy = r8.xy * r3.ww;
        r3.w = dot(r8.xy, r6.zw);
        r3.w = r3.w * 0.5 + 0.5;
        r3.w = log2(abs(r3.w));
        r3.w = 0.400000006 * r3.w;
        r3.w = exp2(r3.w);
        r3.x = r3.x * r3.w;
        r7.xyz = r7.xyz * float3(0.0666666701,0.0666666701,0.0666666701) + -r4.yzw;
        r7.xyz = r3.xxx * r7.xyz + r4.yzw;
        r4.yzw = r6.yyy ? r7.xyz : r4.yzw;
      }
    } else {
      r3.x = 1 + -r4.x;
      r3.x = r6.x * r3.x;
      r6.xyz = r0.xyz * mapAOColor_g.xyz + -r0.xyz;
      r4.yzw = r3.xxx * r6.xyz + r0.xyz;
    }
    o0.xyz = r4.yzw;
    o0.w = 1;
    o1.xyzw = r1.xyzw;
    return;
  }
  r3.x = (int)r3.z & 255;
  r3.x = (uint)r3.x;
  r3.z = 0.00392156886 * r3.x;
  r6.xyw = (uint3)r1.yzw >> int3(8,8,8);
  r6.z = r1.y;
  r4.yz = (int2)r6.zx & int2(255,255);
  r4.yz = (uint2)r4.yz;
  r4.yz = float2(0.00392156886,0.00392156886) * r4.yz;
  r6.xz = r1.zw;
  r6.xyzw = (int4)r6.xyzw & int4(255,255,255,255);
  r6.xyzw = (uint4)r6.xyzw;
  r7.xyzw = float4(0.00392156886,0.00392156886,0.00392156886,0.00392156886) * r6.yxzw;
  r1.x = min(0x00004e1f, (uint)r1.x);
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
  r1.y = deferredParams_g[r1.x].glowLumThreshold;
  r1.z = deferredParams_g[r1.x].glowShadowFadeRatio;
  r1.w = deferredParams_g[r1.x].ssaoIntensity;
  r3.w = deferredParams_g[r1.x].ssrDistance;
  r1.x = deferredParams_g[r1.x].flag;
  r11.w = r12.x;
  r15.xz = r11.yz;
  r15.yw = r12.yz;
  r6.xz = r15.xy + -r11.xw;
  r6.xz = r7.zz * r6.xz + r11.xw;
  r11.xy = r15.zw + -r6.xz;
  r6.xz = r7.ww * r11.xy + r6.xz;
  r2.xy = v1.zw * float2(2,-2) + float2(-1,1);
  r2.w = 1;
  r11.x = dot(r2.xyzw, viewProjInv_g._m00_m10_m20_m30);
  r11.y = dot(r2.xyzw, viewProjInv_g._m01_m11_m21_m31);
  r11.z = dot(r2.xyzw, viewProjInv_g._m02_m12_m22_m32);
  r11.w = dot(r2.xyzw, viewProjInv_g._m03_m13_m23_m33);
  r2.xyzw = r11.xyzw / r11.wwww;
  r4.w = dot(view_g._m02_m12_m22_m32, r2.xyzw);
  r7.zw = lightTileSizeInv_g.xy * v0.xy;
  r7.zw = (uint2)r7.zw;
  r5.z = (uint)r7.w << 5;
  r6.w = (int)r7.z + (int)r5.z;
  r6.w = lightIndices_g[r6.w].tileDepthInv;
  r6.w = r6.w * -r4.w;
  r6.w = min(7, r6.w);
  r6.w = max(0, r6.w);
  r6.w = (uint)r6.w;
  r5.z = mad((int)r6.w, 576, (int)r5.z);
  r5.z = (int)r7.z + (int)r5.z;
  r5.z = min(4607, (uint)r5.z);
  r6.w = lightIndices_g[r5.z].lightProbeCount;
  r6.w = min(14, (uint)r6.w);
  r3.y = r3.y * 3 + -1;
  r7.z = r5.y * r5.y;
  r7.z = r5.x * r5.x + -r7.z;
  r11.xyz = float3(0,0,0);
  r15.w = 0;
  r7.w = 0;
  while (true) {
    r11.w = cmp((uint)r7.w >= (uint)r6.w);
    if (r11.w != 0) break;
    
    // Fixed dynamic lookup:
    int probeIdx = lightIndices_g[r5.z].lightProbeIndices[r7.w];
    
    r12.x = localLightProbes_g[probeIdx].pos.x;
    r12.y = localLightProbes_g[probeIdx].pos.y;
    r12.z = localLightProbes_g[probeIdx].pos.z;
    r12.xyz = r12.xyz + -r2.xyz;
    r12.x = dot(r12.xyz, r12.xyz);
    r12.x = sqrt(r12.x);
    r16.x = localLightProbes_g[probeIdx].radiusInv;
    r16.y = localLightProbes_g[probeIdx].attenuation;
    r16.z = localLightProbes_g[probeIdx].intensity;
    r12.x = r16.x * r12.x;
    r12.x = log2(abs(r12.x));
    r12.x = r16.y * r12.x;
    r12.x = exp2(r12.x);
    r12.x = 1 + -r12.x;
    r12.x = max(0, r12.x);
    r12.y = r12.x * r16.z;
    r12.z = cmp(0 >= r12.y);
    if (r12.z != 0) {
      r12.z = (int)r7.w + 1;
      r7.w = r12.z;
      continue;
    }
    r16.x = localLightProbes_g[probeIdx].sh[0].x;
    r16.y = localLightProbes_g[probeIdx].sh[0].y;
    r16.w = localLightProbes_g[probeIdx].sh[0].z;
    r17.x = localLightProbes_g[probeIdx].sh[1].x;
    r17.y = localLightProbes_g[probeIdx].sh[1].y;
    r17.z = localLightProbes_g[probeIdx].sh[1].z;
    r16.xyw = r17.xyz * r5.xxx + r16.xyw;
    r17.x = localLightProbes_g[probeIdx].sh[2].x;
    r17.y = localLightProbes_g[probeIdx].sh[2].y;
    r17.z = localLightProbes_g[probeIdx].sh[2].z;
    r16.xyw = r17.xyz * r5.yyy + r16.xyw;
    r17.x = localLightProbes_g[probeIdx].sh[3].x;
    r17.y = localLightProbes_g[probeIdx].sh[3].y;
    r17.z = localLightProbes_g[probeIdx].sh[3].z;
    r16.xyw = r17.xyz * r5.www + r16.xyw;
    r17.x = localLightProbes_g[probeIdx].sh[4].x;
    r17.y = localLightProbes_g[probeIdx].sh[4].y;
    r17.z = localLightProbes_g[probeIdx].sh[4].z;
    r17.xyz = r17.xyz * r5.www;
    r16.xyw = r17.xyz * r5.xxx + r16.xyw;
    r17.x = localLightProbes_g[probeIdx].sh[5].x;
    r17.y = localLightProbes_g[probeIdx].sh[5].y;
    r17.z = localLightProbes_g[probeIdx].sh[5].z;
    r17.xyz = r17.xyz * r5.yyy;
    r16.xyw = r17.xyz * r5.www + r16.xyw;
    r17.x = localLightProbes_g[probeIdx].sh[6].x;
    r17.y = localLightProbes_g[probeIdx].sh[6].y;
    r17.z = localLightProbes_g[probeIdx].sh[6].z;
    r17.xyz = r17.xyz * r5.yyy;
    r16.xyw = r17.xyz * r5.xxx + r16.xyw;
    r17.x = localLightProbes_g[probeIdx].sh[7].x;
    r17.y = localLightProbes_g[probeIdx].sh[7].y;
    r17.z = localLightProbes_g[probeIdx].sh[7].z;
    r16.xyw = r17.xyz * r3.yyy + r16.xyw;
    r17.x = localLightProbes_g[probeIdx].sh[8].x;
    r17.y = localLightProbes_g[probeIdx].sh[8].y;
    r17.z = localLightProbes_g[probeIdx].sh[8].z;
    r16.xyw = r17.xyz * r7.zzz + r16.xyw;
    r11.xyz = r16.xyw * r12.yyy + r11.xyz;
    r15.w = r12.x * r16.z + r15.w;
    r7.w = (int)r7.w + 1;
  }
  r6.w = cmp(r15.w == 0.000000);
  r6.w = r6.w ? 1.000000 : 0;
  r6.w = r15.w + r6.w;
  r11.xyz = r11.xyz / r6.www;
  r15.w = saturate(r15.w);
  r6.w = 1 + -r15.w;
  r15.xyz = r15.www * r11.xyz;
  r11.xyzw = max(float4(0,0,0,0), r15.xyzw);
  r7.w = cmp(0 < r6.w);
  r12.xyz = lightProbe_g[1].xyz * r5.xxx + lightProbe_g[0].xyz;
  r12.xyz = lightProbe_g[2].xyz * r5.yyy + r12.xyz;
  r12.xyz = lightProbe_g[3].xyz * r5.www + r12.xyz;
  r15.xyz = lightProbe_g[4].xyz * r5.www;
  r12.xyz = r15.xyz * r5.xxx + r12.xyz;
  r15.xyz = lightProbe_g[5].xyz * r5.yyy;
  r12.xyz = r15.xyz * r5.www + r12.xyz;
  r15.xyz = lightProbe_g[6].xyz * r5.yyy;
  r12.xyz = r15.xyz * r5.xxx + r12.xyz;
  r12.xyz = lightProbe_g[7].xyz * r3.yyy + r12.xyz;
  r12.xyz = lightProbe_g[8].xyz * r7.zzz + r12.xyz;
  r12.xyz = r12.xyz * r6.www;
  r12.xyz = r7.www ? r12.xyz : 0;
  r12.xyz = max(float3(0,0,0), r12.xyz);
  r12.xyz = r12.xyz + r11.xyz;
  r15.xyzw = ssgiTexture.SampleLevel(samLinear_s, v1.zw, 0).xyzw;
  bool ssgi_enabled = sss_injection_data.ssgi_mod_enabled >= 0.5;
  bool shadow_use_jitter = sss_injection_data.shadow_pcss_jitter_enabled >= 0.5;
  int pcss_sample_count = 32;
  int pcss_sample_count_minus_one = max(pcss_sample_count - 1, 0);
  float pcss_sample_inv = rcp((float)pcss_sample_count);
  float pcss_blocker_radius_scale = rsqrt((float)max(pcss_sample_count_minus_one, 1));
  float pcss_filter_radius_scale = rsqrt((float)pcss_sample_count);
  float ssgi_color_boost = max(sss_injection_data.ssgi_color_boost, 0.0);
  float ssgi_alpha_boost = max(sss_injection_data.ssgi_alpha_boost, 0.0);
  float ssgi_pow = max(sss_injection_data.ssgi_pow, 0.01);
  if (!ssgi_enabled) {
    // Fully disable SSGI contribution (not just the custom tuning).
    r15.xyzw = float4(0, 0, 0, 0);
  } else {
    // Increase the weight/alpha of the SSGI.
    r15.xyz = r15.xyz * ssgi_color_boost;
    r15.w = saturate(r15.w * ssgi_alpha_boost);
    // Shape dark-vs-bright bounced light response.
    r15.xyz = pow(abs(r15.xyz), ssgi_pow);
  }
  r16.x = viewInv_g._m30;
  r16.y = viewInv_g._m31;
  r16.z = viewInv_g._m32;
  r17.xyz = r16.xyz + -r2.xyz;
  r3.y = dot(r17.xyz, r17.xyz);
  r3.y = rsqrt(r3.y);
  r18.xyz = r17.xyz * r3.yyy;
  r6.w = r8.w * r3.z;
  r7.z = dot(r5.xyw, r18.xyz);
  bool sss_directional_light_active = (((uint)r1.x & 1u) != 0u);
  r19.xyzw = (int4)r1.xxxx & int4(1,2,4,16);
  if (r19.x != 0) {
    r16.xyz = -r16.xyz + r2.xyz;
    r7.w = dot(r16.xyz, r16.xyz);
    r7.w = sqrt(r7.w);
    r8.w = shadowSplitDistance_g.y + -5;
    r8.w = cmp(r8.w < r7.w);
    if (r8.w != 0) {
      r16.x = dot(r2.xyzw, shadowMtx_g[2]._m00_m10_m20_m30);
      r16.y = dot(r2.xyzw, shadowMtx_g[2]._m01_m11_m21_m31);
      r16.z = dot(r2.xyzw, shadowMtx_g[2]._m02_m12_m22_m32);
      r8.w = dot(r2.xyzw, shadowMtx_g[2]._m03_m13_m23_m33);
      r16.xyz = r16.xyz / r8.www;
      r20.xy = cmp(r16.xy < float2(0,0));
      r20.zw = cmp(float2(1,1) < r16.xy);
      r8.w = (int)r20.z | (int)r20.x;
      r8.w = (int)r20.y | (int)r8.w;
      r8.w = (int)r20.w | (int)r8.w;
      if (r8.w != 0) {
        r8.w = 1;
      } else {
        r17.w = 30 / shadowSplitDistance_g.y;
        r20.xy = float2(0.00124999997,0.000624999986) * r17.ww;
        r17.w = -6 + r16.z;
        r20.zw = r20.xy * r17.ww;
        r20.zw = r20.zw / r16.zz;
        r16.w = 2;
        r21.y = shadowMaps.SampleLevel(SmplMirror_s, r16.xyw, 0).x;
        r16.w = cmp(r21.y < r16.z);
        r21.x = 1;
        r21.xy = r16.ww ? r21.xy : 0;
        if (shadow_use_jitter) {
          // add jitter to shadow filtering
          r16.w = dot(v0.xy, float2(0.0671105608,0.00583714992))+ (sceneTime_g * 77.0);
        } else {
          r16.w = dot(v0.xy, float2(0.0671105608,0.00583714992));
        }
        r16.w = frac(r16.w);
        r16.w = 52.9829178 * r16.w;
        r16.w = frac(r16.w);
        r16.w = 6.28318548 * r16.w;
        r22.z = 1;
        r21.zw = r21.xy;
        r17.w = 0;
        while (true) {
          r18.w = cmp((int)r17.w >= pcss_sample_count_minus_one);
          if (r18.w != 0) break;
          r18.w = (int)r17.w;
          r19.x = 0.5 + r18.w;
          r19.x = sqrt(r19.x);
          r19.x = pcss_blocker_radius_scale * r19.x;
          r18.w = r18.w * 2.4000001 + r16.w;
          sincos(r18.w, r23.x, r24.x);
          r24.x = r24.x * r19.x;
          r24.y = r23.x * r19.x;
          r22.xy = r24.xy * r20.zw + r16.xy;
          r18.w = shadowMaps.SampleLevel(SmplMirror_s, r22.xyz, 0).x;
          r19.x = cmp(r18.w < r16.z);
          r22.y = r21.w + r18.w;
          r22.x = 1 + r21.z;
          r21.zw = r19.xx ? r22.xy : r21.zw;
          r17.w = (int)r17.w + 1;
        }
        r17.w = cmp(r21.z >= 1);
        if (r17.w != 0) {
          r17.w = r21.w / r21.z;
          r17.w = -r17.w + r16.z;
          r17.w = min(0.0500000007, r17.w);
		  // add base softness
          r17.w = (60.0 * r17.w) + 0.2;
          r20.xy = r17.ww * r20.xy;
          
          // Fixed GetDimensions
          float3 dims; float numMips;
          shadowMaps.GetDimensions(0, dims.x, dims.y, dims.z, numMips);
          r20.zw = float2(dims.x, dims.y);
          
          r20.zw = float2(1,1) / r20.zw;
          r20.xy = max(r20.zw, r20.xy);
          r21.z = 2;
          r17.w = 0;
          r18.w = 0;
          while (true) {
            r19.x = cmp((int)r18.w >= pcss_sample_count);
            if (r19.x != 0) break;
            r19.x = (int)r18.w;
            r20.z = 0.5 + r19.x;
            r20.z = sqrt(r20.z);
            r20.z = pcss_filter_radius_scale * r20.z;
            r19.x = r19.x * 2.4000001 + r16.w;
            sincos(r19.x, r19.x, r22.x);
            r22.x = r22.x * r20.z;
            r22.y = r20.z * r19.x;
            r21.xy = r22.xy * r20.xy + r16.xy;
            r19.x = shadowMaps.SampleCmpLevelZero(SmplShadow_s, r21.xyz, r16.z).x;
            r17.w = r19.x + r17.w;
            r18.w = (int)r18.w + 1;
          }
          r8.w = pcss_sample_inv * r17.w;
        } else {
          r8.w = 1;
        }
      }
      r16.x = cmp(r7.w < shadowSplitDistance_g.y);
      if (r16.x != 0) {
        r16.x = dot(r2.xyzw, shadowMtx_g[1]._m00_m10_m20_m30);
        r16.y = dot(r2.xyzw, shadowMtx_g[1]._m01_m11_m21_m31);
        r16.z = dot(r2.xyzw, shadowMtx_g[1]._m02_m12_m22_m32);
        r16.w = dot(r2.xyzw, shadowMtx_g[1]._m03_m13_m23_m33);
        r16.xyz = r16.xyz / r16.www;
        r17.w = 30 / shadowSplitDistance_g.y;
        r20.xy = float2(0.00124999997,0.000624999986) * r17.ww;
        r17.w = -6 + r16.z;
        r20.zw = r20.xy * r17.ww;
        r20.zw = r20.zw / r16.zz;
        r16.w = 1;
        r21.y = shadowMaps.SampleLevel(SmplMirror_s, r16.xyw, 0).x;
        r16.w = cmp(r21.y < r16.z);
        r21.x = 1;
        r21.xy = r16.ww ? r21.xy : 0;
        if (shadow_use_jitter) {
          // add jitter to shadow filtering
          r16.w = dot(v0.xy, float2(0.0671105608,0.00583714992))+ (sceneTime_g * 77.0);
        } else {
          r16.w = dot(v0.xy, float2(0.0671105608,0.00583714992));
        }
      r16.w = frac(r16.w);
      r16.w = 52.9829178 * r16.w;
      r16.w = frac(r16.w);
      r16.w = 6.28318548 * r16.w;
      r22.z = r20.w;
      r23.xy = r21.zw;
      r17.w = 0;
        while (true) {
          r18.w = cmp((int)r17.w >= pcss_sample_count_minus_one);
          if (r18.w != 0) break;
          r18.w = (int)r17.w;
          r19.x = 0.5 + r18.w;
          r19.x = sqrt(r19.x);
          r19.x = pcss_blocker_radius_scale * r19.x;
          r18.w = r18.w * 2.4000001 + r16.w;
          sincos(r18.w, r23.x, r24.x);
          r24.x = r24.x * r19.x;
          r24.y = r23.x * r19.x;
          r22.xy = r24.xy * r20.zw + r16.xy;
          r18.w = shadowMaps.SampleLevel(SmplMirror_s, r22.xyz, 0).x;
          r19.x = cmp(r18.w < r16.z);
          r22.y = r21.w + r18.w;
          r22.x = 1 + r21.z;
          r21.zw = r19.xx ? r22.xy : r21.zw;
          r17.w = (int)r17.w + 1;
        }
        r17.w = cmp(r21.z >= 1);
        if (r17.w != 0) {
          r17.w = r21.w / r21.z;
          r17.w = -r17.w + r16.z;
          r17.w = min(0.0500000007, r17.w);
		  // add base softness
          r17.w = (60.0 * r17.w) + 0.2;
          r20.xy = r17.ww * r20.xy;
          
          // Fixed GetDimensions
          float3 dims; float numMips;
          shadowMaps.GetDimensions(0, dims.x, dims.y, dims.z, numMips);
          r20.zw = float2(dims.x, dims.y);
          
          r20.zw = float2(1,1) / r20.zw;
          r20.xy = max(r20.zw, r20.xy);
          r21.z = 1;
          r17.w = 0;
          r18.w = 0;
          while (true) {
            r19.x = cmp((int)r18.w >= pcss_sample_count);
            if (r19.x != 0) break;
            r19.x = (int)r18.w;
            r20.z = 0.5 + r19.x;
            r20.z = sqrt(r20.z);
            r20.z = pcss_filter_radius_scale * r20.z;
            r19.x = r19.x * 2.4000001 + r16.w;
            sincos(r19.x, r19.x, r22.x);
            r22.x = r22.x * r20.z;
            r22.y = r20.z * r19.x;
            r21.xy = r22.xy * r20.xy + r16.xy;
            r19.x = shadowMaps.SampleCmpLevelZero(SmplShadow_s, r21.xyz, r16.z).x;
            r17.w = r19.x + r17.w;
            r18.w = (int)r18.w + 1;
          }
          r16.x = pcss_sample_inv * r17.w;
        } else {
          r16.x = 1;
        }
        r16.y = shadowSplitDistance_g.y + -r7.w;
        r16.y = 0.200000003 * r16.y;
        r16.x = r16.x + -r8.w;
        r8.w = r16.y * r16.x + r8.w;
      }
      r16.x = -shadowFadeNear_g + r7.w;
      r16.x = saturate(shadowFadeRangeInv_g * r16.x);
      r16.y = 1 + -r8.w;
      r8.w = r16.x * r16.y + r8.w;
    } else {
      r16.x = cmp(r7.w < shadowSplitDistance_g.x);
      r20.xw = r16.xx ? float2(0,0) : float2(4,1);
      r21.x = dot(r2.xyzw, shadowMtx_g[r20.x/4]._m00_m10_m20_m30);
      r21.y = dot(r2.xyzw, shadowMtx_g[r20.x/4]._m01_m11_m21_m31);
      r21.z = dot(r2.xyzw, shadowMtx_g[r20.x/4]._m02_m12_m22_m32);
      r16.y = dot(r2.xyzw, shadowMtx_g[r20.x/4]._m03_m13_m23_m33);
      r20.xyz = r21.xyz / r16.yyy;
      r16.y = 30 / shadowSplitDistance_g.x;
      r16.yz = float2(0.00124999997,0.000624999986) * r16.yy;
      r16.w = -6 + r20.z;
      r21.xy = r16.yz * r16.ww;
      r21.xy = r21.xy / r20.zz;
      r21.w = shadowMaps.SampleLevel(SmplMirror_s, r20.xyw, 0).x;
      r16.w = cmp(r21.w < r20.z);
      r21.z = 1;
      r21.zw = r16.ww ? r21.zw : 0;
      if (shadow_use_jitter) {
        // add jitter to shadow filtering
        r16.w = dot(v0.xy, float2(0.0671105608,0.00583714992)) + (sceneTime_g * 77.0);
      } else {
        r16.w = dot(v0.xy, float2(0.0671105608,0.00583714992));
      }
      r16.w = frac(r16.w);
      r16.w = 52.9829178 * r16.w;
      r16.w = frac(r16.w);
      r16.w = 6.28318548 * r16.w;
      r22.z = r20.w;
      r17.w = 0;
      r18.w = 0;
      while (true) {
        r18.w = cmp((int)r17.w >= pcss_sample_count_minus_one);
        if (r18.w != 0) break;
        r18.w = (int)r17.w;
        r19.x = 0.5 + r18.w;
        r19.x = sqrt(r19.x);
        r19.x = pcss_blocker_radius_scale * r19.x;
        r18.w = r18.w * 2.4000001 + r16.w;
        sincos(r18.w, r24.x, r25.x);
        r25.x = r25.x * r19.x;
        r25.y = r24.x * r19.x;
        r22.xy = r25.xy * r21.xy + r20.xy;
        r18.w = shadowMaps.SampleLevel(SmplMirror_s, r22.xyz, 0).x;
        r19.x = cmp(r18.w < r20.z);
        r22.y = r23.y + r18.w;
        r22.x = 1 + r23.x;
        r23.xy = r19.xx ? r22.xy : r23.xy;
        r17.w = (int)r17.w + 1;
      }
      r16.w = cmp(r23.x >= 1);
      if (r16.w != 0) {
        r16.w = r23.y / r23.x;
        r16.w = r20.z + -r16.w;
        r16.w = min(0.0500000007, r16.w);
		// add base softness
        r16.w = (60.0 * r16.w) + 0.2;
        r21.xy = r16.ww * r16.yz;
        
        // Fixed GetDimensions
        float3 dims; float numMips;
        shadowMaps.GetDimensions(0, dims.x, dims.y, dims.z, numMips);
        r21.zw = float2(dims.x, dims.y);
        
        r21.zw = float2(1,1) / r21.zw;
        r21.xy = max(r21.zw, r21.xy);
        if (shadow_use_jitter) {
          // add jitter to shadow filtering
          r16.w = dot(v0.xy, float2(0.0671105608, 0.00583714992)) + (sceneTime_g * 77.0);
        } else {
          r16.w = dot(v0.xy, float2(0.0671105608,0.00583714992));
        }
        r16.w = frac(r16.w);
        r16.w = 52.9829178 * r16.w;
        r16.w = frac(r16.w);
        r16.w = 6.28318548 * r16.w;
        r22.z = r20.w;
        r17.w = 0;
        r18.w = 0;
        while (true) {
            r19.x = cmp((int)r18.w >= pcss_sample_count);
          if (r19.x != 0) break;
          r19.x = (int)r18.w;
          r20.w = 0.5 + r19.x;
          r20.w = sqrt(r20.w);
          r20.w = pcss_filter_radius_scale * r20.w;
          r19.x = r19.x * 2.4000001 + r16.w;
          sincos(r19.x, r19.x, r23.x);
          r23.x = r23.x * r20.w;
          r23.y = r20.w * r19.x;
          r22.xy = r23.xy * r21.xy + r20.xy;
          r19.x = shadowMaps.SampleCmpLevelZero(SmplShadow_s, r22.xyz, r20.z).x;
          r17.w = r19.x + r17.w;
          r18.w = (int)r18.w + 1;
        }
        r8.w = pcss_sample_inv * r17.w;
      } else {
        r8.w = 1;
      }
      r16.w = shadowSplitDistance_g.x + -5;
      r16.w = cmp(r16.w < r7.w);
      r16.x = r16.w ? r16.x : 0;
      if (r16.x != 0) {
        r20.x = dot(r2.xyzw, shadowMtx_g[1]._m00_m10_m20_m30);
        r20.y = dot(r2.xyzw, shadowMtx_g[1]._m01_m11_m21_m31);
        r20.z = dot(r2.xyzw, shadowMtx_g[1]._m02_m12_m22_m32);
        r16.x = dot(r2.xyzw, shadowMtx_g[1]._m03_m13_m23_m33);
        r20.xyz = r20.xyz / r16.xxx;
        r16.x = -6 + r20.z;
        r16.xw = r16.yz * r16.xx;
        r16.xw = r16.xw / r20.zz;
        r20.w = 1;
        r21.y = shadowMaps.SampleLevel(SmplMirror_s, r20.xyw, 0).x;
        r17.w = cmp(r21.y < r20.z);
        r21.x = 1;
        r21.xy = r17.ww ? r21.xy : 0;
        if (shadow_use_jitter) {
          // add jitter to shadow filtering
          r17.w = dot(v0.xy, float2(0.0671105608,0.00583714992));
          r17.w = frac(r17.w);
          r17.w = 52.9829178 * r17.w;
          r17.w = frac(r17.w);
          r17.w = 6.28318548 * r17.w;
        } else {
          r17.w = 0;
        }
        r22.z = 1;
        r21.zw = r21.xy;
        r18.w = 0;
        while (true) {
          r19.x = cmp((int)r18.w >= pcss_sample_count_minus_one);
          if (r19.x != 0) break;
          r19.x = (int)r18.w;
          r20.w = 0.5 + r19.x;
          r20.w = sqrt(r20.w);
          r20.w = pcss_blocker_radius_scale * r20.w;
          r19.x = r19.x * 2.4000001 + r17.w;
          sincos(r19.x, r19.x, r23.x);
          r23.x = r23.x * r20.w;
          r23.y = r20.w * r19.x;
          r22.xy = r23.xy * r16.xw + r20.xy;
          r19.x = shadowMaps.SampleLevel(SmplMirror_s, r22.xyz, 0).x;
          r20.w = cmp(r19.x < r20.z);
          r22.y = r21.w + r19.x;
          r22.x = 1 + r21.z;
          r21.zw = r20.ww ? r22.xy : r21.zw;
          r18.w = (int)r18.w + 1;
        }
        r16.x = cmp(r21.z >= 1);
        if (r16.x != 0) {
          r16.x = r21.w / r21.z;
          r16.x = r20.z + -r16.x;
          r16.x = min(0.0500000007, r16.x);
		  // add base softness
          r16.x = (60.0 * r16.x) + 0.2;
          r16.xy = r16.xx * r16.yz;
          
          // Fixed GetDimensions
          float3 dims; float numMips;
          shadowMaps.GetDimensions(0, dims.x, dims.y, dims.z, numMips);
          r16.zw = float2(dims.x, dims.y);
          
          r16.zw = float2(1,1) / r16.zw;
          r16.xy = max(r16.zw, r16.xy);
          r21.z = 1;
          r16.zw = float2(0,0);
          while (true) {
            r18.w = cmp((int)r16.w >= pcss_sample_count);
            if (r18.w != 0) break;
            r18.w = (int)r16.w;
            r19.x = 0.5 + r18.w;
            r19.x = sqrt(r19.x);
            r19.x = pcss_filter_radius_scale * r19.x;
            r18.w = r18.w * 2.4000001 + r17.w;
            sincos(r18.w, r22.x, r23.x);
            r23.x = r23.x * r19.x;
            r23.y = r22.x * r19.x;
            r21.xy = r23.xy * r16.xy + r20.xy;
            r18.w = shadowMaps.SampleCmpLevelZero(SmplShadow_s, r21.xyz, r20.z).x;
            r16.z = r18.w + r16.z;
            r16.w = (int)r16.w + 1;
          }
          r16.x = pcss_sample_inv * r16.z;
        } else {
          r16.x = 1;
        }
        r7.w = shadowSplitDistance_g.x + -r7.w;
        r7.w = 0.200000003 * r7.w;
        r16.y = -r16.x + r8.w;
        r8.w = r7.w * r16.y + r16.x;
      }
    }
  } else {
    r8.w = 1;
  }
  if (sss_directional_light_active) {
    r8.w = r8.w * ComputeScreenSpaceShadowBend(v1.xy, r5.xyw);
  }
  r16.xyz = r17.xyz * r3.yyy + -lightDirection_g.xyz;
  r7.w = dot(r16.xyz, r16.xyz);
  r7.w = rsqrt(r7.w);
  r16.xyz = r16.xyz * r7.www;
  r7.w = lightSpecularGlossiness_g * r6.z;
  r16.x = saturate(dot(r16.xyz, r5.xyw));
  r7.w = max(0.00100000005, r7.w);
  r16.x = log2(r16.x);
  r7.w = r16.x * r7.w;
  r7.w = exp2(r7.w);
  r7.w = r7.w * r8.w;
  r7.w = lightSpecularIntensity_g * r7.w;
  r7.w = r19.y ? r7.w : 0;
  r9.xyz = r7.www * r9.xyz;
  r9.xyz = lightColor_g.xyz * r9.xyz;
  if (r19.z != 0) {
    r16.xy = r13.yz * r4.yz;
    r4.y = (int)r1.x & 32;
    if (r4.y != 0) {
      r20.y = resolutionScaling_g.y + -v1.y;
      r20.x = v1.x;
      r20.xyz = texMirror_g.SampleLevel(SmplLinearClamp_s, r20.xy, 0).xyz;
    } else {
      r7.w = r7.z + r7.z;
      r21.xyz = r5.xyw * -r7.www + r18.xyz;
      
      // Fixed GetDimensions
      uint w, h, levels;
      texEnvMap_g.GetDimensions(0, w, h, levels);
      r7.w = levels;
      
      r21.xyz = float3(1,-1,-1) * r21.xyz;
      r7.w = (int)r7.w + -1;
      r7.w = (uint)r7.w;
      r7.w = r16.y * r7.w;
	  // cube
      r20.xyz = texEnvMap_g.SampleLevel(SmplCube_s, r21.xyz, r7.w * 2.0).xyz;
    }
    r7.w = cmp(0 < r6.x);
    r13.y = 1 + -abs(r7.z);
    r13.y = max(0, r13.y);
    r13.y = log2(r13.y);
    r6.x = r13.y * r6.x;
    r6.x = exp2(r6.x);
    r6.x = r7.w ? r6.x : 1;
    r7.w = r16.x * r6.x;
    r21.xyz = r0.xyz * r20.xyz + -r0.xyz;
    r21.xyz = r7.www * r21.xyz + r0.xyz;
    r7.w = dot(r20.xyz, float3(0.298999995,0.587000012,0.114));
    r13.y = r16.y * -9 + 10;
    r7.w = log2(r7.w);
    r7.w = r13.y * r7.w;
    r7.w = exp2(r7.w);
    r13.y = 1 + -r7.w;
    r7.w = r16.x * r13.y + r7.w;
    r16.xyz = r20.xyz * r7.www;
    r16.xyz = r16.xyz * r6.xxx;
    r6.x = -r4.z * r13.z + 1;
    r16.xyz = r16.xyz * r6.xxx + r9.xyz;
    r9.xyz = r4.yyy ? r9.xyz : r16.xyz;
  } else {
    r4.y = (int)r1.x & 8;
    if (r4.y != 0) {
      r4.y = r7.z + r7.z;
      r16.xyz = r5.xyw * -r4.yyy + r18.xyz;
      r4.y = 1 / r13.w;
      r6.x = dot(-r18.xyz, r5.xyw);
      r7.w = r4.y * r4.y;
      r13.y = -r6.x * r6.x + 1;
      r7.w = -r7.w * r13.y + 1;
      r13.y = sqrt(r7.w);
      r6.x = r4.y * r6.x + r13.y;
      r7.w = cmp(r7.w >= 0);
      r20.xyz = r6.xxx * r5.xyw;
      r18.xyz = r4.yyy * -r18.xyz + -r20.xyz;
      r18.xyz = r7.www ? r18.xyz : 0;
      r4.y = r13.z * r4.z;
      
      // Fixed GetDimensions
      uint w, h, levels;
      texEnvMap_g.GetDimensions(0, w, h, levels);
      r6.x = levels;
      
      r16.xyz = float3(1,-1,-1) * r16.xyz;
      r6.x = (int)r6.x + -1;
      r6.x = (uint)r6.x;
      r4.y = r6.x * r4.y;
      r16.xyz = texEnvMap_g.SampleLevel(SmplCube_s, r16.xyz, r4.y).xyz;
      r18.xyz = texEnvMap_g.SampleLevel(SmplCube_s, r18.xyz, r4.y).xyz;
      r4.y = cmp(0 < r14.x);
      r6.x = 1 + -abs(r7.z);
      r6.x = max(0, r6.x);
      r6.x = log2(r6.x);
      r6.x = r14.x * r6.x;
      r6.x = exp2(r6.x);
      r4.y = r4.y ? r6.x : 1;
      r20.xyz = r0.xyz * r16.xyz + -r0.xyz;
      r20.xyz = r4.yyy * r20.xyz + r0.xyz;
      r6.x = -r6.y * 0.00392156886 + 1;
      r7.x = r4.y * r6.x + r7.x;
      r6.x = abs(r7.z) * r14.y;
      r6.x = log2(r6.x);
      r6.x = r14.z * r6.x;
      r6.x = exp2(r6.x);
      r14.xyz = r7.xxx * r16.xyz;
      r14.xyz = r14.xyz * r4.yyy;
      r4.y = -r4.z * r13.z + 1;
      r9.xyz = r14.xyz * r4.yyy + r9.xyz;
      r13.yzw = r6.xxx * r18.xyz;
      r21.xyz = r20.xyz * r13.yzw;
    } else {
      r21.xyz = r0.xyz;
    }
  }
  r4.y = ssgiShadowRatio_g * r15.w;
  r13.yzw = sceneShadowColor_g.xyz * r15.xyz + -sceneShadowColor_g.xyz;
  r13.yzw = r4.yyy * r13.yzw + sceneShadowColor_g.xyz;
  r8.xyz = r13.yzw + r8.xyz;
  r8.xyz = min(float3(1,1,1), r8.xyz);
  r13.yzw = r15.xyz + -r12.xyz;
  r12.xyz = r15.www * r13.yzw + r12.xyz;
  r13.yzw = float3(1,1,1) + -r8.xyz;
  r11.xyz = r13.yzw * r11.xyz;
  r8.xyz = r11.www * r11.xyz + r8.xyz;
  r4.y = r7.x * r8.w;
  r11.xyz = float3(1,1,1) + -r8.xyz;
  r8.xyz = r4.yyy * r11.xyz + r8.xyz;
  r8.xyz = r8.xyz * lightColor_g.xyz + r12.xyz;
  r4.y = min(1, r6.w);
  r6.xyw = float3(1,1,1) + -r8.xyz;
  r6.xyw = r4.yyy * r6.xyw + r8.xyz;
  r4.y = 1 + -abs(r7.z);
  r4.y = max(0, r4.y);
  r4.y = r4.y * r10.w;
  r4.y = log2(r4.y);
  r4.y = r9.w * r4.y;
  r4.y = exp2(r4.y);
  r4.y = min(1, r4.y);
  r7.xzw = r10.xyz * r4.yyy + r9.xyz;
  if (r19.y != 0) {
    r4.y = lightIndices_g[r5.z].pointLightCount;
    r4.y = min(63, (uint)r4.y);
    r8.xyz = float3(0,0,0);
    r9.xyz = float3(0,0,0);
    r4.z = 0;
    while (true) {
      r9.w = cmp((uint)r4.z >= (uint)r4.y);
      if (r9.w != 0) break;
      
      // Fixed dynamic lookup:
      int lightIdx = lightIndices_g[r5.z].pointLightIndices[r4.z];
      
      r10.x = dynamicLights_g[lightIdx].pos.x;
      r10.y = dynamicLights_g[lightIdx].pos.y;
      r10.z = dynamicLights_g[lightIdx].pos.z;
      r10.xyz = r10.xyz + -r2.xyz;
      r10.w = dot(r10.xyz, r10.xyz);
      r11.x = sqrt(r10.w);
      r11.y = dynamicLights_g[lightIdx].radiusInv;
      r11.x = r11.x * r11.y;
      r11.y = dynamicLights_g[lightIdx].attenuation;
      r11.x = log2(abs(r11.x));
      r11.x = r11.y * r11.x;
      r11.x = exp2(r11.x);
      r11.x = 1 + -r11.x;
      r11.x = max(0, r11.x);
      r11.y = cmp(0 < r11.x);
      if (r11.y != 0) {
        r10.w = rsqrt(r10.w);
        r10.xyz = r10.xyz * r10.www;
        r10.w = dynamicLights_g[lightIdx].translucency;
        r11.y = dot(r10.xyz, r5.xyw);
        r10.w = max(r11.y, r10.w);
        r10.w = r11.x * r10.w;
        r11.x = dynamicLights_g[lightIdx].color.x;
        r11.y = dynamicLights_g[lightIdx].color.y;
        r11.z = dynamicLights_g[lightIdx].color.z;
        r9.xyz = r11.xyz * r10.www + r9.xyz;
        r10.xyz = r17.xyz * r3.yyy + r10.xyz;
        r11.w = dot(r10.xyz, r10.xyz);
        r11.w = rsqrt(r11.w);
        r10.xyz = r11.www * r10.xyz;
        r12.x = dynamicLights_g[lightIdx].specularIntensity;
        r12.y = dynamicLights_g[lightIdx].specularGlossiness;
        r9.w = r12.y * r6.z;
        r10.x = saturate(dot(r10.xyz, r5.xyw));
        r9.w = max(0.00100000005, r9.w);
        r10.x = log2(r10.x);
        r9.w = r10.x * r9.w;
        r9.w = exp2(r9.w);
        r10.xyz = r11.xyz * r9.www;
        r10.xyz = r10.xyz * r10.www;
        r8.xyz = r10.xyz * r12.xxx + r8.xyz;
      }
      r4.z = (int)r4.z + 1;
    }
    r9.xyz = r9.xyz * r12.www + r6.xyw;
    r4.y = lightIndices_g[r5.z].spotLightCount;
    r4.y = min(63, (uint)r4.y);
    r10.xyz = r8.xyz;
    r11.xyz = float3(0,0,0);
    r4.z = 0;
    while (true) {
      r9.w = cmp((uint)r4.z >= (uint)r4.y);
      if (r9.w != 0) break;
      
      // Fixed dynamic lookup:
      int lightIdx = lightIndices_g[r5.z].spotLightIndices[r4.z];
      
      r12.x = dynamicLights_g[lightIdx].pos.x;
      r12.y = dynamicLights_g[lightIdx].pos.y;
      r12.z = dynamicLights_g[lightIdx].pos.z;
      r12.xyz = r12.xyz + -r2.xyz;
      r10.w = dot(r12.xyz, r12.xyz);
      r11.w = rsqrt(r10.w);
      r12.xyz = r12.xyz * r11.www;
      r16.x = dynamicLights_g[lightIdx].vec.x;
      r16.y = dynamicLights_g[lightIdx].vec.y;
      r16.z = dynamicLights_g[lightIdx].vec.z;
      r16.w = dynamicLights_g[lightIdx].spotAngleInv;
      r11.w = dot(r12.xyz, r16.xyz);
      r11.w = max(0, r11.w);
      r11.w = 1 + -r11.w;
      r11.w = r11.w * r16.w;
      r13.y = dynamicLights_g[lightIdx].attenuationAngle;
      r11.w = log2(r11.w);
      r11.w = r13.y * r11.w;
      r11.w = exp2(r11.w);
      r11.w = 1 + -r11.w;
      r11.w = max(0, r11.w);
      r13.y = cmp(0 < r11.w);
      if (r13.y != 0) {
        r10.w = sqrt(r10.w);
        r13.y = dynamicLights_g[lightIdx].radiusInv;
        r10.w = r13.y * r10.w;
        r13.y = dynamicLights_g[lightIdx].attenuation;
        r10.w = log2(abs(r10.w));
        r10.w = r13.y * r10.w;
        r10.w = exp2(r10.w);
        r10.w = 1 + -r10.w;
        r10.w = max(0, r10.w);
        r10.w = r11.w * r10.w;
        r11.w = cmp(0 < r10.w);
        if (r11.w != 0) {
          r13.y = dynamicLights_g[lightIdx].translucency;
          r13.z = dynamicLights_g[lightIdx].shadowmapIndex;
          r11.w = cmp((int)r13.z != -1);
          if (r11.w != 0) {
            r16.xyzw = spotShadowMatrices_g[r13.z]._m00_m10_m20_m30;
            r18.xyzw = spotShadowMatrices_g[r13.z]._m01_m11_m21_m31;
            r20.xyzw = spotShadowMatrices_g[r13.z]._m02_m12_m22_m32;
            r22.xyzw = spotShadowMatrices_g[r13.z]._m03_m13_m23_m33;
            r14.x = dot(r2.xyzw, r16.xyzw);
            r14.y = dot(r2.xyzw, r18.xyzw);
            r14.z = dot(r2.xyzw, r20.xyzw);
            r11.w = dot(r2.xyzw, r22.xyzw);
            r16.xyz = r14.xyz / r11.www;
            r16.w = (uint)r13.z;
            r11.w = spotShadowMaps.SampleCmpLevelZero(SmplShadow_s, r16.xyw, r16.z).x;
            r14.xyz = float3(0.00244140625,0,0) + r16.xyw;
            r13.z = spotShadowMaps.SampleCmpLevelZero(SmplShadow_s, r14.xyz, r16.z).x;
            r13.z = 0.200000003 * r13.z;
            r11.w = r11.w * 0.200000003 + r13.z;
            r14.xyz = float3(-0.00244140625,0,0) + r16.xyw;
            r13.z = spotShadowMaps.SampleCmpLevelZero(SmplShadow_s, r14.xyz, r16.z).x;
            r11.w = r13.z * 0.200000003 + r11.w;
            r14.xyz = float3(0,0.00244140625,0) + r16.xyw;
            r13.z = spotShadowMaps.SampleCmpLevelZero(SmplShadow_s, r14.xyz, r16.z).x;
            r11.w = r13.z * 0.200000003 + r11.w;
            r14.xyz = float3(0,-0.00244140625,0) + r16.xyw;
            r13.z = spotShadowMaps.SampleCmpLevelZero(SmplShadow_s, r14.xyz, r16.z).x;
            r11.w = r13.z * 0.200000003 + r11.w;
            r10.w = r11.w * r10.w;
          }
          r11.w = dot(r12.xyz, r5.xyw);
          r11.w = max(r13.y, r11.w);
          r10.w = r11.w * r10.w;
          r13.y = dynamicLights_g[lightIdx].color.x;
          r13.z = dynamicLights_g[lightIdx].color.y;
          r13.w = dynamicLights_g[lightIdx].color.z;
          r11.xyz = r13.yzw * r10.www + r11.xyz;
          r12.xyz = r17.xyz * r3.yyy + r12.xyz;
          r11.w = dot(r12.xyz, r12.xyz);
          r11.w = rsqrt(r11.w);
          r12.xyz = r12.xyz * r11.www;
          r14.x = dynamicLights_g[lightIdx].specularIntensity;
          r14.y = dynamicLights_g[lightIdx].specularGlossiness;
          r9.w = r14.y * r6.z;
          r11.w = saturate(dot(r12.xyz, r5.xyw));
          r9.w = max(0.00100000005, r9.w);
          r11.w = log2(r11.w);
          r9.w = r11.w * r9.w;
          r9.w = exp2(r9.w);
          r12.xyz = r13.yzw * r9.www;
          r12.xyz = r12.xyz * r10.www;
          r10.xyz = r12.xyz * r14.xxx + r10.xyz;
        }
      }
      r4.z = (int)r4.z + 1;
    }
    r8.xyz = r11.xyz * r12.www + r9.xyz;
    r7.xzw = r10.xyz * r12.www + r7.xzw;
  } else {
    r3.y = lightIndices_g[r5.z].pointLightCount;
    r3.y = min(63, (uint)r3.y);
    r9.xyzw = float4(0,0,0,0);
    while (true) {
      r4.y = cmp((uint)r9.w >= (uint)r3.y);
      if (r4.y != 0) break;
      
      // Fixed dynamic lookup:
      int lightIdx = lightIndices_g[r5.z].pointLightIndices[r9.w];
      
      r10.x = dynamicLights_g[lightIdx].pos.x;
      r10.y = dynamicLights_g[lightIdx].pos.y;
      r10.z = dynamicLights_g[lightIdx].pos.z;
      r10.xyz = r10.xyz + -r2.xyz;
      r4.z = dot(r10.xyz, r10.xyz);
      r6.z = sqrt(r4.z);
      r10.w = dynamicLights_g[lightIdx].radiusInv;
      r6.z = r10.w * r6.z;
      r10.w = dynamicLights_g[lightIdx].attenuation;
      r6.z = log2(abs(r6.z));
      r6.z = r10.w * r6.z;
      r6.z = exp2(r6.z);
      r6.z = 1 + -r6.z;
      r6.z = max(0, r6.z);
      r10.w = cmp(0 < r6.z);
      if (r10.w != 0) {
        r10.w = dynamicLights_g[lightIdx].translucency;
        r4.z = rsqrt(r4.z);
        r10.xyz = r10.xyz * r4.zzz;
        r4.z = dot(r10.xyz, r5.xyw);
        r4.z = max(r10.w, r4.z);
        r10.x = dynamicLights_g[lightIdx].color.x;
        r10.y = dynamicLights_g[lightIdx].color.y;
        r10.z = dynamicLights_g[lightIdx].color.z;
        r10.xyz = r10.xyz * r6.zzz;
        r9.xyz = r10.xyz * r4.zzz + r9.xyz;
      }
      r9.w = (int)r9.w + 1;
    }
    r6.xyz = r9.xyz * r12.www + r6.xyw;
    r3.y = lightIndices_g[r5.z].spotLightCount;
    r3.y = min(63, (uint)r3.y);
    r9.xyzw = float4(0,0,0,0);
    while (true) {
      r4.y = cmp((uint)r9.w >= (uint)r3.y);
      if (r4.y != 0) break;
      
      // Fixed dynamic lookup:
      int lightIdx = lightIndices_g[r5.z].spotLightIndices[r9.w];
      
      r10.x = dynamicLights_g[lightIdx].pos.x;
      r10.y = dynamicLights_g[lightIdx].pos.y;
      r10.z = dynamicLights_g[lightIdx].pos.z;
      r10.xyz = r10.xyz + -r2.xyz;
      r4.z = dot(r10.xyz, r10.xyz);
      r6.w = rsqrt(r4.z);
      r10.xyz = r10.xyz * r6.www;
      r11.x = dynamicLights_g[lightIdx].vec.x;
      r11.y = dynamicLights_g[lightIdx].vec.y;
      r11.z = dynamicLights_g[lightIdx].vec.z;
      r11.w = dynamicLights_g[lightIdx].spotAngleInv;
      r6.w = dot(r10.xyz, r11.xyz);
      r6.w = max(0, r6.w);
      r6.w = 1 + -r6.w;
      r6.w = r6.w * r11.w;
      r10.w = dynamicLights_g[lightIdx].attenuationAngle;
      r6.w = log2(r6.w);
      r6.w = r10.w * r6.w;
      r6.w = exp2(r6.w);
      r6.w = 1 + -r6.w;
      r6.w = max(0, r6.w);
      r10.w = cmp(0 < r6.w);
      if (r10.w != 0) {
        r4.z = sqrt(r4.z);
        r10.w = dynamicLights_g[lightIdx].radiusInv;
        r4.z = r10.w * r4.z;
        r10.w = dynamicLights_g[lightIdx].attenuation;
        r4.z = log2(abs(r4.z));
        r4.z = r10.w * r4.z;
        r4.z = exp2(r4.z);
        r4.z = 1 + -r4.z;
        r4.z = max(0, r4.z);
        r4.z = r6.w * r4.z;
        r6.w = cmp(0 < r4.z);
        if (r6.w != 0) {
          r11.x = dynamicLights_g[lightIdx].translucency;
          r11.y = dynamicLights_g[lightIdx].shadowmapIndex;
          r6.w = cmp((int)r11.y != -1);
          if (r6.w != 0) {
            r16.xyzw = spotShadowMatrices_g[r11.y]._m00_m10_m20_m30;
            r17.xyzw = spotShadowMatrices_g[r11.y]._m01_m11_m21_m31;
            r18.xyzw = spotShadowMatrices_g[r11.y]._m02_m12_m22_m32;
            r20.xyzw = spotShadowMatrices_g[r11.y]._m03_m13_m23_m33;
            r12.x = dot(r2.xyzw, r16.xyzw);
            r12.y = dot(r2.xyzw, r17.xyzw);
            r12.z = dot(r2.xyzw, r18.xyzw);
            r6.w = dot(r2.xyzw, r20.xyzw);
            r16.xyz = r12.xyz / r6.www;
            r16.w = (uint)r11.y;
            r6.w = spotShadowMaps.SampleCmpLevelZero(SmplShadow_s, r16.xyw, r16.z).x;
            r11.yzw = float3(0.00244140625,0,0) + r16.xyw;
            r10.w = spotShadowMaps.SampleCmpLevelZero(SmplShadow_s, r11.yzw, r16.z).x;
            r10.w = 0.200000003 * r10.w;
            r6.w = r6.w * 0.200000003 + r10.w;
            r11.yzw = float3(-0.00244140625,0,0) + r16.xyw;
            r10.w = spotShadowMaps.SampleCmpLevelZero(SmplShadow_s, r11.yzw, r16.z).x;
            r6.w = r10.w * 0.200000003 + r6.w;
            r11.yzw = float3(0,0.00244140625,0) + r16.xyw;
            r10.w = spotShadowMaps.SampleCmpLevelZero(SmplShadow_s, r11.yzw, r16.z).x;
            r6.w = r10.w * 0.200000003 + r6.w;
            r11.yzw = float3(0,-0.00244140625,0) + r16.xyw;
            r10.w = spotShadowMaps.SampleCmpLevelZero(SmplShadow_s, r11.yzw, r16.z).x;
            r6.w = r10.w * 0.200000003 + r6.w;
            r4.z = r6.w * r4.z;
          }
          r6.w = dot(r10.xyz, r5.xyw);
          r6.w = max(r11.x, r6.w);
          r10.x = dynamicLights_g[lightIdx].color.x;
          r10.y = dynamicLights_g[lightIdx].color.y;
          r10.z = dynamicLights_g[lightIdx].color.z;
          r10.xyz = r10.xyz * r4.zzz;
          r9.xyz = r10.xyz * r6.www + r9.xyz;
        }
      }
      r9.w = (int)r9.w + 1;
    }
    r8.xyz = r9.xyz * r12.www + r6.xyz;
  }
  r5.xyz = r7.xzw * r7.yyy;
  r5.xyz = r21.xyz * r8.xyz + r5.xyz;
  r3.y = cmp(0 < r1.y);
  r0.x = dot(r0.xyz, float3(0.298999995,0.587000012,0.114));
  r0.x = r0.x + -r1.y;
  r0.x = max(0, r0.x);
  r0.x = r0.x / r1.y;
  r0.x = min(1, r0.x);
  r0.x = r3.y ? r0.x : 1;
  r0.y = r14.w * r3.z;
  r0.x = r0.y * r0.x;
  r0.y = -1 + r8.w;
  r0.y = r1.z * r0.y + 1;
  r0.x = r0.x * r0.y;
  r6.xyz = min(float3(1,1,1), r15.xyz);
  r6.xyz = -mapAOColor_g.xyz + r6.xyz;
  r6.xyz = r15.www * r6.xyz + mapAOColor_g.xyz;
  r0.y = 1 + -r4.x;
  r4.xyz = r5.xyz * r6.xyz + -r5.xyz;
  r4.xyz = r0.yyy * r4.xyz + r5.xyz;
  r0.y = -fogNearDistance_g + -r4.w;
  r0.y = saturate(fogFadeRangeInv_g * r0.y);
  r0.z = -fogHeight_g + r2.y;
  r0.z = saturate(fogHeightRangeInv_g * r0.z);
  r0.y = r0.y * r0.z;
  r0.z = fogIntensity_g * r0.y;
  r0.w = (int)r0.w & 16;
  r1.y = -r3.x * 0.00392156886 + 1;
  r0.w = r0.w ? r1.y : 1;
  r0.z = r0.z * r13.x;
  r0.z = r0.z * r0.w;
  r3.xyz = fogColor_g.xyz + -r4.xyz;
  r3.xyz = r0.zzz * r3.xyz + r4.xyz;
  r0.z = (int)r1.x & 64;
  r0.z = cmp((int)r0.z == 0);
  r0.w = cmp(0 != isEnableSky_g);
  r0.z = r0.w ? r0.z : 0;
  if (r0.z != 0) {
    r0.z = -r4.w / skyLutCameraFarClip_g;
    r1.xy = invVPSize_g.xy * v0.xy;
    r0.z = -skyLutNearOverFarClip_g + r0.z;
    r0.w = -skyLutNearOverFarClip_g + 1;
    r1.z = r0.z / r0.w;
    r4.xyz = atmosphereInscatterLUT.SampleLevel(samLinear_s, r1.xyz, 0).xyz;
    r1.xyz = atmosphereExtinctionLUT.SampleLevel(samLinear_s, r1.xyz, 0).xyz;
    r3.xyz = r3.xyz * r1.xyz + r4.xyz;
  }
  if (outlineShapeCount_g != 0) {
    r0.zw = -outlineShapeMaskUVParam_g.xy + r2.xz;
    r0.zw = outlineShapeMaskUVParam_g.zw * r0.zw;
    r1.xy = cmp(r0.zw >= float2(0,0));
    r1.x = r1.y ? r1.x : 0;
    r1.yz = cmp(float2(1,1) >= r0.zw);
    r1.y = r1.z ? r1.y : 0;
    r1.x = r1.y ? r1.x : 0;
    if (r1.x != 0) {
      r0.z = outlineShapeMask.SampleLevel(samLinear_s, r0.zw, 0).x;
    } else {
      r0.z = 1;
    }
    r2.w = 1;
    r1.xyz = r3.xyz;
    r0.w = 0;
    while (true) {
      r4.x = cmp((uint)r0.w >= outlineShapeCount_g);
      if (r4.x != 0) break;
      r4.x = outlineShapes_g[r0.w].radius;
      r4.y = outlineShapes_g[r0.w].height_base;
      r4.z = outlineShapes_g[r0.w].height_width;
      r4.w = outlineShapes_g[r0.w].height_gradation_width;
      r4.y = -r4.y + r2.y;
      r5.x = cmp(r4.z >= abs(r4.y));
      if (r5.x != 0) {
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
        r8.x = outlineShapes_g[r0.w].gradation_size.x;
        r8.y = outlineShapes_g[r0.w].gradation_size.y;
        r8.z = outlineShapes_g[r0.w].gradation_sharpness;
        r8.w = outlineShapes_g[r0.w].type;
        r9.x = abs(r4.y) + r4.w;
        r9.x = cmp(r4.z < r9.x);
        r4.y = r4.z + -abs(r4.y);
        r4.y = r4.y / r4.w;
        r4.y = r9.x ? r4.y : 1;
        if (r8.w == 0) {
          r6.x = r5.w;
          r4.zw = -r6.xy + r2.xz;
          r4.z = dot(r4.zw, r4.zw);
          r4.z = sqrt(r4.z);
          r4.w = cmp(r4.x < r4.z);
          r9.x = -r8.x + r4.x;
          r9.y = cmp(r4.z >= r9.x);
          r9.z = ~(int)r9.y;
          r4.z = -r9.x + r4.z;
          r4.z = r4.z / r8.x;
          r4.z = r9.y ? r4.z : 0;
          r4.z = r4.w ? 0 : r4.z;
          r4.w = (int)r4.w | (int)r9.z;
        } else {
          r9.x = cmp((int)r8.w == 1);
          if (r9.x != 0) {
            r9.x = outlineShapes_g[r0.w].fan_angle;
            r6.xz = r5.wz;
            r9.yz = -r6.xy + r2.xz;
            r9.w = dot(r9.yz, r9.yz);
            r10.x = sqrt(r9.w);
            r10.y = cmp(r4.x < r10.x);
            r4.x = -r8.x + r4.x;
            r9.w = rsqrt(r9.w);
            r9.yz = r9.yz * r9.ww;
            r9.y = dot(r6.zw, r9.yz);
            r9.z = 1 + -abs(r9.y);
            r9.z = sqrt(r9.z);
            r9.w = abs(r9.y) * -0.0187292993 + 0.0742610022;
            r9.w = r9.w * abs(r9.y) + -0.212114394;
            r9.w = r9.w * abs(r9.y) + 1.57072878;
            r10.z = r9.w * r9.z;
            r10.z = r10.z * -2 + 3.14159274;
            r9.y = cmp(r9.y < -r9.y);
            r9.y = r9.y ? r10.z : 0;
            r9.y = r9.w * r9.z + r9.y;
            r9.z = cmp(r9.x >= r9.y);
            r9.w = ~(int)r9.z;
            r10.z = 6.28318548 * r10.x;
            r9.xy = r10.zz * r9.xy;
            r9.y = 0.159154937 * r9.y;
            r9.x = r9.x * 0.159154937 + -r9.y;
            r9.x = r9.x / r8.x;
            r9.x = min(1, r9.x);
            r9.x = 1 + -r9.x;
            r4.x = r10.x + -r4.x;
            r4.x = r4.x / r8.x;
            r4.x = min(1, r4.x);
            r4.x = max(r9.x, r4.x);
            r4.x = r9.z ? r4.x : 0;
            r4.z = r10.y ? 0 : r4.x;
            r4.w = (int)r10.y | (int)r9.w;
          } else {
            r4.x = cmp((int)r8.w == 2);
            r5.x = dot(r2.xyzw, r5.xyzw);
            r5.y = dot(r2.xwyz, r6.xyzw);
            r5.zw = cmp(r5.xy < float2(0.5,0.5));
            r5.z = r5.w ? r5.z : 0;
            r6.xy = cmp(float2(-0.5,-0.5) < r5.xy);
            r5.w = r6.y ? r6.x : 0;
            r5.z = r5.w ? r5.z : 0;
            r6.xy = float2(0.5,0.5) + -r8.xy;
            r6.zw = cmp(abs(r5.xy) < r6.xy);
            r5.w = r6.w ? r6.z : 0;
            r5.xy = -r6.xy + abs(r5.xy);
            r5.xy = r5.xy / r8.xy;
            r5.x = max(r5.x, r5.y);
            r5.x = r5.w ? 0 : r5.x;
            r5.x = r5.z ? r5.x : 0;
            r5.y = r5.z ? r5.w : -1;
            r4.zw = r4.xx ? r5.xy : 0;
          }
        }
        r4.x = log2(r4.z);
        r4.x = r8.z * r4.x;
        r4.x = exp2(r4.x);
        r4.x = r4.x * r7.w;
        r4.x = r4.x * r0.z;
        r7.w = r4.x * r4.y;
        r4.xyzw = r4.wwww ? float4(0,0,0,0) : r7.xyzw;
      } else {
        r4.xyzw = float4(0,0,0,0);
      }
      r4.xyz = r4.xyz + -r1.xyz;
      r1.xyz = r4.www * r4.xyz + r1.xyz;
      r0.w = (int)r0.w + 1;
    }
    r3.xyz = r1.xyz;
  }
  r0.y = -r0.y * fogIntensity_g + 1;
  r0.y = r1.w * r0.y;
  r0.z = 65.5350037 * r3.w;
  r0.z = (uint)r0.z;
  o1.x = min(0x0000ffff, (uint)r0.z);
  r0.y = 255 * r0.y;
  r0.y = (uint)r0.y;
  r0.y = min(255, (uint)r0.y);
  o1.y = mad((int)r0.y, 256, (int)r0.y);
  r0.x = saturate(0.100000001 * r0.x);
  r0.x = 255 * r0.x;
  r0.x = (uint)r0.x;
  o1.w = r19.w ? r0.x : 0;
  o0.xyz = r3.xyz;
  o0.w = 1;
  o1.z = r0.y;
  return;
}
