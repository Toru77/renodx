#pragma once

// Phase 1 M4: primary-ray BVH traversal pass and one-shot trace statistics.
//
// The debug views dispatch world_bvh_trace against the live region TLAS and
// the mesh store (bvh_live.hpp); the trace pass writes the debug texture
// directly and accumulates integer counters into a small stats buffer.
// Statistics are read back only on request (view activation or UI button) so
// the per-frame path stays synchronization-free. Depth Compare additionally
// binds the game's current-frame lighting depth at t9 and classifies every
// pixel.

#include <atomic>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include "../../../../utils/log.hpp"
#include "../../../../utils/scene.hpp"
#include "bvh_build.hpp"
#include "bvh_pool.hpp"
#include "bvh_resources.hpp"

namespace falcom_world::bvh {

inline constexpr uint32_t kTraceSrvCount = 10u;
inline constexpr uint32_t kTraceUavCount = 2u;
inline constexpr uint32_t kTracePushConstantCount = 36u;
inline constexpr uint32_t kTraceStatsCount = 15u;
inline constexpr uint32_t kTraceStatCameraHidden = 14u;
// Inspect results follow the counters (world_bvh_trace.cs_5_0.hlsl
// TRACE_INSPECT_BASE): flags (1 traced, 2 hit, 4 hidden from the game camera), instance, mesh,
// prim, t, position xyz, depth compare class.
inline constexpr uint32_t kTraceInspectBase = 15u;
inline constexpr uint32_t kTraceInspectCount = 9u;
inline constexpr uint32_t kTraceStatsBufferCount = kTraceInspectBase + kTraceInspectCount;
static_assert(kTraceInspectBase >= kTraceStatsCount, "trace inspect slots overlap the counters");

// Push-constant float offsets, matching cb_trace in world_bvh_trace.cs_5_0.hlsl:
// [0..15] view_proj_inv, [16..19] camera_position, [20] mode, [21] width,
// [22] height, [23] compare range, [24..27] game depth rect, [28..31] inspect
// (pixel x, pixel y, on, unused), [32..35] camera view (hide, near-fade floor,
// unused, unused).
inline constexpr uint32_t kTraceCameraPositionOffset = 16u;
inline constexpr uint32_t kTraceModeOffset = 20u;
inline constexpr uint32_t kTraceWidthOffset = 21u;
inline constexpr uint32_t kTraceHeightOffset = 22u;
inline constexpr uint32_t kTraceCompareRangeOffset = 23u;
inline constexpr uint32_t kTraceDepthRectOffset = 24u;
inline constexpr uint32_t kTraceInspectOffset = 28u;
inline constexpr uint32_t kTraceCameraViewOffset = 32u;

// Debug views offered in the panel, and the trace shader mode each one runs.
enum class BvhView : int {
  Off = 0,
  TraceShaded = 1,
  TraceInstance = 2,
  DepthCompare = 3,
};

inline uint32_t TraceShaderMode(BvhView view) {
  switch (view) {
    case BvhView::TraceInstance: return 8u;
    case BvhView::DepthCompare:  return 9u;
    default:                     return 7u;
  }
}

// Game depth for Depth Compare: the lighting pass depth view of the current
// frame and the texel rectangle the frame actually covers.
struct BvhTraceDepthInput {
  reshade::api::resource_view view = {0u};
  float rect[4] = {};  // x, y, width, height in depth texels
  float compare_range = 0.f;
};

struct BvhTraceState {
  std::atomic_bool stats_requested{false};
  // Middle-click in a trace view: the pixel (as a fraction of the screen) the
  // next dispatch reports.
  std::atomic_bool inspect_pending{false};
  std::atomic<float> inspect_u{0.f};
  std::atomic<float> inspect_v{0.f};
  // Hide what the game camera does not show: instances no camera VS drew
  // (shadow-only casters) and near-faded surfaces (A/B against the game's depth).
  std::atomic_bool hide_camera_hidden{false};
};

inline BvhTraceState g_bvh_trace;

inline bool g_trace_failure_logged = false;

inline void LogTraceResourceFailure(const char* stage) {
  if (g_trace_failure_logged) return;
  g_trace_failure_logged = true;
  renodx::utils::log::w("[world-bvh] trace resource creation failed: ", stage);
}

inline bool EnsureBvhTraceResources(reshade::api::device* device, BvhDeviceData* data) {
  if (device == nullptr || data == nullptr || !data->bvh_ready) return false;
  if (data->trace_pipeline.handle != 0u && data->trace_ready) return true;

#if defined(__world_bvh_trace_EMBED_FILE)
  using DR = reshade::api::descriptor_range;
  using DS = reshade::api::shader_stage;
  using DT = reshade::api::descriptor_type;
  using P = reshade::api::pipeline_layout_param;

  DR srv_range = {0, 0, 0, kTraceSrvCount, DS::all_compute, 1, DT::shader_resource_view};
  DR uav_range = {0, 0, 0, kTraceUavCount, DS::all_compute, 1, DT::unordered_access_view};
  reshade::api::constant_range push_range = {};
  push_range.binding = 0;
  push_range.dx_register_index = 13;
  push_range.dx_register_space = 0;
  push_range.count = kTracePushConstantCount;
  push_range.visibility = DS::all_compute;
  P params[3] = {};
  params[0].type = reshade::api::pipeline_layout_param_type::descriptor_table;
  params[0].descriptor_table.count = 1;
  params[0].descriptor_table.ranges = &srv_range;
  params[1].type = reshade::api::pipeline_layout_param_type::descriptor_table;
  params[1].descriptor_table.count = 1;
  params[1].descriptor_table.ranges = &uav_range;
  params[2].type = reshade::api::pipeline_layout_param_type::push_constants;
  params[2].push_constants = push_range;
  if (data->trace_layout.handle == 0u
      && !device->create_pipeline_layout(3, params, &data->trace_layout)) {
    LogTraceResourceFailure("pipeline layout");
    return false;
  }
  if (data->trace_srv_table.handle == 0u
      && !device->allocate_descriptor_table(data->trace_layout, 0, &data->trace_srv_table)) {
    LogTraceResourceFailure("srv descriptor table");
    return false;
  }
  if (data->trace_uav_table.handle == 0u
      && !device->allocate_descriptor_table(data->trace_layout, 1, &data->trace_uav_table)) {
    LogTraceResourceFailure("uav descriptor table");
    return false;
  }
  if (data->trace_pipeline.handle == 0u) {
    reshade::api::shader_desc shader = {};
    shader.code = __world_bvh_trace.data();
    shader.code_size = __world_bvh_trace.size();
    shader.entry_point = "main";
    reshade::api::pipeline_subobject subobject = {
        reshade::api::pipeline_subobject_type::compute_shader, 1, &shader};
    if (!device->create_pipeline(data->trace_layout, 1, &subobject, &data->trace_pipeline)) {
      LogTraceResourceFailure("compute pipeline");
      return false;
    }
  }
  if (data->trace_stats_buffer.handle == 0u) {
    const uint32_t zeros[kTraceStatsBufferCount] = {};
    if (!CreatePoolBuffer(
            device, zeros, sizeof(zeros), sizeof(uint32_t),
            &data->trace_stats_buffer, &data->trace_stats_uav,
            reshade::api::resource_usage::unordered_access | reshade::api::resource_usage::copy_source)) {
      LogTraceResourceFailure("stats buffer");
      return false;
    }
  }
  if (!data->trace_ready) {
    renodx::utils::log::i("[world-bvh] trace resources ready");
  }
  g_trace_failure_logged = false;
  data->trace_ready = true;
  return true;
#else
  LogTraceResourceFailure("shader not embedded");
  return false;
#endif
}

inline void DispatchBvhTrace(
    reshade::api::device* device,
    reshade::api::command_list* cmd_list,
    BvhDeviceData* data,
    uint32_t mode,
    uint32_t width,
    uint32_t height,
    const BvhTraceDepthInput& depth) {
  if (device == nullptr || cmd_list == nullptr || data == nullptr) return;
  if (!EnsureBvhTraceResources(device, data)) return;
  if (!data->bvh_ready || data->blas_nodes.srv.handle == 0u || data->tlas_node_srv.handle == 0u) return;
  if (data->debug_uav.handle == 0u || data->trace_stats_uav.handle == 0u) return;

  // The whole buffer (offset 0 and its full size: see WriteBufferRange).
  const uint32_t zeros[kTraceStatsBufferCount] = {};
  device->update_buffer_region(zeros, data->trace_stats_buffer, 0u, sizeof(zeros));

  // Slot 9 is rewritten on every dispatch: a game view from an earlier frame
  // may no longer exist, so it is null unless Depth Compare passes this
  // frame's view.
  reshade::api::resource_view srvs[kTraceSrvCount] = {
      data->vertices.srv, data->indices.srv, data->mesh_srv, data->instance_srv, data->active_srv,
      data->blas_nodes.srv, data->blas_leaves.srv, data->tlas_node_srv, data->tlas_leaf_srv,
      depth.view};
  static_assert(sizeof(srvs) / sizeof(srvs[0]) == kTraceSrvCount, "trace SRV table must have exactly 10 entries");
  reshade::api::descriptor_table_update srv_update = {
      data->trace_srv_table, 0, 0, kTraceSrvCount, reshade::api::descriptor_type::shader_resource_view, srvs};
  device->update_descriptor_tables(1, &srv_update);
  reshade::api::resource_view uavs[kTraceUavCount] = {data->debug_uav, data->trace_stats_uav};
  reshade::api::descriptor_table_update uav_update = {
      data->trace_uav_table, 0, 0, kTraceUavCount, reshade::api::descriptor_type::unordered_access_view, uavs};
  device->update_descriptor_tables(1, &uav_update);

  cmd_list->bind_pipeline(reshade::api::pipeline_stage::all_compute, data->trace_pipeline);
  const reshade::api::descriptor_table tables[2] = {data->trace_srv_table, data->trace_uav_table};
  cmd_list->bind_descriptor_tables(
      reshade::api::shader_stage::all_compute, data->trace_layout, 0, 2, tables);

  float constants[kTracePushConstantCount] = {};
  // disableMapObjNearFade_g. Until the scene constants are captured it is
  // unknown: 1 then, so nothing is hidden without evidence.
  float near_fade_floor = 1.f;
  {
    std::lock_guard<std::mutex> lock(g_state.mutex);
    std::memcpy(constants, g_state.camera.view_proj_inv, sizeof(float) * 16u);
    if (g_state.camera.has_fade) near_fade_floor = g_state.camera.near_fade_floor;
  }
  float camera_position[3] = {};
  if (!PoolCameraPosition(camera_position)) {
    std::lock_guard<std::mutex> lock(g_state.mutex);
    camera_position[0] = g_state.camera.view_inv[3];
    camera_position[1] = g_state.camera.view_inv[7];
    camera_position[2] = g_state.camera.view_inv[11];
  }
  constants[kTraceCameraPositionOffset + 0u] = camera_position[0];
  constants[kTraceCameraPositionOffset + 1u] = camera_position[1];
  constants[kTraceCameraPositionOffset + 2u] = camera_position[2];
  constants[kTraceCameraPositionOffset + 3u] = 0.f;
  constants[kTraceModeOffset] = *reinterpret_cast<const float*>(&mode);
  constants[kTraceWidthOffset] = *reinterpret_cast<const float*>(&width);
  constants[kTraceHeightOffset] = *reinterpret_cast<const float*>(&height);
  constants[kTraceCompareRangeOffset] = depth.compare_range;
  for (uint32_t i = 0; i < 4u; ++i) constants[kTraceDepthRectOffset + i] = depth.rect[i];
  const bool hiding = g_bvh_trace.hide_camera_hidden.load(std::memory_order_relaxed);
  data->trace_hiding = hiding;
  constants[kTraceCameraViewOffset + 0u] = hiding ? 1.f : 0.f;
  constants[kTraceCameraViewOffset + 1u] = near_fade_floor;
  static_assert(kTraceDepthRectOffset + 4u <= kTraceInspectOffset, "trace push constants overlap");
  static_assert(kTraceInspectOffset + 4u <= kTraceCameraViewOffset, "trace push constants overlap");
  static_assert(kTraceCameraViewOffset + 4u <= kTracePushConstantCount, "trace push constants overflow");
  data->inspect.requested = false;
  if (g_bvh_trace.inspect_pending.exchange(false, std::memory_order_relaxed) && width != 0u && height != 0u) {
    const float u = (std::min)((std::max)(g_bvh_trace.inspect_u.load(std::memory_order_relaxed), 0.f), 1.f);
    const float v = (std::min)((std::max)(g_bvh_trace.inspect_v.load(std::memory_order_relaxed), 0.f), 1.f);
    data->inspect.x = (std::min)(static_cast<uint32_t>(u * static_cast<float>(width)), width - 1u);
    data->inspect.y = (std::min)(static_cast<uint32_t>(v * static_cast<float>(height)), height - 1u);
    data->inspect.requested = true;
    data->inspect.hiding = hiding;
    constants[kTraceInspectOffset + 0u] = static_cast<float>(data->inspect.x);
    constants[kTraceInspectOffset + 1u] = static_cast<float>(data->inspect.y);
    constants[kTraceInspectOffset + 2u] = 1.f;
  }
  cmd_list->push_constants(
      reshade::api::shader_stage::all_compute, data->trace_layout, 2, 0, kTracePushConstantCount, constants);
  BeginGpuTimer(device, cmd_list, &data->trace_timer);
  cmd_list->dispatch((width + 7u) / 8u, (height + 7u) / 8u, 1u);
  EndGpuTimer(cmd_list, &data->trace_timer);
}

inline bool ReadbackBvhTraceStats(
    reshade::api::device* device,
    reshade::api::command_queue* queue,
    BvhDeviceData* data) {
  if (device == nullptr || queue == nullptr || data == nullptr) return false;
  if (data->trace_stats_buffer.handle == 0u) return false;
  std::vector<uint8_t> bytes;
  if (!renodx::utils::scene::ReadbackBuffer(
          device, queue, data->trace_stats_buffer, 0u,
          sizeof(uint32_t) * kTraceStatsBufferCount, &bytes)) {
    return false;
  }
  if (bytes.size() < sizeof(uint32_t) * kTraceStatsBufferCount) return false;
  uint32_t values[kTraceStatsBufferCount] = {};
  std::memcpy(values, bytes.data(), sizeof(uint32_t) * kTraceStatsBufferCount);
  if (data->inspect.requested) {
    BvhInspect& inspect = data->inspect;
    const uint32_t* result = values + kTraceInspectBase;
    inspect.requested = false;
    inspect.fresh = true;
    inspect.valid = (result[0] & 1u) != 0u;
    inspect.hit = (result[0] & 2u) != 0u;
    inspect.camera_hidden = (result[0] & 4u) != 0u;
    inspect.instance = result[1];
    inspect.mesh = result[2];
    inspect.prim = result[3];
    std::memcpy(&inspect.t, &result[4], sizeof(float));
    std::memcpy(inspect.position, &result[5], sizeof(inspect.position));
    inspect.compare_class = result[8];
  }
  data->trace_stats.rays = values[0];
  data->trace_stats.hits = values[1];
  data->trace_stats.misses = values[2];
  data->trace_stats.invalid_refs = values[3];
  data->trace_stats.stack_overflow = values[4];
  data->trace_stats.triangle_tests = values[5];
  data->trace_stats.max_stack_depth = values[6];
  data->trace_stats.compare_match = values[7];
  data->trace_stats.compare_missing = values[8];
  data->trace_stats.compare_extra = values[9];
  data->trace_stats.compare_extra_sky = values[10];
  data->trace_stats.compare_far = values[11];
  data->trace_stats.compare_sky = values[12];
  data->trace_stats.compare_no_depth = values[13];
  data->trace_stats.camera_hidden = values[kTraceStatCameraHidden];
  data->trace_stats.hiding = data->trace_hiding;
  data->trace_stats.valid = true;
  data->trace_stats.invariant_ok = data->trace_stats.rays == data->trace_stats.hits + data->trace_stats.misses;
  renodx::utils::log::i(
      "[world-bvh] trace: rays=", data->trace_stats.rays,
      " hits=", data->trace_stats.hits,
      " misses=", data->trace_stats.misses,
      " invalid_refs=", data->trace_stats.invalid_refs,
      " stack_overflow=", data->trace_stats.stack_overflow,
      " triangle_tests=", data->trace_stats.triangle_tests,
      " max_stack_depth=", data->trace_stats.max_stack_depth,
      " camera_hidden=", data->trace_stats.camera_hidden,
      " invariant=", data->trace_stats.invariant_ok ? "ok" : "FAIL");
  const BvhTraceStats& stats = data->trace_stats;
  if (stats.compare_match + stats.compare_missing + stats.compare_extra + stats.compare_extra_sky
          + stats.compare_far + stats.compare_sky + stats.compare_no_depth
      != 0u) {
    renodx::utils::log::i(
        "[world-bvh] depth compare: match=", stats.compare_match,
        " missing=", stats.compare_missing,
        " extra=", stats.compare_extra,
        " extra_sky=", stats.compare_extra_sky,
        " far=", stats.compare_far,
        " sky=", stats.compare_sky,
        " no_depth=", stats.compare_no_depth);
  }
  return true;
}

inline void MaybeCaptureBvhTraceStats(
    reshade::api::device* device,
    reshade::api::command_queue* queue,
    BvhDeviceData* data) {
  if (!g_bvh_trace.stats_requested.exchange(false, std::memory_order_relaxed)) return;
  ReadbackBvhTraceStats(device, queue, data);
}

}  // namespace falcom_world::bvh
