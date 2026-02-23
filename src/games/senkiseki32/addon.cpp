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

constexpr uint32_t kUiKanjiShader = 0x976FCD64u;
constexpr uint32_t kUiKanjiFallbackVertexShader = 0x3D5970FEu;  // Logged variant candidate for skill-kanji fill
constexpr uint32_t kUiKanjiSourceTextureSize = 1024u;

struct UiKanjiDrawState {
  reshade::api::device* device = nullptr;
  reshade::api::pipeline original_output_merger = {0u};
  reshade::api::pipeline override_output_merger = {0u};
  bool active = false;
};

struct UiKanjiDrawCallInfo {
  reshade::api::command_list* cmd_list = nullptr;
  bool valid = false;
  bool indexed = false;
  uint32_t draw_count = 0u;
  uint32_t instance_count = 0u;
  uint32_t first_value = 0u;
  int32_t vertex_offset = 0;
  uint32_t first_instance = 0u;
};

struct UiKanjiSourceTextureInfo {
  reshade::api::command_list* cmd_list = nullptr;
  bool valid = false;
  uint32_t width = 0u;
  uint32_t height = 0u;
};

thread_local UiKanjiDrawState ui_kanji_draw_state = {};
thread_local UiKanjiDrawCallInfo ui_kanji_draw_call_info = {};
thread_local UiKanjiSourceTextureInfo ui_kanji_source_texture_info = {};

bool IsLikelyUiKanjiSourceTexture(const reshade::api::command_list* cmd_list) {
  if (cmd_list == nullptr) return false;
  if (!ui_kanji_source_texture_info.valid) return false;
  if (ui_kanji_source_texture_info.cmd_list != cmd_list) return false;

  return ui_kanji_source_texture_info.width == kUiKanjiSourceTextureSize
         && ui_kanji_source_texture_info.height == kUiKanjiSourceTextureSize;
}

bool HasUiKanjiSourceTextureInfo(const reshade::api::command_list* cmd_list) {
  return cmd_list != nullptr
         && ui_kanji_source_texture_info.valid
         && ui_kanji_source_texture_info.cmd_list == cmd_list;
}

bool IsLikelyUiKanjiDrawCall(const reshade::api::command_list* cmd_list) {
  if (cmd_list == nullptr) return false;
  if (!ui_kanji_draw_call_info.valid) return false;
  if (ui_kanji_draw_call_info.cmd_list != cmd_list) return false;

  return !ui_kanji_draw_call_info.indexed
         && ui_kanji_draw_call_info.draw_count == 4u
         && ui_kanji_draw_call_info.instance_count == 1u
         && ui_kanji_draw_call_info.first_value == 0u
         && ui_kanji_draw_call_info.vertex_offset == 0
         && ui_kanji_draw_call_info.first_instance == 0u;
}

bool TryGetResourceViewFromDescriptorUpdate(
    const reshade::api::descriptor_table_update& update,
    uint32_t index,
    reshade::api::resource_view* out_view) {
  if (out_view == nullptr) return false;
  if (index >= update.count || update.descriptors == nullptr) return false;

  switch (update.type) {
    case reshade::api::descriptor_type::sampler_with_resource_view: {
      const auto item = static_cast<const reshade::api::sampler_with_resource_view*>(update.descriptors)[index];
      *out_view = item.view;
      return out_view->handle != 0u;
    }
    case reshade::api::descriptor_type::texture_shader_resource_view:
    case reshade::api::descriptor_type::buffer_shader_resource_view:
    case reshade::api::descriptor_type::texture_unordered_access_view:
    case reshade::api::descriptor_type::buffer_unordered_access_view:
    case reshade::api::descriptor_type::acceleration_structure: {
      *out_view = static_cast<const reshade::api::resource_view*>(update.descriptors)[index];
      return out_view->handle != 0u;
    }
    default:
      return false;
  }
}

void OnPushDescriptorsUiKanji(
    reshade::api::command_list* cmd_list,
    reshade::api::shader_stage stages,
    reshade::api::pipeline_layout layout,
    uint32_t layout_param,
    const reshade::api::descriptor_table_update& update) {
  (void)layout;
  (void)layout_param;
  if (cmd_list == nullptr) return;
  auto* shader_state = renodx::utils::shader::GetCurrentState(cmd_list);
  if (shader_state == nullptr) return;
  if (renodx::utils::shader::GetCurrentPixelShaderHash(shader_state) != kUiKanjiShader) return;

  constexpr uint32_t kPixelStageMask = static_cast<uint32_t>(reshade::api::shader_stage::pixel);
  if ((static_cast<uint32_t>(stages) & kPixelStageMask) == 0u) return;

  for (uint32_t i = 0u; i < update.count; ++i) {
    const uint32_t binding = update.binding + i;
    if (binding != 0u) continue;

    reshade::api::resource_view view = {0u};
    if (!TryGetResourceViewFromDescriptorUpdate(update, i, &view)) continue;

    auto* view_info = renodx::utils::resource::GetResourceViewInfo(view);
    if (view_info == nullptr || view_info->resource_info == nullptr) continue;

    const uint32_t width = view_info->resource_info->desc.texture.width;
    const uint32_t height = view_info->resource_info->desc.texture.height;
    ui_kanji_source_texture_info = {
        .cmd_list = cmd_list,
        .valid = true,
        .width = width,
        .height = height,
    };
  }
}

bool OnDrawUiKanji(
    reshade::api::command_list* cmd_list,
    uint32_t vertex_count,
    uint32_t instance_count,
    uint32_t first_vertex,
    uint32_t first_instance) {
  auto* shader_state = renodx::utils::shader::GetCurrentState(cmd_list);
  if (shader_state == nullptr || renodx::utils::shader::GetCurrentPixelShaderHash(shader_state) != kUiKanjiShader) {
    ui_kanji_draw_call_info = {};
    return false;
  }

  ui_kanji_draw_call_info = {
      .cmd_list = cmd_list,
      .valid = true,
      .indexed = false,
      .draw_count = vertex_count,
      .instance_count = instance_count,
      .first_value = first_vertex,
      .vertex_offset = 0,
      .first_instance = first_instance,
  };
  return false;
}

bool OnDrawIndexedUiKanji(
    reshade::api::command_list* cmd_list,
    uint32_t index_count,
    uint32_t instance_count,
    uint32_t first_index,
    int32_t vertex_offset,
    uint32_t first_instance) {
  auto* shader_state = renodx::utils::shader::GetCurrentState(cmd_list);
  if (shader_state == nullptr || renodx::utils::shader::GetCurrentPixelShaderHash(shader_state) != kUiKanjiShader) {
    ui_kanji_draw_call_info = {};
    return false;
  }

  ui_kanji_draw_call_info = {
      .cmd_list = cmd_list,
      .valid = true,
      .indexed = true,
      .draw_count = index_count,
      .instance_count = instance_count,
      .first_value = first_index,
      .vertex_offset = vertex_offset,
      .first_instance = first_instance,
  };
  return false;
}

bool IsDestAlphaBlendFactor(reshade::api::blend_factor factor) {
  return factor == reshade::api::blend_factor::dest_alpha
         || factor == reshade::api::blend_factor::one_minus_dest_alpha;
}

bool TryBuildUiKanjiBlendOverride(
    const renodx::utils::shader::PipelineShaderDetails* pipeline_details,
    reshade::api::blend_desc* out_blend_state,
    bool* out_disable_depth_stencil) {
  if (pipeline_details == nullptr || out_blend_state == nullptr || out_disable_depth_stencil == nullptr) return false;

  const reshade::api::blend_desc* source_blend_state = nullptr;
  for (const auto& subobject : pipeline_details->subobjects) {
    if (subobject.type != reshade::api::pipeline_subobject_type::blend_state) continue;
    if (subobject.count == 0u || subobject.data == nullptr) continue;
    source_blend_state = static_cast<const reshade::api::blend_desc*>(subobject.data);
    break;
  }
  if (source_blend_state == nullptr) return false;

  *out_blend_state = source_blend_state[0];
  *out_disable_depth_stencil = false;

  bool changed = false;
  for (uint32_t rt = 0u; rt < 8u; ++rt) {
    const bool blend_enabled = out_blend_state->blend_enable[rt];

    const bool uses_dest_alpha_in_color =
        IsDestAlphaBlendFactor(out_blend_state->source_color_blend_factor[rt])
        || IsDestAlphaBlendFactor(out_blend_state->dest_color_blend_factor[rt]);
    const bool writes_alpha = (out_blend_state->render_target_write_mask[rt] & 0x8u) != 0u;
    const bool needs_alpha_blend_promote = !blend_enabled;

    if (uses_dest_alpha_in_color || needs_alpha_blend_promote) {
      if (needs_alpha_blend_promote) {
        *out_disable_depth_stencil = true;
      }
      out_blend_state->blend_enable[rt] = true;
      out_blend_state->logic_op_enable[rt] = false;
      out_blend_state->source_color_blend_factor[rt] = reshade::api::blend_factor::source_alpha;
      out_blend_state->dest_color_blend_factor[rt] = reshade::api::blend_factor::one_minus_source_alpha;
      out_blend_state->color_blend_op[rt] = reshade::api::blend_op::add;
      changed = true;
    }
    if (writes_alpha) {
      out_blend_state->source_alpha_blend_factor[rt] = reshade::api::blend_factor::zero;
      out_blend_state->dest_alpha_blend_factor[rt] = reshade::api::blend_factor::one;
      out_blend_state->alpha_blend_op[rt] = reshade::api::blend_op::add;
      out_blend_state->render_target_write_mask[rt] &= 0x7u;  // Preserve destination alpha
      changed = true;
    }
  }

  return changed;
}

bool ForceUiKanjiOutputMergerState(reshade::api::command_list* cmd_list) {
  ui_kanji_draw_state = {};

  auto* shader_state = renodx::utils::shader::GetCurrentState(cmd_list);
  if (shader_state == nullptr) return true;

  // Ensure pipeline details are populated.
  (void)renodx::utils::shader::GetCurrentPixelShaderHash(shader_state);

  auto* pixel_state = renodx::utils::shader::GetCurrentPixelState(shader_state);
  if (pixel_state == nullptr || pixel_state->pipeline.handle == 0u || pixel_state->pipeline_details == nullptr) {
    return true;
  }

  auto* device = cmd_list->get_device();
  if (device == nullptr) return true;

  const uint32_t vertex_hash = renodx::utils::shader::GetCurrentVertexShaderHash(shader_state);
  if (vertex_hash != kUiKanjiFallbackVertexShader) return true;
  if (!IsLikelyUiKanjiDrawCall(cmd_list)) return true;
  if (HasUiKanjiSourceTextureInfo(cmd_list) && !IsLikelyUiKanjiSourceTexture(cmd_list)) return true;

  reshade::api::blend_desc blend_state = {};
  bool disable_depth_stencil = false;
  if (!TryBuildUiKanjiBlendOverride(pixel_state->pipeline_details, &blend_state, &disable_depth_stencil)) {
    // Fallback for variants without an explicit blend subobject:
    // use known-good kanji blend state and disable depth/stencil.
    blend_state = {};
    blend_state.blend_enable[0] = true;
    blend_state.logic_op_enable[0] = false;
    blend_state.source_color_blend_factor[0] = reshade::api::blend_factor::source_alpha;
    blend_state.dest_color_blend_factor[0] = reshade::api::blend_factor::one_minus_source_alpha;
    blend_state.color_blend_op[0] = reshade::api::blend_op::add;
    blend_state.source_alpha_blend_factor[0] = reshade::api::blend_factor::zero;
    blend_state.dest_alpha_blend_factor[0] = reshade::api::blend_factor::one;
    blend_state.alpha_blend_op[0] = reshade::api::blend_op::add;
    blend_state.render_target_write_mask[0] = 0x7u;  // Preserve destination alpha for kanji fill.
    disable_depth_stencil = true;
  }

  reshade::api::depth_stencil_desc depth_stencil_state = {};
  depth_stencil_state.depth_enable = false;
  depth_stencil_state.depth_write_mask = false;
  depth_stencil_state.depth_func = reshade::api::compare_op::always;
  depth_stencil_state.stencil_enable = false;

  reshade::api::pipeline_subobject subobjects[2] = {};
  subobjects[0] = {
      reshade::api::pipeline_subobject_type::blend_state,
      1,
      &blend_state,
  };
  uint32_t subobject_count = 1u;
  if (disable_depth_stencil) {
    subobjects[1] = {
        reshade::api::pipeline_subobject_type::depth_stencil_state,
        1,
        &depth_stencil_state,
    };
    subobject_count = 2u;
  }

  reshade::api::pipeline override_pipeline = {0u};
  if (!device->create_pipeline(
          pixel_state->pipeline_details->layout,
          subobject_count,
          subobjects,
          &override_pipeline)) {
    return true;
  }

  ui_kanji_draw_state.device = device;
  ui_kanji_draw_state.original_output_merger = pixel_state->pipeline;
  ui_kanji_draw_state.override_output_merger = override_pipeline;
  ui_kanji_draw_state.active = true;

  cmd_list->bind_pipeline(
      reshade::api::pipeline_stage::output_merger,
      ui_kanji_draw_state.override_output_merger);

  return true;
}

void RestoreUiKanjiOutputMergerState(reshade::api::command_list* cmd_list) {
  if (!ui_kanji_draw_state.active) return;

  if (ui_kanji_draw_state.original_output_merger.handle != 0u) {
    cmd_list->bind_pipeline(
        reshade::api::pipeline_stage::output_merger,
        ui_kanji_draw_state.original_output_merger);
  }

  if (ui_kanji_draw_state.device != nullptr && ui_kanji_draw_state.override_output_merger.handle != 0u) {
    ui_kanji_draw_state.device->destroy_pipeline(ui_kanji_draw_state.override_output_merger);
  }

  ui_kanji_draw_state = {};
}

void OnDestroyPipelineUiKanjiState(
    reshade::api::device* device,
    reshade::api::pipeline pipeline) {
  (void)device;
  (void)pipeline;
}

void OnDestroyDeviceUiKanjiState(reshade::api::device* device) {
  if (ui_kanji_draw_state.active && ui_kanji_draw_state.device == device) {
    ui_kanji_draw_state = {};
  }
}


renodx::mods::shader::CustomShaders custom_shaders = {
    // post shaders
    CustomShaderEntry(0x06CB76E4),  // post with glow, filter
    CustomShaderEntry(0x16133C77),  // post with glow, filter, DOF, fade texture
    CustomShaderEntry(0x51F5B458),  // post with glow, filter, monotone, fade texture
    CustomShaderEntry(0x59CCF2EE),  // post with glow
    CustomShaderEntry(0x7C32F088),  // post with glow, monotone, fade color
    CustomShaderEntry(0x7ED31B40),  // post with glow, filter, DOF
    CustomShaderEntry(0x87D5AC74),  // post with glow, filter, fade texture
    CustomShaderEntry(0xAEC8D6B4),  // post with glow, filter, fade color
    CustomShaderEntry(0xC3E7EB5F),  // post with glow, monotone
    CustomShaderEntry(0xCCA14A99),  // post with glow, filter, monotone
    CustomShaderEntry(0xE1D2B099),  // post with glow, fade color
    CustomShaderEntry(0xE59175BE),  // post with glow, fade texture
    CustomShaderEntry(0xEDB16A32),  // post with glow, monotone, fade texture
    CustomShaderEntry(0xF9769AB8),  // post with glow, filter, monotone, fade color
    // UI shaders (saturate output for R8->R16 upgrade)
    CustomShaderEntry(0xA8859457),  // UI: tex * vertex color + specular
    {
        kUiKanjiShader,
        {
            .crc32 = kUiKanjiShader,
            .code = __0x976FCD64,
            .on_draw = &ForceUiKanjiOutputMergerState,
            .on_drawn = &RestoreUiKanjiOutputMergerState,
        },
    },                              // UI: tex * vertex color + specular + alpha threshold (fix blend usage; disable depth/stencil only when promoting non-blended variant)
    CustomShaderEntry(0x1EE56570),  // UI: tex * vertex color + specular + depth alpha
    CustomShaderEntry(0x91F42481),  // UI: minimap
    CustomShaderEntry(0x328E747D),  // output: color buffer passthrough (gamma disabled)
    // effect shaders
    CustomShaderEntry(0x928A59DA),  // effect: diffuse material, lighting, alpha test
    // glow shaders
    CustomShaderEntry(0x10F92CFA),  // glow: gaussian blur (5-tap weighted)
    // godray shaders
    CustomShaderEntry(0x6D8F8A97),  // godray: depth extraction (texture array + depth)
    //AO
    CustomShaderEntry(0x378D40E6),  // AO: screen space ambient occlusion
    // __ALL_CUSTOM_SHADERS
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
        .key = "ToneMapType",
        .binding = &shader_injection.tone_map_type,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 2.f,
        .can_reset = true,
        .label = "Tone Mapper",
        .section = "Tone Mapping",
        .tooltip = "Sets the tone mapper type",
        .labels = {"Vanilla", "DICE", "RenoDRT"},
        .is_visible = []() { return current_settings_mode >= 1; },
    },
    new renodx::utils::settings::Setting{
        .key = "DiceToneMapType",
        .binding = &shader_injection.dice_tone_map_type,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 2.f,
        .label = "DICE Tonemap Type",
        .section = "Tonemapping Config",
        .tooltip = "Selects the DICE tonemapping method.",
        .labels = {"Luminance PQ", "Luminance PQ (Corrected)", "Per Channel PQ"},
        .is_visible = []() { return shader_injection.tone_map_type == 1; },
    },
    new renodx::utils::settings::Setting{
        .key = "DiceShoulderStart",
        .binding = &shader_injection.dice_shoulder_start,
        .default_value = 0.25f,
        .label = "DICE Shoulder Start",
        .section = "Tonemapping Config",
        .tooltip = "Determines where the highlights curve (shoulder) starts.",
        .max = 1.f,
        .format = "%.2f",
        .is_visible = []() { return shader_injection.tone_map_type == 1; },
    },
    new renodx::utils::settings::Setting{
        .key = "DiceDesaturation",
        .binding = &shader_injection.dice_desaturation,
        .default_value = 0.33f,
        .label = "DICE Desaturation",
        .section = "Tonemapping Config",
        .tooltip = "Controls desaturation of out-of-range colors. Only used with Luminance PQ (Corrected).",
        .max = 1.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.dice_tone_map_type == 1; },
        .is_visible = []() { return shader_injection.tone_map_type == 1; },
    },
    new renodx::utils::settings::Setting{
        .key = "DiceDarkening",
        .binding = &shader_injection.dice_darkening,
        .default_value = 0.33f,
        .label = "DICE Darkening",
        .section = "Tonemapping Config",
        .tooltip = "Controls brightness reduction of out-of-range colors. Only used with Luminance PQ (Corrected).",
        .max = 1.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.dice_tone_map_type == 1; },
        .is_visible = []() { return shader_injection.tone_map_type == 1; },
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
        .key = "GammaCorrection",
        .binding = &shader_injection.gamma_correction,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 3.f,
        .label = "Gamma Correction",
        .section = "Tone Mapping",
        .tooltip = "Emulates a display EOTF.",
        .labels = {"Off", "2.2", "BT.1886", "Falcom (2.3)"},
        .on_change_value = [](float previous, float current) {
            (void)previous;
            shader_injection.swap_chain_gamma_correction = current;
            shader_injection.intermediate_encoding = (current == 3.f) ? 2.f : (current + 1.f);
            shader_injection.swap_chain_decoding = shader_injection.intermediate_encoding;
        },
        .is_visible = []() { return current_settings_mode >= 1; },
    },
    new renodx::utils::settings::Setting{
        .key = "ToneMapHueCorrection",
        .binding = &shader_injection.tone_map_hue_correction,
        .default_value = 75.f,
        .label = "Hue Correction",
        .section = "Tone Mapping",
        .tooltip = "Hue retention strength.",
        .min = 0.f,
        .max = 100.f,
        .is_enabled = []() { return shader_injection.tone_map_type >= 1; },
        .parse = [](float value) { return value * 0.01f; },
        .is_visible = []() { return current_settings_mode >= 2; },
    },
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
        .is_visible = []() { return current_settings_mode >= 1; },
    },
    new renodx::utils::settings::Setting{
        .key = "ColorGradeShadows",
        .binding = &shader_injection.tone_map_shadows,
        .default_value = 50.f,
        .label = "Shadows",
        .section = "Color Grading",
        .max = 100.f,
        .parse = [](float value) { return value * 0.02f; },
        .is_visible = []() { return current_settings_mode >= 1; },
    },
    new renodx::utils::settings::Setting{
        .key = "ColorGradeContrast",
        .binding = &shader_injection.tone_map_contrast,
        .default_value = 50.f,
        .label = "Contrast",
        .section = "Color Grading",
        .max = 100.f,
        .parse = [](float value) { return value * 0.02f; },
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
        .is_visible = []() { return current_settings_mode >= 1; },
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
    },
    new renodx::utils::settings::Setting{
        .key = "ColorGradeFlare",
        .binding = &shader_injection.tone_map_flare,
        .default_value = 0.f,
        .label = "Flare",
        .section = "Color Grading",
        .tooltip = "Flare/Glare Compensation",
        .max = 100.f,
        .is_enabled = []() { return shader_injection.tone_map_type == 2; },
        .parse = [](float value) { return value * 0.02f; },
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
        .is_visible = []() { return settings[0]->GetValue() >= 1; },
    },
    new renodx::utils::settings::Setting{
        .key = "RenderingGIEnabled",
        .binding = &shader_injection.gi_enabled,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .can_reset = true,
        .label = "Enable GI",
        .section = "Rendering",
        .tooltip = "Toggles fake Global Illumination (color bleeding into shadows)",
    },
    new renodx::utils::settings::Setting{
        .key = "RenderingGIIntensity",
        .binding = &shader_injection.gi_intensity,
        .default_value = 50.f,
        .label = "GI Intensity",
        .section = "Rendering",
        .tooltip = "Strength of the fake global illumination (color bleeding into shadows)",
        .max = 500.f,
        .is_enabled = []() { return shader_injection.gi_enabled != 0.f; },
        .parse = [](float value) { return value * 0.01f; },
    },
    new renodx::utils::settings::Setting{
        .key = "RenderingShadowSaturation",
        .binding = &shader_injection.shadow_saturation,
        .default_value = 200.f,
        .label = "Shadow Saturation",
        .section = "Rendering",
        .tooltip = "Saturation boost in shadowed areas to mimic subsurface scattering",
        .max = 300.f,
        .is_enabled = []() { return shader_injection.gi_enabled != 0.f; },
        .parse = [](float value) { return value * 0.01f; },
    },
    new renodx::utils::settings::Setting{
        .key = "RenderingAOEnabled",
        .binding = &shader_injection.ao_enabled,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .can_reset = true,
        .label = "Enable AO",
        .section = "Rendering",
        .tooltip = "Toggles custom Ambient Occlusion power curve",
    },
    new renodx::utils::settings::Setting{
        .key = "RenderingAOPower",
        .binding = &shader_injection.ao_power,
        .default_value = 100.f,
        .label = "AO Power",
        .section = "Rendering",
        .tooltip = "Controls the strength/contrast of Ambient Occlusion. Higher = darker shadows",
        .max = 300.f,
        .is_enabled = []() { return shader_injection.ao_enabled != 0.f; },
        .parse = [](float value) { return value * 0.01f; },
    },
    new renodx::utils::settings::Setting{
        .key = "DebugDisableFilter",
        .binding = &shader_injection.debug_disable_filter,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f,
        .can_reset = true,
        .label = "Disable Filter",
        .section = "Debug",
        .tooltip = "Disables the color filter overlay for debugging",
    },
    new renodx::utils::settings::Setting{
        .key = "DebugDisableFading",
        .binding = &shader_injection.debug_disable_fading,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f,
        .can_reset = true,
        .label = "Disable Fading",
        .section = "Debug",
        .tooltip = "Disables the fading texture/color overlay for debugging",
    },
};

void OnPresetOff() {
     renodx::utils::settings::UpdateSetting("toneMapType", 0.f);
     renodx::utils::settings::UpdateSetting("toneMapPeakNits", 203.f);
     renodx::utils::settings::UpdateSetting("toneMapGameNits", 203.f);
     renodx::utils::settings::UpdateSetting("toneMapUINits", 203.f);
  //   renodx::utils::settings::UpdateSetting("toneMapGammaCorrection", 0);
     renodx::utils::settings::UpdateSetting("colorGradeExposure", 1.f);
     renodx::utils::settings::UpdateSetting("colorGradeHighlights", 50.f);
     renodx::utils::settings::UpdateSetting("colorGradeShadows", 50.f);
     renodx::utils::settings::UpdateSetting("colorGradeContrast", 50.f);
     renodx::utils::settings::UpdateSetting("colorGradeSaturation", 50.f);
  //   renodx::utils::settings::UpdateSetting("colorGradeLUTStrength", 100.f);
  //   renodx::utils::settings::UpdateSetting("colorGradeLUTScaling", 0.f);
}

void OnPresent(reshade::api::command_queue* queue,
               reshade::api::swapchain* swapchain,
               const reshade::api::rect* source_rect,
               const reshade::api::rect* dest_rect,
               uint32_t dirty_rect_count,
               const reshade::api::rect* dirty_rects) {
  auto* device = queue->get_device();
  if (device->get_api() == reshade::api::device_api::opengl) {
    shader_injection.custom_flip_uv_y = 1.f;
  }
}

bool initialized = false;

}  // namespace

extern "C" __declspec(dllexport) constexpr const char* NAME = "RenoDX";
extern "C" __declspec(dllexport) constexpr const char* DESCRIPTION = "Trails of Cold Steel Addon Slopped by Toru";

BOOL APIENTRY DllMain(HMODULE h_module, DWORD fdw_reason, LPVOID lpv_reserved) {
  switch (fdw_reason) {
    case DLL_PROCESS_ATTACH:
      if (!reshade::register_addon(h_module)) return FALSE;

      reshade::register_event<reshade::addon_event::destroy_pipeline>(OnDestroyPipelineUiKanjiState);
      reshade::register_event<reshade::addon_event::destroy_device>(OnDestroyDeviceUiKanjiState);
      reshade::register_event<reshade::addon_event::draw>(OnDrawUiKanji);
      reshade::register_event<reshade::addon_event::draw_indexed>(OnDrawIndexedUiKanji);
      reshade::register_event<reshade::addon_event::push_descriptors>(OnPushDescriptorsUiKanji);

      if (!initialized) {
        renodx::mods::shader::force_pipeline_cloning = true;
        renodx::mods::shader::expected_constant_buffer_space = 50;
        renodx::mods::shader::expected_constant_buffer_index = 13;
        renodx::mods::shader::allow_multiple_push_constants = true;

        renodx::mods::swapchain::expected_constant_buffer_index = 13;
        renodx::mods::swapchain::expected_constant_buffer_space = 50;
        renodx::mods::swapchain::use_resource_cloning = true;
        renodx::mods::swapchain::swap_chain_proxy_shaders = {
            {
                reshade::api::device_api::d3d11,
                {
                    .vertex_shader = __swap_chain_proxy_vertex_shader_dx11,
                    .pixel_shader = __swap_chain_proxy_pixel_shader_dx11,
                },
            },
            {
                reshade::api::device_api::d3d12,
                {
                    .vertex_shader = __swap_chain_proxy_vertex_shader_dx12,
                    .pixel_shader = __swap_chain_proxy_pixel_shader_dx12,
                },
            },
        };

        {
          auto* setting = new renodx::utils::settings::Setting{
              .key = "SwapChainEncoding",
              .binding = &shader_injection.swap_chain_encoding,
              .value_type = renodx::utils::settings::SettingValueType::INTEGER,
              .default_value = 5.f,
              .label = "Encoding",
              .section = "Display Output",
              .labels = {"None", "SRGB", "2.2", "2.4", "HDR10", "scRGB"},
              .is_enabled = []() { return shader_injection.tone_map_type >= 1; },
              .on_change_value = [](float previous, float current) {
                bool is_hdr10 = current == 4;
                shader_injection.swap_chain_encoding_color_space = (is_hdr10 ? 1.f : 0.f);
                // return void
              },
              .is_global = true,
              .is_visible = []() { return current_settings_mode >= 2; },
          };
          renodx::utils::settings::LoadSetting(renodx::utils::settings::global_name, setting);
          bool is_hdr10 = setting->GetValue() == 4;
          renodx::mods::swapchain::SetUseHDR10(is_hdr10);
          renodx::mods::swapchain::use_resize_buffer = setting->GetValue() < 4;
          shader_injection.swap_chain_encoding_color_space = is_hdr10 ? 1.f : 0.f;
          settings.push_back(setting);
        }

        renodx::mods::swapchain::force_borderless = false;
        renodx::mods::swapchain::prevent_full_screen = false;
        renodx::mods::swapchain::use_device_proxy = false;
        renodx::mods::swapchain::set_color_space = true;
        renodx::mods::swapchain::device_proxy_wait_idle_source = false;
        renodx::mods::swapchain::device_proxy_wait_idle_destination = false;
        shader_injection.custom_flip_uv_y = 0.f;

        initialized = true;
      }

      // Resource Upgrade
      renodx::mods::swapchain::swap_chain_upgrade_targets.push_back({
          .old_format = reshade::api::format::r8g8b8a8_unorm,
          .new_format = reshade::api::format::r16g16b16a16_float,
          .use_resource_view_cloning = true,
          .aspect_ratio = renodx::mods::swapchain::SwapChainUpgradeTarget::BACK_BUFFER,
          .usage_include = reshade::api::resource_usage::render_target,
      });

      break;
    case DLL_PROCESS_DETACH:
      reshade::unregister_event<reshade::addon_event::destroy_pipeline>(OnDestroyPipelineUiKanjiState);
      reshade::unregister_event<reshade::addon_event::destroy_device>(OnDestroyDeviceUiKanjiState);
      reshade::unregister_event<reshade::addon_event::draw>(OnDrawUiKanji);
      reshade::unregister_event<reshade::addon_event::draw_indexed>(OnDrawIndexedUiKanji);
      reshade::unregister_event<reshade::addon_event::push_descriptors>(OnPushDescriptorsUiKanji);
      reshade::unregister_event<reshade::addon_event::present>(OnPresent);
      reshade::unregister_addon(h_module);
      break;
  }

  renodx::utils::settings::Use(fdw_reason, &settings, &OnPresetOff);
  if (fdw_reason == DLL_PROCESS_ATTACH) {
    shader_injection.tone_map_per_channel = 0.f;  // Force luminance scaling.
    shader_injection.tone_map_clamp_color_space = -1.f;  // Keep clamp disabled.
    shader_injection.tone_map_clamp_peak = -1.f;         // Keep peak clamp disabled.
    shader_injection.color_grade_strength = 1.f;         // Keep scene grading behavior with no UI control.
    shader_injection.swap_chain_gamma_correction = shader_injection.gamma_correction;
    shader_injection.swap_chain_clamp_color_space = 1.f;  // Force display clamp to BT2020.

    // Preserve old "Auto" behavior after removing encoding/decoding settings.
    shader_injection.intermediate_encoding =
        (shader_injection.gamma_correction == 3.f) ? 2.f : (shader_injection.gamma_correction + 1.f);
    shader_injection.swap_chain_decoding = shader_injection.intermediate_encoding;
  }
  renodx::mods::swapchain::Use(fdw_reason, &shader_injection);
  renodx::mods::shader::Use(fdw_reason, custom_shaders, &shader_injection);

  return TRUE;
}
