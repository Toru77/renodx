#pragma once

// DLSS Ray Reconstruction (world/docs/ROADMAP.md, Reference: DLSS Ray Reconstruction): settings, vendor gate and the guide status.
// Harness-safe: no NGX and no graphics API include here (rr_ngx.hpp owns the NGX side).

#include <atomic>
#include <cstdint>

#include "../world_state.hpp"

namespace falcom_world::rr {

// Settings binding (Ray Tracing tab): RrEnable BOOLEAN row.
inline float g_rr_enabled = 0.f;       // DLSS Ray Reconstruction: 1 On (default), 0 Off
inline float g_rr_perf_present = 1.f;  // TEMP perf test (remove)
inline float g_rr_perf_guides = 1.f;   // TEMP perf test (remove)
inline float g_rr_perf_rows = 1.f;     // TEMP perf test (remove)
// RR preset (Ray Tracing tab): 0 Default, 1 D, 2 E, 3 F. Values are the NGX Hint.Render.Preset keys.
inline float g_rr_preset_index = 0.f;
inline constexpr unsigned int kRrPresetValues[4] = {0u, 4u, 5u, 6u};
inline int RrPresetIndex(unsigned int value) {
  for (int index = 0; index < 4; ++index) {
    if (kRrPresetValues[index] == value) return index;
  }
  return -1;
}
inline const char* RrPresetName(int index) {
  static constexpr const char* kNames[] = {"Default", "D", "E", "F"};
  return index >= 0 && index < 4 ? kNames[index] : "Unknown";
}
// Preset index of the last evaluate: a change clears the failure latch so one new try runs.
inline std::atomic_int g_rr_preset_seen{-1};
inline std::atomic_bool g_rr_requested_seen{false};
// RR debug logging (Ray Tracing tab): 1 enables detailed RR logs and recent-event capture.
inline float g_rr_debug_log = 0.f;
inline bool RrDebugLogEnabled() { return g_rr_debug_log > 0.5f; }
// Adapter vendor of the device this addon runs on: -1 no device yet, 0 other vendor, 1 NVIDIA (PCI 0x10DE).
inline std::atomic<int> g_rr_gpu_nvidia{-1};

// RR is requested when the game is supported, the adapter is NVIDIA and the row is on.
inline bool RrRequested() { return g_state.supported && g_rr_gpu_nvidia.load() == 1 && g_rr_enabled > 0.5f; }

// Why the RR guide buffers are not (or are) produced this frame; shown on the status row.
enum class GuideReason : uint8_t { Off,
                                   InputsMissing,
                                   PipelineFailed,
                                   TextureFailed,
                                   Ready };
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
  if (previous != r && RrDebugLogEnabled()) {
    renodx::utils::log::i("[world-rr] guides: ", GuideReasonName(r));
  }
}

// Why RR is or is not running this frame (status row). Written by the present handler and the evaluate hook.
enum class RrReason : uint8_t {
  Off,
  HooksNotArmed,
  NoDlss,
  CreateNotSeen,
  Unsupported,
  GuidesNotReady,
  CreateFailed,
  EvaluateFailed,
  Running,
  InputsInvalid,
  InitNotNative,
  WaitingForNgxIdentity,
  D3D12RuntimeFailed,
  D3D12RuntimeReady,
  D3D12ExportsMissing,
  D3D12DeviceFailed,
  D3D12NgxInitFailed,
  D3D12CapabilityUnavailable,
  D3D12BridgeFailed,
  D3D12BridgeReady,
  D3D12Busy,
  D3D12NgxFailed,
  D3D12Unsafe,
};
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
    case RrReason::WaitingForNgxIdentity:
      return "waiting for game NGX initialization details";
    case RrReason::D3D12RuntimeFailed:
      return "D3D12 NGX runtime initialization failed (see RR Debug Log)";
    case RrReason::D3D12RuntimeReady:
      return "D3D12 runtime ready; waiting for RR inputs";
    case RrReason::D3D12ExportsMissing:
      return "D3D12 NGX exports missing";
    case RrReason::D3D12DeviceFailed:
      return "D3D12 device setup failed";
    case RrReason::D3D12NgxInitFailed:
      return "D3D12 NGX initialization failed";
    case RrReason::D3D12CapabilityUnavailable:
      return "D3D12 RR capability unavailable";
    case RrReason::D3D12BridgeFailed:
      return "D3D12 RR inputs or shared-resource bridge unavailable";
    case RrReason::D3D12BridgeReady:
      return "D3D12 RR inputs staged; using game SR until RR output is queued";
    case RrReason::D3D12Busy:
      return "D3D12 RR is waiting for a free slot or prior feature work; using game SR";
    case RrReason::D3D12NgxFailed:
      return "D3D12 NGX create/evaluate failed; using game SR until retry";
    case RrReason::D3D12Unsafe:
      return "D3D12 RR stopped for this runtime generation; resources retained";
  }
  return "off";
}

// NGX create/evaluate failures are latched until RR is toggled or the preset changes.
inline std::atomic_bool g_rr_failed{false};
inline std::atomic<uint32_t> g_rr_failed_result{0u};
inline std::atomic<uint32_t> g_rr_failed_exception{0u};
inline std::atomic<uint32_t> g_rr_d3d12_failure_hr{0u};
inline std::atomic<uint32_t> g_rr_d3d12_failure_result{0u};
inline std::atomic<uint32_t> g_rr_last_submit_outcome{0u};
inline std::atomic<uint32_t> g_rr_last_submit_stage{0u};
inline std::atomic<uint32_t> g_rr_last_submit_hr{0u};
inline std::atomic<uint32_t> g_rr_last_submit_result{0u};
inline std::atomic<uint32_t> g_rr_last_submit_exception{0u};
inline std::atomic<uint64_t> g_rr_last_signal_a{0u};
inline std::atomic<uint64_t> g_rr_last_signal_b{0u};
inline std::atomic<uint64_t> g_rr_last_signal_c{0u};
inline std::atomic<uint64_t> g_rr_last_completed_b{UINT64_MAX};
inline std::atomic<uint64_t> g_rr_last_completed_c{UINT64_MAX};
inline std::atomic_bool g_rr_last_output_queued{false};
inline std::atomic_bool g_rr_evidence_failure_dumped{false};
enum class RrSubmitOutcome : uint32_t { None,
                                        Success,
                                        Busy,
                                        InvalidInput,
                                        NgxFailure,
                                        DeviceUnsafe,
                                        Failed };
inline const char* RrSubmitOutcomeName(RrSubmitOutcome outcome) {
  switch (outcome) {
    case RrSubmitOutcome::None:
      return "none";
    case RrSubmitOutcome::Success:
      return "success";
    case RrSubmitOutcome::Busy:
      return "busy";
    case RrSubmitOutcome::InvalidInput:
      return "invalid input";
    case RrSubmitOutcome::NgxFailure:
      return "NGX failure";
    case RrSubmitOutcome::DeviceUnsafe:
      return "device unsafe";
    case RrSubmitOutcome::Failed:
      return "failed";
  }
  return "unknown";
}
enum class RrSubmitStage : uint32_t {
  None,
  WaitingForQueue,
  ValidateInputs,
  CreateFeature,
  Evaluate,
  WaitForInputs,
  ExecuteAndSignalB,
  CopyOutputAndSignalC,
  RuntimeUnsafe,
};
inline const char* RrSubmitStageName(RrSubmitStage stage) {
  switch (stage) {
    case RrSubmitStage::None:
      return "none";
    case RrSubmitStage::WaitingForQueue:
      return "waiting for prior queue work / slot";
    case RrSubmitStage::ValidateInputs:
      return "input validation";
    case RrSubmitStage::CreateFeature:
      return "NGX create";
    case RrSubmitStage::Evaluate:
      return "NGX evaluate recording";
    case RrSubmitStage::WaitForInputs:
      return "D3D12 wait for input copies";
    case RrSubmitStage::ExecuteAndSignalB:
      return "D3D12 execution / B signal";
    case RrSubmitStage::CopyOutputAndSignalC:
      return "D3D11 output copy / C signal";
    case RrSubmitStage::RuntimeUnsafe:
      return "runtime safety";
  }
  return "unknown";
}
// SuperSamplingDenoising capability: -1 not queried yet, 0 unavailable, 1 available (written once per session).
inline std::atomic_int g_rr_available{-1};
inline std::atomic<uint32_t> g_rr_last_ok_frame{UINT32_MAX};
inline std::atomic<uint32_t> g_rr_last_sr_eval_frame{UINT32_MAX};
inline std::atomic_bool g_rr_running{false};
// RR keeps owning the denoise while it produced an evaluate within this many frames.
inline constexpr uint32_t kRrMissFrames = 8u;
inline bool RrOwnsDenoise() {
  if (!RrRequested() || g_rr_failed.load(std::memory_order_relaxed) || !g_rr_running.load(std::memory_order_relaxed)) return false;
  const uint32_t last_ok = g_rr_last_ok_frame.load(std::memory_order_relaxed);
  const uint32_t frame = g_state.frame.load(std::memory_order_relaxed);
  // RR evaluation runs after the lighting owners, so a successful frame first affects their next dispatch.
  return last_ok != UINT32_MAX && frame >= last_ok && frame - last_ok <= 1u;
}
// Called while RR is switched off: the next switch On retries the create and evaluate.

}  // namespace falcom_world::rr
