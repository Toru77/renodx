#pragma once

// DLSS Ray Reconstruction (world/docs/ROADMAP.md, Reference: DLSS Ray Reconstruction): settings, vendor gate and the guide status.
// Harness-safe: no NGX and no graphics API include here (rr_ngx.hpp owns the NGX side).

#include <atomic>
#include <cstdint>

#include "../world_state.hpp"

namespace falcom_world::rr {

// Settings binding (Ray Tracing tab): RrEnable BOOLEAN row.
inline float g_rr_enabled = 0.f;  // DLSS Ray Reconstruction: 1 On (default), 0 Off
inline float g_rr_probe_enabled = 0.f;  // RR create probe (Ray Tracing tab): 1 runs it once per session, 0 off (default)
inline float g_rr_perf_present = 1.f;  // TEMP perf test (remove)
inline float g_rr_perf_guides = 1.f;  // TEMP perf test (remove)
inline float g_rr_perf_rows = 1.f;  // TEMP perf test (remove)
// RR preset (Ray Tracing tab): 0 Default, 1 D, 2 E, 3 F. Values are the NGX Hint.Render.Preset keys.
inline float g_rr_preset_index = 0.f;
inline constexpr unsigned int kRrPresetValues[4] = {0u, 4u, 5u, 6u};
// Preset index of the last evaluate: a change clears the failure latch so one new try runs.
inline std::atomic_int g_rr_preset_seen{-1};
// RR debug logging (Ray Tracing tab): 1 writes the diagnostic lines; the failure and RR-created lines are always on.
inline float g_rr_debug_log = 0.f;
// Adapter vendor of the device this addon runs on: -1 no device yet, 0 other vendor, 1 NVIDIA (PCI 0x10DE).
inline std::atomic<int> g_rr_gpu_nvidia{-1};

// RR is requested when the game is supported, the adapter is NVIDIA and the row is on.
inline bool RrRequested() { return g_state.supported && g_rr_gpu_nvidia.load() == 1 && g_rr_enabled > 0.5f; }

// Why the RR guide buffers are not (or are) produced this frame; shown on the status row.
enum class GuideReason : uint8_t { Off, InputsMissing, PipelineFailed, TextureFailed, Ready };
inline std::atomic<GuideReason> g_rr_guide_reason{GuideReason::Off};

inline const char* GuideReasonName(GuideReason r) {
  switch (r) {
    case GuideReason::Off:
      return "off";
    case GuideReason::InputsMissing:
      return "waiting for the lighting inputs";
    case GuideReason::PipelineFailed:
      return "pipeline creation failed";
    case GuideReason::TextureFailed:
      return "texture creation failed";
    case GuideReason::Ready:
      return "ready";
  }
  return "off";
}

// Logs each change of the reason once.
inline void SetGuideReason(GuideReason r) {
  const GuideReason previous = g_rr_guide_reason.exchange(r);
  if (previous != r && g_rr_debug_log > 0.5f) {
    renodx::utils::log::i("[world-rr] guides: ", GuideReasonName(r));
  }
}

// Why RR is or is not running this frame (status row). Written by the present handler and the evaluate hook.
enum class RrReason : uint8_t { Off, HooksNotArmed, NoDlss, CreateNotSeen, Unsupported, GuidesNotReady, CreateFailed, EvaluateFailed, Running, InputsInvalid, InitNotNative };
inline std::atomic<RrReason> g_rr_reason{RrReason::Off};

inline const char* RrReasonName(RrReason r) {
  switch (r) {
    case RrReason::Off:
      return "off";
    case RrReason::HooksNotArmed:
      return "NGX hooks not armed (see ReShade.log)";
    case RrReason::NoDlss:
      return "no DLSS evaluate seen (select DLSS in the game's graphics options)";
    case RrReason::CreateNotSeen:
      return "DLSS was set up before RR was switched on: change the DLSS quality once or restart the game";
    case RrReason::Unsupported:
      return "not supported on this GPU or driver";
    case RrReason::GuidesNotReady:
      return "guide buffers not ready";
    case RrReason::CreateFailed:
      return "feature creation failed (switch RR off and on to retry)";
    case RrReason::EvaluateFailed:
      return "evaluate failed (switch RR off and on to retry)";
    case RrReason::Running:
      return "running";
    case RrReason::InputsInvalid:
      return "an RR input failed validation (see ReShade.log)";
    case RrReason::InitNotNative:
      return "NGX was initialized before RR's hooks: restart the game with RR on";
  }
  return "off";
}

// Latched on the first RR create or evaluate failure: no further RR until the game restarts.
inline std::atomic_bool g_rr_failed{false};
inline std::atomic<uint32_t> g_rr_failed_result{0u};
inline std::atomic<uint32_t> g_rr_failed_exception{0u};
// SuperSamplingDenoising capability: -1 not queried yet, 0 unavailable, 1 available (written once per session).
inline std::atomic_int g_rr_available{-1};
inline std::atomic<uint32_t> g_rr_last_ok_frame{UINT32_MAX};
inline std::atomic<uint32_t> g_rr_last_sr_eval_frame{UINT32_MAX};
inline std::atomic_bool g_rr_running{false};
// RR keeps owning the denoise while it produced an evaluate within this many frames.
inline constexpr uint32_t kRrMissFrames = 8u;
inline bool RrOwnsDenoise() { return g_rr_running.load(std::memory_order_relaxed); }
// Called while RR is switched off: the next switch On retries the create and evaluate.
inline void ClearRrFailure() {
  g_rr_failed.store(false);
  g_rr_failed_result.store(0u);
  g_rr_failed_exception.store(0u);
}

}  // namespace falcom_world::rr
