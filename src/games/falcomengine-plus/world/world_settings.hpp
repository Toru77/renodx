#pragma once

// Ray Tracing tab content.
//
// The tab only exists when DevKit is present (world::AddSettings adds rows only
// when g_state.supported is true). The BVH panel (Pool Scan, GPU debug views)
// is the main workflow; the census panel lists draws per VS family.

#include <algorithm>
#include <cstdio>
#include <functional>

#include <Windows.h>

#include "world_state.hpp"
#include "bvh/world_bvh.hpp"
#include "rtao/rtao.hpp"
#include "rtao/rtao_panel.hpp"
#include "capture/draw_census.hpp"

namespace falcom_world {

inline bool IsDevkitPresent() {
  if (GetModuleHandleA("renodx-devkit.addon64") != nullptr) return true;
  if (GetModuleHandleA("renodx-devkit.addon32") != nullptr) return true;
  static const bool file_present = []() {
    char exe_path[MAX_PATH] = {};
    const DWORD length = GetModuleFileNameA(nullptr, exe_path, MAX_PATH);
    if (length == 0 || length >= MAX_PATH) return false;
    const std::string path(exe_path, length);
    const auto slash = path.find_last_of("\\/");
    if (slash == std::string::npos) return false;
    const std::string directory = path.substr(0, slash + 1);
    static const char* const candidates[] = {
        "renodx-devkit.addon64",
        "renodx-devkit.addon32",
    };
    for (const auto* candidate : candidates) {
      if (GetFileAttributesA((directory + candidate).c_str()) != INVALID_FILE_ATTRIBUTES) return true;
    }
    return false;
  }();
  return file_present;
}

inline void AddSettings(renodx::utils::settings::Settings* settings, bool supported, std::function<bool()> advanced_visible = nullptr) {
  if (settings == nullptr) return;
  g_state.supported = supported && IsDevkitPresent();
  if (!g_state.supported) return;

  using Setting = renodx::utils::settings::Setting;
  using SettingValueType = renodx::utils::settings::SettingValueType;
  const auto visible = []() { return g_state.supported; };
  const std::function<bool()> advanced = advanced_visible ? advanced_visible : std::function<bool()>(visible);

  settings->push_back(new Setting{
      .key = "WorldRayTracingRows",
      .value_type = SettingValueType::CUSTOM,
      .can_reset = false,
      .label = "What enters the BVH",
      .section = "Ray Tracing",
      .on_draw = []() { return bvh::DrawRayTracingRows(); },
      .is_visible = visible,
  });
  // Ray Tracing tab row values ("What enters the BVH"). Hidden from the UI (drawn
  // inline by DrawRayTracingRows) but persisted so they survive a restart.
  settings->push_back(new Setting{
      .key = "WorldRegionSize",
      .binding = &bvh::g_pool.region_size,
      .value_type = SettingValueType::FLOAT,
      .default_value = 256.f,
      .label = "Region size",
      .section = "Ray Tracing",
      .min = bvh::kPoolRegionMinSize,
      .max = bvh::kPoolRegionMaxSize,
      .format = "%.0f",
      .is_visible = [] { return false; },
  });
  settings->push_back(new Setting{
      .key = "WorldWindSwayRange",
      .binding = &bvh::g_pool.wind_sway_range,
      .value_type = SettingValueType::FLOAT,
      .default_value = bvh::kPoolWindSwayRangeMax,
      .label = "Wind sway range",
      .section = "Ray Tracing",
      .min = bvh::kPoolWindSwayRangeMin,
      .max = bvh::kPoolWindSwayRangeMax,
      .format = "%.0f",
      .is_visible = [] { return false; },
  });
  settings->push_back(new Setting{
      .key = "WorldAlphaFoliage",
      .binding = &bvh::g_pool.alpha_foliage_setting,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 1.f,
      .label = "Alpha foliage",
      .section = "Ray Tracing",
      .is_visible = [] { return false; },
  });
  settings->push_back(new Setting{
      .key = "WorldAlphaWindSway",
      .binding = &bvh::g_pool.alpha_wind_sway_setting,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 1.f,
      .label = "Wind sway",
      .section = "Ray Tracing",
      .is_visible = [] { return false; },
  });
  settings->push_back(new Setting{
      .key = "WorldDeformingMeshes",
      .binding = &bvh::g_deform_live.enabled_setting,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 1.f,
      .label = "Deforming meshes",
      .section = "Ray Tracing",
      .is_visible = [] { return false; },
  });
  settings->push_back(new Setting{
      .key = "WorldFollowMoving",
      .binding = &bvh::g_pool.follow_moving_setting,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 1.f,
      .label = "Follow moving",
      .section = "Ray Tracing",
      .is_visible = [] { return false; },
  });
  settings->push_back(new Setting{
      .key = "WorldDynamicPerFrame",
      .binding = &bvh::g_pool.dynamic_per_frame_setting,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 1.f,
      .label = "Moving objects per-frame (P2b)",
      .section = "Ray Tracing",
      .is_visible = [] { return false; },
  });
  settings->push_back(new Setting{
      .key = "WorldExcludeMoving",
      .binding = &bvh::g_pool.exclude_moving_setting,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 0.f,
      .label = "Exclude moving from static BVH",
      .section = "Ray Tracing",
      .is_visible = [] { return false; },
  });
  settings->push_back(new Setting{
      .key = "WorldScanIndirect",
      .binding = &bvh::g_pool.scan_indirect_setting,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 1.f,
      .label = "Scan indirect draws",
      .section = "Ray Tracing",
      .is_visible = [] { return false; },
  });
  settings->push_back(new Setting{
      .key = "WorldResearchEnabled",
      .binding = &g_state.setting_enabled,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 0.f,
      .label = "Draw Census",
      .section = "BVH Diagnostics",
      .tooltip = "Enable the Sora 2nd draw census (draws per VS family).",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "WorldBvhPanel",
      .value_type = SettingValueType::CUSTOM,
      .can_reset = false,
      .label = "Phase 1 World BVH",
      .section = "BVH Diagnostics",
      .on_draw = []() { return bvh::DrawBvhPanel(); },
      .is_visible = visible,
  });
  // BVH Diagnostics switches, persisted so they survive a restart. Hidden from the
  // UI (drawn inline by DrawBvhPanel) but saved to the ReShade ini.
  settings->push_back(new Setting{
      .key = "WorldPoolScan",
      .binding = &bvh::g_pool.scan_active_setting,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 0.f,
      .label = "Pool Scan",
      .section = "BVH Diagnostics",
      .is_visible = [] { return false; },
  });
  settings->push_back(new Setting{
      .key = "WorldPoolCaptureMeshes",
      .binding = &bvh::g_pool.capture_meshes_setting,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 1.f,
      .label = "Capture meshes",
      .section = "BVH Diagnostics",
      .is_visible = [] { return false; },
  });
  settings->push_back(new Setting{
      .key = "WorldPoolLogCaptures",
      .binding = &bvh::g_pool.log_captures_setting,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 0.f,
      .label = "Log mesh captures",
      .section = "BVH Diagnostics",
      .is_visible = [] { return false; },
  });
  settings->push_back(new Setting{
      .key = "WorldPoolVerifyMeshes",
      .binding = &bvh::g_pool.verify_meshes_setting,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 1.f,
      .label = "Verify mesh captures",
      .section = "BVH Diagnostics",
      .is_visible = [] { return false; },
  });
  settings->push_back(new Setting{
      .key = "WorldPoolRetryUnstable",
      .binding = &bvh::g_pool.retry_unstable_setting,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 0.f,
      .label = "Retry unstable meshes",
      .section = "BVH Diagnostics",
      .is_visible = [] { return false; },
  });
  settings->push_back(new Setting{
      .key = "WorldPoolLegacyScale",
      .binding = &bvh::g_pool.legacy_scale_setting,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 0.f,
      .label = "Legacy instance scale limits",
      .section = "BVH Diagnostics",
      .is_visible = [] { return false; },
  });
  settings->push_back(new Setting{
      .key = "WorldPoolPoisonStaging",
      .binding = &bvh::g_pool.poison_staging_setting,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 0.f,
      .label = "Poison mesh staging",
      .section = "BVH Diagnostics",
      .is_visible = [] { return false; },
  });
  settings->push_back(new Setting{
      .key = "WorldPoolAlphaIndirectSource",
      .binding = &bvh::g_pool.alpha_indirect_source_setting,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 1.f,
      .label = "Indirect foliage source copy",
      .section = "BVH Diagnostics",
      .is_visible = [] { return false; },
  });
  settings->push_back(new Setting{
      .key = "WorldPoolAlphaWindOpaque",
      .binding = &bvh::g_pool.alpha_wind_opaque_setting,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 1.f,
      .label = "Admit wind with opaque pixel shader",
      .section = "BVH Diagnostics",
      .is_visible = [] { return false; },
  });
  settings->push_back(new Setting{
      .key = "WorldLiveBvh",
      .binding = &bvh::g_live_bvh.enabled_setting,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 1.f,
      .label = "Live BVH",
      .section = "BVH Diagnostics",
      .is_visible = [] { return false; },
  });
  settings->push_back(new Setting{
      .key = "WorldLiveBvhSkipNoop",
      .binding = &bvh::g_live_bvh.skip_noop_setting,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 0.f,
      .label = "Skip no-op TLAS rebuilds",
      .section = "BVH Diagnostics",
      .is_visible = [] { return false; },
  });
  settings->push_back(new Setting{
      .key = "WorldLiveBvhSlack",
      .binding = &bvh::g_live_bvh.slack_setting,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 0.f,
      .label = "Slack TLAS",
      .section = "BVH Diagnostics",
      .is_visible = [] { return false; },
  });
  settings->push_back(new Setting{
      .key = "WorldGpuDebugMode",
      .binding = &bvh::g_bvh_debug.mode_setting,
      .value_type = SettingValueType::INTEGER,
      .default_value = 0.f,
      .label = "GPU Debug mode",
      .section = "BVH Diagnostics",
      .min = 0.f,
      .max = 3.f,
      .format = "%d",
      .is_visible = [] { return false; },
  });
  settings->push_back(new Setting{
      .key = "WorldGpuDebugHideCameraHidden",
      .binding = &bvh::g_bvh_trace.hide_camera_hidden_setting,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 1.f,
      .label = "Hide camera-hidden from trace",
      .section = "BVH Diagnostics",
      .is_visible = [] { return false; },
  });
  settings->push_back(new Setting{
      .key = "WorldCacheDiag",
      .binding = &bvh::g_cache_diag.enabled_setting,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 1.f,
      .label = "Collect cache diagnostics",
      .section = "BVH Diagnostics",
      .is_visible = [] { return false; },
  });
  settings->push_back(new Setting{
      .key = "WorldCrashLog",
      .binding = &g_crash_log.enabled_setting,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 0.f,
      .label = "Log crashes",
      .section = "BVH Diagnostics",
      .is_visible = [] { return false; },
  });
  settings->push_back(new Setting{
      .key = "WorldPerformance",
      .value_type = SettingValueType::CUSTOM,
      .can_reset = false,
      .label = "Performance",
      .section = "Performance",
      .on_draw = []() {
        bvh::DrawPerformanceTab();
        return false;
      },
      .is_visible = visible,
  });

  settings->push_back(new Setting{
      .key = "RtaoEnabled",
      .binding = &rtao::g_rtao_enabled,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 0.f,
      .label = "RTAO",
      .section = "RTAO",
      .tooltip = "Ray-traced ambient occlusion from the live BVH. Overrides GTVBAO and VBGI while on. Turning it on enables Live BVH and Deforming meshes.",
      .labels = {"Off", "On"},
      .on_change = []() {
        if (rtao::RtaoRequested()) bvh::RtaoEnableBvhInputs();
      },
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtaoStatus",
      .value_type = SettingValueType::CUSTOM,
      .can_reset = false,
      .label = "RTAO status",
      .section = "RTAO",
      .on_draw = []() {
        rtao::DrawRtaoPanel();
        return false;
      },
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtaoDebug",
      .binding = &rtao::g_rtao_debug,
      .value_type = SettingValueType::INTEGER,
      .default_value = 0.f,
      .label = "RTAO Debug",
      .section = "RTAO",
      .tooltip = "Modes are shown directly on screen while RTAO produces AO (grayscale, scale 0.3, as the GTVBAO raw view); no GTVBAO Debug View needed. Modes 1 to 7 apply with Temporal on (pass B writes them); mode 8 needs Spatial Filter on and works with Temporal off too. Off = normal AO. 1 raw AO. 2 accumulated AO = unclamped reprojected history (raw where no tap is valid). 3 history confidence = valid tap fraction. 4 motion = saturate(px / 4), white at 4 px. 5 blend alpha = 0 to History Weight. 6 |raw - AO| = saturate(diff * 2). 7 motion vs camera matrix = saturate(px / 2) (static geometry only). A full white image in mode 3 or 5 means 1.0. 8 filter change = saturate(|filtered - AO| x 8), needs Spatial Filter on; modes 1 to 7 bypass the filter.",
      .labels = {"Off", "Raw AO", "Accumulated AO", "History confidence", "Motion (px)", "Blend alpha", "|raw - AO|", "Motion vs matrix (px)", "Filter change"},
      .is_visible = advanced,
  });
  settings->push_back(new Setting{
      .key = "RtaoTwoSidedDiscovery",
      .binding = &rtao::g_rtao_discovery,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 0.f,
      .label = "Two-Sided discovery (diagnostic)",
      .section = "RTAO",
      .tooltip = "Diagnostic only: counts facing and winding of the traced surfaces. Does not cull anything.",
      .labels = {"Off", "On"},
      .is_visible = advanced,
  });
  // Session-only test rows: the empty key makes SaveSettings and SaveGlobalSettings skip them (settings.hpp).
  settings->push_back(new Setting{
      .key = "",
      .binding = &rtao::g_test_zero_motion,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 0.f,
      .label = "Temporal test: force zero motion",
      .section = "RTAO",
      .tooltip = "Test, session only (not saved). Pass B looks up the history at the same pixel. The motion is still measured. Toggling it resets the history for one frame.",
      .labels = {"Off", "On"},
      .is_visible = advanced,
  });
  settings->push_back(new Setting{
      .key = "",
      .binding = &rtao::g_test_freeze_noise,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 0.f,
      .label = "Temporal test: freeze noise",
      .section = "RTAO",
      .tooltip = "Test, session only (not saved). The ray directions use a fixed slice and seed, so the noise does not change from frame to frame. Toggling it resets the history for one frame.",
      .labels = {"Off", "On"},
      .is_visible = advanced,
  });
  settings->push_back(new Setting{
      .key = "",
      .binding = &rtao::g_test_camera_matrix,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 0.f,
      .label = "Temporal test: camera-matrix reprojection",
      .section = "RTAO",
      .tooltip = "Test, session only (not saved). The history is looked up with the previous camera view-projection instead of the motion. Valid for static geometry only. Force zero motion wins if both are on. Toggling it resets the history for one frame.",
      .labels = {"Off", "On"},
      .is_visible = advanced,
  });
  settings->push_back(new Setting{
      .key = "RtaoRadius",
      .binding = &rtao::g_rtao_radius,
      .value_type = SettingValueType::FLOAT,
      .default_value = 1.f,
      .label = "Radius",
      .section = "RTAO",
      .tooltip = "AO search radius in metres (clamped to Ray Max Distance at use).",
      .min = 0.05f,
      .max = 5.f,
      .format = "%.2f",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtaoStrength",
      .binding = &rtao::g_rtao_strength,
      .value_type = SettingValueType::FLOAT,
      .default_value = 1.f,
      .label = "Strength",
      .section = "RTAO",
      .tooltip = "AO strength. 0 = no occlusion, 1 = physical.",
      .min = 0.f,
      .max = 4.f,
      .format = "%.2f",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtaoSamples",
      .binding = &rtao::g_rtao_samples,
      .value_type = SettingValueType::INTEGER,
      .default_value = 2.f,
      .label = "Samples Per Pixel",
      .section = "RTAO",
      .tooltip = "Cosine-hemisphere rays per pixel per frame.",
      .min = 1.f,
      .max = 8.f,
      .format = "%d",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtaoRayMax",
      .binding = &rtao::g_rtao_ray_max,
      .value_type = SettingValueType::FLOAT,
      .default_value = 2.f,
      .label = "Ray Max Distance",
      .section = "RTAO",
      .tooltip = "Rays longer than this are ignored. Pixels closer than this to a region face return neutral AO.",
      .min = 0.1f,
      .max = bvh::kPoolRegionRayMargin,
      .format = "%.2f",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtaoNormalBias",
      .binding = &rtao::g_rtao_normal_bias,
      .value_type = SettingValueType::FLOAT,
      .default_value = 0.05f,
      .label = "Normal Bias",
      .section = "RTAO",
      .tooltip = "Ray origin offset along the surface normal, in metres.",
      .min = 0.f,
      .max = 0.5f,
      .format = "%.4f",
      .is_visible = visible,
      .is_logarithmic = true,
  });
  settings->push_back(new Setting{
      .key = "RtaoTwoSided",
      .binding = &rtao::g_rtao_two_sided,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 1.f,
      .can_reset = false,
      .label = "Two-Sided Geometry",
      .section = "RTAO",
      .tooltip = "Always on in round 1.",
      .labels = {"Off", "On"},
      .is_enabled = []() { return false; },
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtaoCharacterIntensity",
      .binding = &rtao::g_rtao_char_intensity,
      .value_type = SettingValueType::FLOAT,
      .default_value = 1.f,
      .label = "Character Intensity",
      .section = "RTAO",
      .tooltip = "RTAO on character pixels: 1 = full RTAO, 0 = none. Applies whatever GTVBAO 'Allow GTVBAO' says (RTAO replaces GTVBAO; the GTVBAO character settings do not apply while RTAO runs). Character parts the game shades with the map AO keep the game's own AO tint at any value.",
      .min = 0.f,
      .max = 1.f,
      .format = "%.2f",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtaoTemporalEnabled",
      .binding = &rtao::g_rtao_temporal_enabled,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 1.f,
      .label = "Temporal Accumulation",
      .section = "Temporal Accumulation",
      .tooltip = "Reprojects the previous frame AO with the game motion (RTV4 target) and blends it in after validation. Resets on camera cuts, frame gaps and parameter changes.",
      .labels = {"Off", "On"},
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtaoHistoryWeight",
      .binding = &rtao::g_rtao_history_weight,
      .value_type = SettingValueType::FLOAT,
      .default_value = 0.9f,
      .label = "History Weight",
      .section = "Temporal Accumulation",
      .tooltip = "Maximum blend toward the reprojected history after validation (0 to 0.99).",
      .min = 0.f,
      .max = 0.99f,
      .format = "%.2f",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtaoDepthRejection",
      .binding = &rtao::g_rtao_depth_rejection,
      .value_type = SettingValueType::FLOAT,
      .default_value = 0.05f,
      .label = "Depth Rejection",
      .section = "Temporal Accumulation",
      .tooltip = "Relative distance difference allowed between history and current (0.001 to 0.2).",
      .min = 0.001f,
      .max = 0.2f,
      .format = "%.3f",
      .is_visible = visible,
      .is_logarithmic = true,
  });
  settings->push_back(new Setting{
      .key = "RtaoNormalRejection",
      .binding = &rtao::g_rtao_normal_rejection,
      .value_type = SettingValueType::FLOAT,
      .default_value = 0.9f,
      .label = "Normal Rejection",
      .section = "Temporal Accumulation",
      .tooltip = "History taps whose normal agrees with the current pixel (dot product) at or above this value count fully; below it their weight falls linearly to zero over a 0.4 range (default 0.9: full at 0.9, zero at 0.5). 0 accepts everything facing the same way.",
      .min = 0.f,
      .max = 1.f,
      .format = "%.2f",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtaoHistoryClamp",
      .binding = &rtao::g_rtao_history_clamp,
      .value_type = SettingValueType::FLOAT,
      .default_value = 1.f,
      .label = "History Clamp",
      .section = "Temporal Accumulation",
      .tooltip = "History is clamped to the 3x3 neighbourhood mean plus or minus k sigma (0 = off, 0 to 4).",
      .min = 0.f,
      .max = 4.f,
      .format = "%.2f",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtaoIsfast",
      .binding = &rtao::g_rtao_isfast,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 1.f,
      .label = "IS-FAST Noise",
      .section = "Denoising",
      .tooltip = "Blue-noise source for the AO rays (same IS-FAST texture as DOF and GTVBAO). Off = IGN noise.",
      .labels = {"Off", "On"},
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtaoSpatialEnabled",
      .binding = &rtao::g_rtao_spatial_enabled,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 0.f,
      .label = "Spatial Filter",
      .section = "Denoising",
      .tooltip = "Denoises the final RTAO output with edge-aware weights (depth plane and normal). The history keeps the unfiltered AO. Off = no filter, unchanged output.",
      .labels = {"Off", "On"},
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtaoFilterType",
      .binding = &rtao::g_rtao_filter_type,
      .value_type = SettingValueType::INTEGER,
      .default_value = 0.f,
      .label = "Filter Type",
      .section = "Denoising",
      .tooltip = "Both use the same depth and normal edge stops. Separable bilateral = Gaussian on every pixel, one horizontal and one vertical pass. A-trous = square binomial kernel (Taps x Taps) whose step doubles from pass to pass (the first pass is always dense, so no grid pattern).",
      .labels = {"Separable bilateral", "A-trous"},
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtaoFilterRadius",
      .binding = &rtao::g_rtao_filter_radius,
      .value_type = SettingValueType::INTEGER,
      .default_value = 2.f,
      .label = "Filter Radius",
      .section = "Denoising",
      .tooltip = "Pixel radius (1 to 8). Separable: sets the Gaussian width (sigma = radius / 2); Taps set how far it reaches. A-trous: sets the largest step the passes grow to (max(1, radius / 2); steps 1, 2, 4 up to it). With 1 pass the radius has no effect.",
      .min = 1.f,
      .max = 8.f,
      .format = "%d",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtaoFilterPasses",
      .binding = &rtao::g_rtao_filter_passes,
      .value_type = SettingValueType::INTEGER,
      .default_value = 1.f,
      .label = "Filter Passes",
      .section = "Denoising",
      .tooltip = "Number of filter passes (1 to 4). Separable: each pass is a horizontal and a vertical dispatch. A-trous: each pass is one 2D dispatch; the step doubles per pass up to the Radius ceiling. More passes smooth more and cost more.",
      .min = 1.f,
      .max = static_cast<float>(rtao::kFilterMaxPasses),
      .format = "%d",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtaoFilterTapsSeparable",
      .binding = &rtao::g_rtao_filter_taps_separable,
      .value_type = SettingValueType::INTEGER,
      .default_value = 5.f,
      .label = "Filter Taps (samples per direction)",
      .section = "Denoising",
      .tooltip = "Samples per direction per pass, 3 to 17, including the centre. Odd values are used as they are; an even value is rounded up to the next odd one. Reach = (taps - 1) / 2 pixels per side. Taps = 2 x radius + 1 covers the whole Gaussian.",
      .min = static_cast<float>(rtao::kFilterTapsMin),
      .max = static_cast<float>(rtao::kFilterSeparableTapsMax),
      .format = "%d",
      .is_visible = [visible]() { return visible() && rtao::g_rtao_filter_type < 0.5f; },
  });
  settings->push_back(new Setting{
      .key = "RtaoFilterTapsAtrous",
      .binding = &rtao::g_rtao_filter_taps_atrous,
      .value_type = SettingValueType::INTEGER,
      .default_value = 5.f,
      .label = "Filter Taps (kernel side)",
      .section = "Denoising",
      .tooltip = "Side length of the square kernel, 3 to 9 (taps x taps samples per pass). Odd values are used as they are; an even value is rounded up. Binomial weights: 3 = 1 2 1, 5 = 1 4 6 4 1, 7 = 1 6 15 20 15 6 1, 9 = 1 8 28 56 70 56 28 8 1.",
      .min = static_cast<float>(rtao::kFilterTapsMin),
      .max = static_cast<float>(rtao::kFilterAtrousTapsMax),
      .format = "%d",
      .is_visible = [visible]() { return visible() && rtao::g_rtao_filter_type > 0.5f; },
  });
  settings->push_back(new Setting{
      .key = "RtaoFadeStartPct",
      .binding = &rtao::g_rtao_fade_start_pct,
      .value_type = SettingValueType::FLOAT,
      .default_value = 15.625f,
      .label = "Fade Start",
      .section = "Distance Fade",
      .tooltip = "Where RTAO starts fading out, as a percent of the BVH range (Region). Capped by the built BVH range at use; the panel shows the effective metres.",
      .min = 0.f,
      .max = 100.f,
      .format = "%.1f%%",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtaoFadeEndPct",
      .binding = &rtao::g_rtao_fade_end_pct,
      .value_type = SettingValueType::FLOAT,
      .default_value = 31.25f,
      .label = "Fade End",
      .section = "Distance Fade",
      .tooltip = "Where RTAO is fully faded out, as a percent of the BVH range (Region). Capped by the built BVH range at use; the panel shows the effective metres.",
      .min = 0.f,
      .max = 100.f,
      .format = "%.1f%%",
      .is_visible = visible,
  });
}

}  // namespace falcom_world
