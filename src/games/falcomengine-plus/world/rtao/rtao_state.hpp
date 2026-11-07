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
inline float g_rtao_foliage_skip = 0.f;  // Foliage skip: RTAO rays ignore INSTANCE_FOLIAGE instances
inline float g_rtao_trace_characters = 0.f;  // Trace characters: RTAO rays hit this frame's skinned characters
inline float g_rtao_trace_resolution = 0.f;  // Trace Resolution: RTAO traces at 0 Full, 1 Half, 2 Quarter
inline float g_rtao_char_intensity = 1.f;  // Character Intensity: RTAO kept on character pixels (lighting shader)
inline float g_rtao_max_darkening = 1.f;  // Max Darkening: AO is never below 1 - this (lighting shader)
inline float g_rtao_full_res_denoise = 0.f;
inline float g_rtao_temporal_enabled = 1.f;
inline float g_rtao_history_weight = 0.8f;
inline float g_rtao_depth_rejection = 0.2f;
inline float g_rtao_normal_rejection = 0.f;
inline float g_rtao_isfast = 1.f;
inline float g_rtao_spatial_enabled = 1.f;
inline float g_rtao_svgf_history_length = 32.f;
inline float g_rtao_svgf_radius_scale = 1.f;
inline float g_rtao_svgf_iterations = 5.f;
inline float g_rtao_svgf_depth_sigma = 1.f;
inline float g_rtao_svgf_normal_power = 128.f;
inline float g_rtao_svgf_luminance_edge_stop = 4.f;
inline float g_rtao_fade_start_pct = 15.625f;  // Fade Start: percent of the BVH range (Region)
inline float g_rtao_fade_end_pct = 31.25f;    // Fade End: percent of the BVH range (Region)
inline float g_rtao_debug = 0.f;       // RTAO Debug (advanced): 0 off, 1 raw AO, 2 denoised AO
inline float g_rtao_discovery = 0.f;   // Two-Sided discovery (advanced, diagnostic): GPU facing test when 1
// Test rows (advanced, session only: their key is empty, so settings.hpp never saves them).
inline float g_test_freeze_noise = 0.f; // pass A uses a fixed slice and seed

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
  FilterFailed,    // Spatial filter could not run: the unfiltered AO is used (note)
  PoolReduced,     // G5: the DXR pass fell back while the CPU pool is reduced: vanilla SSAO this frame
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
    case Reason::FilterFailed: return "filter_failed";
    case Reason::PoolReduced: return "pool_reduced";
  }
  return "unknown";
}

struct Fade {
  float start;
  float end;
  float coverage;  // built BVH range (m); the caps apply until the TLAS catches up
  float start_req;  // requested metres, before the caps
  float end_req;
};

// Settings that shape the AO. A change resets the temporal history for one frame (R2-S4).
struct ParameterSnapshot {
  float radius, ray_max, strength, samples, normal_bias, two_sided, isfast, fade_start, fade_end;  // fade: percent of the range
  float temporal_enabled, history_weight, depth_rejection, normal_rejection, debug;
  float freeze_noise;
  float foliage_skip;
  float trace_characters;
  float trace_resolution;
  float svgf_history_length;
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

// Number of Reason codes (frames without AO are counted per code).
inline constexpr size_t kReasonCount = 17u;

// Pass A sampling slice and seed (b12 c1.z and c1.w). Freeze noise gives 0 and 0 for every frame; otherwise the
// values are the ones the round-1 dispatch always used.
struct NoiseSample {
  float slice_base;
  float seed;
};

inline NoiseSample RtaoNoiseSample(uint64_t frame_index, uint32_t spp, bool freeze) {
  if (freeze) return {0.f, 0.f};
  return {static_cast<float>((frame_index * static_cast<uint64_t>(spp)) % 32u),
          static_cast<float>(frame_index % 1024u)};
}

// Share in percent; 0 when the denominator is 0 (no division).
inline float SharePercent(uint32_t count, uint32_t denominator) {
  return denominator > 0u ? 100.f * static_cast<float>(count) / static_cast<float>(denominator) : 0.f;
}

// First ParameterSnapshot field that differs ("" when none does). Field order is the struct order.
inline const char* ParameterFieldName(const ParameterSnapshot& a, const ParameterSnapshot& b) {
  if (a.radius != b.radius) return "radius";
  if (a.ray_max != b.ray_max) return "ray_max";
  if (a.strength != b.strength) return "strength";
  if (a.samples != b.samples) return "samples";
  if (a.normal_bias != b.normal_bias) return "normal_bias";
  if (a.two_sided != b.two_sided) return "two_sided";
  if (a.isfast != b.isfast) return "isfast";
  if (a.fade_start != b.fade_start) return "fade_start";
  if (a.fade_end != b.fade_end) return "fade_end";
  if (a.temporal_enabled != b.temporal_enabled) return "temporal_enabled";
  if (a.history_weight != b.history_weight) return "history_weight";
  if (a.depth_rejection != b.depth_rejection) return "depth_rejection";
  if (a.normal_rejection != b.normal_rejection) return "normal_rejection";
  if (a.debug != b.debug) return "debug";
  if (a.freeze_noise != b.freeze_noise) return "test_freeze_noise";
  if (a.foliage_skip != b.foliage_skip) return "foliage_skip";
  if (a.trace_characters != b.trace_characters) return "trace_characters";
  if (a.trace_resolution != b.trace_resolution) return "trace_resolution";
  if (a.svgf_history_length != b.svgf_history_length) return "svgf_history_length";
  return "";
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

// Fade percents are of the slider range (Region). Fade End is capped by the built BVH range (coverage);
// Fade Start is capped by the effective End. Stored settings are never rewritten.
inline Fade RtaoEffectiveFade(float built_range, float slider_range, float start_pct, float end_pct) {
  const float start_req = start_pct * slider_range / 100.f;
  const float end_req = end_pct * slider_range / 100.f;
  const float end_eff = std::min(end_req, built_range);
  return {std::min(start_req, end_eff), end_eff, built_range, start_req, end_req};
}

// Trace Resolution setting (0 Full, 1 Half, 2 Quarter; GTVBGI also 3 = 1/8) to the block side d: one trace
// pixel per d x d pixels. max_index caps the setting (2 for the RT effects).
inline uint32_t RtgiTraceDivisor(float setting, int max_index = 2) {
  return 1u << std::clamp(static_cast<int>(setting + 0.5f), 0, max_index);
}

// Trace pixels along one axis of full pixels (the last block may be partial).
inline uint32_t RtgiTraceExtent(uint32_t full, uint32_t divisor) { return (full + divisor - 1u) / divisor; }

}  // namespace falcom_world::rtao
