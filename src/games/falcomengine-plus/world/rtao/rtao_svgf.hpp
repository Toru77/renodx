#pragma once

// RTAO SVGF glue: the trace-size raw AO (rtao.hpp pass A) goes through the input pass (world_rtao_svgf_input), the shared
// SVGF chain (rtgi::EnsureRtgiDenoise, rtgi::DispatchRtgiDenoise) and the byte upsample (gtvbao_svgf_upsample), which writes
// the full-size AO the lighting, micro shadows and the deferred list read. Kept out of rtao.hpp: rtgi.hpp includes rtao.hpp.
// No module mutex is held around any graphics call here.

#include <cstdint>
#include <span>
#include <unordered_map>
#include <vector>

#include "../rtgi/rtgi.hpp"
#include "rtao.hpp"

namespace falcom_world::rtao {

#if defined(__world_rtao_svgf_input_EMBED_FILE)
inline constexpr std::span<const std::uint8_t> kRtaoSvgfInputShader = __world_rtao_svgf_input;
#else
inline constexpr std::span<const std::uint8_t> kRtaoSvgfInputShader = {};
#endif
#if defined(__gtvbao_svgf_upsample_EMBED_FILE)
inline constexpr std::span<const std::uint8_t> kRtaoSvgfUpsampleShader = __gtvbao_svgf_upsample;
#else
inline constexpr std::span<const std::uint8_t> kRtaoSvgfUpsampleShader = {};
#endif

inline constexpr uint32_t kRtaoSvgfInputPushCount = 4u;
inline constexpr rtgi::RtgiPipelineDesc kRtaoSvgfInputDesc = {.srv_count = 1u, .uav_count = 1u, .sampler_count = 0u, .push_count = kRtaoSvgfInputPushCount, .code = kRtaoSvgfInputShader, .name = "rtao svgf input"};
inline constexpr rtgi::RtgiPipelineDesc kRtaoSvgfUpsampleDesc = {.srv_count = rtgi::kRtgiUpsampleSrvCount,
                                                                 .uav_count = rtgi::kRtgiUpsampleUavCount,
                                                                 .sampler_count = 0u,
                                                                 .push_count = rtgi::kRtgiUpsamplePushCount,
                                                                 .code = kRtaoSvgfUpsampleShader,
                                                                 .name = "rtao upsample"};

// The reprojection counters are the RTAO stats 20..22 (the same indices as RTGI and the reprojection shader).
static_assert(kRtaoStatDenoiseBase == rtgi::kRtgiStatTemporalPixels, "RTAO SVGF stats must start at the reprojection base");
static_assert(kRtaoStatDenoiseBase + 2u == rtgi::kRtgiStatResetPixels, "RTAO SVGF reset counter index");
static_assert(kRtaoStatDenoiseBase + 3u <= kRtaoStatsCount, "RTAO SVGF counters must fit the stats buffer");

struct RtaoSvgfData {
  rtgi::RtgiTextureSet<1> raw;  // rgba16f, trace size: rgb = visibility, a = 1 traced, -1 not traced
  rtgi::RtgiPipeline input_pipeline;
  rtgi::RtgiDenoiseSet denoise;  // output: r32_uint, the full-size AO byte
};

// One entry per device (destroyed with the device).
inline std::unordered_map<reshade::api::device*, RtaoSvgfData> g_rtao_svgf_devices;

inline RtaoSvgfData& GetRtaoSvgfData(reshade::api::device* device) { return g_rtao_svgf_devices[device]; }

// The inputs of one frame of the SVGF glue.
struct RtaoSvgfInputs {
  reshade::api::resource_view depth_view = {0u};
  reshade::api::resource_view mrt_normal_view = {0u};
  reshade::api::resource_view scene_cbv_view = {0u};
  reshade::api::resource_view motion_view = {0u};
  uint32_t full_width = 0u;
  uint32_t full_height = 0u;
  uint32_t trace_width = 0u;
  uint32_t trace_height = 0u;
  uint32_t divisor = 1u;
  bool temporal = false;
  bool spatial = false;
  bool bypass_svgf = false;
  bool stats = false;  // the reprojection writes the counters (stats buffer bound)
  rtgi::SvgfSettings svgf = {};
};

// Creates the SVGF objects for this frame's sizes: the upsample and input pipelines, the raw target, then the chain
// (rtgi::EnsureRtgiDenoise). A failure returns a not-ok result, and the caller produces no AO this frame.
inline rtgi::RtgiDenoiseReady EnsureRtaoSvgf(reshade::api::device* device, const RtaoSvgfInputs& in) {
  RtaoSvgfData& sd = GetRtaoSvgfData(device);
  if (!rtgi::EnsureRtgiPipeline(device, &sd.denoise.upsample_pipeline, kRtaoSvgfUpsampleDesc)
      || !rtgi::EnsureRtgiPipeline(device, &sd.input_pipeline, kRtaoSvgfInputDesc)
      || !rtgi::EnsureRtgiTextureSet(device, &sd.raw, reshade::api::format::r16g16b16a16_float, 8u, in.trace_width, in.trace_height)) {
    return {};
  }
  return rtgi::EnsureRtgiDenoise(device, &sd.denoise,
                                 {.output_format = reshade::api::format::r32_uint, .output_bytes_per_pixel = 4u, .trace_width = in.trace_width, .trace_height = in.trace_height, .full_width = in.full_width, .full_height = in.full_height, .temporal = in.temporal, .spatial = in.spatial, .log_prefix = "[world-rtao]", .full_resolution = in.svgf.full_resolution, .nearest_fallback = true, .bypass_svgf = in.bypass_svgf});
}

// The newest AO output (the upsampled SVGF output); null when the chain has not produced one.
inline reshade::api::resource_view OutputSrv(reshade::api::device* device) {
  const auto found = g_rtao_svgf_devices.find(device);
  if (found == g_rtao_svgf_devices.end()) return {0u};
  return found->second.denoise.output.srv[found->second.denoise.output_index];
}

// Trace Resolution change or RTAO off: releases the trace target and the SVGF textures (history included); the pipelines,
// samplers, stats and timers stay. find only: no entry is created.
inline void ReleaseRtaoTextures(reshade::api::device* device) {
  const auto trace = g_rtao_devices.find(device);
  if (trace != g_rtao_devices.end()) DestroyRtaoTraceTarget(device, &trace->second);
  const auto svgf = g_rtao_svgf_devices.find(device);
  if (svgf == g_rtao_svgf_devices.end()) return;
  rtgi::DestroyRtgiTextureSet(device, &svgf->second.raw);
  rtgi::ReleaseRtgiDenoiseTextures(device, &svgf->second.denoise);
}

// Destroys the SVGF objects of one device (the device is going away).
inline void DestroyRtaoSvgfData(reshade::api::device* device) {
  const auto found = g_rtao_svgf_devices.find(device);
  if (found == g_rtao_svgf_devices.end()) return;
  RtaoSvgfData& sd = found->second;
  rtgi::DestroyRtgiTextureSet(device, &sd.raw);
  rtgi::DestroyRtgiPipeline(device, sd.input_pipeline);
  rtgi::DestroyRtgiDenoiseSet(device, &sd.denoise);
  g_rtao_svgf_devices.erase(found);
}

// The SVGF glue after pass A: the input pass (trace raw AO to the raw target, -1 kept as not traced), then the shared chain and
// its upsample. The input pass is outside the pass A timer and outside the chain timer, as in GTVBAO. Table nulls are applied
// at the end.
inline void DispatchRtaoSvgf(reshade::api::device* device, reshade::api::command_list* cmd_list, const RtaoSvgfInputs& in,
                             const rtgi::RtgiDenoiseReady& ready) {
  const bvh::PoolCpuTimer post_timer{&bvh::g_perf_gate.block_us[bvh::kPerfBlockRtaoPost]};
  RtaoSvgfData& sd = GetRtaoSvgfData(device);
  const RtaoDeviceData& rd = GetRtaoDeviceData(device);
  const auto UA = reshade::api::resource_usage::unordered_access;
  const auto SR = reshade::api::resource_usage::shader_resource;
  const auto CS = reshade::api::shader_stage::all_compute;
  static const reshade::api::resource_view kNull[1] = {};
  std::vector<reshade::api::descriptor_table_update> table_nulls;
  // Input pass: the trace raw AO (t0) to the SVGF raw layout (u0, rgba16f).
  {
    const rtgi::RtgiPipeline& ip = sd.input_pipeline;
    reshade::api::descriptor_table_update iu[2] = {
        {ip.srv_table, 0, 0, 1u, reshade::api::descriptor_type::shader_resource_view, &rd.trace_srv},
        {ip.uav_table, 0, 0, 1u, reshade::api::descriptor_type::unordered_access_view, &sd.raw.uav[0]},
    };
    device->update_descriptor_tables(2, iu);
    cmd_list->bind_pipeline(reshade::api::pipeline_stage::all_compute, ip.pipeline);
    const reshade::api::descriptor_table ip_tables[2] = {ip.srv_table, ip.uav_table};
    cmd_list->bind_descriptor_tables(CS, ip.layout, 1, 2, ip_tables);
    const float ipush[kRtaoSvgfInputPushCount] = {static_cast<float>(in.trace_width), static_cast<float>(in.trace_height), 0.f, 0.f};
    cmd_list->push_constants(CS, ip.layout, 3, 0, kRtaoSvgfInputPushCount, ipush);
    cmd_list->dispatch((in.trace_width + 7u) / 8u, (in.trace_height + 7u) / 8u, 1u);
    NullComputeSlots(cmd_list, 1, 1, 0);
    cmd_list->barrier(sd.raw.texture[0], UA, SR);
    table_nulls.push_back({ip.srv_table, 0, 0, 1u, reshade::api::descriptor_type::shader_resource_view, kNull});
    table_nulls.push_back({ip.uav_table, 0, 0, 1u, reshade::api::descriptor_type::unordered_access_view, kNull});
  }
  rtgi::DispatchRtgiDenoise(device, cmd_list, &sd.denoise,
                            {.raw_view = sd.raw.srv[0],
                             .depth_view = in.depth_view,
                             .mrt_normal_view = in.mrt_normal_view,
                             .scene_cbv_view = in.scene_cbv_view,
                             .motion_view = in.motion_view,
                             .albedo_view = {0u},
                             .stats_uav = in.stats ? rd.stats_uav : reshade::api::resource_view{0u},
                             .debug_uav = {0u},
                             .debug_texture = {0u},
                             .trace_width = in.trace_width,
                             .trace_height = in.trace_height,
                             .full_width = in.full_width,
                             .full_height = in.full_height,
                             .divisor = static_cast<float>(in.divisor),
                             .debug_mode = 0,
                             .svgf = in.svgf,
                             .luminance_weights = rtgi::kSvgfLuminanceAo,
                             .upsample_intensity = 1.f,
                             .upsample_char_intensity = 1.f,
                             .upsample_saturation = 1.f},
                            ready, &table_nulls);
  device->update_descriptor_tables(static_cast<uint32_t>(table_nulls.size()), table_nulls.data());
}

}  // namespace falcom_world::rtao
