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

// gpu_fence uses NTSTATUS definitions that must precede Windows headers.
#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <Windows.h>
#undef WIN32_NO_STATUS

#include <deps/imgui/imgui.h>
#include <include/reshade.hpp>

#include <embed/shaders.h>

#include "../../utils/settings.hpp"
#include "./dlss.hpp"

namespace {

void SyncFrameGenerationSetting() {
  const uint32_t requested = static_cast<uint32_t>((std::clamp)(farcry6_dlss::dlss::fg_multiplier_setting, 0.f, 5.f));
  farcry6_dlss::dlss::fg_requested_generated.store(requested, std::memory_order_release);
  farcry6_dlss::dlss::fg_enable_requested.store(requested != 0u, std::memory_order_release);
}

renodx::utils::settings::Settings settings = {
    new renodx::utils::settings::Setting{
        .key = "FrameGeneration",
        .binding = &farcry6_dlss::dlss::fg_multiplier_setting,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f,
        .label = "Frame Generation",
        .section = "Antialiasing",
        .tooltip = "Choose a frame multiplier. The supported maximum depends on your GPU and driver; higher selections use the maximum supported. Requires DLSS and SDR, HDR10, or scRGB output.",
        .labels = {"Off", "2x", "3x", "4x", "5x", "6x"},
        .on_change_value = [](float, float) { SyncFrameGenerationSetting(); },
        .is_visible = []() { return farcry6_dlss::dlss::IsSupported(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DLSSEnabled",
        .binding = &farcry6_dlss::dlss::dlss_enabled,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "DLSS Super Resolution",
        .section = "Antialiasing",
        .tooltip = "Replaces the game's TAA with DLSS when its inputs are available.",
        .is_visible = []() { return farcry6_dlss::dlss::IsSupported(); },
    },
    new renodx::utils::settings::Setting{
        .key = "RCASStrength",
        .binding = &farcry6_dlss::dlss::dlss_rcas_strength,
        .default_value = 0.5f,
        .label = "RCAS Sharpening",
        .section = "Antialiasing",
        .tooltip = "AMD RCAS after upscaling, with noise reduction enabled. Zero disables sharpening.",
        .min = 0.f,
        .max = 1.f,
        .format = "%.2f",
        .is_enabled = []() { return farcry6_dlss::dlss::dlss_enabled != 0.f; },
        .is_visible = []() { return farcry6_dlss::dlss::IsSupported(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DLSSPreset",
        .binding = &farcry6_dlss::dlss::dlss_render_preset,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 3.f,
        .label = "DLSS Preset",
        .section = "Antialiasing",
        .labels = {"Default", "F - CNN", "J - Transformer 1", "K - Transformer 1", "L - Transformer 2", "M - Transformer 2"},
        .is_enabled = []() { return farcry6_dlss::dlss::dlss_enabled != 0.f; },
        .is_visible = []() { return farcry6_dlss::dlss::IsSupported() && farcry6_dlss::dlss::upscaler_type < 0.5f; },
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

  renodx::utils::settings::Use(fdw_reason, &settings, &OnPresetOff, &SyncFrameGenerationSetting);
  farcry6_dlss::dlss::Use(fdw_reason);

  return TRUE;
}
