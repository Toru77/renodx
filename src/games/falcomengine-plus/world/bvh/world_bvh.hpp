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

// Stream-out probe of deforming vertex shaders (deform_probe.hpp).
inline void DrawDeformProbePanel() {
  ImGui::SeparatorText("Deforming meshes (stream-out probe)");
  bool enabled = g_deform.enabled.load(std::memory_order_relaxed);
  if (ImGui::Checkbox("Probe deforming shaders", &enabled)) {
    g_deform.enabled.store(enabled, std::memory_order_relaxed);
    RefreshPoolCaptureRequest();
  }
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("Re-runs a few draws of skinned, wind, billboard and animated vertex shaders per frame\n"
                      "(each shader at most every %u frames) with a stream-output geometry shader and nothing\n"
                      "rasterized, then finds which output is the world position. Discovery only: nothing\n"
                      "enters the BVH.",
                      kDeformProbeInterval);
  }
  ImGui::SameLine();
  if (ImGui::Button("Dump Deform JSON")) DumpDeformProbe();
  ImGui::SameLine();
  if (ImGui::Button("Reset Probe Results")) ResetDeformProbeResults();
  DeformStats stats;
  const std::vector<DeformShaderRow> rows = SnapshotDeformShaders(&stats);
  ImGui::Text("Probes %llu  read %llu  world position found %llu   shaders built %u  failed %u",
              static_cast<unsigned long long>(stats.probes), static_cast<unsigned long long>(stats.reads),
              static_cast<unsigned long long>(stats.matched), stats.shaders_created, stats.shader_failures);
  if (stats.resource_failures != 0u || stats.map_failures != 0u) {
    ImGui::TextColored(ImVec4(1.f, 0.4f, 0.4f, 1.f), "Resource failures %u  readback map failures %u",
                       stats.resource_failures, stats.map_failures);
  }
  std::string skips;
  for (size_t i = 1; i < stats.skips.size(); ++i) {
    AppendPoolCount(&skips, DeformSkipName(static_cast<DeformSkip>(i)), stats.skips[i]);
  }
  if (!skips.empty()) ImGui::TextDisabled("Not probed: %s", skips.c_str());
  std::string fails;
  for (size_t i = 1; i < stats.read_fails.size(); ++i) {
    AppendPoolCount(&fails, DeformReadFailName(static_cast<DeformReadFail>(i)), stats.read_fails[i]);
  }
  if (!fails.empty()) ImGui::TextDisabled("Reads not judged: %s", fails.c_str());
  if (rows.empty()) return;
  if (ImGui::BeginTable("deform_probe", 7, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit)) {
    ImGui::TableSetupColumn("VS");
    ImGui::TableSetupColumn("Class");
    ImGui::TableSetupColumn("Probes/read/found");
    ImGui::TableSetupColumn("World position");
    ImGui::TableSetupColumn("Error");
    ImGui::TableSetupColumn("Last capture");
    ImGui::TableSetupColumn("Status");
    ImGui::TableHeadersRow();
    for (const DeformShaderRow& row : rows) {
      const DeformShader& shader = row.shader;
      ImGui::TableNextRow();
      ImGui::TableNextColumn();
      ImGui::Text("0x%08X", shader.hash);
      ImGui::TableNextColumn();
      ImGui::TextUnformatted(contract::VsClassName(static_cast<contract::VsClass>(shader.cls)));
      ImGui::TableNextColumn();
      ImGui::Text("%u / %u / %u", shader.probes, shader.reads, shader.matched);
      ImGui::TableNextColumn();
      ImGui::TextUnformatted(DeformChosenText(shader).c_str());
      ImGui::TableNextColumn();
      if (shader.matched != 0u) {
        ImGui::Text("%.2g", shader.worst_error);
      } else {
        ImGui::TextDisabled("-");
      }
      ImGui::TableNextColumn();
      if (shader.reads != 0u && shader.last_indirect) {
        ImGui::Text("%u tris (indirect)", shader.last.triangles);
      } else if (shader.reads != 0u) {
        ImGui::Text("%u tris (%u x %u)", shader.last.triangles, shader.last_index_count / 3u, shader.last_instance_count);
      } else {
        ImGui::TextDisabled("-");
      }
      ImGui::TableNextColumn();
      std::string status;
      if (!shader.layout_ok) {
        status = shader.layout_error;
      } else if (shader.create_failed) {
        char text[48] = {};
        std::snprintf(text, sizeof(text), "shader failed (hr 0x%08X)", static_cast<uint32_t>(shader.create_hr));
        status = text;
      } else if (shader.inconsistent) {
        status = "outputs differ between probes";
      } else if (shader.reads != 0u && shader.matched == 0u) {
        status = "no output matches";
      } else if (shader.matched != 0u) {
        status = "ok";
      } else {
        for (size_t i = 1; i < shader.skips.size(); ++i) AppendPoolCount(&status, DeformSkipName(static_cast<DeformSkip>(i)), shader.skips[i]);
      }
      ImGui::TextUnformatted(status.c_str());
    }
    ImGui::EndTable();
  }
}

// Deforming meshes in the BVH (deform_live.hpp).
inline void DrawDeformLivePanel() {
  bool enabled = g_deform_live.enabled.load(std::memory_order_relaxed);
  if (ImGui::Checkbox("Deforming meshes in the BVH (characters, water)", &enabled)) {
    g_deform_live.enabled.store(enabled, std::memory_order_relaxed);
    if (enabled) g_deform.enabled.store(true, std::memory_order_relaxed);  // confirms each shader's world position
    RefreshPoolCaptureRequest();
  }
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("Every frame, re-runs each camera draw of a skinned or animated vertex shader whose world\n"
                      "position the probe confirmed, capturing its world-space triangles on the GPU. Each draw's\n"
                      "tree is built once from its first capture and refit every frame; the trace walks them\n"
                      "beside the static BVH. Turns the probe on (it confirms new shaders).");
  }
  DeformLiveStats stats;
  uint32_t traced = 0u;
  float refit_ms = -1.f;
  {
    std::lock_guard<std::mutex> lock(g_deform_live.mutex);
    stats = g_deform_live.stats;
    traced = g_deform_live.object_count;
    refit_ms = g_deform_live.refit_timer.last_ms;
  }
  if (!enabled) return;
  ImGui::Text("This frame: captured %u draws (%llu vertices), traced %u, waiting for their tree %u",
              stats.frame_captures, static_cast<unsigned long long>(stats.frame_vertices), traced, stats.frame_pending);
  ImGui::Text("Draw identities %u: with a tree %u, failed %u   trees built %u, retired %u, store resets %u (%.1f MB used)",
              stats.identities, stats.resident, stats.failed, stats.blas_built, stats.retired, stats.resets,
              static_cast<double>(stats.used_bytes) / (1024.0 * 1024.0));
  if (refit_ms >= 0.f) ImGui::TextDisabled("GPU refit %.3f ms", refit_ms);
  std::string skips;
  for (size_t i = 1; i < stats.skips.size(); ++i) AppendPoolCount(&skips, DynamicSkipName(static_cast<DynamicSkip>(i)), stats.skips[i]);
  if (!skips.empty()) ImGui::TextDisabled("Not captured: %s", skips.c_str());
  if (stats.resource_failures != 0u || stats.map_failures != 0u || stats.topology_mismatch != 0u || stats.object_cap_drops != 0u) {
    ImGui::TextColored(ImVec4(1.f, 0.4f, 0.4f, 1.f), "Resource failures %u  map failures %u  size mismatches %u  object cap %u",
                       stats.resource_failures, stats.map_failures, stats.topology_mismatch, stats.object_cap_drops);
  }
  if (!stats.last_failure.empty()) ImGui::TextDisabled("Last failure: %s", stats.last_failure.c_str());
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
    ImGui::TextDisabled("Indirect: copied %llu  read %llu  empty %llu  truncated %llu  buffer released %llu  avg window %.0f  "
                        "first-time (full window) %llu",
                        static_cast<unsigned long long>(stats.indirect_copied),
                        static_cast<unsigned long long>(stats.indirect_resolved),
                        static_cast<unsigned long long>(stats.indirect_empty),
                        static_cast<unsigned long long>(stats.indirect_truncated),
                        static_cast<unsigned long long>(stats.indirect_dead),
                        stats.indirect_copied != 0u
                            ? static_cast<double>(stats.indirect_window_instances) / static_cast<double>(stats.indirect_copied)
                            : 0.0,
                        static_cast<unsigned long long>(stats.indirect_first_copies));
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
  {
    // Motion probe (path 2 discovery): prevWorld vs world per sighting.
    const PoolMotionStats& motion = stats.motion;
    uint64_t repeat_noise = 0u;
    for (size_t i = 0; i < kPoolMotionUlpBuckets; ++i) {
      if (i <= PoolMotionUlpBucket(kPoolMotionUlpNoise)) repeat_noise += motion.repeat_ulps[i];
    }
    ImGui::Text("Motion (prevWorld vs world): matrix seen in an earlier frame %llu, within %u ULP %.1f%%, max %u ULP",
                static_cast<unsigned long long>(motion.repeat), kPoolMotionUlpNoise,
                motion.repeat != 0u ? 100.0 * static_cast<double>(repeat_noise) / static_cast<double>(motion.repeat) : 0.0,
                motion.max_repeat_ulps);
    ImGui::Text("  moved > 1 mm / rotated: %llu sightings (%llu of a known matrix) in %llu draws (%llu all moving, %llu indirect), %u meshes",
                static_cast<unsigned long long>(motion.moved), static_cast<unsigned long long>(motion.moved_repeat),
                static_cast<unsigned long long>(motion.moving_draws),
                static_cast<unsigned long long>(motion.moving_draws_all),
                static_cast<unsigned long long>(motion.moving_draws_indirect), motion.moved_meshes);
    ImGui::Text("  camera: unmoved %llu (%llu bit-exact), prevWorld zero %llu; shadow: prevWorld zero %llu of %llu unmoved",
                static_cast<unsigned long long>(motion.camera_repeat),
                static_cast<unsigned long long>(motion.camera_repeat_exact),
                static_cast<unsigned long long>(motion.camera_zero_prev),
                static_cast<unsigned long long>(motion.light_zero_prev),
                static_cast<unsigned long long>(motion.light_repeat));
  }
  {
    // Moving rigid objects (P2a).
    const PoolMotionStats& motion = stats.motion;
    ImGui::Text("Moving objects: %zu draw keys moving now (%zu seen), %zu meshes flagged, %u instances removed, %llu admissions refused",
                stats.dynamic_moving_keys, stats.dynamic_live_keys, stats.dynamic_meshes, stats.dynamic_retired,
                static_cast<unsigned long long>(stats.dynamic_blocked));
    ImGui::TextDisabled("  follow: %llu pose updates, %llu relinked, %llu admitted directly, %llu orphans (%llu retired), %llu budget skips",
                        static_cast<unsigned long long>(stats.follow_hits), static_cast<unsigned long long>(stats.follow_relinks),
                        static_cast<unsigned long long>(stats.follow_admits), static_cast<unsigned long long>(stats.orphans),
                        static_cast<unsigned long long>(stats.orphans_retired), static_cast<unsigned long long>(stats.follow_budget_skips));
    ImGui::TextDisabled("  rule: %llu camera sightings with prevWorld differing, %llu stale (matrix seen before, not moving)",
                        static_cast<unsigned long long>(motion.rule_moving), static_cast<unsigned long long>(motion.rule_stale));
    if (stats.dynamic_cap_drops != 0u) ImGui::TextDisabled("  moving keys not recorded (cap): %u", stats.dynamic_cap_drops);
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
  {
    const bool unstable = stats.mesh_capture_mismatches != 0u || stats.mesh_unstable != 0u;
    ImGui::TextColored(unstable ? ImVec4(1.f, 0.8f, 0.3f, 1.f) : ImVec4(0.4f, 1.f, 0.4f, 1.f),
                       "Mesh verification: confirmed %u  captures that differed %u  rejected as unstable %u",
                       stats.mesh_verified, stats.mesh_capture_mismatches, stats.mesh_unstable);
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("Captures that differed: the same draw's index and vertex ranges read back different\n"
                        "content on two captures (the first ones are in ReShade.log).");
    }
  }
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
  bool do_trace = false;
  if (ImGui::Button("Dump Pool JSON")) do_dump = true;
  ImGui::SameLine();
  if (ImGui::Button("Trace prevWorld (3 keys)")) do_trace = true;
  ImGui::SameLine();
  if (ImGui::Button("Dump Region OBJ")) do_dump_obj = true;
  ImGui::SameLine();
  if (ImGui::Button("Reset Pool")) do_reset = true;
  ImGui::SameLine();
  if (ImGui::Button("Open Pool Folder")) {
    const auto folder = PoolOutputDir();
    ShellExecuteA(nullptr, "open", folder.string().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
  }
  if (do_trace) {
    std::lock_guard<std::mutex> lock(g_pool.mutex);
    g_pool.prev_trace = {};
    g_pool.prev_trace.armed_frame = g_state.frame.load();
  }
  if (do_dump) DumpWorldPool();
  if (do_dump_obj) DumpWorldPoolObj();
  if (do_reset) ResetWorldPool();

  DrawDeformProbePanel();
  DrawDeformLivePanel();

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
  ImGui::SameLine();
  if (ImGui::Button("Check GPU Contents")) g_live_bvh.check_requested.store(true, std::memory_order_relaxed);
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("Reads the GPU BVH back and compares it with what was uploaded.\n"
                      "Waits for the GPU: expect a short hitch.");
  }

  int mode = g_bvh_debug.mode.load(std::memory_order_relaxed);
  const char* modes = "Off\0BVH Trace (Shaded)\0BVH Trace (Instance ID)\0Depth Compare\0";
  if (ImGui::Combo("GPU Debug", &mode, modes)) {
    g_bvh_debug.mode.store(mode, std::memory_order_relaxed);
    RefreshPoolCaptureRequest();
    if (mode != static_cast<int>(BvhView::Off)) g_bvh_trace.stats_requested.store(true, std::memory_order_relaxed);
  }
  ImGui::SameLine();
  if (ImGui::Button("Read Stats")) {
    g_bvh_trace.stats_requested.store(true, std::memory_order_relaxed);
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
  const double megabyte = 1024.0 * 1024.0;
  ImGui::Text("Meshes on GPU: %u of %u  waiting %u  failed %u  retired %u   (%.1f MB used, %.1f MB allocated, %.1f MB retired)",
              live_stats.resident_meshes, live_stats.pool_meshes, live_stats.pending_meshes, live_stats.failed_meshes,
              live_stats.meshes_retired, static_cast<double>(live_stats.used_bytes) / megabyte,
              static_cast<double>(live_stats.capacity_bytes) / megabyte,
              static_cast<double>(live_stats.garbage_bytes) / megabyte);
  ImGui::TextDisabled("Uploaded %llu meshes (%llu triangles)  resident triangles %u  buffer grows %u  store resets %u%s%s",
                      static_cast<unsigned long long>(live_stats.meshes_uploaded),
                      static_cast<unsigned long long>(live_stats.triangles_uploaded),
                      live_stats.resident_triangles, live_stats.grows, live_stats.resets,
                      live_stats.last_reset_reason.empty() ? "" : "  last reset: ",
                      live_stats.last_reset_reason.c_str());
  const uint32_t frame = g_state.frame.load();
  ImGui::Text("TLAS: %u instances (never drawn by the camera %u, near-fade %u)  %u nodes  waiting for their mesh %u  mesh failed %u  rebuilds %u  (last: %s, %u frames ago)",
              live_stats.tlas_instances, live_stats.tlas_camera_hidden, live_stats.tlas_near_fade, live_stats.tlas_nodes,
              live_stats.tlas_waiting,
              live_stats.tlas_unusable,
              live_stats.tlas_rebuilds,
              live_stats.tlas_rebuilds != 0u ? live_stats.tlas_reason : "none",
              live_stats.tlas_rebuilds != 0u && frame >= live_stats.tlas_frame ? frame - live_stats.tlas_frame : 0u);
  ImGui::Text("CPU time: mesh uploads %.2f ms (max %.2f)  TLAS %.2f ms (max %.2f)",
              live_stats.mesh_ms_last, live_stats.mesh_ms_max, live_stats.tlas_ms_last, live_stats.tlas_ms_max);
  ImGui::Text("TLAS refits %u (%u instances last, lag last %u, max %u frames)  refit %.2f ms (max %.2f)  missing %u  failures %u",
              live_stats.tlas_refits, live_stats.refit_instances, live_stats.refit_lag_last, live_stats.refit_lag_max,
              live_stats.refit_ms_last, live_stats.refit_ms_max, live_stats.refit_missing, live_stats.refit_failures);
  if (data->trace_timer.last_ms >= 0.f) {
    ImGui::SameLine();
    ImGui::Text("  GPU trace (debug view) %.2f ms", data->trace_timer.last_ms);
  }
  if (live_stats.blas_invalid != 0u || live_stats.tlas_invalid != 0u || live_stats.upload_failures != 0u) {
    ImGui::TextColored(ImVec4(1.f, 0.4f, 0.4f, 1.f), "Failures: mesh BLAS %u  TLAS %u  GPU buffers %u   last: %s",
                       live_stats.blas_invalid, live_stats.tlas_invalid, live_stats.upload_failures,
                       live_stats.last_failure.c_str());
  } else if (!live_stats.last_failure.empty()) {
    ImGui::TextDisabled("Last: %s", live_stats.last_failure.c_str());
  }
  if (live_stats.gpu_checked) {
    const ImVec4 color = live_stats.gpu_ok ? ImVec4(0.4f, 1.f, 0.4f, 1.f) : ImVec4(1.f, 0.4f, 0.4f, 1.f);
    ImGui::TextColored(color, "GPU contents: %s", live_stats.gpu_result.c_str());
  }
  if (!data->bvh_ready) {
    ImGui::TextDisabled("BVH not traceable yet (%s).",
                        live_stats.resident_meshes == 0u ? "no mesh on the GPU" : "no instance of an uploaded mesh in the region");
  }
  if (mode != static_cast<int>(BvhView::Off)) {
    ImGui::TextDisabled("Inspect: middle-click a pixel of the trace view to see what the BVH hit there (also written to ReShade.log).");
  }
  for (const std::string& line : data->inspect.lines) ImGui::TextUnformatted(line.c_str());
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
    ImGui::Text("Pixels the game camera does not show: %u (%s)", data->trace_stats.camera_hidden,
                data->trace_stats.hiding ? "hidden, the ray continues behind" : "shown");
    DrawDepthCompareStats(data->trace_stats);
  }
}

}  // namespace falcom_world::bvh
