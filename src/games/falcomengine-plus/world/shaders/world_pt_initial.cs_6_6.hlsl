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
  if (g_restir.debug_view != PT_DEBUG_VIEW_SHARC && g_restir.debug_view != PT_DEBUG_VIEW_MATERIAL) return;
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
  g_pt_initial_color[id.xy] = float4(ud.color_sum / float(g_restir.restir.initialSampling.numInitialSamples), 1.0);
  // Initial-pass PT debug views own u14-u16; world_pt_final leaves them untouched for these views (A18).
  PtWriteInitialDebug(id.xy, ud.debug_value, ud.debug_status, ud.debug_word);
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
