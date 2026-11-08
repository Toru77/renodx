#pragma once

// DLSS Ray Reconstruction guide buffers (world/docs/ROADMAP.md, Reference: DLSS Ray Reconstruction): the compute pass at the
// lighting hook that writes NormalRoughness, Albedo and SpecularAlbedo at depth size. M1 only prepares the guides;
// the lighting output is not changed and no NGX call is made here.

#include <d3d11.h>
#include <wrl/client.h>

#include <atomic>
#include <cstdint>
#include <memory>
#include <shared_mutex>
#include <unordered_map>
#include <utility>

#include "../rtao/rtao.hpp"
#include "../rtao/rtao_resources.hpp"
#include "../rtgi/rtgi_resources.hpp"
#include "../world_state.hpp"
#include "rr_state.hpp"

namespace falcom_world::rr {

#if defined(__world_rr_guides_EMBED_FILE)
inline constexpr std::span<const std::uint8_t> kRrGuidesShader = __world_rr_guides;
#else
inline constexpr std::span<const std::uint8_t> kRrGuidesShader = {};
#endif

inline constexpr uint32_t kRrGuidesSrvCount = 5u;   // t0 color, t1 mrt0, t2 mrt2, t3 deferred, t4 depth
inline constexpr uint32_t kRrGuidesUavCount = 4u;   // u0 NormalRoughness, u1 Albedo, u2 SpecularAlbedo, u3 Depth copy
inline constexpr uint32_t kRrGuidesPushCount = 4u;  // width, height (floats), 2 unused

inline constexpr rtgi::RtgiPipelineDesc kRrGuidesPipelineDesc = {.srv_count = kRrGuidesSrvCount,
                                                                 .uav_count = kRrGuidesUavCount,
                                                                 .sampler_count = 0u,
                                                                 .push_count = kRrGuidesPushCount,
                                                                 .code = kRrGuidesShader,
                                                                 .name = "rr guides"};

// The lighting inputs of one draw, as views (the caller checks them). Size is the depth size.
struct GuideInputs {
  reshade::api::resource_view color_view = {0u};
  reshade::api::resource_view mrt0_view = {0u};
  reshade::api::resource_view mrt2_view = {0u};
  reshade::api::resource_view deferred_view = {0u};
  reshade::api::resource_view depth_view = {0u};
  reshade::api::resource_view scene_cbv_view = {0u};
  uint32_t width = 0u;
  uint32_t height = 0u;
};

struct RrGuideResources {
  Microsoft::WRL::ComPtr<ID3D11Texture2D> normal_roughness;
  Microsoft::WRL::ComPtr<ID3D11Texture2D> albedo;
  Microsoft::WRL::ComPtr<ID3D11Texture2D> specular;
  Microsoft::WRL::ComPtr<ID3D11Texture2D> depth;
  uint32_t width = 0u;
  uint32_t height = 0u;
};

struct RrDeviceData {
  rtgi::RtgiPipeline pipeline;
  rtgi::RtgiTextureSet<1> normal_roughness;  // r16g16b16a16_float: view-space normal xyz, roughness w
  rtgi::RtgiTextureSet<1> albedo;            // r8g8b8a8_unorm
  rtgi::RtgiTextureSet<1> specular;          // r8g8b8a8_unorm: SpecularAlbedo
  rtgi::RtgiTextureSet<1> depth;             // r32_float: raw hardware depth copy (RR's Depth input)
  std::shared_ptr<const RrGuideResources> published_resources;
};

// One entry per device (destroyed with the device).
inline std::unordered_map<reshade::api::device*, RrDeviceData> g_rr_devices;

// The guides of the last produced frame, read by the RR consumer (frame == UINT32_MAX: none).
struct PublishedGuides {
  std::shared_mutex mutex;
  uint32_t frame = UINT32_MAX;
  std::shared_ptr<const RrGuideResources> resources;
};
inline PublishedGuides g_rr_guides;

// The frame whose first lighting draw attempted the guides (UINT32_MAX: none); the caller gates on it, once per frame.
inline std::atomic<uint32_t> g_rr_guides_attempt_frame{UINT32_MAX};

// Builds the guides for this frame from the lighting inputs. The caller runs it once per frame (g_rr_guides_attempt_frame).
inline void DispatchGuides(reshade::api::device* device, reshade::api::command_list* cmd_list, const GuideInputs& in) {
  RrDeviceData& rd = g_rr_devices[device];
  if (!rtgi::EnsureRtgiPipeline(device, &rd.pipeline, kRrGuidesPipelineDesc)) {
    SetGuideReason(GuideReason::PipelineFailed);
    return;
  }
  const uint32_t w = in.width;
  const uint32_t h = in.height;
  if (!rtgi::EnsureRtgiTextureSet(device, &rd.normal_roughness, reshade::api::format::r16g16b16a16_float, 8u, w, h) || !rtgi::EnsureRtgiTextureSet(device, &rd.albedo, reshade::api::format::r8g8b8a8_unorm, 4u, w, h) || !rtgi::EnsureRtgiTextureSet(device, &rd.specular, reshade::api::format::r8g8b8a8_unorm, 4u, w, h) || !rtgi::EnsureRtgiTextureSet(device, &rd.depth, reshade::api::format::r32_float, 4u, w, h)) {
    SetGuideReason(GuideReason::TextureFailed);
    return;
  }

  const reshade::api::descriptor_type kCbv = reshade::api::descriptor_type::constant_buffer;
  const reshade::api::descriptor_type kSrv = reshade::api::descriptor_type::shader_resource_view;
  const reshade::api::descriptor_type kUav = reshade::api::descriptor_type::unordered_access_view;
  const reshade::api::buffer_range scene_cbv = rtao::SceneCbvRange(in.scene_cbv_view);
  const reshade::api::resource_view srvs[kRrGuidesSrvCount] = {in.color_view, in.mrt0_view, in.mrt2_view, in.deferred_view,
                                                               in.depth_view};
  const reshade::api::resource_view uavs[kRrGuidesUavCount] = {
      rd.normal_roughness.uav[0], rd.albedo.uav[0], rd.specular.uav[0], rd.depth.uav[0]};
  reshade::api::descriptor_table_update updates[3] = {
      {rd.pipeline.cbv_table, 0, 0, 1, kCbv, &scene_cbv},
      {rd.pipeline.srv_table, 0, 0, kRrGuidesSrvCount, kSrv, srvs},
      {rd.pipeline.uav_table, 0, 0, kRrGuidesUavCount, kUav, uavs},
  };
  device->update_descriptor_tables(3, updates);
  cmd_list->bind_pipeline(reshade::api::pipeline_stage::all_compute, rd.pipeline.pipeline);
  const reshade::api::descriptor_table tables[3] = {rd.pipeline.cbv_table, rd.pipeline.srv_table, rd.pipeline.uav_table};
  cmd_list->bind_descriptor_tables(reshade::api::shader_stage::all_compute, rd.pipeline.layout, 0, 3, tables);
  const float push[kRrGuidesPushCount] = {static_cast<float>(w), static_cast<float>(h), 0.f, 0.f};
  cmd_list->push_constants(reshade::api::shader_stage::all_compute, rd.pipeline.layout, 3, 0, kRrGuidesPushCount, push);
  cmd_list->dispatch((w + 7u) / 8u, (h + 7u) / 8u, 1u);
  rtao::NullComputeSlots(cmd_list, kRrGuidesSrvCount, kRrGuidesUavCount, 0u);
  cmd_list->barrier(rd.normal_roughness.texture[0], reshade::api::resource_usage::unordered_access,
                    reshade::api::resource_usage::shader_resource);
  cmd_list->barrier(rd.albedo.texture[0], reshade::api::resource_usage::unordered_access,
                    reshade::api::resource_usage::shader_resource);
  cmd_list->barrier(rd.specular.texture[0], reshade::api::resource_usage::unordered_access,
                    reshade::api::resource_usage::shader_resource);
  cmd_list->barrier(rd.depth.texture[0], reshade::api::resource_usage::unordered_access,
                    reshade::api::resource_usage::shader_resource);

  const auto same_textures = [&](const std::shared_ptr<const RrGuideResources>& resources) {
    return resources != nullptr && resources->width == w && resources->height == h && resources->normal_roughness.Get() == reinterpret_cast<ID3D11Texture2D*>(rd.normal_roughness.texture[0].handle) && resources->albedo.Get() == reinterpret_cast<ID3D11Texture2D*>(rd.albedo.texture[0].handle) && resources->specular.Get() == reinterpret_cast<ID3D11Texture2D*>(rd.specular.texture[0].handle) && resources->depth.Get() == reinterpret_cast<ID3D11Texture2D*>(rd.depth.texture[0].handle);
  };
  if (!same_textures(rd.published_resources)) {
    auto resources = std::make_shared<RrGuideResources>();
    const auto capture = [](reshade::api::resource resource, Microsoft::WRL::ComPtr<ID3D11Texture2D>* texture) {
      if (resource.handle == 0u) return false;
      auto* native_resource = reinterpret_cast<ID3D11Resource*>(resource.handle);  // NOLINT(performance-no-int-to-ptr)
      return SUCCEEDED(native_resource->QueryInterface(IID_PPV_ARGS(texture->GetAddressOf())));
    };
    if (!capture(rd.normal_roughness.texture[0], &resources->normal_roughness) || !capture(rd.albedo.texture[0], &resources->albedo) || !capture(rd.specular.texture[0], &resources->specular) || !capture(rd.depth.texture[0], &resources->depth)) {
      SetGuideReason(GuideReason::TextureFailed);
      return;
    }
    resources->width = w;
    resources->height = h;
    rd.published_resources = std::move(resources);
  }
  std::shared_ptr<const RrGuideResources> retired_resources;
  {
    const std::unique_lock lock(g_rr_guides.mutex);
    retired_resources = std::exchange(g_rr_guides.resources, rd.published_resources);
    g_rr_guides.frame = g_state.frame.load(std::memory_order_relaxed);
  }
  SetGuideReason(GuideReason::Ready);
}

// Frees the guides and the pipeline of one device (called from OnDestroyDeviceBvh).
inline void DestroyRrDeviceData(reshade::api::device* device) {
  const auto found = g_rr_devices.find(device);
  if (found == g_rr_devices.end()) return;
  std::shared_ptr<const RrGuideResources> retired_resources;
  {
    std::unique_lock lock(g_rr_guides.mutex);
    g_rr_guides.frame = UINT32_MAX;
    retired_resources = std::move(g_rr_guides.resources);
  }
  RrDeviceData& rd = found->second;
  rtgi::DestroyRtgiTextureSet(device, &rd.normal_roughness);
  rtgi::DestroyRtgiTextureSet(device, &rd.albedo);
  rtgi::DestroyRtgiTextureSet(device, &rd.specular);
  rtgi::DestroyRtgiTextureSet(device, &rd.depth);
  rtgi::DestroyRtgiPipeline(device, rd.pipeline);
  g_rr_devices.erase(found);
}

}  // namespace falcom_world::rr
