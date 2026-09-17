/*
 * Copyright (C) 2026
 * SPDX-License-Identifier: MIT
 *
 * Far Cry 6 DLSS — replaces the game's TAA passes (0xFF0AF25B / 0x4AD8172A)
 * with NVIDIA DLSS Super Resolution via the D3D12 NGX path.
 *
 * NOTE: the HLSL files under antialiasing/ and uishaders/ are reference
 * decompilations only. They are intentionally NOT registered as custom
 * shaders; the TAA draw is intercepted at the draw event, DLSS evaluates
 * into the TAA output target, and the original draw is skipped.
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
  switch (farcry6_dlss::dlss::slot_hit[i].load()) {
    case 1: return "hit";
    case 2: return "dummy";
    default: return "-";
  }
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
  if (jitter_state.adopted) {
    ImGui::Text(
        "Jitter: game b0[%d]=(%.5f, %.5f)", jitter_state.candidate,
        static_cast<double>(jitter_state.x), static_cast<double>(jitter_state.y));
  } else if (dlss_jitter_source == 0.f) {
    ImGui::TextUnformatted("Jitter: zero (manual)");
  } else {
    ImGui::TextUnformatted("Jitter: scanning b0 (zero until adopted)");
  }
  int learned_count = 0;
  for (const auto& s : learned) {
    if (s.count > 0u || s.is_push) ++learned_count;
  }
  ImGui::Text("Inputs: %d/5 slots learned", learned_count);
  ImGui::Text("Slots: b0:%s t0:%s t2:%s t5:%s t6:%s", SlotState(0), SlotState(1), SlotState(2),
              SlotState(3), SlotState(4));
  return false;
}

renodx::utils::settings::Settings settings = {
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
        .tooltip = "Runs NVIDIA DLSS in place of Far Cry 6's TAA passes when the TAA resources can be resolved. Requires nvngx_dlss.dll next to the game executable. When off (or when resources cannot be resolved), the game's own TAA runs unchanged.",
        .is_visible = []() { return farcry6_dlss::dlss::IsSupported(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DLSSAllowFallback",
        .binding = &farcry6_dlss::dlss::dlss_allow_fallback,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f,
        .label = "Experimental Fallback",
        .section = "Antialiasing",
        .tooltip = "Run DLSS even when some TAA inputs do not resolve, using zero-motion / far-plane stand-ins. The image will be softer than TAA. Pipeline testing only; off keeps vanilla TAA whenever inputs are missing.",
        .is_enabled = []() { return farcry6_dlss::dlss::dlss_enabled != 0.f; },
        .is_visible = []() { return farcry6_dlss::dlss::IsSupported(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DLSSMode",
        .binding = &farcry6_dlss::dlss::dlss_mode,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f,
        .label = "DLSS Mode",
        .section = "Antialiasing",
        .tooltip = "Auto runs DLAA at native resolution and Quality upscaling when the engine renders below display resolution. Manual modes force the NGX quality level.",
        .labels = {"Auto", "DLAA", "Quality", "Balanced", "Performance", "Ultra Performance"},
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
        .tooltip = "Auto scans the TAA constant buffer for the game's render jitter and falls back to zero until it is confidently found (see ReShade.log). Zero is the safe option.",
        .labels = {"Zero", "Auto (game jitter)"},
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
        .tooltip = "Which single-channel TAA input is fed to DLSS as depth: t5 (sampled at the current pixel) or t6 (sampled at the reprojected pixel).",
        .labels = {"t5 (current UV)", "t6 (reprojected UV)"},
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
