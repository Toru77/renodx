// dyncube_sample.hlsli — shared dynamic-cubemap sampling for Falcom Engine+.
//
// DynCubeSampleDynamic() performs the full dynamic-cube lookup chain that was
// previously duplicated inline at both resolve sites of the Sora 2nd lighting
// shader, preserving its operation order exactly:
//   1. world-box (fallback camera-box) parallax correction,
//   2. game<->cube (1,-1,-1) flip, roughness mip, force-mip, blur bias,
//      lookup-direction flip, vertical-offset tilt,
//   3. gated experimental spatial reprojection around the uncorrected ray,
//   4. cube sample, capture-brightness packaging, parallax-face debug tint.
//
// Inclusion contract: the including translation unit must provide
// `shader_injection_data` (shared.h) before including this file.
// Resources are explicit parameters — no registers are declared here.

#ifndef __DYNCUBE_SAMPLE_HLSLI__
#define __DYNCUBE_SAMPLE_HLSLI__

#include "parallax_cubemap.hlsli"
#include "dyncube_spatial.hlsli"

void DynCubeSampleDynamic(
    TextureCube<float4> envTex, SamplerState cubeSampler,
    TextureCube<float4> histPosTex, SamplerState pointSampler,
    StructuredBuffer<float4> worldBox,
    float3 P, float3 R, float roughFactor, float reflectSign, float3 eyePos,
    float ssrGateConf, bool spatialActive, bool forceSSRActive, bool newSSRActive,
    out float3 dynCol, out float3 finalDir, out int face,
    out uint outNumLevels, out float outSampleMip)
{
  // Pre-parallax world reflection ray (spatial search base). Captured here so
  // the experimental search below stays independent of world-box sizing.
  float3 rawRefl = R * reflectSign;
  // Parallax-corrected cubemap lookup (generic Falcom Engine+). Active only for the
  // dynamic cube (enabled + not force-vanilla) so the vanilla path is untouched.
  float3 Rc = R;
  face = -1;
  if (shader_injection_data.dynCube_enabled > 0.5f
      && shader_injection_data.dynCube_force_vanilla < 0.5f
      && shader_injection_data.dynCube_parallax_enabled > 0.5f) {
    float3 parallaxDir;
    // World-fixed box path: persistent world-space proxy, margin applied at
    // lookup time. Falls back to the camera-centered path when the toggle is
    // off or no valid bounds were accumulated yet.
    if (shader_injection_data.dynCube_worldbox_enabled > 0.5f
        && worldBox[0].w > 0.5f) {
      float wbMargin = max(0.0f, shader_injection_data.dynCube_worldbox_margin);
      if (DynCubeParallaxCorrectBox(P, R * reflectSign, eyePos,
          worldBox[0].xyz - wbMargin,
          worldBox[1].xyz + wbMargin,
          parallaxDir, face)) {
        Rc = parallaxDir * reflectSign;
      }
    } else if (DynCubeParallaxCorrect(P, R * reflectSign, eyePos,
        float3(shader_injection_data.dynCube_parallax_box_size_x,
               shader_injection_data.dynCube_parallax_box_size_y,
               shader_injection_data.dynCube_parallax_box_size_z),
        parallaxDir, face)) {
      Rc = parallaxDir * reflectSign;
    }
  }
  uint sampW, sampH;
  envTex.GetDimensions(0, sampW, sampH, outNumLevels);
  float3 Rs = float3(1,-1,-1) * Rc;
  float sampleMip = (float)(outNumLevels - 1);
  sampleMip = sampleMip * roughFactor;
  // Debug: Force Cubemap Mip (dynCube_force_mip >= 0) — bypass roughness LOD to verify the GGX chain.
  // DynCube-only mip overrides apply only to the dynamic path; DynCube OFF = pure game vanilla.
  if (shader_injection_data.dynCube_enabled > 0.5f && shader_injection_data.dynCube_force_mip > -0.5f) {
    sampleMip = clamp(shader_injection_data.dynCube_force_mip, 0.0, (float)(outNumLevels - 1));
  }
  if (shader_injection_data.dynCube_enabled > 0.5f) {
    sampleMip += (shader_injection_data.dynCube_force_vanilla > 0.5f
        ? shader_injection_data.dynCube_vanilla_blur
        : shader_injection_data.dynCube_blur);
  }
  outSampleMip = sampleMip;
  // Lookup flip applies only when DynCube is active; vanilla lookups stay untouched.
  float flip = (shader_injection_data.dynCube_enabled > 0.5f
      && shader_injection_data.dynCube_lookup_direction_flip > 0.5f) ? -1.0 : 1.0;
  float3 sampleDir = Rs * flip;
  // TEST: vertical offset tilt of the dynamic cubemap lookup (degrees, 0 = no-op,
  // + slides content down). Applied before sampling so validity/vanilla follow it.
  if (shader_injection_data.dynCube_enabled > 0.5f
      && shader_injection_data.dynCube_force_vanilla < 0.5f
      && abs(shader_injection_data.dynCube_vertical_offset) > 1e-4) {
    sampleDir.y -= tan(radians(shader_injection_data.dynCube_vertical_offset));
    sampleDir = normalize(sampleDir);
  }
  // EXPERIMENTAL spatial reprojection: search around the uncorrected reflection
  // for captured geometry on the live ray; on success sample that direction.
  // Otherwise the existing dynamic path below runs exactly as before.
  finalDir = sampleDir;
  if (spatialActive
      && !forceSSRActive
      && shader_injection_data.dynCube_force_vanilla < 0.5f
      && newSSRActive && ssrGateConf < 0.02f) {
    float3 searchBase = rawRefl;
    searchBase = float3(1,-1,-1) * searchBase;
    searchBase = searchBase * flip;
    if (abs(shader_injection_data.dynCube_vertical_offset) > 1e-4) {
      searchBase.y -= tan(radians(shader_injection_data.dynCube_vertical_offset));
      searchBase = normalize(searchBase);
    }
    float searchSamples = shader_injection_data.dynCube_spatial_reprojection_samples;
    float3 best;
    if (DynCubeSpatialReproject(
        histPosTex, pointSampler,
        searchBase, P, rawRefl,
        shader_injection_data.dynCube_spatial_reprojection_radius,
        searchSamples < 2.5f ? 1 : (searchSamples < 7.0f ? 5 : 9),
        shader_injection_data.dynCube_spatial_reprojection_error,
        shader_injection_data.dynCube_spatial_reprojection_min_distance,
        best)) {
      finalDir = best;
    }
  }
  dynCol = envTex.SampleLevel(cubeSampler, finalDir, sampleMip).xyz;
  // Package reflection brightness (dynamic + SSR only): mirrors the t17 override
  // condition so vanilla/debug views stay untouched. Vanilla fallback never scaled.
  if (shader_injection_data.dynCube_enabled > 0.5f
      && shader_injection_data.dynCube_force_vanilla < 0.5f
      && shader_injection_data.dynCube_debug != 4.f) {
    dynCol *= clamp(shader_injection_data.dynCube_capture_boost, 0.0, 8.0);
  }
  // Parallax debug: tint by the probe-box exit face (only on a valid box hit).
  if (shader_injection_data.dynCube_parallax_debug > 0.5f && face >= 0) {
    dynCol = DynCubeParallaxFaceColor(face);
  }
}

#endif // __DYNCUBE_SAMPLE_HLSLI__
