#include "world_regir_shared.hlsli"

StructuredBuffer<PtReGirHeader> g_header : register(t0);
StructuredBuffer<float> g_weights : register(t1);
RWStructuredBuffer<PtReGirCell> g_cells : register(u0);
cbuffer ReGirBuildParams : register(b0) {
  uint g_light_count;
  uint g_frame;
};

[numthreads(64, 1, 1)]
void main(uint3 dispatch_id : SV_DispatchThreadID) {
  uint header_count, header_stride, weight_count, weight_stride, cell_count, cell_stride;
  g_header.GetDimensions(header_count, header_stride);
  g_weights.GetDimensions(weight_count, weight_stride);
  g_cells.GetDimensions(cell_count, cell_stride);
  const uint cell_index = dispatch_id.x;
  if (cell_index >= min(cell_count, PT_REGIR_MAX_CELLS)) return;
  PtReGirCell cell = g_cells[cell_index];
  cell.valid = 0u;
  for (uint slot = 0u; slot < PT_REGIR_SLOTS; ++slot) cell.slots[slot] = RTXDI_LIGHT_TABLE_INVALID_INDEX;
  const uint count = min(g_light_count, RTXDI_LIGHT_TABLE_CAPACITY);
  const uint offset = cell_index * RTXDI_LIGHT_TABLE_CAPACITY;
  if (header_count != 0u && g_header[0].valid != 0u && cell_index < g_header[0].cell_count
      && g_light_count == count && count != 0u && offset <= weight_count && count <= weight_count - offset) {
    float total = 0.0;
    for (uint light_index = 0u; light_index < count; ++light_index) {
      const float weight = g_weights[offset + light_index];
      if (isfinite(weight) && weight > 0.0) total += weight;
    }
    if (isfinite(total) && total > 0.0) {
      cell.valid = 1u;
      float draws[PT_REGIR_SLOTS];
      for (uint slot = 0u; slot < PT_REGIR_SLOTS; ++slot) {
        uint hash = cell_index ^ (g_frame * 0x9e3779b9u) ^ (slot * 0x85ebca6bu);
        hash ^= hash >> 16; hash *= 0x7feb352du; hash ^= hash >> 15; hash *= 0x846ca68bu; hash ^= hash >> 16;
        draws[slot] = float(hash >> 8) * (1.0 / 16777216.0) * total;
      }
      float cumulative = 0.0;
      uint assigned = 0u;
      for (uint light_index = 0u; light_index < count && assigned < PT_REGIR_SLOTS; ++light_index) {
        const float weight = g_weights[offset + light_index];
        if (isfinite(weight) && weight > 0.0) cumulative += weight;
        for (uint slot = 0u; slot < PT_REGIR_SLOTS; ++slot) {
          if (cell.slots[slot] == RTXDI_LIGHT_TABLE_INVALID_INDEX && draws[slot] < cumulative) {
            cell.slots[slot] = light_index;
            ++assigned;
          }
        }
      }
      for (uint slot = 0u; slot < PT_REGIR_SLOTS; ++slot) {
        if (cell.slots[slot] >= count) cell.valid = 0u;
      }
    }
  }
  g_cells[cell_index] = cell;
}
