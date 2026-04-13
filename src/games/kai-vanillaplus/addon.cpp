/*
 * Copyright (C) 2026
 * SPDX-License-Identifier: MIT
 */

#define ImTextureID ImU64

#define DEBUG_LEVEL_0

#include <array>
#include <atomic>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <span>
#include <string>
#include <vector>

#include <deps/imgui/imgui.h>
#include <include/reshade.hpp>

#include <embed/shaders.h>

#include "../../mods/shader.hpp"
#include "../../utils/descriptor.hpp"
#include "../../utils/pipeline_layout.hpp"
#include "../../utils/resource.hpp"
#include "../../utils/settings.hpp"
#include "../../utils/platform.hpp"
#include "../../utils/shader.hpp"
#include "../../utils/state.hpp"
#include "../../utils/swapchain.hpp"
#include "./kai-vanillaplus.h"

namespace {

constexpr uint32_t kLightingShader = 0x430ED091u;
constexpr uint32_t kLightingSoftShader = 0xF6C55E5Fu;
constexpr uint32_t kCharacterShader = 0x445A1838u;
constexpr uint32_t kAoPass0Shader = 0x0E83E74Eu;
constexpr uint32_t kAoPass1Shader = 0x3CA978C6u;
constexpr uint32_t kAoPass2Shader = 0x928A59DAu;
constexpr uint32_t kAoPass3Shader = 0xA385AB53u;
constexpr uint32_t kAoPass4Shader = 0xBEA381A1u;
constexpr uint32_t kAoFinalShader = 0x036B0D74u;
constexpr uint32_t kVolFogShader = 0xBD7DFE49u;
constexpr uint32_t kWireFenceShader = 0x26F1598Bu;
constexpr uint32_t kIsFastTextureBinding = 15u;
constexpr uint32_t kIsFastSamplerBinding = 15u;
constexpr uint32_t kWireIsFastTextureBinding = 14u;
constexpr uint32_t kLightingMrtNormalRegister = 1u;
constexpr uint32_t kLightingDepthRegister = 3u;
constexpr uint32_t kLightingSsaoRegister = 4u;
constexpr uint32_t kLightingSceneCbRegister = 0u;
constexpr uint32_t kLightingXeGtaoRegister = 22u;
constexpr uint32_t kXeGtaoPushConstantsLayoutParam = 4u;
constexpr uint64_t kSceneCbMinimumBytes = 95u * 16u;
constexpr uint64_t kSceneCbMaximumBytes = 64u * 1024u;
constexpr uint64_t kXeGTAODeferredStartupGuardFrames = 3u;
constexpr uint64_t kXeGTAOStartupDispatchGuardFrames = 16u;
constexpr uint64_t kXeGTAOStartupRequireCurrentSceneCbvFrames = 64u;
constexpr uint64_t kXeGTAOFallbackStartupQuarantineFrames = 240u;
constexpr uint64_t kXeGTAOClearStartupGuardFrames = 8u;
constexpr uint64_t kXeGTAOResizeDispatchGuardFrames = 4u;
constexpr uint64_t kXeGTAOFallbackSceneCbvMaxAgeFrames = 2u;
constexpr uint64_t kXeGTAOFallbackPostResizeCooldownFrames = 240u;
constexpr uint32_t kXeGTAOFallbackSceneCbvRequiredStableFrames = 240u;
constexpr bool kEnableAddonLogs = false;
std::atomic_bool g_enable_runtime_addon_logs{false};
std::atomic_bool g_xegtao_startup_mode_logged{false};

struct DeviceData;

bool OnBeforeLightingShaderDraw(reshade::api::command_list* cmd_list);
void OnAfterLightingShaderDraw(reshade::api::command_list* cmd_list);
bool OnBeforeVanillaAoDraw(reshade::api::command_list* cmd_list);
bool OnBeforeWireFenceShaderDraw(reshade::api::command_list* cmd_list);
void OnCharacterShaderDrawn(reshade::api::command_list* cmd_list);
void OnAoFinalPassDrawn(reshade::api::command_list* cmd_list);
void OnVolFogShaderDrawn(reshade::api::command_list* cmd_list);
bool BindISFastNoisePixel(reshade::api::command_list* cmd_list);
bool BindISFastNoiseTexturePixel(reshade::api::command_list* cmd_list, uint32_t texture_binding);
void OnInitDevice(reshade::api::device* device);
void OnDestroyDevice(reshade::api::device* device);
void OnInitSwapchain(reshade::api::swapchain* swapchain, bool resize);
void OnDestroySwapchain(reshade::api::swapchain* swapchain, bool resize);
void ReloadIsFastResources(reshade::api::device* device, const char* reason);
void DestroyXeGTAOResources(reshade::api::device* device, DeviceData* data);
void DestroyXeGTAODescriptorTables(reshade::api::device* device, DeviceData* data);
void DestroyXeGTAOState(reshade::api::device* device, DeviceData* data);
void DestroyDedicatedSssViews(reshade::api::device* device, DeviceData* data);
void TransitionResource(
    reshade::api::command_list* cmd_list,
    reshade::api::resource resource,
    reshade::api::resource_usage before,
    reshade::api::resource_usage after);
void ResolveXeGTAOInputsFromCurrentBindings(reshade::api::command_list* cmd_list, DeviceData* data);

inline void AddonLog(reshade::log::level level, const char* message) {
  if (message == nullptr) return;
  if (!kEnableAddonLogs && !g_enable_runtime_addon_logs.load(std::memory_order_relaxed)) return;
  reshade::log::message(level, message);
}

inline void AddonLog(reshade::log::level level, const std::string& message) {
  if (!kEnableAddonLogs && !g_enable_runtime_addon_logs.load(std::memory_order_relaxed)) return;
  reshade::log::message(level, message.c_str());
}

renodx::mods::shader::CustomShaders custom_shaders = {
    {
        kLightingShader,
        {
            .crc32 = kLightingShader,
            .code = __0x430ED091,
            .on_draw = &OnBeforeLightingShaderDraw,
            .on_drawn = &OnAfterLightingShaderDraw,
        },
    },                                             // lighting
    {
        kLightingSoftShader,
        {
            .crc32 = kLightingSoftShader,
            .code = __0xF6C55E5F,
            .on_draw = &OnBeforeLightingShaderDraw,
            .on_drawn = &OnAfterLightingShaderDraw,
        },
    },                                             // lighting soft shadows
    {
        kCharacterShader,
        {
            .crc32 = kCharacterShader,
            .code = __0x445A1838,
            .on_drawn = &OnCharacterShaderDrawn,
        },
    },                                             // character lighting
    CustomShaderEntry(0x209125C1),                 // SSR
    CustomShaderEntry(0xB1CCBCAE),                 // glass
    CustomShaderEntry(0x1A17A133),                 // glass
    CustomShaderEntry(0xCA715B78),                 // glass
    CustomShaderEntry(0xE1E0ACBB),                 // glass
    CustomShaderEntry(0xF237E72F),                 // glass
    CustomShaderEntry(0x07E984A7),                 // glass
    CustomShaderEntry(0xFDC5CDBF),                 // glass
    CustomShaderEntry(0x8337B262),                 // floor
    CustomShaderEntry(0xD97BD91B),                 // glass
    CustomShaderEntry(0xEFB6AC0F),                 // glass
    CustomShaderEntry(0x534E54EA),                 // sss source pass
    CustomShaderEntry(0xAB6DBF4D),                 // dof coc
    CustomShaderEntry(0x2734F870),                 // dof blur composite
    {
      kVolFogShader,
      {
        .crc32 = kVolFogShader,
        .code = __0xBD7DFE49,
        .on_drawn = &OnVolFogShaderDrawn,
      },
    },                                             // vol fog
    {
        kAoPass0Shader,
        {
            .crc32 = kAoPass0Shader,
            .code = __0x0E83E74E,
            //.on_draw = &OnBeforeVanillaAoDraw,
        },
    },                                             // vanilla ao pass 0
    {
        kAoPass1Shader,
        {
            .crc32 = kAoPass1Shader,
            .code = __0x3CA978C6,
            //.on_draw = &OnBeforeVanillaAoDraw,
        },
    },                                             // vanilla ao pass 1
    {
        kAoPass2Shader,
        {
            .crc32 = kAoPass2Shader,
            .code = __0x928A59DA,
            //.on_draw = &OnBeforeVanillaAoDraw,
        },
    },                                             // vanilla ao pass 2
    {
        kAoPass3Shader,
        {
            .crc32 = kAoPass3Shader,
            .code = __0xA385AB53,
            //.on_draw = &OnBeforeVanillaAoDraw,
        },
    },                                             // vanilla ao pass 3
    {
        kAoPass4Shader,
        {
            .crc32 = kAoPass4Shader,
            .code = __0xBEA381A1,
            //.on_draw = &OnBeforeVanillaAoDraw,
        },
    },                                             // vanilla ao pass 4
    {
        kAoFinalShader,
        {
            .crc32 = kAoFinalShader,
            .code = __0x036B0D74,
            //.on_draw = &OnBeforeVanillaAoDraw,
            //.on_drawn = &OnAoFinalPassDrawn,
        },
    },                                             // vanilla ao final pass hook
};



SssInjectData shader_injection = {
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
    .char_shadow_strength = 1.f,
    .shadow_pcss_jitter_enabled = 1.f,
    .shadow_pcss_sample_mode = 1.f,
    .shadow_isfast_jitter_amount = 1.f,
    .shadow_isfast_jitter_speed = 237.f,
    .ssgi_mod_enabled = 1.f,
    .ssgi_color_boost = 1.f,
    .ssgi_alpha_boost = 1.f,
    .ssgi_pow = 1.f,
    .ssr_mode = 1.f,
    .ssr_ray_count_scale = 2.f,
    .ssr_temporal_clamp_enable = 1.f,
    .ssr_temporal_clamp_radius = 1.f,
    .ssr_temporal_clamp_strength = 1.f,
    .ssr_temporal_jitter_enable = 1.f,
    .ssr_temporal_jitter_amount = 1.f,
    .cubemap_improvements_enabled = 1.f,
    .cubemap_lighting_mip_boost = 1.5f,
    .floor_cubemap_mip_scale = 4.f,
    .shadow_base_softness = 0.2f,
    .fog_color_correction_enabled = 1.f,
    .fog_hue = 0.f,
    .fog_chrominance = 0.f,
    .fog_avg_brightness = 0.85f,
    .fog_min_brightness = 0.f,
    .fog_min_chroma_change = 0.f,
    .fog_max_chroma_change = 0.f,
    .fog_lightness_strength = 1.f,
    .fog_color_correction_strength = 0.5f,
    .volfog_tricubic_enabled = 1.f,
    .volfog_is_fast_enabled = 1.f,
    .isfast_noise_bound = 0.f,
    .volfog_color_correction_strength = 0.5f,
    .wire_alpha_mode = 2.f,
    .wire_alpha_sharpen = 1.0f,
    .wire_alpha_threshold_offset = 0.0f,
    .wire_alpha_temporal_amount = 0.77f,
    .wire_alpha_temporal_speed = 237.0f,
    .dof_mode = 1.f,
    .dof_strength = 1.f,
    .dof_radius_scale = 1.33f,
    .dof_sample_count = 24.f,
    .dof_near_scale = 1.f,
    .dof_far_scale = 1.f,
    .dof_coc_curve = 1.f,
    .dof_edge_threshold = 0.25f,
    .foliage_translucency_scale = 1.f,
    .foliage_opacity_scale = 1.f,
    .foliage_ssao_scale = 1.f,
    .foliage_sss_enabled = 1.f,
    .foliage_sss_strength = 1.0f,
    .foliage_sss_sample_count = 24.f,
    .foliage_sss_surface_thickness = 0.005f,
    .foliage_sss_contrast = 2.f,
    .foliage_sss_jitter_enabled = 1.f,
    .foliage_sss_height_enabled = 1.f,
    .foliage_sss_height_min = 0.f,
    .foliage_sss_height_max = 1.f,
    .foliage_sss_height_fade = 0.1f,
    .foliage_sss_vertical_reject = 0.3f,
    .foliage_sss_max_darkening = 0.40f,
    .foliage_sss_bright_reject_threshold = 0.19f,
    .foliage_sss_bright_reject_fade = 0.5f,
    .char_gi_enabled = 1.f,
    .char_gi_strength = 3.0f,
    .char_gi_alpha_scale = 1.0f,
    .char_gi_chroma_strength = 0.50f,
    .char_gi_luma_strength = 0.0f,
    .char_gi_shadow_power = 1.25f,
    .char_gi_headroom_power = 1.25f,
    .char_gi_max_add = 0.020f,
    .char_gi_dark_boost = 0.0f,
    .char_gi_debug_mode = 0.f,
    .char_gi_debug_scale = 1.f,
    .char_gi_debug_chars_only = 1.f,
    .char_gi_bright_boost = 3.0f,
    .char_gi_peak_luma_cap = 0.0f,
    .char_gi_depth_reject = 2.0f,
    .char_gi_normal_reject = 0.15f,
    .char_gi_ao_influence = 0.66f,
    .char_gi_reject_strength = 8.0f,
    .sss_dedicated_bound = 0.f,
    .xegtao_dedicated_bound = 0.f,
    .xegtao_debug_mode = 0.f,
    .xegtao_normal_input_mode = 1.f,
    .xegtao_mrt_normal_valid = 0.f,
    .xegtao_bent_normals = 0.f,
    .xegtao_bent_diffuse_strength = 0.f,
    .xegtao_bent_diffuse_softness = 0.f,
    .xegtao_bent_specular_strength = 0.f,
    .xegtao_bent_specular_proxy_roughness = 0.f,
    .xegtao_bent_max_darkening = 0.f,
    .xegtao_force_neutral_x = 0.f,
    .xegtao_debug_blackout = 0.f,
    .xegtao_ao_active_for_draw = 0.f,
    .xegtao_foliage_ao_blend = 1.0f,
    .volfog_enabled = 1.f,
    .volfog_jitter_enabled = 1.f,
    .volfog_jitter_amount = 1.5f,
    .volfog_jitter_speed = 237.f,
    .xegtao_foliage_mask_method = 0.f,
};

float settings_mode = 0.f;
float isfast_jitter_master = 1.f;
float char_ssgi_composite_method = 1.f;  // 0=Off, 1=On
float xegtao_mode = 1.f;                  // 0=Off, 1=On
float xegtao_quality = 0.f;               // 0=High, 1=Very High, 2=Ultra
float xegtao_fix_mode = 0.f;              // 0=Off/current path, 1..5=cumulative fixes
float xegtao_fix_l5_prefilter = 1.f;      // Level-5 pass isolation toggle
float xegtao_fix_l5_main = 1.f;           // Level-5 pass isolation toggle
float xegtao_fix_l5_denoise = 1.f;        // Level-5 pass isolation toggle
float xegtao_fix_l5_composite = 1.f;      // Level-5 pass isolation toggle
float xegtao_probe_a_dispatch_no_t22 = 0.f;  // Probe A: run dispatch path, suppress t22 bind
float xegtao_probe_b_t22_no_dispatch = 0.f;  // Probe B: exercise t22 bind, suppress dispatch
float xegtao_precision = 2.f;             // Runtime-forced to Full FP32 (legacy key kept for compatibility)
float xegtao_normal_input_mode = 1.f;     // 0=Off(depth fallback), 1=View-Transformed
float xegtao_normal_influence = 0.20f;
float xegtao_normal_depth_blend = 0.70f;
float xegtao_normal_sharpness = 1.f;
float xegtao_normal_edge_rejection = 1.f;
float xegtao_normal_z_preservation = 1.f;
float xegtao_normal_detail_response = 4.f;
float xegtao_normal_max_darkening = 0.6f;
float xegtao_normal_darkening_mode = 0.f;  // 0=Fast, 1=Exact
float xegtao_denoiser_mode = 0.f;  // 0=Vanilla, 1=IS-FAST Only, 2=Hybrid
float xegtao_isfast_passes = 2.f;
float xegtao_isfast_samples = 8.f;
float xegtao_isfast_radius = 1.0f;
float xegtao_isfast_edge_sensitivity = 2.0f;
float xegtao_isfast_spatial_sigma = 1.0f;
float xegtao_isfast_hybrid_blend = 0.5f;
float xegtao_isfast_jitter = 1.f;         // 0=Off, 1=On
float xegtao_isfast_jitter_amount = 1.f;  // 0..1
float xegtao_skip_vanilla_ao = 1.f;       // 0=Off, 1=On
float xegtao_denoise_pass_count = 1.f;    // 0..3
float xegtao_radius = 0.5f;
float xegtao_falloff_range = 0.615f;
float xegtao_radius_multiplier = 1.5f;
float xegtao_final_value_power = 2.0f;
float xegtao_sample_distribution_power = 1.5f;
float xegtao_thin_occluder_compensation = 0.50f;
float xegtao_depth_mip_sampling_offset = 3.3f;
float xegtao_denoise_blur_beta = 8.f;
float xegtao_debug_mode = 0.f;
float xegtao_runtime_debug_logging = 0.f;
float xegtao_enable_fallbacks = 1.f;
float xegtao_foliage_ao_blend = 1.0f;
float xegtao_foliage_mask_method = 0.f;  // 0=SSS parity(t1 strict), 1=legacy broad(t1), 2=t10 strict

std::atomic_uint64_t g_lighting_mrt0_view{0u};
std::atomic_uint64_t g_character_mrt0_view{0u};
HMODULE g_hmodule = nullptr;
reshade::api::resource g_isfast_texture = {0u};
reshade::api::resource_view g_isfast_reshade_srv = {0u};
reshade::api::sampler g_isfast_reshade_sampler = {0u};
bool g_isfast_bind_logged = false;
bool g_isfast_using_debug_texture = false;
bool g_isfast_bind_failed_logged = false;
bool g_isfast_compute_bind_logged = false;
bool g_isfast_mode_use_fast_texture = true;
float isfast_texture_source_enabled = 1.f;  // 0=Off(debug), 1=On(real DDS)
thread_local bool g_skip_descriptor_capture = false;

struct ScopedDescriptorCaptureSkip {
  explicit ScopedDescriptorCaptureSkip(bool enable = true)
      : active_(enable), previous_(g_skip_descriptor_capture) {
    if (active_) {
      g_skip_descriptor_capture = true;
    }
  }

  ~ScopedDescriptorCaptureSkip() {
    if (active_) {
      g_skip_descriptor_capture = previous_;
    }
  }

  ScopedDescriptorCaptureSkip(const ScopedDescriptorCaptureSkip&) = delete;
  ScopedDescriptorCaptureSkip& operator=(const ScopedDescriptorCaptureSkip&) = delete;

 private:
  bool active_;
  bool previous_;
};

constexpr uint64_t kInvalidFrameIndex = std::numeric_limits<uint64_t>::max();
constexpr uint32_t kXeGtaoDepthMipLevels = 5u;
constexpr uint32_t kXeGtaoDescriptorTableParamCount = 4u;
using XeGTAODescriptorTableSet =
  std::array<reshade::api::descriptor_table, kXeGtaoDescriptorTableParamCount>;

enum class XeGTAOMode : uint32_t {
  kOff = 0u,
  kLightingOverride = 1u,
};

enum class XeGTAOFixMode : uint32_t {
  kOff = 0u,
  kProducerConsumerSplit = 1u,
  kDispatchIsolationRestore = 2u,
  kSingleOwnerDeterministic = 3u,
  kStrictSideEffectGuard = 4u,
  kPassIsolationDiagnostics = 5u,
};

enum class XeGTAOPrecision : uint32_t {
  kDepthR16 = 0u,
  kDepthR32 = 1u,
  kFullFP32 = 2u,
};

enum class XeGTAOSceneCbvSource : uint32_t {
  kNone = 0u,
  kCurrentLighting = 1u,
  kFallback = 2u,
};

struct __declspec(uuid("d0ce55f2-f373-4f3a-99ed-f08888d7f11b")) DeviceData {
  reshade::api::resource_view captured_mrt_normal_srv = {};
  reshade::api::resource_view captured_depth_srv = {};
  reshade::api::resource_view captured_ssao_srv = {};
  reshade::api::buffer_range captured_scene_cbv = {};
  bool captured_scene_cbv_valid = false;
  uint64_t captured_scene_cbv_frame = kInvalidFrameIndex;
  XeGTAOSceneCbvSource captured_scene_cbv_source = XeGTAOSceneCbvSource::kNone;
  bool resolved_scene_cbv_from_current_bindings = false;
  reshade::api::buffer_range fallback_scene_cbv = {};
  bool fallback_scene_cbv_seen = false;
  uint64_t fallback_scene_cbv_frame = kInvalidFrameIndex;
  uint64_t fallback_scene_cbv_signature = 0u;
  uint32_t fallback_scene_cbv_stable_count = 0u;

  reshade::api::sampler point_clamp_sampler = {};

  uint32_t working_width = 0u;
  uint32_t working_height = 0u;
  uint32_t working_precision = std::numeric_limits<uint32_t>::max();
  reshade::api::format working_ao_format = reshade::api::format::unknown;

  reshade::api::resource depth_mips_texture = {};
  reshade::api::resource_view depth_mips_srv = {};
  std::array<reshade::api::resource_view, kXeGtaoDepthMipLevels> depth_mips_uavs = {};

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
  reshade::api::resource_desc composite_desc = {};

  reshade::api::resource character_sss_current_resource = {};
  reshade::api::resource_view character_sss_current_srv = {};
  reshade::api::resource character_sss_last_valid_resource = {};
  reshade::api::resource_view character_sss_last_valid_srv = {};
  uint64_t character_sss_current_frame = kInvalidFrameIndex;

  reshade::api::pipeline_layout xegtao_prefilter_layout = {};
  reshade::api::pipeline_layout xegtao_main_layout = {};
  reshade::api::pipeline_layout xegtao_denoise_layout = {};
  reshade::api::pipeline_layout xegtao_denoise_isfast_layout = {};
  reshade::api::pipeline_layout xegtao_composite_layout = {};
  reshade::api::pipeline_layout xegtao_normal_cap_layout = {};

  XeGTAODescriptorTableSet xegtao_prefilter_descriptor_tables = {};
  XeGTAODescriptorTableSet xegtao_main_descriptor_tables = {};
  XeGTAODescriptorTableSet xegtao_denoise_descriptor_tables = {};
  XeGTAODescriptorTableSet xegtao_composite_descriptor_tables = {};
  XeGTAODescriptorTableSet xegtao_normal_cap_descriptor_tables = {};

  reshade::api::pipeline xegtao_prefilter_pipeline = {};
  reshade::api::pipeline xegtao_main_pipeline = {};
  reshade::api::pipeline xegtao_denoise_pipeline = {};
  reshade::api::pipeline xegtao_denoise_isfast_pipeline = {};
  reshade::api::pipeline xegtao_composite_pipeline = {};
  reshade::api::pipeline xegtao_normal_cap_pipeline = {};

  uint64_t present_frame_index = 0u;
  uint64_t xegtao_resize_guard_until_frame = 0u;
  uint64_t xegtao_resize_guard_log_frame = kInvalidFrameIndex;
  uint64_t last_gtao_frame = kInvalidFrameIndex;
  uint64_t last_ao_hook_frame = kInvalidFrameIndex;
  uint64_t last_copyback_frame = kInvalidFrameIndex;
  uint64_t last_gtao_failure_log_frame = kInvalidFrameIndex;
  uint64_t last_skip_vanilla_ao_ignored_log_frame = kInvalidFrameIndex;
  uint64_t xegtao_mrt_normal_frame = kInvalidFrameIndex;
  bool xegtao_mrt_normal_valid = false;
  bool copyback_succeeded = false;
  uint64_t xegtao_copyback_frame = kInvalidFrameIndex;
  bool xegtao_copyback_requested_for_frame = false;
  bool xegtao_copyback_succeeded_for_frame = false;
  bool xegtao_copyback_active_for_apply = false;
  bool xegtao_result_signature_valid = false;
  uint64_t xegtao_result_signature_frame = kInvalidFrameIndex;
  uint64_t xegtao_result_t3_resource_handle = 0u;
  uint64_t xegtao_result_t4_resource_handle = 0u;
  uint64_t xegtao_result_t3_view_handle = 0u;
  uint64_t xegtao_result_t4_view_handle = 0u;
  uint32_t xegtao_result_t3_width = 0u;
  uint32_t xegtao_result_t3_height = 0u;
  uint32_t xegtao_result_t4_width = 0u;
  uint32_t xegtao_result_t4_height = 0u;
  uint32_t xegtao_result_working_width = 0u;
  uint32_t xegtao_result_working_height = 0u;
  bool xegtao_consume_signature_valid = false;
  uint64_t xegtao_consume_signature_frame = kInvalidFrameIndex;
  uint64_t xegtao_consume_t3_resource_handle = 0u;
  uint64_t xegtao_consume_t4_resource_handle = 0u;
  uint32_t xegtao_consume_t3_width = 0u;
  uint32_t xegtao_consume_t3_height = 0u;
  uint32_t xegtao_consume_t4_width = 0u;
  uint32_t xegtao_consume_t4_height = 0u;
  uint32_t xegtao_consume_working_width = 0u;
  uint32_t xegtao_consume_working_height = 0u;
  bool xegtao_consume_owner_valid = false;
  uint64_t xegtao_consume_owner_frame = kInvalidFrameIndex;
  uint32_t xegtao_consume_owner_shader_hash = 0u;
  uint32_t xegtao_consume_owner_draw_ordinal = 0u;
  uint64_t xegtao_consume_owner_gate_signature = 0u;
  bool xegtao_consume_owner_downscaled = false;
  uint64_t xegtao_lighting_draw_counter_frame = kInvalidFrameIndex;
  uint32_t xegtao_lighting_draw_counter = 0u;
  bool xegtao_owner_valid = false;
  uint64_t xegtao_owner_frame = kInvalidFrameIndex;
  uint32_t xegtao_owner_shader_hash = 0u;
  uint32_t xegtao_owner_draw_ordinal = 0u;
  uint64_t xegtao_owner_gate_signature = 0u;
  bool xegtao_owner_downscaled = false;
  bool xegtao_deferred_dispatch_pending = false;
  uint64_t xegtao_deferred_dispatch_frame = kInvalidFrameIndex;
  bool xegtao_deferred_dispatch_executed = false;
  uint64_t xegtao_deferred_gate_signature = 0u;
  uint32_t xegtao_deferred_owner_shader_hash = 0u;
  uint32_t xegtao_deferred_owner_draw_ordinal = 0u;
  reshade::api::resource_view xegtao_deferred_depth_srv = {};
  reshade::api::resource_view xegtao_deferred_ssao_srv = {};
  reshade::api::resource_view xegtao_deferred_mrt_normal_srv = {};
  reshade::api::buffer_range xegtao_deferred_scene_cbv = {};
  bool xegtao_deferred_scene_cbv_valid = false;
  uint64_t xegtao_deferred_scene_cbv_frame = kInvalidFrameIndex;
  XeGTAOSceneCbvSource xegtao_deferred_scene_cbv_source = XeGTAOSceneCbvSource::kNone;
  bool xegtao_deferred_resolved_scene_cbv_from_current_bindings = false;
  uint64_t xegtao_deferred_drop_log_frame = kInvalidFrameIndex;
  uint64_t xegtao_volfog_seen_frame = kInvalidFrameIndex;
  bool last_owner_state_valid = false;
  uint64_t last_owner_diag_hash = 0u;
  bool last_gate_state_valid = false;
  bool last_gate_passed = false;
  uint64_t last_gate_diag_hash = 0u;
  bool last_apply_gate_main_state_valid = false;
  bool last_apply_gate_main_passed = false;
  uint64_t last_apply_gate_main_diag_hash = 0u;
  bool last_apply_gate_soft_state_valid = false;
  bool last_apply_gate_soft_passed = false;
  uint64_t last_apply_gate_soft_diag_hash = 0u;
  bool last_apply_path_main_state_valid = false;
  uint64_t last_apply_path_main_hash = 0u;
  bool last_apply_path_soft_state_valid = false;
  uint64_t last_apply_path_soft_hash = 0u;

  uint64_t xegtao_warmup_signature = 0u;
  uint32_t xegtao_warmup_stable_count = 0u;
  uint64_t last_warmup_enter_signature = 0u;
  uint64_t last_warmup_complete_signature = 0u;
  bool xegtao_dispatch_isolation_active = false;
  bool xegtao_dispatch_restore_mismatch = false;
  uint64_t xegtao_trace_frame = kInvalidFrameIndex;
  bool xegtao_trace_dispatch_attempted = false;
  bool xegtao_trace_dispatch_succeeded = false;
  bool xegtao_trace_main_pass_executed = false;
  bool xegtao_trace_composite_pass_executed = false;
  bool xegtao_trace_t22_bind_executed = false;
  bool xegtao_trace_copyback_requested = false;
  bool xegtao_trace_copyback_succeeded = false;
  bool xegtao_trace_apply_gate_passed = false;
  bool xegtao_trace_probe_a_active = false;
  bool xegtao_trace_probe_b_active = false;
  uint32_t xegtao_trace_owner_draw_ordinal = 0u;
  bool xegtao_trace_state_valid = false;
  uint64_t xegtao_trace_diag_hash = 0u;
  uint32_t xegtao_debug_descriptor_reject_count = 0u;
  uint64_t xegtao_debug_last_descriptor_reject_frame = kInvalidFrameIndex;
  uint32_t xegtao_debug_predispatch_reject_count = 0u;
  uint64_t xegtao_debug_last_predispatch_reject_frame = kInvalidFrameIndex;
  uint32_t xegtao_debug_deferred_drop_count = 0u;
  uint64_t xegtao_debug_last_deferred_drop_frame = kInvalidFrameIndex;
  uint32_t xegtao_debug_normal_fallback_count = 0u;
  uint64_t xegtao_debug_last_normal_fallback_frame = kInvalidFrameIndex;

  uint64_t last_capture_diag_log_frame = kInvalidFrameIndex;
  uint64_t last_logged_depth_view_handle = 0u;
  uint64_t last_logged_ssao_view_handle = 0u;
  uint64_t last_logged_mrt_normal_view_handle = 0u;
  uint32_t last_logged_depth_width = 0u;
  uint32_t last_logged_depth_height = 0u;
  uint32_t last_logged_ssao_width = 0u;
  uint32_t last_logged_ssao_height = 0u;
  uint32_t last_logged_mrt_normal_width = 0u;
  uint32_t last_logged_mrt_normal_height = 0u;
  reshade::api::format last_logged_depth_view_format = reshade::api::format::unknown;
  reshade::api::format last_logged_depth_resource_format = reshade::api::format::unknown;
  reshade::api::format last_logged_ssao_view_format = reshade::api::format::unknown;
  reshade::api::format last_logged_ssao_resource_format = reshade::api::format::unknown;
  reshade::api::format last_logged_mrt_normal_view_format = reshade::api::format::unknown;
  reshade::api::format last_logged_mrt_normal_resource_format = reshade::api::format::unknown;
  XeGTAOSceneCbvSource last_logged_scene_cbv_source = XeGTAOSceneCbvSource::kNone;
  uint64_t last_logged_scene_cbv_buffer_handle = 0u;
  bool last_logged_scene_cbv_valid = false;
  bool last_logged_fallback_scene_cbv_seen = false;
  uint64_t last_logged_fallback_scene_cbv_buffer_handle = 0u;
  uint64_t last_logged_fallback_scene_cbv_frame = kInvalidFrameIndex;
  uint32_t last_logged_fallback_scene_cbv_stable_count = 0u;
};

#pragma pack(push, 1)
struct DdsPixelFormat {
  uint32_t size;
  uint32_t flags;
  uint32_t four_cc;
  uint32_t rgb_bit_count;
  uint32_t r_bit_mask;
  uint32_t g_bit_mask;
  uint32_t b_bit_mask;
  uint32_t a_bit_mask;
};

struct DdsHeader {
  uint32_t size;
  uint32_t flags;
  uint32_t height;
  uint32_t width;
  uint32_t pitch_or_linear_size;
  uint32_t depth;
  uint32_t mip_map_count;
  uint32_t reserved1[11];
  DdsPixelFormat ddspf;
  uint32_t caps;
  uint32_t caps2;
  uint32_t caps3;
  uint32_t caps4;
  uint32_t reserved2;
};

struct DdsHeaderDx10 {
  uint32_t dxgi_format;
  uint32_t resource_dimension;
  uint32_t misc_flag;
  uint32_t array_size;
  uint32_t misc_flags2;
};
#pragma pack(pop)

constexpr uint32_t kDdsMagic = 0x20534444u;  // "DDS "
constexpr uint32_t kDdsFourCCDx10 = 0x30315844u;  // "DX10"
constexpr uint32_t kDxgiFormatR8G8Unorm = 49u;
constexpr uint32_t kD3D10ResourceDimensionTexture3D = 4u;
constexpr uint32_t kExpectedIsFastWidth = 128u;
constexpr uint32_t kExpectedIsFastHeight = 128u;
constexpr uint32_t kExpectedIsFastDepth = 32u;
constexpr uint32_t kIsFastBytesPerTexel = 2u;

void LogIsFast(reshade::log::level level, const std::string& message) {
  if (level == reshade::log::level::debug) return;
  const std::string tagged = "IS-FAST: " + message;
  AddonLog(level, tagged.c_str());
}

bool IsIsFastSupportedDevice(reshade::api::device* device) {
  if (device == nullptr) return false;
  return device->get_api() == reshade::api::device_api::d3d11;
}

std::filesystem::path GetModuleDirectory() {
  if (g_hmodule == nullptr) return std::filesystem::current_path();

  std::array<wchar_t, MAX_PATH> module_path = {};
  const DWORD length = GetModuleFileNameW(g_hmodule, module_path.data(), static_cast<DWORD>(module_path.size()));
  if (length == 0 || length >= module_path.size()) return std::filesystem::current_path();
  return std::filesystem::path(module_path.data()).parent_path();
}

bool ReadFileBytes(const std::filesystem::path& path, std::vector<uint8_t>* out_bytes) {
  if (out_bytes == nullptr) return false;
  out_bytes->clear();

  std::ifstream file(path, std::ios::binary | std::ios::ate);
  if (!file) return false;

  const auto file_size = file.tellg();
  if (file_size <= 0) return false;
  const size_t file_size_bytes = static_cast<size_t>(file_size);
  file.seekg(0, std::ios::beg);

  out_bytes->resize(file_size_bytes);
  if (!file.read(reinterpret_cast<char*>(out_bytes->data()), static_cast<std::streamsize>(file_size_bytes))) {
    out_bytes->clear();
    return false;
  }

  return true;
}

bool ValidateAndExtractIsFastDds(
    const std::vector<uint8_t>& file_bytes,
    std::span<const uint8_t>* out_pixel_data) {
  if (out_pixel_data == nullptr) return false;

  LogIsFast(reshade::log::level::debug, "DDS HEADER VALIDATION begin.");

  constexpr size_t kHeaderSize = sizeof(uint32_t) + sizeof(DdsHeader) + sizeof(DdsHeaderDx10);
  if (file_bytes.size() < kHeaderSize) {
    LogIsFast(reshade::log::level::error, "DDS too small to contain required headers.");
    return false;
  }

  uint32_t magic = 0u;
  std::memcpy(&magic, file_bytes.data(), sizeof(uint32_t));
  if (magic != kDdsMagic) {
    LogIsFast(reshade::log::level::error, "DDS magic mismatch (expected 'DDS ').");
    return false;
  }
  LogIsFast(reshade::log::level::debug, "DDS magic bytes OK.");

  DdsHeader header = {};
  std::memcpy(&header, file_bytes.data() + sizeof(uint32_t), sizeof(DdsHeader));
  if (header.size != 124u || header.ddspf.size != 32u) {
    LogIsFast(reshade::log::level::error, "DDS header size mismatch.");
    return false;
  }
  if (header.ddspf.four_cc != kDdsFourCCDx10) {
    LogIsFast(reshade::log::level::error, "DDS does not use required DX10 extended header.");
    return false;
  }
  LogIsFast(reshade::log::level::debug, "DDS DX10 extended header tag OK.");

  DdsHeaderDx10 header_dx10 = {};
  std::memcpy(&header_dx10, file_bytes.data() + sizeof(uint32_t) + sizeof(DdsHeader), sizeof(DdsHeaderDx10));
  if (header_dx10.dxgi_format != kDxgiFormatR8G8Unorm) {
    LogIsFast(reshade::log::level::error, "DDS format is not R8G8_UNORM.");
    return false;
  }
  if (header_dx10.resource_dimension != kD3D10ResourceDimensionTexture3D || header_dx10.array_size != 1u) {
    LogIsFast(reshade::log::level::error, "DDS is not a Texture3D with array_size=1.");
    return false;
  }
  LogIsFast(reshade::log::level::debug, "DDS texture type/dimension OK (Texture3D).");
  if (header.width != kExpectedIsFastWidth || header.height != kExpectedIsFastHeight || header.depth != kExpectedIsFastDepth) {
    LogIsFast(reshade::log::level::error, "DDS dimensions are invalid (expected 128x128x32).");
    return false;
  }
  LogIsFast(reshade::log::level::debug, "DDS dimensions OK (128x128x32).");

  const size_t pixel_data_offset = sizeof(uint32_t) + sizeof(DdsHeader) + sizeof(DdsHeaderDx10);
  const size_t expected_pixel_bytes =
      static_cast<size_t>(header.width) * static_cast<size_t>(header.height) * static_cast<size_t>(header.depth) * kIsFastBytesPerTexel;

  if (file_bytes.size() < (pixel_data_offset + expected_pixel_bytes)) {
    LogIsFast(reshade::log::level::error, "DDS file is truncated (pixel data too small).");
    return false;
  }
  LogIsFast(reshade::log::level::debug, "DDS file size check OK.");

  const uint8_t* pixel_data_ptr = file_bytes.data() + pixel_data_offset;
  std::span<const uint8_t> pixel_data(pixel_data_ptr, expected_pixel_bytes);

  const size_t inspect_bytes_count = std::min<size_t>(32u, pixel_data.size());
  std::ostringstream inspect;
  inspect << "Pixel[0..31]:";
  for (size_t i = 0; i < inspect_bytes_count; ++i) {
    inspect << ' ' << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
            << static_cast<uint32_t>(pixel_data[i]);
  }
  LogIsFast(reshade::log::level::debug, inspect.str());

  const size_t non_zero_count = std::count_if(
      pixel_data.begin(), pixel_data.end(),
      [](uint8_t value) { return value != 0u; });
  {
    std::ostringstream non_zero_message;
    non_zero_message << "Pixel non-zero bytes: " << non_zero_count << "/" << pixel_data.size();
    LogIsFast(reshade::log::level::debug, non_zero_message.str());
  }

  *out_pixel_data = pixel_data;
  LogIsFast(reshade::log::level::debug, "DDS HEADER VALIDATION passed.");
  return true;
}

void DestroyIsFastResources(reshade::api::device* device) {
  if (device == nullptr) return;

  LogIsFast(reshade::log::level::debug, "Destroying IS-FAST resources.");

  if (g_isfast_reshade_sampler.handle != 0u) {
    device->destroy_sampler(g_isfast_reshade_sampler);
    g_isfast_reshade_sampler = {0u};
  }
  if (g_isfast_reshade_srv.handle != 0u) {
    device->destroy_resource_view(g_isfast_reshade_srv);
    g_isfast_reshade_srv = {0u};
  }
  if (g_isfast_texture.handle != 0u) {
    device->destroy_resource(g_isfast_texture);
    g_isfast_texture = {0u};
  }

  shader_injection.isfast_noise_bound = 0.f;
  g_isfast_using_debug_texture = false;
  g_isfast_bind_logged = false;
  g_isfast_bind_failed_logged = false;
  g_isfast_compute_bind_logged = false;
}

bool CreateIsFastTexture(
    reshade::api::device* device,
    uint32_t width,
    uint32_t height,
    uint32_t depth,
    void* pixel_data,
    uint32_t row_pitch,
    uint32_t slice_pitch,
    bool debug_texture) {
  if (device == nullptr || pixel_data == nullptr) return false;

  {
    std::ostringstream message;
    message << "Create Texture3D begin (" << width << "x" << height << "x" << depth
            << ", " << (debug_texture ? "debug" : "dds") << ").";
    LogIsFast(reshade::log::level::debug, message.str());
  }

  reshade::api::resource_desc desc = {};
  desc.type = reshade::api::resource_type::texture_3d;
  desc.texture = {
      width,
      height,
      static_cast<uint16_t>(depth),
      1,
      reshade::api::format::r8g8_unorm,
      1,
  };
  desc.heap = reshade::api::memory_heap::gpu_only;
  desc.usage = reshade::api::resource_usage::shader_resource;
  desc.flags = reshade::api::resource_flags::none;

  reshade::api::subresource_data initial_data = {
      pixel_data,
      row_pitch,
      slice_pitch,
  };

  if (!device->create_resource(desc, &initial_data, reshade::api::resource_usage::shader_resource, &g_isfast_texture)) {
    LogIsFast(reshade::log::level::error, "Create Texture3D failed.");
    return false;
  }
  LogIsFast(reshade::log::level::debug, "Create Texture3D OK.");

  reshade::api::resource_view_desc view_desc(
      reshade::api::resource_view_type::texture_3d,
      reshade::api::format::r8g8_unorm,
      0,
      UINT32_MAX,
      0,
      UINT32_MAX);
  if (!device->create_resource_view(
          g_isfast_texture,
          reshade::api::resource_usage::shader_resource,
          view_desc,
          &g_isfast_reshade_srv)) {
    LogIsFast(reshade::log::level::error, "Create SRV failed.");
    device->destroy_resource(g_isfast_texture);
    g_isfast_texture = {0u};
    return false;
  }
  LogIsFast(reshade::log::level::debug, "Create SRV OK.");

  reshade::api::sampler_desc sampler_desc = {};
  sampler_desc.filter = reshade::api::filter_mode::min_mag_mip_point;
  sampler_desc.address_u = reshade::api::texture_address_mode::wrap;
  sampler_desc.address_v = reshade::api::texture_address_mode::wrap;
  sampler_desc.address_w = reshade::api::texture_address_mode::wrap;

  if (!device->create_sampler(sampler_desc, &g_isfast_reshade_sampler)) {
    LogIsFast(reshade::log::level::error, "Create sampler failed.");
    device->destroy_resource_view(g_isfast_reshade_srv);
    device->destroy_resource(g_isfast_texture);
    g_isfast_reshade_srv = {0u};
    g_isfast_texture = {0u};
    return false;
  }
  LogIsFast(reshade::log::level::debug, "Create point-wrap sampler OK.");

  g_isfast_using_debug_texture = debug_texture;
  g_isfast_bind_logged = false;
  return true;
}

void OnInitDevice(reshade::api::device* device) {
  if (device == nullptr) return;
  if (!IsIsFastSupportedDevice(device)) return;
  device->create_private_data<DeviceData>();
  if (auto* data = device->get_private_data<DeviceData>(); data != nullptr) {
    data->present_frame_index = 0u;
    data->last_gtao_frame = kInvalidFrameIndex;
    data->last_ao_hook_frame = kInvalidFrameIndex;
    data->last_copyback_frame = kInvalidFrameIndex;
    data->last_gtao_failure_log_frame = kInvalidFrameIndex;
    data->xegtao_mrt_normal_frame = kInvalidFrameIndex;
    data->xegtao_mrt_normal_valid = false;
    data->copyback_succeeded = false;
    data->xegtao_copyback_frame = kInvalidFrameIndex;
    data->xegtao_copyback_requested_for_frame = false;
    data->xegtao_copyback_succeeded_for_frame = false;
    data->xegtao_copyback_active_for_apply = false;
    data->xegtao_result_signature_valid = false;
    data->xegtao_result_signature_frame = kInvalidFrameIndex;
    data->xegtao_result_t3_resource_handle = 0u;
    data->xegtao_result_t4_resource_handle = 0u;
    data->xegtao_result_t3_view_handle = 0u;
    data->xegtao_result_t4_view_handle = 0u;
    data->xegtao_result_t3_width = 0u;
    data->xegtao_result_t3_height = 0u;
    data->xegtao_result_t4_width = 0u;
    data->xegtao_result_t4_height = 0u;
    data->xegtao_result_working_width = 0u;
    data->xegtao_result_working_height = 0u;
    data->xegtao_consume_signature_valid = false;
    data->xegtao_consume_signature_frame = kInvalidFrameIndex;
    data->xegtao_consume_t3_resource_handle = 0u;
    data->xegtao_consume_t4_resource_handle = 0u;
    data->xegtao_consume_t3_width = 0u;
    data->xegtao_consume_t3_height = 0u;
    data->xegtao_consume_t4_width = 0u;
    data->xegtao_consume_t4_height = 0u;
    data->xegtao_consume_working_width = 0u;
    data->xegtao_consume_working_height = 0u;
    data->xegtao_consume_owner_valid = false;
    data->xegtao_consume_owner_frame = kInvalidFrameIndex;
    data->xegtao_consume_owner_shader_hash = 0u;
    data->xegtao_consume_owner_draw_ordinal = 0u;
    data->xegtao_consume_owner_gate_signature = 0u;
    data->xegtao_consume_owner_downscaled = false;
    data->xegtao_lighting_draw_counter_frame = kInvalidFrameIndex;
    data->xegtao_lighting_draw_counter = 0u;
    data->xegtao_owner_valid = false;
    data->xegtao_owner_frame = kInvalidFrameIndex;
    data->xegtao_owner_shader_hash = 0u;
    data->xegtao_owner_draw_ordinal = 0u;
    data->xegtao_owner_gate_signature = 0u;
    data->xegtao_owner_downscaled = false;
    data->xegtao_deferred_dispatch_pending = false;
    data->xegtao_deferred_dispatch_frame = kInvalidFrameIndex;
    data->xegtao_deferred_dispatch_executed = false;
    data->xegtao_deferred_gate_signature = 0u;
    data->xegtao_deferred_owner_shader_hash = 0u;
    data->xegtao_deferred_owner_draw_ordinal = 0u;
    data->xegtao_deferred_depth_srv = {};
    data->xegtao_deferred_ssao_srv = {};
    data->xegtao_deferred_mrt_normal_srv = {};
    data->xegtao_deferred_scene_cbv = {};
    data->xegtao_deferred_scene_cbv_valid = false;
    data->xegtao_deferred_scene_cbv_frame = kInvalidFrameIndex;
    data->xegtao_deferred_scene_cbv_source = XeGTAOSceneCbvSource::kNone;
    data->xegtao_deferred_resolved_scene_cbv_from_current_bindings = false;
    data->xegtao_deferred_drop_log_frame = kInvalidFrameIndex;
    data->xegtao_volfog_seen_frame = kInvalidFrameIndex;
    data->last_owner_state_valid = false;
    data->last_owner_diag_hash = 0u;
    data->fallback_scene_cbv = {};
    data->fallback_scene_cbv_seen = false;
    data->fallback_scene_cbv_frame = kInvalidFrameIndex;
    data->fallback_scene_cbv_signature = 0u;
    data->fallback_scene_cbv_stable_count = 0u;
    data->captured_scene_cbv_source = XeGTAOSceneCbvSource::kNone;
    data->resolved_scene_cbv_from_current_bindings = false;
    data->character_sss_current_frame = kInvalidFrameIndex;
    data->last_capture_diag_log_frame = kInvalidFrameIndex;
    data->last_gate_state_valid = false;
    data->last_gate_passed = false;
    data->last_gate_diag_hash = 0u;
    data->last_apply_gate_main_state_valid = false;
    data->last_apply_gate_main_passed = false;
    data->last_apply_gate_main_diag_hash = 0u;
    data->last_apply_gate_soft_state_valid = false;
    data->last_apply_gate_soft_passed = false;
    data->last_apply_gate_soft_diag_hash = 0u;
    data->last_apply_path_main_state_valid = false;
    data->last_apply_path_main_hash = 0u;
    data->last_apply_path_soft_state_valid = false;
    data->last_apply_path_soft_hash = 0u;
    data->xegtao_warmup_signature = 0u;
    data->xegtao_warmup_stable_count = 0u;
    data->last_warmup_enter_signature = 0u;
    data->last_warmup_complete_signature = 0u;
    data->xegtao_dispatch_isolation_active = false;
    data->xegtao_dispatch_restore_mismatch = false;
    data->xegtao_trace_frame = kInvalidFrameIndex;
    data->xegtao_trace_dispatch_attempted = false;
    data->xegtao_trace_dispatch_succeeded = false;
    data->xegtao_trace_main_pass_executed = false;
    data->xegtao_trace_composite_pass_executed = false;
    data->xegtao_trace_t22_bind_executed = false;
    data->xegtao_trace_copyback_requested = false;
    data->xegtao_trace_copyback_succeeded = false;
    data->xegtao_trace_apply_gate_passed = false;
    data->xegtao_trace_probe_a_active = false;
    data->xegtao_trace_probe_b_active = false;
    data->xegtao_trace_owner_draw_ordinal = 0u;
    data->xegtao_trace_state_valid = false;
    data->xegtao_trace_diag_hash = 0u;
  }
  ReloadIsFastResources(device, "init_device");
}

void OnDestroyDevice(reshade::api::device* device) {
  if (!IsIsFastSupportedDevice(device)) return;
  LogIsFast(reshade::log::level::debug, "OnDestroyDevice begin.");

  // Teardown can run while runtime internals are already being dismantled,
  // so avoid API-side destroy/free calls here and just drop private data.
  g_character_mrt0_view.store(0u, std::memory_order_relaxed);
  g_lighting_mrt0_view.store(0u, std::memory_order_relaxed);

  g_isfast_texture = {0u};
  g_isfast_reshade_srv = {0u};
  g_isfast_reshade_sampler = {0u};
  g_isfast_bind_logged = false;
  g_isfast_using_debug_texture = false;
  g_isfast_bind_failed_logged = false;
  g_isfast_compute_bind_logged = false;

  device->destroy_private_data<DeviceData>();
  LogIsFast(reshade::log::level::debug, "OnDestroyDevice complete. isfast_noise_bound=0.");
}

void InvalidateXeGTAOSwapchainHandles(DeviceData* data) {
  if (data == nullptr) return;

  data->depth_mips_texture = {};
  data->depth_mips_srv = {};
  data->depth_mips_uavs = {};

  data->ao_term_a_texture = {};
  data->ao_term_a_srv = {};
  data->ao_term_a_uav = {};

  data->ao_term_b_texture = {};
  data->ao_term_b_srv = {};
  data->ao_term_b_uav = {};

  data->edges_texture = {};
  data->edges_srv = {};
  data->edges_uav = {};

  data->composite_texture = {};
  data->composite_srv = {};
  data->composite_uav = {};
  data->composite_desc = {};

  data->working_width = 0u;
  data->working_height = 0u;
  data->working_precision = std::numeric_limits<uint32_t>::max();
  data->working_ao_format = reshade::api::format::unknown;

  data->xegtao_prefilter_descriptor_tables = {};
  data->xegtao_main_descriptor_tables = {};
  data->xegtao_denoise_descriptor_tables = {};
  data->xegtao_composite_descriptor_tables = {};
  data->xegtao_normal_cap_descriptor_tables = {};

  data->character_sss_current_resource = {};
  data->character_sss_current_srv = {};
  data->character_sss_last_valid_resource = {};
  data->character_sss_last_valid_srv = {};
  data->character_sss_current_frame = kInvalidFrameIndex;

  data->point_clamp_sampler = {};
}

void OnInitSwapchain(reshade::api::swapchain* swapchain, bool resize) {
  if (swapchain == nullptr) return;
  auto* device = swapchain->get_device();
  if (!IsIsFastSupportedDevice(device)) return;
  auto* data = device->get_private_data<DeviceData>();
  if (data == nullptr) return;

  AddonLog(
      reshade::log::level::info,
      std::format(
          "XeGTAO swapchain init (resize={}, frame={})",
          resize ? 1 : 0,
          data->present_frame_index));

  data->xegtao_resize_guard_until_frame = resize
      ? data->present_frame_index + kXeGTAOResizeDispatchGuardFrames
      : 0u;
  data->xegtao_resize_guard_log_frame = kInvalidFrameIndex;
  g_character_mrt0_view.store(0u, std::memory_order_relaxed);
  g_lighting_mrt0_view.store(0u, std::memory_order_relaxed);

  if (!resize) {
    DestroyXeGTAOResources(device, data);
    DestroyXeGTAODescriptorTables(device, data);
    DestroyDedicatedSssViews(device, data);
  } else {
    InvalidateXeGTAOSwapchainHandles(data);
  }
  data->captured_mrt_normal_srv = {};
  data->last_gtao_frame = kInvalidFrameIndex;
  data->last_ao_hook_frame = kInvalidFrameIndex;
  data->last_copyback_frame = kInvalidFrameIndex;
  data->xegtao_mrt_normal_frame = kInvalidFrameIndex;
  data->xegtao_mrt_normal_valid = false;
  data->character_sss_current_frame = kInvalidFrameIndex;
  data->copyback_succeeded = false;
  data->xegtao_copyback_frame = kInvalidFrameIndex;
  data->xegtao_copyback_requested_for_frame = false;
  data->xegtao_copyback_succeeded_for_frame = false;
  data->xegtao_copyback_active_for_apply = false;
  data->xegtao_result_signature_valid = false;
  data->xegtao_result_signature_frame = kInvalidFrameIndex;
  data->xegtao_result_t3_resource_handle = 0u;
  data->xegtao_result_t4_resource_handle = 0u;
  data->xegtao_result_t3_view_handle = 0u;
  data->xegtao_result_t4_view_handle = 0u;
  data->xegtao_result_t3_width = 0u;
  data->xegtao_result_t3_height = 0u;
  data->xegtao_result_t4_width = 0u;
  data->xegtao_result_t4_height = 0u;
  data->xegtao_result_working_width = 0u;
  data->xegtao_result_working_height = 0u;
  data->xegtao_consume_signature_valid = false;
  data->xegtao_consume_signature_frame = kInvalidFrameIndex;
  data->xegtao_consume_t3_resource_handle = 0u;
  data->xegtao_consume_t4_resource_handle = 0u;
  data->xegtao_consume_t3_width = 0u;
  data->xegtao_consume_t3_height = 0u;
  data->xegtao_consume_t4_width = 0u;
  data->xegtao_consume_t4_height = 0u;
  data->xegtao_consume_working_width = 0u;
  data->xegtao_consume_working_height = 0u;
  data->xegtao_consume_owner_valid = false;
  data->xegtao_consume_owner_frame = kInvalidFrameIndex;
  data->xegtao_consume_owner_shader_hash = 0u;
  data->xegtao_consume_owner_draw_ordinal = 0u;
  data->xegtao_consume_owner_gate_signature = 0u;
  data->xegtao_consume_owner_downscaled = false;
  data->xegtao_lighting_draw_counter_frame = kInvalidFrameIndex;
  data->xegtao_lighting_draw_counter = 0u;
  data->xegtao_owner_valid = false;
  data->xegtao_owner_frame = kInvalidFrameIndex;
  data->xegtao_owner_shader_hash = 0u;
  data->xegtao_owner_draw_ordinal = 0u;
  data->xegtao_owner_gate_signature = 0u;
  data->xegtao_owner_downscaled = false;
  data->xegtao_deferred_dispatch_pending = false;
  data->xegtao_deferred_dispatch_frame = kInvalidFrameIndex;
  data->xegtao_deferred_dispatch_executed = false;
  data->xegtao_deferred_gate_signature = 0u;
  data->xegtao_deferred_owner_shader_hash = 0u;
  data->xegtao_deferred_owner_draw_ordinal = 0u;
  data->xegtao_deferred_depth_srv = {};
  data->xegtao_deferred_ssao_srv = {};
  data->xegtao_deferred_mrt_normal_srv = {};
  data->xegtao_deferred_scene_cbv = {};
  data->xegtao_deferred_scene_cbv_valid = false;
  data->xegtao_deferred_scene_cbv_frame = kInvalidFrameIndex;
  data->xegtao_deferred_scene_cbv_source = XeGTAOSceneCbvSource::kNone;
  data->xegtao_deferred_resolved_scene_cbv_from_current_bindings = false;
  data->xegtao_deferred_drop_log_frame = kInvalidFrameIndex;
  data->xegtao_debug_descriptor_reject_count = 0u;
  data->xegtao_debug_last_descriptor_reject_frame = kInvalidFrameIndex;
  data->xegtao_debug_predispatch_reject_count = 0u;
  data->xegtao_debug_last_predispatch_reject_frame = kInvalidFrameIndex;
  data->xegtao_debug_deferred_drop_count = 0u;
  data->xegtao_debug_last_deferred_drop_frame = kInvalidFrameIndex;
  data->xegtao_debug_normal_fallback_count = 0u;
  data->xegtao_debug_last_normal_fallback_frame = kInvalidFrameIndex;
  data->xegtao_volfog_seen_frame = kInvalidFrameIndex;
  data->last_owner_state_valid = false;
  data->last_owner_diag_hash = 0u;
  data->captured_scene_cbv_source = XeGTAOSceneCbvSource::kNone;
  data->resolved_scene_cbv_from_current_bindings = false;
  data->fallback_scene_cbv = {};
  data->fallback_scene_cbv_seen = false;
  data->fallback_scene_cbv_frame = kInvalidFrameIndex;
  data->fallback_scene_cbv_signature = 0u;
  data->fallback_scene_cbv_stable_count = 0u;
  data->last_capture_diag_log_frame = kInvalidFrameIndex;
  data->last_gate_state_valid = false;
  data->last_gate_passed = false;
  data->last_gate_diag_hash = 0u;
  data->last_apply_gate_main_state_valid = false;
  data->last_apply_gate_main_passed = false;
  data->last_apply_gate_main_diag_hash = 0u;
  data->last_apply_gate_soft_state_valid = false;
  data->last_apply_gate_soft_passed = false;
  data->last_apply_gate_soft_diag_hash = 0u;
  data->last_apply_path_main_state_valid = false;
  data->last_apply_path_main_hash = 0u;
  data->last_apply_path_soft_state_valid = false;
  data->last_apply_path_soft_hash = 0u;
  data->xegtao_warmup_signature = 0u;
  data->xegtao_warmup_stable_count = 0u;
  data->last_warmup_enter_signature = 0u;
  data->last_warmup_complete_signature = 0u;
  data->xegtao_dispatch_isolation_active = false;
  data->xegtao_dispatch_restore_mismatch = false;
  data->xegtao_trace_frame = kInvalidFrameIndex;
  data->xegtao_trace_dispatch_attempted = false;
  data->xegtao_trace_dispatch_succeeded = false;
  data->xegtao_trace_main_pass_executed = false;
  data->xegtao_trace_composite_pass_executed = false;
  data->xegtao_trace_t22_bind_executed = false;
  data->xegtao_trace_copyback_requested = false;
  data->xegtao_trace_copyback_succeeded = false;
  data->xegtao_trace_apply_gate_passed = false;
  data->xegtao_trace_probe_a_active = false;
  data->xegtao_trace_probe_b_active = false;
  data->xegtao_trace_owner_draw_ordinal = 0u;
  data->xegtao_trace_state_valid = false;
  data->xegtao_trace_diag_hash = 0u;
  data->last_logged_depth_view_handle = 0u;
  data->last_logged_ssao_view_handle = 0u;
  data->last_logged_mrt_normal_view_handle = 0u;
  data->last_logged_depth_width = 0u;
  data->last_logged_depth_height = 0u;
  data->last_logged_ssao_width = 0u;
  data->last_logged_ssao_height = 0u;
  data->last_logged_mrt_normal_width = 0u;
  data->last_logged_mrt_normal_height = 0u;
  data->last_logged_depth_view_format = reshade::api::format::unknown;
  data->last_logged_depth_resource_format = reshade::api::format::unknown;
  data->last_logged_ssao_view_format = reshade::api::format::unknown;
  data->last_logged_ssao_resource_format = reshade::api::format::unknown;
  data->last_logged_mrt_normal_view_format = reshade::api::format::unknown;
  data->last_logged_mrt_normal_resource_format = reshade::api::format::unknown;
  data->last_logged_scene_cbv_source = XeGTAOSceneCbvSource::kNone;
  data->last_logged_scene_cbv_buffer_handle = 0u;
  data->last_logged_scene_cbv_valid = false;
  data->last_logged_fallback_scene_cbv_seen = false;
  data->last_logged_fallback_scene_cbv_buffer_handle = 0u;
  data->last_logged_fallback_scene_cbv_frame = kInvalidFrameIndex;
  data->last_logged_fallback_scene_cbv_stable_count = 0u;
}

void OnDestroySwapchain(reshade::api::swapchain* swapchain, bool resize) {
  if (swapchain == nullptr) return;
  auto* device = swapchain->get_device();
  if (!IsIsFastSupportedDevice(device)) return;

  // Resize teardown may happen while runtime state is unstable.
  // Keep this path non-destructive and only invalidate global handles.
  if (resize) {
    g_character_mrt0_view.store(0u, std::memory_order_relaxed);
    g_lighting_mrt0_view.store(0u, std::memory_order_relaxed);
    AddonLog(reshade::log::level::info, "XeGTAO swapchain destroy (resize=1)");
    return;
  }

  auto* data = device->get_private_data<DeviceData>();
  if (data == nullptr) return;

  AddonLog(
      reshade::log::level::info,
      std::format(
          "XeGTAO swapchain destroy (resize={}, frame={})",
          resize ? 1 : 0,
          data->present_frame_index));

  data->xegtao_resize_guard_until_frame = 0u;
  data->xegtao_resize_guard_log_frame = kInvalidFrameIndex;
  g_character_mrt0_view.store(0u, std::memory_order_relaxed);
  g_lighting_mrt0_view.store(0u, std::memory_order_relaxed);

  DestroyXeGTAOResources(device, data);
  DestroyXeGTAODescriptorTables(device, data);
  DestroyDedicatedSssViews(device, data);
  data->captured_mrt_normal_srv = {};
  data->captured_scene_cbv_source = XeGTAOSceneCbvSource::kNone;
  data->resolved_scene_cbv_from_current_bindings = false;
  data->fallback_scene_cbv = {};
  data->fallback_scene_cbv_seen = false;
  data->fallback_scene_cbv_frame = kInvalidFrameIndex;
  data->fallback_scene_cbv_signature = 0u;
  data->fallback_scene_cbv_stable_count = 0u;
  data->last_copyback_frame = kInvalidFrameIndex;
  data->copyback_succeeded = false;
  data->xegtao_copyback_frame = kInvalidFrameIndex;
  data->xegtao_copyback_requested_for_frame = false;
  data->xegtao_copyback_succeeded_for_frame = false;
  data->xegtao_copyback_active_for_apply = false;
  data->xegtao_result_signature_valid = false;
  data->xegtao_result_signature_frame = kInvalidFrameIndex;
  data->xegtao_result_t3_resource_handle = 0u;
  data->xegtao_result_t4_resource_handle = 0u;
  data->xegtao_result_t3_view_handle = 0u;
  data->xegtao_result_t4_view_handle = 0u;
  data->xegtao_result_t3_width = 0u;
  data->xegtao_result_t3_height = 0u;
  data->xegtao_result_t4_width = 0u;
  data->xegtao_result_t4_height = 0u;
  data->xegtao_result_working_width = 0u;
  data->xegtao_result_working_height = 0u;
  data->xegtao_consume_signature_valid = false;
  data->xegtao_consume_signature_frame = kInvalidFrameIndex;
  data->xegtao_consume_t3_resource_handle = 0u;
  data->xegtao_consume_t4_resource_handle = 0u;
  data->xegtao_consume_t3_width = 0u;
  data->xegtao_consume_t3_height = 0u;
  data->xegtao_consume_t4_width = 0u;
  data->xegtao_consume_t4_height = 0u;
  data->xegtao_consume_working_width = 0u;
  data->xegtao_consume_working_height = 0u;
  data->xegtao_consume_owner_valid = false;
  data->xegtao_consume_owner_frame = kInvalidFrameIndex;
  data->xegtao_consume_owner_shader_hash = 0u;
  data->xegtao_consume_owner_draw_ordinal = 0u;
  data->xegtao_consume_owner_gate_signature = 0u;
  data->xegtao_consume_owner_downscaled = false;
  data->xegtao_lighting_draw_counter_frame = kInvalidFrameIndex;
  data->xegtao_lighting_draw_counter = 0u;
  data->xegtao_owner_valid = false;
  data->xegtao_owner_frame = kInvalidFrameIndex;
  data->xegtao_owner_shader_hash = 0u;
  data->xegtao_owner_draw_ordinal = 0u;
  data->xegtao_owner_gate_signature = 0u;
  data->xegtao_owner_downscaled = false;
  data->xegtao_deferred_dispatch_pending = false;
  data->xegtao_deferred_dispatch_frame = kInvalidFrameIndex;
  data->xegtao_deferred_dispatch_executed = false;
  data->xegtao_deferred_gate_signature = 0u;
  data->xegtao_deferred_owner_shader_hash = 0u;
  data->xegtao_deferred_owner_draw_ordinal = 0u;
  data->xegtao_deferred_depth_srv = {};
  data->xegtao_deferred_ssao_srv = {};
  data->xegtao_deferred_mrt_normal_srv = {};
  data->xegtao_deferred_scene_cbv = {};
  data->xegtao_deferred_scene_cbv_valid = false;
  data->xegtao_deferred_scene_cbv_frame = kInvalidFrameIndex;
  data->xegtao_deferred_scene_cbv_source = XeGTAOSceneCbvSource::kNone;
  data->xegtao_deferred_resolved_scene_cbv_from_current_bindings = false;
  data->xegtao_deferred_drop_log_frame = kInvalidFrameIndex;
  data->xegtao_volfog_seen_frame = kInvalidFrameIndex;
  data->last_owner_state_valid = false;
  data->last_owner_diag_hash = 0u;
  data->last_capture_diag_log_frame = kInvalidFrameIndex;
  data->last_gate_state_valid = false;
  data->last_gate_passed = false;
  data->last_gate_diag_hash = 0u;
  data->last_apply_gate_main_state_valid = false;
  data->last_apply_gate_main_passed = false;
  data->last_apply_gate_main_diag_hash = 0u;
  data->last_apply_gate_soft_state_valid = false;
  data->last_apply_gate_soft_passed = false;
  data->last_apply_gate_soft_diag_hash = 0u;
  data->last_apply_path_main_state_valid = false;
  data->last_apply_path_main_hash = 0u;
  data->last_apply_path_soft_state_valid = false;
  data->last_apply_path_soft_hash = 0u;
  data->xegtao_warmup_signature = 0u;
  data->xegtao_warmup_stable_count = 0u;
  data->last_warmup_enter_signature = 0u;
  data->last_warmup_complete_signature = 0u;
  data->xegtao_dispatch_isolation_active = false;
  data->xegtao_dispatch_restore_mismatch = false;
  data->xegtao_trace_frame = kInvalidFrameIndex;
  data->xegtao_trace_dispatch_attempted = false;
  data->xegtao_trace_dispatch_succeeded = false;
  data->xegtao_trace_main_pass_executed = false;
  data->xegtao_trace_composite_pass_executed = false;
  data->xegtao_trace_t22_bind_executed = false;
  data->xegtao_trace_copyback_requested = false;
  data->xegtao_trace_copyback_succeeded = false;
  data->xegtao_trace_apply_gate_passed = false;
  data->xegtao_trace_probe_a_active = false;
  data->xegtao_trace_probe_b_active = false;
  data->xegtao_trace_owner_draw_ordinal = 0u;
  data->xegtao_trace_state_valid = false;
  data->xegtao_trace_diag_hash = 0u;
  data->last_logged_scene_cbv_source = XeGTAOSceneCbvSource::kNone;
  data->last_logged_scene_cbv_buffer_handle = 0u;
  data->last_logged_scene_cbv_valid = false;
  data->last_logged_fallback_scene_cbv_seen = false;
  data->last_logged_fallback_scene_cbv_buffer_handle = 0u;
  data->last_logged_fallback_scene_cbv_frame = kInvalidFrameIndex;
  data->last_logged_fallback_scene_cbv_stable_count = 0u;
}

void ReloadIsFastResources(reshade::api::device* device, const char* reason) {
  if (device == nullptr) return;
  if (!IsIsFastSupportedDevice(device)) return;

  const bool use_fast_texture = isfast_texture_source_enabled >= 0.5f;
  g_isfast_mode_use_fast_texture = use_fast_texture;

  {
    std::ostringstream message;
    message << "Reload requested (" << (reason != nullptr ? reason : "unknown")
            << "), IS-FAST texture mode=" << (use_fast_texture ? "On" : "Off");
    LogIsFast(reshade::log::level::info, message.str());
  }

  DestroyIsFastResources(device);

  if (!use_fast_texture) {
    LogIsFast(reshade::log::level::info, "IS-FAST texture mode Off: forcing debug 1x1x1 test texture.");

    std::array<uint8_t, 2> debug_texel = {
        static_cast<uint8_t>(0.75f * 255.0f + 0.5f),
        static_cast<uint8_t>(0.75f * 255.0f + 0.5f),
    };
    const bool debug_created = CreateIsFastTexture(
        device,
        1u,
        1u,
        1u,
        debug_texel.data(),
        2u,
        2u,
        true);
    if (debug_created) {
      shader_injection.isfast_noise_bound = 1.f;
      LogIsFast(reshade::log::level::info, "Debug 1x1x1 test texture created OK.");
    } else {
      shader_injection.isfast_noise_bound = 0.f;
      LogIsFast(reshade::log::level::error, "Failed to create debug 1x1x1 test texture.");
    }
    return;
  }

  const std::filesystem::path dds_path = GetModuleDirectory() / L"fast_noise_ea.dds";
  {
    std::ostringstream message;
    message << "Searching for fast_noise_ea.dds at: " << dds_path.string();
    LogIsFast(reshade::log::level::debug, message.str());
  }

  std::vector<uint8_t> dds_bytes;
  std::span<const uint8_t> pixel_data;
  bool loaded_noise = false;

  if (ReadFileBytes(dds_path, &dds_bytes)) {
    {
      std::ostringstream message;
      message << "Read DDS file OK (" << dds_bytes.size() << " bytes).";
      LogIsFast(reshade::log::level::debug, message.str());
    }

    if (ValidateAndExtractIsFastDds(dds_bytes, &pixel_data)) {
      const uint32_t row_pitch = kExpectedIsFastWidth * kIsFastBytesPerTexel;
      const uint32_t slice_pitch = kExpectedIsFastWidth * kExpectedIsFastHeight * kIsFastBytesPerTexel;
      loaded_noise = CreateIsFastTexture(
          device,
          kExpectedIsFastWidth,
          kExpectedIsFastHeight,
          kExpectedIsFastDepth,
          const_cast<uint8_t*>(pixel_data.data()),
          row_pitch,
          slice_pitch,
          false);
      if (loaded_noise) {
        shader_injection.isfast_noise_bound = 1.f;
        LogIsFast(reshade::log::level::info, "Loaded noise texture (128x128x32 RG8_UNORM).");
      } else {
        shader_injection.isfast_noise_bound = 0.f;
        LogIsFast(reshade::log::level::error, "Failed to create GPU resources from validated DDS.");
      }
    } else {
      shader_injection.isfast_noise_bound = 0.f;
      LogIsFast(reshade::log::level::warning, "DDS validation failed. IS-FAST noise is disabled.");
    }
  } else {
    shader_injection.isfast_noise_bound = 0.f;
    LogIsFast(reshade::log::level::warning, "Could not read fast_noise_ea.dds. IS-FAST noise is disabled.");
  }

  if (!loaded_noise) {
    std::array<uint8_t, 2> debug_texel = {
        static_cast<uint8_t>(0.75f * 255.0f + 0.5f),
        static_cast<uint8_t>(0.75f * 255.0f + 0.5f),
    };
    const bool debug_created = CreateIsFastTexture(
        device,
        1u,
        1u,
        1u,
        debug_texel.data(),
        2u,
        2u,
        true);
    if (debug_created) {
      LogIsFast(reshade::log::level::info, "Debug 1x1x1 test texture created OK.");
    } else {
      LogIsFast(reshade::log::level::error, "Failed to create debug 1x1x1 test texture.");
    }
  }
}

bool IsViewAlive(reshade::api::device* device, const reshade::api::resource_view view) {
  if (device == nullptr || view.handle == 0u) return false;
  return device->get_resource_from_view(view).handle != 0u;
}

struct XeGTAOCapturedViewInfo {
  reshade::api::resource_view view = {};
  reshade::api::resource resource = {};
  reshade::api::resource_desc resource_desc = {};
  reshade::api::resource_view_desc view_desc = {};
  bool alive = false;
  uint32_t width = 0u;
  uint32_t height = 0u;
};

XeGTAOCapturedViewInfo GetXeGTAOCapturedViewInfo(
    reshade::api::device* device,
    reshade::api::resource_view view) {
  XeGTAOCapturedViewInfo info = {};
  info.view = view;
  if (device == nullptr || view.handle == 0u) return info;

  info.resource = device->get_resource_from_view(view);
  if (info.resource.handle == 0u) return info;

  info.alive = true;
  info.resource_desc = device->get_resource_desc(info.resource);
  info.view_desc = device->get_resource_view_desc(view);
  if (info.resource_desc.type == reshade::api::resource_type::texture_2d) {
    info.width = info.resource_desc.texture.width;
    info.height = info.resource_desc.texture.height;
  }
  return info;
}

std::string FormatXeGTAOCapturedViewInfo(const char* label, const XeGTAOCapturedViewInfo& info) {
  std::ostringstream stream;
  stream << label << "(view=0x" << std::hex << info.view.handle << std::dec;
  if (!info.alive) {
    stream << ", alive=0)";
    return stream.str();
  }

  const auto resource_format = info.resource_desc.type == reshade::api::resource_type::texture_2d
      ? info.resource_desc.texture.format
      : reshade::api::format::unknown;
  auto view_format = info.view_desc.format;
  if (view_format == reshade::api::format::unknown && resource_format != reshade::api::format::unknown) {
    view_format = reshade::api::format_to_default_typed(resource_format);
  }

  stream << ", alive=1";
  stream << ", res=0x" << std::hex << info.resource.handle << std::dec;
  stream << ", type=" << static_cast<uint32_t>(info.resource_desc.type);
  if (info.width != 0u || info.height != 0u) {
    stream << ", size=" << info.width << "x" << info.height;
  }
  stream << ", view_fmt=" << static_cast<uint32_t>(view_format);
  stream << ", res_fmt=" << static_cast<uint32_t>(resource_format);
  stream << ")";
  return stream.str();
}

const char* GetXeGTAOSceneCbvSourceName(XeGTAOSceneCbvSource source) {
  switch (source) {
    case XeGTAOSceneCbvSource::kCurrentLighting:
      return "current_lighting";
    case XeGTAOSceneCbvSource::kFallback:
      return "fallback";
    default:
      return "none";
  }
}

std::string FormatXeGTAOSceneCbvInfo(const DeviceData* data) {
  if (data == nullptr) return "b0(source=none, valid=0)";
  std::ostringstream stream;
  stream << "b0(source=" << GetXeGTAOSceneCbvSourceName(data->captured_scene_cbv_source)
         << ", valid=" << (data->captured_scene_cbv_valid ? 1 : 0);
  if (data->captured_scene_cbv_valid && data->captured_scene_cbv.buffer.handle != 0u) {
    stream << ", buffer=0x" << std::hex << data->captured_scene_cbv.buffer.handle << std::dec
           << ", offset=" << data->captured_scene_cbv.offset
           << ", size=" << data->captured_scene_cbv.size
           << ", frame=" << data->captured_scene_cbv_frame;
  }
  stream << ")";
  return stream.str();
}

std::string FormatXeGTAOResultSignatureInfo(const DeviceData* data) {
  if (data == nullptr || !data->xegtao_result_signature_valid) {
    return "signature(valid=0)";
  }
  std::ostringstream stream;
  stream << "signature(valid=1, frame=" << data->xegtao_result_signature_frame
         << ", t3(res=0x" << std::hex << data->xegtao_result_t3_resource_handle
         << ", view=0x" << data->xegtao_result_t3_view_handle << std::dec
         << ", size=" << data->xegtao_result_t3_width << "x" << data->xegtao_result_t3_height << ")"
         << ", t4(res=0x" << std::hex << data->xegtao_result_t4_resource_handle
         << ", view=0x" << data->xegtao_result_t4_view_handle << std::dec
         << ", size=" << data->xegtao_result_t4_width << "x" << data->xegtao_result_t4_height << "))";
  return stream.str();
}

uint32_t RoundToUint(float value) {
  const float positive = std::max(0.0f, value);
  return static_cast<uint32_t>(positive + 0.5f);
}

uint64_t HashCombineU64(uint64_t seed, uint64_t value) {
  seed ^= value + 0x9e3779b97f4a7c15ull + (seed << 6u) + (seed >> 2u);
  return seed;
}

uint64_t HashString(const std::string& value) {
  uint64_t seed = 1469598103934665603ull;
  for (const char c : value) {
    seed ^= static_cast<uint8_t>(c);
    seed *= 1099511628211ull;
  }
  return seed;
}

uint32_t AbsDiffU32(uint32_t a, uint32_t b) {
  return a > b ? (a - b) : (b - a);
}

bool IsCloseWithinOnePixel(uint32_t a, uint32_t b) {
  return AbsDiffU32(a, b) <= 1u;
}

const char* ClassifyXeGTAOT4ScaleClass(
    uint32_t viewport_width,
    uint32_t viewport_height,
    uint32_t t4_width,
    uint32_t t4_height) {
  if (viewport_width == 0u || viewport_height == 0u || t4_width == 0u || t4_height == 0u) return "invalid";

  if (IsCloseWithinOnePixel(t4_width, viewport_width) && IsCloseWithinOnePixel(t4_height, viewport_height)) {
    return "1x";
  }

  const uint32_t half_width = RoundToUint(static_cast<float>(viewport_width) * 0.5f);
  const uint32_t half_height = RoundToUint(static_cast<float>(viewport_height) * 0.5f);
  if (IsCloseWithinOnePixel(t4_width, half_width) && IsCloseWithinOnePixel(t4_height, half_height)) {
    return "1/2";
  }

  const uint32_t quarter_width = RoundToUint(static_cast<float>(viewport_width) * 0.25f);
  const uint32_t quarter_height = RoundToUint(static_cast<float>(viewport_height) * 0.25f);
  if (IsCloseWithinOnePixel(t4_width, quarter_width) && IsCloseWithinOnePixel(t4_height, quarter_height)) {
    return "1/4";
  }

  return "invalid";
}

uint32_t GetXeGTAOT4ScaleClassCode(const char* scale_class) {
  if (scale_class == nullptr) return 0u;
  if (std::strcmp(scale_class, "1x") == 0) return 1u;
  if (std::strcmp(scale_class, "1/2") == 0) return 2u;
  if (std::strcmp(scale_class, "1/4") == 0) return 4u;
  return 0u;
}

uint64_t BuildXeGTAODrawSignature(
    uint32_t viewport_width,
    uint32_t viewport_height,
    const XeGTAOCapturedViewInfo& rtv_info,
    const XeGTAOCapturedViewInfo& depth_info,
    const XeGTAOCapturedViewInfo& ssao_info,
    const char* t4_scale_class) {
  uint64_t signature = 1469598103934665603ull;
  signature = HashCombineU64(signature, viewport_width);
  signature = HashCombineU64(signature, viewport_height);
  signature = HashCombineU64(signature, rtv_info.view.handle);
  signature = HashCombineU64(signature, rtv_info.width);
  signature = HashCombineU64(signature, rtv_info.height);
  signature = HashCombineU64(signature, depth_info.view.handle);
  signature = HashCombineU64(signature, depth_info.width);
  signature = HashCombineU64(signature, depth_info.height);
  signature = HashCombineU64(signature, ssao_info.view.handle);
  signature = HashCombineU64(signature, ssao_info.width);
  signature = HashCombineU64(signature, ssao_info.height);
  signature = HashCombineU64(signature, GetXeGTAOT4ScaleClassCode(t4_scale_class));
  return signature;
}

bool IsXeGTAOResultDownscaled(const DeviceData* data) {
  if (data == nullptr || !data->xegtao_result_signature_valid) return false;
  if (data->xegtao_result_t3_width == 0u || data->xegtao_result_t3_height == 0u) return false;
  if (data->xegtao_result_working_width == 0u || data->xegtao_result_working_height == 0u) return false;
  return data->xegtao_result_working_width < data->xegtao_result_t3_width
      || data->xegtao_result_working_height < data->xegtao_result_t3_height;
}

bool IsXeGTAOCurrentInputDownscaled(reshade::api::device* device, const DeviceData* data) {
  if (device == nullptr || data == nullptr) return false;
  if (data->working_width == 0u || data->working_height == 0u) return false;
  const auto depth_info = GetXeGTAOCapturedViewInfo(device, data->captured_depth_srv);
  if (!depth_info.alive) return false;
  if (depth_info.resource_desc.type != reshade::api::resource_type::texture_2d) return false;
  return data->working_width < depth_info.width || data->working_height < depth_info.height;
}

std::string FormatXeGTAOOwnerInfo(const DeviceData* data) {
  if (data == nullptr || !data->xegtao_owner_valid) return "owner(valid=0)";
  std::ostringstream stream;
  stream << "owner(valid=1, frame=" << data->xegtao_owner_frame
         << ", shader=0x" << std::hex << data->xegtao_owner_shader_hash << std::dec
         << ", draw=" << data->xegtao_owner_draw_ordinal
         << ", downscaled=" << (data->xegtao_owner_downscaled ? 1 : 0)
         << ", gate_sig=0x" << std::hex << data->xegtao_owner_gate_signature << std::dec
         << ")";
  return stream.str();
}

bool TryBuildXeGTAODrawSignatureForState(
    reshade::api::command_list* cmd_list,
    reshade::api::device* device,
    const XeGTAOCapturedViewInfo& depth_info,
    const XeGTAOCapturedViewInfo& ssao_info,
    uint64_t* out_signature,
    std::string* out_diag) {
  if (out_signature != nullptr) *out_signature = 0u;
  if (out_diag != nullptr) out_diag->clear();
  if (cmd_list == nullptr || device == nullptr) return false;

  auto* state = renodx::utils::state::GetCurrentState(cmd_list);
  if (state == nullptr || state->viewports.empty() || state->render_targets.empty()) {
    if (out_diag != nullptr) *out_diag = "state/viewport/rtv unavailable";
    return false;
  }
  if (state->render_targets.at(0).handle == 0u) {
    if (out_diag != nullptr) *out_diag = "rtv0 is null";
    return false;
  }

  const auto rtv_info = GetXeGTAOCapturedViewInfo(device, state->render_targets.at(0));
  const auto viewport = state->viewports.at(0);
  const uint32_t viewport_width = RoundToUint(viewport.width);
  const uint32_t viewport_height = RoundToUint(viewport.height);
  const char* t4_scale_class = ClassifyXeGTAOT4ScaleClass(
      viewport_width, viewport_height, ssao_info.width, ssao_info.height);

  bool valid = true;
  const char* invalid_reason = nullptr;
  if (!depth_info.alive || depth_info.resource_desc.type != reshade::api::resource_type::texture_2d) {
    valid = false;
    invalid_reason = "depth view invalid";
  } else if (!ssao_info.alive || ssao_info.resource_desc.type != reshade::api::resource_type::texture_2d) {
    valid = false;
    invalid_reason = "AO view invalid";
  } else if (!rtv_info.alive || rtv_info.resource_desc.type != reshade::api::resource_type::texture_2d) {
    valid = false;
    invalid_reason = "RTV0 invalid";
  } else if (viewport_width == 0u || viewport_height == 0u) {
    valid = false;
    invalid_reason = "viewport size is zero";
  } else if (!IsCloseWithinOnePixel(viewport_width, depth_info.width)
             || !IsCloseWithinOnePixel(viewport_height, depth_info.height)
             || !IsCloseWithinOnePixel(viewport_width, rtv_info.width)
             || !IsCloseWithinOnePixel(viewport_height, rtv_info.height)) {
    valid = false;
    invalid_reason = "viewport/RTV/t3 dimensions mismatch";
  } else if (std::strcmp(t4_scale_class, "invalid") == 0) {
    valid = false;
    invalid_reason = "t4 ratio invalid";
  }

  std::ostringstream diag;
  if (!valid) {
    diag << (invalid_reason != nullptr ? invalid_reason : "invalid");
    diag << ", viewport=" << viewport_width << "x" << viewport_height
         << ", rtv0=" << rtv_info.width << "x" << rtv_info.height
         << ", t3=" << depth_info.width << "x" << depth_info.height
         << ", t4=" << ssao_info.width << "x" << ssao_info.height
         << ", t4_scale_class=" << t4_scale_class;
    if (out_diag != nullptr) *out_diag = diag.str();
    return false;
  }

  if (out_signature != nullptr) {
    *out_signature = BuildXeGTAODrawSignature(
        viewport_width,
        viewport_height,
        rtv_info,
        depth_info,
        ssao_info,
        t4_scale_class);
  }

  diag << "viewport=" << viewport_width << "x" << viewport_height
       << ", rtv0=" << rtv_info.width << "x" << rtv_info.height
       << ", t3=" << depth_info.width << "x" << depth_info.height
       << ", t4=" << ssao_info.width << "x" << ssao_info.height
       << ", t4_scale_class=" << t4_scale_class;
  if (out_diag != nullptr) *out_diag = diag.str();
  return true;
}

bool EvaluateXeGTAODrawCandidate(
    reshade::api::command_list* cmd_list,
    DeviceData* data,
    std::string* out_reason,
    std::string* out_diag,
    uint64_t* out_signature) {
  if (out_reason != nullptr) out_reason->clear();
  if (out_diag != nullptr) out_diag->clear();
  if (out_signature != nullptr) *out_signature = 0u;
  if (cmd_list == nullptr || data == nullptr) {
    if (out_reason != nullptr) *out_reason = "command list or device data is null";
    return false;
  }

  auto* device = cmd_list->get_device();
  if (device == nullptr) {
    if (out_reason != nullptr) *out_reason = "device is null";
    return false;
  }

  auto* state = renodx::utils::state::GetCurrentState(cmd_list);
  if (state == nullptr) {
    if (out_reason != nullptr) *out_reason = "state is unavailable";
    return false;
  }
  if (state->viewports.empty()) {
    if (out_reason != nullptr) *out_reason = "viewport is unavailable";
    return false;
  }
  if (state->render_targets.empty() || state->render_targets.at(0).handle == 0u) {
    if (out_reason != nullptr) *out_reason = "RTV0 is unavailable";
    return false;
  }

  ResolveXeGTAOInputsFromCurrentBindings(cmd_list, data);

  const auto depth_info = GetXeGTAOCapturedViewInfo(device, data->captured_depth_srv);
  const auto ssao_info = GetXeGTAOCapturedViewInfo(device, data->captured_ssao_srv);
  const auto rtv_info = GetXeGTAOCapturedViewInfo(device, state->render_targets.at(0));
  const auto viewport = state->viewports.at(0);
  const uint32_t viewport_width = RoundToUint(viewport.width);
  const uint32_t viewport_height = RoundToUint(viewport.height);
  const char* t4_scale_class = ClassifyXeGTAOT4ScaleClass(
      viewport_width, viewport_height, ssao_info.width, ssao_info.height);

  if (!depth_info.alive || depth_info.resource_desc.type != reshade::api::resource_type::texture_2d) {
    if (out_reason != nullptr) *out_reason = "t3 depth is missing or not texture2D";
  } else if (!ssao_info.alive || ssao_info.resource_desc.type != reshade::api::resource_type::texture_2d) {
    if (out_reason != nullptr) *out_reason = "t4 AO is missing or not texture2D";
  } else if (!rtv_info.alive || rtv_info.resource_desc.type != reshade::api::resource_type::texture_2d) {
    if (out_reason != nullptr) *out_reason = "RTV0 is missing or not texture2D";
  } else if (viewport_width == 0u || viewport_height == 0u) {
    if (out_reason != nullptr) *out_reason = "viewport size is zero";
  } else if (!IsCloseWithinOnePixel(viewport_width, depth_info.width)
      || !IsCloseWithinOnePixel(viewport_height, depth_info.height)
      || !IsCloseWithinOnePixel(viewport_width, rtv_info.width)
      || !IsCloseWithinOnePixel(viewport_height, rtv_info.height)) {
    if (out_reason != nullptr) *out_reason = "viewport/RTV/t3 dimensions do not match";
  } else if (std::strcmp(t4_scale_class, "invalid") == 0) {
    if (out_reason != nullptr) *out_reason = "t4 ratio not allowed";
  } else {
    if (out_signature != nullptr) {
      *out_signature = BuildXeGTAODrawSignature(
          viewport_width,
          viewport_height,
          rtv_info,
          depth_info,
          ssao_info,
          t4_scale_class);
    }
    if (out_diag != nullptr) {
      std::ostringstream diag;
      diag << "viewport=" << viewport_width << "x" << viewport_height
           << ", rtv0=" << rtv_info.width << "x" << rtv_info.height
           << ", t3=" << depth_info.width << "x" << depth_info.height
           << ", t4=" << ssao_info.width << "x" << ssao_info.height
           << ", t4_scale_class=" << t4_scale_class;
      *out_diag = diag.str();
    }
    return true;
  }

  if (out_diag != nullptr) {
    std::ostringstream diag;
    diag << "viewport=" << viewport_width << "x" << viewport_height
         << ", rtv0=" << rtv_info.width << "x" << rtv_info.height
         << ", t3=" << depth_info.width << "x" << depth_info.height
         << ", t4=" << ssao_info.width << "x" << ssao_info.height
         << ", t4_scale_class=" << t4_scale_class;
    *out_diag = diag.str();
  }

  return false;
}

void LogXeGTAOCaptureDiagnostics(reshade::api::device* device, DeviceData* data) {
  if (device == nullptr || data == nullptr) return;
  if (data->last_capture_diag_log_frame == data->present_frame_index) return;

  const auto depth_info = GetXeGTAOCapturedViewInfo(device, data->captured_depth_srv);
  const auto ssao_info = GetXeGTAOCapturedViewInfo(device, data->captured_ssao_srv);
  const auto mrt_normal_info = GetXeGTAOCapturedViewInfo(device, data->captured_mrt_normal_srv);
  const auto depth_resource_format = depth_info.resource_desc.type == reshade::api::resource_type::texture_2d
      ? depth_info.resource_desc.texture.format
      : reshade::api::format::unknown;
  const auto ssao_resource_format = ssao_info.resource_desc.type == reshade::api::resource_type::texture_2d
      ? ssao_info.resource_desc.texture.format
      : reshade::api::format::unknown;
  const auto mrt_normal_resource_format =
      mrt_normal_info.resource_desc.type == reshade::api::resource_type::texture_2d
      ? mrt_normal_info.resource_desc.texture.format
      : reshade::api::format::unknown;
  auto depth_view_format = depth_info.view_desc.format;
  if (depth_view_format == reshade::api::format::unknown && depth_resource_format != reshade::api::format::unknown) {
    depth_view_format = reshade::api::format_to_default_typed(depth_resource_format);
  }
  auto ssao_view_format = ssao_info.view_desc.format;
  if (ssao_view_format == reshade::api::format::unknown && ssao_resource_format != reshade::api::format::unknown) {
    ssao_view_format = reshade::api::format_to_default_typed(ssao_resource_format);
  }
  auto mrt_normal_view_format = mrt_normal_info.view_desc.format;
  if (mrt_normal_view_format == reshade::api::format::unknown
      && mrt_normal_resource_format != reshade::api::format::unknown) {
    mrt_normal_view_format = reshade::api::format_to_default_typed(mrt_normal_resource_format);
  }

  const bool depth_transition =
      data->last_logged_depth_view_handle != depth_info.view.handle
      || data->last_logged_depth_width != depth_info.width
      || data->last_logged_depth_height != depth_info.height
      || data->last_logged_depth_view_format != depth_view_format
      || data->last_logged_depth_resource_format != depth_resource_format;
  if (depth_transition) {
    std::ostringstream message;
    message << "XeGTAO capture transition t3: "
            << "view 0x" << std::hex << data->last_logged_depth_view_handle
            << " -> 0x" << depth_info.view.handle << std::dec
            << ", size " << data->last_logged_depth_width << "x" << data->last_logged_depth_height
            << " -> " << depth_info.width << "x" << depth_info.height
            << ", view_fmt " << static_cast<uint32_t>(data->last_logged_depth_view_format)
            << " -> " << static_cast<uint32_t>(depth_view_format)
            << ", res_fmt " << static_cast<uint32_t>(data->last_logged_depth_resource_format)
            << " -> " << static_cast<uint32_t>(depth_resource_format);
    AddonLog(reshade::log::level::info, message.str().c_str());
  }

  const bool ssao_transition =
      data->last_logged_ssao_view_handle != ssao_info.view.handle
      || data->last_logged_ssao_width != ssao_info.width
      || data->last_logged_ssao_height != ssao_info.height
      || data->last_logged_ssao_view_format != ssao_view_format
      || data->last_logged_ssao_resource_format != ssao_resource_format;
  if (ssao_transition) {
    std::ostringstream message;
    message << "XeGTAO capture transition t4: "
            << "view 0x" << std::hex << data->last_logged_ssao_view_handle
            << " -> 0x" << ssao_info.view.handle << std::dec
            << ", size " << data->last_logged_ssao_width << "x" << data->last_logged_ssao_height
            << " -> " << ssao_info.width << "x" << ssao_info.height
            << ", view_fmt " << static_cast<uint32_t>(data->last_logged_ssao_view_format)
            << " -> " << static_cast<uint32_t>(ssao_view_format)
            << ", res_fmt " << static_cast<uint32_t>(data->last_logged_ssao_resource_format)
            << " -> " << static_cast<uint32_t>(ssao_resource_format);
    AddonLog(reshade::log::level::info, message.str().c_str());
  }

  const bool mrt_normal_transition =
      data->last_logged_mrt_normal_view_handle != mrt_normal_info.view.handle
      || data->last_logged_mrt_normal_width != mrt_normal_info.width
      || data->last_logged_mrt_normal_height != mrt_normal_info.height
      || data->last_logged_mrt_normal_view_format != mrt_normal_view_format
      || data->last_logged_mrt_normal_resource_format != mrt_normal_resource_format;
  if (mrt_normal_transition) {
    std::ostringstream message;
    message << "XeGTAO capture transition t1: "
            << "view 0x" << std::hex << data->last_logged_mrt_normal_view_handle
            << " -> 0x" << mrt_normal_info.view.handle << std::dec
            << ", size " << data->last_logged_mrt_normal_width << "x" << data->last_logged_mrt_normal_height
            << " -> " << mrt_normal_info.width << "x" << mrt_normal_info.height
            << ", view_fmt " << static_cast<uint32_t>(data->last_logged_mrt_normal_view_format)
            << " -> " << static_cast<uint32_t>(mrt_normal_view_format)
            << ", res_fmt " << static_cast<uint32_t>(data->last_logged_mrt_normal_resource_format)
            << " -> " << static_cast<uint32_t>(mrt_normal_resource_format);
    AddonLog(reshade::log::level::info, message.str().c_str());
  }

  const bool scene_cbv_valid = data->captured_scene_cbv_valid && data->captured_scene_cbv.buffer.handle != 0u;
  const uint64_t scene_cbv_handle = scene_cbv_valid ? data->captured_scene_cbv.buffer.handle : 0u;
  const bool scene_cbv_transition =
      data->last_logged_scene_cbv_source != data->captured_scene_cbv_source
      || data->last_logged_scene_cbv_valid != scene_cbv_valid
      || data->last_logged_scene_cbv_buffer_handle != scene_cbv_handle;
  if (scene_cbv_transition) {
    std::ostringstream message;
    message << "XeGTAO capture transition b0: source "
            << GetXeGTAOSceneCbvSourceName(data->last_logged_scene_cbv_source)
            << " -> " << GetXeGTAOSceneCbvSourceName(data->captured_scene_cbv_source)
            << ", valid " << static_cast<uint32_t>(data->last_logged_scene_cbv_valid ? 1u : 0u)
            << " -> " << static_cast<uint32_t>(scene_cbv_valid ? 1u : 0u)
            << ", buffer 0x" << std::hex << data->last_logged_scene_cbv_buffer_handle
            << " -> 0x" << scene_cbv_handle << std::dec;
    if (scene_cbv_valid) {
      message << ", frame=" << data->captured_scene_cbv_frame;
    }
    AddonLog(reshade::log::level::info, message.str().c_str());
  }

  const bool fallback_scene_cbv_seen = data->fallback_scene_cbv_seen && data->fallback_scene_cbv.buffer.handle != 0u;
  const uint64_t fallback_scene_cbv_handle = fallback_scene_cbv_seen ? data->fallback_scene_cbv.buffer.handle : 0u;
  const uint64_t fallback_scene_cbv_frame = fallback_scene_cbv_seen ? data->fallback_scene_cbv_frame : kInvalidFrameIndex;
  const uint32_t fallback_scene_cbv_stable_count = fallback_scene_cbv_seen ? data->fallback_scene_cbv_stable_count : 0u;
  const bool fallback_scene_cbv_transition =
      data->last_logged_fallback_scene_cbv_seen != fallback_scene_cbv_seen
      || data->last_logged_fallback_scene_cbv_buffer_handle != fallback_scene_cbv_handle
      || data->last_logged_fallback_scene_cbv_stable_count != fallback_scene_cbv_stable_count;
  if (fallback_scene_cbv_transition) {
    std::ostringstream message;
    message << "XeGTAO fallback b0 transition: seen "
            << static_cast<uint32_t>(data->last_logged_fallback_scene_cbv_seen ? 1u : 0u)
            << " -> " << static_cast<uint32_t>(fallback_scene_cbv_seen ? 1u : 0u)
            << ", buffer 0x" << std::hex << data->last_logged_fallback_scene_cbv_buffer_handle
            << " -> 0x" << fallback_scene_cbv_handle << std::dec
            << ", stable_count=" << fallback_scene_cbv_stable_count;
    if (fallback_scene_cbv_seen) {
      message << ", last_seen_frame=" << fallback_scene_cbv_frame;
    }
    AddonLog(reshade::log::level::info, message.str().c_str());
  }

  data->last_capture_diag_log_frame = data->present_frame_index;
  data->last_logged_depth_view_handle = depth_info.view.handle;
  data->last_logged_depth_width = depth_info.width;
  data->last_logged_depth_height = depth_info.height;
  data->last_logged_depth_view_format = depth_view_format;
  data->last_logged_depth_resource_format = depth_resource_format;
  data->last_logged_ssao_view_handle = ssao_info.view.handle;
  data->last_logged_ssao_width = ssao_info.width;
  data->last_logged_ssao_height = ssao_info.height;
  data->last_logged_ssao_view_format = ssao_view_format;
  data->last_logged_ssao_resource_format = ssao_resource_format;
  data->last_logged_mrt_normal_view_handle = mrt_normal_info.view.handle;
  data->last_logged_mrt_normal_width = mrt_normal_info.width;
  data->last_logged_mrt_normal_height = mrt_normal_info.height;
  data->last_logged_mrt_normal_view_format = mrt_normal_view_format;
  data->last_logged_mrt_normal_resource_format = mrt_normal_resource_format;
  data->last_logged_scene_cbv_source = data->captured_scene_cbv_source;
  data->last_logged_scene_cbv_buffer_handle = scene_cbv_handle;
  data->last_logged_scene_cbv_valid = scene_cbv_valid;
  data->last_logged_fallback_scene_cbv_seen = fallback_scene_cbv_seen;
  data->last_logged_fallback_scene_cbv_buffer_handle = fallback_scene_cbv_handle;
  data->last_logged_fallback_scene_cbv_frame = fallback_scene_cbv_frame;
  data->last_logged_fallback_scene_cbv_stable_count = fallback_scene_cbv_stable_count;
}

XeGTAOMode GetXeGTAOModeSetting() {
  const auto mode = static_cast<uint32_t>(std::clamp(xegtao_mode, 0.f, 1.f));
  return static_cast<XeGTAOMode>(mode);
}

uint32_t ClampXeGTAOFixMode() {
  return static_cast<uint32_t>(std::clamp(std::round(xegtao_fix_mode), 0.f, 5.f));
}

const char* GetXeGTAOFixModeName(uint32_t fix_mode) {
  switch (static_cast<XeGTAOFixMode>(fix_mode)) {
    case XeGTAOFixMode::kProducerConsumerSplit:
      return "l1_producer_consumer";
    case XeGTAOFixMode::kDispatchIsolationRestore:
      return "l2_isolation_restore";
    case XeGTAOFixMode::kSingleOwnerDeterministic:
      return "l3_single_owner";
    case XeGTAOFixMode::kStrictSideEffectGuard:
      return "l4_strict_guard";
    case XeGTAOFixMode::kPassIsolationDiagnostics:
      return "l5_pass_isolation";
    case XeGTAOFixMode::kOff:
    default:
      return "off";
  }
}

uint32_t ClampXeGTAOQuality() {
  return static_cast<uint32_t>(std::clamp(xegtao_quality, 0.f, 2.f));
}

uint32_t ClampBooleanToggle(float value) {
  return static_cast<uint32_t>(std::clamp(std::round(value), 0.f, 1.f));
}

bool AreXeGTAOFallbacksEnabled() {
  return ClampBooleanToggle(xegtao_enable_fallbacks) != 0u;
}

uint64_t BuildSceneCbvSignature(const reshade::api::buffer_range& range) {
  uint64_t signature = 1469598103934665603ull;
  signature = HashCombineU64(signature, range.buffer.handle);
  signature = HashCombineU64(signature, range.offset);
  signature = HashCombineU64(signature, range.size);
  return signature;
}

uint64_t GetXeGTAOFallbackActivationFrame(const DeviceData* data) {
  uint64_t activation_frame = kXeGTAOFallbackStartupQuarantineFrames;
  if (data == nullptr) return activation_frame;

  if (data->xegtao_resize_guard_until_frame != 0u) {
    const uint64_t post_resize_activation_frame =
        data->xegtao_resize_guard_until_frame + kXeGTAOFallbackPostResizeCooldownFrames;
    activation_frame = std::max(activation_frame, post_resize_activation_frame);
  }
  return activation_frame;
}

bool IsXeGTAOFallbackActivationBlocked(const DeviceData* data) {
  if (data == nullptr) return true;
  return data->present_frame_index < GetXeGTAOFallbackActivationFrame(data);
}

bool IsXeGTAOFixLevelAtLeast(XeGTAOFixMode level) {
  return ClampXeGTAOFixMode() >= static_cast<uint32_t>(level);
}

uint32_t ClampXeGTAOPrecision() {
  return static_cast<uint32_t>(XeGTAOPrecision::kFullFP32);
}

uint32_t ClampXeGTAONormalInputMode() {
  return static_cast<uint32_t>(std::clamp(xegtao_normal_input_mode, 0.f, 1.f));
}

uint32_t ClampXeGTAONormalDarkeningMode() {
  return static_cast<uint32_t>(std::clamp(std::round(xegtao_normal_darkening_mode), 0.f, 1.f));
}

uint32_t ClampXeGTAODenoiserMode() {
  return static_cast<uint32_t>(std::clamp(std::round(xegtao_denoiser_mode), 0.f, 2.f));
}

uint32_t ClampXeGTAODenoisePasses() {
  return static_cast<uint32_t>(std::clamp(xegtao_denoise_pass_count, 0.f, 3.f));
}

uint32_t ClampXeGTAOIsFastPasses() {
  return static_cast<uint32_t>(std::clamp(std::round(xegtao_isfast_passes), 1.f, 4.f));
}

uint32_t ClampXeGTAOIsFastSamples() {
  return static_cast<uint32_t>(std::clamp(std::round(xegtao_isfast_samples), 2.f, 16.f));
}

uint32_t ClampXeGTAOIsFastJitterMode() {
  return static_cast<uint32_t>(std::clamp(std::round(xegtao_isfast_jitter), 0.f, 1.f));
}

float ClampXeGTAOIsFastJitterAmount() {
  return std::clamp(xegtao_isfast_jitter_amount, 0.f, 1.f);
}

bool TryNormalizeSceneCbvRange(
    reshade::api::device* device,
    const reshade::api::buffer_range& input,
    reshade::api::buffer_range* output) {
  if (device == nullptr || output == nullptr) return false;
  if (input.buffer.handle == 0u) return false;

  const auto desc = device->get_resource_desc(input.buffer);
  if (desc.type != reshade::api::resource_type::buffer) return false;
  if (desc.buffer.size < kSceneCbMinimumBytes) return false;
  if (input.offset >= desc.buffer.size) return false;

  const uint64_t available_bytes = desc.buffer.size - input.offset;
  const bool has_unknown_size = input.size == 0u || input.size == UINT64_MAX;
  if (!has_unknown_size && input.size < kSceneCbMinimumBytes) return false;

  uint64_t normalized_size = has_unknown_size ? available_bytes : input.size;
  if (normalized_size > available_bytes) normalized_size = available_bytes;
  if (normalized_size > kSceneCbMaximumBytes) normalized_size = kSceneCbMaximumBytes;

  // Constant buffer payload is 16-byte aligned; align down to keep updates valid.
  normalized_size &= ~0xFu;
  if (normalized_size < kSceneCbMinimumBytes) return false;

  reshade::api::buffer_range normalized = input;
  normalized.size = normalized_size;
  *output = normalized;
  return true;
}

bool IsSceneCbvCandidateValid(reshade::api::device* device, const reshade::api::buffer_range& range) {
  reshade::api::buffer_range normalized = {};
  return TryNormalizeSceneCbvRange(device, range, &normalized);
}

void CacheFallbackSceneCbv(reshade::api::command_list* cmd_list, const reshade::api::buffer_range& range) {
  if (!AreXeGTAOFallbacksEnabled()) return;
  if (cmd_list == nullptr) return;
  auto* device = cmd_list->get_device();
  if (device == nullptr) return;

  auto* data = device->get_private_data<DeviceData>();
  if (data == nullptr) return;

  if (IsXeGTAOFallbackActivationBlocked(data)) return;

  reshade::api::buffer_range normalized = {};
  if (!TryNormalizeSceneCbvRange(device, range, &normalized)) return;

  const uint64_t signature = BuildSceneCbvSignature(normalized);
  if (data->fallback_scene_cbv_signature != signature) {
    data->fallback_scene_cbv_signature = signature;
    data->fallback_scene_cbv_stable_count = 1u;
  } else if (data->fallback_scene_cbv_frame != data->present_frame_index
      && data->fallback_scene_cbv_stable_count < kXeGTAOFallbackSceneCbvRequiredStableFrames) {
    data->fallback_scene_cbv_stable_count += 1u;
  }

  data->fallback_scene_cbv = normalized;
  data->fallback_scene_cbv_seen = true;
  data->fallback_scene_cbv_frame = data->present_frame_index;
}

bool TryAdoptFallbackSceneCbv(reshade::api::device* device, DeviceData* data) {
  if (device == nullptr || data == nullptr) return false;
  if (!AreXeGTAOFallbacksEnabled()) return false;

  if (IsXeGTAOFallbackActivationBlocked(data)) return false;

  reshade::api::buffer_range normalized_current = {};
  if (data->captured_scene_cbv_valid
      && data->captured_scene_cbv_source == XeGTAOSceneCbvSource::kCurrentLighting
      && TryNormalizeSceneCbvRange(device, data->captured_scene_cbv, &normalized_current)) {
    data->captured_scene_cbv = normalized_current;
    return true;
  }

  data->captured_scene_cbv = {};
  data->captured_scene_cbv_valid = false;
  data->captured_scene_cbv_frame = kInvalidFrameIndex;
  data->captured_scene_cbv_source = XeGTAOSceneCbvSource::kNone;

  if (!data->fallback_scene_cbv_seen) return false;
  if (data->fallback_scene_cbv_frame == kInvalidFrameIndex) return false;
  const bool enforce_fresh_fallback =
      data->present_frame_index < kXeGTAOStartupRequireCurrentSceneCbvFrames;
  if (enforce_fresh_fallback
      && data->present_frame_index >= data->fallback_scene_cbv_frame
      && data->present_frame_index - data->fallback_scene_cbv_frame > kXeGTAOFallbackSceneCbvMaxAgeFrames) {
    return false;
  }

  reshade::api::buffer_range normalized_fallback = {};
  if (!TryNormalizeSceneCbvRange(device, data->fallback_scene_cbv, &normalized_fallback)) return false;
  const uint64_t fallback_signature = BuildSceneCbvSignature(normalized_fallback);
  if (data->fallback_scene_cbv_signature != fallback_signature) {
    data->fallback_scene_cbv_signature = fallback_signature;
    data->fallback_scene_cbv_stable_count = 1u;
  }
  if (data->fallback_scene_cbv_stable_count < kXeGTAOFallbackSceneCbvRequiredStableFrames) {
    return false;
  }

  data->fallback_scene_cbv = normalized_fallback;

  data->captured_scene_cbv = normalized_fallback;
  data->captured_scene_cbv_valid = true;
  data->captured_scene_cbv_frame = data->present_frame_index;
  data->captured_scene_cbv_source = XeGTAOSceneCbvSource::kFallback;
  return true;
}

void DestroyResourceViewIfValid(reshade::api::device* device, reshade::api::resource_view* view) {
  if (device == nullptr || view == nullptr) return;
  if (view->handle == 0u) return;
  device->destroy_resource_view(*view);
  *view = {0u};
}

void DestroyResourceIfValid(reshade::api::device* device, reshade::api::resource* resource) {
  if (device == nullptr || resource == nullptr) return;
  if (resource->handle == 0u) return;
  device->destroy_resource(*resource);
  *resource = {0u};
}

void DestroyDescriptorTableIfValid(
    reshade::api::device* device,
    reshade::api::descriptor_table* table) {
  if (device == nullptr || table == nullptr) return;
  if (table->handle == 0u) return;
  device->free_descriptor_table(*table);
  *table = {0u};
}

void DestroyXeGTAODescriptorTableSet(
    reshade::api::device* device,
    XeGTAODescriptorTableSet* tables) {
  if (device == nullptr || tables == nullptr) return;
  for (auto& table : *tables) {
    DestroyDescriptorTableIfValid(device, &table);
  }
}

void DestroyXeGTAODescriptorTables(reshade::api::device* device, DeviceData* data) {
  if (device == nullptr || data == nullptr) return;
  DestroyXeGTAODescriptorTableSet(device, &data->xegtao_prefilter_descriptor_tables);
  DestroyXeGTAODescriptorTableSet(device, &data->xegtao_main_descriptor_tables);
  DestroyXeGTAODescriptorTableSet(device, &data->xegtao_denoise_descriptor_tables);
  DestroyXeGTAODescriptorTableSet(device, &data->xegtao_composite_descriptor_tables);
  DestroyXeGTAODescriptorTableSet(device, &data->xegtao_normal_cap_descriptor_tables);
}

void DestroyPipelineLayoutIfValid(reshade::api::device* device, reshade::api::pipeline_layout* layout) {
  if (device == nullptr || layout == nullptr) return;
  if (layout->handle == 0u) return;
  device->destroy_pipeline_layout(*layout);
  *layout = {0u};
}

void DestroyPipelineIfValid(reshade::api::device* device, reshade::api::pipeline* pipeline) {
  if (device == nullptr || pipeline == nullptr) return;
  if (pipeline->handle == 0u) return;
  device->destroy_pipeline(*pipeline);
  *pipeline = {0u};
}

void DestroyXeGTAOResources(reshade::api::device* device, DeviceData* data) {
  if (device == nullptr || data == nullptr) return;

  DestroyResourceViewIfValid(device, &data->depth_mips_srv);
  for (auto& mip_uav : data->depth_mips_uavs) {
    DestroyResourceViewIfValid(device, &mip_uav);
  }
  DestroyResourceIfValid(device, &data->depth_mips_texture);

  DestroyResourceViewIfValid(device, &data->ao_term_a_srv);
  DestroyResourceViewIfValid(device, &data->ao_term_a_uav);
  DestroyResourceIfValid(device, &data->ao_term_a_texture);

  DestroyResourceViewIfValid(device, &data->ao_term_b_srv);
  DestroyResourceViewIfValid(device, &data->ao_term_b_uav);
  DestroyResourceIfValid(device, &data->ao_term_b_texture);

  DestroyResourceViewIfValid(device, &data->edges_srv);
  DestroyResourceViewIfValid(device, &data->edges_uav);
  DestroyResourceIfValid(device, &data->edges_texture);

  DestroyResourceViewIfValid(device, &data->composite_srv);
  DestroyResourceViewIfValid(device, &data->composite_uav);
  DestroyResourceIfValid(device, &data->composite_texture);
  data->composite_desc = {};

  data->working_width = 0u;
  data->working_height = 0u;
  data->working_precision = std::numeric_limits<uint32_t>::max();
  data->working_ao_format = reshade::api::format::unknown;
}

void DestroyDedicatedSssViews(reshade::api::device* device, DeviceData* data) {
  if (device == nullptr || data == nullptr) return;

  DestroyResourceViewIfValid(device, &data->character_sss_current_srv);
  DestroyResourceViewIfValid(device, &data->character_sss_last_valid_srv);
  data->character_sss_current_resource = {};
  data->character_sss_last_valid_resource = {};
  data->character_sss_current_frame = kInvalidFrameIndex;
}

void DestroyXeGTAOPipelines(reshade::api::device* device, DeviceData* data) {
  if (device == nullptr || data == nullptr) return;

  DestroyXeGTAODescriptorTables(device, data);

  DestroyPipelineIfValid(device, &data->xegtao_prefilter_pipeline);
  DestroyPipelineIfValid(device, &data->xegtao_main_pipeline);
  DestroyPipelineIfValid(device, &data->xegtao_denoise_pipeline);
  DestroyPipelineIfValid(device, &data->xegtao_denoise_isfast_pipeline);
  DestroyPipelineIfValid(device, &data->xegtao_composite_pipeline);
  DestroyPipelineIfValid(device, &data->xegtao_normal_cap_pipeline);

  DestroyPipelineLayoutIfValid(device, &data->xegtao_prefilter_layout);
  DestroyPipelineLayoutIfValid(device, &data->xegtao_main_layout);
  DestroyPipelineLayoutIfValid(device, &data->xegtao_denoise_layout);
  DestroyPipelineLayoutIfValid(device, &data->xegtao_denoise_isfast_layout);
  DestroyPipelineLayoutIfValid(device, &data->xegtao_composite_layout);
  DestroyPipelineLayoutIfValid(device, &data->xegtao_normal_cap_layout);
}

void DestroyXeGTAOState(reshade::api::device* device, DeviceData* data) {
  if (device == nullptr || data == nullptr) return;
  DestroyXeGTAOResources(device, data);
  DestroyXeGTAOPipelines(device, data);
  DestroyDedicatedSssViews(device, data);
  g_character_mrt0_view.store(0u, std::memory_order_relaxed);
  g_lighting_mrt0_view.store(0u, std::memory_order_relaxed);

  if (data->point_clamp_sampler.handle != 0u) {
    device->destroy_sampler(data->point_clamp_sampler);
    data->point_clamp_sampler = {0u};
  }

  data->captured_depth_srv = {};
  data->captured_ssao_srv = {};
  data->captured_mrt_normal_srv = {};
  data->captured_scene_cbv = {};
  data->captured_scene_cbv_valid = false;
  data->captured_scene_cbv_frame = kInvalidFrameIndex;
  data->captured_scene_cbv_source = XeGTAOSceneCbvSource::kNone;
  data->resolved_scene_cbv_from_current_bindings = false;
  data->xegtao_resize_guard_until_frame = 0u;
  data->xegtao_resize_guard_log_frame = kInvalidFrameIndex;
  data->fallback_scene_cbv = {};
  data->fallback_scene_cbv_seen = false;
  data->fallback_scene_cbv_frame = kInvalidFrameIndex;
  data->fallback_scene_cbv_signature = 0u;
  data->fallback_scene_cbv_stable_count = 0u;
  data->last_gtao_frame = kInvalidFrameIndex;
  data->last_ao_hook_frame = kInvalidFrameIndex;
  data->last_copyback_frame = kInvalidFrameIndex;
  data->last_gtao_failure_log_frame = kInvalidFrameIndex;
  data->last_skip_vanilla_ao_ignored_log_frame = kInvalidFrameIndex;
  data->xegtao_mrt_normal_frame = kInvalidFrameIndex;
  data->xegtao_mrt_normal_valid = false;
  data->copyback_succeeded = false;
  data->xegtao_copyback_frame = kInvalidFrameIndex;
  data->xegtao_copyback_requested_for_frame = false;
  data->xegtao_copyback_succeeded_for_frame = false;
  data->xegtao_copyback_active_for_apply = false;
  data->xegtao_result_signature_valid = false;
  data->xegtao_result_signature_frame = kInvalidFrameIndex;
  data->xegtao_result_t3_resource_handle = 0u;
  data->xegtao_result_t4_resource_handle = 0u;
  data->xegtao_result_t3_view_handle = 0u;
  data->xegtao_result_t4_view_handle = 0u;
  data->xegtao_result_t3_width = 0u;
  data->xegtao_result_t3_height = 0u;
  data->xegtao_result_t4_width = 0u;
  data->xegtao_result_t4_height = 0u;
  data->xegtao_result_working_width = 0u;
  data->xegtao_result_working_height = 0u;
  data->xegtao_consume_signature_valid = false;
  data->xegtao_consume_signature_frame = kInvalidFrameIndex;
  data->xegtao_consume_t3_resource_handle = 0u;
  data->xegtao_consume_t4_resource_handle = 0u;
  data->xegtao_consume_t3_width = 0u;
  data->xegtao_consume_t3_height = 0u;
  data->xegtao_consume_t4_width = 0u;
  data->xegtao_consume_t4_height = 0u;
  data->xegtao_consume_working_width = 0u;
  data->xegtao_consume_working_height = 0u;
  data->xegtao_consume_owner_valid = false;
  data->xegtao_consume_owner_frame = kInvalidFrameIndex;
  data->xegtao_consume_owner_shader_hash = 0u;
  data->xegtao_consume_owner_draw_ordinal = 0u;
  data->xegtao_consume_owner_gate_signature = 0u;
  data->xegtao_consume_owner_downscaled = false;
  data->xegtao_lighting_draw_counter_frame = kInvalidFrameIndex;
  data->xegtao_lighting_draw_counter = 0u;
  data->xegtao_owner_valid = false;
  data->xegtao_owner_frame = kInvalidFrameIndex;
  data->xegtao_owner_shader_hash = 0u;
  data->xegtao_owner_draw_ordinal = 0u;
  data->xegtao_owner_gate_signature = 0u;
  data->xegtao_owner_downscaled = false;
  data->xegtao_deferred_dispatch_pending = false;
  data->xegtao_deferred_dispatch_frame = kInvalidFrameIndex;
  data->xegtao_deferred_dispatch_executed = false;
  data->xegtao_deferred_gate_signature = 0u;
  data->xegtao_deferred_owner_shader_hash = 0u;
  data->xegtao_deferred_owner_draw_ordinal = 0u;
  data->xegtao_deferred_depth_srv = {};
  data->xegtao_deferred_ssao_srv = {};
  data->xegtao_deferred_mrt_normal_srv = {};
  data->xegtao_deferred_scene_cbv = {};
  data->xegtao_deferred_scene_cbv_valid = false;
  data->xegtao_deferred_scene_cbv_frame = kInvalidFrameIndex;
  data->xegtao_deferred_scene_cbv_source = XeGTAOSceneCbvSource::kNone;
  data->xegtao_deferred_resolved_scene_cbv_from_current_bindings = false;
  data->xegtao_deferred_drop_log_frame = kInvalidFrameIndex;
  data->xegtao_volfog_seen_frame = kInvalidFrameIndex;
  data->last_owner_state_valid = false;
  data->last_owner_diag_hash = 0u;
  data->last_gate_state_valid = false;
  data->last_gate_passed = false;
  data->last_gate_diag_hash = 0u;
  data->last_apply_gate_main_state_valid = false;
  data->last_apply_gate_main_passed = false;
  data->last_apply_gate_main_diag_hash = 0u;
  data->last_apply_gate_soft_state_valid = false;
  data->last_apply_gate_soft_passed = false;
  data->last_apply_gate_soft_diag_hash = 0u;
  data->last_apply_path_main_state_valid = false;
  data->last_apply_path_main_hash = 0u;
  data->last_apply_path_soft_state_valid = false;
  data->last_apply_path_soft_hash = 0u;
  data->xegtao_warmup_signature = 0u;
  data->xegtao_warmup_stable_count = 0u;
  data->last_warmup_enter_signature = 0u;
  data->last_warmup_complete_signature = 0u;
  data->xegtao_dispatch_isolation_active = false;
  data->xegtao_dispatch_restore_mismatch = false;
  data->xegtao_trace_frame = kInvalidFrameIndex;
  data->xegtao_trace_dispatch_attempted = false;
  data->xegtao_trace_dispatch_succeeded = false;
  data->xegtao_trace_main_pass_executed = false;
  data->xegtao_trace_composite_pass_executed = false;
  data->xegtao_trace_t22_bind_executed = false;
  data->xegtao_trace_copyback_requested = false;
  data->xegtao_trace_copyback_succeeded = false;
  data->xegtao_trace_apply_gate_passed = false;
  data->xegtao_trace_probe_a_active = false;
  data->xegtao_trace_probe_b_active = false;
  data->xegtao_trace_owner_draw_ordinal = 0u;
  data->xegtao_trace_state_valid = false;
  data->xegtao_trace_diag_hash = 0u;
  data->last_capture_diag_log_frame = kInvalidFrameIndex;
  data->last_logged_depth_view_handle = 0u;
  data->last_logged_ssao_view_handle = 0u;
  data->last_logged_mrt_normal_view_handle = 0u;
  data->last_logged_depth_width = 0u;
  data->last_logged_depth_height = 0u;
  data->last_logged_ssao_width = 0u;
  data->last_logged_ssao_height = 0u;
  data->last_logged_mrt_normal_width = 0u;
  data->last_logged_mrt_normal_height = 0u;
  data->last_logged_depth_view_format = reshade::api::format::unknown;
  data->last_logged_depth_resource_format = reshade::api::format::unknown;
  data->last_logged_ssao_view_format = reshade::api::format::unknown;
  data->last_logged_ssao_resource_format = reshade::api::format::unknown;
  data->last_logged_mrt_normal_view_format = reshade::api::format::unknown;
  data->last_logged_mrt_normal_resource_format = reshade::api::format::unknown;
  data->last_logged_scene_cbv_source = XeGTAOSceneCbvSource::kNone;
  data->last_logged_scene_cbv_buffer_handle = 0u;
  data->last_logged_scene_cbv_valid = false;
  data->last_logged_fallback_scene_cbv_seen = false;
  data->last_logged_fallback_scene_cbv_buffer_handle = 0u;
  data->last_logged_fallback_scene_cbv_frame = kInvalidFrameIndex;
  data->last_logged_fallback_scene_cbv_stable_count = 0u;
}

bool CreateCharacterSssSrv(
    reshade::api::device* device,
    reshade::api::resource source_resource,
    reshade::api::resource_view source_rtv,
    reshade::api::resource_view* out_srv) {
  if (device == nullptr || out_srv == nullptr) return false;
  *out_srv = {0u};
  if (source_resource.handle == 0u || source_rtv.handle == 0u) return false;

  const auto resource_desc = device->get_resource_desc(source_resource);
  if (resource_desc.type != reshade::api::resource_type::texture_2d) return false;

  auto view_format = device->get_resource_view_desc(source_rtv).format;
  if (view_format == reshade::api::format::unknown) {
    view_format = reshade::api::format_to_default_typed(resource_desc.texture.format);
  }
  if (view_format == reshade::api::format::unknown) return false;

  if (device->create_resource_view(
          source_resource,
          reshade::api::resource_usage::shader_resource,
          reshade::api::resource_view_desc(view_format),
          out_srv)) {
    return true;
  }

  const auto fallback_format = reshade::api::format_to_default_typed(resource_desc.texture.format);
  if (fallback_format == reshade::api::format::unknown) return false;
  return device->create_resource_view(
      source_resource,
      reshade::api::resource_usage::shader_resource,
      reshade::api::resource_view_desc(fallback_format),
      out_srv);
}

bool UpdateCharacterSssCapture(
    reshade::api::device* device,
    DeviceData* data,
    reshade::api::resource_view source_rtv) {
  if (device == nullptr || data == nullptr) return false;
  if (!IsViewAlive(device, source_rtv)) return false;

  const auto source_resource = device->get_resource_from_view(source_rtv);
  if (source_resource.handle == 0u) return false;

  if (data->character_sss_current_resource.handle == source_resource.handle
      && IsViewAlive(device, data->character_sss_current_srv)) {
    data->character_sss_current_frame = data->present_frame_index;
    return true;
  }

  reshade::api::resource_view new_srv = {};
  if (!CreateCharacterSssSrv(device, source_resource, source_rtv, &new_srv)) return false;

  if (IsViewAlive(device, data->character_sss_current_srv)) {
    DestroyResourceViewIfValid(device, &data->character_sss_last_valid_srv);
    data->character_sss_last_valid_srv = data->character_sss_current_srv;
    data->character_sss_last_valid_resource = data->character_sss_current_resource;
  }

  data->character_sss_current_srv = new_srv;
  data->character_sss_current_resource = source_resource;
  data->character_sss_current_frame = data->present_frame_index;
  return true;
}

bool EnsurePointClampSampler(reshade::api::device* device, DeviceData* data) {
  if (device == nullptr || data == nullptr) return false;
  if (data->point_clamp_sampler.handle != 0u) return true;

  reshade::api::sampler_desc sampler_desc = {};
  sampler_desc.filter = reshade::api::filter_mode::min_mag_mip_point;
  sampler_desc.address_u = reshade::api::texture_address_mode::clamp;
  sampler_desc.address_v = reshade::api::texture_address_mode::clamp;
  sampler_desc.address_w = reshade::api::texture_address_mode::clamp;
  return device->create_sampler(sampler_desc, &data->point_clamp_sampler);
}

bool CreateXeGTAOTexture2D(
    reshade::api::device* device,
    uint32_t width,
    uint32_t height,
    uint16_t levels,
    reshade::api::format create_format,
    reshade::api::resource_usage usage,
    reshade::api::resource* out_resource) {
  if (device == nullptr || out_resource == nullptr) return false;
  *out_resource = {0u};

  reshade::api::resource_desc desc = {};
  desc.type = reshade::api::resource_type::texture_2d;
  desc.texture = {
      width,
      height,
      1u,
      levels,
      create_format,
      1u,
  };
  desc.heap = reshade::api::memory_heap::gpu_only;
  desc.usage = usage;
  desc.flags = reshade::api::resource_flags::none;
  return device->create_resource(desc, nullptr, usage, out_resource);
}

bool CreateXeGTAOTextureView(
    reshade::api::device* device,
    reshade::api::resource resource,
    reshade::api::resource_usage usage,
    reshade::api::resource_view_type type,
    reshade::api::format format,
    uint32_t first_level,
    uint32_t level_count,
    reshade::api::resource_view* out_view) {
  if (device == nullptr || out_view == nullptr) return false;
  *out_view = {0u};
  reshade::api::resource_view_desc view_desc(type, format, first_level, level_count, 0, 1);
  return device->create_resource_view(resource, usage, view_desc, out_view);
}

bool EnsureXeGTAOLayout(
    reshade::api::device* device,
    uint32_t srv_count,
    uint32_t uav_count,
    reshade::api::pipeline_layout* out_layout) {
  if (device == nullptr || out_layout == nullptr) return false;
  if (out_layout->handle != 0u) return true;

  reshade::api::descriptor_range sampler_range = {};
  sampler_range.binding = 0;
  sampler_range.dx_register_index = 0;
  sampler_range.dx_register_space = 0;
  sampler_range.count = 1;
  sampler_range.visibility = reshade::api::shader_stage::all_compute;
  sampler_range.type = reshade::api::descriptor_type::sampler;

  reshade::api::descriptor_range cbv_range = {};
  cbv_range.binding = 0;
  cbv_range.dx_register_index = 0;
  cbv_range.dx_register_space = 0;
  cbv_range.count = 1;
  cbv_range.visibility = reshade::api::shader_stage::all_compute;
  cbv_range.type = reshade::api::descriptor_type::constant_buffer;

  reshade::api::descriptor_range srv_range = {};
  srv_range.binding = 0;
  srv_range.dx_register_index = 0;
  srv_range.dx_register_space = 0;
  srv_range.count = srv_count;
  srv_range.visibility = reshade::api::shader_stage::all_compute;
  srv_range.type = reshade::api::descriptor_type::texture_shader_resource_view;

  reshade::api::descriptor_range uav_range = {};
  uav_range.binding = 0;
  uav_range.dx_register_index = 0;
  uav_range.dx_register_space = 0;
  uav_range.count = uav_count;
  uav_range.visibility = reshade::api::shader_stage::all_compute;
  uav_range.type = reshade::api::descriptor_type::texture_unordered_access_view;

  reshade::api::constant_range push_constants_range = {};
  push_constants_range.binding = 0;
  push_constants_range.dx_register_index = 13;
  push_constants_range.dx_register_space = 0;
  // Max push constant payload used by compute passes (xesss uses 32 floats).
  push_constants_range.count = 32;
  push_constants_range.visibility = reshade::api::shader_stage::all_compute;

  reshade::api::pipeline_layout_param param_sampler = {};
  param_sampler.type = reshade::api::pipeline_layout_param_type::descriptor_table;
  param_sampler.descriptor_table.count = 1;
  param_sampler.descriptor_table.ranges = &sampler_range;

  reshade::api::pipeline_layout_param param_cbv = {};
  param_cbv.type = reshade::api::pipeline_layout_param_type::descriptor_table;
  param_cbv.descriptor_table.count = 1;
  param_cbv.descriptor_table.ranges = &cbv_range;

  reshade::api::pipeline_layout_param param_srv = {};
  param_srv.type = reshade::api::pipeline_layout_param_type::descriptor_table;
  param_srv.descriptor_table.count = 1;
  param_srv.descriptor_table.ranges = &srv_range;

  reshade::api::pipeline_layout_param param_uav = {};
  param_uav.type = reshade::api::pipeline_layout_param_type::descriptor_table;
  param_uav.descriptor_table.count = 1;
  param_uav.descriptor_table.ranges = &uav_range;

  reshade::api::pipeline_layout_param param_constants = {};
  param_constants.type = reshade::api::pipeline_layout_param_type::push_constants;
  param_constants.push_constants = push_constants_range;

  std::array<reshade::api::pipeline_layout_param, 5> params = {
      param_sampler,
      param_cbv,
      param_srv,
      param_uav,
      param_constants,
  };
  return device->create_pipeline_layout(static_cast<uint32_t>(params.size()), params.data(), out_layout);
}

bool EnsureXeGTAODescriptorTables(
    reshade::api::device* device,
    reshade::api::pipeline_layout layout,
    XeGTAODescriptorTableSet* out_tables) {
  if (device == nullptr || out_tables == nullptr) return false;
  if (layout.handle == 0u) return false;

  for (uint32_t i = 0u; i < kXeGtaoDescriptorTableParamCount; ++i) {
    auto& table = (*out_tables)[i];
    if (table.handle != 0u) continue;
    if (!device->allocate_descriptor_table(layout, i, &table)) {
      return false;
    }
  }

  return true;
}

bool EnsureXeGTAOComputePipeline(
    reshade::api::device* device,
    reshade::api::pipeline_layout layout,
    std::span<const uint8_t> shader_code,
    reshade::api::pipeline* out_pipeline) {
  if (device == nullptr || out_pipeline == nullptr) return false;
  if (out_pipeline->handle != 0u) return true;
  if (layout.handle == 0u || shader_code.empty()) return false;

  reshade::api::shader_desc shader_desc = {};
  shader_desc.code = shader_code.data();
  shader_desc.code_size = shader_code.size();

  reshade::api::pipeline_subobject pipeline_subobject = {};
  pipeline_subobject.type = reshade::api::pipeline_subobject_type::compute_shader;
  pipeline_subobject.count = 1;
  pipeline_subobject.data = &shader_desc;

  return device->create_pipeline(layout, 1, &pipeline_subobject, out_pipeline);
}

bool DispatchXeGTAOCompute(
    reshade::api::command_list* cmd_list,
    reshade::api::pipeline_layout layout,
    reshade::api::pipeline pipeline,
    XeGTAODescriptorTableSet* descriptor_tables,
    std::span<const reshade::api::descriptor_table_update> descriptor_updates,
    std::span<const float> push_constants,
    uint32_t group_count_x,
    uint32_t group_count_y,
    uint32_t group_count_z) {
  if (cmd_list == nullptr) return false;
  if (layout.handle == 0u || pipeline.handle == 0u) return false;
  if (descriptor_tables == nullptr) return false;
  if (descriptor_updates.empty() || descriptor_updates.size() > kXeGtaoDescriptorTableParamCount) return false;
  if (group_count_x == 0u || group_count_y == 0u || group_count_z == 0u) return false;

  auto* device = cmd_list->get_device();
  if (device == nullptr) return false;
  if (!EnsureXeGTAODescriptorTables(device, layout, descriptor_tables)) return false;
  auto* data = device->get_private_data<DeviceData>();

  renodx::utils::state::CommandListState previous_state = {};
  auto* current_state = renodx::utils::state::GetCurrentState(cmd_list);
  if (current_state == nullptr) {
    if (data != nullptr) {
      data->xegtao_debug_predispatch_reject_count += 1u;
      if (data->xegtao_debug_last_predispatch_reject_frame != data->present_frame_index) {
        data->xegtao_debug_last_predispatch_reject_frame = data->present_frame_index;
        AddonLog(
            reshade::log::level::warning,
            std::format(
                "XeGTAO pre-dispatch reject: command-list state unavailable (frame={})",
                data->present_frame_index));
      }
    }
    return false;
  }
  previous_state = *current_state;

  std::array<reshade::api::descriptor_table_update, kXeGtaoDescriptorTableParamCount> table_updates = {};
  std::array<reshade::api::descriptor_table, kXeGtaoDescriptorTableParamCount> bound_tables = {};
  const uint32_t update_count = static_cast<uint32_t>(descriptor_updates.size());
  for (uint32_t i = 0u; i < update_count; ++i) {
    bound_tables[i] = (*descriptor_tables)[i];
    table_updates[i] = descriptor_updates[i];
    table_updates[i].table = bound_tables[i];
    if (table_updates[i].table.handle == 0u) {
      if (data != nullptr) {
        data->xegtao_debug_descriptor_reject_count += 1u;
        if (data->xegtao_debug_last_descriptor_reject_frame != data->present_frame_index) {
          data->xegtao_debug_last_descriptor_reject_frame = data->present_frame_index;
          AddonLog(
              reshade::log::level::warning,
              std::format(
                  "XeGTAO descriptor reject: null table handle (update_index={}, frame={})",
                  i,
                  data->present_frame_index));
        }
      }
      return false;
    }
    if (table_updates[i].count != 0u && table_updates[i].descriptors == nullptr) {
      if (data != nullptr) {
        data->xegtao_debug_descriptor_reject_count += 1u;
        if (data->xegtao_debug_last_descriptor_reject_frame != data->present_frame_index) {
          data->xegtao_debug_last_descriptor_reject_frame = data->present_frame_index;
          AddonLog(
              reshade::log::level::warning,
              std::format(
                  "XeGTAO descriptor reject: null descriptor payload (update_index={}, frame={})",
                  i,
                  data->present_frame_index));
        }
      }
      return false;
    }
  }

  device->update_descriptor_tables(update_count, table_updates.data());

  cmd_list->bind_pipeline(reshade::api::pipeline_stage::all_compute, pipeline);
  cmd_list->bind_descriptor_tables(
      reshade::api::shader_stage::all_compute,
      layout,
      0u,
      update_count,
      bound_tables.data());

  cmd_list->push_constants(
      reshade::api::shader_stage::all_compute,
      layout,
      kXeGtaoPushConstantsLayoutParam,
      0u,
      static_cast<uint32_t>(push_constants.size()),
      push_constants.data());

  cmd_list->dispatch(group_count_x, group_count_y, group_count_z);

  constexpr bool clear_sampler = true;
  constexpr bool clear_rebind_tables = true;
  const bool startup_clear_guard_active =
      data == nullptr || data->present_frame_index < kXeGTAOClearStartupGuardFrames;

  std::array<reshade::api::sampler, 16> null_samplers = {};

  std::array<reshade::api::descriptor_table_update, kXeGtaoDescriptorTableParamCount> clear_updates = {};
  bool has_clear_descriptor_updates = false;
  for (uint32_t i = 0u; i < update_count; ++i) {
    clear_updates[i] = table_updates[i];
    switch (clear_updates[i].type) {
      case reshade::api::descriptor_type::sampler:
        if (clear_sampler) {
          clear_updates[i].descriptors = null_samplers.data();
          has_clear_descriptor_updates = true;
        }
        break;
      default:
        break;
    }
  }

  if (!startup_clear_guard_active && has_clear_descriptor_updates) {
    device->update_descriptor_tables(update_count, clear_updates.data());
  }

  if (!startup_clear_guard_active && clear_rebind_tables) {
    cmd_list->bind_descriptor_tables(
        reshade::api::shader_stage::all_compute,
        layout,
        0u,
        update_count,
        bound_tables.data());
  }

  previous_state.Apply(cmd_list);
  return true;
}

void TransitionResource(
    reshade::api::command_list* cmd_list,
    reshade::api::resource resource,
    reshade::api::resource_usage before,
    reshade::api::resource_usage after) {
  if (cmd_list == nullptr) return;
  if (resource.handle == 0u) return;
  const reshade::api::resource resources[1] = {resource};
  const reshade::api::resource_usage before_states[1] = {before};
  const reshade::api::resource_usage after_states[1] = {after};
  cmd_list->barrier(1, resources, before_states, after_states);
}

bool EnsureXeGTAOPipelines(reshade::api::device* device, DeviceData* data) {
  if (device == nullptr || data == nullptr) return false;
  if (!EnsureXeGTAOLayout(device, 1u, 5u, &data->xegtao_prefilter_layout)) return false;
  if (!EnsureXeGTAOLayout(device, 1u, 2u, &data->xegtao_main_layout)) return false;
  if (!EnsureXeGTAOLayout(device, 2u, 1u, &data->xegtao_denoise_layout)) return false;
  if (!EnsureXeGTAOLayout(device, 2u, 1u, &data->xegtao_composite_layout)) return false;
  if (!EnsureXeGTAOLayout(device, 2u, 1u, &data->xegtao_normal_cap_layout)) return false;
  if (!EnsureXeGTAOComputePipeline(
          device,
          data->xegtao_prefilter_layout,
          __xegtao_prefilter,
          &data->xegtao_prefilter_pipeline)) {
    return false;
  }
  if (!EnsureXeGTAOComputePipeline(
          device,
          data->xegtao_main_layout,
          __xegtao_main,
          &data->xegtao_main_pipeline)) {
    return false;
  }
  if (!EnsureXeGTAOComputePipeline(
          device,
          data->xegtao_denoise_layout,
          __xegtao_denoise,
          &data->xegtao_denoise_pipeline)) {
    return false;
  }
  if (!EnsureXeGTAOComputePipeline(
          device,
          data->xegtao_composite_layout,
          __xegtao_composite_x_only,
          &data->xegtao_composite_pipeline)) {
    return false;
  }
  if (!EnsureXeGTAOComputePipeline(
          device,
          data->xegtao_normal_cap_layout,
          __xegtao_normal_cap,
          &data->xegtao_normal_cap_pipeline)) {
    return false;
  }
  return true;
}

bool EnsureXeGTAOResources(reshade::api::device* device, DeviceData* data) {
  if (device == nullptr || data == nullptr) return false;
  if (!IsViewAlive(device, data->captured_ssao_srv) || !IsViewAlive(device, data->captured_depth_srv)) return false;

  const auto ssao_resource = device->get_resource_from_view(data->captured_ssao_srv);
  if (ssao_resource.handle == 0u) return false;

  const auto ssao_desc = device->get_resource_desc(ssao_resource);
  if (ssao_desc.type != reshade::api::resource_type::texture_2d) return false;
  if (ssao_desc.texture.samples != 1u) return false;

  const auto depth_resource = device->get_resource_from_view(data->captured_depth_srv);
  if (depth_resource.handle == 0u) return false;
  const auto depth_desc = device->get_resource_desc(depth_resource);
  if (depth_desc.type != reshade::api::resource_type::texture_2d) return false;
  if (depth_desc.texture.samples != 1u) return false;

  auto ssao_view_format = device->get_resource_view_desc(data->captured_ssao_srv).format;
  if (ssao_view_format == reshade::api::format::unknown) {
    ssao_view_format = reshade::api::format_to_default_typed(ssao_desc.texture.format);
  } else {
    const auto typed = reshade::api::format_to_default_typed(ssao_view_format);
    if (typed != reshade::api::format::unknown) {
      ssao_view_format = typed;
    }
  }
  if (ssao_view_format == reshade::api::format::unknown) return false;

  const uint32_t precision_mode = ClampXeGTAOPrecision();
  const reshade::api::format depth_mips_format =
      precision_mode == static_cast<uint32_t>(XeGTAOPrecision::kDepthR16)
      ? reshade::api::format::r16_float
      : reshade::api::format::r32_float;

  const uint32_t target_working_width = std::max(1u, depth_desc.texture.width);
  const uint32_t target_working_height = std::max(1u, depth_desc.texture.height);

  const bool should_recreate =
      data->depth_mips_texture.handle == 0u
      || data->ao_term_a_texture.handle == 0u
      || data->ao_term_b_texture.handle == 0u
      || data->edges_texture.handle == 0u
      || data->composite_texture.handle == 0u
      || data->working_width != target_working_width
      || data->working_height != target_working_height
      || data->working_precision != precision_mode
      || data->working_ao_format != ssao_view_format;
  if (!should_recreate) return true;

  DestroyXeGTAOResources(device, data);

  data->working_width = target_working_width;
  data->working_height = target_working_height;
  data->working_precision = precision_mode;
  data->working_ao_format = ssao_view_format;

  const auto gpu_rw_usage =
      reshade::api::resource_usage::shader_resource
      | reshade::api::resource_usage::unordered_access
      | reshade::api::resource_usage::copy_source
      | reshade::api::resource_usage::copy_dest;

  if (!CreateXeGTAOTexture2D(
          device,
          data->working_width,
          data->working_height,
          static_cast<uint16_t>(kXeGtaoDepthMipLevels),
          depth_mips_format,
          gpu_rw_usage,
          &data->depth_mips_texture)) {
    return false;
  }
  if (!CreateXeGTAOTextureView(
          device,
          data->depth_mips_texture,
          reshade::api::resource_usage::shader_resource,
          reshade::api::resource_view_type::texture_2d,
          depth_mips_format,
          0u,
          kXeGtaoDepthMipLevels,
          &data->depth_mips_srv)) {
    return false;
  }
  for (uint32_t mip_level = 0; mip_level < kXeGtaoDepthMipLevels; ++mip_level) {
    if (!CreateXeGTAOTextureView(
            device,
            data->depth_mips_texture,
            reshade::api::resource_usage::unordered_access,
            reshade::api::resource_view_type::texture_2d,
            depth_mips_format,
            mip_level,
            1u,
            &data->depth_mips_uavs[mip_level])) {
      return false;
    }
  }

  if (!CreateXeGTAOTexture2D(
          device,
          data->working_width,
          data->working_height,
          1u,
          reshade::api::format::r32_uint,
          gpu_rw_usage,
          &data->ao_term_a_texture)) {
    return false;
  }
  if (!CreateXeGTAOTextureView(
          device,
          data->ao_term_a_texture,
          reshade::api::resource_usage::shader_resource,
          reshade::api::resource_view_type::texture_2d,
          reshade::api::format::r32_uint,
          0u,
          1u,
          &data->ao_term_a_srv)) {
    return false;
  }
  if (!CreateXeGTAOTextureView(
          device,
          data->ao_term_a_texture,
          reshade::api::resource_usage::unordered_access,
          reshade::api::resource_view_type::texture_2d,
          reshade::api::format::r32_uint,
          0u,
          1u,
          &data->ao_term_a_uav)) {
    return false;
  }

  if (!CreateXeGTAOTexture2D(
          device,
          data->working_width,
          data->working_height,
          1u,
          reshade::api::format::r32_uint,
          gpu_rw_usage,
          &data->ao_term_b_texture)) {
    return false;
  }
  if (!CreateXeGTAOTextureView(
          device,
          data->ao_term_b_texture,
          reshade::api::resource_usage::shader_resource,
          reshade::api::resource_view_type::texture_2d,
          reshade::api::format::r32_uint,
          0u,
          1u,
          &data->ao_term_b_srv)) {
    return false;
  }
  if (!CreateXeGTAOTextureView(
          device,
          data->ao_term_b_texture,
          reshade::api::resource_usage::unordered_access,
          reshade::api::resource_view_type::texture_2d,
          reshade::api::format::r32_uint,
          0u,
          1u,
          &data->ao_term_b_uav)) {
    return false;
  }

  const reshade::api::format preferred_edges_format =
      precision_mode != static_cast<uint32_t>(XeGTAOPrecision::kDepthR16)
      ? reshade::api::format::r16_unorm
      : reshade::api::format::r8_unorm;
  auto create_edges_resources = [&](reshade::api::format edges_format) -> bool {
    return CreateXeGTAOTexture2D(
               device,
               data->working_width,
               data->working_height,
               1u,
               edges_format,
               gpu_rw_usage,
               &data->edges_texture)
        && CreateXeGTAOTextureView(
            device,
            data->edges_texture,
            reshade::api::resource_usage::shader_resource,
            reshade::api::resource_view_type::texture_2d,
            edges_format,
            0u,
            1u,
            &data->edges_srv)
        && CreateXeGTAOTextureView(
            device,
            data->edges_texture,
            reshade::api::resource_usage::unordered_access,
            reshade::api::resource_view_type::texture_2d,
            edges_format,
            0u,
            1u,
            &data->edges_uav);
  };
  if (!create_edges_resources(preferred_edges_format)) {
    DestroyResourceViewIfValid(device, &data->edges_srv);
    DestroyResourceViewIfValid(device, &data->edges_uav);
    DestroyResourceIfValid(device, &data->edges_texture);
    if (!create_edges_resources(reshade::api::format::r16_float)) {
      return false;
    }
  }

  reshade::api::format composite_create_format = reshade::api::format_to_typeless(ssao_view_format);
  if (composite_create_format == reshade::api::format::unknown) {
    composite_create_format = ssao_view_format;
  }

  if (!CreateXeGTAOTexture2D(
          device,
          data->working_width,
          data->working_height,
          1u,
          composite_create_format,
          gpu_rw_usage,
          &data->composite_texture)
      || !CreateXeGTAOTextureView(
          device,
          data->composite_texture,
          reshade::api::resource_usage::shader_resource,
          reshade::api::resource_view_type::texture_2d,
          ssao_view_format,
          0u,
          1u,
          &data->composite_srv)
      || !CreateXeGTAOTextureView(
          device,
          data->composite_texture,
          reshade::api::resource_usage::unordered_access,
          reshade::api::resource_view_type::texture_2d,
          ssao_view_format,
          0u,
          1u,
          &data->composite_uav)) {
    DestroyResourceViewIfValid(device, &data->composite_srv);
    DestroyResourceViewIfValid(device, &data->composite_uav);
    DestroyResourceIfValid(device, &data->composite_texture);
    if (!CreateXeGTAOTexture2D(
            device,
            data->working_width,
            data->working_height,
            1u,
            reshade::api::format::r16g16b16a16_float,
            gpu_rw_usage,
            &data->composite_texture)
        || !CreateXeGTAOTextureView(
            device,
            data->composite_texture,
            reshade::api::resource_usage::shader_resource,
            reshade::api::resource_view_type::texture_2d,
            reshade::api::format::r16g16b16a16_float,
            0u,
            1u,
            &data->composite_srv)
        || !CreateXeGTAOTextureView(
            device,
            data->composite_texture,
            reshade::api::resource_usage::unordered_access,
            reshade::api::resource_view_type::texture_2d,
            reshade::api::format::r16g16b16a16_float,
            0u,
            1u,
            &data->composite_uav)) {
      return false;
    }
  }
  data->composite_desc = device->get_resource_desc(data->composite_texture);
  return true;
}

std::array<float, 32> BuildXeGTAOPushConstants(
    const DeviceData* data,
    bool denoise_last_pass,
    bool force_downscaled_quality) {
  std::array<float, 32> constants = {};
  const uint32_t denoise_passes = force_downscaled_quality ? 3u : ClampXeGTAODenoisePasses();
  const uint32_t quality = force_downscaled_quality ? 2u : ClampXeGTAOQuality();
  constants[0] = static_cast<float>(quality);
  constants[1] = static_cast<float>(denoise_passes);
  constants[2] = std::max(0.001f, xegtao_radius);
  constants[3] = std::clamp(xegtao_falloff_range, 0.f, 1.f);
  constants[4] = std::clamp(xegtao_radius_multiplier, 0.3f, 3.f);
  constants[5] = std::clamp(xegtao_final_value_power, 0.5f, 5.f);
  constants[6] = std::clamp(xegtao_sample_distribution_power, 1.f, 3.f);
  constants[7] = std::clamp(xegtao_thin_occluder_compensation, 0.f, 0.7f);
  constants[8] = std::clamp(xegtao_depth_mip_sampling_offset, 0.f, 30.f);
  constants[9] = denoise_passes == 0u ? 10000.f : std::max(0.01f, xegtao_denoise_blur_beta);
  constants[10] = denoise_passes == 0u
      ? 0.f
      : static_cast<float>((data != nullptr ? data->present_frame_index : 0u) % 64u);
  constants[11] = std::clamp(xegtao_debug_mode, 0.f, 21.f);
  constants[12] = denoise_last_pass ? 1.f : 0.f;
  const uint32_t normal_input_mode = ClampXeGTAONormalInputMode();
  const bool transformed_normal_mode = normal_input_mode == 1u;
  const float effective_normal_influence = transformed_normal_mode
      ? std::max(xegtao_normal_influence, 0.20f)
      : xegtao_normal_influence;
  const float effective_normal_depth_blend = transformed_normal_mode
      ? std::max(xegtao_normal_depth_blend, 0.70f)
      : xegtao_normal_depth_blend;
  constants[13] = static_cast<float>(normal_input_mode);
  constants[14] = 0.f;
  constants[15] = std::clamp(effective_normal_influence, 0.f, 2.f);
  constants[16] = std::clamp(effective_normal_depth_blend, 0.f, 1.f);
  constants[17] = std::clamp(xegtao_normal_sharpness, 0.5f, 2.5f);
  constants[18] = std::clamp(xegtao_normal_edge_rejection, 0.f, 4.f);
  constants[19] = std::clamp(xegtao_normal_z_preservation, 0.f, 2.f);
  constants[20] = std::clamp(xegtao_normal_detail_response, 0.25f, 4.f);
  constants[21] = std::clamp(xegtao_normal_max_darkening, 0.f, 1.f);
  constants[22] = static_cast<float>(ClampXeGTAONormalDarkeningMode());
  // XeGTAO denoiser is forced to Vanilla-only at runtime.
  constants[23] = 0.f;
  // Copyback-safe composite mode flag:
  // 0 = normal composite contract (x = AO, yz = payload),
  // 1 = preserve original AO.yzw for copyback consumption.
  constants[24] = (data != nullptr && data->xegtao_copyback_requested_for_frame) ? 1.f : 0.f;
  constants[25] = static_cast<float>(ClampXeGTAOIsFastPasses());
  constants[26] = static_cast<float>(ClampXeGTAOIsFastSamples());
  constants[27] = std::clamp(xegtao_isfast_radius, 0.25f, 8.f);
  constants[28] = std::clamp(xegtao_isfast_edge_sensitivity, 0.f, 8.f);
  constants[29] = std::clamp(xegtao_isfast_spatial_sigma, 0.1f, 8.f);
  constants[30] = std::clamp(xegtao_isfast_hybrid_blend, 0.f, 1.f);
  constants[31] = g_isfast_reshade_srv.handle != 0u ? 1.f : 0.f;
  return constants;
}

bool TryCopyBackXeGTAOResult(reshade::api::command_list* cmd_list, DeviceData* data) {
  if (cmd_list == nullptr || data == nullptr) return false;
  auto* device = cmd_list->get_device();
  if (device == nullptr) return false;
  if (!IsViewAlive(device, data->captured_ssao_srv)) return false;
  if (data->composite_texture.handle == 0u) return false;

  const auto src_resource = data->composite_texture;
  const auto dst_resource = device->get_resource_from_view(data->captured_ssao_srv);
  if (dst_resource.handle == 0u) return false;

  const auto src_desc = device->get_resource_desc(src_resource);
  const auto dst_desc = device->get_resource_desc(dst_resource);
  if (src_desc.type != reshade::api::resource_type::texture_2d
      || dst_desc.type != reshade::api::resource_type::texture_2d) {
    return false;
  }
  if (src_desc.texture.width != dst_desc.texture.width
      || src_desc.texture.height != dst_desc.texture.height
      || src_desc.texture.samples != dst_desc.texture.samples) {
    return false;
  }

  const auto src_typeless = reshade::api::format_to_typeless(src_desc.texture.format);
  const auto dst_typeless = reshade::api::format_to_typeless(dst_desc.texture.format);
  if (src_typeless != dst_typeless) return false;

  const reshade::api::resource resources[2] = {src_resource, dst_resource};
  const reshade::api::resource_usage before[2] = {
      reshade::api::resource_usage::unordered_access,
      reshade::api::resource_usage::shader_resource,
  };
  const reshade::api::resource_usage copy_states[2] = {
      reshade::api::resource_usage::copy_source,
      reshade::api::resource_usage::copy_dest,
  };
  const reshade::api::resource_usage after[2] = {
      reshade::api::resource_usage::shader_resource,
      reshade::api::resource_usage::shader_resource,
  };

  cmd_list->barrier(2, resources, before, copy_states);
  cmd_list->copy_texture_region(src_resource, 0, nullptr, dst_resource, 0, nullptr);
  cmd_list->barrier(2, resources, copy_states, after);
  return true;
}

bool RunXeGTAOForFrame(
  reshade::api::command_list* cmd_list,
  DeviceData* data,
  bool request_copy_back,
  bool resolve_inputs_from_current_bindings) {
  if (cmd_list == nullptr || data == nullptr) return false;
  auto* device = cmd_list->get_device();
  auto fail = [data, device](const std::string& reason) -> bool {
    if (data != nullptr && data->last_gtao_failure_log_frame != data->present_frame_index) {
      data->last_gtao_failure_log_frame = data->present_frame_index;
      std::string message = "XeGTAO: skipped this frame (";
      message += reason;
      if (device != nullptr) {
        const auto depth_info = GetXeGTAOCapturedViewInfo(device, data->captured_depth_srv);
        const auto ssao_info = GetXeGTAOCapturedViewInfo(device, data->captured_ssao_srv);
        message += "; ";
        message += FormatXeGTAOCapturedViewInfo("t3", depth_info);
        message += "; ";
        message += FormatXeGTAOCapturedViewInfo("t4", ssao_info);
        message += "; ";
        message += FormatXeGTAOSceneCbvInfo(data);
      }
      message += ")";
      AddonLog(reshade::log::level::warning, message.c_str());
    }
    return false;
  };
  if (device == nullptr) return fail("device is null");

  if (data->present_frame_index < kXeGTAOStartupDispatchGuardFrames) {
    return fail(std::format(
        "startup dispatch guard active (frame {} < {})",
        data->present_frame_index,
        kXeGTAOStartupDispatchGuardFrames));
  }

  data->xegtao_copyback_frame = data->present_frame_index;
  data->xegtao_copyback_requested_for_frame = request_copy_back;
  data->xegtao_copyback_succeeded_for_frame = false;
  data->xegtao_copyback_active_for_apply = false;
  data->xegtao_trace_main_pass_executed = false;
  data->xegtao_trace_composite_pass_executed = false;
  data->xegtao_trace_copyback_requested = request_copy_back;
  data->xegtao_trace_copyback_succeeded = false;

  if (data->xegtao_result_signature_frame != data->present_frame_index) {
    data->xegtao_result_signature_valid = false;
    if (data->xegtao_owner_frame != data->present_frame_index) {
      data->xegtao_owner_valid = false;
    }
  }

  data->xegtao_mrt_normal_frame = kInvalidFrameIndex;
  data->xegtao_mrt_normal_valid = false;

  if (data->captured_scene_cbv_valid && !IsSceneCbvCandidateValid(device, data->captured_scene_cbv)) {
    data->captured_scene_cbv = {};
    data->captured_scene_cbv_valid = false;
    data->captured_scene_cbv_frame = kInvalidFrameIndex;
    data->captured_scene_cbv_source = XeGTAOSceneCbvSource::kNone;
  }

  if (data->captured_scene_cbv_valid
      && data->captured_scene_cbv_source == XeGTAOSceneCbvSource::kCurrentLighting
      && data->captured_scene_cbv_frame != data->present_frame_index) {
    data->captured_scene_cbv = {};
    data->captured_scene_cbv_valid = false;
    data->captured_scene_cbv_frame = kInvalidFrameIndex;
    data->captured_scene_cbv_source = XeGTAOSceneCbvSource::kNone;
  }

  if (resolve_inputs_from_current_bindings && !IsViewAlive(device, data->captured_mrt_normal_srv)) {
    data->captured_mrt_normal_srv = {};
  }

  if (resolve_inputs_from_current_bindings) {
    // Always refresh from currently bound lighting descriptors so quality-path transitions
    // (e.g. high <-> ultra resolution) cannot keep stale-but-alive captures.
    ResolveXeGTAOInputsFromCurrentBindings(cmd_list, data);

    const bool has_current_scene_cbv = data->resolved_scene_cbv_from_current_bindings
        && data->captured_scene_cbv_valid
        && data->captured_scene_cbv_frame == data->present_frame_index
        && data->captured_scene_cbv.buffer.handle != 0u
        && IsSceneCbvCandidateValid(device, data->captured_scene_cbv);

    if (!has_current_scene_cbv) {
      data->captured_scene_cbv = {};
      data->captured_scene_cbv_valid = false;
      data->captured_scene_cbv_frame = kInvalidFrameIndex;
      data->captured_scene_cbv_source = XeGTAOSceneCbvSource::kNone;
    }

    if (!has_current_scene_cbv) {
      (void)TryAdoptFallbackSceneCbv(device, data);
    }
  } else {
    const bool has_frozen_scene_cbv = data->captured_scene_cbv_valid
        && data->captured_scene_cbv_frame == data->present_frame_index
        && data->captured_scene_cbv.buffer.handle != 0u
        && IsSceneCbvCandidateValid(device, data->captured_scene_cbv);
    if (!has_frozen_scene_cbv) {
      (void)TryAdoptFallbackSceneCbv(device, data);
    }
    data->resolved_scene_cbv_from_current_bindings = false;
  }

  LogXeGTAOCaptureDiagnostics(device, data);

  if (!IsViewAlive(device, data->captured_depth_srv)) return fail("lighting depth t3 is not captured/alive");
  if (!IsViewAlive(device, data->captured_ssao_srv)) return fail("lighting AO t4 is not captured/alive");
  if (!data->captured_scene_cbv_valid
      || data->captured_scene_cbv_frame != data->present_frame_index
      || data->captured_scene_cbv.buffer.handle == 0u
      || !IsSceneCbvCandidateValid(device, data->captured_scene_cbv)) {
    std::string reason = "lighting scene CB b0 is not captured";
    const uint64_t fallback_activation_frame = GetXeGTAOFallbackActivationFrame(data);
    if (data->present_frame_index < fallback_activation_frame) {
      reason += std::format(
          " (fallback activation blocked: frame {} < unlock {})",
          data->present_frame_index,
          fallback_activation_frame);
    } else if (!data->fallback_scene_cbv_seen) {
      reason += " (tracked + fallback cache missing)";
    } else if (!IsSceneCbvCandidateValid(device, data->fallback_scene_cbv)) {
      reason += " (fallback cache rejected by validation)";
    } else if (data->fallback_scene_cbv_frame == kInvalidFrameIndex) {
      reason += " (fallback cache frame is invalid)";
    } else if (data->fallback_scene_cbv_stable_count < kXeGTAOFallbackSceneCbvRequiredStableFrames) {
      reason += std::format(
          " (fallback cache warmup: stable_count={} < {})",
          data->fallback_scene_cbv_stable_count,
          kXeGTAOFallbackSceneCbvRequiredStableFrames);
    } else if (data->present_frame_index < kXeGTAOStartupRequireCurrentSceneCbvFrames
        && data->present_frame_index >= data->fallback_scene_cbv_frame
        && data->present_frame_index - data->fallback_scene_cbv_frame > kXeGTAOFallbackSceneCbvMaxAgeFrames) {
      reason += std::format(
          " (fallback cache is stale during startup: age={} frames)",
          data->present_frame_index - data->fallback_scene_cbv_frame);
    } else {
      reason += " (descriptor-table resolve did not expose a valid b0)";
    }
    return fail(reason);
  }
  if (data->present_frame_index < kXeGTAOStartupRequireCurrentSceneCbvFrames
      && data->captured_scene_cbv_source != XeGTAOSceneCbvSource::kCurrentLighting) {
    return fail(std::format(
        "startup scene CBV guard active (source={}, frame {} < {})",
        GetXeGTAOSceneCbvSourceName(data->captured_scene_cbv_source),
        data->present_frame_index,
        kXeGTAOStartupRequireCurrentSceneCbvFrames));
  }
  if (!EnsurePointClampSampler(device, data)) return fail("point clamp sampler creation failed");
  if (!EnsureXeGTAOPipelines(device, data)) return fail("pipeline/layout setup failed");
  if (!EnsureXeGTAOResources(device, data)) return fail("working resource setup failed");

  const uint32_t width = data->working_width;
  const uint32_t height = data->working_height;
  auto note_predispatch_reject = [data](const char* reason) {
    if (data == nullptr) return;
    data->xegtao_debug_predispatch_reject_count += 1u;
    if (data->xegtao_debug_last_predispatch_reject_frame != data->present_frame_index) {
      data->xegtao_debug_last_predispatch_reject_frame = data->present_frame_index;
      AddonLog(
          reshade::log::level::warning,
          std::format("XeGTAO pre-dispatch reject: {} (frame={})", reason, data->present_frame_index));
    }
  };
  if (width == 0u || height == 0u) {
    note_predispatch_reject("working texture dimensions are zero");
    return fail("working texture dimensions are zero");
  }
  if (!IsViewAlive(device, data->captured_depth_srv)) {
    note_predispatch_reject("lighting depth t3 became invalid before dispatch");
    return fail("lighting depth t3 became invalid before dispatch");
  }
  if (!IsViewAlive(device, data->captured_ssao_srv)) {
    note_predispatch_reject("lighting AO t4 became invalid before dispatch");
    return fail("lighting AO t4 became invalid before dispatch");
  }
  if (!data->captured_scene_cbv_valid
      || data->captured_scene_cbv.buffer.handle == 0u
      || !IsSceneCbvCandidateValid(device, data->captured_scene_cbv)) {
    note_predispatch_reject("lighting scene CB b0 became invalid before dispatch");
    return fail("lighting scene CB b0 became invalid before dispatch");
  }
  reshade::api::buffer_range normalized_scene_cbv = {};
  if (!TryNormalizeSceneCbvRange(device, data->captured_scene_cbv, &normalized_scene_cbv)) {
    note_predispatch_reject("lighting scene CB b0 range is unsafe for dispatch");
    return fail("lighting scene CB b0 range is unsafe for dispatch");
  }
  data->captured_scene_cbv = normalized_scene_cbv;
  const uint32_t fix_mode = ClampXeGTAOFixMode();
  const bool l5_pass_isolation =
      fix_mode >= static_cast<uint32_t>(XeGTAOFixMode::kPassIsolationDiagnostics);
  const bool run_prefilter_pass = !l5_pass_isolation || ClampBooleanToggle(xegtao_fix_l5_prefilter) != 0u;
  const bool run_main_pass = !l5_pass_isolation || ClampBooleanToggle(xegtao_fix_l5_main) != 0u;
  const bool run_denoise_pass = !l5_pass_isolation || ClampBooleanToggle(xegtao_fix_l5_denoise) != 0u;
  const bool run_composite_pass = !l5_pass_isolation || ClampBooleanToggle(xegtao_fix_l5_composite) != 0u;
  if (!run_prefilter_pass || !run_main_pass || !run_composite_pass) {
    return fail("level-5 pass isolation disabled a required producer pass");
  }
  const bool force_downscaled_quality = IsXeGTAOCurrentInputDownscaled(device, data);

    const reshade::api::pipeline prefilter_pipeline = data->xegtao_prefilter_pipeline;
    const reshade::api::pipeline main_pipeline = data->xegtao_main_pipeline;
    const reshade::api::pipeline denoise_pipeline = data->xegtao_denoise_pipeline;
    const reshade::api::pipeline normal_cap_pipeline = data->xegtao_normal_cap_pipeline;

  std::array<reshade::api::resource_view, 1> prefilter_srvs = {
      data->captured_depth_srv,
  };
  std::array<reshade::api::resource_view, 5> prefilter_uavs = {
      data->depth_mips_uavs[0],
      data->depth_mips_uavs[1],
      data->depth_mips_uavs[2],
      data->depth_mips_uavs[3],
      data->depth_mips_uavs[4],
  };
  auto prefilter_constants = BuildXeGTAOPushConstants(data, false, force_downscaled_quality);
  std::array<reshade::api::descriptor_table_update, 4> prefilter_updates = {
      reshade::api::descriptor_table_update{{}, 0, 0, 1, reshade::api::descriptor_type::sampler, &data->point_clamp_sampler},
      reshade::api::descriptor_table_update{{}, 0, 0, 1, reshade::api::descriptor_type::constant_buffer, &data->captured_scene_cbv},
      reshade::api::descriptor_table_update{{}, 0, 0, 1, reshade::api::descriptor_type::texture_shader_resource_view, prefilter_srvs.data()},
      reshade::api::descriptor_table_update{{}, 0, 0, 5, reshade::api::descriptor_type::texture_unordered_access_view, prefilter_uavs.data()},
  };
    TransitionResource(
      cmd_list,
      data->depth_mips_texture,
      reshade::api::resource_usage::shader_resource,
      reshade::api::resource_usage::unordered_access);
    if (!DispatchXeGTAOCompute(
        cmd_list,
        data->xegtao_prefilter_layout,
        prefilter_pipeline,
      &data->xegtao_prefilter_descriptor_tables,
        std::span<const reshade::api::descriptor_table_update>(prefilter_updates.data(), prefilter_updates.size()),
        std::span<const float>(prefilter_constants.data(), prefilter_constants.size()),
        (width + 15u) / 16u,
        (height + 15u) / 16u,
        1u)) {
    return fail("prefilter dispatch failed");
    }
    TransitionResource(
      cmd_list,
      data->depth_mips_texture,
      reshade::api::resource_usage::unordered_access,
      reshade::api::resource_usage::shader_resource);

  const auto is_mrt_normal_available_for_dispatch = [&]() -> bool {
    if (resolve_inputs_from_current_bindings) {
      return IsViewAlive(device, data->captured_mrt_normal_srv);
    }
    return data->captured_mrt_normal_srv.handle != 0u;
  };
  const bool mrt_normal_valid = is_mrt_normal_available_for_dispatch();
  if (ClampXeGTAONormalInputMode() == 1u && !mrt_normal_valid) {
    data->xegtao_debug_normal_fallback_count += 1u;
    if (data->xegtao_debug_last_normal_fallback_frame != data->present_frame_index) {
      data->xegtao_debug_last_normal_fallback_frame = data->present_frame_index;
      AddonLog(
          reshade::log::level::info,
          std::format(
              "XeGTAO transformed normal unavailable at dispatch: MRT normal SRV invalid, using depth fallback (frame={})",
              data->present_frame_index));
    }
  }
  auto dispatch_main_pass = [&](bool force_depth_only, reshade::api::resource_view ao_uav, reshade::api::resource ao_texture, const char* pass_name) {
    const bool mrt_normal_valid_now =
      !force_depth_only && is_mrt_normal_available_for_dispatch();
    std::array<reshade::api::resource_view, 2> main_srvs = {
        data->depth_mips_srv,
        data->captured_mrt_normal_srv,
    };
    const uint32_t main_srv_count = mrt_normal_valid_now ? 2u : 1u;
    std::array<reshade::api::resource_view, 2> main_uavs = {
        ao_uav,
        data->edges_uav,
    };
    auto main_constants = BuildXeGTAOPushConstants(data, false, force_downscaled_quality);
    main_constants[13] = force_depth_only ? 0.f : main_constants[13];
    main_constants[14] = mrt_normal_valid_now ? 1.f : 0.f;

    std::array<reshade::api::descriptor_table_update, 4> main_updates = {
        reshade::api::descriptor_table_update{{}, 0, 0, 1, reshade::api::descriptor_type::sampler, &data->point_clamp_sampler},
        reshade::api::descriptor_table_update{{}, 0, 0, 1, reshade::api::descriptor_type::constant_buffer, &data->captured_scene_cbv},
        reshade::api::descriptor_table_update{{}, 0, 0, main_srv_count, reshade::api::descriptor_type::texture_shader_resource_view, main_srvs.data()},
        reshade::api::descriptor_table_update{{}, 0, 0, 2, reshade::api::descriptor_type::texture_unordered_access_view, main_uavs.data()},
    };
    TransitionResource(
        cmd_list,
        ao_texture,
        reshade::api::resource_usage::shader_resource,
        reshade::api::resource_usage::unordered_access);
    TransitionResource(
        cmd_list,
        data->edges_texture,
        reshade::api::resource_usage::shader_resource,
        reshade::api::resource_usage::unordered_access);
    if (!DispatchXeGTAOCompute(
            cmd_list,
            data->xegtao_main_layout,
            main_pipeline,
          &data->xegtao_main_descriptor_tables,
            std::span<const reshade::api::descriptor_table_update>(main_updates.data(), main_updates.size()),
            std::span<const float>(main_constants.data(), main_constants.size()),
            (width + 7u) / 8u,
            (height + 7u) / 8u,
            1u)) {
      return fail(std::string(pass_name) + " dispatch failed");
    }
    TransitionResource(
        cmd_list,
        ao_texture,
        reshade::api::resource_usage::unordered_access,
        reshade::api::resource_usage::shader_resource);
    TransitionResource(
        cmd_list,
        data->edges_texture,
        reshade::api::resource_usage::unordered_access,
        reshade::api::resource_usage::shader_resource);
    return true;
  };

  const bool exact_normal_cap_enabled =
      ClampXeGTAONormalDarkeningMode() == 1u
      && ClampXeGTAONormalInputMode() == 1u
      && mrt_normal_valid
      && xegtao_normal_max_darkening < 0.999f;

  if (exact_normal_cap_enabled) {
    data->xegtao_trace_main_pass_executed = true;
    if (!dispatch_main_pass(true, data->ao_term_b_uav, data->ao_term_b_texture, "main baseline")) return false;
    if (!dispatch_main_pass(false, data->ao_term_a_uav, data->ao_term_a_texture, "main")) return false;

    std::array<reshade::api::resource_view, 2> normal_cap_srvs = {
        data->ao_term_b_srv,
        data->ao_term_a_srv,
    };
    std::array<reshade::api::resource_view, 1> normal_cap_uavs = {
        data->ao_term_a_uav,
    };
    auto normal_cap_constants = BuildXeGTAOPushConstants(data, false, force_downscaled_quality);
    std::array<reshade::api::descriptor_table_update, 4> normal_cap_updates = {
        reshade::api::descriptor_table_update{{}, 0, 0, 1, reshade::api::descriptor_type::sampler, &data->point_clamp_sampler},
        reshade::api::descriptor_table_update{{}, 0, 0, 1, reshade::api::descriptor_type::constant_buffer, &data->captured_scene_cbv},
        reshade::api::descriptor_table_update{{}, 0, 0, 2, reshade::api::descriptor_type::texture_shader_resource_view, normal_cap_srvs.data()},
        reshade::api::descriptor_table_update{{}, 0, 0, 1, reshade::api::descriptor_type::texture_unordered_access_view, normal_cap_uavs.data()},
    };
    TransitionResource(
        cmd_list,
        data->ao_term_a_texture,
        reshade::api::resource_usage::shader_resource,
        reshade::api::resource_usage::unordered_access);
    if (!DispatchXeGTAOCompute(
            cmd_list,
            data->xegtao_normal_cap_layout,
            normal_cap_pipeline,
          &data->xegtao_normal_cap_descriptor_tables,
            std::span<const reshade::api::descriptor_table_update>(normal_cap_updates.data(), normal_cap_updates.size()),
            std::span<const float>(normal_cap_constants.data(), normal_cap_constants.size()),
            (width + 7u) / 8u,
            (height + 7u) / 8u,
            1u)) {
      return fail("normal cap dispatch failed");
    }
    TransitionResource(
        cmd_list,
        data->ao_term_a_texture,
        reshade::api::resource_usage::unordered_access,
        reshade::api::resource_usage::shader_resource);
  } else {
    data->xegtao_trace_main_pass_executed = true;
    if (!dispatch_main_pass(false, data->ao_term_a_uav, data->ao_term_a_texture, "main")) return false;
  }

  data->xegtao_mrt_normal_frame = data->present_frame_index;
  data->xegtao_mrt_normal_valid = mrt_normal_valid;

  const uint32_t denoise_passes = run_denoise_pass
      ? (force_downscaled_quality ? 3u : ClampXeGTAODenoisePasses())
      : 0u;
  bool denoise_source_is_a = true;

  auto dispatch_vanilla_denoise_pass =
      [&](bool is_last_pass, uint32_t pass_index) -> bool {
    const reshade::api::resource_view denoise_source =
        denoise_source_is_a ? data->ao_term_a_srv : data->ao_term_b_srv;
    const reshade::api::resource_view denoise_output =
        denoise_source_is_a ? data->ao_term_b_uav : data->ao_term_a_uav;

    std::array<reshade::api::resource_view, 2> denoise_srvs = {
        denoise_source,
        data->edges_srv,
    };
    std::array<reshade::api::resource_view, 1> denoise_uavs = {
        denoise_output,
    };
    auto denoise_constants = BuildXeGTAOPushConstants(data, is_last_pass, force_downscaled_quality);
    denoise_constants[10] = denoise_constants[10] + static_cast<float>(pass_index);
    denoise_constants[31] = 0.f;
    std::array<reshade::api::descriptor_table_update, 4> denoise_updates = {
        reshade::api::descriptor_table_update{{}, 0, 0, 1, reshade::api::descriptor_type::sampler, &data->point_clamp_sampler},
        reshade::api::descriptor_table_update{{}, 0, 0, 1, reshade::api::descriptor_type::constant_buffer, &data->captured_scene_cbv},
        reshade::api::descriptor_table_update{{}, 0, 0, 2, reshade::api::descriptor_type::texture_shader_resource_view, denoise_srvs.data()},
        reshade::api::descriptor_table_update{{}, 0, 0, 1, reshade::api::descriptor_type::texture_unordered_access_view, denoise_uavs.data()},
    };
    const reshade::api::resource denoise_output_resource =
        denoise_source_is_a ? data->ao_term_b_texture : data->ao_term_a_texture;
    TransitionResource(
        cmd_list,
        denoise_output_resource,
        reshade::api::resource_usage::shader_resource,
        reshade::api::resource_usage::unordered_access);
    if (!DispatchXeGTAOCompute(
            cmd_list,
            data->xegtao_denoise_layout,
            denoise_pipeline,
          &data->xegtao_denoise_descriptor_tables,
            std::span<const reshade::api::descriptor_table_update>(denoise_updates.data(), denoise_updates.size()),
            std::span<const float>(denoise_constants.data(), denoise_constants.size()),
            (width + 15u) / 16u,
            (height + 7u) / 8u,
            1u)) {
      return false;
    }
    TransitionResource(
        cmd_list,
        denoise_output_resource,
        reshade::api::resource_usage::unordered_access,
        reshade::api::resource_usage::shader_resource);
    denoise_source_is_a = !denoise_source_is_a;
    return true;
  };

  const uint32_t vanilla_total_passes = std::max(1u, denoise_passes);
  for (uint32_t pass_index = 0u; pass_index < vanilla_total_passes; ++pass_index) {
    const bool is_last_pass = pass_index + 1u == vanilla_total_passes;
    if (!dispatch_vanilla_denoise_pass(is_last_pass, pass_index)) {
      return fail("vanilla denoise dispatch failed");
    }
  }

  const bool denoise_result_is_a = denoise_source_is_a;
  const reshade::api::resource_view gtao_result_srv = denoise_result_is_a ? data->ao_term_a_srv : data->ao_term_b_srv;
  if (!IsViewAlive(device, gtao_result_srv)) return fail("AO result SRV is invalid");

  std::array<reshade::api::resource_view, 2> composite_srvs = {
      data->captured_ssao_srv,
      gtao_result_srv,
  };
  std::array<reshade::api::resource_view, 1> composite_uavs = {
      data->composite_uav,
  };
  auto composite_constants = BuildXeGTAOPushConstants(data, true, force_downscaled_quality);
  std::array<reshade::api::descriptor_table_update, 4> composite_updates = {
      reshade::api::descriptor_table_update{{}, 0, 0, 1, reshade::api::descriptor_type::sampler, &data->point_clamp_sampler},
      reshade::api::descriptor_table_update{{}, 0, 0, 1, reshade::api::descriptor_type::constant_buffer, &data->captured_scene_cbv},
      reshade::api::descriptor_table_update{{}, 0, 0, 2, reshade::api::descriptor_type::texture_shader_resource_view, composite_srvs.data()},
      reshade::api::descriptor_table_update{{}, 0, 0, 1, reshade::api::descriptor_type::texture_unordered_access_view, composite_uavs.data()},
  };
  TransitionResource(
      cmd_list,
      data->composite_texture,
      reshade::api::resource_usage::shader_resource,
      reshade::api::resource_usage::unordered_access);
  data->xegtao_trace_composite_pass_executed = true;
  if (!DispatchXeGTAOCompute(
          cmd_list,
          data->xegtao_composite_layout,
          data->xegtao_composite_pipeline,
      &data->xegtao_composite_descriptor_tables,
          std::span<const reshade::api::descriptor_table_update>(composite_updates.data(), composite_updates.size()),
          std::span<const float>(composite_constants.data(), composite_constants.size()),
          (width + 7u) / 8u,
          (height + 7u) / 8u,
          1u)) {
    return fail("composite dispatch failed");
  }
  if (!request_copy_back) {
    TransitionResource(
        cmd_list,
        data->composite_texture,
        reshade::api::resource_usage::unordered_access,
        reshade::api::resource_usage::shader_resource);
  }

  if (request_copy_back) {
    data->last_copyback_frame = data->present_frame_index;
    data->copyback_succeeded = TryCopyBackXeGTAOResult(cmd_list, data);
    data->xegtao_copyback_succeeded_for_frame = data->copyback_succeeded;
    data->xegtao_copyback_active_for_apply = data->copyback_succeeded;
    data->xegtao_trace_copyback_succeeded = data->copyback_succeeded;
    if (!data->copyback_succeeded) {
      TransitionResource(
          cmd_list,
          data->composite_texture,
          reshade::api::resource_usage::unordered_access,
          reshade::api::resource_usage::shader_resource);
    }
  } else {
    data->last_copyback_frame = kInvalidFrameIndex;
    data->copyback_succeeded = false;
    data->xegtao_copyback_succeeded_for_frame = false;
    data->xegtao_copyback_active_for_apply = false;
    data->xegtao_trace_copyback_succeeded = false;
  }

  const auto signature_t3 = GetXeGTAOCapturedViewInfo(device, data->captured_depth_srv);
  const auto signature_t4 = GetXeGTAOCapturedViewInfo(device, data->captured_ssao_srv);
  data->xegtao_result_signature_valid =
      signature_t3.alive
      && signature_t4.alive
      && signature_t3.resource_desc.type == reshade::api::resource_type::texture_2d
      && signature_t4.resource_desc.type == reshade::api::resource_type::texture_2d
      && signature_t3.resource.handle != 0u
      && signature_t4.resource.handle != 0u;
  data->xegtao_result_signature_frame = data->present_frame_index;
  data->xegtao_result_t3_resource_handle = signature_t3.resource.handle;
  data->xegtao_result_t4_resource_handle = signature_t4.resource.handle;
  data->xegtao_result_t3_view_handle = signature_t3.view.handle;
  data->xegtao_result_t4_view_handle = signature_t4.view.handle;
  data->xegtao_result_t3_width = signature_t3.width;
  data->xegtao_result_t3_height = signature_t3.height;
  data->xegtao_result_t4_width = signature_t4.width;
  data->xegtao_result_t4_height = signature_t4.height;
  data->xegtao_result_working_width = data->working_width;
  data->xegtao_result_working_height = data->working_height;
  data->last_gtao_frame = data->present_frame_index;
  return true;
}

bool TryResolveTextureRegister(
    reshade::api::pipeline_layout layout,
    uint32_t layout_param,
    uint32_t binding,
    uint32_t* out_register,
    uint32_t* out_space) {
  if (out_register == nullptr || out_space == nullptr) return false;
  auto* layout_data = renodx::utils::pipeline_layout::GetPipelineLayoutData(layout);
  if (layout_data == nullptr) return false;
  if (layout_param >= layout_data->params.size()) return false;

  const auto& param = layout_data->params.at(layout_param);
  auto resolve_range = [binding, out_register, out_space](const reshade::api::descriptor_range& range) -> bool {
    if (range.count == UINT32_MAX) return false;
    if (binding < range.binding || binding >= (range.binding + range.count)) return false;
    *out_register = range.dx_register_index + (binding - range.binding);
    *out_space = range.dx_register_space;
    return true;
  };

  switch (param.type) {
    case reshade::api::pipeline_layout_param_type::push_descriptors: {
      return resolve_range(param.push_descriptors);
    }
    case reshade::api::pipeline_layout_param_type::push_descriptors_with_ranges: {
      for (uint32_t i = 0; i < param.descriptor_table.count; ++i) {
        if (resolve_range(param.descriptor_table.ranges[i])) return true;
      }
      return false;
    }
    case reshade::api::pipeline_layout_param_type::descriptor_table: {
      for (uint32_t i = 0; i < param.descriptor_table.count; ++i) {
        if (resolve_range(param.descriptor_table.ranges[i])) return true;
      }
      return false;
    }
    case reshade::api::pipeline_layout_param_type::descriptor_table_with_static_samplers: {
#if RESHADE_API_VERSION >= 13
      for (uint32_t i = 0; i < param.descriptor_table_with_static_samplers.count; ++i) {
        if (resolve_range(param.descriptor_table_with_static_samplers.ranges[i])) return true;
      }
#endif
      return false;
    }
    default:
      return false;
  }
}

void CaptureTrackedTextureView(
    reshade::api::command_list* cmd_list,
    const uint32_t shader_hash,
    const uint32_t reg,
    const uint32_t space,
    const reshade::api::resource_view view) {
  if (g_skip_descriptor_capture) return;
  if (cmd_list == nullptr) return;
  if (view.handle == 0u) return;
  if (space != 0u) return;

  auto* device = cmd_list->get_device();
  auto* data = device != nullptr ? device->get_private_data<DeviceData>() : nullptr;

  if (shader_hash == kCharacterShader && reg == 2u) {
    // Character pass uses mrtTexture0 at t2.
    g_character_mrt0_view.store(view.handle, std::memory_order_relaxed);
    return;
  }

  if (shader_hash != kLightingShader && shader_hash != kLightingSoftShader) return;

  if (reg == kLightingMrtNormalRegister) {
    // Keep lighting mrt0 as fallback in case character pass capture is unavailable.
    g_lighting_mrt0_view.store(view.handle, std::memory_order_relaxed);
    if (data != nullptr) {
      data->captured_mrt_normal_srv = view;
    }
    return;
  }

  if (data == nullptr) return;
  if (reg == kLightingDepthRegister) {
    data->captured_depth_srv = view;
  } else if (reg == kLightingSsaoRegister) {
    data->captured_ssao_srv = view;
  }
}

void CaptureTrackedConstantBuffer(
    reshade::api::command_list* cmd_list,
    const uint32_t shader_hash,
    const uint32_t reg,
    const uint32_t space,
    const reshade::api::buffer_range& range) {
  if (g_skip_descriptor_capture) return;
  if (cmd_list == nullptr) return;
  if (space != 0u) return;
  if (range.buffer.handle == 0u) return;
  if (shader_hash != kLightingShader && shader_hash != kLightingSoftShader) return;
  if (reg != kLightingSceneCbRegister) return;

  auto* device = cmd_list->get_device();
  if (device == nullptr) return;
  auto* data = device->get_private_data<DeviceData>();
  if (data == nullptr) return;
  reshade::api::buffer_range normalized = {};
  if (!TryNormalizeSceneCbvRange(device, range, &normalized)) return;
  data->captured_scene_cbv = normalized;
  data->captured_scene_cbv_valid = true;
  data->captured_scene_cbv_frame = data->present_frame_index;
  data->captured_scene_cbv_source = XeGTAOSceneCbvSource::kCurrentLighting;
}

void OnPushDescriptorsCaptureLightingTextures(
    reshade::api::command_list* cmd_list,
    reshade::api::shader_stage stages,
    reshade::api::pipeline_layout layout,
    uint32_t layout_param,
    const reshade::api::descriptor_table_update& update) {
  if (g_skip_descriptor_capture) return;
  if (cmd_list == nullptr) return;
  const uint32_t stage_mask = static_cast<uint32_t>(stages);
  const uint32_t pixel_mask = static_cast<uint32_t>(reshade::api::shader_stage::pixel);
  const uint32_t vertex_mask = static_cast<uint32_t>(reshade::api::shader_stage::vertex);
  const bool has_pixel_stage = (stage_mask & pixel_mask) != 0u;
  const bool has_vertex_or_pixel_stage = (stage_mask & (pixel_mask | vertex_mask)) != 0u;
  if (!has_vertex_or_pixel_stage) return;

  auto* shader_state = renodx::utils::shader::GetCurrentState(cmd_list);
  const auto shader_hash = shader_state != nullptr
      ? renodx::utils::shader::GetCurrentPixelShaderHash(shader_state)
      : 0u;
  auto* device = cmd_list->get_device();
  auto* data = device != nullptr ? device->get_private_data<DeviceData>() : nullptr;
  const bool is_tracked_shader =
      shader_hash == kLightingShader || shader_hash == kLightingSoftShader || shader_hash == kCharacterShader;
  const bool has_current_frame_tracked_cbv =
      data != nullptr
      && data->captured_scene_cbv_valid
      && data->captured_scene_cbv_source == XeGTAOSceneCbvSource::kCurrentLighting
      && data->captured_scene_cbv_frame == data->present_frame_index
      && data->captured_scene_cbv.buffer.handle != 0u;
  // Unknown hash capture is only allowed while current-frame tracked b0 is still missing.
  const bool allow_fallback_scene_cb_capture =
      shader_hash == kLightingShader
      || shader_hash == kLightingSoftShader
      || (shader_hash == 0u && !has_current_frame_tracked_cbv);

  for (uint32_t i = 0; i < update.count; ++i) {
    uint32_t reg = 0u;
    uint32_t space = 0u;
    if (!TryResolveTextureRegister(layout, layout_param, update.binding + i, &reg, &space)) {
      reg = update.binding + i;
      space = 0u;
    }

    if (update.type == reshade::api::descriptor_type::constant_buffer) {
      const auto* ranges = static_cast<const reshade::api::buffer_range*>(update.descriptors);
      if (ranges == nullptr) continue;
      if (allow_fallback_scene_cb_capture && has_pixel_stage && reg == kLightingSceneCbRegister && space == 0u) {
        CacheFallbackSceneCbv(cmd_list, ranges[i]);
      }
      if (is_tracked_shader) {
        CaptureTrackedConstantBuffer(cmd_list, shader_hash, reg, space, ranges[i]);
      }
      continue;
    }

    if (!has_pixel_stage) continue;
    if (!is_tracked_shader) continue;

    const auto view = renodx::utils::descriptor::GetResourceViewFromDescriptorUpdate(update, i);
    CaptureTrackedTextureView(cmd_list, shader_hash, reg, space, view);
  }
}

bool TryGetResourceViewFromBoundDescriptorTable(
    reshade::api::device* device,
    const reshade::api::descriptor_table& table,
    const reshade::api::descriptor_range& range,
    const uint32_t descriptor_index,
    reshade::api::resource_view* out_view) {
  if (device == nullptr || out_view == nullptr) return false;
  if (table.handle == 0u) return false;
  if (range.count == UINT32_MAX) return false;
  if (descriptor_index >= range.count) return false;

  auto* descriptor_data = renodx::utils::data::Get<renodx::utils::descriptor::DeviceData>(device);
  if (descriptor_data == nullptr) return false;

  uint32_t base_offset = 0u;
  reshade::api::descriptor_heap heap = {0u};
  device->get_descriptor_heap_offset(table, range.binding, 0, &heap, &base_offset);
  if (heap.handle == 0u) return false;

  const std::shared_lock lock(descriptor_data->mutex);

  auto heap_pair = descriptor_data->heaps.find(heap.handle);
  if (heap_pair == descriptor_data->heaps.end()) return false;

  auto known_pair = descriptor_data->resource_view_heap_locations.find(heap.handle);
  if (known_pair == descriptor_data->resource_view_heap_locations.end()) return false;

  const uint32_t offset = base_offset + descriptor_index;
  const auto& heap_entries = heap_pair->second;
  if (offset >= heap_entries.size()) return false;
  if (!known_pair->second.contains(offset)) return false;

  const auto& [descriptor_type, descriptor_value] = heap_entries[offset];
  reshade::api::resource_view view = {};
  switch (descriptor_type) {
    case reshade::api::descriptor_type::sampler_with_resource_view:
      view = std::get<reshade::api::sampler_with_resource_view>(descriptor_value).view;
      break;
    case reshade::api::descriptor_type::buffer_shader_resource_view:
    case reshade::api::descriptor_type::texture_shader_resource_view:
    case reshade::api::descriptor_type::buffer_unordered_access_view:
    case reshade::api::descriptor_type::texture_unordered_access_view:
      view = std::get<reshade::api::resource_view>(descriptor_value);
      break;
    default:
      return false;
  }
  if (view.handle == 0u) return false;

  *out_view = view;
  return true;
}

bool TryGetBufferRangeFromBoundDescriptorTable(
    reshade::api::device* device,
    const reshade::api::descriptor_table& table,
    const reshade::api::descriptor_range& range,
    const uint32_t descriptor_index,
    reshade::api::buffer_range* out_range) {
  if (device == nullptr || out_range == nullptr) return false;
  if (table.handle == 0u) return false;
  if (range.count == UINT32_MAX) return false;
  if (descriptor_index >= range.count) return false;

  auto* descriptor_data = renodx::utils::data::Get<renodx::utils::descriptor::DeviceData>(device);
  if (descriptor_data == nullptr) return false;

  uint32_t base_offset = 0u;
  reshade::api::descriptor_heap heap = {0u};
  device->get_descriptor_heap_offset(table, range.binding, 0, &heap, &base_offset);
  if (heap.handle == 0u) return false;

  const std::shared_lock lock(descriptor_data->mutex);

  auto heap_pair = descriptor_data->heaps.find(heap.handle);
  if (heap_pair == descriptor_data->heaps.end()) return false;

  const uint32_t offset = base_offset + descriptor_index;
  const auto& heap_entries = heap_pair->second;
  if (offset >= heap_entries.size()) return false;

  const auto& [descriptor_type, descriptor_value] = heap_entries[offset];
  if (descriptor_type != reshade::api::descriptor_type::constant_buffer
      && descriptor_type != reshade::api::descriptor_type::shader_storage_buffer) {
    return false;
  }

  const auto buffer_range = std::get<reshade::api::buffer_range>(descriptor_value);
  if (buffer_range.buffer.handle == 0u) return false;
  *out_range = buffer_range;
  return true;
}

void OnBindDescriptorTablesCaptureLightingTextures(
    reshade::api::command_list* cmd_list,
    reshade::api::shader_stage stages,
    reshade::api::pipeline_layout layout,
    uint32_t first,
    uint32_t count,
    const reshade::api::descriptor_table* tables) {
  if (g_skip_descriptor_capture) return;
  if (cmd_list == nullptr || tables == nullptr || count == 0u) return;
  const uint32_t stage_mask = static_cast<uint32_t>(stages);
  const uint32_t pixel_mask = static_cast<uint32_t>(reshade::api::shader_stage::pixel);
  const uint32_t vertex_mask = static_cast<uint32_t>(reshade::api::shader_stage::vertex);
  const bool has_pixel_stage = (stage_mask & pixel_mask) != 0u;
  const bool has_vertex_or_pixel_stage = (stage_mask & (pixel_mask | vertex_mask)) != 0u;
  if (!has_vertex_or_pixel_stage) return;

  auto* shader_state = renodx::utils::shader::GetCurrentState(cmd_list);
  if (shader_state == nullptr) return;
  const auto shader_hash = renodx::utils::shader::GetCurrentPixelShaderHash(shader_state);
  if (shader_hash != kLightingShader && shader_hash != kLightingSoftShader && shader_hash != kCharacterShader) return;

  auto* layout_data = renodx::utils::pipeline_layout::GetPipelineLayoutData(layout);
  if (layout_data == nullptr) return;

  auto* device = cmd_list->get_device();
  for (uint32_t i = 0; i < count; ++i) {
    const uint32_t param_index = first + i;
    if (param_index >= layout_data->params.size()) continue;

    const auto& param = layout_data->params.at(param_index);
    const auto& table = tables[i];
    if (table.handle == 0u) continue;

    uint32_t range_count = 0u;
    const reshade::api::descriptor_range* ranges = nullptr;
    if (param.type == reshade::api::pipeline_layout_param_type::descriptor_table) {
      range_count = param.descriptor_table.count;
      ranges = param.descriptor_table.ranges;
    } else if (param.type == reshade::api::pipeline_layout_param_type::descriptor_table_with_static_samplers) {
      range_count = param.descriptor_table_with_static_samplers.count;
      ranges = param.descriptor_table_with_static_samplers.ranges;
    } else {
      continue;
    }

    for (uint32_t j = 0; j < range_count; ++j) {
      const auto& range = ranges[j];
      if (range.count == UINT32_MAX) continue;

      switch (range.type) {
        case reshade::api::descriptor_type::sampler_with_resource_view:
        case reshade::api::descriptor_type::buffer_shader_resource_view:
        case reshade::api::descriptor_type::texture_shader_resource_view:
        case reshade::api::descriptor_type::constant_buffer:
          break;
        default:
          continue;
      }

      if ((static_cast<uint32_t>(range.visibility) & static_cast<uint32_t>(reshade::api::shader_stage::pixel)) == 0u) {
        if (range.type != reshade::api::descriptor_type::constant_buffer
            || (static_cast<uint32_t>(range.visibility) & vertex_mask) == 0u) {
          continue;
        }
      }

      for (uint32_t k = 0; k < range.count; ++k) {
        const uint32_t reg = range.dx_register_index + k;
        if (range.type == reshade::api::descriptor_type::constant_buffer) {
          reshade::api::buffer_range buffer_range = {};
          if (!TryGetBufferRangeFromBoundDescriptorTable(device, table, range, k, &buffer_range)) continue;
          CaptureTrackedConstantBuffer(cmd_list, shader_hash, reg, range.dx_register_space, buffer_range);
          continue;
        }

        if (!has_pixel_stage) continue;
        reshade::api::resource_view view = {};
        if (!TryGetResourceViewFromBoundDescriptorTable(device, table, range, k, &view)) continue;
        CaptureTrackedTextureView(cmd_list, shader_hash, reg, range.dx_register_space, view);
      }
    }
  }
}

bool ResolveCurrentLightingTextureViewsForDraw(
    reshade::api::command_list* cmd_list,
    uint32_t expected_shader_hash,
    reshade::api::resource_view* out_depth_srv,
    reshade::api::resource_view* out_ssao_srv) {
  if (out_depth_srv == nullptr || out_ssao_srv == nullptr) return false;
  *out_depth_srv = {};
  *out_ssao_srv = {};
  if (cmd_list == nullptr) return false;

  auto* device = cmd_list->get_device();
  if (device == nullptr) return false;

  auto* shader_state = renodx::utils::shader::GetCurrentState(cmd_list);
  if (shader_state == nullptr) return false;
  const uint32_t shader_hash = renodx::utils::shader::GetCurrentPixelShaderHash(shader_state);
  if (shader_hash != expected_shader_hash) return false;
  if (shader_hash != kLightingShader && shader_hash != kLightingSoftShader) return false;

  auto* state = renodx::utils::state::GetCurrentState(cmd_list);
  if (state == nullptr) return false;

  const uint32_t pixel_mask = static_cast<uint32_t>(reshade::api::shader_stage::pixel);

  for (const auto& [stages, descriptor_state] : state->descriptor_tables) {
    if ((static_cast<uint32_t>(stages) & pixel_mask) == 0u) continue;

    const auto& [layout, tables] = descriptor_state;
    if (layout.handle == 0u || tables.empty()) continue;

    auto* layout_data = renodx::utils::pipeline_layout::GetPipelineLayoutData(layout);
    if (layout_data == nullptr) continue;

    for (uint32_t table_index = 0u; table_index < static_cast<uint32_t>(tables.size()); ++table_index) {
      if (table_index >= layout_data->params.size()) break;

      const auto& table = tables[table_index];
      if (table.handle == 0u) continue;

      const auto& param = layout_data->params.at(table_index);
      uint32_t range_count = 0u;
      const reshade::api::descriptor_range* ranges = nullptr;
      if (param.type == reshade::api::pipeline_layout_param_type::descriptor_table) {
        range_count = param.descriptor_table.count;
        ranges = param.descriptor_table.ranges;
      } else if (param.type == reshade::api::pipeline_layout_param_type::descriptor_table_with_static_samplers) {
        range_count = param.descriptor_table_with_static_samplers.count;
        ranges = param.descriptor_table_with_static_samplers.ranges;
      } else {
        continue;
      }
      if (ranges == nullptr || range_count == 0u) continue;

      for (uint32_t range_index = 0u; range_index < range_count; ++range_index) {
        const auto& range = ranges[range_index];
        if (range.count == UINT32_MAX) continue;
        if (range.dx_register_space != 0u) continue;
        if ((static_cast<uint32_t>(range.visibility) & pixel_mask) == 0u) continue;
        if (range.type != reshade::api::descriptor_type::sampler_with_resource_view
            && range.type != reshade::api::descriptor_type::buffer_shader_resource_view
            && range.type != reshade::api::descriptor_type::texture_shader_resource_view) {
          continue;
        }

        auto resolve_texture_register = [&](uint32_t reg, reshade::api::resource_view* out_view) {
          if (out_view == nullptr || out_view->handle != 0u) return;
          if (reg < range.dx_register_index || reg >= (range.dx_register_index + range.count)) return;
          reshade::api::resource_view view = {};
          const uint32_t descriptor_index = reg - range.dx_register_index;
          if (TryGetResourceViewFromBoundDescriptorTable(device, table, range, descriptor_index, &view)) {
            *out_view = view;
          }
        };

        resolve_texture_register(kLightingDepthRegister, out_depth_srv);
        resolve_texture_register(kLightingSsaoRegister, out_ssao_srv);
      }

      if (out_depth_srv->handle != 0u && out_ssao_srv->handle != 0u) return true;
    }
  }

  return out_depth_srv->handle != 0u && out_ssao_srv->handle != 0u;
}

void ResolveXeGTAOInputsFromCurrentBindings(reshade::api::command_list* cmd_list, DeviceData* data) {
  if (cmd_list == nullptr || data == nullptr) return;
  auto* device = cmd_list->get_device();
  if (device == nullptr) return;
  data->resolved_scene_cbv_from_current_bindings = false;
  const uint32_t pixel_mask = static_cast<uint32_t>(reshade::api::shader_stage::pixel);
  const uint32_t vertex_mask = static_cast<uint32_t>(reshade::api::shader_stage::vertex);

  auto* shader_state = renodx::utils::shader::GetCurrentState(cmd_list);
  if (shader_state == nullptr) return;
  const auto shader_hash = renodx::utils::shader::GetCurrentPixelShaderHash(shader_state);
  if (shader_hash != kLightingShader && shader_hash != kLightingSoftShader) return;

  auto* state = renodx::utils::state::GetCurrentState(cmd_list);
  if (state == nullptr) return;
  bool resolved_scene_cbv_from_current = false;

  auto required_inputs_captured = [data, device]() -> bool {
    return IsViewAlive(device, data->captured_depth_srv)
        && IsViewAlive(device, data->captured_ssao_srv)
        && data->captured_scene_cbv_valid
        && data->captured_scene_cbv_frame == data->present_frame_index
        && data->captured_scene_cbv.buffer.handle != 0u;
  };
  auto optional_normal_input_captured = [data, device]() -> bool {
    return IsViewAlive(device, data->captured_mrt_normal_srv);
  };

  for (const auto& [stages, descriptor_state] : state->descriptor_tables) {
    const uint32_t stage_mask = static_cast<uint32_t>(stages);
    const bool has_pixel_stage = (stage_mask & pixel_mask) != 0u;
    if ((stage_mask & (pixel_mask | vertex_mask)) == 0u) continue;

    const auto& [layout, tables] = descriptor_state;
    if (layout.handle == 0u || tables.empty()) continue;

    auto* layout_data = renodx::utils::pipeline_layout::GetPipelineLayoutData(layout);
    if (layout_data == nullptr) continue;

    for (uint32_t table_index = 0u; table_index < static_cast<uint32_t>(tables.size()); ++table_index) {
      if (table_index >= layout_data->params.size()) break;
      const auto& table = tables[table_index];
      if (table.handle == 0u) continue;

      const auto& param = layout_data->params.at(table_index);
      uint32_t range_count = 0u;
      const reshade::api::descriptor_range* ranges = nullptr;
      if (param.type == reshade::api::pipeline_layout_param_type::descriptor_table) {
        range_count = param.descriptor_table.count;
        ranges = param.descriptor_table.ranges;
      } else if (param.type == reshade::api::pipeline_layout_param_type::descriptor_table_with_static_samplers) {
        range_count = param.descriptor_table_with_static_samplers.count;
        ranges = param.descriptor_table_with_static_samplers.ranges;
      } else {
        continue;
      }
      if (ranges == nullptr || range_count == 0u) continue;

      for (uint32_t range_index = 0u; range_index < range_count; ++range_index) {
        const auto& range = ranges[range_index];
        if (range.count == UINT32_MAX) continue;
        if (range.dx_register_space != 0u) continue;
        const uint32_t visibility_mask = static_cast<uint32_t>(range.visibility);

        if (range.type == reshade::api::descriptor_type::constant_buffer) {
          if ((visibility_mask & (pixel_mask | vertex_mask)) == 0u) continue;
          if (kLightingSceneCbRegister >= range.dx_register_index
              && kLightingSceneCbRegister < (range.dx_register_index + range.count)) {
            reshade::api::buffer_range cbv = {};
            reshade::api::buffer_range normalized_cbv = {};
            const uint32_t descriptor_index = kLightingSceneCbRegister - range.dx_register_index;
            if (TryGetBufferRangeFromBoundDescriptorTable(device, table, range, descriptor_index, &cbv)
                && TryNormalizeSceneCbvRange(device, cbv, &normalized_cbv)) {
              data->captured_scene_cbv = normalized_cbv;
              data->captured_scene_cbv_valid = true;
              data->captured_scene_cbv_frame = data->present_frame_index;
              data->captured_scene_cbv_source = XeGTAOSceneCbvSource::kCurrentLighting;
              resolved_scene_cbv_from_current = true;
            }
          }
          continue;
        }

        if (range.type != reshade::api::descriptor_type::sampler_with_resource_view
            && range.type != reshade::api::descriptor_type::buffer_shader_resource_view
            && range.type != reshade::api::descriptor_type::texture_shader_resource_view) {
          continue;
        }
        if (!has_pixel_stage || (visibility_mask & pixel_mask) == 0u) continue;

        auto resolve_texture_register = [&](uint32_t reg, reshade::api::resource_view* out_view) {
          if (out_view == nullptr) return;
          if (reg < range.dx_register_index || reg >= (range.dx_register_index + range.count)) return;
          reshade::api::resource_view view = {};
          const uint32_t descriptor_index = reg - range.dx_register_index;
          if (TryGetResourceViewFromBoundDescriptorTable(device, table, range, descriptor_index, &view)) {
            *out_view = view;
          }
        };
        resolve_texture_register(kLightingDepthRegister, &data->captured_depth_srv);
        resolve_texture_register(kLightingSsaoRegister, &data->captured_ssao_srv);
        resolve_texture_register(kLightingMrtNormalRegister, &data->captured_mrt_normal_srv);
      }

      if (required_inputs_captured()
          && optional_normal_input_captured()
          && resolved_scene_cbv_from_current) {
        data->resolved_scene_cbv_from_current_bindings = resolved_scene_cbv_from_current;
        return;
      }
    }
  }

  data->resolved_scene_cbv_from_current_bindings = resolved_scene_cbv_from_current;
}

void OnCharacterShaderDrawn(reshade::api::command_list* cmd_list) {
  if (cmd_list == nullptr) return;
  auto* device = cmd_list->get_device();
  if (device == nullptr || !IsIsFastSupportedDevice(device)) return;

  auto* data = device->get_private_data<DeviceData>();
  if (data == nullptr) return;

  auto* state = renodx::utils::state::GetCurrentState(cmd_list);
  if (state == nullptr || state->render_targets.empty()) return;

  const auto source_rtv = state->render_targets.at(0);
  if (source_rtv.handle == 0u) return;
  (void)UpdateCharacterSssCapture(device, data, source_rtv);
}

void OnAoFinalPassDrawn(reshade::api::command_list* cmd_list) {
  if (cmd_list == nullptr) return;

  auto* device = cmd_list->get_device();
  if (device == nullptr || !IsIsFastSupportedDevice(device)) return;

  auto* data = device->get_private_data<DeviceData>();
  if (data == nullptr) return;
}

void OnVolFogShaderDrawn(reshade::api::command_list* cmd_list) {
  if (cmd_list == nullptr) return;

  auto* device = cmd_list->get_device();
  if (device == nullptr || !IsIsFastSupportedDevice(device)) return;

  auto* data = device->get_private_data<DeviceData>();
  if (data == nullptr) return;

  data->xegtao_volfog_seen_frame = data->present_frame_index;
}

void OnAfterLightingShaderDraw(reshade::api::command_list* cmd_list) {
  (void)cmd_list;
  // Safety reset to prevent per-draw XeGTAO state leaking into later draws that
  // may not pass through our lighting on_draw callback on some runtime paths.
  shader_injection.sss_dedicated_bound = 0.f;
  shader_injection.xegtao_dedicated_bound = 0.f;
  shader_injection.xegtao_force_neutral_x = 0.f;
  shader_injection.xegtao_mrt_normal_valid = 0.f;
  shader_injection.xegtao_debug_mode = 0.f;
  shader_injection.xegtao_debug_blackout = 0.f;
  shader_injection.xegtao_ao_active_for_draw = 0.f;
}

bool OnBeforeVanillaAoDraw(reshade::api::command_list* cmd_list) {
  if (cmd_list == nullptr) return true;
  auto* device = cmd_list->get_device();
  if (device == nullptr || !IsIsFastSupportedDevice(device)) return true;
  if (GetXeGTAOModeSetting() == XeGTAOMode::kOff) return true;
  if (xegtao_skip_vanilla_ao >= 0.5f) {
    auto* data = device->get_private_data<DeviceData>();
    if (data != nullptr) {
      if (data->last_skip_vanilla_ao_ignored_log_frame == kInvalidFrameIndex) {
        data->last_skip_vanilla_ao_ignored_log_frame = data->present_frame_index;
        AddonLog(
            reshade::log::level::warning,
            "XeGTAO: 'Skip Vanilla AO' is ignored while XeGTAO is active (AO.z remains the SSS source).");
      }
    }
  }

  // "Use Vanilla Z" SSS policy: AO.z must stay populated from vanilla AO.
  return true;
}

bool OnBeforeLightingShaderDraw(reshade::api::command_list* cmd_list) {
  if (cmd_list == nullptr) return true;

  shader_injection.sss_dedicated_bound = 0.f;
  shader_injection.xegtao_dedicated_bound = 0.f;
  shader_injection.xegtao_force_neutral_x = 0.f;
  shader_injection.xegtao_debug_blackout = 0.f;
  shader_injection.xegtao_ao_active_for_draw = 0.f;
  shader_injection.xegtao_normal_input_mode = static_cast<float>(ClampXeGTAONormalInputMode());
  shader_injection.xegtao_mrt_normal_valid = 0.f;
  shader_injection.xegtao_bent_normals = 0.f;
  shader_injection.xegtao_bent_diffuse_strength = 0.f;
  shader_injection.xegtao_bent_diffuse_softness = 0.f;
  shader_injection.xegtao_bent_specular_strength = 0.f;
  shader_injection.xegtao_bent_specular_proxy_roughness = 0.f;
  shader_injection.xegtao_bent_max_darkening = 0.f;
  shader_injection.xegtao_foliage_ao_blend = std::clamp(xegtao_foliage_ao_blend, 0.f, 1.f);
  shader_injection.xegtao_foliage_mask_method = std::clamp(std::round(xegtao_foliage_mask_method), 0.f, 2.f);

  auto* shader_state = renodx::utils::shader::GetCurrentState(cmd_list);
  const auto shader_hash = shader_state != nullptr
      ? renodx::utils::shader::GetCurrentPixelShaderHash(shader_state)
      : 0u;
  if (shader_hash != kLightingShader && shader_hash != kLightingSoftShader) return true;
  const bool is_main_lighting_draw = shader_hash == kLightingShader;
  const float resolved_xegtao_debug_mode = std::clamp(std::round(xegtao_debug_mode), 0.f, 21.f);
  shader_injection.xegtao_debug_mode = 0.f;

  auto* device = cmd_list->get_device();
  if (device != nullptr && IsIsFastSupportedDevice(device)) {
    auto* data = device->get_private_data<DeviceData>();
    const auto mode = GetXeGTAOModeSetting();
    if (data != nullptr && mode != XeGTAOMode::kOff) {
        const uint32_t fix_mode = ClampXeGTAOFixMode();
        const bool fix_l1_split =
          fix_mode >= static_cast<uint32_t>(XeGTAOFixMode::kProducerConsumerSplit);
        const bool fix_l2_isolation =
          fix_mode >= static_cast<uint32_t>(XeGTAOFixMode::kDispatchIsolationRestore);
        const bool fix_l3_owner =
          fix_mode >= static_cast<uint32_t>(XeGTAOFixMode::kSingleOwnerDeterministic);
        const bool fix_l4_guard =
          fix_mode >= static_cast<uint32_t>(XeGTAOFixMode::kStrictSideEffectGuard);
        const bool probe_a_dispatch_no_t22 = ClampBooleanToggle(xegtao_probe_a_dispatch_no_t22) != 0u;
        const bool probe_b_t22_no_dispatch =
            !probe_a_dispatch_no_t22 && ClampBooleanToggle(xegtao_probe_b_t22_no_dispatch) != 0u;

      const uint64_t frame = data->present_frame_index;
      if (frame < data->xegtao_resize_guard_until_frame) {
        if (data->xegtao_resize_guard_log_frame != frame) {
          data->xegtao_resize_guard_log_frame = frame;
          AddonLog(
              reshade::log::level::info,
              std::format(
                  "XeGTAO resize guard active on frame {} (guard_until={})",
                  frame,
                  data->xegtao_resize_guard_until_frame));
        }
        return true;
      }
      if (data->xegtao_lighting_draw_counter_frame != frame) {
        data->xegtao_lighting_draw_counter_frame = frame;
        data->xegtao_lighting_draw_counter = 0u;
        data->xegtao_dispatch_isolation_active = false;
        data->xegtao_dispatch_restore_mismatch = false;
        data->xegtao_trace_frame = frame;
        data->xegtao_trace_dispatch_attempted = false;
        data->xegtao_trace_dispatch_succeeded = false;
        data->xegtao_trace_main_pass_executed = false;
        data->xegtao_trace_composite_pass_executed = false;
        data->xegtao_trace_t22_bind_executed = false;
        data->xegtao_trace_copyback_requested = false;
        data->xegtao_trace_copyback_succeeded = false;
        data->xegtao_trace_apply_gate_passed = false;
        data->xegtao_trace_probe_a_active = probe_a_dispatch_no_t22;
        data->xegtao_trace_probe_b_active = probe_b_t22_no_dispatch;
        data->xegtao_trace_owner_draw_ordinal = 0u;
        data->xegtao_trace_state_valid = false;
        data->xegtao_trace_diag_hash = 0u;
        data->xegtao_deferred_dispatch_pending = false;
        data->xegtao_deferred_dispatch_frame = kInvalidFrameIndex;
        data->xegtao_deferred_dispatch_executed = false;
        data->xegtao_deferred_gate_signature = 0u;
        data->xegtao_deferred_owner_shader_hash = 0u;
        data->xegtao_deferred_owner_draw_ordinal = 0u;
        data->xegtao_deferred_depth_srv = {};
        data->xegtao_deferred_ssao_srv = {};
        data->xegtao_deferred_mrt_normal_srv = {};
        data->xegtao_deferred_scene_cbv = {};
        data->xegtao_deferred_scene_cbv_valid = false;
        data->xegtao_deferred_scene_cbv_frame = kInvalidFrameIndex;
        data->xegtao_deferred_scene_cbv_source = XeGTAOSceneCbvSource::kNone;
        data->xegtao_deferred_resolved_scene_cbv_from_current_bindings = false;
        data->xegtao_volfog_seen_frame = kInvalidFrameIndex;
        if (fix_l1_split) {
          data->xegtao_consume_signature_valid =
              data->xegtao_result_signature_valid
              && data->xegtao_result_t3_resource_handle != 0u
              && data->xegtao_result_t4_resource_handle != 0u
              && IsViewAlive(device, data->composite_srv);
          data->xegtao_consume_signature_frame = data->xegtao_result_signature_frame;
          data->xegtao_consume_t3_resource_handle = data->xegtao_result_t3_resource_handle;
          data->xegtao_consume_t4_resource_handle = data->xegtao_result_t4_resource_handle;
          data->xegtao_consume_t3_width = data->xegtao_result_t3_width;
          data->xegtao_consume_t3_height = data->xegtao_result_t3_height;
          data->xegtao_consume_t4_width = data->xegtao_result_t4_width;
          data->xegtao_consume_t4_height = data->xegtao_result_t4_height;
          data->xegtao_consume_working_width = data->xegtao_result_working_width;
          data->xegtao_consume_working_height = data->xegtao_result_working_height;
          data->xegtao_consume_owner_valid = data->xegtao_owner_valid;
          data->xegtao_consume_owner_frame = data->xegtao_owner_frame;
          data->xegtao_consume_owner_shader_hash = data->xegtao_owner_shader_hash;
          data->xegtao_consume_owner_draw_ordinal = data->xegtao_owner_draw_ordinal;
          data->xegtao_consume_owner_gate_signature = data->xegtao_owner_gate_signature;
          data->xegtao_consume_owner_downscaled = data->xegtao_owner_downscaled;
        } else {
          data->xegtao_consume_signature_valid = false;
          data->xegtao_consume_signature_frame = kInvalidFrameIndex;
          data->xegtao_consume_t3_resource_handle = 0u;
          data->xegtao_consume_t4_resource_handle = 0u;
          data->xegtao_consume_t3_width = 0u;
          data->xegtao_consume_t3_height = 0u;
          data->xegtao_consume_t4_width = 0u;
          data->xegtao_consume_t4_height = 0u;
          data->xegtao_consume_working_width = 0u;
          data->xegtao_consume_working_height = 0u;
          data->xegtao_consume_owner_valid = false;
          data->xegtao_consume_owner_frame = kInvalidFrameIndex;
          data->xegtao_consume_owner_shader_hash = 0u;
          data->xegtao_consume_owner_draw_ordinal = 0u;
          data->xegtao_consume_owner_gate_signature = 0u;
          data->xegtao_consume_owner_downscaled = false;
        }
      }
      data->xegtao_lighting_draw_counter += 1u;
      const uint32_t lighting_draw_ordinal = data->xegtao_lighting_draw_counter;
      bool has_result_this_frame = data->last_gtao_frame == frame;
        bool has_result_for_apply = fix_l1_split
          ? data->xegtao_consume_signature_valid
          : has_result_this_frame;
      bool dispatched_xegtao_this_draw = false;
      uint64_t dispatched_gate_signature = 0u;

      if (!has_result_this_frame) {
        if (mode == XeGTAOMode::kLightingOverride) {
          std::string gate_reason;
          std::string gate_diag;
          uint64_t gate_signature = 0u;
          const bool gate_passed =
              EvaluateXeGTAODrawCandidate(cmd_list, data, &gate_reason, &gate_diag, &gate_signature);
          if (!gate_passed) {
            const uint64_t gate_diag_hash = HashCombineU64(HashString(gate_reason), HashString(gate_diag));
            if (!data->last_gate_state_valid || data->last_gate_passed || data->last_gate_diag_hash != gate_diag_hash) {
              std::ostringstream message;
              message << "XeGTAO draw-gate rejected on frame " << frame
                      << " (" << gate_reason << ", " << gate_diag
                      << ", " << FormatXeGTAOSceneCbvInfo(data) << ")";
              AddonLog(reshade::log::level::info, message.str().c_str());
            }
            data->last_gate_state_valid = true;
            data->last_gate_passed = false;
            data->last_gate_diag_hash = gate_diag_hash;
            data->xegtao_warmup_signature = 0u;
            data->xegtao_warmup_stable_count = 0u;
          } else {
            data->last_gate_state_valid = true;
            data->last_gate_passed = true;
            data->last_gate_diag_hash = gate_signature;

            bool warmup_ready = false;
            if (data->xegtao_warmup_signature != gate_signature) {
              data->xegtao_warmup_signature = gate_signature;
              data->xegtao_warmup_stable_count = 1u;
              if (data->last_warmup_enter_signature != gate_signature) {
                data->last_warmup_enter_signature = gate_signature;
                std::ostringstream message;
                message << "XeGTAO warmup started on frame " << frame
                        << " (stabilizing capture for 2 matching draws, " << gate_diag << ")";
                AddonLog(reshade::log::level::info, message.str().c_str());
              }
            } else {
              if (data->xegtao_warmup_stable_count < 2u) {
                data->xegtao_warmup_stable_count += 1u;
              }
              if (data->xegtao_warmup_stable_count >= 2u) {
                warmup_ready = true;
              }
            }

            if (warmup_ready && data->last_warmup_complete_signature != gate_signature) {
              data->last_warmup_complete_signature = gate_signature;
              std::ostringstream message;
              message << "XeGTAO warmup completed on frame " << frame
                      << " (" << gate_diag << ")";
              AddonLog(reshade::log::level::info, message.str().c_str());
            }

            if (warmup_ready) {
              if (fix_l3_owner && !is_main_lighting_draw) {
                // Level 3+: only main-lighting draw is allowed to produce the frame token.
              } else if (probe_b_t22_no_dispatch) {
                data->xegtao_trace_dispatch_attempted = false;
              } else if (fix_l2_isolation) {
                if (!data->xegtao_deferred_dispatch_pending
                    || data->xegtao_deferred_dispatch_frame != frame) {
                  if ((!data->captured_scene_cbv_valid
                       || data->captured_scene_cbv_frame != frame
                       || data->captured_scene_cbv.buffer.handle == 0u
                       || !IsSceneCbvCandidateValid(device, data->captured_scene_cbv))
                      && data->fallback_scene_cbv_seen) {
                    // Deferred dispatch snapshots should self-bootstrap from fallback scene CBV,
                    // so fix2+ does not require an off/fix1 toggle to start producing AO.
                    (void)TryAdoptFallbackSceneCbv(device, data);
                  }
                  data->xegtao_deferred_dispatch_pending = true;
                  data->xegtao_deferred_dispatch_frame = frame;
                  data->xegtao_deferred_dispatch_executed = false;
                  data->xegtao_deferred_gate_signature = gate_signature;
                  data->xegtao_deferred_owner_shader_hash = shader_hash;
                  data->xegtao_deferred_owner_draw_ordinal = lighting_draw_ordinal;
                  data->xegtao_deferred_depth_srv = data->captured_depth_srv;
                  data->xegtao_deferred_ssao_srv = data->captured_ssao_srv;
                  data->xegtao_deferred_mrt_normal_srv = data->captured_mrt_normal_srv;
                  data->xegtao_deferred_scene_cbv = data->captured_scene_cbv;
                  data->xegtao_deferred_scene_cbv_valid =
                      data->captured_scene_cbv_valid
                      && data->captured_scene_cbv_frame == frame
                      && data->captured_scene_cbv.buffer.handle != 0u
                      && IsSceneCbvCandidateValid(device, data->captured_scene_cbv);
                  data->xegtao_deferred_scene_cbv_frame = data->captured_scene_cbv_frame;
                  data->xegtao_deferred_scene_cbv_source = data->captured_scene_cbv_source;
                  data->xegtao_deferred_resolved_scene_cbv_from_current_bindings =
                      data->resolved_scene_cbv_from_current_bindings;
                }
                data->xegtao_trace_dispatch_attempted = false;
                data->xegtao_trace_dispatch_succeeded = false;
              } else {
                const bool request_copy_back = false;
                data->xegtao_trace_dispatch_attempted = true;
                data->xegtao_dispatch_isolation_active = fix_l2_isolation;

                ScopedDescriptorCaptureSkip dispatch_capture_guard(fix_l4_guard);
                has_result_this_frame = RunXeGTAOForFrame(
                  cmd_list,
                  data,
                  request_copy_back,
                  true);
                data->xegtao_trace_dispatch_succeeded = has_result_this_frame;

                data->xegtao_dispatch_isolation_active = false;
                data->xegtao_dispatch_restore_mismatch = false;

                if (has_result_this_frame) {
                  dispatched_xegtao_this_draw = true;
                  dispatched_gate_signature = gate_signature;
                }
              }
            }
          }
        }
      }

      if (dispatched_xegtao_this_draw) {
        data->xegtao_owner_valid =
            data->xegtao_result_signature_valid
            && data->xegtao_result_signature_frame == frame;
        data->xegtao_owner_frame = frame;
        data->xegtao_owner_shader_hash = shader_hash;
        data->xegtao_owner_draw_ordinal = lighting_draw_ordinal;
        data->xegtao_trace_owner_draw_ordinal = lighting_draw_ordinal;
        data->xegtao_owner_gate_signature = dispatched_gate_signature;
        data->xegtao_owner_downscaled = data->xegtao_owner_valid && IsXeGTAOResultDownscaled(data);

        uint64_t owner_diag_hash = 0u;
        owner_diag_hash = HashCombineU64(owner_diag_hash, data->xegtao_owner_valid ? 1u : 0u);
        owner_diag_hash = HashCombineU64(owner_diag_hash, data->xegtao_owner_shader_hash);
        owner_diag_hash = HashCombineU64(owner_diag_hash, data->xegtao_owner_draw_ordinal);
        owner_diag_hash = HashCombineU64(owner_diag_hash, data->xegtao_owner_gate_signature);
        owner_diag_hash = HashCombineU64(owner_diag_hash, data->xegtao_owner_downscaled ? 1u : 0u);
        if (!data->last_owner_state_valid || data->last_owner_diag_hash != owner_diag_hash) {
          std::ostringstream message;
          message << "XeGTAO owner captured on frame " << frame
                  << " (" << FormatXeGTAOOwnerInfo(data) << ")";
          AddonLog(reshade::log::level::info, message.str().c_str());
        }
        data->last_owner_state_valid = true;
        data->last_owner_diag_hash = owner_diag_hash;
      }

      if (!fix_l1_split) {
        has_result_for_apply = has_result_this_frame;
      }

      bool should_force_neutral_x = false;
      bool apply_gate_passed = false;
      std::string apply_gate_reason;
      std::string apply_gate_diag;
      std::string apply_gate_source = "none";
      XeGTAOCapturedViewInfo apply_t3_info = {};
      XeGTAOCapturedViewInfo apply_t4_info = {};
      uint64_t apply_draw_signature = 0u;
      bool apply_draw_signature_valid = false;
      std::string apply_draw_signature_diag;
        const bool apply_signature_valid = fix_l1_split
          ? data->xegtao_consume_signature_valid
          : data->xegtao_result_signature_valid;
        const uint64_t apply_signature_frame = fix_l1_split
          ? data->xegtao_consume_signature_frame
          : data->xegtao_result_signature_frame;
        const uint64_t apply_t3_resource_handle = fix_l1_split
          ? data->xegtao_consume_t3_resource_handle
          : data->xegtao_result_t3_resource_handle;
        const uint64_t apply_t4_resource_handle = fix_l1_split
          ? data->xegtao_consume_t4_resource_handle
          : data->xegtao_result_t4_resource_handle;
        const uint32_t apply_t3_width = fix_l1_split
          ? data->xegtao_consume_t3_width
          : data->xegtao_result_t3_width;
        const uint32_t apply_t3_height = fix_l1_split
          ? data->xegtao_consume_t3_height
          : data->xegtao_result_t3_height;
        const uint32_t apply_t4_width = fix_l1_split
          ? data->xegtao_consume_t4_width
          : data->xegtao_result_t4_width;
        const uint32_t apply_t4_height = fix_l1_split
          ? data->xegtao_consume_t4_height
          : data->xegtao_result_t4_height;
        const uint32_t apply_working_width = fix_l1_split
          ? data->xegtao_consume_working_width
          : data->xegtao_result_working_width;
        const uint32_t apply_working_height = fix_l1_split
          ? data->xegtao_consume_working_height
          : data->xegtao_result_working_height;
        const bool copyback_requested = has_result_for_apply
          && data->xegtao_copyback_frame == frame
          && data->xegtao_copyback_requested_for_frame;
      const bool copyback_active_for_apply = copyback_requested
          && data->xegtao_copyback_succeeded_for_frame
          && data->xegtao_copyback_active_for_apply;
      const bool copyback_failed_for_apply = copyback_requested && !copyback_active_for_apply;
      data->xegtao_trace_copyback_requested = copyback_requested;
      data->xegtao_trace_copyback_succeeded = copyback_active_for_apply;
      const bool downscaled_result_for_apply =
          has_result_for_apply
          && apply_signature_valid
          && apply_working_width > 0u
          && apply_working_height > 0u
          && apply_t3_width > 0u
          && apply_t3_height > 0u
          && (apply_working_width < apply_t3_width || apply_working_height < apply_t3_height);
        const bool owner_token_valid = fix_l1_split
          ? data->xegtao_consume_owner_valid
          : data->xegtao_owner_valid;
        const uint64_t owner_token_frame = fix_l1_split
          ? data->xegtao_consume_owner_frame
          : data->xegtao_owner_frame;
        const uint32_t owner_token_shader_hash = fix_l1_split
          ? data->xegtao_consume_owner_shader_hash
          : data->xegtao_owner_shader_hash;
        const uint32_t owner_token_draw_ordinal = fix_l1_split
          ? data->xegtao_consume_owner_draw_ordinal
          : data->xegtao_owner_draw_ordinal;
        const uint64_t owner_token_gate_signature = fix_l1_split
          ? data->xegtao_consume_owner_gate_signature
          : data->xegtao_owner_gate_signature;
        const bool owner_token_downscaled = fix_l1_split
          ? data->xegtao_consume_owner_downscaled
          : data->xegtao_owner_downscaled;
        const bool owner_token_frame_ready = fix_l1_split
          ? (owner_token_frame != kInvalidFrameIndex && owner_token_frame == apply_signature_frame)
          : owner_token_frame == frame;
      const bool owner_ready_for_draw =
          owner_token_valid
          && owner_token_frame_ready
          && shader_hash == owner_token_shader_hash
          && lighting_draw_ordinal == owner_token_draw_ordinal;
      // Owner-only debug policy: only the owning lighting draw emits XeGTAO debug.
      shader_injection.xegtao_debug_mode = owner_ready_for_draw ? resolved_xegtao_debug_mode : 0.f;
      const bool downscaled_owner_token_available =
          owner_token_valid
          && owner_token_frame_ready
          && owner_token_downscaled;
      const bool mrt_normal_valid_for_apply =
          data->xegtao_mrt_normal_valid
          && apply_signature_valid
          && data->xegtao_mrt_normal_frame != kInvalidFrameIndex
          && data->xegtao_mrt_normal_frame == apply_signature_frame;

      auto bind_xegtao_srv = [&](reshade::api::resource_view srv_to_bind) -> bool {
        if (probe_a_dispatch_no_t22) return false;
        if (!IsViewAlive(device, srv_to_bind)) return false;
        shader_injection.xegtao_dedicated_bound = 1.f;
        if (mrt_normal_valid_for_apply) {
          shader_injection.xegtao_mrt_normal_valid = 1.f;
        }
        ScopedDescriptorCaptureSkip capture_skip_guard(true);
        cmd_list->push_descriptors(
            reshade::api::shader_stage::pixel,
            reshade::api::pipeline_layout{0},
            0,
            reshade::api::descriptor_table_update{
                {},
                kLightingXeGtaoRegister,
                0,
                1,
                reshade::api::descriptor_type::texture_shader_resource_view,
                &srv_to_bind,
            });
        data->xegtao_trace_t22_bind_executed = true;
        return true;
      };

      if (probe_b_t22_no_dispatch) {
        apply_gate_source = "probe_b_t22_no_dispatch";
        apply_gate_reason = "probe B forced t22 bind without new XeGTAO dispatch";
        reshade::api::resource_view probe_srv = {};
        if (IsViewAlive(device, data->captured_ssao_srv)) {
          probe_srv = data->captured_ssao_srv;
        } else if (IsViewAlive(device, data->composite_srv)) {
          probe_srv = data->composite_srv;
        }
        if (probe_srv.handle != 0u) {
          (void)bind_xegtao_srv(probe_srv);
        }
        should_force_neutral_x = true;
        shader_injection.xegtao_debug_mode = 0.f;
      } else if (copyback_active_for_apply) {
        if (downscaled_result_for_apply) {
          if (owner_ready_for_draw) {
            apply_gate_passed = true;
            apply_gate_source = "copyback_active_owner";
            apply_gate_reason = "copyback-active owner draw";
            if (shader_injection.xegtao_debug_mode > 0.f && IsViewAlive(device, data->composite_srv)) {
              (void)bind_xegtao_srv(data->composite_srv);
            }
          } else {
            apply_gate_passed = false;
            apply_gate_source = "copyback_active_non_owner";
            apply_gate_reason = downscaled_owner_token_available
                ? "downscaled non-owner draw rejected"
                : "downscaled owner token is unavailable";
            should_force_neutral_x = true;
            shader_injection.xegtao_debug_mode = 0.f;
            shader_injection.xegtao_debug_blackout = 1.f;
          }
        } else {
          apply_gate_passed = true;
          apply_gate_source = "copyback_active_t4";
          apply_gate_reason = "copyback-active path";

          if (shader_injection.xegtao_debug_mode > 0.f && shader_injection.xegtao_debug_mode <= 9.f) {
            if (owner_ready_for_draw && IsViewAlive(device, data->composite_srv)) {
              (void)bind_xegtao_srv(data->composite_srv);
            } else {
              shader_injection.xegtao_debug_blackout = 1.f;
            }
          }
        }
      } else {
        const bool should_try_apply = has_result_for_apply && IsViewAlive(device, data->composite_srv);
        if (should_try_apply) {
          reshade::api::resource_view current_t3 = {};
          reshade::api::resource_view current_t4 = {};
          const bool has_current_views =
              ResolveCurrentLightingTextureViewsForDraw(cmd_list, shader_hash, &current_t3, &current_t4);
          const auto current_t3_info = GetXeGTAOCapturedViewInfo(device, current_t3);
          const auto current_t4_info = GetXeGTAOCapturedViewInfo(device, current_t4);
          const auto tracked_t3_info = GetXeGTAOCapturedViewInfo(device, data->captured_depth_srv);
          const auto tracked_t4_info = GetXeGTAOCapturedViewInfo(device, data->captured_ssao_srv);
          const bool signature_ready =
              apply_signature_valid
              && apply_t3_resource_handle != 0u
              && apply_t4_resource_handle != 0u
              && (fix_l1_split
                ? (apply_signature_frame != kInvalidFrameIndex && apply_signature_frame < frame)
                : apply_signature_frame == frame);

          if (!signature_ready) {
            apply_gate_reason = "XeGTAO result signature unavailable for this frame";
          } else {
            if (has_current_views) {
              apply_gate_source = "current_draw";
              if (!current_t3_info.alive || current_t3_info.resource.handle == 0u
                  || current_t3_info.resource_desc.type != reshade::api::resource_type::texture_2d) {
                apply_gate_reason = "current draw t3 is invalid";
              } else if (!current_t4_info.alive || current_t4_info.resource.handle == 0u
                         || current_t4_info.resource_desc.type != reshade::api::resource_type::texture_2d) {
                apply_gate_reason = "current draw t4 is invalid";
              } else if (current_t3_info.resource.handle != apply_t3_resource_handle
                         || current_t4_info.resource.handle != apply_t4_resource_handle
                         || current_t3_info.width != apply_t3_width
                         || current_t3_info.height != apply_t3_height
                         || current_t4_info.width != apply_t4_width
                         || current_t4_info.height != apply_t4_height) {
                apply_gate_reason = "current draw t3/t4 does not match XeGTAO source signature";
              } else {
                apply_gate_passed = true;
                apply_t3_info = current_t3_info;
                apply_t4_info = current_t4_info;
              }
            } else {
              apply_gate_source = "tracked_fallback";
              const uint64_t fallback_activation_frame = GetXeGTAOFallbackActivationFrame(data);
              if (frame < fallback_activation_frame) {
                apply_gate_reason = std::format(
                    "tracked fallback apply blocked (frame {} < unlock {})",
                    frame,
                    fallback_activation_frame);
              } else if (!tracked_t3_info.alive || tracked_t3_info.resource.handle == 0u
                  || tracked_t3_info.resource_desc.type != reshade::api::resource_type::texture_2d) {
                apply_gate_reason = "tracked fallback t3 is invalid";
              } else if (!tracked_t4_info.alive || tracked_t4_info.resource.handle == 0u
                         || tracked_t4_info.resource_desc.type != reshade::api::resource_type::texture_2d) {
                apply_gate_reason = "tracked fallback t4 is invalid";
              } else if (tracked_t3_info.resource.handle != apply_t3_resource_handle
                         || tracked_t4_info.resource.handle != apply_t4_resource_handle
                         || tracked_t3_info.width != apply_t3_width
                         || tracked_t3_info.height != apply_t3_height
                         || tracked_t4_info.width != apply_t4_width
                         || tracked_t4_info.height != apply_t4_height) {
                apply_gate_reason = "tracked fallback t3/t4 does not match XeGTAO source signature";
              } else {
                apply_gate_passed = true;
                apply_t3_info = tracked_t3_info;
                apply_t4_info = tracked_t4_info;
              }
            }
          }

          if (apply_gate_passed) {
            apply_draw_signature_valid = TryBuildXeGTAODrawSignatureForState(
                cmd_list,
                device,
                apply_t3_info,
                apply_t4_info,
                &apply_draw_signature,
                &apply_draw_signature_diag);
            const bool result_is_downscaled = signature_ready && downscaled_result_for_apply;
            if (result_is_downscaled) {
              const bool owner_ready =
                  owner_token_valid
                  && owner_token_frame_ready
                  && owner_token_downscaled;
              if (!owner_ready) {
                apply_gate_passed = false;
                apply_gate_reason = "downscaled owner token is unavailable";
              } else {
                const bool owner_match =
                    shader_hash == owner_token_shader_hash
                    && lighting_draw_ordinal == owner_token_draw_ordinal
                    && apply_draw_signature_valid
                    && apply_draw_signature == owner_token_gate_signature;
                if (!owner_match) {
                  apply_gate_passed = false;
                  apply_gate_reason = "downscaled non-owner draw rejected";
                }
              }
            }

            if (apply_gate_passed && fix_l3_owner) {
              const bool owner_match =
                  owner_token_valid
                  && owner_token_frame_ready
                  && shader_hash == owner_token_shader_hash
                  && lighting_draw_ordinal == owner_token_draw_ordinal;
              if (!owner_match) {
                apply_gate_passed = false;
                apply_gate_reason = "non-owner draw rejected by deterministic owner policy";
              }
            }
          }

          std::ostringstream diag_stream;
          diag_stream
              << "shader=" << (is_main_lighting_draw ? "main" : "soft")
              << ", draw_ordinal=" << lighting_draw_ordinal
              << ", apply_source=" << apply_gate_source
              << ", current_t3=" << FormatXeGTAOCapturedViewInfo("t3", current_t3_info)
              << ", current_t4=" << FormatXeGTAOCapturedViewInfo("t4", current_t4_info)
              << ", tracked_t3=" << FormatXeGTAOCapturedViewInfo("t3", tracked_t3_info)
              << ", tracked_t4=" << FormatXeGTAOCapturedViewInfo("t4", tracked_t4_info)
              << ", apply_sig=";
          if (apply_draw_signature_valid) {
            diag_stream << "0x" << std::hex << apply_draw_signature << std::dec;
          } else {
            diag_stream << "invalid(" << apply_draw_signature_diag << ")";
          }
          diag_stream
              << ", " << FormatXeGTAOOwnerInfo(data)
              << ", " << FormatXeGTAOResultSignatureInfo(data);
          apply_gate_diag = diag_stream.str();
        } else {
          apply_gate_reason = has_result_for_apply
              ? "XeGTAO composite SRV is not alive"
              : "XeGTAO result unavailable this frame";
          std::ostringstream diag_stream;
          diag_stream
              << "shader=" << (is_main_lighting_draw ? "main" : "soft")
              << ", draw_ordinal=" << lighting_draw_ordinal
              << ", apply_source=" << apply_gate_source
              << ", " << FormatXeGTAOOwnerInfo(data)
              << ", " << FormatXeGTAOResultSignatureInfo(data);
          apply_gate_diag = diag_stream.str();
        }

        uint64_t apply_gate_diag_hash = 1469598103934665603ull;
        apply_gate_diag_hash = HashCombineU64(apply_gate_diag_hash, apply_gate_passed ? 1u : 0u);
        apply_gate_diag_hash = HashCombineU64(apply_gate_diag_hash, HashString(apply_gate_reason));
        apply_gate_diag_hash = HashCombineU64(apply_gate_diag_hash, HashString(apply_gate_source));
        apply_gate_diag_hash = HashCombineU64(
            apply_gate_diag_hash, apply_draw_signature_valid ? apply_draw_signature : 0u);
        bool* apply_gate_state_valid = is_main_lighting_draw
            ? &data->last_apply_gate_main_state_valid
            : &data->last_apply_gate_soft_state_valid;
        bool* apply_gate_last_passed = is_main_lighting_draw
            ? &data->last_apply_gate_main_passed
            : &data->last_apply_gate_soft_passed;
        uint64_t* apply_gate_last_hash = is_main_lighting_draw
            ? &data->last_apply_gate_main_diag_hash
            : &data->last_apply_gate_soft_diag_hash;
        if (apply_gate_state_valid != nullptr && apply_gate_last_passed != nullptr && apply_gate_last_hash != nullptr) {
          const bool expected_non_owner_reject = !apply_gate_passed
              && (apply_gate_reason == "downscaled non-owner draw rejected"
                  || apply_gate_reason == "non-owner draw rejected by deterministic owner policy");
          if (!expected_non_owner_reject) {
            if (!*apply_gate_state_valid
                || *apply_gate_last_passed != apply_gate_passed
                || *apply_gate_last_hash != apply_gate_diag_hash) {
              std::ostringstream message;
              message << "XeGTAO apply-gate " << (apply_gate_passed ? "accepted" : "rejected")
                      << " on frame " << frame;
              if (apply_gate_passed) {
                message << " (signature match via " << apply_gate_source << ", " << apply_gate_diag << ")";
              } else {
                message << " (" << apply_gate_reason << ", " << apply_gate_diag << ")";
              }
              AddonLog(reshade::log::level::info, message.str().c_str());
            }
            *apply_gate_state_valid = true;
            *apply_gate_last_passed = apply_gate_passed;
            *apply_gate_last_hash = apply_gate_diag_hash;
          }
        }

        if (apply_gate_passed && IsViewAlive(device, data->composite_srv)) {
          (void)bind_xegtao_srv(data->composite_srv);
        } else {
          should_force_neutral_x = true;
          shader_injection.xegtao_debug_mode = 0.f;
        }
      }

      if (probe_a_dispatch_no_t22) {
        apply_gate_passed = false;
        apply_gate_source = "probe_a_dispatch_no_t22";
        apply_gate_reason = "probe A suppresses t22 binding";
        should_force_neutral_x = true;
        shader_injection.xegtao_debug_mode = 0.f;
      }

      std::string apply_path = "strict_t22";
      if (probe_a_dispatch_no_t22) {
        apply_path = "probe_a_dispatch_no_t22";
      } else if (probe_b_t22_no_dispatch) {
        apply_path = "probe_b_t22_no_dispatch";
      } else if (copyback_active_for_apply) {
        if (downscaled_result_for_apply) {
          apply_path = owner_ready_for_draw ? "copyback_active_owner" : "non_owner_neutral";
        } else {
          apply_path = "copyback_active_t4";
        }
      } else if (copyback_failed_for_apply) {
        apply_path = "copyback_failed_fallback_t22";
      } else if (!apply_gate_passed && should_force_neutral_x) {
        apply_path = "non_owner_neutral";
      }
      uint64_t apply_path_hash = 1469598103934665603ull;
      apply_path_hash = HashCombineU64(apply_path_hash, HashString(apply_path));
      apply_path_hash = HashCombineU64(apply_path_hash, is_main_lighting_draw ? 1u : 2u);
      bool* apply_path_state_valid = is_main_lighting_draw
          ? &data->last_apply_path_main_state_valid
          : &data->last_apply_path_soft_state_valid;
      uint64_t* last_apply_path_hash = is_main_lighting_draw
          ? &data->last_apply_path_main_hash
          : &data->last_apply_path_soft_hash;
      if (apply_path_state_valid != nullptr && last_apply_path_hash != nullptr) {
        if (!*apply_path_state_valid || *last_apply_path_hash != apply_path_hash) {
          std::ostringstream message;
          message << "XeGTAO apply-path switched on frame " << frame
                  << " (path=" << apply_path
                  << ", mode=" << GetXeGTAOFixModeName(ClampXeGTAOFixMode())
                  << ", shader=" << (is_main_lighting_draw ? "main" : "soft")
                  << ", draw_ordinal=" << lighting_draw_ordinal << ")";
          AddonLog(reshade::log::level::info, message.str().c_str());
        }
        *apply_path_state_valid = true;
        *last_apply_path_hash = apply_path_hash;
      }

      data->xegtao_trace_apply_gate_passed = apply_gate_passed;
      data->xegtao_trace_probe_a_active = probe_a_dispatch_no_t22;
      data->xegtao_trace_probe_b_active = probe_b_t22_no_dispatch;
      uint64_t trace_hash = 1469598103934665603ull;
      trace_hash = HashCombineU64(trace_hash, data->xegtao_trace_dispatch_attempted ? 1u : 0u);
      trace_hash = HashCombineU64(trace_hash, data->xegtao_trace_dispatch_succeeded ? 1u : 0u);
      trace_hash = HashCombineU64(trace_hash, data->xegtao_trace_main_pass_executed ? 1u : 0u);
      trace_hash = HashCombineU64(trace_hash, data->xegtao_trace_composite_pass_executed ? 1u : 0u);
      trace_hash = HashCombineU64(trace_hash, data->xegtao_trace_t22_bind_executed ? 1u : 0u);
      trace_hash = HashCombineU64(trace_hash, data->xegtao_trace_copyback_requested ? 1u : 0u);
      trace_hash = HashCombineU64(trace_hash, data->xegtao_trace_copyback_succeeded ? 1u : 0u);
      trace_hash = HashCombineU64(trace_hash, data->xegtao_trace_apply_gate_passed ? 1u : 0u);
      trace_hash = HashCombineU64(trace_hash, data->xegtao_trace_probe_a_active ? 1u : 0u);
      trace_hash = HashCombineU64(trace_hash, data->xegtao_trace_probe_b_active ? 1u : 0u);
      trace_hash = HashCombineU64(trace_hash, data->xegtao_trace_owner_draw_ordinal);
      trace_hash = HashCombineU64(trace_hash, HashString(apply_path));
      if (!data->xegtao_trace_state_valid || data->xegtao_trace_diag_hash != trace_hash) {
        std::ostringstream trace_message;
        trace_message << "XeGTAO trace frame " << frame
                      << " (dispatch_attempted=" << (data->xegtao_trace_dispatch_attempted ? 1 : 0)
                      << ", dispatch_succeeded=" << (data->xegtao_trace_dispatch_succeeded ? 1 : 0)
                      << ", main_pass=" << (data->xegtao_trace_main_pass_executed ? 1 : 0)
                      << ", composite_pass=" << (data->xegtao_trace_composite_pass_executed ? 1 : 0)
                      << ", t22_bind=" << (data->xegtao_trace_t22_bind_executed ? 1 : 0)
                      << ", copyback_req=" << (data->xegtao_trace_copyback_requested ? 1 : 0)
                      << ", copyback_ok=" << (data->xegtao_trace_copyback_succeeded ? 1 : 0)
                      << ", apply_gate=" << (data->xegtao_trace_apply_gate_passed ? 1 : 0)
                      << ", probe_a=" << (data->xegtao_trace_probe_a_active ? 1 : 0)
                      << ", probe_b=" << (data->xegtao_trace_probe_b_active ? 1 : 0)
                      << ", deferred_mode=" << (fix_l2_isolation ? 1 : 0)
                      << ", owner_draw=" << data->xegtao_trace_owner_draw_ordinal
                      << ", apply_path=" << apply_path << ")";
        AddonLog(reshade::log::level::info, trace_message.str().c_str());
      }
      data->xegtao_trace_state_valid = true;
      data->xegtao_trace_diag_hash = trace_hash;

      shader_injection.xegtao_force_neutral_x = should_force_neutral_x ? 1.f : 0.f;
      const bool xegtao_ao_active_for_draw =
          !should_force_neutral_x
          && (shader_injection.xegtao_dedicated_bound >= 0.5f || copyback_active_for_apply);
      shader_injection.xegtao_ao_active_for_draw = xegtao_ao_active_for_draw ? 1.f : 0.f;
    }
  }

  // Bind character shader's MRT texture to t10 so the lighting shader can read the
  // character mask from it (the lighting shader's own t1 may not contain the bits).
  if (shader_injection.char_gi_enabled < 0.5f) return true;

  reshade::api::resource_view char_mrt0 = {g_character_mrt0_view.load(std::memory_order_relaxed)};
  if (char_mrt0.handle == 0u) {
    char_mrt0 = {g_lighting_mrt0_view.load(std::memory_order_relaxed)};
  }
  if (!IsViewAlive(device, char_mrt0)) return true;

    {
    ScopedDescriptorCaptureSkip capture_skip_guard(true);
    cmd_list->push_descriptors(
      reshade::api::shader_stage::pixel,
      reshade::api::pipeline_layout{0},
      0,
      reshade::api::descriptor_table_update{
        {},
        10,
        0,
        1,
        reshade::api::descriptor_type::texture_shader_resource_view,
        &char_mrt0,
      });
    }
  return true;
}

bool BindISFastNoiseTexturePixel(reshade::api::command_list* cmd_list, uint32_t texture_binding) {
  if (cmd_list == nullptr) return true;
  if (!IsIsFastSupportedDevice(cmd_list->get_device())) {
    shader_injection.isfast_noise_bound = 0.f;
    return true;
  }
  if (g_isfast_reshade_srv.handle == 0u) {
    shader_injection.isfast_noise_bound = 0.f;
    if (!g_isfast_bind_failed_logged) {
      LogIsFast(reshade::log::level::warning, "BindISFASTNoisePixel match=NO (SRV unavailable).");
      g_isfast_bind_failed_logged = true;
    }
    return true;
  }

  shader_injection.isfast_noise_bound = 1.f;

  cmd_list->push_descriptors(
      reshade::api::shader_stage::pixel,
      reshade::api::pipeline_layout{0},
      0,
      reshade::api::descriptor_table_update{
          {},
          texture_binding,
          0,
          1,
          reshade::api::descriptor_type::texture_shader_resource_view,
          &g_isfast_reshade_srv,
      });

  return true;
}

bool BindISFastNoisePixel(reshade::api::command_list* cmd_list) {
  if (cmd_list == nullptr) return true;
  if (!IsIsFastSupportedDevice(cmd_list->get_device())) {
    shader_injection.isfast_noise_bound = 0.f;
    return true;
  }
  if (g_isfast_reshade_sampler.handle == 0u) {
    shader_injection.isfast_noise_bound = 0.f;
    if (!g_isfast_bind_failed_logged) {
      LogIsFast(reshade::log::level::warning, "BindISFASTNoisePixel match=NO (sampler unavailable).");
      g_isfast_bind_failed_logged = true;
    }
    return true;
  }

  if (!BindISFastNoiseTexturePixel(cmd_list, kIsFastTextureBinding)) return false;

  cmd_list->push_descriptors(
      reshade::api::shader_stage::pixel,
      reshade::api::pipeline_layout{0},
      0,
      reshade::api::descriptor_table_update{
          {},
          kIsFastSamplerBinding,
          0,
          1,
          reshade::api::descriptor_type::sampler,
          &g_isfast_reshade_sampler,
      });

  if (!g_isfast_bind_logged) {
    if (shader_injection.isfast_noise_bound > 0.5f) {
      LogIsFast(
          reshade::log::level::debug,
          g_isfast_using_debug_texture
              ? "BindISFASTNoisePixel match=YES (debug texture bound)"
              : "BindISFASTNoisePixel match=YES");
    } else {
      LogIsFast(
          reshade::log::level::debug,
          g_isfast_using_debug_texture
              ? "BindISFASTNoisePixel match=YES (debug texture bound, sampling disabled)"
              : "BindISFASTNoisePixel match=YES (sampling disabled)");
    }
    if (!g_isfast_compute_bind_logged) {
      LogIsFast(reshade::log::level::debug, "BindISFASTNoiseCompute match=YES (N/A, no compute target in this addon).");
      g_isfast_compute_bind_logged = true;
    }
    g_isfast_bind_logged = true;
  }

  return true;
}

bool OnBeforeWireFenceShaderDraw(reshade::api::command_list* cmd_list) {
  if (cmd_list == nullptr) return true;
  if (shader_injection.wire_alpha_mode < 1.5f) return true;
  if (!IsIsFastSupportedDevice(cmd_list->get_device())) {
    shader_injection.isfast_noise_bound = 0.f;
    return true;
  }
  if (g_isfast_reshade_srv.handle == 0u) {
    shader_injection.isfast_noise_bound = 0.f;
    return true;
  }
  shader_injection.isfast_noise_bound = 1.f;
  return BindISFastNoiseTexturePixel(cmd_list, kWireIsFastTextureBinding);
}

void OnPresentAdvanceFrame(
    reshade::api::command_queue* queue,
    reshade::api::swapchain* swapchain,
    const reshade::api::rect* source_rect,
    const reshade::api::rect* dest_rect,
    uint32_t dirty_rect_count,
    const reshade::api::rect* dirty_rects) {
  auto* device = queue != nullptr
      ? queue->get_device()
      : (swapchain != nullptr ? swapchain->get_device() : nullptr);
  if (device != nullptr && IsIsFastSupportedDevice(device)) {
    const bool desired_fast_texture = isfast_texture_source_enabled >= 0.5f;
    if (desired_fast_texture != g_isfast_mode_use_fast_texture) {
      ReloadIsFastResources(device, "settings_change");
    }
    auto* data = device->get_private_data<DeviceData>();
    if (data != nullptr) {
      if (!g_xegtao_startup_mode_logged.exchange(true, std::memory_order_relaxed)) {
        AddonLog(
            reshade::log::level::info,
            std::format(
            "XeGTAO startup mode: fix={}, l5(prefilter={}, main={}, denoise={}, composite={}), probeA={}, probeB={}, fallbacks={}",
                GetXeGTAOFixModeName(ClampXeGTAOFixMode()),
                ClampBooleanToggle(xegtao_fix_l5_prefilter),
                ClampBooleanToggle(xegtao_fix_l5_main),
                ClampBooleanToggle(xegtao_fix_l5_denoise),
                ClampBooleanToggle(xegtao_fix_l5_composite),
                ClampBooleanToggle(xegtao_probe_a_dispatch_no_t22),
                ClampBooleanToggle(xegtao_probe_b_t22_no_dispatch),
                ClampBooleanToggle(xegtao_enable_fallbacks)));
      }
      const uint64_t frame = data->present_frame_index;
      if (data->xegtao_deferred_dispatch_pending
          && data->xegtao_deferred_dispatch_frame == frame
          && !data->xegtao_deferred_dispatch_executed) {
        const uint32_t fix_mode = ClampXeGTAOFixMode();
        const bool use_deferred_dispatch =
            fix_mode >= static_cast<uint32_t>(XeGTAOFixMode::kDispatchIsolationRestore);
        const bool probe_a_dispatch_no_t22 = ClampBooleanToggle(xegtao_probe_a_dispatch_no_t22) != 0u;
        const bool probe_b_t22_no_dispatch =
            !probe_a_dispatch_no_t22 && ClampBooleanToggle(xegtao_probe_b_t22_no_dispatch) != 0u;

        if (!use_deferred_dispatch || probe_b_t22_no_dispatch) {
          data->xegtao_deferred_dispatch_pending = false;
          data->xegtao_deferred_dispatch_executed = true;
        } else if (frame < kXeGTAODeferredStartupGuardFrames) {
          if (data->xegtao_deferred_drop_log_frame != frame) {
            data->xegtao_deferred_drop_log_frame = frame;
            AddonLog(
                reshade::log::level::info,
                std::format(
                    "XeGTAO deferred startup guard: skipping dispatch on frame {}",
                    frame));
          }
          data->xegtao_debug_deferred_drop_count += 1u;
          data->xegtao_debug_last_deferred_drop_frame = frame;
          data->xegtao_deferred_dispatch_pending = false;
          data->xegtao_deferred_dispatch_executed = true;
        } else {
          auto* cmd_list = queue != nullptr
              ? queue->get_immediate_command_list()
              : nullptr;

          if (cmd_list == nullptr) {
            if (data->xegtao_deferred_drop_log_frame != frame) {
              data->xegtao_deferred_drop_log_frame = frame;
              AddonLog(
                  reshade::log::level::warning,
                  "XeGTAO deferred dispatch dropped: present queue/immediate command list unavailable at end-of-frame");
            }
            data->xegtao_debug_deferred_drop_count += 1u;
            data->xegtao_debug_last_deferred_drop_frame = frame;
            data->xegtao_deferred_dispatch_pending = false;
            data->xegtao_deferred_dispatch_executed = true;
          } else {
            const reshade::api::resource_view fallback_lighting_mrt0 = {
                g_lighting_mrt0_view.load(std::memory_order_relaxed)};
            reshade::api::resource_view deferred_mrt_normal_srv =
                data->xegtao_deferred_mrt_normal_srv;
            bool deferred_mrt_normal_valid = deferred_mrt_normal_srv.handle != 0u;
            if (!deferred_mrt_normal_valid
                && AreXeGTAOFallbacksEnabled()
                && fallback_lighting_mrt0.handle != 0u) {
              deferred_mrt_normal_srv = fallback_lighting_mrt0;
              deferred_mrt_normal_valid = true;
            }
            const bool deferred_depth_ssao_valid =
                IsViewAlive(device, data->xegtao_deferred_depth_srv)
                && IsViewAlive(device, data->xegtao_deferred_ssao_srv);
            bool deferred_scene_cbv_valid =
                data->xegtao_deferred_scene_cbv_valid
                && data->xegtao_deferred_scene_cbv.buffer.handle != 0u
                && IsSceneCbvCandidateValid(device, data->xegtao_deferred_scene_cbv);
            if (deferred_scene_cbv_valid) {
              reshade::api::buffer_range normalized_deferred_scene_cbv = {};
              if (TryNormalizeSceneCbvRange(device, data->xegtao_deferred_scene_cbv, &normalized_deferred_scene_cbv)) {
                data->xegtao_deferred_scene_cbv = normalized_deferred_scene_cbv;
              } else {
                deferred_scene_cbv_valid = false;
              }
            }

            if (!deferred_scene_cbv_valid && data->fallback_scene_cbv_seen) {
              data->captured_scene_cbv = data->xegtao_deferred_scene_cbv;
              data->captured_scene_cbv_valid = data->xegtao_deferred_scene_cbv_valid;
              data->captured_scene_cbv_frame = data->xegtao_deferred_scene_cbv_frame;
              data->captured_scene_cbv_source = data->xegtao_deferred_scene_cbv_source;
              if (TryAdoptFallbackSceneCbv(device, data)) {
                data->xegtao_deferred_scene_cbv = data->captured_scene_cbv;
                data->xegtao_deferred_scene_cbv_valid = data->captured_scene_cbv_valid;
                data->xegtao_deferred_scene_cbv_frame = data->captured_scene_cbv_frame;
                data->xegtao_deferred_scene_cbv_source = data->captured_scene_cbv_source;
                deferred_scene_cbv_valid = true;
              }
            }

            if (!deferred_depth_ssao_valid || !deferred_scene_cbv_valid) {
              if (data->xegtao_deferred_drop_log_frame != frame) {
                data->xegtao_deferred_drop_log_frame = frame;
                const std::string drop_reason = !deferred_depth_ssao_valid
                    ? "frozen t3/t4 views invalid"
                    : "frozen scene CB b0 invalid";
                AddonLog(
                    reshade::log::level::warning,
                    std::format(
                        "XeGTAO deferred dispatch dropped: {} (frame={})",
                        drop_reason,
                        frame));
              }
              data->xegtao_debug_deferred_drop_count += 1u;
              data->xegtao_debug_last_deferred_drop_frame = frame;
              data->xegtao_deferred_dispatch_pending = false;
              data->xegtao_deferred_dispatch_executed = true;
            } else {
              data->captured_depth_srv = data->xegtao_deferred_depth_srv;
              data->captured_ssao_srv = data->xegtao_deferred_ssao_srv;
              data->captured_mrt_normal_srv =
                  deferred_mrt_normal_valid ? deferred_mrt_normal_srv : reshade::api::resource_view{};
              if (!deferred_mrt_normal_valid) {
                data->xegtao_debug_normal_fallback_count += 1u;
                if (data->xegtao_debug_last_normal_fallback_frame != frame) {
                  data->xegtao_debug_last_normal_fallback_frame = frame;
                  AddonLog(
                      reshade::log::level::info,
                      std::format(
                          "XeGTAO deferred normal fallback: MRT normal SRV missing, using depth-derived path (frame={})",
                          frame));
                }
              }
              data->captured_scene_cbv = data->xegtao_deferred_scene_cbv;
              data->captured_scene_cbv_valid = data->xegtao_deferred_scene_cbv_valid;
              data->captured_scene_cbv_frame = data->xegtao_deferred_scene_cbv_frame;
              data->captured_scene_cbv_source = data->xegtao_deferred_scene_cbv_source;
              data->resolved_scene_cbv_from_current_bindings =
                  data->xegtao_deferred_resolved_scene_cbv_from_current_bindings;

              const bool fix_l4_guard =
                  fix_mode >= static_cast<uint32_t>(XeGTAOFixMode::kStrictSideEffectGuard);
              data->xegtao_dispatch_isolation_active = true;
              ScopedDescriptorCaptureSkip dispatch_capture_guard(fix_l4_guard);
              data->xegtao_trace_dispatch_attempted = true;
              const bool has_result_this_frame = RunXeGTAOForFrame(
                  cmd_list,
                  data,
                  false,
                  false);
                AddonLog(
                  reshade::log::level::info,
                  std::format(
                    "XeGTAO deferred-present dispatch frame {} (succeeded={}, normal_mode={}, mrt_normal_valid={}, main_pass={}, composite_pass={})",
                    frame,
                    has_result_this_frame ? 1 : 0,
                    ClampXeGTAONormalInputMode(),
                    data->xegtao_mrt_normal_valid ? 1 : 0,
                    data->xegtao_trace_main_pass_executed ? 1 : 0,
                    data->xegtao_trace_composite_pass_executed ? 1 : 0));
              data->xegtao_trace_dispatch_succeeded = has_result_this_frame;
              data->xegtao_dispatch_isolation_active = false;
              data->xegtao_dispatch_restore_mismatch = false;

              data->xegtao_deferred_dispatch_pending = false;
              data->xegtao_deferred_dispatch_executed = true;

              if (has_result_this_frame) {
                data->xegtao_owner_valid =
                    data->xegtao_result_signature_valid
                    && data->xegtao_result_signature_frame == frame;
                data->xegtao_owner_frame = frame;
                data->xegtao_owner_shader_hash = data->xegtao_deferred_owner_shader_hash;
                data->xegtao_owner_draw_ordinal = data->xegtao_deferred_owner_draw_ordinal;
                data->xegtao_owner_gate_signature = data->xegtao_deferred_gate_signature;
                data->xegtao_owner_downscaled = data->xegtao_owner_valid && IsXeGTAOResultDownscaled(data);
                data->xegtao_trace_owner_draw_ordinal = data->xegtao_owner_draw_ordinal;
              }
            }
          }
        }
      }
      data->xegtao_deferred_dispatch_executed = false;
      data->present_frame_index += 1u;
    }
  }
  (void)swapchain;
  (void)source_rect;
  (void)dest_rect;
  (void)dirty_rect_count;
  (void)dirty_rects;

  const float isfast_enabled = isfast_jitter_master >= 0.5f ? 1.f : 0.f;
  shader_injection.shadow_pcss_jitter_enabled = isfast_enabled;
  shader_injection.shadow_isfast_jitter_amount = isfast_enabled;
  shader_injection.shadow_isfast_jitter_speed = 237.f;
  shader_injection.char_shadow_jitter_enabled = isfast_enabled;
  shader_injection.foliage_sss_jitter_enabled = isfast_enabled;
  shader_injection.volfog_enabled = std::clamp(std::round(shader_injection.volfog_enabled), 0.f, 1.f);
  shader_injection.volfog_jitter_enabled =
      std::clamp(std::round(shader_injection.volfog_jitter_enabled), 0.f, 1.f);
  shader_injection.volfog_jitter_amount = std::clamp(shader_injection.volfog_jitter_amount, 0.f, 2.f);
  shader_injection.volfog_jitter_speed = std::clamp(shader_injection.volfog_jitter_speed, 0.f, 1024.f);
  shader_injection.volfog_is_fast_enabled =
      isfast_enabled >= 0.5f
      && shader_injection.volfog_enabled >= 0.5f
      && shader_injection.volfog_jitter_enabled >= 0.5f
      && shader_injection.volfog_jitter_amount > 0.0001f
      ? 1.f
      : 0.f;
  shader_injection.wire_alpha_mode = isfast_enabled > 0.5f ? 2.f : 0.f;
  shader_injection.wire_alpha_threshold_offset = 0.f;
  shader_injection.wire_alpha_sharpen = 1.f;
  shader_injection.wire_alpha_temporal_amount = isfast_enabled > 0.5f ? 0.77f : 0.f;
  shader_injection.wire_alpha_temporal_speed = 237.f;

  // Sync char_gi_enabled: 1 when On (method>=0.5), 0 when Off
  shader_injection.char_gi_enabled =
      (char_ssgi_composite_method >= 0.5f) ? 1.f : 0.f;

  // Basic mode: force all char_gi params to their defaults so sliders have no effect
  if (settings_mode < 0.5f) {
    shader_injection.char_gi_strength = 3.0f;
    shader_injection.char_gi_alpha_scale = 1.0f;
    shader_injection.char_gi_chroma_strength = 0.50f;
    shader_injection.char_gi_luma_strength = 0.0f;
    shader_injection.char_gi_shadow_power = 1.25f;
    shader_injection.char_gi_headroom_power = 1.25f;
    shader_injection.char_gi_max_add = 0.020f;
    shader_injection.char_gi_dark_boost = 0.0f;
    shader_injection.char_gi_debug_mode = 0.f;
    shader_injection.char_gi_debug_scale = 1.f;
    shader_injection.char_gi_debug_chars_only = 1.f;
    shader_injection.char_gi_bright_boost = 3.0f;
    shader_injection.char_gi_peak_luma_cap = 0.0f;
    shader_injection.char_gi_depth_reject = 2.0f;
    shader_injection.char_gi_normal_reject = 0.15f;
    shader_injection.char_gi_ao_influence = 0.66f;
    shader_injection.char_gi_reject_strength = 8.0f;
  }

  shader_injection.sss_dedicated_bound = 0.f;
  shader_injection.xegtao_dedicated_bound = 0.f;
  shader_injection.xegtao_force_neutral_x = 0.f;
  shader_injection.xegtao_debug_blackout = 0.f;
  shader_injection.xegtao_ao_active_for_draw = 0.f;
  xegtao_mode = std::clamp(std::round(xegtao_mode), 0.f, 1.f);
  xegtao_fix_mode = std::clamp(std::round(xegtao_fix_mode), 0.f, 5.f);
  xegtao_fix_l5_prefilter = static_cast<float>(ClampBooleanToggle(xegtao_fix_l5_prefilter));
  xegtao_fix_l5_main = static_cast<float>(ClampBooleanToggle(xegtao_fix_l5_main));
  xegtao_fix_l5_denoise = static_cast<float>(ClampBooleanToggle(xegtao_fix_l5_denoise));
  xegtao_fix_l5_composite = static_cast<float>(ClampBooleanToggle(xegtao_fix_l5_composite));
  xegtao_probe_a_dispatch_no_t22 = static_cast<float>(ClampBooleanToggle(xegtao_probe_a_dispatch_no_t22));
  xegtao_probe_b_t22_no_dispatch = static_cast<float>(ClampBooleanToggle(xegtao_probe_b_t22_no_dispatch));
  xegtao_runtime_debug_logging = static_cast<float>(ClampBooleanToggle(xegtao_runtime_debug_logging));
  xegtao_enable_fallbacks = static_cast<float>(ClampBooleanToggle(xegtao_enable_fallbacks));
  g_enable_runtime_addon_logs.store(xegtao_runtime_debug_logging >= 0.5f, std::memory_order_relaxed);
  xegtao_denoiser_mode = 0.f;
  xegtao_isfast_jitter_amount = std::clamp(xegtao_isfast_jitter_amount, 0.f, 1.f);
  xegtao_foliage_ao_blend = std::clamp(xegtao_foliage_ao_blend, 0.f, 1.f);
  xegtao_foliage_mask_method = std::clamp(std::round(xegtao_foliage_mask_method), 0.f, 2.f);
  shader_injection.xegtao_debug_mode = std::clamp(std::round(xegtao_debug_mode), 0.f, 21.f);
  shader_injection.xegtao_normal_input_mode = static_cast<float>(ClampXeGTAONormalInputMode());
  shader_injection.xegtao_mrt_normal_valid = 0.f;
  shader_injection.xegtao_bent_normals = 0.f;
  shader_injection.xegtao_bent_diffuse_strength = 0.f;
  shader_injection.xegtao_bent_diffuse_softness = 0.f;
  shader_injection.xegtao_bent_specular_strength = 0.f;
  shader_injection.xegtao_bent_specular_proxy_roughness = 0.f;
  shader_injection.xegtao_bent_max_darkening = 0.f;
  shader_injection.xegtao_foliage_ao_blend = std::clamp(xegtao_foliage_ao_blend, 0.f, 1.f);
  shader_injection.xegtao_foliage_mask_method = xegtao_foliage_mask_method;
}

bool IsAdvancedSettingsMode() {
  return settings_mode >= 0.5f;
}

renodx::utils::settings::Settings settings = {
    new renodx::utils::settings::Setting{
        .key = "SettingsMode",
        .binding = &settings_mode,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f,
        .can_reset = false,
        .label = "Settings Mode",
        .section = "Settings",
        .labels = {"Basic", "Intermediate"},
        .is_global = true,
    },
    new renodx::utils::settings::Setting{
        .key = "ISFastJitterMaster",
        .binding = &isfast_jitter_master,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Master",
        .section = "IS-FAST Jitter (Dont enable if you are not using Temporal Solution.)",
        .tooltip = "Off disables all IS-FAST jitter features. On enables them.",
        .labels = {"Off", "On"},
        .is_global = true,
    },
    new renodx::utils::settings::Setting{
        .key = "XeGTAOMode",
        .binding = &xegtao_mode,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 1.f,
        .label = "Mode",
        .section = "XeGTAO",
        .tooltip = "Enable or disable XeGTAO.",
        .labels = {"Off", "On"},
        .min = 0.f,
        .max = 1.f,
    },
    new renodx::utils::settings::Setting{
        .key = "XeGTAOQualityV2",
        .binding = &xegtao_quality,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f,
        .label = "Quality",
        .section = "XeGTAO",
        .tooltip = "High, Very High, and Ultra only. Low/Medium were removed to avoid noisy output.",
        .labels = {"High", "Very High", "Ultra"},
        .is_enabled = []() { return xegtao_mode >= 0.5f; },
    },
    new renodx::utils::settings::Setting{
      .key = "XeGTAOFix",
      .binding = &xegtao_fix_mode,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f,
      .label = "XeGTAO Fix",
        .section = "XeGTAO",
      .tooltip = "Off keeps the current path. Levels 1-5 add cumulative dispatch/apply protections against volumetric side effects.",
      .labels = {
        "Off",
        "1 - Producer/Consumer",
        "2 - + Isolation/Restore",
        "3 - + Single Owner",
        "4 - + Strict Guard",
        "5 - + Pass Isolation",
      },
        .is_enabled = []() { return xegtao_mode >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "XeGTAOFixL5Prefilter",
      .binding = &xegtao_fix_l5_prefilter,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 1.f,
      .label = "Fix L5 Prefilter",
      .section = "XeGTAO",
      .tooltip = "Level 5 diagnostic toggle for prefilter pass.",
      .labels = {"Off", "On"},
      .is_enabled = []() {
        return xegtao_mode >= 0.5f
           && IsXeGTAOFixLevelAtLeast(XeGTAOFixMode::kPassIsolationDiagnostics);
      },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "XeGTAOFixL5Main",
      .binding = &xegtao_fix_l5_main,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 1.f,
      .label = "Fix L5 Main",
      .section = "XeGTAO",
      .tooltip = "Level 5 diagnostic toggle for main AO pass.",
      .labels = {"Off", "On"},
      .is_enabled = []() {
        return xegtao_mode >= 0.5f
           && IsXeGTAOFixLevelAtLeast(XeGTAOFixMode::kPassIsolationDiagnostics);
      },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "XeGTAOFixL5Denoise",
      .binding = &xegtao_fix_l5_denoise,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 1.f,
      .label = "Fix L5 Denoise",
      .section = "XeGTAO",
      .tooltip = "Level 5 diagnostic toggle for denoise passes.",
      .labels = {"Off", "On"},
      .is_enabled = []() {
        return xegtao_mode >= 0.5f
           && IsXeGTAOFixLevelAtLeast(XeGTAOFixMode::kPassIsolationDiagnostics);
      },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "XeGTAOFixL5Composite",
      .binding = &xegtao_fix_l5_composite,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 1.f,
      .label = "Fix L5 Composite",
      .section = "XeGTAO",
      .tooltip = "Level 5 diagnostic toggle for final composite pass.",
      .labels = {"Off", "On"},
      .is_enabled = []() {
        return xegtao_mode >= 0.5f
           && IsXeGTAOFixLevelAtLeast(XeGTAOFixMode::kPassIsolationDiagnostics);
      },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "XeGTAOProbeADispatchNoT22",
      .binding = &xegtao_probe_a_dispatch_no_t22,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 0.f,
      .label = "Probe A: Dispatch No t22",
      .section = "XeGTAO",
      .tooltip = "Runs XeGTAO dispatch/composite but suppresses t22 bind to isolate dispatch-only side effects.",
      .labels = {"Off", "On"},
      .is_enabled = []() {
        return xegtao_mode >= 0.5f
           && IsXeGTAOFixLevelAtLeast(XeGTAOFixMode::kDispatchIsolationRestore);
      },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
      .key = "XeGTAOProbeBT22NoDispatch",
      .binding = &xegtao_probe_b_t22_no_dispatch,
      .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
      .default_value = 0.f,
      .label = "Probe B: t22 No Dispatch",
      .section = "XeGTAO",
      .tooltip = "Exercises t22 bind path without new XeGTAO dispatch to isolate bind-side effects.",
      .labels = {"Off", "On"},
      .is_enabled = []() {
        return xegtao_mode >= 0.5f
           && IsXeGTAOFixLevelAtLeast(XeGTAOFixMode::kDispatchIsolationRestore)
           && xegtao_probe_a_dispatch_no_t22 < 0.5f;
      },
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "XeGTAOPrecision",
        .binding = &xegtao_precision,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 2.f,
        .label = "Depth Precision",
        .section = "XeGTAO",
        .tooltip = "Internal-only compatibility key. XeGTAO precision is runtime-forced to Full FP32.",
        .labels = {"Depth R16", "Depth R32", "Full FP32"},
        .is_enabled = []() { return false; },
        .is_visible = []() { return false; },
    },
    new renodx::utils::settings::Setting{
        .key = "XeGTAONormalInputMode",
        .binding = &xegtao_normal_input_mode,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 1.f,
        .label = "Normal Input",
        .section = "XeGTAO",
        .tooltip = "Off uses depth-fallback normals. View-Transformed uses MRT normals transformed to view space.",
        .labels = {"Off", "View-Transformed"},
        .is_enabled = []() { return xegtao_mode >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "XeGTAONormalInfluence",
        .binding = &xegtao_normal_influence,
        .default_value = 0.20f,
        .label = "Normal Influence",
        .section = "XeGTAO",
        .tooltip = "Scales MRT normal XY contribution before blending.",
        .min = 0.f,
        .max = 2.f,
        .format = "%.2f",
        .is_enabled = []() { return xegtao_mode >= 0.5f && xegtao_normal_input_mode >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "XeGTAONormalDepthBlend",
        .binding = &xegtao_normal_depth_blend,
        .default_value = 0.70f,
        .label = "Normal-Depth Blend",
        .section = "XeGTAO",
        .tooltip = "0 uses depth fallback normals, 1 fully uses tuned MRT normals.",
        .min = 0.f,
        .max = 1.f,
        .format = "%.2f",
        .is_enabled = []() { return xegtao_mode >= 0.5f && xegtao_normal_input_mode >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "XeGTAONormalSharpness",
        .binding = &xegtao_normal_sharpness,
        .default_value = 1.f,
        .label = "Normal Sharpness",
        .section = "XeGTAO",
        .tooltip = "Shapes the normal-depth blend response; higher values sharpen MRT normal influence.",
        .min = 0.5f,
        .max = 2.5f,
        .format = "%.2f",
        .is_enabled = []() { return xegtao_mode >= 0.5f && xegtao_normal_input_mode >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "XeGTAONormalEdgeRejection",
        .binding = &xegtao_normal_edge_rejection,
        .default_value = 1.f,
        .label = "Normal Edge Reject",
        .section = "XeGTAO",
        .tooltip = "Suppresses MRT normal influence near depth edges to reduce halo artifacts.",
        .min = 0.f,
        .max = 4.f,
        .format = "%.2f",
        .is_enabled = []() { return xegtao_mode >= 0.5f && xegtao_normal_input_mode >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "XeGTAONormalZPreservation",
        .binding = &xegtao_normal_z_preservation,
        .default_value = 1.f,
        .label = "Normal Z Preserve",
        .section = "XeGTAO",
        .tooltip = "Preserves or flattens MRT normal Z before blending into depth fallback normals.",
        .min = 0.f,
        .max = 2.f,
        .format = "%.2f",
        .is_enabled = []() { return xegtao_mode >= 0.5f && xegtao_normal_input_mode >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "XeGTAONormalDetailResponse",
        .binding = &xegtao_normal_detail_response,
        .default_value = 4.f,
        .label = "Normal Detail Resp",
        .section = "XeGTAO",
        .tooltip = "Controls how strongly MRT-vs-depth normal deltas boost blend weight.",
        .min = 0.25f,
        .max = 4.f,
        .format = "%.2f",
        .is_enabled = []() { return xegtao_mode >= 0.5f && xegtao_normal_input_mode >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "XeGTAONormalMaxDarkening",
        .binding = &xegtao_normal_max_darkening,
        .default_value = 0.6f,
        .label = "Normal Max Dark",
        .section = "XeGTAO",
        .tooltip = "Limits how much additional darkening normal input can contribute.",
        .min = 0.f,
        .max = 1.f,
        .format = "%.2f",
        .is_enabled = []() { return xegtao_mode >= 0.5f && xegtao_normal_input_mode >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "XeGTAONormalDarkeningMode",
        .binding = &xegtao_normal_darkening_mode,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f,
        .label = "Normal Dark Mode",
        .section = "XeGTAO",
        .tooltip = "Fast clamps normal influence directly. Exact compares depth-only vs normal AO and clamps only extra darkening.",
        .labels = {"Fast", "Exact"},
        .is_enabled = []() { return xegtao_mode >= 0.5f && xegtao_normal_input_mode >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "XeGTAOSkipVanillaAO",
        .binding = &xegtao_skip_vanilla_ao,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Skip Vanilla AO",
        .section = "XeGTAO",
        .tooltip =
            "Visible for A/B only. Ignored while XeGTAO is active because AO.z remains the SSS source.",
        .labels = {"Off", "On"},
        .is_enabled = []() { return xegtao_mode < 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "XeGTAODenoisePasses",
        .binding = &xegtao_denoise_pass_count,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 1.f,
        .label = "Denoise Passes",
        .section = "XeGTAO",
        .tooltip = "0 to 3 denoise passes. 0 still runs the final resolve pass.",
        .min = 0.f,
        .max = 3.f,
        .format = "%d",
        .is_enabled = []() { return xegtao_mode >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "XeGTAODenoiserMode",
        .binding = &xegtao_denoiser_mode,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f,
        .label = "Denoiser Mode",
        .section = "XeGTAO",
        .tooltip = "Vanilla uses XeGTAO denoise only. IS-FAST Only replaces it. Hybrid runs vanilla then IS-FAST.",
        .labels = {"Vanilla", "IS-FAST Only", "Hybrid"},
        .is_enabled = []() { return false; },
        .is_visible = []() { return false; },
    },
    new renodx::utils::settings::Setting{
        .key = "XeGTAOISFASTJitter",
        .binding = &xegtao_isfast_jitter,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 1.f,
        .label = "Temporal Jitter",
        .section = "XeGTAO",
        .tooltip = "Off freezes IS-FAST temporal noise. On enables temporal jitter.",
        .labels = {"Off", "On"},
        .is_enabled = []() { return false; },
        .is_visible = []() { return false; },
    },
    new renodx::utils::settings::Setting{
        .key = "XeGTAOISFASTJitterAmount",
        .binding = &xegtao_isfast_jitter_amount,
        .default_value = 1.f,
        .label = "Temporal Jitter Amt",
        .section = "XeGTAO",
        .tooltip = "0 = static sampling pattern, 1 = full temporal jitter movement.",
        .min = 0.f,
        .max = 1.f,
        .format = "%.2f",
        .is_enabled = []() { return false; },
        .is_visible = []() { return false; },
    },
    new renodx::utils::settings::Setting{
        .key = "XeGTAOISFASTPasses",
        .binding = &xegtao_isfast_passes,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 2.f,
        .label = "IS-FAST Passes",
        .section = "XeGTAO",
        .tooltip = "Number of IS-FAST denoise passes.",
        .min = 1.f,
        .max = 4.f,
        .format = "%d",
        .is_enabled = []() { return false; },
        .is_visible = []() { return false; },
    },
    new renodx::utils::settings::Setting{
        .key = "XeGTAOISFASTSamples",
        .binding = &xegtao_isfast_samples,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 8.f,
        .label = "IS-FAST Samples",
        .section = "XeGTAO",
        .tooltip = "Per-pixel stochastic samples used by each IS-FAST pass.",
        .min = 2.f,
        .max = 16.f,
        .format = "%d",
        .is_enabled = []() { return false; },
        .is_visible = []() { return false; },
    },
    new renodx::utils::settings::Setting{
        .key = "XeGTAOISFASTRadius",
        .binding = &xegtao_isfast_radius,
        .default_value = 1.0f,
        .label = "IS-FAST Radius",
        .section = "XeGTAO",
        .tooltip = "Filter radius in pixels for IS-FAST denoise.",
        .min = 0.25f,
        .max = 8.f,
        .format = "%.2f",
        .is_enabled = []() { return false; },
        .is_visible = []() { return false; },
    },
    new renodx::utils::settings::Setting{
        .key = "XeGTAOISFASTEdgeSensitivity",
        .binding = &xegtao_isfast_edge_sensitivity,
        .default_value = 2.0f,
        .label = "IS-FAST Edge Sens",
        .section = "XeGTAO",
        .tooltip = "Higher values preserve edge discontinuities more aggressively.",
        .min = 0.f,
        .max = 8.f,
        .format = "%.2f",
        .is_enabled = []() { return false; },
        .is_visible = []() { return false; },
    },
    new renodx::utils::settings::Setting{
        .key = "XeGTAOISFASTSpatialSigma",
        .binding = &xegtao_isfast_spatial_sigma,
        .default_value = 1.0f,
        .label = "IS-FAST Sigma",
        .section = "XeGTAO",
        .tooltip = "Spatial Gaussian sigma for IS-FAST denoise.",
        .min = 0.1f,
        .max = 8.f,
        .format = "%.2f",
        .is_enabled = []() { return false; },
        .is_visible = []() { return false; },
    },
    new renodx::utils::settings::Setting{
        .key = "XeGTAOISFASTHybridBlend",
        .binding = &xegtao_isfast_hybrid_blend,
        .default_value = 0.5f,
        .label = "IS-FAST Hybrid Mix",
        .section = "XeGTAO",
        .tooltip = "Blend amount of IS-FAST output in Hybrid mode.",
        .min = 0.f,
        .max = 1.f,
        .format = "%.2f",
        .is_enabled = []() { return false; },
        .is_visible = []() { return false; },
    },
    new renodx::utils::settings::Setting{
        .key = "XeGTAORadius",
        .binding = &xegtao_radius,
        .default_value = 0.5f,
        .label = "Radius",
        .section = "XeGTAO",
        .min = 0.01f,
        .max = 10.f,
        .format = "%.2f",
        .is_enabled = []() { return xegtao_mode >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "XeGTAOFalloffRange",
        .binding = &xegtao_falloff_range,
        .default_value = 0.615f,
        .label = "Falloff Range",
        .section = "XeGTAO",
        .min = 0.f,
        .max = 1.f,
        .format = "%.3f",
        .is_enabled = []() { return xegtao_mode >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "XeGTAORadiusMultiplier",
        .binding = &xegtao_radius_multiplier,
        .default_value = 1.5f,
        .label = "Radius Multiplier",
        .section = "XeGTAO",
        .min = 0.3f,
        .max = 3.f,
        .format = "%.3f",
        .is_enabled = []() { return xegtao_mode >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "XeGTAOFinalPower",
        .binding = &xegtao_final_value_power,
        .default_value = 2.0f,
        .label = "Final Power",
        .section = "XeGTAO",
        .min = 0.5f,
        .max = 5.f,
        .format = "%.2f",
        .is_enabled = []() { return xegtao_mode >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "XeGTAOSampleDistribution",
        .binding = &xegtao_sample_distribution_power,
        .default_value = 1.5f,
        .label = "Sample Distribution",
        .section = "XeGTAO",
        .min = 1.f,
        .max = 3.f,
        .format = "%.2f",
        .is_enabled = []() { return xegtao_mode >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "XeGTAOThinOccluderCompensation",
        .binding = &xegtao_thin_occluder_compensation,
        .default_value = 0.50f,
        .label = "Thin Occluder Comp",
        .section = "XeGTAO",
        .min = 0.f,
        .max = 0.7f,
        .format = "%.2f",
        .is_enabled = []() { return xegtao_mode >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "XeGTAODepthMipOffset",
        .binding = &xegtao_depth_mip_sampling_offset,
        .default_value = 3.3f,
        .label = "Depth MIP Offset",
        .section = "XeGTAO",
        .min = 0.f,
        .max = 30.f,
        .format = "%.2f",
        .is_enabled = []() { return xegtao_mode >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "XeGTAODenoiseBlurBeta",
        .binding = &xegtao_denoise_blur_beta,
        .default_value = 8.f,
        .label = "Denoise Blur Beta",
        .section = "XeGTAO",
        .min = 0.01f,
        .max = 8.f,
        .format = "%.2f",
        .is_enabled = []() { return xegtao_mode >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "XeGTAOFoliageShading",
        .binding = &xegtao_foliage_ao_blend,
        .default_value = 100.f,
        .label = "Foliage Shading",
        .section = "XeGTAO",
        .tooltip = "Blends foliage AO between mask-off neutral shading and full masked XeGTAO shading.",
        .min = 0.f,
        .max = 100.f,
        .format = "%.0f",
        .is_enabled = []() { return xegtao_mode >= 0.5f; },
        .parse = [](float value) { return std::clamp(value, 0.f, 100.f) * 0.01f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
      new renodx::utils::settings::Setting{
        .key = "XeGTAOFoliageMaskMethod",
        .binding = &xegtao_foliage_mask_method,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f,
        .label = "Foliage Mask Method",
        .section = "XeGTAO",
          .tooltip = "Selects XeGTAO foliage mask method: SSS parity strict t1, legacy broad t1, or strict t10 char mask.",
          .labels = {"SSS Parity (t1 Strict)", "Legacy Broad (t1)", "CharMask Strict (t10)"},
        .is_enabled = []() { return xegtao_mode >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
      },
    new renodx::utils::settings::Setting{
        .key = "XeGTAOFallbacks",
        .binding = &xegtao_enable_fallbacks,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Fallbacks",
        .section = "XeGTAO",
        .tooltip =
          "Off disables XeGTAO fallback sources (fallback scene CBV and deferred MRT-normal fallback). On enables those fallback paths.",
        .labels = {"Off", "On"},
        .is_enabled = []() { return xegtao_mode >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
      },
      new renodx::utils::settings::Setting{
        .key = "XeGTAORuntimeDebugLogging",
        .binding = &xegtao_runtime_debug_logging,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f,
        .label = "Runtime Debug Logs",
        .section = "XeGTAO",
        .tooltip =
          "Enables low-volume XeGTAO runtime logs for descriptor rejects, deferred drops, and normal fallbacks.",
        .labels = {"Off", "On"},
        .is_enabled = []() { return xegtao_mode >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "XeGTAODebugMode",
        .binding = &xegtao_debug_mode,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f,
        .label = "Debug",
        .section = "XeGTAO",
        .labels = {
            "00 Off",
            "01 XeGTAO AO",
            "02 Vanilla AO",
            "03 AO Delta",
            "04 XeGTAO RGB",
            "05 Vanilla AO YZ",
            "06 Depth Raw",
            "07 Depth Edge",
            "08 SSS Shadow",
            "09 AO YZ Active",
            "10 MRT Normal XY",
            "11 MRT Normal Z",
            "12 Selected Normal",
            "13 Normal Length",
            "14 Normal Source Mask",
            "15 Pixel Class Mask",
            "16 XeGTAO Bind Mask",
            "17 AO X Source Compare",
            "18 AO Live XYZ",
            "19 AO Effective Proxy",
            "20 Foliage AO Gate",
            "21 Foliage Mask Methods RGB",
        },
        .is_enabled = []() { return xegtao_mode >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "ShadowBaseSoftness",
        .binding = &shader_injection.shadow_base_softness,
        .default_value = 0.2f,
        .label = "Base Softness",
        .section = "Shadows",
        .tooltip = "Adds a constant to the PCSS filter radius. 0.0 is Vanilla.",
        .min = 0.f,
        .max = 0.5f,
        .format = "%.2f",
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CubemapImprovements",
        .binding = &shader_injection.cubemap_improvements_enabled,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 1.f,
        .label = "Mode",
        .section = "Cubemap",
        .labels = {"Vanilla", "Improved"},
    },
    new renodx::utils::settings::Setting{
        .key = "LightingCubemapMipBoost",
        .binding = &shader_injection.cubemap_lighting_mip_boost,
        .default_value = 1.5f,
        .label = "Lighting Mip Boost",
        .section = "Cubemap",
        .tooltip = "Lighting shader cubemap mip scale. Default is 1.5x.",
        .min = 0.5f,
        .max = 4.f,
        .format = "%.1fx",
        .is_enabled = []() { return shader_injection.cubemap_improvements_enabled >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
      new renodx::utils::settings::Setting{
        .key = "FloorCubemapMipScale",
        .binding = &shader_injection.floor_cubemap_mip_scale,
        .default_value = 4.f,
        .label = "Floor Mip Scale",
        .section = "Cubemap",
        .tooltip = "Scales floor reflection roughness/mip response. 1.0 = Vanilla.",
        .min = 0.f,
        .max = 4.f,
        .format = "%.2f",
        .is_visible = []() { return IsAdvancedSettingsMode(); },
      },
    new renodx::utils::settings::Setting{
        .key = "SSGIEnable",
        .binding = &shader_injection.ssgi_mod_enabled,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Enable",
        .section = "SSGI",
        .labels = {"Off", "On"},
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "SSGIColorBoost",
        .binding = &shader_injection.ssgi_color_boost,
        .default_value = 1.f,
        .label = "Color Boost",
        .section = "SSGI",
        .tooltip = "Scales SSGI RGB contribution before power shaping.",
        .min = 0.f,
        .max = 3.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.ssgi_mod_enabled >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "SSGIAlphaBoost",
        .binding = &shader_injection.ssgi_alpha_boost,
        .default_value = 1.f,
        .label = "Alpha Boost",
        .section = "SSGI",
        .tooltip = "Scales SSGI alpha before saturate.",
        .min = 0.f,
        .max = 3.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.ssgi_mod_enabled >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "SSGIPower",
        .binding = &shader_injection.ssgi_pow,
        .default_value = 1.f,
        .label = "Power",
        .section = "SSGI",
        .tooltip = "Applies pow(abs(color), Power) to shape bounce response.",
        .min = 0.1f,
        .max = 3.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.ssgi_mod_enabled >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "SSRMode",
        .binding = &shader_injection.ssr_mode,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 1.f,
        .label = "Mode",
        .section = "SSR",
        .labels = {"Vanilla", "Improved"},
    },
    new renodx::utils::settings::Setting{
        .key = "SSRRayCountScale",
        .binding = &shader_injection.ssr_ray_count_scale,
        .default_value = 2.5f,
        .label = "Ray Count Scale",
        .section = "SSR",
        .tooltip = "Scales the game's SSR ray count budget. 1.0x is Vanilla.",
        .min = 0.5f,
        .max = 8.f,
        .format = "%.1fx",
        .is_enabled = []() { return shader_injection.ssr_mode >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
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
        .is_enabled = []() { return shader_injection.dof_mode >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DOFSampleCount",
        .binding = &shader_injection.dof_sample_count,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 24.f,
        .label = "Sample Count",
        .section = "Depth of Field",
        .tooltip = "Higher values produce smoother bokeh at higher cost.",
        .min = 4.f,
        .max = 64.f,
        .format = "%d",
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
        .default_value = 1.f,
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
        .default_value = 0.25f,
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
        .key = "CharShadowMode",
        .binding = &shader_injection.char_shadow_mode,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 2.f,
        .label = "Mode",
        .section = "Character Shadowing",
        .labels = {"Off", "Vanilla", "Bend_SSS"},
    },
    new renodx::utils::settings::Setting{
        .key = "CharShadowStrength",
        .binding = &shader_injection.char_shadow_strength,
        .default_value = 100.f,
        .label = "Strength",
        .section = "Character Shadowing",
        .tooltip = "Blend strength for character sun screen-space shadowing.",
        .min = 0.f,
        .max = 100.f,
        .is_enabled = []() { return shader_injection.char_shadow_mode >= 1.f; },
        .parse = [](float value) { return value * 0.01f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CharShadowSampleCount",
        .binding = &shader_injection.char_shadow_sample_count,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 32.f,
        .label = "Sample Count",
        .section = "Character Shadowing",
        .tooltip = "Higher values increase shadow reach/quality at higher cost.",
        .min = 1.f,
        .max = 64.f,
        .format = "%d",
        .is_enabled = []() { return shader_injection.char_shadow_mode == 2.f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CharShadowHardSamples",
        .binding = &shader_injection.char_shadow_hard_shadow_samples,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 4.f,
        .label = "Hard Samples",
        .section = "Character Shadowing",
        .tooltip = "Near-contact samples that preserve harder shadows.",
        .min = 0.f,
        .max = 64.f,
        .format = "%d",
        .is_enabled = []() { return shader_injection.char_shadow_mode == 2.f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CharShadowFadeSamples",
        .binding = &shader_injection.char_shadow_fade_out_samples,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 16.f,
        .label = "Fade Samples",
        .section = "Character Shadowing",
        .tooltip = "Tail samples used to soften the far shadow cutoff.",
        .min = 0.f,
        .max = 64.f,
        .format = "%d",
        .is_enabled = []() { return shader_injection.char_shadow_mode == 2.f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CharShadowSurfaceThickness",
        .binding = &shader_injection.char_shadow_surface_thickness,
        .default_value = 0.09f,
        .label = "Surface Thickness",
        .section = "Character Shadowing",
        .tooltip = "Depth thickness assumption for occluder matching.",
        .min = 0.001f,
        .max = 0.2f,
        .format = "%.4f",
        .is_enabled = []() { return shader_injection.char_shadow_mode == 2.f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CharShadowContrast",
        .binding = &shader_injection.char_shadow_contrast,
        .default_value = 9.f,
        .label = "Shadow Contrast",
        .section = "Character Shadowing",
        .tooltip = "Higher values darken/crisp the character shadow transition.",
        .min = 0.f,
        .max = 12.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.char_shadow_mode == 2.f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CharShadowLightFadeStart",
        .binding = &shader_injection.char_shadow_light_screen_fade_start,
        .default_value = 0.f,
        .label = "Light Fade Start",
        .section = "Character Shadowing",
        .tooltip = "Minimum projected sun length before shadows ramp in.",
        .min = 0.f,
        .max = 1.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.char_shadow_mode == 2.f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CharShadowLightFadeEnd",
        .binding = &shader_injection.char_shadow_light_screen_fade_end,
        .default_value = 0.f,
        .label = "Light Fade End",
        .section = "Character Shadowing",
        .tooltip = "Projected sun length where shadows reach full strength.",
        .min = 0.f,
        .max = 1.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.char_shadow_mode == 2.f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CharShadowMinOccluderDepthScale",
        .binding = &shader_injection.char_shadow_min_occluder_depth_scale,
        .default_value = 0.f,
        .label = "Occluder Depth Scale",
        .section = "Character Shadowing",
        .tooltip = "Rejects tiny depth deltas to reduce self-shadowing noise.",
        .min = 0.f,
        .max = 4.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.char_shadow_mode == 2.f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    // ── SSS ──
    new renodx::utils::settings::Setting{
        .key = "FoliageSSSEnabled",
        .binding = &shader_injection.foliage_sss_enabled,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Bend SSS",
        .section = "Screen Space Shadows",
        .tooltip = "Screen-space shadowing effect (Bend Studio algorithm).",
        .labels = {"Off", "On"},
    },
    new renodx::utils::settings::Setting{
        .key = "FoliageSSSStrength",
        .binding = &shader_injection.foliage_sss_strength,
      .default_value = 100.f,
        .label = "Strength",
        .section = "Screen Space Shadows",
        .tooltip = "Blend strength for screen-space shadows.",
        .min = 0.f,
        .max = 100.f,
        .is_enabled = []() { return shader_injection.foliage_sss_enabled >= 0.5f; },
        .parse = [](float value) { return value * 0.01f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "FoliageSSSSampleCount",
        .binding = &shader_injection.foliage_sss_sample_count,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 24.f,
        .label = "Sample Count",
        .section = "Screen Space Shadows",
        .tooltip = "Higher values increase shadow reach/quality at higher cost.",
        .min = 1.f,
        .max = 64.f,
        .format = "%d",
        .is_enabled = []() { return shader_injection.foliage_sss_enabled >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "FoliageSSSSurfaceThickness",
        .binding = &shader_injection.foliage_sss_surface_thickness,
        .default_value = 0.005f,
        .label = "Surface Thickness",
        .section = "Screen Space Shadows",
        .tooltip = "Depth thickness assumption for occluder matching.",
        .min = 0.001f,
        .max = 0.2f,
        .format = "%.4f",
        .is_enabled = []() { return shader_injection.foliage_sss_enabled >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "FoliageSSSContrast",
        .binding = &shader_injection.foliage_sss_contrast,
      .default_value = 2.f,
        .label = "Shadow Contrast",
        .section = "Screen Space Shadows",
        .tooltip = "Higher values darken/crisp the shadow transition.",
        .min = 0.f,
        .max = 12.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.foliage_sss_enabled >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
      new renodx::utils::settings::Setting{
        .key = "FoliageSSSHeightEnable",
        .binding = &shader_injection.foliage_sss_height_enabled,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Height Above Ground",
        .section = "Screen Space Shadows",
        .tooltip = "Only apply SSS to pixels above a certain height from the ground surface below them. Adapts to any map elevation.",
        .labels = {"Off", "On"},
        .is_enabled = []() { return shader_injection.foliage_sss_enabled >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
      },
      new renodx::utils::settings::Setting{
        .key = "FoliageSSSHeightMin",
        .binding = &shader_injection.foliage_sss_height_min,
        .default_value = 0.f,
        .label = "Min Height",
        .section = "Screen Space Shadows",
        .tooltip = "Minimum height above the ground (world units) before SSS starts. Pixels closer to the ground get no SSS.",
        .min = 0.f,
        .max = 10.f,
        .format = "%.2f",
        .is_enabled = []() {
          return shader_injection.foliage_sss_enabled >= 0.5f &&
             shader_injection.foliage_sss_height_enabled >= 0.5f;
        },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
      },
      new renodx::utils::settings::Setting{
        .key = "FoliageSSSHeightMax",
        .binding = &shader_injection.foliage_sss_height_max,
        .default_value = 1.f,
        .label = "Ground Search",
        .section = "Screen Space Shadows",
        .tooltip = "How many pixels downward on-screen to search for the ground surface. Higher = works for taller geometry.",
        .min = 1.f,
        .max = 200.f,
        .format = "%.0f",
        .is_enabled = []() {
          return shader_injection.foliage_sss_enabled >= 0.5f &&
             shader_injection.foliage_sss_height_enabled >= 0.5f;
        },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
      },
      new renodx::utils::settings::Setting{
        .key = "FoliageSSSHeightFade",
        .binding = &shader_injection.foliage_sss_height_fade,
        .default_value = 0.10f,
        .label = "Height Fade",
        .section = "Screen Space Shadows",
        .tooltip = "Smooth transition range (world units) above the min height threshold.",
        .min = 0.f,
        .max = 5.f,
        .format = "%.2f",
        .is_enabled = []() {
          return shader_injection.foliage_sss_enabled >= 0.5f &&
             shader_injection.foliage_sss_height_enabled >= 0.5f;
        },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
      },
      new renodx::utils::settings::Setting{
        .key = "FoliageSSSVerticalReject",
        .binding = &shader_injection.foliage_sss_vertical_reject,
        .default_value = 0.30f,
        .label = "Vertical Reject",
        .section = "Screen Space Shadows",
        .tooltip = "Rejects vertical surfaces (walls, pillars). 0 = off, higher = stricter. Based on how upward-facing the normal is.",
        .min = 0.f,
        .max = 1.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.foliage_sss_enabled >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
      },
      new renodx::utils::settings::Setting{
        .key = "FoliageSSSMaxDarkening",
        .binding = &shader_injection.foliage_sss_max_darkening,
        .default_value = 0.40f,
        .label = "Max Darkening",
        .section = "Screen Space Shadows",
        .tooltip = "Limits how dark shadows can get. 1.0 = full darkening allowed, 0.0 = no darkening at all.",
        .min = 0.f,
        .max = 1.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.foliage_sss_enabled >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
      },
      new renodx::utils::settings::Setting{
        .key = "FoliageSSBrightRejectThreshold",
        .binding = &shader_injection.foliage_sss_bright_reject_threshold,
        .default_value = 0.19f,
        .label = "Brightness Reject",
        .section = "Screen Space Shadows",
        .tooltip = "Pixels brighter than this luminance will resist SSS darkening. Protects lamps and emissive surfaces.",
        .min = 0.f,
        .max = 5.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.foliage_sss_enabled >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
      },
      new renodx::utils::settings::Setting{
        .key = "FoliageSSBrightRejectFade",
        .binding = &shader_injection.foliage_sss_bright_reject_fade,
        .default_value = 0.5f,
        .label = "Brightness Fade",
        .section = "Screen Space Shadows",
        .tooltip = "How gradual the brightness rejection transition is. Lower = sharper cutoff.",
        .min = 0.01f,
        .max = 3.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.foliage_sss_enabled >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
      },
    // -- SSS Debug --
    new renodx::utils::settings::Setting{
        .key = "FoliageDebugMode",
        .binding = &shader_injection.foliage_debug_mode,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f,
        .label = "Debug Mode",
        .section = "SSS Debug",
        .tooltip = "Debug visualization for the SSS pipeline.",
        .labels = {"Off", "SSS Mask", "Shadow Value", "SSAO Texture", "MRT Bits", "Bit9 Raw", "Vanilla Detect", "Raw Bytes"},
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },

    new renodx::utils::settings::Setting{
        .key = "CharacterSSGICompositeMethod",
        .binding = &char_ssgi_composite_method,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 1.f,
        .label = "Apply Game SSGI",
        .section = "Character SSGI Composite",
        .labels = {"Off", "On"},
    },
    new renodx::utils::settings::Setting{
        .key = "CharacterSSGICompositeStrength",
        .binding = &shader_injection.char_gi_strength,
        .default_value = 3.0f,
        .label = "Strength",
        .section = "Character SSGI Composite",
        .tooltip = "Overall contribution scale for character GI.",
        .min = 0.f,
        .max = 3.f,
        .format = "%.2f",
        .is_enabled = []() { return char_ssgi_composite_method >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CharacterSSGICompositeAlphaScale",
        .binding = &shader_injection.char_gi_alpha_scale,
        .default_value = 1.0f,
        .label = "Alpha Scale",
        .section = "Character SSGI Composite",
        .tooltip = "Scales sampled SSGI alpha before blending.",
        .min = 0.f,
        .max = 3.f,
        .format = "%.2f",
        .is_enabled = []() { return char_ssgi_composite_method >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CharacterSSGICompositeChroma",
        .binding = &shader_injection.char_gi_chroma_strength,
        .default_value = 0.50f,
        .label = "Chroma",
        .section = "Character SSGI Composite",
        .tooltip = "Scales colorful GI component; lower values reduce tinting.",
        .min = 0.f,
        .max = 2.f,
        .format = "%.2f",
        .is_enabled = []() { return char_ssgi_composite_method >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CharacterSSGICompositeLuma",
        .binding = &shader_injection.char_gi_luma_strength,
        .default_value = 0.0f,
        .label = "Luma",
        .section = "Character SSGI Composite",
        .tooltip = "Scales neutral GI brightness; keep low to avoid white haze.",
        .min = 0.f,
        .max = 1.f,
        .format = "%.3f",
        .is_enabled = []() { return char_ssgi_composite_method >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CharacterSSGICompositeShadowPower",
        .binding = &shader_injection.char_gi_shadow_power,
        .default_value = 1.25f,
        .label = "Shadow Power",
        .section = "Character SSGI Composite",
        .tooltip = "Higher values concentrate GI toward darker areas.",
        .min = 0.1f,
        .max = 4.f,
        .format = "%.2f",
        .is_enabled = []() { return char_ssgi_composite_method >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CharacterSSGICompositeDarkBoost",
        .binding = &shader_injection.char_gi_dark_boost,
        .default_value = 0.0f,
        .label = "Dark Boost",
        .section = "Character SSGI Composite",
        .tooltip = "Extra GI multiplier in darker regions (after shadow mask).",
        .min = 0.f,
        .max = 4.f,
        .format = "%.2f",
        .is_enabled = []() { return char_ssgi_composite_method >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CharacterSSGICompositeBrightBoost",
        .binding = &shader_injection.char_gi_bright_boost,
        .default_value = 3.0f,
        .label = "Bright Boost",
        .section = "Character SSGI Composite",
        .tooltip = "Boosts GI on brighter regions (values above 1.0 increase bright-side contribution).",
        .min = 0.f,
        .max = 3.f,
        .format = "%.2f",
        .is_enabled = []() { return char_ssgi_composite_method >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CharacterSSGICompositeHeadroomPower",
        .binding = &shader_injection.char_gi_headroom_power,
        .default_value = 1.25f,
        .label = "Headroom Power",
        .section = "Character SSGI Composite",
        .tooltip = "Controls how strongly bright pixels reject additional GI.",
        .min = 0.1f,
        .max = 4.f,
        .format = "%.2f",
        .is_enabled = []() { return char_ssgi_composite_method >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CharacterSSGICompositeMaxAdd",
        .binding = &shader_injection.char_gi_max_add,
        .default_value = 0.020f,
        .label = "Max Add",
        .section = "Character SSGI Composite",
        .tooltip = "Per-channel cap for added GI to prevent haze/bloomy washout.",
        .min = 0.f,
        .max = 1.f,
        .format = "%.3f",
        .is_enabled = []() { return char_ssgi_composite_method >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CharacterSSGICompositePeakLumaCap",
        .binding = &shader_injection.char_gi_peak_luma_cap,
        .default_value = 0.0f,
        .label = "Peak Luma Cap",
        .section = "Character SSGI Composite",
        .tooltip = "Caps peak GI brightness on characters after blending weights. Set 0 to disable.",
        .min = 0.f,
        .max = 1.f,
        .format = "%.3f",
        .is_enabled = []() { return char_ssgi_composite_method >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CharacterSSGICompositeDepthReject",
        .binding = &shader_injection.char_gi_depth_reject,
        .default_value = 2.0f,
        .label = "Depth Reject",
        .section = "Character SSGI Composite",
        .tooltip = "Higher values suppress GI across depth discontinuities and silhouette edges.",
        .min = 0.f,
        .max = 16.f,
        .format = "%.2f",
        .is_enabled = []() { return char_ssgi_composite_method >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CharacterSSGICompositeNormalReject",
        .binding = &shader_injection.char_gi_normal_reject,
        .default_value = 0.15f,
        .label = "Normal Reject",
        .section = "Character SSGI Composite",
        .tooltip = "Higher values suppress GI across normal/material edges.",
        .min = 0.f,
        .max = 8.f,
        .format = "%.2f",
        .is_enabled = []() { return char_ssgi_composite_method >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CharacterSSGICompositeAOInfluence",
        .binding = &shader_injection.char_gi_ao_influence,
        .default_value = 0.66f,
        .label = "AO Influence",
        .section = "Character SSGI Composite",
        .tooltip = "Uses game's AO to dampen character GI in occluded areas.",
        .min = 0.f,
        .max = 1.f,
        .format = "%.2f",
        .is_enabled = []() { return char_ssgi_composite_method >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CharacterSSGICompositeRejectStrength",
        .binding = &shader_injection.char_gi_reject_strength,
        .default_value = 8.0f,
        .label = "Reject Strength",
        .section = "Character SSGI Composite",
        .tooltip = "Amplifies depth/normal/AO rejection visibility without changing base GI sliders.",
        .min = 0.f,
        .max = 8.f,
        .format = "%.2f",
        .is_enabled = []() { return char_ssgi_composite_method >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CharacterSSGICompositeDebugMode",
        .binding = &shader_injection.char_gi_debug_mode,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f,
        .label = "Debug View",
        .section = "Character SSGI Composite",
        .labels = {
            "Off",
            "Character Mask",
            "Raw SSGI RGB",
            "Raw SSGI Alpha",
            "Filtered GI",
            "GI Weight (Pre-Headroom)",
            "GI Contribution Abs",
            "Source Color",
            "GI Contribution Luma",
            "Headroom",
            "Final Gain",
            "Shadow Mask (Adj)",
            "Depth Factor",
            "Normal Factor",
            "AO Factor",
            "Reject Factor",
        },
        .is_enabled = []() { return char_ssgi_composite_method >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CharacterSSGICompositeDebugScale",
        .binding = &shader_injection.char_gi_debug_scale,
        .default_value = 1.f,
        .label = "Debug Scale",
        .section = "Character SSGI Composite",
        .tooltip = "Scales debug intensity for RGB/alpha/weight/contribution views.",
        .min = 0.1f,
        .max = 32.f,
        .format = "%.2f",
        .is_enabled = []() {
          return char_ssgi_composite_method >= 0.5f
                 && shader_injection.char_gi_debug_mode >= 1.f;
        },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CharacterSSGICompositeDebugCharsOnly",
        .binding = &shader_injection.char_gi_debug_chars_only,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Debug Characters Only",
        .section = "Character SSGI Composite",
        .labels = {"Off", "On"},
        .is_enabled = []() {
          return char_ssgi_composite_method >= 0.5f
                 && shader_injection.char_gi_debug_mode >= 1.f;
        },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "VolFogMode",
        .binding = &shader_injection.volfog_enabled,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Mode",
        .section = "Volumetric Fog",
        .tooltip = "Turn volumetric fog on or off.",
        .labels = {"Off", "On"},
    },
    new renodx::utils::settings::Setting{
        .key = "VolFogTemporalJitter",
        .binding = &shader_injection.volfog_jitter_enabled,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Temporal Jitter",
        .section = "Volumetric Fog",
        .tooltip = "Enable or disable temporal jitter for volumetric fog sampling.",
        .labels = {"Off", "On"},
        .is_enabled = []() { return shader_injection.volfog_enabled >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "VolFogTemporalJitterAmount",
        .binding = &shader_injection.volfog_jitter_amount,
        .default_value = 0.7f,
        .label = "Temporal Jitter Amount",
        .section = "Volumetric Fog",
        .tooltip = "Strength of temporal jitter applied to volumetric fog sampling.",
        .min = 0.f,
        .max = 2.f,
        .format = "%.2f",
        .is_enabled = []() {
          return shader_injection.volfog_enabled >= 0.5f
                 && shader_injection.volfog_jitter_enabled >= 0.5f;
        },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "VolFogTemporalJitterSpeed",
        .binding = &shader_injection.volfog_jitter_speed,
        .default_value = 237.f,
        .label = "Temporal Jitter Speed",
        .section = "Volumetric Fog",
        .tooltip = "Temporal progression speed for volumetric fog jitter.",
        .min = 0.f,
        .max = 1024.f,
        .format = "%.0f",
        .is_enabled = []() {
          return shader_injection.volfog_enabled >= 0.5f
                 && shader_injection.volfog_jitter_enabled >= 0.5f;
        },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "VolFogTricubic",
        .binding = &shader_injection.volfog_tricubic_enabled,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Tricubic Sampling",
        .section = "Volumetric Fog",
        .tooltip = "Replaces trilinear with tricubic B-spline filtering on the fog volume, eliminating blocky voxel boundaries.",
        .labels = {"Off", "On"},
        .is_enabled = []() { return shader_injection.volfog_enabled >= 0.5f; },
    },
    new renodx::utils::settings::Setting{
        .key = "VolFogColorCorrectionStrength",
        .binding = &shader_injection.volfog_color_correction_strength,
        .default_value = 0.5f,
        .label = "Color Correction Strength",
        .section = "Volumetric Fog",
        .tooltip = "Independent strength for volumetric fog color correction.",
        .min = 0.f,
        .max = 1.f,
        .format = "%.2f",
        .is_enabled = []() {
          return shader_injection.volfog_enabled >= 0.5f
                 && shader_injection.fog_color_correction_enabled >= 0.5f;
        },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "FogColorCorrection",
        .binding = &shader_injection.fog_color_correction_enabled,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Mode",
        .section = "Fog Color Correction",
        .labels = {"Vanilla", "Improved"},
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
        .is_enabled = []() {
          return shader_injection.fog_color_correction_enabled >= 0.5f;
        },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
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
        .is_enabled = []() {
          return shader_injection.fog_color_correction_enabled >= 0.5f;
        },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
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
        .is_enabled = []() {
          return shader_injection.fog_color_correction_enabled >= 0.5f;
        },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
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
        .is_enabled = []() {
          return shader_injection.fog_color_correction_enabled >= 0.5f;
        },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
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
        .is_enabled = []() {
          return shader_injection.fog_color_correction_enabled >= 0.5f;
        },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
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
        .is_enabled = []() {
          return shader_injection.fog_color_correction_enabled >= 0.5f;
        },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
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
        .is_enabled = []() {
          return shader_injection.fog_color_correction_enabled >= 0.5f;
        },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
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
        .is_enabled = []() {
          return shader_injection.fog_color_correction_enabled >= 0.5f;
        },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
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
        .label = "IS-FAST Jitter: Dont enable if you are not using Temporal Solution.",
        .section = "Info",
    },
    new renodx::utils::settings::Setting{
        .value_type = renodx::utils::settings::SettingValueType::TEXT,
        .label = "Enable Soft/PCSS Shadows and Ultra SSR.",
        .section = "Info",
    },
    new renodx::utils::settings::Setting{
        .value_type = renodx::utils::settings::SettingValueType::TEXT,
        .label = "Enable Character Only Local Shadowing for XeGTAO. Default doesn't work.",
        .section = "Info",
    },
};

}  // namespace

extern "C" __declspec(dllexport) constexpr const char* NAME = "Kai Vanilla+";
extern "C" __declspec(dllexport) constexpr const char* DESCRIPTION = "Kai Vanilla+";

BOOL APIENTRY DllMain(HMODULE h_module, DWORD fdw_reason, LPVOID lpv_reserved) {
  switch (fdw_reason) {
    case DLL_PROCESS_ATTACH:
      if (!reshade::register_addon(h_module)) return FALSE;
      g_hmodule = h_module;

      reshade::register_event<reshade::addon_event::init_device>(OnInitDevice);
      reshade::register_event<reshade::addon_event::destroy_device>(OnDestroyDevice);
      reshade::register_event<reshade::addon_event::init_swapchain>(OnInitSwapchain);
      reshade::register_event<reshade::addon_event::destroy_swapchain>(OnDestroySwapchain);
      reshade::register_event<reshade::addon_event::present>(OnPresentAdvanceFrame);
      reshade::register_event<reshade::addon_event::push_descriptors>(OnPushDescriptorsCaptureLightingTextures);
      reshade::register_event<reshade::addon_event::bind_descriptor_tables>(OnBindDescriptorTablesCaptureLightingTextures);

      renodx::utils::settings::overlay_title = "Kai Vanilla+";
      renodx::utils::settings::global_name = "kai-vanilla-plus";
      renodx::utils::settings::use_presets = false;

      renodx::mods::shader::force_pipeline_cloning = true;
      renodx::mods::shader::allow_multiple_push_constants = true;
      renodx::mods::shader::expected_constant_buffer_index = 13;
      renodx::mods::shader::expected_constant_buffer_space = 0;

      break;
    case DLL_PROCESS_DETACH:
      reshade::unregister_event<reshade::addon_event::init_device>(OnInitDevice);
      reshade::unregister_event<reshade::addon_event::destroy_device>(OnDestroyDevice);
      reshade::unregister_event<reshade::addon_event::init_swapchain>(OnInitSwapchain);
      reshade::unregister_event<reshade::addon_event::destroy_swapchain>(OnDestroySwapchain);
      reshade::unregister_event<reshade::addon_event::present>(OnPresentAdvanceFrame);
      reshade::unregister_event<reshade::addon_event::push_descriptors>(OnPushDescriptorsCaptureLightingTextures);
      reshade::unregister_event<reshade::addon_event::bind_descriptor_tables>(OnBindDescriptorTablesCaptureLightingTextures);
      reshade::unregister_addon(h_module);
      break;
  }

  renodx::utils::resource::Use(fdw_reason);
  renodx::utils::swapchain::Use(fdw_reason);
  renodx::utils::descriptor::trace_descriptor_tables = true;
  renodx::utils::descriptor::Use(fdw_reason);
  renodx::utils::state::Use(fdw_reason);
  renodx::utils::settings::Use(fdw_reason, &settings);
  renodx::mods::shader::Use(fdw_reason, custom_shaders, &shader_injection);

  return TRUE;
}
