#ifndef FALCOM_WORLD_RTXDI_DIAG_HLSLI_
#define FALCOM_WORLD_RTXDI_DIAG_HLSLI_

#include "../rtxdi/rtxdi_diag_layout.h"
#include "../rtxdi/rtxdi_di_layout.h"

RWByteAddressBuffer g_rtxdi_diag : register(u6);

// counter must be the same for every active lane at the call site.
void RtxdiDiagAdd(uint counter, uint value) {
  if ((g_frame_flags & RTXDI_FRAME_FLAG_DIAGNOSTICS) == 0u || counter >= RTXDI_DIAG_COUNTER_COUNT) return;
  const uint total = WaveActiveSum(value);
  uint previous = 0u;
  if (WaveIsFirstLane() && total != 0u) g_rtxdi_diag.InterlockedAdd(counter * 4u, total, previous);
}

#endif  // FALCOM_WORLD_RTXDI_DIAG_HLSLI_
