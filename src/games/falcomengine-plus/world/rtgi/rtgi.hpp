#pragma once

// RTGI dispatch: the trace, the temporal pass, the spatial filter and the upsample, and the stats readback. Called
// inline from the lighting hook (addon.cpp RunRtgiInline), which owns the guards. No module mutex is held around any
// graphics call here.

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

#include "../bvh/bvh_trace.hpp"
#include "../rtao/rtao.hpp"
#include "../world_state.hpp"
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
  uint32_t character_objects = 0u;  // character objects the newest trace used (sun.y)
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
  reshade::api::resource_view albedo_view = {0u};   // this frame's G-buffer colour: trace t22 (one-bounce correction) and upsample t3 (Debug 7); null: not captured this frame
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
  reshade::api::resource lights = {0u};         // Sora 2nd dynamicLights_g (t11); copied for the stats request, null otherwise
  reshade::api::resource light_indices = {0u};  // Sora 2nd lightIndices_g (t12); copied for the stats request, null otherwise
  reshade::api::resource_view lights_view = {0u};         // t23 lamps (Sora 2nd dynamicLights_g); null: no lamps
  reshade::api::resource_view light_indices_view = {0u};  // lamp type pass t0 (Sora 2nd lightIndices_g); null: no lamps
  // Sora 2nd lighting draw views for the O1 estimate (t25 mrtTexture1, t26 shadowTexture, t27 mrtTexture2, t28
  // deferredParams_g); null otherwise.
  reshade::api::resource_view mrt1_view = {0u};
  reshade::api::resource_view shadow_view = {0u};
  reshade::api::resource_view mrt2_view = {0u};
  reshade::api::resource_view deferred_view = {0u};
  bvh::DynamicTraceInputs dynamic = {};         // characters (RefitDeformLiveCharacters), count 0 when off
};

struct RtgiDispatchResult {
  bool ok = false;
  bool debug_written = false;
  bool temporal_ran = false;
  bool filter_ran = false;
};

// What a trace owner asks the denoise chain to prepare for this frame.
struct RtgiDenoiseRequest {
  reshade::api::format output_format;  // RTGI r11g11b10_float, RTLS r8g8b8a8_unorm
  uint32_t output_bytes_per_pixel;
  uint32_t trace_width;
  uint32_t trace_height;
  uint32_t full_width;
  uint32_t full_height;
  bool temporal;           // Temporal on and a motion view available
  bool spatial;            // Spatial filter on
  std::span<const std::uint8_t> filter_code;  // RTGI kRtgiFilterShader; RTLS kRtlsFilterShader (penumbra edge stop)
  const char* log_prefix;  // "[world-rtgi]" or "[world-rtls]"
};

// The passes the chain can run this frame (ok: the output exists).
struct RtgiDenoiseReady {
  bool ok = false;
  bool temporal = false;
  bool spatial = false;
};

// Creates the chain's objects for this frame's sizes, in this order: the output (full size), the temporal pipeline,
// history and samplers when requested, the spatial filter pipeline and targets when requested. A new or resized
// history starts at index 0 (all invalid). The owner ensures the upsample pipeline itself (kRtgiUpsamplePipelineDesc).
inline RtgiDenoiseReady EnsureRtgiDenoise(reshade::api::device* device, RtgiDenoiseSet* set, const RtgiDenoiseRequest& request) {
  if (!EnsureRtgiTextureSet(device, &set->output, request.output_format, request.output_bytes_per_pixel, request.full_width,
                            request.full_height)) {
    renodx::utils::log::w(request.log_prefix, " final texture creation failed");
    return {};
  }
  bool temporal_run = request.temporal
      && EnsureRtgiPipeline(device, &set->temporal_pipeline, {.srv_count = kRtgiTemporalSrvCount, .uav_count = kRtgiTemporalUavCount,
                                                              .sampler_count = kRtgiTemporalSamplerCount, .push_count = kRtgiTemporalPushCount,
                                                              .code = kRtgiTemporalShader, .name = "temporal"});
  if (temporal_run) {
    const bool history_recreated = set->history_color.texture[0].handle == 0u || set->history_color.width != request.trace_width
        || set->history_color.height != request.trace_height;
    temporal_run = EnsureRtgiTextureSet(device, &set->history_color, reshade::api::format::r16g16b16a16_float, 8u, request.trace_width, request.trace_height)
        && EnsureRtgiTextureSet(device, &set->history_normal, reshade::api::format::r16g16_float, 4u, request.trace_width, request.trace_height);
    if (!temporal_run) {
      renodx::utils::log::w(request.log_prefix, " history texture creation failed");
    } else if (history_recreated) {
      set->history_index = 0u;
    }
  }
  if (temporal_run && set->point_sampler.handle == 0u) {
    reshade::api::sampler_desc sd = {};
    sd.filter = reshade::api::filter_mode::min_mag_mip_point;
    sd.address_u = sd.address_v = sd.address_w = reshade::api::texture_address_mode::clamp;
    temporal_run = device->create_sampler(sd, &set->point_sampler);
  }
  if (temporal_run && set->linear_sampler.handle == 0u) {
    reshade::api::sampler_desc sd = {};
    sd.filter = reshade::api::filter_mode::min_mag_mip_linear;
    sd.address_u = sd.address_v = sd.address_w = reshade::api::texture_address_mode::clamp;
    temporal_run = device->create_sampler(sd, &set->linear_sampler);
  }
  const bool spatial_run = request.spatial
      && EnsureRtgiPipeline(device, &set->filter_pipeline, {.srv_count = kRtgiFilterSrvCount, .uav_count = kRtgiFilterUavCount,
                                                            .sampler_count = 0u, .push_count = kRtgiFilterPushCount,
                                                            .code = request.filter_code, .name = "filter"})
      && EnsureRtgiTextureSet(device, &set->filter, reshade::api::format::r16g16b16a16_float, 8u, request.trace_width, request.trace_height);
  return {.ok = true, .temporal = temporal_run, .spatial = spatial_run};
}

// The inputs of one run of the chain.
struct RtgiDenoiseInputs {
  reshade::api::resource_view raw_view;         // trace size, a < 0 = not traced
  reshade::api::resource_view depth_view;
  reshade::api::resource_view mrt_normal_view;
  reshade::api::resource_view scene_cbv_view;
  reshade::api::resource_view motion_view;      // RTV4 (read only when ready.temporal)
  reshade::api::resource_view albedo_view;      // upsample t3 (RTGI Debug 7); null for RTLS
  reshade::api::resource_view stats_uav;        // temporal u1 (stats 20..22)
  reshade::api::resource_view debug_uav;        // null: no debug view this frame
  reshade::api::resource debug_texture;
  uint32_t trace_width;
  uint32_t trace_height;
  uint32_t full_width;
  uint32_t full_height;
  float divisor;                                // trace divisor d (1, 2 or 4)
  int debug_mode;                               // RTGI numbering: 4 accumulated, 5 history confidence, 6 and 7 upsample views
  RtgiTemporalPushConstants temporal_push;      // params and size[3] (history valid) set by the owner
  int filter_type;
  int filter_radius;
  int filter_passes;
  int filter_taps;
  float filter_value_stop;                      // filter c2.w: RTLS penumbra edge stop k; RTGI 0
  float upsample_intensity;                     // upsample c1.z (Debug 7 only)
  float upsample_char_intensity;                // upsample c1.w (Debug 7 only)
  float upsample_saturation;                    // upsample c2.x; 1 = unchanged
};

struct RtgiDenoiseResult {
  bool temporal_debug = false;  // the temporal pass wrote the debug texture
  bool upsample_debug = false;  // the upsample wrote the debug texture
};

// Runs temporal (reads history[history_index], writes the other one), the spatial filter (rtao::MakeFilterPlan over F0,
// F1 and B) and the upsample into output[output_index ^ 1], which becomes the newest. Appends the null updates of its
// own tables to *table_nulls (upsample, then temporal, then filter); the owner applies them with its own.
inline RtgiDenoiseResult DispatchRtgiDenoise(reshade::api::device* device, reshade::api::command_list* cmd_list,
                                             RtgiDenoiseSet* set, const RtgiDenoiseInputs& in, const RtgiDenoiseReady& ready,
                                             std::vector<reshade::api::descriptor_table_update>* table_nulls) {
  const reshade::api::descriptor_type kCbv = reshade::api::descriptor_type::constant_buffer;
  const reshade::api::descriptor_type kSrv = reshade::api::descriptor_type::shader_resource_view;
  const reshade::api::descriptor_type kUav = reshade::api::descriptor_type::unordered_access_view;
  const uint32_t trace_groups_x = (in.trace_width + 7u) / 8u;
  const uint32_t trace_groups_y = (in.trace_height + 7u) / 8u;
  const bool temporal_debug = in.debug_uav.handle != 0u && ready.temporal && (in.debug_mode == 4 || in.debug_mode == 5);
  const bool upsample_debug = in.debug_uav.handle != 0u && (in.debug_mode == 6 || in.debug_mode == 7);
  bvh::BeginGpuTimer(device, cmd_list, &set->timer);
  reshade::api::resource_view input = in.raw_view;
  // Temporal: reads history[history_index], writes the other one; that written history is the pass output.
  if (ready.temporal) {
    const RtgiPipeline& temporal = set->temporal_pipeline;
    const uint32_t read = set->history_index;
    const uint32_t write = set->history_index ^ 1u;
    const reshade::api::resource_view temporal_srvs[kRtgiTemporalSrvCount] = {
        in.raw_view, in.depth_view, in.mrt_normal_view, in.motion_view, set->history_color.srv[read], set->history_normal.srv[read]};
    const reshade::api::resource_view temporal_uavs[kRtgiTemporalUavCount] = {
        set->history_color.uav[write], in.stats_uav, set->history_normal.uav[write],
        temporal_debug ? in.debug_uav : reshade::api::resource_view{0u}};
    const reshade::api::sampler samplers[kRtgiTemporalSamplerCount] = {set->point_sampler, set->linear_sampler};
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
    tpush.full[2] = in.divisor;  // trace divisor
    cmd_list->bind_pipeline(reshade::api::pipeline_stage::all_compute, temporal.pipeline);
    const reshade::api::descriptor_table temporal_tables[4] = {
        temporal.cbv_table, temporal.srv_table, temporal.uav_table, temporal.sampler_table};
    cmd_list->bind_descriptor_tables(reshade::api::shader_stage::all_compute, temporal.layout, 0, 4, temporal_tables);
    cmd_list->push_constants(reshade::api::shader_stage::all_compute, temporal.layout, 4, 0, kRtgiTemporalPushCount, &tpush);
    cmd_list->dispatch(trace_groups_x, trace_groups_y, 1u);
    rtao::NullComputeSlots(cmd_list, kRtgiTemporalSrvCount, kRtgiTemporalUavCount, kRtgiTemporalSamplerCount);
    cmd_list->barrier(set->history_color.texture[write], reshade::api::resource_usage::unordered_access, reshade::api::resource_usage::shader_resource);
    cmd_list->barrier(set->history_normal.texture[write], reshade::api::resource_usage::unordered_access, reshade::api::resource_usage::shader_resource);
    if (temporal_debug) {
      cmd_list->barrier(in.debug_texture, reshade::api::resource_usage::unordered_access, reshade::api::resource_usage::shader_resource);
    }
    set->history_index = write;
    input = set->history_color.srv[write];
  }

  // Spatial filter: the RTAO filter plan (rtao::MakeFilterPlan) over F0, F1 and B; the first pass reads the input.
  if (ready.spatial) {
    const RtgiPipeline& filter = set->filter_pipeline;
    const rtao::FilterPlan plan = rtao::MakeFilterPlan(in.filter_type, in.filter_radius, in.filter_passes, in.filter_taps);
    cmd_list->bind_pipeline(reshade::api::pipeline_stage::all_compute, filter.pipeline);
    for (uint32_t i = 0u; i < plan.pass_count; ++i) {
      const rtao::FilterPass& pass = plan.passes[i];
      // F0 = 0, F1 = 1, B = 2 (FilterSlot values 1..3), as rtao.hpp DispatchSpatialFilter.
      const uint32_t out_index = static_cast<uint32_t>(pass.output) - 1u;
      const reshade::api::resource_view filter_srvs[kRtgiFilterSrvCount] = {
          in.depth_view, in.mrt_normal_view,
          pass.input == rtao::FilterSlot::A ? input : set->filter.srv[static_cast<uint32_t>(pass.input) - 1u]};
      const reshade::api::resource_view filter_uavs[kRtgiFilterUavCount] = {set->filter.uav[out_index]};
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
          .full = {static_cast<float>(in.full_width), static_cast<float>(in.full_height), in.divisor, in.filter_value_stop},
      };
      const reshade::api::descriptor_table filter_tables[3] = {filter.cbv_table, filter.srv_table, filter.uav_table};
      cmd_list->bind_descriptor_tables(reshade::api::shader_stage::all_compute, filter.layout, 0, 3, filter_tables);
      cmd_list->push_constants(reshade::api::shader_stage::all_compute, filter.layout, 3, 0, kRtgiFilterPushCount, &fpush);
      cmd_list->dispatch(trace_groups_x, trace_groups_y, 1u);
      rtao::NullComputeSlots(cmd_list, kRtgiFilterSrvCount, kRtgiFilterUavCount, 0u);
      cmd_list->barrier(set->filter.texture[out_index], reshade::api::resource_usage::unordered_access, reshade::api::resource_usage::shader_resource);
    }
    input = set->filter.srv[2];
  }

  // Upsample to the depth size: writes final_gi[final_index ^ 1], which becomes the newest.
  const RtgiPipeline& upsample = set->upsample_pipeline;
  const uint32_t final_write = set->output_index ^ 1u;
  const reshade::api::resource_view upsample_srvs[kRtgiUpsampleSrvCount] = {in.depth_view, in.mrt_normal_view, input, in.albedo_view};
  const reshade::api::resource_view upsample_uavs[kRtgiUpsampleUavCount] = {
      set->output.uav[final_write], upsample_debug ? in.debug_uav : reshade::api::resource_view{0u}};
  reshade::api::descriptor_table_update upsample_updates[3] = {
      {upsample.cbv_table, 0, 0, 1, kCbv, &in.scene_cbv_view},
      {upsample.srv_table, 0, 0, kRtgiUpsampleSrvCount, kSrv, upsample_srvs},
      {upsample.uav_table, 0, 0, kRtgiUpsampleUavCount, kUav, upsample_uavs},
  };
  device->update_descriptor_tables(3, upsample_updates);
  const RtgiUpsamplePushConstants upush = {
      .size = {static_cast<float>(in.full_width), static_cast<float>(in.full_height), static_cast<float>(in.trace_width),
               static_cast<float>(in.trace_height)},
      .params = {in.divisor, static_cast<float>(in.debug_mode), in.upsample_intensity, in.upsample_char_intensity},
      .extra = {in.upsample_saturation, 0.f, 0.f, 0.f},
  };
  cmd_list->bind_pipeline(reshade::api::pipeline_stage::all_compute, upsample.pipeline);
  const reshade::api::descriptor_table upsample_tables[3] = {upsample.cbv_table, upsample.srv_table, upsample.uav_table};
  cmd_list->bind_descriptor_tables(reshade::api::shader_stage::all_compute, upsample.layout, 0, 3, upsample_tables);
  cmd_list->push_constants(reshade::api::shader_stage::all_compute, upsample.layout, 3, 0, kRtgiUpsamplePushCount, &upush);
  cmd_list->dispatch((in.full_width + 7u) / 8u, (in.full_height + 7u) / 8u, 1u);
  rtao::NullComputeSlots(cmd_list, kRtgiUpsampleSrvCount, kRtgiUpsampleUavCount, 0u);
  cmd_list->barrier(set->output.texture[final_write], reshade::api::resource_usage::unordered_access, reshade::api::resource_usage::shader_resource);
  if (upsample_debug) {
    cmd_list->barrier(in.debug_texture, reshade::api::resource_usage::unordered_access, reshade::api::resource_usage::shader_resource);
  }
  set->output_index = final_write;
  bvh::EndGpuTimer(cmd_list, &set->timer);
  static const reshade::api::resource_view null_views[kRtgiTemporalSrvCount] = {};
  static const reshade::api::sampler null_samplers[kRtgiTemporalSamplerCount] = {};
  table_nulls->push_back({upsample.srv_table, 0, 0, kRtgiUpsampleSrvCount, kSrv, null_views});
  table_nulls->push_back({upsample.uav_table, 0, 0, kRtgiUpsampleUavCount, kUav, null_views});
  if (ready.temporal) {
    table_nulls->push_back({set->temporal_pipeline.srv_table, 0, 0, kRtgiTemporalSrvCount, kSrv, null_views});
    table_nulls->push_back({set->temporal_pipeline.uav_table, 0, 0, kRtgiTemporalUavCount, kUav, null_views});
    table_nulls->push_back({set->temporal_pipeline.sampler_table, 0, 0, kRtgiTemporalSamplerCount,
                            reshade::api::descriptor_type::sampler, null_samplers});
  }
  if (ready.spatial) {
    table_nulls->push_back({set->filter_pipeline.srv_table, 0, 0, kRtgiFilterSrvCount, kSrv, null_views});
    table_nulls->push_back({set->filter_pipeline.uav_table, 0, 0, kRtgiFilterUavCount, kUav, null_views});
  }
  return {.temporal_debug = temporal_debug, .upsample_debug = upsample_debug};
}

inline RtgiDispatchResult Dispatch(reshade::api::device* device, reshade::api::command_list* cmd_list, const RtgiDispatchInputs& in) {
  const bvh::PoolCpuTimer cpu_timer{&bvh::g_perf_gate.rtgi_record_us};
  auto* bvh_data = bvh::GetBvhDeviceData(device);
  RtgiDeviceData& data = GetRtgiDeviceData(device);
  data.lights_copied = false;  // set again only when this dispatch copies both light buffers
  if (bvh_data == nullptr || in.trace_width == 0u || in.trace_height == 0u) return {};
  if (!EnsureRtgiPipeline(device, &data.trace_pipeline, {.srv_count = kRtgiSrvCount, .uav_count = kRtgiUavCount, .sampler_count = 0u,
                                                         .push_count = kRtgiPushConstantCount, .code = kRtgiTraceShader, .name = "trace"})
      || !EnsureRtgiPipeline(device, &data.denoise.upsample_pipeline, kRtgiUpsamplePipelineDesc)) {
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
  // Lamps (Off-screen Dynamic Lights): the type pass needs the lamp buffer (t23) and the game tile lists (t12).
  const bool lamps_run = in.lights_view.handle != 0u && in.light_indices_view.handle != 0u
      && EnsureRtgiPipeline(device, &data.lamp_type_pipeline,
                         {.srv_count = kRtgiLampTypeSrvCount, .uav_count = kRtgiLampTypeUavCount,
                          .sampler_count = 0u, .push_count = kRtgiLampTypePushCount,
                          .code = kRtgiLampTypeShader, .name = "lamp_types"});
  if (lamps_run && data.lamp_types.handle == 0u) {
    const uint32_t zeros[kRtgiLampCount] = {};
    if (!bvh::CreatePoolBuffer(device, zeros, sizeof(zeros), sizeof(uint32_t), &data.lamp_types, &data.lamp_types_uav,
                               reshade::api::resource_usage::unordered_access
                                   | reshade::api::resource_usage::shader_resource)
        || !device->create_resource_view(
               data.lamp_types, reshade::api::resource_usage::shader_resource,
               reshade::api::resource_view_desc(reshade::api::resource_view_type::buffer,
                                                reshade::api::format::unknown, 0, kRtgiLampCount),
               &data.lamp_types_srv)) {
      return {};
    }
  }
  bool debug_ready = false;
  if (in.debug_mode >= 1 && in.debug_mode <= 10) {
    debug_ready = EnsureRtgiTextureSet(device, &data.debug, reshade::api::format::r8g8b8a8_unorm, 4u, in.full_width, in.full_height);
    if (!debug_ready) {
      renodx::utils::log::w("[world-rtgi] debug texture creation failed");
    }
  }
  // Views the trace pass writes (modes 1 to 3, 8 to 10); a failed debug texture leaves u0 null and does not fail the
  // dispatch.
  const bool trace_debug =
      debug_ready && (in.debug_mode <= 3 || in.debug_mode == 8 || in.debug_mode == 9 || in.debug_mode == 10);
  if (!EnsureRtgiTextureSet(device, &data.raw, reshade::api::format::r16g16b16a16_float, 8u, in.trace_width, in.trace_height)) {
    renodx::utils::log::w("[world-rtgi] raw GI texture creation failed");
    return {};
  }
  const RtgiDenoiseReady denoise = EnsureRtgiDenoise(device, &data.denoise,
                                                     {.output_format = reshade::api::format::r11g11b10_float,
                                                      .output_bytes_per_pixel = 4u,
                                                      .trace_width = in.trace_width,
                                                      .trace_height = in.trace_height,
                                                      .full_width = in.full_width,
                                                      .full_height = in.full_height,
                                                      .temporal = in.temporal && in.motion_view.handle != 0u,
                                                      .spatial = in.spatial,
                                                      .filter_code = kRtgiFilterShader,
                                                      .log_prefix = "[world-rtgi]"});
  if (!denoise.ok) return {};

  RtgiPushConstants push = in.push;
  push.size[0] = static_cast<float>(in.trace_width);
  push.size[1] = static_cast<float>(in.trace_height);
  push.size[2] = static_cast<float>(in.full_width);
  push.size[3] = static_cast<float>(in.full_height);
  std::memcpy(&push.fade[3], &bvh_data->dynamic.instance_count, sizeof(uint32_t));  // P2b records (t19)
  push.sun[1] = static_cast<float>(in.dynamic.count);

  // Whole-buffer write (offset 0, full size): see WriteBufferRange in bvh_resources.hpp.
  const uint32_t zeros[kRtgiStatsCount] = {};
  device->update_buffer_region(zeros, data.stats_buffer, 0u, sizeof(zeros));

  const reshade::api::descriptor_type kCbv = reshade::api::descriptor_type::constant_buffer;
  const reshade::api::descriptor_type kSrv = reshade::api::descriptor_type::shader_resource_view;
  const reshade::api::descriptor_type kUav = reshade::api::descriptor_type::unordered_access_view;
  const uint32_t trace_groups_x = (in.trace_width + 7u) / 8u;
  const uint32_t trace_groups_y = (in.trace_height + 7u) / 8u;

  // Lamp types: u0 is zeroed, then each tile cell of the game light index list sets bit 1 (point) or bit 2 (spot)
  // for the lamps it lists (world_rtgi_lights.cs_5_0.hlsl). The trace reads t24 only when this pass ran.
  if (lamps_run) {
    const uint32_t lamp_zeros[kRtgiLampCount] = {};
    device->update_buffer_region(lamp_zeros, data.lamp_types, 0u, sizeof(lamp_zeros));
    const float lamp_count = static_cast<float>((std::min)(
        device->get_resource_desc(device->get_resource_from_view(in.lights_view)).buffer.size / kLightParamBytes,
        uint64_t{kRtgiLampCount}));
    const RtgiPipeline& lamp_types = data.lamp_type_pipeline;
    reshade::api::descriptor_table_update lamp_updates[2] = {
        {lamp_types.srv_table, 0, 0, kRtgiLampTypeSrvCount, kSrv, &in.light_indices_view},
        {lamp_types.uav_table, 0, 0, kRtgiLampTypeUavCount, kUav, &data.lamp_types_uav},
    };
    device->update_descriptor_tables(2, lamp_updates);
    cmd_list->bind_pipeline(reshade::api::pipeline_stage::all_compute, lamp_types.pipeline);
    const reshade::api::descriptor_table lamp_tables[2] = {lamp_types.srv_table, lamp_types.uav_table};
    cmd_list->bind_descriptor_tables(reshade::api::shader_stage::all_compute, lamp_types.layout, 1, 2, lamp_tables);
    cmd_list->push_constants(reshade::api::shader_stage::all_compute, lamp_types.layout, 3, 0, kRtgiLampTypePushCount,
                             &lamp_count);
    cmd_list->dispatch((kRtgiLightTileCount + 63u) / 64u, 1u, 1u);
    rtao::NullComputeSlots(cmd_list, kRtgiLampTypeSrvCount, kRtgiLampTypeUavCount, 0u);
    cmd_list->barrier(data.lamp_types, reshade::api::resource_usage::unordered_access,
                      reshade::api::resource_usage::shader_resource);
  }
  // Trace.
  bvh::BeginGpuTimer(device, cmd_list, &data.timer);
  const RtgiPipeline& trace = data.trace_pipeline;
  reshade::api::resource_view srvs[kRtgiSrvCount] = {};
  bvh::FillBvhSceneSrvs(*bvh_data, in.depth_view, in.dynamic, srvs);
  srvs[17] = in.mrt_normal_view;
  srvs[18] = in.isfast_view;
  srvs[19] = bvh_data->dynamic.instances_srv;
  srvs[20] = in.radiance_view;
  srvs[21] = in.prev_gi_view;
  srvs[22] = in.albedo_view;
  srvs[23] = lamps_run ? in.lights_view : reshade::api::resource_view{0u};
  srvs[24] = lamps_run ? data.lamp_types_srv : reshade::api::resource_view{0u};
  srvs[25] = in.mrt1_view;
  srvs[26] = in.shadow_view;
  srvs[27] = in.mrt2_view;
  srvs[28] = in.deferred_view;
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

  const reshade::api::resource_view table_null_views[kRtgiSrvCount] = {};
  std::vector<reshade::api::descriptor_table_update> table_nulls = {
      {trace.srv_table, 0, 0, kRtgiSrvCount, kSrv, table_null_views},
      {trace.uav_table, 0, 0, kRtgiUavCount, kUav, table_null_views},
  };
  const RtgiDenoiseResult denoised = DispatchRtgiDenoise(
      device, cmd_list, &data.denoise,
      {.raw_view = data.raw.srv[0],
       .depth_view = in.depth_view,
       .mrt_normal_view = in.mrt_normal_view,
       .scene_cbv_view = in.scene_cbv_view,
       .motion_view = in.motion_view,
       .albedo_view = in.albedo_view,
       .stats_uav = data.stats_uav,
       .debug_uav = debug_ready ? data.debug.uav[0] : reshade::api::resource_view{0u},
       .debug_texture = data.debug.texture[0],
       .trace_width = in.trace_width,
       .trace_height = in.trace_height,
       .full_width = in.full_width,
       .full_height = in.full_height,
       .divisor = in.push.params[3],
       .debug_mode = in.debug_mode,
       .temporal_push = in.temporal_push,
       .filter_type = in.filter_type,
       .filter_radius = in.filter_radius,
       .filter_passes = in.filter_passes,
       .filter_taps = in.filter_taps,
       .filter_value_stop = 0.f,
       .upsample_intensity = g_rtgi_intensity,
       .upsample_char_intensity = g_rtgi_char_intensity,
       .upsample_saturation = g_rtgi_saturation},
      denoise, &table_nulls);

  // Light buffers of the game's lighting draw (stats request only): copied now, read back by MaybeCaptureRtgiStats.
  const auto copy_light_buffer = [&](reshade::api::resource source, reshade::api::resource* copy, uint64_t* copy_bytes) {
    if (source.handle == 0u) return false;
    const reshade::api::resource_desc source_desc = device->get_resource_desc(source);
    if (source_desc.type != reshade::api::resource_type::buffer || source_desc.buffer.size == 0u
        || source_desc.buffer.size > renodx::utils::scene::CAPTURE_SIZE_LIMIT) {
      return false;
    }
    if (copy->handle == 0u || *copy_bytes != source_desc.buffer.size) {
      bvh::DestroyBuffer(device, nullptr, copy);
      reshade::api::resource_desc copy_desc = {};
      copy_desc.type = reshade::api::resource_type::buffer;
      copy_desc.buffer.size = source_desc.buffer.size;
      copy_desc.heap = reshade::api::memory_heap::gpu_only;
      copy_desc.usage = reshade::api::resource_usage::copy_dest | reshade::api::resource_usage::copy_source;
      if (!device->create_resource(copy_desc, nullptr, reshade::api::resource_usage::copy_dest, copy)) return false;
      *copy_bytes = source_desc.buffer.size;
    }
    cmd_list->barrier(source, reshade::api::resource_usage::shader_resource, reshade::api::resource_usage::copy_source);
    cmd_list->copy_buffer_region(source, 0u, *copy, 0u, source_desc.buffer.size);
    cmd_list->barrier(source, reshade::api::resource_usage::copy_source, reshade::api::resource_usage::shader_resource);
    return true;
  };
  data.lights_copied = copy_light_buffer(in.lights, &data.lights_copy, &data.lights_copy_bytes)
      && copy_light_buffer(in.light_indices, &data.light_indices_copy, &data.light_indices_copy_bytes);

  // App-side tables: null contents (ReShade keeps descriptors in the table object). The CBV tables keep their view.
  // The trace and the chain's tables were queued above; the lamp type tables follow.
  if (lamps_run) {
    table_nulls.push_back({data.lamp_type_pipeline.srv_table, 0, 0, kRtgiLampTypeSrvCount, kSrv, table_null_views});
    table_nulls.push_back({data.lamp_type_pipeline.uav_table, 0, 0, kRtgiLampTypeUavCount, kUav, table_null_views});
  }
  device->update_descriptor_tables(static_cast<uint32_t>(table_nulls.size()), table_nulls.data());
  return {.ok = true,
          .debug_written = trace_debug || denoised.temporal_debug || denoised.upsample_debug,
          .temporal_ran = denoise.temporal,
          .filter_ran = denoise.spatial};
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
  const CameraSnapshot* camera = nullptr;
};

inline void WriteVec3Json(std::ostream& out, const float* v) {
  out << "[" << bvh::PoolJsonFloat{v[0]} << ", " << bvh::PoolJsonFloat{v[1]} << ", " << bvh::PoolJsonFloat{v[2]} << "]";
}

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
  out << "  \"sun_shadow_distance\": " << g_rtgi_sun_distance << ",\n";
  out << "  \"sun_casters_only\": " << g_rtgi_sun_casters_only << ",\n";
  out << "  \"sun_contribution\": " << g_rtgi_sun_contribution << ",\n";
  out << "  \"fallback_no_albedo\": " << g_rtgi_fallback_no_albedo << ",\n";
  out << "  \"dynamic_lights\": " << g_rtgi_dynamic_lights << ",\n";
  out << "  \"dynamic_light_contribution\": " << g_rtgi_dynamic_light_contribution << ",\n";
  out << "  \"trace_characters\": " << g_rtgi_trace_characters << ",\n";
  out << "  \"game_sun_floor\": " << g_rtgi_game_sun << ",\n";
  out << "  \"character_objects\": " << f.character_objects << ",\n";
  if (in.camera != nullptr && in.camera->has_sun) {
    const CameraSnapshot& sun = *in.camera;
    out << "  \"game_sun\": {\"captured\": true, \"light_color\": ";
    WriteVec3Json(out, sun.light_color);
    out << ", \"light_direction\": ";
    WriteVec3Json(out, sun.light_direction);
    out << ", \"scene_shadow_color\": ";
    WriteVec3Json(out, sun.scene_shadow_color);
    out << ", \"shadow_cascade_count\": " << sun.shadow_cascade_count << ", \"shadow_split_distance\": ";
    WriteVec3Json(out, sun.shadow_split_distance);
    out << "},\n";
  } else {
    out << "  \"game_sun\": {\"captured\": false},\n";
  }
  out << "  \"composite\": {\"intensity\": " << g_rtgi_intensity << ", \"character_intensity\": " << g_rtgi_char_intensity
      << ", \"saturation\": " << g_rtgi_saturation
      << ", \"dispatched_frame\": " << f.dispatched_frame << ", \"applied_frame\": " << f.applied_frame
      << ", \"applied_intensity\": " << f.applied_intensity
      << ", \"subtract_previous\": " << (f.subtracted ? "true" : "false") << "},\n";
  out << "  \"extra_bounces\": {\"on\": " << (g_rtgi_extra_bounces > 0.5f ? "true" : "false")
      << ", \"method\": \"" << (g_rtgi_extra_bounce_method > 0.5f ? "real_extra_rays" : "feedback") << "\""
      << ", \"feedback_strength\": " << g_rtgi_feedback_strength
      << ", \"feedback_strength_effective\": " << RtgiFeedbackStrengthEffective()
      << ", \"rays\": " << RtgiExtraBounceRaysEffective()
      << ", \"note\": \"rays counts primary rays only; hit-class and distance counters count the first bounce; "
         "sun rays, lookups, invalid refs and stack overflow include extra bounces, so ratios can exceed 1 when "
         "extra rays are on\"},\n";
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
  RtgiDeviceData& data = found->second;
  const bool lights_copied = data.lights_copied;
  data.lights_copied = false;  // consumed by this read, so an old copy is never written with a new frame
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

  CameraSnapshot camera;
  {
    std::lock_guard<std::mutex> lock(g_state.mutex);
    camera = g_state.camera;
  }
  const RtgiJsonInput in = {
      .frame = &g_rtgi_frame,
      .values = g_rtgi_stats.values,
      .generated_frame = g_state.frame.load(),
      .perf_gather = bvh::g_perf_gate.gather.load(std::memory_order_relaxed),
      .camera = &camera,
  };
  std::string text = BuildRtgiJson(in);
  renodx::utils::path::WriteTextFile(bvh::PoolOutputDir() / "world_rtgi.json", text);

  if (!lights_copied) return;
  std::vector<uint8_t> light_bytes;
  std::vector<uint8_t> index_bytes;
  if (!renodx::utils::scene::ReadbackBuffer(device, queue, data.lights_copy, 0u, data.lights_copy_bytes, &light_bytes)
      || !renodx::utils::scene::ReadbackBuffer(device, queue, data.light_indices_copy, 0u, data.light_indices_copy_bytes,
                                               &index_bytes)) {
    renodx::utils::log::w("[world-rtgi] light buffer readback failed");
    return;
  }
  const size_t light_count = light_bytes.size() / sizeof(LightParamCpu);
  const size_t cell_count = index_bytes.size() / sizeof(LightIndexDataCpu);
  const auto* lights = reinterpret_cast<const LightParamCpu*>(light_bytes.data());
  const auto* cells = reinterpret_cast<const LightIndexDataCpu*>(index_bytes.data());
  // A light index is an index into dynamicLights_g for both the point and the spot lists.
  std::vector<uint8_t> in_point(light_count, 0u);
  std::vector<uint8_t> in_spot(light_count, 0u);
  uint32_t cells_with_lights = 0u;
  uint32_t point_refs = 0u;
  uint32_t spot_refs = 0u;
  uint32_t max_point_per_cell = 0u;
  uint32_t max_spot_per_cell = 0u;
  uint32_t index_out_of_range = 0u;
  for (size_t c = 0; c < cell_count; ++c) {
    const LightIndexDataCpu& cell = cells[c];
    const uint32_t point_n = (std::min)(cell.point_count, 63u);
    const uint32_t spot_n = (std::min)(cell.spot_count, 63u);
    if (point_n + spot_n > 0u) {
      ++cells_with_lights;
    }
    point_refs += point_n;
    spot_refs += spot_n;
    max_point_per_cell = (std::max)(max_point_per_cell, point_n);
    max_spot_per_cell = (std::max)(max_spot_per_cell, spot_n);
    for (uint32_t i = 0u; i < point_n; ++i) {
      const int32_t index = cell.point_indices[i];
      if (index < 0 || static_cast<size_t>(index) >= light_count) {
        ++index_out_of_range;
        continue;
      }
      in_point[index] = 1u;
    }
    for (uint32_t i = 0u; i < spot_n; ++i) {
      const int32_t index = cell.spot_indices[i];
      if (index < 0 || static_cast<size_t>(index) >= light_count) {
        ++index_out_of_range;
        continue;
      }
      in_spot[index] = 1u;
    }
  }
  uint32_t lights_radius_positive = 0u;
  uint32_t point_lights_in_cells = 0u;
  uint32_t spot_lights_in_cells = 0u;
  uint32_t radius_positive_in_no_cell = 0u;
  for (size_t l = 0; l < light_count; ++l) {
    point_lights_in_cells += in_point[l];
    spot_lights_in_cells += in_spot[l];
    if (lights[l].radius > 0.f) {
      ++lights_radius_positive;
      if (in_point[l] == 0u && in_spot[l] == 0u) {
        ++radius_positive_in_no_cell;
      }
    }
  }
  std::ostringstream lights_out;
  lights_out << "{\n";
  lights_out << "  \"schema\": 1,\n";
  lights_out << "  \"generated_frame\": " << in.generated_frame << ",\n";
  lights_out << "  \"light_buffer_bytes\": " << light_bytes.size() << ",\n";
  lights_out << "  \"light_index_buffer_bytes\": " << index_bytes.size() << ",\n";
  lights_out << "  \"lights\": " << light_count << ",\n";
  lights_out << "  \"lights_radius_positive\": " << lights_radius_positive << ",\n";
  lights_out << "  \"cells\": " << cell_count << ",\n";
  lights_out << "  \"cells_with_lights\": " << cells_with_lights << ",\n";
  lights_out << "  \"point_refs\": " << point_refs << ",\n";
  lights_out << "  \"spot_refs\": " << spot_refs << ",\n";
  lights_out << "  \"max_point_per_cell\": " << max_point_per_cell << ",\n";
  lights_out << "  \"max_spot_per_cell\": " << max_spot_per_cell << ",\n";
  lights_out << "  \"point_lights_in_cells\": " << point_lights_in_cells << ",\n";
  lights_out << "  \"spot_lights_in_cells\": " << spot_lights_in_cells << ",\n";
  lights_out << "  \"radius_positive_in_no_cell\": " << radius_positive_in_no_cell << ",\n";
  lights_out << "  \"index_out_of_range\": " << index_out_of_range << ",\n";
  lights_out << "  \"lights_list\": [\n";
  const size_t list_count = (std::min)(light_count, size_t{64});
  for (size_t l = 0; l < list_count; ++l) {
    const LightParamCpu& light = lights[l];
    const char* in_cells = in_point[l] != 0u ? (in_spot[l] != 0u ? "both" : "point") : (in_spot[l] != 0u ? "spot" : "none");
    lights_out << "    {\"index\": " << l << ", \"in_cells\": \"" << in_cells << "\", \"pos\": ";
    WriteVec3Json(lights_out, light.pos);
    lights_out << ", \"radius\": " << bvh::PoolJsonFloat{light.radius} << ", \"color\": ";
    WriteVec3Json(lights_out, light.color);
    lights_out << ", \"chara_color\": ";
    WriteVec3Json(lights_out, light.chara_color);
    lights_out << ", \"radius_inv\": " << bvh::PoolJsonFloat{light.radius_inv}
               << ", \"attenuation\": " << bvh::PoolJsonFloat{light.attenuation} << ", \"vec\": ";
    WriteVec3Json(lights_out, light.vec);
    lights_out << ", \"spot_angle_inv\": " << bvh::PoolJsonFloat{light.spot_angle_inv}
               << ", \"attenuation_angle\": " << bvh::PoolJsonFloat{light.attenuation_angle}
               << ", \"translucency\": " << bvh::PoolJsonFloat{light.translucency}
               << ", \"shadowmap_index\": " << light.shadowmap_index << "}" << (l + 1u < list_count ? ",\n" : "\n");
  }
  lights_out << "  ]\n}\n";
  renodx::utils::log::i("[world-rtgi] lights: ", light_count, " lights (", lights_radius_positive, " radius > 0), ", cell_count,
                        " cells (", cells_with_lights, " with lights), point refs ", point_refs, ", spot refs ", spot_refs,
                        ", out of range ", index_out_of_range);
  text = lights_out.str();
  renodx::utils::path::WriteTextFile(bvh::PoolOutputDir() / "world_rtgi_lights.json", text);
}

// Gathering was switched OFF: clears the RTGI GPU timers and the newest RTGI frame times (see bvh::ClearPerfGathered).
inline void ClearRtgiPerfGathered(reshade::api::device* device) {
  RtgiDeviceData& data = GetRtgiDeviceData(device);
  bvh::ClearGpuTimerStats(&data.timer);
  bvh::ClearGpuTimerStats(&data.denoise.timer);
  g_rtgi_frame.gpu_ms = -1.f;
  g_rtgi_frame.denoise_gpu_ms = -1.f;
}

}  // namespace falcom_world::rtgi
