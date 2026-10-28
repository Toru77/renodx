/*
 * Copyright (C) 2026
 * SPDX-License-Identifier: MIT
 */

// Isolated Streamline input smoke test; no game resources are used.
// Arguments: signed runtime directory and optional --enable-fg / --volatile / --foreground.
#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

#include <cstdio>
#include <cstring>
#include <utility>

#include "../streamline_runtime.hpp"
#include "../fg_mode.hpp"
#include "../streamline/include/sl_dlss_g.h"
#include "../streamline/include/sl_pcl.h"
#include "../streamline/include/sl_reflex.h"

using Microsoft::WRL::ComPtr;

int wmain(int argc, wchar_t** argv) {
  if (argc < 2 || argc > 5) return 1;
  bool enable_fg = false;
  bool volatile_inputs = false;
  bool foreground = false;
  for (int i = 2; i < argc; ++i) {
    if (wcscmp(argv[i], L"--enable-fg") == 0) {
      enable_fg = true;
    } else if (wcscmp(argv[i], L"--volatile") == 0) {
      volatile_inputs = true;
    } else if (wcscmp(argv[i], L"--foreground") == 0) {
      foreground = true;
    } else {
      return 1;
    }
  }
  farcry6_dlss::streamline::LoadRuntime(argv[1]);
  auto& runtime = farcry6_dlss::streamline::runtime;
  if (runtime.module == nullptr) return 2;
  sl::Feature features[] = {sl::kFeatureDLSS_G, sl::kFeatureReflex};
  const wchar_t* paths[] = {argv[1]};
  sl::Preferences preferences = {};
  preferences.flags = sl::PreferenceFlags::eUseManualHooking | sl::PreferenceFlags::eUseFrameBasedResourceTagging;
  preferences.featuresToLoad = features;
  preferences.numFeaturesToLoad = 2u;
  preferences.pathsToPlugins = paths;
  preferences.numPathsToPlugins = 1u;
  preferences.projectId = "a3f4c8e1-7b2d-4f6a-9c1e-5d8b0f2a6c4e";
  preferences.engineVersion = "1.0";
  preferences.logLevel = sl::LogLevel::eVerbose;
  preferences.logMessageCallback = [](sl::LogType type, const char* message) {
    if ((type == sl::LogType::eWarn || type == sl::LogType::eError) && message != nullptr) {
      std::printf("Streamline %s: %s\n", type == sl::LogType::eError ? "error" : "warning", message);
    } else if (message != nullptr && std::strstr(message, "dlfgPresent.cpp") != nullptr) {
      std::printf("Streamline info: %s\n", message);
    }
  };
  if (runtime.init(preferences, sl::kSDKVersion) != sl::Result::eOk) return 3;

  ComPtr<ID3D12Device> device;
  if (FAILED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device)))) return 4;
  if (runtime.set_device(device.Get()) != sl::Result::eOk) return 5;
  void* upgraded_device = device.Get();
  if (runtime.upgrade_interface(&upgraded_device) != sl::Result::eOk
      || upgraded_device == nullptr || upgraded_device == device.Get()) return 31;
  ComPtr<ID3D12Device> proxy_device;
  proxy_device.Attach(static_cast<ID3D12Device*>(upgraded_device));
  ComPtr<ID3D12CommandQueue> proxy_queue;
  ComPtr<ID3D12CommandQueue> queue;
  D3D12_COMMAND_QUEUE_DESC queue_desc = {};
  if (FAILED(proxy_device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&proxy_queue)))) return 6;
  void* native_queue = nullptr;
  if (runtime.get_native_interface(proxy_queue.Get(), &native_queue) != sl::Result::eOk || native_queue == nullptr) return 32;
  queue.Attach(static_cast<ID3D12CommandQueue*>(native_queue));
  proxy_queue.Reset();
  ComPtr<IDXGIFactory2> factory;
  if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))) return 7;
  void* upgraded = factory.Get();
  if (runtime.upgrade_interface(&upgraded) != sl::Result::eOk || upgraded == nullptr || upgraded == factory.Get()) return 8;
  factory.Reset();
  factory.Attach(static_cast<IDXGIFactory2*>(upgraded));
  HWND window = CreateWindowExW(0, L"STATIC", L"Streamline input test", WS_OVERLAPPEDWINDOW,
                                0, 0, 640, 480, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
  if (window == nullptr) return 9;
  if (foreground) {
    ShowWindow(window, SW_SHOWNORMAL);
    SetForegroundWindow(window);
  }
  DXGI_SWAP_CHAIN_DESC1 desc = {};
  desc.Width = 640u;
  desc.Height = 480u;
  desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  desc.SampleDesc.Count = 1u;
  desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  desc.BufferCount = 2u;
  desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
  ComPtr<IDXGISwapChain1> swapchain;
  if (FAILED(factory->CreateSwapChainForHwnd(queue.Get(), window, &desc, nullptr, nullptr, &swapchain))) return 10;
  ComPtr<IDXGISwapChain3> swapchain3;
  if (FAILED(swapchain.As(&swapchain3))) return 19;

  PFun_slPCLSetMarker* marker = nullptr;
  PFun_slReflexSleep* sleep = nullptr;
  PFun_slDLSSGSetOptions* set_options = nullptr;
  PFun_slDLSSGGetState* get_state = nullptr;
  farcry6_dlss::frame_generation::ModeState mode_state;
  void* marker_function = nullptr;
  if (runtime.get_feature_function(sl::kFeaturePCL, "slPCLSetMarker", marker_function) != sl::Result::eOk
      || marker_function == nullptr) return 20;
  marker = reinterpret_cast<PFun_slPCLSetMarker*>(marker_function);
  if (enable_fg) {
    void* function = nullptr;
    if (runtime.get_feature_function(sl::kFeatureReflex, "slReflexSleep", function) != sl::Result::eOk || function == nullptr) return 21;
    sleep = reinterpret_cast<PFun_slReflexSleep*>(function);
    function = nullptr;
    if (runtime.get_feature_function(sl::kFeatureReflex, "slReflexSetOptions", function) != sl::Result::eOk || function == nullptr) return 22;
    const auto reflex_options = reinterpret_cast<PFun_slReflexSetOptions*>(function);
    function = nullptr;
    if (runtime.get_feature_function(sl::kFeatureDLSS_G, "slDLSSGSetOptions", function) != sl::Result::eOk || function == nullptr) return 23;
    set_options = reinterpret_cast<PFun_slDLSSGSetOptions*>(function);
    function = nullptr;
    if (runtime.get_feature_function(sl::kFeatureDLSS_G, "slDLSSGGetState", function) != sl::Result::eOk || function == nullptr) return 24;
    get_state = reinterpret_cast<PFun_slDLSSGGetState*>(function);
    sl::ReflexOptions reflex = {};
    reflex.mode = sl::ReflexMode::eLowLatency;
    if (reflex_options(reflex) != sl::Result::eOk) return 25;
    sl::DLSSGOptions options = {};
    options.mode = sl::DLSSGMode::eOn;
    if (set_options(sl::ViewportHandle(0u), options) != sl::Result::eOk) return 26;
    mode_state.OptionsAccepted(farcry6_dlss::frame_generation::ModeState::Mode::on);
  }

  D3D12_HEAP_PROPERTIES heap = {};
  heap.Type = D3D12_HEAP_TYPE_DEFAULT;
  D3D12_RESOURCE_DESC texture = {};
  texture.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
  texture.Width = 320u;
  texture.Height = 240u;
  texture.DepthOrArraySize = 1u;
  texture.MipLevels = 1u;
  texture.SampleDesc.Count = 1u;
  texture.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
  texture.Format = DXGI_FORMAT_R32_FLOAT;
  ComPtr<ID3D12Resource> depth;
  if (FAILED(device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &texture, D3D12_RESOURCE_STATE_COMMON,
                                              nullptr, IID_PPV_ARGS(&depth)))) return 11;
  texture.Format = DXGI_FORMAT_R16G16_FLOAT;
  ComPtr<ID3D12Resource> motion;
  if (FAILED(device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &texture, D3D12_RESOURCE_STATE_COMMON,
                                              nullptr, IID_PPV_ARGS(&motion)))) return 12;

  D3D12_DESCRIPTOR_HEAP_DESC rtv_heap_desc = {};
  rtv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
  rtv_heap_desc.NumDescriptors = 4u;
  ComPtr<ID3D12DescriptorHeap> rtv_heap;
  if (FAILED(device->CreateDescriptorHeap(&rtv_heap_desc, IID_PPV_ARGS(&rtv_heap)))) return 33;
  const auto descriptor_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
  D3D12_CPU_DESCRIPTOR_HANDLE rtvs[4] = {};
  rtvs[0] = rtv_heap->GetCPUDescriptorHandleForHeapStart();
  for (uint32_t i = 1u; i < 4u; ++i) rtvs[i].ptr = rtvs[0].ptr + i * descriptor_size;
  device->CreateRenderTargetView(depth.Get(), nullptr, rtvs[0]);
  device->CreateRenderTargetView(motion.Get(), nullptr, rtvs[1]);
  ComPtr<ID3D12Resource> backbuffers[2];
  for (uint32_t i = 0u; i < 2u; ++i) {
    if (FAILED(swapchain->GetBuffer(i, IID_PPV_ARGS(&backbuffers[i])))) return 34;
    device->CreateRenderTargetView(backbuffers[i].Get(), nullptr, rtvs[i + 2u]);
  }
  ComPtr<ID3D12CommandAllocator> allocator;
  ComPtr<ID3D12GraphicsCommandList> commands;
  ComPtr<ID3D12Fence> fence;
  if (FAILED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator)))
      || FAILED(device->CreateCommandList(0u, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator.Get(), nullptr, IID_PPV_ARGS(&commands)))
      || FAILED(commands->Close()) || FAILED(device->CreateFence(0u, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)))) return 35;
  HANDLE event = CreateEventW(nullptr, FALSE, FALSE, nullptr);
  if (event == nullptr) return 36;

  sl::Constants constants = {};
  for (uint32_t row = 0u; row < 4u; ++row) {
    const sl::float4 identity(row == 0u ? 1.f : 0.f, row == 1u ? 1.f : 0.f,
                              row == 2u ? 1.f : 0.f, row == 3u ? 1.f : 0.f);
    constants.cameraViewToClip.setRow(row, identity);
    constants.clipToCameraView.setRow(row, identity);
    constants.clipToPrevClip.setRow(row, identity);
    constants.prevClipToClip.setRow(row, identity);
  }
  constants.jitterOffset = sl::float2(0.f, 0.f);
  constants.mvecScale = sl::float2(1.f / 320.f, 1.f / 240.f);
  constants.cameraPinholeOffset = sl::float2(0.f, 0.f);
  constants.cameraPos = sl::float3(0.f, 0.f, 0.f);
  constants.cameraUp = sl::float3(0.f, 1.f, 0.f);
  constants.cameraRight = sl::float3(1.f, 0.f, 0.f);
  constants.cameraFwd = sl::float3(0.f, 0.f, -1.f);
  constants.cameraNear = 0.1f;
  constants.cameraFar = 1000.f;
  constants.cameraFOV = 1.f;
  constants.cameraAspectRatio = 4.f / 3.f;
  constants.depthInverted = sl::Boolean::eTrue;
  constants.cameraMotionIncluded = sl::Boolean::eTrue;
  constants.motionVectors3D = sl::Boolean::eFalse;
  constants.reset = sl::Boolean::eFalse;
  const sl::ViewportHandle viewport(0u);
  const sl::Extent extent{0u, 0u, 320u, 240u};
  const sl::Extent output_extent{0u, 0u, 640u, 480u};
  sl::Resource depth_resource(sl::ResourceType::eTex2d, depth.Get(), D3D12_RESOURCE_STATE_COMMON);
  sl::Resource motion_resource(sl::ResourceType::eTex2d, motion.Get(), D3D12_RESOURCE_STATE_COMMON);
  const auto input_lifetime = (volatile_inputs ? sl::ResourceLifecycle::eOnlyValidNow : sl::ResourceLifecycle::eValidUntilPresent);
  sl::ResourceTag tags[] = {
      {&depth_resource, sl::kBufferTypeDepth, input_lifetime, &extent},
      {&motion_resource, sl::kBufferTypeMotionVectors, input_lifetime, &extent},
      {nullptr, sl::kBufferTypeBackbuffer, sl::ResourceLifecycle::eValidUntilPresent, &output_extent},
  };
  sl::ResourceTag clear_tags[] = {
      {nullptr, sl::kBufferTypeDepth, sl::ResourceLifecycle::eValidUntilPresent},
      {nullptr, sl::kBufferTypeMotionVectors, sl::ResourceLifecycle::eValidUntilPresent},
      {nullptr, sl::kBufferTypeBackbuffer, sl::ResourceLifecycle::eValidUntilPresent},
  };
  uint32_t presented_frames = 0u;
  uint32_t last_status = 0u;
  const uint32_t total_frames = (enable_fg ? 90u : 8u);
  for (uint32_t frame = 1u; frame <= total_frames; ++frame) {
    sl::FrameToken* token = nullptr;
    const auto token_result = runtime.get_frame_token(token, &frame);
    if (token_result != sl::Result::eOk || token == nullptr) {
      std::printf("frame token failed: %d\n", static_cast<int>(token_result));
      return 13;
    }
    if (enable_fg && (sleep(*token) != sl::Result::eOk
                      || marker(sl::PCLMarker::eSimulationStart, *token) != sl::Result::eOk
                      || marker(sl::PCLMarker::eSimulationEnd, *token) != sl::Result::eOk
                      || marker(sl::PCLMarker::eRenderSubmitStart, *token) != sl::Result::eOk)) return 27;
    if (FAILED(allocator->Reset()) || FAILED(commands->Reset(allocator.Get(), nullptr))) return 37;
    const UINT index = swapchain3->GetCurrentBackBufferIndex();
    if (index >= 2u) return 38;
    ID3D12Resource* resources[] = {depth.Get(), motion.Get(), backbuffers[index].Get()};
    D3D12_RESOURCE_BARRIER barriers[3] = {};
    for (uint32_t i = 0u; i < 3u; ++i) {
      barriers[i].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
      barriers[i].Transition.pResource = resources[i];
      barriers[i].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
      barriers[i].Transition.StateBefore = (i == 2u ? D3D12_RESOURCE_STATE_PRESENT : D3D12_RESOURCE_STATE_COMMON);
      barriers[i].Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    }
    commands->ResourceBarrier(3u, barriers);
    const float depth_value[4] = {0.5f, 0.f, 0.f, 0.f};
    const float motion_value[4] = {0.f, 0.f, 0.f, 0.f};
    const float color[4] = {static_cast<float>(frame % 4u) / 4.f, 0.2f, 0.4f, 1.f};
    commands->ClearRenderTargetView(rtvs[0], depth_value, 0u, nullptr);
    commands->ClearRenderTargetView(rtvs[1], motion_value, 0u, nullptr);
    commands->ClearRenderTargetView(rtvs[index + 2u], color, 0u, nullptr);
    for (auto& barrier : barriers) std::swap(barrier.Transition.StateBefore, barrier.Transition.StateAfter);
    commands->ResourceBarrier(3u, barriers);
    const auto constants_result = runtime.set_constants(constants, *token, viewport);
    if (constants_result != sl::Result::eOk) {
      std::printf("constants failed: %d\n", static_cast<int>(constants_result));
      return 14;
    }
    const auto tag_result = runtime.set_tag(*token, viewport, tags, 3u, volatile_inputs ? commands.Get() : nullptr);
    if (tag_result != sl::Result::eOk) {
      std::printf("tags failed: %d\n", static_cast<int>(tag_result));
      return 15;
    }
    if (FAILED(commands->Close())) return 39;
    ID3D12CommandList* lists[] = {commands.Get()};
    queue->ExecuteCommandLists(1u, lists);
    if (FAILED(queue->Signal(fence.Get(), frame)) || FAILED(fence->SetEventOnCompletion(frame, event))
        || WaitForSingleObject(event, 5000u) != WAIT_OBJECT_0) return 40;
    if (enable_fg && marker(sl::PCLMarker::eRenderSubmitEnd, *token) != sl::Result::eOk) return 28;
    if (marker(sl::PCLMarker::ePresentStart, *token) != sl::Result::eOk) return 28;
    const bool present_succeeded = SUCCEEDED(swapchain->Present(0u, 0u));
    mode_state.PresentCompleted(present_succeeded);
    if (!present_succeeded) return 16;
    if (marker(sl::PCLMarker::ePresentEnd, *token) != sl::Result::eOk) return 29;
    if (enable_fg) {
      sl::DLSSGState state = {};
      if (get_state(viewport, state, nullptr) != sl::Result::eOk) return 30;
      presented_frames += state.numFramesActuallyPresented;
      last_status = static_cast<uint32_t>(state.status);
    }
    if (runtime.set_tag(*token, viewport, clear_tags, 3u, nullptr) != sl::Result::eOk) return 17;
    if (foreground) Sleep(16u);
  }
  if (enable_fg) {
    sl::DLSSGState state = {};
    const auto result = get_state(viewport, state, nullptr);
    std::printf("DLSS-G state=%d status=0x%X presented-total=%u for %u game frames max-generated=%u\n",
                static_cast<int>(result), last_status, presented_frames, total_frames, state.numFramesToGenerateMax);
    if (presented_frames <= total_frames) {
      std::puts("No extra frames were reported; this run did not verify frame generation.");
    }
    sl::DLSSGOptions options = {};
    options.mode = sl::DLSSGMode::eOff;
    if (set_options(viewport, options) != sl::Result::eOk || result != sl::Result::eOk) return 30;
    mode_state.OptionsAccepted(farcry6_dlss::frame_generation::ModeState::Mode::off);
    const bool present_succeeded = SUCCEEDED(swapchain->Present(0u, 0u));
    mode_state.PresentCompleted(present_succeeded);
    if (!present_succeeded) return 42;
  }
  if (FAILED(queue->Signal(fence.Get(), total_frames + 1u)) || FAILED(fence->SetEventOnCompletion(total_frames + 1u, event))
      || WaitForSingleObject(event, 5000u) != WAIT_OBJECT_0) return 41;
  CloseHandle(event);
  if (FAILED(allocator->Reset()) || FAILED(commands->Reset(allocator.Get(), nullptr)) || FAILED(commands->Close())) return 43;
  for (auto& buffer : backbuffers) buffer.Reset();
  if (!mode_state.ReadyForDisplayChange()) return 45;
  if (FAILED(swapchain->ResizeBuffers(2u, 800u, 600u, DXGI_FORMAT_R8G8B8A8_UNORM, 0u))
      || FAILED(swapchain->Present(0u, 0u))) return 44;
  depth.Reset();
  motion.Reset();
  swapchain.Reset();
  swapchain3.Reset();
  DestroyWindow(window);
  factory.Reset();
  queue.Reset();
  proxy_device.Reset();
  if (runtime.shutdown() != sl::Result::eOk) return 18;
  device.Reset();
  std::puts(enable_fg ? "Streamline input API test completed (FG enabled; inspect state above)"
                      : "Streamline frame-token, constants and input-tag smoke test passed (FG off)");
  return 0;
}
