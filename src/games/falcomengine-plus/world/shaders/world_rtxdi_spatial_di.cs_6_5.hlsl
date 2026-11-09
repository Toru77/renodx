#define RTXDI_ENABLE_PRESAMPLING 0
#define RTXDI_LIGHT_RESERVOIR_BUFFER g_rtxdi_reservoirs
#define RTXDI_NEIGHBOR_OFFSETS_BUFFER g_rtxdi_neighbor_offsets
#define RTXDI_ALLOWED_BIAS_CORRECTION RTXDI_BIAS_CORRECTION_BASIC
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

#include "world_rtxdi_light_bridge.hlsli"
#include <Rtxdi/DI/SpatialResampling.hlsli>
#include <Rtxdi/DI/ReservoirStorage.hlsli>

[numthreads(8, 8, 1)]
void main(uint3 dispatch_id : SV_DispatchThreadID) {
  if (any(dispatch_id.xy >= g_dimensions)) return;
  const uint2 pixel = dispatch_id.xy;
  const uint pixel_index = pixel.y * g_dimensions.x + pixel.x;
  RTXDI_DIReservoir reservoir = RTXDI_EmptyDIReservoir();
  RtxdiDiRawResult raw = (RtxdiDiRawResult)0;
  raw.selected_light_index = 0xffffffffu;
  raw.status = RTXDI_DI_STATUS_EMPTY;
  raw.reason = RTXDI_DI_REASON_INVALID_SURFACE;
  g_rtxdi_resolved_output[pixel] = uint4(0xffffffffu, 0u, RTXDI_DI_STATUS_EMPTY, 0u);

  RAB_Surface surface = RAB_GetGBufferSurface(int2(pixel), false);
  if (!RAB_IsSurfaceValid(surface)) {
    RTXDI_StoreDIReservoir(reservoir, g_di.reservoirBufferParams, pixel, g_di.bufferIndices.spatialResamplingOutputBufferIndex);
    g_rtxdi_raw[pixel_index] = raw;
    return;
  }
  if (g_light_count == 0u) {
    raw.reason = RTXDI_DI_REASON_ZERO_LIGHTS;
    RTXDI_StoreDIReservoir(reservoir, g_di.reservoirBufferParams, pixel, g_di.bufferIndices.spatialResamplingOutputBufferIndex);
    g_rtxdi_raw[pixel_index] = raw;
    return;
  }

  RTXDI_DIReservoir center = RTXDI_LoadDIReservoir(g_di.reservoirBufferParams, pixel,
                                                   g_di.bufferIndices.spatialResamplingInputBufferIndex);
  RTXDI_RandomSamplerState rng = RTXDI_InitRandomSampler(pixel, g_runtime.frameIndex, 2u);
  RAB_LightSample selected_sample = RAB_EmptyLightSample();
  reservoir = RTXDI_DISpatialResampling(pixel, surface, center, rng, g_runtime, g_di.reservoirBufferParams,
                                        g_di.bufferIndices.spatialResamplingInputBufferIndex,
                                        g_di.spatialResamplingParams, selected_sample);
  RTXDI_StoreDIReservoir(reservoir, g_di.reservoirBufferParams, pixel, g_di.bufferIndices.spatialResamplingOutputBufferIndex);
  raw.reason = RTXDI_DI_REASON_INVALID_SAMPLE;
  const RTXDI_DIReservoir final_reservoir = RTXDI_LoadDIReservoir(g_di.reservoirBufferParams, pixel,
                                                                 g_di.bufferIndices.shadingInputBufferIndex);
  if (!RTXDI_IsValidDIReservoir(final_reservoir) || !isfinite(final_reservoir.weightSum)
      || final_reservoir.weightSum <= 0.0) {
    g_rtxdi_raw[pixel_index] = raw;
    return;
  }
  const uint selected_index = RTXDI_GetDIReservoirLightIndex(final_reservoir);
  const RAB_LightInfo selected_info = RAB_LoadLightInfo(selected_index, false);
  selected_sample = RAB_SamplePolymorphicLight(selected_info, surface, RTXDI_GetDIReservoirSampleUV(final_reservoir));
  if (selected_sample.valid == 0u) {
    g_rtxdi_raw[pixel_index] = raw;
    return;
  }
  const float inverse_pdf = RTXDI_GetDIReservoirInvPdf(final_reservoir);
  const RtxdiDiRawResult result = RtxdiTraceLocalLight(surface, selected_sample, g_scene_resources_valid, g_coverage_valid,
                                                       g_environment_bias, g_character_bias, g_endpoint_clearance,
                                                       g_region_min, g_region_size, inverse_pdf);
  g_rtxdi_raw[pixel_index] = result;
  g_rtxdi_resolved_output[pixel] = uint4(result.selected_light_index, result.selected_light_type, result.status,
                                         asuint(result.inverse_pdf));
}
