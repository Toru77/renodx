#pragma once

#include "../debug/crash_log.hpp"

// RTAO dispatch: pass A (world_rtao.cs_5_0.hlsl, the trace) writes the trace-size raw AO. The SVGF chain that denoises it and
// upsamples it to full size is in rtao_svgf.hpp. Called inline from the lighting hook (addon.cpp RunRtaoInline), which owns
// the guards. No module mutex is held around any graphics call here.

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
  Reason reason = Reason::Off;  // why AO is not produced this frame
  Reason logged_reason = Reason::Off;
  Reason note = Reason::Off;  // non-fatal note (isfast_unavailable: IGN used)
  bool producing = false;     // AO written this frame (rtao_active == 2)
  uint64_t dispatched_frame = UINT64_MAX;
  uint64_t frames_without_ao = 0u;  // RTAO requested but no AO this frame
  float gpu_ms = -1.f;              // newest completed RTAO dispatch (negative: none yet)
  float denoise_gpu_ms = -1.f;      // newest completed SVGF chain and upsample (negative: none yet)
  bool filter_ran = false;          // the SVGF spatial filter ran this frame
  uint32_t svgf_iterations = 0u;    // a-trous iterations of the newest dispatch
  bool svgf_stats = false;          // the SVGF counters (stats 20..22) were requested at the newest dispatch
  uint64_t texture_bytes = 0u;      // trace AO, stats buffer, SVGF raw, history and output
  bool fade_valid = false;          // fade is computed from the built region
  Fade fade = {};
  // Round 2 (temporal).
  bool temporal_ran = false;                  // the SVGF temporal reprojection ran this frame
  bool temporal_last = false;                 // Temporal was on in the last frame that ran the reset rule
  ResetReason reset = ResetReason::Disabled;  // this frame's reset decision (last one kept below)
  ResetReason last_reset = ResetReason::Disabled;
  uint64_t reset_counts[static_cast<size_t>(ResetReason::Count)] = {};
  bool prev_params_valid = false;
  ParameterSnapshot prev_params = {};
  bool motion_conflict = false;  // sticky for the session: TAA t3 and RTV4 are different resources
  MotionSource motion_source = MotionSource::None;
  uint64_t rcas_res = 0u;                                   // TAA t3 resource handle (0 = not captured)
  uint64_t rtv4_res = 0u;                                   // RTV4 motion resource handle (0 = not captured)
  uint64_t frames_without_ao_by_reason[kReasonCount] = {};  // frames without AO, per Reason code (F1)
  const char* last_parameter_change = "";                   // first field that changed on the last Parameters reset (F1)
  bool motion_dims_ok = true;                               // RTV4 size equals the depth size (texel units)
  uint32_t motion_w = 0u, motion_h = 0u;
  uint32_t depth_w = 0u, depth_h = 0u;
  uint64_t temporal_on_frames = 0u;  // frames with Temporal on (TAA t3 never-captured check)
  ParameterSnapshot captured = {};   // parameters of the last successful dispatch (P0)
  bool captured_isfast_used = false;
  uint32_t p2b_records = 0u;        // P2b records the newest dispatch traced (fade.w)
  uint32_t character_objects = 0u;  // character objects the newest dispatch traced (size.z)
  uint32_t trace_divisor = 1u;      // Trace Resolution of the newest dispatch: block side d (1, 2 or 4)
  uint32_t trace_width = 0u;        // pass A extent of the newest dispatch (ceil(width / d))
  uint32_t trace_height = 0u;
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
  bvh::DynamicTraceInputs dynamic = {};  // characters (RefitDeformLiveCharacters), count 0 when off
  uint32_t trace_divisor = 1u;           // Trace Resolution: block side d (1 Full, 2 Half, 4 Quarter)
  uint32_t scene_characters = 0u;
};

struct RtaoDispatchResult {
  bool ok = false;            // pass A dispatched
  bool pool_reduced = false;  // G5: no AO: the DXR pass fell back while the CPU pool is reduced
  bool dxr_unavailable = false;
};

// Null-array sizes: upper bounds of the SRV, UAV and sampler counts of every pass that calls NullComputeSlots (RTAO, RTGI and
// RTLS). 8 is the D3D11 compute UAV slot count.
inline constexpr uint32_t kNullComputeSrvMax = 32u;
inline constexpr uint32_t kNullComputeUavMax = 8u;
inline constexpr uint32_t kNullComputeSamplerMax = 2u;
// NullComputeSlots: nulls the compute slots on the context with push_descriptors (pipeline_layout{0}),
// which binds directly at the register (table updates do not change the context).
inline void NullComputeSlots(reshade::api::command_list* cmd_list, uint32_t srv_count, uint32_t uav_count, uint32_t sampler_count) {
  const reshade::api::resource_view null_srvs[kNullComputeSrvMax] = {};
  cmd_list->push_descriptors(
      reshade::api::shader_stage::all_compute, reshade::api::pipeline_layout{0}, 0,
      reshade::api::descriptor_table_update{
          {}, 0, 0, srv_count, reshade::api::descriptor_type::texture_shader_resource_view, null_srvs});
  const reshade::api::resource_view null_uavs[kNullComputeUavMax] = {};
  cmd_list->push_descriptors(
      reshade::api::shader_stage::all_compute, reshade::api::pipeline_layout{0}, 0,
      reshade::api::descriptor_table_update{
          {}, 0, 0, uav_count, reshade::api::descriptor_type::texture_unordered_access_view, null_uavs});
  if (sampler_count == 0u) return;
  const reshade::api::sampler null_samplers[kNullComputeSamplerMax] = {};
  cmd_list->push_descriptors(
      reshade::api::shader_stage::all_compute, reshade::api::pipeline_layout{0}, 0,
      reshade::api::descriptor_table_update{
          {}, 0, 0, sampler_count, reshade::api::descriptor_type::sampler, null_samplers});
}

#if !defined(FALCOM_WINDOWS_HARNESS)
}  // namespace falcom_world::rtao
#include "../rt12/rt12_backend.hpp"
#include "../rt12/rt12_rtxdi.hpp"
#include "../rt12/rt12_rtxdi_transfer.hpp"
namespace falcom_world::rtao {
#endif

// Binds, dispatches and unbinds pass A. The trace-size raw AO (u2) is written only by a pass that succeeded; the SVGF input pass
// reads it. Pass A always writes the trace extent (every Trace Resolution), and u0 stays null.
inline RtaoDispatchResult Dispatch(reshade::api::device* device, reshade::api::command_list* cmd_list, const RtaoDispatchInputs& in) {
  const bvh::PoolCpuTimer cpu_timer{&bvh::g_perf_gate.rtao_record_us};
  // Set before any early return, so a skipped dispatch cannot leave the previous trace size in the panel or json.
  const uint32_t d = in.trace_divisor;
  const uint32_t trace_w = RtgiTraceExtent(in.width, d);
  const uint32_t trace_h = RtgiTraceExtent(in.height, d);
  g_rtao_frame.trace_divisor = d;
  g_rtao_frame.trace_width = trace_w;
  g_rtao_frame.trace_height = trace_h;
  auto* bvh_data = bvh::GetBvhDeviceData(device);
  if (bvh_data == nullptr || in.width == 0u || in.height == 0u) return {};
  RtaoDeviceData& data = GetRtaoDeviceData(device);
#if defined(FALCOM_WINDOWS_HARNESS)
  if (!EnsureRtaoPipeline(device, &data)) return {};
#else
  if (!EnsureRtaoLayout(device, &data)) return {};
#endif
  if (!EnsureRtaoStats(device, &data) || !EnsureRtaoTraceTarget(device, &data, trace_w, trace_h)) return {};

  RtaoPushConstants push = in.push;
  push.size[0] = static_cast<float>(in.width);
  push.size[1] = static_cast<float>(in.height);
  push.size[2] = static_cast<float>(in.dynamic.count);
  push.size[3] = 1.f;  // output_raw: pass A writes u2 (the trace-size raw AO) for the SVGF input pass
  push.trace[0] = static_cast<float>(d);
  std::memcpy(&push.fade[3], &bvh_data->dynamic.instance_count, sizeof(uint32_t));  // P2b records (t19)
  g_rtao_frame.p2b_records = bvh_data->dynamic.instance_count;
  g_rtao_frame.character_objects = in.dynamic.count;

  // Whole-buffer write (offset 0, full size): see WriteBufferRange in bvh_resources.hpp.
  const uint32_t zeros[kRtaoStatsCount] = {};
  device->update_buffer_region(zeros, data.stats_buffer, 0u, sizeof(zeros));

  bvh::BeginGpuTimer(device, cmd_list, &data.timer);

  // Pass A: on D3D12 (rt12, DXR backend switch) when it can run this frame, otherwise the compute pass.
  const reshade::api::resource_view uavs[kRtaoUavCount] = {
      reshade::api::resource_view{0u}, data.stats_uav, data.trace_uav};
#if !defined(FALCOM_WINDOWS_HARNESS)
  const rt12::SceneAoInputs ao = {
      .push = push,
      .depth_view = in.depth_view,
      .mrt_normal_view = in.mrt_normal_view,
      .isfast_view = in.isfast_view,
      .uavs = {uavs[0], uavs[1], uavs[2]},
      .trace_w = trace_w,
      .trace_h = trace_h,
  };
  const bool dxr_pass_a = rt12::RunSceneTransaction(
                              device, cmd_list,
                              {.scene_cbv_view = in.scene_cbv_view,
                               .records = bvh_data->dynamic.instance_count,
                               .characters = in.scene_characters,
                               .character_snapshot = bvh::DeformLightingSnapshot(g_state.frame.load()),
                               .frame = g_state.frame.load(),
                               .generation = rt12::g_rt12 ? rt12::g_rt12->generation : 0u},
                              &ao)
                              .ao_produced;
#else
  const bool dxr_pass_a = false;
#endif
#if !defined(FALCOM_WINDOWS_HARNESS)
  if (!dxr_pass_a) {
    const bool pool_reduced = bvh::g_gpu_capture.pool_reduced.load(std::memory_order_relaxed);
    if (pool_reduced) bvh::g_gpu_capture.reduced_fallback.store(true, std::memory_order_relaxed);
    bvh::EndGpuTimer(cmd_list, &data.timer);
    return {.ok = false, .pool_reduced = pool_reduced, .dxr_unavailable = true};
  }
#endif
  if (!dxr_pass_a && bvh::g_gpu_capture.pool_reduced.load(std::memory_order_relaxed)) {
    // G5 (decision 8): the CPU pool is reduced, so the compute pass would trace a stale scene. No AO this frame (vanilla
    // SSAO); the next present turns the full pool back on (UpdateGcPoolReduce, "fallback").
    bvh::g_gpu_capture.reduced_fallback.store(true, std::memory_order_relaxed);
    bvh::EndGpuTimer(cmd_list, &data.timer);
    return {.ok = false, .pool_reduced = true};
  }
  if (!dxr_pass_a) {
    reshade::api::resource_view srvs[kRtaoSrvCount] = {};
    bvh::FillBvhSceneSrvs(*bvh_data, in.depth_view, in.dynamic, srvs);
    srvs[17] = in.mrt_normal_view;
    srvs[18] = in.isfast_view;
    srvs[19] = bvh_data->dynamic.instances_srv;
    const reshade::api::buffer_range scene_cbv = SceneCbvRange(in.scene_cbv_view);
    reshade::api::descriptor_table_update updates[3] = {
        {data.cbv_table, 0, 0, 1, reshade::api::descriptor_type::constant_buffer, &scene_cbv},
        {data.srv_table, 0, 0, kRtaoSrvCount, reshade::api::descriptor_type::shader_resource_view, srvs},
        {data.uav_table, 0, 0, kRtaoUavCount, reshade::api::descriptor_type::unordered_access_view, uavs},
    };
    device->update_descriptor_tables(3, updates);
    cmd_list->bind_pipeline(reshade::api::pipeline_stage::all_compute, data.pipeline);
    const reshade::api::descriptor_table tables[3] = {data.cbv_table, data.srv_table, data.uav_table};
    cmd_list->bind_descriptor_tables(reshade::api::shader_stage::all_compute, data.layout, 0, 3, tables);
    cmd_list->push_constants(
        reshade::api::shader_stage::all_compute, data.layout, 3, 0, kRtaoPushConstantCount, &push);
    cmd_list->dispatch((trace_w + 7u) / 8u, (trace_h + 7u) / 8u, 1u);
    RecordGpuWorkBreadcrumb(GpuWorkKind::Rtao, trace_w);
    NullComputeSlots(cmd_list, kRtaoSrvCount, kRtaoUavCount, 0u);
  }
  cmd_list->barrier(data.trace_texture, reshade::api::resource_usage::unordered_access, reshade::api::resource_usage::shader_resource);
  bvh::EndGpuTimer(cmd_list, &data.timer);

  // App-side tables: null contents (ReShade keeps descriptors in the table object).
  const reshade::api::resource_view table_null_srvs[kRtaoSrvCount] = {};
  const reshade::api::resource_view table_null_uavs[kRtaoUavCount] = {};
  reshade::api::descriptor_table_update table_nulls[2] = {
      {data.srv_table, 0, 0, kRtaoSrvCount, reshade::api::descriptor_type::shader_resource_view, table_null_srvs},
      {data.uav_table, 0, 0, kRtaoUavCount, reshade::api::descriptor_type::unordered_access_view, table_null_uavs},
  };
  device->update_descriptor_tables(2, table_nulls);
  return {.ok = true};
}

// World RTAO json, as a pure function of the values it prints (no device, no globals). The temporal and svgf objects are
// closed before the root keys, so test_modes, two_sided_discovery, temporal_diagnostics and note are top-level keys.
struct RtaoJsonInput {
  const RtaoFrameState* frame;
  const uint32_t* values;  // kRtaoStatsCount entries
  uint64_t generated_frame;
  bool temporal_on;
  bool freeze_noise;
  bool perf_gather;
  uint32_t cpu_draws[4][2];  // camera opaque candidate draws by cull mode and front face
};

inline std::string BuildRtaoJson(const RtaoJsonInput& in) {
  const RtaoFrameState& f = *in.frame;
  const uint32_t* v = in.values;
  const uint32_t* dn = &v[kRtaoStatDenoiseBase];  // SVGF reprojection counters: temporal pixels, no history, reset pixels
  // Captured parameters, the trace, the frames without AO per reason and the motion dimensions, as one json object.
  std::ostringstream diag;
  diag << "{\"captured_parameters\": {\"spp\": " << f.captured.samples
       << ", \"history_weight\": " << f.captured.history_weight
       << ", \"depth_rejection\": " << f.captured.depth_rejection
       << ", \"normal_rejection\": " << f.captured.normal_rejection
       << ", \"isfast_used\": " << (f.captured_isfast_used ? "true" : "false")
       << ", \"radius\": " << f.captured.radius << ", \"ray_max\": " << f.captured.ray_max
       << ", \"strength\": " << f.captured.strength << ", \"normal_bias\": " << f.captured.normal_bias
       << ", \"foliage_skip\": " << f.captured.foliage_skip
       << ", \"trace_characters\": " << f.captured.trace_characters
       << ", \"debug\": " << f.captured.debug << "}"
       << ", \"trace\": {\"resolution\": " << f.captured.trace_resolution << ", \"divisor\": " << f.trace_divisor
       << ", \"width\": " << f.trace_width << ", \"height\": " << f.trace_height
       << ", \"counters\": \"per trace pixel at Half and Quarter\"}"
       << ", \"frames_without_ao_by_reason\": {";
  bool first_reason = true;
  for (size_t r = 0; r < kReasonCount; ++r) {
    if (f.frames_without_ao_by_reason[r] == 0u) continue;
    diag << (first_reason ? "" : ", ") << "\"" << ReasonName(static_cast<Reason>(r)) << "\": " << f.frames_without_ao_by_reason[r];
    first_reason = false;
  }
  diag << "}"
       << ", \"last_parameter_change\": \"" << f.last_parameter_change << "\""
       << ", \"motion_dims\": {\"ok\": " << (f.motion_dims_ok ? "true" : "false")
       << ", \"motion_w\": " << f.motion_w << ", \"motion_h\": " << f.motion_h
       << ", \"depth_w\": " << f.depth_w << ", \"depth_h\": " << f.depth_h << "}}";
  std::ostringstream out;
  out << "{\n  \"schema\": 3,\n  \"generated_frame\": " << in.generated_frame
      << ",\n  \"perf_gather\": " << (in.perf_gather ? "true" : "false")
      << ",\n  \"reason\": \"" << ReasonName(f.reason) << "\""
      << ",\n  \"producing\": " << (f.producing ? "true" : "false")
      << ",\n  \"gpu_ms\": " << f.gpu_ms
      << ",\n  \"denoise_gpu_ms\": " << f.denoise_gpu_ms
      << ",\n  \"texture_bytes\": " << f.texture_bytes
      << ",\n  \"p2b_records\": " << f.p2b_records
      << ",\n  \"character_objects\": " << f.character_objects
      << ",\n  \"frames_without_ao\": " << f.frames_without_ao
      << ",\n  \"fade\": {\"start_eff\": " << f.fade.start << ", \"end_eff\": " << f.fade.end
      << ", \"coverage\": " << f.fade.coverage << ", \"requested_start\": " << f.fade.start_req
      << ", \"requested_end\": " << f.fade.end_req << ", \"range\": " << f.fade.coverage << "}"
      << ",\n  \"stats\": {\"rays\": " << v[kRtaoStatRays] << ", \"hits\": " << v[kRtaoStatHits]
      << ", \"pixels\": " << v[kRtaoStatPixels] << ", \"ao_sum\": " << v[kRtaoStatAoSum]
      << ", \"sky\": " << v[kRtaoStatSky] << ", \"normal\": " << v[kRtaoStatNormal]
      << ", \"region\": " << v[kRtaoStatRegion] << ", \"scaled\": " << v[kRtaoStatScaled]
      << ", \"invalid_refs\": " << v[kRtaoStatInvalidRefs]
      << ", \"stack_overflow\": " << v[kRtaoStatStackOverflow] << "}"
      << ",\n  \"temporal\": {\"on\": " << (in.temporal_on ? "true" : "false")
      << ", \"ran\": " << (f.temporal_ran ? "true" : "false")
      << ", \"reset\": \"" << ResetReasonName(f.reset) << "\""
      << ", \"last_reset\": \"" << ResetReasonName(f.last_reset) << "\""
      << ", \"motion\": \"" << (f.motion_source == MotionSource::Rtv4 ? "rtv4" : f.motion_source == MotionSource::Conflict ? "conflict"
                                                                                                                           : "none")
      << "\""
      << ", \"rcas_motion_res\": " << f.rcas_res << ", \"mb_rtv4_res\": " << f.rtv4_res
      << ", \"reset_counts\": [";
  for (size_t r = 0; r < static_cast<size_t>(ResetReason::Count); ++r) {
    out << (r == 0u ? "" : ", ") << f.reset_counts[r];
  }
  out << "]}"
      << ",\n  \"svgf\": {\"spatial_ran\": " << (f.filter_ran ? "true" : "false")
      << ", \"iterations\": " << f.svgf_iterations
      << ", \"counters_valid\": " << (f.svgf_stats ? "true" : "false")
      << ", \"temporal_pixels\": " << dn[0]
      << ", \"no_history\": " << dn[1]
      << ", \"reset_pixels\": " << dn[2] << "}"
      << ",\n  \"two_sided_discovery\": {\"cpu\": [";
  bool first_cpu = true;
  for (uint32_t cull = 0u; cull < 4u; ++cull) {
    for (uint32_t ccw = 0u; ccw < 2u; ++ccw) {
      const uint32_t draws = in.cpu_draws[cull][ccw];
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
      << ",\n  \"temporal_diagnostics\": " << diag.str()
      << ",\n  \"test_modes\": {\"freeze_noise\": " << (in.freeze_noise ? "true" : "false") << "}"
      << ",\n  \"note\": \"dynamic objects not traced (round 3); scaled pixels are counted by the shader\"\n}\n";
  return out.str();
}

// Reads the stats buffer and writes world_rtao.json. Called once per present; no lock held.
inline void MaybeCaptureRtaoStats(reshade::api::device* device, reshade::api::command_queue* queue) {
  if (!g_rtao_stats.requested.exchange(false, std::memory_order_relaxed)) return;
  const bvh::PoolCpuTimer stats_timer{&bvh::g_perf_gate.block_us[bvh::kPerfBlockRtaoStats]};
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

  RtaoJsonInput in = {};
  in.frame = &g_rtao_frame;
  in.values = g_rtao_stats.values;
  in.generated_frame = g_state.frame.load();
  in.temporal_on = g_rtao_temporal_enabled > 0.5f;
  in.freeze_noise = g_test_freeze_noise > 0.5f;
  in.perf_gather = bvh::g_perf_gate.gather.load(std::memory_order_relaxed);
  for (uint32_t cull = 0u; cull < 4u; ++cull) {
    for (uint32_t ccw = 0u; ccw < 2u; ++ccw) {
      in.cpu_draws[cull][ccw] = g_rtao_cull_hist[cull][ccw].load(std::memory_order_relaxed);
    }
  }
  std::string text = BuildRtaoJson(in);
  renodx::utils::path::WriteTextFile(bvh::PoolOutputDir() / "world_rtao.json", text);
}

// Gathering was switched OFF: clears the RTAO GPU timers and the newest RTAO frame times (see bvh::ClearPerfGathered).
inline void ClearRtaoPerfGathered(reshade::api::device* device) {
  RtaoDeviceData& data = GetRtaoDeviceData(device);
  bvh::ClearGpuTimerStats(&data.timer);
  g_rtao_frame.gpu_ms = -1.f;
  g_rtao_frame.denoise_gpu_ms = -1.f;
}

}  // namespace falcom_world::rtao
