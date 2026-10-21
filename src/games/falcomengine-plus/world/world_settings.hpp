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
#include "rtgi/rtgi.hpp"
#include "rtgi/rtgi_panel.hpp"
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

// Temporal Accumulation and Denoising rows: one definition for RTAO and RTGI (same algorithm, separate values).
struct TemporalRows {
  const char* prefix;  // key prefix: "Rtao" or "Rtgi"
  const char* section;
  const char* enabled_tooltip;
  float* enabled;
  float enabled_default;
  float* history_weight;
  float history_weight_default;
  float* depth_rejection;
  float depth_rejection_default;
  float* normal_rejection;
  float normal_rejection_default;
  float* history_clamp;
  float history_clamp_default;
};
struct SpatialRows {
  const char* prefix;
  const char* section;
  const char* enabled_tooltip;
  float* enabled;
  float enabled_default;
  float* filter_type;
  float filter_type_default;
  float* filter_radius;
  float filter_radius_default;
  float* filter_passes;
  float filter_passes_default;
  float* taps_separable;
  float taps_separable_default;
  float* taps_atrous;
  float taps_atrous_default;
};
// Adds the five Temporal Accumulation rows for RTAO or RTGI. A plain function: MSVC rejects (C2326) the Setting
// aggregate initialisation inside a lambda, because Setting's default member initialisers are lambdas.
inline void AddTemporalRows(renodx::utils::settings::Settings* settings, const std::function<bool()>& visible,
                            const TemporalRows& rows) {
  using Setting = renodx::utils::settings::Setting;
  using SettingValueType = renodx::utils::settings::SettingValueType;
  const std::string prefix = rows.prefix;
  settings->push_back(new Setting{
      .key = prefix + "TemporalEnabled",
      .binding = rows.enabled,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = rows.enabled_default,
      .label = "Temporal Accumulation",
      .section = rows.section,
      .tooltip = rows.enabled_tooltip,
      .labels = {"Off", "On"},
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = prefix + "HistoryWeight",
      .binding = rows.history_weight,
      .value_type = SettingValueType::FLOAT,
      .default_value = rows.history_weight_default,
      .label = "History Weight",
      .section = rows.section,
      .tooltip = "Maximum blend toward the reprojected history after validation (0 to 0.99).",
      .min = 0.f,
      .max = 0.99f,
      .format = "%.2f",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = prefix + "DepthRejection",
      .binding = rows.depth_rejection,
      .value_type = SettingValueType::FLOAT,
      .default_value = rows.depth_rejection_default,
      .label = "Depth Rejection",
      .section = rows.section,
      .tooltip = "Relative distance difference allowed between history and current (0.001 to 0.2).",
      .min = 0.001f,
      .max = 0.2f,
      .format = "%.3f",
      .is_visible = visible,
      .is_logarithmic = true,
  });
  settings->push_back(new Setting{
      .key = prefix + "NormalRejection",
      .binding = rows.normal_rejection,
      .value_type = SettingValueType::FLOAT,
      .default_value = rows.normal_rejection_default,
      .label = "Normal Rejection",
      .section = rows.section,
      .tooltip = "History taps whose normal agrees with the current pixel (dot product) at or above this value count fully; below it their weight falls linearly to zero over a 0.4 range (default 0.9: full at 0.9, zero at 0.5). 0 accepts everything facing the same way.",
      .min = 0.f,
      .max = 1.f,
      .format = "%.2f",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = prefix + "HistoryClamp",
      .binding = rows.history_clamp,
      .value_type = SettingValueType::FLOAT,
      .default_value = rows.history_clamp_default,
      .label = "History Clamp",
      .section = rows.section,
      .tooltip = "History is clamped to the 3x3 neighbourhood mean plus or minus k sigma (0 = off, 0 to 4).",
      .min = 0.f,
      .max = 4.f,
      .format = "%.2f",
      .is_visible = visible,
  });
}

// Adds the six Spatial Filter rows for RTAO or RTGI (a plain function for the reason above).
inline void AddSpatialRows(renodx::utils::settings::Settings* settings, const std::function<bool()>& visible,
                           const SpatialRows& rows) {
  using Setting = renodx::utils::settings::Setting;
  using SettingValueType = renodx::utils::settings::SettingValueType;
  const std::string prefix = rows.prefix;
  settings->push_back(new Setting{
      .key = prefix + "SpatialEnabled",
      .binding = rows.enabled,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = rows.enabled_default,
      .label = "Spatial Filter",
      .section = rows.section,
      .tooltip = rows.enabled_tooltip,
      .labels = {"Off", "On"},
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = prefix + "FilterType",
      .binding = rows.filter_type,
      .value_type = SettingValueType::INTEGER,
      .default_value = rows.filter_type_default,
      .label = "Filter Type",
      .section = rows.section,
      .tooltip = "Both use the same depth and normal edge stops. Separable bilateral = Gaussian on every pixel, one horizontal and one vertical pass. A-trous = square binomial kernel (Taps x Taps) whose step doubles from pass to pass (the first pass is always dense, so no grid pattern).",
      .labels = {"Separable bilateral", "A-trous"},
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = prefix + "FilterRadius",
      .binding = rows.filter_radius,
      .value_type = SettingValueType::INTEGER,
      .default_value = rows.filter_radius_default,
      .label = "Filter Radius",
      .section = rows.section,
      .tooltip = "Pixel radius (1 to 8). Separable: sets the Gaussian width (sigma = radius / 2); Taps set how far it reaches. A-trous: sets the largest step the passes grow to (max(1, radius / 2); steps 1, 2, 4 up to it). With 1 pass the radius has no effect.",
      .min = 1.f,
      .max = 8.f,
      .format = "%d",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = prefix + "FilterPasses",
      .binding = rows.filter_passes,
      .value_type = SettingValueType::INTEGER,
      .default_value = rows.filter_passes_default,
      .label = "Filter Passes",
      .section = rows.section,
      .tooltip = "Number of filter passes (1 to 4). Separable: each pass is a horizontal and a vertical dispatch. A-trous: each pass is one 2D dispatch; the step doubles per pass up to the Radius ceiling. More passes smooth more and cost more.",
      .min = 1.f,
      .max = static_cast<float>(rtao::kFilterMaxPasses),
      .format = "%d",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = prefix + "FilterTapsSeparable",
      .binding = rows.taps_separable,
      .value_type = SettingValueType::INTEGER,
      .default_value = rows.taps_separable_default,
      .label = "Filter Taps (samples per direction)",
      .section = rows.section,
      .tooltip = "Samples per direction per pass, 3 to 17, including the centre. Odd values are used as they are; an even value is rounded up to the next odd one. Reach = (taps - 1) / 2 pixels per side. Taps = 2 x radius + 1 covers the whole Gaussian.",
      .min = static_cast<float>(rtao::kFilterTapsMin),
      .max = static_cast<float>(rtao::kFilterSeparableTapsMax),
      .format = "%d",
      .is_visible = [visible, filter_type = rows.filter_type]() { return visible() && *filter_type < 0.5f; },
  });
  settings->push_back(new Setting{
      .key = prefix + "FilterTapsAtrous",
      .binding = rows.taps_atrous,
      .value_type = SettingValueType::INTEGER,
      .default_value = rows.taps_atrous_default,
      .label = "Filter Taps (kernel side)",
      .section = rows.section,
      .tooltip = "Side length of the square kernel, 3 to 9 (taps x taps samples per pass). Odd values are used as they are; an even value is rounded up. Binomial weights: 3 = 1 2 1, 5 = 1 4 6 4 1, 7 = 1 6 15 20 15 6 1, 9 = 1 8 28 56 70 56 28 8 1.",
      .min = static_cast<float>(rtao::kFilterTapsMin),
      .max = static_cast<float>(rtao::kFilterAtrousTapsMax),
      .format = "%d",
      .is_visible = [visible, filter_type = rows.filter_type]() { return visible() && *filter_type > 0.5f; },
  });
}

inline void AddSettings(renodx::utils::settings::Settings* settings, bool supported, std::function<bool()> advanced_visible = nullptr) {
  if (settings == nullptr) return;
  g_state.supported = supported && IsDevkitPresent();
  if (!g_state.supported) return;

  using Setting = renodx::utils::settings::Setting;
  using SettingValueType = renodx::utils::settings::SettingValueType;
  const std::function<bool()> visible = []() { return g_state.supported; };
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
      .tooltip = "Ray-traced ambient occlusion from the live BVH. Overrides GTVBAO and VBGI while on. Turning it on enables Live BVH and Deforming meshes. Turning it off also turns RTGI off.",
      .labels = {"Off", "On"},
      // on_change_value runs after the new value is written (on_change runs before it, with the old value).
      .on_change_value = [](float, float current) {
        if (current > 0.5f) {
          bvh::RtaoEnableBvhInputs();
        } else if (rtgi::g_rtgi_enabled > 0.5f) {
          renodx::utils::settings::UpdateSetting("RtgiEnabled", 0.f);
        }
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
      .default_value = 0.05f,
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
      .key = "RtaoTraceResolution",
      .binding = &rtao::g_rtao_trace_resolution,
      .value_type = SettingValueType::INTEGER,
      .default_value = 0.f,
      .label = "Trace Resolution",
      .section = "RTAO",
      .tooltip = "Traces RTAO at full, half or quarter resolution, then upsamples to full size before the temporal pass. Full = unchanged.",
      .labels = {"Full", "Half", "Quarter"},
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtaoRayMax",
      .binding = &rtao::g_rtao_ray_max,
      .value_type = SettingValueType::FLOAT,
      .default_value = 0.8f,
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
      .default_value = 0.0040f,
      .label = "Normal Bias (RTAO and RTGI)",
      .section = "RTAO",
      .tooltip = "Ray origin offset along the surface normal, in metres. Used by RTAO and RTGI.",
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
      .key = "RtaoFoliageSkip",
      .binding = &rtao::g_rtao_foliage_skip,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 1.f,
      .label = "Foliage skip",
      .section = "RTAO",
      .tooltip = "RTAO rays pass through wind-driven foliage (instances drawn by a wind-sway vertex shader, recognised from the shader bytecode) and alpha-tested meshes, including fences. Static vegetation is pending. Off = unchanged.",
      .labels = {"Off", "On"},
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtaoTraceCharacters",
      .binding = &rtao::g_rtao_trace_characters,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 0.f,
      .label = "Trace characters",
      .section = "RTAO",
      .tooltip = "RTAO rays hit the characters the game camera draws this frame (skinned meshes from 'Deforming meshes'; water is not traced). Characters then occlude the world. Characters the camera does not draw this frame are not traced. Off = unchanged.",
      .labels = {"Off", "On"},
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtaoCharacterIntensity",
      .binding = &rtao::g_rtao_char_intensity,
      .value_type = SettingValueType::FLOAT,
      .default_value = 0.f,
      .label = "Character Intensity",
      .section = "RTAO",
      .tooltip = "RTAO on character pixels: 1 = full RTAO, 0 = none. Applies whatever GTVBAO 'Allow GTVBAO' says (RTAO replaces GTVBAO; the GTVBAO character settings do not apply while RTAO runs). Character parts the game shades with the map AO keep the game's own AO tint at any value.",
      .min = 0.f,
      .max = 1.f,
      .format = "%.2f",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtaoMaxDarkening",
      .binding = &rtao::g_rtao_max_darkening,
      .value_type = SettingValueType::FLOAT,
      .default_value = 0.5f,
      .label = "Max Darkening",
      .section = "RTAO",
      .tooltip = "Caps the darkest RTAO: no pixel goes below 1 - Max Darkening; lighter pixels are unchanged. 1 = no cap. Micro shadows read the uncapped AO.",
      .min = 0.f,
      .max = 1.f,
      .format = "%.2f",
      .is_visible = visible,
  });
  AddTemporalRows(settings, visible, {.prefix = "Rtao",
                                      .section = "Temporal Accumulation",
                                      .enabled_tooltip = "Reprojects the previous frame AO with the game motion (RTV4 target) and blends it in after validation. Resets on camera cuts, frame gaps and parameter changes.",
                                      .enabled = &rtao::g_rtao_temporal_enabled,
                                      .enabled_default = 1.f,
                                      .history_weight = &rtao::g_rtao_history_weight,
                                      .history_weight_default = 0.9f,
                                      .depth_rejection = &rtao::g_rtao_depth_rejection,
                                      .depth_rejection_default = 0.200f,
                                      .normal_rejection = &rtao::g_rtao_normal_rejection,
                                      .normal_rejection_default = 0.07f,
                                      .history_clamp = &rtao::g_rtao_history_clamp,
                                      .history_clamp_default = 0.f});
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
  AddSpatialRows(settings, visible, {.prefix = "Rtao",
                                     .section = "Denoising",
                                     .enabled_tooltip = "Denoises the final RTAO output with edge-aware weights (depth plane and normal). The history keeps the unfiltered AO. Off = no filter, unchanged output.",
                                     .enabled = &rtao::g_rtao_spatial_enabled,
                                     .enabled_default = 1.f,
                                     .filter_type = &rtao::g_rtao_filter_type,
                                     .filter_type_default = 0.f,
                                     .filter_radius = &rtao::g_rtao_filter_radius,
                                     .filter_radius_default = 8.f,
                                     .filter_passes = &rtao::g_rtao_filter_passes,
                                     .filter_passes_default = 2.f,
                                     .taps_separable = &rtao::g_rtao_filter_taps_separable,
                                     .taps_separable_default = 8.f,
                                     .taps_atrous = &rtao::g_rtao_filter_taps_atrous,
                                     .taps_atrous_default = 8.f});
  settings->push_back(new Setting{
      .key = "RtaoFadeStartPct",
      .binding = &rtao::g_rtao_fade_start_pct,
      .value_type = SettingValueType::FLOAT,
      .default_value = 0.f,
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
      .default_value = 100.f,
      .label = "Fade End",
      .section = "Distance Fade",
      .tooltip = "Where RTAO is fully faded out, as a percent of the BVH range (Region). Capped by the built BVH range at use; the panel shows the effective metres.",
      .min = 0.f,
      .max = 100.f,
      .format = "%.1f%%",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtgiEnabled",
      .binding = &rtgi::g_rtgi_enabled,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 0.f,
      .label = "RTGI",
      .section = "RTGI",
      .tooltip = "Ray-traced diffuse bounce light (one bounce) from the live BVH. Needs RTAO: turning RTGI on also turns RTAO on, and turning it off puts RTAO back to what it was before. Where the bounce is added, RTAO Max Darkening is 1.8x (at most 1); RTAO is otherwise unchanged. On Sora 2nd the bounce (final GI x the surface colour x GI Intensity) is added after the shadow terms, so RTAO and the shadow terms never darken it; fog does not apply to it.",
      .labels = {"Off", "On"},
      .on_change_value = [](float, float current) {
        if (current > 0.5f) {
          renodx::utils::settings::UpdateSetting("RtgiRtaoRestore", rtao::g_rtao_enabled);
          if (!rtao::RtaoRequested()) {
            renodx::utils::settings::UpdateSetting("RtaoEnabled", 1.f);
            bvh::RtaoEnableBvhInputs();  // UpdateSetting runs no callbacks, so RTAO's own call does not happen
          }
        } else {
          renodx::utils::settings::UpdateSetting("RtaoEnabled", rtgi::g_rtgi_rtao_restore);
        }
      },
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtgiRtaoRestore",
      .binding = &rtgi::g_rtgi_rtao_restore,
      .value_type = SettingValueType::FLOAT,
      .default_value = 0.f,
      .can_reset = false,
      .label = "RTGI: RTAO value to restore",
      .section = "RTGI",
      .is_visible = []() { return false; },
  });
  settings->push_back(new Setting{
      .key = "RtgiStatus",
      .value_type = SettingValueType::CUSTOM,
      .can_reset = false,
      .label = "RTGI status",
      .section = "RTGI",
      .on_draw = []() {
        rtgi::DrawRtgiPanel();
        return false;
      },
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtgiDebug",
      .binding = &rtgi::g_rtgi_debug,
      .value_type = SettingValueType::INTEGER,
      .default_value = 0.f,
      .label = "RTGI Debug",
      .section = "RTGI",
      .tooltip = "Views 1 to 9 are drawn over the whole image when the frame is presented, while RTGI runs and no BVH GPU debug mode is selected. Off = normal image. 1 hit classes: red = off-screen hits, green = on-screen hits, blue = hits hidden behind other geometry on screen, black = misses (fractions of the samples). 2 hit distance: mean hit distance / effective Max Ray Distance (white = far), black = no hit. Dark purple = not traced (sky, invalid normal, scaled, outside the region or beyond Fade End). 3 raw GI: this frame's traced bounce radiance before any denoising, shown as rgb / (1 + rgb). 4 accumulated GI after the temporal pass (needs RTGI Temporal on). 5 history confidence = valid history fraction (needs RTGI Temporal on). 6 final GI after the filter and the upsample, as the composite reads it. 7 applied GI: the bounce the Sora 2nd composite adds (final GI, after GI Saturation, x the surface colour x GI Intensity, Character GI Intensity on character shading pixels), shown as rgb / (1 + rgb); black at GI Intensity 0, dark purple where the surface colour was not captured this frame. The image underneath and the next frame's GI are not affected. 8 pixel classes: grey = scaled frame, dark blue = sky, magenta = no G-buffer normal, dark green = outside the BVH region, dark purple = beyond Fade End, yellow = traced but the normal faces away from the camera, cyan = traced with a hit within 1 cm, black = traced with neither. 9 sun GI estimate: the sun part of each pixel's bounce as the Sun Contribution estimate reads it (surface colour x sun light x the sun diffuse factor x the screen shadow, minus the shadow floor), shown as rgb / (1 + rgb); bright = sunlit surfaces. Sun Contribution is not applied; dark purple = not traced.",
      .labels = {"Off", "Hit classes", "Hit distance", "Raw GI", "Accumulated GI", "History confidence", "Final GI", "Applied GI", "Pixel classes", "Sun GI estimate"},
      .is_visible = advanced,
  });
  settings->push_back(new Setting{
      .key = "RtgiIntensity",
      .binding = &rtgi::g_rtgi_intensity,
      .value_type = SettingValueType::FLOAT,
      .default_value = 1.f,
      .label = "GI Intensity",
      .section = "RTGI",
      .tooltip = "Scales the bounce light added to the image (final GI x the surface colour), Sora 2nd. 0 = nothing is added: the image equals RTGI off.",
      .min = 0.f,
      .max = 2.f,
      .format = "%.2f",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtgiSaturation",
      .binding = &rtgi::g_rtgi_saturation,
      .value_type = SettingValueType::FLOAT,
      .default_value = 1.f,
      .label = "GI Saturation",
      .section = "RTGI",
      .tooltip = "Saturation of the bounce light RTGI adds (the diffuse GI that GI Intensity scales). 1 = unchanged, 0 = grey bounce light, 2 = more saturated. Surface colours, the game's own lighting, ambient and highlights are not changed. Also shown in RTGI Debug 6 and 7.",
      .min = 0.f,
      .max = 2.f,
      .format = "%.2f",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtgiCharacterIntensity",
      .binding = &rtgi::g_rtgi_char_intensity,
      .value_type = SettingValueType::FLOAT,
      .default_value = 1.f,
      .label = "Character GI Intensity",
      .section = "RTGI",
      .tooltip = "Bounce light on character pixels the game shades with its character path: 1 = full, 0 = none. Character parts the game shades with the map AO get the full bounce at any value, as with RTAO's Character Intensity.",
      .min = 0.f,
      .max = 1.f,
      .format = "%.2f",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtgiTraceCharacters",
      .binding = &rtgi::g_rtgi_trace_characters,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 0.f,
      .label = "Trace characters",
      .section = "RTGI",
      .tooltip = "RTGI rays hit the characters the game camera draws this frame (water is not traced): they block bounce light. Off = unchanged.",
      .labels = {"Off", "On"},
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtgiSamples",
      .binding = &rtgi::g_rtgi_samples,
      .value_type = SettingValueType::INTEGER,
      .default_value = 1.f,
      .label = "Samples Per Pixel",
      .section = "RTGI",
      .tooltip = "Cosine-hemisphere GI rays per traced pixel per frame.",
      .min = 1.f,
      .max = 4.f,
      .format = "%d",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtgiRayMax",
      .binding = &rtgi::g_rtgi_ray_max,
      .value_type = SettingValueType::FLOAT,
      .default_value = 8.f,
      .label = "Max Ray Distance",
      .section = "RTGI",
      .tooltip = "GI ray length in metres. Capped at use so no traced pixel reaches the BVH region edge: effective = min(this, BVH range + 10 m - effective Fade End). The panel shows both.",
      .min = 1.f,
      .max = 64.f,
      .format = "%.1f",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtgiTraceResolution",
      .binding = &rtgi::g_rtgi_trace_resolution,
      .value_type = SettingValueType::INTEGER,
      .default_value = 1.f,
      .label = "Trace Resolution",
      .section = "RTGI",
      .tooltip = "Resolution of the GI trace: Full = one trace pixel per pixel, Half = one per 2 x 2 block, Quarter = one per 4 x 4 block.",
      .labels = {"Full", "Half", "Quarter"},
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtgiRadianceClamp",
      .binding = &rtgi::g_rtgi_radiance_clamp,
      .value_type = SettingValueType::FLOAT,
      .default_value = 0.f,
      .label = "Radiance Clamp",
      .section = "RTGI",
      .tooltip = "Caps the brightness (luminance) of each GI sample before it is averaged; the colour is kept. 0 = off. Removes bright speckles from small light sources.",
      .min = 0.f,
      .max = 64.f,
      .format = "%.2f",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtgiAmbientContribution",
      .binding = &rtgi::g_rtgi_ambient,
      .value_type = SettingValueType::FLOAT,
      .default_value = 1.f,
      .label = "Ambient Contribution",
      .section = "RTGI",
      .tooltip = "Weight of the game's probe ambient when a GI ray hits a surface that is not visible on screen (that surface is shaded with the sun, a sun shadow ray and the probe ambient). 0 = sun only. It never adds light where a ray misses.",
      .min = 0.f,
      .max = 2.f,
      .format = "%.2f",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtgiSunShadowDistance",
      .binding = &rtgi::g_rtgi_sun_distance,
      .value_type = SettingValueType::FLOAT,
      .default_value = 64.f,
      .label = "Sun Shadow Distance",
      .section = "RTGI",
      .tooltip = "Length in metres of the sun shadow ray cast from GI ray hits that are not read from the screen (hidden or off-screen surfaces facing the sun). It decides only whether such a surface is sunlit; the probe ambient is never shadowed. Shorter is cheaper, but far occluders (tall buildings, large roofs) then stop shadowing those surfaces.",
      .min = 4.f,
      .max = 64.f,
      .format = "%.1f",
      .is_visible = advanced,
  });
  settings->push_back(new Setting{
      .key = "RtgiSunCastersOnly",
      .binding = &rtgi::g_rtgi_sun_casters_only,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 0.f,
      .label = "Sun Shadows From Shadow Casters Only",
      .section = "RTGI",
      .tooltip = "Sun shadow rays hit only what the game draws into its shadow maps. The game's own sun shadows, indoors and outdoors. Off = unchanged.",
      .labels = {"Off", "On"},
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtgiSunContribution",
      .binding = &rtgi::g_rtgi_sun_contribution,
      .value_type = SettingValueType::FLOAT,
      .default_value = 1.f,
      .label = "Sun Contribution",
      .section = "RTGI",
      .tooltip = "Scales only the RTGI bounce light that comes from the sun: sunlight reflected by surfaces the GI rays hit. The game's own lighting, shadows and the image are not changed. 1 = unchanged, 0 = no sun bounce, 2 = double. Ambient and lamp bounce are not changed. Applies to fallback hits and on-screen hits (Sora 2nd on-screen: the sun part is estimated from this frame's G-buffer).",
      .min = 0.f,
      .max = 2.f,
      .format = "%.2f",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtgiFallbackNoAlbedo",
      .binding = &rtgi::g_rtgi_fallback_no_albedo,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 0.f,
      .label = "Off-screen Hits: No Albedo Guess",
      .section = "RTGI",
      .tooltip = "Hits that RTGI cannot read from the screen (hidden behind other surfaces, off screen, or no screen copy yet) are lit with the sun, the probe ambient and, with Off-screen Dynamic Lights, the lamps, then multiplied by a guessed surface colour of 0.5 grey. On: the guess is not applied (surface colour 1.0), so these hits return the light that reaches them. The sun, ambient and lamp light are computed the same way in both states. Off = unchanged.",
      .labels = {"Off", "On"},
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtgiGameSun",
      .binding = &rtgi::g_rtgi_game_sun,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 0.f,
      .label = "Game-Style Fallback Sun",
      .section = "RTGI",
      .tooltip = "Hits not read from the screen get the game's sun shadow colour as a floor: shadowed and back-facing parts get the scene shadow colour times the sun colour, as the game lights them, instead of no sunlight. The floor is not scaled by Sun Contribution. Off = unchanged. Material shadow colour and cloud shadow are not reproduced.",
      .labels = {"Off", "On"},
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtgiDynamicLights",
      .binding = &rtgi::g_rtgi_dynamic_lights,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 0.f,
      .label = "Off-screen Dynamic Lights",
      .section = "RTGI",
      .tooltip = "Lamps the game has loaded for this view (it loads only lamps that touch the view) add light to hits not read from the screen, such as hidden and off-screen surfaces. Lamps behind the camera are not loaded and are not used. Off = unchanged.",
      .labels = {"Off", "On"},
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtgiDynamicLightContribution",
      .binding = &rtgi::g_rtgi_dynamic_light_contribution,
      .value_type = SettingValueType::FLOAT,
      .default_value = 1.f,
      .label = "Dynamic Light Contribution",
      .section = "RTGI",
      .tooltip = "Scales only the RTGI bounce light that comes from the game's lamps: lamp light reflected by surfaces the GI rays hit. The game's own lighting and the image are not changed. 1 = unchanged. Sun and ambient bounce are not changed. Applies to fallback hits and on-screen hits (Sora 2nd on-screen: the lamp part is estimated from this frame's G-buffer). On-screen hits need Off-screen Dynamic Lights On.",
      .min = 0.f,
      .max = 2.f,
      .format = "%.2f",
      .is_visible = visible,
  });
  AddTemporalRows(settings, visible, {.prefix = "Rtgi",
                                      .section = "RTGI Temporal Accumulation",
                                      .enabled_tooltip = "Reprojects the previous frame GI with the game motion (RTV4 target) and blends it in after validation. Resets on camera cuts, frame gaps and parameter changes.",
                                      .enabled = &rtgi::g_rtgi_temporal_enabled,
                                      .enabled_default = 1.f,
                                      .history_weight = &rtgi::g_rtgi_history_weight,
                                      .history_weight_default = 0.95f,
                                      .depth_rejection = &rtgi::g_rtgi_depth_rejection,
                                      .depth_rejection_default = 0.05f,
                                      .normal_rejection = &rtgi::g_rtgi_normal_rejection,
                                      .normal_rejection_default = 0.9f,
                                      .history_clamp = &rtgi::g_rtgi_history_clamp,
                                      .history_clamp_default = 1.f});
  AddSpatialRows(settings, visible, {.prefix = "Rtgi",
                                     .section = "RTGI Denoising",
                                     .enabled_tooltip = "Denoises the RTGI output at trace resolution with edge-aware weights (depth plane and normal), before the upsample. The history keeps the unfiltered GI. Off = no filter.",
                                     .enabled = &rtgi::g_rtgi_spatial_enabled,
                                     .enabled_default = 1.f,
                                     .filter_type = &rtgi::g_rtgi_filter_type,
                                     .filter_type_default = 1.f,
                                     .filter_radius = &rtgi::g_rtgi_filter_radius,
                                     .filter_radius_default = 4.f,
                                     .filter_passes = &rtgi::g_rtgi_filter_passes,
                                     .filter_passes_default = 2.f,
                                     .taps_separable = &rtgi::g_rtgi_filter_taps_separable,
                                     .taps_separable_default = 5.f,
                                     .taps_atrous = &rtgi::g_rtgi_filter_taps_atrous,
                                     .taps_atrous_default = 5.f});
  settings->push_back(new Setting{
      .key = "RtgiFadeStartPct",
      .binding = &rtgi::g_rtgi_fade_start_pct,
      .value_type = SettingValueType::FLOAT,
      .default_value = 10.f,
      .label = "Fade Start",
      .section = "RTGI Distance Fade",
      .tooltip = "Where RTGI starts fading out, as a percent of the BVH range (Region). Capped by the built BVH range at use; the panel shows the effective metres.",
      .min = 0.f,
      .max = 100.f,
      .format = "%.1f%%",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtgiFadeEndPct",
      .binding = &rtgi::g_rtgi_fade_end_pct,
      .value_type = SettingValueType::FLOAT,
      .default_value = 20.f,
      .label = "Fade End",
      .section = "RTGI Distance Fade",
      .tooltip = "Where RTGI is fully faded out (no GI rays beyond it), as a percent of the BVH range (Region). Capped by the built BVH range at use; the panel shows the effective metres.",
      .min = 0.f,
      .max = 100.f,
      .format = "%.1f%%",
      .is_visible = visible,
  });
}

}  // namespace falcom_world
