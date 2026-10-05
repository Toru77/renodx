#pragma once

// Phase 1 world BVH runtime entry.
//
// M1 provides the persistent CPU world pool; M2a uploads it to GPU buffers and
// exposes the reconstruction debug view. The present hook drives both the
// upload/revision handling and the debug pass. M3+ adds BLAS/TLAS here.
//
// The whole module remains DevKit-gated through falcom_world::Use.

#include <Windows.h>
#include <shellapi.h>

#include "bvh_debug.hpp"
#include "bvh_pool.hpp"

namespace falcom_world::bvh {

inline void OnDestroyDeviceBvh(reshade::api::device* device) {
  DestroyBvhDebugResources(device);
  DestroyBvhDeviceData(device);
}

inline void Use(DWORD fdw_reason) {
  switch (fdw_reason) {
    case DLL_PROCESS_ATTACH:
      LoadWorldRecipes();
      reshade::register_event<reshade::addon_event::destroy_device>(OnDestroyDevicePool);
      reshade::register_event<reshade::addon_event::destroy_device>(OnDestroyDeviceBvh);
      reshade::register_event<reshade::addon_event::present>(OnWorldPresentBvh);
      break;
    case DLL_PROCESS_DETACH:
      reshade::unregister_event<reshade::addon_event::destroy_device>(OnDestroyDevicePool);
      reshade::unregister_event<reshade::addon_event::destroy_device>(OnDestroyDeviceBvh);
      reshade::unregister_event<reshade::addon_event::present>(OnWorldPresentBvh);
      break;
    default:
      break;
  }
}

inline void DrawBvhPanel() {
  ImGui::TextDisabled("Phase 1: persistent world pool + GPU reconstruction (runtime_verified structured-instance families).");

  bool scan = g_pool.scan_active.load(std::memory_order_relaxed);
  if (ImGui::Checkbox("Pool Scan", &scan)) {
    g_pool.scan_active.store(scan, std::memory_order_relaxed);
  }
  ImGui::SameLine();
  ImGui::SetNextItemWidth(150.f);
  ImGui::SliderFloat("Region (m)", &g_pool.region_size, kPoolRegionMinSize, kPoolRegionMaxSize, "%.0f");

  PoolStats stats;
  PoolRegion region;
  size_t queued = 0u;
  size_t mesh_queue = 0u;
  float camera[3] = {};
  const bool camera_valid = PoolCameraPosition(camera);
  {
    std::lock_guard<std::mutex> lock(g_pool.mutex);
    UpdatePoolStats();
    stats = g_pool.stats;
    queued = g_pool.pending_snapshots.size();
    mesh_queue = g_pool.mesh_queue.size();
    region = CurrentPoolRegion();
  }

  ImGui::Text("Pending snapshots: %llu  Meshes queued: %llu  Meshes captured: %llu",
              static_cast<unsigned long long>(queued),
              static_cast<unsigned long long>(mesh_queue),
              static_cast<unsigned long long>(stats.meshes));
  ImGui::Text("Observed instances: %llu  Admitted: %llu  In region: %llu",
              static_cast<unsigned long long>(stats.observed),
              static_cast<unsigned long long>(stats.admitted),
              static_cast<unsigned long long>(stats.region));
  ImGui::Text("Skipped unverified: %u  non-instance: %u  satisfied: %u  drops: %u  copy drops: %u  no base: %u  beyond snapshot: %u  mesh failures: %u",
              stats.skipped_unverified,
              stats.skipped_non_instance,
              stats.skipped_satisfied,
              stats.queue_drops,
              stats.copy_drops,
              stats.skipped_no_base,
              stats.skipped_beyond_snapshot,
              stats.mesh_failures);
  ImGui::Text("Gate skips: non-candidate: %u  no SRV: %u  slot invalid: %u  no buffer: %u  indirect drops: %llu",
              stats.skipped_not_candidate,
              stats.skipped_no_srv,
              stats.skipped_slot_invalid,
              stats.skipped_no_buffer,
              static_cast<unsigned long long>(IndirectDroppedCalls()));
  if (camera_valid) {
    ImGui::Text("Camera: %.1f %.1f %.1f   Region cell min: %.0f %.0f %.0f",
                camera[0], camera[1], camera[2],
                region.min[0], region.min[1], region.min[2]);
  } else {
    ImGui::TextDisabled("Camera position unavailable (viewInv not captured yet).");
  }

  bool do_dump = false;
  bool do_dump_obj = false;
  bool do_reset = false;
  if (ImGui::Button("Dump Pool JSON")) do_dump = true;
  ImGui::SameLine();
  if (ImGui::Button("Dump Region OBJ")) do_dump_obj = true;
  ImGui::SameLine();
  if (ImGui::Button("Reset Pool")) do_reset = true;
  ImGui::SameLine();
  if (ImGui::Button("Open Pool Folder")) {
    const auto folder = PoolOutputDir();
    ShellExecuteA(nullptr, "open", folder.string().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
  }
  if (do_dump) DumpWorldPool();
  if (do_dump_obj) DumpWorldPoolObj();
  if (do_reset) ResetWorldPool();

  ImGui::SeparatorText("Recipes");
  if (ImGui::Button("Save Recipes")) SaveWorldRecipes();
  ImGui::SameLine();
  if (ImGui::Button("Load Recipes")) LoadWorldRecipes();
  ImGui::SameLine();
  ImGui::TextDisabled("%s", g_recipes.status.c_str());
  ImGui::TextDisabled("loaded %u  saved %u  saved frame %u",
                      g_recipes.loaded_count,
                      g_recipes.saved_count,
                      g_recipes.saved_frame);

  ImGui::SeparatorText("GPU Reconstruction (M2b/M4)");
  int mode = g_bvh_debug.mode.load(std::memory_order_relaxed);
  const char* modes = "Off\0Instance ID\0Mesh ID\0World Position\0Flat Normal\0Instance AABB\0Shaded\0BVH Trace (Shaded)\0BVH Trace (Instance ID)\0";
  if (ImGui::Combo("GPU Debug", &mode, modes)) {
    g_bvh_debug.mode.store(mode, std::memory_order_relaxed);
    if (mode >= 7) g_bvh_trace.stats_requested.store(true, std::memory_order_relaxed);
  }
  ImGui::SameLine();
  if (ImGui::Button("Upload Pool to GPU")) {
    g_bvh_debug.force_upload.store(true, std::memory_order_relaxed);
  }
  ImGui::SameLine();
  if (ImGui::Button("Trace Stats")) {
    g_bvh_trace.stats_requested.store(true, std::memory_order_relaxed);
  }

  reshade::api::device* device = g_bvh_debug.device.load(std::memory_order_relaxed);
  BvhDeviceData* data = device != nullptr ? GetBvhDeviceData(device) : nullptr;
  if (data != nullptr && data->ready) {
    ImGui::Text("GPU pool: rev %llu  active %u / %u  meshes %u  instances %u  verts %u  indices %u",
                static_cast<unsigned long long>(data->uploaded_revision),
                data->active_count,
                data->instance_count,
                data->mesh_count,
                data->instance_count,
                data->vertex_count,
                data->index_count);
    if (data->checksum_valid) {
      if (data->checksum_match) {
        ImGui::TextColored(ImVec4(0.4f, 1.f, 0.4f, 1.f), "Upload checksum: match");
      } else {
        ImGui::TextColored(ImVec4(1.f, 0.4f, 0.4f, 1.f), "Upload checksum: MISMATCH");
      }
      ImGui::Text("CPU hash %016llX  GPU hash %016llX",
                  static_cast<unsigned long long>(data->cpu_hash),
                  static_cast<unsigned long long>(data->gpu_hash));
    } else {
      ImGui::TextDisabled("Upload checksum: unavailable");
    }
    if (data->bvh_ready) {
      const ImVec4 color = data->blas_valid && data->tlas_valid
                               ? ImVec4(0.4f, 1.f, 0.4f, 1.f)
                               : ImVec4(1.f, 0.4f, 0.4f, 1.f);
      ImGui::TextColored(color, "BVH: %s", data->build_status.c_str());
      ImGui::TextDisabled("built rev %llu  tlas rev %llu  degenerate %u",
                          static_cast<unsigned long long>(data->bvh_built_revision),
                          static_cast<unsigned long long>(data->tlas_built_revision),
                          data->degenerate_leaf_count);
    } else {
      ImGui::TextDisabled("BVH: %s", data->build_status.c_str());
    }
    if (data->trace_stats.valid) {
      const ImVec4 color = data->trace_stats.invariant_ok
                               ? ImVec4(0.4f, 1.f, 0.4f, 1.f)
                               : ImVec4(1.f, 0.4f, 0.4f, 1.f);
      ImGui::TextColored(color, "Trace: rays %u  hits %u  misses %u  (hits+misses=%s)",
                          data->trace_stats.rays,
                          data->trace_stats.hits,
                          data->trace_stats.misses,
                          data->trace_stats.invariant_ok ? "ok" : "FAIL");
      ImGui::TextDisabled("Trace: invalid %u  stack_overflow %u  tri_tests %u  max_depth %u",
                          data->trace_stats.invalid_refs,
                          data->trace_stats.stack_overflow,
                          data->trace_stats.triangle_tests,
                          data->trace_stats.max_stack_depth);
    }
  } else {
    ImGui::TextDisabled("GPU pool not uploaded yet.");
  }
}

}  // namespace falcom_world::bvh
