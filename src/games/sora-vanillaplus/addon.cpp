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
  .debug_show_env_sss = 0.f,
  .xegtao_mode = 1.f,
  .xegtao_quality_level = 2.f,
  .xegtao_denoise_passes = 1.f,
  .xegtao_radius = 0.5f,
  .xegtao_falloff_range = 0.615f,
  .xegtao_radius_multiplier = 1.5f,
  .xegtao_final_power = 2.0f,
  .xegtao_sample_distribution = 1.5f,
  .xegtao_bitmask_thickness = 0.2f,
  .xegtao_depth_mip_offset = 3.30f,
  .xegtao_denoise_blur_beta = 20.0f,
  .xegtao_internal_resolution = 100.f,
  .xegtao_debug_view = 0.f,
  .xegtao_debug_logging = 0.f,
  .xegtao_dedicated_bound = 0.f,
  .xegtao_fix_experimental = 0.f,
  .xegtao_ssgi_bound = 0.f,
  .xegtao_ssgi_debug = 0.f,
  .ssgi_enabled = 0.f,
  .ssgi_intensity = 1.0f,
  .ssgi_saturation = 1.0f,
  .ssgi_char_mask_strength = 0.f,
  .ssgi_multibounce = 0.f,
  .ssgi_multibounce_strength = 1.f,
  .ssgi_multibounce_saturation = 1.f,
  .ssgi_adaptive_r = 0.f,
  .ssgi_adaptive_g = 0.f,
  .ssgi_adaptive_b = 0.f,
  .ssgi_adaptive_mode = 0.f,
  .ssgi_adaptive_luma_strength = 0.f,
  .ssgi_adaptive_luma_blend = 0.5f,
  .ssgi_gi_power = 1.5f,
  .ssgi_debug_logging = 0.f,
  .ssgi_debug_view = 0.f,
  .ssgi_affect_lights = 0.f,
  .ssgi_lights_strength = 1.f,
  .ssgi_lights_saturation = 1.f,
  .shadow_filter_method = 1.f,
  .shadow_edge_tint = 1.f,
  .shadow_pcss_jitter_enabled = 1.f,
  .shadow_pcss_jitter_amount = 1.f,
  .shadow_pcss_jitter_speed = 237.f,
  .shadow_base_softness = 0.2f,
  .shadow_penumbra_scale = 60.f,
  .shadow_pcss_search_radius = 1.f,
  .shadow_pcss_filter_width = 1.f,
  .shadow_pcss_depth_cap = 0.05f,
  .shadow_pcss_cascade_blend = 0.2f,
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
};

// ═══════════ XeGTAO Backend — constants, types, fwd decls ═══════════

constexpr uint32_t kLightingXeGtaoRegister = 22u;
constexpr uint32_t kLightingSsgiRegister   = 23u;  // t23 = ssgiTexture
constexpr uint32_t kLightingDepthRegister = 4u;   // t4 = depthTexture
constexpr uint32_t kLightingSsaoRegister = 5u;    // t5 = ssaoTexture
constexpr uint32_t kLightingSceneCbRegister = 0u; // b0 = cb_scene
constexpr uint32_t kXeGTAODepthMipLevels = 5u;
constexpr uint32_t kXeGtaoDescriptorTableParamCount = 4u;  // sampler, cbv, srv, uav
constexpr uint32_t kXeGtaoPushConstantsLayoutParam = 4u;   // push_constants at b13
constexpr uint32_t kLightingMrtNormalRegister = 1u;  // t1 = mrtTexture0 (g-buffer normals)
constexpr uint64_t kXeGTAOStartupGuardFrames = 8u;
constexpr uint64_t kXeGTAOResizeGuardFrames = 4u;
constexpr uint64_t kSceneCbMinimumBytes = 95u * 16u;

// ── XeGTAO normal tuning globals (separate from ShaderInjectData) ──
static float g_xegtao_normal_input_mode     = 1.f;
static float g_xegtao_normal_influence      = 1.f;
static float g_xegtao_normal_z_preservation = 1.f;
static float g_xegtao_normal_depth_blend    = 0.70f;
static float g_xegtao_normal_sharpness      = 0.75f;
static float g_xegtao_normal_edge_rejection = 0.5f;
static float g_xegtao_normal_detail_response = 0.75f;
static float g_xegtao_normal_max_darkening  = 0.50f;
static float g_xegtao_normal_darkening_mode = 0.f;
static float g_xegtao_normal_transform_mode = 0.f; // 0=view_g, 1=viewInv_g, 2=passthrough

// ── SSGI globals removed — now controlled via ShaderInjectData fields (shared.h). ──
// ssgi_enabled, ssgi_intensity, ssgi_saturation, ssgi_multibounce, ssgi_gi_power
// are all part of shader_injection and pushed via BuildXeGTAOPushConstants.
static float g_ssgi_light_exposure = 0.05f;  // HDR light buffer exposure scale (lower = dimmer GI)

// ── IS-FAST noise ──
static float g_isfast_enabled       = 0.f;
static float g_isfast_strength      = 1.f;
static float g_isfast_debug_logging = 0.f;
static float g_isfast_spatial_scale = 1.f;
static float g_isfast_temporal_speed = 1.f;
static float g_isfast_seed_offset   = 0.f;

// ── CPU optimization toggles ──
static float g_xegtao_frame_skip         = 0.f;  // per-component frame skip (0=off, 1=every 2nd, …)
static float g_ssgi_frame_skip           = 0.f;
static float g_multibounce_frame_skip    = 0.f;
static float g_cpuopt_deferred_dispatch   = 0.f;  // dispatch XeGTAO/SSGI in OnPresent, not inline
static float g_cpuopt_ensure_pipelines    = 0.f;  // kai-style: don't destroy/recreate pipelines every frame
static float g_xegtao_jitter_toggle       = 0.f;  // enable jitter even when denoise is off

using XeGTAODescriptorTableSet =
    std::array<reshade::api::descriptor_table, kXeGtaoDescriptorTableParamCount>;

struct __declspec(uuid("b1a2c3d4-e5f6-7890-abcd-ef1234567890")) DeviceData {
  uint32_t working_width = 0u;
  uint32_t working_height = 0u;

  reshade::api::resource depth_mips_texture = {};
  reshade::api::resource_view depth_mips_srv = {};
  std::array<reshade::api::resource_view, kXeGTAODepthMipLevels> depth_mips_uavs = {};

  reshade::api::resource ao_term_a_texture = {};
  reshade::api::resource_view ao_term_a_srv = {};
  reshade::api::resource_view ao_term_a_uav = {};
  reshade::api::resource ao_term_b_texture = {};
  reshade::api::resource_view ao_term_b_srv = {};
  reshade::api::resource_view ao_term_b_uav = {};

  reshade::api::resource edges_texture = {};
  reshade::api::resource_view edges_srv = {};
  reshade::api::resource_view edges_uav = {};

  reshade::api::resource composite_texture = {};
  reshade::api::resource_view composite_srv = {};
  reshade::api::resource_view composite_uav = {};

  // 1×1 white fallback — always valid, returned when XeGTAO is off / not ready.
  reshade::api::resource fallback_texture = {};
  reshade::api::resource_view fallback_srv = {};

  reshade::api::sampler point_clamp_sampler = {};

  // Resolution-change guard.
  uint32_t last_created_game_width = 0u;
  uint32_t last_created_game_height = 0u;
  float last_created_resolution_scale = 0.f;

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

  // Descriptor tables — pre-allocated per pass.
  XeGTAODescriptorTableSet prefilter_tables = {};
  XeGTAODescriptorTableSet main_tables = {};
  XeGTAODescriptorTableSet denoise_tables = {};

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

  // ── GI resources (now integrated — no separate SSGI pipeline) ──
  reshade::api::resource ssgi_output_texture = {};
  reshade::api::resource_view ssgi_output_srv = {};
  reshade::api::resource_view ssgi_output_uav = {};
  reshade::api::resource ssgi_denoised_texture = {};
  reshade::api::resource_view ssgi_denoised_srv = {};
  reshade::api::resource_view ssgi_denoised_uav = {};
  reshade::api::resource captured_light_buffer_texture = {};
  reshade::api::resource_view captured_light_buffer_srv = {};
  bool captured_light_buffer_valid = false;   // true after first frame's capture
  // ── Multi-bounce accumulation (HDR light buffer + previous GI) ──
  reshade::api::resource multibounce_texture = {};
  reshade::api::resource_view multibounce_srv = {};
  reshade::api::resource_view multibounce_uav = {};
  reshade::api::pipeline multibounce_pipeline = {};
  reshade::api::pipeline_layout multibounce_layout = {};
  XeGTAODescriptorTableSet multibounce_tables = {};
  bool ssgi_denoised_valid = false;            // true after first denoise completes
  reshade::api::resource_view fallback_uav = {};  // 1x1 UAV fallback
  // ── Debug UAV (bitmask debug views 6-8) ──
  reshade::api::resource debug_texture = {};
  reshade::api::resource_view debug_srv = {};
  reshade::api::resource_view debug_uav = {};
  // ── IS-FAST noise ──
  reshade::api::resource isfast_noise_texture = {};
  reshade::api::resource_view isfast_noise_srv = {};
  bool isfast_texture_loaded = false;
  bool isfast_texture_attempted = false;  // only try DDS load once
  bool ssgi_bound = false;

  // CPU optimization tracking
  uint64_t last_bound_pipeline_handle = 0u;
  uint64_t last_srv0_handle = 0u;
  uint64_t last_srv1_handle = 0u;
  uint64_t last_uav0_handle = 0u;
  uint64_t last_cbv_handle = 0u;
  uint64_t last_sampler_handle = 0u;
};

static void CreateXeGTAOResources(reshade::api::device* device, DeviceData* data,
                                   uint32_t gw, uint32_t gh);
static void DestroyXeGTAOResources(reshade::api::device* device, DeviceData* data);
static bool CreateComputePipelinesIfNeeded(reshade::api::device* device, DeviceData* data);
static bool RunXeGTAO(reshade::api::command_list* cmd_list, DeviceData* data);
// SSGI is now integrated into XeGTAO main pass — no separate RunSSGI needed.
static bool OnBeforeLightingShaderDraw(reshade::api::command_list* cmd_list);
static bool OnBeforeSsaoShaderDraw(reshade::api::command_list* cmd_list);
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

// ═══════════ Custom shaders ═══════════

renodx::mods::shader::CustomShaders custom_shaders = {
    CustomShaderEntry(0x954D3D6D),
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
            // No .views — t22 is pushed at draw time via push_descriptors.
            // This avoids pipeline layout injection issues (kai-vanillaplus approach).
        },
    },
};

// ═══════════ Settings ═══════════

renodx::utils::settings::Settings settings = {
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
    },
    new renodx::utils::settings::Setting{
      .key = "CharShadowCameraStrength", .binding = &shader_injection.char_shadow_camera_strength,
      .default_value = 75.f, .label = "Camera Strenght", .section = "Character Shadowing",
      .min = 0.f, .max = 100.f,
      .is_enabled = []() { return shader_injection.char_shadow_mode == 2.f && shader_injection.char_shadow_type != 1.f; },
      .parse = [](float v) { return v * 0.01f; },
    },
    new renodx::utils::settings::Setting{
      .key = "CharShadowWorldStrength", .binding = &shader_injection.char_shadow_world_strength,
      .default_value = 100.f, .label = "World Strenght", .section = "Character Shadowing",
      .min = 0.f, .max = 100.f,
      .is_enabled = []() { return shader_injection.char_shadow_mode == 2.f && shader_injection.char_shadow_type != 0.f; },
      .parse = [](float v) { return v * 0.01f; },
    },
    new renodx::utils::settings::Setting{
      .key = "CharShadowSampleCount", .binding = &shader_injection.char_shadow_sample_count,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 32.f, .label = "Sample Count", .section = "Character Shadowing",
      .min = 1.f, .max = 64.f, .format = "%d",
      .is_enabled = []() { return shader_injection.char_shadow_mode == 2.f; },
    },
    new renodx::utils::settings::Setting{
      .key = "CharShadowHardSamples", .binding = &shader_injection.char_shadow_hard_shadow_samples,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 4.f, .label = "Hard Samples", .section = "Character Shadowing",
      .min = 0.f, .max = 64.f, .format = "%d",
      .is_enabled = []() { return shader_injection.char_shadow_mode == 2.f; },
    },
    new renodx::utils::settings::Setting{
      .key = "CharShadowFadeSamples", .binding = &shader_injection.char_shadow_fade_out_samples,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 16.f, .label = "Fade Samples", .section = "Character Shadowing",
      .min = 0.f, .max = 64.f, .format = "%d",
      .is_enabled = []() { return shader_injection.char_shadow_mode == 2.f; },
    },
    new renodx::utils::settings::Setting{
      .key = "CharShadowSurfaceThickness", .binding = &shader_injection.char_shadow_surface_thickness,
      .default_value = 0.09f, .label = "Surface Thickness", .section = "Character Shadowing",
      .min = 0.001f, .max = 0.2f, .format = "%.4f",
      .is_enabled = []() { return shader_injection.char_shadow_mode == 2.f; },
    },
    new renodx::utils::settings::Setting{
      .key = "CharShadowContrast", .binding = &shader_injection.char_shadow_contrast,
      .default_value = 9.f, .label = "Shadow Contrast", .section = "Character Shadowing",
      .min = 0.f, .max = 12.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.char_shadow_mode == 2.f; },
    },
    new renodx::utils::settings::Setting{
      .key = "CharShadowLightFadeStart", .binding = &shader_injection.char_shadow_light_screen_fade_start,
      .default_value = 0.f, .label = "Light Fade Start", .section = "Character Shadowing",
      .min = 0.f, .max = 1.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.char_shadow_mode == 2.f; },
    },
    new renodx::utils::settings::Setting{
      .key = "CharShadowLightFadeEnd", .binding = &shader_injection.char_shadow_light_screen_fade_end,
      .default_value = 0.f, .label = "Light Fade End", .section = "Character Shadowing",
      .min = 0.f, .max = 1.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.char_shadow_mode == 2.f; },
    },
    new renodx::utils::settings::Setting{
      .key = "CharShadowMinOccluderDepthScale", .binding = &shader_injection.char_shadow_min_occluder_depth_scale,
      .default_value = 0.f, .label = "Occluder Depth Scale", .section = "Character Shadowing",
      .min = 0.f, .max = 4.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.char_shadow_mode == 2.f; },
    },
    new renodx::utils::settings::Setting{
      .key = "EnvSSSEnabled", .binding = &shader_injection.env_sss_enabled,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 0.f, .label = "Bend SSS", .section = "Environment Screen Space Shadows",
      .labels = {"Off", "On"},
    },
    new renodx::utils::settings::Setting{
      .key = "EnvSSSStrength", .binding = &shader_injection.env_sss_strength,
      .default_value = 100.f, .label = "Strength", .section = "Environment Screen Space Shadows",
      .min = 0.f, .max = 100.f,
      .is_enabled = []() { return shader_injection.env_sss_enabled >= 0.5f; },
      .parse = [](float v) { return v * 0.01f; },
    },
    new renodx::utils::settings::Setting{
      .key = "EnvSSSSampleCount", .binding = &shader_injection.env_sss_sample_count,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 24.f, .label = "Sample Count", .section = "Environment Screen Space Shadows",
      .min = 1.f, .max = 64.f, .format = "%d",
      .is_enabled = []() { return shader_injection.env_sss_enabled >= 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "EnvSSSSurfaceThickness", .binding = &shader_injection.env_sss_surface_thickness,
      .default_value = 0.005f, .label = "Surface Thickness", .section = "Environment Screen Space Shadows",
      .min = 0.001f, .max = 0.2f, .format = "%.4f",
      .is_enabled = []() { return shader_injection.env_sss_enabled >= 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "EnvSSSContrast", .binding = &shader_injection.env_sss_contrast,
      .default_value = 2.f, .label = "Shadow Contrast", .section = "Environment Screen Space Shadows",
      .min = 0.f, .max = 12.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.env_sss_enabled >= 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "EnvSSSHeightEnable", .binding = &shader_injection.env_sss_height_enabled,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 1.f, .label = "Height Above Ground", .section = "Environment Screen Space Shadows",
      .labels = {"Off", "On"},
      .is_enabled = []() { return shader_injection.env_sss_enabled >= 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "EnvSSSHeightMin", .binding = &shader_injection.env_sss_height_min,
      .default_value = 0.f, .label = "Min Height", .section = "Environment Screen Space Shadows",
      .min = 0.f, .max = 10.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.env_sss_enabled >= 0.5f && shader_injection.env_sss_height_enabled >= 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "EnvSSSHeightMax", .binding = &shader_injection.env_sss_height_max,
      .default_value = 1.f, .label = "Ground Search", .section = "Environment Screen Space Shadows",
      .min = 1.f, .max = 200.f, .format = "%.0f",
      .is_enabled = []() { return shader_injection.env_sss_enabled >= 0.5f && shader_injection.env_sss_height_enabled >= 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "EnvSSSHeightFade", .binding = &shader_injection.env_sss_height_fade,
      .default_value = 0.10f, .label = "Height Fade", .section = "Environment Screen Space Shadows",
      .min = 0.f, .max = 5.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.env_sss_enabled >= 0.5f && shader_injection.env_sss_height_enabled >= 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "EnvSSSVerticalReject", .binding = &shader_injection.env_sss_vertical_reject,
      .default_value = 0.30f, .label = "Vertical Reject", .section = "Environment Screen Space Shadows",
      .min = 0.f, .max = 1.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.env_sss_enabled >= 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "EnvSSSMaxDarkening", .binding = &shader_injection.env_sss_max_darkening,
      .default_value = 0.40f, .label = "Max Darkening", .section = "Environment Screen Space Shadows",
      .min = 0.f, .max = 1.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.env_sss_enabled >= 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "EnvSSBrightRejectThreshold", .binding = &shader_injection.env_sss_bright_reject_threshold,
      .default_value = 0.19f, .label = "Brightness Reject", .section = "Environment Screen Space Shadows",
      .min = 0.f, .max = 5.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.env_sss_enabled >= 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "EnvSSBrightRejectFade", .binding = &shader_injection.env_sss_bright_reject_fade,
      .default_value = 0.5f, .label = "Brightness Fade", .section = "Environment Screen Space Shadows",
      .min = 0.01f, .max = 3.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.env_sss_enabled >= 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "DebugShowEnvSSS", .binding = &shader_injection.debug_show_env_sss,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 0.f, .label = "Env SSS Debug View", .section = "Environment Screen Space Shadows",
      .labels = {"Off", "Final Shadow", "Character Mask", "Surface Normal", "Raw Shadow"},
    },
    // —— XeGTAO ——
    new renodx::utils::settings::Setting{
      .key = "XeGTAOMode", .binding = &shader_injection.xegtao_mode,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 1.f, .label = "XeGTAO Mode", .section = "XeGTAO",
      .tooltip = "Off = vanilla game AO. On = XeGTAO compute-shader AO.",
      .labels = {"Off (Vanilla AO)", "On (XeGTAO)"},
    },
    new renodx::utils::settings::Setting{
      .key = "XeGTAOQuality", .binding = &shader_injection.xegtao_quality_level,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 1.f, .label = "Quality Level", .section = "XeGTAO",
      .labels = {"Low", "Medium", "High", "Ultra"},
      .is_enabled = []() { return shader_injection.xegtao_mode > 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "XeGTAODenoisePasses", .binding = &shader_injection.xegtao_denoise_passes,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 1.f, .label = "Denoise Passes", .section = "XeGTAO",
      .labels = {"Off", "Sharp (1)", "Medium (2)", "Soft (3)"},
      .is_enabled = []() { return shader_injection.xegtao_mode > 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "XeGTAOJitter", .binding = &g_xegtao_jitter_toggle,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 0.f, .label = "Jitter", .section = "XeGTAO",
      .tooltip = "Enable temporal jitter even when denoising is off.",
      .labels = {"Off", "On"},
      .is_enabled = []() { return shader_injection.xegtao_mode > 0.5f && shader_injection.xegtao_denoise_passes < 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "XeGTAORadius", .binding = &shader_injection.xegtao_radius,
      .default_value = 0.5f, .label = "Radius", .section = "XeGTAO",
      .min = 0.01f, .max = 5.0f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.xegtao_mode > 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "XeGTAOFalloffRange", .binding = &shader_injection.xegtao_falloff_range,
      .default_value = 0.615f, .label = "Falloff Range", .section = "XeGTAO",
      .min = 0.0f, .max = 1.0f, .format = "%.3f",
      .is_enabled = []() { return shader_injection.xegtao_mode > 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "XeGTAORadiusMultiplier", .binding = &shader_injection.xegtao_radius_multiplier,
      .default_value = 1.457f, .label = "Radius Multiplier", .section = "XeGTAO",
      .min = 0.3f, .max = 3.0f, .format = "%.3f",
      .is_enabled = []() { return shader_injection.xegtao_mode > 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "XeGTAOFinalPower", .binding = &shader_injection.xegtao_final_power,
      .default_value = 1.5f, .label = "Final Power", .section = "XeGTAO",
      .min = 0.5f, .max = 5.0f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.xegtao_mode > 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "XeGTAOSampleDistribution", .binding = &shader_injection.xegtao_sample_distribution,
      .default_value = 1.33f, .label = "Sample Distribution", .section = "XeGTAO",
      .min = 1.0f, .max = 3.0f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.xegtao_mode > 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "XeGTAOBitmaskThickness", .binding = &shader_injection.xegtao_bitmask_thickness,
      .default_value = 0.2f, .label = "Bitmask Thickness", .section = "XeGTAO",
      .tooltip = "World-space thickness for visibility bitmask. Higher = more light passes behind surfaces.",
      .min = 0.01f, .max = 2.0f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.xegtao_mode > 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "XeGTAODepthMIPOffset", .binding = &shader_injection.xegtao_depth_mip_offset,
      .default_value = 3.30f, .label = "Depth MIP Offset", .section = "XeGTAO",
      .min = 2.0f, .max = 6.0f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.xegtao_mode > 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "XeGTAODenoiseBlurBeta", .binding = &shader_injection.xegtao_denoise_blur_beta,
      .default_value = 20.0f, .label = "Denoise Blur Beta", .section = "XeGTAO",
      .min = 0.5f, .max = 20.0f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.xegtao_mode > 0.5f && shader_injection.xegtao_denoise_passes > 0.f; },
    },
    new renodx::utils::settings::Setting{
      .key = "XeGTAOInternalResolution", .binding = &shader_injection.xegtao_internal_resolution,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 2.f, .label = "Internal Resolution", .section = "XeGTAO",
      .labels = {"50%", "75%", "100%"},
      .is_enabled = []() { return shader_injection.xegtao_mode > 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "XeGTAONormalInputMode", .binding = &g_xegtao_normal_input_mode,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 1.f, .label = "MRT Normal Input", .section = "XeGTAO",
      .tooltip = "Off = depth normals only. On = use game g-buffer normals.",
      .labels = {"Off (Depth)", "On (MRT)"},
      .is_enabled = []() { return shader_injection.xegtao_mode > 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "XeGTAONormalInfluence", .binding = &g_xegtao_normal_influence,
      .default_value = 1.f, .label = "Normal Influence", .section = "XeGTAO",
      .min = 0.f, .max = 2.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.xegtao_mode > 0.5f && g_xegtao_normal_input_mode > 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "XeGTAONormalDepthBlend", .binding = &g_xegtao_normal_depth_blend,
      .default_value = 0.65f, .label = "Normal Depth Blend", .section = "XeGTAO",
      .min = 0.f, .max = 1.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.xegtao_mode > 0.5f && g_xegtao_normal_input_mode > 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "XeGTAONormalSharpness", .binding = &g_xegtao_normal_sharpness,
      .default_value = 1.f, .label = "Normal Sharpness", .section = "XeGTAO",
      .min = 0.01f, .max = 4.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.xegtao_mode > 0.5f && g_xegtao_normal_input_mode > 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "XeGTAONormalEdgeRejection", .binding = &g_xegtao_normal_edge_rejection,
      .default_value = 0.5f, .label = "Normal Edge Rejection", .section = "XeGTAO",
      .min = 0.f, .max = 2.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.xegtao_mode > 0.5f && g_xegtao_normal_input_mode > 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "XeGTAONormalZPreservation", .binding = &g_xegtao_normal_z_preservation,
      .default_value = 0.f, .label = "Normal Z Preservation", .section = "XeGTAO",
      .min = 0.f, .max = 2.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.xegtao_mode > 0.5f && g_xegtao_normal_input_mode > 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "XeGTAONormalDetailResponse", .binding = &g_xegtao_normal_detail_response,
      .default_value = 1.0f, .label = "Normal Detail Response", .section = "XeGTAO",
      .min = 0.01f, .max = 1.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.xegtao_mode > 0.5f && g_xegtao_normal_input_mode > 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "XeGTAONormalMaxDarkening", .binding = &g_xegtao_normal_max_darkening,
      .default_value = 0.4f, .label = "Normal Max Darkening", .section = "XeGTAO",
      .min = 0.f, .max = 1.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.xegtao_mode > 0.5f && g_xegtao_normal_input_mode > 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "XeGTAONormalDarkeningMode", .binding = &g_xegtao_normal_darkening_mode,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 0.f, .label = "Normal Darkening Mode", .section = "XeGTAO",
      .labels = {"Multiply", "Replace"},
      .is_enabled = []() { return shader_injection.xegtao_mode > 0.5f && g_xegtao_normal_input_mode > 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "XeGTAONormalTransformMode", .binding = &g_xegtao_normal_transform_mode,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 0.f, .label = "Normal Transform Mode", .section = "XeGTAO",
      .tooltip = "How to transform MRT normals to view space. Try alternatives if normals look wrong at some camera angles.",
      .labels = {"view_g (default)", "viewInv_g", "Passthrough"},
      .is_enabled = []() { return shader_injection.xegtao_mode > 0.5f && g_xegtao_normal_input_mode > 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "XeGTAODebugView", .binding = &shader_injection.xegtao_debug_view,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 0.f, .label = "Debug View", .section = "XeGTAO",
      .labels = {"Off", "AO Only", "XeGTAO raw .a", "XeGTAO RGBA", "Vanilla SSAO", "Depth",
                 "6:BitmaskHeat", "7:SectorCount", "8:1stSliceBits"},
    },
    new renodx::utils::settings::Setting{
      .key = "XeGTAODebugLogging", .binding = &shader_injection.xegtao_debug_logging,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 0.f, .label = "Debug Logging", .section = "XeGTAO",
      .labels = {"Off", "On"},
    },
    new renodx::utils::settings::Setting{
      .key = "XeGTAOFixExperimental", .binding = &shader_injection.xegtao_fix_experimental,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 0.f, .label = "Fix Experimental", .section = "XeGTAO",
      .tooltip = "Bitmask AO experimental fixes. 0=Off (baseline). Test each mode to diagnose darkening.",
      .labels = {"Off", "1:Clamp50%", "2:Clamp100%", "3:ScaleDist", "4:SkipBehind", "5:Skip2x"},
    },
    new renodx::utils::settings::Setting{
      .key = "XeGTAOFrameSkip", .binding = &g_xegtao_frame_skip,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 0.f, .label = "Frame Skip", .section = "XeGTAO",
      .tooltip = "Skip XeGTAO AO+GI computation every N frames to improve performance.",
      .labels = {"Off", "2 Frames", "3 Frames", "4 Frames"},
      .is_enabled = []() { return shader_injection.xegtao_mode > 0.5f; },
    },
    // —— SSGI (Screen Space Global Illumination — integrated into XeGTAO) ——
    new renodx::utils::settings::Setting{
      .key = "SSGIEnable", .binding = &shader_injection.ssgi_enabled,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 1.f, .label = "SSGI Enable", .section = "SSGI",
      .tooltip = "Visibility bitmask indirect diffuse GI. Requires XeGTAO Mode = On.",
      .labels = {"Off", "On"},
      .is_enabled = []() { return shader_injection.xegtao_mode > 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "SSGIIntensity", .binding = &shader_injection.ssgi_intensity,
      .default_value = 1.0f, .label = "Intensity", .section = "SSGI",
      .min = 0.0f, .max = 5.0f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.xegtao_mode > 0.5f && shader_injection.ssgi_enabled > 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "SSGISaturation", .binding = &shader_injection.ssgi_saturation,
      .default_value = 1.5f, .label = "Saturation", .section = "SSGI",
      .tooltip = "0 = grayscale GI, 1 = full color GI.",
      .min = 0.0f, .max = 2.0f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.xegtao_mode > 0.5f && shader_injection.ssgi_enabled > 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "SSGICharMaskStrength", .binding = &shader_injection.ssgi_char_mask_strength,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.f, .label = "Character Mask Strength", .section = "SSGI",
      .tooltip = "Reduce SSGI on character models. 0 = full GI on characters, 1 = fully masked.",
      .min = 0.5f, .max = 1.0f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.xegtao_mode > 0.5f && shader_injection.ssgi_enabled > 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "SSGIMultiBounce", .binding = &shader_injection.ssgi_multibounce,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 10.f, .label = "Multi-Bounce", .section = "SSGI",
      .tooltip = "Enables multi-bounce GI: previous frame's indirect light feeds back into the GI computation.",
      .labels = {"Off", "On"},
      .is_enabled = []() { return shader_injection.xegtao_mode > 0.5f && shader_injection.ssgi_enabled > 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "SSGIMultiBounceStrength", .binding = &shader_injection.ssgi_multibounce_strength,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 5.0f, .label = "Multi-Bounce Strength", .section = "SSGI",
      .tooltip = "Intensity of the multi-bounce feedback. 1.0 = natural, higher = stronger accumulation.",
      .min = 0.0f, .max = 10.0f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.xegtao_mode > 0.5f && shader_injection.ssgi_enabled > 0.5f && shader_injection.ssgi_multibounce > 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "SSGIMultiBounceSaturation", .binding = &shader_injection.ssgi_multibounce_saturation,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 1.5f, .label = "Multi-Bounce Saturation", .section = "SSGI",
      .tooltip = "Color saturation of the multi-bounce feedback. 0 = grayscale, 1 = full color.",
      .min = 0.0f, .max = 2.0f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.xegtao_mode > 0.5f && shader_injection.ssgi_enabled > 0.5f && shader_injection.ssgi_multibounce > 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "SSGIAdaptiveR", .binding = &shader_injection.ssgi_adaptive_r,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.f, .label = "Red Adaptive Strength", .section = "SSGI",
      .tooltip = "Per-channel adaptive boost: amplifies a color channel more when it's dominant. 0=off, 1=max.",
      .min = 0.0f, .max = 1.0f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.xegtao_mode > 0.5f && shader_injection.ssgi_enabled > 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "SSGIAdaptiveG", .binding = &shader_injection.ssgi_adaptive_g,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.f, .label = "Green Adaptive Strength", .section = "SSGI",
      .tooltip = "Per-channel adaptive boost: amplifies a color channel more when it's dominant. 0=off, 1=max.",
      .min = 0.0f, .max = 1.0f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.xegtao_mode > 0.5f && shader_injection.ssgi_enabled > 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "SSGIAdaptiveB", .binding = &shader_injection.ssgi_adaptive_b,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.f, .label = "Blue Adaptive Strength", .section = "SSGI",
      .tooltip = "Per-channel adaptive boost: amplifies a color channel more when it's dominant. 0=off, 1=max.",
      .min = 0.0f, .max = 1.0f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.xegtao_mode > 0.5f && shader_injection.ssgi_enabled > 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "SSGIAdaptiveMode", .binding = &shader_injection.ssgi_adaptive_mode,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 0.f, .label = "Adaptive Mode", .section = "SSGI",
      .tooltip = "GI Color = boost channels based on GI's own color. Albedo = boost based on surface color at pixel.",
      .labels = {"GI Color", "Surface Albedo"},
      .is_enabled = []() { return shader_injection.xegtao_mode > 0.5f && shader_injection.ssgi_enabled > 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "SSGIAdaptiveLumaStrength", .binding = &shader_injection.ssgi_adaptive_luma_strength,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.1f, .label = "Adaptive Luma Strength", .section = "SSGI",
      .tooltip = "Target brightness for GI normalization. 0=off. Higher = brighter target. Evens out indoor/outdoor GI.",
      .min = 0.0f, .max = 5.0f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.xegtao_mode > 0.5f && shader_injection.ssgi_enabled > 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "SSGIAdaptiveLumaBlend", .binding = &shader_injection.ssgi_adaptive_luma_blend,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.3f, .label = "Adaptive Luma Blend", .section = "SSGI",
      .tooltip = "Blend between original GI (0) and luma-normalized GI (1).",
      .min = 0.0f, .max = 1.0f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.xegtao_mode > 0.5f && shader_injection.ssgi_enabled > 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "SSGIGIPower", .binding = &shader_injection.ssgi_gi_power,
      .default_value = 1.5f, .label = "GI Power", .section = "SSGI",
      .tooltip = "Power curve applied to GI output. Higher = more contrast.",
      .min = 0.5f, .max = 5.0f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.xegtao_mode > 0.5f && shader_injection.ssgi_enabled > 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "SSGILightExposure", .binding = &g_ssgi_light_exposure,
      .default_value = 1.0f, .label = "Light Exposure", .section = "SSGI",
      .tooltip = "Exposure scale for HDR light buffer. Start at 0.05. Lower = dimmer GI.",
      .min = 0.001f, .max = 5.0f, .format = "%.3f",
      .is_enabled = []() { return shader_injection.xegtao_mode > 0.5f && shader_injection.ssgi_enabled > 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "SSGIFrameSkip", .binding = &g_ssgi_frame_skip,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 0.f, .label = "SSGI Frame Skip", .section = "SSGI",
      .tooltip = "Skip GI computation every N frames. AO still runs every frame.",
      .labels = {"Off", "2 Frames", "3 Frames", "4 Frames"},
      .is_enabled = []() { return shader_injection.xegtao_mode > 0.5f && shader_injection.ssgi_enabled > 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "MultiBounceFrameSkip", .binding = &g_multibounce_frame_skip,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 1.f, .label = "Multi-Bounce Frame Skip", .section = "SSGI",
      .tooltip = "Skip multi-bounce accumulation every N frames.",
      .labels = {"Off", "2 Frames", "3 Frames", "4 Frames"},
      .is_enabled = []() { return shader_injection.xegtao_mode > 0.5f && shader_injection.ssgi_enabled > 0.5f && shader_injection.ssgi_multibounce > 0.5f; },
    },
    // —— SSGI Debug ——
    new renodx::utils::settings::Setting{
      .key = "SSGIDebugView", .binding = &shader_injection.ssgi_debug_view,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 0.f, .label = "SSGI Debug View", .section = "SSGI",
      .tooltip = "Replace scene with SSGI debug textures.",
      .labels = {"Off", "Raw GI", "Denoised GI", "Light Buffer", "Accumulated", "5:Sample Activity", "Light Color"},
    },
    new renodx::utils::settings::Setting{
      .key = "SSGIDebugLogging", .binding = &shader_injection.ssgi_debug_logging,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 0.f, .label = "SSGI Debug Logging", .section = "SSGI",
      .tooltip = "Log SSGI dispatch, push, and texture binding to console.",
      .labels = {"Off", "On"},
    },
    // —— SSGI Affect Lights ——
    new renodx::utils::settings::Setting{
      .key = "SSGIAffectLights", .binding = &shader_injection.ssgi_affect_lights,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 0.f, .label = "Affect Lights", .section = "SSGI",
      .tooltip = "Additively blend the sun's lightColor into the GI contribution, tinting indirect light.",
      .labels = {"Off", "On"},
    },
    new renodx::utils::settings::Setting{
      .key = "SSGILightsStrength", .binding = &shader_injection.ssgi_lights_strength,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 1.f, .label = "Lights Strength", .section = "SSGI",
      .tooltip = "How much lightColor to add. 0=no effect, 1=full sun color, >1=boosted.",
      .min = 0.f, .max = 5.0f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.ssgi_affect_lights > 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "SSGILightsSaturation", .binding = &shader_injection.ssgi_lights_saturation,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 1.f, .label = "Lights Saturation", .section = "SSGI",
      .tooltip = "Vibrance applied to lightColor before adding. 0=grayscale, 1=neutral, >1=vivid.",
      .min = 0.f, .max = 100.0f, .format = "%.1f",
      .is_enabled = []() { return shader_injection.ssgi_affect_lights > 0.5f; },
    },
    // —— IS-FAST ——
    new renodx::utils::settings::Setting{
      .key = "ISFASTEnable", .binding = &g_isfast_enabled,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 1.f, .label = "IS-FAST Noise", .section = "IS-FAST",
      .tooltip = "IS-FAST spatio-temporal blue noise. Requires fast_noise_ea.dds next to game .exe. Falls back to IGN if missing.",
      .labels = {"Off", "On"},
    },
    new renodx::utils::settings::Setting{
      .key = "ISFASTStrength", .binding = &g_isfast_strength,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 1.f, .label = "Noise Strength", .section = "IS-FAST",
      .tooltip = "0 = deterministic (banding), 1 = full noise.",
      .min = 0.0f, .max = 1.0f, .format = "%.2f",
      .is_enabled = []() { return g_isfast_enabled > 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "ISFASTDebugLogging", .binding = &g_isfast_debug_logging,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 0.f, .label = "Debug Logging", .section = "IS-FAST",
      .tooltip = "Log IS-FAST texture load status and noise source.",
      .labels = {"Off", "On"},
    },
    new renodx::utils::settings::Setting{
      .key = "ISFASTSpatialScale", .binding = &g_isfast_spatial_scale,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 1.f, .label = "Spatial Scale", .section = "IS-FAST",
      .tooltip = "Scale noise spatial frequency. <1 zooms in (smoother), >1 adds more detail.",
      .min = 0.25f, .max = 4.0f, .format = "%.2f",
      .is_enabled = []() { return g_isfast_enabled > 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "ISFASTTemporalSpeed", .binding = &g_isfast_temporal_speed,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 1.f, .label = "Temporal Speed", .section = "IS-FAST",
      .tooltip = "Scale noise animation speed. 0 = frozen, 1 = default, 5 = fast flicker.",
      .min = 0.0f, .max = 5.0f, .format = "%.2f",
      .is_enabled = []() { return g_isfast_enabled > 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "ISFASTSeedOffset", .binding = &g_isfast_seed_offset,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 0.f, .label = "Seed Offset", .section = "IS-FAST",
      .tooltip = "Offset the noise seed pattern (0-64). Shift to find optimal noise distribution.",
      .labels = {"0","4","8","12","16","20","24","28","32","36","40","44","48","52","56","60"},
      .is_enabled = []() { return g_isfast_enabled > 0.5f; },
    },
    // —— CPU Optimizations ——
    new renodx::utils::settings::Setting{
      .key = "CPUOptDeferredDispatch", .binding = &g_cpuopt_deferred_dispatch,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 0.f, .label = "Deferred Dispatch", .section = "CPU Opt",
      .tooltip = "Move XeGTAO/SSGI dispatch to OnPresent (kai-style, 1-frame latency).",
      .labels = {"Off", "On"},
    },
    new renodx::utils::settings::Setting{
      .key = "CPUOptEnsurePipelines", .binding = &g_cpuopt_ensure_pipelines,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 1.f, .label = "Ensure Pipelines", .section = "CPU Opt",
      .tooltip = "Don't destroy/recreate pipelines every frame (kai-style).",
      .labels = {"Off", "On"},
    },
    // —— Shadow Maps ——
    new renodx::utils::settings::Setting{
      .key = "ShadowFilterMethod", .binding = &shader_injection.shadow_filter_method,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 2.f, .label = "Shadow Filter Method", .section = "Shadow Maps",
      .tooltip = "CSM filtering: Off = single sample. Falcom = vanilla 10-tap PCF. PCSS = physically-accurate soft shadows.",
      .labels = {"Off", "Falcom", "PCSS"},
    },
    new renodx::utils::settings::Setting{
      .key = "ShadowEdgeTint", .binding = &shader_injection.shadow_edge_tint,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 1.f, .label = "Colored Shadow Penumbra", .section = "Shadow Maps",
      .tooltip = "Off = neutral edges. Falcom = vanilla red tint. Improved = PCSS vibrancy boost in penumbra.",
      .labels = {"Off", "Falcom", "Improved"},
    },
    // —— PCSS Settings (enabled when ShadowFilterMethod = PCSS) ——
    new renodx::utils::settings::Setting{
      .key = "ShadowPCSSJitter", .binding = &shader_injection.shadow_pcss_jitter_enabled,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 1.f, .label = "PCSS Jitter", .section = "Shadow Maps",
      .tooltip = "Use IS-FAST spatio-temporal noise to rotate PCSS sample pattern each frame.",
      .labels = {"Off", "On"},
      .is_enabled = []() { return shader_injection.shadow_filter_method > 1.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "ShadowPCSSJitterAmount", .binding = &shader_injection.shadow_pcss_jitter_amount,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 1.f, .label = "Jitter Amount", .section = "Shadow Maps",
      .tooltip = "0 = static Poisson, 1 = full temporal rotation.",
      .min = 0.0f, .max = 1.0f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.shadow_filter_method > 1.5f && shader_injection.shadow_pcss_jitter_enabled > 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "ShadowPCSSJitterSpeed", .binding = &shader_injection.shadow_pcss_jitter_speed,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 237.f, .label = "Jitter Speed", .section = "Shadow Maps",
      .tooltip = "Temporal animation speed. Higher = faster rotation.",
      .min = 0.0f, .max = 500.0f, .format = "%.0f",
      .is_enabled = []() { return shader_injection.shadow_filter_method > 1.5f && shader_injection.shadow_pcss_jitter_enabled > 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "ShadowBaseSoftness", .binding = &shader_injection.shadow_base_softness,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.05f, .label = "Base Softness", .section = "Shadow Maps",
      .tooltip = "Constant minimum penumbra width. Contact-hard at 0, always soft at 0.5.",
      .min = 0.0f, .max = 1.0f, .format = "%.3f",
      .is_enabled = []() { return shader_injection.shadow_filter_method > 1.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "ShadowPenumbraScale", .binding = &shader_injection.shadow_penumbra_scale,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 30.f, .label = "Penumbra Scale", .section = "Shadow Maps",
      .tooltip = "How fast penumbra widens with occluder distance. Higher = softer distant shadows.",
      .min = 1.0f, .max = 100.0f, .format = "%.1f",
      .is_enabled = []() { return shader_injection.shadow_filter_method > 1.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "ShadowPCSSSearchRadius", .binding = &shader_injection.shadow_pcss_search_radius,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 1.f, .label = "World Softness", .section = "Shadow Maps",
      .tooltip = "Desired softness in world units. Same value = same penumbra width across all cascades. 0.1=sharp, 5=very soft.",
      .min = 0.1f, .max = 2.0f, .format = "%.1f",
      .is_enabled = []() { return shader_injection.shadow_filter_method > 1.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "ShadowPCSSFilterWidth", .binding = &shader_injection.shadow_pcss_filter_width,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 2.f, .label = "Filter Width", .section = "Shadow Maps",
      .tooltip = "PCF filter width multiplier. Lower = sharper, higher = blurrier.",
      .min = 0.1f, .max = 5.0f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.shadow_filter_method > 1.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "ShadowPCSSDepthCap", .binding = &shader_injection.shadow_pcss_depth_cap,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.015f, .label = "Depth Sensitivity", .section = "Shadow Maps",
      .tooltip = "Max depth difference for penumbra. Higher = more distance-based softening.",
      .min = 0.01f, .max = 1.0f, .format = "%.3f",
      .is_enabled = []() { return shader_injection.shadow_filter_method > 1.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "ShadowPCSScascadeBlend", .binding = &shader_injection.shadow_pcss_cascade_blend,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.3f, .label = "Cascade Blend", .section = "Shadow Maps",
      .tooltip = "Cross-fade width between cascades. Lower = wider/smoother blend. 0.02 = 50 units, 1.0 = 1 unit.",
      .min = 0.02f, .max = 1.0f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.shadow_filter_method > 1.5f; },
    },
    // —— Colored Shadow Penumbra (Improved mode, PCSS-only) ——
    new renodx::utils::settings::Setting{
      .key = "ShadowPenumbraColorStrength", .binding = &shader_injection.shadow_penumbra_color_strength,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 1.f, .label = "Penumbra Color Strength", .section = "Shadow Maps",
      .tooltip = "How strongly the vibrancy effect is applied in penumbra regions. 0=off, 1=full.",
      .min = 0.f, .max = 2.0f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.shadow_edge_tint > 1.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "ShadowPenumbraVibrance", .binding = &shader_injection.shadow_penumbra_vibrance,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 1.f, .label = "Penumbra Vibrance", .section = "Shadow Maps",
      .tooltip = "Vibrance adjustment in penumbra. 0=grayscale, 1=neutral, >1=more vivid. Protects already-saturated colors.",
      .min = 0.f, .max = 100.0f, .format = "%.1f",
      .is_enabled = []() { return shader_injection.shadow_edge_tint > 1.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "ShadowPenumbraDetection", .binding = &shader_injection.shadow_penumbra_detection,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.5f, .label = "Penumbra Detection", .section = "Shadow Maps",
      .tooltip = "What counts as penumbra. Higher = wider detection area, more of the image gets the effect.",
      .min = 0.01f, .max = 1.0f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.shadow_edge_tint > 1.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "ShadowPenumbraColorBrightness", .binding = &shader_injection.shadow_penumbra_color_brightness,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 1.f, .label = "Penumbra Color Brightness", .section = "Shadow Maps",
      .tooltip = "Brightness multiplier for the vibrancy tint color. 1=neutral, 0=black, >1=brighter.",
      .min = 0.f, .max = 5.0f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.shadow_edge_tint > 1.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "ShadowPenumbraFalcomBlend", .binding = &shader_injection.shadow_penumbra_falcom_blend,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.f, .label = "Falcom Penumbra Blend", .section = "Shadow Maps",
      .tooltip = "Blend the vibrancy effect toward Falcom's red shadowEdgeColor tint. 0=pure vibrancy, 1=pure Falcom.",
      .min = 0.f, .max = 1.0f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.shadow_edge_tint > 1.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "ShadowPenumbraEdgeVibrance", .binding = &shader_injection.shadow_penumbra_edge_vibrance,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 1.f, .label = "Edge Color Vibrance", .section = "Shadow Maps",
      .tooltip = "Vibrance applied to shadowEdgeColor when Falcom blend > 0. 0=grayscale, 1=neutral, >1=vivid.",
      .min = 0.f, .max = 100.0f, .format = "%.1f",
      .is_enabled = []() { return shader_injection.shadow_edge_tint > 1.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "ShadowPenumbraLightColorBlend", .binding = &shader_injection.shadow_penumbra_lightcolor_blend,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 0.f, .label = "Light Color Blend", .section = "Shadow Maps",
      .tooltip = "Blend the penumbra tint toward the sun's lightColor. 0=no effect, 1=fully sun-colored penumbra.",
      .min = 0.f, .max = 1.0f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.shadow_edge_tint > 1.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "ShadowPenumbraLightColorSaturation", .binding = &shader_injection.shadow_penumbra_lightcolor_saturation,
      .value_type = renodx::utils::settings::SettingValueType::FLOAT,
      .default_value = 1.f, .label = "Light Color Saturation", .section = "Shadow Maps",
      .tooltip = "Vibrance applied to lightColor before blending. 0=grayscale, 1=neutral, >1=vivid sun color.",
      .min = 0.f, .max = 100.0f, .format = "%.1f",
      .is_enabled = []() { return shader_injection.shadow_edge_tint > 1.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "ShadowPenumbraDebugView", .binding = &shader_injection.shadow_penumbra_debug_view,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 0.f, .label = "Penumbra Debug View", .section = "Shadow Maps",
      .tooltip = "Visualize penumbra processing. PenumbraMask=detection area, TintColor=adjusted color, Result=final blend.",
      .labels = {"Off", "Penumbra Mask", "Tint Color", "Result", "Sun Color"},
      .is_enabled = []() { return shader_injection.shadow_edge_tint > 1.5f; },
    },
    // ── SSGI debug views removed — use XeGTAO Debug View for GI inspection. ──
};

// ═══════════ XeGTAO Backend — implementation ═══════════

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

  reshade::log::message(reshade::log::level::info, "[XeGTAO] Device init — fallback SRV created.");
}

static void OnDestroyDevice(reshade::api::device* device) {
  auto* d = device->get_private_data<DeviceData>();
  if (d) {
    DestroyXeGTAOResources(device, d);
    if (d->fallback_srv.handle) device->destroy_resource_view(d->fallback_srv);
    if (d->fallback_texture.handle) device->destroy_resource(d->fallback_texture);
    device->destroy_private_data<DeviceData>();
  }
}

static void OnInitSwapchain(reshade::api::swapchain* sc, bool resize) {
  auto* d = sc->get_device()->get_private_data<DeviceData>();
  if (!d) return;
  if (resize) {
    d->resize_guard_until_frame = d->frame_index + kXeGTAOResizeGuardFrames;
    d->captured_depth_srv = {}; d->captured_ssao_srv = {};
    d->captured_scene_cbv_view = {};
    d->captured_scene_cbv = {}; d->captured_scene_cbv_valid = false;
    d->captured_scene_cbv_frame = UINT64_MAX;
    DestroyXeGTAOResources(sc->get_device(), d);
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
  DestroyXeGTAOResources(sc->get_device(), d);
}

// ── Descriptor table helpers ──

static bool EnsureXeGTAODescriptorTables(
    reshade::api::device* device,
    reshade::api::pipeline_layout layout,
    XeGTAODescriptorTableSet* tables) {
  if (!device || !tables || !layout.handle) return false;
  for (uint32_t i = 0; i < kXeGtaoDescriptorTableParamCount; ++i) {
    if ((*tables)[i].handle != 0u) continue;
    if (!device->allocate_descriptor_table(layout, i, &(*tables)[i]))
      return false;
  }
  return true;
}

static void DestroyXeGTAODescriptorTables(
    reshade::api::device* device, XeGTAODescriptorTableSet* tables) {
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
    // Capture depth t4 — ONLY from the lighting shader (hash 0xFDAAF80E).
    // Post-processing passes may bind full-res textures at t4, overwriting the real depth.
    if (update.binding == kLightingDepthRegister && update.count >= 1
        && views[0].handle != 0u) {
      auto* ss = renodx::utils::shader::GetCurrentState(cmd_list);
      if (ss) {
        uint32_t hash = renodx::utils::shader::GetCurrentPixelShaderHash(ss);
        if (hash == 0xFDAAF80Eu) {
          d->captured_depth_srv = views[0];
          d->captured_scene_cbv_frame = d->frame_index;
          if (shader_injection.xegtao_debug_logging > 0.5f) {
            auto depth_res = device->get_resource_from_view(views[0]);
            if (depth_res.handle != 0u) {
              auto dd = device->get_resource_desc(depth_res);
              reshade::log::message(reshade::log::level::info,
                (std::string("[XeGTAO] Depth captured from lighting: ") +
                 std::to_string(dd.texture.width) + "x" +
                 std::to_string(dd.texture.height)).c_str());
            }
          }
        }
      }
    }
    if (update.binding == kLightingSsaoRegister && update.count >= 1
        && views[0].handle != 0u) {
      d->captured_ssao_srv = views[0];
    }
    if (update.binding == kLightingMrtNormalRegister && update.count >= 1
        && views[0].handle != 0u) {
      d->captured_mrt_normal_srv = views[0];
    }
    // Capture t0 color texture — ONLY from the lighting shader (hash 0xFDAAF80E).
    // Unconditional capture would grab binding 0 from any shader, causing wrong colors.
    if (update.binding == 0u && update.count >= 1
        && views[0].handle != 0u) {
      auto* ss = renodx::utils::shader::GetCurrentState(cmd_list);
      if (ss) {
        uint32_t hash = renodx::utils::shader::GetCurrentPixelShaderHash(ss);
        if (hash == 0xFDAAF80Eu) {
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

  // ── Per-draw gating (only when XeGTAO or SSGI is on). ──
  if (shader_injection.xegtao_mode < 0.5f) return;
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
      (std::string("[XeGTAO] bind_descriptor_tables: first=") +
      std::to_string(first) + ", count=" + std::to_string(count)).c_str());
  }

  if (shader_injection.xegtao_mode < 0.5f) return;

  // Only capture on pixel-stage draws.
  const uint32_t sm = static_cast<uint32_t>(stages);
  if (!(sm & static_cast<uint32_t>(reshade::api::shader_stage::pixel))) return;

  // Verify this is the lighting shader.
  auto* ss = renodx::utils::shader::GetCurrentState(cmd_list);
  if (!ss) return;
  uint32_t hash = renodx::utils::shader::GetCurrentPixelShaderHash(ss);
  if (hash != 0xFDAAF80Eu) return;  // Only lighting shader

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

// ── Present hook ──

static void OnPresent(reshade::api::command_queue* queue, reshade::api::swapchain* sc,
                       const reshade::api::rect*, const reshade::api::rect*,
                       uint32_t, const reshade::api::rect*) {
  auto* dev = queue->get_device();
  auto* cl = queue->get_immediate_command_list();
  auto* d = dev->get_private_data<DeviceData>();
  if (!d) return;
  d->frame_index++;
  if (shader_injection.xegtao_mode < 0.5f) return;
  if (d->frame_index <= kXeGTAOStartupGuardFrames) {
    if (d->frame_index == kXeGTAOStartupGuardFrames) {
      reshade::log::message(reshade::log::level::info,
        "[XeGTAO] Startup guard complete — dispatch begins next frame.");
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
      CreateXeGTAOResources(dev, d, gw, gh);
      d->last_created_game_width = gw;
      d->last_created_game_height = gh;
      d->resources_created = true;
      reshade::log::message(reshade::log::level::info,
        (std::string("[XeGTAO] Resources created: ") +
         std::to_string(d->working_width) + "x" +
         std::to_string(d->working_height) + " (depth=" +
         std::to_string(gw) + "x" + std::to_string(gh) + ")").c_str());
    }
  }
  // Use deferred snapshots from lighting draw (kai-style) — deferred dispatch only.
  // ── Light-buffer capture helper (runs after XeGTAO for multi-bounce feedback) ──
  auto capture_light_buffer_for_next_frame = [&]() {
    if (shader_injection.ssgi_enabled < 0.5f || !d->captured_light_buffer_texture.handle) return;
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

  // Inline dispatch active (deferred off) — XeGTAO runs during lighting pass, not here.
  if (!d->deferred_pending || !d->deferred_depth_srv.handle) {
    capture_light_buffer_for_next_frame();
    return;
  }
  if (!d->deferred_scene_cbv_valid
      || (d->frame_index - d->deferred_scene_cbv_frame) > 1u) {
    capture_light_buffer_for_next_frame();
    if (shader_injection.xegtao_debug_logging > 0.5f) {
      reshade::log::message(reshade::log::level::warning,
                            "[XeGTAO] Dispatch skipped: no deferred scene CBV.");
    }
    return;
  }
  // Restore deferred snapshots as active captures for RunXeGTAO / RunSSGI.
  d->captured_depth_srv = d->deferred_depth_srv;
  d->captured_ssao_srv = d->deferred_ssao_srv;
  d->captured_mrt_normal_srv = d->deferred_mrt_normal_srv;
  d->captured_scene_cbv_view = d->deferred_scene_cbv_view;
  d->captured_scene_cbv = d->deferred_scene_cbv;
  d->captured_scene_cbv_valid = d->deferred_scene_cbv_valid;
  d->captured_scene_cbv_frame = d->deferred_scene_cbv_frame;
  d->deferred_pending = false;

  // XeGTAO reads proj_g directly from the game's scene CBV (b0) in-shader —
  // no CPU-side mapping needed (kai-vanillaplus approach).

  if (shader_injection.xegtao_debug_logging > 0.5f)
    reshade::log::message(reshade::log::level::info,
      (std::string("[XeGTAO] Dispatching (frame=") +
       std::to_string(d->frame_index) + ", res=" +
       std::to_string(d->working_width) + "x" +
       std::to_string(d->working_height) + ")").c_str());

  // Save command-list state.
  auto* cs = renodx::utils::state::GetCurrentState(cl);
  renodx::utils::state::CommandListState prev = {};
  if (cs) prev = *cs;

  bool ok = RunXeGTAO(cl, d);

  // Restore: unbind compute pipeline, restore descriptor tables.
  cl->bind_pipeline(reshade::api::pipeline_stage::all_compute, reshade::api::pipeline{0u});
  if (cs) *cs = prev;

  if (shader_injection.xegtao_debug_logging > 0.5f && ok) {
    std::ostringstream msg;
    msg << "[XeGTAO] Dispatch OK (frame=" << d->frame_index
        << ", res=" << d->working_width << "x" << d->working_height << ")";
    reshade::log::message(reshade::log::level::info, msg.str().c_str());
  } else if (shader_injection.xegtao_debug_logging > 0.5f && !ok) {
    reshade::log::message(reshade::log::level::warning, "[XeGTAO] Dispatch failed.");
  }

  // ── GI is now integrated into XeGTAO main pass (visibility bitmask AO+GI). ──
  // The GI output (ssgi_denoised_srv) is produced during RunXeGTAO denoise pass.
  // No separate SSGI dispatch needed.

  // ── Capture light buffer for next frame's multi-bounce (after GI applied) ──
  capture_light_buffer_for_next_frame();

  shader_injection.xegtao_ssgi_bound = 0.f;  // Reset for next frame's SSAO pass
}

static bool OnBeforeLightingShaderDraw(reshade::api::command_list* cmd_list) {
  // IMPORTANT: returning false would BYPASS the draw (skip it entirely).
  shader_injection.xegtao_dedicated_bound = 0.f;
  SyncISFASTToShaderInjection(cmd_list);  // keep IS-FAST mirrors in sync

  if (shader_injection.xegtao_mode < 0.5f) return true;
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

  // ── Inline dispatch: Run XeGTAO on this frame's command list (only when NOT deferred). ──
  if (g_cpuopt_deferred_dispatch < 0.5f) {
    if (dd->captured_depth_srv.handle && dd->captured_scene_cbv_valid
        && dd->ao_term_a_srv.handle) {
      auto* cs = renodx::utils::state::GetCurrentState(cmd_list);
      renodx::utils::state::CommandListState prev = {};
      if (cs) prev = *cs;

      bool ok = RunXeGTAO(cmd_list, dd);

      cmd_list->bind_pipeline(reshade::api::pipeline_stage::all_compute, reshade::api::pipeline{0u});
      if (cs) *cs = prev;
      (void)ok;
    }
  }

  // Push the XeGTAO AO result at t22.
  // In inline mode: fresh from dispatch above.
  // In deferred mode: result from previous frame's OnPresent dispatch.
  // Effective dpc = max(1, setting) — matches forced denoise in RunXeGTAO.
  int edpc = (int)shader_injection.xegtao_denoise_passes;
  if (edpc < 1) edpc = 1;
  reshade::api::resource_view srv = (edpc & 1)
      ? dd->ao_term_b_srv : dd->ao_term_a_srv;
  if (srv.handle) {
    cmd_list->push_descriptors(
        reshade::api::shader_stage::pixel,
        reshade::api::pipeline_layout{0},
        0,
        reshade::api::descriptor_table_update{
            {}, kLightingXeGtaoRegister, 0, 1,
            reshade::api::descriptor_type::texture_shader_resource_view, &srv,
        });
    shader_injection.xegtao_dedicated_bound = 1.f;
  }

  // ── SSGI push t23 (GI is produced by RunXeGTAO) ──
  shader_injection.xegtao_ssgi_bound = 0.f;
  shader_injection.xegtao_ssgi_debug = 0.f;

  // Determine what to push to t23.
  reshade::api::resource_view push_srv = {};
  bool do_push = false;
  bool debug_replace = false;

  // SSGI debug views (1=Raw GI, 2=Denoised GI, 3=Light Buffer, 4=Accumulated, 5=Samples).
  if (shader_injection.ssgi_debug_view > 0.5f) {
    int dv = (int)shader_injection.ssgi_debug_view;
    if (dv == 1)      push_srv = dd->ssgi_output_srv;
    else if (dv == 2) push_srv = dd->ssgi_denoised_srv;
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
  else if (shader_injection.xegtao_debug_view > 5.5f && shader_injection.xegtao_debug_view < 8.5f) {
    push_srv = dd->debug_srv;
    do_push = true;
    debug_replace = true;
  }
  // Normal SSGI: push denoised GI.
  else if (shader_injection.ssgi_enabled > 0.5f) {
    push_srv = dd->ssgi_denoised_srv;
    do_push = true;
  }

  if (do_push) {
    if (!push_srv.handle) push_srv = dd->fallback_srv;
    if (push_srv.handle) {
      cmd_list->push_descriptors(
          reshade::api::shader_stage::pixel,
          reshade::api::pipeline_layout{0},
          0,
          reshade::api::descriptor_table_update{
              {}, kLightingSsgiRegister, 0, 1,
              reshade::api::descriptor_type::texture_shader_resource_view,
              &push_srv,
          });
      shader_injection.xegtao_ssgi_bound = 1.f;
      if (debug_replace) shader_injection.xegtao_ssgi_debug = 1.f;
    }
    // SSGI debug logging.
    if (shader_injection.ssgi_debug_logging > 0.5f) {
      std::string msg = "[SSGI] t23 push: srv=";
      msg += push_srv.handle ? "valid" : "FALLBACK";
      msg += " debug=" + std::to_string(debug_replace ? 1 : 0);
      msg += " ssgi_enabled=" + std::to_string((int)shader_injection.ssgi_enabled);
      reshade::log::message(reshade::log::level::info, msg.c_str());
    }
  }

  return true;
}

static bool OnBeforeSsaoShaderDraw(reshade::api::command_list*) {
  // Used as on_replace callback via CustomShaderEntryCallback.
  // Return true = use our replacement SSAO shader (with XeGTAO gate).
  return true;
}

// ── Resource create / destroy ──

static void CreateXeGTAOResources(reshade::api::device* dev, DeviceData* d,
                                   uint32_t gw, uint32_t gh) {
  DestroyXeGTAOResources(dev, d);
  // Scale by internal resolution setting (50/75/100%) from depth buffer size.
  // Exact match — no snapping. Shader GetDimensions() handles bounds correctly.
  int ir = (int)shader_injection.xegtao_internal_resolution;  // 0=50%, 1=75%, 2=100%
  float scale = (ir == 0) ? 0.5f : (ir == 1) ? 0.75f : 1.0f;
  uint32_t w = (uint32_t)(gw * scale);
  uint32_t h = (uint32_t)(gh * scale);
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
    rd.texture = {w, h, 1, (uint16_t)kXeGTAODepthMipLevels, reshade::api::format::r32_float, 1};
    rd.heap = reshade::api::memory_heap::gpu_only;
    rd.usage = reshade::api::resource_usage::shader_resource | reshade::api::resource_usage::unordered_access;
    dev->create_resource(rd, nullptr, reshade::api::resource_usage::shader_resource, &d->depth_mips_texture);
    dev->create_resource_view(d->depth_mips_texture, reshade::api::resource_usage::shader_resource,
                               reshade::api::resource_view_desc(reshade::api::resource_view_type::texture_2d,
                                                                 reshade::api::format::r32_float, 0, kXeGTAODepthMipLevels, 0, 1),
                               &d->depth_mips_srv);
    for (uint32_t m = 0; m < kXeGTAODepthMipLevels; ++m)
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
  mk(w, h, reshade::api::format::r32_float, &d->edges_texture, &d->edges_srv, &d->edges_uav);
  mk(gw, gh, reshade::api::format::r8g8b8a8_unorm, &d->composite_texture, &d->composite_srv, &d->composite_uav);

  // ── GI resources (same resolution as AO per user preference) ──
  mk(w, h, reshade::api::format::r16g16b16a16_float,
     &d->ssgi_output_texture, &d->ssgi_output_srv, &d->ssgi_output_uav);
  mk(w, h, reshade::api::format::r16g16b16a16_float,
     &d->ssgi_denoised_texture, &d->ssgi_denoised_srv, &d->ssgi_denoised_uav);
  mk(w, h, reshade::api::format::r8g8b8a8_unorm,
     &d->debug_texture, &d->debug_srv, &d->debug_uav);
  // Light buffer capture at full back-buffer resolution
  mk(gw, gh, reshade::api::format::r16g16b16a16_float,
     &d->captured_light_buffer_texture, &d->captured_light_buffer_srv, nullptr);
  // Multi-bounce accumulation buffer (HDR, same resolution as working set)
  mk(w, h, reshade::api::format::r16g16b16a16_float,
     &d->multibounce_texture, &d->multibounce_srv, &d->multibounce_uav);
  d->ssgi_denoised_valid = false;
}

static void DestroyXeGTAOResources(reshade::api::device* dev, DeviceData* d) {
  if (!dev || !d) return;
  auto dv = [&](reshade::api::resource_view& v) { if (v.handle) { dev->destroy_resource_view(v); v = {}; } };
  auto dr = [&](reshade::api::resource& r) { if (r.handle) { dev->destroy_resource(r); r = {}; } };
  auto dp = [&](reshade::api::pipeline& p) { if (p.handle) { dev->destroy_pipeline(p); p = {}; } };
  auto dl = [&](reshade::api::pipeline_layout& l) { if (l.handle) { dev->destroy_pipeline_layout(l); l = {}; } };

  dv(d->depth_mips_srv); for (auto& u : d->depth_mips_uavs) dv(u); dr(d->depth_mips_texture);
  dv(d->ao_term_a_srv); dv(d->ao_term_a_uav); dr(d->ao_term_a_texture);
  dv(d->ao_term_b_srv); dv(d->ao_term_b_uav); dr(d->ao_term_b_texture);
  dv(d->edges_srv); dv(d->edges_uav); dr(d->edges_texture);
  dv(d->composite_srv); dv(d->composite_uav); dr(d->composite_texture);
  if (d->point_clamp_sampler.handle) { dev->destroy_sampler(d->point_clamp_sampler); d->point_clamp_sampler = {}; }
  dp(d->prefilter_pipeline); dp(d->main_low_pipeline); dp(d->main_medium_pipeline);
  dp(d->main_high_pipeline); dp(d->main_ultra_pipeline); dp(d->denoise_pipeline);
  dp(d->denoise_last_pipeline);
  dl(d->prefilter_layout); dl(d->main_layout); dl(d->denoise_layout);
  DestroyXeGTAODescriptorTables(dev, &d->prefilter_tables);
  DestroyXeGTAODescriptorTables(dev, &d->main_tables);
  DestroyXeGTAODescriptorTables(dev, &d->denoise_tables);
  // GI resources (now integrated — no separate SSGI pipeline)
  dv(d->ssgi_output_srv); dv(d->ssgi_output_uav); dr(d->ssgi_output_texture);
  dv(d->ssgi_denoised_srv); dv(d->ssgi_denoised_uav); dr(d->ssgi_denoised_texture);
  dv(d->captured_light_buffer_srv); dr(d->captured_light_buffer_texture);
  dv(d->multibounce_srv); dv(d->multibounce_uav); dr(d->multibounce_texture);
  dv(d->debug_srv); dv(d->debug_uav); dr(d->debug_texture);
  dp(d->multibounce_pipeline); dl(d->multibounce_layout);
  DestroyXeGTAODescriptorTables(dev, &d->multibounce_tables);
  // IS-FAST noise
  dv(d->isfast_noise_srv); dr(d->isfast_noise_texture);
  d->isfast_texture_loaded = false;
  d->isfast_texture_attempted = false;
  // Do NOT clear captured_depth_srv / captured_scene_cbv —
  // those reference game-owned resources that survive recreation.
  d->resources_created = false;
}

// ── Push constants builder (kai-vanillaplus style) ──

static std::array<float, 48> BuildXeGTAOPushConstants(DeviceData* data, bool denoise_last_pass,
                                                       float ssgi_enabled_override = -1.f) {
  std::array<float, 48> c = {};
  const uint32_t denoise_passes = (uint32_t)shader_injection.xegtao_denoise_passes;
  c[0]  = shader_injection.xegtao_quality_level;
  c[1]  = (float)denoise_passes;
  c[2]  = std::max(0.001f, shader_injection.xegtao_radius);
  c[3]  = std::clamp(shader_injection.xegtao_falloff_range, 0.f, 1.f);
  c[4]  = std::clamp(shader_injection.xegtao_radius_multiplier, 0.3f, 3.f);
  c[5]  = std::clamp(shader_injection.xegtao_final_power, 0.5f, 5.f);
  c[6]  = std::clamp(shader_injection.xegtao_sample_distribution, 1.f, 3.f);
  c[7]  = std::clamp(shader_injection.xegtao_bitmask_thickness, 0.01f, 2.f);
  c[8]  = std::clamp(shader_injection.xegtao_depth_mip_offset, 0.f, 30.f);
  c[9]  = denoise_passes == 0u ? 10000.f : std::max(0.01f, shader_injection.xegtao_denoise_blur_beta);
  c[10] = (denoise_passes == 0u && g_xegtao_jitter_toggle < 0.5f)
      ? 0.f : (float)((data ? data->frame_index : 0u) % 64u);
  c[11] = shader_injection.xegtao_debug_view;
  c[12] = denoise_last_pass ? 1.f : 0.f;
  // Normal input: use game MRT normals when available, depth fallback otherwise.
  c[13] = g_xegtao_normal_input_mode;
  c[14] = (data && data->captured_mrt_normal_srv.handle != 0u) ? 1.f : 0.f;
  c[15] = g_xegtao_normal_influence;
  c[16] = g_xegtao_normal_depth_blend;
  c[17] = g_xegtao_normal_sharpness;
  c[18] = g_xegtao_normal_edge_rejection;
  c[19] = g_xegtao_normal_z_preservation;
  c[20] = g_xegtao_normal_detail_response;
  c[21] = g_xegtao_normal_max_darkening;
  c[22] = g_xegtao_normal_darkening_mode;
  c[23] = g_xegtao_normal_transform_mode;
  c[24] = shader_injection.xegtao_fix_experimental;  // bitmask experimental fix selector (0-5)
  // ── GI parameters (IS-FAST repurpose) ──
  // isfast_passes (c[25]) = g_gi_enabled
  c[25] = (ssgi_enabled_override >= 0.f) ? ssgi_enabled_override : shader_injection.ssgi_enabled; // GI enable
  // isfast_samples (c[26]) = g_gi_light_exposure
  c[26] = std::clamp(g_ssgi_light_exposure, 0.001f, 10.f);    // HDR light buffer exposure
  // isfast_radius (c[27]) = g_gi_power
  c[27] = std::clamp(shader_injection.ssgi_gi_power, 0.5f, 5.f);  // GI power
  // isfast_edge_sensitivity (c[28]) = g_gi_intensity
  c[28] = std::clamp(shader_injection.ssgi_intensity, 0.f, 5.f);  // GI intensity
  // isfast_spatial_sigma (c[29]) = g_gi_saturation
  c[29] = std::clamp(shader_injection.ssgi_saturation, 0.f, 2.f); // GI saturation
  // isfast_hybrid_blend (c[30]) = g_gi_multibounce
  c[30] = shader_injection.ssgi_multibounce;                       // multi-bounce (0/1)
  c[31] = std::clamp(shader_injection.ssgi_multibounce_strength, 0.f, 10.f);  // feedback strength
  c[32] = std::clamp(shader_injection.ssgi_multibounce_saturation, 0.f, 2.f); // feedback saturation
  c[33] = shader_injection.ssgi_debug_view;                         // SSGI debug view (for shader-side activity viz)
  c[34] = g_isfast_enabled;                                          // IS-FAST enable (0/1)
  c[35] = std::clamp(g_isfast_strength, 0.f, 1.f);                   // IS-FAST noise strength
  c[36] = (data && data->isfast_texture_loaded) ? 1.f : 0.f;         // IS-FAST texture loaded flag
  c[37] = shader_injection.ssgi_adaptive_mode;                       // 0=GI color, 1=albedo
  c[38] = std::clamp(shader_injection.ssgi_adaptive_luma_strength, 0.f, 5.f); // 0=off
  c[39] = std::clamp(shader_injection.ssgi_adaptive_luma_blend, 0.f, 1.f);
  c[40] = std::clamp(g_isfast_spatial_scale, 0.25f, 4.f);          // IS-FAST spatial scale
  c[41] = std::clamp(g_isfast_temporal_speed, 0.f, 5.f);           // IS-FAST temporal speed
  c[42] = std::clamp(g_isfast_seed_offset, 0.f, 64.f);             // IS-FAST seed offset
  // c[43-47] reserved for future use
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
  if (g_cpuopt_ensure_pipelines < 0.5f) {
    DestroyXeGTAODescriptorTables(dev, &d->prefilter_tables);
    DestroyXeGTAODescriptorTables(dev, &d->main_tables);
    DestroyXeGTAODescriptorTables(dev, &d->denoise_tables);
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

  // Match kai's EnsureXeGTAOLayout: separate descriptor tables, each with binding=0,
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
    push_constants_range.count = 48;
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

  if (!make_layout(1u, kXeGTAODepthMipLevels, &d->prefilter_layout)) return false;
  // Main: 4 SRVs (depth MIPs, MRT normal, light buffer, IS-FAST noise) + 4 UAVs (AO, edges, GI, debug)
  if (!make_layout(4u, 4u, &d->main_layout)) return false;
  // Denoise: 3 SRVs (AO, edges, raw GI) + 2 UAVs (denoised AO, denoised GI)
  if (!make_layout(3u, 2u, &d->denoise_layout)) return false;
  // Multi-bounce accumulate: 2 SRVs (color, previous GI) + 1 UAV (accumulated)
  if (!make_layout(2u, 1u, &d->multibounce_layout)) return false;

  if (!d->prefilter_pipeline.handle) mkcs(__xegtao_prefilter, "main", d->prefilter_layout, &d->prefilter_pipeline);
  if (!d->main_low_pipeline.handle)      mkcs(__xegtao_main_low, "main", d->main_layout, &d->main_low_pipeline);
  if (!d->main_medium_pipeline.handle)   mkcs(__xegtao_main_medium, "main", d->main_layout, &d->main_medium_pipeline);
  if (!d->main_high_pipeline.handle)     mkcs(__xegtao_main_high, "main", d->main_layout, &d->main_high_pipeline);
  if (!d->main_ultra_pipeline.handle)    mkcs(__xegtao_main_ultra, "main", d->main_layout, &d->main_ultra_pipeline);
  if (!d->denoise_pipeline.handle)       mkcs(__xegtao_denoise_pass, "main", d->denoise_layout, &d->denoise_pipeline);
  if (!d->denoise_last_pipeline.handle)  mkcs(__xegtao_denoise_last, "main", d->denoise_layout, &d->denoise_last_pipeline);
  if (!d->multibounce_pipeline.handle)   mkcs(__xegtao_multibounce_accumulate, "main", d->multibounce_layout, &d->multibounce_pipeline);

  // ── SSGI is now integrated into the main pass (visibility bitmask AO+GI). ──
  // No separate SSGI pipeline needed — main_layout handles both AO and GI outputs.
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

  d->isfast_texture_loaded = true;
  if (g_isfast_debug_logging > 0.5f)
    reshade::log::message(reshade::log::level::info,
      "[IS-FAST] Texture loaded: 128x128x32 RG8_UNORM — noise source: TEXTURE");
  return true;
}

// ── Dispatch ──

static bool RunXeGTAO(reshade::api::command_list* cl, DeviceData* d) {
  if (!d->captured_depth_srv.handle) return false;

  // ── Frame skips (independent per component) ──
  auto skip_this_frame = [&](float setting) -> bool {
    if (setting <= 0.5f) return false;
    uint64_t n = (uint64_t)setting + 1u;
    return (d->frame_index % n) != 0u;
  };
  bool skip_xegtao      = skip_this_frame(g_xegtao_frame_skip);       // skips entire dispatch
  bool skip_ssgi         = skip_this_frame(g_ssgi_frame_skip);         // AO runs, GI off
  bool skip_multibounce  = skip_this_frame(g_multibounce_frame_skip);  // accumulate skipped

  if (skip_xegtao) return true;  // skip everything, no work done

  float ssgi_enabled_this_frame = shader_injection.ssgi_enabled;
  if (skip_ssgi) ssgi_enabled_this_frame = 0.f;
  auto* dev = cl->get_device();

  // ── IS-FAST noise texture (load once) ──
  if (g_isfast_enabled > 0.5f) LoadISFASTNoiseTexture(dev, d);

  if (shader_injection.xegtao_debug_logging > 0.5f)
    reshade::log::message(reshade::log::level::info, "[XeGTAO] RunXeGTAO: creating pipelines...");
  if (!CreateComputePipelinesIfNeeded(dev, d)) return false;
  if (shader_injection.xegtao_debug_logging > 0.5f)
    reshade::log::message(reshade::log::level::info, "[XeGTAO] RunXeGTAO: allocating descriptor tables...");
  if (!EnsureXeGTAODescriptorTables(dev, d->prefilter_layout, &d->prefilter_tables)) return false;
  if (!EnsureXeGTAODescriptorTables(dev, d->main_layout, &d->main_tables)) return false;
  if (!EnsureXeGTAODescriptorTables(dev, d->denoise_layout, &d->denoise_tables)) return false;
  if (!EnsureXeGTAODescriptorTables(dev, d->multibounce_layout, &d->multibounce_tables)) return false;

  uint32_t w = d->working_width, h = d->working_height;
  if (w < 64 || h < 64) return false;

  if (shader_injection.xegtao_debug_logging > 0.5f)
    reshade::log::message(reshade::log::level::info,
      (std::string("[XeGTAO] RunXeGTAO: dispatching pass 1 (") +
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
                                XeGTAODescriptorTableSet* tbl,
                                uint32_t count,
                                const reshade::api::descriptor_table_update* updates) {
    std::array<reshade::api::descriptor_table_update, kXeGtaoDescriptorTableParamCount> u = {};
    for (uint32_t i = 0; i < count; ++i) { u[i] = updates[i]; u[i].table = (*tbl)[i]; }
    dev->update_descriptor_tables(count, u.data());
    std::array<reshade::api::descriptor_table, kXeGtaoDescriptorTableParamCount> b = {};
    for (uint32_t i = 0; i < count; ++i) b[i] = (*tbl)[i];
    cl->bind_descriptor_tables(CS, lo, 0, count, b.data());
  };

  auto bind_pipe = [&](reshade::api::pipeline p) {
    cl->bind_pipeline(AC, p);
  };

  // Pass 1: Prefilter
  if (shader_injection.xegtao_debug_logging > 0.5f)
    reshade::log::message(reshade::log::level::info, "[XeGTAO] Pass 1: binding pipeline...");
  bind_pipe(d->prefilter_pipeline);
  if (shader_injection.xegtao_debug_logging > 0.5f)
    reshade::log::message(reshade::log::level::info, "[XeGTAO] Pass 1: updating descriptors...");
  {
    reshade::api::descriptor_table_update u[4] = {
      {{},0,0,1,reshade::api::descriptor_type::sampler,&d->point_clamp_sampler},
      {{},0,0,1,reshade::api::descriptor_type::constant_buffer,&d->captured_scene_cbv_view},
      {{},0,0,1,reshade::api::descriptor_type::texture_shader_resource_view,&d->captured_depth_srv},
      {{},0,0,kXeGTAODepthMipLevels,reshade::api::descriptor_type::texture_unordered_access_view,d->depth_mips_uavs.data()},
    };
    apply_descriptors(d->prefilter_layout, &d->prefilter_tables, 4, u);
    auto pc = BuildXeGTAOPushConstants(d, false);
    cl->push_constants(CS, d->prefilter_layout, kXeGtaoPushConstantsLayoutParam, 0, 48, pc.data());
  }
  cl->dispatch((w + 15) / 16, (h + 15) / 16, 1);
  bar(d->depth_mips_texture, UA, SR);
  if (shader_injection.xegtao_debug_logging > 0.5f)
    reshade::log::message(reshade::log::level::info, "[XeGTAO] Pass 1 (prefilter) done.");

  // ── Multi-bounce accumulate (HDR light buffer + previous GI) ──
  // Runs BEFORE main pass to create an HDR accumulated light buffer.
  {
    bool mb_enabled  = shader_injection.ssgi_multibounce > 0.5f;
    bool mb_gi_ready = d->ssgi_denoised_valid;
    bool mb_pipe_ok  = d->multibounce_pipeline.handle != 0u;
    bool mb_color_ok = d->captured_color_srv.handle != 0u;
    bool mb_prev_ok  = d->ssgi_denoised_srv.handle != 0u;
    bool mb_uav_ok   = d->multibounce_uav.handle != 0u;

    if (shader_injection.ssgi_debug_logging > 0.5f) {
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
          ? d->ssgi_denoised_srv : d->fallback_srv;
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
      cl->push_constants(CS, d->multibounce_layout, kXeGtaoPushConstantsLayoutParam, 0, 48,
                         BuildXeGTAOPushConstants(d, false).data());
      cl->dispatch((w + 7) / 8, (h + 7) / 8, 1);
      bar(d->multibounce_texture, UA, SR);
      if (shader_injection.ssgi_debug_logging > 0.5f)
        reshade::log::message(reshade::log::level::info, "[SSGI] MultiBounce: accumulate dispatched.");
    } else if (mb_enabled && !mb_gi_ready) {
      if (shader_injection.ssgi_debug_logging > 0.5f)
        reshade::log::message(reshade::log::level::info,
            "[SSGI] MultiBounce: SKIPPED (denoised GI not valid yet — first frame or XeGTAO never ran).");
    } else if (mb_enabled && !mb_pipe_ok) {
      if (shader_injection.ssgi_debug_logging > 0.5f)
        reshade::log::message(reshade::log::level::warning,
            "[SSGI] MultiBounce: SKIPPED (accumulate pipeline missing).");
    }
  }

  // Pass 2: Main
  reshade::api::pipeline mp = d->main_high_pipeline;
  { int q = (int)shader_injection.xegtao_quality_level;
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
    if (shader_injection.ssgi_multibounce > 0.5f && d->ssgi_denoised_valid
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
    if (shader_injection.ssgi_debug_logging > 0.5f) {
      std::string msg = "[SSGI] Main lightBuf=";
      msg += lb_source;
      msg += " mbEnable="; msg += (shader_injection.ssgi_multibounce > 0.5f) ? "1" : "0";
      msg += " mbReady=";  msg += d->ssgi_denoised_valid ? "1" : "0";
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
        d->ssgi_output_uav.handle ? d->ssgi_output_uav : d->fallback_uav,
        d->debug_uav.handle ? d->debug_uav : d->fallback_uav
    };
    reshade::api::descriptor_table_update u[4] = {
      {{},0,0,1,reshade::api::descriptor_type::sampler,&d->point_clamp_sampler},
      {{},0,0,1,reshade::api::descriptor_type::constant_buffer,&d->captured_scene_cbv_view},
      {{},0,0,4,reshade::api::descriptor_type::texture_shader_resource_view,main_srvs},
      {{},0,0,4,reshade::api::descriptor_type::texture_unordered_access_view,main_uavs},
    };
    apply_descriptors(d->main_layout, &d->main_tables, 4, u);
    auto pc = BuildXeGTAOPushConstants(d, false, ssgi_enabled_this_frame);
    cl->push_constants(CS, d->main_layout, kXeGtaoPushConstantsLayoutParam, 0, 48, pc.data());
  }
  cl->dispatch((w + 7) / 8, (h + 7) / 8, 1);
  bar(d->ao_term_a_texture, UA, SR);
  bar(d->edges_texture, UA, SR);
  bar(d->ssgi_output_texture, UA, SR);  // GI output ready for denoise
  bar(d->debug_texture, UA, SR);         // Debug output ready for read
  if (shader_injection.xegtao_debug_logging > 0.5f)
    reshade::log::message(reshade::log::level::info, "[XeGTAO] Pass 2 (main) done.");
  if (shader_injection.ssgi_debug_logging > 0.5f) {
    std::string msg = "[SSGI] Main pass: enableGI=";
    msg += (shader_injection.ssgi_enabled > 0.5f) ? "1" : "0";
    msg += " intensity=" + std::to_string(shader_injection.ssgi_intensity);
    msg += " multibounce=" + std::to_string((int)shader_injection.ssgi_multibounce);
    msg += " lightBuf=";
    if (shader_injection.ssgi_multibounce > 0.5f && d->ssgi_denoised_valid)
      msg += "accumulated";
    else
      msg += (d->captured_color_srv.handle) ? "colorSRV" : (d->captured_light_buffer_srv.handle ? "backbuf" : "MISSING");
    reshade::log::message(reshade::log::level::info, msg.c_str());
  }

  // Pass 3: Denoise (ping-pong) — always run at least one pass to apply
  // the XE_GTAO_OCCLUSION_TERM_SCALE multiply-back (1.5x) in XeGTAO_Output.
  // When dpc==0 the DenoiseBlurBeta=10000 effectively disables blur.
  int dpc = (int)shader_injection.xegtao_denoise_passes;
  if (dpc < 1) dpc = 1;
  {
    bool use_a = true;
    for (int p = 0; p < dpc; ++p) {
      bool last = (p == dpc - 1);
      reshade::api::resource_view src, dst_uav;
      reshade::api::resource dst_tex;
      if (use_a) { src = d->ao_term_a_srv; dst_uav = d->ao_term_b_uav; dst_tex = d->ao_term_b_texture; }
      else       { src = d->ao_term_b_srv; dst_uav = d->ao_term_a_uav; dst_tex = d->ao_term_a_texture; }
      bind_pipe(last ? d->denoise_last_pipeline : d->denoise_pipeline);
      reshade::api::resource_view sv[3] = {src, d->edges_srv,
          d->ssgi_output_srv.handle ? d->ssgi_output_srv : d->fallback_srv};  // raw GI
      reshade::api::resource_view dn_uavs[2] = {dst_uav,
          d->ssgi_denoised_uav.handle ? d->ssgi_denoised_uav : d->fallback_uav};  // denoised GI
      reshade::api::descriptor_table_update u[4] = {
        {{},0,0,1,reshade::api::descriptor_type::sampler,&d->point_clamp_sampler},
        {{},0,0,1,reshade::api::descriptor_type::constant_buffer,&d->captured_scene_cbv_view},
        {{},0,0,3,reshade::api::descriptor_type::texture_shader_resource_view,sv},
        {{},0,0,2,reshade::api::descriptor_type::texture_unordered_access_view,dn_uavs},
      };
      apply_descriptors(d->denoise_layout, &d->denoise_tables, 4, u);
      auto pc = BuildXeGTAOPushConstants(d, last);
      cl->push_constants(CS, d->denoise_layout, kXeGtaoPushConstantsLayoutParam, 0, 48, pc.data());
      cl->dispatch((w + 7) / 8, (h + 7) / 8, 1);
      bar(dst_tex, UA, SR);
      use_a = !use_a;
    }
  }
  bar(d->ssgi_denoised_texture, UA, SR);  // Denoised GI ready for t23 read
  if (!d->ssgi_denoised_valid) {
    d->ssgi_denoised_valid = true;            // Multi-bounce feedback active next frame
    if (shader_injection.ssgi_debug_logging > 0.5f)
      reshade::log::message(reshade::log::level::info,
          "[SSGI] MultiBounce: denoised GI now valid — accumulate will run next frame.");
  }
  if (shader_injection.xegtao_debug_logging > 0.5f)
    reshade::log::message(reshade::log::level::info, "[XeGTAO] All passes complete.");
  return true;
}

}  // namespace

extern "C" __declspec(dllexport) constexpr const char* NAME = "Generic Vanilla+";
extern "C" __declspec(dllexport) constexpr const char* DESCRIPTION =
    "Generic vanilla-plus shader injector";

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
