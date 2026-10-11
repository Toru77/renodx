/*
 * Copyright (C) 2026
 * SPDX-License-Identifier: MIT
 *
 * Forza Horizon 6 Ray Reconstruction — Streamline interception.
 *
 * Requests DLSS Ray Reconstruction at slInit (restart required for the plugin
 * to load), loads it at runtime if needed, and replaces the game's DLSS SR with
 * DLSS-RR. RR receives the game's real G-buffer guides and the raw RT resolve
 * signal (the game's neighbourhood denoisers are bypassed). One switch drives
 * the whole feature; off leaves the game untouched.
 */

#define ImTextureID ImU64

#include <cstdio>

#include <deps/imgui/imgui.h>
#include <include/reshade.hpp>

#include "../../utils/date.hpp"
#include "../../utils/settings.hpp"

#include "./sl_rr.hpp"
#include "./denoise.hpp"
#include "./guides.hpp"

namespace {

// The single user control. Enabling RR applies the runtime load, the SR->RR
// redirect, the raw-signal resolve bypasses and the real guide buffers.
float rr_enable_setting = 1.f;

// DLSSD preset values for the A..F slider (ePresetA..ePresetF; F is the
// current RR 4.5 default preset). Indices match the slider labels.
constexpr uint32_t kRrPresetValues[6] = {1, 2, 3, 4, 5, 6};
float rr_preset_setting = 5.f;  // index into kRrPresetValues; F by default

// Applies the whole "RR on/off" bundle.
void ApplyRrEnabled(bool enabled) {
  sl_rr::SetRuntimeLoad(enabled);
  sl_rr::SetRrRedirect(enabled);
  denoise::SetEnabled(denoise::kGatherHash, enabled);
  denoise::SetSpatialVariant(enabled ? 2 : 0);  // 2 = bypass both
  denoise::SetEnabled(denoise::kBilateralHash, enabled);
  guides::enabled.store(enabled);
}

renodx::utils::settings::Settings settings = {
    new renodx::utils::settings::Setting{
        .key = "RrEnable",
        .binding = &rr_enable_setting,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Enable DLSS Ray Reconstruction",
        .section = "Ray Reconstruction",
        .tooltip = "Requests DLSS-RR at slInit (restart required for the plugin"
                   " to load), auto-loads it at runtime, replaces the game's"
                   " DLSS SR with RR, feeds RR the real G-buffer guides, and"
                   " hands RR the raw resolve signal (4-tap gather, spatial"
                   " filter and bilateral reconstruct bypassed). Off = the"
                   " game's own DLSS SR and denoisers run unchanged.",
        .on_change_value = [](float, float value) { ApplyRrEnabled(value != 0.f); },
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
        .value_type = renodx::utils::settings::SettingValueType::TEXT,
        .label = std::string("Build: ") + renodx::utils::date::ISO_DATE_TIME,
        .section = "About",
    },
    new renodx::utils::settings::Setting{
        .value_type = renodx::utils::settings::SettingValueType::BUTTON,
        .label = "RenoDX Discord",
        .section = "About",
        .tint = 0x5865F2,
        .on_change = []() {
          renodx::utils::platform::LaunchURL("https://discord.gg/", "Ce9bQHQrSV");
        },
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
    "DLSS Ray Reconstruction for Forza Horizon 6 via Streamline";

BOOL APIENTRY DllMain(HMODULE h_module, DWORD fdw_reason, LPVOID lpv_reserved) {
  switch (fdw_reason) {
    case DLL_PROCESS_ATTACH:
      if (!reshade::register_addon(h_module)) return FALSE;
      PinModule();
      sl_rr::InstallLoaderHooks();
      reshade::register_event<reshade::addon_event::present>(OnPresent);
      // Required for the live RT-denoise bypass (denoise.hpp): Add/Remove
      // RuntimeReplacements only take effect on the async bind-time path.
      // Must be set before the first utils::shader::Use call below.
      renodx::utils::shader::use_replace_async = true;
      break;
    case DLL_PROCESS_DETACH:
      sl_rr::UninstallHooks();
      reshade::unregister_event<reshade::addon_event::present>(OnPresent);
      break;
  }

  renodx::utils::settings::use_presets = false;
  renodx::utils::settings::Use(fdw_reason, &settings);
  denoise::Use(fdw_reason);
  guides::Use(fdw_reason);

  if (fdw_reason == DLL_PROCESS_ATTACH) {
    // Request RR at slInit unconditionally: the restart-required part cannot
    // be toggled live, and the merged switch below covers the live parts.
    sl_rr::SetInitInjection(true);
    ApplyRrEnabled(rr_enable_setting != 0.f);
    int preset_index = static_cast<int>(rr_preset_setting);
    if (preset_index < 0) preset_index = 0;
    if (preset_index > 5) preset_index = 5;
    sl_rr::SetRrPreset(kRrPresetValues[preset_index]);
  }
  return TRUE;
}
