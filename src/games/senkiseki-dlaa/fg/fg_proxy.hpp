#pragma once

#include <d3d11.h>
#include <d3d12.h>
#include <dxgi.h>
#include <dxgi1_2.h>
#include <dxgi1_3.h>
#include <dxgi1_4.h>
#include <dxgi1_5.h>
#include <dxgidebug.h>

#include <chrono>
#include <cstdint>
#include <cstdio>

#include <include/reshade.hpp>

#include "../../../utils/directx.hpp"

namespace senkiseki3::fg {

static constexpr int kFgProxyOk = 0;
static constexpr int kFgProxyAwaitStreamline = 20;
static constexpr int kFgProxyErrDirectX = -1;
static constexpr int kFgProxyErrAdapter = -2;
static constexpr int kFgProxyErrDevice12 = -3;
static constexpr int kFgProxyErrQueue = -4;
static constexpr int kFgProxyErrFence = -5;
static constexpr int kFgProxyErrEvent = -6;
static constexpr int kFgProxyErrSharedCreate = -10;
static constexpr int kFgProxyErrSharedHandle = -11;
static constexpr int kFgProxyErrSharedOpen = -12;
static constexpr int kFgProxyErrCopySetup = -13;
static constexpr int kFgProxyErrSyncTimeout = -14;
static constexpr int kFgProxyErrDeviceRemoved = -15;
static constexpr int kFgProxyErrUnsupportedFormat = -16;
static constexpr int kFgProxyErrHdrBlocked = -17;
static constexpr int kFgProxyErrHookInstall = -18;

static bool FgFormatSupported(DXGI_FORMAT fmt) {
  return fmt == DXGI_FORMAT_R8G8B8A8_UNORM || fmt == DXGI_FORMAT_R10G10B10A2_UNORM ||
         fmt == DXGI_FORMAT_R16G16B16A16_FLOAT;
}

struct FgChainObs {
  void* chain = nullptr;
  uint64_t count = 0u;
};

struct FgRuntime {
  ID3D11Device* d11 = nullptr;
  ID3D11DeviceContext* d11ctx = nullptr;
  ID3D12Device* device = nullptr;
  ID3D12CommandQueue* queue = nullptr;
  ID3D12Fence* fence = nullptr;
  HANDLE fence_event = nullptr;
  uint64_t fence_value = 1u;
  int proxy_code = 0;
  bool proxy_ok_logged = false;
  ID3D11Texture2D* shared11 = nullptr;
  ID3D12Resource* shared12 = nullptr;
  HANDLE shared_handle = nullptr;
  ID3D11Query* query = nullptr;
  ID3D12CommandAllocator* copy_alloc = nullptr;
  ID3D12GraphicsCommandList* copy_list = nullptr;
  ID3D12Resource* copy_dst = nullptr;
  uint32_t shared_w = 0u;
  uint32_t shared_h = 0u;
  DXGI_FORMAT shared_fmt = DXGI_FORMAT_UNKNOWN;
  uint64_t frames_tested = 0u;
  uint64_t last_test_code_frame = 0u;
  int last_test_code = 0;
  double last_test_ms = 0.0;
  uint64_t tick_count = 0u;
  uint64_t last_ensure_tick = 0u;
  double fps_ema = 0.0;
  uint64_t fps_pub_tick = 0u;
  bool torn_down = false;
  bool spike_on = false;
  bool prev_spike_on = false;
  bool hook_installed = false;
  bool hook_fatal = false;
  bool game_locked = false;
  void* game_chain = nullptr;
  void* proxy_chain = nullptr;
  IDXGISwapChain* proxy_swap = nullptr;
  IDXGISwapChain3* proxy_chain3 = nullptr;
  ID3D12DescriptorHeap* proxy_rtv_heap = nullptr;
  D3D12_CPU_DESCRIPTOR_HANDLE proxy_rtv[2] = {};
  uint32_t proxy_w = 0u;
  uint32_t proxy_h = 0u;
  bool proxy_pattern = true;
  uint64_t pattern_ok = 0u;
  ID3D11Resource* tick_bb = nullptr;
  DXGI_FORMAT tick_fmt = DXGI_FORMAT_UNKNOWN;
  uint32_t tick_w = 0u;
  uint32_t tick_h = 0u;
  uint64_t tick_seq = 0u;
  uint64_t consumed_seq = 0u;
  FgChainObs obs[8] = {};
  uint64_t hook_presents = 0u;
  uint64_t game_presents = 0u;
  uint64_t suppressed = 0u;
  uint64_t proxy_presents = 0u;
  uint64_t occluded = 0u;
  uint64_t dropped = 0u;
  uint64_t proxy_pub_count = 0u;
  double proxy_pub_ms = 0.0;
  int last_present_hr = 0;
  int last_hook_code = 0;
  HANDLE swap_thread = nullptr;
  HANDLE swap_work = nullptr;
  HANDLE swap_done = nullptr;
  HANDLE swap_exit = nullptr;
  bool swap_create_pending = false;
  HRESULT swap_create_hr = S_OK;
  HWND swap_req_hwnd = nullptr;
  uint32_t swap_req_w = 0u;
  uint32_t swap_req_h = 0u;
  int swap_req_factory = 0;
  int swap_req_variant = 0;
  int swap_step = 0;
  bool swap_latched = false;
  uint64_t last_swap_attempt_tick = 0u;
  IDXGISwapChain* alt_swap = nullptr;
  ID3D11RenderTargetView* alt_rtv = nullptr;
  uint32_t alt_w = 0u;
  uint32_t alt_h = 0u;
};

static void FgProxyRelease(FgRuntime* fg) {
  if (!fg) return;
  if (fg->alt_rtv) { fg->alt_rtv->Release(); fg->alt_rtv = nullptr; }
  if (fg->alt_swap) { fg->alt_swap->Release(); fg->alt_swap = nullptr; }
  fg->alt_w = 0u;
  fg->alt_h = 0u;
  if (fg->proxy_rtv_heap) { fg->proxy_rtv_heap->Release(); fg->proxy_rtv_heap = nullptr; }
  if (fg->swap_thread) {
    SetEvent(fg->swap_exit);
    WaitForSingleObject(fg->swap_thread, 1500u);
    CloseHandle(fg->swap_thread);
    fg->swap_thread = nullptr;
  }
  if (fg->swap_work) { CloseHandle(fg->swap_work); fg->swap_work = nullptr; }
  if (fg->swap_done) { CloseHandle(fg->swap_done); fg->swap_done = nullptr; }
  if (fg->swap_exit) { CloseHandle(fg->swap_exit); fg->swap_exit = nullptr; }
  fg->swap_create_pending = false;
  fg->swap_latched = false;
  fg->swap_step = 0;
  if (fg->proxy_rtv_heap) { fg->proxy_rtv_heap->Release(); fg->proxy_rtv_heap = nullptr; }
  if (fg->proxy_chain3) { fg->proxy_chain3->Release(); fg->proxy_chain3 = nullptr; }
  if (fg->proxy_swap) { fg->proxy_swap->Release(); fg->proxy_swap = nullptr; }
  fg->proxy_chain = nullptr;
  fg->proxy_w = 0u;
  fg->proxy_h = 0u;
  fg->game_chain = nullptr;
  fg->game_locked = false;
  fg->proxy_pattern = true;
  fg->pattern_ok = 0u;
  fg->tick_bb = nullptr;
  for (uint32_t i = 0u; i < 8u; ++i) { fg->obs[i].chain = nullptr; fg->obs[i].count = 0u; }
  if (fg->copy_list) { fg->copy_list->Release(); fg->copy_list = nullptr; }
  if (fg->copy_alloc) { fg->copy_alloc->Release(); fg->copy_alloc = nullptr; }
  if (fg->copy_dst) { fg->copy_dst->Release(); fg->copy_dst = nullptr; }
  if (fg->shared12) { fg->shared12->Release(); fg->shared12 = nullptr; }
  if (fg->shared_handle) { CloseHandle(fg->shared_handle); fg->shared_handle = nullptr; }
  if (fg->shared11) { fg->shared11->Release(); fg->shared11 = nullptr; }
  if (fg->query) { fg->query->Release(); fg->query = nullptr; }
  fg->shared_w = 0u;
  fg->shared_h = 0u;
  if (fg->fence) { fg->fence->Release(); fg->fence = nullptr; }
  if (fg->fence_event) { CloseHandle(fg->fence_event); fg->fence_event = nullptr; }
  if (fg->queue) { fg->queue->Release(); fg->queue = nullptr; }
  if (fg->device) { fg->device->Release(); fg->device = nullptr; }
  if (fg->d11ctx) { fg->d11ctx->Release(); fg->d11ctx = nullptr; }
  if (fg->d11) { fg->d11->Release(); fg->d11 = nullptr; }
  fg->fence_value = 1u;
  fg->proxy_ok_logged = false;
  fg->torn_down = true;
}

static void FgSharedRelease(FgRuntime* fg) {
  if (!fg) return;
  if (fg->copy_list) { fg->copy_list->Release(); fg->copy_list = nullptr; }
  if (fg->copy_alloc) { fg->copy_alloc->Release(); fg->copy_alloc = nullptr; }
  if (fg->copy_dst) { fg->copy_dst->Release(); fg->copy_dst = nullptr; }
  if (fg->shared12) { fg->shared12->Release(); fg->shared12 = nullptr; }
  if (fg->shared_handle) { CloseHandle(fg->shared_handle); fg->shared_handle = nullptr; }
  if (fg->shared11) { fg->shared11->Release(); fg->shared11 = nullptr; }
  if (fg->query) { fg->query->Release(); fg->query = nullptr; }
  fg->shared_w = 0u;
  fg->shared_h = 0u;
}

static bool FgProxyEnsure(ID3D11Device* d11, FgRuntime* fg) {
  if (!d11 || !fg) return false;
  if (!fg->d11) {
    d11->AddRef();
    fg->d11 = d11;
    d11->GetImmediateContext(&fg->d11ctx);
  }
  fg->torn_down = false;
  if (fg->device) return true;
  char line[192];
  if (!renodx::utils::directx::Initialize() ||
      !renodx::utils::directx::pD3D12CreateDevice) {
    fg->proxy_code = kFgProxyErrDirectX;
    snprintf(line, sizeof(line), "[DLAA] FG proxy: DirectX loader unavailable");
    reshade::log::message(reshade::log::level::error, line);
    return false;
  }
  IDXGIDevice* dxgi_dev = nullptr;
  if (FAILED(d11->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgi_dev)))) {
    fg->proxy_code = kFgProxyErrAdapter;
    snprintf(line, sizeof(line), "[DLAA] FG proxy: game device has no IDXGIDevice");
    reshade::log::message(reshade::log::level::error, line);
    return false;
  }
  IDXGIAdapter* adapter = nullptr;
  if (FAILED(dxgi_dev->GetAdapter(&adapter))) {
    dxgi_dev->Release();
    fg->proxy_code = kFgProxyErrAdapter;
    snprintf(line, sizeof(line), "[DLAA] FG proxy: IDXGIAdapter query failed");
    reshade::log::message(reshade::log::level::error, line);
    return false;
  }
  dxgi_dev->Release();
  HRESULT hr = renodx::utils::directx::pD3D12CreateDevice(
      adapter, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&fg->device));
  adapter->Release();
  if (FAILED(hr) || !fg->device) {
    fg->device = nullptr;
    fg->proxy_code = kFgProxyErrDevice12;
    snprintf(line, sizeof(line), "[DLAA] FG proxy: D3D12CreateDevice failed hr=0x%08lX",
             (unsigned long)hr);
    reshade::log::message(reshade::log::level::error, line);
    return false;
  }
  D3D12_COMMAND_QUEUE_DESC qd = {};
  qd.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
  qd.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
  qd.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
  qd.NodeMask = 0u;
  if (FAILED(fg->device->CreateCommandQueue(&qd, IID_PPV_ARGS(&fg->queue)))) {
    fg->proxy_code = kFgProxyErrQueue;
    snprintf(line, sizeof(line), "[DLAA] FG proxy: command queue creation failed");
    reshade::log::message(reshade::log::level::error, line);
    FgProxyRelease(fg);
    return false;
  }
  if (FAILED(fg->device->CreateFence(0u, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fg->fence)))) {
    fg->proxy_code = kFgProxyErrFence;
    snprintf(line, sizeof(line), "[DLAA] FG proxy: fence creation failed");
    reshade::log::message(reshade::log::level::error, line);
    FgProxyRelease(fg);
    return false;
  }
  fg->fence_event = CreateEventA(nullptr, FALSE, FALSE, nullptr);
  if (!fg->fence_event) {
    fg->proxy_code = kFgProxyErrEvent;
    snprintf(line, sizeof(line), "[DLAA] FG proxy: fence event creation failed");
    reshade::log::message(reshade::log::level::error, line);
    FgProxyRelease(fg);
    return false;
  }
  fg->fence_value = 1u;
  fg->proxy_code = kFgProxyOk;
  if (!fg->proxy_ok_logged) {
    fg->proxy_ok_logged = true;
    snprintf(line, sizeof(line), "[DLAA] FG proxy: D3D12 device + direct queue + fence ready");
    reshade::log::message(reshade::log::level::info, line);
  }
  return true;
}

static bool FgSharedEnsure(ID3D11Device* d11, FgRuntime* fg, uint32_t w, uint32_t h,
                           DXGI_FORMAT fmt) {
  if (!d11 || !fg || !fg->device) return false;
  if (!FgFormatSupported(fmt)) {
    fg->proxy_code = kFgProxyErrUnsupportedFormat;
    return false;
  }
  if (fg->shared11 && fg->shared_w == w && fg->shared_h == h && fg->shared_fmt == fmt) return true;
  FgSharedRelease(fg);
  char line[192];
  D3D11_TEXTURE2D_DESC td = {};
  td.Width = w;
  td.Height = h;
  td.MipLevels = 1u;
  td.ArraySize = 1u;
  td.Format = fmt;
  td.SampleDesc.Count = 1u;
  td.Usage = D3D11_USAGE_DEFAULT;
  td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
  td.MiscFlags = D3D11_RESOURCE_MISC_SHARED_NTHANDLE;
  const UINT bind_opts[3] = {D3D11_BIND_SHADER_RESOURCE, 0u, D3D11_BIND_SHADER_RESOURCE};
  const UINT misc_opts[3] = {D3D11_RESOURCE_MISC_SHARED_NTHANDLE,
                             D3D11_RESOURCE_MISC_SHARED_NTHANDLE, D3D11_RESOURCE_MISC_SHARED};
  HRESULT last_hr = S_OK;
  int attempt = -1;
  for (int i = 0; i < 3; ++i) {
    td.BindFlags = bind_opts[i];
    td.MiscFlags = misc_opts[i];
    last_hr = d11->CreateTexture2D(&td, nullptr, &fg->shared11);
    if (SUCCEEDED(last_hr) && fg->shared11) {
      attempt = i;
      break;
    }
    fg->shared11 = nullptr;
    snprintf(line, sizeof(line),
             "[DLAA] FG proxy: shared texture attempt %d bind=0x%X misc=0x%X fmt=%d %ux%u hr=0x%08lX",
             i, bind_opts[i], misc_opts[i], (int)fmt, w, h, (unsigned long)last_hr);
    reshade::log::message(reshade::log::level::warning, line);
  }
  if (attempt < 0) {
    fg->proxy_code = kFgProxyErrSharedCreate;
    snprintf(line, sizeof(line), "[DLAA] FG proxy: shared DX11 texture creation failed %ux%u fmt=%d",
             w, h, (int)fmt);
    reshade::log::message(reshade::log::level::error, line);
    return false;
  }
  if (attempt == 2) {
    IDXGIResource* res0 = nullptr;
    if (FAILED(fg->shared11->QueryInterface(__uuidof(IDXGIResource),
                                            reinterpret_cast<void**>(&res0)))) {
      fg->proxy_code = kFgProxyErrSharedHandle;
      snprintf(line, sizeof(line), "[DLAA] FG proxy: IDXGIResource query failed");
      reshade::log::message(reshade::log::level::error, line);
      FgSharedRelease(fg);
      return false;
    }
    if (FAILED(res0->GetSharedHandle(&fg->shared_handle)) || !fg->shared_handle) {
      res0->Release();
      fg->proxy_code = kFgProxyErrSharedHandle;
      snprintf(line, sizeof(line), "[DLAA] FG proxy: legacy shared handle query failed");
      reshade::log::message(reshade::log::level::error, line);
      FgSharedRelease(fg);
      return false;
    }
    res0->Release();
  } else {
  IDXGIResource1* res1 = nullptr;
  if (FAILED(fg->shared11->QueryInterface(__uuidof(IDXGIResource1),
                                          reinterpret_cast<void**>(&res1)))) {
    fg->proxy_code = kFgProxyErrSharedHandle;
    snprintf(line, sizeof(line), "[DLAA] FG proxy: IDXGIResource1 query failed");
    reshade::log::message(reshade::log::level::error, line);
    FgSharedRelease(fg);
    return false;
  }
  if (FAILED(res1->CreateSharedHandle(nullptr, DXGI_SHARED_RESOURCE_READ, nullptr,
                                      &fg->shared_handle))) {
    res1->Release();
    fg->proxy_code = kFgProxyErrSharedHandle;
    snprintf(line, sizeof(line), "[DLAA] FG proxy: NT shared handle creation failed");
    reshade::log::message(reshade::log::level::error, line);
    FgSharedRelease(fg);
    return false;
  }
  res1->Release();
  }
  if (FAILED(fg->device->OpenSharedHandle(fg->shared_handle, IID_PPV_ARGS(&fg->shared12)))) {
    fg->proxy_code = kFgProxyErrSharedOpen;
    snprintf(line, sizeof(line), "[DLAA] FG proxy: D3D12 OpenSharedHandle failed");
    reshade::log::message(reshade::log::level::error, line);
    FgSharedRelease(fg);
    return false;
  }
  D3D11_QUERY_DESC qd = {};
  qd.Query = D3D11_QUERY_EVENT;
  if (FAILED(d11->CreateQuery(&qd, &fg->query))) {
    fg->proxy_code = kFgProxyErrSharedCreate;
    snprintf(line, sizeof(line), "[DLAA] FG proxy: event query creation failed");
    reshade::log::message(reshade::log::level::error, line);
    FgSharedRelease(fg);
    return false;
  }
  if (FAILED(fg->device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                                IID_PPV_ARGS(&fg->copy_alloc)))) {
    fg->proxy_code = kFgProxyErrCopySetup;
    snprintf(line, sizeof(line), "[DLAA] FG proxy: copy allocator creation failed");
    reshade::log::message(reshade::log::level::error, line);
    FgSharedRelease(fg);
    return false;
  }
  if (FAILED(fg->device->CreateCommandList(0u, D3D12_COMMAND_LIST_TYPE_DIRECT, fg->copy_alloc,
                                            nullptr, IID_PPV_ARGS(&fg->copy_list)))) {
    fg->proxy_code = kFgProxyErrCopySetup;
    snprintf(line, sizeof(line), "[DLAA] FG proxy: copy command list creation failed");
    reshade::log::message(reshade::log::level::error, line);
    FgSharedRelease(fg);
    return false;
  }
  fg->copy_list->Close();
  D3D12_HEAP_PROPERTIES heap = {};
  heap.Type = D3D12_HEAP_TYPE_DEFAULT;
  D3D12_RESOURCE_DESC rd = {};
  rd.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
  rd.Width = w;
  rd.Height = h;
  rd.DepthOrArraySize = 1u;
  rd.MipLevels = 1u;
  rd.Format = fmt;
  rd.SampleDesc.Count = 1u;
  rd.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
  rd.Flags = D3D12_RESOURCE_FLAG_NONE;
  if (FAILED(fg->device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &rd,
                                                  D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                                  IID_PPV_ARGS(&fg->copy_dst)))) {
    fg->proxy_code = kFgProxyErrCopySetup;
    snprintf(line, sizeof(line), "[DLAA] FG proxy: copy destination creation failed");
    reshade::log::message(reshade::log::level::error, line);
    FgSharedRelease(fg);
    return false;
  }
  fg->shared_w = w;
  fg->shared_h = h;
  fg->shared_fmt = fmt;
  fg->proxy_code = kFgProxyOk;
  snprintf(line, sizeof(line), "[DLAA] FG proxy: shared frame path ready %ux%u fmt=%d attempt=%d",
           w, h, (int)fmt, attempt);
  reshade::log::message(reshade::log::level::info, line);
  return true;
}

static int FgSharedTest(ID3D11Device* d11, ID3D11DeviceContext* ctx, ID3D11Resource* src,
                        FgRuntime* fg, double* ms_out) {
  if (!d11 || !ctx || !src || !fg) return kFgProxyErrSharedCreate;
  if (!fg->shared11 || !fg->shared12 || !fg->query || !fg->copy_alloc || !fg->copy_list ||
      !fg->copy_dst || !fg->queue || !fg->fence || !fg->fence_event)
    return kFgProxyErrSharedCreate;
  const auto t0 = std::chrono::steady_clock::now();
  ctx->CopyResource(fg->shared11, src);
  ctx->Flush();
  ctx->End(fg->query);
  const auto deadline = t0 + std::chrono::milliseconds(10);
  BOOL done = FALSE;
  for (;;) {
    HRESULT ghr = ctx->GetData(fg->query, &done, sizeof(done), D3D11_ASYNC_GETDATA_DONOTFLUSH);
    if (ghr == S_OK && done) break;
    if (ghr != S_OK && ghr != S_FALSE) return kFgProxyErrSyncTimeout;
    if (std::chrono::steady_clock::now() >= deadline) return kFgProxyErrSyncTimeout;
  }
  if (FAILED(fg->copy_alloc->Reset())) return kFgProxyErrCopySetup;
  if (FAILED(fg->copy_list->Reset(fg->copy_alloc, nullptr))) return kFgProxyErrCopySetup;
  D3D12_RESOURCE_BARRIER pre = {};
  pre.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  pre.Transition.pResource = fg->shared12;
  pre.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
  pre.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
  pre.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  fg->copy_list->ResourceBarrier(1u, &pre);
  fg->copy_list->CopyResource(fg->copy_dst, fg->shared12);
  D3D12_RESOURCE_BARRIER post = pre;
  post.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
  post.Transition.StateAfter = D3D12_RESOURCE_STATE_COMMON;
  fg->copy_list->ResourceBarrier(1u, &post);
  if (FAILED(fg->copy_list->Close())) return kFgProxyErrCopySetup;
  ID3D12CommandList* lists[1] = {fg->copy_list};
  fg->queue->ExecuteCommandLists(1u, lists);
  const uint64_t v = fg->fence_value;
  if (FAILED(fg->queue->Signal(fg->fence, v))) return kFgProxyErrSyncTimeout;
  if (FAILED(fg->fence->SetEventOnCompletion(v, fg->fence_event))) return kFgProxyErrSyncTimeout;
  if (WaitForSingleObject(fg->fence_event, 250u) != WAIT_OBJECT_0) {
    if (fg->device->GetDeviceRemovedReason() != S_OK) {
      FgProxyRelease(fg);
      return kFgProxyErrDeviceRemoved;
    }
    return kFgProxyErrSyncTimeout;
  }
  fg->fence_value = v + 1u;
  fg->frames_tested++;
  if (ms_out) {
    *ms_out = std::chrono::duration<double, std::milli>(
                  std::chrono::steady_clock::now() - t0)
                  .count();
  }
  return kFgProxyOk;
}

static void* g_patch_slot[12] = {};
static void* g_patch_orig[12] = {};
static int g_patch_kind[12] = {};
static uint32_t g_patch_n = 0u;
static FgRuntime* g_hook_fg = nullptr;

static bool FgFnInDxgi(void* fn);

using FgPresentFn = HRESULT(STDMETHODCALLTYPE*)(IDXGISwapChain*, UINT, UINT);
using FgPresent1Fn = HRESULT(STDMETHODCALLTYPE*)(IDXGISwapChain*, UINT, UINT,
                                                 const DXGI_PRESENT_PARAMETERS*);

static void FgObsTouch(FgRuntime* fg, void* chain) {
  if (!fg || !chain) return;
  for (uint32_t i = 0u; i < 8u; ++i) {
    if (fg->obs[i].chain == chain) {
      fg->obs[i].count++;
      return;
    }
  }
  for (uint32_t i = 0u; i < 8u; ++i) {
    if (fg->obs[i].chain == nullptr) {
      fg->obs[i].chain = chain;
      fg->obs[i].count = 1u;
      return;
    }
  }
}

static void FgProxyFatal(FgRuntime* fg) {
  if (!fg) return;
  if (fg->alt_rtv) { fg->alt_rtv->Release(); fg->alt_rtv = nullptr; }
  if (fg->alt_swap) { fg->alt_swap->Release(); fg->alt_swap = nullptr; }
  fg->alt_w = 0u;
  fg->alt_h = 0u;
  if (fg->proxy_rtv_heap) { fg->proxy_rtv_heap->Release(); fg->proxy_rtv_heap = nullptr; }
  if (fg->proxy_chain3) { fg->proxy_chain3->Release(); fg->proxy_chain3 = nullptr; }
  if (fg->proxy_swap) { fg->proxy_swap->Release(); fg->proxy_swap = nullptr; }
  fg->proxy_chain = nullptr;
  fg->proxy_w = 0u;
  fg->proxy_h = 0u;
  FgSharedRelease(fg);
  fg->game_chain = nullptr;
  fg->game_locked = false;
  fg->hook_fatal = true;
  fg->proxy_code = kFgProxyErrDeviceRemoved;
  char line[128];
  snprintf(line, sizeof(line), "[DLAA] FG proxy: fatal proxy failure, spike inert until retoggled");
  reshade::log::message(reshade::log::level::error, line);
}

static int FgSuppressedFrame(FgRuntime* fg) {
  if (!fg || !fg->d11ctx || !fg->tick_bb) return -2;
  if (fg->tick_seq == fg->consumed_seq) return -3;
  fg->consumed_seq = fg->tick_seq;
  const uint32_t w = fg->tick_w;
  const uint32_t h = fg->tick_h;
  const DXGI_FORMAT fmt = fg->tick_fmt;
  if (w < 64u || h < 64u || fmt != DXGI_FORMAT_R8G8B8A8_UNORM) return -4;
  if (!FgSharedEnsure(fg->d11, fg, w, h, fmt)) return -5;
  ID3D11DeviceContext* ctx = fg->d11ctx;
  ctx->CopyResource(fg->shared11, fg->tick_bb);
  ctx->Flush();
  if (fg->query) {
    ctx->End(fg->query);
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(10);
    BOOL done = FALSE;
    for (;;) {
      HRESULT ghr = ctx->GetData(fg->query, &done, sizeof(done), D3D11_ASYNC_GETDATA_DONOTFLUSH);
      if (ghr == S_OK && done) break;
      if (ghr != S_OK && ghr != S_FALSE) return -6;
      if (std::chrono::steady_clock::now() >= deadline) return -6;
    }
  }
  HRESULT phr = S_OK;
  if (fg->proxy_swap && fg->queue && fg->fence && fg->fence_event && fg->copy_alloc &&
      fg->copy_list) {
    if (FAILED(fg->copy_alloc->Reset())) return -7;
    if (FAILED(fg->copy_list->Reset(fg->copy_alloc, nullptr))) return -7;
    uint32_t idx = 0u;
    if (fg->proxy_chain3) idx = fg->proxy_chain3->GetCurrentBackBufferIndex();
    if (idx > 1u) idx = 0u;
    ID3D12Resource* pbuf = nullptr;
    if (FAILED(fg->proxy_swap->GetBuffer(idx, IID_PPV_ARGS(&pbuf)))) return -8;
    if (fg->proxy_pattern) {
      D3D12_RESOURCE_BARRIER b = {};
      b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
      b.Transition.pResource = pbuf;
      b.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
      b.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
      b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
      fg->copy_list->ResourceBarrier(1u, &b);
      float c[4] = {0.f, 0.f, 0.f, 1.f};
      c[(fg->proxy_presents / 60u) % 3u] = 1.f;
      fg->copy_list->ClearRenderTargetView(fg->proxy_rtv[idx], c, 0u, nullptr);
      b.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
      b.Transition.StateAfter = D3D12_RESOURCE_STATE_COMMON;
      fg->copy_list->ResourceBarrier(1u, &b);
    } else {
      D3D12_RESOURCE_BARRIER bs[2] = {};
      bs[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
      bs[0].Transition.pResource = fg->shared12;
      bs[0].Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
      bs[0].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
      bs[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
      bs[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
      bs[1].Transition.pResource = pbuf;
      bs[1].Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
      bs[1].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
      bs[1].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
      fg->copy_list->ResourceBarrier(2u, bs);
      fg->copy_list->CopyResource(pbuf, fg->shared12);
      bs[0].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
      bs[0].Transition.StateAfter = D3D12_RESOURCE_STATE_COMMON;
      bs[1].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
      bs[1].Transition.StateAfter = D3D12_RESOURCE_STATE_COMMON;
      fg->copy_list->ResourceBarrier(2u, bs);
    }
    if (FAILED(fg->copy_list->Close())) {
      pbuf->Release();
      return -7;
    }
    ID3D12CommandList* lists[1] = {fg->copy_list};
    fg->queue->ExecuteCommandLists(1u, lists);
    pbuf->Release();
    const uint64_t v = fg->fence_value;
    if (FAILED(fg->queue->Signal(fg->fence, v))) return -9;
    phr = fg->proxy_swap->Present(1u, 0u);
    if (FAILED(fg->fence->SetEventOnCompletion(v, fg->fence_event))) return -9;
    if (WaitForSingleObject(fg->fence_event, 250u) != WAIT_OBJECT_0) {
      if (fg->device->GetDeviceRemovedReason() != S_OK) {
        FgProxyFatal(fg);
        return -15;
      }
      return -10;
    }
    fg->fence_value = v + 1u;
  } else if (fg->alt_swap && fg->alt_rtv) {
    if (fg->proxy_pattern) {
      float c[4] = {0.f, 0.f, 0.f, 1.f};
      c[(fg->proxy_presents / 60u) % 3u] = 1.f;
      ctx->ClearRenderTargetView(fg->alt_rtv, c);
    } else {
      ID3D11Texture2D* abuf = nullptr;
      if (FAILED(fg->alt_swap->GetBuffer(0u, IID_PPV_ARGS(&abuf)))) return -8;
      ctx->CopyResource(abuf, fg->shared11);
      abuf->Release();
    }
    phr = fg->alt_swap->Present(0u, 0u);
  } else {
    return -2;
  }
  fg->last_present_hr = (int)phr;
  fg->proxy_presents++;
  if (phr == DXGI_STATUS_OCCLUDED) {
    fg->occluded++;
    return 1;
  }
  if (FAILED(phr)) {
    if (phr == DXGI_ERROR_DEVICE_REMOVED || phr == DXGI_ERROR_DEVICE_RESET) {
      FgProxyFatal(fg);
      return -15;
    }
    return -11;
  }
  if (fg->proxy_pattern) {
    fg->pattern_ok++;
    if (fg->pattern_ok >= 180u) {
      fg->proxy_pattern = false;
      char line[128];
      snprintf(line, sizeof(line), "[DLAA] FG proxy: pattern stable, switching to real frame");
      reshade::log::message(reshade::log::level::info, line);
    }
  }
  return 0;
}

static HRESULT FgHookCommon(uint32_t idx, IDXGISwapChain* self, bool is_p1, UINT sa, UINT fb,
                            const DXGI_PRESENT_PARAMETERS* pp) {
  FgRuntime* fg = g_hook_fg;
  void* orig = (idx < 12u) ? g_patch_orig[idx] : nullptr;
  const int kind = (idx < 12u) ? g_patch_kind[idx] : 0;
  (void)is_p1;
  if (!fg || fg->torn_down || !orig) {
    if (!orig) return S_OK;
    if (kind == 1) return ((FgPresent1Fn)orig)(self, sa, fb, pp);
    return ((FgPresentFn)orig)(self, sa, fb);
  }
  fg->hook_presents++;
  FgObsTouch(fg, self);
  if (self == (IDXGISwapChain*)fg->proxy_chain || self == (IDXGISwapChain*)fg->alt_swap) {
    if (kind == 1) return ((FgPresent1Fn)orig)(self, sa, fb, pp);
    return ((FgPresentFn)orig)(self, sa, fb);
  }
  if (fg->spike_on && !fg->hook_fatal && fg->game_locked &&
      self == (IDXGISwapChain*)fg->game_chain) {
    fg->game_presents++;
    const int rc = FgSuppressedFrame(fg);
    if (rc == 0 || rc == 1) {
      fg->suppressed++;
      return S_OK;
    }
    fg->dropped++;
    if (rc != fg->last_hook_code) {
      fg->last_hook_code = rc;
      char line[128];
      snprintf(line, sizeof(line), "[DLAA] FG proxy: suppressed frame failed code=%d, fail-open", rc);
      reshade::log::message(reshade::log::level::warning, line);
    }
  }
  if (kind == 1) return ((FgPresent1Fn)orig)(self, sa, fb, pp);
  return ((FgPresentFn)orig)(self, sa, fb);
}

template <int I>
static HRESULT STDMETHODCALLTYPE FgPresentHook(IDXGISwapChain* self, UINT sa, UINT fb) {
  return FgHookCommon((uint32_t)I, self, false, sa, fb, nullptr);
}

template <int I>
static HRESULT STDMETHODCALLTYPE FgPresent1Hook(IDXGISwapChain* self, UINT sa, UINT fb,
                                                const DXGI_PRESENT_PARAMETERS* pp) {
  return FgHookCommon((uint32_t)I, self, true, sa, fb, pp);
}

static void* const kFgPresentHooks[12] = {
    (void*)&FgPresentHook<0>, (void*)&FgPresentHook<1>, (void*)&FgPresentHook<2>,
    (void*)&FgPresentHook<3>, (void*)&FgPresentHook<4>, (void*)&FgPresentHook<5>,
    (void*)&FgPresentHook<6>, (void*)&FgPresentHook<7>, (void*)&FgPresentHook<8>,
    (void*)&FgPresentHook<9>, (void*)&FgPresentHook<10>, (void*)&FgPresentHook<11>};
static void* const kFgPresent1Hooks[12] = {
    (void*)&FgPresent1Hook<0>, (void*)&FgPresent1Hook<1>, (void*)&FgPresent1Hook<2>,
    (void*)&FgPresent1Hook<3>, (void*)&FgPresent1Hook<4>, (void*)&FgPresent1Hook<5>,
    (void*)&FgPresent1Hook<6>, (void*)&FgPresent1Hook<7>, (void*)&FgPresent1Hook<8>,
    (void*)&FgPresent1Hook<9>, (void*)&FgPresent1Hook<10>, (void*)&FgPresent1Hook<11>};

static bool FgPatchSlot(void** vt, uint32_t slot, void* hookfn, int kind) {
  if (!vt || !hookfn) return false;
  void** pslot = vt + slot;
  for (uint32_t i = 0u; i < g_patch_n; ++i)
    if (g_patch_slot[i] == (void*)pslot) return true;
  if (g_patch_n >= 12u) return false;
  void* orig = *pslot;
  if (!FgFnInDxgi(orig)) {
    char line[128];
    snprintf(line, sizeof(line), "[DLAA] FG proxy: skipping non-DXGI table slot %u", slot);
    reshade::log::message(reshade::log::level::info, line);
    return false;
  }
  DWORD old = 0u;
  if (!VirtualProtect(pslot, sizeof(void*), PAGE_READWRITE, &old)) return false;
  *pslot = hookfn;
  VirtualProtect(pslot, sizeof(void*), old, &old);
  g_patch_slot[g_patch_n] = (void*)pslot;
  g_patch_orig[g_patch_n] = orig;
  g_patch_kind[g_patch_n] = kind;
  g_patch_n++;
  return orig != nullptr;
}

static bool FgHookEnsure(FgRuntime* fg) {
  if (!fg || fg->hook_installed || (!fg->proxy_swap && !fg->alt_swap))
    return fg && fg->hook_installed;
  char line[192];
  IDXGISwapChain* src = fg->proxy_swap ? fg->proxy_swap : fg->alt_swap;
  IUnknown* qis[5] = {};
  int vers[5] = {};
  uint32_t nq = 0u;
  if (SUCCEEDED(src->QueryInterface(__uuidof(IDXGISwapChain4), (void**)&qis[nq]))) {
    vers[nq] = 4;
    nq++;
  }
  if (SUCCEEDED(src->QueryInterface(__uuidof(IDXGISwapChain3), (void**)&qis[nq]))) {
    vers[nq] = 3;
    nq++;
  }
  if (SUCCEEDED(src->QueryInterface(__uuidof(IDXGISwapChain2), (void**)&qis[nq]))) {
    vers[nq] = 2;
    nq++;
  }
  if (SUCCEEDED(src->QueryInterface(__uuidof(IDXGISwapChain1), (void**)&qis[nq]))) {
    vers[nq] = 1;
    nq++;
  }
  qis[nq] = (IUnknown*)src;
  src->AddRef();
  vers[nq] = 0;
  nq++;
  void* vts[6] = {};
  int vtver[6] = {};
  uint32_t nv = 0u;
  for (uint32_t i = 0u; i < nq; ++i) {
    void** vt = *(void***)qis[i];
    qis[i]->Release();
    int found = -1;
    for (uint32_t j = 0u; j < nv; ++j)
      if (vts[j] == (void*)vt) found = (int)j;
    if (found >= 0) {
      if (vers[i] > vtver[found]) vtver[found] = vers[i];
    } else if (nv < 6u) {
      vts[nv] = (void*)vt;
      vtver[nv] = vers[i];
      nv++;
    }
  }
  uint32_t patched = 0u;
  uint32_t pi = 0u;
  for (uint32_t i = 0u; i < nv; ++i) {
    void** vt = (void**)vts[i];
    if (FgPatchSlot(vt, 8u, kFgPresentHooks[pi % 12u], 0)) {
      patched++;
      pi++;
    }
    if (vtver[i] >= 1 && FgPatchSlot(vt, 18u, kFgPresent1Hooks[pi % 12u], 1)) {
      patched++;
      pi++;
    }
  }
  if (patched == 0u) {
    fg->proxy_code = kFgProxyErrHookInstall;
    snprintf(line, sizeof(line), "[DLAA] FG proxy: Present hook install failed");
    reshade::log::message(reshade::log::level::error, line);
    return false;
  }
  g_hook_fg = fg;
  fg->hook_installed = true;
  snprintf(line, sizeof(line), "[DLAA] FG proxy: Present hook installed (%u slots), observing", patched);
  reshade::log::message(reshade::log::level::info, line);
  return true;
}

static void FgHookEvaluateLock(FgRuntime* fg) {
  if (!fg || fg->game_locked || (!fg->proxy_chain && !fg->alt_swap)) return;
  uint64_t total = 0u, best = 0u, second = 0u;
  void* best_ptr = nullptr;
  for (uint32_t i = 0u; i < 8u; ++i) {
    if (!fg->obs[i].chain || fg->obs[i].chain == fg->proxy_chain ||
        fg->obs[i].chain == (void*)fg->alt_swap)
      continue;
    total += fg->obs[i].count;
    if (fg->obs[i].count > best) {
      second = best;
      best = fg->obs[i].count;
      best_ptr = fg->obs[i].chain;
    } else if (fg->obs[i].count > second) {
      second = fg->obs[i].count;
    }
  }
  if (total < 90u || best < 60u) return;
  if (best > 0u && second * 4u >= best) {
    if (fg->tick_count % 300u == 0u) {
      char line[128];
      snprintf(line, sizeof(line), "[DLAA] FG proxy: present lock ambiguous, waiting");
      reshade::log::message(reshade::log::level::info, line);
    }
    return;
  }
  fg->game_chain = best_ptr;
  fg->game_locked = true;
  char line[160];
  snprintf(line, sizeof(line), "[DLAA] FG proxy: game present chain locked, suppressing");
  reshade::log::message(reshade::log::level::info, line);
}

static bool FgProxyRtvEnsure(FgRuntime* fg) {
  if (!fg || !fg->device || !fg->proxy_swap) return false;
  if (fg->proxy_rtv_heap) { fg->proxy_rtv_heap->Release(); fg->proxy_rtv_heap = nullptr; }
  D3D12_DESCRIPTOR_HEAP_DESC hd = {};
  hd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
  hd.NumDescriptors = 2u;
  if (FAILED(fg->device->CreateDescriptorHeap(&hd, IID_PPV_ARGS(&fg->proxy_rtv_heap)))) return false;
  const uint32_t inc = fg->device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
  D3D12_CPU_DESCRIPTOR_HANDLE base = fg->proxy_rtv_heap->GetCPUDescriptorHandleForHeapStart();
  for (uint32_t i = 0u; i < 2u; ++i) {
    ID3D12Resource* buf = nullptr;
    if (FAILED(fg->proxy_swap->GetBuffer(i, IID_PPV_ARGS(&buf)))) {
      fg->proxy_rtv_heap->Release();
      fg->proxy_rtv_heap = nullptr;
      return false;
    }
    D3D12_CPU_DESCRIPTOR_HANDLE h = {base.ptr + (SIZE_T)i * inc};
    fg->device->CreateRenderTargetView(buf, nullptr, h);
    buf->Release();
    fg->proxy_rtv[i] = h;
  }
  return true;
}

static bool FgFnInDxgi(void* fn) {
  static HMODULE dxgi_mod = nullptr;
  if (!dxgi_mod) dxgi_mod = GetModuleHandleW(L"dxgi.dll");
  if (!dxgi_mod || !fn) return false;
  MEMORY_BASIC_INFORMATION mbi = {};
  if (!VirtualQuery(fn, &mbi, sizeof(mbi))) return false;
  return mbi.AllocationBase == (void*)dxgi_mod;
}

static IDXGIInfoQueue* FgDxgiCaptureBegin();
static void FgDxgiCaptureEnd(IDXGIInfoQueue* q, const char* tag);

static HRESULT FgAcquireFactory(FgRuntime* fg, int factory_mode, IDXGIFactory2** out) {  if (!fg || !out) return E_FAIL;
  *out = nullptr;
  if (factory_mode == 2) {
    if (!renodx::utils::directx::Initialize() || !renodx::utils::directx::pCreateDXGIFactory1)
      return E_FAIL;
    return renodx::utils::directx::pCreateDXGIFactory1(IID_PPV_ARGS(out));
  }
  if (factory_mode == 1 && fg->d11) {
    IDXGIDevice* dxdev = nullptr;
    IDXGIAdapter* ad = nullptr;
    HRESULT hr = fg->d11->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxdev);
    if (SUCCEEDED(hr)) hr = dxdev->GetAdapter(&ad);
    if (SUCCEEDED(hr)) hr = ad->GetParent(__uuidof(IDXGIFactory2), (void**)out);
    if (ad) ad->Release();
    if (dxdev) dxdev->Release();
    return hr;
  }
  if (!renodx::utils::directx::Initialize() || !renodx::utils::directx::pCreateDXGIFactory2)
    return E_FAIL;
  return renodx::utils::directx::pCreateDXGIFactory2(0u, IID_PPV_ARGS(out));
}

static const GUID kFgUnwrapped = {0x7f2c9a11, 0x3b4e, 0x4d6a, {0x81, 0x2f, 0x5e, 0x9c, 0xd3, 0x7a,
                                                              0x1b, 0x42}};

static HRESULT FgAltCreateSync(FgRuntime* fg, HWND hwnd, uint32_t w, uint32_t h, int factory_mode) {
  if (!fg || !fg->d11 || !hwnd || w < 64u || h < 64u) return E_INVALIDARG;
  if (fg->alt_rtv) { fg->alt_rtv->Release(); fg->alt_rtv = nullptr; }
  if (fg->alt_swap) { fg->alt_swap->Release(); fg->alt_swap = nullptr; }
  fg->alt_w = 0u;
  fg->alt_h = 0u;
  IDXGIFactory2* factory = nullptr;
  HRESULT fhr = FgAcquireFactory(fg, factory_mode, &factory);
  if (FAILED(fhr) || !factory) return FAILED(fhr) ? fhr : E_FAIL;
  IDXGIFactory* basef = nullptr;
  fhr = factory->QueryInterface(IID_PPV_ARGS(&basef));
  factory->Release();
  if (FAILED(fhr) || !basef) return FAILED(fhr) ? fhr : E_FAIL;
  DXGI_SWAP_CHAIN_DESC sd = {};
  sd.BufferDesc.Width = w;
  sd.BufferDesc.Height = h;
  sd.BufferDesc.RefreshRate.Numerator = 0u;
  sd.BufferDesc.RefreshRate.Denominator = 0u;
  sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
  sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
  sd.SampleDesc.Count = 1u;
  sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  sd.BufferCount = 2u;
  sd.OutputWindow = hwnd;
  sd.Windowed = TRUE;
  sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
  sd.Flags = 0u;
  IDXGISwapChain* created = nullptr;
  IDXGIInfoQueue* iq = FgDxgiCaptureBegin();
  HRESULT chr = basef->CreateSwapChain((IUnknown*)fg->d11, &sd, &created);
  if (FAILED(chr)) FgDxgiCaptureEnd(iq, "alt-create");
  else if (iq) iq->Release();
  basef->Release();
  if (FAILED(chr) || !created) return chr;
  IUnknown* real = nullptr;
  if (FAILED(created->QueryInterface(kFgUnwrapped, (void**)&real)) || !real) real = nullptr;
  IDXGISwapChain* base = nullptr;
  if (real) {
    real->QueryInterface(IID_PPV_ARGS(&base));
    real->Release();
  }
  if (!base) created->QueryInterface(IID_PPV_ARGS(&base));
  created->Release();
  if (!base) return E_FAIL;
  ID3D11Texture2D* abuf = nullptr;
  if (FAILED(base->GetBuffer(0u, IID_PPV_ARGS(&abuf)))) {
    base->Release();
    return E_FAIL;
  }
  ID3D11RenderTargetView* rtv = nullptr;
  if (FAILED(fg->d11->CreateRenderTargetView(abuf, nullptr, &rtv))) {
    abuf->Release();
    base->Release();
    return E_FAIL;
  }
  abuf->Release();
  fg->alt_swap = base;
  fg->alt_rtv = rtv;
  fg->alt_w = w;
  fg->alt_h = h;
  return S_OK;
}

static void FgWorkerCreateSwap(FgRuntime* fg);

static DWORD WINAPI FgSwapThreadProc(LPVOID p) {
  FgRuntime* fg = (FgRuntime*)p;
  HANDLE ev[2] = {fg->swap_work, fg->swap_exit};
  for (;;) {
    DWORD wr = WaitForMultipleObjects(2u, ev, FALSE, INFINITE);
    if (wr != WAIT_OBJECT_0) return 0u;
    FgWorkerCreateSwap(fg);
    SetEvent(fg->swap_done);
  }
}

static const GUID kFgDxgiDebugAll = {0xe48ae283, 0xda80, 0x490b, {0x87, 0xe6, 0x43, 0xe9, 0xa9,
                                                                      0xcf, 0xda, 0x8}};
static HMODULE g_dxgi_debug_mod = nullptr;

static IDXGIInfoQueue* FgDxgiCaptureBegin() {
  if (!g_dxgi_debug_mod) g_dxgi_debug_mod = LoadLibraryW(L"dxgidebug.dll");
  if (!g_dxgi_debug_mod) return nullptr;
  typedef HRESULT(WINAPI* PfnGetDebug)(UINT, REFIID, void**);
  PfnGetDebug pget =
      (PfnGetDebug)GetProcAddress(g_dxgi_debug_mod, "DXGIGetDebugInterface1");
  if (!pget) return nullptr;
  IDXGIInfoQueue* q = nullptr;
  if (FAILED(pget(0u, __uuidof(IDXGIInfoQueue), (void**)&q)) || !q) return nullptr;
  q->ClearStoredMessages(kFgDxgiDebugAll);
  return q;
}

static void FgDxgiCaptureEnd(IDXGIInfoQueue* q, const char* tag) {
  if (!q) return;
  UINT64 n = q->GetNumStoredMessagesAllowedByRetrievalFilters(kFgDxgiDebugAll);
  UINT64 start = n > 6u ? n - 6u : 0u;
  for (UINT64 i = start; i < n; ++i) {
    SIZE_T len = 0u;
    if (FAILED(q->GetMessage(kFgDxgiDebugAll, i, nullptr, &len)) || len < sizeof(DXGI_INFO_QUEUE_MESSAGE))
      continue;
    if (len > 512u) len = 512u;
    uint8_t* buf = new uint8_t[(size_t)len];
    if (!buf) continue;
    DXGI_INFO_QUEUE_MESSAGE* msg = (DXGI_INFO_QUEUE_MESSAGE*)buf;
    if (SUCCEEDED(q->GetMessage(kFgDxgiDebugAll, i, msg, &len)) && msg->DescriptionByteLength > 0u) {
      char line[256];
      int w = snprintf(line, sizeof(line), "[DLAA] FG proxy: DXGI %s: ", tag);
      if (w < 0) w = 0;
      size_t copy = (size_t)msg->DescriptionByteLength;
      if (copy > sizeof(line) - (size_t)w - 1u) copy = sizeof(line) - (size_t)w - 1u;
      memcpy(line + w, msg->pDescription, copy);
      line[w + copy] = 0;
      for (size_t k = (size_t)w; line[k]; ++k)
        if ((unsigned char)line[k] < 32u || (unsigned char)line[k] > 126u) line[k] = '?';
      reshade::log::message(reshade::log::level::warning, line);
    }
    delete[] buf;
  }
  char summary[128];
  snprintf(summary, sizeof(summary), "[DLAA] FG proxy: DXGI %s: %llu message(s)", tag,
           (unsigned long long)n);
  reshade::log::message(reshade::log::level::info, summary);
  q->Release();
}

static HRESULT FgSwapCreateSync(FgRuntime* fg, HWND hwnd, uint32_t w, uint32_t h, int factory_mode,
                                int variant) {
  if (!fg || !fg->device || !fg->queue) return E_FAIL;
  if (!hwnd || w < 64u || h < 64u) return E_INVALIDARG;
  if (fg->proxy_rtv_heap) { fg->proxy_rtv_heap->Release(); fg->proxy_rtv_heap = nullptr; }
  IDXGIFactory2* factory = nullptr;
  HRESULT fhr = E_FAIL;
  DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsdesc = {};
  const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* pfs = nullptr;
  DXGI_SCALING scaling = DXGI_SCALING_STRETCH;
  if (factory_mode == 2) {
    if (renodx::utils::directx::Initialize() && renodx::utils::directx::pCreateDXGIFactory1)
      fhr = renodx::utils::directx::pCreateDXGIFactory1(IID_PPV_ARGS(&factory));
    fsdesc.RefreshRate.Numerator = 0u;
    fsdesc.RefreshRate.Denominator = 0u;
    fsdesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    fsdesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    fsdesc.Windowed = TRUE;
    pfs = &fsdesc;
    scaling = DXGI_SCALING_NONE;
  } else if (factory_mode == 1 && fg->d11) {
    IDXGIDevice* dxdev = nullptr;
    IDXGIAdapter* ad = nullptr;
    if (SUCCEEDED(fg->d11->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxdev))) {
      if (SUCCEEDED(dxdev->GetAdapter(&ad))) {
        fhr = ad->GetParent(__uuidof(IDXGIFactory2), (void**)&factory);
        ad->Release();
      }
      dxdev->Release();
    }
  } else {
    if (renodx::utils::directx::Initialize() && renodx::utils::directx::pCreateDXGIFactory2)
      fhr = renodx::utils::directx::pCreateDXGIFactory2(0u, IID_PPV_ARGS(&factory));
  }
  if (FAILED(fhr) || !factory) return FAILED(fhr) ? fhr : E_FAIL;
  DXGI_SWAP_CHAIN_DESC1 sd = {};
  sd.Width = w;
  sd.Height = h;
  sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  sd.SampleDesc.Count = 1u;
  sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  sd.BufferCount = (variant == 2) ? 3u : 2u;
  sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
  sd.Flags = (variant == 1) ? 0u : DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
  sd.Scaling = scaling;
  sd.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
  IDXGISwapChain1* sc1 = nullptr;
  IDXGIInfoQueue* iq = FgDxgiCaptureBegin();
  HRESULT chr = factory->CreateSwapChainForHwnd((IUnknown*)fg->queue, hwnd, &sd, pfs, nullptr,
                                                &sc1);
  if (FAILED(chr)) FgDxgiCaptureEnd(iq, "create");
  else if (iq) iq->Release();
  factory->Release();
  if (FAILED(chr) || !sc1) return chr;
  static const GUID kUnwrapped = {0x7f2c9a11, 0x3b4e, 0x4d6a, {0x81, 0x2f, 0x5e, 0x9c, 0xd3, 0x7a,
                                                                0x1b, 0x42}};
  IUnknown* real = nullptr;
  if (FAILED(sc1->QueryInterface(kUnwrapped, (void**)&real)) || !real) real = nullptr;
  IDXGISwapChain* base = nullptr;
  if (real) {
    real->QueryInterface(IID_PPV_ARGS(&base));
    real->Release();
  }
  if (!base) sc1->QueryInterface(IID_PPV_ARGS(&base));
  sc1->Release();
  if (!base) return E_FAIL;
  fg->proxy_swap = base;
  fg->proxy_chain = (void*)base;
  fg->proxy_chain3 = nullptr;
  base->QueryInterface(IID_PPV_ARGS(&fg->proxy_chain3));
  if (!FgProxyRtvEnsure(fg)) {
    if (fg->proxy_chain3) { fg->proxy_chain3->Release(); fg->proxy_chain3 = nullptr; }
    fg->proxy_swap->Release();
    fg->proxy_swap = nullptr;
    fg->proxy_chain = nullptr;
    return E_FAIL;
  }
  fg->proxy_w = w;
  fg->proxy_h = h;
  fg->proxy_pattern = true;
  fg->pattern_ok = 0u;
  return S_OK;
}

static void FgWorkerCreateSwap(FgRuntime* fg) {
  fg->swap_create_hr = E_FAIL;
  if (!fg) return;
  if (fg->swap_req_variant == 3) {
    if (!fg->d11) return;
    fg->swap_create_hr =
        FgAltCreateSync(fg, fg->swap_req_hwnd, fg->swap_req_w, fg->swap_req_h, fg->swap_req_factory);
    return;
  }
  if (!fg->device || !fg->queue) return;
  fg->swap_create_hr = FgSwapCreateSync(fg, fg->swap_req_hwnd, fg->swap_req_w, fg->swap_req_h,
                                        fg->swap_req_factory, fg->swap_req_variant);
}

static bool FgProxySwapEnsure(FgRuntime* fg, HWND hwnd, uint32_t w, uint32_t h) {
  if (!fg || !fg->device || !fg->queue || !hwnd || w < 64u || h < 64u) return false;
  if (fg->alt_swap && (fg->alt_w != w || fg->alt_h != h)) {
    if (fg->alt_rtv) { fg->alt_rtv->Release(); fg->alt_rtv = nullptr; }
    if (fg->alt_swap) { fg->alt_swap->Release(); fg->alt_swap = nullptr; }
    fg->alt_w = 0u;
    fg->alt_h = 0u;
    fg->swap_step = 5;
    fg->swap_latched = false;
  }
  if (fg->alt_swap && fg->alt_w == w && fg->alt_h == h) return true;
  if (fg->proxy_swap && fg->proxy_w == w && fg->proxy_h == h) return true;
  char line[192];
  if (fg->proxy_swap) {
    if (fg->proxy_rtv_heap) { fg->proxy_rtv_heap->Release(); fg->proxy_rtv_heap = nullptr; }
    HRESULT rhr = fg->proxy_swap->ResizeBuffers(2u, w, h, DXGI_FORMAT_R8G8B8A8_UNORM,
                                                DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING);
    if (SUCCEEDED(rhr) && FgProxyRtvEnsure(fg)) {
      fg->proxy_w = w;
      fg->proxy_h = h;
      snprintf(line, sizeof(line), "[DLAA] FG proxy: swapchain resized %ux%u", w, h);
      reshade::log::message(reshade::log::level::info, line);
      return true;
    }
    if (fg->proxy_chain3) { fg->proxy_chain3->Release(); fg->proxy_chain3 = nullptr; }
    if (fg->proxy_swap) { fg->proxy_swap->Release(); fg->proxy_swap = nullptr; }
    fg->proxy_chain = nullptr;
    fg->proxy_w = 0u;
    fg->proxy_h = 0u;
  }
  if (fg->swap_latched || fg->hook_fatal || fg->swap_create_pending) return false;
  if (!fg->swap_thread) {
    fg->swap_work = CreateEventA(nullptr, FALSE, FALSE, nullptr);
    fg->swap_done = CreateEventA(nullptr, FALSE, FALSE, nullptr);
    fg->swap_exit = CreateEventA(nullptr, TRUE, FALSE, nullptr);
    if (!fg->swap_work || !fg->swap_done || !fg->swap_exit) return false;
    fg->swap_thread = CreateThread(nullptr, 0u, FgSwapThreadProc, fg, 0u, nullptr);
    if (!fg->swap_thread) return false;
  }
  if (fg->tick_count - fg->last_swap_attempt_tick < 180u) return false;
  fg->last_swap_attempt_tick = fg->tick_count;
  fg->swap_req_hwnd = hwnd;
  fg->swap_req_w = w;
  fg->swap_req_h = h;
  if (fg->swap_step <= 0) {
    fg->swap_req_factory = 2;
    fg->swap_req_variant = 0;
  } else if (fg->swap_step == 1) {
    fg->swap_req_factory = 0;
    fg->swap_req_variant = 0;
  } else if (fg->swap_step == 2) {
    fg->swap_req_factory = 1;
    fg->swap_req_variant = 0;
  } else if (fg->swap_step == 3) {
    fg->swap_req_factory = 1;
    fg->swap_req_variant = 1;
  } else if (fg->swap_step == 4) {
    fg->swap_req_factory = 1;
    fg->swap_req_variant = 2;
  } else {
    fg->swap_req_factory = 2;
    fg->swap_req_variant = 3;
  }
  fg->swap_create_pending = true;
  SetEvent(fg->swap_work);
  return false;
}

static int FgSwapConsume(FgRuntime* fg) {
  if (!fg || !fg->swap_create_pending) return 1;
  if (WaitForSingleObject(fg->swap_done, 0u) != WAIT_OBJECT_0) return 1;
  fg->swap_create_pending = false;
  if (SUCCEEDED(fg->swap_create_hr) && fg->proxy_swap) return 0;
  return (int)fg->swap_create_hr;
}

}  // namespace senkiseki3::fg
