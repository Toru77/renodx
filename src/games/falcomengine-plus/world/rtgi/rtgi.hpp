#pragma once

// RTGI dispatch: the trace, the temporal pass, the spatial filter and the upsample, and the stats readback. Called
// inline from the lighting hook (addon.cpp RunRtgiInline), which owns the guards. No module mutex is held around any
// graphics call here.

#include <atomic>
#include <cstdint>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>

#include "../bvh/bvh_trace.hpp"
#include "../rtao/rtao.hpp"
#include "rtgi_resources.hpp"
#include "rtgi_state.hpp"

namespace falcom_world::rtgi {

struct RtgiFrameState {
  Reason reason = Reason::Off;
  bool producing = false;      // the RTGI trace ran for this frame
  bool debug_written = false;  // the pass that owns the RTGI Debug view wrote the debug texture this frame
  uint64_t dispatched_frame = UINT64_MAX;
  float gpu_ms = -1.f;
  uint32_t divisor = 1u;
  uint32_t trace_width = 0u;
  uint32_t trace_height = 0u;
  uint32_t full_width = 0u;
  uint32_t full_height = 0u;
  float ray_max_requested = 0.f;
  float ray_max_effective = 0.f;
  bool fade_valid = false;
  rtao::Fade fade = {};
  float samples = 1.f;
  bool isfast_used = false;
  uint64_t texture_bytes = 0u;
  bool radiance_valid = false;
  uint32_t radiance_width = 0u;
  uint32_t radiance_height = 0u;
  uint32_t radiance_format = 0u;
  uint32_t prev_view_proj_register = 0u;
  float denoise_gpu_ms = -1.f;        // temporal, filter and upsample (negative: none yet)
  bool temporal_ran = false;          // the temporal pass ran this frame
  bool temporal_last = false;         // RTGI Temporal was on and ran in the last frame that ran the reset rule
  rtao::ResetReason reset = rtao::ResetReason::Disabled;
  rtao::ResetReason last_reset = rtao::ResetReason::Disabled;
  uint64_t reset_counts[static_cast<size_t>(rtao::ResetReason::Count)] = {};
  bool prev_params_valid = false;
  ParameterSnapshot prev_params = {};
  bool filter_ran = false;            // the spatial filter ran this frame
  uint64_t applied_frame = UINT64_MAX;  // last frame whose lighting draw got the bounce (RunRtgiInline push_gi)
  float applied_intensity = 0.f;        // GI Intensity applied in applied_frame
  bool applied_debug = false;           // applied_frame showed RTGI Debug 7 (the lighting output was the bounce alone)
  bool subtracted = false;              // this frame's trace subtracted last frame's applied bounce (one bounce)
};

inline RtgiFrameState g_rtgi_frame;

struct RtgiStatsState {
  std::atomic_bool requested{false};
  bool valid = false;
  uint32_t values[kRtgiStatsCount] = {};
};

inline RtgiStatsState g_rtgi_stats;

struct RtgiDispatchInputs {
  reshade::api::resource_view depth_view = {0u};
  reshade::api::resource_view mrt_normal_view = {0u};
  reshade::api::resource_view isfast_view = {0u};
  reshade::api::resource_view scene_cbv_view = {0u};
  reshade::api::resource_view radiance_view = {0u};
  reshade::api::resource_view prev_gi_view = {0u};  // t21: last frame's final GI (one-bounce correction; null: off)
  reshade::api::resource_view albedo_view = {0u};   // t22: this frame's G-buffer colour (one-bounce correction; null: off)
  uint32_t full_width = 0u;
  uint32_t full_height = 0u;
  uint32_t trace_width = 0u;
  uint32_t trace_height = 0u;
  RtgiPushConstants push = {};
  int debug_mode = 0;
  reshade::api::resource_view motion_view = {0u};  // RTV4 motion; null: the temporal pass does not run
  bool temporal = false;
  RtgiTemporalPushConstants temporal_push = {};  // params and history valid (size[3]) set by the caller
  bool spatial = false;
  int filter_type = 1;    // 0 separable bilateral, 1 a-trous
  int filter_radius = 4;
  int filter_passes = 2;
  int filter_taps = 5;    // samples per direction (separable) or kernel side (a-trous)
};

struct RtgiDispatchResult {
  bool ok = false;
  bool debug_written = false;
  bool temporal_ran = false;
  bool filter_ran = false;
};

inline RtgiDispatchResult Dispatch(reshade::api::device* device, reshade::api::command_list* cmd_list, const RtgiDispatchInputs& in) {
  const bvh::PoolCpuTimer cpu_timer{&bvh::g_perf_gate.rtgi_record_us};
  auto* bvh_data = bvh::GetBvhDeviceData(device);
  if (bvh_data == nullptr || in.trace_width == 0u || in.trace_height == 0u) return {};
  RtgiDeviceData& data = GetRtgiDeviceData(device);
  // Creates one RTGI compute pipeline on first use (RtgiPipeline layout); a failure is latched and logged once.
  const auto ensure_pipeline = [&](RtgiPipeline* p, const RtgiPipelineDesc& desc) {
    if (p->pipeline.handle != 0u) return true;
    if (p->failed) return false;
    const auto fail = [&](const char* stage) {
      renodx::utils::log::w("[world-rtgi] ", desc.name, " pipeline creation failed: ", stage);
      p->failed = true;
      return false;
    };
    if (desc.code.empty()) return fail("shader not embedded");
    using DR = reshade::api::descriptor_range;
    using DS = reshade::api::shader_stage;
    using DT = reshade::api::descriptor_type;
    using P = reshade::api::pipeline_layout_param;
    DR cbv_range = {0, 0, 0, 1, DS::all_compute, 1, DT::constant_buffer};
    DR srv_range = {0, 0, 0, desc.srv_count, DS::all_compute, 1, DT::shader_resource_view};
    DR uav_range = {0, 0, 0, desc.uav_count, DS::all_compute, 1, DT::unordered_access_view};
    DR sampler_range = {0, 0, 0, desc.sampler_count, DS::all_compute, 1, DT::sampler};
    reshade::api::constant_range push_range = {};
    push_range.binding = 0;
    push_range.dx_register_index = rtao::kRtaoPushRegister;
    push_range.dx_register_space = 0;
    push_range.count = desc.push_count;
    push_range.visibility = DS::all_compute;
    const uint32_t push_index = desc.sampler_count > 0u ? 4u : 3u;
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
    if (desc.sampler_count > 0u) {
      params[3].type = reshade::api::pipeline_layout_param_type::descriptor_table;
      params[3].descriptor_table.count = 1;
      params[3].descriptor_table.ranges = &sampler_range;
    }
    params[push_index].type = reshade::api::pipeline_layout_param_type::push_constants;
    params[push_index].push_constants = push_range;
    if (p->layout.handle == 0u && !device->create_pipeline_layout(push_index + 1u, params, &p->layout)) return fail("pipeline layout");
    if (p->cbv_table.handle == 0u && !device->allocate_descriptor_table(p->layout, 0, &p->cbv_table)) return fail("cbv table");
    if (p->srv_table.handle == 0u && !device->allocate_descriptor_table(p->layout, 1, &p->srv_table)) return fail("srv table");
    if (p->uav_table.handle == 0u && !device->allocate_descriptor_table(p->layout, 2, &p->uav_table)) return fail("uav table");
    if (desc.sampler_count > 0u && p->sampler_table.handle == 0u
        && !device->allocate_descriptor_table(p->layout, 3, &p->sampler_table)) {
      return fail("sampler table");
    }
    reshade::api::shader_desc shader = {};
    shader.code = desc.code.data();
    shader.code_size = desc.code.size();
    shader.entry_point = "main";
    reshade::api::pipeline_subobject subobject = {reshade::api::pipeline_subobject_type::compute_shader, 1, &shader};
    if (!device->create_pipeline(p->layout, 1, &subobject, &p->pipeline)) return fail("compute pipeline");
    renodx::utils::log::i("[world-rtgi] ", desc.name, " pipeline ready");
    return true;
  };
  if (!ensure_pipeline(&data.trace_pipeline, {.srv_count = kRtgiSrvCount, .uav_count = kRtgiUavCount, .sampler_count = 0u,
                                              .push_count = kRtgiPushConstantCount, .code = kRtgiTraceShader, .name = "trace"})
      || !ensure_pipeline(&data.upsample_pipeline, {.srv_count = kRtgiUpsampleSrvCount, .uav_count = kRtgiUpsampleUavCount,
                                                    .sampler_count = 0u, .push_count = kRtgiUpsamplePushCount,
                                                    .code = kRtgiUpsampleShader, .name = "upsample"})) {
    return {};
  }
  if (data.stats_buffer.handle == 0u) {
    const uint32_t zeros[kRtgiStatsCount] = {};
    if (!bvh::CreatePoolBuffer(
            device, zeros, sizeof(zeros), sizeof(uint32_t),
            &data.stats_buffer, &data.stats_uav,
            reshade::api::resource_usage::unordered_access | reshade::api::resource_usage::copy_source)) {
      return {};
    }
  }
  bool debug_ready = false;
  if (in.debug_mode >= 1 && in.debug_mode <= 6) {
    debug_ready = EnsureRtgiTextureSet(device, &data.debug, reshade::api::format::r8g8b8a8_unorm, 4u, in.full_width, in.full_height);
    if (!debug_ready) {
      renodx::utils::log::w("[world-rtgi] debug texture creation failed");
    }
  }
  // Views the trace pass writes (modes 1 to 3); a failed debug texture leaves u0 null and does not fail the dispatch.
  const bool trace_debug = debug_ready && in.debug_mode <= 3;
  if (!EnsureRtgiTextureSet(device, &data.raw, reshade::api::format::r16g16b16a16_float, 8u, in.trace_width, in.trace_height)) {
    renodx::utils::log::w("[world-rtgi] raw GI texture creation failed");
    return {};
  }
  if (!EnsureRtgiTextureSet(device, &data.final_gi, reshade::api::format::r11g11b10_float, 4u, in.full_width, in.full_height)) {
    renodx::utils::log::w("[world-rtgi] final GI texture creation failed");
    return {};
  }
  // Temporal: pipeline, two history sets (a new or resized set starts at index 0, all invalid) and the samplers.
  bool temporal_run = in.temporal && in.motion_view.handle != 0u
      && ensure_pipeline(&data.temporal_pipeline, {.srv_count = kRtgiTemporalSrvCount, .uav_count = kRtgiTemporalUavCount,
                                                   .sampler_count = kRtgiTemporalSamplerCount, .push_count = kRtgiTemporalPushCount,
                                                   .code = kRtgiTemporalShader, .name = "temporal"});
  if (temporal_run) {
    const bool history_recreated = data.history_color.texture[0].handle == 0u || data.history_color.width != in.trace_width
        || data.history_color.height != in.trace_height;
    temporal_run = EnsureRtgiTextureSet(device, &data.history_color, reshade::api::format::r16g16b16a16_float, 8u, in.trace_width, in.trace_height)
        && EnsureRtgiTextureSet(device, &data.history_normal, reshade::api::format::r16g16_float, 4u, in.trace_width, in.trace_height);
    if (!temporal_run) {
      renodx::utils::log::w("[world-rtgi] history texture creation failed");
    } else if (history_recreated) {
      data.history_index = 0u;
    }
  }
  if (temporal_run && data.point_sampler.handle == 0u) {
    reshade::api::sampler_desc sd = {};
    sd.filter = reshade::api::filter_mode::min_mag_mip_point;
    sd.address_u = sd.address_v = sd.address_w = reshade::api::texture_address_mode::clamp;
    temporal_run = device->create_sampler(sd, &data.point_sampler);
  }
  if (temporal_run && data.linear_sampler.handle == 0u) {
    reshade::api::sampler_desc sd = {};
    sd.filter = reshade::api::filter_mode::min_mag_mip_linear;
    sd.address_u = sd.address_v = sd.address_w = reshade::api::texture_address_mode::clamp;
    temporal_run = device->create_sampler(sd, &data.linear_sampler);
  }
  const bool spatial_run = in.spatial
      && ensure_pipeline(&data.filter_pipeline, {.srv_count = kRtgiFilterSrvCount, .uav_count = kRtgiFilterUavCount,
                                                 .sampler_count = 0u, .push_count = kRtgiFilterPushCount,
                                                 .code = kRtgiFilterShader, .name = "filter"})
      && EnsureRtgiTextureSet(device, &data.filter, reshade::api::format::r16g16b16a16_float, 8u, in.trace_width, in.trace_height);
  const bool temporal_debug = debug_ready && temporal_run && (in.debug_mode == 4 || in.debug_mode == 5);
  const bool upsample_debug = debug_ready && in.debug_mode == 6;

  RtgiPushConstants push = in.push;
  push.size[0] = static_cast<float>(in.trace_width);
  push.size[1] = static_cast<float>(in.trace_height);
  push.size[2] = static_cast<float>(in.full_width);
  push.size[3] = static_cast<float>(in.full_height);
  std::memcpy(&push.fade[3], &bvh_data->dynamic.instance_count, sizeof(uint32_t));  // P2b records (t19)

  // Whole-buffer write (offset 0, full size): see WriteBufferRange in bvh_resources.hpp.
  const uint32_t zeros[kRtgiStatsCount] = {};
  device->update_buffer_region(zeros, data.stats_buffer, 0u, sizeof(zeros));

  const reshade::api::descriptor_type kCbv = reshade::api::descriptor_type::constant_buffer;
  const reshade::api::descriptor_type kSrv = reshade::api::descriptor_type::shader_resource_view;
  const reshade::api::descriptor_type kUav = reshade::api::descriptor_type::unordered_access_view;
  const uint32_t trace_groups_x = (in.trace_width + 7u) / 8u;
  const uint32_t trace_groups_y = (in.trace_height + 7u) / 8u;

  // Trace.
  bvh::BeginGpuTimer(device, cmd_list, &data.timer);
  const RtgiPipeline& trace = data.trace_pipeline;
  reshade::api::resource_view srvs[kRtgiSrvCount] = {};
  bvh::FillBvhSceneSrvs(*bvh_data, in.depth_view, bvh::DynamicTraceInputs{}, srvs);
  srvs[17] = in.mrt_normal_view;
  srvs[18] = in.isfast_view;
  srvs[19] = bvh_data->dynamic.instances_srv;
  srvs[20] = in.radiance_view;
  srvs[21] = in.prev_gi_view;
  srvs[22] = in.albedo_view;
  const reshade::api::resource_view uavs[kRtgiUavCount] = {
      trace_debug ? data.debug.uav[0] : reshade::api::resource_view{0u}, data.stats_uav, data.raw.uav[0]};
  reshade::api::descriptor_table_update updates[3] = {
      {trace.cbv_table, 0, 0, 1, kCbv, &in.scene_cbv_view},
      {trace.srv_table, 0, 0, kRtgiSrvCount, kSrv, srvs},
      {trace.uav_table, 0, 0, kRtgiUavCount, kUav, uavs},
  };
  device->update_descriptor_tables(3, updates);
  cmd_list->bind_pipeline(reshade::api::pipeline_stage::all_compute, trace.pipeline);
  const reshade::api::descriptor_table tables[3] = {trace.cbv_table, trace.srv_table, trace.uav_table};
  cmd_list->bind_descriptor_tables(reshade::api::shader_stage::all_compute, trace.layout, 0, 3, tables);
  cmd_list->push_constants(reshade::api::shader_stage::all_compute, trace.layout, 3, 0, kRtgiPushConstantCount, &push);
  cmd_list->dispatch(trace_groups_x, trace_groups_y, 1u);
  rtao::NullComputeSlots(cmd_list, kRtgiSrvCount, kRtgiUavCount, 0u);
  if (trace_debug) {
    cmd_list->barrier(data.debug.texture[0], reshade::api::resource_usage::unordered_access, reshade::api::resource_usage::shader_resource);
  }
  cmd_list->barrier(data.raw.texture[0], reshade::api::resource_usage::unordered_access, reshade::api::resource_usage::shader_resource);
  bvh::EndGpuTimer(cmd_list, &data.timer);

  bvh::BeginGpuTimer(device, cmd_list, &data.denoise_timer);
  reshade::api::resource_view input = data.raw.srv[0];

  // Temporal: reads history[history_index], writes the other one; that written history is the pass output.
  if (temporal_run) {
    const RtgiPipeline& temporal = data.temporal_pipeline;
    const uint32_t read = data.history_index;
    const uint32_t write = data.history_index ^ 1u;
    const reshade::api::resource_view temporal_srvs[kRtgiTemporalSrvCount] = {
        data.raw.srv[0], in.depth_view, in.mrt_normal_view, in.motion_view, data.history_color.srv[read], data.history_normal.srv[read]};
    const reshade::api::resource_view temporal_uavs[kRtgiTemporalUavCount] = {
        data.history_color.uav[write], data.stats_uav, data.history_normal.uav[write],
        temporal_debug ? data.debug.uav[0] : reshade::api::resource_view{0u}};
    const reshade::api::sampler samplers[kRtgiTemporalSamplerCount] = {data.point_sampler, data.linear_sampler};
    reshade::api::descriptor_table_update temporal_updates[4] = {
        {temporal.cbv_table, 0, 0, 1, kCbv, &in.scene_cbv_view},
        {temporal.srv_table, 0, 0, kRtgiTemporalSrvCount, kSrv, temporal_srvs},
        {temporal.uav_table, 0, 0, kRtgiTemporalUavCount, kUav, temporal_uavs},
        {temporal.sampler_table, 0, 0, kRtgiTemporalSamplerCount, reshade::api::descriptor_type::sampler, samplers},
    };
    device->update_descriptor_tables(4, temporal_updates);
    RtgiTemporalPushConstants tpush = in.temporal_push;
    tpush.texel[0] = 1.f / static_cast<float>(in.full_width);
    tpush.texel[1] = 1.f / static_cast<float>(in.full_height);
    tpush.texel[2] = 1.f;  // prevResolutionScale (1, 1)
    tpush.texel[3] = 1.f;
    tpush.size[0] = static_cast<float>(in.trace_width);
    tpush.size[1] = static_cast<float>(in.trace_height);
    tpush.size[2] = static_cast<float>(in.debug_mode);
    tpush.full[0] = static_cast<float>(in.full_width);
    tpush.full[1] = static_cast<float>(in.full_height);
    tpush.full[2] = in.push.params[3];  // trace divisor
    cmd_list->bind_pipeline(reshade::api::pipeline_stage::all_compute, temporal.pipeline);
    const reshade::api::descriptor_table temporal_tables[4] = {
        temporal.cbv_table, temporal.srv_table, temporal.uav_table, temporal.sampler_table};
    cmd_list->bind_descriptor_tables(reshade::api::shader_stage::all_compute, temporal.layout, 0, 4, temporal_tables);
    cmd_list->push_constants(reshade::api::shader_stage::all_compute, temporal.layout, 4, 0, kRtgiTemporalPushCount, &tpush);
    cmd_list->dispatch(trace_groups_x, trace_groups_y, 1u);
    rtao::NullComputeSlots(cmd_list, kRtgiTemporalSrvCount, kRtgiTemporalUavCount, kRtgiTemporalSamplerCount);
    cmd_list->barrier(data.history_color.texture[write], reshade::api::resource_usage::unordered_access, reshade::api::resource_usage::shader_resource);
    cmd_list->barrier(data.history_normal.texture[write], reshade::api::resource_usage::unordered_access, reshade::api::resource_usage::shader_resource);
    if (temporal_debug) {
      cmd_list->barrier(data.debug.texture[0], reshade::api::resource_usage::unordered_access, reshade::api::resource_usage::shader_resource);
    }
    data.history_index = write;
    input = data.history_color.srv[write];
  }

  // Spatial filter: the RTAO filter plan (rtao::MakeFilterPlan) over F0, F1 and B; the first pass reads the input.
  if (spatial_run) {
    const RtgiPipeline& filter = data.filter_pipeline;
    const rtao::FilterPlan plan = rtao::MakeFilterPlan(in.filter_type, in.filter_radius, in.filter_passes, in.filter_taps);
    cmd_list->bind_pipeline(reshade::api::pipeline_stage::all_compute, filter.pipeline);
    for (uint32_t i = 0u; i < plan.pass_count; ++i) {
      const rtao::FilterPass& pass = plan.passes[i];
      // F0 = 0, F1 = 1, B = 2 (FilterSlot values 1..3), as rtao.hpp DispatchSpatialFilter.
      const uint32_t out_index = static_cast<uint32_t>(pass.output) - 1u;
      const reshade::api::resource_view filter_srvs[kRtgiFilterSrvCount] = {
          in.depth_view, in.mrt_normal_view,
          pass.input == rtao::FilterSlot::A ? input : data.filter.srv[static_cast<uint32_t>(pass.input) - 1u]};
      const reshade::api::resource_view filter_uavs[kRtgiFilterUavCount] = {data.filter.uav[out_index]};
      reshade::api::descriptor_table_update filter_updates[3] = {
          {filter.cbv_table, 0, 0, 1, kCbv, &in.scene_cbv_view},
          {filter.srv_table, 0, 0, kRtgiFilterSrvCount, kSrv, filter_srvs},
          {filter.uav_table, 0, 0, kRtgiFilterUavCount, kUav, filter_uavs},
      };
      device->update_descriptor_tables(3, filter_updates);
      const RtgiFilterPushConstants fpush = {
          .size = {static_cast<float>(in.trace_width), static_cast<float>(in.trace_height), static_cast<float>(in.filter_radius),
                   plan.kind == rtao::FilterKind::Separable ? 0.f : 1.f},
          .pass = {static_cast<float>(pass.direction), static_cast<float>(pass.step), static_cast<float>(plan.taps_side),
                   i + 1u == plan.pass_count ? 1.f : 0.f},
          .full = {static_cast<float>(in.full_width), static_cast<float>(in.full_height), in.push.params[3], 0.f},
      };
      const reshade::api::descriptor_table filter_tables[3] = {filter.cbv_table, filter.srv_table, filter.uav_table};
      cmd_list->bind_descriptor_tables(reshade::api::shader_stage::all_compute, filter.layout, 0, 3, filter_tables);
      cmd_list->push_constants(reshade::api::shader_stage::all_compute, filter.layout, 3, 0, kRtgiFilterPushCount, &fpush);
      cmd_list->dispatch(trace_groups_x, trace_groups_y, 1u);
      rtao::NullComputeSlots(cmd_list, kRtgiFilterSrvCount, kRtgiFilterUavCount, 0u);
      cmd_list->barrier(data.filter.texture[out_index], reshade::api::resource_usage::unordered_access, reshade::api::resource_usage::shader_resource);
    }
    input = data.filter.srv[2];
  }

  // Upsample to the depth size: writes final_gi[final_index ^ 1], which becomes the newest.
  const RtgiPipeline& upsample = data.upsample_pipeline;
  const uint32_t final_write = data.final_index ^ 1u;
  const reshade::api::resource_view upsample_srvs[kRtgiUpsampleSrvCount] = {in.depth_view, in.mrt_normal_view, input};
  const reshade::api::resource_view upsample_uavs[kRtgiUpsampleUavCount] = {
      data.final_gi.uav[final_write], upsample_debug ? data.debug.uav[0] : reshade::api::resource_view{0u}};
  reshade::api::descriptor_table_update upsample_updates[3] = {
      {upsample.cbv_table, 0, 0, 1, kCbv, &in.scene_cbv_view},
      {upsample.srv_table, 0, 0, kRtgiUpsampleSrvCount, kSrv, upsample_srvs},
      {upsample.uav_table, 0, 0, kRtgiUpsampleUavCount, kUav, upsample_uavs},
  };
  device->update_descriptor_tables(3, upsample_updates);
  const RtgiUpsamplePushConstants upush = {
      .size = {static_cast<float>(in.full_width), static_cast<float>(in.full_height), static_cast<float>(in.trace_width),
               static_cast<float>(in.trace_height)},
      .params = {in.push.params[3], static_cast<float>(in.debug_mode), 0.f, 0.f},
  };
  cmd_list->bind_pipeline(reshade::api::pipeline_stage::all_compute, upsample.pipeline);
  const reshade::api::descriptor_table upsample_tables[3] = {upsample.cbv_table, upsample.srv_table, upsample.uav_table};
  cmd_list->bind_descriptor_tables(reshade::api::shader_stage::all_compute, upsample.layout, 0, 3, upsample_tables);
  cmd_list->push_constants(reshade::api::shader_stage::all_compute, upsample.layout, 3, 0, kRtgiUpsamplePushCount, &upush);
  cmd_list->dispatch((in.full_width + 7u) / 8u, (in.full_height + 7u) / 8u, 1u);
  rtao::NullComputeSlots(cmd_list, kRtgiUpsampleSrvCount, kRtgiUpsampleUavCount, 0u);
  cmd_list->barrier(data.final_gi.texture[final_write], reshade::api::resource_usage::unordered_access, reshade::api::resource_usage::shader_resource);
  if (upsample_debug) {
    cmd_list->barrier(data.debug.texture[0], reshade::api::resource_usage::unordered_access, reshade::api::resource_usage::shader_resource);
  }
  data.final_index = final_write;
  bvh::EndGpuTimer(cmd_list, &data.denoise_timer);

  // App-side tables: null contents (ReShade keeps descriptors in the table object). The CBV tables keep their view.
  const reshade::api::resource_view table_null_views[kRtgiSrvCount] = {};
  const reshade::api::sampler table_null_samplers[kRtgiTemporalSamplerCount] = {};
  std::vector<reshade::api::descriptor_table_update> table_nulls = {
      {trace.srv_table, 0, 0, kRtgiSrvCount, kSrv, table_null_views},
      {trace.uav_table, 0, 0, kRtgiUavCount, kUav, table_null_views},
      {upsample.srv_table, 0, 0, kRtgiUpsampleSrvCount, kSrv, table_null_views},
      {upsample.uav_table, 0, 0, kRtgiUpsampleUavCount, kUav, table_null_views},
  };
  if (temporal_run) {
    table_nulls.push_back({data.temporal_pipeline.srv_table, 0, 0, kRtgiTemporalSrvCount, kSrv, table_null_views});
    table_nulls.push_back({data.temporal_pipeline.uav_table, 0, 0, kRtgiTemporalUavCount, kUav, table_null_views});
    table_nulls.push_back({data.temporal_pipeline.sampler_table, 0, 0, kRtgiTemporalSamplerCount,
                           reshade::api::descriptor_type::sampler, table_null_samplers});
  }
  if (spatial_run) {
    table_nulls.push_back({data.filter_pipeline.srv_table, 0, 0, kRtgiFilterSrvCount, kSrv, table_null_views});
    table_nulls.push_back({data.filter_pipeline.uav_table, 0, 0, kRtgiFilterUavCount, kUav, table_null_views});
  }
  device->update_descriptor_tables(static_cast<uint32_t>(table_nulls.size()), table_nulls.data());
  return {.ok = true,
          .debug_written = trace_debug || temporal_debug || upsample_debug,
          .temporal_ran = temporal_run,
          .filter_ran = spatial_run};
}

// Copies the lighting output (render target 0 of the lighting draw) for the next frame's RTGI trace: the radiance of
// on-screen hits. Called from the lighting draw's on_drawn hook (addon.cpp OnDrawnLightingShader); no lock is held.
inline void CaptureRtgiRadiance(reshade::api::device* device, reshade::api::command_list* cmd_list,
                                reshade::api::resource_view rtv, uint64_t frame_index) {
  const auto source = device->get_resource_from_view(rtv);
  if (source.handle == 0u) return;
  const auto desc = device->get_resource_desc(source);
  if (desc.type != reshade::api::resource_type::texture_2d || desc.texture.samples != 1) return;
  RtgiDeviceData& data = GetRtgiDeviceData(device);
  if (data.radiance_failed) return;
  const reshade::api::format format = desc.texture.format;
  if (data.radiance_texture.handle == 0u || data.radiance_width != desc.texture.width
      || data.radiance_height != desc.texture.height || data.radiance_format != format) {
    bvh::DestroyBuffer(device, &data.radiance_srv, &data.radiance_texture);
    reshade::api::resource_desc radiance_desc = {};
    radiance_desc.type = reshade::api::resource_type::texture_2d;
    radiance_desc.texture = {desc.texture.width, desc.texture.height, 1, 1, format, 1};
    radiance_desc.heap = reshade::api::memory_heap::gpu_only;
    radiance_desc.usage = reshade::api::resource_usage::shader_resource | reshade::api::resource_usage::copy_dest;
    if (!device->create_resource(radiance_desc, nullptr, reshade::api::resource_usage::shader_resource, &data.radiance_texture)
        || !device->create_resource_view(data.radiance_texture, reshade::api::resource_usage::shader_resource,
                                         reshade::api::resource_view_desc(reshade::api::resource_view_type::texture_2d,
                                                                          reshade::api::format_to_default_typed(format, 0), 0, 1, 0, 1),
                                         &data.radiance_srv)) {
      bvh::DestroyBuffer(device, &data.radiance_srv, &data.radiance_texture);
      data.radiance_failed = true;
      renodx::utils::log::w("[world-rtgi] radiance copy target creation failed (format ", static_cast<uint32_t>(format), ")");
      return;
    }
    data.radiance_width = desc.texture.width;
    data.radiance_height = desc.texture.height;
    data.radiance_format = format;
    renodx::utils::log::i("[world-rtgi] radiance copy: ", data.radiance_width, "x", data.radiance_height, " format ",
                          static_cast<uint32_t>(format));
  }
  cmd_list->barrier(source, reshade::api::resource_usage::render_target, reshade::api::resource_usage::copy_source);
  cmd_list->barrier(data.radiance_texture, reshade::api::resource_usage::shader_resource, reshade::api::resource_usage::copy_dest);
  cmd_list->copy_texture_region(source, 0u, nullptr, data.radiance_texture, 0u, nullptr);
  cmd_list->barrier(data.radiance_texture, reshade::api::resource_usage::copy_dest, reshade::api::resource_usage::shader_resource);
  cmd_list->barrier(source, reshade::api::resource_usage::copy_source, reshade::api::resource_usage::render_target);
  data.radiance_frame = frame_index;
}

struct RtgiJsonInput {
  const RtgiFrameState* frame;
  const uint32_t* values;
  uint64_t generated_frame;
  bool perf_gather;
};

inline std::string BuildRtgiJson(const RtgiJsonInput& in) {
  const RtgiFrameState& f = *in.frame;
  const uint32_t* v = in.values;
  float luminance_max = 0.f;
  std::memcpy(&luminance_max, &v[kRtgiStatLuminanceMax], sizeof(luminance_max));
  std::ostringstream out;
  out << "{\n";
  out << "  \"schema\": 1,\n";
  out << "  \"generated_frame\": " << in.generated_frame << ",\n";
  out << "  \"perf_gather\": " << (in.perf_gather ? "true" : "false") << ",\n";
  out << "  \"reason\": \"" << ReasonName(f.reason) << "\",\n";
  out << "  \"producing\": " << (f.producing ? "true" : "false") << ",\n";
  out << "  \"gpu_ms\": " << f.gpu_ms << ",\n";
  out << "  \"trace\": {\"divisor\": " << f.divisor << ", \"width\": " << f.trace_width
      << ", \"height\": " << f.trace_height << ", \"full_width\": " << f.full_width
      << ", \"full_height\": " << f.full_height << "},\n";
  out << "  \"ray_max\": {\"requested\": " << f.ray_max_requested << ", \"effective\": " << f.ray_max_effective << "},\n";
  out << "  \"fade\": {\"start_eff\": " << f.fade.start << ", \"end_eff\": " << f.fade.end
      << ", \"coverage\": " << f.fade.coverage << ", \"requested_start\": " << f.fade.start_req
      << ", \"requested_end\": " << f.fade.end_req << "},\n";
  out << "  \"radiance\": {\"valid\": " << (f.radiance_valid ? "true" : "false") << ", \"width\": " << f.radiance_width
      << ", \"height\": " << f.radiance_height << ", \"format\": " << f.radiance_format << "},\n";
  out << "  \"samples\": " << f.samples << ",\n";
  out << "  \"radiance_clamp\": " << g_rtgi_radiance_clamp << ",\n";
  out << "  \"ambient_contribution\": " << g_rtgi_ambient << ",\n";
  out << "  \"composite\": {\"intensity\": " << g_rtgi_intensity << ", \"character_intensity\": " << g_rtgi_char_intensity
      << ", \"dispatched_frame\": " << f.dispatched_frame << ", \"applied_frame\": " << f.applied_frame
      << ", \"applied_intensity\": " << f.applied_intensity << ", \"applied_debug\": " << (f.applied_debug ? "true" : "false")
      << ", \"subtract_previous\": " << (f.subtracted ? "true" : "false") << "},\n";
  out << "  \"isfast_used\": " << (f.isfast_used ? "true" : "false") << ",\n";
  out << "  \"prev_view_proj_register\": " << f.prev_view_proj_register << ",\n";
  out << "  \"texture_bytes\": " << f.texture_bytes << ",\n";
  out << "  \"denoise_gpu_ms\": " << f.denoise_gpu_ms << ",\n";
  out << "  \"temporal\": {\"on\": " << (g_rtgi_temporal_enabled > 0.5f ? "true" : "false")
      << ", \"ran\": " << (f.temporal_ran ? "true" : "false")
      << ", \"reset\": \"" << rtao::ResetReasonName(f.reset) << "\""
      << ", \"last_reset\": \"" << rtao::ResetReasonName(f.last_reset) << "\", \"reset_counts\": [";
  for (size_t r = 0; r < static_cast<size_t>(rtao::ResetReason::Count); ++r) {
    out << (r == 0u ? "" : ", ") << f.reset_counts[r];
  }
  out << "]},\n";
  out << "  \"spatial_filter\": {\"on\": " << (g_rtgi_spatial_enabled > 0.5f ? "true" : "false")
      << ", \"ran\": " << (f.filter_ran ? "true" : "false") << ", \"type\": " << g_rtgi_filter_type
      << ", \"radius\": " << g_rtgi_filter_radius << ", \"passes\": " << g_rtgi_filter_passes
      << ", \"taps\": " << (g_rtgi_filter_type > 0.5f ? g_rtgi_filter_taps_atrous : g_rtgi_filter_taps_separable) << "},\n";
  out << "  \"stats\": {\n";
  out << "    \"rays\": " << v[kRtgiStatRays] << ",\n";
  out << "    \"hits\": " << v[kRtgiStatHits] << ",\n";
  out << "    \"pixels\": " << v[kRtgiStatPixels] << ",\n";
  out << "    \"sky\": " << v[kRtgiStatSky] << ",\n";
  out << "    \"normal\": " << v[kRtgiStatNormal] << ",\n";
  out << "    \"region\": " << v[kRtgiStatRegion] << ",\n";
  out << "    \"beyond_fade\": " << v[kRtgiStatBeyondFade] << ",\n";
  out << "    \"invalid_refs\": " << v[kRtgiStatInvalidRefs] << ",\n";
  out << "    \"stack_overflow\": " << v[kRtgiStatStackOverflow] << ",\n";
  out << "    \"scaled\": " << v[kRtgiStatScaled] << ",\n";
  out << "    \"hit_on_screen\": " << v[kRtgiStatHitOnScreen] << ",\n";
  out << "    \"hit_occluded\": " << v[kRtgiStatHitOccluded] << ",\n";
  out << "    \"hit_off_screen\": " << v[kRtgiStatHitOffScreen] << ",\n";
  out << "    \"hit_distance_sum_dm\": " << v[kRtgiStatHitDistanceDm] << ",\n";
  out << "    \"radiance_lookups\": " << v[kRtgiStatRadianceLookups] << ",\n";
  out << "    \"sun_rays\": " << v[kRtgiStatSunRays] << ",\n";
  out << "    \"sun_visible\": " << v[kRtgiStatSunVisible] << ",\n";
  out << "    \"luminance_sum_x10\": " << ((static_cast<uint64_t>(v[kRtgiStatLuminanceSumHi]) << 32) | v[kRtgiStatLuminanceSum]) << ",\n";
  out << "    \"luminance_stat_cap\": " << kRtgiLuminanceStatCap << ",\n";
  out << "    \"luminance_max\": " << luminance_max << ",\n";
  out << "    \"clamped\": " << v[kRtgiStatClamped] << ",\n";
  out << "    \"temporal_pixels\": " << v[kRtgiStatTemporalPixels] << ",\n";
  out << "    \"no_history\": " << v[kRtgiStatNoHistory] << ",\n";
  out << "    \"reset_pixels\": " << v[kRtgiStatResetPixels] << "\n";
  out << "  },\n";
  out << "  \"note\": \"M4: the final GI is added to the image (Sora 2nd lighting shader) while GI Intensity > 0\"\n";
  out << "}\n";
  return out.str();
}

inline void MaybeCaptureRtgiStats(reshade::api::device* device, reshade::api::command_queue* queue) {
  if (!g_rtgi_stats.requested.exchange(false, std::memory_order_relaxed)) return;
  const auto found = g_rtgi_devices.find(device);
  if (found == g_rtgi_devices.end() || found->second.stats_buffer.handle == 0u) return;
  const RtgiDeviceData& data = found->second;
  std::vector<uint8_t> bytes;
  if (!renodx::utils::scene::ReadbackBuffer(device, queue, data.stats_buffer, 0u, sizeof(g_rtgi_stats.values), &bytes)
      || bytes.size() < sizeof(g_rtgi_stats.values)) {
    renodx::utils::log::w("[world-rtgi] stats readback failed");
    return;
  }
  std::memcpy(g_rtgi_stats.values, bytes.data(), sizeof(g_rtgi_stats.values));
  g_rtgi_stats.valid = true;
  const uint32_t* v = g_rtgi_stats.values;
  renodx::utils::log::i("[world-rtgi] stats: rays=", v[kRtgiStatRays], " hits=", v[kRtgiStatHits],
                        " on_screen=", v[kRtgiStatHitOnScreen], " occluded=", v[kRtgiStatHitOccluded],
                        " off_screen=", v[kRtgiStatHitOffScreen], " invalid_refs=", v[kRtgiStatInvalidRefs],
                        " stack_overflow=", v[kRtgiStatStackOverflow], " lookups=", v[kRtgiStatRadianceLookups],
                        " sun_rays=", v[kRtgiStatSunRays], " gpu_ms=", g_rtgi_frame.gpu_ms);

  const RtgiJsonInput in = {
      .frame = &g_rtgi_frame,
      .values = g_rtgi_stats.values,
      .generated_frame = g_state.frame.load(),
      .perf_gather = bvh::g_perf_gate.gather.load(std::memory_order_relaxed),
  };
  std::string text = BuildRtgiJson(in);
  renodx::utils::path::WriteTextFile(bvh::PoolOutputDir() / "world_rtgi.json", text);
}

// Gathering was switched OFF: clears the RTGI GPU timers and the newest RTGI frame times (see bvh::ClearPerfGathered).
inline void ClearRtgiPerfGathered(reshade::api::device* device) {
  RtgiDeviceData& data = GetRtgiDeviceData(device);
  bvh::ClearGpuTimerStats(&data.timer);
  bvh::ClearGpuTimerStats(&data.denoise_timer);
  g_rtgi_frame.gpu_ms = -1.f;
  g_rtgi_frame.denoise_gpu_ms = -1.f;
}

}  // namespace falcom_world::rtgi
