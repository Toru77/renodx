#pragma once

// Ray Tracing tab content.
//
// The tab only exists when DevKit is present (world::AddSettings adds rows only
// when g_state.supported is true). The BVH panel (Pool Scan, GPU debug views)
// is the main workflow; the census panel lists draws per VS family.

#include <algorithm>
#include <cstdio>

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

inline void AddSettings(renodx::utils::settings::Settings* settings, bool supported) {
  if (settings == nullptr) return;
  g_state.supported = supported && IsDevkitPresent();
  if (!g_state.supported) return;

  using Setting = renodx::utils::settings::Setting;
  using SettingValueType = renodx::utils::settings::SettingValueType;
  const auto visible = []() { return g_state.supported; };

  settings->push_back(new Setting{
      .key = "WorldResearchEnabled",
      .binding = &g_state.setting_enabled,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 0.f,
      .label = "Draw Census",
      .section = "Ray Tracing",
      .tooltip = "Enable the Sora 2nd draw census (draws per VS family).",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "WorldBvhPanel",
      .value_type = SettingValueType::CUSTOM,
      .can_reset = false,
      .label = "Phase 1 World BVH",
      .section = "Ray Tracing",
      .on_draw = []() {
        bvh::DrawBvhPanel();
        return false;
      },
      .is_visible = visible,
  });

  const auto not_implemented_temporal = "Round 2 (temporal accumulation). Not implemented yet: this setting does nothing.";
  const auto not_implemented_spatial = "Round 3 (spatial denoising). Not implemented yet: this setting does nothing.";
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
      .max = 10.f,
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
      .format = "%.2f",
      .is_visible = visible,
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
      .key = "RtaoTemporalEnabled",
      .binding = &rtao::g_rtao_temporal_enabled,
      .value_type = SettingValueType::BOOLEAN,
      .default_value = 0.f,
      .label = "Temporal Accumulation (not implemented yet)",
      .section = "Temporal Accumulation",
      .tooltip = not_implemented_temporal,
      .labels = {"Off", "On"},
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtaoHistoryWeight",
      .binding = &rtao::g_rtao_history_weight,
      .value_type = SettingValueType::FLOAT,
      .default_value = 0.9f,
      .label = "History Weight (not implemented yet)",
      .section = "Temporal Accumulation",
      .tooltip = not_implemented_temporal,
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
      .label = "Depth Rejection (not implemented yet)",
      .section = "Temporal Accumulation",
      .tooltip = not_implemented_temporal,
      .min = 0.001f,
      .max = 0.2f,
      .format = "%.3f",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtaoNormalRejection",
      .binding = &rtao::g_rtao_normal_rejection,
      .value_type = SettingValueType::FLOAT,
      .default_value = 0.9f,
      .label = "Normal Rejection (not implemented yet)",
      .section = "Temporal Accumulation",
      .tooltip = not_implemented_temporal,
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
      .label = "History Clamp (not implemented yet)",
      .section = "Temporal Accumulation",
      .tooltip = not_implemented_temporal,
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
      .label = "Spatial Filter (not implemented yet)",
      .section = "Denoising",
      .tooltip = not_implemented_spatial,
      .labels = {"Off", "On"},
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtaoFilterRadius",
      .binding = &rtao::g_rtao_filter_radius,
      .value_type = SettingValueType::INTEGER,
      .default_value = 2.f,
      .label = "Filter Radius (not implemented yet)",
      .section = "Denoising",
      .tooltip = not_implemented_spatial,
      .min = 1.f,
      .max = 8.f,
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtaoFilterQuality",
      .binding = &rtao::g_rtao_filter_quality,
      .value_type = SettingValueType::INTEGER,
      .default_value = 1.f,
      .label = "Filter Quality (not implemented yet)",
      .section = "Denoising",
      .tooltip = not_implemented_spatial,
      .labels = {"Low", "Medium", "High"},
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtaoFadeStart",
      .binding = &rtao::g_rtao_fade_start,
      .value_type = SettingValueType::FLOAT,
      .default_value = 40.f,
      .label = "Fade Start",
      .section = "Distance Fade",
      .tooltip = "Distance from the camera (metres) where RTAO starts fading out. Capped by the built BVH region at use; the panel shows the effective value.",
      .min = 0.f,
      .max = bvh::kPoolRegionMaxSize * 0.25f,
      .format = "%.0f",
      .is_visible = visible,
  });
  settings->push_back(new Setting{
      .key = "RtaoFadeEnd",
      .binding = &rtao::g_rtao_fade_end,
      .value_type = SettingValueType::FLOAT,
      .default_value = 80.f,
      .label = "Fade End",
      .section = "Distance Fade",
      .tooltip = "Distance from the camera (metres) where RTAO is fully faded out. Capped by the built BVH region at use; the panel shows the effective value.",
      .min = 0.f,
      .max = bvh::kPoolRegionMaxSize * 0.25f,
      .format = "%.0f",
      .is_visible = visible,
  });
}

}  // namespace falcom_world
