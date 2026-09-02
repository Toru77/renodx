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
  // —— Dynamic Cubemaps (Sora 2nd) ——
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
  .dynCube_parallax_enabled = 0.f,
  .dynCube_parallax_box_size_x = 20.f,
  .dynCube_parallax_box_size_y = 10.f,
  .dynCube_parallax_box_size_z = 20.f,
  .dynCube_parallax_debug = 0.f,
  .dynCube_reflect_sign_flip = 0.f,
  .dynCube_ssr_enabled = 0.f,
  .dynCube_ssr_quality = 1.f,
  .dynCube_ssr_blur = 2.f,
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

// ── Dynamic Cubemaps (Sora 2nd) — standalone t17 replacement ──
constexpr uint32_t kDynCubeRegister = 17u; // t17 texEnvMap_g
constexpr uint32_t kDynCubeHistPosRegister = 29u; // t29 dynCubeHistPosTex (debug 11/12)
constexpr uint32_t kDynCubeVanillaRegister = 30u; // t30 dynCubeVanillaTex (vanilla cube fallback)
constexpr uint32_t kDynCubeSSRRegister = 31u;     // t31 dynCubeSSRTex (blurred SSR result)
constexpr uint32_t kDynCubeSSRRawRegister = 32u;  // t32 dynCubeSSRRawTex (raw SSR, debug 17)
constexpr uint32_t kDynCubeDefaultSize = 128u;
static uint32_t DynCubeResolveSize(float v);

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

// A complete Dynamic Cubemap resource set for one cube resolution. Cached per size so a
// resolution change never destroys/recreates GPU allocations (D3D11 release-timing leak).
struct DynCubeSet {
  struct HistSet {
    reshade::api::resource color;             // RGBA16F cube-compatible
    reshade::api::resource_view color_cube_srv;
    reshade::api::resource_view color_arr_srv;
    reshade::api::resource_view color_uav;
    reshade::api::resource pos;               // rgb=scaled pos, a=validity
    reshade::api::resource_view pos_arr_srv;
    reshade::api::resource_view pos_cube_srv;
    reshade::api::resource_view pos_uav;
    reshade::api::resource contrib;           // R16F
    reshade::api::resource_view contrib_arr_srv;
    reshade::api::resource_view contrib_uav;
  } hist[2];
  reshade::api::resource cam[2];
  reshade::api::resource_view cam_srv[2];
  reshade::api::resource_view cam_uav[2];
  reshade::api::resource charmask;
  reshade::api::resource_view charmask_srv;
  reshade::api::resource_view charmask_uav;
  reshade::api::resource ggx_in;
  reshade::api::resource_view ggx_in_cube_srv;
  reshade::api::resource ggx_out[2];
  reshade::api::resource_view ggx_out_cube_srv[2];
  reshade::api::resource_view ggx_out_mip_uav[2][8];
  reshade::api::resource solid_cube;
  reshade::api::resource_view solid_cube_srv;
  reshade::api::resource_view solid_cube_uav;
  uint32_t mip_count = 8;
};

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
  reshade::api::resource_view captured_vanilla_env_srv = {};  // game's texEnvMap_g (t17) binding — vanilla cube fallback
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

  // ── Dynamic Cubemaps (Sora 2nd) — Phase 0A/B + Phase 1 history ──
  // Aliases to the current (just-written) history set — dyncube_srv is the cube SRV for t17.
  reshade::api::resource dyncube_texture = {};        // current color resource (alias)
  reshade::api::resource_view dyncube_srv = {};       // current color cube SRV (alias, t17)
  reshade::api::resource_view dyncube_uav = {};       // current color UAV (alias, capture/solid write)
  reshade::api::sampler dyncube_sampler = {};         // point clamp
  reshade::api::pipeline_layout dyncube_capture_layout = {};
  reshade::api::pipeline dyncube_capture_pipeline = {};
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
  uint32_t dyncube_hist_cur = 0;           // current write set (0=A,1=B)
  bool dyncube_needs_reset = true;         // clear history + first-frame reset
  bool dyncube_was_enabled = false;        // rising-edge latch for enabled->reset
  reshade::api::sampler dyncube_linear_sampler = {};     // trilinear clamp (GGX input mips)
  // Character mask (Phase 2)
  reshade::api::resource dyncube_charmask = {};          // RGBA16F cube, 1 mip (character mask)
  reshade::api::resource_view dyncube_charmask_srv = {};   // TextureCube SRV (for debug 9)
  reshade::api::resource_view dyncube_charmask_uav = {};   // Texture2DArray UAV (compute write)
  // Phase 3 GGX prefilter — double-buffered filtered cube (Active/Building) so a
  // partially-written cube is never exposed to lighting.
  uint32_t dyncube_mip_count = 8;                        // computed mips (8 for 128..1024)
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
  // Dedicated solid-color debug cube (Phase 0A). Never aliases history/ggx resources.
  reshade::api::resource dyncube_solid_cube = {};          // RGBA16F cube, 1 mip
  reshade::api::resource_view dyncube_solid_cube_srv = {};   // TextureCube SRV (t17 for debug 3)
  reshade::api::resource_view dyncube_solid_cube_uav = {};   // Texture2DArray UAV (solid write)
  // Simple SSR (screen-space ray march) — independent of history/ggx/inferred.
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
  GTVBAODescriptorTableSet dyncube_ssr_tables = {};
  reshade::api::pipeline_layout dyncube_ssr_blur_layout = {};
  reshade::api::pipeline dyncube_ssr_blur_pipeline = {};
  GTVBAODescriptorTableSet dyncube_ssr_blur_tables = {};
  uint32_t dyncube_pending_size = 0;                     // requested cube size, recreated at frame boundary
  bool dyncube_pending_recreate = false;                 // recreate (old set release deferred to Present)
  bool dyncube_pending_destroy = false;                  // feature disabled -> free the set at Present
  std::map<uint32_t, DynCubeSet> dyncube_cache;          // cached per-size sets (never destroyed during resize)
};

static void CreateGTVBAOResources(reshade::api::device* device, DeviceData* data,
                                   uint32_t gw, uint32_t gh);
static void DestroyGTVBAOResources(reshade::api::device* device, DeviceData* data);
static bool CreateComputePipelinesIfNeeded(reshade::api::device* device, DeviceData* data);
static bool RunGTVBAO(reshade::api::command_list* cmd_list, DeviceData* data);
static bool LoadISFASTNoiseTexture(reshade::api::device* dev, DeviceData* d);
// ── Dynamic Cubemaps — forward decls ──
static bool CreateDynCubeResources(reshade::api::device* dev, DeviceData* d, uint32_t size);
static void DestroyDynCubeResources(reshade::api::device* dev, DeviceData* d);
static void SaveActiveToCache(reshade::api::device* dev, DeviceData* d);
static bool RestoreFromCache(reshade::api::device* dev, DeviceData* d, uint32_t size);
static void DestroyDynCubeCache(reshade::api::device* dev, DeviceData* d);
static void UnbindDynCubeComputeState(reshade::api::command_list* cl);
static bool CreateDynCubePipelinesIfNeeded(reshade::api::device* dev, DeviceData* d);
static bool RunDynCubeSolid(reshade::api::command_list* cl, DeviceData* d);
static bool RunDynCubeCapture(reshade::api::command_list* cl, DeviceData* d);
static bool RunDynCubeInference(reshade::api::command_list* cl, DeviceData* d);
static bool RunDynCubeFilter(reshade::api::command_list* cl, DeviceData* d, bool ggxOn);
static bool RunDynCubeSSR(reshade::api::command_list* cl, DeviceData* d);

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

// ── Kai + Daybreak 2 character lighting callback (Env SSS + Character Shadowing) ──
static bool OnBeforeCharLightingDraw(reshade::api::command_list* cmd_list) {
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
    // —— Dynamic Cubemaps — standalone t17 replacement ——
    new renodx::utils::settings::Setting{
      .key = "DynCubeEnabled", .binding = &shader_injection.dynCube_enabled,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 1.f, .label = "Dynamic Cubemaps", .section = "Dynamic Cubemaps",
      .tooltip = "Replace static texEnvMap_g(t17) with screen-captured dynamic cubemap. Off = vanilla.",
      .labels = {"Off", "On"},
    },
    new renodx::utils::settings::Setting{
      .key = "DynCubeHistory", .binding = &shader_injection.dynCube_history,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 1.f, .label = "Temporal", .section = "Dynamic Cubemaps",
      .tooltip = "Phase1: temporal accumulation. On = blend current capture with previous history. Off = no history (fresh capture each frame).",
      .labels = {"Off", "On"},
      .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "DynCubeDebug", .binding = &shader_injection.dynCube_debug,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 0.f, .label = "Debug View", .section = "Dynamic Cubemaps",
      .tooltip = "0=Normal (dynamic cube in lighting), 1=Show Dynamic Cube, 2=Face Visualization, 3=Solid Face Colors, 4=No Override (vanilla t17), 5=History Validity, 6=History Contribution, 7=Character Mask, 8=GGX Filtered Cube (mip-selectable), 9=SSR Result (blurred), 10=SSR Confidence, 11=Reflection Source, 12=SSR Raw, 13=SSR Edge Fade.",
      .labels = {"Normal", "Show Cube", "Face Viz", "Solid Colors", "No Override", "History Validity", "History Contribution", "Character Mask", "GGX Filtered", "SSR Result", "SSR Confidence", "Reflection Source", "SSR Raw", "SSR Edge Fade"},
      .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "DynCubeResolution", .binding = &shader_injection.dynCube_resolution,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 4.f, .label = "Resolution", .section = "Dynamic Cubemaps",
      .tooltip = "Cubemap face size. 128 = quality eval floor, 1024 = max. Preview rectangle is clamped for visibility.",
      .labels = {"128", "256", "512", "768", "1024"},
      .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "DynCubeCharacterCapture", .binding = &shader_injection.dynCube_character_capture,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 1.f, .label = "Character Capture", .section = "Dynamic Cubemaps",
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
      .default_value = 4.f, .label = "Dynamic Cubemap Blur", .section = "Dynamic Cubemaps",
      .tooltip = "Artistic mip-offset blur on the dynamic cube sample (uses the existing GGX/HW mip chain, no extra pass). 0 = normal sharpness; fractional values give smooth trilinear control; higher = progressively blurrier. Independent of material roughness.",
      .min = 0.f, .max = 8.f, .format = "%.1f",
      .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "DynCubeUpdateInterval", .binding = &shader_injection.dynCube_capture_interval,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 1.f, .label = "Dynamic Cubemap Update Interval", .section = "Dynamic Cubemaps",
      .tooltip = "Frames between new cubemap captures (1=fastest 2-stage cadence, 2=every 2 frames, 4=every 4 frames, 8=every 8 frames). Capture and filter run on separate frames; the previous completed cube stays visible between updates.",
      .labels = {"1", "2", "3", "4", "5", "6", "7", "8"},
      .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f; },
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
    },
    new renodx::utils::settings::Setting{
      .key = "DynCubeCaptureBoost", .binding = &shader_injection.dynCube_capture_boost,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 1.f, .label = "Capture Boost", .section = "Dynamic Cubemaps",
      .tooltip = "Brightness multiplier for captured color before writing to cubemap. 1.0 = neutral, >1 brightens reflections.",
      .min = 0.f, .max = 4.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "DynCubeHistoryBlend", .binding = &shader_injection.dynCube_history_blend,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.5f, .label = "History Blend", .section = "Dynamic Cubemaps",
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
    },
    new renodx::utils::settings::Setting{
      .key = "DynCubeDebugMip", .binding = &shader_injection.dynCube_debug_mip,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 0.f, .label = "Debug Mip", .section = "Dynamic Cubemaps",
      .tooltip = "Mip level for the GGX filtered cube preview when Debug View=8. 0=sharp history, 7=strongly blurred.",
      .labels = {"0", "1", "2", "3", "4", "5", "6", "7"},
      .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f && shader_injection.dynCube_debug == 8.f; },
    },
    new renodx::utils::settings::Setting{
      .key = "DynCubeForceMip", .binding = &shader_injection.dynCube_force_mip,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = -1.f, .label = "Force Cubemap Mip", .section = "Dynamic Cubemaps",
      .tooltip = "Debug: force the t17 reflection mip. -1 = normal roughness LOD. 0..7 = always sample that mip (verifies the GGX chain reaches the reflection).",
      .min = -1.f, .max = 7.f, .format = "%d",
      .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "DynCubeParallax", .binding = &shader_injection.dynCube_parallax_enabled,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 1.f, .label = "Parallax Correction", .section = "Dynamic Cubemaps",
      .tooltip = "OFF = camera-centered cubemap lookup (current). ON = finite probe-box parallax correction for the t17 reflection lookup.",
      .labels = {"Off", "On"},
      .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "DynCubeParallaxBoxX", .binding = &shader_injection.dynCube_parallax_box_size_x,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 42.f, .label = "Parallax Box Size X", .section = "Dynamic Cubemaps",
      .tooltip = "Probe box X extent (world units), centered on the camera. Tune to the room/area bounds.",
      .min = 1.f, .max = 100.f, .format = "%.1f",
      .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "DynCubeParallaxBoxY", .binding = &shader_injection.dynCube_parallax_box_size_y,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 42.f, .label = "Parallax Box Size Y", .section = "Dynamic Cubemaps",
      .tooltip = "Probe box Y extent (world units), centered on the camera. Tune to the room/area bounds.",
      .min = 1.f, .max = 100.f, .format = "%.1f",
      .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "DynCubeParallaxBoxZ", .binding = &shader_injection.dynCube_parallax_box_size_z,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 42.f, .label = "Parallax Box Size Z", .section = "Dynamic Cubemaps",
      .tooltip = "Probe box Z extent (world units), centered on the camera. Tune to the room/area bounds.",
      .min = 1.f, .max = 100.f, .format = "%.1f",
      .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "DynCubeParallaxDebug", .binding = &shader_injection.dynCube_parallax_debug,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 0.f, .label = "Parallax Debug", .section = "Dynamic Cubemaps",
      .tooltip = "ON = tint the reflection by the probe-box exit face hit (requires Parallax Correction ON). For tuning the box and verifying ray-box intersection.",
      .labels = {"Off", "On"},
      .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f && shader_injection.dynCube_parallax_enabled > 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "DynCubeReflectSignFlip", .binding = &shader_injection.dynCube_reflect_sign_flip,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 1.f, .label = "Reflection Sign Flip", .section = "Dynamic Cubemaps",
      .tooltip = "Debug A/B: OFF = use the mathematical reflect ray (current). ON = use the physical reflection ray (negated) for box-parallax correction. Test which direction the correction moves.",
      .labels = {"Off", "On"},
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
    },
    new renodx::utils::settings::Setting{
      .key = "DynCubeSSRQuality", .binding = &shader_injection.dynCube_ssr_quality,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 1.f, .label = "SSR Quality", .section = "Dynamic Cubemaps",
      .tooltip = "Sample count and search distance. Low = 10 samples / 12 units. Medium = 16 / 20. High = 24 / 32.",
      .labels = {"Low", "Medium", "High"},
      .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f && shader_injection.dynCube_ssr_enabled > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "DynCubeSSRBlur", .binding = &shader_injection.dynCube_ssr_blur,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 8.f, .label = "SSR Blur", .section = "Dynamic Cubemaps",
      .tooltip = "Separable Gaussian blur sigma on the SSR result. 0 = sharp/raw SSR, higher = progressively blurrier.",
      .min = 0.f, .max = 8.f, .format = "%.1f",
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
      .default_value = 0.4f, .label = "SSR Edge Fade", .section = "Dynamic Cubemaps",
      .tooltip = "Screen-edge vignette strength on SSR confidence. 0 = none, 1 = fades over 25% from each edge.",
      .min = 0.f, .max = 1.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f && shader_injection.dynCube_ssr_enabled > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "DynCubeSSRGrazingFade", .binding = &shader_injection.dynCube_ssr_grazing_fade,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.3f, .label = "SSR Grazing Fade", .section = "Dynamic Cubemaps",
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
      .default_value = 1.0f, .label = "SSR Character Occlusion Reduction", .section = "Dynamic Cubemaps",
      .tooltip = "How much SSR confidence is reduced when the ray hits a character from a horizontal (floor/water) surface. Attacks third-person disocclusion rings; 0 = off. Vertical mirrors (low upness) are unaffected.",
      .min = 0.f, .max = 1.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f && shader_injection.dynCube_ssr_enabled > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "DynCubeSSRCharOccUpness", .binding = &shader_injection.dynCube_ssr_char_occ_upness,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.f, .label = "SSR Character Occlusion Upness", .section = "Dynamic Cubemaps",
      .tooltip = "Surface upness threshold where the character-hit confidence reduction begins (smooth ±0.25 band). 0 = any up-facing surface, 0.5 = mostly horizontal, 1 = only fully horizontal (floor/water).",
      .min = 0.f, .max = 1.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.dynCube_enabled > 0.5f && shader_injection.dynCube_ssr_enabled > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "DynCubeVanillaBlur", .binding = &shader_injection.dynCube_vanilla_blur,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 1.f, .label = "Vanilla Cubemap Blur", .section = "Dynamic Cubemaps",
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
    DestroyDynCubeResources(device, d);
    DestroyDynCubeCache(device, d);
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
    DestroyDynCubeResources(sc->get_device(), d);
    DestroyDynCubeCache(sc->get_device(), d);
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
    DestroyDynCubeResources(sc->get_device(), d);
    DestroyDynCubeCache(sc->get_device(), d);
    return;
  }
  DestroyGTVBAOResources(sc->get_device(), d);
  DestroyDynCubeResources(sc->get_device(), d);
  DestroyDynCubeCache(sc->get_device(), d);
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
  // Fast path: no feature consumes the captured snapshots -> skip ALL capture work.
  // (Fired on every push_descriptors in the frame; without this gate the body below
  //  runs thousands of times per frame even when every addon feature is disabled.)
  if (shader_injection.gtvbao_mode < 0.5f
      && shader_injection.dynCube_enabled < 0.5f) {
    return;
  }
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
    // Capture the game's vanilla texEnvMap_g (t17) binding — the vanilla cubemap
    // fallback layer. Only from the lighting shader, before our own t17 override,
    // and only when Dynamic Cubemaps is enabled (no work when off).
    if (update.binding == 17u && update.count >= 1
        && views[0].handle != 0u && shader_injection.dynCube_enabled > 0.5f) {
      auto* ss = renodx::utils::shader::GetCurrentState(cmd_list);
      if (ss) {
        uint32_t hash = renodx::utils::shader::GetCurrentPixelShaderHash(ss);
        if (IsLightingShader(hash)) {
          d->captured_vanilla_env_srv = views[0];
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

  // DynCube disabled -> free the resource set at the frame boundary (GPU idle on the old set).
  // Runs before any early-out so it also happens when every feature is off.
  if (d->dyncube_pending_destroy) {
    d->dyncube_pending_destroy = false;
    d->dyncube_pending_recreate = false;
    if (shader_injection.dynCube_debug_logging > 0.5f && d->dyncube_resources_created) {
      const uint32_t sz = d->dyncube_size;
      const uint32_t mips = d->dyncube_mip_count;
      const double mips_wt = (mips >= 2) ? (4.0 / 3.0) : 1.0;
      const double base = (double)sz * (double)sz * 6.0;
      const double bytes = 2.0 * base * (8.0 + 8.0 + 2.0) + base * mips_wt * 8.0 * 3.0 + 2.0 * base * 8.0;
      reshade::log::message(reshade::log::level::info,
        (std::string("[DynCube] disabled: freed ") + std::to_string(sz) + "x" + std::to_string(sz) +
         " set (~" + std::to_string((long long)(bytes / (1024.0 * 1024.0))) + " MB)").c_str());
    }
    DestroyDynCubeResources(dev, d);
    DestroyDynCubeCache(dev, d);  // also free all cached per-size sets
  }

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

  // DynCube can dispatch independently of GTVBAO — must not early-out
  const bool dynCube_present_active = shader_injection.dynCube_enabled > 0.5f;
  if (shader_injection.gtvbao_mode < 0.5f && !dynCube_present_active) return;
  if (d->frame_index <= kGTVBAOStartupGuardFrames) {
    if (d->frame_index == kGTVBAOStartupGuardFrames) {
      reshade::log::message(reshade::log::level::info,
        "[GTVBAO] Startup guard complete — dispatch begins next frame.");
    }
    return;
  }
  if (d->frame_index < d->resize_guard_until_frame) return;

  // DynCube cube-resolution change: at the frame boundary, reuse a cached set of the
  // target size when available (zero create/destroy); otherwise cache the current set
  // and create a fresh one. Old sets are never destroyed during resize (VRAM leak).
  if (d->dyncube_pending_recreate && d->dyncube_pending_size != 0u) {
    const uint32_t wantSize = d->dyncube_pending_size;
    SaveActiveToCache(dev, d);  // cache the current set first (never destroy during resize)
    const bool reused = RestoreFromCache(dev, d, wantSize);
    if (!reused) {
      CreateDynCubeResources(dev, d, wantSize);
      CreateDynCubePipelinesIfNeeded(dev, d);
    }
    if (shader_injection.dynCube_debug_logging > 0.5f) {
      reshade::log::message(reshade::log::level::info,
        (std::string("[DynCube] resized to ") + std::to_string(wantSize) +
         (reused ? " (cache hit)" : "")).c_str());
    }
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
    if (gw > 0u && gh > 0u
        && (!d->resources_created || too_small
            || gw != d->last_created_game_width
            || gh != d->last_created_game_height)) {
      CreateGTVBAOResources(dev, d, gw, gh);
      d->last_created_game_width = gw;
      d->last_created_game_height = gh;
      d->resources_created = true;
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

  // Inline dispatch active (deferred off) — GTVBAO runs during lighting pass, not here.
  if (!d->deferred_pending || !d->deferred_depth_srv.handle) {
    capture_light_buffer_for_next_frame();
    // DynCube debug face preview — must run even when GTVBAO deferred off
    if (d->dyncube_resources_created && d->dyncube_srv.handle && shader_injection.dynCube_enabled >0.5f
        && (shader_injection.dynCube_debug == 1.f || shader_injection.dynCube_debug == 2.f
            || shader_injection.dynCube_debug == 5.f || shader_injection.dynCube_debug == 6.f
            || shader_injection.dynCube_debug == 7.f || shader_injection.dynCube_debug == 8.f)) {
      int face = (int)std::clamp(shader_injection.dynCube_debug_face, 0.f, 5.f);
      const uint32_t outSet = 1u - d->dyncube_hist_cur; // freshly written set (alias target)
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

  // ── DynCube debug face preview — samples capture resources directly (not t17) ──
  if (d->dyncube_resources_created && d->dyncube_srv.handle && shader_injection.dynCube_enabled >0.5f
      && (shader_injection.dynCube_debug == 1.f || shader_injection.dynCube_debug == 2.f
          || shader_injection.dynCube_debug == 5.f || shader_injection.dynCube_debug == 6.f
          || shader_injection.dynCube_debug == 7.f || shader_injection.dynCube_debug == 8.f)) {
    int face = (int)std::clamp(shader_injection.dynCube_debug_face, 0.f, 5.f);
    const uint32_t outSet = 1u - d->dyncube_hist_cur; // freshly written set (alias target)
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

  // Dynamic Cubemaps — standalone, must run even when GTVBAO/SSR off (any Falcom title)
  const bool dyncube_active = shader_injection.dynCube_enabled > 0.5f;
  const bool gtvbao_active = shader_injection.gtvbao_mode > 0.5f;
  // Rising/falling-edge latch (runs before the early-out so it also happens when every
  // feature is off): rising edge resets history; falling edge defers a resource free to
  // OnPresent (the old set is released at the frame boundary).
  auto* dd0 = cmd_list ? cmd_list->get_device()->get_private_data<DeviceData>() : nullptr;
  if (dd0) {
    if (dyncube_active && !dd0->dyncube_was_enabled) dd0->dyncube_needs_reset = true;
    if (!dyncube_active && dd0->dyncube_was_enabled) dd0->dyncube_pending_destroy = true;
    dd0->dyncube_was_enabled = dyncube_active;
  }
  if (!gtvbao_active && !dyncube_active) return true;
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

  // ── Dynamic Cubemaps (Sora2nd): t17 override — Phase 0A/B + Phase 1/2 standalone ──
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

    // ── Multi-frame Dynamic Cubemap scheduler — runs ONCE per present frame ──
    if (dd->dyncube_sched_frame != dd->frame_index) {
      dd->dyncube_sched_frame = dd->frame_index;
      const uint32_t interval = std::max(1u, (uint32_t)std::clamp(shader_injection.dynCube_capture_interval, 1.f, 16.f));

      // SSR runs every frame, independent of the Dynamic Cubemap update interval.
      if (shader_injection.dynCube_ssr_enabled > 0.5f) {
        (void)RunDynCubeSSR(cmd_list, dd);
      }

      // Seed: on first activation (no completed cube yet) capture + filter immediately so
      // the active cube is valid from the first frame (never a black/partial cube).
      if (!dd->dyncube_ggx_valid && RunDynCubeCapture(cmd_list, dd)) {
        if (RunDynCubeFilter(cmd_list, dd, (shader_injection.dynCube_ggx > 0.5f))) {
          dd->dyncube_ggx_active = 1u - dd->dyncube_ggx_active;
          dd->dyncube_ggx_valid = true;
        }
        dd->dyncube_phase = DeviceData::DynCubePhase::Done;
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
          if (RunDynCubeFilter(cmd_list, dd, (shader_injection.dynCube_ggx > 0.5f))) {
            dd->dyncube_ggx_active = 1u - dd->dyncube_ggx_active;
            dd->dyncube_ggx_valid = true;
            dd->dyncube_phase = DeviceData::DynCubePhase::Done;
          }
          break;
      }

      // ── Throttled scheduler log (1/sec) — verify the state machine behavior ──
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
          msg += " fltTot=" + std::to_string(dd->dyncube_filter_updates);
          msg += " ggxMips=" + std::to_string(dd->dyncube_ggx_mip_dispatches);
          msg += " faceCpy=" + std::to_string(dd->dyncube_face_copies);
          reshade::log::message(reshade::log::level::info, msg.c_str());
        }
      }
    }

    // Bind the history position cube (t29) — Dynamic validity source for the blend.
    {
      reshade::api::resource_view histPosSrv = dd->dyncube_hist[1u - dd->dyncube_hist_cur].pos_cube_srv;
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
    // Bind the SSR result (t31 blurred) + raw (t32) — produced once per frame by the scheduler.
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

    if (dbg == 3) {
      // Solid face colors — validates t17 binding + handedness without capture.
      // Uses the DEDICATED solid cube so history resources are never touched.
      if (!forceVanilla && RunDynCubeSolid(cmd_list, dd)) {
        cmd_list->push_descriptors(reshade::api::shader_stage::pixel, reshade::api::pipeline_layout{0}, 0,
          reshade::api::descriptor_table_update{{}, kDynCubeRegister, 0, 1,
            reshade::api::descriptor_type::texture_shader_resource_view, &dd->dyncube_solid_cube_srv});
      }
    } else if (overrideT17) {
      // Push the ACTIVE completed filtered cube (never a partially-written building cube).
      // Before the first filter completes, fall back to the raw history cube.
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



// ═══════════ Dynamic Cubemaps (Sora 2nd) — standalone t17 replacement ═══════════

static uint32_t DynCubeResolveSize(float v) {
  switch ((int)v) {
    case 0: return 128u;
    case 1: return 256u;
    case 2: return 512u;
    case 3: return 768u;
    default: return 1024u;
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
  // Aliases are copies of the above handles — zero them (never double-destroy).
  d->dyncube_srv = {};
  d->dyncube_uav = {};
  d->dyncube_texture = {};
  for (uint32_t i = 0; i < 2; ++i) {
    dv(d->dyncube_cam_srv[i]); dv(d->dyncube_cam_uav[i]); dr(d->dyncube_cam[i]);
  }
  // Character mask
  dv(d->dyncube_charmask_srv); dr(d->dyncube_charmask);
  if (d->dyncube_charmask_uav.handle) { dev->destroy_resource_view(d->dyncube_charmask_uav); d->dyncube_charmask_uav = {}; }
  // Phase 3 GGX
  if (d->dyncube_linear_sampler.handle) { dev->destroy_sampler(d->dyncube_linear_sampler); d->dyncube_linear_sampler = {}; }
  dv(d->dyncube_ggx_in_cube_srv); dr(d->dyncube_ggx_in);
  for (uint32_t i = 0; i < 2; ++i) {
    dv(d->dyncube_ggx_out_cube_srv[i]); dr(d->dyncube_ggx_out[i]);
    for (auto& u : d->dyncube_ggx_out_mip_uav[i]) { if (u.handle) { dev->destroy_resource_view(u); u = {}; } }
  }
  d->dyncube_ggx_valid = false;
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
  dp(d->dyncube_ssr_blur_pipeline); dl(d->dyncube_ssr_blur_layout);
  for (auto& t : d->dyncube_ssr_blur_tables) { if (t.handle) { dev->free_descriptor_table(t); t = {}; } }
  dp(d->dyncube_ggx_pipeline); dl(d->dyncube_ggx_layout);
  for (auto& t : d->dyncube_ggx_tables) { if (t.handle) { dev->free_descriptor_table(t); t = {}; } }
  d->dyncube_hist_cur = 0;
  d->dyncube_needs_reset = true;
  dp(d->dyncube_capture_pipeline); dp(d->dyncube_solid_pipeline);
  dl(d->dyncube_capture_layout); dl(d->dyncube_solid_layout);
  for (auto& t : d->dyncube_capture_tables) { if (t.handle) { dev->free_descriptor_table(t); t = {}; } }
  for (auto& t : d->dyncube_solid_tables) { if (t.handle) { dev->free_descriptor_table(t); t = {}; } }
  d->dyncube_resources_created = false;
  d->dyncube_solid_written = false;
}

static bool CreateDynCubeResources(reshade::api::device* dev, DeviceData* d, uint32_t size) {
  DestroyDynCubeResources(dev, d);
  if (size < 128u) size = 128u;
  if (size > 1024u) size = 1024u;
  d->dyncube_size = size;

  // Throttled logger helper (1/sec) — only when debug logging enabled
  auto should_log = []() -> bool {
    if (shader_injection.dynCube_debug_logging < 0.5f) return false;
    static auto last = std::chrono::steady_clock::now() - std::chrono::seconds(2);
    auto now = std::chrono::steady_clock::now();
    if (now - last < std::chrono::seconds(1)) return false;
    last = now;
    return true;
  };

  // Point clamp sampler for diagnostic — no filtering to test projection
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
    // Position (RGBA16F, rgb=scaled pos, a=validity) — array SRV/UAV + cube SRV (debug 11/12)
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
    // Contribution (R16F) — array SRV/UAV only
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
    dev->create_resource_view(d->dyncube_charmask, reshade::api::resource_usage::unordered_access,
      reshade::api::resource_view_desc(reshade::api::resource_view_type::texture_2d_array,
                                       reshade::api::format::r16g16b16a16_float, 0, 1, 0, 6),
      &d->dyncube_charmask_uav);
  }

  // ── Phase 3 GGX filtered cubes ──
  {
    // Mip count: 8 for all supported resolutions (128..1024); computed defensively.
    uint32_t mips = 1;
    while ((size >> mips) >= 1u && mips < 8u) ++mips;
    d->dyncube_mip_count = mips;

    // Input chain: RGBA16F cube, N mips, hardware GenerateMips flag (variance-reduction source).
    reshade::api::resource_desc rdi = {};
    rdi.type = reshade::api::resource_type::texture_2d;
    rdi.texture = {size, size, 6, (uint16_t)mips, reshade::api::format::r16g16b16a16_float, 1};
    rdi.heap = reshade::api::memory_heap::gpu_only;
    rdi.usage = reshade::api::resource_usage::shader_resource | reshade::api::resource_usage::unordered_access;
    rdi.flags = reshade::api::resource_flags::cube_compatible | reshade::api::resource_flags::generate_mipmaps;
    if (!dev->create_resource(rdi, nullptr, reshade::api::resource_usage::shader_resource, &d->dyncube_ggx_in)) {
      if (should_log()) reshade::log::message(reshade::log::level::error, "[DynCube] Failed to create GGX input");
      DestroyDynCubeResources(dev, d);
      return false;
    }
    dev->create_resource_view(d->dyncube_ggx_in, reshade::api::resource_usage::shader_resource,
      reshade::api::resource_view_desc(reshade::api::resource_view_type::texture_cube,
                                       reshade::api::format::r16g16b16a16_float, 0, mips, 0, 6),
      &d->dyncube_ggx_in_cube_srv);

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
  }

  // Dedicated solid-color debug cube (debug 3) — independent of history/ggx resources.
  {
    reshade::api::resource_desc rs = {};
    rs.type = reshade::api::resource_type::texture_2d;
    rs.texture = {size, size, 6, 1, reshade::api::format::r16g16b16a16_float, 1};
    rs.heap = reshade::api::memory_heap::gpu_only;
    rs.usage = reshade::api::resource_usage::shader_resource | reshade::api::resource_usage::unordered_access;
    rs.flags = reshade::api::resource_flags::cube_compatible;
    if (!dev->create_resource(rs, nullptr, reshade::api::resource_usage::shader_resource, &d->dyncube_solid_cube)) {
      if (should_log()) reshade::log::message(reshade::log::level::error, "[DynCube] Failed to create solid cube");
      DestroyDynCubeResources(dev, d);
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
  }

  d->dyncube_resources_created = true;
  d->dyncube_solid_written = false;
  if (should_log()) {
    const uint32_t mips = d->dyncube_mip_count;
    const double mips_wt = (mips >= 2) ? (4.0 / 3.0) : 1.0;
    const double base = (double)size * (double)size * 6.0;
    const double bytes = 2.0 * base * (8.0 + 8.0 + 2.0) + base * mips_wt * 8.0 * 3.0 + 2.0 * base * 8.0;
    reshade::log::message(reshade::log::level::info,
      (std::string("[DynCube] Resources created: ") + std::to_string(size) + "x" + std::to_string(size) +
       "x6, mips=" + std::to_string(mips) + ", ~" + std::to_string((long long)(bytes / (1024.0 * 1024.0))) + " MB").c_str());
  }
  return true;
}

static bool CreateDynCubePipelinesIfNeeded(reshade::api::device* dev, DeviceData* d) {
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
  // Phase 1+2: 7 SRVs (depth, color, prevColor, prevPos, prevContrib, camPrev, mrt0), 5 UAVs (curColor, curPos, curContrib, camCur, charmask), 7 push floats
  auto make_capture_layout = [&](reshade::api::pipeline_layout* out) -> bool {
    if (out->handle != 0u) return true;
    DR sampler_r = {0,0,0,1,DS::all_compute,1,DT::sampler};
    DR cbv_r     = {0,0,0,1,DS::all_compute,1,DT::constant_buffer}; // b0 cb_scene only
    DR srv_r     = {0,0,0,7,DS::all_compute,1,DT::texture_shader_resource_view}; // t0..t6 (depth, color, prevColor, prevPos, prevContrib, camPrev, mrt0)
    DR uav_r     = {0,0,0,5,DS::all_compute,1,DT::texture_unordered_access_view}; // u0..u4 (curColor, curPos, curContrib, camCur, charmask)
    reshade::api::constant_range push_range = {};
    push_range.binding = 0;
    push_range.dx_register_index = 13;
    push_range.dx_register_space = 0;
    push_range.count = 7; // boost, blend, posThreshold, posScale, reset, characterCapture, charMaskAvailable
    push_range.visibility = DS::all_compute;
    P p0, p1, p2, p3, pPush;
    p0.type = reshade::api::pipeline_layout_param_type::descriptor_table; p0.descriptor_table.count = 1; p0.descriptor_table.ranges = &sampler_r;
    p1.type = reshade::api::pipeline_layout_param_type::descriptor_table; p1.descriptor_table.count = 1; p1.descriptor_table.ranges = &cbv_r;
    p2.type = reshade::api::pipeline_layout_param_type::descriptor_table; p2.descriptor_table.count = 1; p2.descriptor_table.ranges = &srv_r;
    p3.type = reshade::api::pipeline_layout_param_type::descriptor_table; p3.descriptor_table.count = 1; p3.descriptor_table.ranges = &uav_r;
    pPush.type = reshade::api::pipeline_layout_param_type::push_constants; pPush.push_constants = push_range;
    P params[5] = {p0,p1,p2,p3,pPush};
    return dev->create_pipeline_layout(5, params, out);
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
  } else if (!__dyncube_capture.empty()) {
    if (!mkcs(__dyncube_capture, d->dyncube_capture_layout, &d->dyncube_capture_pipeline)) {
      if (pipelog_should()) reshade::log::message(reshade::log::level::warning, "[DynCube] capture pipeline create failed (dyncube_capture)");
    }
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

  // ── Phase 3 GGX prefilter pipeline ──
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

  // ── Simple SSR pipeline ──
  auto make_ssr_layout = [&](reshade::api::pipeline_layout* out) -> bool {
    if (out->handle != 0u) return true;
    DR sampler_r = {0,0,0,1,DS::all_compute,1,DT::sampler};
    DR cbv_r     = {0,0,0,1,DS::all_compute,1,DT::constant_buffer}; // b0 cb_scene
    DR srv_r     = {0,0,0,3,DS::all_compute,1,DT::texture_shader_resource_view}; // t0 color, t1 depth, t2 mrt_normal
    DR uav_r     = {0,0,0,1,DS::all_compute,1,DT::texture_unordered_access_view}; // u0 ssr_result
    reshade::api::constant_range push_range = {};
    push_range.binding = 0;
    push_range.dx_register_index = 13;
    push_range.dx_register_space = 0;
    push_range.count = 8; // sampleCount, maxDist, thickness, distanceFade, edgeFade, grazingFade, charOccStrength, charOccUpness
    push_range.visibility = DS::all_compute;
    P p0, p1, p2, p3, pPush;
    p0.type = reshade::api::pipeline_layout_param_type::descriptor_table; p0.descriptor_table.count = 1; p0.descriptor_table.ranges = &sampler_r;
    p1.type = reshade::api::pipeline_layout_param_type::descriptor_table; p1.descriptor_table.count = 1; p1.descriptor_table.ranges = &cbv_r;
    p2.type = reshade::api::pipeline_layout_param_type::descriptor_table; p2.descriptor_table.count = 1; p2.descriptor_table.ranges = &srv_r;
    p3.type = reshade::api::pipeline_layout_param_type::descriptor_table; p3.descriptor_table.count = 1; p3.descriptor_table.ranges = &uav_r;
    pPush.type = reshade::api::pipeline_layout_param_type::push_constants; pPush.push_constants = push_range;
    P params[5] = {p0,p1,p2,p3,pPush};
    return dev->create_pipeline_layout(5, params, out);
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

  // ── SSR separable blur pipeline (H then V) ──
  auto make_ssr_blur_layout = [&](reshade::api::pipeline_layout* out) -> bool {
    if (out->handle != 0u) return true;
    DR sampler_r = {0,0,0,1,DS::all_compute,1,DT::sampler};
    DR srv_r     = {0,0,0,1,DS::all_compute,1,DT::texture_shader_resource_view}; // t0 raw/blur_h
    DR uav_r     = {0,0,0,1,DS::all_compute,1,DT::texture_unordered_access_view}; // u0 blur_h/blur
    reshade::api::constant_range push_range = {};
    push_range.binding = 0;
    push_range.dx_register_index = 13;
    push_range.dx_register_space = 0;
    push_range.count = 2; // sigma, horizontal
    push_range.visibility = DS::all_compute;
    P p0, p1, p2, pPush;
    p0.type = reshade::api::pipeline_layout_param_type::descriptor_table; p0.descriptor_table.count = 1; p0.descriptor_table.ranges = &sampler_r;
    p1.type = reshade::api::pipeline_layout_param_type::descriptor_table; p1.descriptor_table.count = 1; p1.descriptor_table.ranges = &srv_r;
    p2.type = reshade::api::pipeline_layout_param_type::descriptor_table; p2.descriptor_table.count = 1; p2.descriptor_table.ranges = &uav_r;
    pPush.type = reshade::api::pipeline_layout_param_type::push_constants; pPush.push_constants = push_range;
    P params[4] = {p0,p1,p2,pPush};
    return dev->create_pipeline_layout(4, params, out);
  };
  if (!make_ssr_blur_layout(&d->dyncube_ssr_blur_layout)) return false;
  if (!ensure(d->dyncube_ssr_blur_layout, &d->dyncube_ssr_blur_tables, 3)) return false;
  #ifdef __FalcomSSRBlurCS_EMBED_FILE
  if (!__FalcomSSRBlurCS.empty()) {
    if (!mkcs(__FalcomSSRBlurCS, d->dyncube_ssr_blur_layout, &d->dyncube_ssr_blur_pipeline)) {
      if (pipelog_should()) reshade::log::message(reshade::log::level::warning, "[DynCube] SSR blur pipeline create failed");
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

// Move the current (active) DynCube resource handles out of the DeviceData members into a
// snapshot (zeroing the members). Used to cache the old set on a resize without destroying it.
static void MoveActiveSetTo(DeviceData* d, DynCubeSet& s) {
  for (uint32_t i = 0; i < 2; ++i) {
    s.hist[i].color = d->dyncube_hist[i].color; d->dyncube_hist[i].color = {};
    s.hist[i].color_cube_srv = d->dyncube_hist[i].color_cube_srv; d->dyncube_hist[i].color_cube_srv = {};
    s.hist[i].color_arr_srv = d->dyncube_hist[i].color_arr_srv; d->dyncube_hist[i].color_arr_srv = {};
    s.hist[i].color_uav = d->dyncube_hist[i].color_uav; d->dyncube_hist[i].color_uav = {};
    s.hist[i].pos = d->dyncube_hist[i].pos; d->dyncube_hist[i].pos = {};
    s.hist[i].pos_arr_srv = d->dyncube_hist[i].pos_arr_srv; d->dyncube_hist[i].pos_arr_srv = {};
    s.hist[i].pos_cube_srv = d->dyncube_hist[i].pos_cube_srv; d->dyncube_hist[i].pos_cube_srv = {};
    s.hist[i].pos_uav = d->dyncube_hist[i].pos_uav; d->dyncube_hist[i].pos_uav = {};
    s.hist[i].contrib = d->dyncube_hist[i].contrib; d->dyncube_hist[i].contrib = {};
    s.hist[i].contrib_arr_srv = d->dyncube_hist[i].contrib_arr_srv; d->dyncube_hist[i].contrib_arr_srv = {};
    s.hist[i].contrib_uav = d->dyncube_hist[i].contrib_uav; d->dyncube_hist[i].contrib_uav = {};
    s.cam[i] = d->dyncube_cam[i]; d->dyncube_cam[i] = {};
    s.cam_srv[i] = d->dyncube_cam_srv[i]; d->dyncube_cam_srv[i] = {};
    s.cam_uav[i] = d->dyncube_cam_uav[i]; d->dyncube_cam_uav[i] = {};
  }
  s.charmask = d->dyncube_charmask; d->dyncube_charmask = {};
  s.charmask_srv = d->dyncube_charmask_srv; d->dyncube_charmask_srv = {};
  s.charmask_uav = d->dyncube_charmask_uav; d->dyncube_charmask_uav = {};
  s.ggx_in = d->dyncube_ggx_in; d->dyncube_ggx_in = {};
  s.ggx_in_cube_srv = d->dyncube_ggx_in_cube_srv; d->dyncube_ggx_in_cube_srv = {};
  for (uint32_t i = 0; i < 2; ++i) {
    s.ggx_out[i] = d->dyncube_ggx_out[i]; d->dyncube_ggx_out[i] = {};
    s.ggx_out_cube_srv[i] = d->dyncube_ggx_out_cube_srv[i]; d->dyncube_ggx_out_cube_srv[i] = {};
    for (uint32_t m = 0; m < 8; ++m) {
      s.ggx_out_mip_uav[i][m] = d->dyncube_ggx_out_mip_uav[i][m]; d->dyncube_ggx_out_mip_uav[i][m] = {};
    }
  }
  s.solid_cube = d->dyncube_solid_cube; d->dyncube_solid_cube = {};
  s.solid_cube_srv = d->dyncube_solid_cube_srv; d->dyncube_solid_cube_srv = {};
  s.solid_cube_uav = d->dyncube_solid_cube_uav; d->dyncube_solid_cube_uav = {};
  s.mip_count = d->dyncube_mip_count;
}

// Reverse of MoveActiveSetTo: move a cached snapshot back into the active DeviceData members.
static void MoveSetToActive(DeviceData* d, DynCubeSet& s) {
  for (uint32_t i = 0; i < 2; ++i) {
    d->dyncube_hist[i].color = s.hist[i].color; s.hist[i].color = {};
    d->dyncube_hist[i].color_cube_srv = s.hist[i].color_cube_srv; s.hist[i].color_cube_srv = {};
    d->dyncube_hist[i].color_arr_srv = s.hist[i].color_arr_srv; s.hist[i].color_arr_srv = {};
    d->dyncube_hist[i].color_uav = s.hist[i].color_uav; s.hist[i].color_uav = {};
    d->dyncube_hist[i].pos = s.hist[i].pos; s.hist[i].pos = {};
    d->dyncube_hist[i].pos_arr_srv = s.hist[i].pos_arr_srv; s.hist[i].pos_arr_srv = {};
    d->dyncube_hist[i].pos_cube_srv = s.hist[i].pos_cube_srv; s.hist[i].pos_cube_srv = {};
    d->dyncube_hist[i].pos_uav = s.hist[i].pos_uav; s.hist[i].pos_uav = {};
    d->dyncube_hist[i].contrib = s.hist[i].contrib; s.hist[i].contrib = {};
    d->dyncube_hist[i].contrib_arr_srv = s.hist[i].contrib_arr_srv; s.hist[i].contrib_arr_srv = {};
    d->dyncube_hist[i].contrib_uav = s.hist[i].contrib_uav; s.hist[i].contrib_uav = {};
    d->dyncube_cam[i] = s.cam[i]; s.cam[i] = {};
    d->dyncube_cam_srv[i] = s.cam_srv[i]; s.cam_srv[i] = {};
    d->dyncube_cam_uav[i] = s.cam_uav[i]; s.cam_uav[i] = {};
  }
  d->dyncube_charmask = s.charmask; s.charmask = {};
  d->dyncube_charmask_srv = s.charmask_srv; s.charmask_srv = {};
  d->dyncube_charmask_uav = s.charmask_uav; s.charmask_uav = {};
  d->dyncube_ggx_in = s.ggx_in; s.ggx_in = {};
  d->dyncube_ggx_in_cube_srv = s.ggx_in_cube_srv; s.ggx_in_cube_srv = {};
  for (uint32_t i = 0; i < 2; ++i) {
    d->dyncube_ggx_out[i] = s.ggx_out[i]; s.ggx_out[i] = {};
    d->dyncube_ggx_out_cube_srv[i] = s.ggx_out_cube_srv[i]; s.ggx_out_cube_srv[i] = {};
    for (uint32_t m = 0; m < 8; ++m) {
      d->dyncube_ggx_out_mip_uav[i][m] = s.ggx_out_mip_uav[i][m]; s.ggx_out_mip_uav[i][m] = {};
    }
  }
  d->dyncube_solid_cube = s.solid_cube; s.solid_cube = {};
  d->dyncube_solid_cube_srv = s.solid_cube_srv; s.solid_cube_srv = {};
  d->dyncube_solid_cube_uav = s.solid_cube_uav; s.solid_cube_uav = {};
  d->dyncube_mip_count = s.mip_count;
  // Aliases point at the freshly-restored set A.
  d->dyncube_texture = d->dyncube_hist[0].color;
  d->dyncube_srv = d->dyncube_hist[0].color_cube_srv;
  d->dyncube_uav = d->dyncube_hist[0].color_uav;
  d->dyncube_hist_cur = 0;
  d->dyncube_needs_reset = true;
  d->dyncube_ggx_valid = false;
  d->dyncube_phase = DeviceData::DynCubePhase::Done;
  d->dyncube_next_update_frame = 0;
  d->dyncube_resources_created = true;
}

// Cache the current active set under its size (never destroys anything).
static void SaveActiveToCache(reshade::api::device* dev, DeviceData* d) {
  if (!d || !d->dyncube_resources_created || d->dyncube_size == 0u) return;
  DynCubeSet s;
  MoveActiveSetTo(d, s);
  d->dyncube_cache[d->dyncube_size] = std::move(s);
  d->dyncube_resources_created = false;
  d->dyncube_srv = {};
  d->dyncube_uav = {};
  d->dyncube_texture = {};
}

// Activate a cached set of the given size (returns false if not cached).
static bool RestoreFromCache(reshade::api::device* dev, DeviceData* d, uint32_t size) {
  auto it = d->dyncube_cache.find(size);
  if (it == d->dyncube_cache.end()) return false;
  MoveSetToActive(d, it->second);
  d->dyncube_size = size;
  d->dyncube_cache.erase(it);
  return true;
}

// Destroy every handle in a cached set snapshot.
static void DestroyDynCubeSet(reshade::api::device* dev, DynCubeSet& s) {
  if (!dev) return;
  auto dv = [&](reshade::api::resource_view& v) { if (v.handle) { dev->destroy_resource_view(v); v = {}; } };
  auto dr = [&](reshade::api::resource& r) { if (r.handle) { dev->destroy_resource(r); r = {}; } };
  for (auto& h : s.hist) {
    dv(h.color_cube_srv); dv(h.color_arr_srv); dv(h.color_uav); dr(h.color);
    dv(h.pos_arr_srv); dv(h.pos_cube_srv); dv(h.pos_uav); dr(h.pos);
    dv(h.contrib_arr_srv); dv(h.contrib_uav); dr(h.contrib);
  }
  for (uint32_t i = 0; i < 2; ++i) {
    dv(s.cam_srv[i]); dv(s.cam_uav[i]); dr(s.cam[i]);
  }
  dv(s.charmask_srv); dr(s.charmask);
  if (s.charmask_uav.handle) { dev->destroy_resource_view(s.charmask_uav); s.charmask_uav = {}; }
  dv(s.ggx_in_cube_srv); dr(s.ggx_in);
  for (uint32_t i = 0; i < 2; ++i) {
    dv(s.ggx_out_cube_srv[i]); dr(s.ggx_out[i]);
    for (auto& u : s.ggx_out_mip_uav[i]) { if (u.handle) { dev->destroy_resource_view(u); u = {}; } }
  }
  dv(s.solid_cube_srv); dr(s.solid_cube);
  if (s.solid_cube_uav.handle) { dev->destroy_resource_view(s.solid_cube_uav); s.solid_cube_uav = {}; }
}

// Destroy all cached per-size sets (used on disable / device / swapchain teardown).
static void DestroyDynCubeCache(reshade::api::device* dev, DeviceData* d) {
  if (!dev || !d) return;
  for (auto& [size, set] : d->dyncube_cache) DestroyDynCubeSet(dev, set);
  d->dyncube_cache.clear();
}

static bool RunDynCubeSolid(reshade::api::command_list* cl, DeviceData* d) {
  // Writes a DEDICATED solid-color cube — never the history/ggx resources.
  if (!cl || !d || !d->dyncube_solid_cube_uav.handle) return false;
  auto* dev = cl->get_device();
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

static bool RunDynCubeCapture(reshade::api::command_list* cl, DeviceData* d) {
  if (!cl || !d) return false;
  if (!d->dyncube_resources_created) {
    uint32_t sz = DynCubeResolveSize(shader_injection.dynCube_resolution);
    if (!CreateDynCubeResources(cl->get_device(), d, sz)) return false;
  }
  // Phase 1: require depth+color+cbv (rawDepth >=1-1e-5 reject)
  if (!d->captured_depth_srv.handle || !d->captured_color_srv.handle || !d->captured_scene_cbv_valid) return false;
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

  // Previous set as SRVs (t2 prevColor, t3 prevPos, t4 prevContrib, t5 camPrev) + depth/color (t0/t1) + mrt0 (t6)
  reshade::api::resource_view srvs[7] = {
      d->captured_depth_srv,
      d->captured_color_srv,
      d->dyncube_hist[prev].color_arr_srv,
      d->dyncube_hist[prev].pos_arr_srv,
      d->dyncube_hist[prev].contrib_arr_srv,
      d->dyncube_cam_srv[prev],
      d->captured_mrt_normal_srv,  // mrtTexture0 for character mask
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
  ups[2] = {tbl->at(2), 0, 0, 7, reshade::api::descriptor_type::texture_shader_resource_view, srvs};
  ups[3] = {tbl->at(3), 0, 0, 5, reshade::api::descriptor_type::texture_unordered_access_view, uavs};
  dev->update_descriptor_tables(4, ups);
  std::array<reshade::api::descriptor_table, 4> tables = {tbl->at(0), tbl->at(1), tbl->at(2), tbl->at(3)};
  cl->bind_descriptor_tables(reshade::api::shader_stage::all_compute, d->dyncube_capture_layout, 0, 4, tables.data());

  // Push constants (b13): boost, blend, posThreshold(world), posScale, reset, characterCapture, charMaskAvailable
  {
    const float posScale = 0.001f;
    float reset = (shader_injection.dynCube_history < 0.5f) ? 1.0f : 0.0f;
    float charCapture = (shader_injection.dynCube_character_capture > 0.5f) ? 1.0f : 0.0f;
    float charMaskAvail = (d->captured_mrt_normal_srv.handle) ? 1.0f : 0.0f;
    float pc[7] = {
        std::clamp(shader_injection.dynCube_capture_boost, 0.f, 8.f),
        std::clamp(shader_injection.dynCube_history_blend, 0.f, 1.f),
        std::max(0.f, shader_injection.dynCube_history_pos_threshold),
        posScale,
        reset,
        charCapture,
        charMaskAvail,
    };
    cl->push_constants(reshade::api::shader_stage::all_compute, d->dyncube_capture_layout, 4, 0, 7, pc);
  }

  uint32_t sz = d->dyncube_size;
  cl->dispatch((sz + 7)/8, (sz + 7)/8, 6);
  ++d->dyncube_capture_dispatches;

  // Barrier current history + camera UAVs -> SRV (t17 reads current set, preview + next-frame reads too)
  cl->barrier(d->dyncube_hist[cur].color, reshade::api::resource_usage::unordered_access, reshade::api::resource_usage::shader_resource);
  cl->barrier(d->dyncube_hist[cur].pos, reshade::api::resource_usage::unordered_access, reshade::api::resource_usage::shader_resource);
  cl->barrier(d->dyncube_hist[cur].contrib, reshade::api::resource_usage::unordered_access, reshade::api::resource_usage::shader_resource);
  cl->barrier(d->dyncube_cam[cur], reshade::api::resource_usage::unordered_access, reshade::api::resource_usage::shader_resource);

  // Swap: next frame reads the just-written set as "previous" and writes the other set.
  d->dyncube_hist_cur = prev;
  // Aliases (t17 + preview + solid) point at the freshly written set.
  d->dyncube_texture = d->dyncube_hist[cur].color;
  d->dyncube_srv = d->dyncube_hist[cur].color_cube_srv;
  d->dyncube_uav = d->dyncube_hist[cur].color_uav;
  d->dyncube_solid_written = false; // now contains captured data
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
    if (!copy_mip0(dst)) return false;
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

// Simple screen-space SSR (compute): march captured depth, write RGBA16F result
// (rgb = reflected frame color, a = hit). Independent of history/ggx/inferred.
static bool RunDynCubeSSR(reshade::api::command_list* cl, DeviceData* d) {
  if (!cl || !d) return false;
  if (!d->captured_color_srv.handle || !d->captured_depth_srv.handle
      || !d->captured_mrt_normal_srv.handle || !d->captured_scene_cbv_valid) return false;
  auto* dev = cl->get_device();
  if (!CreateDynCubePipelinesIfNeeded(dev, d)) return false;
  if (!d->dyncube_ssr_pipeline.handle) return false;

  // Lazily create the full-res SSR textures (raw, blur_h, blur) sized to the captured color.
  auto colorRes = dev->get_resource_from_view(d->captured_color_srv);
  auto cd = dev->get_resource_desc(colorRes);
  uint32_t w = cd.texture.width, h = cd.texture.height;
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

  // ── March pass → ssr_raw ──
  cl->bind_pipeline(reshade::api::pipeline_stage::all_compute, d->dyncube_ssr_pipeline);
  auto* st = &d->dyncube_ssr_tables;
  reshade::api::resource_view srvs[3] = {d->captured_color_srv, d->captured_depth_srv, d->captured_mrt_normal_srv};
  reshade::api::descriptor_table_update su[4] = {
      {st->at(0), 0, 0, 1, reshade::api::descriptor_type::sampler, &d->dyncube_sampler},
      {st->at(1), 0, 0, 1, reshade::api::descriptor_type::constant_buffer, &d->captured_scene_cbv_view},
      {st->at(2), 0, 0, 3, reshade::api::descriptor_type::texture_shader_resource_view, srvs},
      {st->at(3), 0, 0, 1, reshade::api::descriptor_type::texture_unordered_access_view, &d->dyncube_ssr_raw_uav},
  };
  dev->update_descriptor_tables(4, su);
  std::array<reshade::api::descriptor_table, 4> stables = {st->at(0), st->at(1), st->at(2), st->at(3)};
  cl->bind_descriptor_tables(reshade::api::shader_stage::all_compute, d->dyncube_ssr_layout, 0, 4, stables.data());
  // SSR Quality → (sampleCount, maxDist): Low 10/12, Medium 16/20, High 24/32.
  const float quality = shader_injection.dynCube_ssr_quality;
  uint32_t sampleCount = 16u;
  float maxDist = 20.f;
  if (quality < 0.5f) { sampleCount = 10u; maxDist = 12.f; }
  else if (quality < 1.5f) { sampleCount = 16u; maxDist = 20.f; }
  else { sampleCount = 24u; maxDist = 32.f; }
  float pc[8] = {
      (float)sampleCount,
      maxDist,
      std::clamp(shader_injection.dynCube_ssr_thickness, 0.f, 10.f),
      std::clamp(shader_injection.dynCube_ssr_distance_fade, 0.f, 1.f),
      std::clamp(shader_injection.dynCube_ssr_edge_fade, 0.f, 1.f),
      std::clamp(shader_injection.dynCube_ssr_grazing_fade, 0.f, 1.f),
      std::clamp(shader_injection.dynCube_ssr_char_occ_strength, 0.f, 1.f),
      std::clamp(shader_injection.dynCube_ssr_char_occ_upness, 0.f, 1.f),
  };
  cl->push_constants(reshade::api::shader_stage::all_compute, d->dyncube_ssr_layout, 4, 0, 8, pc);
  cl->dispatch((w + 7u) / 8u, (h + 7u) / 8u, 1);
  cl->barrier(d->dyncube_ssr_raw, reshade::api::resource_usage::unordered_access, reshade::api::resource_usage::shader_resource);

  // ── Separable blur: H (raw → blur_h), V (blur_h → blur) ──
  if (!d->dyncube_ssr_blur_pipeline.handle) return false;
  const float sigma = std::clamp(shader_injection.dynCube_ssr_blur, 0.f, 8.f);
  auto* bt = &d->dyncube_ssr_blur_tables;
  cl->bind_pipeline(reshade::api::pipeline_stage::all_compute, d->dyncube_ssr_blur_pipeline);
  // H pass
  reshade::api::descriptor_table_update bh[3] = {
      {bt->at(0), 0, 0, 1, reshade::api::descriptor_type::sampler, &d->dyncube_sampler},
      {bt->at(1), 0, 0, 1, reshade::api::descriptor_type::texture_shader_resource_view, &d->dyncube_ssr_raw_srv},
      {bt->at(2), 0, 0, 1, reshade::api::descriptor_type::texture_unordered_access_view, &d->dyncube_ssr_blur_h_uav},
  };
  dev->update_descriptor_tables(3, bh);
  std::array<reshade::api::descriptor_table, 3> btables = {bt->at(0), bt->at(1), bt->at(2)};
  cl->bind_descriptor_tables(reshade::api::shader_stage::all_compute, d->dyncube_ssr_blur_layout, 0, 3, btables.data());
  float pcH[2] = {sigma, 1.f};
  cl->push_constants(reshade::api::shader_stage::all_compute, d->dyncube_ssr_blur_layout, 3, 0, 2, pcH);
  cl->dispatch((w + 7u) / 8u, (h + 7u) / 8u, 1);
  cl->barrier(d->dyncube_ssr_blur_h, reshade::api::resource_usage::unordered_access, reshade::api::resource_usage::shader_resource);
  // V pass
  reshade::api::descriptor_table_update bv[3] = {
      {bt->at(0), 0, 0, 1, reshade::api::descriptor_type::sampler, &d->dyncube_sampler},
      {bt->at(1), 0, 0, 1, reshade::api::descriptor_type::texture_shader_resource_view, &d->dyncube_ssr_blur_h_srv},
      {bt->at(2), 0, 0, 1, reshade::api::descriptor_type::texture_unordered_access_view, &d->dyncube_ssr_blur_uav},
  };
  dev->update_descriptor_tables(3, bv);
  cl->bind_descriptor_tables(reshade::api::shader_stage::all_compute, d->dyncube_ssr_blur_layout, 0, 3, btables.data());
  float pcV[2] = {sigma, 0.f};
  cl->push_constants(reshade::api::shader_stage::all_compute, d->dyncube_ssr_blur_layout, 3, 0, 2, pcV);
  cl->dispatch((w + 7u) / 8u, (h + 7u) / 8u, 1);
  cl->barrier(d->dyncube_ssr_blur, reshade::api::resource_usage::unordered_access, reshade::api::resource_usage::shader_resource);
  UnbindDynCubeComputeState(cl);
  return true;
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
