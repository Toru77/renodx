/*
 * Copyright (C) 2024 Carlos Lopez
 * SPDX-License-Identifier: MIT
 */

#define ImTextureID ImU64

#define DEBUG_LEVEL_0

#define RENODX_MODS_SWAPCHAIN_VERSION 2

#include <deps/imgui/imgui.h>
#include <include/reshade.hpp>

#include <embed/shaders.h>

#include "../../mods/shader.hpp"
#include "../../mods/swapchain.hpp"
#include "../../utils/settings.hpp"
#include "../../utils/swapchain.hpp"
#include "./shared.h"

namespace {

ShaderInjectData shader_injection;


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
    CustomSwapchainShader(0xAD51B4B0), // Kuro final
    UpgradeRTVReplaceShader(0x28FFFB4A), // Kuro proxy
    UpgradeRTVReplaceShader(0x034581D3), // Kuro overlay blending
    UpgradeRTVReplaceShader(0x83F2D19E), // blur sampler
    UpgradeRTVReplaceShader(0x5BB549F7), // blur gen
    UpgradeRTVReplaceShader(0xCE7C6E9D), // depth
    UpgradeRTVReplaceShader(0x0A0B2E57), // depth
    UpgradeRTVReplaceShader(0x43E0BB74), // blur
    UpgradeRTVReplaceShader(0xB38A8D5E), // proxy
    UpgradeRTVReplaceShader(0x2D620443), // proxy
    // Kuro/Kuro2 interpolate (also runs as menublur on the same CRC).
    // The HLSL replacement applies tonemap inline; on_drawn flips the
    // scene_already_tonemapped flag so final/finalkai skip their own tonemap.
    // {0x2D620443,
    //  {
    //      .crc32 = 0x2D620443,
    //      .code = __0x2D620443,
    //      .on_draw = [](reshade::api::command_list* cmd_list) {
    //        auto rtvs = renodx::utils::swapchain::GetRenderTargets(cmd_list);
    //        bool changed = false;
    //        for (auto rtv : rtvs) {
    //          changed = renodx::mods::swapchain::ActivateCloneHotSwap(cmd_list->get_device(), rtv);
    //        }
    //        if (changed) {
    //          renodx::mods::swapchain::FlushDescriptors(cmd_list);
    //          renodx::mods::swapchain::RewriteRenderTargets(cmd_list, rtvs.size(), rtvs.data(), {0});
    //        }
    //        return true;
    //      },
    //      .on_drawn = [](reshade::api::command_list* /*cmd_list*/) {
    //        shader_injection.scene_already_tonemapped = 1.f;
    //      },
    //  }},
    UpgradeRTVReplaceShader(0xAF7B0499), // refraction
    UpgradeRTVReplaceShader(0xFA1A3F24), // atmosphere

    // Kuro 2
    
    UpgradeRTVReplaceShader(0xD21C3838), // bloom blend
    UpgradeRTVReplaceShader(0x7FD880F5), // godray
    UpgradeRTVReplaceShader(0x1336F6F8), // blur
    UpgradeRTVReplaceShader(0x2A89E417), // luma
    UpgradeRTVReplaceShader(0x96FF2893), // depth2
    UpgradeRTVReplaceShader(0xA69F0EDC), // intensity
    UpgradeRTVReplaceShader(0x61EF61EA), // ao
    UpgradeRTVReplaceShader(0xB24294F0), // atmosphere 2
    UpgradeRTVReplaceShader(0xA1DEB90B), // exposure 
    UpgradeRTVReplaceShader(0x16371E61), // neon
    UpgradeRTVReplaceShader(0xB38A8D5E), // proxy
    CustomShaderEntry(0x21990235), // hud

    // Kai

    CustomSwapchainShader(0xA39E174C), // Kai final
    UpgradeRTVReplaceShader(0xD606E924), // neon
    UpgradeRTVReplaceShader(0x25A9822B), // neon
    UpgradeRTVReplaceShader(0x3C7947BE), // neon
    UpgradeRTVReplaceShader(0xC79A113F), // depth3
    UpgradeRTVReplaceShader(0xCDB2A000), // godray
    UpgradeRTVReplaceShader(0x0F02FFD8), // godraygen
    CustomShaderEntry(0x8C8333BF), // hud
    CustomShaderEntry(0xB3719CBF), // hud2
    // UpgradeRTVReplaceShader(0x8C8333BF), // hud
    

    // Sora 

    CustomShaderEntry(0xE20E1A41), // final
    CustomShaderEntry(0x14DAB5E7), // final
    CustomShaderEntry(0xC9FA40B7), // tonemap
    CustomShaderEntry(0xCDE6FA28), // bloomTAA
    CustomShaderEntry(0xD6CF040B), // bloom blend
    CustomShaderEntry(0x0F66FA5C), // bloom
    CustomShaderEntry(0xD1F5CAA2), // bloom
    CustomShaderEntry(0x1343A4D6), // bloom
    CustomShaderEntry(0x2333272C), // bloom
    CustomShaderEntry(0xEA80BE9C), // ao
    CustomShaderEntry(0xAD69DC2B), // sky
    CustomShaderEntry(0x2026566A), // smoke
    CustomShaderEntry(0xFA37EA04), // taa
    CustomShaderEntry(0x31FE6C05), // hud
    CustomShaderEntry(0xDCC360B5), // hud

    // YS X
    CustomShaderEntry(0xC8EBAB1E), // hud
    CustomShaderEntry(0xAA5313D9), // hud
    {0x2EB53B7A,
     {
         .crc32 = 0x2EB53B7A,
         .code = __0x2EB53B7A,
         .on_draw = [](reshade::api::command_list* cmd_list) {
           auto rtvs = renodx::utils::swapchain::GetRenderTargets(cmd_list);
           bool changed = false;
           for (auto rtv : rtvs) {
             changed = renodx::mods::swapchain::ActivateCloneHotSwap(cmd_list->get_device(), rtv);
           }
           if (changed) {
             renodx::mods::swapchain::FlushDescriptors(cmd_list);
             renodx::mods::swapchain::RewriteRenderTargets(cmd_list, rtvs.size(), rtvs.data(), {0});
           }
           return true;
         },
         .on_drawn = [](reshade::api::command_list* /*cmd_list*/) {
           shader_injection.scene_already_tonemapped = 1.f;
         },
     }},

     CustomShaderEntry(0x2388DAAC), // final ys proud nordic
     CustomShaderEntry(0x557E2C4E), // ys overlay
     CustomShaderEntry(0x36B738AD), // ui
     CustomShaderEntry(0x44DFE546), // ui

     //Kyoto Xanadu
     CustomShaderEntry(0xB879528D), // bloom final
     CustomShaderEntry(0x197BCB74), // bloom blend
     CustomShaderEntry(0x5AFABB1C), // bloom godray
     CustomShaderEntry(0xDF53C75B), // hud

    // UpgradeRTVShader(0x1336F6F8),
    // UpgradeRTVShader(0xEF0CAEEA),
    // UpgradeRTVShader(0x7BC8A1E8),
    // CustomSwapchainShader(0x00000000),
    // BypassShaderEntry(0x00000000)
};

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
        .key = "bloomSpace",
        .binding = &shader_injection.bloom_space,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f,
        .can_reset = true,
        .label = "Bloom Space",
        .section = "Game Settings",
        .tooltip = "Falcom by default generates Bloom samples in sRGB.",
        .labels = {"Falcom (sRGB)", "Linear"},
        .parse = [](float value) { return 0.f; },
        // .is_visible = []() { return current_settings_mode >= 1; },
        .is_visible = []() { return false; },
    },
    new renodx::utils::settings::Setting{
        .key = "bloom",
        .binding = &shader_injection.bloom,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 1.f,
        .can_reset = true,
        .label = "Bloom Blending",
        .section = "Game Settings",
        .tooltip = "Bloom blending method.",
        .labels = {"Falcom (SDR)", "HDR"},
        .is_visible = []() { return current_settings_mode >= 1; },
    },

    new renodx::utils::settings::Setting{
        .key = "fxBloom",
        .binding = &shader_injection.bloom_strength,
        .default_value = 50.f,
        .label = "Bloom Strength",
        .section = "Game Settings",
        .tooltip = "Controls Bloom Strength",
        .max = 100.f,
        .parse = [](float value) { return value * 0.1f; },
        .is_visible = []() { return shader_injection.bloom >= 1.f; },
        // .is_visible = []() { return false; },
    },

    new renodx::utils::settings::Setting{
        .key = "fxBloomCorrection",
        .binding = &shader_injection.bloom_hue_correction,
        .default_value = 75.f,
        .label = "Bloom Color Correction",
        .section = "Game Settings",
        .tooltip = "Correcting the colors after rewriting Bloom to avoid red faces.",
        .min = 0.f,
        .max = 100.f,
        .is_enabled = []() { return shader_injection.tone_map_type >= 1; },
        .parse = [](float value) { return value * 0.01f; },
        // .is_visible = []() { return false; },
        .is_visible = []() { return current_settings_mode >= 2 && shader_injection.bloom >= 1.f; },
    },
    new renodx::utils::settings::Setting{
        .key = "ToneMapType",
        .binding = &shader_injection.tone_map_type,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 1.f,
        .can_reset = true,
        .label = "Tonemapping Algorithm",
        .section = "Tone Mapping",
        .tooltip = "Sets the tone mapper type",
        // .labels = {"Vanilla", "Frostbite", "DICE", "Reinhard", "ExponentialRollOff", "Bezier"},
        // .labels = {"Vanilla", "Frostbite", "DICE", "Hermite", "Mass Effect", "Neutwo", "PsychoV"},
        .labels = {"Vanilla", "PsychoV", "RenoDRT"},
        .is_visible = []() { return current_settings_mode >= 1; },
    },
    new renodx::utils::settings::Setting{
        .key = "ToneMapRenoDRTType",
        .binding = &shader_injection.renodrt_tone_map_type,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 1.f,
        .can_reset = true,
        .label = "RenoDRT Algorithm",
        .section = "Tone Mapping",
        .tooltip = "Sets the tone mapper type",
        // .labels = {"Vanilla", "Frostbite", "DICE", "Reinhard", "ExponentialRollOff", "Bezier"},
        // .labels = {"Vanilla", "Frostbite", "DICE", "Hermite", "Mass Effect", "Neutwo", "PsychoV"},
        .labels = {"Danille", "Reinhard", "Hermite Spline"},
        .is_enabled = []() { return shader_injection.tone_map_type == 2; },
        .is_visible = []() { return current_settings_mode >= 2 && shader_injection.tone_map_type == 2; },
        
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
        // .is_visible = []() { return false; },
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
        .is_visible = []() { return current_settings_mode >= 1; },
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

    new renodx::utils::settings::Setting{
        .key = "ColorGradeContrast",
        .binding = &shader_injection.tone_map_lms_contrast,
        .default_value = 50.f,
        .label = "Contrast",
        .section = "Color Grading",
        .max = 100.f,
        .parse = [](float value) { return value * 0.02f; },
    },

    new renodx::utils::settings::Setting{
        .key = "ColorGradeVibrancy",
        .binding = &shader_injection.tone_map_lms_vibrancy,
        .default_value = 50.f,
        .label = "Vibrancy (Saturation)",
        .section = "Color Grading",
        .max = 100.f,
        .parse = [](float value) { return value * 0.02f; },
    },
    new renodx::utils::settings::Setting{
        .key = "ColorGradeDechroma",
        .binding = &shader_injection.tone_map_lms_dechroma,
        .default_value = 0.f,
        .label = "Dechroma",
        .section = "Color Grading",
        .max = 100.f,
        .parse = [](float value) { return value * 0.01f; },
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

// Reset the lazy-tonemap latch each frame. The first registered HUD shader
// drawn in the next frame will dispatch the tonemap pass and flip this back
// to 1; final reads it to decide whether to tonemap itself.
void OnPresent(
    reshade::api::command_queue* /*queue*/,
    reshade::api::swapchain* /*swapchain*/,
    const reshade::api::rect* /*source_rect*/,
    const reshade::api::rect* /*dest_rect*/,
    uint32_t /*dirty_rect_count*/,
    const reshade::api::rect* /*dirty_rects*/) {
  shader_injection.scene_already_tonemapped = 0.f;
}

}  // namespace

extern "C" __declspec(dllexport) constexpr const char* NAME = "RenoDX";
extern "C" __declspec(dllexport) constexpr const char* DESCRIPTION = "RenoDX";

BOOL APIENTRY DllMain(HMODULE h_module, DWORD fdw_reason, LPVOID lpv_reserved) {
  switch (fdw_reason) {
    case DLL_PROCESS_ATTACH:
      if (!reshade::register_addon(h_module)) return FALSE;

      reshade::register_event<reshade::addon_event::present>(OnPresent);

      if (!initialized) {
        renodx::mods::shader::force_pipeline_cloning = true;
        renodx::mods::shader::expected_constant_buffer_space = 50;
        renodx::mods::shader::expected_constant_buffer_index = 13;
        renodx::mods::shader::allow_multiple_push_constants = true;

        renodx::mods::swapchain::expected_constant_buffer_index = 13;
        renodx::mods::swapchain::expected_constant_buffer_space = 50;
        renodx::mods::swapchain::use_resource_cloning = true;


        auto* upgrade_setting = new renodx::utils::settings::Setting{
            .key = "UnsafeUpgrade8bit",
            .binding = &shader_injection.upgrade_8bit,
            .value_type = renodx::utils::settings::SettingValueType::INTEGER,
            .default_value = 1.f,
            .label = "Upgrade 8-bit Render Targets",
            .section = "Display Output",
            .tooltip = "Upgrade Unsafe render targets (untested)",
            .labels = {"Off", "On"},
            .is_enabled = []() { return true; },
            .is_global = true,
            .is_visible = []() { return current_settings_mode >= 2; },
        }; 

        renodx::utils::settings::LoadSetting(renodx::utils::settings::global_name, upgrade_setting);
        bool unsafe_upgrade_8bit = upgrade_setting->GetValue() > 0;
        settings.push_back(upgrade_setting);

        if (unsafe_upgrade_8bit)   {
            renodx::mods::swapchain::swap_chain_upgrade_targets.push_back({
                .old_format = reshade::api::format::r8g8b8a8_unorm,
                .new_format = reshade::api::format::r16g16b16a16_float,
                .ignore_size = true,
                .use_resource_view_cloning = true,
                .use_resource_view_hot_swap = true,          
                //   .aspect_ratio = renodx:mods::swapchain::SwapChainUpgradeTarget::BACK_BUFFER,
            });
        }


        auto* upgrade_setting_2 = new renodx::utils::settings::Setting{
            .key = "UnsafeUpgrade10bit",
            .binding = &shader_injection.upgrade_10bit,
            .value_type = renodx::utils::settings::SettingValueType::INTEGER,
            .default_value = 1.f,
            .label = "Upgrade 10-bit Render Targets",
            .section = "Display Output",
            .tooltip = "Upgrade Unsafe render targets (untested)",
            .labels = {"Off", "On"},
            .is_enabled = []() { return true; },
            .is_global = true,
            .is_visible = []() { return current_settings_mode >= 2; },
        }; 

        renodx::utils::settings::LoadSetting(renodx::utils::settings::global_name, upgrade_setting_2);
        bool unsafe_upgrade_10bit = upgrade_setting_2->GetValue() > 0;
        settings.push_back(upgrade_setting_2);

        if (unsafe_upgrade_10bit)   {
            renodx::mods::swapchain::swap_chain_upgrade_targets.push_back({
            .old_format = reshade::api::format::r11g11b10_float,
            .new_format = reshade::api::format::r16g16b16a16_float,
            .ignore_size = true,
            //   .use_resource_view_cloning = true,
            .use_resource_view_hot_swap = true,          
            //   .aspect_ratio = renodx::mods::swapchain::SwapChainUpgradeTarget::BACK_BUFFER,
        });
        }


        {
            auto* setting = new renodx::utils::settings::Setting{
                .key = "SwapChainEncoding",
                .binding = &shader_injection.hdr_format,
                .value_type = renodx::utils::settings::SettingValueType::INTEGER,
                .default_value = 1.f,
                .label = "HDR Format",
                .section = "Display Output",
                .tooltip = "Sets the HDR format (HDR10 is compatible with Smooth Motion)",
                .labels = {"HDR10", "scRGB (default)"},
                .is_enabled = []() { return true; },
                .is_global = true,
                .is_visible = []() { return current_settings_mode >= 2; },
            }; 

            renodx::utils::settings::LoadSetting(renodx::utils::settings::global_name, setting);
            bool is_hdr10 = setting->GetValue() == 0;
            renodx::mods::swapchain::SetUseHDR10(is_hdr10);
            renodx::mods::swapchain::use_resize_buffer = false;
            shader_injection.swap_chain_encoding = (is_hdr10 ? 4.f : 5.f);
            shader_injection.swap_chain_encoding_color_space = is_hdr10 ? 1.f : 0.f;
            settings.push_back(setting);

            if (is_hdr10)   {
                renodx::mods::swapchain::swap_chain_upgrade_targets.push_back({
                .old_format = reshade::api::format::r10g10b10a2_typeless,
                .new_format = reshade::api::format::r16g16b16a16_typeless,
                .use_resource_view_cloning = true,
                });
            }
        }

        initialized = true;
      }

      break;
    case DLL_PROCESS_DETACH:
      reshade::unregister_event<reshade::addon_event::present>(OnPresent);
      reshade::unregister_addon(h_module);
      break;
  }

  renodx::utils::settings::Use(fdw_reason, &settings, &OnPresetOff);
  renodx::mods::swapchain::Use(fdw_reason, &shader_injection);
  renodx::mods::shader::Use(fdw_reason, custom_shaders, &shader_injection);

  return TRUE;
}
