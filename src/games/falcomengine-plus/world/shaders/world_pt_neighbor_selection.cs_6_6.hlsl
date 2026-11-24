#define PT_COMMON_PASS 0
#define SHARC_UPDATE 0
// This software contains source code provided by NVIDIA Corporation.
#include "world_pt_common.hlsli"
#include "world_pt_bridge.hlsli"
#include <Rtxdi/PT/SpatialNeighborSelection.hlsli>

[numthreads(8, 8, 1)]
void main(uint2 pixel : SV_DispatchThreadID) {
  if (any(pixel >= g_size)) return;
  RTXDI_PTSpatialNeighborSelection(pixel, g_size, g_frame, g_restir.restir.spatialResampling.numSpatialSamples);
}