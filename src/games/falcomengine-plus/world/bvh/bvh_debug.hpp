#pragma once

// Phase 1 GPU debug views (trace only).
//
// Keeps the live BVH current (bvh_live.hpp) at every present and on request
// replaces the backbuffer with a primary-ray trace of the BVH: shaded,
// instance id, or a per-pixel depth comparison against the game.
// The trace writes a debug texture through a UAV; a blit pass copies it to the
// backbuffer. The earlier raster reconstruction views were removed: the trace
// reads the same pool data and is the only view that exercises the BVH.

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <mutex>
#include <vector>

#include "../../../../utils/data.hpp"
#include "../../../../utils/render.hpp"
#include "../rtao/rtao.hpp"
#include "../rtgi/rtgi.hpp"
#include "../rtls/rtls.hpp"
#include "../rtsun/rtsun.hpp"
#include "../rtsun/rtsun_range.hpp"
#include "bvh_build.hpp"
#include "bvh_live.hpp"
#include "deform_probe.hpp"
#include "bvh_resources.hpp"
#include "bvh_trace.hpp"
#include "../rt12/rt12_probe.hpp"
#include "../rt12/rt12_capture.hpp"

namespace falcom_world::bvh {

struct BvhDebugState {
  std::atomic_int mode{0};  // BvhView
  std::atomic<reshade::api::device*> device{nullptr};
  std::atomic_int inputs{0};  // BVH inputs, set by ApplyBvhInputs: 0 idle, 1 Pool Scan or debug view, 2 forced on by RT effects
  // Mirror bound to the settings system so the GPU Debug mode survives a restart.
  float mode_setting{0.f};
};

inline BvhDebugState g_bvh_debug;

// The blit of the debug texture to the backbuffer, one per device. Kept here, not in BvhDeviceData: bvh_resources.hpp
// is compiled by the native harness, which has no utils/render.hpp.
struct __declspec(uuid("3f9c1e52-7a4d-4b8e-9c21-5d6e0a7b8c94")) BvhDebugBlit {
  renodx::utils::render::RenderPass pass;
  reshade::api::format format = reshade::api::format::unknown;  // backbuffer format the pass pipeline was built for
};

inline void DestroyBvhDebugResources(reshade::api::device* device) {
  BvhDeviceData* data = GetBvhDeviceData(device);
  if (data == nullptr) return;
  if (auto* blit = renodx::utils::data::Get<BvhDebugBlit>(device); blit != nullptr) {
    blit->pass.DestroyAll(device);
    renodx::utils::data::Delete(device, blit);
  }
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
  // Inside the region range the TLAS always holds every admitted instance.
  input.compare_range = g_pool.region_size;
  return input;
}

// Draws srv over the whole backbuffer (point sampling, stretched) with the per-device debug blit pass.
inline void BlitDebugTextureToBackbuffer(reshade::api::device* device, reshade::api::command_queue* queue,
                                         reshade::api::command_list* cmd_list, reshade::api::resource backbuffer,
                                         reshade::api::format backbuffer_format, reshade::api::resource_view srv) {
  BvhDebugBlit* blit = nullptr;
  renodx::utils::data::CreateOrGet<BvhDebugBlit>(device, blit);
  if (blit->format != backbuffer_format) {
    blit->pass.DestroyAll(device);
    blit->pass = {};
    blit->format = backbuffer_format;
  }
  auto& pass = blit->pass;
  // The swap chain rotates its buffers and RenderPass keeps the first views it generates: rebuild the RTV every present.
  pass.InvalidateRenderTargets(cmd_list);
  pass.pipeline_subobjects.vertex_shader = __world_bvh_blit_vs;
  pass.pipeline_subobjects.pixel_shader = __world_bvh_blit_ps;
  pass.pipeline_subobjects.compute_shader = {};
  pass.render_target_slots.resources = {backbuffer};
  pass.shader_resource_slots.views = {srv};
  reshade::api::sampler_desc sampler = {};
  sampler.filter = reshade::api::filter_mode::min_mag_mip_point;
  sampler.address_u = reshade::api::texture_address_mode::clamp;
  sampler.address_v = reshade::api::texture_address_mode::clamp;
  sampler.address_w = reshade::api::texture_address_mode::clamp;
  pass.sampler_descs = {sampler};
  pass.Render(cmd_list, queue);
}

inline void RunBvhDebugPass(
    reshade::api::command_queue* queue,
    reshade::api::swapchain* swapchain) {
  if (queue == nullptr || swapchain == nullptr) return;
  const auto view = static_cast<BvhView>(g_bvh_debug.mode.load(std::memory_order_relaxed));
  if (view != BvhView::TraceShaded && view != BvhView::TraceInstance && view != BvhView::DepthCompare) return;
  const PoolCpuTimer debug_timer{&g_perf_gate.block_us[kPerfBlockDebugPass]};
  auto* device = queue->get_device();
  if (device == nullptr) return;
  BvhDeviceData* data = GetBvhDeviceData(device);
  if (data == nullptr || !data->bvh_ready || data->active_count == 0u) return;

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
  const bool stats_requested = g_bvh_trace.stats_requested.exchange(false, std::memory_order_relaxed);
  // One read of the inspect click: the shader choice and the inspect constants must see the same value.
  const bool inspect_requested = g_bvh_trace.inspect_pending.exchange(false, std::memory_order_relaxed);
  const bool counters = stats_requested || inspect_requested
                        || g_bvh_trace.counters_every_frame.load(std::memory_order_relaxed);
  DispatchBvhTrace(device, cmd_list, data, TraceShaderMode(view), width, height, depth, counters, inspect_requested);
  cmd_list->barrier(
      data->debug_texture, reshade::api::resource_usage::unordered_access,
      reshade::api::resource_usage::shader_resource);
  if (stats_requested) {
    ReadbackBvhTraceStats(device, queue, data);
  }
  if (data->inspect.fresh) {
    data->inspect.fresh = false;
    data->inspect.candidates.clear();
    data->inspect.crossing_instances = 0u;
    data->inspect.ray_valid = false;
    if (data->inspect.valid && data->inspect.hit && (data->inspect.instance & kDynamicInstanceFlag) != 0u) {
      DescribeDynamicInspect(data->inspect, LiveCompareClassName(data->inspect.compare_class));
    } else {
      DescribeLiveInspect(data);
    }
    // Kept for world_pool.json ("inspect"). The ids are known for static instances, the mesh uid for P2b records
    // and the identity key for deform objects; resolved here, before g_pool.mutex.
    const bool hit = data->inspect.valid && data->inspect.hit;
    const uint32_t hit_instance = data->inspect.instance;
    const bool known = hit && (hit_instance & kDynamicFlagsMask) == 0u && hit_instance < data->tlas_info.size();
    std::string hit_kind;
    uint64_t p2b_mesh_uid = 0u;
    std::string deform_key;
    if (!hit) {
      // nothing hit
    } else if ((hit_instance & kDynamicFlagsMask) == 0u) {
      hit_kind = "static_instance";
    } else if ((hit_instance & kDynamicFlagsMask) == kDynamicDrawFlag) {
      hit_kind = "p2b_record";
      const uint32_t record = hit_instance & ~kDynamicFlagsMask;
      for (const DynamicDrawGPU& draw : data->dynamic.last_table) {
        if (record >= draw.info[2] && record < draw.info[2] + draw.info[1]) {
          const uint32_t slot = draw.extra[2];
          if (slot < data->slots.size()) p2b_mesh_uid = data->slots[slot].uid;
          break;
        }
      }
    } else if ((hit_instance & kDynamicInstanceFlag) != 0u) {
      hit_kind = "deform_object";
      uint64_t key = 0u;
      if (DynamicObjectKey(hit_instance, &key)) deform_key = renodx::utils::log::AsHex(key);
    }
    std::lock_guard<std::mutex> lock(g_pool.mutex);
    g_pool.inspect = PoolInspectRecord{.set = true, .frame = g_state.frame.load(), .hit = hit,
                                       .instance_id = known ? data->tlas_info[hit_instance].id : 0u,
                                       .mesh_uid = known ? data->tlas_info[hit_instance].uid : p2b_mesh_uid,
                                       .hit_kind = hit_kind,
                                       .deform_key = deform_key,
                                       .lines = data->inspect.lines,
                                       .ray_valid = data->inspect.ray_valid,
                                       .ray_origin = data->inspect.ray_origin,
                                       .ray_direction = data->inspect.ray_direction,
                                       .crossing_instances = data->inspect.crossing_instances,
                                       .candidates = data->inspect.candidates};
  }

  BlitDebugTextureToBackbuffer(device, queue, cmd_list, backbuffer, backbuffer_desc.texture.format, data->debug_srv);
}

// Draw observation (pool scan, live camera for the debug views) runs while
// either needs it, independently of the research census toggle.
inline void RefreshPoolCaptureRequest() {
  const bool requested = g_pool.scan_active.load(std::memory_order_relaxed)
                         || g_bvh_debug.mode.load(std::memory_order_relaxed) != 0
                         || g_deform.enabled.load(std::memory_order_relaxed)
                         || g_deform_live.enabled.load(std::memory_order_relaxed);
  g_state.pool_capture_requested.store(requested, std::memory_order_relaxed);
  if (!CaptureActive()) {
    g_state.tracking_epoch.fetch_add(1u, std::memory_order_relaxed);
  }
}

inline bool RtBvhDemanded() {
  return rtao::RtaoRequested() || rtgi::g_rtgi_enabled > 0.5f || rtls::RtlsRequested() || rtsun::RtSunRequested();
}

inline void ApplyBvhInputs() {
  const bool demand = RtBvhDemanded();
  const bool scan_setting = g_pool.scan_active_setting > 0.5f;
  const bool diagnostic = scan_setting || g_bvh_debug.mode.load(std::memory_order_relaxed) != 0;
  g_bvh_debug.inputs.store(demand ? 2 : (diagnostic ? 1 : 0), std::memory_order_relaxed);
  const bool deform_setting = g_deform_live.enabled_setting > 0.5f;
  auto store = [](std::atomic_bool& flag, bool value) {
    if (flag.load(std::memory_order_relaxed) != value) {
      flag.store(value, std::memory_order_relaxed);
    }
  };
  store(g_pool.scan_active, scan_setting || demand);
  store(g_live_bvh.enabled, demand || (diagnostic && g_live_bvh.enabled_setting > 0.5f));
  store(g_deform.enabled, demand || (diagnostic && deform_setting));
  store(g_deform_live.enabled, demand || (diagnostic && deform_setting));
  // RT sun count latched for the draw hooks (CSM skip) and the trace: nonzero only while RT sun is requested and produced
  // last frame, with a built BVH range (0 = not ready, never unlimited).
  const bool rtsun_produced = rtsun::RtSunRequested() && rtsun::g_rtsun_frame.producing && rtsun::g_rtsun_frame.built_range > 0.f;
  g_state.rt_sun_cascades.store(
      rtsun_produced ? rtsun::RtSunEffectiveCascades(static_cast<int>(rtsun::g_rtsun_cascade_range), g_state.camera, rtsun::g_rtsun_frame.built_range) : 0,
      std::memory_order_relaxed);
  RefreshPoolCaptureRequest();
}

inline void OnWorldPresentBvh(
    reshade::api::command_queue* queue,
    reshade::api::swapchain* swapchain,
    const reshade::api::rect* source_rect,
    const reshade::api::rect* dest_rect,
    uint32_t dirty_rect_count,
    const reshade::api::rect* dirty_rects) {
  const PoolCpuTimer present_timer{&g_perf_gate.present_bvh_us};
  (void)source_rect;
  (void)dest_rect;
  (void)dirty_rect_count;
  (void)dirty_rects;
  if (queue == nullptr || swapchain == nullptr) return;
  auto* device = queue->get_device();
  if (device == nullptr) return;
  if (rt12::g_rt12_probe.requested.exchange(false, std::memory_order_relaxed)) rt12::RunRt12Probe(device, g_state.frame.load());
  if (g_perf_gate.clear_pending.exchange(false)) {
    ClearPerfGathered(GetBvhDeviceData(device));
    rtao::ClearRtaoPerfGathered(device);
    rtgi::ClearRtgiPerfGathered(device);
    rtls::RtlsDeviceData& rtls_data = rtls::GetRtlsDeviceData(device);
    ClearGpuTimerStats(&rtls_data.timer);
    ClearGpuTimerStats(&rtls_data.denoise.timer);
    if (rtsun::RtSunDeviceData* rtsun_data = rtsun::FindRtSunDeviceData(device)) {
      ClearGpuTimerStats(&rtsun_data->timer);
      ClearGpuTimerStats(&rtsun_data->denoise.timer);
      rtsun::g_rtsun_frame.gpu_ms = -1.f;
      rtsun::g_rtsun_frame.denoise_gpu_ms = -1.f;
    }
    rtls::g_rtls_frame.gpu_ms = -1.f;
    rtls::g_rtls_frame.denoise_gpu_ms = -1.f;
  }
  g_bvh_debug.device.store(device, std::memory_order_relaxed);
  ApplyBvhInputs();

  PerfExtraGpu extra;
  if (g_perf_gate.gather.load(std::memory_order_relaxed)) {
    {
      const rtao::RtaoDeviceData& rtao_data = rtao::GetRtaoDeviceData(device);
      extra.rtao = {rtao_data.timer.ms_sum, rtao_data.timer.samples};
      extra.rtao_filter = {rtao_data.filter_timer.ms_sum, rtao_data.filter_timer.samples};
    }
    {
      const rtgi::RtgiDeviceData& rtgi_data = rtgi::GetRtgiDeviceData(device);
      extra.rtgi = {rtgi_data.timer.ms_sum, rtgi_data.timer.samples};
      extra.rtgi_denoise = {rtgi_data.denoise.timer.ms_sum, rtgi_data.denoise.timer.samples};
    }
    {
      const rtls::RtlsDeviceData& rtls_data = rtls::GetRtlsDeviceData(device);
      extra.rtls = {rtls_data.timer.ms_sum, rtls_data.timer.samples};
      extra.rtls_denoise = {rtls_data.denoise.timer.ms_sum, rtls_data.denoise.timer.samples};
    }
    if (const rtsun::RtSunDeviceData* rtsun_data = rtsun::FindRtSunDeviceData(device)) {
      extra.rtsun = {rtsun_data->timer.ms_sum, rtsun_data->timer.samples};
      extra.rtsun_denoise = {rtsun_data->denoise.timer.ms_sum, rtsun_data->denoise.timer.samples};
    }
    {
      std::lock_guard<std::mutex> lock(g_deform_live.mutex);
      extra.deform_refit = {g_deform_live.refit_timer.ms_sum, g_deform_live.refit_timer.samples};
    }
  }
  const auto present_time = SamplePerfPresent(device, extra);
  UpdateLiveBvh(device, queue);
  rt12::OnGcPresent(device, queue);
  rt12::OnPresent(device);
  UpdateCacheDiag(device, present_time);
  UpdateDeformLive(device, queue);
  rtao::MaybeCaptureRtaoStats(device, queue);
  rtgi::MaybeCaptureRtgiStats(device, queue);
  rtls::MaybeCaptureRtlsStats(device, queue);
  rtsun::MaybeCaptureRtSunStats(device, queue);
  RunBvhDebugPass(queue, swapchain);
  // Draws a debug texture over the whole backbuffer (RTLS and RTGI debug views).
  const auto blit_debug = [&](reshade::api::resource_view srv) {
    const auto backbuffer = swapchain->get_current_back_buffer();
    if (backbuffer.handle == 0u) return;
    const auto backbuffer_desc = device->get_resource_desc(backbuffer);
    if (backbuffer_desc.type != reshade::api::resource_type::texture_2d) return;
    auto* cmd_list = queue->get_immediate_command_list();
    if (cmd_list == nullptr) return;
    BlitDebugTextureToBackbuffer(device, queue, cmd_list, backbuffer, backbuffer_desc.texture.format, srv);
  };
  // RTLS Debug 1 to 5, when this frame's trace wrote the debug texture and no BVH GPU debug mode is selected.
  // RTGI Debug, below, draws over it when both are set.
  if (rtls::g_rtls_debug > 0.5f && rtls::g_rtls_frame.producing && rtls::g_rtls_frame.debug_written
      && g_bvh_debug.mode.load(std::memory_order_relaxed) == 0) {
    const auto found_rtls = rtls::g_rtls_devices.find(device);
    if (found_rtls != rtls::g_rtls_devices.end() && found_rtls->second.debug.srv[0].handle != 0u) {
      blit_debug(found_rtls->second.debug.srv[0]);
    }
  }
  // RT Sun Debug 1 to 2: the sun trace debug texture, when this frame's trace wrote it and no BVH GPU debug mode is selected.
  if (rtsun::g_rtsun_debug > 0.5f && rtsun::g_rtsun_frame.producing && rtsun::g_rtsun_frame.debug_written
      && g_bvh_debug.mode.load(std::memory_order_relaxed) == 0) {
    const auto found_rtsun = rtsun::g_rtsun_devices.find(device);
    if (found_rtsun != rtsun::g_rtsun_devices.end() && found_rtsun->second.debug.srv[0].handle != 0u) {
      blit_debug(found_rtsun->second.debug.srv[0]);
    }
  }
  // RTGI Debug 1 to 10: the RTGI debug texture over the whole backbuffer, when the pass that owns the view wrote it this
  // frame. Skipped while a BVH GPU debug mode is selected (that view owns the backbuffer). Last in this handler: the
  // returns below end only the RTGI blit.
  if (rtgi::g_rtgi_debug < 0.5f || rtgi::g_rtgi_debug > 10.5f) return;
  if (!rtgi::g_rtgi_frame.producing || !rtgi::g_rtgi_frame.debug_written) return;
  if (g_bvh_debug.mode.load(std::memory_order_relaxed) != 0) return;
  const auto found = rtgi::g_rtgi_devices.find(device);
  if (found == rtgi::g_rtgi_devices.end() || found->second.debug.srv[0].handle == 0u) return;
  blit_debug(found->second.debug.srv[0]);
}

}  // namespace falcom_world::bvh
