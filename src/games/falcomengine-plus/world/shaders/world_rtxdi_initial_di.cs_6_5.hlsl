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
#include <Rtxdi/DI/InitialSampling.hlsli>
#include <Rtxdi/DI/ReservoirStorage.hlsli>

void RtxdiWriteInitialRaw(uint pixel_index, RtxdiDiRawResult result) {
  if ((g_frame_flags & RTXDI_FRAME_FLAG_SPATIAL_AFTER_INITIAL) == 0u) g_rtxdi_raw[pixel_index] = result;
}

[numthreads(8, 8, 1)]
void main(uint3 dispatch_id : SV_DispatchThreadID) {
  if (any(dispatch_id.xy >= g_dimensions)) return;
  const uint2 pixel = dispatch_id.xy;
  const uint pixel_index = pixel.y * g_dimensions.x + pixel.x;
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

  RTXDI_RandomSamplerState rng = RTXDI_InitRandomSampler(pixel, g_runtime.frameIndex, 0u);
  RTXDI_RandomSamplerState coherent_rng = RTXDI_InitRandomSampler(pixel, g_runtime.frameIndex, 1u);
  RAB_LightSample selected_sample = RAB_EmptyLightSample();
  RTXDI_InitialSamplingMisData mis_data = RTXDI_ComputeInitialSamplingMisData(g_di.initialSamplingParams);
  RTXDI_LightBufferRegion light_region = {0u, g_light_count, 0u, 0u};
  reservoir = RTXDI_SampleLocalLights(rng, coherent_rng, surface, g_di.initialSamplingParams, mis_data,
                                     ReSTIRDI_LocalLightSamplingMode_UNIFORM, light_region, selected_sample);
  if (!RTXDI_IsValidDIReservoir(reservoir) || selected_sample.valid == 0u
      || !isfinite(reservoir.weightSum) || reservoir.weightSum <= 0.0) {
    RtxdiDiRawResult raw = (RtxdiDiRawResult)0;
    raw.selected_light_index = 0xffffffffu;
    raw.status = RTXDI_DI_STATUS_EMPTY;
    raw.reason = RTXDI_DI_REASON_INVALID_SAMPLE;
    RtxdiWriteInitialRaw(pixel_index, raw);
    return;
  }
  RTXDI_StoreDIReservoir(reservoir, g_di.reservoirBufferParams, pixel, g_di.bufferIndices.initialSamplingOutputBufferIndex);
  if ((g_frame_flags & RTXDI_FRAME_FLAG_SPATIAL_AFTER_INITIAL) != 0u) return;
  RtxdiWriteInitialRaw(pixel_index,
                       RtxdiTraceLocalLight(surface, selected_sample, g_scene_resources_valid, g_coverage_valid,
                                            g_environment_bias, g_character_bias, g_endpoint_clearance, g_region_min,
                                            g_region_size, RTXDI_GetDIReservoirInvPdf(reservoir)));
}
