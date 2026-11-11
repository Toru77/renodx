#define RTXDI_DIAG_INITIAL_PASS 1
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
Texture2D<uint4> g_rtxdi_local_noise : register(t61);

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
  float g_local_light_size;
  uint g_noise_mode;
  uint g_noise_slice;
  float g_character_self_softening;
};

cbuffer RtxdiFrameParameters : register(b1) {
  uint2 g_frame_dimensions;
  uint g_history_valid;
  uint g_frame_flags;
  RTXDI_RuntimeParameters g_runtime;
  RTXDI_LightBufferParameters g_light_buffer_params;
  RTXDI_Parameters g_di;
};

#include "world_rtxdi_diag.hlsli"
#include "world_rtxdi_light_bridge.hlsli"
#include <Rtxdi/DI/InitialSampling.hlsli>
#include <Rtxdi/DI/ReservoirStorage.hlsli>

void RtxdiWriteInitialRaw(uint pixel_index, RtxdiDiRawResult result) {
  if ((g_frame_flags & RTXDI_FRAME_FLAG_SPATIAL_AFTER_INITIAL) == 0u) g_rtxdi_raw[pixel_index] = result;
}

bool RtxdiTileEntryUsable(int listed, bool point_entry) {
  return listed >= 0 && uint(listed) < g_light_count
         && g_rtxdi_light_types[uint(listed)] == (point_entry ? RTGI_LAMP_TYPE_POINT : RTGI_LAMP_TYPE_SPOT);
}

[numthreads(8, 8, 1)]
void main(uint3 dispatch_id : SV_DispatchThreadID) {
  if (any(dispatch_id.xy >= g_dimensions)) return;
  const uint2 pixel = dispatch_id.xy;
  const uint pixel_index = pixel.y * g_dimensions.x + pixel.x;
  if ((g_frame_flags & RTXDI_FRAME_FLAG_DIAGNOSTICS) != 0u) {
    const bool light_thread = pixel_index < g_light_count;
    const uint light_type = light_thread ? g_rtxdi_light_types[pixel_index] : 0u;
    const float light_radius = light_thread ? g_rtxdi_lights[pixel_index].radius : 0.0;
    const bool radius_positive = light_thread && isfinite(light_radius) && light_radius > 0.0;
    RtxdiDiagAdd(RTXDI_DIAG_LIGHT_TYPE_NONE, light_thread && light_type == 0u);
    RtxdiDiagAdd(RTXDI_DIAG_LIGHT_TYPE_POINT, light_thread && light_type == RTGI_LAMP_TYPE_POINT);
    RtxdiDiagAdd(RTXDI_DIAG_LIGHT_TYPE_SPOT, light_thread && light_type == RTGI_LAMP_TYPE_SPOT);
    RtxdiDiagAdd(RTXDI_DIAG_LIGHT_TYPE_BOTH, light_thread && light_type == (RTGI_LAMP_TYPE_POINT | RTGI_LAMP_TYPE_SPOT));
    RtxdiDiagAdd(RTXDI_DIAG_LIGHT_RADIUS_POSITIVE, radius_positive);
    RtxdiDiagAdd(RTXDI_DIAG_LIGHT_TYPED_RADIUS_POSITIVE, radius_positive && light_type != 0u);
    const bool tile_thread = pixel_index < g_tile_count;
    const uint point_refs = tile_thread ? min(g_rtxdi_tiles[pixel_index].pointLightCount, 63u) : 0u;
    const uint spot_refs = tile_thread ? min(g_rtxdi_tiles[pixel_index].spotLightCount, 63u) : 0u;
    RtxdiDiagAdd(RTXDI_DIAG_TILE_CELLS_NONEMPTY, point_refs + spot_refs != 0u);
    RtxdiDiagAdd(RTXDI_DIAG_TILE_POINT_REFS, point_refs);
    RtxdiDiagAdd(RTXDI_DIAG_TILE_SPOT_REFS, spot_refs);
  }
  RTXDI_DIReservoir reservoir = RTXDI_EmptyDIReservoir();
  RTXDI_StoreDIReservoir(reservoir, g_di.reservoirBufferParams, pixel, g_di.bufferIndices.initialSamplingOutputBufferIndex);

  RAB_Surface surface = RAB_GetGBufferSurface(int2(pixel), false);
  if (!RAB_IsSurfaceValid(surface)) {
    RtxdiDiRawResult raw = (RtxdiDiRawResult)0;
    raw.selected_light_index = 0xffffffffu;
    raw.status = RTXDI_DI_STATUS_EMPTY;
    raw.reason = RTXDI_DI_REASON_INVALID_SURFACE;
    RtxdiWriteInitialRaw(pixel_index, raw);
    return;
  }
  if (g_light_count == 0u) {
    RtxdiDiRawResult raw = (RtxdiDiRawResult)0;
    raw.selected_light_index = 0xffffffffu;
    raw.status = RTXDI_DI_STATUS_EMPTY;
    raw.reason = RTXDI_DI_REASON_ZERO_LIGHTS;
    RtxdiWriteInitialRaw(pixel_index, raw);
    return;
  }

  RtxdiDiagAdd(RTXDI_DIAG_INITIAL_SURFACES, 1u);
  RTXDI_RandomSamplerState rng = RTXDI_InitRandomSampler(pixel, g_runtime.frameIndex, 0u);
  RAB_LightSample selected_sample = RAB_EmptyLightSample();
  RTXDI_InitialSamplingMisData mis_data = RTXDI_ComputeInitialSamplingMisData(g_di.initialSamplingParams);
  // Candidates come from this pixel's game tile cell: the only lights with a non-zero target.
  if (surface.tile_valid != 0u && g_di.initialSamplingParams.numLocalLightSamples != 0u) {
    const LightIndexData tile = g_rtxdi_tiles[surface.tile_cell];
    const uint point_count = min(tile.pointLightCount, 63u);
    const uint entry_count = point_count + min(tile.spotLightCount, 63u);
    uint usable_count = 0u;
    for (uint entry = 0u; entry < entry_count; ++entry) {
      const bool point_entry = entry < point_count;
      const int listed = point_entry ? tile.pointLightIndices[entry] : tile.spotLightIndices[entry - point_count];
      usable_count += RtxdiTileEntryUsable(listed, point_entry) ? 1u : 0u;
    }
    if (usable_count != 0u) {
      for (uint candidate = 0u; candidate < g_di.initialSamplingParams.numLocalLightSamples; ++candidate) {
        const uint selected_rank = min(uint(RTXDI_GetNextRandom(rng) * float(usable_count)), usable_count - 1u);
        uint light_index = g_light_count;
        uint rank = 0u;
        for (uint entry = 0u; entry < entry_count; ++entry) {
          const bool point_entry = entry < point_count;
          const int listed = point_entry ? tile.pointLightIndices[entry] : tile.spotLightIndices[entry - point_count];
          const bool usable = RtxdiTileEntryUsable(listed, point_entry);
          if (usable && rank == selected_rank) light_index = uint(listed);
          rank += usable ? 1u : 0u;
        }
        const RAB_LightInfo light_info = RAB_LoadLightInfo(light_index, false);
        float2 uv;
        if (g_noise_mode != 0u) {
          uv = asfloat(g_rtxdi_local_noise.Load(int3(pixel.x % 128u, pixel.y % 128u + 128u * g_noise_slice, 0)).xy);
        } else {
          uv = RTXDI_RandomlySelectLocalLightUV(rng);
        }
        RTXDI_StreamLocalLightAtUVIntoReservoir(rng, mis_data, surface, g_di.initialSamplingParams.brdfCutoff,
                                                mis_data.localLightMisWeight, light_index, uv, float(usable_count),
                                                light_info, reservoir, selected_sample);
      }
      RTXDI_FinalizeResampling(reservoir, 1.0, mis_data.numMisSamples);
      reservoir.M = 1;
    }
  }
  if (!RTXDI_IsValidDIReservoir(reservoir) || selected_sample.valid == 0u
      || !isfinite(reservoir.weightSum) || reservoir.weightSum <= 0.0) {
    RtxdiDiRawResult raw = (RtxdiDiRawResult)0;
    raw.selected_light_index = 0xffffffffu;
    raw.status = RTXDI_DI_STATUS_EMPTY;
    raw.reason = RTXDI_DI_REASON_INVALID_SAMPLE;
    RtxdiWriteInitialRaw(pixel_index, raw);
    return;
  }
  RtxdiDiagAdd(RTXDI_DIAG_INITIAL_RESERVOIRS, 1u);
  RTXDI_StoreDIReservoir(reservoir, g_di.reservoirBufferParams, pixel, g_di.bufferIndices.initialSamplingOutputBufferIndex);
  if ((g_frame_flags & RTXDI_FRAME_FLAG_SPATIAL_AFTER_INITIAL) != 0u) return;
  RtxdiWriteInitialRaw(pixel_index,
                       RtxdiTraceLocalLight(surface, selected_sample, g_scene_resources_valid, g_coverage_valid,
                                            g_environment_bias, g_character_bias, g_endpoint_clearance, g_region_min,
                                            g_region_size, RTXDI_GetDIReservoirInvPdf(reservoir)));
}
