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
#include "./shared.h"

namespace {


#define UpgradeRTVShader(value)              \
  {                                          \
      value,                                 \
      {                                      \
          .crc32 = value,                    \
          .on_draw = [](auto* cmd_list) {                                                           \
            auto rtvs = renodx::utils::swapchain::GetRenderTargets(cmd_list);                       \
            bool changed = false;                                                                   \
            for (auto rtv : rtvs) {                                                                 \
              changed = renodx::mods::swapchain::ActivateCloneHotSwap(cmd_list->get_device(), rtv); \
            }                                                                                       \
            if (changed) {                                                                          \
              renodx::mods::swapchain::FlushDescriptors(cmd_list);                                  \
              renodx::mods::swapchain::RewriteRenderTargets(cmd_list, rtvs.size(), rtvs.data(), {0});      \
            }                                                                                       \
            return true; }, \
      },                                     \
  }

#define UpgradeRTVReplaceShader(value)       \
  {                                          \
      value,                                 \
      {                                      \
          .crc32 = value,                    \
          .code = __##value,                 \
          .on_draw = [](auto* cmd_list) {                                                             \
            auto rtvs = renodx::utils::swapchain::GetRenderTargets(cmd_list);                         \
            bool changed = false;                                                                     \
            for (auto rtv : rtvs) {                                                                   \
              changed = renodx::mods::swapchain::ActivateCloneHotSwap(cmd_list->get_device(), rtv);   \
            }                                                                                         \
            if (changed) {                                                                            \
              renodx::mods::swapchain::FlushDescriptors(cmd_list);                                    \
              renodx::mods::swapchain::RewriteRenderTargets(cmd_list, rtvs.size(), rtvs.data(), {0}); \
            }                                                                                         \
            return true; }, \
      },                                     \
  }


renodx::mods::shader::CustomShaders custom_shaders = {
    

    //  Kuro
    CustomShaderEntry(0xAD51B4B0), // Kuro final
    // UpgradeRTVReplaceShader(0x28FFFB4A), // Kuro tonemap
    // UpgradeRTVReplaceShader(0x034581D3), // Kuro overlay blending
    // UpgradeRTVReplaceShader(0x83F2D19E), // blur sampler
    // UpgradeRTVReplaceShader(0x5BB549F7), // blur gen
    // UpgradeRTVReplaceShader(0xCE7C6E9D), // depth
    // UpgradeRTVReplaceShader(0x43E0BB74), // blur
    // UpgradeRTVReplaceShader(0x2D620443), // blur
    // UpgradeRTVReplaceShader(0xAF7B0499), // refraction
    // UpgradeRTVReplaceShader(0xE7562C18), // refraction

    // UpgradeRTVShader(0x1336F6F8),
    // UpgradeRTVShader(0xEF0CAEEA),
    // UpgradeRTVShader(0x7BC8A1E8),
    // CustomSwapchainShader(0x00000000),
    // BypassShaderEntry(0x00000000)
};

ShaderInjectData shader_injection;

float current_settings_mode = 0;

renodx::utils::settings::Settings settings = {
    new renodx::utils::settings::Setting{
        .key = "SettingsMode",
        .binding = &current_settings_mode,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f,
        .can_reset = false,
        .label = "Settings Mode",
        .labels = {"Simple", "Intermediate", "Advanced"},
        .is_global = true,
    },
    new renodx::utils::settings::Setting{
        .key = "gamma",
        .binding = &shader_injection.gamma,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 1.f,
        .can_reset = true,
        .label = "Gamma",
        .section = "Game Settings",
        .tooltip = "The game defaults to 2.3 Gamma.",
        .labels = {"Falcom (2.3)", "sRGB"},
        .is_visible = []() { return false; },
    },
    new renodx::utils::settings::Setting{
        .key = "bloom",
        .binding = &shader_injection.bloom,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 1.f,
        .can_reset = true,
        .label = "Bloom",
        .section = "Game Settings",
        .tooltip = "Bloom blending method.",
        .labels = {"Falcom (SDR)", "HDR"},
        .is_visible = []() { return current_settings_mode >= 1; },
    },

    // new renodx::utils::settings::Setting{
    //     .key = "fxPauseBrightness",
    //     .binding = &shader_injection.pause_brightness,
    //     .default_value = 100.f,
    //     .label = "Pause-Menu Brightness",
    //     .section = "Game Settings",
    //     .tooltip = "Controls the game brightness during the",
    //     .max = 100.f,
    //     .parse = [](float value) { return value * 0.01f; },
    //     .is_visible = []() { return current_settings_mode >= 1; },
    // },

    new renodx::utils::settings::Setting{
        .key = "fxBloom",
        .binding = &shader_injection.bloom_strength,
        .default_value = 50.f,
        .label = "Bloom Strength",
        .section = "Game Settings",
        .tooltip = "Controls Bloom Strength",
        .max = 100.f,
        .parse = [](float value) { return value * 0.01f; },
        .is_visible = []() { return false; },
    },

    new renodx::utils::settings::Setting{
        .key = "fxBloomCorrection",
        .binding = &shader_injection.bloom_hue_correction,
        .default_value = 25.f,
        .label = "Bloom Color Correction",
        .section = "Game Settings",
        .tooltip = "Correcting the colors after rewriting the Bloom.",
        .min = 0.f,
        .max = 100.f,
        .is_enabled = []() { return shader_injection.tone_map_type >= 1; },
        .parse = [](float value) { return value * 0.01f; },
        // .is_visible = []() { return false; },
        .is_visible = []() { return current_settings_mode >= 2; },
    },
    new renodx::utils::settings::Setting{
        .key = "ToneMapType",
        .binding = &shader_injection.tone_map_type,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 2.f,
        .can_reset = true,
        .label = "Tone Mapper",
        .section = "Tone Mapping",
        .tooltip = "Sets the tone mapper type",
        .labels = {"Vanilla", "Frostbite", "DICE", "RenoDRT", "RenoDRTRollOff"},
        .is_visible = []() { return current_settings_mode >= 1; },
    },
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
        .key = "ToneMapUINits",
        .binding = &shader_injection.graphics_white_nits,
        .default_value = 203.f,
        .label = "UI Brightness",
        .section = "Tone Mapping",
        .tooltip = "Sets the brightness of UI and HUD elements in nits",
        .min = 48.f,
        .max = 500.f,
    },
    new renodx::utils::settings::Setting{
        .key = "InverseToneMapExtraHDRSaturation",
        .binding = &shader_injection.inverse_tonemap_extra_hdr_saturation,
        .default_value = 0.f,
        .can_reset = false,
        .label = "Gamut Expansion",
        .section = "Tone Mapping",
        .tooltip = "Generates HDR colors (BT.2020) from bright saturated SDR (BT.709) ones. Neutral at 0.",
        .min = 0.f,
        .max = 100.f,
        .parse = [](float value) { return value * 0.01f; },
    },
    new renodx::utils::settings::Setting{
        .key = "GammaCorrection",
        .binding = &shader_injection.gamma_correction,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 3.f,
        .label = "Gamma Correction",
        .section = "Tone Mapping",
        .tooltip = "Emulates a display EOTF.",
        .labels = {"Off", "2.2", "BT.1886", "Falcom (2.3)"},
        .is_visible = []() { return current_settings_mode >= 1 && shader_injection.gamma == 1; },
    },
    // new renodx::utils::settings::Setting{
    //     .key = "InverseToneMapExtraHDRSaturation",
    //     .binding = &shader_injection.inverse_tonemap_extra_hdr_saturation,
    //     .default_value = 0.f,
    //     .can_reset = false,
    //     .label = "Gamut Expansion",
    //     .section = "Tone Mapping",
    //     .tooltip = "Generates HDR colors (BT.2020) from bright saturated SDR (BT.709) ones. Neutral at 0.",
    //     .min = 0.f,
    //     .max = 500.f,
    //     .parse = [](float value) { return value * 0.01f; },
    // },
    new renodx::utils::settings::Setting{
        .key = "ToneMapWorkingColorSpace",
        .binding = &shader_injection.tone_map_working_color_space,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f,
        .label = "Working Color Space",
        .section = "Tone Mapping",
        .labels = {"BT709", "BT2020", "AP1"},
        .is_enabled = []() { return shader_injection.tone_map_type >= 1; },
        .is_visible = []() { return current_settings_mode >= 1; },
        // .is_visible = []() { return false; },
    },
    new renodx::utils::settings::Setting{
        .key = "DICEToneMapType",
        .binding = &shader_injection.dice_tone_map_type,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 2.f,
        .can_reset = true,
        .label = "DICE ToneMap Type",
        .section = "DICE Configuration",
        .tooltip = "Sets the DICE tone mapper type",
        .labels = {"Luminance RGB", "Luminance PQ", "Luminance PQ w/ Channel Correction", "Channel PQ"},
        .is_visible = []() { return current_settings_mode >= 2 && shader_injection.tone_map_type == 2.f ; },
    },
    new renodx::utils::settings::Setting{
        .key = "DICEShoulderStart",
        .binding = &shader_injection.dice_shoulder_start,
        .default_value = 33.f,
        .label = "DICE Shoulder Start",
        .section = "DICE Configuration",
        .tooltip = "Sets the DICE shoulder start.",
        .min = 0.f,
        .max = 100.f,
        .is_enabled = []() { return shader_injection.tone_map_type >= 1; },
        .parse = [](float value) { return value * 0.01f; },
        .is_visible = []() { return current_settings_mode >= 2 && shader_injection.tone_map_type == 2.f ; },
    },
    new renodx::utils::settings::Setting{
        .key = "DICEDesaturation",
        .binding = &shader_injection.dice_desaturation,
        .default_value = 33.f,
        .label = "DICE Desaturation",
        .section = "DICE Configuration",
        .tooltip = "Sets the DICE desaturation.",
        .min = 0.f,
        .max = 100.f,
        .is_enabled = []() { return shader_injection.tone_map_type >= 1; },
        .parse = [](float value) { return value * 0.01f; },
        .is_visible = []() { return current_settings_mode >= 2 && shader_injection.tone_map_type == 2.f ; },
    },
    new renodx::utils::settings::Setting{
        .key = "DICEDarkening",
        .binding = &shader_injection.dice_darkening,
        .default_value = 33.f,
        .label = "DICE Darkening",
        .section = "DICE Configuration",
        .tooltip = "Sets the DICE darkening.",
        .min = 0.f,
        .max = 100.f,
        .is_enabled = []() { return shader_injection.tone_map_type >= 1; },
        .parse = [](float value) { return value * 0.01f; },
        .is_visible = []() { return current_settings_mode >= 2 && shader_injection.tone_map_type == 2.f ; },
    },
    
    new renodx::utils::settings::Setting{
        .key = "ToneMapScaling",
        .binding = &shader_injection.tone_map_per_channel,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f,
        .label = "Scaling",
        .section = "RenoDRT Configuration",
        .tooltip = "Luminance scales colors consistently while per-channel saturates and blows out sooner",
        .labels = {"Luminance", "Per Channel"},
        .is_enabled = []() { return shader_injection.tone_map_type >= 1; },
        .is_visible = []() { return current_settings_mode >= 2 && shader_injection.tone_map_type >= 3.f; },
    },
    // new renodx::utils::settings::Setting{
    //     .key = "ToneMapWhiteClip",
    //     .binding = &shader_injection.tone_map_white_clip,
    //     .default_value = 100.f,
    //     .label = "White Clip",
    //     .section = "RenoDRT Configuration",
    //     .tooltip = "White clip values.",
    //     .min = 0.f,
    //     .max = 100.f,
    //     .is_enabled = []() { return shader_injection.tone_map_type >= 1; },
    //     .parse = [](float value) { return value * 1.f; },
    //     .is_visible = []() { return current_settings_mode >= 1 && shader_injection.tone_map_type == 3.f; },
    // },
    
    new renodx::utils::settings::Setting{
        .key = "ToneMapHueCorrectionMethod",
        .binding = &shader_injection.tone_map_hue_correction_method,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f,
        .label = "Hue Correction Method",
        .section = "Hue Correction",
        .tooltip = "Selects tonemapping method for hue correction",
        .labels = {"Reinhard", "NeutralSDR", "DICE", "Uncharted2"},
        .is_enabled = []() { return shader_injection.tone_map_type >= 1; },
        // .is_visible = []() { return current_settings_mode >= 2; },
        .is_visible = []() { return shader_injection.tone_map_type >= 1 && current_settings_mode >= 2.f; },
        // .is_visible = []() { return false; },
    },
    new renodx::utils::settings::Setting{
        .key = "ToneMapHueProcessor",
        .binding = &shader_injection.tone_map_hue_processor,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 1.f,
        .label = "Hue Processor",
        .section = "Hue Correction",
        .tooltip = "Selects hue processor",
        .labels = {"OKLab", "ICtCp", "darkTable UCS"},
        .is_enabled = []() { return shader_injection.tone_map_type >= 1; },
        // .is_visible = []() { return current_settings_mode >= 2; },
        .is_visible = []() { return shader_injection.tone_map_type >= 1 && current_settings_mode >= 2.f; },
        // .is_visible = []() { return false; },
    },
    
    new renodx::utils::settings::Setting{
        .key = "ToneMapHueCorrection",
        .binding = &shader_injection.tone_map_hue_correction,
        .default_value = 75.f,
        .label = "Hue Correction Strength",
        .section = "Hue Correction",
        .tooltip = "Hue retention strength.",
        .min = 0.f,
        .max = 100.f,
        .is_enabled = []() { return shader_injection.tone_map_type >= 1; },
        .parse = [](float value) { return value * 0.01f; },
        // .is_visible = []() { return false; },
        .is_visible = []() { return current_settings_mode >= 2; },
    },
    new renodx::utils::settings::Setting{
        .key = "ToneMapHueShift",
        .binding = &shader_injection.tone_map_hue_shift,
        .default_value = 0.f,
        .label = "Hue Shift",
        .section = "Hue Correction",
        .tooltip = "Hue-shift emulation strength.",
        .min = 0.f,
        .max = 100.f,
        .is_enabled = []() { return shader_injection.tone_map_type >= 1; },
        .parse = [](float value) { return value * 0.01f; },
        // .is_visible = []() { return current_settings_mode >= 1; },
        .is_visible = []() { return false; },
    },

    
    // new renodx::utils::settings::Setting{
    //     .key = "ToneMapClampColorSpace",
    //     .binding = &shader_injection.tone_map_clamp_color_space,
    //     .value_type = renodx::utils::settings::SettingValueType::INTEGER,
    //     .default_value = 1.f,
    //     .label = "Clamp Color Space",
    //     .section = "Color Space",
    //     .tooltip = "Hue-shift emulation strength.",
    //     .labels = {"BT709", "BT2020"},
    //     .is_enabled = []() { return shader_injection.tone_map_type >= 1; },
    //     .is_visible = []() { return current_settings_mode >= 2; },
    // },
    // new renodx::utils::settings::Setting{
    //     .key = "ToneMapClampPeak",
    //     .binding = &shader_injection.tone_map_clamp_peak,
    //     .value_type = renodx::utils::settings::SettingValueType::INTEGER,
    //     .default_value = 0.f,
    //     .label = "Clamp Peak",
    //     .section = "Color Space",
    //     .tooltip = "Hue-shift emulation strength.",
    //     .labels = {"None", "BT709", "BT2020", "AP1"},
    //     .is_enabled = []() { return shader_injection.tone_map_type >= 1; },
    //     .parse = [](float value) { return value - 1.f; },
    //     .is_visible = []() { return current_settings_mode >= 2; },
    // },
    ///////////////////////////////////////////

    
    /////////////////////////////////////////////////////////////
    new renodx::utils::settings::Setting{
        .key = "ColorGradeExposure",
        .binding = &shader_injection.tone_map_exposure,
        .default_value = 1.f,
        .label = "Exposure",
        .section = "Color Grading",
        .max = 2.f,
        .format = "%.2f",
        .is_visible = []() { return current_settings_mode >= 1; },
    },
    new renodx::utils::settings::Setting{
        .key = "ColorGradeHighlights",
        .binding = &shader_injection.tone_map_highlights,
        .default_value = 50.f,
        .label = "Highlights",
        .section = "Color Grading",
        .max = 100.f,
        .parse = [](float value) { return value * 0.02f; },
        .is_visible = []() { return shader_injection.tone_map_type  >= 3.f; },
        
    },
    new renodx::utils::settings::Setting{
        .key = "ColorGradeShadows",
        .binding = &shader_injection.tone_map_shadows,
        .default_value = 50.f,
        .label = "Shadows",
        .section = "Color Grading",
        .max = 100.f,
        .parse = [](float value) { return value * 0.02f; },
        .is_visible = []() { return current_settings_mode >= 1 && shader_injection.tone_map_type  >= 3.f; },
    },
    new renodx::utils::settings::Setting{
        .key = "ColorGradeContrast",
        .binding = &shader_injection.tone_map_contrast,
        .default_value = 50.f,
        .label = "Contrast",
        .section = "Color Grading",
        .max = 100.f,
        
        .parse = [](float value) { return value * 0.02f; },
        .is_visible = []() { return current_settings_mode >= 1 && shader_injection.tone_map_type  >= 3.f; },
    },
    new renodx::utils::settings::Setting{
        .key = "ColorGradeSaturation",
        .binding = &shader_injection.tone_map_saturation,
        .default_value = 50.f,
        .label = "Saturation",
        .section = "Color Grading",
        .max = 100.f,
        .parse = [](float value) { return value * 0.02f; },
    },
    new renodx::utils::settings::Setting{
        .key = "ColorGradeHighlightSaturation",
        .binding = &shader_injection.tone_map_highlight_saturation,
        .default_value = 50.f,
        .label = "Highlight Saturation",
        .section = "Color Grading",
        .tooltip = "Adds or removes highlight color.",
        .max = 100.f,
        .is_enabled = []() { return shader_injection.tone_map_type >= 1; },
        .parse = [](float value) { return value * 0.02f; },
        .is_visible = []() { return current_settings_mode >= 1 && shader_injection.tone_map_type  >= 3.f; },
    },
    new renodx::utils::settings::Setting{
        .key = "ColorGradeBlowout",
        .binding = &shader_injection.tone_map_blowout,
        .default_value = 0.f,
        .label = "Blowout",
        .section = "Color Grading",
        .tooltip = "Controls highlight desaturation due to overexposure.",
        .max = 100.f,
        .parse = [](float value) { return value * 0.01f; },
        .is_visible = []() { return current_settings_mode >= 1 && shader_injection.tone_map_type  >= 3.f; },
    },
    new renodx::utils::settings::Setting{
        .key = "ColorGradeFlare",
        .binding = &shader_injection.tone_map_flare,
        .default_value = 0.f,
        .label = "Flare",
        .section = "Color Grading",
        .tooltip = "Flare/Glare Compensation",
        .max = 100.f,
        .is_enabled = []() { return shader_injection.tone_map_type == 3; },
        .parse = [](float value) { return value * 0.02f; },
        .is_visible = []() { return current_settings_mode >= 1 && shader_injection.tone_map_type >= 3.f; },
    },
    new renodx::utils::settings::Setting{
        .key = "SwapChainCustomColorSpace",
        .binding = &shader_injection.swap_chain_custom_color_space,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f,
        .label = "Custom Color Space",
        .section = "Display Output",
        .tooltip = "Selects output color space"
                   "\nUS Modern for BT.709 D65."
                   "\nJPN Modern for BT.709 D93."
                   "\nUS CRT for BT.601 (NTSC-U)."
                   "\nJPN CRT for BT.601 ARIB-TR-B9 D93 (NTSC-J)."
                   "\nDefault: US CRT",
        .labels = {
            "US Modern",
            "JPN Modern",
            "US CRT",
            "JPN CRT",
        },
    },
};

const std::unordered_map<std::string, reshade::api::format> UPGRADE_TARGETS = {
    {"R8G8B8A8_TYPELESS", reshade::api::format::r8g8b8a8_typeless},
    {"B8G8R8A8_TYPELESS", reshade::api::format::b8g8r8a8_typeless},
    {"R8G8B8A8_UNORM", reshade::api::format::r8g8b8a8_unorm},
    {"B8G8R8A8_UNORM", reshade::api::format::b8g8r8a8_unorm},
    {"R8G8B8A8_SNORM", reshade::api::format::r8g8b8a8_snorm},
    {"R8G8B8A8_UNORM_SRGB", reshade::api::format::r8g8b8a8_unorm_srgb},
    {"B8G8R8A8_UNORM_SRGB", reshade::api::format::b8g8r8a8_unorm_srgb},
    {"R10G10B10A2_TYPELESS", reshade::api::format::r10g10b10a2_typeless},
    {"R10G10B10A2_UNORM", reshade::api::format::r10g10b10a2_unorm},
    {"B10G10R10A2_UNORM", reshade::api::format::b10g10r10a2_unorm},
    {"R11G11B10_FLOAT", reshade::api::format::r11g11b10_float},
    {"R16G16B16A16_TYPELESS", reshade::api::format::r16g16b16a16_typeless},
};

void OnPresetOff() {
  //   renodx::utils::settings::UpdateSetting("toneMapType", 0.f);
  //   renodx::utils::settings::UpdateSetting("toneMapPeakNits", 203.f);
  //   renodx::utils::settings::UpdateSetting("toneMapGameNits", 203.f);
  //   renodx::utils::settings::UpdateSetting("toneMapUINits", 203.f);
  //   renodx::utils::settings::UpdateSetting("toneMapGammaCorrection", 0);
  //   renodx::utils::settings::UpdateSetting("colorGradeExposure", 1.f);
  //   renodx::utils::settings::UpdateSetting("colorGradeHighlights", 50.f);
  //   renodx::utils::settings::UpdateSetting("colorGradeShadows", 50.f);
  //   renodx::utils::settings::UpdateSetting("colorGradeContrast", 50.f);
  //   renodx::utils::settings::UpdateSetting("colorGradeSaturation", 50.f);
  //   renodx::utils::settings::UpdateSetting("colorGradeLUTStrength", 100.f);
  //   renodx::utils::settings::UpdateSetting("colorGradeLUTScaling", 0.f);
}

const auto UPGRADE_TYPE_NONE = 0.f;
const auto UPGRADE_TYPE_OUTPUT_SIZE = 1.f;
const auto UPGRADE_TYPE_OUTPUT_RATIO = 2.f;
const auto UPGRADE_TYPE_ANY = 3.f;

bool initialized = false;

}  // namespace

extern "C" __declspec(dllexport) constexpr const char* NAME = "RenoDX";
extern "C" __declspec(dllexport) constexpr const char* DESCRIPTION = "RenoDX (Generic)";

BOOL APIENTRY DllMain(HMODULE h_module, DWORD fdw_reason, LPVOID lpv_reserved) {
  switch (fdw_reason) {
    case DLL_PROCESS_ATTACH:
      if (!reshade::register_addon(h_module)) return FALSE;

      if (!initialized) {
        renodx::mods::shader::force_pipeline_cloning = true;
        renodx::mods::shader::expected_constant_buffer_space = 50;
        renodx::mods::shader::expected_constant_buffer_index = 13;
        renodx::mods::shader::allow_multiple_push_constants = true;

        renodx::mods::swapchain::expected_constant_buffer_index = 13;
        renodx::mods::swapchain::expected_constant_buffer_space = 50;
        renodx::mods::swapchain::use_resource_cloning = true;

    //     renodx::mods::swapchain::swap_chain_upgrade_targets.push_back({
    //       .old_format = reshade::api::format::r8g8b8a8_unorm,
    //       .new_format = reshade::api::format::r16g16b16a16_float,
    //       .ignore_size = true,
    //       .use_resource_view_cloning = true,
    //       .use_resource_view_hot_swap = true,          
    //     //   .aspect_ratio = renodx::mods::swapchain::SwapChainUpgradeTarget::BACK_BUFFER,
    //   });

        renodx::mods::swapchain::swap_chain_upgrade_targets.push_back({.old_format = reshade::api::format::r11g11b10_float,
                                                                     .new_format = reshade::api::format::r16g16b16a16_float,
                                                                     .ignore_size = true,
                                                                     .view_upgrades = {
                                                                         {{reshade::api::resource_usage::shader_resource,
                                                                           reshade::api::format::r11g11b10_float},
                                                                          reshade::api::format::r16g16b16a16_float},
                                                                         {{reshade::api::resource_usage::unordered_access,
                                                                           reshade::api::format::r11g11b10_float},
                                                                          reshade::api::format::r16g16b16a16_float},
                                                                         {{reshade::api::resource_usage::render_target,
                                                                           reshade::api::format::r11g11b10_float},
                                                                          reshade::api::format::r16g16b16a16_float},
                                                                     }});



        // renodx::mods::swapchain::swap_chain_upgrade_targets.push_back({
        //   .old_format = reshade::api::format::r8g8b8a8_unorm,
        //   .new_format = reshade::api::format::r16g16b16a16_float,
        //   .use_resource_view_cloning = true,
        //   .aspect_ratio = renodx::mods::swapchain::SwapChainUpgradeTarget::BACK_BUFFER,
        //   .dimensions = {.width=2560, .height=1440},
        //   .use_resource_view_hot_swap = true,
        //   .usage_include = reshade::api::resource_usage::render_target,
        // });
        
        bool is_hdr10 = false;
        renodx::mods::swapchain::SetUseHDR10(is_hdr10);
        renodx::mods::swapchain::use_resize_buffer = false;
        shader_injection.swap_chain_encoding = is_hdr10 ? 4.f : 5.f;
        shader_injection.swap_chain_encoding_color_space = is_hdr10 ? 1.f : 0.f;
        

        // for (const auto& [key, format] : UPGRADE_TARGETS) {
        //   auto* setting = new renodx::utils::settings::Setting{
        //       .key = "Upgrade_" + key,
        //       .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        //       .default_value = 0.f,
        //       .label = key,
        //       .section = "Resource Upgrades",
        //       .labels = {
        //           "Off",
        //           "Output size",
        //           "Output ratio",
        //           "Any size",
        //       },
        //       .is_global = true,
        //       .is_visible = []() { return settings[0]->GetValue() >= 2; },
        //   };
        //   renodx::utils::settings::LoadSetting(renodx::utils::settings::global_name, setting);
        //   settings.push_back(setting);

        //   auto value = setting->GetValue();
        //   if (value > 0) {
        //     renodx::mods::swapchain::swap_chain_upgrade_targets.push_back({
        //         .old_format = format,
        //         .new_format = reshade::api::format::r16g16b16a16_float,
        //         .ignore_size = (value == UPGRADE_TYPE_ANY),
        //         .use_resource_view_cloning = true,
        //         .aspect_ratio = static_cast<float>((value == UPGRADE_TYPE_OUTPUT_RATIO)
        //                                                ? renodx::mods::swapchain::SwapChainUpgradeTarget::BACK_BUFFER
        //                                                : renodx::mods::swapchain::SwapChainUpgradeTarget::ANY),
        //         .usage_include = reshade::api::resource_usage::render_target,
        //     });
        //     std::stringstream s;
        //     s << "Applying user resource upgrade for ";
        //     s << format << ": " << value;
        //     reshade::log::message(reshade::log::level::info, s.str().c_str());
        //   }
        // }

        initialized = true;
      }

      break;
    case DLL_PROCESS_DETACH:
      reshade::unregister_addon(h_module);
      break;
  }

  renodx::utils::settings::Use(fdw_reason, &settings, &OnPresetOff);
  renodx::mods::swapchain::Use(fdw_reason, &shader_injection);
  renodx::mods::shader::Use(fdw_reason, custom_shaders, &shader_injection);

  return TRUE;
}
