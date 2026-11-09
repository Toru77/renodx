#define RTXDI_ENABLE_PRESAMPLING 0
#define RTXDI_REGIR_MODE 0
#include "../rtxdi/tests/rtxdi_synthetic_bridge.hlsli"
RWStructuredBuffer<RTXDI_PackedDIReservoir> reservoirs : register(u0);
Buffer<float2> neighbors : register(t0);
#define RTXDI_LIGHT_RESERVOIR_BUFFER reservoirs
#define RTXDI_NEIGHBOR_OFFSETS_BUFFER neighbors
#include <Rtxdi/DI/InitialSampling.hlsli>
#include <Rtxdi/DI/TemporalResampling.hlsli>
#include <Rtxdi/DI/SpatialResampling.hlsli>
cbuffer Params : register(b0) {
  RTXDI_RuntimeParameters runtimeParams;
  RTXDI_ReservoirBufferParameters reservoirParams;
  RTXDI_DIInitialSamplingParameters initialParams;
  RTXDI_LightBufferParameters lightParams;
  RTXDI_DITemporalResamplingParameters temporalParams;
  RTXDI_DISpatialResamplingParameters spatialParams;
};
[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID) {
  RTXDI_RandomSamplerState rng = RTXDI_InitRandomSampler(id.xy, 0, 0);
  RTXDI_RandomSamplerState coherent = rng;
  RAB_Surface surface = RAB_GetGBufferSurface(id.xy, false);
  RAB_LightSample light;
  RTXDI_DIReservoir sample = RTXDI_SampleLightsForSurface(rng, coherent, surface, initialParams, lightParams, light);
  RTXDI_StoreDIReservoir(sample, reservoirParams, id.xy, 0);
  sample = RTXDI_LoadDIReservoir(reservoirParams, id.xy, 0);
  int2 temporalPixel;
  // Motion is pixel-space XY and previous-minus-current linear depth in Z.
  sample = RTXDI_DITemporalResampling(id.xy, surface, sample, rng, runtimeParams, reservoirParams, float3(0, 0, 0), 0, temporalParams, temporalPixel, light);
  sample = RTXDI_DISpatialResampling(id.xy, surface, sample, rng, runtimeParams, reservoirParams, 0, spatialParams, light);
  RTXDI_StoreDIReservoir(sample, reservoirParams, id.xy, 1);
}
