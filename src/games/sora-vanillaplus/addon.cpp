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
  // Character Shadowing
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
  .char_shadow_type = 1.f, // 0=Camera,1=World,2=Combined (default World)
  .char_shadow_camera_strength = 1.f,
  .char_shadow_world_strength = 1.f,
  // Screen Space Shadows
  .foliage_sss_enabled = 1.f,
  .foliage_sss_strength = 1.0f,
  .foliage_sss_sample_count = 24.f,
  .foliage_sss_surface_thickness = 0.005f,
  .foliage_sss_contrast = 2.f,
  .foliage_sss_jitter_enabled = 1.f,
  .foliage_sss_height_enabled = 1.f,
  .foliage_sss_height_min = 0.f,
  .foliage_sss_height_max = 1.f,
  .foliage_sss_height_fade = 0.1f,
  .foliage_sss_vertical_reject = 0.3f,
  .foliage_sss_max_darkening = 0.40f,
  .foliage_sss_bright_reject_threshold = 0.19f,
  .foliage_sss_bright_reject_fade = 0.5f,
  .debug_show_env_sss = 0.f,
};

renodx::mods::shader::CustomShaders custom_shaders = {
    CustomShaderEntry(0x954D3D6D),  //volumetric fog
    CustomShaderEntry(0x485E0022),  //ssao + char shadow
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
    // Character Shadowing
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
      .key = "CharShadowType",
      .binding = &shader_injection.char_shadow_type,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 1.f,
      .label = "Shadow Type",
      .section = "Character Shadowing",
      .labels = {"Camera View", "World View", "Combined"},
      .is_enabled = []() { return shader_injection.char_shadow_mode == 2.f; },
    },
    new renodx::utils::settings::Setting{
      .key = "CharShadowCameraStrength",
      .binding = &shader_injection.char_shadow_camera_strength,
      .default_value = 100.f,
      .label = "Camera Strenght",
      .section = "Character Shadowing",
      .tooltip = "Strength for camera-view character SSS pass.",
      .min = 0.f,
      .max = 100.f,
      .is_enabled = []() {
        return shader_injection.char_shadow_mode == 2.f &&
               shader_injection.char_shadow_type != 1.f;
      },
      .parse = [](float value) { return value * 0.01f; },
    },
    new renodx::utils::settings::Setting{
      .key = "CharShadowWorldStrength",
      .binding = &shader_injection.char_shadow_world_strength,
      .default_value = 100.f,
      .label = "World Strenght",
      .section = "Character Shadowing",
      .tooltip = "Strength for world-view character SSS pass.",
      .min = 0.f,
      .max = 100.f,
      .is_enabled = []() {
        return shader_injection.char_shadow_mode == 2.f &&
               shader_injection.char_shadow_type != 0.f;
      },
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
    // Screen Space Shadows
    new renodx::utils::settings::Setting{
      .key = "FoliageSSSEnabled",
      .binding = &shader_injection.foliage_sss_enabled,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 1.f,
      .label = "Bend SSS",
      .section = "Screen Space Shadows",
      .tooltip = "Screen-space shadowing effect (Bend Studio algorithm).",
      .labels = {"Off", "On"},
    },
    new renodx::utils::settings::Setting{
      .key = "FoliageSSSStrength",
      .binding = &shader_injection.foliage_sss_strength,
      .default_value = 100.f,
      .label = "Strength",
      .section = "Screen Space Shadows",
      .tooltip = "Blend strength for screen-space shadows.",
      .min = 0.f,
      .max = 100.f,
      .is_enabled = []() { return shader_injection.foliage_sss_enabled >= 0.5f; },
      .parse = [](float value) { return value * 0.01f; },
    },
    new renodx::utils::settings::Setting{
      .key = "FoliageSSSSampleCount",
      .binding = &shader_injection.foliage_sss_sample_count,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 24.f,
      .label = "Sample Count",
      .section = "Screen Space Shadows",
      .tooltip = "Higher values increase shadow reach/quality at higher cost.",
      .min = 1.f,
      .max = 64.f,
      .format = "%d",
      .is_enabled = []() { return shader_injection.foliage_sss_enabled >= 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "FoliageSSSSurfaceThickness",
      .binding = &shader_injection.foliage_sss_surface_thickness,
      .default_value = 0.005f,
      .label = "Surface Thickness",
      .section = "Screen Space Shadows",
      .tooltip = "Depth thickness assumption for occluder matching.",
      .min = 0.001f,
      .max = 0.2f,
      .format = "%.4f",
      .is_enabled = []() { return shader_injection.foliage_sss_enabled >= 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "FoliageSSSContrast",
      .binding = &shader_injection.foliage_sss_contrast,
      .default_value = 2.f,
      .label = "Shadow Contrast",
      .section = "Screen Space Shadows",
      .tooltip = "Higher values darken/crisp the shadow transition.",
      .min = 0.f,
      .max = 12.f,
      .format = "%.2f",
      .is_enabled = []() { return shader_injection.foliage_sss_enabled >= 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "FoliageSSSHeightEnable",
      .binding = &shader_injection.foliage_sss_height_enabled,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 1.f,
      .label = "Height Above Ground",
      .section = "Screen Space Shadows",
      .tooltip = "Only apply SSS to pixels above a certain height from the ground surface below them.",
      .labels = {"Off", "On"},
      .is_enabled = []() { return shader_injection.foliage_sss_enabled >= 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "FoliageSSSHeightMin",
      .binding = &shader_injection.foliage_sss_height_min,
      .default_value = 0.f,
      .label = "Min Height",
      .section = "Screen Space Shadows",
      .tooltip = "Minimum height above the ground before SSS starts.",
      .min = 0.f,
      .max = 10.f,
      .format = "%.2f",
      .is_enabled = []() {
        return shader_injection.foliage_sss_enabled >= 0.5f &&
           shader_injection.foliage_sss_height_enabled >= 0.5f;
      },
    },
    new renodx::utils::settings::Setting{
      .key = "FoliageSSSHeightMax",
      .binding = &shader_injection.foliage_sss_height_max,
      .default_value = 1.f,
      .label = "Ground Search",
      .section = "Screen Space Shadows",
      .tooltip = "How many pixels downward on-screen to search for the ground surface.",
      .min = 1.f,
      .max = 200.f,
      .format = "%.0f",
      .is_enabled = []() {
        return shader_injection.foliage_sss_enabled >= 0.5f &&
           shader_injection.foliage_sss_height_enabled >= 0.5f;
      },
    },
    new renodx::utils::settings::Setting{
      .key = "FoliageSSSHeightFade",
      .binding = &shader_injection.foliage_sss_height_fade,
      .default_value = 0.10f,
      .label = "Height Fade",
      .section = "Screen Space Shadows",
      .tooltip = "Smooth transition range above the min height threshold.",
      .min = 0.f,
      .max = 5.f,
      .format = "%.2f",
      .is_enabled = []() {
        return shader_injection.foliage_sss_enabled >= 0.5f &&
           shader_injection.foliage_sss_height_enabled >= 0.5f;
      },
    },
    new renodx::utils::settings::Setting{
      .key = "FoliageSSSVerticalReject",
      .binding = &shader_injection.foliage_sss_vertical_reject,
      .default_value = 0.30f,
      .label = "Vertical Reject",
      .section = "Screen Space Shadows",
      .tooltip = "Rejects vertical surfaces (walls, pillars). 0 = off, higher = stricter.",
      .min = 0.f,
      .max = 1.f,
      .format = "%.2f",
      .is_enabled = []() { return shader_injection.foliage_sss_enabled >= 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "FoliageSSSMaxDarkening",
      .binding = &shader_injection.foliage_sss_max_darkening,
      .default_value = 0.40f,
      .label = "Max Darkening",
      .section = "Screen Space Shadows",
      .tooltip = "Limits how dark shadows can get.",
      .min = 0.f,
      .max = 1.f,
      .format = "%.2f",
      .is_enabled = []() { return shader_injection.foliage_sss_enabled >= 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "FoliageSSBrightRejectThreshold",
      .binding = &shader_injection.foliage_sss_bright_reject_threshold,
      .default_value = 0.19f,
      .label = "Brightness Reject",
      .section = "Screen Space Shadows",
      .tooltip = "Pixels brighter than this luminance will resist SSS darkening.",
      .min = 0.f,
      .max = 5.f,
      .format = "%.2f",
      .is_enabled = []() { return shader_injection.foliage_sss_enabled >= 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "FoliageSSBrightRejectFade",
      .binding = &shader_injection.foliage_sss_bright_reject_fade,
      .default_value = 0.5f,
      .label = "Brightness Fade",
      .section = "Screen Space Shadows",
      .tooltip = "How gradual the brightness rejection transition is.",
      .min = 0.01f,
      .max = 3.f,
      .format = "%.2f",
      .is_enabled = []() { return shader_injection.foliage_sss_enabled >= 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "DebugShowEnvSSS",
      .binding = &shader_injection.debug_show_env_sss,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 0.f,
      .label = "Show Env SSS",
      .section = "Screen Space Shadows",
      .tooltip = "Show final environment SSS on a gray background for debugging.",
      .labels = {"Off", "On"},
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
