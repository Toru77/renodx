/*
 * Copyright (C) 2026
 * SPDX-License-Identifier: MIT
 *
 * Far Cry 6 DLSS — replaces the game's TAA pass with NVIDIA DLSS Super
 * Resolution via the D3D12 NGX path.
 *
 * NOTE: the HLSL files under antialiasing/ and uishaders/ are reference
 * decompilations only. They are intentionally NOT registered as custom
 * shaders; the TAA draw is intercepted at the draw event, DLSS evaluates into
 * the TAA output target, and the original draw is skipped.
 */

#define ImTextureID ImU64

#define DEBUG_LEVEL_0

#include <deps/imgui/imgui.h>
#include <include/reshade.hpp>

#include <embed/shaders.h>

#include "../../utils/settings.hpp"
#include "./dlss.hpp"

namespace {

inline const char* SlotState(int i) {
  return farcry6_dlss::dlss::slot_hit[i].load() == 1 ? "ok" : "-";
}

static bool DlssStatusDraw() {
  using namespace farcry6_dlss::dlss;
  const std::string devices = DeviceSummary();
  ImGui::Text("Devices: %s", devices.c_str());
  ImGui::Text(
      "TAA draws: %llu  DLSS replacements: %llu",
      static_cast<unsigned long long>(stat_taa_draws.load()),
      static_cast<unsigned long long>(stat_dlss_ok.load()));
  ImGui::Text("State: %s", FailCodeText(stat_fail_code.load()));
  ImGui::Text(
      "Slots: b0:%s t0:%s t2:%s t5:%s t6:%s", SlotState(0), SlotState(1), SlotState(2),
      SlotState(3), SlotState(4));
  ImGui::Text(
      "Descriptor fingerprints: %llu",
      static_cast<unsigned long long>(CountFingerprints()));
  if (dlss_bypass_ngx != 0.f) {
    ImGui::TextUnformatted("Inputs: bypass (known-good grey through the output path)");
  } else {
    ImGui::Text(
        "Inputs: colour=%s motion=%s depth=%s",
        dlss_color_source >= 0.5f ? "placeholder" : "game",
        dlss_motion_source >= 0.5f ? "placeholder" : "game",
        dlss_depth_source >= 2.5f ? "placeholder"
            : (dlss_depth_source > 1.5f ? "t6" : (dlss_depth_source > 0.5f ? "t5" : "scene")));
  }
  ImGui::TextUnformatted("Jitter: read from the engine's projection when stable");
  ImGui::Text(
      "NGX log: %llu lines   hdr=%s   restore bindings=%s   depth transition=%s",
      static_cast<unsigned long long>(ngx_log.seen.load()),
      dlss_flag_is_hdr != 0.f ? "on" : "off",
      dlss_restore_state != 0.f ? "on" : "off",
      dlss_transition_depth != 0.f ? "on" : "off");
  {
    const std::lock_guard<std::mutex> lock(report_mutex);
    ImGui::Text(
        "Last eval: in=%ux%u target=%ux%u mv=%ux%u depth=%ux%u %s",
        deploy_report.input_width, deploy_report.input_height,
        deploy_report.target_width, deploy_report.target_height,
        deploy_report.mv_width, deploy_report.mv_height,
        deploy_report.depth_width, deploy_report.depth_height,
        GetPerfQualityName(deploy_report.quality));
  }
  ImGui::TextWrapped("Probe: %s", BuildProbeReport().c_str());
  return false;
}

renodx::utils::settings::Settings settings = {
    new renodx::utils::settings::Setting{
        .key = "DLSSCopyReport",
        .binding = nullptr,
        .value_type = renodx::utils::settings::SettingValueType::BUTTON,
        .default_value = 0.f,
        .label = "Copy Report",
        .section = "Antialiasing",
        .tooltip = "Copies the live DLSS pipeline state (inputs, sizes, jitter, counters) to the clipboard.",
        .on_click = [] {
          ImGui::SetClipboardText(farcry6_dlss::dlss::BuildDebugReport().c_str());
          return true;
        },
    },
    new renodx::utils::settings::Setting{
        .key = "DLSSStatus",
        .binding = nullptr,
        .value_type = renodx::utils::settings::SettingValueType::CUSTOM,
        .default_value = 0.f,
        .label = "Status",
        .section = "Antialiasing",
        .tooltip = "Live DLSS pipeline status. Always visible, even when no NVIDIA device is present.",
        .on_draw = [] { return DlssStatusDraw(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DLSSEnabled",
        .binding = &farcry6_dlss::dlss::dlss_enabled,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "DLSS Replacement",
        .section = "Antialiasing",
        .tooltip = "Runs NVIDIA DLSS in place of Far Cry 6's TAA pass when all of its inputs resolve. Requires nvngx_dlss.dll next to the game executable. When off, the game's own TAA runs unchanged.",
        .is_visible = []() { return farcry6_dlss::dlss::IsSupported(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DLSSOutputMode",
        .binding = &farcry6_dlss::dlss::dlss_output_mode,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f,
        .label = "DLSS Output",
        .section = "Antialiasing",
        .tooltip = "What DLSS is allowed to replace, for isolating a rendering fault. Replace skips the TAA draw and puts DLSS output in its target. Evaluate only runs the whole chain but keeps the game's own TAA output. Resolve only resolves and reports the inputs without running DLSS at all.",
        .labels = {"Replace", "Evaluate only", "Resolve only"},
        .is_enabled = []() { return farcry6_dlss::dlss::dlss_enabled != 0.f; },
        .is_visible = []() { return farcry6_dlss::dlss::IsSupported(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DLSSPreset",
        .binding = &farcry6_dlss::dlss::dlss_render_preset,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f,
        .label = "DLSS Preset",
        .section = "Antialiasing",
        .labels = {"Default", "F - CNN", "J - Transformer 1", "K - Transformer 1", "L - Transformer 2", "M - Transformer 2"},
        .is_enabled = []() { return farcry6_dlss::dlss::dlss_enabled != 0.f; },
        .is_visible = []() { return farcry6_dlss::dlss::IsSupported(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DLSSJitterSource",
        .binding = &farcry6_dlss::dlss::dlss_jitter_source,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 1.f,
        .label = "Jitter Source",
        .section = "Antialiasing",
        .tooltip = "What DLSS is told about the game's render jitter. Zero passes none. Projection reads it from the engine's own projection matrix, where a renderer has to apply a sub-pixel shift in normalised device coordinates - the value is read from the matrix, never inferred from a number's size or movement, and zero is passed whenever no projection with a stable shape is in hand.",
        .labels = {"Zero", "Projection matrix"},
        .is_enabled = []() { return farcry6_dlss::dlss::dlss_enabled != 0.f; },
        .is_visible = []() { return farcry6_dlss::dlss::IsSupported(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DLSSDepthSource",
        .binding = &farcry6_dlss::dlss::dlss_depth_source,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f,
        .label = "Depth Source",
        .section = "Antialiasing",
        .tooltip = "What feeds DLSS as depth. Scene depth is the game's own depth buffer, identified by how the engine binds it. t5/t6 are the TAA anti-ghosting masks, which are not depth. Placeholder feeds a far plane, so nothing occludes anything: a valid input with neutral content, for isolating a rendering fault.",
        .labels = {"Scene depth", "t5 (mask)", "t6 (mask)", "Placeholder (far plane)"},
        .is_enabled = []() { return farcry6_dlss::dlss::dlss_enabled != 0.f; },
        .is_visible = []() { return farcry6_dlss::dlss::IsSupported(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DLSSColourSource",
        .binding = &farcry6_dlss::dlss::dlss_color_source,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f,
        .label = "Colour Source",
        .section = "Antialiasing",
        .tooltip = "What feeds DLSS as colour. Placeholder substitutes a flat mid grey of the input size, so DLSS runs normally on input that cannot contain scene content. If artifacts disappear with it, the game's colour is implicated; if they survive, they are not coming from the colour.",
        .labels = {"Game (t2)", "Placeholder (grey)"},
        .is_enabled = []() { return farcry6_dlss::dlss::dlss_enabled != 0.f; },
        .is_visible = []() { return farcry6_dlss::dlss::IsSupported(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DLSSMotionSource",
        .binding = &farcry6_dlss::dlss::dlss_motion_source,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f,
        .label = "Motion Source",
        .section = "Antialiasing",
        .tooltip = "What feeds DLSS as motion vectors. Placeholder substitutes zero motion at the input size, with the jittered-motion flag dropped to match. If artifacts disappear with it, the decoded game motion is implicated.",
        .labels = {"Game (decoded)", "Placeholder (zero motion)"},
        .is_enabled = []() { return farcry6_dlss::dlss::dlss_enabled != 0.f; },
        .is_visible = []() { return farcry6_dlss::dlss::IsSupported(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DLSSBypassNgx",
        .binding = &farcry6_dlss::dlss::dlss_bypass_ngx,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f,
        .label = "Bypass NGX",
        .section = "Antialiasing",
        .tooltip = "Writes a known-good flat grey through the same transitions and copy the DLSS path uses, with NGX not involved at all, and skips the game's TAA draw. If the artifacts survive this, they are ours rather than DLSS's. Overrides DLSS Output and all input sources.",
        .is_enabled = []() { return farcry6_dlss::dlss::dlss_enabled != 0.f; },
        .is_visible = []() { return farcry6_dlss::dlss::IsSupported(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DLSMVJittered",
        .binding = &farcry6_dlss::dlss::dlss_motion_vectors_jittered,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Motion Vectors Jittered",
        .section = "Antialiasing",
        .tooltip = "Tell DLSS the motion vectors span jittered frames (the scene is rendered jittered). Toggle for A/B testing if motion looks wrong.",
        .is_enabled = []() { return farcry6_dlss::dlss::dlss_enabled != 0.f; },
        .is_visible = []() { return farcry6_dlss::dlss::IsSupported(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DLSSDepthInverted",
        .binding = &farcry6_dlss::dlss::dlss_depth_inverted,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f,
        .label = "Depth Inverted",
        .section = "Antialiasing",
        .tooltip = "Tell DLSS the depth buffer is reverse-Z. Toggle for A/B testing if disocclusion handling looks wrong.",
        .is_enabled = []() { return farcry6_dlss::dlss::dlss_enabled != 0.f; },
        .is_visible = []() { return farcry6_dlss::dlss::IsSupported(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DLSSRestoreState",
        .binding = &farcry6_dlss::dlss::dlss_restore_state,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Restore Bindings After DLSS",
        .section = "Antialiasing",
        .tooltip = "Re-applies the game's descriptor heaps and root signature after every DLSS evaluation. NGX dispatches on the command list directly, behind ReShade's hooks, and the descriptor heap binding is shared between compute and graphics, so the game's following draws would otherwise read their descriptors out of NGX's heap. Turn this off only to A/B whether it is the difference.",
        .is_enabled = []() { return farcry6_dlss::dlss::dlss_enabled != 0.f; },
        .is_visible = []() { return farcry6_dlss::dlss::IsSupported(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DLSSTransitionDepth",
        .binding = &farcry6_dlss::dlss::dlss_transition_depth,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Transition Depth For DLSS",
        .section = "Antialiasing",
        .tooltip = "Moves the scene depth into a shader-readable state for the DLSS read and restores the state the engine had it in. The engine leaves it in a depth-stencil state, where a shader read returns zeros - a depth view that is black in DLSS's debug overlay. Turn this off to see exactly that.",
        .is_enabled = []() { return farcry6_dlss::dlss::dlss_enabled != 0.f; },
        .is_visible = []() { return farcry6_dlss::dlss::IsSupported(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DLSSFlagIsHdr",
        .binding = &farcry6_dlss::dlss::dlss_flag_is_hdr,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f,
        .label = "Input Is HDR",
        .section = "Antialiasing",
        .tooltip = "Declares the colour input as HDR, meaning values above 1.0. Far Cry 6's TAA target is 16-bit float linear, which suggests it is, but every previous build ran with this off, so A/B it rather than assume.",
        .is_enabled = []() { return farcry6_dlss::dlss::dlss_enabled != 0.f; },
        .is_visible = []() { return farcry6_dlss::dlss::IsSupported(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DLSSMVScale",
        .binding = &farcry6_dlss::dlss::dlss_mv_scale,
        .default_value = 1.f,
        .label = "Motion Vector Scale",
        .section = "Antialiasing",
        .tooltip = "Calibration multiplier for the decoded motion vectors. 1.0 is nominal (decode x 0.1 x resolution).",
        .min = 0.1f,
        .max = 5.f,
        .is_enabled = []() { return farcry6_dlss::dlss::dlss_enabled != 0.f; },
        .is_visible = []() { return farcry6_dlss::dlss::IsSupported(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DLSSDebugLogging",
        .binding = &farcry6_dlss::dlss::dlss_debug_logging,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f,
        .label = "Debug Logging",
        .section = "Antialiasing",
        .tooltip = "Log DLSS evaluation parameters and jitter detection to ReShade.log (throttled).",
        .is_enabled = []() { return farcry6_dlss::dlss::dlss_enabled != 0.f; },
        .is_visible = []() { return farcry6_dlss::dlss::IsSupported(); },
    },
};

void OnPresetOff() {}

bool initialized = false;

}  // namespace

extern "C" __declspec(dllexport) constexpr const char* NAME = "RenoDX - Far Cry 6 DLSS";
extern "C" __declspec(dllexport) constexpr const char* DESCRIPTION = "Replaces Far Cry 6 TAA with NVIDIA DLSS Super Resolution";

BOOL APIENTRY DllMain(HMODULE h_module, DWORD fdw_reason, LPVOID lpv_reserved) {
  switch (fdw_reason) {
    case DLL_PROCESS_ATTACH:
      if (!reshade::register_addon(h_module)) return FALSE;
      initialized = true;
      break;
    case DLL_PROCESS_DETACH:
      reshade::unregister_addon(h_module);
      break;
  }

  renodx::utils::settings::Use(fdw_reason, &settings, &OnPresetOff);
  farcry6_dlss::dlss::Use(fdw_reason);

  return TRUE;
}
