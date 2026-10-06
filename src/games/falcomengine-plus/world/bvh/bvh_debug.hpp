#pragma once

// Phase 1 M2b: raster reconstruction debug view.
//
// Uploads the M1 CPU pool to GPU buffers (once per pool revision), then
// rasterizes the actual pool triangles with the game camera into a debug
// texture and replaces the backbuffer with it. Modes: instance id, mesh id,
// world position, flat normal, instance AABB. This is the human-readable
// validation path for the GPU pool and the raster reference for M4.

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <vector>

#include "../../../../utils/render.hpp"
#include "bvh_build.hpp"
#include "bvh_recipes.hpp"
#include "bvh_resources.hpp"
#include "bvh_trace.hpp"

namespace falcom_world::bvh {

inline constexpr uint32_t kRasterPushConstantCount = 24u;

struct BvhDebugState {
  std::atomic_int mode{0};
  std::atomic_bool force_upload{false};
  std::atomic<reshade::api::device*> device{nullptr};
};

inline BvhDebugState g_bvh_debug;

inline void DestroyBvhDebugResources(reshade::api::device* device) {
  BvhDeviceData* data = GetBvhDeviceData(device);
  if (data == nullptr) return;
  if (data->raster_pipeline.handle != 0u) {
    device->destroy_pipeline(data->raster_pipeline);
    data->raster_pipeline = {0u};
  }
  if (data->aabb_pipeline.handle != 0u) {
    device->destroy_pipeline(data->aabb_pipeline);
    data->aabb_pipeline = {0u};
  }
  for (auto& table : data->raster_tables) {
    if (table.handle != 0u) {
      device->free_descriptor_table(table);
      table = {0u};
    }
  }
  if (data->raster_layout.handle != 0u) {
    device->destroy_pipeline_layout(data->raster_layout);
    data->raster_layout = {0u};
  }
  DestroyBuffer(device, &data->debug_srv, &data->debug_texture);
  data->debug_rtv = {0u};
  if (data->debug_uav.handle != 0u) {
    device->destroy_resource_view(data->debug_uav);
    data->debug_uav = {0u};
  }
  if (data->depth_dsv.handle != 0u) {
    device->destroy_resource_view(data->depth_dsv);
    data->depth_dsv = {0u};
  }
  if (data->depth_texture.handle != 0u) {
    device->destroy_resource(data->depth_texture);
    data->depth_texture = {0u};
  }
  data->raster_ready = false;
  data->debug_width = 0u;
  data->debug_height = 0u;
}

inline bool EnsureBvhRasterResources(reshade::api::device* device, uint32_t width, uint32_t height) {
  BvhDeviceData* data = GetBvhDeviceData(device);
  if (data == nullptr || width == 0u || height == 0u) return false;
  if (data->raster_ready && data->debug_width == width && data->debug_height == height) return true;
  DestroyBvhDebugResources(device);

  reshade::api::resource_desc color_desc = {};
  color_desc.type = reshade::api::resource_type::texture_2d;
  color_desc.texture.width = width;
  color_desc.texture.height = height;
  color_desc.texture.depth_or_layers = 1u;
  color_desc.texture.levels = 1u;
  color_desc.texture.format = reshade::api::format::r8g8b8a8_unorm;
  color_desc.texture.samples = 1u;
  color_desc.heap = reshade::api::memory_heap::gpu_only;
  color_desc.usage = reshade::api::resource_usage::render_target
                     | reshade::api::resource_usage::shader_resource
                     | reshade::api::resource_usage::unordered_access;
  if (!device->create_resource(color_desc, nullptr, reshade::api::resource_usage::render_target, &data->debug_texture)) {
    return false;
  }
  if (!device->create_resource_view(
          data->debug_texture, reshade::api::resource_usage::render_target,
          reshade::api::resource_view_desc(
              reshade::api::resource_view_type::texture_2d, reshade::api::format::r8g8b8a8_unorm, 0, 1, 0, 1),
          &data->debug_rtv)) {
    DestroyBvhDebugResources(device);
    return false;
  }
  if (!device->create_resource_view(
          data->debug_texture, reshade::api::resource_usage::shader_resource,
          reshade::api::resource_view_desc(
              reshade::api::resource_view_type::texture_2d, reshade::api::format::r8g8b8a8_unorm, 0, 1, 0, 1),
          &data->debug_srv)) {
    DestroyBvhDebugResources(device);
    return false;
  }
  if (!device->create_resource_view(
          data->debug_texture, reshade::api::resource_usage::unordered_access,
          reshade::api::resource_view_desc(
              reshade::api::resource_view_type::texture_2d, reshade::api::format::r8g8b8a8_unorm, 0, 1, 0, 1),
          &data->debug_uav)) {
    DestroyBvhDebugResources(device);
    return false;
  }

  reshade::api::resource_desc depth_desc = {};
  depth_desc.type = reshade::api::resource_type::texture_2d;
  depth_desc.texture.width = width;
  depth_desc.texture.height = height;
  depth_desc.texture.depth_or_layers = 1u;
  depth_desc.texture.levels = 1u;
  depth_desc.texture.format = reshade::api::format::d32_float;
  depth_desc.texture.samples = 1u;
  depth_desc.heap = reshade::api::memory_heap::gpu_only;
  depth_desc.usage = reshade::api::resource_usage::depth_stencil;
  if (!device->create_resource(depth_desc, nullptr, reshade::api::resource_usage::depth_stencil, &data->depth_texture)) {
    DestroyBvhDebugResources(device);
    return false;
  }
  if (!device->create_resource_view(
          data->depth_texture, reshade::api::resource_usage::depth_stencil,
          reshade::api::resource_view_desc(
              reshade::api::resource_view_type::texture_2d, reshade::api::format::d32_float, 0, 1, 0, 1),
          &data->depth_dsv)) {
    DestroyBvhDebugResources(device);
    return false;
  }

#if defined(__world_bvh_raster_vs_EMBED_FILE) && defined(__world_bvh_raster_ps_EMBED_FILE) && defined(__world_bvh_aabb_vs_EMBED_FILE)
  using DR = reshade::api::descriptor_range;
  using DS = reshade::api::shader_stage;
  using DT = reshade::api::descriptor_type;
  using P = reshade::api::pipeline_layout_param;

  DR srv_range = {0, 0, 0, 5, DS::all_graphics, 1, DT::shader_resource_view};
  reshade::api::constant_range push_range = {};
  push_range.binding = 0;
  push_range.dx_register_index = 13;
  push_range.dx_register_space = 0;
  push_range.count = kRasterPushConstantCount;
  push_range.visibility = DS::all_graphics;
  P p0 = {};
  P p_push = {};
  p0.type = reshade::api::pipeline_layout_param_type::descriptor_table;
  p0.descriptor_table.count = 1;
  p0.descriptor_table.ranges = &srv_range;
  p_push.type = reshade::api::pipeline_layout_param_type::push_constants;
  p_push.push_constants = push_range;
  P params[2] = {p0, p_push};
  if (!device->create_pipeline_layout(2, params, &data->raster_layout)) {
    DestroyBvhDebugResources(device);
    return false;
  }
  if (!device->allocate_descriptor_table(data->raster_layout, 0, &data->raster_tables[0])
      || !device->allocate_descriptor_table(data->raster_layout, 0, &data->raster_tables[1])) {
    DestroyBvhDebugResources(device);
    return false;
  }

  const auto make_shader = [](std::span<const uint8_t> code, reshade::api::shader_desc* out) {
    *out = {};
    out->code = code.data();
    out->code_size = code.size();
    out->entry_point = "main";
  };
  reshade::api::shader_desc raster_vs = {};
  reshade::api::shader_desc raster_ps = {};
  reshade::api::shader_desc aabb_vs = {};
  make_shader(__world_bvh_raster_vs, &raster_vs);
  make_shader(__world_bvh_raster_ps, &raster_ps);
  make_shader(__world_bvh_aabb_vs, &aabb_vs);
  // The engine renders reversed-Z (nearer = larger device depth), so the
  // debug pass must test GREATER against a depth buffer cleared to 0.
  reshade::api::depth_stencil_desc depth_state = {};
  depth_state.depth_enable = true;
  depth_state.depth_write_mask = true;
  depth_state.depth_func = reshade::api::compare_op::greater;
  reshade::api::pipeline_subobject raster_subobjects[3] = {
      {reshade::api::pipeline_subobject_type::vertex_shader, 1, &raster_vs},
      {reshade::api::pipeline_subobject_type::pixel_shader, 1, &raster_ps},
      {reshade::api::pipeline_subobject_type::depth_stencil_state, 1, &depth_state},
  };
  if (!device->create_pipeline(data->raster_layout, 3, raster_subobjects, &data->raster_pipeline)) {
    DestroyBvhDebugResources(device);
    return false;
  }
  reshade::api::pipeline_subobject aabb_subobjects[3] = {
      {reshade::api::pipeline_subobject_type::vertex_shader, 1, &aabb_vs},
      {reshade::api::pipeline_subobject_type::pixel_shader, 1, &raster_ps},
      {reshade::api::pipeline_subobject_type::depth_stencil_state, 1, &depth_state},
  };
  if (!device->create_pipeline(data->raster_layout, 3, aabb_subobjects, &data->aabb_pipeline)) {
    DestroyBvhDebugResources(device);
    return false;
  }
#else
  return false;
#endif

  data->debug_width = width;
  data->debug_height = height;
  data->raster_ready = data->raster_pipeline.handle != 0u && data->aabb_pipeline.handle != 0u;
  return data->raster_ready;
}

inline void RunBvhDebugPass(
    reshade::api::command_queue* queue,
    reshade::api::swapchain* swapchain) {
  if (queue == nullptr || swapchain == nullptr) return;
  const int mode = g_bvh_debug.mode.load(std::memory_order_relaxed);
  if (mode <= 0) return;
  auto* device = queue->get_device();
  if (device == nullptr) return;
  BvhDeviceData* data = GetBvhDeviceData(device);
  if (data == nullptr || !data->ready || data->active_count == 0u) return;

  const auto backbuffer = swapchain->get_current_back_buffer();
  if (backbuffer.handle == 0u) return;
  const auto backbuffer_desc = device->get_resource_desc(backbuffer);
  if (backbuffer_desc.type != reshade::api::resource_type::texture_2d) return;
  const uint32_t width = backbuffer_desc.texture.width;
  const uint32_t height = backbuffer_desc.texture.height;
  if (!EnsureBvhRasterResources(device, width, height)) return;

  auto* cmd_list = queue->get_immediate_command_list();
  if (cmd_list == nullptr) return;

  reshade::api::resource_view raster_srvs[5] = {
      data->vertex_srv, data->mesh_srv, data->instance_srv, data->grouped_srv, data->index_srv};
  reshade::api::descriptor_table_update raster_update = {
      data->raster_tables[0], 0, 0, 5, reshade::api::descriptor_type::shader_resource_view, raster_srvs};
  device->update_descriptor_tables(1, &raster_update);
  reshade::api::resource_view aabb_srvs[5] = {
      data->vertex_srv, data->mesh_srv, data->instance_srv, data->active_srv, data->index_srv};
  reshade::api::descriptor_table_update aabb_update = {
      data->raster_tables[1], 0, 0, 5, reshade::api::descriptor_type::shader_resource_view, aabb_srvs};
  device->update_descriptor_tables(1, &aabb_update);

  float view_proj[16] = {};
  {
    std::lock_guard<std::mutex> lock(g_state.mutex);
    std::memcpy(view_proj, g_state.camera.view_proj, sizeof(float) * 16u);
  }

  const bool trace_mode = mode >= 7;
  if (!trace_mode) {
    cmd_list->bind_render_targets_and_depth_stencil(1, &data->debug_rtv, data->depth_dsv);
    reshade::api::viewport viewport = {
        0.f, 0.f, static_cast<float>(width), static_cast<float>(height), 0.f, 1.f};
    cmd_list->bind_viewports(0, 1, &viewport);
    reshade::api::rect scissor = {
        0, 0, static_cast<int32_t>(width), static_cast<int32_t>(height)};
    cmd_list->bind_scissor_rects(0, 1, &scissor);
    const float clear_color[4] = {0.02f, 0.02f, 0.03f, 1.f};
    cmd_list->clear_render_target_view(data->debug_rtv, clear_color);
    const float clear_depth[1] = {0.f};  // reversed-Z far plane
    cmd_list->clear_depth_stencil_view(data->depth_dsv, clear_depth, nullptr);
  }

  const auto push_constants = [&](uint32_t mode_value, uint32_t mesh_id, uint32_t group_offset) {
    float constants[kRasterPushConstantCount] = {};
    std::memcpy(constants, view_proj, sizeof(float) * 16u);
    constants[16] = *reinterpret_cast<const float*>(&mode_value);
    constants[17] = *reinterpret_cast<const float*>(&mesh_id);
    constants[18] = *reinterpret_cast<const float*>(&group_offset);
    cmd_list->push_constants(
        reshade::api::shader_stage::all_graphics, data->raster_layout, 1, 0, kRasterPushConstantCount, constants);
  };

  if (trace_mode) {
    DispatchBvhTrace(device, cmd_list, data, static_cast<uint32_t>(mode), width, height);
    cmd_list->barrier(
        data->debug_texture, reshade::api::resource_usage::unordered_access,
        reshade::api::resource_usage::shader_resource);
    MaybeCaptureBvhTraceStats(device, queue, data);
  } else if (mode == 5) {
    cmd_list->bind_pipeline(reshade::api::pipeline_stage::all_graphics, data->aabb_pipeline);
    cmd_list->bind_descriptor_tables(
        reshade::api::shader_stage::all_graphics, data->raster_layout, 0, 1, &data->raster_tables[1]);
    push_constants(1u, 0u, 0u);
    cmd_list->draw(24u, data->active_count, 0u, 0u);
  } else {
    cmd_list->bind_pipeline(reshade::api::pipeline_stage::all_graphics, data->raster_pipeline);
    cmd_list->bind_descriptor_tables(
        reshade::api::shader_stage::all_graphics, data->raster_layout, 0, 1, &data->raster_tables[0]);
    for (const auto& group : data->draw_groups) {
      if (group.index_count == 0u || group.group_count == 0u) continue;
      push_constants(static_cast<uint32_t>(mode), group.mesh_id, group.group_offset);
      cmd_list->draw(group.index_count, group.group_count, 0u, 0u);
    }
  }

  if (!trace_mode) {
    cmd_list->barrier(
        data->debug_texture, reshade::api::resource_usage::render_target,
        reshade::api::resource_usage::shader_resource);
  }

  renodx::utils::render::RenderPass pass;
  pass.pipeline_subobjects.vertex_shader = __world_bvh_blit_vs;
  pass.pipeline_subobjects.pixel_shader = __world_bvh_blit_ps;
  pass.pipeline_subobjects.compute_shader = {};
  pass.render_target_slots.resources = {backbuffer};
  pass.shader_resource_slots.resources = {data->debug_texture};
  reshade::api::sampler_desc sampler = {};
  sampler.filter = reshade::api::filter_mode::min_mag_mip_point;
  sampler.address_u = reshade::api::texture_address_mode::clamp;
  sampler.address_v = reshade::api::texture_address_mode::clamp;
  sampler.address_w = reshade::api::texture_address_mode::clamp;
  pass.sampler_descs = {sampler};
  pass.Render(cmd_list, queue);
}

// Draw observation (pool scan, live camera for the debug views) runs while
// either needs it, independently of the research census toggle.
inline void RefreshPoolCaptureRequest() {
  const bool requested = g_pool.scan_active.load(std::memory_order_relaxed)
                         || g_bvh_debug.mode.load(std::memory_order_relaxed) != 0;
  g_state.pool_capture_requested.store(requested, std::memory_order_relaxed);
}

inline void OnWorldPresentBvh(
    reshade::api::command_queue* queue,
    reshade::api::swapchain* swapchain,
    const reshade::api::rect* source_rect,
    const reshade::api::rect* dest_rect,
    uint32_t dirty_rect_count,
    const reshade::api::rect* dirty_rects) {
  (void)source_rect;
  (void)dest_rect;
  (void)dirty_rect_count;
  (void)dirty_rects;
  if (queue == nullptr || swapchain == nullptr) return;
  auto* device = queue->get_device();
  if (device == nullptr) return;
  g_bvh_debug.device.store(device, std::memory_order_relaxed);
  RefreshPoolCaptureRequest();

  // Persist verified recipes once per Auto Research run so later launches can
  // skip the full verification pass.
  bool save_recipes = false;
  {
    std::lock_guard<std::mutex> lock(g_state.mutex);
    const bool finalized = g_state.auto_research.finalized;
    if (finalized && !g_recipes.auto_saved) {
      g_recipes.auto_saved = true;
      save_recipes = true;
    } else if (!finalized && g_recipes.auto_saved) {
      g_recipes.auto_saved = false;
    }
  }
  if (save_recipes) SaveWorldRecipes();

  BvhDeviceData* data = GetBvhDeviceData(device);
  if (data == nullptr) return;

  const bool scanning = g_pool.scan_active.load(std::memory_order_relaxed);
  uint64_t revision = 0u;
  {
    std::lock_guard<std::mutex> lock(g_pool.mutex);
    revision = g_pool.revision;
  }
  const bool force = g_bvh_debug.force_upload.exchange(false, std::memory_order_relaxed);
  if (revision != 0u && (force || (!scanning && data->uploaded_revision != revision))) {
    if (UploadWorldPoolToGpu(device, queue)) {
      BuildWorldBvh(device, queue);
    }
  } else if (data->ready && !scanning) {
    const PoolRegion region = CurrentPoolRegion();
    if (region.min[0] != data->region_min[0] || region.min[1] != data->region_min[1]
        || region.min[2] != data->region_min[2]) {
      if (UploadActiveInstancesToGpu(device, queue)) {
        data->region_min[0] = region.min[0];
        data->region_min[1] = region.min[1];
        data->region_min[2] = region.min[2];
        BuildWorldTlas(device, queue);
      }
    }
  }

  RunBvhDebugPass(queue, swapchain);
}

}  // namespace falcom_world::bvh
