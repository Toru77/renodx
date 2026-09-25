// ---- Created with 3Dmigoto v1.4.1 on Fri Aug 21 11:50:58 2026
// RenoDX REPLACEMENT (hash kept): DynCube composite for Sora2nd forward water/puddles.
// Active only when the SSR Replacement toggle serves it (see addon.cpp); otherwise the
// game draws vanilla (this file is bypassed/replaced only under the same gate).
//
// What it does: resolves the VANILLA march result (t0, game-bound ssr1 output) >
// Dynamic > Vanilla per pixel with the shared dyncube implementation and writes
// game-SSR-encoded output (rgb = resolved reflection, w = non-vanilla fraction) so
// downstream water shaders consume it like vanilla SSR. The custom SSR march is
// intentionally NOT consumed here (it stays lighting-local); the Game SSR toggle
// controls whether the vanilla march runs and feeds the composite. Water/puddles
// are assumed smooth: roughness factor is hardcoded 0 (mirror LOD).
//
// Water gate: only mrt0.w&2 pixels take the composite path. Everything else — plus
// the whole frame under Force Vanilla / DynCube-off — runs the verbatim vanilla ssr2
// body inlined below, so bed and non-water pixels are pixel-identical to vanilla.

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

cbuffer cb_ssr : register(b2)
{
  uint maxRayCount_g : packoffset(c0);
  float rayLength_g : packoffset(c0.y);
  float2 prevResolutionScaling_g : packoffset(c0.z);
  float2 texelSize_g : packoffset(c1);
  float2 uvClamp_g : packoffset(c1.z);
  float4x4 ssrPrevViewProj_g : packoffset(c2);
}

SamplerState samLinear_s : register(s0);
SamplerState samPoint_s : register(s1);
Texture2D<float4> ssrTexture : register(t0);
Texture2D<float4> historyTexture : register(t1);
Texture2D<float> depthTexture : register(t2);
Texture2D<uint4> mrtTexture0 : register(t4);
Texture3D<float2> dynCubeIsfastNoiseTex : register(t5);  // IS-FAST volume (128x128x32 RG8), pushed when usable
TextureCube<float4> texEnvMap_g : register(t17);
TextureCube<float4> dynCubeHistPosTex : register(t29);
TextureCube<float4> dynCubeVanillaTex : register(t30);

#include "../../shared.h"
#include "../../dyncube/dyncube_sample.hlsli"
#include "../../dyncube/dyncube_resolve.hlsli"

// Static point-clamp sampler for the IS-FAST noise volume (mirrors the custom
// march sampling: stable texel values, frac-wrapped UVs stay in [0,1)).
SamplerState ssrIsfastNoiseSamp
{
  Filter = MIN_MAG_MIP_POINT;
  AddressU = Clamp;
  AddressV = Clamp;
  AddressW = Clamp;
};

// Static cube sampler (game binds no cube sampler on ssr2 draws):
// trilinear wrap is the cube-sampling standard. If seams ever appear,
// capture the game's s14 object instead (see addon OnBeforeSoraSSR2Draw).
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
  // Writes through to the ssr2 target, hence visible on water surfaces.
  // 9/10 show the vanilla march the composite resolves.
  if (shader_injection_data.dynCube_debug == 9.f) {
    o0 = float4(ssrTexture.SampleLevel(samLinear_s, v1.xy, 0).rgb, 1.0);
    return;
  } else if (shader_injection_data.dynCube_debug == 10.f) {
    float ssrConfDbg = ssrTexture.SampleLevel(samLinear_s, v1.xy, 0).a;
    o0 = float4(ssrConfDbg, ssrConfDbg, ssrConfDbg, ssrConfDbg);
    return;
  }
  // MRT texel first: it drives the eligibility gate below and the normal decode.
  // Gated-out pixels return before any composite math (no depth sample, no cubemap).
  uint mrtW, mrtH;
  mrtTexture0.GetDimensions(mrtW, mrtH);
  int2 mrtPx = int2(min(v1.xy * float2(mrtW, mrtH), float2(mrtW - 1, mrtH - 1)));
  uint4 mrtRaw = mrtTexture0.Load(int3(mrtPx, 0));
  // Water-eligibility gate (verbatim game logic): only mrt0.w&2 pixels take the
  // composite path below, and only while SSR Replacement serves. Everything
  // else runs the vanilla ssr2 body — verbatim when all Vanilla SSR
  // Improvements are off, improved when enabled — so bed and non-water pixels
  // stay correct.
  bool dynCubeWaterEligible = ((((int)mrtRaw.w) & 2) != 0);
  bool dynCubeVanillaBypass = shader_injection_data.dynCube_enabled < 0.5f
      || shader_injection_data.dynCube_force_vanilla > 0.5f;
  bool ssrReplacementServing = shader_injection_data.dynCube_ssr_replacement > 0.5f;
  if (!dynCubeWaterEligible || dynCubeVanillaBypass || !ssrReplacementServing) {
    float4 r0,r1,r2,r3,r4,r5;
    uint4 bitmask, uiDest;
    float4 fDest;

    r0.z = depthTexture.SampleLevel(samPoint_s, v1.xy, 0).x;
    r0.xy = v1.zw * float2(2,-2) + float2(-1,1);
    r0.w = 1;
    r1.x = dot(r0.xyzw, viewProjInv_g._m00_m10_m20_m30);
    r1.y = dot(r0.xyzw, viewProjInv_g._m01_m11_m21_m31);
    r1.z = dot(r0.xyzw, viewProjInv_g._m02_m12_m22_m32);
    r1.w = dot(r0.xyzw, viewProjInv_g._m03_m13_m23_m33);
    r0.xyzw = r1.xyzw / r1.wwww;
    r1.x = dot(r0.xyzw, ssrPrevViewProj_g._m00_m10_m20_m30);
    r1.y = dot(r0.xyzw, ssrPrevViewProj_g._m01_m11_m21_m31);
    r0.x = dot(r0.xyzw, ssrPrevViewProj_g._m03_m13_m23_m33);
    r0.xy = r1.xy / r0.xx;
    r0.xy = r0.xy * float2(0.5,0.5) + float2(0.5,0.5);
    r0.z = 1 + -r0.y;
    // Reprojection motion (Vanilla SSR Improvements): unscaled flipped history
    // UV vs current tap UV, same space Kai measures in. Feeds adaptive weight
    // and the disocclusion tests below.
    float2 ssrHistRawUV = r0.xz;
    float ssrMotionD = length(ssrHistRawUV - v1.xy);
    // Master switch for Vanilla SSR Improvements (off = vanilla code only).
    bool ssrVanillaImpr = shader_injection_data.dynCube_vanilla_ssr_enabled > 0.5f;
    r0.xy = resolutionScaling_g.xy * r0.xz;
    r0.xy = prevResolutionScaling_g.xy * r0.xy;
    float2 ssrHistTapUV = r0.xy;
    // IS-FAST subpixel history distribution (Vanilla SSR Improvements):
    // blue-noise offset of the single history tap so TAA/upscalers receive a
    // temporally distributed signal. Mirrors the custom march pattern (same
    // volume, frame slice, spatial scale, strength blend; strength 0 = zero
    // offset = vanilla). Seed fixed at 0; frame -1 (noise unusable) disables.
    float ssrIsfastFrame = shader_injection_data.dynCube_vanilla_isfast_frame;
    bool ssrIsfastOn = ssrVanillaImpr && shader_injection_data.dynCube_vanilla_isfast > 0.5f && ssrIsfastFrame >= 0.0f;
    if (ssrIsfastOn) {
      uint ssrHistW, ssrHistH;
      historyTexture.GetDimensions(ssrHistW, ssrHistH);
      float ssrIsfastSpatial = max(shader_injection_data.dynCube_ssr_isfast_spatial, 1e-4f);
      float2 ssrIsfNxy = frac((v1.xy * float2(ssrHistW, ssrHistH) + 0.5) / 128.0 * ssrIsfastSpatial);
      float ssrIsfNz = frac((fmod(ssrIsfastFrame, 32.0) + 0.5) / 32.0 * max(shader_injection_data.dynCube_ssr_isfast_temporal, 0.0f));
      float2 ssrIsfN = dynCubeIsfastNoiseTex.SampleLevel(ssrIsfastNoiseSamp, float3(ssrIsfNxy, ssrIsfNz), 0).xy;
      ssrHistTapUV += (ssrIsfN - 0.5) * texelSize_g.xy * clamp(shader_injection_data.dynCube_ssr_isfast_strength, 0.0f, 1.0f);
    }
    r0.xyzw = historyTexture.SampleLevel(samLinear_s, ssrHistTapUV, 0).xyzw;
    r1.xy = saturate(-texelSize_g.xy * float2(0.5,0.5) + v1.xy);
    r1.xyzw = ssrTexture.SampleLevel(samLinear_s, r1.xy, 0).xyzw;
    r2.xy = saturate(texelSize_g.xy * float2(0.5,0.5) + v1.xy);
    r2.xyzw = ssrTexture.SampleLevel(samLinear_s, r2.xy, 0).xyzw;
    r3.xyzw = r2.xyzw + r1.xyzw;
    r4.xyzw = ssrTexture.SampleLevel(samLinear_s, v1.xy, 0).xyzw;
    r5.xyzw = r4.xyzw + r4.xyzw;
    r3.xyzw = r3.xyzw * float4(4,4,4,4) + -r5.xyzw;
    r5.xyzw = -r3.xyzw * float4(0.166666999,0.166666999,0.166666999,0.166666999) + r4.xyzw;
    r4.xyzw = r5.xyzw * float4(1.08731282,1.08731282,1.08731282,1.08731282) + r4.xyzw;
    r4.xyzw = max(float4(0,0,0,0), r4.xyzw);
    r4.xyzw = min(float4(65472,65472,65472,65472), r4.xyzw);
    r5.x = dot(r4.xyz, float3(0.298999995,0.587000012,0.114));
    r5.x = r5.x * r4.w;
    r3.xyzw = r4.xyzw + r3.xyzw;
    r3.xyzw = float4(0.142857,0.142857,0.142857,0.142857) * r3.xyzw;
    r3.x = dot(r3.xyz, float3(0.298999995,0.587000012,0.114));
    r3.x = r3.x * r3.w + -r5.x;
    r5.xyzw = min(r2.xyzw, r1.xyzw);
    r1.xyzw = max(r2.xyzw, r1.xyzw);
    r1.xyzw = abs(r3.xxxx) * float4(4,4,4,4) + r1.xyzw;
    r2.xyzw = -abs(r3.xxxx) * float4(4,4,4,4) + r5.xyzw;
    r3.xyzw = r2.xyzw + r1.xyzw;
    r1.xyzw = -r2.xyzw + r1.xyzw;
    r1.xyzw = float4(0.5,0.5,0.5,0.5) * r1.xyzw;
    r0.xyzw = -r3.xyzw * float4(0.5,0.5,0.5,0.5) + r0.xyzw;
    r2.xyzw = float4(0.5,0.5,0.5,0.5) * r3.xyzw;
    r3.xyzw = float4(9.99999975e-05,9.99999975e-05,9.99999975e-05,9.99999975e-05) + r0.xyzw;
    r1.xyzw = r1.xyzw / r3.xyzw;
    r1.y = min(abs(r1.y), abs(r1.z));
    r1.x = min(abs(r1.x), r1.y);
    r1.x = min(r1.x, abs(r1.w));
    r1.x = min(1, r1.x);
    r0.xyzw = r0.xyzw * r1.xxxx + r2.xyzw;
    r0.xyzw = r0.xyzw + -r4.xyzw;
    // History blend (Vanilla SSR Improvements): fixed slider weight or
    // motion-adaptive (Kai formula: 0.1 static → 0.4 fast). r0 = history.
    // Master switch off restores the fixed vanilla 0.1 blend exactly.
    float ssrCurFrac = 0.1f;
    if (ssrVanillaImpr) {
      ssrCurFrac = (shader_injection_data.dynCube_vanilla_history_fixed > 0.5f)
          ? (1.0f - clamp(shader_injection_data.dynCube_vanilla_history_weight, 0.0f, 0.99f))
          : (0.4 - 0.3 * exp2(-1442.69507 * ssrMotionD));
    }
    float ssrHistFrac = 1.0f - ssrCurFrac;
    r0.xyzw = r0.xyzw * ssrHistFrac + r4.xyzw;
    // Disocclusion reject (Vanilla SSR Improvements): validate the reprojected
    // history against current-frame scene data; on mismatch use the current
    // SSR result instead of stale history. UV bounds exact; motion + depth via
    // sliders. Confidence rides along (current conf on reject, like Kai).
    bool ssrRejectHist = false;
    if (ssrVanillaImpr && shader_injection_data.dynCube_vanilla_disoc_reject > 0.5f) {
      ssrRejectHist = any(ssrHistRawUV < 0.0f) || any(ssrHistRawUV > 1.0f)
          || (ssrMotionD > clamp(shader_injection_data.dynCube_vanilla_disoc_uv, 0.0f, 0.25f));
      if (!ssrRejectHist) {
        float ssrDepthCur = depthTexture.SampleLevel(samPoint_s, v1.xy, 0).x;
        float ssrDepthHist = depthTexture.SampleLevel(samPoint_s, ssrHistRawUV, 0).x;
        float ssrLinCur = dot(projInv_g._m22_m32, float2(ssrDepthCur, 1.0)) / dot(projInv_g._m23_m33, float2(ssrDepthCur, 1.0));
        float ssrLinHist = dot(projInv_g._m22_m32, float2(ssrDepthHist, 1.0)) / dot(projInv_g._m23_m33, float2(ssrDepthHist, 1.0));
        ssrRejectHist = abs(ssrLinHist - ssrLinCur) > clamp(shader_injection_data.dynCube_vanilla_disoc_depth, 0.0f, 2.0f);
      }
      if (ssrRejectHist) r0.xyzw = r4.xyzw;
    }
    r0.xyzw = max(float4(0,0,0,0), r0.xyzw);
    o0.xyzw = min(float4(65472,65472,65472,65472), r0.xyzw);
    return;
  }

  // Composite path (water pixels only).
  // World position (verbatim game reconstruct): NDC from v1.zw, depth from t2.
  float depth = depthTexture.SampleLevel(samPoint_s, v1.xy, 0).x;
  float4 ndcPos = float4(v1.zw * float2(2, -2) + float2(-1, 1), depth, 1);
  float4 worldH =
      float4(dot(ndcPos, viewProjInv_g._m00_m10_m20_m30),
             dot(ndcPos, viewProjInv_g._m01_m11_m21_m31),
             dot(ndcPos, viewProjInv_g._m02_m12_m22_m32),
             dot(ndcPos, viewProjInv_g._m03_m13_m23_m33));
  float3 waterPos = worldH.xyz / worldH.w;

  // World normal from the mrt texel above — same spherical packing the SSR march
  // decodes; see FalcomSSRCS DecodeWorldNormal.
  float2 enc = float2(mrtRaw.x, mrtRaw.y) * (1.0 / 32767.5) - 1.0;
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

  // SSR -> Dynamic -> Vanilla enable set (mirrors Sora lighting setup). The SSR
  // leg is the vanilla march (game-bound t0), live exactly when the Game SSR
  // toggle runs the march; the custom march is never consumed here.
  bool dynCubeNewSSRActive = shader_injection_data.dynCube_enabled > 0.5f
      && shader_injection_data.dynCube_force_vanilla < 0.5f
      && shader_injection_data.dynCube_game_ssr > 0.5f;
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

   float3 waterDynCol;
   float3 waterFinalDir;
   uint waterNumLevels;
   float waterSampleMip;
   DynCubeSampleDynamic(
       texEnvMap_g, DynCubeCubeSampler,
       waterR, 0.0, dynCubeReflectSign,
       waterDynCol, waterFinalDir,
       waterNumLevels, waterSampleMip);
  dynCubeReflDir = waterFinalDir;
  dynCubeReflActive = true;

  // Resolve. Sky/depth-miss pixels carry no validity, so the resolver blends
  // them to vanilla exactly like the lighting path does.
  float3 waterResolved = waterDynCol;
  float waterVanillaW = 1.0;
  DynCubeResolveSSR(
      ssrTexture, samLinear_s,
      dynCubeVanillaTex, DynCubeCubeSampler,
      dynCubeHistPosTex, samPoint_s,
      v1.xy, dynCubeReflDir, 0.0,
      dynCubeReflActive, dynCubeForceDynamicActive, dynCubeForceSSRActive, dynCubeNewSSRActive, false,
      waterResolved, dynCubeReflSrc, waterVanillaW);

  // Game-SSR encoding: rgb = resolved reflection, w = non-vanilla fraction.
  // Miss-everywhere yields {0, 0} (no vanilla-cube tint by design on this path):
  // downstream w-lerp blending shows the bed, identical in effect to a vanilla
  // {sceneColor, 0} miss.
  o0 = float4(waterResolved, saturate(1.0 - waterVanillaW));
  return;
}
