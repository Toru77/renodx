#define PT_COMMON_PASS 0
#define SHARC_UPDATE 0
// This software contains source code provided by NVIDIA Corporation.
#include "world_pt_common.hlsli"
#include "world_pt_bridge.hlsli"
#include <Rtxdi/Utils/RandomSamplerPerPassSeeds.hlsli>

Texture2D<float2> g_pt_dup_map_stub : register(t43);
Texture2D<float> g_pt_smoothed_dup_stub : register(t44);
#define RTXDI_PT_DUPLICATION_MAP g_pt_dup_map_stub
#define RTXDI_PT_SMOOTHED_DUPLICATION_MAP g_pt_smoothed_dup_stub
#include <Rtxdi/PT/Decorrelation.hlsli>

float3 PtInvalidPixelGuard(float3 value) {
  if (any(isnan(value)) || any(isinf(value))) return 0.0;
  return max(value, 0.0);
}

SplitBrdf PtCalculateDirectLighting(RAB_Surface surface, RTXDI_PTReservoir reservoir) {
  const float3 n = RAB_GetSurfaceNormal(surface);
  const float3 v = RAB_GetSurfaceViewDir(surface);
  float3 l = normalize(reservoir.translatedWorldPosition - RAB_GetSurfaceWorldPos(surface));
  if (reservoir.rcVertexLength > 2u) {
    RTXDI_RandomSamplerState replay_rng = RTXDI_GetRngForShading(reservoir);
    RTXDI_BrdfRaySampleProperties props = RTXDI_DefaultBrdfRaySampleProperties();
    RAB_SurfaceImportanceSampleBsdf(surface, replay_rng, l, props);
  }
  return EvaluateBrdfPT(surface, n, v, l);
}

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID) {
  if (any(id.xy >= g_size)) return;
#if !defined(PT_WATER_SURFACE)
  if (g_sparse_enabled != 0u && g_sparse_levels < 256u && !PtSparseSelected(id.xy)) {
    g_pt_diffuse[id.xy] = float4(0, 0, 0, 1);
    const bool character = (RAB_GetGBufferSurface(id.xy, false).flags & 2u) != 0u;
    g_pt_specular[id.xy] = (g_refl_flags & 32u) != 0u && g_refl_samples != 0u && !character
        ? float4(0, 0, 0, 1e-3) : float4(0, 0, 0, 0);
    return;
  }
#endif
  const RAB_Surface surface = RAB_GetGBufferSurface(id.xy, false);
  if (!RAB_IsSurfaceValid(surface)) {
#if defined(PT_WATER_SURFACE)
    g_pt_specular[id.xy] = float4(0, 0, 0, 0);
#else
    g_pt_diffuse[id.xy] = 0;
    g_pt_specular[id.xy] = float4(0, 0, 0, 0);
#endif
    return;
  }
  const uint2 reservoir_position = id.xy;
  RTXDI_PTReservoir reservoir = RTXDI_LoadPTReservoir(g_restir.restir.reservoirBuffer, reservoir_position,
                                                      g_restir.restir.bufferIndices.finalShadingInputBufferIndex);
  RTXDI_PTApplyDecorrelation(id.xy, reservoir_position, reservoir, g_restir.restir.decorrelation,
                             g_restir.restir.bufferIndices, g_restir.restir.reservoirBuffer, g_frame);

  float3 diffuse = 0.0;
  float3 specular = 0.0;
  const SplitBrdf brdf = PtCalculateDirectLighting(surface, reservoir);
  const float3 ucw = reservoir.weightSum * reservoir.targetFunction;
  const float3 total = brdf.demodulatedDiffuse * surface.material.diffuseAlbedo + brdf.specular;
  diffuse = brdf.demodulatedDiffuse * ucw / max(total, 1e-8);
  specular = brdf.specular * ucw / max(total, 1e-8);
  diffuse = PtInvalidPixelGuard(diffuse);
  specular = PtInvalidPixelGuard(DemodulateSpecular(surface.material.specularF0, specular));

#if defined(PT_WATER_SURFACE)
  if (g_sharc_enabled != 0u && reservoir.rcVertexLength == reservoir.pathLength && !isinf(reservoir.radiance.x)) PtCount(26u);
  float3 rgb = g_refl_sample_clamp > 0.0 ? min(specular, g_refl_sample_clamp) : specular;
  rgb = rgb * g_refl_intensity;
  if ((g_refl_flags & 1u) != 0u) rgb = min(rgb, 1.0);
  const float hit_t = max(length(reservoir.translatedWorldPosition - surface.worldPos), 1e-3);
  g_pt_specular[id.xy] = float4(PtInvalidPixelGuard(rgb), isfinite(hit_t) ? hit_t : 1e-3);
#else
  diffuse = PtClampLuminance(diffuse, g_gi_clamp);
  const float sparse_scale = (g_refl_flags & 32u) != 0u && g_sparse_enabled != 0u && g_sparse_levels < 256u
      ? 256.0 / float(g_sparse_levels) : 1.0;
  g_pt_diffuse[id.xy] = float4(PtInvalidPixelGuard(diffuse * sparse_scale), 1.0);
  if (g_sharc_enabled != 0u && reservoir.rcVertexLength == reservoir.pathLength && !isinf(reservoir.radiance.x)) PtCount(26u);
  if (g_refl_samples == 0u || (surface.flags & 2u) != 0u) {
    g_pt_specular[id.xy] = float4(0, 0, 0, 0);
  } else {
    float3 rgb = g_refl_sample_clamp > 0.0 ? min(specular, g_refl_sample_clamp) : specular;
    rgb = rgb * g_refl_intensity * sparse_scale;
    if ((g_refl_flags & 1u) != 0u) rgb = min(rgb, 1.0);
    const float hit_t = length(reservoir.translatedWorldPosition - surface.worldPos);
    g_pt_specular[id.xy] = float4(PtInvalidPixelGuard(rgb), isfinite(hit_t) ? max(hit_t, 1e-3) : 1e-3);
  }
#endif
}
