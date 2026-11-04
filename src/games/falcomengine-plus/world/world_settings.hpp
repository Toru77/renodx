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

#include <deps/imgui/imgui.h>

#include "world_state.hpp"
#include "bvh/world_bvh.hpp"
#include "rr/rr_state.hpp"
#include "rtao/rtao.hpp"
#include "rtao/rtao_panel.hpp"
#include "rtgi/rtgi.hpp"
#include "rtgi/rtgi_panel.hpp"
#include "rtls/rtls.hpp"
#include "rtls/rtls_panel.hpp"
#include "rtsun/rtsun_panel.hpp"
#include "capture/draw_census.hpp"

namespace falcom_world {

inline bool IsDevkitPresent() {
  if (IsDevkitLoaded()) return true;
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

// Adds the SVGF rows of RTAO, GTVBAO, RTGI, GTVBGI, RT Local Shadows and RT Sun: Temporal Accumulation (the reprojection,
// paper Sec. 4.1) and Denoising (Iterations, the a-trous count of Sec. 4.3; Luminance Edge Stop, sigma_l of Sec. 4.4). The
// sigma_z and sigma_n values are fixed by the paper and not shown. A plain function (MSVC C2326).
struct SvgfRows {
  const char* prefix;  // key prefix: "Rtao", "Gtvbao", "Rtgi", "Vbgi", "Rtls" or "RtSun"
  const char* temporal_section;
  const char* temporal_enabled_tooltip;
  float* temporal_enabled;
  float* history_weight;
  float* depth_rejection;
  float* normal_rejection;
  const char* spatial_section;
  const char* spatial_enabled_tooltip;
  float* spatial_enabled;
  float* iterations;
  float* luminance_edge_stop;
  float* half_res_iterations = nullptr;  // GTVBAO only: Half Res Iterations row; Iterations then applies at Full only
};
inline void AddSvgfRows(renodx::utils::settings::Settings* settings, const std::function<bool()>& visible,
                        const std::function<bool()>& advanced, const SvgfRows& rows) {
  using Setting = renodx::utils::settings::Setting;
  using SettingValueType = renodx::utils::settings::SettingValueType;
  const std::string prefix = rows.prefix;
  settings->push_back(new Setting{
      .key = prefix + "TemporalEnabled",
      .binding = rows.temporal_enabled,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 1.f,
      .label = "Temporal Accumulation",
      .section = rows.temporal_section,
      .tooltip = rows.temporal_enabled_tooltip,
      .labels = {"Off", "On"},
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = prefix + "HistoryWeight",
      .binding = rows.history_weight,
      .value_type = SettingValueType::FLOAT,
      .default_value = 0.8f,
      .label = "History Weight",
      .section = rows.temporal_section,
      .tooltip = "Weight of the reprojected history: alpha = 1 - History Weight (0 to 0.99). Higher = smoother, slower to follow changes.",
      .min = 0.f,
      .max = 0.99f,
      .format = "%.2f",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = prefix + "DepthRejection",
      .binding = rows.depth_rejection,
      .value_type = SettingValueType::FLOAT,
      .default_value = 0.2f,
      .label = "Depth Rejection",
      .section = rows.temporal_section,
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
      .default_value = 0.f,
      .label = "Normal Rejection",
      .section = rows.temporal_section,
      .tooltip = "Binary normal test: a history tap counts only when the dot product of its normal with the current normal is at or above this value (0 accepts any normal facing the same way).",
      .min = 0.f,
      .max = 1.f,
      .format = "%.2f",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = prefix + "SpatialEnabled",
      .binding = rows.spatial_enabled,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 1.f,
      .label = "Spatial Filter",
      .section = rows.spatial_section,
      .tooltip = rows.spatial_enabled_tooltip,
      .labels = {"Off", "On"},
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = prefix + "SvgfIterations",
      .binding = rows.iterations,
      .value_type = SettingValueType::INTEGER,
      .default_value = 5.f,
      .label = rows.half_res_iterations != nullptr ? "Full Res Iterations" : "Iterations",
      .section = rows.spatial_section,
      .tooltip = rows.half_res_iterations != nullptr ? "A-trous iterations of the SVGF filter at Full resolution (1 to 5). Half resolution uses Half Res Iterations." : "A-trous iterations of the SVGF filter (1 to 5). The effective count is this minus log2 of the trace divisor (Half: minus 1, Quarter: minus 2), at least 1; the panel shows the effective count.",
      .min = 1.f,
      .max = 5.f,
      .format = "%d",
      .is_visible = visible,
  });
  if (rows.half_res_iterations != nullptr) {
    settings->push_back(new Setting{
        .key = prefix + "SvgfHalfResIterations",
        .binding = rows.half_res_iterations,
        .value_type = SettingValueType::INTEGER,
        .default_value = 4.f,
        .label = "Half Res Iterations",
        .section = rows.spatial_section,
        .tooltip = "A-trous iterations of the SVGF filter at Half resolution (1 to 4). Each iteration doubles the step, so 4 at Half covers the same screen area as 5 at Full.",
        .min = 1.f,
        .max = 4.f,
        .format = "%d",
        .is_visible = visible,
    });
  }
  settings->push_back(new Setting{
      .key = prefix + "SvgfLuminanceEdgeStop",
      .binding = rows.luminance_edge_stop,
      .value_type = SettingValueType::FLOAT,
      .default_value = 4.f,
      .label = "Luminance Edge Stop",
      .section = rows.spatial_section,
      .tooltip = "Sigma of the luminance edge stop, in units of the local luminance standard deviation (0.5 to 16). Lower = sharper edges, more noise; higher = smoother, more blur across edges.",
      .min = 0.5f,
      .max = 16.f,
      .format = "%.1f",
      .is_visible = [visible, advanced]() { return visible() && advanced(); },
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
  settings->push_back(new Setting{
      .key = "RrEnable",
      .binding = &rr::g_rr_enabled,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 1.f,
      .label = "DLSS Ray Reconstruction",
      .section = "Ray Tracing",
      .tooltip = "Replaces the game's DLSS Super Resolution with DLSS Ray Reconstruction (preset F) as the denoiser of the ray-traced effects. Needs an NVIDIA RTX GPU and DLSS selected in the game's graphics options; on other GPUs it stays off. The RT, GTVBAO and GTVBGI denoisers stay on in this build.",
      .labels = {"Off", "On"},
      .is_enabled = []() { return rr::g_rr_gpu_nvidia.load() == 1; },
      .is_visible = [visible]() { return rr::g_rr_perf_rows > 0.5f && visible(); },  // TEMP perf test (remove)
  });
  settings->push_back(new Setting{
      .key = "RrPreset",
      .binding = &rr::g_rr_preset_index,
      .value_type = SettingValueType::INTEGER,
      .default_value = 0.f,
      .label = "RR preset",
      .section = "Ray Tracing",
      .labels = {"Default", "D", "E", "F"},
      .is_enabled = []() { return rr::g_rr_gpu_nvidia.load() == 1; },
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RrDebugLog",
      .binding = &rr::g_rr_debug_log,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 0.f,
      .label = "Ray Reconstruction debug logging",
      .section = "Ray Tracing",
      .labels = {"Off", "On"},
      .is_enabled = []() { return rr::g_rr_gpu_nvidia.load() == 1; },
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RrProbeEnable",
      .binding = &rr::g_rr_probe_enabled,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 0.f,
      .label = "Ray Reconstruction create probe",
      .section = "Ray Tracing",
      .tooltip = "Diagnostic. Creates and releases one DLSS Ray Reconstruction feature per session, at the first DLSS evaluate, to test whether it can be created. Leave Off unless asked to test it.",
      .labels = {"Off", "On"},
      .is_visible = [visible]() { return rr::g_rr_perf_rows > 0.5f && visible(); },  // TEMP perf test (remove)
  });
  settings->push_back(new Setting{  // TEMP perf test (remove)
      .key = "RrPerfPresent",
      .binding = &rr::g_rr_perf_present,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 1.f,
      .label = "[perf test] RR present handler",
      .section = "Ray Tracing",
      .labels = {"Off", "On"},
      .is_visible = visible,
  });
  settings->push_back(new Setting{  // TEMP perf test (remove)
      .key = "RrPerfGuides",
      .binding = &rr::g_rr_perf_guides,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 1.f,
      .label = "[perf test] RR guide pass and snapshot",
      .section = "Ray Tracing",
      .labels = {"Off", "On"},
      .is_visible = visible,
  });
  settings->push_back(new Setting{  // TEMP perf test (remove)
      .key = "RrPerfRows",
      .binding = &rr::g_rr_perf_rows,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 1.f,
      .label = "[perf test] RR settings rows",
      .section = "Ray Tracing",
      .labels = {"Off", "On"},
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "WorldRrStatus",
      .value_type = SettingValueType::CUSTOM,
      .can_reset = false,
      .label = "Ray Reconstruction status",
      .section = "Ray Tracing",
      .on_draw = []() {
        if (rr::g_rr_gpu_nvidia.load() == 0) {
          ImGui::TextDisabled("Ray Reconstruction needs an NVIDIA RTX GPU.");
        } else {
          ImGui::Text("Ray Reconstruction: %s", rr::RrReasonName(rr::g_rr_reason.load()));
          ImGui::Text("Ray Reconstruction guides: %s", rr::GuideReasonName(rr::g_rr_guide_reason.load()));
          if (rr::g_rr_failed.load()) {
            ImGui::Text("NGX result 0x%08X, exception 0x%08X", rr::g_rr_failed_result.load(), rr::g_rr_failed_exception.load());
          }
        }
        return false;
      },
      .is_visible = []() { return rr::g_rr_perf_rows > 0.5f && g_state.supported && (rr::RrRequested() || rr::g_rr_gpu_nvidia.load() == 0); },  // TEMP perf test (remove)
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
      .tooltip = "Ray-traced ambient occlusion from the live BVH. Overrides GTVBAO and VBGI while on. While any RT effect is on the BVH inputs run; with all off nothing runs. Turning it off also turns RTGI off.",
      .labels = {"Off", "On"},
      // on_change_value runs after the new value is written (on_change runs before it, with the old value).
      .on_change_value = [](float, float current) {
        if (current <= 0.5f && rtgi::g_rtgi_enabled > 0.5f) {
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
      .tooltip = "Raw AO = the trace upsampled to full size, without the temporal or spatial filter. Denoised AO = the SVGF output the lighting uses.",
      .labels = {"Off", "Raw AO", "Denoised AO"},
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
      .default_value = 1.f,
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
      .tooltip = "The SVGF denoiser runs at the trace resolution, then the AO is upsampled to full size.",
      .labels = {"Full", "Half", "Quarter"},
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtaoRayMax",
      .binding = &rtao::g_rtao_ray_max,
      .value_type = SettingValueType::FLOAT,
      .default_value = 1.f,
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
      .default_value = 0.01f,
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
      .default_value = 1.f,
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
      .default_value = 0.33f,
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
  AddSvgfRows(settings, visible, advanced, {.prefix = "Rtao",
                                            .temporal_section = "Temporal Accumulation",
                                            .temporal_enabled_tooltip = "Reprojects the previous frame RTAO AO with the game motion (RTV4 target) and blends it in after validation (SVGF). Resets on camera cuts, frame gaps and parameter changes.",
                                            .temporal_enabled = &rtao::g_rtao_temporal_enabled,
                                            .history_weight = &rtao::g_rtao_history_weight,
                                            .depth_rejection = &rtao::g_rtao_depth_rejection,
                                            .normal_rejection = &rtao::g_rtao_normal_rejection,
                                            .spatial_section = "Denoising",
                                            .spatial_enabled_tooltip = "Denoises the RTAO AO with the SVGF a-trous filter (depth, normal and AO edge stops). Off = no filter.",
                                            .spatial_enabled = &rtao::g_rtao_spatial_enabled,
                                            .iterations = &rtao::g_rtao_svgf_iterations,
                                            .luminance_edge_stop = &rtao::g_rtao_svgf_luminance_edge_stop});
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
      .key = "RtaoFadeStartPct",
      .binding = &rtao::g_rtao_fade_start_pct,
      .value_type = SettingValueType::FLOAT,
      .default_value = 15.f,
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
      .default_value = 75.f,
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
      .tooltip = "Ray-traced diffuse bounce light (one bounce) from the live BVH. Needs RTAO: turning RTGI on also turns RTAO on, and turning it off puts RTAO back to what it was before. Where the bounce is added, RTAO Max Darkening is 1.8x (at most 1); RTAO is otherwise unchanged. On Sora 2nd the bounce (final GI x the surface colour x GI Intensity) is added after the shadow terms, so RTAO and the shadow terms never darken it; RTGI is attenuated by the fog and haze the scene gets.",
      .labels = {"Off", "On"},
      .on_change_value = [](float, float current) {
        if (current > 0.5f) {
          renodx::utils::settings::UpdateSetting("RtgiRtaoRestore", rtao::g_rtao_enabled);
          if (!rtao::RtaoRequested()) {
            renodx::utils::settings::UpdateSetting("RtaoEnabled", 1.f);
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
      .tooltip = "Views 1 to 11 are drawn over the whole image when the frame is presented, while RTGI runs and no BVH GPU debug mode is selected. Off = normal image. 1 hit classes: red = off-screen hits, green = on-screen hits, blue = hits hidden behind other geometry on screen, black = misses (fractions of the samples). 2 hit distance: mean hit distance / effective Max Ray Distance (white = far), black = no hit. Dark purple = not traced (sky, invalid normal, scaled, outside the region or beyond Fade End). 3 raw GI: this frame's traced bounce radiance before any denoising, shown as rgb / (1 + rgb). 4 accumulated GI after the temporal pass (needs RTGI Temporal on). 5 history confidence = valid history fraction (needs RTGI Temporal on). 6 final GI after the filter and the upsample, as the composite reads it. 7 applied GI: the bounce the Sora 2nd composite adds (final GI, after GI Saturation, x the surface colour x GI Intensity, Character GI Intensity on character shading pixels), shown as rgb / (1 + rgb); black at GI Intensity 0, dark purple where the surface colour was not captured this frame. The image underneath and the next frame's GI are not affected. 8 pixel classes: grey = scaled frame, dark blue = sky, magenta = no G-buffer normal, dark green = outside the BVH region, dark purple = beyond Fade End, yellow = traced but the normal faces away from the camera, cyan = traced with a hit within 1 cm, black = traced with neither. 9 Sun GI estimate: the sun bounce this pixel would give (surface colour x sun colour x the game's stored sun factor x the screen shadow mask), as rgb / (1 + rgb); black where the sun inputs are not captured. 10 Extra bounce light: only the light Extra Bounces adds (Feedback: the fed-back part; Real Extra Rays: bounces 1 and 2), after Bounce Saturation, raw and at trace resolution, as rgb / (1 + rgb) like view 3. Black with Extra Bounces Off. Not-traced pixels are dark purple, as the other views do. Compare with view 3 (all bounce light) and view 7 (what is added to the image). 11 SVGF variance (needs RTGI Denoising on): grey = sigma / (1 + sigma) of the luminance variance the a-trous filter steers by; blue = 1 where the history is shorter than 4 frames.",
      .labels = {"Off", "Hit classes", "Hit distance", "Raw GI", "Accumulated GI", "History confidence", "Final GI", "Applied GI", "Pixel classes", "Sun GI estimate", "Extra bounce light", "SVGF variance"},
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
      .key = "RtgiExtraBounces",
      .binding = &rtgi::g_rtgi_extra_bounces,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 1.f,
      .label = "Extra Bounces",
      .section = "RTGI",
      .tooltip = "Real Extra Rays: traces extra bounce rays (Extra Bounce Rays, 1 or 2) per first-bounce hit, which also reach off-screen hits through the fallback shading. More expensive; compare cost in the Performance tab. Off = no extra rays. Feedback is always on.",
      .labels = {"Off", "On"},
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtgiFeedbackStrength",
      .binding = &rtgi::g_rtgi_feedback_strength,
      .value_type = SettingValueType::FLOAT,
      .default_value = 0.5f,
      .label = "Feedback Strength",
      .section = "RTGI",
      .tooltip = "Always on, with or without Extra Bounces: feeds back last frame's bounce from the screen, so light bounces more than once over several frames. Only surfaces visible on screen; does nothing on a frame when last frame's bounce was not applied. How much is fed back (0 = none, 1 = full). Automatically limited so that Feedback Strength x GI Intensity stays at or below 0.9, to keep the image stable.",
      .min = 0.f,
      .max = 1.f,
      .format = "%.2f",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtgiExtraBounceRays",
      .binding = &rtgi::g_rtgi_extra_bounce_rays,
      .value_type = SettingValueType::INTEGER,
      .default_value = 1.f,
      .label = "Extra Bounce Rays",
      .section = "RTGI",
      .tooltip = "Number of extra bounce rays per first-bounce hit (1 or 2), used while Extra Bounces is On. Each adds roughly the cost of today's RTGI trace.",
      .min = 1.f,
      .max = 2.f,
      .format = "%d",
      .is_visible = [visible]() { return visible() && rtgi::g_rtgi_extra_bounces > 0.5f; },
  });
  settings->push_back(new Setting{
      .key = "RtgiBounceSaturation",
      .binding = &rtgi::g_rtgi_bounce_saturation,
      .value_type = SettingValueType::FLOAT,
      .default_value = 1.f,
      .label = "Bounce Saturation",
      .section = "RTGI",
      .tooltip = "Saturation of only the extra bounce light (Feedback: the fed-back part; Real Extra Rays: bounces 1 and 2). 1 = unchanged, 0 = grey extra light, 2 = more saturated. The first bounce, GI Saturation and the game's lighting are not changed by this setting. Does nothing with Extra Bounces Off.",
      .min = 0.f,
      .max = 2.f,
      .format = "%.2f",
      .is_visible = [visible]() { return visible() && rtgi::g_rtgi_extra_bounces > 0.5f; },
  });
  settings->push_back(new Setting{
      .key = "RtgiCharacterIntensity",
      .binding = &rtgi::g_rtgi_char_intensity,
      .value_type = SettingValueType::FLOAT,
      .default_value = 0.5f,
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
      .default_value = 1.f,
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
      .default_value = 16.f,
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
      .default_value = 2.f,
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
      .default_value = 0.f,
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
      .default_value = 1.f,
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
      .default_value = 1.f,
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
      .default_value = 1.f,
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
  settings->push_back(new Setting{
      .key = "RtgiLampShadows",
      .binding = &rtgi::g_rtgi_lamp_shadows,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 1.f,
      .label = "Off-screen Lamp Shadows",
      .section = "RTGI",
      .tooltip = "With Off-screen Dynamic Lights on: hidden and off-screen GI hits trace one shadow ray per lamp, so lamps behind walls no longer light them. The ray stops Light Clearance (RT Local Shadows) before the lamp. Off = unchanged.",
      .labels = {"Off", "On"},
      .is_enabled = []() { return rtgi::g_rtgi_dynamic_lights > 0.5f; },
      .is_visible = visible,
  });
  AddSvgfRows(settings, visible, advanced, {.prefix = "Rtgi",
                                            .temporal_section = "RTGI Temporal Accumulation",
                                            .temporal_enabled_tooltip = "Reprojects the previous frame GI with the game motion (RTV4 target) and blends it in after validation. Resets on camera cuts, frame gaps and parameter changes.",
                                            .temporal_enabled = &rtgi::g_rtgi_temporal_enabled,
                                            .history_weight = &rtgi::g_rtgi_history_weight,
                                            .depth_rejection = &rtgi::g_rtgi_depth_rejection,
                                            .normal_rejection = &rtgi::g_rtgi_normal_rejection,
                                            .spatial_section = "RTGI Denoising",
                                            .spatial_enabled_tooltip = "Denoises the RTGI output at trace resolution with edge-aware weights (depth plane and normal), before the upsample. The history keeps the unfiltered GI. Off = no filter.",
                                            .spatial_enabled = &rtgi::g_rtgi_spatial_enabled,
                                            .iterations = &rtgi::g_rtgi_svgf_iterations,
                                            .luminance_edge_stop = &rtgi::g_rtgi_svgf_luminance_edge_stop});
  settings->push_back(new Setting{
      .key = "RtgiFadeStartPct",
      .binding = &rtgi::g_rtgi_fade_start_pct,
      .value_type = SettingValueType::FLOAT,
      .default_value = 15.f,
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
      .default_value = 75.f,
      .label = "Fade End",
      .section = "RTGI Distance Fade",
      .tooltip = "Where RTGI is fully faded out (no GI rays beyond it), as a percent of the BVH range (Region). Capped by the built BVH range at use; the panel shows the effective metres.",
      .min = 0.f,
      .max = 100.f,
      .format = "%.1f%%",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtlsEnabled",
      .binding = &rtls::g_rtls_enabled,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 0.f,
      .label = "RT Local Shadows",
      .section = "RT Local Shadows",
      .tooltip = "Ray-traced shadows from the game's local lamps (point and spot) using the live BVH. Independent of RTAO and RTGI. While any RT effect is on the BVH inputs run; with all off nothing runs. Applied to the image after temporal accumulation, the spatial filter and the upsample. While on, Contact Shadows Local Lights is greyed out and off, and the game's spot shadow maps are replaced inside the RT range (they return beyond Fade End).",
      .labels = {"Off", "On"},
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtlsStatus",
      .value_type = SettingValueType::CUSTOM,
      .can_reset = false,
      .label = "RTLS status",
      .section = "RT Local Shadows",
      .on_draw = []() {
        rtls::DrawRtlsPanel();
        return false;
      },
      .is_visible = visible,
  });
  // Session-only (empty key): not saved.
  settings->push_back(new Setting{
      .key = "",
      .binding = &rtls::g_rtls_debug,
      .value_type = SettingValueType::INTEGER,
      .default_value = 0.f,
      .label = "RTLS Debug",
      .section = "RT Local Shadows",
      .tooltip = "Session only (not saved). 1 shadow term (red point, green spot, blue coverage), 2 lamps lighting the pixel (white = 10), 3 red = lamps over the budget, green = lamps traced, 4 red = rays blocked within 0.5 m of the lamp, green = all blocked rays, 5 pixel classes (grey scaled, dark blue sky, magenta no normal, olive character, dark green outside the region, dark purple beyond Fade End, white traced with lamps, black traced without). Dark purple = not traced. 6 accumulated term after the temporal pass, 7 history confidence (both need RTLS Temporal on), 8 final term after the filter and the upsample (the value t38 holds); 6 and 8 show x / (1 + x) of the shadow term, so a full shadow reads 0.5. 9 SVGF variance (needs RT Local Shadows Denoising on): grey = sigma / (1 + sigma) of the luminance variance the a-trous filter steers by; blue = 1 where the history is shorter than 4 frames.",
      .labels = {"Off", "Raw shadow", "Lamps per pixel", "Traced / over budget", "Blocked near lamp", "Pixel classes", "Accumulated", "History confidence", "Final", "SVGF variance"},
      .is_visible = advanced,
  });
  settings->push_back(new Setting{
      .key = "RtlsPoint",
      .binding = &rtls::g_rtls_point,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 1.f,
      .label = "Point",
      .section = "RT Local Shadows",
      .tooltip = "Traces shadows from point lamps on surfaces the game shades with lamp specular.",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtlsSpot",
      .binding = &rtls::g_rtls_spot,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 1.f,
      .label = "Spot",
      .section = "RT Local Shadows",
      .tooltip = "Traces shadows from spot lamps on surfaces the game shades with lamp specular.",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtlsEnvPoint",
      .binding = &rtls::g_rtls_env_point,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 1.f,
      .label = "Env Point",
      .section = "RT Local Shadows",
      .tooltip = "Traces shadows from point lamps on surfaces the game shades without lamp specular (Env = surfaces the game shades without lamp specular).",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtlsEnvSpot",
      .binding = &rtls::g_rtls_env_spot,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 1.f,
      .label = "Env Spot",
      .section = "RT Local Shadows",
      .tooltip = "Traces shadows from spot lamps on surfaces the game shades without lamp specular (Env = surfaces the game shades without lamp specular).",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtlsCharacters",
      .binding = &rtls::g_rtls_characters,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 1.f,
      .label = "Receive on characters",
      .section = "RT Local Shadows",
      .tooltip = "Characters get the game's lamp light where RT Local Shadows finds the lamp unblocked (the game gives characters no lamp shadows). Fades out with the RT range. Off = unchanged.",
      .labels = {"Off", "On"},
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtlsCharacterLampLight",
      .binding = &rtls::g_rtls_character_lamp,
      .value_type = SettingValueType::FLOAT,
      .default_value = 0.5f,
      .label = "Character Lamp Light",
      .section = "RT Local Shadows",
      .tooltip = "Strength of the lamp light added to characters. 0 = none.",
      .min = 0.f,
      .max = 2.f,
      .format = "%.2f",
      .is_enabled = []() { return rtls::g_rtls_characters > 0.5f; },
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtlsCharacterNormalBias",
      .binding = &rtls::g_rtls_character_normal_bias,
      .value_type = SettingValueType::FLOAT,
      .default_value = 0.02f,
      .label = "Character Normal Bias",
      .section = "RT Local Shadows",
      .tooltip = "Extra ray offset on character pixels against self-shadowing from their own mesh, in metres.",
      .min = 0.f,
      .max = 0.2f,
      .format = "%.3f",
      .is_enabled = []() { return rtls::g_rtls_characters > 0.5f; },
      .is_visible = advanced,
      .is_logarithmic = true,
  });
  settings->push_back(new Setting{
      .key = "RtlsStrength",
      .binding = &rtls::g_rtls_strength,
      .value_type = SettingValueType::FLOAT,
      .default_value = 1.f,
      .label = "Strength",
      .section = "RT Local Shadows",
      .tooltip = "Scales the RT local shadow. 0 = no RT shadow (spot lamps inside the RT range are then unshadowed), 1 = physical.",
      .min = 0.f,
      .max = 1.f,
      .format = "%.2f",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtlsMaxDarkening",
      .binding = &rtls::g_rtls_max_darkening,
      .value_type = SettingValueType::FLOAT,
      .default_value = 1.f,
      .label = "Max Darkening",
      .section = "RT Local Shadows",
      .tooltip = "Caps the darkest RT local shadow: no lamp loses more than this share of its light; lighter shadows are unchanged. 1 = no cap.",
      .min = 0.f,
      .max = 1.f,
      .format = "%.2f",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtlsSamples",
      .binding = &rtls::g_rtls_samples,
      .value_type = SettingValueType::INTEGER,
      .default_value = 1.f,
      .label = "Samples Per Light",
      .section = "RT Local Shadows",
      .min = 1.f,
      .max = 4.f,
      .format = "%d",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtlsTraceResolution",
      .binding = &rtls::g_rtls_trace_resolution,
      .value_type = SettingValueType::INTEGER,
      .default_value = 1.f,
      .label = "Trace Resolution",
      .section = "RT Local Shadows",
      .tooltip = "Resolution of the RTLS trace: Full = one trace pixel per pixel, Half = one per 2 x 2 block, Quarter = one per 4 x 4 block. The denoiser upsamples to full size.",
      .labels = {"Full", "Half", "Quarter"},
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtlsMaxPointLights",
      .binding = &rtls::g_rtls_max_point_lights,
      .value_type = SettingValueType::INTEGER,
      .default_value = 6.f,
      .label = "Max Point Lights Per Pixel",
      .section = "RT Local Shadows",
      .min = 1.f,
      .max = 10.f,
      .format = "%d",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtlsRayMax",
      .binding = &rtls::g_rtls_ray_max,
      .value_type = SettingValueType::FLOAT,
      .default_value = 16.f,
      .label = "Max Ray Distance",
      .section = "RT Local Shadows",
      .tooltip = "Capped at use so no traced pixel reaches the BVH region edge; the panel shows both.",
      .min = 1.f,
      .max = 64.f,
      .format = "%.1f",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtlsLightSize",
      .binding = &rtls::g_rtls_light_size,
      .value_type = SettingValueType::FLOAT,
      .default_value = 0.1f,
      .label = "Light Size",
      .section = "RT Local Shadows",
      .tooltip = "Radius of the lamp for soft shadows; 0 = hard.",
      .min = 0.f,
      .max = 1.f,
      .format = "%.2f",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtlsLightClearance",
      .binding = &rtls::g_rtls_light_clearance,
      .value_type = SettingValueType::FLOAT,
      .default_value = 0.2f,
      .label = "Light Clearance",
      .section = "RT Local Shadows",
      .tooltip = "Rays stop this far before the lamp so its own housing does not block it.",
      .min = 0.f,
      .max = 1.f,
      .format = "%.2f",
      .is_visible = advanced,
  });
  settings->push_back(new Setting{
      .key = "RtlsNormalBias",
      .binding = &rtls::g_rtls_normal_bias,
      .value_type = SettingValueType::FLOAT,
      .default_value = 0.004f,
      .label = "Normal Bias (RTLS)",
      .section = "RT Local Shadows",
      .min = 0.f,
      .max = 0.5f,
      .format = "%.4f",
      .is_visible = visible,
      .is_logarithmic = true,
  });
  settings->push_back(new Setting{
      .key = "RtlsTraceCharacters",
      .binding = &rtls::g_rtls_trace_characters,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 1.f,
      .label = "Trace characters",
      .section = "RT Local Shadows",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtlsIsfast",
      .binding = &rtls::g_rtls_isfast,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 1.f,
      .label = "IS-FAST Noise",
      .section = "RT Local Shadows",
      .is_visible = visible,
  });
  AddSvgfRows(settings, visible, advanced, {.prefix = "Rtls",
                                            .temporal_section = "RTLS Temporal Accumulation",
                                            .temporal_enabled_tooltip = "Reprojects the previous frame's RT local shadow term with the game motion (RTV4 target) and blends it in after validation. Resets on camera cuts, frame gaps and parameter changes.",
                                            .temporal_enabled = &rtls::g_rtls_temporal_enabled,
                                            .history_weight = &rtls::g_rtls_history_weight,
                                            .depth_rejection = &rtls::g_rtls_depth_rejection,
                                            .normal_rejection = &rtls::g_rtls_normal_rejection,
                                            .spatial_section = "RTLS Denoising",
                                            .spatial_enabled_tooltip = "Denoises the RT local shadow term at trace resolution with edge-aware weights (depth plane and normal), before the upsample. The history keeps the unfiltered term. Off = no filter.",
                                            .spatial_enabled = &rtls::g_rtls_spatial_enabled,
                                            .iterations = &rtls::g_rtls_svgf_iterations,
                                            .luminance_edge_stop = &rtls::g_rtls_svgf_luminance_edge_stop});
  settings->push_back(new Setting{
      .key = "RtlsFadeStartPct",
      .binding = &rtls::g_rtls_fade_start_pct,
      .value_type = SettingValueType::FLOAT,
      .default_value = 10.f,
      .label = "Fade Start",
      .section = "RTLS Distance Fade",
      .min = 0.f,
      .max = 100.f,
      .format = "%.1f%%",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtlsFadeEndPct",
      .binding = &rtls::g_rtls_fade_end_pct,
      .value_type = SettingValueType::FLOAT,
      .default_value = 50.f,
      .label = "Fade End",
      .section = "RTLS Distance Fade",
      .min = 0.f,
      .max = 100.f,
      .format = "%.1f%%",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtSunEnabled",
      .binding = &rtsun::g_rtsun_enabled,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 0.f,
      .label = "RT Sun Shadows",
      .section = "RT Sun Shadows",
      .tooltip = "Ray-traced sun shadows over the cascade range RT owns (Cascade Range), traced through the live BVH. This stage records the raw sun term and its debug view; the image is not changed yet. Off = no cost. While on, the BVH inputs run.",
      .labels = {"Off", "On"},
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtSunStatus",
      .value_type = SettingValueType::CUSTOM,
      .can_reset = false,
      .label = "RT sun status",
      .section = "RT Sun Shadows",
      .on_draw = []() {
        rtsun::DrawRtSunPanel();
        return false;
      },
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "",
      .binding = &rtsun::g_rtsun_debug,
      .value_type = SettingValueType::INTEGER,
      .default_value = 0.f,
      .label = "RT Sun Debug",
      .section = "RT Sun Shadows",
      .tooltip = "Session only (not saved). 1 raw sun term (grey = shadow x cascade weight). 2 pixel classes (grey scaled, dark blue sky, magenta no normal, olive character, dark green outside the region, dark purple beyond the cascade range, white traced). 3 unused. 4 accumulated term after the temporal pass, 5 history confidence (both need RT Sun Temporal Accumulation on), 6 final denoised term (the value the lighting shader reads). 7 SVGF variance (needs RT Sun Denoising on): grey = sigma / (1 + sigma) of the luminance variance the a-trous filter steers by; blue = 1 where the history is shorter than 4 frames.",
      .labels = {"Off", "Raw shadow", "Pixel classes", "Unused", "Accumulated", "History confidence", "Final", "SVGF variance"},
      .is_visible = advanced,
  });
  settings->push_back(new Setting{
      .key = "RtSunCascadeRange",
      .binding = &rtsun::g_rtsun_cascade_range,
      .value_type = SettingValueType::INTEGER,
      .default_value = 2.f,
      .label = "Cascade Range",
      .section = "RT Sun Shadows",
      .tooltip = "Number of sun cascades RT owns, counted from the nearest. Capped by the camera's cascade count and by the BVH region; the status line shows the effective count.",
      .min = 1.f,
      .max = 4.f,
      .format = "%d",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtSunKeepCascadeMaps",
      .binding = &rtsun::g_rtsun_keep_cascade_maps,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 1.f,
      .label = "Keep Cascade Maps For Fog",
      .section = "RT Sun Shadows",
      .tooltip = "The game's volumetric fog reads the sun cascade maps. On keeps those maps drawn so fog matches no-RT-sun, at the cost of drawing the RT-owned cascades. Off saves that cost but fog gets thicker. The sun shadow resolve is skipped either way.",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtSunStrength",
      .binding = &rtsun::g_rtsun_strength,
      .value_type = SettingValueType::FLOAT,
      .default_value = 1.f,
      .label = "Strength",
      .section = "RT Sun Shadows",
      .tooltip = "Scales only the RT sun shadow inside the RT range. 0 means no RT sun shadow inside the range.",
      .min = 0.f,
      .max = 1.f,
      .format = "%.2f",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtSunMaxDarkening",
      .binding = &rtsun::g_rtsun_max_darkening,
      .value_type = SettingValueType::FLOAT,
      .default_value = 1.f,
      .label = "Max Darkening",
      .section = "RT Sun Shadows",
      .tooltip = "Caps the darkest RT sun shadow: the sun loses no more than this share of its light inside the RT range; lighter shadows are unchanged. 1 = no cap.",
      .min = 0.f,
      .max = 1.f,
      .format = "%.2f",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtSunTraceCharacters",
      .binding = &rtsun::g_rtsun_trace_characters,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 1.f,
      .label = "Trace characters",
      .section = "RT Sun Shadows",
      .tooltip = "Character casters block the sun ray.",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtSunReceiveCharacters",
      .binding = &rtsun::g_rtsun_receive_characters,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 1.f,
      .label = "Receive on characters",
      .section = "RT Sun Shadows",
      .tooltip = "Character pixels take the RT sun shadow, traced with the sun direction, so cast and receive agree.",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtSunCharacterNormalBias",
      .binding = &rtsun::g_rtsun_character_normal_bias,
      .value_type = SettingValueType::FLOAT,
      .default_value = 0.02f,
      .label = "Character Normal Bias",
      .section = "RT Sun Shadows",
      .min = 0.f,
      .max = 0.5f,
      .format = "%.3f",
      .is_visible = visible,
      .is_logarithmic = true,
  });
  settings->push_back(new Setting{
      .key = "RtSunCharacterIntensity",
      .binding = &rtsun::g_rtsun_character_intensity,
      .value_type = SettingValueType::FLOAT,
      .default_value = 1.f,
      .label = "Character Intensity",
      .section = "RT Sun Shadows",
      .min = 0.f,
      .max = 1.f,
      .format = "%.2f",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtSunCharacterMaxDarkening",
      .binding = &rtsun::g_rtsun_character_max_darkening,
      .value_type = SettingValueType::FLOAT,
      .default_value = 1.f,
      .label = "Character Max Darkening",
      .section = "RT Sun Shadows",
      .tooltip = "Caps the darkest RT sun shadow on characters when Receive on characters is on. Characters lose no more than this share of sun light. Lighter shadows are unchanged, unlike Character Intensity. 1 = no cap. Environment shadows use Max Darkening.",
      .min = 0.f,
      .max = 1.f,
      .format = "%.2f",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtSunFoliageIntensity",
      .binding = &rtsun::g_rtsun_foliage_intensity,
      .value_type = SettingValueType::FLOAT,
      .default_value = 1.f,
      .label = "Foliage Shadow Intensity",
      .section = "RT Sun Shadows",
      .tooltip = "How much sun light cutout foliage (leaves, grass, fences) blocks. 1 = full shadow as before; lower lets part of the sun through, like light through leaves. Characters, buildings and trunks are unchanged.",
      .min = 0.f,
      .max = 1.f,
      .format = "%.2f",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtSunSamples",
      .binding = &rtsun::g_rtsun_samples,
      .value_type = SettingValueType::INTEGER,
      .default_value = 1.f,
      .label = "Samples",
      .section = "RT Sun Shadows",
      .min = 1.f,
      .max = 10.f,
      .format = "%d",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtSunRayMax",
      .binding = &rtsun::g_rtsun_ray_max,
      .value_type = SettingValueType::FLOAT,
      .default_value = 100.f,
      .label = "Max Ray Distance",
      .section = "RT Sun Shadows",
      .min = 1.f,
      .max = 500.f,
      .format = "%.0f",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtSunLightSize",
      .binding = &rtsun::g_rtsun_light_size,
      .value_type = SettingValueType::FLOAT,
      .default_value = 0.53f,
      .label = "Light Size (degrees)",
      .section = "RT Sun Shadows",
      .tooltip = "Full angular diameter of the sun (about 0.53 degrees) for soft shadows; 0 = hard.",
      .min = 0.f,
      .max = 5.f,
      .format = "%.2f",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtSunNormalBias",
      .binding = &rtsun::g_rtsun_normal_bias,
      .value_type = SettingValueType::FLOAT,
      .default_value = 0.02f,
      .label = "Normal Bias",
      .section = "RT Sun Shadows",
      .min = 0.f,
      .max = 0.5f,
      .format = "%.3f",
      .is_visible = visible,
      .is_logarithmic = true,
  });
  settings->push_back(new Setting{
      .key = "RtSunIsfast",
      .binding = &rtsun::g_rtsun_isfast,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 1.f,
      .label = "IS-FAST Noise",
      .section = "RT Sun Shadows",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtSunTraceResolution",
      .binding = &rtsun::g_rtsun_trace_resolution,
      .value_type = SettingValueType::INTEGER,
      .default_value = 1.f,
      .label = "Trace Resolution",
      .section = "RT Sun Shadows",
      .tooltip = "Resolution of the RT sun trace: Full = one trace pixel per pixel, Half = one per 2 x 2 block, Quarter = one per 4 x 4 block. The denoiser upsamples to full size.",
      .labels = {"Full", "Half", "Quarter"},
      .is_visible = visible,
  });
  AddSvgfRows(settings, visible, advanced, {.prefix = "RtSun",
                                            .temporal_section = "RT Sun Temporal Accumulation",
                                            .temporal_enabled_tooltip = "Reprojects the previous frame's RT sun term with the game motion (RTV4 target) and blends it in after validation. Resets on camera cuts, frame gaps and parameter changes (Trace Resolution, Cascade Range, samples and the other trace settings).",
                                            .temporal_enabled = &rtsun::g_rtsun_temporal_enabled,
                                            .history_weight = &rtsun::g_rtsun_history_weight,
                                            .depth_rejection = &rtsun::g_rtsun_depth_rejection,
                                            .normal_rejection = &rtsun::g_rtsun_normal_rejection,
                                            .spatial_section = "RT Sun Denoising",
                                            .spatial_enabled_tooltip = "Denoises the RT sun term at trace resolution with edge-aware weights (depth plane and normal), before the upsample. The history keeps the unfiltered term. Off = no filter.",
                                            .spatial_enabled = &rtsun::g_rtsun_spatial_enabled,
                                            .iterations = &rtsun::g_rtsun_svgf_iterations,
                                            .luminance_edge_stop = &rtsun::g_rtsun_svgf_luminance_edge_stop});
}

}  // namespace falcom_world
