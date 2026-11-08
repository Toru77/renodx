#pragma once

#include <d3d11_4.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl/client.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <exception>
#include <iterator>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <nvsdk_ngx.h>

#include "../../../../utils/directx.hpp"
#include "../../../../utils/gpu_fence.hpp"
#include "../../../../utils/log.hpp"
#include "../world_state.hpp"
#include "rr_d3d12.hpp"
#include "rr_guides.hpp"
#include "rr_state.hpp"

namespace falcom_world::rr {

using Microsoft::WRL::ComPtr;

struct RrSharedTexture {
  ComPtr<ID3D11Texture2D> texture11;
  ComPtr<ID3D12Resource> resource12;
  D3D11_TEXTURE2D_DESC desc = {};
};

struct RrBridgeResources {
  RrSharedTexture color;
  RrSharedTexture motion;
  RrSharedTexture normal_roughness;
  RrSharedTexture albedo;
  RrSharedTexture specular;
  RrSharedTexture depth;
  RrSharedTexture exposure;
  RrSharedTexture output;
  uint64_t generation = 0u;
  uint64_t runtime_id = 0u;
  bool has_exposure = false;
  ComPtr<ID3D11Device> device11;
  ComPtr<ID3D12Device> device12;
};

struct RrBridgeFrameMetadata {
  uint64_t input_signal_a = 0u;
  uint64_t generation = 0u;
  uint32_t frame = UINT32_MAX;
  uint32_t render_width = 0u;
  uint32_t render_height = 0u;
  uint32_t output_width = 0u;
  uint32_t output_height = 0u;
  uint32_t color_base_x = 0u;
  uint32_t color_base_y = 0u;
  uint32_t depth_base_x = 0u;
  uint32_t depth_base_y = 0u;
  uint32_t motion_base_x = 0u;
  uint32_t motion_base_y = 0u;
  uint32_t output_base_x = 0u;
  uint32_t output_base_y = 0u;
  uint32_t active_width = 0u;
  uint32_t active_height = 0u;
  int create_flags = 0;
  int perf_quality = -1;
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
};

struct RrPreparedInputs {
  std::shared_ptr<RrBridgeResources> resources;
  ComPtr<ID3D11Texture2D> game_output;
  RrBridgeFrameMetadata metadata;
};

struct RrInputFrame {
  ComPtr<ID3D11Texture2D> color;
  ComPtr<ID3D11Texture2D> motion;
  ComPtr<ID3D11Texture2D> output;
  ComPtr<ID3D11Texture2D> exposure;
  std::shared_ptr<const RrGuideResources> guides;
  uint32_t frame = UINT32_MAX;
  uint32_t render_width = 0u;
  uint32_t render_height = 0u;
  uint32_t output_width = 0u;
  uint32_t output_height = 0u;
  uint32_t color_base_x = 0u;
  uint32_t color_base_y = 0u;
  uint32_t depth_base_x = 0u;
  uint32_t depth_base_y = 0u;
  uint32_t motion_base_x = 0u;
  uint32_t motion_base_y = 0u;
  uint32_t output_base_x = 0u;
  uint32_t output_base_y = 0u;
  uint32_t active_width = 0u;
  uint32_t active_height = 0u;
  float jitter_x = 0.f;
  float jitter_y = 0.f;
  float mv_scale_x = 1.f;
  float mv_scale_y = 1.f;
  float pre_exposure = 1.f;
  float exposure_scale = 1.f;
  bool reset = false;
  int create_flags = 0;
  int perf_quality = -1;
  bool output_subrects = false;
  std::array<float, 16> world_to_view = {};
  std::array<float, 16> view_to_clip = {};
};

struct RrPendingInputCopy {
  uint64_t signal = 0u;
  std::shared_ptr<RrD3D12Runtime> runtime;
  std::shared_ptr<RrBridgeResources> resources;
  std::shared_ptr<const RrGuideResources> guides;
  ComPtr<ID3D11Texture2D> color;
  ComPtr<ID3D11Texture2D> motion;
  ComPtr<ID3D11Texture2D> output;
  ComPtr<ID3D11Texture2D> exposure;
  std::shared_ptr<const RrPreparedInputs> prepared;
};

struct RrInteropState {
  std::shared_mutex mutex;
  std::shared_ptr<RrBridgeResources> resources;
  std::shared_ptr<const RrPreparedInputs> latest;
  std::vector<RrPendingInputCopy> pending;
  std::vector<RrPendingInputCopy> untracked;
  std::vector<RrPendingInputCopy> quarantine;
  std::vector<std::shared_ptr<RrBridgeResources>> quarantine_resources;
  std::vector<std::shared_ptr<const RrPreparedInputs>> quarantine_latest;
  ID3D11Device* quarantine_device = nullptr;
  bool quarantined = false;
  uint64_t next_generation = 1u;
  std::string status = "waiting for valid RR inputs";
};

// Keep queued-copy owners alive across add-on detach; normal device shutdown explicitly retires them.
inline RrInteropState& g_rr_interop = *new RrInteropState();

inline void SetRrInteropStatus(std::string status) {
  bool changed = false;
  std::string message;
  {
    const std::unique_lock lock(g_rr_interop.mutex);
    changed = status != g_rr_interop.status;
    if (changed) g_rr_interop.status = std::move(status);
    if (changed) message = g_rr_interop.status;
  }
  if (changed && RrDebugLogEnabled()) {
    renodx::utils::log::i("[world-rr] bridge: ", message);
  }
}

inline std::string RrInteropStatusSnapshot() {
  const std::shared_lock lock(g_rr_interop.mutex);
  return g_rr_interop.status;
}

inline bool RrSameDevice(IUnknown* first, IUnknown* second) {
  ComPtr<IUnknown> first_identity;
  ComPtr<IUnknown> second_identity;
  if (first == nullptr || second == nullptr || FAILED(first->QueryInterface(IID_PPV_ARGS(&first_identity))) || FAILED(second->QueryInterface(IID_PPV_ARGS(&second_identity)))) {
    return false;
  }
  return first_identity.Get() == second_identity.Get();
}

inline bool RrTextureDesc(ID3D11Texture2D* texture, D3D11_TEXTURE2D_DESC* desc) {
  if (texture == nullptr || desc == nullptr) return false;
  texture->GetDesc(desc);
  return desc->Width != 0u && desc->Height != 0u && desc->ArraySize == 1u && desc->SampleDesc.Count == 1u && desc->Format != DXGI_FORMAT_UNKNOWN;
}

inline HRESULT CreateRrSharedTexture(RrD3D12Runtime& runtime, const char* name, const D3D11_TEXTURE2D_DESC& source_desc,
                                     bool output, RrSharedTexture* shared) {
  if (shared == nullptr || runtime.Device11() == nullptr || runtime.Device12() == nullptr) return E_INVALIDARG;
  D3D11_TEXTURE2D_DESC desc = {};
  desc.Width = source_desc.Width;
  desc.Height = source_desc.Height;
  desc.MipLevels = 1u;
  desc.ArraySize = 1u;
  desc.Format = source_desc.Format;
  desc.SampleDesc.Count = 1u;
  desc.Usage = D3D11_USAGE_DEFAULT;
  desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | (output ? D3D11_BIND_UNORDERED_ACCESS : 0u);
  desc.MiscFlags = D3D11_RESOURCE_MISC_SHARED | D3D11_RESOURCE_MISC_SHARED_NTHANDLE;
  HRESULT hr = runtime.Device11()->CreateTexture2D(&desc, nullptr, shared->texture11.GetAddressOf());
  if (FAILED(hr)) return hr;
  ComPtr<IDXGIResource1> dxgi_resource;
  hr = shared->texture11.As(&dxgi_resource);
  if (FAILED(hr)) return hr;
  HANDLE handle = nullptr;
  hr = dxgi_resource->CreateSharedHandle(nullptr, DXGI_SHARED_RESOURCE_READ | DXGI_SHARED_RESOURCE_WRITE, nullptr, &handle);
  if (FAILED(hr)) return hr;
  hr = runtime.Device12()->OpenSharedHandle(handle, IID_PPV_ARGS(shared->resource12.GetAddressOf()));
  CloseHandle(handle);
  if (FAILED(hr)) return hr;
  shared->desc = desc;
  if (RrDebugLogEnabled()) {
    renodx::utils::log::i("[world-rr] bridge resource created name=", name, " size=", desc.Width, "x", desc.Height,
                          " format=", static_cast<uint32_t>(desc.Format), " bind=", desc.BindFlags);
  }
  return S_OK;
}

inline bool RrSharedTextureMatches(const RrSharedTexture& shared, const D3D11_TEXTURE2D_DESC& source_desc, bool output) {
  const UINT bind_flags = D3D11_BIND_SHADER_RESOURCE | (output ? D3D11_BIND_UNORDERED_ACCESS : 0u);
  return shared.texture11 != nullptr && shared.resource12 != nullptr && shared.desc.Width == source_desc.Width && shared.desc.Height == source_desc.Height && shared.desc.Format == source_desc.Format && shared.desc.BindFlags == bind_flags;
}

inline HRESULT EnsureRrSharedTexture(RrD3D12Runtime& runtime, const char* name, const D3D11_TEXTURE2D_DESC& source_desc,
                                     bool output, RrSharedTexture* shared) {
  if (RrSharedTextureMatches(*shared, source_desc, output)) return S_OK;
  RrSharedTexture replacement;
  const HRESULT hr = CreateRrSharedTexture(runtime, name, source_desc, output, &replacement);
  if (FAILED(hr)) return hr;
  *shared = std::move(replacement);
  return S_OK;
}

inline void RrCopyTexture(ID3D11DeviceContext* context, ID3D11Texture2D* source, ID3D11Texture2D* destination,
                          const D3D11_TEXTURE2D_DESC& desc) {
  const D3D11_BOX box = {.left = 0u, .top = 0u, .front = 0u, .right = desc.Width, .bottom = desc.Height, .back = 1u};
  context->CopySubresourceRegion(destination, 0u, 0u, 0u, 0u, source, 0u, &box);
}

inline uint64_t RrCompletedInputSignal(const RrPendingInputCopy& copy) {
  try {
    auto* fence = copy.runtime != nullptr ? copy.runtime->Fence11() : nullptr;
    return fence != nullptr ? fence->GetCompletedValue() : UINT64_MAX;
  } catch (const std::exception&) {
    return UINT64_MAX;
  }
}

inline void RrRetireCompletedInputCopies() {
  {
    const std::shared_lock lock(g_rr_interop.mutex);
    if (g_rr_interop.pending.empty() && g_rr_interop.untracked.empty()) return;
  }
  std::vector<RrPendingInputCopy> pending;
  std::vector<RrPendingInputCopy> untracked;
  {
    const std::unique_lock lock(g_rr_interop.mutex);
    pending.swap(g_rr_interop.pending);
    untracked.swap(g_rr_interop.untracked);
  }

  std::vector<RrPendingInputCopy> pending_again;
  std::vector<RrPendingInputCopy> quarantined_copies;
  std::vector<RrPendingInputCopy> retired;
  std::vector<uint64_t> unknown_runtime_ids;
  const auto classify = [&](RrPendingInputCopy* copy, bool no_signal) {
    const uint64_t completed = !no_signal && copy->signal != renodx::utils::gpu_fence::NO_SIGNAL
                                   ? RrCompletedInputSignal(*copy)
                                   : UINT64_MAX;
    if (completed != UINT64_MAX && copy->signal <= completed) {
      retired.push_back(std::move(*copy));
    } else if (completed != UINT64_MAX && copy->signal != renodx::utils::gpu_fence::NO_SIGNAL) {
      pending_again.push_back(std::move(*copy));
    } else {
      if (copy->resources != nullptr && std::find(unknown_runtime_ids.begin(), unknown_runtime_ids.end(), copy->resources->runtime_id) == unknown_runtime_ids.end()) {
        unknown_runtime_ids.push_back(copy->resources->runtime_id);
      }
      quarantined_copies.push_back(std::move(*copy));
    }
  };
  for (RrPendingInputCopy& copy : pending) classify(&copy, false);
  for (RrPendingInputCopy& copy : untracked) classify(&copy, true);

  bool quarantine_added = false;
  {
    const std::unique_lock lock(g_rr_interop.mutex);
    g_rr_interop.pending.insert(g_rr_interop.pending.end(), std::make_move_iterator(pending_again.begin()),
                                std::make_move_iterator(pending_again.end()));
    if (!quarantined_copies.empty()) {
      g_rr_interop.quarantine.insert(g_rr_interop.quarantine.end(),
                                     std::make_move_iterator(quarantined_copies.begin()),
                                     std::make_move_iterator(quarantined_copies.end()));
      for (const uint64_t runtime_id : unknown_runtime_ids) {
        if (g_rr_interop.resources != nullptr && g_rr_interop.resources->runtime_id == runtime_id && std::find(g_rr_interop.quarantine_resources.begin(), g_rr_interop.quarantine_resources.end(), g_rr_interop.resources) == g_rr_interop.quarantine_resources.end()) {
          g_rr_interop.quarantine_resources.push_back(g_rr_interop.resources);
        }
        if (g_rr_interop.latest != nullptr && g_rr_interop.latest->resources != nullptr && g_rr_interop.latest->resources->runtime_id == runtime_id && std::find(g_rr_interop.quarantine_latest.begin(), g_rr_interop.quarantine_latest.end(), g_rr_interop.latest) == g_rr_interop.quarantine_latest.end()) {
          g_rr_interop.quarantine_latest.push_back(g_rr_interop.latest);
        }
      }
      g_rr_interop.quarantined = true;
      if (g_rr_interop.quarantine_device == nullptr && !g_rr_interop.quarantine.empty() && g_rr_interop.quarantine.front().runtime != nullptr) {
        g_rr_interop.quarantine_device = g_rr_interop.quarantine.front().runtime->Device11();
      }
      quarantine_added = true;
    }
  }
  if (quarantine_added) SetRrInteropStatus("D3D11 input completion is unknown; RR owners retained until runtime drain");
}

inline void ReleaseRrInteropOwnersAfterDrain(const std::shared_ptr<RrD3D12Runtime>& runtime) {
  if (runtime == nullptr) return;
  std::vector<RrPendingInputCopy> retired_copies;
  std::vector<std::shared_ptr<RrBridgeResources>> retired_resources;
  std::vector<std::shared_ptr<const RrPreparedInputs>> retired_latest;
  bool remaining_blocked = false;
  {
    const std::unique_lock lock(g_rr_interop.mutex);
    const auto matches = [&runtime](const RrPendingInputCopy& copy) {
      return copy.runtime == runtime;
    };
    auto collect = [&](std::vector<RrPendingInputCopy>* copies) {
      std::vector<RrPendingInputCopy> keep;
      for (RrPendingInputCopy& copy : *copies) {
        if (matches(copy)) {
          retired_copies.push_back(std::move(copy));
        } else {
          keep.push_back(std::move(copy));
        }
      }
      *copies = std::move(keep);
    };
    collect(&g_rr_interop.pending);
    collect(&g_rr_interop.untracked);
    collect(&g_rr_interop.quarantine);
    const uint64_t runtime_id = runtime->RuntimeId();
    if (g_rr_interop.resources != nullptr && g_rr_interop.resources->runtime_id == runtime_id) {
      retired_resources.push_back(std::exchange(g_rr_interop.resources, {}));
    }
    if (g_rr_interop.latest != nullptr && g_rr_interop.latest->resources != nullptr && g_rr_interop.latest->resources->runtime_id == runtime_id) {
      retired_latest.push_back(std::exchange(g_rr_interop.latest, {}));
    }
    auto remove_resources = [&](std::vector<std::shared_ptr<RrBridgeResources>>* resources) {
      std::vector<std::shared_ptr<RrBridgeResources>> keep;
      for (auto& owner : *resources) {
        if (owner != nullptr && owner->runtime_id == runtime_id) {
          retired_resources.push_back(std::move(owner));
        } else {
          keep.push_back(std::move(owner));
        }
      }
      *resources = std::move(keep);
    };
    auto remove_latest = [&](std::vector<std::shared_ptr<const RrPreparedInputs>>* latest) {
      std::vector<std::shared_ptr<const RrPreparedInputs>> keep;
      for (auto& owner : *latest) {
        if (owner != nullptr && owner->resources != nullptr && owner->resources->runtime_id == runtime_id) {
          retired_latest.push_back(std::move(owner));
        } else {
          keep.push_back(std::move(owner));
        }
      }
      *latest = std::move(keep);
    };
    remove_resources(&g_rr_interop.quarantine_resources);
    remove_latest(&g_rr_interop.quarantine_latest);

    g_rr_interop.quarantined = !g_rr_interop.untracked.empty() || !g_rr_interop.quarantine.empty() || !g_rr_interop.quarantine_resources.empty() || !g_rr_interop.quarantine_latest.empty();
    remaining_blocked = g_rr_interop.quarantined;
    g_rr_interop.quarantine_device = nullptr;
    if (!g_rr_interop.untracked.empty() && g_rr_interop.untracked.front().runtime != nullptr) {
      g_rr_interop.quarantine_device = g_rr_interop.untracked.front().runtime->Device11();
    } else if (!g_rr_interop.quarantine.empty() && g_rr_interop.quarantine.front().runtime != nullptr) {
      g_rr_interop.quarantine_device = g_rr_interop.quarantine.front().runtime->Device11();
    } else if (!g_rr_interop.quarantine_resources.empty() && g_rr_interop.quarantine_resources.front() != nullptr) {
      g_rr_interop.quarantine_device = g_rr_interop.quarantine_resources.front()->device11.Get();
    } else if (!g_rr_interop.quarantine_latest.empty() && g_rr_interop.quarantine_latest.front() != nullptr && g_rr_interop.quarantine_latest.front()->resources != nullptr) {
      g_rr_interop.quarantine_device = g_rr_interop.quarantine_latest.front()->resources->device11.Get();
    }
  }
  SetRrInteropStatus(remaining_blocked ? "RR owners drained; another device generation remains quarantined"
                                       : "RR bridge owners retired after GPU drain");
}

inline void ShutdownRrInteropDevice(ID3D11Device* device) {
  bool quarantined = false;
  {
    const std::unique_lock lock(g_rr_interop.mutex);
    const auto matches = [device](const RrPendingInputCopy& copy) {
      return copy.runtime != nullptr && (device == nullptr || copy.runtime->Device11() == device);
    };
    auto collect = [&](std::vector<RrPendingInputCopy>* copies) {
      std::vector<RrPendingInputCopy> keep;
      for (RrPendingInputCopy& copy : *copies) {
        if (!matches(copy)) {
          keep.push_back(std::move(copy));
        } else {
          g_rr_interop.quarantine.push_back(std::move(copy));
        }
      }
      *copies = std::move(keep);
    };
    collect(&g_rr_interop.pending);
    collect(&g_rr_interop.untracked);
    auto retain_resources = [&](const std::shared_ptr<RrBridgeResources>& resources) {
      return resources != nullptr && (device == nullptr || resources->device11.Get() == device) && std::find(g_rr_interop.quarantine_resources.begin(), g_rr_interop.quarantine_resources.end(), resources) == g_rr_interop.quarantine_resources.end();
    };
    if (retain_resources(g_rr_interop.resources)) {
      g_rr_interop.quarantine_resources.push_back(std::exchange(g_rr_interop.resources, {}));
    }
    auto retain_latest = [&](const std::shared_ptr<const RrPreparedInputs>& latest) {
      return latest != nullptr && latest->resources != nullptr && (device == nullptr || latest->resources->device11.Get() == device) && std::find(g_rr_interop.quarantine_latest.begin(), g_rr_interop.quarantine_latest.end(), latest) == g_rr_interop.quarantine_latest.end();
    };
    if (retain_latest(g_rr_interop.latest)) {
      g_rr_interop.quarantine_latest.push_back(std::exchange(g_rr_interop.latest, {}));
    }
    g_rr_interop.quarantined = !g_rr_interop.untracked.empty() || !g_rr_interop.quarantine.empty() || !g_rr_interop.quarantine_resources.empty() || !g_rr_interop.quarantine_latest.empty();
    g_rr_interop.quarantine_device = nullptr;
    if (!g_rr_interop.untracked.empty() && g_rr_interop.untracked.front().runtime != nullptr) {
      g_rr_interop.quarantine_device = g_rr_interop.untracked.front().runtime->Device11();
    } else if (!g_rr_interop.quarantine.empty() && g_rr_interop.quarantine.front().runtime != nullptr) {
      g_rr_interop.quarantine_device = g_rr_interop.quarantine.front().runtime->Device11();
    } else if (!g_rr_interop.quarantine_resources.empty()) {
      g_rr_interop.quarantine_device = g_rr_interop.quarantine_resources.front()->device11.Get();
    }
    quarantined = g_rr_interop.quarantined;
  }
  if (quarantined) SetRrInteropStatus("D3D11 shutdown: RR owners quarantined pending D3D12 runtime drain");
}

inline std::shared_ptr<RrBridgeResources> SnapshotRrBridgeResources() {
  const std::shared_lock lock(g_rr_interop.mutex);
  return g_rr_interop.resources;
}

inline std::shared_ptr<const RrPreparedInputs> SnapshotRrPreparedInputs() {
  const std::shared_lock lock(g_rr_interop.mutex);
  return g_rr_interop.latest;
}

inline bool CaptureRrTexture(const NVSDK_NGX_Parameter* parameters, const char* key, ComPtr<ID3D11Texture2D>* texture,
                             bool optional = false) {
  ID3D11Resource* resource = nullptr;
  const NVSDK_NGX_Result result = parameters->Get(key, &resource);
  if (NVSDK_NGX_FAILED(result) || resource == nullptr) return optional;
  return SUCCEEDED(resource->QueryInterface(IID_PPV_ARGS(texture->GetAddressOf())));
}

inline void ReadRrUint(const NVSDK_NGX_Parameter* parameters, const char* key, uint32_t fallback, uint32_t* value) {
  unsigned int read = fallback;
  if (NVSDK_NGX_SUCCEED(parameters->Get(key, &read))) *value = read;
}

inline void ReadRrFloat(const NVSDK_NGX_Parameter* parameters, const char* key, float fallback, float* value) {
  float read = fallback;
  if (NVSDK_NGX_SUCCEED(parameters->Get(key, &read))) *value = read;
}

inline bool CaptureRrInputFrame(ID3D11DeviceContext* context, const NVSDK_NGX_Parameter* parameters,
                                uint32_t render_width, uint32_t render_height, uint32_t output_width,
                                uint32_t output_height, int create_flags, int perf_quality, int output_subrects, uint32_t frame,
                                RrInputFrame* input, std::string* reason) {
  if (context == nullptr || parameters == nullptr || input == nullptr || reason == nullptr) return false;
  if (context->GetType() != D3D11_DEVICE_CONTEXT_IMMEDIATE) {
    *reason = "RR requires the game's immediate D3D11 context";
    return false;
  }
  if (!CaptureRrTexture(parameters, NVSDK_NGX_Parameter_Color, &input->color) || !CaptureRrTexture(parameters, NVSDK_NGX_Parameter_MotionVectors, &input->motion) || !CaptureRrTexture(parameters, NVSDK_NGX_Parameter_Output, &input->output) || !CaptureRrTexture(parameters, NVSDK_NGX_Parameter_ExposureTexture, &input->exposure, true)) {
    *reason = "a required game NGX texture is missing or not a 2D texture";
    return false;
  }
  {
    const std::shared_lock lock(g_rr_guides.mutex);
    if (g_rr_guides.frame != frame || g_rr_guides.resources == nullptr) {
      *reason = "Falcom RR guides are missing or stale for this frame";
      return false;
    }
    input->guides = g_rr_guides.resources;
  }
  input->frame = frame;
  input->render_width = render_width;
  input->render_height = render_height;
  input->output_width = output_width;
  input->output_height = output_height;
  input->create_flags = create_flags;
  input->perf_quality = perf_quality;
  input->output_subrects = output_subrects != 0;
  ReadRrUint(parameters, NVSDK_NGX_Parameter_DLSS_Input_Color_Subrect_Base_X, 0u, &input->color_base_x);
  ReadRrUint(parameters, NVSDK_NGX_Parameter_DLSS_Input_Color_Subrect_Base_Y, 0u, &input->color_base_y);
  ReadRrUint(parameters, NVSDK_NGX_Parameter_DLSS_Input_Depth_Subrect_Base_X, 0u, &input->depth_base_x);
  ReadRrUint(parameters, NVSDK_NGX_Parameter_DLSS_Input_Depth_Subrect_Base_Y, 0u, &input->depth_base_y);
  ReadRrUint(parameters, NVSDK_NGX_Parameter_DLSS_Input_MV_SubrectBase_X, 0u, &input->motion_base_x);
  ReadRrUint(parameters, NVSDK_NGX_Parameter_DLSS_Input_MV_SubrectBase_Y, 0u, &input->motion_base_y);
  ReadRrUint(parameters, NVSDK_NGX_Parameter_DLSS_Output_Subrect_Base_X, 0u, &input->output_base_x);
  ReadRrUint(parameters, NVSDK_NGX_Parameter_DLSS_Output_Subrect_Base_Y, 0u, &input->output_base_y);
  ReadRrUint(parameters, NVSDK_NGX_Parameter_DLSS_Render_Subrect_Dimensions_Width, render_width, &input->active_width);
  ReadRrUint(parameters, NVSDK_NGX_Parameter_DLSS_Render_Subrect_Dimensions_Height, render_height, &input->active_height);
  ReadRrFloat(parameters, NVSDK_NGX_Parameter_Jitter_Offset_X, 0.f, &input->jitter_x);
  ReadRrFloat(parameters, NVSDK_NGX_Parameter_Jitter_Offset_Y, 0.f, &input->jitter_y);
  ReadRrFloat(parameters, NVSDK_NGX_Parameter_MV_Scale_X, 1.f, &input->mv_scale_x);
  ReadRrFloat(parameters, NVSDK_NGX_Parameter_MV_Scale_Y, 1.f, &input->mv_scale_y);
  ReadRrFloat(parameters, NVSDK_NGX_Parameter_DLSS_Pre_Exposure, 1.f, &input->pre_exposure);
  ReadRrFloat(parameters, NVSDK_NGX_Parameter_DLSS_Exposure_Scale, 1.f, &input->exposure_scale);
  int reset = 0;
  if (NVSDK_NGX_SUCCEED(parameters->Get(NVSDK_NGX_Parameter_Reset, &reset))) input->reset = reset != 0;
  {
    const std::lock_guard lock(g_state.mutex);
    for (unsigned int row = 0u; row < 4u; ++row) {
      for (unsigned int column = 0u; column < 4u; ++column) {
        input->world_to_view[row * 4u + column] = g_state.camera.view[column * 4u + row];
        input->view_to_clip[row * 4u + column] = g_state.camera.proj[column * 4u + row];
      }
    }
  }

  std::array<D3D11_TEXTURE2D_DESC, 7> descs = {};
  const std::array<ID3D11Texture2D*, 7> textures = {input->color.Get(), input->motion.Get(), input->guides->normal_roughness.Get(),
                                                    input->guides->albedo.Get(), input->guides->specular.Get(),
                                                    input->guides->depth.Get(), input->output.Get()};
  for (size_t index = 0u; index < textures.size(); ++index) {
    if (!RrTextureDesc(textures[index], &descs[index])) {
      *reason = "an RR input is not a supported single-sample 2D texture";
      return false;
    }
  }
  if (input->exposure != nullptr) {
    D3D11_TEXTURE2D_DESC exposure_desc = {};
    if (!RrTextureDesc(input->exposure.Get(), &exposure_desc) || (exposure_desc.Format != DXGI_FORMAT_R32_FLOAT && exposure_desc.Format != DXGI_FORMAT_R16_FLOAT)) {
      *reason = "the game's exposure texture format cannot be shared";
      return false;
    }
  }
  if (descs[0].Format != DXGI_FORMAT_R16G16B16A16_FLOAT || descs[1].Format != DXGI_FORMAT_R16G16_FLOAT || descs[2].Format != DXGI_FORMAT_R16G16B16A16_FLOAT || descs[3].Format != DXGI_FORMAT_R8G8B8A8_UNORM || descs[4].Format != DXGI_FORMAT_R8G8B8A8_UNORM || descs[5].Format != DXGI_FORMAT_R32_FLOAT || descs[6].Format != descs[0].Format) {
    *reason = "RR input format does not match Falcom's typed guide layout";
    return false;
  }
  ID3D11Device* context_device = nullptr;
  context->GetDevice(&context_device);
  if (context_device == nullptr) {
    *reason = "the game's D3D11 context has no device";
    return false;
  }
  ComPtr<ID3D11Device> context_device_owner;
  context_device_owner.Attach(context_device);
  ID3D11Device* native_context_device = context_device;
  renodx::utils::directx::NativeFromReShadeProxy(&native_context_device);
  for (ID3D11Texture2D* texture : textures) {
    ID3D11Device* resource_device = nullptr;
    texture->GetDevice(&resource_device);
    if (resource_device == nullptr) {
      *reason = "an RR texture has no D3D11 device";
      return false;
    }
    ComPtr<ID3D11Device> resource_device_owner;
    resource_device_owner.Attach(resource_device);
    renodx::utils::directx::NativeFromReShadeProxy(&resource_device);
    const bool same = RrSameDevice(native_context_device, resource_device);
    if (!same) {
      *reason = "RR input belongs to a different D3D11 device";
      return false;
    }
  }
  if (input->exposure != nullptr) {
    ID3D11Device* resource_device = nullptr;
    input->exposure->GetDevice(&resource_device);
    if (resource_device == nullptr) {
      *reason = "the game's exposure texture has no D3D11 device";
      return false;
    }
    ComPtr<ID3D11Device> resource_device_owner;
    resource_device_owner.Attach(resource_device);
    renodx::utils::directx::NativeFromReShadeProxy(&resource_device);
    if (!RrSameDevice(native_context_device, resource_device)) {
      *reason = "the game's exposure texture belongs to a different D3D11 device";
      return false;
    }
  }
  auto bounded = [](uint32_t base, uint32_t extent, uint32_t allocation) { return extent != 0u && base < allocation && extent <= allocation - base; };
  if (!bounded(input->color_base_x, input->active_width, descs[0].Width) || !bounded(input->color_base_y, input->active_height, descs[0].Height) || !bounded(input->depth_base_x, input->active_width, descs[5].Width) || !bounded(input->depth_base_y, input->active_height, descs[5].Height) || !bounded(input->motion_base_x, input->active_width, descs[1].Width) || !bounded(input->motion_base_y, input->active_height, descs[1].Height) || !bounded(input->output_base_x, input->output_width, descs[6].Width) || !bounded(input->output_base_y, input->output_height, descs[6].Height) || descs[2].Width != descs[5].Width || descs[2].Height != descs[5].Height || descs[3].Width != descs[5].Width || descs[3].Height != descs[5].Height || descs[4].Width != descs[5].Width || descs[4].Height != descs[5].Height) {
    *reason = "RR render/output rectangle or guide allocation is out of bounds";
    return false;
  }
  if (!input->output_subrects && (input->output_base_x != 0u || input->output_base_y != 0u)) {
    *reason = "output subrect base is nonzero while NGX output subrects are disabled";
    return false;
  }
  if ((create_flags & static_cast<int>(NVSDK_NGX_DLSS_Feature_Flags_MVLowRes)) != 0) {
    if (descs[1].Width > descs[5].Width || descs[1].Height > descs[5].Height) {
      *reason = "low-resolution motion allocation exceeds the render allocation";
      return false;
    }
  } else if (descs[1].Width != descs[5].Width || descs[1].Height != descs[5].Height) {
    *reason = "full-resolution motion allocation does not match the render allocation";
    return false;
  }
  return true;
}

inline bool PrepareRrD3D12Inputs(const std::shared_ptr<RrD3D12Runtime>& runtime_owner, ID3D11DeviceContext* context,
                                 const NVSDK_NGX_Parameter* parameters,
                                 uint32_t render_width, uint32_t render_height, uint32_t output_width,
                                 uint32_t output_height, int create_flags, int perf_quality, int output_subrects,
                                 uint32_t frame, std::shared_ptr<const RrPreparedInputs>* prepared_out) {
  if (prepared_out != nullptr) prepared_out->reset();
  if (runtime_owner == nullptr) return false;
  RrD3D12Runtime& runtime = *runtime_owner;
  RrRetireCompletedInputCopies();
  ID3D11Device* context_device = nullptr;
  if (context != nullptr) context->GetDevice(&context_device);
  ComPtr<ID3D11Device> context_device_owner;
  context_device_owner.Attach(context_device);
  ID3D11Device* native_context_device = context_device;
  if (native_context_device != nullptr) renodx::utils::directx::NativeFromReShadeProxy(&native_context_device);
  if (native_context_device == nullptr || !runtime.Matches(native_context_device)) {
    SetRrInteropStatus("D3D11 evaluation context does not match the D3D12 RR runtime device");
    return false;
  }
  bool bridge_blocked = false;
  {
    const std::shared_lock lock(g_rr_interop.mutex);
    bridge_blocked = !g_rr_interop.untracked.empty() || g_rr_interop.quarantined;
  }
  if (bridge_blocked) {
    SetRrInteropStatus("RR bridge has untracked input copies or quarantined resources");
    return false;
  }
  RrInputFrame input;
  std::string reason;
  if (!CaptureRrInputFrame(context, parameters, render_width, render_height, output_width, output_height, create_flags,
                           perf_quality, output_subrects, frame, &input, &reason)) {
    SetRrInteropStatus(std::move(reason));
    return false;
  }
  std::array<D3D11_TEXTURE2D_DESC, 7> descs = {};
  const std::array<ID3D11Texture2D*, 7> textures = {input.color.Get(), input.motion.Get(), input.guides->normal_roughness.Get(),
                                                    input.guides->albedo.Get(), input.guides->specular.Get(),
                                                    input.guides->depth.Get(), input.output.Get()};
  for (size_t index = 0u; index < textures.size(); ++index) textures[index]->GetDesc(&descs[index]);
  D3D11_TEXTURE2D_DESC exposure_desc = {};
  if (input.exposure != nullptr) input.exposure->GetDesc(&exposure_desc);

  std::shared_ptr<RrBridgeResources> current;
  {
    const std::shared_lock lock(g_rr_interop.mutex);
    current = g_rr_interop.resources;
  }
  const bool new_generation = current == nullptr || current->device11.Get() != runtime.Device11() || current->device12.Get() != runtime.Device12() || !RrSharedTextureMatches(current->color, descs[0], false) || !RrSharedTextureMatches(current->motion, descs[1], false) || !RrSharedTextureMatches(current->normal_roughness, descs[2], false) || !RrSharedTextureMatches(current->albedo, descs[3], false) || !RrSharedTextureMatches(current->specular, descs[4], false) || !RrSharedTextureMatches(current->depth, descs[5], false) || !RrSharedTextureMatches(current->output, descs[6], true) || current->has_exposure != (input.exposure != nullptr) || (input.exposure != nullptr && !RrSharedTextureMatches(current->exposure, exposure_desc, false));
  auto replacement = new_generation ? std::make_shared<RrBridgeResources>() : current;
  const char* failed_texture = nullptr;
  auto ensure_texture = [&](const char* name, const D3D11_TEXTURE2D_DESC& desc, bool output, RrSharedTexture* shared) {
    const HRESULT result = EnsureRrSharedTexture(runtime, name, desc, output, shared);
    if (FAILED(result)) failed_texture = name;
    return result;
  };
  HRESULT hr = ensure_texture("Color", descs[0], false, &replacement->color);
  if (SUCCEEDED(hr)) hr = ensure_texture("MotionVectors", descs[1], false, &replacement->motion);
  if (SUCCEEDED(hr)) hr = ensure_texture("PackedNormalRoughness", descs[2], false, &replacement->normal_roughness);
  if (SUCCEEDED(hr)) hr = ensure_texture("DiffuseAlbedo", descs[3], false, &replacement->albedo);
  if (SUCCEEDED(hr)) hr = ensure_texture("SpecularAlbedo", descs[4], false, &replacement->specular);
  if (SUCCEEDED(hr)) hr = ensure_texture("HardwareDepth", descs[5], false, &replacement->depth);
  if (SUCCEEDED(hr) && input.exposure != nullptr) hr = ensure_texture("ExposureTexture", exposure_desc, false, &replacement->exposure);
  if (SUCCEEDED(hr)) hr = ensure_texture("Output", descs[6], true, &replacement->output);
  if (FAILED(hr)) {
    SetRrInteropStatus(renodx::utils::log::BuildString(
        "shared texture allocation failed name=", failed_texture, " hr=", renodx::utils::log::AsPtr(static_cast<uint32_t>(hr))));
    return false;
  }
  if (!runtime.QueueWaitBeforeSharedWrites()) {
    SetRrInteropStatus(runtime.Status());
    return false;
  }
  replacement->has_exposure = input.exposure != nullptr;
  if (new_generation) {
    replacement->device11 = runtime.Device11();
    replacement->device12 = runtime.Device12();
    replacement->runtime_id = runtime.RuntimeId();
    const std::unique_lock lock(g_rr_interop.mutex);
    replacement->generation = g_rr_interop.next_generation++;
  }

  RrCopyTexture(context, input.color.Get(), replacement->color.texture11.Get(), descs[0]);
  RrCopyTexture(context, input.motion.Get(), replacement->motion.texture11.Get(), descs[1]);
  RrCopyTexture(context, input.guides->normal_roughness.Get(), replacement->normal_roughness.texture11.Get(), descs[2]);
  RrCopyTexture(context, input.guides->albedo.Get(), replacement->albedo.texture11.Get(), descs[3]);
  RrCopyTexture(context, input.guides->specular.Get(), replacement->specular.texture11.Get(), descs[4]);
  RrCopyTexture(context, input.guides->depth.Get(), replacement->depth.texture11.Get(), descs[5]);
  if (input.exposure != nullptr) RrCopyTexture(context, input.exposure.Get(), replacement->exposure.texture11.Get(), exposure_desc);

  uint64_t signal = renodx::utils::gpu_fence::NO_SIGNAL;
  try {
    signal = renodx::utils::gpu_fence::QueueSignal(context, runtime.Fence11());
  } catch (const std::exception&) {
    signal = renodx::utils::gpu_fence::NO_SIGNAL;
  }
  context->Flush();
  RrPendingInputCopy pending = {
      .signal = signal,
      .runtime = runtime_owner,
      .resources = replacement,
      .guides = input.guides,
      .color = std::move(input.color),
      .motion = std::move(input.motion),
      .output = std::move(input.output),
      .exposure = std::move(input.exposure),
  };
  auto prepared = std::make_shared<RrPreparedInputs>();
  prepared->resources = replacement;
  prepared->game_output = pending.output;
  prepared->metadata = {
      .input_signal_a = signal,
      .generation = replacement->generation,
      .frame = input.frame,
      .render_width = input.render_width,
      .render_height = input.render_height,
      .output_width = input.output_width,
      .output_height = input.output_height,
      .color_base_x = input.color_base_x,
      .color_base_y = input.color_base_y,
      .depth_base_x = input.depth_base_x,
      .depth_base_y = input.depth_base_y,
      .motion_base_x = input.motion_base_x,
      .motion_base_y = input.motion_base_y,
      .output_base_x = input.output_base_x,
      .output_base_y = input.output_base_y,
      .active_width = input.active_width,
      .active_height = input.active_height,
      .create_flags = input.create_flags,
      .perf_quality = input.perf_quality,
      .output_subrects = input.output_subrects,
      .jitter_x = input.jitter_x,
      .jitter_y = input.jitter_y,
      .mv_scale_x = input.mv_scale_x,
      .mv_scale_y = input.mv_scale_y,
      .pre_exposure = input.pre_exposure,
      .exposure_scale = input.exposure_scale,
      .reset = input.reset,
      .world_to_view = input.world_to_view,
      .view_to_clip = input.view_to_clip,
  };
  pending.prepared = prepared;
  const bool signal_failed = signal == renodx::utils::gpu_fence::NO_SIGNAL;
  std::shared_ptr<RrBridgeResources> retired_resources;
  std::shared_ptr<const RrPreparedInputs> retired_latest;
  {
    const std::unique_lock lock(g_rr_interop.mutex);
    if (signal_failed) {
      g_rr_interop.untracked.push_back(std::move(pending));
    } else {
      retired_resources = std::exchange(g_rr_interop.resources, replacement);
      retired_latest = std::exchange(g_rr_interop.latest, prepared);
      g_rr_interop.pending.push_back(std::move(pending));
    }
  }
  if (signal_failed) {
    g_rr_last_submit_stage.store(static_cast<uint32_t>(RrSubmitStage::WaitForInputs), std::memory_order_relaxed);
    g_rr_last_submit_outcome.store(static_cast<uint32_t>(RrSubmitOutcome::DeviceUnsafe), std::memory_order_relaxed);
    g_rr_last_submit_hr.store(static_cast<uint32_t>(E_FAIL), std::memory_order_relaxed);
    g_rr_last_signal_a.store(0u, std::memory_order_relaxed);
    g_rr_last_output_queued.store(false, std::memory_order_relaxed);
    SetRrInteropStatus("input copies queued but D3D11 completion signal failed; resources retained");
    if (RrDebugLogEnabled()) {
      GpuEvidenceSnapshot snapshot{};
      snapshot.event = GpuEvidenceEvent::RrFailure;
      snapshot.source = GpuEvidenceSource::RrD3D12;
      snapshot.rr_runtime_generation = runtime.RuntimeId();
      snapshot.rr_session_generation = runtime.SessionGeneration();
      snapshot.rr_bridge_generation = replacement->generation;
      snapshot.rr_handle = UINT_MAX;
      snapshot.rr_stage = static_cast<uint32_t>(RrSubmitStage::WaitForInputs);
      snapshot.rr_outcome = static_cast<uint32_t>(RrSubmitOutcome::DeviceUnsafe);
      snapshot.rr_hresult = static_cast<uint32_t>(E_FAIL);
      snapshot.rr_signal_c_target = UINT64_MAX;
      snapshot.rr_completed_c = UINT64_MAX;
      snapshot.rt12_generation = UINT64_MAX;
      snapshot.rt12_submitted = UINT64_MAX;
      snapshot.rt12_completed = UINT64_MAX;
      snapshot.rt12_progress_age_us = UINT64_MAX;
      snapshot.rt12_submission_age_us = UINT64_MAX;
      ::falcom_world::RecordGpuEvidence(snapshot);
      if (!g_rr_evidence_failure_dumped.exchange(true, std::memory_order_relaxed)) DumpGpuEvidence();
    }
    return false;
  }
  if (RrDebugLogEnabled() && (new_generation || frame % 120u == 0u)) {
    renodx::utils::log::i("[world-rr] bridge copy frame=", frame, " generation=", replacement->generation,
                          " A=", signal, " completed=", runtime.Fence11()->GetCompletedValue(), " color=", descs[0].Width, "x",
                          descs[0].Height, " motion=", descs[1].Width, "x", descs[1].Height, " guides=", descs[5].Width,
                          "x", descs[5].Height, " output=", descs[6].Width, "x", descs[6].Height, " active=", input.active_width,
                          "x", input.active_height, " bases=", input.color_base_x, ",", input.color_base_y, "/",
                          input.depth_base_x, ",", input.depth_base_y, "/", input.motion_base_x, ",", input.motion_base_y);
  }
  if (prepared_out != nullptr) *prepared_out = prepared;
  return true;
}

struct RrD3D12Quarantine {
  std::shared_mutex mutex;
  std::vector<std::shared_ptr<RrD3D12Runtime>> runtimes;
};

inline RrD3D12Quarantine& GetRrD3D12Quarantine() {
  static RrD3D12Quarantine* quarantine = new RrD3D12Quarantine();
  return *quarantine;
}

inline void RetireRrD3D12Runtime(std::shared_ptr<RrD3D12Runtime> runtime, bool drain = true) {
  if (runtime == nullptr) return;
  if (drain) {
    if (runtime->Shutdown()) {
      ReleaseRrInteropOwnersAfterDrain(runtime);
      return;
    }
    ShutdownRrInteropDevice(runtime->Device11());
  }
  RrD3D12Quarantine& quarantine = GetRrD3D12Quarantine();
  bool inserted = false;
  {
    const std::unique_lock lock(quarantine.mutex);
    if (std::find(quarantine.runtimes.begin(), quarantine.runtimes.end(), runtime) == quarantine.runtimes.end()) {
      quarantine.runtimes.push_back(runtime);
      inserted = true;
    }
  }
  if (inserted && drain && RrDebugLogEnabled()) {
    renodx::utils::log::w("[world-rr] retained D3D12 runtime generation for process lifetime: ", runtime->Status());
  }
}

inline std::shared_ptr<RrD3D12Runtime> EnsureRrD3D12Runtime(ID3D11Device* device, HMODULE ngx_module,
                                                            const NgxInitIdentity& identity) {
  std::shared_ptr<RrD3D12Runtime> retired;
  {
    const std::unique_lock lock(g_rr_d3d12_mutex);
    if (g_rr_d3d12_runtime != nullptr && g_rr_d3d12_runtime->Matches(device, identity.session_generation)) return g_rr_d3d12_runtime;
    retired = std::exchange(g_rr_d3d12_runtime, {});
  }
  RetireRrD3D12Runtime(std::move(retired), false);
  auto runtime = std::make_shared<RrD3D12Runtime>();
  runtime->Initialize(device, ngx_module, identity);
  {
    const std::unique_lock lock(g_rr_d3d12_mutex);
    g_rr_d3d12_runtime = runtime;
  }
  return runtime;
}

inline void DestroyRrD3D12Runtime(ID3D11Device* device, bool drain = true) {
  std::shared_ptr<RrD3D12Runtime> retired;
  {
    const std::unique_lock lock(g_rr_d3d12_mutex);
    const uint64_t session_generation = CopyNgxIdentity().session_generation;
    if (g_rr_d3d12_runtime != nullptr
        && (device == nullptr || g_rr_d3d12_runtime->Matches(device, session_generation))) {
      retired = std::exchange(g_rr_d3d12_runtime, {});
    }
  }
  RetireRrD3D12Runtime(std::move(retired), drain);
}

inline void PollRrD3D12Lifecycle() {
  if (const std::shared_ptr<RrD3D12Runtime> runtime = SnapshotRrD3D12Runtime(); runtime != nullptr) {
    runtime->PollLifecycle();
    g_rr_last_completed_b.store(runtime->CompletedB(), std::memory_order_relaxed);
    g_rr_last_completed_c.store(runtime->CompletedC(), std::memory_order_relaxed);
  }
  RrD3D12Quarantine& quarantine = GetRrD3D12Quarantine();
  std::vector<std::shared_ptr<RrD3D12Runtime>> snapshot;
  std::vector<std::shared_ptr<RrD3D12Runtime>> completed;
  {
    const std::shared_lock lock(quarantine.mutex);
    snapshot = quarantine.runtimes;
  }
  for (const auto& runtime : snapshot) {
    if (runtime->Shutdown(std::chrono::milliseconds(0))) completed.push_back(runtime);
  }
  if (completed.empty()) return;
  {
    const std::unique_lock lock(quarantine.mutex);
    std::erase_if(quarantine.runtimes, [&](const auto& runtime) {
      return std::find(completed.begin(), completed.end(), runtime) != completed.end();
    });
  }
  for (const auto& runtime : completed) ReleaseRrInteropOwnersAfterDrain(runtime);
}

}  // namespace falcom_world::rr
