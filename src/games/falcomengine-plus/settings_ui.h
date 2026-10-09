#pragma once

#include <algorithm>
#include <cfloat>
#include <cstdint>
#include <cstdio>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <Windows.h>

// Game-local ReShade overlay for the Falcom Engine+ addon.
//
// The shared renodx::utils::settings renderer is intentionally left untouched.
// This file reuses the shared Setting model, persistence helpers, and preset
// config section while providing a sidebar + section-card layout for this mod
// only. Settings are classified into categories locally so the setting table
// above needs no per-entry category field.

namespace falcom_ui {

using renodx::utils::settings::Setting;
using renodx::utils::settings::SettingValueType;

// ---------------------------------------------------------------------------
// Categories
// ---------------------------------------------------------------------------

// Sidebar order follows a typical graphics menu: global, world lighting,
// screen-space effects, character, then image quality.
static constexpr std::string_view kCategories[] = {
    "General",
    "Shadows",
    "Ambient Occlusion",
    "Indirect Lighting",
    "Reflections",
    "Fog",
    "Character",
    "Anti-Aliasing",
    "Post Processing",
    "Ray Tracing",
};

// Section -> category for rows visible in Basic mode.
static const std::unordered_map<std::string_view, std::string_view> kSectionCategory = {
    {"Shadow Maps", "Shadows"},
    {"Shadows", "Shadows"},
    {"Micro Shadows", "Shadows"},
    {"Contact Shadows", "Shadows"},
    {"GTVBAO", "Ambient Occlusion"},
    {"Foliage Grass AO", "Ambient Occlusion"},
    {"VBGI", "Indirect Lighting"},
    {"SSGI (Falcom)", "Indirect Lighting"},
    {"BRDF Improvement", "Indirect Lighting"},
    {"Dynamic Cubemaps", "Reflections"},
    {"Vanilla SSR Improvements", "Reflections"},
    {"Fog Color Correction", "Fog"},
    {"Volumetric Fog", "Fog"},
    {"Character SSGI", "Character"},
    {"Character Shadowing", "Character"},
    {"Character Outline", "Character"},
    {"Custom TAA", "Anti-Aliasing"},
    {"Depth of Field", "Post Processing"},
    {"Motion Blur", "Post Processing"},
    {"Ray Tracing", "Ray Tracing"},
    {"RTAO", "Ray Tracing"},
    {"Temporal Accumulation", "Ray Tracing"},
    {"Denoising", "Ray Tracing"},
    {"Distance Fade", "Ray Tracing"},
    {"Info", "General"},
};

static constexpr float kSidebarWidth = 200.f;
static constexpr float kSidebarItemHeight = 30.f;

// ---------------------------------------------------------------------------
// Feature performance cost
// ---------------------------------------------------------------------------

enum class FeatureCost : uint8_t {
  Faster,
  None,
  Low,
  Medium,
  High,
};

static const char* CostName(FeatureCost cost) {
  switch (cost) {
    case FeatureCost::Faster: return "Faster";
    case FeatureCost::None: return "None";
    case FeatureCost::Low: return "Low";
    case FeatureCost::Medium: return "Medium";
    case FeatureCost::High: return "High";
  }
  return "None";
}

static ImVec4 CostColor(FeatureCost cost) {
  switch (cost) {
    case FeatureCost::Faster:
    case FeatureCost::None:
      return ImVec4(0.36f, 0.85f, 0.44f, 1.f);
    case FeatureCost::Low:
      return ImVec4(0.95f, 0.85f, 0.35f, 1.f);
    case FeatureCost::Medium:
      return ImVec4(1.00f, 0.62f, 0.25f, 1.f);
    case FeatureCost::High:
      return ImVec4(0.95f, 0.35f, 0.35f, 1.f);
  }
  return ImVec4(1.f, 1.f, 1.f, 1.f);
}

// Feature-level costs are shown on the section card heading.
static const std::unordered_map<std::string_view, FeatureCost> kSectionCosts = {
    {"IS-FAST", FeatureCost::None},
    {"Contact Shadows", FeatureCost::Low},
    {"GTVBAO", FeatureCost::High},
    {"Foliage Grass AO", FeatureCost::None},
    {"VBGI", FeatureCost::Low},
    {"BRDF Improvement", FeatureCost::None},
    {"Dynamic Cubemaps", FeatureCost::Medium},
    {"Vanilla SSR Improvements", FeatureCost::None},
    {"Fog Color Correction", FeatureCost::None},
    {"Character SSGI", FeatureCost::None},
    {"Character Shadowing", FeatureCost::None},
    {"Depth of Field", FeatureCost::Faster},
    {"Motion Blur", FeatureCost::Medium},
    {"Character Outline", FeatureCost::None},
    {"Custom TAA", FeatureCost::Low},
    {"RTAO", FeatureCost::High},
};

// Individual costs are shown on the setting row itself.
static const std::unordered_map<std::string_view, FeatureCost> kSettingCosts = {
    {"KaiPenumbraMode", FeatureCost::None},
    {"VolFogHazeAAMode", FeatureCost::None},
};

// Sora-only per-setting costs (Shadow Maps features).
static const std::unordered_map<std::string_view, FeatureCost> kSoraSettingCosts = {
    {"ShadowFilterMethod", FeatureCost::Low},
    {"ShadowEdgeTint", FeatureCost::None},
};

static std::optional<FeatureCost> SectionCost(std::string_view section) {
  const auto found = kSectionCosts.find(section);
  if (found == kSectionCosts.end()) return std::nullopt;
  return found->second;
}

static std::optional<FeatureCost> SettingCost(const Setting* setting) {
  if (IsSora1st() || IsSora2nd()) {
    const auto sora = kSoraSettingCosts.find(setting->key);
    if (sora != kSoraSettingCosts.end()) return sora->second;
  }
  const auto found = kSettingCosts.find(setting->key);
  if (found == kSettingCosts.end()) return std::nullopt;
  return found->second;
}

// ---------------------------------------------------------------------------
// Theme
// ---------------------------------------------------------------------------

static const ImVec4 kAccent = ImVec4(0.298f, 0.553f, 1.000f, 1.000f);
static const ImVec4 kAccentBright = ImVec4(0.451f, 0.667f, 1.000f, 1.000f);
static const ImVec4 kAccentDim = ImVec4(0.231f, 0.427f, 0.784f, 1.000f);
static const ImVec4 kSurface = ImVec4(0.106f, 0.118f, 0.141f, 1.000f);
static const ImVec4 kCard = ImVec4(0.133f, 0.149f, 0.180f, 1.000f);
static const ImVec4 kFrame = ImVec4(0.161f, 0.180f, 0.216f, 1.000f);
static const ImVec4 kFrameHover = ImVec4(0.204f, 0.227f, 0.271f, 1.000f);
static const ImVec4 kFrameActive = ImVec4(0.239f, 0.267f, 0.318f, 1.000f);
static const ImVec4 kBorder = ImVec4(0.200f, 0.227f, 0.271f, 1.000f);
static const ImVec4 kText = ImVec4(0.847f, 0.871f, 0.914f, 1.000f);
static const ImVec4 kTextDim = ImVec4(0.435f, 0.467f, 0.522f, 1.000f);
static const ImVec4 kSelected = ImVec4(0.298f, 0.553f, 1.000f, 0.180f);
static const ImVec4 kHover = ImVec4(1.000f, 1.000f, 1.000f, 0.060f);
static const ImVec4 kPatreon = ImVec4(0.239f, 0.153f, 0.161f, 1.000f);
static const ImVec4 kPatreonHover = ImVec4(0.318f, 0.184f, 0.192f, 1.000f);
static const ImVec4 kPatreonActive = ImVec4(0.388f, 0.216f, 0.224f, 1.000f);

static constexpr int kThemeColorCount = 17;
static constexpr int kThemeVarCount = 6;

static void PushOverlayTheme() {
  ImGui::PushStyleColor(ImGuiCol_Text, kText);
  ImGui::PushStyleColor(ImGuiCol_TextDisabled, kTextDim);
  ImGui::PushStyleColor(ImGuiCol_Border, kBorder);
  ImGui::PushStyleColor(ImGuiCol_FrameBg, kFrame);
  ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, kFrameHover);
  ImGui::PushStyleColor(ImGuiCol_FrameBgActive, kFrameActive);
  ImGui::PushStyleColor(ImGuiCol_Button, kFrame);
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, kFrameHover);
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, kAccentDim);
  ImGui::PushStyleColor(ImGuiCol_SliderGrab, kAccent);
  ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, kAccentBright);
  ImGui::PushStyleColor(ImGuiCol_CheckMark, kAccent);
  ImGui::PushStyleColor(ImGuiCol_Header, kSelected);
  ImGui::PushStyleColor(ImGuiCol_HeaderHovered, kHover);
  ImGui::PushStyleColor(ImGuiCol_HeaderActive, kSelected);
  ImGui::PushStyleColor(ImGuiCol_PopupBg, kCard);
  ImGui::PushStyleColor(ImGuiCol_Separator, kBorder);
  ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.f);
  ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, 4.f);
  ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, 4.f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.f);
  ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 6.f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.f, 8.f));
}

static void PopOverlayTheme() {
  ImGui::PopStyleVar(kThemeVarCount);
  ImGui::PopStyleColor(kThemeColorCount);
}

// ---------------------------------------------------------------------------
// Devkit gate
// ---------------------------------------------------------------------------

// Advanced mode is a developer feature. It unlocks only while RenoDX DevKit is
// loaded or present next to the game executable, so end users stay on Basic.
static bool IsDevkitLoaded() {
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
      if (GetFileAttributesA((directory + candidate).c_str()) != INVALID_FILE_ATTRIBUTES) {
        return true;
      }
    }
    return false;
  }();
  return file_present;
}

// ---------------------------------------------------------------------------
// Classification
// ---------------------------------------------------------------------------

enum class RowClass : uint8_t {
  Basic,
  Advanced,
  Hidden,
};

struct ClassifiedRow {
  Setting* setting;
  RowClass row_class;
};

// Classification temporarily toggles g_settings_mode so it can tell which rows
// are visible in Basic mode, which exist only in Advanced mode, and which are
// unavailable for the running game. Every is_visible lambda in this addon only
// reads the mode flag and cached exe predicates, so the probe is deterministic.
static std::vector<ClassifiedRow>& ClassifiedRows() {
  static std::vector<ClassifiedRow> rows = []() {
    std::vector<ClassifiedRow> result;
    result.reserve(::settings.size());
    const float saved_mode = g_settings_mode;
    for (auto* setting : ::settings) {
      if (setting == nullptr) continue;
      if (setting->section == "Settings") {
        result.push_back({setting, RowClass::Hidden});
        continue;
      }
      g_settings_mode = 1.f;
      const bool advanced_visible = (setting->is_visible == nullptr) || setting->is_visible();
      g_settings_mode = 0.f;
      const bool basic_visible = (setting->is_visible == nullptr) || setting->is_visible();
      RowClass row_class = RowClass::Hidden;
      if (advanced_visible) {
        row_class = basic_visible ? RowClass::Basic : RowClass::Advanced;
      }
      result.push_back({setting, row_class});
    }
    g_settings_mode = saved_mode;
    return result;
  }();
  return rows;
}

static bool IsRowDrawable(const ClassifiedRow& row) {
  if (row.row_class == RowClass::Hidden) return false;
  if (row.row_class == RowClass::Advanced && !IsAdvancedSettingsMode()) return false;
  return true;
}

static std::string_view CategoryFor(const ClassifiedRow& row) {
  const auto found = kSectionCategory.find(row.setting->section);
  if (found != kSectionCategory.end()) return found->second;
  return "General";
}

static bool CategoryHasRows(std::string_view category) {
  for (const auto& row : ClassifiedRows()) {
    if (!IsRowDrawable(row)) continue;
    if (CategoryFor(row) != category) continue;
    if (row.setting->is_visible == nullptr || row.setting->is_visible()) return true;
  }
  return false;
}

static std::vector<std::string_view> SectionsForCategory(std::string_view category) {
  std::vector<std::string_view> sections;
  for (const auto& row : ClassifiedRows()) {
    if (!IsRowDrawable(row)) continue;
    if (CategoryFor(row) != category) continue;
    const std::string& section = row.setting->section;
    if (std::find(sections.begin(), sections.end(), section) == sections.end()) {
      sections.emplace_back(section);
    }
  }
  return sections;
}

static std::string& ActiveCategory() {
  static std::string category = "General";
  return category;
}

// ---------------------------------------------------------------------------
// Small helpers
// ---------------------------------------------------------------------------

static const char* GameLabel() {
  if (IsKai()) return "Trails Beyond the Horizon";
  if (IsDaybreak2()) return "Trails Through Daybreak II";
  if (IsSora1st()) return "Trails in the Sky 1st Chapter";
  if (IsSora2nd()) return "Trails in the Sky 2nd Chapter";
  return "Kyoto Xanadu";
}

static Setting* FindSettingByLabel(const char* label) {
  for (auto* setting : ::settings) {
    if (setting != nullptr && setting->label == label) return setting;
  }
  return nullptr;
}

static Setting* SettingsModeSetting() {
  static Setting* setting = FindSettingByLabel("Settings Mode");
  return setting;
}

static Setting* ResetAllSetting() {
  static Setting* setting = FindSettingByLabel("Reset All Settings to Defaults");
  return setting;
}

static bool IsPlainRow(const Setting* setting) {
  switch (setting->value_type) {
    case SettingValueType::BUTTON:
    case SettingValueType::LABEL:
    case SettingValueType::BULLET:
    case SettingValueType::TEXT:
    case SettingValueType::TEXT_NOWRAP:
    case SettingValueType::CUSTOM:
      return true;
    default:
      return false;
  }
}

static void ApplySettingChange(Setting* setting, float previous_value) {
  setting->on_change();
  {
    const std::unique_lock lock(renodx::utils::mutex::global_mutex);
    setting->Write();
  }
  setting->on_change_value(previous_value, setting->GetValue());
}

// ---------------------------------------------------------------------------
// Controls
// ---------------------------------------------------------------------------

static bool DrawToggleButton(const char* text, bool value) {
  const ImVec4 background = value ? kAccent : kFrame;
  const ImVec4 hovered = value ? kAccentBright : kFrameHover;
  const ImVec4 active = value ? kAccentDim : kFrameActive;
  ImGui::PushStyleColor(ImGuiCol_Button, background);
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hovered);
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, active);
  if (!value) ImGui::PushStyleColor(ImGuiCol_Text, kText);
  const bool pressed = ImGui::Button(text, ImVec2(ImGui::GetFrameHeight() * 2.4f, 0));
  if (!value) ImGui::PopStyleColor();
  ImGui::PopStyleColor(3);
  return pressed;
}

static bool DrawResetButton(Setting* setting, const char* identifier) {
  bool changed = false;
  const bool is_using_default = (setting->GetValue() == setting->default_value);
  ImGui::BeginDisabled(is_using_default);
  int pushed_colors = 0;
  if (is_using_default) {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(ImColor::HSV(0, 0, 0.6f)));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(ImColor::HSV(0, 0, 0.7f)));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(ImColor::HSV(0, 0, 0.8f)));
    pushed_colors = 3;
  }

  const float previous_font_size = ImGui::GetFontSize();
  ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * 0.75f);
  const float current_font_size = ImGui::GetFontSize();
  ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, current_font_size * 2.f);

  ImVec2 cursor_pos = ImGui::GetCursorPos();
  cursor_pos.y += (previous_font_size - current_font_size) * 0.5f;
  ImGui::SetCursorPos(cursor_pos);

  const float button_size = ImGui::GetFrameHeight();
  ImGui::PushID(("##Reset" + std::string(identifier)).c_str());
  if (ImGui::Button(renodx::utils::icons::View(renodx::utils::icons::UNDO), ImVec2(button_size, button_size))) {
    setting->Set(setting->default_value);
    changed = true;
  }
  ImGui::PopID();

  ImGui::PopStyleVar();
  ImGui::PopFont();
  if (pushed_colors > 0) ImGui::PopStyleColor(pushed_colors);
  ImGui::EndDisabled();
  return changed;
}

static bool DrawPlainRow(Setting* setting) {
  bool changed = false;
  switch (setting->value_type) {
    case SettingValueType::BUTTON: {
      const bool is_patreon = (setting->label == "Patreon");
      if (is_patreon) {
        ImGui::PushStyleColor(ImGuiCol_Button, kPatreon);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, kPatreonHover);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, kPatreonActive);
      }
      if (ImGui::Button(setting->label.c_str())) {
        changed = setting->on_click();
      }
      if (is_patreon) ImGui::PopStyleColor(3);
      break;
    }
    case SettingValueType::LABEL:
      ImGui::LabelText(setting->label.c_str(), "%s", setting->labels.empty() ? "" : setting->labels[0].c_str());
      break;
    case SettingValueType::BULLET:
      ImGui::BulletText("%s", setting->label.c_str());
      break;
    case SettingValueType::TEXT:
      ImGui::TextWrapped("%s", setting->label.c_str());
      break;
    case SettingValueType::TEXT_NOWRAP:
      ImGui::TextUnformatted(setting->label.c_str());
      break;
    case SettingValueType::CUSTOM:
      changed = setting->on_draw();
      break;
    default:
      break;
  }
  if (!setting->tooltip.empty() && ImGui::IsItemHovered()) {
    ImGui::SetTooltip("%s", setting->tooltip.c_str());
  }
  return changed;
}

// Small-valued sliders also show the current value as a text readout beside the slider (key -> format).
static const std::unordered_map<std::string_view, const char*> kSliderReadouts = {
    {"RtaoNormalBias", "%.4f m"},
    {"RtaoDepthRejection", "%.3f"},
    {"RtaoRadius", "%.2f m"},
    {"RtaoRayMax", "%.2f m"},
    {"RtaoStrength", "%.2f"},
    {"RtaoHistoryWeight", "%.2f"},
    {"RtaoNormalRejection", "%.2f"},
    {"RtaoHistoryClamp", "%.2f"},
    {"RtaoFadeStart", "%.0f m"},
    {"RtaoFadeEnd", "%.0f m"},
};

static bool DrawSettingRow(Setting* setting) {
  bool changed = false;
  const std::string identifier = setting->key.empty() ? setting->label : setting->key;
  const bool is_disabled = (setting->is_enabled != nullptr) && !setting->is_enabled();
  const bool can_reset = setting->can_reset && setting->value_type < SettingValueType::BUTTON;

  ImGui::TableNextRow();
  ImGui::TableSetColumnIndex(0);
  ImGui::AlignTextToFramePadding();
  ImGui::TextUnformatted(setting->label.c_str());
  if (const auto cost = SettingCost(setting); cost.has_value()) {
    ImGui::SameLine();
    ImGui::TextColored(CostColor(*cost), "Cost: %s", CostName(*cost));
  }

  ImGui::TableSetColumnIndex(1);
  if (is_disabled) ImGui::BeginDisabled();
  const float previous_value = setting->GetValue();
  ImGui::PushID(("##Key" + identifier).c_str());
  ImGui::SetNextItemWidth(-FLT_MIN);

  ImGuiSliderFlags slider_flags = ImGuiSliderFlags_None;
  if (setting->is_logarithmic) slider_flags |= ImGuiSliderFlags_Logarithmic;

  // Readout rows: the tooltip is shown when the slider or the readout is hovered (SetItemTooltip
  // would attach to the readout, the last item drawn).
  bool has_readout = false;
  bool row_hovered = false;
  switch (setting->value_type) {
    case SettingValueType::FLOAT: {
      const auto readout = kSliderReadouts.find(setting->key);
      char text[64] = {};
      has_readout = readout != kSliderReadouts.end();
      if (readout != kSliderReadouts.end()) {
        // Reserve room for the widest readout (the maximum), so the text is never cut off.
        std::snprintf(text, sizeof(text), readout->second, setting->max);
        const float reserve = ImGui::CalcTextSize(text).x + ImGui::GetStyle().ItemSpacing.x;
        ImGui::SetNextItemWidth(-reserve);
      }
      changed |= ImGui::SliderFloat(
          "##Value",
          &setting->value,
          setting->min,
          setting->max,
          setting->format.c_str(),
          slider_flags);
      if (readout != kSliderReadouts.end()) {
        row_hovered = ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip);
        std::snprintf(text, sizeof(text), readout->second, setting->value);
        ImGui::SameLine();
        ImGui::TextColored(kAccent, "%s", text);
        row_hovered = row_hovered || ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip);
      }
      break;
    }
    case SettingValueType::INTEGER:
      if (!setting->labels.empty()) {
        std::string items;
        for (const auto& label : setting->labels) {
          items += label;
          items.push_back('\0');
        }
        items.push_back('\0');
        changed |= ImGui::Combo("##Value", &setting->value_as_int, items.c_str());
      } else {
        changed |= ImGui::SliderInt(
            "##Value",
            &setting->value_as_int,
            static_cast<int>(setting->min),
            static_cast<int>(setting->GetMax()),
            setting->format.c_str(),
            slider_flags | ImGuiSliderFlags_NoInput);
      }
      break;
    case SettingValueType::BOOLEAN: {
      const int last_index = static_cast<int>(setting->labels.size()) - 1;
      const int index = setting->value_as_int < 0
                            ? 0
                            : (setting->value_as_int > last_index ? last_index : setting->value_as_int);
      const char* text = setting->labels.empty()
                             ? (setting->value_as_int == 0 ? "OFF" : "ON")
                             : setting->labels.at(index).c_str();
      if (DrawToggleButton(text, setting->value_as_int != 0)) {
        setting->value_as_int = (setting->value_as_int == 0) ? 1 : 0;
        changed = true;
      }
      break;
    }
    default:
      break;
  }

  ImGui::PopID();
  if (!setting->tooltip.empty()) {
    if (has_readout) {
      if (row_hovered) ImGui::SetTooltip("%s", setting->tooltip.c_str());
    } else {
      ImGui::SetItemTooltip("%s", setting->tooltip.c_str());
    }
  }

  ImGui::TableSetColumnIndex(2);
  if (can_reset) {
    if (DrawResetButton(setting, identifier.c_str())) changed = true;
  }
  if (is_disabled) ImGui::EndDisabled();

  if (changed) {
    ApplySettingChange(setting, previous_value);
  }
  return changed;
}

// ---------------------------------------------------------------------------
// Layout
// ---------------------------------------------------------------------------

static bool DrawHeader() {
  bool changed = false;
  const bool devkit = IsDevkitLoaded();

  ImGui::AlignTextToFramePadding();
  ImGui::TextColored(kAccent, "%s", renodx::utils::settings::overlay_title.c_str());
  ImGui::SameLine();
  ImGui::TextDisabled("%s", GameLabel());

  auto* reset_all = ResetAllSetting();
  const float reset_width = ImGui::CalcTextSize("Reset All").x + (ImGui::GetStyle().FramePadding.x * 2.0f);
  const float combo_width = 132.0f;
  const float right_width = reset_width
                            + (devkit ? combo_width + ImGui::GetStyle().ItemSpacing.x : 0.0f);
  const float right_x = ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - right_width;
  if (right_x > ImGui::GetCursorPosX()) {
    ImGui::SameLine(right_x);
  } else {
    ImGui::SameLine();
  }

  if (devkit) {
    if (auto* mode = SettingsModeSetting(); mode != nullptr) {
      int current = mode->value_as_int;
      ImGui::SetNextItemWidth(combo_width);
      if (ImGui::Combo("##FalcomSettingsMode", &current, "Basic\0Advanced\0")) {
        mode->value_as_int = current;
        {
          const std::unique_lock lock(renodx::utils::mutex::global_mutex);
          mode->Write();
        }
        mode->on_change();
        changed = true;
      }
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Basic hides advanced and developer settings.");
      }
    } else {
      ImGui::Dummy(ImVec2(combo_width, 0));
    }
    ImGui::SameLine();
  }

  ImGui::BeginDisabled(reset_all == nullptr);
  if (ImGui::Button("Reset All")) {
    if (reset_all != nullptr) {
      reset_all->on_click();
      changed = true;
    }
  }
  ImGui::EndDisabled();
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("Reset every setting to its default value.");
  }
  return changed;
}

static void DrawSidebar(float height) {
  ImGui::PushStyleColor(ImGuiCol_ChildBg, kSurface);
  ImGui::BeginChild("##FalcomSidebar", ImVec2(kSidebarWidth, height), ImGuiChildFlags_Borders);
  for (const auto& category : kCategories) {
    if (!CategoryHasRows(category)) continue;

    const bool selected = (ActiveCategory() == category);
    if (selected) {
      ImGui::PushStyleColor(ImGuiCol_Header, kSelected);
      ImGui::PushStyleColor(ImGuiCol_HeaderHovered, kSelected);
      ImGui::PushStyleColor(ImGuiCol_HeaderActive, kSelected);
      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 1.f, 1.f, 1.f));
    } else {
      ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.f, 0.f, 0.f, 0.f));
      ImGui::PushStyleColor(ImGuiCol_HeaderHovered, kHover);
      ImGui::PushStyleColor(ImGuiCol_HeaderActive, kHover);
    }

    if (ImGui::Selectable(
            std::string(category).c_str(),
            selected,
            0,
            ImVec2(0.f, kSidebarItemHeight))) {
      ActiveCategory().assign(category);
    }

    if (selected) {
      const ImVec2 item_min = ImGui::GetItemRectMin();
      const ImVec2 item_max = ImGui::GetItemRectMax();
      ImGui::GetWindowDrawList()->AddRectFilled(
          ImVec2(item_min.x + 1.f, item_min.y + 4.f),
          ImVec2(item_min.x + 4.f, item_max.y - 4.f),
          ImGui::GetColorU32(kAccent),
          1.5f);
    }

    ImGui::PopStyleColor(selected ? 4 : 3);
  }

  const ImGuiStyle& style = ImGui::GetStyle();
  const float buttons_height = (ImGui::GetFrameHeight() * 2.f) + style.ItemSpacing.y;
  const float remaining = ImGui::GetContentRegionAvail().y;
  if (remaining > buttons_height + style.ItemSpacing.y) {
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + remaining - buttons_height);
  } else {
    ImGui::Spacing();
  }

  ImGui::PushStyleColor(ImGuiCol_Button, kPatreon);
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, kPatreonHover);
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, kPatreonActive);
  if (ImGui::Button("Patreon", ImVec2(-FLT_MIN, 0.f))) {
    renodx::utils::platform::LaunchURL("https://www.patreon.com/c/Toru77");
  }
  ImGui::PopStyleColor(3);
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("Support development on Patreon.");
  }

  if (ImGui::Button("Discord", ImVec2(-FLT_MIN, 0.f))) {
    renodx::utils::platform::LaunchURL("https://discord.com/invite/renodx");
  }
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("Join the RenoDX Discord.");
  }

  ImGui::EndChild();
  ImGui::PopStyleColor();
}

static bool DrawCategoryHeader(std::string_view category) {
  bool changed = false;
  ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * 1.2f);
  ImGui::TextColored(kAccent, "%.*s", static_cast<int>(category.size()), category.data());
  ImGui::PopFont();

  const float reset_width = ImGui::CalcTextSize("Reset Category").x + (ImGui::GetStyle().FramePadding.x * 2.0f);
  const float right_x = ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - reset_width;
  if (right_x > ImGui::GetCursorPosX()) {
    ImGui::SameLine(right_x);
  } else {
    ImGui::SameLine();
  }
  if (ImGui::Button("Reset Category")) {
    changed = true;
  }
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("Reset every setting in this category.");
  }
  ImGui::Spacing();
  return changed;
}

static bool ResetCategory(std::string_view category) {
  bool changed = false;
  const std::unique_lock lock(renodx::utils::mutex::global_mutex);
  for (const auto& row : ClassifiedRows()) {
    if (!IsRowDrawable(row)) continue;
    if (CategoryFor(row) != category) continue;
    auto* setting = row.setting;
    if (!setting->can_reset || setting->is_global) continue;
    if (setting->value_type >= SettingValueType::BUTTON) continue;
    setting->Set(setting->default_value)->Write();
    changed = true;
  }
  return changed;
}

static bool DrawSectionCard(std::string_view category, std::string_view section) {
  bool changed = false;

  ImGui::PushID(section.data());
  ImGui::PushStyleColor(ImGuiCol_ChildBg, kCard);
  ImGui::PushStyleColor(ImGuiCol_Border, kBorder);
  ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 6.f);
  ImGui::BeginChild(
      "##Card",
      ImVec2(0.f, 0.f),
      ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders,
      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

  ImGui::TextColored(kAccent, "%.*s", static_cast<int>(section.size()), section.data());
  if (const auto cost = SectionCost(section); cost.has_value()) {
    ImGui::SameLine();
    ImGui::TextColored(CostColor(*cost), "Cost: %s", CostName(*cost));
  }
  ImGui::Spacing();

  const float reset_width = ImGui::GetFrameHeight() + 4.f;
  const float available = ImGui::GetContentRegionAvail().x;
  float label_width = std::clamp(available * 0.46f, 170.f, 360.f);
  if (available - label_width - reset_width < 150.f) {
    label_width = std::max(120.f, available - reset_width - 150.f);
  }

  bool table_open = false;
  int table_index = 0;
  for (const auto& row : ClassifiedRows()) {
    if (!IsRowDrawable(row)) continue;
    if (CategoryFor(row) != category) continue;
    if (row.setting->section != section) continue;

    auto* setting = row.setting;
    if (IsPlainRow(setting)) {
      if (table_open) {
        ImGui::EndTable();
        table_open = false;
      }
      const float previous_value = setting->GetValue();
      if (DrawPlainRow(setting)) {
        ApplySettingChange(setting, previous_value);
        changed = true;
      }
    } else {
      if (!table_open) {
        const std::string table_id = "##Rows" + std::to_string(table_index++);
        table_open = ImGui::BeginTable(table_id.c_str(), 3, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoPadOuterX);
        if (table_open) {
          ImGui::TableSetupColumn("##label", ImGuiTableColumnFlags_WidthFixed, label_width);
          ImGui::TableSetupColumn("##control", ImGuiTableColumnFlags_WidthStretch);
          ImGui::TableSetupColumn("##reset", ImGuiTableColumnFlags_WidthFixed, reset_width);
        }
      }
      if (table_open && DrawSettingRow(setting)) changed = true;
    }
  }
  if (table_open) ImGui::EndTable();

  ImGui::EndChild();
  ImGui::PopStyleVar();
  ImGui::PopStyleColor(2);
  ImGui::PopID();
  ImGui::Dummy(ImVec2(0.f, 2.f));
  return changed;
}

static bool DrawCategoryContent(std::string_view category, float height) {
  bool changed = false;
  const std::string child_id = "##FalcomContent" + std::string(category);
  ImGui::BeginChild(child_id.c_str(), ImVec2(0.f, height), ImGuiChildFlags_AlwaysUseWindowPadding);
  if (DrawCategoryHeader(category)) {
    changed = true;
    ResetCategory(category);
  }
  for (const auto& section : SectionsForCategory(category)) {
    changed |= DrawSectionCard(category, section);
  }
  ImGui::Dummy(ImVec2(0.f, 4.f));
  ImGui::EndChild();
  return changed;
}

// ---------------------------------------------------------------------------
// Overlay
// ---------------------------------------------------------------------------

static void OnRegisterOverlay(reshade::api::effect_runtime* runtime) {
  (void)runtime;

  if (!CategoryHasRows(ActiveCategory())) {
    for (const auto& category : kCategories) {
      if (CategoryHasRows(category)) {
        ActiveCategory().assign(category);
        break;
      }
    }
  }

  PushOverlayTheme();

  bool any_change = DrawHeader();
  ImGui::Separator();

  const float content_height = std::max(160.f, ImGui::GetContentRegionAvail().y);
  DrawSidebar(content_height);
  ImGui::SameLine();
  any_change |= DrawCategoryContent(ActiveCategory(), content_height);

  PopOverlayTheme();

  if (any_change) {
    renodx::utils::settings::SaveSettings(renodx::utils::settings::global_name + "-preset1");
    renodx::utils::settings::SaveGlobalSettings();
  }
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

static bool attached = false;

static void Use(DWORD fdw_reason, renodx::utils::settings::Settings* settings_list) {
  switch (fdw_reason) {
    case DLL_PROCESS_ATTACH:
      if (attached) return;
      attached = true;

      renodx::utils::settings::settings = settings_list;
      renodx::utils::settings::LoadGlobalSettings();
      if (!IsDevkitLoaded()) {
        if (auto* mode = SettingsModeSetting(); mode != nullptr) {
          mode->value_as_int = 0;
          mode->Write();
        }
      }
      renodx::utils::settings::LoadSettings(renodx::utils::settings::global_name + "-preset1");
      reshade::register_overlay(renodx::utils::settings::overlay_title.c_str(), OnRegisterOverlay);

      break;
    case DLL_PROCESS_DETACH:
      if (!attached) return;
      attached = false;
      reshade::unregister_overlay(renodx::utils::settings::overlay_title.c_str(), OnRegisterOverlay);
      break;
  }
}

}  // namespace falcom_ui
