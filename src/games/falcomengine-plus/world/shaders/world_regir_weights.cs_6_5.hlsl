#include "../rtxdi/rtxdi_di_layout.h"

StructuredBuffer<RtxdiLightTableEntry> g_light_table : register(t0);
RWStructuredBuffer<float> g_weights : register(u0);
cbuffer ReGirWeightParams : register(b0) {
  float3 g_cell_center;
  float g_cell_half_extent;
  uint g_light_count;
};

[numthreads(64, 1, 1)]
void main(uint3 dispatch_id : SV_DispatchThreadID) {
  uint table_count, table_stride, weight_count, weight_stride;
  g_light_table.GetDimensions(table_count, table_stride);
  g_weights.GetDimensions(weight_count, weight_stride);
  const uint index = dispatch_id.x;
  if (index >= min(min(g_light_count, RTXDI_LIGHT_TABLE_CAPACITY), min(table_count, weight_count))) return;

  const RtxdiLightTableEntry entry = g_light_table[index];
  float weight = 0.0;
  if ((entry.type == RTXDI_LIGHT_TABLE_TYPE_POINT || entry.type == RTXDI_LIGHT_TABLE_TYPE_SPOT)
      && all(isfinite(g_cell_center)) && all(isfinite(entry.position))
      && isfinite(g_cell_half_extent) && g_cell_half_extent > 0.0
      && isfinite(entry.radius) && entry.radius > 0.0 && isfinite(entry.power) && entry.power > 0.0) {
    const float3 delta = max(abs(entry.position - g_cell_center) - g_cell_half_extent, 0.0);
    const float distance_squared = dot(delta, delta);
    const float candidate_weight = entry.power / max(distance_squared, 1e-4);
    if (isfinite(candidate_weight) && candidate_weight > 0.0) {
      weight = candidate_weight;
    }
  }
  g_weights[index] = weight;
}
