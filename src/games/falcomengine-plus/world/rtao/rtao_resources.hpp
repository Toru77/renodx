#pragma once

// RTAO round 1 GPU resources: the compute pipeline (world_rtao.cs_5_0.hlsl), the AO
// target (r32_uint at depth size, 0..255 texels, 255 = neutral) and the stats buffer.
// Created lazily from the lighting hook (rtao.hpp); no module mutex is held around
// any graphics call here.

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

#include "../../../../utils/log.hpp"
#include "../bvh/bvh_resources.hpp"

namespace falcom_world::rtao {

// t0..t8 BVH, t9 game depth, t10..t13 deforming (null in round 1), t14..t16 alpha,
// t17 MRT normal, t18 IS-FAST noise.
inline constexpr uint32_t kRtaoSrvCount = 19u;
inline constexpr uint32_t kRtaoUavCount = 2u;  // u0 AO, u1 stats
inline constexpr uint32_t kRtaoPushRegister = 12u;
inline constexpr uint32_t kRtaoPushConstantCount = 20u;
inline constexpr uint32_t kRtaoStatsCount = 10u;  // RTAO_STAT_COUNT in world_rtao.cs_5_0.hlsl
// Stats buffer indices, matching RTAO_STAT_* in world_rtao.cs_5_0.hlsl.
inline constexpr uint32_t kRtaoStatRays = 0u;
inline constexpr uint32_t kRtaoStatHits = 1u;
inline constexpr uint32_t kRtaoStatPixels = 2u;
inline constexpr uint32_t kRtaoStatAoSum = 3u;
inline constexpr uint32_t kRtaoStatSky = 4u;
inline constexpr uint32_t kRtaoStatNormal = 5u;
inline constexpr uint32_t kRtaoStatRegion = 6u;
inline constexpr uint32_t kRtaoStatInvalidRefs = 7u;
inline constexpr uint32_t kRtaoStatStackOverflow = 8u;
inline constexpr uint32_t kRtaoStatScaled = 9u;

// b12, matching cbuffer cb_rtao in world_rtao.cs_5_0.hlsl (five float4 registers).
struct RtaoPushConstants {
  float params[4];    // c0: radius, ray_max, strength, normal_bias
  float sampling[4];  // c1: spp, isfast flag (1 loaded), slice base, seed
  float fade[4];      // c2: fade start_eff, fade end_eff, 0, 0
  float region[4];    // c3: region min xyz, region size
  float size[4];      // c4: width, height, dynamic count (0), reserved
};
static_assert(sizeof(RtaoPushConstants) == kRtaoPushConstantCount * sizeof(float), "b12 must be 20 floats");
static_assert(offsetof(RtaoPushConstants, sampling) == 4 * sizeof(float), "b12 c1 offset");
static_assert(offsetof(RtaoPushConstants, fade) == 8 * sizeof(float), "b12 c2 offset");
static_assert(offsetof(RtaoPushConstants, region) == 12 * sizeof(float), "b12 c3 offset");
static_assert(offsetof(RtaoPushConstants, size) == 16 * sizeof(float), "b12 c4 offset");

struct RtaoDeviceData {
  reshade::api::pipeline_layout layout = {0u};
  reshade::api::descriptor_table cbv_table = {0u};
  reshade::api::descriptor_table srv_table = {0u};
  reshade::api::descriptor_table uav_table = {0u};
  reshade::api::pipeline pipeline = {0u};
  bool pipeline_failed = false;
  reshade::api::resource ao_texture = {0u};
  reshade::api::resource_view ao_srv = {0u};
  reshade::api::resource_view ao_uav = {0u};
  uint32_t ao_width = 0u;
  uint32_t ao_height = 0u;
  reshade::api::resource stats_buffer = {0u};
  reshade::api::resource_view stats_uav = {0u};
  bvh::GpuTimer timer = {};
};

// One entry per device (destroyed with the device).
inline std::unordered_map<reshade::api::device*, RtaoDeviceData> g_rtao_devices;

inline RtaoDeviceData& GetRtaoDeviceData(reshade::api::device* device) { return g_rtao_devices[device]; }

inline void DestroyRtaoTarget(reshade::api::device* device, RtaoDeviceData* data) {
  bvh::DestroyBuffer(device, &data->ao_srv, &data->ao_texture);
  bvh::DestroyBuffer(device, &data->ao_uav, nullptr);
  data->ao_width = 0u;
  data->ao_height = 0u;
}

inline bool EnsureRtaoPipeline(reshade::api::device* device, RtaoDeviceData* data) {
  if (data->pipeline.handle != 0u) return true;
  if (data->pipeline_failed) return false;

#if defined(__world_rtao_EMBED_FILE)
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
    renodx::utils::log::w("[world-rtao] pipeline creation failed: ", stage);
    data->pipeline_failed = true;
    return false;
  };
  if (data->layout.handle == 0u && !device->create_pipeline_layout(4, params, &data->layout)) return fail("pipeline layout");
  if (data->cbv_table.handle == 0u && !device->allocate_descriptor_table(data->layout, 0, &data->cbv_table)) return fail("cbv table");
  if (data->srv_table.handle == 0u && !device->allocate_descriptor_table(data->layout, 1, &data->srv_table)) return fail("srv table");
  if (data->uav_table.handle == 0u && !device->allocate_descriptor_table(data->layout, 2, &data->uav_table)) return fail("uav table");

  reshade::api::shader_desc shader = {};
  shader.code = __world_rtao.data();
  shader.code_size = __world_rtao.size();
  shader.entry_point = "main";
  reshade::api::pipeline_subobject subobject = {
      reshade::api::pipeline_subobject_type::compute_shader, 1, &shader};
  if (!device->create_pipeline(data->layout, 1, &subobject, &data->pipeline)) return fail("compute pipeline");
  renodx::utils::log::i("[world-rtao] pipeline ready");
  return true;
#else
  data->pipeline_failed = true;
  renodx::utils::log::w("[world-rtao] pipeline creation failed: shader not embedded");
  return false;
#endif
}

// Creates the AO target at width x height, filled with 255 (neutral). A size change
// replaces the old target; the caller destroys it with no lock held.
inline bool EnsureRtaoTarget(reshade::api::device* device, RtaoDeviceData* data, uint32_t width, uint32_t height) {
  if (data->ao_texture.handle != 0u && data->ao_width == width && data->ao_height == height) return true;
  DestroyRtaoTarget(device, data);
  const reshade::api::format format = reshade::api::format::r32_uint;
  reshade::api::resource_desc desc = {};
  desc.type = reshade::api::resource_type::texture_2d;
  desc.texture = {width, height, 1, 1, format, 1};
  desc.heap = reshade::api::memory_heap::gpu_only;
  desc.usage = reshade::api::resource_usage::shader_resource | reshade::api::resource_usage::unordered_access;
  const std::vector<uint8_t> neutral(static_cast<size_t>(width) * height * 4u, 0xFFu);
  reshade::api::subresource_data initial = {};
  initial.data = const_cast<uint8_t*>(neutral.data());
  initial.row_pitch = width * 4u;
  initial.slice_pitch = width * height * 4u;
  if (!device->create_resource(desc, &initial, reshade::api::resource_usage::shader_resource, &data->ao_texture)) {
    renodx::utils::log::w("[world-rtao] AO texture creation failed");
    return false;
  }
  const reshade::api::resource_view_desc view(reshade::api::resource_view_type::texture_2d, format, 0, 1, 0, 1);
  if (!device->create_resource_view(data->ao_texture, reshade::api::resource_usage::shader_resource, view, &data->ao_srv)
      || !device->create_resource_view(data->ao_texture, reshade::api::resource_usage::unordered_access, view, &data->ao_uav)) {
    renodx::utils::log::w("[world-rtao] AO view creation failed");
    DestroyRtaoTarget(device, data);
    return false;
  }
  data->ao_width = width;
  data->ao_height = height;
  return true;
}

inline bool EnsureRtaoStats(reshade::api::device* device, RtaoDeviceData* data) {
  if (data->stats_buffer.handle != 0u) return true;
  const uint32_t zeros[kRtaoStatsCount] = {};
  return bvh::CreatePoolBuffer(
      device, zeros, sizeof(zeros), sizeof(uint32_t),
      &data->stats_buffer, &data->stats_uav,
      reshade::api::resource_usage::unordered_access | reshade::api::resource_usage::copy_source);
}

inline void DestroyRtaoDeviceData(reshade::api::device* device) {
  const auto found = g_rtao_devices.find(device);
  if (found == g_rtao_devices.end()) return;
  RtaoDeviceData& data = found->second;
  DestroyRtaoTarget(device, &data);
  bvh::DestroyBuffer(device, &data.stats_uav, &data.stats_buffer);
  bvh::DestroyGpuTimer(&data.timer);
  if (data.pipeline.handle != 0u) device->destroy_pipeline(data.pipeline);
  if (data.srv_table.handle != 0u) device->free_descriptor_table(data.srv_table);
  if (data.uav_table.handle != 0u) device->free_descriptor_table(data.uav_table);
  if (data.cbv_table.handle != 0u) device->free_descriptor_table(data.cbv_table);
  if (data.layout.handle != 0u) device->destroy_pipeline_layout(data.layout);
  g_rtao_devices.erase(found);
}

}  // namespace falcom_world::rtao
