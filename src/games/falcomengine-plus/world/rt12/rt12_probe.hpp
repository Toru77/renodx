#pragma once

// One-shot D3D12 side-device probe (M2a, M2b), diagnostic only.

#include <Windows.h>

#include <algorithm>
#include <atomic>
#include <bit>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <initializer_list>
#include <mutex>
#include <sstream>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#if defined(_WIN32)
#include <d3d10.h>
#include <d3d11_4.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl/client.h>
#endif

#include <include/reshade.hpp>

#include "../../../../utils/directx.hpp"
#include "../../../../utils/log.hpp"
#include "../../../../utils/path.hpp"
#include "../bvh/bvh_pool.hpp"

namespace falcom_world::rt12 {

struct Rt12Probe {
  std::atomic<bool> requested{false};
  std::mutex mutex;
  std::string result;
};

inline Rt12Probe g_rt12_probe;

inline constexpr GUID kClsidD3D12SDKConfiguration = {0x7cda6aca, 0xa03e, 0x49c8, {0x94, 0x58, 0x03, 0x34, 0xd2, 0x0e, 0x07, 0xce}};

inline void RunRt12Probe(reshade::api::device* device, uint32_t frame) {
#if defined(_WIN32)
  using Microsoft::WRL::ComPtr;

  if (device->get_api() != reshade::api::device_api::d3d11) {
    std::lock_guard<std::mutex> lock(g_rt12_probe.mutex);
    g_rt12_probe.result = "DX12 probe: not D3D11";
    return;
  }

  struct Iteration {
    uint32_t index = 0u;
    uint64_t value = 0u;
    double gpu_us = std::nan("");
    bool disjoint = false;
    double submit_us = std::nan("");
    double roundtrip_us = std::nan("");
    std::string wait = "skipped";
    bool recovered = false;
  };
  struct Stats {
    size_t count = 0u;
    double min = std::nan("");
    double median = std::nan("");
    double max = std::nan("");
  };

  auto str = [](const std::string& text) { return "\"" + bvh::PoolJsonEscape(text) + "\""; };
  auto hex = [](HRESULT hr) { return renodx::utils::log::AsHex(static_cast<uint32_t>(hr)); };
  auto json_float = [](double value) { return bvh::PoolJsonFloat{static_cast<float>(value)}; };
  auto stats_of = [](const std::vector<double>& values) {
    Stats stats;
    std::vector<double> finite;
    for (const double value : values) {
      if (std::isfinite(value)) finite.push_back(value);
    }
    stats.count = finite.size();
    if (finite.empty()) return stats;
    std::sort(finite.begin(), finite.end());
    const size_t count = finite.size();
    stats.min = finite.front();
    stats.max = finite.back();
    stats.median = (count % 2u != 0u) ? finite[count / 2u] : (finite[count / 2u - 1u] + finite[count / 2u]) / 2.0;
    return stats;
  };
  auto stats_text = [](const Stats& stats) {
    if (stats.count == 0u) return std::string("n/a");
    return "med " + std::to_string(static_cast<long long>(stats.median)) + " (min "
           + std::to_string(static_cast<long long>(stats.min)) + " max " + std::to_string(static_cast<long long>(stats.max))
           + ")";
  };
  auto write_stats = [&json_float](std::ostream& out, const Stats& stats) {
    out << "{\"count\": " << stats.count << ", \"min\": " << json_float(stats.min) << ", \"median\": "
        << json_float(stats.median) << ", \"max\": " << json_float(stats.max) << "}";
  };

  OSVERSIONINFOW os_version{};
  os_version.dwOSVersionInfoSize = sizeof(os_version);
  auto rtl_get_version = reinterpret_cast<LONG(WINAPI*)(OSVERSIONINFOW*)>(
      GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "RtlGetVersion"));
  if (rtl_get_version != nullptr) rtl_get_version(&os_version);

  std::vector<std::pair<std::string, HRESULT>> trail;
  std::vector<std::pair<std::string, HRESULT>> errors;
  auto note = [&trail, &errors](const std::string& stage, HRESULT hr) {
    trail.emplace_back(stage, hr);
    if (FAILED(hr)) errors.emplace_back(stage, hr);
    return FAILED(hr);
  };

  LARGE_INTEGER qpc_frequency{};
  QueryPerformanceFrequency(&qpc_frequency);
  bool adapter_ran = false;
  std::string adapter_description;
  uint32_t vendor_id = 0u;
  uint32_t device_id = 0u;
  LUID luid{};
  std::string driver_version;
  int agility_version_requested = 0;
  char agility_path[512] = {};
  bool runtime_ran = false;
  std::string runtime = "os";
  bool wrapped = false;
  bool caps_ran = false;
  int tier = 0;
  int d3d11_shared_tier = -1;
  int d3d12_shared_tier = -1;
  uint32_t shader_model = 0u;
  bool dxr11 = false;
  bool dxr12 = false;
  uint64_t timestamp_frequency = 0u;
  bool fence_ran = false;
  std::vector<Iteration> iterations;
  bool texture_ran = false;
  bool texture_ok = false;
  HRESULT texture_hr = S_OK;
  struct Variant {
    std::string name;
    bool ran = false;
    bool ok = false;        // open succeeded
    HRESULT hr = S_OK;      // first failing HRESULT, else S_OK
    bool verified = false;  // data check ran with 0 mismatches
    uint32_t mismatches = UINT32_MAX;  // UINT32_MAX = check not run
  };
  std::vector<Variant> buffer_variants;
  std::vector<Variant> carrier_textures;
  Variant output_texture;
  bool transfer_ran = false;
  bool gpu_stuck = false;
  bool dxr_stage_ran = false;
  bool dxr_ran = false;
  uint32_t dxr_status = 0u;
  float dxr_t = std::nanf("");
  float dxr_u = std::nanf("");
  float dxr_v = std::nanf("");
  bool dxr_pass = false;
  double dxr_cpu_us = std::nan("");

  // Stages 1 to 8 stop at their first failure; 9 to 11 record and go on.
  auto run = [&]() {
    // 1. Native D3D11 device, device5/device1 and the immediate context (ctx4 for fences).
    auto* native11 = reinterpret_cast<ID3D11Device*>(static_cast<uintptr_t>(device->get_native()));  // NOLINT(performance-no-int-to-ptr)
    ComPtr<ID3D11Device5> device5;
    ComPtr<ID3D11Device1> device1;
    ComPtr<ID3D11DeviceContext> ctx;
    ComPtr<ID3D11DeviceContext4> ctx4;
    HRESULT hr = native11->QueryInterface(IID_PPV_ARGS(&device5));
    if (note("QI ID3D11Device5", hr)) return false;
    hr = native11->QueryInterface(IID_PPV_ARGS(&device1));
    if (note("QI ID3D11Device1", hr)) return false;
    native11->GetImmediateContext(&ctx);
    hr = ctx.As(&ctx4);
    if (note("QI ID3D11DeviceContext4", hr)) return false;

    // 2. Adapter of the D3D11 device.
    ComPtr<IDXGIDevice> dxgi_device;
    ComPtr<IDXGIAdapter> adapter;
    hr = device5.As(&dxgi_device);
    if (note("QI IDXGIDevice", hr)) return false;
    hr = dxgi_device->GetAdapter(&adapter);
    if (note("GetAdapter", hr)) return false;
    DXGI_ADAPTER_DESC adapter_desc{};
    hr = adapter->GetDesc(&adapter_desc);
    if (note("GetDesc", hr)) return false;
    char description[512] = {};
    WideCharToMultiByte(CP_UTF8, 0, adapter_desc.Description, -1, description, sizeof(description), nullptr, nullptr);
    adapter_description = description;
    vendor_id = adapter_desc.VendorId;
    device_id = adapter_desc.DeviceId;
    luid = adapter_desc.AdapterLuid;
    LARGE_INTEGER driver{};
    hr = adapter->CheckInterfaceSupport(__uuidof(ID3D10Device), &driver);
    note("CheckInterfaceSupport", hr);
    if (SUCCEEDED(hr)) {
      const auto version = static_cast<uint64_t>(driver.QuadPart);
      driver_version = std::to_string((version >> 48) & 0xFFFFu) + "." + std::to_string((version >> 32) & 0xFFFFu) + "."
                       + std::to_string((version >> 16) & 0xFFFFu) + "." + std::to_string(version & 0xFFFFu);
    }
    adapter_ran = true;
    renodx::utils::log::i("[world-rt12] adapter ", adapter_description, ", VendorId ",
                          renodx::utils::log::AsHex(vendor_id), ", DeviceId ", renodx::utils::log::AsHex(device_id),
                          ", LUID ", renodx::utils::log::AsHex(static_cast<uint32_t>(luid.HighPart)), ":",
                          renodx::utils::log::AsHex(luid.LowPart));

    // 3. d3d12.dll from System32, loaded once and never freed.
    static HMODULE d3d12_module = nullptr;
    if (d3d12_module == nullptr) d3d12_module = LoadLibraryExW(L"d3d12.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    const HRESULT load_hr = d3d12_module != nullptr ? S_OK : HRESULT_FROM_WIN32(GetLastError());
    if (note("LoadLibrary d3d12.dll", load_hr)) return false;
    auto create_device = reinterpret_cast<PFN_D3D12_CREATE_DEVICE>(GetProcAddress(d3d12_module, "D3D12CreateDevice"));
    auto get_interface = reinterpret_cast<PFN_D3D12_GET_INTERFACE>(GetProcAddress(d3d12_module, "D3D12GetInterface"));
    if (note("GetProcAddress D3D12", (create_device != nullptr && get_interface != nullptr) ? S_OK : E_FAIL)) return false;

    // 4. Device on the Agility SDK when configured, otherwise on the OS runtime.
    reshade::get_config_value(nullptr, "FalcomWorldProbe", "AgilitySdkVersion", agility_version_requested);
    size_t agility_path_size = sizeof(agility_path);
    reshade::get_config_value(nullptr, "FalcomWorldProbe", "AgilitySdkPath", agility_path, &agility_path_size);
    const bool agility_requested = agility_version_requested > 0 && agility_path[0] != '\0';
    ComPtr<ID3D12Device> wrapper;
    bool agility_ok = false;
    if (agility_requested) {
      ComPtr<ID3D12SDKConfiguration1> sdk_configuration;
      ComPtr<ID3D12DeviceFactory> device_factory;
      hr = get_interface(kClsidD3D12SDKConfiguration, IID_PPV_ARGS(&sdk_configuration));
      note("agility D3D12GetInterface", hr);
      if (SUCCEEDED(hr)) {
        hr = sdk_configuration->CreateDeviceFactory(static_cast<UINT>(agility_version_requested), agility_path,
                                                    IID_PPV_ARGS(&device_factory));
        note("agility CreateDeviceFactory", hr);
      }
      if (SUCCEEDED(hr)) {
        hr = device_factory->CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&wrapper));
        note("agility CreateDevice", hr);
      }
      agility_ok = SUCCEEDED(hr);
      if (agility_ok) {
        runtime = "agility " + std::to_string(agility_version_requested);
      } else {
        renodx::utils::log::i("[world-rt12] Agility SDK ", agility_version_requested, " failed hr=",
                              renodx::utils::log::AsHex(static_cast<uint32_t>(hr)), ", using D3D12CreateDevice");
      }
    }
    if (!agility_ok) {
      hr = create_device(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&wrapper));
      if (note("D3D12CreateDevice", hr)) return false;
    }

    // 5. Unwrap the ReShade device wrapper when present; the wrapper stays alive until the end.
    ID3D12Device* unwrapped = wrapper.Get();
    wrapped = renodx::utils::directx::NativeFromReShadeProxy(&unwrapped);
    ComPtr<ID3D12Device> native12 = unwrapped;
    runtime_ran = true;

    // 6. Ray tracing tier and highest shader model (stepped down from 6.9 to 6.0).
    D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5{};
    hr = native12->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options5, sizeof(options5));
    if (note("CheckFeatureSupport OPTIONS5", hr)) return false;
    tier = static_cast<int>(options5.RaytracingTier);
    D3D12_FEATURE_DATA_SHADER_MODEL shader_model_info{static_cast<D3D_SHADER_MODEL>(0x69)};
    hr = native12->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shader_model_info, sizeof(shader_model_info));
    while (hr == E_INVALIDARG && static_cast<int>(shader_model_info.HighestShaderModel) > 0x60) {
      shader_model_info.HighestShaderModel =
          static_cast<D3D_SHADER_MODEL>(static_cast<int>(shader_model_info.HighestShaderModel) - 1);
      hr = native12->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shader_model_info, sizeof(shader_model_info));
    }
    if (note("CheckFeatureSupport SHADER_MODEL", hr)) return false;
    shader_model = static_cast<uint32_t>(shader_model_info.HighestShaderModel);
    dxr11 = tier >= 11 && shader_model >= 0x65u;
    dxr12 = tier >= 12 && shader_model >= 0x69u;
    caps_ran = true;
    D3D11_FEATURE_DATA_D3D11_OPTIONS5 options11_5{};
    hr = device5->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS5, &options11_5, sizeof(options11_5));
    note("D3D11 OPTIONS5", hr);
    if (SUCCEEDED(hr)) d3d11_shared_tier = static_cast<int>(options11_5.SharedResourceTier);
    D3D12_FEATURE_DATA_D3D12_OPTIONS4 options12_4{};
    hr = native12->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS4, &options12_4, sizeof(options12_4));
    note("D3D12 OPTIONS4", hr);
    if (SUCCEEDED(hr)) d3d12_shared_tier = static_cast<int>(options12_4.SharedResourceCompatibilityTier);

    // 7. Direct queue.
    D3D12_COMMAND_QUEUE_DESC queue_desc{};
    queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    ComPtr<ID3D12CommandQueue> queue12;
    hr = native12->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&queue12));
    if (note("CreateCommandQueue", hr)) return false;

    // 8. Shared fence, opened on both sides.
    ComPtr<ID3D11Fence> fence11;
    hr = device5->CreateFence(0, D3D11_FENCE_FLAG_SHARED, IID_PPV_ARGS(&fence11));
    if (note("ID3D11 CreateFence", hr)) return false;
    HANDLE fence_handle = nullptr;
    hr = fence11->CreateSharedHandle(nullptr, GENERIC_ALL, nullptr, &fence_handle);
    if (note("CreateSharedHandle fence", hr)) return false;
    ComPtr<ID3D12Fence> fence12;
    hr = native12->OpenSharedHandle(fence_handle, IID_PPV_ARGS(&fence12));
    CloseHandle(fence_handle);
    if (note("OpenSharedHandle fence", hr)) return false;

    // 9. Round trip: D3D11 signal, D3D12 queue wait and signal, D3D11 wait, 16 times.
    constexpr uint32_t kIterations = 16u;
    fence_ran = true;
    D3D11_QUERY_DESC disjoint_desc = {D3D11_QUERY_TIMESTAMP_DISJOINT, 0u};
    D3D11_QUERY_DESC timestamp_desc = {D3D11_QUERY_TIMESTAMP, 0u};
    ComPtr<ID3D11Query> disjoint;
    ComPtr<ID3D11Query> ts0;
    ComPtr<ID3D11Query> ts1;
    bool queries_ok = !note("CreateQuery disjoint", native11->CreateQuery(&disjoint_desc, &disjoint));
    queries_ok = queries_ok && !note("CreateQuery ts0", native11->CreateQuery(&timestamp_desc, &ts0));
    queries_ok = queries_ok && !note("CreateQuery ts1", native11->CreateQuery(&timestamp_desc, &ts1));
    HANDLE event = nullptr;
    bool event_pending = false;
    if (queries_ok) {
      event = CreateEventW(nullptr, FALSE, FALSE, nullptr);
      note("CreateEvent", event != nullptr ? S_OK : HRESULT_FROM_WIN32(GetLastError()));
    }
    const bool round_trip = queries_ok && event != nullptr;
    auto to_us = [&qpc_frequency](const LARGE_INTEGER& from, const LARGE_INTEGER& to) {
      return static_cast<double>(to.QuadPart - from.QuadPart) * 1e6 / static_cast<double>(qpc_frequency.QuadPart);
    };
    auto remaining_ms = [&](const LARGE_INTEGER& from, double budget_ms) {
      LARGE_INTEGER now{};
      QueryPerformanceCounter(&now);
      return budget_ms - to_us(from, now) / 1000.0;
    };
    LARGE_INTEGER loop_start{};
    QueryPerformanceCounter(&loop_start);
    auto loop_wait_ms = [&](double limit_ms) {
      return static_cast<DWORD>(std::max(0.0, std::min(limit_ms, remaining_ms(loop_start, 5000.0))));
    };
    auto poll_query = [&](ID3D11Query* query, void* data, UINT size) {
      LARGE_INTEGER start{};
      QueryPerformanceCounter(&start);
      HRESULT poll_hr = ctx->GetData(query, data, size, 0);
      while (poll_hr == S_FALSE) {
        LARGE_INTEGER now{};
        QueryPerformanceCounter(&now);
        if (now.QuadPart - start.QuadPart >= qpc_frequency.QuadPart || remaining_ms(loop_start, 5000.0) <= 0.0) break;
        poll_hr = ctx->GetData(query, data, size, 0);
      }
      return poll_hr == S_FALSE ? HRESULT_FROM_WIN32(WAIT_TIMEOUT) : poll_hr;
    };
    if (round_trip) {
      for (uint32_t i = 0; i < kIterations; ++i) {
        if (remaining_ms(loop_start, 5000.0) <= 0.0) {
          note("round trip budget", HRESULT_FROM_WIN32(WAIT_TIMEOUT));
          break;
        }
        Iteration it;
        it.index = i;
        it.value = 2u * i + 1u;
        const uint64_t v = it.value;
        ctx->Begin(disjoint.Get());
        ctx->End(ts0.Get());
        LARGE_INTEGER t_start{};
        QueryPerformanceCounter(&t_start);
        ctx4->Signal(fence11.Get(), v);
        ctx->Flush();
        queue12->Wait(fence12.Get(), v);
        queue12->Signal(fence12.Get(), v + 1u);
        ctx4->Wait(fence11.Get(), v + 1u);
        LARGE_INTEGER t_submit{};
        QueryPerformanceCounter(&t_submit);
        ctx->End(ts1.Get());
        ctx->End(disjoint.Get());
        ctx->Flush();
        it.submit_us = to_us(t_start, t_submit);
        bool stop = note("SetEventOnCompletion", fence12->SetEventOnCompletion(v + 1u, event));
        if (stop) it.wait = "error";
        if (!stop) {
          const DWORD wait_result = WaitForSingleObject(event, loop_wait_ms(1000.0));
          LARGE_INTEGER t_done{};
          QueryPerformanceCounter(&t_done);
          it.roundtrip_us = to_us(t_start, t_done);
          if (wait_result == WAIT_OBJECT_0) {
            it.wait = "ok";
          } else {
            it.wait = "timeout";
            note("fence timeout", HRESULT_FROM_WIN32(wait_result));
            stop = true;
          }
        }
        if (stop) {
          note("CPU fence signal", fence12->Signal(v + 1u));
          if (!note("SetEventOnCompletion recovery", fence12->SetEventOnCompletion(v + 1u, event))) {
            const DWORD recovery_result = WaitForSingleObject(event, loop_wait_ms(1000.0));
            it.recovered = (recovery_result == WAIT_OBJECT_0);
            if (!it.recovered) note("fence recovery wait", HRESULT_FROM_WIN32(recovery_result));
          }
          event_pending = it.wait == "timeout" && !it.recovered;
          iterations.push_back(it);
          break;
        }
        D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjoint_data{};
        if (!note("GetData disjoint", poll_query(disjoint.Get(), &disjoint_data, sizeof(disjoint_data)))) {
          it.disjoint = disjoint_data.Disjoint != FALSE;
          if (!it.disjoint) {
            if (timestamp_frequency == 0u) timestamp_frequency = disjoint_data.Frequency;
            UINT64 begin_ticks = 0u;
            UINT64 end_ticks = 0u;
            const bool begin_ok = !note("GetData ts0", poll_query(ts0.Get(), &begin_ticks, sizeof(begin_ticks)));
            const bool end_ok = !note("GetData ts1", poll_query(ts1.Get(), &end_ticks, sizeof(end_ticks)));
            if (begin_ok && end_ok) {
              it.gpu_us = static_cast<double>(end_ticks - begin_ticks) * 1e6 / static_cast<double>(disjoint_data.Frequency);
            }
          }
        }
        iterations.push_back(it);
      }
    }
    if (event != nullptr && !event_pending) CloseHandle(event);

    // 10. Shared texture, D3D11 to D3D12.
    texture_ran = true;
    D3D11_TEXTURE2D_DESC texture_desc{};
    texture_desc.Width = 64u;
    texture_desc.Height = 64u;
    texture_desc.MipLevels = 1u;
    texture_desc.ArraySize = 1u;
    texture_desc.Format = DXGI_FORMAT_R16_FLOAT;
    texture_desc.SampleDesc.Count = 1u;
    texture_desc.Usage = D3D11_USAGE_DEFAULT;
    texture_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
    texture_desc.MiscFlags = D3D11_RESOURCE_MISC_SHARED | D3D11_RESOURCE_MISC_SHARED_NTHANDLE;
    ComPtr<ID3D11Texture2D> texture11;
    ComPtr<IDXGIResource1> texture_resource;
    ComPtr<ID3D12Resource> texture12;
    HANDLE texture_handle = nullptr;
    hr = native11->CreateTexture2D(&texture_desc, nullptr, &texture11);
    note("CreateTexture2D", hr);
    if (SUCCEEDED(hr)) {
      hr = texture11.As(&texture_resource);
      note("QI IDXGIResource1", hr);
    }
    if (SUCCEEDED(hr)) {
      hr = texture_resource->CreateSharedHandle(nullptr, DXGI_SHARED_RESOURCE_READ | DXGI_SHARED_RESOURCE_WRITE, nullptr,
                                                &texture_handle);
      note("texture CreateSharedHandle", hr);
    }
    if (SUCCEEDED(hr)) {
      hr = native12->OpenSharedHandle(texture_handle, IID_PPV_ARGS(&texture12));
      note("texture OpenSharedHandle", hr);
    }
    if (texture_handle != nullptr) CloseHandle(texture_handle);
    texture_ok = SUCCEEDED(hr);
    texture_hr = hr;

    // 11a. D3D12 submission: allocator, list and an auto-reset event for CPU waits.
    LARGE_INTEGER submit_start{};
    QueryPerformanceCounter(&submit_start);
    ComPtr<ID3D12CommandAllocator> allocator;
    ComPtr<ID3D12GraphicsCommandList4> list;
    hr = native12->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator));
    note("CreateCommandAllocator", hr);
    if (SUCCEEDED(hr)) {
      hr = native12->CreateCommandList(0u, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator.Get(), nullptr, IID_PPV_ARGS(&list));
      note("CreateCommandList", hr);
    }
    if (list.Get() != nullptr) note("CreateCommandList close", list->Close());
    if (allocator.Get() == nullptr || list.Get() == nullptr) return true;
    transfer_ran = true;

    uint64_t next_fence = 1000u;
    auto make_buffer = [&native12](D3D12_HEAP_TYPE type, uint64_t size, D3D12_RESOURCE_FLAGS flags,
                                   D3D12_RESOURCE_STATES state, D3D12_HEAP_FLAGS heap_flags, ComPtr<ID3D12Resource>* out) {
      D3D12_HEAP_PROPERTIES heap_properties{};
      heap_properties.Type = type;
      D3D12_RESOURCE_DESC desc{};
      desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
      desc.Width = size;
      desc.Height = 1u;
      desc.DepthOrArraySize = 1u;
      desc.MipLevels = 1u;
      desc.SampleDesc.Count = 1u;
      desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
      desc.Flags = flags;
      return native12->CreateCommittedResource(&heap_properties, heap_flags, &desc, state, nullptr,
                                               IID_PPV_ARGS(out->ReleaseAndGetAddressOf()));
    };
    auto submit12 = [&](const char* stage, std::initializer_list<IUnknown*> uses,
                        const std::function<void(ID3D12GraphicsCommandList4*)>& fill) {
      const std::string name = std::string(stage) + " submit ";
      if (gpu_stuck) {
        note(name + "skipped: GPU stuck", E_ABORT);
        return E_ABORT;
      }
      if (remaining_ms(submit_start, 15000.0) <= 0.0) {
        note(name + "skipped: budget", E_ABORT);
        return E_ABORT;
      }
      HRESULT result = allocator->Reset();
      note(name + "allocator Reset", result);
      if (SUCCEEDED(result)) {
        result = list->Reset(allocator.Get(), nullptr);
        note(name + "list Reset", result);
      }
      if (SUCCEEDED(result)) {
        fill(list.Get());
        result = list->Close();
        note(name + "list Close", result);
      }
      if (FAILED(result)) return result;
      ++next_fence;
      result = ctx4->Signal(fence11.Get(), next_fence);
      note(name + "D3D11 Signal", result);
      ctx->Flush();
      if (FAILED(result)) return result;
      result = queue12->Wait(fence12.Get(), next_fence);
      note(name + "queue Wait", result);
      if (FAILED(result)) return result;
      ID3D12CommandList* lists[] = {list.Get()};
      queue12->ExecuteCommandLists(1u, lists);
      LARGE_INTEGER t_execute{};
      QueryPerformanceCounter(&t_execute);
      ++next_fence;
      const HRESULT signal_hr = queue12->Signal(fence12.Get(), next_fence);
      note(name + "queue Signal", signal_hr);
      result = signal_hr;
      HANDLE event = nullptr;
      if (SUCCEEDED(result)) {
        event = CreateEventW(nullptr, FALSE, FALSE, nullptr);
        result = event != nullptr ? S_OK : HRESULT_FROM_WIN32(GetLastError());
        note(name + "CreateEvent", result);
      }
      if (SUCCEEDED(result)) {
        result = fence12->SetEventOnCompletion(next_fence, event);
        note(name + "SetEventOnCompletion", result);
      }
      if (SUCCEEDED(result)) {
        if (WaitForSingleObject(event, static_cast<DWORD>(std::max(0.0, std::min(2000.0, remaining_ms(submit_start, 15000.0)))))
            == WAIT_OBJECT_0) {
          CloseHandle(event);
          return S_OK;
        }
        result = HRESULT_FROM_WIN32(WAIT_TIMEOUT);
        note(name + "wait", result);
      }
      if (SUCCEEDED(signal_hr)) {
        while (fence12->GetCompletedValue() < next_fence && remaining_ms(t_execute, 10000.0) > 0.0
               && remaining_ms(submit_start, 15000.0) > 0.0) {
          Sleep(1);
        }
        if (fence12->GetCompletedValue() >= next_fence) {
          if (event != nullptr) CloseHandle(event);
          return result;
        }
      }
      gpu_stuck = true;
      note(name + "stuck", HRESULT_FROM_WIN32(WAIT_TIMEOUT));
      // leaked on purpose: the GPU may still use them
      for (IUnknown* object : uses) object->AddRef();
      allocator->AddRef();
      list->AddRef();
      queue12->AddRef();
      native12->AddRef();
      fence12->AddRef();
      return HRESULT_FROM_WIN32(WAIT_TIMEOUT);
    };
    auto pattern = [](uint32_t i) { return (i * 2654435761u) ^ 0xA5A5A5A5u; };
    auto pattern_words = [&pattern](size_t count) {
      std::vector<uint32_t> data(count);
      for (size_t i = 0u; i < count; ++i) data[i] = pattern(static_cast<uint32_t>(i));
      return data;
    };
    auto mismatches_in = [&pattern](const uint8_t* data, uint32_t row_pitch_bytes, uint32_t row_words, uint32_t rows) {
      uint32_t mismatches = 0u;
      for (uint32_t row = 0u; row < rows; ++row) {
        const auto* row_data = reinterpret_cast<const uint32_t*>(data + static_cast<size_t>(row) * row_pitch_bytes);
        for (uint32_t word = 0u; word < row_words; ++word) {
          if (row_data[word] != pattern(row * row_words + word)) ++mismatches;
        }
      }
      return mismatches;
    };
    auto count_mismatches = [&](const char* stage, ID3D12Resource* readback_resource, uint64_t offset,
                                uint32_t row_pitch_bytes, uint32_t row_words, uint32_t rows, HRESULT* first_failure) {
      void* data = nullptr;
      const HRESULT map_hr = readback_resource->Map(0u, nullptr, &data);
      if (FAILED(map_hr)) {
        note(std::string(stage) + " readback Map", map_hr);
        if (SUCCEEDED(*first_failure)) *first_failure = map_hr;
        return UINT32_MAX;
      }
      const uint32_t mismatches =
          mismatches_in(static_cast<const uint8_t*>(data) + offset, row_pitch_bytes, row_words, rows);
      readback_resource->Unmap(0u, nullptr);
      return mismatches;
    };
    auto keep_hr = [](Variant& variant, HRESULT step_hr) {
      if (FAILED(step_hr) && SUCCEEDED(variant.hr)) variant.hr = step_hr;
    };
    auto note_step = [&note, &keep_hr](Variant& variant, const std::string& stage, HRESULT step_hr) {
      keep_hr(variant, step_hr);
      note(stage, step_hr);
    };

    // 11b. Shared readback buffer and the buffer variants, D3D11 data into D3D12.
    ComPtr<ID3D12Resource> readback;
    const bool readback_ok = !note("readback create", make_buffer(D3D12_HEAP_TYPE_READBACK, 262144u, D3D12_RESOURCE_FLAG_NONE,
                                                                 D3D12_RESOURCE_STATE_COPY_DEST, D3D12_HEAP_FLAG_NONE, &readback));
    auto verify_buffer = [&](Variant& variant, const char* stage, ID3D12Resource* target12, ID3D11Buffer* target11) {
      if (!readback_ok || !variant.ok) return;
      if (gpu_stuck) {
        note(std::string(stage) + " data check skipped: GPU stuck", E_ABORT);
        keep_hr(variant, E_ABORT);
        return;
      }
      const std::vector<uint32_t> data = pattern_words(16384u);
      ctx->UpdateSubresource(target11, 0u, nullptr, data.data(), 0u, 0u);
      const HRESULT submit_hr = submit12(stage, {target12, readback.Get()}, [&](ID3D12GraphicsCommandList4* commands) {
        commands->CopyBufferRegion(readback.Get(), 0u, target12, 0u, 65536u);
      });
      if (FAILED(submit_hr)) {
        keep_hr(variant, submit_hr);
        return;
      }
      variant.mismatches = count_mismatches(stage, readback.Get(), 0u, 65536u, 16384u, 1u, &variant.hr);
      variant.verified = variant.mismatches == 0u;
    };
    auto d3d12_buffer_variant = [&](const char* stage, const char* name, D3D12_RESOURCE_FLAGS flags) {
      Variant variant{.name = name, .ran = true};
      ComPtr<ID3D12Resource> buffer12;
      ComPtr<ID3D11Resource> resource11;
      ComPtr<ID3D11Buffer> buffer11;
      HANDLE handle = nullptr;
      HRESULT step = make_buffer(D3D12_HEAP_TYPE_DEFAULT, 65536u, flags, D3D12_RESOURCE_STATE_COMMON,
                                 D3D12_HEAP_FLAG_SHARED, &buffer12);
      note_step(variant, variant.name + " CreateCommittedResource", step);
      if (SUCCEEDED(step)) {
        step = native12->CreateSharedHandle(buffer12.Get(), nullptr, GENERIC_ALL, nullptr, &handle);
        note_step(variant, variant.name + " CreateSharedHandle", step);
      }
      if (SUCCEEDED(step)) {
        step = device1->OpenSharedResource1(handle, IID_PPV_ARGS(&resource11));
        note_step(variant, variant.name + " OpenSharedResource1", step);
      }
      if (handle != nullptr) CloseHandle(handle);
      if (SUCCEEDED(step)) {
        step = resource11.As(&buffer11);
        note_step(variant, variant.name + " QI ID3D11Buffer", step);
      }
      variant.ok = SUCCEEDED(step);
      verify_buffer(variant, stage, buffer12.Get(), buffer11.Get());
      buffer_variants.push_back(variant);
    };
    auto d3d11_buffer_variant = [&](const char* stage, const char* name, UINT bind_flags, UINT misc_flags) {
      Variant variant{.name = name, .ran = true};
      D3D11_BUFFER_DESC desc11{};
      desc11.ByteWidth = 65536u;
      desc11.Usage = D3D11_USAGE_DEFAULT;
      desc11.BindFlags = bind_flags;
      desc11.MiscFlags = misc_flags;
      ComPtr<ID3D11Buffer> buffer11;
      ComPtr<IDXGIResource1> resource;
      ComPtr<ID3D12Resource> buffer12;
      HANDLE handle = nullptr;
      HRESULT step = native11->CreateBuffer(&desc11, nullptr, &buffer11);
      note_step(variant, variant.name + " CreateBuffer", step);
      if (SUCCEEDED(step)) {
        step = buffer11.As(&resource);
        note_step(variant, variant.name + " QI IDXGIResource1", step);
      }
      if (SUCCEEDED(step)) {
        step = resource->CreateSharedHandle(nullptr, DXGI_SHARED_RESOURCE_READ | DXGI_SHARED_RESOURCE_WRITE, nullptr, &handle);
        note_step(variant, variant.name + " CreateSharedHandle", step);
      }
      if (SUCCEEDED(step)) {
        step = native12->OpenSharedHandle(handle, IID_PPV_ARGS(&buffer12));
        note_step(variant, variant.name + " OpenSharedHandle", step);
      }
      if (handle != nullptr) CloseHandle(handle);
      variant.ok = SUCCEEDED(step);
      verify_buffer(variant, stage, buffer12.Get(), buffer11.Get());
      buffer_variants.push_back(variant);
    };
    d3d12_buffer_variant("V1", "d3d12_buffer_as_resource", D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    d3d12_buffer_variant("V2", "d3d12_buffer_no_uav", D3D12_RESOURCE_FLAG_NONE);
    d3d11_buffer_variant("V3a", "d3d11_buffer_uav_raw", D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS,
                         D3D11_RESOURCE_MISC_SHARED | D3D11_RESOURCE_MISC_SHARED_NTHANDLE | D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS);
    d3d11_buffer_variant("V3b", "d3d11_buffer_stream_output", D3D11_BIND_STREAM_OUTPUT | D3D11_BIND_SHADER_RESOURCE,
                         D3D11_RESOURCE_MISC_SHARED | D3D11_RESOURCE_MISC_SHARED_NTHANDLE);

    // 11c. Carrier textures, D3D11 data into D3D12.
    auto share_texture = [&](Variant& variant, const D3D11_TEXTURE2D_DESC& desc, ComPtr<ID3D11Texture2D>& texture11,
                             ComPtr<ID3D12Resource>& texture12) {
      ComPtr<IDXGIResource1> resource;
      HANDLE handle = nullptr;
      HRESULT step = native11->CreateTexture2D(&desc, nullptr, &texture11);
      note_step(variant, variant.name + " CreateTexture2D", step);
      if (SUCCEEDED(step)) {
        step = texture11.As(&resource);
        note_step(variant, variant.name + " QI IDXGIResource1", step);
      }
      if (SUCCEEDED(step)) {
        step = resource->CreateSharedHandle(nullptr, DXGI_SHARED_RESOURCE_READ | DXGI_SHARED_RESOURCE_WRITE, nullptr, &handle);
        note_step(variant, variant.name + " CreateSharedHandle", step);
      }
      if (SUCCEEDED(step)) {
        step = native12->OpenSharedHandle(handle, IID_PPV_ARGS(&texture12));
        note_step(variant, variant.name + " OpenSharedHandle", step);
      }
      if (handle != nullptr) CloseHandle(handle);
      return SUCCEEDED(step);
    };
    auto carrier_variant = [&](const char* stage, const char* name, DXGI_FORMAT format, uint32_t width, uint32_t height,
                               uint32_t texel_bytes) {
      Variant variant{.name = name, .ran = true};
      D3D11_TEXTURE2D_DESC desc = texture_desc;
      desc.Width = width;
      desc.Height = height;
      desc.Format = format;
      ComPtr<ID3D11Texture2D> texture11;
      ComPtr<ID3D12Resource> texture12;
      variant.ok = share_texture(variant, desc, texture11, texture12);
      if (variant.ok && gpu_stuck) {
        note(std::string(stage) + " data check skipped: GPU stuck", E_ABORT);
        keep_hr(variant, E_ABORT);
      } else if (variant.ok) {
        const uint32_t row_bytes = width * texel_bytes;
        const uint32_t row_words = row_bytes / 4u;
        const D3D12_RESOURCE_DESC desc12 = texture12->GetDesc();
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};
        native12->GetCopyableFootprints(&desc12, 0u, 1u, 0u, &footprint, nullptr, nullptr, nullptr);
        if (readback_ok) {
          const std::vector<uint32_t> data = pattern_words(static_cast<size_t>(row_words) * height);
          ctx->UpdateSubresource(texture11.Get(), 0u, nullptr, data.data(), row_bytes, 0u);
          const HRESULT submit_hr = submit12(stage, {texture12.Get(), readback.Get()}, [&](ID3D12GraphicsCommandList4* commands) {
            D3D12_TEXTURE_COPY_LOCATION dst{};
            dst.pResource = readback.Get();
            dst.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
            dst.PlacedFootprint = footprint;
            D3D12_TEXTURE_COPY_LOCATION src{};
            src.pResource = texture12.Get();
            src.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            src.SubresourceIndex = 0u;
            commands->CopyTextureRegion(&dst, 0u, 0u, 0u, &src, nullptr);
          });
          if (FAILED(submit_hr)) {
            keep_hr(variant, submit_hr);
          } else {
            variant.mismatches =
                count_mismatches(stage, readback.Get(), footprint.Offset, footprint.Footprint.RowPitch, row_words, height,
                                 &variant.hr);
            variant.verified = variant.mismatches == 0u;
          }
        }
      }
      carrier_textures.push_back(variant);
    };
    carrier_variant("T1", "texture_rgba32_uint", DXGI_FORMAT_R32G32B32A32_UINT, 1024u, 16u, 16u);
    carrier_variant("T2", "texture_r32_uint", DXGI_FORMAT_R32_UINT, 256u, 64u, 4u);

    // 11d. Output texture O1: D3D12 writes, D3D11 reads through staging.
    output_texture.name = "texture_r32_uint_out";
    output_texture.ran = true;
    D3D11_TEXTURE2D_DESC output_desc = texture_desc;
    output_desc.Width = 64u;
    output_desc.Height = 64u;
    output_desc.Format = DXGI_FORMAT_R32_UINT;
    ComPtr<ID3D11Texture2D> output11;
    ComPtr<ID3D12Resource> output12;
    output_texture.ok = share_texture(output_texture, output_desc, output11, output12);
    if (output_texture.ok && gpu_stuck) {
      note("O1 data check skipped: GPU stuck", E_ABORT);
      keep_hr(output_texture, E_ABORT);
    } else if (output_texture.ok) {
      const D3D12_RESOURCE_DESC output_desc12 = output12->GetDesc();
      D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};
      uint64_t total_size = 0u;
      native12->GetCopyableFootprints(&output_desc12, 0u, 1u, 0u, &footprint, nullptr, nullptr, &total_size);
      ComPtr<ID3D12Resource> upload;
      HRESULT step = make_buffer(D3D12_HEAP_TYPE_UPLOAD, total_size, D3D12_RESOURCE_FLAG_NONE,
                                 D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_HEAP_FLAG_NONE, &upload);
      note_step(output_texture, output_texture.name + " upload create", step);
      if (SUCCEEDED(step)) {
        void* mapped = nullptr;
        step = upload->Map(0u, nullptr, &mapped);
        note_step(output_texture, output_texture.name + " upload Map", step);
        if (SUCCEEDED(step)) {
          for (uint32_t row = 0u; row < 64u; ++row) {
            auto* row_words = reinterpret_cast<uint32_t*>(static_cast<uint8_t*>(mapped) + footprint.Offset
                                                          + static_cast<uint64_t>(row) * footprint.Footprint.RowPitch);
            for (uint32_t x = 0u; x < 64u; ++x) row_words[x] = pattern(row * 64u + x);
          }
          upload->Unmap(0u, nullptr);
        }
      }
      if (SUCCEEDED(step)) {
        step = submit12("O1", {output12.Get(), upload.Get()}, [&](ID3D12GraphicsCommandList4* commands) {
          D3D12_TEXTURE_COPY_LOCATION dst{};
          dst.pResource = output12.Get();
          dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
          dst.SubresourceIndex = 0u;
          D3D12_TEXTURE_COPY_LOCATION src{};
          src.pResource = upload.Get();
          src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
          src.PlacedFootprint = footprint;
          commands->CopyTextureRegion(&dst, 0u, 0u, 0u, &src, nullptr);
        });
        keep_hr(output_texture, step);
      }
      D3D11_TEXTURE2D_DESC staging_desc = output_desc;
      staging_desc.Usage = D3D11_USAGE_STAGING;
      staging_desc.BindFlags = 0u;
      staging_desc.MiscFlags = 0u;
      staging_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
      ComPtr<ID3D11Texture2D> staging;
      if (SUCCEEDED(step)) {
        step = native11->CreateTexture2D(&staging_desc, nullptr, &staging);
        note_step(output_texture, output_texture.name + " CreateTexture2D staging", step);
      }
      if (SUCCEEDED(step)) {
        ctx->CopyResource(staging.Get(), output11.Get());
        D3D11_MAPPED_SUBRESOURCE mapped11{};
        step = ctx->Map(staging.Get(), 0u, D3D11_MAP_READ, 0u, &mapped11);
        note_step(output_texture, output_texture.name + " staging Map", step);
        if (SUCCEEDED(step)) {
          output_texture.mismatches = mismatches_in(static_cast<const uint8_t*>(mapped11.pData), mapped11.RowPitch, 64u, 64u);
          output_texture.verified = output_texture.mismatches == 0u;
          ctx->Unmap(staging.Get(), 0u);
        }
      }
    }

    // 12. DXR round trip: BLAS and TLAS build, one RayQuery dispatch, readback. First failure ends stage 12.
    auto align256 = [](uint64_t size) { return (size + 255u) & ~uint64_t{255u}; };
    if (dxr11) {
#if defined(__world_rt12_probe_EMBED_FILE)
      dxr_stage_ran = true;
      ComPtr<ID3D12Device5> device12_5;
      hr = native12.As(&device12_5);
      note("QI ID3D12Device5", hr);

      ComPtr<ID3D12Resource> vertices;
      if (SUCCEEDED(hr)) {
        hr = make_buffer(D3D12_HEAP_TYPE_UPLOAD, 36u, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ,
                         D3D12_HEAP_FLAG_NONE, &vertices);
        note("dxr vertex create", hr);
      }
      if (SUCCEEDED(hr)) {
        void* mapped = nullptr;
        hr = vertices->Map(0u, nullptr, &mapped);
        note("dxr vertex Map", hr);
        if (SUCCEEDED(hr)) {
          const float positions[9] = {-1.f, -1.f, 0.f, 1.f, -1.f, 0.f, 0.f, 1.f, 0.f};
          std::copy_n(positions, 9, static_cast<float*>(mapped));
          vertices->Unmap(0u, nullptr);
        }
      }

      D3D12_RAYTRACING_GEOMETRY_DESC geometry{};
      D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS blas_inputs{};
      D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO blas_info{};
      if (SUCCEEDED(hr)) {
        geometry.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
        geometry.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
        geometry.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
        geometry.Triangles.VertexBuffer.StartAddress = vertices->GetGPUVirtualAddress();
        geometry.Triangles.VertexBuffer.StrideInBytes = 12u;
        geometry.Triangles.VertexCount = 3u;
        blas_inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
        blas_inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
        blas_inputs.NumDescs = 1u;
        blas_inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
        blas_inputs.pGeometryDescs = &geometry;
        device12_5->GetRaytracingAccelerationStructurePrebuildInfo(&blas_inputs, &blas_info);
      }

      ComPtr<ID3D12Resource> instances;
      if (SUCCEEDED(hr)) {
        hr = make_buffer(D3D12_HEAP_TYPE_UPLOAD, sizeof(D3D12_RAYTRACING_INSTANCE_DESC), D3D12_RESOURCE_FLAG_NONE,
                         D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_HEAP_FLAG_NONE, &instances);
        note("dxr instance create", hr);
      }

      D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS tlas_inputs{};
      D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO tlas_info{};
      if (SUCCEEDED(hr)) {
        tlas_inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
        tlas_inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD;
        tlas_inputs.NumDescs = 1u;
        tlas_inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
        tlas_inputs.InstanceDescs = instances->GetGPUVirtualAddress();
        device12_5->GetRaytracingAccelerationStructurePrebuildInfo(&tlas_inputs, &tlas_info);
      }

      ComPtr<ID3D12Resource> blas;
      ComPtr<ID3D12Resource> tlas;
      ComPtr<ID3D12Resource> scratch;
      ComPtr<ID3D12Resource> output;
      ComPtr<ID3D12Resource> dxr_readback;
      if (SUCCEEDED(hr)) {
        hr = make_buffer(D3D12_HEAP_TYPE_DEFAULT, align256(blas_info.ResultDataMaxSizeInBytes),
                         D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
                         D3D12_HEAP_FLAG_NONE, &blas);
        note("dxr BLAS create", hr);
        if (SUCCEEDED(hr)) {
          hr = make_buffer(D3D12_HEAP_TYPE_DEFAULT, align256(tlas_info.ResultDataMaxSizeInBytes),
                           D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
                           D3D12_HEAP_FLAG_NONE, &tlas);
          note("dxr TLAS create", hr);
        }
        if (SUCCEEDED(hr)) {
          hr = make_buffer(D3D12_HEAP_TYPE_DEFAULT,
                         std::max(align256(blas_info.ScratchDataSizeInBytes), align256(tlas_info.ScratchDataSizeInBytes)),
                         D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                           D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_HEAP_FLAG_NONE, &scratch);
          note("dxr scratch create", hr);
        }
        if (SUCCEEDED(hr)) {
          hr = make_buffer(D3D12_HEAP_TYPE_DEFAULT, 256u, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                           D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_HEAP_FLAG_NONE, &output);
          note("dxr output create", hr);
        }
        if (SUCCEEDED(hr)) {
          hr = make_buffer(D3D12_HEAP_TYPE_READBACK, 256u, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COPY_DEST,
                           D3D12_HEAP_FLAG_NONE, &dxr_readback);
          note("dxr readback create", hr);
        }
      }
      if (SUCCEEDED(hr)) {
        void* mapped = nullptr;
        hr = instances->Map(0u, nullptr, &mapped);
        note("dxr instance Map", hr);
        if (SUCCEEDED(hr)) {
          D3D12_RAYTRACING_INSTANCE_DESC instance{};
          instance.Transform[0][0] = 1.f;
          instance.Transform[1][1] = 1.f;
          instance.Transform[2][2] = 1.f;
          instance.InstanceMask = 0xFF;
          instance.AccelerationStructure = blas->GetGPUVirtualAddress();
          *static_cast<D3D12_RAYTRACING_INSTANCE_DESC*>(mapped) = instance;
          instances->Unmap(0u, nullptr);
        }
      }

      ComPtr<ID3D12RootSignature> root_signature;
      if (SUCCEEDED(hr)) {
        D3D12_ROOT_PARAMETER parameters[2]{};
        parameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
        parameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        parameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
        parameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        D3D12_ROOT_SIGNATURE_DESC root_desc{};
        root_desc.NumParameters = 2u;
        root_desc.pParameters = parameters;
        auto serialize = reinterpret_cast<PFN_D3D12_SERIALIZE_ROOT_SIGNATURE>(
            GetProcAddress(d3d12_module, "D3D12SerializeRootSignature"));
        hr = (serialize != nullptr) ? S_OK : E_FAIL;
        note("dxr GetProcAddress D3D12SerializeRootSignature", hr);
        ComPtr<ID3DBlob> blob;
        ComPtr<ID3DBlob> error;
        if (SUCCEEDED(hr)) {
          hr = serialize(&root_desc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &error);
          note("dxr D3D12SerializeRootSignature", hr);
        }
        if (FAILED(hr) && error.Get() != nullptr) {
          renodx::utils::log::i("[world-rt12] root signature: ",
                                std::string(static_cast<const char*>(error->GetBufferPointer()), error->GetBufferSize()));
        }
        if (SUCCEEDED(hr)) {
          hr = native12->CreateRootSignature(0u, blob->GetBufferPointer(), blob->GetBufferSize(),
                                             IID_PPV_ARGS(&root_signature));
          note("dxr CreateRootSignature", hr);
        }
      }

      ComPtr<ID3D12PipelineState> pipeline;
      if (SUCCEEDED(hr)) {
        D3D12_COMPUTE_PIPELINE_STATE_DESC pipeline_desc{};
        pipeline_desc.pRootSignature = root_signature.Get();
        pipeline_desc.CS.pShaderBytecode = __world_rt12_probe.data();
        pipeline_desc.CS.BytecodeLength = __world_rt12_probe.size();
        hr = native12->CreateComputePipelineState(&pipeline_desc, IID_PPV_ARGS(&pipeline));
        note("dxr CreateComputePipelineState", hr);
      }

      if (SUCCEEDED(hr)) {
        LARGE_INTEGER t_start{};
        QueryPerformanceCounter(&t_start);
        hr = submit12("dxr", {vertices.Get(), blas.Get(), tlas.Get(), scratch.Get(), instances.Get(), output.Get(),
                              dxr_readback.Get(), root_signature.Get(), pipeline.Get()}, [&](ID3D12GraphicsCommandList4* commands) {
          D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC blas_build{};
          blas_build.DestAccelerationStructureData = blas->GetGPUVirtualAddress();
          blas_build.Inputs = blas_inputs;
          blas_build.ScratchAccelerationStructureData = scratch->GetGPUVirtualAddress();
          commands->BuildRaytracingAccelerationStructure(&blas_build, 0u, nullptr);
          D3D12_RESOURCE_BARRIER global_barrier{};
          global_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
          global_barrier.UAV.pResource = nullptr;
          commands->ResourceBarrier(1u, &global_barrier);
          D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC tlas_build{};
          tlas_build.DestAccelerationStructureData = tlas->GetGPUVirtualAddress();
          tlas_build.Inputs = tlas_inputs;
          tlas_build.ScratchAccelerationStructureData = scratch->GetGPUVirtualAddress();
          commands->BuildRaytracingAccelerationStructure(&tlas_build, 0u, nullptr);
          D3D12_RESOURCE_BARRIER tlas_barrier{};
          tlas_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
          tlas_barrier.UAV.pResource = tlas.Get();
          commands->ResourceBarrier(1u, &tlas_barrier);
          commands->SetComputeRootSignature(root_signature.Get());
          commands->SetPipelineState(pipeline.Get());
          commands->SetComputeRootShaderResourceView(0u, tlas->GetGPUVirtualAddress());
          commands->SetComputeRootUnorderedAccessView(1u, output->GetGPUVirtualAddress());
          commands->Dispatch(1u, 1u, 1u);
          D3D12_RESOURCE_BARRIER output_barrier{};
          output_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
          output_barrier.Transition.pResource = output.Get();
          output_barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
          output_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
          output_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
          commands->ResourceBarrier(1u, &output_barrier);
          commands->CopyBufferRegion(dxr_readback.Get(), 0u, output.Get(), 0u, 16u);
        });
        LARGE_INTEGER t_done{};
        QueryPerformanceCounter(&t_done);
        dxr_cpu_us = to_us(t_start, t_done);
      }

      if (SUCCEEDED(hr)) {
        void* mapped = nullptr;
        hr = dxr_readback->Map(0u, nullptr, &mapped);
        note("dxr readback Map", hr);
        if (SUCCEEDED(hr)) {
          const auto* words = static_cast<const uint32_t*>(mapped);
          dxr_status = words[0];
          dxr_t = std::bit_cast<float>(words[1]);
          dxr_u = std::bit_cast<float>(words[2]);
          dxr_v = std::bit_cast<float>(words[3]);
          dxr_readback->Unmap(0u, nullptr);
          dxr_ran = true;
          dxr_pass = dxr_status == 1u && std::fabs(dxr_t - 1.f) < 1e-5f && std::fabs(dxr_u - 0.25f) < 1e-5f
                     && std::fabs(dxr_v - 0.5f) < 1e-5f;
        }
      }
#else
      note("dxr shader not embedded", E_NOTIMPL);
#endif
    } else {
      note("dxr skipped: no DXR 1.1", E_NOTIMPL);
    }

    return true;
  };
  const bool completed = run();

  std::vector<double> gpu_values;
  std::vector<double> submit_values;
  std::vector<double> roundtrip_values;
  for (const Iteration& it : iterations) {
    gpu_values.push_back(it.gpu_us);
    submit_values.push_back(it.submit_us);
    roundtrip_values.push_back(it.roundtrip_us);
  }
  const Stats gpu_stats = stats_of(gpu_values);
  const Stats submit_stats = stats_of(submit_values);
  const Stats roundtrip_stats = stats_of(roundtrip_values);

  auto verified_names = [](const std::vector<Variant>& records) {
    std::string names;
    for (const Variant& record : records) {
      if (!record.verified) continue;
      names += (names.empty() ? "" : "+") + record.name;
    }
    return names.empty() ? std::string("none") : names;
  };
  auto write_variant = [&str, &hex](std::ostream& out_stream, const Variant& variant) {
    out_stream << "{\"name\": " << str(variant.name) << ", \"ran\": " << (variant.ran ? "true" : "false")
               << ", \"ok\": " << (variant.ok ? "true" : "false") << ", \"hr\": " << str(hex(variant.hr))
               << ", \"verified\": " << (variant.verified ? "true" : "false") << ", \"mismatches\": "
               << (variant.mismatches == UINT32_MAX ? std::string("null") : std::to_string(variant.mismatches)) << "}";
  };
  auto write_variants = [&write_variant](std::ostream& out_stream, const std::vector<Variant>& variants) {
    out_stream << "[";
    for (size_t i = 0; i < variants.size(); ++i) {
      if (i != 0u) out_stream << ", ";
      write_variant(out_stream, variants[i]);
    }
    out_stream << "]";
  };
  const std::string json_name = "dx12_probe_f" + std::to_string(frame) + ".json";
  std::string line;
  if (completed) {
    line = "DX12 probe f" + std::to_string(frame) + ": runtime " + runtime + ", wrapped " + (wrapped ? "yes" : "no")
           + ", tier " + std::to_string(tier) + ", SM " + renodx::utils::log::AsHex(shader_model) + ", DXR1.1 "
           + (dxr11 ? "yes" : "no") + ", DXR1.2 " + (dxr12 ? "yes" : "no") + ", fence gpu us "
           + stats_text(gpu_stats) + ", submit us " + stats_text(submit_stats) + ", roundtrip us "
           + stats_text(roundtrip_stats) + ", tex share " + (texture_ok ? std::string("ok") : hex(texture_hr))
           + ", buffer " + (transfer_ran ? verified_names(buffer_variants) : std::string("n/a")) + ", carrier "
           + (transfer_ran ? verified_names(carrier_textures) : std::string("n/a"))
           + ", output " + (transfer_ran ? (output_texture.verified ? "ok" : "fail") : "n/a") + ", dxr "
           + (transfer_ran ? (dxr_ran ? (dxr_pass ? "pass" : "fail") : "skipped") : "n/a") + ", errors "
           + std::to_string(errors.size()) + ", json " + json_name;
  } else {
    line = "DX12 probe f" + std::to_string(frame) + ": " + trail.back().first + " failed hr=" + hex(trail.back().second)
           + ", errors " + std::to_string(errors.size()) + ", json " + json_name;
  }

  std::ostringstream out;
  out << "{\"schema\": 2, \"frame\": " << frame << ", \"result\": " << str(line) << ",\n";
  out << "  \"os\": {\"major\": " << os_version.dwMajorVersion << ", \"minor\": " << os_version.dwMinorVersion
      << ", \"build\": " << os_version.dwBuildNumber << "},\n";
  out << "  \"adapter\": ";
  if (adapter_ran) {
    out << "{\"description\": " << str(adapter_description) << ", \"vendor_id\": "
        << str(renodx::utils::log::AsHex(vendor_id)) << ", \"device_id\": " << str(renodx::utils::log::AsHex(device_id))
        << ", \"luid\": "
        << str(renodx::utils::log::AsHex(static_cast<uint32_t>(luid.HighPart)) + ":" + renodx::utils::log::AsHex(luid.LowPart))
        << ", \"driver_version\": " << (driver_version.empty() ? std::string("null") : str(driver_version)) << "},\n";
  } else {
    out << "null,\n";
  }
  out << "  \"runtime\": ";
  if (runtime_ran) {
    out << "{\"agility_version_requested\": " << agility_version_requested << ", \"agility_path\": " << str(agility_path)
        << ", \"used\": " << str(runtime) << ", \"wrapped\": " << (wrapped ? "true" : "false") << "},\n";
  } else {
    out << "null,\n";
  }
  out << "  \"caps\": ";
  if (caps_ran) {
    out << "{\"raytracing_tier\": " << tier << ", \"shader_model\": " << str(renodx::utils::log::AsHex(shader_model))
        << ", \"dxr11\": " << (dxr11 ? "true" : "false") << ", \"dxr12\": " << (dxr12 ? "true" : "false")
        << ", \"d3d11_shared_tier\": " << (d3d11_shared_tier < 0 ? std::string("null") : std::to_string(d3d11_shared_tier))
        << ", \"d3d12_shared_tier\": " << (d3d12_shared_tier < 0 ? std::string("null") : std::to_string(d3d12_shared_tier))
        << "},\n";
  } else {
    out << "null,\n";
  }
  out << "  \"clocks\": {\"qpc_frequency\": " << qpc_frequency.QuadPart << ", \"timestamp_frequency\": "
      << (timestamp_frequency != 0u ? std::to_string(timestamp_frequency) : std::string("null")) << "},\n";
  out << "  \"fence\": ";
  if (fence_ran) {
    out << "{\"iterations\": [";
    for (size_t i = 0; i < iterations.size(); ++i) {
      const Iteration& it = iterations[i];
      if (i != 0u) out << ", ";
      out << "{\"index\": " << it.index << ", \"value\": " << it.value << ", \"gpu_us\": " << json_float(it.gpu_us)
          << ", \"disjoint\": " << (it.disjoint ? "true" : "false") << ", \"submit_us\": " << json_float(it.submit_us)
          << ", \"roundtrip_us\": " << json_float(it.roundtrip_us) << ", \"wait\": " << str(it.wait)
          << ", \"recovered\": " << (it.recovered ? "true" : "false") << "}";
    }
    out << "], \"gpu_us\": ";
    write_stats(out, gpu_stats);
    out << ", \"submit_us\": ";
    write_stats(out, submit_stats);
    out << ", \"roundtrip_us\": ";
    write_stats(out, roundtrip_stats);
    out << "},\n";
  } else {
    out << "null,\n";
  }
  out << "  \"sharing\": {\"texture\": ";
  if (texture_ran) {
    out << "{\"ok\": " << (texture_ok ? "true" : "false") << ", \"hr\": " << str(hex(texture_hr)) << "}";
  } else {
    out << "null";
  }
  if (transfer_ran) {
    out << ", \"buffer_variants\": ";
    write_variants(out, buffer_variants);
    out << ", \"carrier_textures\": ";
    write_variants(out, carrier_textures);
    out << ", \"output_texture\": ";
    write_variant(out, output_texture);
  } else {
    out << ", \"buffer_variants\": null, \"carrier_textures\": null, \"output_texture\": null";
  }
  out << "},\n";
  out << "  \"dxr\": ";
  if (dxr_stage_ran) {
    out << "{\"ran\": " << (dxr_ran ? "true" : "false") << ", \"status\": "
        << (dxr_ran ? std::to_string(dxr_status) : std::string("null")) << ", \"t\": " << json_float(dxr_t)
        << ", \"u\": " << json_float(dxr_u) << ", \"v\": " << json_float(dxr_v) << ", \"pass\": "
        << (dxr_ran ? (dxr_pass ? "true" : "false") : "null") << ", \"cpu_us\": " << json_float(dxr_cpu_us) << "},\n";
  } else {
    out << "null,\n";
  }
  out << "  \"gpu_stuck\": " << (gpu_stuck ? "true" : "false") << ",\n";
  out << "  \"trail\": [";
  for (size_t i = 0; i < trail.size(); ++i) {
    if (i != 0u) out << ", ";
    out << "{\"stage\": " << str(trail[i].first) << ", \"hr\": " << str(hex(trail[i].second)) << "}";
  }
  out << "],\n";
  out << "  \"errors\": [";
  for (size_t i = 0; i < errors.size(); ++i) {
    if (i != 0u) out << ", ";
    out << "{\"stage\": " << str(errors[i].first) << ", \"hr\": " << str(hex(errors[i].second)) << "}";
  }
  out << "]}\n";

  std::string text = out.str();
  const std::filesystem::path json_path = bvh::PoolOutputDir() / json_name;
  renodx::utils::path::WriteTextFile(json_path, text);
  std::error_code size_ec;
  const auto json_size = std::filesystem::file_size(json_path, size_ec);
  if (size_ec || json_size == 0u) {
    line += " (json write failed)";
  }
  renodx::utils::log::i("[world-rt12] ", line);
  std::lock_guard<std::mutex> lock(g_rt12_probe.mutex);
  g_rt12_probe.result = line;
#else
  std::lock_guard<std::mutex> lock(g_rt12_probe.mutex);
  g_rt12_probe.result = "DX12 probe: not Windows";
#endif
}

}  // namespace falcom_world::rt12
