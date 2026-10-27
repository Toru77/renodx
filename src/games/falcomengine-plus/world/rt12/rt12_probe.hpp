#pragma once

// One-shot D3D12 side-device probe (M2a), diagnostic only.

#include <Windows.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <mutex>
#include <sstream>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#if defined(_WIN32)
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
  uint32_t shader_model = 0u;
  bool dxr11 = false;
  bool dxr12 = false;
  uint64_t timestamp_frequency = 0u;
  bool fence_ran = false;
  std::vector<Iteration> iterations;
  bool texture_ran = false;
  bool texture_ok = false;
  HRESULT texture_hr = S_OK;
  bool buffer_ran = false;
  bool buffer_ok = false;
  HRESULT buffer_hr = S_OK;
  uint32_t byte_width = 0u;
  uint32_t bind_flags = 0u;

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
    hr = adapter->CheckInterfaceSupport(__uuidof(ID3D11Device), &driver);
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
    if (queries_ok) {
      event = CreateEventW(nullptr, FALSE, FALSE, nullptr);
      note("CreateEvent", event != nullptr ? S_OK : HRESULT_FROM_WIN32(GetLastError()));
    }
    const bool round_trip = queries_ok && event != nullptr;
    auto to_us = [&qpc_frequency](const LARGE_INTEGER& from, const LARGE_INTEGER& to) {
      return static_cast<double>(to.QuadPart - from.QuadPart) * 1e6 / static_cast<double>(qpc_frequency.QuadPart);
    };
    auto poll_query = [&](ID3D11Query* query, void* data, UINT size) {
      LARGE_INTEGER start{};
      QueryPerformanceCounter(&start);
      HRESULT poll_hr = ctx->GetData(query, data, size, 0);
      while (poll_hr == S_FALSE) {
        LARGE_INTEGER now{};
        QueryPerformanceCounter(&now);
        if (now.QuadPart - start.QuadPart >= qpc_frequency.QuadPart) break;
        poll_hr = ctx->GetData(query, data, size, 0);
      }
      return poll_hr == S_FALSE ? HRESULT_FROM_WIN32(WAIT_TIMEOUT) : poll_hr;
    };
    if (round_trip) {
      for (uint32_t i = 0; i < kIterations; ++i) {
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
          const DWORD wait_result = WaitForSingleObject(event, 1000);
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
            const DWORD recovery_result = WaitForSingleObject(event, 1000);
            it.recovered = (recovery_result == WAIT_OBJECT_0);
            if (!it.recovered) note("fence recovery wait", HRESULT_FROM_WIN32(recovery_result));
          }
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
    if (event != nullptr) CloseHandle(event);

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

    // 11. Shared buffer, D3D12 to D3D11.
    buffer_ran = true;
    D3D12_HEAP_PROPERTIES heap_properties{};
    heap_properties.Type = D3D12_HEAP_TYPE_DEFAULT;
    D3D12_RESOURCE_DESC buffer_desc12{};
    buffer_desc12.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    buffer_desc12.Width = 65536u;
    buffer_desc12.Height = 1u;
    buffer_desc12.DepthOrArraySize = 1u;
    buffer_desc12.MipLevels = 1u;
    buffer_desc12.SampleDesc.Count = 1u;
    buffer_desc12.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    buffer_desc12.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    ComPtr<ID3D12Resource> buffer12;
    ComPtr<ID3D11Buffer> buffer11;
    HANDLE buffer_handle = nullptr;
    hr = native12->CreateCommittedResource(&heap_properties, D3D12_HEAP_FLAG_SHARED, &buffer_desc12,
                                           D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&buffer12));
    note("CreateCommittedResource", hr);
    if (SUCCEEDED(hr)) {
      hr = native12->CreateSharedHandle(buffer12.Get(), nullptr, GENERIC_ALL, nullptr, &buffer_handle);
      note("buffer CreateSharedHandle", hr);
    }
    if (SUCCEEDED(hr)) {
      hr = device1->OpenSharedResource1(buffer_handle, IID_PPV_ARGS(&buffer11));
      note("buffer OpenSharedResource1", hr);
    }
    if (buffer_handle != nullptr) CloseHandle(buffer_handle);
    buffer_ok = SUCCEEDED(hr);
    buffer_hr = hr;
    if (buffer_ok) {
      D3D11_BUFFER_DESC buffer_desc11{};
      buffer11->GetDesc(&buffer_desc11);
      byte_width = buffer_desc11.ByteWidth;
      bind_flags = buffer_desc11.BindFlags;
      renodx::utils::log::i("[world-rt12] shared buffer: ByteWidth ", byte_width, ", BindFlags ",
                            renodx::utils::log::AsHex(bind_flags));
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

  const std::string json_name = "dx12_probe_f" + std::to_string(frame) + ".json";
  std::string line;
  if (completed) {
    line = "DX12 probe f" + std::to_string(frame) + ": runtime " + runtime + ", wrapped " + (wrapped ? "yes" : "no")
           + ", tier " + std::to_string(tier) + ", SM " + renodx::utils::log::AsHex(shader_model) + ", DXR1.1 "
           + (dxr11 ? "yes" : "no") + ", DXR1.2 " + (dxr12 ? "yes" : "no") + ", fence gpu us "
           + stats_text(gpu_stats) + ", submit us " + stats_text(submit_stats) + ", roundtrip us "
           + stats_text(roundtrip_stats) + ", tex share " + (texture_ok ? std::string("ok") : hex(texture_hr))
           + ", buf share " + (buffer_ok ? std::string("ok") : hex(buffer_hr)) + ", errors "
           + std::to_string(errors.size()) + ", json " + json_name;
  } else {
    line = "DX12 probe f" + std::to_string(frame) + ": " + trail.back().first + " failed hr=" + hex(trail.back().second)
           + ", errors " + std::to_string(errors.size()) + ", json " + json_name;
  }

  std::ostringstream out;
  out << "{\"schema\": 1, \"frame\": " << frame << ", \"result\": " << str(line) << ",\n";
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
        << ", \"dxr11\": " << (dxr11 ? "true" : "false") << ", \"dxr12\": " << (dxr12 ? "true" : "false") << "},\n";
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
  out << ", \"buffer\": ";
  if (buffer_ran) {
    out << "{\"ok\": " << (buffer_ok ? "true" : "false") << ", \"hr\": " << str(hex(buffer_hr)) << ", \"byte_width\": "
        << (buffer_ok ? std::to_string(byte_width) : std::string("null")) << ", \"bind_flags\": "
        << (buffer_ok ? str(renodx::utils::log::AsHex(bind_flags)) : std::string("null")) << "}";
  } else {
    out << "null";
  }
  out << "},\n";
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
