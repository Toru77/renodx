#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <span>

#include "../rtxdi/rtxdi_di_layout.h"

namespace falcom_world::pt {

inline constexpr uint32_t kPtReGirMaxPositions = 4096u;
inline constexpr uint32_t kPtReGirMaxCells = 256u;
inline constexpr uint32_t kPtReGirSlotsPerCell = 8u;
inline constexpr float kPtReGirCellEdge = 4.f;
inline constexpr float kPtReGirCoordinateLimit = 1048576.f;

struct PtReGirIdentity {
  uint64_t owner_generation = 0;
  uint64_t transfer_configuration = 0;
  uint64_t table_fence = 0;
  uint32_t frame = 0;
  uint32_t transfer_slot = 0;
  uint32_t light_count = 0;
};

struct PtReGirCellKey {
  int32_t x = 0;
  int32_t y = 0;
  int32_t z = 0;
  bool operator==(const PtReGirCellKey&) const = default;
};

struct PtReGirSlot {
  uint32_t light_index = RTXDI_LIGHT_TABLE_INVALID_INDEX;
  uint64_t ticket_count = 0;
};

struct PtReGirCell {
  PtReGirCellKey key = {};
  std::array<PtReGirSlot, kPtReGirSlotsPerCell> slots = {};
  bool valid = false;
};

struct PtReGirBuild {
  PtReGirIdentity identity = {};
  std::array<PtReGirCell, kPtReGirMaxCells> cells = {};
  uint32_t cell_count = 0;
  uint32_t rejected_position_count = 0;
  uint32_t overflow_position_count = 0;
  bool valid = false;
};

inline bool BuildPtReGirReference(const PtReGirIdentity& identity, std::span<const std::array<float, 3>> positions,
                                  std::span<const RtxdiLightTableEntry> lights, std::span<const uint32_t> draws,
                                  PtReGirBuild* output) {
  if (output == nullptr) return false;
  *output = {};
  if (identity.owner_generation == 0 || identity.transfer_configuration == 0 || identity.table_fence == 0
      || identity.frame == UINT32_MAX || identity.transfer_slot >= 3 || identity.light_count > RTXDI_LIGHT_TABLE_CAPACITY
      || positions.size() > kPtReGirMaxPositions || lights.size() < identity.light_count
      || draws.size() < kPtReGirMaxCells * kPtReGirSlotsPerCell) return false;

  for (const auto& position : positions) {
    bool valid = true;
    for (float coordinate : position) {
      if (!std::isfinite(coordinate) || coordinate < -kPtReGirCoordinateLimit || coordinate >= kPtReGirCoordinateLimit) valid = false;
    }
    if (!valid) {
      ++output->rejected_position_count;
      continue;
    }
    const PtReGirCellKey key = {static_cast<int32_t>(std::floor(position[0] / kPtReGirCellEdge)),
                                static_cast<int32_t>(std::floor(position[1] / kPtReGirCellEdge)),
                                static_cast<int32_t>(std::floor(position[2] / kPtReGirCellEdge))};
    uint32_t cell_index = 0;
    while (cell_index < output->cell_count && !(output->cells[cell_index].key == key)) ++cell_index;
    if (cell_index == output->cell_count) {
      if (output->cell_count == kPtReGirMaxCells) {
        ++output->overflow_position_count;
      } else {
        output->cells[output->cell_count++].key = key;
      }
    }
  }

  for (uint32_t cell_index = 0; cell_index < output->cell_count; ++cell_index) {
    auto& cell = output->cells[cell_index];
    const std::array<float, 3> center = {(static_cast<float>(cell.key.x) + 0.5f) * kPtReGirCellEdge,
                                         (static_cast<float>(cell.key.y) + 0.5f) * kPtReGirCellEdge,
                                         (static_cast<float>(cell.key.z) + 0.5f) * kPtReGirCellEdge};
    std::array<float, RTXDI_LIGHT_TABLE_CAPACITY> weights = {};
    std::array<uint64_t, RTXDI_LIGHT_TABLE_CAPACITY> endpoints = {};
    double total = 0.0;
    for (uint32_t light_index = 0; light_index < identity.light_count; ++light_index) {
      const auto& light = lights[light_index];
      if ((light.type != RTXDI_LIGHT_TABLE_TYPE_POINT && light.type != RTXDI_LIGHT_TABLE_TYPE_SPOT)
          || !std::isfinite(light.radius) || light.radius <= 0.f || !std::isfinite(light.power) || light.power <= 0.f
          || !std::isfinite(light.position[0]) || !std::isfinite(light.position[1]) || !std::isfinite(light.position[2])) continue;
      float distance_squared = 0.f;
      for (size_t axis = 0; axis < 3; ++axis) {
        const float delta = std::max(std::abs(light.position[axis] - center[axis]) - 2.f, 0.f);
        distance_squared += delta * delta;
      }
      const float weight = light.power / std::max(distance_squared, 1e-4f);
      if (std::isfinite(weight) && weight > 0.f) {
        weights[light_index] = weight;
        total += weight;
      }
    }
    if (!std::isfinite(total) || total <= 0.0) continue;
    constexpr uint64_t ticket_total = 4294967296ull;
    double cumulative = 0.0;
    uint64_t previous = 0;
    bool valid = true;
    for (uint32_t light_index = 0; light_index < identity.light_count; ++light_index) {
      cumulative += weights[light_index];
      const double endpoint = std::floor((cumulative / total) * static_cast<double>(ticket_total));
      if (!std::isfinite(cumulative) || !std::isfinite(endpoint)) {
        valid = false;
        break;
      }
      endpoints[light_index] = light_index + 1 == identity.light_count
                                   ? ticket_total
                                   : static_cast<uint64_t>(std::clamp(endpoint, static_cast<double>(previous), static_cast<double>(ticket_total)));
      previous = endpoints[light_index];
    }
    for (uint32_t slot_index = 0; valid && slot_index < kPtReGirSlotsPerCell; ++slot_index) {
      const uint32_t draw = draws[cell_index * kPtReGirSlotsPerCell + slot_index];
      uint64_t lower = 0;
      bool selected = false;
      for (uint32_t light_index = 0; light_index < identity.light_count; ++light_index) {
        if (endpoints[light_index] > draw) {
          cell.slots[slot_index] = {light_index, endpoints[light_index] - lower};
          selected = cell.slots[slot_index].ticket_count != 0;
          break;
        }
        lower = endpoints[light_index];
      }
      if (!selected) valid = false;
    }
    if (!valid) cell.slots = {};
    cell.valid = valid;
  }
  output->identity = identity;
  output->valid = true;
  return true;
}

inline bool PtReGirIdentityMatches(const PtReGirBuild& build, const PtReGirIdentity& identity) {
  return build.valid && build.identity.owner_generation == identity.owner_generation
         && build.identity.transfer_configuration == identity.transfer_configuration
         && build.identity.table_fence == identity.table_fence && build.identity.frame == identity.frame
         && build.identity.transfer_slot == identity.transfer_slot && build.identity.light_count == identity.light_count;
}

}
