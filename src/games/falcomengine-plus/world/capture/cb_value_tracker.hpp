#pragma once

// CPU mirror of the first bytes of small constant buffers.
//
// The pool needs the draw-time instanceOffset_g (cb_instance, b1) on the CPU
// before the draw, so it can copy exactly the instance elements the draw
// reads. utils::constants is not reliable for this buffer: a whole-buffer
// UpdateSubresource reaches it as size == UINT64_MAX, which clears its cache,
// and deferred-context updates arrive as update_buffer_region_command, which
// it does not handle. This tracker mirrors every CPU write path (initial
// data, map/unmap, update on the device and on command lists) for buffers of
// at most kTrackedCbMaxBytes. The pool also copies b1 on the GPU at each
// draw and compares at resolve, so a wrong mirror shows up as a counter
// instead of wrong geometry.

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <unordered_map>

#include <include/reshade.hpp>

namespace falcom_world {

inline constexpr uint64_t kTrackedCbMaxBytes = 256u;
inline constexpr uint32_t kTrackedCbBytes = 160u;

struct TrackedCb {
  std::array<uint8_t, kTrackedCbBytes> bytes = {};
  uint64_t size = 0u;
  uint8_t* mapped = nullptr;
  uint64_t valid_bytes = 0u;  // contiguous prefix of the buffer the mirror holds
};

struct CbValueTracker {
  std::mutex mutex;
  std::unordered_map<uint64_t, TrackedCb> buffers;
  // command list -> resource -> bytes as that deferred recording wrote them
  std::unordered_map<uint64_t, std::unordered_map<uint64_t, TrackedCb>> deferred;
};

inline CbValueTracker& CbTracker() {
  static CbValueTracker tracker;
  return tracker;
}

// Copies the part of [offset, offset + size) that overlaps the mirrored
// bytes. `data` points at the start of the written region.
inline void MirrorCbWrite(TrackedCb* tracked, const void* data, uint64_t offset, uint64_t size) {
  if (data == nullptr || offset >= kTrackedCbBytes) return;
  const uint64_t count = (std::min)(size, static_cast<uint64_t>(kTrackedCbBytes) - offset);
  std::memcpy(tracked->bytes.data() + offset, data, static_cast<size_t>(count));
  if (offset <= tracked->valid_bytes && offset + count > tracked->valid_bytes) tracked->valid_bytes = offset + count;
}

inline void OnInitResourceCbTracker(
    reshade::api::device* device,
    const reshade::api::resource_desc& desc,
    const reshade::api::subresource_data* initial_data,
    reshade::api::resource_usage initial_state,
    reshade::api::resource resource) {
  (void)device;
  (void)initial_state;
  if (desc.type != reshade::api::resource_type::buffer) return;
  if ((desc.usage & reshade::api::resource_usage::constant_buffer) == 0u) return;
  if (desc.buffer.size == 0u || desc.buffer.size > kTrackedCbMaxBytes) return;
  TrackedCb tracked;
  tracked.size = desc.buffer.size;
  if (initial_data != nullptr) MirrorCbWrite(&tracked, initial_data->data, 0u, desc.buffer.size);
  auto& tracker = CbTracker();
  std::lock_guard lock(tracker.mutex);
  tracker.buffers.insert_or_assign(resource.handle, tracked);
}

inline void OnDestroyResourceCbTracker(reshade::api::device* device, reshade::api::resource resource) {
  (void)device;
  auto& tracker = CbTracker();
  std::lock_guard lock(tracker.mutex);
  tracker.buffers.erase(resource.handle);
}

inline void OnMapBufferRegionCbTracker(
    reshade::api::device* device,
    reshade::api::resource resource,
    uint64_t offset,
    uint64_t size,
    reshade::api::map_access access,
    void** data) {
  (void)device;
  (void)size;
  if (data == nullptr || *data == nullptr || access == reshade::api::map_access::read_only) return;
  auto& tracker = CbTracker();
  std::lock_guard lock(tracker.mutex);
  const auto it = tracker.buffers.find(resource.handle);
  if (it == tracker.buffers.end() || offset != 0u) return;
  it->second.mapped = static_cast<uint8_t*>(*data);
}

inline void OnUnmapBufferRegionCbTracker(reshade::api::device* device, reshade::api::resource resource) {
  (void)device;
  auto& tracker = CbTracker();
  std::lock_guard lock(tracker.mutex);
  const auto it = tracker.buffers.find(resource.handle);
  if (it == tracker.buffers.end() || it->second.mapped == nullptr) return;
  MirrorCbWrite(&it->second, it->second.mapped, 0u, it->second.size);
  it->second.mapped = nullptr;
}

inline void TrackCbUpdate(
    reshade::api::command_list* cmd_list,
    reshade::api::resource resource,
    const void* data,
    uint64_t offset,
    uint64_t size) {
  auto& tracker = CbTracker();
  std::lock_guard lock(tracker.mutex);
  const auto it = tracker.buffers.find(resource.handle);
  if (it == tracker.buffers.end()) return;
  // UINT64_MAX means "the whole buffer" (UpdateSubresource without a box).
  if (size == UINT64_MAX) size = it->second.size;
  if (cmd_list != nullptr) {
    auto& recorded = tracker.deferred[reinterpret_cast<uintptr_t>(cmd_list)];
    auto [entry, inserted] = recorded.try_emplace(resource.handle, it->second);
    (void)inserted;
    entry->second.mapped = nullptr;
    MirrorCbWrite(&entry->second, data, offset, size);
  }
  MirrorCbWrite(&it->second, data, offset, size);
}

inline bool OnUpdateBufferRegionCbTracker(
    reshade::api::device* device,
    const void* data,
    reshade::api::resource resource,
    uint64_t offset,
    uint64_t size) {
  (void)device;
  TrackCbUpdate(nullptr, resource, data, offset, size);
  return false;
}

inline bool OnUpdateBufferRegionCommandCbTracker(
    reshade::api::command_list* cmd_list,
    const void* data,
    reshade::api::resource resource,
    uint64_t offset,
    uint64_t size) {
  TrackCbUpdate(cmd_list, resource, data, offset, size);
  return false;
}

// A finished (or destroyed) deferred recording no longer sees its own writes.
inline void ForgetDeferredCbWrites(reshade::api::command_list* cmd_list) {
  if (cmd_list == nullptr) return;
  auto& tracker = CbTracker();
  std::lock_guard lock(tracker.mutex);
  tracker.deferred.erase(reinterpret_cast<uintptr_t>(cmd_list));
}

inline void OnResetCommandListCbTracker(reshade::api::command_list* cmd_list) {
  ForgetDeferredCbWrites(cmd_list);
}

inline void OnDestroyCommandListCbTracker(reshade::api::command_list* cmd_list) {
  ForgetDeferredCbWrites(cmd_list);
}

// The first `bytes` of a tracked constant buffer as a draw recorded on
// `cmd_list` sees them.
inline bool ReadTrackedCbBytes(reshade::api::command_list* cmd_list, reshade::api::resource resource, void* out, uint64_t bytes) {
  if (out == nullptr || resource.handle == 0u || bytes > kTrackedCbBytes) return false;
  auto& tracker = CbTracker();
  std::lock_guard lock(tracker.mutex);
  if (cmd_list != nullptr && !tracker.deferred.empty()) {
    const auto list_it = tracker.deferred.find(reinterpret_cast<uintptr_t>(cmd_list));
    if (list_it != tracker.deferred.end()) {
      const auto entry_it = list_it->second.find(resource.handle);
      if (entry_it != list_it->second.end() && entry_it->second.valid_bytes >= bytes) {
        std::memcpy(out, entry_it->second.bytes.data(), static_cast<size_t>(bytes));
        return true;
      }
    }
  }
  const auto it = tracker.buffers.find(resource.handle);
  if (it == tracker.buffers.end() || it->second.valid_bytes < bytes) return false;
  std::memcpy(out, it->second.bytes.data(), static_cast<size_t>(bytes));
  return true;
}

// First int of a tracked constant buffer (instanceOffset_g for b1).
inline bool ReadTrackedCbInt(reshade::api::command_list* cmd_list, reshade::api::resource resource, int32_t* out) {
  return ReadTrackedCbBytes(cmd_list, resource, out, sizeof(int32_t));
}

inline void RegisterCbTracker() {
  reshade::register_event<reshade::addon_event::init_resource>(OnInitResourceCbTracker);
  reshade::register_event<reshade::addon_event::destroy_resource>(OnDestroyResourceCbTracker);
  reshade::register_event<reshade::addon_event::map_buffer_region>(OnMapBufferRegionCbTracker);
  reshade::register_event<reshade::addon_event::unmap_buffer_region>(OnUnmapBufferRegionCbTracker);
  reshade::register_event<reshade::addon_event::update_buffer_region>(OnUpdateBufferRegionCbTracker);
  reshade::register_event<reshade::addon_event::update_buffer_region_command>(OnUpdateBufferRegionCommandCbTracker);
  reshade::register_event<reshade::addon_event::reset_command_list>(OnResetCommandListCbTracker);
  reshade::register_event<reshade::addon_event::destroy_command_list>(OnDestroyCommandListCbTracker);
}

inline void UnregisterCbTracker() {
  reshade::unregister_event<reshade::addon_event::init_resource>(OnInitResourceCbTracker);
  reshade::unregister_event<reshade::addon_event::destroy_resource>(OnDestroyResourceCbTracker);
  reshade::unregister_event<reshade::addon_event::map_buffer_region>(OnMapBufferRegionCbTracker);
  reshade::unregister_event<reshade::addon_event::unmap_buffer_region>(OnUnmapBufferRegionCbTracker);
  reshade::unregister_event<reshade::addon_event::update_buffer_region>(OnUpdateBufferRegionCbTracker);
  reshade::unregister_event<reshade::addon_event::update_buffer_region_command>(OnUpdateBufferRegionCommandCbTracker);
  reshade::unregister_event<reshade::addon_event::reset_command_list>(OnResetCommandListCbTracker);
  reshade::unregister_event<reshade::addon_event::destroy_command_list>(OnDestroyCommandListCbTracker);
}

}  // namespace falcom_world
