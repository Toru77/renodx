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
  .xegtao_thin_occluder_comp = 0.5f,
  .xegtao_depth_mip_offset = 3.30f,
  .xegtao_denoise_blur_beta = 20.0f,
  .xegtao_internal_resolution = 100.f,
  .xegtao_debug_view = 0.f,
  .xegtao_debug_logging = 0.f,
  .xegtao_dedicated_bound = 0.f,
  .xegtao_fix_experimental = 0.f,
  .xegtao_ssgi_bound = 0.f,
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

// ── SSGI (Visibility Bitmask Indirect Diffuse) globals ──
static float g_ssgi_enabled              = 0.f;
static float g_ssgi_radius               = 2.0f;
static float g_ssgi_steps                = 12.f;
static float g_ssgi_directions           = 4.f;
static float g_ssgi_thickness            = 0.2f;
static float g_ssgi_intensity            = 1.0f;
static float g_ssgi_step_distribution    = 1.f;  // 0=constant, 1=exponential
static float g_ssgi_resolution           = 0.f;   // 0=full, 1=half
static float g_ssgi_multibounce          = 1.f;

// ── SSGI's own XeGTAO settings (override standalone when SSGI is on) ──
static float g_ssgi_xegtao_quality_level     = 2.f;
static float g_ssgi_xegtao_denoise_passes    = 1.f;
static float g_ssgi_xegtao_radius            = 0.5f;
static float g_ssgi_xegtao_falloff_range     = 0.615f;
static float g_ssgi_xegtao_radius_multiplier = 1.5f;
static float g_ssgi_xegtao_final_power       = 2.0f;
static float g_ssgi_xegtao_sample_distribution = 1.5f;
static float g_ssgi_xegtao_thin_occluder_comp = 0.5f;
static float g_ssgi_xegtao_depth_mip_offset  = 3.30f;
static float g_ssgi_xegtao_denoise_blur_beta = 20.0f;

// ── CPU optimization toggles (each independently testable) ──
// ── CPU optimization toggles ──
static float g_cpuopt_xegtao_frame_skip   = 0.f;
static float g_cpuopt_ssgi_frame_skip     = 0.f;
static float g_cpuopt_deferred_dispatch   = 0.f;  // dispatch XeGTAO/SSGI in OnPresent, not inline
static float g_cpuopt_ensure_pipelines    = 0.f;  // kai-style: don't destroy/recreate pipelines every frame
static float g_cpuopt_suppress_capture    = 0.f;  // skip descriptor capture events during our dispatches
static float g_cpuopt_batched_state       = 0.f;  // save/restore cmd list state once for all XeGTAO passes

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

  // ── SSGI resources ──
  reshade::api::resource ssgi_output_texture = {};
  reshade::api::resource_view ssgi_output_srv = {};
  reshade::api::resource_view ssgi_output_uav = {};
  reshade::api::resource ssgi_denoised_texture = {};
  reshade::api::resource_view ssgi_denoised_srv = {};
  reshade::api::resource_view ssgi_denoised_uav = {};
  reshade::api::resource captured_light_buffer_texture = {};
  reshade::api::resource_view captured_light_buffer_srv = {};
  reshade::api::pipeline_layout ssgi_layout = {};
  reshade::api::pipeline_layout ssgi_denoise_layout = {};
  reshade::api::pipeline ssgi_pipeline = {};
  reshade::api::pipeline ssgi_denoise_pipeline = {};
  XeGTAODescriptorTableSet ssgi_tables = {};
  XeGTAODescriptorTableSet ssgi_denoise_tables = {};
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
static void RunSSGI(reshade::api::command_list* cmd_list, DeviceData* data);
static bool OnBeforeLightingShaderDraw(reshade::api::command_list* cmd_list);
static bool OnBeforeSsaoShaderDraw(reshade::api::command_list* cmd_list);
static void OnPushDescriptorsCapture(reshade::api::command_list* cmd_list,
    reshade::api::shader_stage stages, reshade::api::pipeline_layout layout,
    uint32_t param_index, const reshade::api::descriptor_table_update& update);

// ═══════════ Custom shaders ═══════════

renodx::mods::shader::CustomShaders custom_shaders = {
    CustomShaderEntry(0x954D3D6D),
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
      .default_value = 1.f, .label = "Shadow Type", .section = "Character Shadowing",
      .labels = {"Camera View", "World View", "Combined"},
      .is_enabled = []() { return shader_injection.char_shadow_mode == 2.f; },
    },
    new renodx::utils::settings::Setting{
      .key = "CharShadowCameraStrength", .binding = &shader_injection.char_shadow_camera_strength,
      .default_value = 100.f, .label = "Camera Strenght", .section = "Character Shadowing",
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
      .default_value = 1.f, .label = "Bend SSS", .section = "Environment Screen Space Shadows",
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
      .default_value = 0.f, .label = "XeGTAO Mode", .section = "XeGTAO",
      .tooltip = "Off = vanilla game AO. On = XeGTAO compute-shader AO.",
      .labels = {"Off (Vanilla AO)", "On (XeGTAO)"},
    },
    new renodx::utils::settings::Setting{
      .key = "XeGTAOQuality", .binding = &shader_injection.xegtao_quality_level,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 2.f, .label = "Quality Level", .section = "XeGTAO",
      .labels = {"Low", "Medium", "High", "Ultra"},
      .is_enabled = []() { return shader_injection.xegtao_mode > 0.5f && g_ssgi_enabled < 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "XeGTAODenoisePasses", .binding = &shader_injection.xegtao_denoise_passes,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 1.f, .label = "Denoise Passes", .section = "XeGTAO",
      .labels = {"Off", "Sharp (1)", "Medium (2)", "Soft (3)"},
      .is_enabled = []() { return shader_injection.xegtao_mode > 0.5f && g_ssgi_enabled < 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "XeGTAORadius", .binding = &shader_injection.xegtao_radius,
      .default_value = 0.5f, .label = "Radius", .section = "XeGTAO",
      .min = 0.01f, .max = 5.0f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.xegtao_mode > 0.5f && g_ssgi_enabled < 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "XeGTAOFalloffRange", .binding = &shader_injection.xegtao_falloff_range,
      .default_value = 0.615f, .label = "Falloff Range", .section = "XeGTAO",
      .min = 0.0f, .max = 1.0f, .format = "%.3f",
      .is_enabled = []() { return shader_injection.xegtao_mode > 0.5f && g_ssgi_enabled < 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "XeGTAORadiusMultiplier", .binding = &shader_injection.xegtao_radius_multiplier,
      .default_value = 1.457f, .label = "Radius Multiplier", .section = "XeGTAO",
      .min = 0.3f, .max = 3.0f, .format = "%.3f",
      .is_enabled = []() { return shader_injection.xegtao_mode > 0.5f && g_ssgi_enabled < 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "XeGTAOFinalPower", .binding = &shader_injection.xegtao_final_power,
      .default_value = 2.2f, .label = "Final Power", .section = "XeGTAO",
      .min = 0.5f, .max = 5.0f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.xegtao_mode > 0.5f && g_ssgi_enabled < 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "XeGTAOSampleDistribution", .binding = &shader_injection.xegtao_sample_distribution,
      .default_value = 2.0f, .label = "Sample Distribution", .section = "XeGTAO",
      .min = 1.0f, .max = 3.0f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.xegtao_mode > 0.5f && g_ssgi_enabled < 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "XeGTAOThinOccluderComp", .binding = &shader_injection.xegtao_thin_occluder_comp,
      .default_value = 0.0f, .label = "Thin Occluder Comp", .section = "XeGTAO",
      .min = 0.0f, .max = 0.7f, .format = "%.3f",
      .is_enabled = []() { return shader_injection.xegtao_mode > 0.5f && g_ssgi_enabled < 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "XeGTAODepthMIPOffset", .binding = &shader_injection.xegtao_depth_mip_offset,
      .default_value = 3.30f, .label = "Depth MIP Offset", .section = "XeGTAO",
      .min = 2.0f, .max = 6.0f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.xegtao_mode > 0.5f && g_ssgi_enabled < 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "XeGTAODenoiseBlurBeta", .binding = &shader_injection.xegtao_denoise_blur_beta,
      .default_value = 1.2f, .label = "Denoise Blur Beta", .section = "XeGTAO",
      .min = 0.5f, .max = 20.0f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.xegtao_mode > 0.5f && shader_injection.xegtao_denoise_passes > 0.f && g_ssgi_enabled < 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "XeGTAOInternalResolution", .binding = &shader_injection.xegtao_internal_resolution,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 75.f, .label = "Internal Resolution", .section = "XeGTAO",
      .labels = {"50%", "75%", "100%"},
      .is_enabled = []() { return shader_injection.xegtao_mode > 0.5f && g_ssgi_enabled < 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "XeGTAONormalInputMode", .binding = &g_xegtao_normal_input_mode,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 1.f, .label = "MRT Normal Input", .section = "XeGTAO",
      .tooltip = "Off = depth normals only. On = use game g-buffer normals.",
      .labels = {"Off (Depth)", "On (MRT)"},
      .is_enabled = []() { return shader_injection.xegtao_mode > 0.5f && g_ssgi_enabled < 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "XeGTAONormalInfluence", .binding = &g_xegtao_normal_influence,
      .default_value = 1.f, .label = "Normal Influence", .section = "XeGTAO",
      .min = 0.f, .max = 2.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.xegtao_mode > 0.5f && g_xegtao_normal_input_mode > 0.5f && g_ssgi_enabled < 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "XeGTAONormalDepthBlend", .binding = &g_xegtao_normal_depth_blend,
      .default_value = 0.5f, .label = "Normal Depth Blend", .section = "XeGTAO",
      .min = 0.f, .max = 1.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.xegtao_mode > 0.5f && g_xegtao_normal_input_mode > 0.5f && g_ssgi_enabled < 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "XeGTAONormalSharpness", .binding = &g_xegtao_normal_sharpness,
      .default_value = 1.f, .label = "Normal Sharpness", .section = "XeGTAO",
      .min = 0.01f, .max = 4.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.xegtao_mode > 0.5f && g_xegtao_normal_input_mode > 0.5f && g_ssgi_enabled < 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "XeGTAONormalEdgeRejection", .binding = &g_xegtao_normal_edge_rejection,
      .default_value = 0.5f, .label = "Normal Edge Rejection", .section = "XeGTAO",
      .min = 0.f, .max = 2.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.xegtao_mode > 0.5f && g_xegtao_normal_input_mode > 0.5f && g_ssgi_enabled < 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "XeGTAONormalZPreservation", .binding = &g_xegtao_normal_z_preservation,
      .default_value = 1.f, .label = "Normal Z Preservation", .section = "XeGTAO",
      .min = 0.f, .max = 2.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.xegtao_mode > 0.5f && g_xegtao_normal_input_mode > 0.5f && g_ssgi_enabled < 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "XeGTAONormalDetailResponse", .binding = &g_xegtao_normal_detail_response,
      .default_value = 0.35f, .label = "Normal Detail Response", .section = "XeGTAO",
      .min = 0.01f, .max = 1.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.xegtao_mode > 0.5f && g_xegtao_normal_input_mode > 0.5f && g_ssgi_enabled < 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "XeGTAONormalMaxDarkening", .binding = &g_xegtao_normal_max_darkening,
      .default_value = 0.8f, .label = "Normal Max Darkening", .section = "XeGTAO",
      .min = 0.f, .max = 1.f, .format = "%.2f",
      .is_enabled = []() { return shader_injection.xegtao_mode > 0.5f && g_xegtao_normal_input_mode > 0.5f && g_ssgi_enabled < 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "XeGTAONormalDarkeningMode", .binding = &g_xegtao_normal_darkening_mode,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 0.f, .label = "Normal Darkening Mode", .section = "XeGTAO",
      .labels = {"Multiply", "Replace"},
      .is_enabled = []() { return shader_injection.xegtao_mode > 0.5f && g_xegtao_normal_input_mode > 0.5f && g_ssgi_enabled < 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "XeGTAONormalTransformMode", .binding = &g_xegtao_normal_transform_mode,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 0.f, .label = "Normal Transform Mode", .section = "XeGTAO",
      .tooltip = "How to transform MRT normals to view space. Try alternatives if normals look wrong at some camera angles.",
      .labels = {"view_g (default)", "viewInv_g", "Passthrough"},
      .is_enabled = []() { return shader_injection.xegtao_mode > 0.5f && g_xegtao_normal_input_mode > 0.5f && g_ssgi_enabled < 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "XeGTAODebugView", .binding = &shader_injection.xegtao_debug_view,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 0.f, .label = "Debug View", .section = "XeGTAO",
      .labels = {"Off", "AO Only", "XeGTAO raw .a", "XeGTAO RGBA", "Vanilla SSAO", "Depth"},
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
      .tooltip = "Reserved for future A/B testing. Leave at Off.",
      .labels = {"Off", "Reserved 1", "Reserved 2", "Reserved 3", "Reserved 4", "Reserved 5"},
    },
    // —— SSGI (Screen Space Global Illumination) ——
    new renodx::utils::settings::Setting{
      .key = "SSGIEnable", .binding = &g_ssgi_enabled,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 0.f, .label = "SSGI Enable", .section = "SSGI",
      .tooltip = "Visibility bitmask indirect diffuse from screen-space GI.",
      .labels = {"Off", "On"},
    },
    new renodx::utils::settings::Setting{
      .key = "SSGIRadius", .binding = &g_ssgi_radius,
      .default_value = 2.0f, .label = "Radius", .section = "SSGI",
      .min = 0.1f, .max = 10.0f, .format = "%.1f",
      .is_enabled = []() { return g_ssgi_enabled > 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "SSGISteps", .binding = &g_ssgi_steps,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 12.f, .label = "Steps", .section = "SSGI",
      .labels = {"4", "8", "12", "16", "20", "24", "28", "32"},
      .is_enabled = []() { return g_ssgi_enabled > 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "SSGIDirections", .binding = &g_ssgi_directions,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 4.f, .label = "Directions", .section = "SSGI",
      .labels = {"1", "2", "3", "4", "5", "6"},
      .is_enabled = []() { return g_ssgi_enabled > 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "SSGIThickness", .binding = &g_ssgi_thickness,
      .default_value = 0.2f, .label = "Thickness", .section = "SSGI",
      .min = 0.01f, .max = 2.0f, .format = "%.2f",
      .is_enabled = []() { return g_ssgi_enabled > 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "SSGIIntensity", .binding = &g_ssgi_intensity,
      .default_value = 1.0f, .label = "Intensity", .section = "SSGI",
      .min = 0.0f, .max = 5.0f, .format = "%.2f",
      .is_enabled = []() { return g_ssgi_enabled > 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "SSGIStepDistribution", .binding = &g_ssgi_step_distribution,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 1.f, .label = "Step Distribution", .section = "SSGI",
      .labels = {"Constant", "Exponential"},
      .is_enabled = []() { return g_ssgi_enabled > 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "SSGIResolution", .binding = &g_ssgi_resolution,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 0.f, .label = "Resolution", .section = "SSGI",
      .labels = {"Full", "Half"},
      .is_enabled = []() { return g_ssgi_enabled > 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "SSGIMultiBounce", .binding = &g_ssgi_multibounce,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 1.f, .label = "Multi-Bounce", .section = "SSGI",
      .tooltip = "Feed SSGI output back for multiple light bounces.",
      .labels = {"Off", "On"},
      .is_enabled = []() { return g_ssgi_enabled > 0.5f; },
    },
    // —— XeGTAO settings under SSGI (override standalone when SSGI is on) ——
    new renodx::utils::settings::Setting{
      .key = "SSGIXeGTAOQuality", .binding = &g_ssgi_xegtao_quality_level,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 2.f, .label = "XeGTAO Quality", .section = "SSGI",
      .labels = {"Low", "Medium", "High", "Ultra"},
      .is_enabled = []() { return g_ssgi_enabled > 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "SSGIXeGTAODenoisePasses", .binding = &g_ssgi_xegtao_denoise_passes,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 1.f, .label = "XeGTAO Denoise", .section = "SSGI",
      .labels = {"Off", "Sharp (1)", "Medium (2)", "Soft (3)"},
      .is_enabled = []() { return g_ssgi_enabled > 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "SSGIXeGTAORadius", .binding = &g_ssgi_xegtao_radius,
      .default_value = 0.5f, .label = "XeGTAO Radius", .section = "SSGI",
      .min = 0.01f, .max = 5.0f, .format = "%.2f",
      .is_enabled = []() { return g_ssgi_enabled > 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "SSGIXeGTAOFalloff", .binding = &g_ssgi_xegtao_falloff_range,
      .default_value = 0.615f, .label = "XeGTAO Falloff", .section = "SSGI",
      .min = 0.0f, .max = 1.0f, .format = "%.3f",
      .is_enabled = []() { return g_ssgi_enabled > 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "SSGIXeGTAORadiusMult", .binding = &g_ssgi_xegtao_radius_multiplier,
      .default_value = 1.5f, .label = "XeGTAO Radius Mult", .section = "SSGI",
      .min = 0.3f, .max = 3.0f, .format = "%.3f",
      .is_enabled = []() { return g_ssgi_enabled > 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "SSGIXeGTAOFinalPower", .binding = &g_ssgi_xegtao_final_power,
      .default_value = 2.0f, .label = "XeGTAO Final Power", .section = "SSGI",
      .min = 0.5f, .max = 5.0f, .format = "%.2f",
      .is_enabled = []() { return g_ssgi_enabled > 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "SSGIXeGTAOSampleDist", .binding = &g_ssgi_xegtao_sample_distribution,
      .default_value = 1.5f, .label = "XeGTAO Sample Dist", .section = "SSGI",
      .min = 1.0f, .max = 3.0f, .format = "%.2f",
      .is_enabled = []() { return g_ssgi_enabled > 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "SSGIXeGTAOThinOccluder", .binding = &g_ssgi_xegtao_thin_occluder_comp,
      .default_value = 0.5f, .label = "XeGTAO Thin Occluder", .section = "SSGI",
      .min = 0.0f, .max = 0.7f, .format = "%.3f",
      .is_enabled = []() { return g_ssgi_enabled > 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "SSGIXeGTAODepthMIP", .binding = &g_ssgi_xegtao_depth_mip_offset,
      .default_value = 3.30f, .label = "XeGTAO Depth MIP", .section = "SSGI",
      .min = 2.0f, .max = 6.0f, .format = "%.2f",
      .is_enabled = []() { return g_ssgi_enabled > 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "SSGIXeGTAODenoiseBlur", .binding = &g_ssgi_xegtao_denoise_blur_beta,
      .default_value = 20.0f, .label = "XeGTAO Denoise Blur", .section = "SSGI",
      .min = 0.5f, .max = 20.0f, .format = "%.2f",
      .is_enabled = []() { return g_ssgi_enabled > 0.5f; },
    },
    // —— CPU Optimizations ——
    new renodx::utils::settings::Setting{
      .key = "CPUOptXeGTAOFrameSkip", .binding = &g_cpuopt_xegtao_frame_skip,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 0.f, .label = "XeGTAO Frame Skip", .section = "CPU Opt",
      .labels = {"Off", "2 Frames", "3 Frames", "4 Frames"},
    },
    new renodx::utils::settings::Setting{
      .key = "CPUOptSSGIFrameSkip", .binding = &g_cpuopt_ssgi_frame_skip,
      .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 0.f, .label = "SSGI Frame Skip", .section = "CPU Opt",
      .labels = {"Off", "2 Frames", "3 Frames", "4 Frames"},
    },
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
      .default_value = 0.f, .label = "Ensure Pipelines", .section = "CPU Opt",
      .tooltip = "Don't destroy/recreate pipelines every frame (kai-style).",
      .labels = {"Off", "On"},
    },
    new renodx::utils::settings::Setting{
      .key = "CPUOptSuppressCapture", .binding = &g_cpuopt_suppress_capture,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 0.f, .label = "Suppress Capture", .section = "CPU Opt",
      .tooltip = "Skip descriptor capture events during our own compute dispatches.",
      .labels = {"Off", "On"},
    },
    new renodx::utils::settings::Setting{
      .key = "CPUOptBatchedState", .binding = &g_cpuopt_batched_state,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 0.f, .label = "Batched State", .section = "CPU Opt",
      .tooltip = "Save/restore command list state once for all XeGTAO passes.",
      .labels = {"Off", "On"},
    },
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

// ── Global guard: suppress descriptor capture events during our own dispatches. ──
static bool g_xegtao_dispatch_in_progress = false;

static void OnPushDescriptorsCapture(
    reshade::api::command_list* cmd_list,
    reshade::api::shader_stage stages,
    reshade::api::pipeline_layout layout,
    uint32_t param_index,
    const reshade::api::descriptor_table_update& update) {
  if (!cmd_list) return;
  // CPU opt: suppress capture events during our own compute dispatches (kai-style).
  if (g_cpuopt_suppress_capture > 0.5f && g_xegtao_dispatch_in_progress) return;
  auto* device = cmd_list->get_device();
  auto* d = device->get_private_data<DeviceData>();
  if (!d) return;

  // ── Capture depth/SSAO/CBV — unconditional (register-based, kai-style). ──
  if (update.type == reshade::api::descriptor_type::texture_shader_resource_view) {
    auto* views = static_cast<const reshade::api::resource_view*>(update.descriptors);
    if (update.binding == kLightingDepthRegister && update.count >= 1
        && views[0].handle != 0u) {
      d->captured_depth_srv = views[0];
      d->captured_scene_cbv_frame = d->frame_index;
    }
    if (update.binding == kLightingSsaoRegister && update.count >= 1
        && views[0].handle != 0u) {
      d->captured_ssao_srv = views[0];
    }
    if (update.binding == kLightingMrtNormalRegister && update.count >= 1
        && views[0].handle != 0u) {
      d->captured_mrt_normal_srv = views[0];
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
  if (shader_injection.xegtao_mode < 0.5f && g_ssgi_enabled < 0.5f) return;
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

  if (shader_injection.xegtao_mode < 0.5f && g_ssgi_enabled < 0.5f) return;
  // CPU opt: suppress capture events during our own compute dispatches (kai-style).
  if (g_cpuopt_suppress_capture > 0.5f && g_xegtao_dispatch_in_progress) return;

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
  if (shader_injection.xegtao_mode < 0.5f && g_ssgi_enabled < 0.5f) return;
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
  // Use deferred snapshots from lighting draw (kai-style).
  if (!d->deferred_pending || !d->deferred_depth_srv.handle) {
    if (shader_injection.xegtao_debug_logging > 0.5f) {
      reshade::log::message(reshade::log::level::warning,
                            "[XeGTAO] Dispatch skipped: no deferred depth SRV.");
    }
    return;
  }
  if (!d->deferred_scene_cbv_valid
      || (d->frame_index - d->deferred_scene_cbv_frame) > 1u) {
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

  // Save command-list state (unless RunXeGTAO handles it via batched state).
  renodx::utils::state::CommandListState prev = {};
  if (g_cpuopt_batched_state < 0.5f) {
    auto* cs = renodx::utils::state::GetCurrentState(cl);
    if (cs) prev = *cs;
  }

  bool ok = RunXeGTAO(cl, d);

  // Restore only if RunXeGTAO didn't already (batched state).
  if (g_cpuopt_batched_state < 0.5f) {
    cl->bind_pipeline(reshade::api::pipeline_stage::all_compute, reshade::api::pipeline{0u});
    auto* cs = renodx::utils::state::GetCurrentState(cl);
    if (cs) *cs = prev;
  }

  if (shader_injection.xegtao_debug_logging > 0.5f && ok) {
    std::ostringstream msg;
    msg << "[XeGTAO] Dispatch OK (frame=" << d->frame_index
        << ", res=" << d->working_width << "x" << d->working_height << ")";
    reshade::log::message(reshade::log::level::info, msg.str().c_str());
  } else if (shader_injection.xegtao_debug_logging > 0.5f && !ok) {
    reshade::log::message(reshade::log::level::warning, "[XeGTAO] Dispatch failed.");
  }

  // ── SSGI deferred dispatch (runs after XeGTAO, using its depth MIPs) ──
  if (g_ssgi_enabled > 0.5f && d->ssgi_output_srv.handle
      && d->captured_light_buffer_srv.handle) {
    RunSSGI(cl, d);
    cl->bind_pipeline(reshade::api::pipeline_stage::all_compute, reshade::api::pipeline{0u});
  }

  // ── Light buffer capture for SSGI (next frame's input) ──
  if (g_ssgi_enabled > 0.5f && d->captured_light_buffer_texture.handle) {
    auto bb = sc->get_back_buffer(0);
    if (bb.handle) {
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
    }
  }
  shader_injection.xegtao_ssgi_bound = 0.f;  // Reset for next frame's SSAO pass
}

static bool OnBeforeLightingShaderDraw(reshade::api::command_list* cmd_list) {
  // IMPORTANT: returning false would BYPASS the draw (skip it entirely).
  shader_injection.xegtao_dedicated_bound = 0.f;

  if (shader_injection.xegtao_mode < 0.5f && g_ssgi_enabled < 0.5f) return true;
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
      renodx::utils::state::CommandListState prev = {};
      auto* cs = renodx::utils::state::GetCurrentState(cmd_list);
      if (cs) prev = *cs;

      bool ok = RunXeGTAO(cmd_list, dd);

      // Restore only if RunXeGTAO didn't already (batched state).
      if (g_cpuopt_batched_state < 0.5f) {
        cmd_list->bind_pipeline(reshade::api::pipeline_stage::all_compute, reshade::api::pipeline{0u});
        if (cs) *cs = prev;
      }
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

  // ── SSGI dispatch + push t23 ──
  shader_injection.xegtao_ssgi_bound = 0.f;
  if (g_ssgi_enabled > 0.5f
      && dd->ssgi_output_srv.handle) {
    // Inline dispatch (only when NOT deferred): run SSGI, push fresh result.
    // Deferred mode: push previous frame's denoised result from OnPresent.
    if (g_cpuopt_deferred_dispatch < 0.5f) {
      if (dd->captured_light_buffer_srv.handle) {
        RunSSGI(cmd_list, dd);
      }
    }
    if (dd->ssgi_denoised_srv.handle) {
      // Push SSGI result to t23
      cmd_list->push_descriptors(
          reshade::api::shader_stage::pixel,
          reshade::api::pipeline_layout{0},
          0,
          reshade::api::descriptor_table_update{
              {}, kLightingSsgiRegister, 0, 1,
              reshade::api::descriptor_type::texture_shader_resource_view,
              &dd->ssgi_denoised_srv,
          });
      shader_injection.xegtao_ssgi_bound = 1.f;
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
  // Use full captured depth resolution to avoid pixel-mapping artifacts (veil).
  uint32_t w = gw, h = gh;
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

  // ── SSGI resources (at working resolution, or half) ──
  uint32_t ssgi_w = w, ssgi_h = h;
  if (g_ssgi_resolution > 0.5f) { ssgi_w = std::max(64u, w / 2u); ssgi_h = std::max(64u, h / 2u); }
  mk(ssgi_w, ssgi_h, reshade::api::format::r16g16b16a16_float,
     &d->ssgi_output_texture, &d->ssgi_output_srv, &d->ssgi_output_uav);
  mk(ssgi_w, ssgi_h, reshade::api::format::r16g16b16a16_float,
     &d->ssgi_denoised_texture, &d->ssgi_denoised_srv, &d->ssgi_denoised_uav);
  // Light buffer capture at full back-buffer resolution
  mk(gw, gh, reshade::api::format::r16g16b16a16_float,
     &d->captured_light_buffer_texture, &d->captured_light_buffer_srv, nullptr);
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
  // SSGI resources
  dv(d->ssgi_output_srv); dv(d->ssgi_output_uav); dr(d->ssgi_output_texture);
  dv(d->ssgi_denoised_srv); dv(d->ssgi_denoised_uav); dr(d->ssgi_denoised_texture);
  dv(d->captured_light_buffer_srv); dr(d->captured_light_buffer_texture);
  dp(d->ssgi_pipeline); dp(d->ssgi_denoise_pipeline);
  dl(d->ssgi_layout); dl(d->ssgi_denoise_layout);
  DestroyXeGTAODescriptorTables(dev, &d->ssgi_tables);
  DestroyXeGTAODescriptorTables(dev, &d->ssgi_denoise_tables);
  // Do NOT clear captured_depth_srv / captured_scene_cbv —
  // those reference game-owned resources that survive recreation.
  d->resources_created = false;
}

// ── Push constants builder (kai-vanillaplus style) ──

static std::array<float, 32> BuildXeGTAOPushConstants(DeviceData* data, bool denoise_last_pass) {
  std::array<float, 32> c = {};
  const bool ssgi_on = g_ssgi_enabled > 0.5f;
  const uint32_t denoise_passes = ssgi_on
      ? (uint32_t)g_ssgi_xegtao_denoise_passes
      : (uint32_t)shader_injection.xegtao_denoise_passes;
  c[0]  = ssgi_on ? g_ssgi_xegtao_quality_level : shader_injection.xegtao_quality_level;
  c[1]  = (float)denoise_passes;
  c[2]  = std::max(0.001f, ssgi_on ? g_ssgi_xegtao_radius : shader_injection.xegtao_radius);
  c[3]  = std::clamp(ssgi_on ? g_ssgi_xegtao_falloff_range : shader_injection.xegtao_falloff_range, 0.f, 1.f);
  c[4]  = std::clamp(ssgi_on ? g_ssgi_xegtao_radius_multiplier : shader_injection.xegtao_radius_multiplier, 0.3f, 3.f);
  c[5]  = std::clamp(ssgi_on ? g_ssgi_xegtao_final_power : shader_injection.xegtao_final_power, 0.5f, 5.f);
  c[6]  = std::clamp(ssgi_on ? g_ssgi_xegtao_sample_distribution : shader_injection.xegtao_sample_distribution, 1.f, 3.f);
  c[7]  = std::clamp(ssgi_on ? g_ssgi_xegtao_thin_occluder_comp : shader_injection.xegtao_thin_occluder_comp, 0.f, 0.7f);
  c[8]  = std::clamp(ssgi_on ? g_ssgi_xegtao_depth_mip_offset : shader_injection.xegtao_depth_mip_offset, 0.f, 30.f);
  c[9]  = denoise_passes == 0u ? 10000.f : std::max(0.01f,
      ssgi_on ? g_ssgi_xegtao_denoise_blur_beta : shader_injection.xegtao_denoise_blur_beta);
  c[10] = denoise_passes == 0u ? 0.f : (float)((data ? data->frame_index : 0u) % 64u);
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
  c[23] = g_xegtao_normal_transform_mode;  // 0=view_g (default), 1=viewInv_g, 2=passthrough
  c[24] = 0.f;  // copyback_preserve_yzw
  c[25] = 0.f;  // isfast_passes
  c[26] = 0.f;  // isfast_samples
  c[27] = 0.f;  // isfast_radius
  c[28] = 0.f;  // isfast_edge_sensitivity
  c[29] = 0.f;  // isfast_spatial_sigma
  c[30] = 0.f;  // isfast_hybrid_blend
  c[31] = 0.f;  // isfast_noise_available
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
    push_constants_range.count = 32;
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
  if (!make_layout(2u, 2u, &d->main_layout)) return false;
  if (!make_layout(2u, 1u, &d->denoise_layout)) return false;

  if (!d->prefilter_pipeline.handle) mkcs(__xegtao_prefilter, "main", d->prefilter_layout, &d->prefilter_pipeline);
  if (!d->main_low_pipeline.handle)      mkcs(__xegtao_main_low, "main", d->main_layout, &d->main_low_pipeline);
  if (!d->main_medium_pipeline.handle)   mkcs(__xegtao_main_medium, "main", d->main_layout, &d->main_medium_pipeline);
  if (!d->main_high_pipeline.handle)     mkcs(__xegtao_main_high, "main", d->main_layout, &d->main_high_pipeline);
  if (!d->main_ultra_pipeline.handle)    mkcs(__xegtao_main_ultra, "main", d->main_layout, &d->main_ultra_pipeline);
  if (!d->denoise_pipeline.handle)       mkcs(__xegtao_denoise_pass, "main", d->denoise_layout, &d->denoise_pipeline);
  if (!d->denoise_last_pipeline.handle)  mkcs(__xegtao_denoise_last, "main", d->denoise_layout, &d->denoise_last_pipeline);

  // ── SSGI pipeline layout (sampler, cbv, 3 SRVs, 1 UAV, push_constants) ──
  if (!make_layout(3u, 1u, &d->ssgi_layout)) return false;
  if (!d->denoise_pipeline.handle) { /* placeholder — denoise_pipeline already created above */ }
  if (!make_layout(2u, 1u, &d->ssgi_denoise_layout)) return false;
  if (!d->ssgi_pipeline.handle)
    mkcs(__xegtao_ssgi, "main", d->ssgi_layout, &d->ssgi_pipeline);
  if (!d->ssgi_denoise_pipeline.handle)
    mkcs(__xegtao_ssgi_denoise, "main", d->ssgi_denoise_layout, &d->ssgi_denoise_pipeline);

  return d->prefilter_pipeline.handle && d->main_high_pipeline.handle
      && d->denoise_pipeline.handle && d->denoise_last_pipeline.handle;
}

// ── SSGI Dispatch ──

static void RunSSGI(reshade::api::command_list* cl, DeviceData* d) {
  if (!d->captured_depth_srv.handle) return;
  // CPU opt: suppress descriptor capture events during our own dispatch (kai-style).
  struct ScopedDispatchGuard {
    bool prev;
    ScopedDispatchGuard() : prev(g_xegtao_dispatch_in_progress) { g_xegtao_dispatch_in_progress = true; }
    ~ScopedDispatchGuard() { g_xegtao_dispatch_in_progress = prev; }
  } _guard;
  if (g_cpuopt_ssgi_frame_skip > 0.5f) {
    uint64_t n = (uint64_t)g_cpuopt_ssgi_frame_skip + 1u;
    if ((d->frame_index % n) != 0u) return;
  }
  auto* dev = cl->get_device();
  if (!CreateComputePipelinesIfNeeded(dev, d)) return;
  if (!EnsureXeGTAODescriptorTables(dev, d->ssgi_layout, &d->ssgi_tables)) return;
  if (!EnsureXeGTAODescriptorTables(dev, d->ssgi_denoise_layout, &d->ssgi_denoise_tables)) return;

  const uint32_t w = d->working_width, h = d->working_height;
  uint32_t ssgi_w = (g_ssgi_resolution > 0.5f) ? std::max(64u, w / 2u) : w;
  uint32_t ssgi_h = (g_ssgi_resolution > 0.5f) ? std::max(64u, h / 2u) : h;

  const auto UA = reshade::api::resource_usage::unordered_access;
  const auto SR = reshade::api::resource_usage::shader_resource;
  const auto CS = reshade::api::shader_stage::all_compute;
  const auto AC = reshade::api::pipeline_stage::all_compute;

  auto apply_ssgi = [&](reshade::api::pipeline_layout lo,
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

  // SSGI pass: 3 SRVs (depth MIPs, MRT normal, light buffer), 1 UAV (GI output)
  cl->bind_pipeline(AC, d->ssgi_pipeline);
  {
    reshade::api::resource_view ssgi_srvs[3] = {
        d->depth_mips_srv,
        d->captured_mrt_normal_srv.handle ? d->captured_mrt_normal_srv : d->fallback_srv,
        d->captured_light_buffer_srv.handle ? d->captured_light_buffer_srv : d->fallback_srv,
    };
    reshade::api::descriptor_table_update u[4] = {
      {{},0,0,1,reshade::api::descriptor_type::sampler,&d->point_clamp_sampler},
      {{},0,0,1,reshade::api::descriptor_type::constant_buffer,&d->captured_scene_cbv_view},
      {{},0,0,3,reshade::api::descriptor_type::texture_shader_resource_view,ssgi_srvs},
      {{},0,0,1,reshade::api::descriptor_type::texture_unordered_access_view,&d->ssgi_output_uav},
    };
    apply_ssgi(d->ssgi_layout, &d->ssgi_tables, 4, u);
    auto pc = BuildXeGTAOPushConstants(d, false);
    pc[24] = g_ssgi_radius;
    pc[25] = g_ssgi_steps;
    pc[26] = g_ssgi_directions;
    pc[27] = g_ssgi_thickness;
    pc[28] = g_ssgi_intensity;
    pc[30] = g_ssgi_step_distribution;
    cl->push_constants(CS, d->ssgi_layout, kXeGtaoPushConstantsLayoutParam, 0, 32, pc.data());
  }
  cl->dispatch((ssgi_w + 7) / 8, (ssgi_h + 7) / 8, 1);
  cl->barrier(d->ssgi_output_texture, UA, SR);

  // SSGI denoise pass
  cl->bind_pipeline(AC, d->ssgi_denoise_pipeline);
  {
    reshade::api::resource_view dn_srvs[2] = {d->ssgi_output_srv, d->depth_mips_srv};
    reshade::api::descriptor_table_update u[4] = {
      {{},0,0,1,reshade::api::descriptor_type::sampler,&d->point_clamp_sampler},
      {{},0,0,1,reshade::api::descriptor_type::constant_buffer,&d->captured_scene_cbv_view},
      {{},0,0,2,reshade::api::descriptor_type::texture_shader_resource_view,dn_srvs},
      {{},0,0,1,reshade::api::descriptor_type::texture_unordered_access_view,&d->ssgi_denoised_uav},
    };
    apply_ssgi(d->ssgi_denoise_layout, &d->ssgi_denoise_tables, 4, u);
    auto pc = BuildXeGTAOPushConstants(d, false);
    pc[24] = 1.0f;  // blur strength
    pc[25] = 1.0f;  // depth sensitivity
    cl->push_constants(CS, d->ssgi_denoise_layout, kXeGtaoPushConstantsLayoutParam, 0, 32, pc.data());
  }
  cl->dispatch((ssgi_w + 7) / 8, (ssgi_h + 7) / 8, 1);
}

// ── Dispatch ──

static bool RunXeGTAO(reshade::api::command_list* cl, DeviceData* d) {
  if (!d->captured_depth_srv.handle) return false;
  // CPU opt: suppress descriptor capture events during our own dispatch (kai-style).
  struct ScopedDispatchGuard {
    bool prev;
    ScopedDispatchGuard() : prev(g_xegtao_dispatch_in_progress) { g_xegtao_dispatch_in_progress = true; }
    ~ScopedDispatchGuard() { g_xegtao_dispatch_in_progress = prev; }
  } _guard;
  // Frame skip
  if (g_cpuopt_xegtao_frame_skip > 0.5f) {
    uint64_t n = (uint64_t)g_cpuopt_xegtao_frame_skip + 1u;
    if ((d->frame_index % n) != 0u) return true;
  }
  auto* dev = cl->get_device();
  if (shader_injection.xegtao_debug_logging > 0.5f)
    reshade::log::message(reshade::log::level::info, "[XeGTAO] RunXeGTAO: creating pipelines...");
  if (!CreateComputePipelinesIfNeeded(dev, d)) return false;
  if (shader_injection.xegtao_debug_logging > 0.5f)
    reshade::log::message(reshade::log::level::info, "[XeGTAO] RunXeGTAO: allocating descriptor tables...");
  if (!EnsureXeGTAODescriptorTables(dev, d->prefilter_layout, &d->prefilter_tables)) return false;
  if (!EnsureXeGTAODescriptorTables(dev, d->main_layout, &d->main_tables)) return false;
  if (!EnsureXeGTAODescriptorTables(dev, d->denoise_layout, &d->denoise_tables)) return false;

  uint32_t w = d->working_width, h = d->working_height;
  if (w < 64 || h < 64) return false;

  if (shader_injection.xegtao_debug_logging > 0.5f)
    reshade::log::message(reshade::log::level::info,
      (std::string("[XeGTAO] RunXeGTAO: dispatching pass 1 (") +
       std::to_string(w) + "x" + std::to_string(h) + ")").c_str());

  // CPU opt: save/restore command list state once for all passes.
  renodx::utils::state::CommandListState batched_prev = {};
  if (g_cpuopt_batched_state > 0.5f) {
    auto* cs = renodx::utils::state::GetCurrentState(cl);
    if (cs) batched_prev = *cs;
  }

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
    cl->push_constants(CS, d->prefilter_layout, kXeGtaoPushConstantsLayoutParam, 0, 32, pc.data());
  }
  cl->dispatch((w + 15) / 16, (h + 15) / 16, 1);
  bar(d->depth_mips_texture, UA, SR);
  if (shader_injection.xegtao_debug_logging > 0.5f)
    reshade::log::message(reshade::log::level::info, "[XeGTAO] Pass 1 (prefilter) done.");

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
    reshade::api::resource_view mu[2] = {d->ao_term_a_uav, d->edges_uav};
    reshade::api::resource_view main_srvs[2] = {
        d->depth_mips_srv,
        d->captured_mrt_normal_srv.handle ? d->captured_mrt_normal_srv : d->fallback_srv
    };
    reshade::api::descriptor_table_update u[4] = {
      {{},0,0,1,reshade::api::descriptor_type::sampler,&d->point_clamp_sampler},
      {{},0,0,1,reshade::api::descriptor_type::constant_buffer,&d->captured_scene_cbv_view},
      {{},0,0,2,reshade::api::descriptor_type::texture_shader_resource_view,main_srvs},
      {{},0,0,2,reshade::api::descriptor_type::texture_unordered_access_view,mu},
    };
    apply_descriptors(d->main_layout, &d->main_tables, 4, u);
    auto pc = BuildXeGTAOPushConstants(d, false);
    cl->push_constants(CS, d->main_layout, kXeGtaoPushConstantsLayoutParam, 0, 32, pc.data());
  }
  cl->dispatch((w + 7) / 8, (h + 7) / 8, 1);
  bar(d->ao_term_a_texture, UA, SR);
  bar(d->edges_texture, UA, SR);
  if (shader_injection.xegtao_debug_logging > 0.5f)
    reshade::log::message(reshade::log::level::info, "[XeGTAO] Pass 2 (main) done.");

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
      reshade::api::resource_view sv[2] = {src, d->edges_srv};
      reshade::api::descriptor_table_update u[4] = {
        {{},0,0,1,reshade::api::descriptor_type::sampler,&d->point_clamp_sampler},
        {{},0,0,1,reshade::api::descriptor_type::constant_buffer,&d->captured_scene_cbv_view},
        {{},0,0,2,reshade::api::descriptor_type::texture_shader_resource_view,sv},
        {{},0,0,1,reshade::api::descriptor_type::texture_unordered_access_view,&dst_uav},
      };
      apply_descriptors(d->denoise_layout, &d->denoise_tables, 4, u);
      auto pc = BuildXeGTAOPushConstants(d, last);
      cl->push_constants(CS, d->denoise_layout, kXeGtaoPushConstantsLayoutParam, 0, 32, pc.data());
      cl->dispatch((w + 15) / 16, (h + 7) / 8, 1);
      bar(dst_tex, UA, SR);
      use_a = !use_a;
    }
  }
  if (shader_injection.xegtao_debug_logging > 0.5f)
    reshade::log::message(reshade::log::level::info, "[XeGTAO] All passes complete.");

  // CPU opt: restore state after all passes.
  if (g_cpuopt_batched_state > 0.5f) {
    cl->bind_pipeline(reshade::api::pipeline_stage::all_compute, reshade::api::pipeline{0u});
    auto* cs = renodx::utils::state::GetCurrentState(cl);
    if (cs) *cs = batched_prev;
  }
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
