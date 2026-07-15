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
#include "./shared.h"

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
  .shadow_base_softness = 0.2f,
  .shadow_penumbra_scale = 60.f,
  .shadow_pcss_search_radius = 1.f,
  .shadow_pcss_filter_width = 1.f,
  .shadow_pcss_depth_cap = 0.05f,
  .shadow_pcss_cascade_blend = 0.2f,
  .shadow_pcss_fix_texel_radius = 0.f,
  .shadow_pcss_fix_clamp_cascade = 0.f,
  .shadow_pcss_fix_min_radius = 0.f,
  .shadow_pcss_fix_auto_blend = 0.f,
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
  .gtvbao_exclude_foliage = 0.f,
  .gtvbao_foliage_ao_value = 1.f,
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

  // CPU optimization tracking
  uint64_t last_bound_pipeline_handle = 0u;
  uint64_t last_srv0_handle = 0u;
  uint64_t last_srv1_handle = 0u;
  uint64_t last_uav0_handle = 0u;
  uint64_t last_cbv_handle = 0u;
  uint64_t last_sampler_handle = 0u;
};

static void CreateGTVBAOResources(reshade::api::device* device, DeviceData* data,
                                   uint32_t gw, uint32_t gh);
static void DestroyGTVBAOResources(reshade::api::device* device, DeviceData* data);
static bool CreateComputePipelinesIfNeeded(reshade::api::device* device, DeviceData* data);
static bool RunGTVBAO(reshade::api::command_list* cmd_list, DeviceData* data);
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
    CustomShaderEntryCallback(0x485E0022, OnBeforeSsaoShaderDraw),
    {
        0xFDAAF80Eu,
        renodx::mods::shader::CustomShader{
            .crc32 = 0xFDAAF80Eu,
            .code = __0xFDAAF80E,
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
      .tooltip = "Master toggle for IS-FAST spatio-temporal blue noise. Requires fast_noise_ea.dds next to game .exe.",
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
      .tooltip = "Log IS-FAST texture load status and noise source.",
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
      .is_visible = []() { return IsKai(); },
    },
    new renodx::utils::settings::Setting{
      .key = "DOFStrength", .binding = &shader_injection.dof_strength,
      .default_value = 1.f, .label = "Strength", .section = "Depth of Field",
      .tooltip = "Overall blend strength for improved DOF output.",
      .min = 0.f, .max = 2.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.dof_mode >= 0.5f; },
      .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "DOFRadiusScale", .binding = &shader_injection.dof_radius_scale,
      .default_value = 1.33f, .label = "Radius Scale", .section = "Depth of Field",
      .tooltip = "Scales blur radius derived from game CoC.",
      .min = 0.25f, .max = 2.5f, .format = "%.2fx",
      .is_enabled = []() { return shader_injection.dof_mode >= 0.5f; },
      .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "DOFSampleCount", .binding = &shader_injection.dof_sample_count,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 24.f, .label = "Sample Count", .section = "Depth of Field",
      .tooltip = "Higher values produce smoother bokeh at higher cost.",
      .min = 4.f, .max = 64.f, .format = "%d",
      .is_enabled = []() { return shader_injection.dof_mode >= 0.5f; },
      .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "DOFNearScale", .binding = &shader_injection.dof_near_scale,
      .default_value = 1.f, .label = "Near Scale", .section = "Depth of Field",
      .tooltip = "Scales near-field CoC response.",
      .min = 0.f, .max = 2.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.dof_mode >= 0.5f; },
      .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "DOFFarScale", .binding = &shader_injection.dof_far_scale,
      .default_value = 1.f, .label = "Far Scale", .section = "Depth of Field",
      .tooltip = "Scales far-field CoC response.",
      .min = 0.f, .max = 2.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.dof_mode >= 0.5f; },
      .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "DOFCoCCurve", .binding = &shader_injection.dof_coc_curve,
      .default_value = 1.f, .label = "CoC Curve", .section = "Depth of Field",
      .tooltip = "Applies pow(CoC, Curve) before blur; >1 tightens focus transition.",
      .min = 0.25f, .max = 4.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.dof_mode >= 0.5f; },
      .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "DOFEdgeThreshold", .binding = &shader_injection.dof_edge_threshold,
      .default_value = 0.25f, .label = "Edge Threshold", .section = "Depth of Field",
      .tooltip = "Rejects CoC-mismatched taps to reduce foreground/background bleeding.",
      .min = 0.02f, .max = 1.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.dof_mode >= 0.5f; },
      .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
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
      .default_value = 75.f, .label = "Camera Strenght", .section = "Character Shadowing",
      .min = 0.f, .max = 100.f,
      .is_enabled = []() { return shader_injection.char_shadow_mode == 2.f && shader_injection.char_shadow_type != 1.f; },
      .parse = [](float v) { return v * 0.01f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "CharShadowWorldStrength", .binding = &shader_injection.char_shadow_world_strength,
      .default_value = 100.f, .label = "World Strenght", .section = "Character Shadowing",
      .min = 0.f, .max = 100.f,
      .is_enabled = []() { return shader_injection.char_shadow_mode == 2.f && shader_injection.char_shadow_type != 0.f; },
      .parse = [](float v) { return v * 0.01f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "CharGTVBAOMode", .binding = &shader_injection.char_gtvbao_mode,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 2.f, .label = "Allow GTVBAO", .section = "Character Shadowing",
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
      .default_value = 16.f, .label = "Sample Count", .section = "Character Shadowing",
      .min = 1.f, .max = 64.f, .format = "%d",
      .is_enabled = []() { return shader_injection.char_shadow_mode == 2.f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "CharShadowHardSamples", .binding = &shader_injection.char_shadow_hard_shadow_samples,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 4.f, .label = "Hard Samples", .section = "Character Shadowing",
      .min = 0.f, .max = 64.f, .format = "%d",
      .is_enabled = []() { return shader_injection.char_shadow_mode == 2.f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "CharShadowFadeSamples", .binding = &shader_injection.char_shadow_fade_out_samples,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 8.f, .label = "Fade Samples", .section = "Character Shadowing",
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
      .default_value = 0.35f, .label = "Light Fade Start", .section = "Character Shadowing",
      .min = 0.f, .max = 1.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.char_shadow_mode == 2.f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "CharShadowLightFadeEnd", .binding = &shader_injection.char_shadow_light_screen_fade_end,
      .default_value = 1.f, .label = "Light Fade End", .section = "Character Shadowing",
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
      .default_value = 1.f, .label = "Bend SSS", .section = "Environment Screen Space Shadows",
      .labels = {"Off", "On"},
      .is_visible = []() { return IsKai(); },
    },
    new renodx::utils::settings::Setting{
      .key = "EnvSSSStrength", .binding = &shader_injection.env_sss_strength,
      .default_value = 100.f, .label = "Strength", .section = "Environment Screen Space Shadows",
      .min = 0.f, .max = 100.f,
      .is_enabled = []() { return shader_injection.env_sss_enabled >= 0.5f; },
      .parse = [](float v) { return v * 0.01f; },
      .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "EnvSSSSampleCount", .binding = &shader_injection.env_sss_sample_count,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 32.f, .label = "Sample Count", .section = "Environment Screen Space Shadows",
      .min = 1.f, .max = 64.f, .format = "%d",
      .is_enabled = []() { return shader_injection.env_sss_enabled >= 0.5f; },
    .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "EnvSSSHardSamples", .binding = &shader_injection.env_sss_hard_shadow_samples,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 0.f, .label = "Hard Shadow Samples", .section = "Environment Screen Space Shadows",
      .tooltip = "Number of hard-contact samples at the start of the ray march. 0 = auto (Sample Count / 8). Higher = sharper contact shadows, but may miss thin occluders.",
      .min = 0.f, .max = 32.f, .format = "%d",
      .is_enabled = []() { return shader_injection.env_sss_enabled >= 0.5f; },
    .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "EnvSSSFadeSamples", .binding = &shader_injection.env_sss_fade_out_samples,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 0.f, .label = "Fade Out Samples", .section = "Environment Screen Space Shadows",
      .tooltip = "Number of fade-out samples at the end of the ray march. 0 = auto (Sample Count / 3). Higher = smoother transition from shadow to no shadow, reducing banding in soft shadows.",
      .min = 0.f, .max = 32.f, .format = "%d",
      .is_enabled = []() { return shader_injection.env_sss_enabled >= 0.5f; },
    .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "EnvSSSSurfaceThickness", .binding = &shader_injection.env_sss_surface_thickness,
      .default_value = 0.005f, .label = "Surface Thickness", .section = "Environment Screen Space Shadows",
      .min = 0.001f, .max = 0.2f, .format = "%.4f",
      .is_enabled = []() { return shader_injection.env_sss_enabled >= 0.5f; },
    .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "EnvSSSContrast", .binding = &shader_injection.env_sss_contrast,
      .default_value = 2.f, .label = "Shadow Contrast", .section = "Environment Screen Space Shadows",
      .min = 0.f, .max = 12.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.env_sss_enabled >= 0.5f; },
    .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "EnvSSSHeightEnable", .binding = &shader_injection.env_sss_height_enabled,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 1.f, .label = "Height Above Ground", .section = "Environment Screen Space Shadows",
      .labels = {"Off", "On"},
      .is_enabled = []() { return shader_injection.env_sss_enabled >= 0.5f; },
    .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "EnvSSSHeightMin", .binding = &shader_injection.env_sss_height_min,
      .default_value = 0.f, .label = "Min Height", .section = "Environment Screen Space Shadows",
      .min = 0.f, .max = 10.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.env_sss_enabled >= 0.5f && shader_injection.env_sss_height_enabled >= 0.5f; },
    .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "EnvSSSHeightMax", .binding = &shader_injection.env_sss_height_max,
      .default_value = 1.f, .label = "Ground Search", .section = "Environment Screen Space Shadows",
      .min = 1.f, .max = 200.f, .format = "%.0f",
      .is_enabled = []() { return shader_injection.env_sss_enabled >= 0.5f && shader_injection.env_sss_height_enabled >= 0.5f; },
    .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "EnvSSSHeightFade", .binding = &shader_injection.env_sss_height_fade,
      .default_value = 0.10f, .label = "Height Fade", .section = "Environment Screen Space Shadows",
      .min = 0.f, .max = 5.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.env_sss_enabled >= 0.5f && shader_injection.env_sss_height_enabled >= 0.5f; },
    .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "EnvSSSVerticalReject", .binding = &shader_injection.env_sss_vertical_reject,
      .default_value = 0.30f, .label = "Vertical Reject", .section = "Environment Screen Space Shadows",
      .min = 0.f, .max = 1.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.env_sss_enabled >= 0.5f; },
    .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "EnvSSSMaxDarkening", .binding = &shader_injection.env_sss_max_darkening,
      .default_value = 0.40f, .label = "Max Darkening", .section = "Environment Screen Space Shadows",
      .min = 0.f, .max = 1.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.env_sss_enabled >= 0.5f; },
    .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "EnvSSBrightRejectThreshold", .binding = &shader_injection.env_sss_bright_reject_threshold,
      .default_value = 0.5f, .label = "Brightness Reject", .section = "Environment Screen Space Shadows",
      .min = 0.f, .max = 5.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.env_sss_enabled >= 0.5f; },
    .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "EnvSSBrightRejectFade", .binding = &shader_injection.env_sss_bright_reject_fade,
      .default_value = 0.5f, .label = "Brightness Fade", .section = "Environment Screen Space Shadows",
      .min = 0.01f, .max = 3.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.env_sss_enabled >= 0.5f; },
    .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "EnvSSSCSMGate", .binding = &shader_injection.env_sss_csm_gate,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 1.f, .label = "CSM Indoor Gate", .section = "Environment Screen Space Shadows",
      .tooltip = "Skip screen-space shadows on pixels already in deep CSM shadow (prevents false shadows inside buildings).",
      .labels = {"Off", "On"},
      .is_enabled = []() { return shader_injection.env_sss_enabled >= 0.5f; },
      .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "DebugShowEnvSSS", .binding = &shader_injection.debug_show_env_sss,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 0.f, .label = "Env SSS Debug View", .section = "Environment Screen Space Shadows",
      .labels = {"Off", "SSS Mask", "Shadow Value"},
    .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
    },
    // —— Local Screen Space Shadows ——
    new renodx::utils::settings::Setting{
      .key = "LocalSSSEnable", .binding = &shader_injection.local_sss_enabled,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 0.f, .label = "Local SSS", .section = "Local Screen Space Shadows",
      .tooltip = "Enable Bend_SSS screen-space ray-march shadows for point/spot lights.",
      .labels = {"Off", "On"},
      .is_visible = []() { return IsKai() && IsAdvancedSettingsMode(); },
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
      .default_value = 18.f, .label = "Sample Count", .section = "Local Screen Space Shadows",
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
      .default_value = 0.0250f, .label = "Surface Thickness", .section = "Local Screen Space Shadows",
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
      .default_value = 0.65f, .label = "Light Fade Start", .section = "Local Screen Space Shadows",
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
      .default_value = 2.f, .label = "Quality Level", .section = "GTVBAO",
      .labels = {"Low", "Medium", "High", "Ultra"},
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "GTVBAODenoisePasses", .binding = &shader_injection.gtvbao_denoise_passes,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 1.f, .label = "Denoise Passes", .section = "GTVBAO",
      .labels = {"Off", "Sharp (1)", "Medium (2)", "Soft (3)"},
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f; },
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
      .tooltip = "IS-FAST = pre-computed blue noise (needs fast_noise_ea.dds). "
                 "IGN = Interleaved Gradient Noise. Hilbert = Hilbert curve noise. "
                 "Only applies when IS-FAST master toggle is On; forced to Hilbert when Off.",
      .labels = {"IS-FAST", "IGN", "Hilbert"},
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && g_isfast_enabled > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "GTVBAORadius", .binding = &shader_injection.gtvbao_radius,
      .default_value = 0.5f, .label = "Radius", .section = "GTVBAO",
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
      .default_value = 2.2f, .label = "Final Power", .section = "GTVBAO",
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
      .default_value = 2.0f, .label = "Depth MIP Offset", .section = "GTVBAO",
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
      .default_value = 0.65f, .label = "Normal Depth Blend", .section = "GTVBAO",
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
      .default_value = 0.4f, .label = "Normal Max Darkening", .section = "GTVBAO",
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
      .default_value = 0.f, .label = "Normal Transform Mode", .section = "GTVBAO",
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
                 "6:BitmaskHeat", "7:SectorCount", "8:1stSliceBits"},
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
      .default_value = 0.f, .label = "Exclude Foliage", .section = "GTVBAO",
      .tooltip = "Skip AO computation on foliage pixels (prevent wind disocclusion noise).",
      .labels = {"Off", "On"},
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f; },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "GTVBAOFoliageAOValue", .binding = &shader_injection.gtvbao_foliage_ao_value,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 1.f, .label = "Foliage AO Value", .section = "GTVBAO",
      .tooltip = "AO value assigned to excluded foliage pixels. 1.0 = no occlusion, 0.0 = fully occluded.",
      .min = 0.f, .max = 1.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.gtvbao_mode > 0.5f && shader_injection.gtvbao_exclude_foliage > 0.5f; },
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
      .default_value = 0.33f, .label = "Specular Blend", .section = "BRDF Improvement",
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
      .default_value = 5.0f, .label = "Multi-Bounce Strength", .section = "VBGI",
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
      .default_value = 1.f, .label = "Reduce AO with GI", .section = "VBGI",
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
      .default_value = 1.f, .label = "Multi-Bounce Frame Skip", .section = "VBGI",
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
    // —— CPU Optimizations ——
    new renodx::utils::settings::Setting{
      .key = "CPUOptDeferredDispatch", .binding = &g_cpuopt_deferred_dispatch,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 0.f, .label = "Deferred Dispatch", .section = "CPU Opt",
      .tooltip = "Move GTVBAO/VBGI dispatch to OnPresent (1-frame latency). Kai-only, default ON — avoids CS binding contamination.",
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
      .default_value = 1.f, .label = "Shadow Filter Method", .section = "Shadow Maps",
      .tooltip = "CSM filtering: Off = single sample. Falcom = vanilla 10-tap PCF. PCSS = physically-accurate soft shadows.",
      .labels = {"Off", "Falcom", "PCSS"},
      .is_visible = []() { return !IsKai(); },
    },
    new renodx::utils::settings::Setting{
      .key = "ShadowEdgeTint", .binding = &shader_injection.shadow_edge_tint,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 2.f, .label = "Colored Shadow Penumbra", .section = "Shadow Maps",
      .tooltip = "Off = neutral edges. Falcom = vanilla red tint. Improved = PCSS vibrancy boost in penumbra.",
      .labels = {"Off", "Falcom", "Improved"},
      .is_visible = []() { return !IsKai(); },
    },
    // —— PCSS Settings (enabled when ShadowFilterMethod = PCSS) ——
    new renodx::utils::settings::Setting{
      .key = "ShadowPCSSJitter", .binding = &shader_injection.shadow_pcss_jitter_enabled,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 1.f, .label = "PCSS Jitter", .section = "Shadow Maps",
      .tooltip = "Use IS-FAST spatio-temporal noise to rotate PCSS sample pattern each frame.",
      .labels = {"Off", "On"},
      .is_enabled = []() { return shader_injection.shadow_filter_method > 1.5f; },
    .is_visible = []() { return !IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "ShadowPCSSJitterAmount", .binding = &shader_injection.shadow_pcss_jitter_amount,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 1.f, .label = "Jitter Amount", .section = "Shadow Maps",
      .tooltip = "0 = static Poisson, 1 = full temporal rotation.",
      .min = 0.0f, .max = 1.0f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.shadow_filter_method > 1.5f && shader_injection.shadow_pcss_jitter_enabled > 0.5f; },
    .is_visible = []() { return !IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "ShadowPCSSJitterSpeed", .binding = &shader_injection.shadow_pcss_jitter_speed,
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
      .default_value = 0.05f, .label = "Base Softness", .section = "Shadow Maps",
      .tooltip = "Constant minimum penumbra width. Contact-hard at 0, always soft at 0.5.",
      .min = 0.0f, .max = 1.0f, .format = "%.3f",
      .is_enabled = []() { return shader_injection.shadow_filter_method > 1.5f; },
    .is_visible = []() { return !IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "ShadowPenumbraScale", .binding = &shader_injection.shadow_penumbra_scale,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 30.f, .label = "Penumbra Scale", .section = "Shadow Maps",
      .tooltip = "How fast penumbra widens with occluder distance. Higher = softer distant shadows.",
      .min = 1.0f, .max = 100.0f, .format = "%.1f",
      .is_enabled = []() { return shader_injection.shadow_filter_method > 1.5f; },
    .is_visible = []() { return !IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "ShadowPCSSSearchRadius", .binding = &shader_injection.shadow_pcss_search_radius,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 1.f, .label = "World Softness", .section = "Shadow Maps",
      .tooltip = "Desired softness in world units. Same value = same penumbra width across all cascades. 0.1=sharp, 5=very soft.",
      .min = 0.1f, .max = 2.0f, .format = "%.1f",
      .is_enabled = []() { return shader_injection.shadow_filter_method > 1.5f; },
    .is_visible = []() { return !IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "ShadowPCSSFilterWidth", .binding = &shader_injection.shadow_pcss_filter_width,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 3.f, .label = "Filter Width", .section = "Shadow Maps",
      .tooltip = "PCF filter width multiplier. Lower = sharper, higher = blurrier.",
      .min = 0.1f, .max = 5.0f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.shadow_filter_method > 1.5f; },
    .is_visible = []() { return !IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "ShadowPCSSDepthCap", .binding = &shader_injection.shadow_pcss_depth_cap,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.012f, .label = "Depth Sensitivity", .section = "Shadow Maps",
      .tooltip = "Max depth difference for penumbra. Higher = more distance-based softening.",
      .min = 0.01f, .max = 1.0f, .format = "%.3f",
      .is_enabled = []() { return shader_injection.shadow_filter_method > 1.5f; },
    .is_visible = []() { return !IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "ShadowPCSScascadeBlend", .binding = &shader_injection.shadow_pcss_cascade_blend,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.30f, .label = "Cascade Blend", .section = "Shadow Maps",
      .tooltip = "Cross-fade width between cascades. Lower = wider/smoother blend. 0.02 = 50 units, 1.0 = 1 unit.",
      .min = 0.02f, .max = 1.0f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.shadow_filter_method > 1.5f; },
    .is_visible = []() { return !IsKai() && IsAdvancedSettingsMode(); },
    },
    // —— PCSS Experimental Fixes (A/B test, all off = default behavior) ——
    new renodx::utils::settings::Setting{
      .key = "ShadowPCSSFixTexelRadius", .binding = &shader_injection.shadow_pcss_fix_texel_radius,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 0.f, .label = "PCSS Fix A: Texel-Based Softness", .section = "Shadow Maps",
      .tooltip = "Override world-space filter with texel-based radius. Consistent softness across all quality levels.",
      .labels = {"Off", "On"},
      .is_enabled = []() { return shader_injection.shadow_filter_method > 1.5f; },
    .is_visible = []() { return !IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "ShadowPCSSFixClampCascade", .binding = &shader_injection.shadow_pcss_fix_clamp_cascade,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.f, .label = "PCSS Fix B: Clamp Cascade Size", .section = "Shadow Maps",
      .tooltip = "Cap the cascade world size. Prevents Ultra's extended cascades from collapsing the filter. 0=off.",
      .min = 0.f, .max = 500.0f, .format = "%.0f",
      .is_enabled = []() { return shader_injection.shadow_filter_method > 1.5f; },
    .is_visible = []() { return !IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "ShadowPCSSFixMinRadius", .binding = &shader_injection.shadow_pcss_fix_min_radius,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 5.0f, .label = "PCSS Fix C: Minimum Filter Radius", .section = "Shadow Maps",
      .tooltip = "Guaranteed minimum PCF filter radius in shadow map texels. Prevents filter from collapsing. 0=off.",
      .min = 0.f, .max = 100.0f, .format = "%.0f",
      .is_enabled = []() { return shader_injection.shadow_filter_method > 1.5f; },
    .is_visible = []() { return !IsKai() && IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "ShadowPCSSFixAutoBlend", .binding = &shader_injection.shadow_pcss_fix_auto_blend,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 0.f, .label = "PCSS Fix D: Auto-Scale Blend", .section = "Shadow Maps",
      .tooltip = "Scale cascade blend by split distance. Larger cascades get tighter blends. Off = static blend value.",
      .labels = {"Off", "On"},
      .is_enabled = []() { return shader_injection.shadow_filter_method > 1.5f; },
    .is_visible = []() { return !IsKai() && IsAdvancedSettingsMode(); },
    },
    // —— Colored Shadow Penumbra (Improved mode, PCSS-only) ——
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
        .label = "Ultra Shadows are recommended for PCSS. High is minimum.",
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
    return;
  }
  DestroyGTVBAOResources(sc->get_device(), d);
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

  if (shader_injection.gtvbao_mode < 0.5f) return;
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

  bool ok = RunGTVBAO(cl, d);

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

  if (shader_injection.gtvbao_mode < 0.5f) return true;
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
  if (g_cpuopt_deferred_dispatch < 0.5f) {
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
  // Effective dpc = max(1, setting) — matches forced denoise in RunGTVBAO.
  int edpc = (int)shader_injection.gtvbao_denoise_passes;
  if (edpc < 1) edpc = 1;
  reshade::api::resource_view srv = (edpc & 1)
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

  // ── SSGI push t23 (GI is produced by RunGTVBAO) ──
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
  mk(w, h, reshade::api::format::r8g8b8a8_unorm,
     &d->debug_texture, &d->debug_srv, &d->debug_uav);
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
  dl(d->prefilter_layout); dl(d->main_layout); dl(d->denoise_layout);
  DestroyGTVBAODescriptorTables(dev, &d->prefilter_tables);
  DestroyGTVBAODescriptorTables(dev, &d->main_tables);
  DestroyGTVBAODescriptorTables(dev, &d->denoise_tables);
  // GI resources (now integrated — no separate VBGI pipeline)
  dv(d->vbgi_output_srv); dv(d->vbgi_output_uav); dr(d->vbgi_output_texture);
  dv(d->vbgi_denoised_srv); dv(d->vbgi_denoised_uav); dr(d->vbgi_denoised_texture);
  dv(d->captured_light_buffer_srv); dr(d->captured_light_buffer_texture);
  dv(d->multibounce_srv); dv(d->multibounce_uav); dr(d->multibounce_texture);
  dv(d->debug_srv); dv(d->debug_uav); dr(d->debug_texture);
  dp(d->multibounce_pipeline); dl(d->multibounce_layout);
  DestroyGTVBAODescriptorTables(dev, &d->multibounce_tables);
  // IS-FAST noise
  dv(d->isfast_noise_srv); dr(d->isfast_noise_texture);
  if (d->isfast_sampler.handle) { dev->destroy_sampler(d->isfast_sampler); d->isfast_sampler = {}; }
  d->isfast_texture_loaded = false;
  d->isfast_texture_attempted = false;
  // Do NOT clear captured_depth_srv / captured_scene_cbv —
  // those reference game-owned resources that survive recreation.
  d->resources_created = false;
}

// ── Push constants builder (kai-vanillaplus style) ──

static std::array<float, 61> BuildGTVBAOPushConstants(DeviceData* data, bool denoise_last_pass,
                                                       float ssgi_enabled_override = -1.f) {
  std::array<float, 61> c = {};
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
  dp(d->prefilter_pipeline); dp(d->main_low_pipeline); dp(d->main_medium_pipeline);
  dp(d->main_high_pipeline); dp(d->main_ultra_pipeline); dp(d->denoise_pipeline);
  dp(d->denoise_last_pipeline);
  dp(d->denoise_last_kai_pipeline);
  if (g_cpuopt_ensure_pipelines < 0.5f) {
    DestroyGTVBAODescriptorTables(dev, &d->prefilter_tables);
    DestroyGTVBAODescriptorTables(dev, &d->main_tables);
    DestroyGTVBAODescriptorTables(dev, &d->denoise_tables);
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
  // Main: 4 SRVs (depth MIPs, MRT normal, light buffer, IS-FAST noise) + 4 UAVs (AO, edges, GI, debug)
  if (!make_layout(4u, 4u, &d->main_layout)) return false;
  // Denoise: 6 SRVs (AO, edges, raw GI, history AO, depth mip, MRT normal) + 3 UAVs (denoised AO, denoised GI, history AO)
  if (!make_layout(6u, 3u, &d->denoise_layout)) return false;
  // Multi-bounce accumulate: 2 SRVs (color, previous GI) + 1 UAV (accumulated)
  if (!make_layout(2u, 1u, &d->multibounce_layout)) return false;

  if (!d->prefilter_pipeline.handle) mkcs(__gtvbao_prefilter, "main", d->prefilter_layout, &d->prefilter_pipeline);
  if (!d->main_low_pipeline.handle)      mkcs(__gtvbao_main_low, "main", d->main_layout, &d->main_low_pipeline);
  if (!d->main_medium_pipeline.handle)   mkcs(__gtvbao_main_medium, "main", d->main_layout, &d->main_medium_pipeline);
  if (!d->main_high_pipeline.handle)     mkcs(__gtvbao_main_high, "main", d->main_layout, &d->main_high_pipeline);
  if (!d->main_ultra_pipeline.handle)    mkcs(__gtvbao_main_ultra, "main", d->main_layout, &d->main_ultra_pipeline);
  if (!d->denoise_pipeline.handle)       mkcs(__gtvbao_denoise_pass, "main", d->denoise_layout, &d->denoise_pipeline);
  if (!d->denoise_last_pipeline.handle)  mkcs(__gtvbao_denoise_last, "main", d->denoise_layout, &d->denoise_last_pipeline);
  // Kai variant: same layout, different CSO with correct prevViewProj_g at c85
  if (!d->denoise_last_kai_pipeline.handle) mkcs(__gtvbao_denoise_last_kai, "main", d->denoise_layout, &d->denoise_last_kai_pipeline);
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

  // Build path: <exe_dir>/fast_noise_ea.dds
  char exePath[MAX_PATH];
  GetModuleFileNameA(nullptr, exePath, MAX_PATH);
  std::string ddsPath(exePath);
  ddsPath = ddsPath.substr(0, ddsPath.find_last_of("\\/") + 1) + "fast_noise_ea.dds";

  if (g_isfast_debug_logging > 0.5f)
    reshade::log::message(reshade::log::level::info,
      (std::string("[IS-FAST] Searching: ") + ddsPath).c_str());

  FILE* f = nullptr;
  if (fopen_s(&f, ddsPath.c_str(), "rb") != 0 || !f) {
    if (g_isfast_debug_logging > 0.5f)
      reshade::log::message(reshade::log::level::warning,
        "[IS-FAST] fast_noise_ea.dds not found — using IGN fallback.");
    return false;
  }

  // Read magic
  uint32_t magic = 0;
  fread(&magic, 4, 1, f);
  if (magic != 0x20534444) { fclose(f); return false; } // "DDS "

  DDS_HEADER hdr = {};
  fread(&hdr, sizeof(hdr), 1, f);

  uint32_t w = hdr.dwWidth, h = hdr.dwHeight, ddsDepth = hdr.dwDepth;
  uint32_t fmt = 0;
  bool isDX10 = (hdr.ddspf.dwFourCC == 0x30315844); // "DX10"

  if (isDX10) {
    DDS_HEADER_DXT10 dx10 = {};
    fread(&dx10, sizeof(dx10), 1, f);
    fmt = dx10.dxgiFormat;
    if (dx10.resourceDimension != 4) { fclose(f); return false; } // must be Texture3D
  }

  // DXGI_FORMAT_R8G8_UNORM = 49, expected dims: 128×128×32
  if (w != 128 || h != 128 || ddsDepth != 32 || fmt != 49) {
    if (g_isfast_debug_logging > 0.5f) {
      std::string msg = "[IS-FAST] Unexpected DDS: ";
      msg += std::to_string(w) + "x" + std::to_string(h) + "x" + std::to_string(ddsDepth);
      msg += " fmt=" + std::to_string(fmt) + " (expected 128x128x32 RG8_UNORM) — using IGN fallback.";
      reshade::log::message(reshade::log::level::warning, msg.c_str());
    }
    fclose(f);
    return false;
  }

  // Allocate buffer: 128×128×32 × 2 bytes = 1,048,576 bytes
  size_t dataSize = (size_t)w * h * ddsDepth * 2;
  std::vector<uint8_t> data(dataSize);
  fread(data.data(), 1, dataSize, f);
  fclose(f);

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
      "[IS-FAST] Texture loaded: 128x128x32 RG8_UNORM — noise source: TEXTURE");
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

  uint32_t w = d->working_width, h = d->working_height;
  if (w < 64 || h < 64) return false;

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
    cl->push_constants(CS, d->prefilter_layout, kGtvbaoPushConstantsLayoutParam, 0, 61, pc.data());
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
    reshade::api::resource_view main_srvs[4] = {
        d->depth_mips_srv,
        d->captured_mrt_normal_srv.handle ? d->captured_mrt_normal_srv : d->fallback_srv,
        light_buf,
        d->isfast_noise_srv.handle ? d->isfast_noise_srv : d->fallback_srv  // t3 IS-FAST noise
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
      {{},0,0,4,reshade::api::descriptor_type::texture_shader_resource_view,main_srvs},
      {{},0,0,4,reshade::api::descriptor_type::texture_unordered_access_view,main_uavs},
    };
    apply_descriptors(d->main_layout, &d->main_tables, 4, u);
    auto pc = BuildGTVBAOPushConstants(d, false, ssgi_enabled_this_frame);
    cl->push_constants(CS, d->main_layout, kGtvbaoPushConstantsLayoutParam, 0, 61, pc.data());
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
    bool use_a = true;
    for (int p = 0; p < dpc; ++p) {
      bool last = (p == dpc - 1);
      reshade::api::resource_view src, dst_uav;
      reshade::api::resource dst_tex;
      if (use_a) { src = d->ao_term_a_srv; dst_uav = d->ao_term_b_uav; dst_tex = d->ao_term_b_texture; }
      else       { src = d->ao_term_b_srv; dst_uav = d->ao_term_a_uav; dst_tex = d->ao_term_a_texture; }
      auto& last_pipe = IsKai() ? d->denoise_last_kai_pipeline : d->denoise_last_pipeline;
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
      auto pc = BuildGTVBAOPushConstants(d, last);
      cl->push_constants(CS, d->denoise_layout, kGtvbaoPushConstantsLayoutParam, 0, 61, pc.data());
      cl->dispatch((w + 7) / 8, (h + 7) / 8, 1);
      bar(dst_tex, UA, SR);
      use_a = !use_a;
      if (last) { d->history_ao_read_from_a = !d->history_ao_read_from_a; }  // only final pass writes history
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
