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

#include "../../../utils/settings.hpp"

#include "../contract/shader_registry.hpp"
#include "../debug/crash_log.hpp"
#include "bvh_debug.hpp"
#include "bvh_pool.hpp"
#include "cache_diag.hpp"
#include "p2b_diag.hpp"
#include "placement_probe.hpp"

namespace falcom_world::bvh {

inline void OnDestroyDeviceBvh(reshade::api::device* device) {
  rt12::DestroyGc(device);
  rt12::DestroyRt12(device);
  DestroyBvhDebugResources(device);
  rtao::DestroyRtaoDeviceData(device);
  rtgi::DestroyRtgiDeviceData(device);
  rtls::DestroyRtlsDeviceData(device);
  rtsun::DestroyRtSunDeviceData(device);
  DestroyBvhDeviceData(device);
}

inline void Use(DWORD fdw_reason) {
  const uint32_t hook_off = HookOffMask();
  switch (fdw_reason) {
    case DLL_PROCESS_ATTACH:
      if ((hook_off & 128u) == 0u) {
        reshade::register_event<reshade::addon_event::destroy_device>(OnDestroyDevicePool);
      }
      if ((hook_off & 32u) == 0u) {
        reshade::register_event<reshade::addon_event::destroy_resource>(OnDestroyResourcePool);
        reshade::register_event<reshade::addon_event::update_buffer_region>(OnUpdateBufferRegionPool);
        reshade::register_event<reshade::addon_event::update_buffer_region_command>(OnUpdateBufferRegionCommandPool);
        reshade::register_event<reshade::addon_event::map_buffer_region>(OnMapBufferRegionPool);
        reshade::register_event<reshade::addon_event::copy_buffer_region>(OnCopyBufferRegionPool);
        reshade::register_event<reshade::addon_event::copy_resource>(OnCopyResourcePool);
      }
      if ((hook_off & 128u) == 0u) {
        reshade::register_event<reshade::addon_event::destroy_device>(OnDestroyDeviceBvh);
      }
      if ((hook_off & 64u) == 0u) {
        reshade::register_event<reshade::addon_event::present>(OnWorldPresentBvh);
      }
      if ((hook_off & 128u) == 0u) {
        reshade::register_event<reshade::addon_event::destroy_pipeline>(OnDestroyPipelineDeform);
        reshade::register_event<reshade::addon_event::destroy_device>(OnDestroyDeviceDeform);
        reshade::register_event<reshade::addon_event::destroy_pipeline>(OnDestroyPipelineDeformLive);
        reshade::register_event<reshade::addon_event::destroy_device>(OnDestroyDeviceDeformLive);
      }
      break;
    case DLL_PROCESS_DETACH:
      if ((hook_off & 128u) == 0u) {
        reshade::unregister_event<reshade::addon_event::destroy_device>(OnDestroyDevicePool);
      }
      if ((hook_off & 32u) == 0u) {
        reshade::unregister_event<reshade::addon_event::destroy_resource>(OnDestroyResourcePool);
        reshade::unregister_event<reshade::addon_event::update_buffer_region>(OnUpdateBufferRegionPool);
        reshade::unregister_event<reshade::addon_event::update_buffer_region_command>(OnUpdateBufferRegionCommandPool);
        reshade::unregister_event<reshade::addon_event::map_buffer_region>(OnMapBufferRegionPool);
        reshade::unregister_event<reshade::addon_event::copy_buffer_region>(OnCopyBufferRegionPool);
        reshade::unregister_event<reshade::addon_event::copy_resource>(OnCopyResourcePool);
      }
      if ((hook_off & 128u) == 0u) {
        reshade::unregister_event<reshade::addon_event::destroy_device>(OnDestroyDeviceBvh);
      }
      if ((hook_off & 64u) == 0u) {
        reshade::unregister_event<reshade::addon_event::present>(OnWorldPresentBvh);
      }
      if ((hook_off & 128u) == 0u) {
        reshade::unregister_event<reshade::addon_event::destroy_pipeline>(OnDestroyPipelineDeform);
        reshade::unregister_event<reshade::addon_event::destroy_device>(OnDestroyDeviceDeform);
        reshade::unregister_event<reshade::addon_event::destroy_pipeline>(OnDestroyPipelineDeformLive);
        reshade::unregister_event<reshade::addon_event::destroy_device>(OnDestroyDeviceDeformLive);
      }
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
  const char* key;  // settings key for persistence (empty = not persisted)
};

inline bool DrawRayTracingRows() {
  using renodx::utils::settings::FindSetting;
  bool changed = false;

  ImGui::SetNextItemWidth(150.f);
  if (ImGui::SliderFloat("Region (m)", &g_pool.region_size, kPoolRegionMinSize, kPoolRegionMaxSize, "%.0f")) {
    if (auto* s = FindSetting("WorldRegionSize")) {
      s->Set(g_pool.region_size);
      s->Write();
    }
    changed = true;
  }
  ImGui::TextDisabled("BVH range: every pooled object within this distance of the camera is in the BVH; RTAO reaches it at 100% fade.");
  static const std::vector<RayTracingRow> rows = {
      {"Alpha foliage (leaves, grass, fences)",
       "Adds alpha-tested foliage, including foliage drawn by indirect (GPU driven) calls.",
       "Experimental, session only, default on. Off: alpha-tested meshes are not traced and their "
       "instances leave the BVH at once. On: their cutouts are traced through a 256-slice atlas, filled "
       "at the next present (copies of the source textures are made when drawn).",
       {&g_pool.alpha_foliage, &g_pool.alpha_indirect_source}, "WorldAlphaFoliage"},
      {"Foliage motion (wind and sway)",
       "Sways wind foliage. Off: it stays in its rest pose. Needs Alpha foliage.",
       "Experimental, session only, default on. Approximation: wind rest poses are moved in the BVH by the "
       "wind vertex shader's sway at the instance origin (rigid, full amplitude). Off: they stay at their "
       "rest pose.",
       {&g_pool.alpha_wind_sway}, "WorldAlphaWindSway"},
      {"Deforming meshes (characters, water)",
       "Adds skinned and animated meshes.",
       "Every frame, re-runs each camera draw of a skinned or animated vertex shader whose world\n"
       "position the probe confirmed, capturing its world-space triangles on the GPU. Each draw's\n"
       "tree is built once from its first capture and refit every frame; the trace walks them\n"
       "beside the static BVH. The probe (it confirms new shaders) is on while this is on.",
       {&g_deform_live.enabled, &g_deform.enabled}, "WorldDeformingMeshes"},
      {"Moving objects follow their pose",
       "Keeps moving objects in the BVH at their current pose.",
       "Moving objects keep their instances and each one moves to the pose the object\n"
       "has now (a copy of a moving object is taken every frame). Off: the old behaviour.\n"
       "Ignored while \"Keep moving objects out\" is on.",
       {&g_pool.follow_moving}, "WorldFollowMoving"},
      {"Moving objects: per-frame (P2b)",
       "Adds moving objects at the pose of the previous frame.",
       "Session only, default on. A moving mesh is copied at each draw and traced from the instances\n"
       "of the previous frame (one frame behind, no staging delay). Off: the follow path (a few presents behind), or absent when Follow moving is off.",
       {&g_pool.dynamic_per_frame}, "WorldDynamicPerFrame"},
      {"Keep moving objects out of the static BVH",
       "Removes moving objects from the static BVH.",
       "A mesh seen moving in a camera view (its prevWorld differs from world) gets no\n"
       "static instances: those it had are removed and new ones refused, so a door or cart\n"
       "leaves no copy behind where it stopped. Without the per-frame path (P2b) or Follow moving, moving\n"
       "objects are absent from the BVH. Off: admitted as before (applies at once).",
       {&g_pool.exclude_moving}, "WorldExcludeMoving"},
      {"Scan indirect draws",
       "Lets indirect (GPU driven) draws, opaque or alpha, enter the BVH.",
       "Off: indirect draws (counts kept on the GPU) are ignored by the pool.",
       {&g_pool.scan_indirect}, "WorldScanIndirect"},
  };
  for (const RayTracingRow& row : rows) {
    bool on = row.flags.front()->load(std::memory_order_relaxed);
    if (ImGui::Checkbox(row.label, &on)) {
      for (std::atomic_bool* flag : row.flags) flag->store(on, std::memory_order_relaxed);
      if (row.key != nullptr && row.key[0] != '\0') {
        if (auto* s = FindSetting(row.key)) {
          s->Set(on ? 1.f : 0.f);
          s->Write();
        }
      }
      changed = true;
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", row.tooltip);
    ImGui::TextDisabled("%s", row.description);
    if (row.flags.front() == &g_pool.alpha_wind_sway) {
      ImGui::BeginDisabled(!on);
      ImGui::SetNextItemWidth(150.f);
      if (ImGui::SliderFloat("Wind sway range (m)", &g_pool.wind_sway_range, kPoolWindSwayRangeMin, kPoolWindSwayRangeMax, "%.0f")) {
        if (auto* s = FindSetting("WorldWindSwayRange")) {
          s->Set(g_pool.wind_sway_range);
          s->Write();
        }
        changed = true;
      }
      if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) ImGui::SetTooltip("Wind foliage farther than this from the camera keeps its last pose during refits (saves CPU). Rebuilds still sway every instance. Maximum = no limit. Not saved; resets on restart.");
      ImGui::EndDisabled();
    }
  }
  if (changed) {
    LogPoolSwitches();
    RefreshPoolCaptureRequest();
  }
  return changed;
}

// Copies the persisted setting mirrors back into the atomic switch flags (and any
// other runtime state) so values loaded from the ReShade ini take effect even when
// the Ray Tracing tab is never opened in a session. Call once after LoadSettings.
inline void SyncBvhSettings() {
  auto apply = [](std::atomic_bool& flag, float mirror) { flag.store(mirror > 0.5f, std::memory_order_relaxed); };
  apply(g_pool.alpha_foliage, g_pool.alpha_foliage_setting);
  apply(g_pool.alpha_wind_sway, g_pool.alpha_wind_sway_setting);
  apply(g_pool.follow_moving, g_pool.follow_moving_setting);
  apply(g_pool.dynamic_per_frame, g_pool.dynamic_per_frame_setting);
  apply(g_pool.exclude_moving, g_pool.exclude_moving_setting);
  apply(g_pool.scan_indirect, g_pool.scan_indirect_setting);
  apply(g_deform_live.enabled, g_deform_live.enabled_setting);
  apply(g_deform.enabled, g_deform.enabled_setting);
  apply(g_pool.scan_active, g_pool.scan_active_setting);
  apply(g_pool.capture_meshes, g_pool.capture_meshes_setting);
  apply(g_pool.log_captures, g_pool.log_captures_setting);
  apply(g_pool.verify_meshes, g_pool.verify_meshes_setting);
  apply(g_pool.retry_unstable, g_pool.retry_unstable_setting);
  apply(g_pool.legacy_scale, g_pool.legacy_scale_setting);
  apply(g_pool.poison_staging, g_pool.poison_staging_setting);
  apply(g_pool.alpha_indirect_source, g_pool.alpha_indirect_source_setting);
  apply(g_pool.alpha_wind_opaque, g_pool.alpha_wind_opaque_setting);
  apply(g_live_bvh.enabled, g_live_bvh.enabled_setting);
  apply(g_live_bvh.skip_noop, g_live_bvh.skip_noop_setting);
  apply(g_live_bvh.slack, g_live_bvh.slack_setting);
  apply(g_bvh_trace.hide_camera_hidden, g_bvh_trace.hide_camera_hidden_setting);
  apply(g_cache_diag.enabled, g_cache_diag.enabled_setting);
  apply(g_crash_log.enabled, g_crash_log.enabled_setting);
  g_bvh_debug.mode.store(static_cast<int>(g_bvh_debug.mode_setting), std::memory_order_relaxed);
  ApplyBvhInputs();
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
  const auto cell = [](double value) {
    if (std::isnan(value)) {
      ImGui::TextUnformatted("--");
    } else {
      ImGui::Text("%.3f", value);
    }
  };
  const auto row = [&view, &cell](const char* name, const char* unit, double now, double avg, double max, bool clamped) {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted(name);
    ImGui::TableSetColumnIndex(1);
    ImGui::TextUnformatted(unit);
    ImGui::TableSetColumnIndex(2);
    cell(now);
    ImGui::TableSetColumnIndex(3);
    cell(avg);
    if (clamped && ImGui::IsItemHovered()) {
      ImGui::SetTooltip("Raw value before the clamp at zero: now %.3f ms, avg %.3f ms (the rows and the frame interval are one sample window; see ROADMAP).",
                        view.unattr_raw_now, view.unattr_raw_avg);
    }
    ImGui::TableSetColumnIndex(4);
    cell(max);
  };
  const auto header = [] {
    ImGui::TableSetupColumn("Cost");
    ImGui::TableSetupColumn("Unit");
    ImGui::TableSetupColumn("Now");
    ImGui::TableSetupColumn("Avg 30");
    ImGui::TableSetupColumn("Max 30");
    ImGui::TableHeadersRow();
  };
  if (ImGui::BeginTable("perf_rows", 5, ImGuiTableFlags_Borders)) {
    header();
    row("frame_time", "ms/frame", view.frame_now, view.frame_avg, view.max.frame, false);
    for (size_t i = 0; i < kPerfCostCount; ++i) {
      const PerfRole role = kPerfCostInfo[i].role;
      if (role == kPerfInPresent || role == kPerfDrawTime) row(kPerfCostInfo[i].name, "ms/present", view.now[i], view.avg[i], view.max.row[i], false);
    }
    const bool clamped = view.unattr_raw_now < -0.01 || view.unattr_raw_avg < -0.01;
    // Dont change the name longer than unattributed_cpu (c)
    row(clamped ? "unattributed_cpu (c)" : "unattributed_cpu", "ms/present", view.unattr_now, view.unattr_avg, view.max.unattr, clamped);
    for (size_t i = 0; i < kPerfCostCount; ++i) {
      if (kPerfCostInfo[i].role == kPerfGpu) row(kPerfCostInfo[i].name, "ms/measured frame", view.now[i], view.avg[i], view.max.row[i], false);
    }
    row("cpu_total", "ms/present", view.cpu_now, view.cpu_avg, view.max.cpu, false);
    row("gpu_total", "ms/measured frame", view.gpu_now, view.gpu_avg, view.max.gpu, false);
    ImGui::EndTable();
  }
  if (ImGui::CollapsingHeader("Present handlers (brackets; not in the totals)")) {
    if (ImGui::BeginTable("perf_brackets", 5, ImGuiTableFlags_Borders)) {
      header();
      for (size_t i = 0; i < kPerfCostCount; ++i) {
        if (kPerfCostInfo[i].role == kPerfBracket) row(kPerfCostInfo[i].name, "ms/present", view.now[i], view.avg[i], view.max.row[i], false);
      }
      ImGui::EndTable();
    }
  }
  if (ImGui::CollapsingHeader("Subsets (parts of the rows above; not in the totals)")) {
    if (ImGui::BeginTable("perf_phases", 5, ImGuiTableFlags_Borders)) {
      header();
      for (size_t i = 0; i < kPerfCostCount; ++i) {
        if (kPerfCostInfo[i].role == kPerfSubset) row(kPerfCostInfo[i].name, "ms/present", view.now[i], view.avg[i], view.max.row[i], false);
      }
      ImGui::EndTable();
    }
  }
  if (ImGui::CollapsingHeader("Largest frame spike of the last 30 presents")) {
    const PerfSpike& spike = view.spike;
    if (!spike.valid) {
      ImGui::TextDisabled("no present in the window");
    } else {
      size_t order[kPerfBlockRebuildCall];
      for (size_t j = 0; j < kPerfBlockRebuildCall; ++j) order[j] = j;
      std::partial_sort(order, order + 3, order + kPerfBlockRebuildCall, [&spike](size_t a, size_t b) {
        return spike.value[kPerfBlockRow + a] > spike.value[kPerfBlockRow + b];
      });
      std::string top;
      for (size_t j = 0; j < 3 && spike.value[kPerfBlockRow + order[j]] > 0.05; ++j) {
        char part[96];
        std::snprintf(part, sizeof part, "%s%s %.1f", j != 0u ? ", " : "", kPerfCostInfo[kPerfBlockRow + order[j]].name,
                      spike.value[kPerfBlockRow + order[j]]);
        top += part;
      }
      char line[512];
      std::snprintf(line, sizeof line, "frame %.1f ms, %zu presents ago; CPU %.1f ms (unattributed %.1f, handlers %.1f). Top blocks: %s", spike.frame_ms,
                    view.spike_age, spike.cpu_total, spike.unattr, spike.bracket, top.empty() ? "no block over 0.05 ms: the spike is outside the timed blocks" : top.c_str());
      ImGui::TextWrapped("%s", line);
    }
  }
  if (ImGui::CollapsingHeader("Ray Tracing setting costs (live parts; A/B parts in the file)")) {
    if (ImGui::BeginTable("perf_settings", 3, ImGuiTableFlags_Borders)) {
      ImGui::TableSetupColumn("Setting");
      ImGui::TableSetupColumn("CPU avg 30 (ms/present)");
      ImGui::TableSetupColumn("GPU avg 30 (ms/measured frame)");
      ImGui::TableHeadersRow();
      for (const PerfSetting& setting : kPerfSettings) {
        const PerfSettingValue sums = PerfSettingSums(view.avg, setting);
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextUnformatted(setting.key);
        ImGui::TableSetColumnIndex(1);
        cell(sums.cpu);
        ImGui::TableSetColumnIndex(2);
        cell(sums.gpu);
      }
      ImGui::EndTable();
    }
  }
  ImGui::TextDisabled("Last %u presents: now = newest present, avg = mean of the window, max = largest of the window (per row; the totals are the largest per-sample totals, so they need not equal the sum of the row maxima). CPU rows are per present; GPU rows are per measured frame (trace, alpha blit, P2b expand, deform refit, RTAO trace, RTAO filter). Unattributed CPU = the two present handlers minus the rows inside them.",
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

inline bool DrawBvhPanel() {
  using renodx::utils::settings::FindSetting;
  bool changed = false;

  ImGui::TextDisabled("Pool admits draws whose vertex shader the bytecode classifier marks rigid and whose pixel shader does not alpha-test.");
  static const char* const kBvhInputsText[] = {"idle (nothing runs)", "Pool Scan or debug view (follows the rows below)", "forced on by RT effects"};
  ImGui::TextDisabled("BVH inputs: %s", kBvhInputsText[g_bvh_debug.inputs.load(std::memory_order_relaxed)]);

  bool scan = g_pool.scan_active.load(std::memory_order_relaxed);
  if (ImGui::Checkbox("Pool Scan", &scan)) {
    g_pool.scan_active.store(scan, std::memory_order_relaxed);
    g_pool.scan_active_setting = scan ? 1.f : 0.f;
    if (auto* s = FindSetting("WorldPoolScan")) { s->Set(g_pool.scan_active_setting); s->Write(); }
    RefreshPoolCaptureRequest();
    LogPoolSwitches();
    changed = true;
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
  ImGui::SameLine();
  if (ImGui::Button("Log depth targets (2 frames)")) {
    g_state.depth_log_start_frame.store(g_state.frame.load() + 1u);
  }
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("Writes depth_targets.log to the pool output folder two frames after the click. Diagnostic only; no rendering change. Tick Gather performance metrics first for GPU times.");
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
      g_pool.capture_meshes_setting = capture_meshes ? 1.f : 0.f;
      if (auto* s = FindSetting("WorldPoolCaptureMeshes")) { s->Set(g_pool.capture_meshes_setting); s->Write(); }
      switches_changed = true;
      changed = true;
    }
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("Copies the index, then the vertex range of each new mesh at its draws and reads them\n"
                        "two frames later (no GPU wait). Off: no new mesh is read; instances of meshes\n"
                        "not read yet wait unadmitted.");
    }
    ImGui::SameLine();
    if (ImGui::Checkbox("Log mesh captures", &log_captures)) {
      g_pool.log_captures.store(log_captures, std::memory_order_relaxed);
      g_pool.log_captures_setting = log_captures ? 1.f : 0.f;
      if (auto* s = FindSetting("WorldPoolLogCaptures")) { s->Set(g_pool.log_captures_setting); s->Write(); }
      switches_changed = true;
      changed = true;
    }
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("Writes ReShade.log lines for each mesh copy and read,\n"
                        "and when buffers the pool tracks are released.");
    }
    bool verify_meshes = g_pool.verify_meshes.load(std::memory_order_relaxed);
    bool legacy_scale = g_pool.legacy_scale.load(std::memory_order_relaxed);
    if (ImGui::Checkbox("Verify mesh captures", &verify_meshes)) {
      g_pool.verify_meshes.store(verify_meshes, std::memory_order_relaxed);
      g_pool.verify_meshes_setting = verify_meshes ? 1.f : 0.f;
      if (auto* s = FindSetting("WorldPoolVerifyMeshes")) { s->Set(g_pool.verify_meshes_setting); s->Write(); }
      switches_changed = true;
      changed = true;
    }
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("A mesh enters the pool only after two separate captures decode to the same mesh.\n"
                        "Differing captures are counted and written to ReShade.log; a mesh that never\n"
                        "repeats is rejected. Applies to captures from now on: reset the pool after changing.");
    }
    ImGui::SameLine();
    if (ImGui::Checkbox("Legacy instance scale limits (0.05-50)", &legacy_scale)) {
      g_pool.legacy_scale.store(legacy_scale, std::memory_order_relaxed);
      g_pool.legacy_scale_setting = legacy_scale ? 1.f : 0.f;
      if (auto* s = FindSetting("WorldPoolLegacyScale")) { s->Set(g_pool.legacy_scale_setting); s->Write(); }
      switches_changed = true;
      changed = true;
    }
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("Rejects instances with an axis scale outside 0.05-50 (the limits before the\n"
                        "draw-time capture rework) instead of 0.001-10000. For an A/B test: reset the pool\n"
                        "after changing.");
    }
    bool poison_staging = g_pool.poison_staging.load(std::memory_order_relaxed);
    if (ImGui::Checkbox("Poison mesh staging (diagnostic)", &poison_staging)) {
      g_pool.poison_staging.store(poison_staging, std::memory_order_relaxed);
      g_pool.poison_staging_setting = poison_staging ? 1.f : 0.f;
      if (auto* s = FindSetting("WorldPoolPoisonStaging")) { s->Set(g_pool.poison_staging_setting); s->Write(); }
      switches_changed = true;
      changed = true;
    }
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("Mesh staging is cpu-visible and filled with a poison word after each read; each mesh\n"
                        "copy then records its head bytes and where they came from (ReShade.log \"staging:\" and\n"
                        "world_pool.json). Diagnostic only: reset the pool after changing.");
    }
    bool retry_unstable = g_pool.retry_unstable.load(std::memory_order_relaxed);
    if (ImGui::Checkbox("Retry unstable meshes (diagnostic)", &retry_unstable)) {
      g_pool.retry_unstable.store(retry_unstable, std::memory_order_relaxed);
      g_pool.retry_unstable_setting = retry_unstable ? 1.f : 0.f;
      if (auto* s = FindSetting("WorldPoolRetryUnstable")) { s->Set(g_pool.retry_unstable_setting); s->Write(); }
      switches_changed = true;
      changed = true;
    }
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("Off (default): a mesh whose captures never repeat is rejected at once.\n"
                        "On: it is captured again up to 3 times, 180 frames apart. Applies to captures\n"
                        "from now on: reset the pool after changing.");
    }
    bool alpha_indirect = g_pool.alpha_indirect_source.load(std::memory_order_relaxed);
    if (ImGui::Checkbox("Indirect foliage source copy (experimental)", &alpha_indirect)) {
      g_pool.alpha_indirect_source.store(alpha_indirect, std::memory_order_relaxed);
      g_pool.alpha_indirect_source_setting = alpha_indirect ? 1.f : 0.f;
      if (auto* s = FindSetting("WorldPoolAlphaIndirectSource")) { s->Set(g_pool.alpha_indirect_source_setting); s->Write(); }
      switches_changed = true;
      changed = true;
    }
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("Experimental, session only, default on. Effective only with Alpha-tested foliage on. On: indirect "
                        "alpha draws (wind and other indirect foliage) get a source copy and can enter the BVH. Off: they "
                        "leave the BVH at the next present.");
    }
    bool alpha_wind = g_pool.alpha_wind_opaque.load(std::memory_order_relaxed);
    if (ImGui::Checkbox("Admit wind with opaque pixel shader (experimental)", &alpha_wind)) {
      g_pool.alpha_wind_opaque.store(alpha_wind, std::memory_order_relaxed);
      g_pool.alpha_wind_opaque_setting = alpha_wind ? 1.f : 0.f;
      if (auto* s = FindSetting("WorldPoolAlphaWindOpaque")) { s->Set(g_pool.alpha_wind_opaque_setting); s->Write(); }
      switches_changed = true;
      changed = true;
    }
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("Experimental, session only, default on. Effective only with Alpha-tested foliage on. On: wind "
                        "vertex shaders (opaque or alpha-tested pixel shader) are admitted as a rigid rest pose instead of WindOpaque. "
                        "Off: their instances leave the BVH at the next present.");
    }
    bool log_crashes = g_crash_log.enabled_setting > 0.5f;
    if (ImGui::Checkbox("Log crashes", &log_crashes)) {
      g_crash_log.enabled_setting = log_crashes ? 1.f : 0.f;
      SetCrashLogEnabled(log_crashes);
      if (auto* s = FindSetting("WorldCrashLog")) { s->Set(g_crash_log.enabled_setting); s->Write(); }
      changed = true;
    }
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
  if (ImGui::Checkbox("Live BVH", &live)) {
    g_live_bvh.enabled.store(live, std::memory_order_relaxed);
    g_live_bvh.enabled_setting = live ? 1.f : 0.f;
    if (auto* s = FindSetting("WorldLiveBvh")) { s->Set(g_live_bvh.enabled_setting); s->Write(); }
    changed = true;
  }
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("Keeps the GPU BVH in step with the pool every frame: new meshes stream in with their BLAS\n"
                      "built on the CPU (%llu triangles per frame), and the region TLAS is rebuilt when instances,\n"
                      "meshes or the region change (at most every %u frames). Off: the GPU BVH stays as it is.",
                      static_cast<unsigned long long>(kLiveTrianglesPerFrame), kLiveTlasInterval);
  }
  ImGui::SameLine();
  if (ImGui::Button("Rebuild BVH")) g_live_bvh.reset_requested.store(true, std::memory_order_relaxed);
  if (ImGui::IsItemHovered()) ImGui::SetTooltip("Drops every GPU mesh and streams the pool in again.");
  if (ImGui::Button("Run DX12 probe")) rt12::g_rt12_probe.requested.store(true, std::memory_order_relaxed);
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("Creates a Direct3D 12 device on this GPU, logs its ray tracing tier and the D3D11/D3D12 sync cost, then releases it. Waits for the GPU: expect a short hitch.");
  }
  std::string rt12_result;
  {
    std::lock_guard<std::mutex> lock(rt12::g_rt12_probe.mutex);
    rt12_result = rt12::g_rt12_probe.result;
  }
  if (!rt12_result.empty()) {
    ImGui::TextUnformatted(rt12_result.c_str());
  } else {
    ImGui::TextDisabled("DX12 probe: not run");
  }
  bool dxr = rt12::g_rt12_enabled.load(std::memory_order_relaxed);
  if (ImGui::Checkbox("DXR backend (experimental)", &dxr)) rt12::g_rt12_enabled.store(dxr, std::memory_order_relaxed);
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("Session only. RTAO pass A traces on a Direct3D 12 device with hardware ray tracing (DXR 1.1). Turning it on reloads the BVH (a few frames without RT). Characters are traced on DXR. Off: the compute path, exactly as before.");
  }
  ImGui::SameLine();
  if (ImGui::Button("Dump DXR JSON")) rt12::DumpRt12Json();
  rt12::Rt12Panel dxr_panel;
  {
    std::lock_guard<std::mutex> lock(rt12::g_rt12_panel_mutex);
    dxr_panel = rt12::g_rt12_panel;
  }
  std::string dxr_line = std::string("DXR: ") + rt12::Rt12ReasonName(dxr_panel.reason) + "  traced " + std::to_string(dxr_panel.traced_frames)
                         + "  busy " + std::to_string(dxr_panel.busy_frames) + "  wrapped " + (dxr_panel.wrapped ? "yes" : "no");
  dxr_line += ", P2b records " + std::to_string(dxr_panel.records);
  if (dxr_panel.stuck) dxr_line += "  STUCK (objects leaked)";
  if (!dxr_panel.failed_stage.empty()) {
    dxr_line += "  failed at " + dxr_panel.failed_stage + " hr=" + renodx::utils::log::AsHex(static_cast<uint32_t>(dxr_panel.hr));
  }
  ImGui::TextUnformatted(dxr_line.c_str());
  char dxr_scene[256] = {};
  std::snprintf(dxr_scene, sizeof(dxr_scene), "DXR scene: %u meshes, %u BLAS (%.1f MB), TLAS %u (no BLAS %u, holes %u, bad mesh %u), sanitized triangles %llu, alpha mirror %s, "
                "characters %u (%u BLAS, %.1f MB, builds %llu, refits %llu)",
                dxr_panel.mesh_count, dxr_panel.blas_count, static_cast<double>(dxr_panel.blas_bytes) / (1024.0 * 1024.0),
                dxr_panel.tlas_instances, dxr_panel.tlas_no_blas, dxr_panel.tlas_holes, dxr_panel.tlas_bad_mesh,
                static_cast<unsigned long long>(dxr_panel.sanitized_triangles), dxr_panel.alpha_mirror ? "yes" : "no",
                dxr_panel.dynamic_objects, dxr_panel.dynamic_blas_count,
                static_cast<double>(dxr_panel.dynamic_blas_bytes) / (1024.0 * 1024.0),
                static_cast<unsigned long long>(dxr_panel.dynamic_builds), static_cast<unsigned long long>(dxr_panel.dynamic_refits));
  ImGui::TextUnformatted(dxr_scene);
  const auto dxr_ms = [](float sum, uint32_t n) {
    char text[32] = {};
    if (n == 0u) {
      std::snprintf(text, sizeof(text), "--");
    } else {
      std::snprintf(text, sizeof(text), "%.3f", static_cast<double>(sum) / n);
    }
    return std::string(text);
  };
  const std::string dxr_gpu = "DXR GPU ms (D3D12 timestamps, avg): BLAS " + dxr_ms(dxr_panel.blas_ms_sum, dxr_panel.blas_ms_n)
                              + " characters " + dxr_ms(dxr_panel.dynamic_ms_sum, dxr_panel.dynamic_ms_n) + " P2b "
                              + dxr_ms(dxr_panel.records_ms_sum, dxr_panel.records_ms_n) + " TLAS "
                              + dxr_ms(dxr_panel.tlas_ms_sum, dxr_panel.tlas_ms_n) + " trace " + dxr_ms(dxr_panel.trace_ms_sum, dxr_panel.trace_ms_n);
  ImGui::TextUnformatted(dxr_gpu.c_str());
  bool gc_on = bvh::g_gpu_capture.enabled.load(std::memory_order_relaxed);
  if (ImGui::Checkbox("GPU geometry cache (experimental)", &gc_on)) bvh::g_gpu_capture.enabled.store(gc_on, std::memory_order_relaxed);
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("Session only. Needs the DXR backend. Copies and decodes the static meshes on the GPU into a separate D3D12 cache with its own BLAS. Validation only: nothing traces it. Off: no GPU cache work at all.");
  }
  ImGui::SameLine();
  if (ImGui::Button("Check GPU geometry")) rt12::g_gc_check_requested.store(true, std::memory_order_relaxed);
  ImGui::SameLine();
  if (ImGui::Button("Dump GPU cache JSON")) rt12::g_gc_dump_requested.store(true, std::memory_order_relaxed);
  rt12::Rt12GcPanel gc_panel;
  {
    std::lock_guard lock(rt12::g_gc_panel_mutex);
    gc_panel = rt12::g_gc_panel;
  }
  const uint64_t gc_range = gc_panel.phase_counts[0] + gc_panel.phase_counts[1];
  const uint64_t gc_data = gc_panel.phase_counts[2] + gc_panel.phase_counts[3];
  const uint64_t gc_entries = gc_range + gc_data + gc_panel.phase_counts[4] + gc_panel.phase_counts[5];
  const std::string gc_state = "GPU cache: " + gc_panel.state + "  entries " + std::to_string(gc_entries) + " (range " + std::to_string(gc_range)
                               + ", data " + std::to_string(gc_data) + ", done " + std::to_string(gc_panel.phase_counts[4]) + ", refused "
                               + std::to_string(gc_panel.phase_counts[5]) + "), resident " + std::to_string(gc_panel.resident) + " of "
                               + std::to_string(gc_panel.pool_meshes) + " pool meshes, pending " + std::to_string(gc_panel.pending)
                               + ", BLAS queue " + std::to_string(gc_panel.blas_queue);
  ImGui::TextUnformatted(gc_state.c_str());
  constexpr double kMb = 1024.0 * 1024.0;
  const double gc_used = static_cast<double>(gc_panel.arena_used[0] + gc_panel.arena_used[1] + gc_panel.arena_used[2]) / kMb;
  char gc_memory[256] = {};
  std::snprintf(gc_memory, sizeof(gc_memory),
                "GPU cache memory: geometry %.1f / %.0f MB (dead %.1f MB), BLAS %.1f MB, D3D11 112 MB, linear 32 MB; build GPU ms %s, D3D11 decode ms %s",
                gc_used, static_cast<double>(bvh::kGcGeometryBytesMax) / kMb, static_cast<double>(gc_panel.dead_bytes) / kMb,
                static_cast<double>(gc_panel.blas_bytes) / kMb, dxr_ms(static_cast<float>(gc_panel.build_ms_sum), static_cast<uint32_t>(gc_panel.build_ms_n)).c_str(),
                dxr_ms(static_cast<float>(gc_panel.decode_ms_sum), static_cast<uint32_t>(gc_panel.decode_ms_n)).c_str());
  ImGui::TextUnformatted(gc_memory);
  std::string gc_check_line = "Check GPU geometry: not run";
  if (gc_panel.check.run) {
    char wait[32] = {};
    std::snprintf(wait, sizeof(wait), "%.1f", gc_panel.check.wait_ms);
    uint64_t cpu_failed = 0u;
    for (const auto& [reason, count] : gc_panel.check.cpu_failed) cpu_failed += count;
    gc_check_line = "Check GPU geometry: first " + std::to_string(gc_panel.check.checked + gc_panel.check.cpu_absent + cpu_failed) + " meshes with a BLAS by key: "
                    + std::to_string(gc_panel.check.checked) + " meshes, " + std::to_string(gc_panel.check.triangles)
                    + " triangles, " + std::to_string(gc_panel.check.mismatched_triangles) + " mismatched (" + std::to_string(gc_panel.check.mismatched_meshes)
                    + " meshes), cpu absent " + std::to_string(gc_panel.check.cpu_absent) + ", cpu failed " + std::to_string(cpu_failed)
                    + ", uv size mismatch " + std::to_string(gc_panel.check.uv_size_mismatch) + ", wait " + wait + " ms";
  }
  ImGui::TextUnformatted(gc_check_line.c_str());
  bool skip_noop = g_live_bvh.skip_noop.load(std::memory_order_relaxed);
  if (ImGui::Checkbox("Skip no-op TLAS rebuilds", &skip_noop)) {
    g_live_bvh.skip_noop.store(skip_noop, std::memory_order_relaxed);
    g_live_bvh.skip_noop_setting = skip_noop ? 1.f : 0.f;
    if (auto* s = FindSetting("WorldLiveBvhSkipNoop")) { s->Set(g_live_bvh.skip_noop_setting); s->Write(); }
    changed = true;
  }
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("A TLAS rebuild whose instances equal the TLAS already in place is skipped (no sort, build or buffer\n"
                      "creation). Default OFF until the in-game A/B; off: every rebuild runs, as before. Session only, not saved.");
  }
  bool slack = g_live_bvh.slack.load(std::memory_order_relaxed);
  if (ImGui::Checkbox("Slack TLAS (experimental, session only)", &slack)) {
    g_live_bvh.slack.store(slack, std::memory_order_relaxed);
    g_live_bvh.slack_setting = slack ? 1.f : 0.f;
    if (auto* s = FindSetting("WorldLiveBvhSlack")) { s->Set(g_live_bvh.slack_setting); s->Write(); }
    changed = true;
  }
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("Keeps spare slots in the TLAS so most changes are written in place (no sort, build or buffer creation).\n"
                      "Trade: tree quality drifts until the next full rebuild (at most every 120 frames or when the slots run out).\n"
                      "Off by default. Session only, not saved.");
  }

  int mode = g_bvh_debug.mode.load(std::memory_order_relaxed);
  const char* modes = "Off\0BVH Trace (Shaded)\0BVH Trace (Instance ID)\0Depth Compare\0";
  if (ImGui::Combo("GPU Debug", &mode, modes)) {
    g_bvh_debug.mode.store(mode, std::memory_order_relaxed);
    g_bvh_debug.mode_setting = static_cast<float>(mode);
    if (auto* s = FindSetting("WorldGpuDebugMode")) { s->Set(g_bvh_debug.mode_setting); s->Write(); }
    RefreshPoolCaptureRequest();
    if (mode != static_cast<int>(BvhView::Off)) g_bvh_trace.stats_requested.store(true, std::memory_order_relaxed);
    changed = true;
  }
  bool hiding = g_bvh_trace.hide_camera_hidden.load(std::memory_order_relaxed);
  if (ImGui::Checkbox("Hide what the game camera does not show", &hiding)) {
    g_bvh_trace.hide_camera_hidden.store(hiding, std::memory_order_relaxed);
    g_bvh_trace.hide_camera_hidden_setting = hiding ? 1.f : 0.f;
    if (auto* s = FindSetting("WorldGpuDebugHideCameraHidden")) { s->Set(g_bvh_trace.hide_camera_hidden_setting); s->Write(); }
    g_bvh_trace.stats_requested.store(true, std::memory_order_relaxed);
    changed = true;
  }
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("Some BVH instances are never drawn by a camera vertex shader, only into the shadow maps\n"
                      "(shadow-only casters), and the game near-fades map objects very close to the camera.\n"
                      "Both stay in the BVH (shadow casters are wanted for ray-traced shadows).\n"
                      "On: the trace views skip them the way the game camera does.\n"
                      "Off: they stay visible and are counted as hidden-from-camera pixels.\n"
                      "Default on.");
  }
  bool counters_every_frame = g_bvh_trace.counters_every_frame.load(std::memory_order_relaxed);
  if (ImGui::Checkbox("Trace counters every frame (diagnostic, session only)", &counters_every_frame)) {
    g_bvh_trace.counters_every_frame.store(counters_every_frame, std::memory_order_relaxed);
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
    return changed;
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
  if (ImGui::Checkbox("Collect cache diagnostics", &collect)) {
    g_cache_diag.enabled.store(collect, std::memory_order_relaxed);
    g_cache_diag.enabled_setting = collect ? 1.f : 0.f;
    if (auto* s = FindSetting("WorldCacheDiag")) { s->Set(g_cache_diag.enabled_setting); s->Write(); }
    changed = true;
  }
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
  return changed;
}

}  // namespace falcom_world::bvh
