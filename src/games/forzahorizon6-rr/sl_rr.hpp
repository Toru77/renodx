/*
 * Copyright (C) 2026 Carlos Lopez, speedlemur
 * SPDX-License-Identifier: MIT
 *
 * forzahorizon6-rr: Streamline interception layer (M1 diagnostics).
 *
 * Every Streamline core entry point the game can use is detoured, observed,
 * and forwarded untouched. Nothing here changes behavior yet: the point of
 * M1 is an on-screen picture of how Forza Horizon 6 talks to Streamline
 * (sl.dlss / sl.dlss_g), what buffers it tags, what constants it feeds, and
 * whether the shipped sl.dlss_d (Ray Reconstruction) plugin is reachable
 * through the game's own SL instance.
 *
 * Diagnostics are UI-first: all state is collected here and drawn by the
 * addon overlay (see addon.cpp). Log output is limited to hook installation
 * and hard failures.
 */

#pragma once

#include <windows.h>
#include <d3d12.h>

#include <detours.h>
#include <intrin.h>

#include <array>
#include <atomic>
#include <climits>
#include <cstdint>
#include <cstring>
#include <map>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

#include <sl.h>
#include <sl_helpers.h>

#include <include/reshade.hpp>

#include "../../utils/date.hpp"
#include "../../utils/platform.hpp"

namespace sl_rr {

// ---------------------------------------------------------------------------
// logging — only hook arming and hard failures; everything else is on the UI
// ---------------------------------------------------------------------------

inline void LogInfo(const std::string& msg) {
  reshade::log::message(reshade::log::level::info, ("sl-rr: " + msg).c_str());
}

inline void LogWarn(const std::string& msg) {
  reshade::log::message(reshade::log::level::warning, ("sl-rr: " + msg).c_str());
}

// Splits a multi-line report into individual log lines (used by the UI's
// "write report to log" button; normal operation writes nothing).
inline void LogReport(const std::string& report) {
  std::stringstream ss(report);
  std::string line;
  int count = 0;
  while (std::getline(ss, line) && count < 512) {
    reshade::log::message(reshade::log::level::info, line.c_str());
    ++count;
  }
}

// ---------------------------------------------------------------------------
// small formatting helpers (shared by the UI and the report)
// ---------------------------------------------------------------------------

inline std::string ModuleNameOf(void* address) {
  if (address == nullptr) return "?";
  HMODULE module = nullptr;
  if (GetModuleHandleExA(
          GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
          static_cast<LPCSTR>(address), &module)
      == 0) {
    return "?";
  }
  char path[MAX_PATH] = {};
  if (GetModuleFileNameA(module, path, MAX_PATH) == 0) return "?";
  const char* slash = strrchr(path, '\\');
  return slash != nullptr ? slash + 1 : path;
}

// Strips a known prefix so UI cells stay short ("kFeatureDLSS" -> "DLSS").
inline std::string ShortName(const char* name, const char* prefix) {
  std::string text = name != nullptr ? name : "?";
  const std::string strip = prefix != nullptr ? prefix : "";
  if (!strip.empty() && text.rfind(strip, 0) == 0) {
    text = text.substr(strip.size());
  }
  return text;
}

inline std::string FeatureName(uint32_t feature) {
  return ShortName(sl::getFeatureAsStr(feature), "kFeature");
}

inline std::string BufferName(uint32_t type) {
  return ShortName(sl::getBufferTypeAsStr(type), "kBufferType");
}

constexpr int kNever = INT_MIN;

inline std::string ResultName(int result) {
  if (result == kNever) return "-";
  return sl::getResultAsStr(static_cast<sl::Result>(result));
}

inline const char* BooleanName(sl::Boolean value) {
  switch (value) {
    case sl::eFalse: return "false";
    case sl::eTrue: return "true";
    default: return "unset";
  }
}

inline std::string LifecycleName(uint32_t lifecycle) {
  return sl::getResourceLifecycleAsStr(static_cast<sl::ResourceLifecycle>(lifecycle));
}

inline const char* DxgiFormatName(uint32_t format) {
  switch (format) {
    case 0: return "UNKNOWN";
    case 1: return "R32G32B32A32_TYPELESS";
    case 2: return "R32G32B32A32_FLOAT";
    case 3: return "R32G32B32A32_UINT";
    case 5: return "R32G32B32_TYPELESS";
    case 6: return "R32G32B32_FLOAT";
    case 9: return "R16G16B16A16_TYPELESS";
    case 10: return "R16G16B16A16_FLOAT";
    case 11: return "R16G16B16A16_UNORM";
    case 12: return "R16G16B16A16_UINT";
    case 15: return "R32G32_TYPELESS";
    case 16: return "R32G32_FLOAT";
    case 17: return "R32G32_UINT";
    case 23: return "R10G10B10A2_TYPELESS";
    case 24: return "R10G10B10A2_UNORM";
    case 25: return "R10G10B10A2_UINT";
    case 26: return "R11G11B10_FLOAT";
    case 27: return "R8G8B8A8_TYPELESS";
    case 28: return "R8G8B8A8_UNORM";
    case 29: return "R8G8B8A8_UNORM_SRGB";
    case 30: return "R8G8B8A8_UINT";
    case 33: return "R16G16_TYPELESS";
    case 34: return "R16G16_FLOAT";
    case 35: return "R16G16_UNORM";
    case 36: return "R16G16_UINT";
    case 39: return "R32_TYPELESS";
    case 40: return "D32_FLOAT";
    case 41: return "R32_FLOAT";
    case 42: return "R32_UINT";
    case 45: return "D24_UNORM_S8_UINT";
    case 48: return "R8G8_TYPELESS";
    case 49: return "R8G8_UNORM";
    case 53: return "R16_TYPELESS";
    case 54: return "R16_FLOAT";
    case 55: return "D16_UNORM";
    case 56: return "R16_UNORM";
    case 57: return "R16_UINT";
    case 60: return "R8_TYPELESS";
    case 61: return "R8_UNORM";
    case 62: return "R8_UINT";
    case 67: return "R9G9B9E5_SHAREDEXP";
    case 87: return "B8G8R8A8_UNORM";
    case 88: return "B8G8R8X8_UNORM";
    case 103: return "NV12";
    case 104: return "P010";
    default: return nullptr;
  }
}

inline std::string FormatLabel(uint32_t format) {
  const char* name = DxgiFormatName(format);
  if (name != nullptr) return name;
  return "fmt#" + std::to_string(format);
}

// sdkVersion packing: (major << 48) | (minor << 32) | (patch << 16) | magic.
inline std::string SdkVersionLabel(uint64_t sdk_version) {
  if (sdk_version == 0) return "-";
  std::stringstream s;
  s << ((sdk_version >> 48) & 0xFFFFu) << '.' << ((sdk_version >> 32) & 0xFFFFu) << '.'
    << ((sdk_version >> 16) & 0xFFFFu);
  return s.str();
}

// ---------------------------------------------------------------------------
// hook bookkeeping
// ---------------------------------------------------------------------------

enum HookIndex : int {
  kHookInit = 0,
  kHookShutdown,
  kHookSetD3DDevice,
  kHookGetNewFrameToken,
  kHookSetTag,
  kHookSetTagForFrame,
  kHookSetConstants,
  kHookEvaluateFeature,
  kHookGetFeatureFunction,
  kHookIsFeatureSupported,
  kHookIsFeatureLoaded,
  kHookSetFeatureLoaded,
  kHookGetFeatureVersion,
  kHookAllocateResources,
  kHookFreeResources,
};

constexpr int kHookCount = 15;

inline const char* const kHookNames[kHookCount] = {
    "slInit", "slShutdown", "slSetD3DDevice", "slGetNewFrameToken",
    "slSetTag", "slSetTagForFrame", "slSetConstants", "slEvaluateFeature",
    "slGetFeatureFunction", "slIsFeatureSupported", "slIsFeatureLoaded",
    "slSetFeatureLoaded", "slGetFeatureVersion", "slAllocateResources",
    "slFreeResources",
};

// ---------------------------------------------------------------------------
// diagnostics state — written by hooks (game threads), read by the overlay
// ---------------------------------------------------------------------------

struct HookRow {
  bool installed = false;
  uint64_t calls = 0;
  void* last_caller = nullptr;
};

struct TagRow {
  uint32_t type = 0;
  uint64_t calls = 0;
  uint32_t last_frame = 0;
  uint32_t last_viewport = 0xFFFFFFFF;
  uint32_t lifecycle = 0;
  bool via_frame_api = false;
  // Tagged resource (as reported by the game via sl::Resource)
  void* native = nullptr;
  bool cleared = false;
  uint32_t sl_format = 0;
  uint32_t sl_width = 0;
  uint32_t sl_height = 0;
  uint32_t sl_state = 0xFFFFFFFF;
  uint32_t extent_w = 0;
  uint32_t extent_h = 0;
  // Native D3D12 resource description (queried from the resource itself)
  bool desc_ok = false;
  uint32_t desc_format = 0;
  uint64_t desc_width = 0;
  uint32_t desc_height = 0;
  uint32_t desc_layers = 1;
};

struct FeatureRow {
  uint32_t feature = 0;
  // Observed slEvaluateFeature calls
  uint64_t evaluates = 0;
  uint32_t last_eval_frame = 0;
  int last_eval_result = kNever;
  uint32_t last_eval_inputs = 0;
  uint32_t last_eval_input_type = 0;
  uint32_t last_eval_input_version = 0;
  void* last_eval_caller = nullptr;
  // Last known answers to the standard queries (from game calls or our probe)
  int supported_result = kNever;
  int loaded_result = kNever;
  bool loaded = false;
  int version_result = kNever;
  uint32_t sl_major = 0, sl_minor = 0, sl_build = 0;
  uint32_t ngx_major = 0, ngx_minor = 0, ngx_build = 0;
  int set_loaded_value = kNever;  // -1 unloaded, 1 loaded
  bool probed = false;
  uint64_t queries = 0;  // observed query calls from the game
};

struct EvaluateCall {
  uint32_t frame = 0;
  uint32_t feature = 0;
  uint32_t num_inputs = 0;
  uint32_t primary_input_type = 0;
  int result = 0;
  void* caller = nullptr;
};

struct FeatureFunctionRow {
  uint32_t feature = 0;
  std::string name;
  uint64_t calls = 0;
  int last_result = 0;
  uint32_t last_frame = 0;
};

struct Diagnostics {
  // module
  bool module_found = false;
  std::string module_path;
  bool armed = false;
  int hooks_installed = 0;
  std::array<HookRow, kHookCount> hooks{};

  // init
  bool init_seen = false;
  int init_result = kNever;
  uint64_t init_sdk_version = 0;
  uint64_t init_flags = 0;
  int init_log_level = 0;
  int init_engine = 0;
  uint32_t init_app_id = 0;
  int init_render_api = 0;
  uint32_t init_feature_count = 0;
  std::array<uint32_t, 8> init_features{};

  bool shutdown_seen = false;

  // device
  bool device_seen = false;
  uint64_t device = 0;
  bool luid_valid = false;
  std::array<uint8_t, 8> luid{};

  // frames
  uint64_t frame_tokens = 0;
  uint32_t last_frame_index = 0;

  // tags
  uint64_t tag_calls = 0;
  uint64_t tag_for_frame_calls = 0;
  std::vector<TagRow> tags;

  // constants
  bool constants_seen = false;
  uint64_t constants_calls = 0;
  uint32_t constants_last_frame = 0;
  uint32_t constants_last_viewport = 0xFFFFFFFF;
  sl::Constants constants{};

  // features / evaluates / feature functions
  std::map<uint32_t, FeatureRow> features;
  uint64_t evaluates_total = 0;
  std::vector<EvaluateCall> evaluate_recent;
  std::vector<FeatureFunctionRow> feature_functions;

  // probes
  bool probe_pending = false;
  int probe_scope = 0;  // 0 = DLSS-RR only, 1 = all known features
  bool auto_probe_done = false;
};

inline Diagnostics diagnostics;
inline std::mutex diagnostics_mutex;

inline Diagnostics CaptureDiagnostics() {
  const std::lock_guard lock(diagnostics_mutex);
  return diagnostics;
}

inline FeatureRow& FeatureForLocked(uint32_t feature) {
  auto& row = diagnostics.features[feature];
  row.feature = feature;
  return row;
}

inline void NoteCallLocked(HookIndex index, void* caller) {
  const size_t i = static_cast<size_t>(index);
  auto& row = diagnostics.hooks[i];
  ++row.calls;
  row.last_caller = caller;
}

// ---------------------------------------------------------------------------
// safe queries of native resource descriptions
// ---------------------------------------------------------------------------

struct NativeResourceDesc {
  bool ok = false;
  uint32_t format = 0;
  uint64_t width = 0;
  uint32_t height = 0;
  uint32_t layers = 1;
};

inline NativeResourceDesc QueryNativeDesc(void* native) {
  NativeResourceDesc out{};
  if (native == nullptr) return out;
#if defined(_MSC_VER)
  __try {
    const auto desc = reinterpret_cast<ID3D12Resource*>(native)->GetDesc();
    out.format = static_cast<uint32_t>(desc.Format);
    out.width = desc.Width;
    out.height = desc.Height;
    out.layers = desc.DepthOrArraySize;
    out.ok = true;
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    out.ok = false;
  }
#endif
  return out;
}

inline bool TryGetAdapterLuid(void* device, std::array<uint8_t, 8>& out) {
  if (device == nullptr) return false;
#if defined(_MSC_VER)
  __try {
    const auto luid = reinterpret_cast<ID3D12Device*>(device)->GetAdapterLuid();
    memcpy(out.data(), &luid, sizeof(LUID));
    return true;
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    return false;
  }
#else
  return false;
#endif
}

// ---------------------------------------------------------------------------
// tag recording
// ---------------------------------------------------------------------------

inline void RecordTagListLocked(
    const sl::ViewportHandle& viewport, const sl::ResourceTag* tags, uint32_t num_tags,
    uint32_t frame, bool via_frame_api) {
  if (tags == nullptr) return;
  const uint32_t viewport_id = static_cast<uint32_t>(viewport);
  for (uint32_t i = 0; i < num_tags; ++i) {
    const sl::ResourceTag& tag = tags[i];
    TagRow* row = nullptr;
    for (auto& candidate : diagnostics.tags) {
      if (candidate.type == tag.type) {
        row = &candidate;
        break;
      }
    }
    if (row == nullptr) {
      diagnostics.tags.push_back({});
      row = &diagnostics.tags.back();
      row->type = tag.type;
    }
    ++row->calls;
    row->last_frame = frame;
    row->last_viewport = viewport_id;
    row->lifecycle = static_cast<uint32_t>(tag.lifecycle);
    row->via_frame_api = via_frame_api;
    if (tag.resource == nullptr) {
      row->cleared = true;
      row->native = nullptr;
      continue;
    }
    row->cleared = false;
    row->native = tag.resource->native;
    row->sl_format = tag.resource->nativeFormat;
    row->sl_width = tag.resource->width;
    row->sl_height = tag.resource->height;
    row->sl_state = tag.resource->state;
    row->extent_w = tag.extent.width;
    row->extent_h = tag.extent.height;
    const auto desc = QueryNativeDesc(tag.resource->native);
    row->desc_ok = desc.ok;
    row->desc_format = desc.format;
    row->desc_width = desc.width;
    row->desc_height = desc.height;
    row->desc_layers = desc.layers;
  }
}

// ---------------------------------------------------------------------------
// real function pointers
// ---------------------------------------------------------------------------

inline PFun_slInit* real_sl_init = nullptr;
inline PFun_slShutdown* real_sl_shutdown = nullptr;
inline PFun_slSetD3DDevice* real_sl_set_d3d_device = nullptr;
inline PFun_slGetNewFrameToken* real_sl_get_new_frame_token = nullptr;
inline PFun_slSetTag* real_sl_set_tag = nullptr;
inline PFun_slSetTagForFrame* real_sl_set_tag_for_frame = nullptr;
inline PFun_slSetConstants* real_sl_set_constants = nullptr;
inline PFun_slEvaluateFeature* real_sl_evaluate_feature = nullptr;
inline PFun_slGetFeatureFunction* real_sl_get_feature_function = nullptr;
inline PFun_slIsFeatureSupported* real_sl_is_feature_supported = nullptr;
inline PFun_slIsFeatureLoaded* real_sl_is_feature_loaded = nullptr;
inline PFun_slSetFeatureLoaded* real_sl_set_feature_loaded = nullptr;
inline PFun_slGetFeatureVersion* real_sl_get_feature_version = nullptr;
inline PFun_slAllocateResources* real_sl_allocate_resources = nullptr;
inline PFun_slFreeResources* real_sl_free_resources = nullptr;

// ---------------------------------------------------------------------------
// probes — run at a game Streamline call boundary (same thread the game uses
// for SL), never while holding the diagnostics lock.
// ---------------------------------------------------------------------------

inline void RunProbe(int scope) {
  static const uint32_t kAllFeatures[] = {
      sl::kFeatureDLSS, sl::kFeatureNIS, sl::kFeatureReflex, sl::kFeaturePCL,
      sl::kFeatureDeepDVC, sl::kFeatureLatewarp, sl::kFeatureDLSS_G,
      sl::kFeatureDLSS_RR, sl::kFeatureNvPerf, sl::kFeatureDirectSR,
  };
  static const uint32_t kRrFeature[] = {sl::kFeatureDLSS_RR};

  std::array<uint8_t, 8> luid{};
  bool luid_valid = false;
  {
    const std::lock_guard lock(diagnostics_mutex);
    luid = diagnostics.luid;
    luid_valid = diagnostics.luid_valid;
  }

  sl::AdapterInfo adapter{};
  if (luid_valid) {
    adapter.deviceLUID = luid.data();
    adapter.deviceLUIDSizeInBytes = static_cast<uint32_t>(luid.size());
  }

  const uint32_t* list = kRrFeature;
  size_t count = 1;
  if (scope == 1) {
    list = kAllFeatures;
    count = sizeof(kAllFeatures) / sizeof(kAllFeatures[0]);
  }

  struct ProbeResult {
    uint32_t feature = 0;
    bool has_supported = false;
    int supported = 0;
    bool has_loaded = false;
    bool loaded = false;
    int loaded_result = 0;
    bool has_version = false;
    int version_result = 0;
    uint32_t sl_major = 0, sl_minor = 0, sl_build = 0;
    uint32_t ngx_major = 0, ngx_minor = 0, ngx_build = 0;
  };

  std::vector<ProbeResult> results;
  results.reserve(count);
  for (size_t i = 0; i < count; ++i) {
    const uint32_t feature = list[i];
    ProbeResult r{};
    r.feature = feature;
    if (real_sl_is_feature_supported != nullptr) {
      r.has_supported = true;
      r.supported = static_cast<int>(real_sl_is_feature_supported(feature, adapter));
    }
    if (real_sl_is_feature_loaded != nullptr) {
      bool loaded = false;
      r.loaded_result = static_cast<int>(real_sl_is_feature_loaded(feature, loaded));
      r.has_loaded = true;
      r.loaded = loaded;
    }
    if (real_sl_get_feature_version != nullptr) {
      sl::FeatureVersion version{};
      r.version_result = static_cast<int>(real_sl_get_feature_version(feature, version));
      r.has_version = true;
      if (static_cast<sl::Result>(r.version_result) == sl::Result::eOk) {
        r.sl_major = version.versionSL.major;
        r.sl_minor = version.versionSL.minor;
        r.sl_build = version.versionSL.build;
        r.ngx_major = version.versionNGX.major;
        r.ngx_minor = version.versionNGX.minor;
        r.ngx_build = version.versionNGX.build;
      }
    }
    results.push_back(r);
  }

  const std::lock_guard lock(diagnostics_mutex);
  for (const auto& r : results) {
    auto& row = FeatureForLocked(r.feature);
    row.probed = true;
    if (r.has_supported) row.supported_result = r.supported;
    if (r.has_loaded) {
      row.loaded_result = r.loaded_result;
      row.loaded = r.loaded;
    }
    if (r.has_version) {
      row.version_result = r.version_result;
      if (static_cast<sl::Result>(r.version_result) == sl::Result::eOk) {
        row.sl_major = r.sl_major;
        row.sl_minor = r.sl_minor;
        row.sl_build = r.sl_build;
        row.ngx_major = r.ngx_major;
        row.ngx_minor = r.ngx_minor;
        row.ngx_build = r.ngx_build;
      }
    }
  }
}

// Called at the top of hooks that the game invokes every frame, before
// forwarding — the probe then runs between the game's own SL calls, on the
// same thread.
inline void ServiceProbeIfPending() {
  int scope = 0;
  {
    const std::lock_guard lock(diagnostics_mutex);
    if (!diagnostics.probe_pending) return;
    diagnostics.probe_pending = false;
    scope = diagnostics.probe_scope;
  }
  RunProbe(scope);
}

inline void RequestProbe(int scope) {
  const std::lock_guard lock(diagnostics_mutex);
  diagnostics.probe_pending = true;
  diagnostics.probe_scope = scope;
}

// Clears captured activity but keeps module state, the slInit snapshot and
// the device/LUID (one-shot observations that never repeat).
inline void ResetCapture() {
  const std::lock_guard lock(diagnostics_mutex);
  for (auto& hook : diagnostics.hooks) {
    hook.calls = 0;
    hook.last_caller = nullptr;
  }
  diagnostics.frame_tokens = 0;
  diagnostics.last_frame_index = 0;
  diagnostics.tag_calls = 0;
  diagnostics.tag_for_frame_calls = 0;
  diagnostics.tags.clear();
  diagnostics.constants_seen = false;
  diagnostics.constants_calls = 0;
  diagnostics.constants_last_frame = 0;
  diagnostics.constants_last_viewport = 0xFFFFFFFF;
  diagnostics.features.clear();
  diagnostics.evaluates_total = 0;
  diagnostics.evaluate_recent.clear();
  diagnostics.feature_functions.clear();
  diagnostics.probe_pending = false;
  diagnostics.auto_probe_done = false;
  diagnostics.shutdown_seen = false;
}

// ---------------------------------------------------------------------------
// hooks — observe and forward, nothing else
// ---------------------------------------------------------------------------

inline sl::Result HookedSlInit(const sl::Preferences& pref, uint64_t sdk_version) {
  const auto result = real_sl_init(pref, sdk_version);
  const std::lock_guard lock(diagnostics_mutex);
  NoteCallLocked(kHookInit, _ReturnAddress());
  diagnostics.init_seen = true;
  diagnostics.init_result = static_cast<int>(result);
  diagnostics.init_sdk_version = sdk_version;
  diagnostics.init_flags = static_cast<uint64_t>(pref.flags);
  diagnostics.init_log_level = static_cast<int>(pref.logLevel);
  diagnostics.init_engine = static_cast<int>(pref.engine);
  diagnostics.init_app_id = pref.applicationId;
  diagnostics.init_render_api = static_cast<int>(pref.renderAPI);
  diagnostics.init_feature_count = pref.numFeaturesToLoad;
  if (pref.featuresToLoad != nullptr) {
    for (size_t i = 0; i < diagnostics.init_features.size() && i < pref.numFeaturesToLoad; ++i) {
      diagnostics.init_features[i] = pref.featuresToLoad[i];
    }
  }
  return result;
}

inline sl::Result HookedSlShutdown() {
  const auto result = real_sl_shutdown();
  const std::lock_guard lock(diagnostics_mutex);
  NoteCallLocked(kHookShutdown, _ReturnAddress());
  diagnostics.shutdown_seen = true;
  return result;
}

inline sl::Result HookedSlSetD3DDevice(void* d3d_device) {
  std::array<uint8_t, 8> luid{};
  const bool luid_valid = TryGetAdapterLuid(d3d_device, luid);
  const auto result = real_sl_set_d3d_device(d3d_device);
  const std::lock_guard lock(diagnostics_mutex);
  NoteCallLocked(kHookSetD3DDevice, _ReturnAddress());
  diagnostics.device_seen = d3d_device != nullptr;
  diagnostics.device = reinterpret_cast<uint64_t>(d3d_device);
  if (luid_valid) {
    diagnostics.luid = luid;
    diagnostics.luid_valid = true;
  }
  return result;
}

inline sl::Result HookedSlGetNewFrameToken(sl::FrameToken*& token, const uint32_t* frame_index) {
  const auto result = real_sl_get_new_frame_token(token, frame_index);
  const std::lock_guard lock(diagnostics_mutex);
  NoteCallLocked(kHookGetNewFrameToken, _ReturnAddress());
  ++diagnostics.frame_tokens;
  if (result == sl::Result::eOk && token != nullptr) {
    diagnostics.last_frame_index = static_cast<uint32_t>(*token);
  }
  return result;
}

inline sl::Result HookedSlSetTag(
    const sl::ViewportHandle& viewport, const sl::ResourceTag* tags, uint32_t num_tags,
    sl::CommandBuffer* cmd) {
  ServiceProbeIfPending();
  const auto result = real_sl_set_tag(viewport, tags, num_tags, cmd);
  const std::lock_guard lock(diagnostics_mutex);
  NoteCallLocked(kHookSetTag, _ReturnAddress());
  ++diagnostics.tag_calls;
  RecordTagListLocked(viewport, tags, num_tags, diagnostics.last_frame_index, false);
  return result;
}

inline sl::Result HookedSlSetTagForFrame(
    const sl::FrameToken& frame, const sl::ViewportHandle& viewport, const sl::ResourceTag* tags,
    uint32_t num_tags, sl::CommandBuffer* cmd) {
  ServiceProbeIfPending();
  const auto result = real_sl_set_tag_for_frame(frame, viewport, tags, num_tags, cmd);
  const uint32_t frame_index = static_cast<uint32_t>(frame);
  const std::lock_guard lock(diagnostics_mutex);
  NoteCallLocked(kHookSetTagForFrame, _ReturnAddress());
  ++diagnostics.tag_for_frame_calls;
  diagnostics.last_frame_index = frame_index;
  RecordTagListLocked(viewport, tags, num_tags, frame_index, true);
  return result;
}

inline sl::Result HookedSlSetConstants(
    const sl::Constants& values, const sl::FrameToken& frame, const sl::ViewportHandle& viewport) {
  ServiceProbeIfPending();
  const auto result = real_sl_set_constants(values, frame, viewport);
  const uint32_t frame_index = static_cast<uint32_t>(frame);
  const std::lock_guard lock(diagnostics_mutex);
  NoteCallLocked(kHookSetConstants, _ReturnAddress());
  diagnostics.constants_seen = true;
  ++diagnostics.constants_calls;
  diagnostics.constants_last_frame = frame_index;
  diagnostics.constants_last_viewport = static_cast<uint32_t>(viewport);
  diagnostics.constants = values;
  return result;
}

inline sl::Result HookedSlEvaluateFeature(
    sl::Feature feature, const sl::FrameToken& frame, const sl::BaseStructure** inputs,
    uint32_t num_inputs, sl::CommandBuffer* cmd) {
  ServiceProbeIfPending();
  const auto result = real_sl_evaluate_feature(feature, frame, inputs, num_inputs, cmd);
  void* const caller = _ReturnAddress();
  const uint32_t frame_index = static_cast<uint32_t>(frame);
  const std::lock_guard lock(diagnostics_mutex);
  NoteCallLocked(kHookEvaluateFeature, caller);
  ++diagnostics.evaluates_total;
  auto& row = FeatureForLocked(feature);
  ++row.evaluates;
  row.last_eval_frame = frame_index;
  row.last_eval_result = static_cast<int>(result);
  row.last_eval_inputs = num_inputs;
  row.last_eval_caller = caller;
  row.last_eval_input_type = 0;
  row.last_eval_input_version = 0;
  if (inputs != nullptr && num_inputs > 0 && inputs[0] != nullptr) {
    row.last_eval_input_type = inputs[0]->structType.data1;
    row.last_eval_input_version = static_cast<uint32_t>(inputs[0]->structVersion);
  }
  EvaluateCall call{};
  call.frame = frame_index;
  call.feature = feature;
  call.num_inputs = num_inputs;
  call.primary_input_type = row.last_eval_input_type;
  call.result = static_cast<int>(result);
  call.caller = caller;
  diagnostics.evaluate_recent.insert(diagnostics.evaluate_recent.begin(), call);
  while (diagnostics.evaluate_recent.size() > 12) {
    diagnostics.evaluate_recent.pop_back();
  }
  // One-shot automatic probe once the game is clearly running DLSS.
  if (!diagnostics.auto_probe_done) {
    diagnostics.auto_probe_done = true;
    diagnostics.probe_pending = true;
    diagnostics.probe_scope = 0;
  }
  return result;
}

inline sl::Result HookedSlGetFeatureFunction(
    sl::Feature feature, const char* function_name, void*& function) {
  const auto result = real_sl_get_feature_function(feature, function_name, function);
  const std::lock_guard lock(diagnostics_mutex);
  NoteCallLocked(kHookGetFeatureFunction, _ReturnAddress());
  const std::string name = function_name != nullptr ? function_name : "?";
  for (auto& row : diagnostics.feature_functions) {
    if (row.feature == feature && row.name == name) {
      ++row.calls;
      row.last_result = static_cast<int>(result);
      row.last_frame = diagnostics.last_frame_index;
      return result;
    }
  }
  FeatureFunctionRow row{};
  row.feature = feature;
  row.name = name;
  row.calls = 1;
  row.last_result = static_cast<int>(result);
  row.last_frame = diagnostics.last_frame_index;
  diagnostics.feature_functions.push_back(std::move(row));
  return result;
}

inline sl::Result HookedSlIsFeatureSupported(
    sl::Feature feature, const sl::AdapterInfo& adapter_info) {
  const auto result = real_sl_is_feature_supported(feature, adapter_info);
  const std::lock_guard lock(diagnostics_mutex);
  NoteCallLocked(kHookIsFeatureSupported, _ReturnAddress());
  auto& row = FeatureForLocked(feature);
  ++row.queries;
  row.supported_result = static_cast<int>(result);
  return result;
}

inline sl::Result HookedSlIsFeatureLoaded(sl::Feature feature, bool& loaded) {
  const auto result = real_sl_is_feature_loaded(feature, loaded);
  const std::lock_guard lock(diagnostics_mutex);
  NoteCallLocked(kHookIsFeatureLoaded, _ReturnAddress());
  auto& row = FeatureForLocked(feature);
  ++row.queries;
  row.loaded_result = static_cast<int>(result);
  row.loaded = loaded;
  return result;
}

inline sl::Result HookedSlSetFeatureLoaded(sl::Feature feature, bool loaded) {
  const auto result = real_sl_set_feature_loaded(feature, loaded);
  const std::lock_guard lock(diagnostics_mutex);
  NoteCallLocked(kHookSetFeatureLoaded, _ReturnAddress());
  auto& row = FeatureForLocked(feature);
  row.set_loaded_value = loaded ? 1 : -1;
  return result;
}

inline sl::Result HookedSlGetFeatureVersion(sl::Feature feature, sl::FeatureVersion& version) {
  const auto result = real_sl_get_feature_version(feature, version);
  const std::lock_guard lock(diagnostics_mutex);
  NoteCallLocked(kHookGetFeatureVersion, _ReturnAddress());
  auto& row = FeatureForLocked(feature);
  ++row.queries;
  row.version_result = static_cast<int>(result);
  if (result == sl::Result::eOk) {
    row.sl_major = version.versionSL.major;
    row.sl_minor = version.versionSL.minor;
    row.sl_build = version.versionSL.build;
    row.ngx_major = version.versionNGX.major;
    row.ngx_minor = version.versionNGX.minor;
    row.ngx_build = version.versionNGX.build;
  }
  return result;
}

inline sl::Result HookedSlAllocateResources(
    sl::CommandBuffer* cmd, sl::Feature feature, const sl::ViewportHandle& viewport) {
  const auto result = real_sl_allocate_resources(cmd, feature, viewport);
  const std::lock_guard lock(diagnostics_mutex);
  NoteCallLocked(kHookAllocateResources, _ReturnAddress());
  return result;
}

inline sl::Result HookedSlFreeResources(sl::Feature feature, const sl::ViewportHandle& viewport) {
  const auto result = real_sl_free_resources(feature, viewport);
  const std::lock_guard lock(diagnostics_mutex);
  NoteCallLocked(kHookFreeResources, _ReturnAddress());
  return result;
}

// ---------------------------------------------------------------------------
// hook installation
// ---------------------------------------------------------------------------

inline std::mutex detour_mutex;
inline std::atomic<bool> armed = false;

inline bool ArmStreamlineHooks(HMODULE interposer) {
  if (armed.load()) return true;
  if (interposer == nullptr) return false;
  const std::lock_guard install_lock(detour_mutex);
  if (armed.load()) return true;

  auto* p_init = reinterpret_cast<PFun_slInit*>(GetProcAddress(interposer, "slInit"));
  auto* p_shutdown = reinterpret_cast<PFun_slShutdown*>(GetProcAddress(interposer, "slShutdown"));
  auto* p_set_device =
      reinterpret_cast<PFun_slSetD3DDevice*>(GetProcAddress(interposer, "slSetD3DDevice"));
  auto* p_frame_token = reinterpret_cast<PFun_slGetNewFrameToken*>(
      GetProcAddress(interposer, "slGetNewFrameToken"));
  auto* p_set_tag = reinterpret_cast<PFun_slSetTag*>(GetProcAddress(interposer, "slSetTag"));
  auto* p_set_tag_frame = reinterpret_cast<PFun_slSetTagForFrame*>(
      GetProcAddress(interposer, "slSetTagForFrame"));
  auto* p_constants =
      reinterpret_cast<PFun_slSetConstants*>(GetProcAddress(interposer, "slSetConstants"));
  auto* p_evaluate =
      reinterpret_cast<PFun_slEvaluateFeature*>(GetProcAddress(interposer, "slEvaluateFeature"));
  auto* p_get_function = reinterpret_cast<PFun_slGetFeatureFunction*>(
      GetProcAddress(interposer, "slGetFeatureFunction"));
  auto* p_supported = reinterpret_cast<PFun_slIsFeatureSupported*>(
      GetProcAddress(interposer, "slIsFeatureSupported"));
  auto* p_loaded =
      reinterpret_cast<PFun_slIsFeatureLoaded*>(GetProcAddress(interposer, "slIsFeatureLoaded"));
  auto* p_set_loaded =
      reinterpret_cast<PFun_slSetFeatureLoaded*>(GetProcAddress(interposer, "slSetFeatureLoaded"));
  auto* p_version = reinterpret_cast<PFun_slGetFeatureVersion*>(
      GetProcAddress(interposer, "slGetFeatureVersion"));
  auto* p_allocate = reinterpret_cast<PFun_slAllocateResources*>(
      GetProcAddress(interposer, "slAllocateResources"));
  auto* p_free =
      reinterpret_cast<PFun_slFreeResources*>(GetProcAddress(interposer, "slFreeResources"));

  if (p_init == nullptr || p_evaluate == nullptr || p_constants == nullptr
      || p_frame_token == nullptr || p_set_tag_frame == nullptr) {
    LogWarn("sl.interposer.dll is missing core entry points — hooks not installed");
    return false;
  }

  real_sl_init = p_init;
  real_sl_shutdown = p_shutdown;
  real_sl_set_d3d_device = p_set_device;
  real_sl_get_new_frame_token = p_frame_token;
  real_sl_set_tag = p_set_tag;
  real_sl_set_tag_for_frame = p_set_tag_frame;
  real_sl_set_constants = p_constants;
  real_sl_evaluate_feature = p_evaluate;
  real_sl_get_feature_function = p_get_function;
  real_sl_is_feature_supported = p_supported;
  real_sl_is_feature_loaded = p_loaded;
  real_sl_set_feature_loaded = p_set_loaded;
  real_sl_get_feature_version = p_version;
  real_sl_allocate_resources = p_allocate;
  real_sl_free_resources = p_free;

  bool present[kHookCount] = {
      p_init != nullptr,
      p_shutdown != nullptr,
      p_set_device != nullptr,
      p_frame_token != nullptr,
      p_set_tag != nullptr,
      p_set_tag_frame != nullptr,
      p_constants != nullptr,
      p_evaluate != nullptr,
      p_get_function != nullptr,
      p_supported != nullptr,
      p_loaded != nullptr,
      p_set_loaded != nullptr,
      p_version != nullptr,
      p_allocate != nullptr,
      p_free != nullptr,
  };

  LONG error = DetourTransactionBegin();
  const bool transaction_started = error == NO_ERROR;
  if (error == NO_ERROR) error = DetourUpdateThread(GetCurrentThread());
  if (error == NO_ERROR && present[kHookInit]) {
    error = DetourAttach(reinterpret_cast<void**>(&real_sl_init), HookedSlInit);
  }
  if (error == NO_ERROR && present[kHookShutdown]) {
    error = DetourAttach(reinterpret_cast<void**>(&real_sl_shutdown), HookedSlShutdown);
  }
  if (error == NO_ERROR && present[kHookSetD3DDevice]) {
    error = DetourAttach(reinterpret_cast<void**>(&real_sl_set_d3d_device), HookedSlSetD3DDevice);
  }
  if (error == NO_ERROR && present[kHookGetNewFrameToken]) {
    error = DetourAttach(
        reinterpret_cast<void**>(&real_sl_get_new_frame_token), HookedSlGetNewFrameToken);
  }
  if (error == NO_ERROR && present[kHookSetTag]) {
    error = DetourAttach(reinterpret_cast<void**>(&real_sl_set_tag), HookedSlSetTag);
  }
  if (error == NO_ERROR && present[kHookSetTagForFrame]) {
    error = DetourAttach(
        reinterpret_cast<void**>(&real_sl_set_tag_for_frame), HookedSlSetTagForFrame);
  }
  if (error == NO_ERROR && present[kHookSetConstants]) {
    error = DetourAttach(reinterpret_cast<void**>(&real_sl_set_constants), HookedSlSetConstants);
  }
  if (error == NO_ERROR && present[kHookEvaluateFeature]) {
    error = DetourAttach(
        reinterpret_cast<void**>(&real_sl_evaluate_feature), HookedSlEvaluateFeature);
  }
  if (error == NO_ERROR && present[kHookGetFeatureFunction]) {
    error = DetourAttach(
        reinterpret_cast<void**>(&real_sl_get_feature_function), HookedSlGetFeatureFunction);
  }
  if (error == NO_ERROR && present[kHookIsFeatureSupported]) {
    error = DetourAttach(
        reinterpret_cast<void**>(&real_sl_is_feature_supported), HookedSlIsFeatureSupported);
  }
  if (error == NO_ERROR && present[kHookIsFeatureLoaded]) {
    error = DetourAttach(
        reinterpret_cast<void**>(&real_sl_is_feature_loaded), HookedSlIsFeatureLoaded);
  }
  if (error == NO_ERROR && present[kHookSetFeatureLoaded]) {
    error = DetourAttach(
        reinterpret_cast<void**>(&real_sl_set_feature_loaded), HookedSlSetFeatureLoaded);
  }
  if (error == NO_ERROR && present[kHookGetFeatureVersion]) {
    error = DetourAttach(
        reinterpret_cast<void**>(&real_sl_get_feature_version), HookedSlGetFeatureVersion);
  }
  if (error == NO_ERROR && present[kHookAllocateResources]) {
    error = DetourAttach(
        reinterpret_cast<void**>(&real_sl_allocate_resources), HookedSlAllocateResources);
  }
  if (error == NO_ERROR && present[kHookFreeResources]) {
    error = DetourAttach(reinterpret_cast<void**>(&real_sl_free_resources), HookedSlFreeResources);
  }

  if (error != NO_ERROR) {
    if (transaction_started) DetourTransactionAbort();
  } else {
    error = DetourTransactionCommit();
  }
  if (error != NO_ERROR) {
    LogWarn("Detour commit FAILED for Streamline entry points");
    real_sl_init = nullptr;
    real_sl_shutdown = nullptr;
    real_sl_set_d3d_device = nullptr;
    real_sl_get_new_frame_token = nullptr;
    real_sl_set_tag = nullptr;
    real_sl_set_tag_for_frame = nullptr;
    real_sl_set_constants = nullptr;
    real_sl_evaluate_feature = nullptr;
    real_sl_get_feature_function = nullptr;
    real_sl_is_feature_supported = nullptr;
    real_sl_is_feature_loaded = nullptr;
    real_sl_set_feature_loaded = nullptr;
    real_sl_get_feature_version = nullptr;
    real_sl_allocate_resources = nullptr;
    real_sl_free_resources = nullptr;
    return false;
  }

  char module_path[MAX_PATH] = {};
  GetModuleFileNameA(interposer, module_path, MAX_PATH);

  int installed = 0;
  {
    const std::lock_guard lock(diagnostics_mutex);
    diagnostics.module_found = true;
    diagnostics.module_path = module_path;
    diagnostics.armed = true;
    for (int i = 0; i < kHookCount; ++i) {
      diagnostics.hooks[static_cast<size_t>(i)].installed = present[i];
      if (present[i]) ++installed;
    }
    diagnostics.hooks_installed = installed;
  }
  armed.store(true);

  std::stringstream s;
  s << "Streamline entry points hooked (" << installed << "/" << kHookCount << ") in "
    << ModuleNameOf(interposer);
  LogInfo(s.str());
  return true;
}

inline bool TryArmFromLoadedModules() {
  if (armed.load()) return true;
  HMODULE interposer = GetModuleHandleW(L"sl.interposer.dll");
  if (interposer == nullptr) return false;
  return ArmStreamlineHooks(interposer);
}

// Present-time retry: cheap, and only logs when arming actually happens.
inline void Poll() {
  if (!armed.load()) {
    TryArmFromLoadedModules();
  }
}

// ---------------------------------------------------------------------------
// loader hooks — sl.interposer.dll may be imported statically (present before
// this addon loads) or loaded dynamically; watch for both.
// ---------------------------------------------------------------------------

inline decltype(&LoadLibraryW) real_load_library_w = nullptr;
inline decltype(&LoadLibraryExW) real_load_library_ex_w = nullptr;
inline decltype(&LoadLibraryA) real_load_library_a = nullptr;
inline decltype(&LoadLibraryExA) real_load_library_ex_a = nullptr;
inline std::atomic<bool> loader_hooks_installed = false;

inline void MaybeArmFromPath(const wchar_t* path, HMODULE loaded) {
  if (armed.load() || path == nullptr || loaded == nullptr) return;
  const wchar_t* name = wcsrchr(path, L'\\');
  name = (name != nullptr) ? name + 1 : path;
  if (_wcsicmp(name, L"sl.interposer.dll") == 0) {
    ArmStreamlineHooks(loaded);
  }
}

inline void MaybeArmFromPathA(const char* path, HMODULE loaded) {
  if (armed.load() || path == nullptr || loaded == nullptr) return;
  const char* name = strrchr(path, '\\');
  name = (name != nullptr) ? name + 1 : path;
  if (_stricmp(name, "sl.interposer.dll") == 0) {
    ArmStreamlineHooks(loaded);
  }
}

inline HMODULE WINAPI HookedLoadLibraryW(LPCWSTR file_name) {
  HMODULE result = real_load_library_w(file_name);
  MaybeArmFromPath(file_name, result);
  return result;
}

inline HMODULE WINAPI HookedLoadLibraryExW(LPCWSTR file_name, HANDLE file, DWORD flags) {
  HMODULE result = real_load_library_ex_w(file_name, file, flags);
  MaybeArmFromPath(file_name, result);
  return result;
}

inline HMODULE WINAPI HookedLoadLibraryA(LPCSTR file_name) {
  HMODULE result = real_load_library_a(file_name);
  MaybeArmFromPathA(file_name, result);
  return result;
}

inline HMODULE WINAPI HookedLoadLibraryExA(LPCSTR file_name, HANDLE file, DWORD flags) {
  HMODULE result = real_load_library_ex_a(file_name, file, flags);
  MaybeArmFromPathA(file_name, result);
  return result;
}

inline bool IsGameProcess() {
  const auto file_name = renodx::utils::platform::GetCurrentProcessPath().filename().string();
  return _stricmp(file_name.c_str(), "forzahorizon6.exe") == 0;
}

inline void InstallLoaderHooks() {
  if (!IsGameProcess()) {
    LogInfo("not the game process — Streamline hooks not installed");
    return;
  }
  {
    const std::lock_guard install_lock(detour_mutex);
    if (!loader_hooks_installed.load()) {
      real_load_library_w = &LoadLibraryW;
      real_load_library_ex_w = &LoadLibraryExW;
      real_load_library_a = &LoadLibraryA;
      real_load_library_ex_a = &LoadLibraryExA;
      LONG error = DetourTransactionBegin();
      const bool transaction_started = error == NO_ERROR;
      if (error == NO_ERROR) error = DetourUpdateThread(GetCurrentThread());
      if (error == NO_ERROR) {
        error = DetourAttach(reinterpret_cast<void**>(&real_load_library_w), HookedLoadLibraryW);
      }
      if (error == NO_ERROR) {
        error = DetourAttach(reinterpret_cast<void**>(&real_load_library_ex_w), HookedLoadLibraryExW);
      }
      if (error == NO_ERROR) {
        error = DetourAttach(reinterpret_cast<void**>(&real_load_library_a), HookedLoadLibraryA);
      }
      if (error == NO_ERROR) {
        error = DetourAttach(reinterpret_cast<void**>(&real_load_library_ex_a), HookedLoadLibraryExA);
      }
      if (error != NO_ERROR) {
        if (transaction_started) DetourTransactionAbort();
      } else {
        error = DetourTransactionCommit();
      }
      if (error != NO_ERROR) {
        LogWarn("LoadLibrary detours FAILED — will poll for sl.interposer.dll instead");
      } else {
        loader_hooks_installed = true;
        LogInfo("LoadLibrary hooks installed (watching for sl.interposer.dll)");
      }
    }
  }
  TryArmFromLoadedModules();
}

inline void UninstallHooks() {
  const std::lock_guard install_lock(detour_mutex);
  const bool had_loader = loader_hooks_installed.load();
  const bool had_streamline = armed.load();
  if (!had_loader && !had_streamline) return;
  LONG error = DetourTransactionBegin();
  const bool transaction_started = error == NO_ERROR;
  if (error == NO_ERROR) error = DetourUpdateThread(GetCurrentThread());
  if (had_loader) {
    if (error == NO_ERROR) {
      error = DetourDetach(reinterpret_cast<void**>(&real_load_library_w), HookedLoadLibraryW);
    }
    if (error == NO_ERROR) {
      error = DetourDetach(reinterpret_cast<void**>(&real_load_library_ex_w), HookedLoadLibraryExW);
    }
    if (error == NO_ERROR) {
      error = DetourDetach(reinterpret_cast<void**>(&real_load_library_a), HookedLoadLibraryA);
    }
    if (error == NO_ERROR) {
      error = DetourDetach(reinterpret_cast<void**>(&real_load_library_ex_a), HookedLoadLibraryExA);
    }
  }
  if (had_streamline) {
    if (error == NO_ERROR && real_sl_init != nullptr) {
      error = DetourDetach(reinterpret_cast<void**>(&real_sl_init), HookedSlInit);
    }
    if (error == NO_ERROR && real_sl_shutdown != nullptr) {
      error = DetourDetach(reinterpret_cast<void**>(&real_sl_shutdown), HookedSlShutdown);
    }
    if (error == NO_ERROR && real_sl_set_d3d_device != nullptr) {
      error = DetourDetach(reinterpret_cast<void**>(&real_sl_set_d3d_device), HookedSlSetD3DDevice);
    }
    if (error == NO_ERROR && real_sl_get_new_frame_token != nullptr) {
      error =
          DetourDetach(reinterpret_cast<void**>(&real_sl_get_new_frame_token), HookedSlGetNewFrameToken);
    }
    if (error == NO_ERROR && real_sl_set_tag != nullptr) {
      error = DetourDetach(reinterpret_cast<void**>(&real_sl_set_tag), HookedSlSetTag);
    }
    if (error == NO_ERROR && real_sl_set_tag_for_frame != nullptr) {
      error =
          DetourDetach(reinterpret_cast<void**>(&real_sl_set_tag_for_frame), HookedSlSetTagForFrame);
    }
    if (error == NO_ERROR && real_sl_set_constants != nullptr) {
      error = DetourDetach(reinterpret_cast<void**>(&real_sl_set_constants), HookedSlSetConstants);
    }
    if (error == NO_ERROR && real_sl_evaluate_feature != nullptr) {
      error = DetourDetach(
          reinterpret_cast<void**>(&real_sl_evaluate_feature), HookedSlEvaluateFeature);
    }
    if (error == NO_ERROR && real_sl_get_feature_function != nullptr) {
      error = DetourDetach(
          reinterpret_cast<void**>(&real_sl_get_feature_function), HookedSlGetFeatureFunction);
    }
    if (error == NO_ERROR && real_sl_is_feature_supported != nullptr) {
      error = DetourDetach(
          reinterpret_cast<void**>(&real_sl_is_feature_supported), HookedSlIsFeatureSupported);
    }
    if (error == NO_ERROR && real_sl_is_feature_loaded != nullptr) {
      error = DetourDetach(
          reinterpret_cast<void**>(&real_sl_is_feature_loaded), HookedSlIsFeatureLoaded);
    }
    if (error == NO_ERROR && real_sl_set_feature_loaded != nullptr) {
      error = DetourDetach(
          reinterpret_cast<void**>(&real_sl_set_feature_loaded), HookedSlSetFeatureLoaded);
    }
    if (error == NO_ERROR && real_sl_get_feature_version != nullptr) {
      error = DetourDetach(
          reinterpret_cast<void**>(&real_sl_get_feature_version), HookedSlGetFeatureVersion);
    }
    if (error == NO_ERROR && real_sl_allocate_resources != nullptr) {
      error = DetourDetach(
          reinterpret_cast<void**>(&real_sl_allocate_resources), HookedSlAllocateResources);
    }
    if (error == NO_ERROR && real_sl_free_resources != nullptr) {
      error =
          DetourDetach(reinterpret_cast<void**>(&real_sl_free_resources), HookedSlFreeResources);
    }
  }
  if (error != NO_ERROR) {
    if (transaction_started) DetourTransactionAbort();
    LogWarn("hook detach transaction failed");
  } else {
    error = DetourTransactionCommit();
    if (error != NO_ERROR) {
      LogWarn("hook detach commit failed");
    } else {
      if (had_loader) loader_hooks_installed.store(false);
      if (had_streamline) armed.store(false);
    }
  }
}

// ---------------------------------------------------------------------------
// report — the same data the overlay shows, as text (clipboard / log button)
// ---------------------------------------------------------------------------

inline std::string BuildReport() {
  const Diagnostics d = CaptureDiagnostics();
  std::stringstream s;
  s.precision(6);

  s << "== forzahorizon6-rr — Streamline diagnostics ==\n";
  s << "build: " << renodx::utils::date::ISO_DATE_TIME << "\n";

  s << "\n[module]\n";
  s << "sl.interposer.dll: " << (d.module_found ? "found" : "NOT FOUND") << "\n";
  if (d.module_found) {
    s << "path: " << d.module_path << "\n";
    s << "hooks: " << d.hooks_installed << "/" << kHookCount << " (" << (d.armed ? "armed" : "not armed") << ")\n";
  }

  s << "\n[slInit]\n";
  if (!d.init_seen) {
    s << "not seen\n";
  } else {
    s << "result: " << ResultName(d.init_result) << "\n";
    s << "sdkVersion: 0x" << std::hex << d.init_sdk_version << std::dec << " (" << SdkVersionLabel(d.init_sdk_version) << ")\n";
    s << "flags: 0x" << std::hex << d.init_flags << std::dec << "\n";
    s << "logLevel: " << d.init_log_level << "  engine: " << d.init_engine
      << "  appId: " << d.init_app_id << "  renderAPI: " << d.init_render_api << "\n";
    s << "featuresToLoad:";
    for (uint32_t i = 0; i < d.init_feature_count && i < d.init_features.size(); ++i) {
      s << " " << FeatureName(d.init_features[i]);
    }
    s << "\n";
  }
  s << "shutdown seen: " << (d.shutdown_seen ? "yes" : "no") << "\n";

  s << "\n[device/frames]\n";
  s << "device: " << (d.device_seen ? "set" : "not set") << " 0x" << std::hex << d.device << std::dec;
  if (d.luid_valid) {
    s << "  luid=";
    for (size_t i = 0; i < d.luid.size(); ++i) {
      s << std::hex << static_cast<unsigned>(d.luid[i]) << (i + 1 < d.luid.size() ? ":" : "");
    }
    s << std::dec;
  }
  s << "\n";
  s << "frame tokens: " << d.frame_tokens << "  last frame: " << d.last_frame_index << "\n";

  s << "\n[tags] setTag calls: " << d.tag_calls << "  setTagForFrame calls: " << d.tag_for_frame_calls << "\n";
  for (const auto& tag : d.tags) {
    s << "  - " << BufferName(tag.type) << " via " << (tag.via_frame_api ? "slSetTagForFrame" : "slSetTag")
      << " calls=" << tag.calls;
    if (tag.cleared) {
      s << " [cleared]";
    } else {
      s << " res=0x" << std::hex << reinterpret_cast<uint64_t>(tag.native) << std::dec;
      if (tag.desc_ok) {
        s << " fmt=" << FormatLabel(tag.desc_format) << "(" << tag.desc_format << ") " << tag.desc_width << "x"
          << tag.desc_height;
        if (tag.desc_layers > 1) s << "x" << tag.desc_layers;
      } else if (tag.sl_format != 0) {
        s << " fmt=" << FormatLabel(tag.sl_format) << "(" << tag.sl_format << ")";
      }
      s << " state=0x" << std::hex << tag.sl_state << std::dec;
      s << " lifecycle=" << LifecycleName(tag.lifecycle);
      if (tag.extent_w != 0 || tag.extent_h != 0) {
        s << " extent=" << tag.extent_w << "x" << tag.extent_h;
      }
    }
    s << " frame=" << tag.last_frame << "\n";
  }

  s << "\n[constants] calls: " << d.constants_calls << " frame: " << d.constants_last_frame
    << " viewport: " << d.constants_last_viewport << "\n";
  if (d.constants_seen) {
    const auto& c = d.constants;
    s << "  jitter=(" << c.jitterOffset.x << ", " << c.jitterOffset.y << ")"
      << " mvecScale=(" << c.mvecScale.x << ", " << c.mvecScale.y << ")\n";
    s << "  near=" << c.cameraNear << " far=" << c.cameraFar << " fov=" << c.cameraFOV
      << " aspect=" << c.cameraAspectRatio << "\n";
    s << "  camPos=(" << c.cameraPos.x << ", " << c.cameraPos.y << ", " << c.cameraPos.z << ")\n";
    s << "  flags: depthInverted=" << BooleanName(c.depthInverted)
      << " cameraMotionIncluded=" << BooleanName(c.cameraMotionIncluded)
      << " motionVectors3D=" << BooleanName(c.motionVectors3D)
      << " reset=" << BooleanName(c.reset)
      << " jittered=" << BooleanName(c.motionVectorsJittered)
      << " dilated=" << BooleanName(c.motionVectorsDilated)
      << " orthographic=" << BooleanName(c.orthographicProjection) << "\n";
  }

  s << "\n[evaluates] total: " << d.evaluates_total << "\n";
  for (const auto& [feature, row] : d.features) {
    if (row.evaluates == 0 && row.queries == 0 && !row.probed) continue;
    s << "  - " << FeatureName(feature) << " evals=" << row.evaluates
      << " lastResult=" << ResultName(row.last_eval_result) << " lastFrame=" << row.last_eval_frame
      << " inputs=" << row.last_eval_inputs << " caller=" << ModuleNameOf(row.last_eval_caller) << "\n";
    s << "      supported=" << ResultName(row.supported_result)
      << " loaded=" << (row.loaded_result == kNever ? "-" : (row.loaded ? "yes" : "no"))
      << " loadedResult=" << ResultName(row.loaded_result)
      << " versionResult=" << ResultName(row.version_result)
      << " SL=" << row.sl_major << "." << row.sl_minor << "." << row.sl_build
      << " NGX=" << row.ngx_major << "." << row.ngx_minor << "." << row.ngx_build
      << " probed=" << (row.probed ? "yes" : "no") << "\n";
  }
  s << "  recent:\n";
  for (const auto& call : d.evaluate_recent) {
    s << "    [frame " << call.frame << "] " << FeatureName(call.feature)
      << " inputs=" << call.num_inputs << " result=" << ResultName(call.result)
      << " caller=" << ModuleNameOf(call.caller) << "\n";
  }

  s << "\n[feature functions]\n";
  for (const auto& fn : d.feature_functions) {
    s << "  - " << FeatureName(fn.feature) << " " << fn.name << " calls=" << fn.calls
      << " last=" << ResultName(fn.last_result) << " frame=" << fn.last_frame << "\n";
  }

  s << "\n[hooks]\n";
  for (int i = 0; i < kHookCount; ++i) {
    const auto& hook = d.hooks[static_cast<size_t>(i)];
    s << "  - " << kHookNames[i] << " " << (hook.installed ? "armed" : "no")
      << " calls=" << hook.calls << " caller=" << ModuleNameOf(hook.last_caller) << "\n";
  }

  return s.str();
}

}  // namespace sl_rr
