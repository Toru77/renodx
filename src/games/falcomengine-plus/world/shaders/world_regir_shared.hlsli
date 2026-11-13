#ifndef WORLD_REGIR_SHARED_HLSLI
#define WORLD_REGIR_SHARED_HLSLI

#include "../rtxdi/rtxdi_di_layout.h"

#define PT_REGIR_MAX_POSITIONS 4096u
#define PT_REGIR_MAX_CELLS 256u
#define PT_REGIR_SLOTS 8u
#define PT_REGIR_CELL_EDGE 4.0
#define PT_REGIR_COORDINATE_LIMIT 1048576.0

struct PtReGirHeader {
  uint cell_count;
  uint rejected_position_count;
  uint overflow_position_count;
  uint valid;
};

struct PtReGirCell {
  int3 key;
  uint valid;
  uint slots[PT_REGIR_SLOTS];
};

bool PtReGirCellKey(float3 position, out int3 key) {
  key = 0;
  if (!all(isfinite(position)) || any(position < -PT_REGIR_COORDINATE_LIMIT)
      || any(position >= PT_REGIR_COORDINATE_LIMIT)) return false;
  key = int3(floor(position / PT_REGIR_CELL_EDGE));
  return true;
}
struct PtReGirSelection {
  uint valid;
  uint light_index;
  uint c20;
  uint multiplicity;
  float pmf;
};

PtReGirSelection PtReGirSelect(uint x24, uint light_count, uint slots[8]) {
  PtReGirSelection result = (PtReGirSelection)0;
  if (x24 >= (1u << 24) || light_count == 0 || light_count > RTXDI_LIGHT_TABLE_CAPACITY) return result;
  for (uint slot = 0; slot < 8; ++slot) {
    if (slots[slot] >= light_count) return result;
  }
  result.light_index = (x24 & (1u << 23)) == 0
                           ? slots[(x24 >> 20) & 7u]
                           : uint((uint64_t(x24 & 0xfffffu) * uint64_t(light_count)) >> 20);
  for (uint slot = 0; slot < 8; ++slot) {
    result.multiplicity += slots[slot] == result.light_index ? 1u : 0u;
  }
  const uint64_t count = uint64_t(light_count);
  const uint64_t lower = (uint64_t(result.light_index) * uint64_t(1u << 20) + count - 1u) / count;
  const uint64_t upper = (uint64_t(result.light_index + 1u) * uint64_t(1u << 20) + count - 1u) / count;
  result.c20 = uint(upper - lower);
  result.pmf = 0.5 * float(result.multiplicity) / 8.0 + 0.5 * float(result.c20) / float(1u << 20);
  result.valid = isfinite(result.pmf) && result.pmf > 0.0 ? 1u : 0u;
  return result;
}

#endif
