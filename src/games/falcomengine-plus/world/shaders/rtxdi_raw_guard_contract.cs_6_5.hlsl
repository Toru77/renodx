#include "../rtxdi/rtxdi_di_layout.h"

RWStructuredBuffer<RtxdiDiRawResult> g_output : register(u0);

cbuffer RtxdiRawGuardParams : register(b0) {
  float3 g_finite_value;
  float g_finite_scale;
  float g_distance;
  uint g_flags;
};

[numthreads(1, 1, 1)]
void main(uint3 dispatch_id : SV_DispatchThreadID) {
  const float3 overflowed = g_finite_value * g_finite_scale;
  g_output[0] = RtxdiDiMakeVisibleRawResult(overflowed, g_distance, g_flags, 7u, 1u, 1.0);
}
