#pragma once

// Shared SVGF denoise chain (reprojection, variance, a-trous) and upsample kept for the surviving effects.
// No module mutex is held around any graphics call here.

#include <algorithm>
#include <atomic>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

#include "../bvh/bvh_trace.hpp"
#include "../rr/rr_state.hpp"
#include "../rt12/rt12_mirror.hpp"
#include "../compute/compute.hpp"
#include "../world_state.hpp"
#include "denoise_resources.hpp"

namespace falcom_world::denoise {

// What a trace owner asks the denoise chain to prepare for this frame.
struct DenoiseRequest {
  reshade::api::format output_format;  // shared denoise output, r11g11b10_float
  uint32_t output_bytes_per_pixel;
  uint32_t trace_width;
  uint32_t trace_height;
  uint32_t full_width;
  uint32_t full_height;
  bool temporal;           // Temporal on and a motion view available
  bool spatial;            // Spatial filter on
  const char* log_prefix;  // the owning pass's prefix, e.g. "[world-compute]"
  bool full_resolution = false;
  bool nearest_fallback = false;
  bool history_only = false;
  bool bypass_svgf = false;  // RR owns denoising; keep input conversion and output reconstruction only.
};

// The passes the chain can run this frame (ok: the output exists).
struct DenoiseReady {
  bool ok = false;
  bool reproject = false;  // the reprojection runs (needed by the temporal and the spatial filter)
  bool temporal = false;
  bool spatial = false;
};

// Creates the chain's objects for this frame's sizes, in this order: the output (full size), the reprojection pipeline,
// history (colour, normal, moments, guide) and samplers when requested, the variance and a-trous pipelines and the
// integrated and a-trous targets when the spatial filter is requested. A new or resized history starts at index 0 (all
// invalid). The owner ensures the upsample pipeline itself.
inline DenoiseReady EnsureDenoise(reshade::api::device* device, DenoiseSet* set, const DenoiseRequest& request) {
  const bool temporal = request.temporal && !request.bypass_svgf;
  const bool spatial = request.spatial && !request.bypass_svgf;
  if (!request.full_resolution && set->reconstructed.texture[0].handle != 0u) {
    DestroyTextureSet(device, &set->reconstructed);
  }
  if (!spatial) {
    DestroyTextureSet(device, &set->integrated);
    DestroyTextureSet(device, &set->atrous);
    set->atrous_iterations = 0u;
  }
  if (!temporal && !spatial) {
    DestroyTextureSet(device, &set->history_color);
    DestroyTextureSet(device, &set->history_normal);
    DestroyTextureSet(device, &set->history_moments);
    DestroyTextureSet(device, &set->history_guide);
    set->history_index = 0u;
    set->mode_valid = false;
  }
  if (!request.history_only && !EnsureTextureSet(device, &set->output, request.output_format, request.output_bytes_per_pixel, request.full_width, request.full_height)) {
    renodx::utils::log::w(request.log_prefix, " final texture creation failed");
    set->mode_valid = false;
    return {};
  }
  const uint32_t denoise_width = request.full_resolution ? request.full_width : request.trace_width;
  const uint32_t denoise_height = request.full_resolution ? request.full_height : request.trace_height;
  if (request.full_resolution) {
    const bool ao = request.output_format == reshade::api::format::r32_uint;
    if (!EnsureComputePipeline(device, &set->reconstruct_pipeline, {.srv_count = kUpsampleSrvCount, .uav_count = kUpsampleUavCount, .sampler_count = 0u, .push_count = kUpsamplePushCount, .code = request.nearest_fallback ? kSvgfReconstructFallbackShader : kSvgfReconstructShader, .name = "svgf reconstruct"})
        || !EnsureComputePipeline(device, &set->finalize_pipeline, {.srv_count = kUpsampleSrvCount, .uav_count = kUpsampleUavCount, .sampler_count = 0u, .push_count = kUpsamplePushCount, .code = ao ? kSvgfFinalizeAoShader : kSvgfFinalizeShader, .name = "svgf finalize"})
        || !EnsureTextureSet(device, &set->reconstructed, reshade::api::format::r16g16b16a16_float, 8u, request.full_width, request.full_height)) {
      set->mode_valid = false;
      return {};
    }
  }
  bool reproject_run = (temporal || spatial)
                       && EnsureComputePipeline(device, &set->reproject_pipeline, {.srv_count = kSvgfReprojectSrvCount, .uav_count = kSvgfReprojectUavCount, .sampler_count = kSvgfReprojectSamplerCount, .push_count = kSvgfPushCount, .code = kSvgfReprojectShader, .name = "reproject"});
  if (reproject_run) {
    const bool history_recreated = set->history_color.texture[0].handle == 0u || set->history_color.width != denoise_width
                                   || set->history_color.height != denoise_height;
    reproject_run = EnsureTextureSet(device, &set->history_color, reshade::api::format::r16g16b16a16_float, 8u, denoise_width, denoise_height)
                    && EnsureTextureSet(device, &set->history_normal, reshade::api::format::r16g16_float, 4u, denoise_width, denoise_height)
                    && EnsureTextureSet(device, &set->history_moments, reshade::api::format::r32g32b32a32_float, 16u, denoise_width, denoise_height)
                    && EnsureTextureSet(device, &set->history_guide, reshade::api::format::r32g32b32a32_float, 16u, denoise_width, denoise_height);
    if (!reproject_run) {
      renodx::utils::log::w(request.log_prefix, " history texture creation failed");
    } else if (history_recreated) {
      set->history_index = 0u;
      set->mode_valid = false;
    }
  }
  if (reproject_run && set->point_sampler.handle == 0u) {
    reshade::api::sampler_desc sd = {};
    sd.filter = reshade::api::filter_mode::min_mag_mip_point;
    sd.address_u = sd.address_v = sd.address_w = reshade::api::texture_address_mode::clamp;
    reproject_run = device->create_sampler(sd, &set->point_sampler);
  }
  if (reproject_run && set->linear_sampler.handle == 0u) {
    reshade::api::sampler_desc sd = {};
    sd.filter = reshade::api::filter_mode::min_mag_mip_linear;
    sd.address_u = sd.address_v = sd.address_w = reshade::api::texture_address_mode::clamp;
    reproject_run = device->create_sampler(sd, &set->linear_sampler);
  }
  bool spatial_run = reproject_run && spatial
                     && EnsureComputePipeline(device, &set->variance_pipeline, {.srv_count = kSvgfVarianceSrvCount, .uav_count = kSvgfVarianceUavCount, .sampler_count = 0u, .push_count = kSvgfPushCount, .code = kSvgfVarianceShader, .name = "variance"})
                     && EnsureComputePipeline(device, &set->atrous_pipeline, {.srv_count = kSvgfAtrousSrvCount, .uav_count = kSvgfAtrousUavCount, .sampler_count = 0u, .push_count = kSvgfPushCount, .code = kSvgfAtrousShader, .name = "atrous"})
                     && EnsureTextureSet(device, &set->integrated, reshade::api::format::r16g16b16a16_float, 8u, denoise_width, denoise_height)
                     && EnsureTextureSet(device, &set->atrous, reshade::api::format::r16g16b16a16_float, 8u, denoise_width, denoise_height);
  const std::span<const std::uint8_t> kTileCode[kSvgfAtrousTileSteps] = {kSvgfAtrousS1Shader, kSvgfAtrousS2Shader, kSvgfAtrousS4Shader};
  const char* kTileName[kSvgfAtrousTileSteps] = {"atrous s1", "atrous s2", "atrous s4"};
  for (uint32_t k = 0u; k < kSvgfAtrousTileSteps; ++k) {
    spatial_run = spatial_run && EnsureComputePipeline(device, &set->atrous_tile_pipeline[k], {.srv_count = kSvgfAtrousSrvCount, .uav_count = kSvgfAtrousUavCount, .sampler_count = 0u, .push_count = kSvgfPushCount, .code = kTileCode[k], .name = kTileName[k]});
  }
  if (request.full_resolution && ((temporal || spatial) && !reproject_run || (spatial && !spatial_run))) {
    set->mode_valid = false;
    return {};
  }
  return {.ok = true, .reproject = reproject_run, .temporal = reproject_run && temporal, .spatial = spatial_run};
}

struct UpsampleSettings {
  float algorithm = 0.f;                  // 0 = current joint bilateral, 1 = bilinear, 2 = nearest valid, 3 = wider joint bilateral
  float depth_plane_sigma = 0.02f;        // current joint-bilateral default
  float normal_edge_power = 16.f;         // current joint-bilateral default
  float bilinear_weight_sharpness = 1.f;  // 1 = unchanged bilinear weights
  float nearest_valid_fallback = 1.f;     // current GTVBAO/GTVBGI fallback behavior
};

// The inputs of one run of the chain.
struct DenoiseInputs {
  reshade::api::resource_view raw_view;  // trace size, a < 0 = not traced
  reshade::api::resource_view depth_view;
  reshade::api::resource_view mrt_normal_view;
  reshade::api::resource_view scene_cbv_view;
  reshade::api::resource_view motion_view;  // RTV4 (read only when ready.temporal)
  reshade::api::resource_view albedo_view;  // upsample t3 (Debug 7); null when unused
  reshade::api::resource_view stats_uav;    // temporal u1 (stats 20..22)
  reshade::api::resource_view debug_uav;    // null: no debug view this frame
  reshade::api::resource debug_texture;
  uint32_t trace_width;
  uint32_t trace_height;
  uint32_t full_width;
  uint32_t full_height;
  float divisor;                   // trace divisor d (1, 2 or 4)
  int debug_mode;                  // SVGF numbering: 4 accumulated, 5 history confidence, 6 and 7 upsample views
  SvgfSettings svgf;               // SVGF settings of this run (history valid set by the owner)
  const float* luminance_weights;  // rgb luminance weights of the edge stop (push c3)
  float upsample_intensity;        // upsample c1.z (Debug 7 only)
  float upsample_char_intensity;   // upsample c1.w (Debug 7 only)
  float upsample_saturation;       // upsample c2.x; 1 = unchanged
  UpsampleSettings upsample;   // reconstruction controls; defaults preserve current behavior
  bool history_only = false;
};

struct DenoiseResult {
  bool temporal_debug = false;  // the reprojection wrote the debug texture
  bool variance_debug = false;  // the variance pass wrote the debug texture
  bool upsample_debug = false;  // the upsample wrote the debug texture
};

// Runs the SVGF chain and the upsample into output[output_index ^ 1], which becomes the newest. The reprojection (Sec. 4.1
// and the guide data of Sec. 4.2 and 4.4) writes integrated (spatial on) or history colour (spatial off), the moments, the
// guide and the normal of history[history_index ^ 1], which becomes the read history. The variance (Sec. 4.2) and the
// a-trous iterations (Sec. 4.3) run only with the reprojection. Appends the null updates of its own tables to
// *table_nulls (upsample, then reprojection, then variance and a-trous); the owner applies them with its own.
inline DenoiseResult DispatchDenoise(reshade::api::device* device, reshade::api::command_list* cmd_list,
                                             DenoiseSet* set, const DenoiseInputs& in, const DenoiseReady& ready,
                                             std::vector<reshade::api::descriptor_table_update>* table_nulls) {
  const reshade::api::descriptor_type kCbv = reshade::api::descriptor_type::constant_buffer;
  const reshade::api::descriptor_type kSrv = reshade::api::descriptor_type::shader_resource_view;
  const reshade::api::descriptor_type kUav = reshade::api::descriptor_type::unordered_access_view;
  const uint32_t width = in.svgf.full_resolution ? in.full_width : in.trace_width;
  const uint32_t height = in.svgf.full_resolution ? in.full_height : in.trace_height;
  const float divisor = in.svgf.full_resolution ? 1.f : in.divisor;
  const uint32_t trace_groups_x = (width + 7u) / 8u;
  const uint32_t trace_groups_y = (height + 7u) / 8u;
  const bool reproject_debug = in.debug_uav.handle != 0u && ready.reproject && (in.debug_mode == 4 || in.debug_mode == 5);
  const bool variance_debug = in.debug_uav.handle != 0u && ready.spatial && in.debug_mode == 11;
  const bool upsample_debug = !in.history_only && in.debug_uav.handle != 0u && (in.debug_mode == 6 || in.debug_mode == 7);
  const reshade::api::buffer_range scene_cbv = compute::SceneCbvRange(in.scene_cbv_view);
  bvh::BeginGpuTimer(device, cmd_list, &set->timer);
  // Push constants shared by the passes; each pass sets params[2] (history valid) and params[3] (a-trous step).
  SvgfPushConstants push = {};
  push.size[0] = static_cast<float>(width);
  push.size[1] = static_cast<float>(height);
  push.size[2] = static_cast<float>(in.full_width);
  push.size[3] = static_cast<float>(in.full_height);
  push.params[0] = divisor;
  push.params[1] = static_cast<float>(in.debug_mode);
  push.temporal[0] = 1.f - in.svgf.history_weight;  // alpha
  push.temporal[1] = in.svgf.depth_rejection;
  push.temporal[2] = in.svgf.normal_rejection;
  push.temporal[3] = in.svgf.luminance_edge_stop;  // sigma_l
  push.spatial[0] = in.svgf.depth_sigma;
  push.spatial[1] = in.svgf.normal_power;
  push.spatial[2] = std::isfinite(in.svgf.history_length) ? std::clamp(in.svgf.history_length, 4.f, 128.f) : 32.f;
  const uint32_t radius_scale = (std::isfinite(in.svgf.radius_scale)
                                     ? static_cast<uint32_t>(std::round(std::clamp(in.svgf.radius_scale, 1.f, 4.f)))
                                     : 1u);
  push.spatial[3] = static_cast<float>(radius_scale);
  push.luminance[0] = in.luminance_weights[0];
  push.luminance[1] = in.luminance_weights[1];
  push.luminance[2] = in.luminance_weights[2];
  reshade::api::resource_view input = in.raw_view;
  const bool mode_changed = !set->mode_valid || set->full_resolution != in.svgf.full_resolution
      || (ready.spatial && set->radius_scale != radius_scale);
  if (in.svgf.full_resolution && in.divisor > 1.f) {
    const ComputePipeline& reconstruct = set->reconstruct_pipeline;
    const reshade::api::resource_view srvs[kUpsampleSrvCount] = {in.depth_view, in.mrt_normal_view, in.raw_view, in.albedo_view};
    const reshade::api::resource_view uavs[kUpsampleUavCount] = {set->reconstructed.uav[0], {0u}};
    reshade::api::descriptor_table_update updates[3] = {
        {reconstruct.cbv_table, 0, 0, 1, kCbv, &scene_cbv},
        {reconstruct.srv_table, 0, 0, kUpsampleSrvCount, kSrv, srvs},
        {reconstruct.uav_table, 0, 0, kUpsampleUavCount, kUav, uavs},
    };
    device->update_descriptor_tables(3, updates);
    const UpsamplePushConstants constants = {
        .size = {static_cast<float>(in.full_width), static_cast<float>(in.full_height), static_cast<float>(in.trace_width), static_cast<float>(in.trace_height)},
        .params = {in.divisor, 0.f, 1.f, 1.f},
        .extra = {1.f, in.upsample.algorithm, in.upsample.depth_plane_sigma, in.upsample.normal_edge_power},
        .upscale = {in.upsample.bilinear_weight_sharpness, in.upsample.nearest_valid_fallback, 0.f, 0.f},
    };
    cmd_list->bind_pipeline(reshade::api::pipeline_stage::all_compute, reconstruct.pipeline);
    const reshade::api::descriptor_table tables[3] = {reconstruct.cbv_table, reconstruct.srv_table, reconstruct.uav_table};
    cmd_list->bind_descriptor_tables(reshade::api::shader_stage::all_compute, reconstruct.layout, 0, 3, tables);
    cmd_list->push_constants(reshade::api::shader_stage::all_compute, reconstruct.layout, 3, 0, kUpsamplePushCount, &constants);
    cmd_list->dispatch(trace_groups_x, trace_groups_y, 1u);
    compute::NullComputeSlots(cmd_list, kUpsampleSrvCount, kUpsampleUavCount, 0u);
    cmd_list->barrier(set->reconstructed.texture[0], reshade::api::resource_usage::unordered_access, reshade::api::resource_usage::shader_resource);
    input = set->reconstructed.srv[0];
  }
  if (ready.spatial) {
    set->radius_scale = radius_scale;
  }
  set->full_resolution = in.svgf.full_resolution;
  set->mode_valid = true;
  const uint32_t read = set->history_index;
  const uint32_t write = set->history_index ^ 1u;

  // Reprojection: reads history[read], writes history[write] (and integrated when the spatial filter runs).
  if (ready.reproject) {
    const ComputePipeline& reproject = set->reproject_pipeline;
    const reshade::api::resource_view reproject_srvs[kSvgfReprojectSrvCount] = {
        input, in.depth_view, in.mrt_normal_view, in.motion_view, set->history_color.srv[read],
        set->history_moments.srv[read], set->history_guide.srv[read], set->history_normal.srv[read]};
    const reshade::api::resource_view reproject_uavs[kSvgfReprojectUavCount] = {
        ready.spatial ? set->integrated.uav[0] : set->history_color.uav[write], in.stats_uav, set->history_moments.uav[write],
        set->history_guide.uav[write], set->history_normal.uav[write],
        reproject_debug ? in.debug_uav : reshade::api::resource_view{0u}};
    const reshade::api::sampler samplers[kSvgfReprojectSamplerCount] = {set->point_sampler, set->linear_sampler};
    reshade::api::descriptor_table_update reproject_updates[4] = {
        {reproject.cbv_table, 0, 0, 1, kCbv, &scene_cbv},
        {reproject.srv_table, 0, 0, kSvgfReprojectSrvCount, kSrv, reproject_srvs},
        {reproject.uav_table, 0, 0, kSvgfReprojectUavCount, kUav, reproject_uavs},
        {reproject.sampler_table, 0, 0, kSvgfReprojectSamplerCount, reshade::api::descriptor_type::sampler, samplers},
    };
    device->update_descriptor_tables(4, reproject_updates);
    SvgfPushConstants rpush = push;
    rpush.params[2] = in.svgf.history_valid && ready.temporal && !mode_changed ? 1.f : 0.f;
    rpush.params[3] = 0.f;
    cmd_list->bind_pipeline(reshade::api::pipeline_stage::all_compute, reproject.pipeline);
    const reshade::api::descriptor_table reproject_tables[4] = {
        reproject.cbv_table, reproject.srv_table, reproject.uav_table, reproject.sampler_table};
    cmd_list->bind_descriptor_tables(reshade::api::shader_stage::all_compute, reproject.layout, 0, 4, reproject_tables);
    cmd_list->push_constants(reshade::api::shader_stage::all_compute, reproject.layout, 4, 0, kSvgfPushCount, &rpush);
    cmd_list->dispatch(trace_groups_x, trace_groups_y, 1u);
    compute::NullComputeSlots(cmd_list, kSvgfReprojectSrvCount, kSvgfReprojectUavCount, kSvgfReprojectSamplerCount);
    cmd_list->barrier(ready.spatial ? set->integrated.texture[0] : set->history_color.texture[write],
                      reshade::api::resource_usage::unordered_access, reshade::api::resource_usage::shader_resource);
    cmd_list->barrier(set->history_moments.texture[write], reshade::api::resource_usage::unordered_access,
                      reshade::api::resource_usage::shader_resource);
    cmd_list->barrier(set->history_guide.texture[write], reshade::api::resource_usage::unordered_access,
                      reshade::api::resource_usage::shader_resource);
    cmd_list->barrier(set->history_normal.texture[write], reshade::api::resource_usage::unordered_access,
                      reshade::api::resource_usage::shader_resource);
    if (reproject_debug) {
      cmd_list->barrier(in.debug_texture, reshade::api::resource_usage::unordered_access, reshade::api::resource_usage::shader_resource);
    }
    set->history_index = write;
    if (!ready.spatial) input = set->history_color.srv[write];
  }

  // Variance: the 7x7 estimate while the history is short (Sec. 4.2), into atrous[0].
  if (ready.spatial) {
    const ComputePipeline& variance = set->variance_pipeline;
    const reshade::api::resource_view variance_srvs[kSvgfVarianceSrvCount] = {
        set->integrated.srv[0], set->history_moments.srv[write], set->history_guide.srv[write], set->history_normal.srv[write]};
    const reshade::api::resource_view variance_uavs[kSvgfVarianceUavCount] = {
        set->atrous.uav[0], variance_debug ? in.debug_uav : reshade::api::resource_view{0u}};
    reshade::api::descriptor_table_update variance_updates[3] = {
        {variance.cbv_table, 0, 0, 1, kCbv, &scene_cbv},
        {variance.srv_table, 0, 0, kSvgfVarianceSrvCount, kSrv, variance_srvs},
        {variance.uav_table, 0, 0, kSvgfVarianceUavCount, kUav, variance_uavs},
    };
    device->update_descriptor_tables(3, variance_updates);
    cmd_list->bind_pipeline(reshade::api::pipeline_stage::all_compute, variance.pipeline);
    const reshade::api::descriptor_table variance_tables[3] = {variance.cbv_table, variance.srv_table, variance.uav_table};
    cmd_list->bind_descriptor_tables(reshade::api::shader_stage::all_compute, variance.layout, 0, 3, variance_tables);
    cmd_list->push_constants(reshade::api::shader_stage::all_compute, variance.layout, 3, 0, kSvgfPushCount, &push);
    cmd_list->dispatch(trace_groups_x, trace_groups_y, 1u);
    compute::NullComputeSlots(cmd_list, kSvgfVarianceSrvCount, kSvgfVarianceUavCount, 0u);
    cmd_list->barrier(set->atrous.texture[0], reshade::api::resource_usage::unordered_access, reshade::api::resource_usage::shader_resource);
    if (variance_debug) {
      cmd_list->barrier(in.debug_texture, reshade::api::resource_usage::unordered_access, reshade::api::resource_usage::shader_resource);
    }
  }

  // A-trous: N = max(1, clamp(iterations, 1, 5) - log2(d)) iterations of step 2^i. Iteration 0 reads atrous[0] and writes
  // history[write] (the feedback colour); iteration 1 reads history[write] and writes atrous[1]; from 2 on it reads
  // atrous[(i - 1) & 1] and writes atrous[i & 1]. The last one written is the input of the upsample. Iterations
  // whose actual step is 1, 2 or 4 use group-shared tile kernels; other steps use atrous_pipeline.
  set->atrous_iterations = ready.spatial ? SvgfEffectiveIterations(in.svgf.iterations, static_cast<uint32_t>(divisor + 0.5f)) : 0u;
  if (ready.spatial) {
    for (uint32_t i = 0u; i < set->atrous_iterations; ++i) {
      const uint32_t step = radius_scale * (1u << i);
      const bool tiled = step == 1u || step == 2u || step == 4u;
      const ComputePipeline& atrous = (tiled ? set->atrous_tile_pipeline[std::countr_zero(step)] : set->atrous_pipeline);
      cmd_list->bind_pipeline(reshade::api::pipeline_stage::all_compute, atrous.pipeline);
      const bool to_history = i == 0u;
      const uint32_t slot = i & 1u;
      const reshade::api::resource_view atrous_in = i == 0u ? set->atrous.srv[0]
          : (i == 1u ? set->history_color.srv[write] : set->atrous.srv[(i - 1u) & 1u]);
      const reshade::api::resource_view atrous_srvs[kSvgfAtrousSrvCount] = {
          atrous_in, set->history_guide.srv[write], set->history_normal.srv[write]};
      const reshade::api::resource_view atrous_uavs[kSvgfAtrousUavCount] = {
          to_history ? set->history_color.uav[write] : set->atrous.uav[slot]};
      reshade::api::descriptor_table_update atrous_updates[3] = {
          {atrous.cbv_table, 0, 0, 1, kCbv, &scene_cbv},
          {atrous.srv_table, 0, 0, kSvgfAtrousSrvCount, kSrv, atrous_srvs},
          {atrous.uav_table, 0, 0, kSvgfAtrousUavCount, kUav, atrous_uavs},
      };
      device->update_descriptor_tables(3, atrous_updates);
      SvgfPushConstants apush = push;
      apush.params[2] = 0.f;
      apush.params[3] = static_cast<float>(1u << i);
      const reshade::api::descriptor_table atrous_tables[3] = {atrous.cbv_table, atrous.srv_table, atrous.uav_table};
      cmd_list->bind_descriptor_tables(reshade::api::shader_stage::all_compute, atrous.layout, 0, 3, atrous_tables);
      cmd_list->push_constants(reshade::api::shader_stage::all_compute, atrous.layout, 3, 0, kSvgfPushCount, &apush);
      cmd_list->dispatch(trace_groups_x, trace_groups_y, 1u);
      compute::NullComputeSlots(cmd_list, kSvgfAtrousSrvCount, kSvgfAtrousUavCount, 0u);
      cmd_list->barrier(to_history ? set->history_color.texture[write] : set->atrous.texture[slot],
                        reshade::api::resource_usage::unordered_access, reshade::api::resource_usage::shader_resource);
      input = to_history ? set->history_color.srv[write] : set->atrous.srv[slot];
    }
  }

  static const reshade::api::resource_view null_views[kSvgfReprojectSrvCount] = {};
  static const reshade::api::sampler null_samplers[kSvgfReprojectSamplerCount] = {};
  if (!in.history_only) {
    // Upsample to the depth size: writes final_gi[final_index ^ 1], which becomes the newest.
    const ComputePipeline& upsample = in.svgf.full_resolution ? set->finalize_pipeline : set->upsample_pipeline;
    const uint32_t final_write = set->output_index ^ 1u;
    const reshade::api::resource_view upsample_srvs[kUpsampleSrvCount] = {in.depth_view, in.mrt_normal_view, input, in.albedo_view};
    const reshade::api::resource_view upsample_uavs[kUpsampleUavCount] = {
        set->output.uav[final_write], upsample_debug ? in.debug_uav : reshade::api::resource_view{0u}};
    reshade::api::descriptor_table_update upsample_updates[3] = {
        {upsample.cbv_table, 0, 0, 1, kCbv, &scene_cbv},
        {upsample.srv_table, 0, 0, kUpsampleSrvCount, kSrv, upsample_srvs},
        {upsample.uav_table, 0, 0, kUpsampleUavCount, kUav, upsample_uavs},
    };
    device->update_descriptor_tables(3, upsample_updates);
    const UpsamplePushConstants upush = {
        .size = {static_cast<float>(in.full_width), static_cast<float>(in.full_height), static_cast<float>(width),
                 static_cast<float>(height)},
        .params = {divisor, static_cast<float>(in.debug_mode), in.upsample_intensity, in.upsample_char_intensity},
        .extra = {in.upsample_saturation, in.upsample.algorithm, in.upsample.depth_plane_sigma, in.upsample.normal_edge_power},
        .upscale = {in.upsample.bilinear_weight_sharpness, in.upsample.nearest_valid_fallback, 0.f, 0.f},
    };
    cmd_list->bind_pipeline(reshade::api::pipeline_stage::all_compute, upsample.pipeline);
    const reshade::api::descriptor_table upsample_tables[3] = {upsample.cbv_table, upsample.srv_table, upsample.uav_table};
    cmd_list->bind_descriptor_tables(reshade::api::shader_stage::all_compute, upsample.layout, 0, 3, upsample_tables);
    cmd_list->push_constants(reshade::api::shader_stage::all_compute, upsample.layout, 3, 0, kUpsamplePushCount, &upush);
    cmd_list->dispatch((in.full_width + 7u) / 8u, (in.full_height + 7u) / 8u, 1u);
    compute::NullComputeSlots(cmd_list, kUpsampleSrvCount, kUpsampleUavCount, 0u);
    cmd_list->barrier(set->output.texture[final_write], reshade::api::resource_usage::unordered_access, reshade::api::resource_usage::shader_resource);
    if (upsample_debug) {
      cmd_list->barrier(in.debug_texture, reshade::api::resource_usage::unordered_access, reshade::api::resource_usage::shader_resource);
    }
    set->output_index = final_write;
    table_nulls->push_back({upsample.srv_table, 0, 0, kUpsampleSrvCount, kSrv, null_views});
    table_nulls->push_back({upsample.uav_table, 0, 0, kUpsampleUavCount, kUav, null_views});
  }
  bvh::EndGpuTimer(cmd_list, &set->timer);
  if (in.svgf.full_resolution && in.divisor > 1.f) {
    table_nulls->push_back({set->reconstruct_pipeline.srv_table, 0, 0, kUpsampleSrvCount, kSrv, null_views});
    table_nulls->push_back({set->reconstruct_pipeline.uav_table, 0, 0, kUpsampleUavCount, kUav, null_views});
  }
  if (ready.reproject) {
    table_nulls->push_back({set->reproject_pipeline.srv_table, 0, 0, kSvgfReprojectSrvCount, kSrv, null_views});
    table_nulls->push_back({set->reproject_pipeline.uav_table, 0, 0, kSvgfReprojectUavCount, kUav, null_views});
    table_nulls->push_back({set->reproject_pipeline.sampler_table, 0, 0, kSvgfReprojectSamplerCount,
                            reshade::api::descriptor_type::sampler, null_samplers});
  }
  if (ready.spatial) {
    table_nulls->push_back({set->variance_pipeline.srv_table, 0, 0, kSvgfVarianceSrvCount, kSrv, null_views});
    table_nulls->push_back({set->variance_pipeline.uav_table, 0, 0, kSvgfVarianceUavCount, kUav, null_views});
    table_nulls->push_back({set->atrous_pipeline.srv_table, 0, 0, kSvgfAtrousSrvCount, kSrv, null_views});
    table_nulls->push_back({set->atrous_pipeline.uav_table, 0, 0, kSvgfAtrousUavCount, kUav, null_views});
    for (const ComputePipeline& tile : set->atrous_tile_pipeline) {
      table_nulls->push_back({tile.srv_table, 0, 0, kSvgfAtrousSrvCount, kSrv, null_views});
      table_nulls->push_back({tile.uav_table, 0, 0, kSvgfAtrousUavCount, kUav, null_views});
    }
  }
  return {.temporal_debug = reproject_debug, .variance_debug = variance_debug, .upsample_debug = upsample_debug};
}

}  // namespace falcom_world::denoise
