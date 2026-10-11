/*
 * Copyright (C) 2026
 * SPDX-License-Identifier: MIT
 *
 * Forza Horizon 6 Ray Reconstruction — Streamline interception.
 *
 * Requests DLSS Ray Reconstruction at slInit (restart required for the plugin
 * to load), loads it at runtime if needed, and replaces the game's DLSS SR with
 * DLSS-RR. RR receives the game's real G-buffer guides. On High RTGI the game's
 * temporal and gather denoisers are bypassed so RR gets the raw ray signal.
 * RR requires High RTGI: without it the switch behaves as off. One switch
 * drives the whole feature.
 */

#define ImTextureID ImU64

#include <atomic>
#include <cstdio>

#include <deps/imgui/imgui.h>
#include <include/reshade.hpp>

#include "../../utils/bitwise.hpp"
#include "../../utils/date.hpp"
#include "../../utils/settings.hpp"

#include "./sl_rr.hpp"
#include "./denoise.hpp"
#include "./guides.hpp"

namespace {

// The single user control. Enabling RR applies the runtime load, the SR->RR
// redirect and the resolve bypasses. RR runs at any RTGI tier; only the
// raw-signal bypasses require High RTGI (see below).
float rr_enable_setting = 1.f;

// DLSSD preset values for the D/E/F slider. Streamline marks A-C deprecated
// and documents F onwards as "reverts to default", so only D (default
// transformer), E (latest transformer) and F (RR 4.5 default) are offered.
constexpr uint32_t kRrPresetValues[3] = {
    static_cast<uint32_t>(sl::DLSSDPreset::ePresetD),
    static_cast<uint32_t>(sl::DLSSDPreset::ePresetE),
    static_cast<uint32_t>(sl::DLSSDPreset::ePresetF),
};
float rr_preset_setting = 2.f;  // index into kRrPresetValues; F by default

// High RTGI detection. The bind hook arms high_gi_bound whenever the game binds
// the High-tier spatial filter; each present consumes it. Frame generation
// presents frames the game never rendered, so a single miss must not drop RR:
// detection only falls back after kHighGiMissLimit consecutive missed presents,
// which still reacts quickly when the player leaves High RTGI.
constexpr uint32_t kHighGiMissLimit = 8;
std::atomic<bool> high_gi_bound{false};
bool high_gi_active = false;
uint32_t presents_without_high = 0;
// Guards a silent detection failure: warns once High RTGI has been absent long.
bool warned_no_high = false;

// RR runs whenever the switch is on. The raw-signal resolve bypasses are a
// separate concern: they apply only on High RTGI, because that is the only tier
// where RR is fed the raw ray signal instead of the game's filtered resolve.
bool RrActive() {
  return rr_enable_setting != 0.f;
}

// Applies the whole "RR on/off" bundle. On High RTGI the spatial filter (world
// cache, history and neighbourhood) and the gather (reprojection) are bypassed,
// so RR receives the raw ray signal; on Medium and Low the game's own resolve
// feeds RR and the bypasses stay off.
void ApplyRrState() {
  const bool active = RrActive();
  sl_rr::SetRuntimeLoad(active);
  sl_rr::SetRrRedirect(active);
  guides::enabled.store(active);
  const bool bypass = active && high_gi_active;
  denoise::SetEnabled(denoise::kSpatialHighHash, bypass);
  denoise::SetEnabled(denoise::kGatherHash, bypass);
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
                   " DLSS SR with RR and feeds RR the real G-buffer guides. On"
                   " High RTGI the temporal and gather denoisers are bypassed so"
                   " RR gets the raw ray signal; on Medium and Low the game's"
                   " own resolve feeds RR instead. Off = the game's own DLSS SR"
                   " and denoisers run unchanged.",
        .on_change_value = [](float, float) { ApplyRrState(); },
    },
    new renodx::utils::settings::Setting{
        .key = "RrHighGiNotice",
        .value_type = renodx::utils::settings::SettingValueType::TEXT,
        .label = "Turn on Ray Traced Global Illumination at High.",
        .section = "Ray Reconstruction",
        .tint = 0xE0484C,
        .is_visible = [] { return !high_gi_active; },
    },
    new renodx::utils::settings::Setting{
        .key = "RrPreset",
        .binding = &rr_preset_setting,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 2.f,
        .label = "RR preset",
        .section = "Ray Reconstruction",
        .tooltip = "DLSS Ray Reconstruction render preset, applied to every mode. D is the"
                   " default transformer model, E the latest transformer model, and F the"
                   " current RR 4.5 default. Changes apply on the next frame.",
        .labels = {"D", "E", "F"},
        .on_change_value = [](float, float value) {
          sl_rr::SetRrPreset(kRrPresetValues[static_cast<int>(value)]);
        },
    },
    new renodx::utils::settings::Setting{
        .value_type = renodx::utils::settings::SettingValueType::BULLET,
        .label = "Turn DLSS on",
        .section = "Instructions",
    },
    new renodx::utils::settings::Setting{
        .value_type = renodx::utils::settings::SettingValueType::BULLET,
        .label = "Turn Ray Traced Global Illumination at High",
        .section = "Instructions",
    },
    new renodx::utils::settings::Setting{
        .value_type = renodx::utils::settings::SettingValueType::TEXT,
        .label = std::string("Build: ") + renodx::utils::date::ISO_DATE_TIME,
        .section = "About",
    },
    new renodx::utils::settings::Setting{
        .value_type = renodx::utils::settings::SettingValueType::TEXT,
        .label = "Addon built by Toru",
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
    new renodx::utils::settings::Setting{
        .value_type = renodx::utils::settings::SettingValueType::BUTTON,
        .label = "Patreon",
        .section = "About",
        .on_change = []() {
          renodx::utils::platform::LaunchURL("https://www.patreon.com/c/Toru77");
        },
    },
};

// Arms High-RTGI detection when the game binds the High-tier spatial filter.
// Only the flag is set here; RR state changes on the next present, outside the
// command-recording thread.
void OnBindPipeline(
    reshade::api::command_list* cmd_list,
    reshade::api::pipeline_stage stage,
    reshade::api::pipeline pipeline) {
  (void)cmd_list;
  if (!renodx::utils::bitwise::HasFlag(stage, reshade::api::pipeline_stage::compute_shader)) return;
  renodx::utils::shader::GetPipelineShaderDetails(pipeline, [](const auto& details) {
    if (details.shader_hashes.count(denoise::kSpatialHighHash) != 0) {
      high_gi_bound.store(true);
    }
  });
}

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
  // The bind hook arms the flag during this frame's dispatches. A miss is only
  // a frame-generation gap or a transition, so require a run of them before
  // letting go of RR.
  if (high_gi_bound.exchange(false)) {
    presents_without_high = 0;
    if (!high_gi_active) {
      high_gi_active = true;
      ApplyRrState();
    }
  } else if (++presents_without_high >= kHighGiMissLimit) {
    if (high_gi_active) {
      high_gi_active = false;
      ApplyRrState();
    }
    if (!warned_no_high) {
      warned_no_high = true;
      sl_rr::LogWarn("no High-RTGI spatial pipeline bound; DLSS Ray"
                     " Reconstruction stays inactive");
    }
  }
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
      reshade::register_event<reshade::addon_event::bind_pipeline>(OnBindPipeline);
      // Required for the live RT-denoise bypass (denoise.hpp): Add/Remove
      // RuntimeReplacements only take effect on the async bind-time path.
      // Must be set before the first utils::shader::Use call below.
      renodx::utils::shader::use_replace_async = true;
      break;
    case DLL_PROCESS_DETACH:
      sl_rr::UninstallHooks();
      reshade::unregister_event<reshade::addon_event::present>(OnPresent);
      reshade::unregister_event<reshade::addon_event::bind_pipeline>(OnBindPipeline);
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
    // RR loads immediately; the resolve bypasses wait for High RTGI detection.
    ApplyRrState();
    int preset_index = static_cast<int>(rr_preset_setting);
    if (preset_index < 0) preset_index = 0;
    if (preset_index > 2) preset_index = 2;
    sl_rr::SetRrPreset(kRrPresetValues[preset_index]);
  }
  return TRUE;
}
