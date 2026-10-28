/*
 * Copyright (C) 2026
 * SPDX-License-Identifier: MIT
 */

// Standalone test process. Arguments: ReShade DLL path, Streamline DLL folder,
// and optionally an overlay64.dll stand-in to exercise ReShade's native mode.
// This exercises the presentation bootstrap with FG off; it does not generate frames.
#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

#include <cstdio>
#include "../fg_factory.hpp"
#include "../streamline/include/sl_dlss_g.h"
#include "../streamline/include/sl_reflex.h"

struct __declspec(uuid("7F2C9A11-3B4E-4D6A-812F-5E9CD37A1B42")) ReShadeNative : IUnknown {};
using Microsoft::WRL::ComPtr;
int wmain(int argc, wchar_t** argv) {
  if (argc != 3 && argc != 4) return 1;
  if (argc == 4 && LoadLibraryExW(argv[3], nullptr, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32) == nullptr) return 1;
  HMODULE reshade = LoadLibraryExW(argv[1], nullptr, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);
  if (reshade == nullptr) return 2;
  auto create_factory = reinterpret_cast<decltype(&CreateDXGIFactory2)>(GetProcAddress(reshade, "CreateDXGIFactory2"));
  ComPtr<IDXGIFactory2> factory;
  if (create_factory == nullptr || FAILED(create_factory(0, IID_PPV_ARGS(&factory)))) return 7;
  farcry6_dlss::streamline::LoadRuntime(argv[2]);
  auto& runtime = farcry6_dlss::streamline::runtime;
  if (runtime.module == nullptr) return 3;

  sl::Feature features[] = {sl::kFeatureDLSS_G, sl::kFeatureReflex};
  const wchar_t* paths[] = {argv[2]};
  sl::Preferences preferences = {};
  preferences.flags = sl::PreferenceFlags::eUseManualHooking | sl::PreferenceFlags::eUseFrameBasedResourceTagging;
  preferences.featuresToLoad = features;
  preferences.numFeaturesToLoad = 2;
  preferences.pathsToPlugins = paths;
  preferences.numPathsToPlugins = 1;
  preferences.projectId = "a3f4c8e1-7b2d-4f6a-9c1e-5d8b0f2a6c4e";
  preferences.engineVersion = "1.0";
  if (runtime.init(preferences, sl::kSDKVersion) != sl::Result::eOk) return 4;

  ComPtr<ID3D12Device> device;
  if (FAILED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device)))) return 5;
  ComPtr<ReShadeNative> native;
  const HRESULT native_result = device->QueryInterface(IID_PPV_ARGS(&native));
  if (FAILED(native_result)) {
    std::printf("ReShade device unwrap failed: 0x%08lx\n", native_result);
    return 6;
  }
  const sl::Result device_result = runtime.set_device(native.Get());
  if (device_result != sl::Result::eOk) {
    std::printf("Streamline device registration failed: %d\n", static_cast<int>(device_result));
    return 6;
  }
  farcry6_dlss::streamline::bootstrap_device.store(native.Get());
  native.Reset();
  if (!farcry6_dlss::streamline::InstallFactoryHook(factory.Get())) return 8;
  farcry6_dlss::streamline::factory_hook_active.store(true);

  ComPtr<ID3D12CommandQueue> queue;
  D3D12_COMMAND_QUEUE_DESC queue_desc = {};
  if (FAILED(device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&queue)))) return 9;
  DXGI_SWAP_CHAIN_DESC1 desc = {};
  desc.Width = 640;
  desc.Height = 480;
  desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  desc.SampleDesc.Count = 1;
  desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  desc.BufferCount = 2;
  desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
  for (int iteration = 0; iteration < 2; ++iteration) {
    HWND window = CreateWindowExW(0, L"STATIC", L"Frame Generation test", WS_OVERLAPPEDWINDOW,
                                  0, 0, 640, 480, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
    if (window == nullptr) return 10;
    ComPtr<IDXGISwapChain1> chain;
    if (FAILED(factory->CreateSwapChainForHwnd(queue.Get(), window, &desc, nullptr, nullptr, &chain))) return 11;
    ComPtr<ReShadeNative> inner;
    void* real = nullptr;
    if (FAILED(chain->QueryInterface(IID_PPV_ARGS(&inner)))
        || runtime.get_native_interface(inner.Get(), &real) != sl::Result::eOk || real == nullptr) return 12;
    const bool wrapped = real != inner.Get();
    static_cast<IUnknown*>(real)->Release();
    if (!wrapped) return 12;
    inner.Reset();
    for (int frame = 0; frame < 8; ++frame) {
      if (FAILED(chain->Present(0, 0))) return 13;
    }
    if (FAILED(chain->ResizeBuffers(2, 800, 600, DXGI_FORMAT_R8G8B8A8_UNORM, 0)) || FAILED(chain->Present(0, 0))) return 14;
    chain.Reset();
    DestroyWindow(window);
  }
  farcry6_dlss::streamline::factory_hook_active.store(false);
  farcry6_dlss::streamline::bootstrap_device.store(nullptr);
  if (farcry6_dlss::streamline::factory_hook_upgrades.load() != 2
      || farcry6_dlss::streamline::factory_hook_failures.load() != 0) return 15;
  queue.Reset();
  factory.Reset();
  if (runtime.shutdown() != sl::Result::eOk) return 16;
  device.Reset();
  std::puts("ReShade/Streamline factory checks passed");
  return 0;
}
