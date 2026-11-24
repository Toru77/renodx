#define PT_COMMON_PASS 0
#define SHARC_UPDATE 0
// This software contains source code provided by NVIDIA Corporation.
#define RTXDI_ENABLE_BOILING_FILTER
#define RTXDI_BOILING_FILTER_GROUP_SIZE 8
#include "world_pt_common.hlsli"
#include "world_pt_path_tracer.hlsli"
#include <Rtxdi/Utils/RandomSamplerPerPassSeeds.hlsli>
#include <Rtxdi/PT/TemporalResampling.hlsli>
#include <Rtxdi/PT/BoilingFilter.hlsli>
#include <Rtxdi/PT/DuplicationMap.hlsli>
#include <Rtxdi/PT/Decorrelation.hlsli>

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID, uint3 group_id : SV_GroupThreadID) {
  const uint2 pixel = id.xy;
  const bool in_bounds = all(pixel < g_size);
  RTXDI_PTReservoir reservoir = RTXDI_EmptyPTReservoir();
  if (in_bounds) {
    if (g_sparse_enabled != 0u && g_sparse_levels < 256u && !PtSparseSelected(pixel)) {
      PtCount(8u);
      if (RTXDI_PTNeedsDuplicationInputs(g_restir.restir.bufferIndices.temporalResamplingOutputBufferIndex, g_restir.restir)) {
        RTXDI_PT_SAMPLE_ID_TEXTURE[pixel] = 0u;
        RTXDI_PT_DUPLICATION_MAP[pixel].y = 0.0;
      }
    } else {
      const RAB_Surface surface = RAB_GetGBufferSurface(pixel, false);
      if (RAB_IsSurfaceValid(surface)) {
        reservoir = RTXDI_LoadPTReservoir(g_restir.restir.reservoirBuffer, pixel,
                                          g_restir.restir.bufferIndices.initialPathTracerOutputBufferIndex);
        // RTV4 motion conversion, copied from world_rtxdi_temporal_di.cs_6_5.hlsl:90-104 (the authoritative block).
        const float2 motion_xy = g_pt_motion.Load(int3(pixel, 0)).xy;
        const float2 previous_position = float2(pixel) + motion_xy;
        const float4x4 previous_view = float4x4(
            asfloat(g_pt_prev_scene.Load(int3(RTXDI_SURFACE_VIEW_ROW + 0u, 0, 0))),
            asfloat(g_pt_prev_scene.Load(int3(RTXDI_SURFACE_VIEW_ROW + 1u, 0, 0))),
            asfloat(g_pt_prev_scene.Load(int3(RTXDI_SURFACE_VIEW_ROW + 2u, 0, 0))),
            asfloat(g_pt_prev_scene.Load(int3(RTXDI_SURFACE_VIEW_ROW + 3u, 0, 0))));
        const float previous_depth = -mul(previous_view, float4(surface.worldPos, 1.0)).z;
        const float depth_delta = previous_depth - RAB_GetSurfaceLinearDepth(surface);
        if (g_restir.history_valid != 0u && all(isfinite(motion_xy)) && all(isfinite(previous_position))
            && all(previous_position >= 0.0) && all(previous_position < float2(g_size))
            && all(isfinite(previous_view[0])) && all(isfinite(previous_view[1]))
            && all(isfinite(previous_view[2])) && all(isfinite(previous_view[3]))
            && isfinite(previous_depth) && previous_depth > 0.0 && isfinite(depth_delta)) {
          RTXDI_RandomSamplerState rng = RTXDI_InitRandomSampler(pixel, g_frame, RTXDI_PT_TEMPORAL_RESAMPLING_RANDOM_SEED);
          RTXDI_PTTemporalResamplingRuntimeParameters trr = RTXDI_EmptyPTTemporalResamplingRuntimeParameters();
          trr.pixelPosition = pixel;
          trr.reservoirPosition = pixel;
          trr.motionVector = float3(motion_xy, depth_delta);
          trr.cameraPos = PtSceneCamera(g_pt_scene);
          trr.prevCameraPos = PtSceneCamera(g_pt_prev_scene);
          trr.prevPrevCameraPos = trr.prevCameraPos;
          RAB_PathTracerUserData ud = RAB_EmptyPathTracerUserData();
          ud.pixel = pixel;
          RAB_PathTracerUserDataSetPathType(ud, RTXDI_PTPathTraceInvocationType_Temporal);
          bool selected_prev_sample = false;
          reservoir = RTXDI_PTTemporalResampling(
              g_restir.restir.temporalResampling, trr, g_restir.restir.hybridShift, g_restir.restir.reconnection,
              g_restir.runtime, g_restir.restir.reservoirBuffer, rng, g_restir.restir.bufferIndices,
              selected_prev_sample, ud);
          if (selected_prev_sample) PtCount(28u);
        }
      }
    }
  }
  // All threads reach the boiling filter: its group averages use wave operations and a group barrier.
  if (g_restir.restir.boilingFilter.enableBoilingFilter != 0u) {
    RTXDI_PTBoilingFilter(group_id.xy, g_restir.restir.boilingFilter.boilingFilterStrength, reservoir);
  }
  if (in_bounds) {
    if (RTXDI_PTNeedsDuplicationInputs(g_restir.restir.bufferIndices.temporalResamplingOutputBufferIndex, g_restir.restir)) {
      RTXDI_PTStoreDuplicationInputs(pixel, reservoir);
    }
    RTXDI_StorePTReservoir(reservoir, g_restir.restir.reservoirBuffer, pixel,
                           g_restir.restir.bufferIndices.temporalResamplingOutputBufferIndex);
  }
}
