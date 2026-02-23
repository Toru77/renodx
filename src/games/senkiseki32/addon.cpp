/*
 * Copyright (C) 2024 Carlos Lopez
 * SPDX-License-Identifier: MIT
 */

#define ImTextureID ImU64

#define DEBUG_LEVEL_0

#include <mutex>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

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
constexpr bool kTraceUiKanjiVariants = true;

struct UiKanjiDrawState {
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
  uint64_t view_handle = 0u;
  uint32_t width = 0u;
  uint32_t height = 0u;
  uint32_t format = 0u;
};

struct UiKanjiPipelineCacheKey {
  reshade::api::device* device = nullptr;
  uint64_t original_pipeline_handle = 0u;

  bool operator==(const UiKanjiPipelineCacheKey& other) const {
    return device == other.device
           && original_pipeline_handle == other.original_pipeline_handle;
  }
};

struct UiKanjiPipelineCacheKeyHash {
  size_t operator()(const UiKanjiPipelineCacheKey& key) const {
    const size_t h1 = std::hash<void*>{}(key.device);
    const size_t h2 = std::hash<uint64_t>{}(key.original_pipeline_handle);
    return h1 ^ (h2 + 0x9e3779b97f4a7c15ULL + (h1 << 6) + (h1 >> 2));
  }
};

thread_local UiKanjiDrawState ui_kanji_draw_state = {};
thread_local UiKanjiDrawCallInfo ui_kanji_draw_call_info = {};
thread_local UiKanjiSourceTextureInfo ui_kanji_source_texture_info = {};
std::mutex ui_kanji_pipeline_cache_mutex;
std::unordered_map<UiKanjiPipelineCacheKey, reshade::api::pipeline, UiKanjiPipelineCacheKeyHash> ui_kanji_pipeline_cache;
std::mutex ui_kanji_trace_mutex;
std::unordered_set<uint64_t> ui_kanji_seen_variants;
std::mutex ui_kanji_draw_trace_mutex;
std::unordered_set<uint64_t> ui_kanji_seen_draws;
std::mutex ui_kanji_source_trace_mutex;
std::unordered_set<uint64_t> ui_kanji_seen_sources;

uint64_t HashCombine(uint64_t seed, uint64_t value) {
  return seed ^ (value + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2));
}

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

void TraceUiKanjiVariant(
    reshade::api::command_list* cmd_list,
    renodx::utils::shader::CommandListData* shader_state,
    renodx::utils::shader::StageState* pixel_state) {
  if (!kTraceUiKanjiVariants) return;
  if (cmd_list == nullptr || shader_state == nullptr || pixel_state == nullptr || pixel_state->pipeline_details == nullptr) {
    return;
  }

  auto* device = cmd_list->get_device();
  if (device == nullptr) return;

  const uint32_t vertex_hash = renodx::utils::shader::GetCurrentVertexShaderHash(shader_state);
  const uint64_t output_merger_handle = pixel_state->pipeline.handle;

  bool has_blend = false;
  bool blend_enable = false;
  uint32_t src_color = 0u;
  uint32_t dst_color = 0u;
  uint32_t color_op = 0u;
  uint32_t src_alpha = 0u;
  uint32_t dst_alpha = 0u;
  uint32_t alpha_op = 0u;
  uint32_t write_mask = 0u;
  bool has_depth_stencil = false;
  bool depth_enable = false;
  bool stencil_enable = false;

  for (const auto& subobject : pixel_state->pipeline_details->subobjects) {
    if (subobject.type == reshade::api::pipeline_subobject_type::blend_state
        && subobject.count > 0u
        && subobject.data != nullptr) {
      const auto* blend_desc = static_cast<const reshade::api::blend_desc*>(subobject.data);
      has_blend = true;
      blend_enable = blend_desc[0].blend_enable[0];
      src_color = static_cast<uint32_t>(blend_desc[0].source_color_blend_factor[0]);
      dst_color = static_cast<uint32_t>(blend_desc[0].dest_color_blend_factor[0]);
      color_op = static_cast<uint32_t>(blend_desc[0].color_blend_op[0]);
      src_alpha = static_cast<uint32_t>(blend_desc[0].source_alpha_blend_factor[0]);
      dst_alpha = static_cast<uint32_t>(blend_desc[0].dest_alpha_blend_factor[0]);
      alpha_op = static_cast<uint32_t>(blend_desc[0].alpha_blend_op[0]);
      write_mask = static_cast<uint32_t>(blend_desc[0].render_target_write_mask[0]);
    } else if (subobject.type == reshade::api::pipeline_subobject_type::depth_stencil_state
               && subobject.count > 0u
               && subobject.data != nullptr) {
      const auto* depth_desc = static_cast<const reshade::api::depth_stencil_desc*>(subobject.data);
      has_depth_stencil = true;
      depth_enable = depth_desc[0].depth_enable;
      stencil_enable = depth_desc[0].stencil_enable;
    }
  }

  uint32_t rtv0_width = 0u;
  uint32_t rtv0_height = 0u;
  uint32_t rtv0_format = 0u;
  auto& rtvs = renodx::utils::swapchain::GetRenderTargets(cmd_list);
  if (!rtvs.empty() && rtvs[0].handle != 0u) {
    auto* view_info = renodx::utils::resource::GetResourceViewInfo(rtvs[0]);
    if (view_info != nullptr && view_info->resource_info != nullptr) {
      rtv0_width = view_info->resource_info->desc.texture.width;
      rtv0_height = view_info->resource_info->desc.texture.height;
      rtv0_format = static_cast<uint32_t>(view_info->resource_info->desc.texture.format);
    }
  }

  uint64_t signature = 0x7a3c9d55aa2b7f19ULL;
  signature = HashCombine(signature, static_cast<uint64_t>(vertex_hash));
  signature = HashCombine(signature, output_merger_handle);
  signature = HashCombine(signature, static_cast<uint64_t>(has_blend));
  signature = HashCombine(signature, static_cast<uint64_t>(blend_enable));
  signature = HashCombine(signature, static_cast<uint64_t>(src_color));
  signature = HashCombine(signature, static_cast<uint64_t>(dst_color));
  signature = HashCombine(signature, static_cast<uint64_t>(color_op));
  signature = HashCombine(signature, static_cast<uint64_t>(src_alpha));
  signature = HashCombine(signature, static_cast<uint64_t>(dst_alpha));
  signature = HashCombine(signature, static_cast<uint64_t>(alpha_op));
  signature = HashCombine(signature, static_cast<uint64_t>(write_mask));
  signature = HashCombine(signature, static_cast<uint64_t>(has_depth_stencil));
  signature = HashCombine(signature, static_cast<uint64_t>(depth_enable));
  signature = HashCombine(signature, static_cast<uint64_t>(stencil_enable));
  signature = HashCombine(signature, static_cast<uint64_t>(rtv0_width));
  signature = HashCombine(signature, static_cast<uint64_t>(rtv0_height));
  signature = HashCombine(signature, static_cast<uint64_t>(rtv0_format));

  bool should_log = false;
  {
    std::lock_guard<std::mutex> lock(ui_kanji_trace_mutex);
    should_log = ui_kanji_seen_variants.emplace(signature).second;
  }
  if (!should_log) return;

  std::stringstream s;
  s << "senkiseki32::ui976fcd64 variant"
    << " vs=0x" << std::hex << vertex_hash << std::dec
    << " om=0x" << std::hex << output_merger_handle << std::dec
    << " rtv0=" << rtv0_width << "x" << rtv0_height
    << " fmt=" << rtv0_format
    << " blend0=" << (has_blend ? 1 : 0)
    << "/" << (blend_enable ? 1 : 0)
    << " c(" << src_color << "," << dst_color << "," << color_op << ")"
    << " a(" << src_alpha << "," << dst_alpha << "," << alpha_op << ")"
    << " mask=" << write_mask
    << " ds=" << (has_depth_stencil ? 1 : 0)
    << "/" << (depth_enable ? 1 : 0)
    << "/" << (stencil_enable ? 1 : 0);
  reshade::log::message(reshade::log::level::info, s.str().c_str());
}

void TraceUiKanjiDrawCall(
    reshade::api::command_list* cmd_list,
    bool indexed,
    uint32_t draw_count,
    uint32_t instance_count,
    uint32_t first_value,
    int32_t vertex_offset,
    uint32_t first_instance) {
  if (!kTraceUiKanjiVariants) return;
  if (cmd_list == nullptr) return;

  auto* shader_state = renodx::utils::shader::GetCurrentState(cmd_list);
  if (shader_state == nullptr) return;
  const uint32_t pixel_hash = renodx::utils::shader::GetCurrentPixelShaderHash(shader_state);
  if (pixel_hash != kUiKanjiShader) return;

  const uint32_t vertex_hash = renodx::utils::shader::GetCurrentVertexShaderHash(shader_state);
  auto* pixel_state = renodx::utils::shader::GetCurrentPixelState(shader_state);
  const uint64_t output_merger_handle = (pixel_state != nullptr) ? pixel_state->pipeline.handle : 0u;

  uint32_t rtv0_width = 0u;
  uint32_t rtv0_height = 0u;
  uint32_t rtv0_format = 0u;
  auto& rtvs = renodx::utils::swapchain::GetRenderTargets(cmd_list);
  if (!rtvs.empty() && rtvs[0].handle != 0u) {
    auto* view_info = renodx::utils::resource::GetResourceViewInfo(rtvs[0]);
    if (view_info != nullptr && view_info->resource_info != nullptr) {
      rtv0_width = view_info->resource_info->desc.texture.width;
      rtv0_height = view_info->resource_info->desc.texture.height;
      rtv0_format = static_cast<uint32_t>(view_info->resource_info->desc.texture.format);
    }
  }

  uint64_t signature = 0x51f2d7a4d1c39c17ULL;
  signature = HashCombine(signature, static_cast<uint64_t>(indexed));
  signature = HashCombine(signature, static_cast<uint64_t>(vertex_hash));
  signature = HashCombine(signature, output_merger_handle);
  signature = HashCombine(signature, static_cast<uint64_t>(draw_count));
  signature = HashCombine(signature, static_cast<uint64_t>(instance_count));
  signature = HashCombine(signature, static_cast<uint64_t>(first_value));
  signature = HashCombine(signature, static_cast<uint64_t>(static_cast<uint32_t>(vertex_offset)));
  signature = HashCombine(signature, static_cast<uint64_t>(first_instance));
  signature = HashCombine(signature, static_cast<uint64_t>(rtv0_width));
  signature = HashCombine(signature, static_cast<uint64_t>(rtv0_height));
  signature = HashCombine(signature, static_cast<uint64_t>(rtv0_format));

  bool should_log = false;
  {
    std::lock_guard<std::mutex> lock(ui_kanji_draw_trace_mutex);
    should_log = ui_kanji_seen_draws.emplace(signature).second;
  }
  if (!should_log) return;

  std::stringstream s;
  s << "senkiseki32::ui976fcd64 draw"
    << " kind=" << (indexed ? "indexed" : "draw")
    << " vs=0x" << std::hex << vertex_hash << std::dec
    << " om=0x" << std::hex << output_merger_handle << std::dec
    << " count=" << draw_count
    << " inst=" << instance_count
    << " first=" << first_value
    << " vo=" << vertex_offset
    << " fi=" << first_instance
    << " rtv0=" << rtv0_width << "x" << rtv0_height
    << " fmt=" << rtv0_format;
  reshade::log::message(reshade::log::level::info, s.str().c_str());
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

void OnPushDescriptorsUiKanjiTrace(
    reshade::api::command_list* cmd_list,
    reshade::api::shader_stage stages,
    reshade::api::pipeline_layout layout,
    uint32_t layout_param,
    const reshade::api::descriptor_table_update& update) {
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
    const uint32_t format = static_cast<uint32_t>(view_info->resource_info->desc.texture.format);
    ui_kanji_source_texture_info = {
        .cmd_list = cmd_list,
        .valid = true,
        .view_handle = view.handle,
        .width = width,
        .height = height,
        .format = format,
    };

    uint64_t signature = 0xa39e7c81f2d1b547ULL;
    signature = HashCombine(signature, static_cast<uint64_t>(layout.handle));
    signature = HashCombine(signature, static_cast<uint64_t>(layout_param));
    signature = HashCombine(signature, static_cast<uint64_t>(binding));
    signature = HashCombine(signature, static_cast<uint64_t>(view.handle));
    signature = HashCombine(signature, static_cast<uint64_t>(width));
    signature = HashCombine(signature, static_cast<uint64_t>(height));
    signature = HashCombine(signature, static_cast<uint64_t>(format));
    signature = HashCombine(signature, static_cast<uint64_t>(update.type));

    bool should_log = false;
    {
      std::lock_guard<std::mutex> lock(ui_kanji_source_trace_mutex);
      should_log = ui_kanji_seen_sources.emplace(signature).second;
    }
    if (should_log && kTraceUiKanjiVariants) {
      std::stringstream s;
      s << "senkiseki32::ui976fcd64 src"
        << " stage=0x" << std::hex << static_cast<uint32_t>(stages) << std::dec
        << " layout=0x" << std::hex << layout.handle << std::dec
        << " lp=" << layout_param
        << " binding=" << binding
        << " view=0x" << std::hex << view.handle << std::dec
        << " dim=" << width << "x" << height
        << " fmt=" << format
        << " type=" << static_cast<uint32_t>(update.type);
      reshade::log::message(reshade::log::level::info, s.str().c_str());
    }
  }
}

bool OnDrawUiKanjiTrace(
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
  TraceUiKanjiDrawCall(
      cmd_list,
      false,
      vertex_count,
      instance_count,
      first_vertex,
      0,
      first_instance);
  return false;
}

bool OnDrawIndexedUiKanjiTrace(
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
  TraceUiKanjiDrawCall(
      cmd_list,
      true,
      index_count,
      instance_count,
      first_index,
      vertex_offset,
      first_instance);
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
  TraceUiKanjiVariant(cmd_list, shader_state, pixel_state);

  auto* device = cmd_list->get_device();
  if (device == nullptr) return true;

  const auto original_output_merger = pixel_state->pipeline;
  const UiKanjiPipelineCacheKey cache_key = {
      .device = device,
      .original_pipeline_handle = original_output_merger.handle,
  };

  reshade::api::pipeline override_pipeline = {0u};
  {
    std::lock_guard<std::mutex> lock(ui_kanji_pipeline_cache_mutex);
    auto it = ui_kanji_pipeline_cache.find(cache_key);
    if (it != ui_kanji_pipeline_cache.end()) {
      override_pipeline = it->second;
    }
  }

  if (override_pipeline.handle == 0u) {
    reshade::api::blend_desc blend_state = {};
    bool disable_depth_stencil = false;
    if (!TryBuildUiKanjiBlendOverride(pixel_state->pipeline_details, &blend_state, &disable_depth_stencil)) {
      const uint32_t vertex_hash = renodx::utils::shader::GetCurrentVertexShaderHash(shader_state);
      const bool has_source_info = HasUiKanjiSourceTextureInfo(cmd_list);
      if (vertex_hash != kUiKanjiFallbackVertexShader
          || !IsLikelyUiKanjiDrawCall(cmd_list)
          || (has_source_info && !IsLikelyUiKanjiSourceTexture(cmd_list))) {
        return true;
      }

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
      blend_state.render_target_write_mask[0] = 0x7u;
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

    reshade::api::pipeline created_override_pipeline = {0u};
    if (!device->create_pipeline(
            pixel_state->pipeline_details->layout,
            subobject_count,
            subobjects,
            &created_override_pipeline)) {
      return true;
    }

    reshade::api::pipeline duplicate_pipeline = {0u};
    {
      std::lock_guard<std::mutex> lock(ui_kanji_pipeline_cache_mutex);
      auto [it, inserted] = ui_kanji_pipeline_cache.emplace(cache_key, created_override_pipeline);
      if (!inserted) {
        override_pipeline = it->second;
        duplicate_pipeline = created_override_pipeline;
      } else {
        override_pipeline = created_override_pipeline;
      }
    }
    if (duplicate_pipeline.handle != 0u) {
      device->destroy_pipeline(duplicate_pipeline);
    }
  }

  ui_kanji_draw_state.original_output_merger = original_output_merger;
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

  ui_kanji_draw_state = {};
}

void OnDestroyPipelineUiKanjiState(
    reshade::api::device* device,
    reshade::api::pipeline pipeline) {
  std::vector<reshade::api::pipeline> pipelines_to_destroy;
  {
    std::lock_guard<std::mutex> lock(ui_kanji_pipeline_cache_mutex);
    for (auto it = ui_kanji_pipeline_cache.begin(); it != ui_kanji_pipeline_cache.end();) {
      if (it->first.device != device) {
        ++it;
        continue;
      }

      const bool original_destroyed = it->first.original_pipeline_handle == pipeline.handle;
      const bool override_destroyed = it->second.handle == pipeline.handle;
      if (!original_destroyed && !override_destroyed) {
        ++it;
        continue;
      }

      if (!override_destroyed && it->second.handle != 0u) {
        pipelines_to_destroy.push_back(it->second);
      }
      it = ui_kanji_pipeline_cache.erase(it);
    }
  }

  for (const auto& cached_pipeline : pipelines_to_destroy) {
    device->destroy_pipeline(cached_pipeline);
  }
}

void OnDestroyDeviceUiKanjiState(reshade::api::device* device) {
  std::vector<reshade::api::pipeline> pipelines_to_destroy;
  {
    std::lock_guard<std::mutex> lock(ui_kanji_pipeline_cache_mutex);
    for (auto it = ui_kanji_pipeline_cache.begin(); it != ui_kanji_pipeline_cache.end();) {
      if (it->first.device != device) {
        ++it;
        continue;
      }
      if (it->second.handle != 0u) {
        pipelines_to_destroy.push_back(it->second);
      }
      it = ui_kanji_pipeline_cache.erase(it);
    }
  }

  for (const auto& cached_pipeline : pipelines_to_destroy) {
    device->destroy_pipeline(cached_pipeline);
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
    CustomShaderEntry(0x3A2360B2),  // output: color buffer passthrough
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
        .default_value = 1.f,
        .label = "Gamma Correction",
        .section = "Tone Mapping",
        .tooltip = "Emulates a display EOTF.",
        .labels = {"Off", "2.2", "BT.1886", "Falcom (2.3)"},
        .is_visible = []() { return current_settings_mode >= 1; },
    },
    new renodx::utils::settings::Setting{
        .key = "ToneMapScaling",
        .binding = &shader_injection.tone_map_per_channel,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 1.f,
        .label = "Scaling",
        .section = "Tone Mapping",
        .tooltip = "Luminance scales colors consistently while per-channel saturates and blows out sooner",
        .labels = {"Luminance", "Per Channel"},
        .is_enabled = []() { return shader_injection.tone_map_type >= 1; },
        .is_visible = []() { return current_settings_mode >= 2; },
    },
    new renodx::utils::settings::Setting{
        .key = "ToneMapWorkingColorSpace",
        .binding = &shader_injection.tone_map_working_color_space,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f,
        .label = "Working Color Space",
        .section = "Tone Mapping",
        .labels = {"BT709", "BT2020", "AP1"},
        .is_enabled = []() { return shader_injection.tone_map_type >= 1; },
        .is_visible = []() { return current_settings_mode >= 2; },
    },
    new renodx::utils::settings::Setting{
        .key = "ToneMapHueProcessor",
        .binding = &shader_injection.tone_map_hue_processor,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f,
        .label = "Hue Processor",
        .section = "Tone Mapping",
        .tooltip = "Selects hue processor",
        .labels = {"OKLab", "ICtCp", "darkTable UCS"},
        .is_enabled = []() { return shader_injection.tone_map_type >= 1; },
        .is_visible = []() { return current_settings_mode >= 2; },
    },
    new renodx::utils::settings::Setting{
        .key = "ToneMapHueCorrection",
        .binding = &shader_injection.tone_map_hue_correction,
        .default_value = 100.f,
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
        .key = "ToneMapHueShift",
        .binding = &shader_injection.tone_map_hue_shift,
        .default_value = 50.f,
        .label = "Hue Shift",
        .section = "Tone Mapping",
        .tooltip = "Hue-shift emulation strength.",
        .min = 0.f,
        .max = 100.f,
        .is_enabled = []() { return shader_injection.tone_map_type >= 1; },
        .parse = [](float value) { return value * 0.01f; },
        .is_visible = []() { return current_settings_mode >= 1; },
    },
    new renodx::utils::settings::Setting{
        .key = "ToneMapClampColorSpace",
        .binding = &shader_injection.tone_map_clamp_color_space,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f,
        .label = "Clamp Color Space",
        .section = "Tone Mapping",
        .tooltip = "Hue-shift emulation strength.",
        .labels = {"None", "BT709", "BT2020", "AP1"},
        .is_enabled = []() { return shader_injection.tone_map_type >= 1; },
        .parse = [](float value) { return value - 1.f; },
        .is_visible = []() { return current_settings_mode >= 2; },
    },
    new renodx::utils::settings::Setting{
        .key = "ToneMapClampPeak",
        .binding = &shader_injection.tone_map_clamp_peak,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f,
        .label = "Clamp Peak",
        .section = "Tone Mapping",
        .tooltip = "Hue-shift emulation strength.",
        .labels = {"None", "BT709", "BT2020", "AP1"},
        .is_enabled = []() { return shader_injection.tone_map_type >= 1; },
        .parse = [](float value) { return value - 1.f; },
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
        .key = "ColorGradeScene",
        .binding = &shader_injection.color_grade_strength,
        .default_value = 100.f,
        .label = "Scene Grading",
        .section = "Color Grading",
        .tooltip = "Scene grading as applied by the game",
        .max = 100.f,
        .is_enabled = []() { return shader_injection.tone_map_type > 0; },
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
        .is_visible = []() { return settings[0]->GetValue() >= 1; },
    },
    new renodx::utils::settings::Setting{
        .key = "IntermediateDecoding",
        .binding = &shader_injection.intermediate_encoding,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f,
        .label = "Intermediate Encoding",
        .section = "Display Output",
        .labels = {"Auto", "None", "SRGB", "2.2", "2.4"},
        .is_enabled = []() { return shader_injection.tone_map_type >= 1; },
        .parse = [](float value) {
            if (value == 0) {
              float gc = shader_injection.gamma_correction;
              if (gc == 3.f) return 2.f;  // Falcom 2.3 -> use 2.2 encoding
              return gc + 1.f;
            }
            return value - 1.f; },
        .is_visible = []() { return current_settings_mode >= 2; },
    },
    new renodx::utils::settings::Setting{
        .key = "SwapChainDecoding",
        .binding = &shader_injection.swap_chain_decoding,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f,
        .label = "Swapchain Decoding",
        .section = "Display Output",
        .labels = {"Auto", "None", "SRGB", "2.2", "2.4"},
        .is_enabled = []() { return shader_injection.tone_map_type >= 1; },
        .parse = [](float value) {
            if (value == 0) return shader_injection.intermediate_encoding;
            return value - 1.f; },
        .is_visible = []() { return current_settings_mode >= 2; },
    },
    new renodx::utils::settings::Setting{
        .key = "SwapChainGammaCorrection",
        .binding = &shader_injection.swap_chain_gamma_correction,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f,
        .label = "Gamma Correction",
        .section = "Display Output",
        .labels = {"None", "2.2", "2.4"},
        .is_enabled = []() { return shader_injection.tone_map_type >= 1; },
        .is_visible = []() { return current_settings_mode >= 2; },
    },
    new renodx::utils::settings::Setting{
        .key = "SwapChainClampColorSpace",
        .binding = &shader_injection.swap_chain_clamp_color_space,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 2.f,
        .label = "Clamp Color Space",
        .section = "Display Output",
        .labels = {"None", "BT709", "BT2020", "AP1"},
        .is_enabled = []() { return shader_injection.tone_map_type >= 1; },
        .parse = [](float value) { return value - 1.f; },
        .is_visible = []() { return current_settings_mode >= 2; },
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

const auto UPGRADE_TYPE_NONE = 0.f;
const auto UPGRADE_TYPE_OUTPUT_SIZE = 1.f;
const auto UPGRADE_TYPE_OUTPUT_RATIO = 2.f;
const auto UPGRADE_TYPE_ANY = 3.f;

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
      reshade::register_event<reshade::addon_event::draw>(OnDrawUiKanjiTrace);
      reshade::register_event<reshade::addon_event::draw_indexed>(OnDrawIndexedUiKanjiTrace);
      reshade::register_event<reshade::addon_event::push_descriptors>(OnPushDescriptorsUiKanjiTrace);

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
              .key = "SwapChainForceBorderless",
              .value_type = renodx::utils::settings::SettingValueType::INTEGER,
              .default_value = 0.f,
              .label = "Force Borderless",
              .section = "Display Output",
              .tooltip = "Forces fullscreen to be borderless for proper HDR",
              .labels = {
                  "Disabled",
                  "Enabled",
              },
              .on_change_value = [](float previous, float current) { renodx::mods::swapchain::force_borderless = (current == 1.f); },
              .is_global = true,
              .is_visible = []() { return current_settings_mode >= 2; },
          };
          renodx::utils::settings::LoadSetting(renodx::utils::settings::global_name, setting);
          renodx::mods::swapchain::force_borderless = (setting->GetValue() == 1.f);
          settings.push_back(setting);
        }

        {
          auto* setting = new renodx::utils::settings::Setting{
              .key = "SwapChainPreventFullscreen",
              .value_type = renodx::utils::settings::SettingValueType::INTEGER,
              .default_value = 0.f,
              .label = "Prevent Fullscreen",
              .section = "Display Output",
              .tooltip = "Prevent exclusive fullscreen for proper HDR",
              .labels = {
                  "Disabled",
                  "Enabled",
              },
              .on_change_value = [](float previous, float current) { renodx::mods::swapchain::prevent_full_screen = (current == 1.f); },
              .is_global = true,
              .is_visible = []() { return current_settings_mode >= 2; },
          };
          renodx::utils::settings::LoadSetting(renodx::utils::settings::global_name, setting);
          renodx::mods::swapchain::prevent_full_screen = (setting->GetValue() == 1.f);
          settings.push_back(setting);
        }

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

        {
          auto* setting = new renodx::utils::settings::Setting{
              .key = "SwapChainDeviceProxy",
              .value_type = renodx::utils::settings::SettingValueType::INTEGER,
              .default_value = 0.f,
              .label = "Use Display Proxy",
              .section = "Display Proxy",
              .labels = {"Off", "On"},
              .is_global = true,
              .is_visible = []() { return current_settings_mode >= 2; },
          };
          renodx::utils::settings::LoadSetting(renodx::utils::settings::global_name, setting);
          bool use_device_proxy = setting->GetValue() == 1.f;
          renodx::mods::swapchain::use_device_proxy = use_device_proxy;
          renodx::mods::swapchain::set_color_space = !use_device_proxy;
          if (use_device_proxy) {
            reshade::register_event<reshade::addon_event::present>(OnPresent);
          } else {
            shader_injection.custom_flip_uv_y = 0.f;
          }
          settings.push_back(setting);
        }

        {
          auto* setting = new renodx::utils::settings::Setting{
              .key = "SwapChainDeviceProxyBaseWaitIdle",
              .value_type = renodx::utils::settings::SettingValueType::INTEGER,
              .default_value = 0.f,
              .label = "Base Wait Idle",
              .section = "Display Proxy",
              .labels = {"Off", "On"},
              .is_global = true,
              .is_visible = []() { return current_settings_mode >= 2; },
          };
          renodx::utils::settings::LoadSetting(renodx::utils::settings::global_name, setting);
          bool use_device_proxy =
              renodx::mods::swapchain::device_proxy_wait_idle_source = (setting->GetValue() == 1.f);
          settings.push_back(setting);
        }

        {
          auto* setting = new renodx::utils::settings::Setting{
              .key = "SwapChainDeviceProxyProxyWaitIdle",
              .value_type = renodx::utils::settings::SettingValueType::INTEGER,
              .default_value = 0.f,
              .label = "Proxy Wait Idle",
              .section = "Display Proxy",
              .labels = {"Off", "On"},
              .is_global = true,
              .is_visible = []() { return current_settings_mode >= 2; },
          };
          renodx::utils::settings::LoadSetting(renodx::utils::settings::global_name, setting);
          bool use_device_proxy =
              renodx::mods::swapchain::device_proxy_wait_idle_destination = (setting->GetValue() == 1.f);
          settings.push_back(setting);
        }

        for (const auto& [key, format] : UPGRADE_TARGETS) {
          auto* setting = new renodx::utils::settings::Setting{
              .key = "Upgrade_" + key,
              .value_type = renodx::utils::settings::SettingValueType::INTEGER,
              .default_value = 0.f,
              .label = key,
              .section = "Resource Upgrades",
              .labels = {
                  "Off",
                  "Output size",
                  "Output ratio",
                  "Any size",
              },
              .is_global = true,
              .is_visible = []() { return settings[0]->GetValue() >= 2; },
          };
          renodx::utils::settings::LoadSetting(renodx::utils::settings::global_name, setting);
          settings.push_back(setting);

          auto value = setting->GetValue();
          if (value > 0) {
            renodx::mods::swapchain::swap_chain_upgrade_targets.push_back({
                .old_format = format,
                .new_format = reshade::api::format::r16g16b16a16_float,
                .ignore_size = (value == UPGRADE_TYPE_ANY),
                .use_resource_view_cloning = true,
                .aspect_ratio = static_cast<float>((value == UPGRADE_TYPE_OUTPUT_RATIO)
                                                       ? renodx::mods::swapchain::SwapChainUpgradeTarget::BACK_BUFFER
                                                       : renodx::mods::swapchain::SwapChainUpgradeTarget::ANY),
                .usage_include = reshade::api::resource_usage::render_target,
            });
            std::stringstream s;
            s << "Applying user resource upgrade for ";
            s << format << ": " << value;
            reshade::log::message(reshade::log::level::info, s.str().c_str());
          }
        }

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
      reshade::unregister_event<reshade::addon_event::draw>(OnDrawUiKanjiTrace);
      reshade::unregister_event<reshade::addon_event::draw_indexed>(OnDrawIndexedUiKanjiTrace);
      reshade::unregister_event<reshade::addon_event::push_descriptors>(OnPushDescriptorsUiKanjiTrace);
      reshade::unregister_event<reshade::addon_event::present>(OnPresent);
      reshade::unregister_addon(h_module);
      break;
  }

  renodx::utils::settings::Use(fdw_reason, &settings, &OnPresetOff);
  renodx::mods::swapchain::Use(fdw_reason, &shader_injection);
  renodx::mods::shader::Use(fdw_reason, custom_shaders, &shader_injection);

  return TRUE;
}
