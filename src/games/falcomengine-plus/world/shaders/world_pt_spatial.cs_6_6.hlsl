#define PT_COMMON_PASS 0
#define SHARC_UPDATE 0
// This software contains source code provided by NVIDIA Corporation.
#include "world_pt_common.hlsli"
#include "world_pt_path_tracer.hlsli"
#include <Rtxdi/Utils/RandomSamplerPerPassSeeds.hlsli>
#include <Rtxdi/PT/SpatialResampling.hlsli>

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID) {
  const uint2 pixel = id.xy;
  if (any(pixel >= g_size)) return;
  RTXDI_PTReservoir reservoir = RTXDI_EmptyPTReservoir();
  if (g_sparse_enabled != 0u && g_sparse_levels < 256u && !PtSparseSelected(pixel)) {
    PtCount(8u);
  } else {
    const RAB_Surface surface = RAB_GetGBufferSurface(pixel, false);
    if (RAB_IsSurfaceValid(surface)) {
      reservoir = RTXDI_LoadPTReservoir(g_restir.restir.reservoirBuffer, pixel,
                                        g_restir.restir.bufferIndices.spatialResamplingInputBufferIndex);
      RTXDI_RandomSamplerState rng = RTXDI_InitRandomSampler(pixel, g_frame, RTXDI_PT_SPATIAL_RESAMPLING_RANDOM_SEED);
      RTXDI_PTSpatialResamplingRuntimeParameters srr = RTXDI_EmptyPTSpatialResamplingRuntimeParameters();
      srr.pixelPosition = pixel;
      srr.reservoirPosition = pixel;
      srr.cameraPos = PtSceneCamera(g_pt_scene);
      srr.prevCameraPos = PtSceneCamera(g_pt_prev_scene);
      srr.prevPrevCameraPos = srr.prevCameraPos;
      srr.viewportSize = g_size;
      RAB_PathTracerUserData ud = RAB_EmptyPathTracerUserData();
      ud.pixel = pixel;
      RAB_PathTracerUserDataSetPathType(ud, RTXDI_PTPathTraceInvocationType_Spatial);
      bool resampled = false;
      reservoir = RTXDI_PTSpatialResampling(srr, g_restir.restir.spatialResampling, g_restir.restir.hybridShift,
                                            g_restir.restir.reconnection, g_restir.restir.reservoirBuffer,
                                            g_restir.restir.bufferIndices, g_restir.runtime, rng, resampled, ud);
      if (resampled) PtCount(29u);
    }
  }
  RTXDI_StorePTReservoir(reservoir, g_restir.restir.reservoirBuffer, pixel,
                         g_restir.restir.bufferIndices.spatialResamplingOutputBufferIndex);
}
