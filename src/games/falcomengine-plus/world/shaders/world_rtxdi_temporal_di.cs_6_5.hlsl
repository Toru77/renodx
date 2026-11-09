#define RTXDI_ENABLE_PRESAMPLING 0
#define RTXDI_LIGHT_RESERVOIR_BUFFER g_rtxdi_reservoirs
#define RT12_TLAS_REGISTER t29
#define RT12_DYNAMIC_VERTICES_REGISTER t30

#include "world_rt12_trace.hlsli"
#include "world_rt_lamps.hlsli"
#include "world_rtxdi_surface.hlsli"
#include "../rtxdi/rtxdi_di_layout.h"
#include <Rtxdi/DI/ReSTIRDIParameters.h>
#include <Rtxdi/DI/Reservoir.hlsli>
#include <Rtxdi/Utils/RandomSamplerState.hlsli>

StructuredBuffer<LightParam> g_rtxdi_lights : register(t17);
StructuredBuffer<uint> g_rtxdi_light_types : register(t18);
Texture2D<float4> g_rtxdi_color : register(t19);
Texture2D<uint2> g_rtxdi_mrt2 : register(t20);
StructuredBuffer<GameDeferredParam> g_rtxdi_deferred : register(t21);
Texture2D<float> g_rtxdi_depth : register(t31);
Texture2D<uint4> g_rtxdi_normal : register(t32);
Texture2D<uint4> g_rtxdi_scene : register(t33);
Texture2D<float4> g_rtxdi_motion : register(t34);
Texture2D<float> g_rtxdi_prev_depth : register(t35);
Texture2D<uint4> g_rtxdi_prev_normal : register(t36);
Texture2D<uint4> g_rtxdi_prev_scene : register(t37);
Texture2D<float4> g_rtxdi_prev_color : register(t38);
Texture2D<uint4> g_rtxdi_prev_mrt2 : register(t39);
StructuredBuffer<GameDeferredParam> g_rtxdi_prev_deferred : register(t40);
StructuredBuffer<LightParam> g_rtxdi_prev_lights : register(t41);
StructuredBuffer<uint> g_rtxdi_prev_light_types : register(t42);
StructuredBuffer<LightIndexData> g_rtxdi_tiles : register(t43);
StructuredBuffer<LightIndexData> g_rtxdi_prev_tiles : register(t44);

RWStructuredBuffer<RTXDI_PackedDIReservoir> g_rtxdi_reservoirs : register(u0);
RWStructuredBuffer<RtxdiDiRawResult> g_rtxdi_raw : register(u1);

cbuffer RtxdiDiParams : register(b0) {
  uint2 g_dimensions;
  uint g_scene_resources_valid;
  uint g_coverage_valid;
  uint g_light_count;
  float g_environment_bias;
  float g_character_bias;
  float g_endpoint_clearance;
  float3 g_region_min;
  float g_region_size;
  uint g_deferred_count;
  uint g_prev_deferred_count;
  uint g_prev_light_count;
  uint g_tile_count;
  uint g_prev_tile_count;
};

cbuffer RtxdiFrameParameters : register(b1) {
  uint2 g_frame_dimensions;
  uint g_history_valid;
  uint g_frame_flags;
  RTXDI_RuntimeParameters g_runtime;
  RTXDI_LightBufferParameters g_light_buffer_params;
  RTXDI_Parameters g_di;
};

#include "world_rtxdi_light_bridge.hlsli"
#include <Rtxdi/DI/TemporalResampling.hlsli>
#include <Rtxdi/DI/ReservoirStorage.hlsli>

[numthreads(8, 8, 1)]
void main(uint3 dispatch_id : SV_DispatchThreadID) {
  if (any(dispatch_id.xy >= g_dimensions)) return;
  const uint2 pixel = dispatch_id.xy;
  const RAB_Surface surface = RAB_GetGBufferSurface(int2(pixel), false);
  RTXDI_DIReservoir current = RTXDI_LoadDIReservoir(g_di.reservoirBufferParams, pixel,
                                                    g_di.bufferIndices.initialSamplingOutputBufferIndex);
  if (!RAB_IsSurfaceValid(surface) || g_history_valid == 0u) {
    RTXDI_StoreDIReservoir(current, g_di.reservoirBufferParams, pixel,
                           g_di.bufferIndices.temporalResamplingOutputBufferIndex);
    return;
  }

  const float2 motion_xy = g_rtxdi_motion.Load(int3(pixel, 0)).xy;
  const float2 previous_position = float2(pixel) + motion_xy;
  if (!all(isfinite(motion_xy)) || !all(isfinite(previous_position))
      || any(previous_position < 0.0) || any(previous_position >= float2(g_dimensions))) {
    RTXDI_StoreDIReservoir(current, g_di.reservoirBufferParams, pixel,
                           g_di.bufferIndices.temporalResamplingOutputBufferIndex);
    return;
  }

  float4x4 previous_view = float4x4(
      asfloat(g_rtxdi_prev_scene.Load(int3(RTXDI_SURFACE_VIEW_ROW + 0u, 0, 0))),
      asfloat(g_rtxdi_prev_scene.Load(int3(RTXDI_SURFACE_VIEW_ROW + 1u, 0, 0))),
      asfloat(g_rtxdi_prev_scene.Load(int3(RTXDI_SURFACE_VIEW_ROW + 2u, 0, 0))),
      asfloat(g_rtxdi_prev_scene.Load(int3(RTXDI_SURFACE_VIEW_ROW + 3u, 0, 0))));
  const float previous_depth = -mul(float4(surface.world_position, 1.0), previous_view).z;
  const float depth_delta = previous_depth - surface.linear_view_depth;
  if (!all(isfinite(previous_view[0])) || !all(isfinite(previous_view[1]))
      || !all(isfinite(previous_view[2])) || !all(isfinite(previous_view[3]))
      || !isfinite(previous_depth) || previous_depth <= 0.0 || !isfinite(depth_delta)) {
    RTXDI_StoreDIReservoir(current, g_di.reservoirBufferParams, pixel,
                           g_di.bufferIndices.temporalResamplingOutputBufferIndex);
    return;
  }

  // The pinned DI temporal pass tests depth and normal but has no material
  // similarity setting. Mirror its 9-trial search with a copied RNG, and reject
  // before the SDK call if its first acceptable history surface has a different
  // material/class. Leave the real RNG untouched so the SDK repeats the same
  // jitter and candidate sequence.
  RTXDI_RandomSamplerState probe_rng = RTXDI_InitRandomSampler(pixel, g_runtime.frameIndex, 3u);
  float2 probe_motion = motion_xy;
  if (!g_di.temporalResamplingParams.enablePermutationSampling) {
    probe_motion += float2(RTXDI_GetNextRandom(probe_rng), RTXDI_GetNextRandom(probe_rng)) - 0.5;
  }
  const int2 probe_prev_pos = int2(round(float2(pixel) + probe_motion));
  const float radius = g_runtime.activeCheckerboardField == 0u ? 4.0 : 8.0;
  int2 probe_candidate_pixel = int2(-1, -1);
  for (int i = 0; i < 9; ++i) {
    int2 offset = int2(0, 0);
    if (i > 0) {
      offset.x = int((RTXDI_GetNextRandom(probe_rng) - 0.5) * radius);
      offset.y = int((RTXDI_GetNextRandom(probe_rng) - 0.5) * radius);
    }
    int2 candidate_pixel = probe_prev_pos + offset;
    if (g_di.temporalResamplingParams.enablePermutationSampling && i == 0) {
      RTXDI_ApplyPermutationSampling(candidate_pixel, g_di.temporalResamplingParams.uniformRandomNumber);
    }
    RTXDI_ActivateCheckerboardPixel(candidate_pixel, true, g_runtime.activeCheckerboardField);
    const RAB_Surface previous_surface = RAB_GetGBufferSurface(candidate_pixel, true);
    if (!RAB_IsSurfaceValid(previous_surface)
        || !RTXDI_IsValidNeighbor(RAB_GetSurfaceNormal(surface), RAB_GetSurfaceNormal(previous_surface),
                                  previous_depth, RAB_GetSurfaceLinearDepth(previous_surface),
                                  g_di.temporalResamplingParams.normalThreshold,
                                  g_di.temporalResamplingParams.depthThreshold)) continue;

    probe_candidate_pixel = candidate_pixel;
    if (!RAB_AreMaterialsSimilar(RAB_GetMaterial(surface), RAB_GetMaterial(previous_surface))) {
      RTXDI_StoreDIReservoir(current, g_di.reservoirBufferParams, pixel,
                             g_di.bufferIndices.temporalResamplingOutputBufferIndex);
      return;
    }
    break;
  }

  RTXDI_RandomSamplerState rng = RTXDI_InitRandomSampler(pixel, g_runtime.frameIndex, 3u);
  RAB_LightSample selected = RAB_EmptyLightSample();
  int2 temporal_sample_pixel = int2(-1, -1);
  const RTXDI_DIReservoir temporal = RTXDI_DITemporalResampling(
      pixel, surface, current, rng, g_runtime, g_di.reservoirBufferParams,
      float3(motion_xy, depth_delta), g_di.bufferIndices.temporalResamplingInputBufferIndex,
      g_di.temporalResamplingParams, temporal_sample_pixel, selected);
  if (all(temporal_sample_pixel >= 0) && all(probe_candidate_pixel >= 0)
      && any(temporal_sample_pixel != probe_candidate_pixel)) {
    RTXDI_StoreDIReservoir(current, g_di.reservoirBufferParams, pixel,
                           g_di.bufferIndices.temporalResamplingOutputBufferIndex);
    return;
  }
  RTXDI_StoreDIReservoir(temporal, g_di.reservoirBufferParams, pixel,
                         g_di.bufferIndices.temporalResamplingOutputBufferIndex);
}
