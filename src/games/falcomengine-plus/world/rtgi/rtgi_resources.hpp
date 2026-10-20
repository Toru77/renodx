#pragma once

// RTGI GPU resources: the four compute pipelines (trace world_rtgi, temporal world_rtgi_temporal, spatial filter
// world_rtgi_filter, upsample world_rtgi_upsample), the stats buffer, the debug texture (r8g8b8a8_unorm at the depth
// size, created while RTGI Debug is 1 to 7), the raw GI, the history, the filter targets, the final GI and the radiance
// copy. Created lazily from the lighting hook (rtgi.hpp); no module mutex is held around any graphics call here.

#include <cstddef>
#include <cstdint>
#include <span>
#include <unordered_map>

#include "../../../../utils/log.hpp"
#include "../rtao/rtao_resources.hpp"

namespace falcom_world::rtgi {

// t0..t8 BVH, t9 game depth, t10..t13 deforming (null: not traced), t14..t16 alpha, t17 MRT normal, t18 IS-FAST noise,
// t19 P2b records (the RTAO pass A table), t20 radiance: the previous frame's lighting output, t21 the previous final
// GI and t22 the G-buffer colour (the one-bounce correction; null unless kRtgiFlagSubtractPrevious).
inline constexpr uint32_t kRtgiSrvCount = 23u;
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
inline constexpr uint32_t kRtgiStatLuminanceSum = 17u;  // sum of min(luminance, cap) x 10, low word
inline constexpr uint32_t kRtgiStatLuminanceMax = 18u;
inline constexpr uint32_t kRtgiStatClamped = 19u;
inline constexpr uint32_t kRtgiStatLuminanceSumHi = 23u;  // high word of the luminance sum (RTGI_STAT_LUMINANCE_SUM_HI)
inline constexpr float kRtgiLuminanceStatCap = 1000.f;    // per-sample cap of the luminance statistic (RTGI_LUMINANCE_STAT_CAP)
inline constexpr uint32_t kRtgiFlagRadianceValid = 1u;  // b12 c2.z bit (RTGI_FLAG_* in the shader)
inline constexpr uint32_t kRtgiFlagSubtractPrevious = 2u;  // b12 c2.z bit: subtract last frame's applied bounce from lookups
// Temporal pass counters, matching RTGI_STAT_* in world_rtgi_temporal.cs_5_0.hlsl.
inline constexpr uint32_t kRtgiStatTemporalPixels = 20u;
inline constexpr uint32_t kRtgiStatNoHistory = 21u;
inline constexpr uint32_t kRtgiStatResetPixels = 22u;
// Temporal: t0 raw GI, t1 depth, t2 MRT normal, t3 motion, t4 history colour, t5 history normal; u0 history colour
// write, u1 stats, u2 history normal write, u3 debug; s0 point, s1 linear.
inline constexpr uint32_t kRtgiTemporalSrvCount = 6u;
inline constexpr uint32_t kRtgiTemporalUavCount = 4u;
inline constexpr uint32_t kRtgiTemporalSamplerCount = 2u;
inline constexpr uint32_t kRtgiTemporalPushCount = 20u;
// Filter: t0 depth, t1 MRT normal, t2 input; u0 output.
inline constexpr uint32_t kRtgiFilterSrvCount = 3u;
inline constexpr uint32_t kRtgiFilterUavCount = 1u;
inline constexpr uint32_t kRtgiFilterPushCount = 12u;
// Upsample: t0 depth, t1 MRT normal, t2 input, t3 G-buffer colour (Debug 7); u0 final GI, u1 debug.
inline constexpr uint32_t kRtgiUpsampleSrvCount = 4u;
inline constexpr uint32_t kRtgiUpsampleUavCount = 2u;
inline constexpr uint32_t kRtgiUpsamplePushCount = 12u;

#if defined(__world_rtgi_EMBED_FILE)
inline constexpr std::span<const std::uint8_t> kRtgiTraceShader = __world_rtgi;
#else
inline constexpr std::span<const std::uint8_t> kRtgiTraceShader = {};
#endif
#if defined(__world_rtgi_temporal_EMBED_FILE)
inline constexpr std::span<const std::uint8_t> kRtgiTemporalShader = __world_rtgi_temporal;
#else
inline constexpr std::span<const std::uint8_t> kRtgiTemporalShader = {};
#endif
#if defined(__world_rtgi_filter_EMBED_FILE)
inline constexpr std::span<const std::uint8_t> kRtgiFilterShader = __world_rtgi_filter;
#else
inline constexpr std::span<const std::uint8_t> kRtgiFilterShader = {};
#endif
#if defined(__world_rtgi_upsample_EMBED_FILE)
inline constexpr std::span<const std::uint8_t> kRtgiUpsampleShader = __world_rtgi_upsample;
#else
inline constexpr std::span<const std::uint8_t> kRtgiUpsampleShader = {};
#endif

// b12 for the RTGI trace, matching cbuffer cb_rtgi in world_rtgi.cs_5_0.hlsl (six float4 registers).
struct RtgiPushConstants {
  float params[4];    // c0: effective ray max, normal bias, spp, trace divisor
  float sampling[4];  // c1: isfast flag (1 loaded), slice base, seed, debug mode
  float fade[4];      // c2: fade start_eff, fade end_eff, flags (kRtgiFlag* as a float), P2b record count (w, uint bits)
  float region[4];    // c3: region min xyz, region size
  float size[4];      // c4: trace width, trace height, full width, full height
  float shading[4];   // c5: ambient contribution, radiance clamp (0 = off), last frame's applied GI Intensity (one bounce), prevViewProj_g register (RtgiPrevViewProjRegister)
};
static_assert(sizeof(RtgiPushConstants) == kRtgiPushConstantCount * sizeof(float), "b12 must be 24 floats");
static_assert(offsetof(RtgiPushConstants, sampling) == 4 * sizeof(float), "b12 c1 offset");
static_assert(offsetof(RtgiPushConstants, fade) == 8 * sizeof(float), "b12 c2 offset");
static_assert(offsetof(RtgiPushConstants, region) == 12 * sizeof(float), "b12 c3 offset");
static_assert(offsetof(RtgiPushConstants, size) == 16 * sizeof(float), "b12 c4 offset");
static_assert(offsetof(RtgiPushConstants, shading) == 20 * sizeof(float), "b12 c5 offset");

// b12 for the RTGI temporal pass, matching cbuffer cb_rtgi_temporal in world_rtgi_temporal.cs_5_0.hlsl.
struct RtgiTemporalPushConstants {
  float params[4];    // c0: history weight, depth rejection, normal rejection, history clamp (sigma)
  float texel[4];     // c1: full-res texel size xy, prevResolutionScale xy (1, 1)
  float size[4];      // c2: trace width, trace height, debug mode, history valid (0 = reset)
  float full[4];      // c3: full width, full height, trace divisor, unused
  float reserved[4];  // c4: unused
};
static_assert(sizeof(RtgiTemporalPushConstants) == kRtgiTemporalPushCount * sizeof(float), "temporal b12 must be 20 floats");

// b12 for the RTGI spatial filter, matching cbuffer cb_rtgi_filter in world_rtgi_filter.cs_5_0.hlsl.
struct RtgiFilterPushConstants {
  float size[4];  // c0: trace width, trace height, radius, kind (0 separable, 1 a-trous)
  float pass[4];  // c1: direction (0 horizontal, 1 vertical, 2 2D), step, taps (odd), last pass
  float full[4];  // c2: full width, full height, trace divisor, unused
};
static_assert(sizeof(RtgiFilterPushConstants) == kRtgiFilterPushCount * sizeof(float), "filter b12 must be 12 floats");

// b12 for the RTGI upsample, matching cbuffer cb_rtgi_upsample in world_rtgi_upsample.cs_5_0.hlsl.
struct RtgiUpsamplePushConstants {
  float size[4];      // c0: full width, full height, trace width, trace height
  float params[4];    // c1: trace divisor, debug mode, GI Intensity, Character GI Intensity (Debug 7)
  float reserved[4];  // c2: unused
};
static_assert(sizeof(RtgiUpsamplePushConstants) == kRtgiUpsamplePushCount * sizeof(float), "upsample b12 must be 12 floats");

// One RTGI compute pipeline: layout params cbv (0), srv (1), uav (2), an optional sampler table (3), then the push
// constants at b12 (param 3, or 4 with samplers).
struct RtgiPipeline {
  reshade::api::pipeline_layout layout = {0u};
  reshade::api::descriptor_table cbv_table = {0u};
  reshade::api::descriptor_table srv_table = {0u};
  reshade::api::descriptor_table uav_table = {0u};
  reshade::api::descriptor_table sampler_table = {0u};
  reshade::api::pipeline pipeline = {0u};
  bool failed = false;  // creation failed: latched for the session, logged once
};

struct RtgiPipelineDesc {
  uint32_t srv_count;
  uint32_t uav_count;
  uint32_t sampler_count;  // 0: no sampler table
  uint32_t push_count;
  std::span<const std::uint8_t> code;  // empty: the shader is not embedded
  const char* name;                    // log name
};

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
  RtgiPipeline trace_pipeline;
  RtgiPipeline temporal_pipeline;
  RtgiPipeline filter_pipeline;
  RtgiPipeline upsample_pipeline;
  reshade::api::sampler point_sampler = {0u};
  reshade::api::sampler linear_sampler = {0u};
  reshade::api::resource stats_buffer = {0u};
  reshade::api::resource_view stats_uav = {0u};
  RtgiTextureSet<1> debug;  // r8g8b8a8_unorm, depth size; created while RTGI Debug is 1 to 7
  RtgiTextureSet<1> raw;    // rgba16f, trace size: rgb GI, a = 1 traced, -1 not traced
  RtgiTextureSet<2> history_color;   // rgba16f, trace size: rgb accumulated GI, a = distance (0 = invalid)
  RtgiTextureSet<2> history_normal;  // r16g16_float, trace size: octahedral normal
  uint32_t history_index = 0u;       // history[history_index] is read by the temporal pass; the other one is written
  RtgiTextureSet<3> filter;          // rgba16f, trace size: F0, F1, B; created while RTGI Spatial Filter is on
  RtgiTextureSet<2> final_gi;        // r11g11b10_float, depth size: the upsampled GI
  uint32_t final_index = 0u;         // final_gi[final_index] is the newest
  reshade::api::resource radiance_texture = {0u};
  reshade::api::resource_view radiance_srv = {0u};
  uint32_t radiance_width = 0u;
  uint32_t radiance_height = 0u;
  reshade::api::format radiance_format = reshade::api::format::unknown;
  uint64_t radiance_frame = UINT64_MAX;
  bool radiance_failed = false;
  bvh::GpuTimer timer = {};
  bvh::GpuTimer denoise_timer = {};  // temporal, filter and upsample
};

// One entry per device (destroyed with the device).
inline std::unordered_map<reshade::api::device*, RtgiDeviceData> g_rtgi_devices;

inline RtgiDeviceData& GetRtgiDeviceData(reshade::api::device* device) { return g_rtgi_devices[device]; }

inline void DestroyRtgiDeviceData(reshade::api::device* device) {
  const auto found = g_rtgi_devices.find(device);
  if (found == g_rtgi_devices.end()) return;
  RtgiDeviceData& data = found->second;
  DestroyRtgiTextureSet(device, &data.debug);
  DestroyRtgiTextureSet(device, &data.raw);
  DestroyRtgiTextureSet(device, &data.history_color);
  DestroyRtgiTextureSet(device, &data.history_normal);
  DestroyRtgiTextureSet(device, &data.filter);
  DestroyRtgiTextureSet(device, &data.final_gi);
  bvh::DestroyBuffer(device, &data.radiance_srv, &data.radiance_texture);
  bvh::DestroyBuffer(device, &data.stats_uav, &data.stats_buffer);
  bvh::DestroyGpuTimer(&data.timer);
  bvh::DestroyGpuTimer(&data.denoise_timer);
  if (data.point_sampler.handle != 0u) device->destroy_sampler(data.point_sampler);
  if (data.linear_sampler.handle != 0u) device->destroy_sampler(data.linear_sampler);
  const auto destroy_pipeline = [device](const RtgiPipeline& p) {
    if (p.pipeline.handle != 0u) device->destroy_pipeline(p.pipeline);
    if (p.srv_table.handle != 0u) device->free_descriptor_table(p.srv_table);
    if (p.uav_table.handle != 0u) device->free_descriptor_table(p.uav_table);
    if (p.cbv_table.handle != 0u) device->free_descriptor_table(p.cbv_table);
    if (p.sampler_table.handle != 0u) device->free_descriptor_table(p.sampler_table);
    if (p.layout.handle != 0u) device->destroy_pipeline_layout(p.layout);
  };
  destroy_pipeline(data.trace_pipeline);
  destroy_pipeline(data.temporal_pipeline);
  destroy_pipeline(data.filter_pipeline);
  destroy_pipeline(data.upsample_pipeline);
  g_rtgi_devices.erase(found);
}

}  // namespace falcom_world::rtgi
