#pragma once

// RTAO round 1: settings, reason codes and the distance-fade range rule.
// Inert while the RTAO toggle is off.

#include <algorithm>
#include <atomic>
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
inline float g_rtao_temporal_enabled = 1.f;
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
inline float g_rtao_debug = 0.f;       // RTAO Debug (advanced): 0 off, 1 raw AO, 2 accumulated AO, 3 history confidence
inline float g_rtao_discovery = 0.f;   // Two-Sided discovery (advanced, diagnostic): GPU facing test when 1

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
  NoMotion,        // Temporal on but no usable motion: round-1 AO, history invalid (note)
  HistoryReset,    // Temporal history reset for one frame (note)
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
    case Reason::NoMotion: return "no_motion";
    case Reason::HistoryReset: return "history_reset";
  }
  return "unknown";
}

struct Fade {
  float start;
  float end;
  float coverage;
};

// Settings that shape the AO. A change resets the temporal history for one frame (R2-S4).
struct ParameterSnapshot {
  float radius, ray_max, strength, samples, normal_bias, two_sided, isfast, fade_start, fade_end;
  float temporal_enabled, history_weight, depth_rejection, normal_rejection, history_clamp, debug;
  bool operator==(const ParameterSnapshot&) const = default;
};

// Why the temporal history cannot be used this frame (None = history used). Notes, not failures.
enum class ResetReason : uint8_t {
  None = 0,
  Disabled,         // Temporal off: pass B does not run
  TemporalEnabled,  // Temporal turned on this frame (off -> on)
  Resize,           // AO / raw / history targets recreated
  FrameGap,         // a frame without RTAO output in between
  Guard,            // loading wipe pending or resize guard
  Scaled,           // scaled resolution (the shader invalidates those pixels; reserved for the CPU side)
  Parameters,       // a parameter that shapes the AO changed
  NoMotion,         // no usable motion view
  Count,
};

inline const char* ResetReasonName(ResetReason reason) {
  switch (reason) {
    case ResetReason::None: return "none";
    case ResetReason::Disabled: return "disabled";
    case ResetReason::TemporalEnabled: return "temporal_enabled";
    case ResetReason::Resize: return "resize";
    case ResetReason::FrameGap: return "frame_gap";
    case ResetReason::Guard: return "guard";
    case ResetReason::Scaled: return "scaled";
    case ResetReason::Parameters: return "parameters";
    case ResetReason::NoMotion: return "no_motion";
    case ResetReason::Count: break;
  }
  return "unknown";
}

// Motion source check: TAA t3 (rcas) against the RTV4 target. Unknown (0) on either side is not a conflict.
// Different resources: TAA t3 is not proven to be written before lighting, so Temporal must not run.
enum class MotionCheck : uint8_t {
  Ok = 0,
  Conflict,
};

inline MotionCheck CheckMotionResources(uint64_t rcas_res, uint64_t rtv4_res) {
  return (rcas_res != 0u && rtv4_res != 0u && rcas_res != rtv4_res) ? MotionCheck::Conflict : MotionCheck::Ok;
}

// Two-Sided discovery (diagnostic, no culling): CPU histogram of camera opaque candidate draws by cull mode
// (0..3) and front face (0 clockwise, 1 counter-clockwise). Counted only while g_rtao_discovery is on.
inline std::atomic<uint32_t> g_rtao_cull_hist[4][2] = {};

// GPU discovery index (pass A, world_rtao.cs_5_0.hlsl): back winding * 4 + G-buffer normal toward the camera * 2
// + alpha-tested material. Stats index = RTAO_DISC_BASE (16) + this value.
inline uint32_t TwoSidedDiscoveryIndex(bool back_winding, bool normal_toward_camera, bool alpha_tested) {
  return (back_winding ? 4u : 0u) + (normal_toward_camera ? 2u : 0u) + (alpha_tested ? 1u : 0u);
}

// Where this frame's motion came from (panel and json).
enum class MotionSource : uint8_t {
  None = 0,
  Rtv4,
  Conflict,
};

// The inputs of the reset rule for this frame's pass B. last_frame: the last frame that produced RTAO output
// (UINT64_MAX = never).
struct TemporalFrame {
  bool temporal_on;
  bool was_on;          // Temporal was on in the last frame that ran the rule
  bool resized;
  uint64_t frame;
  uint64_t last_frame;
  bool guard;           // loading wipe pending or resize guard
  bool scaled;
  bool params_changed;
  bool motion_ok;
};

// Priority: off, then off -> on, resize, frame gap, guard, scaled, parameters, motion.
inline ResetReason TemporalResetReason(const TemporalFrame& f) {
  if (!f.temporal_on) return ResetReason::Disabled;
  if (!f.was_on) return ResetReason::TemporalEnabled;
  if (f.resized) return ResetReason::Resize;
  if (f.last_frame == UINT64_MAX || f.frame > f.last_frame + 1u) return ResetReason::FrameGap;
  if (f.guard) return ResetReason::Guard;
  if (f.scaled) return ResetReason::Scaled;
  if (f.params_changed) return ResetReason::Parameters;
  if (!f.motion_ok) return ResetReason::NoMotion;
  return ResetReason::None;
}

inline bool RtaoRequested() { return g_rtao_enabled > 0.5f; }

// Fade End is capped by the built region's coverage (tlas_region_size * 0.25 - ray_max);
// Fade Start is capped by the effective End. Stored settings are never rewritten.
inline Fade RtaoEffectiveFade(float tlas_region_size, float ray_max, float start, float end) {
  const float coverage = tlas_region_size * 0.25f - ray_max;
  const float end_eff = std::min(end, coverage);
  return {std::min(start, end_eff), end_eff, coverage};
}

}  // namespace falcom_world::rtao
