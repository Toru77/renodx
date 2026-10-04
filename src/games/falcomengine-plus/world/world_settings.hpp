#pragma once

// Ray Tracing tab content for Phase 0.
//
// The tab only exists when DevKit is present (world::AddSettings adds rows only
// when g_state.supported is true). The normal workflow is: enable World
// Research, press Auto Research, play normally. Manual capture/scan/overlay
// remains as a failure-investigation fallback.

#include <algorithm>
#include <cstdio>

#include <Windows.h>
#include <shellapi.h>

#include "world_state.hpp"
#include "capture/draw_census.hpp"
#include "research/auto_research.hpp"
#include "research/coverage.hpp"
#include "research/reference_hints.hpp"
#include "research/transform_candidates.hpp"
#include "research/world_report.hpp"

namespace falcom_world {

inline bool IsDevkitPresent() {
  if (GetModuleHandleA("renodx-devkit.addon64") != nullptr) return true;
  if (GetModuleHandleA("renodx-devkit.addon32") != nullptr) return true;
  static const bool file_present = []() {
    char exe_path[MAX_PATH] = {};
    const DWORD length = GetModuleFileNameA(nullptr, exe_path, MAX_PATH);
    if (length == 0 || length >= MAX_PATH) return false;
    const std::string path(exe_path, length);
    const auto slash = path.find_last_of("\\/");
    if (slash == std::string::npos) return false;
    const std::string directory = path.substr(0, slash + 1);
    static const char* const candidates[] = {
        "renodx-devkit.addon64",
        "renodx-devkit.addon32",
    };
    for (const auto* candidate : candidates) {
      if (GetFileAttributesA((directory + candidate).c_str()) != INVALID_FILE_ATTRIBUTES) return true;
    }
    return false;
  }();
  return file_present;
}

inline void ApplyFamilyLabel(uint32_t vs_hash, uint8_t label) {
  std::lock_guard<std::mutex> lock(g_state.mutex);
  const auto it = g_state.families.find(vs_hash);
  if (it != g_state.families.end()) {
    it->second.label = label;
    it->second.label_source = static_cast<uint8_t>(LabelSource::Manual);
    it->second.label_prefilled = true;
  }
}

inline void ApplyFamilyVerified(uint32_t vs_hash, bool verified) {
  std::lock_guard<std::mutex> lock(g_state.mutex);
  const auto it = g_state.families.find(vs_hash);
  if (it == g_state.families.end()) return;
  it->second.verified = verified;
  it->second.transform_found = false;
  if (!verified) return;
  if (g_state.captured.draw.vs_hash != vs_hash) return;
  if (g_state.candidates.empty()) return;
  const int index = std::clamp(
      g_state.selected_candidate, 0, static_cast<int>(g_state.candidates.size()) - 1);
  const TransformCandidate& candidate = g_state.candidates[index];
  it->second.transform_found = true;
  it->second.transform_kind = static_cast<uint8_t>(candidate.kind);
  it->second.transform_slot = candidate.slot;
  it->second.transform_offset = candidate.matrix_offset;
  it->second.transform_stride = candidate.stride;
  it->second.transform_base = candidate.base_offset;
  it->second.transform_has_prev = candidate.has_prev;
}

inline void PrefillHintLabels() {
  if (!CensusEnabled()) return;
  std::lock_guard<std::mutex> lock(g_state.mutex);
  for (auto& [hash, family] : g_state.families) {
    if (family.label_prefilled || family.label != 0u) continue;
    const ReferenceHint* hint = FindReferenceHint(hash);
    if (hint == nullptr) continue;
    family.label = static_cast<uint8_t>(hint->suggested_label);
    family.label_source = static_cast<uint8_t>(LabelSource::Hint);
    family.label_prefilled = true;
  }
}

inline void OpenLatestReport() {
  const std::filesystem::path path = LatestReportPath();
  if (!std::filesystem::exists(path)) return;
  ShellExecuteA(nullptr, "open", path.string().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

struct AutoProgressSnapshot {
  bool active = false;
  uint32_t rounds = 0u;
  uint32_t families_total = 0u;
  uint32_t families_done = 0u;
  uint32_t pending = 0u;
  uint32_t last_vs = 0u;
  uint32_t captures_done = 0u;
  uint32_t captures_target = 0u;
  uint32_t verified = 0u;
  uint32_t failed = 0u;
  uint32_t not_observed = 0u;
  uint32_t pending_verdict = 0u;
};

inline AutoProgressSnapshot SnapshotAutoProgress() {
  AutoProgressSnapshot snapshot;
  std::lock_guard<std::mutex> lock(g_state.mutex);
  const AutoState& state = g_state.auto_research;
  snapshot.active = state.active;
  snapshot.rounds = state.rounds;
  snapshot.families_total = static_cast<uint32_t>(state.results.size());
  snapshot.families_done = state.families_done;
  snapshot.pending = static_cast<uint32_t>(state.pending.size());
  snapshot.last_vs = state.last_vs;
  snapshot.captures_target = snapshot.families_total * state.rounds;
  for (const auto& result : state.results) {
    snapshot.captures_done += static_cast<uint32_t>(result.captures.size());
    switch (static_cast<AutoVerdict>(result.verdict)) {
      case AutoVerdict::Verified:
      case AutoVerdict::VerifiedSingleCapture:
        snapshot.verified += 1u;
        break;
      case AutoVerdict::Failed:
        snapshot.failed += 1u;
        break;
      case AutoVerdict::NotObserved:
        snapshot.not_observed += 1u;
        break;
      default:
        snapshot.pending_verdict += 1u;
        break;
    }
  }
  return snapshot;
}

inline void DrawResearchPanel() {
  PrefillHintLabels();

  bool do_reset = false;
  bool do_dump = false;
  bool do_arm = false;
  bool do_scan = false;
  bool do_auto_start = false;
  bool do_auto_stop = false;
  bool do_open_report = false;
  uint32_t arm_vs = 0u;

  std::vector<FamilyStats> families;
  std::vector<CandidateSummary> candidates;
  DrawRecord captured_draw;
  bool captured_valid = false;
  bool mesh_valid = false;
  size_t mesh_vertices = 0u;
  size_t mesh_triangles = 0u;
  int mesh_pos_off = -1;
  bool mesh_heuristic = false;
  std::string mesh_pos_format;
  size_t captured_cbs = 0u;
  size_t captured_srvs = 0u;
  int32_t instance_offset_g = 0;
  bool instance_offset_found = false;
  std::string mesh_status;
  std::string status;
  int selected_candidate = 0;
  uint32_t selected_family = 0u;
  bool arm_active = false;
  bool camera_valid = false;
  bool camera_has_prev = false;
  {
    std::lock_guard<std::mutex> lock(g_state.mutex);
    families.reserve(g_state.families.size());
    for (const auto& [hash, family] : g_state.families) {
      (void)hash;
      families.push_back(family);
    }
    candidates.reserve(g_state.candidates.size());
    for (const auto& candidate : g_state.candidates) {
      candidates.push_back(MakeCandidateSummary(candidate));
    }
    captured_draw = g_state.captured.draw;
    captured_valid = g_state.captured.draw.vs_hash != 0u;
    mesh_valid = g_state.captured.mesh_valid;
    mesh_vertices = g_state.captured.mesh.positions.size();
    mesh_triangles = g_state.captured.mesh.triangles.size();
    mesh_pos_off = g_state.captured.mesh.layout.pos_off;
    mesh_heuristic = g_state.captured.mesh.layout.heuristic;
    mesh_pos_format = renodx::utils::scene::FormatToString(g_state.captured.mesh.layout.pos_format);
    captured_cbs = g_state.captured.cbs.size();
    captured_srvs = g_state.captured.srv_buffers.size();
    instance_offset_g = g_state.captured.instance_offset_g;
    instance_offset_found = g_state.captured.instance_offset_found;
    mesh_status = g_state.captured.mesh_status;
    selected_candidate = g_state.selected_candidate;
    selected_family = g_state.selected_family;
    arm_active = g_state.arm_active;
    camera_valid = g_state.camera.valid;
    camera_has_prev = g_state.camera.has_prev;
    status = g_state.status;
  }

  const AutoProgressSnapshot auto_progress = SnapshotAutoProgress();
  std::sort(families.begin(), families.end(), [](const FamilyStats& a, const FamilyStats& b) {
    if (a.candidate != b.candidate) return a.candidate;
    return a.triangles > b.triangles;
  });
  const CoverageMetrics metrics = ComputeCoverage();

  ImGui::TextWrapped("Status: %s", status.c_str());
  if (arm_active && !auto_progress.active) {
    ImGui::TextColored(ImVec4(1.f, 0.8f, 0.2f, 1.f), "Armed: waiting for the next matching draw.");
  }

  ImGui::SeparatorText("Auto Research");
  if (ImGui::Button(auto_progress.active ? "Stop" : "Start")) {
    if (auto_progress.active) {
      do_auto_stop = true;
    } else {
      do_auto_start = true;
    }
  }
  ImGui::SameLine();
  if (ImGui::Button("Open Latest Report")) do_open_report = true;
  ImGui::SameLine();
  if (ImGui::Button("Dump CSV")) do_dump = true;

  if (auto_progress.active || auto_progress.families_done > 0u) {
    ImGui::Text("Captures: %u/%u   Pending families: %u",
                auto_progress.captures_done, auto_progress.captures_target,
                auto_progress.pending);
    if (auto_progress.last_vs != 0u) {
      ImGui::Text("Last captured: 0x%08X", auto_progress.last_vs);
    }
  }
  ImGui::Text("Verified: %u   Failed: %u   Not observed: %u   Pending: %u",
              auto_progress.verified, auto_progress.failed,
              auto_progress.not_observed, auto_progress.pending_verdict);
  ImGui::TextDisabled("Play normally for 60-90 s. Runtime verification is separate from decompilation verification.");

  ImGui::SeparatorText("Coverage Gate");
  if (metrics.candidate_triangles == 0u) {
    ImGui::TextDisabled("N/A - no eligible candidate families observed yet.");
  } else {
    ImGui::Text("Primary (triangles): %.1f%% %s", metrics.primary * 100.f, metrics.primary_pass ? "PASS" : "FAIL");
    ImGui::Text("Secondary (meshes):  %.1f%% %s", metrics.secondary * 100.f, metrics.secondary_pass ? "PASS" : "FAIL");
    ImGui::Text("Major family missing: %s", metrics.major_family_missing ? "YES" : "no");
    ImGui::Text("Instances: %llu verified / %llu candidates",
                static_cast<unsigned long long>(metrics.verified_instances),
                static_cast<unsigned long long>(metrics.candidate_instances));
    ImGui::TextColored(
        metrics.pass ? ImVec4(0.4f, 0.9f, 0.4f, 1.f) : ImVec4(0.95f, 0.4f, 0.4f, 1.f),
        "Overall: %s", metrics.pass ? "PASS" : "FAIL");
  }

  ImGui::SeparatorText("VS Families");
  if (ImGui::BeginTable("##WorldFamilies", 9, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0.f, 220.f))) {
    ImGui::TableSetupColumn("VS / Hint (manual)");
    ImGui::TableSetupColumn("Draws");
    ImGui::TableSetupColumn("Meshes");
    ImGui::TableSetupColumn("Instances");
    ImGui::TableSetupColumn("Tris");
    ImGui::TableSetupColumn("Depth%");
    ImGui::TableSetupColumn("Auto (ok/total)");
    ImGui::TableSetupColumn("Verdict");
    ImGui::TableSetupColumn("Label (runtime)");
    ImGui::TableHeadersRow();
    for (const auto& family : families) {
      const ReferenceHint* hint = FindReferenceHint(family.vs_hash);
      ImGui::TableNextRow();
      ImGui::PushID(static_cast<int>(family.vs_hash));

      ImGui::TableSetColumnIndex(0);
      char label[96] = {};
      std::snprintf(label, sizeof(label), "0x%08X%s%s%s",
                    family.vs_hash,
                    family.candidate ? " *" : "",
                    family.skin_draws > 0u ? " [skin]" : "",
                    hint != nullptr ? "  [" : "");
      const bool selected = (family.vs_hash == selected_family);
      ImGui::SetNextItemAllowOverlap();
      if (ImGui::Selectable(label, selected, ImGuiSelectableFlags_SpanAllColumns)) {
        selected_family = family.vs_hash;
        std::lock_guard<std::mutex> lock(g_state.mutex);
        g_state.selected_family = family.vs_hash;
      }
      if (hint != nullptr && ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Manual hint: %s / %s\n%s\n%s", hint->tag, hint->category, hint->pattern, hint->file);
      }
      if (hint != nullptr) {
        ImGui::SameLine();
        ImGui::TextDisabled("%s", hint->tag);
      }

      ImGui::TableSetColumnIndex(1);
      ImGui::Text("%llu", static_cast<unsigned long long>(family.draws));
      ImGui::TableSetColumnIndex(2);
      ImGui::Text("%llu", static_cast<unsigned long long>(family.meshes.size()));
      ImGui::TableSetColumnIndex(3);
      ImGui::Text("%llu", static_cast<unsigned long long>(family.instances));
      ImGui::TableSetColumnIndex(4);
      ImGui::Text("%llu", static_cast<unsigned long long>(family.triangles));
      ImGui::TableSetColumnIndex(5);
      const float depth_pct = family.draws != 0u
                                  ? static_cast<float>(family.depth_write_draws) * 100.f / static_cast<float>(family.draws)
                                  : 0.f;
      ImGui::Text("%.0f%%", depth_pct);
      ImGui::TableSetColumnIndex(6);
      if (family.captures_attempted != 0u || family.captures_ok != 0u) {
        ImGui::Text("%u/%u", family.captures_ok, family.captures_attempted);
      } else {
        ImGui::TextDisabled("-");
      }
      ImGui::TableSetColumnIndex(7);
      ImGui::Text("%s", AutoVerdictName(family.auto_verdict));
      ImGui::TableSetColumnIndex(8);
      int label_index = static_cast<int>(family.label);
      const char* labels = "Unknown\0Terrain\0Buildings\0Props\0Glass/Windows\0Character\0Foliage\0Transparent\0Other\0";
      if (ImGui::Combo("##label", &label_index, labels)) {
        ApplyFamilyLabel(family.vs_hash, static_cast<uint8_t>(label_index));
      }
      if (family.label_source == static_cast<uint8_t>(LabelSource::Hint)) {
        ImGui::SameLine();
        ImGui::TextDisabled("(hint)");
      }
      ImGui::PopID();
    }
    ImGui::EndTable();
  }
  ImGui::TextDisabled("* = static opaque candidate; [skin] = skinned (excluded from V1)");

  ImGui::SeparatorText("Manual Investigation");
  if (ImGui::CollapsingHeader("Capture / scan / overlay (failure fallback)")) {
    const auto selected_it = std::find_if(families.begin(), families.end(), [&](const FamilyStats& family) {
      return family.vs_hash == selected_family;
    });
    const bool selected_is_candidate = selected_it != families.end() && selected_it->candidate;

    ImGui::BeginDisabled(selected_family == 0u || !selected_is_candidate || auto_progress.active);
    if (ImGui::Button("Arm Capture (next draw of selected family)")) {
      do_arm = true;
      arm_vs = selected_family;
    }
    ImGui::EndDisabled();
    if (selected_family != 0u && !selected_is_candidate) {
      ImGui::SameLine();
      ImGui::TextDisabled("excluded from V1 (skinned/foliage/non-opaque)");
    }
    ImGui::SameLine();
    ImGui::BeginDisabled(mesh_vertices == 0u);
    if (ImGui::Button("Scan Candidates")) do_scan = true;
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (ImGui::Button("Reset Census")) do_reset = true;

    if (!captured_valid) {
      ImGui::TextDisabled("No draw captured yet.");
    } else {
      ImGui::Text("Serial: %u  VS: 0x%08X  PS: 0x%08X", captured_draw.serial, captured_draw.vs_hash, captured_draw.ps_hash);
      ImGui::Text("Indices: %u  Instances: %u  FirstInstance: %u  Stride: %u  IndexSize: %u",
                  captured_draw.index_count, captured_draw.instance_count, captured_draw.first_instance,
                  captured_draw.vb_stride, captured_draw.index_size);
      ImGui::Text("CB snapshots: %llu  SRV buffers: %llu", static_cast<unsigned long long>(captured_cbs),
                  static_cast<unsigned long long>(captured_srvs));
      if (instance_offset_found) {
        ImGui::Text("instanceOffset_g (b1): %d", instance_offset_g);
      } else {
        ImGui::TextDisabled("instanceOffset_g (b1): not found");
      }
      ImGui::Text("VS SRV mask: 0x%08X", captured_draw.vs_srv_mask);
      ImGui::Text("Camera: %s  prevViewProj: %s",
                  camera_valid ? "captured" : "missing",
                  camera_has_prev ? "available" : "absent");
      ImGui::Text("Mesh: %s (%llu verts, %llu tris)",
                  mesh_valid ? "ok" : mesh_status.c_str(),
                  static_cast<unsigned long long>(mesh_vertices),
                  static_cast<unsigned long long>(mesh_triangles));
      if (mesh_valid) {
        ImGui::Text("Position: off %d fmt %s%s", mesh_pos_off, mesh_pos_format.c_str(),
                    mesh_heuristic ? " (heuristic)" : "");
      }
    }

    if (!candidates.empty()) {
      if (ImGui::BeginTable("##WorldCandidates", 7, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0.f, 150.f))) {
        ImGui::TableSetupColumn("Kind");
        ImGui::TableSetupColumn("Stage");
        ImGui::TableSetupColumn("Slot");
        ImGui::TableSetupColumn("Offset");
        ImGui::TableSetupColumn("Stride");
        ImGui::TableSetupColumn("Base");
        ImGui::TableSetupColumn("Score / instances");
        ImGui::TableHeadersRow();
        for (int i = 0; i < static_cast<int>(candidates.size()); ++i) {
          const auto& candidate = candidates[static_cast<size_t>(i)];
          ImGui::TableNextRow();
          ImGui::PushID(i);
          ImGui::TableSetColumnIndex(0);
          ImGui::SetNextItemAllowOverlap();
          if (ImGui::Selectable(CandidateKindName(candidate.kind), i == selected_candidate, ImGuiSelectableFlags_SpanAllColumns)) {
            selected_candidate = i;
            std::lock_guard<std::mutex> lock(g_state.mutex);
            g_state.selected_candidate = i;
          }
          ImGui::TableSetColumnIndex(1);
          ImGui::Text("%s", candidate.stage == 1u ? "VS" : (candidate.stage == 2u ? "PS" : "-"));
          ImGui::TableSetColumnIndex(2);
          ImGui::Text("b%u", candidate.slot);
          ImGui::TableSetColumnIndex(3);
          ImGui::Text("%u", candidate.matrix_offset);
          ImGui::TableSetColumnIndex(4);
          ImGui::Text("%u", candidate.stride);
          ImGui::TableSetColumnIndex(5);
          ImGui::Text("%u", candidate.base_offset);
          ImGui::TableSetColumnIndex(6);
          ImGui::Text("%.3f  (%u/%u)%s", candidate.set_score, candidate.instances_ok,
                      candidate.element_count, candidate.has_prev ? "  prev" : "");
          ImGui::PopID();
        }
        ImGui::EndTable();
      }
    }
  }

  if (do_reset) ResetCensus();
  if (do_dump) DumpCensusCsv();
  if (do_arm) ArmCapture(arm_vs, 0u);
  if (do_scan) ScanCandidates();
  if (do_auto_start) StartAutoResearch();
  if (do_auto_stop) StopAutoResearch();
  if (do_open_report) OpenLatestReport();
}

inline void AddSettings(renodx::utils::settings::Settings* settings, bool supported) {
  if (settings == nullptr) return;
  g_state.supported = supported && IsDevkitPresent();
  if (!g_state.supported) return;

  using Setting = renodx::utils::settings::Setting;
  using SettingValueType = renodx::utils::settings::SettingValueType;
  const auto visible = []() { return g_state.supported; };

  settings->push_back(new Setting{
      .key = "WorldResearchEnabled",
      .binding = &g_state.setting_enabled,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 0.f,
      .label = "World Research (Phase 0)",
      .section = "Ray Tracing",
      .tooltip = "Enable the Sora 2nd draw census, structured-buffer transform capture, depth validation and Auto Research.",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "WorldAutoRounds",
      .binding = &g_state.setting_auto_rounds,
      .value_type = SettingValueType::INTEGER,
      .default_value = 3.f,
      .label = "Auto Rounds",
      .section = "Ray Tracing",
      .tooltip = "Captures per family (one per round). At least two passing captures are required for automatic verification.",
      .min = 1.f,
      .max = 10.f,
      .format = "%d",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "WorldProbeThreshold",
      .binding = &g_state.setting_probe_threshold,
      .value_type = SettingValueType::FLOAT,
      .default_value = 0.35f,
      .label = "Depth Match Threshold",
      .section = "Ray Tracing",
      .tooltip = "Minimum match / (match + mismatch) ratio for depth validation. Occluded samples are neutral.",
      .min = 0.f,
      .max = 1.f,
      .format = "%.2f",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "WorldProbeTolRel",
      .binding = &g_state.setting_probe_tol_rel,
      .value_type = SettingValueType::FLOAT,
      .default_value = 0.05f,
      .label = "Depth Tolerance (relative)",
      .section = "Ray Tracing",
      .tooltip = "Relative device-depth tolerance for a sample to count as a match.",
      .min = 0.001f,
      .max = 0.5f,
      .format = "%.3f",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "WorldProbeTolAbs",
      .binding = &g_state.setting_probe_tol_abs,
      .value_type = SettingValueType::FLOAT,
      .default_value = 0.002f,
      .label = "Depth Tolerance (absolute)",
      .section = "Ray Tracing",
      .tooltip = "Absolute device-depth floor for the match tolerance.",
      .min = 0.0001f,
      .max = 0.5f,
      .format = "%.4f",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "WorldAutoRerun",
      .binding = &g_state.setting_auto_rerun,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 0.f,
      .label = "Re-run All (include verified)",
      .section = "Ray Tracing",
      .tooltip = "When on, Auto Research also processes families that are already verified.",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "WorldOverlayMode",
      .binding = &g_state.setting_overlay_mode,
      .value_type = SettingValueType::INTEGER,
      .default_value = 0.f,
      .label = "Overlay",
      .section = "Ray Tracing",
      .tooltip = "Manual debug overlay; requires the ReShade overlay to be open.",
      .labels = {"Off", "Points", "Wireframe", "Bounds", "World Position"},
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "WorldOverlayStride",
      .binding = &g_state.setting_overlay_stride,
      .value_type = SettingValueType::INTEGER,
      .default_value = 8.f,
      .label = "Overlay Stride",
      .section = "Ray Tracing",
      .tooltip = "Vertex/triangle subsampling stride for the overlay.",
      .min = 1.f,
      .max = 64.f,
      .format = "%d",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "WorldOverlayShowPrev",
      .binding = &g_state.setting_overlay_show_prev,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 0.f,
      .label = "Show Prev World",
      .section = "Ray Tracing",
      .tooltip = "Draw the previous-frame world position when the instance layout provides prevWorld.",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "WorldResearchPanel",
      .value_type = SettingValueType::CUSTOM,
      .can_reset = false,
      .label = "Phase 0 Research",
      .section = "Ray Tracing",
      .on_draw = []() {
        DrawResearchPanel();
        return false;
      },
      .is_visible = visible,
  });
}

}  // namespace falcom_world
