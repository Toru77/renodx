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
  float filter_gpu_ms = -1.f;        // newest completed spatial filter chain (negative: none yet)
  bool filter_requested = false;     // Spatial Filter on at the newest dispatch
  bool filter_ran = false;           // the filter ran: the output is the filtered AO
  bool filter_failed = false;        // filter targets or pipeline could not be created
  int filter_type = 0;               // configuration used at the newest dispatch
  int filter_radius = 2;
  int filter_taps_requested = 5;     // Filter Taps row value (samples per direction, or a-trous side)
  uint32_t filter_tap_count = 0u;    // effective odd taps N_eff
  uint32_t filter_iterations = 0u;
  uint32_t filter_passes = 0u;
  uint32_t filter_taps = 0u;         // taps per pass (first pass)
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
  uint64_t frames_without_ao_by_reason[kReasonCount] = {};  // frames without AO, per Reason code (F1)
  const char* last_parameter_change = "";  // first field that changed on the last Parameters reset (F1)
  bool motion_dims_ok = true;        // RTV4 size equals the depth size (texel units)
  uint32_t motion_w = 0u, motion_h = 0u;
  uint32_t depth_w = 0u, depth_h = 0u;
  uint64_t temporal_on_frames = 0u;  // frames with Temporal on (TAA t3 never-captured check)
  bool matrix_valid = false;         // CPU matrix self-check ran (D1b; indicative, may be one frame old)
  float matrix_identity_err = 0.f;
  float matrix_prev_diff = 0.f;
  ParameterSnapshot captured = {};   // parameters of the last successful dispatch (P0)
  bool captured_isfast_used = false;
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
  // Spatial filter (S3): runs on the final AO after pass B (or pass A). Debug modes 1..7 bypass it; mode 8 runs it.
  bool spatial = false;
  int filter_type = 0;     // 0 separable bilateral, 1 a-trous
  int filter_radius = 2;
  int filter_passes = 1;   // Filter Passes: 1..4
  int filter_taps = 5;     // Filter Taps: samples per direction (separable) or kernel side (a-trous)
  int debug_mode = 0;
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

// Spatial filter (S3): the MakeFilterPlan chain, one dispatch per pass, slots nulled after each one. The first pass
// reads the AO texel (A); the last pass writes B. Returns false when a target or the pipeline cannot be created
// (the caller then uses A). The filter timer wraps the whole chain.
inline bool DispatchSpatialFilter(reshade::api::device* device, reshade::api::command_list* cmd_list,
                                  const RtaoDispatchInputs& in, RtaoDeviceData* data) {
  if (!EnsureRtaoFilterTargets(device, data, in.width, in.height) || !EnsureRtaoFilterPipeline(device, data)) return false;
  const FilterPlan plan = MakeFilterPlan(in.filter_type, in.filter_radius, in.filter_passes, in.filter_taps);
  bvh::BeginGpuTimer(device, cmd_list, &data->filter_timer);
  cmd_list->barrier(data->ao_texture, reshade::api::resource_usage::unordered_access, reshade::api::resource_usage::shader_resource);
  cmd_list->bind_pipeline(reshade::api::pipeline_stage::all_compute, data->filter_pipeline);
  for (uint32_t i = 0u; i < plan.pass_count; ++i) {
    const FilterPass& pass = plan.passes[i];
    // Intermediate and B slots index the filter arrays as F0 = 0, F1 = 1, B = 2 (FilterSlot values 1..3).
    const uint32_t out_index = static_cast<uint32_t>(pass.output) - 1u;
    const uint32_t in_index = static_cast<uint32_t>(pass.input) - 1u;
    const bool input_is_ao = pass.input == FilterSlot::A;
    const bool output_is_uint = pass.output == FilterSlot::B;
    const reshade::api::resource_view srvs[kRtaoFilterSrvCount] = {
        in.depth_view, in.mrt_normal_view, input_is_ao ? data->ao_srv : reshade::api::resource_view{0u},
        input_is_ao ? reshade::api::resource_view{0u} : data->filter_srv[in_index], data->ao_srv};
    const reshade::api::resource_view uavs[kRtaoFilterUavCount] = {
        output_is_uint ? data->filter_uav[out_index] : reshade::api::resource_view{0u},
        output_is_uint ? reshade::api::resource_view{0u} : data->filter_uav[out_index], data->stats_uav};
    RtaoFilterPushConstants push = {};
    push.size[0] = static_cast<float>(in.width);
    push.size[1] = static_cast<float>(in.height);
    push.size[2] = static_cast<float>(in.filter_radius);
    push.size[3] = plan.kind == FilterKind::Separable ? 0.f : 1.f;
    push.pass[0] = static_cast<float>(pass.direction);
    push.pass[1] = static_cast<float>(pass.step);
    push.pass[2] = static_cast<float>(plan.taps_side);
    push.pass[3] = i + 1u == plan.pass_count ? 1.f : 0.f;
    push.flags[0] = input_is_ao ? 1.f : 0.f;
    push.flags[1] = output_is_uint ? 1.f : 0.f;
    push.flags[2] = static_cast<float>(in.debug_mode);
    reshade::api::descriptor_table_update updates[3] = {
        {data->filter_cbv_table, 0, 0, 1, reshade::api::descriptor_type::constant_buffer, &in.scene_cbv_view},
        {data->filter_srv_table, 0, 0, kRtaoFilterSrvCount, reshade::api::descriptor_type::shader_resource_view, srvs},
        {data->filter_uav_table, 0, 0, kRtaoFilterUavCount, reshade::api::descriptor_type::unordered_access_view, uavs},
    };
    device->update_descriptor_tables(3, updates);
    const reshade::api::descriptor_table tables[3] = {data->filter_cbv_table, data->filter_srv_table, data->filter_uav_table};
    cmd_list->bind_descriptor_tables(reshade::api::shader_stage::all_compute, data->filter_layout, 0, 3, tables);
    cmd_list->push_constants(reshade::api::shader_stage::all_compute, data->filter_layout, 3, 0, kRtaoFilterPushCount, &push);
    cmd_list->dispatch((in.width + 7u) / 8u, (in.height + 7u) / 8u, 1u);
    NullComputeSlots(cmd_list, kRtaoFilterSrvCount, kRtaoFilterUavCount, 0u);
    cmd_list->barrier(data->filter_texture[out_index], reshade::api::resource_usage::unordered_access, reshade::api::resource_usage::shader_resource);
  }
  // App-side tables: null contents (ReShade keeps descriptors in the table object).
  const reshade::api::resource_view null_srvs[kRtaoFilterSrvCount] = {};
  const reshade::api::resource_view null_uavs[kRtaoFilterUavCount] = {};
  reshade::api::descriptor_table_update nulls[2] = {
      {data->filter_srv_table, 0, 0, kRtaoFilterSrvCount, reshade::api::descriptor_type::shader_resource_view, null_srvs},
      {data->filter_uav_table, 0, 0, kRtaoFilterUavCount, reshade::api::descriptor_type::unordered_access_view, null_uavs},
  };
  device->update_descriptor_tables(2, nulls);
  bvh::EndGpuTimer(cmd_list, &data->filter_timer);
  return true;
}

// The AO view the lighting shader (t22), the micro shadows and the deferred list read: the filtered AO when the
// filter ran this frame, otherwise the AO texel itself (A).
inline reshade::api::resource_view OutputSrv(const RtaoDeviceData& rd) {
  return rd.filter_ran ? rd.filter_srv[2] : rd.ao_srv;
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
  data.filter_ran = false;
  if (in.spatial && !data.filter_was_on) data.filter_failed = false;  // off -> on: one retry after a failure
  data.filter_was_on = in.spatial;
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
  const reshade::api::resource_view uavs[kRtaoUavCount] = {data.ao_uav, data.stats_uav, data.raw_uav[data.raw_index]};
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
    cmd_list->barrier(data.raw_texture[data.raw_index], reshade::api::resource_usage::unordered_access, reshade::api::resource_usage::shader_resource);

    // Pass B. History: read the current index, write the other one.
    const uint32_t read = data.history_index;
    const uint32_t write = data.history_index ^ 1u;
    const reshade::api::resource_view temporal_srvs[kRtaoTemporalSrvCount] = {
        data.raw_srv[data.raw_index], in.depth_view, in.mrt_normal_view, in.motion_view, data.history_srv[read],
        data.raw_srv[data.raw_index ^ 1u]};
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
    data.raw_index ^= 1u;        // and this raw AO is the previous frame for the next pass B
  }
  bvh::EndGpuTimer(cmd_list, &data.timer);
  // Spatial filter after the AO is final, outside the main timer span. Debug modes 1..7 write AO-free texels, so the filter does not run for them.
  if (in.spatial && (in.debug_mode < 1 || in.debug_mode > 7)) {
    data.filter_ran = DispatchSpatialFilter(device, cmd_list, in, &data);
  }

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

// World RTAO json, as a pure function of the values it prints (no device, no globals). The temporal object is
// closed before the root keys, so test_modes, two_sided_discovery, temporal_diagnostics and note are top-level keys.
struct RtaoJsonInput {
  const RtaoFrameState* frame;
  const uint32_t* values;  // kRtaoStatsCount entries
  uint64_t generated_frame;
  bool temporal_on;
  bool zero_motion;
  bool camera_matrix;
  bool freeze_noise;
  uint32_t cpu_draws[4][2];  // camera opaque candidate draws by cull mode and front face
};

inline std::string BuildRtaoJson(const RtaoJsonInput& in) {
  const RtaoFrameState& f = *in.frame;
  const uint32_t* v = in.values;
  // Pass B diagnostics (F1) and the per-reason frame counts, as one json object.
  const uint32_t* tv = v;
  const uint32_t* fv = &v[kRtaoStatTemporalFBase];
  const TracedDenominators den = ComputeTracedDenominators(tv[kRtaoStatPixels], tv[kRtaoStatSky], tv[kRtaoStatNormal],
                                                          tv[kRtaoStatRegion], tv[kRtaoStatScaled], tv[kRtaoStatTemporalBase + 5u]);
  const double tpx = den.traced > 0u ? static_cast<double>(den.traced) : 1.0;
  const double wbase = den.traced > 0u ? 1000.0 * den.traced : 1.0;
  const double w_acc = den.traced > 0u ? 100.0 * v[kRtaoStatWeightBase] / wbase : 0.0;
  const double w_dep = den.traced > 0u ? 100.0 * v[kRtaoStatWeightBase + 1u] / wbase : 0.0;
  const double w_nor = den.traced > 0u ? 100.0 * v[kRtaoStatWeightBase + 2u] / wbase : 0.0;
  const uint32_t* qv = &v[kRtaoStatQualityBase];
  std::ostringstream diag;
  diag << "{\"traced_pixels\": " << den.traced << ", \"taps_traced\": " << den.taps
       << ", \"reset_traced\": " << den.reset_traced << ", \"pass_b_pixels\": " << tv[kRtaoStatTemporalBase]
       << ", \"moving_pixels\": " << fv[0] << ", \"motion_max_px\": " << fv[1] / 100.0
       << ", \"mean_motion_px\": " << fv[2] / 100.0 / tpx << ", \"share_moving\": " << fv[0] / tpx
       << ", \"share_clamp_active\": " << fv[3] / tpx << ", \"mean_clamp_shift\": " << fv[4] / 1000.0 / tpx
       << ", \"mean_alpha\": " << fv[5] / 1000.0 / tpx << ", \"mean_abs_raw_minus_ao\": " << fv[6] / 1000.0 / tpx
       << ", \"share_no_history\": " << fv[7] / tpx
       << ", \"texel_differs\": " << fv[8] << ", \"share_texel_differs\": " << fv[8] / tpx
       << ", \"weighted_shares\": {\"accepted\": " << (den.traced > 0u ? 100.0 * v[kRtaoStatWeightBase] / (1000.0 * den.traced) : 0.0)
       << ", \"rejected_depth\": " << (den.traced > 0u ? 100.0 * v[kRtaoStatWeightBase + 1u] / (1000.0 * den.traced) : 0.0)
       << ", \"rejected_normal\": " << (den.traced > 0u ? 100.0 * v[kRtaoStatWeightBase + 2u] / (1000.0 * den.traced) : 0.0) << "}"
       << ", \"weighted_other_pct\": " << (100.0 - w_acc - w_dep - w_nor)
       << ", \"weighted_note\": \"not counted on reset frames; other = no history, out of bounds or reset\""
       << ", \"depth_ratio_pct\": {\"invalid\": " << (den.traced > 0u ? 100.0 * v[kRtaoStatDepthBase] / den.traced : 0.0)
       << ", \"lt_0_1\": " << (den.traced > 0u ? 100.0 * v[kRtaoStatDepthBase + 1u] / den.traced : 0.0)
       << ", \"lt_0_5\": " << (den.traced > 0u ? 100.0 * v[kRtaoStatDepthBase + 2u] / den.traced : 0.0)
       << ", \"lt_1\": " << (den.traced > 0u ? 100.0 * v[kRtaoStatDepthBase + 3u] / den.traced : 0.0)
       << ", \"lt_2\": " << (den.traced > 0u ? 100.0 * v[kRtaoStatDepthBase + 4u] / den.traced : 0.0)
       << ", \"lt_5\": " << (den.traced > 0u ? 100.0 * v[kRtaoStatDepthBase + 5u] / den.traced : 0.0)
       << ", \"ge_5\": " << (den.traced > 0u ? 100.0 * v[kRtaoStatDepthBase + 6u] / den.traced : 0.0) << "}"
       << ", \"normal_dot_pct\": {\"lt_0\": " << (den.traced > 0u ? 100.0 * v[kRtaoStatNormalBase] / den.traced : 0.0)
       << ", \"lt_0_5\": " << (den.traced > 0u ? 100.0 * v[kRtaoStatNormalBase + 1u] / den.traced : 0.0)
       << ", \"lt_0_7\": " << (den.traced > 0u ? 100.0 * v[kRtaoStatNormalBase + 2u] / den.traced : 0.0)
       << ", \"lt_0_9\": " << (den.traced > 0u ? 100.0 * v[kRtaoStatNormalBase + 3u] / den.traced : 0.0)
       << ", \"lt_0_97\": " << (den.traced > 0u ? 100.0 * v[kRtaoStatNormalBase + 4u] / den.traced : 0.0)
       << ", \"ge_0_97\": " << (den.traced > 0u ? 100.0 * v[kRtaoStatNormalBase + 5u] / den.traced : 0.0) << "}"
       << ", \"raw_difference\": {\"count\": " << v[kRtaoStatRawBase + 1u]
       << ", \"mean_abs\": " << (v[kRtaoStatRawBase + 1u] > 0u
            ? v[kRtaoStatRawBase] / 1000.0 / v[kRtaoStatRawBase + 1u] : 0.0)
       << ", \"identical_share_pct\": " << (v[kRtaoStatRawBase + 1u] > 0u
            ? 100.0 * v[kRtaoStatRawBase + 2u] / v[kRtaoStatRawBase + 1u] : 0.0) << "}"
       << ", \"quality\": {\"mean_raw\": " << MeanFromSum(qv[1], den.traced)
       << ", \"mean_output\": " << MeanFromSum(qv[0], den.traced)
       << ", \"raw_minus_output\": " << (MeanFromSum(qv[1], den.traced) - MeanFromSum(qv[0], den.traced))
       << ", \"roughness_raw\": " << MeanFromSum(qv[3], qv[2])
       << ", \"roughness_previous_output\": " << MeanFromSum(qv[4], qv[2])
       << ", \"roughness_ratio\": " << (MeanFromSum(qv[3], qv[2]) > 0.0 ? MeanFromSum(qv[4], qv[2]) / MeanFromSum(qv[3], qv[2]) : 0.0)
       << ", \"roughness_pixels\": " << qv[2]
       << ", \"output_change_mean\": " << MeanFromSum(qv[6], qv[5])
       << ", \"output_change_pixels\": " << qv[5]
       << ", \"note\": \"still camera only; compares with the previous output at the same pixel\"}"
       << ", \"captured_parameters\": {\"spp\": " << f.captured.samples
       << ", \"history_weight\": " << f.captured.history_weight
       << ", \"depth_rejection\": " << f.captured.depth_rejection
       << ", \"normal_rejection\": " << f.captured.normal_rejection
       << ", \"history_clamp\": " << f.captured.history_clamp
       << ", \"isfast_used\": " << (f.captured_isfast_used ? "true" : "false")
       << ", \"radius\": " << f.captured.radius << ", \"ray_max\": " << f.captured.ray_max
       << ", \"strength\": " << f.captured.strength << ", \"normal_bias\": " << f.captured.normal_bias
       << ", \"debug\": " << f.captured.debug << "}"
       << ", \"frames_without_ao_by_reason\": {";
  bool first_reason = true;
  for (size_t r = 0; r < kReasonCount; ++r) {
    if (f.frames_without_ao_by_reason[r] == 0u) continue;
    diag << (first_reason ? "" : ", ") << "\"" << ReasonName(static_cast<Reason>(r)) << "\": " << f.frames_without_ao_by_reason[r];
    first_reason = false;
  }
  // D1b: motion vs camera-matrix difference (static geometry only), over the traced pixels.
  const uint32_t* dv = &v[kRtaoStatTemporalFBase + 9u];
  const uint32_t dtraced = den.traced;
  float jitter_x = 0.f, jitter_y = 0.f;
  std::memcpy(&jitter_x, &v[kRtaoStatJitterX], sizeof(float));
  std::memcpy(&jitter_y, &v[kRtaoStatJitterY], sizeof(float));
  diag << "}, \"camera_matrix_diff\": {\"note\": \"valid for static geometry only; moving objects show real motion here, not a jitter error\""
       << ", \"bins\": [" << dv[0] << ", " << dv[1] << ", " << dv[2] << ", " << dv[3] << ", " << dv[4] << "]"
       << ", \"share_within_0_25_px\": " << (dtraced > 0u ? static_cast<double>(dv[0] + dv[1]) / dtraced : 0.0)
       << ", \"mean_px\": " << (dtraced > 0u ? static_cast<double>(dv[5]) / 100.0 / dtraced : 0.0)
       << ", \"max_px\": " << dv[6] / 100.0
       << ", \"jitter_diff_px\": [" << jitter_x << ", " << jitter_y << "]"
       << ", \"matrix_self_check\": {\"valid\": " << (f.matrix_valid ? "true" : "false")
       << ", \"identity_err\": " << f.matrix_identity_err
       << ", \"prev_diff\": " << f.matrix_prev_diff << "}}"
       << ", \"last_parameter_change\": \"" << f.last_parameter_change << "\""
       << ", \"motion_dims\": {\"ok\": " << (f.motion_dims_ok ? "true" : "false")
       << ", \"motion_w\": " << f.motion_w << ", \"motion_h\": " << f.motion_h
       << ", \"depth_w\": " << f.depth_w << ", \"depth_h\": " << f.depth_h << "}}";
  std::ostringstream out;
  out << "{\n  \"schema\": 2,\n  \"generated_frame\": " << in.generated_frame
      << ",\n  \"reason\": \"" << ReasonName(f.reason) << "\""
      << ",\n  \"producing\": " << (f.producing ? "true" : "false")
      << ",\n  \"gpu_ms\": " << f.gpu_ms
      << ",\n  \"texture_bytes\": " << f.texture_bytes
      << ",\n  \"frames_without_ao\": " << f.frames_without_ao
      << ",\n  \"fade\": {\"start_eff\": " << f.fade.start << ", \"end_eff\": " << f.fade.end
      << ", \"coverage\": " << f.fade.coverage << "}"
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
      << ", \"motion\": \"" << (f.motion_source == MotionSource::Rtv4 ? "rtv4" : f.motion_source == MotionSource::Conflict ? "conflict" : "none") << "\""
      << ", \"rcas_motion_res\": " << f.rcas_res << ", \"mb_rtv4_res\": " << f.rtv4_res
      << ", \"temporal_bytes\": " << f.temporal_bytes << ", \"history_bytes_per_pixel\": 20"
      << ", \"temporal_pixels\": " << v[kRtaoStatTemporalBase + 0u]
      << ", \"valid_taps\": " << v[kRtaoStatTemporalBase + 1u]
      << ", \"rejected_depth\": " << v[kRtaoStatTemporalBase + 2u]
      << ", \"rejected_normal\": " << v[kRtaoStatTemporalBase + 3u]
      << ", \"out_of_bounds\": " << v[kRtaoStatTemporalBase + 4u]
      << ", \"reset_pixels\": " << v[kRtaoStatTemporalBase + 5u]
      << ", \"reset_counts\": [";
  for (size_t r = 0; r < static_cast<size_t>(ResetReason::Count); ++r) {
    out << (r == 0u ? "" : ", ") << f.reset_counts[r];
  }
  out << "]}"
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
      << ",\n  \"spatial_filter\": {\"on\": " << (f.filter_requested ? "true" : "false")
      << ", \"type\": " << f.filter_type << ", \"radius\": " << f.filter_radius << ", \"tap_count\": " << f.filter_tap_count
      << ", \"iterations\": " << f.filter_iterations << ", \"passes\": " << f.filter_passes
      << ", \"taps_per_pass\": " << f.filter_taps << ", \"gpu_ms\": " << f.filter_gpu_ms
      << ", \"pixels\": " << v[kRtaoStatFilterBase]
      << ", \"mean_change\": " << MeanFromSum(v[kRtaoStatFilterBase + 1u], v[kRtaoStatFilterBase])
      << ", \"changed_gt_1lsb_pct\": " << (v[kRtaoStatFilterBase] > 0u ? 100.0 * v[kRtaoStatFilterBase + 2u] / v[kRtaoStatFilterBase] : 0.0)
      << ", \"failed\": " << (f.filter_failed ? "true" : "false") << "}"
      << ",\n  \"test_modes\": {\"zero_motion\": " << (in.zero_motion ? "true" : "false")
      << ", \"camera_matrix\": " << (in.camera_matrix ? "true" : "false")
      << ", \"freeze_noise\": " << (in.freeze_noise ? "true" : "false") << "}"
      << ",\n  \"note\": \"dynamic objects not traced (round 3); scaled pixels are counted by the shader\"\n}\n";
  return out.str();
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

  RtaoJsonInput in = {};
  in.frame = &g_rtao_frame;
  in.values = g_rtao_stats.values;
  in.generated_frame = g_state.frame.load();
  in.temporal_on = g_rtao_temporal_enabled > 0.5f;
  in.zero_motion = g_test_zero_motion > 0.5f;
  in.camera_matrix = g_test_camera_matrix > 0.5f;
  in.freeze_noise = g_test_freeze_noise > 0.5f;
  for (uint32_t cull = 0u; cull < 4u; ++cull) {
    for (uint32_t ccw = 0u; ccw < 2u; ++ccw) {
      in.cpu_draws[cull][ccw] = g_rtao_cull_hist[cull][ccw].load(std::memory_order_relaxed);
    }
  }
  std::string text = BuildRtaoJson(in);
  renodx::utils::path::WriteTextFile(bvh::PoolOutputDir() / "world_rtao.json", text);
}

}  // namespace falcom_world::rtao
