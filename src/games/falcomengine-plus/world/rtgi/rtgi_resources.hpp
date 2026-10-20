#pragma once

// RTGI GPU resources: the trace pipeline (world_rtgi.cs_5_0.hlsl), the stats buffer and the debug texture (r8g8b8a8_unorm
// at the depth size, created while RTGI Debug is 1 to 6). Created lazily from the lighting hook (rtgi.hpp); no module mutex
// is held around any graphics call here.

#include <cstddef>
#include <cstdint>
#include <span>
#include <unordered_map>

#include "../../../../utils/log.hpp"
#include "../rtao/rtao_resources.hpp"

namespace falcom_world::rtgi {

// t0..t8 BVH, t9 game depth, t10..t13 deforming (null: not traced), t14..t16 alpha, t17 MRT normal, t18 IS-FAST noise,
// t19 P2b records (the RTAO pass A table), t20 radiance: the previous frame's lighting output.
inline constexpr uint32_t kRtgiSrvCount = 21u;
inline constexpr uint32_t kRtgiUavCount = 3u;  // u0 debug texture (null unless RTGI Debug is 1 to 6), u1 stats, u2 raw GI
inline constexpr uint32_t kRtgiPushConstantCount = 24u;
inline constexpr uint32_t kRtgiStatsCount = 24u;

// Matching RTGI_STAT_* in world_rtgi.cs_5_0.hlsl.
inline constexpr uint32_t kRtgiStatRays = 0u;
inline constexpr uint32_t kRtgiStatHits = 1u;
inline constexpr uint32_t kRtgiStatPixels = 2u;
inline constexpr uint32_t kRtgiStatSky = 3u;
inline constexpr uint32_t kRtgiStatNormal = 4u;
inline constexpr uint32_t kRtgiStatRegion = 5u;
inline constexpr uint32_t kRtgiStatBeyondFade = 6u;
inline constexpr uint32_t kRtgiStatInvalidRefs = 7u;
inline constexpr uint32_t kRtgiStatStackOverflow = 8u;
inline constexpr uint32_t kRtgiStatScaled = 9u;
inline constexpr uint32_t kRtgiStatHitOnScreen = 10u;
inline constexpr uint32_t kRtgiStatHitOccluded = 11u;
inline constexpr uint32_t kRtgiStatHitOffScreen = 12u;
inline constexpr uint32_t kRtgiStatHitDistanceDm = 13u;
inline constexpr uint32_t kRtgiStatRadianceLookups = 14u;
inline constexpr uint32_t kRtgiStatSunRays = 15u;
inline constexpr uint32_t kRtgiStatSunVisible = 16u;
inline constexpr uint32_t kRtgiStatLuminanceSum = 17u;
inline constexpr uint32_t kRtgiStatLuminanceMax = 18u;
inline constexpr uint32_t kRtgiStatClamped = 19u;
inline constexpr uint32_t kRtgiFlagRadianceValid = 1u;  // b12 c2.z bit (RTGI_FLAG_* in the shader)

#if defined(__world_rtgi_EMBED_FILE)
inline constexpr std::span<const std::uint8_t> kRtgiTraceShader = __world_rtgi;
#else
inline constexpr std::span<const std::uint8_t> kRtgiTraceShader = {};
#endif

// b12 for the RTGI trace, matching cbuffer cb_rtgi in world_rtgi.cs_5_0.hlsl (six float4 registers).
struct RtgiPushConstants {
  float params[4];    // c0: effective ray max, normal bias, spp, trace divisor
  float sampling[4];  // c1: isfast flag (1 loaded), slice base, seed, debug mode
  float fade[4];      // c2: fade start_eff, fade end_eff, flags (kRtgiFlag* as a float), P2b record count (w, uint bits)
  float region[4];    // c3: region min xyz, region size
  float size[4];      // c4: trace width, trace height, full width, full height
  float shading[4];   // c5: ambient contribution, radiance clamp (0 = off), unused, unused
};
static_assert(sizeof(RtgiPushConstants) == kRtgiPushConstantCount * sizeof(float), "b12 must be 24 floats");
static_assert(offsetof(RtgiPushConstants, sampling) == 4 * sizeof(float), "b12 c1 offset");
static_assert(offsetof(RtgiPushConstants, fade) == 8 * sizeof(float), "b12 c2 offset");
static_assert(offsetof(RtgiPushConstants, region) == 12 * sizeof(float), "b12 c3 offset");
static_assert(offsetof(RtgiPushConstants, size) == 16 * sizeof(float), "b12 c4 offset");
static_assert(offsetof(RtgiPushConstants, shading) == 20 * sizeof(float), "b12 c5 offset");

// Count textures of one format and size, each with an SRV and a UAV, zero-initialised (rtao::CreateRtaoTexture).
template <size_t Count>
struct RtgiTextureSet {
  reshade::api::resource texture[Count] = {};
  reshade::api::resource_view srv[Count] = {};
  reshade::api::resource_view uav[Count] = {};
  uint32_t width = 0u;
  uint32_t height = 0u;
};

template <size_t Count>
inline void DestroyRtgiTextureSet(reshade::api::device* device, RtgiTextureSet<Count>* set) {
  for (size_t i = 0; i < Count; ++i) {
    bvh::DestroyBuffer(device, &set->srv[i], &set->texture[i]);
    bvh::DestroyBuffer(device, &set->uav[i], nullptr);
  }
  set->width = 0u;
  set->height = 0u;
}

// Creates the set at width x height; a size change replaces it. On a failure the set is destroyed and false returned.
template <size_t Count>
inline bool EnsureRtgiTextureSet(reshade::api::device* device, RtgiTextureSet<Count>* set, reshade::api::format format,
                                 uint32_t bytes_per_pixel, uint32_t width, uint32_t height) {
  if (set->texture[0].handle != 0u && set->width == width && set->height == height) return true;
  DestroyRtgiTextureSet(device, set);
  for (size_t i = 0; i < Count; ++i) {
    if (!rtao::CreateRtaoTexture(device, format, width, height, bytes_per_pixel, &set->texture[i], &set->srv[i], &set->uav[i])) {
      DestroyRtgiTextureSet(device, set);
      return false;
    }
  }
  set->width = width;
  set->height = height;
  return true;
}

struct RtgiDeviceData {
  reshade::api::pipeline_layout layout = {0u};
  reshade::api::descriptor_table cbv_table = {0u};
  reshade::api::descriptor_table srv_table = {0u};
  reshade::api::descriptor_table uav_table = {0u};
  reshade::api::pipeline pipeline = {0u};
  bool pipeline_failed = false;
  reshade::api::resource stats_buffer = {0u};
  reshade::api::resource_view stats_uav = {0u};
  RtgiTextureSet<1> debug;  // r8g8b8a8_unorm, depth size; created while RTGI Debug is 1 to 6
  RtgiTextureSet<1> raw;    // rgba16f, trace size: rgb GI, a = 1 traced, -1 not traced
  reshade::api::resource radiance_texture = {0u};
  reshade::api::resource_view radiance_srv = {0u};
  uint32_t radiance_width = 0u;
  uint32_t radiance_height = 0u;
  reshade::api::format radiance_format = reshade::api::format::unknown;
  uint64_t radiance_frame = UINT64_MAX;
  bool radiance_failed = false;
  bvh::GpuTimer timer = {};
};

// One entry per device (destroyed with the device).
inline std::unordered_map<reshade::api::device*, RtgiDeviceData> g_rtgi_devices;

inline RtgiDeviceData& GetRtgiDeviceData(reshade::api::device* device) { return g_rtgi_devices[device]; }

// Copies the lighting output for the next frame's RTGI trace. The target follows the source's size and format; a
// creation failure is logged once and disables the copy for the device (radiance_failed).
inline bool EnsureRtgiRadianceTarget(reshade::api::device* device, RtgiDeviceData* data, const reshade::api::resource_desc& source) {
  if (data->radiance_failed) return false;
  const reshade::api::format format = source.texture.format;
  if (data->radiance_texture.handle != 0u && data->radiance_width == source.texture.width
      && data->radiance_height == source.texture.height && data->radiance_format == format) {
    return true;
  }
  bvh::DestroyBuffer(device, &data->radiance_srv, &data->radiance_texture);
  reshade::api::resource_desc desc = {};
  desc.type = reshade::api::resource_type::texture_2d;
  desc.texture = {source.texture.width, source.texture.height, 1, 1, format, 1};
  desc.heap = reshade::api::memory_heap::gpu_only;
  desc.usage = reshade::api::resource_usage::shader_resource | reshade::api::resource_usage::copy_dest;
  const reshade::api::resource_view_desc view(reshade::api::resource_view_type::texture_2d,
                                              reshade::api::format_to_default_typed(format, 0), 0, 1, 0, 1);
  if (!device->create_resource(desc, nullptr, reshade::api::resource_usage::shader_resource, &data->radiance_texture)
      || !device->create_resource_view(data->radiance_texture, reshade::api::resource_usage::shader_resource, view, &data->radiance_srv)) {
    bvh::DestroyBuffer(device, &data->radiance_srv, &data->radiance_texture);
    data->radiance_failed = true;
    renodx::utils::log::w("[world-rtgi] radiance copy target creation failed (format ", static_cast<uint32_t>(format), ")");
    return false;
  }
  data->radiance_width = source.texture.width;
  data->radiance_height = source.texture.height;
  data->radiance_format = format;
  renodx::utils::log::i("[world-rtgi] radiance copy: ", data->radiance_width, "x", data->radiance_height, " format ",
                        static_cast<uint32_t>(format));
  return true;
}

inline void DestroyRtgiDeviceData(reshade::api::device* device) {
  const auto found = g_rtgi_devices.find(device);
  if (found == g_rtgi_devices.end()) return;
  RtgiDeviceData& data = found->second;
  DestroyRtgiTextureSet(device, &data.debug);
  DestroyRtgiTextureSet(device, &data.raw);
  bvh::DestroyBuffer(device, &data.radiance_srv, &data.radiance_texture);
  bvh::DestroyBuffer(device, &data.stats_uav, &data.stats_buffer);
  bvh::DestroyGpuTimer(&data.timer);
  if (data.pipeline.handle != 0u) device->destroy_pipeline(data.pipeline);
  if (data.srv_table.handle != 0u) device->free_descriptor_table(data.srv_table);
  if (data.uav_table.handle != 0u) device->free_descriptor_table(data.uav_table);
  if (data.cbv_table.handle != 0u) device->free_descriptor_table(data.cbv_table);
  if (data.layout.handle != 0u) device->destroy_pipeline_layout(data.layout);
  g_rtgi_devices.erase(found);
}

}  // namespace falcom_world::rtgi
