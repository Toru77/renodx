#pragma once

// Split structured-buffer readback for the armed draw.
//
// The instance buffer can be rewritten later in the frame, so the copy is
// issued on the game's command list at the armed draw (same bytes the draw
// used) and only mapped at present. This mirrors the CB snapshot rationale:
// correctness of the selected readback beats its one-shot cost.

#include <mutex>
#include <utility>

#include "../world_state.hpp"
#include "cb_tracking.hpp"

namespace falcom_world {

struct PendingBufferCopy {
  uint8_t stage = 0u;
  uint32_t slot = 0u;
  reshade::api::resource source = {0u};
  reshade::api::resource staging = {0u};
  uint32_t stride = 0u;
  uint64_t source_size = 0u;
  uint64_t read_offset = 0u;
  uint64_t read_size = 0u;
  bool truncated = false;
};

struct StagingEntry {
  reshade::api::device* device = nullptr;
  reshade::api::resource resource = {0u};
};

inline std::mutex& ReadbackMutex() {
  static std::mutex mutex;
  return mutex;
}

inline std::vector<PendingBufferCopy>& PendingCopies() {
  static std::vector<PendingBufferCopy> copies;
  return copies;
}

inline std::vector<StagingEntry>& StagingPool() {
  static std::vector<StagingEntry> pool;
  return pool;
}

inline reshade::api::resource AcquireStaging(reshade::api::device* device, uint64_t size) {
  if (device == nullptr || size == 0u) return {0u};
  for (auto it = StagingPool().begin(); it != StagingPool().end(); ++it) {
    if (it->device != device) continue;
    const auto desc = device->get_resource_desc(it->resource);
    if (desc.buffer.size < size) continue;
    const reshade::api::resource resource = it->resource;
    StagingPool().erase(it);
    return resource;
  }
  reshade::api::resource_desc desc(
      static_cast<uint64_t>(kMaxSrvReadBytes),
      reshade::api::memory_heap::gpu_to_cpu,
      reshade::api::resource_usage::copy_dest);
  reshade::api::resource staging = {0u};
  if (!device->create_resource(desc, nullptr, reshade::api::resource_usage::copy_dest, &staging)) {
    return {0u};
  }
  return staging;
}

inline void ReleaseStaging(reshade::api::device* device, reshade::api::resource staging) {
  if (device == nullptr || staging.handle == 0u) return;
  if (StagingPool().size() >= 4u) {
    device->destroy_resource(staging);
    return;
  }
  StagingPool().push_back({device, staging});
}

inline void ClearStagingPool(reshade::api::device* device) {
  std::lock_guard<std::mutex> lock(ReadbackMutex());
  auto& pool = StagingPool();
  for (auto it = pool.begin(); it != pool.end();) {
    if (device != nullptr && it->device != device) {
      ++it;
      continue;
    }
    if (it->resource.handle != 0u && it->device != nullptr) {
      it->device->destroy_resource(it->resource);
    }
    it = pool.erase(it);
  }
}

inline void OnDestroyDeviceWorld(reshade::api::device* device) {
  ClearStagingPool(device);
}

inline void IssueStructuredBufferCopies(
    reshade::api::command_list* cmd_list,
    reshade::api::device* device,
    WorldCommandListData* cl_data,
    const DrawRecord& draw,
    CapturedDraw* captured) {
  if (cmd_list == nullptr || device == nullptr || cl_data == nullptr || captured == nullptr) return;

  std::lock_guard<std::mutex> lock(ReadbackMutex());
  auto& pending = PendingCopies();
  for (auto& copy : pending) {
    ReleaseStaging(device, copy.staging);
  }
  pending.clear();

  uint32_t issued = 0u;
  for (uint32_t slot = 0; slot < kSrvSlotCapacity && issued < kMaxSrvSnapshots; ++slot) {
    const reshade::api::resource source = cl_data->vs_srv[slot];
    if (source.handle == 0u) continue;
    const auto desc = device->get_resource_desc(source);
    if (desc.type != reshade::api::resource_type::buffer) continue;
    if (desc.buffer.stride == 0u || desc.buffer.size < 64u) continue;

    const uint64_t stride = desc.buffer.stride;
    uint64_t offset = 0u;
    uint64_t read_size = desc.buffer.size;
    if (captured->instance_offset_found) {
      int64_t base_element = static_cast<int64_t>(draw.first_instance) + captured->instance_offset_g;
      if (base_element < 0) base_element = 0;
      const uint64_t base_byte = static_cast<uint64_t>(base_element) * stride;
      const uint64_t margin = 8u * stride;
      offset = (base_byte > margin) ? (base_byte - margin) : 0u;
      const uint64_t instances = draw.instance_count == 0u ? 1u : draw.instance_count;
      read_size = (instances + 16u) * stride;
    }
    if (read_size > kMaxSrvReadBytes) read_size = kMaxSrvReadBytes;
    if (offset >= desc.buffer.size) offset = 0u;
    if (offset + read_size > desc.buffer.size) read_size = desc.buffer.size - offset;
    if (read_size == 0u) continue;

    const reshade::api::resource staging = AcquireStaging(device, read_size);
    if (staging.handle == 0u) continue;

    cmd_list->copy_buffer_region(source, offset, staging, 0u, read_size);

    PendingBufferCopy copy;
    copy.stage = 1u;
    copy.slot = slot;
    copy.source = source;
    copy.staging = staging;
    copy.stride = static_cast<uint32_t>(stride);
    copy.source_size = desc.buffer.size;
    copy.read_offset = offset;
    copy.read_size = read_size;
    copy.truncated = (offset != 0u) || (read_size != desc.buffer.size);
    pending.push_back(copy);
    issued += 1u;
  }
}

inline void ResolveStructuredBufferCopies(
    reshade::api::command_queue* queue,
    CapturedDraw* captured) {
  if (queue == nullptr || captured == nullptr) return;

  std::lock_guard<std::mutex> lock(ReadbackMutex());
  auto& pending = PendingCopies();
  if (pending.empty()) return;

  queue->flush_immediate_command_list();
  queue->wait_idle();

  for (auto& copy : pending) {
    if (copy.staging.handle == 0u) continue;
    void* mapped = nullptr;
    if (queue->get_device()->map_buffer_region(
            copy.staging, 0u, copy.read_size, reshade::api::map_access::read_only, &mapped)
        && mapped != nullptr) {
      SrvBufferSnapshot snapshot;
      snapshot.stage = copy.stage;
      snapshot.slot = copy.slot;
      snapshot.resource = copy.source;
      snapshot.stride = copy.stride;
      snapshot.size = copy.source_size;
      snapshot.read_offset = copy.read_offset;
      snapshot.read_size = copy.read_size;
      snapshot.bytes.assign(
          static_cast<const uint8_t*>(mapped),
          static_cast<const uint8_t*>(mapped) + static_cast<size_t>(copy.read_size));
      snapshot.truncated = copy.truncated;
      snapshot.valid = true;
      captured->srv_buffers.push_back(std::move(snapshot));
      queue->get_device()->unmap_buffer_region(copy.staging);
    }
    ReleaseStaging(queue->get_device(), copy.staging);
  }
  pending.clear();
}

}  // namespace falcom_world
