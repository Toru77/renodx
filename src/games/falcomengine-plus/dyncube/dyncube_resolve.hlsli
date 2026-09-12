// dyncube_resolve.hlsli — shared SSR > Dynamic > Vanilla resolution.
//
// DynCubeResolveSSR() implements the reflection-source blend previously
// duplicated inline at both resolve sites of the Sora 2nd lighting shader:
// force-dynamic / force-SSR fast paths, SSR tap + edge/confidence-fallback
// weighting, vanilla-cube fallback on its own mip chain, histPos validity +
// optional coverage fade, and manual layer-mix vs automatic confidence blend.
// Operation order is preserved exactly; only local names were unified
// (Site B used ssrUV2/ssrTap2/vanillaCol2/hpV2 suffixed variants).
//
// Inclusion contract: the including translation unit must provide
// `shader_injection_data` (shared.h) before including this file.
// Resources are explicit parameters — no registers are declared here.
// dynCol/reflSrc are inout because the caller seeds them (dynamic sample,
// default source) and the resolver overwrites them on taken paths.
// outVanillaWeight reports the vanilla fraction of the final blend (0 = fully
// SSR/dynamic, 1 = fully vanilla) so consumers that encode confidence separately
// (e.g. a downstream SSR target expecting w = non-vanilla weight) need no blend
// re-derivation. Always assigned, including the !reflActive path (1.0 = vanilla).

#ifndef __DYNCUBE_RESOLVE_HLSLI__
#define __DYNCUBE_RESOLVE_HLSLI__

void DynCubeResolveSSR(
    Texture2D<float4> ssrTex, SamplerState ssrSampler,
    TextureCube<float4> vanillaTex, SamplerState cubeSampler,
    TextureCube<float4> histPosTex, SamplerState pointSampler,
    float2 ssrUV, float3 reflDir, float vanillaMipFactor,
    bool reflActive, bool forceDynamicActive, bool forceSSRActive, bool newSSRActive,
    inout float3 dynCol, inout int reflSrc, out float outVanillaWeight)
{
  outVanillaWeight = 1.0;
  if (reflActive) {
    if (forceDynamicActive) {
      // Force Dynamic: keep the dynamic cube sample (dynCol).
      reflSrc = 1;
      outVanillaWeight = 0.0;
    } else if (forceSSRActive && newSSRActive) {
      // Force SSR: use the SSR color directly.
      float4 ssrTap = ssrTex.SampleLevel(ssrSampler, ssrUV, 0);
      dynCol = ssrTap.rgb;
      if (shader_injection_data.dynCube_enabled > 0.5f
          && shader_injection_data.dynCube_force_vanilla < 0.5f
          && shader_injection_data.dynCube_debug != 4.f) {
        dynCol *= clamp(shader_injection_data.dynCube_capture_boost, 0.0, 8.0);
      }
      reflSrc = 0;
      outVanillaWeight = 0.0;
    } else {
      const float layerMix = shader_injection_data.dynCube_layer_mix;
      // SSR color + confidence (only when SSR active); ssrWeight computed only for automatic mode.
      float3 ssrCol = float3(0, 0, 0);
      float ssrConf = 0.0;
      float ssrWeight = 0.0;
      if (newSSRActive) {
        float4 ssrTap = ssrTex.SampleLevel(ssrSampler, ssrUV, 0);
        ssrCol = ssrTap.rgb;
        if (shader_injection_data.dynCube_enabled > 0.5f
            && shader_injection_data.dynCube_force_vanilla < 0.5f
            && shader_injection_data.dynCube_debug != 4.f) {
          ssrCol *= clamp(shader_injection_data.dynCube_capture_boost, 0.0, 8.0);
        }
        ssrConf = ssrTap.a;
        if (layerMix < -0.5f) {
          float minEdge = min(min(ssrUV.x, 1.0 - ssrUV.x), min(ssrUV.y, 1.0 - ssrUV.y));
          float edgeBand = max(shader_injection_data.dynCube_ssr_edge_fade * 0.25, 1e-4);
          float finalEdgeConf = smoothstep(0.0, edgeBand, minEdge);
          float rawSsr = saturate(ssrTap.a * finalEdgeConf);
          // Continuous AUTO SSR confidence fallback: higher setting suppresses
          // low-confidence SSR sooner (0 = today's weighting, no hard cutoff).
          float ssrFallback = shader_injection_data.dynCube_ssr_confidence_fallback;
          ssrWeight = saturate((rawSsr - ssrFallback) / max(1.0 - ssrFallback, 1e-4));
        }
      }
      // Vanilla fallback: use the vanilla cube's OWN mip chain (its level count), not the
      // dynamic cube's, so the fallback LOD matches the game's native roughness mapping.
      float3 vanillaCol;
      {
        uint vw, vh, vl;
        vanillaTex.GetDimensions(0, vw, vh, vl);
        float vanillaMip = (vl > 1u) ? (float)(vl - 1) * vanillaMipFactor : 0.0;
        vanillaCol = vanillaTex.SampleLevel(cubeSampler, reflDir,
            vanillaMip + shader_injection_data.dynCube_vanilla_blur).xyz;
      }
      // Final dynamic validity: raw histPos capture validity (>0.5 => captured).
      float dynamicConf;
      float4 hpV = histPosTex.SampleLevel(pointSampler, reflDir, 0);
      dynamicConf = (hpV.a > 0.5f) ? 1.0 : 0.0;
      // Smooth the binary dynamic-cubemap validity edge in direction space.
      // Thickness/world-box/parallax data is intentionally not used for coverage.
      float coverageFade = 1.0;
      if (shader_injection_data.dynCube_coverage_fade > 0.5f) {
        float3 covDir = reflDir;
        float3 covRef = (abs(covDir.x) < abs(covDir.y) && abs(covDir.x) < abs(covDir.z))
            ? float3(1.0, 0.0, 0.0)
            : ((abs(covDir.y) < abs(covDir.z)) ? float3(0.0, 1.0, 0.0) : float3(0.0, 0.0, 1.0));
        float3 covU = normalize(cross(covDir, covRef));
        float3 covV = cross(covDir, covU);
        float covR = tan(radians(max(shader_injection_data.dynCube_coverage_width, 0.0)));
        float coverageSum = hpV.a;
        coverageSum += histPosTex.SampleLevel(pointSampler, normalize(covDir + covR * covU), 0).a;
        coverageSum += histPosTex.SampleLevel(pointSampler, normalize(covDir - covR * covU), 0).a;
        coverageSum += histPosTex.SampleLevel(pointSampler, normalize(covDir + covR * covV), 0).a;
        coverageSum += histPosTex.SampleLevel(pointSampler, normalize(covDir - covR * covV), 0).a;
        coverageFade = smoothstep(0.0, 1.0, coverageSum * 0.2);
      }
      // Coverage fade replaces the binary center-validity gate when enabled, so the
      // filtered signal crosses the validity boundary instead of multiplying after it.
      float effectiveDynamicConf = (shader_injection_data.dynCube_coverage_fade > 0.5f)
          ? coverageFade : dynamicConf;
      if (layerMix >= -0.5f) {
        // Manual override (0=SSR, 1=Dynamic, 2=Vanilla) with validity fallback:
        // SSR if confident, else Dynamic, else Vanilla; Dynamic if valid, else Vanilla.
        const bool dynUsable = (dynamicConf > 0.5f);
        const bool ssrUsable = newSSRActive && (ssrConf > 0.02f);
        float3 dynLayer = lerp(vanillaCol, dynCol, effectiveDynamicConf);
        float3 ssrLayer = ssrUsable ? ssrCol : dynLayer;
        float3 vanLayer = vanillaCol;
        if (layerMix <= 1.0f) {
          float t = saturate(layerMix);
          dynCol = lerp(ssrLayer, dynLayer, t);
          reflSrc = (t < 0.5f) ? (ssrUsable ? 0 : (dynUsable ? 1 : 2))
                               : (dynUsable ? 1 : 2);
        } else {
          float u = saturate(layerMix - 1.0f);
          dynCol = lerp(dynLayer, vanLayer, u);
          reflSrc = (u < 0.5f) ? (dynUsable ? 1 : 2) : 2;
        }
        outVanillaWeight = (reflSrc == 2) ? 1.0 : 0.0;
      } else {
        // Automatic confidence blend (weights always sum to 1).
        float remaining = 1.0 - ssrWeight;
        float dynamicWeight = remaining * effectiveDynamicConf;
        float vanillaWeight = remaining - dynamicWeight;
        dynCol = ssrCol * ssrWeight + dynCol * dynamicWeight + vanillaCol * vanillaWeight;
        reflSrc = (ssrWeight >= dynamicWeight && ssrWeight >= vanillaWeight) ? 0
                 : (dynamicWeight >= vanillaWeight) ? 1 : 2;
        outVanillaWeight = vanillaWeight;
      }
    }
  }
}

#endif // __DYNCUBE_RESOLVE_HLSLI__
