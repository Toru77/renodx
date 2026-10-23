#pragma once

// RT Local Shadows dispatch: the trace (world_rtls.cs_5_0.hlsl) and the stats readback. Called inline from the lighting
// hook (addon.cpp RunRtlsInline), which owns the guards. No module mutex is held around any graphics call here.

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
#include "../rtgi/rtgi.hpp"
#include "../world_state.hpp"
#include "rtls_resources.hpp"
#include "rtls_state.hpp"

namespace falcom_world::rtls {

struct RtlsFrameState {
  Reason reason = Reason::Off;
  rtao::Reason input_reason = rtao::Reason::Off;  // the failing guard when reason is InputsMissing
  bool producing = false;
  bool debug_written = false;
  bool isfast_used = false;
  bool isfast_unavailable = false;
  uint64_t dispatched_frame = UINT64_MAX;
  float gpu_ms = -1.f;
  uint32_t trace_width = 0u;
  uint32_t trace_height = 0u;
  uint32_t divisor = 1u;
  float ray_max_requested = 0.f;
  float ray_max_effective = 0.f;
  bool fade_valid = false;
  rtao::Fade fade = {};
  uint32_t lamp_count = 0u;
  uint32_t kinds = 0u;
  uint32_t character_objects = 0u;
  uint64_t texture_bytes = 0u;
  float denoise_gpu_ms = -1.f;        // temporal, filter and upsample (negative: none yet)
  bool temporal_ran = false;
  bool temporal_last = false;         // Temporal was on and ran in the last frame that ran the reset rule
  bool filter_ran = false;
  rtao::ResetReason reset = rtao::ResetReason::Disabled;
  rtao::ResetReason last_reset = rtao::ResetReason::Disabled;
  uint64_t reset_counts[static_cast<size_t>(rtao::ResetReason::Count)] = {};
  bool prev_params_valid = false;
  ParameterSnapshot prev_params = {};
};
inline RtlsFrameState g_rtls_frame;
struct RtlsStatsState { std::atomic_bool requested{false}; bool valid = false; uint32_t values[kRtlsStatsCount] = {}; };
inline RtlsStatsState g_rtls_stats;

struct RtlsDispatchInputs {
  reshade::api::resource_view depth_view = {0u};
  reshade::api::resource_view mrt_normal_view = {0u};
  reshade::api::resource_view isfast_view = {0u};
  reshade::api::resource_view scene_cbv_view = {0u};
  reshade::api::resource_view lights_view = {0u};
  reshade::api::resource_view light_indices_view = {0u};
  uint32_t full_width = 0u;
  uint32_t full_height = 0u;
  uint32_t trace_width = 0u;
  uint32_t trace_height = 0u;
  RtlsPushConstants push = {};      // params[3] is the trace divisor
  int debug_mode = 0;               // RTLS Debug: 1-5 trace views, 6-8 denoise views
  bvh::DynamicTraceInputs dynamic = {};
  reshade::api::resource_view motion_view = {0u};  // null: the temporal pass does not run
  bool temporal = false;
  rtgi::RtgiTemporalPushConstants temporal_push = {};  // params and size[3] (history valid) set by the caller
  bool spatial = false;
  int filter_type = 1;
  int filter_radius = 2;
  int filter_passes = 1;
  int filter_taps = 5;
  float filter_value_stop = 0.f;   // filter c2.w: penumbra edge stop k
};
struct RtlsDispatchResult { bool ok = false; bool debug_written = false; bool temporal_ran = false; bool filter_ran = false; };

inline RtlsDispatchResult Dispatch(reshade::api::device* device, reshade::api::command_list* cmd_list, const RtlsDispatchInputs& in) {
  const bvh::PoolCpuTimer cpu_timer{&bvh::g_perf_gate.rtls_record_us};
  auto* bvh_data = bvh::GetBvhDeviceData(device);
  RtlsDeviceData& data = GetRtlsDeviceData(device);
  if (bvh_data == nullptr || in.full_width == 0u || in.full_height == 0u || in.trace_width == 0u || in.trace_height == 0u) return {};
  if (!rtgi::EnsureRtgiPipeline(device, &data.trace_pipeline, {.srv_count = kRtlsSrvCount, .uav_count = kRtlsUavCount, .sampler_count = 0u,
                                                               .push_count = kRtlsPushConstantCount, .code = kRtlsTraceShader, .name = "rtls_trace"})
      || !rtgi::EnsureRtgiPipeline(device, &data.denoise.upsample_pipeline, rtgi::kRtgiUpsamplePipelineDesc)) {
    return {};
  }
  if (data.stats_buffer.handle == 0u) {
    const uint32_t zeros[kRtlsStatsCount] = {};
    if (!bvh::CreatePoolBuffer(
            device, zeros, sizeof(zeros), sizeof(uint32_t),
            &data.stats_buffer, &data.stats_uav,
            reshade::api::resource_usage::unordered_access | reshade::api::resource_usage::copy_source)) {
      return {};
    }
  }
  bool debug_ready = in.debug_mode >= 1 && in.debug_mode <= 8;
  if (debug_ready && !rtgi::EnsureRtgiTextureSet(device, &data.debug, reshade::api::format::r8g8b8a8_unorm, 4u, in.full_width, in.full_height)) {
    renodx::utils::log::w("[world-rtls] debug texture creation failed");
    debug_ready = false;
  }
  const bool trace_debug = debug_ready && in.debug_mode <= 5;
  if (!rtgi::EnsureRtgiTextureSet(device, &data.raw, reshade::api::format::r16g16b16a16_float, 8u, in.trace_width, in.trace_height)) {
    renodx::utils::log::w("[world-rtls] raw texture creation failed");
    return {};
  }
  const rtgi::RtgiDenoiseReady denoise = rtgi::EnsureRtgiDenoise(device, &data.denoise,
                                                                 {.output_format = reshade::api::format::r8g8b8a8_unorm,
                                                                  .output_bytes_per_pixel = 4u,
                                                                  .trace_width = in.trace_width,
                                                                  .trace_height = in.trace_height,
                                                                  .full_width = in.full_width,
                                                                  .full_height = in.full_height,
                                                                  .temporal = in.temporal && in.motion_view.handle != 0u,
                                                                  .spatial = in.spatial,
                                                                  .filter_code = kRtlsFilterShader,
                                                                  .log_prefix = "[world-rtls]"});
  if (!denoise.ok) return {};

  RtlsPushConstants push = in.push;
  push.size[0] = static_cast<float>(in.trace_width);
  push.size[1] = static_cast<float>(in.trace_height);
  push.size[2] = static_cast<float>(in.full_width);
  push.size[3] = static_cast<float>(in.full_height);
  std::memcpy(&push.fade[3], &bvh_data->dynamic.instance_count, sizeof(uint32_t));  // P2b records (t19)
  push.extra[0] = static_cast<float>(in.dynamic.count);
  push.extra[1] = g_rtls_character_normal_bias;

  // Whole-buffer write (offset 0, full size): see WriteBufferRange in bvh_resources.hpp.
  const uint32_t zeros[kRtlsStatsCount] = {};
  device->update_buffer_region(zeros, data.stats_buffer, 0u, sizeof(zeros));

  bvh::BeginGpuTimer(device, cmd_list, &data.timer);
  const rtgi::RtgiPipeline& trace = data.trace_pipeline;
  reshade::api::resource_view srvs[kRtlsSrvCount] = {};
  bvh::FillBvhSceneSrvs(*bvh_data, in.depth_view, in.dynamic, srvs);
  srvs[17] = in.mrt_normal_view;
  srvs[18] = in.isfast_view;
  srvs[19] = bvh_data->dynamic.instances_srv;
  srvs[20] = in.lights_view;
  srvs[21] = in.light_indices_view;
  const reshade::api::resource_view uavs[kRtlsUavCount] = {
      trace_debug ? data.debug.uav[0] : reshade::api::resource_view{0u}, data.stats_uav, data.raw.uav[0]};
  const reshade::api::descriptor_type kCbv = reshade::api::descriptor_type::constant_buffer;
  const reshade::api::descriptor_type kSrv = reshade::api::descriptor_type::shader_resource_view;
  const reshade::api::descriptor_type kUav = reshade::api::descriptor_type::unordered_access_view;
  reshade::api::descriptor_table_update updates[3] = {
      {trace.cbv_table, 0, 0, 1, kCbv, &in.scene_cbv_view},
      {trace.srv_table, 0, 0, kRtlsSrvCount, kSrv, srvs},
      {trace.uav_table, 0, 0, kRtlsUavCount, kUav, uavs},
  };
  device->update_descriptor_tables(3, updates);
  cmd_list->bind_pipeline(reshade::api::pipeline_stage::all_compute, trace.pipeline);
  const reshade::api::descriptor_table tables[3] = {trace.cbv_table, trace.srv_table, trace.uav_table};
  cmd_list->bind_descriptor_tables(reshade::api::shader_stage::all_compute, trace.layout, 0, 3, tables);
  cmd_list->push_constants(reshade::api::shader_stage::all_compute, trace.layout, 3, 0, kRtlsPushConstantCount, &push);
  cmd_list->dispatch((in.trace_width + 7u) / 8u, (in.trace_height + 7u) / 8u, 1u);
  rtao::NullComputeSlots(cmd_list, kRtlsSrvCount, kRtlsUavCount, 0u);
  if (trace_debug) {
    cmd_list->barrier(data.debug.texture[0], reshade::api::resource_usage::unordered_access, reshade::api::resource_usage::shader_resource);
  }
  cmd_list->barrier(data.raw.texture[0], reshade::api::resource_usage::unordered_access, reshade::api::resource_usage::shader_resource);
  bvh::EndGpuTimer(cmd_list, &data.timer);

  // RTLS Debug 6 to 8 are the chain's views 4 (accumulated), 5 (history confidence) and 6 (final, after the upsample).
  int denoise_debug = 0;
  switch (in.debug_mode) {
    case 6: denoise_debug = 4; break;
    case 7: denoise_debug = 5; break;
    case 8: denoise_debug = 6; break;
    default: break;
  }
  const reshade::api::resource_view table_null_views[kRtlsSrvCount] = {};
  std::vector<reshade::api::descriptor_table_update> table_nulls = {
      {trace.srv_table, 0, 0, kRtlsSrvCount, kSrv, table_null_views},
      {trace.uav_table, 0, 0, kRtlsUavCount, kUav, table_null_views},
  };
  const rtgi::RtgiDenoiseResult denoised = rtgi::DispatchRtgiDenoise(
      device, cmd_list, &data.denoise,
      {.raw_view = data.raw.srv[0],
       .depth_view = in.depth_view,
       .mrt_normal_view = in.mrt_normal_view,
       .scene_cbv_view = in.scene_cbv_view,
       .motion_view = in.motion_view,
       .albedo_view = reshade::api::resource_view{0u},
       .stats_uav = data.stats_uav,
       .debug_uav = debug_ready && denoise_debug != 0 ? data.debug.uav[0] : reshade::api::resource_view{0u},
       .debug_texture = data.debug.texture[0],
       .trace_width = in.trace_width,
       .trace_height = in.trace_height,
       .full_width = in.full_width,
       .full_height = in.full_height,
       .divisor = in.push.params[3],
       .debug_mode = denoise_debug,
       .temporal_push = in.temporal_push,
       .filter_type = in.filter_type,
       .filter_radius = in.filter_radius,
       .filter_passes = in.filter_passes,
       .filter_taps = in.filter_taps,
       .filter_value_stop = in.filter_value_stop,
       .upsample_intensity = 1.f,
       .upsample_char_intensity = 1.f,
       .upsample_saturation = 1.f},
      denoise, &table_nulls);
  // App-side tables: null contents (ReShade keeps descriptors in the table object). The CBV tables keep their view.
  device->update_descriptor_tables(static_cast<uint32_t>(table_nulls.size()), table_nulls.data());
  return {.ok = true,
          .debug_written = trace_debug || denoised.temporal_debug || denoised.upsample_debug,
          .temporal_ran = denoise.temporal,
          .filter_ran = denoise.spatial};
}

struct RtlsJsonInput {
  const RtlsFrameState* frame;
  const uint32_t* values;
  uint64_t generated_frame;
};

inline std::string BuildRtlsJson(const RtlsJsonInput& in) {
  const RtlsFrameState& f = *in.frame;
  const uint32_t* v = in.values;
  std::ostringstream out;
  out << "{\n";
  out << "  \"schema\": 1,\n";
  out << "  \"generated_frame\": " << in.generated_frame << ",\n";
  out << "  \"reason\": \"" << ReasonName(f.reason) << "\",\n";
  out << "  \"input_reason\": \"" << rtao::ReasonName(f.input_reason) << "\",\n";
  out << "  \"producing\": " << (f.producing ? "true" : "false") << ",\n";
  out << "  \"gpu_ms\": " << f.gpu_ms << ",\n";
  out << "  \"trace\": {\"divisor\": " << f.divisor << ", \"width\": " << f.trace_width << ", \"height\": " << f.trace_height << "},\n";
  out << "  \"denoise_gpu_ms\": " << f.denoise_gpu_ms << ",\n";
  out << "  \"temporal\": {\"on\": " << (g_rtls_temporal_enabled > 0.5f ? "true" : "false")
      << ", \"ran\": " << (f.temporal_ran ? "true" : "false")
      << ", \"reset\": \"" << rtao::ResetReasonName(f.reset) << "\""
      << ", \"last_reset\": \"" << rtao::ResetReasonName(f.last_reset) << "\", \"reset_counts\": [";
  for (size_t r = 0; r < static_cast<size_t>(rtao::ResetReason::Count); ++r) {
    out << (r == 0u ? "" : ", ") << f.reset_counts[r];
  }
  out << "]},\n";
  out << "  \"spatial_filter\": {\"on\": " << (g_rtls_spatial_enabled > 0.5f ? "true" : "false")
      << ", \"ran\": " << (f.filter_ran ? "true" : "false") << ", \"type\": " << g_rtls_filter_type
      << ", \"radius\": " << g_rtls_filter_radius << ", \"passes\": " << g_rtls_filter_passes
      << ", \"taps\": " << (g_rtls_filter_type > 0.5f ? g_rtls_filter_taps_atrous : g_rtls_filter_taps_separable)
      << ", \"value_stop\": " << g_rtls_filter_value_stop << "},\n";
  out << "  \"ray_max\": {\"requested\": " << f.ray_max_requested << ", \"effective\": " << f.ray_max_effective << "},\n";
  out << "  \"fade\": {\"start_eff\": " << f.fade.start << ", \"end_eff\": " << f.fade.end
      << ", \"coverage\": " << f.fade.coverage << ", \"requested_start\": " << f.fade.start_req
      << ", \"requested_end\": " << f.fade.end_req << "},\n";
  out << "  \"lamp_count\": " << f.lamp_count << ",\n";
  out << "  \"kinds\": " << f.kinds << ",\n";
  out << "  \"isfast_used\": " << (f.isfast_used ? "true" : "false") << ",\n";
  out << "  \"character_objects\": " << f.character_objects << ",\n";
  out << "  \"settings\": {\"samples\": " << g_rtls_samples << ", \"max_point_lights\": " << g_rtls_max_point_lights
      << ", \"light_size\": " << g_rtls_light_size << ", \"light_clearance\": " << g_rtls_light_clearance
      << ", \"normal_bias\": " << g_rtls_normal_bias << ", \"trace_characters\": " << g_rtls_trace_characters
      << ", \"point\": " << g_rtls_point << ", \"spot\": " << g_rtls_spot << ", \"env_point\": " << g_rtls_env_point
      << ", \"env_spot\": " << g_rtls_env_spot
      << ", \"trace_resolution\": " << g_rtls_trace_resolution
      << ", \"characters\": " << g_rtls_characters << ", \"character_lamp\": " << g_rtls_character_lamp
      << ", \"character_normal_bias\": " << g_rtls_character_normal_bias << "},\n";
  out << "  \"stats\": {\"pixels\": " << v[kRtlsStatPixels] << ", \"traced\": " << v[kRtlsStatTraced]
      << ", \"sky\": " << v[kRtlsStatSky] << ", \"normal\": " << v[kRtlsStatNormal]
      << ", \"region\": " << v[kRtlsStatRegion] << ", \"beyond_fade\": " << v[kRtlsStatBeyondFade]
      << ", \"scaled\": " << v[kRtlsStatScaled] << ", \"character\": " << v[kRtlsStatCharacter]
      << ", \"lights_considered\": " << v[kRtlsStatLightsConsidered] << ", \"lights_traced\": " << v[kRtlsStatLightsTraced]
      << ", \"points_over_budget\": " << v[kRtlsStatPointsOverBudget] << ", \"spots_over_cap\": " << v[kRtlsStatSpotsOverCap]
      << ", \"lights_backlit\": " << v[kRtlsStatLightsBacklit] << ", \"rays\": " << v[kRtlsStatRays]
      << ", \"rays_blocked\": " << v[kRtlsStatRaysBlocked] << ", \"rays_near_light\": " << v[kRtlsStatRaysNearLight]
      << ", \"rays_clipped\": " << v[kRtlsStatRaysClipped] << ", \"invalid_refs\": " << v[kRtlsStatInvalidRefs]
      << ", \"stack_overflow\": " << v[kRtlsStatStackOverflow] << ", \"index_out_of_range\": " << v[kRtlsStatIndexOutOfRange]
      << ", \"character_traced\": " << v[kRtlsStatCharacterTraced] << ", \"character_self_blocked\": " << v[kRtlsStatCharacterSelfBlocked] << ", \"mixed_pixels\": " << v[kRtlsStatMixedPixels]
      << ", \"mixed_pixels_near_blocked\": " << v[kRtlsStatMixedPixelsNear] << ", \"mixed_pixels_no_near\": " << v[kRtlsStatMixedPixelsNoNear]
      << ", \"multi_lamp_pixels\": " << v[kRtlsStatMultiLampPixels]
      << ", \"error_energy\": " << v[kRtlsStatErrorEnergy] << ", \"lit_total\": " << v[kRtlsStatLitTotal]
      << ", \"temporal_pixels\": " << v[rtgi::kRtgiStatTemporalPixels] << ", \"no_history\": " << v[rtgi::kRtgiStatNoHistory]
      << ", \"reset_pixels\": " << v[rtgi::kRtgiStatResetPixels]
      << "},\n";
  out << "  \"per_light\": [";
  bool first = true;
  for (uint32_t i = 0u; i < kRtlsPerLightCount; ++i) {
    const uint32_t rays = v[kRtlsStatPerLightRays + i];
    if (rays == 0u) continue;
    const uint32_t near_blocked = v[kRtlsStatPerLightNear + i];
    out << (first ? "\n" : ",\n") << "    {\"index\": " << i << ", \"rays\": " << rays << ", \"near_blocked\": " << near_blocked
        << ", \"near_blocked_share\": " << rtao::SharePercent(near_blocked, rays) << "}";
    first = false;
  }
  out << (first ? "]\n" : "\n  ]\n");
  out << "}\n";
  return out.str();
}

inline void MaybeCaptureRtlsStats(reshade::api::device* device, reshade::api::command_queue* queue) {
  if (!g_rtls_stats.requested.exchange(false, std::memory_order_relaxed)) return;
  const auto found = g_rtls_devices.find(device);
  if (found == g_rtls_devices.end() || found->second.stats_buffer.handle == 0u) return;
  RtlsDeviceData& data = found->second;
  std::vector<uint8_t> bytes;
  if (!renodx::utils::scene::ReadbackBuffer(device, queue, data.stats_buffer, 0u, sizeof(g_rtls_stats.values), &bytes)
      || bytes.size() < sizeof(g_rtls_stats.values)) {
    renodx::utils::log::w("[world-rtls] stats readback failed");
    return;
  }
  std::memcpy(g_rtls_stats.values, bytes.data(), sizeof(g_rtls_stats.values));
  g_rtls_stats.valid = true;
  const uint32_t* v = g_rtls_stats.values;
  renodx::utils::log::i("[world-rtls] stats: pixels=", v[kRtlsStatPixels], " traced=", v[kRtlsStatTraced],
                        " lights=", v[kRtlsStatLightsConsidered], " traced_lights=", v[kRtlsStatLightsTraced],
                        " over_budget=", v[kRtlsStatPointsOverBudget], " rays=", v[kRtlsStatRays],
                        " blocked=", v[kRtlsStatRaysBlocked], " near_light=", v[kRtlsStatRaysNearLight],
                        " invalid_refs=", v[kRtlsStatInvalidRefs], " stack_overflow=", v[kRtlsStatStackOverflow],
                        " gpu_ms=", g_rtls_frame.gpu_ms);
  std::string text = BuildRtlsJson({.frame = &g_rtls_frame, .values = g_rtls_stats.values, .generated_frame = g_state.frame.load()});
  renodx::utils::path::WriteTextFile(bvh::PoolOutputDir() / "world_rtls.json", text);
}

}  // namespace falcom_world::rtls
