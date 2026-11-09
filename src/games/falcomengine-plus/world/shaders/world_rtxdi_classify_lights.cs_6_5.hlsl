#include "world_rt_lamps.hlsli"

StructuredBuffer<LightIndexData> g_light_indices : register(t0);
RWStructuredBuffer<uint> g_lamp_types : register(u0);

cbuffer RtxdiClassifyParams : register(b12) {
  uint g_light_count;
  uint g_tile_count;
  uint g_mode;
  uint g_padding;
};

[numthreads(64, 1, 1)]
void main(uint3 id : SV_DispatchThreadID) {
  if (g_mode == 0u) {
    if (id.x < g_light_count) g_lamp_types[id.x] = 0u;
    return;
  }
  if (id.x >= g_tile_count) return;

  const LightIndexData tile = g_light_indices[id.x];
  const uint point_count = min(tile.pointLightCount, 63u);
  for (uint i = 0u; i < point_count; ++i) {
    const int source_index = tile.pointLightIndices[i];
    if (source_index >= 0 && (uint)source_index < g_light_count) {
      InterlockedOr(g_lamp_types[source_index], RTGI_LAMP_TYPE_POINT);
    }
  }

  const uint spot_count = min(tile.spotLightCount, 63u);
  for (uint i = 0u; i < spot_count; ++i) {
    const int source_index = tile.spotLightIndices[i];
    if (source_index >= 0 && (uint)source_index < g_light_count) {
      InterlockedOr(g_lamp_types[source_index], RTGI_LAMP_TYPE_SPOT);
    }
  }
}
