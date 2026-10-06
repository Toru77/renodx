#pragma once

// Phase 1 GPU debug views (trace only).
//
// Uploads the CPU pool to GPU buffers (once per pool revision), rebuilds the
// BVH, and on request replaces the backbuffer with a primary-ray trace of the
// BVH: shaded, instance id, or a per-pixel depth comparison against the game.
// The trace writes a debug texture through a UAV; a blit pass copies it to the
// backbuffer. The earlier raster reconstruction views were removed: the trace
// reads the same pool data and is the only view that exercises the BVH.

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <vector>

#include "../../../../utils/render.hpp"
#include "bvh_build.hpp"
#include "bvh_resources.hpp"
#include "bvh_trace.hpp"

namespace falcom_world::bvh {

struct BvhDebugState {
  std::atomic_int mode{0};  // BvhView
  std::atomic_bool force_upload{false};
  std::atomic<reshade::api::device*> device{nullptr};
};

inline BvhDebugState g_bvh_debug;

inline void DestroyBvhDebugResources(reshade::api::device* device) {
  BvhDeviceData* data = GetBvhDeviceData(device);
  if (data == nullptr) return;
  if (data->debug_uav.handle != 0u) {
    device->destroy_resource_view(data->debug_uav);
    data->debug_uav = {0u};
  }
  DestroyBuffer(device, &data->debug_srv, &data->debug_texture);
  data->debug_width = 0u;
  data->debug_height = 0u;
}

// Backbuffer-sized debug texture the trace writes (UAV) and the blit reads (SRV).
inline bool EnsureBvhDebugTarget(reshade::api::device* device, uint32_t width, uint32_t height) {
  BvhDeviceData* data = GetBvhDeviceData(device);
  if (data == nullptr || width == 0u || height == 0u) return false;
  if (data->debug_texture.handle != 0u && data->debug_width == width && data->debug_height == height) return true;
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
  color_desc.usage = reshade::api::resource_usage::shader_resource
                     | reshade::api::resource_usage::unordered_access;
  if (!device->create_resource(color_desc, nullptr, reshade::api::resource_usage::unordered_access, &data->debug_texture)) {
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
  data->debug_width = width;
  data->debug_height = height;
  return true;
}

// Game depth for Depth Compare. Only this frame's lighting depth is used: the
// view of an earlier frame may be stale or already released.
inline BvhTraceDepthInput GetBvhTraceDepthInput(BvhView view) {
  BvhTraceDepthInput input;
  if (view != BvhView::DepthCompare) return input;
  std::lock_guard<std::mutex> lock(g_state.mutex);
  const DepthSource& depth = g_state.depth_source;
  if (!depth.valid || depth.view.handle == 0u || depth.frame != g_state.frame.load()) return input;
  if (depth.viewport_width > 0.f && depth.viewport_height > 0.f) {
    input.rect[0] = depth.viewport_x;
    input.rect[1] = depth.viewport_y;
    input.rect[2] = depth.viewport_width;
    input.rect[3] = depth.viewport_height;
  } else {
    input.rect[2] = static_cast<float>(depth.width);
    input.rect[3] = static_cast<float>(depth.height);
  }
  if (input.rect[2] <= 0.f || input.rect[3] <= 0.f) return input;
  input.view = depth.view;
  // Inside a quarter of the region size the TLAS always holds every admitted
  // instance (the region keeps the camera that far from its border).
  input.compare_range = g_pool.region_size * 0.25f;
  return input;
}

inline void RunBvhDebugPass(
    reshade::api::command_queue* queue,
    reshade::api::swapchain* swapchain) {
  if (queue == nullptr || swapchain == nullptr) return;
  const auto view = static_cast<BvhView>(g_bvh_debug.mode.load(std::memory_order_relaxed));
  if (view != BvhView::TraceShaded && view != BvhView::TraceInstance && view != BvhView::DepthCompare) return;
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
  if (!EnsureBvhDebugTarget(device, width, height)) return;

  auto* cmd_list = queue->get_immediate_command_list();
  if (cmd_list == nullptr) return;

  const BvhTraceDepthInput depth = GetBvhTraceDepthInput(view);
  if (depth.view.handle != 0u) {
    // The game may still have its depth bound for output; D3D11 would then
    // drop the compute SRV. Nothing of the game's frame draws after present.
    cmd_list->bind_render_targets_and_depth_stencil(0, nullptr, {0u});
  }
  DispatchBvhTrace(device, cmd_list, data, TraceShaderMode(view), width, height, depth);
  cmd_list->barrier(
      data->debug_texture, reshade::api::resource_usage::unordered_access,
      reshade::api::resource_usage::shader_resource);
  MaybeCaptureBvhTraceStats(device, queue, data);

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
