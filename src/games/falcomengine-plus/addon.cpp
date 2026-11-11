/*
 * Copyright (C) 2026
 * SPDX-License-Identifier: MIT
 */

#define ImTextureID ImU64

// DynCube uses its own gated 1/sec log via dynCube_debug_logging instead.

#include <deps/imgui/imgui.h>
#include <include/reshade.hpp>

#include <embed/shaders.h>

#include <Windows.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <map>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <sstream>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../../mods/shader.hpp"
#include "../../utils/descriptor.hpp"
#include "../../utils/dlss_hook.hpp"
#include "../../utils/pipeline_layout.hpp"
#include "../../utils/settings.hpp"
#include "../../utils/shader.hpp"
#include "../../utils/state.hpp"
#include "./fast_noise_ea.h"     // baked-in fast_noise_ea.dds (embed_file.exe output)
#include "./kai_hero_light.hpp"  // Kai hero light: matches and zeroes the hero light in mapped light records
#include "./shared.h"
#include "./world/bvh/deform_probe.hpp"  // native D3D11 event query (DynCube validity readback poll)
#include "./world/bvh/gpu_timer.hpp"     // GpuTimer (Dynamic Cubemaps GPU Timing Log)
#include "./world/rr/rr_state.hpp"
#include "./world/rtao/rtao.hpp"       // RTAO round 1 (inline lighting-hook dispatch)
#include "./world/rtao/rtao_svgf.hpp"  // RTAO SVGF glue (inline lighting-hook dispatch)
#include "./world/rtxdi/rtxdi_debug_layout.h"
#include "./world/world.hpp"  // Phase 0 world-space research module (DevKit-only)

namespace {

static_assert(sizeof(ShaderInjectData) == 1784u);
static_assert(alignof(ShaderInjectData) == alignof(float));
static_assert(offsetof(ShaderInjectData, cs_contact_local_light_clearance) == 80u);

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
    // the CharShadowMode setting: the contact term owns character shadowing
    // unless the user deliberately turns the engine's own back on.
    .char_shadow_mode = 0.f,
    .rtxdi_local_bound = 0.f,
    .rtxdi_sun_bound = 0.f,
    .rtxdi_sun_darkening = 1.f,
    .rtao_debug_show = 0.f,
    .rtxdi_sun_character_darkening = 1.f,
    .rtxdi_local_darkening = 1.f,
    .cs_contact_local_light_clearance = 0.2f,
    .rtxdi_local_character_darkening = 1.f,
    .rtxdi_local_character_keep = 1.f,
    .rtxdi_local_character_intensity = 1.f,
    .rtxdi_local_character_fallback = 1.f,
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
    .rtls_flags = 0.f,
    .rtls_strength = 1.f,
    .rtls_max_darkening = 1.f,
    .rtls_char_lamp = 1.f,
    .rtsun_flags = 0.f,
    .rtsun_strength = 1.f,
    .rtsun_max_darkening = 1.f,
    .rtsun_spare = 0.f,
    .rtsun_char_max_darkening = 1.f,
    .cs_contact_local_bound = 0.f,
    .cs_contact_local_normal_bias = 0.05f,
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
    .vbgi_multibounce = 1.f,
    .vbgi_multibounce_strength = 1.f,
    .vbgi_multibounce_bounce_fraction = 0.15f,
    .vbgi_multibounce_saturation = 1.f,
    .vbgi_multibounce_max_clamp = 0.f,
    .rtxdi_sun_foliage_tint_r = 1.f,
    .rtxdi_sun_foliage_tint_g = 1.f,
    .rtxdi_sun_foliage_tint_b = 1.f,
    .rtxdi_sun_foliage_transmission = 0.f,
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
    .char_gtvbgi_mask_strength = 0.75f,
    .gtvbao_prefilter_enabled = 1.f,
    .brdf_hammon_diffuse_enabled = 0.f,
    .brdf_multiscatter_specular_enabled = 0.f,
    .brdf_diffuse_strength = 1.f,
    .brdf_specular_strength = 1.f,
    .brdf_specular_peak_clamp = 4.f,
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
    .gtvbao_atrous_passes = 3.f,
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
    .character_outline_thickness = 1.f,
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
    // �� GTVBAO low-resolution spatial pipeline defaults: Full = existing behavior ��
    .gtvbao_resolution = 0.f,
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
    .rtao_active = 0.f,
    .mb_camera_sign = 1.f,
    .mb_camera_jitter = 1.f,
    .mb_camera_cut = 1.f,
    .mb_camera_cut_px = 120.f,
    // -- Contact Shadows --
    .cs_contact_denoise_enabled = 1.f,
    .cs_contact_denoise_radius = 4.f,
    .cs_contact_denoise_depth_sigma = 0.02f,
    .cs_contact_denoise_normal_sigma = 0.1f,
    .cs_contact_denoise_passes = 1.f,
    .cs_denoise_axis = 0.f,
    .cs_contact_local_denoise_enabled = 1.f,
    .cs_contact_local_denoise_radius = 4.f,
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
    // Medium (8 steps): the local march default moved up from the old raw-slider
    // default of 4 when the sample count became a quality tier, matching the sun
    // march above. Keep in sync with ContactShadowsLocalQuality.
    .cs_contact_local_sample_count = 8.f,
    .cs_contact_local_ray_length = 2.f,
    .cs_contact_local_max_lights = 4.f,
    // Runtime values, written by the addon immediately before the push.
    .cs_noise_frame = -1.f,
    .cs_working_w = 0.f,
    .cs_working_h = 0.f,
    .cs_contact_local_denoise_depth_sigma = 0.02f,
    .cs_contact_local_denoise_normal_sigma = 0.1f,
    .cs_contact_dedicated_bound = 0.f,
    .cs_contact_local_denoise_passes = 1.f,
    // 0 = derive the sun-contact range gate from the engine's last cascade split.
    .cs_contact_sun_range = 0.f,
    // 4 reproduces the original 4-sample appearance exactly, so the estimator change is
    // a no-op at the default Sample Count.
    .cs_contact_response_scale = 4.f,
    // All four local light loops marched by default, which is exactly what the
    // feature did before the per-class toggles existed. Must stay in declaration
    // order with shared.h: designated initialisers are evaluated in the order
    // written, so a mismatch here is a hard compile error rather than a silent
    // misassignment.
    .cs_contact_local_point = 1.f,
    .cs_contact_local_spot = 1.f,
    .cs_contact_local_env_point = 1.f,
    .cs_contact_local_env_spot = 1.f,
    // ── Character Shadowing: custom camera pass ──
    // On by default but only does anything in char_shadow_mode 2. Declaration order must
    // match shared.h.
    .char_cam_enabled = 1.f,
    .char_cam_strength = 1.f,
    .char_cam_sample_count = 8.f,
    // Silhouette reach, in the same neighbourhood as the engine's own march (the vanilla
    // block steps 0.0005..0.002 for 10 steps, so ~0.005..0.02 world units).
    .char_cam_ray_length = 0.02f,
    // Must be comparable to the ray length. A band much larger than the ray puts the
    // whole ray inside or outside it, the coverage estimator degenerates to a binary
    // any-hit test, and Sample Count stops doing anything.
    .char_cam_thickness = 0.008f,
    .char_cam_bias = 0.0001f,
    // The shader folds this into the hit threshold (bias = lift + char_cam_bias) so the
    // receiver cannot occlude itself, so it is the true clearance off the surface. A
    // millimetre suits a 2 cm ray; anything comparable to the ray would start the march
    // in free air beside the face.
    .char_cam_normal_bias = 0.0015f,
    // 2, not 4: at 4 with 8 samples a single spurious sample already costs half the
    // available darkening, which is what made this read as a hard patch. The
    // 4-is-the-original invariant is documented on the environment contact pass and is
    // meaningful THERE, where the geometry is a wall or a floor and its numbers -- a 0.29
    // band against a 2.0 ray -- are proportionate to it. Those numbers are not
    // transferable to a character at all.
    .char_cam_response_scale = 2.f,
    // Below 1 because this multiplies probe-lit ambient and cannot prove the pixel was
    // lit; a face going to solid black reads as a hole.
    .char_cam_max_darkening = 0.6f,
    .char_cam_isfast_enabled = 1.f,
    .char_cam_debug = 0.f,
    .rt_sun_cascades = 0.f,
    .rtxdi_diffuse_bound = 0.f,
    .rtxdi_debug_view = 0.f,
    .rtxdi_debug_bound = 0.f,
    .rtxdi_debug_distance_bound = 0.f,
    .rtxdi_sun_detail_view = static_cast<float>(RTXDI_SUN_DETAIL_STATUS),
    .rtxdi_sun_detail_bound = 0.f,
    // ShaderInjectData ends on a float4 boundary with no pad field, so every float in the
    // struct needs a default here. See the closing comment in shared.h.
};

// ----------- GTVBAO Backend � constants, types, fwd decls -----------

constexpr uint32_t kLightingGtvbaoRegister = 22u;
constexpr uint32_t kLightingVbgiRegister = 23u;     // t23 = vbgiTexture
constexpr uint32_t kLightingRtgiRegister = 37u;     // t37 = rtgiTexture (RTGI final GI, world/rtgi)
constexpr uint32_t kLightingRtlsRegister = 38u;     // t38 = rtlsTexture (RT Local Shadows term, world/rtls)
constexpr uint32_t kLightingRtSunRegister = 39u;    // t39 = rtsunTexture (RT Sun Shadows term, world/rtsun)
constexpr uint32_t kLightingDepthRegister = 4u;     // t4 = depthTexture (Sora)
constexpr uint32_t kLightingDepthRegisterKai = 3u;  // t3 = depthTexture (Kai)
constexpr uint32_t kLightingSsaoRegister = 5u;      // t5 = ssaoTexture (Sora)
constexpr uint32_t kLightingSsaoRegisterKai = 4u;   // t4 = ssaoTexture (Kai)
constexpr uint32_t kLightingSceneCbRegister = 0u;   // b0 = cb_scene
constexpr uint32_t kGTVBAODepthMipLevels = 5u;
constexpr uint32_t kGtvbaoDescriptorTableParamCount = 4u;  // sampler, cbv, srv, uav
constexpr uint32_t kGtvbaoPushConstantsLayoutParam = 4u;   // push_constants at b13
// Float count of the GTVBAO push-constant block (cbuffer cb_gtvbao at b13).
// Must match the cbuffer declared in gtvbao_common.hlsl. Named so the builder,
// the layout range and all nine push sites cannot drift apart -- a mismatch
// here silently truncates the tail of the block rather than failing loudly.
constexpr uint32_t kGtvbaoPushConstantFloats = 48;
constexpr uint32_t kVbgiFarPushConstantFloats = 56;  // GTVBAO block + far tail c[48..55] (gtvbao_common.hlsl)
constexpr uint32_t kGtvbaoSnapshotFloats = 33;
constexpr uint32_t kLightingMrtNormalRegister = 1u;  // t1 = mrtTexture0 (g-buffer normals)
constexpr uint64_t kGTVBAOStartupGuardFrames = 8u;
constexpr uint64_t kGTVBAOResizeGuardFrames = 4u;
// DynCube loading wipe: color captures older than this mean no lighting draws
// (loading screen) � arm history hard-replace so the next scene rebuilds clean.
constexpr uint64_t kDynCubeLoadingStaleFrames = 5u;
// DynCube validity readback: consecutive immediate-context ticks an armed staging copy may
// stay incomplete before the blocking map runs anyway (more than twice the DXGI default queue of 3).
constexpr uint32_t DYNCUBE_VALIDITY_MAX_WAIT_TICKS = 8u;
constexpr uint64_t kSceneCbMinimumBytes = 95u * 16u;

// -- Contact Shadows (shadows/*.cs_5_0.hlsl) --
// One compute pass per entry, so one layout/pipeline/table slot each. The index
// is fixed and used to size the DeviceData arrays, so the shader, the host and
// the two resource sets cannot drift apart.
enum ShadowsPass : uint32_t {
  kShadowsPassContact = 0u,       // contact_shadows.cs_5_0.hlsl  (t0 depth, t1 mrt, t2 noise -> u0)
  kShadowsPassDenoise = 1u,       // contact_denoise.cs_5_0.hlsl  (t0 depth, t1 mrt, t2 contact -> u0), one axis per dispatch
  kShadowsPassLocal = 2u,         // contact_local.cs_5_0.hlsl  (t0 depth, t1 mrt, t2 noise, t3 lamps, t4 light cells, t5 material index, t6 materials -> u0), one dispatch writes all four channels
  kShadowsPassLocalDenoise = 3u,  // contact_local_denoise.cs_5_0.hlsl (t0 depth, t1 mrt, t2 local -> u0), one axis per dispatch
  kShadowsPassCount = 4u,
};
// The lighting pixel shaders read the result from this slot. t34 is declared by no
// game's lighting shader and claimed by no existing push, so it is free in Sora 1st,
// Sora 2nd, Kai, Kai-soft, Kyoto and Daybreak 2 alike (the addon already owns t17,
// t22, t23, t24, t28-t32).
constexpr uint32_t kLightingContactShadowRegister = 34u;
// Inputs for the local-light march, which runs inside the dynamic light loop
// rather than in a pass. Declared by the lighting shaders and the character pass
// for the shadow path alone; the host never writes to the game's own depth or
// noise registers on their behalf.
constexpr uint32_t kLightingShadowNoiseRegister = 35u;
constexpr uint32_t kLightingShadowDepthRegister = 36u;
// The lighting shaders read the local contact term from this slot. Sora 1st, Sora 2nd, Kai and Kai soft's lighting shaders declare t40 (csLocalContactTex), for this term alone, and no other push claims it.
constexpr uint32_t kLightingLocalContactRegister = 40u;
// The character and SSAO passes bind the same noise volume at the same register as
// the lighting shaders. One number, not two, so the two families of shaders cannot
// drift onto different registers and leave one of them sampling a stale descriptor.
// The alias exists only because the constant above is named for the lighting
// shaders, which is where it was first needed.
constexpr uint32_t kCharShadowNoiseRegister = kLightingShadowNoiseRegister;
// SRV counts per pass, matching the register() declarations in each shader.
constexpr uint32_t kShadowsSrvPerPass[kShadowsPassCount] = {3u, 3u, 7u, 3u};
constexpr uint32_t kShadowsUavPerPass[kShadowsPassCount] = {1u, 1u, 1u, 1u};

// -- Motion Blur (Guertin et al. 2013) --
// Fixed pass order; the indices address the layout/pipeline/table arrays in
// DeviceData so the stages cannot drift out of sync. Declared here because
// DeviceData sizes its arrays with it.
//
// Prep is TileMax and NeighborMax, then Gather, with optional Resolve, Downsample and
// Composite. There is deliberately no full-resolution
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
  kMbDownsample = 0,
  kMbTileMax = 1,
  kMbNeighborMax = 2,
  kMbGather = 3,
  kMbComposite = 4,
  // Appended last so the indices above keep their meaning. Splits the game's
  // single motion texture into camera and object components at per-pixel
  // resolution, which TileMax cannot do because it is a reduction. Writes one
  // blended vector in .xy, so every pass above is unchanged by it, and is SKIPPED
  // entirely when both weights are 1 (camera + (game - camera) == game).
  kMbResolve = 5,
  // Appended last so the indices above keep their meaning. The same resolve
  // source compiled with the wide (RGBA16F, .zw camera term) output declaration,
  // selected by RunMotionBlur only while Debug View is 8 or 9. A separate pass
  // rather than a runtime pipeline swap keeps both pipelines created at startup,
  // so the toggle costs nothing but the surface rebuild.
  kMbResolveWide = 6,
  kMotionBlurPassCount = 7,
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
static float g_mb_cs_dispatch_fix = 0.f;
static float g_capture_perf_log = 0.f;
static float g_dyncube_gpu_timing = 0.f;  // Dynamic Cubemaps > GPU Timing Log (1/sec): per-step GPU ms in the [DynCube] line
static float g_dyncube_cs_dispatch_fix = 2.f;

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
// -- Contact shadow quality ladder ---------------------------------------------
// Replaces the raw Sample Count sliders on both the sun and local marches, and
// backs the .parse() of the two Quality settings below. The stored setting value
// is a 0-3 index; parse() writes the step count straight into the injected field,
// so the push block and every shader keep seeing a plain march-step count.
static float ContactShadowQualitySamples(float quality) {
  static constexpr float samples[] = {4.f, 8.f, 12.f, 16.f};
  const int index = static_cast<int>(quality);
  return samples[index < 0 ? 0 : (index > 3 ? 3 : index)];
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
static float g_gtvbao_normal_input_mode = 1.f;       // 1 = use the g-buffer normal
static float g_gtvbao_normal_influence = 1.f;        // xy scale: 1 = untouched
static float g_gtvbao_normal_z_preservation = 1.f;   // z  scale: 1 = untouched
static float g_gtvbao_normal_depth_blend = 1.f;      // MRT weight before shaping
static float g_gtvbao_normal_sharpness = 1.f;        // gamma: 1 = passthrough
static float g_gtvbao_normal_edge_rejection = 0.f;   // 0 = no silhouette rejection
static float g_gtvbao_normal_detail_response = 0.f;  // 0 = no disagreement reweight
static float g_gtvbao_normal_max_darkening = 1.f;    // cap: 1 = uncapped
static float g_gtvbao_hemisphere_weighting = 0.f;
static float g_gtvbao_auto_thickness = 1.f;
static float g_gtvbao_base_thickness = 0.20f;
static float g_gtvbao_upscale_algorithm = 3.f;
static float g_gtvbao_upscale_plane_sigma = 0.02f;
static float g_gtvbao_upscale_normal_power = 64.f;
static float g_gtvbao_upscale_bilinear_sharpness = 1.f;
static float g_gtvbao_upscale_nearest_fallback = 1.f;

// -- Dynamic Cubemaps (Sora 2nd) � standalone t17 replacement --
constexpr uint32_t kDynCubeRegister = 17u;         // t17 texEnvMap_g
constexpr uint32_t kDynCubeHistPosRegister = 29u;  // t29 dynCubeHistPosTex (debug 11/12)
constexpr uint32_t kDynCubeVanillaRegister = 30u;  // t30 dynCubeVanillaTex (vanilla cube fallback)
constexpr uint32_t kDynCubeSSRRegister = 31u;      // t31 dynCubeSSRTex (blurred SSR result)
constexpr uint32_t kDynCubeSSRRawRegister = 32u;   // t32 dynCubeSSRRawTex (raw SSR, debug 17)
constexpr uint32_t kDynCubeSSRLayoutVersion = 6u;
constexpr uint32_t kDynCubeSSRBlurLayoutVersion = 6u;
constexpr uint32_t kDynCubeWorldBoxLayoutVersion = 2u;
constexpr uint32_t kDynCubeWorldBoxChunkPartials = 256u;  // must match kChunkPartials in DynCubeBoundsReduceCS
constexpr uint32_t kDynCubeCaptureLayoutVersion = 2u;     // bump when the capture pipeline layout shape changes (forces recreate)
constexpr uint32_t kRCASLayoutVersion = 5u;               // bump when the RCAS pipeline layout shape changes (forces recreate)
constexpr uint32_t kFXAALayoutVersion = 1u;               // bump when the FXAA pipeline layout shape changes (forces recreate)
// Strip sRGB encoding for UAV-compatible temp storage and raw (non-decoding)
// SRV reads. Copies stay bitwise so the game keeps decoding exactly as before.
// Non-sRGB formats map to identity (FP16 HDR path unchanged).
static reshade::api::format RCASLinearFormat(reshade::api::format fmt) {
  using F = reshade::api::format;
  switch (fmt) {
    case F::r8g8b8a8_unorm_srgb: return F::r8g8b8a8_unorm;
    case F::b8g8r8a8_unorm_srgb: return F::b8g8r8a8_unorm;
    default:                     return fmt;
  }
}
constexpr uint32_t kDynCubeDefaultSize = 128u;
static uint32_t DynCubeResolveSize(float v);

// -- VBGI settings and state (inert until the VBGI pass lands) --
static float g_vbgi_enabled = 0.f;
static float g_vbgi_quality_level = 2.f;
static float g_vbgi_resolution = 1.f;
constexpr int kVbgiMaxResolutionIndex = 3;  // VBGIResolution label count - 1 (1/8)
static float g_vbgi_upscale_algorithm = 3.f;
static float g_vbgi_upscale_plane_sigma = 0.02f;
static float g_vbgi_upscale_normal_power = 1.f;
static float g_vbgi_upscale_bilinear_sharpness = 1.f;
static float g_vbgi_upscale_nearest_fallback = 1.f;
static float g_vbgi_noise_type = 2.f;
static float g_vbgi_radius = 1.5f;
static float g_vbgi_bitmask_thickness = 0.30f;
static float g_vbgi_auto_thickness = 1.f;
static float g_vbgi_base_thickness = 0.20f;
static float g_vbgi_depth_mip_offset = 6.f;
static float g_vbgi_mrt_normal = 1.f;
static float g_vbgi_ambient = 0.5f;
static float g_vbgi_lights = 1.f;
static float g_vbgi_sun = 1.f;
static float g_vbgi_emissive = 1.f;
static float g_vbgi_multibounce_strength = 0.5f;
static float g_vbgi_debug_mode = 0.f;
static float g_vbgi_backface_mode = 1.f;
static float g_vbgi_far_enabled = 1.f;
static float g_vbgi_far_distance = 3.f;
static float g_vbgi_far_intensity = 2.f;
static float g_vbgi_far_hit_window = 0.f;
static float g_vbgi_far_temporal = 1.f;
static float g_vbgi_far_history_length = 30.f;
static float g_vbgi_far_spatial = 1.f;
static float g_vbgi_far_candidates = 4.f;
static float g_vbgi_far_spatial_neighbors = 4.f;
static float g_vbgi_far_spatial_radius = 1.f;
static float g_vbgi_far_multibounce_strength = 0.25f;

// -- IS-FAST noise --
static float g_isfast_enabled = 0.f;
static float g_isfast_strength = 1.f;
static float g_isfast_debug_logging = 0.f;
static float g_isfast_spatial_scale = 1.f;
static float g_isfast_temporal_speed = 1.f;
static float g_isfast_seed_offset = 0.f;

// -- Settings visibility --
static float g_settings_mode = 0.f;  // 0=Basic, 1=Advanced
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
static bool VbgiGameSupported() { return !IsKyoto() && !IsDaybreak2(); }
static bool VbgiRowVisible() { return IsAdvancedSettingsMode() && VbgiGameSupported(); }
static bool VbgiRunning() { return g_vbgi_enabled > 0.5f && VbgiGameSupported() && !falcom_world::rtao::RtaoRequested(); }
static bool VbgiFarRunning() { return VbgiRunning() && g_vbgi_far_enabled > 0.5f; }
// Local contact shadows run as a compute pass (shadows/contact_local.cs_5_0.hlsl) on the titles whose lighting shaders
// read the term at t40. Off while RT Local Shadows is requested, as the Local Lights row is.
static bool LocalContactComputeRunning() {
  return shader_injection.cs_contact_local_enabled > 0.5f && (IsSora1st() || IsSora2nd() || IsKai()) && !falcom_world::rtls::RtlsRequested();
}

// -- Lighting shader identification (Sora + Kai) --
static bool IsLightingShader(uint32_t hash) {
  return hash == 0xFDAAF80Eu      // Sora lighting
         || hash == 0xCA3D8596u   // Sora 2nd lighting
         || hash == 0x0CDCB258u   // Kyoto lighting
         || hash == 0x430ED091u   // Kai lighting
         || hash == 0xF6C55E5Fu;  // Kai lighting soft
}

// -- CPU optimization toggles --
static float g_gtvbao_cs_dispatch_fix = 0.f;    // 0=Off, 1=Restore, 2=Null, 3=Null+Restore
static float g_cpuopt_deferred_dispatch = 1.f;  // dispatch GTVBAO/VBGI in OnPresent, not inline (default ON for Kai)
static float g_cpuopt_ensure_pipelines = 0.f;   // kai-style: don't destroy/recreate pipelines every frame

static float g_gtvbao_full_res_denoise = 0.f;
static float g_gtvbao_temporal_enabled = 0.f;
static float g_gtvbao_history_weight = 0.8f;
static float g_gtvbao_depth_rejection = 0.200f;
static float g_gtvbao_normal_rejection = 0.f;
static float g_gtvbao_svgf_spatial_enabled = 1.f;
static float g_gtvbao_svgf_history_length = 128.f;
static float g_gtvbao_svgf_radius_scale = 1.f;
static float g_gtvbao_svgf_half_res_radius_scale = 1.f;
static float g_gtvbao_svgf_iterations = 1.f;
static float g_gtvbao_svgf_half_res_iterations = 1.f;
static float g_gtvbao_svgf_luminance_edge_stop = 16.f;
static float g_gtvbao_svgf_depth_sigma = 0.10f;
static float g_gtvbao_svgf_normal_power = 128.f;
static float g_vbgi_full_res_denoise = 0.f;
static float g_vbgi_temporal_enabled = 0.f;
static float g_vbgi_temporal_far_only = 1.f;
static float g_vbgi_history_weight = 0.99f;
static float g_vbgi_depth_rejection = 0.05f;
static float g_vbgi_normal_rejection = 0.9f;
static float g_vbgi_spatial_enabled = 1.f;
static float g_vbgi_svgf_history_length = 128.f;
static float g_vbgi_svgf_radius_scale = 4.f;
static float g_vbgi_svgf_iterations = 5.f;
static float g_vbgi_svgf_luminance_edge_stop = 16.f;
static float g_vbgi_svgf_depth_sigma = 8.f;
static float g_vbgi_svgf_normal_power = 128.f;
static float g_gtvbao_debug_ui = 0.f;  // GTVBAODebugMode index into kGtvbaoDebugModes
constexpr int kGtvbaoDebugModes[7] = {0, 1, 22, 23, 24, 25, 26};
// VBGI Debug View row (0=Off, 1..6) maps to GTVBAO debug modes 30..35 while g_vbgi_enabled.
static int VbgiGtvbaoDebugMode() {
  if (!VbgiRunning()) return 0;
  const int row = std::clamp((int)g_vbgi_debug_mode, 0, 9);
  return row == 0 ? 0 : 29 + row;
}
// AO-only: the VBGI row replaces the AO debug view, never mixes with it.
static int GtvbaoDebugMode() {
  return VbgiGtvbaoDebugMode() != 0 ? 0 : kGtvbaoDebugModes[std::clamp((int)g_gtvbao_debug_ui, 0, 6)];
}
// What the lighting shaders show: VBGI wins when its row is set.
static int ActiveGtvbaoDebugMode() {
  const int v = VbgiGtvbaoDebugMode();
  return v != 0 ? v : GtvbaoDebugMode();
}
static bool IsGtvbaoComputeDebug(int mode) { return mode >= 22 && mode <= 38; }

using GTVBAODescriptorTableSet =
    std::array<reshade::api::descriptor_table, kGtvbaoDescriptorTableParamCount>;

// Set by the "Hero Light Toggle" overlay setting: 1 patches the Kai hero light in the mapped light buffer.
static float g_kai_hero_light_toggle = 0.f;

struct __declspec(uuid("b1a2c3d4-e5f6-7890-abcd-ef1234567890")) DeviceData {
  uint32_t working_width = 0u;
  uint32_t working_height = 0u;

  reshade::api::resource depth_mips_texture = {};
  reshade::api::resource_view depth_mips_srv = {};
  std::array<reshade::api::resource_view, kGTVBAODepthMipLevels> depth_mips_uavs = {};

  reshade::api::resource ao_term_a_texture = {};
  reshade::api::resource_view ao_term_a_srv = {};
  reshade::api::resource_view ao_term_a_uav = {};

  // 1�1 white fallback � always valid, returned when GTVBAO is off / not ready.
  reshade::api::resource fallback_texture = {};
  reshade::api::resource_view fallback_srv = {};

  reshade::api::sampler point_clamp_sampler = {};
  reshade::api::sampler linear_clamp_sampler = {};

  // Resolution-change guard.
  uint32_t last_created_game_width = 0u;
  uint32_t last_created_game_height = 0u;
  // Low-resolution spatial pipeline (0=Full existing behavior, 1=Half).
  uint32_t ao_width = 0u;
  uint32_t ao_height = 0u;
  float last_created_gtvbao_resolution = -1.f;
  float last_created_vbgi_resolution = -1.f;

  reshade::api::pipeline_layout prefilter_layout = {};
  reshade::api::pipeline_layout main_layout = {};
  reshade::api::pipeline prefilter_pipeline = {};
  reshade::api::pipeline main_pipeline = {};
  reshade::api::pipeline vbgi_main_pipeline = {};
  reshade::api::pipeline_layout vbgi_main_layout = {};
  reshade::api::pipeline_layout vbgi_radiance_layout = {};
  reshade::api::pipeline vbgi_radiance_pipeline = {};
  reshade::api::pipeline_layout vbgi_mips_layout = {};
  reshade::api::pipeline vbgi_mips_pipeline = {};
  GTVBAODescriptorTableSet vbgi_radiance_tables = {};
  GTVBAODescriptorTableSet vbgi_mips_tables = {};
  reshade::api::pipeline_layout vbgi_radiance_far_layout = {};  // radiance prepass with far feedback: 14 SRVs, 1 UAV
  reshade::api::pipeline vbgi_radiance_far_pipeline = {};
  GTVBAODescriptorTableSet vbgi_radiance_far_tables = {};
  uint32_t temporal_seen_w = 0u;
  uint32_t temporal_seen_h = 0u;
  bool gtvbao_temporal_last = false;
  uint64_t gtvbao_last_frame = UINT64_MAX;
  std::array<float, kGtvbaoSnapshotFloats> gtvbao_prev_params = {};
  bool gtvbao_prev_params_valid = false;
  float gtvbao_prev_svgf_history_length = 32.f;
  float gtvbao_prev_svgf_radius_scale = 1.f;

  // Descriptor tables � pre-allocated per pass.
  GTVBAODescriptorTableSet prefilter_tables = {};
  GTVBAODescriptorTableSet main_tables = {};
  GTVBAODescriptorTableSet vbgi_main_tables = {};
  reshade::api::resource vbgi_raw_texture = {};
  reshade::api::resource_view vbgi_raw_srv = {}, vbgi_raw_uav = {};
  uint32_t vbgi_width = 0u, vbgi_height = 0u;
  // VBGI radiance inputs (lighting draw): 0 mrt1 t2, 1 mrt2 t3, 2 deferredParams t6, 3 shadow t7 (Sora 2nd) / t9 (Sora 1st), 4 dynamicLights t11, 5 lightIndices t12.
  reshade::api::resource_view vbgi_in_srv[6] = {};
  uint64_t vbgi_in_frame[6] = {UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX};
  uint64_t vbgi_in_res[6] = {};            // resource of vbgi_in_srv[k] (destroy-event liveness)
  std::atomic<bool> vbgi_in_live[6] = {};  // false after the view or its resource is destroyed
  reshade::api::resource vbgi_radiance_texture = {};
  reshade::api::resource_view vbgi_radiance_srv = {};
  reshade::api::resource_view vbgi_radiance_mip_srv[4] = {};
  reshade::api::resource_view vbgi_radiance_mip_uav[5] = {};
  uint64_t vbgi_radiance_frame = UINT64_MAX;
  falcom_world::rtgi::RtgiDenoiseSet vbgi_denoise = {};
  falcom_world::rtgi::RtgiDenoiseSet vbgi_far_color_denoise = {};
  bool vbgi_far_color_last = false;
  uint64_t vbgi_far_color_frame = UINT64_MAX;
  uint32_t vbgi_far_color_full_width = 0u, vbgi_far_color_full_height = 0u;
  std::array<float, 6> vbgi_far_color_prev_params = {};
  bool vbgi_far_color_prev_params_valid = false;
  falcom_world::rtgi::RtgiDenoiseSet gtvbao_denoise = {};
  falcom_world::rtgi::RtgiTextureSet<1> gtvbao_svgf_raw = {};  // rgba16f, AO size: rgb = visibility, a = 1
  reshade::api::resource vbgi_stats_buffer = {};
  reshade::api::resource_view vbgi_stats_uav = {};
  uint64_t vbgi_gi_frame = UINT64_MAX;
  bool vbgi_temporal_last = false;
  uint64_t vbgi_last_frame = UINT64_MAX;
  bool vbgi_prev_params_valid = false;
  std::array<float, 21> vbgi_prev_params = {};
  // -- VBGI far field (one candidate per texel, resolve into the joint raw GI) --
  reshade::api::pipeline_layout vbgi_far_main_layout = {};  // far main variant: 5 SRVs, 5 UAVs, 56-float push
  reshade::api::pipeline vbgi_far_main_pipeline = {};
  GTVBAODescriptorTableSet vbgi_far_main_tables = {};
  reshade::api::pipeline_layout vbgi_far_layout = {};  // far passes: 12 SRVs, 4 UAVs, 56-float push
  GTVBAODescriptorTableSet vbgi_far_tables = {};
  reshade::api::pipeline vbgi_far_resolve_pipeline = {};
  reshade::api::pipeline vbgi_far_compose_pipeline = {};
  falcom_world::rtgi::RtgiTextureSet<1> vbgi_far_cand_pos = {};  // rgba32f, trace size: world sample point, W
  falcom_world::rtgi::RtgiTextureSet<1> vbgi_far_cand_rad = {};  // rgba16f, trace size: sender radiance, M
  falcom_world::rtgi::RtgiTextureSet<1> vbgi_far_cand_nrm = {};  // rgba16f, trace size: world sender normal, mismatch
  falcom_world::rtgi::RtgiTextureSet<1> vbgi_far_raw = {};       // rgba16f, trace size: far raw GI
  falcom_world::rtgi::RtgiTextureSet<1> vbgi_far_joint = {};     // rgba16f, trace size: near raw + far (denoise input)
  bool vbgi_far_was_on = false;                                  // far state last RunVBGI (near history resets on a change)
  uint64_t vbgi_far_frame = UINT64_MAX;                          // last frame the far resolve ran
  bool vbgi_far_prev_params_valid = false;
  std::array<float, 11> vbgi_far_prev_params = {};
  reshade::api::pipeline vbgi_far_temporal_pipeline = {};
  falcom_world::rtgi::RtgiTextureSet<2> vbgi_far_res_pos = {};  // rgba32f, trace size: temporal reservoir sample point, W
  falcom_world::rtgi::RtgiTextureSet<2> vbgi_far_res_rad = {};  // rgba16f, trace size: sender radiance, M
  falcom_world::rtgi::RtgiTextureSet<2> vbgi_far_res_nrm = {};  // rgba16f, trace size: sender normal, refreshed flag
  uint32_t vbgi_far_res_index = 0u;                             // [index] holds last frame's temporal reservoir
  bool vbgi_far_reuse_last = false;                             // temporal reuse ran in the last far frame
  uint64_t vbgi_far_reuse_frame = UINT64_MAX;                   // last frame the far reservoir step ran
  reshade::api::pipeline vbgi_far_spatial_pipeline = {};
  falcom_world::rtgi::RtgiTextureSet<1> vbgi_far_sp_pos = {};  // rgba32f, trace size: spatial reservoir sample point, W
  falcom_world::rtgi::RtgiTextureSet<1> vbgi_far_sp_rad = {};  // rgba16f, trace size: sender radiance, M
  falcom_world::rtgi::RtgiTextureSet<1> vbgi_far_sp_nrm = {};  // rgba16f, trace size: sender normal, accepted fraction

  reshade::api::resource_view captured_depth_srv = {};
  std::atomic<bool> captured_depth_live{true};            // destroy-event driven; false = target freed since capture
  uint64_t captured_depth_res = 0u;                       // resource behind the view (destroy matching)
  std::string captured_depth_dims = "none";               // cached at capture (push context only)
  uint32_t captured_depth_w = 0u, captured_depth_h = 0u;  // same, numeric (shadow pass sizing)
  reshade::api::resource_view captured_ssao_srv = {};
  // The game SSAO is only ever an OPTIONAL input, so unlike depth
  // and the MRT normal it has no recorded resource and no destroy-event
  // tracking. The frame stamp is what tells a consumer "captured this frame"
  // from "left over from an earlier frame, possibly a different resolution".
  uint64_t captured_ssao_frame = 0u;
  reshade::api::resource_view captured_mrt_normal_srv = {};
  std::atomic<bool> captured_mrt_live{true};                  // destroy-event driven; false = target freed since capture
  uint64_t captured_mrt_res = 0u;                             // resource behind the view (destroy matching)
  std::string captured_mrt_dims = "none";                     // cached at capture (push context only)
  reshade::api::resource_view captured_color_srv = {};        // t0 � lighting input color texture
  std::atomic<bool> captured_color_live{true};                // destroy-event driven; false = target freed since capture
  uint64_t captured_color_res = 0u;                           // resource behind the view (destroy matching)
  std::string captured_color_dims = "none";                   // cached at capture (push context only)
  uint32_t captured_color_w = 0u, captured_color_h = 0u;      // cached dims (SSR sizing, no draw-time query)
  reshade::api::resource_view captured_ssr1_srv = {};         // ssr2-draw t0 � vanilla ssr1 march result (replacement debug view 2)
  std::atomic<bool> captured_ssr1_live{true};                 // destroy-event driven
  uint64_t captured_ssr1_res = 0u;                            // resource behind the view (destroy matching)
  reshade::api::resource_view captured_ssr_mrt_srv = {};      // ssr1-draw t2 � march's own mrt0 (composite gate + normal decode)
  std::atomic<bool> captured_ssr_mrt_live{true};              // destroy-event driven
  uint64_t captured_ssr_mrt_res = 0u;                         // resource behind the view (destroy matching)
  reshade::api::resource_view captured_vanilla_env_srv = {};  // game's texEnvMap_g (t17) binding � vanilla cube fallback
  reshade::api::resource_view captured_scene_cbv_view = {};   // push_descriptors passes CBV as resource_view
  reshade::api::buffer_range captured_scene_cbv = {};
  falcom_world::rtxdi::SceneRangeCapture rtxdi_scene_range;
  bool captured_scene_cbv_valid = false;
  std::atomic<bool> captured_cbv_live{true};  // destroy-event driven; false = buffer freed since capture
  uint64_t captured_cbv_res = 0u;             // buffer behind the range (destroy matching)
  std::string captured_cbv_dims = "none";     // cached at capture (push/bind context only)
  // Deferred snapshot destroy tracking (plain copies at snapshot time; matched like captured).
  std::atomic<bool> defDepthLive{true}, defMrtLive{true}, defCbvLive{true};
  uint64_t deferred_depth_res = 0u, deferred_mrt_res = 0u, deferred_cbv_res = 0u;
  uint64_t captured_scene_cbv_frame = UINT64_MAX;
  uint64_t captured_color_frame = UINT64_MAX;  // frame_index of last lighting t0 capture (stale = loading screen)
  uint64_t captured_depth_frame = UINT64_MAX;  // frame_index of last lighting depth capture (kept for future detectors)
  bool resources_created = false;
  uint64_t frame_index = 0u;
  uint64_t gtvbao_debug_frame = UINT64_MAX;
  float gtvbao_debug_mode_written = 0.f;
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

  reshade::api::resource_view fallback_uav = {};  // 1x1 UAV fallback
  // -- Foliage mask (AO-domain R8_UINT, pre-pass) --
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

  // CPU optimization tracking
  uint64_t last_bound_pipeline_handle = 0u;
  uint64_t last_srv0_handle = 0u;
  uint64_t last_srv1_handle = 0u;
  uint64_t last_uav0_handle = 0u;
  uint64_t last_cbv_handle = 0u;
  uint64_t last_sampler_handle = 0u;

  // -- Dynamic Cubemaps (Sora 2nd) � Phase 0A/B + Phase 1 history --
  // Aliases to the current (just-written) history set � dyncube_srv is the cube SRV for t17.
  reshade::api::resource dyncube_texture = {};   // current color resource (alias)
  reshade::api::resource_view dyncube_srv = {};  // current color cube SRV (alias, t17)
  reshade::api::resource_view dyncube_uav = {};  // current color UAV (alias, capture/solid write)
  reshade::api::sampler dyncube_sampler = {};    // point clamp
  reshade::api::pipeline_layout dyncube_capture_layout = {};
  reshade::api::pipeline dyncube_capture_pipeline = {};
  uint32_t dyncube_capture_layout_version = 0u;  // recreate layout/tables/pipeline when shape changes
  reshade::api::pipeline_layout dyncube_solid_layout = {};
  reshade::api::pipeline dyncube_solid_pipeline = {};
  std::array<reshade::api::descriptor_table, 5> dyncube_capture_tables = {};  // sampler, cbv, srv, texture uavs, buffer uav (scratch)
  GTVBAODescriptorTableSet dyncube_solid_tables = {};
  bool dyncube_resources_created = false;
  uint32_t dyncube_size = kDynCubeDefaultSize;
  bool dyncube_solid_written = false;
  // Ping-pong history sets (Phase 1). Index 0 = A, 1 = B. dyncube_hist_cur = current write set.
  struct {
    reshade::api::resource color;                // RGBA16F cube-compatible, 128x128x6x1
    reshade::api::resource_view color_cube_srv;  // TextureCube SRV (t17)
    reshade::api::resource_view color_arr_srv;   // Texture2DArray SRV (compute prev read)
    reshade::api::resource_view color_uav;       // Texture2DArray UAV (compute current write)
    reshade::api::resource pos;                  // RGBA16F (rgb=scaled pos, a=validity)
    reshade::api::resource_view pos_arr_srv;
    reshade::api::resource_view pos_cube_srv;  // TextureCube SRV (debug 11/12 histPos lookup)
    reshade::api::resource_view pos_uav;
    reshade::api::resource contrib;  // R16F (history contribution for debug 6)
    reshade::api::resource_view contrib_arr_srv;
    reshade::api::resource_view contrib_uav;
  } dyncube_hist[2];
  // GPU camera ping-pong (1x1 RGBA32F): previous frame's camera position.
  reshade::api::resource dyncube_cam[2];
  reshade::api::resource_view dyncube_cam_srv[2];
  reshade::api::resource_view dyncube_cam_uav[2];
  uint32_t dyncube_hist_cur = 0;                      // current write set (0=A,1=B); reads use dyncube_readSet
  bool dyncube_needs_reset = true;                    // clear history + first-frame reset
  bool dyncube_was_enabled = false;                   // rising-edge latch for enabled->reset
  reshade::api::sampler dyncube_linear_sampler = {};  // trilinear clamp (GGX input mips)
  // Character mask (Phase 2)
  reshade::api::resource dyncube_charmask = {};               // RGBA16F cube, 1 mip (character mask)
  reshade::api::resource_view dyncube_charmask_srv = {};      // TextureCube SRV (for debug 9)
  reshade::api::resource_view dyncube_charmask_arr_srv = {};  // Texture2DArray SRV (world-box reduction read)
  reshade::api::resource_view dyncube_charmask_uav = {};      // Texture2DArray UAV (compute write)
  // World-fixed parallax proxy (Sora2nd v1): persistent GPU bounds, no CPU readback.
  // Survives cache round-trips (size-independent); destroyed + re-initialized on recreate.
  reshade::api::resource dyncube_worldbox_bounds = {};            // structured buffer, 2x float4: [0]=(min,valid) [1]=(max,spare)
  reshade::api::resource_view dyncube_worldbox_bounds_srv = {};   // buffer SRV (t33 lighting read)
  reshade::api::resource_view dyncube_worldbox_bounds_uav = {};   // buffer UAV (reduction merge write)
  reshade::api::resource dyncube_faceextents = {};                // structured buffer, 2x float4: [0]=(+X,+Y,+Z,mask) [1]=(-X,-Y,-Z,spare)
  reshade::api::resource_view dyncube_faceextents_uav = {};       // buffer UAV (reduction per-face extent write)
  reshade::api::resource dyncube_faceExtStaging = {};             // 32B gpu_to_cpu staging copy of extents
  reshade::api::resource dyncube_worldbox_scratch = {};           // structured buffer, groups*2 float4 partials (sized per cube size)
  uint64_t dyncube_worldbox_scratch_groups = 0u;                  // pass-0 group capacity of the live scratch buffer
  reshade::api::resource_view dyncube_worldbox_scratch_srv = {};  // buffer SRV (pass-1 read)
  reshade::api::resource_view dyncube_worldbox_scratch_uav = {};  // buffer UAV (pass-0 write)
  reshade::api::pipeline_layout dyncube_worldbox_layout = {};
  reshade::api::pipeline dyncube_worldbox_pipeline = {};
  uint32_t dyncube_worldbox_layout_version = 0u;  // recreate layout/tables/pipeline when shape changes
  GTVBAODescriptorTableSet dyncube_worldbox_tables = {};
  bool dyncube_worldbox_reset_pending = true;
  // Delayed-validate commit (loading protection): consumers follow readSet, which
  // advances only to validated captures. Staging holds the latest bounds copy.
  reshade::api::resource dyncube_validStaging = {};  // 32B gpu_to_cpu staging copy of bounds
  uint32_t dyncube_readSet = 0;                      // validated consumer set (t29/previews/aliases/filter input)
  uint32_t dyncube_filteredReadSet = 99u;            // readSet last fed into GGX filter (99 = none yet)
  bool dyncube_boxCopyPending = false;               // staged validity copy enqueued, not yet consumed
  uint64_t dyncube_valid_event = 0u;                 // native D3D11 event query ended after the staging copies (0 = unavailable)
  bool dyncube_valid_event_armed = false;            // the latest staging copies were followed by an End on the immediate context
  uint32_t dyncube_valid_wait_ticks = 0u;            // consecutive immediate ticks the armed copy was not complete yet
  uint64_t dyncube_valid_poll_misses = 0u;           // scheduler ticks where the staged copy was not complete yet
  uint64_t dyncube_valid_timeouts = 0u;              // armed copies that hit DYNCUBE_VALIDITY_MAX_WAIT_TICKS (blocking map fallback)
  float dyncube_map_wait_ms_max = 0.f;               // longest validity-staging map wait since the last scheduler log line (CPU ms)
  // Dynamic Cubemaps > GPU Timing Log: one timer per step, read without waiting.
  falcom_world::bvh::GpuTimer dyncube_gpu_capture_timer = {};
  falcom_world::bvh::GpuTimer dyncube_gpu_filter_timer = {};
  falcom_world::bvh::GpuTimer dyncube_gpu_variant_timer = {};
  falcom_world::bvh::GpuTimer dyncube_gpu_ssr_timer = {};
  falcom_world::bvh::GpuTimer dyncube_gpu_box_timer = {};        // world-box merge (split out of the capture timer)
  std::chrono::steady_clock::time_point present_last_time = {};  // previous present, for the frame-interval log
  std::chrono::steady_clock::time_point present_window_t0 = {};  // first interval of the current log window
  bool present_last_valid = false;                               // false until the first interval after the toggle is turned on
  double present_sum_ms = 0.0;                                   // frame intervals in the current log window
  float present_max_ms = 0.f;                                    // longest frame interval in the current log window
  uint32_t present_count = 0u;                                   // frame intervals in the current log window
  uint32_t present_slow_count = 0u;                              // frames longer than 1.5x the previous window's average
  double present_prev_avg_ms = 0.0;                              // average of the previous log window (0 = none yet)
  bool dyncube_wasRejected = false;                              // edge latch for reject/resume logging
  bool dyncube_rejectedGap = false;                              // a capture was rejected since the last dispatched capture (one-shot: first accepted capture hard-replaces history)
  bool dyncube_loadingWipeDone = false;                          // loading wipe already applied for the current stale episode (edge latch)
  bool dyncube_loadingWipePending = false;                       // OnPresent saw a loading-stale episode: wipe on the next scheduler tick
  bool dyncube_captureDirty = false;                             // capture-content settings changed since last filter: force one filter pass
  bool dyncube_dirtyFastForward = false;                         // settings-dirty one-shot: next dispatched capture hard-replaces history
  float dyncube_lastVariantSoften = -1.f;                        // soften value baked into the variant (-1 = none yet)
  float dyncube_lastVariantStrength = -1.f;                      // strength value baked into the variant (-1 = none yet)
  float dyncube_lastCharCapture = -1.f;                          // character-capture value fed into the last filter pass
  float dyncube_lastSparkleRejection = -1.f;
  bool dyncube_hasValidRead = false;       // any validated readSet exists (gates first filter)
  uint64_t dyncube_rejected_captures = 0;  // rejected (unpromoted) capture count
  // Phase 3 GGX prefilter � double-buffered filtered cube (Active/Building) so a
  // partially-written cube is never exposed to lighting.
  uint32_t dyncube_mip_count = 8;              // computed mips (8 for 128..4096)
  reshade::api::resource dyncube_ggx_in = {};  // RGBA16F cube, N mips (GGX input chain)
  reshade::api::resource_view dyncube_ggx_in_cube_srv = {};
  reshade::api::resource dyncube_ggx_out[2] = {};  // RGBA16F cubes, N mips (filtered output)
  reshade::api::resource_view dyncube_ggx_out_cube_srv[2] = {};
  reshade::api::resource_view dyncube_ggx_out_mip_uav[2][8] = {};  // per-mip array UAVs (mips 1..N-1)
  uint32_t dyncube_ggx_active = 0;                                 // index of the completed cube bound to t17
  bool dyncube_ggx_valid = false;                                  // a completed filter has been produced
  // Multi-frame update scheduler (Capture -> Filter -> Done -> wait -> Capture).
  enum class DynCubePhase : uint32_t { Done = 0,
                                       Capture = 1,
                                       Filter = 2,
                                       Variant = 3 };
  DynCubePhase dyncube_phase = DynCubePhase::Done;
  uint64_t dyncube_next_update_frame = 0;     // next frame a capture may begin
  uint64_t dyncube_sched_frame = UINT64_MAX;  // last present frame the scheduler ran
  // Actual GPU-work counters (prove capture/filter frequency; reported in the throttled log).
  uint64_t dyncube_capture_dispatches = 0;  // RunDynCubeCapture dispatched
  uint64_t dyncube_filter_updates = 0;      // RunDynCubeFilter completed
  uint64_t dyncube_ggx_mip_dispatches = 0;  // GGX per-mip dispatches issued
  uint64_t dyncube_face_copies = 0;         // 6-face mip0 copies issued
  reshade::api::pipeline_layout dyncube_ggx_layout = {};
  reshade::api::pipeline dyncube_ggx_pipeline = {};
  GTVBAODescriptorTableSet dyncube_ggx_tables = {};
  // Global-push variant cube (soften + strength for non-lighting t17 consumers).
  // Derived from ggx_out[active]; rebuilt on demand, never part of per-size cache.
  reshade::api::resource dyncube_variant = {};                  // RGBA16F cube, N mips
  reshade::api::resource_view dyncube_variant_cube_srv = {};    // full-chain SRV (global serve)
  reshade::api::resource_view dyncube_variant_mip_uav[8] = {};  // per-mip array UAVs (variant write, mip0..N-1)
  reshade::api::pipeline_layout dyncube_variant_layout = {};
  reshade::api::pipeline dyncube_variant_pipeline = {};
  GTVBAODescriptorTableSet dyncube_variant_tables = {};
  bool dyncube_variant_valid = false;  // variant matches current ggx_out + settings
  // Dedicated solid-color debug cube (Phase 0A). Never aliases history/ggx resources.
  reshade::api::resource dyncube_solid_cube = {};           // RGBA16F cube, 1 mip
  reshade::api::resource_view dyncube_solid_cube_srv = {};  // TextureCube SRV (t17 for debug 3)
  reshade::api::resource_view dyncube_solid_cube_uav = {};  // Texture2DArray UAV (solid write)
  // Simple SSR (screen-space ray march) � independent of history/ggx/inferred.
  reshade::api::resource dyncube_ssr_raw = {};  // RGBA16F 2D, march output (rgb=color, a=confidence)
  reshade::api::resource_view dyncube_ssr_raw_srv = {};
  reshade::api::resource_view dyncube_ssr_raw_uav = {};
  reshade::api::resource dyncube_ssr_blur_h = {};  // RGBA16F 2D, horizontal blur intermediate
  reshade::api::resource_view dyncube_ssr_blur_h_srv = {};
  reshade::api::resource_view dyncube_ssr_blur_h_uav = {};
  reshade::api::resource dyncube_ssr_blur = {};  // RGBA16F 2D, final blurred result (t31)
  reshade::api::resource_view dyncube_ssr_blur_srv = {};
  reshade::api::resource_view dyncube_ssr_blur_uav = {};
  reshade::api::resource dyncube_ssr_hitmask = {};  // R32_UINT 2D, per-8x8-tile SSR hit mask (atomics require R32)
  reshade::api::resource_view dyncube_ssr_hitmask_srv = {};
  reshade::api::resource_view dyncube_ssr_hitmask_uav = {};
  reshade::api::pipeline_layout dyncube_ssr_layout = {};
  reshade::api::pipeline dyncube_ssr_pipeline = {};
  uint32_t dyncube_ssr_layout_version = 0u;  // recreate layout/tables/pipeline when shape changes
  GTVBAODescriptorTableSet dyncube_ssr_tables = {};
  reshade::api::pipeline_layout dyncube_ssr_blur_layout = {};
  reshade::api::pipeline dyncube_ssr_blur_pipeline = {};
  uint32_t dyncube_ssr_blur_layout_version = 0u;  // recreate layout/tables/pipeline when shape changes
  GTVBAODescriptorTableSet dyncube_ssr_blur_tables = {};
  uint32_t dyncube_pending_size = 0;      // requested cube size, recreated at frame boundary
  bool dyncube_pending_recreate = false;  // recreate (old set release deferred to Present)
  bool dyncube_pending_destroy = false;   // feature disabled -> free the set at Present
  // -- RCAS post-TAA sharpening (Stage 1; motion sharpening is Stage 2) --
  // Flow per frame when enabled: TAA draw completes -> copy unsharpened TAA
  // output to rcas_hist (this copy alone is ever bound as next frame's t2) ->
  // RCAS compute reads TAA output, writes rcas_temp -> copy rcas_temp back
  // over the TAA output so downstream reads see sharpened pixels. History
  // isolation is structural: sharpened data never enters rcas_hist or t2.
  reshade::api::resource rcas_temp_texture = {};                  // owned sharpened target (dims of TAA output, linear/UAV-legal variant format)
  reshade::api::resource_view rcas_temp_srv = {};                 // (kept for debugging; not bound downstream)
  reshade::api::resource_view rcas_temp_uav = {};                 // compute write target (u0)
  reshade::api::resource rcas_hist_texture = {};                  // owned UNSHARPENED copy; sole source of overridden t2
  reshade::api::resource_view rcas_hist_srv = {};                 // bound as t2 when RCAS serves history; also the dispatch input (t0)
  uint32_t rcas_w = 0u, rcas_h = 0u;                              // live dims of temp/hist set
  reshade::api::format rcas_fmt = reshade::api::format::unknown;  // live format (preserved, never converted)
  bool rcas_have_copy = false;                                    // true once an unsharpened copy exists for t2 override
  bool rcas_format_logged = false;                                // one-time format log per resource set
  bool rcas_dispatch_logged = false;                              // one-time dispatch-shape log per resource set
  reshade::api::pipeline_layout rcas_layout = {};
  reshade::api::pipeline rcas_pipeline = {};
  uint32_t rcas_layout_version = 0u;
  std::array<reshade::api::descriptor_table, 2> rcas_tables = {};  // [0]=srv t0, [1]=uav u0
  reshade::api::resource_view rcas_last_rtv0 = {};                 // latest bound RTV0 (D3D11 immediate-list assumption; sampled at TAA on_draw)
  reshade::api::resource_view rcas_motion_srv = {};                // game motion buffer (t3) captured from TAA draws (Stage 2)
  uint64_t rcas_motion_res = 0u;
  std::atomic<bool> rcas_motion_live{true};  // destroy-event driven; false = buffer freed since capture
  // Motion vectors read from the GBuffer's fourth render target instead of the
  // TAA's t3. Same buffer, different bind point: the writers are
  // staticfoliage_0xF1EC53A8.ps_5_0.hlsl:201-207 and its 13 siblings, all
  // `o4.xy = jitterDiff_g.xy + (cur - prev) * vpSize_g`, i.e. pixel displacement
  // plus the jitter delta. That is exactly the convention MBGameMotionToUV and the
  // resolve pass already assume, so the two sources are interchangeable downstream
  // and this needs no shader change. The difference is availability, not data:
  // RTV4 is written by the GBuffer pass every frame, whereas t3 only exists once the
  // TAA draw has run.
  reshade::api::resource_view mb_rtv4_srv = {};  // GBuffer RTV4, captured from bind_render_targets
  std::atomic<uint64_t> mb_rtv4_res{0u};
  std::atomic<bool> mb_rtv4_live{true};
  bool mb_rtv4_warned_fmt = false;   // one-shot format sanity warning
  bool mb_logged_rtv4_dims = false;  // one-shot RTV4 dimensions log
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
  std::atomic<bool> mb_rtv4_owned_dirty{false};
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
  reshade::api::resource fxaa_temp_texture = {};                  // owned FXAA output (dims of TAA output, linear/UAV-legal variant format)
  reshade::api::resource_view fxaa_temp_srv = {};                 // RCAS t0 when FXAA serves (or publish source when RCAS skips)
  reshade::api::resource_view fxaa_temp_uav = {};                 // FXAA main write target (u0)
  reshade::api::resource fxaa_luma_texture = {};                  // owned perceptual luma (R8_UNORM)
  reshade::api::resource_view fxaa_luma_srv = {};                 // FXAA main luma input (t1)
  reshade::api::resource_view fxaa_luma_uav = {};                 // luma prepass write target (u0)
  reshade::api::resource_view fxaa_hist_srv = {};                 // linear-variant view of rcas_hist_texture (no sRGB decode); falls back to rcas_hist_srv
  bool fxaa_hist_srv_owned = false;                               // false when fxaa_hist_srv aliases rcas_hist_srv (do not destroy)
  reshade::api::resource fxaa_hist_seen = {};                     // hist texture handle the views above were built from (recreate on change)
  uint32_t fxaa_w = 0u, fxaa_h = 0u;                              // live dims of the FXAA set
  reshade::api::format fxaa_fmt = reshade::api::format::unknown;  // live format
  bool fxaa_logged = false;                                       // one-time log per resource set
  reshade::api::pipeline_layout fxaa_layout = {};
  reshade::api::pipeline fxaa_luma_pipeline = {};
  reshade::api::pipeline fxaa_standard_pipeline = {};
  reshade::api::pipeline fxaa_high_pipeline = {};
  uint32_t fxaa_layout_version = 0u;
  std::array<reshade::api::descriptor_table, 2> fxaa_tables = {};  // [0]=srv t0+t1, [1]=uav u0
  // -- Motion Blur (Guertin 2013), Sora 2nd --
  // Three owned resources: TileMax, NeighborMax, and the gather output. There is no
  // velocity or linear-depth surface � both are read straight from the game's
  // textures and converted in the consuming shader.
  reshade::api::resource mb_tilemax_texture = {};
  reshade::api::resource_view mb_tilemax_srv = {};
  reshade::api::resource_view mb_tilemax_uav = {};
  reshade::api::resource mb_neighbormax_texture = {};
  reshade::api::resource_view mb_neighbormax_srv = {};
  reshade::api::resource_view mb_neighbormax_uav = {};
  reshade::api::resource mb_output_texture = {};  // gather result
  reshade::api::resource_view mb_output_srv = {};
  reshade::api::resource_view mb_output_uav = {};
  // Half-resolution chain, allocated ONLY while MotionBlurHalfRes is on so the
  // default path costs nothing. Both are ceil(W/2) x ceil(H/2).
  reshade::api::resource mb_half_color_texture = {};  // downsampled gather input
  reshade::api::resource_view mb_half_color_srv = {};
  reshade::api::resource_view mb_half_color_uav = {};
  reshade::api::resource mb_half_result_texture = {};  // half-res gather output
  reshade::api::resource_view mb_half_result_srv = {};
  reshade::api::resource_view mb_half_result_uav = {};
  // 1x1x1 stand-in bound at the gather's t4 when IS-FAST is off or missing, so
  // that slot is never a null descriptor.
  reshade::api::resource mb_noise_fallback_res = {};
  // Camera/object split, at the MOTION texture's resolution. .xy is the blended
  // velocity the tile chain reduces; .zw is camera-only for the velocity views,
  // so it only exists on the wide surface. The shipping format is RG16F -- the
  // filter reads only .xy -- and it is RGBA16F while Debug View is 8 or 9. See
  // CreateMotionBlurSet for the capability fallback.
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
  uint32_t mb_motion_w = 0u, mb_motion_h = 0u;  // motion_w and motion_h size the
                                                // camera/object resolve output,
                                                // which is full motion res
  uint32_t mb_tiles_x = 0u, mb_tiles_y = 0u;
  uint32_t mb_radius_px = 0u;
  // Last framerate-normalisation factor written to the log, so only real changes
  // are reported. -1 forces the first frame to log.
  float mb_last_frame_scale = -1.f;
  // Per-channel weights resolved once by the deploy gate, which is where the
  // cutscene signal lives, and pushed by PrepareMotionBlur.
  bool mb_halfres = false;  // half-res surfaces currently allocated
  reshade::api::format mb_output_fmt = reshade::api::format::unknown;
  // The resolve surface's active width: true when it is RG16F (the shipping
  // path), false when RGBA16F (either a camera-term debug view is selected or
  // the device refused the narrow UAV). RunMotionBlur picks the matching
  // resolve pipeline from this, so it must always describe the allocated set.
  bool mb_resolve_narrow = false;
  // Latched when the device refuses the RG16F resolve UAV. Without it the
  // recreate test below would see "requested narrow, active wide" every frame
  // and rebuild the whole set in a loop.
  bool mb_resolve_narrow_failed = false;
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

  // -- Contact Shadows --
  // One rgba8_unorm target per technique, sized to the depth buffer. RGBA8_UNORM
  // is in the D3D11.0 guaranteed UAV type-write set; R8_UNORM and R16_FLOAT are
  // not, and a rejected UAV write leaves the target holding whatever was there
  // before, which reads as a plausible image rather than as a failure. The spare
  // channels carry the per-pixel diagnostic that makes the pass debuggable; see
  // the layout block in shadows/shadows_common.hlsli.
  reshade::api::resource contact_shadow_texture = {};
  reshade::api::resource_view contact_shadow_srv = {};
  reshade::api::resource_view contact_shadow_uav = {};
  // Ping-pong target of the sun contact denoise: the horizontal pass writes here and the
  // vertical pass writes back into contact_shadow_texture, so t34 is unchanged.
  reshade::api::resource contact_denoise_texture = {};
  reshade::api::resource_view contact_denoise_srv = {};
  reshade::api::resource_view contact_denoise_uav = {};
  // Local contact target (shadows/contact_local.cs_5_0.hlsl): .x point share, .y diagnostic, .z spot share, .w written.
  // Bound at t40 for the main lighting draws.
  reshade::api::resource contact_local_texture = {};
  reshade::api::resource_view contact_local_srv = {};
  reshade::api::resource_view contact_local_uav = {};
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
  // Frame the local pass dispatched. Only the main lighting hook runs it: it is the term's only consumer, and on Kai the
  // character hook runs before the frame's lamp lists are certain to be captured.
  uint64_t shadows_local_ran_frame = UINT64_MAX;
  // Frame of the last "blocked: <why>" line. The lighting draw fires several
  // times per frame, so without this the reason is reported once per draw.
  uint64_t shadows_log_frame = UINT64_MAX;
  bool shadows_logged_first_dispatch = false;
  // Which hook dispatched, reported once. Kai and Daybreak 2 run a character
  // lighting pass before the main lighting pass, and the answer decides whether
  // the char pass is reading a same-frame result or last frame's.
  int shadows_ran_from = -1;  // 0 = char lighting, 1 = main lighting
  bool shadows_logged_isfast_missing = false;
  bool shadows_logged_first_local_dispatch = false;
  bool shadows_logged_local_inputs_missing = false;  // cleared by the next successful local dispatch
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
  bool taa_other_addon_active = false;             // another payload was seen for a TAA hash
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

// The newest denoised VBGI output is usable when the last denoise is at most max_age frames old.
static bool VbgiGiFresh(const DeviceData* d, uint64_t max_age) {
  const auto& out = d->vbgi_denoise.output;
  return d->vbgi_gi_frame != UINT64_MAX
         && d->frame_index - d->vbgi_gi_frame <= max_age
         && out.srv[d->vbgi_denoise.output_index].handle != 0u
         && out.width == d->captured_depth_w && out.height == d->captured_depth_h;
}

static void CreateGTVBAOResources(reshade::api::device* device, DeviceData* data,
                                  uint32_t gw, uint32_t gh);
static void DestroyGTVBAOResources(reshade::api::device* device, DeviceData* data);
static bool CreateComputePipelinesIfNeeded(reshade::api::device* device, DeviceData* data);
static bool RunGTVBAO(reshade::api::command_list* cmd_list, DeviceData* data);
static bool RunVBGI(reshade::api::command_list* cmd_list, DeviceData* data, bool prefilter_done);
static bool RunVbgiRadiance(reshade::api::command_list* cmd_list, DeviceData* data);
static bool LoadISFASTNoiseTexture(reshade::api::device* dev, DeviceData* d);
// -- Contact Shadows -- forward decls
static void CreateShadowsResources(reshade::api::device* dev, DeviceData* d,
                                   uint32_t w, uint32_t h);
static void DestroyShadowsResources(reshade::api::device* dev, DeviceData* d);
static bool CreateShadowsPipelinesIfNeeded(reshade::api::device* dev, DeviceData* d);
static bool RunShadows(reshade::api::command_list* cl, DeviceData* d, int fromHook);
enum class ComputeDispatchFixMode : uint32_t {
  OFF,
  RESTORE_STATE,
  NULL_AND_RESTORE,
};
static void ApplyGTVBAOCSDispatchFix(
    reshade::api::command_list* cmd_list,
    renodx::utils::state::CommandListState* cs,
    renodx::utils::state::CommandListState& prev);
static void ApplyComputeDispatchFix(
    reshade::api::command_list* cmd_list,
    renodx::utils::state::CommandListState* cs,
    renodx::utils::state::CommandListState& prev,
    ComputeDispatchFixMode fix,
    uint32_t srv_count,
    uint32_t uav_count,
    uint32_t buffer_uav_count = 0u);
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
static void ConsumeDynCubeStagedValidity(reshade::api::device* dev, DeviceData* d, bool on_immediate);
static bool RunDynCubeInference(reshade::api::command_list* cl, DeviceData* d);
static bool RunDynCubeFilter(reshade::api::command_list* cl, DeviceData* d, bool ggxOn);
static bool RunDynCubeVariant(reshade::api::command_list* cl, DeviceData* d);
static bool RunDynCubeSSR(reshade::api::command_list* cl, DeviceData* d);
// GPU Timing Log: time a step only on the immediate context. The timer reads its
// queries there, and GetData is not valid on a deferred context (photo mode).
static bool DynCubeGpuTimingOn(reshade::api::command_list* cl, const DeviceData* d) {
  return g_dyncube_gpu_timing > 0.5f && cl != nullptr && cl == d->immediate_cmd_list;
}
// GPU Timing Log: start a new frame-interval window (the previous average is kept by the caller's choice).
static void ResetPresentWindow(DeviceData* d) {
  d->present_sum_ms = 0.0;
  d->present_max_ms = 0.f;
  d->present_count = 0u;
  d->present_slow_count = 0u;
}

// VBGI gather: see RunVBGI
static bool OnBeforeLightingShaderDraw(reshade::api::command_list* cmd_list);
// Defined with RunRtgiInline below; the CustomShader table needs it first.
static void OnDrawnLightingShader(reshade::api::command_list* cmd_list);
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
    renodx::utils::pipeline::PipelineInfo* info,
    uint32_t subobject_count,
    const reshade::api::pipeline_subobject* subobjects);
static void OnDestroyPipelineCapture(reshade::api::device* device, reshade::api::pipeline pipeline);
// Forces the draw-time machinery to rebuild the replacement on the next draw
// for every tracked pipeline whose hashes contain `hash` and for which we
// captured init-time subobjects. The rebuild (stock BuildReplacementPipeline)
// clones details->subobjects, which shared infra only stores when
// use_replace_async/use_shader_cache is set (neither addon sets them), so the
// per-handle clones captured below are assigned first; without them the
// rebuild would index an empty array (heap corruption � the startup crash).
// Two-phase (collect handles first, then reset per handle) so no pipeline
// map lock is held across operations. The orphaned replacement pipeline is
// deliberately NOT destroyed here (it may still be bound on this list) and
// leaks bounded (~1 per toggle transition) until device/process teardown.
static void ResetTAAReplacementPipelines(reshade::api::device* dev, uint32_t hash) {
  if (!dev || hash == 0u) return;
  auto* d = dev->get_private_data<DeviceData>();
  if (!d) return;
  std::vector<uint64_t> stale;
  for (const auto& entry : d->taa_subobject_clones) {
    const uint64_t handle = entry.first;
    renodx::utils::pipeline::GetPipelineInfo(
        reshade::api::pipeline{handle},
        [&](const renodx::utils::pipeline::PipelineInfo& info) {
          if (info.device != dev) return;
          for (const auto& shader : info.shader_details) {
            if (shader.shader_hash == hash) {
              stale.push_back(handle);
              break;
            }
          }
        });
  }
  for (uint64_t handle : stale) {
    auto cit = d->taa_subobject_clones.find(handle);
    if (cit == d->taa_subobject_clones.end()) continue;  // no safe source: leave stale (today's behavior)
    reshade::api::pipeline orphan = {0u};
    renodx::utils::pipeline::UpdatePipelineShaderDetails(
        reshade::api::pipeline{handle},
        [&](renodx::utils::pipeline::PipelineShaderDetails& details) {
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
    // The orphaned replacement is deliberately not destroyed (it may still be
    // bound); its PipelineInfo entry is cleaned up with the device.
    if (orphan.handle != 0u) {
      // Drop the clone if the orphan was captured at init.
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
// the vanilla TAA hashes; the pipeline tracker's original hashes cover
// pipelines already baked by another addon (load-order robustness).
static void OnInitPipelineCapture(
    renodx::utils::pipeline::PipelineInfo* info,
    uint32_t subobject_count,
    const reshade::api::pipeline_subobject* subobjects) {
  if (info == nullptr || info->device == nullptr || !subobjects || subobject_count == 0u || info->pipeline.handle == 0u) return;
  auto* d = info->device->get_private_data<DeviceData>();
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
      if (h == want) {
        match = true;
        break;
      }
    }
  }
  if (!match) {
    for (const auto& shader : info->shader_details) {
      for (uint32_t want : kTAAHashes) {
        if (shader.shader_hash == want) {
          match = true;
          break;
        }
      }
      if (match) break;
    }
  }
  if (!match) return;
  auto it = d->taa_subobject_clones.find(info->pipeline.handle);
  if (it != d->taa_subobject_clones.end()) {
    renodx::utils::pipeline::DestroyPipelineSubobjects(it->second.first, it->second.second);
    d->taa_subobject_clones.erase(it);
    d->taa_subobjects_owned.erase(info->pipeline.handle);
  }
  reshade::api::pipeline_subobject* clone =
      renodx::utils::pipeline::ClonePipelineSubObjects(subobjects, subobject_count);
  if (!clone) return;
  d->taa_subobject_clones.emplace(info->pipeline.handle, std::make_pair(clone, subobject_count));
  // If the slot payload is already owned, this pipeline's creation-time build
  // may have used stale bytes: schedule one reset on the next TAA draw.
  if (d->taa_slot_claimed) {
    d->taa_pending_resets.insert(info->pipeline.handle);
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
struct CapturedViewInfo {
  uint64_t res;
  std::string dims;
  uint32_t w;
  uint32_t h;
};
static CapturedViewInfo CSResolveCapture(reshade::api::device* dev, reshade::api::resource_view v);

// -- IS-FAST sync helpers (sync g_isfast_* globals ? shader_injection) --
static void SyncISFASTToShaderInjection(reshade::api::command_list* cmd_list) {
  shader_injection.rt_sun_cascades = static_cast<float>(falcom_world::g_state.rt_sun_cascades.load(std::memory_order_relaxed));
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
                                              ? d->isfast_noise_srv
                                              : d->fallback_srv;
        if (srv.handle) {
          cmd_list->push_descriptors(
              reshade::api::shader_stage::pixel,
              reshade::api::pipeline_layout{0},
              0,
              reshade::api::descriptor_table_update{
                  {},
                  3u,
                  0,
                  1,
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
          ? 1.f
          : 0.f;
}

static bool OnBeforeVolFogDraw(reshade::api::command_list* cmd_list) {
  SyncVolFogISFASTToShaderInjection(cmd_list);
  // Push IS-FAST noise texture at t3 (same pattern as shadow shader at t3)
  if (g_isfast_enabled > 0.5f) {
    if (auto* dev = cmd_list->get_device()) {
      if (auto* d = dev->get_private_data<DeviceData>()) {
        reshade::api::resource_view srv = d->isfast_noise_srv.handle
                                              ? d->isfast_noise_srv
                                              : d->fallback_srv;
        if (srv.handle) {
          cmd_list->push_descriptors(
              reshade::api::shader_stage::pixel,
              reshade::api::pipeline_layout{0}, 0,
              reshade::api::descriptor_table_update{
                  {},
                  3u,
                  0,
                  1,
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
                                              ? d->isfast_noise_srv
                                              : d->fallback_srv;
        if (srv.handle) {
          cmd_list->push_descriptors(
              reshade::api::shader_stage::pixel,
              reshade::api::pipeline_layout{0}, 0,
              reshade::api::descriptor_table_update{
                  {},
                  15u,
                  0,
                  1,
                  reshade::api::descriptor_type::texture_shader_resource_view,
                  &srv,
              });
        }
      }
    }
  }
  return true;
}

// -- Inline shadow-march inputs (t35 noise, t36 depth) --
//
// The local-light march runs inside the dynamic light loops rather than in a pass, so
// it needs the two resources a pass would have supplied: the depth buffer and the
// noise volume. They go to slots the lighting shaders declare for the shadow path
// alone (t35/t36), never to the game's own registers.
//
// DeployShadows calls this when either Sun or Local contact shadows is on. t36 feeds
// both the sun range gate (FalcomApplyContactToSun) and the local march, so it is
// pushed for both; t35 and the noise frame are needed only by the local march and sit
// behind a second gate on the Local toggle.
//
// t36 gets a null view when the depth capture is missing or dead. KillTrackedInput
// only clears the live flag and keeps the handle, so pushing that handle would bind
// a freed view, and skipping the push would leave an earlier frame's depth under the
// march. The sun gate never reads the null view: DeployShadows sets
// cs_contact_dedicated_bound only while captured_depth_live is set, because
// RunShadows' once-a-frame early return does not recheck the capture.
//
// t35 needs no fallback: FalcomSampleISFAST does not sample while
// shadow_isfast_texture_loaded is 0, and the jitter is unused while Local is off.
static void pushInlineShadowInputs(reshade::api::command_list* cmd_list, DeviceData* d) {
  const reshade::api::resource_view depth_srv = (d->captured_depth_live.load()
                                                     ? d->captured_depth_srv
                                                     : reshade::api::resource_view{});
  cmd_list->push_descriptors(
      reshade::api::shader_stage::pixel, reshade::api::pipeline_layout{0}, 0,
      reshade::api::descriptor_table_update{
          {}, kLightingShadowDepthRegister, 0, 1, reshade::api::descriptor_type::texture_shader_resource_view, &depth_srv});
  if (shader_injection.cs_contact_local_enabled < 0.5f) return;
  if (shader_injection.cs_contact_isfast_enabled > 0.5f) {
    LoadISFASTNoiseTexture(cmd_list->get_device(), d);
  }
  const bool isfast_ready = d->isfast_texture_loaded && d->isfast_noise_srv.handle != 0u;
  shader_injection.cs_noise_frame = (isfast_ready
                                         ? static_cast<float>(d->frame_index % 32u)
                                         : -1.f);
  if (isfast_ready) {
    cmd_list->push_descriptors(
        reshade::api::shader_stage::pixel, reshade::api::pipeline_layout{0}, 0,
        reshade::api::descriptor_table_update{
            {}, kLightingShadowNoiseRegister, 0, 1, reshade::api::descriptor_type::texture_shader_resource_view, &d->isfast_noise_srv});
  }
}

// -- Contact Shadows: shared deploy step --
// The contact term is consumed by BOTH a character lighting pass and a main
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
  shader_injection.cs_contact_dedicated_bound = 0.f;
  shader_injection.cs_contact_local_bound = 0.f;
  if (!cmd_list) return;
  auto* dev = cmd_list->get_device();
  auto* d = dev ? dev->get_private_data<DeviceData>() : nullptr;
  if (!d) return;

  const bool contact_on = shader_injection.cs_contact_enabled > 0.5f;
  if (!contact_on && shader_injection.cs_contact_local_enabled < 0.5f) return;
  pushInlineShadowInputs(cmd_list, d);
  const bool local_compute = fromHook == 1 && LocalContactComputeRunning();
  if (!contact_on && !local_compute) return;

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

  if (contact_on && d->contact_shadow_srv.handle && d->captured_depth_live.load()) {
    cmd_list->push_descriptors(
        reshade::api::shader_stage::pixel, reshade::api::pipeline_layout{0}, 0,
        reshade::api::descriptor_table_update{
            {}, kLightingContactShadowRegister, 0, 1, reshade::api::descriptor_type::texture_shader_resource_view, &d->contact_shadow_srv});
    shader_injection.cs_contact_dedicated_bound = 1.f;
  }
  if (local_compute && d->shadows_local_ran_frame == d->frame_index && d->contact_local_srv.handle
      && d->captured_depth_live.load()) {
    cmd_list->push_descriptors(
        reshade::api::shader_stage::pixel, reshade::api::pipeline_layout{0}, 0,
        reshade::api::descriptor_table_update{
            {}, kLightingLocalContactRegister, 0, 1, reshade::api::descriptor_type::texture_shader_resource_view, &d->contact_local_srv});
    shader_injection.cs_contact_local_bound = 1.f;
  }
}

// -- Character/SSAO passes: bind the shadow-path IS-FAST volume --
//
// Shared by the Kai character pass and both Sora SSAO passes because all three run
// the custom camera-facing contact march and all three dither it with the same
// volume, so they share one body and one register.
//
// cs_noise_frame is written here as well as in RunShadows and pushInlineShadowInputs, and that is the point:
// these passes do not call RunShadows and can be the first consumer of the frame, so
// without this the jitter frame would stay at whatever the last pass left -- which
// is -1 whenever Contact Shadows is off, and -1 means slice 0 forever. That is not
// noise, it is one fixed pattern being re-blended each frame.
//
// The t15 push this replaces was already dead: no character shader in this mod
// declared t15, and the Kai one declares t35 for the shadow path. Removing it
// rather than leaving it means there is exactly one binding for this resource.
static void charShadowsPushISFAST(reshade::api::command_list* cmd_list) {
  // Null-checked before the sync helper, which dereferences cmd_list itself. Testing
  // it after the call would be a guard that cannot fire.
  if (!cmd_list) return;
  SyncISFASTToShaderInjection(cmd_list);
  auto* dev = cmd_list->get_device();
  auto* d = dev ? dev->get_private_data<DeviceData>() : nullptr;
  if (!d) return;
  if (g_isfast_enabled > 0.5f) LoadISFASTNoiseTexture(dev, d);
  const bool ready = d->isfast_texture_loaded && d->isfast_noise_srv.handle != 0u;
  shader_injection.cs_noise_frame = ready
                                        ? static_cast<float>(d->frame_index % 32u)
                                        : -1.f;
  if (!ready) return;
  cmd_list->push_descriptors(
      reshade::api::shader_stage::pixel, reshade::api::pipeline_layout{0}, 0,
      reshade::api::descriptor_table_update{
          {}, kCharShadowNoiseRegister, 0, 1, reshade::api::descriptor_type::texture_shader_resource_view, &d->isfast_noise_srv});
}

// -- Kai + Daybreak 2 character lighting callback (Env SSS + Character Shadowing) --
static bool OnBeforeCharLightingDraw(reshade::api::command_list* cmd_list) {
  // The character pass can be the first consumer of the frame, so it may be the
  // one that has to run the shadow compute passes. On games without a character
  // pass this is a no-op and the main lighting draw does the work.
  //
  // The IS-FAST push goes first so cs_noise_frame is set even when DeployShadows
  // returns early (Contact Shadows off), which is the configuration this pass
  // mostly runs in.
  charShadowsPushISFAST(cmd_list);
  DeployShadows(cmd_list, 0);
  // Character shader reads shader_injection_data automatically via b13 injection.
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
                                                ? (float)(d->frame_index % 64u)
                                                : -1.f;
  if (noiseUsable) {
    reshade::api::resource_view srv = d->isfast_noise_srv;
    cmd_list->push_descriptors(
        reshade::api::shader_stage::pixel,
        reshade::api::pipeline_layout{0}, 0,
        reshade::api::descriptor_table_update{
            {}, 5u, 0, 1, reshade::api::descriptor_type::texture_shader_resource_view, &srv});
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
            .on_drawn = OnDrawnLightingShader,
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
    // -- Sora 1st foliage (new hashes) --
    {
        0x8C297700u,
        renodx::mods::shader::CustomShader{
            .crc32 = 0x8C297700u,
            .code = __0x8C297700,
            .on_draw = OnBeforeFoliageDraw,
        },
    },
    {
        0x545AFAA5u,
        renodx::mods::shader::CustomShader{
            .crc32 = 0x545AFAA5u,
            .code = __0x545AFAA5,
            .on_draw = OnBeforeFoliageDraw,
        },
    },
    {
        0x26233414u,
        renodx::mods::shader::CustomShader{
            .crc32 = 0x26233414u,
            .code = __0x26233414,
            .on_draw = OnBeforeFoliageDraw,
        },
    },
    {
        0x6076C5A8u,
        renodx::mods::shader::CustomShader{
            .crc32 = 0x6076C5A8u,
            .code = __0x6076C5A8,
            .on_draw = OnBeforeFoliageDraw,
        },
    },
    // -- Sora 1st foliage (legacy hashes) --
    {
        0x68C07DEAu,
        renodx::mods::shader::CustomShader{
            .crc32 = 0x68C07DEAu,
            .code = __0x68C07DEA,
            .on_draw = OnBeforeFoliageDraw,
        },
    },
    {
        0xBA6CE3DAu,
        renodx::mods::shader::CustomShader{
            .crc32 = 0xBA6CE3DAu,
            .code = __0xBA6CE3DA,
            .on_draw = OnBeforeFoliageDraw,
        },
    },
    {
        0x4942F14Cu,
        renodx::mods::shader::CustomShader{
            .crc32 = 0x4942F14Cu,
            .code = __0x4942F14C,
            .on_draw = OnBeforeFoliageDraw,
        },
    },
    {
        0xAAAD1F02u,
        renodx::mods::shader::CustomShader{
            .crc32 = 0xAAAD1F02u,
            .code = __0xAAAD1F02,
            .on_draw = OnBeforeFoliageDraw,
        },
    },
    {
        0x9645D00Fu,
        renodx::mods::shader::CustomShader{
            .crc32 = 0x9645D00Fu,
            .code = __0x9645D00F,
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
    // -- Sora 2nd foliage (new hashes) --
    {
        0x5F527E52u,
        renodx::mods::shader::CustomShader{
            .crc32 = 0x5F527E52u,
            .code = __0x5F527E52,
            .on_draw = OnBeforeFoliageDraw,
        },
    },
    {
        0x73DC9D7Eu,
        renodx::mods::shader::CustomShader{
            .crc32 = 0x73DC9D7Eu,
            .code = __0x73DC9D7E,
            .on_draw = OnBeforeFoliageDraw,
        },
    },
    {
        0xA2E80908u,
        renodx::mods::shader::CustomShader{
            .crc32 = 0xA2E80908u,
            .code = __0xA2E80908,
            .on_draw = OnBeforeFoliageDraw,
        },
    },
    {
        0x882FADE1u,
        renodx::mods::shader::CustomShader{
            .crc32 = 0x882FADE1u,
            .code = __0x882FADE1,
            .on_draw = OnBeforeFoliageDraw,
        },
    },
    {
        0xAA835FE0u,
        renodx::mods::shader::CustomShader{
            .crc32 = 0xAA835FE0u,
            .code = __0xAA835FE0,
            .on_draw = OnBeforeFoliageDraw,
        },
    },
    {
        0x2DADE2B8u,
        renodx::mods::shader::CustomShader{
            .crc32 = 0x2DADE2B8u,
            .code = __0x2DADE2B8,
            .on_draw = OnBeforeFoliageDraw,
        },
    },
    {
        0x20792A02u,
        renodx::mods::shader::CustomShader{
            .crc32 = 0x20792A02u,
            .code = __0x20792A02,
            .on_draw = OnBeforeFoliageDraw,
        },
    },
    {
        0xFE7B3FC9u,
        renodx::mods::shader::CustomShader{
            .crc32 = 0xFE7B3FC9u,
            .code = __0xFE7B3FC9,
            .on_draw = OnBeforeFoliageDraw,
        },
    },
    {
        0xED7AB349u,
        renodx::mods::shader::CustomShader{
            .crc32 = 0xED7AB349u,
            .code = __0xED7AB349,
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
    // Kyoto lighting (GTVBAO + VBGI)
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
        .key = "ISFASTMasterEnable",
        .binding = &g_isfast_enabled,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "IS-FAST Noise",
        .section = "IS-FAST",
        .tooltip = "Master toggle for IS-FAST spatio-temporal blue noise. Uses the baked-in fast_noise_ea.dds (no file needed next to the game .exe).",
        .labels = {"Off", "On"},
    },
    new renodx::utils::settings::Setting{
        .key = "ISFASTStrength",
        .binding = &g_isfast_strength,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 1.f,
        .label = "Noise Strength",
        .section = "IS-FAST",
        .tooltip = "0 = deterministic (banding), 1 = full noise.",
        .min = 0.0f,
        .max = 1.0f,
        .format = "%.2f",
        .is_enabled = []() { return g_isfast_enabled > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "ISFASTDebugLogging",
        .binding = &g_isfast_debug_logging,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f,
        .label = "Debug Logging",
        .section = "IS-FAST",
        .tooltip = "Log IS-FAST status: whether the baked-in noise texture loaded and which noise source is active.",
        .labels = {"Off", "On"},
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "ISFASTSpatialScale",
        .binding = &g_isfast_spatial_scale,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 1.f,
        .label = "Spatial Scale",
        .section = "IS-FAST",
        .tooltip = "Scale noise spatial frequency. <1 zooms in (smoother), >1 adds more detail.",
        .min = 0.25f,
        .max = 4.0f,
        .format = "%.2f",
        .is_enabled = []() { return g_isfast_enabled > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "ISFASTTemporalSpeed",
        .binding = &g_isfast_temporal_speed,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 1.f,
        .label = "Temporal Speed",
        .section = "IS-FAST",
        .tooltip = "Scale noise animation speed. 0 = frozen, 1 = default, 5 = fast flicker.",
        .min = 0.0f,
        .max = 5.0f,
        .format = "%.2f",
        .is_enabled = []() { return g_isfast_enabled > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "ISFASTSeedOffset",
        .binding = &g_isfast_seed_offset,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f,
        .label = "Seed Offset",
        .section = "IS-FAST",
        .tooltip = "Offset the noise seed pattern (0-64). Shift to find optimal noise distribution.",
        .labels = {"0", "4", "8", "12", "16", "20", "24", "28", "32", "36", "40", "44", "48", "52", "56", "60"},
        .is_enabled = []() { return g_isfast_enabled > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },

    // ----------- Kai / Daybreak 2 - Specific Sections -----------

    // -- SSGI (Falcom) --
    new renodx::utils::settings::Setting{
        .key = "KaiSSGIEnable",
        .binding = &shader_injection.ssgi_mod_enabled,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Enable",
        .section = "SSGI (Falcom)",
        .labels = {"Off", "On"},
        .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "KaiSSGIColorBoost",
        .binding = &shader_injection.ssgi_color_boost,
        .default_value = 1.f,
        .label = "Color Boost",
        .section = "SSGI (Falcom)",
        .tooltip = "Scales SSGI RGB contribution before power shaping.",
        .min = 0.f,
        .max = 3.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.ssgi_mod_enabled >= 0.5f; },
        .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "KaiSSGIAlphaBoost",
        .binding = &shader_injection.ssgi_alpha_boost,
        .default_value = 1.f,
        .label = "Alpha Boost",
        .section = "SSGI (Falcom)",
        .tooltip = "Scales SSGI alpha before saturate.",
        .min = 0.f,
        .max = 3.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.ssgi_mod_enabled >= 0.5f; },
        .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "KaiSSGIPower",
        .binding = &shader_injection.ssgi_pow,
        .default_value = 1.f,
        .label = "Power",
        .section = "SSGI (Falcom)",
        .tooltip = "Applies pow(abs(color), Power) to shape bounce response.",
        .min = 0.1f,
        .max = 3.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.ssgi_mod_enabled >= 0.5f; },
        .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },

    // -- Depth of Field --
    new renodx::utils::settings::Setting{
        .key = "DOFMode",
        .binding = &shader_injection.dof_mode,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 1.f,
        .label = "Mode",
        .section = "Depth of Field",
        .tooltip = "Vanilla keeps the original blur shader. Improved uses DOF method 3 (gather).",
        .labels = {"Vanilla", "Improved"},
    },
    new renodx::utils::settings::Setting{
        .key = "DOFStrength",
        .binding = &shader_injection.dof_strength,
        .default_value = 1.f,
        .label = "Strength",
        .section = "Depth of Field",
        .tooltip = "Overall blend strength for improved DOF output.",
        .min = 0.f,
        .max = 2.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.dof_mode >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DOFRadiusScale",
        .binding = &shader_injection.dof_radius_scale,
        .default_value = 1.33f,
        .label = "Radius Scale",
        .section = "Depth of Field",
        .tooltip = "Scales blur radius derived from game CoC.",
        .min = 0.25f,
        .max = 2.5f,
        .format = "%.2fx",
    },
    new renodx::utils::settings::Setting{
        // Deliberately a different key from the old raw "DOFSampleCount" slider.
        // That setting stored the tap count itself, and this one stores a 0-3
        // quality INDEX, so the two cannot share a key: a saved preset holding 12,
        // 24 or 64 would be clamped by LoadSetting into the valid index range and
        // every one of them would silently resolve to "Ultra". Renaming orphans the
        // stale value instead, so the tiers start clean at their default.
        .key = "DOFQuality",
        .binding = &shader_injection.dof_sample_count,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 2.f,
        .label = "Quality",
        .section = "Depth of Field",
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
        return taps[index < 0 ? 0 : (index > 3 ? 3 : index)]; },
    },
    new renodx::utils::settings::Setting{
        .key = "DOFAdaptiveSamples",
        .binding = &shader_injection.dof_adaptive_samples,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Adaptive Samples",
        .section = "Depth of Field",
        .tooltip = "Scales each pixel's tap count to its own blur radius, so lightly "
                   "blurred pixels cost far less. Quality becomes the maximum. "
                   "Off = every blurred pixel uses the full Quality tap count, as before.",
        .labels = {"Off", "On"},
        .is_enabled = []() { return shader_injection.dof_mode >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DOFTapCountView",
        .binding = &shader_injection.dof_debug_view,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f,
        .label = "Tap Count View",
        .section = "Depth of Field",
        .tooltip = "Replaces the image with a per-pixel map of the adaptive ladder: one "
                   "hue per tap-count rung, and BLACK where the pixel early-outs and "
                   "costs nothing. Read the area fractions to find the real average tap "
                   "count, which is what the ladder thresholds should be tuned against.",
        .labels = {"Off", "On"},
        .is_enabled = []() { return shader_injection.dof_mode >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DOFNearScale",
        .binding = &shader_injection.dof_near_scale,
        .default_value = 1.f,
        .label = "Near Scale",
        .section = "Depth of Field",
        .tooltip = "Scales near-field CoC response.",
        .min = 0.f,
        .max = 2.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.dof_mode >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DOFFarScale",
        .binding = &shader_injection.dof_far_scale,
        .default_value = 1.35f,
        .label = "Far Scale",
        .section = "Depth of Field",
        .tooltip = "Scales far-field CoC response.",
        .min = 0.f,
        .max = 2.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.dof_mode >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DOFCoCCurve",
        .binding = &shader_injection.dof_coc_curve,
        .default_value = 1.f,
        .label = "CoC Curve",
        .section = "Depth of Field",
        .tooltip = "Applies pow(CoC, Curve) before blur; >1 tightens focus transition.",
        .min = 0.25f,
        .max = 4.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.dof_mode >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DOFEdgeThreshold",
        .binding = &shader_injection.dof_edge_threshold,
        .default_value = 0.3f,
        .label = "Edge Threshold",
        .section = "Depth of Field",
        .tooltip = "Rejects CoC-mismatched taps to reduce foreground/background bleeding.",
        .min = 0.02f,
        .max = 1.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.dof_mode >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DOFISFAST",
        .binding = &shader_injection.dof_isfast_enabled,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "IS-FAST Rotation",
        .section = "Depth of Field",
        .tooltip = "Rotates the bokeh gather per pixel with IS-FAST noise. The golden-angle spiral is otherwise identical at every pixel, so undersampling shows up as fixed spiral/ring ghosting; rotating decorrelates neighbouring pixels and turns it into fine noise. Needs the IS-FAST master toggle.",
        .labels = {"Off", "On"},
        .is_enabled = []() { return shader_injection.dof_mode >= 0.5f && g_isfast_enabled > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DOFSignSoftness",
        .binding = &shader_injection.dof_sign_softness,
        .default_value = 1.f,
        .label = "Layer Softness",
        .section = "Depth of Field",
        .tooltip = "Fixes sharp character lines in near blur. Acceptance of taps from the opposite depth layer: 0 = hard reject (vanilla behavior, can leave thin features unblurred), higher = softer separation (more bleed).",
        .min = 0.f,
        .max = 1.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.dof_mode >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DOFCoverageFix",
        .binding = &shader_injection.dof_coverage_enabled,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f,
        .label = "Coverage Composite",
        .section = "Depth of Field",
        .tooltip = "Fixes sharp character lines in near blur. Blends same-layer bokeh toward the full-disc average weighted by how much of the blur disc the feature actually covers.",
        .labels = {"Off", "On"},
        .is_enabled = []() { return shader_injection.dof_mode >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },

    // -- Motion Blur (Guertin et al. 2013) --.
    new renodx::utils::settings::Setting{
        // Key kept from the pre-split Mode so existing configs keep working.
        .key = "MotionBlurMode",
        .binding = &shader_injection.mb_mode,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 1.f,
        .label = "Mode",
        .section = "Motion Blur",
        .tooltip = "When the blur runs: never, only inside cutscenes, or always.",
        .labels = {"Off", "Cutscene Only", "Always On"},
        .is_visible = []() { return IsMotionBlurGame(); },
    },
    new renodx::utils::settings::Setting{
        .key = "MotionBlurCSDispatchFix",
        .binding = &g_mb_cs_dispatch_fix,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 3.f,
        .label = "CS Dispatch Fix",
        .section = "Motion Blur",
        .tooltip = "Controls compute state cleanup after the motion blur chain. "
                   "Fix 1 restores the previous compute pipeline and descriptor tables. "
                   "Fix 2 and Fix 3 clear the compute sampler, SRV and UAV slots, "
                   "then restore the previous compute pipeline and descriptor tables.",
        .labels = {"Off", "Fix 1: Restore State", "Fix 2: Null Compute", "Fix 3: Null + Restore"},
        .is_visible = []() { return IsMotionBlurGame() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "MotionBlurCameraDirection",
        .binding = &shader_injection.mb_camera_sign,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 1.f,
        .label = "Camera Direction",
        .section = "Motion Blur",
        .tooltip = "Convention probe, not a look control. The game writes motion as one signed difference of the previous and current clip positions, but the shader varyings holding them are unlabelled, so which way round it is can only be settled from the image. On a static scene with the camera moving, set Debug View to Object Residual: the correct value here renders it BLACK, because object motion is then zero by definition. A lit residual that mirrors Camera Velocity means this is backwards. Leave it alone once you have picked the right one.",
        .labels = {"Forward (cur - prev)", "Reversed (prev - cur)"},
        .is_visible = []() { return IsMotionBlurGame() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "MotionBlurCameraCut",
        .binding = &shader_injection.mb_camera_cut,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f,
        .label = "Reject Camera Cuts",
        .section = "Motion Blur",
        .tooltip = "Detects a teleport or a cutscene transition, where the view transform is replaced in a single frame, and writes zero velocity for that frame. Without it the whole screen smears toward the warp point for one frame. The test is on the camera transform, not the motion buffer, so a fast camera whip and a moving object cannot trigger it on their own. Toggle this off to A/B against a cut you can see; a false positive costs one sharp frame, a false negative costs the smear.",
        .is_enabled = []() { return MotionBlurActive(true); },
        .is_visible = []() { return IsMotionBlurGame() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "MotionBlurLogging",
        .binding = &g_mb_logging,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f,
        .label = "Logging",
        .section = "Motion Blur",
        .tooltip = "Writes motion blur diagnostics to the ReShade log: the frame rate scale, which chain stage is running, a one-time report of the colour, motion and depth buffer sizes, and any reason the effect could not run. Turn it off to keep the log quiet. The effect itself is not affected either way.",
        .is_enabled = []() { return MotionBlurActive(true); },
        .is_visible = []() { return IsMotionBlurGame() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CapturePerfLog",
        .binding = &g_capture_perf_log,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f,
        .label = "Capture Hook Timing",
        .section = "Motion Blur",
        .tooltip = "Logs the CPU time of the push-descriptor capture hook once per second (DevKit not loaded only). No rendering change.",
        .is_visible = []() { return IsAdvancedSettingsMode() && IsMotionBlurGame() && !falcom_world::g_state.supported; },
    },
    new renodx::utils::settings::Setting{
        .key = "MotionBlurCameraCutThreshold",
        .binding = &shader_injection.mb_camera_cut_px,
        .default_value = 360.f,
        .label = "Camera Cut Threshold",
        .section = "Motion Blur",
        .tooltip = "How far the view transform may move in one frame before it counts as a cut, in 1080-reference pixels, measured at a nominal depth. A fast whip at 60 fps is roughly 30-80 px; a scene cut is hundreds. Raise it if ordinary camera movement is being rejected, lower it if a real cut is smearing.",
        .min = 8.f,
        .max = 512.f,
        .format = "%.0f px",
        .is_enabled = []() { return MotionBlurActive(true) && shader_injection.mb_camera_cut > 0.5f; },
        .is_visible = []() { return IsMotionBlurGame() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "MotionBlurCameraJitter",
        .binding = &shader_injection.mb_camera_jitter,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 1.f,
        .label = "Camera Jitter",
        .section = "Motion Blur",
        .tooltip = "Convention probe, not a look control. The game adds a per-frame jitter delta to every motion vector, and prevViewProj_g may already contain that jitter. Adding it again inflates the camera term, and because object motion is computed as game minus camera, the inflation lands in the object channel at full size. Judge it the same way as Camera Direction: with the camera moving on a static scene, Object Residual must be BLACK.",
        .labels = {"Add jitter delta", "Omit jitter delta"},
        .is_visible = []() { return IsMotionBlurGame() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "MotionBlurIntensity",
        .binding = &shader_injection.mb_intensity,
        .default_value = 0.50f,
        .label = "Intensity",
        .section = "Motion Blur",
        .tooltip = "How far the blur smears, as a fraction of how far things actually moved: 0 is off, higher is softer.",
        .min = 0.f,
        .max = 2.f,
        .format = "%.2fx",
        .is_enabled = []() { return MotionBlurActive(true); },
        .is_visible = []() { return IsMotionBlurGame(); },
    },
    new renodx::utils::settings::Setting{
        .key = "MotionBlurMotionVectorInput",
        .binding = &g_mb_motion_input,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f,
        .label = "Motion Vector Input",
        .section = "Motion Blur",
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
        .key = "MotionBlurQuality",
        .binding = &g_mb_quality,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 2.f,
        .label = "Quality",
        .section = "Motion Blur",
        .tooltip = "How many times a pixel gets sampled while blurring, which keeps fast movement smooth instead of steppy, at a performance cost.",
        .labels = {"Low", "Medium", "High", "Ultra"},
        .is_enabled = []() { return MotionBlurActive(true); },
        .is_visible = []() { return IsMotionBlurGame(); },
    },
    new renodx::utils::settings::Setting{
        .key = "MotionBlurMaxRadius",
        .binding = &shader_injection.mb_max_radius_px,
        .default_value = 40.f,
        .label = "Max Radius",
        .section = "Motion Blur",
        .tooltip = "The longest streak the blur can produce.",
        .min = 8.f,
        .max = 80.f,
        .format = "%.0f px",
        .is_enabled = []() { return MotionBlurActive(true); },
        .is_visible = []() { return IsMotionBlurGame(); },
    },
    new renodx::utils::settings::Setting{
        .key = "MotionBlurCenterWeight",
        .binding = &shader_injection.mb_center_weight_k,
        .default_value = 1.f,
        .label = "Center Weight",
        .section = "Motion Blur",
        .tooltip = "Centre-sample weight divisor (paper k). The unblurred centre contributes N/(k*|v|) of the total, so LOWER keeps more of the original pixel and higher lets the streak dominate. Raise it if thin objects ghost, lower it if fast motion smears too little.",
        .min = 1.f,
        .max = 100.f,
        .format = "%.0f",
        .is_enabled = []() { return MotionBlurActive(true); },
        .is_visible = []() { return IsMotionBlurGame() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "MotionBlurJitter",
        .binding = &shader_injection.mb_jitter_h,
        .default_value = 0.95f,
        .label = "Jitter",
        .section = "Motion Blur",
        .tooltip = "How far past the streak length the integration domain reaches (paper h). Breaks up banding; too high softens the streak.",
        .min = 0.f,
        .max = 4.f,
        .format = "%.2f",
        .is_enabled = []() { return MotionBlurActive(true); },
        .is_visible = []() { return IsMotionBlurGame() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "MotionBlurMinVelocity",
        .binding = &shader_injection.mb_min_velocity_g,
        .default_value = 1.5f,
        .label = "Velocity Threshold",
        .section = "Motion Blur",
        .tooltip = "Below this a pixel's own velocity is replaced by the direction perpendicular to the tile velocity (paper g).",
        .min = 0.f,
        .max = 8.f,
        .format = "%.2f",
        .is_enabled = []() { return MotionBlurActive(true); },
        .is_visible = []() { return IsMotionBlurGame() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "MotionBlurNeighborFalloff",
        .binding = &shader_injection.mb_neighbor_t,
        .default_value = 1.f,
        .label = "NB Max Falloff",
        .section = "Motion Blur",
        .tooltip = "Slope of the stochastic tile lookup near tile borders (paper t), in tiles. Trades tile-edge banding for noise.",
        .min = 0.f,
        .max = 4.f,
        .format = "%.2f",
        .is_enabled = []() { return MotionBlurActive(true); },
        .is_visible = []() { return IsMotionBlurGame() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "MotionBlurDepthTolerance",
        .binding = &shader_injection.mb_depth_tolerance,
        .default_value = 0.1f,
        .label = "Depth Tolerance",
        .section = "Motion Blur",
        .tooltip = "Width of the soft depth transition that separates foreground from background samples. Higher lets more background bleed across depth edges.",
        .min = 0.01f,
        .max = 1.f,
        .format = "%.2f",
        .is_enabled = []() { return MotionBlurActive(true); },
        .is_visible = []() { return IsMotionBlurGame() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "MotionBlurJitterSource",
        .binding = &shader_injection.mb_jitter_source,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 1.f,
        .label = "Jitter Source",
        .section = "Motion Blur",
        .tooltip = "Halton is the paper's deterministic per-pixel sequence. IS-FAST uses the blue-noise volume already loaded for DoF and shadows; it animates, so residual sampling noise reads much less. Falls back to Halton when the volume is unavailable.",
        .labels = {"Halton", "IS-FAST"},
        .is_enabled = []() { return MotionBlurActive(true); },
        .is_visible = []() { return IsMotionBlurGame() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "MotionBlurFrameRateReference",
        .binding = &shader_injection.mb_frame_rate_reference,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f,
        .label = "Frame Rate Reference",
        .section = "Motion Blur",
        .tooltip = "How the streak is tied to framerate. 0 (default) matches Unreal's r.MotionBlurTargetFPS 0: the measured frame time is tracked with a 0.1 moving average and the shutter spans the REAL frame, so a lower framerate gives a proportionally longer streak, and a change of rate ramps over about ten frames instead of stepping. Set a framerate above 0 to rescale the motion into that rate's units instead, which makes the result identical at every framerate at the cost of no longer being physically scaled.",
        .min = 0.f,
        .max = 240.f,
        .format = "%d",
        .is_enabled = []() { return MotionBlurActive(true); },
        .is_visible = []() { return IsMotionBlurGame() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "MotionBlurHalfResThreshold",
        .binding = &shader_injection.mb_halfres_px,
        .default_value = 10.f,
        .label = "Half Res Threshold",
        .section = "Motion Blur",
        .tooltip = "Streak length, in 1080p-equivalent pixels, at which a tile switches to half resolution. Below it the gather runs at full resolution, because a streak that short is dominated by detail that half resolution has already thrown away. Above it the blur dominates and the cheaper path costs little visually. Sits where the sample ladder jumps to its most expensive rungs, so the tiles doing the most work are the ones that get cheaper. Try 4, 6 and 10.",
        .min = 0.f,
        .max = 96.f,
        .format = "%.0f px",
        .is_enabled = []() { return MotionBlurActive(true) && shader_injection.mb_halfres > 0.5f; },
        .is_visible = []() { return IsMotionBlurGame() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "MotionBlurHalfRes",
        .binding = &shader_injection.mb_halfres,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f,
        .label = "Half Resolution",
        .section = "Motion Blur",
        .tooltip = "Splits the gather by motion magnitude: tiles whose streak is short run at full resolution, tiles whose streak is long run at half resolution, and the result is selected per tile rather than blended. A streak too short to see cannot hide the detail that half resolution discards, so this keeps the two aligned. Off by default because the extra full-resolution pass costs about as much as the static gather it replaces. Toggling rebuilds the buffers, costing one frame.",
        .labels = {"Off", "On"},
        .is_enabled = []() { return MotionBlurActive(true); },
        .is_visible = []() { return IsMotionBlurGame() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "MotionBlurDepthTest",
        .binding = &shader_injection.mb_depth_test,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f,
        .label = "Depth Test",
        .section = "Motion Blur",
        .tooltip = "On: every tap reads the game depth and taps that disagree with the centre pixel are down-weighted, which is what separates foreground from background. Off: that fetch and the two cone terms are dropped and only the cylinder term remains, saving one texture load per tap, at the cost of bleeding across depth edges. Note Depth Tolerance already scales the same terms, so a very low value suppresses most of their effect while still paying for the fetch.",
        .labels = {"Off", "On"},
        .is_enabled = []() { return MotionBlurActive(true); },
        .is_visible = []() { return IsMotionBlurGame() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "MotionBlurLocalVelocity",
        .binding = &shader_injection.mb_local_velocity_weights,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 1.f,
        .label = "Local Velocity Weights",
        .section = "Motion Blur",
        .tooltip = "On: each tap reads the motion texture and is weighted by its own direction, which is the paper's feature-aware term and what stops foreground bleeding across depth edges. Off: taps reuse the composite direction, removing ~25 texture loads per pixel (roughly a third of the gather). A/B it on a high-contrast edge.",
        .labels = {"Off", "On"},
        .is_enabled = []() { return MotionBlurActive(true); },
        .is_visible = []() { return IsMotionBlurGame() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "MotionBlurDebugChain",
        .binding = &shader_injection.mb_debug_chain,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f,
        .label = "Debug Chain",
        .section = "Motion Blur",
        .tooltip = "Diagnostic only. Full runs everything. Prep Only runs the three preprocessing dispatches and skips the gather, leaving the image untouched. Gather Only runs just the gather, reusing the PREVIOUS frame's prep, so do not read its image as a correctness comparison, and do not use it to time a moving scene: with prep frozen the tile grid is stale and almost nothing passes the gather's early-out. If preparation has never run it falls back to Full for that frame.",
        .labels = {"Full", "Prep Only", "Gather Only"},
        .is_enabled = []() { return MotionBlurActive(true); },
        .is_visible = []() { return IsMotionBlurGame() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "MotionBlurDebugView",
        .binding = &shader_injection.mb_debug_view,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f,
        .label = "Debug View",
        .section = "Motion Blur",
        .tooltip = "Velocity, NeighborMax, the tile grid, linearized depth, blur amount, sample count, the half resolution split, the reconstructed camera term, or the error in it. Use these to confirm the motion buffer is live and the tile grid is resolution independent. Sample Count shows black where the blur early-outs and one distinct hue per rung of the adaptive ladder, which is what Quality now bounds: this is also the readout for tuning the rungs later. Half Res Detect draws the split partition itself: GREEN = blurred at full resolution, RED = blurred at half resolution, GREY = below the early-out so it will not blur at all, BLUE = Half Resolution is off so everything is full resolution. Boundaries are tile-quantised, so it also reads as the verification that the split no longer changes with framerate. Camera Velocity is the reconstructed camera term on its own. Object Residual is the difference against the game's own motion, and it is the one to settle the Camera Direction and Camera Jitter settings: on a STATIC scene with the camera moving it must be BLACK, because object motion is then zero by definition. A lit residual that mirrors Camera Velocity means the camera estimate is inverted or mis-scaled. It is also a direct readout of what the blur is doing, because the streak length is the camera's share of the total, so anything bright here is exactly the object contribution that was removed.",
        .labels = {"Off", "Velocity", "Neighbor Max", "Tile Grid", "Linear Depth", "Blur Amount", "Sample Count", "Half Res Detect", "Camera Velocity", "Object Residual"},
        .is_enabled = []() { return MotionBlurActive(true); },
        .is_visible = []() { return IsMotionBlurGame() && IsAdvancedSettingsMode(); },
    },

    // -- Character SSGI --
    new renodx::utils::settings::Setting{
        .key = "CharacterSSGICompositeMethod",
        .binding = &g_char_vbgi_composite_method,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Apply Game SSGI",
        .section = "Character SSGI",
        .labels = {"Off", "On"},
        .is_visible = []() { return IsKai(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CharacterSSGICompositeStrength",
        .binding = &shader_injection.char_gi_strength,
        .default_value = 3.0f,
        .label = "Strength",
        .section = "Character SSGI",
        .tooltip = "Overall contribution scale for character GI.",
        .min = 0.f,
        .max = 3.f,
        .format = "%.2f",
        .is_enabled = []() { return g_char_vbgi_composite_method >= 0.5f; },
        .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CharacterSSGICompositeAlphaScale",
        .binding = &shader_injection.char_gi_alpha_scale,
        .default_value = 1.0f,
        .label = "Alpha Scale",
        .section = "Character SSGI",
        .tooltip = "Scales sampled SSGI alpha before blending.",
        .min = 0.f,
        .max = 3.f,
        .format = "%.2f",
        .is_enabled = []() { return g_char_vbgi_composite_method >= 0.5f; },
        .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CharacterSSGICompositeChroma",
        .binding = &shader_injection.char_gi_chroma_strength,
        .default_value = 0.50f,
        .label = "Chroma",
        .section = "Character SSGI",
        .tooltip = "Scales colorful GI component; lower values reduce tinting.",
        .min = 0.f,
        .max = 2.f,
        .format = "%.2f",
        .is_enabled = []() { return g_char_vbgi_composite_method >= 0.5f; },
        .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CharacterSSGICompositeLuma",
        .binding = &shader_injection.char_gi_luma_strength,
        .default_value = 0.0f,
        .label = "Luma",
        .section = "Character SSGI",
        .tooltip = "Scales neutral GI brightness; keep low to avoid white haze.",
        .min = 0.f,
        .max = 1.f,
        .format = "%.3f",
        .is_enabled = []() { return g_char_vbgi_composite_method >= 0.5f; },
        .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CharacterSSGICompositeShadowPower",
        .binding = &shader_injection.char_gi_shadow_power,
        .default_value = 1.25f,
        .label = "Shadow Power",
        .section = "Character SSGI",
        .tooltip = "Higher values concentrate GI toward darker areas.",
        .min = 0.1f,
        .max = 4.f,
        .format = "%.2f",
        .is_enabled = []() { return g_char_vbgi_composite_method >= 0.5f; },
        .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CharacterSSGICompositeDarkBoost",
        .binding = &shader_injection.char_gi_dark_boost,
        .default_value = 0.0f,
        .label = "Dark Boost",
        .section = "Character SSGI",
        .tooltip = "Extra GI multiplier in darker regions (after shadow mask).",
        .min = 0.f,
        .max = 4.f,
        .format = "%.2f",
        .is_enabled = []() { return g_char_vbgi_composite_method >= 0.5f; },
        .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CharacterSSGICompositeBrightBoost",
        .binding = &shader_injection.char_gi_bright_boost,
        .default_value = 3.0f,
        .label = "Bright Boost",
        .section = "Character SSGI",
        .tooltip = "Boosts GI on brighter regions (values above 1.0 increase bright-side contribution).",
        .min = 0.f,
        .max = 3.f,
        .format = "%.2f",
        .is_enabled = []() { return g_char_vbgi_composite_method >= 0.5f; },
        .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CharacterSSGICompositeHeadroomPower",
        .binding = &shader_injection.char_gi_headroom_power,
        .default_value = 1.25f,
        .label = "Headroom Power",
        .section = "Character SSGI",
        .tooltip = "Controls how strongly bright pixels reject additional GI.",
        .min = 0.1f,
        .max = 4.f,
        .format = "%.2f",
        .is_enabled = []() { return g_char_vbgi_composite_method >= 0.5f; },
        .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CharacterSSGICompositeMaxAdd",
        .binding = &shader_injection.char_gi_max_add,
        .default_value = 0.020f,
        .label = "Max Add",
        .section = "Character SSGI",
        .tooltip = "Per-channel cap for added GI to prevent haze/bloomy washout.",
        .min = 0.f,
        .max = 1.f,
        .format = "%.3f",
        .is_enabled = []() { return g_char_vbgi_composite_method >= 0.5f; },
        .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CharacterSSGICompositePeakLumaCap",
        .binding = &shader_injection.char_gi_peak_luma_cap,
        .default_value = 0.0f,
        .label = "Peak Luma Cap",
        .section = "Character SSGI",
        .tooltip = "Caps peak GI brightness on characters after blending weights. Set 0 to disable.",
        .min = 0.f,
        .max = 1.f,
        .format = "%.3f",
        .is_enabled = []() { return g_char_vbgi_composite_method >= 0.5f; },
        .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CharacterSSGICompositeDepthReject",
        .binding = &shader_injection.char_gi_depth_reject,
        .default_value = 2.0f,
        .label = "Depth Reject",
        .section = "Character SSGI",
        .tooltip = "Higher values suppress GI across depth discontinuities and silhouette edges.",
        .min = 0.f,
        .max = 16.f,
        .format = "%.2f",
        .is_enabled = []() { return g_char_vbgi_composite_method >= 0.5f; },
        .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },

    // -- Fog Color Correction --
    new renodx::utils::settings::Setting{
        .key = "FogColorCorrectionMode",
        .binding = &shader_injection.fog_color_correction_enabled,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Mode",
        .section = "Fog Color Correction",
        .labels = {"Vanilla", "Improved"},
        .is_visible = []() { return IsKai(); },
    },
    new renodx::utils::settings::Setting{
        .key = "FogHue",
        .binding = &shader_injection.fog_hue,
        .default_value = 0.f,
        .label = "Fog Hue",
        .section = "Fog Color Correction",
        .min = 0.f,
        .max = 2.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.fog_color_correction_enabled >= 0.5f; },
        .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "FogChrominance",
        .binding = &shader_injection.fog_chrominance,
        .default_value = 0.f,
        .label = "Fog Chroma",
        .section = "Fog Color Correction",
        .min = 0.f,
        .max = 2.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.fog_color_correction_enabled >= 0.5f; },
        .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "FogAvgBrightness",
        .binding = &shader_injection.fog_avg_brightness,
        .default_value = 0.85f,
        .label = "Fog Avg Bright",
        .section = "Fog Color Correction",
        .min = 0.f,
        .max = 2.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.fog_color_correction_enabled >= 0.5f; },
        .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "FogMinBrightness",
        .binding = &shader_injection.fog_min_brightness,
        .default_value = 0.f,
        .label = "Fog Min Bright",
        .section = "Fog Color Correction",
        .min = -0.5f,
        .max = 1.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.fog_color_correction_enabled >= 0.5f; },
        .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "FogMinChroma",
        .binding = &shader_injection.fog_min_chroma_change,
        .default_value = 0.f,
        .label = "Fog Min Chroma",
        .section = "Fog Color Correction",
        .tooltip = "Minimum chroma ratio applied during fog hue/chroma restoration.",
        .min = 0.f,
        .max = 4.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.fog_color_correction_enabled >= 0.5f; },
        .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "FogMaxChroma",
        .binding = &shader_injection.fog_max_chroma_change,
        .default_value = 0.f,
        .label = "Fog Max Chroma",
        .section = "Fog Color Correction",
        .tooltip = "Maximum chroma ratio applied during fog hue/chroma restoration.",
        .min = 0.f,
        .max = 8.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.fog_color_correction_enabled >= 0.5f; },
        .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "FogLightnessStrength",
        .binding = &shader_injection.fog_lightness_strength,
        .default_value = 1.f,
        .label = "Fog Lightness",
        .section = "Fog Color Correction",
        .tooltip = "Scales fog lightness restoration amount.",
        .min = 0.f,
        .max = 2.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.fog_color_correction_enabled >= 0.5f; },
        .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "FogColorCorrectionStrength",
        .binding = &shader_injection.fog_color_correction_strength,
        .default_value = 0.5f,
        .label = "2D Fog Correction Strength",
        .section = "Fog Color Correction",
        .min = 0.f,
        .max = 1.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.fog_color_correction_enabled >= 0.5f; },
        .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "Fog3DCorrectionStrength",
        .binding = &shader_injection.volfog_color_correction_strength,
        .default_value = 0.5f,
        .label = "3D Fog Correction Strength",
        .section = "Fog Color Correction",
        .tooltip = "Controls how strongly fog color correction is applied to volumetric fog. 0 = off, 1 = full.",
        .min = 0.f,
        .max = 1.f,
        .format = "%.2f",
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
        .key = "CharShadowMode",
        .binding = &shader_injection.char_shadow_mode,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 2.f,
        .label = "Mode",
        .section = "Character Shadowing",
        .tooltip = "Off = Disables Screen Space Shadows on characters. Vanilla = Falcom Algorithm. Custom = Customized algorithm.",
        .labels = {"Off", "Vanilla", "Custom"},
        .is_visible = []() { return !IsKyoto() && !IsDaybreak2(); },
    },
    // ── Character Shadowing: custom camera pass ──
    new renodx::utils::settings::Setting{
        .key = "CharCamEnabled",
        .binding = &shader_injection.char_cam_enabled,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Camera Pass",
        .section = "Character Shadowing",
        .tooltip = "A depth march along the engine's own camera-facing character axis, "
                   "replacing the vanilla 10-step version with a tunable sample count, "
                   "coverage-based response and IS-FAST dither. The result is written to "
                   "the same AO channel the vanilla march used, so the game's temporal "
                   "blend resolves the dither with no extra resolve pass.",
        .labels = {"Off", "On"},
        .is_enabled = []() { return shader_injection.char_shadow_mode >= 1.5f; },
        .is_visible = []() { return !IsKyoto() && !IsDaybreak2() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CharCamStrength",
        .binding = &shader_injection.char_cam_strength,
        .default_value = 1.f,
        .label = "Camera Strength",
        .section = "Character Shadowing",
        .tooltip = "Blend strength for the camera-facing character term.",
        .min = 0.f,
        .max = 1.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.char_shadow_mode >= 1.5f
                                    && shader_injection.char_cam_enabled >= 0.5f; },
        .is_visible = []() { return !IsKyoto() && !IsDaybreak2() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CharCamSamples",
        .binding = &shader_injection.char_cam_sample_count,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 8.f,
        .label = "Camera Sample Count",
        .section = "Character Shadowing",
        .tooltip = "March steps for the camera pass. Because the combiner counts "
                   "coverage rather than taking the first hit, this is a pure "
                   "quality/noise control: raising it does not make the shadow wider "
                   "or darker.",
        .min = 1.f,
        .max = 32.f,
        .format = "%d",
        .is_enabled = []() { return shader_injection.char_shadow_mode >= 1.5f
                                    && shader_injection.char_cam_enabled >= 0.5f; },
        .is_visible = []() { return !IsKyoto() && !IsDaybreak2() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CharCamRayLength",
        .binding = &shader_injection.char_cam_ray_length,
        .default_value = 0.02f,
        .label = "Camera Ray Length",
        .section = "Character Shadowing",
        .tooltip = "March length in world units (1 unit is about 1 metre, a head about "
                   "0.18). This is a SILHOUETTE reach, not a contact one: the axis is the "
                   "camera axis, so the ray always heads into the receiver's own surface "
                   "and a screen-space depth test cannot tell that from a real occluder. "
                   "The engine's vanilla march has the same degeneracy and only survives "
                   "it because it is a binary any-hit that the AO channel smooths "
                   "temporally. Expect a softened depth gradient, not a shadow.",
        .min = 0.001f,
        .max = 0.2f,
        .format = "%.4f",
        .is_enabled = []() { return shader_injection.char_shadow_mode >= 1.5f
                                    && shader_injection.char_cam_enabled >= 0.5f; },
        .is_visible = []() { return !IsKyoto() && !IsDaybreak2() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CharCamThickness",
        .binding = &shader_injection.char_cam_thickness,
        .default_value = 0.2f,
        .label = "Camera Surface Thickness",
        .section = "Character Shadowing",
        .tooltip = "How thick an occluder is assumed to be, in world units. KEEP THIS "
                   "COMPARABLE TO RAY LENGTH. A band much larger than the ray puts the "
                   "whole ray inside or outside it, which collapses the coverage "
                   "estimator into a binary any-hit test and makes Sample Count do "
                   "nothing. At the original 0.29 against a 0.02 ray that is exactly what "
                   "happened. It also acts as the validity filter for samples that have "
                   "drifted out of register with the depth they are compared against.",
        .min = 0.0005f,
        .max = 0.2f,
        .format = "%.4f",
        .is_enabled = []() { return shader_injection.char_shadow_mode >= 1.5f
                                    && shader_injection.char_cam_enabled >= 0.5f; },
        .is_visible = []() { return !IsKyoto() && !IsDaybreak2() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CharCamBias",
        .binding = &shader_injection.char_cam_bias,
        .default_value = 0.0001f,
        .label = "Camera Depth Bias",
        .section = "Character Shadowing",
        .tooltip = "Extra penetration required beyond the normal lift. The shader passes "
                   "lift + this to the marcher as its hit threshold, which is what stops "
                   "the receiver reporting itself as its own occluder.",
        .min = 0.f,
        .max = 0.05f,
        .format = "%.4f",
        .is_enabled = []() { return shader_injection.char_shadow_mode >= 1.5f
                                    && shader_injection.char_cam_enabled >= 0.5f; },
        .is_visible = []() { return !IsKyoto() && !IsDaybreak2() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CharCamNormalBias",
        .binding = &shader_injection.char_cam_normal_bias,
        .default_value = 0.0015f,
        .label = "Camera Normal Bias",
        .section = "Character Shadowing",
        .tooltip = "Lift along the surface normal, and simultaneously the floor of the "
                   "marcher's hit threshold (the shader passes lift + Depth Bias). "
                   "Without it the first sample sits on the receiver's own depth and "
                   "reports a self-hit, which reads as a dark band over the whole "
                   "character. Keep it small relative to Ray Length: the original 0.0075 "
                   "was a third of a 0.02 ray, so the march started in free air beside "
                   "the face.",
        .min = 0.f,
        .max = 0.05f,
        .format = "%.4f",
        .is_enabled = []() { return shader_injection.char_shadow_mode >= 1.5f
                                    && shader_injection.char_cam_enabled >= 0.5f; },
        .is_visible = []() { return !IsKyoto() && !IsDaybreak2() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CharCamResponseScale",
        .binding = &shader_injection.char_cam_response_scale,
        .default_value = 2.f,
        .label = "Camera Response Scale",
        .section = "Character Shadowing",
        .tooltip = "How strongly the camera term reads. Higher makes each agreeing "
                   "sample count for more, so this is a strength control once the band "
                   "and the ray are the same size. It defaults to 2 rather than the 4 "
                   "that reproduces the old binary any-hit exactly: at 4 with 8 samples a "
                   "single spurious sample already costs half the available darkening, "
                   "which is what made this read as a hard patch.",
        .min = 0.f,
        .max = 8.f,
        .format = "%.1f",
        .is_enabled = []() { return shader_injection.char_shadow_mode >= 1.5f
                                    && shader_injection.char_cam_enabled >= 0.5f; },
        .is_visible = []() { return !IsKyoto() && !IsDaybreak2() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CharCamMaxDarkening",
        .binding = &shader_injection.char_cam_max_darkening,
        .default_value = 1.0f,
        .label = "Camera Max Darkening",
        .section = "Character Shadowing",
        .tooltip = "Cap on how dark a shadowed character pixel may get. 0 removes the "
                   "effect, 1 permits solid black. Capped below 1 by default because "
                   "this pass multiplies probe-lit ambient and cannot prove the pixel "
                   "was lit, and a face going to solid black reads as a hole.",
        .min = 0.f,
        .max = 1.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.char_shadow_mode >= 1.5f
                                    && shader_injection.char_cam_enabled >= 0.5f; },
        .is_visible = []() { return !IsKyoto() && !IsDaybreak2() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CharCamISFAST",
        .binding = &shader_injection.char_cam_isfast_enabled,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Camera IS-FAST Jitter",
        .section = "Character Shadowing",
        .tooltip = "Dither the step offset with IS-FAST blue noise so a fixed step does "
                   "not band. The camera term is written to the AO channel the game "
                   "already resolves temporally, so the dither is free to leave on. "
                   "There is no IGN fallback by design; without the volume this marches "
                   "at a fixed midpoint and bands visibly.",
        .labels = {"Off", "On"},
        .is_enabled = []() { return shader_injection.char_shadow_mode >= 1.5f
                                    && shader_injection.char_cam_enabled >= 0.5f; },
        .is_visible = []() { return !IsKyoto() && !IsDaybreak2() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CharCamDebug",
        .binding = &shader_injection.char_cam_debug,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f,
        .label = "Camera Debug View",
        .section = "Character Shadowing",
        .tooltip = "Axis Length reports the magnitude of the engine's rayMarchShadowDir_g "
                   "on a log scale. It is on a log scale because nothing in the engine "
                   "settles whether that symbol is a unit direction (Daybreak 2's 39-step "
                   "character march reads as if it is) or a pre-scaled vector (the Sora "
                   "and Kai 10-step march needs it to be), and a linear greyscale cannot "
                   "show both.",
        .labels = {"Off", "Raw Term", "Coverage", "Axis Length"},
        .is_enabled = []() { return shader_injection.char_shadow_mode >= 1.5f
                                    && shader_injection.char_cam_enabled >= 0.5f; },
        .is_visible = []() { return !IsKyoto() && !IsDaybreak2() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CharGTVBAOMode",
        .binding = &shader_injection.char_gtvbao_mode,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 2.f,
        .label = "Allow GTVBAO",
        .section = "GTVBAO",
        .labels = {"Off", "On", "Combined"},
        .is_enabled = []() { return !falcom_world::rtao::RtaoRequested(); },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CharGTVBAOMaskStr",
        .binding = &shader_injection.char_gtvbao_mask_strength,
        .default_value = 75.f,
        .label = "GTVBAO Char Mask",
        .section = "GTVBAO",
        .min = 0.f,
        .max = 100.f,
        .is_enabled = []() { return shader_injection.char_gtvbao_mode > 0.5f && !falcom_world::rtao::RtaoRequested(); },
        .parse = [](float v) { return v * 0.01f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },

    new renodx::utils::settings::Setting{
        .key = "DebugShowEnvSSS",
        .binding = &shader_injection.debug_show_env_sss,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f,
        .label = "Foliage / Char Mask Debug",
        .section = "GTVBAO",
        .tooltip = "Inspects the foliage and character mask the GTVBAO character "
                   "options act on. View 2 now shows the Contact Shadow term; it used to "
                   "show the retired screen-space shadow channel.",
        .labels = {"Off", "Mask", "Contact Term", "SSAO Sample", "Raw mrt0.z"},
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    // �� GTVBAO ��
    new renodx::utils::settings::Setting{
        .key = "GTVBAOMode",
        .binding = &shader_injection.gtvbao_mode,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "GTVBAO mode",
        .section = "GTVBAO",
        .tooltip = "Off = vanilla game AO. On = GTVBAO compute-shader AO.",
        .labels = {"Off (Vanilla AO)", "On (GTVBAO)"},
        .is_enabled = []() { return !falcom_world::rtao::RtaoRequested(); },
    },
    new renodx::utils::settings::Setting{
        .key = "GTVBAOQuality",
        .binding = &shader_injection.gtvbao_quality_level,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 2.f,
        .label = "Quality Level",
        .section = "GTVBAO",
        .tooltip = "Slices x steps per side. Low 2x4, Medium 3x4, High 3x6, Ultra 4x10.",
        .labels = {"Low", "Medium", "High", "Ultra"},
        .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f; },
    },
    new renodx::utils::settings::Setting{
        .key = "GTVBAOResolution",
        .binding = &shader_injection.gtvbao_resolution,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f,
        .label = "GTVBAO Resolution",
        .section = "GTVBAO",
        .tooltip = "Full = native. Half = half size per axis (a quarter of the pixels). Lower resolutions are upsampled with depth and normal aware weights and are softer. The size is relative to the render resolution, so in-game upscaling makes it smaller.",
        .labels = {"Full", "Half"},
        .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f; },
    },
    new renodx::utils::settings::Setting{
        .key = "GTVBAOUpscaleAlgorithm",
        .binding = &g_gtvbao_upscale_algorithm,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 3.f,
        .label = "Reconstruction",
        .section = "GTVBAO Upscaling",
        .tooltip = "Joint Bilateral (Current) preserves the existing 2x2 depth and normal aware filter. Bilinear is smoother but can bleed across edges. Nearest Valid selects one edge-aware tap and can look blocky. Wider Joint Bilateral uses a 4x4 footprint and costs more. All modes pass through at native resolution.",
        .labels = {"Joint Bilateral (Current)", "Bilinear", "Nearest Valid", "Wider Joint Bilateral"},
        .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "GTVBAOUpscalePlaneSigma",
        .binding = &g_gtvbao_upscale_plane_sigma,
        .default_value = 0.02f,
        .label = "Depth Plane Sigma",
        .section = "GTVBAO Upscaling",
        .tooltip = "Current value 0.02. Higher values allow more blending across depth-plane differences; lower values preserve depth edges more strongly and may leave gaps.",
        .min = 0.005f,
        .max = 0.10f,
        .format = "%.3f",
        .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && (g_gtvbao_upscale_algorithm < 0.5f || g_gtvbao_upscale_algorithm > 1.5f); },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "GTVBAOUpscaleNormalPower",
        .binding = &g_gtvbao_upscale_normal_power,
        .default_value = 64.f,
        .label = "Normal Edge Power",
        .section = "GTVBAO Upscaling",
        .tooltip = "Current value 16. Higher values reject taps with different surface normals more strongly; lower values blend across normal edges more.",
        .min = 1.f,
        .max = 64.f,
        .format = "%.0f",
        .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && (g_gtvbao_upscale_algorithm < 0.5f || g_gtvbao_upscale_algorithm > 1.5f); },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "GTVBAOUpscaleBilinearSharpness",
        .binding = &g_gtvbao_upscale_bilinear_sharpness,
        .default_value = 1.f,
        .label = "Bilinear Weight Sharpness",
        .section = "GTVBAO Upscaling",
        .tooltip = "1.0 keeps the current spatial weights. Higher values favor the strongest tap and look sharper; lower values spread weight more evenly. Used by Joint Bilateral, Bilinear and Nearest Valid.",
        .min = 0.5f,
        .max = 4.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && g_gtvbao_upscale_algorithm < 2.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "GTVBAOUpscaleNearestFallback",
        .binding = &g_gtvbao_upscale_nearest_fallback,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Nearest Valid Fallback",
        .section = "GTVBAO Upscaling",
        .tooltip = "When all joint-bilateral taps are rejected by the edge weights, use the valid tap with the strongest spatial weight. Off leaves neutral AO at that pixel.",
        .labels = {"Off", "On"},
        .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && (g_gtvbao_upscale_algorithm < 0.5f || g_gtvbao_upscale_algorithm > 1.5f); },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "GTVBAONoiseType",
        .binding = &shader_injection.gtvbao_noise_type,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 2.f,
        .label = "Noise Type",
        .section = "GTVBAO",
        .tooltip = "IS-FAST = pre-computed blue noise (baked into the addon). "
                   "IGN = Interleaved Gradient Noise. Hilbert = Hilbert curve noise. "
                   "Only applies when IS-FAST master toggle is On; forced to Hilbert when Off.",
        .labels = {"IS-FAST", "IGN", "Hilbert"},
        .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && g_isfast_enabled > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "GTVBAORadius",
        .binding = &shader_injection.gtvbao_radius,
        .default_value = 0.4f,
        .label = "Radius",
        .section = "GTVBAO",
        .min = 0.01f,
        .max = 5.0f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "GTVBAORadiusMultiplier",
        .binding = &shader_injection.gtvbao_radius_multiplier,
        .default_value = 1.f,
        .label = "Radius Multiplier",
        .section = "GTVBAO",
        .min = 0.3f,
        .max = 3.0f,
        .format = "%.3f",
        .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "GTVBAOFinalPower",
        .binding = &shader_injection.gtvbao_final_power,
        .default_value = 0.8f,
        .label = "Final Power",
        .section = "GTVBAO",
        .min = 0.5f,
        .max = 5.0f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "GTVBAOSampleDistribution",
        .binding = &shader_injection.gtvbao_sample_distribution,
        .default_value = 1.5f,
        .label = "Sample Distribution",
        .section = "GTVBAO",
        .min = 1.0f,
        .max = 3.0f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "GTVBAOAutoThickness",
        .binding = &g_gtvbao_auto_thickness,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Auto Thickness",
        .section = "GTVBAO",
        .tooltip = "Estimate occluder thickness from connected depth samples. Base Thickness sets the starting thickness.",
        .labels = {"Off", "On"},
        .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "GTVBAOBitmaskThickness",
        .binding = &shader_injection.gtvbao_bitmask_thickness,
        .default_value = 0.30f,
        .label = "Bitmask Thickness",
        .section = "GTVBAO",
        .tooltip = "Constant thickness used when Auto Thickness is off.",
        .min = 0.01f,
        .max = 2.0f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && g_gtvbao_auto_thickness < 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "GTVBAOBaseThickness",
        .binding = &g_gtvbao_base_thickness,
        .default_value = 0.20f,
        .label = "Base Thickness",
        .section = "GTVBAO",
        .tooltip = "Starting thickness for the Auto Thickness estimate.",
        .min = 0.01f,
        .max = 2.0f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && g_gtvbao_auto_thickness > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "GTVBAOGTVBAOCosineMode",
        .binding = &shader_injection.gtvbao_cosine_mode,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 2.f,
        .label = "Cosine Sampling Mode",
        .section = "GTVBAO",
        .tooltip = "Weight = uniform slices with a per-slice weight (reference mode 1). Project = cosine-lobe projection (mode 2, default). CDF = approximate inverse-CDF sampling (mode 3). Only used with Cosine weighting.",
        .labels = {"Weight", "Project", "CDF"},
        .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && g_gtvbao_hemisphere_weighting < 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "GTVBAOHemisphereWeighting",
        .binding = &g_gtvbao_hemisphere_weighting,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f,
        .label = "Hemisphere Weighting",
        .section = "GTVBAO",
        .tooltip = "Cosine = ground-truth cosine-weighted AO (default). Uniform = uniform-weighted hemisphere.",
        .labels = {"Cosine", "Uniform"},
        .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "GTVBAODepthMIPOffset",
        .binding = &shader_injection.gtvbao_depth_mip_offset,
        .default_value = 3.5f,
        .label = "Depth MIP Offset",
        .section = "GTVBAO",
        .min = 2.0f,
        .max = 6.0f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "GTVBAONormalInputMode",
        .binding = &g_gtvbao_normal_input_mode,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "MRT Normal Input",
        .section = "GTVBAO",
        .tooltip = "Off = depth normals only. On = use game g-buffer normals.",
        .labels = {"Off (Depth)", "On (MRT)"},
        .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "GTVBAONormalInfluence",
        .binding = &g_gtvbao_normal_influence,
        .default_value = 1.f,
        .label = "Normal Influence",
        .section = "GTVBAO",
        .min = 0.f,
        .max = 2.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && g_gtvbao_normal_input_mode > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "GTVBAONormalDepthBlend",
        .binding = &g_gtvbao_normal_depth_blend,
        .default_value = 1.f,
        .label = "Normal Depth Blend",
        .section = "GTVBAO",
        .tooltip = "Base blend weight between the depth-derived normal and the MRT normal. 1.0 (neutral) uses the MRT normal in full; the depth normal is then only a fallback.",
        .min = 0.f,
        .max = 1.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && g_gtvbao_normal_input_mode > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "GTVBAONormalSharpness",
        .binding = &g_gtvbao_normal_sharpness,
        .default_value = 1.f,
        .label = "Normal Sharpness",
        .section = "GTVBAO",
        .tooltip = "Gamma applied to the depth blend. 1.0 (neutral) is a straight passthrough; higher values push the weight toward the depth normal faster.",
        .min = 0.01f,
        .max = 4.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && g_gtvbao_normal_input_mode > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "GTVBAONormalEdgeRejection",
        .binding = &g_gtvbao_normal_edge_rejection,
        .default_value = 0.f,
        .label = "Normal Edge Rejection",
        .section = "GTVBAO",
        .tooltip = "Rejects the MRT normal on depth discontinuities. 0.0 (neutral) applies no rejection; raise it to fall back toward the depth normal at silhouettes.",
        .min = 0.f,
        .max = 2.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && g_gtvbao_normal_input_mode > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "GTVBAONormalZPreservation",
        .binding = &g_gtvbao_normal_z_preservation,
        .default_value = 2.0f,
        .label = "Normal Z Preservation",
        .section = "GTVBAO",
        .tooltip = "Scales the view-space n.z of the MRT normal. 1.0 keeps the encoded normal intact; lower values tilt it. The shader floors this so it can never reach 0 and flatten the normal.",
        .min = 0.f,
        .max = 2.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && g_gtvbao_normal_input_mode > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "GTVBAONormalDetailResponse",
        .binding = &g_gtvbao_normal_detail_response,
        .default_value = 1.f,
        .label = "Normal Detail Response",
        .section = "GTVBAO",
        .tooltip = "Reweights the MRT normal by how far it disagrees with the depth normal. 0.0 (neutral) is an exact no-op; 1.0 leans on the depth normal where they agree and the MRT normal where they diverge.",
        .min = 0.f,
        .max = 1.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && g_gtvbao_normal_input_mode > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "GTVBAONormalMaxDarkening",
        .binding = &g_gtvbao_normal_max_darkening,
        .default_value = 1.f,
        .label = "Normal Max Darkening",
        .section = "GTVBAO",
        .tooltip = "Caps the final MRT normal blend weight in Multiply mode. 1.0 (neutral) applies no cap.",
        .min = 0.f,
        .max = 1.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && g_gtvbao_normal_input_mode > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "GTVBAODebugMode",
        .binding = &g_gtvbao_debug_ui,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f,
        .label = "Debug View",
        .section = "GTVBAO",
        .tooltip = "Off: normal output.\nAO Only: the final AO on the lighting pass.\nRaw AO: this frame's AO before the SVGF denoise.\nTemporal Confidence: SVGF history length, black (new or no surface) to white (32 frames); blue = the reprojection did not run. Purple = sky, orange = no MRT normal (depth normal used), cyan = foliage.\nSlice AO: mean unoccluded slice fraction (flat surface ~1.0).\nBitmask: first slice's horizon mask (R popcount, G first set bit, B last set bit).\nAO Normal: the view-space normal the AO uses.\nKyoto and Daybreak 2 do not support the compute views (Raw AO, Temporal Confidence, Slice AO, Bitmask, AO Normal).",
        .labels = {"Off", "AO Only", "Raw AO", "Temporal Confidence", "Slice AO", "Bitmask", "AO Normal"},
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "GTVBAODebugLogging",
        .binding = &shader_injection.gtvbao_debug_logging,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f,
        .label = "Debug Logging",
        .section = "GTVBAO",
        .labels = {"Off", "On"},
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "GTVBAOFixExperimental",
        .binding = &shader_injection.gtvbao_fix_experimental,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f,
        .label = "Fix Experimental",
        .section = "GTVBAO",
        .tooltip = "Composite-side AO encoding diagnostic (Sora lighting shaders only). 0=Off (baseline).",
        .labels = {"Off", "1:Neutral 1.0", "2:No 0xFF mask", "3:Inverted", "4:All channels"},
        .is_enabled = []() { return !falcom_world::rtao::RtaoRequested(); },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    // -- GTVBAO Foliage --
    new renodx::utils::settings::Setting{
        .key = "GTVBAOExcludeFoliage",
        .binding = &shader_injection.gtvbao_exclude_foliage,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Exclude Foliage",
        .section = "GTVBAO",
        .tooltip = "Skip AO computation on foliage pixels (prevent wind disocclusion noise).",
        .labels = {"Off", "On"},
        .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "GTVBAOFoliageAOValue",
        .binding = &shader_injection.gtvbao_foliage_ao_value,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 0.f,
        .label = "Foliage AO Value",
        .section = "GTVBAO",
        .tooltip = "Blends foliage AO toward fully bright. 1.0 = keep normal GTVBAO AO (like foliage exclusion OFF). 0.0 = no AO on foliage (fully bright).",
        .min = 0.f,
        .max = 1.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && shader_injection.gtvbao_exclude_foliage > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    // -- Foliage Grass AO --
    new renodx::utils::settings::Setting{
        .key = "FoliageGrassAOEnabled",
        .binding = &shader_injection.foliage_grass_ao_enabled,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Grass AO",
        .section = "Foliage Grass AO",
        .tooltip = "Per-blade vertical AO gradient: dark at root, bright at tip. Replaces noisy SSAO/GTAO on foliage with a stable bake. (Ghost of Tsushima �1.5e-ii)",
        .labels = {"Off", "On"},
    },
    new renodx::utils::settings::Setting{
        .key = "FoliageGrassAOBase",
        .binding = &shader_injection.foliage_grass_ao_base,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 0.15f,
        .label = "Grass AO Base",
        .section = "Foliage Grass AO",
        .tooltip = "AO multiplier at the blade root. Lower = darker base (more self-occlusion).",
        .min = 0.f,
        .max = 0.8f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.foliage_grass_ao_enabled > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "FoliageGrassAOTip",
        .binding = &shader_injection.foliage_grass_ao_tip,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 1.2f,
        .label = "Grass AO Tip",
        .section = "Foliage Grass AO",
        .tooltip = "AO multiplier at the blade tip. Values > 1.0 slightly brighten the tip (rim-light effect).",
        .min = 0.5f,
        .max = 1.5f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.foliage_grass_ao_enabled > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "FoliageGrassAOCurve",
        .binding = &shader_injection.foliage_grass_ao_curve,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 0.25f,
        .label = "Grass AO Curve",
        .section = "Foliage Grass AO",
        .tooltip = "Power curve exponent. <1.0 = darkening concentrated at base (recommended). 1.0 = linear. >1.0 = darkening extends further up the blade.",
        .min = 0.1f,
        .max = 2.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.foliage_grass_ao_enabled > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    // -- BRDF Improvement --
    new renodx::utils::settings::Setting{
        .key = "BRDFHammonDiffuse",
        .binding = &shader_injection.brdf_hammon_diffuse_enabled,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Hammon 2017 Diffuse",
        .section = "BRDF Improvement",
        .tooltip = "Applies the Hammon 2017 GGX+Smith multi-scatter energy-conserving diffuse correction to the sun and point/spot/environment lights (GDC 2017).",
        .labels = {"Off", "On"},
    },
    new renodx::utils::settings::Setting{
        .key = "BRDFDiffuseStrength",
        .binding = &shader_injection.brdf_diffuse_strength,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 1.0f,
        .label = "Diffuse Blend",
        .section = "BRDF Improvement",
        .tooltip = "Blend between vanilla Lambert and Hammon diffuse. 0=vanilla, 1=full Hammon, 2=2x boost.",
        .min = 0.f,
        .max = 2.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.brdf_hammon_diffuse_enabled > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "BRDFMultiScatterSpecular",
        .binding = &shader_injection.brdf_multiscatter_specular_enabled,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.0f,
        .label = "Multi-Scatter GGX Specular",
        .section = "BRDF Improvement",
        .tooltip = "Replaces Blinn-Phong specular with GGX D�V�F + Kulla-Conty multi-scatter compensation (SIGGRAPH 2017).",
        .labels = {"Off", "On"},
    },
    new renodx::utils::settings::Setting{
        .key = "BRDFSpecularStrength",
        .binding = &shader_injection.brdf_specular_strength,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 0.5f,
        .label = "Specular Blend",
        .section = "BRDF Improvement",
        .tooltip = "Blend between vanilla Blinn-Phong and GGX+multi-scatter specular. 0=vanilla, 1=full GGX+MS, 2=2x boost.",
        .min = 0.f,
        .max = 2.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.brdf_multiscatter_specular_enabled > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "BRDFSpecularPeakClamp",
        .binding = &shader_injection.brdf_specular_peak_clamp,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 2.0f,
        .label = "Specular Peak Clamp",
        .section = "BRDF Improvement",
        .tooltip = "Soft cap on GGX highlight intensity. Identity below half the value, asymptotes to it. 0 = off.",
        .min = 0.f,
        .max = 16.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.brdf_multiscatter_specular_enabled > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "BRDFRoughnessMin",
        .binding = &shader_injection.brdf_roughness_min,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 0.65f,
        .label = "Roughness Min",
        .section = "BRDF Improvement",
        .tooltip = "Clamp minimum perceptual roughness to prevent GGX singularity. 0.04 is a safe minimum for most materials.",
        .min = 0.f,
        .max = 0.8f,
        .format = "%.2f",
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "BRDFRoughnessMax",
        .binding = &shader_injection.brdf_roughness_max,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 1.0f,
        .label = "Roughness Max",
        .section = "BRDF Improvement",
        .tooltip = "Clamp maximum perceptual roughness. 1.0 = no clamping.",
        .min = 0.2f,
        .max = 1.f,
        .format = "%.2f",
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    // -- GTVBAO dispatch Fix (for double volumetrics) --
    new renodx::utils::settings::Setting{
        .key = "GTVBAOCSDispatchFix",
        .binding = &g_gtvbao_cs_dispatch_fix,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 2.f,
        .label = "CS Dispatch Fix",
        .section = "GTVBAO",
        .tooltip = "Fixes double volumetrics caused by stale compute descriptor bindings. Fix 1: restore state via Apply(). Fix 2: null compute descriptors. Fix 3: null + restore.",
        .labels = {"Off", "Fix 1: Restore State", "Fix 2: Null Compute", "Fix 3: Null + Restore"},
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    // -- VBGI --
    new renodx::utils::settings::Setting{
        .key = "SSGIEnable",
        .binding = &g_vbgi_enabled,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "GTVBGI Enabled",
        .section = "GTVBGI",
        .tooltip = "GTVBGI: ground-truth visibility bitmask indirect diffuse GI (Sora 1st and Sora 2nd). Added to the lit image after the shadow terms. On by default.",
        .labels = {"Off", "On"},
        .is_visible = []() { return VbgiGameSupported(); },
    },
    new renodx::utils::settings::Setting{
        .key = "VBGIQuality",
        .binding = &g_vbgi_quality_level,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 2.f,
        .label = "Quality Level",
        .section = "GTVBGI",
        .tooltip = "Slices x steps per side. Low 2x4, Medium 3x4, High 3x6, Ultra 4x10.",
        .labels = {"Low", "Medium", "High", "Ultra"},
        .is_enabled = []() { return g_vbgi_enabled > 0.5f; },
        .is_visible = []() { return VbgiGameSupported(); },
    },
    new renodx::utils::settings::Setting{
        .key = "VBGIResolution",
        .binding = &g_vbgi_resolution,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 2.f,
        .label = "GTVBGI Resolution",
        .section = "GTVBGI",
        .tooltip = "Trace size per axis relative to the render size: 1/2 = one trace pixel per 2 x 2 block, 1/4 = per 4 x 4, 1/8 = per 8 x 8. Lower is faster and softer; the depth and light sampling covers the same screen area at every size. Default 1/4.",
        .labels = {"Full", "1/2", "1/4", "1/8"},
        .min = 1.f,  // index = log2 divisor; Full (0) is hidden and a saved 0 loads as 1/2
        .is_enabled = []() { return g_vbgi_enabled > 0.5f; },
        .is_visible = []() { return VbgiRowVisible(); },
    },
    new renodx::utils::settings::Setting{
        .key = "VBGIUpscaleAlgorithm",
        .binding = &g_vbgi_upscale_algorithm,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 3.f,
        .label = "Reconstruction",
        .section = "GTVBGI Upscaling",
        .tooltip = "Joint Bilateral (Current) preserves the existing 2x2 depth and normal aware filter. Bilinear is smoother but can bleed across edges. Nearest Valid selects one edge-aware tap and can look blocky. Wider Joint Bilateral uses a 4x4 footprint and costs more. All modes pass through at native resolution.",
        .labels = {"Joint Bilateral (Current)", "Bilinear", "Nearest Valid", "Wider Joint Bilateral"},
        .is_enabled = []() { return g_vbgi_enabled > 0.5f; },
        .is_visible = []() { return VbgiRowVisible(); },
    },
    new renodx::utils::settings::Setting{
        .key = "VBGIUpscalePlaneSigma",
        .binding = &g_vbgi_upscale_plane_sigma,
        .default_value = 0.02f,
        .label = "Depth Plane Sigma",
        .section = "GTVBGI Upscaling",
        .tooltip = "Current value 0.02. Higher values allow more blending across depth-plane differences; lower values preserve depth edges more strongly and may leave gaps.",
        .min = 0.005f,
        .max = 0.10f,
        .format = "%.3f",
        .is_enabled = []() { return g_vbgi_enabled > 0.5f && (g_vbgi_upscale_algorithm < 0.5f || g_vbgi_upscale_algorithm > 1.5f); },
        .is_visible = []() { return VbgiRowVisible(); },
    },
    new renodx::utils::settings::Setting{
        .key = "VBGIUpscaleNormalPower",
        .binding = &g_vbgi_upscale_normal_power,
        .default_value = 1.f,
        .label = "Normal Edge Power",
        .section = "GTVBGI Upscaling",
        .tooltip = "Default 1. Higher values reject taps with different surface normals more strongly; lower values blend across normal edges more.",
        .min = 1.f,
        .max = 64.f,
        .format = "%.0f",
        .is_enabled = []() { return g_vbgi_enabled > 0.5f && (g_vbgi_upscale_algorithm < 0.5f || g_vbgi_upscale_algorithm > 1.5f); },
        .is_visible = []() { return VbgiRowVisible(); },
    },
    new renodx::utils::settings::Setting{
        .key = "VBGIUpscaleBilinearSharpness",
        .binding = &g_vbgi_upscale_bilinear_sharpness,
        .default_value = 1.f,
        .label = "Bilinear Weight Sharpness",
        .section = "GTVBGI Upscaling",
        .tooltip = "1.0 keeps the current spatial weights. Higher values favor the strongest tap and look sharper; lower values spread weight more evenly. Used by Joint Bilateral, Bilinear and Nearest Valid.",
        .min = 0.5f,
        .max = 4.f,
        .format = "%.2f",
        .is_enabled = []() { return g_vbgi_enabled > 0.5f && g_vbgi_upscale_algorithm < 2.5f; },
        .is_visible = []() { return VbgiRowVisible(); },
    },
    new renodx::utils::settings::Setting{
        .key = "VBGIUpscaleNearestFallback",
        .binding = &g_vbgi_upscale_nearest_fallback,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Nearest Valid Fallback",
        .section = "GTVBGI Upscaling",
        .tooltip = "When all joint-bilateral taps are rejected by the edge weights, use the valid tap with the strongest spatial weight. Off leaves black at that pixel.",
        .labels = {"Off", "On"},
        .is_enabled = []() { return g_vbgi_enabled > 0.5f && (g_vbgi_upscale_algorithm < 0.5f || g_vbgi_upscale_algorithm > 1.5f); },
        .is_visible = []() { return VbgiRowVisible(); },
    },
    new renodx::utils::settings::Setting{
        .key = "VBGINoiseType",
        .binding = &g_vbgi_noise_type,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 2.f,
        .label = "Noise Type",
        .section = "GTVBGI",
        .tooltip = "IS-FAST = pre-computed blue noise (baked into the addon). IGN = Interleaved Gradient Noise. Hilbert = Hilbert curve noise. Only applies when IS-FAST master toggle is On; forced to Hilbert when Off.",
        .labels = {"IS-FAST", "IGN", "Hilbert"},
        .is_enabled = []() { return g_vbgi_enabled > 0.5f && g_isfast_enabled > 0.5f; },
        .is_visible = []() { return VbgiRowVisible(); },
    },
    new renodx::utils::settings::Setting{
        .key = "SSGIIntensity",
        .binding = &shader_injection.vbgi_intensity,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 1.0f,
        .label = "Intensity",
        .section = "GTVBGI",
        .min = 0.0f,
        .max = 5.0f,
        .format = "%.2f",
        .is_enabled = []() { return g_vbgi_enabled > 0.5f; },
        .is_visible = []() { return VbgiRowVisible(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CharGTVBGIMaskStr",
        .binding = &shader_injection.char_gtvbgi_mask_strength,
        .default_value = 75.f,
        .label = "Character Mask Strength",
        .section = "GTVBGI",
        .min = 0.f,
        .max = 100.f,
        .is_enabled = []() { return g_vbgi_enabled > 0.5f; },
        .parse = [](float v) { return v * 0.01f; },
        .is_visible = []() { return VbgiRowVisible(); },
    },
    new renodx::utils::settings::Setting{
        .key = "VBGIRadius",
        .binding = &g_vbgi_radius,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 0.5f,
        .label = "Radius",
        .section = "GTVBGI",
        .tooltip = "World-space radius of the visibility search.",
        .min = 0.1f,
        .max = 10.0f,
        .format = "%.2f",
        .is_enabled = []() { return g_vbgi_enabled > 0.5f; },
        .is_visible = []() { return VbgiRowVisible(); },
    },
    new renodx::utils::settings::Setting{
        .key = "VBGIAutoThickness",
        .binding = &g_vbgi_auto_thickness,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Auto Thickness",
        .section = "GTVBGI",
        .tooltip = "Estimate occluder thickness from connected depth samples. Base Thickness sets the starting thickness.",
        .labels = {"Off", "On"},
        .is_enabled = []() { return g_vbgi_enabled > 0.5f; },
        .is_visible = []() { return VbgiRowVisible(); },
    },
    new renodx::utils::settings::Setting{
        .key = "VBGIThickness",
        .binding = &g_vbgi_bitmask_thickness,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 0.30f,
        .label = "Bitmask Thickness",
        .section = "GTVBGI",
        .tooltip = "Constant thickness used when Auto Thickness is off.",
        .min = 0.01f,
        .max = 2.0f,
        .format = "%.2f",
        .is_enabled = []() { return g_vbgi_enabled > 0.5f && g_vbgi_auto_thickness < 0.5f; },
        .is_visible = []() { return VbgiRowVisible(); },
    },
    new renodx::utils::settings::Setting{
        .key = "VBGIBaseThickness",
        .binding = &g_vbgi_base_thickness,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 0.20f,
        .label = "Base Thickness",
        .section = "GTVBGI",
        .tooltip = "Starting thickness for the Auto Thickness estimate.",
        .min = 0.01f,
        .max = 2.0f,
        .format = "%.2f",
        .is_enabled = []() { return g_vbgi_enabled > 0.5f && g_vbgi_auto_thickness > 0.5f; },
        .is_visible = []() { return VbgiRowVisible(); },
    },
    new renodx::utils::settings::Setting{
        .key = "VBGIBackfaceMode",
        .binding = &g_vbgi_backface_mode,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 1.f,
        .label = "Backface Rejection",
        .section = "GTVBGI",
        .labels = {"Off", "Simple", "Exact"},
        .is_enabled = []() { return g_vbgi_enabled > 0.5f; },
        .is_visible = []() { return VbgiRowVisible(); },
    },
    new renodx::utils::settings::Setting{
        .key = "VBGIDepthMipOffset",
        .binding = &g_vbgi_depth_mip_offset,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 6.f,
        .label = "Depth MIP Offset",
        .section = "GTVBGI",
        .min = 2.0f,
        .max = 6.0f,
        .format = "%.2f",
        .is_enabled = []() { return g_vbgi_enabled > 0.5f; },
        .is_visible = []() { return VbgiRowVisible(); },
    },
    new renodx::utils::settings::Setting{
        .key = "VBGIMrtNormal",
        .binding = &g_vbgi_mrt_normal,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "MRT Normal Input",
        .section = "GTVBGI",
        .tooltip = "Off = depth normals only. On = use game g-buffer normals.",
        .labels = {"Off (Depth)", "On (MRT)"},
        .is_enabled = []() { return g_vbgi_enabled > 0.5f; },
        .is_visible = []() { return VbgiRowVisible(); },
    },
    new renodx::utils::settings::Setting{
        .key = "VBGIAmbient",
        .binding = &g_vbgi_ambient,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 0.5f,
        .label = "Ambient Contribution",
        .section = "GTVBGI",
        .min = 0.0f,
        .max = 2.0f,
        .format = "%.2f",
        .is_enabled = []() { return g_vbgi_enabled > 0.5f; },
        .is_visible = []() { return VbgiRowVisible(); },
    },
    new renodx::utils::settings::Setting{
        .key = "VBGILights",
        .binding = &g_vbgi_lights,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 2.0f,
        .label = "Light Contribution",
        .section = "GTVBGI",
        .min = 0.0f,
        .max = 2.0f,
        .format = "%.2f",
        .is_enabled = []() { return g_vbgi_enabled > 0.5f; },
        .is_visible = []() { return VbgiRowVisible(); },
    },
    new renodx::utils::settings::Setting{
        .key = "VBGISun",
        .binding = &g_vbgi_sun,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 1.0f,
        .label = "Sun Contribution",
        .section = "GTVBGI",
        .min = 0.0f,
        .max = 2.0f,
        .format = "%.2f",
        .is_enabled = []() { return g_vbgi_enabled > 0.5f; },
        .is_visible = []() { return VbgiRowVisible(); },
    },
    new renodx::utils::settings::Setting{
        .key = "VBGIEmissive",
        .binding = &g_vbgi_emissive,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 1.0f,
        .label = "Emissive Contribution",
        .section = "GTVBGI",
        .min = 0.0f,
        .max = 2.0f,
        .format = "%.2f",
        .is_enabled = []() { return g_vbgi_enabled > 0.5f; },
        .is_visible = []() { return VbgiRowVisible(); },
    },
    new renodx::utils::settings::Setting{
        .key = "SSGIMultiBounce",
        .binding = &shader_injection.vbgi_multibounce,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Multi-Bounce",
        .section = "GTVBGI",
        .tooltip = "Enables multi-bounce GI: previous frame's indirect light feeds back into the GI computation.",
        .labels = {"Off", "On"},
        .is_enabled = []() { return g_vbgi_enabled > 0.5f; },
        .is_visible = []() { return VbgiRowVisible(); },
    },
    new renodx::utils::settings::Setting{
        .key = "VBGIMultiBounceStrength",
        .binding = &g_vbgi_multibounce_strength,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 0.5f,
        .label = "Multi-Bounce Strength",
        .section = "GTVBGI",
        .min = 0.0f,
        .max = 1.0f,
        .format = "%.2f",
        .is_enabled = []() { return g_vbgi_enabled > 0.5f && shader_injection.vbgi_multibounce > 0.5f; },
        .is_visible = []() { return VbgiRowVisible(); },
    },
    new renodx::utils::settings::Setting{
        .key = "VBGIFarField",
        .binding = &g_vbgi_far_enabled,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Far Field",
        .section = "GTVBGI",
        .tooltip = "Adds bounce light from beyond Radius: one screen-space ray per texel through the directions the near field left open. Off = GTVBGI unchanged.",
        .labels = {"Off", "On"},
        .is_enabled = []() { return g_vbgi_enabled > 0.5f; },
        .is_visible = []() { return VbgiRowVisible(); },
    },
    new renodx::utils::settings::Setting{
        .key = "VBGIFarDistance",
        .binding = &g_vbgi_far_distance,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = IsKai() ? 1.f : 3.f,
        .label = "Far Distance",
        .section = "GTVBGI",
        .tooltip = "World-space end of the far-field ray.",
        .min = 1.f,
        .max = 30.f,
        .format = "%.1f",
        .is_enabled = []() { return g_vbgi_enabled > 0.5f && g_vbgi_far_enabled > 0.5f; },
        .is_visible = []() { return VbgiRowVisible(); },
    },
    new renodx::utils::settings::Setting{
        .key = "VBGIFarIntensity",
        .binding = &g_vbgi_far_intensity,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 2.f,
        .label = "Far Intensity",
        .section = "GTVBGI",
        .tooltip = "Scales the far-field bounce light before the denoise. 1 = physically matched to the near field.",
        .min = 0.f,
        .max = 4.f,
        .format = "%.2f",
        .is_enabled = []() { return g_vbgi_enabled > 0.5f && g_vbgi_far_enabled > 0.5f; },
        .is_visible = []() { return VbgiRowVisible(); },
    },
    new renodx::utils::settings::Setting{
        .key = "VBGIFarHitWindow",
        .binding = &g_vbgi_far_hit_window,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f,
        .label = "Far Hit Window",
        .section = "GTVBGI",
        .tooltip = "Thin = active thickness tolerance. Wide also counts surfaces crossed between march steps (more hits; may add faint halos).",
        .labels = {"Thin", "Wide"},
        .is_enabled = []() { return g_vbgi_enabled > 0.5f && g_vbgi_far_enabled > 0.5f; },
        .is_visible = []() { return VbgiRowVisible(); },
    },
    new renodx::utils::settings::Setting{
        .key = "VBGIFarCandidates",
        .binding = &g_vbgi_far_candidates,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 4.f,
        .label = "Far Candidates",
        .section = "GTVBGI",
        .tooltip = "Fresh far-field rays per texel each frame (at most the Quality's slice count). More = less boiling, more cost.",
        .min = 1.f,
        .max = 4.f,
        .format = "%.0f",
        .is_enabled = []() { return g_vbgi_enabled > 0.5f && g_vbgi_far_enabled > 0.5f; },
        .is_visible = []() { return VbgiRowVisible(); },
    },
    new renodx::utils::settings::Setting{
        .key = "VBGIFarTemporal",
        .binding = &g_vbgi_far_temporal,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Far Temporal Reuse",
        .section = "GTVBGI",
        .tooltip = "Reuses last frame's far-field sample per texel (ReSTIR). Runs only with inline dispatch (Deferred Dispatch Off).",
        .labels = {"Off", "On"},
        .is_enabled = []() { return g_vbgi_enabled > 0.5f && g_vbgi_far_enabled > 0.5f; },
        .is_visible = []() { return VbgiRowVisible(); },
    },
    new renodx::utils::settings::Setting{
        .key = "VBGIFarHistory",
        .binding = &g_vbgi_far_history_length,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 30.f,
        .label = "Far History Length",
        .section = "GTVBGI",
        .tooltip = "Frames of far-field history a texel can keep. Lower = faster response to light changes, more noise.",
        .min = 4.f,
        .max = 30.f,
        .format = "%.0f",
        .is_enabled = []() { return g_vbgi_enabled > 0.5f && g_vbgi_far_enabled > 0.5f && g_vbgi_far_temporal > 0.5f; },
        .is_visible = []() { return VbgiRowVisible(); },
    },
    new renodx::utils::settings::Setting{
        .key = "VBGIFarSpatial",
        .binding = &g_vbgi_far_spatial,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Far Spatial Reuse",
        .section = "GTVBGI",
        .tooltip = "Shares far-field samples between nearby texels (ReSTIR).",
        .labels = {"Off", "On"},
        .is_enabled = []() { return g_vbgi_enabled > 0.5f && g_vbgi_far_enabled > 0.5f; },
        .is_visible = []() { return VbgiRowVisible(); },
    },
    new renodx::utils::settings::Setting{
        .key = "VBGIFarSpatialNeighbors",
        .binding = &g_vbgi_far_spatial_neighbors,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 4.f,
        .label = "Far Spatial Neighbours",
        .section = "GTVBGI",
        .tooltip = "Nearby texels each texel borrows far samples from. More = less noise, slightly more cost.",
        .min = 1.f,
        .max = 8.f,
        .format = "%.0f",
        .is_enabled = []() { return g_vbgi_enabled > 0.5f && g_vbgi_far_enabled > 0.5f && g_vbgi_far_spatial > 0.5f; },
        .is_visible = []() { return VbgiRowVisible(); },
    },
    new renodx::utils::settings::Setting{
        .key = "VBGIFarSpatialRadius",
        .binding = &g_vbgi_far_spatial_radius,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 1.f,
        .label = "Far Spatial Radius",
        .section = "GTVBGI",
        .tooltip = "Scale of the spatial reuse radius. Larger = calmer, but more rejections and possible halos at edges.",
        .min = 0.5f,
        .max = 2.f,
        .format = "%.2f",
        .is_enabled = []() { return g_vbgi_enabled > 0.5f && g_vbgi_far_enabled > 0.5f && g_vbgi_far_spatial > 0.5f; },
        .is_visible = []() { return VbgiRowVisible(); },
    },
    new renodx::utils::settings::Setting{
        .key = "VBGIFarMultiBounceStrength",
        .binding = &g_vbgi_far_multibounce_strength,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 0.25f,
        .label = "Far Multi-Bounce Strength",
        .section = "GTVBGI",
        .tooltip = "Feeds the far field's share of last frame's GI back as sender light. 0 = only the near field feeds back.",
        .min = 0.f,
        .max = 1.f,
        .format = "%.2f",
        .is_enabled = []() { return g_vbgi_enabled > 0.5f && g_vbgi_far_enabled > 0.5f && shader_injection.vbgi_multibounce > 0.5f; },
        .is_visible = []() { return VbgiRowVisible(); },
    },
    // �� VBGI debug ��
    new renodx::utils::settings::Setting{
        .key = "VBGIDebugMode",
        .binding = &g_vbgi_debug_mode,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f,
        .label = "GTVBGI Debug View",
        .section = "GTVBGI",
        .tooltip = "Replace scene with GTVBGI debug textures. Temporal Confidence shows far-color history when Temporal Far Field only is active, and joint history otherwise. While not Off this replaces the GTVBAO debug view.",
        .labels = {"Off", "Final GI", "Raw GI", "Radiance Input", "Bitmask", "Temporal Confidence", "Term Split", "Far Field", "Far Samples", "Far Reservoir"},
        .is_visible = []() { return VbgiRowVisible(); },
    },
    // -- Kai: GTVBAO VBGI Falcom SSGI consumption --
    new renodx::utils::settings::Setting{
        .key = "SSGIKaiConsumeFalcom",
        .binding = &shader_injection.vbgi_kai_consume_falcom,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f,
        .label = "Consume Falcom SSGI (Kai)",
        .section = "GTVBGI",
        .tooltip = "When ON, Falcom's SSGI color modulates the GTVBGI contribution (main path) before it is added. Multiplicative and not energy preserving.",
        .labels = {"Off", "On"},
        .is_enabled = []() { return g_vbgi_enabled > 0.5f; },
        .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "SSGIKaiFalcomBlend",
        .binding = &shader_injection.vbgi_kai_falcom_blend,
        .default_value = 0.5f,
        .label = "Falcom SSGI Blend (Kai)",
        .section = "GTVBGI",
        .tooltip = "How much Falcom SSGI color modulates GTVBGI. 0 = no modulation (additive only), 1 = full multiplicative blend.",
        .min = 0.f,
        .max = 1.0f,
        .format = "%.2f",
        .is_enabled = []() { return g_vbgi_enabled > 0.5f && shader_injection.vbgi_kai_consume_falcom > 0.5f; },
        .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "SSGIKaiGTVBAOOnly",
        .binding = &shader_injection.vbgi_kai_gtvbao_only,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f,
        .label = "GTVBAO VBGI Only (Kai)",
        .section = "GTVBGI",
        .tooltip = "When ON, Falcom's own SSGI is suppressed (its ambient and shadow-colour terms) and only GTVBGI provides indirect light. OFF keeps both.",
        .labels = {"Off", "On"},
        .is_enabled = []() { return g_vbgi_enabled > 0.5f; },
        .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CPUOptDeferredDispatch",
        .binding = &g_cpuopt_deferred_dispatch,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f,
        .label = "Deferred Dispatch",
        .section = "CPU Opt",
        .tooltip = "Move GTVBAO/VBGI dispatch to OnPresent (1-frame latency). Kai-only, default OFF � avoids latency. Temporal Accumulation is disabled while this is On. Debug views are one frame behind.",
        .labels = {"Off", "On"},
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CPUOptEnsurePipelines",
        .binding = &g_cpuopt_ensure_pipelines,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Ensure Pipelines",
        .section = "CPU Opt",
        .tooltip = "Don't destroy/recreate pipelines every frame (kai-style).",
        .labels = {"Off", "On"},
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    // �� Shadow Maps ��
    new renodx::utils::settings::Setting{
        .key = "ShadowFilterMethod",
        .binding = &shader_injection.shadow_filter_method,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 2.f,
        .label = "Shadow Filter Method",
        .section = "Shadow Maps",
        .tooltip = "CSM filtering: Off = single sample. Falcom = vanilla 10-tap PCF. CHSS = contact-hardening soft shadows (Vogel disk + variable-radius PCF).",
        .labels = {"Off", "Falcom", "CHSS"},
        .is_visible = []() { return !IsKai(); },
    },
    new renodx::utils::settings::Setting{
        .key = "ShadowEdgeTint",
        .binding = &shader_injection.shadow_edge_tint,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 2.f,
        .label = "Colored Shadow Penumbra",
        .section = "Shadow Maps",
        .tooltip = "Off = neutral edges. Falcom = vanilla red tint. Improved = vibrancy boost in penumbra.",
        .labels = {"Off", "Falcom", "Improved"},
        .is_visible = []() { return !IsKai(); },
    },
    // �� CHSS Settings (enabled when ShadowFilterMethod = CHSS) ��
    new renodx::utils::settings::Setting{
        .key = "ShadowCHSSJitter",
        .binding = &shader_injection.shadow_pcss_jitter_enabled,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "CHSS Jitter",
        .section = "Shadow Maps",
        .tooltip = "Rotate the CHSS sample pattern each frame using the selected noise source (see Noise Mode).",
        .labels = {"Off", "On"},
        .is_enabled = []() { return shader_injection.shadow_filter_method > 1.5f; },
        .is_visible = []() { return !IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "ShadowCHSSNoiseMode",
        .binding = &shader_injection.shadow_chss_noise_mode,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 1.f,
        .label = "Noise Mode",
        .section = "Shadow Maps",
        .tooltip = "Jitter noise source. IGN = interleaved gradient noise (no texture needed). IS-FAST = pre-computed spatio-temporal blue noise (baked into the addon; requires ISFASTMasterEnable; falls back to IGN if unavailable).",
        .labels = {"IGN", "IS-FAST"},
        .is_enabled = []() { return shader_injection.shadow_filter_method > 1.5f && shader_injection.shadow_pcss_jitter_enabled > 0.5f && g_isfast_enabled > 0.5f; },
        .is_visible = []() { return !IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "ShadowCHSSBlockerSamples",
        .binding = &shader_injection.shadow_chss_blocker_count,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 16.f,
        .label = "Blocker Samples",
        .section = "Shadow Maps",
        .tooltip = "Sample count for the average blocker search. Higher = smoother penumbra estimate, lower = faster.",
        .min = 4.f,
        .max = 64.f,
        .format = "%d",
        .is_enabled = []() { return shader_injection.shadow_filter_method > 1.5f; },
        .is_visible = []() { return !IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "ShadowCHSSFilterSamples",
        .binding = &shader_injection.shadow_chss_sample_count,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 64.f,
        .label = "Filter Samples",
        .section = "Shadow Maps",
        .tooltip = "Sample count for the variable-radius PCF filter. Higher = softer, less noisy shadows, lower = faster.",
        .min = 4.f,
        .max = 64.f,
        .format = "%d",
        .is_enabled = []() { return shader_injection.shadow_filter_method > 1.5f; },
        .is_visible = []() { return !IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "ShadowCHSSJitterAmount",
        .binding = &shader_injection.shadow_pcss_jitter_amount,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 1.f,
        .label = "Jitter Amount",
        .section = "Shadow Maps",
        .tooltip = "0 = static Vogel disk, 1 = full temporal rotation.",
        .min = 0.0f,
        .max = 1.0f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.shadow_filter_method > 1.5f && shader_injection.shadow_pcss_jitter_enabled > 0.5f; },
        .is_visible = []() { return !IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "ShadowCHSSJitterSpeed",
        .binding = &shader_injection.shadow_pcss_jitter_speed,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 237.f,
        .label = "Jitter Speed",
        .section = "Shadow Maps",
        .tooltip = "Temporal animation speed. Higher = faster rotation.",
        .min = 0.0f,
        .max = 500.0f,
        .format = "%.0f",
        .is_enabled = []() { return shader_injection.shadow_filter_method > 1.5f && shader_injection.shadow_pcss_jitter_enabled > 0.5f; },
        .is_visible = []() { return !IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "ShadowBaseSoftness",
        .binding = &shader_injection.shadow_base_softness,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 0.0f,
        .label = "Base Softness",
        .section = "Shadow Maps",
        .tooltip = "Constant minimum penumbra width. Contact-hard at 0, always soft at 0.5.",
        .min = 0.0f,
        .max = 1.0f,
        .format = "%.3f",
        .is_enabled = []() { return shader_injection.shadow_filter_method > 1.5f; },
        .is_visible = []() { return !IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "ShadowCHSSSearchRadius",
        .binding = &shader_injection.shadow_chss_search_radius,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 1.f,
        .label = "World Softness",
        .section = "Shadow Maps",
        .tooltip = "Desired softness in world units. Same value = same penumbra width across all cascades. 0.1=sharp, 5=very soft.",
        .min = 0.1f,
        .max = 5.0f,
        .format = "%.1f",
        .is_enabled = []() { return shader_injection.shadow_filter_method > 1.5f; },
        .is_visible = []() { return !IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "ShadowCHSSPenumbraScale",
        .binding = &shader_injection.shadow_chss_penumbra_scale,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 5000.f,
        .label = "Penumbra Scale",
        .section = "Shadow Maps",
        .tooltip = "How fast penumbra widens with occluder distance (the paper's 80.0 constant). Higher = softer distant shadows.",
        .min = 1.0f,
        .max = 10000.0f,
        .format = "%.1f",
        .is_enabled = []() { return shader_injection.shadow_filter_method > 1.5f; },
        .is_visible = []() { return !IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "ShadowCHSSDepthCap",
        .binding = &shader_injection.shadow_chss_depth_cap,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 1.0f,
        .label = "Depth Sensitivity",
        .section = "Shadow Maps",
        .tooltip = "Max depth difference for penumbra. Higher = more distance-based softening.",
        .min = 0.01f,
        .max = 1.0f,
        .format = "%.3f",
        .is_enabled = []() { return shader_injection.shadow_filter_method > 1.5f; },
        .is_visible = []() { return !IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "ShadowCHSSMinRadius",
        .binding = &shader_injection.shadow_chss_min_radius,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 0.f,
        .label = "Min Filter Radius",
        .section = "Shadow Maps",
        .tooltip = "Guaranteed minimum PCF filter radius in shadow map texels. Prevents filter from collapsing. 0=off.",
        .min = 0.f,
        .max = 100.0f,
        .format = "%.0f",
        .is_enabled = []() { return shader_injection.shadow_filter_method > 1.5f; },
        .is_visible = []() { return !IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "ShadowCHSSPostBlur",
        .binding = &shader_injection.shadow_chss_post_blur,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 100.f,
        .label = "Post Blur",
        .section = "Shadow Maps",
        .tooltip = "Strength of the screen-space bilateral blur applied to the CHSS shadow mask. 0 = off (passthrough), 100 = full strength.",
        .min = 0.0f,
        .max = 100.0f,
        .format = "%.0f",
        .is_enabled = []() { return shader_injection.shadow_filter_method > 1.5f; },
        .is_visible = []() { return !IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "ShadowCHSSOnsetBias",
        .binding = &shader_injection.shadow_chss_onset_bias,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 0.f,
        .label = "Softening Onset",
        .section = "Shadow Maps",
        .tooltip = "Adds depth separation before the penumbra response. Higher values start soft shadows closer to contact; 0 preserves vanilla contact hardening.",
        .min = 0.0f,
        .max = 0.2f,
        .format = "%.4f",
        .is_enabled = []() { return shader_injection.shadow_filter_method > 1.5f; },
        .is_visible = []() { return !IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "ShadowCHSSPenumbraCurve",
        .binding = &shader_injection.shadow_chss_penumbra_curve,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 1.5f,
        .label = "Penumbra Curve",
        .section = "Shadow Maps",
        .tooltip = "Exponent applied to normalized penumbra separation. Values below 2 respond more aggressively near contact; values above 2 stay harder longer.",
        .min = 0.5f,
        .max = 4.0f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.shadow_filter_method > 1.5f; },
        .is_visible = []() { return !IsKai() && IsAdvancedSettingsMode(); },
    },
    // �� Colored Shadow Penumbra (Improved mode) ��
    new renodx::utils::settings::Setting{
        .key = "ShadowPenumbraColorStrength",
        .binding = &shader_injection.shadow_penumbra_color_strength,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 0.65f,
        .label = "Penumbra Color Strength",
        .section = "Shadow Maps",
        .tooltip = "How strongly the vibrancy effect is applied in penumbra regions. 0=off, 1=full.",
        .min = 0.f,
        .max = 2.0f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.shadow_edge_tint > 1.5f; },
        .is_visible = []() { return !IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "ShadowPenumbraVibrance",
        .binding = &shader_injection.shadow_penumbra_vibrance,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 30.f,
        .label = "Penumbra Vibrance",
        .section = "Shadow Maps",
        .tooltip = "Vibrance adjustment in penumbra. 0=grayscale, 1=neutral, >1=more vivid. Protects already-saturated colors.",
        .min = 0.f,
        .max = 100.0f,
        .format = "%.1f",
        .is_enabled = []() { return shader_injection.shadow_edge_tint > 1.5f; },
        .is_visible = []() { return !IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "ShadowPenumbraDetection",
        .binding = &shader_injection.shadow_penumbra_detection,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 1.0f,
        .label = "Penumbra Detection",
        .section = "Shadow Maps",
        .tooltip = "What counts as penumbra. Higher = wider detection area, more of the image gets the effect.",
        .min = 0.01f,
        .max = 1.0f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.shadow_edge_tint > 1.5f; },
        .is_visible = []() { return !IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "ShadowPenumbraColorBrightness",
        .binding = &shader_injection.shadow_penumbra_color_brightness,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 1.f,
        .label = "Penumbra Color Brightness",
        .section = "Shadow Maps",
        .tooltip = "Brightness multiplier for the vibrancy tint color. 1=neutral, 0=black, >1=brighter.",
        .min = 0.f,
        .max = 5.0f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.shadow_edge_tint > 1.5f; },
        .is_visible = []() { return !IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "ShadowPenumbraFalcomBlend",
        .binding = &shader_injection.shadow_penumbra_falcom_blend,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 0.5f,
        .label = "Falcom Penumbra Blend",
        .section = "Shadow Maps",
        .tooltip = "Blend the vibrancy effect toward Falcom's red shadowEdgeColor tint. 0=pure vibrancy, 1=pure Falcom.",
        .min = 0.f,
        .max = 1.0f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.shadow_edge_tint > 1.5f; },
        .is_visible = []() { return !IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "ShadowPenumbraEdgeVibrance",
        .binding = &shader_injection.shadow_penumbra_edge_vibrance,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 0.3f,
        .label = "Edge Color Vibrance",
        .section = "Shadow Maps",
        .tooltip = "Vibrance applied to shadowEdgeColor when Falcom blend > 0. 0=grayscale, 1=neutral, >1=vivid.",
        .min = 0.f,
        .max = 100.0f,
        .format = "%.1f",
        .is_enabled = []() { return shader_injection.shadow_edge_tint > 1.5f; },
        .is_visible = []() { return !IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "ShadowPenumbraLightColorBlend",
        .binding = &shader_injection.shadow_penumbra_lightcolor_blend,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 0.f,
        .label = "Light Color Blend",
        .section = "Shadow Maps",
        .tooltip = "Blend the penumbra tint toward the sun's lightColor. 0=no effect, 1=fully sun-colored penumbra.",
        .min = 0.f,
        .max = 1.0f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.shadow_edge_tint > 1.5f; },
        .is_visible = []() { return !IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "ShadowPenumbraLightColorSaturation",
        .binding = &shader_injection.shadow_penumbra_lightcolor_saturation,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 0.f,
        .label = "Light Color Saturation",
        .section = "Shadow Maps",
        .tooltip = "Vibrance applied to lightColor before blending. 0=grayscale, 1=neutral, >1=vivid sun color.",
        .min = 0.f,
        .max = 100.0f,
        .format = "%.1f",
        .is_enabled = []() { return shader_injection.shadow_edge_tint > 1.5f; },
        .is_visible = []() { return !IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "ShadowPenumbraDebugView",
        .binding = &shader_injection.shadow_penumbra_debug_view,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f,
        .label = "Penumbra Debug View",
        .section = "Shadow Maps",
        .tooltip = "Visualize penumbra processing. PenumbraMask=detection area, TintColor=adjusted color, Result=final blend.",
        .labels = {"Off", "Penumbra Mask", "Tint Color", "Result", "Sun Color"},
        .is_enabled = []() { return shader_injection.shadow_edge_tint > 1.5f; },
        .is_visible = []() { return !IsKai() && IsAdvancedSettingsMode(); },
    },
    // �� Shadows (Kai) ��
    new renodx::utils::settings::Setting{
        .key = "KaiShadowBaseSoftness",
        .binding = &shader_injection.shadow_base_softness,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 0.05f,
        .label = "Base Softness",
        .section = "Shadows",
        .tooltip = "Constant minimum penumbra width for PCSS shadows. 0 = contact-hard, higher = always soft.",
        .min = 0.0f,
        .max = 1.0f,
        .format = "%.3f",
        .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "KaiShadowJitter",
        .binding = &shader_injection.shadow_pcss_jitter_enabled,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "PCSS Jitter",
        .section = "Shadows",
        .tooltip = "Use IS-FAST blue noise to rotate PCSS sample pattern each frame.",
        .labels = {"Off", "On"},
        .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "KaiShadowJitterAmount",
        .binding = &shader_injection.shadow_pcss_jitter_amount,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 1.f,
        .label = "Jitter Amount",
        .section = "Shadows",
        .tooltip = "0 = static Poisson, 1 = full temporal rotation.",
        .min = 0.0f,
        .max = 1.0f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.shadow_pcss_jitter_enabled > 0.5f; },
        .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "KaiShadowJitterSpeed",
        .binding = &shader_injection.shadow_pcss_jitter_speed,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 237.f,
        .label = "Jitter Speed",
        .section = "Shadows",
        .tooltip = "Temporal animation speed. Higher = faster rotation.",
        .min = 0.0f,
        .max = 500.0f,
        .format = "%.0f",
        .is_enabled = []() { return shader_injection.shadow_pcss_jitter_enabled > 0.5f; },
        .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    // -- Colored Shadow Penumbra (Kai) --
    new renodx::utils::settings::Setting{
        .key = "KaiPenumbraMode",
        .binding = &shader_injection.shadow_edge_tint_kai,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Colored Penumbra",
        .section = "Shadows",
        .tooltip = "Improved mode applies vibrance boost in shadow penumbra regions. No Falcom fallback on Kai.",
        .labels = {"Off", "Improved"},
        .is_visible = []() { return IsKai(); },
    },
    // �� Character hero light (Kai) ��
    new renodx::utils::settings::Setting{
        .key = "KaiCharacterLight",
        .binding = &shader_injection.character_light_strength,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 1.f,
        .label = "Hero Light Suppression",
        .section = "Shadows",
        .tooltip = "Suppresses dynamic point/spot lights on character pixels. 0=off (vanilla), 1=fully remove.",
        .min = 0.f,
        .max = 1.0f,
        .format = "%.2f",
        .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "KaiPenumbraStrength",
        .binding = &shader_injection.shadow_penumbra_color_strength,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 0.15f,
        .label = "Penumbra Strength",
        .section = "Shadows",
        .tooltip = "Overall strength of the colored penumbra effect.",
        .min = 0.0f,
        .max = 2.0f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.shadow_edge_tint_kai >= 1.0f; },
        .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "KaiPenumbraVibrance",
        .binding = &shader_injection.shadow_penumbra_vibrance,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 30.f,
        .label = "Penumbra Vibrance",
        .section = "Shadows",
        .tooltip = "Vibrance applied to surface color in penumbra. 0=grayscale, 1=neutral, >1=vivid.",
        .min = 0.0f,
        .max = 100.0f,
        .format = "%.1f",
        .is_enabled = []() { return shader_injection.shadow_edge_tint_kai >= 1.0f; },
        .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "KaiPenumbraDetection",
        .binding = &shader_injection.shadow_penumbra_detection,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 2.0f,
        .label = "Penumbra Detection",
        .section = "Shadows",
        .tooltip = "Penumbra detection width. Higher = wider area gets the effect.",
        .min = 0.01f,
        .max = 2.0f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.shadow_edge_tint_kai >= 1.0f; },
        .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "KaiPenumbraBrightness",
        .binding = &shader_injection.shadow_penumbra_color_brightness,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 1.f,
        .label = "Penumbra Brightness",
        .section = "Shadows",
        .tooltip = "Brightness multiplier for the penumbra tint color.",
        .min = 0.0f,
        .max = 5.0f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.shadow_edge_tint_kai >= 1.0f; },
        .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "KaiPenumbraDebug",
        .binding = &shader_injection.shadow_penumbra_debug_view,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f,
        .label = "Penumbra Debug",
        .section = "Shadows",
        .tooltip = "Visualize penumbra: 0=Off, 1=Detection Mask, 2=Tint Color, 3=Result, 4=Sun Color.",
        .labels = {"Off", "Penumbra Mask", "Tint Color", "Result", "Sun Color"},
        .is_enabled = []() { return shader_injection.shadow_edge_tint >= 1.0f; },
        .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    // -- Contact Shadows ------------------------------------------------
    new renodx::utils::settings::Setting{
        .key = "ContactShadowsEnabled",
        .binding = &shader_injection.cs_contact_enabled,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f,
        .label = "Sun Contact Shadows",
        .section = "Contact Shadows",
        .tooltip = "Sun Contact Shadows add subtle shadows where objects and characters meet the ground, making them feel more naturally connected to their surroundings.",
        .labels = {"Off", "On"},
        .is_visible = []() { return IsAdvancedSettingsMode() && !IsKyoto() && !IsDaybreak2(); },
    },
    new renodx::utils::settings::Setting{
        .key = "ContactShadowsEnvStrength",
        .binding = &shader_injection.cs_contact_env_strength,
        .default_value = 1.f,
        .label = "Environment Strength",
        .section = "Contact Shadows",
        .tooltip = "Strength of the contact term on non-character pixels.",
        .min = 0.f,
        .max = 1.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.cs_contact_enabled >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "ContactShadowsCharStrength",
        .binding = &shader_injection.cs_contact_char_strength,
        .default_value = 1.f,
        .label = "Character Strength",
        .section = "Contact Shadows",
        .tooltip = "Strength of the contact term on character pixels, selected by the "
                   "foliage/character mask.",
        .min = 0.f,
        .max = 1.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.cs_contact_enabled >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        // Deliberately a different key from the old raw "ContactShadowsSamples"
        // slider. That setting stored the step count itself; this one stores a 0-3
        // quality INDEX, so a saved preset holding 32 would be clamped into the
        // index range and silently resolve to Ultra. Renaming orphans the stale
        // value instead, so the tiers start clean at their default. Same reasoning
        // as DOFQuality.
        .key = "ContactShadowsQuality",
        .binding = &shader_injection.cs_contact_sample_count,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 1.f,
        .label = "Quality",
        .section = "Contact Shadows",
        .tooltip = "March quality: Low 4, Medium 8, High 12, Ultra 16 steps. Higher "
                   "tiers resolve a cleaner contact term at linearly higher cost.",
        .labels = {"Low", "Medium", "High", "Ultra"},
        // .min must be 0: with .labels, GetMax() is labels.size() - 1, and a .min of
        // 1 would clamp a saved "Low" (0) up to Medium and make Low unreachable
        // across restarts.
        .min = 0.f,
        .is_enabled = []() { return shader_injection.cs_contact_enabled >= 0.5f; },
        .parse = [](float value) { return ContactShadowQualitySamples(value); },
        .is_visible = []() { return IsAdvancedSettingsMode() && !IsKyoto() && !IsDaybreak2(); },
    },
    new renodx::utils::settings::Setting{
        .key = "ContactShadowsRayLength",
        .binding = &shader_injection.cs_contact_ray_length,
        .default_value = 2.f,
        .label = "Ray Length",
        .section = "Contact Shadows",
        .tooltip = "World-space distance the ray travels toward the light. Short values keep "
                   "the term to contact; long values reach genuine occluders.",
        .min = 1.f,
        .max = 200.f,
        .format = "%.1f",
        .is_enabled = []() { return shader_injection.cs_contact_enabled >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "ContactShadowsThickness",
        .binding = &shader_injection.cs_contact_thickness,
        .default_value = 0.29f,
        .label = "Surface Thickness",
        .section = "Contact Shadows",
        .tooltip = "How deep a hit must penetrate before it counts. Raise it to stop thin "
                   "geometry and depth noise from self-shadowing.",
        .min = 0.001f,
        .max = 4.f,
        .format = "%.3f",
        .is_enabled = []() { return shader_injection.cs_contact_enabled >= 0.5f || shader_injection.cs_contact_local_enabled >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "ContactShadowsBias",
        .binding = &shader_injection.cs_contact_bias,
        .default_value = 0.0001f,
        .label = "Depth Bias",
        .section = "Contact Shadows",
        .tooltip = "Minimum penetration before a hit counts, in device depth. Raise only if "
                   "you see acne.",
        .min = 0.f,
        .max = 0.2f,
        .format = "%.4f",
        .is_enabled = []() { return shader_injection.cs_contact_enabled >= 0.5f || shader_injection.cs_contact_local_enabled >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "ContactShadowsNormalBias",
        .binding = &shader_injection.cs_contact_normal_bias,
        .default_value = 0.1000f,
        .label = "Normal Bias",
        .section = "Contact Shadows",
        .tooltip = "Lifts the ray origin along the surface normal. More stable than depth "
                   "bias on curved surfaces.",
        .min = 0.f,
        .max = 1.f,
        .format = "%.4f",
        .is_enabled = []() { return shader_injection.cs_contact_enabled >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "ContactShadowResponseScale",
        .binding = &shader_injection.cs_contact_response_scale,
        .default_value = 4.f,
        .label = "Contact Shadow Response Scale",
        .section = "Contact Shadows",
        .tooltip = "How strong the contact shadow reads, relative to the ORIGINAL "
                   "4-sample contact shadow appearance. This is NOT a physically based "
                   "parameter and 4 is not a physical unit: 4 is simply the sample count "
                   "whose average appearance is being reproduced, so that the Quality "
                   "setting can be a pure quality control. At Quality Low (4 steps) and "
                   "scale 4 the march reproduces the old any-hit test exactly, value for "
                   "value. Raise this for a darker contact, lower it for a lighter one; "
                   "Quality then only changes how noisy the result is, not how wide or "
                   "how dark it is. Defaults to 4.",
        .min = 0.f,
        .max = 8.f,
        .format = "%.1f",
        .is_enabled = []() { return shader_injection.cs_contact_enabled >= 0.5f || shader_injection.cs_contact_local_enabled >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "ContactShadowsSunRange",
        .binding = &shader_injection.cs_contact_sun_range,
        .default_value = 0.f,
        .label = "Sun Range",
        .section = "Contact Shadows",
        .tooltip = "World distance past which sun contact shadows stop being applied, "
                   "because the CSM no longer has real coverage there. 0 = use the "
                   "engine's own last cascade split. This is the gate that keeps contact "
                   "shadows off interior walls: indoors the engine keeps a sun term "
                   "because the cascade clamps past its split instead of falling off, so "
                   "without this the effect darkens rooms. Raise it only if contact "
                   "shadows vanish too early outdoors; lower it if they still reach into "
                   "interiors. Contact shadows are a near-field effect, so a smaller "
                   "value is usually the honest one.",
        .min = 0.f,
        .max = 500.f,
        .format = "%.1f",
        .is_enabled = []() { return shader_injection.cs_contact_enabled >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "ContactShadowsSkyDepth",
        .binding = &shader_injection.cs_contact_sky_depth,
        .default_value = 100000.f,
        .label = "Sky Depth",
        .section = "Contact Shadows",
        .tooltip = "A device depth linearizing beyond this counts as no geometry, so the "
                   "march is skipped early. Higher is safer, lower is cheaper on open sky.",
        .min = 100.f,
        .max = 1000000.f,
        .format = "%.0f",
        .is_enabled = []() { return shader_injection.cs_contact_enabled >= 0.5f || shader_injection.cs_contact_local_enabled >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "ContactShadowsMaxDarkening",
        .binding = &shader_injection.cs_contact_max_darkening,
        .default_value = 0.5f,
        .label = "Max Darkening",
        .section = "Contact Shadows",
        .tooltip = "Caps how dark a contact-shadowed pixel may get. 1 allows full "
                   "black, 0 disables the darkening entirely. Lower it to keep "
                   "contact shadows from crushing in dark scenes.",
        .min = 0.f,
        .max = 1.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.cs_contact_enabled >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "ContactShadowsISFAST",
        .binding = &shader_injection.cs_contact_isfast_enabled,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "IS-FAST Jitter",
        .section = "Contact Shadows",
        .tooltip = "Jitters the march with the IS-FAST blue-noise volume and lets TAA resolve "
                   "it. There is no IGN fallback: without the volume the march runs unjittered "
                   "and bands visibly, which the log reports once.",
        .labels = {"Off", "On"},
        .is_enabled = []() { return shader_injection.cs_contact_enabled >= 0.5f || shader_injection.cs_contact_local_enabled >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "ContactShadowsDebug",
        .binding = &shader_injection.cs_contact_debug,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f,
        .label = "Debug View",
        .section = "Contact Shadows",
        .tooltip = "Stage Ladder colours each pixel by where the contact pass stopped: white = "
                   "lit, grey = the pass never wrote this pixel, red = the march ran and is occluded, magenta = no normal, "
                   "yellow = bad projection, cyan = beyond the sky cutoff, orange = bad depth, blue = "
                   "no light direction. Linear Depth shows the scene depth, 0 to 1 over 1000 world units.",
        .labels = {"Off", "Stage Ladder", "Linear Depth"},
        .is_enabled = []() { return shader_injection.cs_contact_enabled >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "ContactShadowsSunDenoise",
        .binding = &shader_injection.cs_contact_denoise_enabled,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Sun Denoise",
        .section = "Contact Shadows",
        .tooltip = "Smooths the IS-FAST grain of the sun contact term with an edge-aware blur that stops at depth and normal edges. Off leaves the term exactly as marched.",
        .labels = {"Off", "On"},
        .is_enabled = []() { return shader_injection.cs_contact_enabled >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode() && !IsKyoto() && !IsDaybreak2(); },
    },
    new renodx::utils::settings::Setting{
        .key = "ContactShadowsSunDenoiseRadius",
        .binding = &shader_injection.cs_contact_denoise_radius,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 4.f,
        .label = "Sun Denoise Radius",
        .section = "Contact Shadows",
        .tooltip = "Blur radius in pixels along each axis. Larger is smoother and costs more.",
        .min = 1.f,
        .max = 8.f,
        .format = "%d",
        .is_enabled = []() { return shader_injection.cs_contact_enabled >= 0.5f && shader_injection.cs_contact_denoise_enabled >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "ContactShadowsSunDenoiseDepthSigma",
        .binding = &shader_injection.cs_contact_denoise_depth_sigma,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 0.02f,
        .label = "Sun Denoise Depth Sigma",
        .section = "Contact Shadows",
        .tooltip = "How far a neighbour may sit off this pixel's surface plane, relative to its distance, before it stops contributing. Lower keeps depth edges sharper.",
        .min = 0.001f,
        .max = 0.2f,
        .format = "%.3f",
        .is_enabled = []() { return shader_injection.cs_contact_enabled >= 0.5f && shader_injection.cs_contact_denoise_enabled >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "ContactShadowsSunDenoiseNormalSigma",
        .binding = &shader_injection.cs_contact_denoise_normal_sigma,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 0.1f,
        .label = "Sun Denoise Normal Sigma",
        .section = "Contact Shadows",
        .tooltip = "How different a neighbour's surface normal may be before it stops contributing. Lower keeps creases and corners sharper.",
        .min = 0.01f,
        .max = 1.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.cs_contact_enabled >= 0.5f && shader_injection.cs_contact_denoise_enabled >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "ContactShadowsSunDenoisePasses",
        .binding = &shader_injection.cs_contact_denoise_passes,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 1.f,
        .label = "Sun Denoise Passes",
        .section = "Contact Shadows",
        .tooltip = "Horizontal + vertical iterations. Each extra pass widens the blur and adds the same cost again.",
        .min = 1.f,
        .max = 4.f,
        .format = "%d",
        .is_enabled = []() { return shader_injection.cs_contact_enabled >= 0.5f && shader_injection.cs_contact_denoise_enabled >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "ContactShadowsLocalEnabled",
        .binding = &shader_injection.cs_contact_local_enabled,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Local Lights",
        .section = "Contact Shadows",
        .tooltip = "Local Light Contact Shadows add subtle shadows around objects and characters where nearby lights would naturally create them, making them feel more grounded in the scene. Greyed out and off while RT Local Shadows is on.",
        .labels = {"Off", "On"},
        .is_enabled = []() { return !falcom_world::rtls::RtlsRequested(); },
        .is_visible = []() { return !IsKyoto() && !IsDaybreak2(); },
    },
    new renodx::utils::settings::Setting{
        .key = "ContactShadowsLocalPoint",
        .binding = &shader_injection.cs_contact_local_point,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Point",
        .section = "Contact Shadows",
        .tooltip = "Contact march for point lights on the specular-enabled material path. Point "
                   "lights have NO shadow map of their own in this engine, so this march is their "
                   "only contact detail. A disabled class marches zero times and spends none of the "
                   "Local Light Budget, so the classes left on get all of it.",
        .labels = {"Off", "On"},
        .is_enabled = []() { return shader_injection.cs_contact_local_enabled >= 0.5f && !falcom_world::rtls::RtlsRequested(); },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "ContactShadowsLocalSpot",
        .binding = &shader_injection.cs_contact_local_spot,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f,
        .label = "Spot",
        .section = "Contact Shadows",
        .tooltip = "Contact march for spot lights on the specular-enabled material path. Spot "
                   "lights can already carry a real shadow map (a 5-tap PCF) whenever their "
                   "shadowmapIndex is not -1, so for them the march mostly adds the near-field "
                   "detail a shadow map cannot. A disabled class marches zero times and spends "
                   "none of the Local Light Budget.",
        .labels = {"Off", "On"},
        .is_enabled = []() { return shader_injection.cs_contact_local_enabled >= 0.5f && !falcom_world::rtls::RtlsRequested(); },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "ContactShadowsLocalEnvPoint",
        .binding = &shader_injection.cs_contact_local_env_point,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Env Point",
        .section = "Contact Shadows",
        .tooltip = "Contact march for point lights on the diffuse-only material path -- surfaces "
                   "that take their specular from the environment cube instead of dynamic lights. "
                   "The march is identical to Point and costs the same. A disabled class marches "
                   "zero times and spends none of the Local Light Budget.",
        .labels = {"Off", "On"},
        .is_enabled = []() { return shader_injection.cs_contact_local_enabled >= 0.5f && !falcom_world::rtls::RtlsRequested(); },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "ContactShadowsLocalEnvSpot",
        .binding = &shader_injection.cs_contact_local_env_spot,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f,
        .label = "Env Spot",
        .section = "Contact Shadows",
        .tooltip = "Contact march for spot lights on the diffuse-only material path, the env-path "
                   "counterpart of Spot. The march is identical and costs the same. A disabled "
                   "class marches zero times and spends none of the Local Light Budget.",
        .labels = {"Off", "On"},
        .is_enabled = []() { return shader_injection.cs_contact_local_enabled >= 0.5f && !falcom_world::rtls::RtlsRequested(); },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "ContactShadowsLocalStrength",
        .binding = &shader_injection.cs_contact_local_strength,
        .default_value = 1.f,
        .label = "Local Strength",
        .section = "Contact Shadows",
        .min = 0.f,
        .max = 1.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.cs_contact_local_enabled >= 0.5f && !falcom_world::rtls::RtlsRequested(); },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        // Key renamed with the slider-to-tier change, same reasoning as
        // ContactShadowsQuality above. The default is also new: Medium (8 steps)
        // rather than the old raw slider's 4.
        .key = "ContactShadowsLocalQuality",
        .binding = &shader_injection.cs_contact_local_sample_count,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 1.f,
        .label = "Local Quality",
        .section = "Contact Shadows",
        .tooltip = "March quality per local light: Low 4, Medium 8, High 12, Ultra 16 "
                   "steps. Paid once per light, per pixel, so higher tiers cost more "
                   "here than the sun march's equivalents in a crowded scene.",
        .labels = {"Low", "Medium", "High", "Ultra"},
        .min = 0.f,
        .is_enabled = []() { return shader_injection.cs_contact_local_enabled >= 0.5f && !falcom_world::rtls::RtlsRequested(); },
        .parse = [](float value) { return ContactShadowQualitySamples(value); },
        .is_visible = []() { return !IsKyoto() && !IsDaybreak2(); },
    },
    new renodx::utils::settings::Setting{
        .key = "ContactShadowsLocalRayLength",
        .binding = &shader_injection.cs_contact_local_ray_length,
        .default_value = 2.f,
        .label = "Local Ray Length",
        .section = "Contact Shadows",
        .tooltip = "World-space ray length for the local march. Local lights are short-range, "
                   "so this is much smaller than the sun ray length above.",
        .min = 0.1f,
        .max = 20.f,
        .format = "%.1f",
        .is_enabled = []() { return shader_injection.cs_contact_local_enabled >= 0.5f && !falcom_world::rtls::RtlsRequested(); },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "ContactShadowsLocalLightClearance",
        .binding = &shader_injection.cs_contact_local_light_clearance,
        .default_value = 0.2f,
        .label = "Light Clearance",
        .section = "Contact Shadows",
        .tooltip = "Local contact rays stop this far before the light, in world units, so its own housing does not block it. Independent of RT Local Shadows.",
        .min = 0.f,
        .max = 1.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.cs_contact_local_enabled >= 0.5f && !falcom_world::rtls::RtlsRequested(); },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "ContactShadowsLocalNormalBias",
        .binding = &shader_injection.cs_contact_local_normal_bias,
        .default_value = 0.05f,
        .label = "Local Normal Bias",
        .section = "Contact Shadows",
        .tooltip = "Lifts the local light march's ray origin along the surface normal (against it for a light behind "
                   "the surface), so a lit face does not shadow itself. Separate from the sun's Normal Bias because "
                   "local rays are much shorter. 0 marches from the surface itself.",
        .min = 0.f,
        .max = 0.5f,
        .format = "%.3f",
        .is_enabled = []() { return shader_injection.cs_contact_local_enabled >= 0.5f && !falcom_world::rtls::RtlsRequested(); },
        .is_visible = []() { return IsAdvancedSettingsMode() && !IsKyoto() && !IsDaybreak2(); },
    },
    new renodx::utils::settings::Setting{
        .key = "ContactShadowsLocalMaxLights",
        .binding = &shader_injection.cs_contact_local_max_lights,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 16.f,
        .label = "Local Light Budget",
        .section = "Contact Shadows",
        .tooltip = "Per-pixel cap on how many local lights get a march, so a crowded scene "
                   "cannot multiply the cost without limit. Sora and Kai march the strongest lights first.",
        .min = 1.f,
        .max = 16.f,
        .format = "%d",
        .is_enabled = []() { return shader_injection.cs_contact_local_enabled >= 0.5f && !falcom_world::rtls::RtlsRequested(); },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "ContactShadowsLocalDenoise",
        .binding = &shader_injection.cs_contact_local_denoise_enabled,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Local Denoise",
        .section = "Contact Shadows",
        .tooltip = "Smooths the IS-FAST grain of the local light contact term with an edge-aware blur that stops at depth and normal edges. Off leaves the term exactly as marched.",
        .labels = {"Off", "On"},
        .is_enabled = []() { return shader_injection.cs_contact_local_enabled >= 0.5f && !falcom_world::rtls::RtlsRequested(); },
        .is_visible = []() { return IsAdvancedSettingsMode() && !IsKyoto() && !IsDaybreak2(); },
    },
    new renodx::utils::settings::Setting{
        .key = "ContactShadowsLocalDenoiseRadius",
        .binding = &shader_injection.cs_contact_local_denoise_radius,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 4.f,
        .label = "Local Denoise Radius",
        .section = "Contact Shadows",
        .tooltip = "Blur radius in pixels along each axis. Larger is smoother and costs more.",
        .min = 1.f,
        .max = 8.f,
        .format = "%d",
        .is_enabled = []() { return shader_injection.cs_contact_local_enabled >= 0.5f && !falcom_world::rtls::RtlsRequested() && shader_injection.cs_contact_local_denoise_enabled >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "ContactShadowsLocalDenoiseDepthSigma",
        .binding = &shader_injection.cs_contact_local_denoise_depth_sigma,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 0.02f,
        .label = "Local Denoise Depth Sigma",
        .section = "Contact Shadows",
        .tooltip = "How far a neighbour may sit off this pixel's surface plane, relative to its distance, before it stops contributing. Lower keeps depth edges sharper.",
        .min = 0.001f,
        .max = 0.2f,
        .format = "%.3f",
        .is_enabled = []() { return shader_injection.cs_contact_local_enabled >= 0.5f && !falcom_world::rtls::RtlsRequested() && shader_injection.cs_contact_local_denoise_enabled >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "ContactShadowsLocalDenoiseNormalSigma",
        .binding = &shader_injection.cs_contact_local_denoise_normal_sigma,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 0.1f,
        .label = "Local Denoise Normal Sigma",
        .section = "Contact Shadows",
        .tooltip = "How different a neighbour's surface normal may be before it stops contributing. Lower keeps creases and corners sharper.",
        .min = 0.01f,
        .max = 1.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.cs_contact_local_enabled >= 0.5f && !falcom_world::rtls::RtlsRequested() && shader_injection.cs_contact_local_denoise_enabled >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "ContactShadowsLocalDenoisePasses",
        .binding = &shader_injection.cs_contact_local_denoise_passes,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 1.f,
        .label = "Local Denoise Passes",
        .section = "Contact Shadows",
        .tooltip = "Horizontal + vertical iterations. Each extra pass widens the blur and adds the same cost again.",
        .min = 1.f,
        .max = 4.f,
        .format = "%d",
        .is_enabled = []() { return shader_injection.cs_contact_local_enabled >= 0.5f && !falcom_world::rtls::RtlsRequested() && shader_injection.cs_contact_local_denoise_enabled >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },

    // �� Dynamic Cubemaps � standalone t17 replacement ��
    new renodx::utils::settings::Setting{
        .key = "DynCubeEnabled",
        .binding = &shader_injection.dynCube_enabled,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Dynamic Cubemaps",
        .section = "Dynamic Cubemaps",
        .tooltip = "Dynamic cubemap generation replacing game's static cubemaps.",
        .labels = {"Off", "On"},
    },
    new renodx::utils::settings::Setting{
        .key = "DynCubeHistory",
        .binding = &shader_injection.dynCube_history,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Temporal",
        .section = "Dynamic Cubemaps",
        .tooltip = "Phase1: temporal accumulation. On = blend current capture with previous history. Off = no history (fresh capture each frame).",
        .labels = {"Off", "On"},
        .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DynCubeDebug",
        .binding = &shader_injection.dynCube_debug,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f,
        .label = "Debug View",
        .section = "Dynamic Cubemaps",
        .tooltip = "0=Normal (dynamic cube in lighting), 1=Show Dynamic Cube, 2=Face Visualization, 3=Solid Face Colors, 4=No Override (vanilla t17), 5=History Validity, 6=History Contribution, 7=Character Mask, 8=GGX Filtered Cube (mip-selectable), 9=SSR Result (blurred), 10=SSR Confidence, 11=Reflection Source, 12=SSR Raw, 13=SSR Edge Fade.",
        .labels = {"Normal", "Show Cube", "Face Viz", "Solid Colors", "No Override", "History Validity", "History Contribution", "Character Mask", "GGX Filtered", "SSR Result", "SSR Confidence", "Reflection Source", "SSR Raw", "SSR Edge Fade"},
        .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DynCubeResolution",
        .binding = &shader_injection.dynCube_resolution,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 4.f,
        .label = "Resolution",
        .section = "Dynamic Cubemaps",
        .tooltip = "Cubemap face size. 1536+ costs significant VRAM (a full 2048 set needs ~2GB with history+GGX). Preview rectangle is clamped for visibility.",
        .labels = {"128", "256", "512", "768", "1024", "1536", "2048"},
        .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f; },
    },
    new renodx::utils::settings::Setting{
        .key = "DynCubeCharacterCapture",
        .binding = &shader_injection.dynCube_character_capture,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f,
        .label = "Character Capture",
        .section = "Dynamic Cubemaps",
        .tooltip = "OFF = exclude characters from cubemap capture. ON = include characters in cubemap capture.",
        .labels = {"Off (Exclude)", "On (Include)"},
        .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DynCubeForceVanilla",
        .binding = &shader_injection.dynCube_force_vanilla,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f,
        .label = "Force Vanilla Cubemaps",
        .section = "Dynamic Cubemaps",
        .tooltip = "OFF = use dynamic cubemap for t17. ON = keep vanilla game cubemap for t17 (A/B test). Does not stop dynamic cubemap accumulation.",
        .labels = {"Off", "On"},
        .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DynCubeForceDynamic",
        .binding = &shader_injection.dynCube_force_dynamic,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f,
        .label = "Force Dynamic Cubemaps",
        .section = "Dynamic Cubemaps",
        .tooltip = "Debug: render the dynamic cubemap only (skip SSR and vanilla). Precedence: Force Vanilla > Force Dynamic > Force SSR.",
        .labels = {"Off", "On"},
        .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DynCubeForceSSR",
        .binding = &shader_injection.dynCube_force_ssr,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f,
        .label = "Force SSR",
        .section = "Dynamic Cubemaps",
        .tooltip = "Debug: render SSR only (skip dynamic and vanilla). Requires SSR On. Precedence: Force Vanilla > Force Dynamic > Force SSR.",
        .labels = {"Off", "On"},
        .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f && shader_injection.dynCube_ssr_enabled > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DynCubeLayerMix",
        .binding = &shader_injection.dynCube_layer_mix,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = -1.f,
        .label = "Reflection Layer Mix",
        .section = "Dynamic Cubemaps",
        .tooltip = "Manual reflection source override. -1 = automatic confidence blend. 0 = SSR only. 1 = Dynamic only. 2 = Vanilla only. Values in between blend adjacent sources.",
        .min = -1.f,
        .max = 2.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DynCubeBlur",
        .binding = &shader_injection.dynCube_blur,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 2.f,
        .label = "Dynamic Cubemap Blur",
        .section = "Dynamic Cubemaps",
        .tooltip = "Artistic mip-offset blur on the dynamic cube sample (uses the existing GGX/HW mip chain, no extra pass). 0 = normal sharpness; fractional values give smooth trilinear control; higher = progressively blurrier. Independent of material roughness.",
        .min = 0.f,
        .max = 8.f,
        .format = "%.1f",
        .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DynCubeUpdateInterval",
        .binding = &shader_injection.dynCube_capture_interval,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 3.f,
        .label = "Dynamic Cubemap Update Interval",
        .section = "Dynamic Cubemaps",
        .tooltip = "Frames between new cubemap captures (1=fastest 2-stage cadence, 2=every 2 frames, 4=every 4 frames, 8=every 8 frames). Capture and filter run on separate frames; the previous completed cube stays visible between updates.",
        .labels = {"1", "2", "3", "4", "5", "6", "7", "8"},
        .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DynCubeGGX",
        .binding = &shader_injection.dynCube_ggx,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f,
        .label = "GGX Prefilter",
        .section = "Dynamic Cubemaps",
        .tooltip = "Phase3: 16-tap Hammersley GGX prefilter for roughness mips. Off = hardware box-filter mips (cheap approx).",
        .labels = {"HW Mips", "GGX"},
        .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DynCubeDebugLog",
        .binding = &shader_injection.dynCube_debug_logging,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f,
        .label = "Debug Logging (1/sec)",
        .section = "Dynamic Cubemaps",
        .tooltip = "Emits one log per second to help diagnose which part is broken: resource creation, capture inputs (depth/color/cbv), dispatch success, t17 override, and debug mode.",
        .labels = {"Off", "On"},
        .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DynCubeGpuTimingLog",
        .binding = &g_dyncube_gpu_timing,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f,
        .label = "GPU Timing Log (1/sec)",
        .section = "Dynamic Cubemaps",
        .tooltip = "Adds the GPU time of each cubemap step (capture, filter, variant, SSR) to the once-per-second [DynCube] log line: max and average in ms over the last second, with sample and skipped counts. Results are read without waiting for the GPU. Works without Debug Logging and without Gather performance metrics. Off = nothing is timed.",
        .labels = {"Off", "On"},
        .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DynCubeCSDispatchFix",
        .binding = &g_dyncube_cs_dispatch_fix,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 2.f,
        .label = "CS Dispatch Fix",
        .section = "Dynamic Cubemaps",
        .tooltip = "Controls compute state cleanup for Dynamic Cubemap and custom SSR dispatches. "
                   "Off uses the existing compute-slot unbind. "
                   "Fix 1 restores the previous compute pipeline and descriptor tables. "
                   "Fix 2 and Fix 3 clear sampler, SRV, texture UAV and buffer UAV slots, "
                   "then restore the previous pipeline and descriptor tables.",
        .labels = {"Off", "Fix 1: Restore State", "Fix 2: Null Compute", "Fix 3: Null + Restore"},
        .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CustomShaderLogging",
        .binding = &shader_injection.custom_shader_logging,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f,
        .label = "Custom Shader Log",
        .section = "Dynamic Cubemaps",
        .tooltip = "Throttled step log for GTVBAO + Dynamic Cubemaps + SSR dispatches (sizes, inputs, create/destroy). Arm before a resolution/DLSS change repro: the last line marks the crash step.",
        .labels = {"Off", "On"},
        .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f || shader_injection.gtvbao_mode > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DynCubeCaptureBoost",
        .binding = &shader_injection.dynCube_capture_boost,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 0.66f,
        .label = "Reflection Brightness",
        .section = "Dynamic Cubemaps",
        .tooltip = "Master brightness for SSR and dynamic cubemap reflections. 1.0 = neutral. Does not affect the vanilla fallback.",
        .min = 0.f,
        .max = 4.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DynCubeCaptureSoften",
        .binding = &shader_injection.dynCube_capture_soften,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 0.4f,
        .label = "Capture Soften",
        .section = "Dynamic Cubemaps",
        .tooltip = "Softens globally-pushed dynamic reflections (glass etc.) via a dedicated variant cube. Does not affect the lighting resolve or the vanilla fallback.",
        .min = 0.f,
        .max = 1.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DynCubeGlobalStrength",
        .binding = &shader_injection.dynCube_global_strength,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 0.4f,
        .label = "Global Reflection Strength",
        .section = "Dynamic Cubemaps",
        .tooltip = "Scales globally-pushed dynamic reflections (glass etc.) so they don't dominate. 1 = full. Does not affect the lighting resolve or the vanilla fallback.",
        .min = 0.f,
        .max = 1.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DynCubeHistoryBlend",
        .binding = &shader_injection.dynCube_history_blend,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 0.75f,
        .label = "History Blend",
        .section = "Dynamic Cubemaps",
        .tooltip = "Temporal blend weight when current and previous samples are compatible. 0.5 = Skyrim-style 50/50.",
        .min = 0.f,
        .max = 1.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f && shader_injection.dynCube_history > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DynCubeHistoryPosThreshold",
        .binding = &shader_injection.dynCube_history_pos_threshold,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 0.5f,
        .label = "History Pos Threshold",
        .section = "Dynamic Cubemaps",
        .tooltip = "World-unit position compatibility threshold. If previous vs current reconstructed position differs more than this, history is replaced. Starting point 0.5; tune after 90-deg and slow-pan tests.",
        .min = 0.f,
        .max = 20.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f && shader_injection.dynCube_history > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DynCubeDebugFace",
        .binding = &shader_injection.dynCube_debug_face,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f,
        .label = "Debug Face",
        .section = "Dynamic Cubemaps",
        .tooltip = "Face for debug preview when Debug View=1/2/5/6/7/8: 0=+X 1=-X 2=+Y 3=-Y 4=+Z 5=-Z. Preview samples the capture resource directly.",
        .labels = {"+X", "-X", "+Y", "-Y", "+Z", "-Z"},
        .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f && (shader_injection.dynCube_debug == 1.f || shader_injection.dynCube_debug == 2.f || shader_injection.dynCube_debug == 5.f || shader_injection.dynCube_debug == 6.f || shader_injection.dynCube_debug == 7.f || shader_injection.dynCube_debug == 8.f); },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DynCubeDebugMip",
        .binding = &shader_injection.dynCube_debug_mip,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f,
        .label = "Debug Mip",
        .section = "Dynamic Cubemaps",
        .tooltip = "Mip level for the GGX filtered cube preview when Debug View=8. 0=sharp history, 7=strongly blurred.",
        .labels = {"0", "1", "2", "3", "4", "5", "6", "7"},
        .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f && shader_injection.dynCube_debug == 8.f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DynCubeForceMip",
        .binding = &shader_injection.dynCube_force_mip,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = -1.f,
        .label = "Force Cubemap Mip",
        .section = "Dynamic Cubemaps",
        .tooltip = "Debug: force the t17 reflection mip. -1 = normal roughness LOD. 0..7 = always sample that mip (verifies the GGX chain reaches the reflection).",
        .min = -1.f,
        .max = 7.f,
        .format = "%d",
        .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DynCubeCoverageFade",
        .binding = &shader_injection.dynCube_coverage_fade,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Dynamic Cubemap Coverage Fade",
        .section = "Dynamic Cubemaps",
        .tooltip = "OFF = hard validity cutoff (current). ON = smooth the dynamic-cubemap validity edge in direction space so coverage boundaries fade into the fallback instead of cutting.",
        .labels = {"Off", "On"},
        .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DynCubeCoverageWidth",
        .binding = &shader_injection.dynCube_coverage_width,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 1.f,
        .label = "Dynamic Cubemap Coverage Fade Width",
        .section = "Dynamic Cubemaps",
        .tooltip = "Angular width of the validity smoothing cone in degrees. Controls transition softness only, not the valid boundary. 0 = binary cutoff.",
        .min = 0.f,
        .max = 8.f,
        .format = "%.1f",
        .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f && shader_injection.dynCube_coverage_fade > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DynCubeVerticalOffset",
        .binding = &shader_injection.dynCube_vertical_offset,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 0.f,
        .label = "Dynamic Cubemap Vertical Offset (Test)",
        .section = "Dynamic Cubemaps",
        .tooltip = "TEST: vertically tilt the dynamic cubemap lookup, in degrees. 0 = no change. Positive slides reflection content down, negative slides it up. Validity and vanilla fallback follow the shifted direction.",
        .min = -30.f,
        .max = 30.f,
        .format = "%.1f",
        .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DynCubeSSR",
        .binding = &shader_injection.dynCube_ssr_enabled,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "SSR",
        .section = "Dynamic Cubemaps",
        .tooltip = "Simple screen-space SSR ray march (no Hi-Z/temporal). Confidence blend: SSR > Dynamic Cube > Vanilla Cube.",
        .labels = {"Off", "On"},
        .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DynCubeSSRSamples",
        .binding = &shader_injection.dynCube_ssr_samples,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 16.f,
        .label = "SSR Sample Count",
        .section = "Dynamic Cubemaps",
        .tooltip = "SSR ray-march sample count. Higher = more accurate at higher GPU cost. Old Medium preset = 16.",
        .min = 4.f,
        .max = 96.f,
        .format = "%d",
        .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f && shader_injection.dynCube_ssr_enabled > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DynCubeSSRDistance",
        .binding = &shader_injection.dynCube_ssr_distance,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 192.f,
        .label = "SSR Search Distance",
        .section = "Dynamic Cubemaps",
        .tooltip = "SSR ray-march search distance in world units. Old Medium preset = 20.",
        .min = 4.f,
        .max = 192.f,
        .format = "%.1f",
        .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f && shader_injection.dynCube_ssr_enabled > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DynCubeSSRBlur",
        .binding = &shader_injection.dynCube_ssr_blur,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 4.f,
        .label = "SSR Blur",
        .section = "Dynamic Cubemaps",
        .tooltip = "Separable Gaussian blur sigma on the SSR result. 0 = sharp/raw SSR, higher = progressively blurrier.",
        .min = 0.f,
        .max = 8.f,
        .format = "%.1f",
        .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f && shader_injection.dynCube_ssr_enabled > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DynCubeSSRSymmetricWeights",
        .binding = &shader_injection.dynCube_ssr_symmetric_weights,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "SSR Symmetric Blur Weights (Test)",
        .section = "Dynamic Cubemaps",
        .tooltip = "TEST A/B: evaluate each blur tap pair with a single shared Gaussian weight. Off = legacy per-tap loop (default, unchanged behavior).",
        .labels = {"Off", "On"},
        .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f && shader_injection.dynCube_ssr_enabled > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DynCubeSSRDistanceFade",
        .binding = &shader_injection.dynCube_ssr_distance_fade,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 0.f,
        .label = "SSR Distance Fade",
        .section = "Dynamic Cubemaps",
        .tooltip = "How fast SSR confidence drops with hit distance. 0 = no falloff, 1 = full falloff at max distance.",
        .min = 0.f,
        .max = 1.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f && shader_injection.dynCube_ssr_enabled > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DynCubeSSREdgeFade",
        .binding = &shader_injection.dynCube_ssr_edge_fade,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 0.2f,
        .label = "SSR Edge Fade",
        .section = "Dynamic Cubemaps",
        .tooltip = "Screen-edge vignette strength on SSR confidence. 0 = none, 1 = fades over 25% from each edge.",
        .min = 0.f,
        .max = 1.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f && shader_injection.dynCube_ssr_enabled > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DynCubeSSRGrazingFade",
        .binding = &shader_injection.dynCube_ssr_grazing_fade,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 0.f,
        .label = "SSR Grazing Fade",
        .section = "Dynamic Cubemaps",
        .tooltip = "Reduces SSR confidence at grazing view angles. 0 = none, 1 = aggressive fade.",
        .min = 0.f,
        .max = 1.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f && shader_injection.dynCube_ssr_enabled > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DynCubeSSRThickness",
        .binding = &shader_injection.dynCube_ssr_thickness,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 1.0f,
        .label = "SSR Thickness",
        .section = "Dynamic Cubemaps",
        .tooltip = "Depth tolerance: how far behind the surface the ray must be to count as a hit (world units).",
        .min = 0.01f,
        .max = 1.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f && shader_injection.dynCube_ssr_enabled > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DynCubeSSRCharOccStrength",
        .binding = &shader_injection.dynCube_ssr_char_occ_strength,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 0.f,
        .label = "SSR Character Occlusion Reduction",
        .section = "Dynamic Cubemaps",
        .tooltip = "How much SSR confidence is reduced when the ray hits a character from a horizontal (floor/water) surface. Attacks third-person disocclusion rings; 0 = off. Vertical mirrors (low upness) are unaffected.",
        .min = 0.f,
        .max = 1.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f && shader_injection.dynCube_ssr_enabled > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DynCubeSSRCharOccUpness",
        .binding = &shader_injection.dynCube_ssr_char_occ_upness,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 0.f,
        .label = "SSR Character Occlusion Upness",
        .section = "Dynamic Cubemaps",
        .tooltip = "Surface upness threshold where the character-hit confidence reduction begins (smooth �0.25 band). 0 = any up-facing surface, 0.5 = mostly horizontal, 1 = only fully horizontal (floor/water).",
        .min = 0.f,
        .max = 1.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f && shader_injection.dynCube_ssr_enabled > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DynCubeSSRConfidenceFallback",
        .binding = &shader_injection.dynCube_ssr_confidence_fallback,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 0.f,
        .label = "SSR Confidence Fallback",
        .section = "Dynamic Cubemaps",
        .tooltip = "AUTO-mode remap of SSR confidence: 0 = today's weighting; higher values suppress low-confidence SSR sooner so Dynamic Cubemap takes over earlier. Smooth, no hard cutoff; conf = 1 always stays full SSR.",
        .min = 0.f,
        .max = 0.9f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f && shader_injection.dynCube_ssr_enabled > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DynCubeSSRReplacement",
        .binding = &shader_injection.dynCube_ssr_replacement,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "SSR Replacement",
        .section = "Dynamic Cubemaps",
        .tooltip = "Replace the Sora (1st/2nd) / Kai water SSR resolve with the DynCube composite (game march + dynamic cubemap, misses decay with no vanilla-cube tint) on water-flagged pixels only. Bed and non-water pixels run verbatim vanilla. Off = fully vanilla SSR chain, nothing touched.",
        .labels = {"Off", "On"},
        .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DynCubeGameSSR",
        .binding = &shader_injection.dynCube_game_ssr,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Game SSR",
        .section = "Dynamic Cubemaps",
        .tooltip = "Run the game's own SSR march and feed it to the SSR Replacement composite (game reflection, then dynamic cubemap, misses decay with no vanilla-cube tint). Off skips the march: water receives dynamic cubemap only. Only applies while SSR Replacement is on; otherwise the vanilla chain runs untouched.",
        .labels = {"Off", "On"},
        .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f && shader_injection.dynCube_ssr_replacement > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DynCubeSSRReplacementDebug",
        .binding = &shader_injection.dynCube_ssr_replacement_debug,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f,
        .label = "SSR Replacement Debug",
        .section = "Dynamic Cubemaps",
        .tooltip = "Visualize the SSR chain stages fullscreen on lit surfaces (Off, ssr1 input scene color, ssr1 march result, lighting SSR texture, lighting color). For judging what each stage sees.",
        .labels = {"Off", "SSR1 Input", "SSR2 Input", "Lighting SSR", "Lighting Color"},
        .min = 0.f,
        .max = 4.f,
        .format = "%d",
        .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    // -- Vanilla SSR Improvements (Sora2nd march/denoise correctness, A/B) --
    new renodx::utils::settings::Setting{
        .key = "DynCubeVanillaSSREnabled",
        .binding = &shader_injection.dynCube_vanilla_ssr_enabled,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Enable Improvements",
        .section = "Vanilla SSR Improvements",
        .tooltip = "Improvements to Vanilla SSR denoising.",
        .labels = {"Off", "On"},
    },
    new renodx::utils::settings::Setting{
        .key = "DynCubeVanillaRefineFix",
        .binding = &shader_injection.dynCube_vanilla_refine_fix,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Refine Backtrack Fix",
        .section = "Vanilla SSR Improvements",
        .tooltip = "Fix SSR refinement to bracket the crossing (step back when inside, Kai-style) instead of stepping only forward. Sora2nd ssr1, Sora1st and Kai fused marches. Off restores the verbatim vanilla behavior for A/B.",
        .labels = {"Off", "On"},
        .is_enabled = []() { return shader_injection.dynCube_vanilla_ssr_enabled > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DynCubeVanillaRefineThreshold",
        .binding = &shader_injection.dynCube_vanilla_refine_threshold,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 0.5f,
        .label = "Refine Hit Threshold",
        .section = "Vanilla SSR Improvements",
        .tooltip = "Hit threshold on the refine depth delta (scene depth minus ray depth, view units): hit when delta exceeds this. 0 = any penetration counts (native Kai rule); higher is stricter (fewer, firmer hits). Sora2nd ssr1, Sora1st and Kai fused marches. Only applies while Refine Backtrack Fix is on.",
        .min = 0.f,
        .max = 1.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.dynCube_vanilla_ssr_enabled > 0.5f && shader_injection.dynCube_vanilla_refine_fix > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DynCubeVanillaHistoryFixed",
        .binding = &shader_injection.dynCube_vanilla_history_fixed,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f,
        .label = "Fixed History Blend",
        .section = "Vanilla SSR Improvements",
        .tooltip = "On = fixed history weight from the slider below (0.9 = vanilla); Off = motion-adaptive weighting (Kai formula: mostly history when static, mostly current when moving).",
        .labels = {"Off", "On"},
        .is_enabled = []() { return shader_injection.dynCube_vanilla_ssr_enabled > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DynCubeVanillaHistoryWeight",
        .binding = &shader_injection.dynCube_vanilla_history_weight,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 0.9f,
        .label = "Fixed History Weight",
        .section = "Vanilla SSR Improvements",
        .tooltip = "History fraction used when Fixed History Blend is on (0.9 = vanilla). Higher = longer trails but stabler; lower = fresher but noisier.",
        .min = 0.5f,
        .max = 0.99f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.dynCube_vanilla_ssr_enabled > 0.5f && shader_injection.dynCube_vanilla_history_fixed > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DynCubeVanillaDisocReject",
        .binding = &shader_injection.dynCube_vanilla_disoc_reject,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Disocclusion Reject",
        .section = "Vanilla SSR Improvements",
        .tooltip = "Validate the reprojected history sample against current-frame scene data (UV bounds, motion distance, depth mismatch); on mismatch use the current SSR result instead of stale history.",
        .labels = {"Off", "On"},
        .is_enabled = []() { return shader_injection.dynCube_vanilla_ssr_enabled > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DynCubeVanillaDisocDepth",
        .binding = &shader_injection.dynCube_vanilla_disoc_depth,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 0.15f,
        .label = "Disocclusion Depth Threshold",
        .section = "Vanilla SSR Improvements",
        .tooltip = "Reject history when |linear depth at history UV minus linear depth here| exceeds this (view units). 0 = always reject on depth. Only applies while Disocclusion Reject is on.",
        .min = 0.f,
        .max = 2.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.dynCube_vanilla_ssr_enabled > 0.5f && shader_injection.dynCube_vanilla_disoc_reject > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DynCubeVanillaDisocUV",
        .binding = &shader_injection.dynCube_vanilla_disoc_uv,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 0.05f,
        .label = "Disocclusion Motion Threshold",
        .section = "Vanilla SSR Improvements",
        .tooltip = "Reject history when reprojection motion exceeds this (0-1 UV units, ~5% of screen at default). 0 = any motion rejects. Only applies while Disocclusion Reject is on.",
        .min = 0.f,
        .max = 0.25f,
        .format = "%.3f",
        .is_enabled = []() { return shader_injection.dynCube_vanilla_ssr_enabled > 0.5f && shader_injection.dynCube_vanilla_disoc_reject > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DynCubeVanillaISFAST",
        .binding = &shader_injection.dynCube_vanilla_isfast,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "IS-FAST History Distribution",
        .section = "Vanilla SSR Improvements",
        .tooltip = "Blue-noise subpixel distribution of the single history tap (same 128x128x32 volume, frame slice, spatial scale and strength blend as the custom SSR IS-FAST phase; strength 0 = off). Gives TAA/upscalers a temporally distributed signal. Requires IS-FAST noise available: turn on ISFASTMasterEnable and check the IS-FAST debug log for loaded=yes; the effect is temporal stability in motion, not single-frame denoising.",
        .labels = {"Off", "On"},
        .is_enabled = []() { return shader_injection.dynCube_vanilla_ssr_enabled > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    // -- Character Outline (Sora 1st / 2nd) --
    new renodx::utils::settings::Setting{
        .key = "CharacterOutlineThickness",
        .binding = &shader_injection.character_outline_thickness,
        .default_value = 100.f,
        .label = "Outline Thickness",
        .section = "Character Outline",
        .tooltip = "Scales outline sampling radii in Sora 1st / 2nd. 0% disables outlines; 100% preserves the original width. Independent of Outline Intensity.",
        .min = 0.f,
        .max = 100.f,
        .format = "%.1f%%",
        .parse = [](float value) { return value * 0.01f; },
        .is_visible = []() { return IsSora1st() || IsSora2nd(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CharOutlineIntensity",
        .binding = &shader_injection.char_outline_intensity,
        .default_value = 1.f,
        .label = "Outline Intensity",
        .section = "Character Outline",
        .tooltip = "Scales the character outline blend weight before saturate. 0 = no outline, 1 = vanilla strength.",
        .min = 0.f,
        .max = 1.f,
        .format = "%.2f",
        .is_visible = []() { return IsSora1st() || IsSora2nd(); },
    },
    // -- Custom TAA (Sora 1st / 2nd, from-scratch replacement) --
    new renodx::utils::settings::Setting{
        .key = "CustomTAAEnabled",
        .binding = &shader_injection.custom_taa_enabled,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Enable Custom TAA",
        .section = "Custom TAA",
        .tooltip = "Smooths jagged edges and flickering while moving. On = cleaner custom anti-aliasing. Off = the game's original anti-aliasing.",
        .labels = {"Off", "On"},
        .is_visible = []() { return IsSora1st() || IsSora2nd(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CustomTAAHistoryFilter",
        .binding = &shader_injection.custom_taa_history_filter,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 2.f,
        .label = "History Filter",
        .section = "Custom TAA",
        .tooltip = "History reconstruction: Bilinear (TAA-0 baseline), Adaptive high-order (edge-gated bicubic), Full bicubic.",
        .labels = {"Bilinear", "Adaptive", "Full"},
        .is_enabled = []() { return shader_injection.custom_taa_enabled > 0.5f; },
        .is_visible = []() { return (IsSora1st() || IsSora2nd()) && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CustomTAAClipMode",
        .binding = &shader_injection.custom_taa_clip_mode,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 2.f,
        .label = "History Validation",
        .section = "Custom TAA",
        .tooltip = "History validation: None (TAA-0/1), RGB AABB baseline (TAA-2), k-DOP polytope clip (TAA-3).",
        .labels = {"None", "AABB", "k-DOP"},
        .is_enabled = []() { return shader_injection.custom_taa_enabled > 0.5f; },
        .is_visible = []() { return (IsSora1st() || IsSora2nd()) && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CustomTAAStaticFeedback",
        .binding = &shader_injection.custom_taa_static_feedback,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 0.90f,
        .label = "Static Feedback",
        .section = "Custom TAA",
        .tooltip = "TAA-5 history weight for static pixels: result = lerp(current, history, feedback). High values accumulate many frames on stable pixels.",
        .min = 0.f,
        .max = 0.99f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.custom_taa_enabled > 0.5f; },
        .is_visible = []() { return (IsSora1st() || IsSora2nd()) && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CustomTAADynamicFeedback",
        .binding = &shader_injection.custom_taa_dynamic_feedback,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 0.5f,
        .label = "Dynamic Feedback",
        .section = "Custom TAA",
        .tooltip = "TAA-5 history weight for fast-moving pixels. Lower values cut trailing/temporal softness during motion.",
        .min = 0.f,
        .max = 0.99f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.custom_taa_enabled > 0.5f; },
        .is_visible = []() { return (IsSora1st() || IsSora2nd()) && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CustomTAAMotionScale",
        .binding = &shader_injection.custom_taa_motion_scale,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 0.66f,
        .label = "Motion Scale",
        .section = "Custom TAA",
        .tooltip = "TAA-5 pixels-to-weight scale: weight = saturate(motionPixels * scale). Motion is in pixels/frame and includes the subpixel jitter delta, so the default is small: ~1px stays near Static, ~4px blends halfway, ~8px reaches Dynamic. Do not change during the first test pass.",
        .min = 0.f,
        .max = 2.f,
        .format = "%.3f",
        .is_enabled = []() { return shader_injection.custom_taa_enabled > 0.5f; },
        .is_visible = []() { return (IsSora1st() || IsSora2nd()) && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CustomTAASquaredMotionResponse",
        .binding = &shader_injection.custom_taa_squared_motion_response,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Squared Motion Response",
        .section = "Custom TAA",
        .tooltip = "Motion-weight curve A/B: Off = linear (baseline), On = squared (same static/dynamic endpoints, more history at low/moderate motion). Applies to feedback and silhouette rejection together.",
        .labels = {"Off", "On"},
        .is_enabled = []() { return shader_injection.custom_taa_enabled > 0.5f; },
        .is_visible = []() { return (IsSora1st() || IsSora2nd()) && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CustomTAADetailRestore",
        .binding = &shader_injection.custom_taa_detail_restore,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 0.25f,
        .label = "Detail Restore",
        .section = "Custom TAA",
        .tooltip = "Feedback restoration experiment: restores history feedback toward Static on high-relative-detail pixels whose history agrees with current (flat or disagreeing pixels unchanged). 0 = prior behavior exactly; test value 8.0. Requires validation on.",
        .min = 0.f,
        .max = 32.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.custom_taa_enabled > 0.5f && shader_injection.custom_taa_clip_mode > 0.5f; },
        .is_visible = []() { return (IsSora1st() || IsSora2nd()) && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CustomTAADetailTarget",
        .binding = &shader_injection.custom_taa_detail_target,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 0.99f,
        .label = "Detail Target",
        .section = "Custom TAA",
        .tooltip = "Selective above-static accumulation experiment: high-detail, low-motion, history-agreeing pixels (and silhouette-valid) approach this feedback (~20-frame window at 0.95) without giving that window to the whole image. Equal to Static = off (exact prior behavior); test 0.95 against Static 0.85. Requires validation on.",
        .min = 0.85f,
        .max = 0.99f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.custom_taa_enabled > 0.5f && shader_injection.custom_taa_clip_mode > 0.5f; },
        .is_visible = []() { return (IsSora1st() || IsSora2nd()) && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CustomTAAOvershootSoftness",
        .binding = &shader_injection.custom_taa_overshoot_softness,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 1.0f,
        .label = "Overshoot Softness",
        .section = "Custom TAA",
        .tooltip = "Selective-tolerance experiment (keep epsilon at 0.01 for A/B): history near the k-DOP hull keeps feedback and averages out flicker; history far outside loses feedback and stays responsive. 100 ~= prior behavior (not bit-exact), 1.0 = experiment default, 0.5 = stronger rejection.",
        .min = 0.25f,
        .max = 100.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.custom_taa_enabled > 0.5f && shader_injection.custom_taa_clip_mode > 0.5f; },
        .is_visible = []() { return (IsSora1st() || IsSora2nd()) && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CustomTAASilhouetteRejection",
        .binding = &shader_injection.custom_taa_silhouette_rejection,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 1.f,
        .label = "Silhouette Rejection",
        .section = "Custom TAA",
        .tooltip = "Wrong-surface history experiment: attenuates history feedback where depth discontinuity meets motion (disocc = 1 - motionWeight * saturate(spread * k)). Static detail keeps history regardless of depth edges. 0 = prior behavior exactly.",
        .min = 0.f,
        .max = 16.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.custom_taa_enabled > 0.5f; },
        .is_visible = []() { return (IsSora1st() || IsSora2nd()) && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CustomTAAKDOPAxes",
        .binding = &shader_injection.custom_taa_kdop_axes,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 1.f,
        .label = "k-DOP Axis Count",
        .section = "Custom TAA",
        .tooltip = "k-DOP polytope density: 16-DOP General subset (default, first 8 axes of the paper's 32-DOP set), 22-DOP Paper lightweight set, 32-DOP Paper full set. The paper's scene-optimized 16-DOP sets are future experiments.",
        .labels = {"16-DOP General", "22-DOP Paper", "32-DOP Paper"},
        .is_enabled = []() { return shader_injection.custom_taa_enabled > 0.5f && shader_injection.custom_taa_clip_mode > 1.5f; },
        .is_visible = []() { return (IsSora1st() || IsSora2nd()) && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CustomTAADmin",
        .binding = &shader_injection.custom_taa_dmin,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 0.048f,
        .label = "Adaptive Threshold",
        .section = "Custom TAA",
        .tooltip = "Edge D-term gate for adaptive history filtering (paper start point, tune per game). Below this: bilinear; above: full bicubic correction.",
        .min = 0.f,
        .max = 0.5f,
        .format = "%.3f",
        .is_enabled = []() { return shader_injection.custom_taa_enabled > 0.5f && shader_injection.custom_taa_history_filter > 0.5f && shader_injection.custom_taa_history_filter < 1.5f; },
        .is_visible = []() { return (IsSora1st() || IsSora2nd()) && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CustomTAAKDOPEpsilon",
        .binding = &shader_injection.custom_taa_kdop_epsilon,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 0.05f,
        .label = "k-DOP Epsilon",
        .section = "Custom TAA",
        .tooltip = "Slab extent padding for k-DOP clipping (paper default 1e-5). Investigation range: higher values retain more history (reduces shadow jitter but risks stale-history drag on highlights).",
        .min = 0.f,
        .max = 0.1f,
        .format = "%.5f",
        .is_enabled = []() { return shader_injection.custom_taa_enabled > 0.5f && shader_injection.custom_taa_clip_mode > 1.5f; },
        .is_visible = []() { return (IsSora1st() || IsSora2nd()) && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CustomTAADebug",
        .binding = &shader_injection.custom_taa_debug,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f,
        .label = "Debug View",
        .section = "Custom TAA",
        .tooltip = "Dev only: Normal, History only (is reconstruction working?), Clip factor (is k-DOP rejecting?), Adapt-Bilin x20 (where does adaptive differ from bilinear?), Full-Bilin x20 (where does full bicubic differ?).",
        .labels = {"Normal", "History", "Clip", "Adapt-Bilin", "Full-Bilin"},
        .is_enabled = []() { return shader_injection.custom_taa_enabled > 0.5f; },
        .is_visible = []() { return (IsSora1st() || IsSora2nd()) && IsAdvancedSettingsMode(); },
    },
    // -- RCAS post-TAA sharpening (dedicated pass; never feeds TAA history) --
    new renodx::utils::settings::Setting{
        .key = "RCASEnabled",
        .binding = &shader_injection.rcas_enabled,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Enable RCAS",
        .section = "Custom TAA",
        .tooltip = "Turns image sharpening on or off.",
        .labels = {"Off", "On"},
        .is_enabled = []() { return shader_injection.custom_taa_enabled > 0.5f; },
        .is_visible = []() { return IsSora1st() || IsSora2nd(); },
    },
    new renodx::utils::settings::Setting{
        .key = "RCASStrength",
        .binding = &shader_injection.rcas_strength,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 0.3f,
        .label = "Sharpening",
        .section = "Custom TAA",
        .tooltip = "How crisp the image looks. Higher values give a sharper picture but can cause glowing edges if set too high. 0 turns sharpening off.",
        .min = 0.f,
        .max = 1.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.custom_taa_enabled > 0.5f && shader_injection.rcas_enabled > 0.5f; },
        .is_visible = []() { return IsSora1st() || IsSora2nd(); },
    },
    new renodx::utils::settings::Setting{
        .key = "RCASMotionOn",
        .binding = &shader_injection.rcas_motion_on,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Motion Sharpening",
        .section = "Custom TAA",
        .tooltip = "Automatically strengthens sharpening while the camera or objects move, then relaxes it back when the image is still. Off = always use the Sharpening value above.",
        .labels = {"Off", "On"},
        .is_enabled = []() { return shader_injection.custom_taa_enabled > 0.5f && shader_injection.rcas_enabled > 0.5f; },
        .is_visible = []() { return (IsSora1st() || IsSora2nd()) && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "RCASMotionMultiplier",
        .binding = &shader_injection.rcas_motion_multiplier,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 4.0f,
        .label = "Motion Sharpening Multiplier",
        .section = "Custom TAA",
        .tooltip = "Scales the Sharpening value during fast movement: motion value = base x multiplier, clamped to Max Motion Sharpening below. Example: Sharpening 0.30 with this at 2.0 means a still image uses 0.30 and fast motion uses 0.60.",
        .min = 1.f,
        .max = 10.f,
        .format = "%.1f",
        .is_enabled = []() { return shader_injection.custom_taa_enabled > 0.5f && shader_injection.rcas_enabled > 0.5f && shader_injection.rcas_motion_on > 0.5f; },
        .is_visible = []() { return (IsSora1st() || IsSora2nd()) && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "RCASMotionMax",
        .binding = &shader_injection.rcas_motion_max,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 1.0f,
        .label = "Max Motion Sharpening",
        .section = "Custom TAA",
        .tooltip = "Clamps the motion sharpening value from above. May exceed 1 for stronger motion sharpening (watch for ringing/halos at high values). Example: Sharpening 0.30 with 10x multiplier reaches 3.00 but is clamped to this value.",
        .min = 0.f,
        .max = 3.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.custom_taa_enabled > 0.5f && shader_injection.rcas_enabled > 0.5f && shader_injection.rcas_motion_on > 0.5f; },
        .is_visible = []() { return (IsSora1st() || IsSora2nd()) && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "RCASMotionThreshold",
        .binding = &shader_injection.rcas_motion_threshold,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 0.01f,
        .label = "Motion Threshold",
        .section = "Custom TAA",
        .tooltip = "How much movement is ignored before motion sharpening kicks in. Filters out tiny per-frame jitter so a still camera keeps normal sharpening instead of flickering stronger.",
        .min = 0.f,
        .max = 3.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.custom_taa_enabled > 0.5f && shader_injection.rcas_enabled > 0.5f && shader_injection.rcas_motion_on > 0.5f; },
        .is_visible = []() { return (IsSora1st() || IsSora2nd()) && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "RCASMotionRange",
        .binding = &shader_injection.rcas_motion_range,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 0.25f,
        .label = "Motion Range",
        .section = "Custom TAA",
        .tooltip = "How much more movement it takes to ramp from normal sharpening up to the full motion value. Lower = reaches full strength sooner.",
        .min = 0.25f,
        .max = 8.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.custom_taa_enabled > 0.5f && shader_injection.rcas_enabled > 0.5f && shader_injection.rcas_motion_on > 0.5f; },
        .is_visible = []() { return (IsSora1st() || IsSora2nd()) && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "RCASMotionResponse",
        .binding = &shader_injection.rcas_motion_response,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 0.5f,
        .label = "Motion Response",
        .section = "Custom TAA",
        .tooltip = "Shape of the ramp from normal to full motion sharpening. 1.0 = even ramp; higher values stay gentle longer, then climb steeply near full motion.",
        .min = 0.5f,
        .max = 3.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.custom_taa_enabled > 0.5f && shader_injection.rcas_enabled > 0.5f && shader_injection.rcas_motion_on > 0.5f; },
        .is_visible = []() { return (IsSora1st() || IsSora2nd()) && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "RCASDebug",
        .binding = &shader_injection.rcas_debug,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f,
        .label = "RCAS Debug View",
        .section = "Custom TAA",
        .tooltip = "Diagnostic only: Motion Heat shows where motion sharpening engages (green = base strength, yellow = partial, red = full motion target). Toggle back to Normal for image-quality testing (heat frames pollute TAA history while active).",
        .labels = {"Normal", "Motion Heat"},
        .is_enabled = []() { return shader_injection.custom_taa_enabled > 0.5f && shader_injection.rcas_enabled > 0.5f; },
        .is_visible = []() { return (IsSora1st() || IsSora2nd()) && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "RCASDenoise",
        .binding = &shader_injection.rcas_denoise,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "RCAS Denoise",
        .section = "Custom TAA",
        .tooltip = "Reference FSR_RCAS_DENOISE behavior: scales the lobe by the noise term (0.5..1.0), reducing sharpening where local variation looks grain-like. Off = reference default (full sharpening). May also soften legitimate fine detail such as grass/foliage texture.",
        .labels = {"Off", "On"},
        .is_enabled = []() { return shader_injection.custom_taa_enabled > 0.5f && shader_injection.rcas_enabled > 0.5f; },
        .is_visible = []() { return (IsSora1st() || IsSora2nd()) && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "FXAAEnabled",
        .binding = &shader_injection.fxaa_enabled,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Enable FXAA",
        .section = "Custom TAA",
        .tooltip = "Morphological anti-aliasing between TAA and sharpening. Smooths leftover jagged edges the temporal pass misses. Off = exact prior behavior (RCAS reads the TAA output directly). Never feeds TAA history.",
        .labels = {"Off", "On"},
        .is_enabled = []() { return shader_injection.custom_taa_enabled > 0.5f; },
        .is_visible = []() { return (IsSora1st() || IsSora2nd()) && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "FXAAQuality",
        .binding = &shader_injection.fxaa_quality,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 1.f,
        .label = "FXAA Quality",
        .section = "Custom TAA",
        .tooltip = "Reference FXAA 3.11 quality preset. Standard (12) is the reference default; High (29) searches longer edges at a small extra cost.",
        .labels = {"Standard", "High"},
        .is_enabled = []() { return shader_injection.custom_taa_enabled > 0.5f && shader_injection.fxaa_enabled > 0.5f; },
        .is_visible = []() { return (IsSora1st() || IsSora2nd()) && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "FXAASubpix",
        .binding = &shader_injection.fxaa_subpix,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 0.33f,
        .label = "FXAA Subpixel",
        .section = "Custom TAA",
        .tooltip = "Amount of sub-pixel aliasing removal. Higher softens more (1.00 = softest), lower stays sharper (0.00 = off). Reference default 0.75.",
        .min = 0.f,
        .max = 1.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.custom_taa_enabled > 0.5f && shader_injection.fxaa_enabled > 0.5f; },
        .is_visible = []() { return (IsSora1st() || IsSora2nd()) && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "FXAAEdgeThreshold",
        .binding = &shader_injection.fxaa_edge_threshold,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 0.166f,
        .label = "FXAA Edge Threshold",
        .section = "Custom TAA",
        .tooltip = "Minimum local contrast required before FXAA engages. Lower catches more edges but costs more (reference: 0.333 faster, 0.250 low, 0.166 default, 0.125 high, 0.063 overkill).",
        .min = 0.031f,
        .max = 0.333f,
        .format = "%.3f",
        .is_enabled = []() { return shader_injection.custom_taa_enabled > 0.5f && shader_injection.fxaa_enabled > 0.5f; },
        .is_visible = []() { return (IsSora1st() || IsSora2nd()) && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "FXAAEdgeThresholdMin",
        .binding = &shader_injection.fxaa_edge_threshold_min,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 0.0625f,
        .label = "FXAA Edge Threshold Min",
        .section = "Custom TAA",
        .tooltip = "Trims FXAA from processing darks. Lower processes more dark detail but costs more (reference: 0.0833 default, 0.0625 high quality, 0.0312 visible limit). 0 = process darks fully.",
        .min = 0.f,
        .max = 0.0833f,
        .format = "%.4f",
        .is_enabled = []() { return shader_injection.custom_taa_enabled > 0.5f && shader_injection.fxaa_enabled > 0.5f; },
        .is_visible = []() { return (IsSora1st() || IsSora2nd()) && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DynCubeSSRISFAST",
        .binding = &shader_injection.dynCube_ssr_isfast_enabled,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "SSR IS-FAST Phase",
        .section = "Dynamic Cubemaps",
        .tooltip = "Use IS-FAST spatio-temporal blue noise for the SSR ray-march start phase (breaks up stride banding). Off = deterministic hash phase. Requires the master IS-FAST toggle.",
        .labels = {"Off", "On"},
        .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f && shader_injection.dynCube_ssr_enabled > 0.5f && g_isfast_enabled > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DynCubeSSRISFASTStrength",
        .binding = &shader_injection.dynCube_ssr_isfast_strength,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 1.f,
        .label = "SSR IS-FAST Strength",
        .section = "Dynamic Cubemaps",
        .tooltip = "Blend between the deterministic hash phase (0) and full IS-FAST noise phase (1).",
        .min = 0.f,
        .max = 1.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f && shader_injection.dynCube_ssr_enabled > 0.5f && g_isfast_enabled > 0.5f && shader_injection.dynCube_ssr_isfast_enabled > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DynCubeSSRISFASTSpatial",
        .binding = &shader_injection.dynCube_ssr_isfast_spatial,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 1.f,
        .label = "SSR IS-FAST Spatial Scale",
        .section = "Dynamic Cubemaps",
        .tooltip = "Scale IS-FAST noise spatial frequency for SSR. <1 zooms in (smoother), >1 adds more detail.",
        .min = 0.25f,
        .max = 4.0f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f && shader_injection.dynCube_ssr_enabled > 0.5f && g_isfast_enabled > 0.5f && shader_injection.dynCube_ssr_isfast_enabled > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DynCubeSSRISFASTTemporal",
        .binding = &shader_injection.dynCube_ssr_isfast_temporal,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 1.f,
        .label = "SSR IS-FAST Temporal Speed",
        .section = "Dynamic Cubemaps",
        .tooltip = "Animate the IS-FAST noise slice for SSR. 0 = frozen (stable, no shimmer), higher = faster animation.",
        .min = 0.f,
        .max = 5.0f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f && shader_injection.dynCube_ssr_enabled > 0.5f && g_isfast_enabled > 0.5f && shader_injection.dynCube_ssr_isfast_enabled > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DynCubeVanillaBlur",
        .binding = &shader_injection.dynCube_vanilla_blur,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 2.0f,
        .label = "Vanilla Cubemap Blur",
        .section = "Dynamic Cubemaps",
        .tooltip = "Mip-offset blur on the vanilla cubemap fallback. 0 = original/sharp, higher = progressively blurrier.",
        .min = 0.f,
        .max = 8.f,
        .format = "%.1f",
        .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "KaiHeroLightToggle",
        .binding = &g_kai_hero_light_toggle,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f,
        .label = "Hero Light Toggle",
        .section = "Hero Light",
        .tooltip = "Zeroes the color of the Kai hero (main character) light while it is matched by the light table.",
        .labels = {"Off", "On"},
        .on_change = []() {
        if (g_kai_hero_light_toggle < 0.5f) falcom_world::kai::ResetKaiHeroLight(); },
        .is_visible = []() { return IsKai(); },
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
        .label = "If you like the addon, consider supporting development on Patreon.",
        .section = "Info",
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
        .section = "IS-FAST",
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

#include "./settings_ui.h"

static void OnInitDevice(reshade::api::device* device) {
  reshade::log::message(reshade::log::level::info, "[sora-vanillaplus] Device init � addon loaded.");
  auto* d = renodx::utils::data::Create<DeviceData>(device);
  // DLSS Ray Reconstruction needs an NVIDIA GPU: the adapter this device runs on decides (rr::RrRequested).
  uint32_t vendor_id = 0u;
  const bool vendor_known = device->get_property(reshade::api::device_properties::vendor_id, &vendor_id);
  falcom_world::rr::g_rr_gpu_nvidia.store((vendor_known && vendor_id == 0x10DEu) ? 1 : 0);

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
#if defined(FALCOM_NRD_ENABLED)
  falcom_world::rtxdi::ResetSceneCpuMirrorsForDevice(reinterpret_cast<uintptr_t>(device));
#endif
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
    d->captured_depth_srv = {};
    d->captured_ssao_srv = {};
    d->captured_ssao_frame = 0u;
    d->deferred_ssao_srv = {};
    d->captured_depth_w = 0u;
    d->captured_depth_h = 0u;
    d->captured_depth_live = true;
    d->captured_mrt_live = true;
    d->captured_color_live = true;
    d->captured_cbv_live = true;
    d->captured_scene_cbv_view = {};
    d->captured_scene_cbv = {};
    d->captured_scene_cbv_valid = false;
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
    d->captured_depth_srv = {};
    d->captured_ssao_srv = {};
    d->captured_ssao_frame = 0u;
    d->deferred_ssao_srv = {};
    d->captured_depth_w = 0u;
    d->captured_depth_h = 0u;
    d->captured_depth_live = true;
    d->captured_mrt_live = true;
    d->captured_color_live = true;
    d->captured_cbv_live = true;
    d->captured_scene_cbv_view = {};
    d->captured_scene_cbv = {};
    d->captured_scene_cbv_valid = false;
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
    if (t.handle) {
      device->free_descriptor_table(t);
      t = {};
    }
  }
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

// Invalidates cached captures when a native buffer is destroyed or its handle is reused.
static std::atomic<uint32_t> g_scene_cbv_desc_generation{0u};

enum class RtxdiBindingMap { Unrelated,
                             Mapped,
                             Unknown };

static RtxdiBindingMap ResolveRtxdiSceneParameter(reshade::api::pipeline_layout layout, uint32_t parameter, uint32_t* binding) {
  using namespace reshade::api;
  RtxdiBindingMap result = RtxdiBindingMap::Unknown;
  const bool found = renodx::utils::pipeline_layout::GetPipelineLayoutData(layout, [&](const auto* data) {
    if (parameter >= data->params.size()) return;
    const auto& param = data->params[parameter];
    result = RtxdiBindingMap::Unrelated;
    const auto inspect = [&](const descriptor_range& range) {
      uint32_t mapped = 0u;
      const auto match = falcom_world::rtxdi::MapSceneDescriptorRange(
          {.binding = range.binding, .register_index = range.dx_register_index, .register_space = range.dx_register_space, .count = range.count, .array_size = range.array_size, .constant_buffer = range.type == descriptor_type::constant_buffer, .pixel_visible = (static_cast<uint32_t>(range.visibility) & static_cast<uint32_t>(shader_stage::pixel)) != 0u},
          kLightingSceneCbRegister, &mapped);
      if (match == falcom_world::rtxdi::SceneRangeMapping::Unsupported) result = RtxdiBindingMap::Unknown;
      if (match == falcom_world::rtxdi::SceneRangeMapping::Mapped) {
        if (result != RtxdiBindingMap::Unrelated) {
          result = RtxdiBindingMap::Unknown;
        } else {
          *binding = mapped;
          result = RtxdiBindingMap::Mapped;
        }
      }
    };
    switch (param.type) {
      case pipeline_layout_param_type::push_descriptors: inspect(param.push_descriptors); break;
      case pipeline_layout_param_type::descriptor_table:
      case pipeline_layout_param_type::push_descriptors_with_ranges:
        if (param.descriptor_table.count && !param.descriptor_table.ranges) {
          result = RtxdiBindingMap::Unknown;
          break;
        }
        for (uint32_t i = 0u; i < param.descriptor_table.count; ++i) inspect(param.descriptor_table.ranges[i]);
        break;
      case pipeline_layout_param_type::descriptor_table_with_static_samplers:
      case pipeline_layout_param_type::push_descriptors_with_static_samplers:
        if (param.descriptor_table_with_static_samplers.count && !param.descriptor_table_with_static_samplers.ranges) {
          result = RtxdiBindingMap::Unknown;
          break;
        }
        for (uint32_t i = 0u; i < param.descriptor_table_with_static_samplers.count; ++i) inspect(param.descriptor_table_with_static_samplers.ranges[i]);
        break;
      default: break;
    }
  });
  return (found ? result : RtxdiBindingMap::Unknown);
}

static void TrackRtxdiSceneBinding(DeviceData* data, reshade::api::command_list* cmd_list,
                                   const reshade::api::buffer_range& range, falcom_world::rtxdi::SceneCaptureSource source) {
  falcom_world::rtxdi::TrackSceneBinding(&data->rtxdi_scene_range,
                                         {.buffer = range.buffer.handle, .offset = range.offset, .size = range.size, .frame = data->frame_index, .command = reinterpret_cast<uintptr_t>(cmd_list), .source = source, .live = range.buffer.handle != 0u, .known = range.buffer.handle != 0u});
}

static void TrackRtxdiPush(reshade::api::command_list* cmd_list, reshade::api::shader_stage stages,
                           reshade::api::pipeline_layout layout, uint32_t parameter,
                           const reshade::api::descriptor_table_update& update) {
  if (!cmd_list || !(static_cast<uint32_t>(stages) & static_cast<uint32_t>(reshade::api::shader_stage::pixel))) return;
  auto* device = cmd_list->get_device();
  auto* data = device ? device->get_private_data<DeviceData>() : nullptr;
  if (!data) return;
  uint32_t table_binding = 0u;
  const auto mapped = ResolveRtxdiSceneParameter(layout, parameter, &table_binding);
  if (mapped == RtxdiBindingMap::Unrelated) return;
  reshade::api::buffer_range range = {};
  if (mapped == RtxdiBindingMap::Mapped && update.array_offset == 0u) {
    uint32_t index = 0u;
    if (!falcom_world::rtxdi::SceneDescriptorIndex(update.binding, update.count, table_binding, &index)) return;
    if (update.type == reshade::api::descriptor_type::constant_buffer && update.descriptors) {
      range = static_cast<const reshade::api::buffer_range*>(update.descriptors)[index];
    }
  }
  TrackRtxdiSceneBinding(data, cmd_list, range, falcom_world::rtxdi::SceneCaptureSource::Push);
}

static void TrackRtxdiTables(reshade::api::command_list* cmd_list, reshade::api::shader_stage stages,
                             reshade::api::pipeline_layout layout, uint32_t first, uint32_t count,
                             const reshade::api::descriptor_table* tables) {
  if (!cmd_list || !(static_cast<uint32_t>(stages) & static_cast<uint32_t>(reshade::api::shader_stage::pixel))) return;
  auto* device = cmd_list->get_device();
  auto* data = device ? device->get_private_data<DeviceData>() : nullptr;
  if (!data) return;
  bool relevant = false;
  bool ambiguous = false;
  reshade::api::buffer_range selected = {};
  for (uint32_t i = 0u; i < count; ++i) {
    if (first > UINT32_MAX - i) {
      ambiguous = true;
      break;
    }
    uint32_t binding = 0u;
    const auto mapped = ResolveRtxdiSceneParameter(layout, first + i, &binding);
    if (mapped == RtxdiBindingMap::Unrelated) continue;
    if (relevant || mapped == RtxdiBindingMap::Unknown) {
      ambiguous = true;
      break;
    }
    relevant = true;
    if (tables && tables[i].handle) {
      uint32_t offset = 0u;
      reshade::api::descriptor_heap heap = {};
      device->get_descriptor_heap_offset(tables[i], binding, 0u, &heap, &offset);
      auto* metadata = renodx::utils::data::Get<renodx::utils::descriptor::DeviceData>(device);
      if (metadata && heap.handle) {
        std::shared_lock lock(metadata->mutex);
        const auto it = metadata->heaps.find(heap.handle);
        if (it != metadata->heaps.end() && offset < it->second.size()) selected = it->second[offset].buffer_range;
      }
    }
  }
  if (ambiguous) {
    selected = {};
  }
  if (relevant || ambiguous) {
    TrackRtxdiSceneBinding(data, cmd_list, selected, falcom_world::rtxdi::SceneCaptureSource::DescriptorTable);
  }
}

static void ResetRtxdiSceneBinding(reshade::api::command_list* cmd_list) {
  if (!cmd_list) return;
  auto* device = cmd_list->get_device();
  auto* data = device ? device->get_private_data<DeviceData>() : nullptr;
  if (!data) return;
  falcom_world::rtxdi::ResetSceneBinding(&data->rtxdi_scene_range, reinterpret_cast<uintptr_t>(cmd_list));
}

static void PromoteRtxdiSceneBinding(reshade::api::command_list* cmd_list) {
  namespace di = falcom_world::rtxdi;
  if (!cmd_list) return;
  auto& diagnostics = di::GetDiagnosticState();
  const bool diagnostic_enabled = di::DiagnosticCaptureEnabled(diagnostics);
  const uint64_t diagnostic_epoch = diagnostic_enabled ? di::DiagnosticCaptureEpoch(diagnostics) : 0u;
  const uint32_t request_mask = di::SceneRequestMask();
  if (request_mask == 0u) {
    if (diagnostic_enabled) {
      di::RecordDiagnostic(diagnostics, diagnostic_epoch,
                           {.frame = falcom_world::g_state.frame.load(),
                            .stage = di::DiagnosticStage::ScenePromotion,
                            .reason = di::DiagnosticReason::None,
                            .operation = di::DiagnosticOperation::Promote});
    }
    return;
  }
  auto* shader_state = renodx::utils::shader::GetCurrentState(cmd_list);
  if (!shader_state || !IsLightingShader(renodx::utils::shader::GetCurrentPixelShaderHash(shader_state))) return;
  auto* device = cmd_list->get_device();
  auto* data = device ? device->get_private_data<DeviceData>() : nullptr;
  if (!data) {
    if (diagnostic_enabled) {
      di::RecordDiagnostic(diagnostics, diagnostic_epoch,
                           {.frame = falcom_world::g_state.frame.load(),
                            .stage = di::DiagnosticStage::ScenePromotion,
                            .reason = di::DiagnosticReason::MissingEntry,
                            .operation = di::DiagnosticOperation::Promote});
    }
    return;
  }
  const uintptr_t command = reinterpret_cast<uintptr_t>(cmd_list);
  di::SceneRangeSnapshot binding;
  bool entry_found = false;
  {
    std::shared_lock lock(data->rtxdi_scene_range.mutex);
    const auto it = data->rtxdi_scene_range.bindings.find(command);
    if (it != data->rtxdi_scene_range.bindings.end()) {
      binding = it->second;
      entry_found = true;
    }
  }
  const uint32_t generation = g_scene_cbv_desc_generation.load(std::memory_order_acquire);
  uint64_t normalized = 0u;
  uint64_t descriptor_buffer_bytes = 0u;
  falcom_world::rtxdi::SceneB0BindingEvidence native_evidence = {};
  falcom_world::rtxdi::SceneCopyWindow copy_window = {};
  di::DiagnosticReason reason = di::DiagnosticReason::None;
  bool valid = di::CanPromoteSceneBinding(binding, data->frame_index, command, true);
  if (!valid) {
    reason = !entry_found                         ? di::DiagnosticReason::MissingEntry
             : !binding.known                     ? di::DiagnosticReason::UnknownBinding
             : !binding.live                      ? di::DiagnosticReason::NonliveBinding
             : binding.frame != data->frame_index ? di::DiagnosticReason::StaleFrame
                                                  : di::DiagnosticReason::CommandMismatch;
  }
  if (valid && device->get_api() != reshade::api::device_api::d3d11) {
    valid = false;
    reason = di::DiagnosticReason::InvalidResource;
  }
  if (valid) {
    const auto desc = device->get_resource_desc(reshade::api::resource{binding.buffer});
    descriptor_buffer_bytes = desc.buffer.size;
    if (desc.type != reshade::api::resource_type::buffer) {
      valid = false;
      reason = di::DiagnosticReason::InvalidResource;
    } else if (!di::NormalizeSceneInputRange(desc.buffer.size, binding.offset, binding.size, &normalized)) {
      valid = false;
      reason = di::DiagnosticReason::InvalidRange;
    } else if (!di::ValidateSceneBuffer(reinterpret_cast<ID3D11Buffer*>(binding.buffer), binding.offset, binding.size)) {
      valid = false;
      reason = di::DiagnosticReason::InvalidRange;
    }  // NOLINT(performance-no-int-to-ptr)
  }
  if (valid) {
    const bool native_valid = di::ValidateSceneB0Binding(
        reinterpret_cast<ID3D11DeviceContext*>(cmd_list->get_native()),
        reinterpret_cast<ID3D11Buffer*>(binding.buffer), binding.offset,
        RTXDI_INPUT_SCENE_REQUIRED_ROWS, &native_evidence);  // NOLINT(performance-no-int-to-ptr)
    const bool window_valid = di::MakeSceneCopyWindow(native_evidence.buffer_bytes, binding.offset, binding.size,
                                                      native_evidence.first_constant, native_evidence.constant_count,
                                                      &copy_window);
    if (!native_valid || !window_valid || native_evidence.buffer_bytes != descriptor_buffer_bytes) {
      valid = false;
      reason = native_evidence.context_query_available
                   ? (native_evidence.buffer_bytes == descriptor_buffer_bytes ? di::DiagnosticReason::NativeB0Mismatch
                                                                              : di::DiagnosticReason::InvalidRange)
                   : di::DiagnosticReason::ContextQueryFailed;
    }
  }
  {
    std::unique_lock lock(data->rtxdi_scene_range.mutex);
    const auto it = data->rtxdi_scene_range.bindings.find(command);
    if (valid && generation != g_scene_cbv_desc_generation.load(std::memory_order_acquire)) {
      valid = false;
      reason = di::DiagnosticReason::ResetMismatch;
    }
    if (valid && it == data->rtxdi_scene_range.bindings.end()) {
      valid = false;
      reason = di::DiagnosticReason::MissingEntry;
    }
    if (valid && it->second.sequence != binding.sequence) {
      valid = false;
      reason = di::DiagnosticReason::SequenceMismatch;
    }
    if (valid && it->second.reset_generation != binding.reset_generation) {
      valid = false;
      reason = di::DiagnosticReason::ResetMismatch;
    }
    if (valid && !it->second.live) {
      valid = false;
      reason = di::DiagnosticReason::NonliveBinding;
    }
    if (!valid) data->rtxdi_scene_range.snapshot = {};
    if (valid) {
      binding.normalized_size = normalized;
      binding.native_first_constant = native_evidence.first_constant;
      binding.native_constant_count = native_evidence.constant_count;
      binding.copied_rows = copy_window.copied_rows;
      data->rtxdi_scene_range.snapshot = binding;
    }
  }
  if (diagnostic_enabled) {
    di::DiagnosticEvent event = {};
    event.owner_generation = falcom_world::rt12::g_rt12_generation.load(std::memory_order_relaxed);
    event.frame = data->frame_index;
    event.capture_frame = binding.frame;
    event.value0 = binding.offset;
    event.value1 = binding.size;
    event.normalized_size = normalized;
    event.native_buffer_bytes = native_evidence.buffer_bytes;
    event.native_offset_bytes = uint64_t{native_evidence.first_constant} * 16u;
    event.native_count_bytes = native_evidence.constant_count == 4096u
                                       && event.native_offset_bytes <= native_evidence.buffer_bytes
                                   ? native_evidence.buffer_bytes - event.native_offset_bytes
                                   : uint64_t{native_evidence.constant_count} * 16u;
    event.first_constant = native_evidence.first_constant;
    event.constant_count = native_evidence.constant_count;
    event.native_bind_flags = native_evidence.bind_flags;
    event.available_rows = copy_window.available_rows;
    event.required_rows = RTXDI_INPUT_SCENE_REQUIRED_ROWS;
    event.copied_rows = copy_window.copied_rows;
    event.scene_slot = -1;
    event.stage = di::DiagnosticStage::ScenePromotion;
    event.reason = reason;
    event.operation = di::DiagnosticOperation::Promote;
    event.flags = (entry_found ? 1u : 0u) | (binding.known ? 2u : 0u) | (binding.live ? 4u : 0u)
                  | (normalized != 0u ? 8u : 0u) | (valid ? 16u : 0u)
                  | (native_evidence.context_query_available ? 32u : 0u)
                  | (native_evidence.buffer_available ? 64u : 0u);
    event.identity_tokens[0] = di::DiagnosticIdentityToken(diagnostics, diagnostic_epoch, command,
                                                           binding.reset_generation);
    event.identity_tokens[1] = di::DiagnosticIdentityToken(diagnostics, diagnostic_epoch, binding.buffer,
                                                           binding.reset_generation);
    event.identity_tokens[2] = di::DiagnosticIdentityToken(diagnostics, diagnostic_epoch,
                                                           reinterpret_cast<uint64_t>(device), event.owner_generation);
    di::RecordDiagnostic(diagnostics, diagnostic_epoch, event);
  }
#if defined(FALCOM_NRD_ENABLED)
  if (valid) {
    const uint64_t mirrored_bytes = uint64_t{copy_window.copied_rows} * 16u;
    di::RegisterSceneCpuMirrorRange(reinterpret_cast<uintptr_t>(device), binding.buffer,
                                    binding.offset, mirrored_bytes, binding.frame, binding.sequence);
    falcom_world::SeedInitialSceneCpuMirror(reinterpret_cast<uintptr_t>(device), binding.buffer,
                                            binding.offset, mirrored_bytes);
  }
#endif
}

// -- Push-descriptors event ? capture lighting inputs (kai pattern) --

static void OnPushDescriptorsCapture(
    reshade::api::command_list* cmd_list,
    reshade::api::shader_stage stages,
    reshade::api::pipeline_layout layout,
    uint32_t param_index,
    const reshade::api::descriptor_table_update& update) {
  // Whole-hook timer, started before the gate so the calls the gate rejects are timed too.
  const falcom_world::bvh::PoolCpuTimer capture_timer{&falcom_world::bvh::g_perf_gate.block_us[falcom_world::bvh::kPerfBlockCaptureHook]};
  if (!cmd_list) return;
  TrackRtxdiPush(cmd_list, stages, layout, param_index, update);
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
  // Sun contact shadows read the depth, MRT normal and scene CBV captured below.
  // Local contact shadows need the lighting captures and VBGI's slots 0-2, 4 and 5 (material index, materials, lamp lists).
  if (shader_injection.gtvbao_mode < 0.5f
      && !falcom_world::rtao::RtaoRequested()
      && !falcom_world::rtxdi::SceneRequested()
      && !falcom_world::rr::RrRequested()
      && !VbgiRunning()
      && shader_injection.dynCube_enabled < 0.5f
      && !wantTAAMotionPush
      && !wantMotionBlurPush
      && shader_injection.cs_contact_enabled < 0.5f
      && !LocalContactComputeRunning()) {
    return;
  }
  if (capture_timer.armed) {
    falcom_world::bvh::g_perf_gate.capture_calls.fetch_add(1u, std::memory_order_relaxed);
    if (update.type == reshade::api::descriptor_type::texture_shader_resource_view && update.binding == 0u) {
      falcom_world::bvh::g_perf_gate.capture_b0_calls.fetch_add(1u, std::memory_order_relaxed);
    }
  }
  const bool srv = update.type == reshade::api::descriptor_type::texture_shader_resource_view;
  const bool cbv = update.type == reshade::api::descriptor_type::constant_buffer;
  if (update.count == 0u || (!srv && !cbv) || !update.descriptors) return;
  if (cbv && update.binding != kLightingSceneCbRegister) return;
  if (srv && update.binding != kDynCubeRegister && update.binding > ((VbgiRunning() || LocalContactComputeRunning()) ? 16u : 5u)) return;
  renodx::utils::state::CommandListStateCache state_cache;
  std::optional<uint32_t> ps_hash;
  auto pixel_hash = [&]() {
    if (!ps_hash.has_value()) {
      ps_hash = renodx::utils::state::GetCurrentShaderHash(cmd_list, reshade::api::pipeline_stage::pixel_shader, &state_cache);
    }
    return *ps_hash;
  };
  auto* device = cmd_list->get_device();
  auto* d = device->get_private_data<DeviceData>();
  if (!d) return;

  // -- Capture depth/SSAO/CBV � unconditional (register-based, kai-style). --
  if (update.type == reshade::api::descriptor_type::texture_shader_resource_view) {
    auto* views = static_cast<const reshade::api::resource_view*>(update.descriptors);
    std::optional<bool> vanilla_env_view;
    auto is_vanilla_env_view = [&]() {
      if (!vanilla_env_view.has_value()) vanilla_env_view = IsVanillaEnvCubeView(device, views[0]);
      return *vanilla_env_view;
    };
    // Capture depth: t4 (Sora) or t3 (Kai) � ONLY from lighting shader, game-specific binding.
    uint32_t depthBinding = IsKai() ? kLightingDepthRegisterKai : kLightingDepthRegister;
    if (update.binding == depthBinding && update.count >= 1
        && views[0].handle != 0u) {
      uint32_t hash = pixel_hash();
      if (IsLightingShader(hash)) {
        if (d->captured_depth_srv.handle != views[0].handle
            || !d->captured_depth_live.load(std::memory_order_acquire)) {
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
                                  (std::string("[GTVBAO] Depth captured from lighting: ") + std::to_string(dd.texture.width) + "x" + std::to_string(dd.texture.height)).c_str());
          }
        }
      }
    }
    // Capture SSAO: t5 (Sora) or t4 (Kai) — game-specific binding. Hash-gated for
    // the same reason depth and the MRT normal are: the binding number alone is
    // not unique, so any other shader pushing an SRV at that slot would otherwise
    // silently replace the captured AO.
    uint32_t ssaoBinding = IsKai() ? kLightingSsaoRegisterKai : kLightingSsaoRegister;
    if (update.binding == ssaoBinding && update.count >= 1
        && views[0].handle != 0u) {
      uint32_t hash = pixel_hash();
      if (IsLightingShader(hash)) {
        d->captured_ssao_srv = views[0];
        d->captured_ssao_frame = d->frame_index;
      }
    }
    if ((update.binding == kLightingMrtNormalRegister) && update.count >= 1
        && views[0].handle != 0u) {
      uint32_t hash = pixel_hash();
      if (IsLightingShader(hash)) {
        if (d->captured_mrt_normal_srv.handle != views[0].handle
            || !d->captured_mrt_live.load(std::memory_order_acquire)) {
          auto info = CSResolveCapture(device, views[0]);
          d->captured_mrt_res = info.res;
          d->captured_mrt_dims = info.dims;
          CSLog("capture", std::string("mrt handle -> ") + info.dims);
        }
        d->captured_mrt_normal_srv = views[0];
        d->captured_mrt_live = true;
      }
    }
    // Capture motion buffer t3 � ONLY from TAA draws (both Sora hashes), for
    // RCAS Stage 2 motion-adaptive sharpening. Same over-capture rationale as
    // the lighting-gated captures above: t3 means different things per shader.
    if (update.binding == 3u && update.count >= 1
        && views[0].handle != 0u) {
      uint32_t hash = pixel_hash();
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
    // Capture the tonemap's t0 for the motion blur deploy. The blit samples t0 and
    // writes the output-resolution target, so we need t0's own view for the
    // gather's colour input AND its exact format: the gather output is pushed
    // back at t0 and must stay format compatible with what the game's shader
    // expects. Over-capture of binding 0 is prevented by the hash gate.
    if (update.binding == 0u && update.count >= 1
        && views[0].handle != 0u) {
      uint32_t hash = pixel_hash();
      if (IsMotionBlurDeployHash(hash)) {
        d->mb_tonemap_src_srv = views[0];
        d->mb_tonemap_src_res = device->get_resource_from_view(views[0]).handle;
        d->mb_tonemap_src_live = true;
      }
    }
    // Capture t0 color texture � ONLY from the lighting shader (hash 0xFDAAF80E).
    // Unconditional capture would grab binding 0 from any shader, causing wrong colors.
    if (update.binding == 0u && update.count >= 1
        && views[0].handle != 0u) {
      uint32_t hash = pixel_hash();
      if (IsLightingShader(hash)) {
        if (d->captured_color_srv.handle != views[0].handle
            || !d->captured_color_live.load(std::memory_order_acquire)) {
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
    // VBGI radiance inputs: from the Sora and Kai lighting shaders while VBGI runs. The binding is the array base, so each
    // slot of the update is matched by its own number (shadow t9 on Sora 1st, t7 on Sora 2nd; slot 3 is the depth on Kai, so
    // it is not captured there). Kai slots 16, 11 and 12 come from any pixel-shader push (shape-filtered), see below. Slots 1 (0 on Kai), 2, 4 and 5 are also the local contact pass's material index, materials and lamp lists.
    if ((VbgiRunning() || LocalContactComputeRunning()) && update.count >= 1) {
      // Kai: t16 (cascade array), t11 and t12 (lamp lists) are bound by earlier passes and stay bound into the lighting
      // draw, so they are taken from any pixel-shader push, filtered by resource shape. The last accepted view is what the
      // lighting draw sees.
      if (IsKai()) {
        for (uint32_t i = 0; i < update.count; ++i) {
          const uint32_t slot = update.binding + i;
          const int k = slot == 16u ? 3 : (slot == 11u ? 4 : (slot == 12u ? 5 : -1));
          if (k < 0 || views[i].handle == 0u) continue;
          if (views[i].handle != d->vbgi_in_srv[k].handle) {
            const auto res = device->get_resource_from_view(views[i]);
            if (res.handle == 0u) continue;
            const auto desc = device->get_resource_desc(res);
            const bool ok = k == 3
                                ? (desc.type == reshade::api::resource_type::texture_2d && desc.texture.depth_or_layers == 3u)
                                : (desc.type == reshade::api::resource_type::buffer && desc.buffer.size > 0u
                                   && desc.buffer.size % (k == 4 ? 112u : 576u) == 0u);
            if (!ok) continue;
            d->vbgi_in_srv[k] = views[i];
            d->vbgi_in_res[k] = res.handle;
          }
          d->vbgi_in_live[k] = true;
          d->vbgi_in_frame[k] = d->frame_index;
        }
      }
      const bool kai = IsKai();
      const uint32_t shadow_slot = kai ? 16u : (IsSora1st() ? 9u : 7u);
      const uint32_t deferred_slot = kai ? 5u : 6u;
      std::optional<bool> vbgi_hash_gate;
      for (uint32_t i = 0; i < update.count; ++i) {
        if (views[i].handle == 0u) continue;
        const uint32_t slot = update.binding + i;
        int k = -1;
        if (slot == 2u)
          k = 0;
        else if (slot == 3u && !kai)
          k = 1;
        else if (slot == deferred_slot)
          k = 2;
        else if (!kai && slot == shadow_slot)
          k = 3;
        else if (!kai && slot == 11u)
          k = 4;
        else if (!kai && slot == 12u)
          k = 5;
        if (k < 0) continue;
        if (!vbgi_hash_gate.has_value()) {
          const uint32_t hash = pixel_hash();
          vbgi_hash_gate = hash == 0xFDAAF80Eu || hash == 0xCA3D8596u || (kai && (hash == 0x430ED091u || hash == 0xF6C55E5Fu));
        }
        if (!*vbgi_hash_gate) continue;
        if (views[i].handle != d->vbgi_in_srv[k].handle) d->vbgi_in_res[k] = device->get_resource_from_view(views[i]).handle;
        d->vbgi_in_srv[k] = views[i];
        d->vbgi_in_live[k] = true;
        d->vbgi_in_frame[k] = d->frame_index;
      }
    }
    // Capture ssr2-draw t0 (vanilla ssr1 march result) for the SSR replacement
    // debug view. ONLY from the ssr2 pixel shader � same over-capture rationale
    // as the color capture above.
    if (update.binding == 0u && update.count >= 1
        && views[0].handle != 0u) {
      uint32_t hash = pixel_hash();
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
    // Capture ssr1-draw t2 (the march's own mrt0) for the composite gate + normal
    // decode at t4. Same-resource guarantee: the march decodes normals/flags from
    // this exact view. ONLY from the ssr1 pixel shader.
    if (update.binding == 2u && update.count >= 1
        && views[0].handle != 0u) {
      uint32_t hash = pixel_hash();
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
        && is_vanilla_env_view()) {
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
      if (!s_dynCubeT17SwapGuard && is_vanilla_env_view()
          && (swapDbg != 0 || d->captured_vanilla_env_srv.handle != 0u)
          && d && d->dyncube_resources_created) {
        // Lighting draws always sample sharp (own sample-time blur/brightness);
        // every other global consumer gets the softened/dimmed variant when one is
        // active, sharp otherwise. Validity-gated on current settings so a stale
        // variant can never serve after the sliders return to neutral.
        bool swapIsLighting = false;
        {
          uint32_t swapHash = pixel_hash();
          swapIsLighting = IsLightingShader(swapHash);
          if (swapHash != 0u && shader_injection.custom_shader_logging >= 0.5f) {
            std::ostringstream hs;
            hs << std::hex << swapHash;
            CSLog("draw", std::string("t17 draw hash=0x") + hs.str());
          }
        }
        float swapSoften = std::clamp(shader_injection.dynCube_capture_soften, 0.f, 1.f);
        float swapStrength = std::clamp(shader_injection.dynCube_global_strength, 0.f, 1.f);
        bool serveVariant = !swapIsLighting && d->dyncube_variant_valid
                            && d->dyncube_variant_cube_srv.handle != 0u
                            && (swapSoften > 1e-4f || swapStrength < 1.f - 1e-4f);
        reshade::api::resource_view t17srv;
        if (swapDbg == 3) {
          if (!RunDynCubeSolid(cmd_list, d))
            t17srv = {};
          else
            t17srv = d->dyncube_solid_cube_srv;
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
                                     reshade::api::descriptor_table_update{{}, kDynCubeRegister, 0, 1, reshade::api::descriptor_type::texture_shader_resource_view, &t17srv});
          s_dynCubeT17SwapGuard = false;
        }
      }
    }
  }
  if (update.type == reshade::api::descriptor_type::constant_buffer) {
    if (update.binding == kLightingSceneCbRegister && update.count >= 1) {
      auto* cbv_views = static_cast<const reshade::api::resource_view*>(update.descriptors);
      if (cbv_views[0].handle != 0u) {
        reshade::api::resource buf = {cbv_views[0].handle};
        thread_local uint64_t s_desc_buffer = 0u;
        thread_local uint64_t s_desc_size = 0u;
        thread_local uint32_t s_desc_generation = 0u;
        const uint32_t desc_generation = g_scene_cbv_desc_generation.load(std::memory_order_acquire);
        uint64_t buffer_size = 0u;
        if (s_desc_generation == desc_generation && s_desc_buffer == buf.handle) {
          buffer_size = s_desc_size;
        } else {
          auto desc = device->get_resource_desc(buf);
          if (desc.type == reshade::api::resource_type::buffer
              && desc.buffer.size >= 200u
              && desc.buffer.size <= (64u * 1024u)) {
            buffer_size = desc.buffer.size;
            s_desc_buffer = buf.handle;
            s_desc_size = buffer_size;
            s_desc_generation = desc_generation;
          }
        }
        if (buffer_size != 0u) {
          if (d->captured_scene_cbv_view.handle != cbv_views[0].handle
              || !d->captured_cbv_live.load(std::memory_order_acquire)) {
            CSLog("capture", std::string("cbv handle -> buf:") + std::to_string(buffer_size));
            d->captured_cbv_res = buf.handle;
            d->captured_cbv_dims = std::string("buf:") + std::to_string(buffer_size);
          }
          d->captured_scene_cbv = {buf, 0, buffer_size};
          d->captured_scene_cbv_valid = true;
          d->captured_cbv_live = true;
          d->captured_scene_cbv_frame = d->frame_index;
          d->captured_scene_cbv_view = cbv_views[0];
        }
      }
    }
  }
  // -- Per-draw gating (only when GTVBAO, SSGI or RTAO is on). --
  if (shader_injection.gtvbao_mode < 0.5f && !falcom_world::rtao::RtaoRequested() && !falcom_world::rtxdi::SceneRequested() && !VbgiRunning()) return;
  if (!(static_cast<uint32_t>(stages) & static_cast<uint32_t>(reshade::api::shader_stage::pixel))) return;
}

// -- Bind-descriptor-tables event ? capture lighting inputs --

static void OnBindDescriptorTables(
    reshade::api::command_list* cmd_list,
    reshade::api::shader_stage stages,
    reshade::api::pipeline_layout layout,
    uint32_t first, uint32_t count,
    const reshade::api::descriptor_table* tables) {
  TrackRtxdiTables(cmd_list, stages, layout, first, count, tables);
  // Fast path: GTVBAO and RTAO are the consumers of these captures (depth, scene CBV).
  if (shader_injection.gtvbao_mode < 0.5f && !falcom_world::rtao::RtaoRequested() && !falcom_world::rtxdi::SceneRequested() && !VbgiRunning()) return;
  if (!cmd_list || !tables || count == 0u) return;
  auto* device = cmd_list->get_device();
  auto* d = device->get_private_data<DeviceData>();
  if (!d) return;

  // Log first 5 bind_descriptor_tables calls unconditionally.
  static uint32_t s_bind_log_count = 0u;
  if (s_bind_log_count < 5u) {
    s_bind_log_count++;
    reshade::log::message(reshade::log::level::info,
                          (std::string("[GTVBAO] bind_descriptor_tables: first=") + std::to_string(first) + ", count=" + std::to_string(count)).c_str());
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
      rc = param.descriptor_table.count;
      rr = param.descriptor_table.ranges;
    } else if (param.type == reshade::api::pipeline_layout_param_type::descriptor_table_with_static_samplers) {
      rc = param.descriptor_table_with_static_samplers.count;
      rr = param.descriptor_table_with_static_samplers.ranges;
    } else
      continue;
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
        uint32_t bo = 0u;
        reshade::api::descriptor_heap heap = {0u};
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
        uint32_t bo = 0u;
        reshade::api::descriptor_heap heap = {0u};
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

// -- Compute dispatch cleanup --------------------------------------------------
static void ApplyComputeDispatchFix(
    reshade::api::command_list* cmd_list,
    renodx::utils::state::CommandListState* cs,
    renodx::utils::state::CommandListState& prev,
    ComputeDispatchFixMode fix,
    uint32_t srv_count,
    uint32_t uav_count,
    uint32_t buffer_uav_count) {
  if (fix == ComputeDispatchFixMode::OFF) {
    cmd_list->bind_pipeline(reshade::api::pipeline_stage::all_compute, reshade::api::pipeline{0u});
    if (cs) *cs = prev;
    return;
  }

  // Both callers pass the state copied before their dispatch chain; restore from
  // that copy rather than the live tracker, which now describes our last pass.
  reshade::api::pipeline prev_compute_pipeline = {0u};
  reshade::api::pipeline_layout prev_layout = {0u};
  std::vector<reshade::api::descriptor_table> prev_tables;
  auto it = prev.pipelines.find(reshade::api::pipeline_stage::all_compute);
  if (it != prev.pipelines.end()) prev_compute_pipeline = it->second;
  prev_layout = prev.compute_pipeline_layout;
  prev_tables = prev.compute_descriptor_tables;

  if (fix == ComputeDispatchFixMode::NULL_AND_RESTORE) {
    // Additionally null the individual compute slots (belt and suspenders)
    reshade::api::resource_view null_srv = {};
    reshade::api::resource_view null_uav = {};
    reshade::api::sampler null_sampler = {};
    cmd_list->push_descriptors(reshade::api::shader_stage::all_compute,
                               reshade::api::pipeline_layout{0}, 0,
                               reshade::api::descriptor_table_update{{}, 0, 0, 1, reshade::api::descriptor_type::sampler, &null_sampler});
    for (uint32_t i = 0; i < srv_count; ++i)
      cmd_list->push_descriptors(reshade::api::shader_stage::all_compute,
                                 reshade::api::pipeline_layout{0}, 0,
                                 reshade::api::descriptor_table_update{{}, i, 0, 1, reshade::api::descriptor_type::texture_shader_resource_view, &null_srv});
    for (uint32_t i = 0; i < uav_count; ++i)
      cmd_list->push_descriptors(reshade::api::shader_stage::all_compute,
                                 reshade::api::pipeline_layout{0}, 0,
                                 reshade::api::descriptor_table_update{{}, i, 0, 1, reshade::api::descriptor_type::texture_unordered_access_view, &null_uav});
    for (uint32_t i = 0; i < buffer_uav_count; ++i) {
      cmd_list->push_descriptors(reshade::api::shader_stage::all_compute,
                                 reshade::api::pipeline_layout{0}, 0,
                                 reshade::api::descriptor_table_update{{}, i, 0, 1, reshade::api::descriptor_type::buffer_unordered_access_view, &null_uav});
    }
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

// -- GTVBAO CS Dispatch Fix: clears stale compute bindings that cause double volumetrics --
static void ApplyGTVBAOCSDispatchFix(
    reshade::api::command_list* cmd_list,
    renodx::utils::state::CommandListState* cs,
    renodx::utils::state::CommandListState& prev) {
  const int fix = static_cast<int>(g_gtvbao_cs_dispatch_fix);
  ComputeDispatchFixMode mode = ComputeDispatchFixMode::OFF;
  switch (fix) {
    case 1:  mode = ComputeDispatchFixMode::RESTORE_STATE; break;
    case 2:
    case 3:  mode = ComputeDispatchFixMode::NULL_AND_RESTORE; break;
    default: break;
  }
  ApplyComputeDispatchFix(cmd_list, cs, prev, mode, 5u, 4u);
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
  if (!res.handle) {
    info.dims = "dead";
    return info;
  }
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
  if (deadRes != 0u) falcom_world::rtxdi::InvalidateSceneBuffer(&d->rtxdi_scene_range, deadRes);
  KillTrackedInput(d, d->deferred_depth_srv, d->deferred_depth_res, d->defDepthLive, "defDepth", deadView, deadRes);
  KillTrackedInput(d, d->deferred_mrt_normal_srv, d->deferred_mrt_res, d->defMrtLive, "defMrt", deadView, deadRes);
  KillTrackedInput(d, d->deferred_scene_cbv_view, d->deferred_cbv_res, d->defCbvLive, "defCbv", deadView, deadRes);
  KillTrackedInput(d, d->captured_ssr1_srv, d->captured_ssr1_res, d->captured_ssr1_live, "ssr1", deadView, deadRes);
  KillTrackedInput(d, d->captured_ssr_mrt_srv, d->captured_ssr_mrt_res, d->captured_ssr_mrt_live, "ssrMrt", deadView, deadRes);
  KillTrackedInput(d, d->rcas_motion_srv, d->rcas_motion_res, d->rcas_motion_live, "rcasMotion", deadView, deadRes);
  KillTrackedInput(d, d->mb_rtv4_srv,
                   d->mb_rtv4_res.load(std::memory_order_acquire), d->mb_rtv4_live,
                   "mbRtv4Motion", deadView, deadRes);
  KillTrackedInput(d, d->mb_tonemap_src_srv, d->mb_tonemap_src_res, d->mb_tonemap_src_live, "mbTonemapSrc", deadView, deadRes);
  for (int k = 0; k < 6; ++k) KillTrackedInput(d, d->vbgi_in_srv[k], d->vbgi_in_res[k], d->vbgi_in_live[k], "vbgiInput", deadView, deadRes);
}
static void OnDestroyResourceView(reshade::api::device* device, reshade::api::resource_view view) {
  if (!device || !view.handle) return;
  auto* d = device->get_private_data<DeviceData>();
  if (!d) return;
  KillAllTracked(d, view.handle, 0u);
}
static void OnDestroyResource(reshade::api::device* device, reshade::api::resource res) {
  g_scene_cbv_desc_generation.fetch_add(1u, std::memory_order_release);
  if (!device || !res.handle) return;
  auto* d = device->get_private_data<DeviceData>();
  if (!d) return;
  KillAllTracked(d, 0u, res.handle);
  // The motion resource died, so our SRV onto it is dead too. Flag it rather than
  // destroying it here: this handler runs on the loader thread, and the render
  // thread rebuilds it via MBMotionRtv4Ensure. Leaving a dead view bound for even
  // one frame is the thing that must not happen, and the flag is set before the
  // next chain run, which is the same frame's tonemap.
  if (const uint64_t motion_res = d->mb_rtv4_res.load(std::memory_order_acquire);
      motion_res != 0u && motion_res == res.handle) {
    d->mb_rtv4_res.store(0u, std::memory_order_release);
    d->mb_rtv4_owned_dirty.store(true, std::memory_order_release);
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
// 1=entry 2=liveness-resolver 3=recreate 4=deferred-dispatch, 5=DynCube recreate,
// 6=resource-size resolve, 7=GTVBAO recreate; 0=idle (between presents).
// RAII scope resets to idle on EVERY return path so a stale region can never mislead.
static std::atomic<int> s_presentRegion{0};
static const char* PresentRegionName(int r) {
  switch (r) {
    case 1:  return "entry";
    case 2:  return "resolver";
    case 3:  return "recreate";
    case 4:  return "deferred";
    case 5:  return "DynCube recreate";
    case 6:  return "resource-size resolve";
    case 7:  return "GTVBAO recreate";
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
               clock::now().time_since_epoch())
        .count();
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
      CSLog("watchdog", std::string("no present for ") + std::to_string(gap / 1000u) + "s (stuck in " + PresentRegionName(region) + "?)", true);
    }
  }
}
inline static uint64_t WatchdogNowMs() {
  return (uint64_t)std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::steady_clock::now().time_since_epoch())
      .count();
}

// -- Present hook --

// t11 (dynamicLights_g) of the current Kai lighting draw. The hero light patch works on the records mapped on this buffer.
static std::atomic_uint64_t g_kai_lights_resource{0u};

static void OnKaiHeroMapBuffer(reshade::api::device*, reshade::api::resource resource, uint64_t, uint64_t size,
                               reshade::api::map_access, void** data) {
  if (g_kai_hero_light_toggle > 0.5f && resource.handle == g_kai_lights_resource.load(std::memory_order_relaxed)) {
    falcom_world::kai::g_kai_hero_light.mapped = *data;
    falcom_world::kai::g_kai_hero_light.mapped_bytes = std::min<uint64_t>(size, falcom_world::kai::kHeroLightBufferBytes);
  }
}

static void OnKaiHeroUnmapBuffer(reshade::api::device* device, reshade::api::resource resource) {
  auto& hero = falcom_world::kai::g_kai_hero_light;
  if (hero.mapped != nullptr && resource.handle == g_kai_lights_resource.load(std::memory_order_relaxed)) {
    auto* lights = static_cast<falcom_world::rtgi::LightParamCpu*>(hero.mapped);
    const size_t count = (std::min)(hero.mapped_bytes / sizeof(falcom_world::rtgi::LightParamCpu),
                                    falcom_world::kai::kHeroLightCount);
    auto* d = device->get_private_data<DeviceData>();
    if (d != nullptr) {
      // Matching reads only the copy; the copy is taken before the patch below writes the mapped records.
      std::memcpy(hero.copy.data(), lights, count * sizeof(falcom_world::rtgi::LightParamCpu));
      const bool new_frame = d->frame_index != hero.evaluated_frame;
      if (new_frame) {
        hero.evaluated_frame = d->frame_index;
        hero.max_maps_per_frame = (std::max)(hero.max_maps_per_frame, hero.maps_in_frame);
        hero.maps_in_frame = 0u;
      }
      ++hero.maps_in_frame;
      const auto eval_start = std::chrono::steady_clock::now();
      falcom_world::kai::EvaluateKaiHeroLights(hero.copy.data(), count, new_frame);
      hero.eval_us = static_cast<uint64_t>(
          std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - eval_start).count());
      hero.patched = falcom_world::kai::PatchKaiHeroLights(lights, count);
      const auto now = std::chrono::steady_clock::now();
      if (now - hero.last_log >= std::chrono::seconds(1)) {
        hero.last_log = now;
        std::ostringstream latched;
        uint32_t listed = 0u;
        for (size_t c = 0; c < falcom_world::kai::kHeroConfigCount && listed < 4u; ++c) {
          if (hero.latched_index[c] >= 0) {
            latched << (listed != 0u ? "," : "") << falcom_world::kai::kKaiPlayerLightTable[c].config_id << "@" << hero.latched_index[c];
            ++listed;
          }
        }
        if (listed == 0u) latched << "none";
        renodx::utils::log::i("[kai-hero] latched=", latched.str(), " patched=", hero.patched, " eval_us=", hero.eval_us,
                              " maps_per_frame_max=", hero.max_maps_per_frame);
        hero.max_maps_per_frame = 0u;
      }
    }
    hero.mapped = nullptr;
  }
}

static void OnPresent(reshade::api::command_queue* queue, reshade::api::swapchain* sc,
                      const reshade::api::rect*, const reshade::api::rect*,
                      uint32_t, const reshade::api::rect*) {
  auto* dev = queue->get_device();
  auto* cl = queue->get_immediate_command_list();
  auto* d = dev->get_private_data<DeviceData>();
  if (!d) return;
  d->frame_index++;
  if (g_dyncube_gpu_timing > 0.5f) {
    const auto now = std::chrono::steady_clock::now();
    // The scheduler's 1/sec line resets the window; when it does not run (DynCube off, menus,
    // loading, deferred list) drop a window older than 1.5 s and its stale slow threshold.
    if (d->present_count > 0u && now - d->present_window_t0 > std::chrono::milliseconds(1500)) {
      ResetPresentWindow(d);
      d->present_prev_avg_ms = 0.0;
    }
    if (d->present_last_valid) {
      const double ms = std::chrono::duration<double, std::milli>(now - d->present_last_time).count();
      if (d->present_count == 0u) {
        d->present_window_t0 = now;
      }
      d->present_sum_ms += ms;
      d->present_count += 1u;
      if (ms > d->present_max_ms) d->present_max_ms = static_cast<float>(ms);
      if (d->present_prev_avg_ms > 0.0 && ms > 1.5 * d->present_prev_avg_ms) d->present_slow_count += 1u;
    }
    d->present_last_time = now;
    d->present_last_valid = true;
  } else {
    d->present_last_valid = false;
    ResetPresentWindow(d);
    d->present_prev_avg_ms = 0.0;
  }
  {
    static uint64_t s_cap_frames = 0u;
    static std::chrono::steady_clock::time_point s_cap_window{};
    if (!falcom_world::g_state.supported) {
      const bool want = g_capture_perf_log > 0.5f && IsMotionBlurGame();
      auto& gate = falcom_world::bvh::g_perf_gate;
      const auto now = std::chrono::steady_clock::now();
      if (gate.gather.load(std::memory_order_relaxed) != want) {
        gate.gather.store(want, std::memory_order_relaxed);
        gate.block_us[falcom_world::bvh::kPerfBlockCaptureHook].exchange(0u);
        gate.capture_calls.exchange(0u);
        gate.capture_b0_calls.exchange(0u);
        s_cap_frames = 0u;
        s_cap_window = now;
      }
      if (want) {
        s_cap_frames++;
        const double elapsed = std::chrono::duration<double>(now - s_cap_window).count();
        if (elapsed >= 1.0) {
          const uint64_t frames = s_cap_frames;
          const uint64_t hook_ns = gate.block_us[falcom_world::bvh::kPerfBlockCaptureHook].exchange(0u);
          const uint64_t calls = gate.capture_calls.exchange(0u);
          const uint64_t b0_calls = gate.capture_b0_calls.exchange(0u);
          std::ostringstream msg;
          msg << "[CapturePerf] fps=" << std::fixed << std::setprecision(1) << frames / elapsed
              << " hook_ms_per_frame=" << std::setprecision(3) << hook_ns / 1e6 / frames
              << " calls_per_frame=" << calls / frames
              << " b0_per_frame=" << b0_calls / frames;
          reshade::log::message(reshade::log::level::info, msg.str().c_str());
          s_cap_frames = 0u;
          s_cap_window = now;
        }
      }
    }
  }
  // RTV4 re-arm budget is per frame; a rejected candidate may be retried a few
  // times, but never across a frame boundary.
  d->mb_rtv4_rejects = 0u;
  d->immediate_cmd_list = queue->get_immediate_command_list();
  // RTAO state for this present (0 off, 1 requested but not producing, 2 producing last frame).
  // Latched here, read by the SSAO shader and the GTVBAO gates during the frame.
  {
    auto& rtao_frame = falcom_world::rtao::g_rtao_frame;
    if (!falcom_world::rtao::RtaoRequested()) {
      shader_injection.rtao_active = 0.f;
    } else {
      shader_injection.rtao_active = rtao_frame.producing ? 2.f : 1.f;
      if (!rtao_frame.producing) {
        rtao_frame.frames_without_ao++;
        rtao_frame.frames_without_ao_by_reason[static_cast<size_t>(rtao_frame.reason)]++;
      }
    }
    if (rtao_frame.reason != rtao_frame.logged_reason) {
      rtao_frame.logged_reason = rtao_frame.reason;
      renodx::utils::log::i("[world-rtao] state: reason=", falcom_world::rtao::ReasonName(rtao_frame.reason),
                            " rtao_active=", shader_injection.rtao_active);
    }
  }
  s_lastPresentMs.store(WatchdogNowMs());
  if (!s_watchdogStarted) {
    s_watchdogStarted = true;
    s_watchdogThread = std::thread(&WatchdogThreadMain);
  }
  PresentRegionScope presentRegionScope;

  // 1/sec heartbeat: recency anchor for crashes + transition detector (photo mode,
  // DLSS toggle, loads). Everything else in this log is changes-only, so steady
  // state is this line alone.
  CSBeat(std::string("frame=") + std::to_string(d->frame_index) + " working=" + std::to_string(d->working_width) + "x" + std::to_string(d->working_height) + " liveDepth=" + (d->captured_depth_srv.handle ? d->captured_depth_dims : "none") + " cbv=" + (d->captured_scene_cbv_valid ? "ok" : "MISS") + " gtvbaoRes=" + (d->resources_created ? "1" : "0") + " cubeRes=" + (d->dyncube_resources_created ? "1" : "0") + " rs=" + std::to_string(d->dyncube_readSet) + " flt=" + std::to_string(d->dyncube_filteredReadSet) + " rej=" + std::to_string(d->dyncube_rejected_captures));

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
                            (std::string("[DynCube] disabled: freed ") + std::to_string(d->dyncube_size) + "x" + std::to_string(d->dyncube_size) + " set (~" + std::to_string((long long)(DynCubeEstimatedBytes(d) / (1024ull * 1024ull))) + " MB)").c_str());
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
  if (shader_injection.gtvbao_mode < 0.5f && !dynCube_present_active && !VbgiRunning()) return;

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
    s_presentRegion.store(5);
    const uint32_t wantSize = d->dyncube_pending_size;
    // CreateDynCubeResources destroys the active set before allocating, so the old
    // size is always released (no leak) and never overlaps the new one.
    CreateDynCubeResources(dev, d, wantSize);
    CreateDynCubePipelinesIfNeeded(dev, d);
    if (shader_injection.dynCube_debug_logging > 0.5f) {
      reshade::log::message(reshade::log::level::info,
                            (std::string("[DynCube] resized to ") + std::to_string(wantSize)).c_str());
    }
    CSLog("dyncube", std::string("cube recreate: want=") + std::to_string(wantSize) + " active=" + std::to_string(d->dyncube_size));
    d->dyncube_pending_size = 0u;
    d->dyncube_pending_recreate = false;
  }

  // Create / recreate resources using depth texture size (kai pattern).
  {
    s_presentRegion.store(6);
    uint32_t gw = 0u, gh = 0u;
    const bool depth_view_live = d->captured_depth_live.load(std::memory_order_acquire);
    const bool depth_view_dead = d->captured_depth_srv.handle != 0u && !depth_view_live;
    // Destroy events can invalidate a cached view between frames. Use dimensions
    // captured with the live view instead of resolving that handle during Present.
    if (depth_view_live) {
      gw = d->captured_depth_w;
      gh = d->captured_depth_h;
    }
    if (!depth_view_dead && (gw < 64u || gh < 64u)) {
      auto bb = sc->get_back_buffer(0);
      auto bd = dev->get_resource_desc(bb);
      gw = bd.texture.width;
      gh = bd.texture.height;
    } else if (!depth_view_dead) {
      // Validate: unconditional capture might pick up small SSAO at t4 (e.g. 160x90).
      auto bb = sc->get_back_buffer(0);
      auto bd = dev->get_resource_desc(bb);
      if (gw < bd.texture.width / 4u || gh < bd.texture.height / 4u) {
        gw = bd.texture.width;
        gh = bd.texture.height;
      }
    }
    if (depth_view_dead) CSLog("gtvbao", "resource recreate deferred: captured depth view dead");
    const bool too_small = d->working_width < 320u || d->working_height < 320u;
    const float want_res = std::clamp(std::round(shader_injection.gtvbao_resolution), 0.f, 1.f);
    const float want_vbgi_res = std::clamp(std::round(g_vbgi_resolution), 0.f, static_cast<float>(kVbgiMaxResolutionIndex));
    if (gw > 0u && gh > 0u
        && (!d->resources_created || too_small
            || gw != d->last_created_game_width
            || gh != d->last_created_game_height
            || want_res != d->last_created_gtvbao_resolution
            || want_vbgi_res != d->last_created_vbgi_resolution)) {
      s_presentRegion.store(7);
      CreateGTVBAOResources(dev, d, gw, gh);
      d->last_created_game_width = gw;
      d->last_created_game_height = gh;
      d->resources_created = true;
      CSLog("gtvbao", std::string("resources (re)created: working=") + std::to_string(d->working_width) + "x" + std::to_string(d->working_height) + " depth=" + std::to_string(gw) + "x" + std::to_string(gh));
      reshade::log::message(reshade::log::level::info,
                            (std::string("[GTVBAO] Resources created: ") + std::to_string(d->working_width) + "x" + std::to_string(d->working_height) + " (depth=" + std::to_string(gw) + "x" + std::to_string(gh) + ")").c_str());
    }
  }
  // Use deferred snapshots from lighting draw (kai-style) � deferred dispatch only.
  // Inline dispatch active (deferred off) � GTVBAO runs during lighting pass, not here.
  // Drop snapshots whose targets died since capture (destroy events flip the flags;
  // no queries here). Dispatching on them is never correct.
  s_presentRegion.store(4);
  if (d->deferred_pending && (!d->defDepthLive || !d->defMrtLive || !d->defCbvLive)) {
    CSLog("gtvbao", "deferred snapshot DEAD: dropped", true);
    d->deferred_pending = false;
  }
  if (!d->deferred_pending || !d->deferred_depth_srv.handle) {
    // DynCube debug face preview � must run even when GTVBAO deferred off
    if (d->dyncube_resources_created && d->dyncube_srv.handle && shader_injection.dynCube_enabled > 0.5f
        && (shader_injection.dynCube_debug == 1.f || shader_injection.dynCube_debug == 2.f
            || shader_injection.dynCube_debug == 5.f || shader_injection.dynCube_debug == 6.f
            || shader_injection.dynCube_debug == 7.f || shader_injection.dynCube_debug == 8.f)) {
      int face = (int)std::clamp(shader_injection.dynCube_debug_face, 0.f, 5.f);
      const uint32_t outSet = d->dyncube_readSet;  // validated read set (delayed-validate commit)
      reshade::api::resource srcTex = d->dyncube_texture;
      uint32_t srcSub = (uint32_t)face;
      uint32_t srcW = 0, srcH = 0;
      if (shader_injection.dynCube_debug == 5.f)
        srcTex = d->dyncube_hist[outSet].pos;
      else if (shader_injection.dynCube_debug == 6.f)
        srcTex = d->dyncube_hist[outSet].contrib;
      else if (shader_injection.dynCube_debug == 7.f)
        srcTex = d->dyncube_charmask;
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
        uint32_t srcDim = (srcW != 0u) ? srcW : d->dyncube_size;  // source box dims (mip-sized for debug 10)
        uint32_t preview = std::min(d->dyncube_size * 2, 512u);   // display rect constant so mip progression stays visible
        if (preview <= bbDesc.texture.width && preview <= bbDesc.texture.height) {
          reshade::api::subresource_box srcBox = {0, 0, 0, srcDim, srcDim, 1};
          reshade::api::subresource_box dstBox = {0, 0, 0, preview, preview, 1};
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
                          (std::string("[GTVBAO] Dispatching (frame=") + std::to_string(d->frame_index) + ", res=" + std::to_string(d->working_width) + "x" + std::to_string(d->working_height) + ")").c_str());

  // Save command-list state.
  auto* cs = renodx::utils::state::GetCurrentState(cl);
  renodx::utils::state::CommandListState prev = {};
  if (cs) prev = *cs;

  bool ok = true;
  if (shader_injection.gtvbao_mode > 0.5f && !falcom_world::rtao::RtaoRequested()) {
    const std::string liveDepth = d->captured_depth_srv.handle ? d->captured_depth_dims : "none";
    const std::string wantDims = std::to_string(d->working_width) + "x" + std::to_string(d->working_height);
    const bool dimMismatch = (liveDepth != wantDims);
    CSLog("gtvbao", std::string("invoke working=") + wantDims + " liveDepth=" + liveDepth + " cbv=" + (d->captured_scene_cbv_valid ? "ok" : "MISSING"), dimMismatch || !d->captured_scene_cbv_valid);
    ok = RunGTVBAO(cl, d);
    CSLog("gtvbao", std::string("invoke exit") + (ok ? " ok" : " FAILED"), !ok);
  }
  const bool ao_ok = (shader_injection.gtvbao_mode > 0.5f && !falcom_world::rtao::RtaoRequested()) && ok;
  if (VbgiRunning()) {
    const bool ok_vbgi = RunVBGI(cl, d, ao_ok);
    CSLog("vbgi", std::string("invoke exit") + (ok_vbgi ? " ok" : " FAILED"), !ok_vbgi);
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

  // -- DynCube debug face preview � samples capture resources directly (not t17) --
  if (d->dyncube_resources_created && d->dyncube_srv.handle && shader_injection.dynCube_enabled > 0.5f
      && (shader_injection.dynCube_debug == 1.f || shader_injection.dynCube_debug == 2.f
          || shader_injection.dynCube_debug == 5.f || shader_injection.dynCube_debug == 6.f
          || shader_injection.dynCube_debug == 7.f || shader_injection.dynCube_debug == 8.f)) {
    int face = (int)std::clamp(shader_injection.dynCube_debug_face, 0.f, 5.f);
    const uint32_t outSet = d->dyncube_readSet;  // validated read set (delayed-validate commit)
    reshade::api::resource srcTex = d->dyncube_texture;
    uint32_t srcSub = (uint32_t)face;
    uint32_t srcW = 0, srcH = 0;
    if (shader_injection.dynCube_debug == 5.f)
      srcTex = d->dyncube_hist[outSet].pos;
    else if (shader_injection.dynCube_debug == 6.f)
      srcTex = d->dyncube_hist[outSet].contrib;
    else if (shader_injection.dynCube_debug == 7.f)
      srcTex = d->dyncube_charmask;
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
      uint32_t srcDim = (srcW != 0u) ? srcW : d->dyncube_size;  // source box dims (mip-sized for debug 10)
      uint32_t preview = std::min(d->dyncube_size * 2, 512u);   // display rect constant so mip progression stays visible
      if (preview <= bbDesc.texture.width && preview <= bbDesc.texture.height) {
        reshade::api::subresource_box srcBox = {0, 0, 0, srcDim, srcDim, 1};
        reshade::api::subresource_box dstBox = {0, 0, 0, preview, preview, 1};
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
            {}, kDynCubeRegister, 0, 1, reshade::api::descriptor_type::texture_shader_resource_view, &t17srv});
  }
  if (dd->dyncube_hist[dd->dyncube_readSet].pos_cube_srv.handle) {
    auto histPosSrv = dd->dyncube_hist[dd->dyncube_readSet].pos_cube_srv;
    cmd_list->push_descriptors(
        reshade::api::shader_stage::pixel,
        reshade::api::pipeline_layout{0}, 0,
        reshade::api::descriptor_table_update{
            {}, kDynCubeHistPosRegister, 0, 1, reshade::api::descriptor_type::texture_shader_resource_view, &histPosSrv});
  }
  cmd_list->push_descriptors(
      reshade::api::shader_stage::pixel,
      reshade::api::pipeline_layout{0}, 0,
      reshade::api::descriptor_table_update{
          {}, kDynCubeVanillaRegister, 0, 1, reshade::api::descriptor_type::texture_shader_resource_view, &dd->captured_vanilla_env_srv});
  // t4 mrt normals: prefer the march's own t2 capture (same resource the vanilla
  // march decodes, so gate bits and resolution match by construction); fall back
  // to the lighting capture. First-live-wins: dead views keep game bindings.
  // Unbound Load returns 0, which safely gates out to the vanilla branch.
  reshade::api::resource_view mrtSrv = {};
  if (dd->captured_ssr_mrt_srv.handle && dd->captured_ssr_mrt_live)
    mrtSrv = dd->captured_ssr_mrt_srv;
  else if (dd->captured_mrt_normal_srv.handle && dd->captured_mrt_live)
    mrtSrv = dd->captured_mrt_normal_srv;
  if (mrtSrv.handle) {
    cmd_list->push_descriptors(
        reshade::api::shader_stage::pixel,
        reshade::api::pipeline_layout{0}, 0,
        reshade::api::descriptor_table_update{
            {}, 4u, 0, 1, reshade::api::descriptor_type::texture_shader_resource_view, &mrtSrv});
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
              {}, 5u, 0, 1, reshade::api::descriptor_type::texture_shader_resource_view, &dd->isfast_noise_srv});
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
            {}, kDynCubeRegister, 0, 1, reshade::api::descriptor_type::texture_shader_resource_view, &t17srv});
  }
  if (dd->dyncube_hist[dd->dyncube_readSet].pos_cube_srv.handle) {
    auto histPosSrv = dd->dyncube_hist[dd->dyncube_readSet].pos_cube_srv;
    cmd_list->push_descriptors(
        reshade::api::shader_stage::pixel,
        reshade::api::pipeline_layout{0}, 0,
        reshade::api::descriptor_table_update{
            {}, kDynCubeHistPosRegister, 0, 1, reshade::api::descriptor_type::texture_shader_resource_view, &histPosSrv});
  }
  cmd_list->push_descriptors(
      reshade::api::shader_stage::pixel,
      reshade::api::pipeline_layout{0}, 0,
      reshade::api::descriptor_table_update{
          {}, kDynCubeVanillaRegister, 0, 1, reshade::api::descriptor_type::texture_shader_resource_view, &dd->captured_vanilla_env_srv});
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
              {}, 5u, 0, 1, reshade::api::descriptor_type::texture_shader_resource_view, &dd->isfast_noise_srv});
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
  bool present = false;
  bool matches_desired = false;
  bool matches_alternate = false;
  renodx::utils::shader::ForEachRuntimeReplacement(
      dev,
      [&](uint32_t replacement_hash, std::span<const uint8_t> bytecode) {
        if (replacement_hash != hash) return;
        present = true;
        matches_desired = TAASpanEquals(bytecode, desired);
        matches_alternate = !alternate.empty() && TAASpanEquals(bytecode, alternate);
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
                  {},
                  2u,
                  0,
                  1,
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
  dv(d->rcas_temp_srv);
  dv(d->rcas_temp_uav);
  dr(d->rcas_temp_texture);
  dv(d->rcas_hist_srv);
  dr(d->rcas_hist_texture);
  d->rcas_w = 0u;
  d->rcas_h = 0u;
  d->rcas_fmt = reshade::api::format::unknown;
  d->rcas_have_copy = false;
  d->rcas_format_logged = false;
  d->rcas_dispatch_logged = false;
}

static void DestroyRCASResources(reshade::api::device* dev, DeviceData* d) {
  if (!dev || !d) return;
  DestroyRCASSet(dev, d);
  for (auto& t : d->rcas_tables) {
    if (t.handle) {
      dev->free_descriptor_table(t);
      t = {};
    }
  }
  if (d->rcas_pipeline.handle) {
    dev->destroy_pipeline(d->rcas_pipeline);
    d->rcas_pipeline = {};
  }
  if (d->rcas_layout.handle) {
    dev->destroy_pipeline_layout(d->rcas_layout);
    d->rcas_layout = {};
  }
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
  d->rcas_w = w;
  d->rcas_h = h;
  d->rcas_fmt = fmt;
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
    sd.code = bc.data();
    sd.code_size = bc.size();
    sd.entry_point = "main";
    reshade::api::pipeline_subobject so = {reshade::api::pipeline_subobject_type::compute_shader, 1, &sd};
    return dev->create_pipeline(lo, 1, &so, out);
  };
  if (d->rcas_layout.handle == 0u || d->rcas_layout_version != kRCASLayoutVersion) {
    if (d->rcas_layout.handle) {
      for (auto& t : d->rcas_tables) {
        if (t.handle) {
          dev->free_descriptor_table(t);
          t = {};
        }
      }
      dev->destroy_pipeline_layout(d->rcas_layout);
      d->rcas_layout = {};
      if (d->rcas_pipeline.handle) {
        dev->destroy_pipeline(d->rcas_pipeline);
        d->rcas_pipeline = {};
      }
    }
    DR srv_r = {0, 0, 0, 2, DS::all_compute, 1, DT::texture_shader_resource_view};   // t0 unsharpened copy, t1 motion buffer
    DR uav_r = {0, 0, 0, 1, DS::all_compute, 1, DT::texture_unordered_access_view};  // u0 sharpened temp
    reshade::api::constant_range push_range = {};
    push_range.binding = 0;
    push_range.dx_register_index = 13;
    push_range.dx_register_space = 0;
    push_range.count = 11;  // con, width, height, denoise, motionOn, threshold, range, response, mult, debug, max
    push_range.visibility = DS::all_compute;
    P p0, p1, pPush;
    p0.type = reshade::api::pipeline_layout_param_type::descriptor_table;
    p0.descriptor_table.count = 1;
    p0.descriptor_table.ranges = &srv_r;
    p1.type = reshade::api::pipeline_layout_param_type::descriptor_table;
    p1.descriptor_table.count = 1;
    p1.descriptor_table.ranges = &uav_r;
    pPush.type = reshade::api::pipeline_layout_param_type::push_constants;
    pPush.push_constants = push_range;
    P params[3] = {p0, p1, pPush};
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
  dv(d->fxaa_temp_srv);
  dv(d->fxaa_temp_uav);
  dr(d->fxaa_temp_texture);
  dv(d->fxaa_luma_srv);
  dv(d->fxaa_luma_uav);
  dr(d->fxaa_luma_texture);
  if (d->fxaa_hist_srv_owned) dv(d->fxaa_hist_srv);
  d->fxaa_hist_srv = {};
  d->fxaa_hist_srv_owned = false;
  d->fxaa_hist_seen = {};
  d->fxaa_w = 0u;
  d->fxaa_h = 0u;
  d->fxaa_fmt = reshade::api::format::unknown;
  d->fxaa_logged = false;
}

static void DestroyFXAAResources(reshade::api::device* dev, DeviceData* d) {
  if (!dev || !d) return;
  DestroyFXAASet(dev, d);
  for (auto& t : d->fxaa_tables) {
    if (t.handle) {
      dev->free_descriptor_table(t);
      t = {};
    }
  }
  if (d->fxaa_luma_pipeline.handle) {
    dev->destroy_pipeline(d->fxaa_luma_pipeline);
    d->fxaa_luma_pipeline = {};
  }
  if (d->fxaa_standard_pipeline.handle) {
    dev->destroy_pipeline(d->fxaa_standard_pipeline);
    d->fxaa_standard_pipeline = {};
  }
  if (d->fxaa_high_pipeline.handle) {
    dev->destroy_pipeline(d->fxaa_high_pipeline);
    d->fxaa_high_pipeline = {};
  }
  if (d->fxaa_layout.handle) {
    dev->destroy_pipeline_layout(d->fxaa_layout);
    d->fxaa_layout = {};
  }
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
  d->fxaa_w = w;
  d->fxaa_h = h;
  d->fxaa_fmt = fmt;
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
    sd.code = bc.data();
    sd.code_size = bc.size();
    sd.entry_point = "main";
    reshade::api::pipeline_subobject so = {reshade::api::pipeline_subobject_type::compute_shader, 1, &sd};
    return dev->create_pipeline(lo, 1, &so, out);
  };
  if (d->fxaa_layout.handle == 0u || d->fxaa_layout_version != kFXAALayoutVersion) {
    if (d->fxaa_layout.handle) {
      for (auto& t : d->fxaa_tables) {
        if (t.handle) {
          dev->free_descriptor_table(t);
          t = {};
        }
      }
      dev->destroy_pipeline_layout(d->fxaa_layout);
      d->fxaa_layout = {};
      if (d->fxaa_luma_pipeline.handle) {
        dev->destroy_pipeline(d->fxaa_luma_pipeline);
        d->fxaa_luma_pipeline = {};
      }
      if (d->fxaa_standard_pipeline.handle) {
        dev->destroy_pipeline(d->fxaa_standard_pipeline);
        d->fxaa_standard_pipeline = {};
      }
      if (d->fxaa_high_pipeline.handle) {
        dev->destroy_pipeline(d->fxaa_high_pipeline);
        d->fxaa_high_pipeline = {};
      }
    }
    DR srv_r = {0, 0, 0, 2, DS::all_compute, 1, DT::texture_shader_resource_view};   // t0 color copy, t1 luma
    DR uav_r = {0, 0, 0, 1, DS::all_compute, 1, DT::texture_unordered_access_view};  // u0 target
    reshade::api::constant_range push_range = {};
    push_range.binding = 0;
    push_range.dx_register_index = 13;
    push_range.dx_register_space = 0;
    push_range.count = 6;  // width, height, subpix, edgeThreshold, edgeThresholdMin, lumaMode
    push_range.visibility = DS::all_compute;
    P p0, p1, pPush;
    p0.type = reshade::api::pipeline_layout_param_type::descriptor_table;
    p0.descriptor_table.count = 1;
    p0.descriptor_table.ranges = &srv_r;
    p1.type = reshade::api::pipeline_layout_param_type::descriptor_table;
    p1.descriptor_table.count = 1;
    p1.descriptor_table.ranges = &uav_r;
    pPush.type = reshade::api::pipeline_layout_param_type::push_constants;
    pPush.push_constants = push_range;
    P params[3] = {p0, p1, pPush};
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
                             + " work=" + std::to_string((int)RCASLinearFormat(fmt)))
                                .c_str());
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
                                 + " luma=" + (hdr ? "hdr" : "ldr"))
                                    .c_str());
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
                             + " src=" + std::to_string(srcRes.handle))
                                .c_str());
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
  // Rebuild requested by the resource-destroy event: drop the dead view first, on
  // this thread, then fall through and make a new one.
  if (d->mb_rtv4_owned_dirty.exchange(false, std::memory_order_acq_rel)) {
    if (d->mb_rtv4_owned_srv.handle != 0u) {
      dev->destroy_resource_view(d->mb_rtv4_owned_srv);
      d->mb_rtv4_owned_srv = {};
    }
  }
  if (d->mb_rtv4_srv.handle == 0u || !d->mb_rtv4_live.load()) return;
  reshade::api::resource mres = dev->get_resource_from_view(d->mb_rtv4_srv);
  if (mres.handle == 0u) return;
  if (mres.handle == d->mb_rtv4_res.load(std::memory_order_acquire)
      && d->mb_rtv4_owned_srv.handle != 0u) return;

  if (d->mb_rtv4_owned_srv.handle != 0u) {
    dev->destroy_resource_view(d->mb_rtv4_owned_srv);
    d->mb_rtv4_owned_srv = {};
  }
  d->mb_rtv4_res.store(mres.handle, std::memory_order_release);

  // .texture is a union member, so .type has to be tested before it is read.
  // Reading it on a buffer desc is undefined behaviour, and undefined behaviour
  // resolves differently under Release optimisation.
  auto rdesc = dev->get_resource_desc(mres);
  const bool isTexture = (rdesc.type == reshade::api::resource_type::texture_2d);
  const bool isMotionFormat = isTexture && (rdesc.texture.format == reshade::api::format::r16g16_float);
  const bool isPlausibleSize = isTexture && (rdesc.texture.width > 0u && rdesc.texture.height > 0u);

  if (!isMotionFormat || !isPlausibleSize) {
    d->mb_rtv4_res.store(0u, std::memory_order_release);
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
                             "("
                             + got + "). Expected r16g16_float. Blur is off this frame and the "
                                     "capture is retried; this message appears once.")
                                .c_str());
    }
    return;
  }
  dev->create_resource_view(mres, reshade::api::resource_usage::shader_resource,
                            reshade::api::resource_view_desc(reshade::api::resource_view_type::texture_2d,
                                                             rdesc.texture.format, 0, 1, 0, 1),
                            &d->mb_rtv4_owned_srv);
  if (d->mb_rtv4_owned_srv.handle == 0u) {
    d->mb_rtv4_res.store(0u, std::memory_order_release);
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
    return (d->mb_rtv4_live.load() && d->mb_rtv4_owned_srv.handle != 0u)
               ? d->mb_rtv4_owned_srv
               : reshade::api::resource_view{};
  }
  return (d->rcas_motion_srv.handle != 0u && d->rcas_motion_live.load())
             ? d->rcas_motion_srv
             : reshade::api::resource_view{};
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
// Run inline on the deploy draw's own command list so ordering
// against the game's own work is strict and no frame of latency is added:
//
//   tilemax -> neighbormax -> gather
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
  dv(d->mb_tilemax_srv);
  dv(d->mb_tilemax_uav);
  dr(d->mb_tilemax_texture);
  dv(d->mb_neighbormax_srv);
  dv(d->mb_neighbormax_uav);
  dr(d->mb_neighbormax_texture);
  dv(d->mb_half_color_srv);
  dv(d->mb_half_color_uav);
  dr(d->mb_half_color_texture);
  dv(d->mb_half_result_srv);
  dv(d->mb_half_result_uav);
  dr(d->mb_half_result_texture);
  dv(d->mb_output_srv);
  dv(d->mb_output_uav);
  dr(d->mb_output_texture);
  dv(d->mb_resolve_srv);
  dv(d->mb_resolve_uav);
  dr(d->mb_resolve_texture);
  d->mb_working_w = 0u;
  d->mb_working_h = 0u;
  d->mb_motion_w = 0u;
  d->mb_motion_h = 0u;
  d->mb_tiles_x = 0u;
  d->mb_tiles_y = 0u;
  d->mb_radius_px = 0u;
  d->mb_output_fmt = reshade::api::format::unknown;
  d->mb_halfres = false;
  // Active width, not the capability latch: mb_resolve_narrow_failed survives a
  // set rebuild on purpose, so a device that refused RG16F is not asked again.
  d->mb_resolve_narrow = false;
  d->mb_resources_ready = false;
  d->mb_prep_valid = false;
}

// Full teardown, including the immutable per-device pieces. Called from device
// teardown only; the day-to-day path is DestroyMotionBlurSet.
static void DestroyMotionBlurResources(reshade::api::device* dev, DeviceData* d) {
  if (!dev || !d) return;
  DestroyMotionBlurSet(dev, d);
  // Full teardown, so the RG16F capability latch is re-probed on the next set.
  d->mb_resolve_narrow_failed = false;
  for (auto& set : d->mb_tables) {
    for (auto& t : set) {
      if (t.handle) {
        dev->free_descriptor_table(t);
        t = {};
      }
    }
    set = {};
  }
  for (auto& p : d->mb_pipelines) {
    if (p.handle) {
      dev->destroy_pipeline(p);
      p = {};
    }
  }
  for (auto& l : d->mb_layouts) {
    if (l.handle) {
      dev->destroy_pipeline_layout(l);
      l = {};
    }
  }
  if (d->mb_noise_fallback_srv.handle) {
    dev->destroy_resource_view(d->mb_noise_fallback_srv);
    d->mb_noise_fallback_srv = {};
  }
  if (d->mb_noise_fallback_res.handle) {
    dev->destroy_resource(d->mb_noise_fallback_res);
    d->mb_noise_fallback_res = {};
  }
  if (d->mb_point_clamp_sampler.handle) {
    dev->destroy_sampler(d->mb_point_clamp_sampler);
    d->mb_point_clamp_sampler = {};
  }
  // Our SRV on the game's motion resource. Not part of mb_layouts/mb_tables: it is
  // a view on a game-owned resource, and those are freed on a set rebuild, not on
  // device teardown.
  if (d->mb_rtv4_owned_srv.handle) {
    dev->destroy_resource_view(d->mb_rtv4_owned_srv);
    d->mb_rtv4_owned_srv = {};
  }
  d->mb_rtv4_res.store(0u, std::memory_order_release);
  d->mb_rtv4_owned_dirty.store(false, std::memory_order_release);
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
  static constexpr uint32_t kSrvPerPass[kMotionBlurPassCount] = {1u, 1u, 1u, 6u, 2u, 2u, 2u};
  static const std::span<const uint8_t> kBytecode[kMotionBlurPassCount] = {
      __motion_blur_downsample, __motion_blur_tilemax, __motion_blur_neighbormax,
      __motion_blur_gather, __motion_blur_composite, __motion_blur_resolve,
      __motion_blur_resolve_wide};
  for (uint32_t pass = 0; pass < kMotionBlurPassCount; ++pass) {
    if (d->mb_layouts[pass].handle == 0u) {
      DR sampler_r = {0, 0, 0, 1, DS::all_compute, 1, DT::sampler};
      DR cbv_r = {0, 0, 0, 1, DS::all_compute, 1, DT::constant_buffer};
      DR srv_r = {0, 0, 0, kSrvPerPass[pass], DS::all_compute, 1, DT::texture_shader_resource_view};
      DR uav_r = {0, 0, 0, 1, DS::all_compute, 1, DT::texture_unordered_access_view};
      reshade::api::constant_range push_range = {};
      push_range.binding = 0;
      push_range.dx_register_index = 13;
      push_range.dx_register_space = 0;
      push_range.count = static_cast<uint32_t>(sizeof(ShaderInjectData) / sizeof(uint32_t));
      push_range.visibility = DS::all_compute;
      P param_sampler, param_cbv, param_srv, param_uav, param_constants;
      param_sampler.type = reshade::api::pipeline_layout_param_type::descriptor_table;
      param_sampler.descriptor_table.count = 1;
      param_sampler.descriptor_table.ranges = &sampler_r;
      param_cbv.type = reshade::api::pipeline_layout_param_type::descriptor_table;
      param_cbv.descriptor_table.count = 1;
      param_cbv.descriptor_table.ranges = &cbv_r;
      param_srv.type = reshade::api::pipeline_layout_param_type::descriptor_table;
      param_srv.descriptor_table.count = 1;
      param_srv.descriptor_table.ranges = &srv_r;
      param_uav.type = reshade::api::pipeline_layout_param_type::descriptor_table;
      param_uav.descriptor_table.count = 1;
      param_uav.descriptor_table.ranges = &uav_r;
      param_constants.type = reshade::api::pipeline_layout_param_type::push_constants;
      param_constants.push_constants = push_range;
      P params[5] = {param_sampler, param_cbv, param_srv, param_uav, param_constants};
      if (!dev->create_pipeline_layout(5, params, &d->mb_layouts[pass])) return false;
    }
    if (!EnsureGTVBAODescriptorTables(dev, d->mb_layouts[pass], &d->mb_tables[pass])) return false;
    if (d->mb_pipelines[pass].handle == 0u) {
      // THREE resolve variants, each in a narrow and a wide form (six bytecodes,
      // two pass slots). HLSL packoffset() is a compile-time constant and the
      // three games disagree on the scene cbuffer layout:
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
      if (pass == kMbResolve || pass == kMbResolveWide) {
        // Wide is the same source with MB_RESOLVE_WIDE defined, so the game
        // variant still has to be picked here as well.
        const bool wide = (pass == kMbResolveWide);
        if (IsKai())
          code = wide ? __motion_blur_resolve_kai_wide : __motion_blur_resolve_kai;
        else if (IsSora1st())
          code = wide ? __motion_blur_resolve_sora1st_wide : __motion_blur_resolve_sora1st;
        else if (wide)
          code = __motion_blur_resolve_wide;
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
                                reshade::api::format outputFormat, bool halfRes,
                                bool narrowResolve) {
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
  // The tile set is tiny (2*tiles*tiles texels), so
  // its extra bytes are irrelevant and it stays RGBA16F unconditionally. The
  // resolve surface is the opposite case: the gather fetches it once per tap, so
  // it is allocated RG16F -- the filter reads only .xy -- whenever the device
  // accepts the UAV. See the allocation below.
  const auto tileFmt = reshade::api::format::r16g16b16a16_float;
  bool ok =
      mk(tilesX, tilesY, tileFmt,
         &d->mb_tilemax_texture, &d->mb_tilemax_srv, &d->mb_tilemax_uav)
      && mk(tilesX, tilesY, tileFmt,
            &d->mb_neighbormax_texture, &d->mb_neighbormax_srv, &d->mb_neighbormax_uav)
      && mk(workingW, workingH, outputFormat,
            &d->mb_output_texture, &d->mb_output_srv, &d->mb_output_uav);
  // The camera/object split output, at the MOTION texture's resolution (which is
  // not necessarily workingW/H). .xy is the blended velocity every pass above
  // reduces or samples; .zw is camera-only, read by the two camera-term debug
  // views only.
  //
  // Narrow (RG16F) is the shipping format: the filter reads .xy and nothing
  // else, and the stored half values are identical to the wide surface's, so
  // this is a pure bandwidth change on the most-fetched texture in the chain.
  // The caller requests wide while Debug View 8 or 9 is selected, because those
  // read .zw. A device without the FL11.1 typed-UAV store that RG16F needs falls
  // back here once, latched in mb_resolve_narrow_failed so the recreate test
  // cannot loop.
  bool narrow = false;
  if (ok) {
    const uint32_t resolveW = std::max(motionW, 1u), resolveH = std::max(motionH, 1u);
    if (narrowResolve) {
      narrow = mk(resolveW, resolveH, reshade::api::format::r16g16_float,
                  &d->mb_resolve_texture, &d->mb_resolve_srv, &d->mb_resolve_uav);
    }
    if (!narrow) {
      // mk() leaves whichever handles it did create in place when a later step
      // fails, so tear the partial attempt down before overwriting the pointers.
      auto dv = [&](reshade::api::resource_view& v) { if (v.handle) { dev->destroy_resource_view(v); v = {}; } };
      auto dr = [&](reshade::api::resource& r) { if (r.handle) { dev->destroy_resource(r); r = {}; } };
      dv(d->mb_resolve_srv);
      dv(d->mb_resolve_uav);
      dr(d->mb_resolve_texture);
      ok = mk(resolveW, resolveH, tileFmt,
              &d->mb_resolve_texture, &d->mb_resolve_srv, &d->mb_resolve_uav);
    }
    if (narrowResolve && !narrow) d->mb_resolve_narrow_failed = true;
  }
  // Half-res surfaces are optional and only exist while the toggle is on, so the
  // default path allocates nothing extra. They use the blit's own format so the
  // gather input and output match what the final reconstruct expects.
  const uint32_t halfW = std::max(1u, (workingW + 1u) / 2u);
  const uint32_t halfH = std::max(1u, (workingH + 1u) / 2u);
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
  d->mb_working_w = workingW;
  d->mb_working_h = workingH;
  d->mb_motion_w = motionW;
  d->mb_motion_h = motionH;
  d->mb_tiles_x = tilesX;
  d->mb_tiles_y = tilesY;
  d->mb_radius_px = static_cast<uint32_t>(std::max(shader_injection.mb_max_radius_px, 8.f));
  d->mb_output_fmt = outputFormat;
  d->mb_halfres = halfRes;
  d->mb_resolve_narrow = narrow;
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
  if (!d->captured_depth_live.load() || !d->captured_cbv_live.load()) return false;
  if (!EnsureMotionBlurPipelines(dev, d)) return false;
  if (!EnsureMotionBlurSampler(dev, d)) return false;
  if (!EnsureMotionBlurNoiseFallback(dev, d)) return false;

  // Depth: the game's scene depth, captured from the lighting pass. Queried
  // here rather than cached at capture time so it can never go stale.
  uint32_t depthW = 0u, depthH = 0u;
  if (d->captured_depth_live.load() && d->captured_depth_srv.handle != 0u) {
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
                                 + std::to_string(motionH) + " bound as our own SRV")
                                    .c_str());
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
  const uint32_t tilePx = std::max(1u, radiusPx * 2u);
  const uint32_t tilesX = std::max(1u, (workingW + tilePx - 1u) / tilePx);
  const uint32_t tilesY = std::max(1u, (workingH + tilePx - 1u) / tilePx);
  // The half-res surfaces are part of the owned set, so toggling the setting has
  // to rebuild it. That is the same one-frame hitch Max Radius already causes and
  // is why the surfaces are not simply kept allocated while unused.
  const bool halfRes = shader_injection.mb_halfres > 0.5f;
  // The resolve surface is RG16F while neither camera-term debug view is
  // selected -- those read the camera-only .zw, which only RGBA16F carries -- and
  // the capability fallback lives in CreateMotionBlurSet. The latch means a
  // device that refused RG16F asks for wide for good instead of rebuilding the
  // set every frame.
  const int debugView = static_cast<int>(shader_injection.mb_debug_view + 0.5f);
  const bool wantNarrow = !d->mb_resolve_narrow_failed && (debugView != 8 && debugView != 9);
  // Depth and motion dimensions are no longer resource-sizing inputs for the
  // gather: it samples the game's textures directly and the conversion scale is a
  // per-frame push. motionW and motionH now size the camera/object resolve
  // output, which is full motion resolution.
  const bool needRecreate =
      !d->mb_resources_ready
      || d->mb_working_w != workingW || d->mb_working_h != workingH
      || d->mb_motion_w != motionW || d->mb_motion_h != motionH
      || d->mb_tiles_x != tilesX || d->mb_tiles_y != tilesY
      || d->mb_radius_px != radiusPx
      || d->mb_halfres != halfRes
      || d->mb_output_fmt != outputFormat
      || d->mb_resolve_narrow != wantNarrow;
  if (needRecreate) {
    CreateMotionBlurSet(dev, d, workingW, workingH, motionW, motionH, tilesX, tilesY, outputFormat,
                        halfRes, wantNarrow);
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
                             + " | resolve " + (d->mb_resolve_narrow ? "RG16F" : "RGBA16F")
                             + " | source " + MBMotionInputName()
                             + (motionDiffers
                                    ? " -- motion differs, gather is using filtered reads"
                                    : " -- motion matches, single-tap reads")
                             + (depthDiffers ? " (depth differs, point sampled by design)" : ""))
                                .c_str());
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
                             + " (reference " + std::to_string(static_cast<int>(shader_injection.mb_frame_rate_reference)) + " fps)")
                                .c_str());
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
  if (!dev || !d->mb_resources_ready
      || !d->captured_depth_live.load() || !d->captured_cbv_live.load()) return {};

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
        {{}, 0, 0, 1, reshade::api::descriptor_type::sampler, &d->mb_point_clamp_sampler},
        {{}, 0, 0, 1, reshade::api::descriptor_type::constant_buffer, &d->captured_scene_cbv_view},
        {{}, 0, 0, srvCount, reshade::api::descriptor_type::texture_shader_resource_view, srvs},
        {{}, 0, 0, 1, reshade::api::descriptor_type::texture_unordered_access_view, &uav},
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
  // TileMax dispatch), and because "Gather Only" must not touch passes 1-5 at all.
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
                            chain == 0   ? "[MotionBlur] debug chain: Full"
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
    // before prep because everything downstream reduces what it writes. Narrow is
    // the shipping bytecode: RG16F output, .xy only. Wide writes the same values
    // plus the .zw camera term, and PrepareMotionBlur allocated the surface to
    // match this selection, so the two cannot disagree.
    const uint32_t resolvePass = d->mb_resolve_narrow ? kMbResolve : kMbResolveWide;
    cl->bind_pipeline(AC, d->mb_pipelines[resolvePass]);
    reshade::api::resource_view srvs[2] = {motionSrc, depthSrc};
    apply(resolvePass, srvs, 2, d->mb_resolve_uav);
    cl->dispatch((std::max(d->mb_motion_w, 1u) + 7u) / 8u, (std::max(MH, 1u) + 7u) / 8u, 1u);
    bar(d->mb_resolve_texture, UA, SR);
  }
  // What tilemax and the gather read. TileMax declares float2 and takes .xy; the
  // gather declares float4 and takes .zw for the velocity views.
  const reshade::api::resource_view motionForChain = runResolve ? d->mb_resolve_srv : motionSrc;

  if (runPrep) {
    {  // P1: TileMax over the blended motion, one thread group per tile (paper Section 3)
      cl->bind_pipeline(AC, d->mb_pipelines[kMbTileMax]);
      reshade::api::resource_view srvs[1] = {motionForChain};
      apply(kMbTileMax, srvs, 1, d->mb_tilemax_uav);
      cl->dispatch(TX, TY, 1u);
      bar(d->mb_tilemax_texture, UA, SR);
    }
    {  // P2: NeighborMax, 3x3 one-ring with Section 4.4 diagonal culling
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
                                            ? d->isfast_noise_srv
                                            : d->mb_noise_fallback_srv;

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
                             "Pixel hash 0x"
                             + seenHex.str() + ", deploy gate expects 0x"
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
  ComputeDispatchFixMode fix = ComputeDispatchFixMode::OFF;
  switch (static_cast<int>(g_mb_cs_dispatch_fix)) {
    case 1:  fix = ComputeDispatchFixMode::RESTORE_STATE; break;
    case 2:
    case 3:  fix = ComputeDispatchFixMode::NULL_AND_RESTORE; break;
    default: break;
  }
  auto* cs = renodx::utils::state::GetCurrentState(cmd_list);
  renodx::utils::state::CommandListState prev = {};
  if (cs) prev = *cs;
  auto blurred = RunMotionBlur(cmd_list, d, d->mb_tonemap_src_srv, motionSrc);
  ApplyComputeDispatchFix(cmd_list, cs, prev, fix, 6u, 1u);
  if (!blurred.handle) return true;

  cmd_list->push_descriptors(
      reshade::api::shader_stage::pixel, reshade::api::pipeline_layout{0}, 0,
      reshade::api::descriptor_table_update{{}, 0u, 0, 1, reshade::api::descriptor_type::texture_shader_resource_view, &blurred});
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
            {}, kDynCubeRegister, 0, 1, reshade::api::descriptor_type::texture_shader_resource_view, &t17srv});
  }
  if (dd->dyncube_hist[dd->dyncube_readSet].pos_cube_srv.handle) {
    auto histPosSrv = dd->dyncube_hist[dd->dyncube_readSet].pos_cube_srv;
    cmd_list->push_descriptors(
        reshade::api::shader_stage::pixel,
        reshade::api::pipeline_layout{0}, 0,
        reshade::api::descriptor_table_update{
            {}, kDynCubeHistPosRegister, 0, 1, reshade::api::descriptor_type::texture_shader_resource_view, &histPosSrv});
  }
  cmd_list->push_descriptors(
      reshade::api::shader_stage::pixel,
      reshade::api::pipeline_layout{0}, 0,
      reshade::api::descriptor_table_update{
          {}, kDynCubeVanillaRegister, 0, 1, reshade::api::descriptor_type::texture_shader_resource_view, &dd->captured_vanilla_env_srv});
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
              {}, 5u, 0, 1, reshade::api::descriptor_type::texture_shader_resource_view, &dd->isfast_noise_srv});
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

// Live BVH readiness for the inline RT passes (RTAO, RT Local Shadows): false with the reason when they cannot trace.
static bool RtBvhReady(const falcom_world::bvh::BvhDeviceData* bvh_data, falcom_world::rtao::Reason* reason) {
  if (bvh_data == nullptr) {
    *reason = falcom_world::rtao::Reason::BvhNotReady;
    return false;
  }
  if (!falcom_world::bvh::g_live_bvh.enabled.load(std::memory_order_relaxed)) {
    *reason = falcom_world::rtao::Reason::LiveBvhOff;
    return false;
  }
#if !defined(FALCOM_WINDOWS_HARNESS)
  if (!bvh_data->scene_catalog_ready || !bvh_data->tlas_built) {
#else
  if (!bvh_data->bvh_ready || !bvh_data->tlas_built) {
#endif
    *reason = falcom_world::rtao::Reason::BvhNotReady;
    return false;
  }
  if (bvh_data->active_count == 0u) {
    *reason = falcom_world::rtao::Reason::BvhEmpty;
    return false;
  }
  return true;
}

// This frame's G-buffer and scene inputs for the inline RT passes, and the startup and resize guard: false with the
// reason when they are missing.
static bool RtFrameInputsReady(const DeviceData* dd, falcom_world::rtao::Reason* reason) {
  if (!dd->captured_depth_srv.handle || !dd->captured_depth_live.load()
      || dd->captured_depth_w == 0u || dd->captured_depth_h == 0u) {
    *reason = falcom_world::rtao::Reason::NoDepth;
    return false;
  }
  if (!dd->captured_mrt_normal_srv.handle || !dd->captured_mrt_live.load()) {
    *reason = falcom_world::rtao::Reason::NoNormals;
    return false;
  }
  if (!dd->captured_scene_cbv_view.handle || !dd->captured_scene_cbv_valid || !dd->captured_cbv_live.load()) {
    *reason = falcom_world::rtao::Reason::NoSceneCbv;
    return false;
  }
  if (dd->frame_index < kGTVBAOStartupGuardFrames || dd->frame_index < dd->resize_guard_until_frame) {
    *reason = falcom_world::rtao::Reason::StartupOrResizeGuard;
    return false;
  }
  return true;
}

// Motion for the inline RT temporal passes (RTAO, RTLS, RTGI): with Temporal on, ensures the RTV4 owned SRV and checks
// its size against the depth; checks RTV4 against TAA t3 (a conflict stops Temporal for the session). The state lives in
// rtao::g_rtao_frame and is logged once. Returns whether the motion view can be used.
static bool UpdateRtMotion(reshade::api::device* dev, DeviceData* dd, bool temporal_on) {
  namespace rtao = falcom_world::rtao;
  auto& rf = rtao::g_rtao_frame;
  if (temporal_on) MBMotionRtv4Ensure(dev, dd);
  const uint64_t mb_rtv4_res = dd->mb_rtv4_res.load(std::memory_order_acquire);
  // Motion source (round 2). TAA t3 and RTV4 are different resources: t3 is not proven to be written before
  // lighting, so Temporal stops for the session. Unknown on either side: no decision yet.
  const rtao::MotionCheck motion_check = rtao::CheckMotionResources(dd->rcas_motion_res, mb_rtv4_res);
  rf.rcas_res = dd->rcas_motion_res;
  rf.rtv4_res = mb_rtv4_res;
  rf.depth_w = dd->captured_depth_w;
  rf.depth_h = dd->captured_depth_h;
  if (temporal_on && dd->mb_rtv4_live.load()
      && dd->mb_rtv4_owned_srv.handle != 0u) {
    const auto motion_desc = dev->get_resource_desc(dev->get_resource_from_view(dd->mb_rtv4_owned_srv));
    if (motion_desc.type == reshade::api::resource_type::texture_2d) {
      rf.motion_w = motion_desc.texture.width;
      rf.motion_h = motion_desc.texture.height;
      rf.motion_dims_ok = rf.motion_w == dd->captured_depth_w && rf.motion_h == dd->captured_depth_h;
      static bool dims_logged = false;
      if (!dims_logged) {
        dims_logged = true;
        renodx::utils::log::i("[world-rtao] motion dims ", rf.motion_w, "x", rf.motion_h, ", depth ",
                              dd->captured_depth_w, "x", dd->captured_depth_h,
                              rf.motion_dims_ok ? " (match)" : " (MISMATCH: motion treated as unavailable, no_motion)");
      }
    }
  }
  if (motion_check == rtao::MotionCheck::Conflict && !rf.motion_conflict) {
    rf.motion_conflict = true;
    renodx::utils::log::w("[world-rtao] motion CONFLICT: rcas_motion_res=", dd->rcas_motion_res,
                          " mb_rtv4_res=", mb_rtv4_res,
                          " differ. TAA t3 is not proven written before lighting; Temporal is off for this session (no_motion, round-1 AO).");
  }
  static bool motion_logged = false;
  if (!motion_logged && motion_check == rtao::MotionCheck::Ok && dd->rcas_motion_res != 0u && mb_rtv4_res != 0u) {
    motion_logged = true;
    renodx::utils::log::i("[world-rtao] motion: rcas_motion_res=", dd->rcas_motion_res,
                          " mb_rtv4_res=", mb_rtv4_res, " same resource: RTV4 owned SRV used");
  }
  return dd->mb_rtv4_live.load() && dd->mb_rtv4_owned_srv.handle != 0u
         && !rf.motion_conflict && rf.motion_dims_ok;
}

// RTAO round 1 (world/rtao): dispatched inline before DeployShadows so the RTAO
// consumers read this frame's AO. A failed guard leaves the frame on vanilla SSAO and
// records its reason in rtao::g_rtao_frame.
static void PushRtaoAo(reshade::api::command_list* cmd_list, reshade::api::resource_view ao_srv) {
  shader_injection.rtao_debug_show = falcom_world::rtao::g_rtao_debug > 0.5f ? 1.f : 0.f;
  cmd_list->push_descriptors(
      reshade::api::shader_stage::pixel,
      reshade::api::pipeline_layout{0},
      0,
      reshade::api::descriptor_table_update{
          {}, kLightingGtvbaoRegister, 0, 1, reshade::api::descriptor_type::texture_shader_resource_view, &ao_srv});
  shader_injection.rtao_char_intensity = falcom_world::rtao::g_rtao_char_intensity;
  shader_injection.rtao_ao_bound = 1.f;
  shader_injection.rtao_max_darkening = falcom_world::rtao::g_rtao_max_darkening;
  shader_injection.gtvbao_dedicated_bound = 1.f;
}

static void RunRtaoInline(reshade::api::command_list* cmd_list) {
  const falcom_world::bvh::PoolCpuTimer inline_timer{&falcom_world::bvh::g_perf_gate.block_us[falcom_world::bvh::kPerfBlockRtaoInline]};
  namespace rtao = falcom_world::rtao;
  shader_injection.rtao_debug_show = 0.f;  // set again by PushRtaoAo when RTAO produces this frame
  shader_injection.rtao_ao_bound = 0.f;
  auto& frame = rtao::g_rtao_frame;
  frame.producing = false;
  frame.reason = rtao::Reason::Off;
  frame.note = rtao::Reason::Off;
  if (!rtao::RtaoRequested()) {
    rtao::ReleaseRtaoTextures(cmd_list->get_device());
    return;
  }

  auto* dev = cmd_list->get_device();
  auto* dd = dev ? dev->get_private_data<DeviceData>() : nullptr;
  if (!dd) {
    frame.reason = rtao::Reason::NoDepth;
    return;
  }
  auto& rd = rtao::GetRtaoDeviceData(dev);
  auto& sd = rtao::GetRtaoSvgfData(dev);
  const auto* bvh_data = falcom_world::bvh::GetBvhDeviceData(dev);
  // G5: with the CPU pool reduced, DXR traces the GPU scene and the CPU TLAS may be stale or empty: the region follows the camera.
  const bool pool_reduced = falcom_world::bvh::g_gpu_capture.pool_reduced.load(std::memory_order_relaxed);
  if (pool_reduced && bvh_data == nullptr) {
    frame.reason = rtao::Reason::BvhNotReady;
    return;
  }
  if (!pool_reduced && !RtBvhReady(bvh_data, &frame.reason)) return;
  const falcom_world::bvh::PoolRegion camera_region = pool_reduced ? falcom_world::bvh::CurrentPoolRegion() : falcom_world::bvh::PoolRegion{};
  const float region_range = pool_reduced ? camera_region.range : bvh_data->tlas_region_range;
  const float region_size = pool_reduced ? camera_region.size : bvh_data->tlas_region_size;
  const float* region_min = pool_reduced ? camera_region.min : bvh_data->tlas_region_min;
  const auto fade = rtao::RtaoEffectiveFade(
      region_range, falcom_world::bvh::g_pool.region_size, rtao::g_rtao_fade_start_pct, rtao::g_rtao_fade_end_pct);
  frame.fade = fade;
  frame.fade_valid = true;
  if (fade.coverage < 1.f) {
    frame.reason = rtao::Reason::RangeTooSmall;
    return;
  }
  // Deferred lists may run after their slots are reused: keep the last AO only if it was written and matches the size.
  if (cmd_list != dd->immediate_cmd_list) {
    frame.reason = rtao::Reason::DeferredList;
    if (frame.dispatched_frame != UINT64_MAX && rtao::OutputSrv(dev).handle != 0u
        && sd.denoise.output.width == dd->captured_depth_w && sd.denoise.output.height == dd->captured_depth_h) {
      frame.producing = true;
      PushRtaoAo(cmd_list, rtao::OutputSrv(dev));
    }
    return;
  }
  if (frame.dispatched_frame == dd->frame_index) {
    frame.producing = true;
    PushRtaoAo(cmd_list, rtao::OutputSrv(dev));
    return;
  }
  if (!RtFrameInputsReady(dd, &frame.reason)) return;

  // IS-FAST blue noise is used when its texture is loaded and the IS-FAST master is on; otherwise IGN.
  if (rtao::g_rtao_isfast > 0.5f && g_isfast_enabled > 0.5f) LoadISFASTNoiseTexture(dev, dd);
  const bool isfast_on = g_isfast_enabled > 0.5f && dd->isfast_noise_srv.handle != 0u;
  const bool isfast_used = isfast_on && rtao::g_rtao_isfast > 0.5f;
  if (rtao::g_rtao_isfast > 0.5f && !isfast_used) frame.note = rtao::Reason::IsfastUnavailable;

  const float spp = (std::max)(rtao::g_rtao_samples, 1.f);
  rtao::RtaoDispatchInputs in = {};
  in.depth_view = dd->captured_depth_srv;
  in.mrt_normal_view = dd->captured_mrt_normal_srv;
  in.isfast_view = isfast_used ? dd->isfast_noise_srv : reshade::api::resource_view{0u};
  in.scene_cbv_view = dd->captured_scene_cbv_view;
  in.width = dd->captured_depth_w;
  in.height = dd->captured_depth_h;
  in.trace_divisor = rtao::RtgiTraceDivisor(rtao::g_rtao_trace_resolution);
  const rtao::NoiseSample noise = rtao::RtaoNoiseSample(dd->frame_index, static_cast<uint32_t>(spp),
                                                        rtao::g_test_freeze_noise > 0.5f);
  const uint32_t rtao_flags = (rtao::g_rtao_discovery > 0.5f ? rtao::kRtaoFlagDiscovery : 0u)
                              | (rtao::g_rtao_foliage_skip > 0.5f ? rtao::kRtaoFlagFoliageSkip : 0u);  // RTAO_FLAG_* (c2.z)
  in.push = {
      {rtao::g_rtao_radius, rtao::g_rtao_ray_max, rtao::g_rtao_strength, rtao::g_rtao_normal_bias},
      {spp, isfast_used ? 1.f : 0.f, noise.slice_base, noise.seed},
      {fade.start, fade.end, static_cast<float>(rtao_flags), 0.f},
      {region_min[0], region_min[1], region_min[2], region_size},
      {0.f, 0.f, 0.f, 0.f},
  };

  // Temporal (round 2). The motion view is the G-buffer RTV4 owned SRV only. TAA t3 (rcas_motion) is bound
  // after lighting, so it is not used: there is no proof it is written before this hook.
  auto& rf = rtao::g_rtao_frame;
  const bool rr_bypass = falcom_world::rr::RrOwnsDenoise();
  const bool temporal_on = rtao::g_rtao_temporal_enabled > 0.5f;
  if (temporal_on) rf.temporal_on_frames += 1u;
  static bool taa_never_logged = false;
  if (!taa_never_logged && rf.temporal_on_frames >= 600u && dd->rcas_motion_res == 0u) {
    taa_never_logged = true;
    renodx::utils::log::w("[world-rtao] TAA t3 never captured (rcas_motion_res=0): RTV4 motion convention not verified against TAA");
  }
  const bool motion_ok = UpdateRtMotion(dev, dd, temporal_on);
  const rtao::ParameterSnapshot params = {
      rtao::g_rtao_radius, rtao::g_rtao_ray_max, rtao::g_rtao_strength, rtao::g_rtao_samples,
      rtao::g_rtao_normal_bias, rtao::g_rtao_two_sided, rtao::g_rtao_isfast, rtao::g_rtao_fade_start_pct,
      rtao::g_rtao_fade_end_pct, rtao::g_rtao_temporal_enabled, rtao::g_rtao_history_weight,
      rtao::g_rtao_depth_rejection, rtao::g_rtao_normal_rejection, rtao::g_rtao_debug, rtao::g_test_freeze_noise,
      rtao::g_rtao_foliage_skip, rtao::g_rtao_trace_characters, rtao::g_rtao_trace_resolution, rtao::g_rtao_svgf_history_length};
  const bool params_changed = rf.prev_params_valid && !(params == rf.prev_params);
  if (params_changed) rf.last_parameter_change = rtao::ParameterFieldName(params, rf.prev_params);
  rf.prev_params = params;
  rf.prev_params_valid = true;
  // The denoised output is not the size of this frame (Resize): the next frame restarts the history.
  const bool resized = rtao::OutputSrv(dev).handle != 0u && (sd.denoise.output.width != in.width || sd.denoise.output.height != in.height);
  if (resized) rf.dispatched_frame = UINT64_MAX;
  const rtao::ResetReason reset = rtao::TemporalResetReason(rtao::TemporalFrame{
      temporal_on, rf.temporal_last, resized, dd->frame_index, rf.dispatched_frame,
      dd->dyncube_loadingWipePending || dd->frame_index < dd->resize_guard_until_frame,
      false, params_changed, motion_ok});
  rf.reset = rr_bypass ? rtao::ResetReason::Disabled : reset;
  rf.motion_source = rf.motion_conflict ? rtao::MotionSource::Conflict
                                        : (motion_ok ? rtao::MotionSource::Rtv4 : rtao::MotionSource::None);
  if (!rr_bypass && temporal_on && reset != rtao::ResetReason::None) {
    rf.last_reset = reset;
    rf.reset_counts[static_cast<size_t>(reset)] += 1u;
  }

  // SVGF glue (rtao_svgf.hpp): the chain is ensured before pass A; a failure produces no AO this frame.
  // Debug Raw AO (1) runs neither the temporal nor the spatial pass.
  const bool debug_raw = rtao::g_rtao_debug > 0.5f && rtao::g_rtao_debug < 1.5f;
  const bool spatial_on = rtao::g_rtao_spatial_enabled > 0.5f;
  rtao::RtaoSvgfInputs svgf_in = {};
  svgf_in.depth_view = dd->captured_depth_srv;
  svgf_in.mrt_normal_view = dd->captured_mrt_normal_srv;
  svgf_in.scene_cbv_view = dd->captured_scene_cbv_view;
  svgf_in.motion_view = motion_ok ? dd->mb_rtv4_owned_srv : reshade::api::resource_view{0u};
  svgf_in.full_width = dd->captured_depth_w;
  svgf_in.full_height = dd->captured_depth_h;
  svgf_in.trace_width = rtao::RtgiTraceExtent(in.width, in.trace_divisor);
  svgf_in.trace_height = rtao::RtgiTraceExtent(in.height, in.trace_divisor);
  svgf_in.divisor = in.trace_divisor;
  svgf_in.temporal = temporal_on && motion_ok && !debug_raw;
  svgf_in.spatial = spatial_on && !debug_raw;
  svgf_in.bypass_svgf = rr_bypass;
  svgf_in.stats = rtao::g_rtao_discovery < 0.5f;
  svgf_in.svgf = {rtao::g_rtao_history_weight, rtao::g_rtao_depth_rejection, rtao::g_rtao_normal_rejection,
                  !rr_bypass && reset == rtao::ResetReason::None, rtao::g_rtao_svgf_iterations,
                  rtao::g_rtao_svgf_luminance_edge_stop, rtao::g_rtao_svgf_depth_sigma, rtao::g_rtao_svgf_normal_power, rtao::g_rtao_full_res_denoise > 0.5f, rtao::g_rtao_svgf_history_length, rtao::g_rtao_svgf_radius_scale};
  const falcom_world::rtgi::RtgiDenoiseReady ready = rtao::EnsureRtaoSvgf(dev, svgf_in);
  if (!ready.ok) {
    rf.temporal_ran = false;
    rf.temporal_last = false;
    frame.reason = rtao::Reason::PipelineFailed;
    return;
  }

  const bool rtgi_characters = falcom_world::rtgi::g_rtgi_enabled > 0.5f && falcom_world::rtgi::g_rtgi_trace_characters > 0.5f;
  if (rtao::g_rtao_trace_characters > 0.5f || rtgi_characters) {
    const falcom_world::bvh::DynamicTraceInputs characters = falcom_world::bvh::RefitDeformLiveCharacters(dev, cmd_list);
    in.scene_characters = characters.count;
    if (rtao::g_rtao_trace_characters > 0.5f) in.dynamic = characters;
  }
  const rtao::RtaoDispatchResult result = rtao::Dispatch(dev, cmd_list, in);
  if (!result.ok) {
    rf.temporal_ran = false;
    rf.temporal_last = false;
    frame.reason = result.dxr_unavailable ? rtao::Reason::DxrUnavailable
                                          : (result.pool_reduced ? rtao::Reason::PoolReduced : rtao::Reason::PipelineFailed);
    return;
  }
  rtao::DispatchRtaoSvgf(dev, cmd_list, svgf_in, ready);
  rf.temporal_ran = ready.temporal;
  rf.temporal_last = temporal_on && ready.temporal;
  // Temporal requested but the reprojection did not run: the output is the spatial-only or raw AO this frame.
  if (!svgf_in.bypass_svgf && temporal_on && !debug_raw && !ready.temporal) frame.note = motion_ok ? rtao::Reason::PipelineFailed : rtao::Reason::NoMotion;
  if (!svgf_in.bypass_svgf && spatial_on && !debug_raw && !ready.spatial && frame.note == rtao::Reason::Off) frame.note = rtao::Reason::FilterFailed;
  frame.dispatched_frame = dd->frame_index;
  frame.producing = true;
  frame.gpu_ms = rd.timer.last_ms;
  frame.denoise_gpu_ms = sd.denoise.timer.last_ms;
  frame.filter_ran = ready.spatial;
  frame.svgf_iterations = sd.denoise.atrous_iterations;
  frame.svgf_stats = svgf_in.stats;
  frame.captured = params;
  frame.captured_isfast_used = isfast_used;
  const uint64_t trace_pixels = static_cast<uint64_t>(rf.trace_width) * rf.trace_height;
  frame.texture_bytes = sizeof(uint32_t) * rtao::kRtaoStatsCount + trace_pixels * 2u + trace_pixels * 8u
                        + falcom_world::rtgi::RtgiDenoiseTextureBytes(sd.denoise);
  PushRtaoAo(cmd_list, rtao::OutputSrv(dev));
}

// RT Local Shadows (world/rtls): independent of RTAO and RTGI. Binds this frame's term at t38 for the Sora 2nd lighting draw.
// Kept as a function: its guard returns must end only the RTLS stage, not the lighting hook.
static void RunRtlsInline(reshade::api::command_list* cmd_list) {
  namespace rtls = falcom_world::rtls;
  namespace rtao = falcom_world::rtao;
  namespace rtgi = falcom_world::rtgi;
  auto& frame = rtls::g_rtls_frame;
  frame.producing = false;
  frame.reason = rtls::Reason::Off;
  frame.input_reason = rtao::Reason::Off;  // set again only by a failing guard this call
  frame.isfast_unavailable = false;
  shader_injection.rtls_flags = 0.f;  // set again below: requested (contact march off), and bound when this draw gets the term
  if (!rtls::RtlsRequested()) {
    rtls::ReleaseRtlsTextures(cmd_list->get_device());
    return;
  }
  shader_injection.rtls_flags = static_cast<float>(rtls::kRtlsFlagRequested);
  if (!IsSora2nd()) {
    frame.reason = rtls::Reason::NotSora2nd;
    return;
  }
  auto* dev = cmd_list->get_device();
  auto* dd = dev ? dev->get_private_data<DeviceData>() : nullptr;
  if (!dd) {
    frame.reason = rtls::Reason::InputsMissing;
    frame.input_reason = rtao::Reason::NoDepth;
    return;
  }
  const auto* bvh_data = falcom_world::bvh::GetBvhDeviceData(dev);
  if (!RtBvhReady(bvh_data, &frame.input_reason)) {
    frame.reason = rtls::Reason::InputsMissing;
    return;
  }
  if (cmd_list != dd->immediate_cmd_list) {
    frame.reason = rtls::Reason::DeferredList;
    return;
  }
  // Binds this frame's RTLS term (the denoised output, full size) at t38 for this lighting draw, also on a second
  // lighting draw of the frame, with the lighting kind bits and the Strength and Max Darkening values.
  const auto push_rtls = [&]() {
    const rtls::RtlsDeviceData& data = rtls::GetRtlsDeviceData(dev);
    const reshade::api::resource_view term_srv = data.denoise.output.srv[data.denoise.output_index];
    cmd_list->push_descriptors(
        reshade::api::shader_stage::pixel,
        reshade::api::pipeline_layout{0},
        0,
        reshade::api::descriptor_table_update{
            {}, kLightingRtlsRegister, 0, 1, reshade::api::descriptor_type::texture_shader_resource_view, &term_srv});
    shader_injection.rtls_flags = static_cast<float>(
        rtls::kRtlsFlagRequested | rtls::kRtlsFlagBound
        | (rtls::g_rtls_point > 0.5f ? rtls::kRtlsFlagPoint : 0u)
        | (rtls::g_rtls_spot > 0.5f ? rtls::kRtlsFlagSpot : 0u)
        | (rtls::g_rtls_env_point > 0.5f ? rtls::kRtlsFlagEnvPoint : 0u)
        | (rtls::g_rtls_env_spot > 0.5f ? rtls::kRtlsFlagEnvSpot : 0u)
        | (rtls::g_rtls_characters > 0.5f ? rtls::kRtlsFlagCharacter : 0u));
    shader_injection.rtls_strength = rtls::g_rtls_strength;
    shader_injection.rtls_max_darkening = rtls::g_rtls_max_darkening;
    shader_injection.rtls_char_lamp = rtls::g_rtls_character_lamp;
  };
  if (frame.dispatched_frame == dd->frame_index) {
    frame.producing = true;
    push_rtls();
    return;
  }
  if (!RtFrameInputsReady(dd, &frame.input_reason)) {
    frame.reason = rtls::Reason::InputsMissing;
    return;
  }
  const rtao::Fade fade = rtao::RtaoEffectiveFade(bvh_data->tlas_region_range, falcom_world::bvh::g_pool.region_size,
                                                  rtls::g_rtls_fade_start_pct, rtls::g_rtls_fade_end_pct);
  frame.fade = fade;
  frame.fade_valid = true;
  if (fade.coverage < 1.f) {
    frame.reason = rtls::Reason::RangeTooSmall;
    return;
  }
  const uint32_t kinds = ((rtls::g_rtls_point > 0.5f || rtls::g_rtls_env_point > 0.5f) ? rtls::kRtlsKindPoint : 0u)
                         | ((rtls::g_rtls_spot > 0.5f || rtls::g_rtls_env_spot > 0.5f) ? rtls::kRtlsKindSpot : 0u)
                         | (rtls::g_rtls_characters > 0.5f ? rtls::kRtlsKindCharacters : 0u);
  if (kinds == 0u) {
    frame.reason = rtls::Reason::NoKinds;
    return;
  }
  auto* world_cl = falcom_world::GetWorldCommandListData(cmd_list);
  const reshade::api::resource_view lights_view =
      world_cl ? world_cl->ps_srv_view[rtgi::kLightingLightsRegister] : reshade::api::resource_view{0u};
  const reshade::api::resource_view indices_view =
      world_cl ? world_cl->ps_srv_view[rtgi::kLightingLightIndicesRegister] : reshade::api::resource_view{0u};
  if (lights_view.handle == 0u || indices_view.handle == 0u) {
    frame.reason = rtls::Reason::NoLights;
    return;
  }
  frame.lamp_count = static_cast<uint32_t>(std::min<uint64_t>(
      dev->get_resource_desc(dev->get_resource_from_view(lights_view)).buffer.size / rtgi::kLightParamBytes, rtgi::kRtgiLampCount));
  const float ray_max = rtgi::RtgiEffectiveRayMax(rtls::g_rtls_ray_max, fade.coverage, fade.end,
                                                  falcom_world::bvh::kPoolRegionRayMargin);
  // RTAO's IS-FAST rule: the texture is loaded when this row and the IS-FAST master are on.
  if (rtls::g_rtls_isfast > 0.5f && g_isfast_enabled > 0.5f) LoadISFASTNoiseTexture(dev, dd);
  const bool isfast_used = rtls::g_rtls_isfast > 0.5f && g_isfast_enabled > 0.5f && dd->isfast_noise_srv.handle != 0u;
  frame.isfast_unavailable = rtls::g_rtls_isfast > 0.5f && !isfast_used;
  const float spp = (std::max)(rtls::g_rtls_samples, 1.f);
  const float max_points = std::clamp(rtls::g_rtls_max_point_lights, 1.f, static_cast<float>(rtls::kRtlsMaxPoints));
  const rtao::NoiseSample noise =
      rtao::RtaoNoiseSample(dd->frame_index, static_cast<uint32_t>((max_points + rtls::kRtlsMaxSpots) * spp), false);
  const uint32_t divisor = rtao::RtgiTraceDivisor(rtls::g_rtls_trace_resolution);
  const uint32_t trace_width = rtao::RtgiTraceExtent(dd->captured_depth_w, divisor);
  const uint32_t trace_height = rtao::RtgiTraceExtent(dd->captured_depth_h, divisor);
  // RTLS temporal: the reset rule shared with RTAO and RTGI (rtao::TemporalResetReason) over RTLS's own state.
  const bool temporal_on = rtls::g_rtls_temporal_enabled > 0.5f;
  const bool motion_ok = UpdateRtMotion(dev, dd, temporal_on);
  const rtls::ParameterSnapshot params = {
      rtls::g_rtls_samples, rtls::g_rtls_max_point_lights, rtls::g_rtls_ray_max, rtls::g_rtls_light_size,
      rtls::g_rtls_light_clearance, rtls::g_rtls_normal_bias, rtls::g_rtls_trace_characters, rtls::g_rtls_isfast,
      rtls::g_rtls_point, rtls::g_rtls_spot, rtls::g_rtls_env_point, rtls::g_rtls_env_spot, rtls::g_rtls_fade_start_pct,
      rtls::g_rtls_fade_end_pct, rtls::g_rtls_trace_resolution, rtls::g_rtls_temporal_enabled, rtls::g_rtls_history_weight,
      rtls::g_rtls_depth_rejection, rtls::g_rtls_normal_rejection, rtls::g_rtls_debug, rtls::g_rtls_characters, rtls::g_rtls_character_normal_bias, rtls::g_rtls_svgf_history_length};
  const bool params_changed = frame.prev_params_valid && !(params == frame.prev_params);
  frame.prev_params = params;
  frame.prev_params_valid = true;
  auto& rd = rtls::GetRtlsDeviceData(dev);
  const rtao::ResetReason reset = rtao::TemporalResetReason(rtao::TemporalFrame{
      .temporal_on = temporal_on,
      .was_on = frame.temporal_last,
      .resized = rd.denoise.history_color.width != 0u
                 && (rd.denoise.history_color.width != (rtls::g_rtls_full_res_denoise > 0.5f ? dd->captured_depth_w : trace_width)
                     || rd.denoise.history_color.height != (rtls::g_rtls_full_res_denoise > 0.5f ? dd->captured_depth_h : trace_height)
                     || rd.denoise.output.width != dd->captured_depth_w || rd.denoise.output.height != dd->captured_depth_h),
      .frame = dd->frame_index,
      .last_frame = frame.dispatched_frame,
      .guard = dd->dyncube_loadingWipePending || dd->frame_index < dd->resize_guard_until_frame,
      .scaled = false,
      .params_changed = params_changed,
      .motion_ok = motion_ok});
  frame.reset = reset;
  if (temporal_on && reset != rtao::ResetReason::None) {
    frame.last_reset = reset;
    frame.reset_counts[static_cast<size_t>(reset)] += 1u;
  }
  rtls::RtlsDispatchInputs in = {
      .depth_view = dd->captured_depth_srv,
      .mrt_normal_view = dd->captured_mrt_normal_srv,
      .isfast_view = isfast_used ? dd->isfast_noise_srv : reshade::api::resource_view{0u},
      .scene_cbv_view = dd->captured_scene_cbv_view,
      .lights_view = lights_view,
      .light_indices_view = indices_view,
      .full_width = dd->captured_depth_w,
      .full_height = dd->captured_depth_h,
      .trace_width = trace_width,
      .trace_height = trace_height,
      .push = {
          .params = {ray_max, rtls::g_rtls_normal_bias, spp, static_cast<float>(divisor)},
          .sampling = {isfast_used ? 1.f : 0.f, noise.slice_base, noise.seed, rtls::g_rtls_debug},
          .fade = {fade.start, fade.end, static_cast<float>(kinds), 0.f},
          .region = {bvh_data->tlas_region_min[0], bvh_data->tlas_region_min[1], bvh_data->tlas_region_min[2],
                     bvh_data->tlas_region_size},
          .light = {rtls::g_rtls_light_size, rtls::g_rtls_light_clearance, max_points, static_cast<float>(frame.lamp_count)}},
      .debug_mode = static_cast<int>(rtls::g_rtls_debug),
      .motion_view = motion_ok ? dd->mb_rtv4_owned_srv : reshade::api::resource_view{0u},
      .temporal = temporal_on,
      .spatial = rtls::g_rtls_spatial_enabled > 0.5f,
      .svgf = {rtls::g_rtls_history_weight, rtls::g_rtls_depth_rejection, rtls::g_rtls_normal_rejection, reset == rtao::ResetReason::None, rtls::g_rtls_svgf_iterations, rtls::g_rtls_svgf_luminance_edge_stop, 1.f, 128.f, rtls::g_rtls_full_res_denoise > 0.5f, rtls::g_rtls_svgf_history_length, rtls::g_rtls_svgf_radius_scale}};
  if (rtls::g_rtls_trace_characters > 0.5f) {
    in.dynamic = falcom_world::bvh::RefitDeformLiveCharacters(dev, cmd_list);
  }
  const rtls::RtlsDispatchResult result = rtls::Dispatch(dev, cmd_list, in);
  if (!result.ok) {
    frame.reason = rtls::Reason::PipelineFailed;
    return;
  }
  frame.producing = true;
  frame.debug_written = result.debug_written;
  frame.dispatched_frame = dd->frame_index;
  frame.gpu_ms = rd.timer.last_ms;
  frame.trace_width = trace_width;
  frame.trace_height = trace_height;
  frame.divisor = divisor;
  frame.temporal_ran = result.temporal_ran;
  frame.temporal_last = temporal_on && result.temporal_ran;
  frame.filter_ran = result.filter_ran;
  frame.denoise_gpu_ms = rd.denoise.timer.last_ms;
  frame.ray_max_requested = rtls::g_rtls_ray_max;
  frame.ray_max_effective = ray_max;
  frame.kinds = kinds;
  frame.character_objects = in.dynamic.count;
  frame.isfast_used = isfast_used;
  frame.texture_bytes = static_cast<uint64_t>(rd.raw.width) * rd.raw.height * 8u
                        + static_cast<uint64_t>(rd.debug.width) * rd.debug.height * 4u
                        + rtgi::RtgiDenoiseTextureBytes(rd.denoise)
                        + sizeof(uint32_t) * rtls::kRtlsStatsCount;
  push_rtls();
}

// RT Sun Shadows (world/rtsun, R3a/R3c): traces the sun shadow rays for the cascade range RT owns, denoises the term (the
// shared rtgi chain) and binds the denoised output at t39 for the lighting draw. Dispatched inline after RT Local Shadows
// with the same guards; the guard returns end only this stage.
static void RunRtSunInline(reshade::api::command_list* cmd_list) {
  namespace rtsun = falcom_world::rtsun;
  namespace rtao = falcom_world::rtao;
  auto& frame = rtsun::g_rtsun_frame;
  frame.producing = false;
  frame.reason = rtsun::Reason::Off;
  frame.input_reason = rtao::Reason::Off;  // set again only by a failing guard this call
  frame.isfast_unavailable = false;
  shader_injection.rtsun_flags = 0.f;  // set again by push_rtsun when this lighting draw gets the term
  if (!rtsun::RtSunRequested()) {
    rtsun::ReleaseRtSunTextures(cmd_list->get_device());
    return;
  }
  if (!IsSora2nd()) {
    frame.reason = rtsun::Reason::NotSora2nd;
    return;
  }
  auto* dev = cmd_list->get_device();
  auto* dd = dev ? dev->get_private_data<DeviceData>() : nullptr;
  if (!dd) {
    frame.reason = rtsun::Reason::InputsMissing;
    frame.input_reason = rtao::Reason::NoDepth;
    return;
  }
  // World_rtsun.json gate block: the camera's lighting constants and the depth sizes, for the GPU values read back with them.
  const auto& gate_camera = falcom_world::g_state.camera;
  frame.cpu_has_scaling = gate_camera.has_scaling;
  std::memcpy(frame.cpu_scaling, gate_camera.resolution_scaling, sizeof(frame.cpu_scaling));
  frame.cpu_camera_x = gate_camera.view_inv[3];
  frame.cached_w = dd->captured_depth_w;
  frame.cached_h = dd->captured_depth_h;
  if (dd->captured_depth_srv.handle != 0u) {
    const auto depth_desc = dev->get_resource_desc(dev->get_resource_from_view(dd->captured_depth_srv));
    frame.actual_w = depth_desc.texture.width;
    frame.actual_h = depth_desc.texture.height;
  }
  const auto* bvh_data = falcom_world::bvh::GetBvhDeviceData(dev);
  if (!RtBvhReady(bvh_data, &frame.input_reason)) {
    frame.reason = rtsun::Reason::InputsMissing;
    return;
  }
  frame.built_range = bvh_data->tlas_region_range;
  if (frame.built_range <= 0.f) {
    frame.reason = rtsun::Reason::NoCascades;  // no built range yet: not ready, so no cascades (not unlimited)
    return;
  }
  if (cmd_list != dd->immediate_cmd_list) {
    frame.reason = rtsun::Reason::DeferredList;
    return;
  }
  // Binds this frame's RT sun term (the denoised output, full size) at t39 for this lighting draw, also on a second
  // lighting draw of the frame. Only while the latched count is nonzero: the CSM is then skipped in range this frame and the term replaces it.
  const auto push_rtsun = [&]() {
    if (falcom_world::g_state.rt_sun_cascades.load(std::memory_order_relaxed) == 0) return;
    const rtsun::RtSunDeviceData& data = rtsun::GetRtSunDeviceData(dev);
    const reshade::api::resource_view term_srv = data.denoise.output.srv[data.denoise.output_index];
    cmd_list->push_descriptors(
        reshade::api::shader_stage::pixel,
        reshade::api::pipeline_layout{0},
        0,
        reshade::api::descriptor_table_update{
            {}, kLightingRtSunRegister, 0, 1, reshade::api::descriptor_type::texture_shader_resource_view, &term_srv});
    shader_injection.rtsun_flags = static_cast<float>(rtsun::kRtSunFlagApplied
                                                      | (rtsun::g_rtsun_receive_characters > 0.5f ? rtsun::kRtSunFlagReceive : 0u));
    shader_injection.rtsun_strength = rtsun::g_rtsun_strength;
    shader_injection.rtsun_max_darkening = rtsun::g_rtsun_max_darkening;
    shader_injection.rtsun_spare = rtsun::g_rtsun_character_intensity;
    shader_injection.rtsun_char_max_darkening = rtsun::g_rtsun_character_max_darkening;
  };
  if (frame.dispatched_frame == dd->frame_index) {
    frame.producing = true;
    push_rtsun();
    return;
  }
  if (!RtFrameInputsReady(dd, &frame.input_reason)) {
    frame.reason = rtsun::Reason::InputsMissing;
    return;
  }
  const auto& camera = falcom_world::g_state.camera;
  frame.cascades_requested = static_cast<int>(rtsun::g_rtsun_cascade_range);
  frame.cascades_camera = camera.has_sun ? camera.shadow_cascade_count : 0;
  // The latched count (set in ApplyBvhInputs from last frame's trace) is the one the CSM skip uses; the trace keeps it
  // while nonzero, and computes its own count only before a frame has been latched.
  const int latched_cascades = falcom_world::g_state.rt_sun_cascades.load(std::memory_order_relaxed);
  frame.cascades_effective = latched_cascades != 0
                                 ? latched_cascades
                                 : rtsun::RtSunEffectiveCascades(frame.cascades_requested, camera, bvh_data->tlas_region_range);
  if (frame.cascades_effective == 0) {
    frame.reason = rtsun::Reason::NoCascades;
    return;
  }
  frame.covered_m = rtsun::RtSunBoundary(static_cast<uint32_t>(frame.cascades_effective), camera.shadow_cascade_count,
                                         camera.shadow_split_distance, camera.shadow_fade_near, camera.shadow_fade_range_inv);
  // RTAO's IS-FAST rule: the texture is loaded when this row and the IS-FAST master are on.
  if (rtsun::g_rtsun_isfast > 0.5f && g_isfast_enabled > 0.5f) LoadISFASTNoiseTexture(dev, dd);
  const bool isfast_used = rtsun::g_rtsun_isfast > 0.5f && g_isfast_enabled > 0.5f && dd->isfast_noise_srv.handle != 0u;
  frame.isfast_unavailable = rtsun::g_rtsun_isfast > 0.5f && !isfast_used;
  const float spp = (std::max)(rtsun::g_rtsun_samples, 1.f);
  const rtao::NoiseSample noise = rtao::RtaoNoiseSample(dd->frame_index, static_cast<uint32_t>(spp), false);
  const uint32_t divisor = rtao::RtgiTraceDivisor(rtsun::g_rtsun_trace_resolution);
  const uint32_t trace_width = rtao::RtgiTraceExtent(dd->captured_depth_w, divisor);
  const uint32_t trace_height = rtao::RtgiTraceExtent(dd->captured_depth_h, divisor);
  // RT sun temporal: the reset rule shared with RT Local Shadows (rtao::TemporalResetReason) over the RT sun snapshot.
  // Strength, Max Darkening and the spatial filter act after the history and are not in it.
  const bool temporal_on = rtsun::g_rtsun_temporal_enabled > 0.5f;
  const bool motion_ok = UpdateRtMotion(dev, dd, temporal_on);
  const rtsun::ParameterSnapshot params = {
      rtsun::g_rtsun_cascade_range, rtsun::g_rtsun_samples, rtsun::g_rtsun_ray_max, rtsun::g_rtsun_light_size,
      rtsun::g_rtsun_normal_bias, rtsun::g_rtsun_trace_characters, rtsun::g_rtsun_isfast, rtsun::g_rtsun_trace_resolution,
      rtsun::g_rtsun_temporal_enabled, rtsun::g_rtsun_history_weight, rtsun::g_rtsun_depth_rejection,
      rtsun::g_rtsun_normal_rejection, rtsun::g_rtsun_debug,
      rtsun::g_rtsun_receive_characters, rtsun::g_rtsun_character_normal_bias, rtsun::g_rtsun_foliage_intensity, rtsun::g_rtsun_svgf_history_length};
  const bool params_changed = frame.prev_params_valid && !(params == frame.prev_params);
  frame.prev_params = params;
  frame.prev_params_valid = true;
  auto& rd = rtsun::GetRtSunDeviceData(dev);
  const rtao::ResetReason reset = rtao::TemporalResetReason(rtao::TemporalFrame{
      .temporal_on = temporal_on,
      .was_on = frame.temporal_last,
      .resized = rd.denoise.history_color.width != 0u
                 && (rd.denoise.history_color.width != (rtsun::g_rtsun_full_res_denoise > 0.5f ? dd->captured_depth_w : trace_width)
                     || rd.denoise.history_color.height != (rtsun::g_rtsun_full_res_denoise > 0.5f ? dd->captured_depth_h : trace_height)
                     || rd.denoise.output.width != dd->captured_depth_w || rd.denoise.output.height != dd->captured_depth_h),
      .frame = dd->frame_index,
      .last_frame = frame.dispatched_frame,
      .guard = dd->dyncube_loadingWipePending || dd->frame_index < dd->resize_guard_until_frame,
      .scaled = false,
      .params_changed = params_changed,
      .motion_ok = motion_ok});
  frame.reset = reset;
  if (temporal_on && reset != rtao::ResetReason::None) {
    frame.last_reset = reset;
    frame.reset_counts[static_cast<size_t>(reset)] += 1u;
  }
  rtsun::RtSunDispatchInputs in = {
      .depth_view = dd->captured_depth_srv,
      .mrt_normal_view = dd->captured_mrt_normal_srv,
      .isfast_view = isfast_used ? dd->isfast_noise_srv : reshade::api::resource_view{0u},
      .scene_cbv_view = dd->captured_scene_cbv_view,
      .full_width = dd->captured_depth_w,
      .full_height = dd->captured_depth_h,
      .trace_width = trace_width,
      .trace_height = trace_height,
      .push = {
          .params = {rtsun::g_rtsun_ray_max, rtsun::g_rtsun_normal_bias, spp, static_cast<float>(frame.cascades_effective)},
          .sampling = {isfast_used ? 1.f : 0.f, noise.slice_base, noise.seed, rtsun::g_rtsun_debug},
          .sun = {rtsun::g_rtsun_light_size, 0.f, 0.f, static_cast<float>(divisor)},
          .region = {bvh_data->tlas_region_min[0], bvh_data->tlas_region_min[1], bvh_data->tlas_region_min[2],
                     bvh_data->tlas_region_size},
          .size = {0.f, 0.f, falcom_world::bvh::kPoolRegionRayMargin, rtsun::g_rtsun_foliage_intensity},
          .character = {rtsun::g_rtsun_receive_characters, rtsun::g_rtsun_character_normal_bias}},
      .debug_mode = static_cast<int>(rtsun::g_rtsun_debug),
      .motion_view = motion_ok ? dd->mb_rtv4_owned_srv : reshade::api::resource_view{0u},
      .temporal = temporal_on,
      .spatial = rtsun::g_rtsun_spatial_enabled > 0.5f,
      .svgf = {rtsun::g_rtsun_history_weight, rtsun::g_rtsun_depth_rejection, rtsun::g_rtsun_normal_rejection, reset == rtao::ResetReason::None, rtsun::g_rtsun_svgf_iterations, rtsun::g_rtsun_svgf_luminance_edge_stop, 1.f, 128.f, rtsun::g_rtsun_full_res_denoise > 0.5f, rtsun::g_rtsun_svgf_history_length, rtsun::g_rtsun_svgf_radius_scale},
  };
  if (rtsun::g_rtsun_trace_characters > 0.5f) {
    in.dynamic = falcom_world::bvh::RefitDeformLiveCharacters(dev, cmd_list);
  }
  const rtsun::RtSunDispatchResult result = rtsun::Dispatch(dev, cmd_list, in);
  if (!result.ok) {
    frame.reason = rtsun::Reason::PipelineFailed;
    return;
  }
  frame.producing = true;
  frame.debug_written = result.debug_written;
  frame.dispatched_frame = dd->frame_index;
  frame.gpu_ms = rd.timer.last_ms;
  frame.width = trace_width;
  frame.height = trace_height;
  frame.divisor = divisor;
  frame.temporal_ran = result.temporal_ran;
  frame.temporal_last = temporal_on && result.temporal_ran;
  frame.filter_ran = result.filter_ran;
  frame.denoise_gpu_ms = rd.denoise.timer.last_ms;
  frame.character_objects = in.dynamic.count;
  frame.isfast_used = isfast_used;
  frame.texture_bytes = static_cast<uint64_t>(rd.raw.width) * rd.raw.height * 8u
                        + static_cast<uint64_t>(rd.debug.width) * rd.debug.height * 4u
                        + falcom_world::rtgi::RtgiDenoiseTextureBytes(rd.denoise)
                        + sizeof(uint32_t) * rtsun::kRtSunStatsCount;
  push_rtsun();
}

// RTGI (world/rtgi, ROADMAP "RTGI"). Runs only on a lighting draw where RTAO dispatched this frame on the immediate
// list, so RTAO's guards (live BVH, depth, normals, scene CBV, startup and resize guard) hold for it too.
// Kept as a function: its guard returns must end only the RTGI stage, not the lighting hook.
static void RunRtgiInline(reshade::api::command_list* cmd_list) {
  namespace rtao = falcom_world::rtao;
  namespace rtgi = falcom_world::rtgi;
  namespace rtls = falcom_world::rtls;
  auto& frame = rtgi::g_rtgi_frame;
  frame.producing = false;
  frame.reason = rtgi::Reason::Off;
  shader_injection.rtgi_gi_bound = 0.f;  // set again by push_gi when this lighting draw gets the bounce
  if (rtgi::g_rtgi_enabled < 0.5f) {
    rtgi::ReleaseRtgiTextures(cmd_list->get_device());
    return;
  }
  if (!rtao::RtaoRequested()) {
    rtgi::ReleaseRtgiTextures(cmd_list->get_device());
    frame.reason = rtgi::Reason::RtaoOff;
    return;
  }
  auto* dev = cmd_list->get_device();
  auto* dd = dev ? dev->get_private_data<DeviceData>() : nullptr;
  const auto* bvh_data = dev ? falcom_world::bvh::GetBvhDeviceData(dev) : nullptr;
  if (dd == nullptr || bvh_data == nullptr || cmd_list != dd->immediate_cmd_list
      || rtao::g_rtao_frame.dispatched_frame != dd->frame_index) {
    frame.reason = rtgi::Reason::RtaoNotProducing;
    return;
  }
  // The game's lighting shaders fix the scene-buffer layout; RTGI reads prevViewProj_g at that variant's register.
  rtgi::SceneLayout scene_layout = rtgi::SceneLayout::Unknown;
  if (IsSora2nd()) {
    scene_layout = rtgi::SceneLayout::Sora2nd;
  } else if (IsSora1st()) {
    scene_layout = rtgi::SceneLayout::Sora1st;
  } else if (IsKai()) {
    scene_layout = rtgi::SceneLayout::Kai;
  }
  const uint32_t prev_view_proj_register = rtgi::RtgiPrevViewProjRegister(scene_layout);
  if (prev_view_proj_register == 0u) {
    frame.reason = rtgi::Reason::SceneLayoutUnknown;
    return;
  }
  auto& rd = rtgi::GetRtgiDeviceData(dev);
  // GI Intensity > 0 on Sora 2nd (the only lighting shader with the composite): binds this frame's final GI at t37 for
  // this lighting draw, also on a second lighting draw of the frame. Otherwise rtgi_gi_bound stays 0 and the lighting
  // shader takes its pre-RTGI path.
  const auto push_gi = [&]() {
    if (rtgi::g_rtgi_intensity <= 0.f || scene_layout != rtgi::SceneLayout::Sora2nd) return;
    cmd_list->push_descriptors(
        reshade::api::shader_stage::pixel,
        reshade::api::pipeline_layout{0},
        0,
        reshade::api::descriptor_table_update{
            {}, kLightingRtgiRegister, 0, 1, reshade::api::descriptor_type::texture_shader_resource_view, &rd.denoise.output.srv[rd.denoise.output_index]});
    shader_injection.rtgi_gi_bound = 1.f;
    shader_injection.rtgi_intensity = rtgi::g_rtgi_intensity;
    shader_injection.rtgi_char_intensity = rtgi::g_rtgi_char_intensity;
    // PushRtaoAo wrote RTAO's own value for this draw; the bounce draw gets 1.8x (at most 1).
    shader_injection.rtao_max_darkening = rtgi::RtgiRtaoMaxDarkening(rtao::g_rtao_max_darkening);
    frame.applied_frame = dd->frame_index;
    frame.applied_intensity = rtgi::g_rtgi_intensity;
  };
  if (frame.dispatched_frame == dd->frame_index) {
    frame.producing = true;
    push_gi();
    return;
  }
  const rtao::Fade fade = rtao::RtaoEffectiveFade(bvh_data->tlas_region_range, falcom_world::bvh::g_pool.region_size,
                                                  rtgi::g_rtgi_fade_start_pct, rtgi::g_rtgi_fade_end_pct);
  const float ray_max = rtgi::RtgiEffectiveRayMax(rtgi::g_rtgi_ray_max, fade.coverage, fade.end,
                                                  falcom_world::bvh::kPoolRegionRayMargin);
  const uint32_t divisor = rtao::RtgiTraceDivisor(rtgi::g_rtgi_trace_resolution);
  const float spp = (std::max)(rtgi::g_rtgi_samples, 1.f);
  // RTAO loaded the IS-FAST texture this frame when its own row is on; RTGI follows RTAO's IS-FAST row.
  const bool isfast_used = rtao::g_rtao_isfast > 0.5f && g_isfast_enabled > 0.5f && dd->isfast_noise_srv.handle != 0u;
  const rtao::NoiseSample noise = rtao::RtaoNoiseSample(dd->frame_index, static_cast<uint32_t>(spp), false);
  // RTGI temporal: the same reset rule as RTAO (rtao::TemporalResetReason) over RTGI's own state and parameters.
  const bool temporal_on = rtgi::g_rtgi_temporal_enabled > 0.5f;
  const bool motion_ok = UpdateRtMotion(dev, dd, temporal_on);
  const rtgi::ParameterSnapshot params = {
      rtgi::g_rtgi_samples, rtgi::g_rtgi_ray_max, rtao::g_rtao_normal_bias, rtgi::g_rtgi_trace_resolution, rtao::g_rtao_isfast,
      rtgi::g_rtgi_fade_start_pct, rtgi::g_rtgi_fade_end_pct, rtgi::g_rtgi_radiance_clamp, rtgi::g_rtgi_ambient,
      rtgi::g_rtgi_temporal_enabled, rtgi::g_rtgi_history_weight, rtgi::g_rtgi_depth_rejection, rtgi::g_rtgi_normal_rejection,
      rtgi::g_rtgi_debug, rtgi::g_rtgi_sun_distance, rtgi::g_rtgi_trace_characters,
      rtgi::g_rtgi_sun_casters_only, rtgi::g_rtgi_sun_contribution, rtgi::g_rtgi_fallback_no_albedo,
      rtgi::g_rtgi_dynamic_lights, rtgi::g_rtgi_dynamic_light_contribution, rtgi::g_rtgi_game_sun, rtgi::g_rtgi_lamp_shadows,
      rtgi::g_rtgi_extra_bounces, rtgi::g_rtgi_feedback_strength,
      rtgi::g_rtgi_extra_bounce_rays, rtgi::g_rtgi_bounce_saturation, rtgi::g_rtgi_restir, rtgi::g_rtgi_restir_spatial, rtgi::g_rtgi_svgf_history_length};
  const bool svgf_history_changed = frame.prev_params_valid && params.svgf_history_length != frame.prev_params.svgf_history_length;
  rtgi::ParameterSnapshot restir_previous = frame.prev_params;
  restir_previous.svgf_history_length = params.svgf_history_length;
  const bool params_changed = frame.prev_params_valid && !(params == restir_previous);
  frame.prev_params = params;
  frame.prev_params_valid = true;
  const uint32_t trace_width = rtao::RtgiTraceExtent(dd->captured_depth_w, divisor);
  const uint32_t trace_height = rtao::RtgiTraceExtent(dd->captured_depth_h, divisor);
  rtao::TemporalFrame reset_frame = {
      .temporal_on = temporal_on,
      .was_on = frame.temporal_last,
      .resized = rd.denoise.history_color.width != 0u && (rd.denoise.history_color.width != (rtgi::g_rtgi_full_res_denoise > 0.5f ? dd->captured_depth_w : trace_width) || rd.denoise.history_color.height != (rtgi::g_rtgi_full_res_denoise > 0.5f ? dd->captured_depth_h : trace_height) || rd.denoise.output.width != dd->captured_depth_w || rd.denoise.output.height != dd->captured_depth_h),
      .frame = dd->frame_index,
      .last_frame = frame.dispatched_frame,
      .guard = dd->dyncube_loadingWipePending || dd->frame_index < dd->resize_guard_until_frame,
      .scaled = false,
      .params_changed = params_changed,
      .motion_ok = motion_ok};
  const rtao::ResetReason restir_reset = rtao::TemporalResetReason(reset_frame);
  reset_frame.params_changed = params_changed || svgf_history_changed;
  const rtao::ResetReason reset = rtao::TemporalResetReason(reset_frame);
  frame.reset = reset;
  if (temporal_on && reset != rtao::ResetReason::None) {
    frame.last_reset = reset;
    frame.reset_counts[static_cast<size_t>(reset)] += 1u;
  }
  // The copy holds the previous frame's lighting output only when that frame produced it on this list. When that frame
  // got the bounce (push_gi), the copy contains it: the trace subtracts it (one bounce) with the previous final GI (kept
  // only while the depth size is unchanged) and this frame's G-buffer colour, and does not read the copy when it cannot
  // (an input is missing). The colour also feeds RTGI Debug 7 in the upsample.
  const bool applied_last = frame.applied_frame != UINT64_MAX && frame.applied_frame + 1u == dd->frame_index;
  const bool color_ok = dd->captured_color_srv.handle != 0u && dd->captured_color_live.load()
                        && dd->captured_color_frame == dd->frame_index;
  const bool subtract = applied_last && color_ok && rd.denoise.output.width == dd->captured_depth_w
                        && rd.denoise.output.height == dd->captured_depth_h;
  const bool radiance_valid = rd.radiance_srv.handle != 0u && rd.radiance_frame != UINT64_MAX
                              && rd.radiance_frame + 1u == dd->frame_index && (!applied_last || subtract);
  // The light buffers are copied for a stats request or while a lamp sequence runs (Sora 2nd lighting draw, bound at this draw).
  auto* world_cl = falcom_world::GetWorldCommandListData(cmd_list);
  const bool copy_lights = scene_layout == rtgi::SceneLayout::Sora2nd && world_cl != nullptr
                           && (rtgi::g_rtgi_stats.requested.load(std::memory_order_relaxed)
                               || rtgi::g_rtgi_stats.sequence_left.load(std::memory_order_relaxed) > 0u);
  const bool dynamic_lights = scene_layout == rtgi::SceneLayout::Sora2nd && world_cl != nullptr
                              && rtgi::g_rtgi_dynamic_lights > 0.5f;
  const bool lamp_shadows = dynamic_lights && rtgi::g_rtgi_lamp_shadows > 0.5f;
  const bool sora2nd_bindings = scene_layout == rtgi::SceneLayout::Sora2nd && world_cl != nullptr;
  const float feedback_strength = rtgi::RtgiFeedbackStrengthEffective();
  const uint32_t extra_rays = rtgi::RtgiExtraBounceRaysEffective();
  const bool restir_on = rtgi::g_rtgi_restir > 0.5f;
  static_assert((falcom_world::kPsSrvResourceSlots & (1u << rtgi::kLightingLightsRegister)) != 0u
                    && (falcom_world::kPsSrvResourceSlots & (1u << rtgi::kLightingLightIndicesRegister)) != 0u,
                "the bind hook resolves the RTGI light buffers");
  rtgi::RtgiDispatchInputs in = {
      .depth_view = dd->captured_depth_srv,
      .mrt_normal_view = dd->captured_mrt_normal_srv,
      .isfast_view = isfast_used ? dd->isfast_noise_srv : reshade::api::resource_view{0u},
      .scene_cbv_view = dd->captured_scene_cbv_view,
      .radiance_view = radiance_valid ? rd.radiance_srv : reshade::api::resource_view{0u},
      .prev_gi_view = subtract ? rd.denoise.output.srv[rd.denoise.output_index] : reshade::api::resource_view{0u},
      .albedo_view = color_ok ? dd->captured_color_srv : reshade::api::resource_view{0u},
      .full_width = dd->captured_depth_w,
      .full_height = dd->captured_depth_h,
      .trace_width = trace_width,
      .trace_height = trace_height,
      .push = {
          .params = {ray_max, rtao::g_rtao_normal_bias, spp, static_cast<float>(divisor)},
          .sampling = {isfast_used ? 1.f : 0.f, noise.slice_base, noise.seed, rtgi::g_rtgi_debug},
          .fade = {fade.start, fade.end,
                   static_cast<float>((radiance_valid ? rtgi::kRtgiFlagRadianceValid : 0u)
                                      | (subtract ? rtgi::kRtgiFlagSubtractPrevious : 0u)
                                      | (rtgi::g_rtgi_sun_casters_only > 0.5f ? rtgi::kRtgiFlagSunCastersOnly : 0u)
                                      | (rtgi::g_rtgi_fallback_no_albedo > 0.5f ? rtgi::kRtgiFlagFallbackNoAlbedo : 0u)
                                      | (dynamic_lights ? rtgi::kRtgiFlagDynamicLights : 0u)
                                      | (rtgi::g_rtgi_game_sun > 0.5f ? rtgi::kRtgiFlagGameSun : 0u)
                                      | (feedback_strength > 0.f ? rtgi::kRtgiFlagFeedback : 0u)
                                      | (extra_rays > 0u ? rtgi::kRtgiFlagRealRays : 0u)
                                      | (lamp_shadows ? rtgi::kRtgiFlagLampShadows : 0u)
                                      | (restir_on ? rtgi::kRtgiFlagRestir : 0u)),
                   0.f},
          .region = {bvh_data->tlas_region_min[0], bvh_data->tlas_region_min[1], bvh_data->tlas_region_min[2],
                     bvh_data->tlas_region_size},
          .shading = {rtgi::g_rtgi_ambient, rtgi::g_rtgi_radiance_clamp, subtract ? frame.applied_intensity : 0.f,
                      static_cast<float>(prev_view_proj_register)},
          .sun = {rtgi::g_rtgi_sun_distance, 0.f, rtgi::g_rtgi_sun_contribution, (lamp_shadows ? rtls::g_rtls_light_clearance : 0.f)},
          .extra = {rtgi::g_rtgi_dynamic_light_contribution, feedback_strength, static_cast<float>(extra_rays),
                    rtgi::g_rtgi_bounce_saturation},
      },
      .debug_mode = static_cast<int>(rtgi::g_rtgi_debug),
      .motion_view = motion_ok ? dd->mb_rtv4_owned_srv : reshade::api::resource_view{0u},
      .temporal = temporal_on,
      .svgf = {rtgi::g_rtgi_history_weight, rtgi::g_rtgi_depth_rejection, rtgi::g_rtgi_normal_rejection, reset == rtao::ResetReason::None, rtgi::g_rtgi_svgf_iterations, rtgi::g_rtgi_svgf_luminance_edge_stop, rtgi::g_rtgi_svgf_depth_sigma, rtgi::g_rtgi_svgf_normal_power, rtgi::g_rtgi_full_res_denoise > 0.5f, rtgi::g_rtgi_svgf_history_length, rtgi::g_rtgi_svgf_radius_scale},
      .restir_history_valid = restir_reset == rtao::ResetReason::None,
      .spatial = rtgi::g_rtgi_spatial_enabled > 0.5f,
      .restir_spatial = restir_on && rtgi::g_rtgi_restir_spatial > 0.5f,
      .lights = copy_lights ? world_cl->ps_srv[rtgi::kLightingLightsRegister] : reshade::api::resource{0u},
      .light_indices = copy_lights ? world_cl->ps_srv[rtgi::kLightingLightIndicesRegister] : reshade::api::resource{0u},
      .lights_view = dynamic_lights ? world_cl->ps_srv_view[rtgi::kLightingLightsRegister] : reshade::api::resource_view{0u},
      .light_indices_view = dynamic_lights ? world_cl->ps_srv_view[rtgi::kLightingLightIndicesRegister] : reshade::api::resource_view{0u},
      .mrt1_view = sora2nd_bindings ? world_cl->ps_srv_view[rtgi::kLightingMrt1Register] : reshade::api::resource_view{0u},
      .shadow_view = sora2nd_bindings ? world_cl->ps_srv_view[rtgi::kLightingShadowRegister] : reshade::api::resource_view{0u},
      .mrt2_view = sora2nd_bindings ? world_cl->ps_srv_view[rtgi::kLightingMrt2Register] : reshade::api::resource_view{0u},
      .deferred_view = sora2nd_bindings ? world_cl->ps_srv_view[rtgi::kLightingDeferredRegister] : reshade::api::resource_view{0u},
  };
  if (rtgi::g_rtgi_trace_characters > 0.5f) {
    in.dynamic = falcom_world::bvh::RefitDeformLiveCharacters(dev, cmd_list);
  }
  const rtgi::RtgiDispatchResult result = rtgi::Dispatch(dev, cmd_list, in);
  if (!result.ok) {
    frame.reason = result.dxr_unavailable ? rtgi::Reason::DxrUnavailable : rtgi::Reason::PipelineFailed;
    return;
  }
  frame.producing = true;
  frame.debug_written = result.debug_written;
  frame.temporal_ran = result.temporal_ran;
  frame.temporal_last = temporal_on && result.temporal_ran;
  frame.filter_ran = result.filter_ran;
  frame.denoise_gpu_ms = rd.denoise.timer.last_ms;
  frame.dispatched_frame = dd->frame_index;
  frame.gpu_ms = rd.timer.last_ms;
  frame.divisor = divisor;
  frame.trace_width = in.trace_width;
  frame.trace_height = in.trace_height;
  frame.full_width = in.full_width;
  frame.full_height = in.full_height;
  frame.ray_max_requested = rtgi::g_rtgi_ray_max;
  frame.ray_max_effective = ray_max;
  frame.fade_valid = true;
  frame.fade = fade;
  frame.samples = spp;
  frame.character_objects = in.dynamic.count;
  frame.isfast_used = isfast_used;
  frame.prev_view_proj_register = prev_view_proj_register;
  frame.radiance_valid = radiance_valid;
  frame.subtracted = subtract && radiance_valid;
  frame.restir = restir_on;
  frame.restir_temporal = result.restir_temporal_ran;
  frame.restir_gpu_ms = result.restir_temporal_ran ? rd.restir_timer.last_ms : -1.f;
  frame.restir_spatial = result.restir_spatial_ran;
  frame.restir_spatial_gpu_ms = result.restir_spatial_ran ? rd.restir_spatial_timer.last_ms : -1.f;
  frame.radiance_width = rd.radiance_width;
  frame.radiance_height = rd.radiance_height;
  frame.radiance_format = static_cast<uint32_t>(rd.radiance_format);
  frame.texture_bytes = sizeof(uint32_t) * rtgi::kRtgiStatsCount
                        + static_cast<uint64_t>(rd.debug.width) * rd.debug.height * 4u
                        + static_cast<uint64_t>(rd.raw.width) * rd.raw.height * 8u
                        + static_cast<uint64_t>(rd.restir_sample_pos.width) * rd.restir_sample_pos.height * 16u
                        + static_cast<uint64_t>(rd.restir_sample_data.width) * rd.restir_sample_data.height * 16u
                        + static_cast<uint64_t>(rd.restir_raw.width) * rd.restir_raw.height * 8u
                        + static_cast<uint64_t>(rd.restir_res_sample.width) * rd.restir_res_sample.height * 16u * 2u
                        + static_cast<uint64_t>(rd.restir_res_visible.width) * rd.restir_res_visible.height * 16u * 2u
                        + static_cast<uint64_t>(rd.restir_res_data.width) * rd.restir_res_data.height * 16u * 2u
                        + static_cast<uint64_t>(rd.restir_sp_raw.width) * rd.restir_sp_raw.height * 8u
                        + static_cast<uint64_t>(rd.restir_sp_sample.width) * rd.restir_sp_sample.height * 16u
                        + static_cast<uint64_t>(rd.restir_sp_visible.width) * rd.restir_sp_visible.height * 16u
                        + static_cast<uint64_t>(rd.restir_sp_data.width) * rd.restir_sp_data.height * 16u
                        + rtgi::RtgiDenoiseTextureBytes(rd.denoise)
                        + (rd.radiance_texture.handle != 0u
                               ? static_cast<uint64_t>(reshade::api::format_row_pitch(rd.radiance_format, rd.radiance_width)) * rd.radiance_height
                               : 0u);
  push_gi();
}

static void PrepareRtxdiSceneInline(reshade::api::command_list* cmd_list) {
  namespace di = falcom_world::rtxdi;
  namespace rt12 = falcom_world::rt12;
  const uint32_t frame = falcom_world::g_state.frame.load();
  const uint32_t request_mask = di::SceneRequestMask();
  auto* dev = cmd_list->get_device();
  auto* dd = dev ? dev->get_private_data<DeviceData>() : nullptr;
  const uint64_t generation = (rt12::g_rt12 && rt12::g_rt12->device == dev ? rt12::g_rt12->generation : 0u);
  const uint64_t transfer_configuration = (rt12::g_rt12 && rt12::g_rt12->device == dev
                                               ? rt12::g_rt12->di_transfer.configuration_generation
                                               : 0u);
  const uint64_t di_configuration = (rt12::g_rt12 && rt12::g_rt12->device == dev
                                         ? rt12::g_rt12->di.configuration_generation
                                         : 0u);
  auto& diagnostics = di::GetDiagnosticState();
  const bool diagnostic_enabled = di::DiagnosticCaptureEnabled(diagnostics);
  const uint64_t diagnostic_epoch = diagnostic_enabled ? di::DiagnosticCaptureEpoch(diagnostics) : 0u;
  if (!request_mask) {
    std::unique_lock lock(di::g_di.mutex);
    di::g_di.snapshot = {.status = di::DiStatus::Off, .frame = frame};
  }
  if ((request_mask & 2u) == 0u) {
    std::unique_lock lock(di::g_sun.mutex);
    di::TransitionSunAttempt(&di::g_sun.snapshot, frame, di::SunAttemptAction::Off);
  }
  {
    std::unique_lock lock(di::g_scene.mutex);
    auto& state = di::g_scene.snapshot;
    if (!request_mask) {
      di::RefreshOffState(&state, frame);
      lock.unlock();
      if (diagnostic_enabled) {
        di::RecordDiagnostic(diagnostics, diagnostic_epoch,
                             {.owner_generation = generation,
                              .frame = frame,
                              .capture_frame = dd ? dd->frame_index : UINT32_MAX,
                              .stage = di::DiagnosticStage::ScenePromotion,
                              .reason = di::DiagnosticReason::None,
                              .operation = di::DiagnosticOperation::Validate});
      }
      return;
    }
    if (di::PreserveAttemptForNonImmediate(state, frame, true, dd && cmd_list != dd->immediate_cmd_list)) {
      std::unique_lock sun_lock(di::g_sun.mutex);
      const bool same_request = state.request_mask == request_mask
                                && di::MatchesStagedSunIdentity(di::g_sun.snapshot, frame, generation,
                                                                transfer_configuration, di_configuration);
      di::TransitionSunAttempt(&di::g_sun.snapshot, frame,
                               same_request ? di::SunAttemptAction::PreserveDuplicate : di::SunAttemptAction::Invalidate);
      sun_lock.unlock();
      lock.unlock();
      if (diagnostic_enabled) {
        di::RecordDiagnostic(diagnostics, diagnostic_epoch,
                             {.owner_generation = generation,
                              .frame = frame,
                              .capture_frame = dd ? dd->frame_index : UINT32_MAX,
                              .flags = same_request ? 1u : 0u,
                              .stage = di::DiagnosticStage::ScenePromotion,
                              .reason = same_request ? di::DiagnosticReason::None : di::DiagnosticReason::ConfigurationMismatch,
                              .operation = di::DiagnosticOperation::Validate});
      }
      return;
    }
    if (!dev || !dd) {
      state.status = di::SceneStatus::NoDevice;
      std::unique_lock sun_lock(di::g_sun.mutex);
      di::TransitionSunAttempt(&di::g_sun.snapshot, frame, di::SunAttemptAction::Invalidate);
      sun_lock.unlock();
      lock.unlock();
      if (diagnostic_enabled) {
        di::RecordDiagnostic(diagnostics, diagnostic_epoch,
                             {.owner_generation = generation,
                              .frame = frame,
                              .stage = di::DiagnosticStage::ScenePromotion,
                              .reason = di::DiagnosticReason::MissingInput,
                              .operation = di::DiagnosticOperation::Validate});
      }
      return;
    }
    if (cmd_list != dd->immediate_cmd_list) {
      state.status = di::SceneStatus::DeferredContext;
      std::unique_lock sun_lock(di::g_sun.mutex);
      di::TransitionSunAttempt(&di::g_sun.snapshot, frame, di::SunAttemptAction::Invalidate);
      sun_lock.unlock();
      lock.unlock();
      if (diagnostic_enabled) {
        di::RecordDiagnostic(diagnostics, diagnostic_epoch,
                             {.owner_generation = generation,
                              .frame = frame,
                              .capture_frame = dd->frame_index,
                              .stage = di::DiagnosticStage::ScenePromotion,
                              .reason = di::DiagnosticReason::CommandMismatch,
                              .operation = di::DiagnosticOperation::Validate});
      }
      return;
    }
    if (di::IsDuplicate(state, frame, generation) && state.request_mask == request_mask) {
      std::unique_lock sun_lock(di::g_sun.mutex);
      const auto action = di::MatchesStagedSunIdentity(di::g_sun.snapshot, frame, generation,
                                                       transfer_configuration, di_configuration)
                              ? di::SunAttemptAction::PreserveDuplicate
                              : di::SunAttemptAction::Invalidate;
      di::TransitionSunAttempt(&di::g_sun.snapshot, frame, action);
      sun_lock.unlock();
      lock.unlock();
      if (diagnostic_enabled) {
        di::RecordDiagnostic(diagnostics, diagnostic_epoch,
                             {.owner_generation = generation,
                              .frame = frame,
                              .capture_frame = dd->frame_index,
                              .stage = di::DiagnosticStage::ScenePromotion,
                              .reason = di::DiagnosticReason::None,
                              .operation = di::DiagnosticOperation::Validate});
      }
      return;
    }
    {
      std::unique_lock sun_lock(di::g_sun.mutex);
      di::TransitionSunAttempt(&di::g_sun.snapshot, frame, di::SunAttemptAction::Invalidate);
    }
    state.attempted_frame = frame;
    state.attempted_generation = generation;
    state.request_mask = request_mask;
    state.prepared_frame = UINT32_MAX;
    state.prepared_slot = -1;
    state.prepared_fence = 0u;
    state.prepared_generation = 0u;
    ++state.attempts;
  }
  if (rt12::g_rt12 && rt12::g_rt12->device == dev) rt12::g_rt12->di_transfer.input_frame = UINT32_MAX;
  {
    std::unique_lock lock(di::g_input.mutex);
    di::g_input.snapshot = {};
    di::g_input.snapshot.status = di::InputStatus::WaitingForScene;
    di::g_input.snapshot.attempted_frame = frame;
  }
  di::SceneStatus status = di::SceneStatus::GeometryNotReady;
  rt12::SceneTransactionResult result = {};
  if (!IsSora2nd() || !falcom_world::g_state.supported) {
    status = di::SceneStatus::Guarded;
  } else if (!rt12::g_rt12_enabled.load(std::memory_order_relaxed)) {
    status = di::SceneStatus::DxrOff;
  } else if (!rt12::g_rt12 || rt12::g_rt12->device != dev) {
    status = di::SceneStatus::NotCreated;
  } else if (rt12::g_rt12->unfenced_execution || rt12::g_rt12->stuck) {
    status = di::SceneStatus::Quarantined;
  } else if (rt12::g_rt12->failed) {
    status = di::SceneStatus::Failed;
  } else if (dd->frame_index < kGTVBAOStartupGuardFrames || dd->frame_index < dd->resize_guard_until_frame || dd->dyncube_loadingWipePending) {
    status = di::SceneStatus::Guarded;
  } else if (!dd->captured_scene_cbv_view.handle || !dd->captured_scene_cbv_valid || !dd->captured_cbv_live.load()) {
    status = di::SceneStatus::MissingSceneInput;
  } else {
    const bool rtxdi_isfast_wanted = g_isfast_enabled > 0.5f
        && (((request_mask & 2u) != 0u && di::g_sun_isfast > 0.5f && di::g_sun_light_size > 0.f)
            || ((request_mask & 1u) != 0u && di::g_local_isfast > 0.5f && di::g_local_light_size > 0.f));
    if (rtxdi_isfast_wanted) LoadISFASTNoiseTexture(dev, dd);
    result = rt12::PrepareSceneForRtxdi(dev, cmd_list, dd->captured_scene_cbv_view, true,
                                        rtxdi_isfast_wanted ? dd->isfast_noise_srv : reshade::api::resource_view{0u});
    if (result.scene_ready) {
      status = (result.reused ? di::SceneStatus::Reused : di::SceneStatus::Submitted);
    } else if (rt12::g_rt12->unfenced_execution || rt12::g_rt12->stuck) {
      status = di::SceneStatus::Quarantined;
    } else if (rt12::g_rt12->failed) {
      status = di::SceneStatus::Failed;
    } else if (result.reason == rt12::Rt12Reason::Busy) {
      status = di::SceneStatus::Busy;
    }
  }
  if (result.scene_ready) {
    ID3D11ShaderResourceView* native_views[13] = {};
    if (request_mask & 1u) {
      rt12::g_rt12->ctx11->PSGetShaderResources(0u, 5u, native_views);
      rt12::g_rt12->ctx11->PSGetShaderResources(6u, 1u, &native_views[6]);
      rt12::g_rt12->ctx11->PSGetShaderResources(11u, 2u, &native_views[11]);
    } else {
      rt12::g_rt12->ctx11->PSGetShaderResources(1u, 1u, &native_views[1]);
      rt12::g_rt12->ctx11->PSGetShaderResources(4u, 1u, &native_views[4]);
    }
    std::array<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>, 13> held_views;
    for (size_t i = 0u; i < held_views.size(); ++i) held_views[i].Attach(native_views[i]);
    const auto view = [&](uint32_t index) {
      return reshade::api::resource_view{reinterpret_cast<uint64_t>(held_views[index].Get())};
    };
    rt12::Rt12DiInputResult input_result;
    rt12::Rt12DiInputViews input_views = {};
    uint32_t input_width = 0u, input_height = 0u;
    if (dd->captured_depth_live.load() && dd->captured_mrt_live.load()
        && view(kLightingDepthRegister).handle == dd->captured_depth_srv.handle
        && view(kLightingMrtNormalRegister).handle == dd->captured_mrt_normal_srv.handle
        && di::GetTextureViewExtent(held_views[kLightingDepthRegister].Get(), &input_width, &input_height)) {
      const bool motion_ready = ((request_mask & 1u) != 0u || (di::SunVisibilityRequested(request_mask) && di::SunAccumulationAllowed(di::g_diffuse_denoiser > 0.5f, falcom_world::rr::RrRequested() || falcom_world::rr::RrOwnsDenoise()))) && UpdateRtMotion(dev, dd, true)
                                && dd->mb_rtv4_frame == dd->frame_index;
      const uint32_t requested_debug_view = static_cast<uint32_t>(
          std::clamp(std::isfinite(di::g_debug_view) ? di::g_debug_view : 0.f, 0.f,
                     static_cast<float>(RTXDI_DEBUG_VIEW_SUN_VISIBILITY)));
      const uint32_t effective_debug_view = ResolveRtxdiDebugEffectiveView(
          requested_debug_view, falcom_world::bvh::g_bvh_debug.mode.load(std::memory_order_relaxed) != 0);
      input_views = {.depth = dd->captured_depth_srv, .normal = dd->captured_mrt_normal_srv, .color = (dd->captured_color_live.load() && dd->captured_color_frame == dd->frame_index && view(0u).handle == dd->captured_color_srv.handle ? dd->captured_color_srv : reshade::api::resource_view{}), .mrt1 = view(falcom_world::rtgi::kLightingMrt1Register), .mrt2 = view(falcom_world::rtgi::kLightingMrt2Register), .motion = motion_ready ? dd->mb_rtv4_owned_srv : reshade::api::resource_view{}, .lights = view(falcom_world::rtgi::kLightingLightsRegister), .tiles = view(falcom_world::rtgi::kLightingLightIndicesRegister), .deferred = view(falcom_world::rtgi::kLightingDeferredRegister), .scene_tracker = &dd->rtxdi_scene_range, .capture_frame = dd->frame_index, .frame = frame, .width = input_width, .height = input_height, .lights_register = falcom_world::rtgi::kLightingLightsRegister, .tiles_register = falcom_world::rtgi::kLightingLightIndicesRegister, .deferred_register = falcom_world::rtgi::kLightingDeferredRegister, .effective_debug_view = effective_debug_view, .sun_detail_view = static_cast<uint32_t>(std::clamp(std::isfinite(falcom_world::rtxdi::g_sun_detail_view) ? falcom_world::rtxdi::g_sun_detail_view : 0.f, 0.f, static_cast<float>(RTXDI_SUN_DETAIL_BLOCKER))), .n1_brdf_config = {shader_injection.brdf_hammon_diffuse_enabled, shader_injection.brdf_diffuse_strength, shader_injection.brdf_multiscatter_specular_enabled, shader_injection.brdf_specular_strength, shader_injection.brdf_specular_peak_clamp, shader_injection.brdf_roughness_min, shader_injection.brdf_roughness_max}, .effective_settings = falcom_world::rtxdi::CaptureEffectiveRtxdiSettings(), .scene_slot = result.slot, .scene_fence = result.fence_b, .local = (request_mask & 1u) != 0u, .sun = falcom_world::rtxdi::SunVisibilityRequested(request_mask)};
      input_result = rt12::TransferRtxdiInputs(dev, cmd_list, input_views);
    }
    {
      std::unique_lock lock(di::g_input.mutex);
      auto& input = di::g_input.snapshot;
      input = {};
      input.status = input_result.status;
      input.stage = input_result.stage;
      input.error = input_result.error;
      input.attempted_frame = frame;
      input.ready_frame = (input_result.status == di::InputStatus::Ready ? frame : UINT32_MAX);
      input.width = input_width;
      input.height = input_height;
      input.lights_source = input_result.layout.lights_source;
      input.lights_copied = input_result.layout.lights_copied;
      input.lights_truncated = input_result.layout.lights_truncated;
      input.tiles_source = input_result.layout.tiles_source;
      input.tiles_copied = input_result.layout.tiles_copied;
      input.deferred_source = input_result.layout.deferred_source;
      input.deferred_copied = input_result.layout.deferred_copied;
      input.owner_generation = generation;
      input.configuration_generation = rt12::g_rt12->di_transfer.configuration_generation;
      input.fence = input_result.fence;
      input.scene_slot = result.slot;
      input.transfer_slot = input_result.transfer_slot;
      input.scene_fence = result.fence_b;
      input.motion_available = input_result.layout.motion_available != 0u;
      input.carrier_bytes = uint64_t{input.width} * input.height * (20u + (input.motion_available ? 16u : 0u) + ((request_mask & 1u) ? 48u : 0u))
                            + 1536u + ((request_mask & 1u) ? uint64_t{4096u} * 16u * (RTXDI_INPUT_LIGHT_ROWS + RTXDI_INPUT_TILE_ROWS + RTXDI_INPUT_DEFERRED_ROWS) : 0u);
      input.record_bytes = uint64_t{input.lights_copied} * RTXDI_INPUT_LIGHT_STRIDE + uint64_t{input.tiles_copied} * RTXDI_INPUT_TILE_STRIDE + uint64_t{input.deferred_copied} * RTXDI_INPUT_DEFERRED_STRIDE + 1536u;
    }
    if (request_mask & 1u) {
      if (input_result.status == di::InputStatus::Ready) {
        rt12::DispatchRtxdiInitialDi(dev, cmd_list, input_views, input_result);
      } else {
        std::unique_lock lock(di::g_di.mutex);
        di::g_di.snapshot = {.status = di::DiStatus::WaitingInput, .frame = frame, .scene_slot = result.slot};
      }
    } else {
      std::unique_lock lock(di::g_di.mutex);
      di::g_di.snapshot = {.status = di::DiStatus::Off, .frame = frame};
    }
    if (request_mask & 2u) {
      di::SunSnapshot sun;
      sun.frame = frame;
      if (input_result.status == di::InputStatus::Ready && input_result.transfer_slot >= 0
          && input_result.transfer_slot < static_cast<int32_t>(rt12::kRt12Slots)) {
        const rt12::Rt12DiTransferSlot& slot = rt12::g_rt12->di_transfer.slots[static_cast<uint32_t>(input_result.transfer_slot)];
        if (slot.output_staged && slot.sun_visibility.srv11.handle) {
          sun.status = rt12::g_rt12->failed || rt12::g_rt12->stuck ? di::SunStatus::Quarantined : di::SunStatus::Staged;
          sun.width = slot.output_width;
          sun.height = slot.output_height;
          sun.owner_generation = slot.output_owner_generation;
          sun.configuration_generation = slot.sun_configuration_generation;
          sun.di_configuration_generation = rt12::g_rt12->di.configuration_generation;
          sun.fence_b = slot.output_b_fence;
          sun.scene_slot = slot.output_scene_slot;
          sun.transfer_slot = slot.output_transfer_slot;
          sun.accumulated = slot.sun_accumulated;
          sun.accumulation_history = slot.sun_history_used;
        } else {
          sun.status = di::SunStatus::Failed;
          sun.error = E_FAIL;
        }
      } else if (rt12::g_rt12->failed || rt12::g_rt12->stuck) {
        sun.status = di::SunStatus::Quarantined;
        sun.error = E_FAIL;
      } else if (input_result.status == di::InputStatus::SubmitFailed
                 || input_result.status == di::InputStatus::AllocationFailed) {
        sun.status = di::SunStatus::Failed;
        sun.error = input_result.error;
      } else {
        sun.status = di::SunStatus::WaitingInput;
        sun.error = input_result.error;
      }
      std::unique_lock lock(di::g_sun.mutex);
      di::g_sun.snapshot = sun;
    }
  } else {
    std::unique_lock lock(di::g_input.mutex);
    di::g_input.snapshot = {};
    di::g_input.snapshot.status = di::InputStatus::WaitingForScene;
    di::g_input.snapshot.attempted_frame = frame;
    std::unique_lock di_lock(di::g_di.mutex);
    di::g_di.snapshot = {.status = ((request_mask & 1u) ? di::DiStatus::WaitingInput : di::DiStatus::Off), .frame = frame};
  }
  {
    std::unique_lock lock(di::g_scene.mutex);
    auto& state = di::g_scene.snapshot;
    state.status = status;
    if (result.scene_ready) {
      state.prepared_frame = result.frame;
      state.prepared_slot = result.slot;
      state.prepared_fence = result.fence_b;
      state.prepared_generation = generation;
      if (result.reused) {
        ++state.reuses;
      }
    } else {
      ++state.skips;
    }
    if (result.submitted) {
      ++state.submissions;
    }
  }
  if (diagnostic_enabled) {
    const auto reason = status == di::SceneStatus::Busy                ? di::DiagnosticReason::Busy
                        : status == di::SceneStatus::Quarantined       ? di::DiagnosticReason::Quarantined
                        : status == di::SceneStatus::Failed            ? di::DiagnosticReason::SubmitFailed
                        : status == di::SceneStatus::MissingSceneInput ? di::DiagnosticReason::MissingInput
                                                                       : di::DiagnosticReason::None;
    di::RecordDiagnostic(diagnostics, diagnostic_epoch,
                         {.owner_generation = generation,
                          .frame = frame,
                          .capture_frame = dd ? dd->frame_index : UINT32_MAX,
                          .fence = result.fence_b,
                          .scene_slot = result.slot,
                          .flags = static_cast<uint32_t>(status),
                          .stage = di::DiagnosticStage::ScenePromotion,
                          .reason = reason,
                          .operation = di::DiagnosticOperation::Validate});
  }
}

static void BindRtxdiLocalOutput(reshade::api::command_list* cmd_list, DeviceData* dd) {
  namespace rt12 = falcom_world::rt12;
  namespace di = falcom_world::rtxdi;
  namespace rr = falcom_world::rr;
  auto& diagnostics = di::GetDiagnosticState();
  const bool diagnostic_enabled = di::DiagnosticCaptureEnabled(diagnostics);
  const uint64_t diagnostic_epoch = diagnostic_enabled ? di::DiagnosticCaptureEpoch(diagnostics) : 0u;
  const auto record_binding = [&](bool bound, di::DiagnosticReason reason, uint32_t frame,
                                  uint64_t owner_generation, int32_t transfer_slot, uint64_t fence,
                                  bool requested = true) {
    if (!diagnostic_enabled) return;
    uint64_t effective_identity = 0u;
    {
      std::shared_lock lock(di::g_di.mutex);
      effective_identity = di::g_di.snapshot.settings_identity;
    }
    di::RecordDiagnostic(diagnostics, diagnostic_epoch,
                         {.owner_generation = owner_generation,
                          .frame = frame,
                          .settings_identity = di::CaptureEffectiveRtxdiSettings().identity,
                          .effective_identity = effective_identity,
                          .fence = fence,
                          .transfer_slot = transfer_slot,
                          .flags = (bound ? 1u : 0u) | (requested ? 4u : 0u),
                          .stage = di::DiagnosticStage::LightingBind,
                          .reason = reason,
                          .operation = di::DiagnosticOperation::BindLocal});
  };
  shader_injection.rtxdi_local_bound = 0.f;
  const uint32_t requested_debug_view = static_cast<uint32_t>(
      std::clamp(std::isfinite(di::g_debug_view) ? di::g_debug_view : 0.f, 0.f,
                 static_cast<float>(RTXDI_DEBUG_VIEW_SUN_VISIBILITY)));
  shader_injection.rtxdi_debug_view = static_cast<float>(RTXDI_DEBUG_VIEW_OFF);
  shader_injection.rtxdi_debug_bound = 0.f;
  shader_injection.rtxdi_debug_distance_bound = 0.f;
  shader_injection.rtxdi_sun_detail_bound = 0.f;
  shader_injection.rtxdi_sun_detail_view = static_cast<float>(RTXDI_SUN_DETAIL_STATUS);
  const bool local_requested = di::g_local_requested >= 0.5f;
#if defined(FALCOM_NRD_ENABLED)
  shader_injection.rtxdi_diffuse_bound = 0.f;
#endif
  {
    std::unique_lock lock(di::g_di.mutex);
    di::g_di.snapshot.output_bound = false;
    di::g_di.snapshot.output_bound_frame = UINT32_MAX;
    di::g_di.snapshot.output_bound_transfer_slot = -1;
    di::g_di.snapshot.output_bound_fence_b = 0u;
    di::g_di.snapshot.lighting_draw_seen = false;
#if defined(FALCOM_NRD_ENABLED)
    di::g_di.snapshot.nrd_diffuse_bound = false;
    di::g_di.snapshot.nrd_diffuse_lighting_draw_seen = false;
#endif
  }
  if (!IsSora2nd() || !cmd_list) {
    if (local_requested) {
      record_binding(false, di::DiagnosticReason::MissingInput, falcom_world::g_state.frame.load(), 0u, -1, 0u);
    }
    return;
  }

  const reshade::api::resource_view empty_view = {};
  cmd_list->push_descriptors(reshade::api::shader_stage::pixel, reshade::api::pipeline_layout{0}, 0,
                             reshade::api::descriptor_table_update{{}, 41u, 0u, 1u, reshade::api::descriptor_type::texture_shader_resource_view, &empty_view});
  cmd_list->push_descriptors(reshade::api::shader_stage::pixel, reshade::api::pipeline_layout{0}, 0,
                             reshade::api::descriptor_table_update{{}, 44u, 0u, 1u, reshade::api::descriptor_type::texture_shader_resource_view, &empty_view});
  cmd_list->push_descriptors(reshade::api::shader_stage::pixel, reshade::api::pipeline_layout{0}, 0,
                             reshade::api::descriptor_table_update{{}, 45u, 0u, 1u, reshade::api::descriptor_type::texture_shader_resource_view, &empty_view});
  cmd_list->push_descriptors(reshade::api::shader_stage::pixel, reshade::api::pipeline_layout{0}, 0,
                             reshade::api::descriptor_table_update{{}, 46u, 0u, 1u, reshade::api::descriptor_type::texture_shader_resource_view, &empty_view});
#if defined(FALCOM_NRD_ENABLED)
  cmd_list->push_descriptors(reshade::api::shader_stage::pixel, reshade::api::pipeline_layout{0}, 0,
                             reshade::api::descriptor_table_update{{}, 43u, 0u, 1u, reshade::api::descriptor_type::texture_shader_resource_view, &empty_view});
#endif

  di::DebugSnapshot debug_snapshot = {};
  debug_snapshot.requested_view = requested_debug_view;
  const bool bvh_debug_active = falcom_world::bvh::g_bvh_debug.mode.load(std::memory_order_relaxed) != 0;
  const auto debug_presentation_owner =
      ResolveRtxdiDebugPresentationOwner(requested_debug_view, bvh_debug_active);
  debug_snapshot.effective_view = ResolveRtxdiDebugEffectiveView(requested_debug_view, bvh_debug_active);
  debug_snapshot.sun_detail_view = debug_snapshot.effective_view == RTXDI_DEBUG_VIEW_SUN_VISIBILITY
                                       ? static_cast<uint32_t>(std::clamp(std::isfinite(di::g_sun_detail_view)
                                                                              ? di::g_sun_detail_view
                                                                              : 0.f,
                                                                          0.f, static_cast<float>(RTXDI_SUN_DETAIL_BLOCKER)))
                                       : RTXDI_SUN_DETAIL_STATUS;
  shader_injection.rtxdi_debug_view = static_cast<float>(debug_snapshot.effective_view);
  shader_injection.rtxdi_sun_detail_view = static_cast<float>(
      debug_snapshot.effective_view == RTXDI_DEBUG_VIEW_SUN_VISIBILITY
          ? static_cast<uint32_t>(std::clamp(std::isfinite(di::g_sun_detail_view) ? di::g_sun_detail_view : 0.f, 0.f,
                                             static_cast<float>(RTXDI_SUN_DETAIL_BLOCKER)))
          : RTXDI_SUN_DETAIL_STATUS);
  debug_snapshot.superseded_by_bvh = debug_presentation_owner == RtxdiDebugPresentationOwner::Bvh
                                     && requested_debug_view != RTXDI_DEBUG_VIEW_OFF;
  debug_snapshot.frame = falcom_world::g_state.frame.load();
  debug_snapshot.width = dd ? dd->captured_depth_w : 0u;
  debug_snapshot.height = dd ? dd->captured_depth_h : 0u;
  debug_snapshot.owner_generation = rt12::g_rt12 ? rt12::g_rt12->generation : 0u;
  if (debug_snapshot.superseded_by_bvh) {
    debug_snapshot.rejection_reason = RTXDI_DEBUG_REASON_BVH_SUPERSEDED;
  } else if (requested_debug_view != RTXDI_DEBUG_VIEW_OFF && requested_debug_view == RTXDI_DEBUG_VIEW_SUN_VISIBILITY
             && di::g_sun_requested < 0.5f) {
    debug_snapshot.rejection_reason = RTXDI_DEBUG_REASON_PRODUCER_DISABLED;
  } else if (requested_debug_view != RTXDI_DEBUG_VIEW_OFF
             && requested_debug_view != RTXDI_DEBUG_VIEW_SUN_VISIBILITY && !local_requested) {
    debug_snapshot.rejection_reason = RTXDI_DEBUG_REASON_PRODUCER_DISABLED;
  } else if (requested_debug_view != RTXDI_DEBUG_VIEW_OFF && dd && cmd_list == dd->immediate_cmd_list && rt12::g_rt12
             && rt12::g_rt12->device == cmd_list->get_device() && !rt12::g_rt12->failed && !rt12::g_rt12->stuck
             && !rt12::g_rt12->unfenced_execution) {
    di::InputSnapshot debug_input;
    di::DiSnapshot debug_output;
    di::SceneSnapshot debug_scene;
    {
      std::shared_lock lock(di::g_input.mutex);
      debug_input = di::g_input.snapshot;
    }
    {
      std::shared_lock lock(di::g_di.mutex);
      debug_output = di::g_di.snapshot;
    }
    {
      std::shared_lock lock(di::g_scene.mutex);
      debug_scene = di::g_scene.snapshot;
    }
    debug_snapshot.transfer_configuration = rt12::g_rt12->di_transfer.configuration_generation;
    debug_snapshot.di_configuration = rt12::g_rt12->di.configuration_generation;
    debug_snapshot.scene_slot = debug_output.output_scene_slot;
    debug_snapshot.transfer_slot = debug_output.output_transfer_slot;
    const bool identity_matches = debug_output.status == di::DiStatus::Submitted
                                  && debug_output.frame == debug_snapshot.frame && debug_output.output_staged
                                  && debug_output.output_transfer_slot >= 0
                                  && debug_output.output_transfer_slot < static_cast<int32_t>(rt12::kRt12Slots)
                                  && debug_input.status == di::InputStatus::Ready
                                  && debug_input.ready_frame == debug_snapshot.frame
                                  && debug_input.owner_generation == debug_snapshot.owner_generation
                                  && debug_input.configuration_generation == debug_snapshot.transfer_configuration
                                  && debug_input.scene_slot == debug_output.output_scene_slot
                                  && debug_input.transfer_slot == debug_output.output_transfer_slot
                                  && debug_input.width == debug_snapshot.width
                                  && debug_input.height == debug_snapshot.height
                                  && debug_output.output_owner_generation == debug_snapshot.owner_generation
                                  && debug_output.output_configuration_generation == debug_snapshot.di_configuration
                                  && debug_output.output_width == debug_snapshot.width
                                  && debug_output.output_height == debug_snapshot.height
                                  && debug_output.output_fence_b != 0u
                                  && debug_output.fence == debug_output.output_fence_b
                                  && debug_scene.prepared_frame == debug_snapshot.frame
                                  && debug_scene.prepared_slot == debug_output.output_scene_slot
                                  && debug_scene.prepared_generation == debug_snapshot.owner_generation
                                  && debug_input.scene_fence == debug_scene.prepared_fence;
    if (requested_debug_view == RTXDI_DEBUG_VIEW_SUN_VISIBILITY) {
      di::SunSnapshot debug_sun;
      {
        std::shared_lock lock(di::g_sun.mutex);
        debug_sun = di::g_sun.snapshot;
      }
      debug_snapshot.scene_slot = debug_sun.scene_slot;
      debug_snapshot.transfer_slot = debug_sun.transfer_slot;
      debug_snapshot.transfer_configuration = debug_sun.configuration_generation;
      debug_snapshot.di_configuration = debug_sun.di_configuration_generation;
      if (debug_sun.status == di::SunStatus::Staged && debug_sun.frame == debug_snapshot.frame
          && debug_sun.owner_generation == debug_snapshot.owner_generation
          && debug_sun.configuration_generation == rt12::g_rt12->di_transfer.configuration_generation
          && debug_sun.di_configuration_generation == rt12::g_rt12->di.configuration_generation
          && debug_sun.width == debug_snapshot.width && debug_sun.height == debug_snapshot.height
          && debug_sun.fence_b != 0u && debug_sun.scene_slot == debug_scene.prepared_slot
          && debug_sun.frame == debug_scene.prepared_frame
          && debug_sun.owner_generation == debug_scene.prepared_generation
          && debug_scene.prepared_fence == debug_input.scene_fence
          && debug_input.status == di::InputStatus::Ready && debug_input.ready_frame == debug_snapshot.frame
          && debug_input.owner_generation == debug_snapshot.owner_generation
          && debug_input.scene_slot == debug_sun.scene_slot && debug_input.transfer_slot == debug_sun.transfer_slot
          && debug_input.width == debug_snapshot.width && debug_input.height == debug_snapshot.height
          && debug_sun.transfer_slot >= 0 && debug_sun.transfer_slot < static_cast<int32_t>(rt12::kRt12Slots)) {
        const auto& debug_slot = rt12::g_rt12->di_transfer.slots[static_cast<uint32_t>(debug_sun.transfer_slot)];
        debug_snapshot.source_ready = di::MatchesStagedSunSlotIdentity(
                                          debug_sun, debug_snapshot.frame, debug_snapshot.owner_generation,
                                          rt12::g_rt12->di_transfer.configuration_generation,
                                          rt12::g_rt12->di.configuration_generation,
                                          debug_slot.sun_configuration_generation)
                                      && debug_slot.sun && debug_slot.output_staged
                                      && debug_slot.output_frame == debug_snapshot.frame
                                      && debug_slot.output_owner_generation == debug_snapshot.owner_generation
                                      && debug_slot.output_scene_slot == debug_sun.scene_slot
                                      && debug_slot.output_transfer_slot == debug_sun.transfer_slot
                                      && debug_slot.output_width == debug_snapshot.width
                                      && debug_slot.output_height == debug_snapshot.height
                                      && debug_slot.output_b_fence == debug_sun.fence_b
                                      && debug_slot.sun_visibility.srv11.handle
                                      && rt12::ValidateDiTexture(debug_slot.sun_visibility.srv11, debug_snapshot.width,
                                                                 debug_snapshot.height, rt12::Rt12DiTextureKind::Uint);
        if (debug_snapshot.sun_detail_view != RTXDI_SUN_DETAIL_STATUS) {
          const auto& detail = debug_slot.sun_detail_identity;
          debug_snapshot.sun_detail_source_ready = detail.source_version == 1u && detail.staged && detail.recorded
              && detail.detail_view == debug_snapshot.sun_detail_view && detail.frame == debug_snapshot.frame
              && detail.owner_generation == debug_snapshot.owner_generation
              && detail.transfer_configuration == debug_sun.configuration_generation
              && detail.di_configuration == debug_sun.di_configuration_generation
              && detail.sun_configuration == debug_sun.configuration_generation
              && detail.scene_slot == debug_sun.scene_slot && detail.transfer_slot == debug_sun.transfer_slot
              && detail.width == debug_snapshot.width && detail.height == debug_snapshot.height
              && detail.producer_fence_b == debug_sun.fence_b
              && detail.settings_identity == di::CaptureEffectiveRtxdiSettings().identity
              && debug_slot.sun_detail.srv11.handle
              && rt12::ValidateDiTexture(debug_slot.sun_detail.srv11, debug_snapshot.width, debug_snapshot.height,
                                         rt12::Rt12DiTextureKind::Uint4);
          debug_snapshot.source_ready = debug_snapshot.sun_detail_source_ready;
        } else {
          debug_snapshot.sun_detail_source_ready = false;
        }
        if (!debug_snapshot.source_ready) debug_snapshot.rejection_reason = RTXDI_DEBUG_REASON_INVALID_RESOURCE;
      } else if (debug_input.status != di::InputStatus::Ready || debug_sun.status == di::SunStatus::WaitingInput) {
        debug_snapshot.rejection_reason = RTXDI_DEBUG_REASON_WAITING_INPUT;
      } else {
        debug_snapshot.rejection_reason = RTXDI_DEBUG_REASON_STALE_IDENTITY;
      }
    } else if (identity_matches) {
      rt12::Rt12DiTransferSlot& debug_slot = rt12::g_rt12->di_transfer.slots[static_cast<uint32_t>(debug_output.output_transfer_slot)];
      const bool n1_identity_matches = debug_slot.n1_staged && debug_slot.output_staged
                                       && debug_slot.output_frame == debug_snapshot.frame
                                       && debug_slot.output_owner_generation == debug_snapshot.owner_generation
                                       && debug_slot.output_scene_slot == debug_output.output_scene_slot
                                       && debug_slot.output_transfer_slot == debug_output.output_transfer_slot
                                       && debug_slot.output_width == debug_snapshot.width
                                       && debug_slot.output_height == debug_snapshot.height
                                       && debug_slot.output_b_fence == debug_output.output_fence_b;
      const bool n1_resources_valid = debug_slot.n1_diffuse.srv11.handle && debug_slot.n1_status.srv11.handle
                                      && rt12::ValidateDiTexture(debug_slot.n1_diffuse.srv11, debug_snapshot.width,
                                                                 debug_snapshot.height, rt12::Rt12DiTextureKind::Float)
                                      && rt12::ValidateDiTexture(debug_slot.n1_status.srv11, debug_snapshot.width,
                                                                 debug_snapshot.height, rt12::Rt12DiTextureKind::Uint);
      if (n1_identity_matches && n1_resources_valid) {
#if defined(FALCOM_NRD_ENABLED)
        debug_snapshot.n1_configuration = debug_slot.n1_staged
                                              ? rt12::Rt12NrdN1ConfigurationIdentity(
                                                    debug_slot.n1_configuration_version, debug_slot.n1_brdf_config)
                                              : 0u;
#endif
        cmd_list->push_descriptors(reshade::api::shader_stage::pixel, reshade::api::pipeline_layout{0}, 0,
                                   reshade::api::descriptor_table_update{{}, 45u, 0u, 1u, reshade::api::descriptor_type::texture_shader_resource_view, &debug_slot.n1_status.srv11});
        const std::array<float, 7u> debug_n1_config = {
            shader_injection.brdf_hammon_diffuse_enabled, shader_injection.brdf_diffuse_strength,
            shader_injection.brdf_multiscatter_specular_enabled, shader_injection.brdf_specular_strength,
            shader_injection.brdf_specular_peak_clamp, shader_injection.brdf_roughness_min,
            shader_injection.brdf_roughness_max};
        if (requested_debug_view == RTXDI_DEBUG_VIEW_DISTANCE_VALIDITY
            || requested_debug_view == RTXDI_DEBUG_VIEW_DISTANCE_MAGNITUDE) {
#if defined(FALCOM_NRD_ENABLED)
          const auto& distance_identity = debug_slot.debug_distance_identity;
          const auto& debug_settings = di::CaptureEffectiveRtxdiSettings();
          if (distance_identity.staged && distance_identity.frame == debug_snapshot.frame
              && distance_identity.requested_view == debug_snapshot.effective_view
              && distance_identity.owner_generation == debug_snapshot.owner_generation
              && distance_identity.transfer_configuration == debug_snapshot.transfer_configuration
              && distance_identity.di_configuration == debug_snapshot.di_configuration
              && distance_identity.settings_identity == debug_settings.identity
              && distance_identity.scene_slot == debug_output.output_scene_slot
              && distance_identity.transfer_slot == debug_output.output_transfer_slot
              && distance_identity.width == debug_snapshot.width && distance_identity.height == debug_snapshot.height
              && distance_identity.producer_fence_b == debug_output.output_fence_b
              && distance_identity.n1_configuration_version == debug_slot.n1_configuration_version
              && distance_identity.n1_brdf_config == debug_n1_config
              && debug_slot.nrd_debug_distance.srv11.handle
              && rt12::ValidateDiTexture(debug_slot.nrd_debug_distance.srv11, debug_snapshot.width,
                                         debug_snapshot.height, rt12::Rt12DiTextureKind::Float)) {
            cmd_list->push_descriptors(reshade::api::shader_stage::pixel, reshade::api::pipeline_layout{0}, 0,
                                       reshade::api::descriptor_table_update{{}, 44u, 0u, 1u, reshade::api::descriptor_type::texture_shader_resource_view, &debug_slot.nrd_debug_distance.srv11});
            shader_injection.rtxdi_debug_bound = 1.f;
            shader_injection.rtxdi_debug_distance_bound = 1.f;
            debug_snapshot.source_ready = true;
            debug_snapshot.distance_source = true;
          } else {
            debug_snapshot.rejection_reason = (rr::RrRequested() || rr::RrOwnsDenoise())
                                                  ? RTXDI_DEBUG_REASON_RR_SUPPRESSED
                                                  : RTXDI_DEBUG_REASON_NOT_PRODUCED;
          }
#else
          debug_snapshot.rejection_reason = RTXDI_DEBUG_REASON_NO_SDK;
#endif
        } else {
          cmd_list->push_descriptors(reshade::api::shader_stage::pixel, reshade::api::pipeline_layout{0}, 0,
                                     reshade::api::descriptor_table_update{{}, 44u, 0u, 1u, reshade::api::descriptor_type::texture_shader_resource_view, &debug_slot.n1_diffuse.srv11});
          shader_injection.rtxdi_debug_bound = 1.f;
          debug_snapshot.source_ready = true;
          if (requested_debug_view == RTXDI_DEBUG_VIEW_SELECTED_LIGHT) {
            debug_snapshot.source_ready = debug_slot.di_resolved.srv11.handle
                                          && rt12::ValidateDiTexture(debug_slot.di_resolved.srv11,
                                                                     debug_snapshot.width, debug_snapshot.height,
                                                                     rt12::Rt12DiTextureKind::Uint4);
            if (!debug_snapshot.source_ready) {
              debug_snapshot.rejection_reason = RTXDI_DEBUG_REASON_INVALID_RESOURCE;
            }
          } else if (requested_debug_view == RTXDI_DEBUG_VIEW_FILTERED_DIFFUSE) {
#if defined(FALCOM_NRD_ENABLED)
            if (rr::RrRequested() || rr::RrOwnsDenoise()) {
              debug_snapshot.source_ready = false;
              debug_snapshot.rejection_reason = RTXDI_DEBUG_REASON_RR_SUPPRESSED;
            } else {
              const auto effective_settings = di::CaptureEffectiveRtxdiSettings();
              const uint64_t n1_identity = rt12::Rt12NrdN1ConfigurationIdentity(
                  RTXDI_N1_CONFIG_VERSION, debug_n1_config);
              const uint64_t nrd_identity = rt12::Rt12NrdConfigurationIdentity(
                  rt12::g_rt12->di_transfer.configuration_generation,
                  rt12::g_rt12->di.configuration_generation, effective_settings.identity,
                  rt12::g_rt12->di_transfer.nrd.qualification.motion_correction_index);
              const auto& identity = debug_slot.nrd_diffuse_identity;
              const bool diffuse_identity_matches = debug_output.nrd_diffuse_staged
                                                    && debug_output.nrd_diffuse_frame == debug_snapshot.frame
                                                    && debug_output.nrd_diffuse_transfer_slot
                                                           == debug_output.output_transfer_slot
                                                    && debug_output.nrd_diffuse_fence_b
                                                           == debug_output.output_fence_b
                                                    && rt12::Rt12NrdDiffusePublicationMatches(
                                                        identity, debug_snapshot.frame,
                                                        debug_snapshot.owner_generation,
                                                        debug_snapshot.transfer_configuration,
                                                        debug_snapshot.di_configuration, n1_identity, nrd_identity,
                                                        debug_snapshot.width, debug_snapshot.height,
                                                        debug_output.output_scene_slot,
                                                        debug_output.output_transfer_slot,
                                                        debug_output.output_fence_b);
              const bool diffuse_resource_valid = debug_slot.nrd_diffuse_staged
                                                  && debug_slot.nrd.diffuse_output_valid
                                                  && debug_slot.nrd.diffuse_publication_recorded
                                                  && debug_slot.nrd_diffuse_publication.srv11.handle
                                                  && rt12::ValidateDiTexture(debug_slot.nrd_diffuse_publication.srv11,
                                                                             debug_snapshot.width,
                                                                             debug_snapshot.height,
                                                                             rt12::Rt12DiTextureKind::Float);
              debug_snapshot.source_ready = effective_settings.relax_diffuse && diffuse_identity_matches
                                            && diffuse_resource_valid;
              if (!effective_settings.relax_diffuse || !debug_output.nrd_diffuse_staged) {
                debug_snapshot.rejection_reason = RTXDI_DEBUG_REASON_NOT_PRODUCED;
              } else if (!diffuse_identity_matches) {
                debug_snapshot.rejection_reason = RTXDI_DEBUG_REASON_STALE_IDENTITY;
              } else if (!diffuse_resource_valid) {
                debug_snapshot.rejection_reason = RTXDI_DEBUG_REASON_INVALID_RESOURCE;
              }
            }
#else
            debug_snapshot.source_ready = false;
            debug_snapshot.rejection_reason = RTXDI_DEBUG_REASON_NO_SDK;
#endif
          }
        }
        debug_snapshot.source_status = debug_snapshot.source_ready ? 1u : 0u;
      } else {
        debug_snapshot.rejection_reason = n1_identity_matches    ? RTXDI_DEBUG_REASON_INVALID_RESOURCE
                                          : debug_slot.n1_staged ? RTXDI_DEBUG_REASON_STALE_IDENTITY
                                                                 : RTXDI_DEBUG_REASON_NOT_PRODUCED;
      }
    } else if (debug_input.status != di::InputStatus::Ready || debug_output.status == di::DiStatus::WaitingInput) {
      debug_snapshot.rejection_reason = RTXDI_DEBUG_REASON_WAITING_INPUT;
    } else {
      debug_snapshot.rejection_reason = RTXDI_DEBUG_REASON_STALE_IDENTITY;
    }
  } else if (requested_debug_view != RTXDI_DEBUG_VIEW_OFF) {
    debug_snapshot.rejection_reason = RTXDI_DEBUG_REASON_BACKEND_UNAVAILABLE;
  }
  debug_snapshot.source_status = debug_snapshot.source_ready ? 1u : 0u;
  {
    std::unique_lock lock(di::g_debug.mutex);
    di::g_debug.snapshot = debug_snapshot;
  }

  if (!local_requested) {
    record_binding(false, di::DiagnosticReason::None, falcom_world::g_state.frame.load(),
                   rt12::g_rt12 ? rt12::g_rt12->generation : 0u, -1, 0u, false);
    return;
  }
  if (!dd || cmd_list != dd->immediate_cmd_list || !rt12::g_rt12
      || rt12::g_rt12->device != cmd_list->get_device() || rt12::g_rt12->failed || rt12::g_rt12->stuck
      || rt12::g_rt12->unfenced_execution) {
    record_binding(false, !dd ? di::DiagnosticReason::MissingEntry : di::DiagnosticReason::MissingInput,
                   falcom_world::g_state.frame.load(), rt12::g_rt12 ? rt12::g_rt12->generation : 0u, -1, 0u);
    return;
  }

  const uint32_t frame = falcom_world::g_state.frame.load();
  di::DiSnapshot output;
  di::InputSnapshot input;
  di::SceneSnapshot scene;
  {
    std::shared_lock lock(di::g_di.mutex);
    output = di::g_di.snapshot;
  }
  {
    std::shared_lock lock(di::g_input.mutex);
    input = di::g_input.snapshot;
  }
  {
    std::shared_lock lock(di::g_scene.mutex);
    scene = di::g_scene.snapshot;
  }
  rt12::Rt12State& state = *rt12::g_rt12;
  if (output.status != di::DiStatus::Submitted || output.frame != frame || !output.output_staged) {
    record_binding(false, di::DiagnosticReason::StaleFrame, frame, state.generation,
                   output.output_transfer_slot, output.output_fence_b);
    return;
  }
  if (output.output_transfer_slot < 0 || output.output_transfer_slot >= static_cast<int32_t>(rt12::kRt12Slots)
      || input.status != di::InputStatus::Ready || input.ready_frame != frame || input.width != dd->captured_depth_w
      || input.height != dd->captured_depth_h || input.owner_generation != state.generation
      || input.scene_slot != output.output_scene_slot || input.transfer_slot != output.output_transfer_slot
      || input.configuration_generation != state.di_transfer.configuration_generation
      || scene.prepared_frame != frame || scene.prepared_slot != output.output_scene_slot
      || scene.prepared_generation != state.generation || input.scene_fence != scene.prepared_fence
      || output.output_owner_generation != state.generation || output.output_configuration_generation == 0u
      || output.output_configuration_generation != state.di.configuration_generation
      || output.output_width != dd->captured_depth_w
      || output.output_height != dd->captured_depth_h || output.output_fence_b == 0u
      || output.fence != output.output_fence_b) {
    record_binding(false, di::DiagnosticReason::ConfigurationMismatch, frame, state.generation,
                   output.output_transfer_slot, output.output_fence_b);
    return;
  }

  rt12::Rt12DiTransferSlot& slot = state.di_transfer.slots[static_cast<uint32_t>(output.output_transfer_slot)];
  if (!slot.output_staged || slot.output_frame != frame || slot.output_owner_generation != output.output_owner_generation
      || slot.output_configuration_generation != output.output_configuration_generation
      || slot.output_scene_slot != output.output_scene_slot || slot.output_transfer_slot != output.output_transfer_slot
      || slot.output_width != output.output_width || slot.output_height != output.output_height
      || slot.output_b_fence != output.output_fence_b || !slot.di_resolved.srv11.handle
      || !rt12::ValidateDiTexture(slot.di_resolved.srv11, output.output_width, output.output_height,
                                  rt12::Rt12DiTextureKind::Uint4)) {
    record_binding(false, di::DiagnosticReason::InvalidResource, frame, state.generation,
                   output.output_transfer_slot, output.output_fence_b);
    return;
  }

  cmd_list->push_descriptors(reshade::api::shader_stage::pixel, reshade::api::pipeline_layout{0}, 0,
                             reshade::api::descriptor_table_update{{}, 41u, 0u, 1u, reshade::api::descriptor_type::texture_shader_resource_view, &slot.di_resolved.srv11});
  shader_injection.rtxdi_local_bound = 1.f;
  shader_injection.rtxdi_local_darkening = di::FiniteClamped(di::g_local_shadow_darkening, 1.f, 0.f, 1.f);
  shader_injection.rtxdi_local_character_darkening = di::FiniteClamped(di::g_local_character_shadow_darkening, 1.f, 0.f, 1.f);
  shader_injection.rtxdi_local_character_keep = 1.f - shader_injection.rtxdi_local_character_darkening;
  shader_injection.rtxdi_local_character_intensity = di::FiniteClamped(di::g_local_character_intensity, 1.f, 0.f, 2.f);
  shader_injection.rtxdi_local_character_fallback = di::g_local_character_fallback > 0.5f ? 1.f : 0.f;
  bool still_current = false;
  {
    std::unique_lock lock(di::g_di.mutex);
    still_current = di::g_di.snapshot.output_frame == frame
                    && di::g_di.snapshot.output_transfer_slot == output.output_transfer_slot
                    && di::g_di.snapshot.output_fence_b == output.output_fence_b;
    if (still_current) {
      di::g_di.snapshot.output_bound = true;
      di::g_di.snapshot.output_bound_frame = frame;
      di::g_di.snapshot.output_bound_transfer_slot = output.output_transfer_slot;
      di::g_di.snapshot.output_bound_fence_b = output.output_fence_b;
    }
  }
  if (!still_current) {
    shader_injection.rtxdi_local_bound = 0.f;
    cmd_list->push_descriptors(reshade::api::shader_stage::pixel, reshade::api::pipeline_layout{0}, 0,
                               reshade::api::descriptor_table_update{{}, 41u, 0u, 1u, reshade::api::descriptor_type::texture_shader_resource_view, &empty_view});
  }
  record_binding(still_current, still_current ? di::DiagnosticReason::None : di::DiagnosticReason::ConfigurationMismatch,
                 frame, state.generation, output.output_transfer_slot, output.output_fence_b);
#if defined(FALCOM_NRD_ENABLED)
  const auto effective_rtxdi_settings = di::CaptureEffectiveRtxdiSettings();
  uint8_t binding_reason = 0u;
  if (!still_current) {
    binding_reason = 1u;
  } else if (!output.nrd_diffuse_staged || rr::RrRequested() || rr::RrOwnsDenoise()
             || !effective_rtxdi_settings.relax_diffuse) {
    binding_reason = 2u;
  }
  if (binding_reason == 0u) {
    const std::array<float, 7u> n1_brdf_config = {
        shader_injection.brdf_hammon_diffuse_enabled, shader_injection.brdf_diffuse_strength,
        shader_injection.brdf_multiscatter_specular_enabled, shader_injection.brdf_specular_strength,
        shader_injection.brdf_specular_peak_clamp, shader_injection.brdf_roughness_min,
        shader_injection.brdf_roughness_max};
    const uint64_t n1_identity = rt12::Rt12NrdN1ConfigurationIdentity(RTXDI_N1_CONFIG_VERSION, n1_brdf_config);
    const uint64_t nrd_identity = rt12::Rt12NrdConfigurationIdentity(
        state.di_transfer.configuration_generation, state.di.configuration_generation, effective_rtxdi_settings.identity,
        state.di_transfer.nrd.qualification.motion_correction_index);
    const auto& identity = slot.nrd_diffuse_identity;
    if (input.width != dd->captured_depth_w || input.height != dd->captured_depth_h) {
      binding_reason = 3u;
    } else if (!rt12::Rt12NrdDiffusePublicationMatches(identity, frame, state.generation,
                                                       state.di_transfer.configuration_generation,
                                                       state.di.configuration_generation, n1_identity, nrd_identity,
                                                       dd->captured_depth_w, dd->captured_depth_h,
                                                       output.output_scene_slot, output.output_transfer_slot,
                                                       output.output_fence_b)) {
      binding_reason = 4u;
    } else if (!(slot.nrd_diffuse_staged && slot.nrd.diffuse_output_valid && slot.nrd.diffuse_publication_recorded
                 && slot.output_b_fence == identity.producer_fence_b && slot.nrd_diffuse_publication.srv11.handle)) {
      binding_reason = 5u;
    } else if (!rt12::ValidateDiTexture(slot.nrd_diffuse_publication.srv11, identity.width, identity.height,
                                        rt12::Rt12DiTextureKind::Float)) {
      binding_reason = 6u;
    }
    if (binding_reason == 0u) {
      cmd_list->push_descriptors(reshade::api::shader_stage::pixel, reshade::api::pipeline_layout{0}, 0,
                                 reshade::api::descriptor_table_update{{}, 43u, 0u, 1u, reshade::api::descriptor_type::texture_shader_resource_view, &slot.nrd_diffuse_publication.srv11});
      shader_injection.rtxdi_diffuse_bound = 1.f;
      std::unique_lock lock(di::g_di.mutex);
      const bool still_staged = di::g_di.snapshot.nrd_diffuse_staged
                                && di::g_di.snapshot.frame == frame
                                && di::g_di.snapshot.nrd_diffuse_transfer_slot == identity.transfer_slot
                                && di::g_di.snapshot.nrd_diffuse_fence_b == identity.producer_fence_b;
      if (still_staged) {
        di::g_di.snapshot.nrd_diffuse_bound = true;
      } else {
        binding_reason = 7u;
        shader_injection.rtxdi_diffuse_bound = 0.f;
        cmd_list->push_descriptors(reshade::api::shader_stage::pixel, reshade::api::pipeline_layout{0}, 0,
                                   reshade::api::descriptor_table_update{{}, 43u, 0u, 1u, reshade::api::descriptor_type::texture_shader_resource_view, &empty_view});
      }
    }
  }
  {
    std::unique_lock lock(di::g_di.mutex);
    auto& nrd_gate = di::g_di.snapshot.nrd_gate;
    if (nrd_gate.frame == frame && nrd_gate.status[static_cast<uint32_t>(di::NrdGateLink::Srv)] == di::kNrdGatePass) {
      const auto binding = static_cast<uint32_t>(di::NrdGateLink::Binding);
      nrd_gate.status[binding] = (binding_reason == 0u ? di::kNrdGatePass : di::kNrdGateFail);
      nrd_gate.reason[binding] = binding_reason;
      if (binding_reason == 0u) {
        const bool diffuse_bound = shader_injection.rtxdi_diffuse_bound > 0.5f;
        const auto bound = static_cast<uint32_t>(di::NrdGateLink::Bound);
        nrd_gate.status[bound] = (diffuse_bound ? di::kNrdGatePass : di::kNrdGateFail);
        nrd_gate.reason[bound] = (diffuse_bound ? 0u : 1u);
      }
    }
  }
#endif
}

static void BindRtxdiSunOutput(reshade::api::command_list* cmd_list, DeviceData* dd) {
  namespace rt12 = falcom_world::rt12;
  namespace di = falcom_world::rtxdi;
  auto& diagnostics = di::GetDiagnosticState();
  const bool diagnostic_enabled = di::DiagnosticCaptureEnabled(diagnostics);
  const uint64_t diagnostic_epoch = diagnostic_enabled ? di::DiagnosticCaptureEpoch(diagnostics) : 0u;
  const auto record_binding = [&](bool bound, di::DiagnosticReason reason, uint32_t frame,
                                  uint64_t owner_generation, int32_t transfer_slot, uint64_t fence,
                                  bool requested = true) {
    if (!diagnostic_enabled) return;
    di::RecordDiagnostic(diagnostics, diagnostic_epoch,
                         {.owner_generation = owner_generation,
                          .frame = frame,
                          .fence = fence,
                          .transfer_slot = transfer_slot,
                          .flags = (bound ? 1u : 0u) | (requested ? 4u : 0u),
                          .stage = di::DiagnosticStage::LightingBind,
                          .reason = reason,
                          .operation = di::DiagnosticOperation::BindSun});
  };
  shader_injection.rtxdi_sun_bound = 0.f;
  shader_injection.rtxdi_sun_detail_bound = 0.f;
  const bool sun_requested = di::g_sun_requested >= 0.5f;
  {
    std::unique_lock lock(di::g_sun.mutex);
    di::g_sun.snapshot.output_bound = false;
    di::g_sun.snapshot.output_bound_frame = UINT32_MAX;
    di::g_sun.snapshot.output_bound_transfer_slot = -1;
    di::g_sun.snapshot.output_bound_fence_b = 0u;
    di::g_sun.snapshot.lighting_draw_seen = false;
  }
  if (!IsSora2nd() || !cmd_list) {
    if (sun_requested) {
      record_binding(false, di::DiagnosticReason::MissingInput, falcom_world::g_state.frame.load(), 0u, -1, 0u);
    }
    return;
  }

  const reshade::api::resource_view empty_view = {};
  cmd_list->push_descriptors(reshade::api::shader_stage::pixel, reshade::api::pipeline_layout{0}, 0,
                             reshade::api::descriptor_table_update{{}, 42u, 0u, 1u, reshade::api::descriptor_type::texture_shader_resource_view, &empty_view});
  cmd_list->push_descriptors(reshade::api::shader_stage::pixel, reshade::api::pipeline_layout{0}, 0,
                             reshade::api::descriptor_table_update{{}, 46u, 0u, 1u, reshade::api::descriptor_type::texture_shader_resource_view, &empty_view});
  if (!sun_requested) {
    record_binding(false, di::DiagnosticReason::None, falcom_world::g_state.frame.load(),
                   rt12::g_rt12 ? rt12::g_rt12->generation : 0u, -1, 0u, false);
    return;
  }
  if (!dd || cmd_list != dd->immediate_cmd_list || !rt12::g_rt12
      || rt12::g_rt12->device != cmd_list->get_device() || rt12::g_rt12->failed || rt12::g_rt12->stuck
      || rt12::g_rt12->unfenced_execution) {
    record_binding(false, !dd ? di::DiagnosticReason::MissingEntry : di::DiagnosticReason::MissingInput,
                   falcom_world::g_state.frame.load(), rt12::g_rt12 ? rt12::g_rt12->generation : 0u, -1, 0u);
    return;
  }

  const uint32_t frame = falcom_world::g_state.frame.load();
  di::SunSnapshot sun;
  di::InputSnapshot input;
  di::SceneSnapshot scene;
  {
    std::shared_lock lock(di::g_sun.mutex);
    sun = di::g_sun.snapshot;
  }
  {
    std::shared_lock lock(di::g_input.mutex);
    input = di::g_input.snapshot;
  }
  {
    std::shared_lock lock(di::g_scene.mutex);
    scene = di::g_scene.snapshot;
  }
  rt12::Rt12State& state = *rt12::g_rt12;
  if (sun.status != di::SunStatus::Staged || sun.frame != frame || sun.owner_generation != state.generation
      || sun.configuration_generation != state.di_transfer.configuration_generation
      || sun.di_configuration_generation != state.di.configuration_generation || sun.scene_slot < 0
      || sun.transfer_slot < 0 || sun.transfer_slot >= static_cast<int32_t>(rt12::kRt12Slots)
      || sun.width != dd->captured_depth_w || sun.height != dd->captured_depth_h || sun.fence_b == 0u
      || input.status != di::InputStatus::Ready || input.ready_frame != frame || input.owner_generation != sun.owner_generation
      || input.configuration_generation != sun.configuration_generation || input.scene_slot != sun.scene_slot
      || input.transfer_slot != sun.transfer_slot || input.width != sun.width || input.height != sun.height
      || scene.prepared_frame != frame || scene.prepared_slot != sun.scene_slot
      || scene.prepared_generation != sun.owner_generation || scene.prepared_fence != input.scene_fence) {
    record_binding(false, di::DiagnosticReason::ConfigurationMismatch, frame, state.generation,
                   sun.transfer_slot, sun.fence_b);
    return;
  }

  rt12::Rt12DiTransferSlot& slot = state.di_transfer.slots[static_cast<uint32_t>(sun.transfer_slot)];
  if (!di::MatchesStagedSunSlotIdentity(sun, frame, state.generation, state.di_transfer.configuration_generation,
                                        state.di.configuration_generation, slot.sun_configuration_generation)
      || !slot.sun || !slot.output_staged || slot.output_frame != frame || slot.output_owner_generation != sun.owner_generation
      || slot.sun_configuration_generation != sun.configuration_generation || slot.output_scene_slot != sun.scene_slot
      || slot.output_transfer_slot != sun.transfer_slot || slot.output_width != sun.width || slot.output_height != sun.height
      || slot.output_b_fence != sun.fence_b || !slot.sun_visibility.srv11.handle
      || !rt12::ValidateDiTexture(slot.sun_visibility.srv11, sun.width, sun.height, rt12::Rt12DiTextureKind::Uint)) {
    record_binding(false, di::DiagnosticReason::InvalidResource, frame, state.generation,
                   sun.transfer_slot, sun.fence_b);
    return;
  }

  cmd_list->push_descriptors(reshade::api::shader_stage::pixel, reshade::api::pipeline_layout{0}, 0,
                             reshade::api::descriptor_table_update{{}, 42u, 0u, 1u, reshade::api::descriptor_type::texture_shader_resource_view, &slot.sun_visibility.srv11});
  shader_injection.rtxdi_sun_bound = 1.f;
  shader_injection.rtxdi_sun_darkening = di::FiniteClamped(di::g_sun_shadow_darkening, 1.f, 0.f, 1.f);
  shader_injection.rtxdi_sun_character_darkening = di::FiniteClamped(di::g_sun_character_shadow_darkening, 1.f, 0.f, 1.f);
  shader_injection.rtxdi_sun_foliage_transmission = di::FiniteClamped(di::g_sun_foliage_transmission, 0.f, 0.f, 1.f);
  shader_injection.rtxdi_sun_foliage_tint_r = di::FiniteClamped(di::g_sun_foliage_tint_r, 1.f, 0.f, 1.f);
  shader_injection.rtxdi_sun_foliage_tint_g = di::FiniteClamped(di::g_sun_foliage_tint_g, 1.f, 0.f, 1.f);
  shader_injection.rtxdi_sun_foliage_tint_b = di::FiniteClamped(di::g_sun_foliage_tint_b, 1.f, 0.f, 1.f);
  const uint32_t sun_detail_view = static_cast<uint32_t>(shader_injection.rtxdi_sun_detail_view);
  if (sun_detail_view != RTXDI_SUN_DETAIL_STATUS) {
    const auto& detail = slot.sun_detail_identity;
    const bool detail_identity_matches = detail.source_version == 1u && detail.staged && detail.recorded
        && detail.detail_view == sun_detail_view && detail.frame == frame && detail.owner_generation == sun.owner_generation
        && detail.transfer_configuration == sun.configuration_generation
        && detail.di_configuration == sun.di_configuration_generation
        && detail.sun_configuration == sun.configuration_generation && detail.scene_slot == sun.scene_slot
        && detail.transfer_slot == sun.transfer_slot && detail.width == sun.width && detail.height == sun.height
        && detail.producer_fence_b == sun.fence_b && detail.settings_identity == di::CaptureEffectiveRtxdiSettings().identity
        && slot.sun_detail.srv11.handle
        && rt12::ValidateDiTexture(slot.sun_detail.srv11, sun.width, sun.height, rt12::Rt12DiTextureKind::Uint4);
    if (detail_identity_matches) {
      cmd_list->push_descriptors(reshade::api::shader_stage::pixel, reshade::api::pipeline_layout{0}, 0,
                                 reshade::api::descriptor_table_update{{}, 46u, 0u, 1u, reshade::api::descriptor_type::texture_shader_resource_view, &slot.sun_detail.srv11});
      shader_injection.rtxdi_sun_detail_bound = 1.f;
    }
  }
  bool still_current = false;
  {
    std::unique_lock lock(di::g_sun.mutex);
    still_current = di::g_sun.snapshot.status == di::SunStatus::Staged && di::g_sun.snapshot.frame == frame
                    && di::g_sun.snapshot.transfer_slot == sun.transfer_slot
                    && di::g_sun.snapshot.fence_b == sun.fence_b;
    if (still_current) {
      di::g_sun.snapshot.output_bound = true;
      di::g_sun.snapshot.output_bound_frame = frame;
      di::g_sun.snapshot.output_bound_transfer_slot = sun.transfer_slot;
      di::g_sun.snapshot.output_bound_fence_b = sun.fence_b;
    }
  }
  if (!still_current) {
    shader_injection.rtxdi_sun_bound = 0.f;
    shader_injection.rtxdi_sun_detail_bound = 0.f;
    cmd_list->push_descriptors(reshade::api::shader_stage::pixel, reshade::api::pipeline_layout{0}, 0,
                               reshade::api::descriptor_table_update{{}, 42u, 0u, 1u, reshade::api::descriptor_type::texture_shader_resource_view, &empty_view});
    cmd_list->push_descriptors(reshade::api::shader_stage::pixel, reshade::api::pipeline_layout{0}, 0,
                               reshade::api::descriptor_table_update{{}, 46u, 0u, 1u, reshade::api::descriptor_type::texture_shader_resource_view, &empty_view});
  }
  record_binding(still_current, still_current ? di::DiagnosticReason::None : di::DiagnosticReason::ConfigurationMismatch,
                 frame, state.generation, sun.transfer_slot, sun.fence_b);
}

// DLSS Ray Reconstruction guides (M1): the compute pass at the lighting hook. Runs on the first lighting draw of the frame.
static void RunRrGuidesInline(reshade::api::command_list* cmd_list) {
  if (falcom_world::rr::g_rr_perf_guides < 0.5f) return;  // TEMP perf test (remove)
  namespace rr = falcom_world::rr;
  namespace rtgi = falcom_world::rtgi;
  if (!rr::RrRequested() || !IsSora2nd()) return;
  auto* dev = cmd_list->get_device();
  auto* dd = dev ? dev->get_private_data<DeviceData>() : nullptr;
  auto* world_cl = falcom_world::GetWorldCommandListData(cmd_list);
  if (dd == nullptr || world_cl == nullptr || cmd_list != dd->immediate_cmd_list) return;
  // One attempt per frame: the first lighting draw decides, even when its inputs are missing (M2a falls back to SR).
  const uint32_t frame = falcom_world::g_state.frame.load();
  if (rr::g_rr_guides_attempt_frame.exchange(frame) == frame) return;
  const bool inputs_ready = dd->captured_color_srv.handle != 0u && dd->captured_color_live.load()
                            && dd->captured_color_frame == dd->frame_index
                            && dd->captured_depth_srv.handle != 0u && dd->captured_depth_live.load()
                            && dd->captured_mrt_normal_srv.handle != 0u && dd->captured_mrt_live.load()
                            && dd->captured_scene_cbv_valid
                            && world_cl->ps_srv_view[rtgi::kLightingMrt2Register].handle != 0u
                            && world_cl->ps_srv_view[rtgi::kLightingDeferredRegister].handle != 0u;
  if (!inputs_ready) {
    rr::SetGuideReason(rr::GuideReason::InputsMissing);
    return;
  }
  rr::DispatchGuides(dev, cmd_list, {
                                        .color_view = dd->captured_color_srv,
                                        .mrt0_view = dd->captured_mrt_normal_srv,
                                        .mrt2_view = world_cl->ps_srv_view[rtgi::kLightingMrt2Register],
                                        .deferred_view = world_cl->ps_srv_view[rtgi::kLightingDeferredRegister],
                                        .depth_view = dd->captured_depth_srv,
                                        .scene_cbv_view = dd->captured_scene_cbv_view,
                                        .width = dd->captured_depth_w,
                                        .height = dd->captured_depth_h,
                                    });
}

// RTGI: copies the lighting output after the draw; the next frame's RTGI trace reads it for on-screen hits.
static void OnDrawnLightingShader(reshade::api::command_list* cmd_list) {
  auto* dev = cmd_list->get_device();
  auto* dd = dev ? dev->get_private_data<DeviceData>() : nullptr;
  if (dd == nullptr || cmd_list != dd->immediate_cmd_list) return;
  if (IsSora2nd()) {
    namespace di = falcom_world::rtxdi;
    const uint32_t frame_index = falcom_world::g_state.frame.load();
    bool local_bound = false;
    bool local_draw_seen = false;
    uint32_t local_frame = UINT32_MAX;
    int32_t local_transfer_slot = -1;
    uint64_t local_fence = 0u;
    bool sun_bound = false;
    bool sun_draw_seen = false;
    uint32_t sun_frame = UINT32_MAX;
    int32_t sun_transfer_slot = -1;
    uint64_t sun_fence = 0u;
    {
      std::unique_lock lock(di::g_di.mutex);
      auto& output = di::g_di.snapshot;
      di::RecordOutputLightingDrawSeen(&output.output_bound, &output.output_bound_frame,
                                       &output.output_bound_transfer_slot, &output.output_bound_fence_b,
                                       &output.lighting_draw_seen, frame_index, output.output_frame,
                                       output.output_transfer_slot, output.output_fence_b);
#if defined(FALCOM_NRD_ENABLED)
      di::RecordOutputLightingDrawSeen(&output.nrd_diffuse_bound, &output.nrd_diffuse_frame,
                                       &output.nrd_diffuse_transfer_slot, &output.nrd_diffuse_fence_b,
                                       &output.nrd_diffuse_lighting_draw_seen, frame_index, output.nrd_diffuse_frame,
                                       output.nrd_diffuse_transfer_slot, output.nrd_diffuse_fence_b);
#endif
      local_bound = output.output_bound;
      local_draw_seen = output.lighting_draw_seen;
      local_frame = output.output_frame;
      local_transfer_slot = output.output_transfer_slot;
      local_fence = output.output_fence_b;
    }
    {
      std::unique_lock lock(di::g_sun.mutex);
      auto& sun = di::g_sun.snapshot;
      di::RecordOutputLightingDrawSeen(&sun.output_bound, &sun.output_bound_frame,
                                       &sun.output_bound_transfer_slot, &sun.output_bound_fence_b,
                                       &sun.lighting_draw_seen, frame_index, sun.frame, sun.transfer_slot, sun.fence_b);
      sun_bound = sun.output_bound;
      sun_draw_seen = sun.lighting_draw_seen;
      sun_frame = sun.frame;
      sun_transfer_slot = sun.transfer_slot;
      sun_fence = sun.fence_b;
    }
    auto& diagnostics = di::GetDiagnosticState();
    const bool diagnostic_enabled = di::DiagnosticCaptureEnabled(diagnostics);
    if (diagnostic_enabled) {
      const uint64_t epoch = di::DiagnosticCaptureEpoch(diagnostics);
      di::RecordDiagnostic(diagnostics, epoch,
                           {.frame = frame_index,
                            .capture_frame = local_frame,
                            .fence = local_fence,
                            .transfer_slot = local_transfer_slot,
                            .flags = (local_bound ? 1u : 0u) | (local_draw_seen ? 2u : 0u),
                            .stage = di::DiagnosticStage::LightingDraw,
                            .reason = di::DiagnosticReason::None,
                            .operation = di::DiagnosticOperation::LightingCallback});
      di::RecordDiagnostic(diagnostics, epoch,
                           {.frame = frame_index,
                            .capture_frame = sun_frame,
                            .fence = sun_fence,
                            .transfer_slot = sun_transfer_slot,
                            .flags = (sun_bound ? 1u : 0u) | (sun_draw_seen ? 2u : 0u),
                            .stage = di::DiagnosticStage::LightingDraw,
                            .reason = di::DiagnosticReason::None,
                            .operation = di::DiagnosticOperation::LightingCallback});
    }
  }
  const auto& frame = falcom_world::rtgi::g_rtgi_frame;
  if (!frame.producing || frame.dispatched_frame != dd->frame_index) return;
  auto* cs = renodx::utils::state::GetCurrentState(cmd_list);
  if (cs == nullptr || cs->render_targets.empty() || cs->render_targets[0].handle == 0u) return;
  falcom_world::rtgi::CaptureRtgiRadiance(dev, cmd_list, cs->render_targets[0], dd->frame_index);
}

static bool OnBeforeLightingShaderDraw(reshade::api::command_list* cmd_list) {
  PromoteRtxdiSceneBinding(cmd_list);
  // IMPORTANT: returning false would BYPASS the draw (skip it entirely).
  if (IsKai() && g_kai_hero_light_toggle > 0.5f) {
    auto* context11 = reinterpret_cast<ID3D11DeviceContext*>(cmd_list->get_native());  // NOLINT(performance-no-int-to-ptr)
    ID3D11ShaderResourceView* srv = nullptr;
    if (context11 != nullptr) context11->PSGetShaderResources(falcom_world::rtgi::kLightingLightsRegister, 1, &srv);
    if (srv != nullptr) {
      ID3D11Resource* resource = nullptr;
      srv->GetResource(&resource);
      g_kai_lights_resource.store(reinterpret_cast<uintptr_t>(resource), std::memory_order_relaxed);
      if (resource != nullptr) resource->Release();
      srv->Release();
    }
  }
  shader_injection.gtvbao_debug_view = (float)ActiveGtvbaoDebugMode();
  shader_injection.gtvbao_dedicated_bound = 0.f;
  shader_injection.gtvbao_vbgi_bound = 0.f;
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
                  {}, 24u, 0, 1, reshade::api::descriptor_type::texture_shader_resource_view, &dd->isfast_noise_srv});
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

  namespace perf = falcom_world::bvh;
  const bool perf_immediate = reinterpret_cast<uint64_t>(cmd_list) == perf::g_pool.immediate_cmd_list.load(std::memory_order_relaxed);
  const auto rt_start = perf::PerfNow();
  if (perf_immediate && !perf::g_perf_gate.rt_seen) {
    perf::AddPerfBlockMs(perf::kPerfBlockSegToRt, perf::LiveElapsedMs(perf::g_perf_gate.frame_start));
    perf::g_perf_gate.rt_seen = true;
  }
  // RT dispatches null most compute slots and rebind their own. Snapshot the game's compute state
  // once here and restore it after RTGI: the engine's volumetric-fog compute reads those bindings.
  std::optional<renodx::utils::state::CommandListSnapshot> game_compute_state;
  if (falcom_world::rtxdi::SceneRequested() || falcom_world::rtao::RtaoRequested() || falcom_world::rtls::RtlsRequested() || falcom_world::rtsun::RtSunRequested()
      || (falcom_world::rr::RrRequested() && falcom_world::rr::g_rr_perf_guides > 0.5f  // TEMP perf test (remove)
          && falcom_world::rr::g_rr_guides_attempt_frame.load() != falcom_world::g_state.frame.load())) {
    game_compute_state = renodx::utils::state::GetSnapshot(cmd_list);
  }
  // RTAO runs before the shadow pass.
  RunRtaoInline(cmd_list);

  // RT Local Shadows after RTAO and before RTGI (order RTAO -> RTLS -> RTGI); independent of both.
  RunRtlsInline(cmd_list);

  // RT Sun Shadows (R3a): after RT Local Shadows, same guards; the raw term only, the image is not changed.
  if (IsSora2nd() && falcom_world::rtxdi::g_sun_requested > 0.5f) {
    shader_injection.rtsun_flags = 0.f;
  } else {
    RunRtSunInline(cmd_list);
  }

  // RTGI after RTAO: it runs only on a frame where RTAO dispatched.
  RunRtgiInline(cmd_list);
  PrepareRtxdiSceneInline(cmd_list);
  auto* lighting_device = cmd_list ? cmd_list->get_device() : nullptr;
  BindRtxdiLocalOutput(cmd_list, lighting_device ? lighting_device->get_private_data<DeviceData>() : nullptr);
  BindRtxdiSunOutput(cmd_list, lighting_device ? lighting_device->get_private_data<DeviceData>() : nullptr);
  // DLSS Ray Reconstruction guides: builds the RR guide buffers; the lighting output is not changed.
  RunRrGuidesInline(cmd_list);
  if (game_compute_state) {
    game_compute_state->ApplyCompute(true);
  }
  perf::AddPerfBlockMs(perf::kPerfBlockLightingRt, perf::LiveElapsedMs(rt_start));
  if (perf_immediate) perf::g_perf_gate.rt_end = perf::PerfNow();

  // Contact Shadows. Placed here, before the GTVBAO/DynCube early-out
  // below, because those two are entirely independent of the shadow passes: on a
  // frame where only the shadows are enabled this hook must still run them.
  DeployShadows(cmd_list, 1);

  // Dynamic Cubemaps � standalone, must run even when GTVBAO/SSR off (any Falcom title)
  const bool dyncube_active = shader_injection.dynCube_enabled > 0.5f;
  const bool gtvbao_active = shader_injection.gtvbao_mode > 0.5f && !falcom_world::rtao::RtaoRequested();
  const bool vbgi_active = VbgiRunning();
  // RTAO on: GTVBAO's t23 block is skipped (and the early-out below may return), so its flags are cleared here.
  if (falcom_world::rtao::RtaoRequested()) {
    shader_injection.gtvbao_vbgi_bound = 0.f;
    shader_injection.gtvbao_vbgi_debug = 0.f;
    shader_injection.gtvbao_mrt_normal_debug = 0.f;
  }
  shader_injection.vbgi_debug_view = 0.f;
  // Reset every frame, including when GTVBAO is off: the lighting shader reads this to decide whether the
  // t23 push is a diagnostic texture it should display. Leaving it set would make a later frame show a stale debug view.
  shader_injection.gtvbao_mrt_normal_debug = 0.f;
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
  if (!gtvbao_active && !vbgi_active && !dyncube_active) return true;
  if (!cmd_list) return true;

  auto* dev = cmd_list->get_device();
  auto* dd = dev ? dev->get_private_data<DeviceData>() : nullptr;
  if (!dd) return true;

  // -- VBGI radiance prepass: at this draw in every dispatch mode (the lighting inputs are only alive here). --
  if (vbgi_active && dd->captured_depth_srv.handle && dd->captured_scene_cbv_valid) {
    auto* cs = renodx::utils::state::GetCurrentState(cmd_list);
    renodx::utils::state::CommandListState prev = {};
    if (cs) prev = *cs;
    RunVbgiRadiance(cmd_list, dd);
    ApplyGTVBAOCSDispatchFix(cmd_list, cs, prev);
  }

  // -- Deferred dispatch path: capture snapshots for OnPresent (kai-style). --
  // Plain copies only (no queries): resource pairs + live flags travel with the views.
  // GTVBAO/VBGI: with both off there is nothing to defer to.
  if (g_cpuopt_deferred_dispatch > 0.5f && (gtvbao_active || vbgi_active)) {
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

  // -- Inline dispatch: Run GTVBAO/VBGI on this frame's command list (only when NOT deferred). --
  if ((gtvbao_active || vbgi_active) && g_cpuopt_deferred_dispatch < 0.5f) {
    if (dd->captured_depth_srv.handle && dd->captured_scene_cbv_valid) {
      auto* cs = renodx::utils::state::GetCurrentState(cmd_list);
      renodx::utils::state::CommandListState prev = {};
      if (cs) prev = *cs;

      bool ao_ok = false;
      if (gtvbao_active && dd->ao_term_a_srv.handle) {
        const std::string liveDepth = dd->captured_depth_srv.handle ? dd->captured_depth_dims : "none";
        const std::string wantDims = std::to_string(dd->working_width) + "x" + std::to_string(dd->working_height);
        CSLog("gtvbao", std::string("inline invoke working=") + wantDims + " liveDepth=" + liveDepth,
              liveDepth != wantDims);
        ao_ok = RunGTVBAO(cmd_list, dd);
        CSLog("gtvbao", std::string("inline invoke exit") + (ao_ok ? " ok" : " FAILED"), !ao_ok);
      }
      if (vbgi_active) {
        const bool ok_vbgi = RunVBGI(cmd_list, dd, ao_ok);
        CSLog("vbgi", std::string("inline invoke exit") + (ok_vbgi ? " ok" : " FAILED"), !ok_vbgi);
      }

      ApplyGTVBAOCSDispatchFix(cmd_list, cs, prev);
    }
  }

  if ((!vbgi_active || g_vbgi_temporal_enabled > 0.5f || g_vbgi_temporal_far_only < 0.5f
       || g_vbgi_far_enabled < 0.5f || g_cpuopt_deferred_dispatch > 0.5f)
      && dd->vbgi_far_color_denoise.history_color.texture[0].handle != 0u) {
    falcom_world::rtgi::ReleaseRtgiDenoiseTextures(dev, &dd->vbgi_far_color_denoise);
    dd->vbgi_far_color_last = false;
    dd->vbgi_far_color_prev_params_valid = false;
  }
  if (!gtvbao_active && dd->gtvbao_svgf_raw.texture[0].handle != 0u) {
    falcom_world::rtgi::ReleaseRtgiDenoiseTextures(dev, &dd->gtvbao_denoise);
    falcom_world::rtgi::DestroyRtgiTextureSet(dev, &dd->gtvbao_svgf_raw);
  }

  // Push the GTVBAO AO result at t22.
  // In inline mode: fresh from dispatch above.
  // In deferred mode: result from previous frame's OnPresent dispatch.
  // The buffer is the newest SVGF chain output (gtvbao_denoise.output).
  if (gtvbao_active) {
    const reshade::api::resource_view srv = dd->gtvbao_denoise.output.srv[dd->gtvbao_denoise.output_index];
    if (srv.handle) {
      cmd_list->push_descriptors(
          reshade::api::shader_stage::pixel,
          reshade::api::pipeline_layout{0},
          0,
          reshade::api::descriptor_table_update{
              {},
              kLightingGtvbaoRegister,
              0,
              1,
              reshade::api::descriptor_type::texture_shader_resource_view,
              &srv,
          });
      shader_injection.gtvbao_dedicated_bound = 1.f;
    }
  }

  // -- SSGI push t23 (GI is produced by RunGTVBAO or RunVBGI) --
  if (gtvbao_active || vbgi_active) {
    shader_injection.gtvbao_vbgi_bound = 0.f;
    shader_injection.gtvbao_vbgi_debug = 0.f;

    // Determine what to push to t23.
    reshade::api::resource_view push_srv = {};
    bool do_push = false;

    // Compute debug views 22-35: push the GTVBAO debug UAV output to the lighting shader's t23 slot. VBGI "Final GI"
    // (mode 30) shows the denoised output instead, so it reflects temporal and spatial filtering.
    const uint64_t max_age = (g_cpuopt_deferred_dispatch > 0.5f) ? 1u : 0u;
    bool gi_push = false;
    if (vbgi_active && ActiveGtvbaoDebugMode() == 30 && VbgiGiFresh(dd, max_age)) {
      push_srv = dd->vbgi_denoise.output.srv[dd->vbgi_denoise.output_index];
      do_push = true;
    } else if (IsGtvbaoComputeDebug(ActiveGtvbaoDebugMode())
               && dd->gtvbao_debug_frame != UINT64_MAX
               && (dd->frame_index - dd->gtvbao_debug_frame) <= max_age
               && dd->gtvbao_debug_mode_written == (float)ActiveGtvbaoDebugMode()) {
      push_srv = dd->debug_srv;
      do_push = true;
    } else if (vbgi_active && !IsGtvbaoComputeDebug(ActiveGtvbaoDebugMode()) && VbgiGiFresh(dd, max_age)) {
      // VBGI composite: the newest denoised output, bound for the lighting shader's cachedVBGI.
      push_srv = dd->vbgi_denoise.output.srv[dd->vbgi_denoise.output_index];
      do_push = true;
      gi_push = true;
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
                {},
                giRegister,
                0,
                1,
                reshade::api::descriptor_type::texture_shader_resource_view,
                &push_srv,
            });
        if (gi_push)
          shader_injection.gtvbao_vbgi_bound = 1.f;
        else
          shader_injection.gtvbao_mrt_normal_debug = 1.f;
      }
    }
  }  // gtvbao_active || vbgi_active (t23 section)

  // -- Dynamic Cubemaps (Sora2nd): t17 override � Phase 0A/B + Phase 1/2 standalone --
  // D3D11 hazard: capture writes UAV, then lighting reads SRV. RunDynCube* does
  // UAV->SRV barrier. We dispatch inline here before the draw that consumes t17.
  // A swapchain resize destroys these resources; wait for the shared resize guard
  // before the draw hook can recreate or dispatch them.
  if (dyncube_active && dd->frame_index >= dd->resize_guard_until_frame) {
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
                              (std::string("[DynCube] resize pending ") + std::to_string(dd->dyncube_size) + " -> " + std::to_string(wantSize)).c_str());
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
      CSLog("ctx", std::string("scheduler on ") + ((dd->immediate_cmd_list != nullptr && dd->immediate_cmd_list == cmd_list) ? "immediate" : "DEFERRED"));

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
        for (auto& u : dd->dyncube_cam_uav)
          if (u.handle) cmd_list->clear_unordered_access_view_float(u, zero4);
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
      ConsumeDynCubeStagedValidity(dev, dd, dd->immediate_cmd_list != nullptr && dd->immediate_cmd_list == cmd_list);

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
      if (!dd->dyncube_ggx_valid && dd->dyncube_phase != DeviceData::DynCubePhase::Filter
          && RunDynCubeCapture(cmd_list, dd)) {
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
          // Armed staged validity not consumed yet: promotion and filter must land in the same
          // tick. Unarmed or failed copies do not hold the phase (blocking-map behavior).
          if (dd->dyncube_boxCopyPending && dd->dyncube_valid_event_armed) break;
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
            const bool time_filter = DynCubeGpuTimingOn(cmd_list, dd);
            if (time_filter) {
              falcom_world::bvh::BeginGpuTimer(dev, cmd_list, &dd->dyncube_gpu_filter_timer, true);
            }
            const bool filtered = RunDynCubeFilter(cmd_list, dd, (shader_injection.dynCube_ggx > 0.5f));
            if (time_filter) {
              falcom_world::bvh::EndGpuTimer(cmd_list, &dd->dyncube_gpu_filter_timer);
            }
            if (filtered) {
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
                if (vSoften > 1e-4f || vStrength < 1.f - 1e-4f) {
                  // Built on the next scheduler tick so this frame carries only the filter.
                  dd->dyncube_phase = DeviceData::DynCubePhase::Variant;
                } else {
                  dd->dyncube_variant_valid = false;
                  dd->dyncube_phase = DeviceData::DynCubePhase::Done;
                }
              }
            }
          }
          break;
        case DeviceData::DynCubePhase::Variant:
          // Variant cube on its own frame. Done either way: after a loading wipe or resize
          // in between the build returns false, and the settings-drift refresh rebuilds it when wanted again.
          (void)RunDynCubeVariant(cmd_list, dd);
          dd->dyncube_phase = DeviceData::DynCubePhase::Done;
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
            && dd->dyncube_phase != DeviceData::DynCubePhase::Variant
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
      if (shader_injection.dynCube_debug_logging > 0.5f || g_dyncube_gpu_timing > 0.5f) {
        static auto last_log = std::chrono::steady_clock::now() - std::chrono::seconds(2);
        auto now = std::chrono::steady_clock::now();
        if (now - last_log >= std::chrono::seconds(1)) {
          last_log = now;
          const char* phaseName = (dd->dyncube_phase == DeviceData::DynCubePhase::Capture)   ? "Capture"
                                  : (dd->dyncube_phase == DeviceData::DynCubePhase::Filter)  ? "Filter"
                                  : (dd->dyncube_phase == DeviceData::DynCubePhase::Variant) ? "Variant"
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
          msg += " pollMiss=" + std::to_string(dd->dyncube_valid_poll_misses);
          msg += " timeouts=" + std::to_string(dd->dyncube_valid_timeouts);
          msg += " mapWaitMaxMs=" + std::to_string(dd->dyncube_map_wait_ms_max);
          const bool read_timers = DynCubeGpuTimingOn(cmd_list, dd);
          // Timer stats are written on the immediate context only; off it, leave them alone (no gpuMs part).
          if (g_dyncube_gpu_timing > 0.5f && read_timers) {
            auto timer_text = [&](falcom_world::bvh::GpuTimer* timer) {
              if (read_timers) {
                falcom_world::bvh::PollGpuTimer(falcom_world::bvh::GpuTimerContext(cmd_list), timer);
              }
              std::ostringstream text;
              if (timer->samples == 0u) {
                text << "-";
              } else {
                text << std::fixed << std::setprecision(2) << timer->ms_max << "/" << (timer->ms_sum / timer->samples);
              }
              text << " (n=" << timer->samples << " skip=" << timer->skipped << ")";
              falcom_world::bvh::ClearGpuTimerStats(timer);
              return text.str();
            };
            msg += " gpuMs cap=" + timer_text(&dd->dyncube_gpu_capture_timer);
            msg += " flt=" + timer_text(&dd->dyncube_gpu_filter_timer);
            msg += " var=" + timer_text(&dd->dyncube_gpu_variant_timer);
            msg += " ssr=" + timer_text(&dd->dyncube_gpu_ssr_timer);
            msg += " box=" + timer_text(&dd->dyncube_gpu_box_timer);
            {
              const double avg = dd->present_count > 0u ? dd->present_sum_ms / dd->present_count : 0.0;
              std::ostringstream present_text;
              if (dd->present_count == 0u) {
                present_text << "-";
              } else {
                present_text << std::fixed << std::setprecision(2) << dd->present_max_ms << "/" << avg;
              }
              present_text << " (n=" << dd->present_count << " slow=" << dd->present_slow_count << ")";
              msg += " presentMs=" + present_text.str();
              if (avg > 0.0) dd->present_prev_avg_ms = avg;
              ResetPresentWindow(dd);
            }
          }
          reshade::log::message(reshade::log::level::info, msg.c_str());
          dd->dyncube_map_wait_ms_max = 0.f;
        }
      }
    }

    // Bind the history position cube (t29) � Dynamic validity source for the blend.
    // Reads the validated readSet (delayed-validate commit), never the raw write set.
    {
      reshade::api::resource_view histPosSrv = dd->dyncube_hist[dd->dyncube_readSet].pos_cube_srv;
      if (histPosSrv.handle) {
        cmd_list->push_descriptors(reshade::api::shader_stage::pixel, reshade::api::pipeline_layout{0}, 0,
                                   reshade::api::descriptor_table_update{{}, kDynCubeHistPosRegister, 0, 1, reshade::api::descriptor_type::texture_shader_resource_view, &histPosSrv});
      }
    }
    // Bind the game's vanilla cubemap (t30) for the SSR -> Dynamic -> Vanilla fallback.
    if (dd->captured_vanilla_env_srv.handle) {
      cmd_list->push_descriptors(reshade::api::shader_stage::pixel, reshade::api::pipeline_layout{0}, 0,
                                 reshade::api::descriptor_table_update{{}, kDynCubeVanillaRegister, 0, 1, reshade::api::descriptor_type::texture_shader_resource_view, &dd->captured_vanilla_env_srv});
    }
    // Bind the SSR result (t31 blurred) + raw (t32) � produced once per frame by the scheduler.
    if (shader_injection.dynCube_ssr_enabled > 0.5f && dd->dyncube_ssr_blur_srv.handle) {
      cmd_list->push_descriptors(reshade::api::shader_stage::pixel, reshade::api::pipeline_layout{0}, 0,
                                 reshade::api::descriptor_table_update{{}, kDynCubeSSRRegister, 0, 1, reshade::api::descriptor_type::texture_shader_resource_view, &dd->dyncube_ssr_blur_srv});
      if (dd->dyncube_ssr_raw_srv.handle) {
        cmd_list->push_descriptors(reshade::api::shader_stage::pixel, reshade::api::pipeline_layout{0}, 0,
                                   reshade::api::descriptor_table_update{{}, kDynCubeSSRRawRegister, 0, 1, reshade::api::descriptor_type::texture_shader_resource_view, &dd->dyncube_ssr_raw_srv});
      }
    }
    // Bind the captured vanilla ssr1 march result (t28) for the SSR replacement
    // debug view. Only needed for diagnostics; skipped when never captured or dead.
    if (dd->captured_ssr1_srv.handle && dd->captured_ssr1_live) {
      cmd_list->push_descriptors(reshade::api::shader_stage::pixel, reshade::api::pipeline_layout{0}, 0,
                                 reshade::api::descriptor_table_update{{}, 28u, 0, 1, reshade::api::descriptor_type::texture_shader_resource_view, &dd->captured_ssr1_srv});
    }

    if (dbg == 3) {
      // Solid face colors � validates t17 binding + handedness without capture.
      // Uses the DEDICATED solid cube so history resources are never touched.
      if (!forceVanilla && RunDynCubeSolid(cmd_list, dd)) {
        cmd_list->push_descriptors(reshade::api::shader_stage::pixel, reshade::api::pipeline_layout{0}, 0,
                                   reshade::api::descriptor_table_update{{}, kDynCubeRegister, 0, 1, reshade::api::descriptor_type::texture_shader_resource_view, &dd->dyncube_solid_cube_srv});
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
                                   reshade::api::descriptor_table_update{{}, kDynCubeRegister, 0, 1, reshade::api::descriptor_type::texture_shader_resource_view, &t17srv});
      }
    }
  } else if (dyncube_active) {
    CSLog("dyncube", "scheduler skipped during resize guard");
  }

  return true;
}

static bool OnBeforeSsaoShaderDraw(reshade::api::command_list* cmd_list) {
  // Used as on_replace callback via CustomShaderEntryCallback.
  // Return true = use our replacement SSAO shader (with GTVBAO gate).
  //
  // This is also the deploy point for the Sora custom camera contact march: these
  // two passes run it, so they need the same IS-FAST volume and the same
  // cs_noise_frame the Kai character pass does. It is the same helper, not a copy,
  // because a copy is how the two families would end up on different registers.
  charShadowsPushISFAST(cmd_list);
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
  d->working_width = w;
  d->working_height = h;

  {
    reshade::api::sampler_desc sd = {};
    sd.filter = reshade::api::filter_mode::min_mag_mip_point;
    sd.address_u = sd.address_v = sd.address_w = reshade::api::texture_address_mode::clamp;
    dev->create_sampler(sd, &d->point_clamp_sampler);
  }
  {
    reshade::api::sampler_desc sd = {};
    sd.filter = reshade::api::filter_mode::min_mag_mip_linear;
    sd.address_u = sd.address_v = sd.address_w = reshade::api::texture_address_mode::clamp;
    dev->create_sampler(sd, &d->linear_clamp_sampler);
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

  // �� Low-resolution spatial pipeline: ao/edges/raw-GI follow the AO evaluation
  // resolution; everything else stays full. Ceiling division (odd-safe).
  const int res_index = std::clamp((int)std::lround(shader_injection.gtvbao_resolution), 0, 1);
  const uint32_t div = 1u << res_index;
  const uint32_t aw = (w + div - 1u) / div;
  const uint32_t ah = (h + div - 1u) / div;
  d->ao_width = aw;
  d->ao_height = ah;
  d->last_created_gtvbao_resolution = std::log2((float)div);
  mk(aw, ah, reshade::api::format::r32_uint, &d->ao_term_a_texture, &d->ao_term_a_srv, &d->ao_term_a_uav);
  // VBGI raw gather target: always allocated; RunVBGI never resizes it.
  const uint32_t vdiv = falcom_world::rtao::RtgiTraceDivisor(g_vbgi_resolution, kVbgiMaxResolutionIndex);
  const uint32_t vw = (w + vdiv - 1u) / vdiv;
  const uint32_t vh = (h + vdiv - 1u) / vdiv;
  d->vbgi_width = vw;
  d->vbgi_height = vh;
  d->last_created_vbgi_resolution = std::log2((float)vdiv);
  mk(vw, vh, reshade::api::format::r16g16b16a16_float, &d->vbgi_raw_texture, &d->vbgi_raw_srv, &d->vbgi_raw_uav);
  // VBGI radiance chain: r11g11b10_float over the VBGI domain, 5 mips (mip 0 written by the prepass, 1-4 by the box filter).
  {
    reshade::api::resource_desc rd = {};
    rd.type = reshade::api::resource_type::texture_2d;
    rd.texture = {vw, vh, 1, (uint16_t)5, reshade::api::format::r11g11b10_float, 1};
    rd.heap = reshade::api::memory_heap::gpu_only;
    rd.usage = reshade::api::resource_usage::shader_resource | reshade::api::resource_usage::unordered_access;
    dev->create_resource(rd, nullptr, reshade::api::resource_usage::shader_resource, &d->vbgi_radiance_texture);
    dev->create_resource_view(d->vbgi_radiance_texture, reshade::api::resource_usage::shader_resource,
                              reshade::api::resource_view_desc(reshade::api::resource_view_type::texture_2d,
                                                               reshade::api::format::r11g11b10_float, 0, 5, 0, 1),
                              &d->vbgi_radiance_srv);
    for (uint32_t m = 0; m < 4; ++m)
      dev->create_resource_view(d->vbgi_radiance_texture, reshade::api::resource_usage::shader_resource,
                                reshade::api::resource_view_desc(reshade::api::resource_view_type::texture_2d,
                                                                 reshade::api::format::r11g11b10_float, m, 1, 0, 1),
                                &d->vbgi_radiance_mip_srv[m]);
    for (uint32_t m = 0; m < 5; ++m)
      dev->create_resource_view(d->vbgi_radiance_texture, reshade::api::resource_usage::unordered_access,
                                reshade::api::resource_view_desc(reshade::api::resource_view_type::texture_2d,
                                                                 reshade::api::format::r11g11b10_float, m, 1, 0, 1),
                                &d->vbgi_radiance_mip_uav[m]);
  }

  // Foliage mask (AO working domain R8_UINT: full in Full, AO-sized in low-res modes)
  mk(aw, ah, reshade::api::format::r8_uint,
     &d->foliage_mask_texture, &d->foliage_mask_srv, &d->foliage_mask_uav);
  mk(w, h, reshade::api::format::r8g8b8a8_unorm,
     &d->debug_texture, &d->debug_srv, &d->debug_uav);
}

static void DestroyGTVBAOResources(reshade::api::device* dev, DeviceData* d) {
  if (!dev || !d) return;
  auto dv = [&](reshade::api::resource_view& v) { if (v.handle) { dev->destroy_resource_view(v); v = {}; } };
  auto dr = [&](reshade::api::resource& r) { if (r.handle) { dev->destroy_resource(r); r = {}; } };
  auto dp = [&](reshade::api::pipeline& p) { if (p.handle) { dev->destroy_pipeline(p); p = {}; } };
  auto dl = [&](reshade::api::pipeline_layout& l) { if (l.handle) { dev->destroy_pipeline_layout(l); l = {}; } };

  dv(d->depth_mips_srv);
  for (auto& u : d->depth_mips_uavs) dv(u);
  dr(d->depth_mips_texture);
  dv(d->ao_term_a_srv);
  dv(d->ao_term_a_uav);
  dr(d->ao_term_a_texture);
  if (d->point_clamp_sampler.handle) {
    dev->destroy_sampler(d->point_clamp_sampler);
    d->point_clamp_sampler = {};
  }
  dp(d->prefilter_pipeline);
  dp(d->main_pipeline);
  dl(d->prefilter_layout);
  dl(d->main_layout);
  dp(d->vbgi_main_pipeline);
  DestroyGTVBAODescriptorTables(dev, &d->vbgi_main_tables);
  dv(d->vbgi_raw_srv);
  dv(d->vbgi_raw_uav);
  dr(d->vbgi_raw_texture);
  dl(d->vbgi_main_layout);
  dp(d->vbgi_radiance_pipeline);
  dp(d->vbgi_mips_pipeline);
  dl(d->vbgi_radiance_layout);
  dl(d->vbgi_mips_layout);
  DestroyGTVBAODescriptorTables(dev, &d->vbgi_radiance_tables);
  DestroyGTVBAODescriptorTables(dev, &d->vbgi_mips_tables);
  dp(d->vbgi_radiance_far_pipeline);
  dl(d->vbgi_radiance_far_layout);
  DestroyGTVBAODescriptorTables(dev, &d->vbgi_radiance_far_tables);
  for (auto& v : d->vbgi_radiance_mip_srv) dv(v);
  for (auto& u : d->vbgi_radiance_mip_uav) dv(u);
  dv(d->vbgi_radiance_srv);
  dr(d->vbgi_radiance_texture);
  falcom_world::rtgi::DestroyRtgiDenoiseSet(dev, &d->vbgi_far_color_denoise);
  d->vbgi_far_color_denoise = {};
  d->vbgi_far_color_last = false;
  d->vbgi_far_color_frame = UINT64_MAX;
  d->vbgi_far_color_full_width = d->vbgi_far_color_full_height = 0u;
  d->vbgi_far_color_prev_params_valid = false;
  falcom_world::rtgi::DestroyRtgiDenoiseSet(dev, &d->vbgi_denoise);
  d->vbgi_denoise = falcom_world::rtgi::RtgiDenoiseSet{};
  falcom_world::bvh::DestroyBuffer(dev, &d->vbgi_stats_uav, &d->vbgi_stats_buffer);
  d->vbgi_gi_frame = UINT64_MAX;
  d->vbgi_temporal_last = false;
  d->vbgi_last_frame = UINT64_MAX;
  d->vbgi_prev_params_valid = false;
  falcom_world::rtgi::DestroyRtgiDenoiseSet(dev, &d->gtvbao_denoise);
  d->gtvbao_denoise = falcom_world::rtgi::RtgiDenoiseSet{};
  falcom_world::rtgi::DestroyRtgiTextureSet(dev, &d->gtvbao_svgf_raw);
  dp(d->vbgi_far_main_pipeline);
  dp(d->vbgi_far_resolve_pipeline);
  dp(d->vbgi_far_compose_pipeline);
  dl(d->vbgi_far_main_layout);
  dl(d->vbgi_far_layout);
  DestroyGTVBAODescriptorTables(dev, &d->vbgi_far_main_tables);
  DestroyGTVBAODescriptorTables(dev, &d->vbgi_far_tables);
  falcom_world::rtgi::DestroyRtgiTextureSet(dev, &d->vbgi_far_cand_pos);
  falcom_world::rtgi::DestroyRtgiTextureSet(dev, &d->vbgi_far_cand_rad);
  falcom_world::rtgi::DestroyRtgiTextureSet(dev, &d->vbgi_far_cand_nrm);
  falcom_world::rtgi::DestroyRtgiTextureSet(dev, &d->vbgi_far_raw);
  falcom_world::rtgi::DestroyRtgiTextureSet(dev, &d->vbgi_far_joint);
  d->vbgi_far_was_on = false;
  d->vbgi_far_frame = UINT64_MAX;
  d->vbgi_far_prev_params_valid = false;
  dp(d->vbgi_far_temporal_pipeline);
  falcom_world::rtgi::DestroyRtgiTextureSet(dev, &d->vbgi_far_res_pos);
  falcom_world::rtgi::DestroyRtgiTextureSet(dev, &d->vbgi_far_res_rad);
  falcom_world::rtgi::DestroyRtgiTextureSet(dev, &d->vbgi_far_res_nrm);
  d->vbgi_far_res_index = 0u;
  d->vbgi_far_reuse_last = false;
  d->vbgi_far_reuse_frame = UINT64_MAX;
  dp(d->vbgi_far_spatial_pipeline);
  falcom_world::rtgi::DestroyRtgiTextureSet(dev, &d->vbgi_far_sp_pos);
  falcom_world::rtgi::DestroyRtgiTextureSet(dev, &d->vbgi_far_sp_rad);
  falcom_world::rtgi::DestroyRtgiTextureSet(dev, &d->vbgi_far_sp_nrm);
  DestroyGTVBAODescriptorTables(dev, &d->prefilter_tables);
  DestroyGTVBAODescriptorTables(dev, &d->main_tables);
  DestroyGTVBAODescriptorTables(dev, &d->foliage_mask_tables);
  // GI resources (now integrated � no separate VBGI pipeline)
  dv(d->foliage_mask_srv);
  dv(d->foliage_mask_uav);
  dr(d->foliage_mask_texture);
  dp(d->foliage_mask_pipeline);
  dl(d->foliage_mask_layout);
  dv(d->debug_srv);
  dv(d->debug_uav);
  dr(d->debug_texture);
  if (d->linear_clamp_sampler.handle) {
    dev->destroy_sampler(d->linear_clamp_sampler);
    d->linear_clamp_sampler = {};
  }
  // IS-FAST noise
  dv(d->isfast_noise_srv);
  dr(d->isfast_noise_texture);
  if (d->isfast_sampler.handle) {
    dev->destroy_sampler(d->isfast_sampler);
    d->isfast_sampler = {};
  }
  d->isfast_texture_loaded = false;
  d->isfast_texture_attempted = false;
  // Do NOT clear captured_depth_srv / captured_scene_cbv �
  // those reference game-owned resources that survive recreation.
  d->resources_created = false;
}

// ----------- Contact Shadows (shadows/*.cs_5_0.hlsl) -----------

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

  if (!mk(&d->contact_shadow_texture, &d->contact_shadow_srv, &d->contact_shadow_uav)
      || !mk(&d->contact_denoise_texture, &d->contact_denoise_srv, &d->contact_denoise_uav)
      || !mk(&d->contact_local_texture, &d->contact_local_srv, &d->contact_local_uav)) {
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

  dv(d->contact_shadow_srv);
  dv(d->contact_shadow_uav);
  dr(d->contact_shadow_texture);
  dv(d->contact_denoise_srv);
  dv(d->contact_denoise_uav);
  dr(d->contact_denoise_texture);
  dv(d->contact_local_srv);
  dv(d->contact_local_uav);
  dr(d->contact_local_texture);
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
      __contact_shadows, __contact_denoise, __contact_local, __contact_local_denoise};

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
      DR sampler_r = {0, 0, 0, 1, DS::all_compute, 1, DT::sampler};
      DR cbv_r = {0, 0, 0, 1, DS::all_compute, 1, DT::constant_buffer};
      DR srv_r = {0, 0, 0, kShadowsSrvPerPass[pass], DS::all_compute, 1, DT::texture_shader_resource_view};
      DR uav_r = {0, 0, 0, kShadowsUavPerPass[pass], DS::all_compute, 1, DT::texture_unordered_access_view};
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
      param_sampler.descriptor_table.count = 1;
      param_sampler.descriptor_table.ranges = &sampler_r;
      param_cbv.type = reshade::api::pipeline_layout_param_type::descriptor_table;
      param_cbv.descriptor_table.count = 1;
      param_cbv.descriptor_table.ranges = &cbv_r;
      param_srv.type = reshade::api::pipeline_layout_param_type::descriptor_table;
      param_srv.descriptor_table.count = 1;
      param_srv.descriptor_table.ranges = &srv_r;
      param_uav.type = reshade::api::pipeline_layout_param_type::descriptor_table;
      param_uav.descriptor_table.count = 1;
      param_uav.descriptor_table.ranges = &uav_r;
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
      sd.code = code.data();
      sd.code_size = code.size();
      sd.entry_point = "main";
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

// Runs the sun contact pass and its denoise once a frame, and the local contact pass once a frame from the main lighting hook.
static bool RunShadows(reshade::api::command_list* cl, DeviceData* d, int fromHook) {
  if (!cl || !d) return false;
  const bool sun_due = shader_injection.cs_contact_enabled > 0.5f && d->shadows_ran_frame != d->frame_index;
  const bool local_due = fromHook == 1 && LocalContactComputeRunning() && d->shadows_local_ran_frame != d->frame_index;
  if (!sun_due && !local_due) return true;  // nothing left to dispatch this frame
  auto* dev = cl->get_device();
  if (!dev) return false;

  // Name whichever condition is currently blocking, at most once a frame. Every
  // early return here used to be silent, which made "the pass never ran" and "the
  // pass ran and correctly found nothing" indistinguishable from the outside.
  auto blocked = [&](const char* why) {
    if (d->shadows_log_frame == d->frame_index) return;
    d->shadows_log_frame = d->frame_index;
    reshade::log::message(reshade::log::level::warning,
                          (std::string("[Shadows] blocked: ") + why
                           + " (depth=" + (d->captured_depth_srv.handle ? "ok" : "null")
                           + " mrt=" + (d->captured_mrt_normal_srv.handle ? "ok" : "null")
                           + " cbv=" + (d->captured_scene_cbv_view.handle ? "ok" : "null")
                           + " live=" + std::to_string(d->captured_depth_live.load() && d->captured_mrt_live.load() && d->captured_cbv_live.load())
                           + " grid=" + std::to_string(d->shadows_w) + "x" + std::to_string(d->shadows_h)
                           + " ready=" + (d->shadows_resources_ready ? "1" : "0"))
                              .c_str());
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

  // The IS-FAST volume is an input to the contact march.
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
                                        ? static_cast<float>(d->frame_index % 32u)
                                        : -1.f;

  const auto CS = reshade::api::shader_stage::all_compute;
  const auto AC = reshade::api::pipeline_stage::all_compute;
  const auto UA = reshade::api::resource_usage::unordered_access;
  const auto SR = reshade::api::resource_usage::shader_resource;

  auto run_pass = [&](uint32_t pass, reshade::api::resource_view* srvs,
                      uint32_t srv_count, reshade::api::resource_view uav,
                      reshade::api::resource out_tex) {
    cl->bind_pipeline(AC, d->shadows_pipelines[pass]);
    reshade::api::descriptor_table_update u[kGtvbaoDescriptorTableParamCount] = {
        {{}, 0, 0, 1, reshade::api::descriptor_type::sampler, &d->shadows_point_clamp_sampler},
        {{}, 0, 0, 1, reshade::api::descriptor_type::constant_buffer, &d->captured_scene_cbv_view},
        {{}, 0, 0, srv_count, reshade::api::descriptor_type::texture_shader_resource_view, srvs},
        {{}, 0, 0, 1, reshade::api::descriptor_type::texture_unordered_access_view, &uav},
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

  // Separable denoise of one target: horizontal into contact_denoise_texture, vertical back into the target.
  auto run_denoise = [&](uint32_t pass, float passes_setting, reshade::api::resource_view target_srv,
                         reshade::api::resource_view target_uav, reshade::api::resource target_texture) {
    reshade::api::resource_view horizontal_srvs[3] = {d->captured_depth_srv, d->captured_mrt_normal_srv, target_srv};
    reshade::api::resource_view vertical_srvs[3] = {d->captured_depth_srv, d->captured_mrt_normal_srv, d->contact_denoise_srv};
    const int passes = std::clamp(static_cast<int>(passes_setting + 0.5f), 1, 4);
    for (int denoise_pass = 0; denoise_pass < passes; ++denoise_pass) {
      shader_injection.cs_denoise_axis = 0.f;
      run_pass(pass, horizontal_srvs, 3, d->contact_denoise_uav, d->contact_denoise_texture);
      shader_injection.cs_denoise_axis = 1.f;
      run_pass(pass, vertical_srvs, 3, target_uav, target_texture);
    }
  };

  if (sun_due) {
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
    if (shader_injection.cs_contact_denoise_enabled > 0.5f) {
      run_denoise(kShadowsPassDenoise, shader_injection.cs_contact_denoise_passes, d->contact_shadow_srv,
                  d->contact_shadow_uav, d->contact_shadow_texture);
    }
    d->shadows_ran_frame = d->frame_index;
    if (!d->shadows_logged_first_dispatch) {
      d->shadows_logged_first_dispatch = true;
      CSLog("shadows", "first dispatch ok at " + std::to_string(w) + "x" + std::to_string(h)
                           + " (isfast=" + (isfast_ready ? "ready" : "missing") + ")");
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
  }
  if (local_due) {
    // The pass's game inputs come from the VBGI capture slots: the material index (1 = mrtTexture2 on Sora,
    // 0 = mrtTexture1 on Kai), 2 deferredParams, 4 lamps, 5 light cells. Each must be live and from this frame.
    const uint32_t material_k = IsKai() ? 0u : 1u;
    const std::array<uint32_t, 4> local_inputs = {material_k, 2u, 4u, 5u};
    bool inputs_ready = true;
    for (const uint32_t k : local_inputs) {
      inputs_ready = inputs_ready && d->vbgi_in_srv[k].handle != 0u && d->vbgi_in_live[k].load()
                     && d->vbgi_in_frame[k] == d->frame_index;
    }
    if (inputs_ready) {
      reshade::api::resource_view local_srvs[7] = {
          d->captured_depth_srv, d->captured_mrt_normal_srv,
          isfast_ready ? d->isfast_noise_srv : d->fallback_srv,
          d->vbgi_in_srv[4], d->vbgi_in_srv[5], d->vbgi_in_srv[material_k], d->vbgi_in_srv[2]};
      run_pass(kShadowsPassLocal, local_srvs, 7, d->contact_local_uav, d->contact_local_texture);
      if (shader_injection.cs_contact_local_denoise_enabled > 0.5f) {
        run_denoise(kShadowsPassLocalDenoise, shader_injection.cs_contact_local_denoise_passes, d->contact_local_srv,
                    d->contact_local_uav, d->contact_local_texture);
      }
      d->shadows_local_ran_frame = d->frame_index;
      d->shadows_logged_local_inputs_missing = false;
      if (!d->shadows_logged_first_local_dispatch) {
        d->shadows_logged_first_local_dispatch = true;
        const uint64_t light_bytes = dev->get_resource_desc(dev->get_resource_from_view(d->vbgi_in_srv[4])).buffer.size;
        const uint64_t cell_bytes = dev->get_resource_desc(dev->get_resource_from_view(d->vbgi_in_srv[5])).buffer.size;
        CSLog("shadows", "local first dispatch ok at " + std::to_string(w) + "x" + std::to_string(h)
                             + " (lamps=" + std::to_string(light_bytes / falcom_world::rtgi::kLightParamBytes)
                             + " cells=" + std::to_string(cell_bytes / falcom_world::rtgi::kLightIndexDataBytes)
                             + " material_slot=" + std::to_string(material_k)
                             + " isfast=" + (isfast_ready ? "ready" : "missing") + ")");
      }
    } else if (!d->shadows_logged_local_inputs_missing) {
      d->shadows_logged_local_inputs_missing = true;
      std::string why = "[Shadows] local blocked: lighting inputs not captured this frame (";
      for (const uint32_t k : local_inputs) {
        why += " slot" + std::to_string(k) + "="
               + (d->vbgi_in_srv[k].handle == 0u          ? "null"
                  : !d->vbgi_in_live[k].load()            ? "dead"
                  : d->vbgi_in_frame[k] != d->frame_index ? "stale"
                                                          : "ok");
      }
      why += " )";
      reshade::log::message(reshade::log::level::warning, why.c_str());
      CSLog("shadows", "local blocked: lighting inputs not captured this frame");
    }
  }
  return true;
}

// ----------- Custom SSR (Sora 2nd) � Phase 1: Hi-Z pyramid -----------
// ----------- Dynamic Cubemaps (Sora 2nd) � standalone t17 replacement -----------

static uint32_t DynCubeResolveSize(float v) {
  switch ((int)v) {
    case 0:  return 128u;
    case 1:  return 256u;
    case 2:  return 512u;
    case 3:  return 768u;
    case 4:  return 1024u;
    case 5:  return 1536u;
    default: return 2048u;
  }
}

static void DestroyDynCubeResources(reshade::api::device* dev, DeviceData* d) {
  if (!dev || !d) return;
  auto dv = [&](reshade::api::resource_view& v) { if (v.handle) { dev->destroy_resource_view(v); v = {}; } };
  auto dr = [&](reshade::api::resource& r) { if (r.handle) { dev->destroy_resource(r); r = {}; } };
  auto dp = [&](reshade::api::pipeline& p) { if (p.handle) { dev->destroy_pipeline(p); p = {}; } };
  auto dl = [&](reshade::api::pipeline_layout& l) { if (l.handle) { dev->destroy_pipeline_layout(l); l = {}; } };
  if (d->dyncube_sampler.handle) {
    dev->destroy_sampler(d->dyncube_sampler);
    d->dyncube_sampler = {};
  }
  for (auto& set : d->dyncube_hist) {
    dv(set.color_cube_srv);
    dv(set.color_arr_srv);
    dv(set.color_uav);
    dr(set.color);
    dv(set.pos_arr_srv);
    dv(set.pos_cube_srv);
    dv(set.pos_uav);
    dr(set.pos);
    dv(set.contrib_arr_srv);
    dv(set.contrib_uav);
    dr(set.contrib);
  }
  // Aliases are copies of the above handles � zero them (never double-destroy).
  d->dyncube_srv = {};
  d->dyncube_uav = {};
  d->dyncube_texture = {};
  for (uint32_t i = 0; i < 2; ++i) {
    dv(d->dyncube_cam_srv[i]);
    dv(d->dyncube_cam_uav[i]);
    dr(d->dyncube_cam[i]);
  }
  // Character mask
  dv(d->dyncube_charmask_srv);
  dv(d->dyncube_charmask_arr_srv);
  dr(d->dyncube_charmask);
  if (d->dyncube_charmask_uav.handle) {
    dev->destroy_resource_view(d->dyncube_charmask_uav);
    d->dyncube_charmask_uav = {};
  }
  // World-fixed parallax bounds (persistent; re-initialized on next create)
  dv(d->dyncube_worldbox_bounds_srv);
  dv(d->dyncube_worldbox_bounds_uav);
  dr(d->dyncube_worldbox_bounds);
  dv(d->dyncube_faceextents_uav);
  dr(d->dyncube_faceextents);
  dr(d->dyncube_faceExtStaging);
  dv(d->dyncube_worldbox_scratch_srv);
  dv(d->dyncube_worldbox_scratch_uav);
  dr(d->dyncube_worldbox_scratch);
  d->dyncube_worldbox_scratch_groups = 0u;
  dp(d->dyncube_worldbox_pipeline);
  dl(d->dyncube_worldbox_layout);
  for (auto& t : d->dyncube_worldbox_tables) {
    if (t.handle) {
      dev->free_descriptor_table(t);
      t = {};
    }
  }
  d->dyncube_worldbox_layout_version = 0u;
  d->dyncube_worldbox_reset_pending = true;
  // Phase 3 GGX
  if (d->dyncube_linear_sampler.handle) {
    dev->destroy_sampler(d->dyncube_linear_sampler);
    d->dyncube_linear_sampler = {};
  }
  dv(d->dyncube_ggx_in_cube_srv);
  dr(d->dyncube_ggx_in);
  for (uint32_t i = 0; i < 2; ++i) {
    dv(d->dyncube_ggx_out_cube_srv[i]);
    dr(d->dyncube_ggx_out[i]);
    for (auto& u : d->dyncube_ggx_out_mip_uav[i]) {
      if (u.handle) {
        dev->destroy_resource_view(u);
        u = {};
      }
    }
  }
  d->dyncube_ggx_valid = false;
  // Global-push variant cube (derived): destroy with the rest, rebuild on demand.
  dv(d->dyncube_variant_cube_srv);
  dr(d->dyncube_variant);
  for (auto& u : d->dyncube_variant_mip_uav) {
    if (u.handle) {
      dev->destroy_resource_view(u);
      u = {};
    }
  }
  dp(d->dyncube_variant_pipeline);
  dl(d->dyncube_variant_layout);
  for (auto& t : d->dyncube_variant_tables) {
    if (t.handle) {
      dev->free_descriptor_table(t);
      t = {};
    }
  }
  d->dyncube_variant_valid = false;
  // Dedicated solid-color debug cube
  dv(d->dyncube_solid_cube_srv);
  dr(d->dyncube_solid_cube);
  if (d->dyncube_solid_cube_uav.handle) {
    dev->destroy_resource_view(d->dyncube_solid_cube_uav);
    d->dyncube_solid_cube_uav = {};
  }
  // Simple SSR
  dv(d->dyncube_ssr_raw_srv);
  dr(d->dyncube_ssr_raw);
  if (d->dyncube_ssr_raw_uav.handle) {
    dev->destroy_resource_view(d->dyncube_ssr_raw_uav);
    d->dyncube_ssr_raw_uav = {};
  }
  dv(d->dyncube_ssr_blur_h_srv);
  dr(d->dyncube_ssr_blur_h);
  if (d->dyncube_ssr_blur_h_uav.handle) {
    dev->destroy_resource_view(d->dyncube_ssr_blur_h_uav);
    d->dyncube_ssr_blur_h_uav = {};
  }
  dv(d->dyncube_ssr_blur_srv);
  dr(d->dyncube_ssr_blur);
  if (d->dyncube_ssr_blur_uav.handle) {
    dev->destroy_resource_view(d->dyncube_ssr_blur_uav);
    d->dyncube_ssr_blur_uav = {};
  }
  dv(d->dyncube_ssr_hitmask_srv);
  dr(d->dyncube_ssr_hitmask);
  if (d->dyncube_ssr_hitmask_uav.handle) {
    dev->destroy_resource_view(d->dyncube_ssr_hitmask_uav);
    d->dyncube_ssr_hitmask_uav = {};
  }
  dp(d->dyncube_ssr_pipeline);
  dl(d->dyncube_ssr_layout);
  for (auto& t : d->dyncube_ssr_tables) {
    if (t.handle) {
      dev->free_descriptor_table(t);
      t = {};
    }
  }
  d->dyncube_ssr_layout_version = 0u;
  dp(d->dyncube_ssr_blur_pipeline);
  dl(d->dyncube_ssr_blur_layout);
  for (auto& t : d->dyncube_ssr_blur_tables) {
    if (t.handle) {
      dev->free_descriptor_table(t);
      t = {};
    }
  }
  d->dyncube_ssr_blur_layout_version = 0u;
  dp(d->dyncube_ggx_pipeline);
  dl(d->dyncube_ggx_layout);
  for (auto& t : d->dyncube_ggx_tables) {
    if (t.handle) {
      dev->free_descriptor_table(t);
      t = {};
    }
  }
  d->dyncube_hist_cur = 0;
  d->dyncube_needs_reset = true;
  d->dyncube_loadingWipeDone = false;
  d->dyncube_loadingWipePending = false;
  dr(d->dyncube_validStaging);
  falcom_world::bvh::DestroyDeformQueryD3D11(d->dyncube_valid_event);
  d->dyncube_valid_event = 0u;
  d->dyncube_valid_event_armed = false;
  d->dyncube_valid_wait_ticks = 0u;
  falcom_world::bvh::DestroyGpuTimer(&d->dyncube_gpu_capture_timer);
  falcom_world::bvh::DestroyGpuTimer(&d->dyncube_gpu_filter_timer);
  falcom_world::bvh::DestroyGpuTimer(&d->dyncube_gpu_variant_timer);
  falcom_world::bvh::DestroyGpuTimer(&d->dyncube_gpu_ssr_timer);
  falcom_world::bvh::DestroyGpuTimer(&d->dyncube_gpu_box_timer);
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
  dp(d->dyncube_capture_pipeline);
  dp(d->dyncube_solid_pipeline);
  dl(d->dyncube_capture_layout);
  dl(d->dyncube_solid_layout);
  d->dyncube_capture_layout_version = 0u;
  for (auto& t : d->dyncube_capture_tables) {
    if (t.handle) {
      dev->free_descriptor_table(t);
      t = {};
    }
  }
  for (auto& t : d->dyncube_solid_tables) {
    if (t.handle) {
      dev->free_descriptor_table(t);
      t = {};
    }
  }
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
  bytes += (d->dyncube_worldbox_scratch_groups + d->dyncube_worldbox_scratch_groups / kDynCubeWorldBoxChunkPartials) * 2u * 16u;
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
  // One array UAV per mip: the variant writes every mip directly (no GenerateMips).
  for (uint32_t m = 0; m < mips && m < 8u; ++m) {
    dev->create_resource_view(d->dyncube_variant, reshade::api::resource_usage::unordered_access,
                              reshade::api::resource_view_desc(reshade::api::resource_view_type::texture_2d_array,
                                                               reshade::api::format::r16g16b16a16_float, m, 1, 0, 6),
                              &d->dyncube_variant_mip_uav[m]);
  }
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
        3.402823466e+38f,
        3.402823466e+38f,
        3.402823466e+38f,
        0.f,
        -3.402823466e+38f,
        -3.402823466e+38f,
        -3.402823466e+38f,
        0.f,
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
    const uint64_t scratchEntries = scratchGroups + scratchGroups / kDynCubeWorldBoxChunkPartials;  // partials + chunk results
    reshade::api::resource_desc rs = {};
    rs.type = reshade::api::resource_type::buffer;
    rs.buffer.size = scratchEntries * 2u * 16u;
    rs.buffer.stride = 16;
    rs.heap = reshade::api::memory_heap::gpu_only;
    rs.usage = reshade::api::resource_usage::shader_resource | reshade::api::resource_usage::unordered_access;
    if (!dev->create_resource(rs, nullptr, reshade::api::resource_usage::shader_resource, &d->dyncube_worldbox_scratch)) {
      if (should_log()) reshade::log::message(reshade::log::level::error, "[DynCube] Failed to create world-box scratch");
      DestroyDynCubeResources(dev, d);
      return false;
    }
    const uint64_t scratchElements = scratchEntries * 2u;
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
    d->dyncube_valid_event = falcom_world::bvh::CreateDeformEventD3D11(dev);
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
    rdo.flags = reshade::api::resource_flags::cube_compatible | reshade::api::resource_flags::generate_mipmaps;  // HW A/B path GenerateMips
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
                          (std::string("[DynCube] Resources created: ") + std::to_string(size) + "x" + std::to_string(size) + "x6, mips=" + std::to_string(d->dyncube_mip_count) + ", ~" + std::to_string((long long)(DynCubeEstimatedBytes(d) / (1024ull * 1024ull))) + " MB").c_str());
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
    sd.code = bc.data();
    sd.code_size = bc.size();
    sd.entry_point = "main";
    reshade::api::pipeline_subobject so = {reshade::api::pipeline_subobject_type::compute_shader, 1, &sd};
    return dev->create_pipeline(lo, 1, &so, out);
  };
  // Phase 1+2 + folded world-box pass 0: 8 SRVs (depth, color, prevColor, prevPos, prevContrib, camPrev, mrt0, vanilla),
  // 5 texture UAVs (curColor, curPos, curContrib, camCur, charmask) + 1 buffer UAV (bounds scratch), 14 push floats
  auto make_capture_layout = [&](reshade::api::pipeline_layout* out) -> bool {
    if (out->handle != 0u && d->dyncube_capture_layout_version == kDynCubeCaptureLayoutVersion) return true;
    if (out->handle != 0u) {
      // Stale layout shape: drop layout, tables, and pipeline so they rebuild below.
      for (auto& t : d->dyncube_capture_tables) {
        if (t.handle) {
          dev->free_descriptor_table(t);
          t = {};
        }
      }
      dev->destroy_pipeline_layout(*out);
      *out = {};
      if (d->dyncube_capture_pipeline.handle) {
        dev->destroy_pipeline(d->dyncube_capture_pipeline);
        d->dyncube_capture_pipeline = {};
      }
    }
    DR sampler_r = {0, 0, 0, 1, DS::all_compute, 1, DT::sampler};
    DR cbv_r = {0, 0, 0, 1, DS::all_compute, 1, DT::constant_buffer};                   // b0 cb_scene only
    DR srv_r = {0, 0, 0, 8, DS::all_compute, 1, DT::texture_shader_resource_view};      // t0..t7 (depth, color, prevColor, prevPos, prevContrib, camPrev, mrt0, vanilla)
    DR uav_r = {0, 0, 0, 5, DS::all_compute, 1, DT::texture_unordered_access_view};     // u0..u4 (curColor, curPos, curContrib, camCur, charmask)
    DR uav_buf_r = {0, 5, 0, 1, DS::all_compute, 1, DT::buffer_unordered_access_view};  // u5 (world-box per-group partials)
    reshade::api::constant_range push_range = {};
    push_range.binding = 0;
    push_range.dx_register_index = 13;
    push_range.dx_register_space = 0;
    push_range.count = 14;  // boost, blend, posThreshold, posScale, reset, characterCapture, charMaskAvailable, charComp, charShift, soften, charInvert, sparkleReject, contribThreshold, writeCharmask
    push_range.visibility = DS::all_compute;
    P p0, p1, p2, p3, p4, pPush;
    p0.type = reshade::api::pipeline_layout_param_type::descriptor_table;
    p0.descriptor_table.count = 1;
    p0.descriptor_table.ranges = &sampler_r;
    p1.type = reshade::api::pipeline_layout_param_type::descriptor_table;
    p1.descriptor_table.count = 1;
    p1.descriptor_table.ranges = &cbv_r;
    p2.type = reshade::api::pipeline_layout_param_type::descriptor_table;
    p2.descriptor_table.count = 1;
    p2.descriptor_table.ranges = &srv_r;
    p3.type = reshade::api::pipeline_layout_param_type::descriptor_table;
    p3.descriptor_table.count = 1;
    p3.descriptor_table.ranges = &uav_r;
    p4.type = reshade::api::pipeline_layout_param_type::descriptor_table;
    p4.descriptor_table.count = 1;
    p4.descriptor_table.ranges = &uav_buf_r;
    pPush.type = reshade::api::pipeline_layout_param_type::push_constants;
    pPush.push_constants = push_range;
    P params[6] = {p0, p1, p2, p3, p4, pPush};
    if (!dev->create_pipeline_layout(6, params, out)) return false;
    d->dyncube_capture_layout_version = kDynCubeCaptureLayoutVersion;
    return true;
  };
  auto make_solid_layout = [&](reshade::api::pipeline_layout* out) -> bool {
    if (out->handle != 0u) return true;
    DR uav_r = {0, 0, 0, 1, DS::all_compute, 1, DT::texture_unordered_access_view};
    P p0;
    p0.type = reshade::api::pipeline_layout_param_type::descriptor_table;
    p0.descriptor_table.count = 1;
    p0.descriptor_table.ranges = &uav_r;
    return dev->create_pipeline_layout(1, &p0, out);
  };

  if (!make_capture_layout(&d->dyncube_capture_layout)) return false;
  if (!make_solid_layout(&d->dyncube_solid_layout)) return false;
  // Ensure tables
  auto ensure = [&](reshade::api::pipeline_layout lo, std::span<reshade::api::descriptor_table> tbl, uint32_t count) -> bool {
    for (uint32_t i = 0; i < count; ++i) {
      if (tbl[i].handle != 0u) continue;
      if (!dev->allocate_descriptor_table(lo, i, &tbl[i])) return false;
    }
    return true;
  };
  if (!ensure(d->dyncube_capture_layout, d->dyncube_capture_tables, 5)) return false;
  if (!ensure(d->dyncube_solid_layout, d->dyncube_solid_tables, 1)) return false;

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
    DR sampler_r = {0, 0, 0, 1, DS::all_compute, 1, DT::sampler};
    DR srv_r = {0, 0, 0, 1, DS::all_compute, 1, DT::texture_shader_resource_view};   // t0 GGX input cube
    DR uav_r = {0, 0, 0, 1, DS::all_compute, 1, DT::texture_unordered_access_view};  // u0 filtered mip UAV
    reshade::api::constant_range push_range = {};
    push_range.binding = 0;
    push_range.dx_register_index = 13;
    push_range.dx_register_space = 0;
    push_range.count = 1;  // roughness
    push_range.visibility = DS::all_compute;
    P p0, p1, p2, pPush;
    p0.type = reshade::api::pipeline_layout_param_type::descriptor_table;
    p0.descriptor_table.count = 1;
    p0.descriptor_table.ranges = &sampler_r;
    p1.type = reshade::api::pipeline_layout_param_type::descriptor_table;
    p1.descriptor_table.count = 1;
    p1.descriptor_table.ranges = &srv_r;
    p2.type = reshade::api::pipeline_layout_param_type::descriptor_table;
    p2.descriptor_table.count = 1;
    p2.descriptor_table.ranges = &uav_r;
    pPush.type = reshade::api::pipeline_layout_param_type::push_constants;
    pPush.push_constants = push_range;
    P params[4] = {p0, p1, p2, pPush};
    return dev->create_pipeline_layout(4, params, out);
  };
  if (!make_ggx_layout(&d->dyncube_ggx_layout)) return false;
  if (!ensure(d->dyncube_ggx_layout, d->dyncube_ggx_tables, 3)) return false;
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
    DR sampler_r = {0, 0, 0, 1, DS::all_compute, 1, DT::sampler};                    // s0 trilinear clamp
    DR srv_r = {0, 0, 0, 1, DS::all_compute, 1, DT::texture_shader_resource_view};   // t0 source cube
    DR uav_r = {0, 0, 0, 1, DS::all_compute, 1, DT::texture_unordered_access_view};  // u0 variant mip0 array
    reshade::api::constant_range push_range = {};
    push_range.binding = 0;
    push_range.dx_register_index = 13;
    push_range.dx_register_space = 0;
    push_range.count = 2;  // srcMip, strength
    push_range.visibility = DS::all_compute;
    P p0, p1, p2, pPush;
    p0.type = reshade::api::pipeline_layout_param_type::descriptor_table;
    p0.descriptor_table.count = 1;
    p0.descriptor_table.ranges = &sampler_r;
    p1.type = reshade::api::pipeline_layout_param_type::descriptor_table;
    p1.descriptor_table.count = 1;
    p1.descriptor_table.ranges = &srv_r;
    p2.type = reshade::api::pipeline_layout_param_type::descriptor_table;
    p2.descriptor_table.count = 1;
    p2.descriptor_table.ranges = &uav_r;
    pPush.type = reshade::api::pipeline_layout_param_type::push_constants;
    pPush.push_constants = push_range;
    P params[4] = {p0, p1, p2, pPush};
    return dev->create_pipeline_layout(4, params, out);
  };
  if (!make_variant_layout(&d->dyncube_variant_layout)) return false;
  if (!ensure(d->dyncube_variant_layout, d->dyncube_variant_tables, 3)) return false;
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
      for (auto& t : d->dyncube_ssr_tables) {
        if (t.handle) {
          dev->free_descriptor_table(t);
          t = {};
        }
      }
      dev->destroy_pipeline_layout(*out);
      *out = {};
      if (d->dyncube_ssr_pipeline.handle) {
        dev->destroy_pipeline(d->dyncube_ssr_pipeline);
        d->dyncube_ssr_pipeline = {};
      }
    }
    DR sampler_r = {0, 0, 0, 1, DS::all_compute, 1, DT::sampler};
    DR cbv_r = {0, 0, 0, 1, DS::all_compute, 1, DT::constant_buffer};                // b0 cb_scene
    DR srv_r = {0, 0, 0, 4, DS::all_compute, 1, DT::texture_shader_resource_view};   // t0 color, t1 depth, t2 mrt_normal, t3 IS-FAST noise
    DR uav_r = {0, 0, 0, 2, DS::all_compute, 1, DT::texture_unordered_access_view};  // u0 ssr_result, u1 per-tile hit mask
    reshade::api::constant_range push_range = {};
    push_range.binding = 0;
    push_range.dx_register_index = 13;
    push_range.dx_register_space = 0;
    push_range.count = 18;  // sampleCount, maxDist, thickness, distanceFade, edgeFade, grazingFade, charOccStrength, charOccUpness, isfastEnabled, isfastBound, isfastFrame, isfastStrength, isfastSpatial, isfastTemporal, isfastSeed, charComp, charShift, charInvert
    push_range.visibility = DS::all_compute;
    P p0, p1, p2, p3, pPush;
    p0.type = reshade::api::pipeline_layout_param_type::descriptor_table;
    p0.descriptor_table.count = 1;
    p0.descriptor_table.ranges = &sampler_r;
    p1.type = reshade::api::pipeline_layout_param_type::descriptor_table;
    p1.descriptor_table.count = 1;
    p1.descriptor_table.ranges = &cbv_r;
    p2.type = reshade::api::pipeline_layout_param_type::descriptor_table;
    p2.descriptor_table.count = 1;
    p2.descriptor_table.ranges = &srv_r;
    p3.type = reshade::api::pipeline_layout_param_type::descriptor_table;
    p3.descriptor_table.count = 1;
    p3.descriptor_table.ranges = &uav_r;
    pPush.type = reshade::api::pipeline_layout_param_type::push_constants;
    pPush.push_constants = push_range;
    P params[5] = {p0, p1, p2, p3, pPush};
    if (!dev->create_pipeline_layout(5, params, out)) return false;
    d->dyncube_ssr_layout_version = kDynCubeSSRLayoutVersion;
    return true;
  };
  if (!make_ssr_layout(&d->dyncube_ssr_layout)) return false;
  if (!ensure(d->dyncube_ssr_layout, d->dyncube_ssr_tables, 4)) return false;
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
      for (auto& t : d->dyncube_ssr_blur_tables) {
        if (t.handle) {
          dev->free_descriptor_table(t);
          t = {};
        }
      }
      dev->destroy_pipeline_layout(*out);
      *out = {};
      if (d->dyncube_ssr_blur_pipeline.handle) {
        dev->destroy_pipeline(d->dyncube_ssr_blur_pipeline);
        d->dyncube_ssr_blur_pipeline = {};
      }
    }
    DR sampler_r = {0, 0, 0, 1, DS::all_compute, 1, DT::sampler};
    DR srv_r = {0, 0, 0, 3, DS::all_compute, 1, DT::texture_shader_resource_view};   // t0 raw/blur_h, t1 captured scene depth, t2 per-tile hit mask
    DR uav_r = {0, 0, 0, 1, DS::all_compute, 1, DT::texture_unordered_access_view};  // u0 blur_h/blur
    DR cbv_r = {0, 0, 0, 1, DS::all_compute, 1, DT::constant_buffer};                // b0 cb_scene (proj for depth unpack)
    reshade::api::constant_range push_range = {};
    push_range.binding = 0;
    push_range.dx_register_index = 13;
    push_range.dx_register_space = 0;
    push_range.count = 3;  // sigma, horizontal, symmetricWeights
    push_range.visibility = DS::all_compute;
    P p0, p1, p2, p3, pPush;
    p0.type = reshade::api::pipeline_layout_param_type::descriptor_table;
    p0.descriptor_table.count = 1;
    p0.descriptor_table.ranges = &sampler_r;
    p1.type = reshade::api::pipeline_layout_param_type::descriptor_table;
    p1.descriptor_table.count = 1;
    p1.descriptor_table.ranges = &srv_r;
    p2.type = reshade::api::pipeline_layout_param_type::descriptor_table;
    p2.descriptor_table.count = 1;
    p2.descriptor_table.ranges = &uav_r;
    p3.type = reshade::api::pipeline_layout_param_type::descriptor_table;
    p3.descriptor_table.count = 1;
    p3.descriptor_table.ranges = &cbv_r;
    pPush.type = reshade::api::pipeline_layout_param_type::push_constants;
    pPush.push_constants = push_range;
    P params[5] = {p0, p1, p2, p3, pPush};
    if (!dev->create_pipeline_layout(5, params, out)) return false;
    d->dyncube_ssr_blur_layout_version = kDynCubeSSRBlurLayoutVersion;
    return true;
  };
  if (!make_ssr_blur_layout(&d->dyncube_ssr_blur_layout)) return false;
  if (!ensure(d->dyncube_ssr_blur_layout, d->dyncube_ssr_blur_tables, 4)) return false;
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
      for (auto& t : d->dyncube_worldbox_tables) {
        if (t.handle) {
          dev->free_descriptor_table(t);
          t = {};
        }
      }
      dev->destroy_pipeline_layout(*out);
      *out = {};
      if (d->dyncube_worldbox_pipeline.handle) {
        dev->destroy_pipeline(d->dyncube_worldbox_pipeline);
        d->dyncube_worldbox_pipeline = {};
      }
    }
    DR srv_r = {0, 0, 0, 4, DS::all_compute, 1, DT::texture_shader_resource_view};      // t0..t3 (pos, contrib, charmask, camCur)
    DR srv_buf_r = {0, 0, 0, 3, DS::all_compute, 1, DT::buffer_unordered_access_view};  // u0..u2 (scratch, bounds, faceExt)
    reshade::api::constant_range push_range = {};
    push_range.binding = 0;
    push_range.dx_register_index = 13;
    push_range.dx_register_space = 0;
    push_range.count = 5;  // pass, posScale, reset, scratchCount, contribThreshold
    push_range.visibility = DS::all_compute;
    P p0, p1, pPush;
    p0.type = reshade::api::pipeline_layout_param_type::descriptor_table;
    p0.descriptor_table.count = 1;
    p0.descriptor_table.ranges = &srv_r;
    p1.type = reshade::api::pipeline_layout_param_type::descriptor_table;
    p1.descriptor_table.count = 1;
    p1.descriptor_table.ranges = &srv_buf_r;
    pPush.type = reshade::api::pipeline_layout_param_type::push_constants;
    pPush.push_constants = push_range;
    P params[3] = {p0, p1, pPush};
    if (!dev->create_pipeline_layout(3, params, out)) return false;
    d->dyncube_worldbox_layout_version = kDynCubeWorldBoxLayoutVersion;
    return true;
  };
  if (!make_worldbox_layout(&d->dyncube_worldbox_layout)) return false;
  if (!ensure(d->dyncube_worldbox_layout, d->dyncube_worldbox_tables, 2)) return false;
#ifdef __DynCubeBoundsReduceCS_EMBED_FILE
  if (!__DynCubeBoundsReduceCS.empty()) {
    if (!mkcs(__DynCubeBoundsReduceCS, d->dyncube_worldbox_layout, &d->dyncube_worldbox_pipeline)) {
      if (pipelog_should()) reshade::log::message(reshade::log::level::warning, "[DynCube] World-box pipeline create failed");
    }
  }
#endif
  return true;
}

// Release the compute bindings between nested capture and world-box work so resource
// barriers after the reduction do not encounter its SRV/UAV views still bound.
static void UnbindDynCubeComputeState(reshade::api::command_list* cl) {
  if (!cl) return;
  falcom_world::rtao::NullComputeSlots(cl, 8u, 6u, 1u);
}

class DynCubeComputeDispatchScope final {
 public:
  explicit DynCubeComputeDispatchScope(reshade::api::command_list* cl) : cmd_list_(cl) {
    if (!cmd_list_) return;
    if (auto* cs = renodx::utils::state::GetCurrentState(cmd_list_)) prev_ = *cs;
  }

  ~DynCubeComputeDispatchScope() {
    if (!cmd_list_) return;

    constexpr uint32_t DYNCUBE_COMPUTE_SRV_COUNT = 8u;
    constexpr uint32_t DYNCUBE_COMPUTE_TEXTURE_UAV_COUNT = 6u;
    constexpr uint32_t DYNCUBE_COMPUTE_BUFFER_UAV_COUNT = 6u;

    const int fix = static_cast<int>(g_dyncube_cs_dispatch_fix);
    if (fix == 0) {
      UnbindDynCubeComputeState(cmd_list_);
      return;
    }

    ComputeDispatchFixMode mode = ComputeDispatchFixMode::OFF;
    switch (fix) {
      case 1: mode = ComputeDispatchFixMode::RESTORE_STATE; break;
      case 2:
      case 3: mode = ComputeDispatchFixMode::NULL_AND_RESTORE; break;
      default:
        UnbindDynCubeComputeState(cmd_list_);
        return;
    }
    ApplyComputeDispatchFix(
        cmd_list_, renodx::utils::state::GetCurrentState(cmd_list_), prev_, mode,
        DYNCUBE_COMPUTE_SRV_COUNT, DYNCUBE_COMPUTE_TEXTURE_UAV_COUNT, DYNCUBE_COMPUTE_BUFFER_UAV_COUNT);
  }

 private:
  reshade::api::command_list* cmd_list_ = nullptr;
  renodx::utils::state::CommandListState prev_ = {};
};

static bool RunDynCubeSolid(reshade::api::command_list* cl, DeviceData* d) {
  // Writes a DEDICATED solid-color cube � never the history/ggx resources.
  if (!cl || !d) return false;
  if (d->frame_index < d->resize_guard_until_frame) return false;
  auto* dev = cl->get_device();
  if (!CreateDynCubeSolidResources(dev, d)) return false;
  if (!CreateDynCubePipelinesIfNeeded(dev, d)) return false;
  if (!d->dyncube_solid_pipeline.handle) return false;

  DynCubeComputeDispatchScope dispatch_state(cl);
  cl->bind_pipeline(reshade::api::pipeline_stage::all_compute, d->dyncube_solid_pipeline);
  auto* tbl = &d->dyncube_solid_tables;
  reshade::api::descriptor_table_update upd = {tbl->at(0), 0, 0, 1, reshade::api::descriptor_type::texture_unordered_access_view, &d->dyncube_solid_cube_uav};
  dev->update_descriptor_tables(1, &upd);
  cl->bind_descriptor_tables(reshade::api::shader_stage::all_compute, d->dyncube_solid_layout, 0, 1, &tbl->at(0));
  uint32_t sz = d->dyncube_size;
  cl->dispatch((sz + 7) / 8, (sz + 7) / 8, 6);
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
// so the staged copy always describes the latest capture (no epochs needed). With a deep
// driver frame queue the copy can still be frames behind the CPU, so on the immediate
// context an armed copy is mapped only once its event reports done (polled without
// flush), for at most DYNCUBE_VALIDITY_MAX_WAIT_TICKS ticks; meanwhile the Filter phase
// waits. Deferred-list ticks, unarmed copies, poll errors and the wait bound use the
// blocking map. On map failure the pending flag is kept (frozen, safe direction) and the
// copy is disarmed so the Filter phase keeps running; a later copy overwrites the slot.
static void ConsumeDynCubeStagedValidity(reshade::api::device* dev, DeviceData* dd, bool on_immediate) {
  if (!dev || !dd || !dd->dyncube_boxCopyPending || !dd->dyncube_validStaging.handle) return;
  if (on_immediate && dd->dyncube_valid_event_armed) {
    if (dd->dyncube_valid_wait_ticks < DYNCUBE_VALIDITY_MAX_WAIT_TICKS
        && falcom_world::bvh::PollDeformEventD3D11(dd->immediate_cmd_list, dd->dyncube_valid_event) == 0) {
      ++dd->dyncube_valid_wait_ticks;
      ++dd->dyncube_valid_poll_misses;
      return;
    }
    if (dd->dyncube_valid_wait_ticks >= DYNCUBE_VALIDITY_MAX_WAIT_TICKS) {
      ++dd->dyncube_valid_timeouts;
      CSLog("dyncube", "validity poll timeout: blocking map fallback", true);
    }
  }
  void* staged = nullptr;
  const auto map_start = std::chrono::steady_clock::now();
  const bool mapped = dev->map_buffer_region(dd->dyncube_validStaging, 0, 32, reshade::api::map_access::read_only, &staged);
  const float map_wait_ms = std::chrono::duration<float, std::milli>(std::chrono::steady_clock::now() - map_start).count();
  dd->dyncube_map_wait_ms_max = std::max(dd->dyncube_map_wait_ms_max, map_wait_ms);
  if (!mapped || staged == nullptr) {
    dd->dyncube_valid_event_armed = false;
    CSLog("dyncube", "validity map FAILED: pending kept, Filter gate released", true);
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
          "[DynCube] WorldBox: min=(" + std::to_string(box[0]) + ", " + std::to_string(box[1]) + ", " + std::to_string(box[2]) +
          ") "
          "max=("
          + std::to_string(box[4]) + ", " + std::to_string(box[5]) + ", " + std::to_string(box[6]) +
          ") "
          "size=("
          + std::to_string(box[4] - box[0]) + ", " + std::to_string(box[5] - box[1]) + ", " + std::to_string(box[6] - box[2]) +
          ") "
          "valid="
          + std::to_string((box[3] > 0.5f) ? 1 : 0) + " hasGeom=" + std::to_string((box[7] > 0.5f) ? 1 : 0);
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
            std::string("[DynCube] WorldBoxFaces: +X=") + extStr(fx[0], 1u) + " -X=" + extStr(fx[4], 2u) + " +Y=" + extStr(fx[1], 4u) + " -Y=" + extStr(fx[5], 8u) + " +Z=" + extStr(fx[2], 16u) + " -Z=" + extStr(fx[6], 32u);
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
  const bool time_capture = DynCubeGpuTimingOn(cl, d);
  if (time_capture) {
    falcom_world::bvh::BeginGpuTimer(dev, cl, &d->dyncube_gpu_capture_timer, true);
  }

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
    for (auto& u : d->dyncube_cam_uav)
      if (u.handle) cl->clear_unordered_access_view_float(u, zero4);
    d->dyncube_needs_reset = false;
  }

  DynCubeComputeDispatchScope dispatch_state(cl);
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
      d->captured_mrt_normal_srv,   // mrtTexture0 for character mask
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
  reshade::api::descriptor_table_update ups[5];
  ups[0] = {tbl->at(0), 0, 0, 1, reshade::api::descriptor_type::sampler, &d->dyncube_sampler};
  ups[1] = {tbl->at(1), 0, 0, 1, reshade::api::descriptor_type::constant_buffer, &d->captured_scene_cbv_view};
  ups[2] = {tbl->at(2), 0, 0, 8, reshade::api::descriptor_type::texture_shader_resource_view, srvs};
  ups[3] = {tbl->at(3), 0, 0, 5, reshade::api::descriptor_type::texture_unordered_access_view, uavs};
  ups[4] = {tbl->at(4), 0, 0, 1, reshade::api::descriptor_type::buffer_unordered_access_view, &d->dyncube_worldbox_scratch_uav};
  dev->update_descriptor_tables(5, ups);
  std::array<reshade::api::descriptor_table, 5> tables = {tbl->at(0), tbl->at(1), tbl->at(2), tbl->at(3), tbl->at(4)};
  cl->bind_descriptor_tables(reshade::api::shader_stage::all_compute, d->dyncube_capture_layout, 0, 5, tables.data());

  // Push constants (b13): boost, blend, posThreshold(world), posScale, reset, characterCapture, charMaskAvailable, charComp, charShift, soften, charInvert, sparkleReject, contribThreshold, writeCharmask
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
    float pc[14] = {
        std::clamp(shader_injection.dynCube_capture_boost, 0.f, 8.f),
        fastForward ? 1.0f : std::clamp(shader_injection.dynCube_history_blend, 0.f, 1.f),
        std::max(0.f, shader_injection.dynCube_history_pos_threshold),
        posScale,
        reset,
        charCapture,
        charMaskAvail,
        // Character-bit location: Kai (mrt.z >> 8) & 1 set; Sora 1st/2nd !(mrt.w & 8) (bit 3 clear).
        IsKai() ? 1.f : 0.f,
        IsKai() ? 8.f : 3.f,
        std::clamp(shader_injection.dynCube_capture_soften, 0.f, 1.f),
        IsKai() ? 0.f : 1.f,
        (shader_injection.dynCube_sparkle_rejection > 0.5f) ? 1.f : 0.f,
        // Folded world-box pass 0: same contrib cutoff the reduce used to take.
        std::clamp(shader_injection.dynCube_worldbox_contrib, 0.f, 1.f),
        // Character mask store is only consumed by Debug View 7.
        (shader_injection.dynCube_debug == 7.f) ? 1.f : 0.f,
    };
    cl->push_constants(reshade::api::shader_stage::all_compute, d->dyncube_capture_layout, 5, 0, 14, pc);
  }

  uint32_t sz = d->dyncube_size;
  cl->dispatch((sz + 7) / 8, (sz + 7) / 8, 6);
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
  if (time_capture) {
    falcom_world::bvh::EndGpuTimer(cl, &d->dyncube_gpu_capture_timer);
  }
  const bool time_box = DynCubeGpuTimingOn(cl, d);
  if (time_box) {
    falcom_world::bvh::BeginGpuTimer(dev, cl, &d->dyncube_gpu_box_timer, true);
  }
  // The capture scope owns cleanup across this nested pass. Restoring capture's UAVs here
  // would rebind them before the character-mask transition below.
  if (!RunDynCubeWorldBox(cl, d, cur)) {
    // Reduction unavailable: fall back to immediate promotion (pre-protection behavior).
    CSLog("dyncube", "worldbox UNAVAILABLE: immediate-promote fallback", true);
    PromoteDynCubeReadSet(d);
  }
  if (time_box) {
    falcom_world::bvh::EndGpuTimer(cl, &d->dyncube_gpu_box_timer);
  }
  cl->barrier(d->dyncube_charmask, reshade::api::resource_usage::shader_resource, reshade::api::resource_usage::unordered_access);

  d->dyncube_solid_written = false;  // capture ran (aliases still track the validated readSet)
  return true;
}

// World-fixed parallax bounds reduction (Sora2nd v1). Reads the just-written
// history set (write-cursor index `set`). Pass 0 (per-group partials) is folded
// into the capture dispatch, which writes the scratch buffer from its in-register
// values; this function runs the single-group expand-only merge into the
// persistent bounds (camera included unfiltered for containment).
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
  // Merge pass: the per-group partials were written by the capture dispatch
  // (folded pass 0); scratch is UAV-written there, so transition it before the
  // merge reads. Bounds/faceExt are SRV-tracked and need their UAV transition.
  {
    const uint32_t chunks = groups / kDynCubeWorldBoxChunkPartials;
    float pc[5] = {2.0f, 0.001f, reset, (float)groups,
                   std::clamp(shader_injection.dynCube_worldbox_contrib, 0.f, 1.f)};
    cl->push_constants(reshade::api::shader_stage::all_compute, d->dyncube_worldbox_layout, 2, 0, 5, pc);
    cl->barrier(d->dyncube_worldbox_scratch, reshade::api::resource_usage::unordered_access, reshade::api::resource_usage::shader_resource);
    cl->barrier(d->dyncube_worldbox_bounds, reshade::api::resource_usage::shader_resource, reshade::api::resource_usage::unordered_access);
    cl->barrier(d->dyncube_faceextents, reshade::api::resource_usage::shader_resource, reshade::api::resource_usage::unordered_access);
    CSLog("dyncube", "worldbox chunk + merge dispatch");
    // Chunk pass, then the single-group merge over the chunk results. D3D11 orders
    // UAV writes between dispatches, so no barrier is needed in between.
    cl->dispatch(chunks, 1, 1);
    pc[0] = 3.0f;
    pc[3] = (float)chunks;
    cl->push_constants(reshade::api::shader_stage::all_compute, d->dyncube_worldbox_layout, 2, 0, 5, pc);
    cl->dispatch(1, 1, 1);
    cl->barrier(d->dyncube_worldbox_bounds, reshade::api::resource_usage::unordered_access, reshade::api::resource_usage::shader_resource);
  }
  // Stage bounds for delayed-validate commit (consumed at the first scheduler entry after
  // the copy completed; the per-frame hasGeom bit lives in staged bounds[1].w, float index 7).
  cl->barrier(d->dyncube_worldbox_bounds, reshade::api::resource_usage::shader_resource, reshade::api::resource_usage::copy_source);
  cl->copy_resource(d->dyncube_worldbox_bounds, d->dyncube_validStaging);
  cl->barrier(d->dyncube_worldbox_bounds, reshade::api::resource_usage::copy_source, reshade::api::resource_usage::shader_resource);
  // Stage per-face extents for the WorldBoxFaces diagnostic log.
  cl->barrier(d->dyncube_faceextents, reshade::api::resource_usage::unordered_access, reshade::api::resource_usage::copy_source);
  cl->copy_resource(d->dyncube_faceextents, d->dyncube_faceExtStaging);
  cl->barrier(d->dyncube_faceextents, reshade::api::resource_usage::copy_source, reshade::api::resource_usage::unordered_access);
  // The event lets the scheduler consume the copies without waiting on the GPU. It is
  // ended only on the immediate context: a deferred list may never execute, which would
  // leave the copy pending forever, so that path keeps the blocking map.
  d->dyncube_valid_event_armed = d->dyncube_valid_event != 0u && cl == d->immediate_cmd_list;
  if (d->dyncube_valid_event_armed) {
    d->dyncube_valid_wait_ticks = 0u;
    falcom_world::bvh::EndDeformQueryD3D11(cl, d->dyncube_valid_event);
  }
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
    reshade::api::subresource_box box = {0, 0, 0, sz, sz, 1};
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
  DynCubeComputeDispatchScope dispatch_state(cl);
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
  if (!d->dyncube_variant.handle || !d->dyncube_variant_cube_srv.handle) return false;
  if (!CreateDynCubePipelinesIfNeeded(cl->get_device(), d)) return false;
  if (!d->dyncube_variant_pipeline.handle) return false;
  // Every mip must have its UAV, so the chain is never left partly rebuilt.
  for (uint32_t m = 0; m < d->dyncube_mip_count && m < 8u; ++m) {
    if (!d->dyncube_variant_mip_uav[m].handle) return false;
  }
  auto* dev = cl->get_device();
  const bool time_variant = DynCubeGpuTimingOn(cl, d);
  if (time_variant) {
    falcom_world::bvh::BeginGpuTimer(dev, cl, &d->dyncube_gpu_variant_timer, true);
  }
  const uint32_t mips = d->dyncube_mip_count;
  const uint32_t sz = d->dyncube_size;
  const float soften = std::clamp(shader_injection.dynCube_capture_soften, 0.f, 1.f);
  const float strength = std::clamp(shader_injection.dynCube_global_strength, 0.f, 1.f);
  const float srcMip = std::min(soften * kVariantMaxBlurMip, (float)std::max(mips, 1u) - 1.f);
  DynCubeComputeDispatchScope dispatch_state(cl);
  cl->bind_pipeline(reshade::api::pipeline_stage::all_compute, d->dyncube_variant_pipeline);
  auto* vt = &d->dyncube_variant_tables;
  reshade::api::descriptor_table_update vu[3] = {
      {vt->at(0), 0, 0, 1, reshade::api::descriptor_type::sampler, &d->dyncube_linear_sampler},
      {vt->at(1), 0, 0, 1, reshade::api::descriptor_type::texture_shader_resource_view, &d->dyncube_ggx_out_cube_srv[d->dyncube_ggx_active]},
      {vt->at(2), 0, 0, 1, reshade::api::descriptor_type::texture_unordered_access_view, &d->dyncube_variant_mip_uav[0]},
  };
  dev->update_descriptor_tables(3, vu);
  std::array<reshade::api::descriptor_table, 3> vtables = {vt->at(0), vt->at(1), vt->at(2)};
  cl->bind_descriptor_tables(reshade::api::shader_stage::all_compute, d->dyncube_variant_layout, 0, 3, vtables.data());
  cl->barrier(d->dyncube_variant, reshade::api::resource_usage::shader_resource, reshade::api::resource_usage::unordered_access);
  // One dispatch per mip, each sampling the source at the blur its mip would have had after
  // a box reduction of mip 0: lod = 0.5*log2(4^srcMip + 4^m - 1), exactly srcMip at mip 0.
  // Mips write different subresources and read only the source cube, so no barrier between them.
  for (uint32_t m = 0; m < mips; ++m) {
    vu[2] = {vt->at(2), 0, 0, 1, reshade::api::descriptor_type::texture_unordered_access_view, &d->dyncube_variant_mip_uav[m]};
    dev->update_descriptor_tables(1, &vu[2]);
    cl->bind_descriptor_tables(reshade::api::shader_stage::all_compute, d->dyncube_variant_layout, 0, 3, vtables.data());
    const float lod = (m == 0u) ? srcMip : 0.5f * std::log2(std::exp2(2.f * srcMip) + std::exp2(2.f * (float)m) - 1.f);
    float pc[2] = {lod, strength};
    cl->push_constants(reshade::api::shader_stage::all_compute, d->dyncube_variant_layout, 3, 0, 2, pc);
    const uint32_t mw = std::max(1u, sz >> m);
    cl->dispatch((mw + 7u) / 8u, (mw + 7u) / 8u, 6);
  }
  cl->barrier(d->dyncube_variant, reshade::api::resource_usage::unordered_access, reshade::api::resource_usage::shader_resource);
  if (time_variant) {
    falcom_world::bvh::EndGpuTimer(cl, &d->dyncube_gpu_variant_timer);
  }
  d->dyncube_variant_valid = true;
  d->dyncube_lastVariantSoften = soften;
  d->dyncube_lastVariantStrength = strength;
  if (shader_injection.dynCube_debug_logging > 0.5f) {
    reshade::log::message(reshade::log::level::info,
                          (std::string("[DynCube] variant built: ") + std::to_string(sz) + "x" + std::to_string(sz) + " srcMip=" + std::to_string(srcMip) + " strength=" + std::to_string(strength)).c_str());
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
  // Per-8x8-tile hit mask (one R32_UINT texel per blur group): 1 = tile holds at
  // least one donor (conf > 0.01). Cleared and rebuilt by the march every frame;
  // the blur uses it to skip tiles whose donor neighborhood is empty.
  if (!d->dyncube_ssr_hitmask.handle) {
    reshade::api::resource_desc md = {};
    md.type = reshade::api::resource_type::texture_2d;
    md.texture = {(w + 7u) / 8u, (h + 7u) / 8u, 1, 1, reshade::api::format::r32_uint, 1};
    md.heap = reshade::api::memory_heap::gpu_only;
    md.usage = reshade::api::resource_usage::shader_resource | reshade::api::resource_usage::unordered_access;
    if (!dev->create_resource(md, nullptr, reshade::api::resource_usage::shader_resource, &d->dyncube_ssr_hitmask)) {
      reshade::log::message(reshade::log::level::error, "[DynCube] Failed to create SSR hit mask");
      return false;
    }
    reshade::api::resource_view_desc mvd(reshade::api::resource_view_type::texture_2d,
                                         reshade::api::format::r32_uint, 0, 1, 0, 1);
    dev->create_resource_view(d->dyncube_ssr_hitmask, reshade::api::resource_usage::shader_resource, mvd, &d->dyncube_ssr_hitmask_srv);
    dev->create_resource_view(d->dyncube_ssr_hitmask, reshade::api::resource_usage::unordered_access, mvd, &d->dyncube_ssr_hitmask_uav);
    if (!d->dyncube_ssr_hitmask_srv.handle || !d->dyncube_ssr_hitmask_uav.handle) {
      reshade::log::message(reshade::log::level::error, "[DynCube] Failed to create SSR hit mask views");
      dev->destroy_resource(d->dyncube_ssr_hitmask);
      d->dyncube_ssr_hitmask = {};
      d->dyncube_ssr_hitmask_srv = {};
      d->dyncube_ssr_hitmask_uav = {};
      return false;
    }
  }
  {
    // Mismatch-only (silent when matching): stale-small targets after a
    // resolution change are a shared-input divergence worth one warning.
    auto rawRes = dev->get_resource_from_view(d->dyncube_ssr_raw_srv);
    auto rawDesc = (rawRes.handle != 0u) ? dev->get_resource_desc(rawRes) : reshade::api::resource_desc{};
    if (rawDesc.texture.width != w || rawDesc.texture.height != h)
      CSLog("dyncube", std::string("ssr targets MISMATCH: color=") + std::to_string(w) + "x" + std::to_string(h) + " raw=" + std::to_string(rawDesc.texture.width) + "x" + std::to_string(rawDesc.texture.height), true);
  }

  const bool time_ssr = DynCubeGpuTimingOn(cl, d);
  if (time_ssr) {
    falcom_world::bvh::BeginGpuTimer(dev, cl, &d->dyncube_gpu_ssr_timer, true);
  }
  // -- March pass ? ssr_raw --
  DynCubeComputeDispatchScope dispatch_state(cl);
  cl->bind_pipeline(reshade::api::pipeline_stage::all_compute, d->dyncube_ssr_pipeline);
  auto* st = &d->dyncube_ssr_tables;
  const reshade::api::resource_view isfastSrv =
      d->isfast_noise_srv.handle ? d->isfast_noise_srv : d->fallback_srv;
  reshade::api::resource_view srvs[4] = {d->captured_color_srv, d->captured_depth_srv, d->captured_mrt_normal_srv, isfastSrv};
  reshade::api::resource_view suavs[2] = {d->dyncube_ssr_raw_uav, d->dyncube_ssr_hitmask_uav};
  reshade::api::descriptor_table_update su[4] = {
      {st->at(0), 0, 0, 1, reshade::api::descriptor_type::sampler, &d->dyncube_sampler},
      {st->at(1), 0, 0, 1, reshade::api::descriptor_type::constant_buffer, &d->captured_scene_cbv_view},
      {st->at(2), 0, 0, 4, reshade::api::descriptor_type::texture_shader_resource_view, srvs},
      {st->at(3), 0, 0, 2, reshade::api::descriptor_type::texture_unordered_access_view, suavs},
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
      // Character-bit location: Kai (mrt.z >> 8) & 1 set; Sora 1st/2nd !(mrt.w & 8) (bit 3 clear).
      IsKai() ? 1.f : 0.f,
      IsKai() ? 8.f : 3.f,
      IsKai() ? 0.f : 1.f,
  };
  cl->push_constants(reshade::api::shader_stage::all_compute, d->dyncube_ssr_layout, 4, 0, 18, pc);
  // Mask is atomic-OR built, so zero it first; the march then flags every tile
  // that receives at least one donor.
  uint32_t maskClear[4] = {0u, 0u, 0u, 0u};
  cl->barrier(d->dyncube_ssr_hitmask, reshade::api::resource_usage::shader_resource, reshade::api::resource_usage::unordered_access);
  cl->clear_unordered_access_view_uint(d->dyncube_ssr_hitmask_uav, maskClear, 0u, nullptr);
  cl->dispatch((w + 7u) / 8u, (h + 7u) / 8u, 1);
  cl->barrier(d->dyncube_ssr_raw, reshade::api::resource_usage::unordered_access, reshade::api::resource_usage::shader_resource);
  cl->barrier(d->dyncube_ssr_hitmask, reshade::api::resource_usage::unordered_access, reshade::api::resource_usage::shader_resource);

  // -- Separable bilateral blur: H (raw ? blur_h), V (blur_h ? blur) --
  // Both passes reference ORIGINAL captured scene depth (not blurred intermediates).
  if (!d->dyncube_ssr_blur_pipeline.handle) {
    if (time_ssr) {
      falcom_world::bvh::EndGpuTimer(cl, &d->dyncube_gpu_ssr_timer);
    }
    return false;
  }
  const float sigma = std::clamp(shader_injection.dynCube_ssr_blur, 0.f, 8.f);
  auto* bt = &d->dyncube_ssr_blur_tables;
  cl->bind_pipeline(reshade::api::pipeline_stage::all_compute, d->dyncube_ssr_blur_pipeline);
  // H pass
  reshade::api::resource_view hsrvs[3] = {d->dyncube_ssr_raw_srv, d->captured_depth_srv, d->dyncube_ssr_hitmask_srv};
  reshade::api::descriptor_table_update bh[4] = {
      {bt->at(0), 0, 0, 1, reshade::api::descriptor_type::sampler, &d->dyncube_sampler},
      {bt->at(1), 0, 0, 3, reshade::api::descriptor_type::texture_shader_resource_view, hsrvs},
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
  reshade::api::resource_view vsrvs[3] = {d->dyncube_ssr_blur_h_srv, d->captured_depth_srv, d->dyncube_ssr_hitmask_srv};
  reshade::api::descriptor_table_update bv[4] = {
      {bt->at(0), 0, 0, 1, reshade::api::descriptor_type::sampler, &d->dyncube_sampler},
      {bt->at(1), 0, 0, 3, reshade::api::descriptor_type::texture_shader_resource_view, vsrvs},
      {bt->at(2), 0, 0, 1, reshade::api::descriptor_type::texture_unordered_access_view, &d->dyncube_ssr_blur_uav},
      {bt->at(3), 0, 0, 1, reshade::api::descriptor_type::constant_buffer, &d->captured_scene_cbv_view},
  };
  dev->update_descriptor_tables(4, bv);
  cl->bind_descriptor_tables(reshade::api::shader_stage::all_compute, d->dyncube_ssr_blur_layout, 0, 4, btables.data());
  float pcV[3] = {sigma, 0.f, (shader_injection.dynCube_ssr_symmetric_weights > 0.5f) ? 1.f : 0.f};
  cl->push_constants(reshade::api::shader_stage::all_compute, d->dyncube_ssr_blur_layout, 4, 0, 3, pcV);
  cl->dispatch((w + 7u) / 8u, (h + 7u) / 8u, 1);
  cl->barrier(d->dyncube_ssr_blur, reshade::api::resource_usage::unordered_access, reshade::api::resource_usage::shader_resource);
  if (time_ssr) {
    falcom_world::bvh::EndGpuTimer(cl, &d->dyncube_gpu_ssr_timer);
  }
  return true;
}

// -- Push constants builder (kai-vanillaplus style) --

struct GtvbaoPassPush {
  bool final_pass = false;
  bool foliage_mask_valid = false;
  int filter_kind = 0;
  int filter_direction = 0;
  int filter_step = 1;
  int filter_taps = 1;
  bool history_valid = false;
  bool vbgi = false;
  int skip_unread = 0;
  uint32_t full_width = 0u;
  uint32_t full_height = 0u;
  uint32_t divisor = 1u;
};

// Push indices that make up the temporal reset snapshot (settings that shape the AO; not debug, filter or per-frame values).
constexpr uint32_t kGtvbaoSnapshotIndex[kGtvbaoSnapshotFloats] = {
    0, 35, 1, 2, 3, 4, 5, 6, 26, 36, 25, 19, 20, 21, 22, 23, 24, 27, 28,
    10, 11, 12, 13, 14, 15, 16, 17, 18, 38, 39, 40, 41, 37};

static std::array<float, kGtvbaoPushConstantFloats> BuildGTVBAOPushConstants(DeviceData* data, const GtvbaoPassPush& pass = {}) {
  std::array<float, kGtvbaoPushConstantFloats> c = {};
  c[0] = shader_injection.gtvbao_quality_level;
  c[1] = std::max(0.001f, shader_injection.gtvbao_radius);
  c[2] = std::clamp(shader_injection.gtvbao_radius_multiplier, 0.3f, 3.f);
  c[3] = std::clamp(shader_injection.gtvbao_final_power, 0.5f, 5.f);
  c[4] = std::clamp(shader_injection.gtvbao_sample_distribution, 1.f, 3.f);
  c[5] = std::clamp(shader_injection.gtvbao_bitmask_thickness, 0.01f, 2.f);
  c[6] = std::clamp(shader_injection.gtvbao_depth_mip_offset, 0.f, 30.f);
  c[7] = (float)((data ? data->frame_index : 0u) % 64u);
  c[8] = pass.vbgi ? (float)VbgiGtvbaoDebugMode() : (float)GtvbaoDebugMode();
  c[9] = pass.final_pass ? 1.f : 0.f;
  // Normal input: use game MRT normals when available, depth fallback otherwise.
  c[10] = g_gtvbao_normal_input_mode;
  c[11] = (data && data->captured_mrt_normal_srv.handle != 0u) ? 1.f : 0.f;
  c[12] = g_gtvbao_normal_influence;
  c[13] = g_gtvbao_normal_depth_blend;
  c[14] = g_gtvbao_normal_sharpness;
  c[15] = g_gtvbao_normal_edge_rejection;
  c[16] = g_gtvbao_normal_z_preservation;
  c[17] = g_gtvbao_normal_detail_response;
  c[18] = g_gtvbao_normal_max_darkening;
  c[19] = g_isfast_enabled;                                   // IS-FAST enable (0/1)
  c[20] = std::clamp(g_isfast_strength, 0.f, 1.f);            // IS-FAST noise strength
  c[21] = (data && data->isfast_texture_loaded) ? 1.f : 0.f;  // IS-FAST texture loaded flag
  c[22] = std::clamp(g_isfast_spatial_scale, 0.25f, 4.f);     // IS-FAST spatial scale
  c[23] = std::clamp(g_isfast_temporal_speed, 0.f, 5.f);      // IS-FAST temporal speed
  c[24] = std::clamp(g_isfast_seed_offset, 0.f, 64.f);        // IS-FAST seed offset
  c[25] = shader_injection.gtvbao_noise_type;                 // 0=IS-FAST, 1=IGN, 2=Hilbert
  c[26] = shader_injection.gtvbao_cosine_mode;
  // -- Foliage / prefilter --
  // -- Foliage exclusion --
  c[27] = shader_injection.gtvbao_exclude_foliage;
  c[28] = std::clamp(shader_injection.gtvbao_foliage_ao_value, 0.f, 1.f);
  c[29] = IsKai() ? 1.f : 0.f;
  // c[30] - foliage mask is only fresh when the pre-pass dispatched this frame.
  c[30] = pass.foliage_mask_valid ? 1.f : 0.f;
  // c[31..34]: the main pass skips the texels the AO upsample does not read (gtvbao_main.cs_5_0.hlsl).
  c[31] = static_cast<float>(pass.skip_unread);
  c[32] = static_cast<float>(pass.full_width);
  c[33] = static_cast<float>(pass.full_height);
  c[34] = static_cast<float>(pass.divisor);
  // �� Low-resolution spatial pipeline (appended; Full path ignores these) ��
  c[35] = std::clamp(std::round(shader_injection.gtvbao_resolution), 0.f, 1.f);
  c[36] = g_gtvbao_hemisphere_weighting;  // hemisphere weighting 0=Cosine, 1=Uniform
  c[37] = pass.vbgi ? std::clamp(g_vbgi_auto_thickness, 0.f, 1.f)
                    : std::clamp(g_gtvbao_auto_thickness, 0.f, 1.f);
  c[38] = g_gtvbao_history_weight;
  c[39] = g_gtvbao_depth_rejection;
  c[40] = g_gtvbao_normal_rejection;
  c[41] = pass.vbgi ? std::clamp(g_vbgi_base_thickness, 0.01f, 2.f)
                    : std::clamp(g_gtvbao_base_thickness, 0.01f, 2.f);
  c[42] = pass.history_valid ? 1.f : 0.f;
  if (pass.vbgi) {
    c[0] = g_vbgi_quality_level;
    c[1] = std::max(0.001f, g_vbgi_radius);
    c[2] = 1.f;
    c[3] = 1.f;
    c[4] = 1.5f;
    c[5] = std::clamp(g_vbgi_bitmask_thickness, 0.01f, 2.f);
    c[6] = std::clamp(g_vbgi_depth_mip_offset, 0.f, 30.f);
    c[7] = (float)(((data ? data->frame_index : 0u) % 64u) + 32u);
    c[9] = 0.f;
    c[10] = g_vbgi_mrt_normal;
    c[12] = 1.f;
    c[13] = 1.f;
    c[14] = 1.f;
    c[15] = 0.f;
    c[16] = 1.f;
    c[17] = 0.f;
    c[18] = 1.f;
    c[25] = g_vbgi_noise_type;
    c[26] = 2.f;
    c[27] = 0.f;
    c[30] = 0.f;
    c[35] = std::clamp(std::round(g_vbgi_resolution), 0.f, static_cast<float>(kVbgiMaxResolutionIndex));
    c[36] = 0.f;
    c[43] = 1.f;
    c[44] = IsSora1st() ? 0.f : (IsSora2nd() ? 1.f : 2.f);
    c[45] = 0.f;
    c[46] = 65504.f;
    c[47] = g_vbgi_backface_mode;
  }
  return c;
}

// The GTVBAO push block for the VBGI far passes plus the far tail c[48..55] (gtvbao_common.hlsl).
static std::array<float, kVbgiFarPushConstantFloats> BuildVbgiFarPushConstants(DeviceData* data, bool history_valid) {
  std::array<float, kVbgiFarPushConstantFloats> c = {};
  const auto base = BuildGTVBAOPushConstants(data, {.history_valid = history_valid, .vbgi = true});
  std::copy(base.begin(), base.end(), c.begin());
  c[48] = std::max(g_vbgi_far_distance, 0.1f);
  c[49] = std::max(g_vbgi_far_history_length, 1.f);
  c[50] = std::max(2.f, 30.f / static_cast<float>(falcom_world::rtao::RtgiTraceDivisor(g_vbgi_resolution, kVbgiMaxResolutionIndex)))
          * std::clamp(g_vbgi_far_spatial_radius, 0.5f, 2.f);
  c[51] = std::max(g_vbgi_far_intensity, 0.f);
  c[52] = g_vbgi_far_hit_window > 0.5f ? 1.f : 0.f;
  const int vbgi_quality = static_cast<int>(std::lround(std::clamp(g_vbgi_quality_level, 0.f, 3.f)));
  constexpr float kVbgiSlices[4] = {2.f, 3.f, 3.f, 4.f};  // main pass slices per Quality (gtvbao_main.cs_5_0.hlsl)
  c[53] = std::min(std::clamp(std::round(g_vbgi_far_candidates), 1.f, 4.f), kVbgiSlices[vbgi_quality]);
  c[54] = std::clamp(std::round(g_vbgi_far_spatial_neighbors), 1.f, 8.f);
  return c;
}

// -- Pipeline creation --

static bool CreateComputePipelinesIfNeeded(reshade::api::device* dev, DeviceData* d) {
  // CPU opt: when ensure mode is on, skip destruction (kai-style).
  // When off, force-recreate every call (legacy behavior).
  auto dp = [&](reshade::api::pipeline& p) {
    if (g_cpuopt_ensure_pipelines > 0.5f) return;  // keep existing
    if (p.handle) {
      dev->destroy_pipeline(p);
      p = {};
    }
  };
  auto dl = [&](reshade::api::pipeline_layout& l) {
    if (g_cpuopt_ensure_pipelines > 0.5f) return;  // keep existing
    if (l.handle) {
      dev->destroy_pipeline_layout(l);
      l = {};
    }
  };
  dl(d->prefilter_layout);
  dl(d->main_layout);
  dp(d->prefilter_pipeline);
  dp(d->main_pipeline);
  dp(d->vbgi_main_pipeline);
  dp(d->vbgi_radiance_pipeline);
  dp(d->vbgi_mips_pipeline);
  dl(d->vbgi_main_layout);
  dl(d->vbgi_radiance_layout);
  dl(d->vbgi_mips_layout);
  dp(d->vbgi_radiance_far_pipeline);
  dl(d->vbgi_radiance_far_layout);
  dp(d->vbgi_far_main_pipeline);
  dp(d->vbgi_far_resolve_pipeline);
  dp(d->vbgi_far_compose_pipeline);
  dl(d->vbgi_far_main_layout);
  dl(d->vbgi_far_layout);
  dp(d->vbgi_far_temporal_pipeline);
  dp(d->vbgi_far_spatial_pipeline);
  if (g_cpuopt_ensure_pipelines < 0.5f) {
    DestroyGTVBAODescriptorTables(dev, &d->prefilter_tables);
    DestroyGTVBAODescriptorTables(dev, &d->main_tables);
    DestroyGTVBAODescriptorTables(dev, &d->vbgi_main_tables);
    DestroyGTVBAODescriptorTables(dev, &d->vbgi_radiance_tables);
    DestroyGTVBAODescriptorTables(dev, &d->vbgi_radiance_far_tables);
    DestroyGTVBAODescriptorTables(dev, &d->vbgi_mips_tables);
    DestroyGTVBAODescriptorTables(dev, &d->vbgi_far_main_tables);
    DestroyGTVBAODescriptorTables(dev, &d->vbgi_far_tables);
  }

  auto mkcs = [&](std::span<const uint8_t> bc, const char* ep,
                  reshade::api::pipeline_layout lo, reshade::api::pipeline* out) -> bool {
    if (bc.empty() || !lo.handle) return false;
    reshade::api::shader_desc sd = {};
    sd.code = bc.data();
    sd.code_size = bc.size();
    sd.entry_point = ep;
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
                         reshade::api::pipeline_layout* out, uint32_t sampler_count = 1u,
                         uint32_t push_count = kGtvbaoPushConstantFloats) -> bool {
    if (out->handle != 0u) return true;
    DR sampler_r = {0, 0, 0, sampler_count, DS::all_compute, 1, DT::sampler};
    DR cbv_r = {0, 0, 0, 1, DS::all_compute, 1, DT::constant_buffer};
    DR srv_r = {0, 0, 0, srv_count, DS::all_compute, 1, DT::texture_shader_resource_view};
    DR uav_r = {0, 0, 0, uav_count, DS::all_compute, 1, DT::texture_unordered_access_view};
    reshade::api::constant_range push_constants_range = {};
    push_constants_range.binding = 0;
    push_constants_range.dx_register_index = 13;
    push_constants_range.dx_register_space = 0;
    push_constants_range.count = push_count;
    push_constants_range.visibility = DS::all_compute;
    P param_sampler, param_cbv, param_srv, param_uav, param_constants;
    param_sampler.type = reshade::api::pipeline_layout_param_type::descriptor_table;
    param_sampler.descriptor_table.count = 1;
    param_sampler.descriptor_table.ranges = &sampler_r;
    param_cbv.type = reshade::api::pipeline_layout_param_type::descriptor_table;
    param_cbv.descriptor_table.count = 1;
    param_cbv.descriptor_table.ranges = &cbv_r;
    param_srv.type = reshade::api::pipeline_layout_param_type::descriptor_table;
    param_srv.descriptor_table.count = 1;
    param_srv.descriptor_table.ranges = &srv_r;
    param_uav.type = reshade::api::pipeline_layout_param_type::descriptor_table;
    param_uav.descriptor_table.count = 1;
    param_uav.descriptor_table.ranges = &uav_r;
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
  // Main: 5 SRVs (t0 depth MIPs, t1 MRT normal, t2 IS-FAST noise, t3 foliage mask, t4 game depth) + 2 UAVs (u0 AO, u1 debug)
  if (!make_layout(5u, 2u, &d->main_layout)) return false;
  // VBGI gather: the main pass's 4 SRVs plus t4 radiance (5 SRVs) and 2 UAVs.
  if (!make_layout(5u, 2u, &d->vbgi_main_layout)) return false;
  // VBGI radiance prepass: 9 SRVs + 1 UAV. Radiance mip chain: 1 SRV + 1 UAV.
  if (!make_layout(12u, 1u, &d->vbgi_radiance_layout)) return false;
  EnsureGTVBAODescriptorTables(dev, d->vbgi_radiance_layout, &d->vbgi_radiance_tables);
  // Radiance prepass with far feedback: 14 SRVs (t12 far raw, t13 joint raw) + 1 UAV. Optional: a failure leaves far feedback off.
  if (make_layout(14u, 1u, &d->vbgi_radiance_far_layout)) EnsureGTVBAODescriptorTables(dev, d->vbgi_radiance_far_layout, &d->vbgi_radiance_far_tables);
  if (!make_layout(1u, 1u, &d->vbgi_mips_layout)) return false;
  EnsureGTVBAODescriptorTables(dev, d->vbgi_mips_layout, &d->vbgi_mips_tables);
  // VBGI far field: main variant (5 SRVs, 5 UAVs) and far passes (12 SRVs, 4 UAVs), 56-float push block. Optional: a failure leaves far off.
  make_layout(5u, 5u, &d->vbgi_far_main_layout, 1u, kVbgiFarPushConstantFloats);
  make_layout(12u, 4u, &d->vbgi_far_layout, 1u, kVbgiFarPushConstantFloats);
  EnsureGTVBAODescriptorTables(dev, d->prefilter_layout, &d->prefilter_tables);
  if (!d->prefilter_pipeline.handle) mkcs(__gtvbao_prefilter, "main", d->prefilter_layout, &d->prefilter_pipeline);
  if (!d->main_pipeline.handle) mkcs(__gtvbao_main, "main", d->main_layout, &d->main_pipeline);
  if (!d->vbgi_main_pipeline.handle) mkcs(__gtvbao_vbgi_main, "main", d->vbgi_main_layout, &d->vbgi_main_pipeline);
  if (!d->vbgi_radiance_pipeline.handle) mkcs(__vbgi_radiance, "main", d->vbgi_radiance_layout, &d->vbgi_radiance_pipeline);
  if (!d->vbgi_radiance_far_pipeline.handle) mkcs(__vbgi_radiance_far, "main", d->vbgi_radiance_far_layout, &d->vbgi_radiance_far_pipeline);
  if (!d->vbgi_mips_pipeline.handle) mkcs(__vbgi_radiance_mips, "main", d->vbgi_mips_layout, &d->vbgi_mips_pipeline);
  if (!d->vbgi_far_main_pipeline.handle) mkcs(__gtvbao_vbgi_far_main, "main", d->vbgi_far_main_layout, &d->vbgi_far_main_pipeline);
  if (!d->vbgi_far_resolve_pipeline.handle) mkcs(__vbgi_far_resolve, "main", d->vbgi_far_layout, &d->vbgi_far_resolve_pipeline);
  if (!d->vbgi_far_compose_pipeline.handle) mkcs(__vbgi_far_compose, "main", d->vbgi_far_layout, &d->vbgi_far_compose_pipeline);
  if (!d->vbgi_far_temporal_pipeline.handle) mkcs(__vbgi_far_temporal, "main", d->vbgi_far_layout, &d->vbgi_far_temporal_pipeline);
  if (!d->vbgi_far_spatial_pipeline.handle) mkcs(__vbgi_far_spatial, "main", d->vbgi_far_layout, &d->vbgi_far_spatial_pipeline);

  // -- Main pass: visibility bitmask AO only (GI removed; VBGI inactive). --
  return d->prefilter_pipeline.handle && d->main_pipeline.handle;
}

// -- IS-FAST DDS loader (64�64�64, RG8_UNORM Texture3D) --

#pragma pack(push, 1)
struct DDS_PIXELFORMAT {
  uint32_t dwSize, dwFlags, dwFourCC, dwRGBBitCount, dwRBitMask, dwGBitMask, dwBBitMask, dwABitMask;
};
struct DDS_HEADER {
  uint32_t dwSize, dwFlags, dwHeight, dwWidth, dwPitchOrLinearSize, dwDepth, dwMipMapCount;
  uint32_t dwReserved1[11];
  DDS_PIXELFORMAT ddspf;
  uint32_t dwCaps, dwCaps2, dwCaps3, dwCaps4, dwReserved2;
};
struct DDS_HEADER_DXT10 {
  uint32_t dxgiFormat, resourceDimension, miscFlag, arraySize, miscFlags2;
};
#pragma pack(pop)

static bool LoadISFASTNoiseTexture(reshade::api::device* dev, DeviceData* d) {
  // Diagnostic log every frame when debug logging is on
  if (g_isfast_debug_logging > 0.5f) {
    reshade::log::message(reshade::log::level::info,
                          (std::string("[IS-FAST] Status: attempted=") + (d->isfast_texture_attempted ? "yes" : "no")
                           + ", loaded=" + (d->isfast_texture_loaded ? "yes" : "no")
                           + ", srv=" + (d->isfast_noise_srv.handle ? "valid" : "null"))
                              .c_str());
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
  if (magic != 0x20534444) return false;  // "DDS "

  DDS_HEADER hdr = {};
  memcpy(&hdr, bytes + 4, sizeof(hdr));

  uint32_t w = hdr.dwWidth, h = hdr.dwHeight, ddsDepth = hdr.dwDepth;
  uint32_t fmt = 0;
  bool isDX10 = (hdr.ddspf.dwFourCC == 0x30315844);  // "DX10"

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
    if (dx10.resourceDimension != 4) return false;  // must be Texture3D
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
  sub.slice_pitch = w * 2 * h;  // bytes per 2D slice (row_pitch � height)

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

static void GtvbaoBarrier(reshade::api::command_list* cl, reshade::api::resource r,
                          reshade::api::resource_usage o, reshade::api::resource_usage n) {
  if (r.handle) cl->barrier(r, o, n);
}

static void GtvbaoBindPipeline(reshade::api::command_list* cl, reshade::api::pipeline p) {
  cl->bind_pipeline(reshade::api::pipeline_stage::all_compute, p);
}

static void GtvbaoApplyDescriptors(reshade::api::device* dev, reshade::api::command_list* cl,
                                   reshade::api::pipeline_layout lo,
                                   GTVBAODescriptorTableSet* tbl,
                                   uint32_t count,
                                   const reshade::api::descriptor_table_update* updates) {
  std::array<reshade::api::descriptor_table_update, kGtvbaoDescriptorTableParamCount> u = {};
  for (uint32_t i = 0; i < count; ++i) {
    u[i] = updates[i];
    u[i].table = (*tbl)[i];
  }
  dev->update_descriptor_tables(count, u.data());
  std::array<reshade::api::descriptor_table, kGtvbaoDescriptorTableParamCount> b = {};
  for (uint32_t i = 0; i < count; ++i) b[i] = (*tbl)[i];
  cl->bind_descriptor_tables(reshade::api::shader_stage::all_compute, lo, 0, count, b.data());
}

// Depth prefilter.
static void RunGTVBAOSharedPrep(reshade::api::command_list* cl, DeviceData* d) {
  auto* dev = cl->get_device();
  const uint32_t w = d->working_width, h = d->working_height;
  const auto UA = reshade::api::resource_usage::unordered_access;
  const auto SR = reshade::api::resource_usage::shader_resource;
  const auto CS = reshade::api::shader_stage::all_compute;

  // Pass 1: Prefilter
  if (shader_injection.gtvbao_debug_logging > 0.5f)
    reshade::log::message(reshade::log::level::info, "[GTVBAO] Pass 1: binding pipeline...");
  GtvbaoBindPipeline(cl, d->prefilter_pipeline);
  if (shader_injection.gtvbao_debug_logging > 0.5f)
    reshade::log::message(reshade::log::level::info, "[GTVBAO] Pass 1: updating descriptors...");
  {
    reshade::api::descriptor_table_update u[4] = {
        {{}, 0, 0, 1, reshade::api::descriptor_type::sampler, &d->point_clamp_sampler},
        {{}, 0, 0, 1, reshade::api::descriptor_type::constant_buffer, &d->captured_scene_cbv_view},
        {{}, 0, 0, 1, reshade::api::descriptor_type::texture_shader_resource_view, &d->captured_depth_srv},
        {{}, 0, 0, kGTVBAODepthMipLevels, reshade::api::descriptor_type::texture_unordered_access_view, d->depth_mips_uavs.data()},
    };
    GtvbaoApplyDescriptors(dev, cl, d->prefilter_layout, &d->prefilter_tables, 4, u);
    auto pc = BuildGTVBAOPushConstants(d);
    cl->push_constants(CS, d->prefilter_layout, kGtvbaoPushConstantsLayoutParam, 0, kGtvbaoPushConstantFloats, pc.data());
  }
  cl->dispatch((w + 15) / 16, (h + 15) / 16, 1);
  GtvbaoBarrier(cl, d->depth_mips_texture, UA, SR);
  if (shader_injection.gtvbao_debug_logging > 0.5f)
    reshade::log::message(reshade::log::level::info, "[GTVBAO] Pass 1 (prefilter) done.");
}

// -- Dispatch --

static bool RunGTVBAO(reshade::api::command_list* cl, DeviceData* d) {
  namespace rtao = falcom_world::rtao;
  if (!d->captured_depth_srv.handle) return false;

  auto* dev = cl->get_device();

  // -- IS-FAST noise texture (load once) --
  if (g_isfast_enabled > 0.5f) LoadISFASTNoiseTexture(dev, d);

  if (shader_injection.gtvbao_debug_logging > 0.5f)
    reshade::log::message(reshade::log::level::info, "[GTVBAO] RunGTVBAO: creating pipelines...");
  const bool temporal_wanted = g_gtvbao_temporal_enabled > 0.5f
                               && (IsSora1st() || IsSora2nd() || IsKai()) && g_cpuopt_deferred_dispatch < 0.5f;
  const bool rr_bypass = falcom_world::rr::RrOwnsDenoise();
  if (temporal_wanted && d->gtvbao_last_frame == d->frame_index) return true;
  if (!CreateComputePipelinesIfNeeded(dev, d)) return false;
  if (shader_injection.gtvbao_debug_logging > 0.5f)
    reshade::log::message(reshade::log::level::info, "[GTVBAO] RunGTVBAO: allocating descriptor tables...");
  if (!EnsureGTVBAODescriptorTables(dev, d->prefilter_layout, &d->prefilter_tables)) return false;
  if (!EnsureGTVBAODescriptorTables(dev, d->main_layout, &d->main_tables)) return false;
  if (!EnsureGTVBAODescriptorTables(dev, d->foliage_mask_layout, &d->foliage_mask_tables)) return false;

  uint32_t w = d->working_width, h = d->working_height;
  if (w < 64 || h < 64) {
    CSLog("gtvbao", "run entry ABORT: working too small", true);
    return false;
  }
  // �� Low-resolution spatial pipeline: AO evaluation domain. Full path
  // uses w/h everywhere (existing behavior); low-res uses aw/ah for main,
  // denoise, and �-trous while depth/MRT/light stay full.
  const bool low_res = shader_injection.gtvbao_resolution > 0.5f
                       && d->ao_width >= 32u && d->ao_height >= 32u
                       && d->ao_term_a_texture.handle;
  const float svgf_radius_scale = (low_res ? g_gtvbao_svgf_half_res_radius_scale : g_gtvbao_svgf_radius_scale);
  const uint32_t aw = low_res ? d->ao_width : w;
  const uint32_t ah = low_res ? d->ao_height : h;
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
                          (std::string("[GTVBAO] RunGTVBAO: dispatching pass 1 (") + std::to_string(w) + "x" + std::to_string(h) + ")").c_str());

  const auto UA = reshade::api::resource_usage::unordered_access;
  const auto SR = reshade::api::resource_usage::shader_resource;
  const auto CS = reshade::api::shader_stage::all_compute;
  namespace rtgi = falcom_world::rtgi;
  static constexpr rtgi::RtgiPipelineDesc kGtvbaoUpsampleDesc = {.srv_count = rtgi::kRtgiUpsampleSrvCount, .uav_count = rtgi::kRtgiUpsampleUavCount, .sampler_count = 0u, .push_count = rtgi::kRtgiUpsamplePushCount, .code = __gtvbao_svgf_upsample, .name = "gtvbao upsample"};
  const bool motion_ok = temporal_wanted && UpdateRtMotion(dev, d, true);
  const uint32_t divisor = low_res ? 2u : 1u;
  const uint32_t full_w = d->captured_depth_w, full_h = d->captured_depth_h;
  rtgi::RtgiDenoiseReady ready = {};
  if (d->captured_mrt_normal_srv.handle != 0u && d->captured_scene_cbv_view.handle != 0u && full_w != 0u && full_h != 0u
      && aw == rtao::RtgiTraceExtent(full_w, divisor) && ah == rtao::RtgiTraceExtent(full_h, divisor)
      && rtgi::EnsureRtgiPipeline(dev, &d->gtvbao_denoise.upsample_pipeline, kGtvbaoUpsampleDesc)
      && rtgi::EnsureRtgiTextureSet(dev, &d->gtvbao_svgf_raw, reshade::api::format::r16g16b16a16_float, 8u, aw, ah)) {
    ready = rtgi::EnsureRtgiDenoise(dev, &d->gtvbao_denoise, {.output_format = reshade::api::format::r32_uint, .output_bytes_per_pixel = 4u, .trace_width = aw, .trace_height = ah, .full_width = full_w, .full_height = full_h, .temporal = motion_ok, .spatial = g_gtvbao_svgf_spatial_enabled > 0.5f, .log_prefix = "[gtvbao]", .full_resolution = g_gtvbao_full_res_denoise > 0.5f, .nearest_fallback = true, .bypass_svgf = rr_bypass});
  }
  if (!ready.ok) {
    CSLog("gtvbao", "svgf chain not ready (normal, cbv, size, pipelines or targets)", true);
    return false;
  }
  RunGTVBAOSharedPrep(cl, d);

  // -- Foliage mask pre-pass (AO working domain, reads MRT normal, writes R8_UINT) --
  if (foliage_mask_valid) {
    uint32_t mkW = aw, mkH = ah;
    GtvbaoBindPipeline(cl, d->foliage_mask_pipeline);
    reshade::api::resource_view fm_srvs[2] = {
        d->depth_mips_srv,          // t0 � working depth (GetDimensions)
        d->captured_mrt_normal_srv  // t1 � G-buffer normal (bit 15 test)
    };
    reshade::api::descriptor_table_update fu[4] = {
        {{}, 0, 0, 1, reshade::api::descriptor_type::sampler, &d->point_clamp_sampler},
        {{}, 0, 0, 1, reshade::api::descriptor_type::constant_buffer, &d->captured_scene_cbv_view},
        {{}, 0, 0, 2, reshade::api::descriptor_type::texture_shader_resource_view, fm_srvs},
        {{}, 0, 0, 1, reshade::api::descriptor_type::texture_unordered_access_view, &d->foliage_mask_uav},
    };
    GtvbaoApplyDescriptors(dev, cl, d->foliage_mask_layout, &d->foliage_mask_tables, 4, fu);
    auto pc = BuildGTVBAOPushConstants(d);
    cl->push_constants(CS, d->foliage_mask_layout, kGtvbaoPushConstantsLayoutParam, 0, kGtvbaoPushConstantFloats, pc.data());
    cl->dispatch((mkW + 7) / 8, (mkH + 7) / 8, 1);
    GtvbaoBarrier(cl, d->foliage_mask_texture, UA, SR);
  }

  // Temporal accumulation: the reset rule shared with RTAO and RTGI over this pass's own state.
  // Motion is checked only while Temporal is wanted; without motion the AO stays spatial only.
  std::array<float, kGtvbaoSnapshotFloats> snapshot = {};
  rtao::ResetReason reset = rtao::ResetReason::Disabled;
  if (temporal_wanted) {
    const auto pc_all = BuildGTVBAOPushConstants(d);
    for (uint32_t i = 0; i < kGtvbaoSnapshotFloats; ++i) snapshot[i] = pc_all[kGtvbaoSnapshotIndex[i]];
    reset = rtao::TemporalResetReason(rtao::TemporalFrame{
        .temporal_on = true,
        .was_on = d->gtvbao_temporal_last,
        .resized = d->temporal_seen_w != aw || d->temporal_seen_h != ah,
        .frame = d->frame_index,
        .last_frame = d->gtvbao_last_frame,
        .guard = d->dyncube_loadingWipePending || d->frame_index < d->resize_guard_until_frame,
        .scaled = false,
        .params_changed = d->gtvbao_prev_params_valid
                          && (!(snapshot == d->gtvbao_prev_params) || d->gtvbao_prev_svgf_history_length != g_gtvbao_svgf_history_length
                              || d->gtvbao_prev_svgf_radius_scale != svgf_radius_scale),
        .motion_ok = motion_ok});
    if (!rr_bypass && reset != rtao::ResetReason::None)
      CSLog("gtvbao", std::string("temporal reset: ") + rtao::ResetReasonName(reset), true);
  }
  const bool history_valid = !rr_bypass && reset == rtao::ResetReason::None;

  // Pass 2: Main
  reshade::api::pipeline mp = d->main_pipeline;
  if (!mp.handle) return false;
  GtvbaoBindPipeline(cl, mp);
  {
    reshade::api::resource_view main_srvs[5] = {
        d->depth_mips_srv,
        d->captured_mrt_normal_srv.handle ? d->captured_mrt_normal_srv : d->fallback_srv,
        d->isfast_noise_srv.handle ? d->isfast_noise_srv : d->fallback_srv,  // t2 IS-FAST noise
        d->foliage_mask_srv.handle ? d->foliage_mask_srv : d->fallback_srv,  // t3 foliage mask
        d->captured_depth_srv                                                // t4 game depth (upsample skip)
    };
    // The debug UAV is only read while a debug view is active; bind the
    // fallback otherwise so nothing can sample stale debug output.
    const bool gtvbao_debug_active = IsGtvbaoComputeDebug(GtvbaoDebugMode());
    // Shader register order: u0=AO, u1=debug
    reshade::api::resource_view main_uavs[2] = {
        d->gtvbao_svgf_raw.uav[0],
        (gtvbao_debug_active && d->debug_uav.handle) ? d->debug_uav : d->fallback_uav};
    reshade::api::descriptor_table_update u[4] = {
        {{}, 0, 0, 1, reshade::api::descriptor_type::sampler, &d->point_clamp_sampler},
        {{}, 0, 0, 1, reshade::api::descriptor_type::constant_buffer, &d->captured_scene_cbv_view},
        {{}, 0, 0, 5, reshade::api::descriptor_type::texture_shader_resource_view, main_srvs},
        {{}, 0, 0, 2, reshade::api::descriptor_type::texture_unordered_access_view, main_uavs},
    };
    GtvbaoApplyDescriptors(dev, cl, d->main_layout, &d->main_tables, 4, u);
    // The 4x4 reconstruction reads trace pixels beyond the default 2x2 footprint, so keep those AO samples traced.
    auto pc = BuildGTVBAOPushConstants(d, {.final_pass = false, .foliage_mask_valid = foliage_mask_valid, .skip_unread = (g_gtvbao_full_res_denoise > 0.5f ? 0 : (ready.reproject ? 1 : (low_res && g_gtvbao_upscale_algorithm > 2.5f ? 0 : 2))), .full_width = full_w, .full_height = full_h, .divisor = divisor});
    cl->push_constants(CS, d->main_layout, kGtvbaoPushConstantsLayoutParam, 0, kGtvbaoPushConstantFloats, pc.data());
  }
  cl->dispatch((aw + 7) / 8, (ah + 7) / 8, 1);
  GtvbaoBarrier(cl, d->gtvbao_svgf_raw.texture[0], UA, SR);
  if (IsGtvbaoComputeDebug(GtvbaoDebugMode())) {
    d->gtvbao_debug_frame = d->frame_index;
    d->gtvbao_debug_mode_written = (float)GtvbaoDebugMode();
  }
  // Skip barriers for UAVs nothing reads. debug_texture is only read by the
  // gtvbao debug views (see t23 pushes).
  {
    const bool dbgRead = IsGtvbaoComputeDebug(GtvbaoDebugMode()) && !(GtvbaoDebugMode() == 23 && ready.reproject);
    if (dbgRead)
      GtvbaoBarrier(cl, d->debug_texture, UA, SR);  // Debug output ready for read
  }
  if (shader_injection.gtvbao_debug_logging > 0.5f)
    reshade::log::message(reshade::log::level::info, "[GTVBAO] Pass 2 (main) done.");

  {
    std::vector<reshade::api::descriptor_table_update> table_nulls;
    const bool gt_conf = GtvbaoDebugMode() == 23 && d->debug_uav.handle != 0u;
    // Half: DispatchRtgiDenoise subtracts log2(divisor) = 1, so + 1 makes Half Res Iterations the effective count.
    const float svgf_iterations = low_res ? g_gtvbao_svgf_half_res_iterations + (g_gtvbao_full_res_denoise > 0.5f ? 0.f : 1.f) : g_gtvbao_svgf_iterations;
    rtgi::DispatchRtgiDenoise(dev, cl, &d->gtvbao_denoise,
                              {.raw_view = d->gtvbao_svgf_raw.srv[0],
                               .depth_view = d->captured_depth_srv,
                               .mrt_normal_view = d->captured_mrt_normal_srv,
                               .scene_cbv_view = d->captured_scene_cbv_view,
                               .motion_view = d->mb_rtv4_owned_srv,
                               .albedo_view = reshade::api::resource_view{0u},
                               .stats_uav = reshade::api::resource_view{0u},
                               .debug_uav = gt_conf ? d->debug_uav : reshade::api::resource_view{0u},
                               .debug_texture = d->debug_texture,
                               .trace_width = aw,
                               .trace_height = ah,
                               .full_width = full_w,
                               .full_height = full_h,
                               .divisor = static_cast<float>(divisor),
                               .debug_mode = gt_conf ? 5 : 0,
                               .svgf = {g_gtvbao_history_weight, g_gtvbao_depth_rejection, g_gtvbao_normal_rejection,
                                        history_valid, svgf_iterations, g_gtvbao_svgf_luminance_edge_stop,
                                        g_gtvbao_svgf_depth_sigma, g_gtvbao_svgf_normal_power, g_gtvbao_full_res_denoise > 0.5f, g_gtvbao_svgf_history_length, svgf_radius_scale},
                               .luminance_weights = rtgi::kSvgfLuminanceAo,
                               .upsample_intensity = 1.f,
                               .upsample_char_intensity = 1.f,
                               .upsample_saturation = 1.f,
                               .upsample = {.algorithm = g_gtvbao_upscale_algorithm,
                                            .depth_plane_sigma = g_gtvbao_upscale_plane_sigma,
                                            .normal_edge_power = g_gtvbao_upscale_normal_power,
                                            .bilinear_weight_sharpness = g_gtvbao_upscale_bilinear_sharpness,
                                            .nearest_valid_fallback = g_gtvbao_upscale_nearest_fallback}},
                              ready, &table_nulls);
    dev->update_descriptor_tables(static_cast<uint32_t>(table_nulls.size()), table_nulls.data());
  }
  if (shader_injection.gtvbao_debug_logging > 0.5f)
    reshade::log::message(reshade::log::level::info, "[GTVBAO] All passes complete.");
  d->gtvbao_temporal_last = ready.temporal;
  d->gtvbao_last_frame = d->frame_index;
  d->temporal_seen_w = aw;
  d->temporal_seen_h = ah;
  d->gtvbao_prev_params_valid = temporal_wanted;
  if (temporal_wanted) {
    d->gtvbao_prev_params = snapshot;
    d->gtvbao_prev_svgf_history_length = g_gtvbao_svgf_history_length;
    d->gtvbao_prev_svgf_radius_scale = svgf_radius_scale;
  }
  CSLog("gtvbao", "run exit ok");
  return true;
}

// VBGI radiance prepass (M7.4 step 3): sender radiance and its mip chain, once per frame, from the lighting draw.
constexpr float kVbgiRadianceClamp = 16.f;    // HDR cap per channel
constexpr float kVbgiFeedbackMax = 0.9f;      // Multi-Bounce Strength x Intensity limit (RTGI rule)
constexpr float kVbgiFarFeedbackMax = 0.75f;  // Far Multi-Bounce Strength x Intensity limit (M4)
constexpr float kVbgiFireflyK = 4.0f;         // fed-back luminance limit, x the 8-neighbour mean (M4)
static bool RunVbgiRadiance(reshade::api::command_list* cl, DeviceData* d) {
  if (d->vbgi_radiance_frame == d->frame_index) return true;
  if (!VbgiRunning() || !d->captured_depth_srv.handle || !d->captured_mrt_normal_srv.handle
      || !d->captured_scene_cbv_valid || !d->captured_color_srv.handle || d->captured_color_frame != d->frame_index) {
    CSLog("vbgi", "radiance skipped: depth=" + std::to_string(d->captured_depth_srv.handle != 0) + " mrt=" + std::to_string(d->captured_mrt_normal_srv.handle != 0) + " cbv=" + std::to_string(d->captured_scene_cbv_valid) + " color=" + std::to_string(d->captured_color_srv.handle != 0) + " colorAge=" + std::to_string(d->captured_color_frame == UINT64_MAX ? -1 : (long long)(d->frame_index - d->captured_color_frame)), true);
    return false;
  }
  for (uint32_t k = 0; k < 6; ++k) {
    if (k == 1 && IsKai()) continue;
    if (!d->vbgi_in_srv[k].handle || !d->vbgi_in_live[k] || d->vbgi_in_frame[k] != d->frame_index) {
      static bool s_inputs_logged = false;
      if (!s_inputs_logged) {
        CSLog("vbgi", "radiance skipped: lighting input " + std::to_string(k) + " not captured", true);
        s_inputs_logged = true;
      }
      return false;
    }
  }
  if (!d->vbgi_radiance_texture.handle || !d->vbgi_radiance_srv.handle || !d->vbgi_radiance_mip_uav[0].handle) return false;

  auto* dev = cl->get_device();
  if (!d->vbgi_radiance_pipeline.handle || !d->vbgi_mips_pipeline.handle) CreateComputePipelinesIfNeeded(dev, d);
  if (!d->vbgi_radiance_pipeline.handle || !d->vbgi_mips_pipeline.handle) return false;
  if (!EnsureGTVBAODescriptorTables(dev, d->vbgi_radiance_layout, &d->vbgi_radiance_tables)) return false;
  if (!EnsureGTVBAODescriptorTables(dev, d->vbgi_mips_layout, &d->vbgi_mips_tables)) return false;

  const auto UA = reshade::api::resource_usage::unordered_access;
  const auto SR = reshade::api::resource_usage::shader_resource;
  const uint32_t w = d->vbgi_width, h = d->vbgi_height;
  const uint32_t div = falcom_world::rtao::RtgiTraceDivisor(g_vbgi_resolution, kVbgiMaxResolutionIndex);
  const bool kai = IsKai();

  // Multi-Bounce (M7.6): last frame's denoised GI (the newest output, not written until this frame's denoise) re-enters as
  // sender light. Off, stale or without valid motion: weight 0 and the two extra slots stay on the fallback view.
  float feedback_w = 0.f;
  reshade::api::resource_view prev_gi_srv = d->fallback_srv;
  reshade::api::resource_view motion_srv = d->fallback_srv;
  // Feedback reads the PREVIOUS frame's denoise (this frame's runs after the prepass), so the age is 1, not the composite's 0.
  if (!(g_vbgi_temporal_far_only > 0.5f && g_vbgi_temporal_enabled < 0.5f)
      && shader_injection.vbgi_multibounce > 0.5f
      && VbgiGiFresh(d, 1u)
      && d->frame_index >= d->resize_guard_until_frame && !d->dyncube_loadingWipePending
      && UpdateRtMotion(dev, d, true)) {
    feedback_w = std::min(g_vbgi_multibounce_strength * shader_injection.vbgi_intensity, kVbgiFeedbackMax);
    prev_gi_srv = d->vbgi_denoise.output.srv[d->vbgi_denoise.output_index];
    motion_srv = d->mb_rtv4_owned_srv;
  }

  // Far feedback (M4): with the far field on, the radiance variant splits last frame's joint denoised GI into near and
  // far shares (far raw / joint raw) with separate weights and firefly-clamps it. prev_gi_srv is only set when the
  // near feedback gate above passed (Multi-Bounce on, fresh denoise, no guard, motion valid).
  const bool far_fb = VbgiFarRunning() && shader_injection.vbgi_multibounce > 0.5f
                      && d->vbgi_radiance_far_pipeline.handle != 0u
                      && EnsureGTVBAODescriptorTables(dev, d->vbgi_radiance_far_layout, &d->vbgi_radiance_far_tables);
  float far_feedback_w = 0.f;
  reshade::api::resource_view prev_far_raw_srv = d->fallback_srv;
  reshade::api::resource_view prev_joint_raw_srv = d->fallback_srv;
  if (far_fb && prev_gi_srv.handle != d->fallback_srv.handle && d->vbgi_far_frame != UINT64_MAX
      && d->frame_index - d->vbgi_far_frame <= 1u && d->vbgi_far_raw.srv[0].handle != 0u
      && d->vbgi_far_joint.srv[0].handle != 0u) {
    far_feedback_w = std::min(g_vbgi_far_multibounce_strength * shader_injection.vbgi_intensity, kVbgiFarFeedbackMax);
    prev_far_raw_srv = d->vbgi_far_raw.srv[0];
    prev_joint_raw_srv = d->vbgi_far_joint.srv[0];
  }
  static float s_far_fb_logged[2] = {-1.f, -1.f};
  if (far_fb && (s_far_fb_logged[0] != feedback_w || s_far_fb_logged[1] != far_feedback_w)) {
    s_far_fb_logged[0] = feedback_w;
    s_far_fb_logged[1] = far_feedback_w;
    CSLog("vbgi", "far feedback w_near=" + std::to_string(feedback_w) + " w_far=" + std::to_string(far_feedback_w)
                      + " far_cap=" + std::to_string(kVbgiFarFeedbackMax) + " firefly_k=" + std::to_string(kVbgiFireflyK));
  }

  // Mip 0: the radiance itself.
  GtvbaoBindPipeline(cl, d->vbgi_radiance_pipeline);
  {
    reshade::api::resource_view srvs[12] = {
        d->captured_color_srv,
        d->captured_mrt_normal_srv,
        d->vbgi_in_srv[0],
        kai ? d->fallback_srv : d->vbgi_in_srv[1],
        d->captured_depth_srv,
        d->vbgi_in_srv[2],
        kai ? d->fallback_srv : d->vbgi_in_srv[3],
        d->vbgi_in_srv[4], d->vbgi_in_srv[5],
        prev_gi_srv, motion_srv,
        kai ? d->vbgi_in_srv[3] : d->fallback_srv};
    reshade::api::descriptor_table_update u[4] = {
        {{}, 0, 0, 1, reshade::api::descriptor_type::sampler, &d->point_clamp_sampler},
        {{}, 0, 0, 1, reshade::api::descriptor_type::constant_buffer, &d->captured_scene_cbv_view},
        {{}, 0, 0, 12, reshade::api::descriptor_type::texture_shader_resource_view, srvs},
        {{}, 0, 0, 1, reshade::api::descriptor_type::texture_unordered_access_view, &d->vbgi_radiance_mip_uav[0]},
    };
    GtvbaoApplyDescriptors(dev, cl, d->vbgi_radiance_layout, &d->vbgi_radiance_tables, 4, u);
    std::array<float, kGtvbaoPushConstantFloats> pc = {};
    pc[0] = g_vbgi_ambient;
    pc[1] = g_vbgi_lights;
    pc[2] = g_vbgi_sun;
    pc[3] = g_vbgi_emissive;
    pc[4] = kVbgiRadianceClamp;
    pc[5] = (float)div;
    pc[6] = feedback_w;
    pc[7] = IsSora1st() ? 0.f : (IsSora2nd() ? 1.f : 2.f);
    cl->push_constants(reshade::api::shader_stage::all_compute, d->vbgi_radiance_layout, kGtvbaoPushConstantsLayoutParam, 0, kGtvbaoPushConstantFloats, pc.data());
  }
  if (far_fb) {
    GtvbaoBindPipeline(cl, d->vbgi_radiance_far_pipeline);
    reshade::api::resource_view far_srvs[14] = {
        d->captured_color_srv,
        d->captured_mrt_normal_srv,
        d->vbgi_in_srv[0],
        kai ? d->fallback_srv : d->vbgi_in_srv[1],
        d->captured_depth_srv,
        d->vbgi_in_srv[2],
        kai ? d->fallback_srv : d->vbgi_in_srv[3],
        d->vbgi_in_srv[4], d->vbgi_in_srv[5],
        prev_gi_srv, motion_srv,
        kai ? d->vbgi_in_srv[3] : d->fallback_srv,
        prev_far_raw_srv, prev_joint_raw_srv};
    reshade::api::descriptor_table_update u[4] = {
        {{}, 0, 0, 1, reshade::api::descriptor_type::sampler, &d->point_clamp_sampler},
        {{}, 0, 0, 1, reshade::api::descriptor_type::constant_buffer, &d->captured_scene_cbv_view},
        {{}, 0, 0, 14, reshade::api::descriptor_type::texture_shader_resource_view, far_srvs},
        {{}, 0, 0, 1, reshade::api::descriptor_type::texture_unordered_access_view, &d->vbgi_radiance_mip_uav[0]},
    };
    GtvbaoApplyDescriptors(dev, cl, d->vbgi_radiance_far_layout, &d->vbgi_radiance_far_tables, 4, u);
    std::array<float, kGtvbaoPushConstantFloats> pc = {};
    pc[0] = g_vbgi_ambient;
    pc[1] = g_vbgi_lights;
    pc[2] = g_vbgi_sun;
    pc[3] = g_vbgi_emissive;
    pc[4] = kVbgiRadianceClamp;
    pc[5] = (float)div;
    pc[6] = feedback_w;
    pc[7] = IsSora1st() ? 0.f : (IsSora2nd() ? 1.f : 2.f);
    pc[8] = far_feedback_w;
    pc[9] = kVbgiFireflyK;
    cl->push_constants(reshade::api::shader_stage::all_compute, d->vbgi_radiance_far_layout, kGtvbaoPushConstantsLayoutParam, 0, kGtvbaoPushConstantFloats, pc.data());
  }
  cl->dispatch((w + 7) / 8, (h + 7) / 8, 1);

  // Mips 1-4: 2x2 box filter of the level above.
  for (uint32_t m = 1; m < 5; ++m) {
    GtvbaoBindPipeline(cl, d->vbgi_mips_pipeline);
    reshade::api::resource_view src = d->vbgi_radiance_mip_srv[m - 1];
    reshade::api::resource_view dst = d->vbgi_radiance_mip_uav[m];
    reshade::api::descriptor_table_update u[4] = {
        {{}, 0, 0, 1, reshade::api::descriptor_type::sampler, &d->point_clamp_sampler},
        {{}, 0, 0, 1, reshade::api::descriptor_type::constant_buffer, &d->captured_scene_cbv_view},
        {{}, 0, 0, 1, reshade::api::descriptor_type::texture_shader_resource_view, &src},
        {{}, 0, 0, 1, reshade::api::descriptor_type::texture_unordered_access_view, &dst},
    };
    GtvbaoApplyDescriptors(dev, cl, d->vbgi_mips_layout, &d->vbgi_mips_tables, 4, u);
    cl->dispatch((std::max(1u, w >> m) + 7u) / 8u, (std::max(1u, h >> m) + 7u) / 8u, 1);
  }

  falcom_world::rtao::NullComputeSlots(cl, 12, 1, 1);
  if (far_fb) falcom_world::rtao::NullComputeSlots(cl, 14, 1, 1);
  GtvbaoBarrier(cl, d->vbgi_radiance_texture, UA, SR);
  d->vbgi_radiance_frame = d->frame_index;
  static bool s_logged = false;
  if (!s_logged) {
    CSLog("vbgi", "radiance dispatched " + std::to_string(w) + "x" + std::to_string(h));
    s_logged = true;
  }
  return true;
}

static bool RunVBGI(reshade::api::command_list* cl, DeviceData* d, bool prefilter_done) {
  if (!d->captured_depth_srv.handle) return false;
  auto* dev = cl->get_device();

  if (g_isfast_enabled > 0.5f) LoadISFASTNoiseTexture(dev, d);

  if (!d->prefilter_pipeline.handle || !d->vbgi_main_pipeline.handle || !d->vbgi_main_layout.handle) CreateComputePipelinesIfNeeded(dev, d);
  if (!d->prefilter_pipeline.handle || !d->vbgi_main_pipeline.handle) return false;

  if (!prefilter_done && !EnsureGTVBAODescriptorTables(dev, d->prefilter_layout, &d->prefilter_tables)) return false;
  if (!EnsureGTVBAODescriptorTables(dev, d->vbgi_main_layout, &d->vbgi_main_tables)) return false;

  const uint32_t w = d->working_width, h = d->working_height;
  if (w < 64 || h < 64 || !d->vbgi_raw_texture.handle || !d->vbgi_raw_uav.handle || d->vbgi_width < 32u || d->vbgi_height < 32u) {
    CSLog("vbgi", "run ABORT: resources not ready", true);
    return false;
  }
  if (d->vbgi_radiance_frame == UINT64_MAX || d->frame_index - d->vbgi_radiance_frame > ((g_cpuopt_deferred_dispatch > 0.5f) ? 1u : 0u)) {
    CSLog("vbgi", "run ABORT: radiance not produced", true);
    return false;
  }
  const bool vbgi_dbg = VbgiGtvbaoDebugMode() != 0 && d->debug_uav.handle;

  // VBGI far field: the far main variant also writes one candidate per texel; the resolve adds L x W to the near raw
  // GI (joint raw), which replaces the near raw as the input of the existing denoise chain.
  bool far_on = VbgiFarRunning();
  bool far_params_changed = false;
  if (far_on) {
    const uint32_t tw = d->vbgi_width, th = d->vbgi_height;
    const bool far_psos = d->vbgi_far_main_pipeline.handle != 0u && d->vbgi_far_resolve_pipeline.handle != 0u;
    const bool far_tables = far_psos && EnsureGTVBAODescriptorTables(dev, d->vbgi_far_main_layout, &d->vbgi_far_main_tables)
                            && EnsureGTVBAODescriptorTables(dev, d->vbgi_far_layout, &d->vbgi_far_tables);
    const bool far_textures = far_tables
                              && falcom_world::rtgi::EnsureRtgiTextureSet(dev, &d->vbgi_far_cand_pos, reshade::api::format::r32g32b32a32_float, 16u, tw, th)
                              && falcom_world::rtgi::EnsureRtgiTextureSet(dev, &d->vbgi_far_cand_rad, reshade::api::format::r16g16b16a16_float, 8u, tw, th)
                              && falcom_world::rtgi::EnsureRtgiTextureSet(dev, &d->vbgi_far_cand_nrm, reshade::api::format::r16g16b16a16_float, 8u, tw, th)
                              && falcom_world::rtgi::EnsureRtgiTextureSet(dev, &d->vbgi_far_raw, reshade::api::format::r16g16b16a16_float, 8u, tw, th)
                              && falcom_world::rtgi::EnsureRtgiTextureSet(dev, &d->vbgi_far_joint, reshade::api::format::r16g16b16a16_float, 8u, tw, th);
    far_on = far_textures;
    if (!far_on) {
      CSLog("vbgi", "far skipped: psos=" + std::to_string(far_psos) + " tables=" + std::to_string(far_tables) + " textures=" + std::to_string(far_textures), true);
    }
    const std::array<float, 11> far_params = {g_vbgi_far_distance, g_vbgi_far_history_length, g_vbgi_far_hit_window,
                                              g_vbgi_quality_level, g_vbgi_resolution, g_vbgi_radius,
                                              g_vbgi_bitmask_thickness, g_vbgi_auto_thickness, g_vbgi_base_thickness,
                                              g_vbgi_noise_type, g_vbgi_mrt_normal};
    far_params_changed = d->vbgi_far_prev_params_valid && far_params != d->vbgi_far_prev_params;
    d->vbgi_far_prev_params = far_params;
    d->vbgi_far_prev_params_valid = true;
  }
  if (!far_on && d->vbgi_far_color_denoise.history_color.texture[0].handle != 0u) {
    falcom_world::rtgi::ReleaseRtgiDenoiseTextures(dev, &d->vbgi_far_color_denoise);
    d->vbgi_far_color_last = false;
    d->vbgi_far_color_prev_params_valid = false;
  }
  const bool far_toggled = far_on != d->vbgi_far_was_on;
  d->vbgi_far_was_on = far_on;
  // Binds the far-pass layout (12 SRVs, 4 UAVs, far push block) and dispatches one 8x8 far pass over gw x gh.
  auto dispatch_far = [&](reshade::api::pipeline pso, const std::array<reshade::api::resource_view, 12>& srvs,
                          const std::array<reshade::api::resource_view, 4>& uavs, bool history_valid, uint32_t gw, uint32_t gh) {
    GtvbaoBindPipeline(cl, pso);
    reshade::api::descriptor_table_update u[4] = {
        {{}, 0, 0, 1, reshade::api::descriptor_type::sampler, &d->point_clamp_sampler},
        {{}, 0, 0, 1, reshade::api::descriptor_type::constant_buffer, &d->captured_scene_cbv_view},
        {{}, 0, 0, 12, reshade::api::descriptor_type::texture_shader_resource_view, srvs.data()},
        {{}, 0, 0, 4, reshade::api::descriptor_type::texture_unordered_access_view, uavs.data()},
    };
    GtvbaoApplyDescriptors(dev, cl, d->vbgi_far_layout, &d->vbgi_far_tables, 4, u);
    const auto pc = BuildVbgiFarPushConstants(d, history_valid);
    cl->push_constants(reshade::api::shader_stage::all_compute, d->vbgi_far_layout, kGtvbaoPushConstantsLayoutParam, 0, kVbgiFarPushConstantFloats, pc.data());
    cl->dispatch((gw + 7u) / 8u, (gh + 7u) / 8u, 1);
    falcom_world::rtao::NullComputeSlots(cl, 12, 4, 1);
  };

  if (!prefilter_done) RunGTVBAOSharedPrep(cl, d);

  if (vbgi_dbg) GtvbaoBarrier(cl, d->debug_texture, reshade::api::resource_usage::shader_resource, reshade::api::resource_usage::unordered_access);
  GtvbaoBindPipeline(cl, d->vbgi_main_pipeline);
  {
    reshade::api::resource_view main_srvs[5] = {
        d->depth_mips_srv,
        d->captured_mrt_normal_srv.handle ? d->captured_mrt_normal_srv : d->fallback_srv,
        d->isfast_noise_srv.handle ? d->isfast_noise_srv : d->fallback_srv,
        d->foliage_mask_srv.handle ? d->foliage_mask_srv : d->fallback_srv,
        d->vbgi_radiance_srv};
    reshade::api::resource_view main_uavs[2] = {
        d->vbgi_raw_uav,
        vbgi_dbg ? d->debug_uav : d->fallback_uav};
    reshade::api::descriptor_table_update u[4] = {
        {{}, 0, 0, 1, reshade::api::descriptor_type::sampler, &d->point_clamp_sampler},
        {{}, 0, 0, 1, reshade::api::descriptor_type::constant_buffer, &d->captured_scene_cbv_view},
        {{}, 0, 0, 5, reshade::api::descriptor_type::texture_shader_resource_view, main_srvs},
        {{}, 0, 0, 2, reshade::api::descriptor_type::texture_unordered_access_view, main_uavs},
    };
    GtvbaoApplyDescriptors(dev, cl, d->vbgi_main_layout, &d->vbgi_main_tables, 4, u);
    auto pc = BuildGTVBAOPushConstants(d, {.vbgi = true});
    cl->push_constants(reshade::api::shader_stage::all_compute, d->vbgi_main_layout, kGtvbaoPushConstantsLayoutParam, 0, kGtvbaoPushConstantFloats, pc.data());
  }
  if (far_on) {
    GtvbaoBindPipeline(cl, d->vbgi_far_main_pipeline);
    reshade::api::resource_view far_main_srvs[5] = {
        d->depth_mips_srv,
        d->captured_mrt_normal_srv.handle ? d->captured_mrt_normal_srv : d->fallback_srv,
        d->isfast_noise_srv.handle ? d->isfast_noise_srv : d->fallback_srv,
        d->foliage_mask_srv.handle ? d->foliage_mask_srv : d->fallback_srv,
        d->vbgi_radiance_srv};
    reshade::api::resource_view far_main_uavs[5] = {
        d->vbgi_raw_uav,
        vbgi_dbg ? d->debug_uav : d->fallback_uav,
        d->vbgi_far_cand_pos.uav[0], d->vbgi_far_cand_rad.uav[0], d->vbgi_far_cand_nrm.uav[0]};
    reshade::api::descriptor_table_update u[4] = {
        {{}, 0, 0, 1, reshade::api::descriptor_type::sampler, &d->point_clamp_sampler},
        {{}, 0, 0, 1, reshade::api::descriptor_type::constant_buffer, &d->captured_scene_cbv_view},
        {{}, 0, 0, 5, reshade::api::descriptor_type::texture_shader_resource_view, far_main_srvs},
        {{}, 0, 0, 5, reshade::api::descriptor_type::texture_unordered_access_view, far_main_uavs},
    };
    GtvbaoApplyDescriptors(dev, cl, d->vbgi_far_main_layout, &d->vbgi_far_main_tables, 4, u);
    const auto pc = BuildVbgiFarPushConstants(d, false);
    cl->push_constants(reshade::api::shader_stage::all_compute, d->vbgi_far_main_layout, kGtvbaoPushConstantsLayoutParam, 0, kVbgiFarPushConstantFloats, pc.data());
  }
  cl->dispatch((d->vbgi_width + 7) / 8, (d->vbgi_height + 7) / 8, 1);
  GtvbaoBarrier(cl, d->vbgi_raw_texture, reshade::api::resource_usage::unordered_access, reshade::api::resource_usage::shader_resource);
  falcom_world::rtao::NullComputeSlots(cl, 5, 2, 1);
  if (far_on) {
    namespace rtao = falcom_world::rtao;
    const auto UA = reshade::api::resource_usage::unordered_access;
    const auto SR = reshade::api::resource_usage::shader_resource;
    rtao::NullComputeSlots(cl, 5, 5, 1);
    GtvbaoBarrier(cl, d->vbgi_far_cand_pos.texture[0], UA, SR);
    GtvbaoBarrier(cl, d->vbgi_far_cand_rad.texture[0], UA, SR);
    GtvbaoBarrier(cl, d->vbgi_far_cand_nrm.texture[0], UA, SR);
    const reshade::api::resource_view mrt = d->captured_mrt_normal_srv.handle ? d->captured_mrt_normal_srv : d->fallback_srv;
    // Temporal reuse (M2): merge the candidate with last frame's temporal reservoir (double-buffered, swapped after).
    const bool far_temporal_on = g_vbgi_far_temporal > 0.5f && g_cpuopt_deferred_dispatch < 0.5f;
    bool far_temporal_ran = false;
    if (far_temporal_on) {
      const uint32_t tw = d->vbgi_width, th = d->vbgi_height;
      const bool res_resized = d->vbgi_far_res_pos.width != tw || d->vbgi_far_res_pos.height != th;
      const bool temporal_pso = d->vbgi_far_temporal_pipeline.handle != 0u;
      const bool res_textures = temporal_pso
                                && falcom_world::rtgi::EnsureRtgiTextureSet(dev, &d->vbgi_far_res_pos, reshade::api::format::r32g32b32a32_float, 16u, tw, th)
                                && falcom_world::rtgi::EnsureRtgiTextureSet(dev, &d->vbgi_far_res_rad, reshade::api::format::r16g16b16a16_float, 8u, tw, th)
                                && falcom_world::rtgi::EnsureRtgiTextureSet(dev, &d->vbgi_far_res_nrm, reshade::api::format::r16g16b16a16_float, 8u, tw, th);
      if (!res_textures) {
        CSLog("vbgi", "far temporal skipped: pso=" + std::to_string(temporal_pso) + " textures=" + std::to_string(res_textures), true);
      } else {
        const bool far_motion_ok = UpdateRtMotion(dev, d, true);
        const rtao::ResetReason far_res_reset = rtao::TemporalResetReason(rtao::TemporalFrame{
            .temporal_on = true,
            .was_on = d->vbgi_far_reuse_last,
            .resized = res_resized,
            .frame = d->frame_index,
            .last_frame = d->vbgi_far_reuse_frame,
            .guard = d->dyncube_loadingWipePending || d->frame_index < d->resize_guard_until_frame,
            .scaled = false,
            .params_changed = far_params_changed,
            .motion_ok = far_motion_ok});
        const uint32_t rd = d->vbgi_far_res_index;
        const uint32_t wr = rd ^ 1u;
        dispatch_far(d->vbgi_far_temporal_pipeline,
                     {d->depth_mips_srv, mrt, d->fallback_srv, d->captured_depth_srv, d->vbgi_radiance_srv,
                      far_motion_ok ? d->mb_rtv4_owned_srv : d->fallback_srv,
                      d->vbgi_far_cand_pos.srv[0], d->vbgi_far_cand_rad.srv[0], d->vbgi_far_cand_nrm.srv[0],
                      d->vbgi_far_res_pos.srv[rd], d->vbgi_far_res_rad.srv[rd], d->vbgi_far_res_nrm.srv[rd]},
                     {d->vbgi_far_res_pos.uav[wr], d->vbgi_far_res_rad.uav[wr], d->vbgi_far_res_nrm.uav[wr], d->fallback_uav},
                     far_res_reset == rtao::ResetReason::None, tw, th);
        GtvbaoBarrier(cl, d->vbgi_far_res_pos.texture[wr], UA, SR);
        GtvbaoBarrier(cl, d->vbgi_far_res_rad.texture[wr], UA, SR);
        GtvbaoBarrier(cl, d->vbgi_far_res_nrm.texture[wr], UA, SR);
        d->vbgi_far_res_index = wr;
        far_temporal_ran = true;
      }
    }
    d->vbgi_far_reuse_last = far_temporal_ran;
    d->vbgi_far_reuse_frame = d->frame_index;
    const uint32_t ri = d->vbgi_far_res_index;
    const reshade::api::resource_view in_pos = far_temporal_ran ? d->vbgi_far_res_pos.srv[ri] : d->vbgi_far_cand_pos.srv[0];
    const reshade::api::resource_view in_rad = far_temporal_ran ? d->vbgi_far_res_rad.srv[ri] : d->vbgi_far_cand_rad.srv[0];
    const reshade::api::resource_view in_nrm = far_temporal_ran ? d->vbgi_far_res_nrm.srv[ri] : d->vbgi_far_cand_nrm.srv[0];
    // Spatial reuse (M3): 3 neighbours' temporal reservoirs (or candidates); per frame, read only by the resolve.
    bool far_spatial_ran = false;
    if (g_vbgi_far_spatial > 0.5f) {
      const uint32_t tw = d->vbgi_width, th = d->vbgi_height;
      const bool spatial_pso = d->vbgi_far_spatial_pipeline.handle != 0u;
      const bool sp_textures = spatial_pso
                               && falcom_world::rtgi::EnsureRtgiTextureSet(dev, &d->vbgi_far_sp_pos, reshade::api::format::r32g32b32a32_float, 16u, tw, th)
                               && falcom_world::rtgi::EnsureRtgiTextureSet(dev, &d->vbgi_far_sp_rad, reshade::api::format::r16g16b16a16_float, 8u, tw, th)
                               && falcom_world::rtgi::EnsureRtgiTextureSet(dev, &d->vbgi_far_sp_nrm, reshade::api::format::r16g16b16a16_float, 8u, tw, th);
      if (!sp_textures) {
        CSLog("vbgi", "far spatial skipped: pso=" + std::to_string(spatial_pso) + " textures=" + std::to_string(sp_textures), true);
      } else {
        dispatch_far(d->vbgi_far_spatial_pipeline,
                     {d->depth_mips_srv, mrt, d->fallback_srv, d->fallback_srv, d->fallback_srv, d->fallback_srv,
                      in_pos, in_rad, in_nrm, d->fallback_srv, d->fallback_srv, d->fallback_srv},
                     {d->vbgi_far_sp_pos.uav[0], d->vbgi_far_sp_rad.uav[0], d->vbgi_far_sp_nrm.uav[0], d->fallback_uav},
                     false, tw, th);
        GtvbaoBarrier(cl, d->vbgi_far_sp_pos.texture[0], UA, SR);
        GtvbaoBarrier(cl, d->vbgi_far_sp_rad.texture[0], UA, SR);
        GtvbaoBarrier(cl, d->vbgi_far_sp_nrm.texture[0], UA, SR);
        far_spatial_ran = true;
      }
    }
    dispatch_far(d->vbgi_far_resolve_pipeline,
                 {d->depth_mips_srv, mrt, d->vbgi_raw_srv, d->fallback_srv, d->vbgi_radiance_srv, d->fallback_srv,
                  far_spatial_ran ? d->vbgi_far_sp_pos.srv[0] : in_pos,
                  far_spatial_ran ? d->vbgi_far_sp_rad.srv[0] : in_rad,
                  far_spatial_ran ? d->vbgi_far_sp_nrm.srv[0] : in_nrm,
                  d->fallback_srv, d->fallback_srv, d->fallback_srv},
                 {d->vbgi_far_joint.uav[0], d->vbgi_far_raw.uav[0], d->fallback_uav, vbgi_dbg ? d->debug_uav : d->fallback_uav},
                 false, d->vbgi_width, d->vbgi_height);
    GtvbaoBarrier(cl, d->vbgi_far_joint.texture[0], UA, SR);
    GtvbaoBarrier(cl, d->vbgi_far_raw.texture[0], UA, SR);
    d->vbgi_far_frame = d->frame_index;
  }
  if (vbgi_dbg) {
    d->gtvbao_debug_frame = d->frame_index;
    d->gtvbao_debug_mode_written = (float)VbgiGtvbaoDebugMode();
    GtvbaoBarrier(cl, d->debug_texture, reshade::api::resource_usage::unordered_access, reshade::api::resource_usage::shader_resource);
  }

  // VBGI denoise (M7.5): the shared RTGI chain (temporal, spatial filter, upsample) on the raw GI, into VBGI's own set.
  {
    namespace rtgi = falcom_world::rtgi;
    namespace rtao = falcom_world::rtao;
    // GTVBGI's upsample uses the shared reconstruction modes and runtime-controlled nearest-valid fallback.
    static constexpr rtgi::RtgiPipelineDesc kVbgiUpsamplePipelineDesc = {
        .srv_count = rtgi::kRtgiUpsampleSrvCount, .uav_count = rtgi::kRtgiUpsampleUavCount, .sampler_count = 0u, .push_count = rtgi::kRtgiUpsamplePushCount, .code = __vbgi_upsample, .name = "vbgi upsample"};
    const uint32_t divisor = falcom_world::rtao::RtgiTraceDivisor(g_vbgi_resolution, kVbgiMaxResolutionIndex);
    const uint32_t full_w = d->captured_depth_w, full_h = d->captured_depth_h;
    bool denoise_ok = full_w != 0u && full_h != 0u
                      && d->vbgi_width == rtao::RtgiTraceExtent(full_w, divisor) && d->vbgi_height == rtao::RtgiTraceExtent(full_h, divisor);
    if (!denoise_ok) CSLog("vbgi", "denoise skipped: size mismatch", true);
    if (denoise_ok && !rtgi::EnsureRtgiPipeline(dev, &d->vbgi_denoise.upsample_pipeline, kVbgiUpsamplePipelineDesc)) {
      denoise_ok = false;
      CSLog("vbgi", "denoise skipped: upsample pipeline", true);
    }
    if (denoise_ok && d->vbgi_stats_buffer.handle == 0u) {
      const uint32_t zeros[rtgi::kRtgiStatsCount] = {};
      denoise_ok = falcom_world::bvh::CreatePoolBuffer(dev, zeros, sizeof(zeros), sizeof(uint32_t),
                                                       &d->vbgi_stats_buffer, &d->vbgi_stats_uav,
                                                       reshade::api::resource_usage::unordered_access | reshade::api::resource_usage::copy_source);
      if (!denoise_ok) CSLog("vbgi", "denoise skipped: stats buffer", true);
    }
    if (denoise_ok) {
      const bool temporal_on = g_vbgi_temporal_enabled > 0.5f && g_cpuopt_deferred_dispatch < 0.5f;
      const bool far_only = g_vbgi_temporal_far_only > 0.5f && g_vbgi_temporal_enabled < 0.5f;
      const bool far_temporal_on = far_only && far_on && g_cpuopt_deferred_dispatch < 0.5f;
      const bool motion_ok = (temporal_on || far_temporal_on) && UpdateRtMotion(dev, d, true);
      const std::array<float, 21> params = {g_vbgi_quality_level, g_vbgi_resolution, g_vbgi_radius, g_vbgi_bitmask_thickness,
                                            g_vbgi_auto_thickness, g_vbgi_base_thickness,
                                            g_vbgi_depth_mip_offset, g_vbgi_noise_type, g_vbgi_mrt_normal, g_vbgi_ambient, g_vbgi_lights, g_vbgi_sun,
                                            g_vbgi_emissive, g_vbgi_backface_mode, g_vbgi_history_weight, g_vbgi_depth_rejection, g_vbgi_normal_rejection,
                                            shader_injection.vbgi_multibounce, g_vbgi_multibounce_strength, g_vbgi_svgf_history_length, g_vbgi_svgf_radius_scale};
      const bool params_changed = d->vbgi_prev_params_valid && !(params == d->vbgi_prev_params);
      d->vbgi_prev_params = params;
      d->vbgi_prev_params_valid = true;
      const rtao::ResetReason reset = rtao::TemporalResetReason(rtao::TemporalFrame{
          .temporal_on = temporal_on,
          .was_on = d->vbgi_temporal_last,
          .resized = d->vbgi_denoise.history_color.width != 0u
                     && (d->vbgi_denoise.history_color.width != (g_vbgi_full_res_denoise > 0.5f ? full_w : d->vbgi_width)
                         || d->vbgi_denoise.history_color.height != (g_vbgi_full_res_denoise > 0.5f ? full_h : d->vbgi_height)
                         || d->vbgi_denoise.output.width != full_w || d->vbgi_denoise.output.height != full_h),
          .frame = d->frame_index,
          .last_frame = d->vbgi_last_frame,
          .guard = d->dyncube_loadingWipePending || d->frame_index < d->resize_guard_until_frame,
          .scaled = false,
          .params_changed = params_changed || far_toggled,
          .motion_ok = motion_ok});
      const bool vbgi_conf = VbgiGtvbaoDebugMode() == 34 && d->debug_uav.handle != 0u;
      bool far_color_ran = false;
      if (far_temporal_on && motion_ok && d->vbgi_far_compose_pipeline.handle != 0u) {
        auto& far_color = d->vbgi_far_color_denoise;
        const std::array<float, 6> far_color_params = {g_vbgi_far_intensity, g_vbgi_far_candidates, g_vbgi_far_temporal,
                                                       g_vbgi_far_spatial, g_vbgi_far_spatial_neighbors, g_vbgi_far_spatial_radius};
        const bool far_color_params_changed = d->vbgi_far_color_prev_params_valid && far_color_params != d->vbgi_far_color_prev_params;
        const rtao::ResetReason far_reset = rtao::TemporalResetReason(rtao::TemporalFrame{
            .temporal_on = true,
            .was_on = d->vbgi_far_color_last,
            .resized = far_color.history_color.width != 0u
                       && (far_color.history_color.width != d->vbgi_width || far_color.history_color.height != d->vbgi_height
                           || d->vbgi_far_color_full_width != full_w || d->vbgi_far_color_full_height != full_h),
            .frame = d->frame_index,
            .last_frame = d->vbgi_far_color_frame,
            .guard = d->dyncube_loadingWipePending || d->frame_index < d->resize_guard_until_frame,
            .scaled = false,
            .params_changed = params_changed || far_toggled || far_params_changed || far_color_params_changed,
            .motion_ok = motion_ok});
        const rtgi::RtgiDenoiseReady far_ready = rtgi::EnsureRtgiDenoise(dev, &far_color,
                                                                         {.output_format = reshade::api::format::r16g16b16a16_float, .output_bytes_per_pixel = 8u, .trace_width = d->vbgi_width, .trace_height = d->vbgi_height, .full_width = full_w, .full_height = full_h, .temporal = true, .spatial = false, .log_prefix = "[vbgi-far-color]", .history_only = true, .bypass_svgf = falcom_world::rr::RrOwnsDenoise()});
        if (far_ready.ok && far_ready.temporal) {
          std::vector<reshade::api::descriptor_table_update> far_nulls;
          if (vbgi_conf) GtvbaoBarrier(cl, d->debug_texture, reshade::api::resource_usage::shader_resource, reshade::api::resource_usage::unordered_access);
          rtgi::DispatchRtgiDenoise(dev, cl, &far_color,
                                    {.raw_view = d->vbgi_far_raw.srv[0],
                                     .depth_view = d->captured_depth_srv,
                                     .mrt_normal_view = d->captured_mrt_normal_srv,
                                     .scene_cbv_view = d->captured_scene_cbv_view,
                                     .motion_view = d->mb_rtv4_owned_srv,
                                     .albedo_view = reshade::api::resource_view{0u},
                                     .stats_uav = reshade::api::resource_view{0u},
                                     .debug_uav = vbgi_conf ? d->debug_uav : reshade::api::resource_view{0u},
                                     .debug_texture = d->debug_texture,
                                     .trace_width = d->vbgi_width,
                                     .trace_height = d->vbgi_height,
                                     .full_width = full_w,
                                     .full_height = full_h,
                                     .divisor = static_cast<float>(divisor),
                                     .debug_mode = vbgi_conf ? 5 : 0,
                                     .svgf = {g_vbgi_history_weight, g_vbgi_depth_rejection, g_vbgi_normal_rejection,
                                              far_reset == rtao::ResetReason::None, g_vbgi_svgf_iterations, g_vbgi_svgf_luminance_edge_stop,
                                              g_vbgi_svgf_depth_sigma, g_vbgi_svgf_normal_power, false, g_vbgi_svgf_history_length, g_vbgi_svgf_radius_scale},
                                     .luminance_weights = rtgi::kSvgfLuminanceRgb,
                                     .upsample_intensity = 1.f,
                                     .upsample_char_intensity = 1.f,
                                     .upsample_saturation = 1.f,
                                     .history_only = true},
                                    far_ready, &far_nulls);
          dev->update_descriptor_tables(static_cast<uint32_t>(far_nulls.size()), far_nulls.data());
          GtvbaoBarrier(cl, d->vbgi_far_joint.texture[0], reshade::api::resource_usage::shader_resource, reshade::api::resource_usage::unordered_access);
          dispatch_far(d->vbgi_far_compose_pipeline,
                       {d->depth_mips_srv, d->captured_mrt_normal_srv, d->vbgi_raw_srv, d->fallback_srv, d->vbgi_radiance_srv,
                        d->fallback_srv, far_color.history_color.srv[far_color.history_index], d->fallback_srv, d->fallback_srv,
                        d->fallback_srv, d->fallback_srv, d->fallback_srv},
                       {d->vbgi_far_joint.uav[0], d->fallback_uav, d->fallback_uav, d->fallback_uav},
                       false, d->vbgi_width, d->vbgi_height);
          GtvbaoBarrier(cl, d->vbgi_far_joint.texture[0], reshade::api::resource_usage::unordered_access, reshade::api::resource_usage::shader_resource);
          d->vbgi_far_color_prev_params = far_color_params;
          d->vbgi_far_color_prev_params_valid = true;
          far_color_ran = true;
        }
      }
      if (!far_only && d->vbgi_far_color_denoise.history_color.texture[0].handle != 0u) {
        rtgi::ReleaseRtgiDenoiseTextures(dev, &d->vbgi_far_color_denoise);
        d->vbgi_far_color_prev_params_valid = false;
      }
      if (falcom_world::rr::RrOwnsDenoise()) d->vbgi_far_color_prev_params_valid = false;
      d->vbgi_far_color_last = far_color_ran;
      if (far_color_ran) {
        d->vbgi_far_color_frame = d->frame_index;
        d->vbgi_far_color_full_width = full_w;
        d->vbgi_far_color_full_height = full_h;
      }
      const rtgi::RtgiDenoiseReady ready = rtgi::EnsureRtgiDenoise(dev, &d->vbgi_denoise,
                                                                   {.output_format = reshade::api::format::r11g11b10_float,
                                                                    .output_bytes_per_pixel = 4u,
                                                                    .trace_width = d->vbgi_width,
                                                                    .trace_height = d->vbgi_height,
                                                                    .full_width = full_w,
                                                                    .full_height = full_h,
                                                                    .temporal = temporal_on && motion_ok,
                                                                    .spatial = g_vbgi_spatial_enabled > 0.5f,
                                                                    .log_prefix = "[vbgi]",
                                                                    .full_resolution = g_vbgi_full_res_denoise > 0.5f,
                                                                    .nearest_fallback = true,
                                                                    .bypass_svgf = falcom_world::rr::RrOwnsDenoise()});
      if (!ready.ok) {
        CSLog("vbgi", "denoise skipped: chain not ready", true);
      } else {
        std::vector<reshade::api::descriptor_table_update> table_nulls;
        // Temporal Confidence (GTVBAO debug mode 34): the chain writes the debug view 5 (history confidence) into the
        // GTVBAO debug texture; the texture is in shader-read state after the gather, so it goes back to unordered access.
        const bool joint_conf = vbgi_conf && !far_only;
        if (joint_conf) GtvbaoBarrier(cl, d->debug_texture, reshade::api::resource_usage::shader_resource, reshade::api::resource_usage::unordered_access);
        rtgi::DispatchRtgiDenoise(dev, cl, &d->vbgi_denoise,
                                  {.raw_view = far_on ? d->vbgi_far_joint.srv[0] : d->vbgi_raw_srv,
                                   .depth_view = d->captured_depth_srv,
                                   .mrt_normal_view = d->captured_mrt_normal_srv,
                                   .scene_cbv_view = d->captured_scene_cbv_view,
                                   .motion_view = d->mb_rtv4_owned_srv,
                                   .albedo_view = reshade::api::resource_view{0u},
                                   .stats_uav = d->vbgi_stats_uav,
                                   .debug_uav = joint_conf ? d->debug_uav : reshade::api::resource_view{0u},
                                   .debug_texture = d->debug_texture,
                                   .trace_width = d->vbgi_width,
                                   .trace_height = d->vbgi_height,
                                   .full_width = full_w,
                                   .full_height = full_h,
                                   .divisor = static_cast<float>(divisor),
                                   .debug_mode = joint_conf ? 5 : 0,
                                   .svgf = {g_vbgi_history_weight, g_vbgi_depth_rejection, g_vbgi_normal_rejection,
                                            reset == rtao::ResetReason::None, g_vbgi_svgf_iterations,
                                            g_vbgi_svgf_luminance_edge_stop, g_vbgi_svgf_depth_sigma, g_vbgi_svgf_normal_power, g_vbgi_full_res_denoise > 0.5f, g_vbgi_svgf_history_length, g_vbgi_svgf_radius_scale},
                                   .luminance_weights = rtgi::kSvgfLuminanceRgb,
                                   .upsample_intensity = 1.f,
                                   .upsample_char_intensity = 1.f,
                                   .upsample_saturation = 1.f,
                                   .upsample = {.algorithm = g_vbgi_upscale_algorithm,
                                                .depth_plane_sigma = g_vbgi_upscale_plane_sigma,
                                                .normal_edge_power = g_vbgi_upscale_normal_power,
                                                .bilinear_weight_sharpness = g_vbgi_upscale_bilinear_sharpness,
                                                .nearest_valid_fallback = g_vbgi_upscale_nearest_fallback}},
                                  ready, &table_nulls);
        dev->update_descriptor_tables(static_cast<uint32_t>(table_nulls.size()), table_nulls.data());
        d->vbgi_temporal_last = temporal_on && ready.temporal;
        d->vbgi_last_frame = d->frame_index;
        d->vbgi_gi_frame = d->frame_index;
      }
    }
  }

  static bool s_logged = false;
  if (!s_logged) {
    s_logged = true;
    CSLog("vbgi", "gather dispatched " + std::to_string(d->vbgi_width) + "x" + std::to_string(d->vbgi_height));
  }
  static bool s_far_logged = false;
  if (far_on && !s_far_logged) {
    s_far_logged = true;
    CSLog("vbgi", "far dispatched " + std::to_string(d->vbgi_width) + "x" + std::to_string(d->vbgi_height));
  }
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
      renodx::utils::state::use_snapshot = true;
      reshade::register_event<reshade::addon_event::init_device>(OnInitDevice);
      reshade::register_event<reshade::addon_event::destroy_device>(OnDestroyDevice);
      reshade::register_event<reshade::addon_event::init_swapchain>(OnInitSwapchain);
      reshade::register_event<reshade::addon_event::destroy_swapchain>(OnDestroySwapchain);
      reshade::register_event<reshade::addon_event::present>(OnPresent);
      reshade::register_event<reshade::addon_event::map_buffer_region>(OnKaiHeroMapBuffer);
      reshade::register_event<reshade::addon_event::unmap_buffer_region>(OnKaiHeroUnmapBuffer);
      reshade::register_event<reshade::addon_event::bind_descriptor_tables>(OnBindDescriptorTables);
      reshade::register_event<reshade::addon_event::push_descriptors>(OnPushDescriptorsCapture);
      reshade::register_event<reshade::addon_event::reset_command_list>(ResetRtxdiSceneBinding);
      reshade::register_event<reshade::addon_event::destroy_command_list>(ResetRtxdiSceneBinding);
      reshade::register_event<reshade::addon_event::bind_render_targets_and_depth_stencil>(OnBindRenderTargetsRCAS);
      reshade::register_event<reshade::addon_event::destroy_resource>(OnDestroyResource);
      reshade::register_event<reshade::addon_event::destroy_resource_view>(OnDestroyResourceView);
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
      reshade::unregister_event<reshade::addon_event::map_buffer_region>(OnKaiHeroMapBuffer);
      reshade::unregister_event<reshade::addon_event::unmap_buffer_region>(OnKaiHeroUnmapBuffer);
      reshade::unregister_event<reshade::addon_event::bind_descriptor_tables>(OnBindDescriptorTables);
      reshade::unregister_event<reshade::addon_event::push_descriptors>(OnPushDescriptorsCapture);
      reshade::unregister_event<reshade::addon_event::reset_command_list>(ResetRtxdiSceneBinding);
      reshade::unregister_event<reshade::addon_event::destroy_command_list>(ResetRtxdiSceneBinding);
      reshade::unregister_event<reshade::addon_event::bind_render_targets_and_depth_stencil>(OnBindRenderTargetsRCAS);
      reshade::unregister_event<reshade::addon_event::destroy_resource>(OnDestroyResource);
      reshade::unregister_event<reshade::addon_event::destroy_resource_view>(OnDestroyResourceView);
      reshade::unregister_event<reshade::addon_event::destroy_pipeline>(OnDestroyPipelineCapture);
      reshade::unregister_addon(h_module);
      break;
  }
  if (fdw_reason == DLL_PROCESS_ATTACH) {
    falcom_world::AddSvgfRows(&settings, IsAdvancedSettingsMode, IsAdvancedSettingsMode, {.prefix = "Gtvbao", .temporal_section = "GTVBAO Temporal Accumulation", .temporal_enabled_tooltip = "Reprojects the previous frame's GTVBAO AO with the game motion (RTV4 target) and blends it in after validation (SVGF). Runs on Sora 1st, Sora 2nd and Kai only; off while Deferred Dispatch is On. Resets on camera cuts, frame gaps and parameter changes.", .temporal_enabled = &g_gtvbao_temporal_enabled, .full_res_denoise = &g_gtvbao_full_res_denoise, .history_weight = &g_gtvbao_history_weight, .depth_rejection = &g_gtvbao_depth_rejection, .normal_rejection = &g_gtvbao_normal_rejection, .spatial_section = "GTVBAO Spatial Filter", .spatial_enabled_tooltip = "Denoises the GTVBAO AO with the SVGF a-trous filter (depth, normal and AO edge stops). Off = no filter.", .spatial_enabled = &g_gtvbao_svgf_spatial_enabled, .iterations = &g_gtvbao_svgf_iterations, .luminance_edge_stop = &g_gtvbao_svgf_luminance_edge_stop, .depth_sigma = &g_gtvbao_svgf_depth_sigma, .normal_power = &g_gtvbao_svgf_normal_power, .half_res_iterations = &g_gtvbao_svgf_half_res_iterations, .history_length = &g_gtvbao_svgf_history_length, .radius_scale = &g_gtvbao_svgf_radius_scale, .half_res_radius_scale = &g_gtvbao_svgf_half_res_radius_scale, .default_temporal_enabled = 0.f, .default_full_res_denoise = 0.f, .default_history_weight = 0.8f, .default_history_length = 128.f, .default_depth_rejection = 0.200f, .default_normal_rejection = 0.f, .default_spatial_enabled = 1.f, .default_iterations = 1.f, .default_half_res_iterations = 1.f, .default_luminance_edge_stop = 16.f, .default_depth_sigma = 0.10f});
    falcom_world::AddSvgfRows(&settings, []() { return VbgiRowVisible(); }, IsAdvancedSettingsMode, {.prefix = "Vbgi", .temporal_section = "GTVBGI Temporal Accumulation", .temporal_enabled_tooltip = "Reprojects the previous frame's GTVBGI with the game motion and blends it in after validation. Runs only with inline dispatch.", .temporal_enabled = &g_vbgi_temporal_enabled, .full_res_denoise = &g_vbgi_full_res_denoise, .temporal_far_only = &g_vbgi_temporal_far_only, .history_weight = &g_vbgi_history_weight, .depth_rejection = &g_vbgi_depth_rejection, .normal_rejection = &g_vbgi_normal_rejection, .spatial_section = "GTVBGI Spatial Filter", .spatial_enabled_tooltip = "Denoises the final GTVBGI output with edge-aware weights (depth plane and normal). Off = no filter.", .spatial_enabled = &g_vbgi_spatial_enabled, .iterations = &g_vbgi_svgf_iterations, .luminance_edge_stop = &g_vbgi_svgf_luminance_edge_stop, .depth_sigma = &g_vbgi_svgf_depth_sigma, .normal_power = &g_vbgi_svgf_normal_power, .history_length = &g_vbgi_svgf_history_length, .radius_scale = &g_vbgi_svgf_radius_scale, .default_temporal_enabled = 1.f, .default_temporal_far_only = 1.f, .default_history_weight = 0.99f, .default_history_length = 128.f, .default_depth_rejection = 0.2f, .default_normal_rejection = 0.f, .default_spatial_enabled = 1.f, .default_iterations = 5.f, .default_luminance_edge_stop = 16.f, .default_depth_sigma = 8.f, .default_normal_power = 128.f, .default_radius_scale = 4.f});
    falcom_world::AddSettings(&settings, IsSora2nd(), IsAdvancedSettingsMode);
  }
  falcom_world::Use(fdw_reason, IsSora2nd());
  falcom_ui::Use(fdw_reason, &settings);
  if (fdw_reason == DLL_PROCESS_ATTACH) {
    falcom_world::bvh::SyncBvhSettings();
  }
  if (fdw_reason == DLL_PROCESS_DETACH) {
    renodx::utils::pipeline::UnregisterOnInitCallback(OnInitPipelineCapture);
  }
  renodx::mods::shader::Use(fdw_reason, custom_shaders, &shader_injection);
  if (fdw_reason == DLL_PROCESS_ATTACH) {
    renodx::utils::pipeline::RegisterOnInitCallback(OnInitPipelineCapture);
  }
  return TRUE;
}
