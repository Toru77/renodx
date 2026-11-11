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
#include "rtxdi_lighting_consumer_contract.h"
#include "rtxdi_raw_guard_contract.h"
#include "rtxdi_surface_contract.h"
#include "world_rtxdi_classify_lights.h"
#include "world_rtxdi_initial_di.h"
#include "world_rtxdi_nrd_coverage_contract.h"
#include "world_rtxdi_nrd_hitdistance.h"
#include "world_rtxdi_nrd_sampling_contract.h"
#include "world_rtxdi_reconstruct.h"
#include "world_rtxdi_spatial_di.h"
#include "world_rtxdi_sun_visibility.h"
#include "world_rtxdi_sun_detail.h"
#include "world_rtxdi_temporal_di.h"

using Microsoft::WRL::ComPtr;

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

  const std::array<float, 12> vertex_data = {-20.f, -20.f, 0.f, 0.f, 20.f, -20.f, 0.f, 0.f, 0.f, 20.f, 0.f, 0.f};
  ComPtr<ID3D12Resource> vertices;
  ComPtr<ID3D12Resource> indices;
  const uint32_t index_data[] = {0, 1, 2};
  if (!UploadBuffer(device.Get(), vertex_data.data(), sizeof(vertex_data), &vertices)
      || !UploadBuffer(device.Get(), index_data, sizeof(index_data), &indices)) return 1;

  D3D12_RAYTRACING_GEOMETRY_DESC geometry = {};
  geometry.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
  geometry.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;
  geometry.Triangles.VertexBuffer.StartAddress = vertices->GetGPUVirtualAddress();
  geometry.Triangles.VertexBuffer.StrideInBytes = 16;
  geometry.Triangles.VertexCount = 3;
  geometry.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
  geometry.Triangles.IndexBuffer = indices->GetGPUVirtualAddress();
  geometry.Triangles.IndexCount = 3;
  geometry.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;
  D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS blas_inputs = {};
  blas_inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
  blas_inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
  blas_inputs.NumDescs = 1;
  blas_inputs.pGeometryDescs = &geometry;
  blas_inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
  D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO blas_info = {};
  device->GetRaytracingAccelerationStructurePrebuildInfo(&blas_inputs, &blas_info);
  std::fprintf(stderr, "DXR BLAS prebuild result=%llu scratch=%llu update=%llu geom=%u verts=%u indices=%u vb=%llx ib=%llx node=%u\n",
               static_cast<unsigned long long>(blas_info.ResultDataMaxSizeInBytes),
               static_cast<unsigned long long>(blas_info.ScratchDataSizeInBytes),
               static_cast<unsigned long long>(blas_info.UpdateScratchDataSizeInBytes), blas_inputs.NumDescs,
               geometry.Triangles.VertexCount, geometry.Triangles.IndexCount,
               static_cast<unsigned long long>(geometry.Triangles.VertexBuffer.StartAddress),
               static_cast<unsigned long long>(geometry.Triangles.IndexBuffer), blas_inputs.DescsLayout);
  if (blas_info.ResultDataMaxSizeInBytes == 0 || blas_info.ScratchDataSizeInBytes == 0) {
    std::fprintf(stderr, "FAIL invalid BLAS prebuild sizes\n");
    return 1;
  }
  ComPtr<ID3D12Resource> blas;
  ComPtr<ID3D12Resource> blas_scratch;
  if (!CreateDefaultBuffer(device.Get(), AlignAccelerationStructureSize(blas_info.ResultDataMaxSizeInBytes), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                           D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, &blas)
      || !CreateDefaultBuffer(device.Get(), AlignAccelerationStructureSize(blas_info.ScratchDataSizeInBytes), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                              D3D12_RESOURCE_STATE_COMMON, &blas_scratch)) return 1;
  D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC blas_build = {};
  blas_build.Inputs = blas_inputs;
  blas_build.ScratchAccelerationStructureData = blas_scratch->GetGPUVirtualAddress();
  blas_build.DestAccelerationStructureData = blas->GetGPUVirtualAddress();
  D3D12_RESOURCE_BARRIER scratch_barrier = {};
  scratch_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  scratch_barrier.Transition.pResource = blas_scratch.Get();
  scratch_barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  scratch_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
  scratch_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
  list->ResourceBarrier(1, &scratch_barrier);
  list->BuildRaytracingAccelerationStructure(&blas_build, 0, nullptr);
  D3D12_RESOURCE_BARRIER as_barrier = {};
  as_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
  as_barrier.UAV.pResource = blas.Get();
  list->ResourceBarrier(1, &as_barrier);

  std::array<D3D12_RAYTRACING_INSTANCE_DESC, 2> instances_desc = {};
  for (UINT i = 0; i < instances_desc.size(); ++i) {
    instances_desc[i].Transform[0][0] = 1.f;
    instances_desc[i].Transform[1][1] = 1.f;
    instances_desc[i].Transform[2][2] = 1.f;
    instances_desc[i].Transform[2][3] = (i == 0 ? -0.7f : -0.5f);
    instances_desc[i].InstanceID = i;
    instances_desc[i].InstanceMask = 1;
    instances_desc[i].AccelerationStructure = blas->GetGPUVirtualAddress();
  }
  D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS tlas_inputs = {};
  tlas_inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
  tlas_inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
  tlas_inputs.NumDescs = static_cast<UINT>(instances_desc.size());
  tlas_inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
  D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO tlas_info = {};
  device->GetRaytracingAccelerationStructurePrebuildInfo(&tlas_inputs, &tlas_info);
  constexpr std::array<UINT, 6> instance_masks = {1u, 2u, 4u, 8u, 0u, 1u};
  std::array<ComPtr<ID3D12Resource>, instance_masks.size()> instance_uploads;
  std::array<ComPtr<ID3D12Resource>, instance_masks.size()> tlases;
  std::array<ComPtr<ID3D12Resource>, instance_masks.size()> tlas_scratches;
  for (UINT i = 0; i < instance_masks.size(); ++i) {
    instances_desc[0].InstanceMask = static_cast<UINT8>(instance_masks[i]);
    instances_desc[0].InstanceID = (instance_masks[i] == 8u ? 0xFFFFFFu : 0u);
    instances_desc[1].InstanceMask = static_cast<UINT8>(instance_masks[i]);
    instances_desc[0].Flags = (i == 5 ? D3D12_RAYTRACING_INSTANCE_FLAG_FORCE_OPAQUE : D3D12_RAYTRACING_INSTANCE_FLAG_NONE);
    instances_desc[1].Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
    if (!UploadBuffer(device.Get(), instances_desc.data(), sizeof(instances_desc), &instance_uploads[i])
        || !CreateDefaultBuffer(device.Get(), AlignAccelerationStructureSize(tlas_info.ResultDataMaxSizeInBytes),
                                D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                                D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, &tlases[i])
        || !CreateDefaultBuffer(device.Get(), AlignAccelerationStructureSize(tlas_info.ScratchDataSizeInBytes),
                                D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                                D3D12_RESOURCE_STATE_COMMON, &tlas_scratches[i])) return 1;
    tlas_inputs.InstanceDescs = instance_uploads[i]->GetGPUVirtualAddress();
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC tlas_build = {};
    tlas_build.Inputs = tlas_inputs;
    tlas_build.ScratchAccelerationStructureData = tlas_scratches[i]->GetGPUVirtualAddress();
    tlas_build.DestAccelerationStructureData = tlases[i]->GetGPUVirtualAddress();
    scratch_barrier.Transition.pResource = tlas_scratches[i].Get();
    list->ResourceBarrier(1, &scratch_barrier);
    list->BuildRaytracingAccelerationStructure(&tlas_build, 0, nullptr);
    as_barrier.UAV.pResource = tlases[i].Get();
    list->ResourceBarrier(1, &as_barrier);
  }

  WorldMeshGPU mesh = {};
  mesh.header[1] = 3.f;
  mesh.header[3] = 3.f;
  mesh.bbox_min[0] = -20.f;
  mesh.bbox_min[1] = -20.f;
  mesh.bbox_min[2] = 0.f;
  mesh.bbox_max[0] = 20.f;
  mesh.bbox_max[1] = 20.f;
  std::array<WorldInstanceGPU, 2> world_instances = {};
  for (auto& world_instance : world_instances) {
    world_instance.header[0] = 0.f;
    for (UINT i = 0; i < 4; ++i) {
      world_instance.world[i][i] = 1.f;
      world_instance.inverse_world[i][i] = 1.f;
    }
    world_instance.visibility[2] = 2.f;
  }
  AlphaMaterialGPU alpha_material = {};
  const std::array<float, 12> dynamic_data = {-20.f, -20.f, -0.7f, 0.f, 20.f, -20.f, -0.7f, 0.f,
                                              0.f, 20.f, -0.7f, 0.f};
  const std::array<float, 6> uv_data = {0.0001f, 0.5f, 0.0001f, 0.5f, 0.0001f, 0.5f};
  ComPtr<ID3D12Resource> meshes, instances, alpha_materials, dynamic_vertices, uvs;
  if (!UploadBuffer(device.Get(), &mesh, sizeof(mesh), &meshes)
      || !UploadBuffer(device.Get(), world_instances.data(), sizeof(world_instances), &instances)
      || !UploadBuffer(device.Get(), &alpha_material, sizeof(alpha_material), &alpha_materials)
      || !UploadBuffer(device.Get(), dynamic_data.data(), sizeof(dynamic_data), &dynamic_vertices)
      || !UploadBuffer(device.Get(), uv_data.data(), sizeof(uv_data), &uvs)) return 1;

  std::array<uint32_t, 6 * 4> normal_pixels = {};
  for (UINT i = 0; i < 6; ++i) normal_pixels[i * 4 + 3] = 8u;
  std::array<uint32_t, RTXDI_SURFACE_SCENE_ROW_COUNT * 4> scene_pixels = {};
  const auto set_float = [&](UINT row, UINT column, float value) {
    std::memcpy(&scene_pixels[row * 4 + column], &value, sizeof(value));
  };
  for (UINT i = 0; i < 4; ++i) {
    set_float(RTXDI_SURFACE_VIEW_ROW + i, i, 1.f);
    set_float(RTXDI_SURFACE_INVERSE_VIEW_ROW + i, i, 1.f);
    set_float(RTXDI_SURFACE_INVERSE_VIEW_PROJECTION_ROW + i, i, 1.f);
  }
  scene_pixels[RTXDI_SURFACE_INVERSE_VIEW_PROJECTION_ROW * 4 + 0] = std::bit_cast<uint32_t>(0.1f);
  scene_pixels[(RTXDI_SURFACE_INVERSE_VIEW_PROJECTION_ROW + 1) * 4 + 1] = std::bit_cast<uint32_t>(0.1f);
  scene_pixels[(RTXDI_SURFACE_INVERSE_VIEW_PROJECTION_ROW + 2) * 4 + 2] = std::bit_cast<uint32_t>(2.f);
  scene_pixels[(RTXDI_SURFACE_INVERSE_VIEW_PROJECTION_ROW + 2) * 4 + 3] = std::bit_cast<uint32_t>(-2.f);
  scene_pixels[RTXDI_SURFACE_SUN_RESERVED_ROW * 4 + 2] = std::bit_cast<uint32_t>(-1.f);
  for (UINT channel = 0u; channel < 3u; ++channel) {
    scene_pixels[(RTXDI_SURFACE_SUN_RESERVED_ROW - 1u) * 4u + channel] = std::bit_cast<uint32_t>(1.f);
  }
  scene_pixels[RTXDI_SURFACE_RESOLUTION_SCALING_ROW * 4 + 0] = std::bit_cast<uint32_t>(1.f);
  scene_pixels[RTXDI_SURFACE_RESOLUTION_SCALING_ROW * 4 + 1] = std::bit_cast<uint32_t>(1.f);
  scene_pixels[RTXDI_SURFACE_LIGHT_TILE_SIZE_ROW * 4 + 0] = std::bit_cast<uint32_t>(1.f / 32.f);
  scene_pixels[RTXDI_SURFACE_LIGHT_TILE_SIZE_ROW * 4 + 1] = std::bit_cast<uint32_t>(1.f / 18.f);
  auto previous_scene_pixels = scene_pixels;
  std::array<float, 6> depths = {0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f};
  ComPtr<ID3D12Resource> depth_tex, depth_upload, normal_tex, normal_upload, scene_tex, scene_upload;
  if (!CreateTextureUpload(device.Get(), list.Get(), DXGI_FORMAT_R32_FLOAT, 3, 2, depths.data(), 3 * sizeof(float),
                           &depth_tex, &depth_upload)
      || !CreateTextureUpload(device.Get(), list.Get(), DXGI_FORMAT_R32G32B32A32_UINT, 3, 2, normal_pixels.data(), 3 * 16,
                              &normal_tex, &normal_upload)
      || !CreateTextureUpload(device.Get(), list.Get(), DXGI_FORMAT_R32G32B32A32_UINT,
                              RTXDI_SURFACE_SCENE_ROW_COUNT, 1, scene_pixels.data(), RTXDI_SURFACE_SCENE_ROW_COUNT * 16,
                              &scene_tex, &scene_upload)) return 1;

  ComPtr<ID3D12Resource> atlas;
  if (!CreateTexture(device.Get(), DXGI_FORMAT_R32_UINT, 256, 256, 256, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                     D3D12_RESOURCE_STATE_UNORDERED_ACCESS, &atlas)) return 1;
  D3D12_DESCRIPTOR_HEAP_DESC atlas_heap_desc = {};
  atlas_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
  atlas_heap_desc.NumDescriptors = 1;
  atlas_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
  ComPtr<ID3D12DescriptorHeap> atlas_heap;
  if (!Check(device->CreateDescriptorHeap(&atlas_heap_desc, IID_PPV_ARGS(&atlas_heap)), "create atlas clear heap")) return 1;
  atlas_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
  ComPtr<ID3D12DescriptorHeap> atlas_cpu_heap;
  if (!Check(device->CreateDescriptorHeap(&atlas_heap_desc, IID_PPV_ARGS(&atlas_cpu_heap)), "create atlas CPU heap")) return 1;
  D3D12_UNORDERED_ACCESS_VIEW_DESC atlas_uav = {};
  atlas_uav.Format = DXGI_FORMAT_R32_UINT;
  atlas_uav.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
  atlas_uav.Texture2DArray.ArraySize = 256;
  auto atlas_cpu = atlas_cpu_heap->GetCPUDescriptorHandleForHeapStart();
  auto atlas_gpu = atlas_heap->GetGPUDescriptorHandleForHeapStart();
  device->CreateUnorderedAccessView(atlas.Get(), nullptr, &atlas_uav, atlas_cpu);
  auto atlas_visible_cpu = atlas_heap->GetCPUDescriptorHandleForHeapStart();
  device->CreateUnorderedAccessView(atlas.Get(), nullptr, &atlas_uav, atlas_visible_cpu);
  ID3D12DescriptorHeap* atlas_heaps[] = {atlas_heap.Get()};
  list->SetDescriptorHeaps(1, atlas_heaps);
  constexpr UINT opaque_alpha[4] = {0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu};
  list->ClearUnorderedAccessViewUint(atlas_gpu, atlas_cpu, atlas.Get(), opaque_alpha, 0, nullptr);
  D3D12_RESOURCE_BARRIER atlas_barrier = {};
  atlas_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
  atlas_barrier.UAV.pResource = atlas.Get();
  list->ResourceBarrier(1, &atlas_barrier);
  atlas_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  atlas_barrier.Transition.pResource = atlas.Get();
  atlas_barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  atlas_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
  atlas_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
  list->ResourceBarrier(1, &atlas_barrier);
  std::array<uint32_t, 64> atlas_word_data = {};
  atlas_word_data.fill(0xFFFFFFFFu);
  ComPtr<ID3D12Resource> atlas_word_upload;
  if (!UploadBuffer(device.Get(), atlas_word_data.data(), sizeof(atlas_word_data), &atlas_word_upload)) return 1;

  if (!submit_wait()) return 1;
  std::fprintf(stderr, "DXR AS build submitted and fence completed\n");
  if (!Check(allocator->Reset(), "reset allocator") || !Check(base_list->Reset(allocator.Get(), nullptr), "reset list")) return 1;

  // Production sun shader root table: t0-t3, t14-t16, t29, t30, t31-t33, t61, u0-u1.
  D3D12_DESCRIPTOR_RANGE sun_ranges[8] = {};
  sun_ranges[0] = {D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0, 0, 0};
  sun_ranges[1] = {D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 14, 0, 4};
  sun_ranges[2] = {D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 29, 0, 7};
  sun_ranges[3] = {D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 30, 0, 8};
  sun_ranges[4] = {D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 31, 0, 9};
  sun_ranges[5] = {D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0, 12};
  sun_ranges[6] = {D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 1, 0, 13};
  sun_ranges[7] = {D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 61, 0, 16};
  ComPtr<ID3D12RootSignature> sun_root;
  ComPtr<ID3D12PipelineState> sun_pso;
  if (!CreateRoot(device.Get(), sun_ranges, 8, 24, &sun_root)
      || !CreatePso(device.Get(), sun_root.Get(), __world_rtxdi_sun_visibility, &sun_pso)) return 1;
  std::fprintf(stderr, "DXR sun root + PSO ready\n");
  D3D12_DESCRIPTOR_RANGE sun_detail_ranges[2] = {};
  sun_detail_ranges[0] = {D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1u, 0u, 0u, 0u};
  sun_detail_ranges[1] = {D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1u, 0u, 0u, 1u};
  ComPtr<ID3D12RootSignature> sun_detail_root;
  ComPtr<ID3D12PipelineState> sun_detail_pso;
  if (!CreateRoot(device.Get(), sun_detail_ranges, 2u, 3u, &sun_detail_root)
      || !CreatePso(device.Get(), sun_detail_root.Get(), __world_rtxdi_sun_detail, &sun_detail_pso)) return 1;
  D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
  heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
  heap_desc.NumDescriptors = 17;
  heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
  ComPtr<ID3D12DescriptorHeap> sun_heap;
  if (!Check(device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&sun_heap)), "create sun descriptor heap")) return 1;
  heap_desc.NumDescriptors = 1;
  heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
  ComPtr<ID3D12DescriptorHeap> sun_cpu_heap;
  if (!Check(device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&sun_cpu_heap)), "create sun CPU descriptor heap")) return 1;
  heap_desc.NumDescriptors = 2u;
  heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
  ComPtr<ID3D12DescriptorHeap> sun_detail_heap;
  if (!Check(device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&sun_detail_heap)), "create sun detail descriptor heap")) return 1;
  heap_desc.NumDescriptors = 1u;
  heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
  ComPtr<ID3D12DescriptorHeap> sun_detail_cpu_heap;
  if (!Check(device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&sun_detail_cpu_heap)), "create sun detail CPU heap")) return 1;
  heap_desc.NumDescriptors = 14;
  heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
  std::fprintf(stderr, "DXR sun descriptor heap ready\n");
  const UINT descriptor_size = device->GetDescriptorHandleIncrementSize(heap_desc.Type);
  CreateBufferSrv(device.Get(), sun_heap.Get(), descriptor_size, 0, vertices.Get(), 3, 16);
  CreateBufferSrv(device.Get(), sun_heap.Get(), descriptor_size, 1, indices.Get(), 3, 4);
  CreateBufferSrv(device.Get(), sun_heap.Get(), descriptor_size, 2, meshes.Get(), 1, sizeof(WorldMeshGPU));
  CreateBufferSrv(device.Get(), sun_heap.Get(), descriptor_size, 3, instances.Get(), 2, sizeof(WorldInstanceGPU));
  CreateBufferSrv(device.Get(), sun_heap.Get(), descriptor_size, 4, uvs.Get(), 3, 8);
  CreateBufferSrv(device.Get(), sun_heap.Get(), descriptor_size, 6, alpha_materials.Get(), 1, sizeof(AlphaMaterialGPU));
  CreateBufferSrv(device.Get(), sun_heap.Get(), descriptor_size, 8, dynamic_vertices.Get(), 3, 16);
  auto cpu = sun_heap->GetCPUDescriptorHandleForHeapStart();
  cpu.ptr += static_cast<SIZE_T>(5) * descriptor_size;
  D3D12_SHADER_RESOURCE_VIEW_DESC atlas_srv = {};
  atlas_srv.Format = DXGI_FORMAT_R32_UINT;
  atlas_srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
  atlas_srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  atlas_srv.Texture2DArray.MipLevels = 1;
  atlas_srv.Texture2DArray.ArraySize = 256;
  // Match the production packed atlas: 256 x 256 uint words across 256 slices.
  device->CreateShaderResourceView(atlas.Get(), &atlas_srv, cpu);
  cpu.ptr += static_cast<SIZE_T>(2) * descriptor_size;
  D3D12_SHADER_RESOURCE_VIEW_DESC tlas_srv = {};
  tlas_srv.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
  tlas_srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  tlas_srv.RaytracingAccelerationStructure.Location = tlases[0]->GetGPUVirtualAddress();
  const auto tlas_slot_cpu = cpu;
  device->CreateShaderResourceView(nullptr, &tlas_srv, cpu);
  D3D12_SHADER_RESOURCE_VIEW_DESC depth_srv = {};
  depth_srv.Format = DXGI_FORMAT_R32_FLOAT;
  depth_srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
  depth_srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  depth_srv.Texture2D.MipLevels = 1;
  cpu.ptr += static_cast<SIZE_T>(2) * descriptor_size;
  device->CreateShaderResourceView(depth_tex.Get(), &depth_srv, cpu);
  D3D12_SHADER_RESOURCE_VIEW_DESC uint4_srv = {};
  uint4_srv.Format = DXGI_FORMAT_R32G32B32A32_UINT;
  uint4_srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
  uint4_srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  uint4_srv.Texture2D.MipLevels = 1;
  cpu.ptr += descriptor_size;
  device->CreateShaderResourceView(normal_tex.Get(), &uint4_srv, cpu);
  cpu.ptr += descriptor_size;
  device->CreateShaderResourceView(scene_tex.Get(), &uint4_srv, cpu);

  ComPtr<ID3D12Resource> sun_output;
  ComPtr<ID3D12Resource> sun_readback;
  ComPtr<ID3D12Resource> sun_visibility_output;
  ComPtr<ID3D12Resource> sun_visibility_readback;
  ComPtr<ID3D12Resource> sun_detail_output;
  ComPtr<ID3D12Resource> sun_detail_readback;
  constexpr UINT pixel_count = 6;
  constexpr UINT sun_output_count = pixel_count + 1;
  const auto default_properties = Heap(D3D12_HEAP_TYPE_DEFAULT);
  const auto readback_properties = Heap(D3D12_HEAP_TYPE_READBACK);
  const auto sun_readback_desc = BufferDesc(sizeof(SunResult) * sun_output_count);
  D3D12_RESOURCE_DESC sun_visibility_desc = {};
  sun_visibility_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
  sun_visibility_desc.Width = 4u;
  sun_visibility_desc.Height = 2u;
  sun_visibility_desc.DepthOrArraySize = 1u;
  sun_visibility_desc.MipLevels = 1u;
  sun_visibility_desc.Format = DXGI_FORMAT_R32_UINT;
  sun_visibility_desc.SampleDesc.Count = 1u;
  sun_visibility_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
  sun_visibility_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
  D3D12_PLACED_SUBRESOURCE_FOOTPRINT visibility_footprint = {};
  UINT visibility_rows = 0u;
  UINT64 visibility_row_bytes = 0u, visibility_bytes = 0u;
  device->GetCopyableFootprints(&sun_visibility_desc, 0u, 1u, 0u, &visibility_footprint, &visibility_rows,
                                &visibility_row_bytes, &visibility_bytes);
  const auto visibility_readback_desc = BufferDesc(visibility_bytes);
  D3D12_RESOURCE_DESC sun_detail_desc = {};
  sun_detail_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
  sun_detail_desc.Width = 4u;
  sun_detail_desc.Height = 2u;
  sun_detail_desc.DepthOrArraySize = 1u;
  sun_detail_desc.MipLevels = 1u;
  sun_detail_desc.Format = DXGI_FORMAT_R32G32B32A32_UINT;
  sun_detail_desc.SampleDesc.Count = 1u;
  sun_detail_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
  sun_detail_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
  D3D12_PLACED_SUBRESOURCE_FOOTPRINT sun_detail_footprint = {};
  UINT sun_detail_rows = 0u;
  UINT64 sun_detail_row_bytes = 0u, sun_detail_bytes = 0u;
  device->GetCopyableFootprints(&sun_detail_desc, 0u, 1u, 0u, &sun_detail_footprint, &sun_detail_rows,
                                &sun_detail_row_bytes, &sun_detail_bytes);
  const auto sun_detail_readback_desc = BufferDesc(sun_detail_bytes);
  if (!CreateDefaultBuffer(device.Get(), sizeof(SunResult) * sun_output_count, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                           D3D12_RESOURCE_STATE_COMMON, &sun_output)
      || !Check(device->CreateCommittedResource(&readback_properties, D3D12_HEAP_FLAG_NONE,
                                                &sun_readback_desc, D3D12_RESOURCE_STATE_COPY_DEST,
                                                nullptr, IID_PPV_ARGS(&sun_readback)),
                "create sun readback")
      || !Check(device->CreateCommittedResource(&default_properties, D3D12_HEAP_FLAG_NONE, &sun_visibility_desc,
                                                D3D12_RESOURCE_STATE_COMMON, nullptr,
                                                IID_PPV_ARGS(&sun_visibility_output)),
                "create sun visibility output")
      || !Check(device->CreateCommittedResource(&readback_properties, D3D12_HEAP_FLAG_NONE,
                                                &visibility_readback_desc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                                IID_PPV_ARGS(&sun_visibility_readback)),
                "create sun visibility readback")
      || !CreateTexture(device.Get(), DXGI_FORMAT_R32G32B32A32_UINT, 4u, 2u, 1u,
                        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON, &sun_detail_output)
      || !Check(device->CreateCommittedResource(&readback_properties, D3D12_HEAP_FLAG_NONE,
                                                &sun_detail_readback_desc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                                IID_PPV_ARGS(&sun_detail_readback)),
                "create sun detail readback")) return 1;
  std::fprintf(stderr, "DXR sun output/readback ready\n");
  cpu = sun_heap->GetCPUDescriptorHandleForHeapStart();
  cpu.ptr += static_cast<SIZE_T>(12) * descriptor_size;
  D3D12_UNORDERED_ACCESS_VIEW_DESC result_uav = {};
  result_uav.Format = DXGI_FORMAT_UNKNOWN;
  result_uav.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
  result_uav.Buffer.NumElements = sun_output_count;
  result_uav.Buffer.StructureByteStride = sizeof(SunResult);
  device->CreateUnorderedAccessView(sun_output.Get(), nullptr, &result_uav, cpu);
  cpu.ptr += descriptor_size;
  D3D12_UNORDERED_ACCESS_VIEW_DESC visibility_uav = {};
  visibility_uav.Format = DXGI_FORMAT_R32_UINT;
  visibility_uav.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
  device->CreateUnorderedAccessView(sun_visibility_output.Get(), nullptr, &visibility_uav, cpu);
  device->CreateUnorderedAccessView(sun_visibility_output.Get(), nullptr, &visibility_uav,
                                    sun_cpu_heap->GetCPUDescriptorHandleForHeapStart());
  auto noise_cpu = sun_heap->GetCPUDescriptorHandleForHeapStart();
  noise_cpu.ptr += static_cast<SIZE_T>(16) * descriptor_size;
  D3D12_SHADER_RESOURCE_VIEW_DESC noise_srv = {};
  noise_srv.Format = DXGI_FORMAT_R32G32B32A32_UINT;
  noise_srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
  noise_srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  noise_srv.Texture2D.MipLevels = 1;
  device->CreateShaderResourceView(nullptr, &noise_srv, noise_cpu);
  D3D12_SHADER_RESOURCE_VIEW_DESC detail_source_srv = {};
  detail_source_srv.Format = DXGI_FORMAT_UNKNOWN;
  detail_source_srv.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
  detail_source_srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  detail_source_srv.Buffer.NumElements = pixel_count;
  detail_source_srv.Buffer.StructureByteStride = sizeof(SunResult);
  auto detail_cpu = sun_detail_heap->GetCPUDescriptorHandleForHeapStart();
  device->CreateShaderResourceView(sun_output.Get(), &detail_source_srv, detail_cpu);
  detail_cpu.ptr += descriptor_size;
  D3D12_UNORDERED_ACCESS_VIEW_DESC detail_uav = {};
  detail_uav.Format = DXGI_FORMAT_R32G32B32A32_UINT;
  detail_uav.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
  device->CreateUnorderedAccessView(sun_detail_output.Get(), nullptr, &detail_uav, detail_cpu);
  device->CreateUnorderedAccessView(sun_detail_output.Get(), nullptr, &detail_uav,
                                    sun_detail_cpu_heap->GetCPUDescriptorHandleForHeapStart());
  std::array<UINT, sun_output_count * 4> sentinel_words = {};
  sentinel_words.fill(0xDEADBEEFu);
  ComPtr<ID3D12Resource> sentinel_upload;
  if (!UploadBuffer(device.Get(), sentinel_words.data(), sizeof(sentinel_words), &sentinel_upload)) return 1;

  ID3D12DescriptorHeap* heaps[] = {sun_heap.Get()};
  list->SetDescriptorHeaps(1, heaps);
  list->SetComputeRootSignature(sun_root.Get());
  list->SetPipelineState(sun_pso.Get());
  list->SetComputeRootDescriptorTable(1, sun_heap->GetGPUDescriptorHandleForHeapStart());
  D3D12_RESOURCE_BARRIER output_to_uav = {};
  output_to_uav.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  output_to_uav.Transition.pResource = sun_output.Get();
  output_to_uav.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  output_to_uav.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
  output_to_uav.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
  list->ResourceBarrier(1, &output_to_uav);
  output_to_uav.Transition.pResource = sun_detail_output.Get();
  output_to_uav.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
  list->ResourceBarrier(1, &output_to_uav);
  const auto detail_gpu = sun_detail_heap->GetGPUDescriptorHandleForHeapStart();
  auto detail_uav_gpu = detail_gpu;
  detail_uav_gpu.ptr += descriptor_size;
  const UINT detail_sentinel[4] = {0xDEADBEEFu, 0xDEADBEEFu, 0xDEADBEEFu, 0xDEADBEEFu};
  ID3D12DescriptorHeap* detail_clear_heaps[] = {sun_detail_heap.Get()};
  list->SetDescriptorHeaps(1u, detail_clear_heaps);
  list->ClearUnorderedAccessViewUint(detail_uav_gpu, sun_detail_cpu_heap->GetCPUDescriptorHandleForHeapStart(),
                                    sun_detail_output.Get(), detail_sentinel, 0u, nullptr);
  output_to_uav.Transition.pResource = sun_visibility_output.Get();
  list->ResourceBarrier(1, &output_to_uav);
  const auto dispatch = [&](UINT mask_index, float max_distance, UINT expected_status, UINT expected_reason,
                            bool coverage_valid, const char* label, float expected_distance = -1.f,
                            bool scene_valid = true, float environment_bias = 0.01f, float character_bias = 0.02f,
                            float foliage_transmission = 0.f, UINT expected_layers = 0u, float wind_transmission = 0.f, UINT expected_wind_occlusion = 0u, float wind_softening = 0.f) {
    std::fprintf(stderr, "DXR dispatch %s distance=%f\n", label, max_distance);
    tlas_srv.RaytracingAccelerationStructure.Location = tlases[mask_index]->GetGPUVirtualAddress();
    device->CreateShaderResourceView(nullptr, &tlas_srv, tlas_slot_cpu);
    list->SetDescriptorHeaps(1, heaps);
    list->SetComputeRootSignature(sun_root.Get());
    list->SetPipelineState(sun_pso.Get());
    list->SetComputeRootDescriptorTable(1, sun_heap->GetGPUDescriptorHandleForHeapStart());
    const UINT params[24] = {3, 2, std::bit_cast<UINT>(max_distance), std::bit_cast<UINT>(environment_bias),
                             std::bit_cast<UINT>(character_bias), std::bit_cast<UINT>(-2.f), std::bit_cast<UINT>(-2.f),
                             std::bit_cast<UINT>(-2.f), std::bit_cast<UINT>(4.f), scene_valid ? 1u : 0u,
                             coverage_valid ? 1u : 0u, 1u, 0u, 0u, 0u, 0u, 0u, std::bit_cast<UINT>(foliage_transmission), std::bit_cast<UINT>(wind_transmission), 0u, std::bit_cast<UINT>(1.f), std::bit_cast<UINT>(1.f), std::bit_cast<UINT>(1.f), std::bit_cast<UINT>(wind_softening)};
    list->SetComputeRoot32BitConstants(0, 24, params, 0);
    D3D12_RESOURCE_BARRIER output_barrier = {};
    output_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    output_barrier.Transition.pResource = sun_output.Get();
    output_barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    output_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    output_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
    list->ResourceBarrier(1, &output_barrier);
    list->CopyBufferRegion(sun_output.Get(), 0, sentinel_upload.Get(), 0, sizeof(sentinel_words));
    output_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    output_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    list->ResourceBarrier(1, &output_barrier);
    const auto sun_visibility_gpu = sun_heap->GetGPUDescriptorHandleForHeapStart();
    auto sun_visibility_cpu = sun_cpu_heap->GetCPUDescriptorHandleForHeapStart();
    auto sun_visibility_gpu_offset = sun_visibility_gpu;
    sun_visibility_gpu_offset.ptr += static_cast<UINT64>(13u) * descriptor_size;
    const UINT visibility_sentinel[4] = {0xDEADBEEFu, 0u, 0u, 0u};
    list->ClearUnorderedAccessViewUint(sun_visibility_gpu_offset, sun_visibility_cpu, sun_visibility_output.Get(),
                                       visibility_sentinel, 0u, nullptr);
    list->Dispatch(1, 1, 1);
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier.UAV.pResource = sun_output.Get();
    list->ResourceBarrier(1, &barrier);
    barrier.UAV.pResource = sun_visibility_output.Get();
    list->ResourceBarrier(1, &barrier);
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = sun_output.Get();
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    list->ResourceBarrier(1, &barrier);
    ID3D12DescriptorHeap* detail_heaps[] = {sun_detail_heap.Get()};
    list->SetDescriptorHeaps(1u, detail_heaps);
    list->SetComputeRootSignature(sun_detail_root.Get());
    list->SetPipelineState(sun_detail_pso.Get());
    const UINT detail_params[3] = {3u, 2u, pixel_count};
    list->SetComputeRoot32BitConstants(0u, 3u, detail_params, 0u);
    list->SetComputeRootDescriptorTable(1u, detail_gpu);
    list->Dispatch(1u, 1u, 1u);
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier.UAV.pResource = sun_detail_output.Get();
    list->ResourceBarrier(1u, &barrier);
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = sun_output.Get();
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
    list->ResourceBarrier(1, &barrier);
    list->CopyBufferRegion(sun_readback.Get(), 0, sun_output.Get(), 0, sizeof(SunResult) * sun_output_count);
    barrier.Transition.pResource = sun_detail_output.Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
    list->ResourceBarrier(1u, &barrier);
    D3D12_TEXTURE_COPY_LOCATION detail_source = {};
    detail_source.pResource = sun_detail_output.Get();
    detail_source.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    D3D12_TEXTURE_COPY_LOCATION detail_dest = {};
    detail_dest.pResource = sun_detail_readback.Get();
    detail_dest.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    detail_dest.PlacedFootprint = sun_detail_footprint;
    list->CopyTextureRegion(&detail_dest, 0u, 0u, 0u, &detail_source, nullptr);
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = sun_visibility_output.Get();
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
    list->ResourceBarrier(1, &barrier);
    D3D12_TEXTURE_COPY_LOCATION visibility_source = {};
    visibility_source.pResource = sun_visibility_output.Get();
    visibility_source.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    D3D12_TEXTURE_COPY_LOCATION visibility_dest = {};
    visibility_dest.pResource = sun_visibility_readback.Get();
    visibility_dest.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    visibility_dest.PlacedFootprint = visibility_footprint;
    list->CopyTextureRegion(&visibility_dest, 0u, 0u, 0u, &visibility_source, nullptr);
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    list->ResourceBarrier(1, &barrier);
    barrier.Transition.pResource = sun_detail_output.Get();
    list->ResourceBarrier(1u, &barrier);
    barrier.Transition.pResource = sun_output.Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    list->ResourceBarrier(1, &barrier);
    if (!submit_wait()) {
      std::fprintf(stderr, "FAIL submit %s removed=0x%08lx\n", label,
                   static_cast<unsigned long>(device->GetDeviceRemovedReason()));
      DumpDred(device.Get());
      DumpDebugMessages(device.Get());
      return false;
    }
    std::fprintf(stderr, "DXR dispatch %s fence complete\n", label);
    if (!Check(allocator->Reset(), "reset test allocator") || !Check(base_list->Reset(allocator.Get(), nullptr), "reset test list")) return false;
    void* mapped = nullptr;
    if (!Check(sun_readback->Map(0, nullptr, &mapped), "map sun result")) return false;
    std::array<SunResult, sun_output_count> result_values = {};
    std::memcpy(result_values.data(), mapped, sizeof(result_values));
    sun_readback->Unmap(0, nullptr);
    const auto* results = result_values.data();
    void* visibility_mapped = nullptr;
    if (!Check(sun_visibility_readback->Map(0u, nullptr, &visibility_mapped), "map sun visibility status")) return false;
    const auto* visibility_bytes = static_cast<const uint8_t*>(visibility_mapped);
    for (UINT i = 0; i < pixel_count; ++i) {
      if (results[i].status != expected_status || results[i].reason != expected_reason
          || (expected_distance >= 0.f && std::abs(results[i].distance - expected_distance) > 0.02f)) {
        std::fprintf(stderr, "FAIL %s pixel=%u status=%u expected=%u reason=%u expectedReason=%u distance=%f\n", label, i,
                     results[i].status, expected_status, results[i].reason, expected_reason, results[i].distance);
        std::fprintf(stderr, "DXR failure deviceRemoved=0x%08lx\n",
                     static_cast<unsigned long>(device->GetDeviceRemovedReason()));
        DumpDebugMessages(device.Get());
        return false;
      }
      const bool sampled_sun = expected_reason == RTXDI_SUN_REASON_NONE
                               || expected_reason == RTXDI_SUN_REASON_INVALID_ALPHA
                               || expected_reason == RTXDI_SUN_REASON_COVERAGE_MISS
                               || expected_reason == RTXDI_SUN_REASON_TRUNCATED;
      if (sampled_sun != ((results[i].flags & RTXDI_SUN_FLAG_UNIT_INVERSE_PDF) != 0u)) {
        std::fprintf(stderr, "FAIL %s pixel=%u unit inverse-PDF flag=%u\n", label, i,
                     (results[i].flags & RTXDI_SUN_FLAG_UNIT_INVERSE_PDF) != 0u);
        sun_readback->Unmap(0, nullptr);
        return false;
      }
      const UINT x = i % 3u;
      const UINT y = i / 3u;
      const auto* row = reinterpret_cast<const UINT*>(visibility_bytes + y * visibility_footprint.Footprint.RowPitch);
      if (row[x] != (expected_status | (expected_layers << RTXDI_SUN_LAYER_SHIFT) | (expected_wind_occlusion << RTXDI_SUN_WIND_OCCLUSION_SHIFT))) {
        std::fprintf(stderr, "FAIL %s t42 pixel=%u status=%u expected=%u\n", label, i, row[x], (expected_status | (expected_layers << RTXDI_SUN_LAYER_SHIFT) | (expected_wind_occlusion << RTXDI_SUN_WIND_OCCLUSION_SHIFT)));
        sun_visibility_readback->Unmap(0u, nullptr);
        return false;
      }
    }
    for (UINT y = 0u; y < 2u; ++y) {
      const auto* row = reinterpret_cast<const UINT*>(visibility_bytes + y * visibility_footprint.Footprint.RowPitch);
      if (row[3] != 0xDEADBEEFu) {
        std::fprintf(stderr, "FAIL %s t42 guard overwritten at row=%u status=%u\n", label, y, row[3]);
        sun_visibility_readback->Unmap(0u, nullptr);
        return false;
      }
    }
    sun_visibility_readback->Unmap(0u, nullptr);
    for (UINT i = pixel_count; i < sun_output_count; ++i) {
      const auto* words = reinterpret_cast<const UINT*>(&results[i]);
      if (words[0] != 0xDEADBEEFu || words[1] != 0xDEADBEEFu || words[2] != 0xDEADBEEFu
          || words[3] != 0xDEADBEEFu) {
        std::fprintf(stderr, "FAIL %s output sentinel overwritten at %u\n", label, i);
        return false;
      }
    }
    void* detail_mapped = nullptr;
    if (!Check(sun_detail_readback->Map(0u, nullptr, &detail_mapped), "map sun detail readback")) return false;
    const auto* detail_bytes = static_cast<const uint8_t*>(detail_mapped);
    for (UINT i = 0u; i < pixel_count; ++i) {
      const UINT x = i % 3u;
      const UINT y = i / 3u;
      const auto* row = reinterpret_cast<const UINT*>(detail_bytes + y * sun_detail_footprint.Footprint.RowPitch);
      const UINT offset = x * 4u;
      if (row[offset] != results[i].status || row[offset + 1u] != results[i].reason
          || row[offset + 2u] != std::bit_cast<UINT>(results[i].distance) || row[offset + 3u] != results[i].flags) {
        std::fprintf(stderr, "FAIL %s sun detail tuple mismatch pixel=%u status=%u reason=%u distance=%.5f flags=%u\n",
                     label, i, row[offset], row[offset + 1u], std::bit_cast<float>(row[offset + 2u]), row[offset + 3u]);
        sun_detail_readback->Unmap(0u, nullptr);
        return false;
      }
    }
    for (UINT y = 0u; y < 2u; ++y) {
      const auto* row = reinterpret_cast<const UINT*>(detail_bytes + y * sun_detail_footprint.Footprint.RowPitch);
      const UINT offset = 3u * 4u;
      if (row[offset] != 0xDEADBEEFu || row[offset + 1u] != 0xDEADBEEFu
          || row[offset + 2u] != 0xDEADBEEFu || row[offset + 3u] != 0xDEADBEEFu) {
        std::fprintf(stderr, "FAIL %s sun detail guard overwritten row=%u\n", label, y);
        sun_detail_readback->Unmap(0u, nullptr);
        return false;
      }
    }
    sun_detail_readback->Unmap(0u, nullptr);
    return true;
  };

  const auto update_upload = [](ID3D12Resource* resource, const void* source, size_t size) {
    void* mapped = nullptr;
    if (FAILED(resource->Map(0, nullptr, &mapped))) return false;
    std::memcpy(mapped, source, size);
    resource->Unmap(0, nullptr);
    return true;
  };
  const auto update_pixel_texture = [&](ID3D12Resource* texture, ID3D12Resource* upload, const void* data, UINT row_bytes) {
    const auto desc = texture->GetDesc();
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint = {};
    UINT rows = 0;
    UINT64 row_size = 0;
    UINT64 total = 0;
    device->GetCopyableFootprints(&desc, 0, 1, 0, &footprint, &rows, &row_size, &total);
    uint8_t* mapped = nullptr;
    if (!Check(upload->Map(0, nullptr, reinterpret_cast<void**>(&mapped)), "map pixel staging")) return false;
    for (UINT y = 0; y < desc.Height; ++y) {
      std::memcpy(mapped + footprint.Offset + static_cast<size_t>(y) * footprint.Footprint.RowPitch,
                  static_cast<const uint8_t*>(data) + static_cast<size_t>(y) * row_bytes, row_bytes);
    }
    upload->Unmap(0, nullptr);
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = texture;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
    list->ResourceBarrier(1, &barrier);
    D3D12_TEXTURE_COPY_LOCATION source = {};
    source.pResource = upload;
    source.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    source.PlacedFootprint = footprint;
    D3D12_TEXTURE_COPY_LOCATION dest = {};
    dest.pResource = texture;
    dest.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    list->CopyTextureRegion(&dest, 0, 0, 0, &source, nullptr);
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    list->ResourceBarrier(1, &barrier);
    if (!submit_wait()) return false;
    return Check(allocator->Reset(), "reset pixel allocator")
           && Check(base_list->Reset(allocator.Get(), nullptr), "reset pixel list");
  };
  const auto update_atlas_texel = [&](uint32_t texel) {
    void* mapped = nullptr;
    if (!Check(atlas_word_upload->Map(0, nullptr, &mapped), "map alpha texel upload")) return false;
    std::memcpy(mapped, &texel, sizeof(texel));
    atlas_word_upload->Unmap(0, nullptr);
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = atlas.Get();
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
    list->ResourceBarrier(1, &barrier);
    D3D12_TEXTURE_COPY_LOCATION source = {};
    source.pResource = atlas_word_upload.Get();
    source.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    source.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R32_UINT;
    source.PlacedFootprint.Footprint.Width = 1;
    source.PlacedFootprint.Footprint.Height = 1;
    source.PlacedFootprint.Footprint.Depth = 1;
    source.PlacedFootprint.Footprint.RowPitch = 256;
    D3D12_TEXTURE_COPY_LOCATION dest = {};
    dest.pResource = atlas.Get();
    dest.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dest.SubresourceIndex = 0;
    list->CopyTextureRegion(&dest, 0, 128, 0, &source, nullptr);
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    list->ResourceBarrier(1, &barrier);
    if (!submit_wait()) return false;
    return Check(allocator->Reset(), "reset alpha allocator")
           && Check(base_list->Reset(allocator.Get(), nullptr), "reset alpha list");
  };
  if (!dispatch(0, 0.6f, RTXDI_SUN_STATUS_BLOCKED, RTXDI_SUN_REASON_NONE, true, "opaque DXR blocker")
      || !dispatch(1, 0.6f, RTXDI_SUN_STATUS_BLOCKED, RTXDI_SUN_REASON_NONE, true, "foliage mask")
      || !dispatch(2, 0.6f, RTXDI_SUN_STATUS_BLOCKED, RTXDI_SUN_REASON_NONE, true, "shadow-only mask")
      || !dispatch(3, 0.6f, RTXDI_SUN_STATUS_BLOCKED, RTXDI_SUN_REASON_NONE, true, "dynamic mask")
      || !dispatch(4, 0.6f, RTXDI_SUN_STATUS_VISIBLE, RTXDI_SUN_REASON_NONE, true, "excluded mask")
      || !dispatch(5, 0.6f, RTXDI_SUN_STATUS_BLOCKED, RTXDI_SUN_REASON_NONE, true, "force-opaque instance")
      || !dispatch(0, 0.25f, RTXDI_SUN_STATUS_VISIBLE, RTXDI_SUN_REASON_NONE, true, "finite endpoint miss")
      || !dispatch(4, 4.f, RTXDI_SUN_STATUS_UNKNOWN, RTXDI_SUN_REASON_TRUNCATED, true, "coverage-truncated miss")
      || !dispatch(4, 0.6f, RTXDI_SUN_STATUS_UNKNOWN, RTXDI_SUN_REASON_TRUNCATED, false, "coverage unsupported")) return 1;

  if (!dispatch(4, 0.6f, RTXDI_SUN_STATUS_UNKNOWN, RTXDI_SUN_REASON_INVALID_SCENE, true,
                "scene resources invalid", -1.f, false)) return 1;
  depths.fill(0.f);
  if (!update_pixel_texture(depth_tex.Get(), depth_upload.Get(), depths.data(), 3 * sizeof(float))
      || !dispatch(4, 0.6f, RTXDI_SUN_STATUS_UNKNOWN, RTXDI_SUN_REASON_INVALID_SURFACE, true,
                   "invalid receiver depth")) return 1;
  depths.fill(0.5f);
  if (!update_pixel_texture(depth_tex.Get(), depth_upload.Get(), depths.data(), 3 * sizeof(float))) return 1;

  mesh.bbox_min[3] = 0.f;
  world_instances[0].header[2] = 0.f;
  if (!update_upload(meshes.Get(), &mesh, sizeof(mesh))
      || !update_upload(instances.Get(), world_instances.data(), sizeof(world_instances))) return 1;
  if (!dispatch(0, 0.6f, RTXDI_SUN_STATUS_BLOCKED, RTXDI_SUN_REASON_NONE, true, "environment bias", 0.29f)) return 1;
  for (UINT i = 0; i < 6; ++i) normal_pixels[i * 4 + 3] = 0u;
  if (!update_pixel_texture(normal_tex.Get(), normal_upload.Get(), normal_pixels.data(), 3 * 16)
      || !dispatch(0, 0.6f, RTXDI_SUN_STATUS_BLOCKED, RTXDI_SUN_REASON_NONE, true, "character bias", 0.1f,
                   true, 0.01f, 0.4f)) return 1;
  for (UINT i = 0; i < 6; ++i) normal_pixels[i * 4 + 3] = 8u;
  if (!update_pixel_texture(normal_tex.Get(), normal_upload.Get(), normal_pixels.data(), 3 * 16)) return 1;

  mesh.bbox_min[3] = 1.f;
  world_instances[0].header[2] = 1.f;
  alpha_material.threshold = 0.5f;
  alpha_material.slice = 0u;
  alpha_material.swizzle = 0u;
  if (!update_upload(meshes.Get(), &mesh, sizeof(mesh))
      || !update_upload(instances.Get(), world_instances.data(), sizeof(world_instances))
      || !update_upload(alpha_materials.Get(), &alpha_material, sizeof(alpha_material))) return 1;
  if (!update_atlas_texel(0u)
      || !dispatch(0, 0.6f, RTXDI_SUN_STATUS_BLOCKED, RTXDI_SUN_REASON_NONE, true, "alpha hole reveals opaque behind", 0.49f)
      || !update_atlas_texel(0xFFFFFFFFu)
      || !dispatch(0, 0.6f, RTXDI_SUN_STATUS_BLOCKED, RTXDI_SUN_REASON_NONE, true, "alpha accepted", 0.29f)
      || !dispatch(0, 0.6f, RTXDI_SUN_STATUS_BLOCKED, RTXDI_SUN_REASON_NONE, true, "foliage transmission reveals opaque behind", 0.49f, true, 0.01f, 0.02f, 0.5f, 0u)
      || !dispatch(0, 0.4f, RTXDI_SUN_STATUS_VISIBLE, RTXDI_SUN_REASON_NONE, true, "foliage transmission counts one layer", 0.4f, true, 0.01f, 0.02f, 0.5f, 1u)) return 1;
  world_instances[0].visibility[2] = 10.f;
  if (!update_upload(instances.Get(), world_instances.data(), sizeof(world_instances))
      || !dispatch(0, 0.4f, RTXDI_SUN_STATUS_VISIBLE, RTXDI_SUN_REASON_NONE, true, "wind transmission occludes one wind object", 0.4f, true, 0.01f, 0.02f, 0.f, 0u, 0.5f, 128u)
      || !dispatch(0, 0.6f, RTXDI_SUN_STATUS_BLOCKED, RTXDI_SUN_REASON_NONE, true, "wind transmission zero blocks", 0.29f, true, 0.01f, 0.02f, 0.f, 0u, 0.f, 0u)
      || !dispatch(0, 0.4f, RTXDI_SUN_STATUS_VISIBLE, RTXDI_SUN_REASON_NONE, true, "wind softening offset ray occludes one wind object", 0.4f, true, 0.01f, 0.02f, 0.f, 0u, 0.5f, 128u, 0.001f)) return 1;
  world_instances[0].visibility[2] = 2.f;
  if (!update_upload(instances.Get(), world_instances.data(), sizeof(world_instances))) return 1;
  alpha_material.swizzle = 1u;
  if (!update_upload(alpha_materials.Get(), &alpha_material, sizeof(alpha_material))
      || !update_atlas_texel(0u)
      || !dispatch(0, 0.6f, RTXDI_SUN_STATUS_BLOCKED, RTXDI_SUN_REASON_NONE, true, "forced opaque without UV", 0.29f)) return 1;
  alpha_material.swizzle = 0u;
  mesh.bbox_min[3] = 1.5f;
  if (!update_upload(meshes.Get(), &mesh, sizeof(mesh))
      || !update_upload(alpha_materials.Get(), &alpha_material, sizeof(alpha_material))
      || !dispatch(0, 0.6f, RTXDI_SUN_STATUS_UNKNOWN, RTXDI_SUN_REASON_INVALID_ALPHA, true, "invalid UV metadata")) return 1;

  auto valid_sun_scene = scene_pixels;
  for (UINT channel = 0u; channel < 3u; ++channel) {
    scene_pixels[(RTXDI_SURFACE_SUN_RESERVED_ROW - 1u) * 4u + channel] = std::bit_cast<uint32_t>(0.f);
  }
  if (!update_pixel_texture(scene_tex.Get(), scene_upload.Get(), scene_pixels.data(), RTXDI_SURFACE_SCENE_ROW_COUNT * 16u)
      || !dispatch(4, 0.6f, RTXDI_SUN_STATUS_UNKNOWN, RTXDI_SUN_REASON_INVALID_RAY, true,
                   "invalid sun radiance metadata")) return 1;
  scene_pixels = valid_sun_scene;
  scene_pixels[RTXDI_SURFACE_SUN_RESERVED_ROW * 4u + 2u] = std::bit_cast<uint32_t>(0.f);
  if (!update_pixel_texture(scene_tex.Get(), scene_upload.Get(), scene_pixels.data(), RTXDI_SURFACE_SCENE_ROW_COUNT * 16u)
      || !dispatch(4, 0.6f, RTXDI_SUN_STATUS_UNKNOWN, RTXDI_SUN_REASON_INVALID_RAY, true,
                   "invalid sun direction metadata")) return 1;
  scene_pixels = valid_sun_scene;
  if (!update_pixel_texture(scene_tex.Get(), scene_upload.Get(), scene_pixels.data(), RTXDI_SURFACE_SCENE_ROW_COUNT * 16u)) return 1;

  // Surface contract shader uses the same production reconstruction include with the exact t0-t2/u0 layout.
  D3D12_DESCRIPTOR_RANGE surface_ranges[2] = {};
  surface_ranges[0] = {D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 0, 0, 0};
  surface_ranges[1] = {D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0, 3};
  ComPtr<ID3D12RootSignature> surface_root;
  ComPtr<ID3D12PipelineState> surface_pso;
  if (!CreateRoot(device.Get(), surface_ranges, 2, 4, &surface_root)
      || !CreatePso(device.Get(), surface_root.Get(), __rtxdi_surface_contract, &surface_pso)) return 1;
  heap_desc.NumDescriptors = 4;
  ComPtr<ID3D12DescriptorHeap> surface_heap;
  if (!Check(device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&surface_heap)), "create surface heap")) return 1;
  const auto surface_increment = device->GetDescriptorHandleIncrementSize(heap_desc.Type);
  cpu = surface_heap->GetCPUDescriptorHandleForHeapStart();
  D3D12_SHADER_RESOURCE_VIEW_DESC surface_depth = depth_srv;
  device->CreateShaderResourceView(depth_tex.Get(), &surface_depth, cpu);
  cpu.ptr += surface_increment;
  device->CreateShaderResourceView(normal_tex.Get(), &uint4_srv, cpu);
  cpu.ptr += surface_increment;
  device->CreateShaderResourceView(scene_tex.Get(), &uint4_srv, cpu);
  ComPtr<ID3D12Resource> surface_output;
  ComPtr<ID3D12Resource> surface_readback;
  const auto surface_readback_desc = BufferDesc(sizeof(SurfaceResult));
  if (!CreateDefaultBuffer(device.Get(), sizeof(SurfaceResult), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                           D3D12_RESOURCE_STATE_COMMON, &surface_output)
      || !Check(device->CreateCommittedResource(&readback_properties, D3D12_HEAP_FLAG_NONE,
                                                &surface_readback_desc, D3D12_RESOURCE_STATE_COPY_DEST,
                                                nullptr, IID_PPV_ARGS(&surface_readback)),
                "create surface readback")) return 1;
  cpu = surface_heap->GetCPUDescriptorHandleForHeapStart();
  cpu.ptr += static_cast<SIZE_T>(3) * surface_increment;
  D3D12_UNORDERED_ACCESS_VIEW_DESC surface_uav = {};
  surface_uav.Format = DXGI_FORMAT_UNKNOWN;
  surface_uav.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
  surface_uav.Buffer.NumElements = 1;
  surface_uav.Buffer.StructureByteStride = sizeof(SurfaceResult);
  device->CreateUnorderedAccessView(surface_output.Get(), nullptr, &surface_uav, cpu);
  ID3D12DescriptorHeap* surface_heaps[] = {surface_heap.Get()};
  list->SetDescriptorHeaps(1, surface_heaps);
  list->SetComputeRootSignature(surface_root.Get());
  list->SetPipelineState(surface_pso.Get());
  list->SetComputeRootDescriptorTable(1, surface_heap->GetGPUDescriptorHandleForHeapStart());
  D3D12_RESOURCE_BARRIER surface_barrier = {};
  surface_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  surface_barrier.Transition.pResource = surface_output.Get();
  surface_barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  surface_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
  surface_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
  list->ResourceBarrier(1, &surface_barrier);
  const UINT surface_params[4] = {3, 2, 0, 0};
  list->SetComputeRoot32BitConstants(0, 4, surface_params, 0);
  list->Dispatch(1, 1, 1);
  surface_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  surface_barrier.Transition.pResource = surface_output.Get();
  surface_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
  surface_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
  list->ResourceBarrier(1, &surface_barrier);
  list->CopyBufferRegion(surface_readback.Get(), 0, surface_output.Get(), 0, sizeof(SurfaceResult));
  if (!submit_wait()) return 1;
  void* surface_mapped = nullptr;
  if (!Check(surface_readback->Map(0, nullptr, &surface_mapped), "map surface result")) return 1;
  const auto surface = *static_cast<const SurfaceResult*>(surface_mapped);
  surface_readback->Unmap(0, nullptr);
  if (surface.flags != RTXDI_SURFACE_FLAG_VALID || surface.rejection != RTXDI_SURFACE_REJECT_NONE
      || std::abs(surface.position[2] + 1.f) > 1e-4f || std::abs(surface.linear_depth - 1.f) > 1e-4f
      || std::abs(surface.normal[2] + 1.f) > 1e-4f || surface.view[2] < 0.99f) {
    std::fprintf(stderr, "FAIL production surface result flags=%u reject=%u pz=%f depth=%f nz=%f vz=%f\n",
                 surface.flags, surface.rejection, surface.position[2], surface.linear_depth, surface.normal[2], surface.view[2]);
    return 1;
  }

  // Execute the production classifier and initial DI shader with a 3x2 fixture.
  if (!Check(allocator->Reset(), "reset DI allocator") || !Check(base_list->Reset(allocator.Get(), nullptr), "reset DI list")) return 1;
  std::array<float, 3 * 2 * 4> albedo_pixels = {};
  for (size_t i = 0; i < 6; ++i) {
    albedo_pixels[i * 4 + 0] = 0.5f;
    albedo_pixels[i * 4 + 1] = 0.4f;
    albedo_pixels[i * 4 + 2] = 0.3f;
    albedo_pixels[i * 4 + 3] = 1.0f;
  }
  std::array<uint32_t, 3 * 2 * 4> mrt2_pixels = {};
  for (UINT i = 0; i < 6u; ++i) mrt2_pixels[i * 4u + 1u] = 1023u;
  std::array<uint32_t, 3 * 2 * 4> mrt1_pixels = {};
  for (UINT i = 0u; i < 6u; ++i) mrt1_pixels[i * 4u + 2u] = 255u;
  ComPtr<ID3D12Resource> albedo_tex, albedo_upload, mrt1_tex, mrt1_upload, mrt2_tex, mrt2_upload;
  ComPtr<ID3D12Resource> prev_depth_tex, prev_depth_upload, prev_normal_tex, prev_normal_upload;
  ComPtr<ID3D12Resource> prev_scene_tex, prev_scene_upload, prev_albedo_tex, prev_albedo_upload;
  ComPtr<ID3D12Resource> prev_mrt2_tex, prev_mrt2_upload, motion_tex, motion_upload;
  std::array<float, 3 * 2 * 4> motion_pixels = {};
  if (!CreateTextureUpload(device.Get(), list.Get(), DXGI_FORMAT_R32G32B32A32_FLOAT, 3, 2, albedo_pixels.data(),
                           3 * 4 * sizeof(float), &albedo_tex, &albedo_upload)
      || !CreateTextureUpload(device.Get(), list.Get(), DXGI_FORMAT_R32G32B32A32_UINT, 3, 2, mrt2_pixels.data(),
                              3 * 4 * sizeof(uint32_t), &mrt2_tex, &mrt2_upload)
      || !CreateTextureUpload(device.Get(), list.Get(), DXGI_FORMAT_R32G32B32A32_UINT, 3, 2, mrt1_pixels.data(),
                              3 * 4 * sizeof(uint32_t), &mrt1_tex, &mrt1_upload)
      || !CreateTextureUpload(device.Get(), list.Get(), DXGI_FORMAT_R32_FLOAT, 3, 2, depths.data(),
                              3 * sizeof(float), &prev_depth_tex, &prev_depth_upload)
      || !CreateTextureUpload(device.Get(), list.Get(), DXGI_FORMAT_R32G32B32A32_UINT, 3, 2, normal_pixels.data(),
                              3 * 16, &prev_normal_tex, &prev_normal_upload)
      || !CreateTextureUpload(device.Get(), list.Get(), DXGI_FORMAT_R32G32B32A32_UINT,
                              RTXDI_SURFACE_SCENE_ROW_COUNT, 1, previous_scene_pixels.data(), RTXDI_SURFACE_SCENE_ROW_COUNT * 16,
                              &prev_scene_tex, &prev_scene_upload)
      || !CreateTextureUpload(device.Get(), list.Get(), DXGI_FORMAT_R32G32B32A32_FLOAT, 3, 2, albedo_pixels.data(),
                              3 * 4 * sizeof(float), &prev_albedo_tex, &prev_albedo_upload)
      || !CreateTextureUpload(device.Get(), list.Get(), DXGI_FORMAT_R32G32B32A32_UINT, 3, 2, mrt2_pixels.data(),
                              3 * 4 * sizeof(uint32_t), &prev_mrt2_tex, &prev_mrt2_upload)
      || !CreateTextureUpload(device.Get(), list.Get(), DXGI_FORMAT_R32G32B32A32_FLOAT, 3, 2, motion_pixels.data(),
                              3 * 4 * sizeof(float), &motion_tex, &motion_upload)
      || !submit_wait()
      || !Check(allocator->Reset(), "reset DI input allocator")
      || !Check(base_list->Reset(allocator.Get(), nullptr), "reset DI input list")) return 1;

  for (UINT i = 0; i < 6; ++i) normal_pixels[i * 4 + 1] = 65535u;
  mesh.bbox_min[3] = 0.0f;
  world_instances[0].header[2] = 0.0f;
  if (!update_pixel_texture(normal_tex.Get(), normal_upload.Get(), normal_pixels.data(), 3 * 16)
      || !update_pixel_texture(prev_normal_tex.Get(), prev_normal_upload.Get(), normal_pixels.data(), 3 * 16)
      || !update_upload(meshes.Get(), &mesh, sizeof(mesh))
      || !update_upload(instances.Get(), world_instances.data(), sizeof(world_instances))) return 1;

  std::array<RtxdiLightParam, 4> lights = {};
  for (auto& lamp : lights) {
    lamp.pos[2] = 0.0f;
    lamp.radius = 4.0f;
    lamp.radius_inv = 0.25f;
    lamp.attenuation = 1.0f;
    lamp.attenuation_angle = 1.0f;
    lamp.spot_angle_inv = 1.0f;
    lamp.vec[2] = 1.0f;
  }
  lights[0].color[0] = 8.0f;
  lights[0].specular_intensity = 8.0f;
  lights[0].specular_glossiness = 16.0f;
  lights[1].color[1] = 0.0f;
  lights[2].color[0] = 100.0f;
  lights[3].color[0] = 100.0f;
  RtxdiTileData tile = {};
  std::fill(std::begin(tile.point_indices), std::end(tile.point_indices), -1);
  std::fill(std::begin(tile.spot_indices), std::end(tile.spot_indices), -1);
  tile.point_indices[0] = 0;
  tile.point_indices[1] = 2;
  tile.point_indices[2] = -1;
  tile.point_indices[3] = 99;
  tile.point_count = 4;
  tile.spot_indices[0] = 1;
  tile.spot_indices[1] = 2;
  tile.spot_indices[2] = -1;
  tile.spot_indices[3] = 99;
  tile.spot_count = 4;
  std::vector<RtxdiTileData> tile_records(RTXDI_INPUT_MAX_TILES);
  for (auto& tile_record : tile_records) {
    std::fill(std::begin(tile_record.point_indices), std::end(tile_record.point_indices), -1);
    std::fill(std::begin(tile_record.spot_indices), std::end(tile_record.spot_indices), -1);
  }
  tile_records[0] = tile;
  std::array<DeferredParam, 2> deferred = {};
  const float dynamic_intensity = 1.0f;
  const uint32_t local_specular_flag = 2u;
  for (auto& material : deferred) {
    const std::array<float, 3> material_gloss = {0.25f, 0.75f, 1.0f};
    std::memcpy(material.bytes + 64, material_gloss.data(), sizeof(material_gloss));
    std::memcpy(material.bytes + 76, &dynamic_intensity, sizeof(dynamic_intensity));
    std::memcpy(material.bytes + 136, &local_specular_flag, sizeof(local_specular_flag));
  }
  auto previous_deferred = deferred;
  ComPtr<ID3D12Resource> light_upload, previous_light_upload, previous_type_upload, tile_upload, deferred_upload,
      previous_deferred_upload, frame_upload;
  if (!UploadBuffer(device.Get(), lights.data(), sizeof(lights), &light_upload)
      || !UploadBuffer(device.Get(), lights.data(), sizeof(lights), &previous_light_upload)
      || !UploadBuffer(device.Get(), std::array<uint32_t, 4>{1u, 2u, 3u, 0u}.data(), sizeof(uint32_t) * lights.size(), &previous_type_upload)
      || !UploadBuffer(device.Get(), tile_records.data(), tile_records.size() * sizeof(RtxdiTileData), &tile_upload)
      || !UploadBuffer(device.Get(), deferred.data(), sizeof(deferred), &deferred_upload)
      || !UploadBuffer(device.Get(), previous_deferred.data(), sizeof(previous_deferred), &previous_deferred_upload)) return 1;

  RtxdiFrameUpload di_frame = {};
  di_frame.parameters.width = 3u;
  di_frame.parameters.height = 2u;
  di_frame.parameters.runtime.frameIndex = 19u;
  di_frame.parameters.di.reservoirBufferParams.reservoirBlockRowPitch = 256u;
  di_frame.parameters.di.reservoirBufferParams.reservoirArrayPitch = 256u;
  falcom_world::rtxdi::DiConfig bridge_di_config;
  if (!falcom_world::rtxdi::CalculateDiConfig(3u, 2u, &bridge_di_config)) return 1;
  const auto bridge_settings = falcom_world::rtxdi::CaptureEffectiveRtxdiSettings();
  auto bridge_di_context = falcom_world::rtxdi::CreateDiContext(bridge_di_config, bridge_settings);
  if (!bridge_di_context) return 1;
  di_frame.parameters = falcom_world::rtxdi::MakeDiFrameParameters(bridge_di_config, *bridge_di_context);
  di_frame.parameters.width = 3u;
  di_frame.parameters.height = 2u;
  di_frame.parameters.runtime.frameIndex = 19u;
  if (!UploadBuffer(device.Get(), &di_frame, sizeof(di_frame), &frame_upload)) return 1;

  constexpr UINT di_pixel_count = 6u;
  constexpr UINT di_guard_count = di_pixel_count + 1u;
  constexpr UINT di_row_pitch = 256u;
  constexpr UINT di_array_pitch = 256u;
  constexpr UINT di_reservoir_elements = 3u * di_array_pitch;
  const UINT64 di_reservoir_bytes = UINT64{di_reservoir_elements} * sizeof(RTXDI_PackedDIReservoir);
  const UINT64 di_raw_bytes = UINT64{di_guard_count} * sizeof(RtxdiDiRawResult);
  ComPtr<ID3D12Resource> di_types, di_reservoirs, di_raw, di_resolved, di_resolved_readback;
  ComPtr<ID3D12Resource> n1_diffuse, n1_specular, n1_status, n1_diffuse_readback, n1_specular_readback, n1_status_readback;
  ComPtr<ID3D12Resource> di_type_readback, di_reservoir_readback, di_raw_readback;
  ComPtr<ID3D12Resource> di_type_upload, di_reservoir_upload, di_raw_upload;
  if (!CreateDefaultBuffer(device.Get(), sizeof(uint32_t) * lights.size(), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                           D3D12_RESOURCE_STATE_COMMON, &di_types)
      || !CreateDefaultBuffer(device.Get(), di_reservoir_bytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                              D3D12_RESOURCE_STATE_COMMON, &di_reservoirs)
      || !CreateDefaultBuffer(device.Get(), di_raw_bytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                              D3D12_RESOURCE_STATE_COMMON, &di_raw)
      || !CreateTexture(device.Get(), DXGI_FORMAT_R32G32B32A32_UINT, 3u, 2u, 1u,
                        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON, &di_resolved)
      || !CreateTexture(device.Get(), DXGI_FORMAT_R16G16B16A16_FLOAT, 3u, 2u, 1u,
                        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON, &n1_diffuse)
      || !CreateTexture(device.Get(), DXGI_FORMAT_R16G16B16A16_FLOAT, 3u, 2u, 1u,
                        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON, &n1_specular)
      || !CreateTexture(device.Get(), DXGI_FORMAT_R32_UINT, 3u, 2u, 1u,
                        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON, &n1_status)
      || !UploadBuffer(device.Get(), lights.data(), sizeof(uint32_t) * lights.size(), &di_type_upload)) return 1;
  std::vector<RTXDI_PackedDIReservoir> reservoir_sentinel(di_reservoir_elements);
  std::memset(reservoir_sentinel.data(), 0xEF, di_reservoir_bytes);
  std::vector<RtxdiDiRawResult> raw_sentinel(di_guard_count);
  std::memset(raw_sentinel.data(), 0xEF, static_cast<size_t>(di_raw_bytes));
  if (!UploadBuffer(device.Get(), reservoir_sentinel.data(), di_reservoir_bytes, &di_reservoir_upload)
      || !UploadBuffer(device.Get(), raw_sentinel.data(), di_raw_bytes, &di_raw_upload)) return 1;
  const auto readback_desc = BufferDesc(di_reservoir_bytes);
  const auto raw_readback_desc = BufferDesc(di_raw_bytes);
  UINT resolved_readback_rows = 0u;
  UINT64 resolved_readback_bytes = 0u;
  D3D12_PLACED_SUBRESOURCE_FOOTPRINT resolved_footprint = {};
  const D3D12_RESOURCE_DESC resolved_desc = di_resolved->GetDesc();
  device->GetCopyableFootprints(&resolved_desc, 0u, 1u, 0u, &resolved_footprint, &resolved_readback_rows,
                                nullptr, &resolved_readback_bytes);
  D3D12_PLACED_SUBRESOURCE_FOOTPRINT n1_diffuse_footprint = {}, n1_specular_footprint = {}, n1_status_footprint = {};
  UINT n1_rows = 0u;
  UINT64 n1_diffuse_bytes = 0u, n1_specular_bytes = 0u, n1_status_bytes = 0u;
  const D3D12_RESOURCE_DESC n1_diffuse_desc = n1_diffuse->GetDesc();
  const D3D12_RESOURCE_DESC n1_specular_desc = n1_specular->GetDesc();
  const D3D12_RESOURCE_DESC n1_status_desc = n1_status->GetDesc();
  device->GetCopyableFootprints(&n1_diffuse_desc, 0u, 1u, 0u, &n1_diffuse_footprint, &n1_rows, nullptr, &n1_diffuse_bytes);
  device->GetCopyableFootprints(&n1_specular_desc, 0u, 1u, 0u, &n1_specular_footprint, &n1_rows, nullptr, &n1_specular_bytes);
  device->GetCopyableFootprints(&n1_status_desc, 0u, 1u, 0u, &n1_status_footprint, &n1_rows, nullptr, &n1_status_bytes);
  const auto resolved_readback_desc = BufferDesc(resolved_readback_bytes);
  const auto n1_diffuse_readback_desc = BufferDesc(n1_diffuse_bytes);
  const auto n1_specular_readback_desc = BufferDesc(n1_specular_bytes);
  const auto n1_status_readback_desc = BufferDesc(n1_status_bytes);
  const auto type_readback_desc = BufferDesc(sizeof(uint32_t) * lights.size());
  const auto readback_properties_di = Heap(D3D12_HEAP_TYPE_READBACK);
  if (!Check(device->CreateCommittedResource(&readback_properties_di, D3D12_HEAP_FLAG_NONE, &readback_desc,
                                             D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                             IID_PPV_ARGS(&di_reservoir_readback)),
             "create DI reservoir readback")
      || !Check(device->CreateCommittedResource(&readback_properties_di, D3D12_HEAP_FLAG_NONE, &raw_readback_desc,
                                                D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                                IID_PPV_ARGS(&di_raw_readback)),
                "create DI raw readback")
      || !Check(device->CreateCommittedResource(&readback_properties_di, D3D12_HEAP_FLAG_NONE, &resolved_readback_desc,
                                                D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                                IID_PPV_ARGS(&di_resolved_readback)),
                "create DI resolved readback")
      || !Check(device->CreateCommittedResource(&readback_properties_di, D3D12_HEAP_FLAG_NONE, &n1_diffuse_readback_desc,
                                                D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                                IID_PPV_ARGS(&n1_diffuse_readback)),
                "create N1 diffuse readback")
      || !Check(device->CreateCommittedResource(&readback_properties_di, D3D12_HEAP_FLAG_NONE, &n1_specular_readback_desc,
                                                D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                                IID_PPV_ARGS(&n1_specular_readback)),
                "create N1 specular readback")
      || !Check(device->CreateCommittedResource(&readback_properties_di, D3D12_HEAP_FLAG_NONE, &n1_status_readback_desc,
                                                D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                                IID_PPV_ARGS(&n1_status_readback)),
                "create N1 status readback")
      || !Check(device->CreateCommittedResource(&readback_properties_di, D3D12_HEAP_FLAG_NONE, &type_readback_desc,
                                                D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                                IID_PPV_ARGS(&di_type_readback)),
                "create DI type readback")) return 1;

  D3D12_DESCRIPTOR_HEAP_DESC di_heap_desc = {};
  di_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
  di_heap_desc.NumDescriptors = 40u;
  di_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
  ComPtr<ID3D12DescriptorHeap> di_heap;
  if (!Check(device->CreateDescriptorHeap(&di_heap_desc, IID_PPV_ARGS(&di_heap)), "create initial DI heap")) return 1;
  const UINT di_descriptor_size = device->GetDescriptorHandleIncrementSize(di_heap_desc.Type);
  auto di_cpu = di_heap->GetCPUDescriptorHandleForHeapStart();
  const auto di_cpu_at = [&](UINT index) {
    auto handle = di_cpu;
    handle.ptr += static_cast<SIZE_T>(index) * di_descriptor_size;
    return handle;
  };
  CreateBufferSrv(device.Get(), di_heap.Get(), di_descriptor_size, 0u, vertices.Get(), 3u, 16u);
  CreateBufferSrv(device.Get(), di_heap.Get(), di_descriptor_size, 1u, indices.Get(), 3u, 4u);
  CreateBufferSrv(device.Get(), di_heap.Get(), di_descriptor_size, 2u, meshes.Get(), 1u, sizeof(WorldMeshGPU));
  CreateBufferSrv(device.Get(), di_heap.Get(), di_descriptor_size, 3u, instances.Get(), 2u, sizeof(WorldInstanceGPU));
  CreateBufferSrv(device.Get(), di_heap.Get(), di_descriptor_size, 4u, uvs.Get(), 3u, sizeof(float) * 2u);
  device->CreateShaderResourceView(atlas.Get(), &atlas_srv, di_cpu_at(5u));
  CreateBufferSrv(device.Get(), di_heap.Get(), di_descriptor_size, 6u, alpha_materials.Get(), 1u, sizeof(AlphaMaterialGPU));
  CreateBufferSrv(device.Get(), di_heap.Get(), di_descriptor_size, 7u, light_upload.Get(), 4u, sizeof(RtxdiLightParam));
  CreateBufferSrv(device.Get(), di_heap.Get(), di_descriptor_size, 8u, di_types.Get(), 4u, sizeof(uint32_t));
  D3D12_SHADER_RESOURCE_VIEW_DESC albedo_srv = {};
  albedo_srv.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
  albedo_srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
  albedo_srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  albedo_srv.Texture2D.MipLevels = 1u;
  device->CreateShaderResourceView(albedo_tex.Get(), &albedo_srv, di_cpu_at(9u));
  D3D12_SHADER_RESOURCE_VIEW_DESC mrt2_srv = uint4_srv;
  device->CreateShaderResourceView(mrt2_tex.Get(), &mrt2_srv, di_cpu_at(10u));
  device->CreateShaderResourceView(mrt1_tex.Get(), &uint4_srv, di_cpu_at(35u));
  CreateBufferSrv(device.Get(), di_heap.Get(), di_descriptor_size, 11u, deferred_upload.Get(), 2u, RTXDI_INPUT_DEFERRED_STRIDE);
  std::array<int8_t, falcom_world::rtxdi::kNeighborCount * 2u> neighbor_offsets = {};
  for (UINT i = 0u; i < falcom_world::rtxdi::kNeighborCount; ++i) neighbor_offsets[i * 2u] = 32;
  ComPtr<ID3D12Resource> neighbor_upload;
  if (!UploadBuffer(device.Get(), neighbor_offsets.data(), sizeof(neighbor_offsets), &neighbor_upload)) return 1;
  D3D12_SHADER_RESOURCE_VIEW_DESC neighbor_srv = {};
  neighbor_srv.Format = DXGI_FORMAT_R8G8_SNORM;
  neighbor_srv.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
  neighbor_srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  neighbor_srv.Buffer.NumElements = falcom_world::rtxdi::kNeighborCount;
  device->CreateShaderResourceView(neighbor_upload.Get(), &neighbor_srv, di_cpu_at(12u));
  tlas_srv.RaytracingAccelerationStructure.Location = tlases[4]->GetGPUVirtualAddress();
  device->CreateShaderResourceView(nullptr, &tlas_srv, di_cpu_at(13u));
  CreateBufferSrv(device.Get(), di_heap.Get(), di_descriptor_size, 14u, dynamic_vertices.Get(), 3u, 16u);
  device->CreateShaderResourceView(depth_tex.Get(), &depth_srv, di_cpu_at(15u));
  device->CreateShaderResourceView(normal_tex.Get(), &uint4_srv, di_cpu_at(16u));
  device->CreateShaderResourceView(scene_tex.Get(), &uint4_srv, di_cpu_at(17u));
  device->CreateShaderResourceView(motion_tex.Get(), &albedo_srv, di_cpu_at(22u));
  device->CreateShaderResourceView(prev_depth_tex.Get(), &depth_srv, di_cpu_at(23u));
  device->CreateShaderResourceView(prev_normal_tex.Get(), &uint4_srv, di_cpu_at(24u));
  device->CreateShaderResourceView(prev_scene_tex.Get(), &uint4_srv, di_cpu_at(25u));
  device->CreateShaderResourceView(prev_albedo_tex.Get(), &albedo_srv, di_cpu_at(26u));
  device->CreateShaderResourceView(prev_mrt2_tex.Get(), &uint4_srv, di_cpu_at(27u));
  CreateBufferSrv(device.Get(), di_heap.Get(), di_descriptor_size, 28u, previous_deferred_upload.Get(), 2u,
                  RTXDI_INPUT_DEFERRED_STRIDE);
  CreateBufferSrv(device.Get(), di_heap.Get(), di_descriptor_size, 29u, previous_light_upload.Get(), 4u, sizeof(RtxdiLightParam));
  CreateBufferSrv(device.Get(), di_heap.Get(), di_descriptor_size, 30u, previous_type_upload.Get(), 4u, sizeof(uint32_t));
  const auto create_di_uav = [&](UINT index, ID3D12Resource* resource, UINT count, UINT stride) {
    D3D12_UNORDERED_ACCESS_VIEW_DESC view = {};
    view.Format = DXGI_FORMAT_UNKNOWN;
    view.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    view.Buffer.NumElements = count;
    view.Buffer.StructureByteStride = stride;
    device->CreateUnorderedAccessView(resource, nullptr, &view, di_cpu_at(index));
  };
  create_di_uav(18u, di_reservoirs.Get(), di_reservoir_elements, sizeof(RTXDI_PackedDIReservoir));
  create_di_uav(19u, di_raw.Get(), di_guard_count, sizeof(RtxdiDiRawResult));
  D3D12_UNORDERED_ACCESS_VIEW_DESC resolved_output_uav = {};
  resolved_output_uav.Format = DXGI_FORMAT_R32G32B32A32_UINT;
  resolved_output_uav.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
  device->CreateUnorderedAccessView(di_resolved.Get(), nullptr, &resolved_output_uav, di_cpu_at(20u));
  CreateBufferSrv(device.Get(), di_heap.Get(), di_descriptor_size, 33u, tile_upload.Get(), RTXDI_INPUT_MAX_TILES, sizeof(RtxdiTileData));
  create_di_uav(34u, di_types.Get(), 4u, sizeof(uint32_t));
  const auto create_n1_texture_uav = [&](UINT index, ID3D12Resource* resource, DXGI_FORMAT format) {
    D3D12_UNORDERED_ACCESS_VIEW_DESC view = {};
    view.Format = format;
    view.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    device->CreateUnorderedAccessView(resource, nullptr, &view, di_cpu_at(index));
  };
  create_n1_texture_uav(36u, n1_diffuse.Get(), DXGI_FORMAT_R16G16B16A16_FLOAT);
  create_n1_texture_uav(37u, n1_specular.Get(), DXGI_FORMAT_R16G16B16A16_FLOAT);
  create_n1_texture_uav(38u, n1_status.Get(), DXGI_FORMAT_R32_UINT);
  D3D12_UNORDERED_ACCESS_VIEW_DESC diag_view = {};
  diag_view.Format = DXGI_FORMAT_R32_TYPELESS;
  diag_view.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
  diag_view.Buffer.NumElements = RTXDI_DIAG_COUNTER_COUNT;
  diag_view.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
  device->CreateUnorderedAccessView(nullptr, nullptr, &diag_view, di_cpu_at(39u));
  device->CreateShaderResourceView(nullptr, &uint4_srv, di_cpu_at(21u));
  CreateBufferSrv(device.Get(), di_heap.Get(), di_descriptor_size, 31u, tile_upload.Get(), RTXDI_INPUT_MAX_TILES, sizeof(RtxdiTileData));
  CreateBufferSrv(device.Get(), di_heap.Get(), di_descriptor_size, 32u, tile_upload.Get(), RTXDI_INPUT_MAX_TILES, sizeof(RtxdiTileData));

  D3D12_DESCRIPTOR_RANGE classify_ranges[2] = {};
  classify_ranges[0] = {D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1u, 0u, 0u, 0u};
  classify_ranges[1] = {D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1u, 0u, 0u, 1u};
  D3D12_ROOT_PARAMETER classify_params[2] = {};
  classify_params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
  classify_params[0].Constants.Num32BitValues = 4u;
  classify_params[0].Constants.ShaderRegister = 12u;
  classify_params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
  classify_params[1].DescriptorTable.NumDescriptorRanges = 2u;
  classify_params[1].DescriptorTable.pDescriptorRanges = classify_ranges;
  D3D12_ROOT_SIGNATURE_DESC classify_desc = {};
  classify_desc.NumParameters = 2u;
  classify_desc.pParameters = classify_params;
  ComPtr<ID3DBlob> classify_blob, classify_errors;
  ComPtr<ID3D12RootSignature> classify_root;
  ComPtr<ID3D12PipelineState> classify_pso;
  if (!Check(D3D12SerializeRootSignature(&classify_desc, D3D_ROOT_SIGNATURE_VERSION_1,
                                         &classify_blob, &classify_errors),
             "serialize classifier root")
      || !Check(device->CreateRootSignature(0u, classify_blob->GetBufferPointer(), classify_blob->GetBufferSize(),
                                            IID_PPV_ARGS(&classify_root)),
                "create classifier root")
      || !CreatePso(device.Get(), classify_root.Get(), __world_rtxdi_classify_lights, &classify_pso)) return 1;

  D3D12_DESCRIPTOR_RANGE di_ranges[13] = {};
  di_ranges[0] = {D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4u, 0u, 0u, 0u};
  di_ranges[1] = {D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3u, 14u, 0u, 4u};
  di_ranges[2] = {D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 6u, 17u, 0u, 7u};
  di_ranges[3] = {D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1u, 29u, 0u, 13u};
  di_ranges[4] = {D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1u, 30u, 0u, 14u};
  di_ranges[5] = {D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3u, 31u, 0u, 15u};
  di_ranges[6] = {D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 3u, 0u, 0u, 18u};
  di_ranges[7] = {D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 9u, 34u, 0u, 22u};
  di_ranges[8] = {D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2u, 43u, 0u, 31u};
  di_ranges[9] = {D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1u, 45u, 0u, 35u};
  di_ranges[10] = {D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 3u, 3u, 0u, 36u};
  di_ranges[11] = {D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1u, 6u, 0u, 39u};
  di_ranges[12] = {D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1u, 61u, 0u, 21u};
  D3D12_ROOT_PARAMETER di_params[4] = {};
  di_params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
  di_params[0].Constants.Num32BitValues = 21u;
  di_params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
  di_params[1].Descriptor.ShaderRegister = 1u;
  di_params[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
  di_params[2].DescriptorTable.NumDescriptorRanges = 13u;
  di_params[2].DescriptorTable.pDescriptorRanges = di_ranges;
  di_params[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
  di_params[3].Constants.Num32BitValues = 10u;
  di_params[3].Constants.ShaderRegister = 12u;
  D3D12_ROOT_SIGNATURE_DESC di_root_desc = {};
  di_root_desc.NumParameters = 4u;
  di_root_desc.pParameters = di_params;
  ComPtr<ID3DBlob> di_root_blob, di_root_errors;
  ComPtr<ID3D12RootSignature> di_root;
  ComPtr<ID3D12PipelineState> di_pso, di_temporal_pso, di_spatial_pso, n1_reconstruct_pso;
  if (!Check(D3D12SerializeRootSignature(&di_root_desc, D3D_ROOT_SIGNATURE_VERSION_1,
                                         &di_root_blob, &di_root_errors),
             "serialize DI root")
      || !Check(device->CreateRootSignature(0u, di_root_blob->GetBufferPointer(), di_root_blob->GetBufferSize(),
                                            IID_PPV_ARGS(&di_root)),
                "create DI root")
      || !CreatePso(device.Get(), di_root.Get(), __world_rtxdi_initial_di, &di_pso)
      || !CreatePso(device.Get(), di_root.Get(), __world_rtxdi_temporal_di, &di_temporal_pso)
      || !CreatePso(device.Get(), di_root.Get(), __world_rtxdi_spatial_di, &di_spatial_pso)
      || !CreatePso(device.Get(), di_root.Get(), __world_rtxdi_reconstruct, &n1_reconstruct_pso)) return 1;

  D3D12_DESCRIPTOR_RANGE raw_guard_range = {};
  raw_guard_range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
  raw_guard_range.NumDescriptors = 1u;
  raw_guard_range.OffsetInDescriptorsFromTableStart = 0u;
  D3D12_ROOT_PARAMETER raw_guard_params[2] = {};
  raw_guard_params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
  raw_guard_params[0].Constants.Num32BitValues = 6u;
  raw_guard_params[0].Constants.ShaderRegister = 0u;
  raw_guard_params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
  raw_guard_params[1].DescriptorTable.NumDescriptorRanges = 1u;
  raw_guard_params[1].DescriptorTable.pDescriptorRanges = &raw_guard_range;
  D3D12_ROOT_SIGNATURE_DESC raw_guard_desc = {};
  raw_guard_desc.NumParameters = 2u;
  raw_guard_desc.pParameters = raw_guard_params;
  ComPtr<ID3DBlob> raw_guard_blob, raw_guard_errors;
  ComPtr<ID3D12RootSignature> raw_guard_root;
  ComPtr<ID3D12PipelineState> raw_guard_pso;
  if (!Check(D3D12SerializeRootSignature(&raw_guard_desc, D3D_ROOT_SIGNATURE_VERSION_1,
                                         &raw_guard_blob, &raw_guard_errors),
             "serialize raw guard root")
      || !Check(device->CreateRootSignature(0u, raw_guard_blob->GetBufferPointer(), raw_guard_blob->GetBufferSize(),
                                            IID_PPV_ARGS(&raw_guard_root)),
                "create raw guard root")
      || !CreatePso(device.Get(), raw_guard_root.Get(), __rtxdi_raw_guard_contract, &raw_guard_pso)) return 1;

  ID3D12DescriptorHeap* di_heaps[] = {di_heap.Get()};
  std::vector<RTXDI_PackedDIReservoir> prior_spatial_output;
  std::vector<RTXDI_PackedDIReservoir> prior_reservoir_state;
  UINT prior_spatial_output_index = UINT_MAX;
  UINT baseline_initial_m = 0u;
  bool suppress_di_pass = false;
  const auto upload_deferred_pair = [&]() {
    return update_upload(deferred_upload.Get(), deferred.data(), sizeof(deferred))
           && update_upload(previous_deferred_upload.Get(), previous_deferred.data(), sizeof(previous_deferred));
  };
  const auto upload_current_mrt2 = [&]() {
    return update_pixel_texture(mrt2_tex.Get(), mrt2_upload.Get(), mrt2_pixels.data(), 3u * 4u * sizeof(uint32_t));
  };
  std::array<uint16_t, 3> n1_scale_diffuse = {};
  std::array<uint16_t, 3> n1_scale_specular = {};
  std::array<uint16_t, 3> n1_scale_specular_hit_t = {};
  std::array<uint16_t, 4> n1_formula_diffuse = {};
  std::array<uint16_t, 4> n1_formula_specular = {};
  float n1_formula_inverse_pdf = 0.0f;
  std::array<float, 7> n1_test_config = {0.0f, 1.0f, 0.0f, 1.0f, 4.0f, 0.04f, 1.0f};
  auto run_di = [&](UINT mask_index, UINT local_lights, bool coverage_valid, UINT expected_status,
                    UINT expected_reason, int expected_light_index, const char* label, bool check_types,
                    UINT deferred_count = 1u, bool spatial = false, UINT expected_spatial_m = 0u,
                    bool compatibility_only = false, bool preserve_reservoirs = false, bool temporal = false,
                    bool expect_temporal_merge = false, bool temporal_history_valid = true,
                    bool expect_zero_diffuse = false, float* pixel_zero_target_pdf = nullptr,
                    UINT* pixel_zero_sample_count = nullptr, float* pixel_zero_reservoir_weight = nullptr,
                    float* pixel_zero_raw_radiance = nullptr, float* pixel_zero_inverse_pdf = nullptr,
                    bool check_unwritten_arrays = false, UINT extra_flags = 0u) {
    if (mask_index >= tlases.size() || local_lights > 4u) return false;
    wait_context = label;
    wait_has_di_dispatch = true;
    bridge_di_context->SetResamplingMode(temporal && spatial ? ::rtxdi::ReSTIRDI_ResamplingMode::TemporalAndSpatial
                                         : temporal          ? ::rtxdi::ReSTIRDI_ResamplingMode::Temporal
                                         : spatial           ? ::rtxdi::ReSTIRDI_ResamplingMode::Spatial
                                                             : ::rtxdi::ReSTIRDI_ResamplingMode::None);
    di_frame.parameters = falcom_world::rtxdi::MakeDiFrameParameters(bridge_di_config, *bridge_di_context);
    di_frame.parameters.width = 3u;
    di_frame.parameters.height = 2u;
    di_frame.parameters.flags = RTXDI_FRAME_FLAG_SPATIAL_AFTER_INITIAL
                                | (spatial ? RTXDI_FRAME_FLAG_SPATIAL_ENABLED : 0u) | extra_flags;
    di_frame.parameters.history_valid = temporal && temporal_history_valid ? 1u : 0u;
    di_frame.parameters.di.bufferIndices = bridge_di_context->GetBufferIndices();
    auto effective = falcom_world::rtxdi::CaptureEffectiveRtxdiSettings();
    effective.temporal_enabled = temporal;
    effective.spatial_enabled = spatial;
    if (!falcom_world::rtxdi::ValidDiBufferIndices(di_frame.parameters.di.bufferIndices, effective)) return false;
    if (!update_upload(frame_upload.Get(), &di_frame, sizeof(di_frame))) return false;
    tlas_srv.RaytracingAccelerationStructure.Location = tlases[mask_index]->GetGPUVirtualAddress();
    device->CreateShaderResourceView(nullptr, &tlas_srv, di_cpu_at(13u));
    CreateBufferSrv(device.Get(), di_heap.Get(), di_descriptor_size, 11u, deferred_upload.Get(),
                    (std::max)(deferred_count, 1u), RTXDI_INPUT_DEFERRED_STRIDE);
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.pResource = di_reservoirs.Get();
    if (!preserve_reservoirs) {
      list->ResourceBarrier(1u, &barrier);
      list->CopyBufferRegion(di_reservoirs.Get(), 0u, di_reservoir_upload.Get(), 0u, di_reservoir_bytes);
    }
    barrier.Transition.pResource = di_raw.Get();
    list->ResourceBarrier(1u, &barrier);
    list->CopyBufferRegion(di_raw.Get(), 0u, di_raw_upload.Get(), 0u, di_raw_bytes);
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    barrier.Transition.pResource = di_reservoirs.Get();
    if (preserve_reservoirs) {
      barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
      list->ResourceBarrier(1u, &barrier);
    } else {
      list->ResourceBarrier(1u, &barrier);
    }
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    barrier.Transition.pResource = di_raw.Get();
    list->ResourceBarrier(1u, &barrier);
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    barrier.Transition.pResource = di_types.Get();
    list->ResourceBarrier(1u, &barrier);
    list->SetDescriptorHeaps(1u, di_heaps);
    list->SetComputeRootSignature(classify_root.Get());
    list->SetPipelineState(classify_pso.Get());
    const UINT clear_params[4] = {local_lights, RTXDI_INPUT_MAX_TILES, 0u, 0u};
    list->SetComputeRoot32BitConstants(0u, 4u, clear_params, 0u);
    auto classify_table = di_heap->GetGPUDescriptorHandleForHeapStart();
    classify_table.ptr += static_cast<UINT64>(33u) * di_descriptor_size;
    list->SetComputeRootDescriptorTable(1u, classify_table);
    if (local_lights != 0u) list->Dispatch((local_lights + 63u) / 64u, 1u, 1u);
    if (local_lights != 0u) {
      D3D12_RESOURCE_BARRIER uav = {};
      uav.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
      uav.UAV.pResource = di_types.Get();
      list->ResourceBarrier(1u, &uav);
      const UINT tile_params[4] = {local_lights, RTXDI_INPUT_MAX_TILES, 1u, 0u};
      list->SetComputeRoot32BitConstants(0u, 4u, tile_params, 0u);
      list->Dispatch((RTXDI_INPUT_MAX_TILES + 63u) / 64u, 1u, 1u);
    }
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    barrier.Transition.pResource = di_types.Get();
    list->ResourceBarrier(1u, &barrier);
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    barrier.Transition.pResource = di_resolved.Get();
    list->ResourceBarrier(1u, &barrier);
    list->SetComputeRootSignature(di_root.Get());
    list->SetPipelineState(di_pso.Get());
    const UINT params[21] = {3u, 2u, 1u, coverage_valid ? 1u : 0u, local_lights,
                             std::bit_cast<UINT>(0.01f), std::bit_cast<UINT>(0.02f), std::bit_cast<UINT>(0.001f),
                             std::bit_cast<UINT>(-2.0f), std::bit_cast<UINT>(-2.0f), std::bit_cast<UINT>(-2.0f),
                             std::bit_cast<UINT>(4.0f), deferred_count, deferred_count, local_lights,
                             RTXDI_INPUT_MAX_TILES, RTXDI_INPUT_MAX_TILES, 0u, 0u, 0u, 0u};
    list->SetComputeRoot32BitConstants(0u, 21u, params, 0u);
    list->SetComputeRootConstantBufferView(1u, frame_upload->GetGPUVirtualAddress());
    list->SetComputeRootDescriptorTable(2u, di_heap->GetGPUDescriptorHandleForHeapStart());
    list->Dispatch(1u, 1u, 1u);
    D3D12_RESOURCE_BARRIER reservoir_uav = {};
    reservoir_uav.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    reservoir_uav.UAV.pResource = di_reservoirs.Get();
    list->ResourceBarrier(1u, &reservoir_uav);
    if (temporal) {
      list->SetPipelineState(di_temporal_pso.Get());
      list->Dispatch(1u, 1u, 1u);
      list->ResourceBarrier(1u, &reservoir_uav);
    }
    list->SetPipelineState(di_spatial_pso.Get());
    list->Dispatch(1u, 1u, 1u);
    D3D12_RESOURCE_BARRIER spatial_uav_barriers[2] = {};
    spatial_uav_barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    spatial_uav_barriers[0].UAV.pResource = di_resolved.Get();
    spatial_uav_barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    spatial_uav_barriers[1].UAV.pResource = di_raw.Get();
    list->ResourceBarrier(2u, spatial_uav_barriers);

    D3D12_RESOURCE_BARRIER n1_transitions[3] = {};
    const auto set_n1_transition = [&](UINT index, ID3D12Resource* resource, D3D12_RESOURCE_STATES before,
                                       D3D12_RESOURCE_STATES after) {
      n1_transitions[index].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
      n1_transitions[index].Transition.pResource = resource;
      n1_transitions[index].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
      n1_transitions[index].Transition.StateBefore = before;
      n1_transitions[index].Transition.StateAfter = after;
    };
    set_n1_transition(0u, n1_diffuse.Get(), D3D12_RESOURCE_STATE_COMMON,
                      D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    set_n1_transition(1u, n1_specular.Get(), D3D12_RESOURCE_STATE_COMMON,
                      D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    set_n1_transition(2u, n1_status.Get(), D3D12_RESOURCE_STATE_COMMON,
                      D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    list->ResourceBarrier(3u, n1_transitions);
    list->SetPipelineState(n1_reconstruct_pso.Get());
    const UINT n1_params[10] = {RTXDI_N1_CONFIG_VERSION,
                                std::bit_cast<UINT>(n1_test_config[0]), std::bit_cast<UINT>(n1_test_config[1]),
                                std::bit_cast<UINT>(n1_test_config[2]), std::bit_cast<UINT>(n1_test_config[3]),
                                std::bit_cast<UINT>(n1_test_config[4]), std::bit_cast<UINT>(n1_test_config[5]),
                                std::bit_cast<UINT>(n1_test_config[6]), std::bit_cast<UINT>(0.0f),
                                std::bit_cast<UINT>(0.0f)};
    list->SetComputeRoot32BitConstants(3u, 10u, n1_params, 0u);
    list->Dispatch(1u, 1u, 1u);
    D3D12_RESOURCE_BARRIER n1_uav_barriers[3] = {};
    n1_uav_barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    n1_uav_barriers[0].UAV.pResource = n1_diffuse.Get();
    n1_uav_barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    n1_uav_barriers[1].UAV.pResource = n1_specular.Get();
    n1_uav_barriers[2].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    n1_uav_barriers[2].UAV.pResource = n1_status.Get();
    list->ResourceBarrier(3u, n1_uav_barriers);
    D3D12_RESOURCE_BARRIER barriers[5] = {};
    barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barriers[0].UAV.pResource = di_reservoirs.Get();
    barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barriers[1].UAV.pResource = di_raw.Get();
    list->ResourceBarrier(2u, barriers);
    const auto transition = [&](UINT barrier_index, ID3D12Resource* resource,
                                D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to) {
      barriers[barrier_index] = {};
      barriers[barrier_index].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
      barriers[barrier_index].Transition.pResource = resource;
      barriers[barrier_index].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
      barriers[barrier_index].Transition.StateBefore = from;
      barriers[barrier_index].Transition.StateAfter = to;
    };
    transition(0u, di_types.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_SOURCE);
    transition(1u, di_reservoirs.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
    transition(2u, di_raw.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
    list->ResourceBarrier(3u, barriers);
    list->CopyBufferRegion(di_type_readback.Get(), 0u, di_types.Get(), 0u, sizeof(uint32_t) * lights.size());
    list->CopyBufferRegion(di_reservoir_readback.Get(), 0u, di_reservoirs.Get(), 0u, di_reservoir_bytes);
    list->CopyBufferRegion(di_raw_readback.Get(), 0u, di_raw.Get(), 0u, di_raw_bytes);
    transition(3u, di_resolved.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
    list->ResourceBarrier(1u, &barriers[3]);
    D3D12_TEXTURE_COPY_LOCATION resolved_source = {};
    resolved_source.pResource = di_resolved.Get();
    resolved_source.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    D3D12_TEXTURE_COPY_LOCATION resolved_destination = {};
    resolved_destination.pResource = di_resolved_readback.Get();
    resolved_destination.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    resolved_destination.PlacedFootprint = resolved_footprint;
    list->CopyTextureRegion(&resolved_destination, 0u, 0u, 0u, &resolved_source, nullptr);
    transition(3u, di_resolved.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COMMON);
    list->ResourceBarrier(1u, &barriers[3]);

    transition(3u, n1_diffuse.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
    transition(4u, n1_specular.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
    list->ResourceBarrier(2u, &barriers[3]);
    D3D12_TEXTURE_COPY_LOCATION n1_source = {};
    n1_source.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    D3D12_TEXTURE_COPY_LOCATION n1_destination = {};
    n1_destination.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    n1_source.pResource = n1_diffuse.Get();
    n1_destination.pResource = n1_diffuse_readback.Get();
    n1_destination.PlacedFootprint = n1_diffuse_footprint;
    list->CopyTextureRegion(&n1_destination, 0u, 0u, 0u, &n1_source, nullptr);
    n1_source.pResource = n1_specular.Get();
    n1_destination.pResource = n1_specular_readback.Get();
    n1_destination.PlacedFootprint = n1_specular_footprint;
    list->CopyTextureRegion(&n1_destination, 0u, 0u, 0u, &n1_source, nullptr);
    transition(3u, n1_status.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
    list->ResourceBarrier(1u, &barriers[3]);
    n1_source.pResource = n1_status.Get();
    n1_destination.pResource = n1_status_readback.Get();
    n1_destination.PlacedFootprint = n1_status_footprint;
    list->CopyTextureRegion(&n1_destination, 0u, 0u, 0u, &n1_source, nullptr);
    transition(3u, n1_diffuse.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COMMON);
    transition(4u, n1_specular.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COMMON);
    list->ResourceBarrier(2u, &barriers[3]);
    transition(3u, n1_status.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COMMON);
    list->ResourceBarrier(1u, &barriers[3]);
    transition(0u, di_types.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COMMON);
    transition(1u, di_reservoirs.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COMMON);
    transition(2u, di_raw.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COMMON);
    list->ResourceBarrier(3u, barriers);
    const UINT64 info_messages_before_submit = info_queue->GetNumStoredMessages();
    if (!submit_wait()) return false;
    for (UINT64 message_index = info_messages_before_submit;
         message_index < info_queue->GetNumStoredMessages(); ++message_index) {
      SIZE_T message_size = 0;
      if (FAILED(info_queue->GetMessage(message_index, nullptr, &message_size))) continue;
      std::vector<uint8_t> message_storage(message_size);
      auto* debug_message = reinterpret_cast<D3D12_MESSAGE*>(message_storage.data());
      if (SUCCEEDED(info_queue->GetMessage(message_index, debug_message, &message_size))
          && debug_message->Severity <= D3D12_MESSAGE_SEVERITY_WARNING) {
        std::fprintf(stderr, "FAIL %s generated D3D12 debug id=%d: %s\n", label,
                     static_cast<int>(debug_message->ID), debug_message->pDescription);
      }
    }
    if (!Check(allocator->Reset(), "reset local DI allocator")
        || !Check(base_list->Reset(allocator.Get(), nullptr), "reset local DI list")) return false;
    void* types_mapped = nullptr;
    void* reservoirs_mapped = nullptr;
    void* raw_mapped = nullptr;
    void* resolved_mapped = nullptr;
    void* n1_diffuse_mapped = nullptr;
    void* n1_specular_mapped = nullptr;
    void* n1_status_mapped = nullptr;
    if (!Check(di_type_readback->Map(0u, nullptr, &types_mapped), "map local types")
        || !Check(di_reservoir_readback->Map(0u, nullptr, &reservoirs_mapped), "map local reservoirs")
        || !Check(di_raw_readback->Map(0u, nullptr, &raw_mapped), "map local raw result")) return false;
    if (!Check(di_resolved_readback->Map(0u, nullptr, &resolved_mapped), "map local resolved output")
        || !Check(n1_diffuse_readback->Map(0u, nullptr, &n1_diffuse_mapped), "map N1 diffuse")
        || !Check(n1_specular_readback->Map(0u, nullptr, &n1_specular_mapped), "map N1 specular")
        || !Check(n1_status_readback->Map(0u, nullptr, &n1_status_mapped), "map N1 status")) return false;
    std::array<uint32_t, 4> read_types = {};
    std::memcpy(read_types.data(), types_mapped, sizeof(read_types));
    const auto* reservoirs = static_cast<const RTXDI_PackedDIReservoir*>(reservoirs_mapped);
    const auto* raw = static_cast<const RtxdiDiRawResult*>(raw_mapped);
    const auto* resolved = static_cast<const uint32_t*>(resolved_mapped);
    const UINT result_index = di_frame.parameters.di.bufferIndices.shadingInputBufferIndex;
    if (pixel_zero_target_pdf != nullptr) {
      *pixel_zero_target_pdf = reservoirs[result_index * di_array_pitch].targetPdf;
    }
    if (pixel_zero_sample_count != nullptr) {
      *pixel_zero_sample_count = (reservoirs[result_index * di_array_pitch].mVisibility >> 18u) & 0x3fffu;
    }
    if (pixel_zero_reservoir_weight != nullptr) {
      *pixel_zero_reservoir_weight = reservoirs[result_index * di_array_pitch].weight;
    }
    if (pixel_zero_raw_radiance != nullptr) *pixel_zero_raw_radiance = raw[0].diffuse_radiance[0];
    if (pixel_zero_inverse_pdf != nullptr) *pixel_zero_inverse_pdf = raw[0].inverse_pdf;
    const UINT expected_previous_output_index = prior_spatial_output_index;
    bool ok = true;
    if (check_types && read_types != std::array<uint32_t, 4>{1u, 2u, 3u, 0u}) {
      std::fprintf(stderr, "FAIL %s type bits %u,%u,%u,%u\n", label, read_types[0], read_types[1], read_types[2], read_types[3]);
      ok = false;
    }
    for (UINT y = 0; y < 2u; ++y) {
      for (UINT x = 0; x < 3u; ++x) {
        const UINT pixel = y * 3u + x;
        {
          const auto* status_row = reinterpret_cast<const uint32_t*>(static_cast<const uint8_t*>(n1_status_mapped)
                                                                     + y * n1_status_footprint.Footprint.RowPitch);
          const auto* diffuse_row = reinterpret_cast<const uint16_t*>(static_cast<const uint8_t*>(n1_diffuse_mapped)
                                                                      + y * n1_diffuse_footprint.Footprint.RowPitch);
          const auto* specular_row = reinterpret_cast<const uint16_t*>(static_cast<const uint8_t*>(n1_specular_mapped)
                                                                       + y * n1_specular_footprint.Footprint.RowPitch);
          if (y == 0u && x == 0u) {
            if (std::strcmp(label, "RTXDI N1 formula capture") == 0) {
              std::copy_n(diffuse_row, 4u, n1_formula_diffuse.begin());
              std::copy_n(specular_row, 4u, n1_formula_specular.begin());
            }
            const char* scale_labels[] = {"RTXDI N1 MRT1.z 0", "RTXDI N1 MRT1.z 128", "RTXDI N1 MRT1.z 255"};
            for (UINT scale_index = 0u; scale_index < 3u; ++scale_index) {
              if (std::strcmp(label, scale_labels[scale_index]) == 0) {
                n1_scale_diffuse[scale_index] = diffuse_row[0];
                n1_scale_specular[scale_index] = specular_row[0];
                n1_scale_specular_hit_t[scale_index] = specular_row[3];
              }
            }
          }
          uint32_t expected_n1_code = RTXDI_N1_STATUS_EMPTY;
          bool expected_known = false;
          if (raw[pixel].status == RTXDI_DI_STATUS_VISIBLE) {
            expected_n1_code = RTXDI_N1_STATUS_VISIBLE;
            expected_known = true;
          } else if (raw[pixel].status == RTXDI_DI_STATUS_BLOCKED) {
            expected_n1_code = RTXDI_N1_STATUS_BLOCKED;
            expected_known = true;
          } else if (raw[pixel].status == RTXDI_DI_STATUS_UNKNOWN) {
            expected_n1_code = RTXDI_N1_STATUS_UNKNOWN;
          } else if (raw[pixel].reason == RTXDI_DI_REASON_INVALID_SURFACE) {
            expected_n1_code = RTXDI_N1_STATUS_INVALID;
          }
          const uint32_t expected_n1_mask = expected_n1_code | (expected_known ? RTXDI_N1_STATUS_KNOWN_BIT : 0u);
          const bool outputs_zero = diffuse_row[x * 4u] == 0u && diffuse_row[x * 4u + 1u] == 0u
                                    && diffuse_row[x * 4u + 2u] == 0u && diffuse_row[x * 4u + 3u] == 0u
                                    && specular_row[x * 4u] == 0u && specular_row[x * 4u + 1u] == 0u
                                    && specular_row[x * 4u + 2u] == 0u && specular_row[x * 4u + 3u] == 0u;
          const bool diffuse_has_rgb = diffuse_row[x * 4u] != 0u || diffuse_row[x * 4u + 1u] != 0u
                                       || diffuse_row[x * 4u + 2u] != 0u;
          const bool specular_has_rgb = specular_row[x * 4u] != 0u || specular_row[x * 4u + 1u] != 0u
                                        || specular_row[x * 4u + 2u] != 0u;
          const bool n1_character = (status_row[x] & RTXDI_N1_STATUS_CHARACTER_BIT) != 0u;
          const bool any_n1_rgb = diffuse_has_rgb || specular_has_rgb;
          if ((status_row[x] & (RTXDI_N1_STATUS_KNOWN_BIT | 0x0fu)) != expected_n1_mask
              || (!expected_known && !outputs_zero)
              || (expected_n1_code == RTXDI_N1_STATUS_BLOCKED && !outputs_zero)
              || (expected_n1_code == RTXDI_N1_STATUS_VISIBLE
                  && ((diffuse_row[x * 4u + 3u] != 0u) != any_n1_rgb
                      || (n1_character && specular_row[x * 4u + 3u] != 0u)
                      || (!any_n1_rgb && specular_row[x * 4u + 3u] != 0u)))) {
            std::fprintf(stderr, "FAIL %s N1 pixel=%u raw=%u/%u idx=%u type=%u inv=%g flags=%u status=%u expected=%u diffuseA=%u specA=%u\n", label,
                         pixel, raw[pixel].status, raw[pixel].reason, raw[pixel].selected_light_index,
                         raw[pixel].selected_light_type, raw[pixel].inverse_pdf, raw[pixel].flags,
                         status_row[x], expected_n1_mask,
                         diffuse_row[x * 4u + 3u], specular_row[x * 4u + 3u]);
            ok = false;
          }
          if (std::strcmp(label, "RTXDI N1 visible environment output") == 0
              && expected_n1_code == RTXDI_N1_STATUS_VISIBLE
              && (diffuse_row[x * 4u] == 0u || specular_row[x * 4u] == 0u)) ok = false;
        }
        if (!compatibility_only && (raw[pixel].status != expected_status || raw[pixel].reason != expected_reason)) {
          std::fprintf(stderr, "FAIL %s pixel=%u status=%u expected=%u reason=%u expectedReason=%u\n", label, pixel,
                       raw[pixel].status, expected_status, raw[pixel].reason, expected_reason);
          ok = false;
        }
        if (!compatibility_only && expected_light_index >= 0
            && (raw[pixel].selected_light_index != static_cast<UINT>(expected_light_index)
                || raw[pixel].selected_light_type != read_types[static_cast<size_t>(expected_light_index)]
                || !std::isfinite(raw[pixel].inverse_pdf) || raw[pixel].inverse_pdf <= 0.0f)) {
          std::fprintf(stderr, "FAIL %s raw sample metadata pixel=%u index=%u type=%u inv_pdf=%g\n", label, pixel,
                       raw[pixel].selected_light_index, raw[pixel].selected_light_type, raw[pixel].inverse_pdf);
          ok = false;
        }
        if (!compatibility_only && expected_light_index < 0
            && raw[pixel].selected_light_index != UINT_MAX) {
          std::fprintf(stderr, "FAIL %s empty sample index pixel=%u index=%u\n", label, pixel,
                       raw[pixel].selected_light_index);
          ok = false;
        }
        {
          const auto* resolved_pixel = resolved + (y * resolved_footprint.Footprint.RowPitch / sizeof(uint32_t)) + x * 4u;
          if (resolved_pixel[0] != raw[pixel].selected_light_index || resolved_pixel[1] != raw[pixel].selected_light_type
              || resolved_pixel[2] != raw[pixel].status || resolved_pixel[3] != std::bit_cast<uint32_t>(raw[pixel].inverse_pdf)) {
            std::fprintf(stderr, "FAIL %s resolved output mismatch pixel=%u idx=%u/%u type=%u/%u status=%u/%u inv=%08x/%08x\n",
                         label, pixel, resolved_pixel[0], raw[pixel].selected_light_index, resolved_pixel[1],
                         raw[pixel].selected_light_type, resolved_pixel[2], raw[pixel].status, resolved_pixel[3],
                         std::bit_cast<uint32_t>(raw[pixel].inverse_pdf));
            ok = false;
          }
        }
        bool found_selected = false;
        const UINT initial_index = di_frame.parameters.di.bufferIndices.initialSamplingOutputBufferIndex;
        const UINT previous_index = di_frame.parameters.di.bufferIndices.temporalResamplingInputBufferIndex;
        for (UINT array_index = 0; array_index < 3u; ++array_index) {
          const UINT pointer = array_index * di_array_pitch + y * 16u + x;
          const auto& reservoir = reservoirs[pointer];
          if (pixel == 0u && !temporal && !spatial && array_index == initial_index) {
            baseline_initial_m = (reservoir.mVisibility >> 18u) & 0x3fffu;
          }
          if (pixel == 0u && temporal && array_index == initial_index) {
            const UINT temporal_m = (reservoir.mVisibility >> 18u) & 0x3fffu;
            if (expect_temporal_merge && temporal_m <= baseline_initial_m) {
              std::fprintf(stderr, "FAIL %s temporal M=%u baseline=%u merge=%u\n", label,
                           temporal_m, baseline_initial_m, static_cast<unsigned>(expect_temporal_merge));
              ok = false;
            }
          }
          const bool writable_array = array_index == initial_index || (temporal && array_index == result_index)
                                      || (spatial && array_index == result_index);
          if (writable_array) {
            if (compatibility_only) {
              if (pixel == 0u && array_index == result_index) {
                const UINT sample_count = (reservoir.mVisibility >> 18u) & 0x3fffu;
                if ((reservoir.lightData & 0x80000000u) == 0u
                    || (expected_spatial_m != 0u && sample_count != expected_spatial_m)) {
                  std::fprintf(stderr, "FAIL %s spatial M=%u expected=%u valid=%u\n", label, sample_count,
                               expected_spatial_m, (reservoir.lightData >> 31u) & 1u);
                  ok = false;
                }
              }
            } else if (expected_light_index >= 0) {
              const UINT selected = reservoir.lightData & 0x7FFFFFFFu;
              if ((reservoir.lightData & 0x80000000u) == 0u
                  || (array_index == result_index && selected != static_cast<UINT>(expected_light_index))
                  || !std::isfinite(reservoir.weight) || reservoir.weight <= 0.0f
                  || !std::isfinite(reservoir.targetPdf) || reservoir.targetPdf <= 0.0f) {
                std::fprintf(stderr, "FAIL %s selected array=%u light=%u weight=%f target=%f\n", label, array_index,
                             selected, reservoir.weight, reservoir.targetPdf);
                ok = false;
              }
              if (array_index == result_index) found_selected = true;
              if (array_index == result_index && raw[pixel].selected_light_index != UINT_MAX
                  && (!std::isfinite(raw[pixel].inverse_pdf)
                      || std::abs(raw[pixel].inverse_pdf - reservoir.weight)
                             > (std::max)(1e-6f, std::abs(reservoir.weight) * 1e-6f))) {
                std::fprintf(stderr,
                             "FAIL %s raw/reservoir identity pixel=%u selected=%u/%u type=%u invPdf=%g weight=%g targetPdf=%g\n",
                             label, pixel, raw[pixel].selected_light_index, selected, raw[pixel].selected_light_type,
                             raw[pixel].inverse_pdf, reservoir.weight, reservoir.targetPdf);
                ok = false;
              }
            } else if (reservoir.lightData != 0u || reservoir.weight != 0.0f || reservoir.targetPdf != 0.0f) {
              std::fprintf(stderr, "FAIL %s nonempty writable array=%u pixel=%u lightData=%08x weight=%f target=%f\n",
                           label, array_index, pixel, reservoir.lightData, reservoir.weight, reservoir.targetPdf);
              ok = false;
            }
          } else {
            if (check_unwritten_arrays) {
              if (preserve_reservoirs) {
                const size_t prior_pointer = static_cast<size_t>(array_index) * di_array_pitch + y * 16u + x;
                if (prior_reservoir_state.size() != di_reservoir_elements
                    || std::memcmp(&reservoir, &prior_reservoir_state[prior_pointer], sizeof(reservoir)) != 0) {
                  std::fprintf(stderr, "FAIL %s unwritten reservoir array=%u pixel=%u changed\n", label,
                               array_index, pixel);
                  ok = false;
                }
              } else {
                const auto* words = reinterpret_cast<const uint32_t*>(&reservoir);
                for (UINT word = 0; word < sizeof(reservoir) / sizeof(uint32_t); ++word) {
                  if (words[word] != 0xEFEFEFEFu) {
                    std::fprintf(stderr, "FAIL %s unwritten reservoir sentinel array=%u pixel=%u word=%u value=%08x\n",
                                 label, array_index, pixel, word, words[word]);
                    ok = false;
                  }
                }
              }
            }
            if (array_index == previous_index) {
              if (preserve_reservoirs) {
                if (array_index != expected_previous_output_index || prior_spatial_output.empty()) {
                  std::fprintf(stderr, "FAIL %s temporal array=%u does not match prior spatial output=%u\n", label,
                               array_index, expected_previous_output_index);
                  ok = false;
                } else if (std::memcmp(&reservoir, &prior_spatial_output[y * 16u + x], sizeof(reservoir)) != 0) {
                  std::fprintf(stderr, "FAIL %s temporal reservoir changed pixel=%u\n", label, pixel);
                  ok = false;
                }
              } else {
                const auto* words = reinterpret_cast<const uint32_t*>(&reservoir);
                for (UINT word = 0; word < sizeof(reservoir) / sizeof(uint32_t); ++word) {
                  if (words[word] != 0xEFEFEFEFu) {
                    std::fprintf(stderr, "FAIL %s previous reservoir array=%u pixel=%u changed word=%u value=%08x\n", label,
                                 array_index, pixel, word, words[word]);
                    ok = false;
                  }
                }
              }
            }
          }
        }
        if (!compatibility_only && expected_light_index >= 0 && !found_selected) {
          std::fprintf(stderr, "FAIL %s missing selected reservoir pixel=%u\n", label, pixel);
          ok = false;
        }
        if (!compatibility_only && expected_status == RTXDI_DI_STATUS_VISIBLE && !expect_zero_diffuse
            && (!std::isfinite(raw[pixel].diffuse_radiance[0]) || !std::isfinite(raw[pixel].diffuse_radiance[1])
                || !std::isfinite(raw[pixel].diffuse_radiance[2]) || raw[pixel].diffuse_radiance[0] <= 0.0f)) {
          std::fprintf(stderr, "FAIL %s nonfinite/empty raw diffuse pixel=%u\n", label, pixel);
          ok = false;
        }
        if (!compatibility_only && expect_zero_diffuse
            && (raw[pixel].diffuse_radiance[0] != 0.0f || raw[pixel].diffuse_radiance[1] != 0.0f
                || raw[pixel].diffuse_radiance[2] != 0.0f)) {
          std::fprintf(stderr, "FAIL %s zero-albedo result was not zero pixel=%u diffuse=%g,%g,%g\n", label, pixel,
                       raw[pixel].diffuse_radiance[0], raw[pixel].diffuse_radiance[1], raw[pixel].diffuse_radiance[2]);
          ok = false;
        }
        if (!compatibility_only && expected_status == RTXDI_DI_STATUS_BLOCKED
            && (raw[pixel].diffuse_radiance[0] != 0.0f || raw[pixel].diffuse_radiance[1] != 0.0f
                || raw[pixel].diffuse_radiance[2] != 0.0f)) {
          std::fprintf(stderr, "FAIL %s blocked sample wrote diffuse pixel=%u\n", label, pixel);
          ok = false;
        }
        if (!compatibility_only && expected_reason == RTXDI_DI_REASON_INVALID_SAMPLE
            && (raw[pixel].diffuse_radiance[0] != 0.0f || raw[pixel].diffuse_radiance[1] != 0.0f
                || raw[pixel].diffuse_radiance[2] != 0.0f)) {
          std::fprintf(stderr, "FAIL %s invalid sample wrote diffuse pixel=%u\n", label, pixel);
          ok = false;
        }
      }
    }
    const auto* guard = reinterpret_cast<const uint32_t*>(&raw[di_pixel_count]);
    for (UINT word = 0; word < sizeof(RtxdiDiRawResult) / sizeof(uint32_t); ++word) {
      if (guard[word] != 0xEFEFEFEFu) {
        std::fprintf(stderr, "FAIL %s raw output guard word=%u value=%08x\n", label, word, guard[word]);
        ok = false;
      }
    }
    prior_spatial_output_index = result_index;
    prior_spatial_output.resize(di_array_pitch);
    std::memcpy(prior_spatial_output.data(), reservoirs + result_index * di_array_pitch,
                di_array_pitch * sizeof(RTXDI_PackedDIReservoir));
    if (check_unwritten_arrays) prior_reservoir_state.assign(reservoirs, reservoirs + di_reservoir_elements);
    di_type_readback->Unmap(0u, nullptr);
    di_reservoir_readback->Unmap(0u, nullptr);
    di_raw_readback->Unmap(0u, nullptr);
    di_resolved_readback->Unmap(0u, nullptr);
    n1_diffuse_readback->Unmap(0u, nullptr);
    n1_specular_readback->Unmap(0u, nullptr);
    n1_status_readback->Unmap(0u, nullptr);
    if (ok && !suppress_di_pass) std::printf("PASS %s\n", label);
    return ok;
  };

  di_frame.parameters.di.initialSamplingParams.numLocalLightSamples = 8u;
  di_frame.parameters.di.initialSamplingParams.localLightSamplingMode = ReSTIRDI_LocalLightSamplingMode::Uniform;
  if (!run_di(4u, 4u, true, RTXDI_DI_STATUS_VISIBLE, RTXDI_DI_REASON_NONE, 0, "RTXDI point sample, empty-scene visible, odd 3x2 guard", true)) return 1;
  lights[0].translucency = 1.0f;
  if (!update_upload(light_upload.Get(), lights.data(), sizeof(lights))
      || !run_di(4u, 1u, true, RTXDI_DI_STATUS_VISIBLE, RTXDI_DI_REASON_NONE, 0,
                 "RTXDI N1 visible environment output", true, 2u, true)) return 1;
  const std::array<uint32_t, 3> mrt1_specular_scales = {0u, 128u, 255u};
  const char* mrt1_scale_labels[] = {"RTXDI N1 MRT1.z 0", "RTXDI N1 MRT1.z 128", "RTXDI N1 MRT1.z 255"};
  for (UINT scale_index = 0u; scale_index < mrt1_specular_scales.size(); ++scale_index) {
    for (UINT pixel = 0u; pixel < 6u; ++pixel) mrt1_pixels[pixel * 4u + 2u] = mrt1_specular_scales[scale_index];
    if (!update_pixel_texture(mrt1_tex.Get(), mrt1_upload.Get(), mrt1_pixels.data(), 3u * 4u * sizeof(uint32_t))
        || !run_di(4u, 1u, true, RTXDI_DI_STATUS_VISIBLE, RTXDI_DI_REASON_NONE, 0, mrt1_scale_labels[scale_index],
                   true, 2u, true)) return 1;
  }
  const float specular_scale_ratio = HalfToFloat(n1_scale_specular[1]) / HalfToFloat(n1_scale_specular[2]);
  if (n1_scale_specular[0] != 0u || n1_scale_specular[1] == 0u
      || n1_scale_specular[2] <= n1_scale_specular[1] || n1_scale_diffuse[0] != n1_scale_diffuse[1]
      || n1_scale_diffuse[1] != n1_scale_diffuse[2]
      || n1_scale_specular_hit_t[0] == 0u || n1_scale_specular_hit_t[1] == 0u
      || n1_scale_specular_hit_t[2] == 0u
      || !std::isfinite(specular_scale_ratio)
      || std::abs(specular_scale_ratio - (128.0f / 255.0f)) > 0.005f) {
    std::fprintf(stderr, "FAIL RTXDI N1 MRT1.z scale diffuse=%u,%u,%u specular=%u,%u,%u hitT=%u,%u,%u ratio=%g\n",
                 n1_scale_diffuse[0], n1_scale_diffuse[1], n1_scale_diffuse[2], n1_scale_specular[0],
                 n1_scale_specular[1], n1_scale_specular[2], n1_scale_specular_hit_t[0],
                 n1_scale_specular_hit_t[1], n1_scale_specular_hit_t[2], specular_scale_ratio);
    return 1;
  }
  std::printf("PASS RTXDI N1 MRT1.z scales specular 0/128/255 with invariant diffuse ratio=%g\n",
              specular_scale_ratio);
  for (UINT pixel = 0u; pixel < 6u; ++pixel) mrt1_pixels[pixel * 4u + 2u] = 255u;
  if (!update_pixel_texture(mrt1_tex.Get(), mrt1_upload.Get(), mrt1_pixels.data(), 3u * 4u * sizeof(uint32_t))) return 1;

  const auto original_formula_light = lights[0];
  const auto original_formula_deferred = deferred[0];
  const auto original_formula_tile = tile_records[0];
  lights[0].color[0] = 2.0f;
  lights[0].color[1] = 3.0f;
  lights[0].color[2] = 4.0f;
  lights[0].radius_inv = 0.5f;
  lights[0].attenuation = 2.0f;
  lights[0].attenuation_angle = 1.0f;
  lights[0].spot_angle_inv = 1.0f;
  lights[0].vec[0] = 0.6f;
  lights[0].vec[1] = 0.0f;
  lights[0].vec[2] = 0.8f;
  lights[0].pos[0] = surface.position[0] + 0.6f;
  lights[0].pos[1] = surface.position[1];
  lights[0].pos[2] = surface.position[2] + 0.8f;
  lights[0].translucency = 0.9f;
  lights[0].specular_intensity = 1.5f;
  lights[0].specular_glossiness = 16.0f;
  const std::array<float, 3> colored_f0 = {0.02f, 0.06f, 0.1f};
  std::memcpy(deferred[0].bytes + 16u, colored_f0.data(), sizeof(colored_f0));
  tile_records[0].point_count = 1u;
  tile_records[0].point_indices[0] = 0;
  tile_records[0].spot_count = 0u;
  if (!update_upload(light_upload.Get(), lights.data(), sizeof(lights)) || !upload_deferred_pair()
      || !update_upload(tile_upload.Get(), tile_records.data(), tile_records.size() * sizeof(RtxdiTileData))) return 1;
  for (UINT pixel = 0u; pixel < 6u; ++pixel) mrt1_pixels[pixel * 4u + 2u] = 128u;
  if (!update_pixel_texture(mrt1_tex.Get(), mrt1_upload.Get(), mrt1_pixels.data(), 3u * 4u * sizeof(uint32_t))) return 1;
  n1_test_config = {0.0f, 1.0f, 0.0f, 0.0f, 4.0f, 0.04f, 1.0f};
  float formula_inverse_pdf = 0.0f;
  if (!run_di(4u, 1u, true, RTXDI_DI_STATUS_VISIBLE, RTXDI_DI_REASON_NONE, 0,
              "RTXDI N1 formula capture", false, 1u, true, 0u, false, false, false, false, true, false,
              nullptr, nullptr, nullptr, nullptr, &formula_inverse_pdf)) return 1;
  const auto check_n1_formula = [&](bool spot_light, const char* case_name) {
    const float dx = lights[0].pos[0] - surface.position[0];
    const float dy = lights[0].pos[1] - surface.position[1];
    const float dz = lights[0].pos[2] - surface.position[2];
    const float distance = std::sqrt(dx * dx + dy * dy + dz * dz);
    if (!std::isfinite(distance) || distance <= 0.0f) return false;
    const std::array<float, 3> direction = {dx / distance, dy / distance, dz / distance};
    const float ndotl_raw = direction[2];  // Current N1 input MRT0 encodes +Z for this fixture.
    const float ndotl = (std::clamp)(ndotl_raw, 0.0f, 1.0f);
    const float lit = (std::max)((std::max)(0.0f, ndotl_raw), lights[0].translucency);
    const float attenuation = (std::max)(0.0f, 1.0f - std::pow(std::abs(distance * lights[0].radius_inv), lights[0].attenuation));
    const float spot_dot = direction[0] * lights[0].vec[0] + direction[1] * lights[0].vec[1]
                           + direction[2] * lights[0].vec[2];
    const float spot = spot_light
                           ? (std::max)(0.0f, 1.0f - std::pow((1.0f - (std::max)(0.0f, spot_dot)) * lights[0].spot_angle_inv, lights[0].attenuation_angle))
                           : 1.0f;
    const uint32_t packed_gloss = mrt2_pixels[1u];
    const float gloss_x = static_cast<float>(packed_gloss & 31u) / 31.0f;
    const float gloss_y = static_cast<float>((packed_gloss >> 5u) & 31u) / 31.0f;
    float material_gloss[3] = {};
    std::memcpy(material_gloss, deferred[0].bytes + 64u, sizeof(material_gloss));
    const float gloss = (std::lerp)((std::lerp)(material_gloss[0], material_gloss[1], gloss_x),
                                    material_gloss[2], gloss_y);
    const float exponent = (std::max)(0.001f, lights[0].specular_glossiness * gloss);
    const float hx = surface.view[0] + direction[0];
    const float hy = surface.view[1] + direction[1];
    const float hz = surface.view[2] + direction[2];
    const float half_length = std::sqrt(hx * hx + hy * hy + hz * hz);
    const float ndoth = half_length > 1e-6f
                            ? (std::max)(0.0f, hz / half_length)  // Current N1 input MRT0 encodes +Z.
                            : 0.0f;
    const float blinn = std::pow(ndoth, exponent);
    const float expected_diffuse_scale = attenuation * spot * lit * formula_inverse_pdf;
    const float expected_specular_scale = lights[0].specular_intensity * (attenuation * spot * lit * blinn)
                                          * (128.0f / 255.0f) * formula_inverse_pdf;
    const float actual_diffuse[3] = {HalfToFloat(n1_formula_diffuse[0]), HalfToFloat(n1_formula_diffuse[1]),
                                     HalfToFloat(n1_formula_diffuse[2])};
    const float actual_specular[3] = {HalfToFloat(n1_formula_specular[0]), HalfToFloat(n1_formula_specular[1]),
                                      HalfToFloat(n1_formula_specular[2])};
    for (UINT channel = 0u; channel < 3u; ++channel) {
      const float expected_diffuse = lights[0].color[channel] * expected_diffuse_scale;
      const float expected_specular = lights[0].color[channel] * expected_specular_scale;
      if (std::abs(actual_diffuse[channel] - expected_diffuse) > (std::max)(0.002f, expected_diffuse * 0.003f)
          || std::abs(actual_specular[channel] - expected_specular) > (std::max)(0.002f, expected_specular * 0.01f)) {
        std::fprintf(stderr,
                     "FAIL %s channel=%u diffuse=%g expected=%g specular=%g expected=%g A=%g spot=%g lit=%g n=%g invPdf=%g\n",
                     case_name, channel, actual_diffuse[channel], expected_diffuse, actual_specular[channel],
                     expected_specular, attenuation, spot, lit, ndotl, formula_inverse_pdf);
        return false;
      }
    }
    return attenuation > 0.0f && (!spot_light || (spot > 0.0f && spot < 1.0f)) && lights[0].translucency > ndotl;
  };
  if (!check_n1_formula(false, "RTXDI N1 point game formula")) return 1;
  std::printf("PASS RTXDI N1 point game formula A!=1, translucency>NdotL, colored lamp/F0, inverse PDF, MRT1 scale\n");

  lights[0].pos[0] = surface.position[0] + 1.0f;
  lights[0].pos[2] = surface.position[2];
  lights[0].translucency = 0.4f;
  if (!update_upload(light_upload.Get(), lights.data(), sizeof(lights))) return 1;
  formula_inverse_pdf = 0.0f;
  if (!run_di(4u, 1u, true, RTXDI_DI_STATUS_VISIBLE, RTXDI_DI_REASON_NONE, 0,
              "RTXDI N1 formula capture", false, 1u, true, 0u, false, false, false, false, true, false,
              nullptr, nullptr, nullptr, nullptr, &formula_inverse_pdf)
      || !check_n1_formula(false, "RTXDI N1 zero NdotL translucent game formula")) return 1;
  std::printf("PASS RTXDI N1 NdotL=0 with positive translucency follows game terms\n");

  lights[0].pos[0] = surface.position[0] + 0.6f;
  lights[0].pos[2] = surface.position[2] + 0.8f;
  lights[0].translucency = 0.9f;
  lights[0].vec[0] = 0.8f;
  lights[0].vec[2] = 0.6f;
  tile_records[0].point_count = 0u;
  tile_records[0].spot_count = 1u;
  tile_records[0].spot_indices[0] = 0;
  if (!update_upload(light_upload.Get(), lights.data(), sizeof(lights))
      || !update_upload(tile_upload.Get(), tile_records.data(), tile_records.size() * sizeof(RtxdiTileData))) return 1;
  n1_test_config[3] = 0.0f;
  formula_inverse_pdf = 0.0f;
  if (!run_di(4u, 1u, true, RTXDI_DI_STATUS_VISIBLE, RTXDI_DI_REASON_NONE, 0,
              "RTXDI N1 formula capture", false, 1u, true, 0u, false, false, false, false, true, false,
              nullptr, nullptr, nullptr, nullptr, &formula_inverse_pdf)
      || !check_n1_formula(true, "RTXDI N1 spot game formula")) return 1;
  std::printf("PASS RTXDI N1 spot game formula cone!=1, attenuation!=1, translucency>NdotL\n");

  tile_records[0].point_count = 1u;
  tile_records[0].spot_count = 0u;
  if (!update_upload(light_upload.Get(), lights.data(), sizeof(lights))
      || !update_upload(tile_upload.Get(), tile_records.data(), tile_records.size() * sizeof(RtxdiTileData))) return 1;
  n1_test_config[2] = 1.0f;
  n1_test_config[3] = 0.0f;
  formula_inverse_pdf = 0.0f;
  if (!run_di(4u, 1u, true, RTXDI_DI_STATUS_VISIBLE, RTXDI_DI_REASON_NONE, 0,
              "RTXDI N1 formula capture", false, 1u, true, 0u, false, false, false, false, true, false,
              nullptr, nullptr, nullptr, nullptr, &formula_inverse_pdf)
      || !check_n1_formula(false, "RTXDI N1 GGX strength zero game formula")) return 1;
  const std::array<uint16_t, 4> strength_zero = n1_formula_specular;
  n1_test_config[3] = 0.5f;
  if (!run_di(4u, 1u, true, RTXDI_DI_STATUS_VISIBLE, RTXDI_DI_REASON_NONE, 0,
              "RTXDI N1 formula capture", false, 1u, true, 0u, false, false, false, false, true, false,
              nullptr, nullptr, nullptr, nullptr, &formula_inverse_pdf)) return 1;
  const std::array<uint16_t, 4> strength_half = n1_formula_specular;
  n1_test_config[3] = 1.0f;
  if (!run_di(4u, 1u, true, RTXDI_DI_STATUS_VISIBLE, RTXDI_DI_REASON_NONE, 0,
              "RTXDI N1 formula capture", false, 1u, true, 0u, false, false, false, false, true, false,
              nullptr, nullptr, nullptr, nullptr, &formula_inverse_pdf)) return 1;
  const std::array<uint16_t, 4> strength_one = n1_formula_specular;
  for (UINT channel = 0u; channel < 3u; ++channel) {
    const float zero = HalfToFloat(strength_zero[channel]);
    const float half = HalfToFloat(strength_half[channel]);
    const float one = HalfToFloat(strength_one[channel]);
    if (!std::isfinite(zero) || !std::isfinite(half) || !std::isfinite(one)
        || std::abs(half - 0.5f * (zero + one)) > (std::max)(0.002f, one * 0.01f)) {
      std::fprintf(stderr, "FAIL RTXDI N1 GGX strength channel=%u zero=%g half=%g one=%g\n", channel, zero, half,
                   one);
      return 1;
    }
  }
  const float ggx_red = HalfToFloat(strength_one[0]) / lights[0].color[0];
  const float ggx_green = HalfToFloat(strength_one[1]) / lights[0].color[1];
  const float ggx_blue = HalfToFloat(strength_one[2]) / lights[0].color[2];
  if (!(ggx_green > ggx_red * 1.4f && ggx_blue > ggx_green * 1.4f)) {
    std::fprintf(stderr, "FAIL RTXDI N1 colored F0 collapsed specular channels %g,%g,%g\n", ggx_red, ggx_green,
                 ggx_blue);
    return 1;
  }
  std::printf("PASS RTXDI N1 specular strength 0/0.5/1 blends full colored GGX result F0=%g,%g,%g\n", ggx_red,
              ggx_green, ggx_blue);
  n1_test_config = {0.0f, 1.0f, 0.0f, 1.0f, 4.0f, 0.04f, 1.0f};
  lights[0] = original_formula_light;
  deferred[0] = original_formula_deferred;
  tile_records[0] = original_formula_tile;
  if (!update_upload(light_upload.Get(), lights.data(), sizeof(lights)) || !upload_deferred_pair()
      || !update_upload(tile_upload.Get(), tile_records.data(), tile_records.size() * sizeof(RtxdiTileData))) return 1;
  for (UINT pixel = 0u; pixel < 6u; ++pixel) mrt1_pixels[pixel * 4u + 2u] = 255u;
  if (!update_pixel_texture(mrt1_tex.Get(), mrt1_upload.Get(), mrt1_pixels.data(), 3u * 4u * sizeof(uint32_t))) return 1;
  lights[0].translucency = 0.0f;
  if (!update_upload(light_upload.Get(), lights.data(), sizeof(lights))) return 1;
  const auto original_albedo_pixels = albedo_pixels;
  for (size_t pixel = 0; pixel < 6u; ++pixel) {
    albedo_pixels[pixel * 4u + 0u] = 1.0f;
    albedo_pixels[pixel * 4u + 1u] = 1.0f;
    albedo_pixels[pixel * 4u + 2u] = 1.0f;
  }
  const float material_intensity = 1.0f;
  std::memcpy(deferred[0].bytes + 76, &material_intensity, sizeof(material_intensity));
  if (!upload_deferred_pair()
      || !update_pixel_texture(albedo_tex.Get(), albedo_upload.Get(), albedo_pixels.data(), 3u * 4u * sizeof(float))) return 1;
  float white_target = 0.0f;
  float white_raw = 0.0f;
  float white_inverse_pdf = 0.0f;
  if (!run_di(4u, 1u, true, RTXDI_DI_STATUS_VISIBLE, RTXDI_DI_REASON_NONE, 0,
              "RTXDI one-light pre-material estimate matches game diffuse", false, 1u, false, 0u,
              false, false, false, false, true, false, &white_target, nullptr, nullptr, &white_raw,
              &white_inverse_pdf)) return 1;
  const std::array<float, 3> surface_to_light = {lights[0].pos[0] - surface.position[0],
                                                 lights[0].pos[1] - surface.position[1],
                                                 lights[0].pos[2] - surface.position[2]};
  const float surface_light_distance = std::sqrt(surface_to_light[0] * surface_to_light[0]
                                                 + surface_to_light[1] * surface_to_light[1]
                                                 + surface_to_light[2] * surface_to_light[2]);
  const float one_light_attenuation = (std::max)(0.0f, 1.0f - std::pow(std::abs(surface_light_distance * lights[0].radius_inv), lights[0].attenuation));
  const float one_light_lit = (std::max)(0.0f, surface_to_light[2] / surface_light_distance);
  const float game_one_light_diffuse = lights[0].color[0] * one_light_attenuation * one_light_lit;
  if (!std::isfinite(white_target) || white_target <= 0.0f || !std::isfinite(white_inverse_pdf)
      || std::abs(white_raw - game_one_light_diffuse * white_inverse_pdf)
             > (std::max)(1e-5f, game_one_light_diffuse * white_inverse_pdf * 1e-4f)) {
    std::fprintf(stderr, "FAIL one-light pre-material result raw=%g expected=%g invPdf=%g\n", white_raw,
                 game_one_light_diffuse * white_inverse_pdf, white_inverse_pdf);
    return 1;
  }
  albedo_pixels.fill(0.0f);
  const float zero_material_intensity = 0.0f;
  std::memcpy(deferred[0].bytes + 76, &zero_material_intensity, sizeof(zero_material_intensity));
  if (!upload_deferred_pair()
      || !update_pixel_texture(albedo_tex.Get(), albedo_upload.Get(), albedo_pixels.data(), 3u * 4u * sizeof(float))) return 1;
  float black_target = 0.0f;
  float black_raw = 0.0f;
  float black_inverse_pdf = 0.0f;
  if (!run_di(4u, 1u, true, RTXDI_DI_STATUS_VISIBLE, RTXDI_DI_REASON_NONE, 0,
              "RTXDI black base and zero dynamic intensity preserve pre-material estimates", false, 1u,
              false, 0u, false, false, false, false, true, false, &black_target, nullptr, nullptr,
              &black_raw, &black_inverse_pdf)
      || std::abs(black_target - white_target) > (std::max)(1e-5f, white_target * 1e-5f)
      || std::abs(black_raw - white_raw) > (std::max)(1e-5f, white_raw * 1e-5f)
      || std::abs(black_inverse_pdf - white_inverse_pdf) > 1e-5f) {
    std::fprintf(stderr, "FAIL material-independent pre-material target/raw white=%g/%g black=%g/%g\n",
                 white_target, white_raw, black_target, black_raw);
    return 1;
  }
  for (size_t pixel = 0; pixel < 6u; ++pixel) {
    albedo_pixels[pixel * 4u + 0u] = 0.25f;
    albedo_pixels[pixel * 4u + 1u] = 0.25f;
    albedo_pixels[pixel * 4u + 2u] = 0.25f;
  }
  const float fractional_material_intensity = 0.4f;
  std::memcpy(deferred[0].bytes + 76, &fractional_material_intensity, sizeof(fractional_material_intensity));
  if (!upload_deferred_pair()
      || !update_pixel_texture(albedo_tex.Get(), albedo_upload.Get(), albedo_pixels.data(), 3u * 4u * sizeof(float))) return 1;
  float fractional_target = 0.0f;
  float fractional_raw = 0.0f;
  if (!run_di(4u, 1u, true, RTXDI_DI_STATUS_VISIBLE, RTXDI_DI_REASON_NONE, 0,
              "RTXDI one material and intensity application equals game diffuse", false, 1u, false, 0u,
              false, false, false, false, true, false, &fractional_target, nullptr, nullptr, &fractional_raw)) return 1;
  const float game_post_material_diffuse = game_one_light_diffuse * white_inverse_pdf * 0.25f * 0.4f;
  if (std::abs(fractional_raw * 0.25f * 0.4f - game_post_material_diffuse)
      > (std::max)(1e-5f, game_post_material_diffuse * 1e-4f)) {
    std::fprintf(stderr, "FAIL one-light post-material result=%g expected=%g\n", fractional_raw * 0.25f * 0.4f,
                 game_post_material_diffuse);
    return 1;
  }
  albedo_pixels = original_albedo_pixels;
  std::memcpy(deferred[0].bytes + 76, &material_intensity, sizeof(material_intensity));
  if (!upload_deferred_pair()
      || !update_pixel_texture(albedo_tex.Get(), albedo_upload.Get(), albedo_pixels.data(), 3u * 4u * sizeof(float))) return 1;
  albedo_pixels.fill(0.0f);
  wait_context = "upload zero diffuse albedo";
  float zero_albedo_specular_target = 0.0f;
  if (!update_pixel_texture(albedo_tex.Get(), albedo_upload.Get(), albedo_pixels.data(), 3 * 4 * sizeof(float))
      || !run_di(4u, 4u, true, RTXDI_DI_STATUS_VISIBLE, RTXDI_DI_REASON_NONE, 0,
                 "RTXDI zero-albedo proposal retains pre-material raw diffuse", false, 1u, false, 0u,
                 false, false, false, false, true, false, &zero_albedo_specular_target)
      || zero_albedo_specular_target <= 0.0f) return 1;
  uint32_t disabled_specular = 0u;
  std::memcpy(deferred[0].bytes + 136, &disabled_specular, sizeof(disabled_specular));
  float disabled_specular_target = 0.0f;
  if (!upload_deferred_pair()
      || !run_di(4u, 4u, true, RTXDI_DI_STATUS_VISIBLE, RTXDI_DI_REASON_NONE, 0,
                 "RTXDI zero-albedo flag-off keeps pre-material diffuse proposal", false, 1u, false, 0u,
                 false, false, false, false, true, false, &disabled_specular_target)
      || disabled_specular_target >= zero_albedo_specular_target) {
    std::fprintf(stderr, "FAIL local specular flag did not increase proposal target off=%g on=%g\n",
                 disabled_specular_target, zero_albedo_specular_target);
    return 1;
  }
  const float invalid_material_gloss = std::bit_cast<float>(0x7fc00000u);
  std::memcpy(deferred[0].bytes + 64, &invalid_material_gloss, sizeof(invalid_material_gloss));
  if (!upload_deferred_pair()
      || !run_di(4u, 4u, true, RTXDI_DI_STATUS_EMPTY, RTXDI_DI_REASON_INVALID_SURFACE, -1,
                 "RTXDI nonfinite material gloss invalidates surface metadata", false)) return 1;
  const std::array<float, 3> material_gloss = {0.0f, 0.5f, 1.0f};
  std::memcpy(deferred[0].bytes + 64, material_gloss.data(), sizeof(material_gloss));
  std::memcpy(deferred[0].bytes + 136, &local_specular_flag, sizeof(local_specular_flag));
  std::memcpy(previous_deferred[0].bytes + 64, material_gloss.data(), sizeof(material_gloss));
  std::memcpy(previous_deferred[0].bytes + 136, &local_specular_flag, sizeof(local_specular_flag));
  float gloss_endpoint_target = 0.0f;
  float gloss_middle_target = 0.0f;
  float gloss_high_target = 0.0f;
  mrt2_pixels[1] = 0u;
  if (!upload_deferred_pair() || !upload_current_mrt2()
      || !run_di(4u, 4u, true, RTXDI_DI_STATUS_VISIBLE, RTXDI_DI_REASON_NONE, 0,
                 "RTXDI zero gloss selector endpoint uses exponent clamp", false, 1u, false, 0u,
                 false, false, false, false, true, false, &gloss_endpoint_target)) return 1;
  mrt2_pixels[1] = 31u;
  if (!upload_current_mrt2()
      || !run_di(4u, 4u, true, RTXDI_DI_STATUS_VISIBLE, RTXDI_DI_REASON_NONE, 0,
                 "RTXDI gloss selector midpoint interpolates material", false, 1u, false, 0u,
                 false, false, false, false, true, false, &gloss_middle_target)) return 1;
  mrt2_pixels[1] = 1023u;
  if (!upload_current_mrt2()
      || !run_di(4u, 4u, true, RTXDI_DI_STATUS_VISIBLE, RTXDI_DI_REASON_NONE, 0,
                 "RTXDI gloss selector high endpoint uses material Z", false, 1u, false, 0u,
                 false, false, false, false, true, false, &gloss_high_target)
      || !(gloss_endpoint_target > gloss_middle_target && gloss_middle_target > gloss_high_target)) {
    std::fprintf(stderr, "FAIL RTXDI material gloss target order %g, %g, %g\n", gloss_endpoint_target,
                 gloss_middle_target, gloss_high_target);
    return 1;
  }
  const std::array<float, 3> negative_material_gloss = {-1.0f, -0.5f, -0.25f};
  std::memcpy(deferred[0].bytes + 64, negative_material_gloss.data(), sizeof(negative_material_gloss));
  if (!upload_deferred_pair()
      || !run_di(4u, 4u, true, RTXDI_DI_STATUS_VISIBLE, RTXDI_DI_REASON_NONE, 0,
                 "RTXDI finite negative material gloss uses game exponent clamp", false, 1u, false, 0u,
                 false, false, false, false, true, false)) return 1;
  for (size_t i = 0; i < 6; ++i) {
    albedo_pixels[i * 4 + 0] = 0.5f;
    albedo_pixels[i * 4 + 1] = 0.4f;
    albedo_pixels[i * 4 + 2] = 0.3f;
    albedo_pixels[i * 4 + 3] = 1.0f;
  }
  wait_context = "restore diffuse albedo";
  std::memcpy(deferred[0].bytes + 64, material_gloss.data(), sizeof(material_gloss));
  if (!upload_deferred_pair()
      || !update_pixel_texture(albedo_tex.Get(), albedo_upload.Get(), albedo_pixels.data(), 3 * 4 * sizeof(float))) return 1;
  for (UINT i = 0; i < 6u; ++i) normal_pixels[i * 4u + 1u] = 0u;
  if (!update_pixel_texture(normal_tex.Get(), normal_upload.Get(), normal_pixels.data(), 3u * 16u)
      || !run_di(4u, 4u, true, RTXDI_DI_STATUS_EMPTY, RTXDI_DI_REASON_INVALID_SAMPLE, -1,
                 "RTXDI back-facing zero-translucency surface has no proposal", false, 1u, false, 0u,
                 false, false, false, false, true, true)) return 1;
  lights[0].translucency = 0.5f;
  float translucent_backface_raw = 0.0f;
  float translucent_backface_inverse_pdf = 0.0f;
  if (!update_upload(light_upload.Get(), lights.data(), sizeof(lights))
      || !run_di(4u, 1u, true, RTXDI_DI_STATUS_VISIBLE, RTXDI_DI_REASON_NONE, 0,
                 "RTXDI back-facing translucent surface receives pre-material diffuse", false, 1u, false, 0u,
                 false, false, false, false, true, false, nullptr, nullptr, nullptr,
                 &translucent_backface_raw, &translucent_backface_inverse_pdf)) return 1;
  const float translucent_expected = lights[0].color[0] * one_light_attenuation * 0.5f
                                     * translucent_backface_inverse_pdf;
  if (!std::isfinite(translucent_backface_raw) || translucent_backface_raw <= 0.0f
      || std::abs(translucent_backface_raw - translucent_expected)
             > (std::max)(1e-5f, translucent_expected * 1e-4f)) {
    std::fprintf(stderr, "FAIL translucent backface raw=%g expected=%g\n", translucent_backface_raw,
                 translucent_expected);
    return 1;
  }
  lights[0].translucency = 0.0f;
  for (UINT i = 0; i < 6u; ++i) normal_pixels[i * 4u + 1u] = 65535u;
  if (!update_upload(light_upload.Get(), lights.data(), sizeof(lights))
      || !update_pixel_texture(normal_tex.Get(), normal_upload.Get(), normal_pixels.data(), 3u * 16u)) return 1;
  const auto original_lights = lights;
  lights[0].color[0] = 2.0f;
  lights[1].color[0] = 5.0f;
  lights[0].specular_intensity = 0.0f;
  lights[1].specular_intensity = 0.0f;
  if (!update_upload(light_upload.Get(), lights.data(), sizeof(lights))) return 1;
  constexpr UINT estimator_frames = 128u;
  float pre_material_sum = 0.0f;
  suppress_di_pass = true;
  for (UINT frame = 0u; frame < estimator_frames; ++frame) {
    di_frame.parameters.runtime.frameIndex = frame + 101u;
    float sample = 0.0f;
    if (!run_di(4u, 2u, true, RTXDI_DI_STATUS_VISIBLE, RTXDI_DI_REASON_NONE, -1,
                "RTXDI two-light pre-material estimator sample", false, 1u, false, 0u, true,
                false, false, false, true, false, nullptr, nullptr, nullptr, &sample)) return 1;
    if (!std::isfinite(sample)) {
      std::fprintf(stderr, "FAIL two-light estimator produced nonfinite sample at frame=%u\n", frame);
      return 1;
    }
    pre_material_sum += sample;
  }
  suppress_di_pass = false;
  const float average_two_light_estimate = pre_material_sum / estimator_frames;
  const float game_two_light_sum = 7.0f * one_light_attenuation * one_light_lit;
  if (std::abs(average_two_light_estimate - game_two_light_sum) > game_two_light_sum * 0.12f) {
    std::fprintf(stderr, "FAIL two-light estimator avg=%g game sum=%g frames=%u\n", average_two_light_estimate,
                 game_two_light_sum, estimator_frames);
    return 1;
  }
  std::printf("PASS RTXDI two-light pre-material estimator avg=%g game sum=%g frames=%u\n",
              average_two_light_estimate, game_two_light_sum, estimator_frames);
  lights = original_lights;
  di_frame.parameters.runtime.frameIndex = 19u;
  if (!update_upload(light_upload.Get(), lights.data(), sizeof(lights))) return 1;
  lights[0].pos[0] = 3.0f;
  lights[0].radius = 20.0f;
  lights[0].radius_inv = 0.05f;
  lights[0].specular_intensity = 1e6f;
  lights[0].specular_glossiness = 1000.0f;
  if (!update_upload(light_upload.Get(), lights.data(), sizeof(lights))) return 1;
  float grazing_target = 0.0f;
  float grazing_inverse_pdf = 0.0f;
  if (!run_di(4u, 1u, false, RTXDI_DI_STATUS_UNKNOWN, RTXDI_DI_REASON_TRUNCATED, 0,
              "RTXDI grazing GGX proposal keeps finite target and inverse PDF", false, 1u, false, 0u,
              false, false, false, false, true, true, &grazing_target, nullptr, nullptr, nullptr,
              &grazing_inverse_pdf)
      || !std::isfinite(grazing_target) || grazing_target <= 0.0f || !std::isfinite(grazing_inverse_pdf)
      || grazing_inverse_pdf <= 0.0f) {
    std::fprintf(stderr, "FAIL grazing proposal target=%g inversePdf=%g\n", grazing_target, grazing_inverse_pdf);
    return 1;
  }
  lights = original_lights;
  if (!update_upload(light_upload.Get(), lights.data(), sizeof(lights))) return 1;
  if (!run_di(4u, 4u, true, RTXDI_DI_STATUS_VISIBLE, RTXDI_DI_REASON_NONE, 0,
              "RTXDI spatial point sample, SDK-selected output", true, 1u, true, 2u)) return 1;
  for (UINT frame_index = 1u; frame_index <= 2u; ++frame_index) {
    bridge_di_context->SetFrameIndex(frame_index);
    di_frame.parameters = falcom_world::rtxdi::MakeDiFrameParameters(bridge_di_config, *bridge_di_context);
    di_frame.parameters.width = 3u;
    di_frame.parameters.height = 2u;
    std::printf("RTXDI spatial indices frame=%u initial=%u input=%u output=%u previous=%u saved=%u\n", frame_index,
                di_frame.parameters.di.bufferIndices.initialSamplingOutputBufferIndex,
                di_frame.parameters.di.bufferIndices.spatialResamplingInputBufferIndex,
                di_frame.parameters.di.bufferIndices.spatialResamplingOutputBufferIndex,
                di_frame.parameters.di.bufferIndices.temporalResamplingInputBufferIndex,
                prior_spatial_output_index);
    if (di_frame.parameters.di.bufferIndices.temporalResamplingInputBufferIndex != prior_spatial_output_index
        || !run_di(4u, 4u, true, RTXDI_DI_STATUS_VISIBLE, RTXDI_DI_REASON_NONE, 0,
                   frame_index == 1u ? "RTXDI temporal+spatial frame 1 reuses prior output"
                                     : "RTXDI temporal+spatial frame 2 rotates reservoirs",
                   true, 1u, true, 0u, false, true, true, true)) return 1;
  }
  UINT accepted_temporal_frame = 2u;
  const auto prepare_temporal_frame = [&]() {
    bridge_di_context->SetFrameIndex(++accepted_temporal_frame);
    di_frame.parameters = falcom_world::rtxdi::MakeDiFrameParameters(bridge_di_config, *bridge_di_context);
    di_frame.parameters.width = 3u;
    di_frame.parameters.height = 2u;
  };
  prepare_temporal_frame();
  if (!run_di(4u, 4u, true, RTXDI_DI_STATUS_VISIBLE, RTXDI_DI_REASON_NONE, 0,
              "RTXDI temporal no-history fallback preserves current sample", true, 1u, true, 0u,
              false, true, true, false, false)) return 1;

  motion_pixels[0] = 100.0f;
  wait_context = "upload out-of-view temporal motion";
  if (!update_pixel_texture(motion_tex.Get(), motion_upload.Get(), motion_pixels.data(), 3u * 4u * sizeof(float))) return 1;
  prepare_temporal_frame();
  if (!run_di(4u, 4u, true, RTXDI_DI_STATUS_VISIBLE, RTXDI_DI_REASON_NONE, 0,
              "RTXDI temporal large motion rejects previous pixel", true, 1u, true, 0u,
              false, true, true, false)) return 1;
  motion_pixels[0] = 0.0f;
  wait_context = "restore temporal motion";
  if (!update_pixel_texture(motion_tex.Get(), motion_upload.Get(), motion_pixels.data(), 3u * 4u * sizeof(float))) return 1;

  const auto seed_temporal_history_light = [&](UINT light_index) {
    if (prior_spatial_output.empty() || prior_spatial_output_index >= 3u) return false;
    auto& reservoir = prior_spatial_output[0];
    reservoir.lightData = 0x80000000u | light_index;
    reservoir.weight = 100.0f;
    reservoir.targetPdf = 1e-5f;
    reservoir.mVisibility = 20u << 18u;
    reservoir.distanceAge = 0u;
    std::vector<RTXDI_PackedDIReservoir> seeded_reservoirs(di_reservoir_elements);
    std::memset(seeded_reservoirs.data(), 0xEF, static_cast<size_t>(di_reservoir_bytes));
    std::memcpy(seeded_reservoirs.data() + prior_spatial_output_index * di_array_pitch,
                prior_spatial_output.data(), prior_spatial_output.size() * sizeof(RTXDI_PackedDIReservoir));
    if (!update_upload(di_reservoir_upload.Get(), seeded_reservoirs.data(), static_cast<size_t>(di_reservoir_bytes))) return false;
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = di_reservoirs.Get();
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
    list->ResourceBarrier(1u, &barrier);
    list->CopyBufferRegion(di_reservoirs.Get(), 0u, di_reservoir_upload.Get(), 0u, di_reservoir_bytes);
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COMMON;
    list->ResourceBarrier(1u, &barrier);
    wait_context = "seed previous DI light identity";
    return submit_wait() && Check(allocator->Reset(), "reset temporal identity allocator")
           && Check(base_list->Reset(allocator.Get(), nullptr), "reset temporal identity list");
  };

  std::array<float, 3 * 2 * 4> temporal_albedo = albedo_pixels;
  const auto saved_spatial_output = prior_spatial_output;
  const UINT saved_spatial_output_index = prior_spatial_output_index;
  temporal_albedo.fill(0.0f);
  if (!update_pixel_texture(albedo_tex.Get(), albedo_upload.Get(), temporal_albedo.data(), 3u * 4u * sizeof(float))
      || !update_pixel_texture(prev_albedo_tex.Get(), prev_albedo_upload.Get(), temporal_albedo.data(),
                               3u * 4u * sizeof(float))) return 1;
  const std::array<float, 3> previous_low_gloss = {0.0f, 0.0f, 0.0f};
  uint32_t previous_no_specular = 0u;
  std::memcpy(previous_deferred[0].bytes + 64, previous_low_gloss.data(), sizeof(previous_low_gloss));
  std::memcpy(previous_deferred[0].bytes + 136, &previous_no_specular, sizeof(previous_no_specular));
  if (!upload_deferred_pair()) return 1;
  UINT temporal_m_without_previous_specular = 0u;
  float temporal_weight_without_previous_specular = 0.0f;
  prepare_temporal_frame();
  if (!seed_temporal_history_light(0u)
      || !run_di(4u, 4u, true, RTXDI_DI_STATUS_VISIBLE, RTXDI_DI_REASON_NONE, 0,
                 "RTXDI previous flag and gloss reject zero-albedo specular history", true, 1u, false, 0u,
                 false, true, true, false, true, false, nullptr, &temporal_m_without_previous_specular,
                 &temporal_weight_without_previous_specular)) return 1;
  std::memcpy(previous_deferred[0].bytes + 64, material_gloss.data(), sizeof(material_gloss));
  std::memcpy(previous_deferred[0].bytes + 136, &local_specular_flag, sizeof(local_specular_flag));
  if (!upload_deferred_pair()) return 1;
  UINT temporal_m_with_previous_specular = 0u;
  float temporal_weight_with_previous_specular = 0.0f;
  prepare_temporal_frame();
  if (!seed_temporal_history_light(0u)
      || !run_di(4u, 4u, true, RTXDI_DI_STATUS_VISIBLE, RTXDI_DI_REASON_NONE, 0,
                 "RTXDI previous specular material reuses matching zero-albedo history", true, 1u, false, 0u,
                 false, true, true, true, true, false, nullptr, &temporal_m_with_previous_specular,
                 &temporal_weight_with_previous_specular)
      || temporal_m_with_previous_specular != temporal_m_without_previous_specular
      || !std::isfinite(temporal_weight_without_previous_specular)
      || !std::isfinite(temporal_weight_with_previous_specular)
      || std::abs(temporal_weight_with_previous_specular - temporal_weight_without_previous_specular) <= 1e-6f) {
    std::fprintf(stderr, "FAIL RTXDI previous material temporal M %u/%u weight %.9g/%.9g\n",
                 temporal_m_without_previous_specular, temporal_m_with_previous_specular,
                 temporal_weight_without_previous_specular, temporal_weight_with_previous_specular);
    return 1;
  }
  std::printf("PASS RTXDI previous-material normalization M=%u/%u W_noSpec=%.9g W_spec=%.9g\n",
              temporal_m_without_previous_specular, temporal_m_with_previous_specular,
              temporal_weight_without_previous_specular, temporal_weight_with_previous_specular);
  temporal_albedo = albedo_pixels;
  if (!update_pixel_texture(albedo_tex.Get(), albedo_upload.Get(), temporal_albedo.data(), 3u * 4u * sizeof(float))
      || !update_pixel_texture(prev_albedo_tex.Get(), prev_albedo_upload.Get(), temporal_albedo.data(),
                               3u * 4u * sizeof(float))) return 1;
  previous_deferred = deferred;
  if (!upload_deferred_pair()) return 1;
  prior_spatial_output = saved_spatial_output;
  prior_spatial_output_index = saved_spatial_output_index;
  accepted_temporal_frame = 2u;
  bridge_di_context->SetFrameIndex(accepted_temporal_frame);
  di_frame.parameters = falcom_world::rtxdi::MakeDiFrameParameters(bridge_di_config, *bridge_di_context);
  di_frame.parameters.width = 3u;
  di_frame.parameters.height = 2u;

  auto unchanged_lights = lights;
  lights[1].color[1] = 1e-5f;
  wait_context = "upload current spot for identity test";
  if (!update_upload(light_upload.Get(), lights.data(), sizeof(lights))) return 1;
  auto previous_lights = lights;
  previous_lights[1].pos[0] = 0.25f;
  wait_context = "upload moved previous lights";
  if (!update_upload(previous_light_upload.Get(), previous_lights.data(), sizeof(previous_lights))) return 1;
  prepare_temporal_frame();
  if (!seed_temporal_history_light(1u)) return 1;
  if (!run_di(4u, 4u, true, RTXDI_DI_STATUS_VISIBLE, RTXDI_DI_REASON_NONE, 0,
              "RTXDI temporal rejects moved light identity", true, 1u, true, 0u,
              false, true, true, false)) return 1;
  previous_lights = unchanged_lights;
  std::reverse(previous_lights.begin(), previous_lights.end());
  wait_context = "upload reordered previous lights";
  if (!update_upload(previous_light_upload.Get(), previous_lights.data(), sizeof(previous_lights))) return 1;
  prepare_temporal_frame();
  if (!seed_temporal_history_light(1u)) return 1;
  if (!run_di(4u, 4u, true, RTXDI_DI_STATUS_VISIBLE, RTXDI_DI_REASON_NONE, 0,
              "RTXDI temporal rejects reordered light identity", true, 1u, true, 0u,
              false, true, true, false)) return 1;
  lights = unchanged_lights;
  wait_context = "restore current lights";
  if (!update_upload(light_upload.Get(), lights.data(), sizeof(lights))) return 1;
  wait_context = "restore previous lights";
  if (!update_upload(previous_light_upload.Get(), lights.data(), sizeof(lights))) return 1;

  previous_scene_pixels[(RTXDI_SURFACE_VIEW_ROW + 2u) * 4u + 3u] = std::bit_cast<uint32_t>(-0.5f);
  wait_context = "upload previous camera depth offset";
  if (!update_pixel_texture(prev_scene_tex.Get(), prev_scene_upload.Get(), previous_scene_pixels.data(),
                            RTXDI_SURFACE_SCENE_ROW_COUNT * 16u)) return 1;
  prepare_temporal_frame();
  if (!run_di(4u, 4u, true, RTXDI_DI_STATUS_VISIBLE, RTXDI_DI_REASON_NONE, 0,
              "RTXDI temporal camera depth change rejects history", true, 1u, true, 0u,
              false, true, true, false)) return 1;
  previous_scene_pixels[(RTXDI_SURFACE_VIEW_ROW + 2u) * 4u + 3u] = std::bit_cast<uint32_t>(0.0f);
  wait_context = "restore previous camera";
  if (!update_pixel_texture(prev_scene_tex.Get(), prev_scene_upload.Get(), previous_scene_pixels.data(),
                            RTXDI_SURFACE_SCENE_ROW_COUNT * 16u)) return 1;

  std::array<float, 6> previous_depths = depths;
  previous_depths[0] = 0.9f;
  wait_context = "upload previous disocclusion depth";
  if (!update_pixel_texture(prev_depth_tex.Get(), prev_depth_upload.Get(), previous_depths.data(), 3u * sizeof(float))) return 1;
  prepare_temporal_frame();
  if (!run_di(4u, 4u, true, RTXDI_DI_STATUS_VISIBLE, RTXDI_DI_REASON_NONE, 0,
              "RTXDI temporal disocclusion rejects history", true, 1u, true, 0u,
              false, true, true, false)) return 1;
  previous_depths[0] = depths[0];
  wait_context = "restore previous depth";
  if (!update_pixel_texture(prev_depth_tex.Get(), prev_depth_upload.Get(), previous_depths.data(), 3u * sizeof(float))) return 1;

  auto previous_normals = normal_pixels;
  previous_normals[1] = 0u;
  wait_context = "upload previous normal mismatch";
  if (!update_pixel_texture(prev_normal_tex.Get(), prev_normal_upload.Get(), previous_normals.data(), 3u * 16u)) return 1;
  prepare_temporal_frame();
  if (!run_di(4u, 4u, true, RTXDI_DI_STATUS_VISIBLE, RTXDI_DI_REASON_NONE, 0,
              "RTXDI temporal normal mismatch rejects history", true, 1u, true, 0u,
              false, true, true, false)) return 1;
  wait_context = "restore previous normal";
  if (!update_pixel_texture(prev_normal_tex.Get(), prev_normal_upload.Get(), normal_pixels.data(), 3u * 16u)) return 1;

  std::array<uint32_t, 3u * 2u * 4u> previous_materials = mrt2_pixels;
  previous_materials[0] = 1u;
  wait_context = "upload previous material mismatch";
  if (!update_pixel_texture(prev_mrt2_tex.Get(), prev_mrt2_upload.Get(), previous_materials.data(),
                            3u * 4u * sizeof(uint32_t))) return 1;
  prepare_temporal_frame();
  if (!run_di(4u, 4u, true, RTXDI_DI_STATUS_VISIBLE, RTXDI_DI_REASON_NONE, 0,
              "RTXDI temporal material mismatch rejects history", true, 2u, true, 0u,
              false, true, true, false)) return 1;
  wait_context = "restore previous material";
  if (!update_pixel_texture(prev_mrt2_tex.Get(), prev_mrt2_upload.Get(), mrt2_pixels.data(),
                            3u * 4u * sizeof(uint32_t))) return 1;

  mrt2_pixels[4u] = 1u;
  wait_context = "upload material mismatch fixture";
  if (!update_pixel_texture(mrt2_tex.Get(), mrt2_upload.Get(), mrt2_pixels.data(), 3u * 4u * sizeof(uint32_t))
      || !run_di(4u, 4u, true, 0u, 0u, -1, "RTXDI spatial material mismatch rejects neighbor", false, 2u, true, 1u, true)) return 1;
  mrt2_pixels[4u] = 0u;
  wait_context = "restore material fixture";
  if (!update_pixel_texture(mrt2_tex.Get(), mrt2_upload.Get(), mrt2_pixels.data(), 3u * 4u * sizeof(uint32_t))) return 1;
  depths[1] = 0.9f;
  wait_context = "upload depth mismatch fixture";
  if (!update_pixel_texture(depth_tex.Get(), depth_upload.Get(), depths.data(), 3u * sizeof(float))
      || !run_di(4u, 4u, true, 0u, 0u, -1, "RTXDI spatial depth mismatch rejects neighbor", false, 1u, true, 1u, true)) return 1;
  depths[1] = 0.5f;
  wait_context = "restore depth fixture";
  if (!update_pixel_texture(depth_tex.Get(), depth_upload.Get(), depths.data(), 3u * sizeof(float))) return 1;
  normal_pixels[4u + 1u] = 32768u;
  wait_context = "upload normal mismatch fixture";
  if (!update_pixel_texture(normal_tex.Get(), normal_upload.Get(), normal_pixels.data(), 3u * 16u)
      || !run_di(4u, 4u, true, 0u, 0u, -1, "RTXDI spatial normal mismatch rejects neighbor", false, 1u, true, 1u, true)) return 1;
  normal_pixels[4u + 1u] = 65535u;
  wait_context = "restore normal fixture";
  if (!update_pixel_texture(normal_tex.Get(), normal_upload.Get(), normal_pixels.data(), 3u * 16u)) return 1;
  depths[1] = 0.0f;
  wait_context = "upload invalid neighbor fixture";
  if (!update_pixel_texture(depth_tex.Get(), depth_upload.Get(), depths.data(), 3u * sizeof(float))
      || !run_di(4u, 4u, true, 0u, 0u, -1, "RTXDI spatial invalid neighbor rejects reuse", false, 1u, true, 1u, true)) return 1;
  depths[1] = 0.5f;
  wait_context = "restore invalid neighbor fixture";
  if (!update_pixel_texture(depth_tex.Get(), depth_upload.Get(), depths.data(), 3u * sizeof(float))) return 1;

  for (auto& tile_record : tile_records) {
    std::fill(std::begin(tile_record.point_indices), std::end(tile_record.point_indices), -1);
    std::fill(std::begin(tile_record.spot_indices), std::end(tile_record.spot_indices), -1);
    tile_record.point_count = 0u;
    tile_record.spot_count = 0u;
    tile_record.tile_depth_inv = 0.0f;
  }
  tile_records[1].point_indices[0] = 0;
  tile_records[1].point_count = 1u;
  wait_context = "upload off-tile lamp record";
  if (!update_upload(tile_upload.Get(), tile_records.data(), tile_records.size() * sizeof(RtxdiTileData))
      || !run_di(4u, 1u, true, RTXDI_DI_STATUS_EMPTY, RTXDI_DI_REASON_INVALID_SAMPLE, -1,
                 "RTXDI exact tile membership rejects same light in neighboring tile", false)) return 1;
  tile_records[0].point_indices[0] = 0;
  tile_records[0].point_count = 1u;
  tile_records[1].point_count = 0u;
  wait_context = "upload in-tile lamp record";
  if (!update_upload(tile_upload.Get(), tile_records.data(), tile_records.size() * sizeof(RtxdiTileData))
      || !run_di(4u, 1u, true, RTXDI_DI_STATUS_VISIBLE, RTXDI_DI_REASON_NONE, 0,
                 "RTXDI exact tile membership accepts current tile", false)) return 1;
  tile_records[0].tile_depth_inv = 1.0f;
  tile_records[0].point_count = 0u;
  tile_records[576].point_indices[62] = 0;
  tile_records[576].point_count = UINT32_MAX;
  wait_context = "upload depth slice cap boundary record";
  if (!update_upload(tile_upload.Get(), tile_records.data(), tile_records.size() * sizeof(RtxdiTileData))
      || !run_di(4u, 1u, true, RTXDI_DI_STATUS_VISIBLE, RTXDI_DI_REASON_NONE, 0,
                 "RTXDI depth slice selects capped 63rd tile lamp", false)) return 1;
  tile_records[0].tile_depth_inv = 0.0f;
  wait_context = "upload mismatched depth slice record";
  if (!update_upload(tile_upload.Get(), tile_records.data(), tile_records.size() * sizeof(RtxdiTileData))
      || !run_di(4u, 1u, true, RTXDI_DI_STATUS_EMPTY, RTXDI_DI_REASON_INVALID_SAMPLE, -1,
                 "RTXDI depth slice mismatch rejects other-slice lamp", false)) return 1;
  for (auto& tile_record : tile_records) {
    tile_record.point_count = 0u;
    tile_record.spot_count = 0u;
  }
  wait_context = "upload untyped tile record";
  if (!update_upload(tile_upload.Get(), tile_records.data(), tile_records.size() * sizeof(RtxdiTileData))
      || !run_di(4u, 1u, true, RTXDI_DI_STATUS_EMPTY, RTXDI_DI_REASON_INVALID_SAMPLE, -1,
                 "RTXDI untyped tile candidate writes empty reservoir", false)) return 1;
  tile_records[0] = tile;
  wait_context = "restore local lamp tile records";
  if (!update_upload(tile_upload.Get(), tile_records.data(), tile_records.size() * sizeof(RtxdiTileData))) return 1;
  lights[0].color[0] = 0.0f;
  lights[1].color[1] = 8.0f;
  if (!update_upload(light_upload.Get(), lights.data(), sizeof(lights))) return 1;
  di_frame.parameters.di.initialSamplingParams.localLightSamplingMode = ReSTIRDI_LocalLightSamplingMode::Uniform;
  if (!run_di(0u, 4u, true, RTXDI_DI_STATUS_BLOCKED, RTXDI_DI_REASON_NONE, 1, "RTXDI spot sample, DXR blocker", true,
              1u, true)) return 1;
  mesh.bbox_min[3] = 1.0f;
  world_instances[0].header[2] = 1.0f;
  alpha_material.threshold = 0.5f;
  alpha_material.slice = 0u;
  alpha_material.swizzle = 0u;
  if (!update_upload(meshes.Get(), &mesh, sizeof(mesh))
      || !update_upload(instances.Get(), world_instances.data(), sizeof(world_instances))
      || !update_upload(alpha_materials.Get(), &alpha_material, sizeof(alpha_material))) return 1;
  if (!run_di(0u, 4u, true, RTXDI_DI_STATUS_BLOCKED, RTXDI_DI_REASON_NONE, 1,
              "RTXDI spot sample, alpha hole with opaque behind", false, 1u, true)) return 1;
  if (!update_atlas_texel(0xFFFFFFFFu)
      || !run_di(0u, 4u, true, RTXDI_DI_STATUS_BLOCKED, RTXDI_DI_REASON_NONE, 1,
                 "RTXDI spot sample, alpha accepted blocker", false, 1u, true)) return 1;
  if (!run_di(4u, 0u, true, RTXDI_DI_STATUS_EMPTY, RTXDI_DI_REASON_ZERO_LIGHTS, -1,
              "RTXDI zero-light empty reservoirs", false, 1u, true)) return 1;
  if (!run_di(4u, 4u, true, RTXDI_DI_STATUS_EMPTY, RTXDI_DI_REASON_INVALID_SURFACE, -1,
              "RTXDI zero deferred records invalidate surface", false, 0u, true)) return 1;
  if (!run_di(4u, 4u, false, RTXDI_DI_STATUS_UNKNOWN, RTXDI_DI_REASON_TRUNCATED, 1,
              "RTXDI incomplete coverage unknown", true, 1u, true)) return 1;
  depths.fill(0.0f);
  if (!update_pixel_texture(depth_tex.Get(), depth_upload.Get(), depths.data(), 3 * sizeof(float))) return 1;
  if (!run_di(4u, 4u, true, RTXDI_DI_STATUS_EMPTY, RTXDI_DI_REASON_INVALID_SURFACE, -1,
              "RTXDI invalid receiver empty reservoirs", false, 1u, true)) return 1;

  depths.fill(0.5f);
  auto environment_normals = normal_pixels;
  for (UINT i = 0u; i < 6u; ++i) environment_normals[i * 4u + 3u] = 8u;
  auto character_normals = environment_normals;
  for (UINT i = 0u; i < 6u; ++i) character_normals[i * 4u + 3u] = 0u;
  lights[0].color[0] = 0.0f;
  lights[0].chara_color[0] = 4.0f;
  lights[0].chara_color[1] = 1.0f;
  if (!update_pixel_texture(depth_tex.Get(), depth_upload.Get(), depths.data(), 3u * sizeof(float))
      || !update_pixel_texture(normal_tex.Get(), normal_upload.Get(), character_normals.data(), 3u * 16u)
      || !update_pixel_texture(prev_normal_tex.Get(), prev_normal_upload.Get(), environment_normals.data(), 3u * 16u)
      || !update_upload(light_upload.Get(), lights.data(), sizeof(lights))) return 1;
  float character_target_pdf = 0.0f;
  float character_raw_diffuse = 0.0f;
  float character_inverse_pdf = 0.0f;
  if (!run_di(4u, 1u, true, RTXDI_DI_STATUS_VISIBLE, RTXDI_DI_REASON_NONE, 0,
              "RTXDI character surface accepts no deferred material and samples charaColor", true, 0u, false, 0u,
              false, false, false, false, true, false, &character_target_pdf, nullptr, nullptr,
              &character_raw_diffuse, &character_inverse_pdf)
      || !std::isfinite(character_target_pdf) || character_target_pdf <= 0.0f
      || !std::isfinite(character_raw_diffuse) || character_raw_diffuse <= 0.0f
      || !std::isfinite(character_inverse_pdf) || character_inverse_pdf <= 0.0f) {
    std::fprintf(stderr, "FAIL character no-material DI target=%g raw=%g inversePdf=%g\n", character_target_pdf,
                 character_raw_diffuse, character_inverse_pdf);
    return 1;
  }
  std::printf("PASS RTXDI character DI accepts zero deferred rows and uses charaColor with no specular proposal\n");
  if (!run_di(0u, 1u, true, RTXDI_DI_STATUS_BLOCKED, RTXDI_DI_REASON_NONE, 0,
              "RTXDI character DI opaque blocker", false, 0u, false)
      || !run_di(4u, 1u, false, RTXDI_DI_STATUS_UNKNOWN, RTXDI_DI_REASON_TRUNCATED, 0,
                 "RTXDI character DI incomplete coverage", false, 0u, false)) return 1;

  const auto saved_point_tile_records = tile_records;
  for (auto& tile_record : tile_records) {
    tile_record.point_count = 0u;
    tile_record.spot_count = 0u;
    std::fill(std::begin(tile_record.spot_indices), std::end(tile_record.spot_indices), -1);
  }
  tile_records[0].spot_indices[0] = 0;
  tile_records[0].spot_count = 1u;
  if (!update_upload(tile_upload.Get(), tile_records.data(), tile_records.size() * sizeof(RtxdiTileData))
      || !run_di(4u, 1u, true, RTXDI_DI_STATUS_VISIBLE, RTXDI_DI_REASON_NONE, 0,
                 "RTXDI character spot uses charaColor", false, 0u, false)) return 1;
  tile_records = saved_point_tile_records;
  lights[0].color[0] = 8.0f;
  if (!update_upload(tile_upload.Get(), tile_records.data(), tile_records.size() * sizeof(RtxdiTileData))
      || !update_upload(light_upload.Get(), lights.data(), sizeof(lights))) return 1;

  auto mixed_class_normals = environment_normals;
  mixed_class_normals[3u] = 0u;
  if (!update_pixel_texture(normal_tex.Get(), normal_upload.Get(), mixed_class_normals.data(), 3u * 16u)) return 1;
  UINT mixed_class_spatial_m = 0u;
  if (!run_di(4u, 1u, true, RTXDI_DI_STATUS_VISIBLE, RTXDI_DI_REASON_NONE, 0,
              "RTXDI spatial resampling rejects character/environment class mismatch", false, 1u, true, 1u,
              false, false, false, false, true, false, nullptr, &mixed_class_spatial_m)
      || mixed_class_spatial_m != 1u) return 1;
  UINT mixed_class_temporal_m = 0u;
  prepare_temporal_frame();
  const bool mixed_class_temporal_ok = seed_temporal_history_light(0u)
                                       && run_di(4u, 1u, true, RTXDI_DI_STATUS_VISIBLE, RTXDI_DI_REASON_NONE, 0,
                                                 "RTXDI temporal resampling rejects character/environment class mismatch",
                                                 false, 1u, false, 0u, false, true, true, false, true, false, nullptr,
                                                 &mixed_class_temporal_m);
  if (!mixed_class_temporal_ok || mixed_class_temporal_m != 1u) return 1;
  const auto reverse_class_normals = character_normals;
  if (!update_pixel_texture(normal_tex.Get(), normal_upload.Get(), environment_normals.data(), 3u * 16u)
      || !update_pixel_texture(prev_normal_tex.Get(), prev_normal_upload.Get(), reverse_class_normals.data(),
                               3u * 16u)) return 1;
  UINT reverse_class_temporal_m = 0u;
  prepare_temporal_frame();
  if (!seed_temporal_history_light(0u)
      || !run_di(4u, 1u, true, RTXDI_DI_STATUS_VISIBLE, RTXDI_DI_REASON_NONE, 0,
                 "RTXDI temporal rejects character in jittered previous candidate neighborhood", false, 1u,
                 false, 0u, false, true, true, false, true, false, nullptr, &reverse_class_temporal_m)
      || reverse_class_temporal_m != 1u) return 1;
  if (!update_pixel_texture(normal_tex.Get(), normal_upload.Get(), mixed_class_normals.data(), 3u * 16u)
      || !update_pixel_texture(prev_normal_tex.Get(), prev_normal_upload.Get(), environment_normals.data(),
                               3u * 16u)) return 1;

  // Start the four-mode readback matrix from a stable environment receiver,
  // point-only tile, and matching prior-frame guides.
  if (!update_pixel_texture(normal_tex.Get(), normal_upload.Get(), environment_normals.data(), 3u * 16u)
      || !update_pixel_texture(prev_normal_tex.Get(), prev_normal_upload.Get(), environment_normals.data(), 3u * 16u)
      || !update_pixel_texture(prev_mrt2_tex.Get(), prev_mrt2_upload.Get(), mrt2_pixels.data(),
                               3u * 4u * sizeof(uint32_t))
      || !upload_deferred_pair()) return 1;
  lights[0].color[0] = 8.0f;
  lights[1].color[1] = 0.0f;
  if (!update_upload(light_upload.Get(), lights.data(), sizeof(lights))) return 1;

  const std::array<const char*, 4u> resampling_mode_names = {"None", "Temporal", "Spatial", "Both"};
  if (!update_upload(di_reservoir_upload.Get(), reservoir_sentinel.data(), static_cast<size_t>(di_reservoir_bytes))) return 1;
  for (UINT mode = 0u; mode < resampling_mode_names.size(); ++mode) {
    auto mode_settings = bridge_settings;
    mode_settings.temporal_enabled = (mode & 1u) != 0u;
    mode_settings.spatial_enabled = (mode & 2u) != 0u;
    bridge_di_context = falcom_world::rtxdi::CreateDiContext(bridge_di_config, mode_settings);
    if (!bridge_di_context) return 1;
    prior_spatial_output.clear();
    prior_reservoir_state.clear();
    prior_spatial_output_index = UINT_MAX;
    baseline_initial_m = 0u;
    for (UINT frame = 0u; frame < 6u; ++frame) {
      if (frame != 0u) bridge_di_context->SetFrameIndex(bridge_di_context->GetFrameIndex() + 1u);
      char mode_label[96] = {};
      std::snprintf(mode_label, sizeof(mode_label), "RTXDI final resolve mode %s frame %u",
                    resampling_mode_names[mode], frame);
      const bool temporal_enabled = mode_settings.temporal_enabled;
      const bool spatial_enabled = mode_settings.spatial_enabled;
      const bool history_valid = temporal_enabled && frame != 0u;
      if (!run_di(4u, 1u, true, RTXDI_DI_STATUS_VISIBLE, RTXDI_DI_REASON_NONE, 0, mode_label, true, 1u,
                  spatial_enabled, 0u, false, frame != 0u, temporal_enabled,
                  temporal_enabled && frame != 0u, history_valid, false, nullptr, nullptr, nullptr, nullptr,
                  nullptr, true)) return 1;
      if (temporal_enabled && frame == 0u && !prior_spatial_output.empty()) {
        baseline_initial_m = (prior_spatial_output[0].mVisibility >> 18u) & 0x3fffu;
      }
    }
    std::printf("PASS RTXDI six-frame GPU mode=%s final-index=%u temporal=%u spatial=%u; final raw/tuple/N1 and unwritten-array guards checked\n",
                resampling_mode_names[mode], bridge_di_context->GetBufferIndices().shadingInputBufferIndex,
                static_cast<unsigned>(mode_settings.temporal_enabled),
                static_cast<unsigned>(mode_settings.spatial_enabled));
  }

  D3D12_RESOURCE_BARRIER raw_guard_barrier = {};
  raw_guard_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  raw_guard_barrier.Transition.pResource = di_raw.Get();
  raw_guard_barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  raw_guard_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
  raw_guard_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
  list->ResourceBarrier(1u, &raw_guard_barrier);
  list->SetDescriptorHeaps(1u, di_heaps);
  list->SetComputeRootSignature(raw_guard_root.Get());
  list->SetPipelineState(raw_guard_pso.Get());
  const UINT raw_guard_values[6] = {std::bit_cast<UINT>(3.0e38f), std::bit_cast<UINT>(3.0e38f),
                                    std::bit_cast<UINT>(3.0e38f), std::bit_cast<UINT>(2.0f),
                                    std::bit_cast<UINT>(1.0f), 1u};
  list->SetComputeRoot32BitConstants(0u, 6u, raw_guard_values, 0u);
  auto raw_guard_table = di_heap->GetGPUDescriptorHandleForHeapStart();
  raw_guard_table.ptr += static_cast<UINT64>(19u) * di_descriptor_size;
  list->SetComputeRootDescriptorTable(1u, raw_guard_table);
  list->Dispatch(1u, 1u, 1u);
  D3D12_RESOURCE_BARRIER raw_guard_uav = {};
  raw_guard_uav.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
  raw_guard_uav.UAV.pResource = di_raw.Get();
  list->ResourceBarrier(1u, &raw_guard_uav);
  raw_guard_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
  raw_guard_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
  list->ResourceBarrier(1u, &raw_guard_barrier);
  list->CopyBufferRegion(di_raw_readback.Get(), 0u, di_raw.Get(), 0u, sizeof(RtxdiDiRawResult));
  raw_guard_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
  raw_guard_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COMMON;
  list->ResourceBarrier(1u, &raw_guard_barrier);
  if (!submit_wait()) {
    std::fprintf(stderr, "FAIL raw guard command submission/wait\n");
    return 1;
  }
  void* raw_guard_mapped = nullptr;
  if (!Check(di_raw_readback->Map(0u, nullptr, &raw_guard_mapped), "map raw guard result")) return 1;
  const RtxdiDiRawResult raw_guard_result = *static_cast<const RtxdiDiRawResult*>(raw_guard_mapped);
  const bool raw_guard_ok = raw_guard_result.status == RTXDI_DI_STATUS_UNKNOWN
                            && raw_guard_result.reason == RTXDI_DI_REASON_INVALID_SAMPLE
                            && raw_guard_result.diffuse_radiance[0] == 0.0f
                            && raw_guard_result.diffuse_radiance[1] == 0.0f
                            && raw_guard_result.diffuse_radiance[2] == 0.0f;
  di_raw_readback->Unmap(0u, nullptr);
  if (!raw_guard_ok) {
    std::fprintf(stderr, "FAIL raw guard status=%u reason=%u diffuse=%g,%g,%g\n", raw_guard_result.status,
                 raw_guard_result.reason, raw_guard_result.diffuse_radiance[0], raw_guard_result.diffuse_radiance[1],
                 raw_guard_result.diffuse_radiance[2]);
    return 1;
  }
  std::printf("PASS finite-input raw overflow sanitizer writes UNKNOWN and zero diffuse\n");

  constexpr uint32_t lighting_case_count = 57u;
  std::fprintf(stderr, "DIAG lighting setup start\n");
  std::array<LightingConsumerCase, lighting_case_count> lighting_cases = {};
  for (LightingConsumerCase& test_case : lighting_cases) {
    test_case.light_count = 16u;
    test_case.original_point_count = 2u;
    test_case.original_spot_count = 2u;
    test_case.lamp_radiance = 0.75f;
    test_case.base_color = 0.25f;
    test_case.dynamic_intensity = 0.5f;
    test_case.tile.point_count = 2u;
    test_case.tile.point_indices[0] = 5;
    test_case.tile.point_indices[1] = 6;
    test_case.tile.spot_count = 2u;
    test_case.tile.spot_indices[0] = 7;
    test_case.tile.spot_indices[1] = 8;
  }
  lighting_cases[0].tuple[0] = 5u;
  lighting_cases[0].tuple[1] = RTXDI_DI_LIGHT_TYPE_POINT;
  lighting_cases[0].tuple[2] = RTXDI_DI_STATUS_VISIBLE;
  lighting_cases[0].tuple[3] = std::bit_cast<uint32_t>(2.f);
  lighting_cases[0].bound = 1u;
  lighting_cases[0].lamp_radiance = 0.5f;
  lighting_cases[1].tuple[0] = 7u;
  lighting_cases[1].tuple[1] = RTXDI_DI_LIGHT_TYPE_SPOT;
  lighting_cases[1].tuple[2] = RTXDI_DI_STATUS_VISIBLE;
  lighting_cases[1].tuple[3] = std::bit_cast<uint32_t>(3.f);
  lighting_cases[1].bound = 1u;
  lighting_cases[1].lamp_radiance = 0.25f;
  lighting_cases[2].tuple[0] = 5u;
  lighting_cases[2].tuple[1] = RTXDI_DI_LIGHT_TYPE_POINT;
  lighting_cases[2].tuple[2] = RTXDI_DI_STATUS_BLOCKED;
  lighting_cases[2].tuple[3] = std::bit_cast<uint32_t>(2.f);
  lighting_cases[2].bound = 1u;
  lighting_cases[3].tuple[0] = 5u;
  lighting_cases[3].tuple[1] = RTXDI_DI_LIGHT_TYPE_POINT;
  lighting_cases[3].tuple[2] = RTXDI_DI_STATUS_UNKNOWN;
  lighting_cases[3].tuple[3] = std::bit_cast<uint32_t>(2.f);
  lighting_cases[3].bound = 1u;
  lighting_cases[4] = lighting_cases[0];
  lighting_cases[4].tuple[0] = 9u;
  lighting_cases[5] = lighting_cases[1];
  lighting_cases[5].bound = 0u;
  lighting_cases[6] = lighting_cases[3];
  lighting_cases[6].tuple[2] = RTXDI_DI_STATUS_EMPTY;
  lighting_cases[6].original_point_count = 0u;
  lighting_cases[6].original_spot_count = 0u;
  lighting_cases[7] = lighting_cases[1];
  lighting_cases[7].tuple[1] = 3u;
  lighting_cases[8] = lighting_cases[3];
  lighting_cases[9] = lighting_cases[0];
  lighting_cases[9].nrd_diffuse[0] = 0.125f;
  lighting_cases[9].nrd_diffuse[1] = 0.25f;
  lighting_cases[9].nrd_diffuse[2] = 0.5f;
  lighting_cases[9].nrd_diffuse[3] = 1.0f;
  lighting_cases[9].nrd_bound = 1u;
  lighting_cases[9].expected_class = 1u;
  lighting_cases[9].raw_specular = 5.0f;
  lighting_cases[10] = lighting_cases[1];
  lighting_cases[10].nrd_diffuse[0] = 0.375f;
  lighting_cases[10].nrd_diffuse[1] = 0.125f;
  lighting_cases[10].nrd_diffuse[2] = 0.25f;
  lighting_cases[10].nrd_diffuse[3] = 1.0f;
  lighting_cases[10].nrd_bound = 1u;
  lighting_cases[10].expected_class = 1u;
  lighting_cases[10].raw_specular = 7.0f;
  lighting_cases[11] = lighting_cases[0];
  lighting_cases[11].nrd_diffuse[0] = 0.875f;
  lighting_cases[11].nrd_diffuse[3] = 1.0f;
  lighting_cases[11].nrd_bound = 1u;
  lighting_cases[11].expected_class = 2u;
  lighting_cases[12] = lighting_cases[9];
  lighting_cases[12].nrd_diffuse[0] = std::numeric_limits<float>::quiet_NaN();
  lighting_cases[13] = lighting_cases[9];
  lighting_cases[13].nrd_diffuse[0] = -0.25f;
  lighting_cases[14] = lighting_cases[9];
  lighting_cases[14].nrd_bound = 0u;
  lighting_cases[15] = lighting_cases[2];
  lighting_cases[15].nrd_diffuse[0] = 0.25f;
  lighting_cases[15].nrd_diffuse[1] = 0.125f;
  lighting_cases[15].nrd_diffuse[2] = 0.0625f;
  lighting_cases[15].nrd_diffuse[3] = 1.0f;
  lighting_cases[15].nrd_bound = 1u;
  lighting_cases[15].expected_class = 1u;
  lighting_cases[16] = lighting_cases[0];
  lighting_cases[16].nrd_diffuse[0] = 0.625f;
  lighting_cases[16].nrd_diffuse[1] = 0.25f;
  lighting_cases[16].nrd_diffuse[2] = 0.125f;
  lighting_cases[16].nrd_diffuse[3] = 2.0f;
  lighting_cases[16].nrd_bound = 1u;
  lighting_cases[16].expected_class = 2u;
  lighting_cases[17].debug_view = RTXDI_DEBUG_VIEW_LOCAL_STATUS;
  lighting_cases[17].debug_flags = 1u;
  lighting_cases[17].debug_status = RTXDI_N1_STATUS_VISIBLE | RTXDI_N1_STATUS_KNOWN_BIT;
  lighting_cases[18] = lighting_cases[17];
  lighting_cases[18].debug_status = RTXDI_N1_STATUS_BLOCKED | RTXDI_N1_STATUS_KNOWN_BIT;
  lighting_cases[19] = lighting_cases[17];
  lighting_cases[19].debug_status = RTXDI_N1_STATUS_UNKNOWN;
  lighting_cases[20] = lighting_cases[17];
  lighting_cases[20].debug_status = RTXDI_N1_STATUS_EMPTY;
  lighting_cases[21] = lighting_cases[17];
  lighting_cases[21].debug_status = RTXDI_N1_STATUS_INVALID;
  lighting_cases[22] = lighting_cases[17];
  lighting_cases[22].debug_view = RTXDI_DEBUG_VIEW_RECEIVER_CLASS;
  lighting_cases[22].debug_status = RTXDI_N1_STATUS_VISIBLE | RTXDI_N1_STATUS_KNOWN_BIT
                                    | RTXDI_N1_STATUS_CHARACTER_BIT;
  lighting_cases[23] = lighting_cases[22];
  lighting_cases[23].debug_status = RTXDI_N1_STATUS_BLOCKED | RTXDI_N1_STATUS_KNOWN_BIT;
  lighting_cases[24].debug_view = RTXDI_DEBUG_VIEW_SELECTED_LIGHT;
  lighting_cases[24].debug_flags = 4u;
  lighting_cases[24].debug_tuple[0] = 5u;
  lighting_cases[24].debug_tuple[1] = RTXDI_DI_LIGHT_TYPE_POINT;
  lighting_cases[24].debug_tuple[2] = RTXDI_DI_STATUS_VISIBLE;
  lighting_cases[24].debug_tuple[3] = std::bit_cast<uint32_t>(1.0f);
  lighting_cases[25] = lighting_cases[17];
  lighting_cases[25].debug_view = RTXDI_DEBUG_VIEW_RAW_DIFFUSE;
  lighting_cases[25].debug_flags = 3u;
  lighting_cases[26] = lighting_cases[25];
  lighting_cases[26].debug_flags = 1u;
  lighting_cases[27] = lighting_cases[17];
  lighting_cases[27].debug_view = RTXDI_DEBUG_VIEW_FILTERED_DIFFUSE;
  lighting_cases[27].debug_flags = 9u;
  lighting_cases[27].debug_filtered[3] = 1.0f;
  lighting_cases[28] = lighting_cases[27];
  lighting_cases[28].debug_filtered[3] = 2.0f;
  lighting_cases[29] = lighting_cases[17];
  lighting_cases[29].debug_view = RTXDI_DEBUG_VIEW_DISTANCE_VALIDITY;
  lighting_cases[29].debug_flags = 17u;
  lighting_cases[29].debug_status = RTXDI_N1_STATUS_VISIBLE | RTXDI_N1_STATUS_KNOWN_BIT;
  lighting_cases[29].debug_value[0] = 3.25f;
  lighting_cases[29].debug_value[1] = 1.0f;
  lighting_cases[29].debug_value[3] = 1.0f;
  lighting_cases[30] = lighting_cases[29];
  lighting_cases[30].debug_view = RTXDI_DEBUG_VIEW_DISTANCE_MAGNITUDE;
  lighting_cases[30].debug_value[0] = 65504.0f;
  lighting_cases[30].debug_value[1] = 5.0f;
  lighting_cases[31] = lighting_cases[29];
  lighting_cases[31].debug_value[1] = 0.0f;
  lighting_cases[32] = lighting_cases[29];
  lighting_cases[32].debug_status |= RTXDI_N1_STATUS_CHARACTER_BIT;
  lighting_cases[32].debug_value[1] = 1.0f;
  lighting_cases[33].debug_view = RTXDI_DEBUG_VIEW_SUN_VISIBILITY;
  lighting_cases[33].debug_flags = 32u;
  lighting_cases[33].debug_sun_status = RTXDI_SUN_STATUS_VISIBLE;
  lighting_cases[34] = lighting_cases[33];
  lighting_cases[34].debug_sun_status = RTXDI_SUN_STATUS_BLOCKED;
  lighting_cases[35] = lighting_cases[33];
  lighting_cases[35].debug_sun_status = RTXDI_SUN_STATUS_UNKNOWN;
  lighting_cases[29].pack_enabled = 1u;
  lighting_cases[29].pack_distance = 3.25f;
  lighting_cases[29].pack_status = RTXDI_N1_STATUS_VISIBLE | RTXDI_N1_STATUS_KNOWN_BIT;
  lighting_cases[29].pack_flags = 1u;
  lighting_cases[30].pack_enabled = 1u;
  lighting_cases[30].pack_distance = 65504.0f;
  lighting_cases[30].pack_status = RTXDI_N1_STATUS_BLOCKED | RTXDI_N1_STATUS_KNOWN_BIT;
  lighting_cases[30].pack_flags = 5u;
  lighting_cases[31].pack_enabled = 1u;
  lighting_cases[31].pack_distance = 3.25f;
  lighting_cases[31].pack_status = RTXDI_N1_STATUS_VISIBLE | RTXDI_N1_STATUS_KNOWN_BIT;
  lighting_cases[31].pack_flags = 0u;
  lighting_cases[32].pack_enabled = 1u;
  lighting_cases[32].pack_distance = 3.25f;
  lighting_cases[32].pack_status = RTXDI_N1_STATUS_VISIBLE | RTXDI_N1_STATUS_KNOWN_BIT
                                   | RTXDI_N1_STATUS_CHARACTER_BIT;
  lighting_cases[32].pack_flags = 1u;
  lighting_cases[36].pack_enabled = 1u;
  lighting_cases[36].pack_distance = -1.0f;
  lighting_cases[36].pack_status = RTXDI_N1_STATUS_VISIBLE | RTXDI_N1_STATUS_KNOWN_BIT;
  lighting_cases[36].pack_flags = 1u;
  lighting_cases[37].pack_enabled = 1u;
  lighting_cases[37].pack_distance = std::numeric_limits<float>::quiet_NaN();
  lighting_cases[37].pack_status = RTXDI_N1_STATUS_VISIBLE | RTXDI_N1_STATUS_KNOWN_BIT;
  lighting_cases[37].pack_flags = 1u;
  lighting_cases[38].pack_enabled = 1u;
  lighting_cases[38].pack_distance = 3.25f;
  lighting_cases[38].pack_status = RTXDI_N1_STATUS_UNKNOWN;
  lighting_cases[38].pack_flags = 1u;
  lighting_cases[39].pack_enabled = 1u;
  lighting_cases[39].pack_distance = 65503.5f;
  lighting_cases[39].pack_status = RTXDI_N1_STATUS_VISIBLE | RTXDI_N1_STATUS_KNOWN_BIT;
  lighting_cases[39].pack_flags = 5u;
  lighting_cases[40].pack_enabled = 1u;
  lighting_cases[40].pack_distance = 3.25f;
  lighting_cases[40].pack_status = RTXDI_N1_STATUS_VISIBLE | RTXDI_N1_STATUS_KNOWN_BIT
                                   | RTXDI_N1_STATUS_CHARACTER_BIT;
  lighting_cases[40].pack_flags = 3u;
  lighting_cases[41].pack_enabled = 1u;
  lighting_cases[41].pack_distance = 3.25f;
  lighting_cases[41].pack_status = RTXDI_N1_STATUS_BLOCKED | RTXDI_N1_STATUS_KNOWN_BIT
                                   | RTXDI_N1_STATUS_CHARACTER_BIT;
  lighting_cases[41].pack_flags = 3u;
  for (uint32_t index = 42u; index < lighting_case_count; ++index) {
    lighting_cases[index].debug_view = RTXDI_DEBUG_VIEW_SUN_VISIBILITY;
    lighting_cases[index].debug_consumer_decision = RTXDI_SUN_DETAIL_REASON;
  }
  lighting_cases[42].debug_tuple[0] = RTXDI_SUN_STATUS_VISIBLE;
  lighting_cases[42].debug_tuple[1] = RTXDI_SUN_REASON_NONE;
  lighting_cases[42].debug_tuple[2] = std::bit_cast<uint32_t>(5.0f);
  lighting_cases[43].debug_tuple[0] = RTXDI_SUN_STATUS_UNKNOWN;
  lighting_cases[43].debug_tuple[1] = RTXDI_SUN_REASON_INVALID_SURFACE;
  lighting_cases[44].debug_tuple[0] = RTXDI_SUN_STATUS_UNKNOWN;
  lighting_cases[44].debug_tuple[1] = RTXDI_SUN_REASON_INVALID_SCENE;
  lighting_cases[45].debug_tuple[0] = RTXDI_SUN_STATUS_UNKNOWN;
  lighting_cases[45].debug_tuple[1] = RTXDI_SUN_REASON_INVALID_ALPHA;
  lighting_cases[46].debug_tuple[0] = RTXDI_SUN_STATUS_UNKNOWN;
  lighting_cases[46].debug_tuple[1] = RTXDI_SUN_REASON_COVERAGE_MISS;
  lighting_cases[47].debug_tuple[0] = RTXDI_SUN_STATUS_UNKNOWN;
  lighting_cases[47].debug_tuple[1] = RTXDI_SUN_REASON_TRUNCATED;
  lighting_cases[48].debug_tuple[0] = RTXDI_SUN_STATUS_UNKNOWN;
  lighting_cases[48].debug_tuple[1] = RTXDI_SUN_REASON_INVALID_RAY;
  lighting_cases[49] = lighting_cases[43];
  lighting_cases[49].debug_consumer_decision = RTXDI_SUN_DETAIL_HIT_DISTANCE;
  lighting_cases[50] = lighting_cases[42];
  lighting_cases[50].debug_consumer_decision = RTXDI_SUN_DETAIL_HIT_DISTANCE;
  lighting_cases[50].debug_tuple[2] = std::bit_cast<uint32_t>(3.25f);
  lighting_cases[51] = lighting_cases[50];
  lighting_cases[51].debug_tuple[2] = std::bit_cast<uint32_t>(std::numeric_limits<float>::quiet_NaN());
  lighting_cases[52].debug_view = RTXDI_DEBUG_VIEW_SUN_VISIBILITY;
  lighting_cases[52].debug_consumer_decision = RTXDI_SUN_DETAIL_HIT_DISTANCE;
  lighting_cases[52].debug_tuple[0] = RTXDI_SUN_STATUS_BLOCKED;
  lighting_cases[52].debug_tuple[1] = RTXDI_SUN_REASON_NONE;
  lighting_cases[52].debug_tuple[2] = std::bit_cast<uint32_t>(0.0f);
  lighting_cases[0].sun_bound = 1u;
  lighting_cases[0].sun_status = RTXDI_SUN_STATUS_VISIBLE;
  lighting_cases[1].sun_bound = 1u;
  lighting_cases[1].sun_status = RTXDI_SUN_STATUS_BLOCKED;
  lighting_cases[2].sun_bound = 1u;
  lighting_cases[2].sun_status = RTXDI_SUN_STATUS_UNKNOWN;
  lighting_cases[2].csm_visibility = 0.35f;
  lighting_cases[3].sun_bound = 0u;
  lighting_cases[3].sun_status = RTXDI_SUN_STATUS_VISIBLE;
  lighting_cases[3].csm_visibility = 0.65f;
  lighting_cases[4].sun_bound = 1u;
  lighting_cases[4].sun_status = 17u;
  lighting_cases[4].csm_visibility = 0.2f;
  lighting_cases[53].sun_bound = 1u;
  lighting_cases[53].sun_status = RTXDI_SUN_STATUS_VISIBLE | (51u << RTXDI_SUN_WIND_OCCLUSION_SHIFT);
  lighting_cases[53].sun_transmission = 0.5f;
  lighting_cases[54].sun_bound = 1u;
  lighting_cases[54].sun_status = RTXDI_SUN_STATUS_VISIBLE | (2u << RTXDI_SUN_LAYER_SHIFT);
  lighting_cases[54].sun_transmission = 0.5f;
  lighting_cases[55].sun_bound = 1u;
  lighting_cases[55].sun_status = RTXDI_SUN_STATUS_BLOCKED;
  lighting_cases[55].sun_transmission = 0.5f;
  lighting_cases[56].sun_bound = 1u;
  lighting_cases[56].sun_status = 0x80000000u | RTXDI_SUN_STATUS_BLOCKED;
  lighting_cases[56].sun_transmission = 0.5f;
  lighting_cases[56].csm_visibility = 0.4f;
  for (LightingConsumerCase& test_case : lighting_cases) {
    test_case.padding[0] = 0u;
    test_case.padding[1] = 0u;
    test_case.padding2 = 0.f;
    std::fill(std::begin(test_case.tile.probe_indices), std::end(test_case.tile.probe_indices), -1);
  }
  for (UINT i = 0u; i < 8u; ++i) lighting_cases[i].padding[0] = 1u;

  ComPtr<ID3D12Resource> lighting_input, lighting_output, lighting_readback;
  if (!UploadBuffer(device.Get(), lighting_cases.data(), sizeof(lighting_cases), &lighting_input)
      || !CreateDefaultBuffer(device.Get(), sizeof(LightingConsumerResult) * lighting_case_count,
                              D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON,
                              &lighting_output)) return 1;
  std::fprintf(stderr, "DIAG lighting resources ready\n");
  const auto lighting_readback_desc = BufferDesc(sizeof(LightingConsumerResult) * lighting_case_count);
  if (!Check(device->CreateCommittedResource(&readback_properties, D3D12_HEAP_FLAG_NONE, &lighting_readback_desc,
                                             D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                             IID_PPV_ARGS(&lighting_readback)),
             "create lighting contract readback")) return 1;
  D3D12_DESCRIPTOR_RANGE lighting_ranges[2] = {};
  lighting_ranges[0] = {D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1u, 0u, 0u, 0u};
  lighting_ranges[1] = {D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1u, 0u, 0u, 1u};
  ComPtr<ID3D12RootSignature> lighting_root;
  ComPtr<ID3D12PipelineState> lighting_pso;
  if (!CreateRoot(device.Get(), lighting_ranges, 2u, 1u, &lighting_root)
      || !CreatePso(device.Get(), lighting_root.Get(), __rtxdi_lighting_consumer_contract, &lighting_pso)) return 1;
  std::fprintf(stderr, "DIAG lighting pipeline ready\n");
  D3D12_DESCRIPTOR_HEAP_DESC lighting_heap_desc = {};
  lighting_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
  lighting_heap_desc.NumDescriptors = 2u;
  lighting_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
  ComPtr<ID3D12DescriptorHeap> lighting_heap;
  if (!Check(device->CreateDescriptorHeap(&lighting_heap_desc, IID_PPV_ARGS(&lighting_heap)),
             "create lighting contract heap")) return 1;
  const UINT lighting_increment = device->GetDescriptorHandleIncrementSize(lighting_heap_desc.Type);
  if (!CreateBufferSrv(device.Get(), lighting_heap.Get(), lighting_increment, 0u, lighting_input.Get(),
                       lighting_case_count, sizeof(LightingConsumerCase))) return 1;
  D3D12_UNORDERED_ACCESS_VIEW_DESC lighting_uav = {};
  lighting_uav.Format = DXGI_FORMAT_UNKNOWN;
  lighting_uav.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
  lighting_uav.Buffer.NumElements = lighting_case_count;
  lighting_uav.Buffer.StructureByteStride = sizeof(LightingConsumerResult);
  auto lighting_cpu = lighting_heap->GetCPUDescriptorHandleForHeapStart();
  lighting_cpu.ptr += lighting_increment;
  device->CreateUnorderedAccessView(lighting_output.Get(), nullptr, &lighting_uav, lighting_cpu);

  if (!Check(allocator->Reset(), "reset lighting contract allocator")
      || !Check(base_list->Reset(allocator.Get(), nullptr), "reset lighting contract list")) return 1;
  ID3D12DescriptorHeap* lighting_heaps[] = {lighting_heap.Get()};
  list->SetDescriptorHeaps(1u, lighting_heaps);
  list->SetComputeRootSignature(lighting_root.Get());
  list->SetPipelineState(lighting_pso.Get());
  const UINT lighting_case_count_constant = lighting_case_count;
  list->SetComputeRoot32BitConstants(0u, 1u, &lighting_case_count_constant, 0u);
  list->SetComputeRootDescriptorTable(1u, lighting_heap->GetGPUDescriptorHandleForHeapStart());
  D3D12_RESOURCE_BARRIER lighting_barrier = {};
  lighting_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  lighting_barrier.Transition.pResource = lighting_output.Get();
  lighting_barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  lighting_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
  lighting_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
  list->ResourceBarrier(1u, &lighting_barrier);
  list->Dispatch(1u, 1u, 1u);
  D3D12_RESOURCE_BARRIER lighting_uav_barrier = {};
  lighting_uav_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
  lighting_uav_barrier.UAV.pResource = lighting_output.Get();
  list->ResourceBarrier(1u, &lighting_uav_barrier);
  lighting_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
  lighting_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
  list->ResourceBarrier(1u, &lighting_barrier);
  list->CopyBufferRegion(lighting_readback.Get(), 0u, lighting_output.Get(), 0u,
                         sizeof(LightingConsumerResult) * lighting_case_count);
  lighting_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
  lighting_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COMMON;
  list->ResourceBarrier(1u, &lighting_barrier);
  if (!submit_wait()) {
    std::fprintf(stderr, "FAIL lighting consumer contract submission/wait\n");
    return 1;
  }
  void* lighting_mapped = nullptr;
  if (!Check(lighting_readback->Map(0u, nullptr, &lighting_mapped), "map lighting consumer results")) return 1;
  std::array<LightingConsumerResult, lighting_case_count> lighting_results = {};
  std::memcpy(lighting_results.data(), lighting_mapped, sizeof(lighting_results));
  const uint32_t selected_debug_hash = 5u * 1664525u + 1013904223u;
  const float selected_debug_brightness = 0.35f + 0.65f * (static_cast<float>(selected_debug_hash >> 24u) / 255.0f);
  const bool lighting_contract_ok =
      lighting_results[0].mode == 1u && lighting_results[0].point_count == 1u && lighting_results[0].spot_count == 0u
      && std::abs(lighting_results[0].raw_local_term - 1.f) < 1e-6f
      && std::abs(lighting_results[0].shaded_local_term - 0.125f) < 1e-6f
      && lighting_results[0].selected_index == 5u && lighting_results[0].selected_type == RTXDI_DI_LIGHT_TYPE_POINT
      && lighting_results[1].mode == 1u && lighting_results[1].point_count == 0u && lighting_results[1].spot_count == 1u
      && std::abs(lighting_results[1].raw_local_term - 0.75f) < 1e-6f
      && std::abs(lighting_results[1].shaded_local_term - 0.09375f) < 1e-6f
      && lighting_results[1].selected_index == 7u && lighting_results[1].selected_type == RTXDI_DI_LIGHT_TYPE_SPOT
      && lighting_results[2].mode == 2u && lighting_results[2].point_count == 0u && lighting_results[2].spot_count == 0u
      && lighting_results[2].raw_local_term == 0.f && lighting_results[3].mode == 0u
      && lighting_results[3].point_count == 2u && lighting_results[3].spot_count == 2u
      && std::abs(lighting_results[3].shaded_local_term - 0.09375f) < 1e-6f
      && lighting_results[4].mode == 0u && lighting_results[4].point_count == 2u
      && lighting_results[4].spot_count == 2u && lighting_results[5].mode == 0u
      && lighting_results[5].point_count == 2u && lighting_results[5].spot_count == 2u
      && lighting_results[6].mode == 0u && lighting_results[6].point_count == 0u
      && lighting_results[6].spot_count == 0u && lighting_results[6].raw_local_term == 0.f
      && lighting_results[7].mode == 0u && lighting_results[7].point_count == 2u
      && lighting_results[7].spot_count == 2u
      && lighting_results[0].sun_known == 1u && lighting_results[0].environment_sun_visibility == 1.f
      && lighting_results[0].character_sun_multiplier == 1.f
      && lighting_results[1].sun_known == 1u && lighting_results[1].environment_sun_visibility == 0.f
      && lighting_results[1].character_sun_multiplier == 0.f
      && lighting_results[2].sun_known == 0u && lighting_results[2].environment_sun_visibility == 0.35f
      && lighting_results[2].character_sun_multiplier == 1.f
      && lighting_results[3].sun_known == 0u && lighting_results[3].environment_sun_visibility == 0.65f
      && lighting_results[3].character_sun_multiplier == 1.f
      && lighting_results[4].sun_known == 0u && lighting_results[4].environment_sun_visibility == 0.2f
      && lighting_results[53].sun_known == 1u && std::abs(lighting_results[53].environment_sun_visibility - (1.f - 51.f / 255.f)) < 1e-6f
      && lighting_results[54].sun_known == 1u && std::abs(lighting_results[54].environment_sun_visibility - 0.25f) < 1e-6f
      && lighting_results[55].sun_known == 1u && lighting_results[55].environment_sun_visibility == 0.f
      && lighting_results[56].sun_known == 0u && lighting_results[56].environment_sun_visibility == 0.4f
      && lighting_results[56].character_sun_multiplier == 1.f
      && lighting_results[9].denoised_used == 1u && std::abs(lighting_results[9].diffuse_replaced - 0.125f) < 1e-6f
      && std::abs(lighting_results[9].shaded_local_term - 0.015625f) < 1e-6f
      && lighting_results[9].specular_preserved == 5.0f
      && lighting_results[10].denoised_used == 1u && std::abs(lighting_results[10].diffuse_replaced - 0.375f) < 1e-6f
      && std::abs(lighting_results[10].shaded_local_term - 0.046875f) < 1e-6f
      && lighting_results[10].specular_preserved == 7.0f
      && lighting_results[11].denoised_used == 0u && lighting_results[11].diffuse_replaced == 1.0f
      && lighting_results[12].denoised_used == 0u && lighting_results[12].diffuse_replaced == 1.0f
      && lighting_results[13].denoised_used == 0u && lighting_results[13].diffuse_replaced == 1.0f
      && lighting_results[14].denoised_used == 0u && lighting_results[14].diffuse_replaced == 1.0f
      && lighting_results[15].mode == 2u && lighting_results[15].denoised_used == 1u
      && std::abs(lighting_results[15].diffuse_replaced - 0.25f) < 1e-6f
      && lighting_results[16].denoised_used == 1u && std::abs(lighting_results[16].diffuse_replaced - 0.625f) < 1e-6f
      && lighting_results[16].specular_preserved == 0.0f
      && std::abs(lighting_results[17].debug_color[0] - 0.005f) < 1e-6f
      && std::abs(lighting_results[17].debug_color[1] - 0.05f) < 1e-6f
      && std::abs(lighting_results[18].debug_color[0] - 0.05f) < 1e-6f
      && std::abs(lighting_results[18].debug_color[1] - 0.005f) < 1e-6f
      && std::abs(lighting_results[19].debug_color[0] - 0.05f) < 1e-6f
      && std::abs(lighting_results[19].debug_color[1] - 0.04f) < 1e-6f
      && std::abs(lighting_results[20].debug_color[2] - 0.05f) < 1e-6f
      && std::abs(lighting_results[21].debug_color[0] - 0.05f) < 1e-6f
      && std::abs(lighting_results[22].debug_color[0] - 0.05f) < 1e-6f
      && std::abs(lighting_results[22].debug_color[1] - 0.02f) < 1e-6f
      && std::abs(lighting_results[23].debug_color[1] - 0.04f) < 1e-6f
      && std::abs(lighting_results[24].debug_color[1] - 0.8f * selected_debug_brightness * 0.05f) < 1e-6f
      && std::abs(lighting_results[24].debug_color[2] - selected_debug_brightness * 0.05f) < 1e-6f
      && lighting_results[25].debug_color[0] == 0.f && lighting_results[25].debug_color[1] == 0.f
      && lighting_results[25].debug_color[2] == 0.f
      && std::abs(lighting_results[26].debug_color[0] - 0.0025f) < 1e-6f
      && std::abs(lighting_results[27].debug_color[0]) < 1e-6f
      && std::abs(lighting_results[28].debug_color[0] - 0.05f) < 1e-6f
      && std::abs(lighting_results[29].debug_color[1] - 0.05f) < 1e-6f
      && std::abs(lighting_results[30].debug_color[2] - 0.05f) < 1e-6f
      && std::abs(lighting_results[31].debug_color[0] - 0.05f) < 1e-6f
      && std::abs(lighting_results[32].debug_color[0] - 0.05f) < 1e-6f
      && std::abs(lighting_results[33].debug_color[0] - 0.05f) < 1e-6f
      && lighting_results[34].debug_color[0] == 0.f && lighting_results[34].debug_color[1] == 0.f
      && lighting_results[34].debug_color[2] == 0.f
      && std::abs(lighting_results[35].debug_color[0] - 0.05f) < 1e-6f
      && std::abs(lighting_results[35].debug_color[1] - 0.04f) < 1e-6f
      && std::abs(lighting_results[42].debug_color[0] - 0.05f) < 1e-6f && std::abs(lighting_results[42].debug_color[1] - 0.05f) < 1e-6f
      && std::abs(lighting_results[42].debug_color[2] - 0.05f) < 1e-6f
      && std::abs(lighting_results[43].debug_color[0] - 0.05f) < 1e-6f && lighting_results[43].debug_color[1] == 0.0f
      && std::abs(lighting_results[43].debug_color[2] - 0.05f) < 1e-6f
      && std::abs(lighting_results[44].debug_color[0] - 0.05f) < 1e-6f && std::abs(lighting_results[44].debug_color[1] - 0.02f) < 1e-6f
      && std::abs(lighting_results[45].debug_color[0] - 0.05f) < 1e-6f && std::abs(lighting_results[45].debug_color[1] - 0.005f) < 1e-6f
      && std::abs(lighting_results[46].debug_color[1] - 0.02f) < 1e-6f && std::abs(lighting_results[46].debug_color[2] - 0.05f) < 1e-6f
      && std::abs(lighting_results[47].debug_color[0] - 0.05f) < 1e-6f && std::abs(lighting_results[47].debug_color[1] - 0.04f) < 1e-6f
      && std::abs(lighting_results[48].debug_color[0] - 0.03f) < 1e-6f && std::abs(lighting_results[48].debug_color[2] - 0.05f) < 1e-6f
      && std::abs(lighting_results[49].debug_color[0] - 0.05f) < 1e-6f && std::abs(lighting_results[49].debug_color[1] - 0.04f) < 1e-6f
      && std::abs(lighting_results[50].debug_color[0]
                  - std::log2(4.25f) / std::log2(65505.0f) * 0.05f) < 1e-6f
      && lighting_results[50].debug_color[0] == lighting_results[50].debug_color[1]
      && lighting_results[50].debug_color[1] == lighting_results[50].debug_color[2]
      && std::abs(lighting_results[51].debug_color[0] - 0.05f) < 1e-6f && std::abs(lighting_results[51].debug_color[2] - 0.05f) < 1e-6f
      && lighting_results[52].debug_color[0] == 0.0f && lighting_results[52].debug_color[1] == 0.0f
      && lighting_results[52].debug_color[2] == 0.0f
      && lighting_results[29].debug_packed_distance[0] == 3.25f
      && lighting_results[29].debug_packed_distance[1] == 1.0f
      && lighting_results[29].debug_packed_distance[3] == 1.0f
      && lighting_results[30].debug_packed_distance[0] == 65504.0f
      && lighting_results[30].debug_packed_distance[1] == 5.0f
      && lighting_results[30].debug_packed_distance[3] == 1.0f
      && lighting_results[31].debug_packed_distance[0] == 0.0f
      && lighting_results[31].debug_packed_distance[1] == 0.0f
      && lighting_results[31].debug_packed_distance[3] == 1.0f
      && lighting_results[32].debug_packed_distance[1] == 0.0f
      && lighting_results[36].debug_packed_distance[0] == 0.0f
      && lighting_results[37].debug_packed_distance[0] == 0.0f
      && lighting_results[38].debug_packed_distance[0] == 0.0f
      && lighting_results[39].debug_packed_distance[0] == 0.0f
      && lighting_results[40].debug_packed_distance[1] == 3.0f
      && lighting_results[40].debug_packed_distance[0] == 3.25f
      && lighting_results[41].debug_packed_distance[1] == 3.0f
      && lighting_results[41].debug_packed_distance[0] == 3.25f;
  const bool character_contract_ok =
      lighting_results[0].character_mode == 1u && lighting_results[0].character_type == RTXDI_DI_LIGHT_TYPE_POINT
      && std::abs(lighting_results[0].character_term - 0.375f) < 1e-6f
      && lighting_results[1].character_mode == 1u && lighting_results[1].character_type == RTXDI_DI_LIGHT_TYPE_SPOT
      && std::abs(lighting_results[1].character_term - 0.5625f) < 1e-6f
      && lighting_results[2].character_mode == 2u && lighting_results[2].character_term == 0.f
      && lighting_results[3].character_mode == 3u && std::abs(lighting_results[3].character_term - 0.25f) < 1e-6f
      && lighting_results[4].character_mode == 3u && lighting_results[5].character_mode == 3u
      && lighting_results[6].character_mode == 3u && lighting_results[7].character_mode == 3u
      && lighting_results[8].character_mode == 0u && lighting_results[8].character_term == 0.f;
  lighting_readback->Unmap(0u, nullptr);
  if (!lighting_contract_ok || !character_contract_ok) {
    std::fprintf(stderr, "DIAG lighting_ok=%u character_ok=%u\n", lighting_contract_ok ? 1u : 0u,
                 character_contract_ok ? 1u : 0u);
    std::fprintf(stderr, "FAIL lighting consumer GPU contract point=(%u,%u,%g,%g) spot=(%u,%u,%g,%g) fallback=%u/%u blocked=%u\n",
                 lighting_results[0].point_count, lighting_results[0].spot_count, lighting_results[0].raw_local_term,
                 lighting_results[0].shaded_local_term, lighting_results[1].point_count, lighting_results[1].spot_count,
                 lighting_results[1].raw_local_term, lighting_results[1].shaded_local_term,
                 lighting_results[3].point_count, lighting_results[3].spot_count, lighting_results[2].mode);
    for (uint32_t i = 42u; i < lighting_case_count; ++i) {
      std::fprintf(stderr, "DETAIL[%u]=%g,%g,%g,%g\n", i, lighting_results[i].debug_color[0],
                   lighting_results[i].debug_color[1], lighting_results[i].debug_color[2],
                   lighting_results[i].debug_color[3]);
    }
    return 1;
  }
  std::printf("PASS RTXDI debug palette/carrier contract: status/class/type/raw/filtered/distance/sun and unavailable checker\n");
  std::printf("PASS production lighting selection helper point/spot/blocked/unknown/unbound/invalid fallback; post-material application once\n");
  std::printf("PASS character local DI point/spot charaColor, blocked suppression, unknown/empty/invalid RTLS fallback without double-add\n");
  std::printf("PASS RTXDI sun consumer helper visible/blocked/unknown/unbound/malformed; CSM and character fallback preserved\n");
  std::printf("PASS N3c shared lighting consumer visible/blocked/class, stale/unbound/malformed raw fallback, base/intensity once, specular preserved\n");

  constexpr UINT hitdistance_pixels = 6u;
  constexpr UINT hitdistance_elements = hitdistance_pixels + 1u;
  D3D12_DESCRIPTOR_RANGE hitdistance_ranges[6] = {};
  hitdistance_ranges[0] = {D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4u, 0u, 0u, 0u};
  hitdistance_ranges[1] = {D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3u, 14u, 0u, 4u};
  hitdistance_ranges[2] = {D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1u, 29u, 0u, 7u};
  hitdistance_ranges[3] = {D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1u, 30u, 0u, 8u};
  hitdistance_ranges[4] = {D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4u, 31u, 0u, 9u};
  hitdistance_ranges[5] = {D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 2u, 0u, 0u, 13u};
  ComPtr<ID3D12RootSignature> hitdistance_root;
  ComPtr<ID3D12PipelineState> hitdistance_pso;
  if (!CreateRoot(device.Get(), hitdistance_ranges, 6u, 12u, &hitdistance_root)
      || !CreatePso(device.Get(), hitdistance_root.Get(), __world_rtxdi_nrd_hitdistance, &hitdistance_pso)) return 1;

  std::array<uint32_t, hitdistance_pixels> hitdistance_status = {9u, 10u, 3u, 25u, 9u, 12u};
  for (UINT pixel = 0u; pixel < hitdistance_pixels; ++pixel) normal_pixels[pixel * 4u + 3u] = (pixel == 3u ? 0u : 8u);
  depths.fill(0.5f);
  depths[4] = 0.0f;
  wait_context = "N3b synthetic guide upload";
  if (!Check(allocator->Reset(), "reset N3b guide allocator")
      || !Check(base_list->Reset(allocator.Get(), nullptr), "reset N3b guide list")
      || !update_pixel_texture(normal_tex.Get(), normal_upload.Get(), normal_pixels.data(), 3u * 16u)
      || !update_pixel_texture(depth_tex.Get(), depth_upload.Get(), depths.data(), 3u * sizeof(float))
      || !update_pixel_texture(scene_tex.Get(), scene_upload.Get(), scene_pixels.data(),
                               RTXDI_SURFACE_SCENE_ROW_COUNT * 4u * sizeof(uint32_t))) return 1;
  ComPtr<ID3D12Resource> hitdistance_status_texture, hitdistance_status_upload;
  if (!CreateTextureUpload(device.Get(), list.Get(), DXGI_FORMAT_R32_UINT, 3u, 2u, hitdistance_status.data(),
                           3u * sizeof(uint32_t), &hitdistance_status_texture, &hitdistance_status_upload)) return 1;
  if (!submit_wait()) return 1;
  if (!Check(allocator->Reset(), "reset N3b dispatch allocator")
      || !Check(base_list->Reset(allocator.Get(), nullptr), "reset N3b dispatch list")) return 1;

  D3D12_DESCRIPTOR_HEAP_DESC hitdistance_heap_desc = {};
  hitdistance_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
  hitdistance_heap_desc.NumDescriptors = 15u;
  hitdistance_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
  ComPtr<ID3D12DescriptorHeap> hitdistance_heap;
  if (!Check(device->CreateDescriptorHeap(&hitdistance_heap_desc, IID_PPV_ARGS(&hitdistance_heap)),
             "create N3b descriptor heap")) return 1;
  const UINT hitdistance_descriptor_size = device->GetDescriptorHandleIncrementSize(hitdistance_heap_desc.Type);
  const auto hitdistance_cpu = hitdistance_heap->GetCPUDescriptorHandleForHeapStart();
  const auto hitdistance_cpu_at = [&](UINT index) {
    return D3D12_CPU_DESCRIPTOR_HANDLE{hitdistance_cpu.ptr + static_cast<SIZE_T>(index) * hitdistance_descriptor_size};
  };
  CreateBufferSrv(device.Get(), hitdistance_heap.Get(), hitdistance_descriptor_size, 0u, vertices.Get(), 3u, 16u);
  CreateBufferSrv(device.Get(), hitdistance_heap.Get(), hitdistance_descriptor_size, 1u, indices.Get(), 3u, sizeof(uint32_t));
  CreateBufferSrv(device.Get(), hitdistance_heap.Get(), hitdistance_descriptor_size, 2u, meshes.Get(), 1u, sizeof(WorldMeshGPU));
  CreateBufferSrv(device.Get(), hitdistance_heap.Get(), hitdistance_descriptor_size, 3u, instances.Get(), 2u, sizeof(WorldInstanceGPU));
  CreateBufferSrv(device.Get(), hitdistance_heap.Get(), hitdistance_descriptor_size, 4u, uvs.Get(), 3u, 2u * sizeof(float));
  D3D12_SHADER_RESOURCE_VIEW_DESC hitdistance_atlas_srv = {};
  hitdistance_atlas_srv.Format = DXGI_FORMAT_R32_UINT;
  hitdistance_atlas_srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
  hitdistance_atlas_srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  hitdistance_atlas_srv.Texture2DArray.MipLevels = 1u;
  hitdistance_atlas_srv.Texture2DArray.ArraySize = 256u;
  device->CreateShaderResourceView(atlas.Get(), &hitdistance_atlas_srv, hitdistance_cpu_at(5u));
  CreateBufferSrv(device.Get(), hitdistance_heap.Get(), hitdistance_descriptor_size, 6u, alpha_materials.Get(), 1u,
                  sizeof(AlphaMaterialGPU));
  D3D12_SHADER_RESOURCE_VIEW_DESC hitdistance_tlas_srv = {};
  hitdistance_tlas_srv.Format = DXGI_FORMAT_UNKNOWN;
  hitdistance_tlas_srv.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
  hitdistance_tlas_srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  hitdistance_tlas_srv.RaytracingAccelerationStructure.Location = tlases[0]->GetGPUVirtualAddress();
  device->CreateShaderResourceView(nullptr, &hitdistance_tlas_srv, hitdistance_cpu_at(7u));
  CreateBufferSrv(device.Get(), hitdistance_heap.Get(), hitdistance_descriptor_size, 8u, dynamic_vertices.Get(), 3u, 16u);
  D3D12_SHADER_RESOURCE_VIEW_DESC hitdistance_depth_srv = {};
  hitdistance_depth_srv.Format = DXGI_FORMAT_R32_FLOAT;
  hitdistance_depth_srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
  hitdistance_depth_srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  hitdistance_depth_srv.Texture2D.MipLevels = 1u;
  device->CreateShaderResourceView(depth_tex.Get(), &hitdistance_depth_srv, hitdistance_cpu_at(9u));
  device->CreateShaderResourceView(normal_tex.Get(), &uint4_srv, hitdistance_cpu_at(10u));
  device->CreateShaderResourceView(scene_tex.Get(), &uint4_srv, hitdistance_cpu_at(11u));
  D3D12_SHADER_RESOURCE_VIEW_DESC hitdistance_status_srv = {};
  hitdistance_status_srv.Format = DXGI_FORMAT_R32_UINT;
  hitdistance_status_srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
  hitdistance_status_srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  hitdistance_status_srv.Texture2D.MipLevels = 1u;
  device->CreateShaderResourceView(hitdistance_status_texture.Get(), &hitdistance_status_srv, hitdistance_cpu_at(12u));

  ComPtr<ID3D12Resource> hitdistance_output, hitvalidity_output;
  ComPtr<ID3D12Resource> hitdistance_readback, hitvalidity_readback, hitdistance_sentinel_upload, hitvalidity_sentinel_upload;
  const auto hitdistance_readback_desc = BufferDesc(hitdistance_elements * sizeof(float));
  const auto hitvalidity_readback_desc = BufferDesc(hitdistance_elements * sizeof(uint32_t));
  const auto readback_heap = Heap(D3D12_HEAP_TYPE_READBACK);
  std::array<float, hitdistance_elements> hitdistance_sentinel = {};
  hitdistance_sentinel.fill(std::bit_cast<float>(0x7FC0DEADu));
  std::array<uint32_t, hitdistance_elements> hitvalidity_sentinel = {};
  hitvalidity_sentinel.fill(0xDEADBEEFu);
  if (!CreateDefaultBuffer(device.Get(), hitdistance_elements * sizeof(float), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                           D3D12_RESOURCE_STATE_COMMON, &hitdistance_output)
      || !CreateDefaultBuffer(device.Get(), hitdistance_elements * sizeof(uint32_t), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                              D3D12_RESOURCE_STATE_COMMON, &hitvalidity_output)
      || !UploadBuffer(device.Get(), hitdistance_sentinel.data(), sizeof(hitdistance_sentinel), &hitdistance_sentinel_upload)
      || !UploadBuffer(device.Get(), hitvalidity_sentinel.data(), sizeof(hitvalidity_sentinel), &hitvalidity_sentinel_upload)
      || !Check(device->CreateCommittedResource(&readback_heap, D3D12_HEAP_FLAG_NONE, &hitdistance_readback_desc,
                                                D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                                IID_PPV_ARGS(&hitdistance_readback)),
                "create N3b distance readback")
      || !Check(device->CreateCommittedResource(&readback_heap, D3D12_HEAP_FLAG_NONE, &hitvalidity_readback_desc,
                                                D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                                IID_PPV_ARGS(&hitvalidity_readback)),
                "create N3b mask readback")) return 1;
  D3D12_UNORDERED_ACCESS_VIEW_DESC hitdistance_uav = {};
  hitdistance_uav.Format = DXGI_FORMAT_UNKNOWN;
  hitdistance_uav.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
  hitdistance_uav.Buffer.NumElements = hitdistance_elements;
  hitdistance_uav.Buffer.StructureByteStride = sizeof(float);
  device->CreateUnorderedAccessView(hitdistance_output.Get(), nullptr, &hitdistance_uav, hitdistance_cpu_at(13u));
  hitdistance_uav.Buffer.StructureByteStride = sizeof(uint32_t);
  device->CreateUnorderedAccessView(hitvalidity_output.Get(), nullptr, &hitdistance_uav, hitdistance_cpu_at(14u));

  std::array<float, 3u> hitdistance_coverage_min = {-100000.0f, -100000.0f, -100000.0f};
  float hitdistance_coverage_size = 200000.0f;
  constexpr uint32_t hitdistance_frame = 73u;

  const auto run_hitdistance = [&](UINT mask_index, bool scene_valid, bool coverage_complete, const char* label,
                                   std::array<float, hitdistance_pixels>* distances,
                                   std::array<uint32_t, hitdistance_pixels>* validity) {
    if (mask_index >= tlases.size() || distances == nullptr || validity == nullptr) return false;
    wait_context = label;
    hitdistance_tlas_srv.RaytracingAccelerationStructure.Location = tlases[mask_index]->GetGPUVirtualAddress();
    device->CreateShaderResourceView(nullptr, &hitdistance_tlas_srv, hitdistance_cpu_at(7u));
    D3D12_RESOURCE_BARRIER barriers[4] = {};
    for (UINT index = 0u; index < 2u; ++index) {
      barriers[index].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
      barriers[index].Transition.pResource = index == 0u ? hitdistance_output.Get() : hitvalidity_output.Get();
      barriers[index].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
      barriers[index].Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
      barriers[index].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
    }
    list->ResourceBarrier(2u, barriers);
    list->CopyBufferRegion(hitdistance_output.Get(), 0u, hitdistance_sentinel_upload.Get(), 0u, sizeof(hitdistance_sentinel));
    list->CopyBufferRegion(hitvalidity_output.Get(), 0u, hitvalidity_sentinel_upload.Get(), 0u, sizeof(hitvalidity_sentinel));
    for (UINT index = 0u; index < 2u; ++index) {
      barriers[index].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
      barriers[index].Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    }
    list->ResourceBarrier(2u, barriers);
    ID3D12DescriptorHeap* heaps[] = {hitdistance_heap.Get()};
    list->SetDescriptorHeaps(1u, heaps);
    list->SetComputeRootSignature(hitdistance_root.Get());
    list->SetPipelineState(hitdistance_pso.Get());
    const std::array<uint32_t, 12u> params = {
        3u, 2u, hitdistance_frame, scene_valid ? 1u : 0u, coverage_complete ? 1u : 0u,
        std::bit_cast<uint32_t>(0.01f), std::bit_cast<uint32_t>(0.02f), std::bit_cast<uint32_t>(65504.0f),
        std::bit_cast<uint32_t>(hitdistance_coverage_min[0]),
        std::bit_cast<uint32_t>(hitdistance_coverage_min[1]),
        std::bit_cast<uint32_t>(hitdistance_coverage_min[2]),
        std::bit_cast<uint32_t>(hitdistance_coverage_size)};
    list->SetComputeRoot32BitConstants(0u, static_cast<UINT>(params.size()), params.data(), 0u);
    list->SetComputeRootDescriptorTable(1u, hitdistance_heap->GetGPUDescriptorHandleForHeapStart());
    list->Dispatch(1u, 1u, 1u);
    barriers[0] = {};
    barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barriers[0].UAV.pResource = hitdistance_output.Get();
    barriers[1] = {};
    barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barriers[1].UAV.pResource = hitvalidity_output.Get();
    list->ResourceBarrier(2u, barriers);
    for (UINT index = 0u; index < 2u; ++index) {
      barriers[index] = {};
      barriers[index].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
      barriers[index].Transition.pResource = index == 0u ? hitdistance_output.Get() : hitvalidity_output.Get();
      barriers[index].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
      barriers[index].Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
      barriers[index].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
    }
    list->ResourceBarrier(2u, barriers);
    list->CopyBufferRegion(hitdistance_readback.Get(), 0u, hitdistance_output.Get(), 0u, sizeof(hitdistance_sentinel));
    list->CopyBufferRegion(hitvalidity_readback.Get(), 0u, hitvalidity_output.Get(), 0u, sizeof(hitvalidity_sentinel));
    for (UINT index = 0u; index < 2u; ++index) {
      barriers[index].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
      barriers[index].Transition.StateAfter = D3D12_RESOURCE_STATE_COMMON;
    }
    list->ResourceBarrier(2u, barriers);
    if (!submit_wait()) return false;
    void* distance_data = nullptr;
    void* validity_data = nullptr;
    if (!Check(hitdistance_readback->Map(0u, nullptr, &distance_data), "map N3b distances")
        || !Check(hitvalidity_readback->Map(0u, nullptr, &validity_data), "map N3b validity")) return false;
    const auto* all_distances = static_cast<const float*>(distance_data);
    const auto* all_validity = static_cast<const uint32_t*>(validity_data);
    bool ok = std::bit_cast<uint32_t>(all_distances[hitdistance_pixels]) == 0x7FC0DEADu
              && all_validity[hitdistance_pixels] == 0xDEADBEEFu;
    for (UINT pixel = 0u; pixel < hitdistance_pixels; ++pixel) {
      (*distances)[pixel] = all_distances[pixel];
      (*validity)[pixel] = all_validity[pixel];
      if (!std::isfinite(all_distances[pixel]) || all_distances[pixel] < 0.0f || all_distances[pixel] > 65504.0f) ok = false;
    }
    const D3D12_RANGE no_write = {0u, 0u};
    hitdistance_readback->Unmap(0u, &no_write);
    hitvalidity_readback->Unmap(0u, &no_write);
    if (!ok) std::fprintf(stderr, "FAIL %s N3b nonfinite/bounds/sentinel\n", label);
    return ok && Check(allocator->Reset(), "reset N3b allocator after wait")
           && Check(base_list->Reset(allocator.Get(), nullptr), "reset N3b list after wait");
  };

  std::array<float, hitdistance_pixels> reconstructed_distances = {};
  std::array<uint32_t, hitdistance_pixels> reconstructed_validity = {};
  if (!run_hitdistance(0u, true, true, "N3b closest opaque", &reconstructed_distances, &reconstructed_validity)) return 1;
  uint32_t eligible_pixels = 0u;
  uint32_t valid_distance_pixels = 0u;
  for (uint32_t pixel = 0u; pixel < hitdistance_pixels; ++pixel) {
    const uint32_t status = hitdistance_status[pixel] & 0x7u;
    const bool status_known = (hitdistance_status[pixel] & RTXDI_N1_STATUS_KNOWN_BIT) != 0u
                              && (status == RTXDI_N1_STATUS_VISIBLE || status == RTXDI_N1_STATUS_BLOCKED);
    const bool status_character = (hitdistance_status[pixel] & RTXDI_N1_STATUS_CHARACTER_BIT) != 0u;
    const bool surface_character = (normal_pixels[pixel * 4u + 3u] & 8u) == 0u;
    if (status_known && status_character == surface_character && std::isfinite(depths[pixel]) && depths[pixel] > 0.0f) {
      ++eligible_pixels;
    }
    if ((reconstructed_validity[pixel] & 1u) != 0u) ++valid_distance_pixels;
  }
  if (eligible_pixels != 3u || valid_distance_pixels != eligible_pixels) return 1;
  std::printf("PASS N3b fixture eligibility/ray bound eligible=%u maximum_rays=%u valid=%u invalid=%u\n",
              eligible_pixels, eligible_pixels, valid_distance_pixels, hitdistance_pixels - valid_distance_pixels);
  for (UINT pixel : {0u, 1u, 3u}) {
    if ((reconstructed_validity[pixel] & 1u) == 0u || reconstructed_distances[pixel] <= 0.0f) {
      std::fprintf(stderr, "FAIL N3b known visible/blocked/character pixel=%u mask=%u distance=%g\n",
                   pixel, reconstructed_validity[pixel], reconstructed_distances[pixel]);
      return 1;
    }
  }
  if (reconstructed_validity[2] != 0u || reconstructed_validity[4] != 0u || reconstructed_validity[5] != 0u
      || reconstructed_distances[2] != 0.0f || reconstructed_distances[4] != 0.0f || reconstructed_distances[5] != 0.0f) return 1;
  const float baseline_nearest = reconstructed_distances[0];
  if (!run_hitdistance(2u, true, true, "N3b shadow-only closest hit", &reconstructed_distances, &reconstructed_validity)
      || (reconstructed_validity[0] & 1u) == 0u
      || !run_hitdistance(3u, true, true, "N3b current dynamic closest hit", &reconstructed_distances, &reconstructed_validity)
      || (reconstructed_validity[0] & 1u) == 0u
      || !run_hitdistance(4u, true, true, "N3b certified scene miss", &reconstructed_distances, &reconstructed_validity)
      || reconstructed_distances[0] != 65504.0f || reconstructed_validity[0] != 5u
      || !run_hitdistance(0u, true, false, "N3b incomplete coverage invalid", &reconstructed_distances, &reconstructed_validity)
      || reconstructed_distances[0] != 0.0f || reconstructed_validity[0] != 0u
      || !run_hitdistance(0u, false, true, "N3b invalid scene invalid", &reconstructed_distances, &reconstructed_validity)
      || reconstructed_distances[0] != 0.0f || reconstructed_validity[0] != 0u) return 1;

  hitdistance_coverage_min = {-2.0f, -2.0f, -2.0f};
  hitdistance_coverage_size = 4.0f;
  if (!run_hitdistance(0u, true, true, "N3b hit within clipped coverage", &reconstructed_distances, &reconstructed_validity)
      || (reconstructed_validity[0] & 1u) == 0u || reconstructed_distances[0] <= 0.0f
      || reconstructed_distances[0] >= 4.0f) {
    std::fprintf(stderr, "FAIL N3b clipped hit distance=%g mask=%u\n", reconstructed_distances[0], reconstructed_validity[0]);
    return 1;
  }
  std::printf("PASS N3b short coverage retains interior opaque hit distance=%g\n", reconstructed_distances[0]);
  if (!run_hitdistance(4u, true, true, "N3b truncated empty scene is invalid",
                       &reconstructed_distances, &reconstructed_validity)
      || reconstructed_distances[0] != 0.0f || reconstructed_validity[0] != 0u) {
    std::fprintf(stderr, "FAIL N3b truncated empty distance=%g mask=%u\n", reconstructed_distances[0], reconstructed_validity[0]);
    return 1;
  }
  std::printf("PASS N3b truncated empty scene stays invalid\n");
  hitdistance_coverage_min = {1000.0f, 1000.0f, 1000.0f};
  hitdistance_coverage_size = 10.0f;
  if (!run_hitdistance(0u, true, true, "N3b ray origin outside coverage is invalid",
                       &reconstructed_distances, &reconstructed_validity)
      || reconstructed_distances[0] != 0.0f || reconstructed_validity[0] != 0u) {
    std::fprintf(stderr, "FAIL N3b outside coverage distance=%g mask=%u\n", reconstructed_distances[0], reconstructed_validity[0]);
    return 1;
  }
  std::printf("PASS N3b ray origin outside coverage invalid\n");
  hitdistance_coverage_min = {-100000.0f, -100000.0f, -100000.0f};
  hitdistance_coverage_size = 0.0f;
  if (!run_hitdistance(0u, true, true, "N3b zero-size coverage is invalid",
                       &reconstructed_distances, &reconstructed_validity)
      || reconstructed_distances[0] != 0.0f || reconstructed_validity[0] != 0u) {
    std::fprintf(stderr, "FAIL N3b zero coverage distance=%g mask=%u\n", reconstructed_distances[0], reconstructed_validity[0]);
    return 1;
  }
  std::printf("PASS N3b zero-size coverage invalid\n");
  hitdistance_coverage_size = std::numeric_limits<float>::quiet_NaN();
  if (!run_hitdistance(0u, true, true, "N3b nonfinite coverage is invalid",
                       &reconstructed_distances, &reconstructed_validity)
      || reconstructed_distances[0] != 0.0f || reconstructed_validity[0] != 0u) {
    std::fprintf(stderr, "FAIL N3b nonfinite coverage distance=%g mask=%u\n", reconstructed_distances[0], reconstructed_validity[0]);
    return 1;
  }
  std::printf("PASS N3b nonfinite coverage invalid\n");
  hitdistance_coverage_min = {-100000.0f, -100000.0f, -100000.0f};
  hitdistance_coverage_size = 200000.0f;

  hitdistance_coverage_min = {-4.04427f, -0.24334f, -1.38928f};
  hitdistance_coverage_size = 4.0f;
  if (!run_hitdistance(0u, true, true, "N3b near opaque hit inside clipped coverage",
                       &reconstructed_distances, &reconstructed_validity)
      || reconstructed_validity[0] != 1u || std::abs(reconstructed_distances[0] - baseline_nearest) > 1.0e-4f) {
    std::fprintf(stderr, "FAIL N3b clipped opaque hit distance=%g mask=%u baseline=%g\n",
                 reconstructed_distances[0], reconstructed_validity[0], baseline_nearest);
    return 1;
  }
  std::printf("PASS N3b clipped opaque nearest distance=%g\n", reconstructed_distances[0]);
  mesh.bbox_min[3] = 1.0f;
  world_instances[0].header[2] = 1.0f;
  if (!update_upload(meshes.Get(), &mesh, sizeof(mesh))
      || !update_upload(instances.Get(), world_instances.data(), sizeof(world_instances))
      || !update_atlas_texel(0xFFFFFFFFu)) return 1;
  if (!update_atlas_texel(0x00000000u)
      || !run_hitdistance(0u, true, true, "N3b alpha-rejected near with farther candidate outside coverage",
                          &reconstructed_distances, &reconstructed_validity)
      || reconstructed_distances[0] != 0.0f || reconstructed_validity[0] != 0u) {
    std::fprintf(stderr, "FAIL N3b alpha edge coverage min=(-4.04427,-0.24334,-1.38928) size=4 distance=%g mask=%u\n",
                 reconstructed_distances[0], reconstructed_validity[0]);
    return 1;
  }
  std::printf("PASS N3b rejected near alpha with farther geometry beyond AABB is invalid min=(-4.04427,-0.24334,-1.38928) size=4\n");
  hitdistance_coverage_min = {-100000.0f, -100000.0f, -100000.0f};
  hitdistance_coverage_size = 200000.0f;
  if (!update_atlas_texel(0x00000000u)) return 1;
  if (!run_hitdistance(0u, true, true, "N3b alpha-cut nearest candidate continues to farther hit",
                       &reconstructed_distances, &reconstructed_validity)
      || reconstructed_validity[0] != 1u || reconstructed_distances[0] <= baseline_nearest) return 1;
  world_instances[1].header[2] = 1.0f;
  if (!update_upload(instances.Get(), world_instances.data(), sizeof(world_instances))
      || !run_hitdistance(0u, true, true, "N3b all alpha candidates cut certify miss",
                          &reconstructed_distances, &reconstructed_validity)
      || reconstructed_distances[0] != 65504.0f || reconstructed_validity[0] != 5u) return 1;
  world_instances[0].header[2] = 0.0f;
  world_instances[1].header[2] = 0.0f;
  mesh.bbox_min[3] = 0.0f;
  if (!update_upload(meshes.Get(), &mesh, sizeof(mesh))
      || !update_upload(instances.Get(), world_instances.data(), sizeof(world_instances))
      || !update_atlas_texel(0xFFFFFFFFu)) return 1;
  std::printf("PASS N3b first-hit distance/mask: closest alpha-aware DXR, opaque/shadow-only/dynamic masks, certified miss vs unknown coverage, invalid receiver/scene, character, guard, light-independent sampling\n");

  constexpr UINT cosine_sample_count = 8192u;
  constexpr UINT cosine_sample_bytes = cosine_sample_count * 4u * sizeof(float);
  ComPtr<ID3D12RootSignature> cosine_root;
  ComPtr<ID3D12PipelineState> cosine_pso;
  D3D12_DESCRIPTOR_RANGE cosine_range = {D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1u, 0u, 0u, 0u};
  if (!CreateRoot(device.Get(), &cosine_range, 1u, 2u, &cosine_root)
      || !CreatePso(device.Get(), cosine_root.Get(), __world_rtxdi_nrd_sampling_contract, &cosine_pso)) return 1;
  ComPtr<ID3D12Resource> cosine_output, cosine_readback;
  if (!CreateDefaultBuffer(device.Get(), cosine_sample_bytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                           D3D12_RESOURCE_STATE_COMMON, &cosine_output)) return 1;
  const auto cosine_readback_desc = BufferDesc(cosine_sample_bytes);
  const auto cosine_readback_heap = Heap(D3D12_HEAP_TYPE_READBACK);
  if (!Check(device->CreateCommittedResource(&cosine_readback_heap, D3D12_HEAP_FLAG_NONE, &cosine_readback_desc,
                                             D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                             IID_PPV_ARGS(&cosine_readback)),
             "create cosine sample readback")) return 1;
  D3D12_DESCRIPTOR_HEAP_DESC cosine_heap_desc = {};
  cosine_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
  cosine_heap_desc.NumDescriptors = 1u;
  cosine_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
  ComPtr<ID3D12DescriptorHeap> cosine_heap;
  if (!Check(device->CreateDescriptorHeap(&cosine_heap_desc, IID_PPV_ARGS(&cosine_heap)), "create cosine sample heap")) return 1;
  D3D12_UNORDERED_ACCESS_VIEW_DESC cosine_uav = {};
  cosine_uav.Format = DXGI_FORMAT_UNKNOWN;
  cosine_uav.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
  cosine_uav.Buffer.NumElements = cosine_sample_count;
  cosine_uav.Buffer.StructureByteStride = 4u * sizeof(float);
  device->CreateUnorderedAccessView(cosine_output.Get(), nullptr, &cosine_uav, cosine_heap->GetCPUDescriptorHandleForHeapStart());
  D3D12_RESOURCE_BARRIER cosine_barrier = {};
  cosine_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  cosine_barrier.Transition.pResource = cosine_output.Get();
  cosine_barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  cosine_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
  cosine_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
  list->ResourceBarrier(1u, &cosine_barrier);
  ID3D12DescriptorHeap* cosine_heaps[] = {cosine_heap.Get()};
  list->SetDescriptorHeaps(1u, cosine_heaps);
  list->SetComputeRootSignature(cosine_root.Get());
  list->SetPipelineState(cosine_pso.Get());
  const std::array<uint32_t, 2u> cosine_params = {cosine_sample_count, 73u};
  list->SetComputeRoot32BitConstants(0u, static_cast<UINT>(cosine_params.size()), cosine_params.data(), 0u);
  list->SetComputeRootDescriptorTable(1u, cosine_heap->GetGPUDescriptorHandleForHeapStart());
  list->Dispatch((cosine_sample_count + 63u) / 64u, 1u, 1u);
  D3D12_RESOURCE_BARRIER cosine_uav_barrier = {};
  cosine_uav_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
  cosine_uav_barrier.UAV.pResource = cosine_output.Get();
  list->ResourceBarrier(1u, &cosine_uav_barrier);
  cosine_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
  cosine_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
  list->ResourceBarrier(1u, &cosine_barrier);
  list->CopyBufferRegion(cosine_readback.Get(), 0u, cosine_output.Get(), 0u, cosine_sample_bytes);
  cosine_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
  cosine_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COMMON;
  list->ResourceBarrier(1u, &cosine_barrier);
  wait_context = "N3b cosine distribution moments";
  if (!submit_wait()) return 1;
  void* cosine_mapped = nullptr;
  if (!Check(cosine_readback->Map(0u, nullptr, &cosine_mapped), "map cosine sample directions")) return 1;
  const auto* cosine_samples = static_cast<const float*>(cosine_mapped);
  double mean_x = 0.0, mean_y = 0.0, mean_ndot = 0.0, mean_ndot_squared = 0.0;
  bool cosine_finite = true;
  for (UINT index = 0u; index < cosine_sample_count; ++index) {
    const float x = cosine_samples[index * 4u];
    const float y = cosine_samples[index * 4u + 1u];
    const float z = cosine_samples[index * 4u + 2u];
    const float ndot = -z;
    cosine_finite = cosine_finite && std::isfinite(x) && std::isfinite(y) && std::isfinite(z)
                    && std::abs(x * x + y * y + z * z - 1.0f) < 1.0e-3f && ndot >= 0.0f;
    mean_x += x;
    mean_y += y;
    mean_ndot += ndot;
    mean_ndot_squared += ndot * ndot;
  }
  cosine_readback->Unmap(0u, nullptr);
  mean_x /= cosine_sample_count;
  mean_y /= cosine_sample_count;
  mean_ndot /= cosine_sample_count;
  mean_ndot_squared /= cosine_sample_count;
  if (!cosine_finite || std::abs(mean_x) > 0.025 || std::abs(mean_y) > 0.025
      || std::abs(mean_ndot - 2.0 / 3.0) > 0.015 || std::abs(mean_ndot_squared - 0.5) > 0.015) {
    std::fprintf(stderr, "FAIL N3b cosine moments x=%g y=%g ndot=%g ndot2=%g\n",
                 mean_x, mean_y, mean_ndot, mean_ndot_squared);
    return 1;
  }
  std::printf("PASS N3b deterministic cosine GPU moments samples=%u meanNdot=%g meanNdot2=%g\n",
              cosine_sample_count, mean_ndot, mean_ndot_squared);

  constexpr UINT coverage_case_count = 4u;
  constexpr UINT coverage_bytes = coverage_case_count * 2u * sizeof(float);
  ComPtr<ID3D12RootSignature> coverage_root;
  ComPtr<ID3D12PipelineState> coverage_pso;
  D3D12_DESCRIPTOR_RANGE coverage_range = {D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1u, 0u, 0u, 0u};
  if (!CreateRoot(device.Get(), &coverage_range, 1u, 1u, &coverage_root)
      || !CreatePso(device.Get(), coverage_root.Get(), __world_rtxdi_nrd_coverage_contract, &coverage_pso)) return 1;
  ComPtr<ID3D12Resource> coverage_output, coverage_readback;
  if (!CreateDefaultBuffer(device.Get(), coverage_bytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                           D3D12_RESOURCE_STATE_COMMON, &coverage_output)) return 1;
  const auto coverage_readback_desc = BufferDesc(coverage_bytes);
  const auto coverage_readback_heap = Heap(D3D12_HEAP_TYPE_READBACK);
  if (!Check(device->CreateCommittedResource(&coverage_readback_heap, D3D12_HEAP_FLAG_NONE, &coverage_readback_desc,
                                             D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                             IID_PPV_ARGS(&coverage_readback)),
             "create coverage contract readback")) return 1;
  D3D12_DESCRIPTOR_HEAP_DESC coverage_heap_desc = {};
  coverage_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
  coverage_heap_desc.NumDescriptors = 1u;
  coverage_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
  ComPtr<ID3D12DescriptorHeap> coverage_heap;
  if (!Check(device->CreateDescriptorHeap(&coverage_heap_desc, IID_PPV_ARGS(&coverage_heap)),
             "create coverage contract heap")) return 1;
  D3D12_UNORDERED_ACCESS_VIEW_DESC coverage_uav = {};
  coverage_uav.Format = DXGI_FORMAT_UNKNOWN;
  coverage_uav.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
  coverage_uav.Buffer.NumElements = coverage_case_count;
  coverage_uav.Buffer.StructureByteStride = 2u * sizeof(float);
  device->CreateUnorderedAccessView(coverage_output.Get(), nullptr, &coverage_uav,
                                    coverage_heap->GetCPUDescriptorHandleForHeapStart());
  if (!Check(allocator->Reset(), "reset coverage contract allocator")
      || !Check(base_list->Reset(allocator.Get(), nullptr), "reset coverage contract command list")) return 1;
  D3D12_RESOURCE_BARRIER coverage_barrier = {};
  coverage_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  coverage_barrier.Transition.pResource = coverage_output.Get();
  coverage_barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  coverage_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
  coverage_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
  list->ResourceBarrier(1u, &coverage_barrier);
  ID3D12DescriptorHeap* coverage_heaps[] = {coverage_heap.Get()};
  list->SetDescriptorHeaps(1u, coverage_heaps);
  list->SetComputeRootSignature(coverage_root.Get());
  list->SetPipelineState(coverage_pso.Get());
  list->SetComputeRoot32BitConstant(0u, coverage_case_count, 0u);
  list->SetComputeRootDescriptorTable(1u, coverage_heap->GetGPUDescriptorHandleForHeapStart());
  list->Dispatch(1u, 1u, 1u);
  D3D12_RESOURCE_BARRIER coverage_uav_barrier = {};
  coverage_uav_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
  coverage_uav_barrier.UAV.pResource = coverage_output.Get();
  list->ResourceBarrier(1u, &coverage_uav_barrier);
  coverage_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
  coverage_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
  list->ResourceBarrier(1u, &coverage_barrier);
  list->CopyBufferRegion(coverage_readback.Get(), 0u, coverage_output.Get(), 0u, coverage_bytes);
  coverage_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
  coverage_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COMMON;
  list->ResourceBarrier(1u, &coverage_barrier);
  wait_context = "N3b tiny-component AABB coverage contract";
  if (!submit_wait()) return 1;
  void* coverage_mapped = nullptr;
  if (!Check(coverage_readback->Map(0u, nullptr, &coverage_mapped), "map coverage contract results")) return 1;
  const auto* coverage_results = static_cast<const float*>(coverage_mapped);
  const float tiny_coverage_end = coverage_results[0];
  const float tiny_valid = coverage_results[1];
  const float resolved_coverage_end = coverage_results[2];
  const float resolved_valid = coverage_results[3];
  const float truncated_coverage_end = coverage_results[4];
  const float truncated_valid = coverage_results[5];
  const float full_coverage_end = coverage_results[6];
  const float full_valid = coverage_results[7];
  coverage_readback->Unmap(0u, nullptr);
  if (tiny_coverage_end != 0.0f || tiny_valid != 0.0f || !std::isfinite(resolved_coverage_end)
      || std::abs(resolved_coverage_end - 5.0f) > 0.05f || resolved_valid != 1.0f
      || truncated_coverage_end != 65503.5f || truncated_valid != 0.0f
      || full_coverage_end != 65504.0f || full_valid != 1.0f) {
    std::fprintf(stderr, "FAIL N3b coverage tiny=(%g,%g) resolved=(%g,%g) truncated=(%g,%g) full=(%g,%g)\n",
                 tiny_coverage_end, tiny_valid, resolved_coverage_end, resolved_valid,
                 truncated_coverage_end, truncated_valid, full_coverage_end, full_valid);
    return 1;
  }
  std::printf("PASS N3b tiny outward AABB component fails closed; resolved exit=%g; 65503.5 truncated, 65504 fully covered\n",
              resolved_coverage_end);
  if (!ValidateDebugMessages(info_queue.Get())) return 1;
  std::printf("PASS production surface + DXR visibility (3x2, guard sentinel, masks incl. FORCE_OPAQUE, endpoint, coverage, scene/surface invalids, bias, alpha)\n");
  CloseHandle(event);
  return 0;
}
