#pragma once

#include "../bvh/deform_live.hpp"
#include "../rtxdi/rtxdi_input_d3d11.hpp"
#include "rt12_backend.hpp"

namespace falcom_world::rt12 {

// Prepares the shared scene for a future RTXDI consumer; no lighting work is scheduled here.
inline SceneTransactionResult PrepareSceneForRtxdi(reshade::api::device* device, reshade::api::command_list* cmd_list,
                                                   reshade::api::resource_view scene_cbv_view, bool trace_characters) {
  if (!g_rt12_enabled.load(std::memory_order_relaxed) || !g_rt12 || g_rt12->device != device
      || g_rt12->failed || g_rt12->stuck || g_rt12->unfenced_execution || cmd_list == nullptr || scene_cbv_view.handle == 0u) return {};
  if (reinterpret_cast<ID3D11DeviceContext*>(cmd_list->get_native()) != g_rt12->ctx11.Get()) return {};  // NOLINT(performance-no-int-to-ptr)
  auto* data = bvh::GetBvhDeviceData(device);
  const bool gc = GcOwnsRtao();
  if (data == nullptr || (gc && (!g_rt12_gc_scene.ready || g_rt12_gc_scene.version == 0u))
      || (!gc && (!data->tlas_built || g_rt12->mirror_dropped || bvh::g_gpu_capture.pool_reduced.load(std::memory_order_relaxed)))) {
    return {.reason = Rt12Reason::TlasEmpty};
  }
  const uint32_t records = (gc ? g_rt12_gc_scene.bound : data->dynamic.instance_count);
  if (!gc && records > 0u && (records > bvh::kPoolDynamicInstances || data->dynamic.instances_srv.handle == 0u || !data->dynamic.ready)) {
    return {.reason = Rt12Reason::Records};
  }
  uint32_t characters = 0u;
  if (trace_characters) characters = bvh::RefitDeformLiveCharacters(device, cmd_list).count;
  const uint32_t frame = g_state.frame.load();
  const uint64_t snapshot = bvh::DeformLightingSnapshot(frame);
  Rt12State& s = *g_rt12;
  const bool slot_valid = s.scene_slot >= 0 && static_cast<uint32_t>(s.scene_slot) < kRt12Slots;
  if (rtxdi::CanReuseScene({.healthy = !s.failed && !s.stuck && !s.unfenced_execution,
                            .slot_valid = slot_valid,
                            .tlas_valid = slot_valid && s.lighting_slots[s.scene_slot].tlas != nullptr,
                            .gc = gc,
                            .scene_gc = s.scene_gc,
                            .frame = frame,
                            .scene_frame = s.scene_frame,
                            .records = records,
                            .scene_records = s.scene_record_count,
                            .characters = characters,
                            .scene_characters = s.scene_dynamic_count,
                            .generation = s.generation,
                            .scene_generation = s.scene_generation,
                            .snapshot = snapshot,
                            .scene_snapshot = s.scene_character_snapshot,
                            .gc_version = (gc ? g_rt12_gc_scene.version : 0u),
                            .scene_gc_version = s.scene_gc_version})
      && rtxdi::SceneCoverageSnapshotMatches(s.scene_slot, s.scene_frame, s.scene_generation, s.scene_fence,
                                              (gc ? g_rt12_gc_scene.version : 0u), s.scene_coverage_slot,
                                              s.scene_coverage_frame, s.scene_coverage_generation,
                                              s.scene_coverage_fence, s.scene_coverage_gc_version)) {
    return {.scene_ready = true, .reused = true, .frame = frame, .slot = s.scene_slot, .fence_b = s.scene_fence};
  }
  return RunSceneTransaction(device, cmd_list,
                             {.scene_cbv_view = scene_cbv_view, .records = records, .characters = characters, .character_snapshot = snapshot, .use_gpu_cache = gc, .frame = frame, .generation = s.generation}, nullptr);
}

}  // namespace falcom_world::rt12
