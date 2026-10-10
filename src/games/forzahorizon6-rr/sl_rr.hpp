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
#include <cstdio>
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

inline std::string WideToNarrow(const wchar_t* text) {
  if (text == nullptr || *text == L'\0') return {};
  const int needed = WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
  if (needed <= 1) return {};
  std::vector<char> buffer(static_cast<size_t>(needed));
  WideCharToMultiByte(CP_UTF8, 0, text, -1, buffer.data(), needed, nullptr, nullptr);
  return std::string(buffer.data(), buffer.size() - 1);
}

// Base fields every slEvaluateFeature input starts with (all SL structs are
// BaseStructure-derived, so this layout is fixed).
struct EvalInputInfo {
  uint32_t data1 = 0;
  uint32_t version = 0;
};

// Struct GUID first word -> friendly name, for reporting evaluate inputs.
inline const char* StructTypeName(uint32_t data1) {
  switch (data1) {
    case 0x171b6435: return "ViewportHandle";
    case 0x1ca10965: return "Preferences";
    case 0x3a9d70cf: return "Resource";
    case 0x4c6a5aad: return "ResourceTag";
    case 0x66714097: return "FeatureRequirements";
    case 0x6ac826e4: return "DLSSOptions";
    case 0x0ad87504: return "DLSSDOptions";
    case 0x6d5b51f0: return "FeatureVersion";
    case 0x71873c14: return "DLSSDState";
    case 0x830a0f35: return "FrameToken";
    case 0x9366b056: return "DLSSState";
    case 0xdcd35ad7: return "Constants";
    case 0xef1d0957: return "DLSSOptimalSettings";
    case 0xfbd0c637: return "DLSSDOptimalSettings";
    default: return nullptr;
  }
}

inline std::string EvalInputLabel(const EvalInputInfo& input) {
  if (input.data1 == 0) return "-";
  char buffer[32] = {};
  snprintf(buffer, sizeof buffer, "0x%08X", input.data1);
  std::string out = buffer;
  const char* name = StructTypeName(input.data1);
  if (name != nullptr) out += std::string(" (") + name + ")";
  out += " v" + std::to_string(input.version);
  return out;
}

// DLSSPreset / DLSSDPreset share the same letter ordering (0 = default).
inline const char* PresetName(uint32_t preset) {
  switch (preset) {
    case 0: return "Default";
    case 1: return "A";
    case 2: return "B";
    case 3: return "C";
    case 4: return "D";
    case 5: return "E";
    case 6: return "F";
    case 7: return "G";
    case 8: return "H";
    case 9: return "I";
    case 10: return "J";
    case 11: return "K";
    case 12: return "L";
    case 13: return "M";
    case 14: return "N";
    case 15: return "O";
    case 16: return "Count";
    default: return "?";
  }
}

inline std::string RequirementFlagsText(uint32_t flags) {
  std::string out;
  const auto add = [&](uint32_t bit, const char* name) {
    if ((flags & bit) == 0) return;
    if (!out.empty()) out += "|";
    out += name;
  };
  add(1u << 0, "D3D11Supported");
  add(1u << 1, "D3D12Supported");
  add(1u << 2, "VulkanSupported");
  add(1u << 3, "VSyncOffRequired");
  add(1u << 4, "HardwareSchedulingRequired");
  if (out.empty()) out = "-";
  return out;
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
  uint32_t sl_mips = 0;
  uint32_t sl_layers = 0;
  uint32_t sl_state = 0xFFFFFFFF;
  uint32_t extent_left = 0;
  uint32_t extent_top = 0;
  uint32_t extent_w = 0;
  uint32_t extent_h = 0;
  // Native D3D12 resource description (queried from the resource itself)
  bool desc_ok = false;
  uint32_t desc_format = 0;
  uint64_t desc_width = 0;
  uint32_t desc_height = 0;
  uint32_t desc_layers = 1;
  uint32_t desc_mips = 1;
  // Set/clear history (HUD/UI/Exposure tags are set then cleared each frame)
  void* last_set_native = nullptr;
  uint32_t last_set_frame = 0;
  uint32_t cleared_frame = 0;
};

struct FeatureRow {
  uint32_t feature = 0;
  // Observed slEvaluateFeature calls
  uint64_t evaluates = 0;
  uint32_t last_eval_frame = 0;
  int last_eval_result = kNever;
  uint32_t last_eval_inputs = 0;
  std::array<EvalInputInfo, 4> last_eval_input_info{};
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
  std::array<EvalInputInfo, 4> input_info{};
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

// Options the game hands DLSS (SR), captured by forward-only wrappers so the
// report can mirror them into the DLSS-RR side.
struct DlssOptionsCapture {
  bool captured = false;
  uint32_t last_frame = 0;
  uint32_t set_options_calls = 0;
  uint32_t get_optimal_calls = 0;
  // Last slDLSSSetOptions request
  uint32_t mode = 0;  // sl::DLSSMode
  uint32_t output_width = 0;
  uint32_t output_height = 0;
  float pre_exposure = 1.f;
  float exposure_scale = 1.f;
  int color_buffers_hdr = -1;  // sl::Boolean as int, -1 = not captured
  int use_auto_exposure = -1;
  int alpha_upscaling = -1;
  std::array<uint32_t, 6> presets{};  // DLAA, Quality, Balanced, Performance, UltraPerf, UltraQuality
  // Last slDLSSGetOptimalSettings result (the game's own query)
  bool optimal_captured = false;
  int optimal_result = kNever;
  uint32_t optimal_mode = 0;
  uint32_t optimal_render_width = 0;
  uint32_t optimal_render_height = 0;
  float optimal_sharpness = 0.f;
  uint32_t render_width_min = 0;
  uint32_t render_height_min = 0;
  uint32_t render_width_max = 0;
  uint32_t render_height_max = 0;
};

// Everything the DLSS-RR probe learns beyond the generic supported/loaded/
// version answers (which live in the RR FeatureRow).
struct RrProbeState {
  bool ran = false;
  uint32_t frame = 0;
  uint32_t load_attempts = 0;
  int load_result = kNever;
  bool loaded_after_load = false;
  // Serving plugin module / game-folder file
  bool plugin_module_loaded = false;
  std::string plugin_module_path;
  bool plugin_file_present = false;
  std::string plugin_file_path;
  // slGetFeatureRequirements
  bool requirements_attempted = false;
  int requirements_result = kNever;
  uint32_t requirement_flags = 0;
  uint32_t max_cpu_threads = 0;
  uint32_t max_viewports = 0;
  std::vector<uint32_t> required_tags;
  std::string os_version_required;
  std::string driver_version_required;
  // slGetFeatureFunction availability
  int fn_set_options_result = kNever;
  bool fn_set_options_ok = false;
  int fn_get_optimal_result = kNever;
  bool fn_get_optimal_ok = false;
  int fn_get_state_result = kNever;
  bool fn_get_state_ok = false;
  // Trial calls mirroring the game's own DLSS options
  bool trial_optimal_attempted = false;
  int trial_optimal_result = kNever;
  uint32_t trial_render_width = 0;
  uint32_t trial_render_height = 0;
  float trial_sharpness = 0.f;
  uint32_t trial_render_width_min = 0;
  uint32_t trial_render_height_min = 0;
  uint32_t trial_render_width_max = 0;
  uint32_t trial_render_height_max = 0;
  bool trial_state_attempted = false;
  int trial_state_result = kNever;
  uint64_t trial_vram_bytes = 0;
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
  uint32_t init_calls = 0;
  int init_result = kNever;
  uint64_t init_sdk_version = 0;
  uint64_t init_flags = 0;
  int init_log_level = 0;
  int init_engine = 0;
  uint32_t init_app_id = 0;
  int init_render_api = 0;
  uint32_t init_pref_version = 0;
  bool init_show_console = false;
  uint32_t init_num_plugin_paths = 0;
  std::array<std::string, 4> init_plugin_paths{};
  bool init_has_log_path = false;
  std::string init_log_path;
  bool init_has_engine_version = false;
  std::string init_engine_version;
  uint32_t init_feature_count = 0;
  std::array<uint32_t, 16> init_features_original{};
  uint32_t init_effective_count = 0;
  std::array<uint32_t, 16> init_features_effective{};
  bool init_rr_injected = false;
  std::string init_injection_note;

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

  // captured DLSS (SR) options + DLSS-RR probe extras
  DlssOptionsCapture dlss_options;
  RrProbeState rr_probe;
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
  uint32_t mips = 1;
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
    out.mips = desc.MipLevels;
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
      row->cleared_frame = frame;
      continue;
    }
    row->cleared = false;
    row->native = tag.resource->native;
    row->last_set_native = tag.resource->native;
    row->last_set_frame = frame;
    row->sl_format = tag.resource->nativeFormat;
    row->sl_width = tag.resource->width;
    row->sl_height = tag.resource->height;
    row->sl_mips = tag.resource->mipLevels;
    row->sl_layers = tag.resource->arrayLayers;
    row->sl_state = tag.resource->state;
    row->extent_left = tag.extent.left;
    row->extent_top = tag.extent.top;
    row->extent_w = tag.extent.width;
    row->extent_h = tag.extent.height;
    const auto desc = QueryNativeDesc(tag.resource->native);
    row->desc_ok = desc.ok;
    row->desc_format = desc.format;
    row->desc_width = desc.width;
    row->desc_height = desc.height;
    row->desc_layers = desc.layers;
    row->desc_mips = desc.mips;
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

// Resolved but not detoured: the probe calls it to read RR's requirements.
inline PFun_slGetFeatureRequirements* real_sl_get_feature_requirements = nullptr;

// DLSS-RR feature functions resolved by the probe (kept for M2).
inline PFun_slDLSSDSetOptions* real_dlssd_set_options = nullptr;
inline PFun_slDLSSDGetOptimalSettings* real_dlssd_get_optimal_settings = nullptr;
inline PFun_slDLSSDGetState* real_dlssd_get_state = nullptr;

// slInit feature-list injection. Static storage because Streamline may keep
// the pointer past the slInit call.
constexpr uint32_t kMaxInitFeatures = 16;
inline sl::Feature init_features_patched_storage[kMaxInitFeatures + 1] = {};
inline std::atomic<bool> rr_init_injection_enabled{true};
inline std::atomic<bool> rr_runtime_load_enabled{true};

inline void SetInitInjection(bool enabled) { rr_init_injection_enabled.store(enabled); }
inline bool GetInitInjection() { return rr_init_injection_enabled.load(); }
inline void SetRuntimeLoad(bool enabled) { rr_runtime_load_enabled.store(enabled); }
inline bool GetRuntimeLoad() { return rr_runtime_load_enabled.load(); }

// ---------------------------------------------------------------------------
// probes — run at a game Streamline call boundary (same thread the game uses
// for SL), never while holding the diagnostics lock.
// ---------------------------------------------------------------------------

inline void* FetchRrFunction(const char* name, int& result_out) {
  void* fn = nullptr;
  if (real_sl_get_feature_function == nullptr) {
    result_out = kNever;
    return nullptr;
  }
  result_out = static_cast<int>(real_sl_get_feature_function(sl::kFeatureDLSS_RR, name, fn));
  return fn;
}

// Where the DLSS-RR plugin lives: the loaded module (game folder or driver
// store, whichever Streamline picked) and/or the game-folder copy on disk.
inline void ProbeRrPlugin(RrProbeState& probe) {
  HMODULE plugin = GetModuleHandleW(L"sl.dlss_d.dll");
  if (plugin != nullptr) {
    wchar_t buffer[MAX_PATH] = {};
    if (GetModuleFileNameW(plugin, buffer, MAX_PATH) != 0) {
      probe.plugin_module_path = WideToNarrow(buffer);
      probe.plugin_module_loaded = true;
    }
  }
  wchar_t executable[MAX_PATH] = {};
  if (GetModuleFileNameW(nullptr, executable, MAX_PATH) == 0) return;
  std::wstring path = executable;
  const size_t slash = path.find_last_of(L"\\/");
  if (slash == std::wstring::npos) return;
  path.resize(slash + 1);
  path += L"sl.dlss_d.dll";
  const DWORD attributes = GetFileAttributesW(path.c_str());
  if (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
    probe.plugin_file_path = WideToNarrow(path.c_str());
    probe.plugin_file_present = true;
  }
}

inline void RunProbe(int scope) {
  static const uint32_t kAllFeatures[] = {
      sl::kFeatureDLSS, sl::kFeatureNIS, sl::kFeatureReflex, sl::kFeaturePCL,
      sl::kFeatureDeepDVC, sl::kFeatureLatewarp, sl::kFeatureDLSS_G,
      sl::kFeatureDLSS_RR, sl::kFeatureNvPerf, sl::kFeatureDirectSR,
  };
  static const uint32_t kRrFeature[] = {sl::kFeatureDLSS_RR};

  std::array<uint8_t, 8> luid{};
  bool luid_valid = false;
  DlssOptionsCapture options_snapshot;
  {
    const std::lock_guard lock(diagnostics_mutex);
    luid = diagnostics.luid;
    luid_valid = diagnostics.luid_valid;
    options_snapshot = diagnostics.dlss_options;
  }

  // (a) runtime load fallback: only reaches the plugin when the slInit request
  // did not stick (the game never asks for RR itself). Runs on the game's own
  // SL thread at a call boundary.
  RrProbeState probe{};
  {
    const std::lock_guard lock(diagnostics_mutex);
    probe.load_attempts = diagnostics.rr_probe.load_attempts;
    probe.load_result = diagnostics.rr_probe.load_result;
    probe.loaded_after_load = diagnostics.rr_probe.loaded_after_load;
    probe.frame = diagnostics.last_frame_index;
  }
  if (GetRuntimeLoad() && real_sl_is_feature_loaded != nullptr
      && real_sl_set_feature_loaded != nullptr) {
    bool loaded = false;
    if (real_sl_is_feature_loaded(sl::kFeatureDLSS_RR, loaded) == sl::Result::eOk && !loaded) {
      const auto load_result = real_sl_set_feature_loaded(sl::kFeatureDLSS_RR, true);
      bool loaded_after = false;
      real_sl_is_feature_loaded(sl::kFeatureDLSS_RR, loaded_after);
      ++probe.load_attempts;
      probe.load_result = static_cast<int>(load_result);
      probe.loaded_after_load = loaded_after;
    }
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

  // (c) DLSS-RR extras: serving plugin, requirements, feature functions, and
  // trial calls mirroring the game's own DLSS options. Queries only — nothing
  // here changes rendering behavior.
  probe.ran = true;
  ProbeRrPlugin(probe);

  if (real_sl_get_feature_requirements != nullptr) {
    sl::FeatureRequirements requirements{};
    probe.requirements_attempted = true;
    probe.requirements_result =
        static_cast<int>(real_sl_get_feature_requirements(sl::kFeatureDLSS_RR, requirements));
    if (static_cast<sl::Result>(probe.requirements_result) == sl::Result::eOk) {
      probe.requirement_flags = static_cast<uint32_t>(requirements.flags);
      probe.max_cpu_threads = requirements.maxNumCPUThreads;
      probe.max_viewports = requirements.maxNumViewports;
      probe.os_version_required = requirements.osVersionRequired.toStr();
      probe.driver_version_required = requirements.driverVersionRequired.toStr();
      if (requirements.requiredTags != nullptr) {
        const uint32_t tag_count =
            requirements.numRequiredTags < 32 ? requirements.numRequiredTags : 32;
        for (uint32_t i = 0; i < tag_count; ++i) {
          probe.required_tags.push_back(static_cast<uint32_t>(requirements.requiredTags[i]));
        }
      }
    }
  }

  void* rr_function = nullptr;
  rr_function = FetchRrFunction("slDLSSDSetOptions", probe.fn_set_options_result);
  probe.fn_set_options_ok = rr_function != nullptr;
  real_dlssd_set_options = reinterpret_cast<PFun_slDLSSDSetOptions*>(rr_function);
  rr_function = FetchRrFunction("slDLSSDGetOptimalSettings", probe.fn_get_optimal_result);
  probe.fn_get_optimal_ok = rr_function != nullptr;
  real_dlssd_get_optimal_settings = reinterpret_cast<PFun_slDLSSDGetOptimalSettings*>(rr_function);
  rr_function = FetchRrFunction("slDLSSDGetState", probe.fn_get_state_result);
  probe.fn_get_state_ok = rr_function != nullptr;
  real_dlssd_get_state = reinterpret_cast<PFun_slDLSSDGetState*>(rr_function);

  if (probe.fn_get_optimal_ok && real_dlssd_get_optimal_settings != nullptr
      && options_snapshot.captured) {
    sl::DLSSDOptions options{};
    options.mode = static_cast<sl::DLSSMode>(options_snapshot.mode);
    if (options_snapshot.output_width > 0) options.outputWidth = options_snapshot.output_width;
    if (options_snapshot.output_height > 0) options.outputHeight = options_snapshot.output_height;
    if (options_snapshot.color_buffers_hdr >= 0) {
      options.colorBuffersHDR = static_cast<sl::Boolean>(options_snapshot.color_buffers_hdr);
    }
    sl::DLSSDOptimalSettings settings{};
    probe.trial_optimal_attempted = true;
    probe.trial_optimal_result =
        static_cast<int>(real_dlssd_get_optimal_settings(options, settings));
    if (static_cast<sl::Result>(probe.trial_optimal_result) == sl::Result::eOk) {
      probe.trial_render_width = settings.optimalRenderWidth;
      probe.trial_render_height = settings.optimalRenderHeight;
      probe.trial_sharpness = settings.optimalSharpness;
      probe.trial_render_width_min = settings.renderWidthMin;
      probe.trial_render_height_min = settings.renderHeightMin;
      probe.trial_render_width_max = settings.renderWidthMax;
      probe.trial_render_height_max = settings.renderHeightMax;
    }
  }

  if (probe.fn_get_state_ok && real_dlssd_get_state != nullptr) {
    sl::DLSSDState state{};
    probe.trial_state_attempted = true;
    probe.trial_state_result =
        static_cast<int>(real_dlssd_get_state(sl::ViewportHandle(0u), state));
    if (static_cast<sl::Result>(probe.trial_state_result) == sl::Result::eOk) {
      probe.trial_vram_bytes = state.estimatedVRAMUsageInBytes;
    }
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
  diagnostics.rr_probe = std::move(probe);
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

// Clears captured activity but keeps module state, the slInit snapshot, the
// device/LUID and the captured DLSS options (one-shot observations that never
// repeat in a session).
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
  diagnostics.rr_probe = {};
}

// ---------------------------------------------------------------------------
// M1.5: DLSS-RR load path + DLSS option capture (still forward-only)
// ---------------------------------------------------------------------------

// Forward-only wrappers: the game fetches these pointers once at startup via
// slGetFeatureFunction, so every later call is captured here and still lands
// on the real plugin function unchanged.
inline PFun_slDLSSSetOptions* real_dlss_set_options = nullptr;
inline PFun_slDLSSGetOptimalSettings* real_dlss_get_optimal_settings = nullptr;

inline sl::Result WrappedDlssSetOptions(
    const sl::ViewportHandle& viewport, const sl::DLSSOptions& options) {
  if (real_dlss_set_options == nullptr) return sl::Result::eErrorInvalidState;
  {
    const std::lock_guard lock(diagnostics_mutex);
    auto& capture = diagnostics.dlss_options;
    capture.captured = true;
    capture.last_frame = diagnostics.last_frame_index;
    ++capture.set_options_calls;
    capture.mode = static_cast<uint32_t>(options.mode);
    capture.output_width = options.outputWidth;
    capture.output_height = options.outputHeight;
    capture.pre_exposure = options.preExposure;
    capture.exposure_scale = options.exposureScale;
    capture.color_buffers_hdr = static_cast<int>(options.colorBuffersHDR);
    capture.use_auto_exposure = static_cast<int>(options.useAutoExposure);
    capture.alpha_upscaling = static_cast<int>(options.alphaUpscalingEnabled);
    capture.presets = {
        static_cast<uint32_t>(options.dlaaPreset),
        static_cast<uint32_t>(options.qualityPreset),
        static_cast<uint32_t>(options.balancedPreset),
        static_cast<uint32_t>(options.performancePreset),
        static_cast<uint32_t>(options.ultraPerformancePreset),
        static_cast<uint32_t>(options.ultraQualityPreset),
    };
  }
  return real_dlss_set_options(viewport, options);
}

inline sl::Result WrappedDlssGetOptimalSettings(
    const sl::DLSSOptions& options, sl::DLSSOptimalSettings& settings) {
  if (real_dlss_get_optimal_settings == nullptr) return sl::Result::eErrorInvalidState;
  const auto result = real_dlss_get_optimal_settings(options, settings);
  {
    const std::lock_guard lock(diagnostics_mutex);
    auto& capture = diagnostics.dlss_options;
    ++capture.get_optimal_calls;
    capture.optimal_captured = true;
    capture.optimal_result = static_cast<int>(result);
    capture.optimal_mode = static_cast<uint32_t>(options.mode);
    if (result == sl::Result::eOk) {
      capture.optimal_render_width = settings.optimalRenderWidth;
      capture.optimal_render_height = settings.optimalRenderHeight;
      capture.optimal_sharpness = settings.optimalSharpness;
      capture.render_width_min = settings.renderWidthMin;
      capture.render_height_min = settings.renderHeightMin;
      capture.render_width_max = settings.renderWidthMax;
      capture.render_height_max = settings.renderHeightMax;
    }
  }
  return result;
}

// ---------------------------------------------------------------------------
// hooks — observe and forward, nothing else
// ---------------------------------------------------------------------------

inline sl::Result HookedSlInit(const sl::Preferences& pref, uint64_t sdk_version) {
  // Snapshot what the game asked for before touching anything.
  const uint32_t requested_count = pref.featuresToLoad != nullptr ? pref.numFeaturesToLoad : 0;

  // Build the effective feature list with DLSS-RR appended, unless the game
  // already asked for it, injection is disabled, or the list is a shape we do
  // not touch.
  uint32_t patched_count = 0;
  bool appended_rr = false;
  std::string injection_note;
  if (pref.featuresToLoad == nullptr) {
    injection_note = "featuresToLoad is null";
  } else if (requested_count > kMaxInitFeatures) {
    injection_note = "feature list too large to patch";
  } else {
    bool rr_requested = false;
    for (uint32_t i = 0; i < requested_count; ++i) {
      init_features_patched_storage[patched_count++] = pref.featuresToLoad[i];
      if (pref.featuresToLoad[i] == sl::kFeatureDLSS_RR) rr_requested = true;
    }
    if (rr_requested) {
      injection_note = "DLSS-RR already requested by the game";
    } else if (!GetInitInjection()) {
      injection_note = "disabled by setting";
    } else {
      init_features_patched_storage[patched_count++] = sl::kFeatureDLSS_RR;
      appended_rr = true;
    }
  }

  // Patch the caller's Preferences in place — only the two feature-list fields
  // are touched, so every other field (paths, callbacks, version) stays
  // exactly as the game wrote it. Restored right after the call.
  sl::Preferences& mutable_pref = const_cast<sl::Preferences&>(pref);
  const sl::Feature* const original_features = pref.featuresToLoad;
  const uint32_t original_count = pref.numFeaturesToLoad;
  if (appended_rr) {
    mutable_pref.featuresToLoad = init_features_patched_storage;
    mutable_pref.numFeaturesToLoad = patched_count;
  }

  const auto result = real_sl_init(pref, sdk_version);

  if (appended_rr) {
    mutable_pref.featuresToLoad = original_features;
    mutable_pref.numFeaturesToLoad = original_count;
  }

  const std::lock_guard lock(diagnostics_mutex);
  NoteCallLocked(kHookInit, _ReturnAddress());
  ++diagnostics.init_calls;
  diagnostics.init_seen = true;
  diagnostics.init_result = static_cast<int>(result);
  diagnostics.init_sdk_version = sdk_version;
  diagnostics.init_flags = static_cast<uint64_t>(pref.flags);
  diagnostics.init_log_level = static_cast<int>(pref.logLevel);
  diagnostics.init_engine = static_cast<int>(pref.engine);
  diagnostics.init_app_id = pref.applicationId;
  diagnostics.init_render_api = static_cast<int>(pref.renderAPI);
  diagnostics.init_pref_version = static_cast<uint32_t>(pref.structVersion);
  diagnostics.init_show_console = pref.showConsole;
  diagnostics.init_num_plugin_paths = pref.numPathsToPlugins;
  if (pref.pathsToPlugins != nullptr) {
    const uint32_t path_count =
        pref.numPathsToPlugins < diagnostics.init_plugin_paths.size()
            ? pref.numPathsToPlugins
            : static_cast<uint32_t>(diagnostics.init_plugin_paths.size());
    for (uint32_t i = 0; i < path_count; ++i) {
      diagnostics.init_plugin_paths[i] = WideToNarrow(pref.pathsToPlugins[i]);
    }
  }
  diagnostics.init_has_log_path = pref.pathToLogsAndData != nullptr;
  if (pref.pathToLogsAndData != nullptr) {
    diagnostics.init_log_path = WideToNarrow(pref.pathToLogsAndData);
  }
  diagnostics.init_has_engine_version = pref.engineVersion != nullptr;
  if (pref.engineVersion != nullptr) {
    diagnostics.init_engine_version = pref.engineVersion;
  }
  diagnostics.init_feature_count = requested_count;
  for (uint32_t i = 0; i < diagnostics.init_features_original.size() && i < requested_count;
       ++i) {
    diagnostics.init_features_original[i] = pref.featuresToLoad[i];
  }
  diagnostics.init_rr_injected = appended_rr;
  diagnostics.init_injection_note = std::move(injection_note);
  if (appended_rr) {
    diagnostics.init_effective_count = patched_count;
    for (uint32_t i = 0;
         i < diagnostics.init_features_effective.size() && i < patched_count; ++i) {
      diagnostics.init_features_effective[i] = init_features_patched_storage[i];
    }
  } else {
    diagnostics.init_effective_count = requested_count;
    diagnostics.init_features_effective = diagnostics.init_features_original;
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
  row.last_eval_input_info = {};
  if (inputs != nullptr) {
    const uint32_t captured_inputs =
        num_inputs < row.last_eval_input_info.size()
            ? num_inputs
            : static_cast<uint32_t>(row.last_eval_input_info.size());
    for (uint32_t i = 0; i < captured_inputs; ++i) {
      if (inputs[i] == nullptr) continue;
      row.last_eval_input_info[i].data1 = inputs[i]->structType.data1;
      row.last_eval_input_info[i].version = static_cast<uint32_t>(inputs[i]->structVersion);
    }
  }
  EvaluateCall call{};
  call.frame = frame_index;
  call.feature = feature;
  call.num_inputs = num_inputs;
  call.input_info = row.last_eval_input_info;
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
  {
    const std::lock_guard lock(diagnostics_mutex);
    NoteCallLocked(kHookGetFeatureFunction, _ReturnAddress());
    const std::string name = function_name != nullptr ? function_name : "?";
    bool found = false;
    for (auto& row : diagnostics.feature_functions) {
      if (row.feature == feature && row.name == name) {
        ++row.calls;
        row.last_result = static_cast<int>(result);
        row.last_frame = diagnostics.last_frame_index;
        found = true;
        break;
      }
    }
    if (!found) {
      FeatureFunctionRow row{};
      row.feature = feature;
      row.name = name;
      row.calls = 1;
      row.last_result = static_cast<int>(result);
      row.last_frame = diagnostics.last_frame_index;
      diagnostics.feature_functions.push_back(std::move(row));
    }
  }
  // Forward-only wrapping: hand the game our capture wrapper for the DLSS
  // option calls; the wrapper forwards to the real function unchanged.
  if (result == sl::Result::eOk && function != nullptr && function_name != nullptr
      && feature == sl::kFeatureDLSS) {
    if (std::strcmp(function_name, "slDLSSSetOptions") == 0) {
      real_dlss_set_options = reinterpret_cast<PFun_slDLSSSetOptions*>(function);
      function = reinterpret_cast<void*>(WrappedDlssSetOptions);
    } else if (std::strcmp(function_name, "slDLSSGetOptimalSettings") == 0) {
      real_dlss_get_optimal_settings = reinterpret_cast<PFun_slDLSSGetOptimalSettings*>(function);
      function = reinterpret_cast<void*>(WrappedDlssGetOptimalSettings);
    }
  }
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
  // Not detoured: only called by our probe.
  auto* p_requirements = reinterpret_cast<PFun_slGetFeatureRequirements*>(
      GetProcAddress(interposer, "slGetFeatureRequirements"));

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
  real_sl_get_feature_requirements = p_requirements;

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
    real_sl_get_feature_requirements = nullptr;
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
  s << "RR settings: slInit injection " << (GetInitInjection() ? "enabled" : "disabled")
    << ", runtime load " << (GetRuntimeLoad() ? "enabled" : "disabled") << "\n";

  s << "\n[slInit] calls: " << d.init_calls << "\n";
  if (!d.init_seen) {
    s << "not seen\n";
  } else {
    s << "result: " << ResultName(d.init_result) << "\n";
    s << "sdkVersion: 0x" << std::hex << d.init_sdk_version << std::dec << " (" << SdkVersionLabel(d.init_sdk_version) << ")\n";
    s << "flags: 0x" << std::hex << d.init_flags << std::dec << "\n";
    s << "logLevel: " << d.init_log_level << "  engine: " << d.init_engine
      << "  appId: " << d.init_app_id << "  renderAPI: " << d.init_render_api << "\n";
    s << "pref: structVersion=" << d.init_pref_version
      << " showConsole=" << (d.init_show_console ? "yes" : "no")
      << " pluginPaths=" << d.init_num_plugin_paths << "\n";
    for (size_t i = 0; i < d.init_plugin_paths.size(); ++i) {
      if (d.init_plugin_paths[i].empty()) continue;
      s << "  plugin path[" << i << "]: " << d.init_plugin_paths[i] << "\n";
    }
    if (d.init_has_log_path) s << "logPath: " << d.init_log_path << "\n";
    if (d.init_has_engine_version) s << "engineVersion: " << d.init_engine_version << "\n";
    s << "featuresToLoad (requested):";
    for (uint32_t i = 0; i < d.init_feature_count && i < d.init_features_original.size(); ++i) {
      s << " " << FeatureName(d.init_features_original[i]);
    }
    s << "\n";
    s << "featuresToLoad (effective):";
    for (uint32_t i = 0; i < d.init_effective_count && i < d.init_features_effective.size(); ++i) {
      s << " " << FeatureName(d.init_features_effective[i]);
    }
    if (d.init_rr_injected) {
      s << "   [DLSS-RR appended by mod]";
    } else if (!d.init_injection_note.empty()) {
      s << "   [" << d.init_injection_note << "]";
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
      << " calls=" << tag.calls << " lifecycle=" << LifecycleName(tag.lifecycle)
      << " frame=" << tag.last_frame << "\n";
    s << "      ";
    if (tag.cleared) {
      s << "[cleared at frame " << tag.cleared_frame << "]";
      if (tag.last_set_native != nullptr) {
        s << " lastSet=0x" << std::hex << reinterpret_cast<uint64_t>(tag.last_set_native) << std::dec
          << " at frame " << tag.last_set_frame;
      }
    } else {
      s << "res=0x" << std::hex << reinterpret_cast<uint64_t>(tag.native) << std::dec;
    }
    if (tag.desc_ok) {
      s << " desc=" << FormatLabel(tag.desc_format) << "(" << tag.desc_format << ")"
        << " " << tag.desc_width << "x" << tag.desc_height << " mips=" << tag.desc_mips
        << " layers=" << tag.desc_layers;
    }
    if (tag.sl_format != 0 || tag.sl_width != 0 || tag.sl_height != 0) {
      s << " raw: fmt(" << tag.sl_format << ") " << tag.sl_width << "x" << tag.sl_height
        << " mips=" << tag.sl_mips << " layers=" << tag.sl_layers;
    }
    if (tag.sl_state != 0xFFFFFFFFu) {
      s << " state=0x" << std::hex << tag.sl_state << std::dec;
    }
    if (tag.extent_w != 0 || tag.extent_h != 0 || tag.extent_left != 0 || tag.extent_top != 0) {
      s << " extent: left=" << tag.extent_left << " top=" << tag.extent_top
        << " " << tag.extent_w << "x" << tag.extent_h;
    }
    s << "\n";
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

  s << "\n[DLSS (SR) options]\n";
  {
    const auto& opts = d.dlss_options;
    if (!opts.captured && !opts.optimal_captured) {
      s << "not captured (no slDLSSSetOptions / slDLSSGetOptimalSettings calls seen)\n";
    } else {
      s << "setOptions calls=" << opts.set_options_calls
        << "  optimal queries=" << opts.get_optimal_calls
        << "  lastFrame=" << opts.last_frame << "\n";
      s << "  mode=" << sl::getDLSSModeAsStr(static_cast<sl::DLSSMode>(opts.mode))
        << " output=" << opts.output_width << "x" << opts.output_height
        << " preExposure=" << opts.pre_exposure << " exposureScale=" << opts.exposure_scale;
      if (opts.color_buffers_hdr >= 0) {
        s << " HDR=" << BooleanName(static_cast<sl::Boolean>(opts.color_buffers_hdr));
      }
      if (opts.use_auto_exposure >= 0) {
        s << " autoExposure=" << BooleanName(static_cast<sl::Boolean>(opts.use_auto_exposure));
      }
      if (opts.alpha_upscaling >= 0) {
        s << " alphaUpscale=" << BooleanName(static_cast<sl::Boolean>(opts.alpha_upscaling));
      }
      s << "\n";
      static const char* const kPresetLabels[6] = {
          "DLAA", "Quality", "Balanced", "Performance", "UltraPerf", "UltraQuality"};
      s << "  presets:";
      for (int i = 0; i < 6; ++i) {
        s << " " << kPresetLabels[i] << "=" << PresetName(opts.presets[static_cast<size_t>(i)]);
      }
      s << "\n";
      if (opts.optimal_captured) {
        s << "  game optimal query: mode="
          << sl::getDLSSModeAsStr(static_cast<sl::DLSSMode>(opts.optimal_mode))
          << " result=" << ResultName(opts.optimal_result)
          << " render=" << opts.optimal_render_width << "x" << opts.optimal_render_height
          << " sharpness=" << opts.optimal_sharpness
          << " min=" << opts.render_width_min << "x" << opts.render_height_min
          << " max=" << opts.render_width_max << "x" << opts.render_height_max << "\n";
      }
    }
  }

  s << "\n[evaluates] total: " << d.evaluates_total << "\n";
  for (const auto& [feature, row] : d.features) {
    if (row.evaluates == 0 && row.queries == 0 && !row.probed) continue;
    s << "  - " << FeatureName(feature) << " evals=" << row.evaluates
      << " lastResult=" << ResultName(row.last_eval_result) << " lastFrame=" << row.last_eval_frame
      << " caller=" << ModuleNameOf(row.last_eval_caller) << "\n";
    s << "      inputs=" << row.last_eval_inputs;
    for (uint32_t i = 0; i < row.last_eval_input_info.size() && i < row.last_eval_inputs; ++i) {
      if (row.last_eval_input_info[i].data1 == 0) continue;
      s << " in[" << i << "]=" << EvalInputLabel(row.last_eval_input_info[i]);
    }
    s << "\n";
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
      << " inputs=" << call.num_inputs;
    for (uint32_t i = 0; i < call.input_info.size() && i < call.num_inputs; ++i) {
      if (call.input_info[i].data1 == 0) continue;
      s << " in[" << i << "]=" << EvalInputLabel(call.input_info[i]);
    }
    s << " result=" << ResultName(call.result)
      << " caller=" << ModuleNameOf(call.caller) << "\n";
  }

  s << "\n[DLSS-RR probe]\n";
  {
    const auto& probe = d.rr_probe;
    const auto rr_it = d.features.find(sl::kFeatureDLSS_RR);
    if (!probe.ran && (rr_it == d.features.end() || !rr_it->second.probed)) {
      s << "not run (automatic probe fires at the first DLSS evaluate; the button re-runs it)\n";
    } else {
      if (rr_it != d.features.end()) {
        const auto& rr = rr_it->second;
        s << "  state: supported=" << ResultName(rr.supported_result)
          << " loaded=" << (rr.loaded_result == kNever ? "-" : (rr.loaded ? "yes" : "no"))
          << " loadedResult=" << ResultName(rr.loaded_result)
          << " versionResult=" << ResultName(rr.version_result)
          << " SL=" << rr.sl_major << "." << rr.sl_minor << "." << rr.sl_build
          << " NGX=" << rr.ngx_major << "." << rr.ngx_minor << "." << rr.ngx_build << "\n";
      }
      s << "  frame=" << probe.frame << " loadAttempts=" << probe.load_attempts;
      if (probe.load_attempts > 0) {
        s << " lastLoadResult=" << ResultName(probe.load_result)
          << " loadedAfterLoad=" << (probe.loaded_after_load ? "yes" : "no");
      } else {
        s << " (no runtime attempt: RR was requested at slInit or is disabled)";
      }
      s << "\n";
      if (probe.plugin_module_loaded) {
        s << "  plugin module: loaded at " << probe.plugin_module_path << "\n";
      } else {
        s << "  plugin module: not loaded";
        if (probe.plugin_file_present) {
          s << "; sl.dlss_d.dll present at " << probe.plugin_file_path;
        } else {
          s << "; sl.dlss_d.dll not found in the game folder";
        }
        s << "\n";
      }
      if (probe.requirements_attempted) {
        const bool requirements_ok =
            static_cast<sl::Result>(probe.requirements_result) == sl::Result::eOk;
        s << "  requirements: " << ResultName(probe.requirements_result);
        if (requirements_ok) {
          s << " flags=0x" << std::hex << probe.requirement_flags << std::dec
            << " [" << RequirementFlagsText(probe.requirement_flags) << "]"
            << " maxViewports=" << probe.max_viewports
            << " maxCPUThreads=" << probe.max_cpu_threads;
        }
        s << "\n";
        if (requirements_ok) {
          s << "    required tags:";
          if (probe.required_tags.empty()) {
            s << " (none)";
          } else {
            for (uint32_t tag : probe.required_tags) s << " " << BufferName(tag);
          }
          s << "\n";
          if (!probe.driver_version_required.empty() && probe.driver_version_required != "0.0.0") {
            s << "    driver required: " << probe.driver_version_required << "\n";
          }
          if (!probe.os_version_required.empty() && probe.os_version_required != "0.0.0") {
            s << "    OS required: " << probe.os_version_required << "\n";
          }
        }
      } else {
        s << "  requirements: unavailable (slGetFeatureRequirements not resolved)\n";
      }
      s << "  functions: slDLSSDSetOptions=" << ResultName(probe.fn_set_options_result)
        << " slDLSSDGetOptimalSettings=" << ResultName(probe.fn_get_optimal_result)
        << " slDLSSDGetState=" << ResultName(probe.fn_get_state_result) << "\n";
      s << "  trial optimal (mirrors the game's DLSS mode): ";
      if (!probe.trial_optimal_attempted) {
        s << "skipped (no captured slDLSSSetOptions yet)";
      } else {
        s << ResultName(probe.trial_optimal_result);
        if (static_cast<sl::Result>(probe.trial_optimal_result) == sl::Result::eOk) {
          s << " render=" << probe.trial_render_width << "x" << probe.trial_render_height
            << " sharpness=" << probe.trial_sharpness
            << " min=" << probe.trial_render_width_min << "x" << probe.trial_render_height_min
            << " max=" << probe.trial_render_width_max << "x" << probe.trial_render_height_max;
        }
      }
      s << "\n";
      s << "  trial state (viewport 0): ";
      if (!probe.trial_state_attempted) {
        s << "skipped";
      } else {
        s << ResultName(probe.trial_state_result);
        if (static_cast<sl::Result>(probe.trial_state_result) == sl::Result::eOk) {
          s << " estimatedVRAM=" << probe.trial_vram_bytes << " bytes";
        }
      }
      s << "\n";
    }
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
