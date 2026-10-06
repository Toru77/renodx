#pragma once

// Split structured-buffer readback for the armed draw.
//
// The instance buffer can be rewritten later in the frame, so the copy is
// issued on the game's command list at the armed draw (same bytes the draw
// used) and only mapped at present. This mirrors the CB snapshot rationale:
// correctness of the selected readback beats its one-shot cost.

#include <algorithm>
#include <mutex>
#include <utility>

#include "../world_state.hpp"
#include "cb_tracking.hpp"
#include "mesh_capture.hpp"

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

// ── Indirect-draw argument readback ───────────────────────────────────────────
// Indirect draws carry their counts in a GPU buffer, so the census copies the
// args on the game's command list at the draw and resolves them at present.

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
  ClearStagingPool(device);
  ReleaseIndirectStaging();
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

// ── Constant-buffer one-shot snapshots ────────────────────────────────────────
// CPU-side utils::constants caches can miss large/ring constant buffers (the
// b1 instance-offset buffer did). These copy the live bytes on the game's
// command list at the captured draw and resolve at present.

struct PendingCbCopy {
  bool camera = false;
  uint8_t stage = 0u;
  uint32_t slot = 0u;
  uint32_t frame = 0u;
  reshade::api::resource source = {0u};
  reshade::api::resource staging = {0u};
  uint64_t source_size = 0u;
  uint64_t read_size = 0u;
};

inline std::vector<PendingCbCopy>& PendingCbCopies() {
  static std::vector<PendingCbCopy> copies;
  return copies;
}

inline void IssueCbCopies(
    reshade::api::command_list* cmd_list,
    reshade::api::device* device,
    WorldCommandListData* cl_data,
    const DrawRecord& draw) {
  if (cmd_list == nullptr || device == nullptr || cl_data == nullptr) return;
  std::lock_guard<std::mutex> lock(ReadbackMutex());
  auto& pending = PendingCbCopies();
  for (auto& copy : pending) {
    ReleaseStaging(device, copy.staging);
  }
  pending.clear();

  for (uint32_t slot = 0; slot < 6u && slot < kCbSlotCapacity; ++slot) {
    const reshade::api::resource source = cl_data->vs_cb[slot];
    if (source.handle == 0u) continue;
    const auto desc = device->get_resource_desc(source);
    if (desc.type != reshade::api::resource_type::buffer) continue;
    const uint64_t read_size = (std::min)(desc.buffer.size, static_cast<uint64_t>(4096u));
    if (read_size == 0u) continue;
    const reshade::api::resource staging = AcquireStaging(device, read_size);
    if (staging.handle == 0u) continue;
    cmd_list->copy_buffer_region(source, 0u, staging, 0u, read_size);
    PendingCbCopy copy;
    copy.stage = 1u;
    copy.slot = slot;
    copy.frame = draw.frame;
    copy.source = source;
    copy.staging = staging;
    copy.source_size = desc.buffer.size;
    copy.read_size = read_size;
    pending.push_back(copy);
  }
}

inline void IssueCameraCbCopy(
    reshade::api::command_list* cmd_list,
    reshade::api::device* device,
    WorldCommandListData* cl_data,
    uint32_t frame) {
  if (cmd_list == nullptr || device == nullptr || cl_data == nullptr) return;
  std::lock_guard<std::mutex> lock(ReadbackMutex());
  const reshade::api::resource source = cl_data->ps_cb[0];
  if (source.handle == 0u) return;
  const auto desc = device->get_resource_desc(source);
  if (desc.type != reshade::api::resource_type::buffer) return;
  const uint64_t read_size = (std::min)(desc.buffer.size, static_cast<uint64_t>(4096u));
  if (read_size == 0u) return;
  const reshade::api::resource staging = AcquireStaging(device, read_size);
  if (staging.handle == 0u) return;
  cmd_list->copy_buffer_region(source, 0u, staging, 0u, read_size);
  PendingCbCopy copy;
  copy.camera = true;
  copy.stage = 2u;
  copy.slot = 0u;
  copy.frame = frame;
  copy.source = source;
  copy.staging = staging;
  copy.source_size = desc.buffer.size;
  copy.read_size = read_size;
  PendingCbCopies().push_back(copy);
}

inline void ResolveCbCopies(
    reshade::api::command_queue* queue,
    CapturedDraw* captured) {
  if (queue == nullptr) return;
  std::lock_guard<std::mutex> lock(ReadbackMutex());
  auto& pending = PendingCbCopies();
  if (pending.empty()) return;
  queue->flush_immediate_command_list();
  queue->wait_idle();

  for (auto& copy : pending) {
    if (copy.staging.handle == 0u) continue;
    void* mapped = nullptr;
    if (queue->get_device()->map_buffer_region(
            copy.staging, 0u, copy.read_size, reshade::api::map_access::read_only, &mapped)
        && mapped != nullptr) {
      std::vector<uint8_t> bytes(
          static_cast<const uint8_t*>(mapped),
          static_cast<const uint8_t*>(mapped) + static_cast<size_t>(copy.read_size));
      queue->get_device()->unmap_buffer_region(copy.staging);
      if (copy.camera) {
        CaptureCameraFromBytes(bytes, copy.frame, 2u);
      } else if (captured != nullptr) {
        CbSnapshot snapshot;
        snapshot.stage = copy.stage;
        snapshot.slot = copy.slot;
        snapshot.bytes = std::move(bytes);
        snapshot.gpu_snapshot = true;
        snapshot.resource_size = copy.source_size;
        snapshot.snapshot_size = copy.read_size;
        bool replaced = false;
        for (auto& existing : captured->cbs) {
          if (existing.stage == copy.stage && existing.slot == copy.slot) {
            existing = std::move(snapshot);
            replaced = true;
            break;
          }
        }
        if (!replaced) captured->cbs.push_back(std::move(snapshot));
      }
    }
    ReleaseStaging(queue->get_device(), copy.staging);
  }
  pending.clear();
}

}  // namespace falcom_world
