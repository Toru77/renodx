#pragma once

// Phase 1 world BVH runtime entry.
//
// The pool admits geometry by bytecode shader class (bvh_pool.hpp); M2a
// uploads it to GPU buffers and exposes the reconstruction debug view. The
// present hook drives both the upload/revision handling and the debug pass.
// M3+ adds BLAS/TLAS here.
//
// The whole module remains DevKit-gated through falcom_world::Use.

#include <Windows.h>
#include <shellapi.h>

#include <string>

#include "../contract/shader_registry.hpp"
#include "../debug/crash_log.hpp"
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
      reshade::register_event<reshade::addon_event::destroy_device>(OnDestroyDevicePool);
      reshade::register_event<reshade::addon_event::destroy_resource>(OnDestroyResourcePool);
      reshade::register_event<reshade::addon_event::destroy_device>(OnDestroyDeviceBvh);
      reshade::register_event<reshade::addon_event::present>(OnWorldPresentBvh);
      break;
    case DLL_PROCESS_DETACH:
      reshade::unregister_event<reshade::addon_event::destroy_device>(OnDestroyDevicePool);
      reshade::unregister_event<reshade::addon_event::destroy_resource>(OnDestroyResourcePool);
      reshade::unregister_event<reshade::addon_event::destroy_device>(OnDestroyDeviceBvh);
      reshade::unregister_event<reshade::addon_event::present>(OnWorldPresentBvh);
      break;
    default:
      break;
  }
}

// Appends "label count" pairs for non-zero counts, so long breakdowns stay
// readable in the panel.
inline void AppendPoolCount(std::string* text, const char* label, uint64_t count) {
  if (count == 0u) return;
  if (!text->empty()) text->append("  ");
  text->append(label);
  text->push_back(' ');
  text->append(std::to_string(count));
}

inline float PoolPercent(uint64_t part, uint64_t total) {
  return total == 0u ? 0.f : static_cast<float>(static_cast<double>(part) * 100.0 / static_cast<double>(total));
}

inline void DrawDepthCompareStats(const BvhTraceStats& stats) {
  const uint64_t judged = static_cast<uint64_t>(stats.compare_match) + stats.compare_missing
                          + stats.compare_extra + stats.compare_extra_sky;
  if (judged == 0u && stats.compare_no_depth == 0u && stats.compare_far == 0u && stats.compare_sky == 0u) return;
  if (judged == 0u) {
    ImGui::TextColored(ImVec4(1.f, 0.4f, 0.4f, 1.f),
                       "Depth compare: no pixel judged (no game depth %u, far %u, sky %u)",
                       stats.compare_no_depth, stats.compare_far, stats.compare_sky);
    return;
  }
  ImGui::Text("Depth compare (within %.0f m): match %.1f%%  missing %.1f%%  extra %.1f%%  extra on sky %.1f%%",
              g_pool.region_size * 0.25f,
              PoolPercent(stats.compare_match, judged),
              PoolPercent(stats.compare_missing, judged),
              PoolPercent(stats.compare_extra, judged),
              PoolPercent(stats.compare_extra_sky, judged));
  ImGui::TextDisabled("pixels: match %u  missing %u  extra %u  extra on sky %u  far %u  sky %u  no depth %u",
                      stats.compare_match, stats.compare_missing, stats.compare_extra, stats.compare_extra_sky,
                      stats.compare_far, stats.compare_sky, stats.compare_no_depth);
}

inline void DrawBvhPanel() {
  ImGui::TextDisabled("Pool admits draws whose vertex shader the bytecode classifier marks rigid and whose pixel shader does not alpha-test.");

  bool scan = g_pool.scan_active.load(std::memory_order_relaxed);
  if (ImGui::Checkbox("Pool Scan", &scan)) {
    g_pool.scan_active.store(scan, std::memory_order_relaxed);
    RefreshPoolCaptureRequest();
    LogPoolSwitches();
  }
  ImGui::SameLine();
  ImGui::SetNextItemWidth(150.f);
  ImGui::SliderFloat("Region (m)", &g_pool.region_size, kPoolRegionMinSize, kPoolRegionMaxSize, "%.0f");

  // Diagnostic switches: turn one pool stage off at a time to find which one
  // a crash needs. Logging only happens while "Log mesh captures" is on.
  bool capture_meshes = g_pool.capture_meshes.load(std::memory_order_relaxed);
  bool scan_indirect = g_pool.scan_indirect.load(std::memory_order_relaxed);
  bool log_captures = g_pool.log_captures.load(std::memory_order_relaxed);
  bool switches_changed = false;
  if (ImGui::Checkbox("Capture meshes", &capture_meshes)) {
    g_pool.capture_meshes.store(capture_meshes, std::memory_order_relaxed);
    switches_changed = true;
  }
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("Copies the index, then the vertex range of each new mesh at its draws and reads them\n"
                      "two frames later (no GPU wait). Off: no new mesh is read; instances of meshes\n"
                      "not read yet wait unadmitted.");
  }
  ImGui::SameLine();
  if (ImGui::Checkbox("Scan indirect draws", &scan_indirect)) {
    g_pool.scan_indirect.store(scan_indirect, std::memory_order_relaxed);
    switches_changed = true;
  }
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("Off: indirect draws (counts kept on the GPU) are ignored by the pool.");
  }
  ImGui::SameLine();
  if (ImGui::Checkbox("Log mesh captures", &log_captures)) {
    g_pool.log_captures.store(log_captures, std::memory_order_relaxed);
    switches_changed = true;
  }
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("Writes ReShade.log lines for each mesh copy and read,\n"
                      "and when buffers the pool tracks are released.");
  }
  bool log_crashes = CrashLogEnabled();
  if (ImGui::Checkbox("Log crashes", &log_crashes)) SetCrashLogEnabled(log_crashes);
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("On a crash, writes the error, the pool stage and the call stack to ReShade.log.\n"
                      "The game still crashes as before.");
  }
  if (switches_changed) LogPoolSwitches();

  const contract::RegistryCounts registry = contract::SnapshotRegistryCounts();
  PoolStats stats;
  PoolRegion region;
  float camera[3] = {};
  const bool camera_valid = PoolCameraPosition(camera);
  {
    std::lock_guard<std::mutex> lock(g_pool.mutex);
    UpdatePoolStats();
    stats = g_pool.stats;
    region = CurrentPoolRegion();
  }

  std::string vertex_classes;
  for (size_t i = 0; i < registry.vertex.size(); ++i) {
    AppendPoolCount(&vertex_classes, contract::VsClassName(static_cast<contract::VsClass>(i)), registry.vertex[i]);
  }
  std::string pixel_classes;
  for (size_t i = 0; i < registry.pixel.size(); ++i) {
    AppendPoolCount(&pixel_classes, contract::PsClassName(static_cast<contract::PsClass>(i)), registry.pixel[i]);
  }
  ImGui::Text("Vertex shaders: %s", vertex_classes.empty() ? "none yet" : vertex_classes.c_str());
  ImGui::Text("Pixel shaders: %s", pixel_classes.empty() ? "none yet" : pixel_classes.c_str());

  std::string draw_classes;
  for (size_t i = 0; i < stats.draws_by_vs_class.size(); ++i) {
    AppendPoolCount(&draw_classes, contract::VsClassName(static_cast<contract::VsClass>(i)), stats.draws_by_vs_class[i]);
  }
  std::string skips;
  for (size_t i = static_cast<size_t>(PoolSkip::DrawState); i < stats.skips.size(); ++i) {
    AppendPoolCount(&skips, PoolSkipName(static_cast<PoolSkip>(i)), stats.skips[i]);
  }
  std::string draw_state;
  for (size_t i = 1; i < stats.draw_state.size(); ++i) {
    AppendPoolCount(&draw_state, PoolDrawStateName(static_cast<PoolDrawState>(i)), stats.draw_state[i]);
  }
  std::string indirect_classes;
  for (size_t i = 0; i < stats.indirect_by_vs_class.size(); ++i) {
    AppendPoolCount(&indirect_classes, contract::VsClassName(static_cast<contract::VsClass>(i)), stats.indirect_by_vs_class[i]);
  }
  ImGui::Text("Draws scanned: %s", draw_classes.empty() ? "none" : draw_classes.c_str());
  ImGui::Text("Rigid draws skipped: %s", skips.empty() ? "none" : skips.c_str());
  ImGui::Text("Draw state reasons: %s", draw_state.empty() ? "none" : draw_state.c_str());
  ImGui::Text("Indirect draws: %s", indirect_classes.empty() ? "none" : indirect_classes.c_str());
  {
    uint64_t scanned = 0u;
    for (const uint64_t count : stats.draws_by_vs_class) scanned += count;
    uint64_t indirect = 0u;
    for (const uint64_t count : stats.indirect_by_vs_class) indirect += count;
    ImGui::Text("On deferred contexts: draws %llu of %llu  indirect %llu of %llu",
                static_cast<unsigned long long>(stats.draws_on_deferred),
                static_cast<unsigned long long>(scanned),
                static_cast<unsigned long long>(stats.indirect_on_deferred),
                static_cast<unsigned long long>(indirect));
  }
  if (!indirect_classes.empty()) {
    ImGui::TextDisabled("Indirect: copied %llu  read %llu  empty %llu  truncated %llu  buffer released %llu  avg window %.0f",
                        static_cast<unsigned long long>(stats.indirect_copied),
                        static_cast<unsigned long long>(stats.indirect_resolved),
                        static_cast<unsigned long long>(stats.indirect_empty),
                        static_cast<unsigned long long>(stats.indirect_truncated),
                        static_cast<unsigned long long>(stats.indirect_dead),
                        stats.indirect_copied != 0u
                            ? static_cast<double>(stats.indirect_window_instances) / static_cast<double>(stats.indirect_copied)
                            : 0.0);
  }

  const ImVec4 base_color = stats.base_mismatch == 0u ? ImVec4(0.4f, 1.f, 0.4f, 1.f) : ImVec4(1.f, 0.4f, 0.4f, 1.f);
  ImGui::Text("Copied draws: %llu", static_cast<unsigned long long>(stats.copied_draws));
  ImGui::SameLine();
  ImGui::TextColored(base_color, "b1 verified %llu  mismatch %llu",
                     static_cast<unsigned long long>(stats.base_verified),
                     static_cast<unsigned long long>(stats.base_mismatch));
  if (stats.base_mismatch != 0u) {
    ImGui::SameLine();
    ImGui::TextDisabled("(last cpu %d gpu %d)", stats.last_mismatch_cpu, stats.last_mismatch_gpu);
  }
  if (stats.map_failures != 0u || stats.staging_failures != 0u) {
    ImGui::TextColored(ImVec4(1.f, 0.4f, 0.4f, 1.f), "Readback map failures %u  staging failures %u",
                       stats.map_failures, stats.staging_failures);
  }
  ImGui::Text("Instances: seen %llu  observed %llu  admitted %llu  in region %llu",
              static_cast<unsigned long long>(stats.instances_seen),
              static_cast<unsigned long long>(stats.observed),
              static_cast<unsigned long long>(stats.admitted),
              static_cast<unsigned long long>(stats.region));
  ImGui::Text("Instance rejects: bad matrix %llu  bad bounds %u  duplicates %u  cap %u   (moving at draw: %llu)",
              static_cast<unsigned long long>(stats.rejected_matrix),
              stats.rejected_bounds,
              stats.dedup_instances,
              stats.instance_cap_drops,
              static_cast<unsigned long long>(stats.moving_instances));
  std::string matrix_rejects;
  for (size_t i = 1; i < stats.matrix_rejects.size(); ++i) {
    AppendPoolCount(&matrix_rejects, PoolMatrixRejectName(static_cast<PoolMatrixReject>(i)), stats.matrix_rejects[i]);
  }
  if (!matrix_rejects.empty()) ImGui::Text("Bad matrix reasons: %s", matrix_rejects.c_str());
  ImGui::Text("Near misses (same mesh, almost the same matrix): %llu",
              static_cast<unsigned long long>(stats.near_misses));
  ImGui::Text("Meshes: captured %llu  waiting %llu  in flight %llu  same-content merges %u  failures %u  cap %u",
              static_cast<unsigned long long>(stats.meshes),
              static_cast<unsigned long long>(stats.mesh_queue),
              static_cast<unsigned long long>(stats.mesh_in_flight),
              stats.mesh_dedup,
              stats.mesh_failures,
              stats.mesh_cap_drops);
  ImGui::TextDisabled("Mesh copies: indices %llu  vertices %llu  (%.1f MB)  postponed (staging full) %u  "
                      "dropped (buffer released) %u  expired %u",
                      static_cast<unsigned long long>(stats.mesh_index_copies),
                      static_cast<unsigned long long>(stats.mesh_vertex_copies),
                      static_cast<double>(stats.mesh_copy_bytes) / (1024.0 * 1024.0),
                      stats.mesh_budget_full,
                      stats.mesh_dropped,
                      stats.mesh_expired);
  if (stats.mesh_deferred_skips != 0u) {
    ImGui::TextColored(ImVec4(1.f, 0.8f, 0.3f, 1.f),
                       "Mesh copies not taken at deferred-context draws: %llu (meshes drawn only there wait)",
                       static_cast<unsigned long long>(stats.mesh_deferred_skips));
  }
  ImGui::Text("Retired by buffer release: meshes %u  instances %u  (buffer events %u)",
              stats.meshes_retired,
              stats.instances_retired,
              stats.resource_invalidations);
  if (!stats.last_mesh_error.empty()) {
    ImGui::TextDisabled("Last mesh capture error: %s", stats.last_mesh_error.c_str());
  }
  if (camera_valid) {
    ImGui::Text("Camera: %.1f %.1f %.1f   Region cell min: %.0f %.0f %.0f",
                camera[0], camera[1], camera[2],
                region.min[0], region.min[1], region.min[2]);
  } else {
    ImGui::TextDisabled("Camera position unavailable (lighting pass not seen yet).");
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

  ImGui::SeparatorText("GPU BVH (trace views)");
  int mode = g_bvh_debug.mode.load(std::memory_order_relaxed);
  const char* modes = "Off\0BVH Trace (Shaded)\0BVH Trace (Instance ID)\0Depth Compare\0";
  if (ImGui::Combo("GPU Debug", &mode, modes)) {
    g_bvh_debug.mode.store(mode, std::memory_order_relaxed);
    RefreshPoolCaptureRequest();
    if (mode != static_cast<int>(BvhView::Off)) g_bvh_trace.stats_requested.store(true, std::memory_order_relaxed);
  }
  ImGui::SameLine();
  if (ImGui::Button("Upload Pool to GPU")) {
    g_bvh_debug.force_upload.store(true, std::memory_order_relaxed);
  }
  ImGui::SameLine();
  if (ImGui::Button("Read Stats")) {
    g_bvh_trace.stats_requested.store(true, std::memory_order_relaxed);
  }
  if (mode == static_cast<int>(BvhView::DepthCompare)) {
    ImGui::TextDisabled("green match  blue missing (game surface in front / no BVH hit)  red extra (BVH in front)  orange BVH on game sky  grey beyond range");
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
      DrawDepthCompareStats(data->trace_stats);
    }
  } else {
    ImGui::TextDisabled("GPU pool not uploaded yet.");
  }
}

}  // namespace falcom_world::bvh
