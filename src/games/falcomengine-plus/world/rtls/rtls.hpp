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
#include "../rtgi/rtgi_resources.hpp"
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
  float ray_max_requested = 0.f;
  float ray_max_effective = 0.f;
  bool fade_valid = false;
  rtao::Fade fade = {};
  uint32_t lamp_count = 0u;
  uint32_t kinds = 0u;
  uint32_t character_objects = 0u;
  uint64_t texture_bytes = 0u;
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
  RtlsPushConstants push = {};
  int debug_mode = 0;
  bvh::DynamicTraceInputs dynamic = {};
};
struct RtlsDispatchResult { bool ok = false; bool debug_written = false; };

inline RtlsDispatchResult Dispatch(reshade::api::device* device, reshade::api::command_list* cmd_list, const RtlsDispatchInputs& in) {
  auto* bvh_data = bvh::GetBvhDeviceData(device);
  RtlsDeviceData& data = GetRtlsDeviceData(device);
  if (bvh_data == nullptr || in.full_width == 0u || in.full_height == 0u) return {};
  if (!rtgi::EnsureRtgiPipeline(device, &data.trace_pipeline, {.srv_count = kRtlsSrvCount, .uav_count = kRtlsUavCount, .sampler_count = 0u,
                                                               .push_count = kRtlsPushConstantCount, .code = kRtlsTraceShader, .name = "rtls_trace"})) {
    return {};
  }
  const uint32_t zeros[kRtlsStatsCount] = {};
  if (data.stats_buffer.handle == 0u) {
    if (!bvh::CreatePoolBuffer(
            device, zeros, sizeof(zeros), sizeof(uint32_t),
            &data.stats_buffer, &data.stats_uav,
            reshade::api::resource_usage::unordered_access | reshade::api::resource_usage::copy_source)) {
      return {};
    }
  }
  bool debug_ready = in.debug_mode >= 1 && in.debug_mode <= 5;
  if (debug_ready && !rtgi::EnsureRtgiTextureSet(device, &data.debug, reshade::api::format::r8g8b8a8_unorm, 4u, in.full_width, in.full_height)) {
    renodx::utils::log::w("[world-rtls] debug texture creation failed");
    debug_ready = false;
  }
  if (!rtgi::EnsureRtgiTextureSet(device, &data.raw, reshade::api::format::r16g16b16a16_float, 8u, in.full_width, in.full_height)) {
    renodx::utils::log::w("[world-rtls] raw texture creation failed");
    return {};
  }

  RtlsPushConstants push = in.push;
  push.size[0] = static_cast<float>(in.full_width);
  push.size[1] = static_cast<float>(in.full_height);
  push.size[2] = static_cast<float>(in.full_width);
  push.size[3] = static_cast<float>(in.full_height);
  push.params[3] = 1.f;
  std::memcpy(&push.fade[3], &bvh_data->dynamic.instance_count, sizeof(uint32_t));  // P2b records (t19)
  push.extra[0] = static_cast<float>(in.dynamic.count);

  // Whole-buffer write (offset 0, full size): see WriteBufferRange in bvh_resources.hpp.
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
      debug_ready ? data.debug.uav[0] : reshade::api::resource_view{0u}, data.stats_uav, data.raw.uav[0]};
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
  cmd_list->dispatch((in.full_width + 7u) / 8u, (in.full_height + 7u) / 8u, 1u);
  rtao::NullComputeSlots(cmd_list, kRtlsSrvCount, kRtlsUavCount, 0u);
  if (debug_ready) {
    cmd_list->barrier(data.debug.texture[0], reshade::api::resource_usage::unordered_access, reshade::api::resource_usage::shader_resource);
  }
  cmd_list->barrier(data.raw.texture[0], reshade::api::resource_usage::unordered_access, reshade::api::resource_usage::shader_resource);
  bvh::EndGpuTimer(cmd_list, &data.timer);

  // App-side tables: null contents (ReShade keeps descriptors in the table object). The CBV table keeps its view.
  const reshade::api::resource_view table_null_views[kRtlsSrvCount] = {};
  const reshade::api::descriptor_table_update table_nulls[2] = {
      {trace.srv_table, 0, 0, kRtlsSrvCount, kSrv, table_null_views},
      {trace.uav_table, 0, 0, kRtlsUavCount, kUav, table_null_views},
  };
  device->update_descriptor_tables(2, table_nulls);
  return {.ok = true, .debug_written = debug_ready};
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
  out << "  \"trace\": {\"width\": " << f.trace_width << ", \"height\": " << f.trace_height << "},\n";
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
      << ", \"env_spot\": " << g_rtls_env_spot << "},\n";
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
