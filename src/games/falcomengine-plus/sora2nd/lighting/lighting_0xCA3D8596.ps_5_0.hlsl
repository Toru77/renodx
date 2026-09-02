// ---- Created with 3Dmigoto v1.4.1 on Thu Aug 20 07:13:29 2026

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
    float volumeFogIntensity;      // Offset:  132
    uint flag;                     // Offset:  136
    float reserve;                 // Offset:  140
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
  float fogExp_g : packoffset(c32);
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
  float2 cloudShadowOffset_g : packoffset(c74);
  float cloudShadowScale_g : packoffset(c74.z);
  float4x4 prevViewProj_g : packoffset(c75);
  float2 jitterDiff_g : packoffset(c79);
  float4 shadowBlurRadius_g : packoffset(c80);
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

cbuffer cb_volume_fog : register(b7)
{
  float volumeLightBrightness_g : packoffset(c0);
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
  float temporalRatioMax_g : packoffset(c3.w);
  float2 prevScaling_g : packoffset(c4);
  float2 prevUVClamp_g : packoffset(c4.z);
  float volumeNearFadeInv_g : packoffset(c5);
  float volumeNearFadeValue_g : packoffset(c5.y);
  float temporalRatioMin_g : packoffset(c5.z);
  float densityScale_g : packoffset(c5.w);
}

cbuffer cb_deferred : register(b4)
{
  float2 shadowUVClamp_g : packoffset(c0);
  float3 mapAOColor_g : packoffset(c1);
  float mapAOIntensity_g : packoffset(c1.w);
  float3 shadowEdgeColor_g : packoffset(c2);
  float shadowEdgeSharpness_g : packoffset(c2.w);
}

SamplerState samPoint_s : register(s0);
SamplerState samLinear_s : register(s1);
SamplerState SmplLinearWrap_s : register(s11);
SamplerState SmplCube_s : register(s14);
SamplerState SmplLinearClamp_s : register(s15);
SamplerComparisonState SmplShadow_s : register(s13);
Texture2D<float4> colorTexture : register(t0);
Texture2D<uint4> mrtTexture0 : register(t1);
Texture2D<uint4> mrtTexture1 : register(t2);
Texture2D<uint2> mrtTexture2 : register(t3);
Texture2D<float4> depthTexture : register(t4);
Texture2D<float4> ssaoTexture : register(t5);
Texture2D<uint4> gtvbaoTexture : register(t22);  // GTVBAO AO (r32_uint, packed 0-255)
Texture2D<float4> vbgiTexture : register(t23);   // VBGI indirect diffuse (R16G16B16A16)
StructuredBuffer<DeferredParam> deferredParams_g : register(t6);
Texture2D<float4> shadowTexture : register(t7);
Texture2D<float4> outlinePrepareTexture : register(t8);
StructuredBuffer<LightParam> dynamicLights_g : register(t11);
StructuredBuffer<LightIndexData> lightIndices_g : register(t12);
StructuredBuffer<float4x4> spotShadowMatrices_g : register(t14);
TextureCube<float4> texEnvMap_g : register(t17);
TextureCube<float4> dynCubeHistPosTex : register(t29);  // dynamic cube history world pos (debug 11/12)
TextureCube<float4> dynCubeVanillaTex : register(t30);  // game's vanilla cubemap (fallback layer)
Texture2D<float4> dynCubeSSRTex : register(t31);        // blurred SSR result (rgb=color, a=confidence)
Texture2D<float4> dynCubeSSRRawTex : register(t32);     // raw SSR result (debug 17)
Texture2DArray<float4> spotShadowMaps : register(t18);
Texture3D<float4> atmosphereInscatterLUT : register(t19);
Texture3D<float4> atmosphereExtinctionLUT : register(t20);
Texture2D<float4> texMirror_g : register(t21);
Texture2D<float4> texSSRMap_g : register(t24);
Texture2D<float4> ssrCustomTexture : register(t25);  // removed feature. Non existent
Texture3D<float4> volumeFogTexture_g : register(t26);
Texture2D<float4> texCloudShadow : register(t27);

#include "../../shared.h"
#include "../../dyncube/parallax_cubemap.hlsli"
#include "../../reference/brdf.hlsli"
#include "../../reference/rendering.hlsl"

// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_Position0,
  float4 v1 : TEXCOORD0,
  out float4 o0 : SV_Target0,
  out uint4 o1 : SV_Target1,
  out uint2 o2 : SV_Target2)
{
  float4 r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25;
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
  // ── Custom SSR debug views (Phase 1/3 instrumentation) ──
  // Phase 2.10/3 integration proofs (raw slider indices 28/29/30):
  if (shader_injection_data.ssr_custom_bound > 0.5f) {
    const int dvRaw = (int)(shader_injection_data.ssr_debug_view + 0.5f);
    if (dvRaw == 28) {          // t25 RGB — traced radiance reaching the PS
      float4 t = ssrCustomTexture.SampleLevel(SmplLinearClamp_s,
          resolutionScaling_g.xy * v1.xy, 0);
      o0.rgb = t.rgb; o0.a = 1.0;
      o1.xyzw = r2.xyzw; o2.xy = r3.xy; return;
    }
    if (dvRaw == 29) {          // t25 Alpha — confidence/Fresnel magnitude
      float4 t = ssrCustomTexture.SampleLevel(SmplLinearClamp_s,
          resolutionScaling_g.xy * v1.xy, 0);
      o0.rgb = t.aaa; o0.a = 1.0;
      o1.xyzw = r2.xyzw; o2.xy = r3.xy; return;
    }
    if (dvRaw == 30) {          // Vanilla t24 — eligibility reference
      float4 t = texSSRMap_g.SampleLevel(SmplLinearClamp_s,
          resolutionScaling_g.xy * v1.xy, 0);
      o0.rgb = t.rgb; o0.a = 1.0;
      o1.xyzw = r2.xyzw; o2.xy = r3.xy; return;
    }
    if (dvRaw == 31) {          // SSR Coverage — resolved α as grayscale
      float4 t = ssrCustomTexture.SampleLevel(SmplLinearClamp_s,
          resolutionScaling_g.xy * v1.xy, 0);
      o0.rgb = t.aaa; o0.a = 1.0;
      o1.xyzw = r2.xyzw; o2.xy = r3.xy; return;
    }
  }
  if (shader_injection_data.ssr_custom_bound > 0.5f
      && shader_injection_data.ssr_debug_view > 0.5f) {
    float3 ssrDbg = ssrCustomTexture.SampleLevel(samPoint_s, v1.xy, 0).rgb;
    o0.rgb = ssrDbg * 0.3;
    o0.a = 1.0;
    o1.xyzw = r2.xyzw;
    o2.xy = r3.xy;
    return;
  }
  r4.z = depthTexture.SampleLevel(samPoint_s, v1.xy, 0).x;
  r5.xyz = ssaoTexture.SampleLevel(samLinear_s, v1.xy, 0).xyz;
  // Sample AO: always read vanilla SSAO first, then conditionally
  // replace only the .x channel with GTVBAO (kai-vanillaplus pattern).
  float3 ssao_sample = r5.xyz;
  bool GTVBAO_bound = shader_injection_data.gtvbao_dedicated_bound > 0.5f;

  float3 ao_sample = ssao_sample;
  if (GTVBAO_bound) {
    uint width, height;
    gtvbaoTexture.GetDimensions(width, height);
    uint2 texel = uint2(saturate(v1.xy) * float2(width, height));
    uint4 GTVBAO_raw = gtvbaoTexture.Load(int3(texel, 0));
    float GTVBAO_ao = float(GTVBAO_raw.x) / 255.0;

    int fix = (int)shader_injection_data.gtvbao_fix_experimental;
    if (fix == 1) {
      ao_sample.x = 1.0;  // Neutral: test if veil is from AO value
    } else if (fix == 2) {
      ao_sample.x = float(GTVBAO_raw.x) / 255.0;  // Full uint, no 0xFF mask
    } else if (fix == 3) {
      ao_sample.x = 1.0 - GTVBAO_ao;  // Inverted encoding
    } else if (fix == 4) {
      ao_sample = float3(GTVBAO_ao, GTVBAO_ao, GTVBAO_ao);  // All channels GTVBAO
    } else {
      ao_sample.x = GTVBAO_ao;  // Default current
    }
  }
  r5.xyz = ao_sample;

  // ── Cached VBGI: sample & process once, reused at all sites (Kai-style optimization) ──
  float3 cachedVBGI = float3(0, 0, 0);
  float cachedVBGILuma = 0;
  if (shader_injection_data.gtvbao_vbgi_bound > 0.5f) {
    float3 giRaw = vbgiTexture.SampleLevel(samLinear_s, v1.xy, 0).rgb;
    float giLuma = dot(giRaw, float3(0.299, 0.587, 0.114));
    float3 giColor = lerp(giLuma.xxx, giRaw, shader_injection_data.vbgi_saturation);
    giColor *= shader_injection_data.vbgi_intensity;
    if (shader_injection_data.vbgi_max_clamp > 0.0) {
      giColor = min(giColor, shader_injection_data.vbgi_max_clamp);
    }
    if (shader_injection_data.vbgi_affect_lights > 0.5f) {
      float lightLuma = dot(lightColor_g.xyz, float3(0.299f, 0.587f, 0.114f));
      float3 lightContrib = lerp(lightLuma.xxx, lightColor_g.xyz, shader_injection_data.vbgi_lights_saturation);
      lightContrib = saturate(lightContrib);
      giColor += lightContrib * shader_injection_data.vbgi_lights_strength * 0.2f;
    }
    cachedVBGI = giColor;
    cachedVBGILuma = dot(cachedVBGI, float3(0.333, 0.333, 0.333));
  }

  // Reduce AO where indirect light exists
  if (shader_injection_data.vbgi_reduce_ao > 0.5f && shader_injection_data.gtvbao_vbgi_bound > 0.5f) {
    r5.x = lerp(r5.x, 1.0, saturate(cachedVBGILuma * shader_injection_data.vbgi_reduce_ao_strength));
  }

  // —— GTVBAO Debug View ——
  // Scaled for HDR: raw 0-1 AO values would be blinding without scaling.
  if (shader_injection_data.gtvbao_debug_view > 0.5f) {
    int mode = (int)shader_injection_data.gtvbao_debug_view;
    float hdr_scale = 0.3;
    if (mode == 1) {
      // AO Only: red tint for visibility
      o0.rgb = float3(ao_sample.x * hdr_scale, 0.0, 0.0);
      o0.a = 1.0; o1.xyzw = r2.xyzw; o2.xy = r3.xy; return;
    }
    if (mode == 2) {
      // GTVBAO Raw: uint4.x decoded directly
      uint width, height;
      gtvbaoTexture.GetDimensions(width, height);
      uint2 texel = uint2(saturate(v1.xy) * float2(width, height));
      uint4 raw = gtvbaoTexture.Load(int3(texel, 0));
      float raw_ao = float(raw.x) / 255.0;
      o0.rgb = float3(raw_ao, raw_ao, raw_ao) * hdr_scale;
      o0.a = 1.0; o1.xyzw = r2.xyzw; o2.xy = r3.xy; return;
    }
    if (mode == 3) {
      // GTVBAO Raw RGBA: uint4 channels decoded directly
      uint width, height;
      gtvbaoTexture.GetDimensions(width, height);
      uint2 texel = uint2(saturate(v1.xy) * float2(width, height));
      uint4 raw = gtvbaoTexture.Load(int3(texel, 0));
      o0.rgba = float4(
        float(raw.x) / 255.0,
        float(raw.y) / 255.0,
        float(raw.z) / 255.0,
        float(raw.w) / 255.0
      ) * hdr_scale;
      o0.a = 1.0; o1.xyzw = r2.xyzw; o2.xy = r3.xy; return;
    }
    if (mode == 4) {
      // Vanilla SSAO: t5.x greyscale
      float s = ssaoTexture.SampleLevel(samLinear_s, v1.xy, 0).x;
      o0.rgb = float3(s, s, s) * hdr_scale;
      o0.a = 1.0; o1.xyzw = r2.xyzw; o2.xy = r3.xy; return;
    }
    if (mode == 5) {
      // Depth: t4.x greyscale
      float d = depthTexture.SampleLevel(samLinear_s, v1.xy, 0).x;
      o0.rgb = float3(d, d, d) * hdr_scale;
      o0.a = 1.0; o1.xyzw = r2.xyzw; o2.xy = r3.xy; return;
    }
    if (mode == 9) {
      // FoliageMask: GTVBAO raw AO (mid-gray = foliage detected)
      uint fw, fh;
      gtvbaoTexture.GetDimensions(fw, fh);
      uint2 ftexel = uint2(saturate(v1.xy) * float2(fw, fh));
      uint4 fraw = gtvbaoTexture.Load(int3(ftexel, 0));
      float fraw_ao = float(fraw.x) / 255.0;
      o0.rgb = float3(fraw_ao, fraw_ao, fraw_ao) * hdr_scale;
      o0.a = 1.0; o1.xyzw = r2.xyzw; o2.xy = r3.xy; return;
    }
  }
  r4.xy = v1.zw * float2(2,-2) + float2(-1,1);
  r4.w = 1;
  r6.x = dot(r4.xyzw, viewProjInv_g._m00_m10_m20_m30);
  r6.y = dot(r4.xyzw, viewProjInv_g._m01_m11_m21_m31);
  r6.z = dot(r4.xyzw, viewProjInv_g._m02_m12_m22_m32);
  r6.w = dot(r4.xyzw, viewProjInv_g._m03_m13_m23_m33);
  r4.xyzw = r6.xyzw / r6.wwww;
  r1.w = dot(view_g._m02_m12_m22_m32, r4.xyzw);
  r6.xy = invVPSize_g.xy * v0.xy;
  r3.z = -r1.w / volumeCameraFarClip_g;
  r7.xy = resolutionScaling_g.xy * r6.xy;
  r3.z = r3.z * volumeCameraFarOverMaxFar_g + -volumeNearOverFarClip_g;
  r3.w = -volumeNearOverFarClip_g + 1;
  r7.z = r3.z / r3.w;
  // Sample the volume: vanilla trilinear or improved tricubic haze AA
  {
    float3 uvw = r7.xyz;
    uint volW, volH, volD;
    volumeFogTexture_g.GetDimensions(volW, volH, volD);
    float3 volSize = float3((float)volW, (float)volH, (float)volD);

    float4 volSample;
    if (shader_injection_data.volfog_haze_aa_mode > 0.5) {
      volSample = renodx::rendering::SampleTricubicBSpline(volumeFogTexture_g, SmplLinearClamp_s, uvw, volSize);
    } else {
      volSample = volumeFogTexture_g.SampleLevel(SmplLinearClamp_s, uvw, 0);
    }
    r7.xyzw = volSample.xyzw;
  }
  r3.z = (int)r1.z & 8;
  if (r3.z == 0) {
    r8.yz = (uint2)r3.xx >> int2(5,10);
    r8.x = r3.x;
    r8.xyz = (int3)r8.xyz & int3(31,31,31);
    r8.xyz = (uint3)r8.xyz;
    r9.x = 0.0322580636;
    r9.yz = r0.yz;
    r8.xyz = r9.xyz * r8.xyz;
    r3.zw = (int2)r1.zz & int2(1,64);
    if (r3.z != 0) {
      r9.x = r0.x;
      r9.yz = float2(0.0322580636,0.0322580636);
      r10.xyz = r9.xyz * r8.xyz;
      r3.z = r5.y * r5.z;
      r5.yzw = -r9.xyz * r8.xyz + r0.xyz;
      r9.xyz = r3.zzz * r5.yzw + r10.xyz;
      r3.z = (int)r1.z & 4;
      if (r3.z == 0) {
        r5.yz = outlinePrepareTexture.SampleLevel(samLinear_s, v1.xy, 0).xy;
        r10.x = r5.y;
        r10.yw = float2(1,1);
        r3.z = dot(projInv_g._m22_m32, r10.xy);
        r5.y = dot(projInv_g._m23_m33, r10.xy);
        r3.z = r3.z / r5.y;
        r11.xyzw = -fadeBeginDistance_g.xyzw + -r3.zzzz;
        r11.xyzw = saturate(fadeRangeInv_g.xyzw * r11.xyzw);
r12.xy = float2(maxThickness_g, depthThresholdNear_g);
    r12.z = densityByDepthDiffNear_g;
        r13.x = 1 + -r12.x;
        r13.y = depthThresholdFar_g + -r12.y;
        r13.z = densityByDepthDiffFar_g + -r12.z;
        r8.yzw = r11.yzw * r13.xyz + r12.xyz;
        r5.yw = offsetsAndWeights[0].xy * r8.yy + v1.xy;
        r5.yw = min(uvClamp_g.xy, r5.yw);
        r6.w = 0.5 * r8.y;
        r6.w = max(1, r6.w);
        r10.xy = offsetsAndWeights[0].xy * r6.ww + v1.xy;
        r10.xy = min(uvClamp_g.xy, r10.xy);
        r10.z = outlinePrepareTexture.SampleLevel(samLinear_s, r5.yw, 0).x;
        r5.y = outlinePrepareTexture.SampleLevel(samLinear_s, r10.xy, 0).y;
        r5.w = dot(projInv_g._m22_m32, r10.zw);
        r10.x = dot(projInv_g._m23_m33, r10.zw);
        r5.w = r5.w / r10.x;
        r10.x = cmp(r5.y < 0);
        r5.w = -r5.w + r3.z;
        r10.y = cmp(r5.w >= 0);
        r10.z = cmp(r5.w >= r8.z);
        r12.x = r10.z ? 1.000000 : 0;
        r5.y = r5.z + -r5.y;
        r10.z = cmp(abs(r5.y) >= 0.0299999993);
        r10.yz = r10.yz ? float2(1,1) : 0;
        r12.y = r10.y * r10.z + r12.x;
        r10.z = abs(r5.w);
        r10.w = r10.y * abs(r5.y) + r10.z;
        r5.yw = offsetsAndWeights[1].xy * r8.yy + v1.xy;
        r5.yw = min(uvClamp_g.xy, r5.yw);
        r11.yz = offsetsAndWeights[1].xy * r6.ww + v1.xy;
        r11.yz = min(uvClamp_g.xy, r11.yz);
        r13.x = outlinePrepareTexture.SampleLevel(samLinear_s, r5.yw, 0).x;
        r5.y = outlinePrepareTexture.SampleLevel(samLinear_s, r11.yz, 0).y;
        r13.yw = float2(1,1);
        r5.w = dot(projInv_g._m22_m32, r13.xy);
        r10.y = dot(projInv_g._m23_m33, r13.xy);
        r5.w = r5.w / r10.y;
        r10.y = cmp(r5.y < 0);
        r10.x = (int)r10.x | (int)r10.y;
        r5.w = -r5.w + r3.z;
        r10.y = cmp(r5.w >= 0);
        r10.y = r10.y ? 1.000000 : 0;
        r11.y = cmp(r5.w >= r8.z);
        r13.x = r11.y ? 1.000000 : 0;
        r5.y = r5.z + -r5.y;
        r11.y = cmp(abs(r5.y) >= 0.0299999993);
        r11.y = r11.y ? 1.000000 : 0;
        r13.y = r10.y * r11.y + r13.x;
        r11.yz = offsetsAndWeights[1].zz * r13.xy;
        r11.yz = r12.xy * offsetsAndWeights[0].zz + r11.yz;
        r12.z = abs(r5.w);
        r12.w = r10.y * abs(r5.y) + r12.z;
        r5.yw = max(r12.zw, r10.zw);
        r10.yz = offsetsAndWeights[2].xy * r8.yy + v1.xy;
        r10.yz = min(uvClamp_g.xy, r10.yz);
        r12.xy = offsetsAndWeights[2].xy * r6.ww + v1.xy;
        r12.xy = min(uvClamp_g.xy, r12.xy);
        r13.z = outlinePrepareTexture.SampleLevel(samLinear_s, r10.yz, 0).x;
        r10.y = outlinePrepareTexture.SampleLevel(samLinear_s, r12.xy, 0).y;
        r10.z = dot(projInv_g._m22_m32, r13.zw);
        r10.w = dot(projInv_g._m23_m33, r13.zw);
        r10.z = r10.z / r10.w;
        r10.w = cmp(r10.y < 0);
        r10.x = (int)r10.x | (int)r10.w;
        r10.z = -r10.z + r3.z;
        r10.w = cmp(r10.z >= 0);
        r10.w = r10.w ? 1.000000 : 0;
        r11.w = cmp(r10.z >= r8.z);
        r12.x = r11.w ? 1.000000 : 0;
        r10.y = -r10.y + r5.z;
        r11.w = cmp(abs(r10.y) >= 0.0299999993);
        r11.w = r11.w ? 1.000000 : 0;
        r12.y = r10.w * r11.w + r12.x;
        r11.yz = r12.xy * offsetsAndWeights[2].zz + r11.yz;
        r12.z = abs(r10.z);
        r12.w = r10.w * abs(r10.y) + r12.z;
        r5.yw = max(r12.zw, r5.yw);
        r10.yz = offsetsAndWeights[3].xy * r8.yy + v1.xy;
        r10.yz = min(uvClamp_g.xy, r10.yz);
        r12.xy = offsetsAndWeights[3].xy * r6.ww + v1.xy;
        r12.xy = min(uvClamp_g.xy, r12.xy);
        r13.x = outlinePrepareTexture.SampleLevel(samLinear_s, r10.yz, 0).x;
        r10.y = outlinePrepareTexture.SampleLevel(samLinear_s, r12.xy, 0).y;
        r13.yw = float2(1,1);
        r10.z = dot(projInv_g._m22_m32, r13.xy);
        r10.w = dot(projInv_g._m23_m33, r13.xy);
        r10.z = r10.z / r10.w;
        r10.w = cmp(r10.y < 0);
        r10.x = (int)r10.x | (int)r10.w;
        r10.z = -r10.z + r3.z;
        r10.w = cmp(r10.z >= 0);
        r10.w = r10.w ? 1.000000 : 0;
        r11.w = cmp(r10.z >= r8.z);
        r12.x = r11.w ? 1.000000 : 0;
        r10.y = -r10.y + r5.z;
        r11.w = cmp(abs(r10.y) >= 0.0299999993);
        r11.w = r11.w ? 1.000000 : 0;
        r12.y = r10.w * r11.w + r12.x;
        r11.yz = r12.xy * offsetsAndWeights[3].zz + r11.yz;
        r12.z = abs(r10.z);
        r12.w = r10.w * abs(r10.y) + r12.z;
        r5.yw = max(r12.zw, r5.yw);
        r10.yz = offsetsAndWeights[4].xy * r8.yy + v1.xy;
        r10.yz = min(uvClamp_g.xy, r10.yz);
        r12.xy = offsetsAndWeights[4].xy * r6.ww + v1.xy;
        r12.xy = min(uvClamp_g.xy, r12.xy);
        r13.z = outlinePrepareTexture.SampleLevel(samLinear_s, r10.yz, 0).x;
        r10.y = outlinePrepareTexture.SampleLevel(samLinear_s, r12.xy, 0).y;
        r10.z = dot(projInv_g._m22_m32, r13.zw);
        r10.w = dot(projInv_g._m23_m33, r13.zw);
        r10.z = r10.z / r10.w;
        r10.w = cmp(r10.y < 0);
        r10.x = (int)r10.x | (int)r10.w;
        r10.z = -r10.z + r3.z;
        r10.w = cmp(r10.z >= 0);
        r10.w = r10.w ? 1.000000 : 0;
        r11.w = cmp(r10.z >= r8.z);
        r12.x = r11.w ? 1.000000 : 0;
        r10.y = -r10.y + r5.z;
        r11.w = cmp(abs(r10.y) >= 0.0299999993);
        r11.w = r11.w ? 1.000000 : 0;
        r12.y = r10.w * r11.w + r12.x;
        r11.yz = r12.xy * offsetsAndWeights[4].zz + r11.yz;
        r12.z = abs(r10.z);
        r12.w = r10.w * abs(r10.y) + r12.z;
        r5.yw = max(r12.zw, r5.yw);
        r10.yz = offsetsAndWeights[5].xy * r8.yy + v1.xy;
        r10.yz = min(uvClamp_g.xy, r10.yz);
        r12.xy = offsetsAndWeights[5].xy * r6.ww + v1.xy;
        r12.xy = min(uvClamp_g.xy, r12.xy);
        r13.x = outlinePrepareTexture.SampleLevel(samLinear_s, r10.yz, 0).x;
        r10.y = outlinePrepareTexture.SampleLevel(samLinear_s, r12.xy, 0).y;
        r13.yw = float2(1,1);
        r10.z = dot(projInv_g._m22_m32, r13.xy);
        r10.w = dot(projInv_g._m23_m33, r13.xy);
        r10.z = r10.z / r10.w;
        r10.w = cmp(r10.y < 0);
        r10.x = (int)r10.x | (int)r10.w;
        r10.z = -r10.z + r3.z;
        r10.w = cmp(r10.z >= 0);
        r10.w = r10.w ? 1.000000 : 0;
        r11.w = cmp(r10.z >= r8.z);
        r12.x = r11.w ? 1.000000 : 0;
        r10.y = -r10.y + r5.z;
        r11.w = cmp(abs(r10.y) >= 0.0299999993);
        r11.w = r11.w ? 1.000000 : 0;
        r12.y = r10.w * r11.w + r12.x;
        r11.yz = r12.xy * offsetsAndWeights[5].zz + r11.yz;
        r12.z = abs(r10.z);
        r12.w = r10.w * abs(r10.y) + r12.z;
        r5.yw = max(r12.zw, r5.yw);
        r10.yz = offsetsAndWeights[6].xy * r8.yy + v1.xy;
        r10.yz = min(uvClamp_g.xy, r10.yz);
        r12.xy = offsetsAndWeights[6].xy * r6.ww + v1.xy;
        r12.xy = min(uvClamp_g.xy, r12.xy);
        r13.z = outlinePrepareTexture.SampleLevel(samLinear_s, r10.yz, 0).x;
        r10.y = outlinePrepareTexture.SampleLevel(samLinear_s, r12.xy, 0).y;
        r10.z = dot(projInv_g._m22_m32, r13.zw);
        r10.w = dot(projInv_g._m23_m33, r13.zw);
        r10.z = r10.z / r10.w;
        r10.w = cmp(r10.y < 0);
        r10.x = (int)r10.x | (int)r10.w;
        r10.z = -r10.z + r3.z;
        r10.w = cmp(r10.z >= 0);
        r10.w = r10.w ? 1.000000 : 0;
        r11.w = cmp(r10.z >= r8.z);
        r12.x = r11.w ? 1.000000 : 0;
        r10.y = -r10.y + r5.z;
        r11.w = cmp(abs(r10.y) >= 0.0299999993);
        r11.w = r11.w ? 1.000000 : 0;
        r12.y = r10.w * r11.w + r12.x;
        r11.yz = r12.xy * offsetsAndWeights[6].zz + r11.yz;
        r12.z = abs(r10.z);
        r12.w = r10.w * abs(r10.y) + r12.z;
        r5.yw = max(r12.zw, r5.yw);
        r10.yz = offsetsAndWeights[7].xy * r8.yy + v1.xy;
        r10.yz = min(uvClamp_g.xy, r10.yz);
        r12.xy = offsetsAndWeights[7].xy * r6.ww + v1.xy;
        r12.xy = min(uvClamp_g.xy, r12.xy);
        r10.y = outlinePrepareTexture.SampleLevel(samLinear_s, r10.yz, 0).x;
        r6.w = outlinePrepareTexture.SampleLevel(samLinear_s, r12.xy, 0).y;
        r10.z = 1;
        r8.y = dot(projInv_g._m22_m32, r10.yz);
        r10.y = dot(projInv_g._m23_m33, r10.yz);
        r8.y = r8.y / r10.y;
        r10.y = cmp(r6.w < 0);
        r10.x = (int)r10.x | (int)r10.y;
        r3.z = -r8.y + r3.z;
        r8.y = cmp(r3.z >= 0);
        r8.y = r8.y ? 1.000000 : 0;
        r8.z = cmp(r3.z >= r8.z);
        r12.x = r8.z ? 1.000000 : 0;
        r5.z = -r6.w + r5.z;
        r6.w = cmp(abs(r5.z) >= 0.0299999993);
        r6.w = r6.w ? 1.000000 : 0;
        r12.y = r8.y * r6.w + r12.x;
        r12.xy = r12.xy * offsetsAndWeights[7].zz + r11.yz;
        r10.z = abs(r3.z);
        r10.w = r8.y * abs(r5.z) + r10.z;
        r12.zw = max(r10.zw, r5.yw);
        r5.yz = r10.xx ? r12.xz : r12.yw;
        r10.yw = (uint2)r2.zw >> int2(4,4);
        r10.xz = r2.zw;
        r10.xyzw = (int4)r10.xyzw & int4(15,15,15,15);
        r10.xyzw = (uint4)r10.xyzw;
        r9.w = 0.0666666701;
        r12.xyzw = r10.xyzw * r9.xyzw;
        r3.z = cmp(0 < r10.w);
        r5.z = min(r5.z, r8.w);
        r5.z = r5.z / r8.w;
        r5.z = log2(r5.z);
        r5.z = 1.25 * r5.z;
        r5.z = exp2(r5.z);
        r5.y = r5.y * r5.y;
        r5.y = r5.y * r12.w;
        r5.y = density_g * r5.y;
        r5.y = min(1, r5.y);
        r5.y = r5.y * r5.z;
        r5.z = 1 + -r11.x;
        r5.y = r5.y * r5.z;
        r8.yzw = r12.xyz * float3(0.0666666701,0.0666666701,0.0666666701) + -r9.xyz;
        r5.yzw = r5.yyy * r8.yzw + r9.xyz;
        r9.xyz = r3.zzz ? r5.yzw : r9.xyz;
      }
    } else {
      r3.z = 1 + -r5.x;
      r3.z = r8.x * r3.z;
      r5.yzw = r0.xyz * mapAOColor_g.xyz + -r0.xyz;
      r9.xyz = r3.zzz * r5.yzw + r0.xyz;
    }
    r5.yzw = r9.xyz * r7.www + r7.xyz;
    r5.yzw = r5.yzw + -r9.xyz;
    r5.yzw = combineAlpha_g * r5.yzw + r9.xyz;
    float3 charColor = r3.www ? r9.xyz : r5.yzw;
    // GTVBAO on characters — apply bitmask AO to character pixels
    if (shader_injection_data.char_gtvbao_mode > 0.5f
        && shader_injection_data.gtvbao_dedicated_bound > 0.5f) {
      float gtvbaoCharMask = saturate(shader_injection_data.char_gtvbao_mask_strength);
      float gtvbaoAO = lerp(r5.x, 1.0, gtvbaoCharMask);
      charColor *= gtvbaoAO;
    }
    // Probe ambient debug — show lightProbe_g[0] DC term (indoor/outdoor signal)
    if (shader_injection_data.vbgi_cascade_debug > 0.5f) {
      float3 probeAmbient = lightProbe_g[0].xyz;
      o0.xyz = probeAmbient * 0.5;
      o0.w = r0.w;
      o1.xyzw = r2.xyzw;
      o2.xy = r3.xy;
      return;
    }
    if (shader_injection_data.gtvbao_vbgi_bound > 0.5f) {
      float3 giColor = cachedVBGI;
      // Light Color debug view — shows sun color uniformly
      if ((int)shader_injection_data.vbgi_debug_view == 6) {
        o0.xyz = lightColor_g.xyz * 0.05;
        o0.w = r0.w;
        o1.xyzw = r2.xyzw;
        o2.xy = r3.xy;
        return;
      }
      // Character mask: reduce GI on characters by configured amount.
      giColor *= (1.0 - saturate(shader_injection_data.char_gtvbgi_mask_strength));
      if (shader_injection_data.gtvbao_vbgi_debug > 0.5f) {
        charColor = giColor;  // Debug: replace scene with GI texture
      } else {
        charColor += giColor;  // Normal: add GI to scene
      }
    }
    o0.xyz = charColor;
    o0.w = r0.w;
    o1.xyzw = r2.xyzw;
    o2.xy = r3.xy;
    return;
  }
  r3.zw = r4.xz * cloudShadowScale_g + cloudShadowOffset_g.xy;
  r3.zw = texCloudShadow.SampleLevel(SmplLinearWrap_s, r3.zw, 0).xw;
  r0.w = -1 + r3.z;
  r0.w = r3.w * r0.w + 1;
  r3.zw = min(shadowUVClamp_g.xy, v1.xy);
  r3.z = shadowTexture.SampleLevel(samLinear_s, r3.zw, 0).x;
  r3.w = min(r3.z, r0.w);
  r1.xy = (uint2)r1.xy;
  r8.zw = r1.xy * float2(3.05180438e-05,3.05180438e-05) + float2(-1,-1);
  r1.x = 3.14159274 * r8.z;
  sincos(r1.x, r1.x, r9.x);
  r1.y = r8.w * r8.w;
  r5.y = -r8.w * r8.w + 1;
  r5.y = sqrt(r5.y);
  r8.x = r9.x * r5.y;
  r8.y = r5.y * r1.x;
  r2.xyzw = (uint4)r2.xyzw;
  r9.xyzw = float4(0.00392156886,0.00392156886,0.00392156886,0.00392156886) * r2.wxyz;
  r2.yz = (uint2)r3.yy >> int2(5,10);
  r2.x = r3.y;
  r2.xyz = (int3)r2.xyz & int3(31,31,31);
  r2.xyz = (uint3)r2.xyz;
  r5.yzw = float3(0.0322580636,0.0322580636,0.0322580636) * r2.xyz;
  r1.x = min(0x00004e1f, (uint)r3.x);
  r10.x = deferredParams_g[r1.x].shadowColor.x;
  r10.y = deferredParams_g[r1.x].shadowColor.y;
  r10.z = deferredParams_g[r1.x].shadowColor.z;
  r10.w = deferredParams_g[r1.x].emissive;
  r11.x = deferredParams_g[r1.x].specularColor.x;
  r11.y = deferredParams_g[r1.x].specularColor.y;
  r11.z = deferredParams_g[r1.x].specularColor.z;
  r11.w = deferredParams_g[r1.x].rimLightPower;
  r12.x = deferredParams_g[r1.x].rimLightColor.x;
  r12.y = deferredParams_g[r1.x].rimLightColor.y;
  r12.z = deferredParams_g[r1.x].rimLightColor.z;
  r12.w = deferredParams_g[r1.x].rimIntensity;
  r13.x = deferredParams_g[r1.x].fresnels.x;
  r13.y = deferredParams_g[r1.x].fresnels.y;
  r13.z = deferredParams_g[r1.x].fresnels.z;
  r14.x = deferredParams_g[r1.x].specularGlossinesses.x;
  r14.y = deferredParams_g[r1.x].specularGlossinesses.y;
  r14.z = deferredParams_g[r1.x].specularGlossinesses.z;
  r14.w = deferredParams_g[r1.x].dynamicLightIntensity;
  r15.x = deferredParams_g[r1.x].materialFogIntensity;
  r15.y = deferredParams_g[r1.x].metalness;
  r15.z = deferredParams_g[r1.x].roughness;
  r15.w = deferredParams_g[r1.x].cryRefractionIndex;
  r16.x = deferredParams_g[r1.x].cryFresnel;
  r16.y = deferredParams_g[r1.x].cryBrightness;
  r16.z = deferredParams_g[r1.x].cryBrightnessPower;
  r16.w = deferredParams_g[r1.x].glowIntensity;
  r17.x = deferredParams_g[r1.x].glowLumThreshold;
  r17.y = deferredParams_g[r1.x].glowShadowFadeRatio;
  r17.z = deferredParams_g[r1.x].ssaoIntensity;
  r18.x = deferredParams_g[r1.x].ssrDistance;
  r18.y = deferredParams_g[r1.x].volumeFogIntensity;
  r18.z = deferredParams_g[r1.x].flag;
  r13.w = r14.x;
  r19.xz = r13.yz;
  r19.yw = r14.yz;
  r2.xy = r19.xy + -r13.xw;
  r2.xy = r5.yy * r2.xy + r13.xw;
  r3.xy = r19.zw + -r2.xy;
  r2.xy = r5.zz * r3.xy + r2.xy;
  r3.xy = lightTileSizeInv_g.xy * v0.xy;
  r3.xy = (uint2)r3.xy;
  r1.x = (uint)r3.y << 5;
  r3.y = (int)r3.x + (int)r1.x;
  r3.y = lightIndices_g[r3.y].tileDepthInv;
  r3.y = r3.y * -r1.w;
  r3.y = min(7, r3.y);
  r3.y = max(0, r3.y);
  r3.y = (uint)r3.y;
  r1.x = mad((int)r3.y, 576, (int)r1.x);
  r1.x = (int)r3.x + (int)r1.x;
  r1.x = min(4607, (uint)r1.x);
  r13.xyz = lightProbe_g[1].xyz * r8.xxx + lightProbe_g[0].xyz;
  r13.xyz = lightProbe_g[2].xyz * r8.yyy + r13.xyz;
  r13.xyz = lightProbe_g[3].xyz * r8.www + r13.xyz;
  r14.xyz = lightProbe_g[4].xyz * r8.www;
  r13.xyz = r14.xyz * r8.xxx + r13.xyz;
  r14.xyz = lightProbe_g[5].xyz * r8.yyy;
  r13.xyz = r14.xyz * r8.www + r13.xyz;
  r14.xyz = lightProbe_g[6].xyz * r8.yyy;
  r13.xyz = r14.xyz * r8.xxx + r13.xyz;
  r1.y = r1.y * 3 + -1;
  r13.xyz = lightProbe_g[7].xyz * r1.yyy + r13.xyz;
  r1.y = r8.y * r8.y;
  r1.y = r8.x * r8.x + -r1.y;
  r13.xyz = lightProbe_g[8].xyz * r1.yyy + r13.xyz;
  r13.xyz = max(float3(0,0,0), r13.xyz);
  r14.x = viewInv_g._m30 + -r4.x;
  r14.y = viewInv_g._m31 + -r4.y;
  r14.z = viewInv_g._m32 + -r4.z;
  r1.y = dot(r14.xyz, r14.xyz);
  r1.y = rsqrt(r1.y);
  r19.xyz = r14.xyz * r1.yyy;
  r10.xyz = sceneShadowColor_g.xyz + r10.xyz;
  r10.xyz = min(float3(1,1,1), r10.xyz);
  r3.x = r10.w * r5.w;
  r3.y = dot(r8.xyw, r19.xyz);
  r20.xyzw = (int4)r18.zzzz & int4(1,2,4,16);
  r3.w = r20.x ? r3.w : 1;
  r21.xyz = r14.xyz * r1.yyy + -lightDirection_g.xyz;
  r5.y = dot(r21.xyz, r21.xyz);
  r5.y = rsqrt(r5.y);
  r21.xyz = r21.xyz * r5.yyy;
  r5.y = lightSpecularGlossiness_g * r2.y;
  r5.z = saturate(dot(r21.xyz, r8.xyw));
  r5.y = max(0.00100000005, r5.y);
  r5.z = log2(r5.z);
  r5.y = r5.y * r5.z;
  r5.y = exp2(r5.y);
  // ── BRDF shared inputs ──
  float brdf_NdotV = saturate(dot(r8.xyw, r19.xyz));
  float brdf_roughness_src = r15.z;
  float brdf_roughness = clamp(brdf_roughness_src,
      shader_injection_data.brdf_roughness_min,
      shader_injection_data.brdf_roughness_max);
  float3 brdf_F0 = r9.xyz;
  float3 brdf_V = r19.xyz;
  bool brdf_use_ggx = shader_injection_data.brdf_multiscatter_specular_enabled > 0.5f;
  float brdf_specular_str = shader_injection_data.brdf_specular_strength;
  // ── BRDF Sun GGX ──
  float brdf_blinn_sun = r5.y;
  if (brdf_use_ggx) {
    float brdf_NdotH_sun = saturate(dot(r21.xyz, r8.xyw));
    float brdf_NdotL_sun = saturate(dot(r8.xyw, -lightDirection_g.xyz));
    float brdf_VdotH_sun = saturate(dot(r19.xyz, r21.xyz));
    float3 brdf_ggx_sun = GGX_Specular(brdf_NdotH_sun, brdf_NdotV, brdf_NdotL_sun,
                                       brdf_VdotH_sun, brdf_roughness, brdf_F0);
    brdf_ggx_sun *= MultiScatterCompensation(brdf_NdotV, brdf_NdotL_sun,
                                             brdf_roughness, brdf_F0);
    r5.y = lerp(brdf_blinn_sun, brdf_ggx_sun.x * brdf_NdotL_sun, brdf_specular_str);
  } else {
    r5.y = brdf_blinn_sun;
  }
  r5.y = r5.y * r3.w;
  r5.y = lightSpecularIntensity_g * r5.y;
  r5.y = r20.y ? r5.y : 0;
  r11.xyz = r5.yyy * r11.xyz;
  r11.xyz = lightColor_g.xyz * r11.xyz;
  // DynCube position-aware diagnostic (debug 11/12): carries the selected sample's
  // world position from histPos to the final output override.
  float3 dynCubeDiag = float3(0, 0, 0);
  bool dynCubeDiagActive = false;
  // SSR -> Dynamic -> Vanilla reflection source resolution.
  bool dynCubeNewSSRActive = shader_injection_data.dynCube_enabled > 0.5f
      && shader_injection_data.dynCube_force_vanilla < 0.5f
      && shader_injection_data.dynCube_ssr_enabled > 0.5f;
  bool dynCubeForceDynamicActive = shader_injection_data.dynCube_enabled > 0.5f
      && shader_injection_data.dynCube_force_vanilla < 0.5f
      && shader_injection_data.dynCube_force_dynamic > 0.5f;
  bool dynCubeForceSSRActive = shader_injection_data.dynCube_enabled > 0.5f
      && shader_injection_data.dynCube_force_vanilla < 0.5f
      && shader_injection_data.dynCube_force_ssr > 0.5f;
  bool dynCubeReflResolveActive = dynCubeNewSSRActive || dynCubeForceDynamicActive || dynCubeForceSSRActive;
  float3 dynCubeReflDir = float3(0, 0, 0);
  float dynCubeReflMip = 0.0;
  bool dynCubeReflActive = false;
  int dynCubeReflSrc = 1;  // 0=SSR, 1=Dynamic, 2=Vanilla (debug 16)
  if (r20.z != 0) {
    r5.yz = r15.yz * r9.yz;
    r6.w = (int)r18.z & 32;
    if (r6.w != 0) {
      r21.y = resolutionScaling_g.y + -v1.y;
      r21.x = v1.x;
      r21.xyz = texMirror_g.SampleLevel(SmplLinearClamp_s, r21.xy, 0).xyz;
    } else {
      r8.z = r3.y + r3.y;
      r22.xyz = r8.xyw * -r8.zzz + r19.xyz;
      float3 dynCubeR = r22.xyz;  // world reflection dir (pre flip) for debug 12 ray point
      // Parallax-corrected cubemap lookup (generic Falcom Engine+). Active only for the
      // dynamic cube (enabled + not force-vanilla) so the vanilla path is untouched.
      int parallaxFace = -1;
      if (shader_injection_data.dynCube_enabled > 0.5f
          && shader_injection_data.dynCube_force_vanilla < 0.5f
          && shader_injection_data.dynCube_parallax_enabled > 0.5f) {
        float3 parallaxDir;
        if (DynCubeParallaxCorrect(r4.xyz, r22.xyz, viewInv_g._m30_m31_m32,
            float3(shader_injection_data.dynCube_parallax_box_size_x,
                   shader_injection_data.dynCube_parallax_box_size_y,
                   shader_injection_data.dynCube_parallax_box_size_z),
            parallaxDir, parallaxFace)) {
          r22.xyz = parallaxDir;
        }
      }
      texEnvMap_g.GetDimensions(0, width, height, num_levels);
      r22.xyz = float3(1,-1,-1) * r22.xyz;
      r8.z = (float)(num_levels - 1);
      r8.z = r8.z * r5.z;
      // Debug: Force Cubemap Mip (dynCube_force_mip >= 0) — bypass roughness LOD to verify the GGX chain.
      if (shader_injection_data.dynCube_force_mip > -0.5f) {
        r8.z = clamp(shader_injection_data.dynCube_force_mip, 0.0, (float)(num_levels - 1));
      }
      // Position-aware lookup: refine the sample direction using stored histPos world
      // positions scored against the reflection ray. Only for the dynamic cube path.
      float3 dynCubeFinalDir = r22.xyz;
      if (shader_injection_data.dynCube_enabled > 0.5f
          && shader_injection_data.dynCube_force_vanilla < 0.5f
          && shader_injection_data.dynCube_debug != 4.f
          && shader_injection_data.dynCube_pos_aware_enabled > 0.5f
          && shader_injection_data.dynCube_pos_aware_strength > 0.001f) {
        float3 paDir;
        float paStrength = clamp(shader_injection_data.dynCube_pos_aware_strength, 0.0, 1.0);
        uint paCount = (shader_injection_data.dynCube_pos_aware_samples < 0.5) ? 4u
                     : (shader_injection_data.dynCube_pos_aware_samples < 1.5) ? 8u : 16u;
        if (DynCubePosAwareLookup(dynCubeHistPosTex, samPoint_s, r4.xyz, dynCubeR, r22.xyz,
                                  paCount, shader_injection_data.dynCube_pos_aware_spread, paDir)) {
          dynCubeFinalDir = normalize(lerp(r22.xyz, paDir, paStrength));
        }
      }
      r21.xyz = texEnvMap_g.SampleLevel(SmplCube_s, dynCubeFinalDir, r8.z).xyz;
      // Record the reflection direction + mip for the SSR -> Dynamic -> Vanilla resolution.
      dynCubeReflDir = dynCubeFinalDir;
      dynCubeReflMip = r8.z;
      dynCubeReflActive = true;
      // Parallax debug: tint by the probe-box exit face (only on a valid box hit).
      if (shader_injection_data.dynCube_parallax_debug > 0.5f && parallaxFace >= 0) {
        r21.xyz = DynCubeParallaxFaceColor(parallaxFace);
      }
      // DynCube position-aware diagnostic (debug 11/12/13): histPos of the selected sample.
      // 11 = original direction's histPos, 12 = original position error, 13 = corrected direction's histPos.
      if (shader_injection_data.dynCube_enabled > 0.5f
          && shader_injection_data.dynCube_force_vanilla < 0.5f
          && (shader_injection_data.dynCube_debug == 11.f || shader_injection_data.dynCube_debug == 12.f
              || shader_injection_data.dynCube_debug == 13.f)) {
        float4 hp = (shader_injection_data.dynCube_debug == 13.f)
            ? dynCubeHistPosTex.SampleLevel(samPoint_s, dynCubeFinalDir, 0)
            : dynCubeHistPosTex.SampleLevel(SmplCube_s, r22.xyz, 0);
        float3 sampleWorld = hp.xyz * 1000.0;  // un-scale stored pos (g_posScale = 0.001)
        if (hp.a < 0.5) {
          dynCubeDiag = float3(1.0, 0.0, 1.0);  // magenta = no captured sample
        } else if (shader_injection_data.dynCube_debug == 11.f || shader_injection_data.dynCube_debug == 13.f) {
          float3 relPos = sampleWorld - viewInv_g._m30_m31_m32;
          dynCubeDiag = saturate(relPos * (0.5 / 50.0) + 0.5);  // ±50 world units -> [0,1]
        } else {
          float3 rayPoint = r4.xyz + dynCubeR * 10.0;  // probe distance 10 world units
          float err = length(rayPoint - sampleWorld);
          dynCubeDiag = lerp(float3(0,1,0), float3(1,0,0), saturate(err / 30.0));  // green=close, red=far
        }
        dynCubeDiagActive = true;
      }
    }
    if (!dynCubeReflResolveActive) {
      // ── Existing vanilla/custom SSR eligibility + blend (kept when no new SSR/force path) ──
      r8.z = (int)r1.z & 2;
      bool ssrVanillaElig = (r8.z != 0);
      bool ssrCustomElig = (r15.z <= shader_injection_data.ssr_roughness_threshold);
      bool ssrUseCustom = shader_injection_data.ssr_apply > 0.5f
                       && shader_injection_data.ssr_custom_bound > 0.5f
                       && ((shader_injection_data.ssr_eligibility_mode < 0.5f) ? ssrVanillaElig
                         : (shader_injection_data.ssr_eligibility_mode > 1.5f) ? true
                         : ssrCustomElig);
      if (ssrUseCustom) {
        r20.xz = resolutionScaling_g.xy * v1.zw;
        r22.xyzw = ssrCustomTexture.SampleLevel(SmplLinearClamp_s, r20.xz, 0).xyzw;
        r22.w = r22.w * shader_injection_data.ssr_apply_gain;
      } else if (ssrVanillaElig) {
        r20.xz = resolutionScaling_g.xy * v1.zw;
        r22.xyzw = texSSRMap_g.SampleLevel(SmplLinearClamp_s, r20.xz, 0).xyzw;
      } else {
        r22.xyzw = float4(0, 0, 0, 0);   // no SSR contribution on this pixel
      }
      // Environment-vs-SSR replacement blend (AMD SSSR fallback model):
      // α=0 → cubemap unchanged · α=1 → SSR replaces · intermediate blends.
      r22.xyz = r22.xyz + -r21.xyz;
      r21.xyz = r22.www * r22.xyz + r21.xyz;
    } else {
      // ── SSR > Dynamic > Vanilla resolution (new simple SSR path, with force toggles) ──
      if (dynCubeReflActive) {
        if (dynCubeForceDynamicActive) {
          // Force Dynamic: keep the dynamic cube sample (r21).
          dynCubeReflSrc = 1;
        } else if (dynCubeForceSSRActive && dynCubeNewSSRActive) {
          // Force SSR: use the SSR color directly.
          float2 ssrUV = resolutionScaling_g.xy * v1.zw;
          float4 ssrTap = dynCubeSSRTex.SampleLevel(SmplLinearClamp_s, ssrUV, 0);
          r21.xyz = ssrTap.rgb;
          dynCubeReflSrc = 0;
        } else if (dynCubeNewSSRActive) {
          // Confidence-based blend.
          float2 ssrUV = resolutionScaling_g.xy * v1.zw;
          float4 ssrTap = dynCubeSSRTex.SampleLevel(SmplLinearClamp_s, ssrUV, 0);
          float ssrWeight = saturate(ssrTap.a);
          float4 hpV = dynCubeHistPosTex.SampleLevel(samPoint_s, dynCubeReflDir, 0);
          float dynamicConf = (hpV.a > 0.5f) ? 1.0 : 0.0;  // existing binary dynamic validity
          float dynamicWeight = (1.0 - ssrWeight) * dynamicConf;
          float vanillaWeight = 1.0 - ssrWeight - dynamicWeight;
          float3 vanillaCol = dynCubeVanillaTex.SampleLevel(SmplCube_s, dynCubeReflDir,
              dynCubeReflMip + shader_injection_data.dynCube_vanilla_blur).xyz;
          r21.xyz = ssrTap.rgb * ssrWeight + r21.xyz * dynamicWeight + vanillaCol * vanillaWeight;
          dynCubeReflSrc = (ssrWeight >= dynamicWeight && ssrWeight >= vanillaWeight) ? 0
                         : (dynamicWeight >= vanillaWeight) ? 1 : 2;
        }
      }
    }
    r8.z = cmp(0 < r2.x);
    r9.y = 1 + -abs(r3.y);
    r9.y = max(0, r9.y);
    r9.y = log2(r9.y);
    r2.x = r9.y * r2.x;
    r2.x = exp2(r2.x);
    r2.x = r8.z ? r2.x : 1;
    r8.z = r5.y * r2.x;
    r22.xyz = r0.xyz * r21.xyz + -r0.xyz;
    r22.xyz = r8.zzz * r22.xyz + r0.xyz;
    r8.z = dot(r21.xyz, float3(0.298999995,0.587000012,0.114));
    r5.z = r5.z * -9 + 10;
    r8.z = log2(r8.z);
    r5.z = r8.z * r5.z;
    r5.z = exp2(r5.z);
    r8.z = 1 + -r5.z;
    r5.y = r5.y * r8.z + r5.z;
    r21.xyz = r21.xyz * r5.yyy;
    r21.xyz = r21.xyz * r2.xxx;
    r2.x = -r9.z * r15.z + 1;
    r21.xyz = r21.xyz * r2.xxx + r11.xyz;
    r11.xyz = r6.www ? r11.xyz : r21.xyz;
  } else {
    r2.x = (int)r18.z & 8;
    if (r2.x != 0) {
      r2.x = r3.y + r3.y;
      r21.xyz = r8.xyw * -r2.xxx + r19.xyz;
      float3 dynCubeR2 = r21.xyz;  // world reflection dir (pre flip) for debug 12 ray point
      // Parallax-corrected cubemap lookup (generic Falcom Engine+). Active only for the
      // dynamic cube (enabled + not force-vanilla) so the vanilla path is untouched.
      int parallaxFace2 = -1;
      if (shader_injection_data.dynCube_enabled > 0.5f
          && shader_injection_data.dynCube_force_vanilla < 0.5f
          && shader_injection_data.dynCube_parallax_enabled > 0.5f) {
        float3 parallaxDir2;
        if (DynCubeParallaxCorrect(r4.xyz, r21.xyz, viewInv_g._m30_m31_m32,
            float3(shader_injection_data.dynCube_parallax_box_size_x,
                   shader_injection_data.dynCube_parallax_box_size_y,
                   shader_injection_data.dynCube_parallax_box_size_z),
            parallaxDir2, parallaxFace2)) {
          r21.xyz = parallaxDir2;
        }
      }
      r2.x = 1 / r15.w;
      r5.y = dot(-r19.xyz, r8.xyw);
      r5.z = r2.x * r2.x;
      r6.w = -r5.y * r5.y + 1;
      r5.z = -r5.z * r6.w + 1;
      r6.w = sqrt(r5.z);
      r5.y = r2.x * r5.y + r6.w;
      r5.z = cmp(r5.z >= 0);
      r23.xyz = r5.yyy * r8.xyw;
      r19.xyz = r2.xxx * -r19.xyz + -r23.xyz;
      r19.xyz = r5.zzz ? r19.xyz : 0;
      r2.x = r15.z * r9.z;
      texEnvMap_g.GetDimensions(0, width, height, num_levels);
      r21.xyz = float3(1,-1,-1) * r21.xyz;
      r5.y = (float)(num_levels - 1);
      r2.x = r5.y * r2.x;
      // Debug: Force Cubemap Mip (dynCube_force_mip >= 0) — bypass roughness LOD to verify the GGX chain.
      if (shader_injection_data.dynCube_force_mip > -0.5f) {
        r2.x = clamp(shader_injection_data.dynCube_force_mip, 0.0, (float)(num_levels - 1));
      }
      // Position-aware lookup: refine the sample direction using stored histPos world
      // positions scored against the reflection ray. Only for the dynamic cube path.
      float3 dynCubeFinalDir2 = r21.xyz;
      if (shader_injection_data.dynCube_enabled > 0.5f
          && shader_injection_data.dynCube_force_vanilla < 0.5f
          && shader_injection_data.dynCube_debug != 4.f
          && shader_injection_data.dynCube_pos_aware_enabled > 0.5f
          && shader_injection_data.dynCube_pos_aware_strength > 0.001f) {
        float3 paDir2;
        float paStrength = clamp(shader_injection_data.dynCube_pos_aware_strength, 0.0, 1.0);
        uint paCount = (shader_injection_data.dynCube_pos_aware_samples < 0.5) ? 4u
                     : (shader_injection_data.dynCube_pos_aware_samples < 1.5) ? 8u : 16u;
        if (DynCubePosAwareLookup(dynCubeHistPosTex, samPoint_s, r4.xyz, dynCubeR2, r21.xyz,
                                  paCount, shader_injection_data.dynCube_pos_aware_spread, paDir2)) {
          dynCubeFinalDir2 = normalize(lerp(r21.xyz, paDir2, paStrength));
        }
      }
      float3 dynCubeSampleDir2 = r21.xyz;  // original flipped sample dir for debug 11/12 histPos lookup
      r21.xyz = texEnvMap_g.SampleLevel(SmplCube_s, dynCubeFinalDir2, r2.x).xyz;
      // Record the reflection direction + mip for the SSR -> Dynamic -> Vanilla resolution.
      dynCubeReflDir = dynCubeFinalDir2;
      dynCubeReflMip = r2.x;
      dynCubeReflActive = true;
      // Parallax debug: tint by the probe-box exit face (only on a valid box hit).
      if (shader_injection_data.dynCube_parallax_debug > 0.5f && parallaxFace2 >= 0) {
        r21.xyz = DynCubeParallaxFaceColor(parallaxFace2);
      }
      // DynCube position-aware diagnostic (debug 11/12/13): histPos of the selected sample.
      if (shader_injection_data.dynCube_enabled > 0.5f
          && shader_injection_data.dynCube_force_vanilla < 0.5f
          && (shader_injection_data.dynCube_debug == 11.f || shader_injection_data.dynCube_debug == 12.f
              || shader_injection_data.dynCube_debug == 13.f)) {
        float4 hp = (shader_injection_data.dynCube_debug == 13.f)
            ? dynCubeHistPosTex.SampleLevel(samPoint_s, dynCubeFinalDir2, 0)
            : dynCubeHistPosTex.SampleLevel(SmplCube_s, dynCubeSampleDir2, 0);
        float3 sampleWorld = hp.xyz * 1000.0;  // un-scale stored pos (g_posScale = 0.001)
        if (hp.a < 0.5) {
          dynCubeDiag = float3(1.0, 0.0, 1.0);  // magenta = no captured sample
        } else if (shader_injection_data.dynCube_debug == 11.f || shader_injection_data.dynCube_debug == 13.f) {
          float3 relPos = sampleWorld - viewInv_g._m30_m31_m32;
          dynCubeDiag = saturate(relPos * (0.5 / 50.0) + 0.5);  // ±50 world units -> [0,1]
        } else {
          float3 rayPoint = r4.xyz + dynCubeR2 * 10.0;  // probe distance 10 world units
          float err = length(rayPoint - sampleWorld);
          dynCubeDiag = lerp(float3(0,1,0), float3(1,0,0), saturate(err / 30.0));  // green=close, red=far
        }
        dynCubeDiagActive = true;
      }
      if (!dynCubeReflResolveActive) {
        // ── Existing vanilla/custom SSR eligibility + blend (site 2, kept when no new SSR/force path) ──
        r5.y = (int)r1.z & 2;
        bool ssrVanillaElig2 = (r5.y != 0);
        bool ssrCustomElig2 = (r15.z <= shader_injection_data.ssr_roughness_threshold);
        bool ssrUseCustom2 = shader_injection_data.ssr_apply > 0.5f
                          && shader_injection_data.ssr_custom_bound > 0.5f
                          && ((shader_injection_data.ssr_eligibility_mode < 0.5f) ? ssrVanillaElig2
                            : (shader_injection_data.ssr_eligibility_mode > 1.5f) ? true
                            : ssrCustomElig2);
        if (ssrUseCustom2) {
          r5.yz = resolutionScaling_g.xy * v1.zw;
          r23.xyzw = ssrCustomTexture.SampleLevel(SmplLinearClamp_s, r5.yz, 0).xyzw;
          r23.w = r23.w * shader_injection_data.ssr_apply_gain;
        } else if (ssrVanillaElig2) {
          r5.yz = resolutionScaling_g.xy * v1.zw;
          r23.xyzw = texSSRMap_g.SampleLevel(SmplLinearClamp_s, r5.yz, 0).xyzw;
        } else {
          r23.xyzw = float4(0, 0, 0, 0);
        }
        // Environment-vs-SSR replacement blend (site 2).
        r23.xyz = r23.xyz + -r21.xyz;
        r21.xyz = r23.www * r23.xyz + r21.xyz;
      } else {
        // ── SSR > Dynamic > Vanilla resolution (site 2, with force toggles) ──
        if (dynCubeReflActive) {
          if (dynCubeForceDynamicActive) {
            // Force Dynamic: keep the dynamic cube sample (r21).
            dynCubeReflSrc = 1;
          } else if (dynCubeForceSSRActive && dynCubeNewSSRActive) {
            // Force SSR: use the SSR color directly.
            float2 ssrUV2 = resolutionScaling_g.xy * v1.zw;
            float4 ssrTap2 = dynCubeSSRTex.SampleLevel(SmplLinearClamp_s, ssrUV2, 0);
            r21.xyz = ssrTap2.rgb;
            dynCubeReflSrc = 0;
          } else if (dynCubeNewSSRActive) {
            // Confidence-based blend.
            float2 ssrUV2 = resolutionScaling_g.xy * v1.zw;
            float4 ssrTap2 = dynCubeSSRTex.SampleLevel(SmplLinearClamp_s, ssrUV2, 0);
            float ssrWeight = saturate(ssrTap2.a);
            float4 hpV2 = dynCubeHistPosTex.SampleLevel(samPoint_s, dynCubeReflDir, 0);
            float dynamicConf = (hpV2.a > 0.5f) ? 1.0 : 0.0;
            float dynamicWeight = (1.0 - ssrWeight) * dynamicConf;
            float vanillaWeight = 1.0 - ssrWeight - dynamicWeight;
            float3 vanillaCol2 = dynCubeVanillaTex.SampleLevel(SmplCube_s, dynCubeReflDir,
                dynCubeReflMip + shader_injection_data.dynCube_vanilla_blur).xyz;
            r21.xyz = ssrTap2.rgb * ssrWeight + r21.xyz * dynamicWeight + vanillaCol2 * vanillaWeight;
            dynCubeReflSrc = (ssrWeight >= dynamicWeight && ssrWeight >= vanillaWeight) ? 0
                           : (dynamicWeight >= vanillaWeight) ? 1 : 2;
          }
        }
      }
      r19.xyz = texEnvMap_g.SampleLevel(SmplCube_s, r19.xyz, r2.x).xyz;
      r2.x = cmp(0 < r16.x);
      r5.y = 1 + -abs(r3.y);
      r5.y = max(0, r5.y);
      r5.y = log2(r5.y);
      r5.y = r16.x * r5.y;
      r5.y = exp2(r5.y);
      r2.x = r2.x ? r5.y : 1;
      r23.xyz = r0.xyz * r21.xyz + -r0.xyz;
      r23.xyz = r2.xxx * r23.xyz + r0.xyz;
      r2.w = -r2.w * 0.00392156886 + 1;
      r9.x = r2.x * r2.w + r9.x;
      r2.w = abs(r3.y) * r16.y;
      r2.w = log2(r2.w);
      r2.w = r16.z * r2.w;
      r2.w = exp2(r2.w);
      r16.xyz = r9.xxx * r21.xyz;
      r16.xyz = r16.xyz * r2.xxx;
      r2.x = -r9.z * r15.z + 1;
      r11.xyz = r16.xyz * r2.xxx + r11.xyz;
      r15.yzw = r2.www * r19.xyz;
      r22.xyz = r23.xyz * r15.yzw;
    } else {
      r22.xyz = r0.xyz;
    }
  }
  r2.x = r9.x * r3.w;
  r15.yzw = float3(1,1,1) + -r10.xyz;
  r10.xyz = r2.xxx * r15.yzw + r10.xyz;
  r2.x = log2(r3.z);
  r2.x = shadowEdgeSharpness_g * r2.x;
  r2.x = exp2(r2.x);
  if (shader_injection_data.shadow_edge_tint > 1.5f) {
    // Improved: vibrancy boost in penumbra region
    float penumbra = 1.0f - 2.0f * abs(r3.z - 0.5f);
    penumbra = saturate(penumbra / shader_injection_data.shadow_penumbra_detection);

    // Vibrancy source: colorTexture (r0.xyz) — has full scene color.
    float3 surfaceColor = r0.xyz;
    float luma = dot(surfaceColor, float3(0.333f, 0.333f, 0.333f));
    float maxC = max(surfaceColor.x, max(surfaceColor.y, surfaceColor.z));
    float vibrance = shader_injection_data.shadow_penumbra_vibrance;
    float3 surfaceVibrancy;
    if (vibrance <= 1.0f) {
      surfaceVibrancy = luma + vibrance * (surfaceColor - luma);
    } else {
      float vibranceFactor = 1.0f + (vibrance - 1.0f) * (1.0f - maxC);
      surfaceVibrancy = luma + vibranceFactor * (surfaceColor - luma);
    }
    surfaceVibrancy = saturate(surfaceVibrancy);

    // Vibrancy-adjusted Falcom shadowEdgeColor
    float edgeLuma = dot(shadowEdgeColor_g.xyz, float3(0.333f, 0.333f, 0.333f));
    float edgeMaxC = max(shadowEdgeColor_g.x, max(shadowEdgeColor_g.y, shadowEdgeColor_g.z));
    float edgeVibrance = shader_injection_data.shadow_penumbra_edge_vibrance;
    float3 edgeVibrancy;
    if (edgeVibrance <= 1.0f) {
      edgeVibrancy = edgeLuma + edgeVibrance * (shadowEdgeColor_g.xyz - edgeLuma);
    } else {
      float edgeFactor = 1.0f + (edgeVibrance - 1.0f) * (1.0f - edgeMaxC);
      edgeVibrancy = edgeLuma + edgeFactor * (shadowEdgeColor_g.xyz - edgeLuma);
    }
    edgeVibrancy = saturate(edgeVibrancy);

    // Vibrancy-adjusted lightColor (sun color)
    float lightLuma = dot(lightColor_g.xyz, float3(0.333f, 0.333f, 0.333f));
    float lightMaxC = max(lightColor_g.x, max(lightColor_g.y, lightColor_g.z));
    float lightSaturation = shader_injection_data.shadow_penumbra_lightcolor_saturation;
    float3 lightVibrancy;
    if (lightSaturation <= 1.0f) {
      lightVibrancy = lightLuma + lightSaturation * (lightColor_g.xyz - lightLuma);
    } else {
      float lightFactor = 1.0f + (lightSaturation - 1.0f) * (1.0f - lightMaxC);
      lightVibrancy = lightLuma + lightFactor * (lightColor_g.xyz - lightLuma);
    }
    lightVibrancy = saturate(lightVibrancy);

    // Blend between surface vibrancy and Falcom edge vibrancy
    float3 tintColor = lerp(surfaceVibrancy, edgeVibrancy, shader_injection_data.shadow_penumbra_falcom_blend);

    // Blend toward lightColor vibrancy
    tintColor = lerp(tintColor, lightVibrancy, shader_injection_data.shadow_penumbra_lightcolor_blend);

    // Apply brightness
    tintColor *= shader_injection_data.shadow_penumbra_color_brightness;
    tintColor = saturate(tintColor);

    // Blend vibrancy color onto the shadow-edge-processed r10.xyz
    float strength = shader_injection_data.shadow_penumbra_color_strength * penumbra;
    float3 finalColor = lerp(r10.xyz, tintColor, strength);

    // Debug views — replace output and return early
    // o1/o2 semantics differ in Sora 2nd (glow mask / SSR distance) — approximated.
    static const float kDebugHdrScale = 0.05;
    int debugMode = (int)shader_injection_data.shadow_penumbra_debug_view;
    if (debugMode == 1) {
      o0.rgb = float3(penumbra, penumbra, penumbra) * kDebugHdrScale;
      o0.a = 1.0; o1.xyzw = uint4(0, 255 * saturate(0.1 * r16.w), 0, 0); o2.x = 0; o2.y = min(0xffff, (uint)(65.535 * r18.x)); return;
    }
    if (debugMode == 2) {
      o0.rgb = tintColor * kDebugHdrScale;
      o0.a = 1.0; o1.xyzw = uint4(0, 255 * saturate(0.1 * r16.w), 0, 0); o2.x = 0; o2.y = min(0xffff, (uint)(65.535 * r18.x)); return;
    }
    if (debugMode == 3) {
      o0.rgb = finalColor * kDebugHdrScale;
      o0.a = 1.0; o1.xyzw = uint4(0, 255 * saturate(0.1 * r16.w), 0, 0); o2.x = 0; o2.y = min(0xffff, (uint)(65.535 * r18.x)); return;
    }
    if (debugMode == 4) {
      // Sun color (lightColor_g from cb_scene c25) applied uniformly
      o0.rgb = lightColor_g.xyz * kDebugHdrScale;
      o0.a = 1.0; o1.xyzw = uint4(0, 255 * saturate(0.1 * r16.w), 0, 0); o2.x = 0; o2.y = min(0xffff, (uint)(65.535 * r18.x)); return;
    }

    r15.yzw = finalColor;
  } else if (shader_injection_data.shadow_edge_tint > 0.5f) {
    r15.yzw = shadowEdgeColor_g.xyz * r0.www;
    r16.xyz = -shadowEdgeColor_g.xyz * r0.www + r10.xyz;
    r15.yzw = r3.zzz * r16.xyz + r15.yzw;
  } else {
    r15.yzw = r10.xyz;
  }
  r0.w = r2.x * r9.x;
  r9.xyz = r15.yzw + -r10.xyz;
  r9.xyz = r0.www * r9.xyz + r10.xyz;
  r9.xyz = r9.xyz * lightColor_g.xyz + r13.xyz;
  r0.w = min(1, r3.x);
  r10.xyz = float3(1,1,1) + -r9.xyz;
  r9.xyz = r0.www * r10.xyz + r9.xyz;
  r0.w = 1 + -abs(r3.y);
  r0.w = max(0, r0.w);
  r0.w = r0.w * r12.w;
  r0.w = log2(r0.w);
  r0.w = r11.w * r0.w;
  r0.w = exp2(r0.w);
  r0.w = min(1, r0.w);
  r3.xyz = r12.xyz * r0.www + r11.xyz;
  if (r20.y != 0) {
    r0.w = lightIndices_g[r1.x].pointLightCount;
    r0.w = min(63, (uint)r0.w);
    r10.xyz = float3(0,0,0);
    r11.xyz = float3(0,0,0);
    r2.x = 0;
    while (true) {
      r2.w = cmp((uint)r2.x >= (uint)r0.w);
      if (r2.w != 0) break;
      r2.w = lightIndices_g[r1.x].pointLightIndices[r2.x];
      r12.x = dynamicLights_g[r2.w].pos.x;
      r12.y = dynamicLights_g[r2.w].pos.y;
      r12.z = dynamicLights_g[r2.w].pos.z;
      r12.xyz = r12.xyz + -r4.xyz;
      r5.y = dot(r12.xyz, r12.xyz);
      r5.z = sqrt(r5.y);
      r6.w = dynamicLights_g[r2.w].radiusInv;
      r5.z = r6.w * r5.z;
      r6.w = dynamicLights_g[r2.w].attenuation;
      r5.z = log2(abs(r5.z));
      r5.z = r6.w * r5.z;
      r5.z = exp2(r5.z);
      r5.z = 1 + -r5.z;
      r5.z = max(0, r5.z);
      r6.w = cmp(0 < r5.z);
      if (r6.w != 0) {
        r5.y = rsqrt(r5.y);
        r12.xyz = r12.xyz * r5.yyy;
        r5.y = dynamicLights_g[r2.w].translucency;
        r6.w = dot(r12.xyz, r8.xyw);
        r5.y = max(r6.w, r5.y);
        r5.y = r5.z * r5.y;
        r13.x = dynamicLights_g[r2.w].color.x;
        r13.y = dynamicLights_g[r2.w].color.y;
        r13.z = dynamicLights_g[r2.w].color.z;
        r11.xyz = r13.xyz * r5.yyy + r11.xyz;
        // ── BRDF Point Light ──
        float brdf_rawNdotL_pt = r6.w;
        float3 brdf_H_pt = normalize(brdf_V + r12.xyz);
        float brdf_NdotH_pt = saturate(dot(brdf_H_pt, r8.xyw));
        float brdf_VdotH_pt = saturate(dot(brdf_V, brdf_H_pt));
        r12.xyz = r14.xyz * r1.yyy + r12.xyz;
        r5.z = dot(r12.xyz, r12.xyz);
        r5.z = rsqrt(r5.z);
        r12.xyz = r12.xyz * r5.zzz;
        r15.y = dynamicLights_g[r2.w].specularIntensity;
        r15.z = dynamicLights_g[r2.w].specularGlossiness;
        r2.w = r15.z * r2.y;
        r5.z = saturate(dot(r12.xyz, r8.xyw));
        r2.w = max(0.00100000005, r2.w);
        r5.z = log2(r5.z);
        r2.w = r5.z * r2.w;
        r2.w = exp2(r2.w);
        if (brdf_use_ggx) {
          float3 brdf_ggx_spec_pt = GGX_Specular(brdf_NdotH_pt, brdf_NdotV, brdf_rawNdotL_pt, brdf_VdotH_pt, brdf_roughness, brdf_F0);
          brdf_ggx_spec_pt *= MultiScatterCompensation(brdf_NdotV, brdf_rawNdotL_pt, brdf_roughness, brdf_F0);
          float brdf_ggx_scalar_pt = brdf_ggx_spec_pt.x * brdf_rawNdotL_pt;
          r2.w = lerp(r2.w, brdf_ggx_scalar_pt, brdf_specular_str);
          r2.w = max(r2.w, 0.0f);
        }
        r12.xyz = r13.xyz * r2.www;
        r12.xyz = r12.xyz * r5.yyy;
        r10.xyz = r12.xyz * r15.yyy + r10.xyz;
      }
      r2.x = (int)r2.x + 1;
    }
    r11.xyz = r11.xyz * r14.www + r9.xyz;
    r0.w = lightIndices_g[r1.x].spotLightCount;
    r0.w = min(63, (uint)r0.w);
    r12.yw = localShadowResolutionInv_g * float2(1.25,-1.25);
    r12.xz = float2(0,0);
    r13.xyz = r10.xyz;
    r15.yzw = float3(0,0,0);
    r2.x = 0;
    while (true) {
      r2.w = cmp((uint)r2.x >= (uint)r0.w);
      if (r2.w != 0) break;
      r2.w = lightIndices_g[r1.x].spotLightIndices[r2.x];
      r16.x = dynamicLights_g[r2.w].pos.x;
      r16.y = dynamicLights_g[r2.w].pos.y;
      r16.z = dynamicLights_g[r2.w].pos.z;
      r16.xyz = r16.xyz + -r4.xyz;
      r5.y = dot(r16.xyz, r16.xyz);
      r5.z = rsqrt(r5.y);
      r16.xyz = r16.xyz * r5.zzz;
      r19.x = dynamicLights_g[r2.w].vec.x;
      r19.y = dynamicLights_g[r2.w].vec.y;
      r19.z = dynamicLights_g[r2.w].vec.z;
      r19.w = dynamicLights_g[r2.w].spotAngleInv;
      r5.z = dot(r16.xyz, r19.xyz);
      r5.z = max(0, r5.z);
      r5.z = 1 + -r5.z;
      r5.z = r5.z * r19.w;
      r6.w = dynamicLights_g[r2.w].attenuationAngle;
      r5.z = log2(r5.z);
      r5.z = r6.w * r5.z;
      r5.z = exp2(r5.z);
      r5.z = 1 + -r5.z;
      r5.z = max(0, r5.z);
      r6.w = cmp(0 < r5.z);
      if (r6.w != 0) {
        r5.y = sqrt(r5.y);
        r6.w = dynamicLights_g[r2.w].radiusInv;
        r5.y = r6.w * r5.y;
        r6.w = dynamicLights_g[r2.w].attenuation;
        r5.y = log2(abs(r5.y));
        r5.y = r6.w * r5.y;
        r5.y = exp2(r5.y);
        r5.y = 1 + -r5.y;
        r5.y = max(0, r5.y);
        r5.y = r5.z * r5.y;
        r5.z = cmp(0 < r5.y);
        if (r5.z != 0) {
          r19.x = dynamicLights_g[r2.w].translucency;
          r19.y = dynamicLights_g[r2.w].shadowmapIndex;
          r5.z = cmp((int)r19.y != -1);
          if (r5.z != 0) {
            r21.xyzw = spotShadowMatrices_g[r19.y]._m00_m10_m20_m30;
            r23.xyzw = spotShadowMatrices_g[r19.y]._m01_m11_m21_m31;
            r24.xyzw = spotShadowMatrices_g[r19.y]._m02_m12_m22_m32;
            r25.xyzw = spotShadowMatrices_g[r19.y]._m03_m13_m23_m33;
            r20.x = dot(r4.xyzw, r21.xyzw);
            r20.y = dot(r4.xyzw, r23.xyzw);
            r20.z = dot(r4.xyzw, r24.xyzw);
            r5.z = dot(r4.xyzw, r25.xyzw);
            r21.xyz = r20.xyz / r5.zzz;
            r21.w = (uint)r19.y;
            r5.z = spotShadowMaps.SampleCmpLevelZero(SmplShadow_s, r21.xyw, r21.z).x;
            r20.xy = r21.xy + r12.yx;
            r20.z = r21.w;
            r6.w = spotShadowMaps.SampleCmpLevelZero(SmplShadow_s, r20.xyz, r21.z).x;
            r6.w = 0.200000003 * r6.w;
            r5.z = r5.z * 0.200000003 + r6.w;
            r20.xy = r21.xy + r12.wz;
            r6.w = spotShadowMaps.SampleCmpLevelZero(SmplShadow_s, r20.xyz, r21.z).x;
            r5.z = r6.w * 0.200000003 + r5.z;
            r20.xy = r21.xy + r12.xy;
            r6.w = spotShadowMaps.SampleCmpLevelZero(SmplShadow_s, r20.xyz, r21.z).x;
            r5.z = r6.w * 0.200000003 + r5.z;
            r20.xy = r21.xy + r12.zw;
            r6.w = spotShadowMaps.SampleCmpLevelZero(SmplShadow_s, r20.xyz, r21.z).x;
            r5.z = r6.w * 0.200000003 + r5.z;
            r5.y = r5.y * r5.z;
          }
          r5.z = dot(r16.xyz, r8.xyw);
          r5.z = max(r19.x, r5.z);
          r5.y = r5.y * r5.z;
          r19.x = dynamicLights_g[r2.w].color.x;
          r19.y = dynamicLights_g[r2.w].color.y;
          r19.z = dynamicLights_g[r2.w].color.z;
          r15.yzw = r19.xyz * r5.yyy + r15.yzw;
          // ── BRDF Spot Light ──
          float brdf_rawNdotL_sp = dot(r16.xyz, r8.xyw);
          float3 brdf_H_sp = normalize(brdf_V + r16.xyz);
          float brdf_NdotH_sp = saturate(dot(brdf_H_sp, r8.xyw));
          float brdf_VdotH_sp = saturate(dot(brdf_V, brdf_H_sp));
          r16.xyz = r14.xyz * r1.yyy + r16.xyz;
          r5.z = dot(r16.xyz, r16.xyz);
          r5.z = rsqrt(r5.z);
          r16.xyz = r16.xyz * r5.zzz;
          r20.x = dynamicLights_g[r2.w].specularIntensity;
          r20.y = dynamicLights_g[r2.w].specularGlossiness;
          r2.w = r20.y * r2.y;
          r5.z = saturate(dot(r16.xyz, r8.xyw));
          r2.w = max(0.00100000005, r2.w);
          r5.z = log2(r5.z);
          r2.w = r5.z * r2.w;
          r2.w = exp2(r2.w);
          if (brdf_use_ggx) {
            float3 brdf_ggx_spec_sp = GGX_Specular(brdf_NdotH_sp, brdf_NdotV, brdf_rawNdotL_sp, brdf_VdotH_sp, brdf_roughness, brdf_F0);
            brdf_ggx_spec_sp *= MultiScatterCompensation(brdf_NdotV, brdf_rawNdotL_sp, brdf_roughness, brdf_F0);
            float brdf_ggx_scalar_sp = brdf_ggx_spec_sp.x * brdf_rawNdotL_sp;
            r2.w = lerp(r2.w, brdf_ggx_scalar_sp, brdf_specular_str);
            r2.w = max(r2.w, 0.0f);
          }
          r16.xyz = r19.xyz * r2.www;
          r16.xyz = r16.xyz * r5.yyy;
          r13.xyz = r16.xyz * r20.xxx + r13.xyz;
        }
      }
      r2.x = (int)r2.x + 1;
    }
    r2.xyw = r15.yzw * r14.www + r11.xyz;
    r3.xyz = r13.xyz * r14.www + r3.xyz;
  } else {
    r0.w = lightIndices_g[r1.x].pointLightCount;
    r0.w = min(63, (uint)r0.w);
    r10.xyzw = float4(0,0,0,0);
    while (true) {
      r1.y = cmp((uint)r10.w >= (uint)r0.w);
      if (r1.y != 0) break;
      r1.y = lightIndices_g[r1.x].pointLightIndices[r10.w];
      r11.x = dynamicLights_g[r1.y].pos.x;
      r11.y = dynamicLights_g[r1.y].pos.y;
      r11.z = dynamicLights_g[r1.y].pos.z;
      r11.xyz = r11.xyz + -r4.xyz;
      r5.y = dot(r11.xyz, r11.xyz);
      r5.z = sqrt(r5.y);
      r6.w = dynamicLights_g[r1.y].radiusInv;
      r5.z = r6.w * r5.z;
      r6.w = dynamicLights_g[r1.y].attenuation;
      r5.z = log2(abs(r5.z));
      r5.z = r6.w * r5.z;
      r5.z = exp2(r5.z);
      r5.z = 1 + -r5.z;
      r5.z = max(0, r5.z);
      r6.w = cmp(0 < r5.z);
      if (r6.w != 0) {
        r6.w = dynamicLights_g[r1.y].translucency;
        r5.y = rsqrt(r5.y);
        r11.xyz = r11.xyz * r5.yyy;
        r5.y = dot(r11.xyz, r8.xyw);
        r5.y = max(r6.w, r5.y);
        r11.x = dynamicLights_g[r1.y].color.x;
        r11.y = dynamicLights_g[r1.y].color.y;
        r11.z = dynamicLights_g[r1.y].color.z;
        r11.xyz = r11.xyz * r5.zzz;
        r10.xyz = r11.xyz * r5.yyy + r10.xyz;
      }
      r10.w = (int)r10.w + 1;
    }
    r9.xyz = r10.xyz * r14.www + r9.xyz;
    r0.w = lightIndices_g[r1.x].spotLightCount;
    r0.w = min(63, (uint)r0.w);
    r10.yw = localShadowResolutionInv_g * float2(1.25,-1.25);
    r10.xz = float2(0,0);
    r11.xyzw = float4(0,0,0,0);
    while (true) {
      r1.y = cmp((uint)r11.w >= (uint)r0.w);
      if (r1.y != 0) break;
      r1.y = lightIndices_g[r1.x].spotLightIndices[r11.w];
      r12.x = dynamicLights_g[r1.y].pos.x;
      r12.y = dynamicLights_g[r1.y].pos.y;
      r12.z = dynamicLights_g[r1.y].pos.z;
      r12.xyz = r12.xyz + -r4.xyz;
      r5.y = dot(r12.xyz, r12.xyz);
      r5.z = rsqrt(r5.y);
      r12.xyz = r12.xyz * r5.zzz;
      r13.x = dynamicLights_g[r1.y].vec.x;
      r13.y = dynamicLights_g[r1.y].vec.y;
      r13.z = dynamicLights_g[r1.y].vec.z;
      r13.w = dynamicLights_g[r1.y].spotAngleInv;
      r5.z = dot(r12.xyz, r13.xyz);
      r5.z = max(0, r5.z);
      r5.z = 1 + -r5.z;
      r5.z = r5.z * r13.w;
      r6.w = dynamicLights_g[r1.y].attenuationAngle;
      r5.z = log2(r5.z);
      r5.z = r6.w * r5.z;
      r5.z = exp2(r5.z);
      r5.z = 1 + -r5.z;
      r5.z = max(0, r5.z);
      r6.w = cmp(0 < r5.z);
      if (r6.w != 0) {
        r5.y = sqrt(r5.y);
        r6.w = dynamicLights_g[r1.y].radiusInv;
        r5.y = r6.w * r5.y;
        r6.w = dynamicLights_g[r1.y].attenuation;
        r5.y = log2(abs(r5.y));
        r5.y = r6.w * r5.y;
        r5.y = exp2(r5.y);
        r5.y = 1 + -r5.y;
        r5.y = max(0, r5.y);
        r5.y = r5.z * r5.y;
        r5.z = cmp(0 < r5.y);
        if (r5.z != 0) {
          r13.x = dynamicLights_g[r1.y].translucency;
          r13.y = dynamicLights_g[r1.y].shadowmapIndex;
          r5.z = cmp((int)r13.y != -1);
          if (r5.z != 0) {
            r19.xyzw = spotShadowMatrices_g[r13.y]._m00_m10_m20_m30;
            r21.xyzw = spotShadowMatrices_g[r13.y]._m01_m11_m21_m31;
            r23.xyzw = spotShadowMatrices_g[r13.y]._m02_m12_m22_m32;
            r24.xyzw = spotShadowMatrices_g[r13.y]._m03_m13_m23_m33;
            r14.x = dot(r4.xyzw, r19.xyzw);
            r14.y = dot(r4.xyzw, r21.xyzw);
            r14.z = dot(r4.xyzw, r23.xyzw);
            r5.z = dot(r4.xyzw, r24.xyzw);
            r19.xyz = r14.xyz / r5.zzz;
            r19.w = (uint)r13.y;
            r5.z = spotShadowMaps.SampleCmpLevelZero(SmplShadow_s, r19.xyw, r19.z).x;
            r14.xy = r19.xy + r10.yx;
            r14.z = r19.w;
            r6.w = spotShadowMaps.SampleCmpLevelZero(SmplShadow_s, r14.xyz, r19.z).x;
            r6.w = 0.200000003 * r6.w;
            r5.z = r5.z * 0.200000003 + r6.w;
            r14.xy = r19.xy + r10.wz;
            r6.w = spotShadowMaps.SampleCmpLevelZero(SmplShadow_s, r14.xyz, r19.z).x;
            r5.z = r6.w * 0.200000003 + r5.z;
            r14.xy = r19.xy + r10.xy;
            r6.w = spotShadowMaps.SampleCmpLevelZero(SmplShadow_s, r14.xyz, r19.z).x;
            r5.z = r6.w * 0.200000003 + r5.z;
            r14.xy = r19.xy + r10.zw;
            r6.w = spotShadowMaps.SampleCmpLevelZero(SmplShadow_s, r14.xyz, r19.z).x;
            r5.z = r6.w * 0.200000003 + r5.z;
            r5.y = r5.y * r5.z;
          }
          r5.z = dot(r12.xyz, r8.xyw);
          r5.z = max(r13.x, r5.z);
          r12.x = dynamicLights_g[r1.y].color.x;
          r12.y = dynamicLights_g[r1.y].color.y;
          r12.z = dynamicLights_g[r1.y].color.z;
          r12.xyz = r12.xyz * r5.yyy;
          r11.xyz = r12.xyz * r5.zzz + r11.xyz;
        }
      }
      r11.w = (int)r11.w + 1;
    }
    r2.xyw = r11.xyz * r14.www + r9.xyz;
  }
  r3.xyz = r3.xyz * r9.www;
  r8.xyz = r22.xyz * r2.xyw + r3.xyz;
  r0.w = cmp(0 < r17.x);
  r1.x = dot(r0.xyz, float3(0.298999995,0.587000012,0.114));
  r1.x = r1.x + -r17.x;
  r1.x = max(0, r1.x);
  r1.x = r1.x / r17.x;
  r1.x = min(1, r1.x);
  r0.w = r0.w ? r1.x : 1;
  r1.x = r16.w * r5.w;
  r0.w = r1.x * r0.w;
  r1.x = -1 + r3.w;
  r1.x = r17.y * r1.x + 1;
  r3.w = r1.x * r0.w;
  r3.xyz = r0.xyz * r3.www + r8.xyz;
  r8.w = 0;
  r0.xyzw = r20.wwww ? r3.xyzw : r8.xyzw;
  r1.x = 1 + -r5.x;
  r1.x = r17.z * r1.x;
  r1.x = mapAOIntensity_g * r1.x;
  r2.xyw = r0.xyz * mapAOColor_g.xyz + -r0.xyz;
  r0.xyz = r1.xxx * r2.xyw + r0.xyz;
  r1.x = -fogNearDistance_g + -r1.w;
  r1.x = saturate(fogFadeRangeInv_g * r1.x);
  r1.x = 1 + -r1.x;
  r1.x = log2(r1.x);
  r1.x = fogExp_g * r1.x;
  r1.x = exp2(r1.x);
  r1.x = 1 + -r1.x;
  r1.y = -fogHeight_g + r4.y;
  r1.y = saturate(fogHeightRangeInv_g * r1.y);
  r1.x = r1.x * r1.y;
  r1.x = fogIntensity_g * r1.x;
  r1.y = (int)r1.z & 16;
  r2.x = -r2.z * 0.0322580636 + 1;
  r1.z = 1 + -r2.x;
  r2.y = r18.y * r1.z + r2.x;
  r18.w = 1;
  r1.yz = r1.yy ? r2.xy : r18.wy;
  r1.x = r1.x * r15.x;
  r1.x = r1.x * r1.y;
  r2.xyz = fogColor_g.xyz + -r0.xyz;
  r0.xyz = r1.xxx * r2.xyz + r0.xyz;
  r1.x = -r1.w / skyLutCameraFarClip_g;
  r1.x = -skyLutNearOverFarClip_g + r1.x;
  r1.y = -skyLutNearOverFarClip_g + 1;
  r6.z = r1.x / r1.y;
  r1.xyw = atmosphereInscatterLUT.SampleLevel(samLinear_s, r6.xyz, 0).xyz;
  r2.xyz = atmosphereExtinctionLUT.SampleLevel(samLinear_s, r6.xyz, 0).xyz;
  r0.xyz = r0.xyz * r2.xyz + r1.xyw;
  r1.xyw = r0.xyz * r7.www + r7.xyz;
  r1.z = combineAlpha_g * r1.z;
  r1.xyw = r1.xyw + -r0.xyz;
  o0.xyz = r1.zzz * r1.xyw + r0.xyz;
  // Probe ambient debug — character pixel path
  if (shader_injection_data.vbgi_cascade_debug > 0.5f) {
    float3 probeAmbient = lightProbe_g[0].xyz;
    o0.xyz = probeAmbient * 0.5;
    o0.w = 1;
    o1.xyzw = (uint4)(float4(0, 255, 0, 0) * saturate(0.1 * r0.w));  // native o1
    o2.y = min(0x0000ffff, (uint)(65.535 * r18.x));                  // native o2.y
    o2.x = 0;
    return;
  }
  if (shader_injection_data.gtvbao_vbgi_bound > 0.5f) {
    float3 giColor = cachedVBGI;
    // Light Color debug view — shows sun color uniformly
    if ((int)shader_injection_data.vbgi_debug_view == 6) {
      o0.xyz = lightColor_g.xyz * 0.05;
      o0.w = 1;
      o1.xyzw = (uint4)(float4(0, 255, 0, 0) * saturate(0.1 * r0.w));  // native o1
      o2.y = min(0x0000ffff, (uint)(65.535 * r18.x));                  // native o2.y
      o2.x = 0;
      return;
    }
    if (shader_injection_data.gtvbao_vbgi_debug > 0.5f) {
      o0.xyz = giColor;  // Debug: replace scene with GI texture
    } else {
      o0.xyz += giColor;  // Normal: add GI to scene
    }
  }
  // DynCube position-aware diagnostic (debug 11/12/13): override final output with the
  // selected sample's world position / reflection-ray position error (not the reflection color).
  if (dynCubeDiagActive) {
    o0.xyz = dynCubeDiag;
  } else if (dynCubeNewSSRActive && (shader_injection_data.dynCube_debug == 14.f
      || shader_injection_data.dynCube_debug == 15.f
      || shader_injection_data.dynCube_debug == 17.f || shader_injection_data.dynCube_debug == 18.f)) {
    float2 ssrUV = resolutionScaling_g.xy * v1.zw;
    float4 ssrTap = dynCubeSSRTex.SampleLevel(SmplLinearClamp_s, ssrUV, 0);  // blurred
    if (shader_injection_data.dynCube_debug == 14.f) {
      o0.xyz = ssrTap.rgb;   // SSR Result (blurred)
    } else if (shader_injection_data.dynCube_debug == 15.f) {
      o0.xyz = float3(ssrTap.a, ssrTap.a, ssrTap.a);  // SSR Confidence
    } else if (shader_injection_data.dynCube_debug == 17.f) {
      o0.xyz = dynCubeSSRRawTex.SampleLevel(SmplLinearClamp_s, ssrUV, 0).rgb;  // SSR Raw
    } else {
      float minEdge = min(min(ssrUV.x, 1.0 - ssrUV.x), min(ssrUV.y, 1.0 - ssrUV.y));
      float edgeConf = smoothstep(0.0, max(shader_injection_data.dynCube_ssr_edge_fade * 0.25, 1e-4), minEdge);
      o0.xyz = float3(edgeConf, edgeConf, edgeConf);  // SSR Edge Fade
    }
  } else if (dynCubeReflResolveActive && shader_injection_data.dynCube_debug == 16.f) {
    // Reflection Source: RED=SSR, GREEN=Dynamic, BLUE=Vanilla.
    o0.xyz = !dynCubeReflActive ? float3(0, 0, 0)
           : (dynCubeReflSrc == 0) ? float3(1, 0, 0)
           : (dynCubeReflSrc == 1) ? float3(0, 1, 0)
                                   : float3(0, 0, 1);
  }
  r0.y = saturate(0.100000001 * r0.w);
  r0.xzw = float3(255,255,255);
  r0.xyzw = float4(0,255,0,0) * r0.xyzw;
  o1.xyzw = (uint4)r0.xyzw;
  r0.x = 65.5350037 * r18.x;
  r0.x = (uint)r0.x;
  o2.y = min(0x0000ffff, (uint)r0.x);
  o0.w = 1;
  o2.x = 0;
  return;
}