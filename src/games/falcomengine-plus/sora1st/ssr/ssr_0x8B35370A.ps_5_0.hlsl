// ---- Created with 3Dmigoto v1.4.1 on Sun Sep 13 16:48:06 2026
// RenoDX REPLACEMENT (hash kept): DynCube composite for Sora1st fused water SSR.
// Active only when the SSR Replacement toggle serves it (see addon.cpp); otherwise the
// game draws vanilla (this file is bypassed/replaced only under the same gate).
//
// What it does: runs the VERBATIM vanilla march inline (hit color + confidence),
// resolves Vanilla-march > Dynamic > Vanilla per pixel with the shared dyncube
// implementation, then applies the VERBATIM vanilla motion-weighted temporal lerp,
// so the output contract ({reflection rgb, confidence a}, temporally filtered)
// is preserved for the downstream lighting t24 tap and the history chain.
// The custom SSR march is intentionally NOT consumed here (it stays
// lighting-local); the Game SSR toggle controls whether the inline march runs.
// Water is assumed smooth: roughness comes from the march's own mrt2 term.
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
  uint maxRayCount_g : packoffset(c0);
  float rayLength_g : packoffset(c0.y);
  float2 prevResolutionScaling_g : packoffset(c0.z);
  float4x4 ssrPrevViewProj_g : packoffset(c1);
}

SamplerState samLinear_s : register(s0);
SamplerState samPoint_s : register(s1);
Texture2D<float4> colorTexture : register(t0);
Texture2D<float4> depthTexture : register(t1);
Texture2D<uint4> mrtTexture0 : register(t2);
Texture2D<uint2> mrtTexture2 : register(t3);
Texture2D<float4> prevSSRTexture : register(t4);


// 3Dmigoto declarations
#define cmp -
TextureCube<float4> texEnvMap_g : register(t17);
TextureCube<float4> dynCubeHistPosTex : register(t29);
TextureCube<float4> dynCubeVanillaTex : register(t30);
Texture3D<float2> dynCubeIsfastNoiseTex : register(t5);  // IS-FAST volume (128x128x32 RG8), pushed when usable

#include "../../shared.h"
#include "../../dyncube/dyncube_sample.hlsli"
#include "../../dyncube/dyncube_resolve.hlsli"

// Static point-clamp sampler for the IS-FAST noise volume (mirrors the custom
// march sampling: stable texel values, frac-wrapped UVs stay in [0,1)).
SamplerState s1IsfastNoiseSamp
{
  Filter = MIN_MAG_MIP_POINT;
  AddressU = Clamp;
  AddressV = Clamp;
  AddressW = Clamp;
};

// Static cube sampler (game binds no cube sampler on ssr draws):
// trilinear wrap is the cube-sampling standard.
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
  float4 r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10;
  uint4 bitmask, uiDest;
  float4 fDest;

  mrtTexture0.GetDimensions(0, fDest.x, fDest.y, fDest.z);
  r0.xy = fDest.xy;
  r0.zw = v1.xy * r0.xy;
  r1.xy = (int2)r0.zw;
  r1.zw = float2(0,0);
  r1.xyz = mrtTexture0.Load(r1.xyz).xyw;
  r0.z = (int)r1.z & 2;
  if (r0.z == 0) {
    r2.xyz = colorTexture.SampleLevel(samPoint_s, v1.xy, 0).xyz;
    o0.xyz = r2.xyz;
    o0.w = 0;
    return;
  }
  mrtTexture2.GetDimensions(0, fDest.x, fDest.y, fDest.z);
  r0.zw = fDest.xy;
  r0.zw = v1.xy * r0.zw;
  r2.xy = (int2)r0.zw;
  r2.zw = float2(0,0);
  r0.z = mrtTexture2.Load(r2.xyz).y;
  r2.z = depthTexture.SampleLevel(samPoint_s, v1.xy, 0).x;
  r2.xy = v1.zw * float2(2,-2) + float2(-1,1);
  r2.w = 1;
  // March roughness, saved before the march clobbers r0 (mrt2.y packs a 16-bit
  // roughness: 0.0152590219 == 1000/65535, hence /1000 for 0-1).
  float s1RoughRaw = r0.z;
  float s1Rough01 = saturate(0.0152590219 * (float)(uint)s1RoughRaw / 1000.0);
  // Inline-march gate: the Game SSR toggle runs the vanilla march; off skips
  // the march math (resolve degrades to dynamic/vanilla, temporal hard-replaces).
  bool s1MarchActive = shader_injection_data.dynCube_enabled > 0.5f
      && shader_injection_data.dynCube_force_vanilla < 0.5f
      && shader_injection_data.dynCube_game_ssr > 0.5f;
  if (s1MarchActive) {
    r3.x = dot(r2.xyzw, projInv_g._m00_m10_m20_m30);
    r3.y = dot(r2.xyzw, projInv_g._m01_m11_m21_m31);
    r3.z = dot(r2.xyzw, projInv_g._m02_m12_m22_m32);
    r0.w = dot(r2.xyzw, projInv_g._m03_m13_m23_m33);
    r3.xyz = r3.xyz / r0.www;
    r1.xy = (uint2)r1.xy;
    r1.zw = r1.xy * float2(3.05180438e-05,3.05180438e-05) + float2(-1,-1);
    r0.w = 3.14159274 * r1.z;
    sincos(r0.w, r4.x, r5.x);
    r0.w = -r1.w * r1.w + 1;
    r0.w = sqrt(r0.w);
    r1.x = r5.x * r0.w;
    r1.y = r4.x * r0.w;
    r0.w = dot(r1.xyw, r1.xyw);
    r0.w = rsqrt(r0.w);
    r1.xyz = r1.xyw * r0.www;
    r4.x = dot(r1.xyz, view_g._m00_m10_m20);
    r4.y = dot(r1.xyz, view_g._m01_m11_m21);
    r4.z = dot(r1.xyz, view_g._m02_m12_m22);
    r0.w = dot(r3.xyz, r3.xyz);
    r0.w = rsqrt(r0.w);
    r1.xyz = r3.xyz * r0.www;
    r0.w = dot(r1.xyz, r4.xyz);
    r0.w = r0.w + r0.w;
    r1.xyz = r4.xyz * -r0.www + r1.xyz;
    r0.w = dot(-r3.xyz, -r3.xyz);
    r0.w = rsqrt(r0.w);
    r5.xyz = -r3.xyz * r0.www;
    r0.w = dot(r5.xyz, r4.xyz);
    r0.z = (uint)r0.z;
    r0.z = 0.0152590219 * r0.z;
    r1.w = maxRayCount_g;
    r0.z = r0.z / r1.w;
    r4.xyz = sceneTime_g * r3.xyz;
    r1.w = dot(r4.xyz, float3(12.9898005,78.2330017,56.7869987));
    r1.w = sin(r1.w);
    r1.w = 43758.5469 * r1.w;
    r1.w = frac(r1.w);
    r4.xy = float2(0.899999976,0.200000048) * r0.zz;
    r0.z = r1.w * r4.y + r4.x;
    r5.xyz = r1.xyz * r0.zzz;
    r3.xyw = r1.xyz * r0.zzz + r3.xyz;
    r0.z = 1 + -abs(r0.w);
    r0.z = r0.z * r0.z;
    r0.z = r0.z * r0.z;
    r0.z = r0.z * r0.z;
    r0.z = -r3.z * r0.z;
    r0.z = 0.0199999996 * r0.z;
    r3.xyz = r1.xyz * r0.zzz + r3.xyw;
    r6.w = 1;
    r7.y = 1;
    r8.xyz = r5.xyz;
    r0.zw = float2(0,0);
    r1.w = 0;
    r9.xyz = r3.xyz;
    r3.w = 0;
    while (true) {
      r4.z = cmp((uint)r3.w >= maxRayCount_g);
      if (r4.z != 0) break;
      r6.xyz = r9.xyz;
      r10.x = dot(r6.xyzw, proj_g._m00_m10_m20_m30);
      r10.y = dot(r6.xyzw, proj_g._m01_m11_m21_m31);
      r4.z = dot(r6.xyzw, proj_g._m03_m13_m23_m33);
      r4.zw = r10.xy / r4.zz;
      r7.zw = float2(0.5,0.5) * r4.zw;
      r10.xy = r4.zw * float2(0.5,0.5) + float2(0.5,0.5);
      r4.z = max(abs(r7.z), abs(r7.w));
      r4.z = cmp(0.5 < r4.z);
      if (r4.z != 0) {
        r0.zw = r10.xy;
        break;
      }
      r10.w = 1 + -r10.y;
      r10.z = 1 + -r10.y;
      r4.zw = resolutionScaling_g.xy * r10.xz;
      r7.x = depthTexture.SampleLevel(samPoint_s, r4.zw, 0).x;
      r4.z = dot(projInv_g._m22_m32, r7.xy);
      r4.w = dot(projInv_g._m23_m33, r7.xy);
      r4.z = r4.z / r4.w;
      r4.z = -r9.z + r4.z;
      r4.w = cmp(0 < r4.z);
      r4.z = cmp(r4.z < 10);
      r4.z = r4.z ? r4.w : 0;
      if (r4.z != 0) {
        r0.zw = r10.xz;
        r1.w = -1;
        break;
      }
      r7.xzw = sceneTime_g * r6.xyz;
      r4.z = dot(r7.xzw, float3(12.9898005,78.2330017,56.7869987));
      r4.z = sin(r4.z);
      r4.z = 43758.5469 * r4.z;
      r4.z = frac(r4.z);
      r4.z = r4.z * r4.y + r4.x;
      r8.xyz = r4.zzz * r1.xyz;
      r9.xyz = r1.xyz * r4.zzz + r6.xyz;
      r3.w = (int)r3.w + 1;
      r0.zw = r10.xw;
      r1.w = 0;
    }
    if (r1.w != 0) {
      r1.xyz = r9.xyz + -r8.xyz;
      r3.xyz = float3(0.25,0.25,0.25) * r8.xyz;
      r4.w = 1;
      r5.y = 1;
      r4.xyz = r1.xyz;
      r6.xy = r0.zw;
      r1.w = 2;
      r3.w = 2;
      r5.z = 0;
      while (true) {
        r5.w = cmp((int)r5.z >= 4);
        if (r5.w != 0) break;
        r7.xyz = r3.xyz * r3.www;
        r8.xyz = sceneTime_g * r4.xyz;
        r5.w = dot(r8.xyz, float3(12.9898005,78.2330017,56.7869987));
        r5.w = sin(r5.w);
        r5.w = 43758.5469 * r5.w;
        r5.w = frac(r5.w);
        r5.w = r5.w * 0.200000048 + 0.899999976;
        r4.xyz = r7.xyz * r5.www + r4.xyz;
        r7.x = dot(r4.xyzw, proj_g._m00_m10_m20_m30);
        r7.y = dot(r4.xyzw, proj_g._m01_m11_m21_m31);
        r5.w = dot(r4.xyzw, proj_g._m03_m13_m23_m33);
        r7.xy = r7.xy / r5.ww;
        r6.xz = r7.xy * float2(0.5,0.5) + float2(0.5,0.5);
        r1.w = 0.5 * r1.w;
        r6.y = 1 + -r6.z;
        r6.zw = resolutionScaling_g.xy * r6.xy;
        r5.x = depthTexture.SampleLevel(samPoint_s, r6.zw, 0).x;
        r5.w = dot(projInv_g._m22_m32, r5.xy);
        r5.x = dot(projInv_g._m23_m33, r5.xy);
        r5.x = r5.w / r5.x;
        r5.x = r5.x + -r4.z;
        r5.w = cmp(0 < r5.x);
        // Refine backtrack fix (Vanilla SSR Improvements): step back when the
        // depth delta exceeds the shared hit threshold (0 = any penetration,
        // Kai behavior), Kai-style, so the 4 iterations bracket the crossing.
        // Off = verbatim vanilla (forward-only step, A/B).
        if (shader_injection_data.dynCube_vanilla_ssr_enabled > 0.5f
            && shader_injection_data.dynCube_vanilla_refine_fix > 0.5f) {
          bool s1RefineHit = r5.x > shader_injection_data.dynCube_vanilla_refine_threshold;
          r3.w = s1RefineHit ? -r1.w : r1.w;
        } else {
          r5.x = cmp(r5.x < 0);
          r5.x = r5.x ? r5.w : 0;
          r3.w = r5.x ? -r1.w : r1.w;
        }
        r5.z = (int)r5.z + 1;
      }
      r0.zw = r6.xy;
      r1.xy = float2(-0.5,-0.5) + r0.zw;
      r1.x = dot(r1.xy, r1.xy);
      r1.x = sqrt(r1.x);
      r1.x = r1.x + r1.x;
      r1.y = r1.x * r1.x;
      r1.x = -r1.x * r1.y + 1;
      r1.yz = resolutionScaling_g.xy * r0.zw;
      r0.xy = r1.yz * r0.xy;
      r3.xy = (int2)r0.xy;
      r3.zw = float2(0,0);
      r0.x = mrtTexture0.Load(r3.xyz).w;
      r0.x = (int)r0.x & 2;
      r0.x = r0.x ? 0 : r1.x;
    } else {
      r1.xy = float2(-0.5,-0.5) + r0.zw;
      r0.y = dot(r1.xy, r1.xy);
      r0.y = sqrt(r0.y);
      r0.y = r0.y + r0.y;
      r1.x = r0.y * r0.y;
      r0.x = -r0.y * r1.x + 1;
    }
    r0.yz = resolutionScaling_g.xy * r0.zw;
    r1.xyz = colorTexture.SampleLevel(samPoint_s, r0.yz, 0).xyz;
    r1.w = max(0, r0.x);
  } else {
    r1.xyzw = float4(0, 0, 0, 0);
  }
  // Composite debug views (march observability): 9 = inline march color,
  // 10 = inline march confidence. Bypass resolve + temporal like the sora2nd
  // composite does.
  if (shader_injection_data.dynCube_debug == 9.f) {
    o0 = float4(r1.xyz, 1.0);
    return;
  } else if (shader_injection_data.dynCube_debug == 10.f) {
    o0 = float4(r1.w, r1.w, r1.w, 1.0);
    return;
  }
  float3 s1MarchCol = r1.xyz;
  float s1MarchConf = r1.w;
  // Replacement path only: dynamic sample + resolve. Vanilla-improved
  // mode passes march output straight to temporal below. Conditioned on the
  // replacement toggle alone — Game SSR OFF skips only the march (resolve
  // degrades to dynamic/miss, temporal hard-replaces), never the dynamic.
  bool s1CompositeActive = shader_injection_data.dynCube_ssr_replacement > 0.5f;
  if (s1CompositeActive) {
    // World position (game reconstruct pattern): NDC from v1.zw, depth from t1.
    float s1Depth = depthTexture.SampleLevel(samPoint_s, v1.xy, 0).x;
    float4 s1Ndc = float4(v1.zw * float2(2, -2) + float2(-1, 1), s1Depth, 1);
    float4 s1WorldH =
        float4(dot(s1Ndc, viewProjInv_g._m00_m10_m20_m30),
               dot(s1Ndc, viewProjInv_g._m01_m11_m21_m31),
               dot(s1Ndc, viewProjInv_g._m02_m12_m22_m32),
               dot(s1Ndc, viewProjInv_g._m03_m13_m23_m33));
    float3 s1WaterPos = s1WorldH.xyz / s1WorldH.w;
    // World normal from the mrt texel — same spherical packing the march decodes
    // (azimuth/ring form); the march transforms it to view space, we keep world.
    uint s1MrtW, s1MrtH;
    mrtTexture0.GetDimensions(s1MrtW, s1MrtH);
    int2 s1MrtPx = int2(min(v1.xy * float2(s1MrtW, s1MrtH), float2(s1MrtW - 1, s1MrtH - 1)));
    uint4 s1MrtRaw = mrtTexture0.Load(int3(s1MrtPx, 0));
    float2 s1Enc = float2(s1MrtRaw.x, s1MrtRaw.y) * (1.0 / 32767.5) - 1.0;
    float s1Azimuth = 3.14159274 * s1Enc.x;
    float s1Ring = sqrt(saturate(1.0 - s1Enc.y * s1Enc.y));
    float3 s1WaterN = float3(cos(s1Azimuth) * s1Ring, sin(s1Azimuth) * s1Ring, s1Enc.y);
    if (dot(s1WaterN, s1WaterN) < 1e-6) s1WaterN = float3(0.0, 0.0, -1.0);
    s1WaterN = normalize(s1WaterN);
    // View / reflection (V = pixel -> camera).
    float3 s1CamPos = float3(viewInv_g._m30, viewInv_g._m31, viewInv_g._m32);
    float3 s1WaterV = normalize(s1CamPos - s1WaterPos);
    float s1Ndv = dot(s1WaterN, s1WaterV);
    float3 s1WaterR = s1WaterN * (-(s1Ndv + s1Ndv)) + s1WaterV;
    // SSR -> Dynamic -> Vanilla enable set. The SSR leg is the inline vanilla
    // march (live exactly when the gate above ran it); the custom march is never
    // consumed here.
    bool dynCubeNewSSRActive = shader_injection_data.dynCube_enabled > 0.5f
        && shader_injection_data.dynCube_force_vanilla < 0.5f
        && shader_injection_data.dynCube_game_ssr > 0.5f;
    bool dynCubeForceDynamicActive = shader_injection_data.dynCube_enabled > 0.5f
        && shader_injection_data.dynCube_force_vanilla < 0.5f
        && shader_injection_data.dynCube_force_dynamic > 0.5f;
    bool dynCubeForceSSRActive = shader_injection_data.dynCube_enabled > 0.5f
        && shader_injection_data.dynCube_force_vanilla < 0.5f
        && shader_injection_data.dynCube_force_ssr > 0.5f;
     float dynCubeReflectSign = (shader_injection_data.dynCube_reflect_sign_flip > 0.5f) ? -1.0 : 1.0;
     float3 dynCubeReflDir = float3(0, 0, 0);
     bool dynCubeReflActive = false;
     int dynCubeReflSrc = 1;
     float3 s1DynCol;
     float3 s1FinalDir;
     uint s1NumLevels;
     float s1SampleMip;
     DynCubeSampleDynamic(
         texEnvMap_g, DynCubeCubeSampler,
         s1WaterR, s1Rough01, dynCubeReflectSign,
         s1DynCol, s1FinalDir,
         s1NumLevels, s1SampleMip);
    dynCubeReflDir = s1FinalDir;
    dynCubeReflActive = true;
    // Resolve march-values > dynamic > vanilla. Sky/depth-miss pixels carry no
    // validity, so the resolver blends them to vanilla like the lighting path.
    float3 s1Resolved = s1DynCol;
    float s1VanillaW = 1.0;
    DynCubeResolveSSRValues(
        s1MarchCol, s1MarchConf,
        dynCubeVanillaTex, DynCubeCubeSampler,
        dynCubeHistPosTex, samPoint_s,
        resolutionScaling_g.xy * v1.zw, dynCubeReflDir, s1Rough01,
        dynCubeReflActive, dynCubeForceDynamicActive, dynCubeForceSSRActive, dynCubeNewSSRActive, false,
        s1Resolved, dynCubeReflSrc, s1VanillaW);
    // Game-SSR encoding for the temporal stage + downstream: rgb = resolved
    // reflection, w = non-vanilla fraction (march convention: w = confidence).
    // Miss-everywhere yields {0, 0} (no vanilla-cube tint by design on this path).
    r1.xyz = s1Resolved;
    r1.w = saturate(1.0 - s1VanillaW);
  }
  r0.x = dot(r2.xyzw, viewProjInv_g._m00_m10_m20_m30);
  r0.y = dot(r2.xyzw, viewProjInv_g._m01_m11_m21_m31);
  r0.z = dot(r2.xyzw, viewProjInv_g._m02_m12_m22_m32);
  r0.w = dot(r2.xyzw, viewProjInv_g._m03_m13_m23_m33);
  r0.xyzw = r0.xyzw / r0.wwww;
  r2.x = dot(r0.xyzw, ssrPrevViewProj_g._m00_m10_m20_m30);
  r2.y = dot(r0.xyzw, ssrPrevViewProj_g._m01_m11_m21_m31);
  r0.x = dot(r0.xyzw, ssrPrevViewProj_g._m03_m13_m23_m33);
  r0.xy = r2.xy / r0.xx;
  r0.xy = r0.xy * float2(0.5,0.5) + float2(0.5,0.5);
  r0.z = 1 + -r0.y;
  r0.yw = -v1.zw + r0.xz;
  // Reprojection motion (Vanilla SSR Improvements): unscaled flipped history
  // UV and its distance to current, same space Kai measures in. Feeds the
  // fixed/adaptive blend selection and the disocclusion tests below.
  float2 s1HistRawUV = r0.xz;
  float s1MotionD = length(r0.yw);
  r0.y = dot(r0.yw, r0.yw);
  r0.y = sqrt(r0.y);
  r0.y = -1442.69507 * r0.y;
  r0.y = exp2(r0.y);
  r0.y = r0.y * -0.300000012 + 0.400000006;
  // March skipped: no history to blend toward — hard-replace with current.
  if (!s1MarchActive) r0.y = 1.0;
  r0.xz = resolutionScaling_g.xy * r0.xz;
  r0.xz = prevResolutionScaling_g.xy * r0.xz;
  float2 s1HistTapUV = r0.xz;
  // IS-FAST subpixel history distribution (Vanilla SSR Improvements):
  // blue-noise offset of the single history tap so TAA/upscalers receive a
  // temporally distributed signal. Mirrors the custom march pattern (same
  // volume, frame slice, spatial scale, strength blend; strength 0 = zero
  // offset = vanilla). Seed fixed at 0; frame -1 (noise unusable) disables.
  // Sora1st has no texelSize uniform: texel derived from history dims.
  bool s1VanillaImpr = shader_injection_data.dynCube_vanilla_ssr_enabled > 0.5f;
  if (s1VanillaImpr && shader_injection_data.dynCube_vanilla_isfast > 0.5f
      && shader_injection_data.dynCube_vanilla_isfast_frame >= 0.0f) {
    uint s1HistW, s1HistH;
    prevSSRTexture.GetDimensions(s1HistW, s1HistH);
    float2 s1HistTexel = 1.0 / float2(s1HistW, s1HistH);
    float s1IsfastSpatial = max(shader_injection_data.dynCube_ssr_isfast_spatial, 1e-4f);
    float2 s1IsfNxy = frac((v1.xy * float2(s1HistW, s1HistH) + 0.5) / 128.0 * s1IsfastSpatial);
    float s1IsfNz = frac((fmod(shader_injection_data.dynCube_vanilla_isfast_frame, 32.0) + 0.5) / 32.0 * max(shader_injection_data.dynCube_ssr_isfast_temporal, 0.0f));
    float2 s1IsfN = dynCubeIsfastNoiseTex.SampleLevel(s1IsfastNoiseSamp, float3(s1IsfNxy, s1IsfNz), 0).xy;
    s1HistTapUV += (s1IsfN - 0.5) * s1HistTexel * clamp(shader_injection_data.dynCube_ssr_isfast_strength, 0.0f, 1.0f);
  }
  r2.xyzw = prevSSRTexture.SampleLevel(samLinear_s, s1HistTapUV, 0).xyzw;
  r1.xyzw = -r2.xyzw + r1.xyzw;
  // History blend (Vanilla SSR Improvements): r0.y carries the vanilla
  // motion-adaptive weight; fixed slider or master-off restore fixed paths.
  float s1CurFrac = r0.y;
  if (s1VanillaImpr && shader_injection_data.dynCube_vanilla_history_fixed > 0.5f) {
    s1CurFrac = 1.0f - clamp(shader_injection_data.dynCube_vanilla_history_weight, 0.0f, 0.99f);
  }
  o0.xyzw = s1CurFrac * r1.xyzw + r2.xyzw;
  // Disocclusion reject (Vanilla SSR Improvements): validate the reprojected
  // history against current-frame scene data; on mismatch use the march
  // current instead of stale history (r1 holds current-minus-history here, so
  // current is r1+r2). UV bounds exact; motion + depth via shared sliders.
  // Confidence rides along (march conf on reject, like Kai).
  if (s1VanillaImpr && shader_injection_data.dynCube_vanilla_disoc_reject > 0.5f) {
    bool s1RejectHist = any(s1HistRawUV < 0.0f) || any(s1HistRawUV > 1.0f)
        || (s1MotionD > clamp(shader_injection_data.dynCube_vanilla_disoc_uv, 0.0f, 0.25f));
    if (!s1RejectHist) {
      float s1DepthCur = depthTexture.SampleLevel(samPoint_s, v1.xy, 0).x;
      float s1DepthHist = depthTexture.SampleLevel(samPoint_s, s1HistRawUV, 0).x;
      float s1LinCur = dot(projInv_g._m22_m32, float2(s1DepthCur, 1.0)) / dot(projInv_g._m23_m33, float2(s1DepthCur, 1.0));
      float s1LinHist = dot(projInv_g._m22_m32, float2(s1DepthHist, 1.0)) / dot(projInv_g._m23_m33, float2(s1DepthHist, 1.0));
      s1RejectHist = abs(s1LinHist - s1LinCur) > clamp(shader_injection_data.dynCube_vanilla_disoc_depth, 0.0f, 2.0f);
    }
    if (s1RejectHist) o0.xyzw = r1.xyzw + r2.xyzw;
  }
  return;
}
