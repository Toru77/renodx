// ---- Created with 3Dmigoto v1.4.1 on Wed Mar  4 23:41:21 2026
// Volumetric fog composite — improved with tricubic B-spline sampling,
// and OKLab fog color correction.

#include "./rendering.hlsl"
#include "./kai-vanillaplus.h"

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

SamplerState samLinear_s : register(s0);
SamplerState samPoint_s : register(s1);
SamplerState isfast_sampler : register(s15);
Texture2D<float4> colorTexture : register(t0);
Texture2D<float4> depthTexture : register(t1);
Texture3D<float4> volumeScatter : register(t2);
Texture3D<float2> isfast_noise : register(t15);


void main(
  float4 v0 : SV_Position0,
  float4 v1 : TEXCOORD0,
  out float4 o0 : SV_Target0)
{
  // ---- Depth → linearZ → volume W coordinate (vanilla logic) ----
  float depth = depthTexture.SampleLevel(samPoint_s, v1.xy, 0).x;
  float2 depthVec = float2(depth, 1.0);
  float linearZ = dot(projInv_g._m22_m32, depthVec) / dot(projInv_g._m23_m33, depthVec);
  float remapped = (-linearZ / volumeCameraFarClip_g) * volumeCameraFarOverMaxFar_g
                   - volumeNearOverFarClip_g;
  float volZ = remapped / (1.0 - volumeNearOverFarClip_g);

  // ---- Get volume texture dimensions at runtime ----
  uint volW, volH, volD;
  volumeScatter.GetDimensions(volW, volH, volD);
  float3 volSize = float3((float)volW, (float)volH, (float)volD);

  // ---- Optional IS-FAST temporal jitter (bound texture or fallback) ----
  float3 uvw = float3(v1.xy, volZ);
  if (sss_injection_data.volfog_is_fast_enabled > 0.5) {
    float2 pixelCoord = floor(v0.xy);
    uint frameIndex = (uint)max(sceneTime_g * 60.0, 0.0);
    float jitterZ = renodx::rendering::InterleavedGradientNoiseTemporal(pixelCoord, frameIndex);
    if (sss_injection_data.isfast_noise_bound > 0.5) {
      uint noiseW, noiseH, noiseD;
      isfast_noise.GetDimensions(noiseW, noiseH, noiseD);
      if (noiseW > 0 && noiseD > 0) {
        float2 xi = renodx::rendering::SampleISFAST_RG(
            isfast_noise,
            isfast_sampler,
            pixelCoord,
            frameIndex,
            (float)noiseW,
            (float)noiseD);
        jitterZ = xi.x;
      }
    }

    float3 texelSize = 1.0 / max(volSize, float3(1.0, 1.0, 1.0));
    float halfSlice = 0.5 * texelSize.z;

    uvw.z = clamp(uvw.z + (jitterZ - 0.5) * texelSize.z, halfSlice, 1.0 - halfSlice);
  }

  // ---- Sample volume: tricubic B-spline or vanilla trilinear ----
  float4 fogSample;
  if (sss_injection_data.volfog_tricubic_enabled > 0.5) {
    fogSample = renodx::rendering::SampleTricubicBSpline(volumeScatter, samLinear_s, uvw, volSize);
  } else {
    fogSample = volumeScatter.SampleLevel(samLinear_s, uvw, 0);
  }

  // ---- Scene color ----
  float4 sceneColor = colorTexture.SampleLevel(samPoint_s, v1.xy, 0);

  // ---- Composite: scene * transmittance + inscatter (vanilla formula) ----
  float3 foggedColor = sceneColor.xyz * fogSample.w + fogSample.xyz;

  // ---- Fog color correction (OKLab hue/chroma preservation) ----
  // Treats extinction+inscatter as: extincted_scene + inscatter, matching
  // FogColorCorrection's expected sceneColor + fadeColor decomposition.
  float volfog_fog_cc_strength = saturate(sss_injection_data.volfog_color_correction_strength);
  if (sss_injection_data.fog_color_correction_enabled > 0.5
      && volfog_fog_cc_strength > 0.001) {
    float3 extincted = sceneColor.xyz * fogSample.w;
    foggedColor = renodx::rendering::FogColorCorrection(
        extincted,
        fogSample.xyz,
        sss_injection_data.fog_hue,
        sss_injection_data.fog_chrominance,
        sss_injection_data.fog_avg_brightness,
        sss_injection_data.fog_min_brightness,
        sss_injection_data.fog_min_chroma_change,
        sss_injection_data.fog_max_chroma_change,
        sss_injection_data.fog_lightness_strength,
        volfog_fog_cc_strength);
  }

  // ---- Final blend by combineAlpha (vanilla) ----
  float3 diff = foggedColor - sceneColor.xyz;
  o0.xyz = combineAlpha_g * diff + sceneColor.xyz;
  o0.w = sceneColor.w;
  return;
}
