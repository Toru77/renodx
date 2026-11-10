#define RTXDI_ENABLE_PRESAMPLING 0
#define RTXDI_LIGHT_RESERVOIR_BUFFER g_rtxdi_reservoirs
#define RTXDI_NEIGHBOR_OFFSETS_BUFFER g_rtxdi_neighbor_offsets
#define RTXDI_ALLOWED_BIAS_CORRECTION RTXDI_BIAS_CORRECTION_BASIC
#define RT12_TLAS_REGISTER t29
#define RT12_DYNAMIC_VERTICES_REGISTER t30

#include "world_rt12_trace.hlsli"
#include "world_rt_lamps.hlsli"
#include "world_rtxdi_lighting_consumer.hlsli"
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
Buffer<float2> g_rtxdi_neighbor_offsets : register(t22);
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
RWTexture2D<uint4> g_rtxdi_resolved_output : register(u2);

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

#include "world_rtxdi_diag.hlsli"
#include "world_rtxdi_light_bridge.hlsli"
#include <Rtxdi/DI/SpatialResampling.hlsli>
#include <Rtxdi/DI/ReservoirStorage.hlsli>

void RtxdiDiagCountEmptyTuple(uint reason_counter) {
  RtxdiDiagAdd(RTXDI_DIAG_TUPLE_STATUS_BASE + RTXDI_DI_STATUS_EMPTY, 1u);
  RtxdiDiagAdd(reason_counter, 1u);
}

[numthreads(8, 8, 1)]
void main(uint3 dispatch_id : SV_DispatchThreadID) {
  if (any(dispatch_id.xy >= g_dimensions)) return;
  const uint2 pixel = dispatch_id.xy;
  const uint pixel_index = pixel.y * g_dimensions.x + pixel.x;
  const bool spatial_enabled = (g_frame_flags & RTXDI_FRAME_FLAG_SPATIAL_ENABLED) != 0u;
  RTXDI_DIReservoir reservoir = RTXDI_EmptyDIReservoir();
  RtxdiDiRawResult raw = (RtxdiDiRawResult)0;
  raw.selected_light_index = 0xffffffffu;
  raw.status = RTXDI_DI_STATUS_EMPTY;
  raw.reason = RTXDI_DI_REASON_INVALID_SURFACE;
  g_rtxdi_resolved_output[pixel] = uint4(0xffffffffu, 0u, RTXDI_DI_STATUS_EMPTY, 0u);

  RAB_Surface surface = RAB_GetGBufferSurface(int2(pixel), false);
  const uint surface_reason = g_rtxdi_surface_diag;
  const bool surface_valid = RAB_IsSurfaceValid(surface);
  RtxdiDiagAdd(RTXDI_DIAG_SURFACE_PIXELS, 1u);
  RtxdiDiagAdd(RTXDI_DIAG_SURFACE_VALID, surface_valid);
  RtxdiDiagAdd(RTXDI_DIAG_SURFACE_CHARACTER, surface_valid && (surface.flags & RTXDI_SURFACE_FLAG_CHARACTER) != 0u);
  RtxdiDiagAdd(RTXDI_DIAG_SURFACE_TILE_INVALID, surface_valid && surface.tile_valid == 0u);
  [unroll]
  for (uint reason = 1u; reason <= RTXDI_DIAG_SURFACE_REASON_MATERIAL_NONFINITE; ++reason)
    RtxdiDiagAdd(RTXDI_DIAG_SURFACE_REASON_BASE + reason, !surface_valid && surface_reason == reason);
  if (!surface_valid) {
    if (spatial_enabled) RTXDI_StoreDIReservoir(reservoir, g_di.reservoirBufferParams, pixel, g_di.bufferIndices.spatialResamplingOutputBufferIndex);
    g_rtxdi_raw[pixel_index] = raw;
    RtxdiDiagCountEmptyTuple(RTXDI_DIAG_EMPTY_INVALID_SURFACE);
    return;
  }
  if (g_light_count == 0u) {
    raw.reason = RTXDI_DI_REASON_ZERO_LIGHTS;
    if (spatial_enabled) RTXDI_StoreDIReservoir(reservoir, g_di.reservoirBufferParams, pixel, g_di.bufferIndices.spatialResamplingOutputBufferIndex);
    g_rtxdi_raw[pixel_index] = raw;
    RtxdiDiagCountEmptyTuple(RTXDI_DIAG_EMPTY_ZERO_LIGHTS);
    return;
  }

  RAB_LightSample selected_sample = RAB_EmptyLightSample();
  if (spatial_enabled) {
    RTXDI_DIReservoir center = RTXDI_LoadDIReservoir(g_di.reservoirBufferParams, pixel,
                                                     g_di.bufferIndices.spatialResamplingInputBufferIndex);
    RTXDI_RandomSamplerState rng = RTXDI_InitRandomSampler(pixel, g_runtime.frameIndex, 2u);
    reservoir = RTXDI_DISpatialResampling(pixel, surface, center, rng, g_runtime, g_di.reservoirBufferParams,
                                          g_di.bufferIndices.spatialResamplingInputBufferIndex,
                                          g_di.spatialResamplingParams, selected_sample);
    RTXDI_StoreDIReservoir(reservoir, g_di.reservoirBufferParams, pixel,
                           g_di.bufferIndices.spatialResamplingOutputBufferIndex);
  }
  raw.reason = RTXDI_DI_REASON_INVALID_SAMPLE;
  const RTXDI_DIReservoir final_reservoir = RTXDI_LoadDIReservoir(g_di.reservoirBufferParams, pixel,
                                                                 g_di.bufferIndices.shadingInputBufferIndex);
  if (!RTXDI_IsValidDIReservoir(final_reservoir) || !isfinite(final_reservoir.weightSum)
      || final_reservoir.weightSum <= 0.0) {
    g_rtxdi_raw[pixel_index] = raw;
    RtxdiDiagCountEmptyTuple(RTXDI_DIAG_EMPTY_NO_RESERVOIR);
    return;
  }
  const uint selected_index = RTXDI_GetDIReservoirLightIndex(final_reservoir);
  const RAB_LightInfo selected_info = RAB_LoadLightInfo(selected_index, false);
  selected_sample = RAB_SamplePolymorphicLight(selected_info, surface, RTXDI_GetDIReservoirSampleUV(final_reservoir));
  if (selected_sample.valid == 0u) {
    g_rtxdi_raw[pixel_index] = raw;
    RtxdiDiagCountEmptyTuple(RTXDI_DIAG_EMPTY_SAMPLE_INVALID);
    return;
  }
  const float inverse_pdf = RTXDI_GetDIReservoirInvPdf(final_reservoir);
  const RtxdiDiRawResult result = RtxdiTraceLocalLight(surface, selected_sample, g_scene_resources_valid, g_coverage_valid,
                                                       g_environment_bias, g_character_bias, g_endpoint_clearance,
                                                       g_region_min, g_region_size, inverse_pdf);
  g_rtxdi_raw[pixel_index] = result;
  const uint4 tuple = uint4(result.selected_light_index, result.selected_light_type, result.status, asuint(result.inverse_pdf));
  g_rtxdi_resolved_output[pixel] = tuple;
  if ((g_frame_flags & RTXDI_FRAME_FLAG_DIAGNOSTICS) != 0u) {
    [unroll]
    for (uint status = RTXDI_DI_STATUS_EMPTY; status <= RTXDI_DI_STATUS_UNKNOWN; ++status)
      RtxdiDiagAdd(RTXDI_DIAG_TUPLE_STATUS_BASE + status, result.status == status ? 1u : 0u);
    RtxdiDiagAdd(RTXDI_DIAG_TUPLE_OTHER, result.status > RTXDI_DI_STATUS_UNKNOWN ? 1u : 0u);
    [unroll]
    for (uint reason = RTXDI_DI_REASON_NONE; reason <= RTXDI_DI_REASON_TRUNCATED; ++reason)
      RtxdiDiagAdd(RTXDI_DIAG_UNKNOWN_REASON_BASE + reason,
                   result.status == RTXDI_DI_STATUS_UNKNOWN && result.reason == reason ? 1u : 0u);
    const bool tuple_known = result.status == RTXDI_DI_STATUS_VISIBLE || result.status == RTXDI_DI_STATUS_BLOCKED;
    const bool consumer_valid = tuple_known && surface.tile_valid != 0u
        && RtxdiResolveLightingSelection(true, tuple, g_light_count, g_rtxdi_tiles[surface.tile_cell]).valid;
    RtxdiDiagAdd(RTXDI_DIAG_TUPLE_CONSUMER_VALID, consumer_valid ? 1u : 0u);
    RtxdiDiagAdd(RTXDI_DIAG_TUPLE_CONSUMER_REJECT, tuple_known && !consumer_valid ? 1u : 0u);
    RtxdiDiagAdd(RTXDI_DIAG_BLOCKED_NEAR_LAMP,
                 result.status == RTXDI_DI_STATUS_BLOCKED && isfinite(result.distance)
                         && selected_sample.distance - result.distance <= RTXDI_DIAG_NEAR_LAMP_METERS ? 1u : 0u);
  }
}
