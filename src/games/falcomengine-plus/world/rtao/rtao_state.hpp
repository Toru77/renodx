#pragma once

// RTAO round 1: settings, reason codes and the distance-fade range rule.
// Inert while the RTAO toggle is off.

#include <algorithm>
#include <cstdint>

namespace falcom_world::rtao {

// Settings bindings (Ray Tracing tab, section RTAO and its cards).
inline float g_rtao_enabled = 0.f;
inline float g_rtao_radius = 1.f;
inline float g_rtao_strength = 1.f;
inline float g_rtao_samples = 2.f;
inline float g_rtao_ray_max = 2.f;
inline float g_rtao_normal_bias = 0.05f;
inline float g_rtao_two_sided = 1.f;
inline float g_rtao_temporal_enabled = 0.f;
inline float g_rtao_history_weight = 0.9f;
inline float g_rtao_depth_rejection = 0.05f;
inline float g_rtao_normal_rejection = 0.9f;
inline float g_rtao_history_clamp = 1.f;
inline float g_rtao_isfast = 1.f;
inline float g_rtao_spatial_enabled = 0.f;
inline float g_rtao_filter_radius = 2.f;
inline float g_rtao_filter_quality = 1.f;
inline float g_rtao_fade_start = 40.f;
inline float g_rtao_fade_end = 80.f;

enum class Reason : uint8_t {
  Off = 0,
  LiveBvhOff,
  BvhNotReady,
  BvhEmpty,
  RangeTooSmall,
  NoDepth,
  NoNormals,
  NoSceneCbv,
  DeferredList,
  StartupOrResizeGuard,
  PipelineFailed,
  IsfastUnavailable,
  ScaledResolution,
};

inline const char* ReasonName(Reason reason) {
  switch (reason) {
    case Reason::Off: return "off";
    case Reason::LiveBvhOff: return "live_bvh_off";
    case Reason::BvhNotReady: return "bvh_not_ready";
    case Reason::BvhEmpty: return "bvh_empty";
    case Reason::RangeTooSmall: return "range_too_small";
    case Reason::NoDepth: return "no_depth";
    case Reason::NoNormals: return "no_normals";
    case Reason::NoSceneCbv: return "no_scene_cbv";
    case Reason::DeferredList: return "deferred_list";
    case Reason::StartupOrResizeGuard: return "startup_or_resize_guard";
    case Reason::PipelineFailed: return "pipeline_failed";
    case Reason::IsfastUnavailable: return "isfast_unavailable";
    case Reason::ScaledResolution: return "scaled_resolution";
  }
  return "unknown";
}

struct Fade {
  float start;
  float end;
  float coverage;
};

inline bool RtaoRequested() { return g_rtao_enabled > 0.5f; }

// Fade End is capped by the built region's coverage (tlas_region_size * 0.25 - ray_max);
// Fade Start is capped by the effective End. Stored settings are never rewritten.
inline Fade RtaoEffectiveFade(float tlas_region_size, float ray_max, float start, float end) {
  const float coverage = tlas_region_size * 0.25f - ray_max;
  const float end_eff = std::min(end, coverage);
  return {std::min(start, end_eff), end_eff, coverage};
}

}  // namespace falcom_world::rtao
