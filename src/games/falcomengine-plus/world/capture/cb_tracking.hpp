#pragma once

// Per-command-list VS/PS constant-buffer and VS SRV slot tracking.
//
// D3D11 constant-buffer binds reach ReShade as push_descriptors with
// descriptor_type::constant_buffer; update.binding is the D3D register index
// and descriptors are buffer_range. Shader-resource binds use
// descriptor_type::shader_resource_view with resource_view descriptors.
// Both carry absolute register indices on this backend.
//
// Tracking runs whenever the module is attached, not only while a capture
// toggle is on: the pool copies from the tracked t15/b1 resources, and a
// binding missed during a gap would leave a stale handle that may name a
// destroyed buffer. Null binds clear the slot, and a deferred context's
// bindings are dropped when its recording finishes (its state resets).

#include "../world_state.hpp"
#include "../bvh/perf_gate.hpp"

namespace falcom_world {

struct __declspec(uuid("e0b7d0f1-3f6a-4b21-9c8e-5a1d2f3b4c50")) WorldCommandListData {
  std::array<reshade::api::resource, kCbSlotCapacity> vs_cb = {};
  std::array<reshade::api::resource, kCbSlotCapacity> ps_cb = {};
  std::array<uint64_t, kCbSlotCapacity> ps_cb_offset = {};  // buffer_range offset of the PS bind
  std::array<reshade::api::resource, kSrvSlotCapacity> vs_srv = {};
  std::array<reshade::api::resource_view, kSrvSlotCapacity> vs_srv_view = {};
  std::array<reshade::api::resource, kSrvSlotCapacity> ps_srv = {};
  std::array<reshade::api::resource_view, kSrvSlotCapacity> ps_srv_view = {};
  std::array<uint32_t, kSrvSlotCapacity> vs_srv_generation = {};  // view_generation when vs_srv[slot] was resolved
  std::array<uint32_t, kSrvSlotCapacity> ps_srv_generation = {};  // view_generation when ps_srv[slot] was resolved
  uint32_t epoch = 0u;
};

inline WorldCommandListData* GetWorldCommandListData(reshade::api::command_list* cmd_list) {
  if (cmd_list == nullptr) return nullptr;
  auto* data = renodx::utils::data::Get<WorldCommandListData>(cmd_list);
  if (data != nullptr && data->epoch != g_state.tracking_epoch.load(std::memory_order_relaxed)) {
    *data = {};
    data->epoch = g_state.tracking_epoch.load(std::memory_order_relaxed);
  }
  return data;
}

inline void OnInitWorldCommandList(reshade::api::command_list* cmd_list) {
  if (cmd_list == nullptr) return;
  WorldCommandListData* data = nullptr;
  renodx::utils::data::CreateOrGet<WorldCommandListData>(cmd_list, data);
}

inline void OnDestroyWorldCommandList(reshade::api::command_list* cmd_list) {
  if (cmd_list == nullptr) return;
  renodx::utils::data::Delete<WorldCommandListData>(cmd_list);
}

inline void OnResetWorldCommandList(reshade::api::command_list* cmd_list) {
  if (!WorldDrawHookNeeded()) return;
  auto* data = GetWorldCommandListData(cmd_list);
  if (data != nullptr) *data = {};
}

inline void OnPushDescriptorsWorld(
    reshade::api::command_list* cmd_list,
    reshade::api::shader_stage stages,
    reshade::api::pipeline_layout layout,
    uint32_t param_index,
    const reshade::api::descriptor_table_update& update) {
  const bvh::PoolCpuTimer bind_timer{&bvh::g_perf_gate.block_us[bvh::kPerfBlockBindHook]};
  if (!WorldDrawHookNeeded()) return;
  if (bind_timer.armed) {
    bvh::g_perf_gate.bind_calls.fetch_add(1u, std::memory_order_relaxed);
  }
  (void)layout;
  (void)param_index;
  if (update.count == 0u || update.descriptors == nullptr) return;

  auto* data = GetWorldCommandListData(cmd_list);
  if (data == nullptr) return;

  const uint32_t stage_mask = static_cast<uint32_t>(stages);
  const bool vertex_stage = (stage_mask & static_cast<uint32_t>(reshade::api::shader_stage::vertex)) != 0u;
  const bool pixel_stage = (stage_mask & static_cast<uint32_t>(reshade::api::shader_stage::pixel)) != 0u;

  if (update.type == reshade::api::descriptor_type::constant_buffer) {
    if (!vertex_stage && !pixel_stage) return;
    const auto* ranges = static_cast<const reshade::api::buffer_range*>(update.descriptors);
    for (uint32_t i = 0; i < update.count; ++i) {
      const uint32_t slot = update.binding + i;
      if (slot >= kCbSlotCapacity) continue;
      const reshade::api::resource resource = ranges[i].buffer;
      if (vertex_stage) data->vs_cb[slot] = resource;
      if (pixel_stage) {
        data->ps_cb[slot] = resource;
        data->ps_cb_offset[slot] = ranges[i].offset;
      }
    }
    return;
  }

  if (update.type == reshade::api::descriptor_type::shader_resource_view) {
    if (!vertex_stage && !pixel_stage) return;
    auto* device = cmd_list->get_device();
    if (device == nullptr) return;
    const auto* views = static_cast<const reshade::api::resource_view*>(update.descriptors);
    for (uint32_t i = 0; i < update.count; ++i) {
      const uint32_t slot = update.binding + i;
      if (slot >= kSrvSlotCapacity) continue;
      const reshade::api::resource_view view = views[i];
      const bool resolve = view.handle != 0u && (vertex_stage || (kPsSrvResourceSlots & (1u << slot)) != 0u);
      // Destroy-view hook present (HookOffMask bit 128 clear): a slot's resource is reused within one view generation.
      static const bool view_reuse = (HookOffMask() & 128u) == 0u;
      const uint32_t view_generation = g_state.view_generation.load(std::memory_order_acquire);
      reshade::api::resource resource = {0u};
      if (resolve) {
        if (view_reuse && vertex_stage && data->vs_srv_view[slot].handle == view.handle && data->vs_srv[slot].handle != 0u
            && data->vs_srv_generation[slot] == view_generation) {
          resource = data->vs_srv[slot];
        } else if (view_reuse && !vertex_stage && data->ps_srv_view[slot].handle == view.handle && data->ps_srv[slot].handle != 0u
                   && data->ps_srv_generation[slot] == view_generation) {
          resource = data->ps_srv[slot];
        } else {
          resource = device->get_resource_from_view(view);
          if (bind_timer.armed) {
            bvh::g_perf_gate.bind_srv_resolves.fetch_add(1u, std::memory_order_relaxed);
          }
        }
      }
      if (vertex_stage) {
        data->vs_srv_view[slot] = view;
        data->vs_srv[slot] = resource;
        data->vs_srv_generation[slot] = view_generation;
      }
      if (pixel_stage) {
        data->ps_srv_view[slot] = view;
        data->ps_srv[slot] = ((kPsSrvResourceSlots & (1u << slot)) != 0u ? resource : reshade::api::resource{0u});
        data->ps_srv_generation[slot] = view_generation;
      }
    }
  }
}

}  // namespace falcom_world
