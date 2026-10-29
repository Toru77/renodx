/*
 * Copyright (C) 2026
 * SPDX-License-Identifier: MIT
 */
#pragma once

#include <array>
#include <mutex>
#include <string>

#include <Windows.h>

#include "./streamline/include/sl.h"
#ifndef _UNICODE
#define FARCRY6_RESTORE_UNICODE
#endif
#ifndef UNICODE
#define FARCRY6_RESTORE_WIN32_UNICODE
#endif
#include "./streamline/include/sl_security.h"
#ifdef FARCRY6_RESTORE_UNICODE
#undef _UNICODE
#undef FARCRY6_RESTORE_UNICODE
#endif
#ifdef FARCRY6_RESTORE_WIN32_UNICODE
#undef UNICODE
#undef FARCRY6_RESTORE_WIN32_UNICODE
#endif

namespace farcry6_dlss::streamline {

// Streamline stays loaded for the process lifetime. It must not be unloaded from
// DllMain, where Windows holds the loader lock.
struct Runtime {
  HMODULE module = nullptr;
  PFun_slInit* init = nullptr;
  PFun_slShutdown* shutdown = nullptr;
  PFun_slFreeResources* free_resources = nullptr;
  PFun_slSetD3DDevice* set_device = nullptr;
  PFun_slUpgradeInterface* upgrade_interface = nullptr;
  PFun_slGetNativeInterface* get_native_interface = nullptr;
  PFun_slIsFeatureSupported* is_feature_supported = nullptr;
  PFun_slGetFeatureFunction* get_feature_function = nullptr;
  PFun_slGetNewFrameToken* get_frame_token = nullptr;
  PFun_slSetConstants* set_constants = nullptr;
  PFun_slSetTagForFrame* set_tag = nullptr;
  const char* status = "not checked";
};

inline Runtime runtime;
inline std::once_flag load_once;
inline void LoadRuntime(const std::wstring& process_directory) {
  std::call_once(load_once, [&] {
    if (process_directory.empty() || process_directory == L".") {
      runtime.status = "cannot locate FarCry6.exe directory";
      return;
    }
    const std::wstring path = process_directory + L"\\sl.interposer.dll";
    if (!sl::security::verifyEmbeddedSignature(path.c_str())) {
      runtime.status = "interposer missing or NVIDIA signature verification failed";
      return;
    }
    HMODULE module = LoadLibraryExW(path.c_str(), nullptr, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (module == nullptr) {
      runtime.status = "failed to load interposer from game directory";
      return;
    }
    std::array<wchar_t, 32768> loaded_path = {};
    const DWORD length = GetModuleFileNameW(module, loaded_path.data(), static_cast<DWORD>(loaded_path.size()));
    if (length == 0u || length >= loaded_path.size() || _wcsicmp(path.c_str(), loaded_path.data()) != 0) {
      FreeLibrary(module);
      runtime.status = "a different interposer is already loaded";
      return;
    }
    Runtime found;
    found.module = module;
    found.init = reinterpret_cast<PFun_slInit*>(GetProcAddress(module, "slInit"));
    found.shutdown = reinterpret_cast<PFun_slShutdown*>(GetProcAddress(module, "slShutdown"));
    found.free_resources = reinterpret_cast<PFun_slFreeResources*>(GetProcAddress(module, "slFreeResources"));
    found.set_device = reinterpret_cast<PFun_slSetD3DDevice*>(GetProcAddress(module, "slSetD3DDevice"));
    found.upgrade_interface = reinterpret_cast<PFun_slUpgradeInterface*>(GetProcAddress(module, "slUpgradeInterface"));
    found.get_native_interface = reinterpret_cast<PFun_slGetNativeInterface*>(GetProcAddress(module, "slGetNativeInterface"));
    found.is_feature_supported = reinterpret_cast<PFun_slIsFeatureSupported*>(GetProcAddress(module, "slIsFeatureSupported"));
    found.get_feature_function = reinterpret_cast<PFun_slGetFeatureFunction*>(GetProcAddress(module, "slGetFeatureFunction"));
    found.get_frame_token = reinterpret_cast<PFun_slGetNewFrameToken*>(GetProcAddress(module, "slGetNewFrameToken"));
    found.set_constants = reinterpret_cast<PFun_slSetConstants*>(GetProcAddress(module, "slSetConstants"));
    found.set_tag = reinterpret_cast<PFun_slSetTagForFrame*>(GetProcAddress(module, "slSetTagForFrame"));
    if (found.init == nullptr || found.shutdown == nullptr || found.set_device == nullptr
        || found.upgrade_interface == nullptr || found.get_native_interface == nullptr
        || found.is_feature_supported == nullptr || found.get_feature_function == nullptr
        || found.get_frame_token == nullptr || found.set_constants == nullptr || found.set_tag == nullptr) {
      FreeLibrary(module);
      runtime.status = "interposer is missing required Streamline exports";
      return;
    }
    found.status = "loaded";
    runtime = found;
  });
}

}  // namespace farcry6_dlss::streamline
