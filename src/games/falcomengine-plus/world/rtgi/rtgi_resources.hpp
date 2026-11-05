#pragma once

// RTGI GPU resources: the trace pipeline (world_rtgi), the SVGF denoise chain (world_svgf_reproject, world_svgf_variance,
// world_svgf_atrous) and the upsample (world_rtgi_upsample), the stats buffer, the debug texture (r8g8b8a8_unorm at the depth
// size, created while RTGI Debug is 1 to 12), the raw GI, the history, the SVGF targets, the final GI and the radiance
// copy. Created lazily from the lighting hook (rtgi.hpp); no module mutex is held around any graphics call here.

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <span>
#include <unordered_map>

#include "../../../../utils/log.hpp"
#include "../rtao/rtao_resources.hpp"

namespace falcom_world::rtgi {

// t0..t8 BVH, t9 game depth, t10..t13 characters (null unless Trace characters), t14..t16 alpha,
// t17 MRT normal, t18 IS-FAST noise,
// t19 P2b records (the RTAO pass A table), t20 radiance: the previous frame's lighting output, t21 the previous final
// GI (null unless kRtgiFlagSubtractPrevious) and t22 the G-buffer colour (bound whenever it was captured this frame,
// read only under kRtgiFlagSubtractPrevious): the one-bounce correction.
// t23 the lamps (dynamicLights_g) and t24 the lamp type bits (kRtgiFlagDynamicLights; null unless the lamp type
// pass ran).
// Sora 2nd only (null otherwise), for the O1 sun and lamp estimate of on-screen bounce: t25 G-buffer mrtTexture1
// (sun diffuse factor), t26 the screen shadow map (shadowTexture), t27 G-buffer mrtTexture2 (material index) and
// t28 deferredParams_g (dynamicLightIntensity).
inline constexpr uint32_t kRtgiSrvCount = 29u;
// u0 debug texture (null unless RTGI Debug is 1 to 3, 8, 9, 10 or 12), u1 stats, u2 raw GI, u3 ReSTIR sample position and W,
// u4 ReSTIR sample data (null unless kRtgiFlagRestir)
inline constexpr uint32_t kRtgiUavCount = 5u;
inline constexpr uint32_t kRtgiPushConstantCount = 32u;
inline constexpr uint32_t kRtgiStatsCount = 28u;

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
inline constexpr uint32_t kRtgiFlagSunCastersOnly = 4u;  // b12 c2.z bit: sun rays hit shadow casters only
inline constexpr uint32_t kRtgiFlagFallbackNoAlbedo = 8u;  // b12 c2.z bit: fallback hits use albedo 1.0, not RTGI_FALLBACK_ALBEDO
inline constexpr uint32_t kRtgiFlagDynamicLights = 16u;  // b12 c2.z bit: fallback hits add the lamps of the type pass
inline constexpr uint32_t kRtgiFlagGameSun = 32u;  // b12 c2.z bit: fallback sun gets the game's shadow floor
inline constexpr uint32_t kRtgiFlagFeedback = 64u;  // b12 c2.z bit: add back the feedback share (c7.y) of last frame's bounce
inline constexpr uint32_t kRtgiFlagRealRays = 128u;  // b12 c2.z bit: Extra Bounces on, extra bounce rays from c7.z
inline constexpr uint32_t kRtgiFlagLampShadows = 256u;  // b12 c2.z bit: fallback lamp light gets a shadow ray per lamp
inline constexpr uint32_t kRtgiFlagRestir = 512u;  // b12 c2.z bit: ReSTIR GI initial sample (u3, u4) and the RIS estimate in raw
// Reprojection pass counters, matching RTGI_STAT_* in world_svgf_reproject.cs_5_0.hlsl.
inline constexpr uint32_t kRtgiStatTemporalPixels = 20u;
inline constexpr uint32_t kRtgiStatNoHistory = 21u;
inline constexpr uint32_t kRtgiStatResetPixels = 22u;
// ReSTIR GI temporal pass counters, matching RTGI_STAT_RESTIR_* in world_restir_temporal.cs_5_0.hlsl.
inline constexpr uint32_t kRtgiStatRestirPixels = 24u;       // pixels with an initial reservoir (M > 0)
inline constexpr uint32_t kRtgiStatRestirAccepted = 25u;     // of those, last frame's reservoir merged
inline constexpr uint32_t kRtgiStatRestirFromHistory = 26u;  // of those, the history sample was selected
inline constexpr uint32_t kRtgiStatRestirJacobianRejected = 27u;  // reprojection passed, dropped by the Jacobian or cosine guard
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
inline constexpr uint32_t kSvgfPushCount = 16u;
inline constexpr uint32_t kSvgfMaxIterations = 5u;
inline constexpr float kSvgfLuminanceRgb[3] = {0.2126f, 0.7152f, 0.0722f};  // RTGI and GTVBGI luminance weights
inline constexpr float kSvgfLuminanceShadow[3] = {1.f, 1.f, 0.f};          // RT Local Shadows and RT Sun
// GTVBAO and RTAO: scalar AO (rgb equal), l = r
inline constexpr float kSvgfLuminanceAo[3] = {1.f, 0.f, 0.f};
// Upsample: t0 depth, t1 MRT normal, t2 input, t3 G-buffer colour (Debug 7); u0 final GI, u1 debug.
inline constexpr uint32_t kRtgiUpsampleSrvCount = 4u;
inline constexpr uint32_t kRtgiUpsampleUavCount = 2u;
inline constexpr uint32_t kRtgiUpsamplePushCount = 12u;
// Sora 2nd lighting 0xCA3D8596: t11 dynamicLights_g (LightParam) and t12 lightIndices_g (LightIndexData). Copied for the
// stats request only (rtgi.hpp Dispatch) and read back into world_rtgi_lights.json.
inline constexpr uint32_t kLightingLightsRegister = 11u;
inline constexpr uint32_t kLightingLightIndicesRegister = 12u;
// Sora 2nd lighting 0xCA3D8596 (lighting lines 192, 193, 198, 199): t2 mrtTexture1, t3 mrtTexture2,
// t6 deferredParams_g, t7 shadowTexture. Bound to RTGI t25..t28 (kRtgiSrvCount) for the O1 on-screen sun and lamp
// estimate.
inline constexpr uint32_t kLightingMrt1Register = 2u;
inline constexpr uint32_t kLightingMrt2Register = 3u;
inline constexpr uint32_t kLightingDeferredRegister = 6u;
inline constexpr uint32_t kLightingShadowRegister = 7u;
inline constexpr uint32_t kLightParamBytes = 112u;
inline constexpr uint32_t kLightIndexDataBytes = 576u;
// Lamp type pass (world_rtgi_lights.cs_5_0.hlsl): t0 lightIndices_g, u0 one uint per lamp (bit 1 point, bit 2
// spot), b12 c0.x
// lamp count. The trace reads the lamps at t23 and the types at t24: kRtgiLampCount entries each.
inline constexpr uint32_t kRtgiLampTypeSrvCount = 1u;
inline constexpr uint32_t kRtgiLampTypeUavCount = 1u;
inline constexpr uint32_t kRtgiLampTypePushCount = 1u;
inline constexpr uint32_t kRtgiLampCount = 1024u;
inline constexpr uint32_t kRtgiLightTileCount = 4608u;  // cells of lightIndices_g: 64 per group in the type pass
// ReSTIR GI temporal pass (world_restir_temporal.cs_5_0.hlsl): t0 trace raw GI, t1 depth, t2 MRT normal, t3 motion,
// t4 initial sample x_s W, t5 initial sample data, t6..t8 last frame's temporal reservoir (sample, visible, data);
// u0 raw GI out, u1 stats (24..27), u2..u4 this frame's temporal reservoir, u5 debug; s0 point, s1 linear.
inline constexpr uint32_t kRestirTemporalSrvCount = 9u;
inline constexpr uint32_t kRestirTemporalUavCount = 6u;
inline constexpr uint32_t kRestirTemporalSamplerCount = 2u;
inline constexpr uint32_t kRestirTemporalPushCount = 16u;
inline constexpr float kRestirTemporalMCap = 30.f;  // ReSTIR GI Sec. 5: temporal M clamped to 30

#if defined(__world_rtgi_EMBED_FILE)
inline constexpr std::span<const std::uint8_t> kRtgiTraceShader = __world_rtgi;
#else
inline constexpr std::span<const std::uint8_t> kRtgiTraceShader = {};
#endif
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
#if defined(__world_rtgi_upsample_EMBED_FILE)
inline constexpr std::span<const std::uint8_t> kRtgiUpsampleShader = __world_rtgi_upsample;
#else
inline constexpr std::span<const std::uint8_t> kRtgiUpsampleShader = {};
#endif
#if defined(__world_rtgi_lights_EMBED_FILE)
inline constexpr std::span<const std::uint8_t> kRtgiLampTypeShader = __world_rtgi_lights;
#else
inline constexpr std::span<const std::uint8_t> kRtgiLampTypeShader = {};
#endif
#if defined(__world_restir_temporal_EMBED_FILE)
inline constexpr std::span<const std::uint8_t> kRestirTemporalShader = __world_restir_temporal;
#else
inline constexpr std::span<const std::uint8_t> kRestirTemporalShader = {};
#endif

// b12 for the RTGI trace, matching cbuffer cb_rtgi in world_rtgi.cs_5_0.hlsl (eight float4 registers).
struct RtgiPushConstants {
  float params[4];    // c0: effective ray max, normal bias, spp, trace divisor
  float sampling[4];  // c1: isfast flag (1 loaded), slice base, seed, debug mode
  float fade[4];      // c2: fade start_eff, fade end_eff, flags (kRtgiFlag* as a float), P2b record count (w, uint bits)
  float region[4];    // c3: region min xyz, region size
  float size[4];      // c4: trace width, trace height, full width, full height
  float shading[4];   // c5: ambient contribution, radiance clamp (0 = off), last frame's applied GI Intensity (one bounce), prevViewProj_g register (RtgiPrevViewProjRegister)
  float sun[4];       // c6: Sun Shadow Distance (m), character object count (t10-t13), Sun Contribution, lamp shadow clearance (m; 0 unless kRtgiFlagLampShadows)
  float extra[4];     // c7: x = Dynamic Light Contribution, y = effective Feedback Strength, z = Extra Bounce Rays (Extra Bounces only), w = Bounce Saturation (extra bounce light only)
};
static_assert(sizeof(RtgiPushConstants) == kRtgiPushConstantCount * sizeof(float), "b12 must be 32 floats");
static_assert(offsetof(RtgiPushConstants, sampling) == 4 * sizeof(float), "b12 c1 offset");
static_assert(offsetof(RtgiPushConstants, fade) == 8 * sizeof(float), "b12 c2 offset");
static_assert(offsetof(RtgiPushConstants, region) == 12 * sizeof(float), "b12 c3 offset");
static_assert(offsetof(RtgiPushConstants, size) == 16 * sizeof(float), "b12 c4 offset");
static_assert(offsetof(RtgiPushConstants, shading) == 20 * sizeof(float), "b12 c5 offset");
static_assert(offsetof(RtgiPushConstants, sun) == 24 * sizeof(float), "b12 c6 offset");

// b12 for the SVGF passes (reproject, variance, a-trous), matching cbuffer cb_svgf in world_svgf.hlsli.
struct SvgfPushConstants {
  float size[4];      // c0: trace width, trace height, full width, full height
  float params[4];    // c1: trace divisor, debug mode (RTGI numbering), history valid (0 = reset), a-trous step (2^i, 0 otherwise)
  float temporal[4];  // c2: alpha = 1 - History Weight, depth rejection, normal rejection, luminance edge stop sigma_l
  float luminance[4]; // c3: luminance weights rgb, unused
};
static_assert(sizeof(SvgfPushConstants) == kSvgfPushCount * sizeof(float), "svgf b12 must be 16 floats");
static_assert(offsetof(SvgfPushConstants, params) == 4 * sizeof(float), "svgf b12 c1 offset");
static_assert(offsetof(SvgfPushConstants, temporal) == 8 * sizeof(float), "svgf b12 c2 offset");
static_assert(offsetof(SvgfPushConstants, luminance) == 12 * sizeof(float), "svgf b12 c3 offset");

// b12 for the RTGI upsample, matching cbuffer cb_rtgi_upsample in world_rtgi_upsample.cs_5_0.hlsl.
struct RtgiUpsamplePushConstants {
  float size[4];      // c0: full width, full height, trace width, trace height
  float params[4];    // c1: trace divisor, debug mode, GI Intensity, Character GI Intensity (Debug 7)
  float extra[4];     // c2: GI Saturation, unused, unused, unused
};
static_assert(sizeof(RtgiUpsamplePushConstants) == kRtgiUpsamplePushCount * sizeof(float), "upsample b12 must be 12 floats");

// b12 for the ReSTIR GI temporal pass, matching cbuffer cb_restir_temporal in world_restir_temporal.cs_5_0.hlsl.
struct RestirTemporalPushConstants {
  float size[4];    // c0: trace width, trace height, full width, full height
  float params[4];  // c1: trace divisor, debug mode, history valid (1, 0 = reset), M cap
  float reject[4];  // c2: depth rejection, normal rejection, fade start_eff, fade end_eff
  float seed[4];    // c3: noise seed, unused, unused, unused
};
static_assert(sizeof(RestirTemporalPushConstants) == kRestirTemporalPushCount * sizeof(float), "restir temporal b12 must be 16 floats");

// Mirrors LightParam in lighting_0xCA3D8596.ps_5_0.hlsl (offsets 0 to 108).
struct LightParamCpu {
  float pos[3];
  float radius;
  float color[3];
  float radius_inv;
  float chara_color[3];
  float attenuation;
  float vec[3];
  float spot_angle_inv;
  float attenuation_angle;
  float specular_intensity;
  float specular_glossiness;
  float scatter_anisotropy;
  float scatter_color[3];
  float scatter_density;
  float translucency;
  int32_t shadowmap_index;
  float scatter_offset;
  float user_param;
};
static_assert(sizeof(LightParamCpu) == kLightParamBytes, "LightParam must be 112 bytes");

// Mirrors LightIndexData in lighting_0xCA3D8596.ps_5_0.hlsl (offsets 0 to 572).
struct LightIndexDataCpu {
  int32_t point_indices[63];
  uint32_t point_count;
  int32_t spot_indices[63];
  uint32_t spot_count;
  int32_t probe_indices[14];
  uint32_t probe_count;
  float tile_depth_inv;
};
static_assert(sizeof(LightIndexDataCpu) == kLightIndexDataBytes, "LightIndexData must be 576 bytes");

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

// The upsample pipeline: ensured by the trace owner right after its trace pipeline (RTGI and RTLS keep this order).
inline constexpr RtgiPipelineDesc kRtgiUpsamplePipelineDesc = {.srv_count = kRtgiUpsampleSrvCount, .uav_count = kRtgiUpsampleUavCount,
                                                              .sampler_count = 0u, .push_count = kRtgiUpsamplePushCount,
                                                              .code = kRtgiUpsampleShader, .name = "upsample"};

// Creates one RTGI-layout compute pipeline on first use (RtgiPipeline layout); a failure is latched and logged once.
inline bool EnsureRtgiPipeline(reshade::api::device* device, RtgiPipeline* p, const RtgiPipelineDesc& desc) {
  if (p->pipeline.handle != 0u) return true;
  if (p->failed) return false;
  const auto fail = [&](const char* stage) {
    renodx::utils::log::w("[world-rtgi] ", desc.name, " pipeline creation failed: ", stage);
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
  push_range.dx_register_index = rtao::kRtaoPushRegister;
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
  renodx::utils::log::i("[world-rtgi] ", desc.name, " pipeline ready");
  return true;
}

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

// The objects of the shared denoise chain (SVGF reproject, variance, a-trous, upsample); RTGI, RT Local Shadows, RT Sun
// and GTVBGI each own one.
struct RtgiDenoiseSet {
  RtgiPipeline reproject_pipeline;
  RtgiPipeline variance_pipeline;
  RtgiPipeline atrous_pipeline;
  RtgiPipeline atrous_tile_pipeline[kSvgfAtrousTileSteps];
  RtgiPipeline upsample_pipeline;
  reshade::api::sampler point_sampler = {0u};
  reshade::api::sampler linear_sampler = {0u};
  RtgiTextureSet<2> history_color;   // rgba16f, trace size: rgb colour, a = variance
  RtgiTextureSet<2> history_normal;  // r16g16_float, trace size: octahedral normal
  RtgiTextureSet<2> history_moments; // r32g32b32a32_float, trace size: mu1, mu2, h (history length), 0
  RtgiTextureSet<2> history_guide;   // r32g32b32a32_float, trace size: z, dz/dx, dz/dy, dist (z = 0: no surface)
  uint32_t history_index = 0u;       // history[history_index] is read by the reproject pass; the other one is written
  RtgiTextureSet<1> integrated;      // rgba16f, trace size: integrated colour C' and sigma'^2 (a = -1: not traced)
  RtgiTextureSet<2> atrous;          // rgba16f, trace size: a-trous ping-pong
  RtgiTextureSet<2> output;          // full size, the owner's format (RTGI r11g11b10_float, RTLS r8g8b8a8_unorm)
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
};

// Effective a-trous iterations: clamp(iterations, 1, 5) - log2(divisor), at least 1.
inline uint32_t SvgfEffectiveIterations(float iterations, uint32_t divisor) {
  const int requested = std::clamp(static_cast<int>(iterations + 0.5f), 1, static_cast<int>(kSvgfMaxIterations));
  const int shift = static_cast<int>(std::bit_width(std::max(divisor, 1u))) - 1;
  return static_cast<uint32_t>(std::max(1, requested - shift));
}

// Bytes of the denoise textures: the trace-size sets, and the full-size output (4 bytes per pixel, two sets).
inline uint64_t RtgiDenoiseTextureBytes(const RtgiDenoiseSet& set) {
  const auto bytes = [](const auto& s, uint32_t bpp) { return static_cast<uint64_t>(s.width) * s.height * bpp; };
  return bytes(set.history_color, 8u * 2u) + bytes(set.history_normal, 4u * 2u) + bytes(set.history_moments, 16u * 2u)
      + bytes(set.history_guide, 16u * 2u) + bytes(set.integrated, 8u) + bytes(set.atrous, 8u * 2u)
      + bytes(set.output, 4u * 2u);
}

struct RtgiDeviceData {
  RtgiPipeline trace_pipeline;
  RtgiPipeline lamp_type_pipeline;
  reshade::api::resource stats_buffer = {0u};
  reshade::api::resource_view stats_uav = {0u};
  reshade::api::resource lamp_types = {0u};  // one uint per lamp: bit 1 point, bit 2 spot (kRtgiLampCount)
  reshade::api::resource_view lamp_types_uav = {0u};
  reshade::api::resource_view lamp_types_srv = {0u};
  RtgiTextureSet<1> debug;  // r8g8b8a8_unorm, depth size; created while RTGI Debug is 1 to 10
  RtgiTextureSet<1> raw;    // rgba16f, trace size: rgb GI, a = 1 traced, -1 not traced
  RtgiTextureSet<1> restir_sample_pos;   // r32g32b32a32_float, trace size: x_s, W (only while kRtgiFlagRestir)
  RtgiTextureSet<1> restir_sample_data;  // r32g32b32a32_uint, trace size: n_s, L_o, M, flags (only while kRtgiFlagRestir)
  RtgiTextureSet<1> restir_raw;          // rgba16f, trace size: raw GI after ReSTIR temporal reuse (the chain's input)
  RtgiTextureSet<2> restir_res_sample;   // r32g32b32a32_float, trace size: temporal reservoir x_s, W
  RtgiTextureSet<2> restir_res_visible;  // r32g32b32a32_float, trace size: temporal reservoir x_v, 0
  RtgiTextureSet<2> restir_res_data;     // r32g32b32a32_uint, trace size: n_s, L_o rg, L_o b | M << 16, n_v
  uint32_t restir_res_index = 0u;        // reservoir[restir_res_index] is the newest (read next frame)
  bool restir_res_valid = false;         // reservoir[restir_res_index] holds the previous frame's temporal reservoirs
  RtgiPipeline restir_temporal_pipeline;
  bvh::GpuTimer restir_timer = {};
  RtgiDenoiseSet denoise;   // output: r11g11b10_float, the upsampled GI
  reshade::api::resource radiance_texture = {0u};
  reshade::api::resource_view radiance_srv = {0u};
  uint32_t radiance_width = 0u;
  uint32_t radiance_height = 0u;
  reshade::api::format radiance_format = reshade::api::format::unknown;
  uint64_t radiance_frame = UINT64_MAX;
  bool radiance_failed = false;
  reshade::api::resource lights_copy = {0u};  // Sora 2nd dynamicLights_g copy (stats request only)
  uint64_t lights_copy_bytes = 0u;
  reshade::api::resource light_indices_copy = {0u};  // Sora 2nd lightIndices_g copy (stats request only)
  uint64_t light_indices_copy_bytes = 0u;
  bool lights_copied = false;  // the last dispatch copied both light buffers
  bvh::GpuTimer timer = {};
};

// One entry per device (destroyed with the device).
inline std::unordered_map<reshade::api::device*, RtgiDeviceData> g_rtgi_devices;

inline RtgiDeviceData& GetRtgiDeviceData(reshade::api::device* device) { return g_rtgi_devices[device]; }

// Destroys one RTGI-layout compute pipeline and its descriptor tables.
inline void DestroyRtgiPipeline(reshade::api::device* device, const RtgiPipeline& p) {
  if (p.pipeline.handle != 0u) device->destroy_pipeline(p.pipeline);
  if (p.srv_table.handle != 0u) device->free_descriptor_table(p.srv_table);
  if (p.uav_table.handle != 0u) device->free_descriptor_table(p.uav_table);
  if (p.cbv_table.handle != 0u) device->free_descriptor_table(p.cbv_table);
  if (p.sampler_table.handle != 0u) device->free_descriptor_table(p.sampler_table);
  if (p.layout.handle != 0u) device->destroy_pipeline_layout(p.layout);
}

// Destroys the denoise textures and the history; the pipelines, samplers and timer stay. The next run recreates them.
inline void ReleaseRtgiDenoiseTextures(reshade::api::device* device, RtgiDenoiseSet* set) {
  DestroyRtgiTextureSet(device, &set->history_color);
  DestroyRtgiTextureSet(device, &set->history_normal);
  DestroyRtgiTextureSet(device, &set->history_moments);
  DestroyRtgiTextureSet(device, &set->history_guide);
  DestroyRtgiTextureSet(device, &set->integrated);
  DestroyRtgiTextureSet(device, &set->atrous);
  DestroyRtgiTextureSet(device, &set->output);
  set->history_index = 0u;
  set->output_index = 0u;
  set->atrous_iterations = 0u;
}

// Destroys a denoise set (DestroyRtgiDeviceData and DestroyRtlsDeviceData).
inline void DestroyRtgiDenoiseSet(reshade::api::device* device, RtgiDenoiseSet* set) {
  ReleaseRtgiDenoiseTextures(device, set);
  bvh::DestroyGpuTimer(&set->timer);
  if (set->point_sampler.handle != 0u) device->destroy_sampler(set->point_sampler);
  if (set->linear_sampler.handle != 0u) device->destroy_sampler(set->linear_sampler);
  DestroyRtgiPipeline(device, set->reproject_pipeline);
  DestroyRtgiPipeline(device, set->variance_pipeline);
  DestroyRtgiPipeline(device, set->atrous_pipeline);
  for (RtgiPipeline& tile : set->atrous_tile_pipeline) DestroyRtgiPipeline(device, tile);
  DestroyRtgiPipeline(device, set->upsample_pipeline);
}

inline void DestroyRtgiDeviceData(reshade::api::device* device) {
  const auto found = g_rtgi_devices.find(device);
  if (found == g_rtgi_devices.end()) return;
  RtgiDeviceData& data = found->second;
  DestroyRtgiTextureSet(device, &data.debug);
  DestroyRtgiTextureSet(device, &data.raw);
  DestroyRtgiTextureSet(device, &data.restir_sample_pos);
  DestroyRtgiTextureSet(device, &data.restir_sample_data);
  DestroyRtgiTextureSet(device, &data.restir_raw);
  DestroyRtgiTextureSet(device, &data.restir_res_sample);
  DestroyRtgiTextureSet(device, &data.restir_res_visible);
  DestroyRtgiTextureSet(device, &data.restir_res_data);
  DestroyRtgiDenoiseSet(device, &data.denoise);
  bvh::DestroyBuffer(device, &data.radiance_srv, &data.radiance_texture);
  bvh::DestroyBuffer(device, &data.stats_uav, &data.stats_buffer);
  bvh::DestroyBuffer(device, &data.lamp_types_srv, nullptr);
  bvh::DestroyBuffer(device, &data.lamp_types_uav, &data.lamp_types);
  bvh::DestroyBuffer(device, nullptr, &data.lights_copy);
  bvh::DestroyBuffer(device, nullptr, &data.light_indices_copy);
  bvh::DestroyGpuTimer(&data.timer);
  bvh::DestroyGpuTimer(&data.restir_timer);
  DestroyRtgiPipeline(device, data.trace_pipeline);
  DestroyRtgiPipeline(device, data.lamp_type_pipeline);
  DestroyRtgiPipeline(device, data.restir_temporal_pipeline);
  g_rtgi_devices.erase(found);
}

// Releases the RTGI textures when RTGI is switched off; the pipelines, samplers, stats buffer and timers stay.
inline void ReleaseRtgiTextures(reshade::api::device* device) {
  const auto found = g_rtgi_devices.find(device);
  if (found == g_rtgi_devices.end()) return;
  RtgiDeviceData& data = found->second;
  DestroyRtgiTextureSet(device, &data.debug);
  DestroyRtgiTextureSet(device, &data.raw);
  DestroyRtgiTextureSet(device, &data.restir_sample_pos);
  DestroyRtgiTextureSet(device, &data.restir_sample_data);
  DestroyRtgiTextureSet(device, &data.restir_raw);
  DestroyRtgiTextureSet(device, &data.restir_res_sample);
  DestroyRtgiTextureSet(device, &data.restir_res_visible);
  DestroyRtgiTextureSet(device, &data.restir_res_data);
  data.restir_res_index = 0u;
  data.restir_res_valid = false;
  ReleaseRtgiDenoiseTextures(device, &data.denoise);
  bvh::DestroyBuffer(device, &data.radiance_srv, &data.radiance_texture);
  data.radiance_width = 0u;
  data.radiance_height = 0u;
  data.radiance_format = reshade::api::format::unknown;
  data.radiance_frame = UINT64_MAX;
  data.radiance_failed = false;
}

}  // namespace falcom_world::rtgi
