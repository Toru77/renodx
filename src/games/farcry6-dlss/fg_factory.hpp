/*
 * Copyright (C) 2026
 * SPDX-License-Identifier: MIT
 */
#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <shared_mutex>
#include <string>

#include <d3d12.h>
#include <source/dxgi/dxgi_factory.hpp>

#include "../../utils/detour.hpp"
#include "./fg_native_factory.hpp"
#include "./streamline_runtime.hpp"
#include "./streamline/include/sl_dlss_g.h"
#include "./streamline/include/sl_pcl.h"
#include "./streamline/include/sl_reflex.h"

namespace farcry6_dlss::streamline {

inline std::mutex sdk_interface_mutex;
// Serializes frame APIs with shutdown; acquire before bootstrap_mutex so a
// Streamline call can safely trigger the command-queue hook.
inline std::mutex frame_api_mutex;
struct StreamlineMessages {
  std::shared_mutex mutex;
  std::array<std::string, 8> lines = {};
  uint64_t count = 0u;
};
inline StreamlineMessages streamline_messages;

inline void OnStreamlineMessage(sl::LogType type, const char* message) {
  if (message == nullptr) return;
  const bool disabled_reason = type == sl::LogType::eInfo && std::strstr(message, "DLSS-G disabled:") != nullptr;
  if (type != sl::LogType::eWarn && type != sl::LogType::eError && !disabled_reason) return;
  std::unique_lock lock(streamline_messages.mutex);
  auto& line = streamline_messages.lines[streamline_messages.count++ % streamline_messages.lines.size()];
  line = (type == sl::LogType::eError ? "error: " : (disabled_reason ? "info: " : "warning: "));
  line.append(message, (std::min)(std::strlen(message), size_t{512u}));
}

// Called before ReShade forwards swapchain creation to its underlying factory.
// Repeated swapchain creation may use the same factory, so do not wrap it twice.
inline bool UpgradeReShadeFactory(IDXGIFactory* factory) {
  if (factory == nullptr || runtime.upgrade_interface == nullptr || runtime.get_native_interface == nullptr) return false;

  static std::mutex factory_mutex;
  std::lock_guard lock(factory_mutex);
  std::lock_guard sdk_lock(sdk_interface_mutex);
  DXGIFactory* reshade_factory = nullptr;
  UINT size = sizeof(reshade_factory);
  if (FAILED(factory->GetPrivateData(__uuidof(DXGIFactory), &size, &reshade_factory))
      || size != sizeof(reshade_factory) || reshade_factory == nullptr || reshade_factory->_orig == nullptr) return false;

  void* upgraded = reshade_factory->_orig;
  void* native = nullptr;
  if (runtime.get_native_interface(upgraded, &native) != sl::Result::eOk || native == nullptr) return false;
  const bool already_upgraded = native != upgraded;
  static_cast<IUnknown*>(native)->Release();
  if (already_upgraded) return true;
  if (runtime.upgrade_interface(&upgraded) != sl::Result::eOk || upgraded == nullptr || upgraded == reshade_factory->_orig) return false;
  reshade_factory->_orig->Release();
  reshade_factory->_orig = static_cast<IDXGIFactory*>(upgraded);
  return true;
}

using CreateSwapChainForHwnd = HRESULT(STDMETHODCALLTYPE*)(IDXGIFactory2*, IUnknown*, HWND, const DXGI_SWAP_CHAIN_DESC1*,
                                                          const DXGI_SWAP_CHAIN_FULLSCREEN_DESC*, IDXGIOutput*, IDXGISwapChain1**);
inline CreateSwapChainForHwnd original_create_swapchain_for_hwnd = nullptr;
inline std::atomic<bool> factory_hook_active = false;
inline std::atomic<bool> factory_hook_native = false;
inline std::atomic<uint64_t> factory_hook_upgrades = 0;
inline std::atomic<uint64_t> factory_hook_failures = 0;
inline std::atomic<const char*> bootstrap_status = "off";
inline std::atomic<int> bootstrap_result = 0;
inline std::atomic<int> bootstrap_fg_support = -1;
inline std::atomic<int> bootstrap_reflex_support = -1;
inline std::atomic<uint32_t> bootstrap_missing_functions = 0u;
inline std::atomic<uint64_t> queue_hook_count = 0u;
inline std::atomic<uint64_t> queue_hook_failures = 0u;
inline std::mutex bootstrap_mutex;
inline bool bootstrap_initialized = false;
inline std::atomic<void*> bootstrap_device = nullptr;
inline PFun_slDLSSGSetOptions* fg_set_options = nullptr;
inline PFun_slDLSSGGetState* fg_get_state = nullptr;
inline PFun_slReflexSetOptions* reflex_set_options = nullptr;
inline PFun_slReflexSleep* reflex_sleep = nullptr;
inline PFun_slPCLSetMarker* pcl_set_marker = nullptr;
using CreateCommandQueue = HRESULT(STDMETHODCALLTYPE*)(ID3D12Device*, const D3D12_COMMAND_QUEUE_DESC*, REFIID, void**);
inline CreateCommandQueue original_create_queue = nullptr;
inline void* queue_hook_target = nullptr;
inline ID3D12Device* queue_proxy_device = nullptr;

// Streamline must see queue creation, but ReShade and the game need the native
// queue for their other operations. The proxy device remains alive until the
// ReShade device is destroyed; Streamline's queue proxy refers back to it.
inline HRESULT STDMETHODCALLTYPE HookCreateCommandQueue(ID3D12Device* device, const D3D12_COMMAND_QUEUE_DESC* desc,
                                                         REFIID iid, void** queue) {
  static thread_local bool inside_queue = false;
  if (inside_queue || !factory_hook_active.load(std::memory_order_acquire)
      || device != bootstrap_device.load(std::memory_order_acquire)) return original_create_queue(device, desc, iid, queue);
  std::lock_guard lock(bootstrap_mutex);
  if (!bootstrap_initialized || queue_proxy_device == nullptr || device != bootstrap_device.load()) {
    return original_create_queue(device, desc, iid, queue);
  }
  inside_queue = true;
  HRESULT result = queue_proxy_device->CreateCommandQueue(desc, iid, queue);
  inside_queue = false;
  if (SUCCEEDED(result) && queue != nullptr && *queue != nullptr) {
    void* native = nullptr;
    sl::Result native_result = sl::Result::eErrorInvalidParameter;
    {
      std::lock_guard sdk_lock(sdk_interface_mutex);
      native_result = runtime.get_native_interface(*queue, &native);
    }
    if (native_result == sl::Result::eOk && native != nullptr) {
      if (native != *queue) {
        static_cast<IUnknown*>(*queue)->Release();
        *queue = native;
      } else {
        static_cast<IUnknown*>(native)->Release();
      }
      queue_hook_count.fetch_add(1u, std::memory_order_relaxed);
      return result;
    }
    static_cast<IUnknown*>(*queue)->Release();
    *queue = nullptr;
    queue_hook_failures.fetch_add(1u, std::memory_order_relaxed);
    return original_create_queue(device, desc, iid, queue);
  }
  queue_hook_failures.fetch_add(1u, std::memory_order_relaxed);
  return result;
}

inline HRESULT STDMETHODCALLTYPE HookCreateSwapChainForHwnd(IDXGIFactory2* factory, IUnknown* device, HWND window,
                                                             const DXGI_SWAP_CHAIN_DESC1* desc, const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* fullscreen,
                                                             IDXGIOutput* output, IDXGISwapChain1** swapchain) {
  static thread_local bool inside_native_factory = false;
  if (inside_native_factory) return original_create_swapchain_for_hwnd(factory, device, window, desc, fullscreen, output, swapchain);
  const bool active = factory_hook_active.load(std::memory_order_acquire);
  bool upgraded = false;
  HRESULT result = E_FAIL;
  if (active && factory_hook_native.load(std::memory_order_relaxed) && device != nullptr) {
    ID3D12CommandQueue* queue = nullptr;
    ID3D12Device* queue_device = nullptr;
    bool matches = false;
    if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&queue))) && queue != nullptr
        && SUCCEEDED(queue->GetDevice(IID_PPV_ARGS(&queue_device))) && queue_device != nullptr) {
      void* native_device = queue_device;
      sl::Result native_result = sl::Result::eErrorInvalidParameter;
      {
        std::lock_guard sdk_lock(sdk_interface_mutex);
        native_result = runtime.get_native_interface(queue_device, &native_device);
      }
      if (native_result == sl::Result::eOk) {
        matches = native_device == bootstrap_device.load(std::memory_order_acquire);
        if (native_device != nullptr) static_cast<IUnknown*>(native_device)->Release();
      }
    }
    if (queue_device != nullptr) queue_device->Release();
    if (queue != nullptr) queue->Release();
    if (!matches) return original_create_swapchain_for_hwnd(factory, device, window, desc, fullscreen, output, swapchain);
    void* wrapped = factory;
    {
      std::lock_guard sdk_lock(sdk_interface_mutex);
      upgraded = runtime.upgrade_interface(&wrapped) == sl::Result::eOk && wrapped != nullptr && wrapped != factory;
    }
    if (upgraded) {
      inside_native_factory = true;
      result = static_cast<IDXGIFactory2*>(wrapped)->CreateSwapChainForHwnd(device, window, desc, fullscreen, output, swapchain);
      inside_native_factory = false;
      static_cast<IDXGIFactory2*>(wrapped)->Release();
    } else {
      result = original_create_swapchain_for_hwnd(factory, device, window, desc, fullscreen, output, swapchain);
    }
  } else {
    upgraded = active && UpgradeReShadeFactory(factory);
    result = original_create_swapchain_for_hwnd(factory, device, window, desc, fullscreen, output, swapchain);
  }
  if (active) {
    if (upgraded && SUCCEEDED(result)) {
      factory_hook_upgrades.fetch_add(1, std::memory_order_relaxed);
      bootstrap_status.store("swapchain connected", std::memory_order_relaxed);
    } else {
      factory_hook_failures.fetch_add(1, std::memory_order_relaxed);
      bootstrap_status.store("swapchain connection failed", std::memory_order_relaxed);
    }
  }
  return result;
}

// The hook and its trampoline remain installed until process exit. Pin their
// containing module first so that disabling an addon cannot leave a dangling hook.
inline bool InstallFactoryHook(IDXGIFactory2* factory) {
  static std::mutex installation_mutex;
  std::lock_guard lock(installation_mutex);
  if (factory == nullptr || original_create_swapchain_for_hwnd != nullptr) return false;
  DXGIFactory* reshade_factory = nullptr;
  UINT size = sizeof(reshade_factory);
  const bool proxy = SUCCEEDED(factory->GetPrivateData(__uuidof(DXGIFactory), &size, &reshade_factory))
                  && size == sizeof(reshade_factory) && reshade_factory != nullptr;
  void* target = proxy ? (*reinterpret_cast<void***>(factory))[15] : FindNativeFactoryMethod(factory, 15u);
  if (target == nullptr) {
    bootstrap_status.store(proxy ? "factory method unavailable" : "native factory method unavailable");
    return false;
  }
  HMODULE pinned = nullptr;
  if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
                          reinterpret_cast<LPCWSTR>(&HookCreateSwapChainForHwnd), &pinned)) {
    bootstrap_status.store("unable to pin factory hook");
    bootstrap_result.store(static_cast<int>(GetLastError()));
    return false;
  }
  factory_hook_native.store(!proxy, std::memory_order_relaxed);
  original_create_swapchain_for_hwnd = reinterpret_cast<CreateSwapChainForHwnd>(target);
  try {
    const std::array hooks = {renodx::utils::detour::Function(&original_create_swapchain_for_hwnd, &HookCreateSwapChainForHwnd)};
    renodx::utils::detour::Install(hooks);
  } catch (const std::system_error& error) {
    bootstrap_result.store(error.code().value());
    bootstrap_status.store("factory detour installation failed");
    original_create_swapchain_for_hwnd = nullptr;
    return false;
  } catch (...) {
    bootstrap_status.store("factory detour installation failed");
    original_create_swapchain_for_hwnd = nullptr;
    return false;
  }
  return true;
}

// This is a presentation-only smoke test. It does not enable DLSS-G or tag inputs.
inline void StartBootstrap(const std::wstring& directory) {
  std::lock_guard lock(bootstrap_mutex);
  if (bootstrap_initialized) return;
  LoadRuntime(directory);
  if (runtime.module == nullptr) {
    bootstrap_status.store("interposer unavailable");
    return;
  }
  if (original_create_swapchain_for_hwnd == nullptr) {
    HMODULE dxgi = GetModuleHandleW(L"dxgi.dll");
    auto create_factory = dxgi == nullptr ? nullptr : reinterpret_cast<decltype(&CreateDXGIFactory2)>(GetProcAddress(dxgi, "CreateDXGIFactory2"));
    IDXGIFactory2* factory = nullptr;
    if (create_factory == nullptr || FAILED(create_factory(0, IID_PPV_ARGS(&factory))) || factory == nullptr) {
      bootstrap_status.store("ReShade factory unavailable");
      return;
    }
    const bool installed = InstallFactoryHook(factory);
    factory->Release();
    if (!installed) {
      return;
    }
  }
  sl::Feature features[] = {sl::kFeatureDLSS_G, sl::kFeatureReflex};
  const wchar_t* paths[] = {directory.c_str()};
  sl::Preferences preferences = {};
  preferences.flags = sl::PreferenceFlags::eUseManualHooking | sl::PreferenceFlags::eUseFrameBasedResourceTagging;
  preferences.featuresToLoad = features;
  preferences.numFeaturesToLoad = 2;
  preferences.pathsToPlugins = paths;
  preferences.numPathsToPlugins = 1;
  preferences.projectId = "a3f4c8e1-7b2d-4f6a-9c1e-5d8b0f2a6c4e";
  preferences.engineVersion = "1.0";
  preferences.logMessageCallback = &OnStreamlineMessage;
  const sl::Result result = runtime.init(preferences, sl::kSDKVersion);
  bootstrap_result.store(static_cast<int>(result));
  if (result != sl::Result::eOk) {
    bootstrap_status.store("Streamline initialization failed");
    return;
  }
  bootstrap_initialized = true;
  bootstrap_status.store("initialized; waiting for NVIDIA device");
}

inline void RegisterBootstrapDevice(void* device) {
  std::lock_guard lock(bootstrap_mutex);
  if (!bootstrap_initialized || device == nullptr || bootstrap_device.load() != nullptr) return;
  const sl::Result result = runtime.set_device(device);
  bootstrap_result.store(static_cast<int>(result));
  if (result != sl::Result::eOk) {
    (void)runtime.shutdown();
    bootstrap_initialized = false;
    bootstrap_status.store("Streamline device registration failed");
    return;
  }
  bootstrap_device.store(device, std::memory_order_release);
  LUID luid = static_cast<ID3D12Device*>(device)->GetAdapterLuid();
  sl::AdapterInfo adapter = {};
  adapter.deviceLUID = reinterpret_cast<uint8_t*>(&luid);
  adapter.deviceLUIDSizeInBytes = sizeof(luid);
  bootstrap_fg_support.store(static_cast<int>(runtime.is_feature_supported(sl::kFeatureDLSS_G, adapter)));
  bootstrap_reflex_support.store(static_cast<int>(runtime.is_feature_supported(sl::kFeatureReflex, adapter)));
  uint32_t missing = 0u;
  const auto resolve = [&missing](sl::Feature feature, const char* name, uint32_t bit) {
    void* function = nullptr;
    if (runtime.get_feature_function(feature, name, function) != sl::Result::eOk || function == nullptr) missing |= bit;
    return function;
  };
  fg_set_options = reinterpret_cast<PFun_slDLSSGSetOptions*>(resolve(sl::kFeatureDLSS_G, "slDLSSGSetOptions", 1u));
  fg_get_state = reinterpret_cast<PFun_slDLSSGGetState*>(resolve(sl::kFeatureDLSS_G, "slDLSSGGetState", 2u));
  reflex_set_options = reinterpret_cast<PFun_slReflexSetOptions*>(resolve(sl::kFeatureReflex, "slReflexSetOptions", 4u));
  reflex_sleep = reinterpret_cast<PFun_slReflexSleep*>(resolve(sl::kFeatureReflex, "slReflexSleep", 8u));
  pcl_set_marker = reinterpret_cast<PFun_slPCLSetMarker*>(resolve(sl::kFeaturePCL, "slPCLSetMarker", 16u));
  bootstrap_missing_functions.store(missing);
  void* queue_target = (*reinterpret_cast<void***>(device))[8];
  if (queue_hook_target == nullptr && queue_target != nullptr) {
    original_create_queue = reinterpret_cast<CreateCommandQueue>(queue_target);
    try {
      const std::array hooks = {renodx::utils::detour::Function(&original_create_queue, &HookCreateCommandQueue)};
      renodx::utils::detour::Install(hooks);
      queue_hook_target = queue_target;
    } catch (...) {
      original_create_queue = nullptr;
    }
  }
  if (queue_hook_target != nullptr && queue_hook_target == queue_target) {
    void* proxy = device;
    sl::Result upgrade_result = sl::Result::eErrorInvalidParameter;
    {
      std::lock_guard sdk_lock(sdk_interface_mutex);
      upgrade_result = runtime.upgrade_interface(&proxy);
    }
    if (upgrade_result == sl::Result::eOk && proxy != nullptr && proxy != device) {
      queue_proxy_device = static_cast<ID3D12Device*>(proxy);
    }
  }
  if (queue_proxy_device == nullptr) queue_hook_failures.fetch_add(1u, std::memory_order_relaxed);
  factory_hook_active.store(true, std::memory_order_release);
  bootstrap_status.store("device registered; waiting for swapchain");
}

inline void StopBootstrap(void* device) {
  std::lock_guard frame_lock(frame_api_mutex);
  std::lock_guard lock(bootstrap_mutex);
  if (!bootstrap_initialized || bootstrap_device.load() != device) return;
  factory_hook_active.store(false, std::memory_order_release);
  if (queue_proxy_device != nullptr) {
    queue_proxy_device->Release();
    queue_proxy_device = nullptr;
  }
  const sl::Result result = runtime.shutdown();
  bootstrap_result.store(static_cast<int>(result));
  bootstrap_device.store(nullptr);
  bootstrap_fg_support.store(-1);
  bootstrap_reflex_support.store(-1);
  bootstrap_missing_functions.store(0u);
  fg_set_options = nullptr;
  fg_get_state = nullptr;
  reflex_set_options = nullptr;
  reflex_sleep = nullptr;
  pcl_set_marker = nullptr;
  bootstrap_initialized = false;
  bootstrap_status.store(result == sl::Result::eOk ? "shut down" : "Streamline shutdown failed");
}

}  // namespace farcry6_dlss::streamline
