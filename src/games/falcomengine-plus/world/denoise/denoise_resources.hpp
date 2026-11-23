#pragma once

// Shared GPU resources for the SVGF denoise chain (world_svgf_reproject, world_svgf_variance, world_svgf_atrous) and the
// upsample (world_svgf_upsample), plus the pipeline, texture-set and light-buffer helpers used by the surviving effects.
// Created lazily by the owners; no module mutex is held around any graphics call here.

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <span>
#include <unordered_map>

#include "../../../../utils/log.hpp"
#include "../bvh/bvh_resources.hpp"
#include "../compute/compute_resources.hpp"

namespace falcom_world::denoise {

inline constexpr uint32_t kSvgfStatsCount = 32u;

// Reprojection pass counters, matching SVGF_STAT_* in world_svgf_reproject.cs_5_0.hlsl.
inline constexpr uint32_t kSvgfStatTemporalPixels = 20u;
inline constexpr uint32_t kSvgfStatNoHistory = 21u;
inline constexpr uint32_t kSvgfStatResetPixels = 22u;
// SVGF (world_svgf.hlsli). Reproject: t0 raw GI, t1 depth, t2 MRT normal, t3 motion, t4 history colour, t5 history
// moments, t6 history guide, t7 history normal; u0 colour write (or integrated), u1 stats, u2 moments write, u3 guide
// write, u4 normal write, u5 debug; s0 point, s1 linear.
inline constexpr uint32_t kSvgfReprojectSrvCount = 8u;
inline constexpr uint32_t kSvgfReprojectUavCount = 6u;
inline constexpr uint32_t kSvgfReprojectSamplerCount = 2u;
// Variance: t0 integrated, t1 moments, t2 guide, t3 normal; u0 atrous[0], u1 debug.
inline constexpr uint32_t kSvgfVarianceSrvCount = 4u;
inline constexpr uint32_t kSvgfVarianceUavCount = 2u;
// A-trous: t0 input, t1 guide, t2 normal; u0 output.
inline constexpr uint32_t kSvgfAtrousSrvCount = 3u;
inline constexpr uint32_t kSvgfAtrousUavCount = 1u;
// A-trous iterations 0 to 2 (steps 1, 2, 4) use the group-shared tile kernels world_svgf_atrous_s1, _s2 and _s4.
inline constexpr uint32_t kSvgfAtrousTileSteps = 3u;
inline constexpr uint32_t kSvgfPushCount = 20u;
inline constexpr uint32_t kSvgfMaxIterations = 5u;
inline constexpr float kSvgfLuminanceRgb[3] = {0.2126f, 0.7152f, 0.0722f};  // GTVBGI and the shared denoise chain luminance weights
inline constexpr float kSvgfLuminanceShadow[3] = {1.f, 1.f, 0.f};          // legacy shadow-chain luminance weights
// GTVBAO: scalar AO (rgb equal), l = r
inline constexpr float kSvgfLuminanceAo[3] = {1.f, 0.f, 0.f};
// Upsample: t0 depth, t1 MRT normal, t2 input, t3 G-buffer colour (Debug 7); u0 final GI, u1 debug.
inline constexpr uint32_t kUpsampleSrvCount = 4u;
inline constexpr uint32_t kUpsampleUavCount = 2u;
inline constexpr uint32_t kUpsamplePushCount = 16u;
#if defined(__world_svgf_reproject_EMBED_FILE)
inline constexpr std::span<const std::uint8_t> kSvgfReprojectShader = __world_svgf_reproject;
#else
inline constexpr std::span<const std::uint8_t> kSvgfReprojectShader = {};
#endif
#if defined(__world_svgf_variance_EMBED_FILE)
inline constexpr std::span<const std::uint8_t> kSvgfVarianceShader = __world_svgf_variance;
#else
inline constexpr std::span<const std::uint8_t> kSvgfVarianceShader = {};
#endif
#if defined(__world_svgf_atrous_EMBED_FILE)
inline constexpr std::span<const std::uint8_t> kSvgfAtrousShader = __world_svgf_atrous;
#else
inline constexpr std::span<const std::uint8_t> kSvgfAtrousShader = {};
#endif
#if defined(__world_svgf_atrous_s1_EMBED_FILE)
inline constexpr std::span<const std::uint8_t> kSvgfAtrousS1Shader = __world_svgf_atrous_s1;
#else
inline constexpr std::span<const std::uint8_t> kSvgfAtrousS1Shader = {};
#endif
#if defined(__world_svgf_atrous_s2_EMBED_FILE)
inline constexpr std::span<const std::uint8_t> kSvgfAtrousS2Shader = __world_svgf_atrous_s2;
#else
inline constexpr std::span<const std::uint8_t> kSvgfAtrousS2Shader = {};
#endif
#if defined(__world_svgf_atrous_s4_EMBED_FILE)
inline constexpr std::span<const std::uint8_t> kSvgfAtrousS4Shader = __world_svgf_atrous_s4;
#else
inline constexpr std::span<const std::uint8_t> kSvgfAtrousS4Shader = {};
#endif
#if defined(__world_svgf_reconstruct_EMBED_FILE)
inline constexpr std::span<const std::uint8_t> kSvgfReconstructShader = __world_svgf_reconstruct;
#else
inline constexpr std::span<const std::uint8_t> kSvgfReconstructShader = {};
#endif
#if defined(__world_svgf_reconstruct_fallback_EMBED_FILE)
inline constexpr std::span<const std::uint8_t> kSvgfReconstructFallbackShader = __world_svgf_reconstruct_fallback;
#else
inline constexpr std::span<const std::uint8_t> kSvgfReconstructFallbackShader = {};
#endif
#if defined(__world_svgf_finalize_EMBED_FILE)
inline constexpr std::span<const std::uint8_t> kSvgfFinalizeShader = __world_svgf_finalize;
#else
inline constexpr std::span<const std::uint8_t> kSvgfFinalizeShader = {};
#endif
#if defined(__world_svgf_finalize_ao_EMBED_FILE)
inline constexpr std::span<const std::uint8_t> kSvgfFinalizeAoShader = __world_svgf_finalize_ao;
#else
inline constexpr std::span<const std::uint8_t> kSvgfFinalizeAoShader = {};
#endif
// b12 for the SVGF passes (reproject, variance, a-trous), matching cbuffer cb_svgf in world_svgf.hlsli.
struct SvgfPushConstants {
  float size[4];      // c0: trace width, trace height, full width, full height
  float params[4];    // c1: trace divisor, debug mode (SVGF numbering), history valid (0 = reset), a-trous step (2^i, 0 otherwise)
  float temporal[4];  // c2: alpha = 1 - History Weight, depth rejection, normal rejection, luminance edge stop sigma_l
  float luminance[4]; // c3: luminance weights rgb, unused
  float spatial[4];    // c4: depth sigma, normal power, history length cap, radius scale
};
static_assert(sizeof(SvgfPushConstants) == kSvgfPushCount * sizeof(float), "svgf b12 must be 20 floats");
static_assert(offsetof(SvgfPushConstants, params) == 4 * sizeof(float), "svgf b12 c1 offset");
static_assert(offsetof(SvgfPushConstants, temporal) == 8 * sizeof(float), "svgf b12 c2 offset");
static_assert(offsetof(SvgfPushConstants, luminance) == 12 * sizeof(float), "svgf b12 c3 offset");
static_assert(offsetof(SvgfPushConstants, spatial) == 16 * sizeof(float), "svgf b12 c4 offset");

// b12 for the shared upsample, matching cbuffer cb_svgf_upsample in world_svgf_upsample.cs_5_0.hlsl.
struct UpsamplePushConstants {
  float size[4];     // c0: full width, full height, trace width, trace height
  float params[4];   // c1: trace divisor, debug mode, GI Intensity, Character GI Intensity (Debug 7)
  float extra[4];    // c2: GI Saturation, algorithm, depth-plane sigma, normal edge power
  float upscale[4];  // c3: bilinear weight sharpness, nearest-valid fallback, unused, unused
};
static_assert(sizeof(UpsamplePushConstants) == kUpsamplePushCount * sizeof(float), "upsample b12 must be 16 floats");

// One compute pipeline with the shared layout: cbv (0), srv (1), uav (2), an optional sampler table (3), then the push
// constants at b12 (param 3, or 4 with samplers).
struct ComputePipeline {
  reshade::api::pipeline_layout layout = {0u};
  reshade::api::descriptor_table cbv_table = {0u};
  reshade::api::descriptor_table srv_table = {0u};
  reshade::api::descriptor_table uav_table = {0u};
  reshade::api::descriptor_table sampler_table = {0u};
  reshade::api::pipeline pipeline = {0u};
  bool failed = false;  // creation failed: latched for the session, logged once
};

struct ComputePipelineDesc {
  uint32_t srv_count;
  uint32_t uav_count;
  uint32_t sampler_count;  // 0: no sampler table
  uint32_t push_count;
  std::span<const std::uint8_t> code;  // empty: the shader is not embedded
  const char* name;                    // log name
};

// Creates one compute pipeline on first use (ComputePipeline layout); a failure is latched and logged once.
inline bool EnsureComputePipeline(reshade::api::device* device, ComputePipeline* p, const ComputePipelineDesc& desc) {
  if (p->pipeline.handle != 0u) return true;
  if (p->failed) return false;
  const auto fail = [&](const char* stage) {
    renodx::utils::log::w("[world-denoise] ", desc.name, " pipeline creation failed: ", stage);
    p->failed = true;
    return false;
  };
  if (desc.code.empty()) return fail("shader not embedded");
  using DR = reshade::api::descriptor_range;
  using DS = reshade::api::shader_stage;
  using DT = reshade::api::descriptor_type;
  using P = reshade::api::pipeline_layout_param;
  DR cbv_range = {0, 0, 0, 1, DS::all_compute, 1, DT::constant_buffer};
  DR srv_range = {0, 0, 0, desc.srv_count, DS::all_compute, 1, DT::shader_resource_view};
  DR uav_range = {0, 0, 0, desc.uav_count, DS::all_compute, 1, DT::unordered_access_view};
  DR sampler_range = {0, 0, 0, desc.sampler_count, DS::all_compute, 1, DT::sampler};
  reshade::api::constant_range push_range = {};
  push_range.binding = 0;
  push_range.dx_register_index = compute::kPassPushRegister;
  push_range.dx_register_space = 0;
  push_range.count = desc.push_count;
  push_range.visibility = DS::all_compute;
  const uint32_t push_index = desc.sampler_count > 0u ? 4u : 3u;
  P params[5] = {};
  params[0].type = reshade::api::pipeline_layout_param_type::descriptor_table;
  params[0].descriptor_table.count = 1;
  params[0].descriptor_table.ranges = &cbv_range;
  params[1].type = reshade::api::pipeline_layout_param_type::descriptor_table;
  params[1].descriptor_table.count = 1;
  params[1].descriptor_table.ranges = &srv_range;
  params[2].type = reshade::api::pipeline_layout_param_type::descriptor_table;
  params[2].descriptor_table.count = 1;
  params[2].descriptor_table.ranges = &uav_range;
  if (desc.sampler_count > 0u) {
    params[3].type = reshade::api::pipeline_layout_param_type::descriptor_table;
    params[3].descriptor_table.count = 1;
    params[3].descriptor_table.ranges = &sampler_range;
  }
  params[push_index].type = reshade::api::pipeline_layout_param_type::push_constants;
  params[push_index].push_constants = push_range;
  if (p->layout.handle == 0u && !device->create_pipeline_layout(push_index + 1u, params, &p->layout)) return fail("pipeline layout");
  if (p->cbv_table.handle == 0u && !device->allocate_descriptor_table(p->layout, 0, &p->cbv_table)) return fail("cbv table");
  if (p->srv_table.handle == 0u && !device->allocate_descriptor_table(p->layout, 1, &p->srv_table)) return fail("srv table");
  if (p->uav_table.handle == 0u && !device->allocate_descriptor_table(p->layout, 2, &p->uav_table)) return fail("uav table");
  if (desc.sampler_count > 0u && p->sampler_table.handle == 0u
      && !device->allocate_descriptor_table(p->layout, 3, &p->sampler_table)) {
    return fail("sampler table");
  }
  reshade::api::shader_desc shader = {};
  shader.code = desc.code.data();
  shader.code_size = desc.code.size();
  shader.entry_point = "main";
  reshade::api::pipeline_subobject subobject = {reshade::api::pipeline_subobject_type::compute_shader, 1, &shader};
  if (!device->create_pipeline(p->layout, 1, &subobject, &p->pipeline)) return fail("compute pipeline");
  renodx::utils::log::i("[world-denoise] ", desc.name, " pipeline ready");
  return true;
}

// Count textures of one format and size, each with an SRV and a UAV, zero-initialised (compute::CreateZeroedTexture).
template <size_t Count>
struct TextureSet {
  reshade::api::resource texture[Count] = {};
  reshade::api::resource_view srv[Count] = {};
  reshade::api::resource_view uav[Count] = {};
  uint32_t width = 0u;
  uint32_t height = 0u;
};

template <size_t Count>
inline void DestroyTextureSet(reshade::api::device* device, TextureSet<Count>* set) {
  for (size_t i = 0; i < Count; ++i) {
    bvh::DestroyBuffer(device, &set->srv[i], &set->texture[i]);
    bvh::DestroyBuffer(device, &set->uav[i], nullptr);
  }
  set->width = 0u;
  set->height = 0u;
}

// Creates the set at width x height; a size change replaces it. On a failure the set is destroyed and false returned.
template <size_t Count>
inline bool EnsureTextureSet(reshade::api::device* device, TextureSet<Count>* set, reshade::api::format format,
                                 uint32_t bytes_per_pixel, uint32_t width, uint32_t height) {
  if (set->texture[0].handle != 0u && set->width == width && set->height == height) return true;
  DestroyTextureSet(device, set);
  for (size_t i = 0; i < Count; ++i) {
    if (!compute::CreateZeroedTexture(device, format, width, height, bytes_per_pixel, &set->texture[i], &set->srv[i], &set->uav[i])) {
      DestroyTextureSet(device, set);
      return false;
    }
  }
  set->width = width;
  set->height = height;
  return true;
}

// The objects of the shared denoise chain (SVGF reproject, variance, a-trous, upsample); each owner keeps one.
struct DenoiseSet {
  ComputePipeline reproject_pipeline;
  ComputePipeline variance_pipeline;
  ComputePipeline atrous_pipeline;
  ComputePipeline atrous_tile_pipeline[kSvgfAtrousTileSteps];
  ComputePipeline upsample_pipeline;
  ComputePipeline reconstruct_pipeline;
  ComputePipeline finalize_pipeline;
  TextureSet<1> reconstructed;
  bool full_resolution = false;
  bool mode_valid = false;
  uint32_t radius_scale = 1u;
  reshade::api::sampler point_sampler = {0u};
  reshade::api::sampler linear_sampler = {0u};
  TextureSet<2> history_color;   // rgba16f, trace size: rgb colour, a = variance
  TextureSet<2> history_normal;  // r16g16_float, trace size: octahedral normal
  TextureSet<2> history_moments; // r32g32b32a32_float, trace size: mu1, mu2, h (history length), 0
  TextureSet<2> history_guide;   // r32g32b32a32_float, trace size: z, dz/dx, dz/dy, dist (z = 0: no surface)
  uint32_t history_index = 0u;       // history[history_index] is read by the reproject pass; the other one is written
  TextureSet<1> integrated;      // rgba16f, trace size: integrated colour C' and sigma'^2 (a = -1: not traced)
  TextureSet<2> atrous;          // rgba16f, trace size: a-trous ping-pong
  TextureSet<2> output;          // full size, the owner's output format
  uint32_t output_index = 0u;        // output[output_index] is the newest
  uint32_t atrous_iterations = 0u;   // effective a-trous iterations of the last run
  bvh::GpuTimer timer = {};          // reproject, variance, a-trous and upsample
};

// Per-owner SVGF settings for one dispatch (the values fed to the passes).
struct SvgfSettings {
  float history_weight = 0.8f;      // alpha = 1 - history_weight
  float depth_rejection = 0.2f;
  float normal_rejection = 0.f;
  bool history_valid = false;       // false: reset (ResetReason != None)
  float iterations = 5.f;           // requested a-trous iterations (1 to 5)
  float luminance_edge_stop = 4.f;  // sigma_l
  float depth_sigma = 1.f;
  float normal_power = 128.f;
  bool full_resolution = false;
  float history_length = 32.f;
  float radius_scale = 1.f;
};

// Effective a-trous iterations: clamp(iterations, 1, 5) - log2(divisor), at least 1.
inline uint32_t SvgfEffectiveIterations(float iterations, uint32_t divisor) {
  const int requested = std::clamp(static_cast<int>(iterations + 0.5f), 1, static_cast<int>(kSvgfMaxIterations));
  const int shift = static_cast<int>(std::bit_width(std::max(divisor, 1u))) - 1;
  return static_cast<uint32_t>(std::max(1, requested - shift));
}

// Bytes of the denoise textures: the trace-size sets, and the full-size output (4 bytes per pixel, two sets).
inline uint64_t DenoiseTextureBytes(const DenoiseSet& set) {
  const auto bytes = [](const auto& s, uint32_t bpp) { return static_cast<uint64_t>(s.width) * s.height * bpp; };
  return bytes(set.history_color, 8u * 2u) + bytes(set.history_normal, 4u * 2u) + bytes(set.history_moments, 16u * 2u)
      + bytes(set.history_guide, 16u * 2u) + bytes(set.integrated, 8u) + bytes(set.atrous, 8u * 2u)
      + bytes(set.reconstructed, 8u)
      + bytes(set.output, 4u * 2u);
}

// Destroys one compute pipeline and its descriptor tables.
inline void DestroyComputePipeline(reshade::api::device* device, const ComputePipeline& p) {
  if (p.pipeline.handle != 0u) device->destroy_pipeline(p.pipeline);
  if (p.srv_table.handle != 0u) device->free_descriptor_table(p.srv_table);
  if (p.uav_table.handle != 0u) device->free_descriptor_table(p.uav_table);
  if (p.cbv_table.handle != 0u) device->free_descriptor_table(p.cbv_table);
  if (p.sampler_table.handle != 0u) device->free_descriptor_table(p.sampler_table);
  if (p.layout.handle != 0u) device->destroy_pipeline_layout(p.layout);
}

// Destroys the denoise textures and the history; the pipelines, samplers and timer stay. The next run recreates them.
inline void ReleaseDenoiseTextures(reshade::api::device* device, DenoiseSet* set) {
  DestroyTextureSet(device, &set->reconstructed);
  set->mode_valid = false;
  DestroyTextureSet(device, &set->history_color);
  DestroyTextureSet(device, &set->history_normal);
  DestroyTextureSet(device, &set->history_moments);
  DestroyTextureSet(device, &set->history_guide);
  DestroyTextureSet(device, &set->integrated);
  DestroyTextureSet(device, &set->atrous);
  DestroyTextureSet(device, &set->output);
  set->history_index = 0u;
  set->output_index = 0u;
  set->atrous_iterations = 0u;
}

// Destroys a denoise set owned by an effect.
inline void DestroyDenoiseSet(reshade::api::device* device, DenoiseSet* set) {
  ReleaseDenoiseTextures(device, set);
  bvh::DestroyGpuTimer(&set->timer);
  if (set->point_sampler.handle != 0u) device->destroy_sampler(set->point_sampler);
  if (set->linear_sampler.handle != 0u) device->destroy_sampler(set->linear_sampler);
  DestroyComputePipeline(device, set->reproject_pipeline);
  DestroyComputePipeline(device, set->variance_pipeline);
  DestroyComputePipeline(device, set->atrous_pipeline);
  for (ComputePipeline& tile : set->atrous_tile_pipeline) DestroyComputePipeline(device, tile);
  DestroyComputePipeline(device, set->upsample_pipeline);
  DestroyComputePipeline(device, set->reconstruct_pipeline);
  DestroyComputePipeline(device, set->finalize_pipeline);
}

}  // namespace falcom_world::denoise
