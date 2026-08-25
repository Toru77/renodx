/*
 * Copyright (C) 2026
 * SPDX-License-Identifier: MIT
 */

#define ImTextureID ImU64

#define DEBUG_LEVEL_0

#include <deps/imgui/imgui.h>
#include <include/reshade.hpp>

#include <embed/shaders.h>

#include <array>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <shared_mutex>
#include <sstream>
#include <vector>
#include <Windows.h>

#include "../../mods/shader.hpp"
#include "../../utils/descriptor.hpp"
#include "../../utils/pipeline_layout.hpp"
#include "../../utils/resource.hpp"
#include "../../utils/settings.hpp"
#include "../../utils/shader.hpp"
#include "../../utils/state.hpp"
#include "../../utils/swapchain.hpp"
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
  .char_shadow_mode = 2.f,
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
  .gtvbao_denoiser_type = 0.f,
  .gtvbao_temporal_blend = 0.85f,
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
  // ── Kai-specific defaults ──
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
  .gtvbao_cdf_enabled = 0.f,
  .gtvbao_cosine_enabled = 0.f,
  .gtvbao_cosine_mode = 2.f,
  .gtvbao_thickness_enabled = 0.f,
  .gtvbao_poisson_samples = 8.f,
  .gtvbao_poisson_luma_phi = 5.f,
  .gtvbao_poisson_depth_phi = 5.f,
  .gtvbao_poisson_normal_phi = 5.f,
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
  .gtvbao_temporal_normal_reject = 0.5f,
  .gtvbao_ghost_clamp = 1.5f,
  .gtvbao_atrous_enabled = 0.f,
  .gtvbao_atrous_depth_sigma = 1.f,
  .gtvbao_atrous_normal_sigma = 32.f,
  // ── Sora 2nd Custom SSR ──
  .ssr_max_ray_distance = 300.f,
  .ssr_thickness = 0.15f,
  .ssr_roughness_threshold = 0.6f,
  .ssr_mirror_bias = 0.25f,
  .ssr_intensity = 1.f,
  .ssr_denoise_radius = 4.f,
  .ssr_denoise_taps = 8.f,
  .ssr_debug_view = 0.f,
  .ssr_debug_mip = 2.f,
  .ssr_deferred_dispatch = 0.f,
  .ssr_frame_skip = 0.f,
  .ssr_custom_bound = 0.f,
  // ── Phase 2.1 traversal diagnostics ──
  .ssr_bypass_validation = 0.f,
  .ssr_forced_ray_mode = 0.f,
  .ssr_normal_convention = 1.f,   // canonical (Phase 2.1 data locked mul(n,view_g))
  // ── Phase 2.2 self-hit sweep ──
  .ssr_self_hit_threshold = 2.f,
  // ── Phase 2.3 backface A/B ──
  .ssr_backface_gate = 1.f,
  // ── Phase 2.4 initial-advance displacement ──
  .ssr_initial_advance_bias = 0.f,
  // ── Phase 2.8 sweep ──
  .ssr_thickness_gate = 1.f,
  // ── Phase 2.9/3.0 production baseline (Perpendicular @ 0.15) ──
  .ssr_thickness_mode = 1.f,
  // ── Phase 3 stochastic SSR ──
  .ssr_stochastic = 1.f,
  .ssr_apply = 0.f,
  .ssr_apply_gain = 1.f,
  .ssr_diagnostics = 1.f,
  .ssr_eligibility_mode = 1.f,
  // ── Phase 4 spatial reconstruction ──
  .ssr_resolve_radius = 8.f,
  .ssr_depth_sigma = 0.1f,
  .ssr_normal_sigma = 32.f,
  .ssr_rough_sigma = 0.15f,
  .ssr_same_surface_reject = 0.f,
  .ssr_plane_delta_threshold = 0.1f,
  .ssr_roughness_alpha_blend = 0.f,
  .ssr_ray_count = 1.f,
  .ssr_rough_interp = 0.f,
  .ssr_probe_pixel_x = 960.f,
  .ssr_probe_pixel_y = 540.f,
  .ssr_probe_auto = 1.f,
  .ssr_resolve_enable = 0.f,
  .ssr_max_traversal_steps = 64.f,
  .ssr_log_tracestats = 0.f,
  .ssr_log_probes = 0.f,
  .ssr_log_resolve = 0.f,
  .ssr_log_ngx = 0.f,
  .ssr_log_config = 0.f,
  .ssr_log_init = 0.f,
};

// ═══════════ GTVBAO Backend — constants, types, fwd decls ═══════════

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
constexpr uint32_t kLightingMrtNormalRegister = 1u;  // t1 = mrtTexture0 (g-buffer normals)
constexpr uint64_t kGTVBAOStartupGuardFrames = 8u;
constexpr uint64_t kGTVBAOResizeGuardFrames = 4u;
constexpr uint64_t kSceneCbMinimumBytes = 95u * 16u;

// ── Custom SSR (Sora 2nd) ──
constexpr uint32_t kLightingSSRCustomRegister = 25u;  // t25 = ssrCustomTexture
constexpr uint32_t kSSRHizMipLevels = 8u;             // Hi-Z chain mips 0..7
constexpr uint32_t SSR_RAD_MIP_LEVELS = 8u;           // R3: radiance pyramid mips 0..7
constexpr uint32_t kSSRPushConstantCount = 42u;       // push constants at b13
constexpr uint32_t kSSRStatsWidth = 8u;               // atomic counter texture
constexpr uint32_t kSSRStatsHeight = 29u;

// ── GTVBAO normal tuning globals (separate from ShaderInjectData) ──
static float g_gtvbao_normal_input_mode     = 1.f;
static float g_gtvbao_normal_influence      = 1.f;
static float g_gtvbao_normal_z_preservation = 1.f;
static float g_gtvbao_normal_depth_blend    = 0.70f;
static float g_gtvbao_normal_sharpness      = 0.75f;
static float g_gtvbao_normal_edge_rejection = 0.5f;
static float g_gtvbao_normal_detail_response = 0.75f;
static float g_gtvbao_normal_max_darkening  = 0.50f;
static float g_gtvbao_normal_darkening_mode = 0.f;
static float g_gtvbao_normal_transform_mode = 0.f; // 0=view_g, 1=viewInv_g, 2=passthrough

// ── VBGI globals removed — now controlled via ShaderInjectData fields (shared.h). ──
// vbgi_enabled, vbgi_intensity, vbgi_saturation, vbgi_multibounce, vbgi_gi_power
// are all part of shader_injection and pushed via BuildGTVBAOPushConstants.
static float g_vbgi_light_exposure = 0.05f;  // HDR light buffer exposure scale (lower = dimmer GI)

// ── IS-FAST noise ──
static float g_isfast_enabled       = 0.f;
static float g_isfast_strength      = 1.f;
static float g_isfast_debug_logging = 0.f;
static float g_isfast_spatial_scale = 1.f;
static float g_isfast_temporal_speed = 1.f;
static float g_isfast_seed_offset   = 0.f;

// ── Settings visibility ──
static float g_settings_mode            = 0.f;   // 0=Basic, 1=Advanced
static bool IsAdvancedSettingsMode() { return g_settings_mode >= 0.5f; }

// ── Kai detection ──
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

// ── Sora 2nd detection ──
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

// ── Daybreak 2 detection ──
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

// ── Lighting shader identification (Sora + Kai) ──
static bool IsLightingShader(uint32_t hash) {
  return hash == 0xFDAAF80Eu    // Sora lighting
      || hash == 0xCA3D8596u    // Sora 2nd lighting
      || hash == 0x0CDCB258u    // Kyoto lighting
      || hash == 0x430ED091u    // Kai lighting
      || hash == 0xF6C55E5Fu;   // Kai lighting soft
}

// ── CPU optimization toggles ──
static float g_gtvbao_frame_skip         = 0.f;  // per-component frame skip (0=off, 1=every 2nd, …)
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

  // 1×1 white fallback — always valid, returned when GTVBAO is off / not ready.
  reshade::api::resource fallback_texture = {};
  reshade::api::resource_view fallback_srv = {};

  reshade::api::sampler point_clamp_sampler = {};

  // Resolution-change guard.
  uint32_t last_created_game_width = 0u;
  uint32_t last_created_game_height = 0u;

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
  // ── À-trous wavelet spatial filter (R3) ──
  reshade::api::pipeline_layout atrous_layout = {};
  reshade::api::pipeline atrous_pipeline = {};
  GTVBAODescriptorTableSet atrous_tables = {};
  // Normal pre-decode pass (à-trous perf): decoded MRT normals, RGBA16F
  reshade::api::pipeline_layout normal_prep_layout = {};
  reshade::api::pipeline normal_prep_pipeline = {};
  GTVBAODescriptorTableSet normal_prep_tables = {};
  reshade::api::resource normal_prep_texture = {};
  reshade::api::resource_view normal_prep_srv = {};
  reshade::api::resource_view normal_prep_uav = {};

  // Descriptor tables — pre-allocated per pass.
  GTVBAODescriptorTableSet prefilter_tables = {};
  GTVBAODescriptorTableSet main_tables = {};
  GTVBAODescriptorTableSet denoise_tables = {};

  reshade::api::resource_view captured_depth_srv = {};
  reshade::api::resource_view captured_ssao_srv = {};
  reshade::api::resource_view captured_mrt_normal_srv = {};
  reshade::api::resource_view captured_color_srv = {};   // t0 — lighting input color texture
  reshade::api::resource_view captured_scene_cbv_view = {};  // push_descriptors passes CBV as resource_view
  reshade::api::buffer_range captured_scene_cbv = {};
  bool captured_scene_cbv_valid = false;
  uint64_t captured_scene_cbv_frame = UINT64_MAX;
  bool resources_created = false;
  uint64_t frame_index = 0u;
  uint64_t resize_guard_until_frame = 0u;

  // Deferred dispatch snapshots (kai-style): captured at lighting draw, used at present.
  reshade::api::resource_view deferred_depth_srv = {};
  reshade::api::resource_view deferred_ssao_srv = {};
  reshade::api::resource_view deferred_mrt_normal_srv = {};
  reshade::api::resource_view deferred_scene_cbv_view = {};
  reshade::api::buffer_range deferred_scene_cbv = {};
  bool deferred_scene_cbv_valid = false;
  uint64_t deferred_scene_cbv_frame = UINT64_MAX;
  bool deferred_pending = false;

  // ── GI resources (now integrated — no separate VBGI pipeline) ──
  reshade::api::resource vbgi_output_texture = {};
  reshade::api::resource_view vbgi_output_srv = {};
  reshade::api::resource_view vbgi_output_uav = {};
  reshade::api::resource vbgi_denoised_texture = {};
  reshade::api::resource_view vbgi_denoised_srv = {};
  reshade::api::resource_view vbgi_denoised_uav = {};
  reshade::api::resource captured_light_buffer_texture = {};
  reshade::api::resource_view captured_light_buffer_srv = {};
  bool captured_light_buffer_valid = false;   // true after first frame's capture
  // ── Multi-bounce accumulation (HDR light buffer + previous GI) ──
  reshade::api::resource multibounce_texture = {};
  reshade::api::resource_view multibounce_srv = {};
  reshade::api::resource_view multibounce_uav = {};
  reshade::api::pipeline multibounce_pipeline = {};
  reshade::api::pipeline_layout multibounce_layout = {};
  GTVBAODescriptorTableSet multibounce_tables = {};
  bool vbgi_denoised_valid = false;            // true after first denoise completes
  reshade::api::resource_view fallback_uav = {};  // 1x1 UAV fallback
  // ── Foliage mask (quarter-res R8_UINT, pre-pass) ──
  reshade::api::resource foliage_mask_texture = {};
  reshade::api::resource_view foliage_mask_srv = {};
  reshade::api::resource_view foliage_mask_uav = {};
  reshade::api::pipeline_layout foliage_mask_layout = {};
  reshade::api::pipeline foliage_mask_pipeline = {};
  GTVBAODescriptorTableSet foliage_mask_tables = {};
  // ── Debug UAV (bitmask debug views 6-8) ──
  reshade::api::resource debug_texture = {};
  reshade::api::resource_view debug_srv = {};
  reshade::api::resource_view debug_uav = {};
  // ── IS-FAST noise ──
  reshade::api::resource isfast_noise_texture = {};
  reshade::api::resource_view isfast_noise_srv = {};
  reshade::api::sampler isfast_sampler = {};
  bool isfast_texture_loaded = false;
  bool isfast_texture_attempted = false;  // only try DDS load once
  // ── Phase 3 stochastic SSR captures/resources ──
  // Phase 3 stochastic SSR captures/resources
  reshade::api::resource_view captured_mrt_material_srv = {};    // Sora t3 (material idx)
  reshade::api::resource_view captured_mrt_spec_srv = {};        // Sora t2 (F0/specular)
  reshade::api::resource_view captured_deferred_params_srv = {}; // t6 StructuredBuffer
  reshade::api::resource ssr_radiance_copy_texture = {};         // lazy BackBuffer capture
  reshade::api::resource_view ssr_radiance_copy_srv = {};
  bool ssr_radiance_copy_created = false;
  bool ssr_radiance_verify_done = false;                         // one-shot content readback
  reshade::api::resource ssr_radiance_verify_stage = {};         // 4x4 readback
  reshade::api::sampler ssr_linear_sampler = {};
  // Phase 4 spatial reconstruction output
  reshade::api::resource ssr_output_texture = {};                // RGBA16F WxH
  reshade::api::resource_view ssr_output_srv = {};
  reshade::api::resource_view ssr_output_uav = {};
  reshade::api::pipeline_layout ssr_resolve_layout = {};
  reshade::api::pipeline ssr_resolve_pipeline = {};
  GTVBAODescriptorTableSet ssr_resolve_tables = {};
  bool vbgi_bound = false;
  bool foliage_drawn_this_frame = false;  // set by foliage shader on_draw, reset per frame
  // ── GTVBAO: which ao_term buffer holds the latest final AO result ──
  bool gtvbao_final_in_b = true;

  // CPU optimization tracking
  uint64_t last_bound_pipeline_handle = 0u;
  uint64_t last_srv0_handle = 0u;
  uint64_t last_srv1_handle = 0u;
  uint64_t last_uav0_handle = 0u;
  uint64_t last_cbv_handle = 0u;
  uint64_t last_sampler_handle = 0u;

  // ── Custom SSR (Sora 2nd) ──
  reshade::api::resource ssr_hiz_texture = {};   // R32F chain, mips 0..kSSRHizMipLevels-1
  reshade::api::resource_view ssr_hiz_srv = {};  // full-chain SRV
  std::array<reshade::api::resource_view, kSSRHizMipLevels> ssr_hiz_uavs = {};
  reshade::api::resource ssr_hiz_scratch_texture = {};      // full-res R32F, reduce input (SRV only)
  reshade::api::resource_view ssr_hiz_scratch_srv = {};
  reshade::api::resource ssr_debug_texture = {};            // RGBA8 debug output
  reshade::api::resource_view ssr_debug_srv = {};
  reshade::api::resource_view ssr_debug_uav = {};
  // Phase 2: per-pixel trace result — rgb = debug payload, a = confidence.
  reshade::api::resource ssr_ray_result_texture = {};       // RGBA16F WxH
  reshade::api::resource ssr_ray_meta_texture = {};         // R2C: RGBA16F pdf/dist/dir
  reshade::api::resource_view ssr_ray_meta_srv = {};
  reshade::api::resource_view ssr_ray_meta_uav = {};
  // Phase R3: filtered radiance pyramid (RGBA16F, 8 mips) + scratch.
  reshade::api::resource ssr_rad_pyr_texture = {};
  reshade::api::resource_view ssr_rad_pyr_srv = {};
  reshade::api::resource_view ssr_rad_pyr_uavs[8] = {};   // mips 0..7
  reshade::api::resource_view ssr_rad_scratch_srv = {};
  reshade::api::resource ssr_rad_scratch_texture = {};
  reshade::api::pipeline ssr_rad_base_pipeline = {};
  reshade::api::pipeline ssr_rad_reduce_pipeline = {};
  bool ssr_rad_created = false;
  bool ssr_rad_pyr_valid = false;
  bool ssr_rad_pipelines_ok = false;
  bool ssr_rad_pipelines_diag = false;
  // Phase R3-MV: DLSS motion vector capture.
  reshade::api::resource_view captured_motion_srv = {};
  float captured_mv_scale_x = 0.f;
  float captured_mv_scale_y = 0.f;
  bool motion_captured = false;
  reshade::api::resource_view ssr_ray_result_srv = {};
  reshade::api::resource_view ssr_ray_result_uav = {};
  // Phase 2.1: atomic funnel counters (8x4 R32_UINT) + readback staging.
  reshade::api::resource ssr_stats_texture = {};
  reshade::api::resource_view ssr_stats_uav = {};
  reshade::api::resource ssr_stats_stage = {};
  // Phase 3.Fix19: resolve dispatch state + t25 source selection.
  bool ssr_resolved_this_frame = false;
  // Phase 3.Fix16: CompareProbe auto-selection staging + frozen pixel state.
  reshade::api::resource ssr_probe_stage = {};
  bool ssr_probe_has_selection = false;
  float ssr_probe_frozen_x = 960.f;
  float ssr_probe_frozen_y = 540.f;
  // Readback staging for one-shot numeric Hi-Z verification (debug mode 4):
  // chained check of mip1..mip3 against their immediately preceding mip.
  std::array<reshade::api::resource, 4> ssr_verify_stages = {};
  bool ssr_resources_created = false;
  uint32_t last_created_ssr_width = 0u;
  uint32_t last_created_ssr_height = 0u;
  // All SSR passes share an identical signature: 1 SRV in, 1 UAV out.
  reshade::api::pipeline_layout ssr_common_layout = {};
  reshade::api::pipeline ssr_hiz_base_pipeline = {};
  reshade::api::pipeline ssr_hiz_reduce_pipeline = {};
  reshade::api::pipeline ssr_debug_pipeline = {};
  GTVBAODescriptorTableSet ssr_common_tables = {};
  // Phase 2 trace: 3 SRVs (Hi-Z chain, hardware depth, MRT normals) + 2 UAVs.
  reshade::api::pipeline_layout ssr_trace_layout = {};
  reshade::api::pipeline ssr_trace_pipeline = {};
  GTVBAODescriptorTableSet ssr_trace_tables = {};
  bool ssr_stats_done = false;   // one-shot latch for Trace Stats report
  bool ssr_verify_done = false;   // one-shot latch for numeric verification
};

static void CreateGTVBAOResources(reshade::api::device* device, DeviceData* data,
                                   uint32_t gw, uint32_t gh);
static void DestroyGTVBAOResources(reshade::api::device* device, DeviceData* data);
static bool CreateComputePipelinesIfNeeded(reshade::api::device* device, DeviceData* data);
static bool RunGTVBAO(reshade::api::command_list* cmd_list, DeviceData* data);
// ── Custom SSR (Sora 2nd) — Phase 1: Hi-Z pyramid ──
static void CreateSSRResources(reshade::api::device* device, DeviceData* data,
                               uint32_t gw, uint32_t gh);
static void DestroySSRResources(reshade::api::device* device, DeviceData* data);
static bool CreateSSRPipelinesIfNeeded(reshade::api::device* device, DeviceData* data);
static bool RunSSRHiZ(reshade::api::command_list* cmd_list, DeviceData* data);
static bool RunSSRTrace(reshade::api::command_list* cmd_list, DeviceData* data);
static bool RunSSRResolve(reshade::api::command_list* cmd_list, DeviceData* data);
static bool RunSSRRadiancePyramid(reshade::api::command_list* cmd_list, DeviceData* data);
static float SSR_TranslateDebugView(float slider_value);
static bool SSR_IsRawVsResolvedView(float translated_debug);
static bool SSR_IsMotionVectorView(float translated_debug);
static void SSR_LogTraceStats(reshade::api::command_list* cmd_list, DeviceData* data);
static bool LoadISFASTNoiseTexture(reshade::api::device* dev, DeviceData* d);
static void SSR_LogTraceStats(reshade::api::command_list* cmd_list, DeviceData* data);
// ── Phase R3-MV: NGX motion vector capture callback ──
static void OnNGXEvaluateFeature(ID3D11DeviceContext* ctx, const NVSDK_NGX_Parameter* params);
static void InitMotionVectorCapture();
// File-scope MV capture state (native D3D11 pointers).
static ID3D11ShaderResourceView* g_mv_srv = nullptr;
static float g_mv_scale_x = 0.f;
static float g_mv_scale_y = 0.f;
static bool g_mv_logged = false;
// VBGI is now integrated into GTVBAO main pass — no separate RunVBGI needed.
static bool OnBeforeLightingShaderDraw(reshade::api::command_list* cmd_list);
static bool OnBeforeSsaoShaderDraw(reshade::api::command_list* cmd_list);
static bool OnBeforeCharLightingDraw(reshade::api::command_list* cmd_list);
static bool OnBeforeKaiVolFogDraw(reshade::api::command_list* cmd_list);
static void OnPushDescriptorsCapture(reshade::api::command_list* cmd_list,
    reshade::api::shader_stage stages, reshade::api::pipeline_layout layout,
    uint32_t param_index, const reshade::api::descriptor_table_update& update);

// ── IS-FAST sync helpers (sync g_isfast_* globals → shader_injection) ──
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

// ── Shadow draw callbacks (sync IS-FAST + push IS-FAST SRV at t3) ──
static bool OnBeforeShadowCSMDraw(reshade::api::command_list* cmd_list) {
  SyncISFASTToShaderInjection(cmd_list);
  // Push IS-FAST noise texture at t3 (same pattern as t22 in lighting shader)
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
  return true;
}

static bool OnBeforeShadowBlurDraw(reshade::api::command_list* cmd_list) {
  SyncISFASTToShaderInjection(cmd_list);
  return true;
}

// ── Volfog IS-FAST sync + push IS-FAST SRV at t3 ──
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
  return true;
}

static bool OnBeforeKaiVolFogDraw(reshade::api::command_list* cmd_list) {
  SyncVolFogISFASTToShaderInjection(cmd_list);
  // Sync Sora volfog settings → Kai volfog fields
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
  return true;
}

// ── Kai + Daybreak 2 character lighting callback (Env SSS + Character Shadowing) ──
static bool OnBeforeCharLightingDraw(reshade::api::command_list* cmd_list) {
  // Character shader reads shader_injection_data automatically via b13 injection.
  // Push IS-FAST noise at t15 (Kai char shader uses it; Daybreak 2 char does not).
  if (!IsDaybreak2()) {
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

// ── Foliage draw tracking (for GTVBAO foliage exclusion performance) ──
static bool OnBeforeFoliageDraw(reshade::api::command_list* cmd_list) {
  auto* device = cmd_list->get_device();
  auto* data = device->get_private_data<DeviceData>();
  if (data) data->foliage_drawn_this_frame = true;
  return true;
}

// ═══════════ Custom shaders ═══════════

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
    // ── Sora 2nd shadow pipeline ──
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
    CustomShaderEntryCallback(0x485E0022, OnBeforeSsaoShaderDraw),
    // ── Sora 2nd ssao (GTVBAO gate) ──
    CustomShaderEntryCallback(0x752B2580, OnBeforeSsaoShaderDraw),
    // ── Sora foliage (GTVBAO foliage marker bit 15 in o1.w) ──
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
    // ── Sora clutter/flower foliage ──
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
    // ── Sora potflower foliage ──
    {
        0xd9becfb1u,
        renodx::mods::shader::CustomShader{
            .crc32 = 0xD9BECFB1u,
            .code = __0xD9BECFB1,
            .on_draw = OnBeforeFoliageDraw,
        },
    },
    // ── Sora 2nd foliage (GTVBAO foliage marker bit 15 in o1.w) ──
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
    // ── Kai foliage (GTVBAO foliage marker) ──
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
    // ── Kai lighting (GTVBAO + VBGI) ──
    CustomShaderEntryCallback(0x430ED091, OnBeforeLightingShaderDraw),
    CustomShaderEntryCallback(0xF6C55E5F, OnBeforeLightingShaderDraw),
    // ── Kai volumetric fog (IS-FAST + Haze AA) ──
    CustomShaderEntryCallback(0xBD7DFE49, OnBeforeKaiVolFogDraw),
    // ── Kai character lighting (Env SSS + Character Shadowing) ──
    {
        0x445A1838u,
        renodx::mods::shader::CustomShader{
            .crc32 = 0x445A1838u,
            .code = __0x445A1838,
            .on_draw = OnBeforeCharLightingDraw,
        },
    },
    // ── Kai cubemap (10 glass + floor shaders) ──
    CustomShaderEntryCallback(0xB1CCBCAE, nullptr),
    CustomShaderEntryCallback(0x1A17A133, nullptr),
    CustomShaderEntryCallback(0xCA715B78, nullptr),
    CustomShaderEntryCallback(0xE1E0ACBB, nullptr),
    CustomShaderEntryCallback(0xF237E72F, nullptr),
    CustomShaderEntryCallback(0x07E984A7, nullptr),
    CustomShaderEntryCallback(0xFDC5CDBF, nullptr),
    CustomShaderEntryCallback(0x8337B262, nullptr),
    CustomShaderEntryCallback(0xD97BD91B, nullptr),
    CustomShaderEntryCallback(0xEFB6AC0F, nullptr),
    // ── Daybreak 2 cubemap (3 glass shaders) ──
    CustomShaderEntryCallback(0xE01674A5, nullptr),
    CustomShaderEntryCallback(0xF19E927D, nullptr),
    CustomShaderEntryCallback(0x27748076, nullptr),
    // ── Daybreak 2 volumetric fog (Haze AA) ──
    CustomShaderEntryCallback(0x9A49E6E9, nullptr),
    // ── Daybreak 2 character lighting (Env SSS + Character Shadowing) ──
    {
        0xAC3BA23Cu,
        renodx::mods::shader::CustomShader{
            .crc32 = 0xAC3BA23Cu,
            .code = __0xAC3BA23C,
            .on_draw = OnBeforeCharLightingDraw,
        },
    },
    // ── Kai DOF shaders ──
    CustomShaderEntryCallback(0xAB6DBF4D, nullptr),
    CustomShaderEntryCallback(0x2734F870, nullptr),
    // ── Sora 2nd DOF shaders (port Kai improved) ──
    CustomShaderEntryCallback(0x5BBEC5A3, nullptr),
    CustomShaderEntryCallback(0xCD6FC25D, nullptr),
    // ── Sora 1st DOF shaders ──
    CustomShaderEntryCallback(0x1CA8DE95, nullptr),
    //__ALL_CUSTOM_SHADERS,
};

// ═══════════ Settings ═══════════

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
          if (g_settings_mode < 0.5f) {  // Switched to Basic — reset advanced-only settings
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
    // —— IS-FAST Master Toggle (top-level) ——
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

    // ═══════════ Kai / Daybreak 2 - Specific Sections ═══════════

    // ── Cubemap ──
    new renodx::utils::settings::Setting{
      .key = "CubemapImprovements", .binding = &shader_injection.cubemap_improvements_enabled,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 1.f, .label = "Mode", .section = "Cubemap",
      .labels = {"Vanilla", "Improved"},
      .is_visible = []() { return IsKai() || IsDaybreak2(); },
    },
    new renodx::utils::settings::Setting{
      .key = "LightingCubemapMipBoost", .binding = &shader_injection.cubemap_lighting_mip_boost,
      .default_value = 1.5f, .label = "Lighting Mip Boost", .section = "Cubemap",
      .tooltip = "Lighting shader cubemap mip scale. Default is 1.5x.",
      .min = 0.5f, .max = 4.f, .format = "%.1fx",
      .is_enabled = []() { return shader_injection.cubemap_improvements_enabled >= 0.5f; },
      .is_visible = []() { return (IsKai() || IsDaybreak2()) && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "FloorCubemapMipScale", .binding = &shader_injection.floor_cubemap_mip_scale,
      .default_value = 4.f, .label = "Floor Mip Scale", .section = "Cubemap",
      .tooltip = "Scales floor reflection roughness/mip response. 1.0 = Vanilla.",
      .min = 0.f, .max = 4.f, .format = "%.2f",
      .is_visible = []() { return (IsKai() || IsDaybreak2()) && IsAdvancedSettingsMode(); },
    },

    // ── SSGI (Falcom) ──
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

    // ── Depth of Field ──
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
      .key = "DOFSampleCount", .binding = &shader_injection.dof_sample_count,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 24.f, .label = "Sample Count", .section = "Depth of Field",
      .tooltip = "Higher values produce smoother bokeh at higher cost.",
      .min = 4.f, .max = 64.f, .format = "%d",
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
      .default_value = 1.f, .label = "Far Scale", .section = "Depth of Field",
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
      .default_value = 0.02f, .label = "Edge Threshold", .section = "Depth of Field",
      .tooltip = "Rejects CoC-mismatched taps to reduce foreground/background bleeding.",
      .min = 0.02f, .max = 1.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.dof_mode >= 0.5f; },
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

    // ── Character SSGI ──
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

    // ── Fog Color Correction ──
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
      .default_value = 2.f, .label = "Mode", .section = "Character Shadowing",
      .labels = {"Off", "Vanilla", "Bend_SSS"},
    },
    new renodx::utils::settings::Setting{
      .key = "CharShadowType", .binding = &shader_injection.char_shadow_type,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 2.f, .label = "Shadow Type", .section = "Character Shadowing",
      .labels = {"Camera View", "World View", "Combined"},
      .is_enabled = []() { return shader_injection.char_shadow_mode == 2.f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "CharShadowCameraStrength", .binding = &shader_injection.char_shadow_camera_strength,
      .default_value = 100.f, .label = "Camera Strenght", .section = "Character Shadowing",
      .min = 0.f, .max = 100.f,
      .is_enabled = []() { return shader_injection.char_shadow_mode == 2.f && shader_injection.char_shadow_type != 1.f; },
      .parse = [](float v) { return v * 0.01f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "CharShadowWorldStrength", .binding = &shader_injection.char_shadow_world_strength,
      .default_value = 33.f, .label = "World Strenght", .section = "Character Shadowing",
      .min = 0.f, .max = 100.f,
      .is_enabled = []() { return shader_injection.char_shadow_mode == 2.f && shader_injection.char_shadow_type != 0.f; },
      .parse = [](float v) { return v * 0.01f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "CharGTVBAOMode", .binding = &shader_injection.char_gtvbao_mode,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 0.f, .label = "Allow GTVBAO", .section = "Character Shadowing",
      .labels = {"Off", "On", "Combined"},
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "CharGTVBAOMaskStr", .binding = &shader_injection.char_gtvbao_mask_strength,
      .default_value = 75.f, .label = "GTVBAO Char Mask", .section = "Character Shadowing",
      .min = 0.f, .max = 100.f,
      .is_enabled = []() { return shader_injection.char_gtvbao_mode > 0.5f; },
      .parse = [](float v) { return v * 0.01f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "CharGTVBGIMaskStr", .binding = &shader_injection.char_gtvbgi_mask_strength,
      .default_value = 0.f, .label = "GTVBGI Char Mask", .section = "Character Shadowing",
      .min = 0.f, .max = 100.f,
      .parse = [](float v) { return v * 0.01f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "CharShadowSampleCount", .binding = &shader_injection.char_shadow_sample_count,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 12.f, .label = "Sample Count", .section = "Character Shadowing",
      .min = 1.f, .max = 64.f, .format = "%d",
      .is_enabled = []() { return shader_injection.char_shadow_mode == 2.f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "CharShadowHardSamples", .binding = &shader_injection.char_shadow_hard_shadow_samples,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 0.f, .label = "Hard Samples", .section = "Character Shadowing",
      .min = 0.f, .max = 64.f, .format = "%d",
      .is_enabled = []() { return shader_injection.char_shadow_mode == 2.f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "CharShadowFadeSamples", .binding = &shader_injection.char_shadow_fade_out_samples,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 0.f, .label = "Fade Samples", .section = "Character Shadowing",
      .min = 0.f, .max = 64.f, .format = "%d",
      .is_enabled = []() { return shader_injection.char_shadow_mode == 2.f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "CharShadowSurfaceThickness", .binding = &shader_injection.char_shadow_surface_thickness,
      .default_value = 0.075f, .label = "Surface Thickness", .section = "Character Shadowing",
      .min = 0.001f, .max = 0.2f, .format = "%.4f",
      .is_enabled = []() { return shader_injection.char_shadow_mode == 2.f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "CharShadowContrast", .binding = &shader_injection.char_shadow_contrast,
      .default_value = 9.f, .label = "Shadow Contrast", .section = "Character Shadowing",
      .min = 0.f, .max = 12.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.char_shadow_mode == 2.f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "CharShadowLightFadeStart", .binding = &shader_injection.char_shadow_light_screen_fade_start,
      .default_value = 1.0f, .label = "Light Fade Start", .section = "Character Shadowing",
      .min = 0.f, .max = 1.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.char_shadow_mode == 2.f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "CharShadowLightFadeEnd", .binding = &shader_injection.char_shadow_light_screen_fade_end,
      .default_value = 0.5f, .label = "Light Fade End", .section = "Character Shadowing",
      .min = 0.f, .max = 1.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.char_shadow_mode == 2.f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "CharShadowMinOccluderDepthScale", .binding = &shader_injection.char_shadow_min_occluder_depth_scale,
      .default_value = 0.f, .label = "Occluder Depth Scale", .section = "Character Shadowing",
      .min = 0.f, .max = 4.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.char_shadow_mode == 2.f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "EnvSSSEnabled", .binding = &shader_injection.env_sss_enabled,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 1.f, .label = "Sun SSS", .section = "Sun Screen Space Shadows",
      .labels = {"Off", "On"},
      .is_visible = []() { return IsKai(); },
    },
    new renodx::utils::settings::Setting{
      .key = "EnvSSSStrength", .binding = &shader_injection.env_sss_strength,
      .default_value = 100.f, .label = "Strength", .section = "Sun Screen Space Shadows",
      .min = 0.f, .max = 100.f,
      .is_enabled = []() { return shader_injection.env_sss_enabled >= 0.5f; },
      .parse = [](float v) { return v * 0.01f; },
      .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "EnvSSSSampleCount", .binding = &shader_injection.env_sss_sample_count,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 32.f, .label = "Sample Count", .section = "Sun Screen Space Shadows",
      .min = 1.f, .max = 64.f, .format = "%d",
      .is_enabled = []() { return shader_injection.env_sss_enabled >= 0.5f; },
    .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "EnvSSSHardSamples", .binding = &shader_injection.env_sss_hard_shadow_samples,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 0.f, .label = "Hard Shadow Samples", .section = "Sun Screen Space Shadows",
      .tooltip = "Number of hard-contact samples at the start of the ray march. 0 = auto (Sample Count / 8). Higher = sharper contact shadows, but may miss thin occluders.",
      .min = 0.f, .max = 32.f, .format = "%d",
      .is_enabled = []() { return shader_injection.env_sss_enabled >= 0.5f; },
    .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "EnvSSSFadeSamples", .binding = &shader_injection.env_sss_fade_out_samples,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 0.f, .label = "Fade Out Samples", .section = "Sun Screen Space Shadows",
      .tooltip = "Number of fade-out samples at the end of the ray march. 0 = auto (Sample Count / 3). Higher = smoother transition from shadow to no shadow, reducing banding in soft shadows.",
      .min = 0.f, .max = 32.f, .format = "%d",
      .is_enabled = []() { return shader_injection.env_sss_enabled >= 0.5f; },
    .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "EnvSSSSurfaceThickness", .binding = &shader_injection.env_sss_surface_thickness,
      .default_value = 0.005f, .label = "Surface Thickness", .section = "Sun Screen Space Shadows",
      .min = 0.001f, .max = 0.2f, .format = "%.4f",
      .is_enabled = []() { return shader_injection.env_sss_enabled >= 0.5f; },
    .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "EnvSSSContrast", .binding = &shader_injection.env_sss_contrast,
      .default_value = 2.f, .label = "Shadow Contrast", .section = "Sun Screen Space Shadows",
      .min = 0.f, .max = 12.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.env_sss_enabled >= 0.5f; },
    .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "EnvSSSHeightEnable", .binding = &shader_injection.env_sss_height_enabled,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 1.f, .label = "Height Above Ground", .section = "Sun Screen Space Shadows",
      .labels = {"Off", "On"},
      .is_enabled = []() { return shader_injection.env_sss_enabled >= 0.5f; },
    .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "EnvSSSHeightMin", .binding = &shader_injection.env_sss_height_min,
      .default_value = 0.f, .label = "Min Height", .section = "Sun Screen Space Shadows",
      .min = 0.f, .max = 10.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.env_sss_enabled >= 0.5f && shader_injection.env_sss_height_enabled >= 0.5f; },
    .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "EnvSSSHeightMax", .binding = &shader_injection.env_sss_height_max,
      .default_value = 1.f, .label = "Ground Search", .section = "Sun Screen Space Shadows",
      .min = 1.f, .max = 200.f, .format = "%.0f",
      .is_enabled = []() { return shader_injection.env_sss_enabled >= 0.5f && shader_injection.env_sss_height_enabled >= 0.5f; },
    .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "EnvSSSHeightFade", .binding = &shader_injection.env_sss_height_fade,
      .default_value = 0.10f, .label = "Height Fade", .section = "Sun Screen Space Shadows",
      .min = 0.f, .max = 5.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.env_sss_enabled >= 0.5f && shader_injection.env_sss_height_enabled >= 0.5f; },
    .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "EnvSSSVerticalReject", .binding = &shader_injection.env_sss_vertical_reject,
      .default_value = 0.30f, .label = "Vertical Reject", .section = "Sun Screen Space Shadows",
      .min = 0.f, .max = 1.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.env_sss_enabled >= 0.5f; },
    .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "EnvSSSMaxDarkening", .binding = &shader_injection.env_sss_max_darkening,
      .default_value = 0.40f, .label = "Max Darkening", .section = "Sun Screen Space Shadows",
      .min = 0.f, .max = 1.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.env_sss_enabled >= 0.5f; },
    .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "EnvSSBrightRejectThreshold", .binding = &shader_injection.env_sss_bright_reject_threshold,
      .default_value = 0.5f, .label = "Brightness Reject", .section = "Sun Screen Space Shadows",
      .min = 0.f, .max = 5.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.env_sss_enabled >= 0.5f; },
    .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "EnvSSBrightRejectFade", .binding = &shader_injection.env_sss_bright_reject_fade,
      .default_value = 0.5f, .label = "Brightness Fade", .section = "Sun Screen Space Shadows",
      .min = 0.01f, .max = 3.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.env_sss_enabled >= 0.5f; },
    .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "EnvSSSCSMGate", .binding = &shader_injection.env_sss_csm_gate,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 1.f, .label = "CSM Indoor Gate", .section = "Sun Screen Space Shadows",
      .tooltip = "Skip screen-space shadows on pixels already in deep CSM shadow (prevents false shadows inside buildings).",
      .labels = {"Off", "On"},
      .is_enabled = []() { return shader_injection.env_sss_enabled >= 0.5f; },
      .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "DebugShowEnvSSS", .binding = &shader_injection.debug_show_env_sss,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 0.f, .label = "Sun SSS Debug View", .section = "Sun Screen Space Shadows",
      .labels = {"Off", "SSS Mask", "Shadow Value"},
    .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    // —— Local Screen Space Shadows ——
    new renodx::utils::settings::Setting{
      .key = "LocalSSSEnable", .binding = &shader_injection.local_sss_enabled,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 1.f, .label = "Local SSS", .section = "Local Screen Space Shadows",
      .tooltip = "Enable Bend_SSS screen-space ray-march shadows for point/spot lights.",
      .labels = {"Off", "On"},
      .is_visible = []() { return IsKai(); },
    },
    new renodx::utils::settings::Setting{
      .key = "LocalSSSStrength", .binding = &shader_injection.local_sss_strength,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 1.f, .label = "Strength", .section = "Local Screen Space Shadows",
      .min = 0.f, .max = 1.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.local_sss_enabled >= 0.5f; },
      .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "LocalSSSLightType", .binding = &shader_injection.local_sss_light_type,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 2.f, .label = "Light Type", .section = "Local Screen Space Shadows",
      .tooltip = "Which light types receive screen-space shadows.",
      .labels = {"Spot", "Point", "Both"},
      .is_enabled = []() { return shader_injection.local_sss_enabled >= 0.5f; },
      .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "LocalSSSSampleCount", .binding = &shader_injection.local_sss_sample_count,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 10.f, .label = "Sample Count", .section = "Local Screen Space Shadows",
      .min = 1.f, .max = 64.f, .format = "%d",
      .is_enabled = []() { return shader_injection.local_sss_enabled >= 0.5f; },
      .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "LocalSSSHardSamples", .binding = &shader_injection.local_sss_hard_shadow_samples,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 0.f, .label = "Hard Samples", .section = "Local Screen Space Shadows",
      .tooltip = "Number of hard-contact samples at the start. 0 = auto (Sample Count / 8).",
      .min = 0.f, .max = 32.f, .format = "%d",
      .is_enabled = []() { return shader_injection.local_sss_enabled >= 0.5f; },
      .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "LocalSSSFadeSamples", .binding = &shader_injection.local_sss_fade_out_samples,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 0.f, .label = "Fade Samples", .section = "Local Screen Space Shadows",
      .tooltip = "Number of fade-out samples at the end. 0 = auto (Sample Count / 3).",
      .min = 0.f, .max = 32.f, .format = "%d",
      .is_enabled = []() { return shader_injection.local_sss_enabled >= 0.5f; },
      .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "LocalSSSSurfaceThickness", .binding = &shader_injection.local_sss_surface_thickness,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.0200f, .label = "Surface Thickness", .section = "Local Screen Space Shadows",
      .min = 0.001f, .max = 0.2f, .format = "%.4f",
      .is_enabled = []() { return shader_injection.local_sss_enabled >= 0.5f; },
      .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "LocalSSSContrast", .binding = &shader_injection.local_sss_contrast,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 12.f, .label = "Shadow Contrast", .section = "Local Screen Space Shadows",
      .min = 0.f, .max = 12.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.local_sss_enabled >= 0.5f; },
      .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "LocalSSSLightFadeStart", .binding = &shader_injection.local_sss_light_fade_start,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.15f, .label = "Light Fade Start", .section = "Local Screen Space Shadows",
      .tooltip = "Screen-distance where shadow starts fading (0 = at the light, 1 = at light radius edge).",
      .min = 0.f, .max = 1.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.local_sss_enabled >= 0.5f; },
      .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "LocalSSSLightFadeEnd", .binding = &shader_injection.local_sss_light_fade_end,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 1.f, .label = "Light Fade End", .section = "Local Screen Space Shadows",
      .tooltip = "Screen-distance where shadow is fully faded out.",
      .min = 0.f, .max = 1.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.local_sss_enabled >= 0.5f; },
      .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "LocalSSSOccluderDepthScale", .binding = &shader_injection.local_sss_occluder_depth_scale,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.f, .label = "Occluder Depth Scale", .section = "Local Screen Space Shadows",
      .tooltip = "Scale minimum occluder depth for thin geometry. Higher = wider occluders, fewer false shadows.",
      .min = 0.f, .max = 4.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.local_sss_enabled >= 0.5f; },
      .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    // —— GTVBAO ——
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
      .default_value = 1.f, .label = "Quality Level", .section = "GTVBAO",
      .labels = {"Low", "Medium", "High", "Ultra"},
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "GTVBAODenoisePasses", .binding = &shader_injection.gtvbao_denoise_passes,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 1.f, .label = "Denoise Passes", .section = "GTVBAO",
      .tooltip = "Bilateral chain strength. Ignored while À-Trous Filter is On (fixed 3 wavelet iterations).",
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
      .default_value = 0.35f, .label = "Radius", .section = "GTVBAO",
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
      .default_value = 1.457f, .label = "Radius Multiplier", .section = "GTVBAO",
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
      .default_value = 1.0f, .label = "Sample Distribution", .section = "GTVBAO",
      .min = 1.0f, .max = 3.0f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f; },
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "GTVBAOBitmaskThickness", .binding = &shader_injection.gtvbao_bitmask_thickness,
      .default_value = 0.2f, .label = "Bitmask Thickness", .section = "GTVBAO",
      .tooltip = "World-space thickness for visibility bitmask. Higher = more light passes behind surfaces.",
      .min = 0.01f, .max = 2.0f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f; },
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    // —— GTVBAO Upgrade (visibility bitmask accuracy improvements) ——
    new renodx::utils::settings::Setting{
      .key = "GTVBAOGTVBAOCDF", .binding = &shader_injection.gtvbao_cdf_enabled,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 1.f, .label = "GTVBAO CDF Remap", .section = "GTVBAO",
      .tooltip = "CDF-remap horizon angles to correct sample density near the view pole. Reduces AO bias.",
      .labels = {"Off", "On"},
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f; },
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "GTVBAOGTVBAOCosine", .binding = &shader_injection.gtvbao_cosine_enabled,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 1.f, .label = "GTVBAO Cosine Sampling", .section = "GTVBAO",
      .tooltip = "Sample slice directions from a cosine-weighted hemisphere instead of uniformly. Physically correct AO falloff.",
      .labels = {"Off", "On"},
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f; },
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "GTVBAOGTVBAOCosineMode", .binding = &shader_injection.gtvbao_cosine_mode,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 2.f, .label = "Cosine Sampling Mode", .section = "GTVBAO",
      .tooltip = "Mode 1: Uniform slices with per-slice weight. Mode 2: Ray projection from world-space lobe. Mode 3: CDF importance sampling (best quality/speed).",
      .labels = {"Weight", "Project", "CDF"},
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && shader_injection.gtvbao_cosine_enabled > 0.5f; },
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "GTVBAOGTVBAOThickness", .binding = &shader_injection.gtvbao_thickness_enabled,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 1.f, .label = "GTVBAO Per-Sample Thickness", .section = "GTVBAO",
      .tooltip = "Compute thickness offset per sample direction instead of using fixed view-vector offset. Correct for wide FOV.",
      .labels = {"Off", "On"},
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
    new renodx::utils::settings::Setting{
      .key = "GTVBAODenoiserType", .binding = &shader_injection.gtvbao_denoiser_type,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 0.f, .label = "Denoiser Type", .section = "GTVBAO",
      .tooltip = "Spatial: 5x5 edge-aware blur only. Spatio-Temporal: blends with previous frame for much higher stability on thin geometry. Poisson: disk sampling with luma/depth/normal similarity weights.",
      .labels = {"Spatial", "Spatio-Temporal", "Poisson"},
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && shader_injection.gtvbao_denoise_passes > 0.f; },
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "GTVBAOPoissonSamples", .binding = &shader_injection.gtvbao_poisson_samples,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 6.f, .label = "Poisson Samples", .section = "GTVBAO",
      .tooltip = "Number of Poisson disk samples for denoising. More samples = better quality, higher cost.",
      .labels = {"4","6","8","10","12","14","16","20","24","28","32"},
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && shader_injection.gtvbao_denoise_passes > 0.f && shader_injection.gtvbao_denoiser_type >= 1.5f; },
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "GTVBAOPoissonLumaPhi", .binding = &shader_injection.gtvbao_poisson_luma_phi,
      .default_value = 0.f, .label = "Poisson Luma Phi", .section = "GTVBAO",
      .tooltip = "Luma/AO similarity falloff. Lower = stricter (only very similar pixels contribute). Higher = more blur.",
      .min = 0.0f, .max = 20.0f, .format = "%.1f",
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && shader_injection.gtvbao_denoise_passes > 0.f && shader_injection.gtvbao_denoiser_type >= 1.5f; },
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "GTVBAOPoissonDepthPhi", .binding = &shader_injection.gtvbao_poisson_depth_phi,
      .default_value = 0.f, .label = "Poisson Depth Phi", .section = "GTVBAO",
      .tooltip = "Depth similarity falloff. Lower = stricter (only coplanar surfaces contribute). Higher = more blur across depth edges.",
      .min = 0.0f, .max = 20.0f, .format = "%.1f",
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && shader_injection.gtvbao_denoise_passes > 0.f && shader_injection.gtvbao_denoiser_type >= 1.5f; },
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "GTVBAOPoissonNormalPhi", .binding = &shader_injection.gtvbao_poisson_normal_phi,
      .default_value = 0.f, .label = "Poisson Normal Phi", .section = "GTVBAO",
      .tooltip = "Normal similarity falloff (exponent). Lower = stricter (only same-facing surfaces). Higher = more blur across normals.",
      .min = 0.0f, .max = 20.0f, .format = "%.1f",
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && shader_injection.gtvbao_denoise_passes > 0.f && shader_injection.gtvbao_denoiser_type >= 1.5f; },
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "GTVBAOTemporalFrames", .binding = &shader_injection.gtvbao_temporal_frame_count,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 0.f, .label = "Temporal Frames", .section = "GTVBAO",
      .tooltip = "How many previous frames influence the result. 0-1 = off (spatial only). 2 = fast response. 8 = balanced (default). 16 = most stable, some ghosting.",
      .labels = {"0","1","2","3","4","5","6","7","8","9","10","11","12","13","14","15","16"},
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && shader_injection.gtvbao_denoise_passes > 0.f && shader_injection.gtvbao_denoiser_type > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "GTVBAOTemporalBlend", .binding = &shader_injection.gtvbao_temporal_blend,
      .default_value = 0.35f, .label = "Temporal Blend", .section = "GTVBAO",
      .tooltip = "Overall temporal strength (multiplied with Frames). 1.0 = full effect. 0.5 = half. 0.0 = off.",
      .min = 0.0f, .max = 1.0f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && shader_injection.gtvbao_denoise_passes > 0.f && shader_injection.gtvbao_denoiser_type > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "GTVBAODisocclusionThr", .binding = &shader_injection.gtvbao_disocclusion_threshold,
      .default_value = 0.01f, .label = "Disocclusion Threshold", .section = "GTVBAO",
      .tooltip = "Max depth difference to accept history sample. Higher = more ghosting, less flicker on disocclusion.",
      .min = 0.001f, .max = 1.0f, .format = "%.3f",
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && shader_injection.gtvbao_denoise_passes > 0.f && shader_injection.gtvbao_denoiser_type > 0.5f; },
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "GTVBAOTemporalNormalReject", .binding = &shader_injection.gtvbao_temporal_normal_reject,
      .default_value = 0.5f, .label = "Temporal Normal Reject", .section = "GTVBAO",
      .tooltip = "History normal similarity required for full acceptance (dot product). Reduces temporal ghosting across geometry edges.",
      .min = 0.f, .max = 1.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && shader_injection.gtvbao_denoise_passes > 0.f && shader_injection.gtvbao_denoiser_type > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "GTVBAOGhostClamp", .binding = &shader_injection.gtvbao_ghost_clamp,
      .default_value = 1.5f, .label = "Ghost Clamp", .section = "GTVBAO",
      .tooltip = "Clamps history to the current-frame neighborhood range (in stddevs). Lower = less ghosting, more flicker. 0 = off.",
      .min = 0.f, .max = 4.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && shader_injection.gtvbao_denoise_passes > 0.f && shader_injection.gtvbao_denoiser_type > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "GTVBAOAtrousEnabled", .binding = &shader_injection.gtvbao_atrous_enabled,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 1.f, .label = "À-Trous Filter", .section = "GTVBAO",
      .tooltip = "Edge-aware wavelet spatial filter (3 iterations, growing radius). Works with both Spatial and Spatio-Temporal denoiser types. Replaces the bilateral chain.",
      .labels = {"Off", "On"},
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && shader_injection.gtvbao_denoise_passes > 0.f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "GTVBAOAtrousDepthSigma", .binding = &shader_injection.gtvbao_atrous_depth_sigma,
      .default_value = 1.f, .label = "À-Trous Depth Stop", .section = "GTVBAO",
      .tooltip = "Depth edge sensitivity for the à-trous filter. Higher = smoother across depth steps (more leak).",
      .min = 0.05f, .max = 4.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && shader_injection.gtvbao_denoise_passes > 0.f && shader_injection.gtvbao_atrous_enabled > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "GTVBAOAtrousNormalSigma", .binding = &shader_injection.gtvbao_atrous_normal_sigma,
      .default_value = 32.f, .label = "À-Trous Normal Stop", .section = "GTVBAO",
      .tooltip = "Normal edge sensitivity for the à-trous filter. Quantized to powers of two; higher = sharper edges. 32 is the default.",
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
      .min = 0.f, .max = 1.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && g_gtvbao_normal_input_mode > 0.5f; },
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "GTVBAONormalSharpness", .binding = &g_gtvbao_normal_sharpness,
      .default_value = 1.f, .label = "Normal Sharpness", .section = "GTVBAO",
      .min = 0.01f, .max = 4.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && g_gtvbao_normal_input_mode > 0.5f; },
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "GTVBAONormalEdgeRejection", .binding = &g_gtvbao_normal_edge_rejection,
      .default_value = 0.5f, .label = "Normal Edge Rejection", .section = "GTVBAO",
      .min = 0.f, .max = 2.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && g_gtvbao_normal_input_mode > 0.5f; },
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "GTVBAONormalZPreservation", .binding = &g_gtvbao_normal_z_preservation,
      .default_value = 0.f, .label = "Normal Z Preservation", .section = "GTVBAO",
      .min = 0.f, .max = 2.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && g_gtvbao_normal_input_mode > 0.5f; },
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "GTVBAONormalDetailResponse", .binding = &g_gtvbao_normal_detail_response,
      .default_value = 1.0f, .label = "Normal Detail Response", .section = "GTVBAO",
      .min = 0.01f, .max = 1.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && g_gtvbao_normal_input_mode > 0.5f; },
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "GTVBAONormalMaxDarkening", .binding = &g_gtvbao_normal_max_darkening,
      .default_value = 1.f, .label = "Normal Max Darkening", .section = "GTVBAO",
      .min = 0.f, .max = 1.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && g_gtvbao_normal_input_mode > 0.5f; },
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "GTVBAONormalDarkeningMode", .binding = &g_gtvbao_normal_darkening_mode,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 0.f, .label = "Normal Darkening Mode", .section = "GTVBAO",
      .labels = {"Multiply", "Replace"},
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && g_gtvbao_normal_input_mode > 0.5f; },
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "GTVBAONormalTransformMode", .binding = &g_gtvbao_normal_transform_mode,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 1.f, .label = "Normal Transform Mode", .section = "GTVBAO",
      .tooltip = "How to transform MRT normals to view space. Try alternatives if normals look wrong at some camera angles.",
      .labels = {"view_g (default)", "viewInv_g", "Passthrough"},
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && g_gtvbao_normal_input_mode > 0.5f; },
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "GTVBAODebugView", .binding = &shader_injection.gtvbao_debug_view,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 0.f, .label = "Debug View", .section = "GTVBAO",
      .labels = {"Off", "AO Only", "GTVBAO raw .a", "GTVBAO RGBA", "Vanilla SSAO", "Depth",
                 "6:BitmaskHeat", "7:SectorCount", "8:1stSliceBits", "9:FoliageMask"},
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
      .tooltip = "Bitmask AO experimental fixes. 0=Off (baseline). Test each mode to diagnose darkening.",
      .labels = {"Off", "1:Clamp50%", "2:Clamp100%", "3:ScaleDist", "4:SkipBehind", "5:Skip2x"},
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
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
      .tooltip = "Depth-aware 3×3 bilateral pre-filter on raw AO before power curve (reduces bitmask noise).",
      .labels = {"Off", "On"},
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    // ── GTVBAO Foliage ──
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
    // ── Foliage Grass AO ──
    new renodx::utils::settings::Setting{
      .key = "FoliageGrassAOEnabled", .binding = &shader_injection.foliage_grass_ao_enabled,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 1.f, .label = "Grass AO", .section = "Foliage Grass AO",
      .tooltip = "Per-blade vertical AO gradient: dark at root, bright at tip. Replaces noisy SSAO/GTAO on foliage with a stable bake. (Ghost of Tsushima §1.5e-ii)",
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
    // ── BRDF Improvement ──
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
      .tooltip = "Replaces Blinn-Phong specular with GGX D·V·F + Kulla-Conty multi-scatter compensation (SIGGRAPH 2017).",
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
    // ── GTVBAO dispatch Fix (for double volumetrics) ──
    new renodx::utils::settings::Setting{
      .key = "GTVBAOCSDispatchFix", .binding = &g_gtvbao_cs_dispatch_fix,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 2.f, .label = "CS Dispatch Fix", .section = "GTVBAO",
      .tooltip = "Fixes double volumetrics caused by stale compute descriptor bindings. Fix 1: restore state via Apply(). Fix 2: null compute descriptors. Fix 3: null + restore.",
      .labels = {"Off", "Fix 1: Restore State", "Fix 2: Null Compute", "Fix 3: Null + Restore"},
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    // —— SSGI (Screen Space Global Illumination — integrated into GTVBAO) ——
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
      .default_value = 0.75f, .label = "Intensity", .section = "VBGI",
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
      .tooltip = "Intensity of the multi-bounce feedback. 1.0 = natural, higher = stronger accumulation.",
      .min = 0.0f, .max = 10.0f, .format = "%.2f",
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
      .default_value = 0.1f, .label = "Adaptive Luma Strength", .section = "VBGI",
      .tooltip = "Target brightness for GI normalization. 0=off. Higher = brighter target. Evens out indoor/outdoor GI.",
      .min = 0.0f, .max = 5.0f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && shader_injection.vbgi_enabled > 0.5f; },
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "SSGIAdaptiveLumaBlend", .binding = &shader_injection.vbgi_adaptive_luma_blend,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.15f, .label = "Adaptive Luma Blend", .section = "VBGI",
      .tooltip = "Blend between original GI (0) and luma-normalized GI (1).",
      .min = 0.0f, .max = 1.0f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && shader_injection.vbgi_enabled > 0.5f; },
    .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "SSGIMaxClamp", .binding = &shader_injection.vbgi_max_clamp,
      .default_value = 0.2f, .label = "GI Max Clamp", .section = "VBGI",
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
    // —— VBGI debug ——
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
    // ── Kai: GTVBAO VBGI Falcom SSGI consumption ──
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
    // —— SSGI Affect Lights ——
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
    // —— Custom SSR (Sora 2nd) ——
    new renodx::utils::settings::Setting{
      .key = "CustomSSREnable", .binding = &shader_injection.ssr_mode,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 1.f, .label = "Custom SSR", .section = "Custom SSR",
      .tooltip = "Stochastic screen-space reflections (Sora 2nd): Hi-Z hierarchical tracing with GGX/VNDF importance sampling. Phase 1 builds the depth hierarchy; use Debug View to inspect it. Also gates Kai's improved SSR.",
      .labels = {"Off", "On"},
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "SSRDeferredDispatch", .binding = &shader_injection.ssr_deferred_dispatch,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 0.f, .label = "Deferred Dispatch", .section = "Custom SSR",
      .tooltip = "Move the SSR dispatch to OnPresent (1-frame latency). Default OFF = inline at the lighting draw (no latency). Deferred mode requires CPU-Opt 'Deferred Dispatch' to be ON.",
      .labels = {"Off", "On"},
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "SSRMaxRayDistance", .binding = &shader_injection.ssr_max_ray_distance,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 300.f, .label = "Max Ray Distance", .section = "Custom SSR",
      .tooltip = "Affects stochastic/Production ray travel.\nDeterministic Mirror mode ignores this value for screen-space direction construction; AMD parity requires a unit-length direction.",
      .min = 10.f, .max = 2000.f, .format = "%.0f",
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "SSRMaxTraversalSteps", .binding = &shader_injection.ssr_max_traversal_steps,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 64.f, .label = "Traversal Steps", .section = "Custom SSR",
      .tooltip = "R-budget A/B: hierarchical march iteration cap (16-512). Higher budgets let rays reach distant reflected geometry; watch the budget% termination line in TraceStats.",
      .min = 16.f, .max = 512.f, .format = "%.0f",
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "SSRLogTraceStats", .binding = &shader_injection.ssr_log_tracestats,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 0.f, .label = "Trace Stats Logging", .section = "Custom SSR",
      .tooltip = "Aggregate funnel/reject/termination/thickness statistics dump. Independent of Debug View.",
      .labels = {"Off", "On"},
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "SSRLogProbes", .binding = &shader_injection.ssr_log_probes,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 0.f, .label = "Probe Logging", .section = "Custom SSR",
      .tooltip = "GeoProbe/dirProbe/vndfProbe/FootProbe/CompareProbe per-pixel decode lines within TraceStats dumps.",
      .labels = {"Off", "On"},
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "SSRLogResolve", .binding = &shader_injection.ssr_log_resolve,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 0.f, .label = "Resolve Probe Logging", .section = "Custom SSR",
      .tooltip = "Resolve-side probe lines (ResolveProbe/EstimatorProbe/RvS). Throttled to every 30 frames.",
      .labels = {"Off", "On"},
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "SSRLogNgx", .binding = &shader_injection.ssr_log_ngx,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 0.f, .label = "NGX MV Capture Logging", .section = "Custom SSR",
      .tooltip = "One-shot verification log when DLSS motion vectors are first captured.",
      .labels = {"Off", "On"},
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "SSRLogConfig", .binding = &shader_injection.ssr_log_config,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 0.f, .label = "Config Echo Logging", .section = "Custom SSR",
      .tooltip = "Phase3Config echo and Integrate apply-state transition logs.",
      .labels = {"Off", "On"},
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "SSRLogInit", .binding = &shader_injection.ssr_log_init,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 0.f, .label = "Init Logging", .section = "Custom SSR",
      .tooltip = "R3 pyramid/pipeline creation staged logs. Errors always visible regardless.",
      .labels = {"Off", "On"},
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "SSRThickness", .binding = &shader_injection.ssr_thickness,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.15f, .label = "Depth Thickness", .section = "Custom SSR",
      .tooltip = "Thickness threshold T (world/view units). PRODUCTION BASELINE 0.15 with Perpendicular metric (pending imagery A/B). Formula: reject iff metric >~ T, confidence=(1-smoothstep(0,T,metric))^2. Sweep with Trace Stats thickCDF line; validate Hit UV-Accepted on four surface classes.",
      .min = 0.001f, .max = 0.6f, .format = "%.3f",
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "SSREligibilityMode", .binding = &shader_injection.ssr_eligibility_mode,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 1.f, .label = "Eligibility", .section = "Custom SSR",
      .tooltip = "Which pixels spawn/accept custom SSR rays: Vanilla Flag = game's own SSR mask (water-only in practice; A/B reference). Custom Material = our material criteria: roughness <= Roughness Cutoff (characters/foliage excluded). All = every opaque world material excluding characters/foliage.",
      .labels = {"Vanilla Flag", "Custom Material", "All"},
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "SSRRoughnessThreshold", .binding = &shader_injection.ssr_roughness_threshold,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.6f, .label = "Roughness Cutoff", .section = "Custom SSR",
      .tooltip = "Custom Material eligibility gate: materials with roughness above this do not spawn reflection rays (AMD SSSR roughnessThreshold analogue).",
      .min = 0.f, .max = 1.f, .format = "%.2f",
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "SSRThicknessGate", .binding = &shader_injection.ssr_thickness_gate,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 1.f, .label = "Thickness Gate", .section = "Custom SSR",
      .tooltip = "Reject candidates whose hit-to-surface separation exceeds the thickness confidence budget. Turn OFF (Mirror, Self-Hit 0, Backface OFF) to measure how much validation the criterion consumes; the would-reject counter keeps measuring. TraceStats also logs the raw thickness-metric distribution.",
      .labels = {"Off", "On"},
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "SSRIntensity", .binding = &shader_injection.ssr_intensity,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 1.f, .label = "Intensity", .section = "Custom SSR",
      .tooltip = "Overall reflection strength (0-2). Requires Spatial Resolve ON — scales the resolved SSR contribution before compositing.",
      .min = 0.f, .max = 2.f, .format = "%.2f",
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "SSRDebugView", .binding = &shader_injection.ssr_debug_view,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 0.f, .label = "Debug View", .section = "Custom SSR",
      .tooltip = "HiZ: 1=mip0 depth, 2=selected mip, 3=adjacent bound check, 4=classification + CPU verify. Trace: 5=Hit/Miss, 6=Hit UV (RAW), 7=Term MIP, 8=Iterations, 9=Reject Reason, 10=Ray Dir, 11=Term Reason, 12=Ray Path, 13=Trace Stats, 14=Cand Dist, 15=Cand Travel, 16=Cand Z-Delta. Normals: 17-22. Stochastic: 23=Accepted UV, 24=Validation Class, 25=VNDF vs Mirror, 26=VNDF Params, 27=Raw Radiance (source preview).",
      .labels = {"Off", "HiZ Mip0", "HiZ Mip N", "Bound Check", "Classify",
                 "Hit/Miss", "Hit UV", "Term MIP", "Iterations", "Reject Reason",
                 "Ray Dir", "Term Reason", "Ray Path", "Trace Stats", "Cand Dist",
                 "Cand Travel", "Cand Z-Delta", "Normal Diff", "Depth Normal",
                 "MRT Normal", "V View", "N View", "Refl Delta", "Accepted UV",
                 "Validation Class", "VNDF vs Mirror", "VNDF Params", "Radiance Src",
                 "t25 RGB", "t25 Alpha", "Vanilla t24", "SSR Coverage",
                 "Hit Radiance", "Proj Position", "Mirror Hit Class",
                 "Same Surface", "Compare Probe Pixel", "SSR Raw vs Resolved",
                 "Motion Vectors"},
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "SSRBypassValidation", .binding = &shader_injection.ssr_bypass_validation,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 0.f, .label = "Bypass Validation", .section = "Custom SSR",
      .tooltip = "Accept trace candidates after in-bounds + finite-depth checks only (skips self/sky/backface/thickness/vignette). Compares raw vs validated hit rate.",
      .labels = {"Off", "On"},
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "SSRForcedRayMode", .binding = &shader_injection.ssr_forced_ray_mode,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 0.f, .label = "Forced Ray", .section = "Custom SSR",
      .tooltip = "Deterministic test rays: Mirror = actual G-buffer normal reflect(-V,N). Fixed Normal = known view-space constant (traversal baseline). Screen Diagonal = identical artificial ray (isolates Hi-Z marcher). Depth/Floor Normal = empirically measured per-pixel normal from the depth buffer — the known-floor-normal ladder rung, no coordinate assumptions.",
      .labels = {"Mirror", "Fixed Normal", "Screen Diagonal", "Depth/Floor Normal",
                 "Fixed Plane", "Proj Endpoint", "Linear March"},
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "SSRNormalConvention", .binding = &shader_injection.ssr_normal_convention,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 1.f, .label = "Normal Convention", .section = "Custom SSR",
      .tooltip = "World->view transform for G-buffer normals. mul(n,M) is CANONICAL (locked by Phase 2.1 trace data); mul(M,n) kept for A/B paranoia.",
      .labels = {"mul(M,n)", "mul(n,M)"},
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "SSRSelfHitThreshold", .binding = &shader_injection.ssr_self_hit_threshold,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 2.f, .label = "Self-Hit Threshold", .section = "Custom SSR",
      .tooltip = "Manhattan pixel threshold for the self-intersection reject. Sweep 0/0.5/1/2/4/8 with Trace Stats to see how many candidates die here.",
      .min = 0.f, .max = 8.f, .format = "%.1f",
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "SSRBackfaceGate", .binding = &shader_injection.ssr_backface_gate,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 1.f, .label = "Backface Gate", .section = "Custom SSR",
      .tooltip = "Reject candidates whose hit-texel normal faces along the ray. Turn OFF (with Self-Hit Threshold = 0) to A/B the gate: a large relative jump in validated% means the backface test/convention is a major problem; counters keep measuring either way.",
      .labels = {"Off", "On"},
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "SSRInitialAdvanceBias", .binding = &shader_injection.ssr_initial_advance_bias,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.f, .label = "Initial Advance Bias", .section = "Custom SSR",
      .tooltip = "Minimum screen-space pixel displacement before the FIRST depth test (0 = vanilla SSSR behavior). Sweep 0/0.25/0.5/1/2/4/8 with Trace Stats: the 0-1px candidate bucket should collapse into 2-8/8-32/32+ as bias rises. Watch firstStep and candDist lines.",
      .min = 0.f, .max = 8.f, .format = "%.2f",
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "SSRThicknessMetric", .binding = &shader_injection.ssr_thickness_mode,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 1.f, .label = "Thickness Metric", .section = "Custom SSR",
      .tooltip = "Hit-validation distance metric: Euclidean = length(viewSurf - viewHit) (conflates lateral slide with penetration on grazing floors). Perpendicular = abs(dot(delta, N_depth)) at the hit texel — measures only true surface penetration. PRODUCTION BASELINE = Perpendicular @ T=0.15 (pending reflection-imagery A/B). Both distributions always logged (thickDist / thickDistP); perpFallback counts Euclidean fallbacks.",
      .labels = {"Euclidean", "Perpendicular"},
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "SSRDebugMip", .binding = &shader_injection.ssr_debug_mip,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 2.f, .label = "Debug Mip", .section = "Custom SSR",
      .tooltip = "Mip level used by HiZ debug views 2, 3 and 4.",
      .min = 0.f, .max = 7.f, .format = "%.0f",
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    // —— Phase 3 stochastic SSR ——
    new renodx::utils::settings::Setting{
      .key = "SSRStochasticSampling", .binding = &shader_injection.ssr_stochastic,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 1.f, .label = "Stochastic Sampling", .section = "Custom SSR",
      .tooltip = "Production rays: Heitz GGX VNDF importance sampling with material roughness (IS-FAST noise). OFF = PERMANENT REGRESSION MODE: deterministic Mirror ray, byte-equivalent to the validated Phase 2 pipeline. Keep OFF+RayCount=1+Perp T=0.15 as the known-good reference when comparing future changes (spatial reconstruction etc.).",
      .labels = {"Off", "On"},
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "SSRApplyToScene", .binding = &shader_injection.ssr_apply,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 0.f, .label = "Apply To Scene", .section = "Custom SSR",
      .tooltip = "Lighting shader consumes the stochastic reflection (t25) instead of debug views. Keep OFF until raw stochastic output is validated. Reflections only appear on vanilla SSR-eligible pixels (MRT flag bit 1) - use Vanilla t24 debug view as the eligibility reference.",
      .labels = {"Off", "On"},
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "SSRApplyGain", .binding = &shader_injection.ssr_apply_gain,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 1.f, .label = "Apply Gain", .section = "Custom SSR",
      .tooltip = "DIAGNOSTIC ONLY: multiplies t25 alpha for visibility. Return to 1.0 once composition is proven - not a quality setting.",
      .min = 0.25f, .max = 8.f, .format = "%.2f",
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "SSRDiagnostics", .binding = &shader_injection.ssr_diagnostics,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 1.f, .label = "Diagnostics", .section = "Custom SSR",
      .tooltip = "Probe atomics + heavy debug payloads. OFF = production tracing cost only (Trace Stats requires ON).",
      .labels = {"Off", "On"},
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "SSRRayCount", .binding = &shader_injection.ssr_ray_count,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 1.f, .label = "Ray Count", .section = "Custom SSR",
      .tooltip = "Stochastic rays per pixel (1-4). Heuristic VNDF-weighted accumulation — not yet a physically unbiased MC estimator.",
      .min = 1.f, .max = 4.f, .format = "%.0f",
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "SSRRadianceSource", .binding = &shader_injection.ssr_radiance_source,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 0.f, .label = "Radiance Source", .section = "Custom SSR",
      .tooltip = "Reflected color source: ColorTexture t0 = current frame but may contain incompletely lit world geometry. BackBuffer = fully lit final image, one frame stale.",
      .labels = {"ColorTexture t0", "BackBuffer"},
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "SSRRoughInterpretation", .binding = &shader_injection.ssr_rough_interp,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 0.f, .label = "Roughness Interpretation", .section = "Custom SSR",
      .tooltip = "How DeferredParam.roughness maps to GGX alpha: Perceptual -> alpha=rough^2 (matches our shipped lighting BRDF), Already Alpha -> alpha=rough. Default verified against our lighting replacement's specular math.",
      .labels = {"Perceptual (a=r^2)", "Already Alpha"},
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    // —— Phase R1 spatial reconstruction ——
    new renodx::utils::settings::Setting{
      .key = "SSRResolveEnable", .binding = &shader_injection.ssr_resolve_enable,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 0.f, .label = "Spatial Resolve", .section = "Custom SSR",
      .tooltip = "Phase R1: edge-aware 2px spatial reconstruction of the raw stochastic result. Alpha stays raw center confidence. Route production t25 through the resolved output when ON.",
      .labels = {"Off", "On"},
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "SSRResolveRadius", .binding = &shader_injection.ssr_resolve_radius,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 8.f, .label = "Resolve Radius", .section = "Custom SSR",
      .tooltip = "Maximum spatial resolve radius in pixels. Radius scales with roughness^2 so mirror-like surfaces get near-zero blur.",
      .min = 1.f, .max = 16.f, .format = "%.0f",
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "SSRDepthSigma", .binding = &shader_injection.ssr_depth_sigma,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.1f, .label = "Depth Sigma", .section = "Custom SSR",
      .tooltip = "View-Z similarity sigma for spatial resolve edge stops.",
      .min = 0.01f, .max = 1.0f, .format = "%.2f",
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "SSRNormalSigma", .binding = &shader_injection.ssr_normal_sigma,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 32.f, .label = "Normal Sigma", .section = "Custom SSR",
      .tooltip = "Normal similarity exponent for spatial resolve.",
      .min = 1.f, .max = 128.f, .format = "%.0f",
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "SSRSameSurfaceReject", .binding = &shader_injection.ssr_same_surface_reject,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 0.f, .label = "Same Surface Reject", .section = "Custom SSR",
      .tooltip = "Reject candidates that hit the same planar surface as the origin pixel (normalSim >= threshold AND planeDelta <= threshold). Use Same Surface debug view to verify classification before enabling.",
      .labels = {"Off", "On"},
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "SSRPlaneDeltaThreshold", .binding = &shader_injection.ssr_plane_delta_threshold,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.1f, .label = "Plane Delta Threshold", .section = "Custom SSR",
      .tooltip = "Perpendicular distance from hit position to origin surface plane. Candidates within this distance are classified same-surface.",
      .min = 0.001f, .max = 1.0f, .format = "%.3f",
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "SSRMirrorBias", .binding = &shader_injection.ssr_mirror_bias,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.25f, .label = "Mirror Bias", .section = "Custom SSR",
      .tooltip = "Bias VNDF samples toward the mirror direction (filtered importance sampling). 0=pure VNDF distribution, 1=exact mirror only. Higher values reduce noise on semi-rough surfaces.",
      .min = 0.f, .max = 1.0f, .format = "%.2f",
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    // —— CPU Optimizations ——
    new renodx::utils::settings::Setting{
      .key = "SSRProbeAuto", .binding = &shader_injection.ssr_probe_auto,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 1.f, .label = "Compare Probe Auto", .section = "Custom SSR",
      .tooltip = "Auto-select the probe pixel: while Stochastic is OFF, freezes the brightest mirror-hit pixel (conf>=0.8, max(RGB)>=0.25) and holds it for the Stochastic ON comparison. Overrides Compare Probe X/Y when a selection exists.",
      .labels = {"Off", "On"},
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "SSRProbePixelX", .binding = &shader_injection.ssr_probe_pixel_x,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 960.f, .label = "Compare Probe X", .section = "Custom SSR",
      .tooltip = "Read-only Mirror-vs-VNDF CompareProbe pixel X. Logged in TraceStats when Diagnostics ON.",
      .min = 0.f, .max = 7680.f, .format = "%.0f",
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "SSRProbePixelY", .binding = &shader_injection.ssr_probe_pixel_y,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 540.f, .label = "Compare Probe Y", .section = "Custom SSR",
      .tooltip = "Read-only Mirror-vs-VNDF CompareProbe pixel Y. Logged in TraceStats when Diagnostics ON.",
      .min = 0.f, .max = 4320.f, .format = "%.0f",
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "CPUOptDeferredDispatch", .binding = &g_cpuopt_deferred_dispatch,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 0.f, .label = "Deferred Dispatch", .section = "CPU Opt",
      .tooltip = "Move GTVBAO/VBGI dispatch to OnPresent (1-frame latency). Kai-only, default OFF — avoids latency.",
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
    // —— Shadow Maps ——
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
    // —— CHSS Settings (enabled when ShadowFilterMethod = CHSS) ——
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
      .default_value = 16.f, .label = "Filter Samples", .section = "Shadow Maps",
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
      .default_value = 0.1f, .label = "Base Softness", .section = "Shadow Maps",
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
      .min = 0.1f, .max = 2.0f, .format = "%.1f",
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
      .default_value = 3.f, .label = "Min Filter Radius", .section = "Shadow Maps",
      .tooltip = "Guaranteed minimum PCF filter radius in shadow map texels. Prevents filter from collapsing. 0=off.",
      .min = 0.f, .max = 100.0f, .format = "%.0f",
      .is_enabled = []() { return shader_injection.shadow_filter_method > 1.5f; },
    .is_visible = []() { return !IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "ShadowCHSSPostBlur", .binding = &shader_injection.shadow_chss_post_blur,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 60.f, .label = "Post Blur", .section = "Shadow Maps",
      .tooltip = "Strength of the screen-space bilateral blur applied to the CHSS shadow mask. 0 = off (passthrough), 100 = full strength.",
      .min = 0.0f, .max = 100.0f, .format = "%.0f",
      .is_enabled = []() { return shader_injection.shadow_filter_method > 1.5f; },
    .is_visible = []() { return !IsKai() && IsAdvancedSettingsMode(); },
    },
    // —— Colored Shadow Penumbra (Improved mode) ——
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
    // —— Shadows (Kai) ——
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
    // ── Colored Shadow Penumbra (Kai) ──
    new renodx::utils::settings::Setting{
      .key = "KaiPenumbraMode", .binding = &shader_injection.shadow_edge_tint_kai,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 1.f, .label = "Colored Penumbra", .section = "Shadows",
      .tooltip = "Improved mode applies vibrance boost in shadow penumbra regions. No Falcom fallback on Kai.",
      .labels = {"Off", "Improved"},
      .is_visible = []() { return IsKai(); },
    },
    // —— Character hero light (Kai) ——
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
        .label = "IS-FAST Jitter/Noise: Dont enable if you are not using TAA/FSR/DLSS/XeSS.",
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
    // ── VBGI debug views removed — use GTVBAO debug View for GI inspection. ──

};

// ═══════════ GTVBAO Backend — implementation ═══════════

static void OnInitDevice(reshade::api::device* device) {
  reshade::log::message(reshade::log::level::info, "[sora-vanillaplus] Device init — addon loaded.");
  auto* d = renodx::utils::data::Create<DeviceData>(device);

  // 1×1 white fallback texture so t22 is never bound to null.
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

  reshade::log::message(reshade::log::level::info, "[GTVBAO] Device init — fallback SRV created.");
}

static void OnDestroyDevice(reshade::api::device* device) {
  auto* d = device->get_private_data<DeviceData>();
  if (d) {
    DestroyGTVBAOResources(device, d);
    DestroySSRResources(device, d);
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
    d->captured_depth_srv = {}; d->captured_ssao_srv = {};
    d->captured_scene_cbv_view = {};
    d->captured_scene_cbv = {}; d->captured_scene_cbv_valid = false;
    d->captured_scene_cbv_frame = UINT64_MAX;
    DestroyGTVBAOResources(sc->get_device(), d);
    DestroySSRResources(sc->get_device(), d);
  }
}

static void OnDestroySwapchain(reshade::api::swapchain* sc, bool resize) {
  auto* d = sc->get_device()->get_private_data<DeviceData>();
  if (!d) return;
  if (resize) {
    d->captured_depth_srv = {}; d->captured_ssao_srv = {};
    d->captured_scene_cbv_view = {};
    d->captured_scene_cbv = {}; d->captured_scene_cbv_valid = false;
    d->captured_scene_cbv_frame = UINT64_MAX;
    d->resources_created = false;
    DestroySSRResources(sc->get_device(), d);
    return;
  }
  DestroyGTVBAOResources(sc->get_device(), d);
  DestroySSRResources(sc->get_device(), d);
}

// ── Descriptor table helpers ──

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

// ── Scene CBV helper ──

static bool IsSceneCbvCandidateValid(reshade::api::device* device,
                                      const reshade::api::buffer_range& range) {
  if (!device || range.buffer.handle == 0u) return false;
  auto desc = device->get_resource_desc(range.buffer);
  if (desc.type != reshade::api::resource_type::buffer) return false;
  return desc.buffer.size >= kSceneCbMinimumBytes
      && desc.buffer.size <= (64u * 1024u)
      && range.offset + range.size <= desc.buffer.size;
}

// ── Push-descriptors event → capture lighting inputs (kai pattern) ──

static void OnPushDescriptorsCapture(
    reshade::api::command_list* cmd_list,
    reshade::api::shader_stage stages,
    reshade::api::pipeline_layout layout,
    uint32_t param_index,
    const reshade::api::descriptor_table_update& update) {
  if (!cmd_list) return;
  auto* device = cmd_list->get_device();
  auto* d = device->get_private_data<DeviceData>();
  if (!d) return;

  // ── Capture depth/SSAO/CBV — unconditional (register-based, kai-style). ──
  if (update.type == reshade::api::descriptor_type::texture_shader_resource_view) {
    auto* views = static_cast<const reshade::api::resource_view*>(update.descriptors);
  // Capture depth: t4 (Sora) or t3 (Kai) — ONLY from lighting shader, game-specific binding.
    uint32_t depthBinding = IsKai() ? kLightingDepthRegisterKai : kLightingDepthRegister;
    if (update.binding == depthBinding && update.count >= 1
        && views[0].handle != 0u) {
      auto* ss = renodx::utils::shader::GetCurrentState(cmd_list);
      if (ss) {
        uint32_t hash = renodx::utils::shader::GetCurrentPixelShaderHash(ss);
        if (IsLightingShader(hash)) {
          d->captured_depth_srv = views[0];
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
  // Capture SSAO: t5 (Sora) or t4 (Kai) — game-specific binding.
    uint32_t ssaoBinding = IsKai() ? kLightingSsaoRegisterKai : kLightingSsaoRegister;
    if (update.binding == ssaoBinding && update.count >= 1
        && views[0].handle != 0u) {
      d->captured_ssao_srv = views[0];
    }
    if ((update.binding == kLightingMrtNormalRegister) && update.count >= 1
        && views[0].handle != 0u) {
      auto* ss = renodx::utils::shader::GetCurrentState(cmd_list);
      if (ss) {
        uint32_t hash = renodx::utils::shader::GetCurrentPixelShaderHash(ss);
        if (IsLightingShader(hash)) {
          d->captured_mrt_normal_srv = views[0];
        }
      }
    }
    // Phase 3: Sora 2nd F0/specular texture (mrtTexture1 @ t2).
    if (update.binding == 2u && IsSora2nd() && update.count >= 1
        && views[0].handle != 0u) {
      auto* ss = renodx::utils::shader::GetCurrentState(cmd_list);
      if (ss) {
        uint32_t hash = renodx::utils::shader::GetCurrentPixelShaderHash(ss);
        if (hash == 0xCA3D8596u) {
          d->captured_mrt_spec_srv = views[0];
        }
      }
    }
    // Phase 3: Sora 2nd material-index texture (mrtTexture2 @ t3).
    // Gated to Sora2nd: Kai binds its depth buffer at the same register.
    if (update.binding == 3u && IsSora2nd() && update.count >= 1
        && views[0].handle != 0u) {
      auto* ss = renodx::utils::shader::GetCurrentState(cmd_list);
      if (ss) {
        uint32_t hash = renodx::utils::shader::GetCurrentPixelShaderHash(ss);
        if (hash == 0xCA3D8596u) {
          d->captured_mrt_material_srv = views[0];
        }
      }
    }
    // Capture t0 color texture — ONLY from the lighting shader (hash 0xFDAAF80E).
    // Unconditional capture would grab binding 0 from any shader, causing wrong colors.
    if (update.binding == 0u && update.count >= 1
        && views[0].handle != 0u) {
      auto* ss = renodx::utils::shader::GetCurrentState(cmd_list);
      if (ss) {
        uint32_t hash = renodx::utils::shader::GetCurrentPixelShaderHash(ss);
        if (IsLightingShader(hash)) {
          d->captured_color_srv = views[0];
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
          d->captured_scene_cbv = { buf, 0, desc.buffer.size };
          d->captured_scene_cbv_valid = true;
          d->captured_scene_cbv_frame = d->frame_index;
          d->captured_scene_cbv_view = cbv_views[0];
        }
      }
    }
  }
  // Phase 3: capture the deferred-parameter StructuredBuffer (Sora t6).
  if (update.type == reshade::api::descriptor_type::buffer_shader_resource_view
      || update.type == reshade::api::descriptor_type::shader_resource_view) {
    auto* ss = renodx::utils::shader::GetCurrentState(cmd_list);
    uint32_t hash = ss ? renodx::utils::shader::GetCurrentPixelShaderHash(ss) : 0u;
    if ((hash == 0xCA3D8596u || IsLightingShader(hash))
        && update.binding == 6u && update.count >= 1) {
      auto* views6 = static_cast<const reshade::api::resource_view*>(update.descriptors);
      if (views6[0].handle != 0u) {
        d->captured_deferred_params_srv = views6[0];
      }
    }
  }

  // ── Per-draw gating (only when GTVBAO or SSGI is on). ──
  if (shader_injection.gtvbao_mode < 0.5f) return;
  if (!(static_cast<uint32_t>(stages) & static_cast<uint32_t>(reshade::api::shader_stage::pixel))) return;
}

// ── Bind-descriptor-tables event → capture lighting inputs ──

static void OnBindDescriptorTables(
    reshade::api::command_list* cmd_list,
    reshade::api::shader_stage stages,
    reshade::api::pipeline_layout layout,
    uint32_t first, uint32_t count,
    const reshade::api::descriptor_table* tables) {
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

  if (shader_injection.gtvbao_mode < 0.5f) return;

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
        resolve_tex(kLightingDepthRegister, &d->captured_depth_srv);
        resolve_tex(kLightingSsaoRegister, &d->captured_ssao_srv);
        resolve_tex(kLightingDepthRegisterKai, &d->captured_depth_srv);
        resolve_tex(kLightingSsaoRegisterKai, &d->captured_ssao_srv);
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
          d->captured_scene_cbv_valid = true;
          d->captured_scene_cbv_frame = d->frame_index;
        }
      }
    }
  }
}

// ── GTVBAO CS Dispatch Fix: clears stale compute bindings that cause double volumetrics ──
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

// ── Present hook ──

static void OnPresent(reshade::api::command_queue* queue, reshade::api::swapchain* sc,
                       const reshade::api::rect*, const reshade::api::rect*,
                       uint32_t, const reshade::api::rect*) {
  auto* dev = queue->get_device();
  auto* cl = queue->get_immediate_command_list();
  auto* d = dev->get_private_data<DeviceData>();
  if (!d) return;
  d->frame_index++;

  // ── Basic mode startup guard: reset advanced-only settings to defaults if Basic is selected ──
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

  // Custom SSR (Sora 2nd) can dispatch here independently of GTVBAO.
  const bool sora_ssr_present_active = IsSora2nd() && shader_injection.ssr_mode > 0.5f;
  if (shader_injection.gtvbao_mode < 0.5f && !sora_ssr_present_active) return;
  if (d->frame_index <= kGTVBAOStartupGuardFrames) {
    if (d->frame_index == kGTVBAOStartupGuardFrames) {
      reshade::log::message(reshade::log::level::info,
        "[GTVBAO] Startup guard complete — dispatch begins next frame.");
    }
    return;
  }
  if (d->frame_index < d->resize_guard_until_frame) return;

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
    if (gw > 0u && gh > 0u
        && (!d->resources_created || too_small
            || gw != d->last_created_game_width
            || gh != d->last_created_game_height)) {
      CreateGTVBAOResources(dev, d, gw, gh);
      d->last_created_game_width = gw;
      d->last_created_game_height = gh;
      d->resources_created = true;
      // Custom SSR resources follow the same lifecycle (Sora 2nd only).
      if (IsSora2nd() && shader_injection.ssr_mode > 0.5f) {
        CreateSSRResources(dev, d, gw, gh);
        reshade::log::message(reshade::log::level::info,
          (std::string("[SSR] Resources created: ") +
           std::to_string(gw) + "x" + std::to_string(gh) +
           ", hiz mips=" + std::to_string(kSSRHizMipLevels)).c_str());
      }
      reshade::log::message(reshade::log::level::info,
        (std::string("[GTVBAO] Resources created: ") +
         std::to_string(d->working_width) + "x" +
         std::to_string(d->working_height) + " (depth=" +
         std::to_string(gw) + "x" + std::to_string(gh) + ")").c_str());
    }
  }
  // Use deferred snapshots from lighting draw (kai-style) — deferred dispatch only.
  // ── Light-buffer capture helper (runs after GTVBAO for multi-bounce feedback) ──
  auto capture_light_buffer_for_next_frame = [&]() {
    if (shader_injection.vbgi_enabled < 0.5f || !d->captured_light_buffer_texture.handle) return;
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

  // ── Phase 3: BackBuffer radiance capture for SSR Radiance Source = BackBuffer ──
  // Copies the fully-lit final image for NEXT frame's inline trace (the
  // standard one-frame-stale semantics; identical lifecycle to VBGI's
  // light-buffer capture). Created lazily on first use.
  auto ssr_capture_radiance = [&]() {
    if (!(IsSora2nd() && shader_injection.ssr_mode > 0.5f
          && shader_injection.ssr_radiance_source > 0.5f))
      return;
    auto bb = sc->get_back_buffer(0);
    if (!bb.handle) return;
    if (!d->ssr_radiance_copy_created) {
      auto bd = dev->get_resource_desc(bb);
      reshade::api::resource_desc rd = {};
      rd.type = reshade::api::resource_type::texture_2d;
      rd.texture = {bd.texture.width, bd.texture.height, 1, 1, bd.texture.format, 1};
      rd.heap = reshade::api::memory_heap::gpu_only;
      // Initial state = copy_dest: first barrier below then transitions to SRV.
      rd.usage = reshade::api::resource_usage::copy_dest
               | reshade::api::resource_usage::shader_resource;
      if (!dev->create_resource(rd, nullptr,
                                reshade::api::resource_usage::copy_dest,
                                &d->ssr_radiance_copy_texture))
        return;
      dev->create_resource_view(d->ssr_radiance_copy_texture,
                                reshade::api::resource_usage::shader_resource,
                                reshade::api::resource_view_desc(
                                    reshade::api::resource_view_type::texture_2d,
                                    bd.texture.format, 0, 1, 0, 1),
                                &d->ssr_radiance_copy_srv);
      d->ssr_radiance_copy_created = true;
      // One-shot readback verification staging (4x4).
      reshade::api::resource_desc rv = {};
      rv.type = reshade::api::resource_type::texture_2d;
      rv.texture = {4u, 4u, 1, 1, bd.texture.format, 1};
      rv.heap = reshade::api::memory_heap::gpu_to_cpu;
      rv.usage = reshade::api::resource_usage::copy_dest;
      dev->create_resource(rv, nullptr, reshade::api::resource_usage::copy_dest,
                           &d->ssr_radiance_verify_stage);
      std::ostringstream cl;
      cl << "[SSR] BackBuffer radiance capture created. fmt="
         << (uint32_t)bd.texture.format << " dims=" << bd.texture.width
         << "x" << bd.texture.height;
      reshade::log::message(reshade::log::level::info, cl.str().c_str());
    }
    const auto CSRC = reshade::api::resource_usage::copy_source;
    const auto CD = reshade::api::resource_usage::copy_dest;
    const auto SRVU = reshade::api::resource_usage::shader_resource;
    cl->barrier(bb, reshade::api::resource_usage::present, CSRC);
    cl->barrier(d->ssr_radiance_copy_texture, SRVU, CD);
    cl->copy_texture_region(bb, 0u, nullptr, d->ssr_radiance_copy_texture, 0u, nullptr);

    // One-shot content verification: read back a 4x4 sample and log averages.
    if (!d->ssr_radiance_verify_done && d->ssr_radiance_verify_stage.handle) {
      d->ssr_radiance_verify_done = true;
      cl->barrier(d->ssr_radiance_copy_texture, CD, CD);   // still copy-dest here
      cl->copy_texture_region(d->ssr_radiance_copy_texture, 0u, nullptr,
                              d->ssr_radiance_verify_stage, 0u, nullptr);
      reshade::api::subresource_data sd = {};
      if (dev->map_texture_region(d->ssr_radiance_verify_stage, 0u, nullptr,
                                  reshade::api::map_access::read_only, &sd) && sd.data) {
        double r = 0, g = 0, b = 0; uint32_t n = 0;
        for (uint32_t y = 0; y < 4u; ++y) {
          const auto* row = reinterpret_cast<const uint16_t*>(
              static_cast<const uint8_t*>(sd.data) + static_cast<size_t>(y) * sd.row_pitch);
          for (uint32_t x = 0; x < 4u; ++x) {
            // Half-float decode (RGBA16F).
            auto h2f = [](uint16_t h) -> float {
              uint32_t sign = (uint32_t)(h & 0x8000u) << 16;
              uint32_t e = (h >> 10) & 0x1Fu;
              uint32_t m = h & 0x03FFu;
              uint32_t bits;
              if (e == 0)      bits = sign;                              // zero
              else if (e == 31) bits = sign | 0x7F800000u | (m << 13);   // inf/nan
              else {
                uint32_t ee = e - 15u + 127u;
                bits = sign | (ee << 23) | (m << 13);
              }
              float f; memcpy(&f, &bits, 4); return f;
            };
            r += h2f(row[x * 4 + 0]); g += h2f(row[x * 4 + 1]); b += h2f(row[x * 4 + 2]);
            ++n;
          }
        }
        dev->unmap_texture_region(d->ssr_radiance_verify_stage, 0u);
        std::ostringstream v;
        v << "[SSR] radCopy verify: avgRGB=" << (r / n) << "," << (g / n)
          << "," << (b / n) << " over " << n << " samples"
          << (n ? "" : " (READBACK FAILED)");
        reshade::log::message(reshade::log::level::info, v.str().c_str());
      } else {
        dev->unmap_texture_region(d->ssr_radiance_verify_stage, 0u);
        reshade::log::message(reshade::log::level::warning,
            "[SSR] radCopy verify: map unavailable.");
      }
    }

    cl->barrier(d->ssr_radiance_copy_texture, CD, SRVU);
    cl->barrier(bb, CSRC, reshade::api::resource_usage::present);
  };

  // Inline dispatch active (deferred off) — GTVBAO runs during lighting pass, not here.
  if (!d->deferred_pending || !d->deferred_depth_srv.handle) {
    capture_light_buffer_for_next_frame();
    ssr_capture_radiance();
    return;
  }
  if (!d->deferred_scene_cbv_valid
      || (d->frame_index - d->deferred_scene_cbv_frame) > 1u) {
    capture_light_buffer_for_next_frame();
    ssr_capture_radiance();
    if (shader_injection.gtvbao_debug_logging > 0.5f) {
      reshade::log::message(reshade::log::level::warning,
                            "[GTVBAO] Dispatch skipped: no deferred scene CBV.");
    }
    return;
  }
  // Restore deferred snapshots as active captures for RunGTVBAO / RunVBGI.
  d->captured_depth_srv = d->deferred_depth_srv;
  d->captured_ssao_srv = d->deferred_ssao_srv;
  d->captured_mrt_normal_srv = d->deferred_mrt_normal_srv;
  d->captured_scene_cbv_view = d->deferred_scene_cbv_view;
  d->captured_scene_cbv = d->deferred_scene_cbv;
  d->captured_scene_cbv_valid = d->deferred_scene_cbv_valid;
  d->captured_scene_cbv_frame = d->deferred_scene_cbv_frame;
  d->deferred_pending = false;

  // GTVBAO reads proj_g directly from the game's scene CBV (b0) in-shader —
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
  if (shader_injection.gtvbao_mode > 0.5f) ok = RunGTVBAO(cl, d);

  // ── Custom SSR (Sora 2nd): deferred Hi-Z build + trace (mirrors GTVBAO deferral). ──
  // Requires the CPU-Opt "Deferred Dispatch" machinery to be active, since it
  // reuses its captured snapshots; otherwise the inline lighting-draw path
  // already ran this frame.
  if (sora_ssr_present_active && shader_injection.ssr_deferred_dispatch > 0.5f
      && d->ssr_resources_created && d->captured_depth_srv.handle) {
  RunSSRHiZ(cl, d);
  RunSSRTrace(cl, d);
  RunSSRRadiancePyramid(cl, d);
  // Phase R1: spatial reconstruction (gated by Spatial Resolve toggle).
  RunSSRResolve(cl, d);
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

  // ── GI is now integrated into GTVBAO main pass (visibility bitmask AO+GI). ──
  // The GI output (vbgi_denoised_srv) is produced during RunGTVBAO denoise pass.
  // No separate VBGI dispatch needed.

  // ── Capture light buffer for next frame's multi-bounce (after GI applied) ──
  capture_light_buffer_for_next_frame();
  ssr_capture_radiance();

  shader_injection.gtvbao_vbgi_bound = 0.f;  // Reset for next frame's SSAO pass
}

static bool OnBeforeLightingShaderDraw(reshade::api::command_list* cmd_list) {
  // IMPORTANT: returning false would BYPASS the draw (skip it entirely).
  shader_injection.gtvbao_dedicated_bound = 0.f;
  SyncISFASTToShaderInjection(cmd_list);  // keep IS-FAST mirrors in sync
  // Push IS-FAST noise texture for PCSS shadow jitter (t24 only — NO sampler push,
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

  // ── Kai sync: Character VBGI master toggle + PCSS jitter ──
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

  // Custom SSR (Sora 2nd) runs independently of GTVBAO.
  const bool sora_custom_ssr_active = IsSora2nd() && shader_injection.ssr_mode > 0.5f;
  const bool gtvbao_active = shader_injection.gtvbao_mode > 0.5f;
  if (!gtvbao_active && !sora_custom_ssr_active) return true;
  if (!cmd_list) return true;

  auto* dev = cmd_list->get_device();
  auto* dd = dev ? dev->get_private_data<DeviceData>() : nullptr;
  if (!dd) return true;

  // ── Deferred dispatch path: capture snapshots for OnPresent (kai-style). ──
  if (g_cpuopt_deferred_dispatch > 0.5f) {
    dd->deferred_depth_srv = dd->captured_depth_srv;
    dd->deferred_mrt_normal_srv = dd->captured_mrt_normal_srv;
    dd->deferred_scene_cbv_view = dd->captured_scene_cbv_view;
    dd->deferred_scene_cbv = dd->captured_scene_cbv;
    dd->deferred_scene_cbv_valid = dd->captured_scene_cbv_valid;
    dd->deferred_scene_cbv_frame = dd->captured_scene_cbv_frame;
    dd->deferred_pending = true;
  }

  // ── Inline dispatch: Run GTVBAO on this frame's command list (only when NOT deferred). ──
  if (gtvbao_active && g_cpuopt_deferred_dispatch < 0.5f) {
    if (dd->captured_depth_srv.handle && dd->captured_scene_cbv_valid
        && dd->ao_term_a_srv.handle) {
      auto* cs = renodx::utils::state::GetCurrentState(cmd_list);
      renodx::utils::state::CommandListState prev = {};
      if (cs) prev = *cs;

      bool ok = RunGTVBAO(cmd_list, dd);

      ApplyGTVBAOCSDispatchFix(cmd_list, cs, prev);
      (void)ok;
    }
  }

  // ── Custom SSR (Sora 2nd): inline Hi-Z build + trace (default — no added latency). ──
  if (sora_custom_ssr_active && shader_injection.ssr_deferred_dispatch < 0.5f) {
    if (dd->captured_depth_srv.handle && dd->ssr_resources_created) {
      auto* cs = renodx::utils::state::GetCurrentState(cmd_list);
      renodx::utils::state::CommandListState prev = {};
      if (cs) prev = *cs;

  RunSSRHiZ(cmd_list, dd);
  RunSSRTrace(cmd_list, dd);
  RunSSRRadiancePyramid(cmd_list, dd);
  // Phase R1: spatial reconstruction (gated by Spatial Resolve toggle).
  RunSSRResolve(cmd_list, dd);

      ApplyGTVBAOCSDispatchFix(cmd_list, cs, prev);
    }
  }

  // Push the GTVBAO AO result at t22.
  // In inline mode: fresh from dispatch above.
  // In deferred mode: result from previous frame's OnPresent dispatch.
  // The buffer is tracked by RunGTVBAO (gtvbao_final_in_b) — parity depends on
  // the active denoiser path (legacy / R2 two-stage / à-trous).
  if (gtvbao_active) {
    reshade::api::resource_view srv = dd->gtvbao_final_in_b
        ? dd->ao_term_b_srv : dd->ao_term_a_srv;
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

  // ── SSGI push t23 (GI is produced by RunGTVBAO) ──
  if (gtvbao_active) {
  shader_injection.gtvbao_vbgi_bound = 0.f;
  shader_injection.gtvbao_vbgi_debug = 0.f;

  // Determine what to push to t23.
  reshade::api::resource_view push_srv = {};
  bool do_push = false;
  bool debug_replace = false;

  // VBGI debug views (1=Raw GI, 2=Denoised GI, 3=Light Buffer, 4=Accumulated, 5=Samples).
  if (shader_injection.vbgi_debug_view > 0.5f) {
    int dv = (int)shader_injection.vbgi_debug_view;
    if (dv == 1)      push_srv = dd->vbgi_output_srv;
    else if (dv == 2) push_srv = dd->vbgi_denoised_srv;
    else if (dv == 3) push_srv = dd->captured_color_srv.handle
        ? dd->captured_color_srv : dd->captured_light_buffer_srv;
    else if (dv == 4) push_srv = dd->multibounce_srv.handle
        ? dd->multibounce_srv : dd->fallback_srv;
    else if (dv == 5) push_srv = dd->debug_srv.handle
        ? dd->debug_srv : dd->fallback_srv;
    do_push = true;
    debug_replace = true;
  }
  // Bitmask debug views 6-8: push dedicated debug UAV output.
  else if (shader_injection.gtvbao_debug_view > 5.5f && shader_injection.gtvbao_debug_view < 8.5f) {
    push_srv = dd->debug_srv;
    do_push = true;
    debug_replace = true;
  }
  // Normal SSGI: push denoised GI.
  else if (shader_injection.vbgi_enabled > 0.5f) {
    push_srv = dd->vbgi_denoised_srv;
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

  // ── Custom SSR (Sora 2nd): t25 transport. ──────────────────────────────
  // Debug View != Off -> debug/ray textures via the PS early-out.
  // Apply To Scene ON  -> ray_result consumed by the specular blend.
  // BOTH require ssr_custom_bound=1 or the PS ignores t25 entirely.
  shader_injection.ssr_custom_bound = 0.f;
  if (sora_custom_ssr_active) {
    const bool debug_active = shader_injection.ssr_debug_view > 0.5f;
    const bool trace_view = shader_injection.ssr_debug_view > 4.5f;
    // Phase 3.Fix20 partial-revert: production t25 = raw ssr_ray_result_srv.
    // The resolve output is no longer consumed in production.
    reshade::api::resource_view ssr_push_srv = dd->ssr_ray_result_srv;   // production raw
    // Phase R1: resolved output takes precedence when resolve ran this frame.
    if (dd->ssr_resolved_this_frame && dd->ssr_output_srv.handle)
      ssr_push_srv = dd->ssr_output_srv;
    if (debug_active && !trace_view) ssr_push_srv = dd->ssr_debug_srv;   // HiZ views 1..4
    if (!ssr_push_srv.handle) ssr_push_srv = dd->fallback_srv;
    if (ssr_push_srv.handle) {
      cmd_list->push_descriptors(
          reshade::api::shader_stage::pixel,
          reshade::api::pipeline_layout{0},
          0,
          reshade::api::descriptor_table_update{
              {}, kLightingSSRCustomRegister, 0, 1,
              reshade::api::descriptor_type::texture_shader_resource_view, &ssr_push_srv,
          });
      shader_injection.ssr_custom_bound = 1.f;
      // One-shot integration diagnostic on Apply state transitions.
      static int s_last_apply = -1;
      const int apply_now = (int)shader_injection.ssr_apply;
      if (apply_now != s_last_apply && shader_injection.ssr_log_config > 0.5f) {
        s_last_apply = apply_now;
        auto rt = dd->ssr_ray_result_texture.handle
            ? dev->get_resource_desc(dd->ssr_ray_result_texture)
            : reshade::api::resource_desc{};
        reshade::log::message(reshade::log::level::info,
            (std::string("[SSR] Integrate: apply=") + std::to_string(apply_now)
             + " bound=1 radSrc="
             + std::to_string((int)shader_injection.ssr_radiance_source)
             + " rayTex=" + std::to_string(rt.texture.width) + "x"
             + std::to_string(rt.texture.height)
             + " (reflections require vanilla SSR-eligible pixels: MRT flag bit 1)"
            ).c_str());
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

// ── Resource create / destroy ──

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
    reshade::api::sampler_desc ld = {};
    ld.filter = reshade::api::filter_mode::min_mag_mip_linear;
    ld.address_u = ld.address_v = ld.address_w = reshade::api::texture_address_mode::clamp;
    dev->create_sampler(ld, &d->ssr_linear_sampler);
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

  mk(w, h, reshade::api::format::r32_uint, &d->ao_term_a_texture, &d->ao_term_a_srv, &d->ao_term_a_uav);
  mk(w, h, reshade::api::format::r32_uint, &d->ao_term_b_texture, &d->ao_term_b_srv, &d->ao_term_b_uav);
  mk(w, h, reshade::api::format::r32_uint, &d->history_ao_texture_a, &d->history_ao_srv_a, &d->history_ao_uav_a);
  mk(w, h, reshade::api::format::r32_uint, &d->history_ao_texture_b, &d->history_ao_srv_b, &d->history_ao_uav_b);
  mk(w, h, reshade::api::format::r32_float, &d->edges_texture, &d->edges_srv, &d->edges_uav);
  mk(gw, gh, reshade::api::format::r8g8b8a8_unorm, &d->composite_texture, &d->composite_srv, &d->composite_uav);

  // ── GI resources (same resolution as AO per user preference) ──
  mk(w, h, reshade::api::format::r16g16b16a16_float,
     &d->vbgi_output_texture, &d->vbgi_output_srv, &d->vbgi_output_uav);
  mk(w, h, reshade::api::format::r16g16b16a16_float,
     &d->vbgi_denoised_texture, &d->vbgi_denoised_srv, &d->vbgi_denoised_uav);
  // Foliage mask (full-res R8_UINT)
  mk(w, h, reshade::api::format::r8_uint,
     &d->foliage_mask_texture, &d->foliage_mask_srv, &d->foliage_mask_uav);
  mk(w, h, reshade::api::format::r8g8b8a8_unorm,
     &d->debug_texture, &d->debug_srv, &d->debug_uav);
  // À-trous normal pre-decode target (full-res RGBA16F)
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
  // GI resources (now integrated — no separate VBGI pipeline)
  dv(d->vbgi_output_srv); dv(d->vbgi_output_uav); dr(d->vbgi_output_texture);
  dv(d->vbgi_denoised_srv); dv(d->vbgi_denoised_uav); dr(d->vbgi_denoised_texture);
  dv(d->captured_light_buffer_srv); dr(d->captured_light_buffer_texture);
  dv(d->multibounce_srv); dv(d->multibounce_uav); dr(d->multibounce_texture);
  dv(d->foliage_mask_srv); dv(d->foliage_mask_uav); dr(d->foliage_mask_texture);
  dp(d->foliage_mask_pipeline); dl(d->foliage_mask_layout);
  dv(d->debug_srv); dv(d->debug_uav); dr(d->debug_texture);
  dp(d->multibounce_pipeline); dl(d->multibounce_layout);
  DestroyGTVBAODescriptorTables(dev, &d->multibounce_tables);
  // À-trous wavelet filter + normal pre-decode
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
  // Do NOT clear captured_depth_srv / captured_scene_cbv —
  // those reference game-owned resources that survive recreation.
  d->resources_created = false;
}

// ═══════════ Custom SSR (Sora 2nd) — Phase 1: Hi-Z pyramid ═══════════

static void DestroySSRResources(reshade::api::device* dev, DeviceData* d) {
  if (!dev || !d) return;
  auto dv = [&](reshade::api::resource_view& v) { if (v.handle) { dev->destroy_resource_view(v); v = {}; } };
  auto dr = [&](reshade::api::resource& r) { if (r.handle) { dev->destroy_resource(r); r = {}; } };
  for (auto& u : d->ssr_hiz_uavs) dv(u);
  dv(d->ssr_hiz_srv);
  dr(d->ssr_hiz_texture);
  dv(d->ssr_hiz_scratch_srv);
  dr(d->ssr_hiz_scratch_texture);
  dv(d->ssr_debug_srv); dv(d->ssr_debug_uav);
  dr(d->ssr_debug_texture);
  dv(d->ssr_ray_result_srv); dv(d->ssr_ray_result_uav);
  dr(d->ssr_ray_result_texture);
  dv(d->ssr_ray_meta_srv); dv(d->ssr_ray_meta_uav); dr(d->ssr_ray_meta_texture);
  dv(d->ssr_rad_pyr_srv); dr(d->ssr_rad_pyr_texture);
  for (auto& u : d->ssr_rad_pyr_uavs) dv(u);
  dv(d->ssr_rad_scratch_srv); dr(d->ssr_rad_scratch_texture);
  d->ssr_rad_created = false; d->ssr_rad_pyr_valid = false;
  dv(d->ssr_stats_uav);
  dr(d->ssr_stats_texture);
  dr(d->ssr_stats_stage);
  dr(d->ssr_probe_stage);
  dv(d->ssr_radiance_copy_srv); dr(d->ssr_radiance_copy_texture);
  d->ssr_radiance_copy_created = false;
  dr(d->ssr_radiance_verify_stage);
  if (d->ssr_linear_sampler.handle) { dev->destroy_sampler(d->ssr_linear_sampler); d->ssr_linear_sampler = {}; }
  dv(d->ssr_output_srv); dv(d->ssr_output_uav); dr(d->ssr_output_texture);
  // Resolve pipeline/layout/tables are resolution-independent — kept alive.
  for (auto& s : d->ssr_verify_stages) dr(s);
  // Pipelines/layouts/tables are resolution-independent — kept alive across resizes.
  d->ssr_resources_created = false;
}

static void CreateSSRResources(reshade::api::device* dev, DeviceData* d,
                               uint32_t gw, uint32_t gh) {
  DestroySSRResources(dev, d);
  uint32_t w = gw < 64u ? 64u : gw;
  uint32_t h = gh < 64u ? 64u : gh;

  {
    reshade::api::resource_desc rd = {};
    rd.type = reshade::api::resource_type::texture_2d;
    rd.texture = {w, h, 1, (uint16_t)kSSRHizMipLevels, reshade::api::format::r32_float, 1};
    rd.heap = reshade::api::memory_heap::gpu_only;
    rd.usage = reshade::api::resource_usage::shader_resource
             | reshade::api::resource_usage::unordered_access
             | reshade::api::resource_usage::copy_source;
    dev->create_resource(rd, nullptr, reshade::api::resource_usage::shader_resource,
                         &d->ssr_hiz_texture);
    dev->create_resource_view(d->ssr_hiz_texture, reshade::api::resource_usage::shader_resource,
                              reshade::api::resource_view_desc(reshade::api::resource_view_type::texture_2d,
                                                               reshade::api::format::r32_float,
                                                               0, kSSRHizMipLevels, 0, 1),
                              &d->ssr_hiz_srv);
    for (uint32_t m = 0; m < kSSRHizMipLevels; ++m)
      dev->create_resource_view(d->ssr_hiz_texture, reshade::api::resource_usage::unordered_access,
                                reshade::api::resource_view_desc(reshade::api::resource_view_type::texture_2d,
                                                                 reshade::api::format::r32_float, m, 1, 0, 1),
                                &d->ssr_hiz_uavs[m]);
  }
  {
    reshade::api::resource_desc rd = {};
    rd.type = reshade::api::resource_type::texture_2d;
    rd.texture = {w, h, 1, 1, reshade::api::format::r32_float, 1};
    rd.heap = reshade::api::memory_heap::gpu_only;
    // Scratch is read-only from shader perspective: SRV only. It is refreshed
    // exclusively via copy_texture_region, which has no SRV/UAV exclusivity rule.
    rd.usage = reshade::api::resource_usage::shader_resource
             | reshade::api::resource_usage::copy_dest;
    dev->create_resource(rd, nullptr, reshade::api::resource_usage::shader_resource,
                         &d->ssr_hiz_scratch_texture);
    reshade::api::resource_view_desc vd(reshade::api::resource_view_type::texture_2d,
                                        reshade::api::format::r32_float, 0, 1, 0, 1);
    dev->create_resource_view(d->ssr_hiz_scratch_texture,
                              reshade::api::resource_usage::shader_resource, vd,
                              &d->ssr_hiz_scratch_srv);
  }
  {
    // Readback staging for the one-shot numeric verification (debug mode 4):
    // one texture per chain mip 0..3 so each level can be checked against its
    // immediate predecessor with reducer-identical semantics.
    for (uint32_t i = 0; i < 4; ++i) {
      const uint32_t sw = (w >> i) > 0 ? (w >> i) : 1u;
      const uint32_t sh = (h >> i) > 0 ? (h >> i) : 1u;
      reshade::api::resource_desc rd = {};
      rd.type = reshade::api::resource_type::texture_2d;
      rd.texture = {sw, sh, 1, 1, reshade::api::format::r32_float, 1};
      rd.heap = reshade::api::memory_heap::gpu_to_cpu;
      rd.usage = reshade::api::resource_usage::copy_dest;
      dev->create_resource(rd, nullptr, reshade::api::resource_usage::copy_dest,
                           &d->ssr_verify_stages[i]);
    }
  }
  {
    reshade::api::resource_desc rd = {};
    rd.type = reshade::api::resource_type::texture_2d;
    rd.texture = {w, h, 1, 1, reshade::api::format::r8g8b8a8_unorm, 1};
    rd.heap = reshade::api::memory_heap::gpu_only;
    rd.usage = reshade::api::resource_usage::shader_resource
             | reshade::api::resource_usage::unordered_access;
    dev->create_resource(rd, nullptr, reshade::api::resource_usage::shader_resource,
                         &d->ssr_debug_texture);
    reshade::api::resource_view_desc vd(reshade::api::resource_view_type::texture_2d,
                                        reshade::api::format::r8g8b8a8_unorm, 0, 1, 0, 1);
    dev->create_resource_view(d->ssr_debug_texture,
                              reshade::api::resource_usage::shader_resource, vd,
                              &d->ssr_debug_srv);
    dev->create_resource_view(d->ssr_debug_texture,
                              reshade::api::resource_usage::unordered_access, vd,
                              &d->ssr_debug_uav);
  }
  {
    // Phase 2 trace output (RGBA16F: rgb = debug payload, a = confidence).
    reshade::api::resource_desc rd = {};
    rd.type = reshade::api::resource_type::texture_2d;
    rd.texture = {w, h, 1, 1, reshade::api::format::r16g16b16a16_float, 1};
    rd.heap = reshade::api::memory_heap::gpu_only;
    rd.usage = reshade::api::resource_usage::shader_resource
             | reshade::api::resource_usage::unordered_access;
    dev->create_resource(rd, nullptr, reshade::api::resource_usage::shader_resource,
                         &d->ssr_ray_result_texture);
    reshade::api::resource_view_desc vd(reshade::api::resource_view_type::texture_2d,
                                        reshade::api::format::r16g16b16a16_float, 0, 1, 0, 1);
      dev->create_resource_view(d->ssr_ray_result_texture,
                                reshade::api::resource_usage::shader_resource, vd,
                                &d->ssr_ray_result_srv);
      dev->create_resource_view(d->ssr_ray_result_texture,
                                reshade::api::resource_usage::unordered_access, vd,
                                &d->ssr_ray_result_uav);
      // Phase R2C: per-ray estimator metadata (RGBA16F:
      // r=pdf_L, g=hit view-distance, ba=octahedral view-space direction).
      reshade::api::resource_desc rm = {};
      rm.type = reshade::api::resource_type::texture_2d;
      rm.texture = {w, h, 1, 1, reshade::api::format::r16g16b16a16_float, 1};
      rm.heap = reshade::api::memory_heap::gpu_only;
      rm.usage = reshade::api::resource_usage::shader_resource
               | reshade::api::resource_usage::unordered_access;
      dev->create_resource(rm, nullptr, reshade::api::resource_usage::shader_resource,
                           &d->ssr_ray_meta_texture);
      dev->create_resource_view(d->ssr_ray_meta_texture,
                                reshade::api::resource_usage::shader_resource, vd,
                                &d->ssr_ray_meta_srv);
      dev->create_resource_view(d->ssr_ray_meta_texture,
                                reshade::api::resource_usage::unordered_access, vd,
                                &d->ssr_ray_meta_uav);
    }
    {
      // Phase R3: filtered radiance pyramid (RGBA16F, 8 mips) + scratch.
      reshade::api::resource_desc rp = {};
      rp.type = reshade::api::resource_type::texture_2d;
      rp.texture = {w, h, 1, SSR_RAD_MIP_LEVELS, reshade::api::format::r16g16b16a16_float, 1};
      rp.heap = reshade::api::memory_heap::gpu_only;
      rp.usage = reshade::api::resource_usage::shader_resource
               | reshade::api::resource_usage::unordered_access;
      if (dev->create_resource(rp, nullptr, reshade::api::resource_usage::shader_resource,
                               &d->ssr_rad_pyr_texture)) {
        if (shader_injection.ssr_log_init > 0.5f)
          reshade::log::message(reshade::log::level::info,
          "[SSR] R3 init: pyr tex created");
        bool views_ok = true;
        dev->create_resource_view(d->ssr_rad_pyr_texture,
                                  reshade::api::resource_usage::shader_resource,
                                  reshade::api::resource_view_desc(
                                      reshade::api::resource_view_type::texture_2d,
                                      reshade::api::format::r16g16b16a16_float, 0, SSR_RAD_MIP_LEVELS, 0, 1),
                                  &d->ssr_rad_pyr_srv);
        if (!d->ssr_rad_pyr_srv.handle) views_ok = false;
        for (uint32_t m = 0; m < SSR_RAD_MIP_LEVELS; ++m) {
          // NOTE: uav array covers mips 0..7 (index == mip). The [-1] here was
          // the startup-crash OOB write found by the Stage-0b-style triage.
          dev->create_resource_view(d->ssr_rad_pyr_texture,
                                    reshade::api::resource_usage::unordered_access,
                                    reshade::api::resource_view_desc(
                                        reshade::api::resource_view_type::texture_2d,
                                        reshade::api::format::r16g16b16a16_float, m, 1, 0, 1),
                                    &d->ssr_rad_pyr_uavs[m]);
          if (!d->ssr_rad_pyr_uavs[m].handle) views_ok = false;
        }
        if (views_ok && shader_injection.ssr_log_init > 0.5f)
          reshade::log::message(reshade::log::level::info,
            "[SSR] R3 init: srv+8 uavs created");

        // Scratch buffer — guarded creation (was unguarded).
        bool scr_ok = false;
        reshade::api::resource_desc rs = {};
        rs.type = reshade::api::resource_type::texture_2d;
        rs.texture = {w, h, 1, 1, reshade::api::format::r16g16b16a16_float, 1};
        rs.heap = reshade::api::memory_heap::gpu_only;
        rs.usage = reshade::api::resource_usage::shader_resource | reshade::api::resource_usage::unordered_access;
        reshade::api::resource scr_tmp = {};
        if (dev->create_resource(rs, nullptr, reshade::api::resource_usage::shader_resource,
                                 &scr_tmp)) {
          d->ssr_rad_scratch_texture = scr_tmp;
          dev->create_resource_view(d->ssr_rad_scratch_texture,
                                    reshade::api::resource_usage::shader_resource,
                                    reshade::api::resource_view_desc(
                                        reshade::api::resource_view_type::texture_2d,
                                        reshade::api::format::r16g16b16a16_float, 0, 1, 0, 1),
                                    &d->ssr_rad_scratch_srv);
          scr_ok = d->ssr_rad_scratch_srv.handle != 0u;
        }
        if (scr_ok && shader_injection.ssr_log_init > 0.5f)
          reshade::log::message(reshade::log::level::info,
            "[SSR] R3 init: scratch created");

        d->ssr_rad_created = views_ok && scr_ok &&
                             d->ssr_rad_pyr_srv.handle != 0u;
        if (d->ssr_rad_created && shader_injection.ssr_log_init > 0.5f)
          reshade::log::message(reshade::log::level::info,
            "[SSR] R3 init: complete");
        else {
          reshade::log::message(reshade::log::level::error,
            "[SSR] R3 init: FAILED - filtered radiance disabled (SSR continues without it)");
          // Degrade cleanly: release partials so nothing half-valid lingers.
          if (d->ssr_rad_pyr_srv.handle) { dev->destroy_resource_view(d->ssr_rad_pyr_srv); d->ssr_rad_pyr_srv = {}; }
          for (auto& u : d->ssr_rad_pyr_uavs)
            if (u.handle) { dev->destroy_resource_view(u); u = {}; }
          if (d->ssr_rad_pyr_texture.handle) { dev->destroy_resource(d->ssr_rad_pyr_texture); d->ssr_rad_pyr_texture = {}; }
          if (d->ssr_rad_scratch_srv.handle) { dev->destroy_resource_view(d->ssr_rad_scratch_srv); d->ssr_rad_scratch_srv = {}; }
          if (d->ssr_rad_scratch_texture.handle) { dev->destroy_resource(d->ssr_rad_scratch_texture); d->ssr_rad_scratch_texture = {}; }
        }
      } else {
        reshade::log::message(reshade::log::level::error,
          "[SSR] R3 init: pyr tex create FAILED - filtered radiance disabled");
      }
    }
    {
      // Phase 4: resolved output (RGBA16F).
      reshade::api::resource_desc rd = {};
      rd.type = reshade::api::resource_type::texture_2d;
      rd.texture = {w, h, 1, 1, reshade::api::format::r16g16b16a16_float, 1};
      rd.heap = reshade::api::memory_heap::gpu_only;
      rd.usage = reshade::api::resource_usage::shader_resource
               | reshade::api::resource_usage::unordered_access;
      dev->create_resource(rd, nullptr, reshade::api::resource_usage::shader_resource,
                           &d->ssr_output_texture);
      reshade::api::resource_view_desc vd(reshade::api::resource_view_type::texture_2d,
                                          reshade::api::format::r16g16b16a16_float, 0, 1, 0, 1);
      dev->create_resource_view(d->ssr_output_texture,
                                reshade::api::resource_usage::shader_resource, vd,
                                &d->ssr_output_srv);
      dev->create_resource_view(d->ssr_output_texture,
                                reshade::api::resource_usage::unordered_access, vd,
                                &d->ssr_output_uav);
    }
  {
    // Phase 2.1 atomic funnel counters (8x13 R32_UINT) + readback staging.
    reshade::api::resource_desc rd = {};
    rd.type = reshade::api::resource_type::texture_2d;
    rd.texture = {kSSRStatsWidth, kSSRStatsHeight, 1, 1, reshade::api::format::r32_uint, 1};
    rd.heap = reshade::api::memory_heap::gpu_only;
    rd.usage = reshade::api::resource_usage::unordered_access
             | reshade::api::resource_usage::copy_source;
    dev->create_resource(rd, nullptr, reshade::api::resource_usage::unordered_access,
                         &d->ssr_stats_texture);
    dev->create_resource_view(d->ssr_stats_texture,
                              reshade::api::resource_usage::unordered_access,
                              reshade::api::resource_view_desc(
                                  reshade::api::resource_view_type::texture_2d,
                                  reshade::api::format::r32_uint, 0, 1, 0, 1),
                              &d->ssr_stats_uav);
    reshade::api::resource_desc rs = {};
    rs.type = reshade::api::resource_type::texture_2d;
    rs.texture = {kSSRStatsWidth, kSSRStatsHeight, 1, 1, reshade::api::format::r32_uint, 1};
    rs.heap = reshade::api::memory_heap::gpu_to_cpu;
    rs.usage = reshade::api::resource_usage::copy_dest;
    dev->create_resource(rs, nullptr, reshade::api::resource_usage::copy_dest,
                         &d->ssr_stats_stage);
    // Phase 3.Fix16: full-res RGBA16F readback staging for probe auto-select.
    reshade::api::resource_desc rp = {};
    rp.type = reshade::api::resource_type::texture_2d;
    rp.texture = {w, h, 1, 1, reshade::api::format::r16g16b16a16_float, 1};
    rp.heap = reshade::api::memory_heap::gpu_to_cpu;
    rp.usage = reshade::api::resource_usage::copy_dest;
    dev->create_resource(rp, nullptr, reshade::api::resource_usage::copy_dest,
                         &d->ssr_probe_stage);
  }
  d->last_created_ssr_width = gw;
  d->last_created_ssr_height = gh;
  d->ssr_resources_created = true;
}

static bool CreateSSRPipelinesIfNeeded(reshade::api::device* dev, DeviceData* d) {
  using DR = reshade::api::descriptor_range;
  using DS = reshade::api::shader_stage;
  using DT = reshade::api::descriptor_type;
  using P = reshade::api::pipeline_layout_param;

  auto mkcs = [&](std::span<const uint8_t> bc,
                  reshade::api::pipeline_layout lo, reshade::api::pipeline* out) -> bool {
    if (bc.empty() || !lo.handle) return false;
    if (out->handle != 0u) return true;  // ensure-style: create once
    reshade::api::shader_desc sd = {};
    sd.code = bc.data(); sd.code_size = bc.size(); sd.entry_point = "main";
    reshade::api::pipeline_subobject so = {reshade::api::pipeline_subobject_type::compute_shader, 1, &sd};
    return dev->create_pipeline(lo, 1, &so, out);
  };

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
    push_constants_range.count = kSSRPushConstantCount;
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

  // Trace variant: generic SRV range (textures t0..t6 + StructuredBuffer t7)
  // and a two-sampler table (point-clamp + linear-clamp).
  auto make_trace_layout = [&](reshade::api::pipeline_layout* out) -> bool {
    if (out->handle != 0u) return true;
    DR sampler_r = {0,0,0,2,DS::all_compute,1,DT::sampler};
    DR cbv_r     = {0,0,0,1,DS::all_compute,1,DT::constant_buffer};
    DR srv_r     = {0,0,0,9,DS::all_compute,1,DT::shader_resource_view};
    DR uav_r     = {0,0,0,3,DS::all_compute,1,DT::texture_unordered_access_view};   // R2C: rays+meta+stats
    reshade::api::constant_range push_constants_range = {};
    push_constants_range.binding = 0;
    push_constants_range.dx_register_index = 13;
    push_constants_range.dx_register_space = 0;
    push_constants_range.count = kSSRPushConstantCount;
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

  if (!make_layout(1u, 1u, &d->ssr_common_layout)) return false;
  if (!make_trace_layout(&d->ssr_trace_layout)) return false;
  // Resolve layout: Phase R1 Stage 0b probe — EXACT structural clone of the
  // proven trace layout (generic SRV type/count 8, two samplers, 2 UAVs).
  // The previous variant used texture-specific descriptor types; after two
  // failed red tests we eliminate every delta from the working path.
  {
    DR sampler_r = {0,0,0,2,DS::all_compute,1,DT::sampler};
    DR cbv_r     = {0,0,0,1,DS::all_compute,1,DT::constant_buffer};
    DR srv_r     = {0,0,0,9,DS::all_compute,1,DT::shader_resource_view};
    DR uav_r     = {0,0,0,3,DS::all_compute,1,DT::texture_unordered_access_view};
    reshade::api::constant_range push_constants_range = {};
    push_constants_range.binding = 0;
    push_constants_range.dx_register_index = 13;
    push_constants_range.dx_register_space = 0;
    push_constants_range.count = kSSRPushConstantCount;
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
    if (!dev->create_pipeline_layout(5, params, &d->ssr_resolve_layout))
      return false;
  }

  if (!mkcs(__ssr_hiz_base, d->ssr_common_layout, &d->ssr_hiz_base_pipeline)) return false;
  if (!mkcs(__ssr_hiz_reduce, d->ssr_common_layout, &d->ssr_hiz_reduce_pipeline)) return false;
  if (!mkcs(__ssr_debug, d->ssr_common_layout, &d->ssr_debug_pipeline)) return false;
  if (!mkcs(__ssr_trace, d->ssr_trace_layout, &d->ssr_trace_pipeline)) return false;
  if (!mkcs(__ssr_resolve, d->ssr_resolve_layout, &d->ssr_resolve_pipeline)) return false;
  // Phase R3 pipelines: NON-FATAL. A failure here previously aborted the
  // entire SSR subsystem (white views / dead TraceStats) via this function's
  // return-false contract. Degrade to no-pyramid instead.
  {
    static bool s_rad_fail_logged = false;
    bool ok = mkcs(__ssr_rad_base, d->ssr_common_layout, &d->ssr_rad_base_pipeline);
    ok = ok && mkcs(__ssr_rad_reduce, d->ssr_common_layout, &d->ssr_rad_reduce_pipeline);
    d->ssr_rad_pipelines_ok = ok;
    if (!ok && !s_rad_fail_logged) {
      s_rad_fail_logged = true;
      reshade::log::message(reshade::log::level::error,
        "[SSR] R3 init: rad pipeline creation FAILED - filtered radiance disabled (SSR continues)");
    }
    if (!s_rad_fail_logged && d->ssr_rad_pipelines_ok && !d->ssr_rad_pipelines_diag) {
      d->ssr_rad_pipelines_diag = true;
      reshade::log::message(reshade::log::level::info,
        "[SSR] R3 init: pipelines created");
    }
  }

  if (!EnsureGTVBAODescriptorTables(dev, d->ssr_common_layout, &d->ssr_common_tables)) return false;
  if (!EnsureGTVBAODescriptorTables(dev, d->ssr_trace_layout, &d->ssr_trace_tables)) return false;
  if (!EnsureGTVBAODescriptorTables(dev, d->ssr_resolve_layout, &d->ssr_resolve_tables)) return false;
  return true;
}

// Push-constant layout mirrors cb_ssr in ssr_common.hlsl exactly.
// Debug View slider stores a contiguous label index; trace modes live at
// codes 10..15, so indices 5..10 translate as index+5. Codes 0..4 frozen.
static float SSR_TranslateDebugView(float slider_value) {
  return (slider_value > 4.5f) ? (slider_value + 5.0f) : slider_value;
}

// Phase 3.Fix19: Raw-vs-Resolved split view code (label idx 37 -> 42).
static bool SSR_IsRawVsResolvedView(float translated_debug) {
  return fabsf(translated_debug - 42.0f) < 0.25f;
}
static bool SSR_IsMotionVectorView(float translated_debug) {
  return fabsf(translated_debug - 43.0f) < 0.25f;
}

static std::array<float, kSSRPushConstantCount> BuildSSRPushConstants(
    DeviceData* data, float scratch_width, float scratch_height) {
  std::array<float, kSSRPushConstantCount> c = {};
  const float dbg_translated = SSR_TranslateDebugView(shader_injection.ssr_debug_view);
  c[0]  = shader_injection.ssr_mode;
  c[1]  = dbg_translated;
  c[2]  = shader_injection.ssr_debug_mip;
  c[3]  = std::max(1.0f, shader_injection.ssr_max_ray_distance);
  c[4]  = shader_injection.ssr_thickness;
  c[5]  = shader_injection.ssr_roughness_threshold;
  c[6]  = (float)((data ? data->frame_index : 0u) % 64u);
  c[7]  = std::clamp(shader_injection.ssr_max_traversal_steps, 16.0f, 512.0f);   // traversal budget (slider, R-budget A/B)
  c[8]  = shader_injection.ssr_mirror_bias;
  c[9]  = shader_injection.ssr_intensity;
  c[10] = shader_injection.ssr_denoise_radius;
  // Phase 3.Fix19: c11 (SSR_denoise_taps, unused by all shaders) carries the
  // Raw-vs-Resolved split flag for the resolve pass when view 42 is selected.
  c[11] = SSR_IsRawVsResolvedView(dbg_translated) ? 1.0f : 0.0f;
  // Phase R3: c12 (SSR_half_res_trace, reserved/unused) carries the filtered
  // radiance pyramid validity flag for the resolve pass.
  c[12] = (data != nullptr && data->ssr_rad_pyr_valid
           && shader_injection.ssr_radiance_source < 0.5f) ? 1.0f : 0.0f;
  c[13] = shader_injection.ssr_radiance_source;
  c[14] = std::max(1.0f, scratch_width);
  c[15] = std::max(1.0f, scratch_height);
  c[16] = shader_injection.ssr_bypass_validation;
  c[17] = shader_injection.ssr_forced_ray_mode;
  c[18] = shader_injection.ssr_normal_convention;
  c[19] = std::max(0.0f, shader_injection.ssr_self_hit_threshold);
  c[20] = shader_injection.ssr_backface_gate;
  c[21] = std::clamp(shader_injection.ssr_initial_advance_bias, 0.0f, 8.0f);
  c[22] = shader_injection.ssr_thickness_gate;
  c[23] = shader_injection.ssr_thickness_mode;
  c[24] = shader_injection.ssr_diagnostics;
  c[25] = std::max(0.0f, shader_injection.brdf_roughness_min);
  c[26] = std::clamp(shader_injection.brdf_roughness_max, 0.0f, 1.0f);
  // c13 already carries ssr_radiance_source (bound-time SRV selection mirror)
  c[27] = std::clamp(shader_injection.ssr_ray_count, 1.0f, 4.0f);
  c[28] = shader_injection.ssr_rough_interp;
  c[29] = shader_injection.ssr_stochastic;
  c[30] = shader_injection.ssr_eligibility_mode;
  c[31] = std::clamp(shader_injection.ssr_roughness_threshold, 0.0f, 1.0f);
  c[32] = std::clamp(shader_injection.ssr_resolve_radius, 1.0f, 16.0f);
  c[33] = std::max(0.01f, shader_injection.ssr_depth_sigma);
  c[34] = std::max(1.0f, shader_injection.ssr_normal_sigma);
  c[35] = std::max(0.01f, shader_injection.ssr_rough_sigma);
  c[36] = shader_injection.ssr_same_surface_reject;
  c[37] = std::max(0.001f, shader_injection.ssr_plane_delta_threshold);
  // NOTE (audit finding, unfixed): shader expects same_surface_reject at
  // c37 and plane_delta_threshold at c38 — these two writes are off by one.
  // Left untouched per Phase 3.Fix15 scope; flagged for separate approval.
  // Phase 3.Fix16: auto-selected frozen coords take precedence in auto mode.
  const bool probe_auto_sel = shader_injection.ssr_probe_auto > 0.5f &&
                              data != nullptr && data->ssr_probe_has_selection;
  c[39] = std::clamp(probe_auto_sel ? data->ssr_probe_frozen_x
                                    : shader_injection.ssr_probe_pixel_x,
                     0.0f, 7680.0f);
  c[40] = std::clamp(probe_auto_sel ? data->ssr_probe_frozen_y
                                    : shader_injection.ssr_probe_pixel_y,
                     0.0f, 4320.0f);
  // Phase R2E.2: RNG source flag — IS-FAST volume bound vs IGN fallback.
  c[41] = (data != nullptr && data->isfast_noise_srv.handle != 0u) ? 1.0f : 0.0f;
  return c;
}

// One-shot numeric Hi-Z verification (debug mode 4): reads back chain mips
// 0..3 and validates, for each level M in 1..3, every texel of mip M against
// the explicit 2x2 MIN of its mip-(M-1) children — replicating the GPU
// reducer exactly: floor-halving dimensions, child coords = dst*2 + {0,1},
// FLT_MAX padding outside the source region (odd/orphan edges).
static void SSR_VerifyHiZMips(reshade::api::command_list* cl, DeviceData* d) {
  if (!cl || !d) return;
  auto* dev = cl->get_device();
  bool have_all = true;
  for (const auto& s : d->ssr_verify_stages)
    if (!s.handle) have_all = false;
  if (!have_all) {
    reshade::log::message(reshade::log::level::warning,
        "[SSR] HiZ verify skipped: staging textures missing.");
    return;
  }
  const uint32_t w = d->working_width, h = d->working_height;
  const float kEmpty = 3.402823466e+38f;   // matches SSR_FLT_MAX in shaders

  // Floor-halving dims per mip (identical to reducer dispatch math).
  uint32_t mw[4], mh[4];
  mw[0] = w; mh[0] = h;
  for (uint32_t i = 1; i < 4; ++i) {
    mw[i] = std::max(1u, w >> i);
    mh[i] = std::max(1u, h >> i);
  }

  // Whole-subresource copies: dimensions match by construction.
  const auto SR = reshade::api::resource_usage::shader_resource;
  const auto CSRC = reshade::api::resource_usage::copy_source;
  cl->barrier(d->ssr_hiz_texture, SR, CSRC);
  for (uint32_t i = 0; i < 4; ++i)
    cl->copy_texture_region(d->ssr_hiz_texture, i, nullptr,
                            d->ssr_verify_stages[i], 0u, nullptr);

  struct MappedMip { reshade::api::subresource_data data; bool ok; };
  MappedMip mm[4] = {};
  for (uint32_t i = 0; i < 4; ++i)
    mm[i].ok = dev->map_texture_region(d->ssr_verify_stages[i], 0u, nullptr,
                                       reshade::api::map_access::read_only,
                                       &mm[i].data);
  cl->barrier(d->ssr_hiz_texture, CSRC, SR);
  if (!mm[0].ok || !mm[1].ok || !mm[2].ok || !mm[3].ok
      || !mm[0].data.data || !mm[1].data.data
      || !mm[2].data.data || !mm[3].data.data) {
    reshade::log::message(reshade::log::level::error,
        "[SSR] HiZ verify failed: map_texture_region unavailable.");
    for (uint32_t i = 0; i < 4; ++i)
      if (mm[i].ok) dev->unmap_texture_region(d->ssr_verify_stages[i], 0u);
    return;
  }

  auto row_of = [](const reshade::api::subresource_data& sd,
                   uint32_t y) -> const float* {
    return reinterpret_cast<const float*>(
        static_cast<const uint8_t*>(sd.data) + static_cast<size_t>(y) * sd.row_pitch);
  };

  // Chained check: each level against its immediate predecessor.
  for (uint32_t m = 1; m < 4; ++m) {
    const uint32_t sw = mw[m - 1], sh = mh[m - 1];   // source (parent) dims
    const uint32_t dw = mw[m], dh = mh[m];           // destination dims

    uint64_t mismatches = 0;
    float got = 0.f, want = 0.f, quad[4] = {0.f, 0.f, 0.f, 0.f};
    uint32_t qx = 0, qy = 0;
    bool first_saved = false;

    for (uint32_t y = 0; y < dh; ++y) {
      const bool has_row1 = (2u * y + 1u) < sh;
      const float* rowA = row_of(mm[m - 1].data, 2u * y);
      const float* rowB = has_row1 ? row_of(mm[m - 1].data, 2u * y + 1u) : nullptr;
      const float* rowD = row_of(mm[m].data, y);
      for (uint32_t x = 0; x < dw; ++x) {
        const bool has_col1 = (2u * x + 1u) < sw;
        const float a = rowA[2u * x];
        const float b = has_col1 ? rowA[2u * x + 1u] : kEmpty;
        const float c = has_row1 ? rowB[2u * x] : kEmpty;
        const float e = (has_row1 && has_col1) ? rowB[2u * x + 1u] : kEmpty;
        const float expected = (std::min)((std::min)(a, b), (std::min)(c, e));
        const float actual = rowD[x];
        if (actual != expected) {
          if (!first_saved) {
            got = actual; want = expected;
            qx = x; qy = y;
            quad[0] = a; quad[1] = b; quad[2] = c; quad[3] = e;
            first_saved = true;
          }
          ++mismatches;
        }
      }
    }

    std::ostringstream msg;
    if (mismatches == 0u) {
      msg << "[SSR] HiZ verify: mip" << m << " PASS (" << (uint64_t)dw * dh
          << " texels vs mip" << (m - 1) << " " << sw << "x" << sh << ")";
      reshade::log::message(reshade::log::level::info, msg.str().c_str());
    } else {
      msg << "[SSR] HiZ verify: mip" << m << " FAIL — " << mismatches << "/"
          << ((uint64_t)dw * dh) << " mismatches vs mip" << (m - 1)
          << ". first @ (" << qx << "," << qy << ") got=" << got
          << " want=" << want << " | mip" << (m - 1) << " quad [TL=" << quad[0]
          << " TR=" << quad[1] << " BL=" << quad[2] << " BR=" << quad[3] << "]";
      reshade::log::message(reshade::log::level::error, msg.str().c_str());
    }
  }

  for (uint32_t i = 0; i < 4; ++i)
    dev->unmap_texture_region(d->ssr_verify_stages[i], 0u);
}

// Builds the SSR Hi-Z min-pyramid from this frame's captured depth buffer.
// Sequential per-level passes (correctness first). Every pass binds exactly
// one UAV at slot 0 and never holds an SRV+UAV on the same resource:
//   base   : depth SRV -> hiz mip0 UAV
//   level L: hiz is COPY_SOURCE while scratch absorbs it, then scratch SRV ->
//            hiz mip L UAV. Scratch itself is never bound as a UAV anywhere.
static bool RunSSRHiZ(reshade::api::command_list* cl, DeviceData* d) {
  if (!cl || !d) return false;
  if (!d->captured_depth_srv.handle || !d->captured_scene_cbv_valid) return false;
  auto* dev = cl->get_device();
  if (!dev) return false;

  const int debug_mode = (int)(shader_injection.ssr_debug_view + 0.5f);
  if (debug_mode != 4) d->ssr_verify_done = false;   // re-arm when leaving mode 4

  if (!CreateSSRPipelinesIfNeeded(dev, d)) return false;
  if (!d->ssr_resources_created || !d->ssr_hiz_srv.handle) return false;

  const uint32_t w = d->working_width, h = d->working_height;
  if (w < 64u || h < 64u) return false;

  const auto CS = reshade::api::shader_stage::all_compute;
  const auto AC = reshade::api::pipeline_stage::all_compute;
  const auto UA = reshade::api::resource_usage::unordered_access;
  const auto SR = reshade::api::resource_usage::shader_resource;
  const auto CD = reshade::api::resource_usage::copy_dest;
  const auto CSRC = reshade::api::resource_usage::copy_source;

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

  // ── Pass 1a: base level (depth -> hiz mip0 only; single UAV binding) ──
  {
    cl->bind_pipeline(AC, d->ssr_hiz_base_pipeline);
    reshade::api::descriptor_table_update u[4] = {
      {{},0,0,1,reshade::api::descriptor_type::sampler,&d->point_clamp_sampler},
      {{},0,0,1,reshade::api::descriptor_type::constant_buffer,&d->captured_scene_cbv_view},
      {{},0,0,1,reshade::api::descriptor_type::texture_shader_resource_view,&d->captured_depth_srv},
      {{},0,0,1,reshade::api::descriptor_type::texture_unordered_access_view,&d->ssr_hiz_uavs[0]},
    };
    apply_descriptors(d->ssr_common_layout, &d->ssr_common_tables, 4, u);
    auto pc = BuildSSRPushConstants(d, (float)w, (float)h);
    cl->push_constants(CS, d->ssr_common_layout, kGtvbaoPushConstantsLayoutParam,
                       0, kSSRPushConstantCount, pc.data());
    cl->dispatch((w + 7) / 8, (h + 7) / 8, 1);
  }

  // ── Pass 1b: sequential per-level MIN reduction ──
  // Each iteration: copy prev level into scratch (no shader-visible conflict),
  // then reduce scratch SRV -> hiz mip L UAV.
  cl->bind_pipeline(AC, d->ssr_hiz_reduce_pipeline);
  for (uint32_t level = 1; level < kSSRHizMipLevels; ++level) {
    const uint32_t pw = (w >> (level - 1)) > 0 ? (w >> (level - 1)) : 1u;
    const uint32_t ph = (h >> (level - 1)) > 0 ? (h >> (level - 1)) : 1u;
    const uint32_t dw = (w >> level) > 0 ? (w >> level) : 1u;
    const uint32_t dh = (h >> level) > 0 ? (h >> level) : 1u;

    const reshade::api::subresource_box src_box = {0u, 0u, 0u, pw, ph, 1u};
    const reshade::api::subresource_box dst_box = {0u, 0u, 0u, pw, ph, 1u};
    cl->barrier(d->ssr_hiz_texture, UA, CSRC);
    cl->barrier(d->ssr_hiz_scratch_texture, SR, CD);
    cl->copy_texture_region(d->ssr_hiz_texture, level - 1u, &src_box,
                            d->ssr_hiz_scratch_texture, 0u, &dst_box);
    cl->barrier(d->ssr_hiz_scratch_texture, CD, SR);

    reshade::api::descriptor_table_update u[4] = {
      {{},0,0,1,reshade::api::descriptor_type::sampler,&d->point_clamp_sampler},
      {{},0,0,1,reshade::api::descriptor_type::constant_buffer,&d->captured_scene_cbv_view},
      {{},0,0,1,reshade::api::descriptor_type::texture_shader_resource_view,&d->ssr_hiz_scratch_srv},
      {{},0,0,1,reshade::api::descriptor_type::texture_unordered_access_view,&d->ssr_hiz_uavs[level]},
    };
    apply_descriptors(d->ssr_common_layout, &d->ssr_common_tables, 4, u);
    auto pc = BuildSSRPushConstants(d, (float)pw, (float)ph);
    cl->push_constants(CS, d->ssr_common_layout, kGtvbaoPushConstantsLayoutParam,
                       0, kSSRPushConstantCount, pc.data());
    cl->dispatch((dw + 7) / 8, (dh + 7) / 8, 1);
  }
  cl->barrier(d->ssr_hiz_texture, UA, SR);

  // ── Debug visualization (Phase 1 validation; HiZ views = slider codes 1..4) ──
  if (shader_injection.ssr_debug_view > 0.5f && shader_injection.ssr_debug_view < 4.5f
      && d->ssr_debug_pipeline.handle) {
    cl->bind_pipeline(AC, d->ssr_debug_pipeline);
    reshade::api::descriptor_table_update u[4] = {
      {{},0,0,1,reshade::api::descriptor_type::sampler,&d->point_clamp_sampler},
      {{},0,0,1,reshade::api::descriptor_type::constant_buffer,&d->captured_scene_cbv_view},
      {{},0,0,1,reshade::api::descriptor_type::texture_shader_resource_view,&d->ssr_hiz_srv},
      {{},0,0,1,reshade::api::descriptor_type::texture_unordered_access_view,&d->ssr_debug_uav},
    };
    apply_descriptors(d->ssr_common_layout, &d->ssr_common_tables, 4, u);
    auto pc = BuildSSRPushConstants(d, (float)w, (float)h);
    cl->push_constants(CS, d->ssr_common_layout, kGtvbaoPushConstantsLayoutParam,
                       0, kSSRPushConstantCount, pc.data());
    cl->dispatch((w + 7) / 8, (h + 7) / 8, 1);
    cl->barrier(d->ssr_debug_texture, UA, SR);
  }

  // ── One-shot numeric verification (debug mode 4): mip1..3 vs predecessors ──
  if (debug_mode == 4 && !d->ssr_verify_done && d->ssr_verify_stages[0].handle) {
    d->ssr_verify_done = true;
    SSR_VerifyHiZMips(cl, d);
  }

  return true;
}

// One-shot trace funnel statistics (Debug View 'Trace Stats'): snapshots the
// 8x4 atomic counter block and logs the aggregate percentages/averages.
static void SSR_LogTraceStats(reshade::api::command_list* cl, DeviceData* d) {
  if (!cl || !d) return;
  auto* dev = cl->get_device();
  if (!dev || !d->ssr_stats_texture.handle || !d->ssr_stats_stage.handle) {
    reshade::log::message(reshade::log::level::warning,
        "[SSR] TraceStats skipped: staging missing.");
    return;
  }
  const auto UA = reshade::api::resource_usage::unordered_access;
  const auto CSRC = reshade::api::resource_usage::copy_source;
  const auto CD = reshade::api::resource_usage::copy_dest;
  cl->barrier(d->ssr_stats_texture, UA, CSRC);
  cl->barrier(d->ssr_stats_stage, CD, CD);
  cl->copy_texture_region(d->ssr_stats_texture, 0u, nullptr,
                          d->ssr_stats_stage, 0u, nullptr);

  reshade::api::subresource_data sd = {};
  if (!dev->map_texture_region(d->ssr_stats_stage, 0u, nullptr,
                               reshade::api::map_access::read_only, &sd) || !sd.data) {
    reshade::log::message(reshade::log::level::error,
        "[SSR] TraceStats failed: map unavailable.");
    return;
  }
  uint32_t c[kSSRStatsHeight][kSSRStatsWidth] = {};
  for (uint32_t y = 0; y < kSSRStatsHeight; ++y) {
    const auto* row = reinterpret_cast<const uint32_t*>(
        static_cast<const uint8_t*>(sd.data) + static_cast<size_t>(y) * sd.row_pitch);
    memcpy(c[y], row, kSSRStatsWidth * sizeof(uint32_t));
  }
  dev->unmap_texture_region(d->ssr_stats_stage, 0u);
  cl->barrier(d->ssr_stats_texture, CSRC, UA);

  const double pixels      = static_cast<double>(c[0][0]);
  const double traversed   = static_cast<double>(c[0][3]);
  const double candidates  = static_cast<double>(c[0][4]);
  const double raw_hits    = static_cast<double>(c[1][6]);
  const double accepted    = static_cast<double>(c[1][4]);
  auto pct_of_pixels = [&](double v) { return pixels > 0 ? 100.0 * v / pixels : 0.0; };
  auto pct_of_cand   = [&](double v) { return candidates > 0 ? 100.0 * v / candidates : 0.0; };

  std::ostringstream msg;
  msg << "[SSR] TraceStats: pixels=" << (uint64_t)pixels
      << " traversed=" << pct_of_pixels(traversed) << "%pix"
      << " candidates=" << pct_of_pixels(candidates) << "%pix"
      << " raw_hits=" << pct_of_pixels(raw_hits) << "%pix"
      << " validated=" << pct_of_pixels(accepted) << "%pix"
      << " self_thr=" << shader_injection.ssr_self_hit_threshold << "px"
      << " bypass=" << (shader_injection.ssr_bypass_validation > 0.5f ? "ON" : "off")
      << " thickGate=" << (shader_injection.ssr_thickness_gate > 0.5f ? "ON" : "OFF")
      << " thickT=" << shader_injection.ssr_thickness
      << " thickMode=" << (shader_injection.ssr_thickness_mode > 0.5f ? "PERP" : "EUCLID");
  reshade::log::message(reshade::log::level::info, msg.str().c_str());
  {
    std::ostringstream px;
    px << "[SSR]   accounting: logicalPixels=" << (uint64_t)pixels
       << " rayExecutions=" << c[12][1]
       << " candidateExecutions=" << c[12][2]
       << " acceptedExecutions=" << c[12][3]
       << " perpFallback=" << c[12][0]
       // Phase 3.Fix17: below-horizon regeneration funnel (row y8).
       << " | belowHorizonInitial=" << c[8][0]
       << " regenerated=" << c[8][1]
       << " regenRejected=" << c[8][2];
    reshade::log::message(reshade::log::level::info, px.str().c_str());
  }
  {
    std::ostringstream r;
    r << "[SSR]   reject (%cand):"
      << " self=" << pct_of_cand(c[0][7]) << "%"
      << " sky=" << pct_of_cand(c[1][0]) << "%"
      << " bf=" << pct_of_cand(c[1][1]) << "%"
      << " off(inbounds)=" << pct_of_cand(c[0][5]) << "%"
      << " finite_depth=" << pct_of_cand(c[0][6]) << "%"
      << " thickness=" << pct_of_cand(c[1][2]) << "%"
      << " vignette=" << pct_of_cand(c[1][3]) << "%";
    reshade::log::message(reshade::log::level::info, r.str().c_str());
  }
  {
    std::ostringstream t;
    t << "[SSR]   term (% of pixels):"
      << " candidate=" << pct_of_pixels(c[3][0]) << "%"
      << " budget=" << pct_of_pixels(c[3][1]) << "%"
      << " degenerate=" << pct_of_pixels(c[3][2]) << "%"
      << " sky_origin=" << pct_of_pixels(c[3][3]) << "%"
      << " | bypass_accepted=" << c[1][5];
    reshade::log::message(reshade::log::level::info, t.str().c_str());
  }
  {
    std::ostringstream i;
    double iters_avg = traversed > 0 ? static_cast<double>(c[2][0]) / traversed : 0.0;
    double fmip_avg  = traversed > 0 ? static_cast<double>(c[2][2]) / traversed : 0.0;
    double cmip_avg  = traversed > 0 ? static_cast<double>(c[2][3]) / traversed : 0.0;
    i << "[SSR]   iterations avg=" << iters_avg << " max=" << c[2][1]
      << " | mip final_avg=" << fmip_avg << " coarse_avg=" << cmip_avg
      << " (hits only)";
    reshade::log::message(reshade::log::level::info, i.str().c_str());
  }
  { // Phase 2.3 candidate-distance bins (% of candidates)
    std::ostringstream d;
    d << "[SSR]   candDist (%cand):"
      << " 0-1=" << pct_of_cand(c[4][0]) << "%"
      << " 1-2=" << pct_of_cand(c[4][1]) << "%"
      << " 2-8=" << pct_of_cand(c[4][2]) << "%"
      << " 8-32=" << pct_of_cand(c[4][3]) << "%"
      << " 32+=" << pct_of_cand(c[4][4]) << "%";
    reshade::log::message(reshade::log::level::info, d.str().c_str());
  }
  { // Phase 2.3 candidate z-delta signs (% of candidates)
    std::ostringstream z;
    z << "[SSR]   candZDelta (%cand):"
      << " behind(ray past surf)=" << pct_of_cand(c[4][5]) << "%"
      << " front=" << pct_of_cand(c[4][6]) << "%"
      << " eps(grazing)=" << pct_of_cand(c[4][7]) << "%";
    reshade::log::message(reshade::log::level::info, z.str().c_str());
  }
  { // Phase 2.3 backface dot-product distribution (% of candidates)
    std::ostringstream b;
    b << "[SSR]   bfDot (%cand):"
      << " <-0.75=" << pct_of_cand(c[5][0]) << "%"
      << " -0.75..-0.25=" << pct_of_cand(c[5][1]) << "%"
      << " -0.25..0=" << pct_of_cand(c[5][2]) << "%"
      << " 0..0.25=" << pct_of_cand(c[5][3]) << "%"
      << " 0.25..0.75=" << pct_of_cand(c[5][4]) << "%"
      << " >0.75=" << pct_of_cand(c[5][5]) << "%"
      << " | gate=" << (shader_injection.ssr_backface_gate > 0.5f ? "ON" : "OFF");
    reshade::log::message(reshade::log::level::info, b.str().c_str());
  }
  { // Phase 2.4 first depth-test displacement (% of traversed)
    auto pct_of_trav = [&](double v) { return traversed > 0 ? 100.0 * v / traversed : 0.0; };
    std::ostringstream f;
    f << "[SSR]   firstStep (%trav):"
      << " <0.5=" << pct_of_trav(c[6][0]) << "%"
      << " <1=" << pct_of_trav(c[6][1]) << "%"
      << " <1.5=" << pct_of_trav(c[6][2]) << "%"
      << " <2=" << pct_of_trav(c[6][3]) << "%"
      << " <4=" << pct_of_trav(c[6][4]) << "%"
      << " >=4=" << pct_of_trav(c[6][5]) << "%"
      << " | bias=" << shader_injection.ssr_initial_advance_bias << "px";
    reshade::log::message(reshade::log::level::info, f.str().c_str());
  }
  { // Phase 2.8 thickness-metric distribution + CDF at sweep stops (% of candidates)
    auto pc = [&](uint32_t v) {
      return candidates > 0 ? 100.0 * static_cast<double>(v) / candidates : 0.0;
    };
    const double b0 = pc(c[10][0]), b1 = pc(c[10][1]), b2 = pc(c[10][2]);
    const double b3 = pc(c[10][3]), b4 = pc(c[10][4]), b5 = pc(c[10][5]);
    const double b6 = pc(c[10][6]), b7 = pc(c[10][7]);
    std::ostringstream t;
    t << "[SSR]   thickDist (%cand):"
      << " <0.01=" << b0 << "%"
      << " .01-.025=" << b1 << "%"
      << " .025-.05=" << b2 << "%"
      << " .05-.1=" << b3 << "%"
      << " .1-.15=" << b4 << "%"
      << " .15-.25=" << b5 << "%"
      << " .25-.5=" << b6 << "%"
      << " >=.5=" << b7 << "%";
    reshade::log::message(reshade::log::level::info, t.str().c_str());
    {
      std::ostringstream cdf;
      cdf << "[SSR]   thickCDF accepted-% at T:"
          << " .01->" << b0
          << " | .025->" << (b0 + b1)
          << " | .05->" << (b0 + b1 + b2)
          << " | .1->" << (b0 + b1 + b2 + b3)
          << " | .15->" << (b0 + b1 + b2 + b3 + b4)
          << " | .25->" << (b0 + b1 + b2 + b3 + b4 + b5)
          << " | .5->" << (b0 + b1 + b2 + b3 + b4 + b5 + b6);
      reshade::log::message(reshade::log::level::info, cdf.str().c_str());
    }
    { // Phase 2.9 perpendicular-metric distribution (% of candidates, same grid)
      auto pp = [&](uint32_t v) {
        return candidates > 0 ? 100.0 * static_cast<double>(v) / candidates : 0.0;
      };
      std::ostringstream p;
      p << "[SSR]   thickDistP (%cand):"
        << " <0.01=" << pp(c[11][0]) << "%"
        << " .01-.025=" << pp(c[11][1]) << "%"
        << " .025-.05=" << pp(c[11][2]) << "%"
        << " .05-.1=" << pp(c[11][3]) << "%"
        << " .1-.15=" << pp(c[11][4]) << "%"
        << " .15-.25=" << pp(c[11][5]) << "%"
        << " .25-.5=" << pp(c[11][6]) << "%"
        << " >=.5=" << pp(c[11][7]) << "%";
      reshade::log::message(reshade::log::level::info, p.str().c_str());
      std::ostringstream fb;
      fb << "[SSR]   perpFallback=" << c[12][0]
         << " (" << pct_of_pixels(static_cast<double>(c[12][0])) << "%pix"
         << ", " << pc(c[12][0]) << "%cand)"
         << " — candidates using Euclidean because hit-texel depth normal was unavailable";
      reshade::log::message(reshade::log::level::info, fb.str().c_str());
    }
  }
  { // Phase 2.5 normal-family diagnostics (% of traversed) — cells y7..y9
    auto pt = [&](uint32_t v) {
      return traversed > 0 ? 100.0 * static_cast<double>(v) / traversed : 0.0;
    };
    std::ostringstream n;
    n << "[SSR]   normals (%trav):"
      << " diff>30deg=" << pt(c[6][6]) << "%"
      << " depthNA=" << pt(c[6][7]) << "%"
      << " | mrtLen: exact=" << pt(c[7][0]) << "%"
      << " near=" << pt(c[7][1]) << "%"
      << " off=" << pt(c[7][2]) << "%";
    reshade::log::message(reshade::log::level::info, n.str().c_str());
    {
      std::ostringstream d;
      d << "[SSR]   mrtVsDepthDot (%trav, mutually exclusive):"
        << " >=0.99=" << pt(c[7][3]) << "%"
        << " 0.95-0.99=" << pt(c[7][4]) << "%"
        << " 0.90-0.95=" << pt(c[7][5]) << "%"
        << " 0.80-0.90=" << pt(c[7][6]) << "%"
        << " <0.80=" << pt(c[7][7]) << "%";
      reshade::log::message(reshade::log::level::info, d.str().c_str());
    }
    {
      std::ostringstream r;
      r << "[SSR]   reflDelta (%trav): <10deg=" << pt(c[8][0]) << "%"
        << " 10-30=" << pt(c[8][1]) << "%"
        << " >=30=" << pt(c[8][2]) << "%"
        << " | agreeSubset(dot>=.95): <10=" << pt(c[9][0]) << "%"
        << " 10-30=" << pt(c[9][1]) << "%"
        << " >=30=" << pt(c[9][2]) << "%";
      reshade::log::message(reshade::log::level::info, r.str().c_str());
    }
    {
      std::ostringstream g;
      g << "[SSR]   vGrazing dot(V,N_depth) (%trav): <=-0.2=" << pt(c[8][3]) << "%"
        << " -0.2..0=" << pt(c[8][4]) << "%"
        << " 0..+0.2=" << pt(c[8][5]) << "%"
        << " >+0.2=" << pt(c[8][6]) << "%"
        << " (expect ~0% non-positive)";
      reshade::log::message(reshade::log::level::info, g.str().c_str());
    }
    { // Raw counts audit line — makes any accumulation anomaly instantly visible.
      std::ostringstream a;
      a << "[SSR]   audit counts: y7=[" << c[7][0] << "," << c[7][1] << ","
        << c[7][2] << "," << c[7][3] << "," << c[7][4] << "," << c[7][5] << ","
        << c[7][6] << "," << c[7][7] << "]"
        << " y8=[" << c[8][0] << "," << c[8][1] << "," << c[8][2] << ","
        << c[8][3] << "," << c[8][4] << "," << c[8][5] << "," << c[8][6] << "]"
        << " y9=[" << c[9][0] << "," << c[9][1] << "," << c[9][2] << ","
        << c[9][3] << "," << c[9][4] << "," << c[9][5] << "]";
      reshade::log::message(reshade::log::level::info, a.str().c_str());
    }
    { // Phase 3.Fix9: GeoProbe — decode bit-cast floats from y13/y14.
      auto dec = [&](uint32_t cell) -> float {
        float f; memcpy(&f, &cell, 4); return f;
      };
      std::ostringstream g;
      g << "[SSR] GeoProbe:"
        << " P_view=(" << dec(c[13][0]) << "," << dec(c[13][1]) << "," << dec(c[13][2]) << ")"
        << " V_view=(" << dec(c[13][3]) << "," << dec(c[13][4]) << "," << dec(c[13][5]) << ")"
        << " hw_depth=" << dec(c[13][6])
        << " dot(V,N)=" << dec(c[13][7]);
      reshade::log::message(reshade::log::level::info, g.str().c_str());
      std::ostringstream g2;
      g2 << "[SSR]   R_minus=(" << dec(c[14][0]) << "," << dec(c[14][1]) << "," << dec(c[14][2]) << ")"
         << " UV_end=(" << dec(c[14][3]) << "," << dec(c[14][4]) << ")"
         << " dot(R_m,N)=" << dec(c[14][5])
         << " dot(R_p,N)=" << dec(c[14][6])
         << " dot(R_plus,N)=" << dec(c[14][7]);
      reshade::log::message(reshade::log::level::info, g2.str().c_str());
      // Phase 3.Fix12: dirProbe (y15) — old endpoint-projection |duv| sweep
      // vs new AMD unit-step constant. unitStep must not vary with the
      // Max Ray Distance slider.
      std::ostringstream g3;
      g3 << "[SSR]   nearPlaneDist=" << dec(c[15][0])
         << " dirProbe: oldL1=" << dec(c[15][1])
         << " oldL10=" << dec(c[15][2])
         << " oldL100=" << dec(c[15][3])
         << " oldL300=" << dec(c[15][4])
         << " unitStep=" << dec(c[15][5]);
      reshade::log::message(reshade::log::level::info, g3.str().c_str());
      // Phase 3.Fix13: vndfProbe (y16) — numeric VNDF direction probe.
      // dotLM=1/angDeg=0 baseline in Mirror mode; sentinels -2/-1 = no ray.
      // Phase 3.Fix17: flip column renamed "rejected" — 1 = both samples
      // below horizon, ray never marched. Per-attempt rows in y20/y21.
      std::ostringstream g4;
      g4 << "[SSR]   vndfProbe: rough=" << dec(c[16][0])
         << " alpha=" << dec(c[16][1])
         << " dotLM=" << dec(c[16][2])
         << " angDeg=" << dec(c[16][3])
         << " ndotl=" << dec(c[16][4])
         << " rejected=" << dec(c[16][5])
         << " bias=" << dec(c[16][6])
         << " rays=" << dec(c[16][7]);
      reshade::log::message(reshade::log::level::info, g4.str().c_str());
      {
        std::ostringstream g5;
        g5 << "[SSR]   vndfProbe att0: dotLM=" << dec(c[20][0])
           << " angDeg=" << dec(c[20][1])
           << " ndotl=" << dec(c[20][2])
           << " belowHorizon=" << dec(c[20][3]);
        reshade::log::message(reshade::log::level::info, g5.str().c_str());
        if (dec(c[20][4]) > 0.5f) {
          std::ostringstream g6;
          g6 << "[SSR]   vndfProbe att1: dotLM=" << dec(c[21][0])
             << " angDeg=" << dec(c[21][1])
             << " ndotl=" << dec(c[21][2])
             << " belowHorizon=" << dec(c[21][3]);
          reshade::log::message(reshade::log::level::info, g6.str().c_str());
        }
      }
      // Phase 3.Fix15: Mirror-vs-VNDF CompareProbe (y17-y19). resolveA is
      // logged as -1: the Phase 4 resolve pass has no dispatch call site.
      std::ostringstream p1;
      p1 << "[SSR] CompareProbe M: uv=(" << dec(c[17][0]) << "," << dec(c[17][1]) << ")"
         << " rad=(" << dec(c[17][2]) << "," << dec(c[17][3]) << "," << dec(c[17][4]) << ")"
         << " a=" << dec(c[17][5]) << " hit=" << dec(c[17][6]);
      reshade::log::message(reshade::log::level::info, p1.str().c_str());
      std::ostringstream p2;
      p2 << "[SSR] CompareProbe V: uv=(" << dec(c[18][0]) << "," << dec(c[18][1]) << ")"
         << " rad=(" << dec(c[18][2]) << "," << dec(c[18][3]) << "," << dec(c[18][4]) << ")"
         << " a=" << dec(c[18][5])
         << " angDeg=" << dec(c[18][6]) << " hit=" << dec(c[18][7]);
      reshade::log::message(reshade::log::level::info, p2.str().c_str());
      std::ostringstream p3;
      p3 << "[SSR] CompareProbe blend: rawConf=" << dec(c[19][0])
         << " t25a=" << dec(c[19][1])
         << " resolveA=" << dec(c[19][2]);
      reshade::log::message(reshade::log::level::info, p3.str().c_str());
      // Phase 3.Fix18: radiance footprint probe at the V hit UV.
      std::ostringstream p4;
      p4 << "[SSR] footProbe: c=(" << dec(c[22][0]) << "," << dec(c[22][1]) << "," << dec(c[22][2]) << ")"
         << " avg3=(" << dec(c[22][3]) << "," << dec(c[22][4]) << "," << dec(c[22][5]) << ")"
         << " valid=" << dec(c[22][6]);
      reshade::log::message(reshade::log::level::info, p4.str().c_str());
      std::ostringstream p5;
      p5 << "[SSR] footProbe: avg5=(" << dec(c[23][0]) << "," << dec(c[23][1]) << "," << dec(c[23][2]) << ")"
         << " max5=(" << dec(c[23][3]) << "," << dec(c[23][4]) << "," << dec(c[23][5]) << ")"
         << " maxLuma=" << dec(c[23][6]);
      reshade::log::message(reshade::log::level::info, p5.str().c_str());
      // Phase R1 Probe B: hit classification at the probe pixel (row y27).
      std::ostringstream pc;
      pc << "[SSR] CompareProbe Class: VsameSurf=" << dec(c[27][0])
         << " VrejReason=" << dec(c[27][1])
         << " MsameSurf=" << dec(c[27][2])
         << " VndotlFinal=" << dec(c[27][3])
         << " MrejReason=" << dec(c[27][4]);
      reshade::log::message(reshade::log::level::info, pc.str().c_str());
      // Phase R1: RvS comparison now logs directly from RunSSRResolve
      // (Option B) — the stats readback here would predate the resolve.
    }
  }
}

// Phase 2: deterministic mirror trace — reflect(-V, N), SSSR hierarchical
// march, ValidateHit confidence. Writes rgb = debug payload (codes 10..17)
// and a = hit confidence into ssr_ray_result. No radiance sampling yet.
// Phase 3.Fix16: CompareProbe auto-selection. Reads back the raw stochastic
// output (RGBA16F, alpha = confidence) and freezes the brightest pixel whose
// mirror-mode ray hit with conf >= 0.8 and max(RGB) >= 0.25. Runs ONLY while
// Stochastic is OFF and Debug View is off (production frames), throttled to
// every 10th frame; the selection then stays frozen while the user switches
// Stochastic ON — giving a perfectly controlled same-pixel comparison.
// Relaxed fallback: if no pixel passes the brightness floor, picks the
// brightest conf>=0.8 pixel so the probe always has a target.
static float SSR_ProbeHalfToFloat(uint16_t h) {
  const uint32_t sign = (uint32_t)(h & 0x8000u) << 16;
  const uint32_t exp16 = (uint32_t)(h >> 10) & 0x1Fu;
  const uint32_t man16 = (uint32_t)h & 0x03FFu;
  uint32_t bits;
  if (exp16 == 0u) {
    if (man16 == 0u) {
      bits = sign;
    } else {
      uint32_t e = 127 - 15 + 1;
      uint32_t m = man16;
      while ((m & 0x400u) == 0u) { m <<= 1; --e; }
      bits = sign | (e << 23) | ((m & 0x3FFu) << 13);
    }
  } else if (exp16 == 31u) {
    bits = sign | 0x7F800000u | (man16 << 13);
  } else {
    bits = sign | ((exp16 - 15u + 127u) << 23) | (man16 << 13);
  }
  float f;
  memcpy(&f, &bits, 4);
  return f;
}

static void SSR_AutoSelectProbePixel(reshade::api::command_list* cl,
                                     reshade::api::device* dev, DeviceData* d) {
  if (!cl || !dev || !d) return;
  if (shader_injection.ssr_probe_auto < 0.5f) return;      // manual X/Y mode
  if (shader_injection.ssr_stochastic > 0.5f) return;       // freeze while stochastic
  if (shader_injection.ssr_debug_view > 0.5f) return;       // production frames only
  if ((d->frame_index % 10u) != 0u) return;                 // throttle
  if (!d->ssr_ray_result_texture.handle || !d->ssr_probe_stage.handle) return;

  const auto SR = reshade::api::resource_usage::shader_resource;
  const auto CSRC = reshade::api::resource_usage::copy_source;
  const auto CD = reshade::api::resource_usage::copy_dest;
  cl->barrier(d->ssr_ray_result_texture, SR, CSRC);
  cl->barrier(d->ssr_probe_stage, CD, CD);
  cl->copy_texture_region(d->ssr_ray_result_texture, 0u, nullptr,
                          d->ssr_probe_stage, 0u, nullptr);
  reshade::api::subresource_data sd = {};
  bool mapped = dev->map_texture_region(d->ssr_probe_stage, 0u, nullptr,
                                        reshade::api::map_access::read_only, &sd)
                && sd.data != nullptr;
  float best_bright = -1.0f, best_conf_bright = -1.0f;
  float sel_a = 0.f, sel_r = 0.f, sel_g = 0.f, sel_b = 0.f;
  float fb_a = 0.f;
  uint32_t sel_x = 0, sel_y = 0, fb_x = 0, fb_y = 0;
  if (mapped) {
    const uint32_t w = d->last_created_ssr_width;
    const uint32_t hgt = d->last_created_ssr_height;
    constexpr float kMinConf = 0.8f;
    constexpr float kMinBright = 0.25f;
    for (uint32_t y = 0; y < hgt; ++y) {
      const uint16_t* row = reinterpret_cast<const uint16_t*>(
          static_cast<const uint8_t*>(sd.data) + static_cast<size_t>(y) * sd.row_pitch);
      for (uint32_t x = 0; x < w; ++x) {
        const uint16_t* p = row + x * 4u;
        const float a = SSR_ProbeHalfToFloat(p[3]);
        if (a < kMinConf) continue;
        const float r = SSR_ProbeHalfToFloat(p[0]);
        const float g = SSR_ProbeHalfToFloat(p[1]);
        const float b = SSR_ProbeHalfToFloat(p[2]);
        const float bright = std::max(r, std::max(g, b));
        if (bright >= kMinBright && bright > best_bright) {
          best_bright = bright;
          sel_x = x; sel_y = y; sel_a = a; sel_r = r; sel_g = g; sel_b = b;
        }
        if (bright > best_conf_bright) {
          best_conf_bright = bright;
          fb_x = x; fb_y = y; fb_a = a;
        }
      }
    }
    dev->unmap_texture_region(d->ssr_probe_stage, 0u);
  }
  cl->barrier(d->ssr_ray_result_texture, CSRC, SR);

  bool have = false;
  bool relaxed = false;
  float lx = 0.f, ly = 0.f;
  if (best_bright >= 0.0f) {
    have = true; lx = (float)sel_x; ly = (float)sel_y;
  } else if (best_conf_bright >= 0.0f) {
    have = true; relaxed = true;
    lx = (float)fb_x; ly = (float)fb_y; sel_a = fb_a;
    sel_r = sel_g = sel_b = best_conf_bright;
  }
  if (!have) return;
  const bool changed = !d->ssr_probe_has_selection ||
      fabsf(d->ssr_probe_frozen_x - lx) > 0.5f ||
      fabsf(d->ssr_probe_frozen_y - ly) > 0.5f;
  d->ssr_probe_frozen_x = lx;
  d->ssr_probe_frozen_y = ly;
  d->ssr_probe_has_selection = true;
  if (changed) {
    std::ostringstream s;
    s << "[SSR] CompareProbe selected pixel=(" << (int)lx << "," << (int)ly << ")"
      << " mirrorRad=(" << sel_r << "," << sel_g << "," << sel_b << ")"
      << " conf=" << sel_a
      << (relaxed ? " (relaxed: no pixel passed brightness floor)" : "");
    reshade::log::message(reshade::log::level::info, s.str().c_str());
  }
}


static bool RunSSRTrace(reshade::api::command_list* cl, DeviceData* d) {
  if (!cl || !d) return false;
  if (!d->captured_depth_srv.handle || !d->captured_scene_cbv_valid) return false;
  auto* dev = cl->get_device();
  if (!dev) return false;
  if (!CreateSSRPipelinesIfNeeded(dev, d)) return false;
  InitMotionVectorCapture();
  if (!d->ssr_resources_created || !d->ssr_ray_result_uav.handle
      || !d->ssr_hiz_srv.handle || !d->ssr_trace_pipeline.handle) return false;

  const int debug_slider = (int)(shader_injection.ssr_debug_view + 0.5f);
  // Trace Stats decoupled from Debug View — gated on ssr_log_tracestats toggle.
  {
    static bool s_prev_tracestats = false;
    const bool tracestats_on = shader_injection.ssr_log_tracestats > 0.5f;
    if (!tracestats_on && s_prev_tracestats) d->ssr_stats_done = false;
    s_prev_tracestats = tracestats_on;
  }

  // Explicit configuration echo — on Trace Stats trigger or setting change.
  {
    static uint32_t s_cfg_hash = 0u;
    const uint32_t h = (uint32_t)shader_injection.ssr_stochastic
        + (uint32_t)shader_injection.ssr_ray_count * 7u
        + (uint32_t)shader_injection.ssr_forced_ray_mode * 11u
        + (uint32_t)shader_injection.ssr_thickness_mode * 13u
        + (uint32_t)shader_injection.ssr_diagnostics * 17u
        + (uint32_t)shader_injection.ssr_radiance_source * 19u
        + (uint32_t)(shader_injection.ssr_thickness * 1000.0f);
    if (h != s_cfg_hash && shader_injection.ssr_log_config > 0.5f) {
      s_cfg_hash = h;
      std::ostringstream cfg;
      cfg << "[SSR] Phase3Config: stochastic="
          << (int)shader_injection.ssr_stochastic
          << " rayCount=" << (int)shader_injection.ssr_ray_count
          << " forcedRayMode=" << (int)shader_injection.ssr_forced_ray_mode
          << " diag=" << (int)shader_injection.ssr_diagnostics
          << " radSrc=" << (int)shader_injection.ssr_radiance_source
          << " thickMode=" << (shader_injection.ssr_thickness_mode > 0.5f ? "PERP" : "EUCLID")
          << " thickT=" << shader_injection.ssr_thickness;
      reshade::log::message(reshade::log::level::info, cfg.str().c_str());
    }
  }

  const uint32_t w = d->working_width, h = d->working_height;
  if (w < 64u || h < 64u) return false;

  const auto CS = reshade::api::shader_stage::all_compute;
  const auto AC = reshade::api::pipeline_stage::all_compute;
  const auto UA = reshade::api::resource_usage::unordered_access;
  const auto SR = reshade::api::resource_usage::shader_resource;

  cl->bind_pipeline(AC, d->ssr_trace_pipeline);

  // Reset funnel counters for this frame's accumulation.
  const uint32_t zero[4] = {0u, 0u, 0u, 0u};
  if (d->ssr_stats_uav.handle)
    cl->clear_unordered_access_view_uint(d->ssr_stats_uav, zero);

  // IS-FAST volume is required by the stochastic path regardless of the
  // global IS-FAST master toggle.
  if (!d->isfast_noise_srv.handle && shader_injection.ssr_stochastic > 0.5f)
    LoadISFASTNoiseTexture(dev, d);

  // Radiance source selection at bind time:
  //   0 = colorTexture t0 (current frame), 1 = BackBuffer copy (lazy-created
  //   in OnPresent when selected).
  reshade::api::resource_view rad_srv = d->captured_color_srv.handle
      ? d->captured_color_srv : d->fallback_srv;
  if (shader_injection.ssr_radiance_source > 0.5f && d->ssr_radiance_copy_srv.handle)
    rad_srv = d->ssr_radiance_copy_srv;

  const reshade::api::resource_view fallback = d->fallback_srv;
  reshade::api::resource_view srvs[7] = {
      d->ssr_hiz_srv,
      d->captured_depth_srv,
      d->captured_mrt_normal_srv.handle ? d->captured_mrt_normal_srv : fallback,
      d->captured_mrt_spec_srv.handle ? d->captured_mrt_spec_srv : fallback,
      d->captured_mrt_material_srv.handle ? d->captured_mrt_material_srv : fallback,
      rad_srv,
      d->isfast_noise_srv.handle ? d->isfast_noise_srv : fallback};
  reshade::api::resource_view uavs[3] = {d->ssr_ray_result_uav, d->ssr_ray_meta_uav, d->ssr_stats_uav};
  reshade::api::sampler smp[2] = {d->point_clamp_sampler, d->ssr_linear_sampler};

  // Six updates: sampler span, CBV, texture SRV span t0..t6, buffer SRV t7,
  // UAV span. The trace layout's generic shader_resource_view range accepts
  // both texture and buffer SRVs.
  std::array<reshade::api::descriptor_table_update, 5> uu = {};
  uu[0] = {{},0,0,2,reshade::api::descriptor_type::sampler,smp};
  uu[1] = {{},0,0,1,reshade::api::descriptor_type::constant_buffer,&d->captured_scene_cbv_view};
  uu[2] = {{},0,0,7,reshade::api::descriptor_type::texture_shader_resource_view,srvs};
  uu[3] = {{},7,0,1,reshade::api::descriptor_type::buffer_shader_resource_view,
           &d->captured_deferred_params_srv};
  uu[4] = {{},0,0,3,reshade::api::descriptor_type::texture_unordered_access_view,uavs};
  // Param mapping: 0=sampler, 1=cbv, 2=SRV table (texture span AND the t7
  // buffer entry both target param 2), 3=UAV (R2C: rays+meta+stats).
  const uint32_t param_of[5] = {0u, 1u, 2u, 2u, 3u};
  uint32_t update_count = 5u;
  for (uint32_t i = 0; i < 5; ++i) {
    if ((i == 3) && !d->captured_deferred_params_srv.handle) {
      update_count = 4u;                       // drop buffer entry if uncaptured
      continue;
    }
    uu[i].table = d->ssr_trace_tables[param_of[i]];
  }
  dev->update_descriptor_tables(update_count, uu.data());
  std::array<reshade::api::descriptor_table, kGtvbaoDescriptorTableParamCount> bb = {};
  for (uint32_t i = 0; i < 4; ++i) bb[i] = d->ssr_trace_tables[i];
  cl->bind_descriptor_tables(CS, d->ssr_trace_layout, 0, 4, bb.data());

  auto pc = BuildSSRPushConstants(d, (float)w, (float)h);
  cl->push_constants(CS, d->ssr_trace_layout, kGtvbaoPushConstantsLayoutParam,
                     0, kSSRPushConstantCount, pc.data());
  cl->dispatch((w + 7) / 8, (h + 7) / 8, 1);
  cl->barrier(d->ssr_ray_result_texture, UA, SR);

  // Phase 3.Fix16: auto-select the CompareProbe pixel from this mirror frame.
  SSR_AutoSelectProbePixel(cl, dev, d);

  // One-shot aggregate report when the user selects 'Trace Stats'.
  if (shader_injection.ssr_log_tracestats > 0.5f && !d->ssr_stats_done && d->ssr_stats_stage.handle) {
    d->ssr_stats_done = true;
    SSR_LogTraceStats(cl, d);
  }
  return true;
}

// ── Phase 3.Fix19: spatial resolve dispatch (was created but never called) ──
// Chain: ssr_trace → ssr_ray_result → ssr_resolve → ssr_output → t25 → PS.
// Runs on production frames (Debug View off) and on the Raw-vs-Resolved
// split view (code 42); skipped for all other debug views because those
// overwrite ray_result with visualization payloads.
static bool RunSSRResolve(reshade::api::command_list* cl, DeviceData* d) {
  if (!cl || !d) return false;
  d->ssr_resolved_this_frame = false;
  // Phase R1: production dispatch only when the Spatial Resolve toggle is ON.
  // Resetting the logger latch here means off→on toggles re-log.
  static bool s_prev_pass = false;
  if (shader_injection.ssr_resolve_enable < 0.5f) { s_prev_pass = false; return false; }
  auto* dev = cl->get_device();
  if (!dev) return false;

  const float dbg_translated = SSR_TranslateDebugView(shader_injection.ssr_debug_view);
  const bool rvr_view = SSR_IsRawVsResolvedView(dbg_translated);
  const bool mv_view = SSR_IsMotionVectorView(dbg_translated);
  if (shader_injection.ssr_debug_view > 0.5f && !rvr_view && !mv_view) return false;
  if (!d->ssr_output_uav.handle || !d->ssr_output_srv.handle
      || !d->ssr_resolve_pipeline.handle
      || !d->ssr_ray_result_srv.handle || !d->ssr_hiz_srv.handle) return false;

  const uint32_t w = d->last_created_ssr_width;
  const uint32_t h = d->last_created_ssr_height;

  const auto SR = reshade::api::resource_usage::shader_resource;
  const auto UA = reshade::api::resource_usage::unordered_access;
  const auto AC = reshade::api::pipeline_stage::all_compute; // bind_pipeline
  const auto CS = reshade::api::shader_stage::all_compute;   // descriptors/push
  // ── Phase R1 Stage 0b FIX: bind the resolve pipeline. Without this the
  // dispatch executed whatever compute pipeline was still bound (trace's),
  // so the stub never ran and ssr_output stayed creation-zeros.
  cl->bind_pipeline(AC, d->ssr_resolve_pipeline);
  cl->barrier(d->ssr_output_texture, SR, UA);

  // -- Phase R2C resolve bindings ------------------------------------------
  // Table param 2 slot map (binding == HLSL t-register):
  //   0=ray_result 1=hiz 2=mrt_normal 3=mrt_matidx
  //   4=deferredParams(StructuredBuffer) 5=ray_meta 6=mrt_spec(F0)
  // The StructuredBuffer keeps its own buffer_shader_resource_view entry
  // (mixing types into one span invalidates the whole table update).
  // -- Phase R3 resolve bindings ------------------------------------------
  // Layout uses generic shader_resource_view type (same as trace layout),
  // so all 9 SRVs (textures + StructuredBuffer) go into ONE span update.
  // Table param 2 slot map (binding == HLSL t-register):
  //   0=ray_result 1=hiz 2=mrt_normal 3=mrt_matidx 4=deferredParams(buf)
  //   5=ray_meta 6=mrt_spec(F0) 7=radPyr(pyramid) 8=motionVectors(DLSS)
  reshade::api::resource_view srvs_all[9] = {
      d->ssr_ray_result_srv,
      d->ssr_hiz_srv,
      d->captured_mrt_normal_srv.handle ? d->captured_mrt_normal_srv : d->fallback_srv,
      d->captured_mrt_material_srv.handle ? d->captured_mrt_material_srv : d->fallback_srv,
      d->captured_deferred_params_srv,
      d->ssr_ray_meta_srv.handle ? d->ssr_ray_meta_srv : d->fallback_srv,
      d->captured_mrt_spec_srv.handle ? d->captured_mrt_spec_srv : d->fallback_srv,
      d->ssr_rad_pyr_srv.handle ? d->ssr_rad_pyr_srv : d->fallback_srv,
      d->captured_motion_srv.handle ? d->captured_motion_srv : d->fallback_srv};
  reshade::api::resource_view uavs[2] = {d->ssr_output_uav, d->ssr_stats_uav};

  reshade::api::sampler smps[2] = {d->point_clamp_sampler, d->ssr_linear_sampler};
  // Single generic SRV span covering all 9 slots (t0-t8) — same approach as
  // trace layout which mixes Texture2D + StructuredBuffer in one range.
  std::array<reshade::api::descriptor_table_update, 4> uu = {};
  uu[0] = {{},0,0,2,reshade::api::descriptor_type::sampler,smps};
  uu[1] = {{},0,0,1,reshade::api::descriptor_type::constant_buffer,&d->captured_scene_cbv_view};
  uu[2] = {{},0,0,9,reshade::api::descriptor_type::shader_resource_view,srvs_all};
  uu[3] = {{},0,0,2,reshade::api::descriptor_type::texture_unordered_access_view,uavs};
  const uint32_t param_of[4] = {0u, 1u, 2u, 3u};
  for (uint32_t i = 0; i < 4; ++i) uu[i].table = d->ssr_resolve_tables[param_of[i]];
  dev->update_descriptor_tables(4, uu.data());
  std::array<reshade::api::descriptor_table, kGtvbaoDescriptorTableParamCount> bb = {};
  for (uint32_t i = 0; i < 4; ++i) bb[i] = d->ssr_resolve_tables[i];
  cl->bind_descriptor_tables(CS, d->ssr_resolve_layout, 0, 4, bb.data());

  auto pc = BuildSSRPushConstants(d, (float)w, (float)h);
  cl->push_constants(CS, d->ssr_resolve_layout, kGtvbaoPushConstantsLayoutParam,
                     0, kSSRPushConstantCount, pc.data());
  cl->dispatch((w + 7) / 8, (h + 7) / 8, 1);
  cl->barrier(d->ssr_output_texture, UA, SR);
  d->ssr_resolved_this_frame = true;

  // ── Probe readback. Gated on ssr_log_resolve + 30-frame throttle. ──
  if (shader_injection.ssr_log_resolve > 0.5f &&
      d->ssr_stats_texture.handle && d->ssr_stats_stage.handle &&
      d->frame_index % 30u == 0u) {
    const auto CSRC = reshade::api::resource_usage::copy_source;
    const auto CD = reshade::api::resource_usage::copy_dest;
    cl->barrier(d->ssr_stats_texture, UA, CSRC);
    cl->barrier(d->ssr_stats_stage, CD, CD);
    cl->copy_texture_region(d->ssr_stats_texture, 0u, nullptr,
                            d->ssr_stats_stage, 0u, nullptr);
    reshade::api::subresource_data sd = {};
    if (dev->map_texture_region(d->ssr_stats_stage, 0u, nullptr,
                                reshade::api::map_access::read_only, &sd) && sd.data) {
      const uint32_t* row = reinterpret_cast<const uint32_t*>(
          static_cast<const uint8_t*>(sd.data) + static_cast<size_t>(25) * sd.row_pitch);
      auto dec = [&](uint32_t cell) -> float { float f; memcpy(&f, &cell, 4); return f; };
      if (dec(row[6]) > 0.0f) {   // rawA > 0 — probe pixel had a valid center ray
        std::ostringstream r;
        r << "[SSR] CompareProbe RvS(post): raw=(" << dec(row[0]) << "," << dec(row[1]) << "," << dec(row[2]) << ")"
          << " res=(" << dec(row[3]) << "," << dec(row[4]) << "," << dec(row[5]) << ")"
          << " rawA=" << dec(row[6]) << " resA=" << dec(row[7]);
        reshade::log::message(reshade::log::level::info, r.str().c_str());
      }
      // Probe A: tap classification at the probe pixel (row y26).
      const uint32_t* row26 = reinterpret_cast<const uint32_t*>(
          static_cast<const uint8_t*>(sd.data) + static_cast<size_t>(26) * sd.row_pitch);
      std::ostringstream rp;
      rp << "[SSR] ResolveProbe: validNbr=" << row26[0]
         << " alphaRej=" << row26[1]
         << " depthRej=" << row26[2]
         << " normalRej=" << row26[3]
         << " roughRej=" << row26[4]
         << " centerW=" << dec(row26[5])
         << " nbrW=" << dec(row26[6])
         << " totalW=" << dec(row26[7]);
      reshade::log::message(reshade::log::level::info, rp.str().c_str());
      // Phase R2C estimator diagnosis (row y29).
      const uint32_t* row29 = reinterpret_cast<const uint32_t*>(
          static_cast<const uint8_t*>(sd.data) + static_cast<size_t>(29) * sd.row_pitch);
      std::ostringstream e;
      e << "[SSR] EstimatorProbe: nbrW_est=" << dec(row29[0])
        << " totalW_est=" << dec(row29[1])
        << " pdfAvg=" << dec(row29[2])
        << " clampHits=" << row29[3]
        << " coverage=" << dec(row29[4])
        << " resA=" << dec(row29[5]);
      reshade::log::message(reshade::log::level::info, e.str().c_str());
      dev->unmap_texture_region(d->ssr_stats_stage, 0u);
    }
    cl->barrier(d->ssr_stats_texture, CSRC, UA);
  }

  // Status log: re-emits whenever resolve resumes after a gap (toggle
  // off/on, view switches) or on resolution change.
  const bool first_after_gap = !s_prev_pass;
  s_prev_pass = true;
  static uint32_t s_log_w = 0, s_log_h = 0;
  if (first_after_gap || s_log_w != w || s_log_h != h) {
    s_log_w = w; s_log_h = h;
    std::ostringstream s;
    s << "[SSR] Resolve: dispatched=1 input=" << w << "x" << h
      << " output=" << w << "x" << h
      << (rvr_view ? " (raw-vs-resolved split view)" : "");
    reshade::log::message(reshade::log::level::info, s.str().c_str());
  }
  return true;
}
// -- Phase R3: filtered radiance pyramid build ------------------------------
// mip0 = captured HDR scene color (requires radSrc == ColorTexture);
// mips 1..7 = sequential 2x2 box-average reduction, Hi-Z machinery cloned.
// Gated OFF entirely when radSrc != 0: the pyramid would then hold stale
// content while the user tests the BackBuffer source. Logged on transition.
static bool RunSSRRadiancePyramid(reshade::api::command_list* cl, DeviceData* d) {
  if (!cl || !d) return false;
  const bool want = shader_injection.ssr_radiance_source < 0.5f;
  static bool s_prev_want = false;
  if (!want) {
    if (s_prev_want && shader_injection.ssr_log_init > 0.5f) {
      reshade::log::message(reshade::log::level::info,
        "[SSR] RadiancePyramid: disabled (filtered radiance requires radSrc=ColorTexture t0).");
    }
    s_prev_want = false;
    d->ssr_rad_pyr_valid = false;
    return false;
  }
  if (!d->ssr_rad_pipelines_ok || !d->ssr_rad_created
      || !d->ssr_rad_pyr_srv.handle || !d->ssr_rad_base_pipeline.handle)
    return false;
  auto* dev = cl->get_device();
  if (!dev || !d->captured_color_srv.handle) return false;

  const uint32_t w = d->last_created_ssr_width, h = d->last_created_ssr_height;
  const auto CS = reshade::api::shader_stage::all_compute;
  const auto AC = reshade::api::pipeline_stage::all_compute;
  const auto UA = reshade::api::resource_usage::unordered_access;
  const auto SR = reshade::api::resource_usage::shader_resource;
  const auto CD = reshade::api::resource_usage::copy_dest;
  const auto CSRC = reshade::api::resource_usage::copy_source;

  auto apply_descriptors = [&](uint32_t count,
                               const reshade::api::descriptor_table_update* updates) {
    std::array<reshade::api::descriptor_table_update, kGtvbaoDescriptorTableParamCount> u = {};
    for (uint32_t i = 0; i < count; ++i) { u[i] = updates[i]; u[i].table = d->ssr_common_tables[i]; }
    dev->update_descriptor_tables(count, u.data());
    std::array<reshade::api::descriptor_table, kGtvbaoDescriptorTableParamCount> b = {};
    for (uint32_t i = 0; i < count; ++i) b[i] = d->ssr_common_tables[i];
    cl->bind_descriptor_tables(CS, d->ssr_common_layout, 0, count, b.data());
  };

  // Pass R3a: base level (colorTexture -> pyramid mip 0 + scratch pre-seed).
  cl->barrier(d->ssr_rad_pyr_texture, SR, UA);
  cl->bind_pipeline(AC, d->ssr_rad_base_pipeline);
  {
    reshade::api::descriptor_table_update u[4] = {
      {{},0,0,1,reshade::api::descriptor_type::sampler,&d->point_clamp_sampler},
      {{},0,0,1,reshade::api::descriptor_type::constant_buffer,&d->captured_scene_cbv_view},
      {{},0,0,1,reshade::api::descriptor_type::texture_shader_resource_view,&d->captured_color_srv},
      {{},0,0,1,reshade::api::descriptor_type::texture_unordered_access_view,&d->ssr_rad_pyr_uavs[0]},
    };




    apply_descriptors(4, u);
  }
  auto pc = BuildSSRPushConstants(d, (float)w, (float)h);
  cl->push_constants(CS, d->ssr_common_layout, kGtvbaoPushConstantsLayoutParam,
                     0, kSSRPushConstantCount, pc.data());
  cl->dispatch((w + 7) / 8, (h + 7) / 8, 1);

  // Pass R3b: per-level box reduction (scratch ping-pong, Hi-Z pattern).
  cl->bind_pipeline(AC, d->ssr_rad_reduce_pipeline);
  for (uint32_t level = 1; level < SSR_RAD_MIP_LEVELS; ++level) {
    const uint32_t pw = (w >> (level - 1)) > 0 ? (w >> (level - 1)) : 1u;
    const uint32_t ph = (h >> (level - 1)) > 0 ? (h >> (level - 1)) : 1u;
    const uint32_t dw = (w >> level) > 0 ? (w >> level) : 1u;
    const uint32_t dh = (h >> level) > 0 ? (h >> level) : 1u;

    cl->barrier(d->ssr_rad_pyr_texture, UA, CSRC);
    cl->barrier(d->ssr_rad_scratch_texture, SR, CD);
    const reshade::api::subresource_box src_box = {0u, 0u, 0u, pw, ph, 1u};
    const reshade::api::subresource_box dst_box = {0u, 0u, 0u, pw, ph, 1u};
    cl->copy_texture_region(d->ssr_rad_pyr_texture, level - 1u, &src_box,
                            d->ssr_rad_scratch_texture, 0u, &dst_box);
    cl->barrier(d->ssr_rad_scratch_texture, CD, SR);

    reshade::api::descriptor_table_update u[4] = {
      {{},0,0,1,reshade::api::descriptor_type::sampler,&d->point_clamp_sampler},
      {{},0,0,1,reshade::api::descriptor_type::constant_buffer,&d->captured_scene_cbv_view},
      {{},0,0,1,reshade::api::descriptor_type::texture_shader_resource_view,&d->ssr_rad_scratch_srv},
      {{},0,0,1,reshade::api::descriptor_type::texture_unordered_access_view,&d->ssr_rad_pyr_uavs[level - 1]},
    };
    apply_descriptors(4, u);
    auto pcr = BuildSSRPushConstants(d, (float)pw, (float)ph);
    cl->push_constants(CS, d->ssr_common_layout, kGtvbaoPushConstantsLayoutParam,
                       0, kSSRPushConstantCount, pcr.data());
    cl->dispatch((dw + 7) / 8, (dh + 7) / 8, 1);
  }
  cl->barrier(d->ssr_rad_pyr_texture, UA, SR);
  d->ssr_rad_pyr_valid = true;
  s_prev_want = true;
  return true;
}

// -- Phase R3-MV: DLSS motion vector capture ---------------------------------
// State declared at file scope (line ~677). This is the implementation.

static void OnNGXEvaluateFeature(ID3D11DeviceContext* ctx, const NVSDK_NGX_Parameter* params) {
  if (!ctx || !params) return;

  void* mv_resource = nullptr;
  double mv_sx = 0.0, mv_sy = 0.0;

  const_cast<NVSDK_NGX_Parameter*>(params)->Get(
      NVSDK_NGX_Parameter_MotionVectors, &mv_resource);
  const_cast<NVSDK_NGX_Parameter*>(params)->Get(
      NVSDK_NGX_Parameter_MV_Scale_X, &mv_sx);
  const_cast<NVSDK_NGX_Parameter*>(params)->Get(
      NVSDK_NGX_Parameter_MV_Scale_Y, &mv_sy);

  if (mv_resource == nullptr) return;

  g_mv_scale_x = static_cast<float>(mv_sx);
  g_mv_scale_y = static_cast<float>(mv_sy);

  // Create SRV once per unique resource (DLSS reuses the same texture).
  if (!g_mv_srv) {
    ID3D11Device* device = nullptr;
    ctx->GetDevice(&device);
    if (!device) return;
    auto* res = static_cast<ID3D11Resource*>(mv_resource);
    D3D11_TEXTURE2D_DESC desc = {};
    auto* tex2d = static_cast<ID3D11Texture2D*>(res);
    if (tex2d != nullptr) { tex2d->GetDesc(&desc); }
    if (shader_injection.ssr_log_ngx > 0.5f) {
      std::ostringstream s;
      s << "[SSR] R3-MV: motion vectors captured"
        << " dims=" << desc.Width << "x" << desc.Height
        << " fmt=" << std::hex << desc.Format
        << " scale=(" << g_mv_scale_x << "," << g_mv_scale_y << ")";
      reshade::log::message(reshade::log::level::info, s.str().c_str());
    }
    device->CreateShaderResourceView(res, nullptr, &g_mv_srv);
    g_mv_logged = true;
  }
}

// Lazy-init: scan for nvngx_dlss.dll and attach NGX hooks for MV capture.
static void InitMotionVectorCapture() {
  static bool attempted = false;
  if (attempted || g_mv_srv != nullptr) return;
  attempted = true;

  auto* nvngx_module = renodx::utils::platform::FindModule("nvngx_dlss.dll");
  if (nvngx_module == nullptr) nvngx_module = renodx::utils::platform::FindModule("nvngx");
  if (nvngx_module == nullptr) {
    reshade::log::message(reshade::log::level::warning,
      "[SSR] R3-MV: nvngx_dlss.dll not found - motion vectors unavailable");
    return;
  }
  renodx::utils::vtable::Hook(nvngx_module,
      renodx::utils::dlss::nvngx::DLSS_HOOKS);
  renodx::utils::dlss::nvngx::on_evaluate_feature_d3d11 = OnNGXEvaluateFeature;
  if (shader_injection.ssr_log_ngx > 0.5f)
    reshade::log::message(reshade::log::level::info,
      "[SSR] R3-MV: NGX hooks attached for motion vector capture");
}


// ── Push constants builder (kai-vanillaplus style) ──

static std::array<float, 70> BuildGTVBAOPushConstants(DeviceData* data, bool denoise_last_pass,
                                                       float ssgi_enabled_override = -1.f,
                                                       bool foliage_mask_valid = false,
                                                       int denoise_stage = 0,
                                                       float atrous_step = 1.f) {
  std::array<float, 70> c = {};
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
  c[24] = shader_injection.gtvbao_fix_experimental;  // bitmask experimental fix selector (0-5)
  // ── GI parameters (IS-FAST repurpose) ──
  // isfast_passes (c[25]) = g_gi_enabled
  c[25] = (ssgi_enabled_override >= 0.f) ? ssgi_enabled_override : shader_injection.vbgi_enabled; // GI enable
  // isfast_samples (c[26]) = g_gi_light_exposure
  c[26] = std::clamp(g_vbgi_light_exposure, 0.001f, 10.f);    // HDR light buffer exposure
  // isfast_radius (c[27]) = g_gi_power
  c[27] = 1.5f;  // GI power (fixed, removed from UI)
  // isfast_edge_sensitivity (c[28]) = g_gi_intensity
  c[28] = std::clamp(shader_injection.vbgi_intensity, 0.f, 5.f);  // GI intensity
  // isfast_spatial_sigma (c[29]) = g_gi_saturation
  c[29] = std::clamp(shader_injection.vbgi_saturation, 0.f, 2.f); // GI saturation
  // isfast_hybrid_blend (c[30]) = g_gi_multibounce
  c[30] = shader_injection.vbgi_multibounce;                       // multi-bounce (0/1)
  c[31] = std::clamp(shader_injection.vbgi_multibounce_strength, 0.f, 10.f);  // feedback strength
  c[32] = std::clamp(shader_injection.vbgi_multibounce_saturation, 0.f, 2.f); // feedback saturation
  c[33] = std::clamp(shader_injection.vbgi_multibounce_max_clamp, 0.f, 20.f);  // multi-bounce max clamp
  c[34] = shader_injection.vbgi_debug_view;                         // VBGI debug view
  c[35] = g_isfast_enabled;                                          // IS-FAST enable (0/1)
  c[36] = std::clamp(g_isfast_strength, 0.f, 1.f);                   // IS-FAST noise strength
  c[37] = (data && data->isfast_texture_loaded) ? 1.f : 0.f;         // IS-FAST texture loaded flag
  c[38] = shader_injection.vbgi_adaptive_mode;                       // 0=GI color, 1=albedo
  c[39] = std::clamp(shader_injection.vbgi_adaptive_luma_strength, 0.f, 5.f); // 0=off
  c[40] = std::clamp(shader_injection.vbgi_adaptive_luma_blend, 0.f, 1.f);
  c[41] = std::clamp(g_isfast_spatial_scale, 0.25f, 4.f);          // IS-FAST spatial scale
  c[42] = std::clamp(g_isfast_temporal_speed, 0.f, 5.f);           // IS-FAST temporal speed
  c[43] = std::clamp(g_isfast_seed_offset, 0.f, 64.f);             // IS-FAST seed offset
  // ── Denoiser leak parameters ──
  c[44] = std::clamp(shader_injection.gtvbao_denoise_leak_threshold, 1.f, 4.f);
  c[45] = std::clamp(shader_injection.gtvbao_denoise_leak_strength, 0.f, 1.f);
  // ── Spatio-Temporal denoiser ──
  c[46] = shader_injection.gtvbao_denoiser_type;                     // 0=Spatial, 1=Spatio-Temporal
  // Temporal blend: base weight from frame count, scaled by blend strength
  {
    float fc = shader_injection.gtvbao_temporal_frame_count;
    float baseWeight = (fc > 1.f) ? ((fc - 1.f) / fc) : 0.f;
    float blendScale = std::clamp(shader_injection.gtvbao_temporal_blend, 0.f, 1.f);
    c[47] = std::clamp(baseWeight * blendScale, 0.0f, 0.98f);
  }
  c[48] = std::clamp(shader_injection.gtvbao_disocclusion_threshold, 0.001f, 0.1f);
  c[49] = shader_injection.gtvbao_noise_type;    // 0=IS-FAST, 1=IGN, 2=Hilbert
  // ── GTVBAO upgrade toggles ──
  c[50] = shader_injection.gtvbao_cdf_enabled;
  c[51] = shader_injection.gtvbao_cosine_enabled;
  c[52] = shader_injection.gtvbao_cosine_mode;
  c[53] = shader_injection.gtvbao_thickness_enabled;
  // ── Poisson denoiser ──
  c[54] = std::clamp(shader_injection.gtvbao_poisson_samples, 4.f, 32.f);
  c[55] = std::clamp(shader_injection.gtvbao_poisson_luma_phi, 0.5f, 20.f);
  c[56] = std::clamp(shader_injection.gtvbao_poisson_depth_phi, 0.5f, 20.f);
  c[57] = std::clamp(shader_injection.gtvbao_poisson_normal_phi, 0.5f, 20.f);
  c[58] = shader_injection.gtvbao_prefilter_enabled;
  // ── Foliage exclusion ──
  c[59] = shader_injection.gtvbao_exclude_foliage;
  c[60] = std::clamp(shader_injection.gtvbao_foliage_ao_value, 0.f, 1.f);
  c[61] = IsKai() ? 1.f : 0.f;
  // c[62] — foliage mask is only fresh when the pre-pass dispatched this frame.
  c[62] = foliage_mask_valid ? 1.f : 0.f;
  // ── Denoiser upgrades (R1-R4) ──
  c[63] = (float)denoise_stage;                                        // dispatch mode for denoise_last
  c[64] = std::clamp(shader_injection.gtvbao_temporal_normal_reject, 0.f, 1.f);
  c[65] = std::clamp(shader_injection.gtvbao_ghost_clamp, 0.f, 4.f);
  c[66] = shader_injection.gtvbao_atrous_enabled;
  c[67] = std::clamp(shader_injection.gtvbao_atrous_depth_sigma, 0.01f, 8.f);
  c[68] = std::clamp(shader_injection.gtvbao_atrous_normal_sigma, 1.f, 128.f);
  c[69] = std::clamp(atrous_step, 1.f, 8.f);                           // à-trous stride (1/2/4)
  return c;
}

// ── Pipeline creation ──

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
  dp(d->prefilter_pipeline); dp(d->main_low_pipeline); dp(d->main_medium_pipeline);
  dp(d->main_high_pipeline); dp(d->main_ultra_pipeline); dp(d->denoise_pipeline);
  dp(d->denoise_last_pipeline);
  dp(d->denoise_last_kai_pipeline);
  dp(d->denoise_last_sora2nd_pipeline);
  dp(d->atrous_pipeline);
  dp(d->normal_prep_pipeline);
  if (g_cpuopt_ensure_pipelines < 0.5f) {
    DestroyGTVBAODescriptorTables(dev, &d->prefilter_tables);
    DestroyGTVBAODescriptorTables(dev, &d->main_tables);
    DestroyGTVBAODescriptorTables(dev, &d->denoise_tables);
    DestroyGTVBAODescriptorTables(dev, &d->atrous_tables);
    DestroyGTVBAODescriptorTables(dev, &d->normal_prep_tables);
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
    push_constants_range.count = 54;
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
  // Foliage mask (t0=depth, t1=MRT normal → u0=foliage mask)
  if (!make_layout(2u, 1u, &d->foliage_mask_layout)) return false;
  EnsureGTVBAODescriptorTables(dev, d->foliage_mask_layout, &d->foliage_mask_tables);
  if (!d->foliage_mask_pipeline.handle)
    mkcs(__gtvbao_foliage_mask, "main", d->foliage_mask_layout, &d->foliage_mask_pipeline);
  // Main: 5 SRVs (depth MIPs, MRT normal, light buffer, IS-FAST noise, foliage mask) + 4 UAVs (AO, edges, GI, debug)
  if (!make_layout(5u, 4u, &d->main_layout)) return false;
  // Denoise: 6 SRVs (AO, edges, raw GI, history AO, depth mip, MRT normal) + 3 UAVs (denoised AO, denoised GI, history AO)
  if (!make_layout(6u, 3u, &d->denoise_layout)) return false;
  // À-trous: 3 SRVs (AO src, depth MIP0, pre-decoded normals) + 1 UAV (AO dst)
  if (!make_layout(3u, 1u, &d->atrous_layout)) return false;
  EnsureGTVBAODescriptorTables(dev, d->atrous_layout, &d->atrous_tables);
  // Normal prep: 1 SRV (MRT normal) + 1 UAV (decoded normals)
  if (!make_layout(1u, 1u, &d->normal_prep_layout)) return false;
  EnsureGTVBAODescriptorTables(dev, d->normal_prep_layout, &d->normal_prep_tables);
  // Multi-bounce accumulate: 2 SRVs (color, previous GI) + 1 UAV (accumulated)
  if (!make_layout(2u, 1u, &d->multibounce_layout)) return false;

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
  // À-trous wavelet spatial filter (R3)
  if (!d->atrous_pipeline.handle) mkcs(__gtvbao_atrous, "main", d->atrous_layout, &d->atrous_pipeline);
  // Normal pre-decode (à-trous perf)
  if (!d->normal_prep_pipeline.handle) mkcs(__gtvbao_normal_prep, "main", d->normal_prep_layout, &d->normal_prep_pipeline);
  if (!d->multibounce_pipeline.handle)   mkcs(__gtvbao_multibounce_accumulate, "main", d->multibounce_layout, &d->multibounce_pipeline);

  // ── SSGI is now integrated into the main pass (visibility bitmask AO+GI). ──
  // no separate VBGI pipeline needed — main_layout handles both AO and GI outputs.
  return d->prefilter_pipeline.handle && d->main_high_pipeline.handle
      && d->denoise_pipeline.handle && d->denoise_last_pipeline.handle
      && d->multibounce_pipeline.handle;
}

// ── IS-FAST DDS loader (64×64×64, RG8_UNORM Texture3D) ──

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
        "[IS-FAST] Embedded fast_noise_ea.dds too small — using IGN fallback.");
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
          "[IS-FAST] Embedded fast_noise_ea.dds truncated DX10 header — using IGN fallback.");
      return false;
    }
    DDS_HEADER_DXT10 dx10 = {};
    memcpy(&dx10, bytes + 4 + sizeof(hdr), sizeof(dx10));
    fmt = dx10.dxgiFormat;
    if (dx10.resourceDimension != 4) return false; // must be Texture3D
  }

  // DXGI_FORMAT_R8G8_UNORM = 49, expected dims: 128×128×32
  if (w != 128 || h != 128 || ddsDepth != 32 || fmt != 49) {
    if (g_isfast_debug_logging > 0.5f) {
      std::string msg = "[IS-FAST] Unexpected DDS: ";
      msg += std::to_string(w) + "x" + std::to_string(h) + "x" + std::to_string(ddsDepth);
      msg += " fmt=" + std::to_string(fmt) + " (expected 128x128x32 RG8_UNORM) — using IGN fallback.";
      reshade::log::message(reshade::log::level::warning, msg.c_str());
    }
    return false;
  }

  // Payload: 128×128×32 × 2 bytes = 1,048,576 bytes
  size_t dataSize = (size_t)w * h * ddsDepth * 2;
  if (byte_count < header_size + dataSize) {
    if (g_isfast_debug_logging > 0.5f)
      reshade::log::message(reshade::log::level::warning,
        "[IS-FAST] Embedded fast_noise_ea.dds truncated payload — using IGN fallback.");
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
  sub.slice_pitch = w * 2 * h;       // bytes per 2D slice (row_pitch × height)

  if (!dev->create_resource(rd, &sub, reshade::api::resource_usage::shader_resource,
                            &d->isfast_noise_texture)) {
    if (g_isfast_debug_logging > 0.5f)
      reshade::log::message(reshade::log::level::warning,
        "[IS-FAST] Failed to create 3D noise texture — using IGN fallback.");
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
      "[IS-FAST] Texture loaded (embedded): 128x128x32 RG8_UNORM — noise source: TEXTURE");
  return true;
}

// ── Dispatch ──

static bool RunGTVBAO(reshade::api::command_list* cl, DeviceData* d) {
  if (!d->captured_depth_srv.handle) return false;

  // ── Frame skips (independent per component) ──
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

  // ── IS-FAST noise texture (load once) ──
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

  uint32_t w = d->working_width, h = d->working_height;
  if (w < 64 || h < 64) return false;

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
    cl->push_constants(CS, d->prefilter_layout, kGtvbaoPushConstantsLayoutParam, 0, 70, pc.data());
  }
  cl->dispatch((w + 15) / 16, (h + 15) / 16, 1);
  bar(d->depth_mips_texture, UA, SR);
  if (shader_injection.gtvbao_debug_logging > 0.5f)
    reshade::log::message(reshade::log::level::info, "[GTVBAO] Pass 1 (prefilter) done.");

  // ── Multi-bounce accumulate (HDR light buffer + previous GI) ──
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

    if (mb_enabled && mb_gi_ready && mb_pipe_ok && !skip_multibounce) {
      bind_pipe(d->multibounce_pipeline);
      reshade::api::resource_view acc_color = mb_color_ok
          ? d->captured_color_srv : d->fallback_srv;
      reshade::api::resource_view acc_prev_gi = mb_prev_ok
          ? d->vbgi_denoised_srv : d->fallback_srv;
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
      cl->push_constants(CS, d->multibounce_layout, kGtvbaoPushConstantsLayoutParam, 0, 57,
                         BuildGTVBAOPushConstants(d, false).data());
      cl->dispatch((w + 7) / 8, (h + 7) / 8, 1);
      bar(d->multibounce_texture, UA, SR);
      if (shader_injection.vbgi_debug_logging > 0.5f)
        reshade::log::message(reshade::log::level::info, "[SSGI] MultiBounce: accumulate dispatched.");
    } else if (mb_enabled && !mb_gi_ready) {
      if (shader_injection.vbgi_debug_logging > 0.5f)
        reshade::log::message(reshade::log::level::info,
            "[SSGI] MultiBounce: SKIPPED (denoised GI not valid yet — first frame or GTVBAO never ran).");
    } else if (mb_enabled && !mb_pipe_ok) {
      if (shader_injection.vbgi_debug_logging > 0.5f)
        reshade::log::message(reshade::log::level::warning,
            "[SSGI] MultiBounce: SKIPPED (accumulate pipeline missing).");
    }
  }

  // ── Foliage mask pre-pass (full-res, reads MRT normal, writes R8_UINT) ──
  if (foliage_mask_valid) {
    uint32_t mkW = w, mkH = h;
    bind_pipe(d->foliage_mask_pipeline);
    reshade::api::resource_view fm_srvs[2] = {
        d->depth_mips_srv,                          // t0 — working depth (GetDimensions)
        d->captured_mrt_normal_srv                  // t1 — G-buffer normal (bit 15 test)
    };
    reshade::api::descriptor_table_update fu[4] = {
      {{},0,0,1,reshade::api::descriptor_type::sampler,&d->point_clamp_sampler},
      {{},0,0,1,reshade::api::descriptor_type::constant_buffer,&d->captured_scene_cbv_view},
      {{},0,0,2,reshade::api::descriptor_type::texture_shader_resource_view,fm_srvs},
      {{},0,0,1,reshade::api::descriptor_type::texture_unordered_access_view,&d->foliage_mask_uav},
    };
    apply_descriptors(d->foliage_mask_layout, &d->foliage_mask_tables, 4, fu);
    auto pc = BuildGTVBAOPushConstants(d, false);
    cl->push_constants(CS, d->foliage_mask_layout, kGtvbaoPushConstantsLayoutParam, 0, 70, pc.data());
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
        d->edges_uav,
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
    cl->push_constants(CS, d->main_layout, kGtvbaoPushConstantsLayoutParam, 0, 70, pc.data());
  }
  cl->dispatch((w + 7) / 8, (h + 7) / 8, 1);
  bar(d->ao_term_a_texture, UA, SR);
  bar(d->edges_texture, UA, SR);
  bar(d->vbgi_output_texture, UA, SR);  // GI output ready for denoise
  bar(d->debug_texture, UA, SR);         // Debug output ready for read
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

  // Pass 3: Denoise (ping-pong) — always run at least one pass to apply
  // the XE_GTAO_OCCLUSION_TERM_SCALE multiply-back (1.5x) in GTVBAO_Output.
  // When dpc==0 the DenoiseBlurBeta=10000 effectively disables blur.
  int dpc = (int)shader_injection.gtvbao_denoise_passes;
  if (dpc < 1) dpc = 1;
  {
    auto& last_pipe = IsKai() ? d->denoise_last_kai_pipeline
        : (IsSora2nd() ? d->denoise_last_sora2nd_pipeline : d->denoise_last_pipeline);
    const int dtype = (int)shader_injection.gtvbao_denoiser_type;
    const bool atrous_active = shader_injection.gtvbao_atrous_enabled > 0.5f
        && d->atrous_pipeline.handle != 0u;

    // ── À-trous helpers (shared by Spatio-Temporal and Spatial-only paths) ──

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
      cl->push_constants(CS, d->normal_prep_layout, kGtvbaoPushConstantsLayoutParam, 0, 70, pc_np.data());
      cl->dispatch((w + 7) / 8, (h + 7) / 8, 1);
      bar(d->normal_prep_texture, UA, SR);
    };

    // 3 à-trous iterations (strides 1/2/4). The last iteration folds the
    // ×OCCLUSION_TERM_SCALE multiply-back via denoise_is_last_pass.
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
        cl->push_constants(CS, d->atrous_layout, kGtvbaoPushConstantsLayoutParam, 0, 70, pc_a.data());
        cl->dispatch((w + 7) / 8, (h + 7) / 8, 1);
        bar(a_dst_tex, UA, SR);
        cur_b = !cur_b;
      }
      return cur_b;
    };

    // ── R2: Spatio-Temporal runs as two stages — temporal FIRST on raw main
    // output, then the spatial chain on the accumulated buffer. ──
    if (dtype == 1 && last_pipe.handle) {
      // Stage T (temporal-only): reads raw ao_term_a, blends history,
      // writes accumulated to ao_term_b + 16-bit history.
      bind_pipe(last_pipe);
      reshade::api::resource_view hist_srv = d->history_ao_read_from_a
          ? (d->history_ao_srv_a.handle ? d->history_ao_srv_a : d->fallback_srv)
          : (d->history_ao_srv_b.handle ? d->history_ao_srv_b : d->fallback_srv);
      reshade::api::resource_view hist_uav = d->history_ao_read_from_a
          ? (d->history_ao_uav_b.handle ? d->history_ao_uav_b : d->fallback_uav)
          : (d->history_ao_uav_a.handle ? d->history_ao_uav_a : d->fallback_uav);
      reshade::api::resource_view sv_t[6] = {d->ao_term_a_srv, d->edges_srv,
          d->vbgi_output_srv.handle ? d->vbgi_output_srv : d->fallback_srv,
          hist_srv, d->depth_mips_srv,
          d->captured_mrt_normal_srv.handle ? d->captured_mrt_normal_srv : d->fallback_srv};
      reshade::api::resource_view dn_uavs_t[3] = {d->ao_term_b_uav,
          d->vbgi_denoised_uav.handle ? d->vbgi_denoised_uav : d->fallback_uav,
          hist_uav};
      reshade::api::descriptor_table_update u_t[4] = {
        {{},0,0,1,reshade::api::descriptor_type::sampler,&d->point_clamp_sampler},
        {{},0,0,1,reshade::api::descriptor_type::constant_buffer,&d->captured_scene_cbv_view},
        {{},0,0,6,reshade::api::descriptor_type::texture_shader_resource_view,sv_t},
        {{},0,0,3,reshade::api::descriptor_type::texture_unordered_access_view,dn_uavs_t},
      };
      apply_descriptors(d->denoise_layout, &d->denoise_tables, 4, u_t);
      auto pc_t = BuildGTVBAOPushConstants(d, false, -1.f, false, /*stage*/1);
      cl->push_constants(CS, d->denoise_layout, kGtvbaoPushConstantsLayoutParam, 0, 70, pc_t.data());
      cl->dispatch((w + 7) / 8, (h + 7) / 8, 1);
      bar(d->ao_term_b_texture, UA, SR);
      d->history_ao_read_from_a = !d->history_ao_read_from_a;  // temporal stage owns history flip

      // ── Spatial chain from ao_term_b ──
      if (atrous_active) {
        // ── R3: à-trous wavelet chain — scale-back folded into last iteration ──
        run_normal_prep();
        d->gtvbao_final_in_b = run_atrous_chain(/*start_in_b*/true);
      } else {
        bool use_a = false;  // current data lives in ao_term_b after stage T
        for (int p = 0; p < dpc; ++p) {
          bool last = (p == dpc - 1);
          reshade::api::resource_view src, dst_uav;
          reshade::api::resource dst_tex;
          if (!use_a) { src = d->ao_term_b_srv; dst_uav = d->ao_term_a_uav; dst_tex = d->ao_term_a_texture; }
          else        { src = d->ao_term_a_srv; dst_uav = d->ao_term_b_uav; dst_tex = d->ao_term_b_texture; }
          bind_pipe(last ? last_pipe : d->denoise_pipeline);
          reshade::api::resource_view sv[6] = {src, d->edges_srv,
              d->vbgi_output_srv.handle ? d->vbgi_output_srv : d->fallback_srv,
              d->fallback_srv,  // history not read by spatial stages
              d->depth_mips_srv,
              d->captured_mrt_normal_srv.handle ? d->captured_mrt_normal_srv : d->fallback_srv};
          reshade::api::resource_view dn_uavs[3] = {dst_uav,
              d->vbgi_denoised_uav.handle ? d->vbgi_denoised_uav : d->fallback_uav,
              d->fallback_uav};  // spatial stages never write history
          reshade::api::descriptor_table_update u[4] = {
            {{},0,0,1,reshade::api::descriptor_type::sampler,&d->point_clamp_sampler},
            {{},0,0,1,reshade::api::descriptor_type::constant_buffer,&d->captured_scene_cbv_view},
            {{},0,0,6,reshade::api::descriptor_type::texture_shader_resource_view,sv},
            {{},0,0,3,reshade::api::descriptor_type::texture_unordered_access_view,dn_uavs},
          };
          apply_descriptors(d->denoise_layout, &d->denoise_tables, 4, u);
          auto pc = BuildGTVBAOPushConstants(d, last, -1.f, false, /*stage*/ last ? 2 : 0);
          cl->push_constants(CS, d->denoise_layout, kGtvbaoPushConstantsLayoutParam, 0, 70, pc.data());
          cl->dispatch((w + 7) / 8, (h + 7) / 8, 1);
          bar(dst_tex, UA, SR);
          use_a = !use_a;
          if (last) d->gtvbao_final_in_b = !use_a;  // final result lands in the just-written buffer
        }
      }
    } else if (dtype == 0 && atrous_active && last_pipe.handle) {
      // ── Spatial-only + à-trous: wavelet chain replaces the combined final
      // dispatch; scale-back folds into the last iteration. A GI-only tail
      // (stage 4) keeps the GI bilateral running. ──
      run_normal_prep();
      d->gtvbao_final_in_b = run_atrous_chain(/*start_in_b*/false);  // main wrote ao_term_a
      bind_pipe(last_pipe);
      reshade::api::resource_view sv_g[6] = {
          d->fallback_srv,                                                    // t0 AO (unused by stage 4)
          d->edges_srv,                                                       // t1 depth for GI filter
          d->vbgi_output_srv.handle ? d->vbgi_output_srv : d->fallback_srv,   // t2 raw GI
          d->fallback_srv, d->depth_mips_srv,
          d->captured_mrt_normal_srv.handle ? d->captured_mrt_normal_srv : d->fallback_srv};
      reshade::api::resource_view dn_uavs_g[3] = {d->fallback_uav,           // u0 untouched by stage 4
          d->vbgi_denoised_uav.handle ? d->vbgi_denoised_uav : d->fallback_uav,
          d->fallback_uav};
      reshade::api::descriptor_table_update u_g[4] = {
        {{},0,0,1,reshade::api::descriptor_type::sampler,&d->point_clamp_sampler},
        {{},0,0,1,reshade::api::descriptor_type::constant_buffer,&d->captured_scene_cbv_view},
        {{},0,0,6,reshade::api::descriptor_type::texture_shader_resource_view,sv_g},
        {{},0,0,3,reshade::api::descriptor_type::texture_unordered_access_view,dn_uavs_g},
      };
      apply_descriptors(d->denoise_layout, &d->denoise_tables, 4, u_g);
      auto pc_g = BuildGTVBAOPushConstants(d, true, -1.f, false, /*stage*/4);
      cl->push_constants(CS, d->denoise_layout, kGtvbaoPushConstantsLayoutParam, 0, 70, pc_g.data());
      cl->dispatch((w + 7) / 8, (h + 7) / 8, 1);
      // vbgi_denoised barrier happens after the Pass-3 block.
    } else {
      // ── Legacy path (Spatial / Poisson): unchanged combined structure. ──
      bool use_a = true;
      for (int p = 0; p < dpc; ++p) {
        bool last = (p == dpc - 1);
        reshade::api::resource_view src, dst_uav;
        reshade::api::resource dst_tex;
        if (use_a) { src = d->ao_term_a_srv; dst_uav = d->ao_term_b_uav; dst_tex = d->ao_term_b_texture; }
        else       { src = d->ao_term_b_srv; dst_uav = d->ao_term_a_uav; dst_tex = d->ao_term_a_texture; }
        bind_pipe(last ? last_pipe : d->denoise_pipeline);
        // Ping-pong history: read from last frame's write target, write to other buffer
        reshade::api::resource_view hist_srv = d->history_ao_read_from_a
            ? (d->history_ao_srv_a.handle ? d->history_ao_srv_a : d->fallback_srv)
            : (d->history_ao_srv_b.handle ? d->history_ao_srv_b : d->fallback_srv);
        reshade::api::resource_view hist_uav = d->history_ao_read_from_a
            ? (d->history_ao_uav_b.handle ? d->history_ao_uav_b : d->fallback_uav)
            : (d->history_ao_uav_a.handle ? d->history_ao_uav_a : d->fallback_uav);
        reshade::api::resource_view sv[6] = {src, d->edges_srv,
            d->vbgi_output_srv.handle ? d->vbgi_output_srv : d->fallback_srv,  // raw GI
            hist_srv,                                                           // history AO (read)
            d->depth_mips_srv,                                                  // depth MIP0 for reprojection
            d->captured_mrt_normal_srv.handle ? d->captured_mrt_normal_srv : d->fallback_srv}; // MRT normal
        reshade::api::resource_view dn_uavs[3] = {dst_uav,
            d->vbgi_denoised_uav.handle ? d->vbgi_denoised_uav : d->fallback_uav,  // denoised GI
            hist_uav};                                                              // history AO (write)
        reshade::api::descriptor_table_update u[4] = {
          {{},0,0,1,reshade::api::descriptor_type::sampler,&d->point_clamp_sampler},
          {{},0,0,1,reshade::api::descriptor_type::constant_buffer,&d->captured_scene_cbv_view},
          {{},0,0,6,reshade::api::descriptor_type::texture_shader_resource_view,sv},
          {{},0,0,3,reshade::api::descriptor_type::texture_unordered_access_view,dn_uavs},
        };
        apply_descriptors(d->denoise_layout, &d->denoise_tables, 4, u);
        auto pc = BuildGTVBAOPushConstants(d, last, -1.f, false, /*stage*/0);
        cl->push_constants(CS, d->denoise_layout, kGtvbaoPushConstantsLayoutParam, 0, 70, pc.data());
        cl->dispatch((w + 7) / 8, (h + 7) / 8, 1);
        bar(dst_tex, UA, SR);
        use_a = !use_a;
        if (last) {
          d->history_ao_read_from_a = !d->history_ao_read_from_a;  // only final pass writes history
          d->gtvbao_final_in_b = !use_a;
        }
      }
    }
  }
  bar(d->vbgi_denoised_texture, UA, SR);  // Denoised GI ready for t23 read
  if (!d->vbgi_denoised_valid) {
    d->vbgi_denoised_valid = true;            // Multi-bounce feedback active next frame
    if (shader_injection.vbgi_debug_logging > 0.5f)
      reshade::log::message(reshade::log::level::info,
          "[SSGI] MultiBounce: denoised GI now valid — accumulate will run next frame.");
  }
  if (shader_injection.gtvbao_debug_logging > 0.5f)
    reshade::log::message(reshade::log::level::info, "[GTVBAO] All passes complete.");
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
      break;
    case DLL_PROCESS_DETACH:
      reshade::unregister_event<reshade::addon_event::init_device>(OnInitDevice);
      reshade::unregister_event<reshade::addon_event::destroy_device>(OnDestroyDevice);
      reshade::unregister_event<reshade::addon_event::init_swapchain>(OnInitSwapchain);
      reshade::unregister_event<reshade::addon_event::destroy_swapchain>(OnDestroySwapchain);
      reshade::unregister_event<reshade::addon_event::present>(OnPresent);
      reshade::unregister_event<reshade::addon_event::bind_descriptor_tables>(OnBindDescriptorTables);
      reshade::unregister_event<reshade::addon_event::push_descriptors>(OnPushDescriptorsCapture);
      reshade::unregister_addon(h_module);
      break;
  }
  renodx::utils::settings::Use(fdw_reason, &settings);
  renodx::mods::shader::Use(fdw_reason, custom_shaders, &shader_injection);
  return TRUE;
}
