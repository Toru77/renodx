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
#include "./kai-vanillaplıs.h"

namespace {

renodx::mods::shader::CustomShaders custom_shaders = {
    CustomShaderEntry(0x430ED091),                 // lighting
    CustomShaderEntry(0x445A1838),                 // character lighting
    CustomShaderEntry(0x209125C1),                 // SSR
    CustomShaderEntry(0xB1CCBCAE),                 // glass
    CustomShaderEntry(0xE1E0ACBB),                 // glass
    CustomShaderEntry(0xF237E72F),                 // glass
};

SssInjectData shader_injection = {
    .sss_enabled = 1.f,
    .sss_sample_count = 8.f,
    .sss_hard_shadow_samples = 2.f,
    .sss_fade_out_samples = 4.f,
    .sss_surface_thickness = 0.008f,
    .sss_shadow_contrast = 5.f,
    .sss_light_screen_fade_start = 0.f,
    .sss_light_screen_fade_end = 1.f,
    .sss_min_occluder_depth_scale = 1.5f,
    .sss_jitter_enabled = 1.f,
    .char_shadow_mode = 2.f,
    .char_shadow_sample_count = 32.f,
    .char_shadow_hard_shadow_samples = 4.f,
    .char_shadow_fade_out_samples = 16.f,
    .char_shadow_surface_thickness = 0.09f,
    .char_shadow_contrast = 9.f,
    .char_shadow_light_screen_fade_start = 0.f,
    .char_shadow_light_screen_fade_end = 0.f,
    .char_shadow_min_occluder_depth_scale = 0.f,
    .char_shadow_jitter_enabled = 1.f,
    .char_shadow_strength = 1.f,
    .shadow_pcss_jitter_enabled = 1.f,
    .shadow_pcss_sample_mode = 1.f,
    .ssgi_mod_enabled = 1.f,
    .ssgi_color_boost = 1.f,
    .ssgi_alpha_boost = 1.f,
    .ssgi_pow = 1.f,
    .ssr_ray_count_mode = 1.f,
    .padding = 0.f,
};

renodx::utils::settings::Settings settings = {
    new renodx::utils::settings::Setting{
        .key = "SSSEnable",
        .binding = &shader_injection.sss_enabled,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Enable",
        .section = "Screen Space Shadows",
        .labels = {"Off", "On"},
    },
    new renodx::utils::settings::Setting{
        .key = "SSSSampleCount",
        .binding = &shader_injection.sss_sample_count,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 8.f,
        .label = "Sample Count",
        .section = "Screen Space Shadows",
        .tooltip = "Higher values extend shadow reach at higher GPU cost.",
        .min = 1.f,
        .max = 64.f,
        .format = "%d",
    },
    new renodx::utils::settings::Setting{
        .key = "SSSJitter",
        .binding = &shader_injection.sss_jitter_enabled,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Use Jitter",
        .section = "Screen Space Shadows",
        .labels = {"Off", "On"},
    },
    new renodx::utils::settings::Setting{
        .key = "SSSHardShadowSamples",
        .binding = &shader_injection.sss_hard_shadow_samples,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 2.f,
        .label = "Hard Samples",
        .section = "Screen Space Shadows",
        .tooltip = "Near-contact samples that keep stronger hard shadowing.",
        .min = 0.f,
        .max = 64.f,
        .format = "%d",
    },
    new renodx::utils::settings::Setting{
        .key = "SSSFadeOutSamples",
        .binding = &shader_injection.sss_fade_out_samples,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 4.f,
        .label = "Fade Samples",
        .section = "Screen Space Shadows",
        .tooltip = "Tail samples used to soften the far-end cutoff.",
        .min = 0.f,
        .max = 64.f,
        .format = "%d",
    },
    new renodx::utils::settings::Setting{
        .key = "SSSSurfaceThickness",
        .binding = &shader_injection.sss_surface_thickness,
        .default_value = 0.008f,
        .label = "Surface Thickness",
        .section = "Screen Space Shadows",
        .tooltip = "Depth thickness assumption for occluder matching.",
        .min = 0.001f,
        .max = 0.2f,
        .format = "%.4f",
    },
    new renodx::utils::settings::Setting{
        .key = "SSSShadowContrast",
        .binding = &shader_injection.sss_shadow_contrast,
        .default_value = 5.f,
        .label = "Shadow Contrast",
        .section = "Screen Space Shadows",
        .tooltip = "Higher values darken/crisp transitions.",
        .min = 0.f,
        .max = 12.f,
        .format = "%.2f",
    },
    new renodx::utils::settings::Setting{
        .key = "SSSLightFadeStart",
        .binding = &shader_injection.sss_light_screen_fade_start,
        .default_value = 0.f,
        .label = "Light Fade Start",
        .section = "Screen Space Shadows",
        .tooltip = "Minimum projected light length before SSS ramps in.",
        .min = 0.f,
        .max = 1.f,
        .format = "%.2f",
    },
    new renodx::utils::settings::Setting{
        .key = "SSSLightFadeEnd",
        .binding = &shader_injection.sss_light_screen_fade_end,
        .default_value = 1.f,
        .label = "Light Fade End",
        .section = "Screen Space Shadows",
        .tooltip = "Projected light length where SSS reaches full strength.",
        .min = 0.f,
        .max = 1.f,
        .format = "%.2f",
    },
    new renodx::utils::settings::Setting{
        .key = "SSSMinOccluderDepthScale",
        .binding = &shader_injection.sss_min_occluder_depth_scale,
        .default_value = 1.5f,
        .label = "Occluder Depth Scale",
        .section = "Screen Space Shadows",
        .tooltip = "Rejects tiny depth deltas to reduce self-shadowing noise.",
        .min = 0.f,
        .max = 4.f,
        .format = "%.2f",
    },
    new renodx::utils::settings::Setting{
        .key = "ShadowPCSSJitter",
        .binding = &shader_injection.shadow_pcss_jitter_enabled,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Shadow Jitter",
        .section = "Shadows",
        .labels = {"Off", "On"},
    },
    new renodx::utils::settings::Setting{
        .key = "SSGIEnable",
        .binding = &shader_injection.ssgi_mod_enabled,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Enable",
        .section = "SSGI",
        .labels = {"Off", "On"},
    },
    new renodx::utils::settings::Setting{
        .key = "SSGIColorBoost",
        .binding = &shader_injection.ssgi_color_boost,
        .default_value = 1.f,
        .label = "Color Boost",
        .section = "SSGI",
        .tooltip = "Scales SSGI RGB contribution before power shaping.",
        .min = 0.f,
        .max = 3.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.ssgi_mod_enabled >= 0.5f; },
    },
    new renodx::utils::settings::Setting{
        .key = "SSGIAlphaBoost",
        .binding = &shader_injection.ssgi_alpha_boost,
        .default_value = 1.f,
        .label = "Alpha Boost",
        .section = "SSGI",
        .tooltip = "Scales SSGI alpha before saturate.",
        .min = 0.f,
        .max = 3.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.ssgi_mod_enabled >= 0.5f; },
    },
    new renodx::utils::settings::Setting{
        .key = "SSGIPower",
        .binding = &shader_injection.ssgi_pow,
        .default_value = 1.f,
        .label = "Power",
        .section = "SSGI",
        .tooltip = "Applies pow(abs(color), Power) to shape bounce response.",
        .min = 0.1f,
        .max = 3.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.ssgi_mod_enabled >= 0.5f; },
    },
    new renodx::utils::settings::Setting{
        .key = "SSRRayCountMode",
        .binding = &shader_injection.ssr_ray_count_mode,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 1.f,
        .label = "Mode",
        .section = "SSR Ray Count",
        .labels = {"Vanilla", "2x"},
    },
    new renodx::utils::settings::Setting{
        .key = "CharShadowMode",
        .binding = &shader_injection.char_shadow_mode,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 2.f,
        .label = "Mode",
        .section = "Character Shadowing",
        .labels = {"Off", "Vanilla", "Bend_SSS"},
    },
    new renodx::utils::settings::Setting{
        .key = "CharShadowStrength",
        .binding = &shader_injection.char_shadow_strength,
        .default_value = 100.f,
        .label = "Strength",
        .section = "Character Shadowing",
        .tooltip = "Blend strength for character sun screen-space shadowing.",
        .min = 0.f,
        .max = 100.f,
        .is_enabled = []() { return shader_injection.char_shadow_mode >= 1.f; },
        .parse = [](float value) { return value * 0.01f; },
    },
    new renodx::utils::settings::Setting{
        .key = "CharShadowSampleCount",
        .binding = &shader_injection.char_shadow_sample_count,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 32.f,
        .label = "Sample Count",
        .section = "Character Shadowing",
        .tooltip = "Higher values increase shadow reach/quality at higher cost.",
        .min = 1.f,
        .max = 64.f,
        .format = "%d",
        .is_enabled = []() { return shader_injection.char_shadow_mode == 2.f; },
    },
    new renodx::utils::settings::Setting{
        .key = "CharShadowHardSamples",
        .binding = &shader_injection.char_shadow_hard_shadow_samples,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 4.f,
        .label = "Hard Samples",
        .section = "Character Shadowing",
        .tooltip = "Near-contact samples that preserve harder shadows.",
        .min = 0.f,
        .max = 64.f,
        .format = "%d",
        .is_enabled = []() { return shader_injection.char_shadow_mode == 2.f; },
    },
    new renodx::utils::settings::Setting{
        .key = "CharShadowFadeSamples",
        .binding = &shader_injection.char_shadow_fade_out_samples,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 16.f,
        .label = "Fade Samples",
        .section = "Character Shadowing",
        .tooltip = "Tail samples used to soften the far shadow cutoff.",
        .min = 0.f,
        .max = 64.f,
        .format = "%d",
        .is_enabled = []() { return shader_injection.char_shadow_mode == 2.f; },
    },
    new renodx::utils::settings::Setting{
        .key = "CharShadowSurfaceThickness",
        .binding = &shader_injection.char_shadow_surface_thickness,
        .default_value = 0.09f,
        .label = "Surface Thickness",
        .section = "Character Shadowing",
        .tooltip = "Depth thickness assumption for occluder matching.",
        .min = 0.001f,
        .max = 0.2f,
        .format = "%.4f",
        .is_enabled = []() { return shader_injection.char_shadow_mode == 2.f; },
    },
    new renodx::utils::settings::Setting{
        .key = "CharShadowContrast",
        .binding = &shader_injection.char_shadow_contrast,
        .default_value = 9.f,
        .label = "Shadow Contrast",
        .section = "Character Shadowing",
        .tooltip = "Higher values darken/crisp the character shadow transition.",
        .min = 0.f,
        .max = 12.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.char_shadow_mode == 2.f; },
    },
    new renodx::utils::settings::Setting{
        .key = "CharShadowLightFadeStart",
        .binding = &shader_injection.char_shadow_light_screen_fade_start,
        .default_value = 0.f,
        .label = "Light Fade Start",
        .section = "Character Shadowing",
        .tooltip = "Minimum projected sun length before shadows ramp in.",
        .min = 0.f,
        .max = 1.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.char_shadow_mode == 2.f; },
    },
    new renodx::utils::settings::Setting{
        .key = "CharShadowLightFadeEnd",
        .binding = &shader_injection.char_shadow_light_screen_fade_end,
        .default_value = 0.f,
        .label = "Light Fade End",
        .section = "Character Shadowing",
        .tooltip = "Projected sun length where shadows reach full strength.",
        .min = 0.f,
        .max = 1.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.char_shadow_mode == 2.f; },
    },
    new renodx::utils::settings::Setting{
        .key = "CharShadowMinOccluderDepthScale",
        .binding = &shader_injection.char_shadow_min_occluder_depth_scale,
        .default_value = 0.f,
        .label = "Occluder Depth Scale",
        .section = "Character Shadowing",
        .tooltip = "Rejects tiny depth deltas to reduce self-shadowing noise.",
        .min = 0.f,
        .max = 4.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.char_shadow_mode == 2.f; },
    },
    new renodx::utils::settings::Setting{
        .key = "CharShadowJitter",
        .binding = &shader_injection.char_shadow_jitter_enabled,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Use Jitter",
        .section = "Character Shadowing",
        .labels = {"Off", "On"},
        .is_enabled = []() { return shader_injection.char_shadow_mode == 2.f; },
    },
};

}  // namespace

extern "C" __declspec(dllexport) constexpr const char* NAME = "Kai Vanilla+";
extern "C" __declspec(dllexport) constexpr const char* DESCRIPTION = "Kai Vanilla+";

BOOL APIENTRY DllMain(HMODULE h_module, DWORD fdw_reason, LPVOID lpv_reserved) {
  switch (fdw_reason) {
    case DLL_PROCESS_ATTACH:
      if (!reshade::register_addon(h_module)) return FALSE;

      renodx::utils::settings::overlay_title = "Kai Vanilla+";
      renodx::utils::settings::global_name = "kai-vanilla-plus";
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
