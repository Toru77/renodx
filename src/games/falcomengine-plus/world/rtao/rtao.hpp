#pragma once

// RTAO dispatch: pass A (world_rtao.cs_5_0.hlsl, the trace) and, with Temporal on, pass B
// (world_rtao_temporal.cs_5_0.hlsl, the accumulation). Called inline from the lighting hook
// (addon.cpp RunRtaoInline), which owns the guards. No module mutex is held around any graphics call here.

#include <atomic>
#include <cstdint>
#include <cstring>
#include <sstream>
#include <vector>

#include "../bvh/bvh_trace.hpp"
#include "rtao_resources.hpp"
#include "rtao_state.hpp"

namespace falcom_world::rtao {

struct RtaoFrameState {
  Reason reason = Reason::Off;       // why AO is not produced this frame
  Reason logged_reason = Reason::Off;
  Reason note = Reason::Off;         // non-fatal note (isfast_unavailable: IGN used)
  bool producing = false;            // AO written this frame (rtao_active == 2)
  uint64_t dispatched_frame = UINT64_MAX;
  uint64_t frames_without_ao = 0u;   // RTAO requested but no AO this frame
  float gpu_ms = -1.f;               // newest completed RTAO dispatch (negative: none yet)
  uint64_t texture_bytes = 0u;       // AO target, stats buffer, and (Temporal) raw AO and history
  bool fade_valid = false;           // fade is computed from the built region
  Fade fade = {};
  // Round 2 (temporal).
  bool temporal_ran = false;         // pass B ran this frame
  bool temporal_last = false;        // Temporal was on in the last frame that ran the reset rule
  ResetReason reset = ResetReason::Disabled;       // this frame's reset decision (last one kept below)
  ResetReason last_reset = ResetReason::Disabled;
  uint64_t reset_counts[static_cast<size_t>(ResetReason::Count)] = {};
  bool prev_params_valid = false;
  ParameterSnapshot prev_params = {};
  bool motion_conflict = false;      // sticky for the session: TAA t3 and RTV4 are different resources
  MotionSource motion_source = MotionSource::None;
  uint64_t rcas_res = 0u;            // TAA t3 resource handle (0 = not captured)
  uint64_t rtv4_res = 0u;            // RTV4 motion resource handle (0 = not captured)
  uint64_t temporal_bytes = 0u;      // raw AO and two history targets
};

inline RtaoFrameState g_rtao_frame;

// Stats readback runs only when requested (panel "Read RTAO Stats", handled in OnWorldPresentBvh).
struct RtaoStatsState {
  std::atomic_bool requested{false};
  bool valid = false;
  uint32_t values[kRtaoStatsCount] = {};
};

inline RtaoStatsState g_rtao_stats;

struct RtaoDispatchInputs {
  reshade::api::resource_view depth_view = {0u};
  reshade::api::resource_view mrt_normal_view = {0u};
  reshade::api::resource_view isfast_view = {0u};
  reshade::api::resource_view scene_cbv_view = {0u};
  uint32_t width = 0u;
  uint32_t height = 0u;
  RtaoPushConstants push = {};
  // Temporal (pass B). Pass B runs only when temporal is set and its motion view and resources are available;
  // otherwise pass A writes the round-1 AO texel.
  bool temporal = false;
  reshade::api::resource_view motion_view = {0u};
  RtaoTemporalPushConstants temporal_push = {};  // params, debug mode and history valid set by the caller
};

struct RtaoDispatchResult {
  bool ok = false;           // pass A (and pass B when it ran) dispatched
  bool temporal_ran = false; // pass B ran; history was swapped
};

// NullComputeSlots: nulls the compute slots on the context with push_descriptors (pipeline_layout{0}),
// which binds directly at the register (table updates do not change the context).
inline void NullComputeSlots(reshade::api::command_list* cmd_list, uint32_t srv_count, uint32_t uav_count, uint32_t sampler_count) {
  const reshade::api::resource_view null_srvs[kRtaoSrvCount] = {};
  cmd_list->push_descriptors(
      reshade::api::shader_stage::all_compute, reshade::api::pipeline_layout{0}, 0,
      reshade::api::descriptor_table_update{
          {}, 0, 0, srv_count, reshade::api::descriptor_type::texture_shader_resource_view, null_srvs});
  const reshade::api::resource_view null_uavs[kRtaoUavCount] = {};
  cmd_list->push_descriptors(
      reshade::api::shader_stage::all_compute, reshade::api::pipeline_layout{0}, 0,
      reshade::api::descriptor_table_update{
          {}, 0, 0, uav_count, reshade::api::descriptor_type::texture_unordered_access_view, null_uavs});
  if (sampler_count == 0u) return;
  const reshade::api::sampler null_samplers[kRtaoTemporalSamplerCount] = {};
  cmd_list->push_descriptors(
      reshade::api::shader_stage::all_compute, reshade::api::pipeline_layout{0}, 0,
      reshade::api::descriptor_table_update{
          {}, 0, 0, sampler_count, reshade::api::descriptor_type::sampler, null_samplers});
}

// Binds, dispatches and unbinds. The AO and the history are written only by a pass that succeeded.
inline RtaoDispatchResult Dispatch(reshade::api::device* device, reshade::api::command_list* cmd_list, const RtaoDispatchInputs& in) {
  auto* bvh_data = bvh::GetBvhDeviceData(device);
  if (bvh_data == nullptr || in.width == 0u || in.height == 0u) return {};
  RtaoDeviceData& data = GetRtaoDeviceData(device);
  if (!EnsureRtaoPipeline(device, &data) || !EnsureRtaoTarget(device, &data, in.width, in.height)
      || !EnsureRtaoStats(device, &data)) {
    return {};
  }
  // Pass B only when its pipeline, targets and motion are available; otherwise pass A writes the round-1 AO.
  const bool temporal_run = in.temporal && in.motion_view.handle != 0u
      && EnsureRtaoTemporalPipeline(device, &data)
      && EnsureRtaoTemporalTargets(device, &data, in.width, in.height);

  RtaoPushConstants push = in.push;
  push.size[0] = static_cast<float>(in.width);
  push.size[1] = static_cast<float>(in.height);
  push.size[2] = 0.f;  // round 1: deforming objects are not traced (t10-t13 null)
  push.size[3] = temporal_run ? 1.f : 0.f;  // output_raw: pass A writes u2 for pass B

  // Whole-buffer write (offset 0, full size): see WriteBufferRange in bvh_resources.hpp.
  const uint32_t zeros[kRtaoStatsCount] = {};
  device->update_buffer_region(zeros, data.stats_buffer, 0u, sizeof(zeros));

  bvh::BeginGpuTimer(device, cmd_list, &data.timer);

  // Pass A.
  reshade::api::resource_view srvs[kRtaoSrvCount] = {};
  bvh::FillBvhSceneSrvs(*bvh_data, in.depth_view, bvh::DynamicTraceInputs{}, srvs);
  srvs[17] = in.mrt_normal_view;
  srvs[18] = in.isfast_view;
  const reshade::api::resource_view uavs[kRtaoUavCount] = {data.ao_uav, data.stats_uav, data.raw_uav};
  reshade::api::descriptor_table_update updates[3] = {
      {data.cbv_table, 0, 0, 1, reshade::api::descriptor_type::constant_buffer, &in.scene_cbv_view},
      {data.srv_table, 0, 0, kRtaoSrvCount, reshade::api::descriptor_type::shader_resource_view, srvs},
      {data.uav_table, 0, 0, kRtaoUavCount, reshade::api::descriptor_type::unordered_access_view, uavs},
  };
  device->update_descriptor_tables(3, updates);
  cmd_list->bind_pipeline(reshade::api::pipeline_stage::all_compute, data.pipeline);
  const reshade::api::descriptor_table tables[3] = {data.cbv_table, data.srv_table, data.uav_table};
  cmd_list->bind_descriptor_tables(reshade::api::shader_stage::all_compute, data.layout, 0, 3, tables);
  cmd_list->push_constants(
      reshade::api::shader_stage::all_compute, data.layout, 3, 0, kRtaoPushConstantCount, &push);
  cmd_list->dispatch((in.width + 7u) / 8u, (in.height + 7u) / 8u, 1u);
  NullComputeSlots(cmd_list, kRtaoSrvCount, kRtaoUavCount, 0u);

  if (temporal_run) {
    // Pass A wrote the raw AO through u2; pass B reads it through t0.
    cmd_list->barrier(data.raw_texture, reshade::api::resource_usage::unordered_access, reshade::api::resource_usage::shader_resource);

    // Pass B. History: read the current index, write the other one.
    const uint32_t read = data.history_index;
    const uint32_t write = data.history_index ^ 1u;
    const reshade::api::resource_view temporal_srvs[kRtaoTemporalSrvCount] = {
        data.raw_srv, in.depth_view, in.mrt_normal_view, in.motion_view, data.history_srv[read]};
    const reshade::api::resource_view temporal_uavs[kRtaoTemporalUavCount] = {
        data.ao_uav, data.stats_uav, data.history_uav[write]};
    const reshade::api::sampler samplers[kRtaoTemporalSamplerCount] = {data.point_sampler, data.linear_sampler};
    reshade::api::descriptor_table_update temporal_updates[4] = {
        {data.temporal_cbv_table, 0, 0, 1, reshade::api::descriptor_type::constant_buffer, &in.scene_cbv_view},
        {data.temporal_srv_table, 0, 0, kRtaoTemporalSrvCount, reshade::api::descriptor_type::shader_resource_view, temporal_srvs},
        {data.temporal_uav_table, 0, 0, kRtaoTemporalUavCount, reshade::api::descriptor_type::unordered_access_view, temporal_uavs},
        {data.temporal_sampler_table, 0, 0, kRtaoTemporalSamplerCount, reshade::api::descriptor_type::sampler, samplers},
    };
    device->update_descriptor_tables(4, temporal_updates);
    RtaoTemporalPushConstants tpush = in.temporal_push;
    tpush.texel[0] = 1.f / static_cast<float>(in.width);
    tpush.texel[1] = 1.f / static_cast<float>(in.height);
    tpush.texel[2] = 1.f;  // prevResolutionScale (1, 1)
    tpush.texel[3] = 1.f;
    tpush.size[0] = static_cast<float>(in.width);
    tpush.size[1] = static_cast<float>(in.height);
    cmd_list->bind_pipeline(reshade::api::pipeline_stage::all_compute, data.temporal_pipeline);
    const reshade::api::descriptor_table temporal_tables[4] = {
        data.temporal_cbv_table, data.temporal_srv_table, data.temporal_uav_table, data.temporal_sampler_table};
    cmd_list->bind_descriptor_tables(reshade::api::shader_stage::all_compute, data.temporal_layout, 0, 4, temporal_tables);
    cmd_list->push_constants(
        reshade::api::shader_stage::all_compute, data.temporal_layout, 4, 0, kRtaoPushConstantCount, &tpush);
    cmd_list->dispatch((in.width + 7u) / 8u, (in.height + 7u) / 8u, 1u);
    NullComputeSlots(cmd_list, kRtaoTemporalSrvCount, kRtaoTemporalUavCount, kRtaoTemporalSamplerCount);
    data.history_index = write;  // both passes succeeded: this history is now the one to read
  }
  bvh::EndGpuTimer(cmd_list, &data.timer);

  // App-side tables: null contents (ReShade keeps descriptors in the table object).
  const reshade::api::resource_view table_null_srvs[kRtaoSrvCount] = {};
  const reshade::api::resource_view table_null_uavs[kRtaoUavCount] = {};
  reshade::api::descriptor_table_update table_nulls[2] = {
      {data.srv_table, 0, 0, kRtaoSrvCount, reshade::api::descriptor_type::shader_resource_view, table_null_srvs},
      {data.uav_table, 0, 0, kRtaoUavCount, reshade::api::descriptor_type::unordered_access_view, table_null_uavs},
  };
  device->update_descriptor_tables(2, table_nulls);
  return {true, temporal_run};
}

// Reads the stats buffer and writes world_rtao.json. Called once per present; no lock held.
inline void MaybeCaptureRtaoStats(reshade::api::device* device, reshade::api::command_queue* queue) {
  if (!g_rtao_stats.requested.exchange(false, std::memory_order_relaxed)) return;
  const auto found = g_rtao_devices.find(device);
  if (found == g_rtao_devices.end() || found->second.stats_buffer.handle == 0u) return;
  const RtaoDeviceData& data = found->second;
  std::vector<uint8_t> bytes;
  if (!renodx::utils::scene::ReadbackBuffer(device, queue, data.stats_buffer, 0u, sizeof(g_rtao_stats.values), &bytes)
      || bytes.size() < sizeof(g_rtao_stats.values)) {
    renodx::utils::log::w("[world-rtao] stats readback failed");
    return;
  }
  std::memcpy(g_rtao_stats.values, bytes.data(), sizeof(g_rtao_stats.values));
  g_rtao_stats.valid = true;
  const uint32_t* v = g_rtao_stats.values;
  const float occluded = v[kRtaoStatRays] > 0u ? 100.f * v[kRtaoStatHits] / v[kRtaoStatRays] : 0.f;
  const float mean_ao = v[kRtaoStatPixels] > 0u ? static_cast<float>(v[kRtaoStatAoSum]) / (255.f * v[kRtaoStatPixels]) : 1.f;
  renodx::utils::log::i("[world-rtao] stats: rays=", v[kRtaoStatRays], " hits=", v[kRtaoStatHits],
                        " occluded_pct=", occluded, " mean_ao=", mean_ao, " pixels=", v[kRtaoStatPixels],
                        " sky=", v[kRtaoStatSky], " normal=", v[kRtaoStatNormal], " region=", v[kRtaoStatRegion],
                        " scaled=", v[kRtaoStatScaled], " invalid_refs=", v[kRtaoStatInvalidRefs],
                        " stack_overflow=", v[kRtaoStatStackOverflow], " gpu_ms=", g_rtao_frame.gpu_ms);

  std::ostringstream out;
  out << "{\n  \"schema\": 2,\n  \"generated_frame\": " << g_state.frame.load()
      << ",\n  \"reason\": \"" << ReasonName(g_rtao_frame.reason) << "\""
      << ",\n  \"producing\": " << (g_rtao_frame.producing ? "true" : "false")
      << ",\n  \"gpu_ms\": " << g_rtao_frame.gpu_ms
      << ",\n  \"texture_bytes\": " << g_rtao_frame.texture_bytes
      << ",\n  \"frames_without_ao\": " << g_rtao_frame.frames_without_ao
      << ",\n  \"fade\": {\"start_eff\": " << g_rtao_frame.fade.start << ", \"end_eff\": " << g_rtao_frame.fade.end
      << ", \"coverage\": " << g_rtao_frame.fade.coverage << "}"
      << ",\n  \"stats\": {\"rays\": " << v[kRtaoStatRays] << ", \"hits\": " << v[kRtaoStatHits]
      << ", \"pixels\": " << v[kRtaoStatPixels] << ", \"ao_sum\": " << v[kRtaoStatAoSum]
      << ", \"sky\": " << v[kRtaoStatSky] << ", \"normal\": " << v[kRtaoStatNormal]
      << ", \"region\": " << v[kRtaoStatRegion] << ", \"scaled\": " << v[kRtaoStatScaled]
      << ", \"invalid_refs\": " << v[kRtaoStatInvalidRefs]
      << ", \"stack_overflow\": " << v[kRtaoStatStackOverflow] << "}"
      << ",\n  \"temporal\": {\"on\": " << (g_rtao_temporal_enabled > 0.5f ? "true" : "false")
      << ", \"ran\": " << (g_rtao_frame.temporal_ran ? "true" : "false")
      << ", \"reset\": \"" << ResetReasonName(g_rtao_frame.reset) << "\""
      << ", \"last_reset\": \"" << ResetReasonName(g_rtao_frame.last_reset) << "\""
      << ", \"motion\": \"" << (g_rtao_frame.motion_source == MotionSource::Rtv4 ? "rtv4" : g_rtao_frame.motion_source == MotionSource::Conflict ? "conflict" : "none") << "\""
      << ", \"rcas_motion_res\": " << g_rtao_frame.rcas_res << ", \"mb_rtv4_res\": " << g_rtao_frame.rtv4_res
      << ", \"temporal_bytes\": " << g_rtao_frame.temporal_bytes << ", \"history_bytes_per_pixel\": 18"
      << ", \"temporal_pixels\": " << v[kRtaoStatTemporalBase + 0u]
      << ", \"valid_taps\": " << v[kRtaoStatTemporalBase + 1u]
      << ", \"rejected_depth\": " << v[kRtaoStatTemporalBase + 2u]
      << ", \"rejected_normal\": " << v[kRtaoStatTemporalBase + 3u]
      << ", \"out_of_bounds\": " << v[kRtaoStatTemporalBase + 4u]
      << ", \"reset_pixels\": " << v[kRtaoStatTemporalBase + 5u]
      << ", \"reset_counts\": [";
  for (size_t r = 0; r < static_cast<size_t>(ResetReason::Count); ++r) {
    out << (r == 0u ? "" : ", ") << g_rtao_frame.reset_counts[r];
  }
  out << "]"
      << ",\n  \"two_sided_discovery\": {\"cpu\": [";
  bool first_cpu = true;
  for (uint32_t cull = 0u; cull < 4u; ++cull) {
    for (uint32_t ccw = 0u; ccw < 2u; ++ccw) {
      const uint32_t draws = g_rtao_cull_hist[cull][ccw].load(std::memory_order_relaxed);
      if (draws == 0u) continue;
      out << (first_cpu ? "" : ", ") << "{\"cull\": " << cull << ", \"front_ccw\": " << ccw << ", \"draws\": " << draws << "}";
      first_cpu = false;
    }
  }
  static const char* const kDiscoveryNames[kRtaoStatDiscoveryCount] = {
      "front_away_opaque", "front_away_alpha", "front_toward_opaque", "front_toward_alpha",
      "back_away_opaque", "back_away_alpha", "back_toward_opaque", "back_toward_alpha"};
  out << "], \"gpu\": {";
  for (uint32_t i = 0u; i < kRtaoStatDiscoveryCount; ++i) {
    out << (i == 0u ? "" : ", ") << "\"" << kDiscoveryNames[i] << "\": " << v[kRtaoStatDiscoveryBase + i];
  }
  out << "}}"
      << ",\n  \"note\": \"dynamic objects not traced (round 3); scaled pixels are counted by the shader\"\n}\n";
  std::string text = out.str();
  renodx::utils::path::WriteTextFile(bvh::PoolOutputDir() / "world_rtao.json", text);
}

}  // namespace falcom_world::rtao
