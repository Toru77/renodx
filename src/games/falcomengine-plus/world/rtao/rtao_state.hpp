#pragma once

// Temporal reset rule, motion-resource check and trace-extent helpers shared by the DXR scene passes.

#include <algorithm>
#include <cstdint>

namespace falcom_world::rtao {

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

// The inputs of the reset rule for this frame's pass B. last_frame: the last frame that produced output
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

// Trace Resolution setting (0 Full, 1 Half, 2 Quarter; GTVBGI also 3 = 1/8) to the block side d: one trace
// pixel per d x d pixels. max_index caps the setting (2 for the RT effects).
inline uint32_t RtgiTraceDivisor(float setting, int max_index = 2) {
  return 1u << std::clamp(static_cast<int>(setting + 0.5f), 0, max_index);
}

// Trace pixels along one axis of full pixels (the last block may be partial).
inline uint32_t RtgiTraceExtent(uint32_t full, uint32_t divisor) { return (full + divisor - 1u) / divisor; }

}  // namespace falcom_world::rtao
