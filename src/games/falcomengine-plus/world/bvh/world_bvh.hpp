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

#include <cmath>
#include <mutex>
#include <string>
#include <vector>

#include "../contract/shader_registry.hpp"
#include "../debug/crash_log.hpp"
#include "bvh_debug.hpp"
#include "bvh_pool.hpp"
#include "cache_diag.hpp"
#include "p2b_diag.hpp"
#include "placement_probe.hpp"

namespace falcom_world::bvh {

inline void OnDestroyDeviceBvh(reshade::api::device* device) {
  DestroyBvhDebugResources(device);
  rtao::DestroyRtaoDeviceData(device);
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
              g_pool.region_size,
              PoolPercent(stats.compare_match, judged),
              PoolPercent(stats.compare_missing, judged),
              PoolPercent(stats.compare_extra, judged),
              PoolPercent(stats.compare_extra_sky, judged));
}

// Ray Tracing tab (world_settings.hpp): the switches that decide what enters the BVH. Session only, defaults as before.
struct RayTracingRow {
  const char* label;
  const char* description;
  const char* tooltip;
  std::vector<std::atomic_bool*> flags;  // written together; the first one shows the state
};

inline void DrawRayTracingRows() {
  ImGui::SetNextItemWidth(150.f);
  ImGui::SliderFloat("Region (m)", &g_pool.region_size, kPoolRegionMinSize, kPoolRegionMaxSize, "%.0f");
  ImGui::TextDisabled("BVH range: every pooled object within this distance of the camera is in the BVH; RTAO reaches it at 100% fade.");
  static const std::vector<RayTracingRow> rows = {
      {"Alpha foliage (leaves, grass, fences)",
       "Adds alpha-tested foliage, including foliage drawn by indirect (GPU driven) calls.",
       "Experimental, session only, default on. Off: alpha-tested meshes are not traced and their "
       "instances leave the BVH at once. On: their cutouts are traced through a 256-slice atlas, filled "
       "at the next present (copies of the source textures are made when drawn).",
       {&g_pool.alpha_foliage, &g_pool.alpha_indirect_source}},
      {"Foliage motion (wind and sway)",
       "Sways wind foliage. Off: it stays in its rest pose. Needs Alpha foliage.",
       "Experimental, session only, default on. Approximation: wind rest poses are moved in the BVH by the "
       "wind vertex shader's sway at the instance origin (rigid, full amplitude). Off: they stay at their "
       "rest pose.",
       {&g_pool.alpha_wind_sway}},
      {"Deforming meshes (characters, water)",
       "Adds skinned and animated meshes.",
       "Every frame, re-runs each camera draw of a skinned or animated vertex shader whose world\n"
       "position the probe confirmed, capturing its world-space triangles on the GPU. Each draw's\n"
       "tree is built once from its first capture and refit every frame; the trace walks them\n"
       "beside the static BVH. The probe (it confirms new shaders) is on while this is on.",
       {&g_deform_live.enabled, &g_deform.enabled}},
      {"Moving objects follow their pose",
       "Keeps moving objects in the BVH at their current pose.",
       "Moving objects keep their instances and each one moves to the pose the object\n"
       "has now (a copy of a moving object is taken every frame). Off: the old behaviour.\n"
       "Ignored while \"Keep moving objects out\" is on.",
       {&g_pool.follow_moving}},
      {"Moving objects: per-frame (P2b)",
       "Adds moving objects at the pose of the previous frame.",
       "Session only, default on. A moving mesh is copied at each draw and traced from the instances\n"
       "of the previous frame (one frame behind, no staging delay). Off: the follow path (a few presents behind), or absent when Follow moving is off.",
       {&g_pool.dynamic_per_frame}},
      {"Keep moving objects out of the static BVH",
       "Removes moving objects from the static BVH.",
       "A mesh seen moving in a camera view (its prevWorld differs from world) gets no\n"
       "static instances: those it had are removed and new ones refused, so a door or cart\n"
       "leaves no copy behind where it stopped. Without the per-frame path (P2b) or Follow moving, moving\n"
       "objects are absent from the BVH. Off: admitted as before (applies at once).",
       {&g_pool.exclude_moving}},
      {"Scan indirect draws",
       "Lets indirect (GPU driven) draws, opaque or alpha, enter the BVH.",
       "Off: indirect draws (counts kept on the GPU) are ignored by the pool.",
       {&g_pool.scan_indirect}},
  };
  bool changed = false;
  for (const RayTracingRow& row : rows) {
    bool on = row.flags.front()->load(std::memory_order_relaxed);
    if (ImGui::Checkbox(row.label, &on)) {
      for (std::atomic_bool* flag : row.flags) flag->store(on, std::memory_order_relaxed);
      changed = true;
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", row.tooltip);
    ImGui::TextDisabled("%s", row.description);
  }
  if (changed) {
    LogPoolSwitches();
    RefreshPoolCaptureRequest();
  }
}

// Label of the cache diagnostics dump and of the Performance test (file name only).
inline char g_diag_label[64] = {};

// Performance tab (world_settings.hpp): every timing of the add-on is shown here and nowhere else. Nothing is gathered
// while "Gather performance metrics" is off (perf_gate.hpp). NaN shows "--".
inline void DrawPerformanceTab() {
  PerfTabView view;
  bool running = false;
  double elapsed_s = 0.0;
  std::string perf_label;
  {
    std::lock_guard<std::mutex> lock(g_perf.mutex);
    view = g_perf.tab.view;
    running = g_perf.active.load();
    elapsed_s = g_perf.run.elapsed_s;
    perf_label = g_perf.run.label;
  }
  bool gather = g_perf_gate.gather.load(std::memory_order_relaxed);
  if (ImGui::Checkbox("Gather performance metrics", &gather)) {
    g_perf_gate.clear_pending.store(!gather, std::memory_order_relaxed);  // OFF: the next present clears what was gathered
    if (gather) {
      std::lock_guard<std::mutex> lock(g_perf.mutex);
      g_perf.tab = PerfTabState{};
    }
    g_perf_gate.gather.store(gather, std::memory_order_relaxed);
  }
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("Costs a few clock reads per present and changes no rendering. Off (default): nothing is measured.\n"
                      "Switching it off clears everything gathered.");
  }
  if (!gather) {
    ImGui::TextDisabled("Metrics are off. Switch on to measure. Dumps written while off carry no timings (see perf_gather in the file).");
    return;
  }
  const auto row = [](const char* name, const char* unit, double now, double avg) {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted(name);
    ImGui::TableSetColumnIndex(1);
    ImGui::TextUnformatted(unit);
    ImGui::TableSetColumnIndex(2);
    if (std::isnan(now)) {
      ImGui::TextUnformatted("--");
    } else {
      ImGui::Text("%.3f", now);
    }
    ImGui::TableSetColumnIndex(3);
    if (std::isnan(avg)) {
      ImGui::TextUnformatted("--");
    } else {
      ImGui::Text("%.3f", avg);
    }
  };
  const auto header = [] {
    ImGui::TableSetupColumn("Cost");
    ImGui::TableSetupColumn("Unit");
    ImGui::TableSetupColumn("Now");
    ImGui::TableSetupColumn("Avg 30");
    ImGui::TableHeadersRow();
  };
  if (ImGui::BeginTable("perf_rows", 4, ImGuiTableFlags_Borders)) {
    header();
    row("frame_time", "ms/frame", view.frame_now, view.frame_avg);
    for (size_t i = 0; i < kPerfCostCount; ++i) {
      const PerfRole role = kPerfCostInfo[i].role;
      if (role == kPerfInPresent || role == kPerfDrawTime) row(kPerfCostInfo[i].name, "ms/present", view.now[i], view.avg[i]);
    }
    const bool clamped = view.unattr_raw_now < -0.01 || view.unattr_raw_avg < -0.01;
    row(clamped ? "unattributed_cpu (clamped)" : "unattributed_cpu", "ms/present", view.unattr_now, view.unattr_avg);
    if (clamped && ImGui::IsItemHovered()) {
      ImGui::SetTooltip("Raw value before the clamp at zero: now %.3f ms, avg %.3f ms (the rows are not frame-aligned; see ROADMAP).",
                        view.unattr_raw_now, view.unattr_raw_avg);
    }
    for (size_t i = 0; i < kPerfCostCount; ++i) {
      if (kPerfCostInfo[i].role == kPerfGpu) row(kPerfCostInfo[i].name, "ms/measured frame", view.now[i], view.avg[i]);
    }
    row("cpu_total", "ms/present", view.cpu_now, view.cpu_avg);
    row("gpu_total", "ms/measured frame", view.gpu_now, view.gpu_avg);
    ImGui::EndTable();
  }
  if (ImGui::CollapsingHeader("Present handlers (brackets; not in the totals)")) {
    if (ImGui::BeginTable("perf_brackets", 4, ImGuiTableFlags_Borders)) {
      header();
      for (size_t i = 0; i < kPerfCostCount; ++i) {
        if (kPerfCostInfo[i].role == kPerfBracket) row(kPerfCostInfo[i].name, "ms/present", view.now[i], view.avg[i]);
      }
      ImGui::EndTable();
    }
  }
  if (ImGui::CollapsingHeader("BVH phases (subsets of the rows above; not in the totals)")) {
    if (ImGui::BeginTable("perf_phases", 4, ImGuiTableFlags_Borders)) {
      header();
      for (size_t i = 0; i < kPerfCostCount; ++i) {
        if (kPerfCostInfo[i].role == kPerfSubset) row(kPerfCostInfo[i].name, "ms/present", view.now[i], view.avg[i]);
      }
      ImGui::EndTable();
    }
  }
  ImGui::TextDisabled("Last %u presents: now = newest present, avg = mean of the window. CPU rows are per present; GPU rows are per measured frame (trace, alpha blit, P2b expand, deform refit, RTAO trace, RTAO filter). Unattributed CPU = the two present handlers minus the rows inside them.",
                      static_cast<unsigned>(kPerfTabWindow));
  if (ImGui::Button("Performance test (30 s)")) RequestPerfRun(g_diag_label);
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("Averages over 30 s of presents, written to its own perf file (keys as in the table; perf_gather says whether gathering was on).\n"
                      "Start it after the map has settled (not during the refill after a map change).\n"
                      "Label: optional, file name only.");
  }
  if (running) {
    ImGui::TextColored(ImVec4(1.f, 0.85f, 0.4f, 1.f), "Performance test running: %.1f / %.1f s [%s]", elapsed_s, kPerfWindowSeconds,
                       perf_label.c_str());
  }
  std::string result;
  {
    std::lock_guard<std::mutex> lock(g_cache_diag.mutex);
    result = g_cache_diag.data.last_result;
  }
  if (!result.empty()) ImGui::TextUnformatted(result.c_str());
}

inline void DrawBvhPanel() {
  ImGui::TextDisabled("Pool admits draws whose vertex shader the bytecode classifier marks rigid and whose pixel shader does not alpha-test.");

  bool scan = g_pool.scan_active.load(std::memory_order_relaxed);
  if (ImGui::Checkbox("Pool Scan", &scan)) {
    g_pool.scan_active.store(scan, std::memory_order_relaxed);
    RefreshPoolCaptureRequest();
    LogPoolSwitches();
  }
  bool switches_changed = false;

  bool do_dump = false;
  bool do_reset = false;
  if (ImGui::Button("Dump Pool JSON")) do_dump = true;
  ImGui::SameLine();
  if (ImGui::Button("Diagnostics (all)")) {
    g_p2b_diag.armed_frame.store(g_state.frame.load());
    g_bvh_trace.stats_requested.store(true);
    g_p2b_diag.requested.store(true);
  }
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("Writes world_p2b_diag.json, world_p2b_summary.txt, world_pool*.json, world_deform_live.json and the manifest to the pool folder. Waits for the GPU once.\n"
                      "Needs a trace view on (Trace Shaded or Trace Instance) and one middle-click on the moving object first; without them the trace verdicts say 'no fresh trace'.");
  }
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
    bool alpha_indirect = g_pool.alpha_indirect_source.load(std::memory_order_relaxed);
    if (ImGui::Checkbox("Indirect foliage source copy (experimental)", &alpha_indirect)) {
      g_pool.alpha_indirect_source.store(alpha_indirect, std::memory_order_relaxed);
      switches_changed = true;
    }
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("Experimental, session only, default on. Effective only with Alpha-tested foliage on. On: indirect "
                        "alpha draws (wind and other indirect foliage) get a source copy and can enter the BVH. Off: they "
                        "leave the BVH at the next present.");
    }
    bool alpha_wind = g_pool.alpha_wind_opaque.load(std::memory_order_relaxed);
    if (ImGui::Checkbox("Admit wind with opaque pixel shader (experimental)", &alpha_wind)) {
      g_pool.alpha_wind_opaque.store(alpha_wind, std::memory_order_relaxed);
      switches_changed = true;
    }
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("Experimental, session only, default on. Effective only with Alpha-tested foliage on. On: wind "
                        "vertex shaders (opaque or alpha-tested pixel shader) are admitted as a rigid rest pose instead of WindOpaque. "
                        "Off: their instances leave the BVH at the next present.");
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
    DumpWorldPool(g_bvh_trace.hide_camera_hidden.load(std::memory_order_relaxed));
    DumpDeformLive();
  }
  if (do_dump_obj) DumpWorldPoolObj();
  if (do_reset) ResetWorldPool();


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
  bool skip_noop = g_live_bvh.skip_noop.load(std::memory_order_relaxed);
  if (ImGui::Checkbox("Skip no-op TLAS rebuilds", &skip_noop)) g_live_bvh.skip_noop.store(skip_noop, std::memory_order_relaxed);
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("A TLAS rebuild whose instances equal the TLAS already in place is skipped (no sort, build or buffer\n"
                      "creation). Default OFF until the in-game A/B; off: every rebuild runs, as before. Session only, not saved.");
  }
  bool slack = g_live_bvh.slack.load(std::memory_order_relaxed);
  if (ImGui::Checkbox("Slack TLAS (experimental, session only)", &slack)) g_live_bvh.slack.store(slack, std::memory_order_relaxed);
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("Keeps spare slots in the TLAS so most changes are written in place (no sort, build or buffer creation).\n"
                      "Trade: tree quality drifts until the next full rebuild (at most every 120 frames or when the slots run out).\n"
                      "Off by default. Session only, not saved.");
  }

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
                      "Off: they stay visible and are counted as hidden-from-camera pixels.\n"
                      "Default on.");
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
  if (CollectCacheErrors(data).total != 0u) ImGui::TextColored(ImVec4(1.f, 0.4f, 0.4f, 1.f), "Errors recorded: see the dump (errors block)");
  if (live_stats.gpu_checked) {
    const ImVec4 color = live_stats.gpu_ok ? ImVec4(0.4f, 1.f, 0.4f, 1.f) : ImVec4(1.f, 0.4f, 0.4f, 1.f);
    ImGui::TextColored(color, "GPU contents: %s", live_stats.gpu_ok ? "match" : "MISMATCH");
  }
  if (mode != static_cast<int>(BvhView::Off)) {
    ImGui::TextDisabled("Inspect: middle-click a pixel of the trace view to see what the BVH hit there (also written to ReShade.log).");
  }
  for (const std::string& line : data->inspect.lines) ImGui::TextUnformatted(line.c_str());
  if (data->trace_stats.valid) DrawDepthCompareStats(data->trace_stats);

  // Phase 0 cache diagnostics (diagnostic only: no rendering change). Counts and timings are in the dump (timings when
  // Gather performance metrics is on); costs are on the Performance tab. Each dump and each performance test writes its own file.
  ImGui::SeparatorText("Cache diagnostics (Phase 0 and 0b, diagnostic only: no rendering change)");
  bool collect = g_cache_diag.enabled.load(std::memory_order_relaxed);
  if (ImGui::Checkbox("Collect cache diagnostics", &collect)) g_cache_diag.enabled.store(collect, std::memory_order_relaxed);
  ImGui::InputText("Label", g_diag_label, sizeof(g_diag_label));
  if (ImGui::Button("Dump cache diagnostics")) {
    // The first press asks the next present to compute the tiers; the press after that (once a present has run) writes the file.
    bool ready = false;
    {
      std::lock_guard<std::mutex> lock(g_cache_diag.mutex);
      ready = g_cache_diag.data.dump_armed && !g_cache_diag.force_tiers_requested.load(std::memory_order_relaxed);
      g_cache_diag.data.dump_armed = !ready;
      if (!ready) {
        g_cache_diag.force_tiers_requested.store(true, std::memory_order_relaxed);
        g_cache_diag.data.last_result = "Dump pending: the tiers are recomputed on the next present; press Dump again after one frame";
      }
    }
    if (ready) {
      const uint32_t dump_frame = g_state.frame.load();
      const std::filesystem::path path = WriteCacheDiagFile(data, dump_frame, g_diag_label, ReadCacheSwitches());
      std::error_code ec;
      const uintmax_t bytes = std::filesystem::file_size(path, ec);
      const bool written = !ec && bytes != 0u;
      std::string result = "Cache diagnostics: FAILED to write " + path.string();
      if (written) {
        const std::vector<std::string> report = CacheDiagLines(data);
        std::string text;
        for (const std::string& line : report) text += line + "\n";
        ImGui::SetClipboardText(text.c_str());
        result = "Cache diagnostics: wrote " + path.string() + " (" + std::to_string(bytes) + " bytes) at f" + std::to_string(dump_frame)
                 + "; report copied to clipboard (" + std::to_string(report.size()) + " lines)";
      }
      std::lock_guard<std::mutex> lock(g_cache_diag.mutex);
      g_cache_diag.data.last_result = result;
      if (written) {
        g_cache_diag.data.last_path = path.string();
        g_cache_diag.data.last_bytes = bytes;
      }
    }
  }
  ImGui::SameLine();
  if (ImGui::Button("Reset diag counters")) {
    g_cache_diag.reset_requested.store(true, std::memory_order_relaxed);
    std::lock_guard<std::mutex> lock(g_cache_diag.mutex);
    g_cache_diag.data.last_result = "Diag counters reset at f" + std::to_string(g_state.frame.load());
  }
  {
    std::string result;
    {
      std::lock_guard<std::mutex> lock(g_cache_diag.mutex);
      result = g_cache_diag.data.last_result;
    }
    if (!result.empty()) ImGui::TextUnformatted(result.c_str());
  }

  ImGui::TextDisabled("Placement probe: static instances within %.0f m of the camera (Chebyshev), nearest first, up to %u records.",
                      static_cast<double>(kPlacementRadius), static_cast<unsigned>(kPlacementCap));
  ImGui::TextDisabled("Compare needs the same map and spot: after a reload, return to the same map and spot before Compare to A.");
  if (ImGui::Button("Placement: snapshot now")) {
    PlacementSnapshot snapshot;
    TakePlacementSnapshot(&snapshot);
    const std::filesystem::path path = PoolOutputDir() / "world_placement_now.txt";
    std::string message = "Placement snapshot now: FAILED to write " + path.string();
    if (WritePlacementFile(path, snapshot)) {
      std::error_code ec;
      const uintmax_t bytes = std::filesystem::file_size(path, ec);
      message = "Placement snapshot now: " + std::to_string(snapshot.records.size()) + " records at f" + std::to_string(snapshot.frame)
                + (snapshot.truncated ? " (truncated)" : "") + ", wrote " + path.string() + " (" + std::to_string(ec ? 0u : bytes) + " bytes)";
    }
    std::lock_guard<std::mutex> lock(g_placement.mutex);
    g_placement.lines = {message};
  }
  ImGui::SameLine();
  if (ImGui::Button("Save snapshot A")) {
    PlacementSnapshot snapshot;
    TakePlacementSnapshot(&snapshot);
    const std::filesystem::path path = PoolOutputDir() / "world_placement_A.txt";
    std::string message = "Snapshot A (memory): " + std::to_string(snapshot.records.size()) + " records at f" + std::to_string(snapshot.frame);
    if (WritePlacementFile(path, snapshot)) {
      std::error_code ec;
      const uintmax_t bytes = std::filesystem::file_size(path, ec);
      message += ", wrote " + path.string() + " (" + std::to_string(ec ? 0u : bytes) + " bytes)";
    } else {
      message += ", FAILED to write " + path.string();
    }
    std::lock_guard<std::mutex> lock(g_placement.mutex);
    g_placement.a = std::move(snapshot);
    g_placement.a_valid = true;
    g_placement.lines = {message};
  }
  ImGui::SameLine();
  if (ImGui::Button("Compare to A")) {
    std::vector<std::string> report;
    PlacementSnapshot a;
    bool have_a = false;
    {
      std::lock_guard<std::mutex> lock(g_placement.mutex);
      if (g_placement.a_valid) {
        a = g_placement.a;
        have_a = true;
      }
    }
    const std::filesystem::path a_path = PoolOutputDir() / "world_placement_A.txt";
    if (!have_a) {
      std::error_code ec;
      std::string error;
      if (std::filesystem::exists(a_path, ec)) {
        if (ReadPlacementFile(a_path, &a, &error)) {
          have_a = true;
          report.push_back("A: loaded from file " + a_path.string());
          std::lock_guard<std::mutex> lock(g_placement.mutex);
          g_placement.a = a;
          g_placement.a_valid = true;
        } else {
          report.push_back("A: could not read " + a_path.string() + ": " + error);
        }
      }
    }
    if (!have_a) {
      report.push_back("NO SNAPSHOT A: press \"Save snapshot A\" first");
    } else {
      PlacementSnapshot b;
      TakePlacementSnapshot(&b);
      const PlacementCompare compare = ComparePlacementSnapshots(a, b);
      const std::filesystem::path b_path = PoolOutputDir() / "world_placement_B.txt";
      const std::filesystem::path c_path = PoolOutputDir() / "world_placement_compare.txt";
      std::string text;
      for (const std::string& line : compare.lines) text += line + "\n";
      std::error_code ec_stale;
      std::filesystem::remove(b_path, ec_stale);  // a failed write must not leave the previous compare behind
      std::filesystem::remove(c_path, ec_stale);
      const bool wrote_b = WritePlacementFile(b_path, b);
      renodx::utils::path::WriteTextFile(c_path, text);
      std::error_code ec_b;
      std::error_code ec_c;
      const uintmax_t bytes_b = wrote_b ? std::filesystem::file_size(b_path, ec_b) : 0u;
      const uintmax_t bytes_c = std::filesystem::file_size(c_path, ec_c);
      report.insert(report.end(), compare.lines.begin(), compare.lines.end());
      ImGui::SetClipboardText(text.c_str());
      report.push_back("Wrote " + b_path.string() + " (" + std::to_string(wrote_b && !ec_b ? bytes_b : 0u) + " bytes) and " + c_path.string()
                       + " (" + std::to_string(ec_c ? 0u : bytes_c) + " bytes); report copied to clipboard");
    }
    std::lock_guard<std::mutex> lock(g_placement.mutex);
    g_placement.lines = report;
  }
  ImGui::SameLine();
  if (ImGui::Button("Clear snapshots")) {
    std::lock_guard<std::mutex> lock(g_placement.mutex);
    g_placement.a = PlacementSnapshot{};
    g_placement.a_valid = false;
    g_placement.lines.clear();
  }
  std::vector<std::string> placement_lines;
  {
    std::lock_guard<std::mutex> lock(g_placement.mutex);
    placement_lines = g_placement.lines;
  }
  for (const std::string& line : placement_lines) {
    if (line.rfind("Verdict: PASS", 0) == 0) {
      ImGui::TextColored(ImVec4(0.4f, 1.f, 0.4f, 1.f), "%s", line.c_str());
    } else if (line.rfind("Verdict: WARN", 0) == 0) {
      ImGui::TextColored(ImVec4(1.f, 0.75f, 0.2f, 1.f), "%s", line.c_str());
    } else if (line.rfind("Verdict: FAIL", 0) == 0 || line.rfind("Verdict: INCONCLUSIVE", 0) == 0) {
      ImGui::TextColored(ImVec4(1.f, 0.4f, 0.4f, 1.f), "%s", line.c_str());
    } else {
      ImGui::TextUnformatted(line.c_str());
    }
  }
}

}  // namespace falcom_world::bvh
