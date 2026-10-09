#pragma once

// RTAO GPU resources: pass A (world_rtao.cs_5_0.hlsl: the trace), the AO target (r32_uint at depth size,
// 0..255 texels, 255 = neutral), the stats buffer, and round 2 pass B (world_rtao_temporal.cs_5_0.hlsl: the
// temporal accumulation) with its raw AO and history targets (created only while Temporal is on).
// Created lazily from the lighting hook (rtao.hpp); no module mutex is held around any graphics call here.

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
// Pass A: u0 AO, u1 stats, u2 raw AO (used only with output_raw). Pass B: u0 AO, u1 stats, u2 history write.
inline constexpr uint32_t kRtaoUavCount = 3u;
inline constexpr uint32_t kRtaoPushRegister = 12u;
inline constexpr uint32_t kRtaoPushConstantCount = 20u;
// Stats buffer: pass A indices 0..9 (RTAO_STAT_COUNT in world_rtao.cs_5_0.hlsl), pass B 10..15 (RTAO_TSTAT_*), discovery 16..23, pass B diagnostics 24..31 (RTAO_FSTAT_* in world_rtao_temporal.cs_5_0.hlsl).
inline constexpr uint32_t kRtaoStatsCount = 68u;
// P0-B2 frame-to-frame raw difference, indices 52..54: sum |raw - previous raw| (x1000), pair count, count below 0.01.
inline constexpr uint32_t kRtaoStatRawBase = 52u;
inline constexpr uint32_t kRtaoStatRawCount = 3u;
// P0-B output quality (diagnostic, valid on a still camera only), indices 55..61: sums x1000, pair counts (see RTAO_QSTAT_* in the pass B shader).
inline constexpr uint32_t kRtaoStatQualityBase = 55u;
inline constexpr uint32_t kRtaoStatQualityCount = 7u;
// P0-C normal-dot histogram of the highest-weight tap (valid history, in bounds), indices 62..67: dot <0, <0.5, <0.7, <0.9, <0.97, >=0.97.
inline constexpr uint32_t kRtaoStatNormalBase = 62u;
inline constexpr uint32_t kRtaoStatNormalCount = 6u;
// P0-B depth-ratio histogram of the highest-weight history tap, indices 45..51 (invalid, <0.1%, <0.5%, <1%, <2%, <5%, >=5%).
inline constexpr uint32_t kRtaoStatDepthBase = 45u;
inline constexpr uint32_t kRtaoStatDepthCount = 7u;
// P0 weighted taps, indices 42..44: accepted, rejected by depth, rejected by normal (bilinear weight x1000 per traced pixel).
inline constexpr uint32_t kRtaoStatWeightBase = 42u;
inline constexpr uint32_t kRtaoStatWeightCount = 3u;
// D1b: motion vs camera-matrix difference, indices 33..39 (bins, sum, max; see RTAO_FSTAT_* in the pass B shader);
// jitterDiff_g.x and .y as bit patterns at 40 and 41.
inline constexpr uint32_t kRtaoStatJitterX = 40u;
inline constexpr uint32_t kRtaoStatJitterY = 41u;
// Pass B diagnostics (F1), indices 24..31: moving pixels, motion max (1/100 px), motion sum (1/100 px),
// clamp active, clamp shift sum (1/1000), alpha sum (1/1000), |raw - AO| sum (1/1000), no-history pixels.
inline constexpr uint32_t kRtaoStatTemporalFBase = 24u;
inline constexpr uint32_t kRtaoStatTemporalFCount = 16u;  // 32: texel_differs (D1); 33..39: D1b
// Two-Sided discovery (pass A, diagnostic): indices 16..23, see TwoSidedDiscoveryIndex in rtao_state.hpp.
inline constexpr uint32_t kRtaoStatDiscoveryBase = 16u;
inline constexpr uint32_t kRtaoStatDiscoveryCount = 8u;
inline constexpr uint32_t kRtaoStatTemporalBase = 10u;
inline constexpr uint32_t kRtaoTemporalSrvCount = 6u;      // t0 raw AO, t1 depth, t2 MRT normal, t3 motion, t4 history, t5 previous raw AO
inline constexpr uint32_t kRtaoTemporalUavCount = 3u;      // u0 AO, u1 stats, u2 history write
inline constexpr uint32_t kRtaoTemporalSamplerCount = 2u;  // s0 point clamp, s1 linear clamp
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

// b12 for pass A, matching cbuffer cb_rtao in world_rtao.cs_5_0.hlsl (five float4 registers).
struct RtaoPushConstants {
  float params[4];    // c0: radius, ray_max, strength, normal_bias
  float sampling[4];  // c1: spp, isfast flag (1 loaded), slice base, seed
  float fade[4];      // c2: fade start_eff, fade end_eff, 0, 0
  float region[4];    // c3: region min xyz, region size
  float size[4];      // c4: width, height, dynamic count (0), output_raw (1 = write u2 raw AO, not u0)
};
static_assert(sizeof(RtaoPushConstants) == kRtaoPushConstantCount * sizeof(float), "b12 must be 20 floats");
static_assert(offsetof(RtaoPushConstants, sampling) == 4 * sizeof(float), "b12 c1 offset");
static_assert(offsetof(RtaoPushConstants, fade) == 8 * sizeof(float), "b12 c2 offset");
static_assert(offsetof(RtaoPushConstants, region) == 12 * sizeof(float), "b12 c3 offset");
static_assert(offsetof(RtaoPushConstants, size) == 16 * sizeof(float), "b12 c4 offset");

// b12 for pass B, matching cbuffer cb_rtao_temporal in world_rtao_temporal.cs_5_0.hlsl.
struct RtaoTemporalPushConstants {
  float params[4];     // c0: history weight, depth rejection, normal rejection, history clamp (sigma)
  float texel[4];      // c1: texel size xy, prevResolutionScale xy
  float size[4];       // c2: width, height, debug mode, history valid (0 = reset)
  float reserved0[4];  // c3: x = test, force zero motion (1 = history looked up at the same pixel)
  float reserved1[4];  // c4
};
static_assert(sizeof(RtaoTemporalPushConstants) == kRtaoPushConstantCount * sizeof(float), "pass B b12 must be 20 floats");
static_assert(offsetof(RtaoTemporalPushConstants, texel) == 4 * sizeof(float), "pass B c1 offset");
static_assert(offsetof(RtaoTemporalPushConstants, size) == 8 * sizeof(float), "pass B c2 offset");

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
  // Round 2 (temporal): pass B pipeline, samplers, raw AO and two history targets (created only while Temporal is on).
  reshade::api::pipeline_layout temporal_layout = {0u};
  reshade::api::descriptor_table temporal_cbv_table = {0u};
  reshade::api::descriptor_table temporal_srv_table = {0u};
  reshade::api::descriptor_table temporal_uav_table = {0u};
  reshade::api::descriptor_table temporal_sampler_table = {0u};
  reshade::api::pipeline temporal_pipeline = {0u};
  bool temporal_pipeline_failed = false;
  reshade::api::sampler point_sampler = {0u};
  reshade::api::sampler linear_sampler = {0u};
  reshade::api::resource raw_texture[2] = {};
  reshade::api::resource_view raw_srv[2] = {};
  reshade::api::resource_view raw_uav[2] = {};
  uint32_t raw_index = 0u;  // raw_texture[raw_index] is written by pass A; the other one holds the previous frame
  reshade::api::resource history_texture[2] = {};
  reshade::api::resource_view history_srv[2] = {};
  reshade::api::resource_view history_uav[2] = {};
  uint32_t temporal_width = 0u;
  uint32_t temporal_height = 0u;
  uint32_t history_index = 0u;  // history[history_index] is read by pass B; the other one is written
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

inline void DestroyRtaoTemporalTargets(reshade::api::device* device, RtaoDeviceData* data) {
  for (int i = 0; i < 2; ++i) {
    bvh::DestroyBuffer(device, &data->raw_srv[i], &data->raw_texture[i]);
    bvh::DestroyBuffer(device, &data->raw_uav[i], nullptr);
  }
  data->raw_index = 0u;
  for (int i = 0; i < 2; ++i) {
    bvh::DestroyBuffer(device, &data->history_srv[i], &data->history_texture[i]);
    bvh::DestroyBuffer(device, &data->history_uav[i], nullptr);
  }
  data->temporal_width = 0u;
  data->temporal_height = 0u;
}

// Raw AO (r16_float) and two history targets (rgba16f: R accumulated AO, G distance, BA octahedral normal).
// A size change replaces them. Created only while Temporal is on; the caller holds no lock.
inline bool EnsureRtaoTemporalTargets(reshade::api::device* device, RtaoDeviceData* data, uint32_t width, uint32_t height) {
  if (data->raw_texture[0].handle != 0u && data->temporal_width == width && data->temporal_height == height) return true;
  DestroyRtaoTemporalTargets(device, data);
  const bool created = CreateRtaoTexture(device, reshade::api::format::r16_float, width, height, 2u,
                                         &data->raw_texture[0], &data->raw_srv[0], &data->raw_uav[0])
      && CreateRtaoTexture(device, reshade::api::format::r16_float, width, height, 2u,
                           &data->raw_texture[1], &data->raw_srv[1], &data->raw_uav[1])
      && CreateRtaoTexture(device, reshade::api::format::r16g16b16a16_float, width, height, 8u,
                           &data->history_texture[0], &data->history_srv[0], &data->history_uav[0])
      && CreateRtaoTexture(device, reshade::api::format::r16g16b16a16_float, width, height, 8u,
                           &data->history_texture[1], &data->history_srv[1], &data->history_uav[1]);
  if (!created) {
    DestroyRtaoTemporalTargets(device, data);
    renodx::utils::log::w("[world-rtao] temporal target creation failed");
    return false;
  }
  data->temporal_width = width;
  data->temporal_height = height;
  return true;
}

inline bool EnsureRtaoTemporalPipeline(reshade::api::device* device, RtaoDeviceData* data) {
  if (data->temporal_pipeline.handle != 0u) return true;
  if (data->temporal_pipeline_failed) return false;

#if defined(__world_rtao_temporal_EMBED_FILE)
  using DR = reshade::api::descriptor_range;
  using DS = reshade::api::shader_stage;
  using DT = reshade::api::descriptor_type;
  using P = reshade::api::pipeline_layout_param;

  DR cbv_range = {0, 0, 0, 1, DS::all_compute, 1, DT::constant_buffer};
  DR srv_range = {0, 0, 0, kRtaoTemporalSrvCount, DS::all_compute, 1, DT::shader_resource_view};
  DR uav_range = {0, 0, 0, kRtaoTemporalUavCount, DS::all_compute, 1, DT::unordered_access_view};
  DR sampler_range = {0, 0, 0, kRtaoTemporalSamplerCount, DS::all_compute, 1, DT::sampler};
  reshade::api::constant_range push_range = {};
  push_range.binding = 0;
  push_range.dx_register_index = kRtaoPushRegister;
  push_range.dx_register_space = 0;
  push_range.count = kRtaoPushConstantCount;
  push_range.visibility = DS::all_compute;
  P params[5] = {};
  params[0].type = reshade::api::pipeline_layout_param_type::descriptor_table;
  params[0].descriptor_table.count = 1;
  params[0].descriptor_table.ranges = &cbv_range;
  params[1].type = reshade::api::pipeline_layout_param_type::descriptor_table;
  params[1].descriptor_table.count = 1;
  params[1].descriptor_table.ranges = &srv_range;
  params[2].type = reshade::api::pipeline_layout_param_type::descriptor_table;
  params[2].descriptor_table.count = 1;
  params[2].descriptor_table.ranges = &uav_range;
  params[3].type = reshade::api::pipeline_layout_param_type::descriptor_table;
  params[3].descriptor_table.count = 1;
  params[3].descriptor_table.ranges = &sampler_range;
  params[4].type = reshade::api::pipeline_layout_param_type::push_constants;
  params[4].push_constants = push_range;
  const auto fail = [&](const char* stage) {
    renodx::utils::log::w("[world-rtao] temporal pipeline creation failed: ", stage);
    data->temporal_pipeline_failed = true;
    return false;
  };
  if (data->temporal_layout.handle == 0u && !device->create_pipeline_layout(5, params, &data->temporal_layout)) return fail("pipeline layout");
  if (data->temporal_cbv_table.handle == 0u && !device->allocate_descriptor_table(data->temporal_layout, 0, &data->temporal_cbv_table)) return fail("cbv table");
  if (data->temporal_srv_table.handle == 0u && !device->allocate_descriptor_table(data->temporal_layout, 1, &data->temporal_srv_table)) return fail("srv table");
  if (data->temporal_uav_table.handle == 0u && !device->allocate_descriptor_table(data->temporal_layout, 2, &data->temporal_uav_table)) return fail("uav table");
  if (data->temporal_sampler_table.handle == 0u && !device->allocate_descriptor_table(data->temporal_layout, 3, &data->temporal_sampler_table)) return fail("sampler table");
  if (data->point_sampler.handle == 0u) {
    reshade::api::sampler_desc sd = {};
    sd.filter = reshade::api::filter_mode::min_mag_mip_point;
    sd.address_u = sd.address_v = sd.address_w = reshade::api::texture_address_mode::clamp;
    if (!device->create_sampler(sd, &data->point_sampler)) return fail("point sampler");
  }
  if (data->linear_sampler.handle == 0u) {
    reshade::api::sampler_desc sd = {};
    sd.filter = reshade::api::filter_mode::min_mag_mip_linear;
    sd.address_u = sd.address_v = sd.address_w = reshade::api::texture_address_mode::clamp;
    if (!device->create_sampler(sd, &data->linear_sampler)) return fail("linear sampler");
  }

  reshade::api::shader_desc shader = {};
  shader.code = __world_rtao_temporal.data();
  shader.code_size = __world_rtao_temporal.size();
  shader.entry_point = "main";
  reshade::api::pipeline_subobject subobject = {
      reshade::api::pipeline_subobject_type::compute_shader, 1, &shader};
  if (!device->create_pipeline(data->temporal_layout, 1, &subobject, &data->temporal_pipeline)) return fail("compute pipeline");
  renodx::utils::log::i("[world-rtao] temporal pipeline ready");
  return true;
#else
  data->temporal_pipeline_failed = true;
  renodx::utils::log::w("[world-rtao] temporal pipeline creation failed: shader not embedded");
  return false;
#endif
}

inline void DestroyRtaoDeviceData(reshade::api::device* device) {
  const auto found = g_rtao_devices.find(device);
  if (found == g_rtao_devices.end()) return;
  RtaoDeviceData& data = found->second;
  DestroyRtaoTarget(device, &data);
  DestroyRtaoTemporalTargets(device, &data);
  bvh::DestroyBuffer(device, &data.stats_uav, &data.stats_buffer);
  bvh::DestroyGpuTimer(&data.timer);
  if (data.pipeline.handle != 0u) device->destroy_pipeline(data.pipeline);
  if (data.srv_table.handle != 0u) device->free_descriptor_table(data.srv_table);
  if (data.uav_table.handle != 0u) device->free_descriptor_table(data.uav_table);
  if (data.cbv_table.handle != 0u) device->free_descriptor_table(data.cbv_table);
  if (data.layout.handle != 0u) device->destroy_pipeline_layout(data.layout);
  if (data.temporal_pipeline.handle != 0u) device->destroy_pipeline(data.temporal_pipeline);
  if (data.temporal_srv_table.handle != 0u) device->free_descriptor_table(data.temporal_srv_table);
  if (data.temporal_uav_table.handle != 0u) device->free_descriptor_table(data.temporal_uav_table);
  if (data.temporal_cbv_table.handle != 0u) device->free_descriptor_table(data.temporal_cbv_table);
  if (data.temporal_sampler_table.handle != 0u) device->free_descriptor_table(data.temporal_sampler_table);
  if (data.temporal_layout.handle != 0u) device->destroy_pipeline_layout(data.temporal_layout);
  if (data.point_sampler.handle != 0u) device->destroy_sampler(data.point_sampler);
  if (data.linear_sampler.handle != 0u) device->destroy_sampler(data.linear_sampler);
  g_rtao_devices.erase(found);
}

}  // namespace falcom_world::rtao
