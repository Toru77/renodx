#pragma once

// DLSS Ray Reconstruction (world/rr): observe the game's D3D11 NGX session and initialize a separate D3D12 NGX runtime.
// The game links the NGX SDK statically, so its NVSDK_NGX_D3D11_* calls land in the driver's _nvngx.dll.

#include <d3d11.h>
#include <d3d11_4.h>
#include <d3d12.h>
#include <nvsdk_ngx.h>
#include <Windows.h>
#include <wrl/client.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <cstdint>
#include <cstdio>
#include <cwchar>
#include <filesystem>
#include <include/reshade.hpp>
#include <memory>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "../../../../utils/detour.hpp"
#include "../../../../utils/directx.hpp"
#include "../../../../utils/log.hpp"
#include "../../../../utils/platform.hpp"
#include "../debug/crash_log.hpp"
#include "../world_state.hpp"
#include "rr_d3d12.hpp"
#include "rr_interop.hpp"
#include "rr_state.hpp"

namespace falcom_world::rr {

inline decltype(&NVSDK_NGX_D3D11_CreateFeature) g_real_create_feature = nullptr;
inline decltype(&NVSDK_NGX_D3D11_EvaluateFeature) g_real_evaluate_feature = nullptr;
inline decltype(&NVSDK_NGX_D3D11_ReleaseFeature) g_real_release_feature = nullptr;
// nvsdk_ngx.h declares NVSDK_NGX_D3D11_Shutdown only under NGX_ENABLE_DEPRECATED_SHUTDOWN; the driver still exports it.
using NgxD3D11ShutdownFn = NVSDK_NGX_Result(NVSDK_CONV*)();
inline NgxD3D11ShutdownFn g_real_shutdown = nullptr;
inline decltype(&NVSDK_NGX_D3D11_Shutdown1) g_real_shutdown1 = nullptr;
// Capture the game identity for the separate D3D12 NGX session, while passing the game's D3D11 init through unchanged.
using NgxD3D11InitFn = NVSDK_NGX_Result(NVSDK_CONV*)(uint64_t, const wchar_t*, ID3D11Device*, NVSDK_NGX_Version);
using NgxD3D11InitExtFn = NVSDK_NGX_Result(NVSDK_CONV*)(uint64_t, const wchar_t*, ID3D11Device*, NVSDK_NGX_Version,
                                                        const NVSDK_NGX_FeatureCommonInfo*);
using NgxInitProjectIdRawFn = NVSDK_NGX_Result(NVSDK_CONV*)(const char*, NVSDK_NGX_EngineType, const char*, const wchar_t*,
                                                            ID3D11Device*, NVSDK_NGX_Version,
                                                            const NVSDK_NGX_FeatureCommonInfo*);
inline NgxD3D11InitFn g_real_init = nullptr;
inline NgxD3D11InitExtFn g_real_init_ext = nullptr;
inline NgxInitProjectIdRawFn g_real_init_with_project_id = nullptr;
inline std::atomic_bool g_ngx_init_seen{false};
// True when the game's Init reached NGX with the native device (unwrapped and succeeded). This controls forwarding the
// game's D3D11 calls with the unwrapped context; RR itself runs through the separate D3D12 runtime.
inline std::atomic<bool> g_ngx_init_native{false};

struct NgxFeatureRecord {
  int feature_id = -1;
  unsigned int width = 0u, height = 0u, out_width = 0u, out_height = 0u;
  int perf_quality = -1, create_flags = 0, output_subrects = 0;
  bool eval_logged = false;
  bool fail_logged = false;
  uint64_t generation = 0u;
};

struct JitterDiag {
  unsigned int handle_id = UINT_MAX;
  uint32_t samples = 0u;
  std::vector<uint64_t> keys;
  bool logged = false;
};

struct NgxState {
  std::shared_mutex mutex;
  std::unordered_map<unsigned int, NgxFeatureRecord> features;
  JitterDiag jitter;
};

inline NgxState g_ngx;
inline std::atomic_uint64_t g_ngx_feature_generation{0u};
inline std::atomic_uint g_rr_active_handle{UINT_MAX};
inline std::atomic_uint64_t g_rr_active_feature_generation{0u};
inline std::shared_mutex g_ngx_arm_mutex;
// Set while ArmNgxHooks runs on this thread: its preload LoadLibraryW passes through HookLoadLibrary*, which must not
// arm again. Restored on every exit.
inline thread_local bool g_in_arm = false;
struct ArmScope {
  ArmScope() : previous(g_in_arm) { g_in_arm = true; }
  ~ArmScope() { g_in_arm = previous; }
  const bool previous;
};

inline std::atomic_bool g_ngx_armed{false};
inline std::atomic_bool g_ngx_arm_attempted{false};
inline std::atomic<uint32_t> g_ngx_init_calls{0u};
inline std::atomic<uint32_t> g_ngx_create_calls{0u};
inline std::atomic<uint32_t> g_ngx_evaluate_calls{0u};
inline std::atomic<uint32_t> g_ngx_release_calls{0u};
inline std::atomic<uint32_t> g_ngx_logged_status_frame{UINT32_MAX};

inline std::string NgxResultHex(NVSDK_NGX_Result result) {
  char buffer[16];
  std::snprintf(buffer, sizeof(buffer), "0x%08X", static_cast<unsigned int>(result));
  return buffer;
}

inline std::string NgxModulePath(HMODULE module) {
  if (module == nullptr) {
    return "none";
  }
  wchar_t path[MAX_PATH];
  GetModuleFileNameW(module, path, MAX_PATH);
  return std::filesystem::path(path).string();
}

inline void SetRrReason(RrReason reason) {
  if (g_rr_reason.exchange(reason, std::memory_order_relaxed) == reason) return;
  if (!RrDebugLogEnabled()) return;
  renodx::utils::log::i(
      "[world-rr] reason frame=", g_state.frame.load(std::memory_order_relaxed), " reason=", RrReasonName(reason),
      " requested=", RrRequested(), " armed=", g_ngx_armed.load(std::memory_order_relaxed),
      " arm_attempted=", g_ngx_arm_attempted.load(std::memory_order_relaxed),
      " init_seen=", g_ngx_init_seen.load(std::memory_order_relaxed),
      " init_native=", g_ngx_init_native.load(std::memory_order_relaxed),
      " running=", g_rr_running.load(std::memory_order_relaxed), " failed=", g_rr_failed.load(std::memory_order_relaxed));
}
inline void ClearRrFailure(const char* reason) {
  const bool was_failed = g_rr_failed.exchange(false, std::memory_order_relaxed);
  g_rr_evidence_failure_dumped.store(false, std::memory_order_relaxed);
  g_rr_failed_result.store(0u, std::memory_order_relaxed);
  g_rr_failed_exception.store(0u, std::memory_order_relaxed);
  if (was_failed && RrDebugLogEnabled()) renodx::utils::log::i("[world-rr] failure latch cleared reason=", reason);
}

inline void SetRrFailure(const char* reason, uint32_t result, uint32_t exception_code) {
  g_rr_failed_result.store(result, std::memory_order_relaxed);
  g_rr_failed_exception.store(exception_code, std::memory_order_relaxed);
  if (!g_rr_failed.exchange(true, std::memory_order_relaxed) && RrDebugLogEnabled()) {
    renodx::utils::log::w("[world-rr] failure latch set reason=", reason,
                          " result=", NgxResultHex(static_cast<NVSDK_NGX_Result>(result)),
                          " exception=", renodx::utils::log::AsPtr(static_cast<uint32_t>(exception_code)));
  }
}
inline NVSDK_NGX_Result NVSDK_CONV HookCreateFeature(
    ID3D11DeviceContext* InDevCtx,
    NVSDK_NGX_Feature InFeatureID,
    NVSDK_NGX_Parameter* InParameters,
    NVSDK_NGX_Handle** OutHandle) {
  if (RrDebugLogEnabled()) g_ngx_create_calls.fetch_add(1u, std::memory_order_relaxed);
  ID3D11DeviceContext* native_context = InDevCtx;
  if (g_ngx_init_native.load()) {
    renodx::utils::directx::NativeFromReShadeProxy(&native_context);
  }
  const NVSDK_NGX_Result result = g_real_create_feature(native_context, InFeatureID, InParameters, OutHandle);
  if (NVSDK_NGX_SUCCEED(result) && InParameters != nullptr && OutHandle != nullptr && *OutHandle != nullptr) {
    unsigned int width = 0u, height = 0u, out_width = 0u, out_height = 0u;
    int perf_quality = -1, create_flags = 0, output_subrects = 0;
    InParameters->Get(NVSDK_NGX_Parameter_Width, &width);
    InParameters->Get(NVSDK_NGX_Parameter_Height, &height);
    InParameters->Get(NVSDK_NGX_Parameter_OutWidth, &out_width);
    InParameters->Get(NVSDK_NGX_Parameter_OutHeight, &out_height);
    InParameters->Get(NVSDK_NGX_Parameter_PerfQualityValue, &perf_quality);
    InParameters->Get(NVSDK_NGX_Parameter_DLSS_Feature_Create_Flags, &create_flags);
    InParameters->Get(NVSDK_NGX_Parameter_DLSS_Enable_Output_Subrects, &output_subrects);

    const NgxFeatureRecord record{
        .feature_id = static_cast<int>(InFeatureID),
        .width = width,
        .height = height,
        .out_width = out_width,
        .out_height = out_height,
        .perf_quality = perf_quality,
        .create_flags = create_flags,
        .output_subrects = output_subrects,
        .generation = g_ngx_feature_generation.fetch_add(1u, std::memory_order_relaxed) + 1u,
    };
    const unsigned int handle_id = (*OutHandle)->Id;
    {
      std::unique_lock lock(g_ngx.mutex);
      g_ngx.features[handle_id] = record;
    }
    if (RrDebugLogEnabled()) {
      renodx::utils::log::i(
          "[world-rr] create feature=", record.feature_id, " handle=", handle_id,
          " in=", width, "x", height, " out=", out_width, "x", out_height,
          " perf=", perf_quality, " flags=", renodx::utils::log::AsPtr(static_cast<uint32_t>(create_flags)),
          " subrects=", output_subrects, " result=", NgxResultHex(result),
          " ctx=", renodx::utils::log::AsPtr(InDevCtx), " passed=", renodx::utils::log::AsPtr(native_context),
          " thread=", GetCurrentThreadId());
    }
  }
  return result;
}

inline NVSDK_NGX_Result NVSDK_CONV HookEvaluateFeature(
    ID3D11DeviceContext* InDevCtx,
    const NVSDK_NGX_Handle* InFeatureHandle,
    const NVSDK_NGX_Parameter* InParameters,
    PFN_NVSDK_NGX_ProgressCallback InCallback) {
  const bool rr_debug = RrDebugLogEnabled();
  if (rr_debug) g_ngx_evaluate_calls.fetch_add(1u, std::memory_order_relaxed);
  const unsigned int handle_id = InFeatureHandle != nullptr ? InFeatureHandle->Id : UINT_MAX;
  ID3D11DeviceContext* native_context = InDevCtx;
  if (g_ngx_init_native.load(std::memory_order_relaxed)) {
    renodx::utils::directx::NativeFromReShadeProxy(&native_context);
  }

  NgxFeatureRecord record;
  bool known = false;
  bool jitter_sample = false;
  {
    std::shared_lock lock(g_ngx.mutex);
    const auto found = g_ngx.features.find(handle_id);
    known = found != g_ngx.features.end();
    if (known) {
      record = found->second;
    }
    jitter_sample = rr_debug && known && InParameters != nullptr && record.feature_id == NVSDK_NGX_Feature_SuperSampling
                    && (g_ngx.jitter.handle_id == UINT_MAX || g_ngx.jitter.handle_id == handle_id)
                    && g_ngx.jitter.samples < 256u;
  }

  if (rr_debug && known && InParameters != nullptr && !record.eval_logged) {
    const auto describe_resource = [&](const char* name) -> std::string {
      ID3D11Resource* resource = nullptr;
      if (NVSDK_NGX_FAILED(InParameters->Get(name, &resource)) || resource == nullptr) {
        return "null";
      }
      ID3D11Texture2D* texture = nullptr;
      if (FAILED(resource->QueryInterface(IID_PPV_ARGS(&texture)))) {
        return "not-tex2d";
      }
      D3D11_TEXTURE2D_DESC desc = {};
      texture->GetDesc(&desc);
      texture->Release();
      return renodx::utils::log::BuildString(
          desc.Width, "x", desc.Height, " fmt=", static_cast<int>(desc.Format),
          " bind=", renodx::utils::log::AsPtr(desc.BindFlags));
    };
    const auto describe_scalar = [&](const char* name, auto zero) -> std::string {
      decltype(zero) value = zero;
      if (NVSDK_NGX_FAILED(InParameters->Get(name, &value))) {
        return "null";
      }
      return renodx::utils::log::BuildString(value);
    };
    renodx::utils::log::i(
        "[world-rr] eval feature=", record.feature_id, " handle=", handle_id,
        " ctx=", renodx::utils::log::AsPtr(InDevCtx), " thread=", GetCurrentThreadId(),
        " Color=", describe_resource(NVSDK_NGX_Parameter_Color),
        " Output=", describe_resource(NVSDK_NGX_Parameter_Output),
        " Depth=", describe_resource(NVSDK_NGX_Parameter_Depth),
        " MotionVectors=", describe_resource(NVSDK_NGX_Parameter_MotionVectors),
        " ExposureTexture=", describe_resource(NVSDK_NGX_Parameter_ExposureTexture),
        " TransparencyMask=", describe_resource(NVSDK_NGX_Parameter_TransparencyMask),
        " BiasCurrentColorMask=", describe_resource(NVSDK_NGX_Parameter_DLSS_Input_Bias_Current_Color_Mask),
        " jitter=", describe_scalar(NVSDK_NGX_Parameter_Jitter_Offset_X, 0.f), ",",
        describe_scalar(NVSDK_NGX_Parameter_Jitter_Offset_Y, 0.f),
        " mv_scale=", describe_scalar(NVSDK_NGX_Parameter_MV_Scale_X, 0.f), ",",
        describe_scalar(NVSDK_NGX_Parameter_MV_Scale_Y, 0.f),
        " pre_exposure=", describe_scalar(NVSDK_NGX_Parameter_DLSS_Pre_Exposure, 0.f),
        " exposure_scale=", describe_scalar(NVSDK_NGX_Parameter_DLSS_Exposure_Scale, 0.f),
        " reset=", describe_scalar(NVSDK_NGX_Parameter_Reset, 0),
        " subrect=", describe_scalar(NVSDK_NGX_Parameter_DLSS_Render_Subrect_Dimensions_Width, 0u), "x",
        describe_scalar(NVSDK_NGX_Parameter_DLSS_Render_Subrect_Dimensions_Height, 0u));
    std::unique_lock lock(g_ngx.mutex);
    const auto found = g_ngx.features.find(handle_id);
    if (found != g_ngx.features.end()) {
      found->second.eval_logged = true;
    }
  }

  if (jitter_sample && RrDebugLogEnabled()) {
    float jitter_x = 0.f, jitter_y = 0.f;
    InParameters->Get(NVSDK_NGX_Parameter_Jitter_Offset_X, &jitter_x);
    InParameters->Get(NVSDK_NGX_Parameter_Jitter_Offset_Y, &jitter_y);
    const uint64_t key = (static_cast<uint64_t>(std::bit_cast<uint32_t>(jitter_x)) << 32)
                         | std::bit_cast<uint32_t>(jitter_y);
    bool log_now = false;
    std::size_t distinct = 0u;
    {
      std::unique_lock lock(g_ngx.mutex);
      JitterDiag& jitter = g_ngx.jitter;
      if (jitter.handle_id == UINT_MAX) {
        jitter.handle_id = handle_id;
      }
      if (jitter.handle_id == handle_id && jitter.samples < 256u) {
        jitter.samples++;
        if (std::find(jitter.keys.begin(), jitter.keys.end(), key) == jitter.keys.end()) {
          jitter.keys.push_back(key);
        }
        if (jitter.samples == 256u && !jitter.logged) {
          jitter.logged = true;
          log_now = true;
          distinct = jitter.keys.size();
        }
      }
    }
    if (log_now) {
      renodx::utils::log::i("[world-rr] jitter: ", distinct, " distinct positions over 256 evaluates");
    }
  }

  // The game SR call supplies the settings and resources used by the D3D12 RR path.
  if (known && InParameters != nullptr && record.feature_id == NVSDK_NGX_Feature_SuperSampling) {
    g_rr_last_sr_eval_frame.store(g_state.frame.load(std::memory_order_relaxed), std::memory_order_relaxed);
    const int preset_index = std::clamp(static_cast<int>(g_rr_preset_index), 0, 3);
    const int previous_preset = g_rr_preset_seen.exchange(preset_index, std::memory_order_relaxed);
    if (previous_preset != -1 && previous_preset != preset_index) {
      ClearRrFailure("RR preset changed");
      if (const auto runtime = SnapshotRrD3D12Runtime(); runtime != nullptr) {
        runtime->RequestFeatureRetirement();
        runtime->MarkHistoryReset();
      }
    }
    if (RrRequested()) {
      g_rr_last_output_queued.store(false, std::memory_order_relaxed);
      if (g_rr_failed.load(std::memory_order_relaxed)) {
        SetRrReason(RrReason::D3D12NgxFailed);
      } else if (const std::shared_ptr<RrD3D12Runtime> runtime = SnapshotRrD3D12Runtime();
                 runtime != nullptr && runtime->Ready() && !runtime->SubmissionUnsafe()) {
        std::shared_ptr<const RrPreparedInputs> prepared;
        const uint32_t frame = g_state.frame.load(std::memory_order_relaxed);
        const bool staged = PrepareRrD3D12Inputs(runtime, native_context, InParameters, record.width, record.height,
                                                 record.out_width, record.out_height, record.create_flags, record.perf_quality,
                                                 record.output_subrects, frame, &prepared);
        if (staged && prepared != nullptr && prepared->resources != nullptr) {
          const RrBridgeResources& resources = *prepared->resources;
          const RrBridgeFrameMetadata& metadata = prepared->metadata;
          RrD3D12FrameInput rr_input = {
              .color = resources.color.resource12.Get(),
              .motion = resources.motion.resource12.Get(),
              .normal_roughness = resources.normal_roughness.resource12.Get(),
              .albedo = resources.albedo.resource12.Get(),
              .specular = resources.specular.resource12.Get(),
              .depth = resources.depth.resource12.Get(),
              .exposure = resources.has_exposure ? resources.exposure.resource12.Get() : nullptr,
              .output = resources.output.resource12.Get(),
              .output11 = resources.output.texture11.Get(),
              .game_output = prepared->game_output.Get(),
              .input_signal_a = metadata.input_signal_a,
              .generation = metadata.generation,
              .frame = metadata.frame,
              .source_handle = handle_id,
              .source_generation = record.generation,
              .render_width = metadata.render_width,
              .render_height = metadata.render_height,
              .target_width = metadata.output_width,
              .target_height = metadata.output_height,
              .active_width = metadata.active_width,
              .active_height = metadata.active_height,
              .color_base_x = metadata.color_base_x,
              .color_base_y = metadata.color_base_y,
              .depth_base_x = metadata.depth_base_x,
              .depth_base_y = metadata.depth_base_y,
              .motion_base_x = metadata.motion_base_x,
              .motion_base_y = metadata.motion_base_y,
              .output_base_x = metadata.output_base_x,
              .output_base_y = metadata.output_base_y,
              .perf_quality = metadata.perf_quality,
              .create_flags = metadata.create_flags,
              .output_subrects = metadata.output_subrects,
              .jitter_x = metadata.jitter_x,
              .jitter_y = metadata.jitter_y,
              .mv_scale_x = metadata.mv_scale_x,
              .mv_scale_y = metadata.mv_scale_y,
              .pre_exposure = metadata.pre_exposure,
              .exposure_scale = metadata.exposure_scale,
              .reset = metadata.reset,
              .world_to_view = metadata.world_to_view,
              .view_to_clip = metadata.view_to_clip,
              .frame_owner = prepared,
          };
          uint64_t signal_b = 0u;
          const bool submitted = runtime->Submit(rr_input, kRrPresetValues[preset_index], &signal_b);
          const auto submit_outcome = runtime->LastSubmitOutcome();
          g_rr_last_submit_outcome.store(static_cast<uint32_t>(submit_outcome), std::memory_order_relaxed);
          g_rr_last_submit_stage.store(static_cast<uint32_t>(runtime->LastSubmitStage()), std::memory_order_relaxed);
          g_rr_last_submit_hr.store(runtime->LastSubmitHr(), std::memory_order_relaxed);
          g_rr_last_submit_result.store(runtime->LastSubmitResult(), std::memory_order_relaxed);
          g_rr_last_submit_exception.store(runtime->LastSubmitException(), std::memory_order_relaxed);
          g_rr_last_signal_a.store(runtime->LastSignalA(), std::memory_order_relaxed);
          g_rr_last_signal_b.store(runtime->LastSignalB(), std::memory_order_relaxed);
          g_rr_last_signal_c.store(runtime->LastSignalC(), std::memory_order_relaxed);
          g_rr_last_completed_b.store(runtime->CompletedB(), std::memory_order_relaxed);
          g_rr_last_completed_c.store(runtime->CompletedC(), std::memory_order_relaxed);
          g_rr_last_output_queued.store(submitted, std::memory_order_relaxed);
          if (submitted) {
            g_rr_running.store(true, std::memory_order_relaxed);
            g_rr_failed.store(false, std::memory_order_relaxed);
            g_rr_last_ok_frame.store(frame, std::memory_order_relaxed);
            g_rr_active_handle.store(handle_id, std::memory_order_relaxed);
            g_rr_active_feature_generation.store(record.generation, std::memory_order_relaxed);
            SetRrInteropStatus(runtime->Status());
            SetRrReason(RrReason::Running);
            return NVSDK_NGX_Result_Success;
          }
          if (runtime->LastSubmitWasNgxFailure()) {
            SetRrFailure("D3D12 NGX create/evaluate failed", runtime->LastSubmitResult(), runtime->LastSubmitException());
          }
          if (submit_outcome == RrD3D12Runtime::SubmitOutcome::NgxFailure
              || submit_outcome == RrD3D12Runtime::SubmitOutcome::DeviceUnsafe) {
            runtime->RecordFailureEvidence();
            if (RrDebugLogEnabled() && !g_rr_evidence_failure_dumped.exchange(true, std::memory_order_relaxed)) {
              DumpGpuEvidence();
            }
          }
          SetRrInteropStatus(runtime->Status());
          switch (submit_outcome) {
            case RrD3D12Runtime::SubmitOutcome::Busy:
              SetRrReason(RrReason::D3D12Busy);
              break;
            case RrD3D12Runtime::SubmitOutcome::InvalidInput:
              SetRrReason(RrReason::InputsInvalid);
              break;
            case RrD3D12Runtime::SubmitOutcome::NgxFailure:
              SetRrReason(RrReason::D3D12NgxFailed);
              break;
            case RrD3D12Runtime::SubmitOutcome::DeviceUnsafe:
              SetRrReason(RrReason::D3D12Unsafe);
              break;
            case RrD3D12Runtime::SubmitOutcome::None:
            case RrD3D12Runtime::SubmitOutcome::Success:
            case RrD3D12Runtime::SubmitOutcome::Failed:
              SetRrReason(RrReason::D3D12BridgeFailed);
              break;
          }
        } else {
          g_rr_last_output_queued.store(false, std::memory_order_relaxed);
          if (staged) SetRrInteropStatus("RR bridge returned no prepared frame inputs");
          SetRrReason(RrReason::InputsInvalid);
        }
        g_rr_running.store(false, std::memory_order_relaxed);
        runtime->MarkHistoryReset();
      }
    }
  } else if (!known && RrRequested()) {
    SetRrReason(RrReason::CreateNotSeen);
  }

  const NVSDK_NGX_Result result = g_real_evaluate_feature(native_context, InFeatureHandle, InParameters, InCallback);
  const bool log_rr_failure = RrDebugLogEnabled();
  if (NVSDK_NGX_FAILED(result) && log_rr_failure) {
    bool log_failure = false;
    {
      std::unique_lock lock(g_ngx.mutex);
      const auto found = g_ngx.features.find(handle_id);
      if (found != g_ngx.features.end() && !found->second.fail_logged) {
        found->second.fail_logged = true;
        log_failure = true;
      }
    }
    if (log_failure) {
      renodx::utils::log::w("[world-rr] evaluate failed handle=", handle_id, " result=", NgxResultHex(result));
    }
  }
  return result;
}

inline NVSDK_NGX_Result NVSDK_CONV HookReleaseFeature(NVSDK_NGX_Handle* InHandle) {
  if (RrDebugLogEnabled()) g_ngx_release_calls.fetch_add(1u, std::memory_order_relaxed);
  const unsigned int handle_id = InHandle != nullptr ? InHandle->Id : UINT_MAX;
  bool tracked = false;
  uint64_t source_generation = 0u;
  if (InHandle != nullptr) {
    std::unique_lock lock(g_ngx.mutex);
    const auto found = g_ngx.features.find(handle_id);
    if (found != g_ngx.features.end()) {
      tracked = true;
      source_generation = found->second.generation;
      g_ngx.features.erase(found);
    }
  }
  if (tracked) {
    if (const auto runtime = SnapshotRrD3D12Runtime(); runtime != nullptr) {
      runtime->RequestFeatureRetirement(handle_id, source_generation);
    }
    if (g_rr_active_handle.load(std::memory_order_relaxed) == handle_id
        && g_rr_active_feature_generation.load(std::memory_order_relaxed) == source_generation) {
      g_rr_running.store(false, std::memory_order_relaxed);
      g_rr_active_handle.store(UINT_MAX, std::memory_order_relaxed);
    }
  }
  if (RrDebugLogEnabled()) {
    renodx::utils::log::i("[world-rr] release handle=", handle_id, " tracked=", tracked);
  }
  return g_real_release_feature(InHandle);
}

inline void ClearNgxTracking() {
  {
    std::unique_lock lock(g_ngx.mutex);
    g_ngx.features.clear();
    g_ngx.jitter = {};
  }
}

inline NVSDK_NGX_Result NVSDK_CONV HookShutdown() {
  ClearNgxTracking();
  ShutdownRrInteropDevice(nullptr);
  DestroyRrD3D12Runtime(nullptr);
  ClearNgxIdentity();
  if (RrDebugLogEnabled()) renodx::utils::log::i("[world-rr] shutdown");
  return g_real_shutdown();
}

inline NVSDK_NGX_Result NVSDK_CONV HookShutdown1(ID3D11Device* InDevice) {
  renodx::utils::directx::NativeFromReShadeProxy(&InDevice);
  ClearNgxTracking();
  ShutdownRrInteropDevice(InDevice);
  DestroyRrD3D12Runtime(InDevice);
  ClearNgxIdentity(InDevice);
  if (RrDebugLogEnabled()) {
    renodx::utils::log::i("[world-rr] shutdown1 device=", renodx::utils::log::AsPtr(InDevice));
  }
  return g_real_shutdown1(InDevice);
}

// The Init hooks unwrap the device the game passes (a ReShade proxy) before NGX sees it.
inline NVSDK_NGX_Result NVSDK_CONV HookInit(
    uint64_t InApplicationId,
    const wchar_t* InApplicationDataPath,
    ID3D11Device* InDevice,
    NVSDK_NGX_Version InSDKVersion) {
  if (RrDebugLogEnabled()) g_ngx_init_calls.fetch_add(1u, std::memory_order_relaxed);
  g_ngx_init_seen.store(true);
  const bool unwrapped = renodx::utils::directx::NativeFromReShadeProxy(&InDevice);
  if (RrDebugLogEnabled()) {
    renodx::utils::log::i("[world-rr] init export=NVSDK_NGX_D3D11_Init device unwrap=", unwrapped ? "ok" : "not-proxy");
  }
  NgxInitIdentity identity = {};
  identity.kind = NgxInitKind::ApplicationId;
  identity.application_id = InApplicationId;
  identity.application_data_path = CopyNgxWideString(InApplicationDataPath);
  identity.sdk_version = InSDKVersion;
  identity.device11 = InDevice;
  const NVSDK_NGX_Result result = g_real_init(InApplicationId, InApplicationDataPath, InDevice, InSDKVersion);
  g_ngx_init_native.store(unwrapped && NVSDK_NGX_SUCCEED(result));
  if (NVSDK_NGX_SUCCEED(result)) PublishNgxIdentity(std::move(identity));
  return result;
}

inline NVSDK_NGX_Result NVSDK_CONV HookInitExt(
    uint64_t InApplicationId,
    const wchar_t* InApplicationDataPath,
    ID3D11Device* InDevice,
    NVSDK_NGX_Version InSDKVersion,
    const NVSDK_NGX_FeatureCommonInfo* InFeatureInfo) {
  if (RrDebugLogEnabled()) g_ngx_init_calls.fetch_add(1u, std::memory_order_relaxed);
  g_ngx_init_seen.store(true);
  const bool unwrapped = renodx::utils::directx::NativeFromReShadeProxy(&InDevice);
  if (RrDebugLogEnabled()) {
    renodx::utils::log::i("[world-rr] init export=NVSDK_NGX_D3D11_Init_Ext device unwrap=", unwrapped ? "ok" : "not-proxy");
  }
  NgxInitIdentity identity = {};
  identity.kind = NgxInitKind::ApplicationId;
  identity.application_id = InApplicationId;
  identity.application_data_path = CopyNgxWideString(InApplicationDataPath);
  identity.sdk_version = InSDKVersion;
  identity.feature_paths = CopyNgxFeaturePaths(InFeatureInfo);
  identity.device11 = InDevice;
  const NVSDK_NGX_Result result =
      g_real_init_ext(InApplicationId, InApplicationDataPath, InDevice, InSDKVersion, InFeatureInfo);
  g_ngx_init_native.store(unwrapped && NVSDK_NGX_SUCCEED(result));
  if (NVSDK_NGX_SUCCEED(result)) PublishNgxIdentity(std::move(identity));
  return result;
}

inline NVSDK_NGX_Result NVSDK_CONV HookInitWithProjectId(
    const char* InProjectId,
    NVSDK_NGX_EngineType InEngineType,
    const char* InEngineVersion,
    const wchar_t* InApplicationDataPath,
    ID3D11Device* InDevice,
    NVSDK_NGX_Version InSDKVersion,
    const NVSDK_NGX_FeatureCommonInfo* InFeatureInfo) {
  if (RrDebugLogEnabled()) g_ngx_init_calls.fetch_add(1u, std::memory_order_relaxed);
  g_ngx_init_seen.store(true);
  const bool rr_debug = RrDebugLogEnabled();
  ID3D11Device* const proxy_device = rr_debug ? InDevice : nullptr;
  const bool unwrapped = renodx::utils::directx::NativeFromReShadeProxy(&InDevice);
  static std::atomic_bool enter_logged{false};
  if (rr_debug && !enter_logged.exchange(true)) {
    renodx::utils::log::i(
        "[world-rr] init export=NVSDK_NGX_D3D11_Init_ProjectID enter device=", renodx::utils::log::AsPtr(proxy_device),
        " native=", renodx::utils::log::AsPtr(InDevice), " unwrap=", (unwrapped ? "ok" : "not-proxy"));
  }
  NgxInitIdentity identity = {};
  identity.kind = NgxInitKind::ProjectId;
  identity.project_id = CopyNgxNarrowString(InProjectId);
  identity.engine_type = InEngineType;
  identity.engine_version = CopyNgxNarrowString(InEngineVersion);
  identity.application_data_path = CopyNgxWideString(InApplicationDataPath);
  identity.sdk_version = InSDKVersion;
  identity.feature_paths = CopyNgxFeaturePaths(InFeatureInfo);
  identity.device11 = InDevice;
  const NVSDK_NGX_Result result = g_real_init_with_project_id(
      InProjectId, InEngineType, InEngineVersion, InApplicationDataPath, InDevice, InSDKVersion, InFeatureInfo);
  g_ngx_init_native.store(unwrapped && NVSDK_NGX_SUCCEED(result));
  if (NVSDK_NGX_SUCCEED(result)) PublishNgxIdentity(std::move(identity));
  static std::atomic_bool exit_logged{false};
  if (rr_debug && !exit_logged.exchange(true)) {
    renodx::utils::log::i("[world-rr] init exit result=", NgxResultHex(result));
  }
  return result;
}

inline const std::array<renodx::utils::detour::Export, 8> kNgxHookExports = {{
    {"NVSDK_NGX_D3D11_CreateFeature", &g_real_create_feature, &HookCreateFeature},
    {"NVSDK_NGX_D3D11_EvaluateFeature", &g_real_evaluate_feature, &HookEvaluateFeature},
    {"NVSDK_NGX_D3D11_ReleaseFeature", &g_real_release_feature, &HookReleaseFeature},
    {"NVSDK_NGX_D3D11_Shutdown", &g_real_shutdown, &HookShutdown},
    {"NVSDK_NGX_D3D11_Shutdown1", &g_real_shutdown1, &HookShutdown1},
    {"NVSDK_NGX_D3D11_Init", &g_real_init, &HookInit},
    {"NVSDK_NGX_D3D11_Init_Ext", &g_real_init_ext, &HookInitExt},
    {"NVSDK_NGX_D3D11_Init_ProjectID", &g_real_init_with_project_id, &HookInitWithProjectId},
}};

inline void ArmNgxHooks(bool preload) {
  if (g_ngx_armed.load() || g_ngx_arm_attempted.load()) {
    return;
  }
  const ArmScope arm_scope;

  // The preload LoadLibraryW runs before g_ngx_arm_mutex is taken: no lock is held across the loader call.
  HMODULE ngx = GetModuleHandleW(L"_nvngx.dll");
  if (ngx == nullptr && preload) {
    const HMODULE driver = GetModuleHandleW(L"nvwgf2umx.dll");
    if (driver != nullptr) {
      wchar_t driver_path[MAX_PATH];
      GetModuleFileNameW(driver, driver_path, MAX_PATH);
      const std::filesystem::path ngx_path = std::filesystem::path(driver_path).parent_path() / L"_nvngx.dll";
      if (std::filesystem::exists(ngx_path)) {
        ngx = LoadLibraryW(ngx_path.c_str());
      }
    }
  }

  std::unique_lock arm_lock(g_ngx_arm_mutex);
  if (g_ngx_armed.load() || g_ngx_arm_attempted.load()) {
    return;
  }
  if (ngx == nullptr) {
    return;
  }
  g_ngx_arm_attempted.store(true);

  renodx::utils::detour::ExportInstallResult install{};
  std::string error;
  try {
    install = renodx::utils::detour::Install(ngx, kNgxHookExports);
    if (RrDebugLogEnabled() && install.error) {
      error = install.error.message();
    }
  } catch (const std::exception& exception) {
    if (RrDebugLogEnabled()) error = exception.what();
  }

  if (install.Active() > 0u) {
    g_ngx_armed.store(true);
  }
  if (RrDebugLogEnabled()) {
    const std::filesystem::path game_dir = renodx::utils::platform::GetCurrentProcessPath().parent_path();
    renodx::utils::log::i(
        "[world-rr] nvngx_dlss.dll version=", renodx::utils::platform::GetFileVersion(game_dir / L"nvngx_dlss.dll"),
        " nvngx_dlssd.dll version=", renodx::utils::platform::GetFileVersion(game_dir / L"nvngx_dlssd.dll"));
    if (install.Active() > 0u) {
      renodx::utils::log::i(
          "[world-rr] hooks armed path=", NgxModulePath(ngx), " requested=", install.requested,
          " installed=", install.installed, " missing=", install.missing, " failed=", install.failed, " error=", error);
    } else {
      renodx::utils::log::w(
          "[world-rr] hooks NOT armed path=", NgxModulePath(ngx), " requested=", install.requested,
          " installed=", install.installed, " missing=", install.missing, " failed=", install.failed, " error=", error);
    }
  }
}

inline void OnInitDeviceRr(reshade::api::device* device) {
  if (device->get_api() == reshade::api::device_api::d3d11) {
    ArmNgxHooks(true);
  }
}

inline void OnDestroyDeviceRr(reshade::api::device* device) {
  if (device == nullptr || device->get_api() != reshade::api::device_api::d3d11) return;
  auto* native_device = reinterpret_cast<ID3D11Device*>(device->get_native());  // NOLINT(performance-no-int-to-ptr)
  renodx::utils::directx::NativeFromReShadeProxy(&native_device);
  if (!NgxIdentityMatchesDevice(native_device)) return;
  g_rr_running.store(false, std::memory_order_relaxed);
  g_rr_active_handle.store(UINT_MAX, std::memory_order_relaxed);
  g_rr_active_feature_generation.store(0u, std::memory_order_relaxed);
  ShutdownRrInteropDevice(native_device);
  DestroyRrD3D12Runtime(native_device);
  ClearNgxIdentity(native_device);
}

inline void OnRrPresent(
    reshade::api::command_queue* queue,
    reshade::api::swapchain* swapchain,
    const reshade::api::rect* source_rect,
    const reshade::api::rect* dest_rect,
    uint32_t dirty_rect_count,
    const reshade::api::rect* dirty_rects) {
  RrRetireCompletedInputCopies();
  const uint32_t frame = g_state.frame.load(std::memory_order_relaxed);
  const bool requested = RrRequested();
  const bool was_requested = g_rr_requested_seen.exchange(requested, std::memory_order_relaxed);
  if (was_requested != requested) {
    ClearRrFailure(requested ? "RR enabled" : "RR disabled");
    g_rr_running.store(false, std::memory_order_relaxed);
    g_rr_last_output_queued.store(false, std::memory_order_relaxed);
    g_rr_last_ok_frame.store(UINT32_MAX, std::memory_order_relaxed);
    if (!requested) g_rr_active_handle.store(UINT_MAX, std::memory_order_relaxed);
    if (const auto runtime = SnapshotRrD3D12Runtime(); runtime != nullptr) {
      runtime->MarkHistoryReset();
      if (!requested) runtime->RequestFeatureRetirement();
    }
  }
  if (!requested) {
    g_rr_running.store(false, std::memory_order_relaxed);
    if (const auto runtime = SnapshotRrD3D12Runtime(); runtime != nullptr) runtime->RequestFeatureRetirement();
    SetRrReason(RrReason::Off);
    ClearRrFailure("RR disabled");
  } else {
    ArmNgxHooks(false);
    if (!g_ngx_armed.load(std::memory_order_relaxed)) {
      SetRrReason(RrReason::HooksNotArmed);
    } else if (!g_ngx_identity_ready.load(std::memory_order_acquire)) {
      SetRrReason(RrReason::WaitingForNgxIdentity);
    } else {
      ID3D11Device* runtime_device = nullptr;
      if (queue != nullptr) {
        reshade::api::device* present_reshade_device = queue->get_device();
        if (present_reshade_device != nullptr && present_reshade_device->get_api() == reshade::api::device_api::d3d11) {
          runtime_device = reinterpret_cast<ID3D11Device*>(present_reshade_device->get_native());  // NOLINT(performance-no-int-to-ptr)
          renodx::utils::directx::NativeFromReShadeProxy(&runtime_device);
        }
      }
      if (runtime_device == nullptr) {
        SetRrReason(RrReason::D3D12RuntimeFailed);
      } else {
        const NgxInitIdentity identity = CopyNgxIdentity();
        if (identity.device11 != runtime_device) {
          SetRrReason(RrReason::WaitingForNgxIdentity);
        } else {
          std::shared_ptr<RrD3D12Runtime> runtime =
              EnsureRrD3D12Runtime(runtime_device, GetModuleHandleW(L"_nvngx.dll"), identity);
          if (runtime->Ready()) {
            if (runtime->SubmissionUnsafe()) {
              g_rr_running.store(false, std::memory_order_relaxed);
              SetRrReason(RrReason::D3D12Unsafe);
              if (RrDebugLogEnabled() && !g_rr_evidence_failure_dumped.exchange(true, std::memory_order_relaxed)) {
                DumpGpuEvidence();
              }
            } else {
              const uint32_t last_ok = g_rr_last_ok_frame.load(std::memory_order_relaxed);
              const bool success_recent = last_ok != UINT32_MAX && frame - last_ok <= 1u;
              if (!success_recent) g_rr_running.store(false, std::memory_order_relaxed);
              const RrReason current_reason = g_rr_reason.load(std::memory_order_relaxed);
              if (!success_recent && current_reason != RrReason::D3D12BridgeFailed
                  && current_reason != RrReason::D3D12Busy && current_reason != RrReason::D3D12NgxFailed
                  && current_reason != RrReason::D3D12Unsafe && current_reason != RrReason::InputsInvalid) {
                SetRrReason(RrReason::D3D12RuntimeReady);
              }
            }
          } else {
            const RrReason failure_reason = runtime->FailureReason();
            if (RrDebugLogEnabled() && g_rr_reason.load(std::memory_order_relaxed) != failure_reason) {
              renodx::utils::log::w("[world-rr] D3D12 runtime: ", runtime->Status());
            }
            SetRrReason(failure_reason);
          }
        }
      }
    }
  }

  PollRrD3D12Lifecycle();

  if (!RrDebugLogEnabled()) return;
  if (frame % 600u != 0u || g_ngx_logged_status_frame.exchange(frame, std::memory_order_relaxed) == frame) return;
  renodx::utils::log::i(
      "[world-rr] status frame=", frame, " armed=", g_ngx_armed.load(std::memory_order_relaxed),
      " arm_attempted=", g_ngx_arm_attempted.load(std::memory_order_relaxed),
      " init_seen=", g_ngx_init_seen.load(std::memory_order_relaxed),
      " init_native=", g_ngx_init_native.load(std::memory_order_relaxed), " requested=", RrRequested(),
      " running=", g_rr_running.load(std::memory_order_relaxed), " reason=", RrReasonName(g_rr_reason.load(std::memory_order_relaxed)),
      " failed=", g_rr_failed.load(std::memory_order_relaxed),
      " ngx_init=", g_ngx_init_calls.load(std::memory_order_relaxed),
      " create=", g_ngx_create_calls.load(std::memory_order_relaxed),
      " ngx_eval_hook_calls=", g_ngx_evaluate_calls.load(std::memory_order_relaxed),
      " release=", g_ngx_release_calls.load(std::memory_order_relaxed),
      " module=", NgxModulePath(GetModuleHandleW(L"_nvngx.dll")));
}

// Loader hooks: arm the NGX hooks the moment _nvngx.dll loads, before the game's first NGX call (its Init). Each hook
// calls the real loader first and holds no lock across it or across ArmNgxHooks.
inline decltype(&LoadLibraryW) g_real_load_library_w = nullptr;
inline decltype(&LoadLibraryExW) g_real_load_library_ex_w = nullptr;
inline decltype(&LoadLibraryA) g_real_load_library_a = nullptr;
inline decltype(&LoadLibraryExA) g_real_load_library_ex_a = nullptr;
inline std::atomic_bool g_loader_hooks_active{false};

inline void ArmOnNgxLoad(HMODULE module) {
  if (module == nullptr) return;
  wchar_t path[MAX_PATH] = {};
  if (GetModuleFileNameW(module, path, MAX_PATH) == 0) return;
  const std::wstring name = std::filesystem::path(path).filename().wstring();
  constexpr std::wstring_view kSuffix = L"_nvngx.dll";
  if (name.size() < kSuffix.size() || _wcsicmp(name.c_str() + name.size() - kSuffix.size(), kSuffix.data()) != 0) return;
  static std::atomic_bool armed_logged{false};
  if (RrDebugLogEnabled() && !armed_logged.exchange(true)) {
    renodx::utils::log::i("[world-rr] armed on load of _nvngx.dll");
  }
  ArmNgxHooks(false);
}

inline HMODULE WINAPI HookLoadLibraryW(LPCWSTR file_name) {
  const HMODULE result = g_real_load_library_w(file_name);
  if (!g_in_arm) {
    ArmOnNgxLoad(result);
  }
  return result;
}

inline HMODULE WINAPI HookLoadLibraryExW(LPCWSTR file_name, HANDLE file, DWORD flags) {
  const HMODULE result = g_real_load_library_ex_w(file_name, file, flags);
  if (!g_in_arm) {
    ArmOnNgxLoad(result);
  }
  return result;
}

inline HMODULE WINAPI HookLoadLibraryA(LPCSTR file_name) {
  const HMODULE result = g_real_load_library_a(file_name);
  if (!g_in_arm) {
    ArmOnNgxLoad(result);
  }
  return result;
}

inline HMODULE WINAPI HookLoadLibraryExA(LPCSTR file_name, HANDLE file, DWORD flags) {
  const HMODULE result = g_real_load_library_ex_a(file_name, file, flags);
  if (!g_in_arm) {
    ArmOnNgxLoad(result);
  }
  return result;
}

inline const std::array<renodx::utils::detour::Export, 4> kLoaderHookExports = {{
    {"LoadLibraryW", &g_real_load_library_w, &HookLoadLibraryW},
    {"LoadLibraryExW", &g_real_load_library_ex_w, &HookLoadLibraryExW},
    {"LoadLibraryA", &g_real_load_library_a, &HookLoadLibraryA},
    {"LoadLibraryExA", &g_real_load_library_ex_a, &HookLoadLibraryExA},
}};

inline void InstallLoaderHooks() {
  try {
    const auto install = renodx::utils::detour::Install(GetModuleHandleW(L"kernel32.dll"), kLoaderHookExports);
    g_loader_hooks_active.store(install.Active() > 0u);
    if (!g_loader_hooks_active.load() && RrDebugLogEnabled()) {
      renodx::utils::log::w("[world-rr] loader hooks NOT installed: init_device preload is the only arming path");
    }
  } catch (const std::exception& exception) {
    if (RrDebugLogEnabled()) renodx::utils::log::w("[world-rr] loader hooks failed: ", exception.what());
  }
}

inline void Use(DWORD fdw_reason) {
  switch (fdw_reason) {
    case DLL_PROCESS_ATTACH: {
      reshade::register_event<reshade::addon_event::init_device>(OnInitDeviceRr);
      reshade::register_event<reshade::addon_event::destroy_device>(OnDestroyDeviceRr);
      reshade::register_event<reshade::addon_event::present>(OnRrPresent);
      InstallLoaderHooks();
      break;
    }
    case DLL_PROCESS_DETACH: {
      reshade::unregister_event<reshade::addon_event::init_device>(OnInitDeviceRr);
      reshade::unregister_event<reshade::addon_event::destroy_device>(OnDestroyDeviceRr);
      reshade::unregister_event<reshade::addon_event::present>(OnRrPresent);
      DestroyRrD3D12Runtime(nullptr, false);
      if (g_loader_hooks_active.load()) {
        try {
          renodx::utils::detour::Uninstall(kLoaderHookExports);
        } catch (...) {
          // No-op: DLL detach cannot safely log or perform recovery work.
        }
        g_loader_hooks_active.store(false);
      }
      if (g_ngx_armed.load()) {
        std::unique_lock arm_lock(g_ngx_arm_mutex);
        try {
          renodx::utils::detour::Uninstall(kNgxHookExports);
        } catch (...) {
          // No-op: DLL detach cannot safely log or perform recovery work.
        }
        g_ngx_armed.store(false);
      }
      break;
    }
    default:
      break;
  }
}

}  // namespace falcom_world::rr
