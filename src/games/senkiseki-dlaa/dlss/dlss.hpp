#pragma once

#include <array>
#include <cstdint>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>

#include <Windows.h>
#include <d3d11.h>
#include <dxgi1_6.h>
#include <tlhelp32.h>
#include <include/reshade.hpp>
#include <nvsdk_ngx.h>
#include <nvsdk_ngx_helpers.h>
#include <wrl/client.h>

namespace senkiseki3::dlss {

// ── Settings globals (written by addon.cpp settings system) ──
inline float dlss_enabled = 1.f;
inline float dlss_frame_time_delta_ms = 16.7f;  // measured frame time (ms) for InFrameTimeDeltaInMsec
inline float dlss_render_preset = 0.f;
inline float dlss_motion_vectors_jittered = 0.f;
inline float dlss_debug_logging = 0.f;
inline float dlss_flag_is_hdr = 0.f;
inline float dlss_flag_depth_inverted = 1.f;
inline float dlss_flag_auto_exposure = 0.f;

// ── Instance ──
struct InstanceData {
  Microsoft::WRL::ComPtr<ID3D11Device> device;
  Microsoft::WRL::ComPtr<ID3D11Texture2D> output_texture;
  NVSDK_NGX_Parameter* capability_parameters = nullptr;
  NVSDK_NGX_Parameter* runtime_parameters = nullptr;
  NVSDK_NGX_Handle* feature = nullptr;
  uint32_t width = 0u;
  uint32_t height = 0u;
  DXGI_FORMAT output_format = DXGI_FORMAT_UNKNOWN;
  int requested_render_preset = -1;
  int render_preset = -1;
  int perf_quality = -1;
  int feature_flags = 0;
  bool initialized = false;
  bool supported = false;
  bool init_failed = false;
  bool create_failed = false;
  bool eval_failed = false;
  bool reset = true;
  bool logged_success = false;
  bool logged_jitter = false;
};

inline InstanceData ngx;

// ── Helpers ──

inline std::wstring GetProcessDirectory() {
  std::array<wchar_t, MAX_PATH> path = {};
  const DWORD length = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
  if (length == 0u || length >= path.size()) return L".";
  std::wstring result(path.data(), length);
  const auto separator = result.find_last_of(L"\\/");
  return separator == std::wstring::npos ? L"." : result.substr(0u, separator);
}

inline const char* ResultToString(NVSDK_NGX_Result result) {
  switch (result) {
    case NVSDK_NGX_Result_Success: return "Success";
    case NVSDK_NGX_Result_FAIL_FeatureNotSupported: return "FeatureNotSupported";
    case NVSDK_NGX_Result_FAIL_PlatformError: return "PlatformError";
    case NVSDK_NGX_Result_FAIL_FeatureAlreadyExists: return "FeatureAlreadyExists";
    case NVSDK_NGX_Result_FAIL_FeatureNotFound: return "FeatureNotFound";
    case NVSDK_NGX_Result_FAIL_InvalidParameter: return "InvalidParameter";
    case NVSDK_NGX_Result_FAIL_ScratchBufferTooSmall: return "ScratchBufferTooSmall";
    case NVSDK_NGX_Result_FAIL_NotInitialized: return "NotInitialized";
    case NVSDK_NGX_Result_FAIL_UnsupportedInputFormat: return "UnsupportedInputFormat";
    case NVSDK_NGX_Result_FAIL_RWFlagMissing: return "RWFlagMissing";
    case NVSDK_NGX_Result_FAIL_MissingInput: return "MissingInput";
    case NVSDK_NGX_Result_FAIL_UnableToInitializeFeature: return "UnableToInitializeFeature";
    case NVSDK_NGX_Result_FAIL_OutOfDate: return "OutOfDate";
    case NVSDK_NGX_Result_FAIL_OutOfGPUMemory: return "OutOfGPUMemory";
    case NVSDK_NGX_Result_FAIL_UnsupportedFormat: return "UnsupportedFormat";
    case NVSDK_NGX_Result_FAIL_UnableToWriteToAppDataPath: return "UnableToWriteToAppDataPath";
    case NVSDK_NGX_Result_FAIL_UnsupportedParameter: return "UnsupportedParameter";
    case NVSDK_NGX_Result_FAIL_Denied: return "Denied";
    case NVSDK_NGX_Result_FAIL_NotImplemented: return "NotImplemented";
    default: return "Unknown";
  }
}

// ── Preset / feature flags ──

inline int GetRenderPresetValue() {
  switch (static_cast<int>(dlss_render_preset)) {
    case 1: return static_cast<int>(NVSDK_NGX_DLSS_Hint_Render_Preset_F);
    case 2: return static_cast<int>(NVSDK_NGX_DLSS_Hint_Render_Preset_J);
    case 3: return static_cast<int>(NVSDK_NGX_DLSS_Hint_Render_Preset_K);
    case 4: return static_cast<int>(NVSDK_NGX_DLSS_Hint_Render_Preset_L);
    case 5: return static_cast<int>(NVSDK_NGX_DLSS_Hint_Render_Preset_M);
    default: return static_cast<int>(NVSDK_NGX_DLSS_Hint_Render_Preset_Default);
  }
}

inline int GetFeatureFlags() {
  // NOTE: MVLowRes intentionally omitted — our velocity compute shader emits
  // full-resolution motion vectors, so DLSS must not reinterpret them as half-res.
  // IsHDR / DepthInverted / AutoExposure are toggled from settings for A/B testing
  // (defaults: LDR input, reverse-Z depth, exposure already applied).
  int flags = 0;
  if (dlss_flag_depth_inverted != 0.f) flags |= NVSDK_NGX_DLSS_Feature_Flags_DepthInverted;
  if (dlss_flag_auto_exposure != 0.f) flags |= NVSDK_NGX_DLSS_Feature_Flags_AutoExposure;
  if (dlss_flag_is_hdr != 0.f) flags |= NVSDK_NGX_DLSS_Feature_Flags_IsHDR;
  if (dlss_motion_vectors_jittered != 0.f) flags |= NVSDK_NGX_DLSS_Feature_Flags_MVJittered;
  return flags;
}

inline const char* GetRenderPresetName(int preset) {
  switch (preset) {
    case static_cast<int>(NVSDK_NGX_DLSS_Hint_Render_Preset_F): return "F";
    case static_cast<int>(NVSDK_NGX_DLSS_Hint_Render_Preset_J): return "J";
    case static_cast<int>(NVSDK_NGX_DLSS_Hint_Render_Preset_K): return "K";
    case static_cast<int>(NVSDK_NGX_DLSS_Hint_Render_Preset_L): return "L";
    case static_cast<int>(NVSDK_NGX_DLSS_Hint_Render_Preset_M): return "M";
    default: return "Default";
  }
}

inline void SetRenderPresetParameters(NVSDK_NGX_Parameter* parameters, int render_preset) {
  if (parameters == nullptr) return;
  NVSDK_NGX_Parameter_SetUI(parameters, NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_DLAA, render_preset);
  NVSDK_NGX_Parameter_SetUI(parameters, NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Quality, render_preset);
  NVSDK_NGX_Parameter_SetUI(parameters, NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Balanced, render_preset);
  NVSDK_NGX_Parameter_SetUI(parameters, NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Performance, render_preset);
  NVSDK_NGX_Parameter_SetUI(parameters, NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_UltraPerformance, render_preset);
  NVSDK_NGX_Parameter_SetUI(parameters, NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_UltraQuality, render_preset);
}

// ── Feature lifecycle ──

inline void ReleaseFeatureHandleOnly() {
  if (ngx.feature != nullptr) {
    NVSDK_NGX_D3D11_ReleaseFeature(ngx.feature);
    ngx.feature = nullptr;
  }
  if (ngx.runtime_parameters != nullptr) {
    NVSDK_NGX_D3D11_DestroyParameters(ngx.runtime_parameters);
    ngx.runtime_parameters = nullptr;
  }
  ngx.requested_render_preset = -1;
  ngx.render_preset = -1;
  ngx.perf_quality = -1;
  ngx.feature_flags = 0;
  ngx.create_failed = false;
  ngx.eval_failed = false;
  ngx.reset = true;
  ngx.logged_success = false;
  ngx.logged_jitter = false;
}

inline void ReleaseFeature() {
  ReleaseFeatureHandleOnly();
  ngx.output_texture.Reset();
  ngx.width = 0u;
  ngx.height = 0u;
  ngx.output_format = DXGI_FORMAT_UNKNOWN;
}

inline void ReleaseNgx() {
  ReleaseFeature();
  if (ngx.capability_parameters != nullptr) {
    NVSDK_NGX_D3D11_DestroyParameters(ngx.capability_parameters);
    ngx.capability_parameters = nullptr;
  }
  if (ngx.initialized) {
    NVSDK_NGX_D3D11_Shutdown1(ngx.device.Get());
  }
  ngx.device.Reset();
  ngx.initialized = false;
  ngx.supported = false;
  ngx.init_failed = false;
}

// Log the loaded NGX/DLSS runtime module version + the NGX API version we
// compiled against. The game never shipped DLSS — the user injects this runtime —
// so this identifies exactly which DLSS binary is being driven. Robust: tries the
// exact module name, then enumerates loaded modules for anything "nvngx"-named,
// and logs the path even when no version resource is present.
static void LogNgxRuntimeVersion() {
  HMODULE mod = GetModuleHandleW(L"nvngx_dlss.dll");
  wchar_t path[MAX_PATH] = {};
  if (mod) {
    const DWORD len = GetModuleFileNameW(mod, path, MAX_PATH);
    if (len == 0u || len >= MAX_PATH) return;
  } else {
    // Enumerate loaded modules for a candidate (case-insensitive "nvngx").
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, 0);
    if (snap == INVALID_HANDLE_VALUE) return;
    MODULEENTRY32W me = {};
    me.dwSize = sizeof(me);
    if (Module32FirstW(snap, &me)) {
      do {
        if (_wcsnicmp(me.szModule, L"nvngx", 5) == 0 ||
            _wcsnicmp(me.szModule, L"dlss", 4) == 0) {
          wcscpy_s(path, me.szExePath);
          mod = me.hModule;
          break;
        }
      } while (Module32NextW(snap, &me));
    }
    CloseHandle(snap);
    if (path[0] == 0u) return;
  }
  char buf[320];
  DWORD dummy = 0;
  const DWORD vsize = GetFileVersionInfoSizeW(path, &dummy);
  if (vsize != 0u) {
    std::vector<uint8_t> blob(vsize);
    if (GetFileVersionInfoW(path, dummy, vsize, blob.data())) {
      VS_FIXEDFILEINFO* ffi = nullptr;
      UINT ffi_len = 0;
      if (VerQueryValueW(blob.data(), L"\\", reinterpret_cast<void**>(&ffi), &ffi_len) && ffi) {
        snprintf(buf, sizeof(buf),
                 "[DLAA] NGX runtime %u.%u.%u.%u, API 0x%X (%ls)",
                 HIWORD(ffi->dwFileVersionMS), LOWORD(ffi->dwFileVersionMS),
                 HIWORD(ffi->dwFileVersionLS), LOWORD(ffi->dwFileVersionLS),
                 NVSDK_NGX_VERSION_API_MACRO, path);
        reshade::log::message(reshade::log::level::info, buf);
        return;
      }
    }
  }
  // No version resource — still report the module path.
  snprintf(buf, sizeof(buf), "[DLAA] NGX runtime module: %ls (no version info)", path);
  reshade::log::message(reshade::log::level::info, buf);
}

inline bool EnsureNgxInitialized(ID3D11Device* native_device) {
  if (ngx.initialized) return ngx.supported;
  if (ngx.init_failed || native_device == nullptr) return false;

  const std::wstring process_directory = GetProcessDirectory();
  wchar_t* feature_paths[] = {const_cast<wchar_t*>(process_directory.c_str())};
  NVSDK_NGX_FeatureCommonInfo feature_info = {};
  feature_info.PathListInfo.Length = 1u;
  feature_info.PathListInfo.Path = feature_paths;

  const NVSDK_NGX_Result init_result = NVSDK_NGX_D3D11_Init(
      0x2050,
      process_directory.c_str(),
      native_device,
      &feature_info,
      NVSDK_NGX_Version_API);
  if (NVSDK_NGX_FAILED(init_result)) {
    ngx.init_failed = true;
    std::stringstream s;
    s << "[DLAA] NGX init failed: " << ResultToString(init_result)
      << " (0x" << std::hex << static_cast<uint32_t>(init_result) << ")";
    reshade::log::message(reshade::log::level::error, s.str().c_str());
    return false;
  }

  LogNgxRuntimeVersion();

  const NVSDK_NGX_Result params_result = NVSDK_NGX_D3D11_GetCapabilityParameters(&ngx.capability_parameters);
  if (NVSDK_NGX_FAILED(params_result) || ngx.capability_parameters == nullptr) {
    ngx.init_failed = true;
    std::stringstream s;
    s << "[DLAA] NGX capability parameters failed: " << ResultToString(params_result)
      << " (0x" << std::hex << static_cast<uint32_t>(params_result) << ")";
    reshade::log::message(reshade::log::level::error, s.str().c_str());
    NVSDK_NGX_D3D11_Shutdown1(native_device);
    return false;
  }

  int super_sampling_available = 0;
  ngx.capability_parameters->Get(NVSDK_NGX_EParameter_SuperSampling_Available, &super_sampling_available);
  ngx.supported = super_sampling_available > 0;
  if (!ngx.supported) {
    reshade::log::message(reshade::log::level::warning, "[DLAA] NGX reports DLAA is not supported on this device");
  }

  ngx.device = native_device;
  ngx.initialized = true;
  if (dlss_debug_logging != 0.f)
    reshade::log::message(reshade::log::level::info, "[DLAA] NGX initialized");
  return ngx.supported;
}

inline bool EnsureOutputTexture(uint32_t width, uint32_t height, DXGI_FORMAT format) {
  if (width == 0u || height == 0u || format == DXGI_FORMAT_UNKNOWN) return false;
  if (ngx.output_texture != nullptr && ngx.width == width && ngx.height == height && ngx.output_format == format) return true;

  ReleaseFeature();

  D3D11_TEXTURE2D_DESC desc = {};
  desc.Width = width;
  desc.Height = height;
  desc.MipLevels = 1u;
  desc.ArraySize = 1u;
  desc.Format = format;
  desc.SampleDesc.Count = 1u;
  desc.Usage = D3D11_USAGE_DEFAULT;
  desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

  const HRESULT hr = ngx.device->CreateTexture2D(&desc, nullptr, ngx.output_texture.ReleaseAndGetAddressOf());
  if (FAILED(hr)) {
    std::stringstream s;
    s << "[DLAA] output texture creation failed: 0x" << std::hex << static_cast<uint32_t>(hr);
    reshade::log::message(reshade::log::level::error, s.str().c_str());
    return false;
  }

  ngx.width = width;
  ngx.height = height;
  ngx.output_format = format;
  return true;
}

inline bool EnsureFeature(ID3D11DeviceContext* command_list, uint32_t width, uint32_t height,
                          DXGI_FORMAT output_format, int render_preset, int feature_flags) {
  if (command_list == nullptr || ngx.capability_parameters == nullptr || ngx.output_texture == nullptr) return false;
  const int perf_quality = static_cast<int>(NVSDK_NGX_PerfQuality_Value_DLAA);
  if (ngx.feature != nullptr && ngx.runtime_parameters != nullptr
      && ngx.width == width && ngx.height == height
      && ngx.requested_render_preset == render_preset
      && ngx.feature_flags == feature_flags) return true;
  if (ngx.feature != nullptr) ReleaseFeatureHandleOnly();
  if (ngx.create_failed) return false;

  const NVSDK_NGX_Result params_result = NVSDK_NGX_D3D11_AllocateParameters(&ngx.runtime_parameters);
  if (NVSDK_NGX_FAILED(params_result) || ngx.runtime_parameters == nullptr) {
    ngx.create_failed = true;
    std::stringstream s;
    s << "[DLAA] runtime parameters allocation failed: " << ResultToString(params_result)
      << " (0x" << std::hex << static_cast<uint32_t>(params_result) << ")";
    reshade::log::message(reshade::log::level::error, s.str().c_str());
    return false;
  }

  NVSDK_NGX_DLSS_Create_Params params = {};
  params.Feature.InWidth = width;
  params.Feature.InHeight = height;
  params.Feature.InTargetWidth = width;
  params.Feature.InTargetHeight = height;
  params.Feature.InPerfQualityValue = static_cast<NVSDK_NGX_PerfQuality_Value>(perf_quality);
  params.InFeatureCreateFlags = feature_flags;
  params.InEnableOutputSubrects = false;

  SetRenderPresetParameters(ngx.runtime_parameters, render_preset);

  int actual_render_preset = render_preset;
  NVSDK_NGX_Result result = NGX_D3D11_CREATE_DLSS_EXT(command_list, &ngx.feature, ngx.runtime_parameters, &params);
  if (NVSDK_NGX_FAILED(result)) {
    actual_render_preset = static_cast<int>(NVSDK_NGX_DLSS_Hint_Render_Preset_Default);
    SetRenderPresetParameters(ngx.runtime_parameters, actual_render_preset);
    result = NGX_D3D11_CREATE_DLSS_EXT(command_list, &ngx.feature, ngx.runtime_parameters, &params);
  }

  if (NVSDK_NGX_FAILED(result) || ngx.feature == nullptr) {
    ngx.create_failed = true;
    if (ngx.runtime_parameters != nullptr) {
      NVSDK_NGX_D3D11_DestroyParameters(ngx.runtime_parameters);
      ngx.runtime_parameters = nullptr;
    }
    ngx.requested_render_preset = -1;
    ngx.render_preset = -1;
    ngx.perf_quality = -1;
    std::stringstream s;
    s << "[DLAA] feature creation failed: " << ResultToString(result)
      << " (0x" << std::hex << static_cast<uint32_t>(result) << ")";
    reshade::log::message(reshade::log::level::error, s.str().c_str());
    return false;
  }

  ngx.requested_render_preset = render_preset;
  ngx.render_preset = actual_render_preset;
  ngx.perf_quality = perf_quality;
  ngx.feature_flags = feature_flags;
  ngx.reset = true;

  if (dlss_debug_logging != 0.f) {
    std::stringstream s;
    s << "[DLAA] feature created at " << width << "x" << height
      << " preset=" << GetRenderPresetName(ngx.render_preset)
      << " flags=" << feature_flags;
    reshade::log::message(reshade::log::level::info, s.str().c_str());
  }
  return true;
}

// ── Public API ──

inline bool InitDLSS(ID3D11Device* device, uint32_t width, uint32_t height) {
  if (!EnsureNgxInitialized(device)) return false;
  DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
  return EnsureOutputTexture(width, height, format);
}

inline void ShutdownDLSS() {
  ReleaseNgx();
}

inline bool EvaluateDLSS(ID3D11DeviceContext* command_list,
                         ID3D11Resource* source_color,
                         ID3D11Resource* output_color,
                         ID3D11Resource* motion_vectors,
                         ID3D11Resource* depth_buffer,
                         float jitter_x, float jitter_y,
                         float mv_scale_x, float mv_scale_y) {
  if (command_list == nullptr || dlss_enabled == 0.f || !ngx.supported) return false;
  if (source_color == nullptr || output_color == nullptr || motion_vectors == nullptr || depth_buffer == nullptr) return false;
  if (ngx.eval_failed) return false;

  D3D11_TEXTURE2D_DESC color_desc;
  static_cast<ID3D11Texture2D*>(source_color)->GetDesc(&color_desc);
  const uint32_t width = color_desc.Width;
  const uint32_t height = color_desc.Height;
  if (width == 0u || height == 0u) return false;

  const int render_preset = GetRenderPresetValue();
  const int feature_flags = GetFeatureFlags();

  D3D11_TEXTURE2D_DESC output_desc;
  static_cast<ID3D11Texture2D*>(output_color)->GetDesc(&output_desc);
  const DXGI_FORMAT output_format = output_desc.Format;

  if (!EnsureOutputTexture(width, height, output_format)) return false;
  if (!EnsureFeature(command_list, width, height, output_format, render_preset, feature_flags)) return false;

  NVSDK_NGX_D3D11_DLSS_Eval_Params eval = {};
  eval.Feature.pInColor = source_color;
  eval.Feature.pInOutput = ngx.output_texture.Get();
  eval.pInDepth = depth_buffer;
  eval.pInMotionVectors = motion_vectors;
  eval.InJitterOffsetX = jitter_x;
  eval.InJitterOffsetY = jitter_y;
  eval.InRenderSubrectDimensions.Width = width;
  eval.InRenderSubrectDimensions.Height = height;
  eval.InReset = ngx.reset ? 1 : 0;
  eval.InMVScaleX = mv_scale_x;
  eval.InMVScaleY = mv_scale_y;
  eval.InPreExposure = 1.f;
  eval.InExposureScale = 1.f;
  eval.InFrameTimeDeltaInMsec = dlss_frame_time_delta_ms;

  // DIAGNOSTIC: dump EVERY parameter handed to DLSS right before evaluation so we
  // can verify the integration populates the eval struct exactly as the runtime
  // expects. The user controls all DLSS inputs (the game never shipped DLSS).
  if (dlss_debug_logging != 0.f) {
    static int eval_log_count = 0;
    if (++eval_log_count % 60 == 0) {
      char buf[320];
      snprintf(buf, sizeof(buf),
        "[DLAA] eval W=%ux%u jit=(%.3f,%.3f) mvscale=(%.3f,%.3f) mvjit=%d reset=%d ftd=%.1fms sharp=%.2f preexp=%.2f flags=0x%X preset=%s",
        width, height, jitter_x, jitter_y, mv_scale_x, mv_scale_y,
        (feature_flags & NVSDK_NGX_DLSS_Feature_Flags_MVJittered) ? 1 : 0,
        eval.InReset, dlss_frame_time_delta_ms, eval.Feature.InSharpness,
        eval.InPreExposure, feature_flags, GetRenderPresetName(ngx.render_preset));
      reshade::log::message(reshade::log::level::info, buf);
    }
  }

  const NVSDK_NGX_Result result = NGX_D3D11_EVALUATE_DLSS_EXT(command_list, ngx.feature, ngx.runtime_parameters, &eval);
  if (NVSDK_NGX_FAILED(result)) {
    ngx.eval_failed = true;
    std::stringstream s;
    s << "[DLAA] evaluation failed: " << ResultToString(result)
      << " (0x" << std::hex << static_cast<uint32_t>(result) << ")";
    reshade::log::message(reshade::log::level::error, s.str().c_str());
    return false;
  }
  ngx.reset = false;

  if (dlss_debug_logging != 0.f) {
    if (!ngx.logged_success) {
      ngx.logged_success = true;
      std::stringstream s;
      s << "[DLAA] replacing FXAA at " << width << "x" << height
        << " preset=" << GetRenderPresetName(ngx.render_preset);
      reshade::log::message(reshade::log::level::info, s.str().c_str());
    }
    if (!ngx.logged_jitter) {
      ngx.logged_jitter = true;
      std::stringstream s;
      s << "[DLAA] jitter=(" << jitter_x << "," << jitter_y << ")"
        << " mv_scale=(" << mv_scale_x << "," << mv_scale_y << ")"
        << " mv_jittered=" << (dlss_motion_vectors_jittered != 0.f ? "on" : "off")
        << " reset=" << eval.InReset;
      reshade::log::message(reshade::log::level::info, s.str().c_str());
    }
  }
  return true;
}

}  // namespace senkiseki3::dlss
