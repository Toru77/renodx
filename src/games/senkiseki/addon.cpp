/*
 * Copyright (C) 2024 Carlos Lopez
 * SPDX-License-Identifier: MIT
 */

#define DEBUG_LEVEL_0

#define RENODX_MODS_SWAPCHAIN_VERSION 2

#include <include/reshade_api_resource.hpp>
#include <iostream>
#define ImTextureID ImU64

#include <deps/imgui/imgui.h>
#include <include/reshade.hpp>

#include <embed/shaders.h>

#include "../../mods/shader.hpp"
#include "../../mods/swapchain.hpp"
#include "../../utils/settings.hpp"
#include <filesystem>
#include <regex>
#include "./shared.h"

namespace fs = std::filesystem;

namespace {

#define UpgradeRTVReplaceShader(value)         \
  {                                            \
    value,                                     \
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

#define UpgradeRTVShader(value)                \
  {                                            \
    value,                                     \
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

renodx::mods::shader::CustomShaders artifact_shaders = {

    CustomShaderEntry(0x0A800445), // Artifact 
    CustomShaderEntry(0x24E3F2E9), // Artifact
    CustomShaderEntry(0x51A0872B), // Artifact
    CustomShaderEntry(0xC24A5C78),
    CustomShaderEntry(0xC7178B80), // Artifact
    CustomShaderEntry(0xD9D77A15), // Artifact
    CustomShaderEntry(0xF5F14005), // Artifact
    CustomShaderEntry(0x1A8F8A60), // Artifact
    CustomShaderEntry(0x010A2698), // Artifact
    CustomShaderEntry(0x1FEBC77F), // Artifact
    CustomShaderEntry(0xFB81CC22), // Artifact
    CustomShaderEntry(0x53F9EA61), // Helix
    CustomShaderEntry(0x8377693A), // Helix
    CustomShaderEntry(0xF52ED722), // Helix
    CustomShaderEntry(0x7C713EA2), // artifact
    CustomShaderEntry(0x899A5037), // artifact
    CustomShaderEntry(0x92AF88E8), // artifact
    CustomShaderEntry(0x693BB6AD), // artifact
    CustomShaderEntry(0x0617F17E), // artifact
    CustomShaderEntry(0x864D0B94), // artifact
    CustomShaderEntry(0x28A12202), // artifact
    CustomShaderEntry(0x03A9635B), // artifact
    CustomShaderEntry(0xD4CC30DD), // artifact
    CustomShaderEntry(0x5904850F), // artifact
    CustomShaderEntry(0xAEFC72F9), // artifact
    CustomShaderEntry(0x5A14FF14), // artifact
    CustomShaderEntry(0x97B6686A), // artifact
    CustomShaderEntry(0x16C5613F), // artifact
    CustomShaderEntry(0xE9CCCC87), // artifact
    CustomShaderEntry(0xE011F8E0), // artifact
    CustomShaderEntry(0xAC986310), // artifact
    CustomShaderEntry(0xF4F7DF67), // artifact
    CustomShaderEntry(0x00D25A6E), // artifact
    CustomShaderEntry(0x2CA64FEA), // artifact
    CustomShaderEntry(0x4EFC489C), // artifact
    CustomShaderEntry(0xD76CAD22), // artifact
    CustomShaderEntry(0x27C49A90), // artifact
    CustomShaderEntry(0xEB8EAAE7), // artifact
    CustomShaderEntry(0x2140995A), // artifact
    CustomShaderEntry(0xBA8780BB), // artifact
    CustomShaderEntry(0xC757392B), // artifact
    CustomShaderEntry(0xFBEC108C), // artifact
    CustomShaderEntry(0xB6F58E83), // artifact
    CustomShaderEntry(0x83970024), // artifact
    CustomShaderEntry(0x9971D43C), // artifact
    CustomShaderEntry(0x40AAA6CF), // artifact
    CustomShaderEntry(0x0CE1250D), // artifact
    CustomShaderEntry(0x2F9ED134), // artifact
    CustomShaderEntry(0x4EFC489C), // artifact
    CustomShaderEntry(0x374F364D), // artifact
    CustomShaderEntry(0xF299DBA1), // artifact
    CustomShaderEntry(0x4A5044D3), // artifact
    CustomShaderEntry(0x85AF2462), // artifact
    CustomShaderEntry(0x96E2605E), // artifact
    CustomShaderEntry(0x4C191089), // artifact
    CustomShaderEntry(0xA15E5BC7), // artifact
    CustomShaderEntry(0x19B22350), // artifact
    CustomShaderEntry(0xA750A152), // artifact
    CustomShaderEntry(0x9FCDD407), // artifact
    CustomShaderEntry(0x92449B53), // artifact
    CustomShaderEntry(0x63655D7C), // artifact
    CustomShaderEntry(0x2286C934), // artifact
    CustomShaderEntry(0xC41CBE29), // artifact
    CustomShaderEntry(0xBDFDE2B7), // artifact
    CustomShaderEntry(0x4CB2EE15), // artifact
    CustomShaderEntry(0x1E7B91F3), // artifact
    CustomShaderEntry(0x6E9186AE), // artifact
    CustomShaderEntry(0x2DAF865D), // artifact
    CustomShaderEntry(0x8BAAE7E7), // artifact
    CustomShaderEntry(0xB57CA59A), // artifact
    CustomShaderEntry(0x0368DF1C), // artifact
    CustomShaderEntry(0xCCF227A3), // artifact
    CustomShaderEntry(0xF6FA90E3), // artifact
    CustomShaderEntry(0x9B942E73), // artifact
    CustomShaderEntry(0x03B8A7E5), // artifact
    CustomShaderEntry(0x0B878318), // artifact
    CustomShaderEntry(0x5B6BFD59), // artifact
    CustomShaderEntry(0x6F6DE38C), // artifact
    CustomShaderEntry(0xF5810D76), // artifact
    CustomShaderEntry(0x9CCE33D1), // artifact
    CustomShaderEntry(0xB68A3D3F), // artifact
    CustomShaderEntry(0x3100C46D), // artifact
    CustomShaderEntry(0x11149045), // artifact
    CustomShaderEntry(0xBD20A5EA), // artifact
    CustomShaderEntry(0xD2EAD98A), // artifact
    CustomShaderEntry(0x6FEA38DA), // artifact
    
    
};

renodx::mods::shader::CustomShaders custom_shaders = {

    CustomShaderEntry(0x46A727D9), // final 4
    CustomShaderEntry(0x2BF0C94B), // Final 3
    CustomShaderEntry(0x3E08A3A6), // Final 
    CustomShaderEntry(0x2A8422DE), // Final 2
    CustomShaderEntry(0x49107B8F), // Final 6
    CustomShaderEntry(0x1C7DCC30), // Final 7
    CustomShaderEntry(0x3541804A), // Final 8
    CustomShaderEntry(0xFD245ECC), // Final 9
    CustomShaderEntry(0x93DEF816), // Final 10
    CustomShaderEntry(0x0469E6D6), // Final 11
    CustomShaderEntry(0x16DA1605), // Final 12
    CustomShaderEntry(0xC2D07E63), // Final 5
    CustomShaderEntry(0x1ABB15C9), // Final 13
    CustomShaderEntry(0x228096A4), // Final 14
    CustomShaderEntry(0x2005E90C), // Final 15
    CustomShaderEntry(0x2F2D2517), // Final 16
    CustomShaderEntry(0x30064E3D), // Final 17
    CustomShaderEntry(0x38B0F676), // Final 18
    CustomShaderEntry(0x46589838), // Final 19
    CustomShaderEntry(0x4C17DC8C), // Final 20
    CustomShaderEntry(0x4D291F04), // Final 21
    CustomShaderEntry(0x4E97BECA), // Final 22
    CustomShaderEntry(0x4F6D55D7), // Final 23
    CustomShaderEntry(0x5F75A7B5), // Final 24
    CustomShaderEntry(0x6417B671), // Final 25
    CustomShaderEntry(0x575568A5), // Final 26
    CustomShaderEntry(0x68F73C03), // Final 27
    CustomShaderEntry(0x71DC9089), // Final 28
    CustomShaderEntry(0x762A5B0E), // Final 29
    CustomShaderEntry(0x7DD4CB97), // Final 30
    CustomShaderEntry(0x7E245ADC), // Final 31
    CustomShaderEntry(0x83446FEF), // Final 32
    CustomShaderEntry(0x84648647), // Final 33
    CustomShaderEntry(0x8589D7AF), // Final 34
    CustomShaderEntry(0x8A4275AA), // Final 35
    CustomShaderEntry(0x8F06D84C), // Final 36
    CustomShaderEntry(0xA07D8921), // Final 37
    CustomShaderEntry(0x94889944), // Final 38
    CustomShaderEntry(0x9BF2734C), // Final 39
    CustomShaderEntry(0xA01BCC76), // Final 40
    CustomShaderEntry(0xA20ECB4B), // Final 41
    CustomShaderEntry(0xA210474C), // Final 42
    CustomShaderEntry(0xA4560071), // Final 43
    CustomShaderEntry(0xA6A642EC), // Final 44
    CustomShaderEntry(0xA716393F), // Final 45
    CustomShaderEntry(0xA7C2CED9), // Final 46
    CustomShaderEntry(0xAC7DDDAC), // Final 47
    CustomShaderEntry(0xB1E0A90D), // Final 48
    CustomShaderEntry(0xB41DEC9B), // Final 49
    CustomShaderEntry(0xB573287E), // Final 50
    CustomShaderEntry(0xB59D548A), // Final 51
    CustomShaderEntry(0xB5A7008B), // Final 52
    CustomShaderEntry(0xB936A99B), // Final 53
    CustomShaderEntry(0xBB0DEC2A), // Final 54
    CustomShaderEntry(0xBEE3AF35), // Final 55
    CustomShaderEntry(0xCC00713E), // Final 56
    CustomShaderEntry(0xCF4D44BC), // Final 57
    CustomShaderEntry(0xD01F775C), // Final 58
    CustomShaderEntry(0xD0D6FE14), // Final 59
    CustomShaderEntry(0xDA5C467F), // Final 60
    CustomShaderEntry(0xDBC91624), // Final 61
    CustomShaderEntry(0xDECB5EF1), // Final 62
    CustomShaderEntry(0xE1D7A2B6), // Final 63
    CustomShaderEntry(0xE65E731D), // Final 64
    CustomShaderEntry(0xEC63410F), // Final 65
    CustomShaderEntry(0xED6722DC), // Final 66
    CustomShaderEntry(0xED849BD1), // Final 67
    CustomShaderEntry(0xF0566A0F), // Final 68
    CustomShaderEntry(0xFCBCF123), // Final 69

    CustomShaderEntry(0x256687A6), // sun
    CustomShaderEntry(0x2403F73F), // ui
    CustomShaderEntry(0xD6241E03), // ui
    CustomShaderEntry(0x631BC5B3), // ui
    CustomShaderEntry(0x0844C159), // ui
    CustomShaderEntry(0xB55FDE0D), // ui
    CustomShaderEntry(0x131CC98E),
    
    
    
    CustomShaderEntry(0x9DB02646), // swapchain unclamp
    CustomShaderEntry(0xF0FA2768), // artifact
    CustomShaderEntry(0x386909EF), // bloom artifact
    CustomShaderEntry(0xE8C7EBA2), // bloom artifact
    // CustomShaderEntry(0x96BB8CFF), // FXAA
    CustomShaderEntry(0xC5F739B8), // Tone
    CustomShaderEntry(0x312CE29D), // PreBloom
    CustomShaderEntry(0x640BFEB8), // Bloom
    CustomShaderEntry(0xC1EA843D), // Bloom 2
    CustomShaderEntry(0xD63B1562), // Bloom 4
    CustomShaderEntry(0x96FE091D), // Bloom 1
    CustomShaderEntry(0xFB26B904), // Bloom godray 3
    CustomShaderEntry(0xA3F66EB7), // Artifact
    CustomShaderEntry(0xA77E6454), // Artifact 
    CustomShaderEntry(0x3EB64A30), // Artifact 
    CustomShaderEntry(0x085FEFA2), // Artifact 
    CustomShaderEntry(0x466B2B3A), // Artifact 
    CustomShaderEntry(0xA8FAC241), // Artifact 
    CustomShaderEntry(0x0EC06FAC), // Lantern
    CustomShaderEntry(0x4D7BB28D), // Helicopter
    CustomShaderEntry(0xACB821F7), // effect
    CustomShaderEntry(0x7DF4072B), // effect
    CustomShaderEntry(0x90904C40), // effect
    CustomShaderEntry(0xD2EE0840), // effect
    CustomShaderEntry(0x1275C3E6), // environment
    CustomShaderEntry(0x605593B6), // environment
    CustomShaderEntry(0x6907BF88), // lantern
    CustomShaderEntry(0xCF69F81A), // effect
    
    // CustomShaderEntry(0x6C72EE93), // Bloom Generator
    // CS4
    
    CustomShaderEntry(0xE92523E6), // Light 
    CustomShaderEntry(0x2CE1FDAD), // Moon
    CustomShaderEntry(0x7CC40E3A), // Artifact 
    CustomShaderEntry(0xDCCD09BC), // Artifact 
    CustomShaderEntry(0xC36D80C8), // Artifact 
    CustomShaderEntry(0x825031AD), // Artifact 
    CustomShaderEntry(0x2EB3D75C), // Artifact 
    CustomShaderEntry(0x311ECA34), // Artifact 
    CustomShaderEntry(0x5DFE166C), // Artifact 
    CustomShaderEntry(0xE291FC5C), // Artifact 
    CustomShaderEntry(0xE291FC5C), // Artifact 
    CustomShaderEntry(0x2A1ED860), // Artifact 
    CustomShaderEntry(0x3F916BC5), // Sky Artifact 
    CustomShaderEntry(0x40E73DDB), // mirror light
    CustomShaderEntry(0x39657C8A), // light
    CustomShaderEntry(0xBB9756CF), // light
    CustomShaderEntry(0x24FB4CC2), // light
    CustomShaderEntry(0x36E4465F), // light
    CustomShaderEntry(0x50ED90FA), // light
    CustomShaderEntry(0xD15C5D7D), // light
    CustomShaderEntry(0xE7F4B7AF), // lantern
    CustomShaderEntry(0x2BBCADD8), // lantern
    CustomShaderEntry(0xF5F14005), // lantern
    CustomShaderEntry(0x8E631192), // waterfall
    CustomShaderEntry(0x6E7F5CBB), // waterfall2
    CustomShaderEntry(0xB64DF407), // sky
    CustomShaderEntry(0x8C099E7B), // lantern
    CustomShaderEntry(0xB9AF63CD), // rean eff
    CustomShaderEntry(0xAF5B4CE1), // lantern
    CustomShaderEntry(0xE3E9C74F), // lantern
    CustomShaderEntry(0x51EB2788), // lantern
    CustomShaderEntry(0x01906E4B), // lantern
    CustomShaderEntry(0x4C831242), // lantern 
    CustomShaderEntry(0xCD0D39E7), // light
    CustomShaderEntry(0x124B65AE), // mcburn effect
    CustomShaderEntry(0x80752CEC), // mcburn effect
    CustomShaderEntry(0xBF10EB11), // ssao
    CustomShaderEntry(0x6FFD6C4A), // effect
    CustomShaderEntry(0x8D2570A9), // effect
    CustomShaderEntry(0xBB1F4A52), // effect

    

    // CS3
    CustomShaderEntry(0xDB20052A), // CS3 swapchain
    CustomShaderEntry(0x50F76D1C), // tone
    CustomShaderEntry(0xFAD1BDE8), // CS3 bloom
    CustomShaderEntry(0x8E0121CE), // CS3 bloom
    CustomShaderEntry(0x36ED28F7), // CS3 bloom
    CustomShaderEntry(0x6B574B6E), // CS3 final 
    CustomShaderEntry(0xABF4E009), // CS3 final 2
    CustomShaderEntry(0xB4406452), // CS3 final 3
    CustomShaderEntry(0x95F02C1D), // CS3 final 4
    CustomShaderEntry(0x26217A30), // CS3 final 5
    CustomShaderEntry(0x46DC6C58), // CS3 final 6
    CustomShaderEntry(0x5A5A4C5A), // CS3 final 7
    CustomShaderEntry(0x322E20D4), // CS3 final 8
    CustomShaderEntry(0x00C4E2A6), // CS3 final 9
    CustomShaderEntry(0x0161438C), // CS3 final 10
    CustomShaderEntry(0x01E1FF0F), // CS3 final 11
    CustomShaderEntry(0x02E4863E), // CS3 final 12
    CustomShaderEntry(0x0EBE33EC), // CS3 final 13
    CustomShaderEntry(0x11CAE0E6), // CS3 final 14
    CustomShaderEntry(0x1486065D), // CS3 final 15
    CustomShaderEntry(0x160A4895), // CS3 final 16
    CustomShaderEntry(0x167A12F5), // CS3 final 17
    CustomShaderEntry(0x1D403F97), // CS3 final 18
    CustomShaderEntry(0x1EDECCEC), // CS3 final 19
    CustomShaderEntry(0x215168AB), // CS3 final 20
    CustomShaderEntry(0x2594FB2C), // CS3 final 21
    CustomShaderEntry(0x269DDBDF), // CS3 final 22
    CustomShaderEntry(0x281F2814), // CS3 final 23
    CustomShaderEntry(0x31E727B4), // CS3 final 24
    CustomShaderEntry(0x3415A1BE), // CS3 final 25
    CustomShaderEntry(0x34EAE8B6), // CS3 final 26
    CustomShaderEntry(0x38BB9A49), // CS3 final 27
    CustomShaderEntry(0x3FB18938), // CS3 final 28
    CustomShaderEntry(0x513CF99F), // CS3 final 29
    CustomShaderEntry(0x517BF7B2), // CS3 final 30
    CustomShaderEntry(0x51DB8AC5), // CS3 final 31
    CustomShaderEntry(0x52DBC392), // CS3 final 32
    CustomShaderEntry(0x53B8577C), // CS3 final 33
    CustomShaderEntry(0x5A1C72D5), // CS3 final 34
    CustomShaderEntry(0x5BBE9F57), // CS3 final 35
    CustomShaderEntry(0x67F0BD68), // CS3 final 36
    CustomShaderEntry(0x6C550D4F), // CS3 final 37
    CustomShaderEntry(0x78F1AC05), // CS3 final 38
    CustomShaderEntry(0x884D23A7), // CS3 final 39
    CustomShaderEntry(0x8923A287), // CS3 final 40
    CustomShaderEntry(0x8BDD1E5D), // CS3 final 41
    CustomShaderEntry(0x90180FAF), // CS3 final 42
    CustomShaderEntry(0x9655B4B2), // CS3 final 43
    CustomShaderEntry(0x97EB039C), // CS3 final 44
    CustomShaderEntry(0x9B0A1C60), // CS3 final 45
    CustomShaderEntry(0xA4945FF3), // CS3 final 46
    CustomShaderEntry(0xA4D9C6FD), // CS3 final 47
    CustomShaderEntry(0xA671C250), // CS3 final 48
    CustomShaderEntry(0xA9F09088), // CS3 final 49
    CustomShaderEntry(0xABE9BEE1), // CS3 final 50
    CustomShaderEntry(0xAC84E828), // CS3 final 51
    CustomShaderEntry(0xACFCDFC2), // CS3 final 52
    CustomShaderEntry(0xB32BC083), // CS3 final 53
    CustomShaderEntry(0xB3BF88B3), // CS3 final 54
    CustomShaderEntry(0xB78778F5), // CS3 final 55
    CustomShaderEntry(0xBA62ACD7), // CS3 final 56
    CustomShaderEntry(0xC2DFA434), // CS3 final 57
    CustomShaderEntry(0xCED8E8A8), // CS3 final 58
    CustomShaderEntry(0xCFF51135), // CS3 final 59
    CustomShaderEntry(0xD7BC302F), // CS3 final 60
    CustomShaderEntry(0xD8F91B51), // CS3 final 61
    CustomShaderEntry(0xE0E54773), // CS3 final 62
    CustomShaderEntry(0xE13AAD9B), // CS3 final 63
    CustomShaderEntry(0xE2FC9C22), // CS3 final 64
    CustomShaderEntry(0xE52B3C27), // CS3 final 65
    CustomShaderEntry(0xE742F2C3), // CS3 final 66
    CustomShaderEntry(0xEC64A31A), // CS3 final 67
    CustomShaderEntry(0xEFAC375C), // CS3 final 68
    CustomShaderEntry(0xEFC9A329), // CS3 final 69


    CustomShaderEntry(0x547D8B6A), // dragon boss
    CustomShaderEntry(0x70EAAEFC), // lighthouse
    CustomShaderEntry(0xD4F2A488), // ortis light
    CustomShaderEntry(0xA7E488FF), // aion
    CustomShaderEntry(0x9243FD49), // valimar
    CustomShaderEntry(0xFF76CAFD), // valimar2
    CustomShaderEntry(0x1355F463), // house light
    CustomShaderEntry(0x63462E18), // smoke
    CustomShaderEntry(0x0EA0D071), // sky
    CustomShaderEntry(0x728F5ED1), // sky
    CustomShaderEntry(0xE645FEED), // sky
    CustomShaderEntry(0xD589DF82), // sky
    CustomShaderEntry(0x523978D3), // lantern 5
    CustomShaderEntry(0xCDCBC858), // window light
    CustomShaderEntry(0xC7E8E2F6), // hbao
    CustomShaderEntry(0x036B0D74), // ao
    CustomShaderEntry(0x786C04E3), // fire 1
    CustomShaderEntry(0x1132F020), // fire 2
    
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
        .key = "SettingsBloom",
        .binding = &shader_injection.bloom,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 1.f,
        .can_reset = false,
        .label = "Game Bloom",
        .labels = {"Enabled (Approximated)", "Enabled", "Disabled"},
        
        .is_global = true,
        .is_visible = []() { return false; },
    },
    new renodx::utils::settings::Setting{
        .key = "SettingsSafeClamp",
        .binding = &shader_injection.safe_clamp,
        .default_value = 150.f,
        .label = "Safe Clamp",
        .tooltip = "Safe Clamp values for base rendering. Lower is safer but can clamp HDR values.",
        .min = 100.f,
        .max = 500.f,
        .parse = [](float value) { return value * 0.01f; },
        // .is_visible = []() { return false; },
        .is_global = true,
    },
    new renodx::utils::settings::Setting{
        .key = "SettingsBloomStrength",
        .binding = &shader_injection.bloom_strength,
        .default_value = 100.f,
        .label = "Bloom Strength",
        .tooltip = "Bloom Strength.",
        .min = 0.f,
        .max = 1000.f,
        .parse = [](float value) { return value * 0.01f; },
        // .is_visible = []() { return false; }
    },
    new renodx::utils::settings::Setting{
        .key = "SettingsBloomMethod",
        .binding = &shader_injection.bloom_approx_method,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f,
        .can_reset = false,
        .label = "Bloom Approximation",
        .labels = {"Perception", "Luminance"},
        .is_global = true,
        // .is_visible = []() { return shader_injection.bloom == 0.f && current_settings_mode >= 1; },
        .is_visible = []() { return false; },
    },
    new renodx::utils::settings::Setting{
        .key = "SettingsBloomSpace",
        .binding = &shader_injection.bloom_processing_space,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f,
        .can_reset = false,
        .label = "Bloom Processing Space",
        .labels = {"sRGB", "Linear"},
        // .is_visible = []() { return false; },
        .is_global = true,
        .is_visible = []() { return false; },
        // .is_visible = []() { return shader_injection.bloom == 0.f && current_settings_mode >= 1; },
        // .is_visible = []() { return false; },
    },
    // new renodx::utils::settings::Setting{
    //     .key = "SettingsBloomRescale",
    //     .binding = &shader_injection.bloom_rescale,
    //     .default_value = 0.f,
    //     .can_reset = false,
    //     .label = "Game Bloom Rescaling Factor",
    //     .tooltip = "Game Bloom.",
    //     .min = 0.f,
    //     .max = 500.f,
    //     .parse = [](float value) { return value * 0.01f; },
    // },
    // new renodx::utils::settings::Setting{
    //     .key = "SettingsAA",
    //     .binding = &shader_injection.fxaa,
    //     .value_type = renodx::utils::settings::SettingValueType::INTEGER,
    //     .default_value = 0.f,
    //     .can_reset = false,
    //     .label = "Game FXAA.",
    //     .labels = {"Disabled", "Enabled"},
    //     .is_global = true,
    // },
    new renodx::utils::settings::Setting{
        .key = "GammaCorrection",
        .binding = &shader_injection.gamma_correction,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 2.f,
        .label = "Gamma Correction",
        .section = "Tone Mapping",
        .tooltip = "Emulates a display EOTF.",
        .labels = {"Off", "2.2", "BT.1886", "Falcom (2.3)"},
        .is_visible = []() { return current_settings_mode >= 1; },
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
       .labels = {"Vanilla (Hue clipped)", "PsychoV", "RenoDRT"},
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
        .labels = {"Reinhard", "Hermite Spline", "Neutwo"},
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
    },
    
    // new renodx::utils::settings::Setting{
    //     .key = "ColorGradeExposure",
    //     .binding = &shader_injection.tone_map_exposure,
    //     .default_value = 1.f,
    //     .label = "Exposure",
    //     .section = "Color Grading",
    //     .max = 2.f,
    //     .format = "%.2f",
    //     .is_visible = []() { return shader_injection.tone_map_type  >= 1.f; },
    // },
    // new renodx::utils::settings::Setting{
    //     .key = "ColorGradeHighlights",
    //     .binding = &shader_injection.tone_map_highlights,
    //     .default_value = 50.f,
    //     .label = "Highlights",
    //     .section = "Color Grading",
    //     .max = 100.f,
    //     .parse = [](float value) { return value * 0.02f; },
    //     .is_visible = []() { return shader_injection.tone_map_type  >= 1.f; },
        
    // },
    // new renodx::utils::settings::Setting{
    //     .key = "ColorGradeShadows",
    //     .binding = &shader_injection.tone_map_shadows,
    //     .default_value = 50.f,
    //     .label = "Shadows",
    //     .section = "Color Grading",
    //     .max = 100.f,
    //     .parse = [](float value) { return value * 0.02f; },
    //     .is_visible = []() { return shader_injection.tone_map_type  >= 1.f; },
    // },
    new renodx::utils::settings::Setting{
        .key = "ColorGradeContrast",
        .binding = &shader_injection.tone_map_contrast,
        .default_value = 50.f,
        .label = "Contrast",
        .section = "Color Grading",
        .max = 100.f,
        
        .parse = [](float value) { return value * 0.02f; },
        .is_visible = []() { return shader_injection.tone_map_type  >= 1.f; },
    },
    new renodx::utils::settings::Setting{
        .key = "ColorGradeSaturation",
        .binding = &shader_injection.tone_map_saturation,
        .default_value = 50.f,
        .label = "Saturation",
        .section = "Color Grading",
        .max = 100.f,
        .parse = [](float value) { return value * 0.02f; },
        .is_visible = []() { return shader_injection.tone_map_type  >= 1.f; },
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
    // new renodx::utils::settings::Setting{
    //     .key = "IntermediateDecoding",
    //     .binding = &shader_injection.intermediate_encoding,
    //     .value_type = renodx::utils::settings::SettingValueType::INTEGER,
    //     .default_value = 0.f,
    //     .label = "Intermediate Encoding",
    //     .section = "Display Output",
    //     .labels = {"Auto", "None", "SRGB", "2.2", "2.4"},
    //     .is_enabled = []() { return shader_injection.tone_map_type >= 1; },
    //     .parse = [](float value) {
    //         if (value == 0) return shader_injection.gamma_correction + 1.f;
    //         return value - 1.f; },
    //     .is_visible = []() { return current_settings_mode >= 2; },
    // },
    // new renodx::utils::settings::Setting{
    //     .key = "SwapChainDecoding",
    //     .binding = &shader_injection.swap_chain_decoding,
    //     .value_type = renodx::utils::settings::SettingValueType::INTEGER,
    //     .default_value = 0.f,
    //     .label = "Swapchain Decoding",
    //     .section = "Display Output",
    //     .labels = {"Auto", "None", "SRGB", "2.2", "2.4"},
    //     .is_enabled = []() { return shader_injection.tone_map_type >= 1; },
    //     .parse = [](float value) {
    //         if (value == 0) return shader_injection.intermediate_encoding;
    //         return value - 1.f; },
    //     .is_visible = []() { return current_settings_mode >= 2; },
    // },
    new renodx::utils::settings::Setting{
        .key = "SwapChainGammaCorrection",
        .binding = &shader_injection.swap_chain_gamma_correction,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 2.f,
        .label = "Gamma Correction",
        .section = "Display Output",
        .labels = {"None", "2.2", "2.4"},
        .is_enabled = []() { return shader_injection.tone_map_type >= 1; },
        // .is_visible = []() { return current_settings_mode >= 2; },
        .is_visible = []() { return false; },
    },
    // new renodx::utils::settings::Setting{
    //     .key = "SwapChainClampColorSpace",
    //     .binding = &shader_injection.swap_chain_clamp_color_space,
    //     .value_type = renodx::utils::settings::SettingValueType::INTEGER,
    //     .default_value = 2.f,
    //     .label = "Clamp Color Space",
    //     .section = "Display Output",
    //     .labels = {"None", "BT709", "BT2020", "AP1"},
    //     .is_enabled = []() { return shader_injection.tone_map_type >= 1; },
    //     .parse = [](float value) { return value - 1.f; },
    //     .is_visible = []() { return current_settings_mode >= 2; },
    // },
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

extern "C" __declspec(dllexport) constexpr const char* NAME = "Sen no Kiseki - RenoDX";
extern "C" __declspec(dllexport) constexpr const char* DESCRIPTION = "RenoDX for Trails of Cold Steel games";

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
        
        renodx::mods::swapchain::swapchain_proxy_compatibility_mode = true;
        renodx::mods::swapchain::swapchain_proxy_revert_state = true;
        renodx::mods::swapchain::swap_chain_proxy_vertex_shader = __swap_chain_proxy_vertex_shader;
        renodx::mods::swapchain::swap_chain_proxy_pixel_shader = __swap_chain_proxy_pixel_shader;



        renodx::mods::swapchain::swap_chain_upgrade_targets.push_back({
            .old_format = reshade::api::format::r8g8b8a8_unorm,
            .new_format = reshade::api::format::r16g16b16a16_float,
            .use_resource_view_cloning = true,
            .aspect_ratio = renodx::mods::swapchain::SwapChainUpgradeTarget::BACK_BUFFER,
            // .ignore_size = true,
            .usage_include = reshade::api::resource_usage::render_target
        });
    
        // bool is_hdr10 = true;
        // renodx::mods::swapchain::SetUseHDR10(is_hdr10);
        // renodx::mods::swapchain::use_resize_buffer = false;
        // shader_injection.swap_chain_encoding = is_hdr10 ? 4.f : 5.f;
        // shader_injection.swap_chain_encoding_color_space = is_hdr10 ? 1.0f : 0.f;
        
        {
            // auto* setting = new renodx::utils::settings::Setting{
            //     .key = "SwapChainEncoding",
            //     .binding = &shader_injection.hdr_format,
            //     .value_type = renodx::utils::settings::SettingValueType::INTEGER,
            //     .default_value = 0.f,
            //     .label = "HDR Format",
            //     .section = "Display Output",
            //     .tooltip = "Sets the HDR format (HDR10 is compatible with Smooth Motion)",
            //     .labels = {"HDR10", "scRGB (default)"},
            //     .is_enabled = []() { return true; },
            //     .is_global = true,
            //     .is_visible = []() { return current_settings_mode >= 2; },
            // };

            // renodx::utils::settings::LoadSetting(renodx::utils::settings::global_name, setting);
            shader_injection.hdr_format = 0.f;
            bool is_hdr10 = true;
            renodx::mods::swapchain::SetUseHDR10(is_hdr10);
            renodx::mods::swapchain::use_resize_buffer = false;
            shader_injection.swap_chain_encoding = (is_hdr10 ? 4.f : 5.f);
            shader_injection.swap_chain_encoding_color_space = is_hdr10 ? 1.f : 0.f;
            // settings.push_back(setting);
        }

        initialized = true;
      }

      break;
    case DLL_PROCESS_DETACH:
      reshade::unregister_addon(h_module);
      break;
  }

  renodx::utils::settings::Use(fdw_reason, &settings, &OnPresetOff);
  renodx::mods::swapchain::Use(fdw_reason, &shader_injection);

  custom_shaders.insert(artifact_shaders.begin(), artifact_shaders.end());
  renodx::mods::shader::Use(fdw_reason, custom_shaders, &shader_injection);

  return TRUE;
}
