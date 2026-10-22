#pragma once

// RT Local Shadows (world/docs/ROADMAP.md, RT Local Shadows): settings, reason codes. Inert while the toggle is off.
// Independent of RTAO and RTGI.

#include <cstdint>

#include "../rtao/rtao_state.hpp"

namespace falcom_world::rtls {

// Settings bindings (Ray Tracing tab, section RT Local Shadows).
inline float g_rtls_enabled = 0.f;             // RT Local Shadows: BOOLEAN row RtlsEnabled
inline float g_rtls_debug = 0.f;               // RTLS Debug (advanced, session only)
inline float g_rtls_point = 1.f;               // Point
inline float g_rtls_spot = 1.f;                // Spot
inline float g_rtls_env_point = 1.f;           // Env Point
inline float g_rtls_env_spot = 1.f;            // Env Spot
inline float g_rtls_strength = 1.f;            // Strength (lighting shader; no history reset)
inline float g_rtls_max_darkening = 1.f;       // Max Darkening (lighting shader): shadow amount capped at this
inline float g_rtls_samples = 1.f;             // Samples Per Light
inline float g_rtls_max_point_lights = 6.f;    // Max Point Lights Per Pixel
inline float g_rtls_ray_max = 16.f;            // Max Ray Distance
inline float g_rtls_light_size = 0.1f;         // Light Size
inline float g_rtls_light_clearance = 0.2f;    // Light Clearance
inline float g_rtls_normal_bias = 0.004f;      // Normal Bias (RTLS)
inline float g_rtls_trace_characters = 1.f;    // Trace characters
inline float g_rtls_isfast = 1.f;              // IS-FAST Noise
inline float g_rtls_fade_start_pct = 12.5f;    // Fade Start (RTLS Distance Fade)
inline float g_rtls_fade_end_pct = 25.f;       // Fade End (RTLS Distance Fade)
inline float g_rtls_trace_resolution = 1.f;    // Trace Resolution: 0 Full, 1 Half, 2 Quarter
inline float g_rtls_temporal_enabled = 1.f;    // RTLS Temporal Accumulation rows
inline float g_rtls_history_weight = 0.9f;
inline float g_rtls_depth_rejection = 0.05f;
inline float g_rtls_normal_rejection = 0.9f;
inline float g_rtls_history_clamp = 1.f;
inline float g_rtls_spatial_enabled = 1.f;     // RTLS Denoising rows
inline float g_rtls_filter_type = 1.f;         // 0 separable bilateral, 1 a-trous
inline float g_rtls_filter_radius = 2.f;
inline float g_rtls_filter_passes = 1.f;
inline float g_rtls_filter_taps_separable = 5.f;
inline float g_rtls_filter_taps_atrous = 5.f;

enum class Reason : uint8_t { Off = 0, NotSora2nd, InputsMissing, DeferredList, RangeTooSmall, NoLights, NoKinds, PipelineFailed };

inline const char* ReasonName(Reason reason) {
  switch (reason) {
    case Reason::Off: return "off";
    case Reason::NotSora2nd: return "not_sora2nd";
    case Reason::InputsMissing: return "inputs_missing";
    case Reason::DeferredList: return "deferred_list";
    case Reason::RangeTooSmall: return "range_too_small";
    case Reason::NoLights: return "no_lights";
    case Reason::NoKinds: return "no_kinds";
    case Reason::PipelineFailed: return "pipeline_failed";
  }
  return "unknown";
}

inline bool RtlsRequested() { return g_rtls_enabled > 0.5f; }

// Lighting shader flag bits in shader_injection.rtls_flags (RTLS_FLAG_* in lighting_0xCA3D8596.ps_5_0.hlsl).
// Requested: the local contact march is off. Bound: t38 holds this frame's RTLS term. Point/Spot: the specular material
// path; EnvPoint/EnvSpot: the diffuse-only material path.
inline constexpr uint32_t kRtlsFlagRequested = 1u;
inline constexpr uint32_t kRtlsFlagBound = 2u;
inline constexpr uint32_t kRtlsFlagPoint = 4u;
inline constexpr uint32_t kRtlsFlagSpot = 8u;
inline constexpr uint32_t kRtlsFlagEnvPoint = 16u;
inline constexpr uint32_t kRtlsFlagEnvSpot = 32u;

// RTLS settings that shape the traced term; a change resets the RTLS history for one frame (rtao::TemporalResetReason).
// Strength, Max Darkening and the spatial filter act after the history and are not part of it.
struct ParameterSnapshot {
  float samples, max_point_lights, ray_max, light_size, light_clearance, normal_bias, trace_characters, isfast;
  float point, spot, env_point, env_spot, fade_start, fade_end, trace_resolution;
  float temporal_enabled, history_weight, depth_rejection, normal_rejection, history_clamp, debug;
  bool operator==(const ParameterSnapshot&) const = default;
};

}  // namespace falcom_world::rtls
