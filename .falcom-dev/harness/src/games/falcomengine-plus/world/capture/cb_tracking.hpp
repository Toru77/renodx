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

namespace falcom_world {

struct __declspec(uuid("e0b7d0f1-3f6a-4b21-9c8e-5a1d2f3b4c50")) WorldCommandListData {
  std::array<reshade::api::resource, kCbSlotCapacity> vs_cb = {};
  std::array<reshade::api::resource, kCbSlotCapacity> ps_cb = {};
  std::array<reshade::api::resource, kSrvSlotCapacity> vs_srv = {};
  std::array<reshade::api::resource_view, kSrvSlotCapacity> vs_srv_view = {};
  std::array<reshade::api::resource, kSrvSlotCapacity> ps_srv = {};
  std::array<reshade::api::resource_view, kSrvSlotCapacity> ps_srv_view = {};
};

inline WorldCommandListData* GetWorldCommandListData(reshade::api::command_list* cmd_list) {
  if (cmd_list == nullptr) return nullptr;
  return renodx::utils::data::Get<WorldCommandListData>(cmd_list);
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
  auto* data = GetWorldCommandListData(cmd_list);
  if (data != nullptr) *data = {};
}

inline void OnPushDescriptorsWorld(
    reshade::api::command_list* cmd_list,
    reshade::api::shader_stage stages,
    reshade::api::pipeline_layout layout,
    uint32_t param_index,
    const reshade::api::descriptor_table_update& update) {
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
      if (pixel_stage) data->ps_cb[slot] = resource;
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
      const reshade::api::resource resource = (view.handle != 0u) ? device->get_resource_from_view(view) : reshade::api::resource{0u};
      if (vertex_stage) {
        data->vs_srv_view[slot] = view;
        data->vs_srv[slot] = resource;
      }
      if (pixel_stage) {
        data->ps_srv_view[slot] = view;
        data->ps_srv[slot] = resource;
      }
    }
  }
}

}  // namespace falcom_world
