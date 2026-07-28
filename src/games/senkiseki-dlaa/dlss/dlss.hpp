#pragma once

#include <cstdint>
#include <string>
#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <include/reshade.hpp>
#include <nvsdk_ngx.h>
#include <wrl/client.h>

namespace senkiseki3::dlss {

inline float dlss_enabled = 1.f;
inline float dlss_render_preset = 0.f;
inline bool dlss_initialized = false;
inline bool dlss_supported = false;

struct InstanceData {
  Microsoft::WRL::ComPtr<ID3D12Device> device;
  NVSDK_NGX_Handle* feature = nullptr;
  uint32_t width = 2560u;
  uint32_t height = 1440u;
  bool initialized = false;
};
inline InstanceData ngx;

inline bool InitDLSS(ID3D12Device* device, uint32_t width, uint32_t height) {
  if (ngx.initialized || !device) return ngx.initialized;
  ngx.width = width; ngx.height = height;
  ngx.device = device;

  // Real NGX integration needs:
  //   NVSDK_NGX_D3D12_Init(0x2050, L".", device)
  //   NVSDK_NGX_D3D12_CreateFeature(cmd_list, NVSDK_NGX_Feature_SuperSampling, params, &feature)
  // with params created via C++ virtual NVSDK_NGX_Parameter::Set()
  // Requires NGX Core SDK import library (not just nvngx_dlss.dll).
  // TODO: link against full NGX SDK or dynamically load via GetProcAddress.

  ngx.initialized = true; dlss_initialized = true; dlss_supported = true;
  reshade::log::message(reshade::log::level::info, "[DLAA] Init OK (NGX SDK integration pending)");
  return true;
}

inline void ShutdownDLSS() {
  // TODO: NVSDK_NGX_D3D12_ReleaseFeature/Shutdown1
  ngx = {}; dlss_initialized = false; dlss_supported = false;
}

inline void EvaluateDLSS(ID3D12GraphicsCommandList*, ID3D12Resource*, ID3D12Resource*,
                         ID3D12Resource*, ID3D12Resource*, float, float, float, float) {
  static int c = 0;
  if (++c <= 3) reshade::log::message(reshade::log::level::info,
    (std::string("[DLAA] EvaluateDLSS frame ") + std::to_string(c) + " (NGX SDK pending)").c_str());
}

}  // namespace senkiseki3::dlss
