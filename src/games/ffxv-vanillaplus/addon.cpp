/*
 * Copyright (C) 2026
 * SPDX-License-Identifier: MIT
 */

#define ImTextureID ImU64

#define DEBUG_LEVEL_0

#include <algorithm>
#include <array>
#include <atomic>
#include <shared_mutex>
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
#include "./shared.h"

namespace {

constexpr uint32_t kAmbientDiffuseShader = 0x3D895A00u;
constexpr uint32_t kAmbientSpecularShader = 0xD776269Au;
constexpr uint32_t kAmbientLpvShader = 0xB3DCADD1u;

constexpr uint32_t kAmbientAlbedoRegister = 1u;
constexpr uint32_t kAmbientSpecularRegister = 2u;
constexpr uint32_t kAmbientNormalRegister = 3u;
constexpr uint32_t kAmbientDepthRegister = 4u;
constexpr uint32_t kAmbientAoRegister = 5u;
constexpr uint32_t kAmbientShadowRegister = 6u;
constexpr uint32_t kAmbientEnvLutRegister = 8u;
constexpr uint32_t kAmbientSsrRegister = 9u;

void OnAmbientShaderDrawnApplyComposite(reshade::api::command_list* cmd_list);

ShaderInjectData shader_injection = {
    .mod_enabled = 1.f,
    .slider_1 = 25.f,
    .slider_2 = 50.f,
    .slider_3 = 100.f,
};

float ambient_gi_composite_enabled = 1.f;
float ambient_gi_color_boost = 100.f;
float ambient_gi_bleed_radius = 1.f;
float ambient_gi_tint_source_a = 2.f;
float ambient_gi_tint_source_b = 0.f;
float ambient_gi_tint_source_blend = 0.f;
float ambient_gi_debug_mode = 0.f;
float ambient_gi_debug_scale = 1.f;

std::atomic_uint64_t g_ambient_albedo_view{0u};
std::atomic_uint64_t g_ambient_specular_view{0u};
std::atomic_uint64_t g_ambient_normal_view{0u};
std::atomic_uint64_t g_ambient_depth_view{0u};
std::atomic_uint64_t g_ambient_ao_view{0u};
std::atomic_uint64_t g_ambient_shadow_view{0u};
std::atomic_uint64_t g_ambient_env_lut_view{0u};
std::atomic_uint64_t g_ambient_ssr_view{0u};
std::atomic_uint64_t g_present_frame_index{0u};
std::atomic_uint64_t g_last_composite_frame_index{static_cast<uint64_t>(-1)};

struct __declspec(uuid("f0619fd3-e383-4ef5-9f70-e1ad8cc53449")) DeviceData {
  reshade::api::resource composite_source_texture = {};
  reshade::api::resource_view composite_source_texture_view = {};
  reshade::api::resource_desc composite_source_desc = {};
  renodx::utils::render::RenderPass composite_pass = {};
};

bool IsAmbientShader(const uint32_t shader_hash) {
  return shader_hash == kAmbientDiffuseShader
      || shader_hash == kAmbientSpecularShader
      || shader_hash == kAmbientLpvShader;
}

renodx::mods::shader::CustomShaders BuildCustomShaders() {
  renodx::mods::shader::CustomShaders shaders = {
#ifdef __ALL_CUSTOM_SHADERS
      __ALL_CUSTOM_SHADERS,
#endif
  };

  // Fullscreen composite shaders are invoked through RenderPass, not global replacement.
  shaders.erase(0xFFFFFFFDu);
  shaders.erase(0xFFFFFFFEu);

  shaders.insert_or_assign(
      kAmbientDiffuseShader,
      renodx::mods::shader::CustomShader{
          .crc32 = kAmbientDiffuseShader,
          .on_drawn = &OnAmbientShaderDrawnApplyComposite,
      });
  shaders.insert_or_assign(
      kAmbientSpecularShader,
      renodx::mods::shader::CustomShader{
          .crc32 = kAmbientSpecularShader,
          .on_drawn = &OnAmbientShaderDrawnApplyComposite,
      });
  shaders.insert_or_assign(
      kAmbientLpvShader,
      renodx::mods::shader::CustomShader{
          .crc32 = kAmbientLpvShader,
          .on_drawn = &OnAmbientShaderDrawnApplyComposite,
      });

  return shaders;
}

renodx::mods::shader::CustomShaders custom_shaders = BuildCustomShaders();

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
  if (!IsAmbientShader(shader_hash)) return;

  if (shader_hash == kAmbientLpvShader) {
    // ambient_0xB3DCADD1 register layout:
    // t0 normal, t1 diffuse, t2 specular, t3 AO, t4 depth.
    switch (reg) {
      case 0u:
        g_ambient_normal_view.store(view.handle, std::memory_order_relaxed);
        break;
      case 1u:
        g_ambient_albedo_view.store(view.handle, std::memory_order_relaxed);
        break;
      case 2u:
        g_ambient_specular_view.store(view.handle, std::memory_order_relaxed);
        break;
      case 3u:
        g_ambient_ao_view.store(view.handle, std::memory_order_relaxed);
        break;
      case 4u:
        g_ambient_depth_view.store(view.handle, std::memory_order_relaxed);
        break;
      default:
        break;
    }
    return;
  }

  switch (reg) {
    case kAmbientAlbedoRegister:
      g_ambient_albedo_view.store(view.handle, std::memory_order_relaxed);
      break;
    case kAmbientSpecularRegister:
      g_ambient_specular_view.store(view.handle, std::memory_order_relaxed);
      break;
    case kAmbientNormalRegister:
      g_ambient_normal_view.store(view.handle, std::memory_order_relaxed);
      break;
    case kAmbientDepthRegister:
      g_ambient_depth_view.store(view.handle, std::memory_order_relaxed);
      break;
    case kAmbientAoRegister:
      g_ambient_ao_view.store(view.handle, std::memory_order_relaxed);
      break;
    case kAmbientShadowRegister:
      g_ambient_shadow_view.store(view.handle, std::memory_order_relaxed);
      break;
    case kAmbientEnvLutRegister:
      if (shader_hash == kAmbientSpecularShader) {
        g_ambient_env_lut_view.store(view.handle, std::memory_order_relaxed);
      }
      break;
    case kAmbientSsrRegister:
      if (shader_hash == kAmbientSpecularShader) {
        g_ambient_ssr_view.store(view.handle, std::memory_order_relaxed);
      }
      break;
    default:
      break;
  }
}

void OnPushDescriptorsCaptureAmbientTextures(
    reshade::api::command_list* cmd_list,
    reshade::api::shader_stage stages,
    reshade::api::pipeline_layout layout,
    uint32_t layout_param,
    const reshade::api::descriptor_table_update& update) {
  if (cmd_list == nullptr) return;
  if ((static_cast<uint32_t>(stages) & static_cast<uint32_t>(reshade::api::shader_stage::pixel)) == 0u) return;
  if (update.type == reshade::api::descriptor_type::constant_buffer) return;

  auto* shader_state = renodx::utils::shader::GetCurrentState(cmd_list);
  if (shader_state == nullptr) return;
  const auto shader_hash = renodx::utils::shader::GetCurrentPixelShaderHash(shader_state);
  if (!IsAmbientShader(shader_hash)) return;

  for (uint32_t i = 0; i < update.count; ++i) {
    uint32_t reg = 0u;
    uint32_t space = 0u;
    if (!TryResolveTextureRegister(layout, layout_param, update.binding + i, &reg, &space)) continue;

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

void OnBindDescriptorTablesCaptureAmbientTextures(
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
  if (!IsAmbientShader(shader_hash)) return;

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

bool IsViewAlive(const reshade::api::resource_view view) {
  if (view.handle == 0u) return false;
  auto* info = renodx::utils::resource::GetResourceViewInfo(view);
  return info != nullptr && !info->destroyed;
}

bool ApplyAmbientGiComposite(
    reshade::api::command_list* cmd_list,
    reshade::api::resource_view target_rtv) {
  if (cmd_list == nullptr || target_rtv.handle == 0u) return false;

  auto* device = cmd_list->get_device();
  if (device == nullptr) return false;
  auto* data = device->get_private_data<DeviceData>();
  if (data == nullptr) return false;

  reshade::api::resource_view albedo_view = {g_ambient_albedo_view.load(std::memory_order_relaxed)};
  reshade::api::resource_view specular_view = {g_ambient_specular_view.load(std::memory_order_relaxed)};
  reshade::api::resource_view normal_view = {g_ambient_normal_view.load(std::memory_order_relaxed)};
  reshade::api::resource_view depth_view = {g_ambient_depth_view.load(std::memory_order_relaxed)};
  reshade::api::resource_view ao_view = {g_ambient_ao_view.load(std::memory_order_relaxed)};
  reshade::api::resource_view shadow_view = {g_ambient_shadow_view.load(std::memory_order_relaxed)};
  reshade::api::resource_view env_lut_view = {g_ambient_env_lut_view.load(std::memory_order_relaxed)};
  reshade::api::resource_view ssr_view = {g_ambient_ssr_view.load(std::memory_order_relaxed)};

  bool has_albedo = IsViewAlive(albedo_view);
  const bool has_depth = IsViewAlive(depth_view);
  const bool has_ao = IsViewAlive(ao_view);
  if (!has_depth || !has_ao) return false;
  if (!has_albedo) albedo_view = data->composite_source_texture_view;

  bool has_normal = IsViewAlive(normal_view);
  bool has_shadow = IsViewAlive(shadow_view);
  bool has_specular = IsViewAlive(specular_view);
  bool has_env_lut = IsViewAlive(env_lut_view);
  bool has_ssr = IsViewAlive(ssr_view);
  if (!has_normal) normal_view = albedo_view;
  if (!has_shadow) shadow_view = ao_view;
  if (!has_specular) specular_view = albedo_view;

  auto target_resource = device->get_resource_from_view(target_rtv);
  if (target_resource.handle == 0u) return false;
  auto target_desc = device->get_resource_desc(target_resource);
  if (!EnsureCompositeSourceTexture(device, data, target_desc)) return false;

  if (!has_env_lut) env_lut_view = data->composite_source_texture_view;
  if (!has_ssr) ssr_view = data->composite_source_texture_view;

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

  std::array<float, 12> composite_data = {
      (ambient_gi_composite_enabled >= 0.5f && shader_injection.mod_enabled >= 0.5f) ? 1.f : 0.f,
      std::clamp(shader_injection.slider_1, 0.f, 100.f),
      std::clamp(shader_injection.slider_2, 0.f, 100.f),
      std::clamp(shader_injection.slider_3, 0.f, 100.f),
      std::clamp(ambient_gi_color_boost, 0.f, 400.f),
      std::clamp(ambient_gi_debug_mode, 0.f, 14.f),
      std::clamp(ambient_gi_debug_scale, 0.f, 8.f),
      std::clamp(ambient_gi_bleed_radius, 0.f, 4.f),
      std::clamp(ambient_gi_tint_source_a, 0.f, 7.f),
      std::clamp(ambient_gi_tint_source_b, 0.f, 7.f),
      std::clamp(ambient_gi_tint_source_blend, 0.f, 100.f),
      0.f,
  };

  data->composite_pass.InvalidateRenderTargets(cmd_list);
  data->composite_pass.render_target_slots.views = {target_rtv};
  data->composite_pass.shader_resource_slots.views = {
      data->composite_source_texture_view,
      albedo_view,
      normal_view,
      depth_view,
      ao_view,
      shadow_view,
      specular_view,
      ssr_view,
      env_lut_view,
  };
  data->composite_pass.sampler_descs = {reshade::api::sampler_desc{}};
  data->composite_pass.pipeline_subobjects.vertex_shader = __0xFFFFFFFD;
  data->composite_pass.pipeline_subobjects.pixel_shader = __0xFFFFFFFE;
  data->composite_pass.pipeline_subobjects.compute_shader = {};
  data->composite_pass.push_constants.clear();
  data->composite_pass.push_constants[renodx::utils::render::ConstantBuffersSlots{
      .slot = 13,
      .space = 0,
  }] = std::span<const float>(composite_data.data(), composite_data.size());
  data->composite_pass.auto_generate_descriptor_table_updates = true;
  data->composite_pass.revert_state_after_render = true;
  data->composite_pass.flush_after_render = false;
  return data->composite_pass.Render(cmd_list);
}

void OnAmbientShaderDrawnApplyComposite(reshade::api::command_list* cmd_list) {
  if (cmd_list == nullptr) return;
  if (ambient_gi_composite_enabled < 0.5f || shader_injection.mod_enabled < 0.5f) return;

  const auto frame_index = g_present_frame_index.load(std::memory_order_relaxed);
  if (g_last_composite_frame_index.load(std::memory_order_relaxed) == frame_index) return;

  auto* swapchain_state = renodx::utils::swapchain::GetCurrentState(cmd_list);
  if (swapchain_state == nullptr) return;
  if (swapchain_state->current_render_targets.empty()) return;
  auto target_rtv = swapchain_state->current_render_targets.at(0);
  if (target_rtv.handle == 0u) return;

  if (ApplyAmbientGiComposite(cmd_list, target_rtv)) {
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
        .key = "ModEnabled",
        .binding = &shader_injection.mod_enabled,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Enabled",
        .section = "Vanilla+",
        .labels = {"Off", "On"},
    },
    new renodx::utils::settings::Setting{
        .key = "AmbientGIComposite",
        .binding = &ambient_gi_composite_enabled,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Ambient GI Composite",
        .section = "Vanilla+",
        .labels = {"Off", "On"},
    },
    new renodx::utils::settings::Setting{
        .key = "AmbientGIStrength",
        .binding = &shader_injection.slider_1,
        .default_value = 25.f,
        .label = "GI Strength",
        .section = "Vanilla+",
        .tooltip = "Scales composite GI contribution derived from ambient inputs.",
        .min = 0.f,
        .max = 100.f,
    },
    new renodx::utils::settings::Setting{
        .key = "AmbientGIAlbedoSaturation",
        .binding = &shader_injection.slider_2,
        .default_value = 50.f,
        .label = "Tint Saturation",
        .section = "Vanilla+",
        .tooltip = "Controls saturation applied to the selected tint sources.",
        .min = 0.f,
        .max = 100.f,
    },
    new renodx::utils::settings::Setting{
        .key = "AmbientGIShadowInfluence",
        .binding = &shader_injection.slider_3,
        .default_value = 100.f,
        .label = "Shadow Influence",
        .section = "Vanilla+",
        .tooltip = "How much shadowing affects GI visibility.",
        .min = 0.f,
        .max = 100.f,
    },
    new renodx::utils::settings::Setting{
        .key = "AmbientGIColorBoost",
        .binding = &ambient_gi_color_boost,
        .default_value = 100.f,
        .label = "Color Boost",
        .section = "Vanilla+",
        .tooltip = "Boosts GI chroma (colorfulness) without boosting gray luma as much.",
        .min = 0.f,
        .max = 400.f,
    },
    new renodx::utils::settings::Setting{
        .key = "AmbientGIBleedRadius",
        .binding = &ambient_gi_bleed_radius,
        .default_value = 1.f,
        .label = "Bleed Radius",
        .section = "Vanilla+",
        .tooltip = "Samples nearby albedo/AO to create visible color bleed.",
        .min = 0.f,
        .max = 4.f,
    },
    new renodx::utils::settings::Setting{
        .key = "AmbientGITintSourceA",
        .binding = &ambient_gi_tint_source_a,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 2.f,
        .label = "Tint Source A",
        .section = "Vanilla+",
        .labels = {
            "Albedo",
            "Specular",
            "Scene",
            "SSR",
            "AO",
            "Shadow",
            "Normal",
            "Env LUT",
        },
    },
    new renodx::utils::settings::Setting{
        .key = "AmbientGITintSourceB",
        .binding = &ambient_gi_tint_source_b,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f,
        .label = "Tint Source B",
        .section = "Vanilla+",
        .labels = {
            "Albedo",
            "Specular",
            "Scene",
            "SSR",
            "AO",
            "Shadow",
            "Normal",
            "Env LUT",
        },
    },
    new renodx::utils::settings::Setting{
        .key = "AmbientGITintBlend",
        .binding = &ambient_gi_tint_source_blend,
        .default_value = 0.f,
        .label = "Tint Blend",
        .section = "Vanilla+",
        .tooltip = "Blends between Tint Source A (0) and B (100).",
        .min = 0.f,
        .max = 100.f,
    },
    new renodx::utils::settings::Setting{
        .key = "AmbientGIDebugMode",
        .binding = &ambient_gi_debug_mode,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f,
        .label = "Debug Mode",
        .section = "Vanilla+",
        .labels = {
            "Off",
            "GI Contrib",
            "GI Tint",
            "AO Mask",
            "Shadow Mask",
            "Edge Reject",
            "Bounce Mask",
            "AO Raw",
            "Shadow Raw",
            "Source A",
            "Source B",
            "GI Chroma",
            "Tint Source",
            "A Bleed",
            "B Bleed",
        },
    },
    new renodx::utils::settings::Setting{
        .key = "AmbientGIDebugScale",
        .binding = &ambient_gi_debug_scale,
        .default_value = 1.f,
        .label = "Debug Scale",
        .section = "Vanilla+",
        .tooltip = "Scales debug visualization intensity.",
        .min = 0.f,
        .max = 8.f,
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

      reshade::register_event<reshade::addon_event::init_device>(OnInitDevice);
      reshade::register_event<reshade::addon_event::destroy_device>(OnDestroyDevice);
      reshade::register_event<reshade::addon_event::init_swapchain>(OnInitSwapchain);
      reshade::register_event<reshade::addon_event::destroy_swapchain>(OnDestroySwapchain);
      reshade::register_event<reshade::addon_event::present>(OnPresentAdvanceFrame);
      reshade::register_event<reshade::addon_event::push_descriptors>(OnPushDescriptorsCaptureAmbientTextures);
      reshade::register_event<reshade::addon_event::bind_descriptor_tables>(OnBindDescriptorTablesCaptureAmbientTextures);

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
      reshade::unregister_event<reshade::addon_event::push_descriptors>(OnPushDescriptorsCaptureAmbientTextures);
      reshade::unregister_event<reshade::addon_event::bind_descriptor_tables>(OnBindDescriptorTablesCaptureAmbientTextures);
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
