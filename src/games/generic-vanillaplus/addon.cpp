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
};

renodx::mods::shader::CustomShaders custom_shaders = {
#ifdef __ALL_CUSTOM_SHADERS
    __ALL_CUSTOM_SHADERS
#endif
};

renodx::utils::settings::Settings settings = {
    new renodx::utils::settings::Setting{
        .key = "ModEnabled",
        .binding = &shader_injection.mod_enabled,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Enabled",
        .section = "Vanilla+",
        .labels = {"Off", "On"},
    },
    new renodx::utils::settings::Setting{
        .key = "Slider1",
        .binding = &shader_injection.slider_1,
        .default_value = 50.f,
        .label = "Slider 1",
        .section = "Vanilla+",
        .tooltip = "Generic slider value passed to injected shaders.",
        .min = 0.f,
        .max = 100.f,
    },
    new renodx::utils::settings::Setting{
        .key = "Slider2",
        .binding = &shader_injection.slider_2,
        .default_value = 50.f,
        .label = "Slider 2",
        .section = "Vanilla+",
        .tooltip = "Generic slider value passed to injected shaders.",
        .min = 0.f,
        .max = 100.f,
    },
    new renodx::utils::settings::Setting{
        .key = "Slider3",
        .binding = &shader_injection.slider_3,
        .default_value = 0.f,
        .label = "Slider 3",
        .section = "Vanilla+",
        .tooltip = "Generic slider value passed to injected shaders.",
        .min = -100.f,
        .max = 100.f,
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
