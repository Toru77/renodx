#pragma once

// RTAO round 1: settings, reason codes and the distance-fade range rule.
// Inert while the RTAO toggle is off.

#include <algorithm>
#include <atomic>
#include <cmath>
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
// Test rows (advanced, session only: their key is empty, so settings.hpp never saves them).
inline float g_test_zero_motion = 0.f;  // pass B looks up the history at the same pixel
inline float g_test_freeze_noise = 0.f; // pass A uses a fixed slice and seed
inline float g_test_camera_matrix = 0.f; // pass B looks up the history with the camera matrix (force zero motion wins)

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
  float zero_motion, freeze_noise, camera_matrix;
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
inline constexpr size_t kReasonCount = 16u;

// Names of the pass B diagnostics, in index order (stats index = kRtaoStatTemporalFBase + i).
inline constexpr const char* kTemporalDiagnosticNames[16] = {
    "moving_pixels", "motion_max", "motion_sum", "clamp_active",
    "clamp_shift_sum", "alpha_sum", "abs_raw_ao_sum", "no_history", "texel_differs",
    "matrix_lt_0.1", "matrix_lt_0.25", "matrix_lt_0.5", "matrix_lt_1", "matrix_ge_1", "matrix_diff_sum", "matrix_diff_max"};

// Names of the output quality statistics, in index order (stats index = kRtaoStatQualityBase + i).
inline constexpr const char* kQualityStatNames[7] = {
    "ao_sum", "raw_sum", "rough_pairs", "rough_raw_sum", "rough_prev_out_sum", "change_pairs", "change_sum"};

// Mean of a per-pixel sum kept x1000 over count pixels; 0 when count is 0.
inline double MeanFromSum(uint32_t sum_x1000, uint32_t count) {
  return count > 0u ? sum_x1000 / 1000.0 / count : 0.0;
}

// Width of the soft normal weight ramp (HLSL kNormalRampWidth in world_rtao_temporal.cs_5_0.hlsl): full at T, zero at T - width.
inline constexpr float kNormalRampWidth = 0.4f;

// Histogram bin of a motion vs camera-matrix difference in pixels: 0 below 0.1, 1 below 0.25, 2 below 0.5,
// 3 below 1, 4 otherwise. Negative values count as 0; NaN counts as 4 (never in the first bin). The HLSL does the same.
inline uint32_t DiffBin(float diff_px) {
  if (diff_px != diff_px) return 4u;
  if (diff_px < 0.1f) return 0u;
  if (diff_px < 0.25f) return 1u;
  if (diff_px < 0.5f) return 2u;
  if (diff_px < 1.0f) return 3u;
  return 4u;
}

// CPU matrix self-check (no graphics call). identity_err = max |VP * VPinv - I| (a true inverse pair gives ~0; the
// product is the same in either order). prev_diff = max |VP - prevVP| over the 16 elements. Matrices are row-major
// float[16], the layout CameraSnapshot stores.
struct MatrixSelfCheck {
  float identity_err;
  float prev_diff;
};

inline MatrixSelfCheck CheckSceneMatrices(const float vp[16], const float vp_inv[16], const float prev_vp[16]) {
  MatrixSelfCheck result = {0.f, 0.f};
  for (int r = 0; r < 4; ++r) {
    for (int c = 0; c < 4; ++c) {
      float sum = 0.f;
      for (int k = 0; k < 4; ++k) sum += vp[r * 4 + k] * vp_inv[k * 4 + c];
      const float err = std::fabs(sum - (r == c ? 1.f : 0.f));
      if (err > result.identity_err) result.identity_err = err;
    }
  }
  for (int i = 0; i < 16; ++i) {
    const float d = std::fabs(vp[i] - prev_vp[i]);
    if (d > result.prev_diff) result.prev_diff = d;
  }
  return result;
}

// Depth-ratio bin of the highest-weight history tap (P0-B), the same edges as the pass B shader. 0 invalid history
// (distance <= 0 or NaN); 1 below 0.1%; 2 below 0.5%; 3 below 1%; 4 below 2%; 5 below 5%; 6 otherwise (and NaN).
inline uint32_t DepthRatioBin(float history_dist, float ratio) {
  if (!(history_dist > 0.f)) return 0u;
  if (ratio != ratio) return 6u;
  if (ratio < 0.001f) return 1u;
  if (ratio < 0.005f) return 2u;
  if (ratio < 0.01f) return 3u;
  if (ratio < 0.02f) return 4u;
  if (ratio < 0.05f) return 5u;
  return 6u;
}

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

// Denominators of the pass B shares (D0). Pass A and pass B both count every pixel of the dispatch. The four
// neutral classes (sky, normal, region, scaled) are exactly the pixels that pass A marks raw = -1 and pass B
// skips: the traced count is the rest. reset_traced removes the neutral pixels from pass B reset_pixels.
struct TracedDenominators {
  uint32_t traced;
  uint32_t taps;         // 4 bilinear taps per traced pixel
  uint32_t reset_traced;
};

inline TracedDenominators ComputeTracedDenominators(uint32_t pixels, uint32_t sky, uint32_t normal, uint32_t region,
                                                    uint32_t scaled, uint32_t reset_pixels) {
  const uint32_t neutral = sky + normal + region + scaled;
  const uint32_t traced = pixels > neutral ? pixels - neutral : 0u;
  const uint32_t reset_traced = reset_pixels > neutral ? reset_pixels - neutral : 0u;
  return {traced, 4u * traced, reset_traced};
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
  if (a.history_clamp != b.history_clamp) return "history_clamp";
  if (a.debug != b.debug) return "debug";
  if (a.zero_motion != b.zero_motion) return "test_zero_motion";
  if (a.freeze_noise != b.freeze_noise) return "test_freeze_noise";
  if (a.camera_matrix != b.camera_matrix) return "test_camera_matrix";
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

// Fade End is capped by the built region's coverage (tlas_region_size * 0.25 - ray_max);
// Fade Start is capped by the effective End. Stored settings are never rewritten.
inline Fade RtaoEffectiveFade(float tlas_region_size, float ray_max, float start, float end) {
  const float coverage = tlas_region_size * 0.25f - ray_max;
  const float end_eff = std::min(end, coverage);
  return {std::min(start, end_eff), end_eff, coverage};
}

}  // namespace falcom_world::rtao
