#pragma once

// Separate D3D12 NGX runtime, feature evaluation and fenced image handoff for Falcom's D3D11 game.

#include <d3d11.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <nvsdk_ngx.h>
#include <nvsdk_ngx_defs_dlssd.h>
#include <nvsdk_ngx_helpers_dlssd.h>
#include <Windows.h>
#include <wrl/client.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <format>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "../../../../utils/directx.hpp"
#include "../../../../utils/gpu_fence.hpp"
#include "../../../../utils/log.hpp"
#include "../../../../utils/platform.hpp"
#include "../debug/crash_log.hpp"
#include "rr_state.hpp"

namespace falcom_world::rr {

struct RrD3D12FrameInput {
  ID3D12Resource* color = nullptr;
  ID3D12Resource* motion = nullptr;
  ID3D12Resource* normal_roughness = nullptr;
  ID3D12Resource* albedo = nullptr;
  ID3D12Resource* specular = nullptr;
  ID3D12Resource* depth = nullptr;
  ID3D12Resource* exposure = nullptr;
  ID3D12Resource* output = nullptr;
  ID3D11Texture2D* output11 = nullptr;
  ID3D11Texture2D* game_output = nullptr;
  uint64_t input_signal_a = 0u;
  uint64_t generation = 0u;
  uint32_t frame = UINT32_MAX;
  uint32_t source_handle = UINT32_MAX;
  uint64_t source_generation = 0u;
  uint32_t render_width = 0u;
  uint32_t render_height = 0u;
  uint32_t target_width = 0u;
  uint32_t target_height = 0u;
  uint32_t active_width = 0u;
  uint32_t active_height = 0u;
  uint32_t color_base_x = 0u;
  uint32_t color_base_y = 0u;
  uint32_t depth_base_x = 0u;
  uint32_t depth_base_y = 0u;
  uint32_t motion_base_x = 0u;
  uint32_t motion_base_y = 0u;
  uint32_t output_base_x = 0u;
  uint32_t output_base_y = 0u;
  int perf_quality = -1;
  int create_flags = 0;
  bool output_subrects = false;
  float jitter_x = 0.f;
  float jitter_y = 0.f;
  float mv_scale_x = 1.f;
  float mv_scale_y = 1.f;
  float pre_exposure = 1.f;
  float exposure_scale = 1.f;
  bool reset = false;
  std::array<float, 16> world_to_view = {};
  std::array<float, 16> view_to_clip = {};
  std::shared_ptr<const void> frame_owner;
};

struct RrD3D12FeatureKey {
  uint32_t source_handle = UINT32_MAX;
  uint64_t source_generation = 0u;
  uint32_t render_width = 0u;
  uint32_t render_height = 0u;
  uint32_t target_width = 0u;
  uint32_t target_height = 0u;
  int perf_quality = -1;
  int create_flags = 0;
  bool output_subrects = false;
  uint32_t preset = 0u;

  [[nodiscard]] bool operator==(const RrD3D12FeatureKey& other) const {
    return source_handle == other.source_handle && source_generation == other.source_generation
           && render_width == other.render_width && render_height == other.render_height
           && target_width == other.target_width && target_height == other.target_height && perf_quality == other.perf_quality
           && create_flags == other.create_flags && output_subrects == other.output_subrects && preset == other.preset;
  }
};

enum class NgxInitKind : uint8_t { ApplicationId,
                                   ProjectId };

struct NgxInitIdentity {
  NgxInitKind kind = NgxInitKind::ApplicationId;
  uint64_t application_id = 0u;
  std::string project_id;
  NVSDK_NGX_EngineType engine_type = NVSDK_NGX_ENGINE_TYPE_CUSTOM;
  std::string engine_version;
  std::wstring application_data_path;
  NVSDK_NGX_Version sdk_version = NVSDK_NGX_Version_API;
  std::vector<std::wstring> feature_paths;
  ID3D11Device* device11 = nullptr;
  uint64_t session_generation = 0u;
};

inline std::shared_mutex g_ngx_identity_mutex;
inline NgxInitIdentity g_ngx_identity;
inline std::atomic_bool g_ngx_identity_ready{false};
inline std::atomic_uint64_t g_rr_d3d12_next_runtime_id{1u};
inline std::atomic_uint64_t g_ngx_session_generation{0u};

inline void PublishNgxIdentity(NgxInitIdentity identity) {
  identity.session_generation = g_ngx_session_generation.fetch_add(1u, std::memory_order_relaxed) + 1u;
  {
    const std::unique_lock lock(g_ngx_identity_mutex);
    g_ngx_identity = std::move(identity);
  }
  g_ngx_identity_ready.store(true, std::memory_order_release);
}

inline NgxInitIdentity CopyNgxIdentity() {
  const std::shared_lock lock(g_ngx_identity_mutex);
  return g_ngx_identity;
}

inline bool NgxIdentityMatchesDevice(ID3D11Device* device) {
  const std::shared_lock lock(g_ngx_identity_mutex);
  return g_ngx_identity_ready.load(std::memory_order_acquire) && g_ngx_identity.device11 == device;
}

inline void ClearNgxIdentity(ID3D11Device* device = nullptr) {
  {
    const std::unique_lock lock(g_ngx_identity_mutex);
    if (device != nullptr && g_ngx_identity.device11 != device) return;
    g_ngx_identity = {};
  }
  g_ngx_identity_ready.store(false, std::memory_order_release);
}

inline std::wstring CopyNgxWideString(const wchar_t* value) { return value != nullptr ? std::wstring(value) : std::wstring{}; }
inline std::string CopyNgxNarrowString(const char* value) { return value != nullptr ? std::string(value) : std::string{}; }

inline std::vector<std::wstring> CopyNgxFeaturePaths(const NVSDK_NGX_FeatureCommonInfo* feature_info) {
  std::vector<std::wstring> paths;
  if (feature_info == nullptr || feature_info->PathListInfo.Path == nullptr) return paths;
  const unsigned int path_count = std::min(feature_info->PathListInfo.Length, 32u);
  paths.reserve(path_count);
  for (unsigned int index = 0u; index < path_count; ++index) {
    if (feature_info->PathListInfo.Path[index] != nullptr) paths.emplace_back(feature_info->PathListInfo.Path[index]);
  }
  return paths;
}

class RrD3D12Runtime {
 public:
  using SubmitOutcome = RrSubmitOutcome;
  enum class Failure : uint8_t { None,
                                 Device,
                                 Exports,
                                 NgxInit,
                                 Capability };
  using InitFn = NVSDK_NGX_Result(NVSDK_CONV*)(uint64_t, const wchar_t*, ID3D12Device*, NVSDK_NGX_Version);
  using InitExtFn = NVSDK_NGX_Result(NVSDK_CONV*)(uint64_t, const wchar_t*, ID3D12Device*, NVSDK_NGX_Version,
                                                  const NVSDK_NGX_FeatureCommonInfo*);
  using InitProjectIdFn = NVSDK_NGX_Result(NVSDK_CONV*)(const char*, NVSDK_NGX_EngineType, const char*, const wchar_t*,
                                                        ID3D12Device*, NVSDK_NGX_Version,
                                                        const NVSDK_NGX_FeatureCommonInfo*);
  using GetCapabilityParametersFn = decltype(&NVSDK_NGX_D3D12_GetCapabilityParameters);
  using AllocateParametersFn = decltype(&NVSDK_NGX_D3D12_AllocateParameters);
  using DestroyParametersFn = decltype(&NVSDK_NGX_D3D12_DestroyParameters);
  using CreateFeatureFn = decltype(&NVSDK_NGX_D3D12_CreateFeature);
  using EvaluateFeatureFn = decltype(&NVSDK_NGX_D3D12_EvaluateFeature);
  using ReleaseFeatureFn = decltype(&NVSDK_NGX_D3D12_ReleaseFeature);
  using ShutdownFn = decltype(&NVSDK_NGX_D3D12_Shutdown1);

  struct Slot {
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> list;
    NVSDK_NGX_Parameter* eval_parameters = nullptr;
    uint64_t signal_b = 0u;
    uint64_t signal_c = 0u;
    std::shared_ptr<const void> frame_owner;
  };

  bool Initialize(ID3D11Device* device11, HMODULE ngx_module, const NgxInitIdentity& identity) {
    if (device11 == nullptr || ngx_module == nullptr) return Fail("D3D12 runtime prerequisites missing", E_INVALIDARG);
    if (identity.device11 != device11 || identity.session_generation == 0u) {
      return Fail("D3D12 runtime NGX device/session identity mismatch", E_INVALIDARG);
    }
    runtime_id_ = g_rr_d3d12_next_runtime_id.fetch_add(1u, std::memory_order_relaxed);
    session_generation_ = identity.session_generation;
    device11_ = device11;
    device11_->GetImmediateContext(context11_.GetAddressOf());
    if (context11_ == nullptr) return Fail("D3D11 immediate context unavailable", E_FAIL);

    Microsoft::WRL::ComPtr<IDXGIDevice> dxgi_device;
    Microsoft::WRL::ComPtr<IDXGIAdapter> adapter;
    HRESULT hr = device11_.As(&dxgi_device);
    if (SUCCEEDED(hr)) hr = dxgi_device->GetAdapter(&adapter);
    if (FAILED(hr)) return Fail("Get game adapter", hr);

    static HMODULE d3d12_module_cache = LoadLibraryExW(L"d3d12.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    d3d12_module_ = d3d12_module_cache;
    if (d3d12_module_ == nullptr) return Fail("Load d3d12.dll", HRESULT_FROM_WIN32(GetLastError()));
    const auto create_device = reinterpret_cast<PFN_D3D12_CREATE_DEVICE>(GetProcAddress(d3d12_module_, "D3D12CreateDevice"));
    if (create_device == nullptr) return Fail("Resolve D3D12CreateDevice", HRESULT_FROM_WIN32(GetLastError()));
    hr = create_device(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(wrapper12_.GetAddressOf()));
    if (FAILED(hr)) return Fail("Create D3D12 device", hr);
    ID3D12Device* native_device12 = wrapper12_.Get();
    if (renodx::utils::directx::NativeFromReShadeProxy(&native_device12)) {
      device12_ = native_device12;
    } else {
      device12_ = wrapper12_;
    }

    D3D12_COMMAND_QUEUE_DESC queue_desc = {};
    queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    hr = device12_->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(queue_.GetAddressOf()));
    if (FAILED(hr)) return Fail("Create D3D12 direct queue", hr);
    for (Slot& slot : slots_) {
      hr = device12_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(slot.allocator.GetAddressOf()));
      if (FAILED(hr)) return Fail("Create D3D12 command allocator", hr);
      hr = device12_->CreateCommandList(0u, D3D12_COMMAND_LIST_TYPE_DIRECT, slot.allocator.Get(), nullptr,
                                        IID_PPV_ARGS(slot.list.GetAddressOf()));
      if (FAILED(hr)) return Fail("Create D3D12 command list", hr);
      hr = slot.list->Close();
      if (FAILED(hr)) return Fail("Close initial D3D12 command list", hr);
    }

    try {
      fence11_ = std::make_unique<renodx::utils::gpu_fence::Fence>(device11_.Get());
      fence12_ = std::make_unique<renodx::utils::gpu_fence::Fence>(device12_.Get());
    } catch (const std::exception&) {
      return Fail("Create D3D11/D3D12 fence timelines", E_FAIL);
    }

    ngx_module_ = ngx_module;
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, reinterpret_cast<LPCWSTR>(ngx_module_),
                            &ngx_module_owner_)) {
      return Fail("Retain NGX module", HRESULT_FROM_WIN32(GetLastError()), Failure::Exports);
    }
    if (!ResolveNgx()) return false;
    if (!InitializeNgx(identity)) return false;
    NVSDK_NGX_Result result = get_capability_parameters_(&capability_parameters_);
    if (NVSDK_NGX_FAILED(result) || capability_parameters_ == nullptr) {
      return FailNgx("Get D3D12 capability parameters", result, Failure::Capability);
    }
    int available = 0;
    result = capability_parameters_->Get(NVSDK_NGX_Parameter_SuperSamplingDenoising_Available, &available);
    if (NVSDK_NGX_FAILED(result)) return FailNgx("Query RR capability", result, Failure::Capability);
    if (available == 0) return Fail("D3D12 RR capability unavailable", E_FAIL, Failure::Capability);
    for (Slot& slot : slots_) {
      result = allocate_parameters_(&slot.eval_parameters);
      if (NVSDK_NGX_FAILED(result) || slot.eval_parameters == nullptr) {
        return FailNgx("Allocate D3D12 RR evaluation parameters", result, Failure::NgxInit);
      }
    }
    initialized_ = true;
    g_rr_d3d12_failure_hr.store(0u, std::memory_order_relaxed);
    g_rr_d3d12_failure_result.store(0u, std::memory_order_relaxed);
    status_ = "D3D12 runtime ready";
    if (RrDebugLogEnabled()) {
      renodx::utils::log::i("[world-rr] D3D12 runtime ready runtime=", runtime_id_, " session=", session_generation_);
    }
    return true;
  }

  bool Shutdown(std::chrono::milliseconds wait_budget = std::chrono::seconds(2)) {
    if (shutdown_done_) return true;
    initialized_ = false;
    if (!d3d12_work_may_have_been_queued_ && !d3d11_work_may_have_been_queued_) {
      shutdown_marker12_ = shutdown_marker11_ = 0u;
    }
    if (d3d12_work_may_have_been_queued_ && shutdown_marker12_ == 0u && queue_ != nullptr && fence12_ != nullptr) {
      try {
        shutdown_marker12_ = renodx::utils::gpu_fence::QueueSignal(queue_.Get(), fence12_.get());
      } catch (const std::exception&) {
        shutdown_marker12_ = renodx::utils::gpu_fence::NO_SIGNAL;
      }
    }
    if (d3d11_work_may_have_been_queued_ && shutdown_marker11_ == 0u && context11_ != nullptr && fence11_ != nullptr) {
      try {
        shutdown_marker11_ = renodx::utils::gpu_fence::QueueSignal(context11_.Get(), fence11_.get());
        context11_->Flush();
      } catch (const std::exception&) {
        shutdown_marker11_ = renodx::utils::gpu_fence::NO_SIGNAL;
      }
    }

    const auto deadline = std::chrono::steady_clock::now() + wait_budget;
    bool complete12 = !d3d12_work_may_have_been_queued_;
    bool complete11 = !d3d11_work_may_have_been_queued_;
    bool removed12 = false;
    bool removed11 = false;
    while (!(complete12 || removed12) || !(complete11 || removed11)) {
      if (!complete12 && shutdown_marker12_ != 0u && shutdown_marker12_ != renodx::utils::gpu_fence::NO_SIGNAL) {
        try {
          const uint64_t completed = fence12_->GetCompletedValue();
          complete12 = completed != UINT64_MAX && completed >= shutdown_marker12_;
        } catch (const std::exception&) {
          complete12 = false;
        }
      }
      if (!complete11 && shutdown_marker11_ != 0u && shutdown_marker11_ != renodx::utils::gpu_fence::NO_SIGNAL) {
        try {
          const uint64_t completed = fence11_->GetCompletedValue();
          complete11 = completed != UINT64_MAX && completed >= shutdown_marker11_;
        } catch (const std::exception&) {
          complete11 = false;
        }
      }
      if (!complete12 && device12_ != nullptr) removed12 = FAILED(device12_->GetDeviceRemovedReason());
      if (!complete11 && device11_ != nullptr) removed11 = FAILED(device11_->GetDeviceRemovedReason());
      if ((complete12 || removed12) && (complete11 || removed11)) break;
      if (std::chrono::steady_clock::now() >= deadline) break;
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    if (!(complete12 || removed12) || !(complete11 || removed11)) {
      status_ = std::format("D3D12 runtime retained: shutdown GPU markers incomplete after 2s (D3D12 marker={} complete={} removed={}, D3D11 marker={} complete={} removed={})",
                            shutdown_marker12_, complete12, removed12, shutdown_marker11_, complete11, removed11);
      if (RrDebugLogEnabled()) renodx::utils::log::w("[world-rr] ", status_);
      return false;
    }

    status_ = std::format("D3D12 runtime shutdown safe (D3D12 complete={} removed={}, D3D11 complete={} removed={})",
                          complete12, removed12, complete11, removed11);
    if (capability_parameters_ != nullptr && destroy_parameters_ != nullptr) {
      destroy_parameters_(capability_parameters_);
      capability_parameters_ = nullptr;
    }
    if (feature_ != nullptr && release_feature_ != nullptr) {
      release_feature_(feature_);
      feature_ = nullptr;
    }
    if (feature_parameters_ != nullptr && destroy_parameters_ != nullptr) {
      destroy_parameters_(feature_parameters_);
      feature_parameters_ = nullptr;
    }
    for (Slot& slot : slots_) {
      if (slot.eval_parameters != nullptr && destroy_parameters_ != nullptr) {
        destroy_parameters_(slot.eval_parameters);
        slot.eval_parameters = nullptr;
      }
      slot.frame_owner.reset();
    }
    if (ngx_initialized_) {
      if (shutdown_ != nullptr) shutdown_(device12_.Get());
      ngx_initialized_ = false;
    }
    if (ngx_module_owner_ != nullptr) {
      FreeLibrary(ngx_module_owner_);
      ngx_module_owner_ = nullptr;
    }
    shutdown_done_ = true;
    return true;
  }

  [[nodiscard]] uint64_t RuntimeId() const { return runtime_id_; }
  [[nodiscard]] bool Ready() const { return initialized_; }
  [[nodiscard]] ID3D11Device* Device11() const { return device11_.Get(); }
  [[nodiscard]] ID3D11DeviceContext* Context11() const { return context11_.Get(); }
  [[nodiscard]] ID3D12Device* Device12() const { return device12_.Get(); }
  [[nodiscard]] ID3D12CommandQueue* Queue12() const { return queue_.Get(); }
  [[nodiscard]] renodx::utils::gpu_fence::Fence* Fence11() const { return fence11_.get(); }
  [[nodiscard]] renodx::utils::gpu_fence::Fence* Fence12() const { return fence12_.get(); }
  [[nodiscard]] const std::string& Status() const { return status_; }
  [[nodiscard]] bool SubmissionUnsafe() const { return submission_unsafe_; }
  [[nodiscard]] bool LastSubmitWasNgxFailure() const { return last_submit_ngx_failure_; }
  [[nodiscard]] uint32_t LastSubmitResult() const { return last_submit_result_; }
  [[nodiscard]] uint32_t LastSubmitException() const { return last_submit_exception_; }
  [[nodiscard]] uint32_t LastSubmitHr() const { return static_cast<uint32_t>(last_submit_hr_); }
  [[nodiscard]] RrSubmitStage LastSubmitStage() const { return last_submit_stage_; }
  [[nodiscard]] SubmitOutcome LastSubmitOutcome() const { return last_submit_outcome_; }
  [[nodiscard]] uint64_t SessionGeneration() const { return session_generation_; }
  [[nodiscard]] uint64_t LastSignalB() const { return last_submit_signal_b_; }
  [[nodiscard]] uint64_t LastSignalA() const { return last_submit_signal_a_; }
  [[nodiscard]] uint64_t LastSignalC() const { return last_submit_signal_c_; }
  [[nodiscard]] uint64_t CompletedB() const { return completed_b_; }
  [[nodiscard]] uint64_t CompletedC() const { return completed_c_; }
  void RecordFailureEvidence() const { RecordRrEvidence(GpuEvidenceEvent::RrFailure); }

  void RequestFeatureRetirement(uint32_t source_handle = UINT32_MAX, uint64_t source_generation = 0u) {
    if (feature_ == nullptr) return;
    if (source_handle != UINT32_MAX
        && (feature_key_.source_handle != source_handle || feature_key_.source_generation != source_generation)) return;
    feature_retirement_requested_ = true;
    force_history_reset_ = true;
  }

  void PollLifecycle() {
    if (fence12_ == nullptr || fence11_ == nullptr) return;
    uint64_t completed_b = UINT64_MAX;
    uint64_t completed_c = UINT64_MAX;
    try {
      completed_b = fence12_->GetCompletedValue();
      completed_c = fence11_->GetCompletedValue();
    } catch (const std::exception&) {
      submission_unsafe_ = true;
      status_ = "RR lifecycle fence query failed; resources retained";
      return;
    }
    if (completed_b == UINT64_MAX || completed_c == UINT64_MAX) {
      submission_unsafe_ = true;
      status_ = "RR lifecycle fence reports device loss; resources retained";
      return;
    }
    for (Slot& slot : slots_) {
      if ((slot.signal_b == 0u || slot.signal_b <= completed_b) && (slot.signal_c == 0u || slot.signal_c <= completed_c)) {
        slot.frame_owner.reset();
      }
    }
    if (feature_retirement_requested_ && (last_feature_signal_b_ == 0u || last_feature_signal_b_ <= completed_b)) {
      if (feature_ != nullptr && release_feature_ != nullptr) release_feature_(std::exchange(feature_, nullptr));
      if (feature_parameters_ != nullptr && destroy_parameters_ != nullptr) {
        destroy_parameters_(std::exchange(feature_parameters_, nullptr));
      }
      feature_key_ = {};
      last_feature_signal_b_ = 0u;
      feature_retirement_requested_ = false;
    }
  }

  [[nodiscard]] bool Matches(ID3D11Device* device, uint64_t session_generation) const {
    return device11_.Get() == device && session_generation_ == session_generation;
  }
  [[nodiscard]] bool Matches(ID3D11Device* device) const { return device11_.Get() == device; }

  void MarkHistoryReset() { force_history_reset_ = true; }

  bool QueueWaitBeforeSharedWrites() {
    if (submission_unsafe_) return false;
    d3d11_work_may_have_been_queued_ = true;
    if (last_signal_b_ == 0u) return true;
    uint64_t completed = UINT64_MAX;
    try {
      if (fence12_ != nullptr) completed = fence12_->GetCompletedValue();
    } catch (const std::exception&) {
      submission_unsafe_ = true;
      status_ = "D3D12 completion fence query failed before shared input overwrite";
      return false;
    }
    if (completed == UINT64_MAX) {
      submission_unsafe_ = true;
      status_ = "D3D12 completion fence failed before shared input overwrite";
      return false;
    }
    try {
      if (!renodx::utils::gpu_fence::QueueWait(context11_.Get(), fence12_.get(), last_signal_b_)) {
        submission_unsafe_ = true;
        status_ = "D3D11 queue rejected the D3D12 shared-input completion wait";
        return false;
      }
    } catch (const std::exception&) {
      submission_unsafe_ = true;
      status_ = "D3D11 queue wait for D3D12 shared inputs failed";
      return false;
    }
    return true;
  }

  bool Submit(const RrD3D12FrameInput& input, uint32_t preset, uint64_t* signal_b) {
    last_submit_ngx_failure_ = false;
    last_submit_outcome_ = SubmitOutcome::Failed;
    last_submit_stage_ = RrSubmitStage::ValidateInputs;
    last_submit_hr_ = S_OK;
    last_submit_result_ = 0u;
    last_submit_exception_ = 0u;
    last_submit_signal_a_ = input.input_signal_a;
    last_submit_signal_b_ = 0u;
    last_submit_signal_c_ = 0u;
    last_submit_source_handle_ = input.source_handle;
    last_submit_source_generation_ = input.source_generation;
    last_submit_bridge_generation_ = input.generation;
    last_submit_preset_ = preset;
    RecordRrEvidence(GpuEvidenceEvent::RrBegin);
    PollLifecycle();
    if (!Ready()) {
      status_ = "D3D12 RR runtime is not ready";
      return false;
    }
    if (submission_unsafe_) {
      last_submit_stage_ = RrSubmitStage::RuntimeUnsafe;
      last_submit_outcome_ = SubmitOutcome::DeviceUnsafe;
      status_ = "D3D12 RR runtime generation is unsafe";
      return false;
    }
    uint32_t invalid_mask = 0u;
    std::string invalid_fields;
    auto mark_invalid = [&](bool invalid, uint32_t bit, const char* field) {
      if (!invalid) return;
      invalid_mask |= bit;
      if (RrDebugLogEnabled()) {
        if (!invalid_fields.empty()) invalid_fields += ',';
        invalid_fields += field;
      }
    };
    mark_invalid(signal_b == nullptr, 1u << 0u, "signal_b");
    mark_invalid(input.input_signal_a == 0u, 1u << 1u, "input_signal_a");
    mark_invalid(input.frame_owner == nullptr, 1u << 2u, "frame_owner");
    mark_invalid(input.game_output == nullptr, 1u << 3u, "game_output");
    mark_invalid(input.color == nullptr, 1u << 4u, "color");
    mark_invalid(input.motion == nullptr, 1u << 5u, "motion");
    mark_invalid(input.normal_roughness == nullptr, 1u << 6u, "normal_roughness");
    mark_invalid(input.albedo == nullptr, 1u << 7u, "albedo");
    mark_invalid(input.specular == nullptr, 1u << 8u, "specular");
    mark_invalid(input.depth == nullptr, 1u << 9u, "depth");
    mark_invalid(input.output == nullptr, 1u << 10u, "output");
    mark_invalid(input.output11 == nullptr, 1u << 11u, "output11");
    mark_invalid(input.render_width == 0u, 1u << 12u, "render_width");
    mark_invalid(input.render_height == 0u, 1u << 13u, "render_height");
    mark_invalid(input.target_width == 0u, 1u << 14u, "target_width");
    mark_invalid(input.target_height == 0u, 1u << 15u, "target_height");
    if (invalid_mask != 0u) {
      status_ = std::format("D3D12 RR submit inputs invalid mask=0x{:04X}", invalid_mask);
      last_submit_outcome_ = SubmitOutcome::InvalidInput;
      if (RrDebugLogEnabled()) {
        renodx::utils::log::w("[world-rr] D3D12 submit input validation failed mask=0x",
                              std::format("{:04X}", invalid_mask), " fields=", invalid_fields);
      }
      return false;
    }
    const RrD3D12FeatureKey key = {
        .source_handle = input.source_handle,
        .source_generation = input.source_generation,
        .render_width = input.render_width,
        .render_height = input.render_height,
        .target_width = input.target_width,
        .target_height = input.target_height,
        .perf_quality = input.perf_quality,
        .create_flags = input.create_flags | static_cast<int>(NVSDK_NGX_DLSS_Feature_Flags_IsHDR),
        .output_subrects = input.output_subrects,
        .preset = preset,
    };
    const bool create_feature = feature_ == nullptr || !(key == feature_key_);
    if (feature_retirement_requested_) {
      last_submit_stage_ = RrSubmitStage::WaitingForQueue;
      status_ = "D3D12 RR feature retirement is pending";
      last_submit_outcome_ = SubmitOutcome::Busy;
      force_history_reset_ = true;
      return false;
    }
    uint64_t completed_b = UINT64_MAX;
    uint64_t completed_c = UINT64_MAX;
    try {
      if (fence12_ != nullptr) completed_b = fence12_->GetCompletedValue();
      if (fence11_ != nullptr) completed_c = fence11_->GetCompletedValue();
    } catch (const std::exception&) {
      submission_unsafe_ = true;
      last_submit_stage_ = RrSubmitStage::RuntimeUnsafe;
      last_submit_outcome_ = SubmitOutcome::DeviceUnsafe;
      status_ = "RR slot completion fence query failed";
      return false;
    }
    completed_b_ = completed_b;
    completed_c_ = completed_c;
    if (completed_b == UINT64_MAX) {
      submission_unsafe_ = true;
      last_submit_stage_ = RrSubmitStage::RuntimeUnsafe;
      last_submit_outcome_ = SubmitOutcome::DeviceUnsafe;
      status_ = "D3D12 completion fence reports device loss";
      return false;
    }
    if (create_feature && feature_ != nullptr && last_feature_signal_b_ > completed_b) {
      last_submit_stage_ = RrSubmitStage::WaitingForQueue;
      last_submit_outcome_ = SubmitOutcome::Busy;
      status_ = "D3D12 RR feature change is waiting for the previous feature submission";
      force_history_reset_ = true;
      return false;
    }

    Slot* selected = nullptr;
    if (completed_c == UINT64_MAX) {
      submission_unsafe_ = true;
      last_submit_stage_ = RrSubmitStage::RuntimeUnsafe;
      last_submit_outcome_ = SubmitOutcome::DeviceUnsafe;
      status_ = "D3D11 copyback fence reports device loss";
      return false;
    }
    for (size_t attempt = 0u; attempt < slots_.size(); ++attempt) {
      Slot& slot = slots_[(next_slot_ + attempt) % slots_.size()];
      if ((slot.signal_b == 0u || slot.signal_b <= completed_b) && (slot.signal_c == 0u || slot.signal_c <= completed_c)) {
        selected = &slot;
        next_slot_ = (next_slot_ + attempt + 1u) % slots_.size();
        slot.frame_owner.reset();
        slot.eval_parameters->Reset();
        break;
      }
    }
    if (selected == nullptr) {
      last_submit_stage_ = RrSubmitStage::WaitingForQueue;
      last_submit_outcome_ = SubmitOutcome::Busy;
      status_ = "all D3D12 RR command slots are busy";
      force_history_reset_ = true;
      return false;
    }

    HRESULT hr = selected->allocator->Reset();
    if (SUCCEEDED(hr)) hr = selected->list->Reset(selected->allocator.Get(), nullptr);
    if (FAILED(hr)) {
      submission_unsafe_ = true;
      last_submit_stage_ = RrSubmitStage::RuntimeUnsafe;
      last_submit_hr_ = hr;
      last_submit_outcome_ = SubmitOutcome::DeviceUnsafe;
      status_ = std::format("reset D3D12 RR command slot failed hr=0x{:08X}", static_cast<uint32_t>(hr));
      return false;
    }
    bool created_this_list = false;
    if (create_feature) {
      last_submit_stage_ = RrSubmitStage::CreateFeature;
      RecordRrEvidence(GpuEvidenceEvent::RrPhase);
      if (RrDebugLogEnabled()) {
        renodx::utils::log::i("[world-rr] D3D12 feature create begin runtime=", runtime_id_, " session=", session_generation_,
                              " source=", input.source_handle, " source_gen=", input.source_generation,
                              " preset=", preset, " input=", input.render_width, "x", input.render_height,
                              " output=", input.target_width, "x", input.target_height);
      }
      if (feature_ != nullptr) {
        release_feature_(feature_);
        feature_ = nullptr;
        destroy_parameters_(feature_parameters_);
        feature_parameters_ = nullptr;
      }
      NVSDK_NGX_Result result = allocate_parameters_(&feature_parameters_);
      if (NVSDK_NGX_FAILED(result) || feature_parameters_ == nullptr) {
        last_submit_ngx_failure_ = true;
        last_submit_outcome_ = SubmitOutcome::NgxFailure;
        failure_result_ = static_cast<uint32_t>(result);
        last_submit_result_ = static_cast<uint32_t>(result);
        selected->list->Close();
        status_ = std::format("allocate D3D12 RR feature parameters failed result=0x{:08X}", static_cast<uint32_t>(result));
        return false;
      }
      NVSDK_NGX_Parameter* params = feature_parameters_;
      const int rr_flags = input.create_flags | static_cast<int>(NVSDK_NGX_DLSS_Feature_Flags_IsHDR);
      params->Set(NVSDK_NGX_Parameter_CreationNodeMask, 1u);
      params->Set(NVSDK_NGX_Parameter_VisibilityNodeMask, 1u);
      params->Set(NVSDK_NGX_Parameter_Width, input.render_width);
      params->Set(NVSDK_NGX_Parameter_Height, input.render_height);
      params->Set(NVSDK_NGX_Parameter_OutWidth, input.target_width);
      params->Set(NVSDK_NGX_Parameter_OutHeight, input.target_height);
      params->Set(NVSDK_NGX_Parameter_PerfQualityValue, input.perf_quality);
      params->Set(NVSDK_NGX_Parameter_DLSS_Feature_Create_Flags, rr_flags);
      params->Set(NVSDK_NGX_Parameter_DLSS_Enable_Output_Subrects, input.output_subrects ? 1 : 0);
      params->Set(NVSDK_NGX_Parameter_DLSS_Denoise_Mode, static_cast<int>(NVSDK_NGX_DLSS_Denoise_Mode_DLUnified));
      params->Set(NVSDK_NGX_Parameter_DLSS_Roughness_Mode, static_cast<unsigned int>(NVSDK_NGX_DLSS_Roughness_Mode_Packed));
      params->Set(NVSDK_NGX_Parameter_Use_HW_Depth, static_cast<unsigned int>(NVSDK_NGX_DLSS_Depth_Type_HW));
      for (const char* preset_name : {NVSDK_NGX_Parameter_RayReconstruction_Hint_Render_Preset_DLAA,
                                      NVSDK_NGX_Parameter_RayReconstruction_Hint_Render_Preset_Quality,
                                      NVSDK_NGX_Parameter_RayReconstruction_Hint_Render_Preset_Balanced,
                                      NVSDK_NGX_Parameter_RayReconstruction_Hint_Render_Preset_Performance,
                                      NVSDK_NGX_Parameter_RayReconstruction_Hint_Render_Preset_UltraPerformance,
                                      NVSDK_NGX_Parameter_RayReconstruction_Hint_Render_Preset_UltraQuality}) {
        params->Set(preset_name, preset);
      }
      params->Set(NVSDK_NGX_Parameter_FreeMemOnReleaseFeature, 1);
      uint32_t create_exception = 0u;
      const NVSDK_NGX_Result create_result = InvokeFeatureCall(
          true, selected->list.Get(), NVSDK_NGX_Feature_RayReconstruction, feature_parameters_, &feature_, &create_exception);
      if (NVSDK_NGX_FAILED(create_result) || feature_ == nullptr) {
        last_submit_ngx_failure_ = true;
        last_submit_outcome_ = SubmitOutcome::NgxFailure;
        failure_result_ = static_cast<uint32_t>(create_result);
        last_submit_result_ = static_cast<uint32_t>(create_result);
        last_submit_exception_ = create_exception;
        selected->list->Close();
        if (feature_ != nullptr) release_feature_(std::exchange(feature_, nullptr));
        destroy_parameters_(std::exchange(feature_parameters_, nullptr));
        status_ = std::format("D3D12 RR create failed result=0x{:08X} exception=0x{:08X}",
                              static_cast<uint32_t>(create_result), create_exception);
        if (RrDebugLogEnabled()) {
          renodx::utils::log::w("[world-rr] D3D12 create failed result=", renodx::utils::log::AsPtr(static_cast<uint32_t>(create_result)),
                                " exception=", renodx::utils::log::AsPtr(create_exception));
        }
        return false;
      }
      feature_key_ = key;
      created_this_list = true;
      if (RrDebugLogEnabled()) {
        renodx::utils::log::i("[world-rr] D3D12 feature create recorded runtime=", runtime_id_, " source=", input.source_handle,
                              " source_gen=", input.source_generation, " preset=", preset);
      }
    }

    NVSDK_NGX_Parameter* params = selected->eval_parameters;
    params->Reset();
    std::array<D3D12_RESOURCE_BARRIER, 8> barriers = {};
    std::array<ID3D12Resource*, 8> resources = {input.color, input.motion, input.normal_roughness, input.albedo,
                                                input.specular, input.depth, input.exposure, input.output};
    UINT barrier_count = 0u;
    for (size_t index = 0u; index < resources.size(); ++index) {
      if (resources[index] == nullptr) continue;
      barriers[barrier_count++] = {
          .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
          .Transition = {.pResource = resources[index],
                         .Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
                         .StateBefore = D3D12_RESOURCE_STATE_COMMON,
                         .StateAfter = (index == resources.size() - 1u) ? D3D12_RESOURCE_STATE_UNORDERED_ACCESS
                                                                        : D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE},
      };
    }
    if (barrier_count != 0u) selected->list->ResourceBarrier(barrier_count, barriers.data());

    params->Set(NVSDK_NGX_Parameter_Color, input.color);
    params->Set(NVSDK_NGX_Parameter_Output, input.output);
    params->Set(NVSDK_NGX_Parameter_Depth, input.depth);
    params->Set(NVSDK_NGX_Parameter_MotionVectors, input.motion);
    params->Set(NVSDK_NGX_Parameter_ExposureTexture, input.exposure);
    params->Set(NVSDK_NGX_Parameter_DiffuseAlbedo, input.albedo);
    params->Set(NVSDK_NGX_Parameter_SpecularAlbedo, input.specular);
    params->Set(NVSDK_NGX_Parameter_GBuffer_Albedo, input.albedo);
    params->Set(NVSDK_NGX_Parameter_GBuffer_Normals, input.normal_roughness);
    params->Set(NVSDK_NGX_Parameter_GBuffer_Roughness, input.normal_roughness);
    params->Set(NVSDK_NGX_Parameter_Jitter_Offset_X, input.jitter_x);
    params->Set(NVSDK_NGX_Parameter_Jitter_Offset_Y, input.jitter_y);
    params->Set(NVSDK_NGX_Parameter_MV_Scale_X, input.mv_scale_x);
    params->Set(NVSDK_NGX_Parameter_MV_Scale_Y, input.mv_scale_y);
    params->Set(NVSDK_NGX_Parameter_DLSS_Pre_Exposure, input.pre_exposure);
    params->Set(NVSDK_NGX_Parameter_DLSS_Exposure_Scale, input.exposure_scale);
    const bool reset_history = input.reset || force_history_reset_ || created_this_list
                               || last_success_generation_ != input.generation;
    params->Set(NVSDK_NGX_Parameter_Reset, reset_history ? 1 : 0);
    params->Set(NVSDK_NGX_Parameter_DLSS_Render_Subrect_Dimensions_Width, input.active_width);
    params->Set(NVSDK_NGX_Parameter_DLSS_Render_Subrect_Dimensions_Height, input.active_height);
    params->Set(NVSDK_NGX_Parameter_DLSS_Input_Color_Subrect_Base_X, input.color_base_x);
    params->Set(NVSDK_NGX_Parameter_DLSS_Input_Color_Subrect_Base_Y, input.color_base_y);
    params->Set(NVSDK_NGX_Parameter_DLSS_Input_Depth_Subrect_Base_X, input.depth_base_x);
    params->Set(NVSDK_NGX_Parameter_DLSS_Input_Depth_Subrect_Base_Y, input.depth_base_y);
    params->Set(NVSDK_NGX_Parameter_DLSS_Input_MV_SubrectBase_X, input.motion_base_x);
    params->Set(NVSDK_NGX_Parameter_DLSS_Input_MV_SubrectBase_Y, input.motion_base_y);
    params->Set(NVSDK_NGX_Parameter_DLSS_Output_Subrect_Base_X, input.output_base_x);
    params->Set(NVSDK_NGX_Parameter_DLSS_Output_Subrect_Base_Y, input.output_base_y);
    for (const auto& [key_x, key_y] : {std::pair{NVSDK_NGX_Parameter_DLSS_Input_DiffuseAlbedo_Subrect_Base_X,
                                                 NVSDK_NGX_Parameter_DLSS_Input_DiffuseAlbedo_Subrect_Base_Y},
                                       std::pair{NVSDK_NGX_Parameter_DLSS_Input_SpecularAlbedo_Subrect_Base_X,
                                                 NVSDK_NGX_Parameter_DLSS_Input_SpecularAlbedo_Subrect_Base_Y},
                                       std::pair{NVSDK_NGX_Parameter_DLSS_Input_Normals_Subrect_Base_X,
                                                 NVSDK_NGX_Parameter_DLSS_Input_Normals_Subrect_Base_Y},
                                       std::pair{NVSDK_NGX_Parameter_DLSS_Input_Roughness_Subrect_Base_X,
                                                 NVSDK_NGX_Parameter_DLSS_Input_Roughness_Subrect_Base_Y}}) {
      params->Set(key_x, input.depth_base_x);
      params->Set(key_y, input.depth_base_y);
    }
    params->Set(NVSDK_NGX_Parameter_DLSS_WORLD_TO_VIEW_MATRIX, const_cast<float*>(input.world_to_view.data()));
    params->Set(NVSDK_NGX_Parameter_DLSS_VIEW_TO_CLIP_MATRIX, const_cast<float*>(input.view_to_clip.data()));
    for (const char* key : {NVSDK_NGX_Parameter_TransparencyMask, NVSDK_NGX_Parameter_DLSS_Input_Bias_Current_Color_Mask,
                            NVSDK_NGX_Parameter_GBuffer_Metallic, NVSDK_NGX_Parameter_GBuffer_Specular,
                            NVSDK_NGX_Parameter_GBuffer_Subsurface, NVSDK_NGX_Parameter_GBuffer_ShadingModelId,
                            NVSDK_NGX_Parameter_GBuffer_MaterialId, NVSDK_NGX_Parameter_GBuffer_Atrrib_8,
                            NVSDK_NGX_Parameter_GBuffer_Atrrib_9, NVSDK_NGX_Parameter_GBuffer_Atrrib_11,
                            NVSDK_NGX_Parameter_GBuffer_Atrrib_12, NVSDK_NGX_Parameter_GBuffer_Atrrib_13,
                            NVSDK_NGX_Parameter_GBuffer_Atrrib_14, NVSDK_NGX_Parameter_GBuffer_Atrrib_15,
                            NVSDK_NGX_Parameter_DLSSD_Alpha, NVSDK_NGX_Parameter_DLSSD_OutputAlpha,
                            NVSDK_NGX_Parameter_DLSSD_ReflectedAlbedo, NVSDK_NGX_Parameter_DLSSD_ColorBeforeParticles,
                            NVSDK_NGX_Parameter_DLSSD_ColorAfterParticles, NVSDK_NGX_Parameter_DLSSD_ColorBeforeTransparency,
                            NVSDK_NGX_Parameter_DLSSD_ColorAfterTransparency, NVSDK_NGX_Parameter_DLSSD_ColorBeforeFog,
                            NVSDK_NGX_Parameter_DLSSD_ColorAfterFog, NVSDK_NGX_Parameter_DLSSD_ScreenSpaceSubsurfaceScatteringGuide,
                            NVSDK_NGX_Parameter_DLSSD_ColorBeforeScreenSpaceSubsurfaceScattering,
                            NVSDK_NGX_Parameter_DLSSD_ColorAfterScreenSpaceSubsurfaceScattering,
                            NVSDK_NGX_Parameter_DLSSD_ScreenSpaceRefractionGuide,
                            NVSDK_NGX_Parameter_DLSSD_ColorBeforeScreenSpaceRefraction,
                            NVSDK_NGX_Parameter_DLSSD_ColorAfterScreenSpaceRefraction, NVSDK_NGX_Parameter_DLSSD_DepthOfFieldGuide,
                            NVSDK_NGX_Parameter_DLSSD_ColorBeforeDepthOfField, NVSDK_NGX_Parameter_DLSSD_ColorAfterDepthOfField,
                            NVSDK_NGX_Parameter_DLSSD_DiffuseHitDistance, NVSDK_NGX_Parameter_DLSSD_SpecularHitDistance,
                            NVSDK_NGX_Parameter_DLSSD_DiffuseRayDirection, NVSDK_NGX_Parameter_DLSSD_SpecularRayDirection,
                            NVSDK_NGX_Parameter_DLSSD_DiffuseRayDirectionHitDistance,
                            NVSDK_NGX_Parameter_DLSSD_SpecularRayDirectionHitDistance,
                            NVSDK_NGX_Parameter_DLSS_TransparencyLayer, NVSDK_NGX_Parameter_DLSS_TransparencyLayerOpacity,
                            NVSDK_NGX_Parameter_DLSS_TransparencyLayerMvecs, NVSDK_NGX_Parameter_DLSS_DisocclusionMask}) {
      params->Set(key, static_cast<ID3D12Resource*>(nullptr));
    }

    uint32_t eval_exception = 0u;
    last_submit_stage_ = RrSubmitStage::Evaluate;
    RecordRrEvidence(GpuEvidenceEvent::RrPhase);
    const NVSDK_NGX_Result eval_result = InvokeFeatureCall(
        false, selected->list.Get(), NVSDK_NGX_Feature_RayReconstruction, params, &feature_, &eval_exception);
    if (NVSDK_NGX_FAILED(eval_result)) {
      last_submit_ngx_failure_ = true;
      last_submit_outcome_ = SubmitOutcome::NgxFailure;
      failure_result_ = static_cast<uint32_t>(eval_result);
      last_submit_result_ = static_cast<uint32_t>(eval_result);
      last_submit_exception_ = eval_exception;
      selected->list->Close();
      force_history_reset_ = true;
      status_ = std::format("D3D12 RR evaluate recording failed result=0x{:08X} exception=0x{:08X}",
                            static_cast<uint32_t>(eval_result), eval_exception);
      if (RrDebugLogEnabled()) {
        renodx::utils::log::w("[world-rr] D3D12 evaluate failed result=", renodx::utils::log::AsPtr(static_cast<uint32_t>(eval_result)),
                              " exception=", renodx::utils::log::AsPtr(eval_exception));
      }
      if (created_this_list) {
        release_feature_(std::exchange(feature_, nullptr));
        destroy_parameters_(std::exchange(feature_parameters_, nullptr));
      }
      return false;
    }
    for (UINT index = 0u; index < barrier_count; ++index) {
      std::swap(barriers[index].Transition.StateBefore, barriers[index].Transition.StateAfter);
    }
    if (barrier_count != 0u) selected->list->ResourceBarrier(barrier_count, barriers.data());
    hr = selected->list->Close();
    if (FAILED(hr)) {
      last_submit_hr_ = hr;
      submission_unsafe_ = created_this_list;
      force_history_reset_ = true;
      status_ = std::format("close D3D12 RR command list failed hr=0x{:08X}", static_cast<uint32_t>(hr));
      if (created_this_list) {
        release_feature_(std::exchange(feature_, nullptr));
        destroy_parameters_(std::exchange(feature_parameters_, nullptr));
      }
      return false;
    }
    try {
      last_submit_stage_ = RrSubmitStage::WaitForInputs;
      RecordRrEvidence(GpuEvidenceEvent::RrPhase);
      d3d12_work_may_have_been_queued_ = true;
      if (!renodx::utils::gpu_fence::QueueWait(queue_.Get(), fence11_.get(), input.input_signal_a)) {
        status_ = "D3D12 queue rejected the staged-input fence wait";
        force_history_reset_ = true;
        if (created_this_list) {
          release_feature_(std::exchange(feature_, nullptr));
          destroy_parameters_(std::exchange(feature_parameters_, nullptr));
        }
        return false;
      }
      ID3D12CommandList* lists[] = {selected->list.Get()};
      last_submit_stage_ = RrSubmitStage::ExecuteAndSignalB;
      RecordRrEvidence(GpuEvidenceEvent::RrPhase);
      queue_->ExecuteCommandLists(1u, lists);
      const uint64_t accepted_b = renodx::utils::gpu_fence::QueueSignal(queue_.Get(), fence12_.get());
      if (accepted_b == renodx::utils::gpu_fence::NO_SIGNAL) {
        submission_unsafe_ = true;
        last_submit_outcome_ = SubmitOutcome::DeviceUnsafe;
        status_ = "D3D12 RR executed but its completion fence signal failed; generation quarantined";
        selected->frame_owner = input.frame_owner;
        selected->signal_b = UINT64_MAX;
        force_history_reset_ = true;
        return false;
      }
      selected->signal_b = accepted_b;
      last_submit_signal_b_ = accepted_b;
      selected->frame_owner = input.frame_owner;
      last_signal_b_ = accepted_b;
      last_feature_signal_b_ = accepted_b;
      try {
        last_submit_stage_ = RrSubmitStage::CopyOutputAndSignalC;
        RecordRrEvidence(GpuEvidenceEvent::RrPhase);
        if (!renodx::utils::gpu_fence::QueueWait(context11_.Get(), fence12_.get(), accepted_b)) {
          submission_unsafe_ = true;
          last_submit_outcome_ = SubmitOutcome::DeviceUnsafe;
          selected->signal_c = UINT64_MAX;
          status_ = "D3D11 queue rejected the D3D12 RR output wait; resources retained";
          force_history_reset_ = true;
          return false;
        }
        D3D11_BOX output_box = {input.output_base_x, input.output_base_y, 0u,
                                input.output_base_x + input.target_width, input.output_base_y + input.target_height, 1u};
        context11_->CopySubresourceRegion(input.game_output, 0u, input.output_base_x, input.output_base_y, 0u,
                                          input.output11, 0u, &output_box);
        const uint64_t accepted_c = renodx::utils::gpu_fence::QueueSignal(context11_.Get(), fence11_.get());
        if (accepted_c == renodx::utils::gpu_fence::NO_SIGNAL) {
          submission_unsafe_ = true;
          last_submit_outcome_ = SubmitOutcome::DeviceUnsafe;
          selected->signal_c = UINT64_MAX;
          status_ = "D3D11 RR output copy queued but completion signal failed; resources retained";
          force_history_reset_ = true;
          return false;
        }
        selected->signal_c = accepted_c;
        last_submit_signal_c_ = accepted_c;
        completed_b_ = fence12_->GetCompletedValue();
        completed_c_ = fence11_->GetCompletedValue();
      } catch (const std::exception&) {
        submission_unsafe_ = true;
        selected->signal_c = UINT64_MAX;
        status_ = "D3D11 RR output handoff failed; resources retained";
        force_history_reset_ = true;
        return false;
      }
      force_history_reset_ = false;
      last_submit_outcome_ = SubmitOutcome::Success;
      last_success_generation_ = input.generation;
      status_ = "D3D12 RR evaluation and D3D11 output copy queued";
      RecordRrEvidence(GpuEvidenceEvent::RrEnd);
      const bool feature_changed = last_logged_source_handle_ != input.source_handle
                                   || last_logged_source_generation_ != input.source_generation
                                   || last_logged_bridge_generation_ != input.generation
                                   || last_logged_preset_ != preset;
      const bool first_success = last_logged_success_frame_ == UINT32_MAX;
      const bool summary_due = input.frame % 600u == 0u && last_logged_success_frame_ != input.frame;
      if (RrDebugLogEnabled() && (feature_changed || first_success || summary_due)) {
        const std::string completed_b_text = completed_b_ == UINT64_MAX ? "unavailable" : std::to_string(completed_b_);
        const std::string completed_c_text = completed_c_ == UINT64_MAX ? "unavailable" : std::to_string(completed_c_);
        renodx::utils::log::i("[world-rr] D3D12 evaluate queued frame=", input.frame, " source=", input.source_handle,
                              " source_gen=", input.source_generation, " runtime=", runtime_id_, " session=", session_generation_,
                              " bridge_gen=", input.generation, " preset=", preset, " input=", input.render_width, "x",
                              input.render_height, " output=", input.target_width, "x", input.target_height,
                              " source_hdr=", (input.create_flags & static_cast<int>(NVSDK_NGX_DLSS_Feature_Flags_IsHDR)) != 0,
                              " effective_hdr=1 flags=0x", std::format("{:08X}", static_cast<uint32_t>(key.create_flags)),
                              " queued=1 A=", input.input_signal_a, " B=", accepted_b, " C=", selected->signal_c,
                              " completedB=", completed_b_text, " completedC=", completed_c_text,
                              " reset=", (reset_history ? 1 : 0));
        last_logged_source_handle_ = input.source_handle;
        last_logged_source_generation_ = input.source_generation;
        last_logged_bridge_generation_ = input.generation;
        last_logged_preset_ = preset;
        last_logged_success_frame_ = input.frame;
      }
      *signal_b = accepted_b;
      return true;
    } catch (const std::exception&) {
      submission_unsafe_ = true;
      last_submit_outcome_ = SubmitOutcome::DeviceUnsafe;
      status_ = "D3D12 RR submission failed after command recording; generation quarantined";
      selected->frame_owner = input.frame_owner;
      selected->signal_b = UINT64_MAX;
      force_history_reset_ = true;
      return false;
    }
  }
  [[nodiscard]] RrReason FailureReason() const {
    switch (failure_) {
      case Failure::Device:
        return RrReason::D3D12DeviceFailed;
      case Failure::Exports:
        return RrReason::D3D12ExportsMissing;
      case Failure::NgxInit:
        return RrReason::D3D12NgxInitFailed;
      case Failure::Capability:
        return RrReason::D3D12CapabilityUnavailable;
      case Failure::None:
        break;
    }
    return RrReason::D3D12RuntimeFailed;
  }

 private:
  void RecordRrEvidence(GpuEvidenceEvent event) const {
    if (!RrDebugLogEnabled()) return;
    GpuEvidenceSnapshot snapshot{};
    snapshot.event = event;
    snapshot.source = GpuEvidenceSource::RrD3D12;
    snapshot.rr_handle = last_submit_source_handle_;
    snapshot.preset = last_submit_preset_;
    snapshot.preset_index = RrPresetIndex(last_submit_preset_);
    snapshot.rr_result = last_submit_result_;
    snapshot.rr_exception = last_submit_exception_;
    snapshot.rr_hresult = static_cast<uint32_t>(last_submit_hr_);
    snapshot.rr_stage = static_cast<uint32_t>(last_submit_stage_);
    snapshot.rr_outcome = static_cast<uint32_t>(last_submit_outcome_);
    snapshot.rr_runtime_generation = runtime_id_;
    snapshot.rr_session_generation = session_generation_;
    snapshot.rr_bridge_generation = last_submit_bridge_generation_;
    snapshot.rr_source_generation = last_submit_source_generation_;
    snapshot.rt12_generation = UINT64_MAX;
    snapshot.rt12_submitted = UINT64_MAX;
    snapshot.rt12_completed = UINT64_MAX;
    snapshot.rt12_progress_age_us = UINT64_MAX;
    snapshot.rt12_submission_age_us = UINT64_MAX;
    snapshot.a_target = last_submit_signal_a_;
    snapshot.a_available = completed_c_ != UINT64_MAX;
    snapshot.a_completed = completed_c_;
    snapshot.b_target = last_submit_signal_b_;
    snapshot.b_completed = completed_b_;
    snapshot.rr_signal_c_target = last_submit_signal_c_;
    snapshot.rr_completed_c = completed_c_;
    snapshot.rr_c_available = completed_c_ != UINT64_MAX;
    snapshot.signal_a_result = last_submit_signal_a_ != 0u ? S_OK : E_FAIL;
    snapshot.signal_b_result = last_submit_signal_b_ != 0u ? S_OK : E_FAIL;
    snapshot.d3d11_wait_b_target = last_submit_signal_b_;
    snapshot.d3d11_wait_b_accepted = last_submit_signal_c_ != 0u;
    ::falcom_world::RecordGpuEvidence(snapshot);
  }

  NVSDK_NGX_Result InvokeFeatureCall(bool create, ID3D12GraphicsCommandList* list, NVSDK_NGX_Feature feature,
                                     NVSDK_NGX_Parameter* parameters, NVSDK_NGX_Handle** handle, uint32_t* exception) {
    *exception = 0u;
    __try {
      if (create) return create_feature_(list, feature, parameters, handle);
      return evaluate_feature_(list, *handle, parameters, nullptr);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
      *exception = GetExceptionCode();
      return NVSDK_NGX_Result_Fail;
    }
  }

  bool Fail(const char* stage, HRESULT hr, Failure failure = Failure::Device) {
    status_ = std::format("{} hr=0x{:08X}", stage, static_cast<uint32_t>(hr));
    failure_hr_ = hr;
    failure_ = failure;
    g_rr_d3d12_failure_hr.store(static_cast<uint32_t>(hr), std::memory_order_relaxed);
    g_rr_d3d12_failure_result.store(0u, std::memory_order_relaxed);
    return false;
  }

  bool FailNgx(const char* stage, NVSDK_NGX_Result result, Failure failure = Failure::NgxInit) {
    status_ = std::format("{} result=0x{:08X}", stage, static_cast<uint32_t>(result));
    failure_result_ = static_cast<uint32_t>(result);
    failure_ = failure;
    g_rr_d3d12_failure_hr.store(0u, std::memory_order_relaxed);
    g_rr_d3d12_failure_result.store(failure_result_, std::memory_order_relaxed);
    return false;
  }

  template <typename T>
  bool Resolve(HMODULE module, const char* name, T* function) {
    *function = reinterpret_cast<T>(GetProcAddress(module, name));
    if (*function != nullptr) return true;
    status_ = std::string("Missing D3D12 NGX export: ") + name;
    failure_ = Failure::Exports;
    g_rr_d3d12_failure_hr.store(0u, std::memory_order_relaxed);
    g_rr_d3d12_failure_result.store(0u, std::memory_order_relaxed);
    return false;
  }

  bool ResolveNgx() {
    if (!Resolve(ngx_module_, "NVSDK_NGX_D3D12_Init", &init_) || !Resolve(ngx_module_, "NVSDK_NGX_D3D12_Init_Ext", &init_ext_) || !Resolve(ngx_module_, "NVSDK_NGX_D3D12_Init_ProjectID", &init_project_id_) || !Resolve(ngx_module_, "NVSDK_NGX_D3D12_GetCapabilityParameters", &get_capability_parameters_) || !Resolve(ngx_module_, "NVSDK_NGX_D3D12_AllocateParameters", &allocate_parameters_) || !Resolve(ngx_module_, "NVSDK_NGX_D3D12_DestroyParameters", &destroy_parameters_) || !Resolve(ngx_module_, "NVSDK_NGX_D3D12_CreateFeature", &create_feature_) || !Resolve(ngx_module_, "NVSDK_NGX_D3D12_EvaluateFeature", &evaluate_feature_) || !Resolve(ngx_module_, "NVSDK_NGX_D3D12_ReleaseFeature", &release_feature_) || !Resolve(ngx_module_, "NVSDK_NGX_D3D12_Shutdown1", &shutdown_)) {
      return false;
    }
    return true;
  }

  bool InitializeNgx(const NgxInitIdentity& identity) {
    std::vector<std::wstring> paths = identity.feature_paths;
    const std::wstring game_directory = std::filesystem::path(renodx::utils::platform::GetCurrentProcessPath()).parent_path().wstring();
    if (std::find(paths.begin(), paths.end(), game_directory) == paths.end()) paths.push_back(game_directory);
    std::vector<const wchar_t*> path_pointers;
    path_pointers.reserve(paths.size());
    for (const std::wstring& path : paths) path_pointers.push_back(path.c_str());
    NVSDK_NGX_FeatureCommonInfo common_info = {};
    common_info.PathListInfo.Path = path_pointers.data();
    common_info.PathListInfo.Length = static_cast<unsigned int>(path_pointers.size());

    NVSDK_NGX_Result result = NVSDK_NGX_Result_Fail;
    if (identity.kind == NgxInitKind::ProjectId) {
      result = init_project_id_(identity.project_id.c_str(), identity.engine_type, identity.engine_version.c_str(),
                                identity.application_data_path.c_str(), device12_.Get(), identity.sdk_version, &common_info);
    } else {
      result = init_ext_(identity.application_id, identity.application_data_path.c_str(), device12_.Get(), identity.sdk_version,
                         &common_info);
    }
    if (NVSDK_NGX_FAILED(result)) return FailNgx("Initialize D3D12 NGX", result);
    ngx_initialized_ = true;
    return true;
  }

  Microsoft::WRL::ComPtr<ID3D11Device> device11_;
  Microsoft::WRL::ComPtr<ID3D11DeviceContext> context11_;
  Microsoft::WRL::ComPtr<ID3D12Device> wrapper12_;
  Microsoft::WRL::ComPtr<ID3D12Device> device12_;
  Microsoft::WRL::ComPtr<ID3D12CommandQueue> queue_;
  std::array<Slot, 3> slots_;
  std::unique_ptr<renodx::utils::gpu_fence::Fence> fence11_;
  std::unique_ptr<renodx::utils::gpu_fence::Fence> fence12_;
  HMODULE d3d12_module_ = nullptr;
  HMODULE ngx_module_ = nullptr;
  HMODULE ngx_module_owner_ = nullptr;
  NVSDK_NGX_Parameter* capability_parameters_ = nullptr;
  NVSDK_NGX_Parameter* feature_parameters_ = nullptr;
  NVSDK_NGX_Handle* feature_ = nullptr;
  RrD3D12FeatureKey feature_key_;
  bool ngx_initialized_ = false;
  bool initialized_ = false;
  bool shutdown_done_ = false;
  bool submission_unsafe_ = false;
  bool d3d11_work_may_have_been_queued_ = false;
  bool d3d12_work_may_have_been_queued_ = false;
  bool feature_retirement_requested_ = false;
  bool last_submit_ngx_failure_ = false;
  SubmitOutcome last_submit_outcome_ = SubmitOutcome::None;
  RrSubmitStage last_submit_stage_ = RrSubmitStage::None;
  HRESULT last_submit_hr_ = S_OK;
  uint32_t last_submit_result_ = 0u;
  uint32_t last_submit_exception_ = 0u;
  bool force_history_reset_ = true;
  Failure failure_ = Failure::None;
  HRESULT failure_hr_ = S_OK;
  uint32_t failure_result_ = 0u;
  uint32_t next_slot_ = 0u;
  uint64_t last_signal_b_ = 0u;
  uint64_t completed_b_ = UINT64_MAX;
  uint64_t completed_c_ = UINT64_MAX;
  uint64_t last_submit_signal_a_ = 0u;
  uint64_t last_submit_signal_b_ = 0u;
  uint64_t last_submit_signal_c_ = 0u;
  unsigned int last_submit_source_handle_ = UINT_MAX;
  uint64_t last_submit_source_generation_ = 0u;
  uint64_t last_submit_bridge_generation_ = 0u;
  uint32_t last_submit_preset_ = 0u;
  uint64_t last_feature_signal_b_ = 0u;
  uint64_t last_success_generation_ = 0u;
  uint64_t runtime_id_ = 0u;
  uint64_t session_generation_ = 0u;
  uint64_t last_logged_source_generation_ = UINT64_MAX;
  uint64_t last_logged_bridge_generation_ = UINT64_MAX;
  unsigned int last_logged_source_handle_ = UINT_MAX;
  unsigned int last_logged_preset_ = UINT_MAX;
  uint32_t last_logged_success_frame_ = UINT32_MAX;
  uint64_t shutdown_marker11_ = 0u;
  uint64_t shutdown_marker12_ = 0u;
  std::string status_ = "not initialized";
  InitFn init_ = nullptr;
  InitExtFn init_ext_ = nullptr;
  InitProjectIdFn init_project_id_ = nullptr;
  GetCapabilityParametersFn get_capability_parameters_ = nullptr;
  AllocateParametersFn allocate_parameters_ = nullptr;
  DestroyParametersFn destroy_parameters_ = nullptr;
  CreateFeatureFn create_feature_ = nullptr;
  EvaluateFeatureFn evaluate_feature_ = nullptr;
  ReleaseFeatureFn release_feature_ = nullptr;
  ShutdownFn shutdown_ = nullptr;
};

inline std::shared_mutex g_rr_d3d12_mutex;
inline std::shared_ptr<RrD3D12Runtime> g_rr_d3d12_runtime;

inline std::shared_ptr<RrD3D12Runtime> SnapshotRrD3D12Runtime() {
  const std::shared_lock lock(g_rr_d3d12_mutex);
  return g_rr_d3d12_runtime;
}

}  // namespace falcom_world::rr
