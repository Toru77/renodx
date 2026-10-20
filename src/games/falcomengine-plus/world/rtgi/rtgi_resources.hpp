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
// t19 P2b records (the RTAO pass A table).
inline constexpr uint32_t kRtgiSrvCount = 20u;
inline constexpr uint32_t kRtgiUavCount = 2u;  // u0 debug texture (null unless RTGI Debug is 1 to 6), u1 stats
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
  float shading[4];   // c5: reserved for the hit shading (M2)
};
static_assert(sizeof(RtgiPushConstants) == kRtgiPushConstantCount * sizeof(float), "b12 must be 24 floats");
static_assert(offsetof(RtgiPushConstants, sampling) == 4 * sizeof(float), "b12 c1 offset");
static_assert(offsetof(RtgiPushConstants, fade) == 8 * sizeof(float), "b12 c2 offset");
static_assert(offsetof(RtgiPushConstants, region) == 12 * sizeof(float), "b12 c3 offset");
static_assert(offsetof(RtgiPushConstants, size) == 16 * sizeof(float), "b12 c4 offset");
static_assert(offsetof(RtgiPushConstants, shading) == 20 * sizeof(float), "b12 c5 offset");

struct RtgiDeviceData {
  reshade::api::pipeline_layout layout = {0u};
  reshade::api::descriptor_table cbv_table = {0u};
  reshade::api::descriptor_table srv_table = {0u};
  reshade::api::descriptor_table uav_table = {0u};
  reshade::api::pipeline pipeline = {0u};
  bool pipeline_failed = false;
  reshade::api::resource stats_buffer = {0u};
  reshade::api::resource_view stats_uav = {0u};
  reshade::api::resource debug_texture = {0u};
  reshade::api::resource_view debug_srv = {0u};
  reshade::api::resource_view debug_uav = {0u};
  uint32_t debug_width = 0u;
  uint32_t debug_height = 0u;
  bvh::GpuTimer timer = {};
};

// One entry per device (destroyed with the device).
inline std::unordered_map<reshade::api::device*, RtgiDeviceData> g_rtgi_devices;

inline RtgiDeviceData& GetRtgiDeviceData(reshade::api::device* device) { return g_rtgi_devices[device]; }

inline void DestroyRtgiDebugTarget(reshade::api::device* device, RtgiDeviceData* data) {
  bvh::DestroyBuffer(device, &data->debug_srv, &data->debug_texture);
  bvh::DestroyBuffer(device, &data->debug_uav, nullptr);
  data->debug_width = 0u;
  data->debug_height = 0u;
}

inline void DestroyRtgiDeviceData(reshade::api::device* device) {
  const auto found = g_rtgi_devices.find(device);
  if (found == g_rtgi_devices.end()) return;
  RtgiDeviceData& data = found->second;
  DestroyRtgiDebugTarget(device, &data);
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
