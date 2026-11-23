#pragma once

// Shared D3D11 helper-pass layout used by the rt12 passes and the RR guides. Created lazily from the lighting hook;
// no module mutex is held around any graphics call here.

#include <cstdint>
#include <unordered_map>
#include <vector>

#include "../../../../utils/log.hpp"

namespace falcom_world::rtao {

inline constexpr uint32_t kRtaoSrvCount = 20u;
inline constexpr uint32_t kRtaoUavCount = 3u;
inline constexpr uint32_t kRtaoPushRegister = 12u;
inline constexpr uint32_t kRtaoPushConstantCount = 24u;

struct RtaoDeviceData {
  reshade::api::pipeline_layout layout = {0u};
  reshade::api::descriptor_table cbv_table = {0u};
  reshade::api::descriptor_table srv_table = {0u};
  reshade::api::descriptor_table uav_table = {0u};
  bool layout_failed = false;
};

// One entry per device (destroyed with the device).
inline std::unordered_map<reshade::api::device*, RtaoDeviceData> g_rtao_devices;

inline RtaoDeviceData& GetRtaoDeviceData(reshade::api::device* device) { return g_rtao_devices[device]; }

inline void DestroyRtaoLayout(reshade::api::device* device, RtaoDeviceData* data) {
  if (data->srv_table.handle != 0u) device->free_descriptor_table(data->srv_table);
  if (data->uav_table.handle != 0u) device->free_descriptor_table(data->uav_table);
  if (data->cbv_table.handle != 0u) device->free_descriptor_table(data->cbv_table);
  if (data->layout.handle != 0u) device->destroy_pipeline_layout(data->layout);
  data->srv_table = {0u};
  data->uav_table = {0u};
  data->cbv_table = {0u};
  data->layout = {0u};
}

inline bool EnsureRtaoLayout(reshade::api::device* device, RtaoDeviceData* data) {
  if (data->layout.handle != 0u && data->cbv_table.handle != 0u && data->srv_table.handle != 0u
      && data->uav_table.handle != 0u) return true;
  if (data->layout_failed) return false;

  using DR = reshade::api::descriptor_range;
  using DS = reshade::api::shader_stage;
  using DT = reshade::api::descriptor_type;
  using P = reshade::api::pipeline_layout_param;

  DR cbv_range = {0, 0, 0, 1, DS::all_compute, 1, DT::constant_buffer};
  DR srv_range = {0, 0, 0, kRtaoSrvCount, DS::all_compute, 1, DT::shader_resource_view};
  DR uav_range = {0, 0, 0, kRtaoUavCount, DS::all_compute, 1, DT::unordered_access_view};
  reshade::api::constant_range push_range = {};
  push_range.binding = 0;
  push_range.dx_register_index = kRtaoPushRegister;
  push_range.dx_register_space = 0;
  push_range.count = kRtaoPushConstantCount;
  push_range.visibility = DS::all_compute;
  P params[4] = {};
  params[0].type = reshade::api::pipeline_layout_param_type::descriptor_table;
  params[0].descriptor_table.count = 1;
  params[0].descriptor_table.ranges = &cbv_range;
  params[1].type = reshade::api::pipeline_layout_param_type::descriptor_table;
  params[1].descriptor_table.count = 1;
  params[1].descriptor_table.ranges = &srv_range;
  params[2].type = reshade::api::pipeline_layout_param_type::descriptor_table;
  params[2].descriptor_table.count = 1;
  params[2].descriptor_table.ranges = &uav_range;
  params[3].type = reshade::api::pipeline_layout_param_type::push_constants;
  params[3].push_constants = push_range;
  const auto fail = [&](const char* stage) {
    renodx::utils::log::w("[world-rtao] shared layout creation failed: ", stage);
    DestroyRtaoLayout(device, data);
    data->layout_failed = true;
    return false;
  };
  if (data->layout.handle == 0u && !device->create_pipeline_layout(4, params, &data->layout)) return fail("pipeline layout");
  if (data->cbv_table.handle == 0u && !device->allocate_descriptor_table(data->layout, 0, &data->cbv_table)) return fail("cbv table");
  if (data->srv_table.handle == 0u && !device->allocate_descriptor_table(data->layout, 1, &data->srv_table)) return fail("srv table");
  if (data->uav_table.handle == 0u && !device->allocate_descriptor_table(data->layout, 2, &data->uav_table)) return fail("uav table");
  return true;
}

// Creates a 2D target with zero contents (history starts invalid: distance 0).
inline bool CreateRtaoTexture(
    reshade::api::device* device, reshade::api::format format, uint32_t width, uint32_t height, uint32_t bytes_per_pixel,
    reshade::api::resource* texture, reshade::api::resource_view* srv, reshade::api::resource_view* uav) {
  reshade::api::resource_desc desc = {};
  desc.type = reshade::api::resource_type::texture_2d;
  desc.texture = {width, height, 1, 1, format, 1};
  desc.heap = reshade::api::memory_heap::gpu_only;
  desc.usage = reshade::api::resource_usage::shader_resource | reshade::api::resource_usage::unordered_access;
  const std::vector<uint8_t> zeros(static_cast<size_t>(width) * height * bytes_per_pixel, 0u);
  reshade::api::subresource_data initial = {};
  initial.data = const_cast<uint8_t*>(zeros.data());
  initial.row_pitch = width * bytes_per_pixel;
  initial.slice_pitch = width * height * bytes_per_pixel;
  if (!device->create_resource(desc, &initial, reshade::api::resource_usage::shader_resource, texture)) return false;
  const reshade::api::resource_view_desc view(reshade::api::resource_view_type::texture_2d, format, 0, 1, 0, 1);
  return device->create_resource_view(*texture, reshade::api::resource_usage::shader_resource, view, srv)
         && device->create_resource_view(*texture, reshade::api::resource_usage::unordered_access, view, uav);
}

// update_descriptor_tables reads a constant_buffer descriptor as api::buffer_range (3 words), never as a resource_view.
inline reshade::api::buffer_range SceneCbvRange(reshade::api::resource_view view) {
  return {reshade::api::resource{view.handle}, 0u, UINT64_MAX};
}

inline void DestroyRtaoDeviceData(reshade::api::device* device) {
  const auto found = g_rtao_devices.find(device);
  if (found == g_rtao_devices.end()) return;
  RtaoDeviceData& data = found->second;
  DestroyRtaoLayout(device, &data);
  g_rtao_devices.erase(found);
}

}  // namespace falcom_world::rtao
