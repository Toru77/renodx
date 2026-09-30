/*
 * Copyright (C) 2026
 * SPDX-License-Identifier: MIT
 */

#define ImTextureID ImU64

// DynCube uses its own gated 1/sec log via dynCube_debug_logging instead.

#include <deps/imgui/imgui.h>
#include <include/reshade.hpp>

#include <embed/shaders.h>

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <map>
#include <mutex>
#include <shared_mutex>
#include <sstream>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <Windows.h>

#include "../../mods/shader.hpp"
#include "../../utils/descriptor.hpp"
#include "../../utils/pipeline_layout.hpp"
#include "../../utils/settings.hpp"
#include "../../utils/shader.hpp"
#include "../../utils/state.hpp"
#include "../../utils/dlss_hook.hpp"
#include "./shared.h"
#include "./fast_noise_ea.h"  // baked-in fast_noise_ea.dds (embed_file.exe output)

namespace {

ShaderInjectData shader_injection = {
    .mod_enabled = 1.f,
    .slider_1 = 50.f,
    .slider_2 = 50.f,
    .slider_3 = 0.f,
    .volfog_haze_aa_mode = 0.f,
    .volfog_isfast_enabled = 0.f,
    .volfog_isfast_texture_loaded = 0.f,
    .volfog_jitter_enabled = 0.f,
    .volfog_jitter_amount = 0.5f,
    .volfog_jitter_speed = 237.f,
    .volfog_isfast_spatial_scale = 1.f,
    .volfog_noise_strength = 1.f,
    .volfog_isfast_dedicated_sampler = 0.f,
  // 0 = off, 1 = the engine's own character shadow march. Default off, matching
  // the CharShadowMode setting: the contact/micro terms own character shadowing
  // unless the user deliberately turns the engine's own back on.
  .char_shadow_mode = 0.f,
  .char_shadow_sample_count = 32.f,
  .char_shadow_hard_shadow_samples = 4.f,
  .char_shadow_fade_out_samples = 16.f,
  .char_shadow_surface_thickness = 0.09f,
  .char_shadow_contrast = 9.f,
  .char_shadow_light_screen_fade_start = 0.f,
  .char_shadow_light_screen_fade_end = 0.f,
  .char_shadow_min_occluder_depth_scale = 0.f,
  .char_shadow_jitter_enabled = 1.f,
  .char_shadow_type = 1.f,
  .char_shadow_camera_strength = 1.f,
  .char_shadow_world_strength = 1.f,
  .env_sss_enabled = 1.f,
  .env_sss_strength = 1.0f,
  .env_sss_sample_count = 24.f,
  .env_sss_hard_shadow_samples = 0.f,
  .env_sss_fade_out_samples = 0.f,
  .env_sss_surface_thickness = 0.005f,
  .env_sss_contrast = 2.f,
  .env_sss_jitter_enabled = 1.f,
  .env_sss_height_enabled = 1.f,
  .env_sss_height_min = 0.f,
  .env_sss_height_max = 1.f,
  .env_sss_height_fade = 0.1f,
  .env_sss_vertical_reject = 0.3f,
  .env_sss_max_darkening = 0.40f,
  .env_sss_bright_reject_threshold = 0.19f,
  .env_sss_bright_reject_fade = 0.5f,
  .env_sss_csm_gate = 0.f,
  .debug_show_env_sss = 0.f,
  .local_sss_enabled = 0.f,
  .local_sss_strength = 1.f,
  .local_sss_light_type = 2.f,
  .local_sss_sample_count = 24.f,
  .local_sss_hard_shadow_samples = 0.f,
  .local_sss_fade_out_samples = 0.f,
  .local_sss_surface_thickness = 0.005f,
  .local_sss_contrast = 2.f,
  .local_sss_light_fade_start = 0.f,
  .local_sss_light_fade_end = 1.f,
  .local_sss_occluder_depth_scale = 0.f,
  .gtvbao_mode = 1.f,
  .gtvbao_quality_level = 2.f,
  .gtvbao_denoise_passes = 1.f,
  .gtvbao_radius = 0.5f,
  .gtvbao_falloff_range = 0.615f,
  .gtvbao_radius_multiplier = 1.5f,
  .gtvbao_final_power = 2.0f,
  .gtvbao_sample_distribution = 1.5f,
  .gtvbao_bitmask_thickness = 0.2f,
  .gtvbao_depth_mip_offset = 3.30f,
  .gtvbao_denoise_blur_beta = 20.0f,
  .gtvbao_denoise_leak_threshold = 2.5f,
  .gtvbao_denoise_leak_strength = 0.5f,
  .gtvbao_temporal_blend = 0.f,
  .gtvbao_disocclusion_threshold = 0.01f,
  .gtvbao_debug_view = 0.f,
  .gtvbao_debug_logging = 0.f,
  .gtvbao_dedicated_bound = 0.f,
  .gtvbao_fix_experimental = 0.f,
  .gtvbao_vbgi_bound = 0.f,
  .gtvbao_vbgi_debug = 0.f,
  .vbgi_enabled = 0.f,
  .vbgi_intensity = 1.0f,
  .vbgi_saturation = 1.0f,
  .vbgi_char_mask_strength = 0.f,
  .vbgi_multibounce = 0.f,
  .vbgi_multibounce_strength = 1.f,
  .vbgi_multibounce_bounce_fraction = 0.15f,
  .vbgi_multibounce_saturation = 1.f,
  .vbgi_multibounce_max_clamp = 0.f,
  .vbgi_adaptive_r = 0.f,
  .vbgi_adaptive_g = 0.f,
  .vbgi_adaptive_b = 0.f,
  .vbgi_adaptive_mode = 0.f,
  .vbgi_adaptive_luma_strength = 0.f,
  .vbgi_adaptive_luma_blend = 0.5f,
  .vbgi_max_clamp = 0.f,
  .vbgi_reduce_ao = 0.f,
  .vbgi_reduce_ao_strength = 1.f,
  .vbgi_debug_logging = 0.f,
  .vbgi_debug_view = 0.f,
  .vbgi_affect_lights = 0.f,
  .vbgi_lights_strength = 1.f,
  .vbgi_lights_saturation = 1.f,
  .vbgi_cascade_debug = 0.f,
  .shadow_filter_method = 1.f,
  .shadow_edge_tint = 2.f,
  .shadow_pcss_jitter_enabled = 1.f,
  .shadow_pcss_jitter_amount = 1.f,
  .shadow_pcss_jitter_speed = 237.f,
  .shadow_chss_noise_mode = 0.f,
  .shadow_base_softness = 0.2f,
  .shadow_chss_search_radius = 1.f,
  .shadow_chss_penumbra_scale = 80.f,
  .shadow_chss_depth_cap = 0.05f,
  .shadow_chss_min_radius = 0.f,
  .shadow_chss_post_blur = 0.f,
  .shadow_chss_blocker_count = 16.f,
  .shadow_chss_sample_count = 16.f,
  .shadow_chss_onset_bias = 0.f,
  .shadow_chss_penumbra_curve = 2.f,
  .shadow_penumbra_color_strength = 1.f,
  .shadow_penumbra_vibrance = 1.f,
  .shadow_penumbra_detection = 0.5f,
  .shadow_penumbra_debug_view = 0.f,
  .shadow_penumbra_color_brightness = 1.f,
  .shadow_penumbra_falcom_blend = 0.f,
  .shadow_penumbra_edge_vibrance = 1.f,
  .shadow_penumbra_lightcolor_blend = 0.f,
  .shadow_penumbra_lightcolor_saturation = 1.f,
  .shadow_isfast_enabled = 0.f,
  .shadow_isfast_texture_loaded = 0.f,
  .shadow_isfast_spatial_scale = 1.f,
  .shadow_isfast_temporal_speed = 1.f,
  .shadow_isfast_seed_offset = 0.f,
  // -- Kai-specific defaults --
  .cubemap_improvements_enabled = 1.f,
  .cubemap_lighting_mip_boost = 1.5f,
  .floor_cubemap_mip_scale = 4.f,
  .ssgi_mod_enabled = 1.f,
  .ssgi_color_boost = 1.f,
  .ssgi_alpha_boost = 1.f,
  .ssgi_pow = 1.f,
  .dof_mode = 1.f,
  .dof_strength = 1.f,
  .dof_radius_scale = 1.33f,
  .dof_sample_count = 24.f,
  .dof_near_scale = 1.f,
  .dof_far_scale = 1.f,
  .dof_coc_curve = 1.f,
  .dof_edge_threshold = 0.25f,
  .char_gi_strength = 3.0f,
  .char_gi_alpha_scale = 1.0f,
  .char_gi_chroma_strength = 0.50f,
  .char_gi_luma_strength = 0.0f,
  .char_gi_shadow_power = 1.25f,
  .char_gi_dark_boost = 0.0f,
  .char_gi_bright_boost = 3.0f,
  .char_gi_headroom_power = 1.25f,
  .char_gi_max_add = 0.020f,
  .char_gi_peak_luma_cap = 0.0f,
  .char_gi_depth_reject = 2.0f,
  .fog_color_correction_enabled = 1.f,
  .fog_hue = 0.f,
  .fog_chrominance = 0.f,
  .fog_avg_brightness = 0.85f,
  .fog_min_brightness = 0.f,
  .fog_min_chroma_change = 0.f,
  .fog_max_chroma_change = 0.f,
  .fog_lightness_strength = 1.f,
  .fog_color_correction_strength = 0.5f,
  .ssr_mode = 1.f,
  .ssr_ray_count_scale = 1.f,
  .foliage_translucency_scale = 1.f,
  .foliage_opacity_scale = 1.f,
  .foliage_ssao_scale = 1.f,
  .char_shadow_strength = 1.f,
  .foliage_debug_mode = 0.f,
  .sss_dedicated_bound = 0.f,
  .char_gi_enabled = 1.f,
  .volfog_enabled = 1.f,
  .volfog_tricubic_enabled = 1.f,
  .volfog_color_correction_strength = 0.5f,
  .vbgi_kai_consume_falcom = 0.f,
  .vbgi_kai_falcom_blend = 0.5f,
  .vbgi_kai_gtvbao_only = 0.f,
  .shadow_edge_tint_kai = 1.f,
  .character_light_strength = 0.f,
  .gtvbao_cdf_enabled = 1.f,
  .gtvbao_cosine_enabled = 1.f,
  .gtvbao_cosine_mode = 2.f,
  .gtvbao_thickness_enabled = 1.f,
  // -- Bitmask fix toggles: all default OFF = previously shipped behaviour --
  .char_gtvbao_mode = 0.f,
  .char_gtvbao_mask_strength = 0.f,
  .char_gtvbgi_mask_strength = 0.f,
  .gtvbao_prefilter_enabled = 1.f,
  .brdf_hammon_diffuse_enabled = 0.f,
  .brdf_multiscatter_specular_enabled = 0.f,
  .brdf_diffuse_strength = 1.f,
  .brdf_specular_strength = 1.f,
  .brdf_roughness_min = 0.04f,
  .brdf_roughness_max = 1.f,
  .brdf_f0_source = 0.f,
  .gtvbao_exclude_foliage = 1.f,
  .gtvbao_foliage_ao_value = 1.f,
  .gtvbao_foliage_channel_mode = 0.f,
  .foliage_grass_ao_enabled = 0.f,
  .foliage_grass_ao_base = 0.25f,
  .foliage_grass_ao_tip = 1.f,
  .foliage_grass_ao_curve = 0.5f,
  .dof_sign_softness = 0.4f,
  .dof_coverage_enabled = 1.f,
  .gtvbao_atrous_enabled = 0.f,
  .gtvbao_atrous_depth_sigma = 1.f,
  .gtvbao_atrous_normal_sigma = 32.f,
  // �� Dynamic Cubemaps (Sora 2nd) ��
  .dynCube_enabled = 0.f,
  .dynCube_debug = 0.f,
  .dynCube_resolution = 0.f,
  .dynCube_history = 0.f,
  .dynCube_ggx = 0.f,
  .dynCube_capture_interval = 1.f,
  .dynCube_roughness_boost = 1.f,
  .dynCube_debug_logging = 0.f,
  .dynCube_debug_face = 0.f,
  .dynCube_debug_mip = 0.f,
  .dynCube_force_mip = -1.f,
  .dynCube_ssr_enabled = 0.f,
  .dynCube_ssr_samples = 16.f,
  .dynCube_ssr_distance = 20.f,
  .dynCube_ssr_blur = 2.f,
  .dynCube_ssr_symmetric_weights = 0.f,
  .dynCube_ssr_distance_fade = 0.5f,
  .dynCube_ssr_edge_fade = 0.3f,
  .dynCube_ssr_grazing_fade = 0.5f,
  .dynCube_ssr_thickness = 0.1f,
  .dynCube_ssr_char_occ_strength = 0.8f,
  .dynCube_ssr_char_occ_upness = 0.5f,
  .dynCube_vanilla_blur = 0.f,
  .dynCube_capture_boost = 1.f,
  .dynCube_history_blend = 0.5f,
  .dynCube_history_pos_threshold = 0.5f,
  .dynCube_character_capture = 0.f,
  .dynCube_force_vanilla = 0.f,
  .dynCube_force_dynamic = 0.f,
  .dynCube_force_ssr = 0.f,
  .dynCube_layer_mix = -1.f,
  .dynCube_blur = 0.f,
  .dynCube_lookup_direction_flip = 0.f,
  .dynCube_coverage_fade = 0.f,
  .dynCube_coverage_width = 2.f,
  .dynCube_ssr_isfast_enabled = 1.f,
  .dynCube_ssr_isfast_strength = 1.f,
  .dynCube_ssr_isfast_spatial = 1.f,
  .dynCube_ssr_isfast_temporal = 1.f,
  .dynCube_ssr_confidence_fallback = 0.f,
  .dynCube_vertical_offset = 0.f,
  .dynCube_worldbox_contrib = 0.25f,
  .dynCube_capture_soften = 0.f,
  .dynCube_global_strength = 1.f,
  .dynCube_ssr_replacement = 0.f,
  .dynCube_ssr_replacement_debug = 0.f,
  .dynCube_game_ssr = 1.f,
  .dynCube_vanilla_refine_fix = 1.f,
  .dynCube_vanilla_refine_threshold = 0.5f,
  .dynCube_vanilla_history_fixed = 0.f,
  .dynCube_vanilla_history_weight = 0.9f,
  .dynCube_vanilla_disoc_reject = 1.f,
  .dynCube_vanilla_disoc_depth = 0.25f,
  .dynCube_vanilla_disoc_uv = 0.05f,
  .dynCube_vanilla_isfast = 1.f,
  .dynCube_vanilla_isfast_frame = -1.f,
  .dynCube_vanilla_ssr_enabled = 1.f,
  .gtvbao_optimization = 1.f,
  .custom_shader_logging = 0.f,
  .dynCube_sparkle_rejection = 0.f,
  .char_outline_intensity = 1.f,
  // -- Custom TAA (Sora 1st/2nd) defaults: off = vanilla TAA runs untouched --
  .custom_taa_enabled = 0.f,
  .custom_taa_history_filter = 0.f,
  .custom_taa_clip_mode = 0.f,
  .custom_taa_kdop_axes = 0.f,
  .custom_taa_dmin = 0.048f,
  .custom_taa_kdop_epsilon = 0.00001f,
  .custom_taa_static_feedback = 0.9f,
  .custom_taa_dynamic_feedback = 0.5f,
  .custom_taa_motion_scale = 0.125f,
  .custom_taa_debug = 0.f,
  .custom_taa_history_valid = 0.f,
  .custom_taa_overshoot_softness = 1.f,
  .custom_taa_silhouette_rejection = 4.f,
  .custom_taa_squared_motion_response = 0.f,
  .custom_taa_detail_restore = 0.f,
  .custom_taa_detail_target = 0.85f,
  // -- RCAS post-TAA sharpening defaults: always on, strength 0 = off --
  .rcas_strength = 0.2f,
  .rcas_enabled = 1.f,
  .rcas_denoise = 0.f,
  .rcas_motion_on = 1.f,
  .rcas_motion_multiplier = 2.0f,
  .rcas_motion_max = 1.0f,
  .rcas_motion_threshold = 1.0f,
  .rcas_motion_range = 2.0f,
  .rcas_motion_response = 1.0f,
  .rcas_debug = 0.f,
  // -- FXAA post-TAA defaults: on (between TAA and RCAS), High quality --
  .fxaa_enabled = 1.f,
  .fxaa_quality = 1.f,
  .fxaa_subpix = 0.75f,
  .fxaa_edge_threshold = 0.166f,
  .fxaa_edge_threshold_min = 0.0625f,
  // �� GTVBAO half-resolution spatial pipeline defaults: Full = existing behavior ��
  .gtvbao_resolution = 1.f,
  .gtvbao_upscale_plane_sigma = 40.f,
  .gtvbao_upscale_normal_power = 16.f,
  .gtvbao_upscale_debug = 0.f,
  // �� DOF IS-FAST rotated gather ��
  .dof_isfast_enabled = 1.f,
  .dof_isfast_noise_frame = -1.f,
  // �� Motion Blur (Guertin 2013) defaults: off = no dispatches, no pushes ��
  .mb_mode = 0.f,
  .mb_intensity = 1.f,
        .mb_sample_count = 16.f,
  .mb_max_radius_px = 40.f,
  .mb_center_weight_k = 40.f,
  .mb_jitter_h = 0.95f,
  .mb_min_velocity_g = 1.5f,
  .mb_neighbor_t = 1.f,
  .mb_depth_tolerance = 0.1f,
  .mb_velocity_format = 0.f,  // DEPRECATED, unread (no velocity surface remains)
  .mb_jitter_source = 0.f,
  .mb_jitter_ready = 0.f,
  .mb_debug_view = 0.f,
  .mb_working_w = 0.f,
  .mb_working_h = 0.f,
  .mb_depth_w = 0.f,
  .mb_depth_h = 0.f,
  .mb_motion_w = 0.f,
  .mb_motion_h = 0.f,
  .mb_tiles_x = 1.f,
  .mb_tiles_y = 1.f,
  .mb_tile_uv = 0.f,
  .mb_pass = 0.f,
  .mb_frame_index = 0.f,
  .mb_motion_valid = 0.f,
  .mb_debug_chain = 0.f,
  .mb_local_velocity_weights = 1.f,
  .mb_depth_test = 1.f,
  .mb_halfres = 0.f,
  .mb_output_mask = 0.f,
  .mb_gather_side = 0.f,
  .mb_halfres_px = 10.f,
  .mb_frame_rate_reference = 60.f,
      .mb_frame_scale = 1.f,
      .mb_reserved_length = 1.f,
      .mb_camera_sign = 1.f,
        .mb_camera_jitter = 1.f,
        .mb_camera_cut = 1.f,
        .mb_camera_cut_px = 120.f,
  // -- Contact / Micro Shadows --
  // Defaults are the settled values from the reference implementation, not
  // experimental ones: micro at full opacity with the paper's aperture, contact
  // at 8 samples over 50 world units with a jittered clip-space march. Both are
  // OFF by default so enabling them is a deliberate act, and both have to be
  // enabled separately because they are separate passes with separate costs.
  .cs_micro_enabled = 0.f,
  .cs_micro_strength = 1.f,
  .cs_micro_env_strength = 1.f,
  .cs_micro_char_strength = 1.f,
  .cs_micro_opacity = 1.f,
  .cs_micro_aperture_scale = 1.f,
  .cs_micro_debug = 0.f,
  // GTVBAO AO rather than the game AO: the game capture is not frame-reliable,
  // and AO = 1 makes the micro term identically 1 at every pixel.
  .cs_micro_ao_source = 1.f,
  .cs_contact_enabled = 0.f,
  .cs_contact_strength = 1.f,
  .cs_contact_env_strength = 1.f,
  .cs_contact_char_strength = 1.f,
  .cs_contact_sample_count = 8.f,
  .cs_contact_ray_length = 50.f,
  .cs_contact_thickness = 0.35f,
  .cs_contact_bias = 0.0001f,
  .cs_contact_normal_bias = 0.0001f,
  .cs_contact_sky_depth = 100000.f,
  .cs_contact_max_darkening = 1.f,
  .cs_contact_isfast_enabled = 1.f,
  .cs_contact_debug = 0.f,
  .cs_contact_local_enabled = 0.f,
  .cs_contact_local_strength = 1.f,
  .cs_contact_local_sample_count = 4.f,
  .cs_contact_local_ray_length = 2.f,
  .cs_contact_local_max_lights = 4.f,
  // Runtime values, written by the addon immediately before the push.
  .cs_noise_frame = -1.f,
  .cs_working_w = 0.f,
  .cs_working_h = 0.f,
  .cs_ao_bound = 0.f,
  .cs_micro_dedicated_bound = 0.f,
  .cs_contact_dedicated_bound = 0.f,
  // The AO-quantisation dither is off by default: it only helps on surfaces where
  // GTVBAO's 8-bit visibility visibly bands, and it costs a noise sample per pixel.
  .cs_micro_isfast_enabled = 0.f,
  // 0 = derive the sun-contact range gate from the engine's last cascade split.
  .cs_contact_sun_range = 0.f,
  // 4 reproduces the original 4-sample appearance exactly, so the estimator change is
  // a no-op at the default Sample Count.
  .cs_contact_response_scale = 4.f,
  };

// ----------- GTVBAO Backend � constants, types, fwd decls -----------

constexpr uint32_t kLightingGtvbaoRegister = 22u;
constexpr uint32_t kLightingVbgiRegister   = 23u;  // t23 = vbgiTexture
constexpr uint32_t kLightingDepthRegister = 4u;   // t4 = depthTexture (Sora)
constexpr uint32_t kLightingDepthRegisterKai = 3u; // t3 = depthTexture (Kai)
constexpr uint32_t kLightingSsaoRegister = 5u;    // t5 = ssaoTexture (Sora)
constexpr uint32_t kLightingSsaoRegisterKai = 4u; // t4 = ssaoTexture (Kai)
constexpr uint32_t kLightingSceneCbRegister = 0u; // b0 = cb_scene
constexpr uint32_t kGTVBAODepthMipLevels = 5u;
constexpr uint32_t kGtvbaoDescriptorTableParamCount = 4u;  // sampler, cbv, srv, uav
constexpr uint32_t kGtvbaoPushConstantsLayoutParam = 4u;   // push_constants at b13
// Float count of the GTVBAO push-constant block (cbuffer cb_gtvbao at b13).
// Must match the cbuffer declared in gtvbao_common.hlsl. Named so the builder,
// the layout range and all nine push sites cannot drift apart -- a mismatch
// here silently truncates the tail of the block rather than failing loudly.
constexpr uint32_t kGtvbaoPushConstantFloats = 70;
constexpr uint32_t kLightingMrtNormalRegister = 1u;  // t1 = mrtTexture0 (g-buffer normals)
constexpr uint64_t kGTVBAOStartupGuardFrames = 8u;
constexpr uint64_t kGTVBAOResizeGuardFrames = 4u;
// DynCube loading wipe: color captures older than this mean no lighting draws
// (loading screen) � arm history hard-replace so the next scene rebuilds clean.
constexpr uint64_t kDynCubeLoadingStaleFrames = 5u;
constexpr uint64_t kSceneCbMinimumBytes = 95u * 16u;

// -- Contact / Micro Shadows (shadows/*.cs_5_0.hlsl) --
// Two independent compute passes, so two layout/pipeline/table slots. The index
// is fixed and used to size the DeviceData arrays, so the shader, the host and
// the two resource sets cannot drift apart.
enum ShadowsPass : uint32_t {
  kShadowsPassMicro = 0u,    // micro_shadows.cs_5_0.hlsl    (t0 mrt, t1 aoU, t2 aoF -> u0)
  kShadowsPassContact = 1u,  // contact_shadows.cs_5_0.hlsl  (t0 depth, t1 mrt, t2 noise -> u0)
  kShadowsPassCount = 2u,
};
// The lighting pixel shaders read the two results from these slots. t33/t34 are
// declared by no game's lighting shader and claimed by no existing push, so they
// are free in Sora 1st, Sora 2nd, Kai, Kai-soft, Kyoto and Daybreak 2 alike (the
// addon already owns t17, t22, t23, t24, t28-t32).
constexpr uint32_t kLightingMicroShadowRegister = 33u;
constexpr uint32_t kLightingContactShadowRegister = 34u;
// Inputs for the local-light march, which runs inside the dynamic light loop
// rather than in a pass. Declared by the lighting shaders and the character pass
// for the shadow path alone; the host never writes to the game's own depth or
// noise registers on their behalf.
constexpr uint32_t kLightingShadowNoiseRegister = 35u;
constexpr uint32_t kLightingShadowDepthRegister = 36u;
// SRV counts per pass, matching the register() declarations in each shader.
constexpr uint32_t kShadowsSrvPerPass[kShadowsPassCount] = {4u, 3u};
constexpr uint32_t kShadowsUavPerPass[kShadowsPassCount] = {1u, 1u};

// -- Motion Blur (Guertin et al. 2013) --
// Fixed pass order; the indices address the layout/pipeline/table arrays in
// DeviceData so the stages cannot drift out of sync. Declared here because
// DeviceData sizes its arrays with it.
//
// Four dispatches, three shaders: TileMax is compiled once and dispatched twice
// (mb_pass selects the axis). There is deliberately no full-resolution
// intermediate for velocity or for linear depth � the gather and the tile
// stages read the game's own motion and depth textures and convert at the point
// of use. Those intermediates cost 44 MB of writes per frame at 1440p, enough
// to evict the gather's working set from L2 and push its 25 taps per pixel out
// to DRAM, which measured as the dominant cost of the whole feature.
enum MotionBlurPass : uint32_t {
  // kMbDownsample and kMbComposite bracket the gather and only run when
  // MotionBlurHalfRes is on. They are separate passes rather than compile-time
  // variants of the gather because the gather itself is already resolution
  // agnostic: every dimension it reads is either a push constant or derived from
  // UV, so halving mb_working_w/h is all it takes to gather at half res.
  //
  // The composite is not optional polish: the gather's early-out writes its
  // centre sample unchanged, so a static frame routed through the reduction would
  // come out softened. It restores unblurred pixels from the full-res source using
  // the mask the gather leaves in alpha.
  kMbDownsample = 0, kMbTileMax = 1, kMbNeighborMax = 2, kMbGather = 3, kMbComposite = 4,
  // Appended last so the indices above keep their meaning. Splits the game's
  // single motion texture into camera and object components at per-pixel
  // resolution, which TileMax cannot do because it is a reduction. Writes one
  // blended vector in .xy, so every pass above is unchanged by it, and is SKIPPED
  // entirely when both weights are 1 (camera + (game - camera) == game).
  kMbResolve = 5,
  kMotionBlurPassCount = 6,
};
// Sora 1st and Sora 2nd post-TAA resolve / tonemap. Confirmed the SAME hash in
// both games, so one constant serves both. Registered with an EMPTY payload on
// purpose: the game keeps running its own bytecode and we only take a per-draw
// hook, so the tonemap can never regress.
constexpr uint32_t kSoraTonemapHash = 0xC9FA40B7u;
// Kai's own deploy pass. Different game, different hash, same job. Named rather
// than left as a literal so the registration and the t0 capture below cannot drift
// apart -- see IsMotionBlurDeployHash.
constexpr uint32_t kKaiTonemapHash = 0x034581D3u;
// -- Motion blur activation ----------------------------------------------------
// One channel, gated on shader_injection.mb_mode. Cutscene Only is resolved
// against the DoF-dispatch signal, which only fires in cutscenes, so that half of
// the test needs the frame's cutscene flag and cannot live in the setting
// predicate.
static bool MotionBlurActive(bool cutscene) {
  return shader_injection.mb_mode >= 1.5f
      || (shader_injection.mb_mode >= 0.5f && cutscene);
}
// The settings that hang off the chain (Max Radius, Quality, Half Resolution,
// the debug views) pass cutscene=true to ask whether the mode could EVER run,
// which is the right question for greying out a control: the answer must not
// flicker as the user walks in and out of a cutscene.

// -- Motion blur logging -------------------------------------------------------
// Deliberately NOT in ShaderInjectData: no shader can read it, so it must not
// consume a float in the pushed block.
//
// Default ON. These are one-shot and rate-limited rather than per-frame, and the
// buffer-dimension report is the only way to confirm from outside whether the
// game's depth and motion textures actually match the tonemap's resolution.
static float g_mb_logging = 1.f;
static bool MotionBlurLogEnabled() { return g_mb_logging > 0.5f; }

// -- Motion blur quality preset ------------------------------------------------
// Replaces the raw Max Samples slider. The slider's useful range was narrow, the
// bottom of it produced visible stepping in fast movement, and the exact number
// is not something a user can judge without an A/B against a specific scene --
// so four named steps, each one a rung the adaptive ladder already uses.
//
// Deliberately NOT in ShaderInjectData: the shader wants a plain tap count, not a
// quality name, so translating here keeps the push block unchanged and stops a
// preset index from ever reaching the gather as if it were a sample count.
static float g_mb_quality = 1.f;  // 0 Low, 1 Medium, 2 High, 3 Ultra

// -- Motion vector source ------------------------------------------------------
// 0 = the GBuffer's RTV4 (the engine's own motion output, written every frame),
// 1 = the TAA draw's t3 (today's path, which only exists once TAA has run).
//
// Deliberately NOT in ShaderInjectData: no shader needs to know which route the
// motion arrived by. The two views hold the same bytes in the same encoding, so
// every pass downstream is unchanged by this setting.
static float g_mb_motion_input = 0.f;  // RTV4 is the default: it does not depend on TAA
// Medium is the default. Note this is a change from the old 25: the presets top
// out at 24 because the ladder's own rungs stop at 16 and only the top bucket uses
// the ceiling, so a higher number buys smoothness only on the fastest tiles.
static float MBQualitySampleCount(float quality) {
  switch (static_cast<int>(quality + 0.5f)) {
    case 0:  return 12.f;  // Low
    case 2:  return 20.f;  // High
    case 3:  return 24.f;  // Ultra
    default: return 16.f;  // Medium
  }
}
// Height the shaders divide pixel-denominated paper constants by, so a 4K
// player sees the same blur as a 1080p player. Must match MB_REF_H in
// motionblur/motion_blur_common.hlsli.
constexpr float kMotionBlurRefHeight = 1080.0f;
constexpr uint32_t kMotionBlurLogFrameGap = 120u;

// -- GTVBAO normal tuning globals (separate from ShaderInjectData) --
// Every default below is the NEUTRAL value for its knob: with all of them at
// these values the MRT normal reaches GTVBAO_MainPass completely unmodified
// (final_blend == 1.0, no xy/z scaling), so the g-buffer normal is the only
// thing driving the horizon search and the depth-derived normal is a fallback
// only. Raising a knob turns that one term on; nothing biases the result by
// default.
//
// NOTE: the authoritative defaults are the Setting .default_value entries in
// the GTVBAO settings block below. LoadSettings() overwrites these with
// default_value whenever the key is absent from the ReShade ini, and
// ResetSettings() restores default_value on top of them, so a divergence
// between the two is silently lost at runtime. Keep them in sync; the values
// here only apply before the first LoadSettings() pass.
static float g_gtvbao_normal_input_mode     = 1.f;   // 1 = use the g-buffer normal
static float g_gtvbao_normal_influence      = 1.f;   // xy scale: 1 = untouched
static float g_gtvbao_normal_z_preservation = 1.f;   // z  scale: 1 = untouched
static float g_gtvbao_normal_depth_blend    = 1.f;   // MRT weight before shaping
static float g_gtvbao_normal_sharpness      = 1.f;   // gamma: 1 = passthrough
static float g_gtvbao_normal_edge_rejection = 0.f;   // 0 = no silhouette rejection
static float g_gtvbao_normal_detail_response = 0.f; // 0 = no disagreement reweight
static float g_gtvbao_normal_max_darkening  = 1.f;   // cap: 1 = uncapped
static float g_gtvbao_normal_darkening_mode = 0.f;
// 2x2 A/B matrix: {view_g, viewInv_g} x {flip Z, no flip}.
// 0=view_g+flipZ (suspected correct: GTVBAO is +Z forward, view_g is -Z),
// 1=viewInv_g, 2=passthrough, 3=view_g, 4=viewInv_g+flipZ
static float g_gtvbao_normal_transform_mode = 0.f;
// -- VBGI (GI) normal settings: independent copies of the four that shape the
// GI sample normal (input mode, influence, z preservation, transform mode) --
// Defaults match the AO neutral values, so enabling VBGI changes nothing until
// a knob moves. Deliberately NOT split: texel mapping, the packed decode, and
// the transform arithmetic. Those are correctness surfaces, and letting AO and
// GI read different texels or decode differently is how the two drift apart.
static float g_gtvbao_gi_normal_input_mode     = 1.f;
static float g_gtvbao_gi_normal_influence      = 1.f;
static float g_gtvbao_gi_normal_z_preservation = 1.f;
static float g_gtvbao_gi_normal_transform_mode = 0.f;

// -- Dynamic Cubemaps (Sora 2nd) � standalone t17 replacement --
constexpr uint32_t kDynCubeRegister = 17u; // t17 texEnvMap_g
constexpr uint32_t kDynCubeHistPosRegister = 29u; // t29 dynCubeHistPosTex (debug 11/12)
constexpr uint32_t kDynCubeVanillaRegister = 30u; // t30 dynCubeVanillaTex (vanilla cube fallback)
constexpr uint32_t kDynCubeSSRRegister = 31u;     // t31 dynCubeSSRTex (blurred SSR result)
constexpr uint32_t kDynCubeSSRRawRegister = 32u;  // t32 dynCubeSSRRawTex (raw SSR, debug 17)
constexpr uint32_t kDynCubeSSRLayoutVersion = 5u;
constexpr uint32_t kDynCubeSSRBlurLayoutVersion = 5u;
constexpr uint32_t kDynCubeWorldBoxLayoutVersion = 2u;
constexpr uint32_t kDynCubeCaptureLayoutVersion = 1u;  // bump when the capture pipeline layout shape changes (forces recreate)
constexpr uint32_t kRCASLayoutVersion = 5u;  // bump when the RCAS pipeline layout shape changes (forces recreate)
constexpr uint32_t kFXAALayoutVersion = 1u;  // bump when the FXAA pipeline layout shape changes (forces recreate)
// Strip sRGB encoding for UAV-compatible temp storage and raw (non-decoding)
// SRV reads. Copies stay bitwise so the game keeps decoding exactly as before.
// Non-sRGB formats map to identity (FP16 HDR path unchanged).
static reshade::api::format RCASLinearFormat(reshade::api::format fmt) {
  using F = reshade::api::format;
  switch (fmt) {
    case F::r8g8b8a8_unorm_srgb: return F::r8g8b8a8_unorm;
    case F::b8g8r8a8_unorm_srgb: return F::b8g8r8a8_unorm;
    default: return fmt;
  }
}
constexpr uint32_t kDynCubeDefaultSize = 128u;
static uint32_t DynCubeResolveSize(float v);

// -- VBGI globals removed � now controlled via ShaderInjectData fields (shared.h). --
// vbgi_enabled, vbgi_intensity, vbgi_saturation, vbgi_multibounce, vbgi_gi_power
// are all part of shader_injection and pushed via BuildGTVBAOPushConstants.
static float g_vbgi_light_exposure = 0.05f;  // HDR light buffer exposure scale (lower = dimmer GI)

// -- IS-FAST noise --
static float g_isfast_enabled       = 0.f;
static float g_isfast_strength      = 1.f;
static float g_isfast_debug_logging = 0.f;
static float g_isfast_spatial_scale = 1.f;
static float g_isfast_temporal_speed = 1.f;
static float g_isfast_seed_offset   = 0.f;

// -- Settings visibility --
static float g_settings_mode            = 0.f;   // 0=Basic, 1=Advanced
static bool IsAdvancedSettingsMode() { return g_settings_mode >= 0.5f; }

// -- Kai detection --
static float g_char_vbgi_composite_method = 1.f;  // Kai Character VBGI master toggle

static bool IsKai() {
  static bool checked = false;
  static bool is_kai = false;
  if (!checked) {
    char exePath[MAX_PATH];
    GetModuleFileNameA(nullptr, exePath, MAX_PATH);
    std::string name(exePath);
    auto lastSlash = name.find_last_of("\\/");
    if (lastSlash != std::string::npos) name = name.substr(lastSlash + 1);
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
    is_kai = (name == "kai.exe");
    checked = true;
  }
  return is_kai;
}

// -- Sora 2nd detection --
static bool IsSora2nd() {
  static bool checked = false;
  static bool is_sora2nd = false;
  if (!checked) {
    char exePath[MAX_PATH];
    GetModuleFileNameA(nullptr, exePath, MAX_PATH);
    std::string name(exePath);
    auto lastSlash = name.find_last_of("\\/");
    if (lastSlash != std::string::npos) name = name.substr(lastSlash + 1);
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
    is_sora2nd = (name == "sora_2nd.exe");
    checked = true;
  }
  return is_sora2nd;
}

// -- Sora 1st detection --
static bool IsSora1st() {
  static bool checked = false;
  static bool is_sora1st = false;
  if (!checked) {
    char exePath[MAX_PATH];
    GetModuleFileNameA(nullptr, exePath, MAX_PATH);
    std::string name(exePath);
    auto lastSlash = name.find_last_of("\\/");
    if (lastSlash != std::string::npos) name = name.substr(lastSlash + 1);
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
    is_sora1st = (name == "sora_1st.exe");
    checked = true;
  }
  return is_sora1st;
}

// -- Games with motion blur ---------------------------------------------
// Kai and the two Sora engines. Named for the FEATURE, not for an engine family,
// because Kai is not a Sora and the set of games that support motion blur is not
// the same set that any other feature happens to cover.
//
// Deliberately NOT IsDynCubeT17Game() below, which happens to be the same three
// games today. That one is named for the global t17 cubemap capture, and reusing
// it would tie motion blur's availability to an unrelated feature's naming -- the
// sort of coupling that breaks silently when either side gains or loses a game.
static bool IsMotionBlurGame() {
  return IsKai() || IsSora1st() || IsSora2nd();
}

// Kai is forced onto the render target. It resolves no TAA hash in this addon, so
// the t3 capture can never happen there, and the setting is hidden for Kai
// precisely because it would be a choice with one dead branch. Testing the value
// here rather than trusting the UI means a saved config from another game cannot
// leave Kai on a source that never binds.
static bool MBMotionUseRtv() {
  return IsKai() || g_mb_motion_input < 0.5f;
}

// The deploy point and the t0 capture MUST agree on which shader is the tonemap.
//
// They were maintained independently, and registering Kai's deploy without adding
// it to the capture is exactly how the deploy hook fired and then immediately
// bailed with "needs the tonemap's t0 view": the hash was accepted in one place
// and rejected in the other. One predicate, used by both, so the next game cannot
// reproduce it.
static bool IsMotionBlurDeployHash(uint32_t hash) {
  return hash == kSoraTonemapHash || (IsKai() && hash == kKaiTonemapHash);
}

static const char* MBMotionInputName() {
  return MBMotionUseRtv() ? "GBuffer RTV" : "TAA t3";
}

// -- Dynamic Cubemap t17 supported games --
// The global t17 vanilla-capture + dynamic override only serve Kai, Sora 1st and
// Sora 2nd. Every other game keeps fully vanilla cubemap bindings.
static bool IsDynCubeT17Game() {
  return IsKai() || IsSora1st() || IsSora2nd();
}

// -- Daybreak 2 detection --
static bool IsDaybreak2() {
  static bool checked = false;
  static bool is_db2 = false;
  if (!checked) {
    char exePath[MAX_PATH];
    GetModuleFileNameA(nullptr, exePath, MAX_PATH);
    std::string name(exePath);
    auto lastSlash = name.find_last_of("\\/");
    if (lastSlash != std::string::npos) name = name.substr(lastSlash + 1);
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
    is_db2 = (name == "kuro2.exe");
    checked = true;
  }
  return is_db2;
}

// Kyoto Xanadu. Identified as "none of the other four" rather than by exe name
// because it has never had an explicit predicate here and those four are the only
// other titles this addon supports. It matters to the screen-space shadow options
// because Kyoto has no character lighting pass at all: its lighting shader is a
// single full-screen draw with no character branch, so there is no engine
// character shadow for the vanilla toggle to drive.
static bool IsKyoto() {
  return !IsKai() && !IsSora1st() && !IsSora2nd() && !IsDaybreak2();
}

// -- Lighting shader identification (Sora + Kai) --
static bool IsLightingShader(uint32_t hash) {
  return hash == 0xFDAAF80Eu    // Sora lighting
      || hash == 0xCA3D8596u    // Sora 2nd lighting
      || hash == 0x0CDCB258u    // Kyoto lighting
      || hash == 0x430ED091u    // Kai lighting
      || hash == 0xF6C55E5Fu;   // Kai lighting soft
}

// -- CPU optimization toggles --
static float g_gtvbao_frame_skip         = 0.f;  // per-component frame skip (0=off, 1=every 2nd, �)
static float g_gtvbao_cs_dispatch_fix    = 0.f;  // 0=Off, 1=Restore, 2=Null, 3=Null+Restore
static float g_vbgi_frame_skip           = 0.f;
static float g_multibounce_frame_skip    = 0.f;
static float g_cpuopt_deferred_dispatch   = 1.f;  // dispatch GTVBAO/VBGI in OnPresent, not inline (default ON for Kai)
static float g_cpuopt_ensure_pipelines    = 0.f;  // kai-style: don't destroy/recreate pipelines every frame
static float g_gtvbao_jitter_toggle       = 0.f;  // enable jitter even when denoise is off

using GTVBAODescriptorTableSet =
    std::array<reshade::api::descriptor_table, kGtvbaoDescriptorTableParamCount>;

struct __declspec(uuid("b1a2c3d4-e5f6-7890-abcd-ef1234567890")) DeviceData {
  uint32_t working_width = 0u;
  uint32_t working_height = 0u;

  reshade::api::resource depth_mips_texture = {};
  reshade::api::resource_view depth_mips_srv = {};
  std::array<reshade::api::resource_view, kGTVBAODepthMipLevels> depth_mips_uavs = {};

  reshade::api::resource ao_term_a_texture = {};
  reshade::api::resource_view ao_term_a_srv = {};
  reshade::api::resource_view ao_term_a_uav = {};
  reshade::api::resource ao_term_b_texture = {};
  reshade::api::resource_view ao_term_b_srv = {};
  reshade::api::resource_view ao_term_b_uav = {};

  reshade::api::resource history_ao_texture_a = {};   // spatio-temporal history (ping-pong)
  reshade::api::resource_view history_ao_srv_a = {};
  reshade::api::resource_view history_ao_uav_a = {};
  reshade::api::resource history_ao_texture_b = {};
  reshade::api::resource_view history_ao_srv_b = {};
  reshade::api::resource_view history_ao_uav_b = {};
  bool history_ao_read_from_a = true;  // ping-pong toggle

  reshade::api::resource edges_texture = {};
  reshade::api::resource_view edges_srv = {};
  reshade::api::resource_view edges_uav = {};

  reshade::api::resource composite_texture = {};
  reshade::api::resource_view composite_srv = {};
  reshade::api::resource_view composite_uav = {};

  // 1�1 white fallback � always valid, returned when GTVBAO is off / not ready.
  reshade::api::resource fallback_texture = {};
  reshade::api::resource_view fallback_srv = {};

  reshade::api::sampler point_clamp_sampler = {};

  // Resolution-change guard.
  uint32_t last_created_game_width = 0u;
  uint32_t last_created_game_height = 0u;
  // Half-resolution spatial pipeline (0=Full existing behavior, 1=Half).
  uint32_t half_width = 0u;
  uint32_t half_height = 0u;
  float last_created_gtvbao_resolution = -1.f;
  // Half-mode GI denoised (half) + full-res reconstruction targets.
  reshade::api::resource vbgi_denoised_half_texture = {};
  reshade::api::resource_view vbgi_denoised_half_srv = {};
  reshade::api::resource_view vbgi_denoised_half_uav = {};
  reshade::api::resource upscale_ao_texture = {};
  reshade::api::resource_view upscale_ao_srv = {};
  reshade::api::resource_view upscale_ao_uav = {};
  reshade::api::resource upscale_gi_texture = {};
  reshade::api::resource_view upscale_gi_srv = {};
  reshade::api::resource_view upscale_gi_uav = {};
  reshade::api::pipeline_layout upscale_layout = {};
  reshade::api::pipeline upscale_pipeline = {};  // 4-Tap joint reconstruction (Half mode)
  GTVBAODescriptorTableSet upscale_tables = {};

  reshade::api::pipeline_layout prefilter_layout = {};
  reshade::api::pipeline_layout main_layout = {};
  reshade::api::pipeline_layout denoise_layout = {};
  reshade::api::pipeline prefilter_pipeline = {};
  reshade::api::pipeline main_low_pipeline = {};
  reshade::api::pipeline main_medium_pipeline = {};
  reshade::api::pipeline main_high_pipeline = {};
  reshade::api::pipeline main_ultra_pipeline = {};
  reshade::api::pipeline denoise_pipeline = {};
  reshade::api::pipeline denoise_last_pipeline = {};
  reshade::api::pipeline denoise_last_kai_pipeline = {};  // Kai: correct prevViewProj_g offset (c85)
  reshade::api::pipeline denoise_last_sora2nd_pipeline = {};  // Sora 2nd: correct prevViewProj_g offset (c75)
  // -- �-trous wavelet spatial filter (R3) --
  reshade::api::pipeline_layout atrous_layout = {};
  reshade::api::pipeline atrous_pipeline = {};
  GTVBAODescriptorTableSet atrous_tables = {};
  // Normal pre-decode pass (�-trous perf): decoded MRT normals, RGBA16F
  reshade::api::pipeline_layout normal_prep_layout = {};
  reshade::api::pipeline normal_prep_pipeline = {};
  GTVBAODescriptorTableSet normal_prep_tables = {};
  reshade::api::resource normal_prep_texture = {};
  reshade::api::resource_view normal_prep_srv = {};
  reshade::api::resource_view normal_prep_uav = {};

  // Descriptor tables � pre-allocated per pass.
  GTVBAODescriptorTableSet prefilter_tables = {};
  GTVBAODescriptorTableSet main_tables = {};
  GTVBAODescriptorTableSet denoise_tables = {};

  reshade::api::resource_view captured_depth_srv = {};
  std::atomic<bool> captured_depth_live{true};  // destroy-event driven; false = target freed since capture
  uint64_t captured_depth_res = 0u;             // resource behind the view (destroy matching)
  std::string captured_depth_dims = "none";     // cached at capture (push context only)
  uint32_t captured_depth_w = 0u, captured_depth_h = 0u;  // same, numeric (shadow pass sizing)
  reshade::api::resource_view captured_ssao_srv = {};
  // The game SSAO is only ever an OPTIONAL micro-shadow input, so unlike depth
  // and the MRT normal it has no recorded resource and no destroy-event
  // tracking. The frame stamp is what tells a consumer "captured this frame"
  // from "left over from an earlier frame, possibly a different resolution".
  uint64_t captured_ssao_frame = 0u;
  reshade::api::resource_view captured_mrt_normal_srv = {};
  std::atomic<bool> captured_mrt_live{true};    // destroy-event driven; false = target freed since capture
  uint64_t captured_mrt_res = 0u;               // resource behind the view (destroy matching)
  std::string captured_mrt_dims = "none";       // cached at capture (push context only)
  reshade::api::resource_view captured_color_srv = {};   // t0 � lighting input color texture
  std::atomic<bool> captured_color_live{true};  // destroy-event driven; false = target freed since capture
  uint64_t captured_color_res = 0u;             // resource behind the view (destroy matching)
  std::string captured_color_dims = "none";     // cached at capture (push context only)
  uint32_t captured_color_w = 0u, captured_color_h = 0u;  // cached dims (SSR sizing, no draw-time query)
  reshade::api::resource_view captured_ssr1_srv = {};   // ssr2-draw t0 � vanilla ssr1 march result (replacement debug view 2)
  std::atomic<bool> captured_ssr1_live{true};   // destroy-event driven
  uint64_t captured_ssr1_res = 0u;              // resource behind the view (destroy matching)
  reshade::api::resource_view captured_ssr_mrt_srv = {};  // ssr1-draw t2 � march's own mrt0 (composite gate + normal decode)
  std::atomic<bool> captured_ssr_mrt_live{true};  // destroy-event driven
  uint64_t captured_ssr_mrt_res = 0u;             // resource behind the view (destroy matching)
  reshade::api::resource_view captured_vanilla_env_srv = {};  // game's texEnvMap_g (t17) binding � vanilla cube fallback
  reshade::api::resource_view captured_scene_cbv_view = {};  // push_descriptors passes CBV as resource_view
  reshade::api::buffer_range captured_scene_cbv = {};
  bool captured_scene_cbv_valid = false;
  std::atomic<bool> captured_cbv_live{true};  // destroy-event driven; false = buffer freed since capture
  uint64_t captured_cbv_res = 0u;             // buffer behind the range (destroy matching)
  std::string captured_cbv_dims = "none";     // cached at capture (push/bind context only)
  // Deferred snapshot destroy tracking (plain copies at snapshot time; matched like captured).
  std::atomic<bool> defDepthLive{true}, defMrtLive{true}, defCbvLive{true};
  uint64_t deferred_depth_res = 0u, deferred_mrt_res = 0u, deferred_cbv_res = 0u;
  uint64_t captured_scene_cbv_frame = UINT64_MAX;
  uint64_t captured_color_frame = UINT64_MAX;   // frame_index of last lighting t0 capture (stale = loading screen)
  uint64_t captured_depth_frame = UINT64_MAX;   // frame_index of last lighting depth capture (kept for future detectors)
  bool resources_created = false;
  uint64_t frame_index = 0u;
  uint64_t resize_guard_until_frame = 0u;
  reshade::api::command_list* immediate_cmd_list = nullptr;  // refreshed every present; scheduler ctx check

  // Deferred dispatch snapshots (kai-style): captured at lighting draw, used at present.
  reshade::api::resource_view deferred_depth_srv = {};
  reshade::api::resource_view deferred_ssao_srv = {};
  reshade::api::resource_view deferred_mrt_normal_srv = {};
  reshade::api::resource_view deferred_scene_cbv_view = {};
  reshade::api::buffer_range deferred_scene_cbv = {};
  bool deferred_scene_cbv_valid = false;
  uint64_t deferred_scene_cbv_frame = UINT64_MAX;
  bool deferred_pending = false;

  // -- GI resources (now integrated � no separate VBGI pipeline) --
  reshade::api::resource vbgi_output_texture = {};
  reshade::api::resource_view vbgi_output_srv = {};
  reshade::api::resource_view vbgi_output_uav = {};
  reshade::api::resource vbgi_denoised_texture = {};
  reshade::api::resource_view vbgi_denoised_srv = {};
  reshade::api::resource_view vbgi_denoised_uav = {};
  reshade::api::resource captured_light_buffer_texture = {};
  reshade::api::resource_view captured_light_buffer_srv = {};
  bool captured_light_buffer_valid = false;   // true after first frame's capture
  // -- Multi-bounce accumulation (HDR light buffer + previous GI) --
  reshade::api::resource multibounce_texture = {};
  reshade::api::resource_view multibounce_srv = {};
  reshade::api::resource_view multibounce_uav = {};
  reshade::api::pipeline multibounce_pipeline = {};
  reshade::api::pipeline_layout multibounce_layout = {};
  GTVBAODescriptorTableSet multibounce_tables = {};
  bool vbgi_denoised_valid = false;            // true after first denoise completes
  reshade::api::resource_view fallback_uav = {};  // 1x1 UAV fallback
  // -- Foliage mask (quarter-res R8_UINT, pre-pass) --
  reshade::api::resource foliage_mask_texture = {};
  reshade::api::resource_view foliage_mask_srv = {};
  reshade::api::resource_view foliage_mask_uav = {};
  reshade::api::pipeline_layout foliage_mask_layout = {};
  reshade::api::pipeline foliage_mask_pipeline = {};
  GTVBAODescriptorTableSet foliage_mask_tables = {};
  // -- Debug UAV (bitmask debug views 6-8) --
  reshade::api::resource debug_texture = {};
  reshade::api::resource_view debug_srv = {};
  reshade::api::resource_view debug_uav = {};
  // -- IS-FAST noise --
  reshade::api::resource isfast_noise_texture = {};
  reshade::api::resource_view isfast_noise_srv = {};
  reshade::api::sampler isfast_sampler = {};
  bool isfast_texture_loaded = false;
  bool isfast_texture_attempted = false;  // only try DDS load once
  bool vbgi_bound = false;
  bool foliage_drawn_this_frame = false;  // set by foliage shader on_draw, reset per frame
  // -- GTVBAO: which ao_term buffer holds the latest final AO result --
  bool gtvbao_final_in_b = true;

  // CPU optimization tracking
  uint64_t last_bound_pipeline_handle = 0u;
  uint64_t last_srv0_handle = 0u;
  uint64_t last_srv1_handle = 0u;
  uint64_t last_uav0_handle = 0u;
  uint64_t last_cbv_handle = 0u;
  uint64_t last_sampler_handle = 0u;

  // -- Dynamic Cubemaps (Sora 2nd) � Phase 0A/B + Phase 1 history --
  // Aliases to the current (just-written) history set � dyncube_srv is the cube SRV for t17.
  reshade::api::resource dyncube_texture = {};        // current color resource (alias)
  reshade::api::resource_view dyncube_srv = {};       // current color cube SRV (alias, t17)
  reshade::api::resource_view dyncube_uav = {};       // current color UAV (alias, capture/solid write)
  reshade::api::sampler dyncube_sampler = {};         // point clamp
  reshade::api::pipeline_layout dyncube_capture_layout = {};
  reshade::api::pipeline dyncube_capture_pipeline = {};
  uint32_t dyncube_capture_layout_version = 0u;  // recreate layout/tables/pipeline when shape changes
  reshade::api::pipeline_layout dyncube_solid_layout = {};
  reshade::api::pipeline dyncube_solid_pipeline = {};
  GTVBAODescriptorTableSet dyncube_capture_tables = {};
  GTVBAODescriptorTableSet dyncube_solid_tables = {};
  bool dyncube_resources_created = false;
  uint32_t dyncube_size = kDynCubeDefaultSize;
  bool dyncube_solid_written = false;
  // Ping-pong history sets (Phase 1). Index 0 = A, 1 = B. dyncube_hist_cur = current write set.
  struct {
    reshade::api::resource color;          // RGBA16F cube-compatible, 128x128x6x1
    reshade::api::resource_view color_cube_srv;  // TextureCube SRV (t17)
    reshade::api::resource_view color_arr_srv;   // Texture2DArray SRV (compute prev read)
    reshade::api::resource_view color_uav;       // Texture2DArray UAV (compute current write)
    reshade::api::resource pos;            // RGBA16F (rgb=scaled pos, a=validity)
    reshade::api::resource_view pos_arr_srv;
    reshade::api::resource_view pos_cube_srv;   // TextureCube SRV (debug 11/12 histPos lookup)
    reshade::api::resource_view pos_uav;
    reshade::api::resource contrib;        // R16F (history contribution for debug 6)
    reshade::api::resource_view contrib_arr_srv;
    reshade::api::resource_view contrib_uav;
  } dyncube_hist[2];
  // GPU camera ping-pong (1x1 RGBA32F): previous frame's camera position.
  reshade::api::resource dyncube_cam[2];
  reshade::api::resource_view dyncube_cam_srv[2];
  reshade::api::resource_view dyncube_cam_uav[2];
  uint32_t dyncube_hist_cur = 0;           // current write set (0=A,1=B); reads use dyncube_readSet
  bool dyncube_needs_reset = true;         // clear history + first-frame reset
  bool dyncube_was_enabled = false;        // rising-edge latch for enabled->reset
  reshade::api::sampler dyncube_linear_sampler = {};     // trilinear clamp (GGX input mips)
  // Character mask (Phase 2)
  reshade::api::resource dyncube_charmask = {};          // RGBA16F cube, 1 mip (character mask)
  reshade::api::resource_view dyncube_charmask_srv = {};   // TextureCube SRV (for debug 9)
  reshade::api::resource_view dyncube_charmask_arr_srv = {}; // Texture2DArray SRV (world-box reduction read)
  reshade::api::resource_view dyncube_charmask_uav = {};   // Texture2DArray UAV (compute write)
  // World-fixed parallax proxy (Sora2nd v1): persistent GPU bounds, no CPU readback.
  // Survives cache round-trips (size-independent); destroyed + re-initialized on recreate.
  reshade::api::resource dyncube_worldbox_bounds = {};       // structured buffer, 2x float4: [0]=(min,valid) [1]=(max,spare)
  reshade::api::resource_view dyncube_worldbox_bounds_srv = {}; // buffer SRV (t33 lighting read)
  reshade::api::resource_view dyncube_worldbox_bounds_uav = {}; // buffer UAV (reduction merge write)
  reshade::api::resource dyncube_faceextents = {};              // structured buffer, 2x float4: [0]=(+X,+Y,+Z,mask) [1]=(-X,-Y,-Z,spare)
  reshade::api::resource_view dyncube_faceextents_uav = {};     // buffer UAV (reduction per-face extent write)
  reshade::api::resource dyncube_faceExtStaging = {};           // 32B gpu_to_cpu staging copy of extents
  reshade::api::resource dyncube_worldbox_scratch = {};      // structured buffer, groups*2 float4 partials (sized per cube size)
  uint64_t dyncube_worldbox_scratch_groups = 0u;  // pass-0 group capacity of the live scratch buffer
  reshade::api::resource_view dyncube_worldbox_scratch_srv = {}; // buffer SRV (pass-1 read)
  reshade::api::resource_view dyncube_worldbox_scratch_uav = {}; // buffer UAV (pass-0 write)
  reshade::api::pipeline_layout dyncube_worldbox_layout = {};
  reshade::api::pipeline dyncube_worldbox_pipeline = {};
  uint32_t dyncube_worldbox_layout_version = 0u;  // recreate layout/tables/pipeline when shape changes
  GTVBAODescriptorTableSet dyncube_worldbox_tables = {};
   bool dyncube_worldbox_reset_pending = true;
  // Delayed-validate commit (loading protection): consumers follow readSet, which
  // advances only to validated captures. Staging holds the latest bounds copy.
  reshade::api::resource dyncube_validStaging = {}; // 32B gpu_to_cpu staging copy of bounds
  uint32_t dyncube_readSet = 0;            // validated consumer set (t29/previews/aliases/filter input)
  uint32_t dyncube_filteredReadSet = 99u;  // readSet last fed into GGX filter (99 = none yet)
  bool dyncube_boxCopyPending = false;     // staged validity copy enqueued, not yet consumed
  bool dyncube_wasRejected = false;        // edge latch for reject/resume logging
  bool dyncube_rejectedGap = false;        // a capture was rejected since the last dispatched capture (one-shot: first accepted capture hard-replaces history)
  bool dyncube_loadingWipeDone = false;   // loading wipe already applied for the current stale episode (edge latch)
  bool dyncube_loadingWipePending = false;  // OnPresent saw a loading-stale episode: wipe on the next scheduler tick
  bool dyncube_captureDirty = false;          // capture-content settings changed since last filter: force one filter pass
  bool dyncube_dirtyFastForward = false;      // settings-dirty one-shot: next dispatched capture hard-replaces history
  float dyncube_lastVariantSoften = -1.f;      // soften value baked into the variant (-1 = none yet)
  float dyncube_lastVariantStrength = -1.f;    // strength value baked into the variant (-1 = none yet)
  float dyncube_lastCharCapture = -1.f;       // character-capture value fed into the last filter pass
  float dyncube_lastSparkleRejection = -1.f;
  bool dyncube_hasValidRead = false;       // any validated readSet exists (gates first filter)
  uint64_t dyncube_rejected_captures = 0;  // rejected (unpromoted) capture count
  // Phase 3 GGX prefilter � double-buffered filtered cube (Active/Building) so a
  // partially-written cube is never exposed to lighting.
  uint32_t dyncube_mip_count = 8;                        // computed mips (8 for 128..4096)
  reshade::api::resource dyncube_ggx_in = {};            // RGBA16F cube, N mips (GGX input chain)
  reshade::api::resource_view dyncube_ggx_in_cube_srv = {};
  reshade::api::resource dyncube_ggx_out[2] = {};        // RGBA16F cubes, N mips (filtered output)
  reshade::api::resource_view dyncube_ggx_out_cube_srv[2] = {};
  reshade::api::resource_view dyncube_ggx_out_mip_uav[2][8] = {};  // per-mip array UAVs (mips 1..N-1)
  uint32_t dyncube_ggx_active = 0;                       // index of the completed cube bound to t17
  bool dyncube_ggx_valid = false;                        // a completed filter has been produced
  // Multi-frame update scheduler (Capture -> Filter -> Done -> wait -> Capture).
  enum class DynCubePhase : uint32_t { Done = 0, Capture = 1, Filter = 2 };
  DynCubePhase dyncube_phase = DynCubePhase::Done;
  uint64_t dyncube_next_update_frame = 0;                // next frame a capture may begin
  uint64_t dyncube_sched_frame = UINT64_MAX;             // last present frame the scheduler ran
  // Actual GPU-work counters (prove capture/filter frequency; reported in the throttled log).
  uint64_t dyncube_capture_dispatches = 0;               // RunDynCubeCapture dispatched
  uint64_t dyncube_filter_updates = 0;                   // RunDynCubeFilter completed
  uint64_t dyncube_ggx_mip_dispatches = 0;               // GGX per-mip dispatches issued
  uint64_t dyncube_face_copies = 0;                      // 6-face mip0 copies issued
  reshade::api::pipeline_layout dyncube_ggx_layout = {};
  reshade::api::pipeline dyncube_ggx_pipeline = {};
  GTVBAODescriptorTableSet dyncube_ggx_tables = {};
  // Global-push variant cube (soften + strength for non-lighting t17 consumers).
  // Derived from ggx_out[active]; rebuilt on demand, never part of per-size cache.
  reshade::api::resource dyncube_variant = {};                // RGBA16F cube, N mips
  reshade::api::resource_view dyncube_variant_cube_srv = {};  // full-chain SRV (global serve)
  reshade::api::resource_view dyncube_variant_mip0_uav = {};  // mip0 array UAV (variant write)
  reshade::api::pipeline_layout dyncube_variant_layout = {};
  reshade::api::pipeline dyncube_variant_pipeline = {};
  GTVBAODescriptorTableSet dyncube_variant_tables = {};
  bool dyncube_variant_valid = false;          // variant matches current ggx_out + settings
  // Dedicated solid-color debug cube (Phase 0A). Never aliases history/ggx resources.
  reshade::api::resource dyncube_solid_cube = {};          // RGBA16F cube, 1 mip
  reshade::api::resource_view dyncube_solid_cube_srv = {};   // TextureCube SRV (t17 for debug 3)
  reshade::api::resource_view dyncube_solid_cube_uav = {};   // Texture2DArray UAV (solid write)
  // Simple SSR (screen-space ray march) � independent of history/ggx/inferred.
  reshade::api::resource dyncube_ssr_raw = {};          // RGBA16F 2D, march output (rgb=color, a=confidence)
  reshade::api::resource_view dyncube_ssr_raw_srv = {};
  reshade::api::resource_view dyncube_ssr_raw_uav = {};
  reshade::api::resource dyncube_ssr_blur_h = {};       // RGBA16F 2D, horizontal blur intermediate
  reshade::api::resource_view dyncube_ssr_blur_h_srv = {};
  reshade::api::resource_view dyncube_ssr_blur_h_uav = {};
  reshade::api::resource dyncube_ssr_blur = {};         // RGBA16F 2D, final blurred result (t31)
  reshade::api::resource_view dyncube_ssr_blur_srv = {};
  reshade::api::resource_view dyncube_ssr_blur_uav = {};
  reshade::api::pipeline_layout dyncube_ssr_layout = {};
  reshade::api::pipeline dyncube_ssr_pipeline = {};
  uint32_t dyncube_ssr_layout_version = 0u;  // recreate layout/tables/pipeline when shape changes
  GTVBAODescriptorTableSet dyncube_ssr_tables = {};
  reshade::api::pipeline_layout dyncube_ssr_blur_layout = {};
  reshade::api::pipeline dyncube_ssr_blur_pipeline = {};
  uint32_t dyncube_ssr_blur_layout_version = 0u;  // recreate layout/tables/pipeline when shape changes
  GTVBAODescriptorTableSet dyncube_ssr_blur_tables = {};
  uint32_t dyncube_pending_size = 0;                     // requested cube size, recreated at frame boundary
  bool dyncube_pending_recreate = false;                 // recreate (old set release deferred to Present)
  bool dyncube_pending_destroy = false;                  // feature disabled -> free the set at Present
  // -- RCAS post-TAA sharpening (Stage 1; motion sharpening is Stage 2) --
  // Flow per frame when enabled: TAA draw completes -> copy unsharpened TAA
  // output to rcas_hist (this copy alone is ever bound as next frame's t2) ->
  // RCAS compute reads TAA output, writes rcas_temp -> copy rcas_temp back
  // over the TAA output so downstream reads see sharpened pixels. History
  // isolation is structural: sharpened data never enters rcas_hist or t2.
  reshade::api::resource rcas_temp_texture = {};         // owned sharpened target (dims of TAA output, linear/UAV-legal variant format)
  reshade::api::resource_view rcas_temp_srv = {};        // (kept for debugging; not bound downstream)
  reshade::api::resource_view rcas_temp_uav = {};        // compute write target (u0)
  reshade::api::resource rcas_hist_texture = {};         // owned UNSHARPENED copy; sole source of overridden t2
  reshade::api::resource_view rcas_hist_srv = {};        // bound as t2 when RCAS serves history; also the dispatch input (t0)
  uint32_t rcas_w = 0u, rcas_h = 0u;                     // live dims of temp/hist set
  reshade::api::format rcas_fmt = reshade::api::format::unknown;  // live format (preserved, never converted)
  bool rcas_have_copy = false;                           // true once an unsharpened copy exists for t2 override
  bool rcas_format_logged = false;                       // one-time format log per resource set
  bool rcas_dispatch_logged = false;                     // one-time dispatch-shape log per resource set
  reshade::api::pipeline_layout rcas_layout = {};
  reshade::api::pipeline rcas_pipeline = {};
  uint32_t rcas_layout_version = 0u;
  std::array<reshade::api::descriptor_table, 2> rcas_tables = {};  // [0]=srv t0, [1]=uav u0
  reshade::api::resource_view rcas_last_rtv0 = {};            // latest bound RTV0 (D3D11 immediate-list assumption; sampled at TAA on_draw)
  reshade::api::resource_view rcas_motion_srv = {};          // game motion buffer (t3) captured from TAA draws (Stage 2)
  uint64_t rcas_motion_res = 0u;
  std::atomic<bool> rcas_motion_live{true};                  // destroy-event driven; false = buffer freed since capture
  // Motion vectors read from the GBuffer's fourth render target instead of the
  // TAA's t3. Same buffer, different bind point: the writers are
  // staticfoliage_0xF1EC53A8.ps_5_0.hlsl:201-207 and its 13 siblings, all
  // `o4.xy = jitterDiff_g.xy + (cur - prev) * vpSize_g`, i.e. pixel displacement
  // plus the jitter delta. That is exactly the convention MBGameMotionToUV and the
  // resolve pass already assume, so the two sources are interchangeable downstream
  // and this needs no shader change. The difference is availability, not data:
  // RTV4 is written by the GBuffer pass every frame, whereas t3 only exists once the
  // TAA draw has run.
  reshade::api::resource_view mb_rtv4_srv = {};              // GBuffer RTV4, captured from bind_render_targets
  uint64_t mb_rtv4_res = 0u;
  std::atomic<bool> mb_rtv4_live{true};
  bool mb_rtv4_warned_fmt = false;                           // one-shot format sanity warning
  bool mb_logged_rtv4_dims = false;                          // one-shot RTV4 dimensions log
  // OUR OWN shader-resource view on the motion resource, and nothing else.
  //
  // This is not a safety measure, it is a correctness one. ReShade's D3D11
  // update_descriptor_tables memcpy's the 64-bit handle straight into the
  // descriptor table with no type check, and the driver then consumes it as an
  // ID3D11ShaderResourceView. Handing it the ID3D11RenderTargetView that
  // OMSetRenderTargets gave us made it walk an RTV vtable as an SRV, which is a
  // bad indirect call: 0xC0000005 / BEX64, faulting module "unknown", the instant
  // the chain updated its first table. The TAA t3 source never showed this
  // because that view already IS an SRV.
  //
  // The chain threads one variable, motionSrc, to both places that read the
  // game's motion -- the resolve at t0 and the gather at t5 -- so making
  // motionSrc this view fixes both. No copy is involved: it is a view onto the
  // game's own motion resource, recreated only when that resource changes.
  reshade::api::resource_view mb_rtv4_owned_srv = {};
  // Set when the motion resource dies, so the owned view is destroyed and rebuilt
  // on the RENDER thread by MBMotionRtv4Ensure. It is deliberately not destroyed in
  // the destroy event itself: those run on the loader thread and every other
  // handler there is comment-documented as touching no view objects at all.
  bool mb_rtv4_owned_dirty = false;
  // First-wins-per-frame guard. Several passes bind exactly five render targets,
  // and the GBuffer's is only the FIRST of them, so a last-wins capture gets
  // clobbered by a later pass that happens to have something else in slot 4. This
  // mirrors captured_color_frame / captured_depth_frame, including the UINT64_MAX
  // initialiser so frame 0 captures instead of being skipped.
  uint64_t mb_rtv4_frame = UINT64_MAX;
  // Rejections allowed per frame, so a frame in which NO five-target bind is the
  // motion target cannot spin: each rejection re-arms the capture, and after the
  // cap the blur simply stays off for that frame.
  uint32_t mb_rtv4_rejects = 0u;
  static constexpr uint32_t kMotionBlurRtv4RejectCap = 4u;
  // -- FXAA post-TAA (TAA -> FXAA -> RCAS; never feeds TAA history) --
  // Layout/tables are shared by all three FXAA pipelines (same t0/t1/u0
  // shape); only the bytecode differs (luma prepass vs quality preset).
  reshade::api::resource fxaa_temp_texture = {};       // owned FXAA output (dims of TAA output, linear/UAV-legal variant format)
  reshade::api::resource_view fxaa_temp_srv = {};      // RCAS t0 when FXAA serves (or publish source when RCAS skips)
  reshade::api::resource_view fxaa_temp_uav = {};      // FXAA main write target (u0)
  reshade::api::resource fxaa_luma_texture = {};       // owned perceptual luma (R8_UNORM)
  reshade::api::resource_view fxaa_luma_srv = {};      // FXAA main luma input (t1)
  reshade::api::resource_view fxaa_luma_uav = {};      // luma prepass write target (u0)
  reshade::api::resource_view fxaa_hist_srv = {};      // linear-variant view of rcas_hist_texture (no sRGB decode); falls back to rcas_hist_srv
  bool fxaa_hist_srv_owned = false;                  // false when fxaa_hist_srv aliases rcas_hist_srv (do not destroy)
  reshade::api::resource fxaa_hist_seen = {};          // hist texture handle the views above were built from (recreate on change)
  uint32_t fxaa_w = 0u, fxaa_h = 0u;                   // live dims of the FXAA set
  reshade::api::format fxaa_fmt = reshade::api::format::unknown;  // live format
  bool fxaa_logged = false;                            // one-time log per resource set
  reshade::api::pipeline_layout fxaa_layout = {};
  reshade::api::pipeline fxaa_luma_pipeline = {};
  reshade::api::pipeline fxaa_standard_pipeline = {};
  reshade::api::pipeline fxaa_high_pipeline = {};
  uint32_t fxaa_layout_version = 0u;
  std::array<reshade::api::descriptor_table, 2> fxaa_tables = {};  // [0]=srv t0+t1, [1]=uav u0
  // -- Motion Blur (Guertin 2013), Sora 2nd --
  // Four owned resources: the two TileMax intermediates, NeighborMax, and the
  // gather output. There is no velocity or linear-depth surface � both are read
  // straight from the game's textures and converted in the consuming shader.
  reshade::api::resource mb_tilemax_h_texture = {};
  reshade::api::resource_view mb_tilemax_h_srv = {};
  reshade::api::resource_view mb_tilemax_h_uav = {};
  reshade::api::resource mb_tilemax_texture = {};
  reshade::api::resource_view mb_tilemax_srv = {};
  reshade::api::resource_view mb_tilemax_uav = {};
  reshade::api::resource mb_neighbormax_texture = {};
  reshade::api::resource_view mb_neighbormax_srv = {};
  reshade::api::resource_view mb_neighbormax_uav = {};
  reshade::api::resource mb_output_texture = {};       // gather result
  reshade::api::resource_view mb_output_srv = {};
  reshade::api::resource_view mb_output_uav = {};
  // Half-resolution chain, allocated ONLY while MotionBlurHalfRes is on so the
  // default path costs nothing. Both are ceil(W/2) x ceil(H/2).
  reshade::api::resource mb_half_color_texture = {};   // downsampled gather input
  reshade::api::resource_view mb_half_color_srv = {};
  reshade::api::resource_view mb_half_color_uav = {};
  reshade::api::resource mb_half_result_texture = {}; // half-res gather output
  reshade::api::resource_view mb_half_result_srv = {};
  reshade::api::resource_view mb_half_result_uav = {};
  // 1x1x1 stand-in bound at the gather's t4 when IS-FAST is off or missing, so
  // that slot is never a null descriptor.
  reshade::api::resource mb_noise_fallback_res = {};
  // Camera/object split, at the MOTION texture's resolution. .xy is the blended
  // velocity the tile chain reduces, .zw is camera-only for the velocity views.
  // This is the full-resolution velocity intermediate that Phase 2 removed, and
  // it only exists while the two channels disagree -- at equal weights the pass
  // is skipped and the game motion is bound directly instead.
  reshade::api::resource mb_resolve_texture = {};
  reshade::api::resource_view mb_resolve_srv = {};
  reshade::api::resource_view mb_resolve_uav = {};
  reshade::api::resource_view mb_noise_fallback_srv = {};
  // Point-clamp sampler for this chain's own descriptor table slot. Owned here
  // rather than borrowed from GTVBAO: point_clamp_sampler is created and
  // destroyed by CreateGTVBAOResources, so sharing its handle tied this chain's
  // lifetime to GTVBAO's, and a null handle there made sampling return zero.
  // The gather no longer samples at all, but the shared layout still declares
  // the slot, so it must be filled with a valid descriptor.
  reshade::api::sampler mb_point_clamp_sampler = {};
  std::array<reshade::api::pipeline_layout, kMotionBlurPassCount> mb_layouts = {};
  std::array<reshade::api::pipeline, kMotionBlurPassCount> mb_pipelines = {};
  std::array<GTVBAODescriptorTableSet, kMotionBlurPassCount> mb_tables = {};
  uint32_t mb_working_w = 0u, mb_working_h = 0u;
  uint32_t mb_motion_w = 0u, mb_motion_h = 0u;  // motion_h sizes tilemax_h; motion_w
                                                // sizes the camera/object resolve
                                                // output, which is full motion res
  uint32_t mb_tiles_x = 0u, mb_tiles_y = 0u;
  uint32_t mb_radius_px = 0u;
  // Last framerate-normalisation factor written to the log, so only real changes
  // are reported. -1 forces the first frame to log.
  float mb_last_frame_scale = -1.f;
  // Per-channel weights resolved once by the deploy gate, which is where the
  // cutscene signal lives, and pushed by PrepareMotionBlur.
  bool mb_halfres = false;         // half-res surfaces currently allocated
  reshade::api::format mb_output_fmt = reshade::api::format::unknown;
  bool mb_resources_ready = false;
  bool mb_warned_tonemap_src = false;
  // True once the prep passes have run at least once against the current
  // resource set, so "Gather Only" never dispatches against garbage.
  bool mb_prep_valid = false;
  bool mb_warned_gather_only = false;
  // One-shot report of the colour / motion / depth buffer dimensions. The only
  // way to confirm from outside the process whether the game's motion and depth
  // textures actually match the tonemap's resolution, which decides whether the
  // gather's filtered motion reads engage.
  bool mb_logged_dims = false;
  // One-shot: names which resolve bytecode variant is in the pipeline.
  bool mb_logged_resolve_variant = false;
  int mb_last_chain = -1;  // last chain value logged, so toggles are confirmed
  uint64_t mb_last_log_frame = 0u;
  // Deploy: the tonemap's t0, captured for both the gather's colour input and
  // the exact format the gather output must match before it can replace t0.
  reshade::api::resource_view mb_tonemap_src_srv = {};
  uint64_t mb_tonemap_src_res = 0u;
  std::atomic<bool> mb_tonemap_src_live{true};
  // Frame in which the Sora 2nd DoF gather last drew; the "Cutscene Only" gate.
  uint64_t mb_dof_drew_frame = UINT64_MAX;

  // -- Contact / Micro Shadows --
  // One rgba8_unorm target per technique, sized to the depth buffer. RGBA8_UNORM
  // is in the D3D11.0 guaranteed UAV type-write set; R8_UNORM and R16_FLOAT are
  // not, and a rejected UAV write leaves the target holding whatever was there
  // before, which reads as a plausible image rather than as a failure. The spare
  // channels carry the per-pixel diagnostic that makes the pass debuggable; see
  // the layout block in shadows/shadows_common.hlsli.
  reshade::api::resource micro_shadow_texture = {};
  reshade::api::resource_view micro_shadow_srv = {};
  reshade::api::resource_view micro_shadow_uav = {};
  reshade::api::resource contact_shadow_texture = {};
  reshade::api::resource_view contact_shadow_srv = {};
  reshade::api::resource_view contact_shadow_uav = {};
  // Owned by this chain. GTVBAO creates and destroys point_clamp_sampler inside
  // its own resource lifecycle, so borrowing it meant the shadow passes could not
  // dispatch at all with GTVBAO off -- a failure that looks exactly like "the
  // feature does nothing". Same reasoning as mb_point_clamp_sampler.
  reshade::api::sampler shadows_point_clamp_sampler = {};
  std::array<reshade::api::pipeline_layout, kShadowsPassCount> shadows_layouts = {};
  std::array<reshade::api::pipeline, kShadowsPassCount> shadows_pipelines = {};
  std::array<GTVBAODescriptorTableSet, kShadowsPassCount> shadows_tables = {};
  uint32_t shadows_w = 0u, shadows_h = 0u;
  bool shadows_resources_ready = false;
  // Frame that already dispatched, so the two hook points (the character pass and
  // the lighting pass, either of which can be the first consumer) cannot both run
  // the passes. Whichever draw comes first does the work; the later one only
  // consumes the result.
  uint64_t shadows_ran_frame = UINT64_MAX;
  // Frame of the last "blocked: <why>" line. The lighting draw fires several
  // times per frame, so without this the reason is reported once per draw.
  uint64_t shadows_log_frame = UINT64_MAX;
  bool shadows_logged_first_dispatch = false;
  // Which hook dispatched, reported once. Kai and Daybreak 2 run a character
  // lighting pass before the main lighting pass, and the answer decides whether
  // the char pass is reading a same-frame result or last frame's.
  int shadows_ran_from = -1;  // 0 = char lighting, 1 = main lighting
  bool shadows_logged_ao_fallback = false;
  bool shadows_logged_isfast_missing = false;
  // -- Custom TAA cross-addon slot ownership (compat with the falcomengine addon,
  // which replaces the same TAA hashes unconditionally) --
  // Replacement bytecode lives in one cross-addon shared slot per hash
  // (last registration wins), while on_draw/on_drawn callbacks stay
  // per-addon. This addon persistently owns the slot for the device lifetime:
  // Custom TAA ON selects the custom bytecode, while OFF selects this addon's
  // vanilla reference bytecode. The friend addon's bytes are never restored.
  bool taa_slot_claimed = false;
  uint32_t taa_claimed_hash = 0u;
  std::span<const uint8_t> taa_claimed_code = {};  // embed bytes currently in the slot (static storage)
  bool taa_other_addon_active = false;  // another payload was seen for a TAA hash
  // Per-pipeline deep clones of TAA pipeline subobjects, captured at
  // init_pipeline (handle known there) keyed by pipeline handle. Feeds the
  // draw-time rebuild, which cannot use details->subobjects (only stored by
  // shared infra when use_replace_async/use_shader_cache is set � neither
  // addon sets them). taa_subobjects_owned tracks handles whose tracked
  // details currently hold OUR clone (ownership for later replacement).
  std::unordered_map<uint64_t, std::pair<reshade::api::pipeline_subobject*, uint32_t>> taa_subobject_clones = {};
  std::unordered_set<uint64_t> taa_subobjects_owned = {};
  // Pipelines captured after the slot payload stabilized (level loads,
  // resolution changes, device resets): their creation-time build used
  // whatever the slot held before, so force one reset on the next TAA draw.
  std::unordered_set<uint64_t> taa_pending_resets = {};
};

static void CreateGTVBAOResources(reshade::api::device* device, DeviceData* data,
                                   uint32_t gw, uint32_t gh);
static void DestroyGTVBAOResources(reshade::api::device* device, DeviceData* data);
static bool CreateComputePipelinesIfNeeded(reshade::api::device* device, DeviceData* data);
static bool RunGTVBAO(reshade::api::command_list* cmd_list, DeviceData* data);
static bool LoadISFASTNoiseTexture(reshade::api::device* dev, DeviceData* d);
// -- Contact / Micro Shadows -- forward decls
static void CreateShadowsResources(reshade::api::device* dev, DeviceData* d,
                                   uint32_t w, uint32_t h);
static void DestroyShadowsResources(reshade::api::device* dev, DeviceData* d);
static bool CreateShadowsPipelinesIfNeeded(reshade::api::device* dev, DeviceData* d);
static bool RunShadows(reshade::api::command_list* cl, DeviceData* d, int fromHook);
static void ApplyGTVBAOCSDispatchFix(
    reshade::api::command_list* cmd_list,
    renodx::utils::state::CommandListState* cs,
    renodx::utils::state::CommandListState& prev);
// -- Dynamic Cubemaps � forward decls --
static bool CreateDynCubeResources(reshade::api::device* dev, DeviceData* d, uint32_t size);
static bool CreateDynCubeVariantResources(reshade::api::device* dev, DeviceData* d, uint32_t size, uint32_t mips);
static bool CreateDynCubeSolidResources(reshade::api::device* dev, DeviceData* d);
static bool CreateDynCubeGGXInResources(reshade::api::device* dev, DeviceData* d);
static void DestroyDynCubeResources(reshade::api::device* dev, DeviceData* d);
static uint64_t DynCubeEstimatedBytes(const DeviceData* d);
static void UnbindDynCubeComputeState(reshade::api::command_list* cl);
static bool CreateDynCubePipelinesIfNeeded(reshade::api::device* dev, DeviceData* d);
static bool RunDynCubeSolid(reshade::api::command_list* cl, DeviceData* d);
static bool RunDynCubeCapture(reshade::api::command_list* cl, DeviceData* d);
static bool RunDynCubeWorldBox(reshade::api::command_list* cl, DeviceData* d, uint32_t set);
static void PromoteDynCubeReadSet(DeviceData* d);
static void ConsumeDynCubeStagedValidity(reshade::api::device* dev, DeviceData* d);
static bool RunDynCubeInference(reshade::api::command_list* cl, DeviceData* d);
static bool RunDynCubeFilter(reshade::api::command_list* cl, DeviceData* d, bool ggxOn);
static bool RunDynCubeVariant(reshade::api::command_list* cl, DeviceData* d);
static bool RunDynCubeSSR(reshade::api::command_list* cl, DeviceData* d);

// VBGI is now integrated into GTVBAO main pass � no separate RunVBGI needed.
static bool OnBeforeLightingShaderDraw(reshade::api::command_list* cmd_list);
static bool OnBeforeSoraSSR1Draw(reshade::api::command_list* cmd_list);
static bool OnReplaceSoraSSR1Draw(reshade::api::command_list* cmd_list);
static bool OnBeforeSoraSSR2Draw(reshade::api::command_list* cmd_list);
static bool OnReplaceSoraSSR2Draw(reshade::api::command_list* cmd_list);
static bool OnBeforeSora1stSSRDraw(reshade::api::command_list* cmd_list);
static bool OnReplaceSora1stSSRDraw(reshade::api::command_list* cmd_list);
// -- RCAS post-TAA sharpening � forward decls --
static void CreateRCASResources(reshade::api::device* dev, DeviceData* d,
                                uint32_t w, uint32_t h, reshade::api::format fmt);
static void DestroyRCASResources(reshade::api::device* dev, DeviceData* d);
static bool CreateRCASPipelineIfNeeded(reshade::api::device* dev, DeviceData* d);
static void CreateFXAAResources(reshade::api::device* dev, DeviceData* d,
                                uint32_t w, uint32_t h, reshade::api::format fmt);
static void DestroyFXAAResources(reshade::api::device* dev, DeviceData* d);
static bool CreateFXAAPipelineIfNeeded(reshade::api::device* dev, DeviceData* d);
static void DestroyMotionBlurResources(reshade::api::device* dev, DeviceData* d);
static bool OnBeforeCustomTAADraw(reshade::api::command_list* cmd_list);
static bool OnReplaceCustomTAADraw(reshade::api::command_list* cmd_list);
static void EnsureTAAPayload(reshade::api::device* dev, uint32_t hash, std::span<const uint8_t> desired, std::span<const uint8_t> alternate);
static void OnInitPipelineCapture(
    reshade::api::device* device,
    reshade::api::pipeline_layout layout,
    uint32_t subobject_count,
    const reshade::api::pipeline_subobject* subobjects,
    reshade::api::pipeline pipeline);
static void OnDestroyPipelineCapture(reshade::api::device* device, reshade::api::pipeline pipeline);
// Forces the draw-time machinery to rebuild the replacement on the next draw
// for every tracked pipeline whose hashes contain `hash` and for which we
// captured init-time subobjects. The rebuild (stock BuildReplacementPipeline)
// clones details->subobjects, which shared infra only stores when
// use_replace_async/use_shader_cache is set (neither addon sets them), so the
// per-handle clones captured below are assigned first; without them the
// rebuild would index an empty array (heap corruption � the startup crash).
// Two-phase (collect handles under a read lock, then reset per handle) so no
// map lock is held across operations. The orphaned replacement pipeline is
// deliberately NOT destroyed here (it may still be bound on this list);
// its tracking entry is dropped for handle-reuse safety and the object leaks
// bounded (~1 per toggle transition) until device/process teardown.
static void ResetTAAReplacementPipelines(reshade::api::device* dev, uint32_t hash) {
  if (!dev || hash == 0u) return;
  if (renodx::utils::shader::shared.data == nullptr) return;
  auto* d = dev->get_private_data<DeviceData>();
  if (!d) return;
  std::vector<uint64_t> stale;
  renodx::utils::shader::shared.data->pipeline_shader_details.for_each(
      [&](const auto& pair) {
        const auto& details = pair.second;
        if (details.device != dev) return;
        if (details.shader_hashes.find(hash) == details.shader_hashes.end()) return;
        stale.push_back(pair.first);
      });
  for (uint64_t handle : stale) {
    auto cit = d->taa_subobject_clones.find(handle);
    if (cit == d->taa_subobject_clones.end()) continue;  // no safe source: leave stale (today's behavior)
    reshade::api::pipeline orphan = {0u};
    renodx::utils::shader::shared.data->pipeline_shader_details.modify_if(
        handle,
        [&](std::pair<const uint64_t, renodx::utils::shader::PipelineShaderDetails>& pair) {
          auto& details = pair.second;
          if (details.subobjects.empty()) {
            auto* fresh = renodx::utils::pipeline::ClonePipelineSubObjects(
                cit->second.first, cit->second.second);
            if (!fresh) return;  // leave flags: a flag-only reset would rebuild from nothing
            details.subobjects.assign(fresh, fresh + cit->second.second);
            delete[] fresh;
            d->taa_subobjects_owned.insert(handle);
          } else if (d->taa_subobjects_owned.contains(handle)) {
            // Replace our previous clone (stock never populates these when
            // async/cache are off; when on (devkit), stock owns non-empty
            // entries we must not free � those handles are never in owned).
            renodx::utils::pipeline::DestroyPipelineSubobjects(details.subobjects);
            auto* fresh = renodx::utils::pipeline::ClonePipelineSubObjects(
                cit->second.first, cit->second.second);
            if (!fresh) {
              details.subobjects.clear();
              d->taa_subobjects_owned.erase(handle);
              return;
            }
            details.subobjects.assign(fresh, fresh + cit->second.second);
            delete[] fresh;
          }
          // else: stock-populated (devkit async/cache): leave descs, just reset below.
          orphan = details.replacement_pipeline;
          details.replacement_pipeline = {0u};
          details.initialized_replacement = false;
        });
    // Outside the map lock (erase re-enters the map): drop the orphaned
    // replacement's tracking entry. The object itself is deliberately not
    // destroyed (may still be bound); bounded leak until teardown.
    if (orphan.handle != 0u) {
      renodx::utils::shader::shared.data->pipeline_shader_details.erase_if(
          orphan.handle, [](const auto&) { return true; });
      // The orphan may itself have been captured at init (friend-bytecode
      // rebuilds match the inverse fallback): drop that clone too.
      auto oit = d->taa_subobject_clones.find(orphan.handle);
      if (oit != d->taa_subobject_clones.end()) {
        renodx::utils::pipeline::DestroyPipelineSubobjects(oit->second.first, oit->second.second);
        d->taa_subobject_clones.erase(oit);
        d->taa_subobjects_owned.erase(orphan.handle);
      }
    }
  }
}

// Captures a deep clone of TAA pipeline subobjects at init time (the pipeline
// handle is known here; original descs die with the event return). Matched on
// the vanilla TAA hashes, with an inverse-map fallback for pipelines already
// baked by another addon (load-order robustness).
static void OnInitPipelineCapture(
    reshade::api::device* device,
    reshade::api::pipeline_layout /*layout*/,
    uint32_t subobject_count,
    const reshade::api::pipeline_subobject* subobjects,
    reshade::api::pipeline pipeline) {
  if (!device || !subobjects || subobject_count == 0u || pipeline.handle == 0u) return;
  auto* d = device->get_private_data<DeviceData>();
  if (!d) return;
  static const uint32_t kTAAHashes[2] = {0xFA37EA04u, 0x9D91FAC3u};
  bool match = false;
  for (uint32_t i = 0; i < subobject_count && !match; ++i) {
    if (subobjects[i].type != reshade::api::pipeline_subobject_type::pixel_shader) continue;
    auto* desc = static_cast<const reshade::api::shader_desc*>(subobjects[i].data);
    if (!desc || desc->code_size == 0u) continue;
    const uint32_t h = renodx::utils::hash::ComputeCRC32(
        static_cast<const uint8_t*>(desc->code), desc->code_size);
    for (uint32_t want : kTAAHashes) {
      if (h == want) { match = true; break; }
    }
    if (!match && renodx::utils::shader::shared.data != nullptr) {
      renodx::utils::shader::shared.data->shader_replacements_inverse.if_contains(
          std::pair<reshade::api::device*, uint32_t>{device, h},
          [&](const auto& pair) {
            for (uint32_t want : kTAAHashes) {
              if (pair.second == want) { match = true; break; }
            }
          });
    }
  }
  if (!match) return;
  auto it = d->taa_subobject_clones.find(pipeline.handle);
  if (it != d->taa_subobject_clones.end()) {
    renodx::utils::pipeline::DestroyPipelineSubobjects(it->second.first, it->second.second);
    d->taa_subobject_clones.erase(it);
    d->taa_subobjects_owned.erase(pipeline.handle);
  }
  reshade::api::pipeline_subobject* clone =
      renodx::utils::pipeline::ClonePipelineSubObjects(subobjects, subobject_count);
  if (!clone) return;
  d->taa_subobject_clones.emplace(pipeline.handle, std::make_pair(clone, subobject_count));
  // If the slot payload is already owned, this pipeline's creation-time build
  // may have used stale bytes: schedule one reset on the next TAA draw.
  if (d->taa_slot_claimed) {
    d->taa_pending_resets.insert(pipeline.handle);
  }
}

// Frees a stored subobject clone when its pipeline dies (bounds the store
// across level loads) and drops any ownership tracking for the handle.
static void OnDestroyPipelineCapture(reshade::api::device* device, reshade::api::pipeline pipeline) {
  if (!device || pipeline.handle == 0u) return;
  auto* d = device->get_private_data<DeviceData>();
  if (!d) return;
  auto it = d->taa_subobject_clones.find(pipeline.handle);
  if (it == d->taa_subobject_clones.end()) return;
  renodx::utils::pipeline::DestroyPipelineSubobjects(it->second.first, it->second.second);
  d->taa_subobject_clones.erase(it);
  d->taa_subobjects_owned.erase(pipeline.handle);
  d->taa_pending_resets.erase(pipeline.handle);
}
static void OnDrawnCustomTAA(reshade::api::command_list* cmd_list);
static bool OnBeforeKaiSSRDraw(reshade::api::command_list* cmd_list);
static bool OnReplaceKaiSSRDraw(reshade::api::command_list* cmd_list);
static bool OnBeforeSsaoShaderDraw(reshade::api::command_list* cmd_list);
static bool OnBeforeCharLightingDraw(reshade::api::command_list* cmd_list);
static bool OnBeforeKaiVolFogDraw(reshade::api::command_list* cmd_list);
static void OnPushDescriptorsCapture(reshade::api::command_list* cmd_list,
    reshade::api::shader_stage stages, reshade::api::pipeline_layout layout,
    uint32_t param_index, const reshade::api::descriptor_table_update& update);

// -- Custom Shader crash-tracing log (defined before OnPresent; used everywhere) --
static void CSLog(const char* tag, const std::string& msg, bool warn = false);
static std::string CSViewDims(reshade::api::device* dev, reshade::api::resource_view v);
static bool DynCubeSceneLive(const DeviceData* d);
struct CapturedViewInfo { uint64_t res; std::string dims; uint32_t w; uint32_t h; };
static CapturedViewInfo CSResolveCapture(reshade::api::device* dev, reshade::api::resource_view v);

// -- IS-FAST sync helpers (sync g_isfast_* globals ? shader_injection) --
static void SyncISFASTToShaderInjection(reshade::api::command_list* cmd_list) {
  shader_injection.shadow_isfast_enabled = g_isfast_enabled;
  shader_injection.shadow_isfast_spatial_scale = g_isfast_spatial_scale;
  shader_injection.shadow_isfast_temporal_speed = g_isfast_temporal_speed;
  shader_injection.shadow_isfast_seed_offset = g_isfast_seed_offset;
  if (auto* dev = cmd_list->get_device()) {
    if (auto* d = dev->get_private_data<DeviceData>()) {
      shader_injection.shadow_isfast_texture_loaded = d->isfast_texture_loaded ? 1.f : 0.f;
    }
  }
}

// -- Shadow draw callbacks (sync IS-FAST + push IS-FAST SRV at t3) --
static bool OnBeforeShadowCSMDraw(reshade::api::command_list* cmd_list) {
  SyncISFASTToShaderInjection(cmd_list);
  // Push IS-FAST noise texture at t3 (same pattern as t22 in lighting shader)
  if (g_isfast_enabled > 0.5f) {
    if (auto* dev = cmd_list->get_device()) {
      if (auto* d = dev->get_private_data<DeviceData>()) {
        reshade::api::resource_view srv = d->isfast_noise_srv.handle
            ? d->isfast_noise_srv : d->fallback_srv;
        if (srv.handle) {
          cmd_list->push_descriptors(
              reshade::api::shader_stage::pixel,
              reshade::api::pipeline_layout{0},
              0,
              reshade::api::descriptor_table_update{
                  {}, 3u, 0, 1,
                  reshade::api::descriptor_type::texture_shader_resource_view,
                  &srv,
              });
        }
      }
    }
  }
  return true;
}

static bool OnBeforeShadowBlurDraw(reshade::api::command_list* cmd_list) {
  SyncISFASTToShaderInjection(cmd_list);
  return true;
}

// -- Volfog IS-FAST sync + push IS-FAST SRV at t3 --
static void SyncVolFogISFASTToShaderInjection(reshade::api::command_list* cmd_list) {
  shader_injection.volfog_isfast_spatial_scale = g_isfast_spatial_scale;
  if (auto* dev = cmd_list->get_device()) {
    if (auto* d = dev->get_private_data<DeviceData>()) {
      shader_injection.volfog_isfast_texture_loaded = d->isfast_texture_loaded ? 1.f : 0.f;
    }
  }
  // Derive effective IS-FAST flag from master + volfog toggle
  shader_injection.volfog_isfast_enabled =
      g_isfast_enabled >= 0.5f
      && shader_injection.volfog_jitter_enabled >= 0.5f
      && shader_injection.volfog_jitter_amount > 0.0001f
      ? 1.f : 0.f;
}

static bool OnBeforeVolFogDraw(reshade::api::command_list* cmd_list) {
  SyncVolFogISFASTToShaderInjection(cmd_list);
  // Push IS-FAST noise texture at t3 (same pattern as shadow shader at t3)
  if (g_isfast_enabled > 0.5f) {
    if (auto* dev = cmd_list->get_device()) {
      if (auto* d = dev->get_private_data<DeviceData>()) {
        reshade::api::resource_view srv = d->isfast_noise_srv.handle
            ? d->isfast_noise_srv : d->fallback_srv;
        if (srv.handle) {
          cmd_list->push_descriptors(
              reshade::api::shader_stage::pixel,
              reshade::api::pipeline_layout{0}, 0,
              reshade::api::descriptor_table_update{
                  {}, 3u, 0, 1,
                  reshade::api::descriptor_type::texture_shader_resource_view,
                  &srv,
              });
        }
      }
    }
  }
  return true;
}

static bool OnBeforeKaiVolFogDraw(reshade::api::command_list* cmd_list) {
  SyncVolFogISFASTToShaderInjection(cmd_list);
  // Sync Sora volfog settings ? Kai volfog fields
  shader_injection.volfog_tricubic_enabled = shader_injection.volfog_haze_aa_mode;
  shader_injection.volfog_is_fast_enabled = shader_injection.volfog_isfast_enabled;
  // Note: volfog_color_correction_strength is bound to Fog3DCorrectionStrength setting;
  // do NOT overwrite it with the 2D fog_color_correction_strength.
  if (auto* dev = cmd_list->get_device()) {
    if (auto* d = dev->get_private_data<DeviceData>()) {
      shader_injection.isfast_noise_bound = d->isfast_texture_loaded ? 1.f : 0.f;
    }
  }
  // Push IS-FAST noise texture at t15 (Kai's volfog register)
  if (g_isfast_enabled > 0.5f) {
    if (auto* dev = cmd_list->get_device()) {
      if (auto* d = dev->get_private_data<DeviceData>()) {
        reshade::api::resource_view srv = d->isfast_noise_srv.handle
            ? d->isfast_noise_srv : d->fallback_srv;
        if (srv.handle) {
          cmd_list->push_descriptors(
              reshade::api::shader_stage::pixel,
              reshade::api::pipeline_layout{0}, 0,
              reshade::api::descriptor_table_update{
                  {}, 15u, 0, 1,
                  reshade::api::descriptor_type::texture_shader_resource_view,
                  &srv,
              });
        }
      }
    }
  }
  return true;
}

// -- Contact / Micro Shadows: shared deploy step --
// The two techniques are consumed by BOTH a character lighting pass and a main
// lighting pass. Whichever draw happens first in the frame dispatches the compute
// passes; the other only reads the result. Doing it this way means the frame does
// not depend on the relative order of those two draws, which differs between
// games and is not something the addon should have to encode.
//
// `fromHook` is 0 for the character pass and 1 for the main lighting pass; it is
// only used to report which one won.
//
// The two bound flags are reset at the top of every call, before any early-out,
// because the lighting shader reads them to decide whether to sample at all. A
// flag left set from an earlier frame would make it sample a texture that no
// longer matches the current resolution.
static void DeployShadows(reshade::api::command_list* cmd_list, int fromHook) {
  shader_injection.cs_micro_dedicated_bound = 0.f;
  shader_injection.cs_contact_dedicated_bound = 0.f;
  if (!cmd_list) return;
  auto* dev = cmd_list->get_device();
  auto* d = dev ? dev->get_private_data<DeviceData>() : nullptr;
  if (!d) return;

  const bool micro_on = shader_injection.cs_micro_enabled > 0.5f;
  const bool contact_on = shader_injection.cs_contact_enabled > 0.5f;
  if (!micro_on && !contact_on) return;

  // The shadow grid is the DEPTH buffer's grid: both passes read depth, and both
  // sample the G-buffer on the same texels. Sizing the output to anything else
  // would put the result on a different texel than the geometry it describes.
  // captured_depth_w/h is cached at capture time, so no GPU query is needed here.
  if (d->captured_depth_w == 0u || d->captured_depth_h == 0u) {
    if (d->shadows_log_frame != d->frame_index) {
      d->shadows_log_frame = d->frame_index;
      reshade::log::message(reshade::log::level::warning,
          "[Shadows] blocked: scene depth has not been captured yet (no lighting "
          "draw has pushed its depth register this frame).");
      CSLog("shadows", "blocked: depth not captured yet");
    }
    return;
  }
  if (d->shadows_w != d->captured_depth_w || d->shadows_h != d->captured_depth_h
      || !d->shadows_resources_ready) {
    CreateShadowsResources(dev, d, d->captured_depth_w, d->captured_depth_h);
  }
  // CreateShadowsResources logs which call failed; repeating it here would only
  // add noise on a channel that is already reported.
  if (!d->shadows_resources_ready) return;

  auto* cs = renodx::utils::state::GetCurrentState(cmd_list);
  renodx::utils::state::CommandListState prev = {};
  if (cs) prev = *cs;
  const bool ok = RunShadows(cmd_list, d, fromHook);
  // Restore the game's compute state either way: leaving our SRVs and UAVs bound
  // at the compute slots makes the engine's own next dispatch read them, which is
  // the "double volumetrics" artifact the deferred-dispatch setting documents.
  ApplyGTVBAOCSDispatchFix(cmd_list, cs, prev);
  if (!ok) return;

  if (micro_on && d->micro_shadow_srv.handle) {
    cmd_list->push_descriptors(
        reshade::api::shader_stage::pixel, reshade::api::pipeline_layout{0}, 0,
        reshade::api::descriptor_table_update{
            {}, kLightingMicroShadowRegister, 0, 1,
            reshade::api::descriptor_type::texture_shader_resource_view,
            &d->micro_shadow_srv});
    shader_injection.cs_micro_dedicated_bound = 1.f;
  }
  if (contact_on && d->contact_shadow_srv.handle) {
    cmd_list->push_descriptors(
        reshade::api::shader_stage::pixel, reshade::api::pipeline_layout{0}, 0,
        reshade::api::descriptor_table_update{
            {}, kLightingContactShadowRegister, 0, 1,
            reshade::api::descriptor_type::texture_shader_resource_view,
            &d->contact_shadow_srv});
    shader_injection.cs_contact_dedicated_bound = 1.f;
  }

  // The local-light march runs inside the dynamic light loop rather than in a
  // pass, so it needs the two resources a pass would have supplied: the depth
  // buffer and the noise volume. Both go to slots the lighting shaders declare
  // for the shadow path alone (t35/t36), never to the game's own registers.
  if (contact_on && shader_injection.cs_contact_local_enabled > 0.5f) {
    if (d->isfast_noise_srv.handle) {
      cmd_list->push_descriptors(
          reshade::api::shader_stage::pixel, reshade::api::pipeline_layout{0}, 0,
          reshade::api::descriptor_table_update{
              {}, kLightingShadowNoiseRegister, 0, 1,
              reshade::api::descriptor_type::texture_shader_resource_view,
              &d->isfast_noise_srv});
    }
    if (d->captured_depth_srv.handle) {
      cmd_list->push_descriptors(
          reshade::api::shader_stage::pixel, reshade::api::pipeline_layout{0}, 0,
          reshade::api::descriptor_table_update{
              {}, kLightingShadowDepthRegister, 0, 1,
              reshade::api::descriptor_type::texture_shader_resource_view,
              &d->captured_depth_srv});
    }
  }
}

// -- Kai + Daybreak 2 character lighting callback (Env SSS + Character Shadowing) --
static bool OnBeforeCharLightingDraw(reshade::api::command_list* cmd_list) {
  // The character pass can be the first consumer of the frame, so it may be the
  // one that has to run the shadow compute passes. On games without a character
  // pass this is a no-op and the main lighting draw does the work.
  // Sync first: RunShadows and the local-light march both read the IS-FAST
  // mirrors, and this hook would otherwise be the only one that can see them
  // stale.
  SyncISFASTToShaderInjection(cmd_list);
  DeployShadows(cmd_list, 0);
  // Character shader reads shader_injection_data automatically via b13 injection.
  // Push IS-FAST noise at t15 (Kai char shader uses it; Daybreak 2 char does not).
  if (!IsDaybreak2() && g_isfast_enabled > 0.5f) {
    if (auto* dev = cmd_list->get_device()) {
      if (auto* d = dev->get_private_data<DeviceData>()) {
        reshade::api::resource_view srv = d->isfast_noise_srv.handle
            ? d->isfast_noise_srv : d->fallback_srv;
        if (srv.handle) {
          cmd_list->push_descriptors(
              reshade::api::shader_stage::pixel,
              reshade::api::pipeline_layout{0}, 0,
            reshade::api::descriptor_table_update{
                {}, 15u, 0, 1,
                reshade::api::descriptor_type::texture_shader_resource_view,
                &srv,
            });
        }
      }
    }
  }
  return true;
}

// -- Foliage draw tracking (for GTVBAO foliage exclusion performance) --
static bool OnBeforeFoliageDraw(reshade::api::command_list* cmd_list) {
  auto* device = cmd_list->get_device();
  auto* data = device->get_private_data<DeviceData>();
  if (data) data->foliage_drawn_this_frame = true;
  return true;
}

// -- DOF gather (Improved) draw: IS-FAST rotation --
// The gather shaders sample only t0, so t5 is free for the IS-FAST volume.
// Mirrors the Vanilla-SSR IS-FAST path exactly (same volume, same frame
// slice, same "frame < 0 means unusable" gating), so the noise the bokeh
// gathers match the noise the other effects already use.
static bool OnBeforeDofGatherDraw(reshade::api::command_list* cmd_list) {
  auto* dev = cmd_list->get_device();
  if (!dev) return true;
  auto* d = dev->get_private_data<DeviceData>();
  if (!d) return true;
  const bool noiseUsable = g_isfast_enabled > 0.5f
      && shader_injection.dof_isfast_enabled > 0.5f
      && shader_injection.dof_mode >= 0.5f
      && d->isfast_noise_srv.handle != 0u;
  // -1 disables sampling in the shader, which keeps the gather deterministic.
  shader_injection.dof_isfast_noise_frame = noiseUsable
      ? (float)(d->frame_index % 64u) : -1.f;
  if (noiseUsable) {
    reshade::api::resource_view srv = d->isfast_noise_srv;
    cmd_list->push_descriptors(
        reshade::api::shader_stage::pixel,
        reshade::api::pipeline_layout{0}, 0,
        reshade::api::descriptor_table_update{
            {}, 5u, 0, 1,
            reshade::api::descriptor_type::texture_shader_resource_view,
            &srv});
  }
  return true;
}

// -- Motion Blur callbacks (implemented in the Motion Blur section) --
static void OnDrawnDofGather(reshade::api::command_list* cmd_list);
static bool OnBeforeTonemapDraw(reshade::api::command_list* cmd_list);

// ----------- Custom shaders -----------

renodx::mods::shader::CustomShaders custom_shaders = {
    {
        0x954D3D6Du,
        renodx::mods::shader::CustomShader{
            .crc32 = 0x954D3D6Du,
            .code = __0x954D3D6D,
            .on_draw = OnBeforeVolFogDraw,
        },
    },
    {
        0x79359F5Cu,
        renodx::mods::shader::CustomShader{
            .crc32 = 0x79359F5Cu,
            .code = __0x79359F5C,
            .on_draw = OnBeforeShadowCSMDraw,
        },
    },
    {
        0x55E4FE42u,
        renodx::mods::shader::CustomShader{
            .crc32 = 0x55E4FE42u,
            .code = __0x55E4FE42,
            .on_draw = OnBeforeShadowBlurDraw,
        },
    },
    // -- Sora 2nd shadow pipeline --
    {
        0xF320152Cu,
        renodx::mods::shader::CustomShader{
            .crc32 = 0xF320152Cu,
            .code = __0xF320152C,
            .on_draw = OnBeforeShadowCSMDraw,
        },
    },
    {
        0xF1575FE3u,
        renodx::mods::shader::CustomShader{
            .crc32 = 0xF1575FE3u,
            .code = __0xF1575FE3,
            .on_draw = OnBeforeShadowBlurDraw,
        },
    },
    {
        0xCA3D8596u,
        renodx::mods::shader::CustomShader{
            .crc32 = 0xCA3D8596u,
            .code = __0xCA3D8596,
            .on_draw = OnBeforeLightingShaderDraw,
        },
    },
    // -- Sora 2nd SSR (replacement-gated; vanilla when the toggle is off) --
    // NOTE: ssr1 runs the vanilla march (composite input when replacing; skipped
    // only when replacement is on and Game SSR is off). ssr2 runs the DynCube
    // composite under the gate; otherwise the game draws vanilla.
    {
        0xE2F406C7u,
        renodx::mods::shader::CustomShader{
            .crc32 = 0xE2F406C7u,
            .code = __0xE2F406C7,
            .on_replace = OnReplaceSoraSSR1Draw,
            .on_draw = OnBeforeSoraSSR1Draw,
        },
    },
    {
        0x17F931DEu,
        renodx::mods::shader::CustomShader{
            .crc32 = 0x17F931DEu,
            .code = __0x17F931DE,
            .on_replace = OnReplaceSoraSSR2Draw,
            .on_draw = OnBeforeSoraSSR2Draw,
        },
    },
    // -- Sora 1st SSR (fused march + temporal, replacement-gated) --
    // NOTE: the single pass runs the vanilla march inline and temporally filters
    // it; the composite resolves march > dynamic > miss (no vanilla cube on this
    // path) and keeps the vanilla temporal stage. Otherwise the game draws vanilla.
    {
        0x8B35370Au,
        renodx::mods::shader::CustomShader{
            .crc32 = 0x8B35370Au,
            .code = __0x8B35370A,
            .on_replace = OnReplaceSora1stSSRDraw,
            .on_draw = OnBeforeSora1stSSRDraw,
        },
    },
    // -- Custom TAA (Sora 1st/2nd; vanilla baseline, custom when enabled) --
    // NOTE: custom files use unique embed stems (taa_custom_sora1st/2nd) so the
    // dumped vanilla reference files under sora1st/taa and sora2nd/taa keep
    // compiling untouched; the runtime CRCs below are the game's TAA hashes.
    // The initial slot payload is vanilla. OnBeforeCustomTAADraw selects the
    // custom payload when enabled and reasserts vanilla when disabled.
    {
        0xFA37EA04u,
        renodx::mods::shader::CustomShader{
            .crc32 = 0xFA37EA04u,
            .code = __0xFA37EA04,
            .on_replace = OnReplaceCustomTAADraw,
            .on_draw = OnBeforeCustomTAADraw,
            .on_drawn = OnDrawnCustomTAA,
        },
    },
    {
        0x9D91FAC3u,
        renodx::mods::shader::CustomShader{
            .crc32 = 0x9D91FAC3u,
            .code = __0x9D91FAC3,
            .on_replace = OnReplaceCustomTAADraw,
            .on_draw = OnBeforeCustomTAADraw,
            .on_drawn = OnDrawnCustomTAA,
        },
    },
    // -- Kai SSR (fused march + temporal, replacement-gated; High + Ultra) --
    // NOTE: both quality variants run the vanilla march inline and temporally
    // filter it; each composite resolves march > dynamic > miss (no vanilla
    // cube on this path) and keeps the vanilla temporal stage. Only one variant
    // runs at a time. Otherwise the game draws vanilla.
    {
        0xA1668427u,
        renodx::mods::shader::CustomShader{
            .crc32 = 0xA1668427u,
            .code = __0xA1668427,
            .on_replace = OnReplaceKaiSSRDraw,
            .on_draw = OnBeforeKaiSSRDraw,
        },
    },
    {
        0x209125C1u,
        renodx::mods::shader::CustomShader{
            .crc32 = 0x209125C1u,
            .code = __0x209125C1,
            .on_replace = OnReplaceKaiSSRDraw,
            .on_draw = OnBeforeKaiSSRDraw,
        },
    },
    CustomShaderEntryCallback(0x485E0022, OnBeforeSsaoShaderDraw),
    // -- Sora 2nd ssao (GTVBAO gate) --
    CustomShaderEntryCallback(0x752B2580, OnBeforeSsaoShaderDraw),
    // -- Sora foliage (GTVBAO foliage marker bit 15 in o1.w) --
    {
        0x76E6E95Eu,
        renodx::mods::shader::CustomShader{
            .crc32 = 0x76E6E95Eu,
            .code = __0x76E6E95E,
            .on_draw = OnBeforeFoliageDraw,
        },
    },
    {
        0x39F91AE8u,
        renodx::mods::shader::CustomShader{
            .crc32 = 0x39F91AE8u,
            .code = __0x39F91AE8,
            .on_draw = OnBeforeFoliageDraw,
        },
    },
    // -- Sora clutter/flower foliage --
    {
        0xb03759DAu,
        renodx::mods::shader::CustomShader{
            .crc32 = 0xB03759DAu,
            .code = __0xB03759DA,
            .on_draw = OnBeforeFoliageDraw,
        },
    },
    {
        0xf96f9811u,
        renodx::mods::shader::CustomShader{
            .crc32 = 0xF96F9811u,
            .code = __0xF96F9811,
            .on_draw = OnBeforeFoliageDraw,
        },
    },
    // -- Sora potflower foliage --
    {
        0xd9becfb1u,
        renodx::mods::shader::CustomShader{
            .crc32 = 0xD9BECFB1u,
            .code = __0xD9BECFB1,
            .on_draw = OnBeforeFoliageDraw,
        },
    },
    // -- Sora 2nd foliage (GTVBAO foliage marker bit 15 in o1.w) --
    {
        0x46FCDC51u,
        renodx::mods::shader::CustomShader{
            .crc32 = 0x46FCDC51u,
            .code = __0x46FCDC51,
            .on_draw = OnBeforeFoliageDraw,
        },
    },
    {
        0x5C33E765u,
        renodx::mods::shader::CustomShader{
            .crc32 = 0x5C33E765u,
            .code = __0x5C33E765,
            .on_draw = OnBeforeFoliageDraw,
        },
    },
    {
        0xF6733CDDu,
        renodx::mods::shader::CustomShader{
            .crc32 = 0xF6733CDDu,
            .code = __0xF6733CDD,
            .on_draw = OnBeforeFoliageDraw,
        },
    },
    {
        0x533C1853u,
        renodx::mods::shader::CustomShader{
            .crc32 = 0x533C1853u,
            .code = __0x533C1853,
            .on_draw = OnBeforeFoliageDraw,
        },
    },
    {
        0xF1EC53A8u,
        renodx::mods::shader::CustomShader{
            .crc32 = 0xF1EC53A8u,
            .code = __0xF1EC53A8,
            .on_draw = OnBeforeFoliageDraw,
        },
    },
    {
        0x2F107485u,
        renodx::mods::shader::CustomShader{
            .crc32 = 0x2F107485u,
            .code = __0x2F107485,
            .on_draw = OnBeforeFoliageDraw,
        },
    },
    {
        0x37C0064Bu,
        renodx::mods::shader::CustomShader{
            .crc32 = 0x37C0064Bu,
            .code = __0x37C0064B,
            .on_draw = OnBeforeFoliageDraw,
        },
    },
    {
        0xA8291F30u,
        renodx::mods::shader::CustomShader{
            .crc32 = 0xA8291F30u,
            .code = __0xA8291F30,
            .on_draw = OnBeforeFoliageDraw,
        },
    },
    // -- Kai foliage (GTVBAO foliage marker) --
    {
        0x534E54EAu,
        renodx::mods::shader::CustomShader{
            .crc32 = 0x534E54EAu,
            .code = __0x534E54EA,
            .on_draw = OnBeforeFoliageDraw,
        },
    },
    {
        0x5EF4EAD7u,
        renodx::mods::shader::CustomShader{
            .crc32 = 0x5EF4EAD7u,
            .code = __0x5EF4EAD7,
            .on_draw = OnBeforeFoliageDraw,
        },
    },
    {
        0xFDAAF80Eu,
        renodx::mods::shader::CustomShader{
            .crc32 = 0xFDAAF80Eu,
            .code = __0xFDAAF80E,
            .on_draw = OnBeforeLightingShaderDraw,
        },
    },
    //Kyoto lighting (GTVBAO + VBGI)
    {
        0x0CDCB258u,
        renodx::mods::shader::CustomShader{
            .crc32 = 0x0CDCB258u,
            .code = __0x0CDCB258,
            .on_draw = OnBeforeLightingShaderDraw,
        },
    },
    // -- Kai lighting (GTVBAO + VBGI) --
    CustomShaderEntryCallback(0x430ED091, OnBeforeLightingShaderDraw),
    CustomShaderEntryCallback(0xF6C55E5F, OnBeforeLightingShaderDraw),
    // -- Kai volumetric fog (IS-FAST + Haze AA) --
    CustomShaderEntryCallback(0xBD7DFE49, OnBeforeKaiVolFogDraw),
    // -- Kai character lighting (Env SSS + Character Shadowing) --
    {
        0x445A1838u,
        renodx::mods::shader::CustomShader{
            .crc32 = 0x445A1838u,
            .code = __0x445A1838,
            .on_draw = OnBeforeCharLightingDraw,
        },
    },
    // -- Kai cubemap shaders stay vanilla (no CustomShaderEntry overrides) --
    // -- Daybreak 2 cubemap shaders stay vanilla (no CustomShaderEntry overrides) --
    // -- Daybreak 2 volumetric fog (Haze AA) --
    CustomShaderEntryCallback(0x9A49E6E9, nullptr),
    // -- Daybreak 2 character lighting (Env SSS + Character Shadowing) --
    {
        0xAC3BA23Cu,
        renodx::mods::shader::CustomShader{
            .crc32 = 0xAC3BA23Cu,
            .code = __0xAC3BA23C,
            .on_draw = OnBeforeCharLightingDraw,
        },
    },
    // -- Kai DOF shaders --
    CustomShaderEntryCallback(0xAB6DBF4D, nullptr),
    // Expanded from CustomShaderEntryCallback, which only sets on_draw, to the
    // explicit struct so it ALSO gets on_drawn. That is what makes Kai's
    // "Cutscene Only" work: on_drawn is the cutscene signal, and the DoF gather
    // draws only when the game dispatches depth of field. Same shape as the Sora
    // 0xCD6FC25D entry below.
    {
    0x2734F870u,
    renodx::mods::shader::CustomShader{
    .crc32 = 0x2734F870u,
    .code = __0x2734F870,
    .on_draw = OnBeforeDofGatherDraw,
    .on_drawn = OnDrawnDofGather,
    },
    },
    // -- Sora 2nd DOF shaders (port Kai improved) --
    CustomShaderEntryCallback(0x5BBEC5A3, nullptr),
    // Gather pass � shared by Sora 2nd and Sora 1st (Sora 1st supplies its own
    // CoC pass, 0x1CA8DE95, and reuses this one). on_drawn is the "Cutscene
    // Only" motion blur deploy point; on_replace keeps the IS-FAST t5 push.
    {
        0xCD6FC25Du,
        renodx::mods::shader::CustomShader{
            .crc32 = 0xCD6FC25Du,
            .code = __0xCD6FC25D,
            .on_replace = OnBeforeDofGatherDraw,
            .on_drawn = OnDrawnDofGather,
        },
    },
    // -- Sora 1st DOF shaders --
    CustomShaderEntryCallback(0x1CA8DE95, nullptr),
    // -- Motion Blur "Always On" deploy point: the post-TAA tonemap --
    // Empty payload = no replacement; the game's own shader runs and we only
    // swap t0 for the blurred result.
    {
        kSoraTonemapHash,
        renodx::mods::shader::CustomShader{
            .crc32 = kSoraTonemapHash,
            .on_draw = OnBeforeTonemapDraw,
        },
    },
    // Kai's motion blur deploy point, same mechanism as the Sora tonemap above:
    // Empty payload = no replacement; the game's own shader runs and we only
    // swap t0 for the blurred result. Kai's pass reads its colour from t0.
    {
        kKaiTonemapHash,
        renodx::mods::shader::CustomShader{
            .crc32 = kKaiTonemapHash,
            .on_draw = OnBeforeTonemapDraw,
        },
    },
  //__ALL_CUSTOM_SHADERS,
};

// ----------- Settings -----------

renodx::utils::settings::Settings settings = {
    new renodx::utils::settings::Setting{
        .key = "SettingsMode",
        .binding = &g_settings_mode,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f,
        .can_reset = false,
        .label = "Settings Mode",
        .section = "Settings",
        .labels = {"Basic", "Advanced"},
        .on_change = []() {
          if (g_settings_mode < 0.5f) {  // Switched to Basic � reset advanced-only settings
            float saved = g_settings_mode;
            g_settings_mode = 1.0f;
            std::vector<renodx::utils::settings::Setting*> advanced;
            for (auto* s : settings) {
              if (s->key.empty() || !s->can_reset || s->is_global) continue;
              if (s->is_visible()) advanced.push_back(s);
            }
            g_settings_mode = saved;
            for (auto* s : advanced) {
              if (!s->is_visible()) {
                s->Set(s->default_value);
                s->Write();
              }
            }
          }
        },
        .is_global = true,
    },
    // �� IS-FAST Master Toggle (top-level) ��
    new renodx::utils::settings::Setting{
      .key = "ISFASTMasterEnable", .binding = &g_isfast_enabled,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 1.f, .label = "IS-FAST Noise", .section = "IS-FAST",
      .tooltip = "Master toggle for IS-FAST spatio-temporal blue noise. Uses the baked-in fast_noise_ea.dds (no file needed next to the game .exe).",
      .labels = {"Off", "On"},
    },
    new renodx::utils::settings::Setting{
      .key = "ISFASTStrength", .binding = &g_isfast_strength,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 1.f, .label = "Noise Strength", .section = "IS-FAST",
      .tooltip = "0 = deterministic (banding), 1 = full noise.",
      .min = 0.0f, .max = 1.0f, .format = "%.2f",
      .is_enabled = []() { return g_isfast_enabled > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "ISFASTDebugLogging", .binding = &g_isfast_debug_logging,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 0.f, .label = "Debug Logging", .section = "IS-FAST",
      .tooltip = "Log IS-FAST status: whether the baked-in noise texture loaded and which noise source is active.",
      .labels = {"Off", "On"},
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "ISFASTSpatialScale", .binding = &g_isfast_spatial_scale,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 1.f, .label = "Spatial Scale", .section = "IS-FAST",
      .tooltip = "Scale noise spatial frequency. <1 zooms in (smoother), >1 adds more detail.",
      .min = 0.25f, .max = 4.0f, .format = "%.2f",
      .is_enabled = []() { return g_isfast_enabled > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "ISFASTTemporalSpeed", .binding = &g_isfast_temporal_speed,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 1.f, .label = "Temporal Speed", .section = "IS-FAST",
      .tooltip = "Scale noise animation speed. 0 = frozen, 1 = default, 5 = fast flicker.",
      .min = 0.0f, .max = 5.0f, .format = "%.2f",
      .is_enabled = []() { return g_isfast_enabled > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "ISFASTSeedOffset", .binding = &g_isfast_seed_offset,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 0.f, .label = "Seed Offset", .section = "IS-FAST",
      .tooltip = "Offset the noise seed pattern (0-64). Shift to find optimal noise distribution.",
      .labels = {"0","4","8","12","16","20","24","28","32","36","40","44","48","52","56","60"},
      .is_enabled = []() { return g_isfast_enabled > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },

    // ----------- Kai / Daybreak 2 - Specific Sections -----------

    // -- SSGI (Falcom) --
    new renodx::utils::settings::Setting{
      .key = "KaiSSGIEnable", .binding = &shader_injection.ssgi_mod_enabled,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 1.f, .label = "Enable", .section = "SSGI (Falcom)",
      .labels = {"Off", "On"},
      .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "KaiSSGIColorBoost", .binding = &shader_injection.ssgi_color_boost,
      .default_value = 1.f, .label = "Color Boost", .section = "SSGI (Falcom)",
      .tooltip = "Scales SSGI RGB contribution before power shaping.",
      .min = 0.f, .max = 3.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.ssgi_mod_enabled >= 0.5f; },
      .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "KaiSSGIAlphaBoost", .binding = &shader_injection.ssgi_alpha_boost,
      .default_value = 1.f, .label = "Alpha Boost", .section = "SSGI (Falcom)",
      .tooltip = "Scales SSGI alpha before saturate.",
      .min = 0.f, .max = 3.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.ssgi_mod_enabled >= 0.5f; },
      .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "KaiSSGIPower", .binding = &shader_injection.ssgi_pow,
      .default_value = 1.f, .label = "Power", .section = "SSGI (Falcom)",
      .tooltip = "Applies pow(abs(color), Power) to shape bounce response.",
      .min = 0.1f, .max = 3.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.ssgi_mod_enabled >= 0.5f; },
      .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },

    // -- Depth of Field --
    new renodx::utils::settings::Setting{
      .key = "DOFMode", .binding = &shader_injection.dof_mode,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 1.f, .label = "Mode", .section = "Depth of Field",
      .tooltip = "Vanilla keeps the original blur shader. Improved uses DOF method 3 (gather).",
      .labels = {"Vanilla", "Improved"},
    },
    new renodx::utils::settings::Setting{
      .key = "DOFStrength", .binding = &shader_injection.dof_strength,
      .default_value = 1.f, .label = "Strength", .section = "Depth of Field",
      .tooltip = "Overall blend strength for improved DOF output.",
      .min = 0.f, .max = 2.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.dof_mode >= 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "DOFRadiusScale", .binding = &shader_injection.dof_radius_scale,
      .default_value = 1.33f, .label = "Radius Scale", .section = "Depth of Field",
      .tooltip = "Scales blur radius derived from game CoC.",
      .min = 0.25f, .max = 2.5f, .format = "%.2fx",
    },
    new renodx::utils::settings::Setting{
      // Deliberately a different key from the old raw "DOFSampleCount" slider.
      // That setting stored the tap count itself, and this one stores a 0-3
      // quality INDEX, so the two cannot share a key: a saved preset holding 12,
      // 24 or 64 would be clamped by LoadSetting into the valid index range and
      // every one of them would silently resolve to "Ultra". Renaming orphans the
      // stale value instead, so the tiers start clean at their default.
      .key = "DOFQuality", .binding = &shader_injection.dof_sample_count,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 2.f, .label = "Quality", .section = "Depth of Field",
      .tooltip = "Bokeh gather quality: Low 12, Medium 18, High 24, Ultra 30 taps. "
                 "Higher tiers produce smoother bokeh at higher cost. With Adaptive "
                 "Samples on, this becomes the MAXIMUM and only the most blurred "
                 "pixels reach it.",
      .labels = {"Low", "Medium", "High", "Ultra"},
      // .min must be 0: with .labels, GetMax() is labels.size() - 1, and a .min of
      // 4 would clamp the index into the empty range [4, 3] on load.
      .min = 0.f,
      .is_enabled = []() { return shader_injection.dof_mode >= 0.5f; },
      .parse = [](float value) {
        static constexpr float taps[] = {12.f, 18.f, 24.f, 30.f};
        const int index = static_cast<int>(value);
        return taps[index < 0 ? 0 : (index > 3 ? 3 : index)];
      },
    },
    new renodx::utils::settings::Setting{
      .key = "DOFAdaptiveSamples", .binding = &shader_injection.dof_adaptive_samples,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 1.f, .label = "Adaptive Samples", .section = "Depth of Field",
      .tooltip = "Scales each pixel's tap count to its own blur radius, so lightly "
                 "blurred pixels cost far less. Quality becomes the maximum. "
                 "Off = every blurred pixel uses the full Quality tap count, as before.",
      .labels = {"Off", "On"},
      .is_enabled = []() { return shader_injection.dof_mode >= 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "DOFTapCountView", .binding = &shader_injection.dof_debug_view,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 0.f, .label = "Tap Count View", .section = "Depth of Field",
      .tooltip = "Replaces the image with a per-pixel map of the adaptive ladder: one "
                 "hue per tap-count rung, and BLACK where the pixel early-outs and "
                 "costs nothing. Read the area fractions to find the real average tap "
                 "count, which is what the ladder thresholds should be tuned against.",
      .labels = {"Off", "On"},
      .is_enabled = []() { return shader_injection.dof_mode >= 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "DOFNearScale", .binding = &shader_injection.dof_near_scale,
      .default_value = 1.f, .label = "Near Scale", .section = "Depth of Field",
      .tooltip = "Scales near-field CoC response.",
      .min = 0.f, .max = 2.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.dof_mode >= 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "DOFFarScale", .binding = &shader_injection.dof_far_scale,
      .default_value = 1.35f, .label = "Far Scale", .section = "Depth of Field",
      .tooltip = "Scales far-field CoC response.",
      .min = 0.f, .max = 2.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.dof_mode >= 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "DOFCoCCurve", .binding = &shader_injection.dof_coc_curve,
      .default_value = 1.f, .label = "CoC Curve", .section = "Depth of Field",
      .tooltip = "Applies pow(CoC, Curve) before blur; >1 tightens focus transition.",
      .min = 0.25f, .max = 4.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.dof_mode >= 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "DOFEdgeThreshold", .binding = &shader_injection.dof_edge_threshold,
      .default_value = 0.3f, .label = "Edge Threshold", .section = "Depth of Field",
      .tooltip = "Rejects CoC-mismatched taps to reduce foreground/background bleeding.",
      .min = 0.02f, .max = 1.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.dof_mode >= 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "DOFISFAST", .binding = &shader_injection.dof_isfast_enabled,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 1.f, .label = "IS-FAST Rotation", .section = "Depth of Field",
      .tooltip = "Rotates the bokeh gather per pixel with IS-FAST noise. The golden-angle spiral is otherwise identical at every pixel, so undersampling shows up as fixed spiral/ring ghosting; rotating decorrelates neighbouring pixels and turns it into fine noise. Needs the IS-FAST master toggle.",
      .labels = {"Off", "On"},
      .is_enabled = []() { return shader_injection.dof_mode >= 0.5f && g_isfast_enabled > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "DOFSignSoftness", .binding = &shader_injection.dof_sign_softness,
      .default_value = 1.f, .label = "Layer Softness", .section = "Depth of Field",
      .tooltip = "Fixes sharp character lines in near blur. Acceptance of taps from the opposite depth layer: 0 = hard reject (vanilla behavior, can leave thin features unblurred), higher = softer separation (more bleed).",
      .min = 0.f, .max = 1.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.dof_mode >= 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "DOFCoverageFix", .binding = &shader_injection.dof_coverage_enabled,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 0.f, .label = "Coverage Composite", .section = "Depth of Field",
      .tooltip = "Fixes sharp character lines in near blur. Blends same-layer bokeh toward the full-disc average weighted by how much of the blur disc the feature actually covers.",
      .labels = {"Off", "On"},
      .is_enabled = []() { return shader_injection.dof_mode >= 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },

    // -- Motion Blur (Guertin et al. 2013) --.
    new renodx::utils::settings::Setting{
      // Key kept from the pre-split Mode so existing configs keep working.
      .key = "MotionBlurMode", .binding = &shader_injection.mb_mode,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 1.f, .label = "Mode", .section = "Motion Blur",
      .tooltip = "When the blur runs: never, only inside cutscenes, or always.",
      .labels = {"Off", "Cutscene Only", "Always On"},
      .is_visible = []() { return IsMotionBlurGame(); },
    },
    new renodx::utils::settings::Setting{
      .key = "MotionBlurCameraDirection", .binding = &shader_injection.mb_camera_sign,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 1.f, .label = "Camera Direction", .section = "Motion Blur",
      .tooltip = "Convention probe, not a look control. The game writes motion as one signed difference of the previous and current clip positions, but the shader varyings holding them are unlabelled, so which way round it is can only be settled from the image. On a static scene with the camera moving, set Debug View to Object Residual: the correct value here renders it BLACK, because object motion is then zero by definition. A lit residual that mirrors Camera Velocity means this is backwards. Leave it alone once you have picked the right one.",
      .labels = {"Forward (cur - prev)", "Reversed (prev - cur)"},
      .is_visible = []() { return IsMotionBlurGame() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "MotionBlurCameraCut", .binding = &shader_injection.mb_camera_cut,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 0.f, .label = "Reject Camera Cuts", .section = "Motion Blur",
      .tooltip = "Detects a teleport or a cutscene transition, where the view transform is replaced in a single frame, and writes zero velocity for that frame. Without it the whole screen smears toward the warp point for one frame. The test is on the camera transform, not the motion buffer, so a fast camera whip and a moving object cannot trigger it on their own. Toggle this off to A/B against a cut you can see; a false positive costs one sharp frame, a false negative costs the smear.",
      .is_enabled = []() { return MotionBlurActive(true); },
      .is_visible = []() { return IsMotionBlurGame() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "MotionBlurLogging", .binding = &g_mb_logging,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 0.f, .label = "Logging", .section = "Motion Blur",
      .tooltip = "Writes motion blur diagnostics to the ReShade log: the frame rate scale, which chain stage is running, a one-time report of the colour, motion and depth buffer sizes, and any reason the effect could not run. Turn it off to keep the log quiet. The effect itself is not affected either way.",
      .is_enabled = []() { return MotionBlurActive(true); },
      .is_visible = []() { return IsMotionBlurGame() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{      .key = "MotionBlurCameraCutThreshold", .binding = &shader_injection.mb_camera_cut_px,
      .default_value = 360.f, .label = "Camera Cut Threshold", .section = "Motion Blur",
      .tooltip = "How far the view transform may move in one frame before it counts as a cut, in 1080-reference pixels, measured at a nominal depth. A fast whip at 60 fps is roughly 30-80 px; a scene cut is hundreds. Raise it if ordinary camera movement is being rejected, lower it if a real cut is smearing.",
      .min = 8.f, .max = 512.f, .format = "%.0f px",
      .is_enabled = []() { return MotionBlurActive(true) && shader_injection.mb_camera_cut > 0.5f; },
      .is_visible = []() { return IsMotionBlurGame() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{      .key = "MotionBlurCameraJitter", .binding = &shader_injection.mb_camera_jitter,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 1.f, .label = "Camera Jitter", .section = "Motion Blur",
      .tooltip = "Convention probe, not a look control. The game adds a per-frame jitter delta to every motion vector, and prevViewProj_g may already contain that jitter. Adding it again inflates the camera term, and because object motion is computed as game minus camera, the inflation lands in the object channel at full size. Judge it the same way as Camera Direction: with the camera moving on a static scene, Object Residual must be BLACK.",
      .labels = {"Add jitter delta", "Omit jitter delta"},
      .is_visible = []() { return IsMotionBlurGame() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "MotionBlurIntensity", .binding = &shader_injection.mb_intensity,
      .default_value = 0.50f, .label = "Intensity", .section = "Motion Blur",
      .tooltip = "How far the blur smears, as a fraction of how far things actually moved: 0 is off, higher is softer.",
      .min = 0.f, .max = 2.f, .format = "%.2fx",
      .is_enabled = []() { return MotionBlurActive(true); },
      .is_visible = []() { return IsMotionBlurGame(); },
    },
    new renodx::utils::settings::Setting{
      .key = "MotionBlurMotionVectorInput", .binding = &g_mb_motion_input,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 0.f, .label = "Motion Vector Input", .section = "Motion Blur",
      .tooltip = "Where the blur reads its motion vectors from. TAA Motion Buffer is the copy the anti-aliasing pass reads, and is the safe default. Game RTV is the engine's own motion output, which is the only source available when anti-aliasing is switched off; the blur makes its own view onto it, so it is worth trying if the blur is missing without anti-aliasing. Both hold the same values, so they should look identical. Kai has no anti-aliasing in this addon, so it always reads the render target.",
      // Index order must match the tests: < 0.5 selects the render target, >= 0.5
      // selects TAA t3. The default is therefore 1, which is why TAA is the
      // default despite being listed second.
      .labels = {"Game RTV", "TAA Motion Buffer"},
      .is_enabled = []() { return MotionBlurActive(true); },
      // Hidden on Kai: its TAA hash resolves to none, so the t3 capture can never
      // happen and this option could only ever mean "no blur". Offering a choice
      // that has one dead branch is worse than not offering it.
      .is_visible = []() { return IsMotionBlurGame() && !IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "MotionBlurQuality", .binding = &g_mb_quality,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 2.f, .label = "Quality", .section = "Motion Blur",
      .tooltip = "How many times a pixel gets sampled while blurring, which keeps fast movement smooth instead of steppy, at a performance cost.",
      .labels = {"Low", "Medium", "High", "Ultra"},
      .is_enabled = []() { return MotionBlurActive(true); },
      .is_visible = []() { return IsMotionBlurGame(); },
    },
    new renodx::utils::settings::Setting{
      .key = "MotionBlurMaxRadius", .binding = &shader_injection.mb_max_radius_px,
      .default_value = 40.f, .label = "Max Radius", .section = "Motion Blur",
      .tooltip = "The longest streak the blur can produce.",
      .min = 8.f, .max = 80.f, .format = "%.0f px",
      .is_enabled = []() { return MotionBlurActive(true); },
      .is_visible = []() { return IsMotionBlurGame(); },
    },
    new renodx::utils::settings::Setting{
      .key = "MotionBlurCenterWeight", .binding = &shader_injection.mb_center_weight_k,
      .default_value = 1.f, .label = "Center Weight", .section = "Motion Blur",
      .tooltip = "Centre-sample weight divisor (paper k). The unblurred centre contributes N/(k*|v|) of the total, so LOWER keeps more of the original pixel and higher lets the streak dominate. Raise it if thin objects ghost, lower it if fast motion smears too little.",
      .min = 1.f, .max = 100.f, .format = "%.0f",
      .is_enabled = []() { return MotionBlurActive(true); },
      .is_visible = []() { return IsMotionBlurGame() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "MotionBlurJitter", .binding = &shader_injection.mb_jitter_h,
      .default_value = 0.95f, .label = "Jitter", .section = "Motion Blur",
      .tooltip = "How far past the streak length the integration domain reaches (paper h). Breaks up banding; too high softens the streak.",
      .min = 0.f, .max = 4.f, .format = "%.2f",
      .is_enabled = []() { return MotionBlurActive(true); },
      .is_visible = []() { return IsMotionBlurGame() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "MotionBlurMinVelocity", .binding = &shader_injection.mb_min_velocity_g,
      .default_value = 1.5f, .label = "Velocity Threshold", .section = "Motion Blur",
      .tooltip = "Below this a pixel's own velocity is replaced by the direction perpendicular to the tile velocity (paper g).",
      .min = 0.f, .max = 8.f, .format = "%.2f",
      .is_enabled = []() { return MotionBlurActive(true); },
      .is_visible = []() { return IsMotionBlurGame() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "MotionBlurNeighborFalloff", .binding = &shader_injection.mb_neighbor_t,
      .default_value = 1.f, .label = "NB Max Falloff", .section = "Motion Blur",
      .tooltip = "Slope of the stochastic tile lookup near tile borders (paper t), in tiles. Trades tile-edge banding for noise.",
      .min = 0.f, .max = 4.f, .format = "%.2f",
      .is_enabled = []() { return MotionBlurActive(true); },
      .is_visible = []() { return IsMotionBlurGame() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "MotionBlurDepthTolerance", .binding = &shader_injection.mb_depth_tolerance,
      .default_value = 0.1f, .label = "Depth Tolerance", .section = "Motion Blur",
      .tooltip = "Width of the soft depth transition that separates foreground from background samples. Higher lets more background bleed across depth edges.",
      .min = 0.01f, .max = 1.f, .format = "%.2f",
      .is_enabled = []() { return MotionBlurActive(true); },
      .is_visible = []() { return IsMotionBlurGame() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "MotionBlurJitterSource", .binding = &shader_injection.mb_jitter_source,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 1.f, .label = "Jitter Source", .section = "Motion Blur",
      .tooltip = "Halton is the paper's deterministic per-pixel sequence. IS-FAST uses the blue-noise volume already loaded for DoF and shadows; it animates, so residual sampling noise reads much less. Falls back to Halton when the volume is unavailable.",
      .labels = {"Halton", "IS-FAST"},
      .is_enabled = []() { return MotionBlurActive(true); },
      .is_visible = []() { return IsMotionBlurGame() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "MotionBlurFrameRateReference", .binding = &shader_injection.mb_frame_rate_reference,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 0.f, .label = "Frame Rate Reference", .section = "Motion Blur",
      .tooltip = "How the streak is tied to framerate. 0 (default) matches Unreal's r.MotionBlurTargetFPS 0: the measured frame time is tracked with a 0.1 moving average and the shutter spans the REAL frame, so a lower framerate gives a proportionally longer streak, and a change of rate ramps over about ten frames instead of stepping. Set a framerate above 0 to rescale the motion into that rate's units instead, which makes the result identical at every framerate at the cost of no longer being physically scaled.",
      .min = 0.f, .max = 240.f, .format = "%d",
      .is_enabled = []() { return MotionBlurActive(true); },
      .is_visible = []() { return IsMotionBlurGame() && IsAdvancedSettingsMode(); },
      },
    new renodx::utils::settings::Setting{
      .key = "MotionBlurHalfResThreshold", .binding = &shader_injection.mb_halfres_px,
      .default_value = 10.f, .label = "Half Res Threshold", .section = "Motion Blur",
      .tooltip = "Streak length, in 1080p-equivalent pixels, at which a tile switches to half resolution. Below it the gather runs at full resolution, because a streak that short is dominated by detail that half resolution has already thrown away. Above it the blur dominates and the cheaper path costs little visually. Sits where the sample ladder jumps to its most expensive rungs, so the tiles doing the most work are the ones that get cheaper. Try 4, 6 and 10.",
      .min = 0.f, .max = 96.f, .format = "%.0f px",
      .is_enabled = []() { return MotionBlurActive(true) && shader_injection.mb_halfres > 0.5f; },
      .is_visible = []() { return IsMotionBlurGame() && IsAdvancedSettingsMode(); },
      },
    new renodx::utils::settings::Setting{
      .key = "MotionBlurHalfRes", .binding = &shader_injection.mb_halfres,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 0.f, .label = "Half Resolution", .section = "Motion Blur",
      .tooltip = "Splits the gather by motion magnitude: tiles whose streak is short run at full resolution, tiles whose streak is long run at half resolution, and the result is selected per tile rather than blended. A streak too short to see cannot hide the detail that half resolution discards, so this keeps the two aligned. Off by default because the extra full-resolution pass costs about as much as the static gather it replaces. Toggling rebuilds the buffers, costing one frame.",
      .labels = {"Off", "On"},
      .is_enabled = []() { return MotionBlurActive(true); },
      .is_visible = []() { return IsMotionBlurGame() && IsAdvancedSettingsMode(); },
      },
    new renodx::utils::settings::Setting{
      .key = "MotionBlurDepthTest", .binding = &shader_injection.mb_depth_test,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 0.f, .label = "Depth Test", .section = "Motion Blur",
      .tooltip = "On: every tap reads the game depth and taps that disagree with the centre pixel are down-weighted, which is what separates foreground from background. Off: that fetch and the two cone terms are dropped and only the cylinder term remains, saving one texture load per tap, at the cost of bleeding across depth edges. Note Depth Tolerance already scales the same terms, so a very low value suppresses most of their effect while still paying for the fetch.",
      .labels = {"Off", "On"},
      .is_enabled = []() { return MotionBlurActive(true); },
      .is_visible = []() { return IsMotionBlurGame() && IsAdvancedSettingsMode(); },
      },
    new renodx::utils::settings::Setting{
      .key = "MotionBlurLocalVelocity", .binding = &shader_injection.mb_local_velocity_weights,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 1.f, .label = "Local Velocity Weights", .section = "Motion Blur",
      .tooltip = "On: each tap reads the motion texture and is weighted by its own direction, which is the paper's feature-aware term and what stops foreground bleeding across depth edges. Off: taps reuse the composite direction, removing ~25 texture loads per pixel (roughly a third of the gather). A/B it on a high-contrast edge.",
      .labels = {"Off", "On"},
      .is_enabled = []() { return MotionBlurActive(true); },
      .is_visible = []() { return IsMotionBlurGame() && IsAdvancedSettingsMode(); },
      },
    new renodx::utils::settings::Setting{
      .key = "MotionBlurDebugChain", .binding = &shader_injection.mb_debug_chain,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 0.f, .label = "Debug Chain", .section = "Motion Blur",
      .tooltip = "Diagnostic only. Full runs everything. Prep Only runs the three preprocessing dispatches and skips the gather, leaving the image untouched. Gather Only runs just the gather, reusing the PREVIOUS frame's prep, so do not read its image as a correctness comparison, and do not use it to time a moving scene: with prep frozen the tile grid is stale and almost nothing passes the gather's early-out. If preparation has never run it falls back to Full for that frame.",
      .labels = {"Full", "Prep Only", "Gather Only"},
      .is_enabled = []() { return MotionBlurActive(true); },
      .is_visible = []() { return IsMotionBlurGame() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "MotionBlurDebugView", .binding = &shader_injection.mb_debug_view,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 0.f, .label = "Debug View", .section = "Motion Blur",
      .tooltip = "Velocity, NeighborMax, the tile grid, linearized depth, blur amount, sample count, the half resolution split, the reconstructed camera term, or the error in it. Use these to confirm the motion buffer is live and the tile grid is resolution independent. Sample Count shows black where the blur early-outs and one distinct hue per rung of the adaptive ladder, which is what Quality now bounds: this is also the readout for tuning the rungs later. Half Res Detect draws the split partition itself: GREEN = blurred at full resolution, RED = blurred at half resolution, GREY = below the early-out so it will not blur at all, BLUE = Half Resolution is off so everything is full resolution. Boundaries are tile-quantised, so it also reads as the verification that the split no longer changes with framerate. Camera Velocity is the reconstructed camera term on its own. Object Residual is the difference against the game's own motion, and it is the one to settle the Camera Direction and Camera Jitter settings: on a STATIC scene with the camera moving it must be BLACK, because object motion is then zero by definition. A lit residual that mirrors Camera Velocity means the camera estimate is inverted or mis-scaled. It is also a direct readout of what the blur is doing, because the streak length is the camera's share of the total, so anything bright here is exactly the object contribution that was removed.",
      .labels = {"Off", "Velocity", "Neighbor Max", "Tile Grid", "Linear Depth", "Blur Amount", "Sample Count", "Half Res Detect", "Camera Velocity", "Object Residual"},
      .is_enabled = []() { return MotionBlurActive(true); },
      .is_visible = []() { return IsMotionBlurGame() && IsAdvancedSettingsMode(); },
    },

    // -- Character SSGI --
    new renodx::utils::settings::Setting{
      .key = "CharacterSSGICompositeMethod", .binding = &g_char_vbgi_composite_method,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 1.f, .label = "Apply Game SSGI", .section = "Character SSGI",
      .labels = {"Off", "On"},
      .is_visible = []() { return IsKai(); },
    },
    new renodx::utils::settings::Setting{
      .key = "CharacterSSGICompositeStrength", .binding = &shader_injection.char_gi_strength,
      .default_value = 3.0f, .label = "Strength", .section = "Character SSGI",
      .tooltip = "Overall contribution scale for character GI.",
      .min = 0.f, .max = 3.f, .format = "%.2f",
      .is_enabled = []() { return g_char_vbgi_composite_method >= 0.5f; },
      .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "CharacterSSGICompositeAlphaScale", .binding = &shader_injection.char_gi_alpha_scale,
      .default_value = 1.0f, .label = "Alpha Scale", .section = "Character SSGI",
      .tooltip = "Scales sampled SSGI alpha before blending.",
      .min = 0.f, .max = 3.f, .format = "%.2f",
      .is_enabled = []() { return g_char_vbgi_composite_method >= 0.5f; },
      .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "CharacterSSGICompositeChroma", .binding = &shader_injection.char_gi_chroma_strength,
      .default_value = 0.50f, .label = "Chroma", .section = "Character SSGI",
      .tooltip = "Scales colorful GI component; lower values reduce tinting.",
      .min = 0.f, .max = 2.f, .format = "%.2f",
      .is_enabled = []() { return g_char_vbgi_composite_method >= 0.5f; },
      .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "CharacterSSGICompositeLuma", .binding = &shader_injection.char_gi_luma_strength,
      .default_value = 0.0f, .label = "Luma", .section = "Character SSGI",
      .tooltip = "Scales neutral GI brightness; keep low to avoid white haze.",
      .min = 0.f, .max = 1.f, .format = "%.3f",
      .is_enabled = []() { return g_char_vbgi_composite_method >= 0.5f; },
      .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "CharacterSSGICompositeShadowPower", .binding = &shader_injection.char_gi_shadow_power,
      .default_value = 1.25f, .label = "Shadow Power", .section = "Character SSGI",
      .tooltip = "Higher values concentrate GI toward darker areas.",
      .min = 0.1f, .max = 4.f, .format = "%.2f",
      .is_enabled = []() { return g_char_vbgi_composite_method >= 0.5f; },
      .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "CharacterSSGICompositeDarkBoost", .binding = &shader_injection.char_gi_dark_boost,
      .default_value = 0.0f, .label = "Dark Boost", .section = "Character SSGI",
      .tooltip = "Extra GI multiplier in darker regions (after shadow mask).",
      .min = 0.f, .max = 4.f, .format = "%.2f",
      .is_enabled = []() { return g_char_vbgi_composite_method >= 0.5f; },
      .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "CharacterSSGICompositeBrightBoost", .binding = &shader_injection.char_gi_bright_boost,
      .default_value = 3.0f, .label = "Bright Boost", .section = "Character SSGI",
      .tooltip = "Boosts GI on brighter regions (values above 1.0 increase bright-side contribution).",
      .min = 0.f, .max = 3.f, .format = "%.2f",
      .is_enabled = []() { return g_char_vbgi_composite_method >= 0.5f; },
      .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "CharacterSSGICompositeHeadroomPower", .binding = &shader_injection.char_gi_headroom_power,
      .default_value = 1.25f, .label = "Headroom Power", .section = "Character SSGI",
      .tooltip = "Controls how strongly bright pixels reject additional GI.",
      .min = 0.1f, .max = 4.f, .format = "%.2f",
      .is_enabled = []() { return g_char_vbgi_composite_method >= 0.5f; },
      .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "CharacterSSGICompositeMaxAdd", .binding = &shader_injection.char_gi_max_add,
      .default_value = 0.020f, .label = "Max Add", .section = "Character SSGI",
      .tooltip = "Per-channel cap for added GI to prevent haze/bloomy washout.",
      .min = 0.f, .max = 1.f, .format = "%.3f",
      .is_enabled = []() { return g_char_vbgi_composite_method >= 0.5f; },
      .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "CharacterSSGICompositePeakLumaCap", .binding = &shader_injection.char_gi_peak_luma_cap,
      .default_value = 0.0f, .label = "Peak Luma Cap", .section = "Character SSGI",
      .tooltip = "Caps peak GI brightness on characters after blending weights. Set 0 to disable.",
      .min = 0.f, .max = 1.f, .format = "%.3f",
      .is_enabled = []() { return g_char_vbgi_composite_method >= 0.5f; },
      .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "CharacterSSGICompositeDepthReject", .binding = &shader_injection.char_gi_depth_reject,
      .default_value = 2.0f, .label = "Depth Reject", .section = "Character SSGI",
      .tooltip = "Higher values suppress GI across depth discontinuities and silhouette edges.",
      .min = 0.f, .max = 16.f, .format = "%.2f",
      .is_enabled = []() { return g_char_vbgi_composite_method >= 0.5f; },
      .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },

    // -- Fog Color Correction --
    new renodx::utils::settings::Setting{
      .key = "FogColorCorrectionMode", .binding = &shader_injection.fog_color_correction_enabled,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 1.f, .label = "Mode", .section = "Fog Color Correction",
      .labels = {"Vanilla", "Improved"},
      .is_visible = []() { return IsKai(); },
    },
    new renodx::utils::settings::Setting{
      .key = "FogHue", .binding = &shader_injection.fog_hue,
      .default_value = 0.f, .label = "Fog Hue", .section = "Fog Color Correction",
      .min = 0.f, .max = 2.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.fog_color_correction_enabled >= 0.5f; },
      .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "FogChrominance", .binding = &shader_injection.fog_chrominance,
      .default_value = 0.f, .label = "Fog Chroma", .section = "Fog Color Correction",
      .min = 0.f, .max = 2.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.fog_color_correction_enabled >= 0.5f; },
      .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "FogAvgBrightness", .binding = &shader_injection.fog_avg_brightness,
      .default_value = 0.85f, .label = "Fog Avg Bright", .section = "Fog Color Correction",
      .min = 0.f, .max = 2.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.fog_color_correction_enabled >= 0.5f; },
      .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "FogMinBrightness", .binding = &shader_injection.fog_min_brightness,
      .default_value = 0.f, .label = "Fog Min Bright", .section = "Fog Color Correction",
      .min = -0.5f, .max = 1.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.fog_color_correction_enabled >= 0.5f; },
      .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "FogMinChroma", .binding = &shader_injection.fog_min_chroma_change,
      .default_value = 0.f, .label = "Fog Min Chroma", .section = "Fog Color Correction",
      .tooltip = "Minimum chroma ratio applied during fog hue/chroma restoration.",
      .min = 0.f, .max = 4.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.fog_color_correction_enabled >= 0.5f; },
      .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "FogMaxChroma", .binding = &shader_injection.fog_max_chroma_change,
      .default_value = 0.f, .label = "Fog Max Chroma", .section = "Fog Color Correction",
      .tooltip = "Maximum chroma ratio applied during fog hue/chroma restoration.",
      .min = 0.f, .max = 8.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.fog_color_correction_enabled >= 0.5f; },
      .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "FogLightnessStrength", .binding = &shader_injection.fog_lightness_strength,
      .default_value = 1.f, .label = "Fog Lightness", .section = "Fog Color Correction",
      .tooltip = "Scales fog lightness restoration amount.",
      .min = 0.f, .max = 2.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.fog_color_correction_enabled >= 0.5f; },
      .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "FogColorCorrectionStrength", .binding = &shader_injection.fog_color_correction_strength,
      .default_value = 0.5f, .label = "2D Fog Correction Strength", .section = "Fog Color Correction",
      .min = 0.f, .max = 1.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.fog_color_correction_enabled >= 0.5f; },
      .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "Fog3DCorrectionStrength", .binding = &shader_injection.volfog_color_correction_strength,
      .default_value = 0.5f, .label = "3D Fog Correction Strength", .section = "Fog Color Correction",
      .tooltip = "Controls how strongly fog color correction is applied to volumetric fog. 0 = off, 1 = full.",
      .min = 0.f, .max = 1.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.fog_color_correction_enabled >= 0.5f; },
      .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "VolFogHazeAAMode",
        .binding = &shader_injection.volfog_haze_aa_mode,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Volumetric Haze AA",
        .section = "Volumetric Fog",
        .tooltip = "Mode for volumetric haze anti-aliasing: Vanilla or Improved.",
        .labels = {"Vanilla", "Improved"},
    },
    new renodx::utils::settings::Setting{
      .key = "CharShadowMode", .binding = &shader_injection.char_shadow_mode,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 0.f,
      .label = "Vanilla Character Shadowing",
      .section = "Contact Shadows",
      .tooltip = "The ENGINE's own screen-space character shadowing: the short "
                 "camera-facing march it ships with. Off by default, so the "
                 "contact and micro terms own character shadowing unless you "
                 "deliberately want both. Hidden on Kyoto, which has no "
                 "character lighting pass, and on Daybreak 2, which is out of "
                 "scope for this add-on.",
      .labels = {"Off", "On (engine)"},
      .is_visible = []() { return !IsKyoto() && !IsDaybreak2(); },
    },
    
    
    
    new renodx::utils::settings::Setting{
      .key = "CharGTVBAOMode", .binding = &shader_injection.char_gtvbao_mode,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 0.f, .label = "Allow GTVBAO", .section = "GTVBAO",
      .labels = {"Off", "On", "Combined"},
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "CharGTVBAOMaskStr", .binding = &shader_injection.char_gtvbao_mask_strength,
      .default_value = 75.f, .label = "GTVBAO Char Mask", .section = "GTVBAO",
      .min = 0.f, .max = 100.f,
      .is_enabled = []() { return shader_injection.char_gtvbao_mode > 0.5f; },
      .parse = [](float v) { return v * 0.01f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "CharGTVBGIMaskStr", .binding = &shader_injection.char_gtvbgi_mask_strength,
      .default_value = 0.f, .label = "GTVBGI Char Mask", .section = "GTVBAO",
      .min = 0.f, .max = 100.f,
      .parse = [](float v) { return v * 0.01f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    new renodx::utils::settings::Setting{
      .key = "DebugShowEnvSSS", .binding = &shader_injection.debug_show_env_sss,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 0.f,
      .label = "Foliage / Char Mask Debug",
      .section = "GTVBAO",
      .tooltip = "Inspects the foliage and character mask the GTVBAO character "
                 "options act on, and the occlusion the micro pass reads. View 2 "
                 "now shows the Contact Shadow term; it used to show the "
                 "retired screen-space shadow channel.",
      .labels = {"Off", "Mask", "Contact Term", "SSAO Sample", "Raw mrt0.z"},
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    // �� GTVBAO ��
    new renodx::utils::settings::Setting{
      .key = "GTVBAOMode", .binding = &shader_injection.gtvbao_mode,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 1.f, .label = "GTVBAO mode", .section = "GTVBAO",
      .tooltip = "Off = vanilla game AO. On = GTVBAO compute-shader AO.",
      .labels = {"Off (Vanilla AO)", "On (GTVBAO)"},
    },
    new renodx::utils::settings::Setting{
      .key = "GTVBAOQuality", .binding = &shader_injection.gtvbao_quality_level,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 2.f, .label = "Quality Level", .section = "GTVBAO",
      .labels = {"Low", "Medium", "High", "Ultra"},
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "GTVBAOResolution", .binding = &shader_injection.gtvbao_resolution,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 1.f, .label = "GTVBAO Resolution", .section = "GTVBAO",
      .tooltip = "Half is recommended at high or native resolutions, but it halves whatever resolution you render at, so it can get very low when upscaling is enabled.",
      .labels = {"Full", "Half"},
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "GTVBAOUpscalePlaneSigma", .binding = &shader_injection.gtvbao_upscale_plane_sigma,
      .default_value = 40.f, .label = "Upscale Plane Sigma", .section = "GTVBAO",
      .tooltip = "Half-mode reconstruction plane edge-stop sigma. Higher = smoother across depth steps.",
      .min = 1.f, .max = 400.f, .format = "%.1f",
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && shader_injection.gtvbao_resolution > 0.5f; },
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "GTVBAOUpscaleNormalPower", .binding = &shader_injection.gtvbao_upscale_normal_power,
      .default_value = 16.f, .label = "Upscale Normal Power", .section = "GTVBAO",
      .tooltip = "Half-mode reconstruction normal weight power.",
      .min = 1.f, .max = 64.f, .format = "%.1f",
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && shader_injection.gtvbao_resolution > 0.5f; },
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "GTVBAOUpscaleDebug", .binding = &shader_injection.gtvbao_upscale_debug,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 0.f, .label = "Upscale Debug", .section = "GTVBAO",
      .tooltip = "Half-mode reconstruction diagnostics (replaces the scene while active).",
      .labels = {"Final", "Raw Half AO", "Denoised Half AO", "Reconstructed AO", "Depth Weight", "Normal Weight", "Combined Weight", "Raw Half GI", "Denoised Half GI", "Reconstructed GI"},
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "GTVBAODenoisePasses", .binding = &shader_injection.gtvbao_denoise_passes,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 1.f, .label = "Denoise Passes", .section = "GTVBAO",
      .tooltip = "Bilateral chain strength. Ignored while �-Trous Filter is On (fixed 3 wavelet iterations).",
      .labels = {"Off", "Sharp (1)", "Medium (2)", "Soft (3)"},
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && shader_injection.gtvbao_atrous_enabled < 0.5f; },
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "GTVBAOJitter", .binding = &g_gtvbao_jitter_toggle,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 0.f, .label = "Jitter", .section = "GTVBAO",
      .tooltip = "Enable temporal jitter even when denoising is off.",
      .labels = {"Off", "On"},
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && shader_injection.gtvbao_denoise_passes < 0.5f; },
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "GTVBAONoiseType", .binding = &shader_injection.gtvbao_noise_type,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 0.f, .label = "Noise Type", .section = "GTVBAO",
      .tooltip = "IS-FAST = pre-computed blue noise (baked into the addon). "
                 "IGN = Interleaved Gradient Noise. Hilbert = Hilbert curve noise. "
                 "Only applies when IS-FAST master toggle is On; forced to Hilbert when Off.",
      .labels = {"IS-FAST", "IGN", "Hilbert"},
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && g_isfast_enabled > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "GTVBAORadius", .binding = &shader_injection.gtvbao_radius,
      .default_value = 1.0f, .label = "Radius", .section = "GTVBAO",
      .min = 0.01f, .max = 5.0f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f; },
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "GTVBAOFalloffRange", .binding = &shader_injection.gtvbao_falloff_range,
      .default_value = 0.615f, .label = "Falloff Range", .section = "GTVBAO",
      .min = 0.0f, .max = 1.0f, .format = "%.3f",
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f; },
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "GTVBAORadiusMultiplier", .binding = &shader_injection.gtvbao_radius_multiplier,
      .default_value = 1.8f, .label = "Radius Multiplier", .section = "GTVBAO",
      .min = 0.3f, .max = 3.0f, .format = "%.3f",
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f; },
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "GTVBAOFinalPower", .binding = &shader_injection.gtvbao_final_power,
      .default_value = 1.8f, .label = "Final Power", .section = "GTVBAO",
      .min = 0.5f, .max = 5.0f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f; },
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "GTVBAOSampleDistribution", .binding = &shader_injection.gtvbao_sample_distribution,
      .default_value = 1.5f, .label = "Sample Distribution", .section = "GTVBAO",
      .min = 1.0f, .max = 3.0f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f; },
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "GTVBAOBitmaskThickness", .binding = &shader_injection.gtvbao_bitmask_thickness,
      .default_value = 0.35f, .label = "Bitmask Thickness", .section = "GTVBAO",
      .tooltip = "World-space thickness for visibility bitmask. Higher = more light passes behind surfaces.",
      .min = 0.01f, .max = 2.0f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f; },
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    // �� GTVBAO Upgrade (visibility bitmask accuracy improvements, always On) ��
    new renodx::utils::settings::Setting{
      .key = "GTVBAOGTVBAOCosineMode", .binding = &shader_injection.gtvbao_cosine_mode,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 1.f, .label = "Cosine Sampling Mode", .section = "GTVBAO",
      .tooltip = "Mode 1: Uniform slices with per-slice weight. Mode 2: Ray projection from world-space lobe. Mode 3: CDF importance sampling (best quality/speed).",
      .labels = {"Weight", "Project", "CDF"},
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f; },
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "GTVBAODepthMIPOffset", .binding = &shader_injection.gtvbao_depth_mip_offset,
      .default_value = 3.5f, .label = "Depth MIP Offset", .section = "GTVBAO",
      .min = 2.0f, .max = 6.0f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f; },
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "GTVBAODenoiseBlurBeta", .binding = &shader_injection.gtvbao_denoise_blur_beta,
      .default_value = 200.0f, .label = "Denoise Blur Beta", .section = "GTVBAO",
      .min = 0.5f, .max = 200.0f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && shader_injection.gtvbao_denoise_passes > 0.f; },
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "GTVBAODenoiseLeakThreshold", .binding = &shader_injection.gtvbao_denoise_leak_threshold,
      .default_value = 1.0f, .label = "Denoise Leak Threshold", .section = "GTVBAO",
      .tooltip = "Min edges before AO leaks between pixels. Lower = more temporal stability, slightly softer shadows. 2.5 = default.",
      .min = 1.0f, .max = 4.0f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && shader_injection.gtvbao_denoise_passes > 0.f; },
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "GTVBAODenoiseLeakStrength", .binding = &shader_injection.gtvbao_denoise_leak_strength,
      .default_value = 1.0f, .label = "Denoise Leak Strength", .section = "GTVBAO",
      .tooltip = "How strongly AO leaks across edges. Higher = less flicker on grass/thin geometry, slightly softer contact shadows. 0.5 = default.",
      .min = 0.0f, .max = 1.0f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && shader_injection.gtvbao_denoise_passes > 0.f; },
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    // Spatial denoiser only (Spatio-Temporal / Poisson removed). �-Trous kept below.
    new renodx::utils::settings::Setting{
      .key = "GTVBAOAtrousEnabled", .binding = &shader_injection.gtvbao_atrous_enabled,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 1.f, .label = "�-Trous Filter", .section = "GTVBAO",
      .tooltip = "Edge-aware wavelet spatial filter (3 iterations, growing radius). Replaces the bilateral chain.",
      .labels = {"Off", "On"},
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && shader_injection.gtvbao_denoise_passes > 0.f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "GTVBAOAtrousDepthSigma", .binding = &shader_injection.gtvbao_atrous_depth_sigma,
      .default_value = 1.0f, .label = "�-Trous Depth Stop", .section = "GTVBAO",
      .tooltip = "Depth edge sensitivity for the �-trous filter. Higher = smoother across depth steps (more leak).",
      .min = 0.05f, .max = 4.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && shader_injection.gtvbao_denoise_passes > 0.f && shader_injection.gtvbao_atrous_enabled > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "GTVBAOAtrousNormalSigma", .binding = &shader_injection.gtvbao_atrous_normal_sigma,
      .default_value = 64.f, .label = "�-Trous Normal Stop", .section = "GTVBAO",
      .tooltip = "Normal edge sensitivity for the �-trous filter. Quantized to powers of two; higher = sharper edges. 32 is the default.",
      .min = 2.f, .max = 64.f, .format = "%.0f",
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && shader_injection.gtvbao_denoise_passes > 0.f && shader_injection.gtvbao_atrous_enabled > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "GTVBAONormalInputMode", .binding = &g_gtvbao_normal_input_mode,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 1.f, .label = "MRT Normal Input", .section = "GTVBAO",
      .tooltip = "Off = depth normals only. On = use game g-buffer normals.",
      .labels = {"Off (Depth)", "On (MRT)"},
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f; },
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "GTVBAONormalInfluence", .binding = &g_gtvbao_normal_influence,
      .default_value = 1.f, .label = "Normal Influence", .section = "GTVBAO",
      .min = 0.f, .max = 2.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && g_gtvbao_normal_input_mode > 0.5f; },
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "GTVBAONormalDepthBlend", .binding = &g_gtvbao_normal_depth_blend,
      .default_value = 1.f, .label = "Normal Depth Blend", .section = "GTVBAO",
      .tooltip = "Base blend weight between the depth-derived normal and the MRT normal. 1.0 (neutral) uses the MRT normal in full; the depth normal is then only a fallback.",
      .min = 0.f, .max = 1.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && g_gtvbao_normal_input_mode > 0.5f; },
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "GTVBAONormalSharpness", .binding = &g_gtvbao_normal_sharpness,
      .default_value = 1.f, .label = "Normal Sharpness", .section = "GTVBAO",
      .tooltip = "Gamma applied to the depth blend. 1.0 (neutral) is a straight passthrough; higher values push the weight toward the depth normal faster.",
      .min = 0.01f, .max = 4.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && g_gtvbao_normal_input_mode > 0.5f; },
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "GTVBAONormalEdgeRejection", .binding = &g_gtvbao_normal_edge_rejection,
      .default_value = 0.f, .label = "Normal Edge Rejection", .section = "GTVBAO",
      .tooltip = "Rejects the MRT normal on depth discontinuities. 0.0 (neutral) applies no rejection; raise it to fall back toward the depth normal at silhouettes.",
      .min = 0.f, .max = 2.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && g_gtvbao_normal_input_mode > 0.5f; },
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "GTVBAONormalZPreservation", .binding = &g_gtvbao_normal_z_preservation,
      .default_value = 1.0f, .label = "Normal Z Preservation", .section = "GTVBAO",
      .tooltip = "Scales the view-space n.z of the MRT normal. 1.0 keeps the encoded normal intact; lower values tilt it. The shader floors this so it can never reach 0 and flatten the normal.",
      .min = 0.f, .max = 2.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && g_gtvbao_normal_input_mode > 0.5f; },
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "GTVBAONormalDetailResponse", .binding = &g_gtvbao_normal_detail_response,
      .default_value = 0.f, .label = "Normal Detail Response", .section = "GTVBAO",
      .tooltip = "Reweights the MRT normal by how far it disagrees with the depth normal. 0.0 (neutral) is an exact no-op; 1.0 leans on the depth normal where they agree and the MRT normal where they diverge.",
      .min = 0.f, .max = 1.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && g_gtvbao_normal_input_mode > 0.5f; },
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "GTVBAONormalMaxDarkening", .binding = &g_gtvbao_normal_max_darkening,
      .default_value = 1.f, .label = "Normal Max Darkening", .section = "GTVBAO",
      .tooltip = "Caps the final MRT normal blend weight in Multiply mode. 1.0 (neutral) applies no cap.",
      .min = 0.f, .max = 1.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && g_gtvbao_normal_input_mode > 0.5f; },
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "GTVBAONormalDarkeningMode", .binding = &g_gtvbao_normal_darkening_mode,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 0.f, .label = "Normal Darkening Mode", .section = "GTVBAO",
      .tooltip = "Multiply applies the Normal Max Darkening cap to the final blend weight; Replace skips it. Neutral either way while Max Darkening is at 1.0.",
      .labels = {"Multiply", "Replace"},
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && g_gtvbao_normal_input_mode > 0.5f; },
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "GTVBAOGINormalInputMode", .binding = &g_gtvbao_gi_normal_input_mode,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 1.f, .label = "VBGI Normal Input", .section = "GTVBAO",
      .tooltip = "VBGI's own normal source, independent of the AO one above. Off = depth-derived sample normals, which are softer and produce fewer fireflies. On = the same g-buffer normals AO uses.",
      .labels = {"Off (Depth)", "On (MRT)"},
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && shader_injection.vbgi_enabled > 0.5f; },
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "GTVBAOGINormalInfluence", .binding = &g_gtvbao_gi_normal_influence,
      .default_value = 1.f, .label = "VBGI Normal Influence", .section = "GTVBAO",
      .tooltip = "Scales the XY of VBGI's sample normal. 1.0 (neutral) leaves it untouched.",
      .min = 0.f, .max = 2.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && shader_injection.vbgi_enabled > 0.5f; },
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "GTVBAOGINormalZPreservation", .binding = &g_gtvbao_gi_normal_z_preservation,
      .default_value = 1.f, .label = "VBGI Normal Z Preservation", .section = "GTVBAO",
      .tooltip = "Scales the view-space n.z of VBGI's sample normal. 1.0 (neutral) keeps it intact; the shader floors this so it can never flatten the normal.",
      .min = 0.f, .max = 2.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && shader_injection.vbgi_enabled > 0.5f; },
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "GTVBAOGINormalTransformMode", .binding = &g_gtvbao_gi_normal_transform_mode,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 3.f, .label = "VBGI Normal Transform Mode", .section = "GTVBAO",
      .tooltip = "Same encoding as the AO transform mode, applied independently. Use this only if VBGI's normals look rotated while AO's are correct.",
      .labels = {"view_g + flip Z (default)", "viewInv_g", "Passthrough", "view_g (no flip)", "viewInv_g + flip Z"},
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && shader_injection.vbgi_enabled > 0.5f; },
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "GTVBAONormalTransformMode", .binding = &g_gtvbao_normal_transform_mode,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 4.f, .label = "Normal Transform Mode", .section = "GTVBAO",
      .tooltip = "How to transform MRT normals into GTVBAO's view space. The MRT stores WORLD-space normals and GTVBAO's space is +Z forward while the engine's view_g is -Z forward, so view_g needs a Z flip. The other modes are wrong baselines kept for A/B comparison.",
      .labels = {"view_g + flip Z (suspected correct)", "viewInv_g (no flip)", "Passthrough (no transform)", "view_g (no flip)", "viewInv_g + flip Z"},
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && g_gtvbao_normal_input_mode > 0.5f; },
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "GTVBAODebugView", .binding = &shader_injection.gtvbao_debug_view,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 0.f, .label = "Debug View", .section = "GTVBAO",
      .labels = {"Off", "AO Only", "GTVBAO raw .a", "GTVBAO RGBA", "Vanilla SSAO", "Depth",
                 "6:BitmaskHeat", "7:SectorCount", "8:1stSliceBits", "9:FoliageMask",
                 "10-21:Kai MRT (see Kai shader)", "22:Raw MRT0 xy", "23:Decoded normal + valid",
                 "24:View normal (GTVBAO space)", "25:NdotV + source", "26:cosNorm/projLen/vis",
                 "27:Raw slice AO", "28:Weighting loss", "29:NdotV/MRT Z/occlusion"},
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "GTVBAODebugLogging", .binding = &shader_injection.gtvbao_debug_logging,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 0.f, .label = "Debug Logging", .section = "GTVBAO",
      .labels = {"Off", "On"},
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "GTVBAOFixExperimental", .binding = &shader_injection.gtvbao_fix_experimental,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 0.f, .label = "Fix Experimental", .section = "GTVBAO",
      .tooltip = "Composite-side AO encoding diagnostic (Sora lighting shaders only). 0=Off (baseline).",
      .labels = {"Off", "1:Neutral 1.0", "2:No 0xFF mask", "3:Inverted", "4:All channels"},
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    // -- GTVBAO scheduling --
    new renodx::utils::settings::Setting{
      .key = "GTVBAOFrameSkip", .binding = &g_gtvbao_frame_skip,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 0.f, .label = "Frame Skip", .section = "GTVBAO",
      .tooltip = "Skip GTVBAO AO+GI computation every N frames to improve performance.",
      .labels = {"Off", "2 Frames", "3 Frames", "4 Frames"},
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f; },
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "GTVBAOPrefilter", .binding = &shader_injection.gtvbao_prefilter_enabled,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 1.f, .label = "Pre-filter AO", .section = "GTVBAO",
      .tooltip = "Depth-aware 3�3 bilateral pre-filter on raw AO before power curve (reduces bitmask noise).",
      .labels = {"Off", "On"},
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    // -- GTVBAO Foliage --
    new renodx::utils::settings::Setting{
      .key = "GTVBAOExcludeFoliage", .binding = &shader_injection.gtvbao_exclude_foliage,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 1.f, .label = "Exclude Foliage", .section = "GTVBAO",
      .tooltip = "Skip AO computation on foliage pixels (prevent wind disocclusion noise).",
      .labels = {"Off", "On"},
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "GTVBAOFoliageAOValue", .binding = &shader_injection.gtvbao_foliage_ao_value,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.f, .label = "Foliage AO Value", .section = "GTVBAO",
      .tooltip = "Blends foliage AO toward fully bright. 1.0 = keep normal GTVBAO AO (like foliage exclusion OFF). 0.0 = no AO on foliage (fully bright).",
      .min = 0.f, .max = 1.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && shader_injection.gtvbao_exclude_foliage > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    // -- Foliage Grass AO --
    new renodx::utils::settings::Setting{
      .key = "FoliageGrassAOEnabled", .binding = &shader_injection.foliage_grass_ao_enabled,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 1.f, .label = "Grass AO", .section = "Foliage Grass AO",
      .tooltip = "Per-blade vertical AO gradient: dark at root, bright at tip. Replaces noisy SSAO/GTAO on foliage with a stable bake. (Ghost of Tsushima �1.5e-ii)",
      .labels = {"Off", "On"},
    },
    new renodx::utils::settings::Setting{
      .key = "FoliageGrassAOBase", .binding = &shader_injection.foliage_grass_ao_base,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.15f, .label = "Grass AO Base", .section = "Foliage Grass AO",
      .tooltip = "AO multiplier at the blade root. Lower = darker base (more self-occlusion).",
      .min = 0.f, .max = 0.8f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.foliage_grass_ao_enabled > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "FoliageGrassAOTip", .binding = &shader_injection.foliage_grass_ao_tip,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 1.2f, .label = "Grass AO Tip", .section = "Foliage Grass AO",
      .tooltip = "AO multiplier at the blade tip. Values > 1.0 slightly brighten the tip (rim-light effect).",
      .min = 0.5f, .max = 1.5f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.foliage_grass_ao_enabled > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "FoliageGrassAOCurve", .binding = &shader_injection.foliage_grass_ao_curve,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.25f, .label = "Grass AO Curve", .section = "Foliage Grass AO",
      .tooltip = "Power curve exponent. <1.0 = darkening concentrated at base (recommended). 1.0 = linear. >1.0 = darkening extends further up the blade.",
      .min = 0.1f, .max = 2.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.foliage_grass_ao_enabled > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    // -- BRDF Improvement --
    new renodx::utils::settings::Setting{
      .key = "BRDFHammonDiffuse", .binding = &shader_injection.brdf_hammon_diffuse_enabled,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 0.f, .label = "Hammon 2017 Diffuse", .section = "BRDF Improvement",
      .tooltip = "Replaces Lambert diffuse with Hammon 2017 GGX+Smith multi-scatter energy-conserving diffuse (GDC 2017).",
      .labels = {"Off", "On"},
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "BRDFDiffuseStrength", .binding = &shader_injection.brdf_diffuse_strength,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 1.f, .label = "Diffuse Blend", .section = "BRDF Improvement",
      .tooltip = "Blend between vanilla Lambert and Hammon diffuse. 0=vanilla, 1=full Hammon, 2=2x boost.",
      .min = 0.f, .max = 2.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.brdf_hammon_diffuse_enabled > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "BRDFMultiScatterSpecular", .binding = &shader_injection.brdf_multiscatter_specular_enabled,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 1.f, .label = "Multi-Scatter GGX Specular", .section = "BRDF Improvement",
      .tooltip = "Replaces Blinn-Phong specular with GGX D�V�F + Kulla-Conty multi-scatter compensation (SIGGRAPH 2017).",
      .labels = {"Off", "On"},
    },
    new renodx::utils::settings::Setting{
      .key = "BRDFSpecularStrength", .binding = &shader_injection.brdf_specular_strength,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.1f, .label = "Specular Blend", .section = "BRDF Improvement",
      .tooltip = "Blend between vanilla Blinn-Phong and GGX+multi-scatter specular. 0=vanilla, 1=full GGX+MS, 2=2x boost.",
      .min = 0.f, .max = 2.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.brdf_multiscatter_specular_enabled > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "BRDFRoughnessMin", .binding = &shader_injection.brdf_roughness_min,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.5f, .label = "Roughness Min", .section = "BRDF Improvement",
      .tooltip = "Clamp minimum perceptual roughness to prevent GGX singularity. 0.04 is a safe minimum for most materials.",
      .min = 0.f, .max = 0.5f, .format = "%.2f",
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "BRDFRoughnessMax", .binding = &shader_injection.brdf_roughness_max,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.75f, .label = "Roughness Max", .section = "BRDF Improvement",
      .tooltip = "Clamp maximum perceptual roughness. 1.0 = no clamping.",
      .min = 0.5f, .max = 1.f, .format = "%.2f",
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    // -- GTVBAO dispatch Fix (for double volumetrics) --
    new renodx::utils::settings::Setting{
      .key = "GTVBAOCSDispatchFix", .binding = &g_gtvbao_cs_dispatch_fix,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 2.f, .label = "CS Dispatch Fix", .section = "GTVBAO",
      .tooltip = "Fixes double volumetrics caused by stale compute descriptor bindings. Fix 1: restore state via Apply(). Fix 2: null compute descriptors. Fix 3: null + restore.",
      .labels = {"Off", "Fix 1: Restore State", "Fix 2: Null Compute", "Fix 3: Null + Restore"},
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    // �� SSGI (Screen Space Global Illumination � integrated into GTVBAO) ��
    new renodx::utils::settings::Setting{
      .key = "SSGIEnable", .binding = &shader_injection.vbgi_enabled,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 1.f, .label = "VBGI Enable", .section = "VBGI",
      .tooltip = "Visibility bitmask indirect diffuse GI. Requires GTVBAO mode = On.",
      .labels = {"Off", "On"},
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "SSGIIntensity", .binding = &shader_injection.vbgi_intensity,
      .default_value = 1.0f, .label = "Intensity", .section = "VBGI",
      .min = 0.0f, .max = 5.0f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && shader_injection.vbgi_enabled > 0.5f; },
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "SSGISaturation", .binding = &shader_injection.vbgi_saturation,
      .default_value = 1.5f, .label = "Saturation", .section = "VBGI",
      .tooltip = "0 = grayscale GI, 1 = full color GI.",
      .min = 0.0f, .max = 2.0f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && shader_injection.vbgi_enabled > 0.5f; },
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "SSGICharMaskStrength", .binding = &shader_injection.vbgi_char_mask_strength,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.5f, .label = "Character Mask Strength", .section = "VBGI",
      .tooltip = "Reduce SSGI on character models. 0 = full GI on characters, 1 = fully masked.",
      .min = 0.5f, .max = 1.0f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && shader_injection.vbgi_enabled > 0.5f; },
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "SSGIMultiBounce", .binding = &shader_injection.vbgi_multibounce,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 1.f, .label = "Multi-Bounce", .section = "VBGI",
      .tooltip = "Enables multi-bounce GI: previous frame's indirect light feeds back into the GI computation.",
      .labels = {"Off", "On"},
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && shader_injection.vbgi_enabled > 0.5f; },
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "SSGIMultiBounceStrength", .binding = &shader_injection.vbgi_multibounce_strength,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 1.0f, .label = "Multi-Bounce Strength", .section = "VBGI",
      .tooltip = "Extra gain on top of the bounce fraction. 1.0 = use the fraction as-is. The feedback is already normalised per pixel, so this is only a trim; the pair is capped at 1.0.",
      .min = 0.0f, .max = 10.0f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && shader_injection.vbgi_enabled > 0.5f && shader_injection.vbgi_multibounce > 0.5f; },
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "SSGIMultiBounceBounceFraction", .binding = &shader_injection.vbgi_multibounce_bounce_fraction,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.15f, .label = "Multi-Bounce Bounce Fraction", .section = "VBGI",
      .tooltip = "Fraction of local direct light re-emitted per bounce (diffuse albedo). The feedback is normalised per pixel, so this is the fraction actually added everywhere instead of only in bright, unoccluded pockets. 0.15 = subtle, 0.3 = strong, 0.5 = very strong.",
      .min = 0.0f, .max = 0.5f, .format = "%.3f",
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && shader_injection.vbgi_enabled > 0.5f && shader_injection.vbgi_multibounce > 0.5f; },
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "SSGIMultiBounceSaturation", .binding = &shader_injection.vbgi_multibounce_saturation,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 1.0f, .label = "Multi-Bounce Saturation", .section = "VBGI",
      .tooltip = "Color saturation of the multi-bounce feedback. 0 = grayscale, 1 = full color.",
      .min = 0.0f, .max = 2.0f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && shader_injection.vbgi_enabled > 0.5f && shader_injection.vbgi_multibounce > 0.5f; },
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "SSGIMultiBounceMaxClamp", .binding = &shader_injection.vbgi_multibounce_max_clamp,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.f, .label = "Multi-Bounce Max Clamp", .section = "VBGI",
      .tooltip = "Clamp multi-bounce feedback per-channel to prevent over-brightening. 0 = off.",
      .min = 0.0f, .max = 20.0f, .format = "%.1f",
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && shader_injection.vbgi_enabled > 0.5f && shader_injection.vbgi_multibounce > 0.5f; },
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "SSGIAdaptiveR", .binding = &shader_injection.vbgi_adaptive_r,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.f, .label = "Red Adaptive Strength", .section = "VBGI",
      .tooltip = "Per-channel adaptive boost: amplifies a color channel more when it's dominant. 0=off, 1=max.",
      .min = 0.0f, .max = 1.0f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && shader_injection.vbgi_enabled > 0.5f; },
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "SSGIAdaptiveG", .binding = &shader_injection.vbgi_adaptive_g,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.f, .label = "Green Adaptive Strength", .section = "VBGI",
      .tooltip = "Per-channel adaptive boost: amplifies a color channel more when it's dominant. 0=off, 1=max.",
      .min = 0.0f, .max = 1.0f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && shader_injection.vbgi_enabled > 0.5f; },
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "SSGIAdaptiveB", .binding = &shader_injection.vbgi_adaptive_b,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.f, .label = "Blue Adaptive Strength", .section = "VBGI",
      .tooltip = "Per-channel adaptive boost: amplifies a color channel more when it's dominant. 0=off, 1=max.",
      .min = 0.0f, .max = 1.0f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && shader_injection.vbgi_enabled > 0.5f; },
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "SSGIAdaptiveMode", .binding = &shader_injection.vbgi_adaptive_mode,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 0.f, .label = "Adaptive Mode", .section = "VBGI",
      .tooltip = "GI Color = boost channels based on GI's own color. Albedo = boost based on surface color at pixel.",
      .labels = {"GI Color", "Surface Albedo"},
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && shader_injection.vbgi_enabled > 0.5f; },
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "SSGIAdaptiveLumaStrength", .binding = &shader_injection.vbgi_adaptive_luma_strength,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.0f, .label = "Adaptive Luma Strength", .section = "VBGI",
      .tooltip = "Target brightness for GI normalization. 0=off. Higher = brighter target. Evens out indoor/outdoor GI.",
      .min = 0.0f, .max = 5.0f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && shader_injection.vbgi_enabled > 0.5f; },
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "SSGIAdaptiveLumaBlend", .binding = &shader_injection.vbgi_adaptive_luma_blend,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.0f, .label = "Adaptive Luma Blend", .section = "VBGI",
      .tooltip = "Blend between original GI (0) and luma-normalized GI (1).",
      .min = 0.0f, .max = 1.0f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && shader_injection.vbgi_enabled > 0.5f; },
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "SSGIMaxClamp", .binding = &shader_injection.vbgi_max_clamp,
      .default_value = 0.0f, .label = "GI Max Clamp", .section = "VBGI",
      .tooltip = "Clamp GI per-channel to this maximum. 0 = off.",
      .min = 0.0f, .max = 20.0f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && shader_injection.vbgi_enabled > 0.5f; },
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "SSGIReduceAO", .binding = &shader_injection.vbgi_reduce_ao,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 0.f, .label = "Reduce AO with GI", .section = "VBGI",
      .tooltip = "Reduce GTVBAO occlusion where indirect light is strong. Keeps dark crevices dark while brightening lit surfaces.",
      .labels = {"Off", "On"},
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && shader_injection.vbgi_enabled > 0.5f; },
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "SSGIReduceAOStrength", .binding = &shader_injection.vbgi_reduce_ao_strength,
      .default_value = 2.f, .label = "Reduce AO Strength", .section = "VBGI",
      .tooltip = "How strongly indirect light reduces AO. 0=no change, 1=full reduction.",
      .min = 0.0f, .max = 5.0f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && shader_injection.vbgi_enabled > 0.5f && shader_injection.vbgi_reduce_ao > 0.5f; },
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "SSGILightExposure", .binding = &g_vbgi_light_exposure,
      .default_value = 1.0f, .label = "Light Exposure", .section = "VBGI",
      .tooltip = "Exposure scale for HDR light buffer. Start at 0.05. Lower = dimmer GI.",
      .min = 0.001f, .max = 5.0f, .format = "%.3f",
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && shader_injection.vbgi_enabled > 0.5f; },
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "SSGIFrameSkip", .binding = &g_vbgi_frame_skip,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 0.f, .label = "SSGI Frame Skip", .section = "VBGI",
      .tooltip = "Skip GI computation every N frames. AO still runs every frame.",
      .labels = {"Off", "2 Frames", "3 Frames", "4 Frames"},
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && shader_injection.vbgi_enabled > 0.5f; },
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "MultiBounceFrameSkip", .binding = &g_multibounce_frame_skip,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 0.f, .label = "Multi-Bounce Frame Skip", .section = "VBGI",
      .tooltip = "Skip multi-bounce accumulation every N frames.",
      .labels = {"Off", "2 Frames", "3 Frames", "4 Frames"},
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && shader_injection.vbgi_enabled > 0.5f && shader_injection.vbgi_multibounce > 0.5f; },
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    // �� VBGI debug ��
    new renodx::utils::settings::Setting{
      .key = "SSGIDebugView", .binding = &shader_injection.vbgi_debug_view,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 0.f, .label = "VBGI debug View", .section = "VBGI",
      .tooltip = "Replace scene with VBGI debug textures.",
      .labels = {"Off", "Raw GI", "Denoised GI", "Light Buffer", "Accumulated", "5:Sample Activity", "Light Color", "Final GI"},
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "SSGIDebugLogging", .binding = &shader_injection.vbgi_debug_logging,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 0.f, .label = "VBGI debug Logging", .section = "VBGI",
      .tooltip = "Log VBGI dispatch, push, and texture binding to console.",
      .labels = {"Off", "On"},
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    // -- Kai: GTVBAO VBGI Falcom SSGI consumption --
    new renodx::utils::settings::Setting{
      .key = "SSGIKaiConsumeFalcom", .binding = &shader_injection.vbgi_kai_consume_falcom,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 0.f, .label = "Consume Falcom SSGI (Kai)", .section = "VBGI",
      .tooltip = "When ON, Falcom's SSGI color modulates GTVBAO VBGI before blending. Creates a multiplicative interaction between the two GI sources.",
      .labels = {"Off", "On"},
      .is_enabled = []() { return shader_injection.vbgi_enabled > 0.5f; },
      .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "SSGIKaiFalcomBlend", .binding = &shader_injection.vbgi_kai_falcom_blend,
      .default_value = 0.5f, .label = "Falcom SSGI Blend (Kai)", .section = "VBGI",
      .tooltip = "How much Falcom SSGI color modulates GTVBAO VBGI. 0 = no modulation (additive only), 1 = full multiplicative blend.",
      .min = 0.f, .max = 1.0f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.vbgi_enabled > 0.5f && shader_injection.vbgi_kai_consume_falcom > 0.5f; },
      .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "SSGIKaiGTVBAOOnly", .binding = &shader_injection.vbgi_kai_gtvbao_only,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 0.f, .label = "GTVBAO VBGI Only (Kai)", .section = "VBGI",
      .tooltip = "When ON, Falcom SSGI is suppressed from output and only GTVBAO VBGI is visible. GTVBAO can still consume Falcom SSGI internally for modulation.",
      .labels = {"Off", "On"},
      .is_enabled = []() { return shader_injection.vbgi_enabled > 0.5f; },
      .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    // �� SSGI Affect Lights ��
    new renodx::utils::settings::Setting{
      .key = "SSGIAffectLights", .binding = &shader_injection.vbgi_affect_lights,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 0.f, .label = "Affect Lights", .section = "VBGI",
      .tooltip = "Additively blend the sun's lightColor into the GI contribution, tinting indirect light.",
      .labels = {"Off", "On"},
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "SSGILightsStrength", .binding = &shader_injection.vbgi_lights_strength,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 1.f, .label = "Lights Strength", .section = "VBGI",
      .tooltip = "How much lightColor to add. 0=no effect, 1=full sun color, >1=boosted.",
      .min = 0.f, .max = 5.0f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.vbgi_affect_lights > 0.5f; },
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "SSGILightsSaturation", .binding = &shader_injection.vbgi_lights_saturation,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 1.f, .label = "Lights Saturation", .section = "VBGI",
      .tooltip = "Vibrance applied to lightColor before adding. 0=grayscale, 1=neutral, >1=vivid.",
      .min = 0.f, .max = 100.0f, .format = "%.1f",
      .is_enabled = []() { return shader_injection.vbgi_affect_lights > 0.5f; },
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "SSGICascadeDebug", .binding = &shader_injection.vbgi_cascade_debug,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 0.f, .label = "CascadeCount Debug", .section = "VBGI",
      .tooltip = "Color overlay by shadowmapCascadeCount_g: 0=red, 1=yellow, 2=green, 3=cyan, 4=blue.",
      .labels = {"Off", "On"},
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "CPUOptDeferredDispatch", .binding = &g_cpuopt_deferred_dispatch,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 0.f, .label = "Deferred Dispatch", .section = "CPU Opt",
      .tooltip = "Move GTVBAO/VBGI dispatch to OnPresent (1-frame latency). Kai-only, default OFF � avoids latency.",
      .labels = {"Off", "On"},
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "CPUOptEnsurePipelines", .binding = &g_cpuopt_ensure_pipelines,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 1.f, .label = "Ensure Pipelines", .section = "CPU Opt",
      .tooltip = "Don't destroy/recreate pipelines every frame (kai-style).",
      .labels = {"Off", "On"},
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    // �� Shadow Maps ��
    new renodx::utils::settings::Setting{
      .key = "ShadowFilterMethod", .binding = &shader_injection.shadow_filter_method,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 2.f, .label = "Shadow Filter Method", .section = "Shadow Maps",
      .tooltip = "CSM filtering: Off = single sample. Falcom = vanilla 10-tap PCF. CHSS = contact-hardening soft shadows (Vogel disk + variable-radius PCF).",
      .labels = {"Off", "Falcom", "CHSS"},
      .is_visible = []() { return !IsKai(); },
    },
    new renodx::utils::settings::Setting{
      .key = "ShadowEdgeTint", .binding = &shader_injection.shadow_edge_tint,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 2.f, .label = "Colored Shadow Penumbra", .section = "Shadow Maps",
      .tooltip = "Off = neutral edges. Falcom = vanilla red tint. Improved = vibrancy boost in penumbra.",
      .labels = {"Off", "Falcom", "Improved"},
      .is_visible = []() { return !IsKai(); },
    },
    // �� CHSS Settings (enabled when ShadowFilterMethod = CHSS) ��
    new renodx::utils::settings::Setting{
      .key = "ShadowCHSSJitter", .binding = &shader_injection.shadow_pcss_jitter_enabled,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 1.f, .label = "CHSS Jitter", .section = "Shadow Maps",
      .tooltip = "Rotate the CHSS sample pattern each frame using the selected noise source (see Noise Mode).",
      .labels = {"Off", "On"},
      .is_enabled = []() { return shader_injection.shadow_filter_method > 1.5f; },
    .is_visible = []() { return !IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "ShadowCHSSNoiseMode", .binding = &shader_injection.shadow_chss_noise_mode,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 1.f, .label = "Noise Mode", .section = "Shadow Maps",
      .tooltip = "Jitter noise source. IGN = interleaved gradient noise (no texture needed). IS-FAST = pre-computed spatio-temporal blue noise (baked into the addon; requires ISFASTMasterEnable; falls back to IGN if unavailable).",
      .labels = {"IGN", "IS-FAST"},
      .is_enabled = []() { return shader_injection.shadow_filter_method > 1.5f && shader_injection.shadow_pcss_jitter_enabled > 0.5f && g_isfast_enabled > 0.5f; },
    .is_visible = []() { return !IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "ShadowCHSSBlockerSamples", .binding = &shader_injection.shadow_chss_blocker_count,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 16.f, .label = "Blocker Samples", .section = "Shadow Maps",
      .tooltip = "Sample count for the average blocker search. Higher = smoother penumbra estimate, lower = faster.",
      .min = 4.f, .max = 64.f, .format = "%d",
      .is_enabled = []() { return shader_injection.shadow_filter_method > 1.5f; },
    .is_visible = []() { return !IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "ShadowCHSSFilterSamples", .binding = &shader_injection.shadow_chss_sample_count,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 64.f, .label = "Filter Samples", .section = "Shadow Maps",
      .tooltip = "Sample count for the variable-radius PCF filter. Higher = softer, less noisy shadows, lower = faster.",
      .min = 4.f, .max = 64.f, .format = "%d",
      .is_enabled = []() { return shader_injection.shadow_filter_method > 1.5f; },
    .is_visible = []() { return !IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "ShadowCHSSJitterAmount", .binding = &shader_injection.shadow_pcss_jitter_amount,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 1.f, .label = "Jitter Amount", .section = "Shadow Maps",
      .tooltip = "0 = static Vogel disk, 1 = full temporal rotation.",
      .min = 0.0f, .max = 1.0f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.shadow_filter_method > 1.5f && shader_injection.shadow_pcss_jitter_enabled > 0.5f; },
    .is_visible = []() { return !IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "ShadowCHSSJitterSpeed", .binding = &shader_injection.shadow_pcss_jitter_speed,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 237.f, .label = "Jitter Speed", .section = "Shadow Maps",
      .tooltip = "Temporal animation speed. Higher = faster rotation.",
      .min = 0.0f, .max = 500.0f, .format = "%.0f",
      .is_enabled = []() { return shader_injection.shadow_filter_method > 1.5f && shader_injection.shadow_pcss_jitter_enabled > 0.5f; },
    .is_visible = []() { return !IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "ShadowBaseSoftness", .binding = &shader_injection.shadow_base_softness,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.0f, .label = "Base Softness", .section = "Shadow Maps",
      .tooltip = "Constant minimum penumbra width. Contact-hard at 0, always soft at 0.5.",
      .min = 0.0f, .max = 1.0f, .format = "%.3f",
      .is_enabled = []() { return shader_injection.shadow_filter_method > 1.5f; },
    .is_visible = []() { return !IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "ShadowCHSSSearchRadius", .binding = &shader_injection.shadow_chss_search_radius,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 1.f, .label = "World Softness", .section = "Shadow Maps",
      .tooltip = "Desired softness in world units. Same value = same penumbra width across all cascades. 0.1=sharp, 5=very soft.",
      .min = 0.1f, .max = 5.0f, .format = "%.1f",
      .is_enabled = []() { return shader_injection.shadow_filter_method > 1.5f; },
    .is_visible = []() { return !IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "ShadowCHSSPenumbraScale", .binding = &shader_injection.shadow_chss_penumbra_scale,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 5000.f, .label = "Penumbra Scale", .section = "Shadow Maps",
      .tooltip = "How fast penumbra widens with occluder distance (the paper's 80.0 constant). Higher = softer distant shadows.",
      .min = 1.0f, .max = 10000.0f, .format = "%.1f",
      .is_enabled = []() { return shader_injection.shadow_filter_method > 1.5f; },
    .is_visible = []() { return !IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "ShadowCHSSDepthCap", .binding = &shader_injection.shadow_chss_depth_cap,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 1.0f, .label = "Depth Sensitivity", .section = "Shadow Maps",
      .tooltip = "Max depth difference for penumbra. Higher = more distance-based softening.",
      .min = 0.01f, .max = 1.0f, .format = "%.3f",
      .is_enabled = []() { return shader_injection.shadow_filter_method > 1.5f; },
    .is_visible = []() { return !IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "ShadowCHSSMinRadius", .binding = &shader_injection.shadow_chss_min_radius,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.f, .label = "Min Filter Radius", .section = "Shadow Maps",
      .tooltip = "Guaranteed minimum PCF filter radius in shadow map texels. Prevents filter from collapsing. 0=off.",
      .min = 0.f, .max = 100.0f, .format = "%.0f",
      .is_enabled = []() { return shader_injection.shadow_filter_method > 1.5f; },
    .is_visible = []() { return !IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "ShadowCHSSPostBlur", .binding = &shader_injection.shadow_chss_post_blur,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 100.f, .label = "Post Blur", .section = "Shadow Maps",
      .tooltip = "Strength of the screen-space bilateral blur applied to the CHSS shadow mask. 0 = off (passthrough), 100 = full strength.",
      .min = 0.0f, .max = 100.0f, .format = "%.0f",
      .is_enabled = []() { return shader_injection.shadow_filter_method > 1.5f; },
    .is_visible = []() { return !IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "ShadowCHSSOnsetBias", .binding = &shader_injection.shadow_chss_onset_bias,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.f, .label = "Softening Onset", .section = "Shadow Maps",
      .tooltip = "Adds depth separation before the penumbra response. Higher values start soft shadows closer to contact; 0 preserves vanilla contact hardening.",
      .min = 0.0f, .max = 0.2f, .format = "%.4f",
      .is_enabled = []() { return shader_injection.shadow_filter_method > 1.5f; },
    .is_visible = []() { return !IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "ShadowCHSSPenumbraCurve", .binding = &shader_injection.shadow_chss_penumbra_curve,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 1.5f, .label = "Penumbra Curve", .section = "Shadow Maps",
      .tooltip = "Exponent applied to normalized penumbra separation. Values below 2 respond more aggressively near contact; values above 2 stay harder longer.",
      .min = 0.5f, .max = 4.0f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.shadow_filter_method > 1.5f; },
    .is_visible = []() { return !IsKai() && IsAdvancedSettingsMode(); },
    },
    // �� Colored Shadow Penumbra (Improved mode) ��
    new renodx::utils::settings::Setting{
      .key = "ShadowPenumbraColorStrength", .binding = &shader_injection.shadow_penumbra_color_strength,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.65f, .label = "Penumbra Color Strength", .section = "Shadow Maps",
      .tooltip = "How strongly the vibrancy effect is applied in penumbra regions. 0=off, 1=full.",
      .min = 0.f, .max = 2.0f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.shadow_edge_tint > 1.5f; },
    .is_visible = []() { return !IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "ShadowPenumbraVibrance", .binding = &shader_injection.shadow_penumbra_vibrance,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 30.f, .label = "Penumbra Vibrance", .section = "Shadow Maps",
      .tooltip = "Vibrance adjustment in penumbra. 0=grayscale, 1=neutral, >1=more vivid. Protects already-saturated colors.",
      .min = 0.f, .max = 100.0f, .format = "%.1f",
      .is_enabled = []() { return shader_injection.shadow_edge_tint > 1.5f; },
    .is_visible = []() { return !IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "ShadowPenumbraDetection", .binding = &shader_injection.shadow_penumbra_detection,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 1.0f, .label = "Penumbra Detection", .section = "Shadow Maps",
      .tooltip = "What counts as penumbra. Higher = wider detection area, more of the image gets the effect.",
      .min = 0.01f, .max = 1.0f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.shadow_edge_tint > 1.5f; },
    .is_visible = []() { return !IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "ShadowPenumbraColorBrightness", .binding = &shader_injection.shadow_penumbra_color_brightness,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 1.f, .label = "Penumbra Color Brightness", .section = "Shadow Maps",
      .tooltip = "Brightness multiplier for the vibrancy tint color. 1=neutral, 0=black, >1=brighter.",
      .min = 0.f, .max = 5.0f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.shadow_edge_tint > 1.5f; },
    .is_visible = []() { return !IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "ShadowPenumbraFalcomBlend", .binding = &shader_injection.shadow_penumbra_falcom_blend,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.5f, .label = "Falcom Penumbra Blend", .section = "Shadow Maps",
      .tooltip = "Blend the vibrancy effect toward Falcom's red shadowEdgeColor tint. 0=pure vibrancy, 1=pure Falcom.",
      .min = 0.f, .max = 1.0f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.shadow_edge_tint > 1.5f; },
    .is_visible = []() { return !IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "ShadowPenumbraEdgeVibrance", .binding = &shader_injection.shadow_penumbra_edge_vibrance,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.3f, .label = "Edge Color Vibrance", .section = "Shadow Maps",
      .tooltip = "Vibrance applied to shadowEdgeColor when Falcom blend > 0. 0=grayscale, 1=neutral, >1=vivid.",
      .min = 0.f, .max = 100.0f, .format = "%.1f",
      .is_enabled = []() { return shader_injection.shadow_edge_tint > 1.5f; },
    .is_visible = []() { return !IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "ShadowPenumbraLightColorBlend", .binding = &shader_injection.shadow_penumbra_lightcolor_blend,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.f, .label = "Light Color Blend", .section = "Shadow Maps",
      .tooltip = "Blend the penumbra tint toward the sun's lightColor. 0=no effect, 1=fully sun-colored penumbra.",
      .min = 0.f, .max = 1.0f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.shadow_edge_tint > 1.5f; },
    .is_visible = []() { return !IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "ShadowPenumbraLightColorSaturation", .binding = &shader_injection.shadow_penumbra_lightcolor_saturation,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.f, .label = "Light Color Saturation", .section = "Shadow Maps",
      .tooltip = "Vibrance applied to lightColor before blending. 0=grayscale, 1=neutral, >1=vivid sun color.",
      .min = 0.f, .max = 100.0f, .format = "%.1f",
      .is_enabled = []() { return shader_injection.shadow_edge_tint > 1.5f; },
    .is_visible = []() { return !IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "ShadowPenumbraDebugView", .binding = &shader_injection.shadow_penumbra_debug_view,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 0.f, .label = "Penumbra Debug View", .section = "Shadow Maps",
      .tooltip = "Visualize penumbra processing. PenumbraMask=detection area, TintColor=adjusted color, Result=final blend.",
      .labels = {"Off", "Penumbra Mask", "Tint Color", "Result", "Sun Color"},
      .is_enabled = []() { return shader_injection.shadow_edge_tint > 1.5f; },
    .is_visible = []() { return !IsKai() && IsAdvancedSettingsMode(); },
    },
    // �� Shadows (Kai) ��
    new renodx::utils::settings::Setting{
      .key = "KaiShadowBaseSoftness", .binding = &shader_injection.shadow_base_softness,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.05f, .label = "Base Softness", .section = "Shadows",
      .tooltip = "Constant minimum penumbra width for PCSS shadows. 0 = contact-hard, higher = always soft.",
      .min = 0.0f, .max = 1.0f, .format = "%.3f",
      .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "KaiShadowJitter", .binding = &shader_injection.shadow_pcss_jitter_enabled,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 1.f, .label = "PCSS Jitter", .section = "Shadows",
      .tooltip = "Use IS-FAST blue noise to rotate PCSS sample pattern each frame.",
      .labels = {"Off", "On"},
      .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "KaiShadowJitterAmount", .binding = &shader_injection.shadow_pcss_jitter_amount,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 1.f, .label = "Jitter Amount", .section = "Shadows",
      .tooltip = "0 = static Poisson, 1 = full temporal rotation.",
      .min = 0.0f, .max = 1.0f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.shadow_pcss_jitter_enabled > 0.5f; },
      .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "KaiShadowJitterSpeed", .binding = &shader_injection.shadow_pcss_jitter_speed,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 237.f, .label = "Jitter Speed", .section = "Shadows",
      .tooltip = "Temporal animation speed. Higher = faster rotation.",
      .min = 0.0f, .max = 500.0f, .format = "%.0f",
      .is_enabled = []() { return shader_injection.shadow_pcss_jitter_enabled > 0.5f; },
      .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    // -- Colored Shadow Penumbra (Kai) --
    new renodx::utils::settings::Setting{
      .key = "KaiPenumbraMode", .binding = &shader_injection.shadow_edge_tint_kai,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 1.f, .label = "Colored Penumbra", .section = "Shadows",
      .tooltip = "Improved mode applies vibrance boost in shadow penumbra regions. No Falcom fallback on Kai.",
      .labels = {"Off", "Improved"},
      .is_visible = []() { return IsKai(); },
    },
    // �� Character hero light (Kai) ��
    new renodx::utils::settings::Setting{
      .key = "KaiCharacterLight", .binding = &shader_injection.character_light_strength,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.f, .label = "Hero Light Suppression", .section = "Shadows",
      .tooltip = "Suppresses dynamic point/spot lights on character pixels. 0=off (vanilla), 1=fully remove.",
      .min = 0.f, .max = 1.0f, .format = "%.2f",
      .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "KaiPenumbraStrength", .binding = &shader_injection.shadow_penumbra_color_strength,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.15f, .label = "Penumbra Strength", .section = "Shadows",
      .tooltip = "Overall strength of the colored penumbra effect.",
      .min = 0.0f, .max = 2.0f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.shadow_edge_tint_kai >= 1.0f; },
      .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "KaiPenumbraVibrance", .binding = &shader_injection.shadow_penumbra_vibrance,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 30.f, .label = "Penumbra Vibrance", .section = "Shadows",
      .tooltip = "Vibrance applied to surface color in penumbra. 0=grayscale, 1=neutral, >1=vivid.",
      .min = 0.0f, .max = 100.0f, .format = "%.1f",
      .is_enabled = []() { return shader_injection.shadow_edge_tint_kai >= 1.0f; },
      .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "KaiPenumbraDetection", .binding = &shader_injection.shadow_penumbra_detection,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 2.0f, .label = "Penumbra Detection", .section = "Shadows",
      .tooltip = "Penumbra detection width. Higher = wider area gets the effect.",
      .min = 0.01f, .max = 2.0f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.shadow_edge_tint_kai >= 1.0f; },
      .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "KaiPenumbraBrightness", .binding = &shader_injection.shadow_penumbra_color_brightness,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 1.f, .label = "Penumbra Brightness", .section = "Shadows",
      .tooltip = "Brightness multiplier for the penumbra tint color.",
      .min = 0.0f, .max = 5.0f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.shadow_edge_tint_kai >= 1.0f; },
      .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "KaiPenumbraDebug", .binding = &shader_injection.shadow_penumbra_debug_view,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 0.f, .label = "Penumbra Debug", .section = "Shadows",
      .tooltip = "Visualize penumbra: 0=Off, 1=Detection Mask, 2=Tint Color, 3=Result, 4=Sun Color.",
      .labels = {"Off", "Penumbra Mask", "Tint Color", "Result", "Sun Color"},
      .is_enabled = []() { return shader_injection.shadow_edge_tint >= 1.0f; },
      .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    // -- Micro Shadows --------------------------------------------------
    // An AO-driven aperture term applied to NdotL. Replaces the retired Bend_SSS
    // character ray-march as the soft half of contact shading.
    new renodx::utils::settings::Setting{
      .key = "MicroShadowsEnabled", .binding = &shader_injection.cs_micro_enabled,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 0.f, .label = "Micro Shadows", .section = "Micro Shadows",
      .tooltip = "An AO-driven aperture term applied to NdotL (Uncharted 4). A surface "
                 "buried in ambient occlusion stops receiving as much key light, which "
                 "restores the soft darkening a rasteriser cannot produce on its own.",
      .labels = {"Off", "On"},
      .is_visible = []() { return !IsKyoto() && !IsDaybreak2(); },
    },
    new renodx::utils::settings::Setting{
      .key = "MicroShadowsEnvStrength", .binding = &shader_injection.cs_micro_env_strength,
      .default_value = 1.f, .label = "Environment Strength", .section = "Micro Shadows",
      .tooltip = "Strength of the micro term on non-character pixels.",
      .min = 0.f, .max = 1.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.cs_micro_enabled >= 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "MicroShadowsCharStrength", .binding = &shader_injection.cs_micro_char_strength,
      .default_value = 1.f, .label = "Character Strength", .section = "Micro Shadows",
      .tooltip = "Strength of the micro term on character pixels, selected by the "
                 "foliage/character mask.",
      .min = 0.f, .max = 1.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.cs_micro_enabled >= 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "MicroShadowsOpacity", .binding = &shader_injection.cs_micro_opacity,
      .default_value = 1.f, .label = "Opacity", .section = "Micro Shadows",
      .tooltip = "How far the micro term replaces the diffuse NdotL term. 1 applies it "
                 "fully; 0 leaves the surface unshadowed.",
      .min = 0.f, .max = 1.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.cs_micro_enabled >= 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "MicroShadowsAperture", .binding = &shader_injection.cs_micro_aperture_scale,
      .default_value = 1.f, .label = "Aperture", .section = "Micro Shadows",
      .tooltip = "Scales the 2*AO*AO aperture. RAISE it to open the aperture, i.e. to "
                 "let more light in and REDUCE the micro shadow. LOWER it to deepen the "
                 "shadow; 0 disables the term entirely without turning the pass off.",
      .min = 0.f, .max = 4.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.cs_micro_enabled >= 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "MicroShadowsAOSource", .binding = &shader_injection.cs_micro_ao_source,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 1.f, .label = "AO Source", .section = "Micro Shadows",
      .tooltip = "Which occlusion the pass reads. Both sources are bound every frame and "
                 "decoded in their own encoding, because reading either as the wrong type "
                 "yields plausible garbage rather than an error. GTVBAO is the default "
                 "because the game's own deferred AO is not captured on every frame, and "
                 "with AO = 1 the micro term saturate(NdotL + 2*AO*AO - 1) is identically "
                 "1 everywhere, i.e. no visible effect. The log warns if you pick GTVBAO "
                 "while GTVBAO is disabled.",
      .labels = {"Game SSAO", "GTVBAO"},
      .is_enabled = []() { return shader_injection.cs_micro_enabled >= 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "MicroShadowsISFAST", .binding = &shader_injection.cs_micro_isfast_enabled,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 0.f, .label = "IS-FAST Dither", .section = "Micro Shadows",
      .tooltip = "Off by default. The micro term has no ray to jitter, so this does NOT "
                 "dither a march. It dithers the AO QUANTISATION: GTVBAO stores "
                 "visibility in a single byte, so a slowly varying surface steps through "
                 "discrete AO cells and 2*AO*AO turns that into visible banding. This "
                 "reads the AO one texel off in a blue-noise direction and lets TAA "
                 "average it back. Turn it on only if you see stepping on large smooth "
                 "surfaces under GTVBAO; it trades a little noise for that.",
      .labels = {"Off", "On"},
      .is_enabled = []() { return shader_injection.cs_micro_enabled >= 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "MicroShadowsDebug", .binding = &shader_injection.cs_micro_debug,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 0.f, .label = "Debug View", .section = "Micro Shadows",
      .tooltip = "1 shows the raw micro term (white = lit). 2 shows diagnostics: R = term, "
                 "G = the AO actually used, B = marker. G reading 1 everywhere means the "
                 "occlusion source was not captured, which is the one failure mode that is "
                 "indistinguishable from 'the effect does nothing'.",
      .labels = {"Off", "Raw Term", "Diagnostics"},
      .is_enabled = []() { return shader_injection.cs_micro_enabled >= 0.5f; },
    },
    // -- Contact Shadows ------------------------------------------------
    new renodx::utils::settings::Setting{
      .key = "ContactShadowsEnabled", .binding = &shader_injection.cs_contact_enabled,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 0.f, .label = "Contact Shadows", .section = "Contact Shadows",
      .tooltip = "A clip-space depth march from each surface toward the light, jittered with "
                 "IS-FAST blue noise and resolved by the game's TAA. This is the term that "
                 "grounds characters and props; Micro Shadows only handles the soft "
                 "ambient half.",
      .labels = {"Off", "On"},
      .is_visible = []() { return !IsKyoto() && !IsDaybreak2(); },
    },
    new renodx::utils::settings::Setting{
      .key = "ContactShadowsEnvStrength", .binding = &shader_injection.cs_contact_env_strength,
      .default_value = 1.f, .label = "Environment Strength", .section = "Contact Shadows",
      .tooltip = "Strength of the contact term on non-character pixels.",
      .min = 0.f, .max = 1.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.cs_contact_enabled >= 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "ContactShadowsCharStrength", .binding = &shader_injection.cs_contact_char_strength,
      .default_value = 1.f, .label = "Character Strength", .section = "Contact Shadows",
      .tooltip = "Strength of the contact term on character pixels, selected by the "
                 "foliage/character mask.",
      .min = 0.f, .max = 1.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.cs_contact_enabled >= 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "ContactShadowsSamples", .binding = &shader_injection.cs_contact_sample_count,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 8.f, .label = "Sample Count", .section = "Contact Shadows",
      .tooltip = "March steps. 8 is what the reference settled on, and the same order "
                 "Unreal uses. Beyond 16 the gain is small and the cost is linear.",
      .min = 1.f, .max = 32.f, .format = "%d",
      .is_enabled = []() { return shader_injection.cs_contact_enabled >= 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "ContactShadowsRayLength", .binding = &shader_injection.cs_contact_ray_length,
      .default_value = 2.f, .label = "Ray Length", .section = "Contact Shadows",
      .tooltip = "World-space distance the ray travels toward the light. Short values keep "
                 "the term to contact; long values reach genuine occluders.",
      .min = 1.f, .max = 200.f, .format = "%.1f",
      .is_enabled = []() { return shader_injection.cs_contact_enabled >= 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "ContactShadowsThickness", .binding = &shader_injection.cs_contact_thickness,
      .default_value = 0.29f, .label = "Surface Thickness", .section = "Contact Shadows",
      .tooltip = "How deep a hit must penetrate before it counts. Raise it to stop thin "
                 "geometry and depth noise from self-shadowing.",
      .min = 0.001f, .max = 4.f, .format = "%.3f",
      .is_enabled = []() { return shader_injection.cs_contact_enabled >= 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "ContactShadowsBias", .binding = &shader_injection.cs_contact_bias,
      .default_value = 0.0001f, .label = "Depth Bias", .section = "Contact Shadows",
      .tooltip = "Minimum penetration before a hit counts, in device depth. Raise only if "
                 "you see acne.",
      .min = 0.f, .max = 0.2f, .format = "%.4f",
      .is_enabled = []() { return shader_injection.cs_contact_enabled >= 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "ContactShadowsNormalBias", .binding = &shader_injection.cs_contact_normal_bias,
      .default_value = 0.1000f, .label = "Normal Bias", .section = "Contact Shadows",
      .tooltip = "Lifts the ray origin along the surface normal. More stable than depth "
                 "bias on curved surfaces.",
      .min = 0.f, .max = 1.f, .format = "%.4f",
      .is_enabled = []() { return shader_injection.cs_contact_enabled >= 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "ContactShadowResponseScale",
      .binding = &shader_injection.cs_contact_response_scale,
      .default_value = 4.f, .label = "Contact Shadow Response Scale",
      .section = "Contact Shadows",
      .tooltip = "How strong the contact shadow reads, relative to the ORIGINAL "
                 "4-sample contact shadow appearance. This is NOT a physically based "
                 "parameter and 4 is not a physical unit: 4 is simply the sample count "
                 "whose average appearance is being reproduced, so that Sample Count can "
                 "be a pure quality control. At Sample Count 4 and scale 4 the march "
                 "reproduces the old any-hit test exactly, value for value. Raise this "
                 "for a darker contact, lower it for a lighter one; Sample Count then "
                 "only changes how noisy the result is, not how wide or how dark it is. "
                 "Defaults to 4.",
      .min = 0.f, .max = 8.f, .format = "%.1f",
      .is_enabled = []() { return shader_injection.cs_contact_enabled >= 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "ContactShadowsSunRange", .binding = &shader_injection.cs_contact_sun_range,
      .default_value = 0.f, .label = "Sun Range", .section = "Contact Shadows",
      .tooltip = "World distance past which sun contact shadows stop being applied, "
                 "because the CSM no longer has real coverage there. 0 = use the "
                 "engine's own last cascade split. This is the gate that keeps contact "
                 "shadows off interior walls: indoors the engine keeps a sun term "
                 "because the cascade clamps past its split instead of falling off, so "
                 "without this the effect darkens rooms. Raise it only if contact "
                 "shadows vanish too early outdoors; lower it if they still reach into "
                 "interiors. Contact shadows are a near-field effect, so a smaller "
                 "value is usually the honest one.",
      .min = 0.f, .max = 500.f, .format = "%.1f",
      .is_enabled = []() { return shader_injection.cs_contact_enabled >= 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "ContactShadowsSkyDepth", .binding = &shader_injection.cs_contact_sky_depth,
      .default_value = 100000.f, .label = "Sky Depth", .section = "Contact Shadows",
      .tooltip = "A device depth linearizing beyond this counts as no geometry, so the "
                 "march is skipped early. Higher is safer, lower is cheaper on open sky.",
      .min = 100.f, .max = 1000000.f, .format = "%.0f",
      .is_enabled = []() { return shader_injection.cs_contact_enabled >= 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "ContactShadowsMaxDarkening", .binding = &shader_injection.cs_contact_max_darkening,
      .default_value = 0.5f, .label = "Max Darkening", .section = "Contact Shadows",
      .tooltip = "Caps how dark a contact-shadowed pixel may get. 1 allows full "
                 "black, 0 disables the darkening entirely. Lower it to keep "
                 "contact shadows from crushing in dark scenes.",
      .min = 0.f, .max = 1.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.cs_contact_enabled >= 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "ContactShadowsISFAST", .binding = &shader_injection.cs_contact_isfast_enabled,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 1.f, .label = "IS-FAST Jitter", .section = "Contact Shadows",
      .tooltip = "Jitters the march with the IS-FAST blue-noise volume and lets TAA resolve "
                 "it. There is no IGN fallback: without the volume the march runs unjittered "
                 "and bands visibly, which the log reports once.",
      .labels = {"Off", "On"},
      .is_enabled = []() { return shader_injection.cs_contact_enabled >= 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "ContactShadowsDebug", .binding = &shader_injection.cs_contact_debug,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 0.f, .label = "Debug View", .section = "Contact Shadows",
      .tooltip = "1 shows the raw contact term (white = lit). 2 shows diagnostics: R = term, "
                 "G = stage code (0 marched, 1 no normal, 2 bad projection, 3 sky, 4 bad "
                 "depth), B = linear depth. G of 0 with R of 1 means the march runs and finds "
                 "nothing; G of 1, 2 or 4 means it bails out before marching.",
      .labels = {"Off", "Raw Term", "Diagnostics"},
      .is_enabled = []() { return shader_injection.cs_contact_enabled >= 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "ContactShadowsLocalEnabled", .binding = &shader_injection.cs_contact_local_enabled,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 1.f, .label = "Local Lights", .section = "Contact Shadows",
      .tooltip = "Extends the contact march to point and spot lights, evaluated inside the "
                 "dynamic light loop. Off by default: it is per-light work in a pixel "
                 "shader, so the cost scales with the per-pixel light count instead of being "
                 "one full-screen pass.",
      .labels = {"Off", "On"},
      .is_enabled = []() { return shader_injection.cs_contact_enabled >= 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "ContactShadowsLocalStrength", .binding = &shader_injection.cs_contact_local_strength,
      .default_value = 1.f, .label = "Local Strength", .section = "Contact Shadows",
      .min = 0.f, .max = 1.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.cs_contact_local_enabled >= 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "ContactShadowsLocalSamples", .binding = &shader_injection.cs_contact_local_sample_count,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 4.f, .label = "Local Sample Count", .section = "Contact Shadows",
      .tooltip = "March steps per local light. Keep this low: it is paid once per light, per "
                 "pixel.",
      .min = 2.f, .max = 16.f, .format = "%d",
      .is_enabled = []() { return shader_injection.cs_contact_local_enabled >= 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "ContactShadowsLocalRayLength", .binding = &shader_injection.cs_contact_local_ray_length,
      .default_value = 2.f, .label = "Local Ray Length", .section = "Contact Shadows",
      .tooltip = "World-space ray length for the local march. Local lights are short-range, "
                 "so this is much smaller than the sun ray length above.",
      .min = 0.1f, .max = 20.f, .format = "%.1f",
      .is_enabled = []() { return shader_injection.cs_contact_local_enabled >= 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "ContactShadowsLocalMaxLights", .binding = &shader_injection.cs_contact_local_max_lights,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 16.f, .label = "Local Light Budget", .section = "Contact Shadows",
      .tooltip = "Per-pixel cap on how many local lights get a march, so a crowded scene "
                 "cannot multiply the cost without limit.",
      .min = 1.f, .max = 16.f, .format = "%d",
      .is_enabled = []() { return shader_injection.cs_contact_local_enabled >= 0.5f; },
    },

    // �� Dynamic Cubemaps � standalone t17 replacement ��
    new renodx::utils::settings::Setting{
      .key = "DynCubeEnabled", .binding = &shader_injection.dynCube_enabled,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 1.f, .label = "Dynamic Cubemaps", .section = "Dynamic Cubemaps",
      .tooltip = "Dynamic cubemap generation replacing game's static cubemaps.",
      .labels = {"Off", "On"},
    },
    new renodx::utils::settings::Setting{
      .key = "DynCubeHistory", .binding = &shader_injection.dynCube_history,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 1.f, .label = "Temporal", .section = "Dynamic Cubemaps",
      .tooltip = "Phase1: temporal accumulation. On = blend current capture with previous history. Off = no history (fresh capture each frame).",
      .labels = {"Off", "On"},
      .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "DynCubeDebug", .binding = &shader_injection.dynCube_debug,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 0.f, .label = "Debug View", .section = "Dynamic Cubemaps",
      .tooltip = "0=Normal (dynamic cube in lighting), 1=Show Dynamic Cube, 2=Face Visualization, 3=Solid Face Colors, 4=No Override (vanilla t17), 5=History Validity, 6=History Contribution, 7=Character Mask, 8=GGX Filtered Cube (mip-selectable), 9=SSR Result (blurred), 10=SSR Confidence, 11=Reflection Source, 12=SSR Raw, 13=SSR Edge Fade.",
      .labels = {"Normal", "Show Cube", "Face Viz", "Solid Colors", "No Override", "History Validity", "History Contribution", "Character Mask", "GGX Filtered", "SSR Result", "SSR Confidence", "Reflection Source", "SSR Raw", "SSR Edge Fade"},
      .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "DynCubeResolution", .binding = &shader_injection.dynCube_resolution,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 4.f, .label = "Resolution", .section = "Dynamic Cubemaps",
      .tooltip = "Cubemap face size. 1536+ costs significant VRAM (a full 2048 set needs ~2GB with history+GGX). Preview rectangle is clamped for visibility.",
      .labels = {"128", "256", "512", "768", "1024", "1536", "2048"},
      .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "DynCubeCharacterCapture", .binding = &shader_injection.dynCube_character_capture,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 0.f, .label = "Character Capture", .section = "Dynamic Cubemaps",
      .tooltip = "OFF = exclude characters from cubemap capture. ON = include characters in cubemap capture.",
      .labels = {"Off (Exclude)", "On (Include)"},
      .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "DynCubeForceVanilla", .binding = &shader_injection.dynCube_force_vanilla,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 0.f, .label = "Force Vanilla Cubemaps", .section = "Dynamic Cubemaps",
      .tooltip = "OFF = use dynamic cubemap for t17. ON = keep vanilla game cubemap for t17 (A/B test). Does not stop dynamic cubemap accumulation.",
      .labels = {"Off", "On"},
      .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "DynCubeForceDynamic", .binding = &shader_injection.dynCube_force_dynamic,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 0.f, .label = "Force Dynamic Cubemaps", .section = "Dynamic Cubemaps",
      .tooltip = "Debug: render the dynamic cubemap only (skip SSR and vanilla). Precedence: Force Vanilla > Force Dynamic > Force SSR.",
      .labels = {"Off", "On"},
      .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "DynCubeForceSSR", .binding = &shader_injection.dynCube_force_ssr,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 0.f, .label = "Force SSR", .section = "Dynamic Cubemaps",
      .tooltip = "Debug: render SSR only (skip dynamic and vanilla). Requires SSR On. Precedence: Force Vanilla > Force Dynamic > Force SSR.",
      .labels = {"Off", "On"},
      .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f && shader_injection.dynCube_ssr_enabled > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "DynCubeLayerMix", .binding = &shader_injection.dynCube_layer_mix,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = -1.f, .label = "Reflection Layer Mix", .section = "Dynamic Cubemaps",
      .tooltip = "Manual reflection source override. -1 = automatic confidence blend. 0 = SSR only. 1 = Dynamic only. 2 = Vanilla only. Values in between blend adjacent sources.",
      .min = -1.f, .max = 2.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "DynCubeBlur", .binding = &shader_injection.dynCube_blur,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 2.f, .label = "Dynamic Cubemap Blur", .section = "Dynamic Cubemaps",
      .tooltip = "Artistic mip-offset blur on the dynamic cube sample (uses the existing GGX/HW mip chain, no extra pass). 0 = normal sharpness; fractional values give smooth trilinear control; higher = progressively blurrier. Independent of material roughness.",
      .min = 0.f, .max = 8.f, .format = "%.1f",
      .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "DynCubeUpdateInterval", .binding = &shader_injection.dynCube_capture_interval,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 3.f, .label = "Dynamic Cubemap Update Interval", .section = "Dynamic Cubemaps",
      .tooltip = "Frames between new cubemap captures (1=fastest 2-stage cadence, 2=every 2 frames, 4=every 4 frames, 8=every 8 frames). Capture and filter run on separate frames; the previous completed cube stays visible between updates.",
      .labels = {"1", "2", "3", "4", "5", "6", "7", "8"},
      .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "DynCubeGGX", .binding = &shader_injection.dynCube_ggx,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 0.f, .label = "GGX Prefilter", .section = "Dynamic Cubemaps",
      .tooltip = "Phase3: 16-tap Hammersley GGX prefilter for roughness mips. Off = hardware box-filter mips (cheap approx).",
      .labels = {"HW Mips", "GGX"},
      .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "DynCubeDebugLog", .binding = &shader_injection.dynCube_debug_logging,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 0.f, .label = "Debug Logging (1/sec)", .section = "Dynamic Cubemaps",
      .tooltip = "Emits one log per second to help diagnose which part is broken: resource creation, capture inputs (depth/color/cbv), dispatch success, t17 override, and debug mode.",
      .labels = {"Off", "On"},
      .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "CustomShaderLogging", .binding = &shader_injection.custom_shader_logging,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 0.f, .label = "Custom Shader Log", .section = "Dynamic Cubemaps",
      .tooltip = "Throttled step log for GTVBAO + Dynamic Cubemaps + SSR dispatches (sizes, inputs, create/destroy). Arm before a resolution/DLSS change repro: the last line marks the crash step.",
      .labels = {"Off", "On"},
      .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f || shader_injection.gtvbao_mode > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "DynCubeCaptureBoost", .binding = &shader_injection.dynCube_capture_boost,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.66f, .label = "Reflection Brightness", .section = "Dynamic Cubemaps",
      .tooltip = "Master brightness for SSR and dynamic cubemap reflections. 1.0 = neutral. Does not affect the vanilla fallback.",
      .min = 0.f, .max = 4.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "DynCubeCaptureSoften", .binding = &shader_injection.dynCube_capture_soften,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.4f, .label = "Capture Soften", .section = "Dynamic Cubemaps",
      .tooltip = "Softens globally-pushed dynamic reflections (glass etc.) via a dedicated variant cube. Does not affect the lighting resolve or the vanilla fallback.",
      .min = 0.f, .max = 1.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "DynCubeGlobalStrength", .binding = &shader_injection.dynCube_global_strength,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.4f, .label = "Global Reflection Strength", .section = "Dynamic Cubemaps",
      .tooltip = "Scales globally-pushed dynamic reflections (glass etc.) so they don't dominate. 1 = full. Does not affect the lighting resolve or the vanilla fallback.",
      .min = 0.f, .max = 1.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "DynCubeHistoryBlend", .binding = &shader_injection.dynCube_history_blend,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.75f, .label = "History Blend", .section = "Dynamic Cubemaps",
      .tooltip = "Temporal blend weight when current and previous samples are compatible. 0.5 = Skyrim-style 50/50.",
      .min = 0.f, .max = 1.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f && shader_injection.dynCube_history > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "DynCubeHistoryPosThreshold", .binding = &shader_injection.dynCube_history_pos_threshold,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.5f, .label = "History Pos Threshold", .section = "Dynamic Cubemaps",
      .tooltip = "World-unit position compatibility threshold. If previous vs current reconstructed position differs more than this, history is replaced. Starting point 0.5; tune after 90-deg and slow-pan tests.",
      .min = 0.f, .max = 20.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f && shader_injection.dynCube_history > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "DynCubeDebugFace", .binding = &shader_injection.dynCube_debug_face,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 0.f, .label = "Debug Face", .section = "Dynamic Cubemaps",
      .tooltip = "Face for debug preview when Debug View=1/2/5/6/7/8: 0=+X 1=-X 2=+Y 3=-Y 4=+Z 5=-Z. Preview samples the capture resource directly.",
      .labels = {"+X", "-X", "+Y", "-Y", "+Z", "-Z"},
      .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f && (shader_injection.dynCube_debug == 1.f || shader_injection.dynCube_debug == 2.f || shader_injection.dynCube_debug == 5.f || shader_injection.dynCube_debug == 6.f || shader_injection.dynCube_debug == 7.f || shader_injection.dynCube_debug == 8.f); },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "DynCubeDebugMip", .binding = &shader_injection.dynCube_debug_mip,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 0.f, .label = "Debug Mip", .section = "Dynamic Cubemaps",
      .tooltip = "Mip level for the GGX filtered cube preview when Debug View=8. 0=sharp history, 7=strongly blurred.",
      .labels = {"0", "1", "2", "3", "4", "5", "6", "7"},
      .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f && shader_injection.dynCube_debug == 8.f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "DynCubeForceMip", .binding = &shader_injection.dynCube_force_mip,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = -1.f, .label = "Force Cubemap Mip", .section = "Dynamic Cubemaps",
      .tooltip = "Debug: force the t17 reflection mip. -1 = normal roughness LOD. 0..7 = always sample that mip (verifies the GGX chain reaches the reflection).",
      .min = -1.f, .max = 7.f, .format = "%d",
      .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "DynCubeCoverageFade", .binding = &shader_injection.dynCube_coverage_fade,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 1.f, .label = "Dynamic Cubemap Coverage Fade", .section = "Dynamic Cubemaps",
      .tooltip = "OFF = hard validity cutoff (current). ON = smooth the dynamic-cubemap validity edge in direction space so coverage boundaries fade into the fallback instead of cutting.",
      .labels = {"Off", "On"},
      .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "DynCubeCoverageWidth", .binding = &shader_injection.dynCube_coverage_width,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 1.f, .label = "Dynamic Cubemap Coverage Fade Width", .section = "Dynamic Cubemaps",
      .tooltip = "Angular width of the validity smoothing cone in degrees. Controls transition softness only, not the valid boundary. 0 = binary cutoff.",
      .min = 0.f, .max = 8.f, .format = "%.1f",
      .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f && shader_injection.dynCube_coverage_fade > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "DynCubeVerticalOffset", .binding = &shader_injection.dynCube_vertical_offset,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.f, .label = "Dynamic Cubemap Vertical Offset (Test)", .section = "Dynamic Cubemaps",
      .tooltip = "TEST: vertically tilt the dynamic cubemap lookup, in degrees. 0 = no change. Positive slides reflection content down, negative slides it up. Validity and vanilla fallback follow the shifted direction.",
      .min = -30.f, .max = 30.f, .format = "%.1f",
      .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "DynCubeSSR", .binding = &shader_injection.dynCube_ssr_enabled,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 1.f, .label = "SSR", .section = "Dynamic Cubemaps",
      .tooltip = "Simple screen-space SSR ray march (no Hi-Z/temporal). Confidence blend: SSR > Dynamic Cube > Vanilla Cube.",
      .labels = {"Off", "On"},
      .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "DynCubeSSRSamples", .binding = &shader_injection.dynCube_ssr_samples,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 16.f, .label = "SSR Sample Count", .section = "Dynamic Cubemaps",
      .tooltip = "SSR ray-march sample count. Higher = more accurate at higher GPU cost. Old Medium preset = 16.",
      .min = 4.f, .max = 96.f, .format = "%d",
      .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f && shader_injection.dynCube_ssr_enabled > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "DynCubeSSRDistance", .binding = &shader_injection.dynCube_ssr_distance,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 192.f, .label = "SSR Search Distance", .section = "Dynamic Cubemaps",
      .tooltip = "SSR ray-march search distance in world units. Old Medium preset = 20.",
      .min = 4.f, .max = 192.f, .format = "%.1f",
      .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f && shader_injection.dynCube_ssr_enabled > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "DynCubeSSRBlur", .binding = &shader_injection.dynCube_ssr_blur,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 4.f, .label = "SSR Blur", .section = "Dynamic Cubemaps",
      .tooltip = "Separable Gaussian blur sigma on the SSR result. 0 = sharp/raw SSR, higher = progressively blurrier.",
      .min = 0.f, .max = 8.f, .format = "%.1f",
      .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f && shader_injection.dynCube_ssr_enabled > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "DynCubeSSRSymmetricWeights", .binding = &shader_injection.dynCube_ssr_symmetric_weights,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 1.f, .label = "SSR Symmetric Blur Weights (Test)", .section = "Dynamic Cubemaps",
      .tooltip = "TEST A/B: evaluate each blur tap pair with a single shared Gaussian weight. Off = legacy per-tap loop (default, unchanged behavior).",
      .labels = {"Off", "On"},
      .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f && shader_injection.dynCube_ssr_enabled > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "DynCubeSSRDistanceFade", .binding = &shader_injection.dynCube_ssr_distance_fade,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.f, .label = "SSR Distance Fade", .section = "Dynamic Cubemaps",
      .tooltip = "How fast SSR confidence drops with hit distance. 0 = no falloff, 1 = full falloff at max distance.",
      .min = 0.f, .max = 1.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f && shader_injection.dynCube_ssr_enabled > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "DynCubeSSREdgeFade", .binding = &shader_injection.dynCube_ssr_edge_fade,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.2f, .label = "SSR Edge Fade", .section = "Dynamic Cubemaps",
      .tooltip = "Screen-edge vignette strength on SSR confidence. 0 = none, 1 = fades over 25% from each edge.",
      .min = 0.f, .max = 1.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f && shader_injection.dynCube_ssr_enabled > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "DynCubeSSRGrazingFade", .binding = &shader_injection.dynCube_ssr_grazing_fade,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.f, .label = "SSR Grazing Fade", .section = "Dynamic Cubemaps",
      .tooltip = "Reduces SSR confidence at grazing view angles. 0 = none, 1 = aggressive fade.",
      .min = 0.f, .max = 1.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f && shader_injection.dynCube_ssr_enabled > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "DynCubeSSRThickness", .binding = &shader_injection.dynCube_ssr_thickness,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 1.0f, .label = "SSR Thickness", .section = "Dynamic Cubemaps",
      .tooltip = "Depth tolerance: how far behind the surface the ray must be to count as a hit (world units).",
      .min = 0.01f, .max = 1.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f && shader_injection.dynCube_ssr_enabled > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "DynCubeSSRCharOccStrength", .binding = &shader_injection.dynCube_ssr_char_occ_strength,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.f, .label = "SSR Character Occlusion Reduction", .section = "Dynamic Cubemaps",
      .tooltip = "How much SSR confidence is reduced when the ray hits a character from a horizontal (floor/water) surface. Attacks third-person disocclusion rings; 0 = off. Vertical mirrors (low upness) are unaffected.",
      .min = 0.f, .max = 1.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f && shader_injection.dynCube_ssr_enabled > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "DynCubeSSRCharOccUpness", .binding = &shader_injection.dynCube_ssr_char_occ_upness,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.f, .label = "SSR Character Occlusion Upness", .section = "Dynamic Cubemaps",
      .tooltip = "Surface upness threshold where the character-hit confidence reduction begins (smooth �0.25 band). 0 = any up-facing surface, 0.5 = mostly horizontal, 1 = only fully horizontal (floor/water).",
      .min = 0.f, .max = 1.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f && shader_injection.dynCube_ssr_enabled > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "DynCubeSSRConfidenceFallback", .binding = &shader_injection.dynCube_ssr_confidence_fallback,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.f, .label = "SSR Confidence Fallback", .section = "Dynamic Cubemaps",
      .tooltip = "AUTO-mode remap of SSR confidence: 0 = today's weighting; higher values suppress low-confidence SSR sooner so Dynamic Cubemap takes over earlier. Smooth, no hard cutoff; conf = 1 always stays full SSR.",
      .min = 0.f, .max = 0.9f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f && shader_injection.dynCube_ssr_enabled > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "DynCubeSSRReplacement", .binding = &shader_injection.dynCube_ssr_replacement,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 1.f, .label = "SSR Replacement", .section = "Dynamic Cubemaps",
      .tooltip = "Replace the Sora (1st/2nd) / Kai water SSR resolve with the DynCube composite (game march + dynamic cubemap, misses decay with no vanilla-cube tint) on water-flagged pixels only. Bed and non-water pixels run verbatim vanilla. Off = fully vanilla SSR chain, nothing touched.",
      .labels = {"Off", "On"},
      .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "DynCubeGameSSR", .binding = &shader_injection.dynCube_game_ssr,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 1.f, .label = "Game SSR", .section = "Dynamic Cubemaps",
      .tooltip = "Run the game's own SSR march and feed it to the SSR Replacement composite (game reflection, then dynamic cubemap, misses decay with no vanilla-cube tint). Off skips the march: water receives dynamic cubemap only. Only applies while SSR Replacement is on; otherwise the vanilla chain runs untouched.",
      .labels = {"Off", "On"},
      .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f && shader_injection.dynCube_ssr_replacement > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "DynCubeSSRReplacementDebug", .binding = &shader_injection.dynCube_ssr_replacement_debug,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 0.f, .label = "SSR Replacement Debug", .section = "Dynamic Cubemaps",
      .tooltip = "Visualize the SSR chain stages fullscreen on lit surfaces (Off, ssr1 input scene color, ssr1 march result, lighting SSR texture, lighting color). For judging what each stage sees.",
      .labels = {"Off", "SSR1 Input", "SSR2 Input", "Lighting SSR", "Lighting Color"},
      .min = 0.f, .max = 4.f, .format = "%d",
      .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    // -- Vanilla SSR Improvements (Sora2nd march/denoise correctness, A/B) --
    new renodx::utils::settings::Setting{
      .key = "DynCubeVanillaSSREnabled", .binding = &shader_injection.dynCube_vanilla_ssr_enabled,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 1.f, .label = "Enable Improvements", .section = "Vanilla SSR Improvements",
      .tooltip = "Improvements to Vanilla SSR denoising.",
      .labels = {"Off", "On"},
    },
    new renodx::utils::settings::Setting{
      .key = "DynCubeVanillaRefineFix", .binding = &shader_injection.dynCube_vanilla_refine_fix,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 1.f, .label = "Refine Backtrack Fix", .section = "Vanilla SSR Improvements",
      .tooltip = "Fix SSR refinement to bracket the crossing (step back when inside, Kai-style) instead of stepping only forward. Sora2nd ssr1, Sora1st and Kai fused marches. Off restores the verbatim vanilla behavior for A/B.",
      .labels = {"Off", "On"},
      .is_enabled = []() { return shader_injection.dynCube_vanilla_ssr_enabled > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "DynCubeVanillaRefineThreshold", .binding = &shader_injection.dynCube_vanilla_refine_threshold,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.5f, .label = "Refine Hit Threshold", .section = "Vanilla SSR Improvements",
      .tooltip = "Hit threshold on the refine depth delta (scene depth minus ray depth, view units): hit when delta exceeds this. 0 = any penetration counts (native Kai rule); higher is stricter (fewer, firmer hits). Sora2nd ssr1, Sora1st and Kai fused marches. Only applies while Refine Backtrack Fix is on.",
      .min = 0.f, .max = 1.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.dynCube_vanilla_ssr_enabled > 0.5f && shader_injection.dynCube_vanilla_refine_fix > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "DynCubeVanillaHistoryFixed", .binding = &shader_injection.dynCube_vanilla_history_fixed,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 0.f, .label = "Fixed History Blend", .section = "Vanilla SSR Improvements",
      .tooltip = "On = fixed history weight from the slider below (0.9 = vanilla); Off = motion-adaptive weighting (Kai formula: mostly history when static, mostly current when moving).",
      .labels = {"Off", "On"},
      .is_enabled = []() { return shader_injection.dynCube_vanilla_ssr_enabled > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "DynCubeVanillaHistoryWeight", .binding = &shader_injection.dynCube_vanilla_history_weight,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.9f, .label = "Fixed History Weight", .section = "Vanilla SSR Improvements",
      .tooltip = "History fraction used when Fixed History Blend is on (0.9 = vanilla). Higher = longer trails but stabler; lower = fresher but noisier.",
      .min = 0.5f, .max = 0.99f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.dynCube_vanilla_ssr_enabled > 0.5f && shader_injection.dynCube_vanilla_history_fixed > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "DynCubeVanillaDisocReject", .binding = &shader_injection.dynCube_vanilla_disoc_reject,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 1.f, .label = "Disocclusion Reject", .section = "Vanilla SSR Improvements",
      .tooltip = "Validate the reprojected history sample against current-frame scene data (UV bounds, motion distance, depth mismatch); on mismatch use the current SSR result instead of stale history.",
      .labels = {"Off", "On"},
      .is_enabled = []() { return shader_injection.dynCube_vanilla_ssr_enabled > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "DynCubeVanillaDisocDepth", .binding = &shader_injection.dynCube_vanilla_disoc_depth,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.15f, .label = "Disocclusion Depth Threshold", .section = "Vanilla SSR Improvements",
      .tooltip = "Reject history when |linear depth at history UV minus linear depth here| exceeds this (view units). 0 = always reject on depth. Only applies while Disocclusion Reject is on.",
      .min = 0.f, .max = 2.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.dynCube_vanilla_ssr_enabled > 0.5f && shader_injection.dynCube_vanilla_disoc_reject > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "DynCubeVanillaDisocUV", .binding = &shader_injection.dynCube_vanilla_disoc_uv,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.05f, .label = "Disocclusion Motion Threshold", .section = "Vanilla SSR Improvements",
      .tooltip = "Reject history when reprojection motion exceeds this (0-1 UV units, ~5% of screen at default). 0 = any motion rejects. Only applies while Disocclusion Reject is on.",
      .min = 0.f, .max = 0.25f, .format = "%.3f",
      .is_enabled = []() { return shader_injection.dynCube_vanilla_ssr_enabled > 0.5f && shader_injection.dynCube_vanilla_disoc_reject > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "DynCubeVanillaISFAST", .binding = &shader_injection.dynCube_vanilla_isfast,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 1.f, .label = "IS-FAST History Distribution", .section = "Vanilla SSR Improvements",
      .tooltip = "Blue-noise subpixel distribution of the single history tap (same 128x128x32 volume, frame slice, spatial scale and strength blend as the custom SSR IS-FAST phase; strength 0 = off). Gives TAA/upscalers a temporally distributed signal. Requires IS-FAST noise available: turn on ISFASTMasterEnable and check the IS-FAST debug log for loaded=yes; the effect is temporal stability in motion, not single-frame denoising.",
      .labels = {"Off", "On"},
      .is_enabled = []() { return shader_injection.dynCube_vanilla_ssr_enabled > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    // -- Character Outline (Sora 1st / 2nd) --
    new renodx::utils::settings::Setting{
      .key = "CharOutlineIntensity", .binding = &shader_injection.char_outline_intensity,
      .default_value = 1.f, .label = "Outline Intensity", .section = "Character Outline",
      .tooltip = "Scales the character outline blend weight before saturate. 0 = no outline, 1 = vanilla strength.",
      .min = 0.f, .max = 1.f, .format = "%.2f",
      .is_visible = []() { return IsSora1st() || IsSora2nd(); },
    },
    // -- Custom TAA (Sora 1st / 2nd, from-scratch replacement) --
    new renodx::utils::settings::Setting{
      .key = "CustomTAAEnabled", .binding = &shader_injection.custom_taa_enabled,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 1.f, .label = "Enable Custom TAA", .section = "Custom TAA",
      .tooltip = "Smooths jagged edges and flickering while moving. On = cleaner custom anti-aliasing. Off = the game's original anti-aliasing.",
      .labels = {"Off", "On"},
      .is_visible = []() { return IsSora1st() || IsSora2nd(); },
    },
    new renodx::utils::settings::Setting{
      .key = "CustomTAAHistoryFilter", .binding = &shader_injection.custom_taa_history_filter,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 2.f, .label = "History Filter", .section = "Custom TAA",
      .tooltip = "History reconstruction: Bilinear (TAA-0 baseline), Adaptive high-order (edge-gated bicubic), Full bicubic.",
      .labels = {"Bilinear", "Adaptive", "Full"},
      .is_enabled = []() { return shader_injection.custom_taa_enabled > 0.5f; },
      .is_visible = []() { return (IsSora1st() || IsSora2nd()) && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "CustomTAAClipMode", .binding = &shader_injection.custom_taa_clip_mode,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 2.f, .label = "History Validation", .section = "Custom TAA",
      .tooltip = "History validation: None (TAA-0/1), RGB AABB baseline (TAA-2), k-DOP polytope clip (TAA-3).",
      .labels = {"None", "AABB", "k-DOP"},
      .is_enabled = []() { return shader_injection.custom_taa_enabled > 0.5f; },
      .is_visible = []() { return (IsSora1st() || IsSora2nd()) && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "CustomTAAStaticFeedback", .binding = &shader_injection.custom_taa_static_feedback,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.90f, .label = "Static Feedback", .section = "Custom TAA",
      .tooltip = "TAA-5 history weight for static pixels: result = lerp(current, history, feedback). High values accumulate many frames on stable pixels.",
      .min = 0.f, .max = 0.99f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.custom_taa_enabled > 0.5f; },
      .is_visible = []() { return (IsSora1st() || IsSora2nd()) && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "CustomTAADynamicFeedback", .binding = &shader_injection.custom_taa_dynamic_feedback,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.5f, .label = "Dynamic Feedback", .section = "Custom TAA",
      .tooltip = "TAA-5 history weight for fast-moving pixels. Lower values cut trailing/temporal softness during motion.",
      .min = 0.f, .max = 0.99f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.custom_taa_enabled > 0.5f; },
      .is_visible = []() { return (IsSora1st() || IsSora2nd()) && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "CustomTAAMotionScale", .binding = &shader_injection.custom_taa_motion_scale,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.66f, .label = "Motion Scale", .section = "Custom TAA",
      .tooltip = "TAA-5 pixels-to-weight scale: weight = saturate(motionPixels * scale). Motion is in pixels/frame and includes the subpixel jitter delta, so the default is small: ~1px stays near Static, ~4px blends halfway, ~8px reaches Dynamic. Do not change during the first test pass.",
      .min = 0.f, .max = 2.f, .format = "%.3f",
      .is_enabled = []() { return shader_injection.custom_taa_enabled > 0.5f; },
      .is_visible = []() { return (IsSora1st() || IsSora2nd()) && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "CustomTAASquaredMotionResponse", .binding = &shader_injection.custom_taa_squared_motion_response,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 1.f, .label = "Squared Motion Response", .section = "Custom TAA",
      .tooltip = "Motion-weight curve A/B: Off = linear (baseline), On = squared (same static/dynamic endpoints, more history at low/moderate motion). Applies to feedback and silhouette rejection together.",
      .labels = {"Off", "On"},
      .is_enabled = []() { return shader_injection.custom_taa_enabled > 0.5f; },
      .is_visible = []() { return (IsSora1st() || IsSora2nd()) && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "CustomTAADetailRestore", .binding = &shader_injection.custom_taa_detail_restore,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.25f, .label = "Detail Restore", .section = "Custom TAA",
      .tooltip = "Feedback restoration experiment: restores history feedback toward Static on high-relative-detail pixels whose history agrees with current (flat or disagreeing pixels unchanged). 0 = prior behavior exactly; test value 8.0. Requires validation on.",
      .min = 0.f, .max = 32.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.custom_taa_enabled > 0.5f && shader_injection.custom_taa_clip_mode > 0.5f; },
      .is_visible = []() { return (IsSora1st() || IsSora2nd()) && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "CustomTAADetailTarget", .binding = &shader_injection.custom_taa_detail_target,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.99f, .label = "Detail Target", .section = "Custom TAA",
      .tooltip = "Selective above-static accumulation experiment: high-detail, low-motion, history-agreeing pixels (and silhouette-valid) approach this feedback (~20-frame window at 0.95) without giving that window to the whole image. Equal to Static = off (exact prior behavior); test 0.95 against Static 0.85. Requires validation on.",
      .min = 0.85f, .max = 0.99f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.custom_taa_enabled > 0.5f && shader_injection.custom_taa_clip_mode > 0.5f; },
      .is_visible = []() { return (IsSora1st() || IsSora2nd()) && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "CustomTAAOvershootSoftness", .binding = &shader_injection.custom_taa_overshoot_softness,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 1.0f, .label = "Overshoot Softness", .section = "Custom TAA",
      .tooltip = "Selective-tolerance experiment (keep epsilon at 0.01 for A/B): history near the k-DOP hull keeps feedback and averages out flicker; history far outside loses feedback and stays responsive. 100 ~= prior behavior (not bit-exact), 1.0 = experiment default, 0.5 = stronger rejection.",
      .min = 0.25f, .max = 100.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.custom_taa_enabled > 0.5f && shader_injection.custom_taa_clip_mode > 0.5f; },
      .is_visible = []() { return (IsSora1st() || IsSora2nd()) && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "CustomTAASilhouetteRejection", .binding = &shader_injection.custom_taa_silhouette_rejection,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 1.f, .label = "Silhouette Rejection", .section = "Custom TAA",
      .tooltip = "Wrong-surface history experiment: attenuates history feedback where depth discontinuity meets motion (disocc = 1 - motionWeight * saturate(spread * k)). Static detail keeps history regardless of depth edges. 0 = prior behavior exactly.",
      .min = 0.f, .max = 16.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.custom_taa_enabled > 0.5f; },
      .is_visible = []() { return (IsSora1st() || IsSora2nd()) && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "CustomTAAKDOPAxes", .binding = &shader_injection.custom_taa_kdop_axes,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 1.f, .label = "k-DOP Axis Count", .section = "Custom TAA",
      .tooltip = "k-DOP polytope density: 16-DOP General subset (default, first 8 axes of the paper's 32-DOP set), 22-DOP Paper lightweight set, 32-DOP Paper full set. The paper's scene-optimized 16-DOP sets are future experiments.",
      .labels = {"16-DOP General", "22-DOP Paper", "32-DOP Paper"},
      .is_enabled = []() { return shader_injection.custom_taa_enabled > 0.5f && shader_injection.custom_taa_clip_mode > 1.5f; },
      .is_visible = []() { return (IsSora1st() || IsSora2nd()) && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "CustomTAADmin", .binding = &shader_injection.custom_taa_dmin,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.048f, .label = "Adaptive Threshold", .section = "Custom TAA",
      .tooltip = "Edge D-term gate for adaptive history filtering (paper start point, tune per game). Below this: bilinear; above: full bicubic correction.",
      .min = 0.f, .max = 0.5f, .format = "%.3f",
      .is_enabled = []() { return shader_injection.custom_taa_enabled > 0.5f && shader_injection.custom_taa_history_filter > 0.5f && shader_injection.custom_taa_history_filter < 1.5f; },
      .is_visible = []() { return (IsSora1st() || IsSora2nd()) && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "CustomTAAKDOPEpsilon", .binding = &shader_injection.custom_taa_kdop_epsilon,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.05f, .label = "k-DOP Epsilon", .section = "Custom TAA",
      .tooltip = "Slab extent padding for k-DOP clipping (paper default 1e-5). Investigation range: higher values retain more history (reduces shadow jitter but risks stale-history drag on highlights).",
      .min = 0.f, .max = 0.1f, .format = "%.5f",
      .is_enabled = []() { return shader_injection.custom_taa_enabled > 0.5f && shader_injection.custom_taa_clip_mode > 1.5f; },
      .is_visible = []() { return (IsSora1st() || IsSora2nd()) && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "CustomTAADebug", .binding = &shader_injection.custom_taa_debug,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 0.f, .label = "Debug View", .section = "Custom TAA",
      .tooltip = "Dev only: Normal, History only (is reconstruction working?), Clip factor (is k-DOP rejecting?), Adapt-Bilin x20 (where does adaptive differ from bilinear?), Full-Bilin x20 (where does full bicubic differ?).",
      .labels = {"Normal", "History", "Clip", "Adapt-Bilin", "Full-Bilin"},
      .is_enabled = []() { return shader_injection.custom_taa_enabled > 0.5f; },
      .is_visible = []() { return (IsSora1st() || IsSora2nd()) && IsAdvancedSettingsMode(); },
    },
    // -- RCAS post-TAA sharpening (dedicated pass; never feeds TAA history) --
    new renodx::utils::settings::Setting{
      .key = "RCASEnabled", .binding = &shader_injection.rcas_enabled,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 1.f, .label = "Enable RCAS", .section = "Custom TAA",
      .tooltip = "Turns image sharpening on or off.",
      .labels = {"Off", "On"},
      .is_enabled = []() { return shader_injection.custom_taa_enabled > 0.5f; },
      .is_visible = []() { return IsSora1st() || IsSora2nd(); },
    },
    new renodx::utils::settings::Setting{
      .key = "RCASStrength", .binding = &shader_injection.rcas_strength,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.3f, .label = "Sharpening", .section = "Custom TAA",
      .tooltip = "How crisp the image looks. Higher values give a sharper picture but can cause glowing edges if set too high. 0 turns sharpening off.",
      .min = 0.f, .max = 1.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.custom_taa_enabled > 0.5f && shader_injection.rcas_enabled > 0.5f; },
      .is_visible = []() { return IsSora1st() || IsSora2nd(); },
    },
    new renodx::utils::settings::Setting{
      .key = "RCASMotionOn", .binding = &shader_injection.rcas_motion_on,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 1.f, .label = "Motion Sharpening", .section = "Custom TAA",
      .tooltip = "Automatically strengthens sharpening while the camera or objects move, then relaxes it back when the image is still. Off = always use the Sharpening value above.",
      .labels = {"Off", "On"},
      .is_enabled = []() { return shader_injection.custom_taa_enabled > 0.5f && shader_injection.rcas_enabled > 0.5f; },
      .is_visible = []() { return (IsSora1st() || IsSora2nd()) && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "RCASMotionMultiplier", .binding = &shader_injection.rcas_motion_multiplier,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 4.0f, .label = "Motion Sharpening Multiplier", .section = "Custom TAA",
      .tooltip = "Scales the Sharpening value during fast movement: motion value = base x multiplier, clamped to Max Motion Sharpening below. Example: Sharpening 0.30 with this at 2.0 means a still image uses 0.30 and fast motion uses 0.60.",
      .min = 1.f, .max = 10.f, .format = "%.1f",
      .is_enabled = []() { return shader_injection.custom_taa_enabled > 0.5f && shader_injection.rcas_enabled > 0.5f && shader_injection.rcas_motion_on > 0.5f; },
      .is_visible = []() { return (IsSora1st() || IsSora2nd()) && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "RCASMotionMax", .binding = &shader_injection.rcas_motion_max,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 1.0f, .label = "Max Motion Sharpening", .section = "Custom TAA",
      .tooltip = "Clamps the motion sharpening value from above. May exceed 1 for stronger motion sharpening (watch for ringing/halos at high values). Example: Sharpening 0.30 with 10x multiplier reaches 3.00 but is clamped to this value.",
      .min = 0.f, .max = 3.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.custom_taa_enabled > 0.5f && shader_injection.rcas_enabled > 0.5f && shader_injection.rcas_motion_on > 0.5f; },
      .is_visible = []() { return (IsSora1st() || IsSora2nd()) && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "RCASMotionThreshold", .binding = &shader_injection.rcas_motion_threshold,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.01f, .label = "Motion Threshold", .section = "Custom TAA",
      .tooltip = "How much movement is ignored before motion sharpening kicks in. Filters out tiny per-frame jitter so a still camera keeps normal sharpening instead of flickering stronger.",
      .min = 0.f, .max = 3.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.custom_taa_enabled > 0.5f && shader_injection.rcas_enabled > 0.5f && shader_injection.rcas_motion_on > 0.5f; },
      .is_visible = []() { return (IsSora1st() || IsSora2nd()) && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "RCASMotionRange", .binding = &shader_injection.rcas_motion_range,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.25f, .label = "Motion Range", .section = "Custom TAA",
      .tooltip = "How much more movement it takes to ramp from normal sharpening up to the full motion value. Lower = reaches full strength sooner.",
      .min = 0.25f, .max = 8.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.custom_taa_enabled > 0.5f && shader_injection.rcas_enabled > 0.5f && shader_injection.rcas_motion_on > 0.5f; },
      .is_visible = []() { return (IsSora1st() || IsSora2nd()) && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "RCASMotionResponse", .binding = &shader_injection.rcas_motion_response,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.5f, .label = "Motion Response", .section = "Custom TAA",
      .tooltip = "Shape of the ramp from normal to full motion sharpening. 1.0 = even ramp; higher values stay gentle longer, then climb steeply near full motion.",
      .min = 0.5f, .max = 3.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.custom_taa_enabled > 0.5f && shader_injection.rcas_enabled > 0.5f && shader_injection.rcas_motion_on > 0.5f; },
      .is_visible = []() { return (IsSora1st() || IsSora2nd()) && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "RCASDebug", .binding = &shader_injection.rcas_debug,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 0.f, .label = "RCAS Debug View", .section = "Custom TAA",
      .tooltip = "Diagnostic only: Motion Heat shows where motion sharpening engages (green = base strength, yellow = partial, red = full motion target). Toggle back to Normal for image-quality testing (heat frames pollute TAA history while active).",
      .labels = {"Normal", "Motion Heat"},
      .is_enabled = []() { return shader_injection.custom_taa_enabled > 0.5f && shader_injection.rcas_enabled > 0.5f; },
      .is_visible = []() { return (IsSora1st() || IsSora2nd()) && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "RCASDenoise", .binding = &shader_injection.rcas_denoise,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 1.f, .label = "RCAS Denoise", .section = "Custom TAA",
      .tooltip = "Reference FSR_RCAS_DENOISE behavior: scales the lobe by the noise term (0.5..1.0), reducing sharpening where local variation looks grain-like. Off = reference default (full sharpening). May also soften legitimate fine detail such as grass/foliage texture.",
      .labels = {"Off", "On"},
      .is_enabled = []() { return shader_injection.custom_taa_enabled > 0.5f && shader_injection.rcas_enabled > 0.5f; },
      .is_visible = []() { return (IsSora1st() || IsSora2nd()) && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "FXAAEnabled", .binding = &shader_injection.fxaa_enabled,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 1.f, .label = "Enable FXAA", .section = "Custom TAA",
      .tooltip = "Morphological anti-aliasing between TAA and sharpening. Smooths leftover jagged edges the temporal pass misses. Off = exact prior behavior (RCAS reads the TAA output directly). Never feeds TAA history.",
      .labels = {"Off", "On"},
      .is_enabled = []() { return shader_injection.custom_taa_enabled > 0.5f; },
      .is_visible = []() { return (IsSora1st() || IsSora2nd()) && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "FXAAQuality", .binding = &shader_injection.fxaa_quality,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 1.f, .label = "FXAA Quality", .section = "Custom TAA",
      .tooltip = "Reference FXAA 3.11 quality preset. Standard (12) is the reference default; High (29) searches longer edges at a small extra cost.",
      .labels = {"Standard", "High"},
      .is_enabled = []() { return shader_injection.custom_taa_enabled > 0.5f && shader_injection.fxaa_enabled > 0.5f; },
      .is_visible = []() { return (IsSora1st() || IsSora2nd()) && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "FXAASubpix", .binding = &shader_injection.fxaa_subpix,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.33f, .label = "FXAA Subpixel", .section = "Custom TAA",
      .tooltip = "Amount of sub-pixel aliasing removal. Higher softens more (1.00 = softest), lower stays sharper (0.00 = off). Reference default 0.75.",
      .min = 0.f, .max = 1.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.custom_taa_enabled > 0.5f && shader_injection.fxaa_enabled > 0.5f; },
      .is_visible = []() { return (IsSora1st() || IsSora2nd()) && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "FXAAEdgeThreshold", .binding = &shader_injection.fxaa_edge_threshold,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.166f, .label = "FXAA Edge Threshold", .section = "Custom TAA",
      .tooltip = "Minimum local contrast required before FXAA engages. Lower catches more edges but costs more (reference: 0.333 faster, 0.250 low, 0.166 default, 0.125 high, 0.063 overkill).",
      .min = 0.031f, .max = 0.333f, .format = "%.3f",
      .is_enabled = []() { return shader_injection.custom_taa_enabled > 0.5f && shader_injection.fxaa_enabled > 0.5f; },
      .is_visible = []() { return (IsSora1st() || IsSora2nd()) && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "FXAAEdgeThresholdMin", .binding = &shader_injection.fxaa_edge_threshold_min,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.0625f, .label = "FXAA Edge Threshold Min", .section = "Custom TAA",
      .tooltip = "Trims FXAA from processing darks. Lower processes more dark detail but costs more (reference: 0.0833 default, 0.0625 high quality, 0.0312 visible limit). 0 = process darks fully.",
      .min = 0.f, .max = 0.0833f, .format = "%.4f",
      .is_enabled = []() { return shader_injection.custom_taa_enabled > 0.5f && shader_injection.fxaa_enabled > 0.5f; },
      .is_visible = []() { return (IsSora1st() || IsSora2nd()) && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "DynCubeSSRISFAST", .binding = &shader_injection.dynCube_ssr_isfast_enabled,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 1.f, .label = "SSR IS-FAST Phase", .section = "Dynamic Cubemaps",
      .tooltip = "Use IS-FAST spatio-temporal blue noise for the SSR ray-march start phase (breaks up stride banding). Off = deterministic hash phase. Requires the master IS-FAST toggle.",
      .labels = {"Off", "On"},
      .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f && shader_injection.dynCube_ssr_enabled > 0.5f && g_isfast_enabled > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "DynCubeSSRISFASTStrength", .binding = &shader_injection.dynCube_ssr_isfast_strength,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 1.f, .label = "SSR IS-FAST Strength", .section = "Dynamic Cubemaps",
      .tooltip = "Blend between the deterministic hash phase (0) and full IS-FAST noise phase (1).",
      .min = 0.f, .max = 1.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f && shader_injection.dynCube_ssr_enabled > 0.5f && g_isfast_enabled > 0.5f && shader_injection.dynCube_ssr_isfast_enabled > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "DynCubeSSRISFASTSpatial", .binding = &shader_injection.dynCube_ssr_isfast_spatial,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 1.f, .label = "SSR IS-FAST Spatial Scale", .section = "Dynamic Cubemaps",
      .tooltip = "Scale IS-FAST noise spatial frequency for SSR. <1 zooms in (smoother), >1 adds more detail.",
      .min = 0.25f, .max = 4.0f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f && shader_injection.dynCube_ssr_enabled > 0.5f && g_isfast_enabled > 0.5f && shader_injection.dynCube_ssr_isfast_enabled > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "DynCubeSSRISFASTTemporal", .binding = &shader_injection.dynCube_ssr_isfast_temporal,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 1.f, .label = "SSR IS-FAST Temporal Speed", .section = "Dynamic Cubemaps",
      .tooltip = "Animate the IS-FAST noise slice for SSR. 0 = frozen (stable, no shimmer), higher = faster animation.",
      .min = 0.f, .max = 5.0f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f && shader_injection.dynCube_ssr_enabled > 0.5f && g_isfast_enabled > 0.5f && shader_injection.dynCube_ssr_isfast_enabled > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "DynCubeVanillaBlur", .binding = &shader_injection.dynCube_vanilla_blur,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 2.0f, .label = "Vanilla Cubemap Blur", .section = "Dynamic Cubemaps",
      .tooltip = "Mip-offset blur on the vanilla cubemap fallback. 0 = original/sharp, higher = progressively blurrier.",
      .min = 0.f, .max = 8.f, .format = "%.1f",
      .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .value_type = renodx::utils::settings::SettingValueType::BUTTON,
      .label = "Reset All Settings to Defaults",
      .section = "Settings",
      .on_click = []() {
        for (auto* s : settings) {
          if (s->binding != nullptr && s->can_reset) {
            s->value = s->default_value;
            s->value_as_int = static_cast<int>(s->default_value);
            s->Write();
          }
        }
        return true;
      },
    },
    new renodx::utils::settings::Setting{
      .value_type = renodx::utils::settings::SettingValueType::BUTTON,
      .label = "Patreon",
      .section = "Info",
      .on_click = []() {
        renodx::utils::platform::LaunchURL("https://www.patreon.com/c/Toru77");
        return false;
      },
    },
    new renodx::utils::settings::Setting{
      .value_type = renodx::utils::settings::SettingValueType::TEXT,
      .label = "Addon made by Toru.",
      .section = "Info",
    },
    new renodx::utils::settings::Setting{
        .value_type = renodx::utils::settings::SettingValueType::TEXT,
        .label = "Thanks to Shortfuse for RenoDX.",
        .section = "Info",
    },
    new renodx::utils::settings::Setting{
        .value_type = renodx::utils::settings::SettingValueType::TEXT,
        .label = "Thanks to Forge for rendering techniques and inspiring me.",
        .section = "Info",
    },
    new renodx::utils::settings::Setting{
        .value_type = renodx::utils::settings::SettingValueType::TEXT,
        .label = "IS-FAST Noise: Dont enable if you are not using TAA/FSR/DLSS/XeSS.",
        .section = "Info",
    },
    new renodx::utils::settings::Setting{
        .value_type = renodx::utils::settings::SettingValueType::TEXT,
        .label = "Ultra Shadows are recommended for CHSS. High is minimum.",
        .section = "Info",
        .is_visible = []() { return !IsKai(); },
    },
    new renodx::utils::settings::Setting{
        .value_type = renodx::utils::settings::SettingValueType::TEXT,
        .label = "Disable SSAO from in game settings for small performance boost if you are using GTVBAO.",
        .section = "Info",
        .is_visible = []() { return !IsKai(); },
    },
    new renodx::utils::settings::Setting{
        .value_type = renodx::utils::settings::SettingValueType::TEXT,
        .label = "Enable Soft or PCSS Shadow Filtering in-game.",
        .section = "Info",
        .is_visible = []() { return IsKai(); },
    },
    new renodx::utils::settings::Setting{
        .value_type = renodx::utils::settings::SettingValueType::TEXT,
        .label = "If you are going to be using GTVBAO, make sure in-game setting for local shadowing is set to character only.",
        .section = "Info",
        .is_visible = []() { return IsKai(); },
    },
    // -- VBGI debug views removed � use GTVBAO debug View for GI inspection. --

};

// ----------- GTVBAO Backend � implementation -----------

static void OnInitDevice(reshade::api::device* device) {
  reshade::log::message(reshade::log::level::info, "[sora-vanillaplus] Device init � addon loaded.");
  auto* d = renodx::utils::data::Create<DeviceData>(device);

  // 1�1 white fallback texture so t22 is never bound to null.
  uint32_t white = 0xFFFFFFFF;
  reshade::api::subresource_data initial = {&white, 4u, 4u};
  reshade::api::resource_desc rd = {};
  rd.type = reshade::api::resource_type::texture_2d;
  rd.texture = {1u, 1u, 1u, 1u, reshade::api::format::r8g8b8a8_unorm, 1u};
  rd.heap = reshade::api::memory_heap::gpu_only;
  rd.usage = reshade::api::resource_usage::shader_resource;
  device->create_resource(rd, &initial, reshade::api::resource_usage::shader_resource,
                          &d->fallback_texture);
  device->create_resource_view(d->fallback_texture, reshade::api::resource_usage::shader_resource,
                                reshade::api::resource_view_desc(
                                    reshade::api::resource_view_type::texture_2d,
                                    reshade::api::format::r8g8b8a8_unorm, 0, 1, 0, 1),
                                &d->fallback_srv);

  reshade::log::message(reshade::log::level::info, "[GTVBAO] Device init � fallback SRV created.");
}

static void OnDestroyDevice(reshade::api::device* device) {
  auto* d = device->get_private_data<DeviceData>();
  if (d) {
    DestroyGTVBAOResources(device, d);
    DestroyDynCubeResources(device, d);
    DestroyRCASResources(device, d);
    DestroyFXAAResources(device, d);
    DestroyMotionBlurResources(device, d);
    DestroyShadowsResources(device, d);
    for (auto& [handle, clone] : d->taa_subobject_clones) {
      renodx::utils::pipeline::DestroyPipelineSubobjects(clone.first, clone.second);
    }
    d->taa_subobject_clones.clear();
    d->taa_subobjects_owned.clear();
    d->taa_pending_resets.clear();
    if (d->fallback_srv.handle) device->destroy_resource_view(d->fallback_srv);
    if (d->fallback_texture.handle) device->destroy_resource(d->fallback_texture);
    device->destroy_private_data<DeviceData>();
  }
}

static void OnInitSwapchain(reshade::api::swapchain* sc, bool resize) {
  auto* d = sc->get_device()->get_private_data<DeviceData>();
  if (!d) return;
  if (resize) {
    d->resize_guard_until_frame = d->frame_index + kGTVBAOResizeGuardFrames;
    CSLog("swapchain", "init resize: 4-frame guard armed, depth/ssao/cbv cleared, GTVBAO+DynCube destroyed");
    d->captured_depth_srv = {}; d->captured_ssao_srv = {};
    d->captured_ssao_frame = 0u; d->deferred_ssao_srv = {};
    d->captured_depth_w = 0u; d->captured_depth_h = 0u;
    d->captured_depth_live = true; d->captured_mrt_live = true;
    d->captured_color_live = true; d->captured_cbv_live = true;
    d->captured_scene_cbv_view = {};
    d->captured_scene_cbv = {}; d->captured_scene_cbv_valid = false;
    d->captured_scene_cbv_frame = UINT64_MAX;
    DestroyGTVBAOResources(sc->get_device(), d);
    DestroyDynCubeResources(sc->get_device(), d);
    DestroyShadowsResources(sc->get_device(), d);
  }
}

static void OnDestroySwapchain(reshade::api::swapchain* sc, bool resize) {
  auto* d = sc->get_device()->get_private_data<DeviceData>();
  if (!d) return;
  if (resize) {
    CSLog("swapchain", "destroy resize: depth/ssao/cbv cleared, DynCube destroyed");
    d->captured_depth_srv = {}; d->captured_ssao_srv = {};
    d->captured_ssao_frame = 0u; d->deferred_ssao_srv = {};
    d->captured_depth_w = 0u; d->captured_depth_h = 0u;
    d->captured_depth_live = true; d->captured_mrt_live = true;
    d->captured_color_live = true; d->captured_cbv_live = true;
    d->captured_scene_cbv_view = {};
    d->captured_scene_cbv = {}; d->captured_scene_cbv_valid = false;
    d->captured_scene_cbv_frame = UINT64_MAX;
    d->resources_created = false;
    DestroyDynCubeResources(sc->get_device(), d);
    DestroyShadowsResources(sc->get_device(), d);
    return;
  }
  DestroyGTVBAOResources(sc->get_device(), d);
  DestroyDynCubeResources(sc->get_device(), d);
  DestroyShadowsResources(sc->get_device(), d);
}

// -- Descriptor table helpers --

static bool EnsureGTVBAODescriptorTables(
    reshade::api::device* device,
    reshade::api::pipeline_layout layout,
    GTVBAODescriptorTableSet* tables) {
  if (!device || !tables || !layout.handle) return false;
  for (uint32_t i = 0; i < kGtvbaoDescriptorTableParamCount; ++i) {
    if ((*tables)[i].handle != 0u) continue;
    if (!device->allocate_descriptor_table(layout, i, &(*tables)[i]))
      return false;
  }
  return true;
}

static void DestroyGTVBAODescriptorTables(
    reshade::api::device* device, GTVBAODescriptorTableSet* tables) {
  if (!device || !tables) return;
  for (auto& t : *tables) {
    if (t.handle) { device->free_descriptor_table(t); t = {}; }
  }
}

// The half-res GI chain (raw -> denoised -> full upscale) is usable only when
// the half denoised texture exists AND both of its views were created. The
// texture, SRV and UAV come from separate create calls, so they are not
// guaranteed to succeed together; if only some exist, a consumer that picks
// its target from a different subset silently reads or writes a different
// buffer than its neighbour. Every GI consumer must therefore derive its
// target from this single predicate.
static bool GTVBAO_GiHalfChainReady(const DeviceData* d, bool half_mode) {
  return half_mode
      && d->vbgi_denoised_half_texture.handle
      && d->vbgi_denoised_half_srv.handle
      && d->vbgi_denoised_half_uav.handle;
}

// -- Vanilla env-cube identification (strict) --
// A 512/1024 square R8G8B8A8_UNORM cube view is the game's vanilla env cubemap.
// Shared by the vanilla capture and the global t17 swap so both agree exactly.
// Doubles as self-exclusion: our pushed cubes are RGBA16F and can never match,
// regardless of push-event re-entrancy. sRGB never matches by design.
static bool IsVanillaEnvCubeView(reshade::api::device* device, reshade::api::resource_view view) {
  if (!device || view.handle == 0u) return false;
  auto vdesc = device->get_resource_view_desc(view);
  if (vdesc.type != reshade::api::resource_view_type::texture_cube
      || vdesc.format != reshade::api::format::r8g8b8a8_unorm) return false;
  auto res = device->get_resource_from_view(view);
  if (res.handle == 0u) return false;
  auto rdesc = device->get_resource_desc(res);
  return rdesc.type == reshade::api::resource_type::texture_2d
      && (rdesc.texture.width == 1024u || rdesc.texture.width == 512u)
      && rdesc.texture.width == rdesc.texture.height;
}

// -- Scene CBV helper --

static bool IsSceneCbvCandidateValid(reshade::api::device* device,
                                      const reshade::api::buffer_range& range) {
  if (!device || range.buffer.handle == 0u) return false;
  auto desc = device->get_resource_desc(range.buffer);
  if (desc.type != reshade::api::resource_type::buffer) return false;
  return desc.buffer.size >= kSceneCbMinimumBytes
      && desc.buffer.size <= (64u * 1024u)
      && range.offset + range.size <= desc.buffer.size;
}

// -- Push-descriptors event ? capture lighting inputs (kai pattern) --

static void OnPushDescriptorsCapture(
    reshade::api::command_list* cmd_list,
    reshade::api::shader_stage stages,
    reshade::api::pipeline_layout layout,
    uint32_t param_index,
    const reshade::api::descriptor_table_update& update) {
  if (!cmd_list) return;
  // Fast path: no feature consumes the captured snapshots -> skip ALL capture work.
  // (Fired on every push_descriptors in the frame; without this gate the body below
  //  runs thousands of times per frame even when every addon feature is disabled.)
  // RCAS motion capture needs this path too whenever Custom TAA runs on Sora
  // (TAA-only testing runs with GTVBAO/DynCube off, which must not skip it).
  const bool wantTAAMotionPush =
      shader_injection.custom_taa_enabled > 0.5f && (IsSora1st() || IsSora2nd());
  // Motion blur's own capture work has to be in this gate too. The tonemap t0
  // capture below is only reachable when this check passes, and motion blur was
  // missing from the list for so long that it only ever worked by riding on
  // GTVBAO, DynCube or Custom TAA happening to be enabled alongside it. Kai has
  // no Custom TAA, so all three were off at once, the body returned early, and
  // the deploy reported a t0 that was never captured. GTVBAO being on made motion
  // blur work on Kai, which is what identified this gate as the cause.
  //
  // Gate on the FEATURE, not on the per-frame cutscene state: MotionBlurActive()
  // still makes the real per-frame decision, and a capture that flickers off when
  // the player walks out of a cutscene would drop the view the deploy still needs.
  const bool wantMotionBlurPush = shader_injection.mb_mode > 0.5f;
  if (shader_injection.gtvbao_mode < 0.5f
      && shader_injection.dynCube_enabled < 0.5f
      && !wantTAAMotionPush
      && !wantMotionBlurPush) {
    return;
  }
  auto* device = cmd_list->get_device();
  auto* d = device->get_private_data<DeviceData>();
  if (!d) return;

  // -- Capture depth/SSAO/CBV � unconditional (register-based, kai-style). --
  if (update.type == reshade::api::descriptor_type::texture_shader_resource_view) {
    auto* views = static_cast<const reshade::api::resource_view*>(update.descriptors);
  // Capture depth: t4 (Sora) or t3 (Kai) � ONLY from lighting shader, game-specific binding.
    uint32_t depthBinding = IsKai() ? kLightingDepthRegisterKai : kLightingDepthRegister;
    if (update.binding == depthBinding && update.count >= 1
        && views[0].handle != 0u) {
      auto* ss = renodx::utils::shader::GetCurrentState(cmd_list);
      if (ss) {
        uint32_t hash = renodx::utils::shader::GetCurrentPixelShaderHash(ss);
        if (IsLightingShader(hash)) {
          if (d->captured_depth_srv.handle != views[0].handle) {
            auto info = CSResolveCapture(device, views[0]);
            d->captured_depth_res = info.res;
            d->captured_depth_dims = info.dims;
            // Cached alongside the log string so a consumer that has to size its
            // own output to the depth grid does not need a fresh GPU query: the
            // shadow passes run every frame and a query per frame is not free.
            d->captured_depth_w = info.w;
            d->captured_depth_h = info.h;
            CSLog("capture", std::string("depth handle -> ") + info.dims);
          }
          d->captured_depth_srv = views[0];
          d->captured_depth_live = true;
          d->captured_depth_frame = d->frame_index;
          d->captured_scene_cbv_frame = d->frame_index;
          if (shader_injection.gtvbao_debug_logging > 0.5f) {
            auto depth_res = device->get_resource_from_view(views[0]);
            if (depth_res.handle != 0u) {
              auto dd = device->get_resource_desc(depth_res);
              reshade::log::message(reshade::log::level::info,
                (std::string("[GTVBAO] Depth captured from lighting: ") +
                 std::to_string(dd.texture.width) + "x" +
                 std::to_string(dd.texture.height)).c_str());
            }
          }
        }
      }
    }
  // Capture SSAO: t5 (Sora) or t4 (Kai) — game-specific binding. Hash-gated for
  // the same reason depth and the MRT normal are: the binding number alone is
  // not unique, so any other shader pushing an SRV at that slot would otherwise
  // silently replace the AO the micro-shadow pass reads.
    uint32_t ssaoBinding = IsKai() ? kLightingSsaoRegisterKai : kLightingSsaoRegister;
    if (update.binding == ssaoBinding && update.count >= 1
        && views[0].handle != 0u) {
      auto* ss = renodx::utils::shader::GetCurrentState(cmd_list);
      if (ss) {
        uint32_t hash = renodx::utils::shader::GetCurrentPixelShaderHash(ss);
        if (IsLightingShader(hash)) {
          d->captured_ssao_srv = views[0];
          d->captured_ssao_frame = d->frame_index;
        }
      }
    }
    if ((update.binding == kLightingMrtNormalRegister) && update.count >= 1
        && views[0].handle != 0u) {
      auto* ss = renodx::utils::shader::GetCurrentState(cmd_list);
      if (ss) {
        uint32_t hash = renodx::utils::shader::GetCurrentPixelShaderHash(ss);
        if (IsLightingShader(hash)) {
          if (d->captured_mrt_normal_srv.handle != views[0].handle) {
            auto info = CSResolveCapture(device, views[0]);
            d->captured_mrt_res = info.res;
            d->captured_mrt_dims = info.dims;
            CSLog("capture", std::string("mrt handle -> ") + info.dims);
          }
          d->captured_mrt_normal_srv = views[0];
          d->captured_mrt_live = true;
        }
      }
    }
    // Capture motion buffer t3 � ONLY from TAA draws (both Sora hashes), for
    // RCAS Stage 2 motion-adaptive sharpening. Same over-capture rationale as
    // the lighting-gated captures above: t3 means different things per shader.
    if (update.binding == 3u && update.count >= 1
        && views[0].handle != 0u) {
      auto* ss = renodx::utils::shader::GetCurrentState(cmd_list);
      if (ss) {
        uint32_t hash = renodx::utils::shader::GetCurrentPixelShaderHash(ss);
        if (hash == 0xFA37EA04u || hash == 0x9D91FAC3u) {
          if (d->rcas_motion_srv.handle != views[0].handle) {
            auto info = CSResolveCapture(device, views[0]);
            d->rcas_motion_res = info.res;
            reshade::log::message(reshade::log::level::info,
              (std::string("[RCAS] motion ") + info.dims).c_str());
          }
          d->rcas_motion_srv = views[0];
          d->rcas_motion_live = true;
        }
      }
    }
    // Capture the tonemap's t0 for the motion blur deploy. The blit samples t0 and
    // writes the output-resolution target, so we need t0's own view for the
    // gather's colour input AND its exact format: the gather output is pushed
    // back at t0 and must stay format compatible with what the game's shader
    // expects. Over-capture of binding 0 is prevented by the hash gate.
    if (update.binding == 0u && update.count >= 1
        && views[0].handle != 0u) {
      auto* ss = renodx::utils::shader::GetCurrentState(cmd_list);
      if (ss) {
        uint32_t hash = renodx::utils::shader::GetCurrentPixelShaderHash(ss);
        if (IsMotionBlurDeployHash(hash)) {
          d->mb_tonemap_src_srv = views[0];
          d->mb_tonemap_src_res = device->get_resource_from_view(views[0]).handle;
          d->mb_tonemap_src_live = true;
        }
      }
    }
    // Capture t0 color texture � ONLY from the lighting shader (hash 0xFDAAF80E).
    // Unconditional capture would grab binding 0 from any shader, causing wrong colors.
    if (update.binding == 0u && update.count >= 1
        && views[0].handle != 0u) {
      auto* ss = renodx::utils::shader::GetCurrentState(cmd_list);
      if (ss) {
        uint32_t hash = renodx::utils::shader::GetCurrentPixelShaderHash(ss);
        if (IsLightingShader(hash)) {
          if (d->captured_color_srv.handle != views[0].handle) {
            auto info = CSResolveCapture(device, views[0]);
            d->captured_color_res = info.res;
            d->captured_color_dims = info.dims;
            d->captured_color_w = info.w;
            d->captured_color_h = info.h;
            CSLog("capture", std::string("color handle -> ") + info.dims);
          }
          d->captured_color_srv = views[0];
          d->captured_color_live = true;
          d->captured_color_frame = d->frame_index;
        }
      }
    }
    // Capture ssr2-draw t0 (vanilla ssr1 march result) for the SSR replacement
    // debug view. ONLY from the ssr2 pixel shader � same over-capture rationale
    // as the color capture above.
    if (update.binding == 0u && update.count >= 1
        && views[0].handle != 0u) {
      auto* ss = renodx::utils::shader::GetCurrentState(cmd_list);
      if (ss) {
        uint32_t hash = renodx::utils::shader::GetCurrentPixelShaderHash(ss);
        if (hash == 0x17F931DEu) {
          if (d->captured_ssr1_srv.handle != views[0].handle) {
            auto info = CSResolveCapture(device, views[0]);
            d->captured_ssr1_res = info.res;
            CSLog("capture", std::string("ssr1 handle -> ") + info.dims);
          }
          d->captured_ssr1_srv = views[0];
          d->captured_ssr1_live = true;
        }
      }
    }
    // Capture ssr1-draw t2 (the march's own mrt0) for the composite gate + normal
    // decode at t4. Same-resource guarantee: the march decodes normals/flags from
    // this exact view. ONLY from the ssr1 pixel shader.
    if (update.binding == 2u && update.count >= 1
        && views[0].handle != 0u) {
      auto* ss = renodx::utils::shader::GetCurrentState(cmd_list);
      if (ss) {
        uint32_t hash = renodx::utils::shader::GetCurrentPixelShaderHash(ss);
        if (hash == 0xE2F406C7u) {
          if (d->captured_ssr_mrt_srv.handle != views[0].handle) {
            auto info = CSResolveCapture(device, views[0]);
            d->captured_ssr_mrt_res = info.res;
            CSLog("capture", std::string("ssrMrt handle -> ") + info.dims);
          }
          d->captured_ssr_mrt_srv = views[0];
          d->captured_ssr_mrt_live = true;
        }
      }
    }
    // Capture the game's vanilla texEnvMap_g (t17) binding � the vanilla cubemap
    // fallback layer (t30). From ANY pixel-shader t17 bind (lighting, glass, ...),
    // refreshed on every bind so game reallocations/resizes can never leave a stale
    // handle and the pre-first-capture window collapses to ~zero. Strict criterion
    // via IsVanillaEnvCubeView (also self-excludes our own pushes). Only when
    // Dynamic Cubemaps is enabled (no work when off). Supported games only
    // (Kai, Sora 1st, Sora 2nd).
    if (update.binding == 17u && update.count >= 1
        && views[0].handle != 0u && shader_injection.dynCube_enabled > 0.5f
        && IsDynCubeT17Game()
        && (static_cast<uint32_t>(stages) & static_cast<uint32_t>(reshade::api::shader_stage::pixel))
        && IsVanillaEnvCubeView(device, views[0])) {
      d->captured_vanilla_env_srv = views[0];
    }
    // Global Dynamic Cubemap t17 override: any pixel-shader bind of a 512x512 or
    // 1024x1024 R8G8B8A8_UNORM cube view at slot 17 IS the game's vanilla env cubemap
    // (strict criterion via IsVanillaEnvCubeView, UNORM only � sRGB never matches).
    // Swap in the active dynamic cube so every consumer (lighting, glass, future
    // shaders) gets live reflections with no per-shader registration. Respects Force
    // Vanilla and debug 4 (vanilla A/B paths keep the game cube). Normal display
    // (dbg==0) additionally requires a captured vanilla cube, so pre-first-capture
    // draws show true game vanilla instead of black dynamic; debug modes override
    // regardless. Re-entrancy: our own cube fails the criterion and the handler
    // terminates; a static guard makes this airtight regardless.
    // The vanilla capture above requires no lighting hash, so our pushes here
    // can never poison the vanilla fallback (criterion excludes them anyway).
    // Supported games only (Kai, Sora 1st, Sora 2nd).
    if (update.binding == 17u && update.count >= 1
        && views[0].handle != 0u && shader_injection.dynCube_enabled > 0.5f
        && IsDynCubeT17Game()
        && shader_injection.dynCube_force_vanilla < 0.5f
        && (int)shader_injection.dynCube_debug != 4
        && (static_cast<uint32_t>(stages) & static_cast<uint32_t>(reshade::api::shader_stage::pixel))) {
      static bool s_dynCubeT17SwapGuard = false;
      int swapDbg = (int)shader_injection.dynCube_debug;
      if (!s_dynCubeT17SwapGuard && IsVanillaEnvCubeView(device, views[0])
          && (swapDbg != 0 || d->captured_vanilla_env_srv.handle != 0u)
          && d && d->dyncube_resources_created) {
        // Lighting draws always sample sharp (own sample-time blur/brightness);
        // every other global consumer gets the softened/dimmed variant when one is
        // active, sharp otherwise. Validity-gated on current settings so a stale
        // variant can never serve after the sliders return to neutral.
        bool swapIsLighting = false;
        {
          auto* swapState = renodx::utils::shader::GetCurrentState(cmd_list);
          if (swapState) {
            uint32_t swapHash = renodx::utils::shader::GetCurrentPixelShaderHash(swapState);
            swapIsLighting = IsLightingShader(swapHash);
            if (swapHash != 0u) {
              std::ostringstream hs;
              hs << std::hex << swapHash;
              CSLog("draw", std::string("t17 draw hash=0x") + hs.str());
            }
          }
        }
        float swapSoften = std::clamp(shader_injection.dynCube_capture_soften, 0.f, 1.f);
        float swapStrength = std::clamp(shader_injection.dynCube_global_strength, 0.f, 1.f);
        bool serveVariant = !swapIsLighting && d->dyncube_variant_valid
            && d->dyncube_variant_cube_srv.handle != 0u
            && (swapSoften > 1e-4f || swapStrength < 1.f - 1e-4f);
        reshade::api::resource_view t17srv;
        if (swapDbg == 3) {
          if (!RunDynCubeSolid(cmd_list, d)) t17srv = {};
          else t17srv = d->dyncube_solid_cube_srv;
        } else if (serveVariant) {
          t17srv = d->dyncube_variant_cube_srv;
        } else {
          // Active completed filtered cube; raw history cube before first filter.
          t17srv = d->dyncube_ggx_valid
              ? d->dyncube_ggx_out_cube_srv[d->dyncube_ggx_active]
              : d->dyncube_srv;
        }
        // Once the vanilla cube is known, only swap onto that exact resource
        // (extra safety against hijacking unrelated 512/1024 cubes). Before the
        // first capture, the strict criterion identifies it (and the dbg==0 gate
        // above already required the capture, so this is belt-and-braces there).
        bool swapAllowed = true;
        if (d->captured_vanilla_env_srv.handle != 0u) {
          auto swapResHere = device->get_resource_from_view(views[0]);
          auto knownRes = device->get_resource_from_view(d->captured_vanilla_env_srv);
          swapAllowed = (swapResHere.handle != 0u && knownRes.handle != 0u && knownRes.handle == swapResHere.handle);
        }
        if (t17srv.handle && swapAllowed && !DynCubeSceneLive(d))
          CSLog("serve", "t17 swap skipped (stale scene)");
        if (t17srv.handle && swapAllowed && DynCubeSceneLive(d)) {
          s_dynCubeT17SwapGuard = true;
          cmd_list->push_descriptors(reshade::api::shader_stage::pixel,
              reshade::api::pipeline_layout{0}, 0,
              reshade::api::descriptor_table_update{{}, kDynCubeRegister, 0, 1,
                  reshade::api::descriptor_type::texture_shader_resource_view, &t17srv});
          s_dynCubeT17SwapGuard = false;
        }
      }
    }
  }
  if (update.type == reshade::api::descriptor_type::constant_buffer) {
    if (update.binding == kLightingSceneCbRegister && update.count >= 1) {
      auto* cbv_views = static_cast<const reshade::api::resource_view*>(update.descriptors);
      if (cbv_views[0].handle != 0u) {
        reshade::api::resource buf = { cbv_views[0].handle };
        auto desc = device->get_resource_desc(buf);
        if (desc.type == reshade::api::resource_type::buffer
            && desc.buffer.size >= 200u
            && desc.buffer.size <= (64u * 1024u)) {
          if (d->captured_scene_cbv_view.handle != cbv_views[0].handle) {
            CSLog("capture", std::string("cbv handle -> buf:") + std::to_string(desc.buffer.size));
            d->captured_cbv_res = buf.handle;
            d->captured_cbv_dims = std::string("buf:") + std::to_string(desc.buffer.size);
          }
          d->captured_scene_cbv = { buf, 0, desc.buffer.size };
          d->captured_scene_cbv_valid = true;
          d->captured_cbv_live = true;
          d->captured_scene_cbv_frame = d->frame_index;
          d->captured_scene_cbv_view = cbv_views[0];
        }
      }
    }
  }
  // -- Per-draw gating (only when GTVBAO or SSGI is on). --
  if (shader_injection.gtvbao_mode < 0.5f) return;
  if (!(static_cast<uint32_t>(stages) & static_cast<uint32_t>(reshade::api::shader_stage::pixel))) return;
}

// -- Bind-descriptor-tables event ? capture lighting inputs --

static void OnBindDescriptorTables(
    reshade::api::command_list* cmd_list,
    reshade::api::shader_stage stages,
    reshade::api::pipeline_layout layout,
    uint32_t first, uint32_t count,
    const reshade::api::descriptor_table* tables) {
  // Fast path: GTVBAO is the only consumer of these captures.
  if (shader_injection.gtvbao_mode < 0.5f) return;
  if (!cmd_list || !tables || count == 0u) return;
  auto* device = cmd_list->get_device();
  auto* d = device->get_private_data<DeviceData>();
  if (!d) return;

  // Log first 5 bind_descriptor_tables calls unconditionally.
  static uint32_t s_bind_log_count = 0u;
  if (s_bind_log_count < 5u) {
    s_bind_log_count++;
    reshade::log::message(reshade::log::level::info,
      (std::string("[GTVBAO] bind_descriptor_tables: first=") +
      std::to_string(first) + ", count=" + std::to_string(count)).c_str());
  }

  // Only capture on pixel-stage draws.
  const uint32_t sm = static_cast<uint32_t>(stages);
  if (!(sm & static_cast<uint32_t>(reshade::api::shader_stage::pixel))) return;

  // Verify this is the lighting shader.
  auto* ss = renodx::utils::shader::GetCurrentState(cmd_list);
  if (!ss) return;
  uint32_t hash = renodx::utils::shader::GetCurrentPixelShaderHash(ss);
  if (!IsLightingShader(hash)) return;  // Only lighting shader (Sora + Kai)

  auto* ld = renodx::utils::pipeline_layout::GetPipelineLayoutData(layout);
  if (!ld) return;

  for (uint32_t i = 0; i < count; ++i) {
    uint32_t pi = first + i;
    if (pi >= ld->params.size()) continue;
    const auto& param = ld->params[pi];
    const auto& table = tables[i];
    if (!table.handle) continue;

    uint32_t rc = 0u;
    const reshade::api::descriptor_range* rr = nullptr;
    if (param.type == reshade::api::pipeline_layout_param_type::descriptor_table) {
      rc = param.descriptor_table.count; rr = param.descriptor_table.ranges;
    } else if (param.type == reshade::api::pipeline_layout_param_type::descriptor_table_with_static_samplers) {
      rc = param.descriptor_table_with_static_samplers.count;
      rr = param.descriptor_table_with_static_samplers.ranges;
    } else continue;
    if (!rr || rc == 0u) continue;

    for (uint32_t j = 0; j < rc; ++j) {
      const auto& r = rr[j];
      if (r.count == UINT32_MAX) continue;
      if (r.dx_register_space != 0u) continue;
      auto vm = static_cast<uint32_t>(r.visibility);
      if (!(vm & sm)) continue;

      auto resolve_tex = [&](uint32_t reg, reshade::api::resource_view* out) {
        if (reg < r.dx_register_index || reg >= r.dx_register_index + r.count) return;
        uint32_t di = reg - r.dx_register_index;
        uint32_t bo = 0u; reshade::api::descriptor_heap heap = {0u};
        device->get_descriptor_heap_offset(table, r.binding, di, &heap, &bo);
        if (!heap.handle) return;
        auto* dd = renodx::utils::data::Get<renodx::utils::descriptor::DeviceData>(device);
        if (!dd) return;
        std::shared_lock lock(dd->mutex);
        auto it = dd->heaps.find(heap.handle);
        if (it == dd->heaps.end() || bo >= it->second.size()) return;
        *out = it->second[bo].resource_view;
        d->captured_scene_cbv_frame = d->frame_index;
      };

      if (r.type == reshade::api::descriptor_type::texture_shader_resource_view) {
        // One register per slot. These constants are per-GAME, not per-pass: Kai
        // binds depth at t3 and ssao at t4, everything else binds depth at t4 and
        // ssao at t5. resolve_tex range-checks each call independently, so
        // resolving all four unconditionally lets a later call overwrite an
        // earlier one -- on Sora that left captured_depth_srv holding
        // mrtTexture2 (t3) and captured_ssao_srv holding depthTexture (t4).
        const bool kai = IsKai();
        resolve_tex(kai ? kLightingDepthRegisterKai : kLightingDepthRegister,
                    &d->captured_depth_srv);
        const uint64_t prev_ssao_handle = d->captured_ssao_srv.handle;
        resolve_tex(kai ? kLightingSsaoRegisterKai : kLightingSsaoRegister,
                    &d->captured_ssao_srv);
        if (d->captured_ssao_srv.handle != prev_ssao_handle) {
          d->captured_ssao_frame = d->frame_index;
        }
      }
      if (r.type == reshade::api::descriptor_type::constant_buffer) {
        if (!(vm & (sm | static_cast<uint32_t>(reshade::api::shader_stage::vertex)))) continue;
        if (kLightingSceneCbRegister < r.dx_register_index
            || kLightingSceneCbRegister >= r.dx_register_index + r.count) continue;
        uint32_t di = kLightingSceneCbRegister - r.dx_register_index;
        uint32_t bo = 0u; reshade::api::descriptor_heap heap = {0u};
        device->get_descriptor_heap_offset(table, r.binding, di, &heap, &bo);
        if (!heap.handle) continue;
        auto* dd = renodx::utils::data::Get<renodx::utils::descriptor::DeviceData>(device);
        if (!dd) continue;
        std::shared_lock lock(dd->mutex);
        auto it = dd->heaps.find(heap.handle);
        if (it == dd->heaps.end() || bo >= it->second.size()) continue;
        reshade::api::buffer_range cbv = it->second[bo].buffer_range;
        if (IsSceneCbvCandidateValid(device, cbv)) {
          d->captured_scene_cbv = cbv;
          d->captured_cbv_res = cbv.buffer.handle;
          d->captured_cbv_dims = std::string("buf:") + std::to_string(cbv.size);
          d->captured_scene_cbv_valid = true;
          d->captured_cbv_live = true;
          d->captured_scene_cbv_frame = d->frame_index;
        }
      }
    }
  }
}

// -- GTVBAO CS Dispatch Fix: clears stale compute bindings that cause double volumetrics --
static void ApplyGTVBAOCSDispatchFix(
    reshade::api::command_list* cmd_list,
    renodx::utils::state::CommandListState* cs,
    renodx::utils::state::CommandListState& prev) {
  int fix = (int)g_gtvbao_cs_dispatch_fix;
  if (fix < 1 || fix > 3) {
    // Fix 0 (Off): just null pipeline + struct copy
    cmd_list->bind_pipeline(reshade::api::pipeline_stage::all_compute, reshade::api::pipeline{0u});
    if (cs) *cs = prev;
    return;
  }

  // Fix 1/2/3: properly save and restore compute state
  // GTVBAO binds descriptors via bind_descriptor_tables with a proper pipeline layout.
  // We save the previous compute pipeline + descriptor tables, then restore them.
  // This handles both "there were previous compute bindings" and "clean slate" cases.
  reshade::api::pipeline prev_compute_pipeline = {0u};
  reshade::api::pipeline_layout prev_layout = {0u};
  std::vector<reshade::api::descriptor_table> prev_tables;
  if (cs) {
    auto it = cs->pipelines.find(reshade::api::pipeline_stage::all_compute);
    if (it != cs->pipelines.end()) prev_compute_pipeline = it->second;
    prev_layout = cs->compute_pipeline_layout;
    prev_tables = cs->compute_descriptor_tables;
  }

  if (fix == 2 || fix == 3) {
    // Additionally null the individual compute slots (belt and suspenders)
    reshade::api::resource_view null_srv = {};
    reshade::api::resource_view null_uav = {};
    reshade::api::sampler null_sampler = {};
    cmd_list->push_descriptors(reshade::api::shader_stage::all_compute,
        reshade::api::pipeline_layout{0}, 0,
        reshade::api::descriptor_table_update{{}, 0, 0, 1, reshade::api::descriptor_type::sampler, &null_sampler});
    for (int i = 0; i < 5; ++i)
      cmd_list->push_descriptors(reshade::api::shader_stage::all_compute,
          reshade::api::pipeline_layout{0}, 0,
          reshade::api::descriptor_table_update{{}, (uint32_t)i, 0, 1, reshade::api::descriptor_type::texture_shader_resource_view, &null_srv});
    for (int i = 0; i < 4; ++i)
      cmd_list->push_descriptors(reshade::api::shader_stage::all_compute,
          reshade::api::pipeline_layout{0}, 0,
          reshade::api::descriptor_table_update{{}, (uint32_t)i, 0, 1, reshade::api::descriptor_type::texture_unordered_access_view, &null_uav});
  }

  // Restore previous compute descriptor tables (if any were bound)
  if (prev_layout.handle != 0u && !prev_tables.empty()) {
    cmd_list->bind_descriptor_tables(
        reshade::api::shader_stage::all_compute,
        prev_layout, 0,
        static_cast<uint32_t>(prev_tables.size()),
        prev_tables.data());
  }

  // Restore previous compute pipeline (or null it)
  cmd_list->bind_pipeline(reshade::api::pipeline_stage::all_compute, prev_compute_pipeline);

  if (cs) *cs = prev;
}

// -- Custom Shader crash-tracing log --
// Changes-only step log for GTVBAO + Dynamic Cubemaps + SSR dispatches. Gated by the
// "Custom Shader Log" toggle (default Off): zero overhead and zero log lines when off.
// Semantics per tag: first sight and any change log immediately; identical repeats
// never re-log (steady state is silent); warnings repeat at most once per second.
// A 1/sec "beat" line carries frame + key state as the crash recency anchor.
// ReShade log flushes synchronously per call, so the last lines before a crash survive.
// Log mutex: destroy-event handlers can fire on loader threads while the render
// thread logs. Recursive (handlers log through CSLog while holding it).
static std::recursive_mutex s_cslogMutex;
static void CSLog(const char* tag, const std::string& msg, bool warn) {
  if (shader_injection.custom_shader_logging < 0.5f) return;
  std::lock_guard<std::recursive_mutex> csLogLock(s_cslogMutex);
  if (shader_injection.custom_shader_logging < 0.5f) return;
  using clock = std::chrono::steady_clock;
  // Per-message suppression: alternating messages under one tag must not defeat
  // the throttle (each differs from the previous). First sight and any change log
  // immediately; identical repeats never re-log; warnings repeat at most 1/sec.
  // Callers keep rotating values (e.g. frame numbers) OUT of messages.
  static std::map<std::string, clock::time_point> s_last;
  const auto now = clock::now();
  const std::string key = std::string(tag) + '\x1f' + msg;
  auto it = s_last.find(key);
  if (it != s_last.end()) {
    if (!warn) return;
    if ((now - it->second) < std::chrono::seconds(1)) return;
  }
  s_last[key] = now;
  reshade::log::message(warn ? reshade::log::level::warning : reshade::log::level::info,
    (std::string("[CustomShader] ") + tag + ": " + msg).c_str());
}

// 1/sec heartbeat emitter (recency anchor). Gated by the same toggle.
static void CSBeat(const std::string& msg) {
  if (shader_injection.custom_shader_logging < 0.5f) return;
  std::lock_guard<std::recursive_mutex> csBeatLock(s_cslogMutex);
  using clock = std::chrono::steady_clock;
  static clock::time_point s_last{};
  static bool s_init = false;
  const auto now = clock::now();
  if (s_init && (now - s_last) < std::chrono::seconds(1)) return;
  s_init = true;
  s_last = now;
  reshade::log::message(reshade::log::level::info,
    (std::string("[CustomShader] beat: ") + msg).c_str());
}

// Scene liveness for transition gating: true while lighting draws are stamping
// fresh captures. False across loading screens (and before first capture).
// The t17 swap and present-time readers use it to stay off dead inputs.
static bool DynCubeSceneLive(const DeviceData* d) {
  if (!d) return false;
  if (d->captured_color_frame == UINT64_MAX) return false;
  if (d->frame_index < d->captured_color_frame) return false;
  return (d->frame_index - d->captured_color_frame) <= kDynCubeLoadingStaleFrames;
}

// ViewDims for crash tracing: "WxH" for textures, "buf:N" for buffers, "null" for
// empty handles, "dead" when the view no longer resolves to a resource (the game
// freed the target � e.g. a DLSS/resolution realloc the addon was never told about).
static std::string CSViewDims(reshade::api::device* dev, reshade::api::resource_view v) {
  if (!dev || !v.handle) return "null";
  auto res = dev->get_resource_from_view(v);
  if (!res.handle) return "dead";
  auto desc = dev->get_resource_desc(res);
  if (desc.type == reshade::api::resource_type::buffer)
    return "buf:" + std::to_string(desc.buffer.size);
  return std::to_string(desc.texture.width) + "x" + std::to_string(desc.texture.height);
}

// Push/bind-context ONLY capture resolver: describes a captured view and caches it
// (proven-safe query context, same calls the t17 swap already makes per bind).
// Never call from draw/present paths � liveness there comes from destroy events.
static CapturedViewInfo CSResolveCapture(reshade::api::device* dev, reshade::api::resource_view v) {
  CapturedViewInfo info = {0u, "null", 0u, 0u};
  if (!dev || !v.handle) return info;
  auto res = dev->get_resource_from_view(v);
  if (!res.handle) { info.dims = "dead"; return info; }
  info.res = res.handle;
  auto desc = dev->get_resource_desc(res);
  if (desc.type == reshade::api::resource_type::buffer) {
    info.dims = "buf:" + std::to_string(desc.buffer.size);
  } else {
    info.w = desc.texture.width;
    info.h = desc.texture.height;
    info.dims = std::to_string(info.w) + "x" + std::to_string(info.h);
  }
  return info;
}

// Destroy-event liveness: the authoritative death source for tracked game inputs.
// Runs on whatever thread frees the target (often the loader thread) � touches only
// tracked handles/flags plus the mutex-guarded log. No view queries of any kind.
static void KillTrackedInput(DeviceData* d, reshade::api::resource_view tracked, uint64_t resHandle,
                             std::atomic<bool>& live, const char* name, uint64_t deadView, uint64_t deadRes) {
  if (!d) return;
  if (tracked.handle != 0u && (tracked.handle == deadView || (resHandle != 0u && resHandle == deadRes))) {
    if (live.load()) {
      live.store(false);
      CSLog("capture", std::string(name) + " view DEAD", true);
    }
  }
}
static void KillAllTracked(DeviceData* d, uint64_t deadView, uint64_t deadRes) {
  if (!d) return;
  KillTrackedInput(d, d->captured_depth_srv, d->captured_depth_res, d->captured_depth_live, "depth", deadView, deadRes);
  KillTrackedInput(d, d->captured_color_srv, d->captured_color_res, d->captured_color_live, "color", deadView, deadRes);
  KillTrackedInput(d, d->captured_mrt_normal_srv, d->captured_mrt_res, d->captured_mrt_live, "mrt", deadView, deadRes);
  KillTrackedInput(d, d->captured_scene_cbv_view, d->captured_cbv_res, d->captured_cbv_live, "cbv", deadView, deadRes);
  KillTrackedInput(d, d->deferred_depth_srv, d->deferred_depth_res, d->defDepthLive, "defDepth", deadView, deadRes);
  KillTrackedInput(d, d->deferred_mrt_normal_srv, d->deferred_mrt_res, d->defMrtLive, "defMrt", deadView, deadRes);
  KillTrackedInput(d, d->deferred_scene_cbv_view, d->deferred_cbv_res, d->defCbvLive, "defCbv", deadView, deadRes);
  KillTrackedInput(d, d->captured_ssr1_srv, d->captured_ssr1_res, d->captured_ssr1_live, "ssr1", deadView, deadRes);
  KillTrackedInput(d, d->captured_ssr_mrt_srv, d->captured_ssr_mrt_res, d->captured_ssr_mrt_live, "ssrMrt", deadView, deadRes);
  KillTrackedInput(d, d->rcas_motion_srv, d->rcas_motion_res, d->rcas_motion_live, "rcasMotion", deadView, deadRes);
  KillTrackedInput(d, d->mb_rtv4_srv, d->mb_rtv4_res, d->mb_rtv4_live, "mbRtv4Motion", deadView, deadRes);
  KillTrackedInput(d, d->mb_tonemap_src_srv, d->mb_tonemap_src_res, d->mb_tonemap_src_live, "mbTonemapSrc", deadView, deadRes);
}
static void OnDestroyResourceView(reshade::api::device* device, reshade::api::resource_view view) {
  if (!device || !view.handle) return;
  auto* d = device->get_private_data<DeviceData>();
  if (!d) return;
  KillAllTracked(d, view.handle, 0u);
}
static void OnDestroyResource(reshade::api::device* device, reshade::api::resource res) {
  if (!device || !res.handle) return;
  auto* d = device->get_private_data<DeviceData>();
  if (!d) return;
  KillAllTracked(d, 0u, res.handle);
  // The motion resource died, so our SRV onto it is dead too. Flag it rather than
  // destroying it here: this handler runs on the loader thread, and the render
  // thread rebuilds it via MBMotionRtv4Ensure. Leaving a dead view bound for even
  // one frame is the thing that must not happen, and the flag is set before the
  // next chain run, which is the same frame's tonemap.
  if (d->mb_rtv4_res != 0u && d->mb_rtv4_res == res.handle) {
    d->mb_rtv4_res = 0u;
    d->mb_rtv4_owned_dirty = true;
  }
  // RCAS note: no per-resource tracking remains. The dispatch reads only the
  // owned unsharpened copy; a dead game target is detected next TAA drawn via
  // dims/format mismatch (set recreated) or desc query failure (skip frame).
}

// Present watchdog: proves render-thread stalls vs process death across silent gaps.
// Touches no D3D state (one atomic timestamp). Silent unless presents stall >2.5s.
static std::atomic<uint64_t> s_lastPresentMs{0};
static std::atomic<bool> s_watchdogStop{false};
static std::thread s_watchdogThread;
static bool s_watchdogStarted = false;

// Render-thread region tracking: the watchdog reports where a stalled present stopped.
// 1=entry 2=liveness-resolver 3=recreate 4=deferred-dispatch; 0=idle (between presents).
// RAII scope resets to idle on EVERY return path so a stale region can never mislead.
static std::atomic<int> s_presentRegion{0};
static const char* PresentRegionName(int r) {
  switch (r) {
    case 1: return "entry";
    case 2: return "resolver";
    case 3: return "recreate";
    case 4: return "deferred";
    default: return "idle";
  }
}
struct PresentRegionScope {
  PresentRegionScope() { s_presentRegion.store(1); }
  ~PresentRegionScope() { s_presentRegion.store(0); }
};
static void WatchdogThreadMain() {
  using clock = std::chrono::steady_clock;
  auto now_ms = []() -> uint64_t {
    return (uint64_t)std::chrono::duration_cast<std::chrono::milliseconds>(
        clock::now().time_since_epoch()).count();
  };
  while (!s_watchdogStop.load()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    if (shader_injection.custom_shader_logging < 0.5f) continue;
    const uint64_t last = s_lastPresentMs.load();
    if (last == 0u) continue;
    const uint64_t now = now_ms();
    const uint64_t gap = (now >= last) ? (now - last) : 0u;
    if (gap > 2500u) {
      const int region = s_presentRegion.load();
      CSLog("watchdog", std::string("no present for ") + std::to_string(gap / 1000u) +
        "s (stuck in " + PresentRegionName(region) + "?)", true);
    }
  }
}
static inline uint64_t WatchdogNowMs() {
  return (uint64_t)std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now().time_since_epoch()).count();
}

// -- Present hook --

static void OnPresent(reshade::api::command_queue* queue, reshade::api::swapchain* sc,
                       const reshade::api::rect*, const reshade::api::rect*,
                       uint32_t, const reshade::api::rect*) {
  auto* dev = queue->get_device();
  auto* cl = queue->get_immediate_command_list();
  auto* d = dev->get_private_data<DeviceData>();
  if (!d) return;
  d->frame_index++;
  // RTV4 re-arm budget is per frame; a rejected candidate may be retried a few
  // times, but never across a frame boundary.
  d->mb_rtv4_rejects = 0u;
  d->immediate_cmd_list = queue->get_immediate_command_list();
  s_lastPresentMs.store(WatchdogNowMs());
  if (!s_watchdogStarted) {
    s_watchdogStarted = true;
    s_watchdogThread = std::thread(&WatchdogThreadMain);
  }
  PresentRegionScope presentRegionScope;

  // 1/sec heartbeat: recency anchor for crashes + transition detector (photo mode,
  // DLSS toggle, loads). Everything else in this log is changes-only, so steady
  // state is this line alone.
  CSBeat(std::string("frame=") + std::to_string(d->frame_index) +
    " working=" + std::to_string(d->working_width) + "x" + std::to_string(d->working_height) +
    " liveDepth=" + (d->captured_depth_srv.handle ? d->captured_depth_dims : "none") +
    " cbv=" + (d->captured_scene_cbv_valid ? "ok" : "MISS") +
    " gtvbaoRes=" + (d->resources_created ? "1" : "0") +
    " cubeRes=" + (d->dyncube_resources_created ? "1" : "0") +
    " rs=" + std::to_string(d->dyncube_readSet) +
    " flt=" + std::to_string(d->dyncube_filteredReadSet) +
    " rej=" + std::to_string(d->dyncube_rejected_captures));

  // DynCube loading wipe (generic, all games): with no fresh lighting t0 for a
  // while, no scene draws are running (loading screen) � arm history
  // hard-replace so the next scene rebuilds from scratch instead of blending
  // stale backdrops. Runs before any early-out; arming is idempotent (consumed
  // once by the next dispatched capture) and safe on hitches (threshold).
  // Covers the opaque-backdrop blind spot geometry rejection admits it can't see.
  if (d->dyncube_resources_created
      && d->captured_color_frame != UINT64_MAX
      && d->frame_index >= d->captured_color_frame
      && (d->frame_index - d->captured_color_frame) > kDynCubeLoadingStaleFrames) {
    d->dyncube_rejectedGap = true;
    // The DynCube scheduler only ticks on lighting draws, so it never runs during
    // a load: hand the wipe to the next scheduler tick via a pending flag.
    if (!d->dyncube_loadingWipeDone) {
      d->dyncube_loadingWipeDone = true;
      d->dyncube_loadingWipePending = true;
    }
  } else {
    d->dyncube_loadingWipeDone = false;
  }

  // DynCube disable path (retained, currently untriggered): frees the resource set
  // at the frame boundary. Toggle-off no longer arms this; device-loss/swapchain
  // paths call Destroy directly. Runs before any early-out so it also happens
  // when every feature is off.
  if (d->dyncube_pending_destroy) {
    d->dyncube_pending_destroy = false;
    d->dyncube_pending_recreate = false;
    if (shader_injection.dynCube_debug_logging > 0.5f && d->dyncube_resources_created) {
      reshade::log::message(reshade::log::level::info,
        (std::string("[DynCube] disabled: freed ") + std::to_string(d->dyncube_size) + "x" + std::to_string(d->dyncube_size) +
         " set (~" + std::to_string((long long)(DynCubeEstimatedBytes(d) / (1024ull * 1024ull))) + " MB)").c_str());
    }
    DestroyDynCubeResources(dev, d);
  }
// -- Basic mode startup guard: reset advanced-only settings to defaults if Basic is selected --
  static bool s_basic_startup_checked = false;
  if (!s_basic_startup_checked) {
    s_basic_startup_checked = true;
    if (g_settings_mode < 0.5f) {
      float saved = g_settings_mode;
      g_settings_mode = 1.0f;
      std::vector<renodx::utils::settings::Setting*> advanced;
      for (auto* s : settings) {
        if (s->key.empty() || !s->can_reset || s->is_global) continue;
        if (s->is_visible()) advanced.push_back(s);
      }
      g_settings_mode = saved;
      for (auto* s : advanced) {
        if (!s->is_visible()) {
          s->Set(s->default_value);
          s->Write();
        }
      }
    }
  }

  // DynCube can dispatch independently of GTVBAO � must not early-out
  const bool dynCube_present_active = shader_injection.dynCube_enabled > 0.5f;
  if (shader_injection.gtvbao_mode < 0.5f && !dynCube_present_active) return;

  // Liveness checkpoint (region marker only): input liveness is maintained by
  // capture edges (bind = live) and destroy events (free = dead) � no view
  // queries here or anywhere on the render thread outside push/bind captures.
  s_presentRegion.store(2);
  if (d->frame_index <= kGTVBAOStartupGuardFrames) {
    if (d->frame_index == kGTVBAOStartupGuardFrames) {
      reshade::log::message(reshade::log::level::info,
        "[GTVBAO] Startup guard complete � dispatch begins next frame.");
    }
    return;
  }
  if (d->frame_index < d->resize_guard_until_frame) {
    CSLog("present", "in resize guard");
    return;
  }

  // DynCube cube-resolution change: at the frame boundary, destroy the active set
  // and create one of the target size. No per-size sets are retained, so the peak
  // is a single set regardless of how many resolutions were visited this session.
  s_presentRegion.store(3);
  if (d->dyncube_pending_recreate && d->dyncube_pending_size != 0u) {
    const uint32_t wantSize = d->dyncube_pending_size;
    // CreateDynCubeResources destroys the active set before allocating, so the old
    // size is always released (no leak) and never overlaps the new one.
    CreateDynCubeResources(dev, d, wantSize);
    CreateDynCubePipelinesIfNeeded(dev, d);
    if (shader_injection.dynCube_debug_logging > 0.5f) {
      reshade::log::message(reshade::log::level::info,
        (std::string("[DynCube] resized to ") + std::to_string(wantSize)).c_str());
    }
    CSLog("dyncube", std::string("cube recreate: want=") + std::to_string(wantSize) +
      " active=" + std::to_string(d->dyncube_size));
    d->dyncube_pending_size = 0u;
    d->dyncube_pending_recreate = false;
  }

  // Create / recreate resources using depth texture size (kai pattern).
  {
    uint32_t gw = 0u, gh = 0u;
    if (d->captured_depth_srv.handle != 0u) {
      auto depth_res = dev->get_resource_from_view(d->captured_depth_srv);
      if (depth_res.handle != 0u) {
        auto dd = dev->get_resource_desc(depth_res);
        gw = dd.texture.width;
        gh = dd.texture.height;
      }
    }
    if (gw < 64u || gh < 64u) {
      auto bb = sc->get_back_buffer(0);
      auto bd = dev->get_resource_desc(bb);
      gw = bd.texture.width;
      gh = bd.texture.height;
    } else {
      // Validate: unconditional capture might pick up small SSAO at t4 (e.g. 160x90).
      auto bb = sc->get_back_buffer(0);
      auto bd = dev->get_resource_desc(bb);
      if (gw < bd.texture.width / 4u || gh < bd.texture.height / 4u) {
        gw = bd.texture.width;
        gh = bd.texture.height;
      }
    }
    const bool too_small = d->working_width < 320u || d->working_height < 320u;
    const float want_res = shader_injection.gtvbao_resolution > 0.5f ? 1.f : 0.f;
    if (gw > 0u && gh > 0u
        && (!d->resources_created || too_small
            || gw != d->last_created_game_width
            || gh != d->last_created_game_height
            || want_res != d->last_created_gtvbao_resolution)) {
      CreateGTVBAOResources(dev, d, gw, gh);
      d->last_created_game_width = gw;
      d->last_created_game_height = gh;
      d->resources_created = true;
      CSLog("gtvbao", std::string("resources (re)created: working=") +
        std::to_string(d->working_width) + "x" + std::to_string(d->working_height) +
        " depth=" + std::to_string(gw) + "x" + std::to_string(gh));
      reshade::log::message(reshade::log::level::info,
        (std::string("[GTVBAO] Resources created: ") +
         std::to_string(d->working_width) + "x" +
         std::to_string(d->working_height) + " (depth=" +
         std::to_string(gw) + "x" + std::to_string(gh) + ")").c_str());
    }
  }
  // Use deferred snapshots from lighting draw (kai-style) � deferred dispatch only.
  // -- Light-buffer capture helper (runs after GTVBAO for multi-bounce feedback) --
  auto capture_light_buffer_for_next_frame = [&]() {
    if (shader_injection.vbgi_enabled < 0.5f || !d->captured_light_buffer_texture.handle) return;
    // Skip when captured color is live: both consumers (multibounce accumulate,
    // main light-buffer select) prefer it and touch the light buffer only as
    // fallback (terminal fallback_srv preserved).
    if (d->captured_color_srv.handle) return;
    auto bb = sc->get_back_buffer(0);
    if (!bb.handle) return;
    // Recreate capture texture if back buffer format changed (e.g. HDR vs SDR mismatch).
    auto bb_desc = dev->get_resource_desc(bb);
    auto cap_desc = dev->get_resource_desc(d->captured_light_buffer_texture);
    if (bb_desc.texture.format != cap_desc.texture.format
        || bb_desc.texture.width != cap_desc.texture.width
        || bb_desc.texture.height != cap_desc.texture.height) {
      if (d->captured_light_buffer_srv.handle) dev->destroy_resource_view(d->captured_light_buffer_srv);
      if (d->captured_light_buffer_texture.handle) dev->destroy_resource(d->captured_light_buffer_texture);
      d->captured_light_buffer_srv = {};
      d->captured_light_buffer_texture = {};
      d->captured_light_buffer_valid = false;
      reshade::api::resource_desc rd = {};
      rd.type = reshade::api::resource_type::texture_2d;
      rd.texture = {bb_desc.texture.width, bb_desc.texture.height, 1, 1, bb_desc.texture.format, 1};
      rd.heap = reshade::api::memory_heap::gpu_only;
      rd.usage = reshade::api::resource_usage::shader_resource | reshade::api::resource_usage::copy_dest;
      dev->create_resource(rd, nullptr, reshade::api::resource_usage::shader_resource,
                           &d->captured_light_buffer_texture);
      dev->create_resource_view(d->captured_light_buffer_texture,
                                 reshade::api::resource_usage::shader_resource,
                                 reshade::api::resource_view_desc(
                                     reshade::api::resource_view_type::texture_2d,
                                     bb_desc.texture.format, 0, 1, 0, 1),
                                 &d->captured_light_buffer_srv);
    }
    cl->barrier(bb, reshade::api::resource_usage::present,
                reshade::api::resource_usage::copy_source);
    cl->barrier(d->captured_light_buffer_texture,
                reshade::api::resource_usage::shader_resource,
                reshade::api::resource_usage::copy_dest);
    cl->copy_texture_region(bb, 0, nullptr,
                            d->captured_light_buffer_texture, 0, nullptr);
    cl->barrier(d->captured_light_buffer_texture,
                reshade::api::resource_usage::copy_dest,
                reshade::api::resource_usage::shader_resource);
    cl->barrier(bb, reshade::api::resource_usage::copy_source,
                reshade::api::resource_usage::present);
    d->captured_light_buffer_valid = true;
  };

  // Inline dispatch active (deferred off) � GTVBAO runs during lighting pass, not here.
  // Drop snapshots whose targets died since capture (destroy events flip the flags;
  // no queries here). Dispatching on them is never correct.
  s_presentRegion.store(4);
  if (d->deferred_pending && (!d->defDepthLive || !d->defMrtLive || !d->defCbvLive)) {
    CSLog("gtvbao", "deferred snapshot DEAD: dropped", true);
    d->deferred_pending = false;
  }
  if (!d->deferred_pending || !d->deferred_depth_srv.handle) {
    capture_light_buffer_for_next_frame();
    // DynCube debug face preview � must run even when GTVBAO deferred off
    if (d->dyncube_resources_created && d->dyncube_srv.handle && shader_injection.dynCube_enabled >0.5f
        && (shader_injection.dynCube_debug == 1.f || shader_injection.dynCube_debug == 2.f
            || shader_injection.dynCube_debug == 5.f || shader_injection.dynCube_debug == 6.f
            || shader_injection.dynCube_debug == 7.f || shader_injection.dynCube_debug == 8.f)) {
      int face = (int)std::clamp(shader_injection.dynCube_debug_face, 0.f, 5.f);
      const uint32_t outSet = d->dyncube_readSet; // validated read set (delayed-validate commit)
      reshade::api::resource srcTex = d->dyncube_texture;
      uint32_t srcSub = (uint32_t)face;
      uint32_t srcW = 0, srcH = 0;
      if (shader_injection.dynCube_debug == 5.f) srcTex = d->dyncube_hist[outSet].pos;
      else if (shader_injection.dynCube_debug == 6.f) srcTex = d->dyncube_hist[outSet].contrib;
      else if (shader_injection.dynCube_debug == 7.f) srcTex = d->dyncube_charmask;
      else if (shader_injection.dynCube_debug == 8.f) {
        // GGX filtered cube: preview a specific mip of the ACTIVE completed cube.
        srcTex = d->dyncube_ggx_out[d->dyncube_ggx_active];
        int mip = (int)std::clamp(shader_injection.dynCube_debug_mip, 0.f, (float)(d->dyncube_mip_count - 1));
        srcSub = (uint32_t)mip + (uint32_t)face * d->dyncube_mip_count;
        srcW = std::max(1u, d->dyncube_size >> mip);
        srcH = srcW;
      }
      auto bb = sc->get_back_buffer(0);
      if (bb.handle && srcTex.handle) {
        auto bbDesc = dev->get_resource_desc(bb);
        uint32_t srcDim = (srcW != 0u) ? srcW : d->dyncube_size; // source box dims (mip-sized for debug 10)
        uint32_t preview = std::min(d->dyncube_size * 2, 512u);  // display rect constant so mip progression stays visible
        if (preview <= bbDesc.texture.width && preview <= bbDesc.texture.height) {
          reshade::api::subresource_box srcBox = {0,0,0, srcDim, srcDim, 1};
          reshade::api::subresource_box dstBox = {0,0,0, preview, preview, 1};
          cl->barrier(srcTex, reshade::api::resource_usage::shader_resource, reshade::api::resource_usage::copy_source);
          cl->barrier(bb, reshade::api::resource_usage::present, reshade::api::resource_usage::copy_dest);
          cl->copy_texture_region(srcTex, srcSub, &srcBox, bb, 0, &dstBox, reshade::api::filter_mode::min_mag_mip_point);
          cl->barrier(srcTex, reshade::api::resource_usage::copy_source, reshade::api::resource_usage::shader_resource);
          cl->barrier(bb, reshade::api::resource_usage::copy_dest, reshade::api::resource_usage::present);
        }
      }
    }
    return;
  }
  if (!d->deferred_scene_cbv_valid
      || (d->frame_index - d->deferred_scene_cbv_frame) > 1u) {
    capture_light_buffer_for_next_frame();
    if (shader_injection.gtvbao_debug_logging > 0.5f) {
      reshade::log::message(reshade::log::level::warning,
                            "[GTVBAO] Dispatch skipped: no deferred scene CBV.");
    }
    return;
  }
  // Restore deferred snapshots as active captures for RunGTVBAO / RunVBGI.
  // Liveness travels with the views (plain flag copies, no queries).
  d->captured_depth_srv = d->deferred_depth_srv;
  d->captured_depth_res = d->deferred_depth_res;
  d->captured_depth_live = d->defDepthLive.load();
  // deferred_ssao_srv is only populated by the lighting-draw snapshot; when the
  // snapshot predates that field it stays zero, and restoring a zero here would
  // discard a perfectly good same-frame capture. Restore only a real handle.
  if (d->deferred_ssao_srv.handle) {
    d->captured_ssao_srv = d->deferred_ssao_srv;
    d->captured_ssao_frame = d->frame_index;
  }
  d->captured_mrt_normal_srv = d->deferred_mrt_normal_srv;
  d->captured_mrt_res = d->deferred_mrt_res;
  d->captured_mrt_live = d->defMrtLive.load();
  d->captured_scene_cbv_view = d->deferred_scene_cbv_view;
  d->captured_scene_cbv = d->deferred_scene_cbv;
  d->captured_scene_cbv_valid = d->deferred_scene_cbv_valid;
  d->captured_cbv_res = d->deferred_cbv_res;
  d->captured_cbv_live = d->defCbvLive.load();
  d->captured_scene_cbv_frame = d->deferred_scene_cbv_frame;
  d->deferred_pending = false;

  // GTVBAO reads proj_g directly from the game's scene CBV (b0) in-shader �
  // no CPU-side mapping needed (kai-vanillaplus approach).

  if (shader_injection.gtvbao_debug_logging > 0.5f)
    reshade::log::message(reshade::log::level::info,
      (std::string("[GTVBAO] Dispatching (frame=") +
       std::to_string(d->frame_index) + ", res=" +
       std::to_string(d->working_width) + "x" +
       std::to_string(d->working_height) + ")").c_str());

  // Save command-list state.
  auto* cs = renodx::utils::state::GetCurrentState(cl);
  renodx::utils::state::CommandListState prev = {};
  if (cs) prev = *cs;

  bool ok = true;
  if (shader_injection.gtvbao_mode > 0.5f) {
    const std::string liveDepth = d->captured_depth_srv.handle ? d->captured_depth_dims : "none";
    const std::string wantDims = std::to_string(d->working_width) + "x" + std::to_string(d->working_height);
    const bool dimMismatch = (liveDepth != wantDims);
    CSLog("gtvbao", std::string("invoke working=") + wantDims + " liveDepth=" + liveDepth +
      " cbv=" + (d->captured_scene_cbv_valid ? "ok" : "MISSING"), dimMismatch || !d->captured_scene_cbv_valid);
    ok = RunGTVBAO(cl, d);
    CSLog("gtvbao", std::string("invoke exit") + (ok ? " ok" : " FAILED"), !ok);
  }

  // Restore: apply dispatch fix, then restore previous state.
  ApplyGTVBAOCSDispatchFix(cl, cs, prev);

  if (shader_injection.gtvbao_debug_logging > 0.5f && ok) {
    std::ostringstream msg;
    msg << "[GTVBAO] Dispatch OK (frame=" << d->frame_index
        << ", res=" << d->working_width << "x" << d->working_height << ")";
    reshade::log::message(reshade::log::level::info, msg.str().c_str());
  } else if (shader_injection.gtvbao_debug_logging > 0.5f && !ok) {
    reshade::log::message(reshade::log::level::warning, "[GTVBAO] Dispatch failed.");
  }

  // -- GI is now integrated into GTVBAO main pass (visibility bitmask AO+GI). --
  // The GI output (vbgi_denoised_srv) is produced during RunGTVBAO denoise pass.
  // No separate VBGI dispatch needed.

  // -- Capture light buffer for next frame's multi-bounce (after GI applied) --
  capture_light_buffer_for_next_frame();

  // -- DynCube debug face preview � samples capture resources directly (not t17) --
  if (d->dyncube_resources_created && d->dyncube_srv.handle && shader_injection.dynCube_enabled >0.5f
      && (shader_injection.dynCube_debug == 1.f || shader_injection.dynCube_debug == 2.f
          || shader_injection.dynCube_debug == 5.f || shader_injection.dynCube_debug == 6.f
          || shader_injection.dynCube_debug == 7.f || shader_injection.dynCube_debug == 8.f)) {
    int face = (int)std::clamp(shader_injection.dynCube_debug_face, 0.f, 5.f);
      const uint32_t outSet = d->dyncube_readSet; // validated read set (delayed-validate commit)
      reshade::api::resource srcTex = d->dyncube_texture;
    uint32_t srcSub = (uint32_t)face;
    uint32_t srcW = 0, srcH = 0;
    if (shader_injection.dynCube_debug == 5.f) srcTex = d->dyncube_hist[outSet].pos;
    else if (shader_injection.dynCube_debug == 6.f) srcTex = d->dyncube_hist[outSet].contrib;
    else if (shader_injection.dynCube_debug == 7.f) srcTex = d->dyncube_charmask;
    else if (shader_injection.dynCube_debug == 8.f) {
      // GGX filtered cube: preview a specific mip of the ACTIVE completed cube.
      srcTex = d->dyncube_ggx_out[d->dyncube_ggx_active];
      int mip = (int)std::clamp(shader_injection.dynCube_debug_mip, 0.f, (float)(d->dyncube_mip_count - 1));
      srcSub = (uint32_t)mip + (uint32_t)face * d->dyncube_mip_count;
      srcW = std::max(1u, d->dyncube_size >> mip);
      srcH = srcW;
    }
    auto bb = sc->get_back_buffer(0);
    if (bb.handle && srcTex.handle) {
      auto bbDesc = dev->get_resource_desc(bb);
      uint32_t srcDim = (srcW != 0u) ? srcW : d->dyncube_size; // source box dims (mip-sized for debug 10)
      uint32_t preview = std::min(d->dyncube_size * 2, 512u);  // display rect constant so mip progression stays visible
      if (preview <= bbDesc.texture.width && preview <= bbDesc.texture.height) {
        reshade::api::subresource_box srcBox = {0,0,0, srcDim, srcDim, 1};
        reshade::api::subresource_box dstBox = {0,0,0, preview, preview, 1};
        cl->barrier(srcTex, reshade::api::resource_usage::shader_resource, reshade::api::resource_usage::copy_source);
        cl->barrier(bb, reshade::api::resource_usage::present, reshade::api::resource_usage::copy_dest);
        cl->copy_texture_region(srcTex, srcSub, &srcBox, bb, 0, &dstBox, reshade::api::filter_mode::min_mag_mip_point);
        cl->barrier(srcTex, reshade::api::resource_usage::copy_source, reshade::api::resource_usage::shader_resource);
        cl->barrier(bb, reshade::api::resource_usage::copy_dest, reshade::api::resource_usage::present);
      }
    }
  }

  shader_injection.gtvbao_vbgi_bound = 0.f;  // Reset for next frame's SSAO pass
}

// -- Sora2nd SSR Replacement (vanilla march at ssr1, composite at ssr2) --
// Master gate: the composite can only serve when a vanilla fallback and a
// servable dynamic cube exist. Deliberately NOT gated on the custom
// SSR toggle or its result: the resolved SSR source is the vanilla march
// (game-bound t0, live exactly when the Game SSR toggle runs it), so replacement
// works standalone. Game-SSR-off degrades to dynamic/vanilla inside the shared
// resolver. Otherwise vanilla passes run.
static bool SoraSSRReplaceActive(reshade::api::command_list* cmd_list) {
  if (!cmd_list) return false;
  if (shader_injection.dynCube_ssr_replacement < 0.5f) return false;
  if (shader_injection.dynCube_enabled < 0.5f
      || shader_injection.dynCube_force_vanilla > 0.5f
      || shader_injection.dynCube_debug == 4.f) return false;
  auto* dev = cmd_list->get_device();
  if (!dev) return false;
  auto* dd = dev->get_private_data<DeviceData>();
  if (!dd) return false;
  if (!dd->captured_vanilla_env_srv.handle) return false;
  reshade::api::resource_view t17srv = dd->dyncube_ggx_valid
      ? dd->dyncube_ggx_out_cube_srv[dd->dyncube_ggx_active]
      : dd->dyncube_srv;
  if (!t17srv.handle) return false;
  return true;
}

// ssr1: run the vanilla march, except skip it when replacement is on and Game SSR
// is off (nothing consumes the march output then; the composite resolves
// dynamic/vanilla only). No pushes needed: vanilla uses only game bindings.
static bool OnBeforeSoraSSR1Draw(reshade::api::command_list* cmd_list) {
  if (shader_injection.dynCube_ssr_replacement > 0.5f
      && shader_injection.dynCube_game_ssr < 0.5f) return false;
  return true;
}

// ssr1 code replacement gate: the tree file must actually execute whenever any
// consumer needs its edited behavior � the full replacement chain, or any
// Vanilla SSR Improvements toggle (future toggles extend this OR). False keeps
// the game bytecode (fully vanilla path, bit-exact).
static bool OnReplaceSoraSSR1Draw(reshade::api::command_list* cmd_list) {
  if (SoraSSRReplaceActive(cmd_list)) return true;
  if (shader_injection.dynCube_vanilla_ssr_enabled > 0.5f
      && shader_injection.dynCube_vanilla_refine_fix > 0.5f) return true;
  return false;
}
// ssr2 vanilla-improvement gate: the tree file must execute (instead of game
// bytecode) whenever any Vanilla SSR Improvements toggle changes vanilla ssr2
// behavior � motion-adaptive history, non-0.9 fixed weight, disocclusion
// reject, or usable IS-FAST distribution. All off = bytecode, bit-exact.
// (The replacement chain serves via SoraSSRReplaceActive separately.)
static bool SoraSSRVanillaActive(reshade::api::command_list* cmd_list) {
  if (!cmd_list) return false;
  if (shader_injection.dynCube_ssr_replacement > 0.5f) return false;
  if (shader_injection.dynCube_enabled < 0.5f
      || shader_injection.dynCube_force_vanilla > 0.5f
      || shader_injection.dynCube_debug == 4.f) return false;
  if (shader_injection.dynCube_vanilla_ssr_enabled < 0.5f) return false;
  if (shader_injection.dynCube_vanilla_history_fixed < 0.5f) return true;  // motion-adaptive differs from fixed 0.9
  if (shader_injection.dynCube_vanilla_history_weight < 0.8999f
      || shader_injection.dynCube_vanilla_history_weight > 0.9001f) return true;
  if (shader_injection.dynCube_vanilla_disoc_reject > 0.5f) return true;
  if (shader_injection.dynCube_vanilla_isfast > 0.5f && g_isfast_enabled > 0.5f) {
    auto* dev = cmd_list->get_device();
    auto* dd = dev ? dev->get_private_data<DeviceData>() : nullptr;
    if (dd && dd->isfast_noise_srv.handle) return true;
  }
  return false;
}
// ssr2: push everything the composite PS needs (game binds t0/t1/t2/s0/s1/b0/b2).
// No custom-SSR pushes: the composite resolves the vanilla march tap, never t31.
// Pushes when the composite serves or vanilla improvements are active;
// otherwise vanilla draws untouched.
static bool OnBeforeSoraSSR2Draw(reshade::api::command_list* cmd_list) {
  if (!SoraSSRReplaceActive(cmd_list) && !SoraSSRVanillaActive(cmd_list)) return true;
  auto* dev = cmd_list->get_device();
  if (!dev) return true;
  auto* dd = dev->get_private_data<DeviceData>();
  if (!dd) return true;
  reshade::api::resource_view t17srv = dd->dyncube_ggx_valid
      ? dd->dyncube_ggx_out_cube_srv[dd->dyncube_ggx_active]
      : dd->dyncube_srv;
  if (t17srv.handle) {
    cmd_list->push_descriptors(
        reshade::api::shader_stage::pixel,
        reshade::api::pipeline_layout{0}, 0,
        reshade::api::descriptor_table_update{
            {}, kDynCubeRegister, 0, 1,
            reshade::api::descriptor_type::texture_shader_resource_view,
            &t17srv});
  }
  if (dd->dyncube_hist[dd->dyncube_readSet].pos_cube_srv.handle) {
    auto histPosSrv = dd->dyncube_hist[dd->dyncube_readSet].pos_cube_srv;
    cmd_list->push_descriptors(
        reshade::api::shader_stage::pixel,
        reshade::api::pipeline_layout{0}, 0,
        reshade::api::descriptor_table_update{
            {}, kDynCubeHistPosRegister, 0, 1,
            reshade::api::descriptor_type::texture_shader_resource_view,
            &histPosSrv});
  }
  cmd_list->push_descriptors(
      reshade::api::shader_stage::pixel,
      reshade::api::pipeline_layout{0}, 0,
      reshade::api::descriptor_table_update{
          {}, kDynCubeVanillaRegister, 0, 1,
          reshade::api::descriptor_type::texture_shader_resource_view,
          &dd->captured_vanilla_env_srv});
  // t4 mrt normals: prefer the march's own t2 capture (same resource the vanilla
  // march decodes, so gate bits and resolution match by construction); fall back
  // to the lighting capture. First-live-wins: dead views keep game bindings.
  // Unbound Load returns 0, which safely gates out to the vanilla branch.
  reshade::api::resource_view mrtSrv = {};
  if (dd->captured_ssr_mrt_srv.handle && dd->captured_ssr_mrt_live) mrtSrv = dd->captured_ssr_mrt_srv;
  else if (dd->captured_mrt_normal_srv.handle && dd->captured_mrt_live) mrtSrv = dd->captured_mrt_normal_srv;
  if (mrtSrv.handle) {
    cmd_list->push_descriptors(
        reshade::api::shader_stage::pixel,
        reshade::api::pipeline_layout{0}, 0,
        reshade::api::descriptor_table_update{
            {}, 4u, 0, 1,
            reshade::api::descriptor_type::texture_shader_resource_view,
            &mrtSrv});
  }
  // Vanilla-ISFAST frame slice (exact frame_index % 64 mirror of the custom
  // march; -1 = noise unusable -> the shader falls back to hash behavior).
  // IS-FAST noise volume (t5) only when usable; the shader gates sampling on it.
  {
    const bool noiseUsable = g_isfast_enabled > 0.5f && dd->isfast_noise_srv.handle;
    shader_injection.dynCube_vanilla_isfast_frame = noiseUsable ? (float)(dd->frame_index % 64u) : -1.f;
    if (noiseUsable && shader_injection.dynCube_vanilla_isfast > 0.5f) {
      cmd_list->push_descriptors(
          reshade::api::shader_stage::pixel,
          reshade::api::pipeline_layout{0}, 0,
          reshade::api::descriptor_table_update{
              {}, 5u, 0, 1,
              reshade::api::descriptor_type::texture_shader_resource_view,
              &dd->isfast_noise_srv});
    }
  }
  return true;
}

// ssr2 code replacement gate: false keeps the vanilla shader (still draws).
// Serves the tree file for the replacement composite and for vanilla temporal
// improvements alike; the file selects its path in-shader.
static bool OnReplaceSoraSSR2Draw(reshade::api::command_list* cmd_list) {
  return SoraSSRReplaceActive(cmd_list) || SoraSSRVanillaActive(cmd_list);
}

// -- Sora1st SSR Replacement (fused march + temporal composite) --
// Master gate mirrors the Sora2nd one (shared toggles), minus any mrt capture
// requirement: the composite reads mrt0 from the game-bound t2, exactly like
// the vanilla pass it replaces, so no capture can starve serving.
static bool Sora1stSSRReplaceActive(reshade::api::command_list* cmd_list) {
  if (!cmd_list) return false;
  if (shader_injection.dynCube_ssr_replacement < 0.5f) return false;
  if (shader_injection.dynCube_enabled < 0.5f
      || shader_injection.dynCube_force_vanilla > 0.5f
      || shader_injection.dynCube_debug == 4.f) return false;
  auto* dev = cmd_list->get_device();
  if (!dev) return false;
  auto* dd = dev->get_private_data<DeviceData>();
  if (!dd) return false;
  if (!dd->captured_vanilla_env_srv.handle) return false;
  reshade::api::resource_view t17srv = dd->dyncube_ggx_valid
      ? dd->dyncube_ggx_out_cube_srv[dd->dyncube_ggx_active]
      : dd->dyncube_srv;
  if (!t17srv.handle) return false;
  return true;
}

// sora1st ssr vanilla-improvement gate: the tree file must execute (instead of
// game bytecode) whenever any Vanilla SSR Improvements toggle changes vanilla
// behavior � refine fix, motion-adaptive history, non-0.9 fixed weight,
// disocclusion reject, or usable IS-FAST distribution. All off = bytecode,
// bit-exact. (The replacement chain serves via Sora1stSSRReplaceActive.)
static bool Sora1stSSRVanillaActive(reshade::api::command_list* cmd_list) {
  if (!cmd_list) return false;
  if (shader_injection.dynCube_ssr_replacement > 0.5f) return false;
  if (shader_injection.dynCube_enabled < 0.5f
      || shader_injection.dynCube_force_vanilla > 0.5f
      || shader_injection.dynCube_debug == 4.f) return false;
  if (shader_injection.dynCube_vanilla_ssr_enabled < 0.5f) return false;
  if (shader_injection.dynCube_vanilla_refine_fix > 0.5f) return true;
  // NOTE (Sora1st direction differs from Sora2nd): vanilla Sora1st temporal is
  // already motion-adaptive, so FIXED mode is the deviation that needs serving.
  if (shader_injection.dynCube_vanilla_history_fixed > 0.5f) return true;
  if (shader_injection.dynCube_vanilla_disoc_reject > 0.5f) return true;
  if (shader_injection.dynCube_vanilla_isfast > 0.5f && g_isfast_enabled > 0.5f) {
    auto* dev = cmd_list->get_device();
    auto* dd = dev ? dev->get_private_data<DeviceData>() : nullptr;
    if (dd && dd->isfast_noise_srv.handle) return true;
  }
  return false;
}
// sora1st ssr: push everything the composite PS needs (game binds
// t0/t1/t2/t3/t4/s0/s1/b0/b2). No custom-SSR pushes: the composite resolves the
// inline vanilla march, never t31. Pushes when the composite serves or vanilla
// improvements are active; otherwise vanilla draws untouched.
static bool OnBeforeSora1stSSRDraw(reshade::api::command_list* cmd_list) {
  if (!Sora1stSSRReplaceActive(cmd_list) && !Sora1stSSRVanillaActive(cmd_list)) return true;
  auto* dev = cmd_list->get_device();
  if (!dev) return true;
  auto* dd = dev->get_private_data<DeviceData>();
  if (!dd) return true;
  reshade::api::resource_view t17srv = dd->dyncube_ggx_valid
      ? dd->dyncube_ggx_out_cube_srv[dd->dyncube_ggx_active]
      : dd->dyncube_srv;
  if (t17srv.handle) {
    cmd_list->push_descriptors(
        reshade::api::shader_stage::pixel,
        reshade::api::pipeline_layout{0}, 0,
        reshade::api::descriptor_table_update{
            {}, kDynCubeRegister, 0, 1,
            reshade::api::descriptor_type::texture_shader_resource_view,
            &t17srv});
  }
  if (dd->dyncube_hist[dd->dyncube_readSet].pos_cube_srv.handle) {
    auto histPosSrv = dd->dyncube_hist[dd->dyncube_readSet].pos_cube_srv;
    cmd_list->push_descriptors(
        reshade::api::shader_stage::pixel,
        reshade::api::pipeline_layout{0}, 0,
        reshade::api::descriptor_table_update{
            {}, kDynCubeHistPosRegister, 0, 1,
            reshade::api::descriptor_type::texture_shader_resource_view,
            &histPosSrv});
  }
  cmd_list->push_descriptors(
      reshade::api::shader_stage::pixel,
      reshade::api::pipeline_layout{0}, 0,
      reshade::api::descriptor_table_update{
          {}, kDynCubeVanillaRegister, 0, 1,
          reshade::api::descriptor_type::texture_shader_resource_view,
          &dd->captured_vanilla_env_srv});
  // Vanilla-ISFAST frame slice (exact frame_index % 64 mirror of the custom
  // march; -1 = noise unusable -> the shader falls back to hash behavior).
  // IS-FAST noise volume (t5) only when usable; the shader gates sampling on it.
  {
    const bool noiseUsable = g_isfast_enabled > 0.5f && dd->isfast_noise_srv.handle;
    shader_injection.dynCube_vanilla_isfast_frame = noiseUsable ? (float)(dd->frame_index % 64u) : -1.f;
    if (noiseUsable && shader_injection.dynCube_vanilla_isfast > 0.5f) {
      cmd_list->push_descriptors(
          reshade::api::shader_stage::pixel,
          reshade::api::pipeline_layout{0}, 0,
          reshade::api::descriptor_table_update{
              {}, 5u, 0, 1,
              reshade::api::descriptor_type::texture_shader_resource_view,
              &dd->isfast_noise_srv});
    }
  }
  return true;
}

// sora1st ssr code replacement gate: false keeps the vanilla shader (still draws).
// Serves the tree file for the replacement composite and for vanilla temporal
// improvements alike; the file selects its path in-shader.
static bool OnReplaceSora1stSSRDraw(reshade::api::command_list* cmd_list) {
  return Sora1stSSRReplaceActive(cmd_list) || Sora1stSSRVanillaActive(cmd_list);
}

// -- Custom TAA gate (Sora 1st/2nd) --
// ON: the from-scratch custom shader serves the TAA hash. OFF: this addon's
// vanilla reference serves the TAA hash when another addon has registered it;
// otherwise the game's original TAA draws untouched. The dumped vanilla files
// are reference only, never edited.
// History validity: the first custom frame after the OFF->ON transition must
// never accumulate against stale vanilla history, so on_draw reports invalid
// (shader outputs current only) and arms accumulation from the next frame.
// Resolution-change auto-reset is TAA-5 work; toggle transition is enough for
// TAA-0 A/B cleanliness.
static bool s_custom_taa_was_active = false;

static bool CustomTAAReplaceActive(reshade::api::command_list* cmd_list) {
  if (!cmd_list) return false;
  if (shader_injection.custom_taa_enabled < 0.5f) return false;
  if (!IsSora1st() && !IsSora2nd()) return false;
  return true;
}

static bool OnReplaceCustomTAADraw(reshade::api::command_list* cmd_list) {
  if (!cmd_list) return false;
  if (CustomTAAReplaceActive(cmd_list)) return true;
  // When another addon has registered the same TAA hash, keep forcing this
  // addon's selected payload (vanilla when Custom TAA is off). Otherwise leave
  // the game's original pipeline untouched.
  if (!IsSora1st() && !IsSora2nd()) return false;
  auto* dev = cmd_list->get_device();
  auto* d = (dev != nullptr) ? dev->get_private_data<DeviceData>() : nullptr;
  return d != nullptr && d->taa_other_addon_active;
}

// Compares replacement payloads before rewriting the shared slot. Transition
// cost comes only from changed payloads: AddRuntimeReplacement invalidates
// cached replacement pipelines.
static bool TAASpanEquals(std::span<const uint8_t> a, std::span<const uint8_t> b) {
  return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin());
}

// Persistently owns a TAA replacement slot for the device lifetime. The
// caller supplies both this addon's custom and vanilla payloads; the slot
// holds whichever payload is desired. Only a changed payload rewrites the
// shared map and resets cached replacement pipelines.
static void EnsureTAAPayload(
    reshade::api::device* dev,
    uint32_t hash,
    std::span<const uint8_t> desired,
    std::span<const uint8_t> alternate) {
  if (!dev || hash == 0u || desired.empty()) return;
  auto* d = dev->get_private_data<DeviceData>();
  if (!d) return;
  if (renodx::utils::shader::shared.data == nullptr) return;  // shared state unavailable: keep today's behavior
  bool present = false;
  bool matches_desired = false;
  bool matches_alternate = false;
  renodx::utils::shader::shared.data->runtime_replacements.if_contains(
      std::pair<reshade::api::device*, uint32_t>{dev, hash},
      [&](const auto& pair) {
        present = true;
        matches_desired = TAASpanEquals(pair.second, desired);
        matches_alternate = !alternate.empty() && TAASpanEquals(pair.second, alternate);
      });
  // A nonempty payload different from both known payloads means another addon
  // has registered this TAA hash. Remember that so replacement stays forced
  // even when Custom TAA is off.
  if (present && !matches_desired && !matches_alternate) {
    d->taa_other_addon_active = true;
  }
  if (matches_desired) {
    d->taa_slot_claimed = true;
    d->taa_claimed_hash = hash;
    d->taa_claimed_code = desired;
    return;
  }
  renodx::utils::shader::AddRuntimeReplacement(dev, hash, desired);
  ResetTAAReplacementPipelines(dev, hash);
  d->taa_slot_claimed = true;
  d->taa_claimed_hash = hash;
  d->taa_claimed_code = desired;
}

static bool OnBeforeCustomTAADraw(reshade::api::command_list* cmd_list) {
  // Cross-addon slot ownership: runs before on_replace/ApplyReplacement in the
  // same handler invocation, so the served bytecode follows the toggle. OFF
  // selects this addon's vanilla reference bytecode, never a friend payload.
  if (cmd_list) {
    if (auto* dev = cmd_list->get_device()) {
      const bool custom_taa_on = CustomTAAReplaceActive(cmd_list);
      if (IsSora1st()) {
        EnsureTAAPayload(
            dev,
            0xFA37EA04u,
            custom_taa_on ? __taa_custom_sora1st : __0xFA37EA04,
            custom_taa_on ? __0xFA37EA04 : __taa_custom_sora1st);
      } else if (IsSora2nd()) {
        EnsureTAAPayload(
            dev,
            0x9D91FAC3u,
            custom_taa_on ? __taa_custom_sora2nd : __0x9D91FAC3,
            custom_taa_on ? __0x9D91FAC3 : __taa_custom_sora2nd);
      }
      // Pipelines created after the slot stabilized (level loads, resolution
      // changes) were built from stale bytes: reset once on the next draw.
      if (auto* dd = dev->get_private_data<DeviceData>()) {
        if (!dd->taa_pending_resets.empty()) {
          ResetTAAReplacementPipelines(
              dev, IsSora1st() ? 0xFA37EA04u : (IsSora2nd() ? 0x9D91FAC3u : 0u));
          dd->taa_pending_resets.clear();
        }
      }
    }
  }
  if (!CustomTAAReplaceActive(cmd_list)) {
    s_custom_taa_was_active = false;
  } else {
    shader_injection.custom_taa_history_valid = s_custom_taa_was_active ? 1.f : 0.f;
    s_custom_taa_was_active = true;
  }
  // -- RCAS history isolation: serve the owned UNSHARPENED copy as t2 --
  // Runs only while Custom TAA is on (same gate as the stage itself). The
  // sharpened temp/RT is never bound here; only rcas_hist_srv is.
  if (shader_injection.custom_taa_enabled > 0.5f && (IsSora1st() || IsSora2nd())) {
    if (auto* dev = cmd_list->get_device()) {
      if (auto* dd = dev->get_private_data<DeviceData>()) {
        if (dd->rcas_have_copy && dd->rcas_hist_srv.handle) {
          reshade::api::resource_view srv = dd->rcas_hist_srv;
          cmd_list->push_descriptors(
              reshade::api::shader_stage::pixel,
              reshade::api::pipeline_layout{0}, 0,
              reshade::api::descriptor_table_update{
                  {}, 2u, 0, 1,
                  reshade::api::descriptor_type::texture_shader_resource_view,
                  &srv,
              });
        }
      }
    }
  }
  return true;
}

// ----------- RCAS post-TAA sharpening � implementation (Stage 1) -----------
// Per-frame flow (all inline, same command list, zero added latency):
//  TAA draw completes (on_drawn) -> copy unsharpened TAA output to rcas_hist
//  -> RCAS compute reads rcas_hist (owned, never RTV-bound: no D3D11
//  SRV<->RTV binding hazard), writes rcas_temp -> copy rcas_temp back
//  over the TAA output so downstream reads see sharpened pixels.
// Next frame's TAA draw gets t2 overridden to rcas_hist_srv above, so TAA
// history is always unsharpened. Sharpened data can never enter history:
// rcas_temp/rcas_temp_srv are never bound as t2 anywhere in this file.

static void DestroyRCASSet(reshade::api::device* dev, DeviceData* d) {
  if (!dev || !d) return;
  auto dv = [&](reshade::api::resource_view& v) { if (v.handle) { dev->destroy_resource_view(v); v = {}; } };
  auto dr = [&](reshade::api::resource& r) { if (r.handle) { dev->destroy_resource(r); r = {}; } };
  dv(d->rcas_temp_srv); dv(d->rcas_temp_uav); dr(d->rcas_temp_texture);
  dv(d->rcas_hist_srv); dr(d->rcas_hist_texture);
  d->rcas_w = 0u; d->rcas_h = 0u;
  d->rcas_fmt = reshade::api::format::unknown;
  d->rcas_have_copy = false;
  d->rcas_format_logged = false;
  d->rcas_dispatch_logged = false;
}

static void DestroyRCASResources(reshade::api::device* dev, DeviceData* d) {
  if (!dev || !d) return;
  DestroyRCASSet(dev, d);
  for (auto& t : d->rcas_tables) { if (t.handle) { dev->free_descriptor_table(t); t = {}; } }
  if (d->rcas_pipeline.handle) { dev->destroy_pipeline(d->rcas_pipeline); d->rcas_pipeline = {}; }
  if (d->rcas_layout.handle) { dev->destroy_pipeline_layout(d->rcas_layout); d->rcas_layout = {}; }
  d->rcas_layout_version = 0u;
}

static void CreateRCASResources(reshade::api::device* dev, DeviceData* d,
                                uint32_t w, uint32_t h, reshade::api::format fmt) {
  DestroyRCASSet(dev, d);
  if (!dev || !d || w == 0u || h == 0u || fmt == reshade::api::format::unknown) return;
  auto mk = [&](reshade::api::format vfmt,
                reshade::api::resource* res, reshade::api::resource_view* srv,
                reshade::api::resource_view* uav) {
    reshade::api::resource_desc rd = {};
    rd.type = reshade::api::resource_type::texture_2d;
    rd.texture = {w, h, 1, 1, vfmt, 1};
    rd.heap = reshade::api::memory_heap::gpu_only;
    rd.usage = reshade::api::resource_usage::shader_resource | reshade::api::resource_usage::unordered_access;
    if (!dev->create_resource(rd, nullptr, reshade::api::resource_usage::shader_resource, res)) return;
    reshade::api::resource_view_desc vd(reshade::api::resource_view_type::texture_2d, vfmt, 0, 1, 0, 1);
    if (srv && !dev->create_resource_view(*res, reshade::api::resource_usage::shader_resource, vd, srv)) return;
    if (uav) dev->create_resource_view(*res, reshade::api::resource_usage::unordered_access, vd, uav);
  };
  // Temp uses the linear (UAV-legal) variant; hist keeps the verbatim game
  // format so RT<->hist copies stay bitwise and t2 decodes exactly as before.
  mk(RCASLinearFormat(fmt), &d->rcas_temp_texture, &d->rcas_temp_srv, &d->rcas_temp_uav);
  mk(fmt, &d->rcas_hist_texture, &d->rcas_hist_srv, nullptr);
  if (!d->rcas_temp_texture.handle || !d->rcas_temp_uav.handle || !d->rcas_hist_texture.handle) {
    DestroyRCASSet(dev, d);
    return;
  }
  d->rcas_w = w; d->rcas_h = h; d->rcas_fmt = fmt;
}

static bool CreateRCASPipelineIfNeeded(reshade::api::device* dev, DeviceData* d) {
  using DR = reshade::api::descriptor_range;
  using DS = reshade::api::shader_stage;
  using DT = reshade::api::descriptor_type;
  using P = reshade::api::pipeline_layout_param;
  if (!dev || !d) return false;
  auto mkcs = [&](std::span<const uint8_t> bc, reshade::api::pipeline_layout lo, reshade::api::pipeline* out) -> bool {
    if (bc.empty() || !lo.handle) return false;
    if (out->handle != 0u) return true;
    reshade::api::shader_desc sd = {};
    sd.code = bc.data(); sd.code_size = bc.size(); sd.entry_point = "main";
    reshade::api::pipeline_subobject so = {reshade::api::pipeline_subobject_type::compute_shader, 1, &sd};
    return dev->create_pipeline(lo, 1, &so, out);
  };
  if (d->rcas_layout.handle == 0u || d->rcas_layout_version != kRCASLayoutVersion) {
    if (d->rcas_layout.handle) {
      for (auto& t : d->rcas_tables) { if (t.handle) { dev->free_descriptor_table(t); t = {}; } }
      dev->destroy_pipeline_layout(d->rcas_layout); d->rcas_layout = {};
      if (d->rcas_pipeline.handle) { dev->destroy_pipeline(d->rcas_pipeline); d->rcas_pipeline = {}; }
    }
    DR srv_r = {0,0,0,2,DS::all_compute,1,DT::texture_shader_resource_view};  // t0 unsharpened copy, t1 motion buffer
    DR uav_r = {0,0,0,1,DS::all_compute,1,DT::texture_unordered_access_view}; // u0 sharpened temp
    reshade::api::constant_range push_range = {};
    push_range.binding = 0;
    push_range.dx_register_index = 13;
    push_range.dx_register_space = 0;
    push_range.count = 11;  // con, width, height, denoise, motionOn, threshold, range, response, mult, debug, max
    push_range.visibility = DS::all_compute;
    P p0, p1, pPush;
    p0.type = reshade::api::pipeline_layout_param_type::descriptor_table; p0.descriptor_table.count = 1; p0.descriptor_table.ranges = &srv_r;
    p1.type = reshade::api::pipeline_layout_param_type::descriptor_table; p1.descriptor_table.count = 1; p1.descriptor_table.ranges = &uav_r;
    pPush.type = reshade::api::pipeline_layout_param_type::push_constants; pPush.push_constants = push_range;
    P params[3] = {p0,p1,pPush};
    if (!dev->create_pipeline_layout(3, params, &d->rcas_layout)) return false;
    d->rcas_layout_version = kRCASLayoutVersion;
  }
  for (uint32_t i = 0; i < 2; ++i) {
    if (d->rcas_tables[i].handle == 0u) {
      if (!dev->allocate_descriptor_table(d->rcas_layout, i, &d->rcas_tables[i])) return false;
    }
  }
#ifdef __RCASSharpenCS_EMBED_FILE
  if (!__RCASSharpenCS.empty()) {
    if (!mkcs(__RCASSharpenCS, d->rcas_layout, &d->rcas_pipeline)) {
      CSLog("rcas", "pipeline create failed", true);
      return false;
    }
  }
#endif
  return d->rcas_pipeline.handle != 0u;
}

// -- FXAA post-TAA (TAA -> FXAA -> RCAS; never feeds TAA history) --
// Owns fxaa_temp (FXAA output, linear/UAV-legal variant format so RCAS sees
// the same raw convention as before) + fxaa_luma (R8 perceptual luma) +
// fxaa_hist_srv (linear-variant SRV view of the RCAS-owned hist texture, so
// SampleLevel never sRGB-decodes; falls back to rcas_hist_srv if the
// reinterpreted view fails). The hist texture is RCAS-owned and may be
// recreated at any time: fxaa_hist_seen tracks the handle the views were
// built from and forces a rebuild on change.
static void DestroyFXAASet(reshade::api::device* dev, DeviceData* d) {
  if (!dev || !d) return;
  auto dv = [&](reshade::api::resource_view& v) { if (v.handle) { dev->destroy_resource_view(v); v = {}; } };
  auto dr = [&](reshade::api::resource& r) { if (r.handle) { dev->destroy_resource(r); r = {}; } };
  dv(d->fxaa_temp_srv); dv(d->fxaa_temp_uav); dr(d->fxaa_temp_texture);
  dv(d->fxaa_luma_srv); dv(d->fxaa_luma_uav); dr(d->fxaa_luma_texture);
  if (d->fxaa_hist_srv_owned) dv(d->fxaa_hist_srv);
  d->fxaa_hist_srv = {};
  d->fxaa_hist_srv_owned = false;
  d->fxaa_hist_seen = {};
  d->fxaa_w = 0u; d->fxaa_h = 0u;
  d->fxaa_fmt = reshade::api::format::unknown;
  d->fxaa_logged = false;
}

static void DestroyFXAAResources(reshade::api::device* dev, DeviceData* d) {
  if (!dev || !d) return;
  DestroyFXAASet(dev, d);
  for (auto& t : d->fxaa_tables) { if (t.handle) { dev->free_descriptor_table(t); t = {}; } }
  if (d->fxaa_luma_pipeline.handle) { dev->destroy_pipeline(d->fxaa_luma_pipeline); d->fxaa_luma_pipeline = {}; }
  if (d->fxaa_standard_pipeline.handle) { dev->destroy_pipeline(d->fxaa_standard_pipeline); d->fxaa_standard_pipeline = {}; }
  if (d->fxaa_high_pipeline.handle) { dev->destroy_pipeline(d->fxaa_high_pipeline); d->fxaa_high_pipeline = {}; }
  if (d->fxaa_layout.handle) { dev->destroy_pipeline_layout(d->fxaa_layout); d->fxaa_layout = {}; }
  d->fxaa_layout_version = 0u;
}

static void CreateFXAAResources(reshade::api::device* dev, DeviceData* d,
                                uint32_t w, uint32_t h, reshade::api::format fmt) {
  DestroyFXAASet(dev, d);
  if (!dev || !d || w == 0u || h == 0u || fmt == reshade::api::format::unknown) return;
  if (!d->rcas_hist_texture.handle) return;  // hist view needs the RCAS-owned texture
  auto mk = [&](reshade::api::format vfmt,
                reshade::api::resource* res, reshade::api::resource_view* srv,
                reshade::api::resource_view* uav) {
    reshade::api::resource_desc rd = {};
    rd.type = reshade::api::resource_type::texture_2d;
    rd.texture = {w, h, 1, 1, vfmt, 1};
    rd.heap = reshade::api::memory_heap::gpu_only;
    rd.usage = reshade::api::resource_usage::shader_resource | reshade::api::resource_usage::unordered_access;
    if (!dev->create_resource(rd, nullptr, reshade::api::resource_usage::shader_resource, res)) return;
    reshade::api::resource_view_desc vd(reshade::api::resource_view_type::texture_2d, vfmt, 0, 1, 0, 1);
    if (srv && !dev->create_resource_view(*res, reshade::api::resource_usage::shader_resource, vd, srv)) return;
    if (uav) dev->create_resource_view(*res, reshade::api::resource_usage::unordered_access, vd, uav);
  };
  mk(RCASLinearFormat(fmt), &d->fxaa_temp_texture, &d->fxaa_temp_srv, &d->fxaa_temp_uav);
  mk(reshade::api::format::r8_unorm, &d->fxaa_luma_texture, &d->fxaa_luma_srv, &d->fxaa_luma_uav);
  // Linear-variant view of the hist texture (same bits, no sRGB decode).
  // Non-typeless sRGB resources reject reinterpretation: fall back to the
  // game-format view (hardware decode on sample; luma mode 0 still applies).
  reshade::api::resource_view_desc hvd(reshade::api::resource_view_type::texture_2d,
                                       RCASLinearFormat(fmt), 0, 1, 0, 1);
  if (!dev->create_resource_view(d->rcas_hist_texture, reshade::api::resource_usage::shader_resource,
                                 hvd, &d->fxaa_hist_srv)) {
    d->fxaa_hist_srv = d->rcas_hist_srv;
    d->fxaa_hist_srv_owned = false;
  } else {
    d->fxaa_hist_srv_owned = true;
  }
  if (!d->fxaa_temp_texture.handle || !d->fxaa_temp_uav.handle
      || !d->fxaa_luma_texture.handle || !d->fxaa_luma_uav.handle
      || !d->fxaa_hist_srv.handle) {
    DestroyFXAASet(dev, d);
    return;
  }
  d->fxaa_w = w; d->fxaa_h = h; d->fxaa_fmt = fmt;
  d->fxaa_hist_seen = d->rcas_hist_texture;
}

static bool CreateFXAAPipelineIfNeeded(reshade::api::device* dev, DeviceData* d) {
  using DR = reshade::api::descriptor_range;
  using DS = reshade::api::shader_stage;
  using DT = reshade::api::descriptor_type;
  using P = reshade::api::pipeline_layout_param;
  if (!dev || !d) return false;
  auto mkcs = [&](std::span<const uint8_t> bc, reshade::api::pipeline_layout lo, reshade::api::pipeline* out) -> bool {
    if (bc.empty() || !lo.handle) return false;
    if (out->handle != 0u) return true;
    reshade::api::shader_desc sd = {};
    sd.code = bc.data(); sd.code_size = bc.size(); sd.entry_point = "main";
    reshade::api::pipeline_subobject so = {reshade::api::pipeline_subobject_type::compute_shader, 1, &sd};
    return dev->create_pipeline(lo, 1, &so, out);
  };
  if (d->fxaa_layout.handle == 0u || d->fxaa_layout_version != kFXAALayoutVersion) {
    if (d->fxaa_layout.handle) {
      for (auto& t : d->fxaa_tables) { if (t.handle) { dev->free_descriptor_table(t); t = {}; } }
      dev->destroy_pipeline_layout(d->fxaa_layout); d->fxaa_layout = {};
      if (d->fxaa_luma_pipeline.handle) { dev->destroy_pipeline(d->fxaa_luma_pipeline); d->fxaa_luma_pipeline = {}; }
      if (d->fxaa_standard_pipeline.handle) { dev->destroy_pipeline(d->fxaa_standard_pipeline); d->fxaa_standard_pipeline = {}; }
      if (d->fxaa_high_pipeline.handle) { dev->destroy_pipeline(d->fxaa_high_pipeline); d->fxaa_high_pipeline = {}; }
    }
    DR srv_r = {0,0,0,2,DS::all_compute,1,DT::texture_shader_resource_view};  // t0 color copy, t1 luma
    DR uav_r = {0,0,0,1,DS::all_compute,1,DT::texture_unordered_access_view}; // u0 target
    reshade::api::constant_range push_range = {};
    push_range.binding = 0;
    push_range.dx_register_index = 13;
    push_range.dx_register_space = 0;
    push_range.count = 6;  // width, height, subpix, edgeThreshold, edgeThresholdMin, lumaMode
    push_range.visibility = DS::all_compute;
    P p0, p1, pPush;
    p0.type = reshade::api::pipeline_layout_param_type::descriptor_table; p0.descriptor_table.count = 1; p0.descriptor_table.ranges = &srv_r;
    p1.type = reshade::api::pipeline_layout_param_type::descriptor_table; p1.descriptor_table.count = 1; p1.descriptor_table.ranges = &uav_r;
    pPush.type = reshade::api::pipeline_layout_param_type::push_constants; pPush.push_constants = push_range;
    P params[3] = {p0,p1,pPush};
    if (!dev->create_pipeline_layout(3, params, &d->fxaa_layout)) return false;
    d->fxaa_layout_version = kFXAALayoutVersion;
  }
  for (uint32_t i = 0; i < 2; ++i) {
    if (d->fxaa_tables[i].handle == 0u) {
      if (!dev->allocate_descriptor_table(d->fxaa_layout, i, &d->fxaa_tables[i])) return false;
    }
  }
#ifdef __FXAALuma_EMBED_FILE
  if (!__FXAALuma.empty()) {
    if (!mkcs(__FXAALuma, d->fxaa_layout, &d->fxaa_luma_pipeline)) {
      CSLog("fxaa", "luma pipeline create failed", true);
      return false;
    }
  }
#endif
#ifdef __FXAAStandard_EMBED_FILE
  if (!__FXAAStandard.empty()) {
    if (!mkcs(__FXAAStandard, d->fxaa_layout, &d->fxaa_standard_pipeline)) {
      CSLog("fxaa", "standard pipeline create failed", true);
      return false;
    }
  }
#endif
#ifdef __FXAAHigh_EMBED_FILE
  if (!__FXAAHigh.empty()) {
    if (!mkcs(__FXAAHigh, d->fxaa_layout, &d->fxaa_high_pipeline)) {
      CSLog("fxaa", "high pipeline create failed", true);
      return false;
    }
  }
#endif
  return d->fxaa_luma_pipeline.handle != 0u
      && d->fxaa_standard_pipeline.handle != 0u
      && d->fxaa_high_pipeline.handle != 0u;
}

// Runs inline on the TAA draw's own command list, immediately after the draw
// completes: no added frame latency, strict ordering vs downstream readers.
static void OnDrawnCustomTAA(reshade::api::command_list* cmd_list) {
  // RCAS runs only while Custom TAA is on (it sharpens the custom TAA output
  // specifically). Strength 0 is the off position (passthrough below).
  if (!cmd_list) return;
  if (shader_injection.custom_taa_enabled < 0.5f) return;
  if (!IsSora1st() && !IsSora2nd()) return;
  auto* dev = cmd_list->get_device();
  if (!dev) return;
  auto* d = dev->get_private_data<DeviceData>();
  if (!d) return;
  reshade::api::resource_view rtv = d->rcas_last_rtv0;
  if (!rtv.handle) return;
  reshade::api::resource srcRes = dev->get_resource_from_view(rtv);
  if (!srcRes.handle) return;
  auto desc = dev->get_resource_desc(srcRes);
  if (desc.type != reshade::api::resource_type::texture_2d) return;
  if (desc.texture.samples != 1) {  // multisampled targets cannot be shader-loaded; skip
    if (!d->rcas_format_logged) {
      reshade::log::message(reshade::log::level::warning, "[RCAS] skip: multisampled TAA target");
      d->rcas_format_logged = true;
    }
    return;
  }
  uint32_t w = desc.texture.width, h = desc.texture.height;
  auto fmt = desc.texture.format;
  if (w == 0u || h == 0u || fmt == reshade::api::format::unknown) return;
  // Pipeline first: no barrier/copy may be issued unless the dispatch below
  // is guaranteed to run. (Previously the pipeline check sat after the first
  // barriers � a failure there could leave the target in a wrong state.)
  if (!CreateRCASPipelineIfNeeded(dev, d)) {
    if (!d->rcas_format_logged) {
      reshade::log::message(reshade::log::level::warning, "[RCAS] skip: pipeline unavailable");
      d->rcas_format_logged = true;
    }
    return;
  }
  // (Re)create the owned set when the TAA target changes shape. Game format is
  // preserved for hist/copies; temp uses the linear variant (UAV-legal).
  if (!d->rcas_temp_texture.handle || !d->rcas_temp_uav.handle
      || d->rcas_w != w || d->rcas_h != h || d->rcas_fmt != fmt) {
    CreateRCASResources(dev, d, w, h, fmt);
    if (!d->rcas_temp_texture.handle || !d->rcas_temp_uav.handle) {
      if (!d->rcas_format_logged) {
        reshade::log::message(reshade::log::level::warning, "[RCAS] skip: owned target creation failed");
        d->rcas_format_logged = true;
      }
      return;
    }
    if (!d->rcas_format_logged) {
      reshade::log::message(reshade::log::level::info,
        (std::string("[RCAS] target ") + std::to_string(w) + "x" + std::to_string(h)
          + " fmt=" + std::to_string((int)fmt)
          + " work=" + std::to_string((int)RCASLinearFormat(fmt))).c_str());
      d->rcas_format_logged = true;
    }
  }
  // Dispatch input is the owned UNSHARPENED copy (rcas_hist_srv, game format).

  // 1) Save the UNSHARPENED output for next frame's t2 BEFORE sharpening.
  cmd_list->barrier(srcRes, reshade::api::resource_usage::render_target,
                    reshade::api::resource_usage::copy_source);
  cmd_list->barrier(d->rcas_hist_texture, reshade::api::resource_usage::shader_resource,
                    reshade::api::resource_usage::copy_dest);
  cmd_list->copy_texture_region(srcRes, 0, nullptr, d->rcas_hist_texture, 0, nullptr);
  cmd_list->barrier(d->rcas_hist_texture, reshade::api::resource_usage::copy_dest,
                    reshade::api::resource_usage::shader_resource);
  cmd_list->barrier(srcRes, reshade::api::resource_usage::copy_source,
                    reshade::api::resource_usage::shader_resource);
  d->rcas_have_copy = true;
  // 1b) FXAA (optional, default on): luma prepass reads the unsharpened copy
  // (linear-variant view: raw convention, no sRGB decode), main pass writes
  // fxaa_temp. rcas_hist is never touched here, so TAA history stays clean.
  // Off (or any setup failure) leaves fxaa_active false and every downstream
  // reader uses rcas_hist exactly as before: zero behavior change.
  bool fxaa_active = false;
  if (shader_injection.fxaa_enabled > 0.5f) {
    if (CreateFXAAPipelineIfNeeded(dev, d)) {
      if (!d->fxaa_temp_texture.handle || !d->fxaa_temp_uav.handle
          || !d->fxaa_luma_texture.handle || !d->fxaa_luma_uav.handle
          || !d->fxaa_hist_srv.handle || d->fxaa_w != w || d->fxaa_h != h
          || d->fxaa_fmt != fmt || d->fxaa_hist_seen.handle != d->rcas_hist_texture.handle) {
        CreateFXAAResources(dev, d, w, h, fmt);
      }
      if (d->fxaa_temp_uav.handle && d->fxaa_luma_uav.handle && d->fxaa_hist_srv.handle) {
        using F = reshade::api::format;
        const bool hdr = (fmt == F::r16g16b16a16_float) || (fmt == F::r11g11b10_float)
            || (fmt == F::r32g32b32a32_float);
        float pc[6] = {(float)w, (float)h,
                       std::clamp(shader_injection.fxaa_subpix, 0.f, 1.f),
                       std::clamp(shader_injection.fxaa_edge_threshold, 0.031f, 0.333f),
                       std::clamp(shader_injection.fxaa_edge_threshold_min, 0.f, 0.0833f),
                       hdr ? 1.f : 0.f};
        // Luma prepass: hist(SRV) -> luma(UAV).
        cmd_list->barrier(d->fxaa_luma_texture, reshade::api::resource_usage::shader_resource,
                          reshade::api::resource_usage::unordered_access);
        cmd_list->bind_pipeline(reshade::api::pipeline_stage::all_compute, d->fxaa_luma_pipeline);
        reshade::api::descriptor_table_update luma_ups[2];
        reshade::api::resource_view luma_srvs[2] = {d->fxaa_hist_srv, d->fallback_srv};
        luma_ups[0] = {d->fxaa_tables[0], 0, 0, 2, reshade::api::descriptor_type::texture_shader_resource_view, luma_srvs};
        luma_ups[1] = {d->fxaa_tables[1], 0, 0, 1, reshade::api::descriptor_type::texture_unordered_access_view, &d->fxaa_luma_uav};
        dev->update_descriptor_tables(2, luma_ups);
        std::array<reshade::api::descriptor_table, 2> luma_tables = {d->fxaa_tables[0], d->fxaa_tables[1]};
        cmd_list->bind_descriptor_tables(reshade::api::shader_stage::all_compute, d->fxaa_layout, 0, 2, luma_tables.data());
        cmd_list->push_constants(reshade::api::shader_stage::all_compute, d->fxaa_layout, 2, 0, 6, pc);
        cmd_list->dispatch((w + 7u) / 8u, (h + 7u) / 8u, 1u);
        cmd_list->barrier(d->fxaa_luma_texture, reshade::api::resource_usage::unordered_access,
                          reshade::api::resource_usage::shader_resource);
        // Main pass: hist + luma(SRV) -> temp(UAV); temp ends in SRV state for RCAS/publish.
        cmd_list->barrier(d->fxaa_temp_texture, reshade::api::resource_usage::shader_resource,
                          reshade::api::resource_usage::unordered_access);
        cmd_list->bind_pipeline(reshade::api::pipeline_stage::all_compute,
                                (shader_injection.fxaa_quality > 0.5f) ? d->fxaa_high_pipeline : d->fxaa_standard_pipeline);
        reshade::api::descriptor_table_update main_ups[2];
        reshade::api::resource_view main_srvs[2] = {d->fxaa_hist_srv, d->fxaa_luma_srv};
        main_ups[0] = {d->fxaa_tables[0], 0, 0, 2, reshade::api::descriptor_type::texture_shader_resource_view, main_srvs};
        main_ups[1] = {d->fxaa_tables[1], 0, 0, 1, reshade::api::descriptor_type::texture_unordered_access_view, &d->fxaa_temp_uav};
        dev->update_descriptor_tables(2, main_ups);
        std::array<reshade::api::descriptor_table, 2> main_tables = {d->fxaa_tables[0], d->fxaa_tables[1]};
        cmd_list->bind_descriptor_tables(reshade::api::shader_stage::all_compute, d->fxaa_layout, 0, 2, main_tables.data());
        cmd_list->push_constants(reshade::api::shader_stage::all_compute, d->fxaa_layout, 2, 0, 6, pc);
        cmd_list->dispatch((w + 7u) / 8u, (h + 7u) / 8u, 1u);
        cmd_list->barrier(d->fxaa_temp_texture, reshade::api::resource_usage::unordered_access,
                          reshade::api::resource_usage::shader_resource);
        fxaa_active = true;
        if (!d->fxaa_logged) {
          reshade::log::message(reshade::log::level::info,
            (std::string("[FXAA] dispatch groups=") + std::to_string((w + 7u) / 8u) + "x" + std::to_string((h + 7u) / 8u)
              + " quality=" + (shader_injection.fxaa_quality > 0.5f ? "high" : "standard")
              + " luma=" + (hdr ? "hdr" : "ldr")).c_str());
          d->fxaa_logged = true;
        }
      } else if (!d->fxaa_logged) {
        reshade::log::message(reshade::log::level::warning, "[FXAA] skip: owned target creation failed");
        d->fxaa_logged = true;
      }
    } else if (!d->fxaa_logged) {
      reshade::log::message(reshade::log::level::warning, "[FXAA] skip: pipeline unavailable");
      d->fxaa_logged = true;
    }
  }
  // Enable toggle forces 0 sharpening (stage machinery above runs identically).
  // Strength 0 is the off position: unsharpened output stays in place.
  // Dispatch runs whenever base sharpening or motion sharpening could apply;
  // base 0 with motion on + live motion view still dispatches (motion value
  // comes from base x multiplier, so it is 0 there by arithmetic, exactly).
  float effStrength = (shader_injection.rcas_enabled > 0.5f) ? shader_injection.rcas_strength : 0.f;
  bool motionCouldApply = (shader_injection.rcas_enabled > 0.5f)
      && (shader_injection.rcas_motion_on > 0.5f)
      && d->rcas_motion_srv.handle && d->rcas_motion_live.load();
  if (effStrength <= 0.0005f && !motionCouldApply) {
    if (!fxaa_active) return;
    // RCAS skipped: publish the FXAA result over the TAA output so downstream
    // reads see FXAA'd pixels. History already saved un-FXAA'd in (1).
    cmd_list->barrier(d->fxaa_temp_texture, reshade::api::resource_usage::shader_resource,
                      reshade::api::resource_usage::copy_source);
    cmd_list->barrier(srcRes, reshade::api::resource_usage::shader_resource,
                      reshade::api::resource_usage::copy_dest);
    cmd_list->copy_texture_region(d->fxaa_temp_texture, 0, nullptr, srcRes, 0, nullptr);
    cmd_list->barrier(srcRes, reshade::api::resource_usage::copy_dest,
                      reshade::api::resource_usage::render_target);
    cmd_list->barrier(d->fxaa_temp_texture, reshade::api::resource_usage::copy_source,
                      reshade::api::resource_usage::shader_resource);
    return;
  }

  // 2) RCAS compute: reads the owned unsharpened copy (t0) + motion (t1),
  // writes temp (u0). t0 is the FXAA output when FXAA served, else the hist
  // copy exactly as before. Motion view falls back to the 1x1 white fallback
  // (never sampled: availability is folded into motionOn below).
  reshade::api::resource_view motionSrv =
      (d->rcas_motion_srv.handle && d->rcas_motion_live.load()) ? d->rcas_motion_srv : d->fallback_srv;
  cmd_list->bind_pipeline(reshade::api::pipeline_stage::all_compute, d->rcas_pipeline);
  reshade::api::descriptor_table_update ups[2];
  reshade::api::resource_view srvs[2] = {fxaa_active ? d->fxaa_temp_srv : d->rcas_hist_srv, motionSrv};
  ups[0] = {d->rcas_tables[0], 0, 0, 2, reshade::api::descriptor_type::texture_shader_resource_view, srvs};
  ups[1] = {d->rcas_tables[1], 0, 0, 1, reshade::api::descriptor_type::texture_unordered_access_view, &d->rcas_temp_uav};
  dev->update_descriptor_tables(2, ups);
  std::array<reshade::api::descriptor_table, 2> tables = {d->rcas_tables[0], d->rcas_tables[1]};
  cmd_list->bind_descriptor_tables(reshade::api::shader_stage::all_compute, d->rcas_layout, 0, 2, tables.data());
  {
    // Linear response: con = S directly (visible effect is ~linear in con
    // pre-clamp). S=1 matches the old mapping's maximum exactly.
    // Motion sharpening interpolates base -> min(base x multiplier, max) per
    // pixel; Off (or dead/missing motion view) uses base con exactly.
    float s = std::clamp(effStrength, 0.f, 1.f);
    float dn = (shader_injection.rcas_denoise > 0.5f) ? 1.f : 0.f;
    bool motionOk = (shader_injection.rcas_motion_on > 0.5f)
        && d->rcas_motion_srv.handle && d->rcas_motion_live.load();
    float pc[11] = {s, (float)w, (float)h, dn,
                   motionOk ? 1.f : 0.f,
                   std::max(0.f, shader_injection.rcas_motion_threshold),
                   std::max(0.25f, shader_injection.rcas_motion_range),
                   std::clamp(shader_injection.rcas_motion_response, 0.5f, 3.f),
                   std::clamp(shader_injection.rcas_motion_multiplier, 1.f, 10.f),
                   shader_injection.rcas_debug > 0.5f ? 1.f : 0.f,
                   std::clamp(shader_injection.rcas_motion_max, 0.f, 3.f)};
    cmd_list->push_constants(reshade::api::shader_stage::all_compute, d->rcas_layout, 2, 0, 11, pc);
    if (!d->rcas_dispatch_logged) {
      reshade::log::message(reshade::log::level::info,
        (std::string("[RCAS] dispatch groups=") + std::to_string((w + 7u) / 8u) + "x" + std::to_string((h + 7u) / 8u)
          + " con=" + std::to_string(pc[0])
          + " tables=" + std::to_string(d->rcas_tables[0].handle) + "/" + std::to_string(d->rcas_tables[1].handle)
          + " src=" + std::to_string(srcRes.handle)).c_str());
      d->rcas_dispatch_logged = true;
    }
  }
  cmd_list->dispatch((w + 7u) / 8u, (h + 7u) / 8u, 1u);

  // 3) Publish: copy sharpened temp back over the TAA output so downstream
  // reads see sharpened pixels. History already saved unsharpened in (1).
  cmd_list->barrier(d->rcas_temp_texture, reshade::api::resource_usage::unordered_access,
                    reshade::api::resource_usage::copy_source);
  cmd_list->barrier(srcRes, reshade::api::resource_usage::shader_resource,
                    reshade::api::resource_usage::copy_dest);
  cmd_list->copy_texture_region(d->rcas_temp_texture, 0, nullptr, srcRes, 0, nullptr);
  cmd_list->barrier(srcRes, reshade::api::resource_usage::copy_dest,
                    reshade::api::resource_usage::render_target);
  cmd_list->barrier(d->rcas_temp_texture, reshade::api::resource_usage::copy_source,
                    reshade::api::resource_usage::unordered_access);
}

// Resolve the GBuffer RTV4 to its resource, REJECT anything that is not plausibly
// the motion buffer, and keep OUR OWN shader-resource view on what survives.
//
// The game's render target can never be handed to the chain directly. ReShade's
// D3D11 update_descriptor_tables copies the handle verbatim and the driver then
// consumes it as an ID3D11ShaderResourceView, so passing an
// ID3D11RenderTargetView is a bad indirect call. Passing OUR view, which is a real
// SRV on the same resource, costs no copy and no extra memory.
//
// Rejecting rather than warning is the point. A wrong capture used to bind
// anyway, which produced an image that looked plausible and was wrong -- the
// hardest kind of bug to notice. Now a wrong candidate is refused, logged once with
// what it actually was, and the capture is re-armed so a later five-target bind in
// the same frame can still take its place. The blur is off for that frame, which
// is a state you can act on.
//
// Id3D11View::GetResource is the one view call that IS valid on a render target,
// and it is the same call the TAA path already makes successfully every frame.
static void MBMotionRtv4Ensure(reshade::api::device* dev, DeviceData* d) {
  if (!dev || !d) return;
  if (d->mb_rtv4_srv.handle == 0u || !d->mb_rtv4_live.load()) return;
  // Rebuild requested by the resource-destroy event: drop the dead view first, on
  // this thread, then fall through and make a new one.
  if (d->mb_rtv4_owned_dirty) {
    d->mb_rtv4_owned_dirty = false;
    if (d->mb_rtv4_owned_srv.handle != 0u) {
      dev->destroy_resource_view(d->mb_rtv4_owned_srv);
      d->mb_rtv4_owned_srv = {};
    }
  }
  reshade::api::resource mres = dev->get_resource_from_view(d->mb_rtv4_srv);
  if (mres.handle == 0u) return;
  if (mres.handle == d->mb_rtv4_res && d->mb_rtv4_owned_srv.handle != 0u) return;

  if (d->mb_rtv4_owned_srv.handle != 0u) {
    dev->destroy_resource_view(d->mb_rtv4_owned_srv);
    d->mb_rtv4_owned_srv = {};
  }
  d->mb_rtv4_res = mres.handle;

  // .texture is a union member, so .type has to be tested before it is read.
  // Reading it on a buffer desc is undefined behaviour, and undefined behaviour
  // resolves differently under Release optimisation.
  auto rdesc = dev->get_resource_desc(mres);
  const bool isTexture = (rdesc.type == reshade::api::resource_type::texture_2d);
  const bool isMotionFormat = isTexture && (rdesc.texture.format == reshade::api::format::r16g16_float);
  const bool isPlausibleSize = isTexture && (rdesc.texture.width > 0u && rdesc.texture.height > 0u);

  if (!isMotionFormat || !isPlausibleSize) {
    d->mb_rtv4_res = 0u;
    // Re-arm, but only while the per-frame budget allows it, so a frame where no
    // five-target bind is the motion target cannot spin.
    if (d->mb_rtv4_rejects < DeviceData::kMotionBlurRtv4RejectCap) {
      d->mb_rtv4_rejects++;
      d->mb_rtv4_frame = UINT64_MAX;
    }
    if (!d->mb_rtv4_warned_fmt) {
      d->mb_rtv4_warned_fmt = true;
      const std::string got = isTexture
          ? (std::to_string(rdesc.texture.width) + "x" + std::to_string(rdesc.texture.height)
             + " format=" + std::to_string(static_cast<int>(rdesc.texture.format)))
          : std::string("not a texture_2d");
      reshade::log::message(reshade::log::level::warning,
          ("[MotionBlur] the first five-target bind's slot 4 is not the motion buffer "
           "(" + got + "). Expected r16g16_float. Blur is off this frame and the "
           "capture is retried; this message appears once.").c_str());
    }
    return;
  }
  dev->create_resource_view(mres, reshade::api::resource_usage::shader_resource,
      reshade::api::resource_view_desc(reshade::api::resource_view_type::texture_2d,
                                       rdesc.texture.format, 0, 1, 0, 1),
      &d->mb_rtv4_owned_srv);
  if (d->mb_rtv4_owned_srv.handle == 0u) {
    d->mb_rtv4_res = 0u;
    reshade::log::message(reshade::log::level::warning,
        "[MotionBlur] could not create a shader resource view on the RTV4 motion "
        "resource. Blur is off this frame; it will retry. This is a declared "
        "limitation, not a fallback to the render target.");
  }
}

// The selected motion view, or an empty view when it is not available. NO fallback
// to the other source: a silent substitution would make the setting lie, and with
// two candidates the only way to diagnose a stopped blur is a log line naming the
// one that is absent.
//
// The RTV4 branch returns OUR view, never the game's render target -- see
// MBMotionRtv4Ensure. MBMotionRtv4Ensure must have run first.
//
// Defined here rather than beside the other motion blur statics because it needs
// DeviceData complete, and that struct is declared further down the file.
static reshade::api::resource_view MBMotionInputView(DeviceData* d) {
  if (!d) return {};
  if (MBMotionUseRtv()) {
    return (d->mb_rtv4_owned_srv.handle != 0u)
             ? d->mb_rtv4_owned_srv : reshade::api::resource_view{};
  }
  return (d->rcas_motion_srv.handle != 0u && d->rcas_motion_live.load())
           ? d->rcas_motion_srv : reshade::api::resource_view{};
}

// Latest-bound RTV0 tracker for the RCAS stage, plus the motion blur's RTV4
// motion capture (D3D11 immediate-list assumption, same as the rest of this
// file's per-frame capture logic: binds and draws are ordered on one list, and
// on_draw fires after binds).
//
// -- THIS FUNCTION MUST MAKE NO DEVICE CALLS -----------------------------------
// It runs on every OMSetRenderTargets in the process, including those issued by
// the upscaler proxies, which own their own D3D11 devices and views. ReShade's
// get_resource_from_view is an unchecked reinterpret_cast plus a virtual call,
// guarded only by an assert, and an assert is compiled out of a Release build:
//
//   assert(view != 0);
//   reinterpret_cast<ID3D11View *>(view.handle)->GetResource(&resource);
//
// So a view handle that is non-zero but not a live ID3D11View on THIS device is a
// wild pointer dereference, and it took the process down with 0xC0000005 inside
// d3d11.dll. Only a null slot is detectable here, and a null slot is the one
// case that cannot be a live view.
//
// Everything that needs to query the device -- resolving the resource, the
// r16g16_float check, the dimensions log -- happens in PrepareMotionBlur, at a
// point where the device and command list are known correct and the identical
// query already succeeds for the TAA motion view every frame.
static void OnBindRenderTargetsRCAS(reshade::api::command_list* cmd_list, uint32_t count,
                                    const reshade::api::resource_view* rtvs,
                                    reshade::api::resource_view /*dsv*/) {
  if (!cmd_list || !rtvs) return;
  auto* dev = cmd_list->get_device();
  if (!dev) return;
  auto* d = dev->get_private_data<DeviceData>();
  if (!d) return;
  d->rcas_last_rtv0 = (count > 0) ? rtvs[0] : reshade::api::resource_view{};

  // -- Motion blur: the GBuffer motion slot, which is NOT the same on every game.
  //
  // A table rather than scattered conditionals, because the two numbers have to
  // agree and index 4 is only valid when count is 5:
  //
  //   Sora 1st / Sora 2nd : 5 targets, motion in slot 4 (SV_Target4)
  //   Kai                  : 4 targets, motion in slot 3 (SV_Target3)
  //
  // Sourced from the GBuffer writers themselves, and the count is corroborated
  // across ALL of each game's writers: the 8 Sora shaders all declare o0..o4, and
  // all 11 Kai shaders declare o0..o3. A wider "N or more" test is what let a later
  // pass overwrite the capture and produced a 2560x1440 texture that was not the
  // motion buffer, so the count is tested EXACTLY.
  //
  // FIRST matching bind of the frame wins: the GBuffer is the earliest one, so
  // first-wins is the GBuffer and later passes cannot clobber it. MBMotionRtv4Ensure
  // clears mb_rtv4_frame when it rejects a candidate, re-arming this for the rest
  // of that frame.
  const uint32_t wantCount = IsKai() ? 4u : 5u;
  const uint32_t motionSlot = IsKai() ? 3u : 4u;
  if (count != wantCount || rtvs[motionSlot].handle == 0u) return;
  if (d->mb_rtv4_frame == d->frame_index) return;
  d->mb_rtv4_frame = d->frame_index;
  d->mb_rtv4_srv = rtvs[motionSlot];
  d->mb_rtv4_live = true;
}

// ----------- Motion Blur (Guertin et al. 2013) -----------
//
// Six dispatches, run inline on the deploy draw's own command list so ordering
// against the game's own work is strict and no frame of latency is added:
//
//   linearize depth -> velocity -> tilemax X -> tilemax Y -> neighbormax -> gather
//
// Two modes, ONE deploy point. The chain runs inline on the tonemap draw's own
// command list, so ordering against the game's own work is strict and no frame of
// latency is added:
//
//   1 Cutscene Only: same deploy, but only on frames whose DoF gather drew.
//     This game dispatches depth of field only in cutscenes, so that draw is the
//     cutscene signal. Because the tonemap is dispatched after the DoF chain, the
//     blur still lands on top of DoF.
//   2 Always On: same deploy, every frame.
//
// Exactly one motion blur is ever active; mode only changes the trigger. There is
// no second deploy point and no copy-out/copy-back, because the result is pushed
// at the tonemap's t0 and the game's own shader consumes it.
//
// Resources are created lazily from inside the deploy callback rather than from
// the present hook. That is deliberate: the previous attempt created them under
// the present hook's GTVBAO gate, which meant motion blur silently did nothing
// unless GTVBAO was enabled.

static void DestroyMotionBlurSet(reshade::api::device* dev, DeviceData* d) {
  if (!dev || !d) return;
  auto dv = [&](reshade::api::resource_view& v) { if (v.handle) { dev->destroy_resource_view(v); v = {}; } };
  auto dr = [&](reshade::api::resource& r) { if (r.handle) { dev->destroy_resource(r); r = {}; } };
  dv(d->mb_tilemax_h_srv); dv(d->mb_tilemax_h_uav); dr(d->mb_tilemax_h_texture);
  dv(d->mb_tilemax_srv); dv(d->mb_tilemax_uav); dr(d->mb_tilemax_texture);
  dv(d->mb_neighbormax_srv); dv(d->mb_neighbormax_uav); dr(d->mb_neighbormax_texture);
  dv(d->mb_half_color_srv); dv(d->mb_half_color_uav); dr(d->mb_half_color_texture);
  dv(d->mb_half_result_srv); dv(d->mb_half_result_uav); dr(d->mb_half_result_texture);
    dv(d->mb_output_srv); dv(d->mb_output_uav); dr(d->mb_output_texture);
    dv(d->mb_resolve_srv); dv(d->mb_resolve_uav); dr(d->mb_resolve_texture);
    d->mb_working_w = 0u; d->mb_working_h = 0u;
    d->mb_motion_w = 0u; d->mb_motion_h = 0u;
  d->mb_tiles_x = 0u; d->mb_tiles_y = 0u;
  d->mb_radius_px = 0u;
  d->mb_output_fmt = reshade::api::format::unknown;
  d->mb_halfres = false;
  d->mb_resources_ready = false;
  d->mb_prep_valid = false;
}

// Full teardown, including the immutable per-device pieces. Called from device
// teardown only; the day-to-day path is DestroyMotionBlurSet.
static void DestroyMotionBlurResources(reshade::api::device* dev, DeviceData* d) {
  if (!dev || !d) return;
  DestroyMotionBlurSet(dev, d);
  for (auto& set : d->mb_tables) {
    for (auto& t : set) { if (t.handle) { dev->free_descriptor_table(t); t = {}; } }
    set = {};
  }
  for (auto& p : d->mb_pipelines) { if (p.handle) { dev->destroy_pipeline(p); p = {}; } }
  for (auto& l : d->mb_layouts) { if (l.handle) { dev->destroy_pipeline_layout(l); l = {}; } }
  if (d->mb_noise_fallback_srv.handle) { dev->destroy_resource_view(d->mb_noise_fallback_srv); d->mb_noise_fallback_srv = {}; }
  if (d->mb_noise_fallback_res.handle) { dev->destroy_resource(d->mb_noise_fallback_res); d->mb_noise_fallback_res = {}; }
  if (d->mb_point_clamp_sampler.handle) { dev->destroy_sampler(d->mb_point_clamp_sampler); d->mb_point_clamp_sampler = {}; }
  // Our SRV on the game's motion resource. Not part of mb_layouts/mb_tables: it is
  // a view on a game-owned resource, and those are freed on a set rebuild, not on
  // device teardown.
  if (d->mb_rtv4_owned_srv.handle) { dev->destroy_resource_view(d->mb_rtv4_owned_srv); d->mb_rtv4_owned_srv = {}; }
  d->mb_rtv4_res = 0u;
  d->mb_rtv4_owned_dirty = false;
}

static bool EnsureMotionBlurPipelines(reshade::api::device* dev, DeviceData* d) {
  using DR = reshade::api::descriptor_range;
  using DS = reshade::api::shader_stage;
  using DT = reshade::api::descriptor_type;
  using P = reshade::api::pipeline_layout_param;
  if (!dev || !d) return false;
  // Same layout shape as the GTVBAO stages: four descriptor tables at root
  // params 0-3, push constants at root param 4. The push block is the WHOLE
  // ShaderInjectData rather than a hand-mapped subset, because the motion blur
  // shaders include shared.h and read the mb_* fields directly � one source of
  // truth, no per-shader re-declaration to drift.
    // gather binds 6: color, motion, neighbormax, depth, noise, and the GAME
    // motion alongside the resolved one, which only the two velocity views read
    // (they return before the tap loop, so it costs nothing in the normal path).
    static constexpr uint32_t kSrvPerPass[kMotionBlurPassCount] = {1u, 1u, 1u, 6u, 2u, 2u};
    static const std::span<const uint8_t> kBytecode[kMotionBlurPassCount] = {
        __motion_blur_downsample, __motion_blur_tilemax, __motion_blur_neighbormax,
        __motion_blur_gather, __motion_blur_composite, __motion_blur_resolve};
  for (uint32_t pass = 0; pass < kMotionBlurPassCount; ++pass) {
    if (d->mb_layouts[pass].handle == 0u) {
      DR sampler_r = {0,0,0,1,DS::all_compute,1,DT::sampler};
      DR cbv_r     = {0,0,0,1,DS::all_compute,1,DT::constant_buffer};
      DR srv_r     = {0,0,0,kSrvPerPass[pass],DS::all_compute,1,DT::texture_shader_resource_view};
      DR uav_r     = {0,0,0,1,DS::all_compute,1,DT::texture_unordered_access_view};
      reshade::api::constant_range push_range = {};
      push_range.binding = 0;
      push_range.dx_register_index = 13;
      push_range.dx_register_space = 0;
      push_range.count = static_cast<uint32_t>(sizeof(ShaderInjectData) / sizeof(uint32_t));
      push_range.visibility = DS::all_compute;
      P param_sampler, param_cbv, param_srv, param_uav, param_constants;
      param_sampler.type = reshade::api::pipeline_layout_param_type::descriptor_table;
      param_sampler.descriptor_table.count = 1; param_sampler.descriptor_table.ranges = &sampler_r;
      param_cbv.type = reshade::api::pipeline_layout_param_type::descriptor_table;
      param_cbv.descriptor_table.count = 1; param_cbv.descriptor_table.ranges = &cbv_r;
      param_srv.type = reshade::api::pipeline_layout_param_type::descriptor_table;
      param_srv.descriptor_table.count = 1; param_srv.descriptor_table.ranges = &srv_r;
      param_uav.type = reshade::api::pipeline_layout_param_type::descriptor_table;
      param_uav.descriptor_table.count = 1; param_uav.descriptor_table.ranges = &uav_r;
      param_constants.type = reshade::api::pipeline_layout_param_type::push_constants;
      param_constants.push_constants = push_range;
      P params[5] = {param_sampler, param_cbv, param_srv, param_uav, param_constants};
      if (!dev->create_pipeline_layout(5, params, &d->mb_layouts[pass])) return false;
    }
    if (!EnsureGTVBAODescriptorTables(dev, d->mb_layouts[pass], &d->mb_tables[pass])) return false;
    if (d->mb_pipelines[pass].handle == 0u) {
      // THREE resolve variants, one pass slot. HLSL packoffset() is a compile-time
      // constant and the three games disagree on the scene cbuffer layout:
      //
      //   prevViewProj_g / jitter     Sora 1st  c74 / jitterDiff_g c78
      //                              Sora 2nd  c75 / jitterDiff_g c79
      //                              Kai       c85 / motionJitterOffset_g c93
      //
      // Kai is not just another offset pair: its jitter field has a different NAME
      // too, because the game's motion writer applies it negated. view_g, proj_g
      // and viewProjInv_g are c0/c8/c20 on all three, which is why the gather needs
      // no variant. The difference therefore has to live in the bytecode.
      //
      // The game is fixed for the process lifetime, so this is decided once here
      // and never needs a rebuild guard. RunMotionBlur logs which variant is in the
      // pipeline, so a wrong pick shows up as one line rather than a wrong image.
      std::span<const uint8_t> code = kBytecode[pass];
      if (pass == kMbResolve) {
        if (IsKai())          code = __motion_blur_resolve_kai;
        else if (IsSora1st()) code = __motion_blur_resolve_sora1st;
      }
      if (code.empty() || d->mb_layouts[pass].handle == 0u) return false;
      reshade::api::shader_desc sd = {};
      sd.code = code.data();
      sd.code_size = code.size();
      sd.entry_point = "main";
      reshade::api::pipeline_subobject so = {reshade::api::pipeline_subobject_type::compute_shader, 1, &sd};
      if (!dev->create_pipeline(d->mb_layouts[pass], 1, &so, &d->mb_pipelines[pass])) return false;
    }
  }
  return true;
}

// The shared layout still declares a sampler table at root param 0 and apply()
// fills it for every pass, so the descriptor must be valid even though the
// gather stopped sampling. Owned by this chain, not borrowed from GTVBAO: the
// GTVBAO handle is destroyed and recreated with that feature's resources, and a
// null sampler there makes D3D11 sampling return zero, which silently zeroes the
// filter's only input.
static bool EnsureMotionBlurSampler(reshade::api::device* dev, DeviceData* d) {
  if (!dev || !d) return false;
  if (d->mb_point_clamp_sampler.handle) return true;
  reshade::api::sampler_desc sd = {};
  sd.filter = reshade::api::filter_mode::min_mag_mip_point;
  sd.address_u = sd.address_v = sd.address_w = reshade::api::texture_address_mode::clamp;
  return dev->create_sampler(sd, &d->mb_point_clamp_sampler);
}

// The gather declares a Texture3D at t4 even when the jitter source is the
// Halton sequence, and a null descriptor is not a legal binding, so a 1x1x1
// stand-in is bound whenever the real IS-FAST volume is unusable.
static bool EnsureMotionBlurNoiseFallback(reshade::api::device* dev, DeviceData* d) {
  if (!dev || !d || d->mb_noise_fallback_srv.handle) return d->mb_noise_fallback_srv.handle != 0u;
  reshade::api::resource_desc rd = {};
  rd.type = reshade::api::resource_type::texture_3d;
  rd.texture = {1, 1, 1, 1, reshade::api::format::r8_unorm, 1};
  rd.heap = reshade::api::memory_heap::gpu_only;
  rd.usage = reshade::api::resource_usage::shader_resource;
  if (!dev->create_resource(rd, nullptr, reshade::api::resource_usage::shader_resource, &d->mb_noise_fallback_res)) return false;
  reshade::api::resource_view_desc vd(reshade::api::resource_view_type::texture_3d,
                                       reshade::api::format::r8_unorm, 0, 1, 0, 1);
  if (!dev->create_resource_view(d->mb_noise_fallback_res, reshade::api::resource_usage::shader_resource,
                                 vd, &d->mb_noise_fallback_srv)) {
    dev->destroy_resource(d->mb_noise_fallback_res);
    d->mb_noise_fallback_res = {};
    return false;
  }
  return true;
}

static void CreateMotionBlurSet(reshade::api::device* dev, DeviceData* d,
                                uint32_t workingW, uint32_t workingH,
                                uint32_t motionW, uint32_t motionH,
                                uint32_t tilesX, uint32_t tilesY,
                                reshade::api::format outputFormat, bool halfRes) {
  DestroyMotionBlurSet(dev, d);
  if (!dev || !d) return;
  auto mk = [&](uint32_t w, uint32_t h, reshade::api::format fmt,
                reshade::api::resource* res, reshade::api::resource_view* srv,
                reshade::api::resource_view* uav) -> bool {
    reshade::api::resource_desc rd = {};
    rd.type = reshade::api::resource_type::texture_2d;
    rd.texture = {w, h, 1, 1, fmt, 1};
    rd.heap = reshade::api::memory_heap::gpu_only;
    rd.usage = reshade::api::resource_usage::shader_resource | reshade::api::resource_usage::unordered_access;
    if (!dev->create_resource(rd, nullptr, reshade::api::resource_usage::shader_resource, res)) return false;
    reshade::api::resource_view_desc vd(reshade::api::resource_view_type::texture_2d, fmt, 0, 1, 0, 1);
    if (srv && !dev->create_resource_view(*res, reshade::api::resource_usage::shader_resource, vd, srv)) return false;
    if (uav && !dev->create_resource_view(*res, reshade::api::resource_usage::unordered_access, vd, uav)) return false;
    return true;
  };
  // Tile textures hold plain UV velocity read straight from the game's RG16F
  // motion texture, so there is no re-encode step and no 8-bit variant to switch
  // on, and the shaders declare float2 over them either way.
  //
  // The format must have GUARANTEED D3D11.0 UAV type-write support. R16G16_FLOAT
  // does not: level 11_0 requires R32_*, RG32_*, RGBA8_*, RGBA16_UINT/SNORM/
  // FLOAT and RGBA32_*, and R16G16_FLOAT only joins the required set in D3D11.1
  // / feature level 12_0. Reading it as an SRV is always legal, which is why the
  // game stores motion in it and why the gather's Velocity view reads correctly
  // while the tile chain sits at zero -- a dropped UAV write leaves tilemax and
  // neighbormax zeroed, so vmax is zero, the early-out always fires, and the
  // filter is a silent passthrough with no error anywhere.
  //
  // RGBA16F is in the required set and is what the pre-Phase-2 velocity
  // intermediate used. The extra bandwidth is irrelevant at this size: the tile
  // set is tiles*tileH + 2*tiles*tiles texels (~40k at 1440p), so 8 B/texel
  // costs ~160 KB more against a 2560x1440 output buffer. The bandwidth argument
  // that justified R16G16F applied to the full-resolution velocity intermediate,
  // which no longer exists. The shaders declare float2 over this, taking .xy.
  const auto tileFmt = reshade::api::format::r16g16b16a16_float;
  const bool tileOk =
     mk(tilesX, std::max(motionH, 1u), tileFmt,
        &d->mb_tilemax_h_texture, &d->mb_tilemax_h_srv, &d->mb_tilemax_h_uav)
   && mk(tilesX, tilesY, tileFmt,
        &d->mb_tilemax_texture, &d->mb_tilemax_srv, &d->mb_tilemax_uav)
   && mk(tilesX, tilesY, tileFmt,
        &d->mb_neighbormax_texture, &d->mb_neighbormax_srv, &d->mb_neighbormax_uav)
    && mk(workingW, workingH, outputFormat,
         &d->mb_output_texture, &d->mb_output_srv, &d->mb_output_uav)
    // The camera/object split output, at the MOTION texture's resolution (which
    // is not necessarily workingW/H). Allocated unconditionally: it is only read
    // or written when the two weights differ, so at defaults this is 29.5 MB of
    // untouched VRAM at 1440p, which is a far better trade than tying resource
    // lifetime to a settings value and rebuilding the whole set when it moves.
    // RGBA16F, not R16G16F, for the same reason as the tile chain above.
    && mk(std::max(motionW, 1u), std::max(motionH, 1u), tileFmt,
         &d->mb_resolve_texture, &d->mb_resolve_srv, &d->mb_resolve_uav);
  // Half-res surfaces are optional and only exist while the toggle is on, so the
  // default path allocates nothing extra. They use the blit's own format so the
  // gather input and output match what the final reconstruct expects.
  const uint32_t halfW = std::max(1u, (workingW + 1u) / 2u);
  const uint32_t halfH = std::max(1u, (workingH + 1u) / 2u);
  bool ok = tileOk;
  if (ok && halfRes) {
    ok = mk(halfW, halfH, outputFormat,
            &d->mb_half_color_texture, &d->mb_half_color_srv, &d->mb_half_color_uav)
      && mk(halfW, halfH, outputFormat,
            &d->mb_half_result_texture, &d->mb_half_result_srv, &d->mb_half_result_uav);
  }
  if (!ok) {
    DestroyMotionBlurSet(dev, d);
    return;
  }
  d->mb_working_w = workingW; d->mb_working_h = workingH;
  d->mb_motion_w = motionW; d->mb_motion_h = motionH;
  d->mb_tiles_x = tilesX; d->mb_tiles_y = tilesY;
  d->mb_radius_px = static_cast<uint32_t>(std::max(shader_injection.mb_max_radius_px, 8.f));
  d->mb_output_fmt = outputFormat;
  d->mb_halfres = halfRes;
  d->mb_resources_ready = true;
}

// Framerate normalisation. A motion vector is a per-frame displacement, so a
// camera crossing the same point covers half as many pixels at 120 fps as at 60.
// Every length in this filter derives from that vector -- the streak, the ladder
// buckets, the early-out threshold, and the half-resolution split -- so all of
// them would move with framerate. Scaling the velocity by
// (reference interval / actual interval) puts them into reference-frame units in
// one place and makes every one of those thresholds framerate-independent for
// free. That is the point: patching the split threshold alone would have fixed
// the reported symptom and left the streak, the ladder and the early-out still
// -- Frame-rate scaling: UE's r.MotionBlurTargetFPS semantics -------------------
//
// Motion vectors are per-frame displacements, so a camera crossing the same point
// covers half as many pixels at 120 fps as at 60. Every length in the filter is
// derived from that vector, so without a scale the streak, the sample ladder, the
// early-out and the half-res split all change with framerate.
//
// This reproduces UE's TargetDeltaTime formulation rather than only its
// steady-state result:
//
//   targetFps <= 0 : targetDt = lerp(targetDt, dt, 0.1)   // track the real frame time
//   targetFps >  0 : targetDt = 1 / targetFps              // fixed target
//   scale          = targetDt / dt
//
// With no target set, the scale converges to 1.0 at ANY steady frame rate, which
// is the point: the shutter then spans the real frame, so a longer frame is a
// longer streak. The 0.1 moving average is what makes a 60->30->60 transition
// RAMP over roughly ten frames instead of stepping, which a hard scale of 1.0
// would not do. With a target set, the target is fixed and un-smoothed, matching
// UE.
//
// The divisor is deliberately the INSTANTANEOUS dt, not an average of it. Smoothing
// the divisor is what this used to do, and it meant one stall depressed the scale
// for as long as the average took to recover. The cost of the UE form is that a
// genuinely fast frame now spikes the scale for a single frame, toward the clamp;
// the clamp is what bounds that.
//
// dt < 0.5f is PAUSE handling, not temporal smoothing: it keeps an alt-tab or a
// loading stall from feeding the average a multi-second delta, which would then
// take ~20 frames to decay and show as a burst of maximum blur on resume. A frame
// the guard rejects contributes nothing and returns the previous scale unchanged.
//
// This measures the RENDER rate, not the display rate. For normally-paced games
// they agree; a game rendering internally faster than it presents would size the
// streak for the render rate. Reading the swapchain's present count would fix
// that and is far more machinery than this is worth.
static float MotionBlurFrameScale() {
  static std::chrono::steady_clock::time_point last{};
  static float targetDt = 0.0f;  // seconds; UE's TargetDeltaTime
  static float lastScale = 1.0f;
  const auto now = std::chrono::steady_clock::now();
  const float dt = std::chrono::duration<float>(now - last).count();
  last = now;
  if (!(dt > 1e-4f && dt < 0.5f)) return lastScale;  // not a real frame
  if (shader_injection.mb_frame_rate_reference <= 0.0f) {
    targetDt = (targetDt <= 0.0f) ? dt : targetDt + (dt - targetDt) * 0.1f;
  } else {
    targetDt = 1.0f / shader_injection.mb_frame_rate_reference;
  }
  lastScale = std::clamp(targetDt / dt, 0.25f, 4.0f);
  return lastScale;
}

// Resolves the game's depth and motion inputs, recreates the owned set when
// anything that affects shape or layout moved, and mirrors everything the
// shaders read into the push block. Returns false when the chain cannot run.
// motionOut receives the exact view that mb_motion_valid describes: the game's
// motion texture when valid, the 1x1 stand-in otherwise. Callers bind that
// return value; they must not re-resolve it from DeviceData.
static bool PrepareMotionBlur(reshade::api::device* dev, DeviceData* d,
                              uint32_t workingW, uint32_t workingH,
                              reshade::api::format outputFormat,
                              reshade::api::resource_view* motionOut) {
  if (!dev || !d || !motionOut) return false;
  *motionOut = d->fallback_srv;
  if (workingW == 0u || workingH == 0u) return false;
  if (outputFormat == reshade::api::format::unknown) return false;
  if (!EnsureMotionBlurPipelines(dev, d)) return false;
  if (!EnsureMotionBlurSampler(dev, d)) return false;
  if (!EnsureMotionBlurNoiseFallback(dev, d)) return false;

  // Depth: the game's scene depth, captured from the lighting pass. Queried
  // here rather than cached at capture time so it can never go stale.
  uint32_t depthW = 0u, depthH = 0u;
  if (d->captured_depth_srv.handle != 0u) {
    reshade::api::resource dres = dev->get_resource_from_view(d->captured_depth_srv);
    if (dres.handle != 0u) {
      auto ddesc = dev->get_resource_desc(dres);
      if (ddesc.type == reshade::api::resource_type::texture_2d) {
        depthW = ddesc.texture.width;
        depthH = ddesc.texture.height;
      }
    }
  }
  if (depthW == 0u || depthH == 0u) return false;

  // Motion: the game's t3 from the TAA draw.
  //
  // The view handed back through motionOut and the mb_motion_valid flag are
  // derived from the SAME branch on purpose. They must never be decided in two
  // places: binding the 1x1 white stand-in while reporting the buffer valid
  // makes every pixel read a uniform ~0.0004 UV velocity, which silently
  // collapses the whole filter to a passthrough (black Blur Amount view, cyan
  // Velocity view, no blur at all). Validity lives in the push block rather than
  // in a null binding, matching the DoF IS-FAST pattern; the gather zeroes vmax
  // when it is invalid, so the chain degrades to a passthrough by design.
  //
  // motionBound is the ONLY thing that decides the binding and the flag, and it
  // is deliberately the same two-part test RCAS uses (RCASSharpenCS binds the
  // same view the same way). The resource desc query below is a REFINEMENT of
  // the divisor and the tile height, never a gate: a view that failed to resolve
  // is still perfectly readable, and treating the query as authoritative is what
  // silently disabled this filter once already. It must not be re-promoted.
  // RTV4 source: build our own SRV on the motion resource FIRST, so that whatever
  // MBMotionInputView hands the chain below is a view the chain may legally bind.
  if (MBMotionUseRtv()) {
    MBMotionRtv4Ensure(dev, d);
  }
  const reshade::api::resource_view motionView = MBMotionInputView(d);
  const bool motionBound = motionView.handle != 0u;
  uint32_t motionW = workingW, motionH = workingH;
  if (motionBound) {
    reshade::api::resource mres = dev->get_resource_from_view(motionView);
    if (mres.handle != 0u) {
      // mdesc.texture is a union member: .type must be tested before it is read,
      // exactly as CSResolveCapture does. Reading it on a buffer desc is undefined
      // behaviour and resolves differently under Release optimisation.
      auto mdesc = dev->get_resource_desc(mres);
      if (mdesc.type == reshade::api::resource_type::texture_2d) {
        if (mdesc.texture.width > 0u && mdesc.texture.height > 0u) {
          motionW = mdesc.texture.width;
          motionH = mdesc.texture.height;
        }
        // The RTV4 format check lives in MBMotionRtv4Ensure, next to the view it
        // describes. Only the dimensions report is left here.
        if (MBMotionUseRtv() && !d->mb_logged_rtv4_dims
            && MotionBlurLogEnabled()) {
          d->mb_logged_rtv4_dims = true;
          reshade::log::message(reshade::log::level::info,
              ("[MotionBlur] RTV4 motion " + std::to_string(motionW) + "x"
               + std::to_string(motionH) + " bound as our own SRV").c_str());
        }
      }
    }
  }
  *motionOut = motionBound ? motionView : d->fallback_srv;

  const uint32_t radiusPx = static_cast<uint32_t>(std::max(shader_injection.mb_max_radius_px, 8.f));
  // Tiles are sized in PIXELS, not counted, so they stay square on a 16:9 frame.
  // A square count gave 27x27 over 2560x1440, i.e. 95x54 px tiles, and the paper's
  // 1-ring property plus the Section 4.2 t-in-tiles falloff both assume roughly
  // uniform tiles; 54 px was only 1.3x the longest streak.
  //
  // Two tiles of headroom keeps the whole 0..2 Intensity range meaningful: the
  // domain is (|v|max + jitter) * intensity <= (40 + 1.3) px at 1080p reference,
  // so a 2*40 px tile leaves ~1.94 before the gather's one-tile clamp engages.
  // Total tilemax loads are unchanged by this (tilesX * motionH * tileW is
  // invariant), so prep should neither improve nor regress -- the win is
  // correctness, not speed.
  const uint32_t tilePx = std::max(1u, radiusPx * 2u);
  const uint32_t tilesX = std::max(1u, (workingW + tilePx - 1u) / tilePx);
  const uint32_t tilesY = std::max(1u, (workingH + tilePx - 1u) / tilePx);
  // The half-res surfaces are part of the owned set, so toggling the setting has
  // to rebuild it. That is the same one-frame hitch Max Radius already causes and
  // is why the surfaces are not simply kept allocated while unused.
  const bool halfRes = shader_injection.mb_halfres > 0.5f;
  // Depth and motion dimensions are no longer resource-sizing inputs for the
  // gather: it samples the game's textures directly and the conversion scale is a
  // per-frame push. motionH still matters because it is the height of the
  // tilemax_h intermediate, and motionW now also sizes the camera/object resolve
  // output, which is full motion resolution.
  const bool needRecreate =
      !d->mb_resources_ready
      || d->mb_working_w != workingW || d->mb_working_h != workingH
      || d->mb_motion_w != motionW || d->mb_motion_h != motionH
      || d->mb_tiles_x != tilesX || d->mb_tiles_y != tilesY
      || d->mb_radius_px != radiusPx
      || d->mb_halfres != halfRes
      || d->mb_output_fmt != outputFormat;
  if (needRecreate) {
    CreateMotionBlurSet(dev, d, workingW, workingH, motionW, motionH, tilesX, tilesY, outputFormat, halfRes);
    if (!d->mb_resources_ready) return false;
  }

  // -- push block --
  shader_injection.mb_working_w = static_cast<float>(workingW);
  shader_injection.mb_working_h = static_cast<float>(workingH);
  shader_injection.mb_depth_w = static_cast<float>(std::max(depthW, 1u));
  shader_injection.mb_depth_h = static_cast<float>(std::max(depthH, 1u));
  shader_injection.mb_motion_w = static_cast<float>(std::max(motionW, 1u));
  shader_injection.mb_motion_h = static_cast<float>(std::max(motionH, 1u));
  // Translated here rather than bound straight to the setting: the gather wants a
  // tap count, and this is the last point where the preset is still a preset.
  shader_injection.mb_sample_count = MBQualitySampleCount(g_mb_quality);

  // Report the three resolutions once. Every cross-texture read already addresses
  // by normalised UV against its own texture's dimensions, so a mismatch is not a
  // correctness problem -- but it does decide whether the gather has to FILTER its
  // motion reads instead of point sampling them, and that is invisible from the
  // image alone. Saying so explicitly is the difference between "upscaling is
  // supported" and "upscaling is assumed to work".
  if (!d->mb_logged_dims) {
    d->mb_logged_dims = true;
    if (MotionBlurLogEnabled()) {
      const bool motionDiffers = (motionW != workingW) || (motionH != workingH);
      const bool depthDiffers = (depthW != workingW) || (depthH != workingH);
      reshade::log::message(reshade::log::level::info,
          ("[MotionBlur] buffers: colour " + std::to_string(workingW) + "x" + std::to_string(workingH)
           + " | motion " + std::to_string(motionW) + "x" + std::to_string(motionH)
           + " | depth " + std::to_string(depthW) + "x" + std::to_string(depthH)
           + " | source " + MBMotionInputName()
           + (motionDiffers
                ? " -- motion differs, gather is using filtered reads"
                : " -- motion matches, single-tap reads")
           + (depthDiffers ? " (depth differs, point sampled by design)" : "")).c_str());
    }
  }
  shader_injection.mb_tiles_x = static_cast<float>(d->mb_tiles_x);
  shader_injection.mb_tiles_y = static_cast<float>(d->mb_tiles_y);
  shader_injection.mb_tile_uv = radiusPx / kMotionBlurRefHeight;
  shader_injection.mb_frame_index = static_cast<float>(d->frame_index % 32u);
  shader_injection.mb_motion_valid = motionBound ? 1.f : 0.f;
  // Availability only; the user's mb_jitter_source choice is never overwritten,
  // so it survives the volume being briefly unavailable.
  shader_injection.mb_jitter_ready = d->isfast_noise_srv.handle != 0u ? 1.f : 0.f;
  // Default so the single-dispatch full-res path is correct without the dispatch
  // having to remember to clear it; RunMotionBlur flips it around the half-res
  // gather when the split is active.
  shader_injection.mb_gather_side = 0.f;
  // Once per frame here, at the top of the push block, because MBGameMotionToUV
  // reads it in every shader of the chain. 1.0 at the reference rate, so the
  // default is a no-op there.
  shader_injection.mb_frame_scale = MotionBlurFrameScale();
  // Log movement in the factor. A clamp or a value stuck at 1.0 would otherwise
  // silently disable the compensation with nothing on screen to say so, and
  // "which framerate is it actually normalising to" is otherwise unobservable.
  if (std::fabs(shader_injection.mb_frame_scale - d->mb_last_frame_scale) >= 0.1f) {
    d->mb_last_frame_scale = shader_injection.mb_frame_scale;
    if (MotionBlurLogEnabled()) {
      reshade::log::message(reshade::log::level::info,
          ("[MotionBlur] frame scale " + std::to_string(shader_injection.mb_frame_scale)
           + " (reference " + std::to_string(static_cast<int>(
                  shader_injection.mb_frame_rate_reference)) + " fps)").c_str());
    }
  }
  return true;
}

// Runs the six-pass chain over colorSrc. Returns the gather output view, or an
// empty view if anything failed; the caller decides how to publish it.
static reshade::api::resource_view RunMotionBlur(reshade::api::command_list* cl, DeviceData* d,
                                                 reshade::api::resource_view colorSrc,
                                                 reshade::api::resource_view motionSrc) {
  if (!cl || !d || !colorSrc.handle) return {};
  auto* dev = cl->get_device();
  if (!dev || !d->mb_resources_ready) return {};

  const auto CS = reshade::api::shader_stage::all_compute;
  const auto AC = reshade::api::pipeline_stage::all_compute;
  const auto SR = reshade::api::resource_usage::shader_resource;
  const auto UA = reshade::api::resource_usage::unordered_access;
  auto bar = [&](reshade::api::resource r, reshade::api::resource_usage o, reshade::api::resource_usage n) {
    if (r.handle) cl->barrier(r, o, n);
  };
  auto apply = [&](uint32_t pass, reshade::api::resource_view* srvs, uint32_t srvCount,
                   reshade::api::resource_view uav) {
    std::array<reshade::api::descriptor_table_update, kGtvbaoDescriptorTableParamCount> u = {{
      {{},0,0,1,reshade::api::descriptor_type::sampler,&d->mb_point_clamp_sampler},
      {{},0,0,1,reshade::api::descriptor_type::constant_buffer,&d->captured_scene_cbv_view},
      {{},0,0,srvCount,reshade::api::descriptor_type::texture_shader_resource_view,srvs},
      {{},0,0,1,reshade::api::descriptor_type::texture_unordered_access_view,&uav},
    }};
    for (uint32_t i = 0; i < kGtvbaoDescriptorTableParamCount; ++i) u[i].table = d->mb_tables[pass][i];
    dev->update_descriptor_tables(kGtvbaoDescriptorTableParamCount, u.data());
    std::array<reshade::api::descriptor_table, kGtvbaoDescriptorTableParamCount> tables = {
        d->mb_tables[pass][0], d->mb_tables[pass][1], d->mb_tables[pass][2], d->mb_tables[pass][3]};
    cl->bind_descriptor_tables(CS, d->mb_layouts[pass], 0, kGtvbaoDescriptorTableParamCount, tables.data());
    cl->push_constants(CS, d->mb_layouts[pass], kGtvbaoPushConstantsLayoutParam, 0,
                       static_cast<uint32_t>(sizeof(ShaderInjectData) / sizeof(uint32_t)),
                       static_cast<const void*>(&shader_injection));
  };

  const uint32_t W = d->mb_working_w, H = d->mb_working_h;
  const uint32_t MH = std::max(d->mb_motion_h, 1u);
  const uint32_t TX = d->mb_tiles_x, TY = d->mb_tiles_y;
  // Half res is a whole-pipeline decision, not a gather-only one, so it is
  // resolved from the ALLOCATED surfaces rather than from the setting. If those
  // are missing the chain silently runs full res instead of dispatching against
  // null descriptors.
  const bool halfRes = d->mb_halfres
      && d->mb_half_color_uav.handle != 0u && d->mb_half_result_uav.handle != 0u;
  const uint32_t GW = halfRes ? std::max(1u, (W + 1u) / 2u) : W;  // gather resolution
  const uint32_t GH = halfRes ? std::max(1u, (H + 1u) / 2u) : H;
  reshade::api::resource_view depthSrc =
      d->captured_depth_srv.handle != 0u ? d->captured_depth_srv : d->fallback_srv;
  // motionSrc is supplied by PrepareMotionBlur and is the same view
  // mb_motion_valid describes. Resolving it here instead would let the bound
  // view and the flag drift apart, which silently degrades the filter to a
  // passthrough with no error anywhere.

  // -- Diagnostic chain isolation (mb_debug_chain) --
  // 0 Full, 1 Prep Only, 2 Gather Only. Branched here rather than in a shader
  // because every gate in this chain is already a CPU decision (including the
  // TileMax axis), and because "Gather Only" must not touch passes 1-5 at all.
  //
  //   chain | prep valid | runPrep | runGather | half-res downsample
  //   ------+------------+---------+-----------+---------------------
  //     0   |     -      |   yes   |    yes    | yes
  //     1   |     -      |   yes   |    no     | no (output unused)
  //     2   |    yes     |   no    |    yes    | no (stale colour reused)
  //     2   |     no     |   yes   |    yes    | yes (fallback to Full)
  //
  // The fallback avoids a misleading result on the first frame after the owned
  // set is rebuilt, when no prep pass has run against the new textures. Gather
  // Only reuses the previous frame's downsampled colour, exactly as it already
  // reuses a stale neighbormax: it is a timing mode, not a correctness one.
  const int chain = static_cast<int>(shader_injection.mb_debug_chain + 0.5f);
  const bool gatherOnly = (chain == 2) && d->mb_prep_valid;
  if (chain != d->mb_last_chain) {
    d->mb_last_chain = chain;
    d->mb_warned_gather_only = false;
    if (MotionBlurLogEnabled()) {
      reshade::log::message(reshade::log::level::info,
          chain == 0 ? "[MotionBlur] debug chain: Full"
          : chain == 1 ? "[MotionBlur] debug chain: Prep Only (gather skipped, image untouched)"
          : "[MotionBlur] debug chain: Gather Only (previous frame's prep)");
    }
  }
  if (chain == 2 && !gatherOnly && !d->mb_warned_gather_only) {
    // The flag is only consumed when the message actually goes out, so enabling
    // logging later still produces this warning instead of finding it already
    // spent.
    if (MotionBlurLogEnabled()) {
      d->mb_warned_gather_only = true;
      reshade::log::message(reshade::log::level::warning,
          "[MotionBlur] Gather Only ran before any prep pass; running the full chain "
          "for this frame instead of dispatching against unprepared textures.");
    }
  }
  const bool runPrep = !gatherOnly;
  const bool runGather = (chain != 1);

  // Half Res Detect (debug view 7) is drawn by the full-res gather alone. Running
  // the half-resolution pass as well would only get its output upsampled back over
  // the map, softening the tile-quantised boundary for no benefit, and it would
  // cost more than the view itself. splitPath therefore gates the downsample, the
  // half-res gather and the composite together; the full-res gather still runs and
  // the trailing barrier still publishes it, so the view works in both modes.
  const bool detectView = (static_cast<int>(shader_injection.mb_debug_view + 0.5f) == 7);
  const bool splitPath = halfRes && !detectView;

  if (runGather && splitPath) {
    // P0: reduce the colour input the half-res gather will read. Runs at FULL
    // working dims, which is what this shader reads to derive its own destination
    // size. The gather is left at full dims: the full-res gather needs them.
    cl->bind_pipeline(AC, d->mb_pipelines[kMbDownsample]);
    reshade::api::resource_view srvs[1] = {colorSrc};
    apply(kMbDownsample, srvs, 1, d->mb_half_color_uav);
    cl->dispatch((GW + 7u) / 8u, (GH + 7u) / 8u, 1u);
    bar(d->mb_half_color_texture, UA, SR);
  }
  // -- camera term reconstruction --
  // Always runs. It is not an optional separation: it scales the streak length by
  // the camera's share of the total magnitude, which is the whole reason object
  // motion does not reach the camera path. Binding the game's motion directly
  // instead would be a different image, not a cheaper equivalent.
  const bool runResolve = d->mb_resolve_uav.handle != 0u;
  if (runResolve) {
    // Which resolve bytecode is actually in the resolve pipeline. A wrong variant
    // on Sora 1st would not crash -- it would read a different cbuffer slot as the
    // previous view-projection and produce a subtly wrong streak length, which is
    // exactly the kind of failure that is hard to notice. One line settles it.
    if (!d->mb_logged_resolve_variant && MotionBlurLogEnabled()) {
      d->mb_logged_resolve_variant = true;
      reshade::log::message(reshade::log::level::info,
          IsKai()
              ? "[MotionBlur] scene cbuffer variant: kai (prevViewProj c85, motionJitterOffset c93)"
              : (IsSora1st()
                  ? "[MotionBlur] scene cbuffer variant: sora1st (prevViewProj c74, jitterDiff c78)"
                  : "[MotionBlur] scene cbuffer variant: sora2nd (prevViewProj c75, jitterDiff c79)"));
    }
    // P0: reconstruct .xy = camera-length-scaled motion / .zw = camera only.
    // Runs at the MOTION texture's resolution, not the working resolution, and
    // before prep because everything downstream reduces what it writes.
    cl->bind_pipeline(AC, d->mb_pipelines[kMbResolve]);
    reshade::api::resource_view srvs[2] = {motionSrc, depthSrc};
    apply(kMbResolve, srvs, 2, d->mb_resolve_uav);
    cl->dispatch((std::max(d->mb_motion_w, 1u) + 7u) / 8u, (std::max(MH, 1u) + 7u) / 8u, 1u);
    bar(d->mb_resolve_texture, UA, SR);
  }
  // What tilemax and the gather read. TileMax declares float2 and takes .xy; the
  // gather declares float4 and takes .zw for the velocity views.
  const reshade::api::resource_view motionForChain = runResolve ? d->mb_resolve_srv : motionSrc;

  if (runPrep) {
  {  // P1: TileMax, horizontal (separable, paper Section 3) over the blended motion
    cl->bind_pipeline(AC, d->mb_pipelines[kMbTileMax]);
    shader_injection.mb_pass = 0.f;
    reshade::api::resource_view srvs[1] = {motionForChain};
    apply(kMbTileMax, srvs, 1, d->mb_tilemax_h_uav);
    cl->dispatch((TX + 7u) / 8u, (MH + 7u) / 8u, 1u);
    bar(d->mb_tilemax_h_texture, UA, SR);
  }
  {  // P2: TileMax, vertical
    cl->bind_pipeline(AC, d->mb_pipelines[kMbTileMax]);
    shader_injection.mb_pass = 1.f;
    reshade::api::resource_view srvs[1] = {d->mb_tilemax_h_srv};
    apply(kMbTileMax, srvs, 1, d->mb_tilemax_uav);
    cl->dispatch((TX + 7u) / 8u, (TY + 7u) / 8u, 1u);
    bar(d->mb_tilemax_texture, UA, SR);
  }
  {  // P3: NeighborMax, 3x3 one-ring with Section 4.4 diagonal culling
    cl->bind_pipeline(AC, d->mb_pipelines[kMbNeighborMax]);
    reshade::api::resource_view srvs[1] = {d->mb_tilemax_srv};
    apply(kMbNeighborMax, srvs, 1, d->mb_neighbormax_uav);
    cl->dispatch((TX + 7u) / 8u, (TY + 7u) / 8u, 1u);
    bar(d->mb_neighbormax_texture, UA, SR);
  }
    d->mb_prep_valid = true;
  }
  if (runGather) {
    reshade::api::resource_view noise = d->isfast_noise_srv.handle != 0u
        ? d->isfast_noise_srv : d->mb_noise_fallback_srv;

    // P4: full-res gather, owns the SHORT-motion tiles. Writes every pixel:
    // blurred where it owns them, untouched source where it does not, so the
    // composite never reads a stale texel and a boundary misjudgement shows an
    // unblurred frame rather than a ghost.
    // t5 is the GAME motion alongside the resolved one. Only the Camera/Object
    // Velocity views read it, and they return before the tap loop, so binding it
    // costs nothing in the normal path.
    cl->bind_pipeline(AC, d->mb_pipelines[kMbGather]);
    shader_injection.mb_gather_side = 0.f;
    reshade::api::resource_view fullSrvs[6] = {colorSrc, motionForChain, d->mb_neighbormax_srv,
                                                depthSrc, noise, motionSrc};
    apply(kMbGather, fullSrvs, 6, d->mb_output_uav);
    cl->dispatch((W + 7u) / 8u, (H + 7u) / 8u, 1u);

    if (splitPath) {
      // The prep passes are agnostic (tilemax reads mb_motion_w/h and
      // mb_tiles_x/y, neighbormax reads mb_tiles_x/y), so this is the only point
      // where the gather resolution is halved.
      shader_injection.mb_working_w = static_cast<float>(GW);
      shader_injection.mb_working_h = static_cast<float>(GH);
      // P5: same shader, other half of the partition, at half resolution. The
      // routing test inside the shader keeps it off the short-motion tiles.
      cl->bind_pipeline(AC, d->mb_pipelines[kMbGather]);
      shader_injection.mb_gather_side = 1.f;
      reshade::api::resource_view halfSrvs[6] = {d->mb_half_color_srv, motionForChain,
                                                  d->mb_neighbormax_srv, depthSrc, noise, motionSrc};
      apply(kMbGather, halfSrvs, 6, d->mb_half_result_uav);
      cl->dispatch((GW + 7u) / 8u, (GH + 7u) / 8u, 1u);
      bar(d->mb_half_result_texture, UA, SR);
      // Full resolution back for the composite, which derives the half size itself.
      shader_injection.mb_working_w = static_cast<float>(W);
      shader_injection.mb_working_h = static_cast<float>(H);
      // P6: write-only overlay. It overwrites ONLY the long-motion tiles with the
      // half-res result and never reads mb_output_texture, so a UAV barrier between
      // the two writers is sufficient and no second full-res buffer is needed.
      cl->bind_pipeline(AC, d->mb_pipelines[kMbComposite]);
      bar(d->mb_output_texture, UA, UA);
      reshade::api::resource_view cmp[2] = {d->mb_neighbormax_srv, d->mb_half_result_srv};
      apply(kMbComposite, cmp, 2, d->mb_output_uav);
      cl->dispatch((W + 7u) / 8u, (H + 7u) / 8u, 1u);
    }
    bar(d->mb_output_texture, UA, SR);
  } else {
    // Nothing was produced, so the caller must not publish. Returning an empty
    // view makes the deploy callback skip the t0 push, leaving the game's
    // tonemap reading the original image � which is exactly the isolation
    // "Prep Only" is for.
    return {};
  }
  return d->mb_output_srv;
}

static void LogMotionBlurOnce(DeviceData* d, const std::string& message) {
  if (!d || !MotionBlurLogEnabled()) return;
  if (d->frame_index < d->mb_last_log_frame + kMotionBlurLogFrameGap) return;
  d->mb_last_log_frame = d->frame_index;
  reshade::log::message(reshade::log::level::info, ("[MotionBlur] " + message).c_str());
}

// -- Motion Blur cutscene gate --
// The DoF gather draws only when the game dispatches depth of field, which in
// Sora 2nd it does only in cutscenes, so "the DoF gather drew" IS the cutscene
// signal and nothing has to be kept in sync with a separate flag. frame_index is
// incremented once per present (addon.cpp OnPresent), so every draw inside one
// frame compares equal.
//
// The tonemap is dispatched after the DoF chain, so gating there still puts
// motion blur on top of depth of field. This callback only records the frame: it
// never touches D3D state and never dispatches anything.
static void OnDrawnDofGather(reshade::api::command_list* cmd_list) {
  if (!cmd_list || !IsMotionBlurGame()) return;
  if (auto* dev = cmd_list->get_device()) {
    if (auto* d = dev->get_private_data<DeviceData>()) {
      d->mb_dof_drew_frame = d->frame_index;
    }
  }
}

// -- Motion Blur deploy point (both modes) --
// One deploy serves both modes. The gather result replaces the tonemap's t0 and
// the game's own blit/tonemap runs on it, so the tonemap stays bit-identical to
// vanilla and cannot regress. Mode only changes the trigger:
//
//   1 Cutscene Only : run, but only on frames whose DoF gather drew
//   2 Always On     : run every frame
//
// Being post-TAA means TAA never accumulates over the blur; being pre-tonemap
// means the blur is computed on true HDR linear values. The game's original t0
// view supplies both the gather's colour input and the exact output format,
// because the result is pushed back at t0 and must stay format compatible with
// what the game's shader expects.
static bool OnBeforeTonemapDraw(reshade::api::command_list* cmd_list) {
  if (!cmd_list) return true;
  if (!IsMotionBlurGame()) return true;
  auto* dev = cmd_list->get_device();
  if (!dev) return true;
  auto* d = dev->get_private_data<DeviceData>();
  if (!d) return true;

  // Per-channel activation. Each channel runs on its own mode: Always On every
  // frame, Cutscene Only only on frames whose DoF gather drew. The chain runs
  // when EITHER channel is live and produces ONE gather from the blend, so
  // enabling both costs the same as enabling one.
  //
  // This replaces the single pre-split gate, which had to be evaluated before the
  // device was fetched because it did not need one. Reading the DoF frame does.
  // The reorder is safe: nothing between the old and new position has a side
  // effect. A stale frame index simply fails the comparison, so an unexpected
  // draw order degrades to "no blur" rather than to blur in the wrong place.
  const bool cutscene = (d->mb_dof_drew_frame == d->frame_index);
  if (!MotionBlurActive(cutscene)) return true;

  if (d->mb_tonemap_src_srv.handle == 0u || !d->mb_tonemap_src_live.load()) {
    if (MotionBlurLogEnabled() && !d->mb_warned_tonemap_src) {
      d->mb_warned_tonemap_src = true;
      // Report the hash the engine ACTUALLY reported, not the one we registered.
      // Reporting both is what separates the real causes, because they look
      // identical from outside:
      //   hash matches -> the hash gate is correct, so the capture never saw a
      //     push descriptor for this shader at all. Check the fast-path gate at the
      //     top of OnPushDescriptorsCapture first: it skips the whole body unless
      //     some other feature wants it, and that gate once left motion blur out
      //     entirely. Only after that is ruled out does "binds through a
      //     descriptor table" become the remaining explanation, which is NOT
      //     recoverable by widening a gate -- a D3D11 descriptor table is an
      //     opaque CPU-side handle array with no read-back path.
      //   hash differs -> the deploy matched by registration while the engine
      //     reports something else, so IsMotionBlurDeployHash tests the wrong value.
      uint32_t seen = 0u;
      if (auto* ss = renodx::utils::shader::GetCurrentState(cmd_list)) {
        seen = renodx::utils::shader::GetCurrentPixelShaderHash(ss);
      }
      std::ostringstream seenHex;
      seenHex << std::hex << seen;
      std::ostringstream wantHex;
      wantHex << std::hex << (IsKai() ? kKaiTonemapHash : kSoraTonemapHash);
      reshade::log::message(reshade::log::level::warning,
          ("[MotionBlur] needs the tonemap's t0 view, which has not been captured. "
           "Pixel hash 0x" + seenHex.str() + ", deploy gate expects 0x"
           + wantHex.str()
           + ". A matching hash means no push descriptor was captured for this "
             "shader, not that the hash is wrong: check the fast-path gate in "
             "OnPushDescriptorsCapture, and only then suspect descriptor tables.")
             .c_str());
    }
    return true;
  }
  d->mb_warned_tonemap_src = false;

  reshade::api::resource srcRes = dev->get_resource_from_view(d->mb_tonemap_src_srv);
  if (!srcRes.handle) return true;
  auto desc = dev->get_resource_desc(srcRes);
  if (desc.type != reshade::api::resource_type::texture_2d || desc.texture.samples != 1) return true;
  uint32_t w = desc.texture.width, h = desc.texture.height;
  if (w == 0u || h == 0u || desc.texture.format == reshade::api::format::unknown) return true;

  reshade::api::resource_view motionSrc;
  if (!PrepareMotionBlur(dev, d, w, h, RCASLinearFormat(desc.texture.format), &motionSrc)) {
    // With two selectable sources and no fallback, "unavailable" is not actionable
    // on its own: the useful fact is WHICH source is missing, because that is what
    // tells you whether the GBuffer pass is not writing four targets or the TAA
    // draw has not run.
    if (d && MBMotionInputView(d).handle == 0u) {
      LogMotionBlurOnce(d, std::string("inactive: ") + MBMotionInputName()
                                + " motion view not captured this frame");
    } else {
      LogMotionBlurOnce(d, "inactive: depth or pipelines unavailable");
    }
    return true;
  }
  auto blurred = RunMotionBlur(cmd_list, d, d->mb_tonemap_src_srv, motionSrc);
  if (!blurred.handle) return true;

  cmd_list->push_descriptors(
      reshade::api::shader_stage::pixel, reshade::api::pipeline_layout{0}, 0,
      reshade::api::descriptor_table_update{{}, 0u, 0, 1,
          reshade::api::descriptor_type::texture_shader_resource_view, &blurred});
  // Passing cutscene=true asks "is the mode configured to ever run", which is the
  // right question for a log line, unlike the per-frame gate above.
  const char* mbName = !MotionBlurActive(true) ? "off"
                     : (shader_injection.mb_mode >= 1.5f ? "always-on" : "cutscene-only");
  const char* stage = shader_injection.mb_debug_chain < 0.5f ? "full"
                     : (shader_injection.mb_debug_chain < 1.5f ? "prep-only" : "gather-only");
  LogMotionBlurOnce(d, std::string("active: mode=") + mbName
      + " MB under the game's tonemap, chain=" + stage);
  return true;
}

// -- Kai SSR Replacement (fused march + temporal composites, High + Ultra) --
// Master gate mirrors the Sora gates (shared toggles), minus any mrt capture
// requirement: each composite reads mrt0 from the game-bound t2, exactly like
// the vanilla pass it replaces, so no capture can starve serving. Only one
// quality variant runs at a time; the gate is identical for both.
static bool KaiSSRReplaceActive(reshade::api::command_list* cmd_list) {
  if (!cmd_list) return false;
  if (shader_injection.dynCube_ssr_replacement < 0.5f) return false;
  if (shader_injection.dynCube_enabled < 0.5f
      || shader_injection.dynCube_force_vanilla > 0.5f
      || shader_injection.dynCube_debug == 4.f) return false;
  auto* dev = cmd_list->get_device();
  if (!dev) return false;
  auto* dd = dev->get_private_data<DeviceData>();
  if (!dd) return false;
  if (!dd->captured_vanilla_env_srv.handle) return false;
  reshade::api::resource_view t17srv = dd->dyncube_ggx_valid
      ? dd->dyncube_ggx_out_cube_srv[dd->dyncube_ggx_active]
      : dd->dyncube_srv;
  if (!t17srv.handle) return false;
  return true;
}

// kai ssr vanilla-improvement gate: the tree files must execute (instead of
// game bytecode) whenever any Vanilla SSR Improvements toggle changes vanilla
// behavior � refine threshold, fixed history, disocclusion reject, or usable
// IS-FAST distribution. All off = bytecode, bit-exact. (The replacement chain
// serves via KaiSSRReplaceActive separately.)
static bool KaiSSRVanillaActive(reshade::api::command_list* cmd_list) {
  if (!cmd_list) return false;
  if (shader_injection.dynCube_ssr_replacement > 0.5f) return false;
  if (shader_injection.dynCube_enabled < 0.5f
      || shader_injection.dynCube_force_vanilla > 0.5f
      || shader_injection.dynCube_debug == 4.f) return false;
  if (shader_injection.dynCube_vanilla_ssr_enabled < 0.5f) return false;
  if (shader_injection.dynCube_vanilla_refine_fix > 0.5f) return true;
  if (shader_injection.dynCube_vanilla_history_fixed > 0.5f) return true;
  if (shader_injection.dynCube_vanilla_history_weight < 0.8999f
      || shader_injection.dynCube_vanilla_history_weight > 0.9001f) return true;
  if (shader_injection.dynCube_vanilla_disoc_reject > 0.5f) return true;
  if (shader_injection.dynCube_vanilla_isfast > 0.5f && g_isfast_enabled > 0.5f) {
    auto* dev = cmd_list->get_device();
    auto* dd = dev ? dev->get_private_data<DeviceData>() : nullptr;
    if (dd && dd->isfast_noise_srv.handle) return true;
  }
  return false;
}
// kai ssr: push everything the composite PS needs (game binds
// t0/t1/t2/t3/t4/s0/s1/b0/b2). No custom-SSR pushes: each composite resolves
// the inline vanilla march, never t31. Pushes when the composite serves or
// vanilla improvements are active; otherwise vanilla draws untouched.
static bool OnBeforeKaiSSRDraw(reshade::api::command_list* cmd_list) {
  if (!KaiSSRReplaceActive(cmd_list) && !KaiSSRVanillaActive(cmd_list)) return true;
  auto* dev = cmd_list->get_device();
  if (!dev) return true;
  auto* dd = dev->get_private_data<DeviceData>();
  if (!dd) return true;
  reshade::api::resource_view t17srv = dd->dyncube_ggx_valid
      ? dd->dyncube_ggx_out_cube_srv[dd->dyncube_ggx_active]
      : dd->dyncube_srv;
  if (t17srv.handle) {
    cmd_list->push_descriptors(
        reshade::api::shader_stage::pixel,
        reshade::api::pipeline_layout{0}, 0,
        reshade::api::descriptor_table_update{
            {}, kDynCubeRegister, 0, 1,
            reshade::api::descriptor_type::texture_shader_resource_view,
            &t17srv});
  }
  if (dd->dyncube_hist[dd->dyncube_readSet].pos_cube_srv.handle) {
    auto histPosSrv = dd->dyncube_hist[dd->dyncube_readSet].pos_cube_srv;
    cmd_list->push_descriptors(
        reshade::api::shader_stage::pixel,
        reshade::api::pipeline_layout{0}, 0,
        reshade::api::descriptor_table_update{
            {}, kDynCubeHistPosRegister, 0, 1,
            reshade::api::descriptor_type::texture_shader_resource_view,
            &histPosSrv});
  }
  cmd_list->push_descriptors(
      reshade::api::shader_stage::pixel,
      reshade::api::pipeline_layout{0}, 0,
      reshade::api::descriptor_table_update{
          {}, kDynCubeVanillaRegister, 0, 1,
          reshade::api::descriptor_type::texture_shader_resource_view,
          &dd->captured_vanilla_env_srv});
  // Vanilla-ISFAST frame slice (exact frame_index % 64 mirror of the custom
  // march; -1 = noise unusable -> the shader falls back to hash behavior).
  // IS-FAST noise volume (t5) only when usable; the shader gates sampling on it.
  {
    const bool noiseUsable = g_isfast_enabled > 0.5f && dd->isfast_noise_srv.handle;
    shader_injection.dynCube_vanilla_isfast_frame = noiseUsable ? (float)(dd->frame_index % 64u) : -1.f;
    if (noiseUsable && shader_injection.dynCube_vanilla_isfast > 0.5f) {
      cmd_list->push_descriptors(
          reshade::api::shader_stage::pixel,
          reshade::api::pipeline_layout{0}, 0,
          reshade::api::descriptor_table_update{
              {}, 5u, 0, 1,
              reshade::api::descriptor_type::texture_shader_resource_view,
              &dd->isfast_noise_srv});
    }
  }
  return true;
}

// kai ssr code replacement gate: false keeps the vanilla shader (still draws).
// Serves the tree files for the replacement composite and for vanilla temporal
// improvements alike; the files select their path in-shader.
static bool OnReplaceKaiSSRDraw(reshade::api::command_list* cmd_list) {
  return KaiSSRReplaceActive(cmd_list) || KaiSSRVanillaActive(cmd_list);
}

static bool OnBeforeLightingShaderDraw(reshade::api::command_list* cmd_list) {
  // IMPORTANT: returning false would BYPASS the draw (skip it entirely).
  shader_injection.gtvbao_dedicated_bound = 0.f;
  SyncISFASTToShaderInjection(cmd_list);  // keep IS-FAST mirrors in sync
  // Push IS-FAST noise texture for PCSS shadow jitter (t24 only � NO sampler push,
  // uses game's samPoint_s at s0 with manual wrap in shader to avoid heap corruption)
  if (g_isfast_enabled > 0.5f) {
    if (auto* dev = cmd_list->get_device()) {
      if (auto* dd = dev->get_private_data<DeviceData>()) {
        if (dd->isfast_noise_srv.handle) {
          cmd_list->push_descriptors(
              reshade::api::shader_stage::pixel,
              reshade::api::pipeline_layout{0}, 0,
              reshade::api::descriptor_table_update{
                  {}, 24u, 0, 1,
                  reshade::api::descriptor_type::texture_shader_resource_view,
                  &dd->isfast_noise_srv});
        }
      }
    }
  }

  // -- Kai sync: Character VBGI master toggle + PCSS jitter --
  shader_injection.char_gi_enabled = (g_char_vbgi_composite_method >= 0.5f) ? 1.f : 0.f;
  shader_injection.shadow_isfast_jitter_amount = shader_injection.shadow_pcss_jitter_amount;
  shader_injection.shadow_isfast_jitter_speed = shader_injection.shadow_pcss_jitter_speed;
  // Zero out jitter when IS-FAST master is off
  if (g_isfast_enabled < 0.5f) {
    shader_injection.shadow_isfast_jitter_amount = 0.f;
    shader_injection.shadow_isfast_jitter_speed = 0.f;
  }
  // Sync Kai debug views from shared settings
  shader_injection.gtvbao_debug_mode = shader_injection.gtvbao_debug_view;
  shader_injection.foliage_debug_mode = shader_injection.debug_show_env_sss;

  // Contact / Micro Shadows. Placed here, before the GTVBAO/DynCube early-out
  // below, because those two are entirely independent of the shadow passes: on a
  // frame where only the shadows are enabled this hook must still run them.
  DeployShadows(cmd_list, 1);

  // Dynamic Cubemaps � standalone, must run even when GTVBAO/SSR off (any Falcom title)
  const bool dyncube_active = shader_injection.dynCube_enabled > 0.5f;
  const bool gtvbao_active = shader_injection.gtvbao_mode > 0.5f;
  // Rising/falling-edge latch (runs before the early-out so it also happens when every
  // feature is off): rising edge schedules an immediate refresh cycle WITHOUT wiping
  // history (toggle off/on preserves the cache; first boot still clears via Create).
  // Falling edge only updates the latch � resources and cache are intentionally kept
  // so re-enabling resumes instantly. (No destroy-on-disable by design.)
  auto* dd0 = cmd_list ? cmd_list->get_device()->get_private_data<DeviceData>() : nullptr;
  if (dd0) {
    if (dyncube_active && !dd0->dyncube_was_enabled) {
      dd0->dyncube_phase = DeviceData::DynCubePhase::Capture;
      dd0->dyncube_next_update_frame = 0;
    }
     dd0->dyncube_was_enabled = dyncube_active;
  }
  if (!gtvbao_active && !dyncube_active) return true;
  if (!cmd_list) return true;

  auto* dev = cmd_list->get_device();
  auto* dd = dev ? dev->get_private_data<DeviceData>() : nullptr;
  if (!dd) return true;

  // -- Deferred dispatch path: capture snapshots for OnPresent (kai-style). --
  // Plain copies only (no queries): resource pairs + live flags travel with the views.
  // GTVBAO-only: with the mode off there is nothing to defer to.
  if (g_cpuopt_deferred_dispatch > 0.5f && shader_injection.gtvbao_mode > 0.5f) {
    dd->deferred_depth_srv = dd->captured_depth_srv;
    dd->deferred_depth_res = dd->captured_depth_res;
    dd->defDepthLive = dd->captured_depth_live.load();
    // The shadow passes read the same SSAO view and run from the same hook, so
    // the snapshot has to carry it too -- otherwise the deferred restore would
    // hand them a handle from an arbitrary earlier frame.
    dd->deferred_ssao_srv = dd->captured_ssao_srv;
    dd->deferred_mrt_normal_srv = dd->captured_mrt_normal_srv;
    dd->deferred_mrt_res = dd->captured_mrt_res;
    dd->defMrtLive = dd->captured_mrt_live.load();
    dd->deferred_scene_cbv_view = dd->captured_scene_cbv_view;
    dd->deferred_cbv_res = dd->captured_cbv_res;
    dd->defCbvLive = dd->captured_cbv_live.load();
    dd->deferred_scene_cbv = dd->captured_scene_cbv;
    dd->deferred_scene_cbv_valid = dd->captured_scene_cbv_valid;
    dd->deferred_scene_cbv_frame = dd->captured_scene_cbv_frame;
    dd->deferred_pending = true;
  }

  // -- Inline dispatch: Run GTVBAO on this frame's command list (only when NOT deferred). --
  if (gtvbao_active && g_cpuopt_deferred_dispatch < 0.5f) {
    if (dd->captured_depth_srv.handle && dd->captured_scene_cbv_valid
        && dd->ao_term_a_srv.handle) {
      auto* cs = renodx::utils::state::GetCurrentState(cmd_list);
      renodx::utils::state::CommandListState prev = {};
      if (cs) prev = *cs;

      const std::string liveDepth = dd->captured_depth_srv.handle ? dd->captured_depth_dims : "none";
      const std::string wantDims = std::to_string(dd->working_width) + "x" + std::to_string(dd->working_height);
      CSLog("gtvbao", std::string("inline invoke working=") + wantDims + " liveDepth=" + liveDepth,
        liveDepth != wantDims);
      bool ok = RunGTVBAO(cmd_list, dd);
      CSLog("gtvbao", std::string("inline invoke exit") + (ok ? " ok" : " FAILED"), !ok);

      ApplyGTVBAOCSDispatchFix(cmd_list, cs, prev);
      (void)ok;
    }
  }

  // Push the GTVBAO AO result at t22.
  // In inline mode: fresh from dispatch above.
  // In deferred mode: result from previous frame's OnPresent dispatch.
  // The buffer is tracked by RunGTVBAO (gtvbao_final_in_b) � parity depends on
  // the active denoiser path (legacy / R2 two-stage / �-trous).
  if (gtvbao_active) {
    // Half mode: lighting sees the reconstructed full-res AO (same t22 slot).
    const bool half_active = shader_injection.gtvbao_resolution > 0.5f
        && dd->upscale_ao_srv.handle;
    reshade::api::resource_view srv = half_active
        ? dd->upscale_ao_srv
        : (dd->gtvbao_final_in_b ? dd->ao_term_b_srv : dd->ao_term_a_srv);
    if (srv.handle) {
      cmd_list->push_descriptors(
          reshade::api::shader_stage::pixel,
          reshade::api::pipeline_layout{0},
          0,
          reshade::api::descriptor_table_update{
              {}, kLightingGtvbaoRegister, 0, 1,
              reshade::api::descriptor_type::texture_shader_resource_view, &srv,
          });
      shader_injection.gtvbao_dedicated_bound = 1.f;
    }
  }

  // -- SSGI push t23 (GI is produced by RunGTVBAO) --
  if (gtvbao_active) {
  shader_injection.gtvbao_vbgi_bound = 0.f;
  shader_injection.gtvbao_vbgi_debug = 0.f;
  // Reset every frame: the lighting shader reads this to decide whether the
  // t23 push is a diagnostic texture it should display. Leaving it set would
  // make a later frame show a stale debug view.
  shader_injection.gtvbao_mrt_normal_debug = 0.f;

  // Determine what to push to t23.
  reshade::api::resource_view push_srv = {};
  bool do_push = false;
  bool debug_replace = false;
  // Set when the pushed texture is the GTVBAO compute pass's own debug UAV
  // (bitmask views 6-8, MRT/horizon diagnostics 22-25). The lighting shader
  // displays it on this flag rather than on vbgi_debug_view, which is a
  // persistent user setting and must not be overwritten to make the push
  // visible.
  bool push_is_gtvbao_debug = false;

  // VBGI debug views (1=Raw GI, 2=Denoised GI, 3=Light Buffer, 4=Accumulated, 5=Samples).
  // Half mode: raw/denoised views show the half buffers; the normal path
  // shows the reconstructed full-res GI (same t23 slot as Full mode).
  const bool half_gi_active = shader_injection.gtvbao_resolution > 0.5f
      && dd->upscale_gi_srv.handle;
  // Upscale reconstruction diagnostics are lowest priority so an explicitly
  // requested VBGI/GTVBAO debug view is never silently overridden.
  // Sora/Kyoto replace via gtvbao_vbgi_debug; Kai/Daybreak2 via the widened
  // vbgi_debug_view || gtvbao_upscale_debug checks in their lighting shaders.
  const bool upscale_dbg_active = half_gi_active
      && shader_injection.gtvbao_upscale_debug > 0.5f
      && dd->debug_srv.handle;
  if (shader_injection.vbgi_debug_view > 0.5f) {
    int dv = (int)shader_injection.vbgi_debug_view;
    if (dv == 1)      push_srv = dd->vbgi_output_srv;
    else if (dv == 2) push_srv = GTVBAO_GiHalfChainReady(dd, half_gi_active)
        ? dd->vbgi_denoised_half_srv : dd->vbgi_denoised_srv;
    else if (dv == 3) push_srv = dd->captured_color_srv.handle
        ? dd->captured_color_srv : dd->captured_light_buffer_srv;
    else if (dv == 4) push_srv = dd->multibounce_srv.handle
        ? dd->multibounce_srv : dd->fallback_srv;
    else if (dv == 5) push_srv = dd->debug_srv.handle
        ? dd->debug_srv : dd->fallback_srv;
    do_push = true;
    debug_replace = true;
  }
  // Bitmask debug views 6-8, and MRT/horizon diagnostics 22-29: push the
  // dedicated debug UAV output to the lighting shader's VBGI debug slot.
  else if ((shader_injection.gtvbao_debug_view > 5.5f && shader_injection.gtvbao_debug_view < 8.5f)
        || (shader_injection.gtvbao_debug_view > 21.5f && shader_injection.gtvbao_debug_view < 29.5f)) {
    push_srv = dd->debug_srv;
    do_push = true;
    debug_replace = true;
    push_is_gtvbao_debug = true;
  }
  else if (upscale_dbg_active) {
    push_srv = dd->debug_srv;
    do_push = true;
    debug_replace = true;
  }
  // Normal SSGI: push denoised GI (reconstructed full-res in Half mode).
  else if (shader_injection.vbgi_enabled > 0.5f) {
    push_srv = half_gi_active ? dd->upscale_gi_srv : dd->vbgi_denoised_srv;
    do_push = true;
  }

  if (do_push) {
    if (!push_srv.handle) push_srv = dd->fallback_srv;
    if (push_srv.handle) {
      uint32_t giRegister = IsKai() ? 23u : kLightingVbgiRegister;  // Kai uses t23 for GTVBAO VBGI
      cmd_list->push_descriptors(
          reshade::api::shader_stage::pixel,
          reshade::api::pipeline_layout{0},
          0,
          reshade::api::descriptor_table_update{
              {}, giRegister, 0, 1,
              reshade::api::descriptor_type::texture_shader_resource_view,
              &push_srv,
          });
      shader_injection.gtvbao_vbgi_bound = 1.f;
      if (debug_replace) shader_injection.gtvbao_vbgi_debug = 1.f;
      if (push_is_gtvbao_debug) shader_injection.gtvbao_mrt_normal_debug = 1.f;
    }
    // VBGI debug logging.
    if (shader_injection.vbgi_debug_logging > 0.5f) {
      std::string msg = "[SSGI] t23 push: srv=";
      msg += push_srv.handle ? "valid" : "FALLBACK";
      msg += " debug=" + std::to_string(debug_replace ? 1 : 0);
      msg += " vbgi_enabled=" + std::to_string((int)shader_injection.vbgi_enabled);
      reshade::log::message(reshade::log::level::info, msg.c_str());
    }
  }
  }  // gtvbao_active (t23 section)

  // -- Dynamic Cubemaps (Sora2nd): t17 override � Phase 0A/B + Phase 1/2 standalone --
  // D3D11 hazard: capture writes UAV, then lighting reads SRV. RunDynCube* does
  // UAV->SRV barrier. We dispatch inline here before the draw that consumes t17.
  if (dyncube_active) {
    int dbg = (int)shader_injection.dynCube_debug;
    bool forceVanilla = (shader_injection.dynCube_force_vanilla > 0.5f);
    // Force Vanilla OFF: allow dynamic cubemap to override t17
    // Force Vanilla ON: keep vanilla game cubemap for t17 (A/B test), but still run capture/history/inference in background
    bool overrideT17 = (dbg != 4) && !forceVanilla;
    // Ensure resources: size from setting.
    // First activation creates immediately; a SIZE CHANGE defers the destroy+create to
    // OnPresent (frame boundary) so the old set is released when the GPU is idle
    // (D3D11 deferred-release leak avoidance).
    uint32_t wantSize = DynCubeResolveSize(shader_injection.dynCube_resolution);
    if (!dd->dyncube_resources_created) {
      CreateDynCubeResources(dev, dd, wantSize);
      // After create, pipelines may need re-creation
      CreateDynCubePipelinesIfNeeded(dev, dd);
    } else if (dd->dyncube_size != wantSize) {
      dd->dyncube_pending_size = wantSize;
      dd->dyncube_pending_recreate = true;
      if (shader_injection.dynCube_debug_logging > 0.5f) {
        reshade::log::message(reshade::log::level::info,
          (std::string("[DynCube] resize pending ") + std::to_string(dd->dyncube_size) +
           " -> " + std::to_string(wantSize)).c_str());
      }
    } else if (!dd->dyncube_solid_pipeline.handle || !dd->dyncube_capture_pipeline.handle) {
      CreateDynCubePipelinesIfNeeded(dev, dd);
    }

    // -- Multi-frame Dynamic Cubemap scheduler � runs ONCE per present frame --
    if (dd->dyncube_sched_frame != dd->frame_index) {
      dd->dyncube_sched_frame = dd->frame_index;
      const uint32_t interval = std::max(1u, (uint32_t)std::clamp(shader_injection.dynCube_capture_interval, 1.f, 16.f));

      // Execution context for crash tracing (edge-only): photo mode rendering on
      // a deferred list would run all of the below dispatches there.
      CSLog("ctx", std::string("scheduler on ") +
        ((dd->immediate_cmd_list != nullptr && dd->immediate_cmd_list == cmd_list) ? "immediate" : "DEFERRED"));

      // -- Loading wipe: consumes the OnPresent loading signal (this scheduler only
      // ticks on lighting draws, so the stale predicate itself is evaluated there).
      // Deletes the whole temporal cache once per stale episode so the next scene
      // seeds from scratch: zeroed history reads as invalid (serves fall back to
      // vanilla), the expand-only world-box re-latches via the existing reset path,
      // and validation/filter rebuild from the first post-load capture. Custom SSR
      // is stateless (march+blur from live inputs) so there is no SSR cache to clear.
      if (dd->dyncube_loadingWipePending && dd->dyncube_resources_created) {
        dd->dyncube_loadingWipePending = false;
        float zero4[4] = {0, 0, 0, 0};
        float zero1[4] = {0, 0, 0, 0};
        for (auto& set : dd->dyncube_hist) {
          if (set.color_uav.handle) cmd_list->clear_unordered_access_view_float(set.color_uav, zero4);
          if (set.pos_uav.handle) cmd_list->clear_unordered_access_view_float(set.pos_uav, zero4);
          if (set.contrib_uav.handle) cmd_list->clear_unordered_access_view_float(set.contrib_uav, zero1);
        }
        for (auto& u : dd->dyncube_cam_uav) if (u.handle) cmd_list->clear_unordered_access_view_float(u, zero4);
        dd->dyncube_hasValidRead = false;
        dd->dyncube_filteredReadSet = 99u;
        dd->dyncube_ggx_valid = false;
        dd->dyncube_variant_valid = false;
        dd->dyncube_boxCopyPending = false;
        dd->dyncube_wasRejected = false;
        dd->dyncube_rejectedGap = false;
        dd->dyncube_dirtyFastForward = false;
        dd->dyncube_needs_reset = true;
        dd->dyncube_worldbox_reset_pending = true;
        dd->dyncube_next_update_frame = 0;
            if (shader_injection.dynCube_debug_logging > 0.5f) {
              reshade::log::message(reshade::log::level::info, "[DynCube] loading wipe: temporal cache cleared");
            }
            CSLog("dyncube", "loading wipe applied");
      }

      // Delayed-validate commit: consume the staged hasGeom bit BEFORE any new
      // capture/filter work, so promotion always pairs with the latest capture.
      ConsumeDynCubeStagedValidity(dev, dd);

      // Capture-content settings dirtiness: character_capture bakes into history
      // texels, but the served cube only rebuilds on readSet change. A toggle in a
      // static scene would otherwise never reach t17. Force one filter pass +
      // one-shot history fast-forward so feedback is immediate, then resume no-churn.
      // Gated on effective t17 serving so untouched paths never pay for it.
      // (Soften/strength drive the variant cube instead � tracked separately below;
      // capture brightness stays sample-time by design and is not tracked here.)
      {
        bool serveT17 = !forceVanilla && dbg != 4;
        float curCharCap = (shader_injection.dynCube_character_capture > 0.5f) ? 1.0f : 0.0f;
        float curSparkleReject = (shader_injection.dynCube_sparkle_rejection > 0.5f) ? 1.0f : 0.0f;
        if (serveT17 && curCharCap != dd->dyncube_lastCharCapture) {
          dd->dyncube_captureDirty = true;
          dd->dyncube_dirtyFastForward = true;
        }
        if (serveT17 && curSparkleReject != dd->dyncube_lastSparkleRejection) {
          dd->dyncube_captureDirty = true;
          dd->dyncube_dirtyFastForward = true;
        }
      }

      // SSR runs every frame, independent of the Dynamic Cubemap update interval.
      if (shader_injection.dynCube_ssr_enabled > 0.5f) {
        (void)RunDynCubeSSR(cmd_list, dd);
      }

      // Seed: on first activation (no completed cube yet) capture immediately; the
      // filter is deferred to the gated Filter phase so an unvalidated first capture
      // can never bake the GGX cubes (costs ~1 frame of first-image latency).
      if (!dd->dyncube_ggx_valid && RunDynCubeCapture(cmd_list, dd)) {
        dd->dyncube_phase = DeviceData::DynCubePhase::Filter;
        dd->dyncube_next_update_frame = dd->frame_index + interval;
      }

      // State machine: Capture -> Filter -> Done -> wait for interval -> Capture.
      // A new capture never begins while a previous update is still in Capture/Filter.
      switch (dd->dyncube_phase) {
        case DeviceData::DynCubePhase::Done:
          if (dd->frame_index >= dd->dyncube_next_update_frame) dd->dyncube_phase = DeviceData::DynCubePhase::Capture;
          break;
        case DeviceData::DynCubePhase::Capture:
          if (RunDynCubeCapture(cmd_list, dd)) {
            dd->dyncube_next_update_frame = dd->frame_index + interval;
            dd->dyncube_phase = DeviceData::DynCubePhase::Filter;
          }
          break;
        case DeviceData::DynCubePhase::Filter:
          if (!dd->dyncube_hasValidRead) {
            // No validated read set yet (e.g. loading at boot): wait, retry on cadence.
            dd->dyncube_phase = DeviceData::DynCubePhase::Done;
          } else if (dd->dyncube_readSet == dd->dyncube_filteredReadSet && !dd->dyncube_captureDirty) {
            // Already filtered (frozen valid state): skip redundant work, no churn.
            dd->dyncube_phase = DeviceData::DynCubePhase::Done;
          } else {
            // Settings-dirty path: the forced filter must rebuild from the LATEST
            // capture, but readSet/aliases only advance on validated promotion (frozen
            // in static scenes). Promote first � but ONLY when the normal flow did not
            // just promote itself (readSet still equal): a second promote would flip
            // past the fresh set onto stale content. Gated on hasValidRead above, so
            // boot content can never be promoted; a mid-loader slider drag may
            // transiently alias loading content until the next validated promote
            // (self-healing, same exposure as gap-resume). Normal validation-gated
            // flow otherwise untouched.
            if (dd->dyncube_captureDirty && dd->dyncube_readSet == dd->dyncube_filteredReadSet)
              PromoteDynCubeReadSet(dd);
            if (RunDynCubeFilter(cmd_list, dd, (shader_injection.dynCube_ggx > 0.5f))) {
              dd->dyncube_ggx_active = 1u - dd->dyncube_ggx_active;
              dd->dyncube_ggx_valid = true;
              dd->dyncube_filteredReadSet = dd->dyncube_readSet;
              dd->dyncube_captureDirty = false;
              dd->dyncube_lastCharCapture = (shader_injection.dynCube_character_capture > 0.5f) ? 1.0f : 0.0f;
              dd->dyncube_lastSparkleRejection = (shader_injection.dynCube_sparkle_rejection > 0.5f) ? 1.0f : 0.0f;
              // Keep the global-push variant in sync with fresh filter output, but only
              // when wanted (soften/strength active); otherwise it stays invalid and the
              // sharp cube serves. Snapshots update inside the variant build.
              {
                float vSoften = std::clamp(shader_injection.dynCube_capture_soften, 0.f, 1.f);
                float vStrength = std::clamp(shader_injection.dynCube_global_strength, 0.f, 1.f);
                if (vSoften > 1e-4f || vStrength < 1.f - 1e-4f)
                  (void)RunDynCubeVariant(cmd_list, dd);
                else
                  dd->dyncube_variant_valid = false;
              }
              dd->dyncube_phase = DeviceData::DynCubePhase::Done;
            }
          }
          break;
      }

      // Variant refresh for global pushes (no capture/promote/filter churn): rebuild
      // the softened/dimmed cube from the current filtered cube whenever its two
      // settings drift. Lighting always samples sharp and is unaffected.
      {
        float vSoften = std::clamp(shader_injection.dynCube_capture_soften, 0.f, 1.f);
        float vStrength = std::clamp(shader_injection.dynCube_global_strength, 0.f, 1.f);
        bool wantVariant = (vSoften > 1e-4f || vStrength < 1.f - 1e-4f);
        if (dd->dyncube_ggx_valid && wantVariant
            && (vSoften != dd->dyncube_lastVariantSoften || vStrength != dd->dyncube_lastVariantStrength)) {
          // Self-healing: the variant cube is derived and never part of a full
          // Create, so it may be absent (first use, or a resize destroyed it) �
          // build it here. Guarded on the handle so this never re-creates.
          if (!dd->dyncube_variant.handle)
            CreateDynCubeVariantResources(dev, dd, dd->dyncube_size, dd->dyncube_mip_count);
          (void)RunDynCubeVariant(cmd_list, dd);
        }
      }

      // -- Throttled scheduler log (1/sec) � verify the state machine behavior --
      if (shader_injection.dynCube_debug_logging > 0.5f) {
        static auto last_log = std::chrono::steady_clock::now() - std::chrono::seconds(2);
        auto now = std::chrono::steady_clock::now();
        if (now - last_log >= std::chrono::seconds(1)) {
          last_log = now;
          const char* phaseName = (dd->dyncube_phase == DeviceData::DynCubePhase::Capture) ? "Capture"
                                : (dd->dyncube_phase == DeviceData::DynCubePhase::Filter) ? "Filter"
                                : "Done";
          std::string msg = "[DynCube] ";
          msg += "interval=" + std::to_string(interval);
          msg += " state=" + std::string(phaseName);
          msg += " active=" + std::string(dd->dyncube_ggx_active ? "B" : "A");
          msg += " frame=" + std::to_string(dd->frame_index);
          msg += " res=" + std::to_string(wantSize);
          msg += " ssr=" + std::to_string((int)shader_injection.dynCube_ssr_enabled);
          msg += " ggxValid=" + std::string(dd->dyncube_ggx_valid ? "1" : "0");
          msg += " capTot=" + std::to_string(dd->dyncube_capture_dispatches);
          msg += " rej=" + std::to_string(dd->dyncube_rejected_captures);
          msg += " rs=" + std::to_string(dd->dyncube_readSet);
          msg += " fltTot=" + std::to_string(dd->dyncube_filter_updates);
          msg += " ggxMips=" + std::to_string(dd->dyncube_ggx_mip_dispatches);
          msg += " faceCpy=" + std::to_string(dd->dyncube_face_copies);
          reshade::log::message(reshade::log::level::info, msg.c_str());
        }
      }
    }

    // Bind the history position cube (t29) � Dynamic validity source for the blend.
    // Reads the validated readSet (delayed-validate commit), never the raw write set.
    {
      reshade::api::resource_view histPosSrv = dd->dyncube_hist[dd->dyncube_readSet].pos_cube_srv;
      if (histPosSrv.handle) {
        cmd_list->push_descriptors(reshade::api::shader_stage::pixel, reshade::api::pipeline_layout{0}, 0,
          reshade::api::descriptor_table_update{{}, kDynCubeHistPosRegister, 0, 1,
            reshade::api::descriptor_type::texture_shader_resource_view, &histPosSrv});
      }
    }
    // Bind the game's vanilla cubemap (t30) for the SSR -> Dynamic -> Vanilla fallback.
    if (dd->captured_vanilla_env_srv.handle) {
      cmd_list->push_descriptors(reshade::api::shader_stage::pixel, reshade::api::pipeline_layout{0}, 0,
        reshade::api::descriptor_table_update{{}, kDynCubeVanillaRegister, 0, 1,
          reshade::api::descriptor_type::texture_shader_resource_view, &dd->captured_vanilla_env_srv});
    }
    // Bind the SSR result (t31 blurred) + raw (t32) � produced once per frame by the scheduler.
    if (shader_injection.dynCube_ssr_enabled > 0.5f && dd->dyncube_ssr_blur_srv.handle) {
      cmd_list->push_descriptors(reshade::api::shader_stage::pixel, reshade::api::pipeline_layout{0}, 0,
        reshade::api::descriptor_table_update{{}, kDynCubeSSRRegister, 0, 1,
          reshade::api::descriptor_type::texture_shader_resource_view, &dd->dyncube_ssr_blur_srv});
      if (dd->dyncube_ssr_raw_srv.handle) {
        cmd_list->push_descriptors(reshade::api::shader_stage::pixel, reshade::api::pipeline_layout{0}, 0,
          reshade::api::descriptor_table_update{{}, kDynCubeSSRRawRegister, 0, 1,
            reshade::api::descriptor_type::texture_shader_resource_view, &dd->dyncube_ssr_raw_srv});
      }
    }
    // Bind the captured vanilla ssr1 march result (t28) for the SSR replacement
    // debug view. Only needed for diagnostics; skipped when never captured or dead.
    if (dd->captured_ssr1_srv.handle && dd->captured_ssr1_live) {
      cmd_list->push_descriptors(reshade::api::shader_stage::pixel, reshade::api::pipeline_layout{0}, 0,
        reshade::api::descriptor_table_update{{}, 28u, 0, 1,
          reshade::api::descriptor_type::texture_shader_resource_view, &dd->captured_ssr1_srv});
    }

    if (dbg == 3) {
      // Solid face colors � validates t17 binding + handedness without capture.
      // Uses the DEDICATED solid cube so history resources are never touched.
      if (!forceVanilla && RunDynCubeSolid(cmd_list, dd)) {
        cmd_list->push_descriptors(reshade::api::shader_stage::pixel, reshade::api::pipeline_layout{0}, 0,
          reshade::api::descriptor_table_update{{}, kDynCubeRegister, 0, 1,
            reshade::api::descriptor_type::texture_shader_resource_view, &dd->dyncube_solid_cube_srv});
      }
    } else if (overrideT17 && (dbg != 0 || dd->captured_vanilla_env_srv.handle != 0u)) {
      // Push the ACTIVE completed filtered cube (never a partially-written building cube).
      // Before the first filter completes, fall back to the raw history cube.
      // Normal display additionally requires a captured vanilla cube (fallback must
      // exist before dynamic takes over); debug modes override regardless.
      reshade::api::resource_view t17srv = dd->dyncube_ggx_valid
          ? dd->dyncube_ggx_out_cube_srv[dd->dyncube_ggx_active]
          : dd->dyncube_srv;
      if (t17srv.handle) {
        cmd_list->push_descriptors(reshade::api::shader_stage::pixel, reshade::api::pipeline_layout{0}, 0,
          reshade::api::descriptor_table_update{{}, kDynCubeRegister, 0, 1,
            reshade::api::descriptor_type::texture_shader_resource_view, &t17srv});
      }
    }
  }

  return true;
}

static bool OnBeforeSsaoShaderDraw(reshade::api::command_list*) {
  // Used as on_replace callback via CustomShaderEntryCallback.
  // Return true = use our replacement SSAO shader (with GTVBAO gate).
  return true;
}

// -- Resource create / destroy --

static void CreateGTVBAOResources(reshade::api::device* dev, DeviceData* d,
                                   uint32_t gw, uint32_t gh) {
  DestroyGTVBAOResources(dev, d);
  // Always at full resolution.
  uint32_t w = gw;
  uint32_t h = gh;
  if (w < 64u) w = 64u;
  if (h < 64u) h = 64u;
  d->working_width = w; d->working_height = h;

  {
    reshade::api::sampler_desc sd = {};
    sd.filter = reshade::api::filter_mode::min_mag_mip_point;
    sd.address_u = sd.address_v = sd.address_w = reshade::api::texture_address_mode::clamp;
    dev->create_sampler(sd, &d->point_clamp_sampler);
  }
  {
    reshade::api::resource_desc rd = {};
    rd.type = reshade::api::resource_type::texture_2d;
    rd.texture = {w, h, 1, (uint16_t)kGTVBAODepthMipLevels, reshade::api::format::r32_float, 1};
    rd.heap = reshade::api::memory_heap::gpu_only;
    rd.usage = reshade::api::resource_usage::shader_resource | reshade::api::resource_usage::unordered_access;
    dev->create_resource(rd, nullptr, reshade::api::resource_usage::shader_resource, &d->depth_mips_texture);
    dev->create_resource_view(d->depth_mips_texture, reshade::api::resource_usage::shader_resource,
                               reshade::api::resource_view_desc(reshade::api::resource_view_type::texture_2d,
                                                                 reshade::api::format::r32_float, 0, kGTVBAODepthMipLevels, 0, 1),
                               &d->depth_mips_srv);
    for (uint32_t m = 0; m < kGTVBAODepthMipLevels; ++m)
      dev->create_resource_view(d->depth_mips_texture, reshade::api::resource_usage::unordered_access,
                                 reshade::api::resource_view_desc(reshade::api::resource_view_type::texture_2d,
                                                                   reshade::api::format::r32_float, m, 1, 0, 1),
                                 &d->depth_mips_uavs[m]);
  }

  auto mk = [&](uint32_t tw, uint32_t th, reshade::api::format fmt,
                reshade::api::resource* res, reshade::api::resource_view* srv,
                reshade::api::resource_view* uav) {
    reshade::api::resource_desc rd = {};
    rd.type = reshade::api::resource_type::texture_2d;
    rd.texture = {tw, th, 1, 1, fmt, 1};
    rd.heap = reshade::api::memory_heap::gpu_only;
    rd.usage = reshade::api::resource_usage::shader_resource | reshade::api::resource_usage::unordered_access;
    dev->create_resource(rd, nullptr, reshade::api::resource_usage::shader_resource, res);
    reshade::api::resource_view_desc vd(reshade::api::resource_view_type::texture_2d, fmt, 0, 1, 0, 1);
    if (srv) dev->create_resource_view(*res, reshade::api::resource_usage::shader_resource, vd, srv);
    if (uav) dev->create_resource_view(*res, reshade::api::resource_usage::unordered_access, vd, uav);
  };

  // �� Half-resolution spatial pipeline: ao/edges/raw-GI follow the AO evaluation
  // resolution; everything else stays full. Ceiling division (odd-safe).
  const bool half_mode = shader_injection.gtvbao_resolution > 0.5f;
  const uint32_t hw = (w + 1u) / 2u;
  const uint32_t hh = (h + 1u) / 2u;
  d->half_width = hw; d->half_height = hh;
  d->last_created_gtvbao_resolution = half_mode ? 1.f : 0.f;
  const uint32_t aw = half_mode ? hw : w;
  const uint32_t ah = half_mode ? hh : h;
  mk(aw, ah, reshade::api::format::r32_uint, &d->ao_term_a_texture, &d->ao_term_a_srv, &d->ao_term_a_uav);
  mk(aw, ah, reshade::api::format::r32_uint, &d->ao_term_b_texture, &d->ao_term_b_srv, &d->ao_term_b_uav);
  mk(w, h, reshade::api::format::r32_uint, &d->history_ao_texture_a, &d->history_ao_srv_a, &d->history_ao_uav_a);
  mk(w, h, reshade::api::format::r32_uint, &d->history_ao_texture_b, &d->history_ao_srv_b, &d->history_ao_uav_b);
  mk(aw, ah, reshade::api::format::r32_float, &d->edges_texture, &d->edges_srv, &d->edges_uav);
  mk(gw, gh, reshade::api::format::r8g8b8a8_unorm, &d->composite_texture, &d->composite_srv, &d->composite_uav);

  // -- GI resources (same resolution as AO per user preference) --
  mk(aw, ah, reshade::api::format::r16g16b16a16_float,
     &d->vbgi_output_texture, &d->vbgi_output_srv, &d->vbgi_output_uav);
  // Existing full-res denoised GI is always kept (Full path + debug use).
  mk(w, h, reshade::api::format::r16g16b16a16_float,
     &d->vbgi_denoised_texture, &d->vbgi_denoised_srv, &d->vbgi_denoised_uav);
  if (half_mode) {
    // Half-mode chain: raw half GI -> denoised half GI -> full-res reconstruction.
    mk(hw, hh, reshade::api::format::r16g16b16a16_float,
       &d->vbgi_denoised_half_texture, &d->vbgi_denoised_half_srv, &d->vbgi_denoised_half_uav);
    mk(w, h, reshade::api::format::r32_uint,
       &d->upscale_ao_texture, &d->upscale_ao_srv, &d->upscale_ao_uav);
    mk(w, h, reshade::api::format::r16g16b16a16_float,
       &d->upscale_gi_texture, &d->upscale_gi_srv, &d->upscale_gi_uav);
    // Texture/SRV/UAV are separate create calls; report partial failure
    // explicitly instead of letting the GI chain degrade silently later.
    if (!GTVBAO_GiHalfChainReady(d, true)
        || !d->upscale_ao_uav.handle || !d->upscale_gi_uav.handle) {
      reshade::log::message(reshade::log::level::error,
          "[GTVBAO] Half-res reconstruction resources incomplete � "
          "VBGI reconstruction will be unavailable in Half mode.");
    }
  }
  // Foliage mask (full-res R8_UINT)
  mk(w, h, reshade::api::format::r8_uint,
     &d->foliage_mask_texture, &d->foliage_mask_srv, &d->foliage_mask_uav);
  mk(w, h, reshade::api::format::r8g8b8a8_unorm,
     &d->debug_texture, &d->debug_srv, &d->debug_uav);
  // �-trous normal pre-decode target (full-res RGBA16F)
  mk(w, h, reshade::api::format::r16g16b16a16_float,
     &d->normal_prep_texture, &d->normal_prep_srv, &d->normal_prep_uav);
  // Light buffer capture at full back-buffer resolution
  mk(gw, gh, reshade::api::format::r16g16b16a16_float,
     &d->captured_light_buffer_texture, &d->captured_light_buffer_srv, nullptr);
  // Multi-bounce accumulation buffer (HDR, same resolution as working set)
  mk(w, h, reshade::api::format::r16g16b16a16_float,
     &d->multibounce_texture, &d->multibounce_srv, &d->multibounce_uav);
  d->vbgi_denoised_valid = false;
}

static void DestroyGTVBAOResources(reshade::api::device* dev, DeviceData* d) {
  if (!dev || !d) return;
  auto dv = [&](reshade::api::resource_view& v) { if (v.handle) { dev->destroy_resource_view(v); v = {}; } };
  auto dr = [&](reshade::api::resource& r) { if (r.handle) { dev->destroy_resource(r); r = {}; } };
  auto dp = [&](reshade::api::pipeline& p) { if (p.handle) { dev->destroy_pipeline(p); p = {}; } };
  auto dl = [&](reshade::api::pipeline_layout& l) { if (l.handle) { dev->destroy_pipeline_layout(l); l = {}; } };

  dv(d->depth_mips_srv); for (auto& u : d->depth_mips_uavs) dv(u); dr(d->depth_mips_texture);
  dv(d->ao_term_a_srv); dv(d->ao_term_a_uav); dr(d->ao_term_a_texture);
  dv(d->ao_term_b_srv); dv(d->ao_term_b_uav); dr(d->ao_term_b_texture);
  dv(d->history_ao_srv_a); dv(d->history_ao_uav_a); dr(d->history_ao_texture_a);
  dv(d->history_ao_srv_b); dv(d->history_ao_uav_b); dr(d->history_ao_texture_b);
  dv(d->edges_srv); dv(d->edges_uav); dr(d->edges_texture);
  dv(d->composite_srv); dv(d->composite_uav); dr(d->composite_texture);
  if (d->point_clamp_sampler.handle) { dev->destroy_sampler(d->point_clamp_sampler); d->point_clamp_sampler = {}; }
  dp(d->prefilter_pipeline); dp(d->main_low_pipeline); dp(d->main_medium_pipeline);
  dp(d->main_high_pipeline); dp(d->main_ultra_pipeline); dp(d->denoise_pipeline);
  dp(d->denoise_last_pipeline);
  dp(d->denoise_last_kai_pipeline);
  dp(d->denoise_last_sora2nd_pipeline);
  dl(d->prefilter_layout); dl(d->main_layout); dl(d->denoise_layout);
  DestroyGTVBAODescriptorTables(dev, &d->prefilter_tables);
  DestroyGTVBAODescriptorTables(dev, &d->main_tables);
  DestroyGTVBAODescriptorTables(dev, &d->denoise_tables);
  DestroyGTVBAODescriptorTables(dev, &d->foliage_mask_tables);
  // GI resources (now integrated � no separate VBGI pipeline)
  dv(d->vbgi_output_srv); dv(d->vbgi_output_uav); dr(d->vbgi_output_texture);
  dv(d->vbgi_denoised_srv); dv(d->vbgi_denoised_uav); dr(d->vbgi_denoised_texture);
  dv(d->vbgi_denoised_half_srv); dv(d->vbgi_denoised_half_uav); dr(d->vbgi_denoised_half_texture);
  dv(d->upscale_ao_srv); dv(d->upscale_ao_uav); dr(d->upscale_ao_texture);
  dv(d->upscale_gi_srv); dv(d->upscale_gi_uav); dr(d->upscale_gi_texture);
  dp(d->upscale_pipeline); dl(d->upscale_layout);
  DestroyGTVBAODescriptorTables(dev, &d->upscale_tables);
  dv(d->captured_light_buffer_srv); dr(d->captured_light_buffer_texture);
  dv(d->multibounce_srv); dv(d->multibounce_uav); dr(d->multibounce_texture);
  dv(d->foliage_mask_srv); dv(d->foliage_mask_uav); dr(d->foliage_mask_texture);
  dp(d->foliage_mask_pipeline); dl(d->foliage_mask_layout);
  dv(d->debug_srv); dv(d->debug_uav); dr(d->debug_texture);
  dp(d->multibounce_pipeline); dl(d->multibounce_layout);
  DestroyGTVBAODescriptorTables(dev, &d->multibounce_tables);
  // �-trous wavelet filter + normal pre-decode
  dp(d->atrous_pipeline); dl(d->atrous_layout);
  DestroyGTVBAODescriptorTables(dev, &d->atrous_tables);
  dp(d->normal_prep_pipeline); dl(d->normal_prep_layout);
  DestroyGTVBAODescriptorTables(dev, &d->normal_prep_tables);
  dv(d->normal_prep_srv); dv(d->normal_prep_uav); dr(d->normal_prep_texture);
  // IS-FAST noise
  dv(d->isfast_noise_srv); dr(d->isfast_noise_texture);
  if (d->isfast_sampler.handle) { dev->destroy_sampler(d->isfast_sampler); d->isfast_sampler = {}; }
  d->isfast_texture_loaded = false;
  d->isfast_texture_attempted = false;
  // Do NOT clear captured_depth_srv / captured_scene_cbv �
  // those reference game-owned resources that survive recreation.
  d->resources_created = false;
}

// ----------- Contact / Micro Shadows (shadows/*.cs_5_0.hlsl) -----------

// Rebuilds the two rgba8_unorm targets at the depth buffer's resolution. The depth
// buffer is the authority: both passes read it and both sample the G-buffer on the
// same texel grid, so a shadow output at any other size would have to be
// resampled and would land on a different texel than the geometry it describes.
//
// w/h are parameters rather than read back from d->shadows_w/h on purpose. The
// create path opens with DestroyShadowsResources, and destroy zeroes shadows_w/h
// as its last act -- so reading them here returned 0 every time and the targets
// came out 1x1. The symptom was self-contradictory from the outside: resources
// reported "ready", and the dispatch guard then refused the 0x0 grid.
static void CreateShadowsResources(reshade::api::device* dev, DeviceData* d,
                                   uint32_t w, uint32_t h) {
  if (!dev || !d) return;
  DestroyShadowsResources(dev, d);
  if (w < 8u || h < 8u) {
    CSLog("shadows", "refusing to create a shadow target smaller than 8x8", true);
    return;
  }

  auto mk = [&](reshade::api::resource* res, reshade::api::resource_view* srv,
                reshade::api::resource_view* uav) -> bool {
    reshade::api::resource_desc rd = {};
    rd.type = reshade::api::resource_type::texture_2d;
    // R8G8B8A8_UNORM is in the D3D11.0 guaranteed UAV type-write set. R8_UNORM and
    // R16_FLOAT are NOT -- they only join that set at feature level 11_1 / 12_0 --
    // and a rejected UAV write leaves the target holding whatever was there
    // before, which reads as a plausible image rather than as a failure. Every
    // failure below is logged by name, because this function used to return false
    // silently and the symptom was indistinguishable from "the effect does
    // nothing".
    rd.texture = {w, h, 1, 1, reshade::api::format::r8g8b8a8_unorm, 1};
    rd.heap = reshade::api::memory_heap::gpu_only;
    rd.usage = reshade::api::resource_usage::shader_resource
             | reshade::api::resource_usage::unordered_access;
    if (!dev->create_resource(rd, nullptr, reshade::api::resource_usage::shader_resource, res)) {
      CSLog("shadows", "create_resource failed", true);
      return false;
    }
    const reshade::api::resource_view_desc vd(
        reshade::api::resource_view_type::texture_2d, reshade::api::format::r8g8b8a8_unorm, 0, 1, 0, 1);
    if (srv && !dev->create_resource_view(*res, reshade::api::resource_usage::shader_resource, vd, srv)) {
      CSLog("shadows", "create_resource_view(SRV) failed", true);
      return false;
    }
    if (uav && !dev->create_resource_view(*res, reshade::api::resource_usage::unordered_access, vd, uav)) {
      CSLog("shadows", "create_resource_view(UAV) failed", true);
      return false;
    }
    return true;
  };

  if (!mk(&d->micro_shadow_texture, &d->micro_shadow_srv, &d->micro_shadow_uav)) {
    DestroyShadowsResources(dev, d);
    return;
  }
  if (!mk(&d->contact_shadow_texture, &d->contact_shadow_srv, &d->contact_shadow_uav)) {
    DestroyShadowsResources(dev, d);
    return;
  }
  d->shadows_resources_ready = true;
  // Recorded only on success, and never by the destroy path inside this function,
  // so the size that describes the live targets is the size they were made at.
  d->shadows_w = w;
  d->shadows_h = h;
}

static void DestroyShadowsResources(reshade::api::device* dev, DeviceData* d) {
  if (!dev || !d) return;
  auto dv = [&](reshade::api::resource_view& v) { if (v.handle) { dev->destroy_resource_view(v); v = {}; } };
  auto dr = [&](reshade::api::resource& r) { if (r.handle) { dev->destroy_resource(r); r = {}; } };
  auto dp = [&](reshade::api::pipeline& p) { if (p.handle) { dev->destroy_pipeline(p); p = {}; } };
  auto dl = [&](reshade::api::pipeline_layout& l) { if (l.handle) { dev->destroy_pipeline_layout(l); l = {}; } };

  dv(d->micro_shadow_srv); dv(d->micro_shadow_uav); dr(d->micro_shadow_texture);
  dv(d->contact_shadow_srv); dv(d->contact_shadow_uav); dr(d->contact_shadow_texture);
  if (d->shadows_point_clamp_sampler.handle) {
    dev->destroy_sampler(d->shadows_point_clamp_sampler);
    d->shadows_point_clamp_sampler = {};
  }
  for (uint32_t pass = 0; pass < kShadowsPassCount; ++pass) {
    dp(d->shadows_pipelines[pass]);
    dl(d->shadows_layouts[pass]);
    DestroyGTVBAODescriptorTables(dev, &d->shadows_tables[pass]);
  }
  d->shadows_resources_ready = false;
  d->shadows_w = 0u;
  d->shadows_h = 0u;
}

static bool CreateShadowsPipelinesIfNeeded(reshade::api::device* dev, DeviceData* d) {
  using DR = reshade::api::descriptor_range;
  using DS = reshade::api::shader_stage;
  using DT = reshade::api::descriptor_type;
  using P = reshade::api::pipeline_layout_param;
  if (!dev || !d) return false;
  static const std::span<const uint8_t> kBytecode[kShadowsPassCount] = {
      __micro_shadows, __contact_shadows};

  if (!d->shadows_point_clamp_sampler.handle) {
    reshade::api::sampler_desc sd = {};
    sd.filter = reshade::api::filter_mode::min_mag_mip_point;
    sd.address_u = sd.address_v = sd.address_w = reshade::api::texture_address_mode::clamp;
    if (!dev->create_sampler(sd, &d->shadows_point_clamp_sampler)) {
      CSLog("shadows", "create_sampler failed", true);
      return false;
    }
  }

  for (uint32_t pass = 0; pass < kShadowsPassCount; ++pass) {
    if (d->shadows_layouts[pass].handle == 0u) {
      DR sampler_r = {0,0,0,1,DS::all_compute,1,DT::sampler};
      DR cbv_r     = {0,0,0,1,DS::all_compute,1,DT::constant_buffer};
      DR srv_r     = {0,0,0,kShadowsSrvPerPass[pass],DS::all_compute,1,DT::texture_shader_resource_view};
      DR uav_r     = {0,0,0,kShadowsUavPerPass[pass],DS::all_compute,1,DT::texture_unordered_access_view};
      reshade::api::constant_range push_range = {};
      push_range.binding = 0;
      push_range.dx_register_index = 13;
      push_range.dx_register_space = 0;
      // The WHOLE ShaderInjectData, not a hand-mapped subset: the shadow shaders
      // include shared.h and read the cs_* fields directly, so there is no second
      // declaration that can drift from the C++ side.
      push_range.count = static_cast<uint32_t>(sizeof(ShaderInjectData) / sizeof(uint32_t));
      push_range.visibility = DS::all_compute;
      P param_sampler, param_cbv, param_srv, param_uav, param_constants;
      param_sampler.type = reshade::api::pipeline_layout_param_type::descriptor_table;
      param_sampler.descriptor_table.count = 1; param_sampler.descriptor_table.ranges = &sampler_r;
      param_cbv.type = reshade::api::pipeline_layout_param_type::descriptor_table;
      param_cbv.descriptor_table.count = 1; param_cbv.descriptor_table.ranges = &cbv_r;
      param_srv.type = reshade::api::pipeline_layout_param_type::descriptor_table;
      param_srv.descriptor_table.count = 1; param_srv.descriptor_table.ranges = &srv_r;
      param_uav.type = reshade::api::pipeline_layout_param_type::descriptor_table;
      param_uav.descriptor_table.count = 1; param_uav.descriptor_table.ranges = &uav_r;
      param_constants.type = reshade::api::pipeline_layout_param_type::push_constants;
      param_constants.push_constants = push_range;
      P params[5] = {param_sampler, param_cbv, param_srv, param_uav, param_constants};
      if (!dev->create_pipeline_layout(5, params, &d->shadows_layouts[pass])) {
        CSLog("shadows", "create_pipeline_layout failed", true);
        return false;
      }
    }
    if (!EnsureGTVBAODescriptorTables(dev, d->shadows_layouts[pass], &d->shadows_tables[pass]))
      return false;
    if (d->shadows_pipelines[pass].handle == 0u) {
      const std::span<const uint8_t> code = kBytecode[pass];
      if (code.empty() || d->shadows_layouts[pass].handle == 0u) return false;
      reshade::api::shader_desc sd = {};
      sd.code = code.data(); sd.code_size = code.size(); sd.entry_point = "main";
      reshade::api::pipeline_subobject so = {
          reshade::api::pipeline_subobject_type::compute_shader, 1, &sd};
      if (!dev->create_pipeline(d->shadows_layouts[pass], 1, &so, &d->shadows_pipelines[pass])) {
        CSLog("shadows", "create_pipeline failed", true);
        return false;
      }
    }
  }
  return true;
}

// Runs whichever of the two passes the user has enabled, and only those.
//
// The two are independent by construction: micro needs the G-buffer normal and an
// occlusion term, contact needs depth, the normal and the noise volume. So each
// is dispatched only if its own toggle is on, and neither waits for the other's
// output. That is what lets a user turn one on without paying for the other's
// memory traffic.
static bool RunShadows(reshade::api::command_list* cl, DeviceData* d, int fromHook) {
  if (!cl || !d) return false;
  if (d->shadows_ran_frame == d->frame_index) return true;  // already done this frame
  auto* dev = cl->get_device();
  if (!dev) return false;

  const bool micro_on = shader_injection.cs_micro_enabled > 0.5f;
  const bool contact_on = shader_injection.cs_contact_enabled > 0.5f;
  if (!micro_on && !contact_on) return true;

  // Name whichever condition is currently blocking, at most once a frame. Every
  // early return here used to be silent, which made "the pass never ran" and "the
  // pass ran and correctly found nothing" indistinguishable from the outside.
  auto blocked = [&](const char* why) {
    if (d->shadows_log_frame == d->frame_index) return;
    d->shadows_log_frame = d->frame_index;
    reshade::log::message(reshade::log::level::warning,
        (std::string("[Shadows] blocked: ") + why
         + " (micro=" + (micro_on ? "on" : "off")
         + " contact=" + (contact_on ? "on" : "off")
         + " depth=" + (d->captured_depth_srv.handle ? "ok" : "null")
         + " mrt=" + (d->captured_mrt_normal_srv.handle ? "ok" : "null")
         + " cbv=" + (d->captured_scene_cbv_view.handle ? "ok" : "null")
         + " live=" + std::to_string(d->captured_depth_live.load()
                                   && d->captured_mrt_live.load()
                                   && d->captured_cbv_live.load())
         + " grid=" + std::to_string(d->shadows_w) + "x" + std::to_string(d->shadows_h)
         + " ready=" + (d->shadows_resources_ready ? "1" : "0")).c_str());
    CSLog("shadows", std::string("blocked: ") + why);
  };

  if (!d->captured_depth_srv.handle || !d->captured_mrt_normal_srv.handle
      || !d->captured_scene_cbv_view.handle) {
    blocked("scene captures missing");
    return false;
  }
  if (!d->captured_depth_live.load() || !d->captured_mrt_live.load()
      || !d->captured_cbv_live.load()) {
    blocked("a captured target was destroyed");
    return false;
  }

  // The IS-FAST volume is an input to the contact march, not to micro shadows.
  // There is deliberately no IGN fallback: if the volume is missing the march
  // runs unjittered, which bands visibly, and that is the honest result.
  const bool isfast_wanted = shader_injection.cs_contact_isfast_enabled > 0.5f;
  if (isfast_wanted) LoadISFASTNoiseTexture(dev, d);
  const bool isfast_ready = d->isfast_texture_loaded && d->isfast_noise_srv.handle != 0u;
  if (isfast_wanted && !isfast_ready && !d->shadows_logged_isfast_missing) {
    d->shadows_logged_isfast_missing = true;
    reshade::log::message(reshade::log::level::warning,
        "[Shadows] IS-FAST requested but the volume is unavailable: contact "
        "shadows will run unjittered. Enable the IS-FAST master in the IS-FAST "
        "section to fix the banding.");
  }

  // The micro pass takes BOTH occlusion sources and picks between them, because
  // the two have genuinely different encodings: GTVBAO quantises visibility into
  // byte 0 of an r32_uint, the game's deferred AO is a float channel. Binding
  // either as the wrong type yields plausible garbage, so both are bound and the
  // shader decodes each correctly.
  //
  // GTVBAO is resolved with exactly the same expression the t22 push uses, so the
  // AO the pass reads and the AO a user can compare against are guaranteed to be
  // the same buffer, in Full and in Half mode alike.
  reshade::api::resource_view ao_gtvbao = {};
  if (shader_injection.gtvbao_mode > 0.5f) {
    const bool half_active = shader_injection.gtvbao_resolution > 0.5f
                          && d->upscale_ao_srv.handle;
    ao_gtvbao = half_active
        ? d->upscale_ao_srv
        : (d->gtvbao_final_in_b ? d->ao_term_b_srv : d->ao_term_a_srv);
  }
  reshade::api::resource_view ao_ssao = d->captured_ssao_srv;
  if (!ao_ssao.handle) ao_ssao = d->fallback_srv;  // AO = 1, the no-occlusion floor

  if (micro_on && !ao_gtvbao.handle && !ao_ssao.handle) {
    blocked("no AO source bound for the micro pass");
    return false;
  }
  if (micro_on && shader_injection.cs_micro_ao_source > 0.5f && !ao_gtvbao.handle
      && !d->shadows_logged_ao_fallback) {
    // Asked for GTVBAO AO but there is none. Say so once rather than quietly
    // substituting the other source, which would make the setting a lie.
    d->shadows_logged_ao_fallback = true;
    reshade::log::message(reshade::log::level::warning,
        "[Shadows] Micro Shadows AO source is set to GTVBAO, but no GTVBAO AO "
        "buffer is available (is GTVBAO enabled?). Switch the source to Game "
        "SSAO, or enable GTVBAO.");
  }

  const uint32_t w = std::max(d->shadows_w, 1u);
  const uint32_t h = std::max(d->shadows_h, 1u);
  if (w < 8u || h < 8u) {
    blocked("shadow grid too small");
    return false;
  }

  if (!CreateShadowsPipelinesIfNeeded(dev, d)) {
    blocked("pipeline creation failed");
    return false;
  }

  // -- push block --
  shader_injection.cs_working_w = static_cast<float>(w);
  shader_injection.cs_working_h = static_cast<float>(h);
  shader_injection.cs_noise_frame = isfast_ready
      ? static_cast<float>(d->frame_index % 32u) : -1.f;
  shader_injection.cs_ao_bound = (ao_gtvbao.handle || ao_ssao.handle) ? 1.f : 0.f;

  const auto CS = reshade::api::shader_stage::all_compute;
  const auto AC = reshade::api::pipeline_stage::all_compute;
  const auto UA = reshade::api::resource_usage::unordered_access;
  const auto SR = reshade::api::resource_usage::shader_resource;

  auto run_pass = [&](uint32_t pass, reshade::api::resource_view* srvs,
                      uint32_t srv_count, reshade::api::resource_view uav,
                      reshade::api::resource out_tex) {
    cl->bind_pipeline(AC, d->shadows_pipelines[pass]);
    reshade::api::descriptor_table_update u[kGtvbaoDescriptorTableParamCount] = {
      {{},0,0,1,reshade::api::descriptor_type::sampler,&d->shadows_point_clamp_sampler},
      {{},0,0,1,reshade::api::descriptor_type::constant_buffer,&d->captured_scene_cbv_view},
      {{},0,0,srv_count,reshade::api::descriptor_type::texture_shader_resource_view,srvs},
      {{},0,0,1,reshade::api::descriptor_type::texture_unordered_access_view,&uav},
    };
    for (uint32_t i = 0; i < kGtvbaoDescriptorTableParamCount; ++i)
      u[i].table = d->shadows_tables[pass][i];
    dev->update_descriptor_tables(kGtvbaoDescriptorTableParamCount, u);
    std::array<reshade::api::descriptor_table, kGtvbaoDescriptorTableParamCount> tables = {
        d->shadows_tables[pass][0], d->shadows_tables[pass][1],
        d->shadows_tables[pass][2], d->shadows_tables[pass][3]};
    cl->bind_descriptor_tables(CS, d->shadows_layouts[pass], 0,
                               kGtvbaoDescriptorTableParamCount, tables.data());
    cl->push_constants(CS, d->shadows_layouts[pass], kGtvbaoPushConstantsLayoutParam, 0,
                       static_cast<uint32_t>(sizeof(ShaderInjectData) / sizeof(uint32_t)),
                       static_cast<const void*>(&shader_injection));
    cl->dispatch((w + 7u) / 8u, (h + 7u) / 8u, 1u);
    // Barrier the resource, not the view: the views themselves are unchanged, so
    // a view-level transition would be a no-op and the sampling SRV would keep
    // seeing the pre-dispatch state.
    if (out_tex.handle) cl->barrier(out_tex, UA, SR);
  };

  if (micro_on) {
    // Four SRVs: the normal, both AO sources, and the IS-FAST volume. The volume is
    // bound even when the dither is off, because a null descriptor is not a legal
    // binding; the shader gates its use on cs_micro_isfast_enabled so the 1x1
    // stand-in is never actually sampled when the toggle is off.
    reshade::api::resource_view micro_srvs[4] = {
        d->captured_mrt_normal_srv,
        ao_gtvbao.handle ? ao_gtvbao : d->fallback_srv,
        ao_ssao.handle ? ao_ssao : d->fallback_srv,
        isfast_ready ? d->isfast_noise_srv : d->fallback_srv};
    run_pass(kShadowsPassMicro, micro_srvs, 4, d->micro_shadow_uav,
             d->micro_shadow_texture);
  }
  if (contact_on) {
    // The noise slot is filled with the real 3D volume when it is usable. When it
    // is not, the fallback is a 1x1 2D stand-in: the shader declares t2 as a
    // Texture3D and is gated on shadow_isfast_texture_loaded, so it never samples
    // it. A null descriptor is not a legal binding, which is the only reason the
    // stand-in exists.
    reshade::api::resource_view contact_srvs[3] = {
        d->captured_depth_srv,
        d->captured_mrt_normal_srv,
        isfast_ready ? d->isfast_noise_srv : d->fallback_srv};
    run_pass(kShadowsPassContact, contact_srvs, 3, d->contact_shadow_uav,
             d->contact_shadow_texture);
  }

  d->shadows_ran_frame = d->frame_index;
  if (!d->shadows_logged_first_dispatch) {
    d->shadows_logged_first_dispatch = true;
    CSLog("shadows", "first dispatch ok at " + std::to_string(w) + "x" + std::to_string(h)
        + " (micro=" + (micro_on ? "on" : "off")
        + " contact=" + (contact_on ? "on" : "off")
        + " ao=" + (shader_injection.cs_micro_ao_source > 0.5f ? "gtvbao" : "ssao")
        + " isfast=" + (isfast_ready ? "ready" : "missing") + ")");
  }
  if (d->shadows_ran_from != fromHook) {
    // Report the first dispatch point once. Kai runs a character lighting pass
    // before the main lighting pass, so which one reaches this first is what
    // decides whether the character pass reads this frame's result or the
    // previous frame's -- and that is not something to have to guess at from a
    // symptom.
    d->shadows_ran_from = fromHook;
    CSLog("shadows", fromHook == 0
        ? "dispatched from the character lighting pass"
        : "dispatched from the main lighting pass");
  }
  return true;
}

// ----------- Custom SSR (Sora 2nd) � Phase 1: Hi-Z pyramid -----------
// ----------- Dynamic Cubemaps (Sora 2nd) � standalone t17 replacement -----------

static uint32_t DynCubeResolveSize(float v) {
  switch ((int)v) {
    case 0: return 128u;
    case 1: return 256u;
    case 2: return 512u;
    case 3: return 768u;
    case 4: return 1024u;
    case 5: return 1536u;
    default: return 2048u;
  }
}

static void DestroyDynCubeResources(reshade::api::device* dev, DeviceData* d) {
  if (!dev || !d) return;
  auto dv = [&](reshade::api::resource_view& v) { if (v.handle) { dev->destroy_resource_view(v); v = {}; } };
  auto dr = [&](reshade::api::resource& r) { if (r.handle) { dev->destroy_resource(r); r = {}; } };
  auto dp = [&](reshade::api::pipeline& p) { if (p.handle) { dev->destroy_pipeline(p); p = {}; } };
  auto dl = [&](reshade::api::pipeline_layout& l) { if (l.handle) { dev->destroy_pipeline_layout(l); l = {}; } };
  if (d->dyncube_sampler.handle) { dev->destroy_sampler(d->dyncube_sampler); d->dyncube_sampler = {}; }
  for (auto& set : d->dyncube_hist) {
    dv(set.color_cube_srv); dv(set.color_arr_srv); dv(set.color_uav); dr(set.color);
    dv(set.pos_arr_srv); dv(set.pos_cube_srv); dv(set.pos_uav); dr(set.pos);
    dv(set.contrib_arr_srv); dv(set.contrib_uav); dr(set.contrib);
  }
  // Aliases are copies of the above handles � zero them (never double-destroy).
  d->dyncube_srv = {};
  d->dyncube_uav = {};
  d->dyncube_texture = {};
  for (uint32_t i = 0; i < 2; ++i) {
    dv(d->dyncube_cam_srv[i]); dv(d->dyncube_cam_uav[i]); dr(d->dyncube_cam[i]);
  }
  // Character mask
  dv(d->dyncube_charmask_srv); dv(d->dyncube_charmask_arr_srv); dr(d->dyncube_charmask);
  if (d->dyncube_charmask_uav.handle) { dev->destroy_resource_view(d->dyncube_charmask_uav); d->dyncube_charmask_uav = {}; }
  // World-fixed parallax bounds (persistent; re-initialized on next create)
  dv(d->dyncube_worldbox_bounds_srv); dv(d->dyncube_worldbox_bounds_uav); dr(d->dyncube_worldbox_bounds);
  dv(d->dyncube_faceextents_uav); dr(d->dyncube_faceextents);
  dr(d->dyncube_faceExtStaging);
  dv(d->dyncube_worldbox_scratch_srv); dv(d->dyncube_worldbox_scratch_uav); dr(d->dyncube_worldbox_scratch);
  d->dyncube_worldbox_scratch_groups = 0u;
  dp(d->dyncube_worldbox_pipeline); dl(d->dyncube_worldbox_layout);
  for (auto& t : d->dyncube_worldbox_tables) { if (t.handle) { dev->free_descriptor_table(t); t = {}; } }
  d->dyncube_worldbox_layout_version = 0u;
  d->dyncube_worldbox_reset_pending = true;
  // Phase 3 GGX
  if (d->dyncube_linear_sampler.handle) { dev->destroy_sampler(d->dyncube_linear_sampler); d->dyncube_linear_sampler = {}; }
  dv(d->dyncube_ggx_in_cube_srv); dr(d->dyncube_ggx_in);
  for (uint32_t i = 0; i < 2; ++i) {
    dv(d->dyncube_ggx_out_cube_srv[i]); dr(d->dyncube_ggx_out[i]);
    for (auto& u : d->dyncube_ggx_out_mip_uav[i]) { if (u.handle) { dev->destroy_resource_view(u); u = {}; } }
  }
  d->dyncube_ggx_valid = false;
  // Global-push variant cube (derived): destroy with the rest, rebuild on demand.
  dv(d->dyncube_variant_cube_srv); dr(d->dyncube_variant);
  if (d->dyncube_variant_mip0_uav.handle) { dev->destroy_resource_view(d->dyncube_variant_mip0_uav); d->dyncube_variant_mip0_uav = {}; }
  dp(d->dyncube_variant_pipeline); dl(d->dyncube_variant_layout);
  for (auto& t : d->dyncube_variant_tables) { if (t.handle) { dev->free_descriptor_table(t); t = {}; } }
  d->dyncube_variant_valid = false;
  // Dedicated solid-color debug cube
  dv(d->dyncube_solid_cube_srv); dr(d->dyncube_solid_cube);
  if (d->dyncube_solid_cube_uav.handle) { dev->destroy_resource_view(d->dyncube_solid_cube_uav); d->dyncube_solid_cube_uav = {}; }
  // Simple SSR
  dv(d->dyncube_ssr_raw_srv); dr(d->dyncube_ssr_raw);
  if (d->dyncube_ssr_raw_uav.handle) { dev->destroy_resource_view(d->dyncube_ssr_raw_uav); d->dyncube_ssr_raw_uav = {}; }
  dv(d->dyncube_ssr_blur_h_srv); dr(d->dyncube_ssr_blur_h);
  if (d->dyncube_ssr_blur_h_uav.handle) { dev->destroy_resource_view(d->dyncube_ssr_blur_h_uav); d->dyncube_ssr_blur_h_uav = {}; }
  dv(d->dyncube_ssr_blur_srv); dr(d->dyncube_ssr_blur);
  if (d->dyncube_ssr_blur_uav.handle) { dev->destroy_resource_view(d->dyncube_ssr_blur_uav); d->dyncube_ssr_blur_uav = {}; }
  dp(d->dyncube_ssr_pipeline); dl(d->dyncube_ssr_layout);
  for (auto& t : d->dyncube_ssr_tables) { if (t.handle) { dev->free_descriptor_table(t); t = {}; } }
  d->dyncube_ssr_layout_version = 0u;
  dp(d->dyncube_ssr_blur_pipeline); dl(d->dyncube_ssr_blur_layout);
  for (auto& t : d->dyncube_ssr_blur_tables) { if (t.handle) { dev->free_descriptor_table(t); t = {}; } }
  d->dyncube_ssr_blur_layout_version = 0u;
  dp(d->dyncube_ggx_pipeline); dl(d->dyncube_ggx_layout);
  for (auto& t : d->dyncube_ggx_tables) { if (t.handle) { dev->free_descriptor_table(t); t = {}; } }
  d->dyncube_hist_cur = 0;
  d->dyncube_needs_reset = true;
  d->dyncube_loadingWipeDone = false;
  d->dyncube_loadingWipePending = false;
  dr(d->dyncube_validStaging);
  dr(d->dyncube_faceExtStaging);
  d->dyncube_readSet = 0;
  d->dyncube_filteredReadSet = 99u;
  d->dyncube_boxCopyPending = false;
  d->dyncube_wasRejected = false;
  d->dyncube_rejectedGap = false;
  d->dyncube_captureDirty = false;
  d->dyncube_dirtyFastForward = false;
  d->dyncube_lastVariantSoften = -1.f;
  d->dyncube_lastVariantStrength = -1.f;
  d->dyncube_lastCharCapture = -1.f;
  d->dyncube_lastSparkleRejection = -1.f;
  d->dyncube_hasValidRead = false;
  dp(d->dyncube_capture_pipeline); dp(d->dyncube_solid_pipeline);
  dl(d->dyncube_capture_layout); dl(d->dyncube_solid_layout);
  d->dyncube_capture_layout_version = 0u;
  for (auto& t : d->dyncube_capture_tables) { if (t.handle) { dev->free_descriptor_table(t); t = {}; } }
  for (auto& t : d->dyncube_solid_tables) { if (t.handle) { dev->free_descriptor_table(t); t = {}; } }
  d->dyncube_resources_created = false;
  d->dyncube_solid_written = false;
}

// Texels across a full cube mip chain (mips clamped by the caller-supplied count).
static double DynCubeChainTexels(uint32_t size, uint32_t mips) {
  double total = 0.0;
  for (uint32_t m = 0; m < mips; ++m) {
    const uint32_t dim = std::max(1u, size >> m);
    total += static_cast<double>(dim) * static_cast<double>(dim) * 6.0;
  }
  return total;
}

// Estimated GPU bytes for the active set. Only counts resources that actually
// exist, so the log tracks the real VRAM delta: the GGX input chain, the global
// variant cube, and the solid debug cube are all created on demand.
static uint64_t DynCubeEstimatedBytes(const DeviceData* d) {
  if (d == nullptr || !d->dyncube_resources_created || d->dyncube_size == 0u) return 0u;
  const uint64_t base = static_cast<uint64_t>(d->dyncube_size) * d->dyncube_size * 6u;
  const uint64_t chain = static_cast<uint64_t>(DynCubeChainTexels(d->dyncube_size, d->dyncube_mip_count));
  uint64_t bytes = 2u * base * (8u + 8u + 2u);  // history color + position + contribution
  bytes += base * 8u;                           // character mask
  bytes += 2u * chain * 8u;                     // filtered A/B cubes
  if (d->dyncube_ggx_in.handle != 0u) bytes += chain * 8u;
  if (d->dyncube_variant.handle != 0u) bytes += chain * 8u;
  if (d->dyncube_solid_cube.handle != 0u) bytes += base * 8u;
  bytes += d->dyncube_worldbox_scratch_groups * 2u * 16u;
  return bytes;
}

// Global-push variant cube (soften + strength): same desc/shape as ggx_out (full mip
// chain for consumer roughness LOD), derived content. Created on demand by the
// variant rebuild, which is the only consumer, so it is not part of a full Create.
static bool CreateDynCubeVariantResources(reshade::api::device* dev, DeviceData* d, uint32_t size, uint32_t mips) {
  if (!dev || !d) return false;
  if (size < 64u) size = 64u;
  if (mips < 1u) mips = 1u;
  reshade::api::resource_desc rdv = {};
  rdv.type = reshade::api::resource_type::texture_2d;
  rdv.texture = {size, size, 6, (uint16_t)mips, reshade::api::format::r16g16b16a16_float, 1};
  rdv.heap = reshade::api::memory_heap::gpu_only;
  rdv.usage = reshade::api::resource_usage::shader_resource | reshade::api::resource_usage::unordered_access;
  rdv.flags = reshade::api::resource_flags::cube_compatible | reshade::api::resource_flags::generate_mipmaps;
  if (!dev->create_resource(rdv, nullptr, reshade::api::resource_usage::shader_resource, &d->dyncube_variant)) {
    reshade::log::message(reshade::log::level::error, "[DynCube] Failed to create variant cube");
    return false;
  }
  dev->create_resource_view(d->dyncube_variant, reshade::api::resource_usage::shader_resource,
    reshade::api::resource_view_desc(reshade::api::resource_view_type::texture_cube,
                                     reshade::api::format::r16g16b16a16_float, 0, mips, 0, 6),
    &d->dyncube_variant_cube_srv);
  dev->create_resource_view(d->dyncube_variant, reshade::api::resource_usage::unordered_access,
    reshade::api::resource_view_desc(reshade::api::resource_view_type::texture_2d_array,
                                     reshade::api::format::r16g16b16a16_float, 0, 1, 0, 6),
    &d->dyncube_variant_mip0_uav);
  return true;
}

// Dedicated solid-color debug cube (debug face 3), one mip, sized to the active
// cube. Only RunDynCubeSolid reads it, so it is created on first debug use instead
// of on every full Create.
static bool CreateDynCubeSolidResources(reshade::api::device* dev, DeviceData* d) {
  if (!dev || !d) return false;
  if (!d->dyncube_resources_created || d->dyncube_size == 0u) return false;
  if (d->dyncube_solid_cube.handle != 0u) {
    return d->dyncube_solid_cube_srv.handle != 0u && d->dyncube_solid_cube_uav.handle != 0u;
  }
  reshade::api::resource_desc rs = {};
  rs.type = reshade::api::resource_type::texture_2d;
  rs.texture = {d->dyncube_size, d->dyncube_size, 6, 1, reshade::api::format::r16g16b16a16_float, 1};
  rs.heap = reshade::api::memory_heap::gpu_only;
  rs.usage = reshade::api::resource_usage::shader_resource | reshade::api::resource_usage::unordered_access;
  rs.flags = reshade::api::resource_flags::cube_compatible;
  if (!dev->create_resource(rs, nullptr, reshade::api::resource_usage::shader_resource, &d->dyncube_solid_cube)) {
    reshade::log::message(reshade::log::level::error, "[DynCube] Failed to create solid cube");
    return false;
  }
  dev->create_resource_view(d->dyncube_solid_cube, reshade::api::resource_usage::shader_resource,
    reshade::api::resource_view_desc(reshade::api::resource_view_type::texture_cube,
                                     reshade::api::format::r16g16b16a16_float, 0, 1, 0, 6),
    &d->dyncube_solid_cube_srv);
  dev->create_resource_view(d->dyncube_solid_cube, reshade::api::resource_usage::unordered_access,
    reshade::api::resource_view_desc(reshade::api::resource_view_type::texture_2d_array,
                                     reshade::api::format::r16g16b16a16_float, 0, 1, 0, 6),
    &d->dyncube_solid_cube_uav);
  d->dyncube_solid_written = false;
  return d->dyncube_solid_cube_srv.handle != 0u && d->dyncube_solid_cube_uav.handle != 0u;
}

// GGX-only input chain (mip0 + hardware mips) used to seed the roughness filter
// without an SRV/UAV same-resource hazard. The hardware-mips path never reads it,
// so it is only allocated when GGX filtering is enabled.
static bool CreateDynCubeGGXInResources(reshade::api::device* dev, DeviceData* d) {
  if (!dev || !d) return false;
  if (!d->dyncube_resources_created || d->dyncube_size == 0u) return false;
  if (d->dyncube_ggx_in.handle != 0u) return d->dyncube_ggx_in_cube_srv.handle != 0u;
  reshade::api::resource_desc rdi = {};
  rdi.type = reshade::api::resource_type::texture_2d;
  rdi.texture = {d->dyncube_size, d->dyncube_size, 6, (uint16_t)d->dyncube_mip_count,
                 reshade::api::format::r16g16b16a16_float, 1};
  rdi.heap = reshade::api::memory_heap::gpu_only;
  rdi.usage = reshade::api::resource_usage::shader_resource | reshade::api::resource_usage::unordered_access;
  rdi.flags = reshade::api::resource_flags::cube_compatible | reshade::api::resource_flags::generate_mipmaps;
  if (!dev->create_resource(rdi, nullptr, reshade::api::resource_usage::shader_resource, &d->dyncube_ggx_in)) {
    reshade::log::message(reshade::log::level::error, "[DynCube] Failed to create GGX input");
    return false;
  }
  dev->create_resource_view(d->dyncube_ggx_in, reshade::api::resource_usage::shader_resource,
    reshade::api::resource_view_desc(reshade::api::resource_view_type::texture_cube,
                                     reshade::api::format::r16g16b16a16_float, 0, d->dyncube_mip_count, 0, 6),
    &d->dyncube_ggx_in_cube_srv);
  return d->dyncube_ggx_in_cube_srv.handle != 0u;
}

static bool CreateDynCubeResources(reshade::api::device* dev, DeviceData* d, uint32_t size) {
  DestroyDynCubeResources(dev, d);
  if (size < 128u) size = 128u;
  if (size > 2048u) size = 2048u;
  d->dyncube_size = size;

  // Throttled logger helper (1/sec) � only when debug logging enabled
  auto should_log = []() -> bool {
    if (shader_injection.dynCube_debug_logging < 0.5f) return false;
    static auto last = std::chrono::steady_clock::now() - std::chrono::seconds(2);
    auto now = std::chrono::steady_clock::now();
    if (now - last < std::chrono::seconds(1)) return false;
    last = now;
    return true;
  };

  // Point clamp sampler for diagnostic � no filtering to test projection
  reshade::api::sampler_desc sd = {};
  sd.filter = reshade::api::filter_mode::min_mag_mip_point;
  sd.address_u = reshade::api::texture_address_mode::clamp;
  sd.address_v = reshade::api::texture_address_mode::clamp;
  sd.address_w = reshade::api::texture_address_mode::clamp;
  dev->create_sampler(sd, &d->dyncube_sampler);

  auto make_hist_set = [&](uint32_t idx) -> bool {
    // Color (cube-compatible, RGBA16F): cube SRV (t17) + array SRV (prev read) + array UAV (cur write)
    reshade::api::resource_desc rd = {};
    rd.type = reshade::api::resource_type::texture_2d;
    rd.texture = {size, size, 6, 1, reshade::api::format::r16g16b16a16_float, 1};
    rd.heap = reshade::api::memory_heap::gpu_only;
    rd.usage = reshade::api::resource_usage::shader_resource | reshade::api::resource_usage::unordered_access;
    rd.flags = reshade::api::resource_flags::cube_compatible;
    if (!dev->create_resource(rd, nullptr, reshade::api::resource_usage::shader_resource, &d->dyncube_hist[idx].color)) {
      if (should_log()) reshade::log::message(reshade::log::level::error, "[DynCube] Failed to create history color");
      return false;
    }
    dev->create_resource_view(d->dyncube_hist[idx].color, reshade::api::resource_usage::shader_resource,
      reshade::api::resource_view_desc(reshade::api::resource_view_type::texture_cube,
                                       reshade::api::format::r16g16b16a16_float, 0, 1, 0, 6),
      &d->dyncube_hist[idx].color_cube_srv);
    dev->create_resource_view(d->dyncube_hist[idx].color, reshade::api::resource_usage::shader_resource,
      reshade::api::resource_view_desc(reshade::api::resource_view_type::texture_2d_array,
                                       reshade::api::format::r16g16b16a16_float, 0, 1, 0, 6),
      &d->dyncube_hist[idx].color_arr_srv);
    dev->create_resource_view(d->dyncube_hist[idx].color, reshade::api::resource_usage::unordered_access,
      reshade::api::resource_view_desc(reshade::api::resource_view_type::texture_2d_array,
                                       reshade::api::format::r16g16b16a16_float, 0, 1, 0, 6),
      &d->dyncube_hist[idx].color_uav);
    // Position (RGBA16F, rgb=scaled pos, a=validity) � array SRV/UAV + cube SRV (debug 11/12)
    if (!dev->create_resource(rd, nullptr, reshade::api::resource_usage::shader_resource, &d->dyncube_hist[idx].pos)) {
      if (should_log()) reshade::log::message(reshade::log::level::error, "[DynCube] Failed to create history pos");
      return false;
    }
    dev->create_resource_view(d->dyncube_hist[idx].pos, reshade::api::resource_usage::shader_resource,
      reshade::api::resource_view_desc(reshade::api::resource_view_type::texture_2d_array,
                                       reshade::api::format::r16g16b16a16_float, 0, 1, 0, 6),
      &d->dyncube_hist[idx].pos_arr_srv);
    dev->create_resource_view(d->dyncube_hist[idx].pos, reshade::api::resource_usage::shader_resource,
      reshade::api::resource_view_desc(reshade::api::resource_view_type::texture_cube,
                                       reshade::api::format::r16g16b16a16_float, 0, 1, 0, 6),
      &d->dyncube_hist[idx].pos_cube_srv);
    dev->create_resource_view(d->dyncube_hist[idx].pos, reshade::api::resource_usage::unordered_access,
      reshade::api::resource_view_desc(reshade::api::resource_view_type::texture_2d_array,
                                       reshade::api::format::r16g16b16a16_float, 0, 1, 0, 6),
      &d->dyncube_hist[idx].pos_uav);
    // Contribution (R16F) � array SRV/UAV only
    reshade::api::resource_desc rc = {};
    rc.type = reshade::api::resource_type::texture_2d;
    rc.texture = {size, size, 6, 1, reshade::api::format::r16_float, 1};
    rc.heap = reshade::api::memory_heap::gpu_only;
    rc.usage = reshade::api::resource_usage::shader_resource | reshade::api::resource_usage::unordered_access;
    if (!dev->create_resource(rc, nullptr, reshade::api::resource_usage::shader_resource, &d->dyncube_hist[idx].contrib)) {
      if (should_log()) reshade::log::message(reshade::log::level::error, "[DynCube] Failed to create history contrib");
      return false;
    }
    dev->create_resource_view(d->dyncube_hist[idx].contrib, reshade::api::resource_usage::shader_resource,
      reshade::api::resource_view_desc(reshade::api::resource_view_type::texture_2d_array,
                                       reshade::api::format::r16_float, 0, 1, 0, 6),
      &d->dyncube_hist[idx].contrib_arr_srv);
    dev->create_resource_view(d->dyncube_hist[idx].contrib, reshade::api::resource_usage::unordered_access,
      reshade::api::resource_view_desc(reshade::api::resource_view_type::texture_2d_array,
                                       reshade::api::format::r16_float, 0, 1, 0, 6),
      &d->dyncube_hist[idx].contrib_uav);
    return true;
  };

  for (uint32_t i = 0; i < 2; ++i) {
    if (!make_hist_set(i)) {
      DestroyDynCubeResources(dev, d);
      return false;
    }
    // GPU camera ping-pong (1x1 RGBA32F)
    reshade::api::resource_desc cd = {};
    cd.type = reshade::api::resource_type::texture_2d;
    cd.texture = {1, 1, 1, 1, reshade::api::format::r32g32b32a32_float, 1};
    cd.heap = reshade::api::memory_heap::gpu_only;
    cd.usage = reshade::api::resource_usage::shader_resource | reshade::api::resource_usage::unordered_access;
    if (!dev->create_resource(cd, nullptr, reshade::api::resource_usage::shader_resource, &d->dyncube_cam[i])) {
      if (should_log()) reshade::log::message(reshade::log::level::error, "[DynCube] Failed to create cam buffer");
      DestroyDynCubeResources(dev, d);
      return false;
    }
    reshade::api::resource_view_desc cvd(reshade::api::resource_view_type::texture_2d,
                                         reshade::api::format::r32g32b32a32_float, 0, 1, 0, 1);
    dev->create_resource_view(d->dyncube_cam[i], reshade::api::resource_usage::shader_resource, cvd, &d->dyncube_cam_srv[i]);
    dev->create_resource_view(d->dyncube_cam[i], reshade::api::resource_usage::unordered_access, cvd, &d->dyncube_cam_uav[i]);
  }

  // Aliases -> current history set (A initially); needs reset on first capture.
  d->dyncube_hist_cur = 0;
  d->dyncube_needs_reset = true;
  d->dyncube_loadingWipeDone = false;
  d->dyncube_loadingWipePending = false;
  d->dyncube_texture = d->dyncube_hist[0].color;
  d->dyncube_srv = d->dyncube_hist[0].color_cube_srv;
  d->dyncube_uav = d->dyncube_hist[0].color_uav;

  // Trilinear clamp sampler for GGX input-chain mip sampling (distinct from point clamp).
  {
    reshade::api::sampler_desc ls = {};
    ls.filter = reshade::api::filter_mode::min_mag_mip_linear;
    ls.address_u = ls.address_v = ls.address_w = reshade::api::texture_address_mode::clamp;
    dev->create_sampler(ls, &d->dyncube_linear_sampler);
  }

  // Character mask (Phase 2): RGBA16F cube, 1 mip
  {
    reshade::api::resource_desc rd = {};
    rd.type = reshade::api::resource_type::texture_2d;
    rd.texture = {size, size, 6, 1, reshade::api::format::r16g16b16a16_float, 1};
    rd.heap = reshade::api::memory_heap::gpu_only;
    rd.usage = reshade::api::resource_usage::shader_resource | reshade::api::resource_usage::unordered_access;
    rd.flags = reshade::api::resource_flags::cube_compatible;
    if (!dev->create_resource(rd, nullptr, reshade::api::resource_usage::shader_resource, &d->dyncube_charmask)) {
      if (should_log()) reshade::log::message(reshade::log::level::error, "[DynCube] Failed to create character mask");
      DestroyDynCubeResources(dev, d);
      return false;
    }
    dev->create_resource_view(d->dyncube_charmask, reshade::api::resource_usage::shader_resource,
      reshade::api::resource_view_desc(reshade::api::resource_view_type::texture_cube,
                                       reshade::api::format::r16g16b16a16_float, 0, 1, 0, 6),
      &d->dyncube_charmask_srv);
    dev->create_resource_view(d->dyncube_charmask, reshade::api::resource_usage::shader_resource,
      reshade::api::resource_view_desc(reshade::api::resource_view_type::texture_2d_array,
                                       reshade::api::format::r16g16b16a16_float, 0, 1, 0, 6),
      &d->dyncube_charmask_arr_srv);
    dev->create_resource_view(d->dyncube_charmask, reshade::api::resource_usage::unordered_access,
      reshade::api::resource_view_desc(reshade::api::resource_view_type::texture_2d_array,
                                       reshade::api::format::r16g16b16a16_float, 0, 1, 0, 6),
      &d->dyncube_charmask_uav);
  }

  // -- World-fixed parallax bounds (Sora2nd v1): persistent, size-independent --
  {
    // bounds: 2x float4 [0]=(min,valid) [1]=(max,spare); initialized empty/invalid.
    // Initial upload + in-shader stored-validity check make the first merge safe.
    float initBounds[8] = {
      3.402823466e+38f, 3.402823466e+38f, 3.402823466e+38f, 0.f,
      -3.402823466e+38f, -3.402823466e+38f, -3.402823466e+38f, 0.f,
    };
    reshade::api::subresource_data initData = {initBounds, sizeof(initBounds), sizeof(initBounds)};
    reshade::api::resource_desc rb = {};
    rb.type = reshade::api::resource_type::buffer;
    rb.buffer.size = sizeof(initBounds);
    rb.buffer.stride = 16;
    rb.heap = reshade::api::memory_heap::gpu_only;
    rb.usage = reshade::api::resource_usage::shader_resource | reshade::api::resource_usage::unordered_access;
    if (!dev->create_resource(rb, &initData, reshade::api::resource_usage::shader_resource, &d->dyncube_worldbox_bounds)) {
      if (should_log()) reshade::log::message(reshade::log::level::error, "[DynCube] Failed to create world-box bounds");
      DestroyDynCubeResources(dev, d);
      return false;
    }
    // NOTE: buffer view offset/size are STRUCTURED ELEMENT counts for the D3D11
    // backend (FirstElement/NumElements), not bytes � UINT64_MAX is invalid here.
    if (!dev->create_resource_view(d->dyncube_worldbox_bounds, reshade::api::resource_usage::shader_resource,
        reshade::api::resource_view_desc(reshade::api::resource_view_type::buffer, reshade::api::format::unknown, 0, 2),
        &d->dyncube_worldbox_bounds_srv)
        || !d->dyncube_worldbox_bounds_srv.handle) {
      if (should_log()) reshade::log::message(reshade::log::level::error, "[DynCube] Failed to create world-box bounds SRV");
      DestroyDynCubeResources(dev, d);
      return false;
    }
    if (!dev->create_resource_view(d->dyncube_worldbox_bounds, reshade::api::resource_usage::unordered_access,
        reshade::api::resource_view_desc(reshade::api::resource_view_type::buffer, reshade::api::format::unknown, 0, 2),
        &d->dyncube_worldbox_bounds_uav)
        || !d->dyncube_worldbox_bounds_uav.handle) {
      if (should_log()) reshade::log::message(reshade::log::level::error, "[DynCube] Failed to create world-box bounds UAV");
      DestroyDynCubeResources(dev, d);
      return false;
    }
    // scratch: per-group min/max pairs, sized for the active cube size (not a fixed max).
    const uint64_t scratchGroups = (uint64_t)((size + 7u) / 8u) * ((size + 7u) / 8u) * 6u;
    reshade::api::resource_desc rs = {};
    rs.type = reshade::api::resource_type::buffer;
    rs.buffer.size = scratchGroups * 2u * 16u;
    rs.buffer.stride = 16;
    rs.heap = reshade::api::memory_heap::gpu_only;
    rs.usage = reshade::api::resource_usage::shader_resource | reshade::api::resource_usage::unordered_access;
    if (!dev->create_resource(rs, nullptr, reshade::api::resource_usage::shader_resource, &d->dyncube_worldbox_scratch)) {
      if (should_log()) reshade::log::message(reshade::log::level::error, "[DynCube] Failed to create world-box scratch");
      DestroyDynCubeResources(dev, d);
      return false;
    }
    const uint64_t scratchElements = scratchGroups * 2u;
    if (!dev->create_resource_view(d->dyncube_worldbox_scratch, reshade::api::resource_usage::shader_resource,
        reshade::api::resource_view_desc(reshade::api::resource_view_type::buffer, reshade::api::format::unknown, 0, scratchElements),
        &d->dyncube_worldbox_scratch_srv)
        || !d->dyncube_worldbox_scratch_srv.handle) {
      if (should_log()) reshade::log::message(reshade::log::level::error, "[DynCube] Failed to create world-box scratch SRV");
      DestroyDynCubeResources(dev, d);
      return false;
    }
    if (!dev->create_resource_view(d->dyncube_worldbox_scratch, reshade::api::resource_usage::unordered_access,
        reshade::api::resource_view_desc(reshade::api::resource_view_type::buffer, reshade::api::format::unknown, 0, scratchElements),
        &d->dyncube_worldbox_scratch_uav)
        || !d->dyncube_worldbox_scratch_uav.handle) {
      if (should_log()) reshade::log::message(reshade::log::level::error, "[DynCube] Failed to create world-box scratch UAV");
      DestroyDynCubeResources(dev, d);
      return false;
    }
    d->dyncube_worldbox_scratch_groups = scratchGroups;
    // faceExtents: 2x float4 [0]=(+X,+Y,+Z,faceMask) [1]=(-X,-Y,-Z,spare); written by pass 1, staged for logging.
    float initExtents[8] = {0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f};
    reshade::api::subresource_data initExtData = {initExtents, sizeof(initExtents), sizeof(initExtents)};
    reshade::api::resource_desc re = {};
    re.type = reshade::api::resource_type::buffer;
    re.buffer.size = sizeof(initExtents);
    re.buffer.stride = 16;
    re.heap = reshade::api::memory_heap::gpu_only;
    re.usage = reshade::api::resource_usage::shader_resource | reshade::api::resource_usage::unordered_access;
    if (!dev->create_resource(re, &initExtData, reshade::api::resource_usage::shader_resource, &d->dyncube_faceextents)) {
      if (should_log()) reshade::log::message(reshade::log::level::error, "[DynCube] Failed to create world-box face extents");
      DestroyDynCubeResources(dev, d);
      return false;
    }
    if (!dev->create_resource_view(d->dyncube_faceextents, reshade::api::resource_usage::unordered_access,
        reshade::api::resource_view_desc(reshade::api::resource_view_type::buffer, reshade::api::format::unknown, 0, 2),
        &d->dyncube_faceextents_uav)
        || !d->dyncube_faceextents_uav.handle) {
      if (should_log()) reshade::log::message(reshade::log::level::error, "[DynCube] Failed to create world-box face extents UAV");
      DestroyDynCubeResources(dev, d);
      return false;
    }
    d->dyncube_worldbox_reset_pending = true;
  }

  // Face-extent staging for logging: 32B gpu_to_cpu copy of the extents buffer.
  // Same lifetime as worldbox resources.
  {
    reshade::api::resource_desc rsb = {};
    rsb.type = reshade::api::resource_type::buffer;
    rsb.buffer.size = 32u;
    rsb.buffer.stride = 0;
    rsb.heap = reshade::api::memory_heap::gpu_to_cpu;
    rsb.usage = reshade::api::resource_usage::copy_dest;
    if (!dev->create_resource(rsb, nullptr, reshade::api::resource_usage::copy_dest, &d->dyncube_faceExtStaging)) {
      if (should_log()) reshade::log::message(reshade::log::level::error, "[DynCube] Failed to create face-extent staging buffer");
      DestroyDynCubeResources(dev, d);
      return false;
    }
  }

  // Validity staging for delayed-validate commit: 32B gpu_to_cpu copy of the bounds
  // buffer (bounds[1].w carries per-frame hasGeom). Same lifetime as worldbox resources.
  {
    reshade::api::resource_desc rsb = {};
    rsb.type = reshade::api::resource_type::buffer;
    rsb.buffer.size = 32u;
    rsb.buffer.stride = 0;
    rsb.heap = reshade::api::memory_heap::gpu_to_cpu;
    rsb.usage = reshade::api::resource_usage::copy_dest;
    if (!dev->create_resource(rsb, nullptr, reshade::api::resource_usage::copy_dest, &d->dyncube_validStaging)) {
      if (should_log()) reshade::log::message(reshade::log::level::error, "[DynCube] Failed to create validity staging buffer");
      DestroyDynCubeResources(dev, d);
      return false;
    }
  }

  // -- Phase 3 filtered cubes --
  {
    // Mip count: 8 for all supported resolutions (128..4096); computed defensively.
    uint32_t mips = 1;
    while ((size >> mips) >= 1u && mips < 8u) ++mips;
    d->dyncube_mip_count = mips;

    // Output: two RGBA16F cubes, N mips each (mip0 = sharp history copy, mips 1..N-1 = filtered).
    // Double-buffered (Active/Building) so a partially-written cube is never exposed to t17.
    reshade::api::resource_desc rdo = {};
    rdo.type = reshade::api::resource_type::texture_2d;
    rdo.texture = {size, size, 6, (uint16_t)mips, reshade::api::format::r16g16b16a16_float, 1};
    rdo.heap = reshade::api::memory_heap::gpu_only;
    rdo.usage = reshade::api::resource_usage::shader_resource | reshade::api::resource_usage::unordered_access;
    rdo.flags = reshade::api::resource_flags::cube_compatible | reshade::api::resource_flags::generate_mipmaps; // HW A/B path GenerateMips
    for (uint32_t i = 0; i < 2; ++i) {
      if (!dev->create_resource(rdo, nullptr, reshade::api::resource_usage::shader_resource, &d->dyncube_ggx_out[i])) {
        if (should_log()) reshade::log::message(reshade::log::level::error, "[DynCube] Failed to create GGX output");
        DestroyDynCubeResources(dev, d);
        return false;
      }
      dev->create_resource_view(d->dyncube_ggx_out[i], reshade::api::resource_usage::shader_resource,
        reshade::api::resource_view_desc(reshade::api::resource_view_type::texture_cube,
                                         reshade::api::format::r16g16b16a16_float, 0, mips, 0, 6),
        &d->dyncube_ggx_out_cube_srv[i]);
      // Per-mip array UAVs (mips 1..N-1) for the GGX filter dispatches.
      for (uint32_t m = 1; m < mips; ++m) {
        dev->create_resource_view(d->dyncube_ggx_out[i], reshade::api::resource_usage::unordered_access,
          reshade::api::resource_view_desc(reshade::api::resource_view_type::texture_2d_array,
                                           reshade::api::format::r16g16b16a16_float, m, 1, 0, 6),
          &d->dyncube_ggx_out_mip_uav[i][m]);
      }
    }
    d->dyncube_ggx_active = 0;
    d->dyncube_ggx_valid = false;
    // Global-push variant cube (soften + strength) and the solid-color debug cube are
    // both derived: created on demand by their own passes, never allocated here.
    d->dyncube_variant_valid = false;
    d->dyncube_lastVariantSoften = -1.f;
    d->dyncube_lastVariantStrength = -1.f;
  }

  d->dyncube_resources_created = true;
  d->dyncube_solid_written = false;
  // Delayed-validate commit state: consumers start on set 0 (matches aliases above).
  d->dyncube_readSet = 0;
  d->dyncube_filteredReadSet = 99u;
  d->dyncube_boxCopyPending = false;
  d->dyncube_wasRejected = false;
  d->dyncube_rejectedGap = false;
  d->dyncube_captureDirty = false;
  d->dyncube_dirtyFastForward = false;
  d->dyncube_lastVariantSoften = -1.f;
  d->dyncube_lastVariantStrength = -1.f;
  d->dyncube_lastCharCapture = -1.f;
  d->dyncube_lastSparkleRejection = -1.f;
  d->dyncube_hasValidRead = false;
  if (should_log()) {
    reshade::log::message(reshade::log::level::info,
      (std::string("[DynCube] Resources created: ") + std::to_string(size) + "x" + std::to_string(size) +
       "x6, mips=" + std::to_string(d->dyncube_mip_count) +
       ", ~" + std::to_string((long long)(DynCubeEstimatedBytes(d) / (1024ull * 1024ull))) + " MB").c_str());
  }
  return true;
}

static bool CreateDynCubePipelinesIfNeeded(reshade::api::device* dev, DeviceData* d) {
  using DR = reshade::api::descriptor_range;
  using DS = reshade::api::shader_stage;
  using DT = reshade::api::descriptor_type;
  using P = reshade::api::pipeline_layout_param;
  if (!dev || !d) return false;

  // GGX input chain is only read by the GGX filter branch, so it exists only when
  // GGX is on. This runs on every filter pass, so toggling it allocates on demand.
  if (shader_injection.dynCube_ggx > 0.5f) CreateDynCubeGGXInResources(dev, d);

  auto mkcs = [&](std::span<const uint8_t> bc, reshade::api::pipeline_layout lo, reshade::api::pipeline* out) -> bool {
    if (bc.empty() || !lo.handle) return false;
    if (out->handle != 0u) return true;
    reshade::api::shader_desc sd = {};
    sd.code = bc.data(); sd.code_size = bc.size(); sd.entry_point = "main";
    reshade::api::pipeline_subobject so = {reshade::api::pipeline_subobject_type::compute_shader, 1, &sd};
    return dev->create_pipeline(lo, 1, &so, out);
  };
  // Phase 1+2: 7 SRVs (depth, color, prevColor, prevPos, prevContrib, camPrev, mrt0), 5 UAVs (curColor, curPos, curContrib, camCur, charmask), 10 push floats
  auto make_capture_layout = [&](reshade::api::pipeline_layout* out) -> bool {
    if (out->handle != 0u && d->dyncube_capture_layout_version == kDynCubeCaptureLayoutVersion) return true;
    if (out->handle != 0u) {
      // Stale layout shape: drop layout, tables, and pipeline so they rebuild below.
      for (auto& t : d->dyncube_capture_tables) { if (t.handle) { dev->free_descriptor_table(t); t = {}; } }
      dev->destroy_pipeline_layout(*out); *out = {};
      if (d->dyncube_capture_pipeline.handle) { dev->destroy_pipeline(d->dyncube_capture_pipeline); d->dyncube_capture_pipeline = {}; }
    }
    DR sampler_r = {0,0,0,1,DS::all_compute,1,DT::sampler};
    DR cbv_r     = {0,0,0,1,DS::all_compute,1,DT::constant_buffer}; // b0 cb_scene only
    DR srv_r     = {0,0,0,8,DS::all_compute,1,DT::texture_shader_resource_view}; // t0..t7 (depth, color, prevColor, prevPos, prevContrib, camPrev, mrt0, vanilla)
    DR uav_r     = {0,0,0,5,DS::all_compute,1,DT::texture_unordered_access_view}; // u0..u4 (curColor, curPos, curContrib, camCur, charmask)
    reshade::api::constant_range push_range = {};
    push_range.binding = 0;
    push_range.dx_register_index = 13;
    push_range.dx_register_space = 0;
    push_range.count = 12; // boost, blend, posThreshold, posScale, reset, characterCapture, charMaskAvailable, charComp, charShift, soften, charInvert, sparkleReject
    push_range.visibility = DS::all_compute;
    P p0, p1, p2, p3, pPush;
    p0.type = reshade::api::pipeline_layout_param_type::descriptor_table; p0.descriptor_table.count = 1; p0.descriptor_table.ranges = &sampler_r;
    p1.type = reshade::api::pipeline_layout_param_type::descriptor_table; p1.descriptor_table.count = 1; p1.descriptor_table.ranges = &cbv_r;
    p2.type = reshade::api::pipeline_layout_param_type::descriptor_table; p2.descriptor_table.count = 1; p2.descriptor_table.ranges = &srv_r;
    p3.type = reshade::api::pipeline_layout_param_type::descriptor_table; p3.descriptor_table.count = 1; p3.descriptor_table.ranges = &uav_r;
    pPush.type = reshade::api::pipeline_layout_param_type::push_constants; pPush.push_constants = push_range;
    P params[5] = {p0,p1,p2,p3,pPush};
    if (!dev->create_pipeline_layout(5, params, out)) return false;
    d->dyncube_capture_layout_version = kDynCubeCaptureLayoutVersion;
    return true;
  };
  auto make_solid_layout = [&](reshade::api::pipeline_layout* out) -> bool {
    if (out->handle != 0u) return true;
    DR uav_r = {0,0,0,1,DS::all_compute,1,DT::texture_unordered_access_view};
    P p0; p0.type = reshade::api::pipeline_layout_param_type::descriptor_table; p0.descriptor_table.count = 1; p0.descriptor_table.ranges = &uav_r;
    return dev->create_pipeline_layout(1, &p0, out);
  };

  if (!make_capture_layout(&d->dyncube_capture_layout)) return false;
  if (!make_solid_layout(&d->dyncube_solid_layout)) return false;
  // Ensure tables
  auto ensure = [&](reshade::api::pipeline_layout lo, GTVBAODescriptorTableSet* tbl, uint32_t count) -> bool {
    for (uint32_t i = 0; i < count; ++i) {
      if ((*tbl)[i].handle != 0u) continue;
      if (!dev->allocate_descriptor_table(lo, i, &(*tbl)[i])) return false;
    }
    return true;
  };
  if (!ensure(d->dyncube_capture_layout, &d->dyncube_capture_tables, 4)) return false;
  if (!ensure(d->dyncube_solid_layout, &d->dyncube_solid_tables, 1)) return false;

  // Embedded shaders are inline constexpr spans defined in <embed/shaders.h>
  // Prefer generic DynamicCubemapCaptureCS, fallback to legacy dyncube_capture for compat.
  bool haveCapture = false;
  bool haveSolid = false;
  #ifdef __DynamicCubemapCaptureCS_EMBED_FILE
  haveCapture = !__DynamicCubemapCaptureCS.empty();
  #else
  haveCapture = !__dyncube_capture.empty();
  #endif
  haveSolid = !__dyncube_solid.empty();
  if (!haveCapture && !haveSolid) {
    return true;
  }
  auto pipelog_should = []() -> bool {
    if (shader_injection.dynCube_debug_logging < 0.5f) return false;
    static auto last = std::chrono::steady_clock::now() - std::chrono::seconds(2);
    auto now = std::chrono::steady_clock::now();
    if (now - last < std::chrono::seconds(1)) return false;
    last = now;
    return true;
  };
  #ifdef __DynamicCubemapCaptureCS_EMBED_FILE
  if (!__DynamicCubemapCaptureCS.empty()) {
    if (!mkcs(__DynamicCubemapCaptureCS, d->dyncube_capture_layout, &d->dyncube_capture_pipeline)) {
      if (pipelog_should()) reshade::log::message(reshade::log::level::warning, "[DynCube] capture pipeline create failed (DynamicCubemapCaptureCS)");
    }
  } else {
    // Canonical capture shader missing (twin dyncube_capture.cs_5_0.hlsl was
    // deduplicated); nothing to fall back to. Behavior unchanged: the removed
    // twin was byte-identical and the old fallback branch never executed while both existed.
    if (pipelog_should()) reshade::log::message(reshade::log::level::warning, "[DynCube] capture pipeline create failed (DynamicCubemapCaptureCS)");
  }
  #else
  if (!__dyncube_capture.empty()) {
    if (!mkcs(__dyncube_capture, d->dyncube_capture_layout, &d->dyncube_capture_pipeline)) {
      if (pipelog_should()) reshade::log::message(reshade::log::level::warning, "[DynCube] capture pipeline create failed");
    }
  }
  #endif
  if (!__dyncube_solid.empty())
    if (!mkcs(__dyncube_solid, d->dyncube_solid_layout, &d->dyncube_solid_pipeline)) {
      if (pipelog_should()) reshade::log::message(reshade::log::level::warning, "[DynCube] solid pipeline create failed");
    }

  // -- Phase 3 GGX prefilter pipeline --
  auto make_ggx_layout = [&](reshade::api::pipeline_layout* out) -> bool {
    if (out->handle != 0u) return true;
    DR sampler_r = {0,0,0,1,DS::all_compute,1,DT::sampler};
    DR srv_r     = {0,0,0,1,DS::all_compute,1,DT::texture_shader_resource_view}; // t0 GGX input cube
    DR uav_r     = {0,0,0,1,DS::all_compute,1,DT::texture_unordered_access_view}; // u0 filtered mip UAV
    reshade::api::constant_range push_range = {};
    push_range.binding = 0;
    push_range.dx_register_index = 13;
    push_range.dx_register_space = 0;
    push_range.count = 1; // roughness
    push_range.visibility = DS::all_compute;
    P p0, p1, p2, pPush;
    p0.type = reshade::api::pipeline_layout_param_type::descriptor_table; p0.descriptor_table.count = 1; p0.descriptor_table.ranges = &sampler_r;
    p1.type = reshade::api::pipeline_layout_param_type::descriptor_table; p1.descriptor_table.count = 1; p1.descriptor_table.ranges = &srv_r;
    p2.type = reshade::api::pipeline_layout_param_type::descriptor_table; p2.descriptor_table.count = 1; p2.descriptor_table.ranges = &uav_r;
    pPush.type = reshade::api::pipeline_layout_param_type::push_constants; pPush.push_constants = push_range;
    P params[4] = {p0,p1,p2,pPush};
    return dev->create_pipeline_layout(4, params, out);
  };
  if (!make_ggx_layout(&d->dyncube_ggx_layout)) return false;
  if (!ensure(d->dyncube_ggx_layout, &d->dyncube_ggx_tables, 3)) return false;
  #ifdef __SpecularIrradianceCS_EMBED_FILE
  if (!__SpecularIrradianceCS.empty()) {
    if (!mkcs(__SpecularIrradianceCS, d->dyncube_ggx_layout, &d->dyncube_ggx_pipeline)) {
      if (pipelog_should()) reshade::log::message(reshade::log::level::warning, "[DynCube] GGX pipeline create failed");
    }
  }
  #endif

  // -- Global-push variant pipeline (soften + strength resample) --
  auto make_variant_layout = [&](reshade::api::pipeline_layout* out) -> bool {
    if (out->handle != 0u) return true;
    DR sampler_r = {0,0,0,1,DS::all_compute,1,DT::sampler}; // s0 trilinear clamp
    DR srv_r     = {0,0,0,1,DS::all_compute,1,DT::texture_shader_resource_view}; // t0 source cube
    DR uav_r     = {0,0,0,1,DS::all_compute,1,DT::texture_unordered_access_view}; // u0 variant mip0 array
    reshade::api::constant_range push_range = {};
    push_range.binding = 0;
    push_range.dx_register_index = 13;
    push_range.dx_register_space = 0;
    push_range.count = 2; // srcMip, strength
    push_range.visibility = DS::all_compute;
    P p0, p1, p2, pPush;
    p0.type = reshade::api::pipeline_layout_param_type::descriptor_table; p0.descriptor_table.count = 1; p0.descriptor_table.ranges = &sampler_r;
    p1.type = reshade::api::pipeline_layout_param_type::descriptor_table; p1.descriptor_table.count = 1; p1.descriptor_table.ranges = &srv_r;
    p2.type = reshade::api::pipeline_layout_param_type::descriptor_table; p2.descriptor_table.count = 1; p2.descriptor_table.ranges = &uav_r;
    pPush.type = reshade::api::pipeline_layout_param_type::push_constants; pPush.push_constants = push_range;
    P params[4] = {p0,p1,p2,pPush};
    return dev->create_pipeline_layout(4, params, out);
  };
  if (!make_variant_layout(&d->dyncube_variant_layout)) return false;
  if (!ensure(d->dyncube_variant_layout, &d->dyncube_variant_tables, 3)) return false;
  #ifdef __DynCubeVariantCS_EMBED_FILE
  if (!__DynCubeVariantCS.empty()) {
    if (!mkcs(__DynCubeVariantCS, d->dyncube_variant_layout, &d->dyncube_variant_pipeline)) {
      if (pipelog_should()) reshade::log::message(reshade::log::level::warning, "[DynCube] Variant pipeline create failed");
    }
  }
  #endif

  // -- Simple SSR pipeline --
  auto make_ssr_layout = [&](reshade::api::pipeline_layout* out) -> bool {
    if (out->handle != 0u && d->dyncube_ssr_layout_version == kDynCubeSSRLayoutVersion) return true;
    if (out->handle != 0u) {
      // Stale layout shape (e.g. pre-IS-FAST): drop layout, tables, and pipeline so they rebuild below.
      for (auto& t : d->dyncube_ssr_tables) { if (t.handle) { dev->free_descriptor_table(t); t = {}; } }
      dev->destroy_pipeline_layout(*out); *out = {};
      if (d->dyncube_ssr_pipeline.handle) { dev->destroy_pipeline(d->dyncube_ssr_pipeline); d->dyncube_ssr_pipeline = {}; }
    }
    DR sampler_r = {0,0,0,1,DS::all_compute,1,DT::sampler};
    DR cbv_r     = {0,0,0,1,DS::all_compute,1,DT::constant_buffer}; // b0 cb_scene
    DR srv_r     = {0,0,0,4,DS::all_compute,1,DT::texture_shader_resource_view}; // t0 color, t1 depth, t2 mrt_normal, t3 IS-FAST noise
    DR uav_r     = {0,0,0,1,DS::all_compute,1,DT::texture_unordered_access_view}; // u0 ssr_result
    reshade::api::constant_range push_range = {};
    push_range.binding = 0;
    push_range.dx_register_index = 13;
    push_range.dx_register_space = 0;
    push_range.count = 18; // sampleCount, maxDist, thickness, distanceFade, edgeFade, grazingFade, charOccStrength, charOccUpness, isfastEnabled, isfastBound, isfastFrame, isfastStrength, isfastSpatial, isfastTemporal, isfastSeed, charComp, charShift, charInvert
    push_range.visibility = DS::all_compute;
    P p0, p1, p2, p3, pPush;
    p0.type = reshade::api::pipeline_layout_param_type::descriptor_table; p0.descriptor_table.count = 1; p0.descriptor_table.ranges = &sampler_r;
    p1.type = reshade::api::pipeline_layout_param_type::descriptor_table; p1.descriptor_table.count = 1; p1.descriptor_table.ranges = &cbv_r;
    p2.type = reshade::api::pipeline_layout_param_type::descriptor_table; p2.descriptor_table.count = 1; p2.descriptor_table.ranges = &srv_r;
    p3.type = reshade::api::pipeline_layout_param_type::descriptor_table; p3.descriptor_table.count = 1; p3.descriptor_table.ranges = &uav_r;
    pPush.type = reshade::api::pipeline_layout_param_type::push_constants; pPush.push_constants = push_range;
    P params[5] = {p0,p1,p2,p3,pPush};
    if (!dev->create_pipeline_layout(5, params, out)) return false;
    d->dyncube_ssr_layout_version = kDynCubeSSRLayoutVersion;
    return true;
  };
  if (!make_ssr_layout(&d->dyncube_ssr_layout)) return false;
  if (!ensure(d->dyncube_ssr_layout, &d->dyncube_ssr_tables, 4)) return false;
  #ifdef __FalcomSSRCS_EMBED_FILE
  if (!__FalcomSSRCS.empty()) {
    if (!mkcs(__FalcomSSRCS, d->dyncube_ssr_layout, &d->dyncube_ssr_pipeline)) {
      if (pipelog_should()) reshade::log::message(reshade::log::level::warning, "[DynCube] SSR pipeline create failed");
    }
  }
  #endif

  // -- SSR separable blur pipeline (H then V) --
  auto make_ssr_blur_layout = [&](reshade::api::pipeline_layout* out) -> bool {
    if (out->handle != 0u && d->dyncube_ssr_blur_layout_version == kDynCubeSSRBlurLayoutVersion) return true;
    if (out->handle != 0u) {
      // Stale layout shape (e.g. pre-depth-bilateral): drop layout, tables, and pipeline so they rebuild below.
      for (auto& t : d->dyncube_ssr_blur_tables) { if (t.handle) { dev->free_descriptor_table(t); t = {}; } }
      dev->destroy_pipeline_layout(*out); *out = {};
      if (d->dyncube_ssr_blur_pipeline.handle) { dev->destroy_pipeline(d->dyncube_ssr_blur_pipeline); d->dyncube_ssr_blur_pipeline = {}; }
    }
    DR sampler_r = {0,0,0,1,DS::all_compute,1,DT::sampler};
    DR srv_r     = {0,0,0,2,DS::all_compute,1,DT::texture_shader_resource_view}; // t0 raw/blur_h, t1 captured scene depth
    DR uav_r     = {0,0,0,1,DS::all_compute,1,DT::texture_unordered_access_view}; // u0 blur_h/blur
    DR cbv_r     = {0,0,0,1,DS::all_compute,1,DT::constant_buffer}; // b0 cb_scene (proj for depth unpack)
    reshade::api::constant_range push_range = {};
    push_range.binding = 0;
    push_range.dx_register_index = 13;
    push_range.dx_register_space = 0;
    push_range.count = 3; // sigma, horizontal, symmetricWeights
    push_range.visibility = DS::all_compute;
    P p0, p1, p2, p3, pPush;
    p0.type = reshade::api::pipeline_layout_param_type::descriptor_table; p0.descriptor_table.count = 1; p0.descriptor_table.ranges = &sampler_r;
    p1.type = reshade::api::pipeline_layout_param_type::descriptor_table; p1.descriptor_table.count = 1; p1.descriptor_table.ranges = &srv_r;
    p2.type = reshade::api::pipeline_layout_param_type::descriptor_table; p2.descriptor_table.count = 1; p2.descriptor_table.ranges = &uav_r;
    p3.type = reshade::api::pipeline_layout_param_type::descriptor_table; p3.descriptor_table.count = 1; p3.descriptor_table.ranges = &cbv_r;
    pPush.type = reshade::api::pipeline_layout_param_type::push_constants; pPush.push_constants = push_range;
    P params[5] = {p0,p1,p2,p3,pPush};
    if (!dev->create_pipeline_layout(5, params, out)) return false;
    d->dyncube_ssr_blur_layout_version = kDynCubeSSRBlurLayoutVersion;
    return true;
  };
  if (!make_ssr_blur_layout(&d->dyncube_ssr_blur_layout)) return false;
  if (!ensure(d->dyncube_ssr_blur_layout, &d->dyncube_ssr_blur_tables, 4)) return false;
  #ifdef __FalcomSSRBlurCS_EMBED_FILE
  if (!__FalcomSSRBlurCS.empty()) {
    if (!mkcs(__FalcomSSRBlurCS, d->dyncube_ssr_blur_layout, &d->dyncube_ssr_blur_pipeline)) {
      if (pipelog_should()) reshade::log::message(reshade::log::level::warning, "[DynCube] SSR blur pipeline create failed");
    }
  }
  #endif

  // -- World-fixed parallax bounds reduction pipeline --
  // 4 SRVs (pos, contrib, charmask, camCur), 2 UAVs (scratch, bounds), 5 push floats
  auto make_worldbox_layout = [&](reshade::api::pipeline_layout* out) -> bool {
    if (out->handle != 0u && d->dyncube_worldbox_layout_version == kDynCubeWorldBoxLayoutVersion) return true;
    if (out->handle != 0u) {
      // Stale layout shape (e.g. pre-contrib-threshold): drop layout, tables, and pipeline so they rebuild below.
      for (auto& t : d->dyncube_worldbox_tables) { if (t.handle) { dev->free_descriptor_table(t); t = {}; } }
      dev->destroy_pipeline_layout(*out); *out = {};
      if (d->dyncube_worldbox_pipeline.handle) { dev->destroy_pipeline(d->dyncube_worldbox_pipeline); d->dyncube_worldbox_pipeline = {}; }
    }
    DR srv_r     = {0,0,0,4,DS::all_compute,1,DT::texture_shader_resource_view}; // t0..t3 (pos, contrib, charmask, camCur)
    DR srv_buf_r = {0,0,0,3,DS::all_compute,1,DT::buffer_unordered_access_view}; // u0..u2 (scratch, bounds, faceExt)
    reshade::api::constant_range push_range = {};
    push_range.binding = 0;
    push_range.dx_register_index = 13;
    push_range.dx_register_space = 0;
    push_range.count = 5; // pass, posScale, reset, scratchCount, contribThreshold
    push_range.visibility = DS::all_compute;
    P p0, p1, pPush;
    p0.type = reshade::api::pipeline_layout_param_type::descriptor_table; p0.descriptor_table.count = 1; p0.descriptor_table.ranges = &srv_r;
    p1.type = reshade::api::pipeline_layout_param_type::descriptor_table; p1.descriptor_table.count = 1; p1.descriptor_table.ranges = &srv_buf_r;
    pPush.type = reshade::api::pipeline_layout_param_type::push_constants; pPush.push_constants = push_range;
    P params[3] = {p0,p1,pPush};
    if (!dev->create_pipeline_layout(3, params, out)) return false;
    d->dyncube_worldbox_layout_version = kDynCubeWorldBoxLayoutVersion;
    return true;
  };
  if (!make_worldbox_layout(&d->dyncube_worldbox_layout)) return false;
  if (!ensure(d->dyncube_worldbox_layout, &d->dyncube_worldbox_tables, 2)) return false;
  #ifdef __DynCubeBoundsReduceCS_EMBED_FILE
  if (!__DynCubeBoundsReduceCS.empty()) {
    if (!mkcs(__DynCubeBoundsReduceCS, d->dyncube_worldbox_layout, &d->dyncube_worldbox_pipeline)) {
      if (pipelog_should()) reshade::log::message(reshade::log::level::warning, "[DynCube] World-box pipeline create failed");
    }
  }
  #endif
  return true;
}

// Null the compute-stage slots used by the DynCube passes so the D3D11 runtime releases
// its references (otherwise the bound SRVs/UAVs keep the textures alive and
// destroy_resource cannot free them on resize).
static void UnbindDynCubeComputeState(reshade::api::command_list* cl) {
  if (!cl) return;
  reshade::api::resource_view null_srv = {};
  reshade::api::resource_view null_uav = {};
  reshade::api::sampler null_sampler = {};
  cl->push_descriptors(reshade::api::shader_stage::all_compute, reshade::api::pipeline_layout{0}, 0,
      reshade::api::descriptor_table_update{{}, 0, 0, 1, reshade::api::descriptor_type::sampler, &null_sampler});
  for (int i = 0; i <= 6; ++i)
    cl->push_descriptors(reshade::api::shader_stage::all_compute, reshade::api::pipeline_layout{0}, 0,
        reshade::api::descriptor_table_update{{}, (uint32_t)i, 0, 1, reshade::api::descriptor_type::texture_shader_resource_view, &null_srv});
  for (int i = 0; i <= 4; ++i)
    cl->push_descriptors(reshade::api::shader_stage::all_compute, reshade::api::pipeline_layout{0}, 0,
        reshade::api::descriptor_table_update{{}, (uint32_t)i, 0, 1, reshade::api::descriptor_type::texture_unordered_access_view, &null_uav});
}

static bool RunDynCubeSolid(reshade::api::command_list* cl, DeviceData* d) {
  // Writes a DEDICATED solid-color cube � never the history/ggx resources.
  if (!cl || !d) return false;
  auto* dev = cl->get_device();
  if (!CreateDynCubeSolidResources(dev, d)) return false;
  if (!CreateDynCubePipelinesIfNeeded(dev, d)) return false;
  if (!d->dyncube_solid_pipeline.handle) return false;

  cl->bind_pipeline(reshade::api::pipeline_stage::all_compute, d->dyncube_solid_pipeline);
  auto* tbl = &d->dyncube_solid_tables;
  reshade::api::descriptor_table_update upd = {tbl->at(0), 0, 0, 1, reshade::api::descriptor_type::texture_unordered_access_view, &d->dyncube_solid_cube_uav};
  dev->update_descriptor_tables(1, &upd);
  cl->bind_descriptor_tables(reshade::api::shader_stage::all_compute, d->dyncube_solid_layout, 0, 1, &tbl->at(0));
  uint32_t sz = d->dyncube_size;
  cl->dispatch((sz + 7)/8, (sz + 7)/8, 6);
  cl->barrier(d->dyncube_solid_cube, reshade::api::resource_usage::unordered_access, reshade::api::resource_usage::shader_resource);
  d->dyncube_solid_written = true;
  return true;
}

// Delayed-validate commit: promote the just-written set to the consumer read set
// and advance the write cursor. Call ONLY with a validated set, except for the
// reduction-unavailable fail-safe path (which preserves pre-protection behavior).
// Aliases, t29, previews, and GGX input all follow readSet � never the raw write set.
static void PromoteDynCubeReadSet(DeviceData* d) {
  if (!d) return;
  d->dyncube_readSet = d->dyncube_hist_cur;
  d->dyncube_hist_cur = 1u - d->dyncube_hist_cur;
  d->dyncube_texture = d->dyncube_hist[d->dyncube_readSet].color;
  d->dyncube_srv = d->dyncube_hist[d->dyncube_readSet].color_cube_srv;
  d->dyncube_uav = d->dyncube_hist[d->dyncube_readSet].color_uav;
  d->dyncube_hasValidRead = true;
}

// Delayed-validate commit: consume the staged hasGeom bit (bounds[1].w, float index 7)
// from the previous capture. Runs at scheduler entry before any new capture/filter work,
// so the staged copy always describes the latest capture (no epochs needed). Blocking map
// is safe: staged data is >=1 present old by construction. On map failure the pending flag
// is kept and the system stays frozen (safe direction); a later copy overwrites the slot.
static void ConsumeDynCubeStagedValidity(reshade::api::device* dev, DeviceData* dd) {
  if (!dev || !dd || !dd->dyncube_boxCopyPending || !dd->dyncube_validStaging.handle) return;
  void* staged = nullptr;
  if (!dev->map_buffer_region(dd->dyncube_validStaging, 0, 32, reshade::api::map_access::read_only, &staged) || staged == nullptr) {
    return;
  }
  dd->dyncube_boxCopyPending = false;
  const bool validNow = (reinterpret_cast<const float*>(staged)[7] > 0.5f);
  // WorldBox diagnostic: report the full active bounds from the staged copy.
  // Staging holds ONLY the final merged (persistent) box: float[0..2] = min,
  // float[3] = latched valid, float[4..6] = max, float[7] = this-capture hasGeom.
  // A hasGeom=0 line therefore means "persistent history only, no new geometry".
  // NOTE: no CPU camera copy exists (camCur lives only in GPU 1x1 textures and
  // the scene CBV is a GPU buffer reference), so record the camera manually
  // alongside this log. No new GPU readback was added for this diagnostic.
  if (shader_injection.dynCube_debug_logging > 0.5f) {
    float box[8];
    for (int i = 0; i < 8; ++i) box[i] = (reinterpret_cast<const float*>(staged))[i];
    static float lastBox[8] = {0};
    static bool boxLogInit = false;
    bool boxChanged = !boxLogInit;
    for (int i = 0; !boxChanged && i < 8; ++i) boxChanged = (box[i] != lastBox[i]);
    if (boxChanged) {
      for (int i = 0; i < 8; ++i) lastBox[i] = box[i];
      boxLogInit = true;
      const std::string msg =
        "[DynCube] WorldBox: min=(" + std::to_string(box[0]) + ", " + std::to_string(box[1]) + ", " + std::to_string(box[2]) + ") "
        "max=(" + std::to_string(box[4]) + ", " + std::to_string(box[5]) + ", " + std::to_string(box[6]) + ") "
        "size=(" + std::to_string(box[4] - box[0]) + ", " + std::to_string(box[5] - box[1]) + ", " + std::to_string(box[6] - box[2]) + ") "
        "valid=" + std::to_string((box[3] > 0.5f) ? 1 : 0) + " hasGeom=" + std::to_string((box[7] > 0.5f) ? 1 : 0);
      reshade::log::message(reshade::log::level::info, msg.c_str());
    }
  }
  dev->unmap_buffer_region(dd->dyncube_validStaging);
  // Face-extent diagnostic: [0]=(+X,+Y,+Z,mask) [1]=(-X,-Y,-Z,spare), mask bit
  // order +X,-X,+Y,-Y,+Z,-Z. "(fb)" marks a side using the safety fallback.
  if (shader_injection.dynCube_debug_logging > 0.5f && dd->dyncube_faceExtStaging.handle) {
    void* stagedExt = nullptr;
    if (dev->map_buffer_region(dd->dyncube_faceExtStaging, 0, 32, reshade::api::map_access::read_only, &stagedExt) && stagedExt != nullptr) {
      float fx[8];
      for (int i = 0; i < 8; ++i) fx[i] = (reinterpret_cast<const float*>(stagedExt))[i];
      dev->unmap_buffer_region(dd->dyncube_faceExtStaging);
      static float lastFx[8] = {0};
      static bool fxLogInit = false;
      bool fxChanged = !fxLogInit;
      for (int i = 0; !fxChanged && i < 8; ++i) fxChanged = (fx[i] != lastFx[i]);
      if (fxChanged) {
        for (int i = 0; i < 8; ++i) lastFx[i] = fx[i];
        fxLogInit = true;
        uint32_t mask = 0u;
        memcpy(&mask, &fx[3], sizeof(mask));
        auto extStr = [&](float v, uint32_t bit) -> std::string {
          std::string s = std::to_string(v);
          if ((mask & bit) == 0u) s += "(fb)";
          return s;
        };
        const std::string msg =
          std::string("[DynCube] WorldBoxFaces: +X=") + extStr(fx[0], 1u) +
          " -X=" + extStr(fx[4], 2u) +
          " +Y=" + extStr(fx[1], 4u) +
          " -Y=" + extStr(fx[5], 8u) +
          " +Z=" + extStr(fx[2], 16u) +
          " -Z=" + extStr(fx[6], 32u);
        reshade::log::message(reshade::log::level::info, msg.c_str());
      }
    }
  }
  if (validNow) {
    PromoteDynCubeReadSet(dd);
    if (dd->dyncube_wasRejected && shader_injection.dynCube_debug_logging > 0.5f) {
      reshade::log::message(reshade::log::level::info, "[DynCube] capture validation resumed (hasGeom valid)");
    }
    dd->dyncube_wasRejected = false;
  } else {
    // Rejected: readSet/aliases/filter input untouched; the scratch set will be
    // overwritten by the next capture (self-cleaning). Limitation: an opaque loading
    // backdrop that writes valid depth still yields hasGeom=true and cannot be
    // rejected by geometry coverage (indistinguishable from real vista geometry).
    ++dd->dyncube_rejected_captures;
    dd->dyncube_rejectedGap = true;  // arm one-shot fast-forward: next dispatched capture hard-replaces stale history
    CSLog("dyncube", "capture REJECTED (no valid geometry)", true);
    if (!dd->dyncube_wasRejected && shader_injection.dynCube_debug_logging > 0.5f) {
      reshade::log::message(reshade::log::level::info, "[DynCube] capture rejected (no valid geometry)");
    }
    dd->dyncube_wasRejected = true;
  }
}

static bool RunDynCubeCapture(reshade::api::command_list* cl, DeviceData* d) {
  if (!cl || !d) return false;
  if (!d->dyncube_resources_created) {
    uint32_t sz = DynCubeResolveSize(shader_injection.dynCube_resolution);
    if (!CreateDynCubeResources(cl->get_device(), d, sz)) return false;
  }
  // Phase 1: require live depth+color+mrt+cbv (rawDepth >=1-1e-5 reject). Liveness flags
  // come from capture edges (bind = live) and destroy events (free = dead).
  if (!d->captured_depth_srv.handle || !d->captured_color_srv.handle || !d->captured_scene_cbv_valid
      || !d->captured_depth_live || !d->captured_color_live || !d->captured_cbv_live
      || !d->captured_mrt_live) {
    CSLog("dyncube", "capture SKIP (missing or dead inputs)", true);
    return false;
  }
  auto* dev = cl->get_device();
  if (!CreateDynCubePipelinesIfNeeded(dev, d)) return false;
  if (!d->dyncube_capture_pipeline.handle) return false;

  const uint32_t cur = d->dyncube_hist_cur;
  const uint32_t prev = 1u - cur;

  // Reset: clear all history + camera UAVs (resource recreate / enable transition)
  if (d->dyncube_needs_reset) {
    float zero4[4] = {0, 0, 0, 0};
    float zero1[4] = {0, 0, 0, 0};
    for (auto& set : d->dyncube_hist) {
      if (set.color_uav.handle) cl->clear_unordered_access_view_float(set.color_uav, zero4);
      if (set.pos_uav.handle) cl->clear_unordered_access_view_float(set.pos_uav, zero4);
      if (set.contrib_uav.handle) cl->clear_unordered_access_view_float(set.contrib_uav, zero1);
    }
    for (auto& u : d->dyncube_cam_uav) if (u.handle) cl->clear_unordered_access_view_float(u, zero4);
    d->dyncube_needs_reset = false;
  }

  cl->bind_pipeline(reshade::api::pipeline_stage::all_compute, d->dyncube_capture_pipeline);
  auto* tbl = &d->dyncube_capture_tables;

  // Previous set as SRVs (t2 prevColor, t3 prevPos, t4 prevContrib, t5 camPrev) + depth/color (t0/t1) + mrt0 (t6) + vanilla (t7, fallback paint)
  reshade::api::resource_view srvs[8] = {
      d->captured_depth_srv,
      d->captured_color_srv,
      d->dyncube_hist[prev].color_arr_srv,
      d->dyncube_hist[prev].pos_arr_srv,
      d->dyncube_hist[prev].contrib_arr_srv,
      d->dyncube_cam_srv[prev],
      d->captured_mrt_normal_srv,  // mrtTexture0 for character mask
      d->captured_vanilla_env_srv,  // game vanilla cube (may be null pre-first-capture: reads 0, today's black)
  };
  // Current set as UAVs (u0 curColor, u1 curPos, u2 curContrib, u3 camCur, u4 charmask)
  reshade::api::resource_view uavs[5] = {
      d->dyncube_hist[cur].color_uav,
      d->dyncube_hist[cur].pos_uav,
      d->dyncube_hist[cur].contrib_uav,
      d->dyncube_cam_uav[cur],
      d->dyncube_charmask_uav,
  };
  reshade::api::descriptor_table_update ups[4];
  ups[0] = {tbl->at(0), 0, 0, 1, reshade::api::descriptor_type::sampler, &d->dyncube_sampler};
  ups[1] = {tbl->at(1), 0, 0, 1, reshade::api::descriptor_type::constant_buffer, &d->captured_scene_cbv_view};
  ups[2] = {tbl->at(2), 0, 0, 8, reshade::api::descriptor_type::texture_shader_resource_view, srvs};
  ups[3] = {tbl->at(3), 0, 0, 5, reshade::api::descriptor_type::texture_unordered_access_view, uavs};
  dev->update_descriptor_tables(4, ups);
  std::array<reshade::api::descriptor_table, 4> tables = {tbl->at(0), tbl->at(1), tbl->at(2), tbl->at(3)};
  cl->bind_descriptor_tables(reshade::api::shader_stage::all_compute, d->dyncube_capture_layout, 0, 4, tables.data());

  // Push constants (b13): boost, blend, posThreshold(world), posScale, reset, characterCapture, charMaskAvailable, charComp, charShift, soften, charInvert
  {
    const float posScale = 0.001f;
    float reset = (shader_injection.dynCube_history < 0.5f) ? 1.0f : 0.0f;
    float charCapture = (shader_injection.dynCube_character_capture > 0.5f) ? 1.0f : 0.0f;
    float charMaskAvail = (d->captured_mrt_normal_srv.handle) ? 1.0f : 0.0f;
    // One-shot fast-forward: first dispatched capture after a rejection gap (or a
    // capture-content settings change) uses blend=1.0, which is exactly the
    // hard-replace branch outputs (no lerp with stale pre-gap history).
    // Consumed here so exactly one capture sees it.
    const bool fastForward = d->dyncube_rejectedGap || d->dyncube_dirtyFastForward;
    d->dyncube_rejectedGap = false;
    d->dyncube_dirtyFastForward = false;
    float pc[12] = {
        std::clamp(shader_injection.dynCube_capture_boost, 0.f, 8.f),
        fastForward ? 1.0f : std::clamp(shader_injection.dynCube_history_blend, 0.f, 1.f),
        std::max(0.f, shader_injection.dynCube_history_pos_threshold),
        posScale,
        reset,
        charCapture,
        charMaskAvail,
        // Character-bit location: Sora mrt.w bit 0; Kai (mrt.z >> 8) bit 0; Sora1st mrt.w bit 3, inverted.
        IsKai() ? 1.f : 0.f,
        IsKai() ? 8.f : (IsSora1st() ? 3.f : 0.f),
        std::clamp(shader_injection.dynCube_capture_soften, 0.f, 1.f),
        IsSora1st() ? 1.f : 0.f,
        (shader_injection.dynCube_sparkle_rejection > 0.5f) ? 1.f : 0.f,
    };
    cl->push_constants(reshade::api::shader_stage::all_compute, d->dyncube_capture_layout, 4, 0, 12, pc);
  }

  uint32_t sz = d->dyncube_size;
  cl->dispatch((sz + 7)/8, (sz + 7)/8, 6);
  ++d->dyncube_capture_dispatches;

  // Barrier current history + camera UAVs -> SRV (t17 reads current set, preview + next-frame reads too)
  cl->barrier(d->dyncube_hist[cur].color, reshade::api::resource_usage::unordered_access, reshade::api::resource_usage::shader_resource);
  cl->barrier(d->dyncube_hist[cur].pos, reshade::api::resource_usage::unordered_access, reshade::api::resource_usage::shader_resource);
  cl->barrier(d->dyncube_hist[cur].contrib, reshade::api::resource_usage::unordered_access, reshade::api::resource_usage::shader_resource);
  cl->barrier(d->dyncube_cam[cur], reshade::api::resource_usage::unordered_access, reshade::api::resource_usage::shader_resource);

  // World-box reduction doubles as the capture-validity detector: it must run for
  // every capture regardless of the World Fixed toggle (lighting use stays gated).
  // Charmask transition for the reduction read, restored afterwards.
  // NOTE: no swap/alias promotion here � the consumer readSet advances only via
  // delayed validation (see scheduler consume), so an invalid capture can never
  // become t29/t17/filter input. The write cursor advances on promotion only,
  // which keeps rejected scratch sets disposable (overwritten by the next capture).
  cl->barrier(d->dyncube_charmask, reshade::api::resource_usage::unordered_access, reshade::api::resource_usage::shader_resource);
  if (!RunDynCubeWorldBox(cl, d, cur)) {
    // Reduction unavailable: fall back to immediate promotion (pre-protection behavior).
    CSLog("dyncube", "worldbox UNAVAILABLE: immediate-promote fallback", true);
    PromoteDynCubeReadSet(d);
  }
  cl->barrier(d->dyncube_charmask, reshade::api::resource_usage::shader_resource, reshade::api::resource_usage::unordered_access);

  d->dyncube_solid_written = false; // capture ran (aliases still track the validated readSet)
  UnbindDynCubeComputeState(cl);
  return true;
}

// World-fixed parallax bounds reduction (Sora2nd v1). Reads the just-written
// history set (write-cursor index `set`): pos/contrib/charmask array SRVs + camCur.
// Two passes: per-group partials into scratch, then a single-group expand-only
// merge into the persistent bounds (camera included unfiltered for containment).
// Runs for every capture (also doubles as the capture-validity detector for
// delayed-validate commit); lighting use of the bounds stays toggle-gated.
static bool RunDynCubeWorldBox(reshade::api::command_list* cl, DeviceData* d, uint32_t set) {
  if (!cl || !d || set > 1u) return false;
  if (!d->dyncube_resources_created) return false;
  if (!d->dyncube_worldbox_pipeline.handle) return false;
  if (!d->dyncube_hist[set].pos_arr_srv.handle
      || !d->dyncube_hist[set].contrib_arr_srv.handle
      || !d->dyncube_charmask_arr_srv.handle
      || !d->dyncube_cam_srv[set].handle
      || !d->dyncube_worldbox_scratch_uav.handle
      || !d->dyncube_worldbox_scratch_srv.handle
      || !d->dyncube_worldbox_bounds_uav.handle
      || !d->dyncube_faceextents_uav.handle
      || !d->dyncube_validStaging.handle
      || !d->dyncube_faceExtStaging.handle) return false;
  auto* dev = cl->get_device();

  const uint32_t sz = d->dyncube_size;
  const uint32_t g = (sz + 7u) / 8u;   // groups per face axis (matches capture dispatch)
  const uint32_t groups = g * g * 6u;  // total pass-0 groups

  cl->bind_pipeline(reshade::api::pipeline_stage::all_compute, d->dyncube_worldbox_pipeline);
  auto* tbl = &d->dyncube_worldbox_tables;
  reshade::api::resource_view srvs[4] = {
      d->dyncube_hist[set].pos_arr_srv,
      d->dyncube_hist[set].contrib_arr_srv,
      d->dyncube_charmask_arr_srv,
      d->dyncube_cam_srv[set],
  };
  reshade::api::resource_view uavs[3] = {
      d->dyncube_worldbox_scratch_uav,
      d->dyncube_worldbox_bounds_uav,
      d->dyncube_faceextents_uav,
  };
  reshade::api::descriptor_table_update ups[2];
  ups[0] = {tbl->at(0), 0, 0, 4, reshade::api::descriptor_type::texture_shader_resource_view, srvs};
  ups[1] = {tbl->at(1), 0, 0, 3, reshade::api::descriptor_type::buffer_unordered_access_view, uavs};
  dev->update_descriptor_tables(2, ups);
  std::array<reshade::api::descriptor_table, 2> tables = {tbl->at(0), tbl->at(1)};
  cl->bind_descriptor_tables(reshade::api::shader_stage::all_compute, d->dyncube_worldbox_layout, 0, 2, tables.data());

  const float reset = d->dyncube_worldbox_reset_pending ? 1.0f : 0.0f;
  // Pass 0: per-group partials. Scratch must be UAV-writable.
  {
    float pc[5] = {0.0f, 0.001f, reset, (float)groups,
        std::clamp(shader_injection.dynCube_worldbox_contrib, 0.f, 1.f)};
    cl->push_constants(reshade::api::shader_stage::all_compute, d->dyncube_worldbox_layout, 2, 0, 5, pc);
    cl->barrier(d->dyncube_worldbox_scratch, reshade::api::resource_usage::shader_resource, reshade::api::resource_usage::unordered_access);
    CSLog("dyncube", std::string("worldbox pass0 dispatch groups=") + std::to_string(g) +
      (reset > 0.5f ? " RESET" : ""));
    cl->dispatch(g, g, 6);
  }
  // Pass 1: single-group merge. Scratch UAV->SRV, bounds SRV->UAV, then merge,
  // then bounds UAV->SRV so lighting (t33) reads the finished result.
  {
    float pc[5] = {1.0f, 0.001f, reset, (float)groups,
        std::clamp(shader_injection.dynCube_worldbox_contrib, 0.f, 1.f)};
    cl->push_constants(reshade::api::shader_stage::all_compute, d->dyncube_worldbox_layout, 2, 0, 5, pc);
    cl->barrier(d->dyncube_worldbox_scratch, reshade::api::resource_usage::unordered_access, reshade::api::resource_usage::shader_resource);
    cl->barrier(d->dyncube_worldbox_bounds, reshade::api::resource_usage::shader_resource, reshade::api::resource_usage::unordered_access);
    cl->barrier(d->dyncube_faceextents, reshade::api::resource_usage::shader_resource, reshade::api::resource_usage::unordered_access);
    CSLog("dyncube", "worldbox pass1 merge dispatch");
    cl->dispatch(1, 1, 1);
    cl->barrier(d->dyncube_worldbox_bounds, reshade::api::resource_usage::unordered_access, reshade::api::resource_usage::shader_resource);
  }
  // Stage bounds for delayed-validate commit (consumed next scheduler entry; the
  // per-frame hasGeom bit lives in staged bounds[1].w, float index 7).
  cl->barrier(d->dyncube_worldbox_bounds, reshade::api::resource_usage::shader_resource, reshade::api::resource_usage::copy_source);
  cl->copy_resource(d->dyncube_worldbox_bounds, d->dyncube_validStaging);
  cl->barrier(d->dyncube_worldbox_bounds, reshade::api::resource_usage::copy_source, reshade::api::resource_usage::shader_resource);
  // Stage per-face extents for the WorldBoxFaces diagnostic log.
  cl->barrier(d->dyncube_faceextents, reshade::api::resource_usage::unordered_access, reshade::api::resource_usage::copy_source);
  cl->copy_resource(d->dyncube_faceextents, d->dyncube_faceExtStaging);
  cl->barrier(d->dyncube_faceextents, reshade::api::resource_usage::copy_source, reshade::api::resource_usage::unordered_access);
  d->dyncube_boxCopyPending = true;
  d->dyncube_worldbox_reset_pending = false;

  UnbindDynCubeComputeState(cl);
  return true;
}

// Phase 3: filter the freshly captured history cube into a multi-mip roughness chain.
// ggxOn = false -> hardware GenerateMips (box-filter) path.
// ggxOn = true  -> GGX NDF importance-sampled mips 1..N-1 (mip0 stays a sharp copy).
// Source = the just-written history color cube (dyncube_texture). Output = the BUILDING
// ggx_out cube (index 1 - ggx_active); the caller swaps ggx_active on success so a
// partially-written cube is never bound to t17.
static bool RunDynCubeFilter(reshade::api::command_list* cl, DeviceData* d, bool ggxOn) {
  if (!cl || !d) return false;
  if (!d->dyncube_resources_created) return false;
  if (!d->dyncube_texture.handle) return false;
  auto* dev = cl->get_device();
  if (!CreateDynCubePipelinesIfNeeded(dev, d)) return false;

  const uint32_t mips = d->dyncube_mip_count;
  const uint32_t sz = d->dyncube_size;
  const uint32_t building = 1u - d->dyncube_ggx_active;
  if (mips < 2) return false;
  ++d->dyncube_filter_updates;

  reshade::api::resource src = d->dyncube_texture;
  reshade::api::resource dst = d->dyncube_ggx_out[building];

  // Copy the 6 history faces into mip0 of a target cube (dst sub = face * mips).
  auto copy_mip0 = [&](reshade::api::resource dstRes) -> bool {
    if (!dstRes.handle) return false;
    cl->barrier(src, reshade::api::resource_usage::shader_resource, reshade::api::resource_usage::copy_source);
    cl->barrier(dstRes, reshade::api::resource_usage::shader_resource, reshade::api::resource_usage::copy_dest);
    reshade::api::subresource_box box = {0,0,0, sz, sz, 1};
    for (uint32_t f = 0; f < 6; ++f) {
      cl->copy_texture_region(src, f, &box, dstRes, f * mips, &box, reshade::api::filter_mode::min_mag_mip_point);
      ++d->dyncube_face_copies;
    }
    cl->barrier(src, reshade::api::resource_usage::copy_source, reshade::api::resource_usage::shader_resource);
    cl->barrier(dstRes, reshade::api::resource_usage::copy_dest, reshade::api::resource_usage::shader_resource);
    return true;
  };

  if (!ggxOn) {
    // Hardware box-filtered mips: mip0 = sharp history copy, rest = GenerateMips.
    if (!copy_mip0(dst)) {
      CSLog("dyncube", "filter FAILED: mip0 face copy", true);
      return false;
    }
    cl->generate_mipmaps(d->dyncube_ggx_out_cube_srv[building]);
    return true;
  }

  // GGX path: build the input chain (mip0 + hardware mips) on a separate resource to
  // avoid the SRV/UAV same-resource hazard during filtering.
  if (!d->dyncube_ggx_in.handle || !d->dyncube_ggx_in_cube_srv.handle) return false;
  if (!copy_mip0(d->dyncube_ggx_in)) return false;
  cl->generate_mipmaps(d->dyncube_ggx_in_cube_srv);
  if (!copy_mip0(dst)) return false;

  if (!d->dyncube_ggx_pipeline.handle) return false;
  cl->bind_pipeline(reshade::api::pipeline_stage::all_compute, d->dyncube_ggx_pipeline);
  auto* gt = &d->dyncube_ggx_tables;

  // Filter mips 1..N-1, roughness = mip / (N-1) (matches Skyrim reference).
  cl->barrier(dst, reshade::api::resource_usage::shader_resource, reshade::api::resource_usage::unordered_access);
  const float delta = 1.0f / float(mips - 1);
  for (uint32_t m = 1; m < mips; ++m) {
    if (!d->dyncube_ggx_out_mip_uav[building][m].handle) continue;
    reshade::api::descriptor_table_update gu[3] = {
        {gt->at(0), 0, 0, 1, reshade::api::descriptor_type::sampler, &d->dyncube_linear_sampler},
        {gt->at(1), 0, 0, 1, reshade::api::descriptor_type::texture_shader_resource_view, &d->dyncube_ggx_in_cube_srv},
        {gt->at(2), 0, 0, 1, reshade::api::descriptor_type::texture_unordered_access_view, &d->dyncube_ggx_out_mip_uav[building][m]},
    };
    dev->update_descriptor_tables(3, gu);
    std::array<reshade::api::descriptor_table, 3> gtables = {gt->at(0), gt->at(1), gt->at(2)};
    cl->bind_descriptor_tables(reshade::api::shader_stage::all_compute, d->dyncube_ggx_layout, 0, 3, gtables.data());
    float rough = (float)m * delta;
    cl->push_constants(reshade::api::shader_stage::all_compute, d->dyncube_ggx_layout, 3, 0, 1, &rough);
    uint32_t mw = std::max(1u, sz >> m);
    cl->dispatch((mw + 7u) / 8u, (mw + 7u) / 8u, 6);
    ++d->dyncube_ggx_mip_dispatches;
  }
  cl->barrier(dst, reshade::api::resource_usage::unordered_access, reshade::api::resource_usage::shader_resource);
  UnbindDynCubeComputeState(cl);
  return true;
}

// Global-push variant build: resample the served filtered cube (ggx_out[active])
// at a fractional mip (soften) and scale it (strength) into the variant cube,
// served ONLY on non-lighting t17 pushes. Lighting always samples sharp.
// Runs on rebuild, never per frame.
static bool RunDynCubeVariant(reshade::api::command_list* cl, DeviceData* d) {
  static const float kVariantMaxBlurMip = 3.0f;  // soften=1 resamples this source LOD
  if (!cl || !d) return false;
  if (!d->dyncube_ggx_valid) return false;
  if (!d->dyncube_variant.handle || !d->dyncube_variant_cube_srv.handle
      || !d->dyncube_variant_mip0_uav.handle) return false;
  if (!CreateDynCubePipelinesIfNeeded(cl->get_device(), d)) return false;
  if (!d->dyncube_variant_pipeline.handle) return false;
  auto* dev = cl->get_device();
  const uint32_t mips = d->dyncube_mip_count;
  const uint32_t sz = d->dyncube_size;
  const float soften = std::clamp(shader_injection.dynCube_capture_soften, 0.f, 1.f);
  const float strength = std::clamp(shader_injection.dynCube_global_strength, 0.f, 1.f);
  const float srcMip = std::min(soften * kVariantMaxBlurMip, (float)std::max(mips, 1u) - 1.f);
  cl->bind_pipeline(reshade::api::pipeline_stage::all_compute, d->dyncube_variant_pipeline);
  auto* vt = &d->dyncube_variant_tables;
  reshade::api::descriptor_table_update vu[3] = {
      {vt->at(0), 0, 0, 1, reshade::api::descriptor_type::sampler, &d->dyncube_linear_sampler},
      {vt->at(1), 0, 0, 1, reshade::api::descriptor_type::texture_shader_resource_view, &d->dyncube_ggx_out_cube_srv[d->dyncube_ggx_active]},
      {vt->at(2), 0, 0, 1, reshade::api::descriptor_type::texture_unordered_access_view, &d->dyncube_variant_mip0_uav},
  };
  dev->update_descriptor_tables(3, vu);
  std::array<reshade::api::descriptor_table, 3> vtables = {vt->at(0), vt->at(1), vt->at(2)};
  cl->bind_descriptor_tables(reshade::api::shader_stage::all_compute, d->dyncube_variant_layout, 0, 3, vtables.data());
  float pc[2] = {srcMip, strength};
  cl->push_constants(reshade::api::shader_stage::all_compute, d->dyncube_variant_layout, 3, 0, 2, pc);
  cl->barrier(d->dyncube_variant, reshade::api::resource_usage::shader_resource, reshade::api::resource_usage::unordered_access);
  cl->dispatch((sz + 7u) / 8u, (sz + 7u) / 8u, 6);
  cl->barrier(d->dyncube_variant, reshade::api::resource_usage::unordered_access, reshade::api::resource_usage::shader_resource);
  cl->generate_mipmaps(d->dyncube_variant_cube_srv);
  UnbindDynCubeComputeState(cl);
  d->dyncube_variant_valid = true;
  d->dyncube_lastVariantSoften = soften;
  d->dyncube_lastVariantStrength = strength;
  if (shader_injection.dynCube_debug_logging > 0.5f) {
    reshade::log::message(reshade::log::level::info,
      (std::string("[DynCube] variant built: ") + std::to_string(sz) + "x" + std::to_string(sz) +
       " srcMip=" + std::to_string(srcMip) + " strength=" + std::to_string(strength)).c_str());
  }
  return true;
}

// Simple screen-space SSR (compute): march captured depth, write RGBA16F result
// (rgb = reflected frame color, a = hit). Independent of history/ggx/inferred.
static bool RunDynCubeSSR(reshade::api::command_list* cl, DeviceData* d) {
  if (!cl || !d) return false;
  // Require live inputs (flags from the present-time resolver; draw callbacks must
  // not query views). Transition reallocs leave nonzero dangling views.
  if (!d->captured_color_srv.handle || !d->captured_depth_srv.handle
      || !d->captured_mrt_normal_srv.handle || !d->captured_scene_cbv_valid
      || !d->captured_color_live || !d->captured_depth_live || !d->captured_mrt_live || !d->captured_cbv_live) {
    CSLog("dyncube", "ssr SKIP (missing or dead inputs)", true);
    return false;
  }
  auto* dev = cl->get_device();
  if (!CreateDynCubePipelinesIfNeeded(dev, d)) return false;
  if (!d->dyncube_ssr_pipeline.handle) return false;

  // -- IS-FAST noise texture (load once; GTVBAO path also loads it when active) --
  if (g_isfast_enabled > 0.5f) LoadISFASTNoiseTexture(dev, d);

  // Lazily create the full-res SSR textures (raw, blur_h, blur) sized to the captured color.
  // Cached dims (no draw-time queries): refreshed at every color capture in push context.
  uint32_t w = d->captured_color_w, h = d->captured_color_h;
  if (w == 0u || h == 0u) return false;
  auto make_tex = [&](reshade::api::resource* r, reshade::api::resource_view* srv, reshade::api::resource_view* uav,
                      const char* name) -> bool {
    if (r->handle) return true;
    reshade::api::resource_desc rd = {};
    rd.type = reshade::api::resource_type::texture_2d;
    rd.texture = {w, h, 1, 1, reshade::api::format::r16g16b16a16_float, 1};
    rd.heap = reshade::api::memory_heap::gpu_only;
    rd.usage = reshade::api::resource_usage::shader_resource | reshade::api::resource_usage::unordered_access;
    if (!dev->create_resource(rd, nullptr, reshade::api::resource_usage::shader_resource, r)) {
      reshade::log::message(reshade::log::level::error, (std::string("[DynCube] Failed to create ") + name).c_str());
      return false;
    }
    reshade::api::resource_view_desc rvd(reshade::api::resource_view_type::texture_2d,
                                         reshade::api::format::r16g16b16a16_float, 0, 1, 0, 1);
    dev->create_resource_view(*r, reshade::api::resource_usage::shader_resource, rvd, srv);
    dev->create_resource_view(*r, reshade::api::resource_usage::unordered_access, rvd, uav);
    return true;
  };
  if (!make_tex(&d->dyncube_ssr_raw, &d->dyncube_ssr_raw_srv, &d->dyncube_ssr_raw_uav, "SSR raw")) return false;
  if (!make_tex(&d->dyncube_ssr_blur_h, &d->dyncube_ssr_blur_h_srv, &d->dyncube_ssr_blur_h_uav, "SSR blur H")) return false;
  if (!make_tex(&d->dyncube_ssr_blur, &d->dyncube_ssr_blur_srv, &d->dyncube_ssr_blur_uav, "SSR blur")) return false;
  {
    // Mismatch-only (silent when matching): stale-small targets after a
    // resolution change are a shared-input divergence worth one warning.
    auto rawRes = dev->get_resource_from_view(d->dyncube_ssr_raw_srv);
    auto rawDesc = (rawRes.handle != 0u) ? dev->get_resource_desc(rawRes) : reshade::api::resource_desc{};
    if (rawDesc.texture.width != w || rawDesc.texture.height != h)
      CSLog("dyncube", std::string("ssr targets MISMATCH: color=") + std::to_string(w) + "x" + std::to_string(h) +
        " raw=" + std::to_string(rawDesc.texture.width) + "x" + std::to_string(rawDesc.texture.height), true);
  }

  // -- March pass ? ssr_raw --
  cl->bind_pipeline(reshade::api::pipeline_stage::all_compute, d->dyncube_ssr_pipeline);
  auto* st = &d->dyncube_ssr_tables;
  const reshade::api::resource_view isfastSrv =
      d->isfast_noise_srv.handle ? d->isfast_noise_srv : d->fallback_srv;
  reshade::api::resource_view srvs[4] = {d->captured_color_srv, d->captured_depth_srv, d->captured_mrt_normal_srv, isfastSrv};
  reshade::api::descriptor_table_update su[4] = {
      {st->at(0), 0, 0, 1, reshade::api::descriptor_type::sampler, &d->dyncube_sampler},
      {st->at(1), 0, 0, 1, reshade::api::descriptor_type::constant_buffer, &d->captured_scene_cbv_view},
      {st->at(2), 0, 0, 4, reshade::api::descriptor_type::texture_shader_resource_view, srvs},
      {st->at(3), 0, 0, 1, reshade::api::descriptor_type::texture_unordered_access_view, &d->dyncube_ssr_raw_uav},
  };
  dev->update_descriptor_tables(4, su);
  std::array<reshade::api::descriptor_table, 4> stables = {st->at(0), st->at(1), st->at(2), st->at(3)};
  cl->bind_descriptor_tables(reshade::api::shader_stage::all_compute, d->dyncube_ssr_layout, 0, 4, stables.data());
  // Direct SSR march parameters (replaces the old Low/Medium/High quality presets).
  const uint32_t sampleCount = (uint32_t)std::clamp((int)shader_injection.dynCube_ssr_samples, 4, 96);
  const float maxDist = std::clamp(shader_injection.dynCube_ssr_distance, 4.f, 192.f);
  // Effective IS-FAST gate: SSR toggle AND master toggle; shader falls back to hash phase otherwise.
  const float ssrIsfastEffective =
      (shader_injection.dynCube_ssr_isfast_enabled > 0.5f && g_isfast_enabled > 0.5f) ? 1.f : 0.f;
  float pc[18] = {
      (float)sampleCount,
      maxDist,
      std::clamp(shader_injection.dynCube_ssr_thickness, 0.f, 10.f),
      std::clamp(shader_injection.dynCube_ssr_distance_fade, 0.f, 1.f),
      std::clamp(shader_injection.dynCube_ssr_edge_fade, 0.f, 1.f),
      std::clamp(shader_injection.dynCube_ssr_grazing_fade, 0.f, 1.f),
      std::clamp(shader_injection.dynCube_ssr_char_occ_strength, 0.f, 1.f),
      std::clamp(shader_injection.dynCube_ssr_char_occ_upness, 0.f, 1.f),
      ssrIsfastEffective,
      d->isfast_texture_loaded ? 1.f : 0.f,
      (float)(d->frame_index % 64u),
      std::clamp(shader_injection.dynCube_ssr_isfast_strength, 0.f, 1.f),
      std::clamp(shader_injection.dynCube_ssr_isfast_spatial, 0.25f, 4.f),
      std::clamp(shader_injection.dynCube_ssr_isfast_temporal, 0.f, 5.f),
      std::clamp(g_isfast_seed_offset, 0.f, 64.f),
      // Character-bit location: Sora mrt.w bit 0; Kai (mrt.z >> 8) bit 0; Sora1st mrt.w bit 3, inverted.
      IsKai() ? 1.f : 0.f,
      IsKai() ? 8.f : (IsSora1st() ? 3.f : 0.f),
      IsSora1st() ? 1.f : 0.f,
  };
  cl->push_constants(reshade::api::shader_stage::all_compute, d->dyncube_ssr_layout, 4, 0, 18, pc);
  cl->dispatch((w + 7u) / 8u, (h + 7u) / 8u, 1);
  cl->barrier(d->dyncube_ssr_raw, reshade::api::resource_usage::unordered_access, reshade::api::resource_usage::shader_resource);

  // -- Separable bilateral blur: H (raw ? blur_h), V (blur_h ? blur) --
  // Both passes reference ORIGINAL captured scene depth (not blurred intermediates).
  if (!d->dyncube_ssr_blur_pipeline.handle) return false;
  const float sigma = std::clamp(shader_injection.dynCube_ssr_blur, 0.f, 8.f);
  auto* bt = &d->dyncube_ssr_blur_tables;
  cl->bind_pipeline(reshade::api::pipeline_stage::all_compute, d->dyncube_ssr_blur_pipeline);
  // H pass
  reshade::api::resource_view hsrvs[2] = {d->dyncube_ssr_raw_srv, d->captured_depth_srv};
  reshade::api::descriptor_table_update bh[4] = {
      {bt->at(0), 0, 0, 1, reshade::api::descriptor_type::sampler, &d->dyncube_sampler},
      {bt->at(1), 0, 0, 2, reshade::api::descriptor_type::texture_shader_resource_view, hsrvs},
      {bt->at(2), 0, 0, 1, reshade::api::descriptor_type::texture_unordered_access_view, &d->dyncube_ssr_blur_h_uav},
      {bt->at(3), 0, 0, 1, reshade::api::descriptor_type::constant_buffer, &d->captured_scene_cbv_view},
  };
  dev->update_descriptor_tables(4, bh);
  std::array<reshade::api::descriptor_table, 4> btables = {bt->at(0), bt->at(1), bt->at(2), bt->at(3)};
  cl->bind_descriptor_tables(reshade::api::shader_stage::all_compute, d->dyncube_ssr_blur_layout, 0, 4, btables.data());
  float pcH[3] = {sigma, 1.f, (shader_injection.dynCube_ssr_symmetric_weights > 0.5f) ? 1.f : 0.f};
  cl->push_constants(reshade::api::shader_stage::all_compute, d->dyncube_ssr_blur_layout, 4, 0, 3, pcH);
  cl->dispatch((w + 7u) / 8u, (h + 7u) / 8u, 1);
  cl->barrier(d->dyncube_ssr_blur_h, reshade::api::resource_usage::unordered_access, reshade::api::resource_usage::shader_resource);
  // V pass
  reshade::api::resource_view vsrvs[2] = {d->dyncube_ssr_blur_h_srv, d->captured_depth_srv};
  reshade::api::descriptor_table_update bv[4] = {
      {bt->at(0), 0, 0, 1, reshade::api::descriptor_type::sampler, &d->dyncube_sampler},
      {bt->at(1), 0, 0, 2, reshade::api::descriptor_type::texture_shader_resource_view, vsrvs},
      {bt->at(2), 0, 0, 1, reshade::api::descriptor_type::texture_unordered_access_view, &d->dyncube_ssr_blur_uav},
      {bt->at(3), 0, 0, 1, reshade::api::descriptor_type::constant_buffer, &d->captured_scene_cbv_view},
  };
  dev->update_descriptor_tables(4, bv);
  cl->bind_descriptor_tables(reshade::api::shader_stage::all_compute, d->dyncube_ssr_blur_layout, 0, 4, btables.data());
  float pcV[3] = {sigma, 0.f, (shader_injection.dynCube_ssr_symmetric_weights > 0.5f) ? 1.f : 0.f};
  cl->push_constants(reshade::api::shader_stage::all_compute, d->dyncube_ssr_blur_layout, 4, 0, 3, pcV);
  cl->dispatch((w + 7u) / 8u, (h + 7u) / 8u, 1);
  cl->barrier(d->dyncube_ssr_blur, reshade::api::resource_usage::unordered_access, reshade::api::resource_usage::shader_resource);
  UnbindDynCubeComputeState(cl);
  return true;
}

// -- Push constants builder (kai-vanillaplus style) --

static std::array<float, kGtvbaoPushConstantFloats> BuildGTVBAOPushConstants(DeviceData* data, bool denoise_last_pass,
                                                       float ssgi_enabled_override = -1.f,
                                                       bool foliage_mask_valid = false,
                                                       int denoise_stage = 0,
                                                       float atrous_step = 1.f) {
  std::array<float, kGtvbaoPushConstantFloats> c = {};
  const uint32_t denoise_passes = (uint32_t)shader_injection.gtvbao_denoise_passes;
  c[0]  = shader_injection.gtvbao_quality_level;
  c[1]  = (float)denoise_passes;
  c[2]  = std::max(0.001f, shader_injection.gtvbao_radius);
  c[3]  = std::clamp(shader_injection.gtvbao_falloff_range, 0.f, 1.f);
  c[4]  = std::clamp(shader_injection.gtvbao_radius_multiplier, 0.3f, 3.f);
  c[5]  = std::clamp(shader_injection.gtvbao_final_power, 0.5f, 5.f);
  c[6]  = std::clamp(shader_injection.gtvbao_sample_distribution, 1.f, 3.f);
  c[7]  = std::clamp(shader_injection.gtvbao_bitmask_thickness, 0.01f, 2.f);
  c[8]  = std::clamp(shader_injection.gtvbao_depth_mip_offset, 0.f, 30.f);
  c[9]  = denoise_passes == 0u ? 10000.f : std::max(0.01f, shader_injection.gtvbao_denoise_blur_beta);
  c[10] = (denoise_passes == 0u && g_gtvbao_jitter_toggle < 0.5f)
      ? 0.f : (float)((data ? data->frame_index : 0u) % 64u);
  c[11] = shader_injection.gtvbao_debug_view;
  c[12] = denoise_last_pass ? 1.f : 0.f;
  // Normal input: use game MRT normals when available, depth fallback otherwise.
  c[13] = g_gtvbao_normal_input_mode;
  c[14] = (data && data->captured_mrt_normal_srv.handle != 0u) ? 1.f : 0.f;
  c[15] = g_gtvbao_normal_influence;
  c[16] = g_gtvbao_normal_depth_blend;
  c[17] = g_gtvbao_normal_sharpness;
  c[18] = g_gtvbao_normal_edge_rejection;
  c[19] = g_gtvbao_normal_z_preservation;
  c[20] = g_gtvbao_normal_detail_response;
  c[21] = g_gtvbao_normal_max_darkening;
  c[22] = g_gtvbao_normal_darkening_mode;
  c[23] = g_gtvbao_normal_transform_mode;
  // -- GI parameters (IS-FAST repurpose) --
  // c[24] = g_gi_enabled
  c[24] = (ssgi_enabled_override >= 0.f) ? ssgi_enabled_override : shader_injection.vbgi_enabled; // GI enable
  // c[25] = g_gi_light_exposure
  c[25] = std::clamp(g_vbgi_light_exposure, 0.001f, 10.f);    // HDR light buffer exposure
  // c[26] = g_gi_intensity
  c[26] = std::clamp(shader_injection.vbgi_intensity, 0.f, 5.f);  // GI intensity
  // c[27] = g_gi_saturation
  c[27] = std::clamp(shader_injection.vbgi_saturation, 0.f, 2.f); // GI saturation
  // c[28] = g_gi_multibounce
  c[28] = shader_injection.vbgi_multibounce;                       // multi-bounce (0/1)
  c[29] = std::clamp(shader_injection.vbgi_multibounce_strength, 0.f, 10.f);  // feedback strength
  c[30] = std::clamp(shader_injection.vbgi_multibounce_saturation, 0.f, 2.f); // feedback saturation
  c[31] = std::clamp(shader_injection.vbgi_multibounce_max_clamp, 0.f, 20.f);  // multi-bounce max clamp
  c[32] = shader_injection.vbgi_debug_view;                         // VBGI debug view
  c[33] = g_isfast_enabled;                                          // IS-FAST enable (0/1)
  c[34] = std::clamp(g_isfast_strength, 0.f, 1.f);                   // IS-FAST noise strength
  c[35] = (data && data->isfast_texture_loaded) ? 1.f : 0.f;         // IS-FAST texture loaded flag
  c[36] = shader_injection.vbgi_adaptive_mode;                       // 0=GI color, 1=albedo
  c[37] = std::clamp(shader_injection.vbgi_adaptive_luma_strength, 0.f, 5.f); // 0=off
  c[38] = std::clamp(shader_injection.vbgi_adaptive_luma_blend, 0.f, 1.f);
  c[39] = std::clamp(g_isfast_spatial_scale, 0.25f, 4.f);          // IS-FAST spatial scale
  c[40] = std::clamp(g_isfast_temporal_speed, 0.f, 5.f);           // IS-FAST temporal speed
  c[41] = std::clamp(g_isfast_seed_offset, 0.f, 64.f);             // IS-FAST seed offset
  // -- Denoiser leak parameters --
  c[42] = std::clamp(shader_injection.gtvbao_denoise_leak_threshold, 1.f, 4.f);
  c[43] = std::clamp(shader_injection.gtvbao_denoise_leak_strength, 0.f, 1.f);
  // -- Spatial denoiser only (Spatio-Temporal / Poisson removed) --
  c[44] = 0.f;  // temporal_blend: off (spatial only)
  c[45] = 0.01f;  // disocclusion_threshold: unused by spatial path
  c[46] = shader_injection.gtvbao_noise_type;    // 0=IS-FAST, 1=IGN, 2=Hilbert
  // -- GTVBAO upgrades: always On (UI toggles removed) --
  c[47] = 1.f;  // cdf_enabled
  c[48] = 1.f;  // cosine_enabled
  c[49] = shader_injection.gtvbao_cosine_mode;
  c[50] = 1.f;  // thickness_enabled
  // -- Foliage / prefilter --
  c[51] = shader_injection.gtvbao_prefilter_enabled;
  // -- Foliage exclusion --
  c[52] = shader_injection.gtvbao_exclude_foliage;
  c[53] = std::clamp(shader_injection.gtvbao_foliage_ao_value, 0.f, 1.f);
  c[54] = IsKai() ? 1.f : 0.f;
  // c[55] - foliage mask is only fresh when the pre-pass dispatched this frame.
  c[55] = foliage_mask_valid ? 1.f : 0.f;
  // -- Denoiser upgrades (R1-R4) --
  c[56] = (float)denoise_stage;                                        // dispatch mode for denoise_last
  c[57] = shader_injection.gtvbao_atrous_enabled;
  c[58] = std::clamp(shader_injection.gtvbao_atrous_depth_sigma, 0.01f, 8.f);
  c[59] = std::clamp(shader_injection.gtvbao_atrous_normal_sigma, 1.f, 128.f);
  c[60] = std::clamp(atrous_step, 1.f, 8.f);                           // �-trous stride (1/2/4)
  // �� Half-resolution spatial pipeline (appended; Full path ignores these) ��
  c[61] = shader_injection.gtvbao_resolution > 0.5f ? 1.f : 0.f;
  c[62] = std::clamp(shader_injection.gtvbao_upscale_plane_sigma, 1.f, 400.f);
  c[63] = std::clamp(shader_injection.gtvbao_upscale_normal_power, 1.f, 64.f);
  c[64] = shader_injection.gtvbao_upscale_debug;
  // -- VBGI (GI) normal settings, independent of the AO ones at c[13]/c[15]/
  // c[19]/c[23]. VBGI is a bleeding effect and frequently wants different
  // geometry handling from AO -- most often MRT normals for AO (accurate but
  // sharp) and depth-derived for GI (softer, fewer fireflies). Defaults match
  // the AO neutral values so enabling VBGI changes nothing until a knob moves.
  c[65] = g_gtvbao_gi_normal_input_mode;
  c[66] = g_gtvbao_gi_normal_influence;
  c[67] = g_gtvbao_gi_normal_z_preservation;
  c[68] = g_gtvbao_gi_normal_transform_mode;
  c[69] = std::clamp(shader_injection.vbgi_multibounce_bounce_fraction, 0.f, 0.5f); // bounce fraction (albedo)
  return c;
}

// -- Pipeline creation --

static bool CreateComputePipelinesIfNeeded(reshade::api::device* dev, DeviceData* d) {
  // CPU opt: when ensure mode is on, skip destruction (kai-style).
  // When off, force-recreate every call (legacy behavior).
  auto dp = [&](reshade::api::pipeline& p) {
    if (g_cpuopt_ensure_pipelines > 0.5f) return;  // keep existing
    if (p.handle) { dev->destroy_pipeline(p); p = {}; }
  };
  auto dl = [&](reshade::api::pipeline_layout& l) {
    if (g_cpuopt_ensure_pipelines > 0.5f) return;  // keep existing
    if (l.handle) { dev->destroy_pipeline_layout(l); l = {}; }
  };
  dl(d->prefilter_layout); dl(d->main_layout); dl(d->denoise_layout);
  dl(d->atrous_layout);
  dl(d->normal_prep_layout);
  dl(d->upscale_layout);
  dp(d->prefilter_pipeline); dp(d->main_low_pipeline); dp(d->main_medium_pipeline);
  dp(d->main_high_pipeline); dp(d->main_ultra_pipeline); dp(d->denoise_pipeline);
  dp(d->denoise_last_pipeline);
  dp(d->denoise_last_kai_pipeline);
  dp(d->denoise_last_sora2nd_pipeline);
  dp(d->atrous_pipeline);
  dp(d->normal_prep_pipeline);
  dp(d->upscale_pipeline);
  if (g_cpuopt_ensure_pipelines < 0.5f) {
    DestroyGTVBAODescriptorTables(dev, &d->prefilter_tables);
    DestroyGTVBAODescriptorTables(dev, &d->main_tables);
    DestroyGTVBAODescriptorTables(dev, &d->denoise_tables);
    DestroyGTVBAODescriptorTables(dev, &d->atrous_tables);
    DestroyGTVBAODescriptorTables(dev, &d->normal_prep_tables);
    DestroyGTVBAODescriptorTables(dev, &d->upscale_tables);
  }

  auto mkcs = [&](std::span<const uint8_t> bc, const char* ep,
                  reshade::api::pipeline_layout lo, reshade::api::pipeline* out) -> bool {
    if (bc.empty() || !lo.handle) return false;
    reshade::api::shader_desc sd = {};
    sd.code = bc.data(); sd.code_size = bc.size(); sd.entry_point = ep;
    reshade::api::pipeline_subobject so = {reshade::api::pipeline_subobject_type::compute_shader, 1, &sd};
    return dev->create_pipeline(lo, 1, &so, out);
  };

  using DR = reshade::api::descriptor_range;
  using DS = reshade::api::shader_stage;
  using DT = reshade::api::descriptor_type;
  using P = reshade::api::pipeline_layout_param;

  // Match kai's EnsureGTVBAOLayout: separate descriptor tables, each with binding=0,
  // plus push_constants at b13.
  auto make_layout = [&](uint32_t srv_count, uint32_t uav_count,
                         reshade::api::pipeline_layout* out) -> bool {
    if (out->handle != 0u) return true;
    DR sampler_r = {0,0,0,1,DS::all_compute,1,DT::sampler};
    DR cbv_r     = {0,0,0,1,DS::all_compute,1,DT::constant_buffer};
    DR srv_r     = {0,0,0,srv_count,DS::all_compute,1,DT::texture_shader_resource_view};
    DR uav_r     = {0,0,0,uav_count,DS::all_compute,1,DT::texture_unordered_access_view};
    reshade::api::constant_range push_constants_range = {};
    push_constants_range.binding = 0;
    push_constants_range.dx_register_index = 13;
    push_constants_range.dx_register_space = 0;
    push_constants_range.count = kGtvbaoPushConstantFloats;
    push_constants_range.visibility = DS::all_compute;
    P param_sampler, param_cbv, param_srv, param_uav, param_constants;
    param_sampler.type = reshade::api::pipeline_layout_param_type::descriptor_table;
    param_sampler.descriptor_table.count = 1; param_sampler.descriptor_table.ranges = &sampler_r;
    param_cbv.type = reshade::api::pipeline_layout_param_type::descriptor_table;
    param_cbv.descriptor_table.count = 1; param_cbv.descriptor_table.ranges = &cbv_r;
    param_srv.type = reshade::api::pipeline_layout_param_type::descriptor_table;
    param_srv.descriptor_table.count = 1; param_srv.descriptor_table.ranges = &srv_r;
    param_uav.type = reshade::api::pipeline_layout_param_type::descriptor_table;
    param_uav.descriptor_table.count = 1; param_uav.descriptor_table.ranges = &uav_r;
    param_constants.type = reshade::api::pipeline_layout_param_type::push_constants;
    param_constants.push_constants = push_constants_range;
    P params[5] = {param_sampler, param_cbv, param_srv, param_uav, param_constants};
    return dev->create_pipeline_layout(5, params, out);
  };

  if (!make_layout(1u, kGTVBAODepthMipLevels, &d->prefilter_layout)) return false;
  // Foliage mask (t0=depth, t1=MRT normal ? u0=foliage mask)
  if (!make_layout(2u, 1u, &d->foliage_mask_layout)) return false;
  EnsureGTVBAODescriptorTables(dev, d->foliage_mask_layout, &d->foliage_mask_tables);
  if (!d->foliage_mask_pipeline.handle)
    mkcs(__gtvbao_foliage_mask, "main", d->foliage_mask_layout, &d->foliage_mask_pipeline);
  // Main: 5 SRVs (depth MIPs, MRT normal, light buffer, IS-FAST noise, foliage mask) + 4 UAVs (AO, edges, GI, debug)
  if (!make_layout(5u, 4u, &d->main_layout)) return false;
  // Denoise: 6 SRVs (AO, edges, raw GI, history AO, depth mip, MRT normal) + 3 UAVs (denoised AO, denoised GI, history AO)
  if (!make_layout(6u, 3u, &d->denoise_layout)) return false;
  // �-trous: 3 SRVs (AO src, depth MIP0, pre-decoded normals) + 1 UAV (AO dst)
  if (!make_layout(3u, 1u, &d->atrous_layout)) return false;
  EnsureGTVBAODescriptorTables(dev, d->atrous_layout, &d->atrous_tables);
  // Normal prep: 1 SRV (MRT normal) + 1 UAV (decoded normals)
  if (!make_layout(1u, 1u, &d->normal_prep_layout)) return false;
  EnsureGTVBAODescriptorTables(dev, d->normal_prep_layout, &d->normal_prep_tables);
  // Multi-bounce accumulate: 2 SRVs (color, previous GI) + 1 UAV (accumulated)
  if (!make_layout(2u, 1u, &d->multibounce_layout)) return false;
  // Upscale (half->full joint reconstruction):
  // t0=half AO, t1=half GI denoised, t2=full depth MIPs, t3=full MRT normal,
  // t4=full pre-decoded normals -> u0=full AO, u1=full GI, u2=full debug
  if (!make_layout(5u, 3u, &d->upscale_layout)) return false;
  EnsureGTVBAODescriptorTables(dev, d->upscale_layout, &d->upscale_tables);

  EnsureGTVBAODescriptorTables(dev, d->prefilter_layout, &d->prefilter_tables);
  if (!d->prefilter_pipeline.handle) mkcs(__gtvbao_prefilter, "main", d->prefilter_layout, &d->prefilter_pipeline);
  if (!d->main_low_pipeline.handle)      mkcs(__gtvbao_main_low, "main", d->main_layout, &d->main_low_pipeline);
  if (!d->main_medium_pipeline.handle)   mkcs(__gtvbao_main_medium, "main", d->main_layout, &d->main_medium_pipeline);
  if (!d->main_high_pipeline.handle)     mkcs(__gtvbao_main_high, "main", d->main_layout, &d->main_high_pipeline);
  if (!d->main_ultra_pipeline.handle)    mkcs(__gtvbao_main_ultra, "main", d->main_layout, &d->main_ultra_pipeline);
  if (!d->denoise_pipeline.handle)       mkcs(__gtvbao_denoise_pass, "main", d->denoise_layout, &d->denoise_pipeline);
  if (!d->denoise_last_pipeline.handle)  mkcs(__gtvbao_denoise_last, "main", d->denoise_layout, &d->denoise_last_pipeline);
  // Kai variant: same layout, different CSO with correct prevViewProj_g at c85
  if (!d->denoise_last_kai_pipeline.handle) mkcs(__gtvbao_denoise_last_kai, "main", d->denoise_layout, &d->denoise_last_kai_pipeline);
  // Sora 2nd variant: same layout, different CSO with correct prevViewProj_g at c75
  if (!d->denoise_last_sora2nd_pipeline.handle) mkcs(__gtvbao_denoise_last_sora2nd, "main", d->denoise_layout, &d->denoise_last_sora2nd_pipeline);
  // �-trous wavelet spatial filter (R3)
  if (!d->atrous_pipeline.handle) mkcs(__gtvbao_atrous, "main", d->atrous_layout, &d->atrous_pipeline);
  // Normal pre-decode (�-trous perf)
  if (!d->normal_prep_pipeline.handle) mkcs(__gtvbao_normal_prep, "main", d->normal_prep_layout, &d->normal_prep_pipeline);
  if (!d->multibounce_pipeline.handle)   mkcs(__gtvbao_multibounce_accumulate, "main", d->multibounce_layout, &d->multibounce_pipeline);
  if (!d->upscale_pipeline.handle) mkcs(__gtvbao_upscale, "main", d->upscale_layout, &d->upscale_pipeline);

  // -- SSGI is now integrated into the main pass (visibility bitmask AO+GI). --
  // no separate VBGI pipeline needed � main_layout handles both AO and GI outputs.
  return d->prefilter_pipeline.handle && d->main_high_pipeline.handle
      && d->denoise_pipeline.handle && d->denoise_last_pipeline.handle
      && d->multibounce_pipeline.handle;
}

// -- IS-FAST DDS loader (64�64�64, RG8_UNORM Texture3D) --

#pragma pack(push, 1)
struct DDS_PIXELFORMAT { uint32_t dwSize, dwFlags, dwFourCC, dwRGBBitCount, dwRBitMask, dwGBitMask, dwBBitMask, dwABitMask; };
struct DDS_HEADER {
  uint32_t dwSize, dwFlags, dwHeight, dwWidth, dwPitchOrLinearSize, dwDepth, dwMipMapCount;
  uint32_t dwReserved1[11];
  DDS_PIXELFORMAT ddspf;
  uint32_t dwCaps, dwCaps2, dwCaps3, dwCaps4, dwReserved2;
};
struct DDS_HEADER_DXT10 { uint32_t dxgiFormat, resourceDimension, miscFlag, arraySize, miscFlags2; };
#pragma pack(pop)

static bool LoadISFASTNoiseTexture(reshade::api::device* dev, DeviceData* d) {
  // Diagnostic log every frame when debug logging is on
  if (g_isfast_debug_logging > 0.5f) {
    reshade::log::message(reshade::log::level::info,
      (std::string("[IS-FAST] Status: attempted=") + (d->isfast_texture_attempted ? "yes" : "no")
       + ", loaded=" + (d->isfast_texture_loaded ? "yes" : "no")
       + ", srv=" + (d->isfast_noise_srv.handle ? "valid" : "null")).c_str());
  }
  if (d->isfast_texture_attempted) return d->isfast_texture_loaded;
  d->isfast_texture_attempted = true;

  // Source: baked-in fast_noise_ea.dds bytes (embed_file.exe header).
  // No external file next to the game .exe is required anymore.
  const auto* bytes = __fast_noise_ea.data();
  const size_t byte_count = __fast_noise_ea.size();

  if (g_isfast_debug_logging > 0.5f)
    reshade::log::message(reshade::log::level::info,
      (std::string("[IS-FAST] Embedded fast_noise_ea.dds bytes: ") + std::to_string(byte_count)).c_str());

  if (byte_count < sizeof(DDS_HEADER) + 4) {
    if (g_isfast_debug_logging > 0.5f)
      reshade::log::message(reshade::log::level::warning,
        "[IS-FAST] Embedded fast_noise_ea.dds too small � using IGN fallback.");
    return false;
  }

  // Read magic
  uint32_t magic = 0;
  memcpy(&magic, bytes, 4);
  if (magic != 0x20534444) return false; // "DDS "

  DDS_HEADER hdr = {};
  memcpy(&hdr, bytes + 4, sizeof(hdr));

  uint32_t w = hdr.dwWidth, h = hdr.dwHeight, ddsDepth = hdr.dwDepth;
  uint32_t fmt = 0;
  bool isDX10 = (hdr.ddspf.dwFourCC == 0x30315844); // "DX10"

  size_t header_size = 4 + sizeof(hdr);
  if (isDX10) {
    header_size += sizeof(DDS_HEADER_DXT10);
    if (byte_count < header_size) {
      if (g_isfast_debug_logging > 0.5f)
        reshade::log::message(reshade::log::level::warning,
          "[IS-FAST] Embedded fast_noise_ea.dds truncated DX10 header � using IGN fallback.");
      return false;
    }
    DDS_HEADER_DXT10 dx10 = {};
    memcpy(&dx10, bytes + 4 + sizeof(hdr), sizeof(dx10));
    fmt = dx10.dxgiFormat;
    if (dx10.resourceDimension != 4) return false; // must be Texture3D
  }

  // DXGI_FORMAT_R8G8_UNORM = 49, expected dims: 128�128�32
  if (w != 128 || h != 128 || ddsDepth != 32 || fmt != 49) {
    if (g_isfast_debug_logging > 0.5f) {
      std::string msg = "[IS-FAST] Unexpected DDS: ";
      msg += std::to_string(w) + "x" + std::to_string(h) + "x" + std::to_string(ddsDepth);
      msg += " fmt=" + std::to_string(fmt) + " (expected 128x128x32 RG8_UNORM) � using IGN fallback.";
      reshade::log::message(reshade::log::level::warning, msg.c_str());
    }
    return false;
  }

  // Payload: 128�128�32 � 2 bytes = 1,048,576 bytes
  size_t dataSize = (size_t)w * h * ddsDepth * 2;
  if (byte_count < header_size + dataSize) {
    if (g_isfast_debug_logging > 0.5f)
      reshade::log::message(reshade::log::level::warning,
        "[IS-FAST] Embedded fast_noise_ea.dds truncated payload � using IGN fallback.");
    return false;
  }
  std::vector<uint8_t> data(dataSize);
  memcpy(data.data(), bytes + header_size, dataSize);

  // Create 3D texture
  reshade::api::resource_desc rd = {};
  rd.type = reshade::api::resource_type::texture_3d;
  rd.texture = {w, h, (uint16_t)ddsDepth, 1, reshade::api::format::r8g8_unorm, 1};
  rd.heap = reshade::api::memory_heap::gpu_only;
  rd.usage = reshade::api::resource_usage::shader_resource | reshade::api::resource_usage::copy_dest;

  reshade::api::subresource_data sub = {};
  sub.data = data.data();
  sub.row_pitch = w * 2;
  sub.slice_pitch = w * 2 * h;       // bytes per 2D slice (row_pitch � height)

  if (!dev->create_resource(rd, &sub, reshade::api::resource_usage::shader_resource,
                            &d->isfast_noise_texture)) {
    if (g_isfast_debug_logging > 0.5f)
      reshade::log::message(reshade::log::level::warning,
        "[IS-FAST] Failed to create 3D noise texture � using IGN fallback.");
    return false;
  }

  dev->create_resource_view(d->isfast_noise_texture, reshade::api::resource_usage::shader_resource,
    reshade::api::resource_view_desc(reshade::api::resource_view_type::texture_3d,
                                     reshade::api::format::r8g8_unorm, 0, 1, 0, 1),
    &d->isfast_noise_srv);

  // Create point-wrap sampler for IS-FAST noise sampling
  {
    reshade::api::sampler_desc sd = {};
    sd.filter = reshade::api::filter_mode::min_mag_mip_point;
    sd.address_u = reshade::api::texture_address_mode::wrap;
    sd.address_v = reshade::api::texture_address_mode::wrap;
    sd.address_w = reshade::api::texture_address_mode::wrap;
    dev->create_sampler(sd, &d->isfast_sampler);
  }

  d->isfast_texture_loaded = true;
  if (g_isfast_debug_logging > 0.5f)
    reshade::log::message(reshade::log::level::info,
      "[IS-FAST] Texture loaded (embedded): 128x128x32 RG8_UNORM � noise source: TEXTURE");
  return true;
}

// -- Dispatch --

static bool RunGTVBAO(reshade::api::command_list* cl, DeviceData* d) {
  if (!d->captured_depth_srv.handle) return false;

  // -- Frame skips (independent per component) --
  auto skip_this_frame = [&](float setting) -> bool {
    if (setting <= 0.5f) return false;
    uint64_t n = (uint64_t)setting + 1u;
    return (d->frame_index % n) != 0u;
  };
  bool skip_GTVBAO      = skip_this_frame(g_gtvbao_frame_skip);       // skips entire dispatch
  bool skip_ssgi         = skip_this_frame(g_vbgi_frame_skip);         // AO runs, GI off
  bool skip_multibounce  = skip_this_frame(g_multibounce_frame_skip);  // accumulate skipped

  if (skip_GTVBAO) return true;  // skip everything, no work done

  float ssgi_enabled_this_frame = shader_injection.vbgi_enabled;
  if (skip_ssgi) ssgi_enabled_this_frame = 0.f;
  auto* dev = cl->get_device();

  // -- IS-FAST noise texture (load once) --
  if (g_isfast_enabled > 0.5f) LoadISFASTNoiseTexture(dev, d);

  if (shader_injection.gtvbao_debug_logging > 0.5f)
    reshade::log::message(reshade::log::level::info, "[GTVBAO] RunGTVBAO: creating pipelines...");
  if (!CreateComputePipelinesIfNeeded(dev, d)) return false;
  if (shader_injection.gtvbao_debug_logging > 0.5f)
    reshade::log::message(reshade::log::level::info, "[GTVBAO] RunGTVBAO: allocating descriptor tables...");
  if (!EnsureGTVBAODescriptorTables(dev, d->prefilter_layout, &d->prefilter_tables)) return false;
  if (!EnsureGTVBAODescriptorTables(dev, d->main_layout, &d->main_tables)) return false;
  if (!EnsureGTVBAODescriptorTables(dev, d->denoise_layout, &d->denoise_tables)) return false;
  if (!EnsureGTVBAODescriptorTables(dev, d->multibounce_layout, &d->multibounce_tables)) return false;
  if (!EnsureGTVBAODescriptorTables(dev, d->foliage_mask_layout, &d->foliage_mask_tables)) return false;
  if (!EnsureGTVBAODescriptorTables(dev, d->upscale_layout, &d->upscale_tables)) return false;

  uint32_t w = d->working_width, h = d->working_height;
  if (w < 64 || h < 64) {
    CSLog("gtvbao", "run entry ABORT: working too small", true);
    return false;
  }
  // �� Half-resolution spatial pipeline: AO/GI evaluation domain. Full path
  // uses w/h everywhere (existing behavior); Half uses hw/hh for main,
  // denoise, and �-trous while depth/MRT/light/multibounce stay full.
  const bool half_mode = shader_injection.gtvbao_resolution > 0.5f
      && d->half_width >= 32u && d->half_height >= 32u
      && d->ao_term_a_texture.handle && d->upscale_ao_texture.handle;
  const uint32_t aw = half_mode ? d->half_width : w;
  const uint32_t ah = half_mode ? d->half_height : h;
  {
    const std::string liveDepth = d->captured_depth_srv.handle ? d->captured_depth_dims : "none";
    const std::string wantDims = std::to_string(w) + "x" + std::to_string(h);
    CSLog("gtvbao", std::string("run entry working=") + wantDims + " liveDepth=" + liveDepth, liveDepth != wantDims);
  }

  // Save + reset per-frame foliage tracking (set true in foliage shader on_draw callbacks)
  bool had_foliage_draws = d->foliage_drawn_this_frame;
  d->foliage_drawn_this_frame = false;
  // The foliage mask is only fresh when the pre-pass actually dispatched this frame.
  // Otherwise it holds stale data from an earlier frame (e.g. a previous scene with foliage)
  // and must NOT be consumed by the main pass.
  const bool foliage_mask_valid = had_foliage_draws
      && shader_injection.gtvbao_exclude_foliage > 0.5f
      && d->foliage_mask_pipeline.handle
      && d->foliage_mask_uav.handle
      && d->captured_mrt_normal_srv.handle;

  if (shader_injection.gtvbao_debug_logging > 0.5f)
    reshade::log::message(reshade::log::level::info,
      (std::string("[GTVBAO] RunGTVBAO: dispatching pass 1 (") +
       std::to_string(w) + "x" + std::to_string(h) + ")").c_str());

  auto bar = [&](reshade::api::resource r, reshade::api::resource_usage o, reshade::api::resource_usage n) {
    if (r.handle) cl->barrier(r, o, n);
  };
  const auto UA = reshade::api::resource_usage::unordered_access;
  const auto SR = reshade::api::resource_usage::shader_resource;
  const auto CS = reshade::api::shader_stage::all_compute;
  const auto AC = reshade::api::pipeline_stage::all_compute;

  // Helper: build & apply descriptor updates.
  auto apply_descriptors = [&](reshade::api::pipeline_layout lo,
                                GTVBAODescriptorTableSet* tbl,
                                uint32_t count,
                                const reshade::api::descriptor_table_update* updates) {
    std::array<reshade::api::descriptor_table_update, kGtvbaoDescriptorTableParamCount> u = {};
    for (uint32_t i = 0; i < count; ++i) { u[i] = updates[i]; u[i].table = (*tbl)[i]; }
    dev->update_descriptor_tables(count, u.data());
    std::array<reshade::api::descriptor_table, kGtvbaoDescriptorTableParamCount> b = {};
    for (uint32_t i = 0; i < count; ++i) b[i] = (*tbl)[i];
    cl->bind_descriptor_tables(CS, lo, 0, count, b.data());
  };

  auto bind_pipe = [&](reshade::api::pipeline p) {
    cl->bind_pipeline(AC, p);
  };

  // Pass 1: Prefilter
  if (shader_injection.gtvbao_debug_logging > 0.5f)
    reshade::log::message(reshade::log::level::info, "[GTVBAO] Pass 1: binding pipeline...");
  bind_pipe(d->prefilter_pipeline);
  if (shader_injection.gtvbao_debug_logging > 0.5f)
    reshade::log::message(reshade::log::level::info, "[GTVBAO] Pass 1: updating descriptors...");
  {
    reshade::api::descriptor_table_update u[4] = {
      {{},0,0,1,reshade::api::descriptor_type::sampler,&d->point_clamp_sampler},
      {{},0,0,1,reshade::api::descriptor_type::constant_buffer,&d->captured_scene_cbv_view},
      {{},0,0,1,reshade::api::descriptor_type::texture_shader_resource_view,&d->captured_depth_srv},
      {{},0,0,kGTVBAODepthMipLevels,reshade::api::descriptor_type::texture_unordered_access_view,d->depth_mips_uavs.data()},
    };
    apply_descriptors(d->prefilter_layout, &d->prefilter_tables, 4, u);
    auto pc = BuildGTVBAOPushConstants(d, false);
    cl->push_constants(CS, d->prefilter_layout, kGtvbaoPushConstantsLayoutParam, 0, kGtvbaoPushConstantFloats, pc.data());
  }
  cl->dispatch((w + 15) / 16, (h + 15) / 16, 1);
  bar(d->depth_mips_texture, UA, SR);
  if (shader_injection.gtvbao_debug_logging > 0.5f)
    reshade::log::message(reshade::log::level::info, "[GTVBAO] Pass 1 (prefilter) done.");

  // -- Multi-bounce accumulate (HDR light buffer + previous GI) --
  // Runs BEFORE main pass to create an HDR accumulated light buffer.
  {
    bool mb_enabled  = shader_injection.vbgi_multibounce > 0.5f;
    bool mb_gi_ready = d->vbgi_denoised_valid;
    bool mb_pipe_ok  = d->multibounce_pipeline.handle != 0u;
    bool mb_color_ok = d->captured_color_srv.handle != 0u;
    bool mb_prev_ok  = d->vbgi_denoised_srv.handle != 0u;
    bool mb_uav_ok   = d->multibounce_uav.handle != 0u;

    if (shader_injection.vbgi_debug_logging > 0.5f) {
      std::string msg = "[SSGI] MultiBounce: enabled=";
      msg += mb_enabled ? "1" : "0";
      msg += " denoisedValid="; msg += mb_gi_ready ? "1" : "0";
      msg += " pipeline=";      msg += mb_pipe_ok ? "OK" : "MISSING";
      msg += " colorSRV=";      msg += mb_color_ok ? "OK" : "MISSING";
      msg += " prevGI_SRV=";    msg += mb_prev_ok ? "OK" : "MISSING";
      msg += " accUAV=";        msg += mb_uav_ok ? "OK" : "MISSING";
      reshade::log::message(reshade::log::level::info, msg.c_str());
    }

    // Skip the full-res accumulate when VBGI is effectively off and no consumer
    // can observe it. Exempted: debug view 4 (reads the accumulator) and
    // Albedo adaptive mode (samples t2 in main even with GI off).
    const bool mb_unused = ssgi_enabled_this_frame < 0.5f
        && (int)shader_injection.vbgi_debug_view != 4
        && shader_injection.vbgi_adaptive_mode < 0.5f;
    if (mb_enabled && mb_gi_ready && mb_pipe_ok && !skip_multibounce && !mb_unused) {
      bind_pipe(d->multibounce_pipeline);
      reshade::api::resource_view acc_color = mb_color_ok
          ? d->captured_color_srv : d->fallback_srv;
      // Half mode: previous frame's full-res GI is the upscale result;
      // Full mode: existing denoised GI. (Frame order otherwise unchanged:
      // accumulate runs BEFORE the current frame's main pass.)
      reshade::api::resource_view half_prev_gi = d->upscale_gi_srv.handle
          ? d->upscale_gi_srv : d->fallback_srv;
      reshade::api::resource_view acc_prev_gi = half_mode
          ? half_prev_gi
          : (mb_prev_ok ? d->vbgi_denoised_srv : d->fallback_srv);
      reshade::api::resource_view acc_srvs[2] = {acc_color, acc_prev_gi};
      reshade::api::resource_view acc_uav_arr = mb_uav_ok
          ? d->multibounce_uav : d->fallback_uav;
      reshade::api::descriptor_table_update au[4] = {
        {{},0,0,1,reshade::api::descriptor_type::sampler,&d->point_clamp_sampler},
        {{},0,0,1,reshade::api::descriptor_type::constant_buffer,&d->captured_scene_cbv_view},
        {{},0,0,2,reshade::api::descriptor_type::texture_shader_resource_view,acc_srvs},
        {{},0,0,1,reshade::api::descriptor_type::texture_unordered_access_view,&acc_uav_arr},
      };
      apply_descriptors(d->multibounce_layout, &d->multibounce_tables, 4, au);
      cl->push_constants(CS, d->multibounce_layout, kGtvbaoPushConstantsLayoutParam, 0, kGtvbaoPushConstantFloats,
                         BuildGTVBAOPushConstants(d, false).data());
      cl->dispatch((w + 7) / 8, (h + 7) / 8, 1);
      bar(d->multibounce_texture, UA, SR);
      if (shader_injection.vbgi_debug_logging > 0.5f)
        reshade::log::message(reshade::log::level::info, "[SSGI] MultiBounce: accumulate dispatched.");
    } else if (mb_enabled && !mb_gi_ready) {
      if (shader_injection.vbgi_debug_logging > 0.5f)
        reshade::log::message(reshade::log::level::info,
            "[SSGI] MultiBounce: SKIPPED (denoised GI not valid yet � first frame or GTVBAO never ran).");
    } else if (mb_enabled && !mb_pipe_ok) {
      if (shader_injection.vbgi_debug_logging > 0.5f)
        reshade::log::message(reshade::log::level::warning,
            "[SSGI] MultiBounce: SKIPPED (accumulate pipeline missing).");
    }
  }

  // -- Foliage mask pre-pass (full-res, reads MRT normal, writes R8_UINT) --
  if (foliage_mask_valid) {
    uint32_t mkW = w, mkH = h;
    bind_pipe(d->foliage_mask_pipeline);
    reshade::api::resource_view fm_srvs[2] = {
        d->depth_mips_srv,                          // t0 � working depth (GetDimensions)
        d->captured_mrt_normal_srv                  // t1 � G-buffer normal (bit 15 test)
    };
    reshade::api::descriptor_table_update fu[4] = {
      {{},0,0,1,reshade::api::descriptor_type::sampler,&d->point_clamp_sampler},
      {{},0,0,1,reshade::api::descriptor_type::constant_buffer,&d->captured_scene_cbv_view},
      {{},0,0,2,reshade::api::descriptor_type::texture_shader_resource_view,fm_srvs},
      {{},0,0,1,reshade::api::descriptor_type::texture_unordered_access_view,&d->foliage_mask_uav},
    };
    apply_descriptors(d->foliage_mask_layout, &d->foliage_mask_tables, 4, fu);
    auto pc = BuildGTVBAOPushConstants(d, false);
    cl->push_constants(CS, d->foliage_mask_layout, kGtvbaoPushConstantsLayoutParam, 0, kGtvbaoPushConstantFloats, pc.data());
    cl->dispatch((mkW + 7) / 8, (mkH + 7) / 8, 1);
    bar(d->foliage_mask_texture, UA, SR);
  }

  // Pass 2: Main
  reshade::api::pipeline mp = d->main_high_pipeline;
  { int q = (int)shader_injection.gtvbao_quality_level;
    if (q == 0 && d->main_low_pipeline.handle) mp = d->main_low_pipeline;
    else if (q == 1 && d->main_medium_pipeline.handle) mp = d->main_medium_pipeline;
    else if (q == 3 && d->main_ultra_pipeline.handle) mp = d->main_ultra_pipeline;
    if (!mp.handle) mp = d->main_high_pipeline;
    if (!mp.handle) mp = d->main_medium_pipeline;
    if (!mp.handle) mp = d->main_low_pipeline; }
  if (!mp.handle) return false;
  bind_pipe(mp);
  // Edges UAV is unread by the atrous kernel (binds AO/depth/prepped-normal)
  // and, with GI off, by the skipped stage-4 tail: route to fallback so the
  // full-res write is dropped. Spatial-only path keeps the real UAV otherwise.
  const bool atrous_no_edges = shader_injection.gtvbao_atrous_enabled > 0.5f
      && d->atrous_pipeline.handle != 0u
      && shader_injection.vbgi_enabled < 0.5f;
  {
    // Light buffer: HDR accumulated (multi-bounce ON) or direct-only (OFF).
    reshade::api::resource_view light_buf;
    const char* lb_source = "unknown";
    if (shader_injection.vbgi_multibounce > 0.5f && d->vbgi_denoised_valid
        && d->multibounce_srv.handle) {
      // Multi-bounce ON: use HDR accumulated buffer (color + previous GI).
      light_buf = d->multibounce_srv;
      lb_source = "accumulated";
    } else {
      // Single-bounce: use direct-only HDR color texture.
      if (d->captured_color_srv.handle) {
        light_buf = d->captured_color_srv;
        lb_source = "colorSRV";
      } else if (d->captured_light_buffer_srv.handle) {
        light_buf = d->captured_light_buffer_srv;
        lb_source = "backbuf";
      } else {
        light_buf = d->fallback_srv;
        lb_source = "FALLBACK";
      }
    }
    if (shader_injection.vbgi_debug_logging > 0.5f) {
      std::string msg = "[SSGI] Main lightBuf=";
      msg += lb_source;
      msg += " mbEnable="; msg += (shader_injection.vbgi_multibounce > 0.5f) ? "1" : "0";
      msg += " mbReady=";  msg += d->vbgi_denoised_valid ? "1" : "0";
      msg += " mbSRV=";    msg += d->multibounce_srv.handle ? "OK" : "no";
      msg += " colorSRV="; msg += d->captured_color_srv.handle ? "OK" : "no";
      reshade::log::message(reshade::log::level::info, msg.c_str());
    }
    reshade::api::resource_view main_srvs[5] = {
        d->depth_mips_srv,
        d->captured_mrt_normal_srv.handle ? d->captured_mrt_normal_srv : d->fallback_srv,
        light_buf,
        d->isfast_noise_srv.handle ? d->isfast_noise_srv : d->fallback_srv,  // t3 IS-FAST noise
        d->foliage_mask_srv.handle ? d->foliage_mask_srv : d->fallback_srv   // t4 foliage mask
    };
    // Shader register order: u0=AO, u1=edges, u2=GI, u3=debug
    reshade::api::resource_view main_uavs[4] = {
        d->ao_term_a_uav,
        atrous_no_edges ? d->fallback_uav : d->edges_uav,
        d->vbgi_output_uav.handle ? d->vbgi_output_uav : d->fallback_uav,
        d->debug_uav.handle ? d->debug_uav : d->fallback_uav
    };
    reshade::api::descriptor_table_update u[4] = {
      {{},0,0,1,reshade::api::descriptor_type::sampler,&d->point_clamp_sampler},
      {{},0,0,1,reshade::api::descriptor_type::constant_buffer,&d->captured_scene_cbv_view},
      {{},0,0,5,reshade::api::descriptor_type::texture_shader_resource_view,main_srvs},
      {{},0,0,4,reshade::api::descriptor_type::texture_unordered_access_view,main_uavs},
    };
    apply_descriptors(d->main_layout, &d->main_tables, 4, u);
    auto pc = BuildGTVBAOPushConstants(d, false, ssgi_enabled_this_frame, foliage_mask_valid);
    cl->push_constants(CS, d->main_layout, kGtvbaoPushConstantsLayoutParam, 0, kGtvbaoPushConstantFloats, pc.data());
  }
  cl->dispatch((aw + 7) / 8, (ah + 7) / 8, 1);
  bar(d->ao_term_a_texture, UA, SR);
  if (!atrous_no_edges)
    bar(d->edges_texture, UA, SR);
  // Skip barriers for UAVs nothing reads. vbgi_output is read only by the
  // denoise GI section (gated on vbgi_enabled in-shader) and debug view 1;
  // debug_texture only by debug views 5 / 6-8 (see t23 pushes).
  {
    const bool giRead = shader_injection.vbgi_enabled > 0.5f;
    const bool vbgiDbg1 = shader_injection.vbgi_debug_view > 0.5f
        && (int)shader_injection.vbgi_debug_view == 1;
    if (giRead || vbgiDbg1)
      bar(d->vbgi_output_texture, UA, SR);  // GI output ready for denoise
    const bool dbgRead = (shader_injection.vbgi_debug_view > 0.5f
            && (int)shader_injection.vbgi_debug_view == 5)
        || (shader_injection.gtvbao_debug_view > 5.5f
            && shader_injection.gtvbao_debug_view < 8.5f)
        || (shader_injection.gtvbao_debug_view > 21.5f
            && shader_injection.gtvbao_debug_view < 29.5f);
    if (dbgRead)
      bar(d->debug_texture, UA, SR);         // Debug output ready for read
  }
  if (shader_injection.gtvbao_debug_logging > 0.5f)
    reshade::log::message(reshade::log::level::info, "[GTVBAO] Pass 2 (main) done.");
  if (shader_injection.vbgi_debug_logging > 0.5f) {
    std::string msg = "[SSGI] Main pass: enableGI=";
    msg += (shader_injection.vbgi_enabled > 0.5f) ? "1" : "0";
    msg += " intensity=" + std::to_string(shader_injection.vbgi_intensity);
    msg += " multibounce=" + std::to_string((int)shader_injection.vbgi_multibounce);
    msg += " lightBuf=";
    if (shader_injection.vbgi_multibounce > 0.5f && d->vbgi_denoised_valid)
      msg += "accumulated";
    else
      msg += (d->captured_color_srv.handle) ? "colorSRV" : (d->captured_light_buffer_srv.handle ? "backbuf" : "MISSING");
    reshade::log::message(reshade::log::level::info, msg.c_str());
  }

  // Pass 3: Denoise (ping-pong) � always run at least one pass to apply
  // the XE_GTAO_OCCLUSION_TERM_SCALE multiply-back (1.5x) in GTVBAO_Output.
  // When dpc==0 the DenoiseBlurBeta=10000 effectively disables blur.
  int dpc = (int)shader_injection.gtvbao_denoise_passes;
  if (dpc < 1) dpc = 1;
  {
    auto& last_pipe = IsKai() ? d->denoise_last_kai_pipeline
        : (IsSora2nd() ? d->denoise_last_sora2nd_pipeline : d->denoise_last_pipeline);
    const bool atrous_active = shader_injection.gtvbao_atrous_enabled > 0.5f
        && d->atrous_pipeline.handle != 0u;
    // Half mode: GI denoise targets the half-res denoised buffer; the
    // full-res upscale result feeds t23 + next-frame multibounce instead.
    // Target selection is shared with the upscale t1 read and the t23 debug
    // view so the tail can never write a different buffer than they read.
    const bool gi_half_chain = GTVBAO_GiHalfChainReady(d, half_mode);
    reshade::api::resource_view gi_denoised_uav = gi_half_chain
        ? d->vbgi_denoised_half_uav
        : (d->vbgi_denoised_uav.handle ? d->vbgi_denoised_uav : d->fallback_uav);
    reshade::api::resource gi_denoised_tex = gi_half_chain
        ? d->vbgi_denoised_half_texture
        : d->vbgi_denoised_texture;
    {
      static uint32_t logged_w = 0u, logged_h = 0u;
      if (w != logged_w || h != logged_h) {
        logged_w = w; logged_h = h;
        // SRV and UAV come from separate create calls; identity mismatch means
        // the tail writes a different texture than the upscale samples.
        const uint64_t r_half_srv = d->vbgi_denoised_half_srv.handle
            ? dev->get_resource_from_view(d->vbgi_denoised_half_srv).handle : 0u;
        const uint64_t r_half_uav = d->vbgi_denoised_half_uav.handle
            ? dev->get_resource_from_view(d->vbgi_denoised_half_uav).handle : 0u;
        std::ostringstream ids;
        ids << std::hex << r_half_srv << "/" << r_half_uav;
        reshade::log::message(reshade::log::level::info,
          (std::string("[GTVBAO] GI tail: halfMode=") + (half_mode ? "1" : "0")
           + " target=" + (gi_half_chain ? "HALF" : "FULL")
           + " halfTex=" + (d->vbgi_denoised_half_texture.handle ? "ok" : "NULL")
           + " halfSRV=" + (d->vbgi_denoised_half_srv.handle ? "ok" : "NULL")
           + " halfUAV=" + (d->vbgi_denoised_half_uav.handle ? "ok" : "NULL")
           + " resSRV/UAV=" + ids.str()
           + (r_half_srv == r_half_uav ? " MATCH" : " MISMATCH")
           + " aw=" + std::to_string(aw) + " ah=" + std::to_string(ah)).c_str());
      }
    }

    // -- �-trous helpers (spatial-only) --

    // Pre-decode MRT normals once so atrous taps skip the sincos/sqrt decode.
    auto run_normal_prep = [&]() {
      bind_pipe(d->normal_prep_pipeline);
      reshade::api::resource_view np_srvs[1] = {
          d->captured_mrt_normal_srv.handle ? d->captured_mrt_normal_srv : d->fallback_srv};
      reshade::api::descriptor_table_update nu[4] = {
        {{},0,0,1,reshade::api::descriptor_type::sampler,&d->point_clamp_sampler},
        {{},0,0,1,reshade::api::descriptor_type::constant_buffer,&d->captured_scene_cbv_view},
        {{},0,0,1,reshade::api::descriptor_type::texture_shader_resource_view,np_srvs},
        {{},0,0,1,reshade::api::descriptor_type::texture_unordered_access_view,&d->normal_prep_uav},
      };
      apply_descriptors(d->normal_prep_layout, &d->normal_prep_tables, 4, nu);
      auto pc_np = BuildGTVBAOPushConstants(d, false);
      cl->push_constants(CS, d->normal_prep_layout, kGtvbaoPushConstantsLayoutParam, 0, kGtvbaoPushConstantFloats, pc_np.data());
      cl->dispatch((w + 7) / 8, (h + 7) / 8, 1);
      bar(d->normal_prep_texture, UA, SR);
    };

    // 3 �-trous iterations (strides 1/2/4). The last iteration folds the
    // �OCCLUSION_TERM_SCALE multiply-back via denoise_is_last_pass.
    // Returns true when the final result lives in ao_term_b.
    auto run_atrous_chain = [&](bool start_in_b) -> bool {
      bool cur_b = start_in_b;
      for (int i = 0; i < 3; ++i) {
        const bool last_iter = (i == 2);
        bind_pipe(d->atrous_pipeline);
        reshade::api::resource_view a_src = cur_b ? d->ao_term_b_srv : d->ao_term_a_srv;
        reshade::api::resource_view a_dst_uav = cur_b ? d->ao_term_a_uav : d->ao_term_b_uav;
        reshade::api::resource a_dst_tex = cur_b ? d->ao_term_a_texture : d->ao_term_b_texture;
        reshade::api::resource_view a_srvs[3] = {a_src, d->depth_mips_srv,
            d->normal_prep_srv.handle ? d->normal_prep_srv : d->fallback_srv};
        reshade::api::descriptor_table_update au[4] = {
          {{},0,0,1,reshade::api::descriptor_type::sampler,&d->point_clamp_sampler},
          {{},0,0,1,reshade::api::descriptor_type::constant_buffer,&d->captured_scene_cbv_view},
          {{},0,0,3,reshade::api::descriptor_type::texture_shader_resource_view,a_srvs},
          {{},0,0,1,reshade::api::descriptor_type::texture_unordered_access_view,&a_dst_uav},
        };
        apply_descriptors(d->atrous_layout, &d->atrous_tables, 4, au);
        auto pc_a = BuildGTVBAOPushConstants(d, last_iter, -1.f, false, /*stage*/0,
                                             /*step*/float(1 << i));
        cl->push_constants(CS, d->atrous_layout, kGtvbaoPushConstantsLayoutParam, 0, kGtvbaoPushConstantFloats, pc_a.data());
        cl->dispatch((aw + 7) / 8, (ah + 7) / 8, 1);
        bar(a_dst_tex, UA, SR);
        cur_b = !cur_b;
      }
      return cur_b;
    };

    // -- Spatial-only paths (Spatio-Temporal R2 / Poisson removed) --
    if (atrous_active && last_pipe.handle) {
      // -- Spatial-only + �-trous: wavelet chain replaces the combined final
      // dispatch; scale-back folds into the last iteration. A GI-only tail
      // (stage 4) keeps the GI bilateral running. --
      run_normal_prep();
      d->gtvbao_final_in_b = run_atrous_chain(/*start_in_b*/false);  // main wrote ao_term_a
      // Stage-4 GI tail writes nothing when GI is off (empty stage body, gated
      // GI bilateral): skip bind/push/dispatch, keep flags/barriers downstream.
      if (shader_injection.vbgi_enabled > 0.5f) {
      bind_pipe(last_pipe);
      // t0 supplies working dimensions only: BuildGTAOConstants reads its
      // size, and the stage-4 body never samples it. Binding the 1x1 fallback
      // made ViewportPixelSize (1,1) and collapsed every GI edge-stop depth
      // sample onto one edge texel, so bind the real half-res AO term.
      reshade::api::resource_view gi_tail_ao_src = d->gtvbao_final_in_b
          ? d->ao_term_b_srv : d->ao_term_a_srv;
      reshade::api::resource_view sv_g[6] = {
          gi_tail_ao_src.handle ? gi_tail_ao_src : d->fallback_srv,           // t0 half AO (dims only)
          d->edges_srv,                                                       // t1 depth for GI filter
          d->vbgi_output_srv.handle ? d->vbgi_output_srv : d->fallback_srv,   // t2 raw GI
          d->fallback_srv, d->depth_mips_srv,
          d->captured_mrt_normal_srv.handle ? d->captured_mrt_normal_srv : d->fallback_srv};
      reshade::api::resource_view dn_uavs_g[3] = {d->fallback_uav,           // u0 untouched by stage 4
          gi_denoised_uav,
          d->fallback_uav};
      reshade::api::descriptor_table_update u_g[4] = {
        {{},0,0,1,reshade::api::descriptor_type::sampler,&d->point_clamp_sampler},
        {{},0,0,1,reshade::api::descriptor_type::constant_buffer,&d->captured_scene_cbv_view},
        {{},0,0,6,reshade::api::descriptor_type::texture_shader_resource_view,sv_g},
        {{},0,0,3,reshade::api::descriptor_type::texture_unordered_access_view,dn_uavs_g},
      };
      apply_descriptors(d->denoise_layout, &d->denoise_tables, 4, u_g);
      auto pc_g = BuildGTVBAOPushConstants(d, true, -1.f, false, /*stage*/4);
      cl->push_constants(CS, d->denoise_layout, kGtvbaoPushConstantsLayoutParam, 0, kGtvbaoPushConstantFloats, pc_g.data());
      // denoise_last threads cover 2 px each (dt*uint2(2,1) + sides): halve the grid.
      cl->dispatch((aw + 15) / 16, (ah + 7) / 8, 1);
      }  // end GI-on stage-4 dispatch
      // The vbgi_denoised UAV->SR barrier is emitted at the top of the
      // upscale block below, so it lands after this write but before any
      // descriptor that samples the buffer is published.
    } else {
      // -- Spatial bilateral chain (Poisson / temporal removed). --
      bool use_a = true;
      for (int p = 0; p < dpc; ++p) {
        bool last = (p == dpc - 1);
        reshade::api::resource_view src, dst_uav;
        reshade::api::resource dst_tex;
        if (use_a) { src = d->ao_term_a_srv; dst_uav = d->ao_term_b_uav; dst_tex = d->ao_term_b_texture; }
        else       { src = d->ao_term_b_srv; dst_uav = d->ao_term_a_uav; dst_tex = d->ao_term_a_texture; }
        bind_pipe(last ? last_pipe : d->denoise_pipeline);
        // Spatial-only: history slots bound to fallback (never read/written).
        reshade::api::resource_view sv[6] = {src, d->edges_srv,
            d->vbgi_output_srv.handle ? d->vbgi_output_srv : d->fallback_srv,  // raw GI
            d->fallback_srv,                                                    // history AO (unused)
            d->depth_mips_srv,
            d->captured_mrt_normal_srv.handle ? d->captured_mrt_normal_srv : d->fallback_srv}; // MRT normal
        reshade::api::resource_view dn_uavs[3] = {dst_uav,
            gi_denoised_uav,  // denoised GI (half buffer in Half mode)
            d->fallback_uav};                                                      // history AO (unused)
        reshade::api::descriptor_table_update u[4] = {
          {{},0,0,1,reshade::api::descriptor_type::sampler,&d->point_clamp_sampler},
          {{},0,0,1,reshade::api::descriptor_type::constant_buffer,&d->captured_scene_cbv_view},
          {{},0,0,6,reshade::api::descriptor_type::texture_shader_resource_view,sv},
          {{},0,0,3,reshade::api::descriptor_type::texture_unordered_access_view,dn_uavs},
        };
        apply_descriptors(d->denoise_layout, &d->denoise_tables, 4, u);
        auto pc = BuildGTVBAOPushConstants(d, last, -1.f, false, /*stage*/0);
        cl->push_constants(CS, d->denoise_layout, kGtvbaoPushConstantsLayoutParam, 0, kGtvbaoPushConstantFloats, pc.data());
        // denoise_last threads cover 2 px each: halve the grid (bounds-fail covers overhang).
        cl->dispatch((aw + 15) / 16, (ah + 7) / 8, 1);
        bar(dst_tex, UA, SR);
        use_a = !use_a;
        if (last) {
          d->gtvbao_final_in_b = !use_a;
        }
      }
    }

    // �� Half mode: full-resolution joint reconstruction (4-Tap) ��
    // Half AO/GI (+ half denoise above) -> full AO/GI for t22/t23 +
    // next-frame multibounce. The joint reconstruction consumes pre-decoded
    // full-res normals, so the normal prep must run when the �-trous chain
    // (which produces them) is off.
    reshade::api::pipeline up_pipe = d->upscale_pipeline;
    if (half_mode && up_pipe.handle && d->upscale_ao_uav.handle
        && d->upscale_gi_uav.handle) {
      // Demote the GI tail output to SR BEFORE any descriptor that samples it
      // is published. update_descriptor_tables copies descriptors into the GPU
      // heap at call time, so publishing t1 while the resource is still UAV
      // bound captures an unusable view and the upscale Load() returns zero.
      // This is the same ordering the main pass uses for vbgi_output
      // (dispatch -> bar -> apply_descriptors), which is known to work.
      bar(gi_denoised_tex, UA, SR);
      if (!atrous_active) run_normal_prep();
      bind_pipe(up_pipe);
      reshade::api::resource_view up_ao_src = d->gtvbao_final_in_b
          ? d->ao_term_b_srv : d->ao_term_a_srv;
      // t1 follows the same shared predicate as the GI tail write above, so
      // the upscale always samples the exact buffer the tail just wrote.
      reshade::api::resource_view up_gi_src = gi_half_chain
          ? d->vbgi_denoised_half_srv
          : d->fallback_srv;
      reshade::api::resource_view up_srvs[5] = {
          up_ao_src.handle ? up_ao_src : d->fallback_srv,                    // t0 half AO denoised
          up_gi_src.handle ? up_gi_src : d->fallback_srv,                    // t1 half GI denoised
          d->depth_mips_srv,                                                // t2 full depth MIPs
          d->captured_mrt_normal_srv.handle ? d->captured_mrt_normal_srv    // t3 full MRT
              : d->fallback_srv,
          d->normal_prep_srv.handle ? d->normal_prep_srv : d->fallback_srv}; // t4 full normals
      // Debug UAV only when upscale diagnostics are requested; otherwise the
      // main-pass bitmask/sample-activity debug output survives untouched.
      const bool upscale_dbg_out = shader_injection.gtvbao_upscale_debug > 0.5f
          && d->debug_uav.handle;
      reshade::api::resource_view up_uavs[3] = {
          d->upscale_ao_uav,
          (shader_injection.vbgi_enabled > 0.5f) ? d->upscale_gi_uav : d->fallback_uav,
          upscale_dbg_out ? d->debug_uav : d->fallback_uav};
      reshade::api::descriptor_table_update uu[4] = {
        {{},0,0,1,reshade::api::descriptor_type::sampler,&d->point_clamp_sampler},
        {{},0,0,1,reshade::api::descriptor_type::constant_buffer,&d->captured_scene_cbv_view},
        {{},0,0,5,reshade::api::descriptor_type::texture_shader_resource_view,up_srvs},
        {{},0,0,3,reshade::api::descriptor_type::texture_unordered_access_view,up_uavs},
      };
      apply_descriptors(d->upscale_layout, &d->upscale_tables, 4, uu);
      auto pc_u = BuildGTVBAOPushConstants(d, true);
      cl->push_constants(CS, d->upscale_layout, kGtvbaoPushConstantsLayoutParam, 0, kGtvbaoPushConstantFloats, pc_u.data());
      cl->dispatch((w + 7) / 8, (h + 7) / 8, 1);
      bar(d->upscale_ao_texture, UA, SR);
      if (shader_injection.vbgi_enabled > 0.5f)
        bar(d->upscale_gi_texture, UA, SR);
      if (upscale_dbg_out)
        bar(d->debug_texture, UA, SR);  // upscale diagnostics -> t23 read
    }
  }
  // vbgi_denoised is read by the t23 push (VBGI on), debug view 2, and
  // next-frame multibounce (toggle on, valid after this denoise). Skip the
  // flush only when no reader can exist. Half mode serves t23/multibounce
  // from the full-res upscale result instead.
  {
    const bool denRead = shader_injection.vbgi_enabled > 0.5f
        || (shader_injection.vbgi_debug_view > 0.5f
            && (int)shader_injection.vbgi_debug_view == 2)
        || shader_injection.vbgi_multibounce > 0.5f;
    if (denRead) {
      if (half_mode) {
        if (d->upscale_gi_texture.handle)
          bar(d->upscale_gi_texture, UA, SR);  // Full VBGI ready for t23 read
      } else {
        bar(d->vbgi_denoised_texture, UA, SR);  // Denoised GI ready for t23 read
      }
    }
  }
  if (!d->vbgi_denoised_valid) {
    d->vbgi_denoised_valid = true;            // Multi-bounce feedback active next frame
    if (shader_injection.vbgi_debug_logging > 0.5f)
      reshade::log::message(reshade::log::level::info,
          "[SSGI] MultiBounce: denoised GI now valid � accumulate will run next frame.");
  }
  if (shader_injection.gtvbao_debug_logging > 0.5f)
    reshade::log::message(reshade::log::level::info, "[GTVBAO] All passes complete.");
  CSLog("gtvbao", "run exit ok");
  return true;
}

}  // namespace

extern "C" __declspec(dllexport) constexpr const char* NAME = "Falcom Engine+";
extern "C" __declspec(dllexport) constexpr const char* DESCRIPTION =
    "Falcom Engine+ made by Toru. It supports Beyond the Horizon and Sora 1st at the moment.";

BOOL APIENTRY DllMain(HMODULE h_module, DWORD fdw_reason, LPVOID lpv_reserved) {
  switch (fdw_reason) {
    case DLL_PROCESS_ATTACH:
      if (!reshade::register_addon(h_module)) return FALSE;
      renodx::utils::settings::use_presets = false;
      renodx::mods::shader::force_pipeline_cloning = true;
      renodx::mods::shader::allow_multiple_push_constants = true;
      renodx::mods::shader::expected_constant_buffer_index = 13;
      renodx::mods::shader::expected_constant_buffer_space = 0;
      reshade::register_event<reshade::addon_event::init_device>(OnInitDevice);
      reshade::register_event<reshade::addon_event::destroy_device>(OnDestroyDevice);
      reshade::register_event<reshade::addon_event::init_swapchain>(OnInitSwapchain);
      reshade::register_event<reshade::addon_event::destroy_swapchain>(OnDestroySwapchain);
      reshade::register_event<reshade::addon_event::present>(OnPresent);
      reshade::register_event<reshade::addon_event::bind_descriptor_tables>(OnBindDescriptorTables);
      reshade::register_event<reshade::addon_event::push_descriptors>(OnPushDescriptorsCapture);
      reshade::register_event<reshade::addon_event::bind_render_targets_and_depth_stencil>(OnBindRenderTargetsRCAS);
      reshade::register_event<reshade::addon_event::destroy_resource>(OnDestroyResource);
      reshade::register_event<reshade::addon_event::destroy_resource_view>(OnDestroyResourceView);
      reshade::register_event<reshade::addon_event::init_pipeline>(OnInitPipelineCapture);
      reshade::register_event<reshade::addon_event::destroy_pipeline>(OnDestroyPipelineCapture);
      break;
    case DLL_PROCESS_DETACH:
      s_watchdogStop.store(true);
      if (s_watchdogThread.joinable()) s_watchdogThread.join();
      reshade::unregister_event<reshade::addon_event::init_device>(OnInitDevice);
      reshade::unregister_event<reshade::addon_event::destroy_device>(OnDestroyDevice);
      reshade::unregister_event<reshade::addon_event::init_swapchain>(OnInitSwapchain);
      reshade::unregister_event<reshade::addon_event::destroy_swapchain>(OnDestroySwapchain);
      reshade::unregister_event<reshade::addon_event::present>(OnPresent);
      reshade::unregister_event<reshade::addon_event::bind_descriptor_tables>(OnBindDescriptorTables);
      reshade::unregister_event<reshade::addon_event::push_descriptors>(OnPushDescriptorsCapture);
      reshade::unregister_event<reshade::addon_event::bind_render_targets_and_depth_stencil>(OnBindRenderTargetsRCAS);
      reshade::unregister_event<reshade::addon_event::destroy_resource>(OnDestroyResource);
      reshade::unregister_event<reshade::addon_event::destroy_resource_view>(OnDestroyResourceView);
      reshade::unregister_event<reshade::addon_event::init_pipeline>(OnInitPipelineCapture);
      reshade::unregister_event<reshade::addon_event::destroy_pipeline>(OnDestroyPipelineCapture);
      reshade::unregister_addon(h_module);
      break;
  }
  renodx::utils::settings::Use(fdw_reason, &settings);
  renodx::mods::shader::Use(fdw_reason, custom_shaders, &shader_injection);
  return TRUE;
}

