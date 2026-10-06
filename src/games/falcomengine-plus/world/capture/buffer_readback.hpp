#pragma once

// Census readback of indirect-draw arguments.
//
// Indirect draws carry their counts in a GPU buffer, so the census copies the
// args on the game's command list at the draw and resolves them at present.
// Only used while the census is on; the pool reads indirect args itself
// (bvh_pool.hpp). IndirectDroppedCalls() is reported in the pool dump.

#include <vector>

#include "../world_state.hpp"
#include "cb_tracking.hpp"

namespace falcom_world {

inline constexpr uint32_t kMaxIndirectCallsPerFrame = 1024u;
inline constexpr uint32_t kMaxIndirectDrawsPerCall = 256u;
inline constexpr uint64_t kMaxIndirectReadBytes = 512ull * 1024ull;

struct PendingIndirectDraw {
  DrawRecord record;
  bool indexed = true;
  reshade::api::resource args_buffer = {0u};
  uint64_t args_offset = 0u;
  uint32_t draw_count = 0u;
  uint32_t stride = 0u;
  uint64_t staging_offset = 0u;
  uint64_t read_size = 0u;
  WorldCommandListData cl_data = {};
};

inline std::vector<PendingIndirectDraw>& PendingIndirectDraws() {
  static std::vector<PendingIndirectDraw> pending;
  return pending;
}

inline reshade::api::resource& IndirectStagingResource() {
  static reshade::api::resource resource = {0u};
  return resource;
}

inline uint64_t& IndirectStagingCapacity() {
  static uint64_t capacity = 0u;
  return capacity;
}

inline uint64_t& IndirectStagingUsed() {
  static uint64_t used = 0u;
  return used;
}

// Calls dropped by the per-frame indirect interception budget (call count or
// staging bytes), accumulated since the staging buffer was created. Nonzero
// means the census/pool did not see every indirect draw.
inline uint64_t& IndirectDroppedCalls() {
  static uint64_t dropped = 0u;
  return dropped;
}

inline reshade::api::device*& IndirectStagingDevice() {
  static reshade::api::device* device = nullptr;
  return device;
}

inline void ReleaseIndirectStaging() {
  if (IndirectStagingResource().handle != 0u && IndirectStagingDevice() != nullptr) {
    IndirectStagingDevice()->destroy_resource(IndirectStagingResource());
  }
  IndirectStagingResource() = {0u};
  IndirectStagingCapacity() = 0u;
  IndirectStagingUsed() = 0u;
  IndirectStagingDevice() = nullptr;
  PendingIndirectDraws().clear();
  IndirectDroppedCalls() = 0u;
}

inline bool EnsureIndirectStaging(reshade::api::device* device, uint64_t needed) {
  if (device == nullptr || needed == 0u || needed > kMaxIndirectReadBytes) return false;
  if (IndirectStagingDevice() != device) ReleaseIndirectStaging();
  if (IndirectStagingResource().handle != 0u && IndirectStagingCapacity() >= needed) return true;
  ReleaseIndirectStaging();
  reshade::api::resource_desc desc(
      kMaxIndirectReadBytes,
      reshade::api::memory_heap::gpu_to_cpu,
      reshade::api::resource_usage::copy_dest);
  reshade::api::resource staging = {0u};
  if (!device->create_resource(desc, nullptr, reshade::api::resource_usage::copy_dest, &staging)) {
    return false;
  }
  IndirectStagingResource() = staging;
  IndirectStagingCapacity() = kMaxIndirectReadBytes;
  IndirectStagingDevice() = device;
  return true;
}

inline void OnDestroyDeviceWorld(reshade::api::device* device) {
  (void)device;
  ReleaseIndirectStaging();
}

}  // namespace falcom_world
