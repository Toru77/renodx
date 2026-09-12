// ---- Created with 3Dmigoto v1.4.1 on Fri Aug 21 11:50:58 2026
// RenoDX REPLACEMENT (hash kept): DynCube composite for Sora2nd forward water/puddles.
// Active only when the SSR Replacement toggle serves it (see addon.cpp); otherwise the
// game draws vanilla (this file is bypassed/replaced only under the same gate).
//
// What it does: resolves SSR > Dynamic > Vanilla per pixel with the shared dyncube
// implementation and writes game-SSR-encoded output (rgb = resolved reflection,
// w = non-vanilla fraction) into ssr1's RTV0. Vanilla ssr2 then denoises it and sends
// it downstream, so no vanilla SSR math is wasted and temporal continuity is kept.
// Reads: game bindings (t0 color, t1 depth, t2 mrt0, s1 point, b0), pushed t17/t29/
// t30/t31/t32/t33, shader_injection (b13, auto-delivered). Ignores t3/b2.
// Water/puddles are assumed smooth: roughness factor is hardcoded 0 (mirror LOD).

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

SamplerState samPoint_s : register(s1);
Texture2D<float4> colorTexture : register(t0);
Texture2D<float4> depthTexture : register(t1);
Texture2D<uint4> mrtTexture0 : register(t2);
TextureCube<float4> texEnvMap_g : register(t17);
TextureCube<float4> dynCubeHistPosTex : register(t29);
TextureCube<float4> dynCubeVanillaTex : register(t30);
Texture2D<float4> dynCubeSSRTex : register(t31);
Texture2D<float4> dynCubeSSRRawTex : register(t32);
StructuredBuffer<float4> dynCubeWorldBox : register(t33);

#include "../../shared.h"
#include "../../dyncube/dyncube_spatial.hlsli"
#include "../../dyncube/dyncube_sample.hlsli"
#include "../../dyncube/dyncube_resolve.hlsli"

// Static cube sampler (game binds no cube sampler on ssr1 draws):
// trilinear wrap is the cube-sampling standard. If seams ever appear,
// capture the game's s14 object instead (see addon OnBeforeSoraSSR1Draw).
SamplerState DynCubeCubeSampler
{
  Filter = MIN_MAG_MIP_LINEAR;
  AddressU = Wrap;
  AddressV = Wrap;
  AddressW = Wrap;
};

void main(
  float4 v0 : SV_Position0,
  float4 v1 : TEXCOORD0,
  out float4 o0 : SV_Target0)
{
  // Composite debug views (water-SSR observability — the lighting-local debug
  // views never execute on forward water pixels, so the composite carries its own).
  // Writes through to ssr1's target, hence visible downstream on water surfaces.
  if (shader_injection_data.dynCube_debug == 9.f) {
    o0 = float4(dynCubeSSRTex.SampleLevel(samPoint_s, v1.xy, 0).rgb, 1.0);
    return;
  } else if (shader_injection_data.dynCube_debug == 10.f) {
    float ssrConfDbg = dynCubeSSRTex.SampleLevel(samPoint_s, v1.xy, 0).a;
    o0 = float4(ssrConfDbg, ssrConfDbg, ssrConfDbg, 1.0);
    return;
  } else if (shader_injection_data.dynCube_debug == 12.f) {
    o0 = float4(dynCubeSSRRawTex.SampleLevel(samPoint_s, v1.xy, 0).rgb, 1.0);
    return;
  }
  // Eligibility gate (verbatim game logic): only mrt0.w&2 pixels reflect.
  // Anything else emits scene color with zero confidence, preserving vanilla
  // cost profile (no resolve math) and strict vanilla coverage.
  float4 fDest;
  mrtTexture0.GetDimensions(0, fDest.x, fDest.y, fDest.z);
  float2 eligUV = v1.xy * fDest.xy;
  uint4 mrtElig = mrtTexture0.Load(int3(int2(eligUV), 0));
  if (((int)mrtElig.w & 2) == 0) {
    o0.xyz = colorTexture.SampleLevel(samPoint_s, v1.xy, 0).xyz;
    o0.w = 0;
    return;
  }
  // World position (verbatim game reconstruct pattern): NDC from v1.zw.
  float depth = depthTexture.SampleLevel(samPoint_s, v1.xy, 0).x;
  float4 ndcPos = float4(v1.zw * float2(2, -2) + float2(-1, 1), 1, 1);
  float4 worldH =
      float4(dot(ndcPos, viewProjInv_g._m00_m10_m20_m30),
             dot(ndcPos, viewProjInv_g._m01_m11_m21_m31),
             dot(ndcPos, viewProjInv_g._m02_m12_m22_m32),
             dot(ndcPos, viewProjInv_g._m03_m13_m23_m33));
  float3 waterPos = worldH.xyz / worldH.w;

  // World normal from the same mrt texel (same spherical packing the SSR march
  // decodes; see FalcomSSRCS DecodeWorldNormal).
  float2 enc = float2(mrtElig.x, mrtElig.y) * (1.0 / 32767.5) - 1.0;
  float azimuth = 3.14159274 * enc.x;
  float ring = sqrt(saturate(1.0 - enc.y * enc.y));
  float3 waterN = float3(cos(azimuth) * ring, sin(azimuth) * ring, enc.y);
  if (dot(waterN, waterN) < 1e-6) waterN = float3(0.0, 0.0, -1.0);
  waterN = normalize(waterN);

  // View / reflection (Site-A lighting convention: V = pixel -> camera).
  float3 camPos = float3(viewInv_g._m30, viewInv_g._m31, viewInv_g._m32);
  float3 waterV = normalize(camPos - waterPos);
  float ndv = dot(waterN, waterV);
  float3 waterR = waterN * (-(ndv + ndv)) + waterV;

  // SSR -> Dynamic -> Vanilla enable set (mirrors Sora lighting setup).
  bool dynCubeNewSSRActive = shader_injection_data.dynCube_enabled > 0.5f
      && shader_injection_data.dynCube_force_vanilla < 0.5f
      && shader_injection_data.dynCube_ssr_enabled > 0.5f;
  bool dynCubeForceDynamicActive = shader_injection_data.dynCube_enabled > 0.5f
      && shader_injection_data.dynCube_force_vanilla < 0.5f
      && shader_injection_data.dynCube_force_dynamic > 0.5f;
  bool dynCubeForceSSRActive = shader_injection_data.dynCube_enabled > 0.5f
      && shader_injection_data.dynCube_force_vanilla < 0.5f
      && shader_injection_data.dynCube_force_ssr > 0.5f;
  bool dynCubeReflResolveActive = shader_injection_data.dynCube_enabled > 0.5f
    && shader_injection_data.dynCube_force_vanilla < 0.5f;
  float dynCubeReflectSign = (shader_injection_data.dynCube_reflect_sign_flip > 0.5f) ? -1.0 : 1.0;
  float3 dynCubeReflDir = float3(0, 0, 0);
  bool dynCubeReflActive = false;
  int dynCubeReflSrc = 1;
  bool dynCubeSpatialActive = shader_injection_data.dynCube_enabled > 0.5f
      && shader_injection_data.dynCube_spatial_reprojection > 0.5f;
  float dynCubeSsrGateConf = 1.0f;
  if (dynCubeSpatialActive && dynCubeNewSSRActive) {
    dynCubeSsrGateConf = dynCubeSSRTex.SampleLevel(samPoint_s, v1.xy, 0).a;
  }

  // Dynamic sample. Water/puddles are assumed smooth (mirror LOD); the vanilla
  // mip factor is 0 for the same reason. v1.xy are 0-1 UVs (no rescale).
  float3 waterDynCol;
  float3 waterFinalDir;
  int waterParallaxFace;
  uint waterNumLevels;
  float waterSampleMip;
  DynCubeSampleDynamic(
      texEnvMap_g, DynCubeCubeSampler,
      dynCubeHistPosTex, samPoint_s,
      dynCubeWorldBox,
      waterPos, waterR, 0.0, dynCubeReflectSign, camPos,
      dynCubeSsrGateConf, dynCubeSpatialActive, dynCubeForceSSRActive, dynCubeNewSSRActive,
      waterDynCol, waterFinalDir, waterParallaxFace,
      waterNumLevels, waterSampleMip);
  dynCubeReflDir = waterFinalDir;
  dynCubeReflActive = true;

  // Resolve. Sky/depth-miss pixels carry no validity, so the resolver blends
  // them to vanilla exactly like the lighting path does.
  float3 waterResolved = waterDynCol;
  float waterVanillaW = 1.0;
  DynCubeResolveSSR(
      dynCubeSSRTex, samPoint_s,
      dynCubeVanillaTex, DynCubeCubeSampler,
      dynCubeHistPosTex, samPoint_s,
      v1.xy, dynCubeReflDir, 0.0,
      dynCubeReflActive, dynCubeForceDynamicActive, dynCubeForceSSRActive, dynCubeNewSSRActive,
      waterResolved, dynCubeReflSrc, waterVanillaW);

  // Game-SSR encoding: rgb = resolved reflection, w = non-vanilla fraction.
  // Miss-everywhere yields {vanillaCol, 0}, identical in effect to a vanilla
  // {sceneColor, 0} miss under downstream w-lerp blending.
  o0 = float4(waterResolved, saturate(1.0 - waterVanillaW));
  return;
}
