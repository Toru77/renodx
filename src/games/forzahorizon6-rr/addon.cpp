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

// The value the session booted with is kept so the overlay can say "restart"
// when the saved one differs (both are read by sl_rr at slInit time only).
float rr_init_inject_setting = 1.f;
float rr_init_inject_boot_value = 1.f;
float rr_runtime_load_setting = 1.f;
float rr_runtime_load_boot_value = 1.f;
// M2: live toggle, applies on the next evaluate.
float rr_redirect_setting = 1.f;
// Raises Streamline's own log level while the mod captures its messages.
float rr_sl_log_verbose_setting = 1.f;
// DLSSD preset values for the A..F slider (ePresetA..ePresetF; F is the
// current RR 4.5 default preset). Indices match the slider labels.
constexpr uint32_t kRrPresetValues[6] = {1, 2, 3, 4, 5, 6};
float rr_preset_setting = 5.f;  // index into kRrPresetValues; F by default

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
    ImGui::Text(
        "pref v%u, showConsole %s, plugin paths %u",
        d.init_pref_version, d.init_show_console ? "yes" : "no", d.init_num_plugin_paths);
    if (d.init_feature_count > 0 || d.init_effective_count > 0) {
      std::stringstream features;
      features << "featuresToLoad:";
      for (uint32_t i = 0;
           i < d.init_effective_count
           && i < static_cast<uint32_t>(d.init_features_effective.size());
           ++i) {
        features << ' ' << sl_rr::FeatureName(d.init_features_effective[i]);
      }
      if (d.init_rr_injected) features << "  (+DLSS-RR, mod)";
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
  const auto& probe = d.rr_probe;
  if (probe.load_attempts > 0) {
    ImGui::TextColored(
        probe.loaded_after_load ? kColorOk : kColorBad,
        "runtime load attempts: %u, last %s (loaded after: %s)", probe.load_attempts,
        sl_rr::ResultName(probe.load_result).c_str(), probe.loaded_after_load ? "yes" : "no");
  } else if (d.init_rr_injected) {
    ImGui::TextUnformatted("load: requested at slInit (featuresToLoad + DLSS-RR)");
  }
  if (probe.plugin_module_loaded) {
    ImGui::TextWrapped("plugin: %s", probe.plugin_module_path.c_str());
  } else if (probe.ran) {
    ImGui::TextColored(
        kColorWarn, "plugin: not loaded%s",
        probe.plugin_file_present ? " (sl.dlss_d.dll present on disk)"
                                  : " (sl.dlss_d.dll not found)");
  }
  if (probe.ngx_module_loaded) {
    ImGui::TextWrapped("ngx module: %s", probe.ngx_module_path.c_str());
  }
  const auto& redirect = d.rr_redirect;
  if (sl_rr::GetRrRedirect()) {
    if (redirect.redirected > 0 || redirect.fallbacks > 0) {
      const bool healthy = redirect.redirected > 0 && redirect.last_reason.empty();
      ImGui::TextColored(
          healthy ? kColorOk : kColorWarn,
          "Redirect: %llu RR frames, %llu fallbacks (frame %u)",
          static_cast<unsigned long long>(redirect.redirected),
          static_cast<unsigned long long>(redirect.fallbacks), redirect.last_frame);
      if (!redirect.last_reason.empty()) {
        ImGui::TextColored(kColorBad, "last fallback: %s", redirect.last_reason.c_str());
      } else {
        ImGui::Text(
            "last: slDLSSDSetOptions %s, RR evaluate %s",
            sl_rr::ResultName(redirect.last_set_options_result).c_str(),
            sl_rr::ResultName(redirect.last_eval_result).c_str());
      }
    } else {
      ImGui::TextUnformatted("Redirect: on — waiting for the next DLSS evaluate");
    }
  } else {
    ImGui::TextUnformatted("Redirect: off — the game's DLSS SR runs unchanged");
  }
  ImGui::Text(
      "RR preset: %s (%u) on all hints", sl_rr::PresetName(sl_rr::GetRrPreset()),
      sl_rr::GetRrPreset());
  if (d.guides.created) {
    ImGui::Text(
        "guides: placeholder %ux%u (recreates %u)", d.guides.width, d.guides.height,
        d.guides.recreates);
  } else if (!d.guides.last_error.empty()) {
    ImGui::TextColored(kColorWarn, "guides: %s", d.guides.last_error.c_str());
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
      if (tag.desc_mips > 1) size += " m" + std::to_string(tag.desc_mips);
    } else if (tag.sl_width != 0 || tag.sl_height != 0) {
      char buffer[64] = {};
      snprintf(buffer, sizeof buffer, "%ux%u", tag.sl_width, tag.sl_height);
      size = buffer;
      if (tag.sl_layers > 1) size += " x" + std::to_string(tag.sl_layers);
      if (tag.sl_mips > 1) size += " m" + std::to_string(tag.sl_mips);
    }
    std::string extent = "-";
    if (tag.extent_w != 0 || tag.extent_h != 0 || tag.extent_left != 0 || tag.extent_top != 0) {
      char buffer[64] = {};
      if (tag.extent_left != 0 || tag.extent_top != 0) {
        snprintf(
            buffer, sizeof buffer, "l%u t%u %ux%u", tag.extent_left, tag.extent_top,
            tag.extent_w, tag.extent_h);
      } else {
        snprintf(buffer, sizeof buffer, "%ux%u", tag.extent_w, tag.extent_h);
      }
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
        sl_rr::EvalInputLabel(call.input_info[0]),
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

bool DrawDlssOptionsPanel() {
  const sl_rr::Diagnostics d = sl_rr::CaptureDiagnostics();
  const auto& opts = d.dlss_options;
  if (!opts.captured && !opts.optimal_captured) {
    ImGui::TextColored(kColorWarn, "slDLSSSetOptions / slDLSSGetOptimalSettings not seen yet.");
    ImGui::TextUnformatted("Captured through forward-only wrappers; no behavior is changed.");
    return false;
  }
  ImGui::Text(
      "setOptions calls %u, optimal queries %u, last frame %u", opts.set_options_calls,
      opts.get_optimal_calls, opts.last_frame);
  ImGui::Text(
      "mode %s  output %ux%u", sl::getDLSSModeAsStr(static_cast<sl::DLSSMode>(opts.mode)),
      opts.output_width, opts.output_height);
  const std::string hdr = opts.color_buffers_hdr >= 0
                              ? sl_rr::BooleanName(static_cast<sl::Boolean>(opts.color_buffers_hdr))
                              : "?";
  const std::string auto_exposure =
      opts.use_auto_exposure >= 0
          ? sl_rr::BooleanName(static_cast<sl::Boolean>(opts.use_auto_exposure))
          : "?";
  const std::string alpha =
      opts.alpha_upscaling >= 0
          ? sl_rr::BooleanName(static_cast<sl::Boolean>(opts.alpha_upscaling))
          : "?";
  ImGui::Text(
      "preExposure %.4f  exposureScale %.4f  HDR %s  autoExposure %s  alphaUpscale %s",
      opts.pre_exposure, opts.exposure_scale, hdr.c_str(), auto_exposure.c_str(), alpha.c_str());
  static const char* const kPresetLabels[6] = {
      "DLAA", "Quality", "Balanced", "Performance", "UltraPerf", "UltraQuality"};
  std::stringstream presets;
  presets << "presets:";
  for (int i = 0; i < 6; ++i) {
    presets << ' ' << kPresetLabels[i] << '=' << sl_rr::PresetName(opts.presets[static_cast<size_t>(i)]);
  }
  ImGui::TextUnformatted(presets.str().c_str());
  if (opts.optimal_captured) {
    ImGui::Text(
        "game optimal query: mode %s, %s render %ux%u sharpness %.3f (min %ux%u, max %ux%u)",
        sl::getDLSSModeAsStr(static_cast<sl::DLSSMode>(opts.optimal_mode)),
        sl_rr::ResultName(opts.optimal_result).c_str(), opts.optimal_render_width,
        opts.optimal_render_height, opts.optimal_sharpness, opts.render_width_min,
        opts.render_height_min, opts.render_width_max, opts.render_height_max);
  }
  return false;
}

bool DrawRrProbePanel() {
  const sl_rr::Diagnostics d = sl_rr::CaptureDiagnostics();
  const auto& probe = d.rr_probe;
  if (!probe.ran) {
    ImGui::TextColored(
        kColorWarn, "Probe has not run yet (automatic at the first DLSS evaluate).");
    return false;
  }
  ImGui::Text("frame %u  load attempts %u", probe.frame, probe.load_attempts);
  if (probe.load_attempts > 0) {
    ImGui::Text(
        "last load: %s -> loaded after: %s", sl_rr::ResultName(probe.load_result).c_str(),
        probe.loaded_after_load ? "yes" : "no");
  } else {
    ImGui::TextUnformatted("load: requested via slInit (no runtime attempt needed)");
  }
  if (probe.plugin_module_loaded) {
    ImGui::TextWrapped("plugin module: %s", probe.plugin_module_path.c_str());
  } else if (probe.plugin_file_present) {
    ImGui::TextWrapped("plugin: not loaded; sl.dlss_d.dll present at %s", probe.plugin_file_path.c_str());
  } else {
    ImGui::TextColored(kColorWarn, "plugin: sl.dlss_d.dll not found");
  }
  if (probe.ngx_module_loaded) {
    ImGui::TextWrapped("ngx module: %s", probe.ngx_module_path.c_str());
  } else {
    ImGui::TextUnformatted("ngx module (nvngx_dlssd.dll): not loaded");
  }
  if (probe.requirements_attempted) {
    ImGui::Text(
        "requirements: %s flags 0x%X [%s]", sl_rr::ResultName(probe.requirements_result).c_str(),
        probe.requirement_flags, sl_rr::RequirementFlagsText(probe.requirement_flags).c_str());
    ImGui::Text(
        "max viewports %u, max CPU threads %u", probe.max_viewports, probe.max_cpu_threads);
    if (!probe.required_tags.empty()) {
      std::stringstream tags;
      tags << "required tags:";
      for (uint32_t tag : probe.required_tags) tags << ' ' << sl_rr::BufferName(tag);
      ImGui::TextWrapped("%s", tags.str().c_str());
    }
    if (!probe.driver_version_required.empty() && probe.driver_version_required != "0.0.0") {
      ImGui::Text("driver required: %s", probe.driver_version_required.c_str());
    }
  }
  ImGui::Text(
      "functions: SetOptions %s, GetOptimalSettings %s, GetState %s",
      sl_rr::ResultName(probe.fn_set_options_result).c_str(),
      sl_rr::ResultName(probe.fn_get_optimal_result).c_str(),
      sl_rr::ResultName(probe.fn_get_state_result).c_str());
  if (probe.trial_optimal_attempted) {
    if (static_cast<sl::Result>(probe.trial_optimal_result) == sl::Result::eOk) {
      ImGui::Text(
          "trial optimal: render %ux%u sharpness %.3f (min %ux%u, max %ux%u)",
          probe.trial_render_width, probe.trial_render_height, probe.trial_sharpness,
          probe.trial_render_width_min, probe.trial_render_height_min,
          probe.trial_render_width_max, probe.trial_render_height_max);
    } else {
      ImGui::TextColored(
          kColorWarn, "trial optimal: %s", sl_rr::ResultName(probe.trial_optimal_result).c_str());
    }
  } else {
    ImGui::TextUnformatted("trial optimal: skipped (no captured DLSS options yet)");
  }
  if (probe.trial_state_attempted) {
    if (static_cast<sl::Result>(probe.trial_state_result) == sl::Result::eOk) {
      ImGui::Text(
          "trial state: %.1f MB estimated VRAM",
          static_cast<double>(probe.trial_vram_bytes) / (1024.0 * 1024.0));
    } else {
      ImGui::TextColored(
          kColorWarn, "trial state: %s", sl_rr::ResultName(probe.trial_state_result).c_str());
    }
  }
  if (!probe.dlss_modules.empty()) {
    if (ImGui::TreeNode("dlss modules in process")) {
      for (const auto& module : probe.dlss_modules) {
        ImGui::TextWrapped("%s", module.c_str());
      }
      ImGui::TreePop();
    }
  }
  return false;
}

bool DrawSlLogPanel() {
  const sl_rr::Diagnostics d = sl_rr::CaptureDiagnostics();
  ImGui::Text(
      "callback: %s (game chained: %s), verbose: %s, messages: %llu (warn/error: %llu)",
      d.sl_log_callback_installed ? "installed by mod" : "not installed",
      d.sl_log_game_chained ? "yes" : "no", d.sl_log_verbose ? "requested" : "default",
      static_cast<unsigned long long>(d.sl_log_total),
      static_cast<unsigned long long>(d.sl_log_warn_errors));
  if (d.sl_log_last_problems.empty() && d.sl_log_last.empty()) {
    ImGui::TextColored(kColorWarn, "No Streamline log messages captured yet.");
    return false;
  }
  if (!d.sl_log_last_problems.empty()) {
    ImGui::SeparatorText("Last warn/error");
    for (const auto& entry : d.sl_log_last_problems) {
      std::string suffix;
      if (entry.repeats > 0) suffix = " (x" + std::to_string(entry.repeats + 1) + ")";
      ImGui::TextColored(
          entry.type == 2 ? kColorBad : kColorWarn, "[%s]%s", sl_rr::SlLogTypeName(entry.type),
          suffix.c_str());
      ImGui::TextWrapped("%s", entry.message.c_str());
    }
  }
  if (!d.sl_log_last.empty()) {
    ImGui::SeparatorText("Last messages");
    for (const auto& entry : d.sl_log_last) {
      ImGui::TextWrapped("[%s] %s", sl_rr::SlLogTypeName(entry.type), entry.message.c_str());
    }
  }
  return false;
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
        .key = "RrInitInject",
        .binding = &rr_init_inject_setting,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Request DLSS-RR at slInit (restart required)",
        .section = "Ray Reconstruction",
        .tooltip = "Appends DLSS Ray Reconstruction to the game's slInit featuresToLoad so the"
                   " sl.dlss_d plugin loads with the game. Takes effect on the next launch.",
        .on_change_value = [](float, float value) { sl_rr::SetInitInjection(value != 0.f); },
    },
    new renodx::utils::settings::Setting{
        .value_type = renodx::utils::settings::SettingValueType::TEXT,
        .label = "Restart required.",
        .section = "Ray Reconstruction",
        .tint = 0xFF0000,
        .is_visible = []() { return rr_init_inject_setting != rr_init_inject_boot_value; },
    },
    new renodx::utils::settings::Setting{
        .key = "RrAutoLoad",
        .binding = &rr_runtime_load_setting,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Auto-load DLSS-RR at runtime",
        .section = "Ray Reconstruction",
        .tooltip = "If the slInit request did not load DLSS-RR, the probe calls"
                   " slSetFeatureLoaded(DLSS-RR, true) and reports the result.",
        .on_change_value = [](float, float value) { sl_rr::SetRuntimeLoad(value != 0.f); },
    },
    new renodx::utils::settings::Setting{
        .key = "RrRedirect",
        .binding = &rr_redirect_setting,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Replace DLSS SR with Ray Reconstruction",
        .section = "Ray Reconstruction",
        .tooltip = "Redirects the game's slEvaluateFeature(DLSS) to DLSS-RR with options"
                   " mirrored from the game's own settings (internal resolution stays"
                   " game-controlled). Any frame where the redirect fails runs on DLSS SR"
                   " instead.",
        .on_change_value = [](float, float value) { sl_rr::SetRrRedirect(value != 0.f); },
    },
    new renodx::utils::settings::Setting{
        .key = "RrSlLogVerbose",
        .binding = &rr_sl_log_verbose_setting,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "SL verbose log capture (diagnostics, restart required)",
        .section = "Ray Reconstruction",
        .tooltip = "Raises Streamline's log level to verbose while the mod captures its"
                   " messages (bounded, shown in the report). Warnings/errors are always"
                   " captured regardless of this switch.",
        .on_change_value = [](float, float value) { sl_rr::SetSlLogVerbose(value != 0.f); },
    },
    new renodx::utils::settings::Setting{
        .key = "RrPreset",
        .binding = &rr_preset_setting,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 5.f,
        .label = "RR preset",
        .section = "Ray Reconstruction",
        .tooltip = "DLSS Ray Reconstruction render preset, applied to every mode. F is the"
                   " current RR 4.5 default preset; D/E are the transformer models; A-C are"
                   " NVIDIA-deprecated. Changes apply on the next frame.",
        .labels = {"A", "B", "C", "D", "E", "F"},
        .on_change_value = [](float, float value) {
          sl_rr::SetRrPreset(kRrPresetValues[static_cast<int>(value)]);
        },
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
        .label = "DLSS options (captured)",
        .section = "Ray Reconstruction",
        .tooltip = "Options the game passed to slDLSSSetOptions / slDLSSGetOptimalSettings,"
                   " captured through forward-only wrappers.",
        .on_draw = [] { return DrawDlssOptionsPanel(); },
    },
    new renodx::utils::settings::Setting{
        .value_type = renodx::utils::settings::SettingValueType::CUSTOM,
        .label = "DLSS-RR probe",
        .section = "Ray Reconstruction",
        .tooltip = "Load status, serving plugin, requirements and trial calls for DLSS Ray"
                   " Reconstruction.",
        .on_draw = [] { return DrawRrProbePanel(); },
    },
    new renodx::utils::settings::Setting{
        .value_type = renodx::utils::settings::SettingValueType::CUSTOM,
        .label = "SL log (captured)",
        .section = "Ray Reconstruction",
        .tooltip = "Streamline's own log messages, captured through the logMessageCallback"
                   " installed at slInit.",
        .on_draw = [] { return DrawSlLogPanel(); },
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

  if (fdw_reason == DLL_PROCESS_ATTACH) {
    // Keep the boot values for the "restart required" hint and push the loaded
    // settings into sl_rr before the game reaches slInit.
    rr_init_inject_boot_value = rr_init_inject_setting;
    rr_runtime_load_boot_value = rr_runtime_load_setting;
    sl_rr::SetInitInjection(rr_init_inject_setting != 0.f);
    sl_rr::SetRuntimeLoad(rr_runtime_load_setting != 0.f);
    sl_rr::SetRrRedirect(rr_redirect_setting != 0.f);
    sl_rr::SetSlLogVerbose(rr_sl_log_verbose_setting != 0.f);
    int preset_index = static_cast<int>(rr_preset_setting);
    if (preset_index < 0) preset_index = 0;
    if (preset_index > 5) preset_index = 5;
    sl_rr::SetRrPreset(kRrPresetValues[preset_index]);
  }

  if (fdw_reason == DLL_PROCESS_DETACH) {
    reshade::unregister_addon(h_module);
  }

  return TRUE;
}
