#define PT_COMMON_PASS 0
#define SHARC_UPDATE 0
// This software contains source code provided by NVIDIA Corporation.
#include "world_pt_common.hlsli"
#define RTXDI_PT_ENABLE_DUPLICATION_MAP_COUNT
#include <Rtxdi/PT/DuplicationMap.hlsli>

[numthreads(16, 16, 1)]
void main(uint2 group_id : SV_GroupID, uint2 thread_id : SV_GroupThreadID) {
  const uint2 pixel = group_id * 16u + thread_id;
  if (all(pixel < g_size) && g_sparse_enabled != 0u && g_sparse_levels < 256u && !PtSparseSelected(pixel)) {
    RTXDI_PT_SAMPLE_ID_TEXTURE[pixel] = 0u;
    RTXDI_PT_DUPLICATION_MAP[pixel].y = 0.0;
  }
  RTXDI_PTComputeDuplicationMap(group_id, thread_id, g_size);
}