/*
 * Copyright (C) 2026
 * SPDX-License-Identifier: MIT
 */

#define ImTextureID ImU64

#define DEBUG_LEVEL_0

#include <atomic>
#include <span>
#include <vector>

#include <deps/imgui/imgui.h>
#include <include/reshade.hpp>

#include <embed/shaders.h>

#include "../../mods/shader.hpp"
#include "../../utils/descriptor.hpp"
#include "../../utils/pipeline_layout.hpp"
#include "../../utils/render.hpp"
#include "../../utils/resource.hpp"
#include "../../utils/settings.hpp"
#include "../../utils/shader.hpp"
#include "../../utils/swapchain.hpp"
#include "./kai-vanillaplus.h"

namespace {

constexpr uint32_t kLightingShader = 0x430ED091u;
constexpr uint32_t kCharacterShader = 0x445A1838u;

void OnLightingShaderDrawnApplyCharacterSSGI(reshade::api::command_list* cmd_list);

renodx::mods::shader::CustomShaders custom_shaders = {
    {
        kLightingShader,
        {
            .crc32 = kLightingShader,
            .code = __0x430ED091,
            .on_drawn = &OnLightingShaderDrawnApplyCharacterSSGI,
        },
    },                                             // lighting
    CustomShaderEntry(0x445A1838),                 // character lighting
    CustomShaderEntry(0x209125C1),                 // SSR
    CustomShaderEntry(0xB1CCBCAE),                 // glass
    CustomShaderEntry(0xE1E0ACBB),                 // glass
    CustomShaderEntry(0xF237E72F),                 // glass
};

SssInjectData shader_injection = {
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

float char_ssgi_composite_enabled = 1.f;

struct CharacterGiCompositeData {
  float strength = 3.0f;
  float alpha_scale = 0.75f;
  float chroma_strength = 0.50f;
  float luma_strength = 0.0f;
  float shadow_power = 1.25f;
  float headroom_power = 1.25f;
  float max_add = 0.020f;
  float dark_boost = 0.50f;
  float debug_mode = 0.f;
  float debug_scale = 1.f;
  float debug_chars_only = 1.f;
  float bright_boost = 3.0f;
  float peak_luma_cap = 0.0f;
  float depth_reject = 2.0f;
  float normal_reject = 0.15f;
  float ao_influence = 0.66f;
  float reject_strength = 8.0f;
  float _reserved0 = 0.0f;
  float _reserved1 = 0.0f;
  float _reserved2 = 0.0f;
};
static_assert(sizeof(CharacterGiCompositeData) == sizeof(float) * 20);

CharacterGiCompositeData character_gi_composite_data = {};

std::atomic_uint64_t g_lighting_ssgi_view{0u};
std::atomic_uint64_t g_lighting_mrt0_view{0u};
std::atomic_uint64_t g_lighting_mrt1_view{0u};
std::atomic_uint64_t g_lighting_depth_view{0u};
std::atomic_uint64_t g_lighting_ssao_view{0u};
std::atomic_uint64_t g_character_mrt0_view{0u};
std::atomic_uint64_t g_character_depth_view{0u};
std::atomic_uint64_t g_present_frame_index{0u};
std::atomic_uint64_t g_last_composite_frame_index{static_cast<uint64_t>(-1)};

struct __declspec(uuid("1f4fe7b8-271a-4d04-af0d-a52578061ef1")) DeviceData {
  reshade::api::resource composite_source_texture = {};
  reshade::api::resource_view composite_source_texture_view = {};
  reshade::api::resource_desc composite_source_desc = {};
  renodx::utils::render::RenderPass composite_pass = {};
};

bool TryResolveTextureRegister(
    reshade::api::pipeline_layout layout,
    uint32_t layout_param,
    uint32_t binding,
    uint32_t* out_register,
    uint32_t* out_space) {
  if (out_register == nullptr || out_space == nullptr) return false;
  auto* layout_data = renodx::utils::pipeline_layout::GetPipelineLayoutData(layout);
  if (layout_data == nullptr) return false;
  if (layout_param >= layout_data->params.size()) return false;

  const auto& param = layout_data->params.at(layout_param);
  auto resolve_range = [binding, out_register, out_space](const reshade::api::descriptor_range& range) -> bool {
    if (range.count == UINT32_MAX) return false;
    if (binding < range.binding || binding >= (range.binding + range.count)) return false;
    *out_register = range.dx_register_index + (binding - range.binding);
    *out_space = range.dx_register_space;
    return true;
  };

  switch (param.type) {
    case reshade::api::pipeline_layout_param_type::push_descriptors: {
      return resolve_range(param.push_descriptors);
    }
    case reshade::api::pipeline_layout_param_type::push_descriptors_with_ranges: {
      for (uint32_t i = 0; i < param.descriptor_table.count; ++i) {
        if (resolve_range(param.descriptor_table.ranges[i])) return true;
      }
      return false;
    }
    case reshade::api::pipeline_layout_param_type::descriptor_table: {
      for (uint32_t i = 0; i < param.descriptor_table.count; ++i) {
        if (resolve_range(param.descriptor_table.ranges[i])) return true;
      }
      return false;
    }
    case reshade::api::pipeline_layout_param_type::descriptor_table_with_static_samplers: {
#if RESHADE_API_VERSION >= 13
      for (uint32_t i = 0; i < param.descriptor_table_with_static_samplers.count; ++i) {
        if (resolve_range(param.descriptor_table_with_static_samplers.ranges[i])) return true;
      }
#endif
      return false;
    }
    default:
      return false;
  }
}

void CaptureTrackedTextureView(
    const uint32_t shader_hash,
    const uint32_t reg,
    const uint32_t space,
    const reshade::api::resource_view view) {
  if (view.handle == 0u) return;
  if (space != 0u) return;

  if (shader_hash == kLightingShader && reg == 9u) {
    g_lighting_ssgi_view.store(view.handle, std::memory_order_relaxed);
  } else if (shader_hash == kLightingShader && reg == 2u) {
    g_lighting_mrt1_view.store(view.handle, std::memory_order_relaxed);
  } else if (shader_hash == kLightingShader && reg == 3u) {
    g_lighting_depth_view.store(view.handle, std::memory_order_relaxed);
  } else if (shader_hash == kLightingShader && reg == 4u) {
    g_lighting_ssao_view.store(view.handle, std::memory_order_relaxed);
  } else if (shader_hash == kCharacterShader && reg == 0u) {
    // Character pass depth texture.
    g_character_depth_view.store(view.handle, std::memory_order_relaxed);
  } else if (shader_hash == kCharacterShader && reg == 2u) {
    // Character pass uses mrtTexture0 at t2.
    g_character_mrt0_view.store(view.handle, std::memory_order_relaxed);
  } else if (shader_hash == kLightingShader && reg == 1u) {
    // Keep lighting mrt0 as fallback in case character pass capture is unavailable.
    g_lighting_mrt0_view.store(view.handle, std::memory_order_relaxed);
  }
}

void OnPushDescriptorsCaptureLightingTextures(
    reshade::api::command_list* cmd_list,
    reshade::api::shader_stage stages,
    reshade::api::pipeline_layout layout,
    uint32_t layout_param,
    const reshade::api::descriptor_table_update& update) {
  if (cmd_list == nullptr) return;
  if ((static_cast<uint32_t>(stages) & static_cast<uint32_t>(reshade::api::shader_stage::pixel)) == 0u) return;

  auto* shader_state = renodx::utils::shader::GetCurrentState(cmd_list);
  if (shader_state == nullptr) return;
  const auto shader_hash = renodx::utils::shader::GetCurrentPixelShaderHash(shader_state);
  const bool is_tracked_shader = shader_hash == kLightingShader || shader_hash == kCharacterShader;

  for (uint32_t i = 0; i < update.count; ++i) {
    uint32_t reg = 0u;
    uint32_t space = 0u;
    if (!TryResolveTextureRegister(layout, layout_param, update.binding + i, &reg, &space)) {
      reg = update.binding + i;
      space = 0u;
    }
    if (update.type == reshade::api::descriptor_type::constant_buffer) continue;
    if (!is_tracked_shader) continue;
    const auto view = renodx::utils::descriptor::GetResourceViewFromDescriptorUpdate(update, i);
    CaptureTrackedTextureView(shader_hash, reg, space, view);
  }
}

bool TryGetResourceViewFromBoundDescriptorTable(
    reshade::api::device* device,
    const reshade::api::descriptor_table& table,
    const reshade::api::descriptor_range& range,
    const uint32_t descriptor_index,
    reshade::api::resource_view* out_view) {
  if (device == nullptr || out_view == nullptr) return false;
  if (table.handle == 0u) return false;
  if (range.count == UINT32_MAX) return false;
  if (descriptor_index >= range.count) return false;

  auto* descriptor_data = renodx::utils::data::Get<renodx::utils::descriptor::DeviceData>(device);
  if (descriptor_data == nullptr) return false;

  uint32_t base_offset = 0u;
  reshade::api::descriptor_heap heap = {0u};
  device->get_descriptor_heap_offset(table, range.binding, 0, &heap, &base_offset);
  if (heap.handle == 0u) return false;

  const std::shared_lock lock(descriptor_data->mutex);

  auto heap_pair = descriptor_data->heaps.find(heap.handle);
  if (heap_pair == descriptor_data->heaps.end()) return false;

  auto known_pair = descriptor_data->resource_view_heap_locations.find(heap.handle);
  if (known_pair == descriptor_data->resource_view_heap_locations.end()) return false;

  const uint32_t offset = base_offset + descriptor_index;
  const auto& heap_entries = heap_pair->second;
  if (offset >= heap_entries.size()) return false;
  if (!known_pair->second.contains(offset)) return false;

  const auto& [descriptor_type, descriptor_value] = heap_entries[offset];
  reshade::api::resource_view view = {};
  switch (descriptor_type) {
    case reshade::api::descriptor_type::sampler_with_resource_view:
      view = std::get<reshade::api::sampler_with_resource_view>(descriptor_value).view;
      break;
    case reshade::api::descriptor_type::buffer_shader_resource_view:
    case reshade::api::descriptor_type::texture_shader_resource_view:
    case reshade::api::descriptor_type::buffer_unordered_access_view:
    case reshade::api::descriptor_type::texture_unordered_access_view:
      view = std::get<reshade::api::resource_view>(descriptor_value);
      break;
    default:
      return false;
  }
  if (view.handle == 0u) return false;

  *out_view = view;
  return true;
}

void OnBindDescriptorTablesCaptureLightingTextures(
    reshade::api::command_list* cmd_list,
    reshade::api::shader_stage stages,
    reshade::api::pipeline_layout layout,
    uint32_t first,
    uint32_t count,
    const reshade::api::descriptor_table* tables) {
  if (cmd_list == nullptr || tables == nullptr || count == 0u) return;
  if ((static_cast<uint32_t>(stages) & static_cast<uint32_t>(reshade::api::shader_stage::pixel)) == 0u) return;

  auto* shader_state = renodx::utils::shader::GetCurrentState(cmd_list);
  if (shader_state == nullptr) return;
  const auto shader_hash = renodx::utils::shader::GetCurrentPixelShaderHash(shader_state);
  if (shader_hash != kLightingShader && shader_hash != kCharacterShader) return;

  auto* layout_data = renodx::utils::pipeline_layout::GetPipelineLayoutData(layout);
  if (layout_data == nullptr) return;

  auto* device = cmd_list->get_device();
  for (uint32_t i = 0; i < count; ++i) {
    const uint32_t param_index = first + i;
    if (param_index >= layout_data->params.size()) continue;

    const auto& param = layout_data->params.at(param_index);
    const auto& table = tables[i];
    if (table.handle == 0u) continue;

    uint32_t range_count = 0u;
    const reshade::api::descriptor_range* ranges = nullptr;
    if (param.type == reshade::api::pipeline_layout_param_type::descriptor_table) {
      range_count = param.descriptor_table.count;
      ranges = param.descriptor_table.ranges;
    } else if (param.type == reshade::api::pipeline_layout_param_type::descriptor_table_with_static_samplers) {
      range_count = param.descriptor_table_with_static_samplers.count;
      ranges = param.descriptor_table_with_static_samplers.ranges;
    } else {
      continue;
    }

    for (uint32_t j = 0; j < range_count; ++j) {
      const auto& range = ranges[j];
      if (range.count == UINT32_MAX) continue;

      switch (range.type) {
        case reshade::api::descriptor_type::sampler_with_resource_view:
        case reshade::api::descriptor_type::buffer_shader_resource_view:
        case reshade::api::descriptor_type::texture_shader_resource_view:
          break;
        default:
          continue;
      }

      if ((static_cast<uint32_t>(range.visibility) & static_cast<uint32_t>(reshade::api::shader_stage::pixel)) == 0u) {
        continue;
      }

      for (uint32_t k = 0; k < range.count; ++k) {
        reshade::api::resource_view view = {};
        if (!TryGetResourceViewFromBoundDescriptorTable(device, table, range, k, &view)) continue;
        CaptureTrackedTextureView(shader_hash, range.dx_register_index + k, range.dx_register_space, view);
      }
    }
  }
}

void DestroyCompositeResources(reshade::api::device* device, DeviceData* data) {
  if (device == nullptr || data == nullptr) return;

  data->composite_pass.DestroyAll(device);

  if (data->composite_source_texture_view.handle != 0u) {
    device->destroy_resource_view(data->composite_source_texture_view);
    data->composite_source_texture_view = {0u};
  }
  if (data->composite_source_texture.handle != 0u) {
    device->destroy_resource(data->composite_source_texture);
    data->composite_source_texture = {0u};
  }
  data->composite_source_desc = {};
}

void OnInitDevice(reshade::api::device* device) {
  device->create_private_data<DeviceData>();
}

void OnDestroyDevice(reshade::api::device* device) {
  auto* data = device->get_private_data<DeviceData>();
  if (data != nullptr) {
    DestroyCompositeResources(device, data);
  }
  device->destroy_private_data<DeviceData>();
}

void OnInitSwapchain(reshade::api::swapchain* swapchain, bool resize) {
  (void)resize;
  auto* device = swapchain->get_device();
  auto* data = device->get_private_data<DeviceData>();
  if (data == nullptr) return;

  DestroyCompositeResources(device, data);
  data->composite_pass.auto_generate_render_target_formats = true;
  data->composite_pass.auto_generate_descriptor_table_updates = true;
}

void OnDestroySwapchain(reshade::api::swapchain* swapchain, bool resize) {
  (void)resize;
  auto* device = swapchain->get_device();
  auto* data = device->get_private_data<DeviceData>();
  if (data == nullptr) return;
  DestroyCompositeResources(device, data);
}

bool EnsureCompositeSourceTexture(
    reshade::api::device* device,
    DeviceData* data,
    const reshade::api::resource_desc& target_desc) {
  if (device == nullptr || data == nullptr) return false;
  if (target_desc.type != reshade::api::resource_type::texture_2d) return false;
  if (target_desc.texture.samples != 1) return false;

  const auto target_typeless = reshade::api::format_to_typeless(target_desc.texture.format);
  const bool texture_matches =
      data->composite_source_texture.handle != 0u
      && data->composite_source_desc.type == reshade::api::resource_type::texture_2d
      && data->composite_source_desc.texture.width == target_desc.texture.width
      && data->composite_source_desc.texture.height == target_desc.texture.height
      && data->composite_source_desc.texture.depth_or_layers == 1u
      && data->composite_source_desc.texture.levels == 1u
      && data->composite_source_desc.texture.samples == 1u
      && data->composite_source_desc.texture.format == target_typeless;
  if (texture_matches && data->composite_source_texture_view.handle != 0u) return true;

  if (data->composite_source_texture_view.handle != 0u) {
    device->destroy_resource_view(data->composite_source_texture_view);
    data->composite_source_texture_view = {0u};
  }
  if (data->composite_source_texture.handle != 0u) {
    device->destroy_resource(data->composite_source_texture);
    data->composite_source_texture = {0u};
  }
  data->composite_source_desc = {};

  reshade::api::resource_desc source_desc = {};
  source_desc.type = reshade::api::resource_type::texture_2d;
  source_desc.texture = {
      target_desc.texture.width,
      target_desc.texture.height,
      1,
      1,
      target_typeless,
      1,
  };
  source_desc.heap = reshade::api::memory_heap::gpu_only;
  source_desc.usage = reshade::api::resource_usage::copy_dest | reshade::api::resource_usage::shader_resource;
  source_desc.flags = reshade::api::resource_flags::none;

  if (!device->create_resource(
          source_desc,
          nullptr,
          reshade::api::resource_usage::shader_resource,
          &data->composite_source_texture)) {
    return false;
  }

  if (!device->create_resource_view(
          data->composite_source_texture,
          reshade::api::resource_usage::shader_resource,
          reshade::api::resource_view_desc(reshade::api::format_to_default_typed(source_desc.texture.format)),
          &data->composite_source_texture_view)) {
    device->destroy_resource(data->composite_source_texture);
    data->composite_source_texture = {0u};
    return false;
  }

  data->composite_source_desc = source_desc;
  return true;
}

bool ApplyCharacterSSGIComposite(
    reshade::api::command_list* cmd_list,
    reshade::api::resource_view target_rtv) {
  if (cmd_list == nullptr || target_rtv.handle == 0u) return false;

  auto* device = cmd_list->get_device();
  if (device == nullptr) return false;
  auto* data = device->get_private_data<DeviceData>();
  if (data == nullptr) return false;

  reshade::api::resource_view ssgi_view = {g_lighting_ssgi_view.load(std::memory_order_relaxed)};
  reshade::api::resource_view mrt0_view = {g_character_mrt0_view.load(std::memory_order_relaxed)};
  reshade::api::resource_view mrt1_view = {g_lighting_mrt1_view.load(std::memory_order_relaxed)};
  reshade::api::resource_view depth_view = {g_character_depth_view.load(std::memory_order_relaxed)};
  if (depth_view.handle == 0u) {
    depth_view = {g_lighting_depth_view.load(std::memory_order_relaxed)};
  }
  reshade::api::resource_view ssao_view = {g_lighting_ssao_view.load(std::memory_order_relaxed)};
  if (mrt0_view.handle == 0u) {
    mrt0_view = {g_lighting_mrt0_view.load(std::memory_order_relaxed)};
  }
  if (mrt0_view.handle == 0u) return false;

  auto* target_view_info = renodx::utils::resource::GetResourceViewInfo(target_rtv);
  auto* mrt0_view_info = renodx::utils::resource::GetResourceViewInfo(mrt0_view);
  if (target_view_info == nullptr || mrt0_view_info == nullptr) return false;
  if (target_view_info->destroyed || mrt0_view_info->destroyed) return false;

  bool has_ssgi = ssgi_view.handle != 0u;
  bool has_mrt1 = mrt1_view.handle != 0u;
  bool has_depth = depth_view.handle != 0u;
  bool has_ssao = ssao_view.handle != 0u;
  if (has_ssgi) {
    auto* info = renodx::utils::resource::GetResourceViewInfo(ssgi_view);
    has_ssgi = info != nullptr && !info->destroyed;
  }
  if (has_mrt1) {
    auto* info = renodx::utils::resource::GetResourceViewInfo(mrt1_view);
    has_mrt1 = info != nullptr && !info->destroyed;
  }
  if (has_depth) {
    auto* info = renodx::utils::resource::GetResourceViewInfo(depth_view);
    has_depth = info != nullptr && !info->destroyed;
  }
  if (has_ssao) {
    auto* info = renodx::utils::resource::GetResourceViewInfo(ssao_view);
    has_ssao = info != nullptr && !info->destroyed;
  }

  auto target_resource = device->get_resource_from_view(target_rtv);
  if (target_resource.handle == 0u) return false;
  auto target_desc = device->get_resource_desc(target_resource);
  if (!EnsureCompositeSourceTexture(device, data, target_desc)) return false;

  if (!has_ssgi) {
    ssgi_view = data->composite_source_texture_view;
  }
  if (!has_mrt1) {
    mrt1_view = mrt0_view;
  }
  if (!has_depth) {
    depth_view = data->composite_source_texture_view;
  }
  if (!has_ssao) {
    ssao_view = data->composite_source_texture_view;
  }

  CharacterGiCompositeData composite_data = character_gi_composite_data;
  if (char_ssgi_composite_enabled < 0.5f) {
    composite_data.strength = 0.0f;
    composite_data.alpha_scale = 0.0f;
    composite_data.chroma_strength = 0.0f;
    composite_data.luma_strength = 0.0f;
  }
  if (!has_ssgi) {
    composite_data.strength = 0.0f;
    composite_data.alpha_scale = 0.0f;
  }
  if (!has_mrt1) {
    composite_data.normal_reject = 0.0f;
  }
  if (!has_depth) {
    composite_data.depth_reject = 0.0f;
  }
  if (!has_ssao) {
    composite_data.ao_influence = 0.0f;
  }

  {
    const reshade::api::resource resources[2] = {target_resource, data->composite_source_texture};
    const reshade::api::resource_usage state_old[2] = {
        reshade::api::resource_usage::render_target,
        reshade::api::resource_usage::shader_resource};
    const reshade::api::resource_usage state_new[2] = {
        reshade::api::resource_usage::copy_source,
        reshade::api::resource_usage::copy_dest};
    cmd_list->barrier(2, resources, state_old, state_new);
    cmd_list->copy_texture_region(target_resource, 0, nullptr, data->composite_source_texture, 0, nullptr);
    cmd_list->barrier(2, resources, state_new, state_old);
  }

  data->composite_pass.InvalidateRenderTargets(cmd_list);
  data->composite_pass.render_target_slots.views = {target_rtv};
  data->composite_pass.shader_resource_slots.views = {
      data->composite_source_texture_view,
      ssgi_view,
      mrt0_view,
      mrt1_view,
      depth_view,
      ssao_view,
  };
  data->composite_pass.sampler_descs = {reshade::api::sampler_desc{}};
  data->composite_pass.pipeline_subobjects.vertex_shader = __0xFFFFFFFD;
  data->composite_pass.pipeline_subobjects.pixel_shader = __0xFFFFFFFE;
  data->composite_pass.pipeline_subobjects.compute_shader = {};
  data->composite_pass.push_constants.clear();
  data->composite_pass.push_constants[renodx::utils::render::ConstantBuffersSlots{
      .slot = 13,
      .space = 0,
  }] = std::span<const float>(
      reinterpret_cast<const float*>(&composite_data),
      sizeof(composite_data) / sizeof(float));
  data->composite_pass.auto_generate_descriptor_table_updates = true;
  data->composite_pass.revert_state_after_render = true;
  data->composite_pass.flush_after_render = false;
  return data->composite_pass.Render(cmd_list);
}

void OnLightingShaderDrawnApplyCharacterSSGI(reshade::api::command_list* cmd_list) {
  if (char_ssgi_composite_enabled < 0.5f) return;
  if (cmd_list == nullptr) return;

  const auto frame_index = g_present_frame_index.load(std::memory_order_relaxed);
  if (g_last_composite_frame_index.load(std::memory_order_relaxed) == frame_index) return;

  auto* swapchain_state = renodx::utils::swapchain::GetCurrentState(cmd_list);
  if (swapchain_state == nullptr) return;
  if (swapchain_state->current_render_targets.empty()) return;
  auto target_rtv = swapchain_state->current_render_targets.at(0);
  if (target_rtv.handle == 0u) return;

  if (ApplyCharacterSSGIComposite(cmd_list, target_rtv)) {
    g_last_composite_frame_index.store(frame_index, std::memory_order_relaxed);
  }
}

void OnPresentAdvanceFrame(
    reshade::api::command_queue* queue,
    reshade::api::swapchain* swapchain,
    const reshade::api::rect* source_rect,
    const reshade::api::rect* dest_rect,
    uint32_t dirty_rect_count,
    const reshade::api::rect* dirty_rects) {
  (void)queue;
  (void)swapchain;
  (void)source_rect;
  (void)dest_rect;
  (void)dirty_rect_count;
  (void)dirty_rects;
  g_present_frame_index.fetch_add(1u, std::memory_order_relaxed);
}

renodx::utils::settings::Settings settings = {
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
    new renodx::utils::settings::Setting{
        .key = "CharacterSSGICompositeEnable",
        .binding = &char_ssgi_composite_enabled,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Apply Game SSGI",
        .section = "Character SSGI Composite",
        .labels = {"Off", "On"},
    },
    new renodx::utils::settings::Setting{
        .key = "CharacterSSGICompositeStrength",
        .binding = &character_gi_composite_data.strength,
        .default_value = 3.0f,
        .label = "Strength",
        .section = "Character SSGI Composite",
        .tooltip = "Overall contribution scale for character GI.",
        .min = 0.f,
        .max = 3.f,
        .format = "%.2f",
        .is_enabled = []() { return char_ssgi_composite_enabled >= 0.5f; },
    },
    new renodx::utils::settings::Setting{
        .key = "CharacterSSGICompositeAlphaScale",
        .binding = &character_gi_composite_data.alpha_scale,
        .default_value = 0.75f,
        .label = "Alpha Scale",
        .section = "Character SSGI Composite",
        .tooltip = "Scales sampled SSGI alpha before blending.",
        .min = 0.f,
        .max = 3.f,
        .format = "%.2f",
        .is_enabled = []() { return char_ssgi_composite_enabled >= 0.5f; },
    },
    new renodx::utils::settings::Setting{
        .key = "CharacterSSGICompositeChroma",
        .binding = &character_gi_composite_data.chroma_strength,
        .default_value = 0.50f,
        .label = "Chroma",
        .section = "Character SSGI Composite",
        .tooltip = "Scales colorful GI component; lower values reduce tinting.",
        .min = 0.f,
        .max = 2.f,
        .format = "%.2f",
        .is_enabled = []() { return char_ssgi_composite_enabled >= 0.5f; },
    },
    new renodx::utils::settings::Setting{
        .key = "CharacterSSGICompositeLuma",
        .binding = &character_gi_composite_data.luma_strength,
        .default_value = 0.0f,
        .label = "Luma",
        .section = "Character SSGI Composite",
        .tooltip = "Scales neutral GI brightness; keep low to avoid white haze.",
        .min = 0.f,
        .max = 1.f,
        .format = "%.3f",
        .is_enabled = []() { return char_ssgi_composite_enabled >= 0.5f; },
    },
    new renodx::utils::settings::Setting{
        .key = "CharacterSSGICompositeShadowPower",
        .binding = &character_gi_composite_data.shadow_power,
        .default_value = 1.25f,
        .label = "Shadow Power",
        .section = "Character SSGI Composite",
        .tooltip = "Higher values concentrate GI toward darker areas.",
        .min = 0.1f,
        .max = 4.f,
        .format = "%.2f",
        .is_enabled = []() { return char_ssgi_composite_enabled >= 0.5f; },
    },
    new renodx::utils::settings::Setting{
        .key = "CharacterSSGICompositeDarkBoost",
        .binding = &character_gi_composite_data.dark_boost,
        .default_value = 0.50f,
        .label = "Dark Boost",
        .section = "Character SSGI Composite",
        .tooltip = "Extra GI multiplier in darker regions (after shadow mask).",
        .min = 0.f,
        .max = 4.f,
        .format = "%.2f",
        .is_enabled = []() { return char_ssgi_composite_enabled >= 0.5f; },
    },
    new renodx::utils::settings::Setting{
        .key = "CharacterSSGICompositeBrightBoost",
        .binding = &character_gi_composite_data.bright_boost,
        .default_value = 3.0f,
        .label = "Bright Boost",
        .section = "Character SSGI Composite",
        .tooltip = "Boosts GI on brighter regions (values above 1.0 increase bright-side contribution).",
        .min = 0.f,
        .max = 3.f,
        .format = "%.2f",
        .is_enabled = []() { return char_ssgi_composite_enabled >= 0.5f; },
    },
    new renodx::utils::settings::Setting{
        .key = "CharacterSSGICompositeHeadroomPower",
        .binding = &character_gi_composite_data.headroom_power,
        .default_value = 1.25f,
        .label = "Headroom Power",
        .section = "Character SSGI Composite",
        .tooltip = "Controls how strongly bright pixels reject additional GI.",
        .min = 0.1f,
        .max = 4.f,
        .format = "%.2f",
        .is_enabled = []() { return char_ssgi_composite_enabled >= 0.5f; },
    },
    new renodx::utils::settings::Setting{
        .key = "CharacterSSGICompositeMaxAdd",
        .binding = &character_gi_composite_data.max_add,
        .default_value = 0.020f,
        .label = "Max Add",
        .section = "Character SSGI Composite",
        .tooltip = "Per-channel cap for added GI to prevent haze/bloomy washout.",
        .min = 0.f,
        .max = 1.f,
        .format = "%.3f",
        .is_enabled = []() { return char_ssgi_composite_enabled >= 0.5f; },
    },
    new renodx::utils::settings::Setting{
        .key = "CharacterSSGICompositePeakLumaCap",
        .binding = &character_gi_composite_data.peak_luma_cap,
        .default_value = 0.0f,
        .label = "Peak Luma Cap",
        .section = "Character SSGI Composite",
        .tooltip = "Caps peak GI brightness on characters after blending weights. Set 0 to disable.",
        .min = 0.f,
        .max = 1.f,
        .format = "%.3f",
        .is_enabled = []() { return char_ssgi_composite_enabled >= 0.5f; },
    },
    new renodx::utils::settings::Setting{
        .key = "CharacterSSGICompositeDepthReject",
        .binding = &character_gi_composite_data.depth_reject,
        .default_value = 2.0f,
        .label = "Depth Reject",
        .section = "Character SSGI Composite",
        .tooltip = "Higher values suppress GI across depth discontinuities and silhouette edges.",
        .min = 0.f,
        .max = 16.f,
        .format = "%.2f",
        .is_enabled = []() { return char_ssgi_composite_enabled >= 0.5f; },
    },
    new renodx::utils::settings::Setting{
        .key = "CharacterSSGICompositeNormalReject",
        .binding = &character_gi_composite_data.normal_reject,
        .default_value = 0.15f,
        .label = "Normal Reject",
        .section = "Character SSGI Composite",
        .tooltip = "Higher values suppress GI across normal/material edges.",
        .min = 0.f,
        .max = 8.f,
        .format = "%.2f",
        .is_enabled = []() { return char_ssgi_composite_enabled >= 0.5f; },
    },
    new renodx::utils::settings::Setting{
        .key = "CharacterSSGICompositeAOInfluence",
        .binding = &character_gi_composite_data.ao_influence,
        .default_value = 0.66f,
        .label = "AO Influence",
        .section = "Character SSGI Composite",
        .tooltip = "Uses game's AO to dampen character GI in occluded areas.",
        .min = 0.f,
        .max = 1.f,
        .format = "%.2f",
        .is_enabled = []() { return char_ssgi_composite_enabled >= 0.5f; },
    },
    new renodx::utils::settings::Setting{
        .key = "CharacterSSGICompositeRejectStrength",
        .binding = &character_gi_composite_data.reject_strength,
        .default_value = 8.0f,
        .label = "Reject Strength",
        .section = "Character SSGI Composite",
        .tooltip = "Amplifies depth/normal/AO rejection visibility without changing base GI sliders.",
        .min = 0.f,
        .max = 8.f,
        .format = "%.2f",
        .is_enabled = []() { return char_ssgi_composite_enabled >= 0.5f; },
    },
    new renodx::utils::settings::Setting{
        .key = "CharacterSSGICompositeDebugMode",
        .binding = &character_gi_composite_data.debug_mode,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f,
        .label = "Debug View",
        .section = "Character SSGI Composite",
        .labels = {
            "Off",
            "Character Mask",
            "Raw SSGI RGB",
            "Raw SSGI Alpha",
            "Filtered GI",
            "GI Weight (Pre-Headroom)",
            "GI Contribution Abs",
            "Source Color",
            "GI Contribution Luma",
            "Headroom",
            "Final Gain",
            "Shadow Mask (Adj)",
            "Depth Factor",
            "Normal Factor",
            "AO Factor",
            "Reject Factor",
        },
        .is_enabled = []() { return char_ssgi_composite_enabled >= 0.5f; },
    },
    new renodx::utils::settings::Setting{
        .key = "CharacterSSGICompositeDebugScale",
        .binding = &character_gi_composite_data.debug_scale,
        .default_value = 1.f,
        .label = "Debug Scale",
        .section = "Character SSGI Composite",
        .tooltip = "Scales debug intensity for RGB/alpha/weight/contribution views.",
        .min = 0.1f,
        .max = 32.f,
        .format = "%.2f",
        .is_enabled = []() {
          return char_ssgi_composite_enabled >= 0.5f
                 && character_gi_composite_data.debug_mode >= 1.f;
        },
    },
    new renodx::utils::settings::Setting{
        .key = "CharacterSSGICompositeDebugCharsOnly",
        .binding = &character_gi_composite_data.debug_chars_only,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Debug Characters Only",
        .section = "Character SSGI Composite",
        .labels = {"Off", "On"},
        .is_enabled = []() {
          return char_ssgi_composite_enabled >= 0.5f
                 && character_gi_composite_data.debug_mode >= 1.f;
        },
    },
};

}  // namespace

extern "C" __declspec(dllexport) constexpr const char* NAME = "Kai Vanilla+";
extern "C" __declspec(dllexport) constexpr const char* DESCRIPTION = "Kai Vanilla+";

BOOL APIENTRY DllMain(HMODULE h_module, DWORD fdw_reason, LPVOID lpv_reserved) {
  switch (fdw_reason) {
    case DLL_PROCESS_ATTACH:
      if (!reshade::register_addon(h_module)) return FALSE;

      reshade::register_event<reshade::addon_event::init_device>(OnInitDevice);
      reshade::register_event<reshade::addon_event::destroy_device>(OnDestroyDevice);
      reshade::register_event<reshade::addon_event::init_swapchain>(OnInitSwapchain);
      reshade::register_event<reshade::addon_event::destroy_swapchain>(OnDestroySwapchain);
      reshade::register_event<reshade::addon_event::present>(OnPresentAdvanceFrame);
      reshade::register_event<reshade::addon_event::push_descriptors>(OnPushDescriptorsCaptureLightingTextures);
      reshade::register_event<reshade::addon_event::bind_descriptor_tables>(OnBindDescriptorTablesCaptureLightingTextures);

      renodx::utils::settings::overlay_title = "Kai Vanilla+";
      renodx::utils::settings::global_name = "kai-vanilla-plus";
      renodx::utils::settings::use_presets = false;

      renodx::mods::shader::force_pipeline_cloning = true;
      renodx::mods::shader::allow_multiple_push_constants = true;
      renodx::mods::shader::expected_constant_buffer_index = 13;
      renodx::mods::shader::expected_constant_buffer_space = 0;

      break;
    case DLL_PROCESS_DETACH:
      reshade::unregister_event<reshade::addon_event::init_device>(OnInitDevice);
      reshade::unregister_event<reshade::addon_event::destroy_device>(OnDestroyDevice);
      reshade::unregister_event<reshade::addon_event::init_swapchain>(OnInitSwapchain);
      reshade::unregister_event<reshade::addon_event::destroy_swapchain>(OnDestroySwapchain);
      reshade::unregister_event<reshade::addon_event::present>(OnPresentAdvanceFrame);
      reshade::unregister_event<reshade::addon_event::push_descriptors>(OnPushDescriptorsCaptureLightingTextures);
      reshade::unregister_event<reshade::addon_event::bind_descriptor_tables>(OnBindDescriptorTablesCaptureLightingTextures);
      reshade::unregister_addon(h_module);
      break;
  }

  renodx::utils::resource::Use(fdw_reason);
  renodx::utils::swapchain::Use(fdw_reason);
  renodx::utils::descriptor::trace_descriptor_tables = true;
  renodx::utils::descriptor::Use(fdw_reason);
  renodx::utils::settings::Use(fdw_reason, &settings);
  renodx::mods::shader::Use(fdw_reason, custom_shaders, &shader_injection);

  return TRUE;
}
