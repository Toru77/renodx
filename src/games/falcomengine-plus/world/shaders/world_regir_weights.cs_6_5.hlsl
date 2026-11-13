#include "world_regir_shared.hlsli"

StructuredBuffer<RtxdiLightTableEntry> g_light_table : register(t0);
StructuredBuffer<PtReGirHeader> g_header : register(t1);
StructuredBuffer<PtReGirCell> g_cells : register(t2);
RWStructuredBuffer<float> g_weights : register(u0);
cbuffer ReGirWeightParams : register(b0) {
  uint g_light_count;
};

[numthreads(64, 1, 1)]
void main(uint3 dispatch_id : SV_DispatchThreadID) {
  uint table_count, table_stride, header_count, header_stride, cell_count, cell_stride, weight_count, weight_stride;
  g_light_table.GetDimensions(table_count, table_stride);
  g_header.GetDimensions(header_count, header_stride);
  g_cells.GetDimensions(cell_count, cell_stride);
  g_weights.GetDimensions(weight_count, weight_stride);
  const uint light_index = dispatch_id.x;
  const uint cell_index = dispatch_id.y;
  if (cell_index >= PT_REGIR_MAX_CELLS || light_index >= RTXDI_LIGHT_TABLE_CAPACITY) return;
  const uint weight_index = cell_index * RTXDI_LIGHT_TABLE_CAPACITY + light_index;
  if (weight_index >= weight_count) return;
  float weight = 0.0;
  if (header_count != 0u && g_header[0].valid != 0u && cell_index < min(g_header[0].cell_count, cell_count)
      && light_index < min(min(g_light_count, RTXDI_LIGHT_TABLE_CAPACITY), table_count)) {
    const RtxdiLightTableEntry entry = g_light_table[light_index];
    const float3 center = (float3(g_cells[cell_index].key) + 0.5) * PT_REGIR_CELL_EDGE;
    if ((entry.type == RTXDI_LIGHT_TABLE_TYPE_POINT || entry.type == RTXDI_LIGHT_TABLE_TYPE_SPOT)
        && all(isfinite(entry.position)) && isfinite(entry.radius) && entry.radius > 0.0
        && isfinite(entry.power) && entry.power > 0.0) {
      const float3 delta = max(abs(entry.position - center) - PT_REGIR_CELL_EDGE * 0.5, 0.0);
      const float candidate_weight = entry.power / max(dot(delta, delta), 1e-4);
      if (isfinite(candidate_weight) && candidate_weight > 0.0) weight = candidate_weight;
    }
  }
  g_weights[weight_index] = weight;
}