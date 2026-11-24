#include <d3d12.h>
#include <dxgi1_2.h>
#include <wrl/client.h>

#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>
#include <span>
#include <thread>
#include <vector>

#include "../rtxdi_config.hpp"
#include "../rtxdi_di_layout.h"
#include "../rtxdi_frame_parameters.h"
#include "../rtxdi_input_layout.h"
#include "../rtxdi_surface_layout.h"
#include "rtxdi_raw_guard_contract.h"
#include "rtxdi_surface_contract.h"
#include "world_light_table.h"
#include "world_rtxdi_classify_lights.h"

using Microsoft::WRL::ComPtr;

// Standalone test executable: provides the addon NAME symbol that the shared headers expect.
extern "C" __declspec(dllexport) const char* const NAME = "falcom-rtxdi-bridge";

namespace {
using Clock = std::chrono::steady_clock;

struct WorldMeshGPU {
  float header[4];
  float bbox_min[4];
  float bbox_max[4];
  float build[4];
};
struct WorldInstanceGPU {
  float header[4];
  float world[4][4];
  float inverse_world[4][4];
  float bounds_min[4];
  float bounds_max[4];
  float visibility[4];
};
struct AlphaMaterialGPU {
  float threshold;
  float scroll[2];
  uint32_t swizzle;
  uint32_t slice;
  uint32_t pad[3];
};
struct SunResult {
  uint32_t status;
  float distance;
  uint32_t reason;
  uint32_t flags;
};
struct SurfaceResult {
  float position[3];
  float normal[3];
  float view[3];
  float linear_depth;
  uint32_t flags;
  uint32_t rejection;
};
struct RtxdiLightParam {
  float pos[3];
  float radius;
  float color[3];
  float radius_inv;
  float chara_color[3];
  float attenuation;
  float vec[3];
  float spot_angle_inv;
  float attenuation_angle;
  float specular_intensity;
  float specular_glossiness;
  float scatter_anisotropy;
  float scatter_color[3];
  float scatter_density;
  float translucency;
  int32_t shadowmap_index;
  float scatter_offset;
  float user_param;
};
struct RtxdiTileData {
  int32_t point_indices[63];
  uint32_t point_count;
  int32_t spot_indices[63];
  uint32_t spot_count;
  int32_t probe_indices[14];
  uint32_t probe_count;
  float tile_depth_inv;
};
struct DeferredParam {
  uint8_t bytes[RTXDI_INPUT_DEFERRED_STRIDE];
};
struct LightingConsumerCase {
  uint32_t tuple[4];
  uint32_t light_count;
  uint32_t bound;
  uint32_t padding[2];
  uint32_t original_point_count;
  uint32_t original_spot_count;
  float lamp_radiance;
  float base_color;
  float dynamic_intensity;
  float padding2;
  RtxdiTileData tile;
  uint32_t sun_bound;
  uint32_t sun_status;
  float csm_visibility;
  float sun_transmission;
  float nrd_diffuse[4];
  uint32_t nrd_bound;
  uint32_t expected_class;
  float raw_specular;
  float sun_wind_transmission;
  uint32_t debug_view;
  uint32_t debug_flags;
  uint32_t debug_status;
  uint32_t debug_consumer_decision;
  float debug_value[4];
  uint32_t debug_tuple[4];
  float debug_filtered[4];
  uint32_t debug_sun_status;
  uint32_t debug_pixel[2];
  uint32_t debug_padding;
  float pack_distance;
  uint32_t pack_status;
  uint32_t pack_flags;
  uint32_t pack_enabled;
};
struct LightingConsumerResult {
  uint32_t mode;
  uint32_t point_count;
  uint32_t spot_count;
  float inverse_pdf;
  float raw_local_term;
  float shaded_local_term;
  uint32_t selected_index;
  uint32_t selected_type;
  uint32_t sun_known;
  float environment_sun_visibility;
  float character_sun_multiplier;
  uint32_t padding;
  uint32_t character_mode;
  uint32_t character_type;
  float character_term;
  uint32_t denoised_used;
  float diffuse_replaced;
  float specular_preserved;
  uint32_t padding2;
  float debug_color[3];
  uint32_t padding3;
  float debug_packed_distance[4];
};
static_assert(sizeof(WorldMeshGPU) == 64);
static_assert(sizeof(WorldInstanceGPU) == 192);
static_assert(sizeof(AlphaMaterialGPU) == 32);
static_assert(sizeof(SunResult) == RTXDI_SUN_RESULT_STRIDE);
static_assert(sizeof(SurfaceResult) == 48);
static_assert(sizeof(RtxdiLightParam) == RTXDI_INPUT_LIGHT_STRIDE);
static_assert(sizeof(RtxdiTileData) == RTXDI_INPUT_TILE_STRIDE);
static_assert(sizeof(DeferredParam) == RTXDI_INPUT_DEFERRED_STRIDE);
static_assert(sizeof(LightingConsumerCase) == 776u);
static_assert(sizeof(LightingConsumerResult) == 108u);

bool Check(HRESULT hr, const char* operation) {
  if (FAILED(hr)) std::fprintf(stderr, "FAIL DXR %s: HRESULT 0x%08lx\n", operation, static_cast<unsigned long>(hr));
  return SUCCEEDED(hr);
}

void DumpDred(ID3D12Device* device) {
  ComPtr<ID3D12DeviceRemovedExtendedData1> dred;
  if (FAILED(device->QueryInterface(IID_PPV_ARGS(&dred)))) {
    std::fprintf(stderr, "DRED interface unavailable\n");
    return;
  }
  D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT1 breadcrumbs = {};
  D3D12_DRED_PAGE_FAULT_OUTPUT1 page_fault = {};
  if (SUCCEEDED(dred->GetAutoBreadcrumbsOutput1(&breadcrumbs))) {
    for (auto* node = breadcrumbs.pHeadAutoBreadcrumbNode; node != nullptr; node = node->pNext) {
      std::fprintf(stderr, "DRED list=%s queue=%s breadcrumbs=%u last=%u\n",
                   node->pCommandListDebugNameA ? node->pCommandListDebugNameA : "(unnamed)",
                   node->pCommandQueueDebugNameA ? node->pCommandQueueDebugNameA : "(unnamed)",
                   node->BreadcrumbCount, node->pLastBreadcrumbValue ? *node->pLastBreadcrumbValue : 0u);
    }
  }
  if (SUCCEEDED(dred->GetPageFaultAllocationOutput1(&page_fault))) {
    std::fprintf(stderr, "DRED page fault VA=0x%llx\n", static_cast<unsigned long long>(page_fault.PageFaultVA));
    for (auto* node = page_fault.pHeadExistingAllocationNode; node != nullptr; node = node->pNext) {
      std::fprintf(stderr, "DRED existing allocation %s type=%u\n", node->ObjectNameA ? node->ObjectNameA : "(unnamed)",
                   static_cast<unsigned>(node->AllocationType));
    }
    for (auto* node = page_fault.pHeadRecentFreedAllocationNode; node != nullptr; node = node->pNext) {
      std::fprintf(stderr, "DRED freed allocation %s type=%u\n", node->ObjectNameA ? node->ObjectNameA : "(unnamed)",
                   static_cast<unsigned>(node->AllocationType));
    }
  }
}

void DumpDebugMessages(ID3D12Device* device) {
  ComPtr<ID3D12InfoQueue> info;
  if (FAILED(device->QueryInterface(IID_PPV_ARGS(&info)))) return;
  for (UINT64 index = 0; index < info->GetNumStoredMessages(); ++index) {
    SIZE_T size = 0;
    info->GetMessage(index, nullptr, &size);
    std::vector<uint8_t> storage(size);
    auto* message = reinterpret_cast<D3D12_MESSAGE*>(storage.data());
    if (SUCCEEDED(info->GetMessage(index, message, &size))) {
      std::fprintf(stderr, "D3D12 debug[%llu] id=%d: %s\n", static_cast<unsigned long long>(index),
                   static_cast<int>(message->ID), message->pDescription);
    }
  }
}

bool ValidateDebugMessages(ID3D12InfoQueue* info) {
  UINT warning_count = 0;
  for (UINT64 index = 0; index < info->GetNumStoredMessages(); ++index) {
    SIZE_T size = 0;
    if (FAILED(info->GetMessage(index, nullptr, &size))) return false;
    std::vector<uint8_t> storage(size);
    auto* message = reinterpret_cast<D3D12_MESSAGE*>(storage.data());
    if (FAILED(info->GetMessage(index, message, &size))) return false;
    if (message->Severity <= D3D12_MESSAGE_SEVERITY_WARNING) {
      ++warning_count;
      std::fprintf(stderr, "FAIL D3D12 debug[%llu] severity=%u id=%d: %s\n", static_cast<unsigned long long>(index),
                   static_cast<unsigned>(message->Severity), static_cast<int>(message->ID), message->pDescription);
    }
  }
  if (warning_count != 0u) {
    std::fprintf(stderr, "FAIL D3D12 InfoQueue reported %u warning/error messages\n", warning_count);
    return false;
  }
  std::printf("PASS D3D12 InfoQueue clean (%llu messages, no warnings/errors)\n",
              static_cast<unsigned long long>(info->GetNumStoredMessages()));
  return true;
}

float HalfToFloat(uint16_t value) {
  const uint32_t sign = static_cast<uint32_t>(value & 0x8000u) << 16u;
  uint32_t exponent = (value >> 10u) & 0x1fu;
  uint32_t mantissa = value & 0x3ffu;
  uint32_t bits = sign;
  if (exponent == 0u) {
    if (mantissa != 0u) {
      int32_t adjusted_exponent = -14;
      while ((mantissa & 0x400u) == 0u) {
        mantissa <<= 1u;
        --adjusted_exponent;
      }
      mantissa &= 0x3ffu;
      bits |= static_cast<uint32_t>(adjusted_exponent + 127) << 23u;
      bits |= mantissa << 13u;
    }
  } else if (exponent == 0x1fu) {
    bits |= 0x7f800000u | (mantissa << 13u);
  } else {
    bits |= (exponent + 112u) << 23u;
    bits |= mantissa << 13u;
  }
  return std::bit_cast<float>(bits);
}

D3D12_HEAP_PROPERTIES Heap(D3D12_HEAP_TYPE type) {
  D3D12_HEAP_PROPERTIES result = {};
  result.Type = type;
  result.CreationNodeMask = 1;
  result.VisibleNodeMask = 1;
  return result;
}

D3D12_RESOURCE_DESC BufferDesc(UINT64 bytes, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE) {
  D3D12_RESOURCE_DESC result = {};
  result.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  result.Width = bytes;
  result.Height = 1;
  result.DepthOrArraySize = 1;
  result.MipLevels = 1;
  result.SampleDesc.Count = 1;
  result.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  result.Flags = flags;
  return result;
}

UINT64 AlignAccelerationStructureSize(UINT64 size) {
  return (size + D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT - 1)
         & ~(static_cast<UINT64>(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT) - 1);
}

bool UploadBuffer(ID3D12Device* device, const void* bytes, UINT64 size, ComPtr<ID3D12Resource>* resource) {
  const auto properties = Heap(D3D12_HEAP_TYPE_UPLOAD);
  const auto desc = BufferDesc(size);
  if (!Check(device->CreateCommittedResource(&properties, D3D12_HEAP_FLAG_NONE, &desc,
                                             D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&(*resource))),
             "create upload buffer")) return false;
  void* mapped = nullptr;
  if (!Check((*resource)->Map(0, nullptr, &mapped), "map upload buffer")) return false;
  std::memcpy(mapped, bytes, static_cast<size_t>(size));
  (*resource)->Unmap(0, nullptr);
  return true;
}

bool CreateDefaultBuffer(ID3D12Device* device, UINT64 size, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES state,
                         ComPtr<ID3D12Resource>* output) {
  const auto properties = Heap(D3D12_HEAP_TYPE_DEFAULT);
  const auto desc = BufferDesc(size, flags);
  const HRESULT hr = device->CreateCommittedResource(&properties, D3D12_HEAP_FLAG_NONE, &desc, state,
                                                     nullptr, IID_PPV_ARGS(&(*output)));
  if (FAILED(hr)) {
    std::fprintf(stderr, "FAIL create default buffer size=%llu flags=%u state=%u result=0x%08lx removed=0x%08lx\n",
                 static_cast<unsigned long long>(size), static_cast<unsigned>(flags), static_cast<unsigned>(state),
                 static_cast<unsigned long>(hr), static_cast<unsigned long>(device->GetDeviceRemovedReason()));
    DumpDred(device);
    DumpDebugMessages(device);
  }
  return SUCCEEDED(hr);
}

bool WaitFor(ID3D12Device* device, ID3D12Fence* fence, UINT64 value, HANDLE event) {
  ResetEvent(event);
  if (fence->GetCompletedValue() < value && FAILED(fence->SetEventOnCompletion(value, event))) return false;
  if (fence->GetCompletedValue() < value && WaitForSingleObject(event, 10000) != WAIT_OBJECT_0) return false;
  return fence->GetCompletedValue() >= value && SUCCEEDED(device->GetDeviceRemovedReason());
}

bool CreateTexture(ID3D12Device* device, DXGI_FORMAT format, UINT width, UINT height, UINT layers,
                   D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES state, ComPtr<ID3D12Resource>* texture) {
  D3D12_RESOURCE_DESC desc = {};
  desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
  desc.Width = width;
  desc.Height = height;
  desc.DepthOrArraySize = static_cast<UINT16>(layers);
  desc.MipLevels = 1;
  desc.Format = format;
  desc.SampleDesc.Count = 1;
  desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
  desc.Flags = flags;
  const auto properties = Heap(D3D12_HEAP_TYPE_DEFAULT);
  return Check(device->CreateCommittedResource(&properties, D3D12_HEAP_FLAG_NONE, &desc, state,
                                               nullptr, IID_PPV_ARGS(&(*texture))),
               "create texture");
}

bool CreateBufferSrv(ID3D12Device* device, ID3D12DescriptorHeap* heap, UINT increment, UINT slot,
                     ID3D12Resource* resource, UINT elements, UINT stride) {
  D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
  desc.Format = DXGI_FORMAT_UNKNOWN;
  desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
  desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  desc.Buffer.NumElements = elements;
  desc.Buffer.StructureByteStride = stride;
  auto handle = heap->GetCPUDescriptorHandleForHeapStart();
  handle.ptr += static_cast<SIZE_T>(slot) * increment;
  device->CreateShaderResourceView(resource, &desc, handle);
  return true;
}

bool CreateRoot(ID3D12Device* device, const D3D12_DESCRIPTOR_RANGE* ranges, UINT range_count, UINT constants,
                ComPtr<ID3D12RootSignature>* root) {
  D3D12_ROOT_PARAMETER params[2] = {};
  params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
  params[0].Constants.Num32BitValues = constants;
  params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
  params[1].DescriptorTable.NumDescriptorRanges = range_count;
  params[1].DescriptorTable.pDescriptorRanges = ranges;
  D3D12_ROOT_SIGNATURE_DESC desc = {};
  desc.NumParameters = 2;
  desc.pParameters = params;
  ComPtr<ID3DBlob> blob;
  ComPtr<ID3DBlob> errors;
  if (!Check(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &errors), "serialize root signature")) {
    if (errors) std::fprintf(stderr, "%s\n", static_cast<const char*>(errors->GetBufferPointer()));
    return false;
  }
  return Check(device->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&(*root))),
               "create root signature");
}

bool CreatePso(ID3D12Device* device, ID3D12RootSignature* root, std::span<const uint8_t> shader,
               ComPtr<ID3D12PipelineState>* pso) {
  D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};
  desc.pRootSignature = root;
  desc.CS.pShaderBytecode = shader.data();
  desc.CS.BytecodeLength = shader.size();
  return Check(device->CreateComputePipelineState(&desc, IID_PPV_ARGS(&(*pso))), "create compute PSO");
}

bool CreateTextureUpload(ID3D12Device* device, ID3D12GraphicsCommandList* list, DXGI_FORMAT format, UINT width,
                         UINT height, const void* input, UINT row_bytes, ComPtr<ID3D12Resource>* texture,
                         ComPtr<ID3D12Resource>* upload) {
  if (!CreateTexture(device, format, width, height, 1, D3D12_RESOURCE_FLAG_NONE,
                     D3D12_RESOURCE_STATE_COPY_DEST, texture)) return false;
  const auto desc = (*texture)->GetDesc();
  D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint = {};
  UINT rows = 0;
  UINT64 row_size = 0;
  UINT64 total = 0;
  device->GetCopyableFootprints(&desc, 0, 1, 0, &footprint, &rows, &row_size, &total);
  const auto upload_properties = Heap(D3D12_HEAP_TYPE_UPLOAD);
  const auto upload_desc = BufferDesc(total);
  if (!Check(device->CreateCommittedResource(&upload_properties, D3D12_HEAP_FLAG_NONE, &upload_desc,
                                             D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&(*upload))),
             "create texture upload")) return false;
  uint8_t* mapped = nullptr;
  if (!Check((*upload)->Map(0, nullptr, reinterpret_cast<void**>(&mapped)), "map texture upload")) return false;
  for (UINT y = 0; y < height; ++y) {
    std::memcpy(mapped + footprint.Offset + static_cast<size_t>(y) * footprint.Footprint.RowPitch,
                static_cast<const uint8_t*>(input) + static_cast<size_t>(y) * row_bytes, row_bytes);
  }
  (*upload)->Unmap(0, nullptr);
  D3D12_TEXTURE_COPY_LOCATION source = {};
  source.pResource = upload->Get();
  source.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
  source.PlacedFootprint = footprint;
  D3D12_TEXTURE_COPY_LOCATION dest = {};
  dest.pResource = texture->Get();
  dest.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
  list->CopyTextureRegion(&dest, 0, 0, 0, &source, nullptr);
  D3D12_RESOURCE_BARRIER barrier = {};
  barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  barrier.Transition.pResource = texture->Get();
  barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
  barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
  list->ResourceBarrier(1, &barrier);
  return true;
}

}  // namespace

int RunRtxdiDxrBridge() {
  ComPtr<ID3D12Debug1> debug;
  if (!Check(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)), "enable debug layer")) return 1;
  debug->EnableDebugLayer();
  ComPtr<ID3D12DeviceRemovedExtendedDataSettings1> dred_settings;
  if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&dred_settings)))) {
    dred_settings->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
    dred_settings->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
  }
  ComPtr<IDXGIFactory1> factory;
  ComPtr<IDXGIAdapter1> adapter;
  ComPtr<ID3D12Device> base_device;
  if (!Check(CreateDXGIFactory1(IID_PPV_ARGS(&factory)), "create DXGI factory")) return 1;
  HRESULT adapter_result = factory->EnumAdapters1(0, &adapter);
  if (adapter_result == DXGI_ERROR_NOT_FOUND) {
    std::fprintf(stderr, "SKIP DXR adapter unavailable\n");
    return 77;
  }
  if (!Check(adapter_result, "find DXR adapter")) return 1;
  HRESULT hr = D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&base_device));
  if (FAILED(hr)) {
    std::fprintf(stderr, "SKIP DXR unavailable: 0x%08lx\n", static_cast<unsigned long>(hr));
    return 77;
  }
  D3D12_FEATURE_DATA_D3D12_OPTIONS5 options = {};
  D3D12_FEATURE_DATA_SHADER_MODEL model = {D3D_SHADER_MODEL_6_5};
  if (FAILED(base_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options, sizeof(options)))
      || options.RaytracingTier < D3D12_RAYTRACING_TIER_1_1
      || FAILED(base_device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &model, sizeof(model)))
      || model.HighestShaderModel < D3D_SHADER_MODEL_6_5) {
    std::fprintf(stderr, "SKIP DXR 1.1 / SM 6.5 unavailable\n");
    return 77;
  }
  ComPtr<ID3D12Device5> device;
  if (!Check(base_device.As(&device), "query ID3D12Device5")) return 1;
  ComPtr<ID3D12InfoQueue> info_queue;
  if (!Check(base_device.As(&info_queue), "query D3D12 InfoQueue")) return 1;

  D3D12_COMMAND_QUEUE_DESC queue_desc = {};
  queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
  ComPtr<ID3D12CommandQueue> queue;
  ComPtr<ID3D12CommandAllocator> allocator;
  ComPtr<ID3D12GraphicsCommandList> base_list;
  ComPtr<ID3D12GraphicsCommandList4> list;
  ComPtr<ID3D12Fence> fence;
  HANDLE event = CreateEventW(nullptr, FALSE, FALSE, nullptr);
  if (event == nullptr
      || !Check(device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&queue)), "create queue")
      || !Check(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator)), "create allocator")
      || !Check(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator.Get(), nullptr, IID_PPV_ARGS(&base_list)), "create list")
      || !Check(base_list.As(&list), "query ID3D12GraphicsCommandList4")
      || !Check(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)), "create fence")) return 1;
  const char* wait_context = "startup";
  bool wait_has_di_dispatch = false;
  const auto submit_wait = [&]() {
    const HRESULT close_hr = list->Close();
    if (!Check(close_hr, "close list")) {
      std::fprintf(stderr, "FAIL close context=%s device-removed=0x%08lx\n", wait_context,
                   static_cast<unsigned long>(device->GetDeviceRemovedReason()));
      DumpDebugMessages(device.Get());
      return false;
    }
    ID3D12CommandList* lists[] = {list.Get()};
    static UINT64 fence_value = 1;
    const UINT64 signal_value = fence_value++;
    queue->ExecuteCommandLists(1, lists);
    if (!Check(queue->Signal(fence.Get(), signal_value), "signal fence")) return false;
    const bool done = WaitFor(device.Get(), fence.Get(), signal_value, event);
    if (!done) {
      std::fprintf(stderr, "FAIL DXR wait value=%llu completed=%llu context=%s di-dispatch=%s removed=0x%08lx\n",
                   static_cast<unsigned long long>(signal_value),
                   static_cast<unsigned long long>(fence->GetCompletedValue()),
                   wait_context, wait_has_di_dispatch ? "yes" : "no",
                   static_cast<unsigned long>(device->GetDeviceRemovedReason()));
      DumpDred(device.Get());
      DumpDebugMessages(device.Get());
    }
    wait_has_di_dispatch = false;
    return done;
  };

  {
    constexpr UINT light_count = 4u;
    constexpr UINT diagnostic_count = RTXDI_DIAG_COUNTER_COUNT;
    std::array<RtxdiLightParam, light_count> light_data = {};
    light_data[0].pos[1] = -1.f / 256.f;
    light_data[0].pos[2] = 1.f;
    light_data[0].radius = 0.5f;
    light_data[0].radius_inv = 2.f;
    light_data[0].color[0] = 1.f;
    light_data[0].color[2] = -1.f;
    light_data[0].spot_angle_inv = 1.f;
    light_data[0].attenuation_angle = 1.f;
    light_data[0].vec[2] = 1.f;
    light_data[1].pos[0] = 2.f;
    light_data[1].radius = 1.f;
    light_data[1].radius_inv = 1.f;
    light_data[1].color[1] = 0.5f;
    light_data[2].pos[0] = std::numeric_limits<float>::infinity();
    light_data[2].radius = 1.f;
    light_data[2].radius_inv = 1.f;
    light_data[3].pos[0] = 9000000.f;
    light_data[3].radius = 1.f;
    light_data[3].radius_inv = 1.f;
    const std::array<uint32_t, light_count> light_types = {RTXDI_LIGHT_TABLE_TYPE_SPOT,
                                                          RTXDI_LIGHT_TABLE_TYPE_POINT,
                                                          RTXDI_LIGHT_TABLE_TYPE_AMBIGUOUS,
                                                          RTXDI_LIGHT_TABLE_TYPE_UNTYPED};
    ComPtr<ID3D12Resource> current_lights, current_types, previous_lights, previous_table, current_table_dummy;
    if (!UploadBuffer(device.Get(), light_data.data(), sizeof(light_data), &current_lights)
        || !UploadBuffer(device.Get(), light_types.data(), sizeof(light_types), &current_types)
        || !UploadBuffer(device.Get(), light_data.data(), sizeof(light_data), &previous_lights)) return 1;
    std::array<RtxdiLightTableEntry, light_count> dummy_table = {};
    if (!UploadBuffer(device.Get(), dummy_table.data(), sizeof(dummy_table), &previous_table)
        || !UploadBuffer(device.Get(), dummy_table.data(), sizeof(dummy_table), &current_table_dummy)) return 1;
    const std::array<uint32_t, diagnostic_count> zero_diagnostics = {};
    std::array<uint32_t, light_count> reset_maps = {};
    reset_maps.fill(RTXDI_LIGHT_TABLE_INVALID_INDEX);
    ComPtr<ID3D12Resource> diagnostic_zero_upload;
    ComPtr<ID3D12Resource> map_reset_upload;
    if (!UploadBuffer(device.Get(), zero_diagnostics.data(), sizeof(zero_diagnostics), &diagnostic_zero_upload)
        || !UploadBuffer(device.Get(), reset_maps.data(), sizeof(reset_maps), &map_reset_upload)) return 1;
    ComPtr<ID3D12Resource> output_table, previous_to_current, current_to_previous, diagnostics;
    if (!CreateDefaultBuffer(device.Get(), sizeof(RtxdiLightTableEntry) * light_count,
                             D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON, &output_table)
        || !CreateDefaultBuffer(device.Get(), sizeof(uint32_t) * light_count,
                                D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON, &previous_to_current)
        || !CreateDefaultBuffer(device.Get(), sizeof(uint32_t) * light_count,
                                D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON, &current_to_previous)
        || !CreateDefaultBuffer(device.Get(), sizeof(uint32_t) * diagnostic_count,
                                D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON, &diagnostics)) return 1;
    D3D12_DESCRIPTOR_HEAP_DESC table_heap_desc = {};
    table_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    table_heap_desc.NumDescriptors = 9u;
    table_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ComPtr<ID3D12DescriptorHeap> table_heap;
    if (!Check(device->CreateDescriptorHeap(&table_heap_desc, IID_PPV_ARGS(&table_heap)), "create light table heap")) return 1;
    const UINT table_descriptor_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    CreateBufferSrv(device.Get(), table_heap.Get(), table_descriptor_size, 0u, current_lights.Get(), light_count, sizeof(RtxdiLightParam));
    CreateBufferSrv(device.Get(), table_heap.Get(), table_descriptor_size, 1u, current_types.Get(), light_count, sizeof(uint32_t));
    CreateBufferSrv(device.Get(), table_heap.Get(), table_descriptor_size, 2u, previous_lights.Get(), light_count, sizeof(RtxdiLightParam));
    CreateBufferSrv(device.Get(), table_heap.Get(), table_descriptor_size, 3u, previous_table.Get(), light_count, sizeof(RtxdiLightTableEntry));
    CreateBufferSrv(device.Get(), table_heap.Get(), table_descriptor_size, 4u, current_table_dummy.Get(), light_count, sizeof(RtxdiLightTableEntry));
    const auto create_table_uav = [&](UINT descriptor_index, ID3D12Resource* resource, UINT elements, UINT stride) {
      D3D12_UNORDERED_ACCESS_VIEW_DESC desc = {};
      desc.Format = DXGI_FORMAT_UNKNOWN;
      desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
      desc.Buffer.NumElements = elements;
      desc.Buffer.StructureByteStride = stride;
      auto handle = table_heap->GetCPUDescriptorHandleForHeapStart();
      handle.ptr += static_cast<SIZE_T>(descriptor_index) * table_descriptor_size;
      device->CreateUnorderedAccessView(resource, nullptr, &desc, handle);
    };
    create_table_uav(5u, output_table.Get(), light_count, sizeof(RtxdiLightTableEntry));
    create_table_uav(6u, previous_to_current.Get(), light_count, sizeof(uint32_t));
    create_table_uav(7u, current_to_previous.Get(), light_count, sizeof(uint32_t));
    create_table_uav(8u, diagnostics.Get(), diagnostic_count, sizeof(uint32_t));
    D3D12_DESCRIPTOR_RANGE table_ranges[2] = {};
    table_ranges[0] = {D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 5u, 0u, 0u, 0u};
    table_ranges[1] = {D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 4u, 0u, 0u, 5u};
    ComPtr<ID3D12RootSignature> table_root;
    ComPtr<ID3D12PipelineState> table_pso;
    if (!CreateRoot(device.Get(), table_ranges, 2u, 4u, &table_root)
        || !CreatePso(device.Get(), table_root.Get(), __world_light_table, &table_pso)) return 1;
    const auto table_readback_desc = BufferDesc(sizeof(RtxdiLightTableEntry) * light_count);
    const auto diagnostic_readback_desc = BufferDesc(sizeof(uint32_t) * diagnostic_count);
    const auto readback_heap = Heap(D3D12_HEAP_TYPE_READBACK);
    ComPtr<ID3D12Resource> table_readback, diagnostic_readback;
    if (!Check(device->CreateCommittedResource(&readback_heap, D3D12_HEAP_FLAG_NONE, &table_readback_desc,
                                                D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&table_readback)),
               "create light table readback")
        || !Check(device->CreateCommittedResource(&readback_heap, D3D12_HEAP_FLAG_NONE, &diagnostic_readback_desc,
                                                  D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&diagnostic_readback)),
                 "create light table diagnostics readback")) return 1;
    std::array<D3D12_RESOURCE_BARRIER, 4> table_barriers = {};
    const std::array<ID3D12Resource*, 4> table_outputs = {output_table.Get(), previous_to_current.Get(),
                                                        current_to_previous.Get(), diagnostics.Get()};
    for (UINT index = 0u; index < table_outputs.size(); ++index) {
      table_barriers[index].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
      table_barriers[index].Transition.pResource = table_outputs[index];
      table_barriers[index].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
      table_barriers[index].Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
      table_barriers[index].Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    }
    list->ResourceBarrier(static_cast<UINT>(table_barriers.size()), table_barriers.data());
    ID3D12DescriptorHeap* table_heaps[] = {table_heap.Get()};
    list->SetDescriptorHeaps(1u, table_heaps);
    list->SetComputeRootSignature(table_root.Get());
    list->SetPipelineState(table_pso.Get());
    list->SetComputeRoot32BitConstants(0u, 4u, std::array<UINT, 4>{light_count, 0u, 0u, 0u}.data(), 0u);
    list->SetComputeRootDescriptorTable(1u, table_heap->GetGPUDescriptorHandleForHeapStart());
    D3D12_RESOURCE_BARRIER diagnostic_reset = {};
    diagnostic_reset.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    diagnostic_reset.Transition.pResource = diagnostics.Get();
    diagnostic_reset.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    diagnostic_reset.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    diagnostic_reset.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
    list->ResourceBarrier(1u, &diagnostic_reset);
    list->CopyBufferRegion(diagnostics.Get(), 0u, diagnostic_zero_upload.Get(), 0u,
                           sizeof(uint32_t) * diagnostic_count);
    diagnostic_reset.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    diagnostic_reset.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    list->ResourceBarrier(1u, &diagnostic_reset);
    list->Dispatch(1u, 1u, 1u);
    for (UINT index = 0u; index < table_outputs.size(); ++index) {
      table_barriers[index] = {};
      table_barriers[index].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
      table_barriers[index].UAV.pResource = table_outputs[index];
    }
    list->ResourceBarrier(static_cast<UINT>(table_barriers.size()), table_barriers.data());
    for (UINT index = 0u; index < table_outputs.size(); ++index) {
      table_barriers[index] = {};
      table_barriers[index].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
      table_barriers[index].Transition.pResource = table_outputs[index];
      table_barriers[index].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
      table_barriers[index].Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
      table_barriers[index].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
    }
    list->ResourceBarrier(static_cast<UINT>(table_barriers.size()), table_barriers.data());
    list->CopyBufferRegion(table_readback.Get(), 0u, output_table.Get(), 0u, sizeof(RtxdiLightTableEntry) * light_count);
    list->CopyBufferRegion(diagnostic_readback.Get(), 0u, diagnostics.Get(), 0u, sizeof(uint32_t) * diagnostic_count);
    wait_context = "M2 light table build and structured-buffer readback";
    if (!submit_wait()) return 1;
    std::array<RtxdiLightTableEntry, light_count> table_results = {};
    std::array<uint32_t, diagnostic_count> table_diagnostics = {};
    void* table_mapped = nullptr;
    if (!Check(table_readback->Map(0u, nullptr, &table_mapped), "map light table readback")) return 1;
    std::memcpy(table_results.data(), table_mapped, sizeof(table_results));
    table_readback->Unmap(0u, nullptr);
    void* table_diagnostics_mapped = nullptr;
    if (!Check(diagnostic_readback->Map(0u, nullptr, &table_diagnostics_mapped), "map light table diagnostics")) return 1;
    std::memcpy(table_diagnostics.data(), table_diagnostics_mapped, sizeof(table_diagnostics));
    diagnostic_readback->Unmap(0u, nullptr);
    if (!Check(allocator->Reset(), "reset M2 build allocator")
        || !Check(base_list->Reset(allocator.Get(), nullptr), "reset M2 build list")) return 1;
    const uint64_t expected_hash = 0xc80e9739cad116a6ull;
    if (table_results[0].stable_id_lo != static_cast<uint32_t>(expected_hash)
        || table_results[0].stable_id_hi != static_cast<uint32_t>(expected_hash >> 32u)
        || table_results[0].type != RTXDI_LIGHT_TABLE_TYPE_SPOT || std::abs(table_results[0].power - 0.05315f) > 1e-6f
        || (table_results[1].stable_id_lo == 0u && table_results[1].stable_id_hi == 0u)
        || table_results[1].type != RTXDI_LIGHT_TABLE_TYPE_POINT
        || table_results[2].stable_id_lo != 0u || table_results[2].stable_id_hi != 0u
        || table_results[3].stable_id_lo != 0u || table_results[3].stable_id_hi != 0u
        || table_diagnostics[RTXDI_DIAG_LIGHT_TABLE_WRITTEN] != light_count
        || table_diagnostics[RTXDI_DIAG_LIGHT_INVALID_ID] != 1u) {
      std::fprintf(stderr, "FAIL M2 GPU table golden hash=%08x%08x expected=%016llx written=%u invalid=%u\n",
                   table_results[0].stable_id_hi, table_results[0].stable_id_lo,
                   static_cast<unsigned long long>(expected_hash),
                   table_diagnostics[RTXDI_DIAG_LIGHT_TABLE_WRITTEN], table_diagnostics[RTXDI_DIAG_LIGHT_INVALID_ID]);
      return 1;
    }
    auto previous_light_data = light_data;
    std::swap(previous_light_data[0], previous_light_data[1]);
    auto previous_table_data = table_results;
    std::swap(previous_table_data[0], previous_table_data[1]);
    void* previous_lights_mapped = nullptr;
    if (!Check(previous_lights->Map(0u, nullptr, &previous_lights_mapped), "map previous light test upload")) return 1;
    std::memcpy(previous_lights_mapped, previous_light_data.data(), sizeof(previous_light_data));
    previous_lights->Unmap(0u, nullptr);
    void* previous_table_mapped = nullptr;
    if (!Check(previous_table->Map(0u, nullptr, &previous_table_mapped), "map previous table test upload")) return 1;
    std::memcpy(previous_table_mapped, previous_table_data.data(), sizeof(previous_table_data));
    previous_table->Unmap(0u, nullptr);
    ComPtr<ID3D12Resource> match_table_dummy;
    if (!CreateDefaultBuffer(device.Get(), sizeof(RtxdiLightTableEntry) * light_count,
                             D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON,
                             &match_table_dummy)) return 1;
    CreateBufferSrv(device.Get(), table_heap.Get(), table_descriptor_size, 4u, output_table.Get(), light_count,
                    sizeof(RtxdiLightTableEntry));
    create_table_uav(5u, match_table_dummy.Get(), light_count, sizeof(RtxdiLightTableEntry));
    D3D12_RESOURCE_BARRIER match_barriers[4] = {};
    const std::array<ID3D12Resource*, 4> match_resources = {output_table.Get(), previous_to_current.Get(),
                                                            current_to_previous.Get(), diagnostics.Get()};
    const std::array<D3D12_RESOURCE_STATES, 4> match_before = {D3D12_RESOURCE_STATE_COPY_SOURCE,
                                                               D3D12_RESOURCE_STATE_COPY_SOURCE,
                                                               D3D12_RESOURCE_STATE_COPY_SOURCE,
                                                               D3D12_RESOURCE_STATE_COPY_SOURCE};
    const std::array<D3D12_RESOURCE_STATES, 4> match_after = {D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                                                              D3D12_RESOURCE_STATE_COPY_DEST,
                                                              D3D12_RESOURCE_STATE_COPY_DEST,
                                                              D3D12_RESOURCE_STATE_COPY_DEST};
    for (UINT index = 0u; index < match_resources.size(); ++index) {
      match_barriers[index].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
      match_barriers[index].Transition.pResource = match_resources[index];
      match_barriers[index].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
      match_barriers[index].Transition.StateBefore = match_before[index];
      match_barriers[index].Transition.StateAfter = match_after[index];
    }
    list->ResourceBarrier(4u, match_barriers);
    list->CopyBufferRegion(previous_to_current.Get(), 0u, map_reset_upload.Get(), 0u, sizeof(reset_maps));
    list->CopyBufferRegion(current_to_previous.Get(), 0u, map_reset_upload.Get(), 0u, sizeof(reset_maps));
    D3D12_RESOURCE_BARRIER map_state_barriers[2] = {};
    const std::array<ID3D12Resource*, 2> map_resources = {previous_to_current.Get(), current_to_previous.Get()};
    for (UINT index = 0u; index < map_resources.size(); ++index) {
      map_state_barriers[index].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
      map_state_barriers[index].Transition.pResource = map_resources[index];
      map_state_barriers[index].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
      map_state_barriers[index].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
      map_state_barriers[index].Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    }
    list->ResourceBarrier(2u, map_state_barriers);
    D3D12_RESOURCE_BARRIER dummy_barrier = {};
    dummy_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    dummy_barrier.Transition.pResource = match_table_dummy.Get();
    dummy_barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    dummy_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
    dummy_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    list->ResourceBarrier(1u, &dummy_barrier);
    list->CopyBufferRegion(diagnostics.Get(), 0u, diagnostic_zero_upload.Get(), 0u,
                           sizeof(uint32_t) * diagnostic_count);
    match_barriers[3].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    match_barriers[3].Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    list->ResourceBarrier(1u, &match_barriers[3]);
    list->SetDescriptorHeaps(1u, table_heaps);
    list->SetComputeRootSignature(table_root.Get());
    list->SetPipelineState(table_pso.Get());
    list->SetComputeRoot32BitConstants(0u, 4u, std::array<UINT, 4>{light_count, light_count, 1u, 1u}.data(), 0u);
    list->SetComputeRootDescriptorTable(1u, table_heap->GetGPUDescriptorHandleForHeapStart());
    list->Dispatch(1u, 1u, 1u);
    D3D12_RESOURCE_BARRIER match_uav_barriers[3] = {};
    for (UINT index = 0u; index < 3u; ++index) {
      match_uav_barriers[index].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
      match_uav_barriers[index].UAV.pResource = index == 0u ? match_table_dummy.Get() : match_resources[index];
    }
    list->ResourceBarrier(3u, match_uav_barriers);
    for (UINT index = 1u; index < 4u; ++index) {
      match_barriers[index] = {};
      match_barriers[index].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
      match_barriers[index].Transition.pResource = match_resources[index];
      match_barriers[index].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
      match_barriers[index].Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
      match_barriers[index].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
    }
    list->ResourceBarrier(3u, &match_barriers[1]);
    list->CopyBufferRegion(table_readback.Get(), 0u, previous_to_current.Get(), 0u,
                           sizeof(uint32_t) * light_count);
    list->CopyBufferRegion(table_readback.Get(), sizeof(uint32_t) * light_count, current_to_previous.Get(), 0u,
                           sizeof(uint32_t) * light_count);
    list->CopyBufferRegion(diagnostic_readback.Get(), 0u, diagnostics.Get(), 0u,
                           sizeof(uint32_t) * diagnostic_count);
    wait_context = "M2 reordered light map and reverse-map readback";
    if (!submit_wait()) return 1;
    std::array<uint32_t, light_count * 2u> map_results = {};
    if (!Check(table_readback->Map(0u, nullptr, &table_mapped), "map remap result")) return 1;
    std::memcpy(map_results.data(), table_mapped, sizeof(map_results));
    table_readback->Unmap(0u, nullptr);
    if (!Check(diagnostic_readback->Map(0u, nullptr, &table_diagnostics_mapped), "map remap diagnostics")) return 1;
    std::memcpy(table_diagnostics.data(), table_diagnostics_mapped, sizeof(table_diagnostics));
    diagnostic_readback->Unmap(0u, nullptr);
    const auto quantize_identity = [](const RtxdiLightParam& light, std::array<int32_t, 7>* values) {
      const float inputs[7] = {light.pos[0], light.pos[1], light.pos[2], light.radius,
                               light.color[0], light.color[1], light.color[2]};
      const float scales[7] = {256.f, 256.f, 256.f, 256.f, 1024.f, 1024.f, 1024.f};
      for (UINT component = 0u; component < 7u; ++component) {
        if (!std::isfinite(inputs[component])) return false;
        const float rounded = std::floor(inputs[component] * scales[component] + 0.5f);
        if (!std::isfinite(rounded) || rounded < -2147483648.f || rounded >= 2147483648.f) return false;
        (*values)[component] = static_cast<int32_t>(rounded);
      }
      return true;
    };
    std::array<uint32_t, light_count> cpu_previous_to_current = {}, cpu_current_to_previous = {};
    cpu_previous_to_current.fill(RTXDI_LIGHT_TABLE_INVALID_INDEX);
    cpu_current_to_previous.fill(RTXDI_LIGHT_TABLE_INVALID_INDEX);
    for (UINT previous_index = 0u; previous_index < light_count; ++previous_index) {
      std::array<int32_t, 7> previous_values = {};
      if (!quantize_identity(previous_light_data[previous_index], &previous_values)) continue;
      uint32_t occurrence = 0u;
      for (UINT earlier = 0u; earlier < previous_index; ++earlier) {
        std::array<int32_t, 7> earlier_values = {};
        if (previous_table_data[earlier].type == previous_table_data[previous_index].type
            && quantize_identity(previous_light_data[earlier], &earlier_values) && earlier_values == previous_values) ++occurrence;
      }
      uint32_t candidate_occurrence = 0u;
      for (UINT current_index = 0u; current_index < light_count; ++current_index) {
        std::array<int32_t, 7> current_values = {};
        if (previous_table_data[previous_index].stable_id_lo != table_results[current_index].stable_id_lo
            || previous_table_data[previous_index].stable_id_hi != table_results[current_index].stable_id_hi
            || previous_table_data[previous_index].type != table_results[current_index].type
            || !quantize_identity(light_data[current_index], &current_values) || current_values != previous_values) continue;
        if (candidate_occurrence++ != occurrence) continue;
        if (previous_table_data[previous_index].type == RTXDI_LIGHT_TABLE_TYPE_POINT
            || previous_table_data[previous_index].type == RTXDI_LIGHT_TABLE_TYPE_SPOT) {
          cpu_previous_to_current[previous_index] = current_index;
          cpu_current_to_previous[current_index] = previous_index;
        }
        break;
      }
    }
    if (!std::equal(cpu_previous_to_current.begin(), cpu_previous_to_current.end(), map_results.begin())
        || !std::equal(cpu_current_to_previous.begin(), cpu_current_to_previous.end(), map_results.begin() + light_count)
        || table_diagnostics[RTXDI_DIAG_LIGHT_ID_MATCHED] != 2u
        || table_diagnostics[RTXDI_DIAG_LIGHT_REMAP_HITS] != 2u
        || table_diagnostics[RTXDI_DIAG_LIGHT_ID_UNMATCHED] != 1u
        || table_diagnostics[RTXDI_DIAG_LIGHT_CURRENT_UNMATCHED] != 1u) {
      std::fprintf(stderr, "FAIL M2 GPU reordered map %u,%u,%u,%u matched=%u unmatched=%u current_unmatched=%u\n",
                   map_results[0], map_results[1], map_results[2], map_results[3],
                   table_diagnostics[RTXDI_DIAG_LIGHT_ID_MATCHED], table_diagnostics[RTXDI_DIAG_LIGHT_ID_UNMATCHED],
                   table_diagnostics[RTXDI_DIAG_LIGHT_CURRENT_UNMATCHED]);
      return 1;
    }
    std::printf("PASS M2 GPU FNV64 golden lanes: low=0x%08x high=0x%08x\n", table_results[0].stable_id_lo, table_results[0].stable_id_hi);
    std::printf("PASS M2 GPU structured table stride=32, FNV golden hash, invalid inputs, reordered bidirectional map and diagnostics\n");
    if (!Check(allocator->Reset(), "reset M2 table allocator")
        || !Check(base_list->Reset(allocator.Get(), nullptr), "reset M2 table list")) return 1;
    D3D12_RESOURCE_BARRIER no_history_barriers[3] = {};
    const std::array<ID3D12Resource*, 3> no_history_resources = {
        previous_to_current.Get(), current_to_previous.Get(), diagnostics.Get()};
    for (UINT index = 0u; index < no_history_resources.size(); ++index) {
      no_history_barriers[index].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
      no_history_barriers[index].Transition.pResource = no_history_resources[index];
      no_history_barriers[index].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
      no_history_barriers[index].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
      no_history_barriers[index].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
    }
    list->ResourceBarrier(3u, no_history_barriers);
    list->CopyBufferRegion(previous_to_current.Get(), 0u, map_reset_upload.Get(), 0u, sizeof(uint32_t) * 2u);
    list->CopyBufferRegion(current_to_previous.Get(), 0u, map_reset_upload.Get(), 0u, sizeof(uint32_t) * 2u);
    list->CopyBufferRegion(diagnostics.Get(), 0u, diagnostic_zero_upload.Get(), 0u,
                           sizeof(uint32_t) * diagnostic_count);
    for (UINT index = 0u; index < no_history_resources.size(); ++index) {
      no_history_barriers[index].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
      no_history_barriers[index].Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    }
    list->ResourceBarrier(3u, no_history_barriers);
    list->SetDescriptorHeaps(1u, table_heaps);
    list->SetComputeRootSignature(table_root.Get());
    list->SetPipelineState(table_pso.Get());
    list->SetComputeRoot32BitConstants(0u, 4u, std::array<UINT, 4>{2u, 0u, 0u, 1u}.data(), 0u);
    list->SetComputeRootDescriptorTable(1u, table_heap->GetGPUDescriptorHandleForHeapStart());
    list->Dispatch(1u, 1u, 1u);
    for (UINT index = 0u; index < no_history_resources.size(); ++index) {
      no_history_barriers[index] = {};
      no_history_barriers[index].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
      no_history_barriers[index].UAV.pResource = no_history_resources[index];
    }
    list->ResourceBarrier(3u, no_history_barriers);
    for (UINT index = 0u; index < no_history_resources.size(); ++index) {
      no_history_barriers[index] = {};
      no_history_barriers[index].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
      no_history_barriers[index].Transition.pResource = no_history_resources[index];
      no_history_barriers[index].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
      no_history_barriers[index].Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
      no_history_barriers[index].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
    }
    list->ResourceBarrier(3u, no_history_barriers);
    list->CopyBufferRegion(table_readback.Get(), 0u, previous_to_current.Get(), 0u, sizeof(uint32_t) * 2u);
    list->CopyBufferRegion(table_readback.Get(), sizeof(uint32_t) * 2u, current_to_previous.Get(), 0u,
                           sizeof(uint32_t) * 2u);
    list->CopyBufferRegion(diagnostic_readback.Get(), 0u, diagnostics.Get(), 0u,
                           sizeof(uint32_t) * diagnostic_count);
    wait_context = "M2 no-history invalid maps and counters";
    if (!submit_wait()) return 1;
    std::array<uint32_t, 4> no_history_maps = {};
    if (!Check(table_readback->Map(0u, nullptr, &table_mapped), "map no-history maps")) return 1;
    std::memcpy(no_history_maps.data(), table_mapped, sizeof(no_history_maps));
    table_readback->Unmap(0u, nullptr);
    if (!Check(diagnostic_readback->Map(0u, nullptr, &table_diagnostics_mapped), "map no-history counters")) return 1;
    std::memcpy(table_diagnostics.data(), table_diagnostics_mapped, sizeof(table_diagnostics));
    diagnostic_readback->Unmap(0u, nullptr);
    if (no_history_maps != std::array<uint32_t, 4>{RTXDI_LIGHT_TABLE_INVALID_INDEX, RTXDI_LIGHT_TABLE_INVALID_INDEX,
                                                    RTXDI_LIGHT_TABLE_INVALID_INDEX, RTXDI_LIGHT_TABLE_INVALID_INDEX}
        || table_diagnostics[RTXDI_DIAG_LIGHT_CURRENT_UNMATCHED] != 2u
        || table_diagnostics[RTXDI_DIAG_LIGHT_REMAP_HITS] != 0u) return 1;
    std::printf("PASS M2 GPU no-history maps remain invalid\n");
    if (!Check(allocator->Reset(), "reset M2 no-history allocator")
        || !Check(base_list->Reset(allocator.Get(), nullptr), "reset M2 no-history list")) return 1;
    D3D12_RESOURCE_BARRIER zero_light_barriers[3] = {};
    for (UINT index = 0u; index < no_history_resources.size(); ++index) {
      zero_light_barriers[index].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
      zero_light_barriers[index].Transition.pResource = no_history_resources[index];
      zero_light_barriers[index].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
      zero_light_barriers[index].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
      zero_light_barriers[index].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
    }
    list->ResourceBarrier(3u, zero_light_barriers);
    list->CopyBufferRegion(previous_to_current.Get(), 0u, map_reset_upload.Get(), 0u, sizeof(reset_maps));
    list->CopyBufferRegion(current_to_previous.Get(), 0u, map_reset_upload.Get(), 0u, sizeof(reset_maps));
    list->CopyBufferRegion(diagnostics.Get(), 0u, diagnostic_zero_upload.Get(), 0u,
                           sizeof(uint32_t) * diagnostic_count);
    for (UINT index = 0u; index < no_history_resources.size(); ++index) {
      zero_light_barriers[index].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
      zero_light_barriers[index].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
    }
    list->ResourceBarrier(3u, zero_light_barriers);
    list->CopyBufferRegion(table_readback.Get(), 0u, previous_to_current.Get(), 0u, sizeof(reset_maps));
    list->CopyBufferRegion(table_readback.Get(), sizeof(reset_maps), current_to_previous.Get(), 0u, sizeof(reset_maps));
    list->CopyBufferRegion(diagnostic_readback.Get(), 0u, diagnostics.Get(), 0u,
                           sizeof(uint32_t) * diagnostic_count);
    wait_context = "M2 zero-light clear without shader dispatch";
    if (!submit_wait()) return 1;
    std::array<uint32_t, light_count * 2u> zero_light_maps = {};
    if (!Check(table_readback->Map(0u, nullptr, &table_mapped), "map zero-light maps")) return 1;
    std::memcpy(zero_light_maps.data(), table_mapped, sizeof(zero_light_maps));
    table_readback->Unmap(0u, nullptr);
    if (!Check(diagnostic_readback->Map(0u, nullptr, &table_diagnostics_mapped), "map zero-light counters")) return 1;
    std::memcpy(table_diagnostics.data(), table_diagnostics_mapped, sizeof(table_diagnostics));
    diagnostic_readback->Unmap(0u, nullptr);
    if (zero_light_maps != std::array<uint32_t, light_count * 2u>{RTXDI_LIGHT_TABLE_INVALID_INDEX,
            RTXDI_LIGHT_TABLE_INVALID_INDEX, RTXDI_LIGHT_TABLE_INVALID_INDEX, RTXDI_LIGHT_TABLE_INVALID_INDEX,
            RTXDI_LIGHT_TABLE_INVALID_INDEX, RTXDI_LIGHT_TABLE_INVALID_INDEX, RTXDI_LIGHT_TABLE_INVALID_INDEX,
            RTXDI_LIGHT_TABLE_INVALID_INDEX}
        || std::any_of(table_diagnostics.begin(), table_diagnostics.end(), [](uint32_t value) { return value != 0u; })) return 1;
    std::printf("PASS M2 GPU zero lights leaves counters zero\n");
    if (!Check(allocator->Reset(), "reset M2 zero-light allocator")
        || !Check(base_list->Reset(allocator.Get(), nullptr), "reset M2 zero-light list")) return 1;
  }

  if (!ValidateDebugMessages(info_queue.Get())) return 1;
  std::printf("PASS production surface + DXR visibility (3x2, guard sentinel, masks incl. FORCE_OPAQUE, endpoint, coverage, scene/surface invalids, bias, alpha)\n");
  CloseHandle(event);
  return 0;
}
