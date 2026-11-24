#pragma once

// Ray Tracing tab content.
//
// The tab only exists when DevKit is present (world::AddSettings adds rows only
// when g_state.supported is true). The BVH panel (Pool Scan, GPU debug views)
// is the main workflow; the census panel lists draws per VS family.

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <functional>
#include <string>

#include <Windows.h>

#include <deps/imgui/imgui.h>

#include "bvh/world_bvh.hpp"
#include "capture/draw_census.hpp"
#include "pt/pt_diagnostics.hpp"
#include "pt/pt_state.hpp"
#include "rr/rr_interop.hpp"
#include "rr/rr_state.hpp"
#include "compute/compute.hpp"
#include "denoise/denoise.hpp"
#include "rtxdi/rtxdi_config.hpp"
#include "rtxdi/rtxdi_debug_layout.h"
#include "rtxdi/rtxdi_diagnostics.hpp"
#include "rtxdi/rtxdi_state.hpp"
#include "world_state.hpp"

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

// Adds the SVGF rows of GTVBAO and GTVBGI: Temporal Accumulation (the reprojection,
// paper Sec. 4.1) and Denoising (Iterations, the a-trous count of Sec. 4.3; Luminance Edge Stop, sigma_l of Sec. 4.4). The
// Optional spatial depth and normal edge stops are exposed independently of temporal rejection.
struct SvgfRows {
  const char* prefix;  // key prefix: "Gtvbao" or "Vbgi"
  const char* temporal_section;
  const char* temporal_enabled_tooltip;
  float* temporal_enabled;
  float* full_res_denoise;
  float* temporal_far_only = nullptr;
  float* history_weight;
  float* depth_rejection;
  float* normal_rejection;
  const char* spatial_section;
  const char* spatial_enabled_tooltip;
  float* spatial_enabled;
  float* iterations;
  float* luminance_edge_stop;
  float* depth_sigma = nullptr;
  float* normal_power = nullptr;
  float* half_res_iterations = nullptr;  // GTVBAO only: Half Res Iterations row; Iterations then applies at Full only
  float* history_length = nullptr;
  float* radius_scale = nullptr;
  float* half_res_radius_scale = nullptr;
  float default_temporal_enabled = 1.f;
  float default_temporal_far_only = 0.f;
  float default_full_res_denoise = 0.f;
  float default_history_weight = 0.8f;
  float default_history_length = 32.f;
  float default_depth_rejection = 0.2f;
  float default_normal_rejection = 0.f;
  float default_spatial_enabled = 1.f;
  float default_iterations = 5.f;
  float default_half_res_iterations = 4.f;
  float default_luminance_edge_stop = 4.f;
  float default_depth_sigma = 1.f;
  float default_normal_power = 128.f;
  float default_radius_scale = 1.f;
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
      .default_value = rows.default_temporal_enabled,
      .label = "Temporal Accumulation",
      .section = rows.temporal_section,
      .tooltip = rows.temporal_enabled_tooltip,
      .labels = {"Off", "On"},
      .is_visible = visible,
  });
  if (rows.temporal_far_only != nullptr) {
    settings->push_back(new Setting{
        .key = prefix + "TemporalFarFieldOnly",
        .binding = rows.temporal_far_only,
        .value_type = SettingValueType::BOOLEAN,
        .default_value = rows.default_temporal_far_only,
        .label = "Temporal Far Field only",
        .section = rows.temporal_section,
        .tooltip = "Accumulates resolved far-field color while near GI stays current. Requires the far field and motion; independent of Far Temporal Reuse. Multi-Bounce feedback is disabled while this mode is selected.",
        .labels = {"Off", "On"},
        .is_visible = [visible, enabled = rows.temporal_enabled]() { return visible() && *enabled < 0.5f; },
    });
  }
  settings->push_back(new Setting{
      .key = prefix + "FullResDenoise",
      .binding = rows.full_res_denoise,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = rows.default_full_res_denoise,
      .label = "Full Res Denoise",
      .section = rows.temporal_section,
      .tooltip = "Reconstruct to full resolution before temporal accumulation and spatial filtering. Uses substantially more GPU memory and processing time.",
      .labels = {"Off", "On"},
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = prefix + "HistoryWeight",
      .binding = rows.history_weight,
      .value_type = SettingValueType::FLOAT,
      .default_value = rows.default_history_weight,
      .label = "History Weight",
      .section = rows.temporal_section,
      .tooltip = "Weight of the reprojected history: alpha = 1 - History Weight (0 to 0.99). Higher = smoother, slower to follow changes.",
      .min = 0.f,
      .max = 0.99f,
      .format = "%.2f",
      .is_visible = visible,
  });
  if (rows.history_length != nullptr) {
    settings->push_back(new Setting{
        .key = prefix + "SvgfHistoryLength",
        .binding = rows.history_length,
        .value_type = SettingValueType::INTEGER,
        .default_value = rows.default_history_length,
        .label = "SVGF History Length",
        .section = rows.temporal_section,
        .tooltip = "Caps accepted per-pixel SVGF history age and sets the minimum current-frame weight. Higher values can smooth more but increase lag and ghosting. For GTVBGI, Far History Length separately caps far reservoir candidates.",
        .min = 4.f,
        .max = 128.f,
        .format = "%d",
        .is_visible = visible,
    });
  }
  settings->push_back(new Setting{
      .key = prefix + "DepthRejection",
      .binding = rows.depth_rejection,
      .value_type = SettingValueType::FLOAT,
      .default_value = rows.default_depth_rejection,
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
      .default_value = rows.default_normal_rejection,
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
      .default_value = rows.default_spatial_enabled,
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
      .default_value = rows.default_iterations,
      .label = rows.half_res_iterations != nullptr ? "Full Res Iterations" : "Iterations",
      .section = rows.spatial_section,
      .tooltip = rows.half_res_iterations != nullptr ? "A-trous iterations of the SVGF filter at Full resolution (1 to 5). Half resolution uses Half Res Iterations." : "A-trous iterations of the SVGF filter (1 to 5). The effective count is this minus log2 of the trace divisor (Half: minus 1, Quarter: minus 2), at least 1; the panel shows the effective count.",
      .min = 1.f,
      .max = 5.f,
      .format = "%d",
      .is_visible = visible,
  });
  if (rows.radius_scale != nullptr) {
    settings->push_back(new Setting{
        .key = prefix + "SvgfRadiusScale",
        .binding = rows.radius_scale,
        .value_type = SettingValueType::INTEGER,
        .default_value = rows.default_radius_scale,
        .label = "Spatial Radius Scale",
        .section = rows.spatial_section,
        .tooltip = "Scales the spacing between a-trous taps without changing the iteration count or kernel. Higher spacing skips neighbors and can cause shimmer or grid patterns.",
        .min = 1.f,
        .max = 4.f,
        .format = "%d",
        .is_visible = visible,
    });
  }
  if (rows.half_res_iterations != nullptr) {
    settings->push_back(new Setting{
        .key = prefix + "SvgfHalfResIterations",
        .binding = rows.half_res_iterations,
        .value_type = SettingValueType::INTEGER,
        .default_value = rows.default_half_res_iterations,
        .label = "Half Res Iterations",
        .section = rows.spatial_section,
        .tooltip = "A-trous iterations of the SVGF filter at Half resolution (1 to 4). Each iteration doubles the step, so 4 at Half covers the same screen area as 5 at Full.",
        .min = 1.f,
        .max = 4.f,
        .format = "%d",
        .is_visible = visible,
    });
  }
  if (rows.half_res_radius_scale != nullptr) {
    settings->push_back(new Setting{
        .key = prefix + "SvgfHalfResRadiusScale",
        .binding = rows.half_res_radius_scale,
        .value_type = SettingValueType::INTEGER,
        .default_value = 1.f,
        .label = "Half Res Spatial Radius Scale",
        .section = rows.spatial_section,
        .tooltip = "Scales the spacing between a-trous taps when tracing at Half resolution without changing the iteration count or kernel. Higher spacing skips neighbors and can cause shimmer or grid patterns.",
        .min = 1.f,
        .max = 4.f,
        .format = "%d",
        .is_visible = visible,
    });
  }
  if (rows.depth_sigma != nullptr) {
    settings->push_back(new Setting{
        .key = prefix + "SvgfDepthSigma",
        .binding = rows.depth_sigma,
        .value_type = SettingValueType::FLOAT,
        .default_value = rows.default_depth_sigma,
        .label = "Spatial Depth Sigma",
        .section = rows.spatial_section,
        .tooltip = "A-trous spatial filter edge stop. Higher values weaken depth rejection. Separate from temporal history rejection and upsampling.",
        .min = 0.1f,
        .max = 8.f,
        .format = "%.2f",
        .is_visible = [visible, advanced]() { return visible() && advanced(); },
    });
  }
  if (rows.normal_power != nullptr) {
    settings->push_back(new Setting{
        .key = prefix + "SvgfNormalPower",
        .binding = rows.normal_power,
        .value_type = SettingValueType::FLOAT,
        .default_value = rows.default_normal_power,
        .label = "Spatial Normal Power",
        .section = rows.spatial_section,
        .tooltip = "A-trous spatial filter edge stop. Lower values weaken normal rejection. Separate from temporal history rejection and upsampling.",
        .min = 1.f,
        .max = 128.f,
        .format = "%.1f",
        .is_visible = [visible, advanced]() { return visible() && advanced(); },
    });
  }
  settings->push_back(new Setting{
      .key = prefix + "SvgfLuminanceEdgeStop",
      .binding = rows.luminance_edge_stop,
      .value_type = SettingValueType::FLOAT,
      .default_value = rows.default_luminance_edge_stop,
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
      .tooltip = "Runs DLSS Ray Reconstruction through a separate D3D12 NGX runtime and shared-resource bridge while the game remains on D3D11. The selected preset is shown below. Needs an NVIDIA RTX GPU and DLSS selected in the game's graphics options; game DLSS Super Resolution is used as the fallback when RR cannot submit an output.",
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
      // TEMP perf test (remove)
      .key = "RrPerfPresent",
      .binding = &rr::g_rr_perf_present,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 1.f,
      .label = "[perf test] RR present handler",
      .section = "Ray Tracing",
      .labels = {"Off", "On"},
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      // TEMP perf test (remove)
      .key = "RrPerfGuides",
      .binding = &rr::g_rr_perf_guides,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 1.f,
      .label = "[perf test] RR guide pass and snapshot",
      .section = "Ray Tracing",
      .labels = {"Off", "On"},
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      // TEMP perf test (remove)
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
          const int preset_index = std::clamp(static_cast<int>(rr::g_rr_preset_index), 0, 3);
          ImGui::Text("Backend: D3D12 NGX (game API: D3D11)");
          ImGui::Text("Preset: %s (%u)", rr::RrPresetName(preset_index), rr::kRrPresetValues[preset_index]);
          ImGui::Text("Ray Reconstruction: %s", rr::RrReasonName(rr::g_rr_reason.load()));
          ImGui::Text("This attempt: %s", rr::g_rr_running.load() ? "RR output queued; game SR fallback inactive" : "using game SR fallback");
          ImGui::Text("Ray Reconstruction guides: %s", rr::GuideReasonName(rr::g_rr_guide_reason.load()));
          const std::string bridge_status = rr::RrInteropStatusSnapshot();
          ImGui::Text("RR input bridge: %s", bridge_status.c_str());
          const uint32_t submit_stage = rr::g_rr_last_submit_stage.load();
          if (submit_stage != static_cast<uint32_t>(rr::RrSubmitStage::None)) {
            const auto submit_outcome = static_cast<rr::RrSubmitOutcome>(rr::g_rr_last_submit_outcome.load());
            ImGui::Text("Last RR attempt: %s", rr::RrSubmitOutcomeName(submit_outcome));
            ImGui::Text("Last RR stage: %s", rr::RrSubmitStageName(static_cast<rr::RrSubmitStage>(submit_stage)));
            ImGui::Text("Last failure: HRESULT 0x%08X, NGX 0x%08X, exception 0x%08X",
                        rr::g_rr_last_submit_hr.load(), rr::g_rr_last_submit_result.load(),
                        rr::g_rr_last_submit_exception.load());
          }
          const uint64_t signal_a = rr::g_rr_last_signal_a.load();
          const uint64_t signal_b = rr::g_rr_last_signal_b.load();
          const uint64_t signal_c = rr::g_rr_last_signal_c.load();
          const uint64_t completed_b = rr::g_rr_last_completed_b.load();
          const uint64_t completed_c = rr::g_rr_last_completed_c.load();
          ImGui::Text("Last handoff: A=%llu, B=%llu, C=%llu (%s)", static_cast<unsigned long long>(signal_a),
                      static_cast<unsigned long long>(signal_b), static_cast<unsigned long long>(signal_c),
                      rr::g_rr_last_output_queued.load() ? "output queued" : "output not queued");
          const std::string completed_b_text = completed_b == UINT64_MAX ? "unavailable" : std::to_string(completed_b);
          const std::string completed_c_text = completed_c == UINT64_MAX ? "unavailable" : std::to_string(completed_c);
          ImGui::Text("Observed complete: B=%s, C=%s", completed_b_text.c_str(), completed_c_text.c_str());
          if (rr::g_rr_failed.load()) {
            ImGui::Text("NGX result 0x%08X, exception 0x%08X", rr::g_rr_failed_result.load(), rr::g_rr_failed_exception.load());
          }
          if (rr::g_rr_d3d12_failure_hr.load() != 0u || rr::g_rr_d3d12_failure_result.load() != 0u) {
            ImGui::Text("D3D12 setup code: HRESULT 0x%08X, NGX 0x%08X", rr::g_rr_d3d12_failure_hr.load(),
                        rr::g_rr_d3d12_failure_result.load());
          }
        }
        return false; },
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

#if defined(FALCOM_NRD_ENABLED)
#endif
  settings->push_back(new Setting{
      .key = "",
      .binding = &rtxdi::g_debug_view,
      .value_type = SettingValueType::INTEGER,
      .default_value = static_cast<float>(RTXDI_DEBUG_VIEW_OFF),
      .label = "RTXDI Debug View",
      .section = "Path Tracing Debug",
      .tooltip = "Session only. Observes current RTXDI signals; unavailable sources use a checker. It does not enable sampling or change lighting settings.",
      .labels = {"Off", "Local status", "Receiver class", "Selected light/type", "Raw diffuse", "Filtered diffuse", "Distance validity", "Distance magnitude", "Consumer selection", "Sun visibility"},
      .min = static_cast<float>(RTXDI_DEBUG_VIEW_OFF),
      .max = static_cast<float>(RTXDI_DEBUG_VIEW_SUN_VISIBILITY),
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "",
      .binding = &rtxdi::g_sun_detail_view,
      .value_type = SettingValueType::INTEGER,
      .default_value = static_cast<float>(RTXDI_SUN_DETAIL_STATUS),
      .label = "RTXDI Sun Detail",
      .section = "Path Tracing Debug",
      .tooltip = "Session only. Status preserves the Sun Visibility view (green = sun reaches the pixel through foliage layers); Reason, Hit distance, Layers and Blocker inspect the matching sun result record.",
      .labels = {"Status", "Reason", "Hit distance", "Layers", "Blocker"},
      .min = static_cast<float>(RTXDI_SUN_DETAIL_STATUS),
      .max = static_cast<float>(RTXDI_SUN_DETAIL_BLOCKER),
      .is_visible = [visible] {
        return visible() && static_cast<uint32_t>(std::clamp(std::isfinite(rtxdi::g_debug_view)
                                                                 ? rtxdi::g_debug_view
                                                                 : 0.f,
                                                             0.f, static_cast<float>(RTXDI_DEBUG_VIEW_SUN_VISIBILITY)))
               == RTXDI_DEBUG_VIEW_SUN_VISIBILITY;
      },
  });
  settings->push_back(new Setting{
      .key = "RtxdiScenePrepStatus",
      .value_type = SettingValueType::CUSTOM,
      .can_reset = false,
      .label = "Scene preparation",
      .section = "Path Tracing Debug",
      .on_draw = []() {
        rtxdi::SceneSnapshot snapshot;
        {
          std::shared_lock lock(rtxdi::g_scene.mutex);
          snapshot = rtxdi::g_scene.snapshot;
        }
        ImGui::Text("Scene status: %s", rtxdi::SceneStatusName(snapshot.status));
        rtxdi::InputSnapshot input;
        {
          std::shared_lock lock(rtxdi::g_input.mutex);
          input = rtxdi::g_input.snapshot;
        }
        ImGui::Text("Input bridge: %s", rtxdi::InputStatusName(input.status));
        rtxdi::DiSnapshot di;
        {
          std::shared_lock lock(rtxdi::g_di.mutex);
          di = rtxdi::g_di.snapshot;
        }
        ImGui::Text("DI pipeline: %s | frame %u | config %llu | settings %llx | scene %d | transfer %d | history %s | temporal %s/%s (%u->%u) | spatial %s/%s (%u->%u), candidates %u, samples %u+%u, cap %u | fence %llu | output staged %s, tuple bound %s, lighting draw with tuple bound %s (%ux%u frame %u; B %llu, C %llu) | error 0x%08x",
                    rtxdi::DiStatusName(di.status), di.frame,
                    static_cast<unsigned long long>(di.configuration_generation),
                    static_cast<unsigned long long>(di.settings_identity), di.scene_slot, di.transfer_slot,
                    di.history_valid ? "valid" : "reset", di.temporal_ran ? "ran" : "skipped",
                    di.temporal_enabled ? "on" : "off", di.temporal_source_index, di.temporal_output_index,
                    di.spatial_ran ? "ran" : "skipped", di.spatial_enabled ? "on" : "off",
                    di.spatial_source_index, di.spatial_output_index, di.local_candidates, di.spatial_samples,
                    di.spatial_disocclusion, di.history_length,
                    static_cast<unsigned long long>(di.fence), di.output_staged ? "yes" : "no",
                    di.output_bound ? "yes" : "no", di.lighting_draw_seen ? "yes" : "no",
                    di.output_width, di.output_height, di.output_frame,
                    static_cast<unsigned long long>(di.output_fence_b), static_cast<unsigned long long>(di.output_fence_c),
                    static_cast<unsigned int>(di.error));
        rt12::Rt12Panel rt12_panel;
        {
          std::lock_guard lock(rt12::g_rt12_panel_mutex);
          rt12_panel = rt12::g_rt12_panel;
        }
        ImGui::Text("Stable light remap: %s", di.stable_light_remap ? "On" : "Off");
        if (rt12_panel.light_table_available) {
          const char* light_table_status = rt12_panel.light_table_status == RTXDI_LIGHT_TABLE_STATUS_READY           ? "Ready"
                                          : rt12_panel.light_table_status == RTXDI_LIGHT_TABLE_STATUS_NO_HISTORY    ? "NoHistory"
                                          : rt12_panel.light_table_status == RTXDI_LIGHT_TABLE_STATUS_ZERO_LIGHTS   ? "ZeroLights"
                                          : rt12_panel.light_table_status == RTXDI_LIGHT_TABLE_STATUS_INVALID_INPUT ? "InvalidInput"
                                          : rt12_panel.light_table_status == RTXDI_LIGHT_TABLE_STATUS_RESOURCE_FAILED
                                              ? "ResourceCreateFailed"
                                          : rt12_panel.light_table_status == RTXDI_LIGHT_TABLE_STATUS_PIPELINE_FAILED
                                              ? "PipelineCreateFailed"
                                          : rt12_panel.light_table_status == RTXDI_LIGHT_TABLE_STATUS_DISPATCH_FAILED
                                              ? "DispatchOrFenceFailed"
                                              : "Truncated";
          ImGui::Text("Light table: %s reason=%u entries=%u/%u truncated=%u", light_table_status,
                      rt12_panel.light_table_reason, rt12_panel.light_table_written, rt12_panel.light_table_copied,
                      rt12_panel.light_table_truncated);
          ImGui::Text("Light remap: %s matched=%u unmatched_prev=%u unmatched_current=%u hits=%u",
                      rt12_panel.light_table_remap_enabled ? "On" : "Off", rt12_panel.light_table_matched,
                      rt12_panel.light_table_unmatched_previous, rt12_panel.light_table_unmatched_current,
                      rt12_panel.light_table_hits);
          ImGui::Text("Light identity: collisions=%u duplicates=%u invalid=%u incompatible=%u",
                      rt12_panel.light_table_collisions, rt12_panel.light_table_duplicates,
                      rt12_panel.light_table_invalid, rt12_panel.light_table_incompatible);
        } else {
          ImGui::Text("Light table: unavailable");
          ImGui::Text("Light remap: %s matched=unavailable unmatched_prev=unavailable unmatched_current=unavailable hits=unavailable",
                      di.stable_light_remap ? "On" : "Off");
          ImGui::Text("Light identity: collisions=unavailable duplicates=unavailable invalid=unavailable incompatible=unavailable");
        }
        ImGui::Text("IS-FAST noise copies: %llu (sun uses it only when IS-FAST is On and Light Size > 0; 0 = SDK random sequence)",
                    static_cast<unsigned long long>(rt12_panel.noise_copies));
        const auto isfast_state = [&rt12_panel](float light_size, float isfast) -> const char* {
          if (light_size <= 0.f) return "off: Light Size 0";
          if (isfast <= 0.5f) return "off";
          if (rt12_panel.noise_copies == 0u) return "off: noise not copied (IS-FAST master Off or texture not loaded)";
          return "on";
        };
        ImGui::Text("Sun IS-FAST: %s", isfast_state(rtxdi::g_sun_light_size, rtxdi::g_sun_isfast));
        ImGui::Text("Local IS-FAST: %s", isfast_state(rtxdi::g_local_light_size, rtxdi::g_local_isfast));
        rtxdi::DebugSnapshot debug;
        {
          std::shared_lock lock(rtxdi::g_debug.mutex);
          debug = rtxdi::g_debug.snapshot;
        }
        const bool render_observed_bvh_supersession = debug.superseded_by_bvh;
        const auto presentation_owner =
            ResolveRtxdiDebugPresentationOwner(debug.requested_view, render_observed_bvh_supersession);
        const char* presentation_label = "lighting image has pixel precedence";
        if (presentation_owner == RtxdiDebugPresentationOwner::Bvh) {
          presentation_label = "superseded by BVH";
        } else if (presentation_owner == RtxdiDebugPresentationOwner::Rtxdi) {
          presentation_label = "RTXDI view has pixel precedence";
        }
        ImGui::Text("Render-observed debug view: requested %u effective %u | source %s reason %u | frame %u | scene %d transfer %d | %ux%u | %s",
                    debug.requested_view, debug.effective_view, debug.source_ready ? "ready" : "unavailable",
                    debug.rejection_reason, debug.frame, debug.scene_slot, debug.transfer_slot, debug.width,
                    debug.height, presentation_label);
        if (debug.effective_view == RTXDI_DEBUG_VIEW_SUN_VISIBILITY
            && debug.sun_detail_view != RTXDI_SUN_DETAIL_STATUS) {
          ImGui::Text("Sun detail view: %s | carrier %s",
                      debug.sun_detail_view == RTXDI_SUN_DETAIL_REASON         ? "Reason"
                      : debug.sun_detail_view == RTXDI_SUN_DETAIL_HIT_DISTANCE ? "Hit distance"
                      : debug.sun_detail_view == RTXDI_SUN_DETAIL_LAYERS       ? "Layers"
                                                                               : "Blocker",
                      debug.sun_detail_source_ready ? "ready" : "unavailable");
        }
        return false;
      },
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "",
      .binding = &rtxdi::g_debug_logging,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 0.f,
      .label = "RTXDI Debug Logging",
      .section = "RTXDI Shared",
      .tooltip = "Session-only bounded diagnostics. No scene or shader contents are included.",
      .labels = {"Off", "On"},
      .on_change_value = [](float, float current) {
        rtxdi::SetDiagnosticCapture(rtxdi::GetDiagnosticState(), current > 0.5f);
      },
      .is_visible = []() { return false; },
  });
  settings->push_back(new Setting{
      .key = "RtxdiDebugReport",
      .value_type = SettingValueType::CUSTOM,
      .can_reset = false,
      .label = "RTXDI diagnostic report",
      .section = "Path Tracing Debug",
      .on_draw = []() {
        auto& diagnostic_state = rtxdi::GetDiagnosticState();
        const bool logging = rtxdi::DiagnosticCaptureEnabled(diagnostic_state);
        rtxdi::SceneSnapshot scene;
        {
          std::shared_lock lock(rtxdi::g_scene.mutex);
          scene = rtxdi::g_scene.snapshot;
        }
        rtxdi::InputSnapshot input;
        {
          std::shared_lock lock(rtxdi::g_input.mutex);
          input = rtxdi::g_input.snapshot;
        }
        rtxdi::DiSnapshot di;
        {
          std::shared_lock lock(rtxdi::g_di.mutex);
          di = rtxdi::g_di.snapshot;
        }
        rtxdi::DebugSnapshot debug;
        {
          std::shared_lock lock(rtxdi::g_debug.mutex);
          debug = rtxdi::g_debug.snapshot;
        }
        if (logging) {
          const uint32_t live_requested_view = static_cast<uint32_t>(
              std::clamp(std::isfinite(rtxdi::g_debug_view) ? rtxdi::g_debug_view : 0.f, 0.f,
                         static_cast<float>(RTXDI_DEBUG_VIEW_SUN_VISIBILITY)));
          const uint64_t live_world_frame = falcom_world::g_state.frame.load();
          const bool live_bvh_active = bvh::g_bvh_debug.mode.load(std::memory_order_relaxed) != 0;
          const uint32_t live_effective_view = ResolveRtxdiDebugEffectiveView(live_requested_view, live_bvh_active);
          rtxdi::DiagnosticSummary summary = {};
          summary.frame = di.frame != UINT32_MAX ? di.frame : input.attempted_frame;
          summary.capture_frame = input.attempted_frame;
          summary.owner_generation = input.owner_generation;
          summary.requested_settings = rtxdi::SceneRequestMask();
          summary.effective_settings = di.settings_identity;
          summary.configuration_generation = input.configuration_generation;
          summary.n1_configuration = 0u;
          summary.nrd_configuration = 0u;
          summary.scene_age = scene.prepared_frame != UINT32_MAX && scene.prepared_frame <= summary.frame
                                  ? summary.frame - scene.prepared_frame
                                  : UINT64_MAX;
          summary.input_age = input.ready_frame != UINT32_MAX && input.ready_frame <= summary.frame
                                  ? summary.frame - input.ready_frame
                                  : UINT64_MAX;
          summary.di_age = di.frame != UINT32_MAX && di.frame <= summary.frame ? summary.frame - di.frame : UINT64_MAX;
          summary.scene_fence = scene.prepared_fence;
          summary.input_fence_b = input.fence;
          summary.consumer_fence_c = di.output_fence_c;
          summary.attempts = scene.attempts;
          summary.submissions = scene.submissions;
          summary.skips = scene.skips;
          summary.bindings = static_cast<uint64_t>(di.output_bound);
          summary.matching_draw_callbacks = static_cast<uint64_t>(di.lighting_draw_seen);
          summary.current_status_reports_failure = scene.status == rtxdi::SceneStatus::Failed
                                                           || input.status == rtxdi::InputStatus::SubmitFailed
                                                       ? 1u
                                                       : 0u;
          summary.width = input.width;
          summary.height = input.height;
          summary.scene_slot = scene.prepared_slot >= 0 ? static_cast<uint32_t>(scene.prepared_slot) : UINT32_MAX;
          summary.transfer_slot = input.transfer_slot >= 0 ? static_cast<uint32_t>(input.transfer_slot) : UINT32_MAX;
          summary.scene_status = static_cast<uint32_t>(scene.status);
          summary.input_status = static_cast<uint32_t>(input.status);
          summary.input_stage = static_cast<uint32_t>(input.stage);
          summary.di_status = static_cast<uint32_t>(di.status);
          summary.debug_observed_requested_view = debug.requested_view;
          summary.debug_observed_effective_view = debug.effective_view;
          summary.debug_source_ready = debug.source_ready ? 1u : 0u;
          summary.debug_rejection_reason = debug.rejection_reason;
          summary.debug_observed_frame = debug.frame;
          summary.debug_live_requested_view = live_requested_view;
          summary.debug_live_effective_view = live_effective_view;
          summary.debug_live_frame = live_world_frame;
          summary.debug_freshness = static_cast<uint32_t>(rtxdi::GetDiagnosticDebugFreshness(
              live_requested_view, live_effective_view, live_world_frame, debug.requested_view,
              debug.effective_view, debug.frame));
          summary.debug_width = debug.width;
          summary.debug_height = debug.height;
          summary.debug_scene_slot = debug.scene_slot >= 0 ? static_cast<uint32_t>(debug.scene_slot) : UINT32_MAX;
          summary.debug_transfer_slot = debug.transfer_slot >= 0 ? static_cast<uint32_t>(debug.transfer_slot) : UINT32_MAX;
          summary.debug_owner_generation = debug.owner_generation;
          summary.debug_transfer_configuration = debug.transfer_configuration;
          summary.debug_di_configuration = debug.di_configuration;
          summary.debug_n1_configuration = debug.n1_configuration;
          summary.debug_superseded_by_bvh = debug.superseded_by_bvh ? 1u : 0u;
          summary.publication_status = static_cast<uint32_t>(di.output_staged);
          summary.binding_status = static_cast<uint32_t>(di.output_bound);
          summary.input_lights_source = input.lights_source;
          summary.input_lights_copied = input.lights_copied;
          summary.input_lights_truncated = input.lights_truncated;
          summary.input_tiles = input.tiles_copied;
          summary.input_deferred = input.deferred_copied;
          summary.di_history_valid = di.history_valid ? 1u : 0u;
          summary.di_temporal_enabled = di.temporal_enabled ? 1u : 0u;
          summary.di_temporal_ran = di.temporal_ran ? 1u : 0u;
          summary.di_spatial_enabled = di.spatial_enabled ? 1u : 0u;
          summary.di_spatial_ran = di.spatial_ran ? 1u : 0u;
          summary.rr_requested = rr::RrRequested() ? 1u : 0u;
          summary.rr_owns_denoise = rr::RrOwnsDenoise() ? 1u : 0u;
          summary.last_hresult = input.error != 0 ? input.error : di.error;
          summary.requested_candidates = rtxdi::g_local_candidates > 0.f
                                             ? static_cast<uint32_t>(rtxdi::g_local_candidates)
                                             : 0u;
          summary.effective_candidates = di.local_candidates;
          summary.requested_spatial_samples = rtxdi::g_spatial_samples > 0.f
                                                  ? static_cast<uint32_t>(rtxdi::g_spatial_samples)
                                                  : 0u;
          summary.effective_spatial_samples = di.spatial_samples;
          summary.requested_denoiser = rtxdi::g_diffuse_denoiser > 0.5f ? 1u : 0u;
          rt12::Rt12Panel nrd_panel;
          {
            std::lock_guard lock(rt12::g_rt12_panel_mutex);
            nrd_panel = rt12::g_rt12_panel;
          }
#if defined(FALCOM_NRD_ENABLED)
          summary.nrd_compiled = 1u;
          summary.nrd_status = nrd_panel.nrd_backend_state;
          summary.nrd_failure_stage = nrd_panel.nrd_failure_stage;
          summary.nrd_output_staged = di.nrd_diffuse_staged ? 1u : 0u;
          summary.nrd_gate = di.nrd_gate;
          summary.camera_qualification = (std::min)(di.nrd_gate.camera_passes, 255u)
                                         | ((std::min)(di.nrd_gate.camera_correction, 255u) << 8u)
                                         | (uint32_t{di.nrd_gate.reason[static_cast<uint32_t>(rtxdi::NrdGateLink::Camera)]} << 16u);
          if (!summary.requested_denoiser) {
            summary.nrd_policy = static_cast<uint32_t>(rtxdi::DiagnosticNrdPolicy::RawSelected);
          } else if (summary.rr_requested || summary.rr_owns_denoise) {
            summary.nrd_policy = static_cast<uint32_t>(rtxdi::DiagnosticNrdPolicy::RayReconstructionSuppressed);
          } else if (nrd_panel.nrd_backend_state == 2u) {
            summary.nrd_policy = static_cast<uint32_t>(rtxdi::DiagnosticNrdPolicy::RelaxSelected);
          } else {
            summary.nrd_policy = static_cast<uint32_t>(rtxdi::DiagnosticNrdPolicy::BackendUnavailable);
          }
          summary.effective_denoiser = summary.nrd_policy == static_cast<uint32_t>(rtxdi::DiagnosticNrdPolicy::RelaxSelected) ? 1u : 0u;
#else
          summary.nrd_compiled = 0u;
          summary.nrd_status = static_cast<uint32_t>(rtxdi::DiagnosticNrdBackend::NotCompiled);
          summary.nrd_failure_stage = 0u;
          summary.nrd_policy = !summary.requested_denoiser ? static_cast<uint32_t>(rtxdi::DiagnosticNrdPolicy::RawSelected)
                              : (summary.rr_requested || summary.rr_owns_denoise) ? static_cast<uint32_t>(rtxdi::DiagnosticNrdPolicy::RayReconstructionSuppressed)
                                                                                : static_cast<uint32_t>(rtxdi::DiagnosticNrdPolicy::NoSdk);
          summary.nrd_output_staged = 0u;
          summary.effective_denoiser = 0u;
#endif
          summary.denoiser_reason = summary.nrd_policy;
          summary.runtime_gates = (falcom_world::g_state.supported ? 1u : 0u)
                                  | (rtxdi::g_local_requested > 0.5f ? 2u : 0u);
          summary.available_mask = rtxdi::DiagnosticStatesAvailable | rtxdi::DiagnosticSettingsAvailable
                                   | rtxdi::DiagnosticRrStateAvailable | rtxdi::DiagnosticNrdStatusAvailable;
          if (summary.frame != UINT32_MAX) summary.available_mask |= rtxdi::DiagnosticFrameAvailable;
          if (input.transfer_slot >= 0) summary.available_mask |= rtxdi::DiagnosticInputIdentityAvailable;
          if (input.width != 0u && input.height != 0u) summary.available_mask |= rtxdi::DiagnosticDimensionsAvailable;
          if (scene.prepared_fence != 0u) summary.available_mask |= rtxdi::DiagnosticSceneFenceAvailable;
          if (input.fence != 0u) summary.available_mask |= rtxdi::DiagnosticInputFenceBAvailable;
          if (di.output_fence_c != 0u) summary.available_mask |= rtxdi::DiagnosticConsumerFenceCAvailable;
          rtxdi::UpdateDiagnosticSummary(diagnostic_state, rtxdi::DiagnosticCaptureEpoch(diagnostic_state), summary);
        }
        const auto snapshot = rtxdi::CopyDiagnosticSnapshot(diagnostic_state);
        ImGui::Text("Logging %s | epoch %llu | recent %u/%u (overwritten %llu) | first failures %u/%u | untracked %llu",
                    logging ? "on" : "off", static_cast<unsigned long long>(snapshot.capture_epoch),
                    snapshot.transition_size, rtxdi::kDiagnosticTransitionCapacity,
                    static_cast<unsigned long long>(snapshot.transition_overwritten), snapshot.failure_size,
                    rtxdi::kDiagnosticFailureCapacity, static_cast<unsigned long long>(snapshot.untracked_identities));
        ImGui::BeginDisabled(!logging);
        static bool copied = false;
        if (ImGui::Button("Copy Report")) {
          const auto copy = rtxdi::CopyDiagnosticSnapshot(diagnostic_state);
          const std::string report = rtxdi::SerializeDiagnosticReport(copy);
          ImGui::SetClipboardText(report.c_str());
          copied = true;
        }
        ImGui::EndDisabled();
        ImGui::SameLine();
        if (ImGui::Button("Clear Report")) {
          rtxdi::ClearDiagnosticCapture(diagnostic_state);
          copied = false;
        }
        if (copied) {
          ImGui::SameLine();
          ImGui::TextUnformatted("Copied");
        }
        return false;
      },
      .is_visible = visible,
  });

  settings->push_back(new Setting{
      .key = "RestirPtDirect",
      .binding = &pt::g_restir_direct,
      .value_type = SettingValueType::FLOAT,
      .default_value = 1.f,
      .label = "Direct Light from Path Reservoir",
      .section = "Path Tracing Direct",
      .tooltip = "Sun and lamp direct light comes from the unified path reservoir for environment and characters.",
      .min = 0.f,
      .max = 1.f,
      .format = "%.0f",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RestirPtDirectClampEnabled",
      .binding = &pt::g_restir_direct_clamp_enabled,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 1.f,
      .label = "Direct Clamp Enabled",
      .section = "Path Tracing Direct",
      .tooltip = "On = clamp direct-light luminance per sample; off = no direct clamp.",
      .labels = {"Off", "On"},
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RestirPtDirectClamp",
      .binding = &pt::g_restir_direct_clamp,
      .value_type = SettingValueType::FLOAT,
      .default_value = 16.f,
      .label = "Direct Clamp",
      .section = "Path Tracing Direct",
      .tooltip = "Luminance clamp applied to direct light from the path reservoir (0 = off).",
      .min = 0.5f,
      .max = 64.f,
      .format = "%.2f",
      .is_enabled = [] { return pt::g_restir_direct_clamp_enabled > 0.5f; },
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RestirPtCharacterShading",
      .binding = &pt::g_restir_char_toon,
      .value_type = SettingValueType::INTEGER,
      .default_value = 1.f,
      .label = "Character Shading",
      .section = "Path Tracing Direct",
      .tooltip = "Path traced uses the PT Lambert/GGX BRDF for character direct light; Game toon keeps the game's toon sun and CSM with PT indirect light only.",
      .labels = {"Path traced (Lambert/GGX)", "Game toon"},
      .min = 0.f,
      .max = 1.f,
      .format = "%d",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RestirPtGameSunShadows",
      .binding = &pt::g_restir_game_sun_shadows,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 1.f,
      .label = "RT Sun Shadows (Game Lighting)",
      .section = "Path Tracing Direct",
      .tooltip = "One world-traced sun ray shades the game's own toon and CSM lighting on pixels path tracing does not light directly.",
      .labels = {"Off", "On"},
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtxdiLocalNormalBias",
      .binding = &rtxdi::g_local_normal_bias,
      .value_type = SettingValueType::FLOAT,
      .default_value = 0.02f,
      .label = "Normal Bias (Local)",
      .section = "Path Tracing Direct",
      .min = 0.f,
      .max = 0.5f,
      .format = "%.4f",
      .is_visible = visible,
      .is_logarithmic = true,
  });
  settings->push_back(new Setting{
      .key = "RtxdiLocalCharacterNormalBias",
      .binding = &rtxdi::g_local_character_normal_bias,
      .value_type = SettingValueType::FLOAT,
      .default_value = 0.02f,
      .label = "Character Normal Bias (Local)",
      .section = "Path Tracing Direct",
      .min = 0.f,
      .max = 0.5f,
      .format = "%.3f",
      .is_visible = visible,
      .is_logarithmic = true,
  });
  settings->push_back(new Setting{
      .key = "RtxdiLocalLightClearance",
      .binding = &rtxdi::g_local_light_clearance,
      .value_type = SettingValueType::FLOAT,
      .default_value = 0.2f,
      .label = "Light Clearance (Local)",
      .section = "Path Tracing Direct",
      .tooltip = "Shadow rays stop this far short of the lamp. Inside this distance a lamp is treated as visible.",
      .min = 0.f,
      .max = 1.f,
      .format = "%.2f",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtxdiSunNormalBias",
      .binding = &rtxdi::g_sun_normal_bias,
      .value_type = SettingValueType::FLOAT,
      .default_value = 0.02f,
      .label = "Normal Bias (Sun)",
      .section = "Path Tracing Direct",
      .min = 0.f,
      .max = 0.5f,
      .format = "%.3f",
      .is_visible = visible,
      .is_logarithmic = true,
  });
  settings->push_back(new Setting{
      .key = "RtxdiSunMaxRayDistance",
      .binding = &rtxdi::g_sun_max_ray_distance,
      .value_type = SettingValueType::FLOAT,
      .default_value = 100.f,
      .label = "Max Ray Distance (Sun)",
      .section = "Path Tracing Direct",
      .tooltip = "Longest distance a sun shadow ray travels. Occluders farther away than this do not cast path-traced sun shadows.",
      .min = 1.f,
      .max = 500.f,
      .format = "%.0f",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtxdiSunCharacterNormalBias",
      .binding = &rtxdi::g_sun_character_normal_bias,
      .value_type = SettingValueType::FLOAT,
      .default_value = 0.02f,
      .label = "Character Normal Bias (Sun)",
      .section = "Path Tracing Direct",
      .min = 0.f,
      .max = 0.5f,
      .format = "%.3f",
      .is_visible = visible,
      .is_logarithmic = true,
  });
  settings->push_back(new Setting{
      .key = "RtxdiSunCastersOnly",
      .binding = &rtxdi::g_sun_casters_only,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 1.f,
      .label = "Shadow Casters Only",
      .section = "Path Tracing Direct",
      .tooltip = "Sun shadows come only from objects the game draws into its shadow maps, matching the game. Off = every object casts.",
      .labels = {"Off", "On"},
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtxdiSunShadowDarkening",
      .binding = &rtxdi::g_sun_shadow_darkening,
      .value_type = SettingValueType::FLOAT,
      .default_value = 1.f,
      .label = "Shadow Darkening (Sun)",
      .section = "Path Tracing Direct",
      .tooltip = "How dark a blocked sun shadow gets. 1 = full shadow; lower values keep more sunlight in shadow.",
      .min = 0.f,
      .max = 1.f,
      .format = "%.2f",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtxdiSunCharacterShadowDarkening",
      .binding = &rtxdi::g_sun_character_shadow_darkening,
      .value_type = SettingValueType::FLOAT,
      .default_value = 1.f,
      .label = "Character Shadow Darkening (Sun)",
      .section = "Path Tracing Direct",
      .tooltip = "How dark a character's blocked sun shadow gets: characters keep at least (1 - value) of their sun light. 1 = full shadow.",
      .min = 0.f,
      .max = 1.f,
      .format = "%.2f",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtxdiSunLightSize",
      .binding = &rtxdi::g_sun_light_size,
      .value_type = SettingValueType::FLOAT,
      .default_value = 0.f,
      .label = "Light Size (Sun, degrees)",
      .section = "Path Tracing Direct",
      .tooltip = "Angular size of the sun for soft shadows. 0 = a point sun (sharp shadows).",
      .min = 0.f,
      .max = 5.f,
      .format = "%.2f",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtxdiSunIsfast",
      .binding = &rtxdi::g_sun_isfast,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 1.f,
      .label = "IS-FAST Noise (Sun)",
      .section = "Path Tracing Direct",
      .tooltip = "Uses the blue-noise IS-FAST texture for the sun disk jitter when the IS-FAST texture is loaded (IS-FAST master On); otherwise the SDK random sequence is used. Only has an effect when Light Size > 0.",
      .labels = {"Off", "On"},
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtxdiSunCharacterSelfSoftening",
      .binding = &rtxdi::g_sun_character_self_softening,
      .value_type = SettingValueType::FLOAT,
      .default_value = 0.f,
      .label = "Character Self-Shadow Softening (Sun, m)",
      .section = "Path Tracing Direct",
      .tooltip = "Softens sun shadows on characters as if the blocker were this many metres from the skin, so arm, hair and clothing shadows get soft edges. Scales with Light Size (Sun); no effect at Light Size 0. 0 = off.",
      .min = 0.f,
      .max = 10.f,
      .format = "%.2f",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtxdiSunFoliageTransmission",
      .binding = &rtxdi::g_sun_foliage_transmission,
      .value_type = SettingValueType::FLOAT,
      .default_value = 0.f,
      .label = "Foliage Transmission",
      .section = "Path Tracing Direct",
      .tooltip = "Share of sunlight each alpha-tested leaf layer lets through. 0 = leaves block fully (unchanged).",
      .min = 0.f,
      .max = 1.f,
      .format = "%.2f",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtxdiSunWindFoliageTransmission",
      .binding = &rtxdi::g_sun_wind_transmission,
      .value_type = SettingValueType::FLOAT,
      .default_value = 0.5f,
      .label = "Wind Foliage Transmission",
      .section = "Path Tracing Direct",
      .tooltip = "Share of sunlight each wind-moved foliage object (opaque or alpha-tested) lets through, so swaying foliage casts softer shadows. Use Light Size (Sun) for soft edges. 0 = blocks fully (old behaviour).",
      .min = 0.f,
      .max = 1.f,
      .format = "%.2f",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtxdiSunWindFoliageSoftening",
      .binding = &rtxdi::g_sun_wind_softening,
      .value_type = SettingValueType::FLOAT,
      .default_value = 0.5f,
      .label = "Wind Foliage Softening (Sun, m)",
      .section = "Path Tracing Direct",
      .tooltip = "Radius in metres over which each wind-moved foliage shadow is blurred, as if the foliage swayed (noisy like Light Size). 0 = hard per-object shadow. Use with Wind Foliage Transmission.",
      .min = 0.f,
      .max = 4.f,
      .format = "%.2f",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtxdiSunFoliageTintR",
      .binding = &rtxdi::g_sun_foliage_tint_r,
      .value_type = SettingValueType::FLOAT,
      .default_value = 1.f,
      .label = "Foliage Tint R",
      .section = "Path Tracing Direct",
      .tooltip = "Red share of sunlight each leaf layer lets through, on top of Foliage Transmission. 1 = no tint.",
      .min = 0.f,
      .max = 1.f,
      .format = "%.2f",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtxdiSunFoliageTintG",
      .binding = &rtxdi::g_sun_foliage_tint_g,
      .value_type = SettingValueType::FLOAT,
      .default_value = 1.f,
      .label = "Foliage Tint G",
      .section = "Path Tracing Direct",
      .tooltip = "Green share of sunlight each leaf layer lets through, on top of Foliage Transmission. 1 = no tint.",
      .min = 0.f,
      .max = 1.f,
      .format = "%.2f",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtxdiSunFoliageTintB",
      .binding = &rtxdi::g_sun_foliage_tint_b,
      .value_type = SettingValueType::FLOAT,
      .default_value = 1.f,
      .label = "Foliage Tint B",
      .section = "Path Tracing Direct",
      .tooltip = "Blue share of sunlight each leaf layer lets through, on top of Foliage Transmission. 1 = no tint.",
      .min = 0.f,
      .max = 1.f,
      .format = "%.2f",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtxdiLocalLightSize",
      .binding = &rtxdi::g_local_light_size,
      .value_type = SettingValueType::FLOAT,
      .default_value = 0.f,
      .label = "Light Size (Local, m)",
      .section = "Path Tracing Direct",
      .tooltip = "Radius of the local lamp for soft shadows. 0 = a point light (sharp shadows).",
      .min = 0.f,
      .max = 0.5f,
      .format = "%.2f",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtxdiLocalIsfast",
      .binding = &rtxdi::g_local_isfast,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 1.f,
      .label = "IS-FAST Noise (Local)",
      .section = "Path Tracing Direct",
      .tooltip = "Uses the blue-noise IS-FAST texture for the local light point jitter when the IS-FAST texture is loaded (IS-FAST master On); otherwise the SDK random sequence is used. Only has an effect when Light Size > 0.",
      .labels = {"Off", "On"},
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtxdiLocalCharacterSelfSoftening",
      .binding = &rtxdi::g_local_character_self_softening,
      .value_type = SettingValueType::FLOAT,
      .default_value = 0.f,
      .label = "Character Self-Shadow Softening (Local, m)",
      .section = "Path Tracing Direct",
      .tooltip = "Softens lamp shadows on characters as if the blocker were this many metres from the skin, so arm, hair and clothing shadows get soft edges. Scales with Light Size (Local); no effect at Light Size 0. 0 = off.",
      .min = 0.f,
      .max = 2.f,
      .format = "%.2f",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtxdiLocalLightClampEnabled",
      .binding = &rtxdi::g_local_light_clamp_enabled,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 0.f,
      .label = "Light Sampling Clamp (Local)",
      .section = "Path Tracing Direct",
      .tooltip = "Limits the luminance of each sampled lamp (diffuse and specular, scaled together). Fewer lamp fireflies, but removes energy. Off = current look.",
      .labels = {"Off", "On"},
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtxdiLocalLightClamp",
      .binding = &rtxdi::g_local_light_clamp,
      .value_type = SettingValueType::FLOAT,
      .default_value = 16.f,
      .label = "Light Sampling Clamp Limit (Local)",
      .section = "Path Tracing Direct",
      .tooltip = "Limits the luminance of each sampled lamp (diffuse and specular, scaled together). Fewer lamp fireflies, but removes energy. Off = current look.",
      .min = 1.f,
      .max = 1000.f,
      .format = "%.1f",
      .is_enabled = [] { return rtxdi::g_local_light_clamp_enabled > 0.5f; },
      .is_visible = visible,
      .is_logarithmic = true,
  });
  settings->push_back(new Setting{
      .key = "RtxdiLocalCharacterIntensity",
      .binding = &rtxdi::g_local_character_intensity,
      .value_type = SettingValueType::FLOAT,
      .default_value = 1.f,
      .label = "Character Light Intensity",
      .section = "Path Tracing Direct",
      .tooltip = "Scales path-traced lamp light on characters.",
      .min = 0.f,
      .max = 2.f,
      .format = "%.2f",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "PtEnabled",
      .binding = &pt::g_pt_enabled,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 0.f,
      .label = "Path Traced GI",
      .section = "Path Tracing",
      .tooltip = "Sora 2nd. Needs DXR with shader model 6.6 and RTXDI Local (scene transfer and lamps); works with RTXDI Sun off. Replaces probe ambient, SSAO and GTVBAO/VBGI in the image. Raw (no denoiser) unless Ray Reconstruction is on.",
      .labels = {"Off", "On"},
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "PtStatus",
      .value_type = SettingValueType::CUSTOM,
      .can_reset = false,
      .label = "Path Tracing status",
      .section = "Path Tracing",
      .on_draw = []() { pt::DrawPtPanel(); return false; },
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "PtDebugLogging",
      .binding = &pt::g_pt_debug_logging,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 0.f,
      .label = "Debug Logging",
      .section = "Path Tracing",
      .tooltip = "Off: path tracer messages go to ReShade.log. On: they are kept in a bounded in-memory capture instead, and Copy Report includes them. Shared backend failure messages (FailSession) still go to ReShade.log.",
      .labels = {"Off", "On"},
      .on_change_value = [](float, float current) { pt::SetPtDiagnosticCapture(current > 0.5f); },
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "PtDebugReport",
      .value_type = SettingValueType::CUSTOM,
      .can_reset = false,
      .label = "Path Tracing report",
      .section = "Path Tracing",
      .on_draw = []() {
        pt::SyncPtDiagnosticCapture(pt::g_pt_debug_logging > 0.5f);
        static bool report_copied = false;
        uint64_t overwritten = 0u;
        {
          std::lock_guard lock(pt::g_pt_event_mutex);
          overwritten = pt::g_pt_event_overwritten;
        }
        ImGui::Text("Capture: %s, overwritten %llu", pt::PtDiagnosticCaptureEnabled() ? "on" : "off",
                    static_cast<unsigned long long>(overwritten));
        if (ImGui::Button("Copy Report")) {
          ImGui::SetClipboardText(pt::SerializePtReport().c_str());
          report_copied = true;
        }
        if (report_copied) {
          ImGui::SameLine();
          ImGui::Text("Report copied");
        }
        return false;
      },
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "PtReGirEnabled",
      .binding = &pt::g_pt_regir_enabled,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 0.f,
      .label = "ReGIR",
      .section = "Path Tracing",
      .tooltip = "Builds primary-surface light pools for matching path-traced hits; unmatched hits use uniform sampling.",
      .labels = {"Off", "On"},
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "PtSparseEnabled",
      .binding = &pt::g_pt_sparse_enabled,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 0.f,
      .label = "Sparse Rendering",
      .section = "Path Tracing",
      .tooltip = "With Ray Reconstruction active, trace diffuse GI at a subset of opaque pixels, with optional sparse reflections; water stays dense.",
      .labels = {"Off", "On"},
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "PtSparseReflections",
      .binding = &pt::g_pt_sparse_reflections,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 1.f,
      .label = "Sparse Rendering Reflections",
      .section = "Path Tracing",
      .tooltip = "Apply sparse sampling to path-traced reflections. Off keeps reflections dense while diffuse GI remains sparse.",
      .labels = {"Off", "On"},
      .is_visible = [visible]() { return visible() && pt::g_pt_sparse_enabled > 0.5f; },
  });
  settings->push_back(new Setting{
      .key = "PtSparseAdaptive",
      .binding = &pt::g_pt_sparse_adaptive,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 1.f,
      .label = "Sparse Adaptive Rate",
      .section = "Path Tracing",
      .tooltip = "Scale the reference rate with render resolution.",
      .labels = {"Off", "On"},
      .is_visible = [visible]() { return visible() && pt::g_pt_sparse_enabled > 0.5f; },
  });
  settings->push_back(new Setting{
      .key = "PtSparseRate",
      .binding = &pt::g_pt_sparse_rate,
      .value_type = SettingValueType::FLOAT,
      .default_value = 0.10f,
      .label = "Sparse Sampling Rate",
      .section = "Path Tracing",
      .tooltip = "Fraction at reference resolution, or fixed fraction when adaptive is off.",
      .min = 0.0625f, .max = 1.f, .format = "%.4f",
      .is_visible = [visible]() { return visible() && pt::g_pt_sparse_enabled > 0.5f; },
  });
  settings->push_back(new Setting{
      .key = "PtSparseExponent",
      .binding = &pt::g_pt_sparse_exponent,
      .value_type = SettingValueType::FLOAT,
      .default_value = 0.91f,
      .label = "Sparse Resolution Exponent",
      .section = "Path Tracing",
      .tooltip = "How strongly the rate rises at lower resolutions.",
      .min = 0.f, .max = 1.f, .format = "%.2f",
      .is_visible = [visible]() { return visible() && pt::g_pt_sparse_enabled > 0.5f; },
  });
  settings->push_back(new Setting{
      .key = "PtSparseQuality",
      .binding = &pt::g_pt_sparse_quality,
      .value_type = SettingValueType::FLOAT,
      .default_value = 0.30f,
      .label = "Sparse Quality",
      .section = "Path Tracing",
      .tooltip = "Moves adaptive rate logarithmically toward 1/16 or 1. Ignored when adaptive is off.",
      .min = 0.f, .max = 1.f, .format = "%.2f",
      .is_visible = [visible]() { return visible() && pt::g_pt_sparse_enabled > 0.5f; },
  });
  settings->push_back(new Setting{
      .key = "PtSparseMaxRate",
      .binding = &pt::g_pt_sparse_max_rate,
      .value_type = SettingValueType::FLOAT,
      .default_value = 0.75f,
      .label = "Sparse Max Rate",
      .section = "Path Tracing",
      .tooltip = "Dense rendering above this computed rate; 1.0 allows an explicit full-rate comparison.",
      .min = 0.0625f, .max = 1.f, .format = "%.4f",
      .is_visible = [visible]() { return visible() && pt::g_pt_sparse_enabled > 0.5f; },
  });
  settings->push_back(new Setting{
      .key = "PtSamples",
      .binding = &pt::g_pt_samples,
      .value_type = SettingValueType::INTEGER,
      .default_value = 1.f,
      .label = "Samples Per Pixel",
      .section = "Path Tracing",
      .tooltip = "Path trees per pixel for GI, reflections and direct light (ReSTIR PT numInitialSamples).",
      .min = 1.f,
      .max = 16.f,
      .format = "%d",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "PtMaxBounces",
      .binding = &pt::g_pt_max_bounces,
      .value_type = SettingValueType::INTEGER,
      .default_value = 16.f,
      .label = "Max Bounces",
      .section = "Path Tracing",
      .tooltip = "Bounces after the primary surface (maxBounceDepth − 1).",
      .min = 1.f,
      .max = 16.f,
      .format = "%d",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "PtRussianRoulette",
      .binding = &pt::g_pt_russian_roulette,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 1.f,
      .label = "Russian Roulette",
      .section = "Path Tracing",
      .labels = {"Off", "On"},
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "PtRouletteMinBounce",
      .binding = &pt::g_pt_roulette_min_bounce,
      .value_type = SettingValueType::INTEGER,
      .default_value = 1.f,
      .label = "Russian Roulette From Bounce",
      .section = "Path Tracing",
      .min = 1.f,
      .max = 16.f,
      .format = "%d",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "PtIntensity",
      .binding = &pt::g_pt_intensity,
      .value_type = SettingValueType::FLOAT,
      .default_value = 1.f,
      .label = "GI Intensity",
      .section = "Path Tracing",
      .min = 0.f,
      .max = 2.f,
      .format = "%.2f",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "PtCharIntensity",
      .binding = &pt::g_pt_char_intensity,
      .value_type = SettingValueType::FLOAT,
      .default_value = 1.f,
      .label = "Character GI Intensity",
      .section = "Path Tracing",
      .min = 0.f,
      .max = 1.f,
      .format = "%.2f",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "PtSkyIntensity",
      .binding = &pt::g_pt_sky_intensity,
      .value_type = SettingValueType::FLOAT,
      .default_value = 1.f,
      .label = "Sky Intensity",
      .section = "Path Tracing",
      .min = 0.f,
      .max = 4.f,
      .format = "%.2f",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "PtRayMax",
      .binding = &pt::g_pt_ray_max,
      .value_type = SettingValueType::FLOAT,
      .default_value = 500.f,
      .label = "Max Ray Distance",
      .section = "Path Tracing",
      .tooltip = "Longest bounce and sun shadow ray, in metres. A bounce ray that hits nothing within it sees the sky.",
      .min = 1.f,
      .max = 500.f,
      .format = "%.0f",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "PtEnvBias",
      .binding = &pt::g_pt_env_bias,
      .value_type = SettingValueType::FLOAT,
      .default_value = 0.02f,
      .label = "Normal Bias",
      .section = "Path Tracing",
      .tooltip = "Rays start this far off the surface along its normal (environment and every bounce hit), against self-hits.",
      .min = 0.f,
      .max = 0.5f,
      .format = "%.3f",
      .is_visible = visible,
      .is_logarithmic = true,
  });
  settings->push_back(new Setting{
      .key = "PtCharBias",
      .binding = &pt::g_pt_char_bias,
      .value_type = SettingValueType::FLOAT,
      .default_value = 0.02f,
      .label = "Character Normal Bias",
      .section = "Path Tracing",
      .tooltip = "Normal Bias for the first ray from character pixels.",
      .min = 0.f,
      .max = 0.5f,
      .format = "%.3f",
      .is_visible = visible,
      .is_logarithmic = true,
  });
  settings->push_back(new Setting{
      .key = "PtLampClearance",
      .binding = &pt::g_pt_lamp_clearance,
      .value_type = SettingValueType::FLOAT,
      .default_value = 0.2f,
      .label = "Lamp Clearance",
      .section = "Path Tracing",
      .tooltip = "Lamp shadow rays stop this far short of the lamp. Inside this distance a lamp is treated as visible.",
      .min = 0.f,
      .max = 1.f,
      .format = "%.2f",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "PtSunCasters",
      .binding = &pt::g_pt_sun_casters,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 0.f,
      .label = "Sun Shadow Casters Only",
      .section = "Path Tracing",
      .tooltip = "Sun shadows at bounce hits come only from objects the game draws into its shadow maps. Off = every object casts.",
      .labels = {"Off", "On"},
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "PtSharcEnabled",
      .binding = &pt::g_pt_sharc_enabled,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 1.f,
      .label = "SHARC Radiance Cache",
      .section = "Path Tracing",
      .tooltip = "World-space radiance cache (NVIDIA SHARC). Paths end early on cached light: much less noise, slightly smoother and slower to follow lighting changes. Off = the uncached path tracer.",
      .labels = {"Off", "On"},
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "PtSharcAccumulationFrames",
      .binding = &pt::g_pt_sharc_accumulation_frames,
      .value_type = SettingValueType::INTEGER,
      .default_value = 32.f,
      .label = "SHARC Accumulation Frames",
      .section = "Path Tracing",
      .tooltip = "Frames a cache cell averages. Higher = smoother, slower to react. Shortened to 4 for 32 frames after the sun changes.",
      .min = 1.f,
      .max = 1024.f,
      .format = "%d",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "PtSharcStaleFrames",
      .binding = &pt::g_pt_sharc_stale_frames,
      .value_type = SettingValueType::INTEGER,
      .default_value = 64.f,
      .label = "SHARC Stale Frames",
      .section = "Path Tracing",
      .tooltip = "Frames without new samples before a cache cell is dropped.",
      .min = 8.f,
      .max = 1024.f,
      .format = "%d",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "PtSharcSceneScale",
      .binding = &pt::g_pt_sharc_scene_scale,
      .value_type = SettingValueType::INTEGER,
      .default_value = 15.f,
      .label = "SHARC Scene Scale",
      .section = "Path Tracing",
      .tooltip = "Cache cell density. Higher = smaller cells (more detail, more noise, more memory). Changing it clears the cache.",
      .min = 1.f,
      .max = 200.f,
      .format = "%d",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "PtSharcDownscale",
      .binding = &pt::g_pt_sharc_downscale,
      .value_type = SettingValueType::INTEGER,
      .default_value = 5.f,
      .label = "SHARC Update Downscale",
      .section = "Path Tracing",
      .tooltip = "One cache-training path per N x N pixels. Lower = faster training, more GPU time. Raised automatically above 82944 paths.",
      .min = 1.f,
      .max = 16.f,
      .format = "%d",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "PtSharcRadianceScale",
      .binding = &pt::g_pt_sharc_radiance_scale,
      .value_type = SettingValueType::FLOAT,
      .default_value = 100000.f,
      .label = "SHARC Radiance Scale",
      .section = "Path Tracing",
      .tooltip = "Fixed-point scale SHARC uses to store colour. Raise it if colour fades indoors with SHARC on; very high values use more of each cell's 32-bit sums. Changing it clears the SHARC cache.",
      .min = 1000.f,
      .max = 1000000.f,
      .format = "%.0f",
      .is_visible = visible,
      .is_logarithmic = true,
  });
  settings->push_back(new Setting{
      .key = "PtSharcCapacityLog2",
      .binding = &pt::g_pt_sharc_capacity_log2,
      .value_type = SettingValueType::INTEGER,
      .default_value = 22.f,
      .label = "SHARC Capacity (log2)",
      .section = "Path Tracing",
      .tooltip = "Cache entries = 2^N. 22 = 4M entries, 256 MB of GPU memory; 20 = 64 MB. Changing it reallocates and clears.",
      .min = 16.f,
      .max = 22.f,
      .format = "%d",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "PtSharcFireflyClamp",
      .binding = &pt::g_pt_sharc_firefly_clamp_enabled,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 0.f,
      .label = "SHARC Firefly Clamp",
      .section = "Path Tracing",
      .tooltip = "Limits SHARC cache updates that are far above their cell's average (OpenRemix sharcFireflyClamp). Fewer cache fireflies, but darkens bright cached light. Off = current look.",
      .labels = {"Off", "On"},
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "PtSharcFireflyClampLimit",
      .binding = &pt::g_pt_sharc_firefly_clamp,
      .value_type = SettingValueType::FLOAT,
      .default_value = 10.f,
      .label = "SHARC Firefly Clamp Limit",
      .section = "Path Tracing",
      .tooltip = "Largest SHARC update, as a multiple of its cell's average luminance. Lower = fewer fireflies, less energy.",
      .min = 1.f,
      .max = 100.f,
      .format = "%.1f",
      .is_enabled = [] { return pt::g_pt_sharc_firefly_clamp_enabled > 0.5f; },
      .is_visible = visible,
      .is_logarithmic = true,
  });
  settings->push_back(new Setting{
      .key = "PtSharcClear",
      .value_type = SettingValueType::CUSTOM,
      .can_reset = false,
      .label = "Clear Path Tracing Caches",
      .section = "Path Tracing",
      .on_draw = []() { if (ImGui::Button("Clear Path Tracing Caches")) ++pt::g_pt_sharc_reset_request; return false; },
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "PtMaterialCache",
      .binding = &pt::g_pt_material_enabled,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 1.f,
      .label = "Material Cache",
      .section = "Path Tracing",
      .tooltip = "Bounce hits take albedo from the G-buffer when the hit is visible on screen, else from a world-space cache learned from visible pixels. Off = grey 0.5 at every hit (as before M3).",
      .labels = {"Off", "On"},
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "PtMaterialRate",
      .binding = &pt::g_pt_material_rate,
      .value_type = SettingValueType::INTEGER,
      .default_value = 4.f,
      .label = "Material Cache Write Rate",
      .section = "Path Tracing",
      .tooltip = "One pixel per N x N writes the cache each frame. Lower = learns faster, more GPU time.",
      .min = 1.f,
      .max = 16.f,
      .format = "%d",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "PtMaterialCellScale",
      .binding = &pt::g_pt_material_cell_scale,
      .value_type = SettingValueType::INTEGER,
      .default_value = 30.f,
      .label = "Material Cache Cell Scale",
      .section = "Path Tracing",
      .tooltip = "Cell density. Higher = finer colour detail, more memory. Changing it clears both caches.",
      .min = 1.f,
      .max = 200.f,
      .format = "%d",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "PtMaterialCapacityLog2",
      .binding = &pt::g_pt_material_capacity_log2,
      .value_type = SettingValueType::INTEGER,
      .default_value = 21.f,
      .label = "Material Cache Capacity (log2)",
      .section = "Path Tracing",
      .tooltip = "Entries = 2^N, 24 bytes each: 21 = 48 MB, 22 = 96 MB. Changing it reallocates and clears.",
      .min = 16.f,
      .max = 22.f,
      .format = "%d",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "PtCharRoughness",
      .binding = &pt::g_pt_char_roughness,
      .value_type = SettingValueType::FLOAT,
      .default_value = 0.5f,
      .label = "Character Roughness",
      .section = "Path Tracing",
      .tooltip = "Characters have no roughness in the G-buffer; the cache stores this value for them (used by specular in M4).",
      .min = 0.05f,
      .max = 1.f,
      .format = "%.2f",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "PtCharF0",
      .binding = &pt::g_pt_char_f0,
      .value_type = SettingValueType::FLOAT,
      .default_value = 0.04f,
      .label = "Character F0",
      .section = "Path Tracing",
      .tooltip = "Characters have no specular colour in the G-buffer; the cache stores this reflectance for them (M4).",
      .min = 0.f,
      .max = 0.2f,
      .format = "%.3f",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "PtReflection",
      .binding = &pt::g_pt_reflection_enabled,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 0.f,
      .label = "Path-Traced Reflection",
      .section = "Path Tracing",
      .tooltip = "Sora 2nd: environment reflections and water use one traced reflected ray per sample. While on, dynamic cubemaps and game SSR are switched off at runtime (saved settings unchanged); forward glass and other t17 materials keep the game cube.",
      .labels = {"Off", "On"},
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "PtReflectionIntensity",
      .binding = &pt::g_pt_reflection_intensity,
      .value_type = SettingValueType::FLOAT,
      .default_value = 1.f,
      .label = "Reflection Intensity",
      .section = "Path Tracing",
      .tooltip = "Scales the traced reflection before the game's own reflection strength and Fresnel. 1 = as traced.",
      .min = 0.f,
      .max = 2.f,
      .format = "%.2f",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "PtReflectionRoughnessScale",
      .binding = &pt::g_pt_reflection_roughness_scale,
      .value_type = SettingValueType::FLOAT,
      .default_value = 1.f,
      .label = "Reflection Roughness Scale",
      .section = "Path Tracing",
      .tooltip = "Multiplies the game's reflection roughness for the traced ray. Below 1 = sharper reflections, above 1 = blurrier. Ray length uses Max Ray Distance.",
      .min = 0.f,
      .max = 2.f,
      .format = "%.2f",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "PtReflectionClampEnabled",
      .binding = &pt::g_pt_reflection_clamp_enabled,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 1.f,
      .label = "Reflection Sample Clamp Enabled",
      .section = "Path Tracing",
      .tooltip = "On = each reflected ray is clamped by Reflection Sample Clamp (current look). Off = no per-ray clamp; brighter highlights, more fireflies.",
      .labels = {"Off", "On"},
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "PtReflectionSampleClamp",
      .binding = &pt::g_pt_reflection_sample_clamp,
      .value_type = SettingValueType::FLOAT,
      .default_value = 16.f,
      .label = "Reflection Sample Clamp",
      .section = "Path Tracing",
      .tooltip = "Reflection radiance clamp applied to the shaded output (0 = off).",
      .min = 1.f,
      .max = 1000.f,
      .format = "%.1f",
      .is_enabled = [] { return pt::g_pt_reflection_clamp_enabled > 0.5f; },
      .is_visible = visible,
      .is_logarithmic = true,
  });
  settings->push_back(new Setting{
      .key = "PtReflectionCapToOne",
      .binding = &pt::g_pt_reflection_cap_to_one,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 0.f,
      .label = "Cap Reflection To 1",
      .section = "Path Tracing",
      .tooltip = "A/B test: limits the final traced reflection to 1.0 (the game cubemap range). On = no highlight headroom.",
      .labels = {"Off", "On"},
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "PtGiClampEnabled",
      .binding = &pt::g_pt_gi_clamp_enabled,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 0.f,
      .label = "GI Clamp Enabled",
      .section = "Path Tracing",
      .tooltip = "Clamps the luminance of each bounce's light and each SHARC cache read in the GI path. Fewer fireflies, but removes energy (darker bright bounces). Off = current look.",
      .labels = {"Off", "On"},
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "PtGiClamp",
      .binding = &pt::g_pt_gi_clamp,
      .value_type = SettingValueType::FLOAT,
      .default_value = 16.f,
      .label = "GI Clamp",
      .section = "Path Tracing",
      .tooltip = "GI luminance clamp applied to the shaded output (0 = off).",
      .min = 1.f,
      .max = 1000.f,
      .format = "%.1f",
      .is_enabled = [] { return pt::g_pt_gi_clamp_enabled > 0.5f; },
      .is_visible = visible,
      .is_logarithmic = true,
  });
  settings->push_back(new Setting{
      .key = "PtGiClampMode",
      .binding = &pt::g_pt_gi_clamp_mode,
      .value_type = SettingValueType::INTEGER,
      .default_value = 0.f,
      .label = "GI Clamp Mode",
      .section = "Path Tracing",
      .tooltip = "Output (current) clamps the final indirect output. Per bounce (pre-unification) clamps each recorded light contribution to GI Clamp instead.",
      .labels = {"Output (current)", "Per bounce (pre-unification)"},
      .min = 0.f,
      .max = 1.f,
      .format = "%d",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "PtReflectionUseSharc",
      .binding = &pt::g_pt_reflection_use_sharc,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 1.f,
      .label = "Reflection Uses SHARC",
      .section = "Path Tracing",
      .tooltip = "On: reflected hits use the SHARC radiance cache (smoother, indirect light included). Off: direct light plus ambient at every reflected hit (no cache).",
      .labels = {"Off", "On"},
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "PtReflectionFootprint",
      .binding = &pt::g_pt_reflection_footprint,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 1.f,
      .label = "Reflection Cache Footprint Test",
      .section = "Path Tracing",
      .tooltip = "On: a reflected hit uses the cache only when its footprint is wider than a cache voxel, so near-mirror reflections do not show cache blocks. Off: use the cache whenever it has a value.",
      .labels = {"Off", "On"},
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "PtReflectionWaterRule",
      .binding = &pt::g_pt_reflection_water_rule,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 0.f,
      .label = "Water Reflection Bed Test",
      .section = "Path Tracing",
      .tooltip = "A/B test for lake bed in water reflections. On: water pixels use a mirror direction, and traced hits on other water pixels show sky instead of the bed. Off: current behaviour.",
      .labels = {"Off", "On"},
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "PtReflectionWaterSurface",
      .binding = &pt::g_pt_reflection_water_surface,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 0.f,
      .label = "Water surface reflection",
      .section = "Path Tracing",
      .tooltip = "On: water reflections are traced by the path tracer from a copy of the SSR surface (depth and mrt0 at the SSR draw), and ssr2 reads that water reflection. Off: current water reflection. Needs Path-Traced Reflection.",
      .labels = {"Off", "On"},
      .min = 0.f,
      .max = 1.f,
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "PtReflectionWaterRoughness",
      .binding = &pt::g_pt_reflection_water_roughness,
      .value_type = SettingValueType::FLOAT,
      .default_value = 0.05f,
      .label = "Water roughness",
      .section = "Path Tracing",
      .tooltip = "Base roughness of water reflections (0 = mirror). Reflection Roughness Scale still multiplies it.",
      .min = 0.f,
      .max = 1.f,
      .format = "%.2f",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RestirPtMaxRcVertexLength",
      .binding = &pt::g_restir_max_rc_vertex_length,
      .value_type = SettingValueType::INTEGER,
      .default_value = 5.f,
      .label = "Max Reconnection Vertex Length",
      .section = "Path Tracing Sampling",
      .tooltip = "ReSTIR PT maxRcVertexLength.",
      .min = 2.f,
      .max = 8.f,
      .format = "%d",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RestirPtNeeLocalSamples",
      .binding = &pt::g_restir_nee_local_samples,
      .value_type = SettingValueType::INTEGER,
      .default_value = 1.f,
      .label = "NEE Local Samples",
      .section = "Path Tracing Sampling",
      .tooltip = "ReSTIR PT numLocalLightSamples; 1 keeps the current one-lamp estimator.",
      .min = 0.f,
      .max = 32.f,
      .format = "%d",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RestirPtReconnectionMode",
      .binding = &pt::g_restir_reconnection_mode,
      .value_type = SettingValueType::INTEGER,
      .default_value = 1.f,
      .label = "Reconnection Mode",
      .section = "Path Tracing Reconnection",
      .tooltip = "ReSTIR PT reconnectionMode (0 = Fixed Threshold, 1 = Footprint).",
      .labels = {"Fixed Threshold", "Footprint"},
      .min = 0.f,
      .max = 1.f,
      .format = "%d",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RestirPtMinConnectionFootprint",
      .binding = &pt::g_restir_min_connection_footprint,
      .value_type = SettingValueType::FLOAT,
      .default_value = 0.02f,
      .label = "Min Connection Footprint",
      .section = "Path Tracing Reconnection",
      .tooltip = "ReSTIR PT minConnectionFootprint.",
      .min = 0.f,
      .max = 1.f,
      .format = "%.3f",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RestirPtMinConnectionFootprintSigma",
      .binding = &pt::g_restir_min_connection_footprint_sigma,
      .value_type = SettingValueType::FLOAT,
      .default_value = 0.2f,
      .label = "Min Connection Footprint Sigma",
      .section = "Path Tracing Reconnection",
      .tooltip = "ReSTIR PT minConnectionFootprintSigma.",
      .min = 0.f,
      .max = 1.f,
      .format = "%.3f",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RestirPtMinPdfRoughness",
      .binding = &pt::g_restir_min_pdf_roughness,
      .value_type = SettingValueType::FLOAT,
      .default_value = 0.1f,
      .label = "Min PDF Roughness",
      .section = "Path Tracing Reconnection",
      .tooltip = "ReSTIR PT minPdfRoughness.",
      .min = 0.f,
      .max = 1.f,
      .format = "%.3f",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RestirPtMinPdfRoughnessSigma",
      .binding = &pt::g_restir_min_pdf_roughness_sigma,
      .value_type = SettingValueType::FLOAT,
      .default_value = 0.01f,
      .label = "Min PDF Roughness Sigma",
      .section = "Path Tracing Reconnection",
      .tooltip = "ReSTIR PT minPdfRoughnessSigma.",
      .min = 0.f,
      .max = 1.f,
      .format = "%.3f",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RestirPtRoughnessThreshold",
      .binding = &pt::g_restir_roughness_threshold,
      .value_type = SettingValueType::FLOAT,
      .default_value = 0.1f,
      .label = "Roughness Threshold",
      .section = "Path Tracing Reconnection",
      .tooltip = "ReSTIR PT roughnessThreshold.",
      .min = 0.f,
      .max = 1.f,
      .format = "%.3f",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RestirPtDistanceThreshold",
      .binding = &pt::g_restir_distance_threshold,
      .value_type = SettingValueType::FLOAT,
      .default_value = 0.f,
      .label = "Distance Threshold",
      .section = "Path Tracing Reconnection",
      .tooltip = "ReSTIR PT distanceThreshold.",
      .min = 0.f,
      .max = 10.f,
      .format = "%.2f",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RestirPtResampling",
      .binding = &pt::g_restir_resampling,
      .value_type = SettingValueType::INTEGER,
      .default_value = 3.f,
      .label = "Resampling",
      .section = "Path Tracing Resampling",
      .tooltip = "ReSTIR PT temporal and spatial reuse passes.",
      .labels = {"Off", "Temporal", "Spatial", "Temporal + Spatial"},
      .min = 0.f,
      .max = 3.f,
      .format = "%d",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RestirPtTemporalDepthThreshold",
      .binding = &pt::g_restir_temporal_depth_threshold,
      .value_type = SettingValueType::FLOAT,
      .default_value = 0.1f,
      .label = "Temporal Depth Threshold",
      .section = "Path Tracing Resampling",
      .tooltip = "ReSTIR PT temporal depthThreshold.",
      .min = 0.f,
      .max = 1.f,
      .format = "%.3f",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RestirPtTemporalNormalThreshold",
      .binding = &pt::g_restir_temporal_normal_threshold,
      .value_type = SettingValueType::FLOAT,
      .default_value = 0.6f,
      .label = "Temporal Normal Threshold",
      .section = "Path Tracing Resampling",
      .tooltip = "ReSTIR PT temporal normalThreshold.",
      .min = 0.f,
      .max = 1.f,
      .format = "%.3f",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RestirPtTemporalVisibility",
      .binding = &pt::g_restir_temporal_visibility,
      .value_type = SettingValueType::INTEGER,
      .default_value = 1.f,
      .label = "Temporal Visibility",
      .section = "Path Tracing Resampling",
      .tooltip = "Re-tests reused bounce light for blockers after motion (SDK enableVisibilityBeforeCombine).",
      .labels = {"Off", "On"},
      .min = 0.f,
      .max = 1.f,
      .format = "%d",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RestirPtPermutationSampling",
      .binding = &pt::g_restir_permutation_sampling,
      .value_type = SettingValueType::INTEGER,
      .default_value = 0.f,
      .label = "Permutation Sampling",
      .section = "Path Tracing Resampling",
      .tooltip = "ReSTIR PT temporal enablePermutationSampling.",
      .labels = {"Off", "On"},
      .min = 0.f,
      .max = 1.f,
      .format = "%d",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RestirPtSpatialSamples",
      .binding = &pt::g_restir_spatial_samples,
      .value_type = SettingValueType::INTEGER,
      .default_value = 1.f,
      .label = "Spatial Samples",
      .section = "Path Tracing Resampling",
      .tooltip = "ReSTIR PT numSpatialSamples.",
      .min = 1.f,
      .max = 8.f,
      .format = "%d",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RestirPtDisocclusionBoostSamples",
      .binding = &pt::g_restir_disocclusion_boost_samples,
      .value_type = SettingValueType::INTEGER,
      .default_value = 8.f,
      .label = "Disocclusion Boost Samples",
      .section = "Path Tracing Resampling",
      .tooltip = "ReSTIR PT numDisocclusionBoostSamples.",
      .min = 0.f,
      .max = 16.f,
      .format = "%d",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RestirPtSpatialRadius",
      .binding = &pt::g_restir_spatial_radius,
      .value_type = SettingValueType::INTEGER,
      .default_value = 32.f,
      .label = "Spatial Radius",
      .section = "Path Tracing Resampling",
      .tooltip = "ReSTIR PT samplingRadius.",
      .min = 1.f,
      .max = 64.f,
      .format = "%d",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RestirPtSpatialDepthThreshold",
      .binding = &pt::g_restir_spatial_depth_threshold,
      .value_type = SettingValueType::FLOAT,
      .default_value = 0.1f,
      .label = "Spatial Depth Threshold",
      .section = "Path Tracing Resampling",
      .tooltip = "ReSTIR PT spatial depthThreshold.",
      .min = 0.f,
      .max = 1.f,
      .format = "%.3f",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RestirPtSpatialNormalThreshold",
      .binding = &pt::g_restir_spatial_normal_threshold,
      .value_type = SettingValueType::FLOAT,
      .default_value = 0.6f,
      .label = "Spatial Normal Threshold",
      .section = "Path Tracing Resampling",
      .tooltip = "ReSTIR PT spatial normalThreshold.",
      .min = 0.f,
      .max = 1.f,
      .format = "%.3f",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RestirPtBoilingFilter",
      .binding = &pt::g_restir_boiling_filter,
      .value_type = SettingValueType::INTEGER,
      .default_value = 1.f,
      .label = "Boiling Filter",
      .section = "Path Tracing Resampling",
      .tooltip = "ReSTIR PT boiling filter in the temporal pass.",
      .labels = {"Off", "On"},
      .min = 0.f,
      .max = 1.f,
      .format = "%d",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RestirPtBoilingFilterStrength",
      .binding = &pt::g_restir_boiling_filter_strength,
      .value_type = SettingValueType::FLOAT,
      .default_value = 0.2f,
      .label = "Boiling Filter Strength",
      .section = "Path Tracing Resampling",
      .tooltip = "ReSTIR PT boilingFilterStrength.",
      .min = 0.f,
      .max = 1.f,
      .format = "%.2f",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RestirPtCompatibilityNeighbors",
      .binding = &pt::g_restir_compatibility_neighbors,
      .value_type = SettingValueType::INTEGER,
      .default_value = 0.f,
      .label = "Compatibility Neighbors",
      .section = "Path Tracing Resampling",
      .tooltip = "ReSTIR PT spatial enableSpatialHeuristicMode.",
      .labels = {"Off", "On"},
      .min = 0.f,
      .max = 1.f,
      .format = "%d",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RestirPtDecorrelationFactor",
      .binding = &pt::g_restir_decorrelation_factor,
      .value_type = SettingValueType::FLOAT,
      .default_value = 0.4f,
      .label = "Decorrelation Factor",
      .section = "Path Tracing Resampling",
      .tooltip = "ReSTIR PT decorrelationFactor; mode and firefly replacement follow Ray Reconstruction state.",
      .min = 0.f,
      .max = 1.f,
      .format = "%.2f",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RestirPtStableLightRemap",
      .binding = &pt::g_restir_stable_light_remap,
      .value_type = SettingValueType::INTEGER,
      .default_value = 0.f,
      .label = "Stable Light Remap",
      .section = "Path Tracing Resampling",
      .tooltip = "ReSTIR PT temporal light-index remap through the transfer slot maps.",
      .labels = {"Off", "On"},
      .min = 0.f,
      .max = 1.f,
      .format = "%d",
      .is_visible = visible,
  });
}

}  // namespace falcom_world
