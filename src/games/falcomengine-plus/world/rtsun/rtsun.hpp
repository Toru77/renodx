#pragma once

// RT Sun Shadows dispatch: the trace (world_rtsun.cs_5_0.hlsl) and the stats readback. Called inline from the lighting
// hook (addon.cpp RunRtSunInline), which owns the guards. No module mutex is held around any graphics call here.

#include <atomic>
#include <cstdint>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>

#include "../bvh/bvh_trace.hpp"
#include "../rtao/rtao.hpp"
#include "../rtgi/rtgi.hpp"
#include "../world_state.hpp"
#include "rtsun_range.hpp"
#include "rtsun_resources.hpp"
#include "rtsun_state.hpp"

namespace falcom_world::rtsun {

struct RtSunFrameState {
  Reason reason = Reason::Off;
  rtao::Reason input_reason = rtao::Reason::Off;  // the failing guard when reason is InputsMissing
  bool producing = false;
  bool debug_written = false;
  bool isfast_used = false;
  bool isfast_unavailable = false;
  uint64_t dispatched_frame = UINT64_MAX;
  float gpu_ms = -1.f;
  uint32_t width = 0u;
  uint32_t height = 0u;
  int cascades_requested = 0;
  int cascades_effective = 0;
  int cascades_camera = 0;
  float covered_m = 0.f;       // outer boundary of the effective cascades (m)
  float built_range = 0.f;     // BVH region range (m) the effective count was limited by
  uint32_t character_objects = 0u;
  uint64_t texture_bytes = 0u;
  bool cpu_has_scaling = false;        // gate: camera snapshot of the lighting constants (set in RunRtSunInline)
  float cpu_scaling[2] = {};
  float cpu_camera_x = 0.f;
  uint32_t cached_w = 0u;              // gate: depth size the trace was sized from (captured_depth_w/h)
  uint32_t cached_h = 0u;
  uint32_t actual_w = 0u;              // gate: the captured depth texture's size
  uint32_t actual_h = 0u;
  uint32_t divisor = 1u;               // trace divisor (1 Full, 2 Half, 4 Quarter); width and height are the trace size
  float denoise_gpu_ms = -1.f;         // temporal, filter and upsample (negative: none yet)
  bool temporal_ran = false;
  bool temporal_last = false;          // Temporal was on and ran in the last frame that ran the reset rule
  bool filter_ran = false;
  rtao::ResetReason reset = rtao::ResetReason::Disabled;
  rtao::ResetReason last_reset = rtao::ResetReason::Disabled;
  uint64_t reset_counts[static_cast<size_t>(rtao::ResetReason::Count)] = {};
  bool prev_params_valid = false;
  ParameterSnapshot prev_params = {};
};
inline RtSunFrameState g_rtsun_frame;
struct RtSunStatsState { std::atomic_bool requested{false}; bool valid = false; uint32_t values[kRtSunStatsCount] = {}; };
inline RtSunStatsState g_rtsun_stats;

struct RtSunDispatchInputs {
  reshade::api::resource_view depth_view = {0u};
  reshade::api::resource_view mrt_normal_view = {0u};
  reshade::api::resource_view isfast_view = {0u};
  reshade::api::resource_view scene_cbv_view = {0u};
  uint32_t full_width = 0u;
  uint32_t full_height = 0u;
  uint32_t trace_width = 0u;
  uint32_t trace_height = 0u;
  RtSunPushConstants push = {};     // sun[3] is the trace divisor
  int debug_mode = 0;               // RT Sun Debug: 1 raw shadow, 2 pixel classes, 4 to 6 denoise views
  reshade::api::resource_view motion_view = {0u};  // null: the temporal pass does not run
  bool temporal = false;
  rtgi::RtgiTemporalPushConstants temporal_push = {};  // params and size[3] (history valid) set by the caller
  bool spatial = false;
  int filter_type = 1;
  int filter_radius = 2;
  int filter_passes = 1;
  int filter_taps = 5;
  float filter_value_stop = 0.f;   // filter c2.w: penumbra edge stop k
  bvh::DynamicTraceInputs dynamic = {};
};
struct RtSunDispatchResult { bool ok = false; bool debug_written = false; bool temporal_ran = false; bool filter_ran = false; };

inline RtSunDispatchResult Dispatch(reshade::api::device* device, reshade::api::command_list* cmd_list, const RtSunDispatchInputs& in) {
  const bvh::PoolCpuTimer cpu_timer{&bvh::g_perf_gate.rtsun_record_us};
  auto* bvh_data = bvh::GetBvhDeviceData(device);
  RtSunDeviceData& data = GetRtSunDeviceData(device);
  if (bvh_data == nullptr || in.full_width == 0u || in.full_height == 0u || in.trace_width == 0u || in.trace_height == 0u) return {};
  if (!rtgi::EnsureRtgiPipeline(device, &data.trace_pipeline, {.srv_count = kRtSunSrvCount, .uav_count = kRtSunUavCount, .sampler_count = 0u,
                                                               .push_count = kRtSunPushConstantCount, .code = kRtSunTraceShader, .name = "rtsun_trace"})
      || !rtgi::EnsureRtgiPipeline(device, &data.denoise.upsample_pipeline, rtgi::kRtgiUpsamplePipelineDesc)) {
    return {};
  }
  if (data.stats_buffer.handle == 0u) {
    const uint32_t zeros[kRtSunStatsCount] = {};
    if (!bvh::CreatePoolBuffer(
            device, zeros, sizeof(zeros), sizeof(uint32_t),
            &data.stats_buffer, &data.stats_uav,
            reshade::api::resource_usage::unordered_access | reshade::api::resource_usage::copy_source)) {
      return {};
    }
  }
  bool debug_ready = (in.debug_mode >= 1 && in.debug_mode <= 2) || (in.debug_mode >= 4 && in.debug_mode <= 6);
  if (debug_ready && !rtgi::EnsureRtgiTextureSet(device, &data.debug, reshade::api::format::r8g8b8a8_unorm, 4u, in.full_width, in.full_height)) {
    renodx::utils::log::w("[world-rtsun] debug texture creation failed");
    debug_ready = false;
  }
  const bool trace_debug = debug_ready && in.debug_mode <= 2;
  if (!rtgi::EnsureRtgiTextureSet(device, &data.raw, reshade::api::format::r16g16b16a16_float, 8u, in.trace_width, in.trace_height)) {
    renodx::utils::log::w("[world-rtsun] raw texture creation failed");
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
                                                                  .filter_code = rtls::kRtlsFilterShader,
                                                                  .log_prefix = "[world-rtsun]"});
  if (!denoise.ok) return {};

  RtSunPushConstants push = in.push;
  push.size[0] = static_cast<float>(in.full_width);
  push.size[1] = static_cast<float>(in.full_height);
  std::memcpy(&push.sun[2], &bvh_data->dynamic.instance_count, sizeof(uint32_t));  // P2b records (t19)
  push.sun[1] = static_cast<float>(in.dynamic.count);

  // Whole-buffer write (offset 0, full size): see WriteBufferRange in bvh_resources.hpp.
  const uint32_t zeros[kRtSunStatsCount] = {};
  device->update_buffer_region(zeros, data.stats_buffer, 0u, sizeof(zeros));

  bvh::BeginGpuTimer(device, cmd_list, &data.timer);
  const rtgi::RtgiPipeline& trace = data.trace_pipeline;
  reshade::api::resource_view srvs[kRtSunSrvCount] = {};
  bvh::FillBvhSceneSrvs(*bvh_data, in.depth_view, in.dynamic, srvs);
  srvs[17] = in.mrt_normal_view;
  srvs[18] = in.isfast_view;
  srvs[19] = bvh_data->dynamic.instances_srv;
  const reshade::api::resource_view uavs[kRtSunUavCount] = {
      trace_debug ? data.debug.uav[0] : reshade::api::resource_view{0u}, data.stats_uav, data.raw.uav[0]};
  const reshade::api::descriptor_type kCbv = reshade::api::descriptor_type::constant_buffer;
  const reshade::api::descriptor_type kSrv = reshade::api::descriptor_type::shader_resource_view;
  const reshade::api::descriptor_type kUav = reshade::api::descriptor_type::unordered_access_view;
  const reshade::api::buffer_range scene_cbv = rtao::SceneCbvRange(in.scene_cbv_view);
  reshade::api::descriptor_table_update updates[3] = {
      {trace.cbv_table, 0, 0, 1, kCbv, &scene_cbv},
      {trace.srv_table, 0, 0, kRtSunSrvCount, kSrv, srvs},
      {trace.uav_table, 0, 0, kRtSunUavCount, kUav, uavs},
  };
  device->update_descriptor_tables(3, updates);
  cmd_list->bind_pipeline(reshade::api::pipeline_stage::all_compute, trace.pipeline);
  const reshade::api::descriptor_table tables[3] = {trace.cbv_table, trace.srv_table, trace.uav_table};
  cmd_list->bind_descriptor_tables(reshade::api::shader_stage::all_compute, trace.layout, 0, 3, tables);
  cmd_list->push_constants(reshade::api::shader_stage::all_compute, trace.layout, 3, 0, kRtSunPushConstantCount, &push);
  cmd_list->dispatch((in.trace_width + 7u) / 8u, (in.trace_height + 7u) / 8u, 1u);
  rtao::NullComputeSlots(cmd_list, kRtSunSrvCount, kRtSunUavCount, 0u);
  if (trace_debug) {
    cmd_list->barrier(data.debug.texture[0], reshade::api::resource_usage::unordered_access, reshade::api::resource_usage::shader_resource);
  }
  cmd_list->barrier(data.raw.texture[0], reshade::api::resource_usage::unordered_access, reshade::api::resource_usage::shader_resource);
  bvh::EndGpuTimer(cmd_list, &data.timer);

  // RT Sun Debug 4 to 6 are the chain's views 4 (accumulated), 5 (history confidence) and 6 (final, after the upsample).
  const int denoise_debug = in.debug_mode >= 4 && in.debug_mode <= 6 ? in.debug_mode : 0;
  const reshade::api::resource_view table_null_views[kRtSunSrvCount] = {};
  std::vector<reshade::api::descriptor_table_update> table_nulls = {
      {trace.srv_table, 0, 0, kRtSunSrvCount, kSrv, table_null_views},
      {trace.uav_table, 0, 0, kRtSunUavCount, kUav, table_null_views},
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
       .divisor = in.push.sun[3],
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

struct RtSunJsonInput {
  const RtSunFrameState* frame;
  const uint32_t* values;
  uint64_t generated_frame;
};

inline std::string BuildRtSunJson(const RtSunJsonInput& in) {
  const RtSunFrameState& f = *in.frame;
  const uint32_t* v = in.values;
  std::ostringstream out;
  out << "{\n";
  out << "  \"schema\": 1,\n";
  out << "  \"generated_frame\": " << in.generated_frame << ",\n";
  out << "  \"reason\": \"" << ReasonName(f.reason) << "\",\n";
  out << "  \"input_reason\": \"" << rtao::ReasonName(f.input_reason) << "\",\n";
  out << "  \"producing\": " << (f.producing ? "true" : "false") << ",\n";
  out << "  \"gpu_ms\": " << f.gpu_ms << ",\n";
  out << "  \"trace\": {\"divisor\": " << f.divisor << ", \"width\": " << f.width << ", \"height\": " << f.height << "},\n";
  out << "  \"denoise_gpu_ms\": " << f.denoise_gpu_ms << ",\n";
  out << "  \"temporal\": {\"on\": " << (g_rtsun_temporal_enabled > 0.5f ? "true" : "false")
      << ", \"ran\": " << (f.temporal_ran ? "true" : "false")
      << ", \"reset\": \"" << rtao::ResetReasonName(f.reset) << "\""
      << ", \"last_reset\": \"" << rtao::ResetReasonName(f.last_reset) << "\", \"reset_counts\": [";
  for (size_t r = 0; r < static_cast<size_t>(rtao::ResetReason::Count); ++r) {
    out << (r == 0u ? "" : ", ") << f.reset_counts[r];
  }
  out << "]},\n";
  out << "  \"spatial_filter\": {\"on\": " << (g_rtsun_spatial_enabled > 0.5f ? "true" : "false")
      << ", \"ran\": " << (f.filter_ran ? "true" : "false") << ", \"type\": " << g_rtsun_filter_type
      << ", \"radius\": " << g_rtsun_filter_radius << ", \"passes\": " << g_rtsun_filter_passes
      << ", \"taps\": " << (g_rtsun_filter_type > 0.5f ? g_rtsun_filter_taps_atrous : g_rtsun_filter_taps_separable)
      << ", \"value_stop\": " << g_rtsun_filter_value_stop << "},\n";
  out << "  \"cascades\": {\"requested\": " << f.cascades_requested << ", \"effective\": " << f.cascades_effective
      << ", \"camera\": " << f.cascades_camera << ", \"covered_m\": " << f.covered_m
      << ", \"built_range_m\": " << f.built_range << "},\n";
  out << "  \"isfast_used\": " << (f.isfast_used ? "true" : "false") << ",\n";
  out << "  \"character_objects\": " << f.character_objects << ",\n";
  out << "  \"settings\": {\"debug\": " << g_rtsun_debug << ", \"cascade_range\": " << g_rtsun_cascade_range
      << ", \"trace_characters\": " << g_rtsun_trace_characters << ", \"samples\": " << g_rtsun_samples
      << ", \"max_ray_m\": " << g_rtsun_ray_max << ", \"light_size_deg\": " << g_rtsun_light_size
      << ", \"normal_bias_m\": " << g_rtsun_normal_bias << ", \"isfast\": " << g_rtsun_isfast
      << ", \"trace_resolution\": " << g_rtsun_trace_resolution << ", \"foliage_intensity\": " << g_rtsun_foliage_intensity << "},\n";
  out << "  \"stats\": {\"unit\": \"per trace pixel\", \"pixels\": " << v[kRtSunStatPixels] << ", \"traced\": " << v[kRtSunStatTraced]
      << ", \"sky\": " << v[kRtSunStatSky] << ", \"normal\": " << v[kRtSunStatNormal]
      << ", \"region\": " << v[kRtSunStatRegion] << ", \"beyond_range\": " << v[kRtSunStatBeyondRange]
      << ", \"scaled\": " << v[kRtSunStatScaled] << ", \"character\": " << v[kRtSunStatCharacter]
      << ", \"rays\": " << v[kRtSunStatRays] << ", \"rays_blocked\": " << v[kRtSunStatRaysBlocked]
      << ", \"character_rays\": " << v[kRtSunStatCharacterRays] << ", \"self_blocked\": " << v[kRtSunStatSelfBlocked]
      << ", \"characters_traced\": " << v[kRtSunStatCharactersTraced] << ", \"characters_self_blocked\": " << v[kRtSunStatCharactersSelfBlocked]
      << ", \"invalid_refs\": " << v[kRtSunStatInvalidRefs] << ", \"stack_overflow\": " << v[kRtSunStatStackOverflow]
      << ", \"temporal_pixels\": " << v[rtgi::kRtgiStatTemporalPixels] << ", \"temporal_no_history\": " << v[rtgi::kRtgiStatNoHistory]
      << ", \"temporal_reset_pixels\": " << v[rtgi::kRtgiStatResetPixels]
      << ", \"foliage_first_hits\": " << v[kRtSunStatFoliageFirstHits] << ", \"foliage_opaque_behind\": " << v[kRtSunStatFoliageOpaqueBehind]
      << "},\n";
  float gpu_scaling[2] = {};
  std::memcpy(gpu_scaling, &v[kRtSunStatGateScalingX], sizeof(gpu_scaling));
  int32_t gpu_cascade_count = 0;
  std::memcpy(&gpu_cascade_count, &v[kRtSunStatGateCascadeCount], sizeof(gpu_cascade_count));
  float gpu_camera_x = 0.f;
  std::memcpy(&gpu_camera_x, &v[kRtSunStatGateCameraX], sizeof(gpu_camera_x));
  out << "  \"gate\": {\n";
  out << "    \"reason\": \"" << (f.producing ? "ok" : ReasonName(f.reason)) << "\",\n";
  out << "    \"latch\": " << g_state.rt_sun_cascades.load(std::memory_order_relaxed) << ",\n";
  out << "    \"cascades_effective\": " << f.cascades_effective << ",\n";
  out << "    \"gpu_scene\": {\"scaling_x\": " << gpu_scaling[0] << ", \"scaling_y\": " << gpu_scaling[1]
      << ", \"cascade_count\": " << gpu_cascade_count << ", \"camera_x\": " << gpu_camera_x << "},\n";
  out << "    \"cpu_scene\": {\"has_scaling\": " << (f.cpu_has_scaling ? "true" : "false") << ", \"scaling_x\": " << f.cpu_scaling[0]
      << ", \"scaling_y\": " << f.cpu_scaling[1] << ", \"camera_x\": " << f.cpu_camera_x << "},\n";
  out << "    \"depth\": {\"cached_w\": " << f.cached_w << ", \"cached_h\": " << f.cached_h << ", \"actual_w\": " << f.actual_w
      << ", \"actual_h\": " << f.actual_h << "},\n";
  out << "    \"dispatched_frame\": " << f.dispatched_frame << ",\n";
  out << "    \"generated_frame\": " << in.generated_frame << "\n";
  out << "  },\n";
  out << "  \"texture_bytes\": " << f.texture_bytes << "\n";
  out << "}\n";
  return out.str();
}

inline void MaybeCaptureRtSunStats(reshade::api::device* device, reshade::api::command_queue* queue) {
  if (!g_rtsun_stats.requested.exchange(false, std::memory_order_relaxed)) return;
  const auto found = g_rtsun_devices.find(device);
  if (found == g_rtsun_devices.end() || found->second.stats_buffer.handle == 0u) return;
  RtSunDeviceData& data = found->second;
  std::vector<uint8_t> bytes;
  if (!renodx::utils::scene::ReadbackBuffer(device, queue, data.stats_buffer, 0u, sizeof(g_rtsun_stats.values), &bytes)
      || bytes.size() < sizeof(g_rtsun_stats.values)) {
    renodx::utils::log::w("[world-rtsun] stats readback failed");
    return;
  }
  std::memcpy(g_rtsun_stats.values, bytes.data(), sizeof(g_rtsun_stats.values));
  g_rtsun_stats.valid = true;
  const uint32_t* v = g_rtsun_stats.values;
  renodx::utils::log::i("[world-rtsun] stats (per trace pixel): pixels=", v[kRtSunStatPixels], " traced=", v[kRtSunStatTraced],
                        " beyond_range=", v[kRtSunStatBeyondRange], " rays=", v[kRtSunStatRays],
                        " blocked=", v[kRtSunStatRaysBlocked], " character_rays=", v[kRtSunStatCharacterRays],
                        " self_blocked=", v[kRtSunStatSelfBlocked], " invalid_refs=", v[kRtSunStatInvalidRefs],
                        " stack_overflow=", v[kRtSunStatStackOverflow], " gpu_ms=", g_rtsun_frame.gpu_ms);
  std::string text = BuildRtSunJson({.frame = &g_rtsun_frame, .values = g_rtsun_stats.values, .generated_frame = g_state.frame.load()});
  renodx::utils::path::WriteTextFile(bvh::PoolOutputDir() / "world_rtsun.json", text);
}

}  // namespace falcom_world::rtsun
