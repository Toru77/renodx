#define PT_COMMON_PASS 0
#define SHARC_UPDATE 0
// This software contains source code provided by NVIDIA Corporation.
#include "world_pt_common.hlsli"
#if defined(PT_WATER_SURFACE)
#include "world_pt_bridge.hlsli"
#endif
#include <Rtxdi/PT/DuplicationMap.hlsli>

[numthreads(16, 16, 1)]
void main(uint2 pixel : SV_DispatchThreadID) {
  if (any(pixel >= g_size)) return;
  if (g_sparse_enabled != 0u && g_sparse_levels < 256u && !PtSparseSelected(pixel)) {
    RTXDI_PT_SMOOTHED_DUPLICATION_MAP[pixel] = 0.0;
    return;
  }
#if defined(PT_WATER_SURFACE)
  const RAB_Surface surface = RAB_GetGBufferSurface(pixel, false);
  const bool water_valid = RAB_IsSurfaceValid(surface);
  const float2 motion_xy = water_valid ? PtWaterCameraMotion(pixel, surface.worldPos) : float2(0.0, 0.0);
#else
  const float2 motion_xy = g_pt_motion.Load(int3(pixel, 0)).xy;
#endif
  const int2 prev_pixel = int2(round(float2(pixel) + motion_xy));
#if defined(PT_WATER_SURFACE)
  const bool prev_valid = water_valid && all(prev_pixel >= 0) && all(prev_pixel < int2(g_size));
#else
  const bool prev_valid = all(prev_pixel >= 0) && all(prev_pixel < int2(g_size));
#endif
  RTXDI_PTComputeSmoothedDuplicationMap(pixel, g_size, g_restir.restir.decorrelation.decorrelationEmaFactor, prev_pixel, prev_valid);
}