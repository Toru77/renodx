/*
 * Copyright (C) 2026
 * SPDX-License-Identifier: MIT
 *
 * Forza Horizon 6 Ray Reconstruction — Streamline interception.
 *
 * M1 diagnostics build: every Streamline entry point the game uses is
 * detoured, observed and forwarded untouched. All diagnostics are drawn in
 * the ReShade overlay ("Ray Reconstruction" section) so no log reading is
 * needed; the log only receives hook installation and hard failures.
 */

#define ImTextureID ImU64

#define DEBUG_LEVEL_0

#include <cstdio>
#include <sstream>

#include <deps/imgui/imgui.h>
#include <include/reshade.hpp>

#include "../../utils/date.hpp"
#include "../../utils/settings.hpp"

#include "./sl_rr.hpp"

namespace {

const ImVec4 kColorOk = {0.40f, 0.90f, 0.40f, 1.0f};
const ImVec4 kColorWarn = {1.00f, 0.80f, 0.30f, 1.0f};
const ImVec4 kColorBad = {1.00f, 0.40f, 0.40f, 1.0f};

std::string Hex64(uint64_t value) {
  char buffer[32] = {};
  snprintf(buffer, sizeof buffer, "0x%llX", static_cast<unsigned long long>(value));
  return buffer;
}

std::string LuidText(const std::array<uint8_t, 8>& luid) {
  char buffer[64] = {};
  snprintf(
      buffer, sizeof buffer, "%02X%02X%02X%02X%02X%02X%02X%02X",
      luid[3], luid[2], luid[1], luid[0], luid[7], luid[6], luid[5], luid[4]);
  return buffer;
}

// Generic read-only table used by every diagnostics panel.
bool DrawRowTable(
    const char* id, const std::vector<const char*>& headers,
    const std::vector<std::vector<std::string>>& rows) {
  if (!ImGui::BeginTable(
          id, static_cast<int>(headers.size()),
          ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit
              | ImGuiTableFlags_ScrollX)) {
    return false;
  }
  for (const char* header : headers) {
    ImGui::TableSetupColumn(header);
  }
  ImGui::TableHeadersRow();
  for (const auto& row : rows) {
    ImGui::TableNextRow();
    for (const auto& cell : row) {
      ImGui::TableNextColumn();
      ImGui::TextUnformatted(cell.c_str());
    }
  }
  ImGui::EndTable();
  return false;
}

// ---------------------------------------------------------------------------

bool DrawStatusPanel() {
  const sl_rr::Diagnostics d = sl_rr::CaptureDiagnostics();

  if (!d.module_found) {
    ImGui::TextColored(kColorWarn, "Streamline: waiting for sl.interposer.dll");
    ImGui::TextUnformatted("Clears as soon as the game loads Streamline into this process.");
  } else {
    ImGui::TextColored(
        d.armed ? kColorOk : kColorWarn, "Streamline: %s (%d/%d hooks)",
        d.armed ? "hooked" : "found, arming", d.hooks_installed, sl_rr::kHookCount);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", d.module_path.c_str());
  }

  if (!d.init_seen) {
    ImGui::TextColored(kColorWarn, "slInit: not seen yet");
  } else {
    ImGui::TextColored(
        static_cast<sl::Result>(d.init_result) == sl::Result::eOk ? kColorOk : kColorBad,
        "slInit: %s, SDK %s", sl_rr::ResultName(d.init_result).c_str(),
        sl_rr::SdkVersionLabel(d.init_sdk_version).c_str());
    ImGui::Text(
        "flags 0x%llX, log %d, engine %d, appId %u, API %d",
        static_cast<unsigned long long>(d.init_flags), d.init_log_level, d.init_engine,
        d.init_app_id, d.init_render_api);
    if (d.init_feature_count > 0) {
      std::stringstream features;
      features << "featuresToLoad:";
      for (uint32_t i = 0; i < d.init_feature_count
                           && i < static_cast<uint32_t>(d.init_features.size());
           ++i) {
        features << ' ' << sl_rr::FeatureName(d.init_features[i]);
      }
      ImGui::TextUnformatted(features.str().c_str());
    }
  }
  if (d.shutdown_seen) {
    ImGui::TextColored(kColorBad, "slShutdown was called");
  }

  if (d.device_seen) {
    ImGui::Text(
        "Device: set, LUID %s%s",
        d.luid_valid ? LuidText(d.luid).c_str() : "-",
        d.luid_valid ? "" : " (not captured)");
  } else {
    ImGui::TextColored(kColorWarn, "Device: not set yet");
  }
  ImGui::Text(
      "Frames: %llu tokens, last #%u", static_cast<unsigned long long>(d.frame_tokens),
      d.last_frame_index);

  ImGui::SeparatorText("Buffers and calls");
  ImGui::Text(
      "Tags: %llu slSetTag, %llu slSetTagForFrame, %d buffer type(s)",
      static_cast<unsigned long long>(d.tag_calls),
      static_cast<unsigned long long>(d.tag_for_frame_calls), static_cast<int>(d.tags.size()));
  ImGui::Text(
      "Constants: %llu call(s), last frame %u",
      static_cast<unsigned long long>(d.constants_calls), d.constants_last_frame);
  ImGui::Text("Evaluates: %llu total", static_cast<unsigned long long>(d.evaluates_total));

  ImGui::SeparatorText("DLSS Ray Reconstruction");
  if (d.probe_pending) {
    ImGui::TextColored(kColorWarn, "Probe pending — runs at the next Streamline call.");
  }
  const auto rr_it = d.features.find(sl::kFeatureDLSS_RR);
  if (rr_it == d.features.end() || (!rr_it->second.probed && rr_it->second.queries == 0)) {
    ImGui::TextColored(
        kColorWarn, "Not probed yet. Click \"Probe DLSS-RR\" (or wait for the automatic probe).");
  } else {
    const auto& rr = rr_it->second;
    const bool supported = static_cast<sl::Result>(rr.supported_result) == sl::Result::eOk;
    ImGui::TextColored(
        supported ? kColorOk : kColorBad, "supported: %s",
        sl_rr::ResultName(rr.supported_result).c_str());
    const bool loaded_ok = rr.loaded_result == sl_rr::kNever
                           || static_cast<sl::Result>(rr.loaded_result) == sl::Result::eOk;
    ImGui::TextColored(
        loaded_ok ? kColorOk : kColorBad, "loaded: %s (%s)",
        rr.loaded_result == sl_rr::kNever ? "-" : (rr.loaded ? "yes" : "no"),
        sl_rr::ResultName(rr.loaded_result).c_str());
    if (rr.version_result != sl_rr::kNever) {
      std::stringstream version;
      version << "SL " << rr.sl_major << '.' << rr.sl_minor << '.' << rr.sl_build << ", NGX "
              << rr.ngx_major << '.' << rr.ngx_minor << '.' << rr.ngx_build;
      ImGui::Text("version: %s (%s)", version.str().c_str(), sl_rr::ResultName(rr.version_result).c_str());
    }
  }
  return false;
}

bool DrawTagsPanel() {
  const sl_rr::Diagnostics d = sl_rr::CaptureDiagnostics();
  if (d.tags.empty()) {
    ImGui::TextColored(kColorWarn, "No buffer tags observed yet.");
    ImGui::TextUnformatted("Tags appear once the game renders with DLSS active.");
    return false;
  }
  std::vector<std::vector<std::string>> rows;
  rows.reserve(d.tags.size());
  for (const auto& tag : d.tags) {
    std::string format = "-";
    if (tag.desc_ok) {
      format = sl_rr::FormatLabel(tag.desc_format);
    } else if (tag.sl_format != 0) {
      format = sl_rr::FormatLabel(tag.sl_format);
    }
    std::string size = "-";
    if (tag.desc_ok) {
      char buffer[64] = {};
      snprintf(
          buffer, sizeof buffer, "%llux%u", static_cast<unsigned long long>(tag.desc_width),
          tag.desc_height);
      size = buffer;
      if (tag.desc_layers > 1) size += " x" + std::to_string(tag.desc_layers);
    } else if (tag.sl_width != 0 || tag.sl_height != 0) {
      char buffer[64] = {};
      snprintf(buffer, sizeof buffer, "%ux%u", tag.sl_width, tag.sl_height);
      size = buffer;
    }
    std::string extent = "-";
    if (tag.extent_w != 0 || tag.extent_h != 0) {
      char buffer[64] = {};
      snprintf(buffer, sizeof buffer, "%ux%u", tag.extent_w, tag.extent_h);
      extent = buffer;
    }
    std::string state = "-";
    if (tag.sl_state != 0xFFFFFFFFu) {
      char buffer[32] = {};
      snprintf(buffer, sizeof buffer, "0x%X", tag.sl_state);
      state = buffer;
    }
    rows.push_back({
        sl_rr::BufferName(tag.type) + " (" + std::to_string(tag.type) + ")",
        tag.via_frame_api ? "SetTagForFrame" : "SetTag",
        std::to_string(tag.calls),
        tag.cleared ? "(cleared)" : Hex64(reinterpret_cast<uint64_t>(tag.native)),
        format,
        size,
        state,
        sl_rr::LifecycleName(tag.lifecycle),
        extent,
        std::to_string(tag.last_frame),
    });
  }
  ImGui::TextColored(kColorOk, "%d buffer type(s) tagged by the game", static_cast<int>(rows.size()));
  return DrawRowTable(
      "##sl_rr_tags",
      {"Buffer", "Via", "Calls", "Resource", "Format", "Size", "State", "Lifecycle", "Extent", "Frame"},
      rows);
}

bool DrawConstantsPanel() {
  const sl_rr::Diagnostics d = sl_rr::CaptureDiagnostics();
  if (!d.constants_seen) {
    ImGui::TextColored(kColorWarn, "slSetConstants not seen yet.");
    return false;
  }
  const auto& c = d.constants;
  ImGui::Text(
      "Calls %llu, last frame %u, viewport %u",
      static_cast<unsigned long long>(d.constants_calls), d.constants_last_frame,
      d.constants_last_viewport);
  ImGui::Text(
      "Jitter (%.5f, %.5f)   MvecScale (%.5f, %.5f)", c.jitterOffset.x, c.jitterOffset.y,
      c.mvecScale.x, c.mvecScale.y);
  ImGui::Text(
      "Near %.4f   Far %.4f   FOV %.5f   Aspect %.5f", c.cameraNear, c.cameraFar, c.cameraFOV,
      c.cameraAspectRatio);
  ImGui::Text("Camera (%.3f, %.3f, %.3f)", c.cameraPos.x, c.cameraPos.y, c.cameraPos.z);
  ImGui::Text(
      "depthInverted %s | cameraMotionIncluded %s | 3D MVs %s | reset %s | jittered %s | dilated %s",
      sl_rr::BooleanName(c.depthInverted), sl_rr::BooleanName(c.cameraMotionIncluded),
      sl_rr::BooleanName(c.motionVectors3D), sl_rr::BooleanName(c.reset),
      sl_rr::BooleanName(c.motionVectorsJittered), sl_rr::BooleanName(c.motionVectorsDilated));
  if (ImGui::TreeNode("Matrices")) {
    auto print_matrix = [](const char* name, const sl::float4x4& m) {
      ImGui::TextUnformatted(name);
      for (int r = 0; r < 4; ++r) {
        ImGui::Text(
            "  %9.5f %9.5f %9.5f %9.5f", m[r].x, m[r].y, m[r].z, m[r].w);
      }
    };
    print_matrix("cameraViewToClip", c.cameraViewToClip);
    print_matrix("clipToCameraView", c.clipToCameraView);
    print_matrix("clipToPrevClip", c.clipToPrevClip);
    ImGui::TreePop();
  }
  return false;
}

bool DrawFeaturesPanel() {
  const sl_rr::Diagnostics d = sl_rr::CaptureDiagnostics();
  if (d.features.empty()) {
    ImGui::TextColored(kColorWarn, "No feature activity observed yet.");
    return false;
  }
  std::vector<std::vector<std::string>> rows;
  for (const auto& [feature, row] : d.features) {
    std::string version = "-";
    if (row.sl_major != 0 || row.ngx_major != 0) {
      std::stringstream v;
      v << row.sl_major << '.' << row.sl_minor << '.' << row.sl_build << " / " << row.ngx_major
        << '.' << row.ngx_minor << '.' << row.ngx_build;
      version = v.str();
    }
    rows.push_back({
        sl_rr::FeatureName(feature) + " (" + std::to_string(feature) + ")",
        std::to_string(row.evaluates),
        sl_rr::ResultName(row.last_eval_result),
        std::to_string(row.last_eval_inputs),
        sl_rr::ResultName(row.supported_result),
        row.loaded_result == sl_rr::kNever ? "-" : (row.loaded ? "yes" : "no"),
        version,
        row.probed ? "yes" : "no",
    });
  }
  ImGui::TextUnformatted("SL/NGX version column reads \"SL / NGX\".");
  return DrawRowTable(
      "##sl_rr_features",
      {"Feature", "Evaluates", "Last result", "Inputs", "Supported", "Loaded", "Version", "Probed"},
      rows);
}

bool DrawFeatureFunctionsPanel() {
  const sl_rr::Diagnostics d = sl_rr::CaptureDiagnostics();
  if (d.feature_functions.empty()) {
    ImGui::TextColored(kColorWarn, "No slGetFeatureFunction requests observed yet.");
    return false;
  }
  std::vector<std::vector<std::string>> rows;
  rows.reserve(d.feature_functions.size());
  for (const auto& fn : d.feature_functions) {
    rows.push_back({
        sl_rr::FeatureName(fn.feature),
        fn.name,
        std::to_string(fn.calls),
        sl_rr::ResultName(fn.last_result),
        std::to_string(fn.last_frame),
    });
  }
  return DrawRowTable(
      "##sl_rr_feature_functions", {"Feature", "Function", "Calls", "Last result", "Frame"}, rows);
}

bool DrawEvaluatesPanel() {
  const sl_rr::Diagnostics d = sl_rr::CaptureDiagnostics();
  if (d.evaluate_recent.empty()) {
    ImGui::TextColored(kColorWarn, "No slEvaluateFeature calls observed yet.");
    return false;
  }
  std::vector<std::vector<std::string>> rows;
  rows.reserve(d.evaluate_recent.size());
  for (const auto& call : d.evaluate_recent) {
    rows.push_back({
        std::to_string(call.frame),
        sl_rr::FeatureName(call.feature),
        std::to_string(call.num_inputs),
        call.primary_input_type != 0 ? Hex64(call.primary_input_type) : "-",
        sl_rr::ResultName(call.result),
        sl_rr::ModuleNameOf(call.caller),
    });
  }
  return DrawRowTable(
      "##sl_rr_evaluates", {"Frame", "Feature", "Inputs", "First input", "Result", "Caller"}, rows);
}

bool DrawHooksPanel() {
  const sl_rr::Diagnostics d = sl_rr::CaptureDiagnostics();
  std::vector<std::vector<std::string>> rows;
  rows.reserve(sl_rr::kHookCount);
  for (int i = 0; i < sl_rr::kHookCount; ++i) {
    const auto& hook = d.hooks[static_cast<size_t>(i)];
    rows.push_back({
        sl_rr::kHookNames[i],
        hook.installed ? "yes" : "no",
        std::to_string(hook.calls),
        hook.last_caller != nullptr ? sl_rr::ModuleNameOf(hook.last_caller) : "-",
    });
  }
  return DrawRowTable("##sl_rr_hooks", {"Hook", "Armed", "Calls", "Last caller"}, rows);
}

// ---------------------------------------------------------------------------

renodx::utils::settings::Settings settings = {
    new renodx::utils::settings::Setting{
        .value_type = renodx::utils::settings::SettingValueType::CUSTOM,
        .label = "Status",
        .section = "Ray Reconstruction",
        .tooltip = "Live Streamline state. Diagnostics only: hooks observe and forward.",
        .on_draw = [] { return DrawStatusPanel(); },
    },
    new renodx::utils::settings::Setting{
        .value_type = renodx::utils::settings::SettingValueType::BUTTON,
        .label = "Probe DLSS-RR",
        .section = "Ray Reconstruction",
        .group = "button-line-probe",
        .tooltip = "Calls slIsFeatureSupported / slIsFeatureLoaded / slGetFeatureVersion for "
                   "DLSS Ray Reconstruction at the next in-game Streamline call.",
        .on_change = []() { sl_rr::RequestProbe(0); },
    },
    new renodx::utils::settings::Setting{
        .value_type = renodx::utils::settings::SettingValueType::BUTTON,
        .label = "Probe all features",
        .section = "Ray Reconstruction",
        .group = "button-line-probe",
        .tooltip = "Same as above for DLSS, DLSS-G, Reflex, NIS, DeepDVC, etc.",
        .on_change = []() { sl_rr::RequestProbe(1); },
    },
    new renodx::utils::settings::Setting{
        .value_type = renodx::utils::settings::SettingValueType::BUTTON,
        .label = "Copy report",
        .section = "Ray Reconstruction",
        .group = "button-line-report",
        .tooltip = "Copies the full diagnostics report (all panels as text) to the clipboard.",
        .on_change = []() { ImGui::SetClipboardText(sl_rr::BuildReport().c_str()); },
    },
    new renodx::utils::settings::Setting{
        .value_type = renodx::utils::settings::SettingValueType::BUTTON,
        .label = "Write report to log",
        .section = "Ray Reconstruction",
        .group = "button-line-report",
        .tooltip = "Writes the full diagnostics report to reshade.log.",
        .on_change = []() { sl_rr::LogReport(sl_rr::BuildReport()); },
    },
    new renodx::utils::settings::Setting{
        .value_type = renodx::utils::settings::SettingValueType::BUTTON,
        .label = "Reset capture",
        .section = "Ray Reconstruction",
        .group = "button-line-report",
        .tooltip = "Clears captured counters, tags, constants and features. slInit and device "
                   "snapshots are kept.",
        .on_change = []() { sl_rr::ResetCapture(); },
    },
    new renodx::utils::settings::Setting{
        .value_type = renodx::utils::settings::SettingValueType::CUSTOM,
        .label = "Game buffers (tags)",
        .section = "Ray Reconstruction",
        .tooltip = "Every buffer the game hands Streamline via slSetTag/slSetTagForFrame.",
        .on_draw = [] { return DrawTagsPanel(); },
    },
    new renodx::utils::settings::Setting{
        .value_type = renodx::utils::settings::SettingValueType::CUSTOM,
        .label = "Frame constants",
        .section = "Ray Reconstruction",
        .tooltip = "Latest slSetConstants snapshot (jitterm MV scale, camera, matrices).",
        .on_draw = [] { return DrawConstantsPanel(); },
    },
    new renodx::utils::settings::Setting{
        .value_type = renodx::utils::settings::SettingValueType::CUSTOM,
        .label = "Features",
        .section = "Ray Reconstruction",
        .tooltip = "Per-feature evaluate/query results, including the DLSS-RR probe.",
        .on_draw = [] { return DrawFeaturesPanel(); },
    },
    new renodx::utils::settings::Setting{
        .value_type = renodx::utils::settings::SettingValueType::CUSTOM,
        .label = "Feature functions",
        .section = "Ray Reconstruction",
        .tooltip = "Which feature functions the game requests via slGetFeatureFunction.",
        .on_draw = [] { return DrawFeatureFunctionsPanel(); },
    },
    new renodx::utils::settings::Setting{
        .value_type = renodx::utils::settings::SettingValueType::CUSTOM,
        .label = "Recent evaluates",
        .section = "Ray Reconstruction",
        .tooltip = "The last slEvaluateFeature calls with caller module names.",
        .on_draw = [] { return DrawEvaluatesPanel(); },
    },
    new renodx::utils::settings::Setting{
        .value_type = renodx::utils::settings::SettingValueType::CUSTOM,
        .label = "Hook activity",
        .section = "Ray Reconstruction",
        .tooltip = "Call counts per hooked Streamline entry point.",
        .on_draw = [] { return DrawHooksPanel(); },
    },
    new renodx::utils::settings::Setting{
        .value_type = renodx::utils::settings::SettingValueType::TEXT,
        .label = std::string("Build: ") + renodx::utils::date::ISO_DATE_TIME,
        .section = "About",
    },
    new renodx::utils::settings::Setting{
        .value_type = renodx::utils::settings::SettingValueType::BUTTON,
        .label = "RenoDX Discord",
        .section = "About",
        .tint = 0x5865F2,
        .on_change = []() { renodx::utils::platform::LaunchURL("https://discord.gg/", "Ce9bQHQrSV"); },
    },
};

void OnPresent(
    reshade::api::command_queue* queue, reshade::api::swapchain* swapchain,
    const reshade::api::rect* source_rect, const reshade::api::rect* dest_rect,
    uint32_t dirty_rect_count, const reshade::api::rect* dirty_rects) {
  (void)queue;
  (void)swapchain;
  (void)source_rect;
  (void)dest_rect;
  (void)dirty_rect_count;
  (void)dirty_rects;
  sl_rr::Poll();
}

// Keep the addon loaded for the process lifetime so installed hooks cannot
// outlive its code.
extern "C" IMAGE_DOS_HEADER __ImageBase;
void PinModule() {
  HMODULE self = nullptr;
  if (GetModuleHandleExW(
          GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
          reinterpret_cast<LPCWSTR>(&__ImageBase), &self)
      == 0) {
    sl_rr::LogWarn("could NOT pin the addon module — hooks may dangle on unload");
  }
}

}  // namespace

extern "C" __declspec(dllexport) constexpr const char* NAME = "RenoDX - Forza Horizon 6";
extern "C" __declspec(dllexport) constexpr const char* DESCRIPTION =
    "DLSS Ray Reconstruction for Forza Horizon 6 via Streamline (diagnostics build)";

BOOL APIENTRY DllMain(HMODULE h_module, DWORD fdw_reason, LPVOID lpv_reserved) {
  switch (fdw_reason) {
    case DLL_PROCESS_ATTACH:
      if (!reshade::register_addon(h_module)) return FALSE;
      PinModule();
      sl_rr::InstallLoaderHooks();
      reshade::register_event<reshade::addon_event::present>(OnPresent);
      break;
    case DLL_PROCESS_DETACH:
      sl_rr::UninstallHooks();
      reshade::unregister_event<reshade::addon_event::present>(OnPresent);
      break;
  }

  renodx::utils::settings::use_presets = false;  // diagnostics mod: no presets
  renodx::utils::settings::Use(fdw_reason, &settings);

  if (fdw_reason == DLL_PROCESS_DETACH) {
    reshade::unregister_addon(h_module);
  }

  return TRUE;
}
