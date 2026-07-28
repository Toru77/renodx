#pragma once

#include <array>
#include <cstdint>
#include <cstring>
#include <sstream>
#include <string>
#include <utility>

#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <include/reshade.hpp>
#include <nvsdk_ngx.h>
#include <nvsdk_ngx_helpers.h>
#include <wrl/client.h>

#include "../../utils/bitwise.hpp"
#include "../../utils/descriptor.hpp"
#include "../../utils/pipeline_layout.hpp"
#include "../../utils/resource.hpp"
#include "../../utils/shader.hpp"

namespace halo_infinite::dlss {

static constexpr uint32_t kTemporalAAHash = 0x36E82E75u;
static constexpr uint32_t kTrackedTableCount = 32u;
static constexpr uint32_t kTrackedCbvCount = 16u;
static constexpr uint32_t kTrackedRtvCount = 8u;

struct TaaConstantsSubset {
  float tcHistoryTexelSize[4];       // c000
  float tcTargetTexelSize[4];        // c001
  float tcSourceTexelSize[4];        // c002
  float tcVelocityTexelSize[4];      // c003
  float tcJitterOffset[4];           // c004
  float c005[4];
  float c006[4];
  float c007[4];
  float c008[4];
  uint32_t c009[4];
  int32_t c010[2];
  uint32_t c010_zw[2];
  int32_t c011[4];
  uint32_t texIndexDepth;            // c012.x, currently not used by shader
  uint32_t texIndexVelocity;         // c012.y
  uint32_t texIndexPresent;          // c012.z
  uint32_t texIndexHistory;          // c012.w
  uint32_t texIndexHistoryAux;       // c013.x
  uint32_t texIndexCompressedDepth;  // c013.y
  uint32_t texIndexStencil;          // c013.z
  uint32_t uavIndexVelocity;         // c013.w
};

struct __declspec(uuid("126b4faa-a67e-4dd1-8f94-d2d8ee5e3c36")) CommandListData {
  std::array<reshade::api::descriptor_table, kTrackedTableCount> descriptor_tables = {};
  std::array<reshade::api::buffer_range, kTrackedCbvCount> constant_buffers = {};
  std::array<reshade::api::resource_view, kTrackedRtvCount> rtvs = {};
  reshade::api::resource_view dsv = {};
};

namespace super_resolution {

struct SettingsData {
  uint32_t render_width = 0u;
  uint32_t render_height = 0u;
  DXGI_FORMAT output_format = DXGI_FORMAT_UNKNOWN;
  int render_preset = -1;
  int feature_flags = 0;
};

struct DrawData {
  ID3D12Resource* source_color = nullptr;
  ID3D12Resource* output_color = nullptr;
  ID3D12Resource* motion_vectors = nullptr;
  ID3D12Resource* depth_buffer = nullptr;
  float jitter_x = 0.f;
  float jitter_y = 0.f;
  float mv_scale_x = 0.f;
  float mv_scale_y = 0.f;
};

struct InstanceData {
  Microsoft::WRL::ComPtr<ID3D12Device> device;
  Microsoft::WRL::ComPtr<ID3D12Resource> output_texture;
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

inline InstanceData instance;

}  // namespace super_resolution

inline float dlss_enabled = 1.f;
inline float dlss_render_preset = 0.f;
inline float dlss_motion_vectors_jittered = 0.f;
inline bool is_nvidia_device = false;
inline bool attached = false;
inline bool logged_taa_inputs = false;
inline bool logged_taa_draw_detected = false;
inline bool logged_missing_constants = false;
inline bool logged_missing_resources = false;
inline auto& ngx = super_resolution::instance;

inline CommandListData* Get(reshade::api::command_list* cmd_list) {
  if (cmd_list == nullptr) return nullptr;
  auto* data = cmd_list->get_private_data<CommandListData>();
  return data != nullptr ? data : cmd_list->create_private_data<CommandListData>();
}

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

inline bool IsSupported() { return is_nvidia_device && (!ngx.initialized || ngx.supported); }

inline uint32_t GetBindlessTextureIndex(uint32_t packed_index) {
  return ((packed_index & 0xFFF00000u) == 0xC3500000u) ? (packed_index & 0x000FFFFFu) : 17u;
}

inline void ReleaseFeatureHandleOnly() {
  if (ngx.feature != nullptr) {
    NVSDK_NGX_D3D12_ReleaseFeature(ngx.feature);
    ngx.feature = nullptr;
  }
  if (ngx.runtime_parameters != nullptr) {
    NVSDK_NGX_D3D12_DestroyParameters(ngx.runtime_parameters);
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
    NVSDK_NGX_D3D12_DestroyParameters(ngx.capability_parameters);
    ngx.capability_parameters = nullptr;
  }
  if (ngx.initialized) {
    NVSDK_NGX_D3D12_Shutdown1(ngx.device.Get());
  }
  ngx.device.Reset();
  ngx.initialized = false;
  ngx.supported = false;
  ngx.init_failed = false;
}

inline bool EnsureNgxInitialized(reshade::api::device* device) {
  if (ngx.initialized) return ngx.supported;
  if (ngx.init_failed || device == nullptr || device->get_api() != reshade::api::device_api::d3d12) return false;

  auto* native_device = reinterpret_cast<ID3D12Device*>(device->get_native());
  if (native_device == nullptr) return false;

  const std::wstring process_directory = GetProcessDirectory();
  wchar_t* feature_paths[] = {const_cast<wchar_t*>(process_directory.c_str())};
  NVSDK_NGX_FeatureCommonInfo feature_info = {};
  feature_info.PathListInfo.Length = 1u;
  feature_info.PathListInfo.Path = feature_paths;

  const NVSDK_NGX_Result init_result = NVSDK_NGX_D3D12_Init_with_ProjectID(
      "2bce07a2-a7da-4c76-9a65-52d9c9819a0e",
      NVSDK_NGX_ENGINE_TYPE_CUSTOM,
      "1.0",
      process_directory.c_str(),
      native_device,
      &feature_info,
      NVSDK_NGX_Version_API);
  if (NVSDK_NGX_FAILED(init_result)) {
    ngx.init_failed = true;
    std::stringstream s;
    s << "Halo Infinite DLSS: NGX init failed: " << ResultToString(init_result)
      << " (0x" << std::hex << static_cast<uint32_t>(init_result) << ")";
    reshade::log::message(reshade::log::level::error, s.str().c_str());
    return false;
  }

  const NVSDK_NGX_Result params_result = NVSDK_NGX_D3D12_GetCapabilityParameters(&ngx.capability_parameters);
  if (NVSDK_NGX_FAILED(params_result) || ngx.capability_parameters == nullptr) {
    ngx.init_failed = true;
    std::stringstream s;
    s << "Halo Infinite DLSS: NGX capability parameters failed: " << ResultToString(params_result)
      << " (0x" << std::hex << static_cast<uint32_t>(params_result) << ")";
    reshade::log::message(reshade::log::level::error, s.str().c_str());
    NVSDK_NGX_D3D12_Shutdown1(native_device);
    return false;
  }

  int super_sampling_available = 0;
  ngx.capability_parameters->Get(NVSDK_NGX_EParameter_SuperSampling_Available, &super_sampling_available);
  ngx.supported = super_sampling_available > 0;
  if (!ngx.supported) {
    reshade::log::message(reshade::log::level::warning, "Halo Infinite DLSS: NGX reports DLSS/DLAA is not supported");
  }

  ngx.device = native_device;
  ngx.initialized = true;
  reshade::log::message(reshade::log::level::info, "Halo Infinite DLSS: NGX initialized");
  return ngx.supported;
}

inline bool EnsureOutputTexture(ID3D12Device* device, uint32_t width, uint32_t height, DXGI_FORMAT format) {
  if (device == nullptr || width == 0u || height == 0u || format == DXGI_FORMAT_UNKNOWN) return false;
  if (ngx.output_texture != nullptr && ngx.width == width && ngx.height == height && ngx.output_format == format) return true;

  ReleaseFeature();

  D3D12_HEAP_PROPERTIES heap_props = {};
  heap_props.Type = D3D12_HEAP_TYPE_DEFAULT;
  heap_props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
  heap_props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
  heap_props.CreationNodeMask = 1u;
  heap_props.VisibleNodeMask = 1u;

  D3D12_RESOURCE_DESC desc = {};
  desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
  desc.Alignment = 0u;
  desc.Width = width;
  desc.Height = height;
  desc.DepthOrArraySize = 1u;
  desc.MipLevels = 1u;
  desc.Format = format;
  desc.SampleDesc.Count = 1u;
  desc.SampleDesc.Quality = 0u;
  desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
  desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS | D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

  const HRESULT hr = device->CreateCommittedResource(
      &heap_props,
      D3D12_HEAP_FLAG_NONE,
      &desc,
      D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
      nullptr,
      IID_PPV_ARGS(ngx.output_texture.ReleaseAndGetAddressOf()));
  if (FAILED(hr)) {
    std::stringstream s;
    s << "Halo Infinite DLSS: output texture creation failed: 0x" << std::hex << static_cast<uint32_t>(hr);
    reshade::log::message(reshade::log::level::error, s.str().c_str());
    return false;
  }

  ngx.width = width;
  ngx.height = height;
  ngx.output_format = format;
  return true;
}

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
  int flags = NVSDK_NGX_DLSS_Feature_Flags_MVLowRes | NVSDK_NGX_DLSS_Feature_Flags_DepthInverted | NVSDK_NGX_DLSS_Feature_Flags_AutoExposure | NVSDK_NGX_DLSS_Feature_Flags_IsHDR;
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

inline bool EnsureFeature(ID3D12GraphicsCommandList* command_list, const super_resolution::SettingsData& settings) {
  const uint32_t width = settings.render_width;
  const uint32_t height = settings.render_height;
  if (command_list == nullptr || ngx.capability_parameters == nullptr || ngx.output_texture == nullptr) return false;
  const int render_preset = settings.render_preset;
  const int feature_flags = settings.feature_flags;
  const int perf_quality = static_cast<int>(NVSDK_NGX_PerfQuality_Value_DLAA);
  if (ngx.feature != nullptr && ngx.runtime_parameters != nullptr && ngx.width == width && ngx.height == height && ngx.requested_render_preset == render_preset && ngx.feature_flags == feature_flags) return true;
  if (ngx.feature != nullptr) ReleaseFeatureHandleOnly();
  if (ngx.create_failed) return false;

  const NVSDK_NGX_Result params_result = NVSDK_NGX_D3D12_AllocateParameters(&ngx.runtime_parameters);
  if (NVSDK_NGX_FAILED(params_result) || ngx.runtime_parameters == nullptr) {
    ngx.create_failed = true;
    std::stringstream s;
    s << "Halo Infinite DLSS: runtime parameters allocation failed: " << ResultToString(params_result)
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
  int actual_perf_quality = perf_quality;
  NVSDK_NGX_Result result = NGX_D3D12_CREATE_DLSS_EXT(command_list, 1u, 1u, &ngx.feature, ngx.runtime_parameters, &params);
  if (NVSDK_NGX_FAILED(result)) {
    actual_render_preset = static_cast<int>(NVSDK_NGX_DLSS_Hint_Render_Preset_Default);
    actual_perf_quality = static_cast<int>(NVSDK_NGX_PerfQuality_Value_Balanced);
    params.Feature.InPerfQualityValue = static_cast<NVSDK_NGX_PerfQuality_Value>(actual_perf_quality);
    SetRenderPresetParameters(ngx.runtime_parameters, actual_render_preset);
    result = NGX_D3D12_CREATE_DLSS_EXT(command_list, 1u, 1u, &ngx.feature, ngx.runtime_parameters, &params);
  }

  if (NVSDK_NGX_FAILED(result) || ngx.feature == nullptr) {
    ngx.create_failed = true;
    if (ngx.runtime_parameters != nullptr) {
      NVSDK_NGX_D3D12_DestroyParameters(ngx.runtime_parameters);
      ngx.runtime_parameters = nullptr;
    }
    ngx.requested_render_preset = -1;
    ngx.render_preset = -1;
    ngx.perf_quality = -1;
    std::stringstream s;
    s << "Halo Infinite DLSS: feature creation failed: " << ResultToString(result)
      << " (0x" << std::hex << static_cast<uint32_t>(result) << ")";
    reshade::log::message(reshade::log::level::error, s.str().c_str());
    return false;
  }

  ngx.requested_render_preset = render_preset;
  ngx.render_preset = actual_render_preset;
  ngx.perf_quality = actual_perf_quality;
  ngx.feature_flags = feature_flags;
  ngx.reset = true;

  std::stringstream s;
  s << "Halo Infinite DLAA: feature created at " << width << "x" << height
    << " preset=" << GetRenderPresetName(ngx.render_preset)
    << " flags=" << feature_flags;
  reshade::log::message(reshade::log::level::info, s.str().c_str());
  return true;
}

namespace super_resolution {

inline bool IsSupported() { return is_nvidia_device && (!instance.initialized || instance.supported); }

inline bool UpdateSettings(reshade::api::device* device, ID3D12GraphicsCommandList* command_list, const SettingsData& settings) {
  if (device == nullptr || command_list == nullptr) return false;
  if (!EnsureNgxInitialized(device)) return false;
  if (!EnsureOutputTexture(instance.device.Get(), settings.render_width, settings.render_height, settings.output_format)) return false;
  return EnsureFeature(command_list, settings);
}

inline bool Draw(ID3D12GraphicsCommandList* command_list, const DrawData& draw_data) {
  if (command_list == nullptr || instance.runtime_parameters == nullptr || instance.feature == nullptr) return false;
  if (draw_data.source_color == nullptr || draw_data.output_color == nullptr || draw_data.motion_vectors == nullptr || draw_data.depth_buffer == nullptr) return false;
  if (instance.eval_failed) return false;

  NVSDK_NGX_D3D12_DLSS_Eval_Params eval = {};
  eval.Feature.pInColor = draw_data.source_color;
  eval.Feature.pInOutput = instance.output_texture.Get();
  eval.pInDepth = draw_data.depth_buffer;
  eval.pInMotionVectors = draw_data.motion_vectors;
  eval.InJitterOffsetX = draw_data.jitter_x;
  eval.InJitterOffsetY = draw_data.jitter_y;
  eval.InRenderSubrectDimensions.Width = instance.width;
  eval.InRenderSubrectDimensions.Height = instance.height;
  eval.InReset = instance.reset ? 1 : 0;
  eval.InMVScaleX = draw_data.mv_scale_x;
  eval.InMVScaleY = draw_data.mv_scale_y;
  eval.InPreExposure = 1.f;
  eval.InExposureScale = 1.f;

  const NVSDK_NGX_Result result = NGX_D3D12_EVALUATE_DLSS_EXT(command_list, instance.feature, instance.runtime_parameters, &eval);
  if (NVSDK_NGX_FAILED(result)) {
    instance.eval_failed = true;
    std::stringstream s;
    s << "Halo Infinite DLSS: evaluation failed: " << ResultToString(result)
      << " (0x" << std::hex << static_cast<uint32_t>(result) << ")";
    reshade::log::message(reshade::log::level::error, s.str().c_str());
    return false;
  }
  instance.reset = false;

  D3D12_RESOURCE_BARRIER barriers[2] = {};
  barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  barriers[0].Transition.pResource = instance.output_texture.Get();
  barriers[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
  barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
  barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  barriers[1].Transition.pResource = draw_data.output_color;
  barriers[1].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
  barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
  command_list->ResourceBarrier(2u, barriers);
  command_list->CopyResource(draw_data.output_color, instance.output_texture.Get());
  std::swap(barriers[0].Transition.StateBefore, barriers[0].Transition.StateAfter);
  std::swap(barriers[1].Transition.StateBefore, barriers[1].Transition.StateAfter);
  command_list->ResourceBarrier(2u, barriers);

  if (!instance.logged_success) {
    instance.logged_success = true;
    std::stringstream s;
    s << "Halo Infinite DLAA: replacing TAA at " << instance.width << "x" << instance.height;
    reshade::log::message(reshade::log::level::info, s.str().c_str());
  }
  if (!instance.logged_jitter) {
    instance.logged_jitter = true;
    std::stringstream s;
    s << "Halo Infinite DLAA: jitter ngx_pixels=(" << eval.InJitterOffsetX << ", " << eval.InJitterOffsetY
      << ")"
      << " mv_scale=(" << eval.InMVScaleX << ", " << eval.InMVScaleY << ")"
      << " mv_low_res=on"
      << " mv_jittered=" << (dlss_motion_vectors_jittered != 0.f ? "on" : "off")
      << " reset=" << eval.InReset;
    reshade::log::message(reshade::log::level::info, s.str().c_str());
  }
  return true;
}

}  // namespace super_resolution

inline bool ResolveRegister(
    reshade::api::pipeline_layout layout,
    uint32_t layout_param,
    const reshade::api::descriptor_table_update& update,
    uint32_t descriptor_index,
    uint32_t& dx_register_index,
    uint32_t& dx_register_space) {
  const auto* layout_data = renodx::utils::pipeline_layout::GetPipelineLayoutData(layout);
  if (layout_data == nullptr || layout_param >= layout_data->params.size()) return false;

  const auto& param = layout_data->params[layout_param];
  const uint32_t binding = update.binding + descriptor_index;
  switch (param.type) {
    case reshade::api::pipeline_layout_param_type::push_descriptors:
      dx_register_index = param.push_descriptors.dx_register_index + binding;
      dx_register_space = param.push_descriptors.dx_register_space;
      return true;
    case reshade::api::pipeline_layout_param_type::descriptor_table:
    case reshade::api::pipeline_layout_param_type::push_descriptors_with_ranges:
    case reshade::api::pipeline_layout_param_type::push_descriptors_with_static_samplers:
    case reshade::api::pipeline_layout_param_type::descriptor_table_with_static_samplers:
      if (layout_param >= layout_data->ranges.size()) return false;
      for (const auto& range : layout_data->ranges[layout_param]) {
        const bool in_range = binding >= range.binding && (range.count == UINT32_MAX || binding < range.binding + range.count);
        if (!in_range) continue;
        dx_register_index = range.dx_register_index + (binding - range.binding);
        dx_register_space = range.dx_register_space;
        return true;
      }
      return false;
    default:
      return false;
  }
}

inline bool FindDescriptorTableForRegister(
    reshade::api::pipeline_layout layout,
    uint32_t dx_register_index,
    uint32_t dx_register_space,
    uint32_t& layout_param,
    uint32_t& binding) {
  const auto* layout_data = renodx::utils::pipeline_layout::GetPipelineLayoutData(layout);
  if (layout_data == nullptr) return false;
  for (uint32_t param_index = 0u; param_index < layout_data->ranges.size(); ++param_index) {
    for (const auto& range : layout_data->ranges[param_index]) {
      if (range.dx_register_space != dx_register_space) continue;
      if (dx_register_index < range.dx_register_index) continue;
      const uint32_t offset = dx_register_index - range.dx_register_index;
      if (range.count != UINT32_MAX && offset >= range.count) continue;
      layout_param = param_index;
      binding = range.binding + offset;
      return true;
    }
  }
  return false;
}

inline reshade::api::resource_view ResolveBindlessSrv(
    reshade::api::command_list* cmd_list,
    const CommandListData* data,
    uint32_t bindless_index) {
  if (cmd_list == nullptr || data == nullptr) return {0};
  auto* shader_state = renodx::utils::shader::GetCurrentState(cmd_list);
  if (shader_state == nullptr) return {0};
  auto* pixel_state = renodx::utils::shader::GetCurrentPixelState(shader_state);
  renodx::utils::shader::PopulateStageState(pixel_state);
  if (pixel_state->pipeline_details == nullptr) return {0};

  uint32_t layout_param = 0u;
  uint32_t binding = 0u;
  if (!FindDescriptorTableForRegister(pixel_state->pipeline_details->layout, 0u, 1u, layout_param, binding)) return {0};
  if (layout_param >= data->descriptor_tables.size()) return {0};
  const auto table = data->descriptor_tables[layout_param];
  if (table.handle == 0u) return {0};

  auto* device = cmd_list->get_device();
  auto* descriptor_data = renodx::utils::data::Get<renodx::utils::descriptor::DeviceData>(device);
  if (device == nullptr || descriptor_data == nullptr) return {0};

  uint32_t heap_offset = 0u;
  reshade::api::descriptor_heap heap = {0};
  device->get_descriptor_heap_offset(table, binding, 0u, &heap, &heap_offset);
  heap_offset += bindless_index;

  const std::shared_lock lock(descriptor_data->mutex);
  const auto heap_it = descriptor_data->heaps.find(heap.handle);
  if (heap_it == descriptor_data->heaps.end() || heap_offset >= heap_it->second.size()) return {0};
  const auto& slot = heap_it->second[heap_offset];
  return slot.HasResourceView() ? slot.resource_view : reshade::api::resource_view{0};
}

inline ID3D12Resource* GetNativeResource(reshade::api::device* device, reshade::api::resource_view view) {
  if (device == nullptr || view.handle == 0u) return nullptr;
  const auto resource = renodx::utils::resource::GetResourceFromView(device, view);
  if (resource.handle == 0u) return nullptr;
  return reinterpret_cast<ID3D12Resource*>(resource.handle);
}

inline bool ReadTaaConstants(const reshade::api::buffer_range& buffer_range, TaaConstantsSubset& constants) {
  if (buffer_range.buffer.handle == 0u) return false;
  auto* resource = reinterpret_cast<ID3D12Resource*>(buffer_range.buffer.handle);
  if (resource == nullptr) return false;

  const uint64_t offset = buffer_range.offset == UINT64_MAX ? 0u : buffer_range.offset;
  D3D12_RANGE read_range = {static_cast<SIZE_T>(offset), static_cast<SIZE_T>(offset + sizeof(TaaConstantsSubset))};
  void* mapped = nullptr;
  if (FAILED(resource->Map(0u, &read_range, &mapped)) || mapped == nullptr) return false;
  std::memcpy(&constants, static_cast<const uint8_t*>(mapped) + offset, sizeof(constants));
  D3D12_RANGE written_range = {0u, 0u};
  resource->Unmap(0u, &written_range);
  return true;
}

inline bool IsTemporalAADraw(reshade::api::command_list* cmd_list, CommandListData*& data) {
  auto* shader_state = renodx::utils::shader::GetCurrentState(cmd_list);
  if (shader_state == nullptr) return false;
  if (renodx::utils::shader::GetCurrentPixelShaderHash(shader_state) != kTemporalAAHash) return false;
  data = Get(cmd_list);
  return data != nullptr && data->rtvs[0].handle != 0u;
}

inline bool EvaluateDLSS(reshade::api::command_list* cmd_list, const CommandListData* data) {
  if (cmd_list == nullptr || data == nullptr || dlss_enabled == 0.f || !IsSupported()) return false;
  auto* device = cmd_list->get_device();
  if (device == nullptr || device->get_api() != reshade::api::device_api::d3d12) return false;

  auto* command_list = reinterpret_cast<ID3D12GraphicsCommandList*>(cmd_list->get_native());
  if (command_list == nullptr) return false;

  TaaConstantsSubset constants = {};
  if (!ReadTaaConstants(data->constant_buffers[0], constants)) {
    if (!logged_missing_constants) {
      logged_missing_constants = true;
      reshade::log::message(reshade::log::level::warning, "Halo Infinite DLSS: TAA draw found, but b0 constants were not captured");
    }
    return false;
  }

  const auto color_view = ResolveBindlessSrv(cmd_list, data, GetBindlessTextureIndex(constants.texIndexPresent));
  const auto motion_view = ResolveBindlessSrv(cmd_list, data, GetBindlessTextureIndex(constants.texIndexVelocity));
  const auto depth_view = ResolveBindlessSrv(cmd_list, data, GetBindlessTextureIndex(constants.texIndexCompressedDepth));
  auto* color = GetNativeResource(device, color_view);
  auto* motion_vectors = GetNativeResource(device, motion_view);
  auto* depth = GetNativeResource(device, depth_view);
  auto* output_target = GetNativeResource(device, data->rtvs[0]);
  if (color == nullptr || motion_vectors == nullptr || depth == nullptr || output_target == nullptr) {
    if (!logged_missing_resources) {
      logged_missing_resources = true;
      std::stringstream s;
      s << "Halo Infinite DLSS: TAA draw found, but resources failed to resolve"
        << " color_idx=" << GetBindlessTextureIndex(constants.texIndexPresent)
        << " velocity_idx=" << GetBindlessTextureIndex(constants.texIndexVelocity)
        << " depth_idx=" << GetBindlessTextureIndex(constants.texIndexCompressedDepth)
        << " color_view=" << reinterpret_cast<void*>(color_view.handle)
        << " velocity_view=" << reinterpret_cast<void*>(motion_view.handle)
        << " depth_view=" << reinterpret_cast<void*>(depth_view.handle)
        << " rtv0=" << reinterpret_cast<void*>(data->rtvs[0].handle);
      reshade::log::message(reshade::log::level::warning, s.str().c_str());
    }
    return false;
  }

  D3D12_RESOURCE_DESC color_desc = color->GetDesc();
  D3D12_RESOURCE_DESC output_desc = output_target->GetDesc();
  const uint32_t width = static_cast<uint32_t>(color_desc.Width);
  const uint32_t height = color_desc.Height;
  if (width == 0u || height == 0u || output_desc.Width != color_desc.Width || output_desc.Height != color_desc.Height) return false;

  super_resolution::SettingsData settings = {};
  settings.render_width = width;
  settings.render_height = height;
  settings.output_format = output_desc.Format;
  settings.render_preset = GetRenderPresetValue();
  settings.feature_flags = GetFeatureFlags();
  if (!super_resolution::UpdateSettings(device, command_list, settings)) return false;

  super_resolution::DrawData draw_data = {};
  draw_data.source_color = color;
  draw_data.output_color = output_target;
  draw_data.motion_vectors = motion_vectors;
  draw_data.depth_buffer = depth;
  // Halo's TAA constants store jitter as normalized texture coordinates (the
  // shader subtracts it from UVs before sampling the current frame). NGX expects
  // sub-pixel jitter in render pixels, so convert the UV offset with the source
  // texture dimensions.
  draw_data.jitter_x = constants.tcJitterOffset[0] * constants.tcSourceTexelSize[2];
  draw_data.jitter_y = constants.tcJitterOffset[1] * constants.tcSourceTexelSize[3];

  draw_data.mv_scale_x = 0;
  draw_data.mv_scale_y = 0;
  return super_resolution::Draw(command_list, draw_data);
}

inline void OnInitCommandList(reshade::api::command_list* cmd_list) { cmd_list->create_private_data<CommandListData>(); }
inline void OnDestroyCommandList(reshade::api::command_list* cmd_list) { cmd_list->destroy_private_data<CommandListData>(); }
inline void OnResetCommandList(reshade::api::command_list* cmd_list) { if (auto* data = Get(cmd_list)) *data = {}; }

inline void OnBindRenderTargetsAndDepthStencil(reshade::api::command_list* cmd_list, uint32_t count, const reshade::api::resource_view* rtvs, reshade::api::resource_view dsv) {
  auto* data = Get(cmd_list);
  if (data == nullptr) return;
  data->rtvs = {};
  for (uint32_t i = 0u; i < count && i < kTrackedRtvCount; ++i) data->rtvs[i] = rtvs != nullptr ? rtvs[i] : reshade::api::resource_view{};
  data->dsv = dsv;
}

inline void OnPushDescriptors(reshade::api::command_list* cmd_list, reshade::api::shader_stage stages, reshade::api::pipeline_layout layout, uint32_t layout_param, const reshade::api::descriptor_table_update& update) {
  if (!renodx::utils::bitwise::HasFlag(stages, reshade::api::shader_stage::pixel)) return;
  auto* data = Get(cmd_list);
  if (data == nullptr || update.type != reshade::api::descriptor_type::constant_buffer) return;
  for (uint32_t i = 0u; i < update.count; ++i) {
    uint32_t dx_register_index = 0u;
    uint32_t dx_register_space = 0u;
    if (!ResolveRegister(layout, layout_param, update, i, dx_register_index, dx_register_space)) continue;
    if (dx_register_space != 0u || dx_register_index >= kTrackedCbvCount) continue;
    data->constant_buffers[dx_register_index] = static_cast<const reshade::api::buffer_range*>(update.descriptors)[i];
  }
}

inline void OnBindDescriptorTables(reshade::api::command_list* cmd_list, reshade::api::shader_stage stages, reshade::api::pipeline_layout, uint32_t first, uint32_t count, const reshade::api::descriptor_table* tables) {
  if (!renodx::utils::bitwise::HasFlag(stages, reshade::api::shader_stage::pixel)) return;
  auto* data = Get(cmd_list);
  if (data == nullptr || tables == nullptr) return;
  for (uint32_t i = 0u; i < count && first + i < kTrackedTableCount; ++i) data->descriptor_tables[first + i] = tables[i];
}

inline bool OnDraw(reshade::api::command_list* cmd_list, uint32_t, uint32_t, uint32_t, uint32_t) {
  CommandListData* data = nullptr;
  if (!IsTemporalAADraw(cmd_list, data)) return false;
  if (!logged_taa_draw_detected) {
    logged_taa_draw_detected = true;
    reshade::log::message(reshade::log::level::info, "Halo Infinite DLSS: detected TAA draw 0x36E82E75");
  }
  return EvaluateDLSS(cmd_list, data);
}

inline bool OnDrawIndexed(reshade::api::command_list* cmd_list, uint32_t, uint32_t, uint32_t, int32_t, uint32_t) {
  CommandListData* data = nullptr;
  if (!IsTemporalAADraw(cmd_list, data)) return false;
  if (!logged_taa_draw_detected) {
    logged_taa_draw_detected = true;
    reshade::log::message(reshade::log::level::info, "Halo Infinite DLSS: detected TAA indexed draw 0x36E82E75");
  }
  return EvaluateDLSS(cmd_list, data);
}

inline void OnInitDevice(reshade::api::device* device) {
  is_nvidia_device = false;
  if (device == nullptr) return;
  int vendor_id = 0;
  is_nvidia_device = device->get_property(reshade::api::device_properties::vendor_id, &vendor_id) && vendor_id == 0x10de;
  if (!is_nvidia_device) {
    std::stringstream s;
    s << "Halo Infinite DLSS: current device is not NVIDIA, vendor=0x" << std::hex << vendor_id;
    reshade::log::message(reshade::log::level::info, s.str().c_str());
  } else {
    reshade::log::message(reshade::log::level::info, "Halo Infinite DLSS: NVIDIA device detected");
  }
}

inline void OnDestroyDevice(reshade::api::device*) {
  ReleaseNgx();
  is_nvidia_device = false;
}

inline void Use(DWORD fdw_reason) {
  renodx::utils::descriptor::trace_descriptor_tables = true;
  renodx::utils::descriptor::Use(fdw_reason);
  renodx::utils::pipeline_layout::Use(fdw_reason);
  renodx::utils::resource::Use(fdw_reason);

  switch (fdw_reason) {
    case DLL_PROCESS_ATTACH:
      if (attached) return;
      attached = true;
      reshade::register_event<reshade::addon_event::init_device>(OnInitDevice);
      reshade::register_event<reshade::addon_event::destroy_device>(OnDestroyDevice);
      reshade::register_event<reshade::addon_event::init_command_list>(OnInitCommandList);
      reshade::register_event<reshade::addon_event::destroy_command_list>(OnDestroyCommandList);
      reshade::register_event<reshade::addon_event::reset_command_list>(OnResetCommandList);
      reshade::register_event<reshade::addon_event::bind_render_targets_and_depth_stencil>(OnBindRenderTargetsAndDepthStencil);
      reshade::register_event<reshade::addon_event::push_descriptors>(OnPushDescriptors);
      reshade::register_event<reshade::addon_event::bind_descriptor_tables>(OnBindDescriptorTables);
      reshade::register_event<reshade::addon_event::draw>(OnDraw);
      reshade::register_event<reshade::addon_event::draw_indexed>(OnDrawIndexed);
      break;
    case DLL_PROCESS_DETACH:
      if (!attached) return;
      attached = false;
      reshade::unregister_event<reshade::addon_event::init_device>(OnInitDevice);
      reshade::unregister_event<reshade::addon_event::destroy_device>(OnDestroyDevice);
      reshade::unregister_event<reshade::addon_event::init_command_list>(OnInitCommandList);
      reshade::unregister_event<reshade::addon_event::destroy_command_list>(OnDestroyCommandList);
      reshade::unregister_event<reshade::addon_event::reset_command_list>(OnResetCommandList);
      reshade::unregister_event<reshade::addon_event::bind_render_targets_and_depth_stencil>(OnBindRenderTargetsAndDepthStencil);
      reshade::unregister_event<reshade::addon_event::push_descriptors>(OnPushDescriptors);
      reshade::unregister_event<reshade::addon_event::bind_descriptor_tables>(OnBindDescriptorTables);
      reshade::unregister_event<reshade::addon_event::draw>(OnDraw);
      reshade::unregister_event<reshade::addon_event::draw_indexed>(OnDrawIndexed);
      ReleaseNgx();
      break;
  }
}

}  // namespace halo_infinite::dlss
