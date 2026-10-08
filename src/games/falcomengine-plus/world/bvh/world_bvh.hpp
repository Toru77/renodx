#pragma once

// Phase 1 world BVH runtime entry.
//
// The pool admits geometry by bytecode shader class (bvh_pool.hpp); the live
// BVH (bvh_live.hpp) keeps the GPU mesh store and region TLAS in step with it
// at every present, and the trace debug views (bvh_debug.hpp) read them. This
// file registers the events and draws the panel.
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
      reshade::register_event<reshade::addon_event::update_buffer_region>(OnUpdateBufferRegionPool);
      reshade::register_event<reshade::addon_event::update_buffer_region_command>(OnUpdateBufferRegionCommandPool);
      reshade::register_event<reshade::addon_event::map_buffer_region>(OnMapBufferRegionPool);
      reshade::register_event<reshade::addon_event::copy_buffer_region>(OnCopyBufferRegionPool);
      reshade::register_event<reshade::addon_event::copy_resource>(OnCopyResourcePool);
      reshade::register_event<reshade::addon_event::destroy_device>(OnDestroyDeviceBvh);
      reshade::register_event<reshade::addon_event::present>(OnWorldPresentBvh);
      reshade::register_event<reshade::addon_event::destroy_pipeline>(OnDestroyPipelineDeform);
      reshade::register_event<reshade::addon_event::destroy_device>(OnDestroyDeviceDeform);
      reshade::register_event<reshade::addon_event::destroy_pipeline>(OnDestroyPipelineDeformLive);
      reshade::register_event<reshade::addon_event::destroy_device>(OnDestroyDeviceDeformLive);
      break;
    case DLL_PROCESS_DETACH:
      reshade::unregister_event<reshade::addon_event::destroy_device>(OnDestroyDevicePool);
      reshade::unregister_event<reshade::addon_event::destroy_resource>(OnDestroyResourcePool);
      reshade::unregister_event<reshade::addon_event::update_buffer_region>(OnUpdateBufferRegionPool);
      reshade::unregister_event<reshade::addon_event::update_buffer_region_command>(OnUpdateBufferRegionCommandPool);
      reshade::unregister_event<reshade::addon_event::map_buffer_region>(OnMapBufferRegionPool);
      reshade::unregister_event<reshade::addon_event::copy_buffer_region>(OnCopyBufferRegionPool);
      reshade::unregister_event<reshade::addon_event::copy_resource>(OnCopyResourcePool);
      reshade::unregister_event<reshade::addon_event::destroy_device>(OnDestroyDeviceBvh);
      reshade::unregister_event<reshade::addon_event::present>(OnWorldPresentBvh);
      reshade::unregister_event<reshade::addon_event::destroy_pipeline>(OnDestroyPipelineDeform);
      reshade::unregister_event<reshade::addon_event::destroy_device>(OnDestroyDeviceDeform);
      reshade::unregister_event<reshade::addon_event::destroy_pipeline>(OnDestroyPipelineDeformLive);
      reshade::unregister_event<reshade::addon_event::destroy_device>(OnDestroyDeviceDeformLive);
      break;
    default:
      break;
  }
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
}

// Deforming meshes in the BVH (deform_live.hpp): the checkbox and one status line.
inline void DrawDeformLivePanel(const DeformLiveStats& stats, uint32_t traced) {
  bool enabled = g_deform_live.enabled.load(std::memory_order_relaxed);
  if (ImGui::Checkbox("Deforming meshes in the BVH (characters, water)", &enabled)) {
    g_deform_live.enabled.store(enabled, std::memory_order_relaxed);
    g_deform.enabled.store(enabled, std::memory_order_relaxed);  // confirms each shader's world position
    RefreshPoolCaptureRequest();
  }
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("Every frame, re-runs each camera draw of a skinned or animated vertex shader whose world\n"
                      "position the probe confirmed, capturing its world-space triangles on the GPU. Each draw's\n"
                      "tree is built once from its first capture and refit every frame; the trace walks them\n"
                      "beside the static BVH. The probe (it confirms new shaders) is on while this is on.");
  }
  if (!enabled) return;
  ImGui::Text("Deforming: traced %u of %u draws, %u waiting for tree, %u failed",
              traced, stats.frame_captures, stats.frame_pending, stats.failed);
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

  bool switches_changed = false;
  bool follow_moving = g_pool.follow_moving.load(std::memory_order_relaxed);
  if (ImGui::Checkbox("Moving objects follow their pose", &follow_moving)) {
    g_pool.follow_moving.store(follow_moving, std::memory_order_relaxed);
    switches_changed = true;
  }
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("Moving objects keep their instances and each one moves to the pose the object\n"
                      "has now (a copy of a moving object is taken every frame). Off: the old behaviour.\n"
                      "Ignored while \"Keep moving objects out\" is on.");
  }

  bool do_dump = false;
  bool do_reset = false;
  if (ImGui::Button("Dump Pool JSON")) do_dump = true;
  ImGui::SameLine();
  if (ImGui::Button("Reset Pool")) do_reset = true;
  ImGui::SameLine();
  if (ImGui::Button("Open Pool Folder")) {
    const auto folder = PoolOutputDir();
    ShellExecuteA(nullptr, "open", folder.string().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
  }

  bool do_dump_obj = false;
  bool do_trace = false;
  if (ImGui::CollapsingHeader("Advanced")) {
    // Diagnostic switches: turn one pool stage off at a time to find which one
    // a crash needs. Logging only happens while "Log mesh captures" is on.
    bool capture_meshes = g_pool.capture_meshes.load(std::memory_order_relaxed);
    bool scan_indirect = g_pool.scan_indirect.load(std::memory_order_relaxed);
    bool log_captures = g_pool.log_captures.load(std::memory_order_relaxed);
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
    bool verify_meshes = g_pool.verify_meshes.load(std::memory_order_relaxed);
    bool legacy_scale = g_pool.legacy_scale.load(std::memory_order_relaxed);
    if (ImGui::Checkbox("Verify mesh captures", &verify_meshes)) {
      g_pool.verify_meshes.store(verify_meshes, std::memory_order_relaxed);
      switches_changed = true;
    }
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("A mesh enters the pool only after two separate captures decode to the same mesh.\n"
                        "Differing captures are counted and written to ReShade.log; a mesh that never\n"
                        "repeats is rejected. Applies to captures from now on: reset the pool after changing.");
    }
    ImGui::SameLine();
    if (ImGui::Checkbox("Legacy instance scale limits (0.05-50)", &legacy_scale)) {
      g_pool.legacy_scale.store(legacy_scale, std::memory_order_relaxed);
      switches_changed = true;
    }
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("Rejects instances with an axis scale outside 0.05-50 (the limits before the\n"
                        "draw-time capture rework) instead of 0.001-10000. For an A/B test: reset the pool\n"
                        "after changing.");
    }
    bool poison_staging = g_pool.poison_staging.load(std::memory_order_relaxed);
    if (ImGui::Checkbox("Poison mesh staging (diagnostic)", &poison_staging)) {
      g_pool.poison_staging.store(poison_staging, std::memory_order_relaxed);
      switches_changed = true;
    }
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("Mesh staging is cpu-visible and filled with a poison word after each read; each mesh\n"
                        "copy then records its head bytes and where they came from (ReShade.log \"staging:\" and\n"
                        "world_pool.json). Diagnostic only: reset the pool after changing.");
    }
    bool retry_unstable = g_pool.retry_unstable.load(std::memory_order_relaxed);
    if (ImGui::Checkbox("Retry unstable meshes (diagnostic)", &retry_unstable)) {
      g_pool.retry_unstable.store(retry_unstable, std::memory_order_relaxed);
      switches_changed = true;
    }
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("Off (default): a mesh whose captures never repeat is rejected at once.\n"
                        "On: it is captured again up to 3 times, 180 frames apart. Applies to captures\n"
                        "from now on: reset the pool after changing.");
    }
    bool exclude_moving = g_pool.exclude_moving.load(std::memory_order_relaxed);
    if (ImGui::Checkbox("Keep moving objects out of the static BVH", &exclude_moving)) {
      g_pool.exclude_moving.store(exclude_moving, std::memory_order_relaxed);
      switches_changed = true;
    }
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("A mesh seen moving in a camera view (its prevWorld differs from world) gets no\n"
                        "static instances: those it had are removed and new ones refused, so a door or cart\n"
                        "leaves no copy behind where it stopped. Until the per-frame path exists, moving\n"
                        "objects are absent from the BVH. Off: admitted as before (applies at once).");
    }
    bool alpha_foliage = g_pool.alpha_foliage.load(std::memory_order_relaxed);
    if (ImGui::Checkbox("Alpha-tested foliage (experimental)", &alpha_foliage)) {
      g_pool.alpha_foliage.store(alpha_foliage, std::memory_order_relaxed);
      switches_changed = true;
    }
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("Experimental, session only, default off. Off: alpha-tested meshes are not traced and their "
                        "instances leave the BVH at once. On: their cutouts are traced through a 256-slice atlas, filled "
                        "at the next present (copies of the source textures are made when drawn).");
    }
    bool log_crashes = CrashLogEnabled();
    if (ImGui::Checkbox("Log crashes", &log_crashes)) SetCrashLogEnabled(log_crashes);
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("On a crash, writes the error, the pool stage and the call stack to ReShade.log.\n"
                        "The game still crashes as before.");
    }

    if (ImGui::Button("Dump Deform JSON")) DumpDeformProbe();
    ImGui::SameLine();
    if (ImGui::Button("Dump Region OBJ")) do_dump_obj = true;
    ImGui::SameLine();
    if (ImGui::Button("Trace prevWorld (3 keys)")) do_trace = true;
    if (do_trace) {
      std::lock_guard<std::mutex> lock(g_pool.mutex);
      g_pool.prev_trace = {};
      g_pool.prev_trace.armed_frame = g_state.frame.load();
    }
    {
      // prevWorld trace: read under the lock, formatted outside it.
      uint32_t armed_frame = 0u;
      bool written = false;
      std::array<PoolPrevTrace::Key, kPoolTraceKeys> keys = {};
      {
        std::lock_guard<std::mutex> lock(g_pool.mutex);
        armed_frame = g_pool.prev_trace.armed_frame;
        written = g_pool.prev_trace.written;
        keys = g_pool.prev_trace.keys;
      }
      std::string line = "prevWorld trace:";
      for (size_t k = 0; k < keys.size(); ++k) {
        const PoolPrevTrace::Key& key = keys[k];
        const char role = static_cast<char>('A' + k);
        if (key.state == PoolPrevTrace::KeyState::Waiting) {
          line += std::string("  ") + role + " waiting";
        } else if (key.state == PoolPrevTrace::KeyState::GaveUp) {
          line += "  " + std::string(1, role) + " " + std::to_string(key.mesh_key) + " (VS " + PoolHashText(key.vs_hash)
                  + ") gave up (" + std::to_string(key.frames_recorded) + "/" + std::to_string(kPoolTraceFrames) + ")";
        } else {
          line += "  " + std::string(1, role) + " " + std::to_string(key.mesh_key) + " (VS " + PoolHashText(key.vs_hash)
                  + ") " + std::to_string(key.frames_recorded) + "/" + std::to_string(kPoolTraceFrames);
        }
      }
      if (armed_frame == 0u) {
        ImGui::TextDisabled("%s off (press Trace prevWorld)", line.c_str());
      } else {
        ImGui::Text("%s  [armed f%u]%s", line.c_str(), armed_frame,
                    written ? "  written: world_prev_trace.json" : "");
      }
    }
    if (ImGui::Button("Check GPU Contents")) g_live_bvh.check_requested.store(true, std::memory_order_relaxed);
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("Reads the GPU BVH back and compares it with what was uploaded.\n"
                        "Waits for the GPU: expect a short hitch.");
    }
    ImGui::SameLine();
    if (ImGui::Button("Read Stats")) {
      g_bvh_trace.stats_requested.store(true, std::memory_order_relaxed);
    }
  }
  if (switches_changed) LogPoolSwitches();
  if (do_dump) {
    DumpWorldPool();
    DumpDeformLive();
  }
  if (do_dump_obj) DumpWorldPoolObj();
  if (do_reset) ResetWorldPool();

  PoolStats stats;
  {
    std::lock_guard<std::mutex> lock(g_pool.mutex);
    UpdatePoolStats(PoolNewestSeenByKey());
    stats = g_pool.stats;
  }
  DeformLiveStats deform;
  uint32_t deform_traced = 0u;
  float deform_refit_ms = -1.f;
  {
    std::lock_guard<std::mutex> lock(g_deform_live.mutex);
    deform = g_deform_live.stats;
    deform_traced = g_deform_live.object_count;
    deform_refit_ms = g_deform_live.refit_timer.last_ms;
  }

  if (stats.mesh_failures != 0u) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.4f, 0.4f, 1.f));
  ImGui::Text("Meshes: %llu ok (%u admitted after a retry), %llu failed (%u unstable), %llu waiting, %llu retrying",
              static_cast<unsigned long long>(stats.meshes), stats.meshes_admitted_by_retry,
              static_cast<unsigned long long>(stats.mesh_failures),
              stats.mesh_unstable, static_cast<unsigned long long>(stats.mesh_queue), static_cast<unsigned long long>(stats.mesh_retry));
  if (stats.mesh_failures != 0u) ImGui::PopStyleColor();
  ImGui::Text("Instances: %llu admitted, %llu in region, following %llu, orphans retired %llu",
              static_cast<unsigned long long>(stats.admitted), static_cast<unsigned long long>(stats.region),
              static_cast<unsigned long long>(stats.following), static_cast<unsigned long long>(stats.orphans_retired));
  ImGui::Text("Alpha foliage %s: draws %llu, conflicts %llu, meshes conflicted %llu, refused off %llu, conflict refused %llu, no UVs %llu, removed %llu",
              g_pool.alpha_foliage.load(std::memory_order_relaxed) ? "on" : "off", static_cast<unsigned long long>(stats.alpha_draws),
              static_cast<unsigned long long>(stats.alpha_conflicts), static_cast<unsigned long long>(stats.alpha_meshes_conflicted),
              static_cast<unsigned long long>(stats.alpha_refused_off), static_cast<unsigned long long>(stats.alpha_conflict_refused),
              static_cast<unsigned long long>(stats.alpha_no_uv), static_cast<unsigned long long>(stats.alpha_removed));
  PoolAlphaGpuStats gpu;
  {
    std::lock_guard<std::mutex> lock(g_pool.mutex);
    gpu = g_pool.alpha_gpu;
  }
  ImGui::Text("Alpha GPU: slices %u/%u, blits %llu (%u this present), copies %llu, proxies %u (%.1f MB), source refused %llu (bytes %llu, format %llu), requeues %llu, cap refused %llu",
              gpu.slices_used, kAlphaAtlasSlices, static_cast<unsigned long long>(gpu.blits), gpu.blits_frame,
              static_cast<unsigned long long>(stats.alpha_source_copies), gpu.proxies, gpu.proxy_bytes / (1024.0 * 1024.0),
              static_cast<unsigned long long>(stats.alpha_source_refused), static_cast<unsigned long long>(stats.alpha_source_refused_bytes),
              static_cast<unsigned long long>(stats.alpha_source_refused_format), static_cast<unsigned long long>(stats.alpha_uv_requeues),
              static_cast<unsigned long long>(gpu.cap_refused));
  ImGui::Text("Alpha TLAS: instances %u, waiting %u; trace tests %llu, cut %llu; blit %.3f ms, trace %.3f ms",
              gpu.tlas_instances, gpu.waiting, static_cast<unsigned long long>(gpu.tests), static_cast<unsigned long long>(gpu.cut),
              gpu.blit_ms, gpu.trace_ms);
  DrawDeformLivePanel(deform, deform_traced);

  ImGui::SeparatorText("GPU BVH (live)");
  bool live = g_live_bvh.enabled.load(std::memory_order_relaxed);
  if (ImGui::Checkbox("Live BVH", &live)) g_live_bvh.enabled.store(live, std::memory_order_relaxed);
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("Keeps the GPU BVH in step with the pool every frame: new meshes stream in with their BLAS\n"
                      "built on the CPU (%llu triangles per frame), and the region TLAS is rebuilt when instances,\n"
                      "meshes or the region change (at most every %u frames). Off: the GPU BVH stays as it is.",
                      static_cast<unsigned long long>(kLiveTrianglesPerFrame), kLiveTlasInterval);
  }
  ImGui::SameLine();
  if (ImGui::Button("Rebuild BVH")) g_live_bvh.reset_requested.store(true, std::memory_order_relaxed);
  if (ImGui::IsItemHovered()) ImGui::SetTooltip("Drops every GPU mesh and streams the pool in again.");

  int mode = g_bvh_debug.mode.load(std::memory_order_relaxed);
  const char* modes = "Off\0BVH Trace (Shaded)\0BVH Trace (Instance ID)\0Depth Compare\0";
  if (ImGui::Combo("GPU Debug", &mode, modes)) {
    g_bvh_debug.mode.store(mode, std::memory_order_relaxed);
    RefreshPoolCaptureRequest();
    if (mode != static_cast<int>(BvhView::Off)) g_bvh_trace.stats_requested.store(true, std::memory_order_relaxed);
  }
  bool hiding = g_bvh_trace.hide_camera_hidden.load(std::memory_order_relaxed);
  if (ImGui::Checkbox("Hide what the game camera does not show", &hiding)) {
    g_bvh_trace.hide_camera_hidden.store(hiding, std::memory_order_relaxed);
    g_bvh_trace.stats_requested.store(true, std::memory_order_relaxed);
  }
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("Some BVH instances are never drawn by a camera vertex shader, only into the shadow maps\n"
                      "(shadow-only casters), and the game near-fades map objects very close to the camera.\n"
                      "Both stay in the BVH (shadow casters are wanted for ray-traced shadows).\n"
                      "On: the trace views skip them the way the game camera does.\n"
                      "Off: they stay visible and are counted as hidden-from-camera pixels.");
  }
  if (mode == static_cast<int>(BvhView::DepthCompare)) {
    ImGui::TextDisabled("green match  blue missing (game surface in front / no BVH hit)  red extra (BVH in front)  orange BVH on game sky  grey beyond range");
  }
  // Inspect: middle-click anywhere while a trace view is on.
  if (mode != static_cast<int>(BvhView::Off) && ImGui::IsMouseClicked(ImGuiMouseButton_Middle)) {
    const ImGuiIO& io = ImGui::GetIO();
    if (io.DisplaySize.x > 0.f && io.DisplaySize.y > 0.f) {
      g_bvh_trace.inspect_u.store(io.MousePos.x / io.DisplaySize.x, std::memory_order_relaxed);
      g_bvh_trace.inspect_v.store(io.MousePos.y / io.DisplaySize.y, std::memory_order_relaxed);
      g_bvh_trace.inspect_pending.store(true, std::memory_order_relaxed);
      g_bvh_trace.stats_requested.store(true, std::memory_order_relaxed);
    }
  }

  reshade::api::device* device = g_bvh_debug.device.load(std::memory_order_relaxed);
  BvhDeviceData* data = device != nullptr ? GetBvhDeviceData(device) : nullptr;
  if (data == nullptr) {
    ImGui::TextDisabled("No device yet.");
    return;
  }
  const LiveBvhStats& live_stats = data->live;
  const uint32_t frame = g_state.frame.load();
  ImGui::Text("TLAS: %u instances, last rebuild %s (%u frames ago)", live_stats.tlas_instances,
              live_stats.tlas_rebuilds != 0u ? live_stats.tlas_reason : "none",
              live_stats.tlas_rebuilds != 0u && frame >= live_stats.tlas_frame ? frame - live_stats.tlas_frame : 0u);
  {
    const uint64_t readback_errors = stats.map_failures + stats.staging_failures + stats.base_mismatch;
    const uint64_t bvh_errors = live_stats.blas_invalid + live_stats.tlas_invalid + live_stats.upload_failures
                                + live_stats.refit_failures + data->trace_stats.invalid_refs + data->trace_stats.stack_overflow;
    const uint64_t deform_errors = deform.failed + deform.resource_failures + deform.map_failures;
    if (stats.mesh_failures + readback_errors + bvh_errors + deform_errors != 0u) {
      ImGui::TextColored(ImVec4(1.f, 0.4f, 0.4f, 1.f), "Errors: meshes %llu  readback %llu  BVH %llu  deform %llu",
                         static_cast<unsigned long long>(stats.mesh_failures),
                         static_cast<unsigned long long>(readback_errors), static_cast<unsigned long long>(bvh_errors),
                         static_cast<unsigned long long>(deform_errors));
    }
  }
  ImGui::Text("CPU: meshes %.2f ms, TLAS %.2f ms (refit %.2f ms)",
              live_stats.mesh_ms_last, live_stats.tlas_ms_last, live_stats.refit_ms_last);
  ImGui::Text("GPU: deform refit %.3f ms, trace %.2f ms", deform_refit_ms, data->trace_timer.last_ms);
  ImGui::Text("Pool CPU per frame: hooks %.2f ms, drain %.2f ms, deform %.2f ms",
              static_cast<double>(g_pool.cpu_hook_frame_us.load(std::memory_order_relaxed)) / 1000.0,
              static_cast<double>(g_pool.cpu_drain_frame_us.load(std::memory_order_relaxed)) / 1000.0,
              static_cast<double>(g_deform_live.cpu_frame_us.load(std::memory_order_relaxed)) / 1000.0);
  if (live_stats.gpu_checked) {
    const ImVec4 color = live_stats.gpu_ok ? ImVec4(0.4f, 1.f, 0.4f, 1.f) : ImVec4(1.f, 0.4f, 0.4f, 1.f);
    ImGui::TextColored(color, "GPU contents: %s", live_stats.gpu_result.c_str());
  }
  if (mode != static_cast<int>(BvhView::Off)) {
    ImGui::TextDisabled("Inspect: middle-click a pixel of the trace view to see what the BVH hit there (also written to ReShade.log).");
  }
  for (const std::string& line : data->inspect.lines) ImGui::TextUnformatted(line.c_str());
  if (data->trace_stats.valid) DrawDepthCompareStats(data->trace_stats);
}

}  // namespace falcom_world::bvh
