#pragma once

// Ray Tracing tab content.
//
// The tab only exists when DevKit is present (world::AddSettings adds rows only
// when g_state.supported is true). The BVH panel (Pool Scan, GPU debug views)
// is the main workflow; the census panel lists draws per VS family.

#include <algorithm>
#include <cstdio>

#include <Windows.h>

#include "world_state.hpp"
#include "bvh/world_bvh.hpp"
#include "capture/draw_census.hpp"
#include "research/reference_hints.hpp"

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

inline void DrawResearchPanel() {
  PrefillHintLabels();

  bool do_reset = false;
  std::vector<FamilyStats> families;
  std::string status;
  {
    std::lock_guard<std::mutex> lock(g_state.mutex);
    families.reserve(g_state.families.size());
    for (const auto& [hash, family] : g_state.families) {
      (void)hash;
      families.push_back(family);
    }
    status = g_state.status;
  }

  std::sort(families.begin(), families.end(), [](const FamilyStats& a, const FamilyStats& b) {
    if (a.candidate != b.candidate) return a.candidate;
    return a.triangles > b.triangles;
  });

  ImGui::TextWrapped("Status: %s", status.c_str());

  ImGui::SeparatorText("VS Families");
  if (ImGui::BeginTable("##WorldFamilies", 7, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0.f, 220.f))) {
    ImGui::TableSetupColumn("VS / Hint (manual)");
    ImGui::TableSetupColumn("Draws");
    ImGui::TableSetupColumn("Meshes");
    ImGui::TableSetupColumn("Instances");
    ImGui::TableSetupColumn("Tris");
    ImGui::TableSetupColumn("Depth%");
    ImGui::TableSetupColumn("Label (runtime)");
    ImGui::TableHeadersRow();
    for (const auto& family : families) {
      const ReferenceHint* hint = FindReferenceHint(family.vs_hash);
      ImGui::TableNextRow();
      ImGui::PushID(static_cast<int>(family.vs_hash));

      ImGui::TableSetColumnIndex(0);
      ImGui::Text("0x%08X%s%s", family.vs_hash, family.candidate ? " *" : "", family.skin_draws > 0u ? " [skin]" : "");
      if (hint != nullptr && ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Manual hint: %s / %s\n%s\n%s", hint->tag, hint->category, hint->pattern, hint->file);
      }
      if (hint != nullptr) {
        ImGui::SameLine();
        ImGui::TextDisabled("[%s]", hint->tag);
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
  ImGui::TextDisabled("* = static opaque candidate; [skin] = skinned");
  if (ImGui::Button("Reset Census")) do_reset = true;

  if (do_reset) ResetCensus();
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
      .label = "Draw Census",
      .section = "Ray Tracing",
      .tooltip = "Enable the Sora 2nd draw census (draws per VS family).",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "WorldResearchPanel",
      .value_type = SettingValueType::CUSTOM,
      .can_reset = false,
      .label = "Census Families",
      .section = "Ray Tracing",
      .on_draw = []() {
        DrawResearchPanel();
        return false;
      },
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "WorldBvhPanel",
      .value_type = SettingValueType::CUSTOM,
      .can_reset = false,
      .label = "Phase 1 World BVH",
      .section = "Ray Tracing",
      .on_draw = []() {
        bvh::DrawBvhPanel();
        return false;
      },
      .is_visible = visible,
  });
}

}  // namespace falcom_world
