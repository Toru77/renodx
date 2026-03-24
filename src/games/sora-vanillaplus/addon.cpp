/*
 * Copyright (C) 2026
 * SPDX-License-Identifier: MIT
 */

#define ImTextureID ImU64

#define DEBUG_LEVEL_0

#include <deps/imgui/imgui.h>
#include <include/reshade.hpp>

#include <embed/shaders.h>

#include "../../mods/shader.hpp"
#include "../../utils/settings.hpp"
#include "./shared.h"

namespace {

ShaderInjectData shader_injection = {
    .mod_enabled = 1.f,
    .slider_1 = 50.f,
    .slider_2 = 50.f,
    .slider_3 = 0.f,
    .volfog_haze_aa_mode = 0.f,
};

renodx::mods::shader::CustomShaders custom_shaders = {
    CustomShaderEntry(0x954D3D6D),  //volumetric fog
    CustomShaderEntry(0xFDAAF80E),  //lighting
};

renodx::utils::settings::Settings settings = {
    // NOTE: removed generic ModEnabled and Slider{1,2,3} settings per request
    new renodx::utils::settings::Setting{
        .key = "VolFogHazeAAMode",
        .binding = &shader_injection.volfog_haze_aa_mode,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Volumetric Haze AA",
        .section = "Volumetric Fog",
        .tooltip = "Mode for volumetric haze anti-aliasing: Vanilla or Improved (tricubic B-spline).",
        .labels = {"Vanilla", "Improved"},
    },
    
};

}  // namespace

extern "C" __declspec(dllexport) constexpr const char* NAME = "Generic Vanilla+";
extern "C" __declspec(dllexport) constexpr const char* DESCRIPTION =
    "Generic vanilla-plus shader injector";

BOOL APIENTRY DllMain(HMODULE h_module, DWORD fdw_reason, LPVOID lpv_reserved) {
  switch (fdw_reason) {
    case DLL_PROCESS_ATTACH:
      if (!reshade::register_addon(h_module)) return FALSE;

      renodx::utils::settings::use_presets = false;
      renodx::mods::shader::force_pipeline_cloning = true;
      renodx::mods::shader::allow_multiple_push_constants = true;
      renodx::mods::shader::expected_constant_buffer_index = 13;
      renodx::mods::shader::expected_constant_buffer_space = 0;
      break;
    case DLL_PROCESS_DETACH:
      reshade::unregister_addon(h_module);
      break;
  }

  renodx::utils::settings::Use(fdw_reason, &settings);
  renodx::mods::shader::Use(fdw_reason, custom_shaders, &shader_injection);

  return TRUE;
}
