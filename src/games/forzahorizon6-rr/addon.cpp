/*
 * Copyright (C) 2026
 * SPDX-License-Identifier: MIT
 *
 * Forza Horizon 6 Ray Reconstruction — Streamline interception.
 *
 * Requests DLSS Ray Reconstruction at slInit (restart required for the plugin
 * to load), loads it at runtime if needed, and replaces the game's DLSS SR with
 * DLSS-RR. RR receives the game's real G-buffer guides. On Medium and High
 * RTGI the game's resolve denoisers are bypassed so RR gets the raw ray signal;
 * on Low the game's own resolve feeds RR. One switch drives the whole feature.
 */

#define ImTextureID ImU64

#include <atomic>
#include <cstdio>

#include <deps/imgui/imgui.h>
#include <include/reshade.hpp>

#include "../../utils/bitwise.hpp"
#include "../../utils/date.hpp"
#include "../../utils/pipeline.hpp"
#include "../../utils/settings.hpp"

#include "./sl_rr.hpp"
#include "./denoise.hpp"
#include "./guides.hpp"
#include "./nvngx_responsivity.hpp"

namespace {

// The single user control. Enabling RR applies the runtime load, the SR->RR
// redirect and the resolve bypasses. RR runs at any RTGI tier; only the
// raw-signal bypasses require Medium or High RTGI (see below).
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

// Diagnostic logging for the RR and guide paths; off by default.
float rr_debug_log_setting = 0.f;

// DLSSD.ResponsivityMask, the one temporal-response control RR has. Negative
// favours stability (more history), positive favours responsiveness, and 0
// omits the mask entirely - RR's own default, so nothing changes until it is
// moved. This is the knob that can average the half-res fireflies out without
// clamping or filtering the signal.
float rr_responsivity_setting = 0.f;

// Speed-relative responsivity. Off = the slider above owns the value (today's
// behaviour). On = camera motion drives it: still means maximum stability, where
// the half-res fireflies are worst and history is most affordable; movement
// means responsiveness, where ghosting would otherwise appear.
float rr_responsivity_auto_setting = 0.f;

// RTGI tier detection. The bind hook arms the tier's bound flag whenever the
// game binds that tier's spatial filter; each present consumes it. Frame
// generation presents frames the game never rendered, so a single miss must
// not drop the tier: detection only falls back after kGiMissLimit consecutive
// missed presents, which still reacts quickly when the player leaves the tier.
constexpr uint32_t kGiMissLimit = 8;
struct TierDetection {
  std::atomic<bool> bound{false};
  bool active = false;
  uint32_t presents_without = 0;
};
TierDetection high_gi;
TierDetection medium_gi;
// Guards a silent detection failure: warns once no supported tier has been
// seen for long.
bool warned_no_gi = false;

// RR runs whenever the switch is on. The raw-signal resolve bypasses are a
// separate concern: they apply only on Medium and High RTGI, the tiers where RR
// is fed the raw ray signal instead of the game's filtered resolve.
bool RrActive() {
  return rr_enable_setting != 0.f;
}

// Applies the whole "RR on/off" bundle. On Medium and High RTGI the game's
// resolve denoisers are bypassed so RR receives the raw ray signal; on Low the
// game's own resolve feeds RR and the bypasses stay off. The stochastic gather
// is shared by both raw tiers.
void ApplyRrState() {
  const bool active = RrActive();
  const bool bypass_high = active && high_gi.active;
  const bool bypass_medium = active && medium_gi.active;
  if (guides::DiagGuidesEnabled()) {
    const char* tier = high_gi.active ? "High" : (medium_gi.active ? "Medium" : "none");
    guides::LogEvent(
        "RR state -> " + std::string(active ? "ON" : "OFF") + " (RTGI tier "
        + tier + ", bypasses "
        + std::string((bypass_high || bypass_medium) ? "ON" : "OFF") + ")");
  }
  // A latched failure parks RR until the game restarts. It is deliberately NOT
  // cleared by the toggle: the reporter's log shows the latch holding for two
  // minutes of clean play, then dying the instant an off>on toggle re-armed RR
  // into the evaluate that had just thrown. Retrying must be a restart, which
  // re-creates the Streamline state the failure came from.
  sl_rr::SetRuntimeLoad(active);
  sl_rr::SetRrRedirect(active);
  responsivity::enabled.store(active);
  responsivity::SetAuto(active && rr_responsivity_auto_setting != 0.f);
  if (!responsivity::GetAuto()) {
    responsivity::value.store(rr_responsivity_setting);
  }
  guides::enabled.store(active);
  denoise::SetEnabled(denoise::kSpatialHighHash, bypass_high);
  denoise::SetEnabled(denoise::kSpatialMediumHash, bypass_medium);
  denoise::SetEnabled(denoise::kReconMediumHash, bypass_medium);
  denoise::SetEnabled(denoise::kGatherHash, bypass_high || bypass_medium);
}

renodx::utils::settings::Settings settings = {
    new renodx::utils::settings::Setting{
        .key = "RrEnable",
        .binding = &rr_enable_setting,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Enable DLSS Ray Reconstruction",
        .section = "Ray Reconstruction",
        .tooltip = "Intercepts the game's DLSS SR then replaces with RR.",
        .on_change_value = [](float, float) { ApplyRrState(); },
    },
    new renodx::utils::settings::Setting{
        .key = "RrHighGiNotice",
        .value_type = renodx::utils::settings::SettingValueType::TEXT,
        .label = "Turn on Ray Traced Global Illumination at Medium or High.",
        .section = "Ray Reconstruction",
        .tint = 0xE0484C,
        .is_visible = [] { return !high_gi.active && !medium_gi.active; },
    },
    new renodx::utils::settings::Setting{
        .key = "RrVramNotice",
        .value_type = renodx::utils::settings::SettingValueType::TEXT,
        .label = "Ray Reconstruction is rendering, but DLSS is over the VRAM"
                 " budget. Expect stutter. Turning Frame Generation off frees the"
                 " most VRAM.",
        .section = "Ray Reconstruction",
        .tint = 0xE0484C,
        .is_visible = [] { return sl_rr::GetVramWarned(); },
    },
    new renodx::utils::settings::Setting{
        .key = "RrOldDllNotice",
        .value_type = renodx::utils::settings::SettingValueType::TEXT,
        .label = "Ray Reconstruction could not start because the DLSS library"
                 " next to the game is out of date. Update DLSS or your GPU"
                 " driver so nvngx_dlssd.dll is current, then restart the game."
                 " See ReShade.log for the version found.",
        .section = "Ray Reconstruction",
        .tint = 0xE0484C,
        .is_visible = [] {
          return sl_rr::GetRrTrip()
              && sl_rr::GetRrTripResult()
                     == static_cast<uint32_t>(sl::Result::eErrorExceptionHandler);
        },
    },
    new renodx::utils::settings::Setting{
        .key = "RrTrippedNotice",
        .value_type = renodx::utils::settings::SettingValueType::TEXT,
        .label = "Ray Reconstruction stopped for this session after a failed"
                 " evaluate. DLSS SR is running normally. Restart the game to"
                 " retry.",
        .section = "Ray Reconstruction",
        .tint = 0xE0484C,
        .is_visible = [] { return sl_rr::GetRrTrip(); },
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
        .key = "RRResponsivity",
        .binding = &rr_responsivity_setting,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = -0.2f,
        .label = "RR responsivity",
        .section = "Ray Reconstruction",
        .tooltip = "Preset F only. Lower more stable, higher more responsive."
                   " Ignored while speed-relative responsivity is on.",
        .min = -1.f,
        .max = 1.f,
        .format = "%.2f",
        .is_enabled = [] { return rr_responsivity_auto_setting == 0.f; },
        .on_change_value = [](float, float) { ApplyRrState(); },
    },
    new renodx::utils::settings::Setting{
        .key = "RrResponsivityAuto",
        .binding = &rr_responsivity_auto_setting,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Speed-relative responsivity",
        .section = "Ray Reconstruction",
        .tooltip = "Stable when you are not moving. Responsive at higher speeds.",
        .on_change_value = [](float, float) { ApplyRrState(); },
    },
    new renodx::utils::settings::Setting{
        .key = "RrDebugLog",
        .binding = &rr_debug_log_setting,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f,
        .label = "Debug logging",
        .section = "Debug",
        .tooltip = "Enables debug logging for sending bug reports: one line per distinct"
                   " outcome in ReShade.log (capture gates, guide HRESULTs, RR state,"
                   " redirect fallbacks). Also raises Streamline's verbosity and routes"
                   " its own log plus DLSS-RR's internal text into ReShade.log, which"
                   " is the only place that output exists at all. Both parts only"
                   " apply if this is on before the game starts, so restart the game"
                   " after turning it on. Expect a large log; turn it off and restart"
                   " for normal play.",
        .on_change_value = [](float, float value) { guides::SetDiagnostics(value != 0.f); },
    },
    new renodx::utils::settings::Setting{
        .value_type = renodx::utils::settings::SettingValueType::BULLET,
        .label = "Turn DLSS on",
        .section = "Instructions",
    },
    new renodx::utils::settings::Setting{
        .value_type = renodx::utils::settings::SettingValueType::BULLET,
        .label = "Turn Ray Traced Global Illumination at Medium or High",
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

// Arms tier detection when the game binds a tier's spatial filter. Only the
// flags are set here; RR state changes on the next present, outside the
// command-recording thread.
void OnBindPipeline(
    reshade::api::command_list* cmd_list,
    reshade::api::pipeline_stage stage,
    reshade::api::pipeline pipeline) {
  (void)cmd_list;
  if (!renodx::utils::bitwise::HasFlag(stage, reshade::api::pipeline_stage::compute_shader)) return;
  renodx::utils::pipeline::GetPipelineInfo(pipeline, [](const renodx::utils::pipeline::PipelineInfo& info) {
    for (const auto& shader : info.shader_details) {
      if (shader.shader_hash == denoise::kSpatialHighHash) {
        high_gi.bound.store(true);
      }
      if (shader.shader_hash == denoise::kSpatialMediumHash) {
        medium_gi.bound.store(true);
      }
    }
  });
}

// Consumes one present for a tier: a bind this frame keeps it alive, otherwise
// it drops after kGiMissLimit consecutive missed presents. The tiers track
// independently, so a tier switch flips through each tracker's own miss streak.
void PollTier(TierDetection& tier, const char* name, const char* hash) {
  if (tier.bound.exchange(false)) {
    tier.presents_without = 0;
    if (!tier.active) {
      tier.active = true;
      if (guides::DiagGuidesEnabled()) {
        guides::LogEvent(std::string(name) + " spatial filter " + hash + " detected");
      }
      ApplyRrState();
    }
  } else if (++tier.presents_without >= kGiMissLimit) {
    if (tier.active) {
      tier.active = false;
      if (guides::DiagGuidesEnabled()) {
        guides::LogEvent(std::string(name) + " no longer bound; bypasses released");
      }
      ApplyRrState();
    }
  }
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
  responsivity::Poll();
  // Camera motion for this frame drives the mask when speed-relative mode is on.
  responsivity::UpdateMotion(sl_rr::GetCameraMotion());
  // The bind hook arms the flags during this frame's dispatches. A miss is only
  // a frame-generation gap or a transition, so require a run of them before
  // letting go of a tier.
  PollTier(high_gi, "High-RTGI", "0x4DAF8A48");
  PollTier(medium_gi, "Medium-RTGI", "0x596D3E8F");
  if (!high_gi.active && !medium_gi.active
      && high_gi.presents_without >= kGiMissLimit && !warned_no_gi) {
    warned_no_gi = true;
    sl_rr::LogWarn("no Medium/High-RTGI spatial pipeline bound; resolve"
                   " bypasses stay off");
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
      responsivity::Disarm();
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
    // Apply the saved debug-logging state; on_change_value only fires on edits.
    guides::SetDiagnostics(rr_debug_log_setting != 0.f);
    int preset_index = static_cast<int>(rr_preset_setting);
    if (preset_index < 0) preset_index = 0;
    if (preset_index > 2) preset_index = 2;
    sl_rr::SetRrPreset(kRrPresetValues[preset_index]);
  }
  return TRUE;
}
