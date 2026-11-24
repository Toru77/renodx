#define PT_COMMON_PASS 0
#define SHARC_UPDATE 0
// This software contains source code provided by NVIDIA Corporation.
#include "world_pt_common.hlsli"
#include <Rtxdi/PT/DuplicationMap.hlsli>

[numthreads(16, 16, 1)]
void main(uint2 pixel : SV_DispatchThreadID) {
  if (any(pixel >= g_size)) return;
  if (g_sparse_enabled != 0u && g_sparse_levels < 256u && !PtSparseSelected(pixel)) {
    RTXDI_PT_SMOOTHED_DUPLICATION_MAP[pixel] = 0.0;
    return;
  }
  const float2 motion_xy = g_pt_motion.Load(int3(pixel, 0)).xy;
  const int2 prev_pixel = int2(round(float2(pixel) + motion_xy));
  const bool prev_valid = all(prev_pixel >= 0) && all(prev_pixel < int2(g_size));
  RTXDI_PTComputeSmoothedDuplicationMap(pixel, g_size, g_restir.restir.decorrelation.decorrelationEmaFactor, prev_pixel, prev_valid);
}