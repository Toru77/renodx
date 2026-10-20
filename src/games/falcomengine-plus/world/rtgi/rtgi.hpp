#pragma once

// RTGI dispatch (M1: the diagnostic trace) and the stats readback. Called inline from the lighting hook (addon.cpp
// RunRtgiInline), which owns the guards. No module mutex is held around any graphics call here.

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
  uint32_t full_width = 0u;
  uint32_t full_height = 0u;
  uint32_t trace_width = 0u;
  uint32_t trace_height = 0u;
  RtgiPushConstants push = {};
  int debug_mode = 0;
};

struct RtgiDispatchResult {
  bool ok = false;
  bool debug_written = false;
};

inline RtgiDispatchResult Dispatch(reshade::api::device* device, reshade::api::command_list* cmd_list, const RtgiDispatchInputs& in) {
  const bvh::PoolCpuTimer cpu_timer{&bvh::g_perf_gate.rtgi_record_us};
  auto* bvh_data = bvh::GetBvhDeviceData(device);
  if (bvh_data == nullptr || in.trace_width == 0u || in.trace_height == 0u) return {};
  RtgiDeviceData& data = GetRtgiDeviceData(device);
  if (data.pipeline.handle == 0u && !data.pipeline_failed) {
    using DR = reshade::api::descriptor_range;
    using DS = reshade::api::shader_stage;
    using DT = reshade::api::descriptor_type;
    using P = reshade::api::pipeline_layout_param;
    const auto fail = [&](const char* stage) {
      renodx::utils::log::w("[world-rtgi] pipeline creation failed: ", stage);
      data.pipeline_failed = true;
    };
    if (kRtgiTraceShader.empty()) {
      fail("shader not embedded");
      return {};
    }
    DR cbv_range = {0, 0, 0, 1, DS::all_compute, 1, DT::constant_buffer};
    DR srv_range = {0, 0, 0, kRtgiSrvCount, DS::all_compute, 1, DT::shader_resource_view};
    DR uav_range = {0, 0, 0, kRtgiUavCount, DS::all_compute, 1, DT::unordered_access_view};
    reshade::api::constant_range push_range = {};
    push_range.binding = 0;
    push_range.dx_register_index = rtao::kRtaoPushRegister;
    push_range.dx_register_space = 0;
    push_range.count = kRtgiPushConstantCount;
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
    if (data.layout.handle == 0u && !device->create_pipeline_layout(4, params, &data.layout)) {
      fail("pipeline layout");
      return {};
    }
    if (data.cbv_table.handle == 0u && !device->allocate_descriptor_table(data.layout, 0, &data.cbv_table)) {
      fail("cbv table");
      return {};
    }
    if (data.srv_table.handle == 0u && !device->allocate_descriptor_table(data.layout, 1, &data.srv_table)) {
      fail("srv table");
      return {};
    }
    if (data.uav_table.handle == 0u && !device->allocate_descriptor_table(data.layout, 2, &data.uav_table)) {
      fail("uav table");
      return {};
    }
    reshade::api::shader_desc shader = {};
    shader.code = kRtgiTraceShader.data();
    shader.code_size = kRtgiTraceShader.size();
    shader.entry_point = "main";
    reshade::api::pipeline_subobject subobject = {
        reshade::api::pipeline_subobject_type::compute_shader, 1, &shader};
    if (!device->create_pipeline(data.layout, 1, &subobject, &data.pipeline)) {
      fail("compute pipeline");
      return {};
    }
    renodx::utils::log::i("[world-rtgi] pipeline ready");
  }
  if (data.pipeline.handle == 0u) return {};
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

  RtgiPushConstants push = in.push;
  push.size[0] = static_cast<float>(in.trace_width);
  push.size[1] = static_cast<float>(in.trace_height);
  push.size[2] = static_cast<float>(in.full_width);
  push.size[3] = static_cast<float>(in.full_height);
  std::memcpy(&push.fade[3], &bvh_data->dynamic.instance_count, sizeof(uint32_t));  // P2b records (t19)

  // Whole-buffer write (offset 0, full size): see WriteBufferRange in bvh_resources.hpp.
  const uint32_t zeros[kRtgiStatsCount] = {};
  device->update_buffer_region(zeros, data.stats_buffer, 0u, sizeof(zeros));

  bvh::BeginGpuTimer(device, cmd_list, &data.timer);

  reshade::api::resource_view srvs[kRtgiSrvCount] = {};
  bvh::FillBvhSceneSrvs(*bvh_data, in.depth_view, bvh::DynamicTraceInputs{}, srvs);
  srvs[17] = in.mrt_normal_view;
  srvs[18] = in.isfast_view;
  srvs[19] = bvh_data->dynamic.instances_srv;
  srvs[20] = in.radiance_view;
  const reshade::api::resource_view uavs[kRtgiUavCount] = {
      trace_debug ? data.debug.uav[0] : reshade::api::resource_view{0u}, data.stats_uav, data.raw.uav[0]};
  reshade::api::descriptor_table_update updates[3] = {
      {data.cbv_table, 0, 0, 1, reshade::api::descriptor_type::constant_buffer, &in.scene_cbv_view},
      {data.srv_table, 0, 0, kRtgiSrvCount, reshade::api::descriptor_type::shader_resource_view, srvs},
      {data.uav_table, 0, 0, kRtgiUavCount, reshade::api::descriptor_type::unordered_access_view, uavs},
  };
  device->update_descriptor_tables(3, updates);
  cmd_list->bind_pipeline(reshade::api::pipeline_stage::all_compute, data.pipeline);
  const reshade::api::descriptor_table tables[3] = {data.cbv_table, data.srv_table, data.uav_table};
  cmd_list->bind_descriptor_tables(reshade::api::shader_stage::all_compute, data.layout, 0, 3, tables);
  cmd_list->push_constants(reshade::api::shader_stage::all_compute, data.layout, 3, 0, kRtgiPushConstantCount, &push);
  cmd_list->dispatch((in.trace_width + 7u) / 8u, (in.trace_height + 7u) / 8u, 1u);
  rtao::NullComputeSlots(cmd_list, kRtgiSrvCount, kRtgiUavCount, 0u);

  if (trace_debug) {
    cmd_list->barrier(data.debug.texture[0], reshade::api::resource_usage::unordered_access, reshade::api::resource_usage::shader_resource);
  }
  cmd_list->barrier(data.raw.texture[0], reshade::api::resource_usage::unordered_access, reshade::api::resource_usage::shader_resource);
  bvh::EndGpuTimer(cmd_list, &data.timer);

  // App-side tables: null contents (ReShade keeps descriptors in the table object). The CBV table keeps its view.
  const reshade::api::resource_view table_null_srvs[kRtgiSrvCount] = {};
  const reshade::api::resource_view table_null_uavs[kRtgiUavCount] = {};
  reshade::api::descriptor_table_update table_nulls[2] = {
      {data.srv_table, 0, 0, kRtgiSrvCount, reshade::api::descriptor_type::shader_resource_view, table_null_srvs},
      {data.uav_table, 0, 0, kRtgiUavCount, reshade::api::descriptor_type::unordered_access_view, table_null_uavs},
  };
  device->update_descriptor_tables(2, table_nulls);
  return {.ok = true, .debug_written = trace_debug};
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
  out << "  \"isfast_used\": " << (f.isfast_used ? "true" : "false") << ",\n";
  out << "  \"prev_view_proj_register\": " << f.prev_view_proj_register << ",\n";
  out << "  \"texture_bytes\": " << f.texture_bytes << ",\n";
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
  out << "    \"clamped\": " << v[kRtgiStatClamped] << "\n";
  out << "  },\n";
  out << "  \"note\": \"M2 diagnostic: raw GI is computed but not added to the image\"\n";
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
  const std::string text = BuildRtgiJson(in);
  renodx::utils::path::WriteTextFile(bvh::PoolOutputDir() / "world_rtgi.json", text);
}

// Gathering was switched OFF: clears the RTGI GPU timer and the newest RTGI frame time (see bvh::ClearPerfGathered).
inline void ClearRtgiPerfGathered(reshade::api::device* device) {
  bvh::ClearGpuTimerStats(&GetRtgiDeviceData(device).timer);
  g_rtgi_frame.gpu_ms = -1.f;
}

}  // namespace falcom_world::rtgi
