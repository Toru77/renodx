#define RTXDI_RESTIR_PT_INITIAL_SAMPLING
#define PT_COMMON_PASS 0
#define SHARC_UPDATE 0
// This software contains source code provided by NVIDIA Corporation.
#include "world_pt_common.hlsli"
#include "world_pt_path_tracer.hlsli"
#include <Rtxdi/Utils/RandomSamplerPerPassSeeds.hlsli>
#include <Rtxdi/PT/Decorrelation.hlsli>
#include <Rtxdi/PT/InitialSampling.hlsli>

#if !defined(PT_WATER_SURFACE)
// Initial-pass PT debug views own u14-u16 and write every pixel while active, so they never show stale data (A18.2).
void PtWriteInitialDebug(uint2 pixel, float4 value, uint status, uint4 word) {
  if (g_restir.debug_view != PT_DEBUG_VIEW_SHARC && g_restir.debug_view != PT_DEBUG_VIEW_MATERIAL
      && g_restir.debug_view != PT_DEBUG_VIEW_AO) return;
  g_pt_debug_value[pixel] = value;
  g_pt_debug_status[pixel] = status;
  g_pt_sun_detail[pixel] = word;
}
#endif

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID) {
  if (any(id.xy >= g_size)) return;
#if defined(PT_WATER_SURFACE)
  PtCount(5u);
  for (uint trace = 0u; trace < g_restir.restir.initialSampling.numInitialSamples; ++trace) PtCount(6u);
#else
  if (g_sparse_enabled != 0u && g_sparse_levels < 256u) {
    if (!PtSparseSelected(id.xy)) {
      RTXDI_StorePTReservoir(RTXDI_EmptyPTReservoir(), g_restir.restir.reservoirBuffer, id.xy,
                             g_restir.restir.bufferIndices.initialPathTracerOutputBufferIndex);
      if (RTXDI_PTNeedsPreservedInitialSample(g_restir.restir.decorrelation)) {
        RTXDI_StorePTReservoir(RTXDI_EmptyPTReservoir(), g_restir.restir.reservoirBuffer, id.xy,
                               g_restir.restir.bufferIndices.initialPathTracerPreservedBufferIndex);
      }
      PtCount(8u);
      if (g_restir.restir.spatialResampling.enableSpatialHeuristicMode != 0u) {
        g_pt_neighbor_selection_gbuffer[id.xy] = float4(0, 0, 0, 0);
      }
      g_pt_diffuse[id.xy] = float4(0, 0, 0, 1);
      g_pt_specular[id.xy] = float4(0, 0, 0, 1e-3);
      PtWriteInitialDebug(id.xy, 0.0, 4u, 0u);
      return;
    }
    PtCount(7u);
  }
#endif
  const bool needs_neighbor_selection_gbuffer = g_restir.restir.spatialResampling.enableSpatialHeuristicMode != 0u;
  const RAB_Surface surface = RAB_GetGBufferSurface(id.xy, false);
  if (!RAB_IsSurfaceValid(surface)) {
    if (needs_neighbor_selection_gbuffer) g_pt_neighbor_selection_gbuffer[id.xy] = float4(0, 0, 0, 0);
    RTXDI_StorePTReservoir(RTXDI_EmptyPTReservoir(), g_restir.restir.reservoirBuffer, id.xy,
                           g_restir.restir.bufferIndices.initialPathTracerOutputBufferIndex);
    if (RTXDI_PTNeedsPreservedInitialSample(g_restir.restir.decorrelation)) {
      RTXDI_StorePTReservoir(RTXDI_EmptyPTReservoir(), g_restir.restir.reservoirBuffer, id.xy,
                             g_restir.restir.bufferIndices.initialPathTracerPreservedBufferIndex);
    }
#if !defined(PT_WATER_SURFACE)
    PtWriteInitialDebug(id.xy, 0.0, 4u, 0u);
#endif
    return;
  }
  RAB_PathTracerUserData ud = RAB_EmptyPathTracerUserData();
  ud.pixel = id.xy;
  RAB_PathTracerUserDataSetPathType(ud, RTXDI_PTPathTraceInvocationType_Initial);
#if !defined(PT_WATER_SURFACE)
  // Debug facts are reset once per pixel, not per path tree, so the first tree that reaches a hit wins (A21); no hit
  // keeps status 0 (SHARC) or 3 (Material): no hit / sky. Value and word are zeroed by RAB_EmptyPathTracerUserData.
  ud.debug_status = g_restir.debug_view == PT_DEBUG_VIEW_MATERIAL ? 3u : 0u;
#endif
#if !defined(PT_WATER_SURFACE)
  float ao = 1.0;
  if (g_restir.ao_rays > 0u) {
    RTXDI_RandomSamplerState ao_rng = RTXDI_InitRandomSampler(id.xy, g_frame, 27u);
    const WorldCameraView ao_view = {true, 1.0, 0u, 0u};
    WorldTraceCounters ao_counters;
    uint unoccluded = 0u;
    for (uint ray = 0u; ray < g_restir.ao_rays; ++ray) {
      const float2 u = float2(RTXDI_GetNextRandom(ao_rng), RTXDI_GetNextRandom(ao_rng));
      float pdf;
      const float3 dir = TangentToWorld(surface, SampleCosHemisphere(u, pdf));
      g_rt12_dynamic_mask = g_dynamic_on != 0u ? RT12_MASK_DYNAMIC : 0u;
      const WorldTraceHit hit = TraceWorldRayDxr(surface.worldPos + surface.geoNormal * 1e-3, dir, 1e-3,
                                                 g_restir.ao_radius, true, ao_view, ao_counters);
      if (hit.prim == 0xFFFFFFFFu) ++unoccluded;
    }
    ao = float(unoccluded) / float(g_restir.ao_rays);
  }
  if (g_restir.debug_view == PT_DEBUG_VIEW_AO) PtWriteInitialDebug(id.xy, float4(ao.xxx, 1.0), 0u, 0u);
#endif
  const RTXDI_PathTracerRandomContext rc = RTXDI_InitializePathTracerRandomContext(
      id.xy, g_frame, RTXDI_PT_GENERATE_INITIAL_SAMPLES_RANDOM_SEED, RTXDI_PT_GENERATE_INITIAL_SAMPLES_REPLAY_RANDOM_SEED);
  RTXDI_PTInitialSamplingRuntimeParameters isr = RTXDI_EmptyPTInitialSamplingRuntimeParameters();
  isr.cameraPos = g_sharc_camera;
  isr.prevCameraPos = g_sharc_camera_prev;
  isr.prevPrevCameraPos = g_sharc_camera_prev;
  const RTXDI_PTReservoir reservoir = GenerateInitialSamples(g_restir.restir.initialSampling, isr,
                                                             g_restir.restir.reconnection, rc, surface, ud);
  RTXDI_StorePTReservoir(reservoir, g_restir.restir.reservoirBuffer, id.xy,
                         g_restir.restir.bufferIndices.initialPathTracerOutputBufferIndex);
  if (RTXDI_PTNeedsPreservedInitialSample(g_restir.restir.decorrelation)) {
    RTXDI_StorePTReservoir(reservoir, g_restir.restir.reservoirBuffer, id.xy,
                           g_restir.restir.bufferIndices.initialPathTracerPreservedBufferIndex);
  }
#if !defined(PT_WATER_SURFACE)
  g_pt_initial_color[id.xy] = float4(ud.color_sum / float(g_restir.restir.initialSampling.numInitialSamples), ao);
  // Initial-pass PT debug views own u14-u16; world_pt_final leaves them untouched for these views (A18).
  if (g_restir.debug_view == PT_DEBUG_VIEW_MATERIAL) {
    // A21 PT Material channels, written as display colours with status 0: Source, Albedo and Roughness of the first
    // hit (sky blue when no tree reached one), then Cache at Surface and Cache Index of the material cache at this
    // pixel's own surface (no ray, dark red on a miss).
    const uint channel = g_restir.debug_channel;
    float3 color;
    uint4 word = ud.debug_word;
    if (channel >= 3u) {
      PtMaterial cached;
      const bool cache_hit = PtCacheMaterial(surface.worldPos, surface.normal, cached);
      const uint h = PtHash(cached.debug_index);
      if (!cache_hit) color = float3(0.35, 0.05, 0.05);
      else if (channel == 3u) color = cached.albedo;
      else color = float3(h & 255u, (h >> 8) & 255u, (h >> 16) & 255u) / 255.0;
      word = uint4(cached.debug_index, cache_hit ? 1u : 0u, 0u, 0u);
    } else if (ud.debug_status == 3u) {
      color = float3(0.1, 0.25, 1.0);
    } else if (channel == 1u) {
      color = ud.debug_value.rgb;
    } else if (channel == 2u) {
      color = saturate(ud.debug_value.w).xxx;
    } else if (ud.debug_status == 0u) {
      color = float3(0.1, 1.0, 0.1);
    } else if (ud.debug_status == 1u) {
      color = float3(0.0, 0.8, 1.0);
    } else {
      color = float3(0.5, 0.5, 0.5);
    }
    PtWriteInitialDebug(id.xy, float4(color, 1.0), 0u, word);
  } else if (g_restir.debug_view != PT_DEBUG_VIEW_AO) {
    PtWriteInitialDebug(id.xy, ud.debug_value, ud.debug_status, ud.debug_word);
  }
#endif
  if (RTXDI_IsValidPTReservoir(reservoir)) {
    PtCount(21u);
    if (reservoir.pathLength == 2u) PtCount(22u);
    if (reservoir.pathLength >= 3u) PtCount(23u);
    if (isinf(reservoir.radiance.x)) PtCount(24u);
    const uint energy = uint(min(CalcLuminance(reservoir.weightSum * reservoir.targetFunction), 65535.0) * 16.0);
    if (reservoir.pathLength == 2u) InterlockedAdd(g_pt_counters[35u], energy);
    else if (reservoir.pathLength == 3u) InterlockedAdd(g_pt_counters[36u], energy);
    else if (reservoir.pathLength <= 5u) InterlockedAdd(g_pt_counters[37u], energy);
    else InterlockedAdd(g_pt_counters[38u], energy);
  }
  if (needs_neighbor_selection_gbuffer) {
    g_pt_neighbor_selection_gbuffer[id.xy] = float4(RAB_GetSurfaceNormal(surface), surface.viewDepth);
  }
}
