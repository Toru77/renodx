#pragma once

// Phase 1 M4: primary-ray BVH traversal pass and one-shot trace statistics.
//
// Modes 7/8 dispatch world_bvh_trace against the region TLAS/BLAS; the trace
// pass writes the debug texture directly and accumulates integer counters into
// a small stats buffer. Statistics are read back only on request (mode
// activation or UI button) so the per-frame path stays synchronization-free.

#include <atomic>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include "../../../../utils/log.hpp"
#include "bvh_build.hpp"
#include "bvh_resources.hpp"

namespace falcom_world::bvh {

inline constexpr uint32_t kTraceSrvCount = 9u;
inline constexpr uint32_t kTraceUavCount = 2u;
inline constexpr uint32_t kTracePushConstantCount = 24u;
inline constexpr uint32_t kTraceStatsCount = 7u;

// Push-constant float offsets, matching cb_trace in world_bvh_trace.cs_5_0.hlsl:
// [0..15] view_proj_inv, [16..19] camera_position, [20] mode, [21] width, [22] height.
inline constexpr uint32_t kTraceCameraPositionOffset = 16u;
inline constexpr uint32_t kTraceModeOffset = 20u;
inline constexpr uint32_t kTraceWidthOffset = 21u;
inline constexpr uint32_t kTraceHeightOffset = 22u;

struct BvhTraceState {
  std::atomic_bool stats_requested{false};
};

inline BvhTraceState g_bvh_trace;

inline bool g_trace_failure_logged = false;

inline void LogTraceResourceFailure(const char* stage) {
  if (g_trace_failure_logged) return;
  g_trace_failure_logged = true;
  renodx::utils::log::w("[world-bvh] trace resource creation failed: ", stage);
}

inline bool EnsureBvhTraceResources(reshade::api::device* device, BvhDeviceData* data) {
  if (device == nullptr || data == nullptr || !data->ready) return false;
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
    const uint32_t zeros[kTraceStatsCount + 1u] = {};
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
    uint32_t height) {
  if (device == nullptr || cmd_list == nullptr || data == nullptr) return;
  if (!EnsureBvhTraceResources(device, data)) return;
  if (!data->bvh_ready || data->blas_node_srv.handle == 0u || data->tlas_node_srv.handle == 0u) return;
  if (data->debug_uav.handle == 0u || data->trace_stats_uav.handle == 0u) return;

  const uint32_t zeros[kTraceStatsCount + 1u] = {};
  device->update_buffer_region(zeros, data->trace_stats_buffer, 0u, sizeof(zeros));

  reshade::api::resource_view srvs[kTraceSrvCount] = {
      data->vertex_srv, data->index_srv, data->mesh_srv, data->instance_srv, data->active_srv,
      data->blas_node_srv, data->blas_leaf_srv, data->tlas_node_srv, data->tlas_leaf_srv};
  static_assert(sizeof(srvs) / sizeof(srvs[0]) == kTraceSrvCount, "trace SRV table must have exactly 9 entries");
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
  {
    std::lock_guard<std::mutex> lock(g_state.mutex);
    std::memcpy(constants, g_state.camera.view_proj_inv, sizeof(float) * 16u);
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
  static_assert(kTraceModeOffset + 3u <= kTracePushConstantCount, "trace push constants overflow");
  cmd_list->push_constants(
      reshade::api::shader_stage::all_compute, data->trace_layout, 2, 0, kTracePushConstantCount, constants);
  cmd_list->dispatch((width + 7u) / 8u, (height + 7u) / 8u, 1u);
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
          sizeof(uint32_t) * (kTraceStatsCount + 1u), &bytes)) {
    return false;
  }
  if (bytes.size() < sizeof(uint32_t) * kTraceStatsCount) return false;
  uint32_t values[kTraceStatsCount + 1u] = {};
  std::memcpy(values, bytes.data(), sizeof(uint32_t) * kTraceStatsCount);
  data->trace_stats.rays = values[0];
  data->trace_stats.hits = values[1];
  data->trace_stats.misses = values[2];
  data->trace_stats.invalid_refs = values[3];
  data->trace_stats.stack_overflow = values[4];
  data->trace_stats.triangle_tests = values[5];
  data->trace_stats.max_stack_depth = values[6];
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
      " invariant=", data->trace_stats.invariant_ok ? "ok" : "FAIL");
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
