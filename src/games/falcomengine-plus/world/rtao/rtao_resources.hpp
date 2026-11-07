#pragma once

// RTAO GPU resources: pass A (world_rtao.cs_5_0.hlsl: the trace), its trace-size raw AO (r16_float, -1 = not traced)
// and the stats buffer. The SVGF denoise and the output are in rtao_svgf.hpp.
// Created lazily from the lighting hook (rtao.hpp); no module mutex is held around any graphics call here.

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

#include "../../../../utils/log.hpp"
#include "../bvh/bvh_resources.hpp"

namespace falcom_world::rtao {

// t0..t8 BVH, t9 game depth, t10..t13 characters (null unless Trace characters), t14..t16 alpha,
// t17 MRT normal, t18 IS-FAST noise, t19 P2b records (bvh_resources.hpp DynamicExpandGpu; the trace reads them at t17).
inline constexpr uint32_t kRtaoSrvCount = 20u;
// Pass A: u0 unused (null), u1 stats, u2 trace-size raw AO.
inline constexpr uint32_t kRtaoUavCount = 3u;
inline constexpr uint32_t kRtaoPushRegister = 12u;
inline constexpr uint32_t kRtaoPushConstantCount = 24u;
// Stats buffer: pass A indices 0..9 (RTAO_STAT_COUNT in world_rtao.cs_5_0.hlsl), discovery 16..23, SVGF reproject 20..22
// (kRtaoStatDenoiseBase).
inline constexpr uint32_t kRtaoStatsCount = 24u;
inline constexpr uint32_t kRtaoStatDenoiseBase = 20u;
// D1b: motion vs camera-matrix difference, indices 33..39 (bins, sum, max; see RTAO_FSTAT_* in the pass B shader);
// jitterDiff_g.x and .y as bit patterns at 40 and 41.
inline constexpr uint32_t kRtaoStatJitterX = 40u;
inline constexpr uint32_t kRtaoStatJitterY = 41u;
// Two-Sided discovery (pass A, diagnostic): indices 16..23, see TwoSidedDiscoveryIndex in rtao_state.hpp.
inline constexpr uint32_t kRtaoStatDiscoveryBase = 16u;
inline constexpr uint32_t kRtaoStatDiscoveryCount = 8u;
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
inline constexpr uint32_t kRtaoFlagDiscovery = 1u;    // b12 c2.z bits (RTAO_FLAG_* in world_rtao.cs_5_0.hlsl)
inline constexpr uint32_t kRtaoFlagFoliageSkip = 2u;

// b12 for pass A, matching cbuffer cb_rtao in world_rtao.cs_5_0.hlsl (five float4 registers).
struct RtaoPushConstants {
  float params[4];    // c0: radius, ray_max, strength, normal_bias
  float sampling[4];  // c1: spp, isfast flag (1 loaded), slice base, seed
  float fade[4];      // c2: fade start_eff, fade end_eff, flags (z: kRtaoFlag* as a float), P2b record count (w, uint bits)
  float region[4];    // c3: region min xyz, region size
  float size[4];      // c4: width, height, character object count, output_raw (1 = write u2 raw AO, not u0)
  float trace[4];     // c5: trace divisor d in x (1 Full, 2 Half, 4 Quarter); the trace extent is ceil(size / d)
};
static_assert(sizeof(RtaoPushConstants) == kRtaoPushConstantCount * sizeof(float), "b12 must be 24 floats");
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
  bool layout_failed = false;
  reshade::api::resource stats_buffer = {0u};
  reshade::api::resource_view stats_uav = {0u};
  bvh::GpuTimer timer = {};
  // Pass A writes the trace-size raw AO (r16_float, -1 = not traced); the SVGF input pass reads it.
  reshade::api::resource trace_texture = {0u};
  reshade::api::resource_view trace_srv = {0u};
  reshade::api::resource_view trace_uav = {0u};
  uint32_t trace_width = 0u;
  uint32_t trace_height = 0u;
};

// One entry per device (destroyed with the device).
inline std::unordered_map<reshade::api::device*, RtaoDeviceData> g_rtao_devices;

inline RtaoDeviceData& GetRtaoDeviceData(reshade::api::device* device) { return g_rtao_devices[device]; }

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
    data->layout_failed = true;
    return false;
  };
  if (data->layout.handle == 0u && !device->create_pipeline_layout(4, params, &data->layout)) return fail("pipeline layout");
  if (data->cbv_table.handle == 0u && !device->allocate_descriptor_table(data->layout, 0, &data->cbv_table)) return fail("cbv table");
  if (data->srv_table.handle == 0u && !device->allocate_descriptor_table(data->layout, 1, &data->srv_table)) return fail("srv table");
  if (data->uav_table.handle == 0u && !device->allocate_descriptor_table(data->layout, 2, &data->uav_table)) return fail("uav table");
  return true;
}

inline bool EnsureRtaoPipeline(reshade::api::device* device, RtaoDeviceData* data) {
  if (data->pipeline.handle != 0u) return true;
  if (data->pipeline_failed || !EnsureRtaoLayout(device, data)) return false;

#if defined(__world_rtao_EMBED_FILE)
  reshade::api::shader_desc shader = {};
  shader.code = __world_rtao.data();
  shader.code_size = __world_rtao.size();
  shader.entry_point = "main";
  reshade::api::pipeline_subobject subobject = {
      reshade::api::pipeline_subobject_type::compute_shader, 1, &shader};
  if (!device->create_pipeline(data->layout, 1, &subobject, &data->pipeline)) {
    renodx::utils::log::w("[world-rtao] compute pipeline creation failed");
    data->pipeline_failed = true;
    return false;
  }
  renodx::utils::log::i("[world-rtao] pipeline ready");
  return true;
#else
  data->pipeline_failed = true;
  renodx::utils::log::w("[world-rtao] pipeline creation failed: shader not embedded");
  return false;
#endif
}

inline bool EnsureRtaoStats(reshade::api::device* device, RtaoDeviceData* data) {
  if (data->stats_buffer.handle != 0u) return true;
  const uint32_t zeros[kRtaoStatsCount] = {};
  return bvh::CreatePoolBuffer(
      device, zeros, sizeof(zeros), sizeof(uint32_t),
      &data->stats_buffer, &data->stats_uav,
      reshade::api::resource_usage::unordered_access | reshade::api::resource_usage::copy_source);
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

inline void DestroyRtaoTraceTarget(reshade::api::device* device, RtaoDeviceData* data) {
  bvh::DestroyBuffer(device, &data->trace_srv, &data->trace_texture);
  bvh::DestroyBuffer(device, &data->trace_uav, nullptr);
  data->trace_width = 0u;
  data->trace_height = 0u;
}

// Trace-size raw AO (r16_float, -1 = not traced). A size change replaces it.
inline bool EnsureRtaoTraceTarget(reshade::api::device* device, RtaoDeviceData* data, uint32_t width, uint32_t height) {
  if (data->trace_texture.handle != 0u && data->trace_width == width && data->trace_height == height) return true;
  DestroyRtaoTraceTarget(device, data);
  if (!CreateRtaoTexture(device, reshade::api::format::r16_float, width, height, 2u,
                         &data->trace_texture, &data->trace_srv, &data->trace_uav)) {
    DestroyRtaoTraceTarget(device, data);
    renodx::utils::log::w("[world-rtao] trace target creation failed");
    return false;
  }
  data->trace_width = width;
  data->trace_height = height;
  return true;
}


inline void DestroyRtaoDeviceData(reshade::api::device* device) {
  const auto found = g_rtao_devices.find(device);
  if (found == g_rtao_devices.end()) return;
  RtaoDeviceData& data = found->second;
  DestroyRtaoTraceTarget(device, &data);
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
