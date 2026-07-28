/*
 * Copyright (C) 2024 Carlos Lopez
 * SPDX-License-Identifier: MIT
 */

#define ImTextureID ImU64

#define DEBUG_LEVEL_0

#include <deps/imgui/imgui.h>
#include <include/reshade.hpp>

#include <embed/shaders.h>

#include "../../mods/shader.hpp"
#include "../../mods/swapchain.hpp"
#include "../../utils/settings.hpp"
#include "./dlss.hpp"
#include "./shared.h"

namespace {

renodx::mods::shader::CustomShaders custom_shaders = {
    CustomShaderEntry(0x17DD2497),  // lutbuilder

    CustomShaderEntry(0xAE2AA79C),  // lut
    CustomShaderEntry(0xCD04D0B9),  // lut2
    CustomShaderEntry(0xDA87482C),  // lut3

    CustomShaderEntry(0x079440F7),  // ui

    CustomShaderEntry(0x71D481A2),  // composite

};

ShaderInjectData shader_injection;

renodx::utils::settings::Settings settings = {
    new renodx::utils::settings::Setting{
        .key = "ToneMapPeakNits",
        .binding = &shader_injection.peak_white_nits,
        .default_value = 1000.f,
        .can_reset = false,
        .label = "Peak Brightness",
        .section = "Tone Mapping",
        .tooltip = "Sets the value of peak white in nits",
        .min = 48.f,
        .max = 4000.f,
    },
    new renodx::utils::settings::Setting{
        .key = "ToneMapGameNits",
        .binding = &shader_injection.diffuse_white_nits,
        .default_value = 203.f,
        .label = "Game Brightness",
        .section = "Tone Mapping",
        .tooltip = "Sets the value of 100% white in nits",
        .min = 48.f,
        .max = 500.f,
    },
    new renodx::utils::settings::Setting{
        .key = "ToneMapGraphicsNits",
        .binding = &shader_injection.graphics_white_nits,
        .default_value = 203.f,
        .label = "Graphics Brightness",
        .section = "Tone Mapping",
        .tooltip = "Sets the brightness of ui in nits",
        .min = 48.f,
        .max = 500.f,
    },
    new renodx::utils::settings::Setting{
            .value_type = renodx::utils::settings::SettingValueType::TEXT,
            .label = "Make sure the HDR and Brightness are set to 0 in the HDR calibration screen and that sharpening is set to at least 1% in the accessibility settings",
            .section = "Note",
    },
    new renodx::utils::settings::Setting{
      .key = "DLSSReplacement",
      .binding = &halo_infinite::dlss::dlss_enabled,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 1.f,
      .label = "DLAA Replacement",
      .section = "Antialiasing",
      .tooltip = "Runs NVIDIA DLAA in place of Halo Infinite's TAA pass when the TAA resources can be resolved. Requires nvngx_dlss.dll next to the game executable.",
      .labels = {"Passthrough", "DLAA"},
      .is_visible = []() { return halo_infinite::dlss::IsSupported(); },
    },
    new renodx::utils::settings::Setting{
      .key = "DLSSPreset",
      .binding = &halo_infinite::dlss::dlss_render_preset,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 0.f,
      .label = "DLSS Preset",
      .section = "Antialiasing",
      .labels = {"Default", "F - CNN", "J - Transformer 1", "K - Transformer 1", "L - Transformer 2", "M - Transformer 2"},
      .is_enabled = []() { return halo_infinite::dlss::dlss_enabled != 0.f; },
      .is_visible = []() { return halo_infinite::dlss::IsSupported(); },
    },
    new renodx::utils::settings::Setting{
            .value_type = renodx::utils::settings::SettingValueType::TEXT,
            .label = "Mod by TheGreatHmmmmm, RenoDX Framework by ShortFuse.",
            .section = "About",
    },
};

void OnPresetOff() {
  renodx::utils::settings::UpdateSetting("toneMapPeakNits", 203.f);
  renodx::utils::settings::UpdateSetting("toneMapGameNits", 203.f);
}

bool initialized = false;

}  // namespace

extern "C" __declspec(dllexport) constexpr const char* NAME = "RenoDX";
extern "C" __declspec(dllexport) constexpr const char* DESCRIPTION = "RenoDX for Halo Infinite";

BOOL APIENTRY DllMain(HMODULE h_module, DWORD fdw_reason, LPVOID lpv_reserved) {
  switch (fdw_reason) {
    case DLL_PROCESS_ATTACH:
      if (!reshade::register_addon(h_module)) return FALSE;

      if (!initialized) {
        renodx::mods::shader::revert_constant_buffer_ranges = true;
        renodx::mods::shader::allow_multiple_push_constants = true;

        renodx::mods::shader::expected_constant_buffer_space = 50;
        renodx::mods::shader::expected_constant_buffer_index = 11;
        renodx::mods::shader::force_pipeline_cloning = true;

        renodx::mods::swapchain::swap_chain_upgrade_targets.push_back({
          .old_format = reshade::api::format::r11g11b10_float,
          .new_format = reshade::api::format::r16g16b16a16_float,
          .aspect_ratio = renodx::mods::swapchain::SwapChainUpgradeTarget::BACK_BUFFER,
          .usage_include = reshade::api::resource_usage::render_target,
        });

        initialized = true;
      }

      break;
    case DLL_PROCESS_DETACH:
      reshade::unregister_addon(h_module);
      break;
  }

  renodx::utils::settings::Use(fdw_reason, &settings, &OnPresetOff);
  halo_infinite::dlss::Use(fdw_reason);
  renodx::mods::shader::Use(fdw_reason, custom_shaders, &shader_injection);

  return TRUE;
}