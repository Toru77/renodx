#include "../rt12_nrd.hpp"
#include "world_rtxdi_nrd_prep.h"
#include "world_rtxdi_nrd_probe.h"
#include "world_rtxdi_nrd_publish.h"

#include <dxgi1_2.h>
#include <windows.h>

#include <array>
#include <bit>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <memory>
#include <thread>
#include <vector>

using Microsoft::WRL::ComPtr;

namespace {
bool Check(HRESULT hr, const char* operation) {
  if (FAILED(hr)) std::fprintf(stderr, "FAIL NRD D3D12 %s: HRESULT 0x%08lx\n", operation, static_cast<unsigned long>(hr));
  return SUCCEEDED(hr);
}

DXGI_FORMAT NrdDxgiFormat(nrd::Format format) {
  switch (format) {
    case nrd::Format::R8_UNORM:             return DXGI_FORMAT_R8_UNORM;
    case nrd::Format::RGBA8_UNORM:          return DXGI_FORMAT_R8G8B8A8_UNORM;
    case nrd::Format::R16_SFLOAT:           return DXGI_FORMAT_R16_FLOAT;
    case nrd::Format::RG16_SFLOAT:          return DXGI_FORMAT_R16G16_FLOAT;
    case nrd::Format::RGBA16_SFLOAT:        return DXGI_FORMAT_R16G16B16A16_FLOAT;
    case nrd::Format::R32_SFLOAT:           return DXGI_FORMAT_R32_FLOAT;
    case nrd::Format::R11_G11_B10_UFLOAT:   return DXGI_FORMAT_R11G11B10_FLOAT;
    case nrd::Format::RGBA8_UINT:           return DXGI_FORMAT_R8G8B8A8_UINT;
    case nrd::Format::R10_G10_B10_A2_UNORM: return DXGI_FORMAT_R10G10B10A2_UNORM;
    default:                                return DXGI_FORMAT_UNKNOWN;
  }
}

DXGI_FORMAT NrdExternalFormat(nrd::ResourceType type) {
  switch (type) {
    case nrd::ResourceType::IN_MV:                     return DXGI_FORMAT_R16G16_FLOAT;
    case nrd::ResourceType::IN_NORMAL_ROUGHNESS:       return DXGI_FORMAT_R10G10B10A2_UNORM;
    case nrd::ResourceType::IN_VIEWZ:                  return DXGI_FORMAT_R32_FLOAT;
    case nrd::ResourceType::IN_DIFF_RADIANCE_HITDIST:
    case nrd::ResourceType::IN_SPEC_RADIANCE_HITDIST:
    case nrd::ResourceType::OUT_DIFF_RADIANCE_HITDIST:
    case nrd::ResourceType::OUT_SPEC_RADIANCE_HITDIST:
    case nrd::ResourceType::IN_DIFF_SH0:               return DXGI_FORMAT_R16G16B16A16_FLOAT;
    case nrd::ResourceType::IN_DIFF_DIRECTION_HITDIST: return DXGI_FORMAT_R8G8B8A8_UNORM;
    default:                                           return DXGI_FORMAT_UNKNOWN;
  }
}

uint32_t NrdPixelBytes(DXGI_FORMAT format) {
  switch (format) {
    case DXGI_FORMAT_R8_UNORM:           return 1u;
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UINT:
    case DXGI_FORMAT_R10G10B10A2_UNORM:  return 4u;
    case DXGI_FORMAT_R16_FLOAT:          return 2u;
    case DXGI_FORMAT_R16G16_FLOAT:       return 4u;
    case DXGI_FORMAT_R16G16B16A16_FLOAT: return 8u;
    case DXGI_FORMAT_R32_FLOAT:
    case DXGI_FORMAT_R32_UINT:
    case DXGI_FORMAT_R11G11B10_FLOAT:    return 4u;
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
    case DXGI_FORMAT_R32G32B32A32_UINT:  return 16u;
    default:                             return 0u;
  }
}

D3D12_HEAP_PROPERTIES NrdHeapProperties(D3D12_HEAP_TYPE type) {
  D3D12_HEAP_PROPERTIES properties = {};
  properties.Type = type;
  properties.CreationNodeMask = 1u;
  properties.VisibleNodeMask = 1u;
  return properties;
}

uint16_t NrdFloatToHalf(float value) {
  const uint32_t bits = std::bit_cast<uint32_t>(value);
  const uint32_t sign = (bits >> 16u) & 0x8000u;
  int32_t exponent = static_cast<int32_t>((bits >> 23u) & 0xffu) - 127 + 15;
  uint32_t mantissa = bits & 0x7fffffu;
  if (exponent <= 0) {
    if (exponent < -10) return static_cast<uint16_t>(sign);
    mantissa = (mantissa | 0x800000u) >> static_cast<uint32_t>(1 - exponent);
    return static_cast<uint16_t>(sign | ((mantissa + 0x1000u) >> 13u));
  }
  if (exponent >= 31) return static_cast<uint16_t>(sign | 0x7c00u);
  return static_cast<uint16_t>(sign | (static_cast<uint32_t>(exponent) << 10u) | ((mantissa + 0x1000u) >> 13u));
}

float NrdHalfToFloat(uint16_t value) {
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

bool NrdCreateTexture(ID3D12Device* device, uint32_t width, uint32_t height, DXGI_FORMAT format,
                      bool allow_unordered_access, falcom_world::rt12::Rt12NrdTexture* output) {
  if (device == nullptr || output == nullptr || width == 0u || height == 0u || format == DXGI_FORMAT_UNKNOWN) return false;
  D3D12_RESOURCE_DESC desc = {};
  desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
  desc.Width = width;
  desc.Height = height;
  desc.DepthOrArraySize = 1u;
  desc.MipLevels = 1u;
  desc.Format = format;
  desc.SampleDesc.Count = 1u;
  desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
  desc.Flags = allow_unordered_access ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE;
  const D3D12_HEAP_PROPERTIES heap = NrdHeapProperties(D3D12_HEAP_TYPE_DEFAULT);
  if (!Check(device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COMMON,
                                             nullptr, IID_PPV_ARGS(&output->resource)),
             "create NRD texture")) return false;
  output->format = format;
  output->width = width;
  output->height = height;
  output->state = D3D12_RESOURCE_STATE_COMMON;
  output->unordered_access_written = false;
  return true;
}

bool NrdCreateUploadBuffer(ID3D12Device* device, uint64_t bytes, ComPtr<ID3D12Resource>* output) {
  if (bytes == 0u || output == nullptr) return false;
  D3D12_RESOURCE_DESC desc = {};
  desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  desc.Width = bytes;
  desc.Height = 1u;
  desc.DepthOrArraySize = 1u;
  desc.MipLevels = 1u;
  desc.SampleDesc.Count = 1u;
  desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  const D3D12_HEAP_PROPERTIES heap = NrdHeapProperties(D3D12_HEAP_TYPE_UPLOAD);
  const HRESULT hr = device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc,
                                                     D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                                     IID_PPV_ARGS(output->GetAddressOf()));
  if (FAILED(hr)) {
    std::fprintf(stderr, "NRD upload allocation removed reason=0x%08lx bytes=%llu\n",
                 static_cast<unsigned long>(device->GetDeviceRemovedReason()), static_cast<unsigned long long>(bytes));
  }
  return Check(hr, "create NRD upload buffer");
}

bool NrdCreateReadbackBuffer(ID3D12Device* device, uint64_t bytes, ComPtr<ID3D12Resource>* output) {
  if (bytes == 0u || output == nullptr) return false;
  D3D12_RESOURCE_DESC desc = {};
  desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  desc.Width = bytes;
  desc.Height = 1u;
  desc.DepthOrArraySize = 1u;
  desc.MipLevels = 1u;
  desc.SampleDesc.Count = 1u;
  desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  const D3D12_HEAP_PROPERTIES heap = NrdHeapProperties(D3D12_HEAP_TYPE_READBACK);
  return Check(device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc,
                                               D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                               IID_PPV_ARGS(output->GetAddressOf())),
               "create NRD readback buffer");
}

bool NrdCreateBuffer(ID3D12Device* device, D3D12_HEAP_TYPE heap_type, uint64_t bytes,
                     D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES initial_state,
                     ComPtr<ID3D12Resource>* output) {
  if (device == nullptr || bytes == 0u || output == nullptr) return false;
  D3D12_RESOURCE_DESC desc = {};
  desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  desc.Width = bytes;
  desc.Height = 1u;
  desc.DepthOrArraySize = 1u;
  desc.MipLevels = 1u;
  desc.SampleDesc.Count = 1u;
  desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  desc.Flags = flags;
  const D3D12_HEAP_PROPERTIES heap = NrdHeapProperties(heap_type);
  return Check(device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc, initial_state,
                                               nullptr, IID_PPV_ARGS(output->GetAddressOf())),
               "create camera probe buffer");
}

bool NrdCreateSlotResources(ID3D12Device* device, const nrd::InstanceDesc& instance_desc,
                            const nrd::DispatchDesc* dispatches, uint32_t dispatches_num,
                            uint32_t width, uint32_t height, falcom_world::rt12::Rt12NrdResourcePools* pools,
                            falcom_world::rt12::Rt12NrdSlot* slot, uint32_t descriptor_stride, bool create_pools) {
  using falcom_world::rt12::Rt12NrdTexture;
  if (device == nullptr || dispatches == nullptr || pools == nullptr || slot == nullptr) return false;
  if (create_pools) {
    pools->permanent.resize(instance_desc.permanentPoolSize);
    pools->transient.resize(instance_desc.transientPoolSize);
  }
  const auto create_pool = [&](const nrd::TextureDesc& source, Rt12NrdTexture* target) {
    const uint32_t factor = source.downsampleFactor;
    const uint32_t pool_width = (width + factor - 1u) / factor;
    const uint32_t pool_height = (height + factor - 1u) / factor;
    const DXGI_FORMAT format = NrdDxgiFormat(source.format);
    if (!NrdCreateTexture(device, pool_width, pool_height, format, true, target)) return false;
    const D3D12_RESOURCE_DESC resource_desc = target->resource->GetDesc();
    pools->bytes += device->GetResourceAllocationInfo(0u, 1u, &resource_desc).SizeInBytes;
    return true;
  };
  if (create_pools) {
    for (uint32_t index = 0; index < instance_desc.permanentPoolSize; ++index) {
      if (!create_pool(instance_desc.permanentPool[index], &pools->permanent[index])) return false;
    }
    for (uint32_t index = 0; index < instance_desc.transientPoolSize; ++index) {
      if (!create_pool(instance_desc.transientPool[index], &pools->transient[index])) return false;
    }
  }

  const uint32_t external_count = static_cast<uint32_t>(nrd::ResourceType::TRANSIENT_POOL);
  std::array<bool, static_cast<uint32_t>(nrd::ResourceType::MAX_NUM)> used_external = {};
  for (uint32_t dispatch_index = 0; dispatch_index < dispatches_num; ++dispatch_index) {
    for (uint32_t resource_index = 0; resource_index < dispatches[dispatch_index].resourcesNum; ++resource_index) {
      const nrd::ResourceType type = dispatches[dispatch_index].resources[resource_index].type;
      const uint32_t type_index = static_cast<uint32_t>(type);
      if (type_index < external_count) used_external[type_index] = true;
    }
  }
  for (uint32_t type_index = 0; type_index < external_count; ++type_index) {
    if (!used_external[type_index]) continue;
    const auto type = static_cast<nrd::ResourceType>(type_index);
    const DXGI_FORMAT format = NrdExternalFormat(type);
    if (format == DXGI_FORMAT_UNKNOWN) {
      std::fprintf(stderr, "FAIL unsupported synthetic NRD resource type=%u\n", type_index);
      return false;
    }
    const bool is_output = type == nrd::ResourceType::OUT_DIFF_RADIANCE_HITDIST
                           || type == nrd::ResourceType::OUT_SPEC_RADIANCE_HITDIST;
    if (!NrdCreateTexture(device, width, height, format, is_output, &slot->resources[type_index])) return false;
  }

  D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
  heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
  heap_desc.NumDescriptors = descriptor_stride * dispatches_num;
  heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
  if (!Check(device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&slot->descriptors)), "create NRD descriptor heap")) return false;

  if (!Check(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&slot->allocator)),
             "create NRD command allocator")
      || !Check(device->CreateCommandList(0u, D3D12_COMMAND_LIST_TYPE_DIRECT, slot->allocator.Get(), nullptr,
                                          IID_PPV_ARGS(&slot->command_list)),
                "create NRD command list")
      || !Check(slot->command_list->Close(), "close initial NRD command list")) return false;

  const uint64_t cb_stride = (instance_desc.constantBufferMaxDataSize + 255u) & ~uint64_t(255u);
  const uint64_t cb_bytes = cb_stride * dispatches_num;
  if (!NrdCreateUploadBuffer(device, cb_bytes, &slot->constants)) return false;
  slot->dispatch_descriptors = slot->descriptors;
  slot->dispatch_constants = slot->constants;
  slot->dispatch_count = dispatches_num;
  D3D12_RANGE no_read = {0u, 0u};
  void* constants = nullptr;
  if (!Check(slot->constants->Map(0u, &no_read, &constants), "map NRD constants")) return false;
  std::memset(constants, 0, static_cast<size_t>(cb_bytes));
  slot->constants->Unmap(0u, nullptr);

  for (nrd::ResourceType output_type : {nrd::ResourceType::OUT_DIFF_RADIANCE_HITDIST,
                                        nrd::ResourceType::OUT_SPEC_RADIANCE_HITDIST}) {
    const auto& resource = slot->resources[static_cast<uint32_t>(output_type)];
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint = {};
    UINT rows = 0u;
    UINT64 row_bytes = 0u;
    UINT64 total_bytes = 0u;
    const D3D12_RESOURCE_DESC resource_desc = resource.resource->GetDesc();
    device->GetCopyableFootprints(&resource_desc, 0u, 1u, 0u, &footprint, &rows, &row_bytes, &total_bytes);
    ComPtr<ID3D12Resource> readback;
    if (!NrdCreateReadbackBuffer(device, total_bytes, &readback)) return false;
    if (output_type == nrd::ResourceType::OUT_DIFF_RADIANCE_HITDIST)
      slot->diffuse_readback = readback;
    else
      slot->specular_readback = readback;
  }
  return true;
}

void NrdCreateNullTextureSrv(ID3D12Device* device, DXGI_FORMAT format, D3D12_CPU_DESCRIPTOR_HANDLE handle) {
  D3D12_SHADER_RESOURCE_VIEW_DESC view = {};
  view.Format = format;
  view.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
  view.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  view.Texture2D.MipLevels = 1u;
  device->CreateShaderResourceView(nullptr, &view, handle);
}

void NrdCreateNullTextureUav(ID3D12Device* device, DXGI_FORMAT format, D3D12_CPU_DESCRIPTOR_HANDLE handle) {
  D3D12_UNORDERED_ACCESS_VIEW_DESC view = {};
  view.Format = format;
  view.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
  device->CreateUnorderedAccessView(nullptr, nullptr, &view, handle);
}

bool NrdWaitForFence(ID3D12CommandQueue* queue, ID3D12Fence* fence, uint64_t value) {
  if (!Check(queue->Signal(fence, value), "signal NRD smoke fence")) return false;
  const auto report_device_removed = [&]() {
    ComPtr<ID3D12Device> device;
    if (SUCCEEDED(fence->GetDevice(IID_PPV_ARGS(&device)))) {
      std::fprintf(stderr, "FAIL NRD fence reports device removed: 0x%08lx\n",
                   static_cast<unsigned long>(device->GetDeviceRemovedReason()));
    } else {
      std::fprintf(stderr, "FAIL NRD fence reports device removed; device reason unavailable\n");
    }
  };
  const uint64_t completed_value = fence->GetCompletedValue();
  if (completed_value == UINT64_MAX) {
    report_device_removed();
    return false;
  }
  if (completed_value >= value) return true;
  HANDLE event_handle = CreateEventW(nullptr, FALSE, FALSE, nullptr);
  if (event_handle == nullptr) {
    std::fprintf(stderr, "FAIL NRD create fence event: %lu\n", GetLastError());
    return false;
  }
  const HRESULT event_hr = fence->SetEventOnCompletion(value, event_handle);
  const DWORD wait_result = SUCCEEDED(event_hr) ? WaitForSingleObject(event_handle, 15000u) : WAIT_FAILED;
  CloseHandle(event_handle);
  if (!Check(event_hr, "set fence event")) return false;
  const uint64_t completed_after_wait = fence->GetCompletedValue();
  if (completed_after_wait == UINT64_MAX) {
    report_device_removed();
    return false;
  }
  if (wait_result != WAIT_OBJECT_0 || completed_after_wait < value) {
    std::fprintf(stderr, "FAIL NRD bounded fence wait result=%lu completed=%llu requested=%llu\n", wait_result,
                 static_cast<unsigned long long>(completed_after_wait), static_cast<unsigned long long>(value));
    return false;
  }
  return true;
}

bool NrdReadbackOutput(ID3D12Device* device, ID3D12Resource* texture, ID3D12Resource* readback,
                       uint32_t width, uint32_t height, const char* name) {
  D3D12_RANGE read_range = {0u, static_cast<SIZE_T>(readback->GetDesc().Width)};
  const D3D12_RESOURCE_DESC desc = texture->GetDesc();
  D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint = {};
  UINT rows = 0u;
  UINT64 row_bytes = 0u;
  UINT64 total_bytes = 0u;
  device->GetCopyableFootprints(&desc, 0u, 1u, 0u, &footprint, &rows, &row_bytes, &total_bytes);
  const uint8_t* mapped = nullptr;
  if (!Check(readback->Map(0u, &read_range, reinterpret_cast<void**>(const_cast<uint8_t**>(&mapped))), "map NRD output")) {
    return false;
  }
  double sum = 0.0;
  uint64_t values = 0u;
  bool finite = rows == height && row_bytes >= static_cast<UINT64>(width) * 8u;
  for (uint32_t row = 0; row < height && finite; ++row) {
    const auto* pixels = reinterpret_cast<const uint16_t*>(mapped + footprint.Offset
                                                           + static_cast<size_t>(row) * footprint.Footprint.RowPitch);
    for (uint32_t column = 0; column < width; ++column) {
      for (uint32_t channel = 0; channel < 4u; ++channel) {
        const float value = NrdHalfToFloat(pixels[column * 4u + channel]);
        if (!std::isfinite(value)) {
          finite = false;
          break;
        }
        sum += value;
        ++values;
      }
    }
  }
  const D3D12_RANGE no_write = {0u, 0u};
  readback->Unmap(0u, &no_write);
  if (!finite || values == 0u) {
    std::fprintf(stderr, "FAIL NRD %s readback dimensions or values invalid\n", name);
    return false;
  }
  std::printf("PASS NRD %s %ux%u finite readback mean=%.6f\n", name, width, height, sum / values);
  return true;
}

falcom_world::rt12::Rt12NrdTexture* NrdGetTexture(const nrd::ResourceDesc& resource,
                                                  falcom_world::rt12::Rt12NrdResourcePools* pools,
                                                  falcom_world::rt12::Rt12NrdSlot* slot) {
  using falcom_world::rt12::Rt12NrdTexture;
  switch (resource.type) {
    case nrd::ResourceType::PERMANENT_POOL:
      return resource.indexInPool < pools->permanent.size() ? &pools->permanent[resource.indexInPool] : nullptr;
    case nrd::ResourceType::TRANSIENT_POOL:
      return resource.indexInPool < pools->transient.size() ? &pools->transient[resource.indexInPool] : nullptr;
    default: {
      const uint32_t index = static_cast<uint32_t>(resource.type);
      if (index >= static_cast<uint32_t>(nrd::ResourceType::TRANSIENT_POOL)) return nullptr;
      Rt12NrdTexture& texture = slot->resources[index];
      return texture.resource != nullptr ? &texture : nullptr;
    }
  }
}

void NrdTransition(ID3D12GraphicsCommandList* list, falcom_world::rt12::Rt12NrdTexture* texture,
                   D3D12_RESOURCE_STATES after, std::vector<D3D12_RESOURCE_BARRIER>* barriers) {
  if (texture->state != after) {
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = texture->resource.Get();
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = texture->state;
    barrier.Transition.StateAfter = after;
    barriers->push_back(barrier);
    texture->state = after;
    texture->unordered_access_written = false;
  } else if (after == D3D12_RESOURCE_STATE_UNORDERED_ACCESS && texture->unordered_access_written) {
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier.UAV.pResource = texture->resource.Get();
    barriers->push_back(barrier);
    texture->unordered_access_written = false;
  }
}

bool NrdUploadTexture(ID3D12Device* device, ID3D12GraphicsCommandList* list,
                      falcom_world::rt12::Rt12NrdSlot* slot,
                      falcom_world::rt12::Rt12NrdTexture* texture, const std::vector<uint8_t>& pixels) {
  const uint32_t pixel_bytes = NrdPixelBytes(texture->format);
  const uint32_t source_pitch = texture->width * pixel_bytes;
  if (pixel_bytes == 0u || pixels.size() != static_cast<size_t>(source_pitch) * texture->height) return false;

  const D3D12_RESOURCE_DESC texture_desc = texture->resource->GetDesc();
  D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint = {};
  UINT rows = 0u;
  UINT64 row_bytes = 0u;
  UINT64 total_bytes = 0u;
  device->GetCopyableFootprints(&texture_desc, 0u, 1u, 0u, &footprint, &rows, &row_bytes, &total_bytes);
  ComPtr<ID3D12Resource> upload;
  if (!NrdCreateUploadBuffer(device, total_bytes, &upload)) return false;
  D3D12_RANGE no_read = {0u, 0u};
  uint8_t* mapped = nullptr;
  if (!Check(upload->Map(0u, &no_read, reinterpret_cast<void**>(&mapped)), "map NRD texture staging")) return false;
  for (uint32_t row = 0; row < texture->height; ++row) {
    std::memcpy(mapped + footprint.Offset + static_cast<size_t>(row) * footprint.Footprint.RowPitch,
                pixels.data() + static_cast<size_t>(row) * source_pitch, source_pitch);
  }
  upload->Unmap(0u, nullptr);
  slot->staging.push_back(upload);

  std::vector<D3D12_RESOURCE_BARRIER> barriers;
  NrdTransition(list, texture, D3D12_RESOURCE_STATE_COPY_DEST, &barriers);
  if (!barriers.empty()) list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
  D3D12_TEXTURE_COPY_LOCATION source = {};
  source.pResource = upload.Get();
  source.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
  source.PlacedFootprint = footprint;
  D3D12_TEXTURE_COPY_LOCATION destination = {};
  destination.pResource = texture->resource.Get();
  destination.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
  destination.SubresourceIndex = 0u;
  list->CopyTextureRegion(&destination, 0u, 0u, 0u, &source, nullptr);
  barriers.clear();
  NrdTransition(list, texture, D3D12_RESOURCE_STATE_COMMON, &barriers);
  if (!barriers.empty()) list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
  return true;
}

std::vector<uint8_t> NrdSyntheticPixels(nrd::ResourceType type, uint32_t width, uint32_t height,
                                        float specular_demodulation) {
  const DXGI_FORMAT format = NrdExternalFormat(type);
  const uint32_t pixel_bytes = NrdPixelBytes(format);
  std::vector<uint8_t> pixels(static_cast<size_t>(width) * height * pixel_bytes, 0u);
  const uint32_t viewz_half = NrdFloatToHalf(10.0f);
  const uint32_t normal_x = 512u;
  const uint32_t normal_y = 512u;
  const uint32_t normal_z = static_cast<uint32_t>(std::lround(0.75f * 1023.0f));
  const uint32_t material_id = 1u;
  const uint32_t normal_roughness = normal_x | (normal_y << 10u) | (normal_z << 20u) | (material_id << 30u);
  const std::array<uint16_t, 4> diffuse = {NrdFloatToHalf(0.4f), NrdFloatToHalf(0.25f), NrdFloatToHalf(0.1f),
                                           NrdFloatToHalf(4.0f)};
  const std::array<uint16_t, 4> specular = {NrdFloatToHalf(0.08f / specular_demodulation),
                                            NrdFloatToHalf(0.04f / specular_demodulation),
                                            NrdFloatToHalf(0.02f / specular_demodulation), NrdFloatToHalf(3.0f)};
  for (size_t pixel = 0; pixel < static_cast<size_t>(width) * height; ++pixel) {
    uint8_t* destination = pixels.data() + pixel * pixel_bytes;
    switch (type) {
      case nrd::ResourceType::IN_MV: {
        const std::array<uint16_t, 2> motion = {0u, 0u};
        std::memcpy(destination, motion.data(), sizeof(motion));
        break;
      }
      case nrd::ResourceType::IN_NORMAL_ROUGHNESS:
        std::memcpy(destination, &normal_roughness, sizeof(normal_roughness));
        break;
      case nrd::ResourceType::IN_VIEWZ:
        std::memcpy(destination, &viewz_half, sizeof(uint16_t));
        break;
      case nrd::ResourceType::IN_DIFF_RADIANCE_HITDIST:
        std::memcpy(destination, diffuse.data(), sizeof(diffuse));
        break;
      case nrd::ResourceType::IN_SPEC_RADIANCE_HITDIST:
        std::memcpy(destination, specular.data(), sizeof(specular));
        break;
      default: break;
    }
  }
  return pixels;
}

bool NrdUploadSyntheticInputs(ID3D12Device* device, ID3D12GraphicsCommandList* list,
                              falcom_world::rt12::Rt12NrdSlot* slot, float specular_demodulation) {
  constexpr uint32_t external_count = static_cast<uint32_t>(nrd::ResourceType::TRANSIENT_POOL);
  for (uint32_t type_index = 0; type_index < external_count; ++type_index) {
    auto& texture = slot->resources[type_index];
    if (texture.resource == nullptr) continue;
    if (type_index == static_cast<uint32_t>(nrd::ResourceType::OUT_DIFF_RADIANCE_HITDIST)
        || type_index == static_cast<uint32_t>(nrd::ResourceType::OUT_SPEC_RADIANCE_HITDIST)) continue;
    if (!NrdUploadTexture(device, list, slot, &texture,
                          NrdSyntheticPixels(static_cast<nrd::ResourceType>(type_index), texture.width, texture.height,
                                             specular_demodulation))) {
      std::fprintf(stderr, "FAIL NRD upload input type=%u format=%u size=%ux%u\n", type_index,
                   static_cast<uint32_t>(texture.format), texture.width, texture.height);
      return false;
    }
  }
  return true;
}

bool NrdRecordDispatches(ID3D12Device* device, const nrd::InstanceDesc& instance_desc,
                         const nrd::DispatchDesc* dispatches, uint32_t dispatches_num,
                         falcom_world::rt12::Rt12NrdPipelineSet* native_pipelines,
                         falcom_world::rt12::Rt12NrdResourcePools* pools,
                         falcom_world::rt12::Rt12NrdSlot* slot, uint32_t descriptor_stride) {
  if (device == nullptr || dispatches == nullptr || native_pipelines == nullptr || pools == nullptr || slot == nullptr) {
    return false;
  }
  if (!Check(slot->allocator->Reset(), "reset NRD allocator")
      || !Check(slot->command_list->Reset(slot->allocator.Get(), nullptr), "reset NRD command list")) return false;

  slot->staging.clear();
  ID3D12GraphicsCommandList* list = slot->command_list.Get();
  if (!NrdUploadSyntheticInputs(device, list, slot, 0.25f)) return false;
  const uint32_t srv_count = instance_desc.descriptorPoolDesc.perSetTexturesMaxNum;
  const uint32_t uav_count = instance_desc.descriptorPoolDesc.perSetStorageTexturesMaxNum;
  const uint32_t descriptor_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
  const D3D12_CPU_DESCRIPTOR_HANDLE heap_cpu = slot->descriptors->GetCPUDescriptorHandleForHeapStart();
  const D3D12_GPU_DESCRIPTOR_HANDLE heap_gpu = slot->descriptors->GetGPUDescriptorHandleForHeapStart();
  const uint64_t cb_stride = (instance_desc.constantBufferMaxDataSize + 255u) & ~uint64_t(255u);
  void* constant_data = nullptr;
  const D3D12_RANGE no_read = {0u, 0u};
  if (!Check(slot->constants->Map(0u, &no_read, &constant_data), "map NRD dispatch constants")) return false;
  std::memset(constant_data, 0, static_cast<size_t>(cb_stride) * dispatches_num);
  auto* constant_bytes = static_cast<uint8_t*>(constant_data);
  uint32_t previous_constant_dispatch = UINT32_MAX;
  for (uint32_t dispatch_index = 0; dispatch_index < dispatches_num; ++dispatch_index) {
    const nrd::DispatchDesc& dispatch = dispatches[dispatch_index];
    if (dispatch.pipelineIndex >= native_pipelines->pipelines.size() || dispatch.resources == nullptr
        || dispatch.gridWidth == 0u || dispatch.gridHeight == 0u
        || dispatch.resourcesNum > srv_count + uav_count) {
      std::fprintf(stderr, "FAIL NRD invalid dispatch record %u pipeline=%u grid=%ux%u resources=%u limits=%u/%u\n",
                   dispatch_index, dispatch.pipelineIndex, dispatch.gridWidth, dispatch.gridHeight,
                   dispatch.resourcesNum, srv_count, uav_count);
      return false;
    }

    if (!dispatch.constantBufferDataMatchesPreviousDispatch) {
      if (dispatch.constantBufferDataSize > cb_stride
          || (dispatch.constantBufferDataSize != 0u && dispatch.constantBufferData == nullptr)) {
        std::fprintf(stderr, "FAIL NRD dispatch constants index=%u data=%p bytes=%u stride=%llu\n", dispatch_index,
                     dispatch.constantBufferData, dispatch.constantBufferDataSize,
                     static_cast<unsigned long long>(cb_stride));
        return false;
      }
      if (dispatch.constantBufferDataSize != 0u) {
        std::memcpy(constant_bytes + static_cast<size_t>(dispatch_index) * cb_stride,
                    dispatch.constantBufferData, dispatch.constantBufferDataSize);
      }
      previous_constant_dispatch = dispatch_index;
    } else if (previous_constant_dispatch == UINT32_MAX) {
      std::fprintf(stderr, "FAIL NRD dispatch %u reuses missing constants\n", dispatch_index);
      return false;
    } else {
      std::memcpy(constant_bytes + static_cast<size_t>(dispatch_index) * cb_stride,
                  constant_bytes + static_cast<size_t>(previous_constant_dispatch) * cb_stride,
                  static_cast<size_t>(cb_stride));
      previous_constant_dispatch = dispatch_index;
    }

    const D3D12_CPU_DESCRIPTOR_HANDLE dispatch_cpu = {
        heap_cpu.ptr + static_cast<SIZE_T>(dispatch_index) * descriptor_stride * descriptor_size};
    const D3D12_GPU_DESCRIPTOR_HANDLE dispatch_gpu = {
        heap_gpu.ptr + static_cast<UINT64>(dispatch_index) * descriptor_stride * descriptor_size};
    for (uint32_t index = 0; index < srv_count; ++index) {
      NrdCreateNullTextureSrv(device, DXGI_FORMAT_R16G16B16A16_FLOAT,
                              {dispatch_cpu.ptr + static_cast<SIZE_T>(index) * descriptor_size});
    }
    for (uint32_t index = 0; index < uav_count; ++index) {
      NrdCreateNullTextureUav(device, DXGI_FORMAT_R16G16B16A16_FLOAT,
                              {dispatch_cpu.ptr + static_cast<SIZE_T>(srv_count + index) * descriptor_size});
    }

    std::vector<D3D12_RESOURCE_BARRIER> barriers;
    uint32_t srv_index = 0u;
    uint32_t uav_index = 0u;
    for (uint32_t resource_index = 0; resource_index < dispatch.resourcesNum; ++resource_index) {
      const nrd::ResourceDesc& resource_desc = dispatch.resources[resource_index];
      auto* texture = NrdGetTexture(resource_desc, pools, slot);
      if (texture == nullptr) {
        std::fprintf(stderr, "FAIL NRD resource lookup dispatch=%u resource=%u type=%u pool_index=%u\n",
                     dispatch_index, resource_index, static_cast<uint32_t>(resource_desc.type), resource_desc.indexInPool);
        return false;
      }
      if (resource_desc.descriptorType == nrd::DescriptorType::TEXTURE) {
        if (srv_index >= srv_count) return false;
        NrdTransition(list, texture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, &barriers);
        D3D12_SHADER_RESOURCE_VIEW_DESC view = {};
        view.Format = texture->format;
        view.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        view.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        view.Texture2D.MipLevels = 1u;
        device->CreateShaderResourceView(texture->resource.Get(), &view,
                                         {dispatch_cpu.ptr + static_cast<SIZE_T>(srv_index) * descriptor_size});
        ++srv_index;
      } else if (resource_desc.descriptorType == nrd::DescriptorType::STORAGE_TEXTURE) {
        if (uav_index >= uav_count) return false;
        NrdTransition(list, texture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, &barriers);
        D3D12_UNORDERED_ACCESS_VIEW_DESC view = {};
        view.Format = texture->format;
        view.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
        device->CreateUnorderedAccessView(texture->resource.Get(), nullptr, &view,
                                          {dispatch_cpu.ptr + static_cast<SIZE_T>(srv_count + uav_index) * descriptor_size});
        texture->unordered_access_written = true;
        ++uav_index;
      } else {
        std::fprintf(stderr, "FAIL NRD descriptor type=%u dispatch=%u resource=%u\n",
                     static_cast<uint32_t>(resource_desc.descriptorType), dispatch_index, resource_index);
        return false;
      }
    }
    if (!barriers.empty()) list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());

    ID3D12DescriptorHeap* heaps[] = {slot->descriptors.Get()};
    list->SetDescriptorHeaps(1u, heaps);
    list->SetComputeRootSignature(native_pipelines->root_signature.Get());
    list->SetComputeRootConstantBufferView(0u, slot->constants->GetGPUVirtualAddress()
                                                   + static_cast<uint64_t>(dispatch_index) * cb_stride);
    list->SetComputeRootDescriptorTable(1u, dispatch_gpu);
    list->SetComputeRootDescriptorTable(2u, {dispatch_gpu.ptr + static_cast<UINT64>(srv_count) * descriptor_size});
    list->SetPipelineState(native_pipelines->pipelines[dispatch.pipelineIndex].Get());
    list->Dispatch(dispatch.gridWidth, dispatch.gridHeight, 1u);
  }
  slot->constants->Unmap(0u, nullptr);

  for (nrd::ResourceType output_type : {nrd::ResourceType::OUT_DIFF_RADIANCE_HITDIST,
                                        nrd::ResourceType::OUT_SPEC_RADIANCE_HITDIST}) {
    auto& texture = slot->resources[static_cast<uint32_t>(output_type)];
    std::vector<D3D12_RESOURCE_BARRIER> barriers;
    NrdTransition(list, &texture, D3D12_RESOURCE_STATE_COPY_SOURCE, &barriers);
    if (!barriers.empty()) list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
    const D3D12_RESOURCE_DESC texture_desc = texture.resource->GetDesc();
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint = {};
    UINT rows = 0u;
    UINT64 row_bytes = 0u;
    UINT64 total_bytes = 0u;
    device->GetCopyableFootprints(&texture_desc, 0u, 1u, 0u, &footprint, &rows, &row_bytes, &total_bytes);
    ID3D12Resource* readback = output_type == nrd::ResourceType::OUT_DIFF_RADIANCE_HITDIST
                                   ? slot->diffuse_readback.Get()
                                   : slot->specular_readback.Get();
    D3D12_TEXTURE_COPY_LOCATION source = {};
    source.pResource = texture.resource.Get();
    source.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    source.SubresourceIndex = 0u;
    D3D12_TEXTURE_COPY_LOCATION destination = {};
    destination.pResource = readback;
    destination.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    destination.PlacedFootprint = footprint;
    list->CopyTextureRegion(&destination, 0u, 0u, 0u, &source, nullptr);
    barriers.clear();
    NrdTransition(list, &texture, D3D12_RESOURCE_STATE_COMMON, &barriers);
    if (!barriers.empty()) list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
  }
  return Check(list->Close(), "close NRD command list");
}

bool RunN1PrepRelaxSmoke(ID3D12Device* device, ID3D12CommandQueue* queue, ID3D12Fence* fence,
                         ID3D12InfoQueue* info_queue, bool debug_layer_enabled,
                         falcom_world::rt12::Rt12NrdGeneration* generation, const nrd::InstanceDesc& instance_desc,
                         falcom_world::rt12::Rt12NrdPipelineSet* pipelines,
                         falcom_world::rt12::Rt12NrdResourcePools* pools, falcom_world::rt12::Rt12NrdSlot* nrd_slot,
                         const nrd::DispatchDesc* dispatches, uint32_t dispatch_count,
                         uint32_t descriptor_stride, uint64_t* fence_value) {
  using namespace falcom_world::rt12;
  constexpr uint32_t width = 13u, height = 9u, pixel_count = width * height;
  if (device == nullptr || queue == nullptr || fence == nullptr || info_queue == nullptr || generation == nullptr
      || pipelines == nullptr || pools == nullptr || nrd_slot == nullptr || dispatches == nullptr
      || fence_value == nullptr) return false;
  Rt12NrdSlot& slot = *nrd_slot;
  ComPtr<ID3D12DescriptorHeap> prep_heap;
  D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
  heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
  heap_desc.NumDescriptors = 15u;
  heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
  if (!Check(device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&prep_heap)), "create N1 prep heap")) return false;
  const uint32_t descriptor_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
  const D3D12_CPU_DESCRIPTOR_HANDLE heap_cpu = prep_heap->GetCPUDescriptorHandleForHeapStart();
  const D3D12_GPU_DESCRIPTOR_HANDLE heap_gpu = prep_heap->GetGPUDescriptorHandleForHeapStart();
  const auto cpu_at = [&](uint32_t index) {
    return D3D12_CPU_DESCRIPTOR_HANDLE{heap_cpu.ptr + static_cast<SIZE_T>(index) * descriptor_size};
  };
  const std::array<DXGI_FORMAT, 8u> formats = {DXGI_FORMAT_R32_FLOAT, DXGI_FORMAT_R32G32B32A32_UINT,
                                               DXGI_FORMAT_R32G32B32A32_UINT, DXGI_FORMAT_R32G32B32A32_FLOAT,
                                               DXGI_FORMAT_R32G32B32A32_UINT, DXGI_FORMAT_UNKNOWN,
                                               DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_R32_UINT};
  std::array<Rt12NrdTexture, 7u> inputs = {};
  for (uint32_t index = 0u; index < formats.size(); ++index) {
    if (index == 5u) continue;
    const uint32_t resource_index = index < 5u ? index : index - 1u;
    const uint32_t input_width = index == 2u ? RTXDI_SURFACE_SCENE_ROW_COUNT : width;
    const uint32_t input_height = index == 2u ? 1u : height;
    if (!NrdCreateTexture(device, input_width, input_height, formats[index], false, &inputs[resource_index])) return false;
    D3D12_SHADER_RESOURCE_VIEW_DESC view = {};
    view.Format = formats[index];
    view.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    if (index == 5u) {
      view.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
      view.Buffer.NumElements = 1u;
      view.Buffer.StructureByteStride = 144u;
    } else {
      view.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
      view.Texture2D.MipLevels = 1u;
    }
    device->CreateShaderResourceView(index == 5u ? nullptr : inputs[resource_index].resource.Get(), &view, cpu_at(index));
  }
  ComPtr<ID3D12Resource> hit_distance_input, hit_validity_input;
  if (!NrdCreateUploadBuffer(device, pixel_count * sizeof(float), &hit_distance_input)
      || !NrdCreateUploadBuffer(device, pixel_count * sizeof(uint32_t), &hit_validity_input)) return false;
  std::vector<float> traced_hit_distance(pixel_count, 3.25f);
  std::vector<uint32_t> traced_hit_validity(pixel_count, 0u);
  for (uint32_t pixel = 0u; pixel < pixel_count; ++pixel) {
    const uint32_t scenario = pixel & 7u;
    if (scenario == 0u || scenario == 1u || scenario == 4u || scenario == 5u) traced_hit_validity[pixel] = 1u;
  }
  void* hit_distance_mapped = nullptr;
  void* hit_validity_mapped = nullptr;
  if (FAILED(hit_distance_input->Map(0u, nullptr, &hit_distance_mapped))
      || FAILED(hit_validity_input->Map(0u, nullptr, &hit_validity_mapped))) return false;
  std::memcpy(hit_distance_mapped, traced_hit_distance.data(), traced_hit_distance.size() * sizeof(float));
  std::memcpy(hit_validity_mapped, traced_hit_validity.data(), traced_hit_validity.size() * sizeof(uint32_t));
  hit_distance_input->Unmap(0u, nullptr);
  hit_validity_input->Unmap(0u, nullptr);
  for (uint32_t index = 0u; index < 2u; ++index) {
    D3D12_SHADER_RESOURCE_VIEW_DESC view = {};
    view.Format = DXGI_FORMAT_UNKNOWN;
    view.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    view.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    view.Buffer.NumElements = pixel_count;
    view.Buffer.StructureByteStride = sizeof(uint32_t);
    device->CreateShaderResourceView(index == 0u ? hit_distance_input.Get() : hit_validity_input.Get(), &view, cpu_at(8u + index));
  }
  const std::array<nrd::ResourceType, 5u> prep_output_types = {nrd::ResourceType::IN_MV,
                                                               nrd::ResourceType::IN_NORMAL_ROUGHNESS,
                                                               nrd::ResourceType::IN_VIEWZ,
                                                               nrd::ResourceType::IN_DIFF_RADIANCE_HITDIST,
                                                               nrd::ResourceType::IN_SPEC_RADIANCE_HITDIST};
  for (uint32_t index = 0u; index < prep_output_types.size(); ++index) {
    Rt12NrdTexture& target = slot.resources[static_cast<uint32_t>(prep_output_types[index])];
    Rt12NrdTexture uav_target;
    if (!NrdCreateTexture(device, width, height, target.format, true, &uav_target)) {
      std::fprintf(stderr, "FAIL N1 prep output texture type=%u format=%u\n",
                   static_cast<uint32_t>(prep_output_types[index]), static_cast<uint32_t>(target.format));
      return false;
    }
    target = std::move(uav_target);
    D3D12_UNORDERED_ACCESS_VIEW_DESC view = {};
    view.Format = target.format;
    view.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    device->CreateUnorderedAccessView(target.resource.Get(), nullptr, &view, cpu_at(10u + index));
  }
  D3D12_FEATURE_DATA_FORMAT_SUPPORT r10_support = {};
  r10_support.Format = DXGI_FORMAT_R10G10B10A2_UNORM;
  const HRESULT feature_hr = device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &r10_support, sizeof(r10_support));
  if (FAILED(feature_hr) || (r10_support.Support1 & D3D12_FORMAT_SUPPORT1_TYPED_UNORDERED_ACCESS_VIEW) == 0u) {
    std::fprintf(stderr, "FAIL N1 prep R10 typed UAV feature hr=0x%08lx support=0x%08lx\n",
                 static_cast<unsigned long>(feature_hr), static_cast<unsigned long>(r10_support.Support1));
    return false;
  }
  const auto bytes = [](const auto& values) {
    std::vector<uint8_t> result(values.size() * sizeof(values[0]));
    std::memcpy(result.data(), values.data(), result.size());
    return result;
  };
  std::vector<float> depth(pixel_count, 0.5f);
  std::vector<uint32_t> normal(pixel_count * 4u, 0u);
  std::vector<uint32_t> scene(RTXDI_SURFACE_SCENE_ROW_COUNT * 4u, 0u);
  const auto set_row = [&](uint32_t row, std::array<float, 4u> values) {
    for (uint32_t component = 0u; component < 4u; ++component) scene[row * 4u + component] = std::bit_cast<uint32_t>(values[component]);
  };
  set_row(0u, {1.0f, 0.0f, 0.0f, 0.0f});
  set_row(1u, {0.0f, 1.0f, 0.0f, 0.0f});
  set_row(2u, {0.0f, 0.0f, -140000.0f, 0.0f});
  set_row(3u, {0.0f, 0.0f, 0.0f, 1.0f});
  set_row(20u, {1.0f, 0.0f, 0.0f, 0.0f});
  set_row(21u, {0.0f, 1.0f, 0.0f, 0.0f});
  set_row(22u, {0.0f, 0.0f, 1.0f, 0.0f});
  set_row(23u, {0.0f, 0.0f, 0.0f, 1.0f});
  std::vector<float> motion(pixel_count * 4u, 0.0f);
  std::vector<uint32_t> mrt2(pixel_count * 4u, 0u);
  std::vector<uint16_t> n1_diffuse(pixel_count * 4u, 0u);
  std::vector<uint32_t> n1_status(pixel_count, 0u);
  for (uint32_t pixel = 0u; pixel < pixel_count; ++pixel) {
    motion[pixel * 4u] = 0.5f;
    motion[pixel * 4u + 1u] = -0.25f;
    const uint32_t scenario = pixel & 7u;
    n1_status[pixel] = scenario == 0u ? 9u : scenario == 1u ? 10u
                                         : scenario == 2u   ? 3u
                                         : scenario == 3u   ? 4u
                                         : scenario == 4u   ? 25u
                                         : scenario == 5u   ? 26u
                                         : scenario == 6u   ? 19u
                                                            : 20u;
    if (scenario != 1u && scenario != 5u) {
      n1_diffuse[pixel * 4u + 0u] = NrdFloatToHalf(0.5f);
      n1_diffuse[pixel * 4u + 1u] = NrdFloatToHalf(0.25f);
      n1_diffuse[pixel * 4u + 2u] = NrdFloatToHalf(0.125f);
      n1_diffuse[pixel * 4u + 3u] = NrdFloatToHalf(2.0f);
    }
  }
  Rt12NrdSlot upload_slot;
  if (!Check(slot.allocator->Reset(), "reset N1 prep allocator")
      || !Check(slot.command_list->Reset(slot.allocator.Get(), nullptr), "reset N1 prep list")) return false;
  const std::array<std::vector<uint8_t>, 7u> upload_pixels = {bytes(depth), bytes(normal), bytes(scene), bytes(motion),
                                                              bytes(mrt2), bytes(n1_diffuse), bytes(n1_status)};
  for (uint32_t index = 0u; index < inputs.size(); ++index) {
    if (!NrdUploadTexture(device, slot.command_list.Get(), &upload_slot, &inputs[index], upload_pixels[index])) {
      std::fprintf(stderr, "FAIL N1 prep synthetic input upload index=%u width=%u height=%u format=%u bytes=%llu\n",
                   index, inputs[index].width, inputs[index].height, static_cast<uint32_t>(inputs[index].format),
                   static_cast<unsigned long long>(upload_pixels[index].size()));
      return false;
    }
  }
  for (Rt12NrdTexture& source : inputs) {
    if (source.resource == nullptr) continue;
    std::vector<D3D12_RESOURCE_BARRIER> barriers;
    NrdTransition(slot.command_list.Get(), &source, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, &barriers);
    if (!barriers.empty()) slot.command_list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
  }
  for (nrd::ResourceType type : prep_output_types) {
    std::vector<D3D12_RESOURCE_BARRIER> barriers;
    NrdTransition(slot.command_list.Get(), &slot.resources[static_cast<uint32_t>(type)],
                  D3D12_RESOURCE_STATE_UNORDERED_ACCESS, &barriers);
    if (!barriers.empty()) slot.command_list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
  }
  ComPtr<ID3D12RootSignature> prep_root;
  ComPtr<ID3D12PipelineState> prep_pso;
  D3D12_DESCRIPTOR_RANGE ranges[2] = {};
  ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
  ranges[0].NumDescriptors = 10u;
  ranges[0].OffsetInDescriptorsFromTableStart = 0u;
  ranges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
  ranges[1].NumDescriptors = 5u;
  ranges[1].OffsetInDescriptorsFromTableStart = 0u;
  D3D12_ROOT_PARAMETER parameters[3] = {};
  parameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
  parameters[0].Constants.ShaderRegister = 0u;
  parameters[0].Constants.Num32BitValues = 10u;
  parameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
  parameters[1].DescriptorTable.NumDescriptorRanges = 1u;
  parameters[1].DescriptorTable.pDescriptorRanges = &ranges[0];
  parameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
  parameters[2].DescriptorTable.NumDescriptorRanges = 1u;
  parameters[2].DescriptorTable.pDescriptorRanges = &ranges[1];
  D3D12_ROOT_SIGNATURE_DESC root_desc = {};
  root_desc.NumParameters = 3u;
  root_desc.pParameters = parameters;
  ComPtr<ID3DBlob> root_blob, root_errors;
  if (!Check(D3D12SerializeRootSignature(&root_desc, D3D_ROOT_SIGNATURE_VERSION_1, &root_blob, &root_errors),
             "serialize N1 prep root")) return false;
  if (!Check(device->CreateRootSignature(0u, root_blob->GetBufferPointer(), root_blob->GetBufferSize(), IID_PPV_ARGS(&prep_root)),
             "create N1 prep root")) return false;
  D3D12_COMPUTE_PIPELINE_STATE_DESC prep_pipeline = {};
  prep_pipeline.pRootSignature = prep_root.Get();
  prep_pipeline.CS = {__world_rtxdi_nrd_prep.data(), __world_rtxdi_nrd_prep.size()};
  if (!Check(device->CreateComputePipelineState(&prep_pipeline, IID_PPV_ARGS(&prep_pso)), "create N1 prep PSO")) return false;
  ID3D12DescriptorHeap* prep_heaps[] = {prep_heap.Get()};
  slot.command_list->SetDescriptorHeaps(1u, prep_heaps);
  slot.command_list->SetComputeRootSignature(prep_root.Get());
  slot.command_list->SetPipelineState(prep_pso.Get());
  const std::array<uint32_t, 10u> prep_constants = {width, height, 0u, 0u,
                                                    std::bit_cast<uint32_t>(0.125f),
                                                    std::bit_cast<uint32_t>(-0.25f), 0u, 0u, 1u, 0u};
  slot.command_list->SetComputeRoot32BitConstants(0u, 10u, prep_constants.data(), 0u);
  slot.command_list->SetComputeRootDescriptorTable(1u, heap_gpu);
  slot.command_list->SetComputeRootDescriptorTable(2u, {heap_gpu.ptr + static_cast<UINT64>(10u) * descriptor_size});
  slot.command_list->Dispatch((width + 7u) / 8u, (height + 7u) / 8u, 1u);
  for (Rt12NrdTexture& source : inputs) {
    if (source.resource == nullptr) continue;
    std::vector<D3D12_RESOURCE_BARRIER> barriers;
    NrdTransition(slot.command_list.Get(), &source, D3D12_RESOURCE_STATE_COMMON, &barriers);
    if (!barriers.empty()) slot.command_list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
  }
  for (nrd::ResourceType type : prep_output_types) slot.resources[static_cast<uint32_t>(type)].unordered_access_written = true;

  nrd::CommonSettings common = {};
  for (uint32_t diagonal = 0u; diagonal < 4u; ++diagonal) {
    common.viewToClipMatrix[diagonal * 5u] = common.viewToClipMatrixPrev[diagonal * 5u] = 1.0f;
    common.worldToViewMatrix[diagonal * 5u] = common.worldToViewMatrixPrev[diagonal * 5u] = 1.0f;
  }
  common.resourceSize[0] = common.resourceSizePrev[0] = common.rectSize[0] = common.rectSizePrev[0] = width;
  common.resourceSize[1] = common.resourceSizePrev[1] = common.rectSize[1] = common.rectSizePrev[1] = height;
  common.motionVectorScale[0] = common.motionVectorScale[1] = 1.0f;
  common.accumulationMode = nrd::AccumulationMode::RESTART;
  common.frameIndex = 0u;
  if (nrd::SetCommonSettings(*generation->Instance(), common) != nrd::Result::SUCCESS) return false;
  nrd::RelaxSettings relax = {};
  relax.hitDistanceReconstructionMode = nrd::HitDistanceReconstructionMode::OFF;
  relax.minMaterialForDiffuse = 0.0f;
  relax.minMaterialForSpecular = 0.0f;
  if (nrd::SetDenoiserSettings(*generation->Instance(), 1u, &relax) != nrd::Result::SUCCESS) return false;
  const nrd::Identifier identifier = 1u;
  const nrd::DispatchDesc* frame_dispatches = nullptr;
  uint32_t frame_dispatch_count = 0u;
  if (nrd::GetComputeDispatches(*generation->Instance(), &identifier, 1u, frame_dispatches, frame_dispatch_count)
          != nrd::Result::SUCCESS
      || frame_dispatches == nullptr || frame_dispatch_count == 0u) return false;
  slot.dispatch_descriptors = slot.descriptors;
  slot.dispatch_constants = slot.constants;
  slot.dispatch_count = frame_dispatch_count;
  // The prep pass wrote external input UAVs, so preserve their tracked state for the shared recorder.
  for (nrd::ResourceType type : prep_output_types) {
    slot.resources[static_cast<uint32_t>(type)].state = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
  }
  if (!Rt12NrdRecordDispatches(device, slot.command_list.Get(), instance_desc, frame_dispatches,
                               frame_dispatch_count, pipelines, pools, &slot, descriptor_stride)) {
    std::fprintf(stderr, "FAIL shared N1 prep RELAX dispatch recorder count=%u\n", frame_dispatch_count);
    return false;
  }
  const auto copy_texture_to_readback = [&](Rt12NrdTexture* texture, ID3D12Resource* readback,
                                            D3D12_PLACED_SUBRESOURCE_FOOTPRINT* footprint_out) {
    if (texture == nullptr || !texture->resource || readback == nullptr || footprint_out == nullptr) return false;
    const D3D12_RESOURCE_DESC desc = texture->resource->GetDesc();
    UINT rows = 0u;
    UINT64 row_bytes = 0u, total_bytes = 0u;
    device->GetCopyableFootprints(&desc, 0u, 1u, 0u, footprint_out, &rows, &row_bytes, &total_bytes);
    std::vector<D3D12_RESOURCE_BARRIER> barriers;
    NrdTransition(slot.command_list.Get(), texture, D3D12_RESOURCE_STATE_COPY_SOURCE, &barriers);
    if (!barriers.empty()) slot.command_list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
    D3D12_TEXTURE_COPY_LOCATION source = {};
    source.pResource = texture->resource.Get();
    source.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    D3D12_TEXTURE_COPY_LOCATION destination = {};
    destination.pResource = readback;
    destination.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    destination.PlacedFootprint = *footprint_out;
    slot.command_list->CopyTextureRegion(&destination, 0u, 0u, 0u, &source, nullptr);
    barriers.clear();
    NrdTransition(slot.command_list.Get(), texture, D3D12_RESOURCE_STATE_COMMON, &barriers);
    if (!barriers.empty()) slot.command_list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
    return rows == height && total_bytes <= readback->GetDesc().Width;
  };
  Rt12NrdTexture publication_texture;
  if (!NrdCreateTexture(device, width, height, DXGI_FORMAT_R16G16B16A16_FLOAT, true, &publication_texture)) return false;
  Rt12NrdTexture status_texture = inputs[6];
  Rt12NrdTexture hit_validity_texture;
  hit_validity_texture.resource = hit_validity_input;
  hit_validity_texture.width = width;
  hit_validity_texture.height = height;
  hit_validity_texture.state = D3D12_RESOURCE_STATE_GENERIC_READ;
  D3D12_DESCRIPTOR_HEAP_DESC publication_heap_desc = {};
  publication_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
  publication_heap_desc.NumDescriptors = 5u;
  publication_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
  ComPtr<ID3D12DescriptorHeap> publication_heap;
  ComPtr<ID3D12Resource> publication_constants, publication_readback;
  ComPtr<ID3D12RootSignature> publication_root;
  ComPtr<ID3D12PipelineState> publication_pso;
  if (!Check(device->CreateDescriptorHeap(&publication_heap_desc, IID_PPV_ARGS(&publication_heap)),
             "create diffuse publication heap")
      || !NrdCreateUploadBuffer(device, 256u, &publication_constants)
      || !Rt12NrdCreateDiffusePublicationPipeline(device, __world_rtxdi_nrd_publish.data(),
                                                  __world_rtxdi_nrd_publish.size(),
                                                  &publication_root, &publication_pso)
      || !Rt12NrdRecordDiffusePublication(
          device, slot.command_list.Get(), publication_root.Get(), publication_pso.Get(), publication_heap.Get(),
          publication_constants.Get(),
          &slot.resources[static_cast<uint32_t>(nrd::ResourceType::OUT_DIFF_RADIANCE_HITDIST)],
          &slot.resources[static_cast<uint32_t>(nrd::ResourceType::IN_VIEWZ)], &status_texture,
          &hit_validity_texture, &publication_texture, width, height)) {
    std::fprintf(stderr, "FAIL N3c diffuse publication record\n");
    return false;
  }
  const D3D12_RESOURCE_DESC publication_desc = publication_texture.resource->GetDesc();
  D3D12_PLACED_SUBRESOURCE_FOOTPRINT publication_footprint = {};
  UINT publication_rows = 0u;
  UINT64 publication_row_bytes = 0u, publication_total_bytes = 0u;
  device->GetCopyableFootprints(&publication_desc, 0u, 1u, 0u, &publication_footprint,
                                &publication_rows, &publication_row_bytes, &publication_total_bytes);
  if (!NrdCreateReadbackBuffer(device, publication_total_bytes, &publication_readback)
      || !copy_texture_to_readback(&publication_texture, publication_readback.Get(), &publication_footprint)) return false;
  ComPtr<ID3D12Resource> prep_diffuse_readback, prep_specular_readback, prep_viewz_readback;
  const std::array<nrd::ResourceType, 4u> check_types = {nrd::ResourceType::IN_DIFF_RADIANCE_HITDIST,
                                                         nrd::ResourceType::IN_SPEC_RADIANCE_HITDIST,
                                                         nrd::ResourceType::IN_VIEWZ,
                                                         nrd::ResourceType::IN_MV};
  std::array<ComPtr<ID3D12Resource>, 4u> check_readbacks = {};
  std::array<D3D12_PLACED_SUBRESOURCE_FOOTPRINT, 4u> check_footprints = {};
  bool prep_inputs_valid = true;
  for (uint32_t index = 0u; index < check_types.size(); ++index) {
    Rt12NrdTexture& texture = slot.resources[static_cast<uint32_t>(check_types[index])];
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint = {};
    UINT rows = 0u;
    UINT64 row_bytes = 0u, total_bytes = 0u;
    const D3D12_RESOURCE_DESC texture_desc = texture.resource->GetDesc();
    device->GetCopyableFootprints(&texture_desc, 0u, 1u, 0u, &footprint, &rows, &row_bytes, &total_bytes);
    if (!NrdCreateReadbackBuffer(device, total_bytes, &check_readbacks[index])
        || !copy_texture_to_readback(&texture, check_readbacks[index].Get(), &check_footprints[index])) return false;
  }
  std::array<D3D12_PLACED_SUBRESOURCE_FOOTPRINT, 2u> output_footprints = {};
  if (!copy_texture_to_readback(&slot.resources[static_cast<uint32_t>(nrd::ResourceType::OUT_DIFF_RADIANCE_HITDIST)],
                                slot.diffuse_readback.Get(), &output_footprints[0])
      || !copy_texture_to_readback(&slot.resources[static_cast<uint32_t>(nrd::ResourceType::OUT_SPEC_RADIANCE_HITDIST)],
                                   slot.specular_readback.Get(), &output_footprints[1])) return false;
  if (!Check(slot.command_list->Close(), "close N1 prep + RELAX list")) return false;
  ID3D12CommandList* lists[] = {slot.command_list.Get()};
  queue->ExecuteCommandLists(1u, lists);
  if (!NrdWaitForFence(queue, fence, ++*fence_value)) return false;
  for (uint32_t index = 0u; index < check_types.size(); ++index) {
    void* mapped = nullptr;
    const D3D12_RANGE range = {0u, static_cast<SIZE_T>(check_readbacks[index]->GetDesc().Width)};
    if (FAILED(check_readbacks[index]->Map(0u, &range, &mapped))) return false;
    const uint8_t* pixels = static_cast<const uint8_t*>(mapped);
    bool valid = true;
    for (uint32_t pixel = 0u; pixel < pixel_count && valid; ++pixel) {
      const uint32_t x = pixel % width, y = pixel / width;
      const uint8_t* value = pixels + check_footprints[index].Offset
                             + static_cast<size_t>(y) * check_footprints[index].Footprint.RowPitch;
      if (index < 2u) {
        const auto* half = reinterpret_cast<const uint16_t*>(value) + x * 4u;
        for (uint32_t channel = 0u; channel < 4u; ++channel) {
          const float component = NrdHalfToFloat(half[channel]);
          const uint32_t scenario = pixel & 3u;
          const float expected = index == 0u && scenario == 0u && channel == 0u   ? 0.5f
                                 : index == 0u && scenario == 0u && channel == 1u ? 0.25f
                                 : index == 0u && scenario == 0u && channel == 2u ? 0.125f
                                 : index == 0u && scenario < 2u && channel == 3u  ? 3.25f
                                                                                  : 0.0f;
          if (!std::isfinite(component) || std::abs(component - expected) > 0.002f) {
            std::fprintf(stderr, "N1 prep mismatch guide=%u pixel=%u channel=%u status=%u actual=%g expected=%g\n",
                         index, pixel, channel, n1_status[pixel], component, expected);
            valid = false;
          }
        }
      } else if (index == 2u) {
        const auto* viewz = reinterpret_cast<const float*>(value) + x;
        const uint32_t scenario = pixel & 3u;
        const float expected = scenario < 2u ? 70000.0f : 1000000.0f;
        if (!std::isfinite(*viewz) || std::abs(*viewz - expected) > 0.5f) valid = false;
      } else {
        const auto* half = reinterpret_cast<const uint16_t*>(value) + x * 2u;
        const float expected_x = (0.5f - 0.125f) / static_cast<float>(width);
        const float expected_y = 0.0f;
        if (!std::isfinite(NrdHalfToFloat(half[0])) || !std::isfinite(NrdHalfToFloat(half[1]))
            || std::abs(NrdHalfToFloat(half[0]) - expected_x) > 0.0001f
            || std::abs(NrdHalfToFloat(half[1]) - expected_y) > 0.0001f) valid = false;
      }
    }
    const D3D12_RANGE no_write = {0u, 0u};
    check_readbacks[index]->Unmap(0u, &no_write);
    if (!valid) {
      std::fprintf(stderr, "FAIL N1 prep guide/status output index=%u\n", index);
      prep_inputs_valid = false;
    }
  }
  if (!prep_inputs_valid) return false;
  void* publication_mapped = nullptr;
  const D3D12_RANGE publication_read_range = {0u, static_cast<SIZE_T>(publication_readback->GetDesc().Width)};
  if (FAILED(publication_readback->Map(0u, &publication_read_range, &publication_mapped))) return false;
  const uint8_t* publication_bytes = static_cast<const uint8_t*>(publication_mapped);
  uint32_t env_publications = 0u, character_publications = 0u;
  bool publication_valid = true;
  for (uint32_t pixel = 0u; pixel < pixel_count && publication_valid; ++pixel) {
    const uint32_t x = pixel % width, y = pixel / width, scenario = pixel & 7u;
    const uint8_t* row = publication_bytes + publication_footprint.Offset
                         + static_cast<size_t>(y) * publication_footprint.Footprint.RowPitch;
    const auto* half = reinterpret_cast<const uint16_t*>(row) + x * 4u;
    const std::array<float, 4u> value = {NrdHalfToFloat(half[0]), NrdHalfToFloat(half[1]),
                                         NrdHalfToFloat(half[2]), NrdHalfToFloat(half[3])};
    const float expected_class = scenario == 0u || scenario == 1u   ? 1.0f
                                 : scenario == 4u || scenario == 5u ? 2.0f
                                                                    : 0.0f;
    if (!std::isfinite(value[0]) || !std::isfinite(value[1]) || !std::isfinite(value[2])
        || !std::isfinite(value[3]) || value[3] != expected_class
        || value[0] < 0.0f || value[1] < 0.0f || value[2] < 0.0f
        || value[0] > 65504.0f || value[1] > 65504.0f || value[2] > 65504.0f
        || (expected_class == 0.0f && (value[0] != 0.0f || value[1] != 0.0f || value[2] != 0.0f))) {
      std::fprintf(stderr, "N3c publication mismatch pixel=%u status=%u hit=%u rgba=(%g,%g,%g,%g) expected-class=%g\n",
                   pixel, n1_status[pixel], traced_hit_validity[pixel], value[0], value[1], value[2], value[3],
                   expected_class);
      publication_valid = false;
      break;
    }
    env_publications += expected_class == 1.0f ? 1u : 0u;
    character_publications += expected_class == 2.0f ? 1u : 0u;
  }
  const D3D12_RANGE no_publication_write = {0u, 0u};
  publication_readback->Unmap(0u, &no_publication_write);
  if (!publication_valid || env_publications == 0u || character_publications == 0u) return false;
  if (!NrdReadbackOutput(device,
                         slot.resources[static_cast<uint32_t>(nrd::ResourceType::OUT_DIFF_RADIANCE_HITDIST)].resource.Get(),
                         slot.diffuse_readback.Get(), width, height, "N1-prepared RELAX diffuse")
      || !NrdReadbackOutput(device,
                            slot.resources[static_cast<uint32_t>(nrd::ResourceType::OUT_SPEC_RADIANCE_HITDIST)].resource.Get(),
                            slot.specular_readback.Get(), width, height, "N1-prepared zero-spec RELAX specular")) return false;
  if (!debug_layer_enabled) return false;
  const UINT64 messages = info_queue->GetNumStoredMessagesAllowedByRetrievalFilter();
  if (messages != 0u) {
    std::fprintf(stderr, "FAIL N1 prep + RELAX D3D12 InfoQueue messages=%llu\n",
                 static_cast<unsigned long long>(messages));
    return false;
  }
  std::printf("PASS N1 prep + RELAX + N3c publication env=%u character=%u invalid-zero=%u (%ux%u)\n",
              env_publications, character_publications, pixel_count - env_publications - character_publications,
              width, height);
  return true;
}

bool RunCameraProbeSmoke(ID3D12Device* device, ID3D12CommandQueue* queue, ID3D12InfoQueue* info_queue) {
  using namespace falcom_world::rt12;
  constexpr uint32_t width = 64u, height = 36u, sample_count = 32u * 18u;
  std::array<Rt12NrdTexture, 5u> inputs = {};
  const std::array<DXGI_FORMAT, 5u> formats = {DXGI_FORMAT_R32_FLOAT, DXGI_FORMAT_R32_FLOAT,
                                               DXGI_FORMAT_R32G32B32A32_FLOAT, DXGI_FORMAT_R32G32B32A32_UINT,
                                               DXGI_FORMAT_R32G32B32A32_UINT};
  for (uint32_t index = 0u; index < inputs.size(); ++index) {
    if (!NrdCreateTexture(device, width, height, formats[index], false, &inputs[index])) return false;
  }

  D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
  heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
  heap_desc.NumDescriptors = 6u;
  heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
  ComPtr<ID3D12DescriptorHeap> descriptors;
  if (!Check(device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&descriptors)), "create camera probe heap")) return false;
  const uint32_t descriptor_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
  auto cpu_at = [&](uint32_t index) {
    D3D12_CPU_DESCRIPTOR_HANDLE handle = descriptors->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += static_cast<SIZE_T>(index) * descriptor_size;
    return handle;
  };
  for (uint32_t index = 0u; index < inputs.size(); ++index) {
    D3D12_SHADER_RESOURCE_VIEW_DESC view = {};
    view.Format = formats[index];
    view.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    view.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    view.Texture2D.MipLevels = 1u;
    device->CreateShaderResourceView(inputs[index].resource.Get(), &view, cpu_at(index));
  }

  D3D12_DESCRIPTOR_RANGE ranges[2] = {};
  ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
  ranges[0].NumDescriptors = 5u;
  ranges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
  ranges[1].NumDescriptors = 1u;
  D3D12_ROOT_PARAMETER parameters[3] = {};
  parameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
  parameters[0].Descriptor.ShaderRegister = 0u;
  parameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
  parameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
  parameters[1].DescriptorTable.NumDescriptorRanges = 1u;
  parameters[1].DescriptorTable.pDescriptorRanges = &ranges[0];
  parameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
  parameters[2].DescriptorTable.NumDescriptorRanges = 1u;
  parameters[2].DescriptorTable.pDescriptorRanges = &ranges[1];
  D3D12_ROOT_SIGNATURE_DESC root_desc = {};
  root_desc.NumParameters = 3u;
  root_desc.pParameters = parameters;
  ComPtr<ID3DBlob> root_blob, root_errors;
  if (!Check(D3D12SerializeRootSignature(&root_desc, D3D_ROOT_SIGNATURE_VERSION_1, &root_blob, &root_errors),
             "serialize camera probe root")) return false;
  ComPtr<ID3D12RootSignature> root;
  if (!Check(device->CreateRootSignature(0u, root_blob->GetBufferPointer(), root_blob->GetBufferSize(),
                                         IID_PPV_ARGS(&root)),
             "create camera probe root")) return false;
  D3D12_COMPUTE_PIPELINE_STATE_DESC pso_desc = {};
  pso_desc.pRootSignature = root.Get();
  pso_desc.CS = {__world_rtxdi_nrd_probe.data(), __world_rtxdi_nrd_probe.size()};
  ComPtr<ID3D12PipelineState> pso;
  if (!Check(device->CreateComputePipelineState(&pso_desc, IID_PPV_ARGS(&pso)), "create camera probe PSO")) return false;

  ComPtr<ID3D12Resource> constants, output, readback, timestamps;
  if (!NrdCreateUploadBuffer(device, 512u, &constants)
      || !NrdCreateBuffer(device, D3D12_HEAP_TYPE_DEFAULT, sample_count * 2u * sizeof(uint32_t),
                          D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON, &output)
      || !NrdCreateReadbackBuffer(device, sample_count * 2u * sizeof(uint32_t), &readback)
      || !NrdCreateReadbackBuffer(device, 2u * sizeof(uint64_t), &timestamps)) return false;
  D3D12_UNORDERED_ACCESS_VIEW_DESC output_view = {};
  output_view.Format = DXGI_FORMAT_UNKNOWN;
  output_view.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
  output_view.Buffer.NumElements = sample_count * 2u;
  output_view.Buffer.StructureByteStride = sizeof(uint32_t);
  device->CreateUnorderedAccessView(output.Get(), nullptr, &output_view, cpu_at(5u));

  ComPtr<ID3D12QueryHeap> query_heap;
  D3D12_QUERY_HEAP_DESC query_desc = {};
  query_desc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
  query_desc.Count = 2u;
  if (!Check(device->CreateQueryHeap(&query_desc, IID_PPV_ARGS(&query_heap)), "create camera probe timestamps")) return false;
  ComPtr<ID3D12CommandAllocator> allocator;
  ComPtr<ID3D12GraphicsCommandList> list;
  if (!Check(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator)),
             "create camera probe allocator")
      || !Check(device->CreateCommandList(0u, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator.Get(), pso.Get(),
                                          IID_PPV_ARGS(&list)),
                "create camera probe list")) return false;

  constexpr float near_z = 0.1f, far_z = 100.0f;
  const auto make_projection = [&](float jitter_x, float jitter_y) {
    const float p22 = far_z / (near_z - far_z);
    const float p32 = near_z * far_z / (near_z - far_z);
    return falcom_world::rtxdi::CameraMatrix{1.0f, 0.0f, 0.0f, 0.0f,
                                             0.0f, 1.0f, 0.0f, 0.0f,
                                             -2.0f * jitter_x / width, 2.0f * jitter_y / height, p22, -1.0f,
                                             0.0f, 0.0f, p32, 0.0f};
  };
  constexpr float current_jitter_x = 0.1f, current_jitter_y = -0.1f;
  constexpr float previous_jitter_x = 0.3f, previous_jitter_y = 0.2f;
  const auto current_projection = make_projection(current_jitter_x, current_jitter_y);
  const auto previous_projection = make_projection(previous_jitter_x, previous_jitter_y);
  auto non_jittered_projection = make_projection(0.0f, 0.0f);
  falcom_world::rtxdi::CameraMatrix current_inverse = {}, previous_inverse = {};
  if (!falcom_world::rtxdi::InvertCameraMatrix(current_projection, &current_inverse)
      || !falcom_world::rtxdi::InvertCameraMatrix(previous_projection, &previous_inverse)) return false;
  const auto pack_matrix = [](const falcom_world::rtxdi::CameraMatrix& matrix, float* output_values) {
    std::copy(matrix.begin(), matrix.end(), output_values);
  };

  std::vector<uint8_t> depth_pixels(static_cast<size_t>(width) * height * sizeof(float));
  const float plane_depth = far_z / (far_z - near_z) - (near_z * far_z / (far_z - near_z)) / 10.0f;
  for (size_t pixel = 0u; pixel < static_cast<size_t>(width) * height; ++pixel) {
    std::memcpy(depth_pixels.data() + pixel * sizeof(float), &plane_depth, sizeof(plane_depth));
  }
  std::vector<uint8_t> motion_pixels(static_cast<size_t>(width) * height * sizeof(float) * 4u, 0u);
  std::vector<uint8_t> normal_pixels(static_cast<size_t>(width) * height * sizeof(uint32_t) * 4u, 0u);
  const std::array<uint32_t, 4u> normal = {1024u, 1024u, 3072u, 8u};
  for (size_t pixel = 0u; pixel < static_cast<size_t>(width) * height; ++pixel) {
    std::memcpy(normal_pixels.data() + pixel * sizeof(normal), normal.data(), sizeof(normal));
  }

  Rt12NrdSlot staging_slot;
  if (!NrdUploadTexture(device, list.Get(), &staging_slot, &inputs[0], depth_pixels)
      || !NrdUploadTexture(device, list.Get(), &staging_slot, &inputs[1], depth_pixels)
      || !NrdUploadTexture(device, list.Get(), &staging_slot, &inputs[2], motion_pixels)
      || !NrdUploadTexture(device, list.Get(), &staging_slot, &inputs[3], normal_pixels)
      || !NrdUploadTexture(device, list.Get(), &staging_slot, &inputs[4], normal_pixels)) return false;
  D3D12_RESOURCE_BARRIER input_barriers[5] = {};
  for (uint32_t index = 0u; index < inputs.size(); ++index) {
    input_barriers[index].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    input_barriers[index].Transition.pResource = inputs[index].resource.Get();
    input_barriers[index].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    input_barriers[index].Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
    input_barriers[index].Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    inputs[index].state = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
  }
  list->ResourceBarrier(static_cast<UINT>(inputs.size()), input_barriers);
  if (!Check(list->Close(), "close camera probe upload list")) return false;
  ID3D12CommandList* upload_lists[] = {list.Get()};
  queue->ExecuteCommandLists(1u, upload_lists);
  ComPtr<ID3D12Fence> fence;
  if (!Check(device->CreateFence(0u, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)), "create camera probe fence")) return false;
  uint64_t fence_value = 1u;
  if (!NrdWaitForFence(queue, fence.Get(), fence_value)) return false;
  staging_slot.staging.clear();

  const std::array<std::array<float, 2u>, 2u> corrections = {{{0.2f, 0.3f}, {0.4f, 0.6f}}};
  float best_gpu_microseconds = -1.0f;
  falcom_world::rtxdi::CameraQualification qualification = {};
  double record_cpu_microseconds = 0.0;
  double readback_cpu_microseconds = 0.0;
  for (uint32_t scenario = 0u; scenario < 7u; ++scenario) {
    const uint32_t expected_correction = scenario == 4u ? 1u : 0u;
    const bool reject_previous_class = scenario == 5u;
    const bool reject_previous_depth = scenario == 6u;
    const auto record_start = std::chrono::steady_clock::now();
    Rt12NrdProbeConstants probe_constants = {};
    probe_constants.resource_size[0] = width;
    probe_constants.resource_size[1] = height;
    probe_constants.current_jitter_pixels[0] = current_jitter_x;
    probe_constants.current_jitter_pixels[1] = current_jitter_y;
    probe_constants.previous_jitter_pixels[0] = previous_jitter_x;
    probe_constants.previous_jitter_pixels[1] = previous_jitter_y;
    for (uint32_t index = 0u; index < 2u; ++index) {
      probe_constants.motion_correction_pixels[index][0] = corrections[index][0];
      probe_constants.motion_correction_pixels[index][1] = corrections[index][1];
    }
    falcom_world::rtxdi::CameraMatrix previous_view = {1.0f, 0.0f, 0.0f, 0.0f,
                                                       0.0f, 1.0f, 0.0f, 0.0f,
                                                       0.0f, 0.0f, 1.0f, 0.0f,
                                                       -0.05f, 0.0f, 0.0f, 1.0f};
    const auto previous_jittered_vp = falcom_world::rtxdi::MultiplyCameraMatrices(previous_view, previous_projection);
    const auto previous_non_jittered_vp = falcom_world::rtxdi::MultiplyCameraMatrices(previous_view,
                                                                                      non_jittered_projection);
    if (!falcom_world::rtxdi::InvertCameraMatrix(previous_jittered_vp, &previous_inverse)) return false;
    pack_matrix(current_inverse, probe_constants.current_inverse_view_projection);
    pack_matrix(previous_inverse, probe_constants.previous_inverse_view_projection);
    pack_matrix(non_jittered_projection, probe_constants.current_view_projection_non_jittered);
    pack_matrix(previous_non_jittered_vp, probe_constants.previous_view_projection_non_jittered);
    pack_matrix(previous_view, probe_constants.previous_view);
    void* constants_map = nullptr;
    const D3D12_RANGE no_read = {0u, 0u};
    if (!Check(constants->Map(0u, &no_read, &constants_map), "map camera probe constants")) return false;
    std::memset(constants_map, 0, 512u);
    std::memcpy(constants_map, &probe_constants, sizeof(probe_constants));
    constants->Unmap(0u, nullptr);

    std::vector<uint8_t> selected_motion(static_cast<size_t>(width) * height * sizeof(float) * 4u, 0u);
    const std::array<float, 2u> expected_motion = {-0.16f, 0.0f};
    const std::array<uint32_t, 4u> mismatched_previous_normal = {1024u, 1024u, 3072u, 0u};
    std::vector<uint8_t> previous_normal_pixels;
    if (reject_previous_class) previous_normal_pixels.resize(normal_pixels.size());
    std::vector<uint8_t> invalid_previous_depth;
    if (reject_previous_depth) invalid_previous_depth.resize(depth_pixels.size(), 0u);
    for (size_t pixel = 0u; pixel < static_cast<size_t>(width) * height; ++pixel) {
      const std::array<float, 2u> game_motion = {corrections[expected_correction][0] + expected_motion[0],
                                                 corrections[expected_correction][1] + expected_motion[1]};
      std::memcpy(selected_motion.data() + pixel * sizeof(float) * 4u, game_motion.data(), sizeof(game_motion));
      if (reject_previous_class) {
        std::memcpy(previous_normal_pixels.data() + pixel * sizeof(mismatched_previous_normal),
                    mismatched_previous_normal.data(), sizeof(mismatched_previous_normal));
      }
    }
    if (!Check(list->Reset(allocator.Get(), pso.Get()), "reset camera probe list")) return false;
    if (!NrdUploadTexture(device, list.Get(), &staging_slot, &inputs[2], selected_motion)) return false;
    if (reject_previous_class
        && !NrdUploadTexture(device, list.Get(), &staging_slot, &inputs[4], previous_normal_pixels)) return false;
    if (reject_previous_depth
        && !NrdUploadTexture(device, list.Get(), &staging_slot, &inputs[4], normal_pixels)) return false;
    if (reject_previous_depth
        && !NrdUploadTexture(device, list.Get(), &staging_slot, &inputs[1], invalid_previous_depth)) return false;
    D3D12_RESOURCE_BARRIER to_shader[3] = {};
    to_shader[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    to_shader[0].Transition.pResource = inputs[2].resource.Get();
    to_shader[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    to_shader[0].Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
    to_shader[0].Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    inputs[2].state = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    UINT to_shader_count = 1u;
    if (reject_previous_class || reject_previous_depth) {
      to_shader[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
      to_shader[1].Transition.pResource = inputs[4].resource.Get();
      to_shader[1].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
      to_shader[1].Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
      to_shader[1].Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
      inputs[4].state = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
      to_shader_count = 2u;
    }
    if (reject_previous_depth) {
      to_shader[to_shader_count].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
      to_shader[to_shader_count].Transition.pResource = inputs[1].resource.Get();
      to_shader[to_shader_count].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
      to_shader[to_shader_count].Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
      to_shader[to_shader_count].Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
      inputs[1].state = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
      ++to_shader_count;
    }
    list->ResourceBarrier(to_shader_count, to_shader);
    D3D12_RESOURCE_BARRIER to_uav = {};
    to_uav.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    to_uav.Transition.pResource = output.Get();
    to_uav.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    to_uav.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
    to_uav.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    list->ResourceBarrier(1u, &to_uav);
    ID3D12DescriptorHeap* visible_heaps[] = {descriptors.Get()};
    list->SetDescriptorHeaps(1u, visible_heaps);
    list->SetComputeRootSignature(root.Get());
    list->SetComputeRootConstantBufferView(0u, constants->GetGPUVirtualAddress());
    const D3D12_GPU_DESCRIPTOR_HANDLE table = descriptors->GetGPUDescriptorHandleForHeapStart();
    list->SetComputeRootDescriptorTable(1u, table);
    D3D12_GPU_DESCRIPTOR_HANDLE output_table = table;
    output_table.ptr += static_cast<UINT64>(5u) * descriptor_size;
    list->SetComputeRootDescriptorTable(2u, output_table);
    list->SetPipelineState(pso.Get());
    list->EndQuery(query_heap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 0u);
    list->Dispatch(4u, 3u, 1u);
    list->EndQuery(query_heap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 1u);
    D3D12_RESOURCE_BARRIER output_barriers[2] = {};
    output_barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    output_barriers[0].UAV.pResource = output.Get();
    output_barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    output_barriers[1].Transition.pResource = output.Get();
    output_barriers[1].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    output_barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    output_barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
    list->ResourceBarrier(2u, output_barriers);
    list->CopyBufferRegion(readback.Get(), 0u, output.Get(), 0u, sample_count * 2u * sizeof(uint32_t));
    D3D12_RESOURCE_BARRIER output_to_common = {};
    output_to_common.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    output_to_common.Transition.pResource = output.Get();
    output_to_common.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    output_to_common.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
    output_to_common.Transition.StateAfter = D3D12_RESOURCE_STATE_COMMON;
    list->ResourceBarrier(1u, &output_to_common);
    list->ResolveQueryData(query_heap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 0u, 2u, timestamps.Get(), 0u);
    if (!Check(list->Close(), "close camera probe dispatch list")) return false;
    record_cpu_microseconds += std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - record_start).count();
    ID3D12CommandList* dispatch_lists[] = {list.Get()};
    queue->ExecuteCommandLists(1u, dispatch_lists);
    if (!NrdWaitForFence(queue, fence.Get(), ++fence_value)) return false;

    const auto readback_start = std::chrono::steady_clock::now();
    void* result_map = nullptr;
    const D3D12_RANGE read_range = {0u, sample_count * 2u * sizeof(uint32_t)};
    if (!Check(readback->Map(0u, &read_range, &result_map), "map camera probe output")) return false;
    const auto* result_bits = static_cast<const uint32_t*>(result_map);
    std::array<float, sample_count> residuals[2] = {};
    std::array<uint32_t, 2u> valid_count = {};
    std::array<uint32_t, 16u> rejected = {};
    for (uint32_t sample = 0u; sample < sample_count; ++sample) {
      for (uint32_t candidate = 0u; candidate < 2u; ++candidate) {
        const uint32_t bits = result_bits[sample * 2u + candidate];
        const float error = std::bit_cast<float>(bits);
        if (std::isfinite(error))
          residuals[candidate][valid_count[candidate]++] = error;
        else if ((bits & 0xfffffff0u) == 0x7fc00000u && (bits & 0xfu) < rejected.size())
          ++rejected[bits & 0xfu];
      }
    }
    const D3D12_RANGE no_write = {0u, 0u};
    readback->Unmap(0u, &no_write);
    uint32_t selected = UINT32_MAX;
    uint32_t qualified_candidates = 0u;
    for (uint32_t candidate = 0u; candidate < 2u; ++candidate) {
      if (valid_count[candidate] < 64u) continue;
      std::sort(residuals[candidate].begin(), residuals[candidate].begin() + valid_count[candidate]);
      const float median = residuals[candidate][valid_count[candidate] / 2u];
      const float p90 = residuals[candidate][(valid_count[candidate] * 9u) / 10u];
      if (median <= 0.35f && p90 <= 1.0f) {
        selected = candidate;
        ++qualified_candidates;
      }
    }
    const bool reject_all = reject_previous_class || reject_previous_depth;
    const uint32_t expected_reason = reject_previous_class ? 8u : (reject_previous_depth ? 9u : 0u);
    if (reject_all ? (valid_count[0] != 0u || valid_count[1] != 0u || rejected[expected_reason] != sample_count * 2u)
                   : (selected != expected_correction || qualified_candidates != 1u)) {
      std::fprintf(stderr, "FAIL NRD camera probe scenario=%u expected=%u actual=%u valid=%u/%u\n",
                   scenario,
                   expected_correction, selected, valid_count[0], valid_count[1]);
      if (valid_count[0] != 0u) {
        std::sort(residuals[0].begin(), residuals[0].begin() + valid_count[0]);
        std::fprintf(stderr, "NRD camera probe diagnostic finite median=%.4f max=%.4f\n",
                     residuals[0][valid_count[0] / 2u], residuals[0][valid_count[0] - 1u]);
      }
      for (uint32_t reason = 1u; reason < rejected.size(); ++reason) {
        if (rejected[reason] != 0u) std::fprintf(stderr, "NRD camera probe rejected reason=%u count=%u\n", reason,
                                                 rejected[reason] / 2u);
      }
      return false;
    }
    readback_cpu_microseconds += std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - readback_start).count();
    void* timestamp_map = nullptr;
    const D3D12_RANGE timestamp_range = {0u, 2u * sizeof(uint64_t)};
    if (!Check(timestamps->Map(0u, &timestamp_range, &timestamp_map), "map camera probe timestamps")) return false;
    const auto* timestamp_values = static_cast<const uint64_t*>(timestamp_map);
    uint64_t timestamp_frequency = 0u;
    if (!Check(queue->GetTimestampFrequency(&timestamp_frequency), "get camera probe timestamp frequency")
        || timestamp_frequency == 0u) return false;
    best_gpu_microseconds = static_cast<double>(timestamp_values[1] - timestamp_values[0]) * 1.0e6
                            / timestamp_frequency;
    timestamps->Unmap(0u, &no_write);
    staging_slot.staging.clear();
    if (!reject_all) {
      const bool qualified = falcom_world::rtxdi::ObserveCameraProbe(
          &qualification, scenario, 1u, 1u, 7u, width, height, true, true, selected);
      if (scenario < 3u && qualified) return false;
      if (scenario == 3u && !qualified) return false;
      std::printf("PASS NRD camera probe static-world camera-move correction=%u samples=%u median=%.4f p90=%.4f GPU=%.2f us\n",
                  selected, valid_count[selected], residuals[selected][valid_count[selected] / 2u],
                  residuals[selected][(valid_count[selected] * 9u) / 10u], best_gpu_microseconds);
      if (scenario == 3u) std::printf("PASS NRD camera probe four completed frames qualify only on frame 4\n");
    } else {
      const bool qualified = falcom_world::rtxdi::ObserveCameraProbe(
          &qualification, scenario, 1u, 1u, 7u, width, height, true, false, UINT32_MAX);
      if (qualified || qualification.qualified) return false;
      std::printf("PASS NRD camera probe rejects previous %s samples=%u and resets streak\n",
                  reject_previous_class ? "character class" : "depth/disocclusion", rejected[expected_reason] / 2u);
    }
    if (info_queue->GetNumStoredMessagesAllowedByRetrievalFilter() != 0u) {
      std::fprintf(stderr, "FAIL NRD camera probe InfoQueue messages=%llu\n",
                   static_cast<unsigned long long>(info_queue->GetNumStoredMessagesAllowedByRetrievalFilter()));
      return false;
    }
  }
  if (qualification.qualified || qualification.consecutive_passes != 0u) return false;
  std::printf("PASS NRD camera probe CPU staging+readback %.2f/%.2f us across seven completed submissions\n",
              record_cpu_microseconds / 7.0, readback_cpu_microseconds / 7.0);
  return true;
}

bool RunDispatchDescriptionSmoke() {
  using namespace falcom_world::rt12;
  Rt12NrdGeneration generation;
  if (!generation.CreateRelaxDiffuseSpecular()) {
    std::fprintf(stderr, "FAIL NRD runtime version/encoding or instance creation\n");
    return false;
  }

  nrd::CommonSettings common = {};
  for (uint32_t diagonal = 0; diagonal < 4; ++diagonal) {
    common.viewToClipMatrix[diagonal * 5u] = 1.0f;
    common.viewToClipMatrixPrev[diagonal * 5u] = 1.0f;
    common.worldToViewMatrix[diagonal * 5u] = 1.0f;
    common.worldToViewMatrixPrev[diagonal * 5u] = 1.0f;
  }
  common.resourceSize[0] = 13u;
  common.resourceSize[1] = 9u;
  common.resourceSizePrev[0] = 13u;
  common.resourceSizePrev[1] = 9u;
  common.rectSize[0] = 13u;
  common.rectSize[1] = 9u;
  common.rectSizePrev[0] = 13u;
  common.rectSizePrev[1] = 9u;
  common.frameIndex = 0u;
  common.accumulationMode = nrd::AccumulationMode::RESTART;
  if (nrd::SetCommonSettings(*generation.Instance(), common) != nrd::Result::SUCCESS) {
    std::fprintf(stderr, "FAIL NRD initial CommonSettings\n");
    return false;
  }

  nrd::RelaxSettings relax = {};
  relax.hitDistanceReconstructionMode = nrd::HitDistanceReconstructionMode::OFF;
  if (nrd::SetDenoiserSettings(*generation.Instance(), 1u, &relax) != nrd::Result::SUCCESS) {
    std::fprintf(stderr, "FAIL NRD RELAX settings\n");
    return false;
  }

  const nrd::Identifier identifier = 1u;
  const nrd::DispatchDesc* dispatches = nullptr;
  uint32_t dispatches_num = 0u;
  if (nrd::GetComputeDispatches(*generation.Instance(), &identifier, 1u, dispatches, dispatches_num) != nrd::Result::SUCCESS
      || dispatches == nullptr || dispatches_num == 0u) {
    std::fprintf(stderr, "FAIL NRD compute dispatch query\n");
    return false;
  }

  const nrd::InstanceDesc* instance_desc = nrd::GetInstanceDesc(*generation.Instance());
  if (instance_desc == nullptr || instance_desc->pipelinesNum == 0u || instance_desc->permanentPoolSize == 0u
      || instance_desc->transientPoolSize == 0u || instance_desc->descriptorPoolDesc.setsMaxNum == 0u) {
    std::fprintf(stderr, "FAIL NRD instance resource/pipeline description\n");
    return false;
  }

  ComPtr<ID3D12Debug> debug;
  const bool debug_layer_enabled = SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)));
  if (debug_layer_enabled) debug->EnableDebugLayer();
  ComPtr<IDXGIFactory1> factory;
  ComPtr<IDXGIAdapter1> adapter;
  ComPtr<ID3D12Device> device;
  if (!Check(CreateDXGIFactory1(IID_PPV_ARGS(&factory)), "create DXGI factory")
      || !Check(factory->EnumAdapters1(0u, &adapter), "select DXGI adapter")) return false;
  const HRESULT device_hr = D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device));
  if (FAILED(device_hr)) {
    std::fprintf(stderr, "SKIP NRD D3D12 device unavailable: 0x%08lx\n", static_cast<unsigned long>(device_hr));
    return false;
  }
  if (!generation.CreateD3D12Backend(device.Get(), D3D12SerializeRootSignature)) return false;
  auto& native_pipelines = generation.pipelines;
  nrd::RelaxSettings relax_settings = {};
  relax_settings.hitDistanceReconstructionMode = nrd::HitDistanceReconstructionMode::OFF;
  const auto dispatch_stride = instance_desc->descriptorPoolDesc.perSetTexturesMaxNum
                               + instance_desc->descriptorPoolDesc.perSetStorageTexturesMaxNum;
  falcom_world::rt12::Rt12NrdResourcePools pools;
  falcom_world::rt12::Rt12NrdSlot slot;
  if (!NrdCreateSlotResources(device.Get(), *instance_desc, dispatches, dispatches_num, 13u, 9u, &pools, &slot,
                              dispatch_stride, true)) {
    std::fprintf(stderr, "FAIL NRD create slot resources\n");
    return false;
  }
  std::printf("PASS NRD native D3D12 root signature and %u DXIL compute pipelines\n",
              static_cast<uint32_t>(native_pipelines.pipelines.size()));

  std::printf("PASS NRD SDK %u.%u.%u encoding normal=%u roughness=%u pipelines=%u dispatches=%u pools=%u/%u\n",
              NRD_VERSION_MAJOR, NRD_VERSION_MINOR, NRD_VERSION_BUILD,
              static_cast<uint32_t>(nrd::GetLibraryDesc()->normalEncoding),
              static_cast<uint32_t>(nrd::GetLibraryDesc()->roughnessEncoding), instance_desc->pipelinesNum,
              dispatches_num, instance_desc->permanentPoolSize, instance_desc->transientPoolSize);
  for (uint32_t index = 0; index < dispatches_num; ++index) {
    const auto& dispatch = dispatches[index];
    if (dispatch.pipelineIndex >= instance_desc->pipelinesNum || dispatch.gridWidth == 0u || dispatch.gridHeight == 0u
        || dispatch.resources == nullptr || dispatch.resourcesNum == 0u) {
      std::fprintf(stderr, "FAIL NRD dispatch descriptor %u\n", index);
      return false;
    }
  }

  D3D12_COMMAND_QUEUE_DESC queue_desc = {};
  queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
  ComPtr<ID3D12CommandQueue> queue;
  ComPtr<ID3D12Fence> fence;
  if (!Check(device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&queue)), "create NRD queue")
      || !Check(device->CreateFence(0u, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)), "create NRD fence")) return false;
  ComPtr<ID3D12InfoQueue> info_queue;
  if (!debug_layer_enabled || FAILED(device.As(&info_queue))) return false;
  info_queue->ClearStoredMessages();
  uint64_t fence_value = 0u;
  if (!RunN1PrepRelaxSmoke(device.Get(), queue.Get(), fence.Get(), info_queue.Get(), debug_layer_enabled,
                           &generation, *instance_desc, &native_pipelines, &pools, &slot, dispatches, dispatches_num,
                           dispatch_stride, &fence_value)) {
    std::fprintf(stderr, "FAIL N1 prep + RELAX diffuse-only chain\n");
    return false;
  }
  const auto execute_frame = [&](uint32_t width, uint32_t height, uint32_t frame_index,
                                 nrd::AccumulationMode accumulation_mode,
                                 Rt12NrdSlot* frame_slot, Rt12NrdResourcePools* frame_pools) {
    common.resourceSize[0] = width;
    common.resourceSize[1] = height;
    common.resourceSizePrev[0] = width;
    common.resourceSizePrev[1] = height;
    common.rectSize[0] = width;
    common.rectSize[1] = height;
    common.rectSizePrev[0] = width;
    common.rectSizePrev[1] = height;
    common.frameIndex = frame_index;
    common.accumulationMode = accumulation_mode;
    if (nrd::SetCommonSettings(*generation.Instance(), common) != nrd::Result::SUCCESS) {
      std::fprintf(stderr, "FAIL NRD common settings frame=%u size=%ux%u\n", frame_index, width, height);
      return false;
    }
    const nrd::DispatchDesc* frame_dispatches = nullptr;
    uint32_t frame_dispatches_num = 0u;
    if (nrd::GetComputeDispatches(*generation.Instance(), &identifier, 1u, frame_dispatches,
                                  frame_dispatches_num)
            != nrd::Result::SUCCESS
        || frame_dispatches == nullptr || frame_dispatches_num == 0u) return false;
    if (!NrdRecordDispatches(device.Get(), *instance_desc, frame_dispatches, frame_dispatches_num,
                             &native_pipelines, frame_pools, frame_slot, dispatch_stride)) {
      std::fprintf(stderr, "FAIL NRD record dispatches frame=%u\n", frame_index);
      return false;
    }
    ID3D12CommandList* command_lists[] = {frame_slot->command_list.Get()};
    queue->ExecuteCommandLists(1u, command_lists);
    frame_slot->fence_value = ++fence_value;
    if (!NrdWaitForFence(queue.Get(), fence.Get(), frame_slot->fence_value)) {
      std::fprintf(stderr, "NRD device removed reason: 0x%08lx\n",
                   static_cast<unsigned long>(device->GetDeviceRemovedReason()));
      return false;
    }
    if (!NrdReadbackOutput(device.Get(),
                           frame_slot->resources[static_cast<uint32_t>(nrd::ResourceType::OUT_DIFF_RADIANCE_HITDIST)].resource.Get(),
                           frame_slot->diffuse_readback.Get(), width, height, "RELAX diffuse")
        || !NrdReadbackOutput(device.Get(),
                              frame_slot->resources[static_cast<uint32_t>(nrd::ResourceType::OUT_SPEC_RADIANCE_HITDIST)].resource.Get(),
                              frame_slot->specular_readback.Get(), width, height, "RELAX specular")) return false;
    ComPtr<ID3D12InfoQueue> info_queue;
    if (!debug_layer_enabled || FAILED(device.As(&info_queue))) {
      std::fprintf(stderr, "FAIL NRD debug layer/InfoQueue unavailable\n");
      return false;
    }
    const UINT64 message_count = info_queue->GetNumStoredMessagesAllowedByRetrievalFilter();
    if (message_count != 0u) {
      std::fprintf(stderr, "FAIL NRD D3D12 InfoQueue has %llu messages\n",
                   static_cast<unsigned long long>(message_count));
      return false;
    }
    frame_slot->staging.clear();
    std::printf("PASS NRD frame=%u size=%ux%u accumulation=%u pool=%.2f MiB\n", frame_index, width, height,
                static_cast<uint32_t>(accumulation_mode), static_cast<double>(frame_pools->bytes) / (1024.0 * 1024.0));
    return true;
  };

  if (!execute_frame(13u, 9u, 0u, nrd::AccumulationMode::RESTART, &slot, &pools)) return false;
  nrd::RelaxSettings continuation_settings = {};
  continuation_settings.hitDistanceReconstructionMode = nrd::HitDistanceReconstructionMode::OFF;
  if (nrd::SetDenoiserSettings(*generation.Instance(), 1u, &continuation_settings) != nrd::Result::SUCCESS) return false;
  if (!execute_frame(13u, 9u, 1u, nrd::AccumulationMode::CONTINUE, &slot, &pools)) {
    return false;
  }
  if (!execute_frame(13u, 9u, 3u, nrd::AccumulationMode::RESTART, &slot, &pools)) {
    return false;
  }

  Rt12NrdResourcePools resized_pools;
  Rt12NrdSlot resized_slot;
  common.resourceSize[0] = 7u;
  common.resourceSize[1] = 5u;
  common.resourceSizePrev[0] = 7u;
  common.resourceSizePrev[1] = 5u;
  common.rectSize[0] = 7u;
  common.rectSize[1] = 5u;
  common.rectSizePrev[0] = 7u;
  common.rectSizePrev[1] = 5u;
  common.frameIndex = 0u;
  common.accumulationMode = nrd::AccumulationMode::RESTART;
  if (nrd::SetCommonSettings(*generation.Instance(), common) != nrd::Result::SUCCESS) return false;
  if (nrd::GetComputeDispatches(*generation.Instance(), &identifier, 1u, dispatches, dispatches_num) != nrd::Result::SUCCESS
      || dispatches == nullptr || dispatches_num == 0u
      || !NrdCreateSlotResources(device.Get(), *instance_desc, dispatches, dispatches_num, 7u, 5u, &resized_pools,
                                 &resized_slot, dispatch_stride, true)
      || !execute_frame(7u, 5u, 0u, nrd::AccumulationMode::RESTART, &resized_slot, &resized_pools)) return false;
  ComPtr<ID3D12InfoQueue> probe_info_queue;
  if (debug_layer_enabled && SUCCEEDED(device.As(&probe_info_queue))) probe_info_queue->ClearStoredMessages();
  if (!probe_info_queue || !RunCameraProbeSmoke(device.Get(), queue.Get(), probe_info_queue.Get())) {
    std::fprintf(stderr, "FAIL NRD asynchronous camera-probe bridge\n");
    return false;
  }
  std::printf("PASS NRD D3D12 InfoQueue clean across four submissions\n");
  std::printf("PASS NRD size recreation 13x9 -> 7x5 after completed slot fence\n");
  return true;
}

bool RunHistoryIdentityContracts() {
  using falcom_world::rt12::Rt12NrdCanContinueHistory;
  using falcom_world::rt12::Rt12NrdConfigurationIdentity;
  using falcom_world::rt12::Rt12NrdDiffusePublicationIdentity;
  using falcom_world::rt12::Rt12NrdDiffusePublicationMatches;
  using falcom_world::rt12::Rt12NrdHistoryIdentity;
  using falcom_world::rt12::Rt12NrdN1ConfigurationIdentity;
  using falcom_world::rt12::Rt12NrdSubmissionAllowed;
  if (!Rt12NrdSubmissionAllowed(false, false, true) || Rt12NrdSubmissionAllowed(true, false, true)
      || Rt12NrdSubmissionAllowed(false, true, true) || Rt12NrdSubmissionAllowed(false, false, false)) return false;
  std::printf("PASS NRD submissions suppressed while RR owns denoising\n");
  constexpr Rt12NrdHistoryIdentity previous = {4u, 8u, 2u, 19u, 13u, 9u, true};
  constexpr Rt12NrdHistoryIdentity next = {4u, 8u, 2u, 20u, 13u, 9u, true};
  if (!Rt12NrdCanContinueHistory(previous, next)) return false;
  const std::array<Rt12NrdHistoryIdentity, 7> reset_cases = {{
      {4u, 8u, 2u, 21u, 13u, 9u, true},
      {5u, 8u, 2u, 20u, 13u, 9u, true},
      {4u, 9u, 2u, 20u, 13u, 9u, true},
      {4u, 8u, 3u, 20u, 13u, 9u, true},
      {4u, 8u, 2u, 20u, 17u, 9u, true},
      {4u, 8u, 2u, 20u, 13u, 9u, false},
      {4u, 8u, 2u, 0u, 13u, 9u, true},
  }};
  for (const auto& current : reset_cases) {
    if (Rt12NrdCanContinueHistory(previous, current)) return false;
  }
  std::printf("PASS NRD history identity/gap/resize/owner/generation/config/device-reset contracts\n");
  const std::array<float, 7u> n1_settings = {1.f, 0.5f, 1.f, 0.75f, 8.f, 0.02f, 0.9f};
  const uint64_t n1_identity = Rt12NrdN1ConfigurationIdentity(1u, n1_settings);
  const uint64_t nrd_identity = Rt12NrdConfigurationIdentity(4u, 7u, 11u, 0u);
  const Rt12NrdDiffusePublicationIdentity publication = {
      .owner_generation = 8u,
      .transfer_configuration = 4u,
      .di_configuration = 7u,
      .n1_configuration = n1_identity,
      .nrd_configuration = nrd_identity,
      .producer_fence_b = 21u,
      .frame = 19u,
      .width = 13u,
      .height = 9u,
      .scene_slot = 2,
      .transfer_slot = 1,
      .accepted = true};
  const auto matches_publication = [&](const Rt12NrdDiffusePublicationIdentity& identity,
                                       uint32_t frame, uint64_t owner, uint64_t transfer_config,
                                       uint64_t di_config, uint64_t n1_config, uint64_t nrd_config,
                                       uint32_t width, uint32_t height, int32_t scene_slot,
                                       int32_t transfer_slot, uint64_t fence) {
    return Rt12NrdDiffusePublicationMatches(identity, frame, owner, transfer_config, di_config,
                                            n1_config, nrd_config, width, height, scene_slot,
                                            transfer_slot, fence);
  };
  if (!matches_publication(publication, 19u, 8u, 4u, 7u, n1_identity, nrd_identity,
                           13u, 9u, 2, 1, 21u)) return false;
  auto stale_publication = publication;
  stale_publication.accepted = false;
  if (matches_publication(stale_publication, 19u, 8u, 4u, 7u, n1_identity, nrd_identity,
                          13u, 9u, 2, 1, 21u)) return false;
  stale_publication = publication;
  stale_publication.producer_fence_b = 0u;
  if (matches_publication(stale_publication, 19u, 8u, 4u, 7u, n1_identity, nrd_identity,
                          13u, 9u, 2, 1, 21u)) return false;
  if (matches_publication(publication, 20u, 8u, 4u, 7u, n1_identity, nrd_identity, 13u, 9u, 2, 1, 21u)
      || matches_publication(publication, 19u, 9u, 4u, 7u, n1_identity, nrd_identity, 13u, 9u, 2, 1, 21u)
      || matches_publication(publication, 19u, 8u, 5u, 7u, n1_identity, nrd_identity, 13u, 9u, 2, 1, 21u)
      || matches_publication(publication, 19u, 8u, 4u, 8u, n1_identity, nrd_identity, 13u, 9u, 2, 1, 21u)
      || matches_publication(publication, 19u, 8u, 4u, 7u, n1_identity + 1u, nrd_identity, 13u, 9u, 2, 1, 21u)
      || matches_publication(publication, 19u, 8u, 4u, 7u, n1_identity, nrd_identity + 1u, 13u, 9u, 2, 1, 21u)
      || matches_publication(publication, 19u, 8u, 4u, 7u, n1_identity, nrd_identity, 14u, 9u, 2, 1, 21u)
      || matches_publication(publication, 19u, 8u, 4u, 7u, n1_identity, nrd_identity, 13u, 10u, 2, 1, 21u)
      || matches_publication(publication, 19u, 8u, 4u, 7u, n1_identity, nrd_identity, 13u, 9u, 1, 1, 21u)
      || matches_publication(publication, 19u, 8u, 4u, 7u, n1_identity, nrd_identity, 13u, 9u, 2, 2, 21u)
      || matches_publication(publication, 19u, 8u, 4u, 7u, n1_identity, nrd_identity, 13u, 9u, 2, 1, 22u)) return false;
  auto changed_n1_settings = n1_settings;
  changed_n1_settings[3] += 0.01f;
  if (Rt12NrdN1ConfigurationIdentity(1u, changed_n1_settings) == n1_identity) return false;
  std::printf("PASS N3c publication frame/owner/config/N1-BRDF/NRD/size/scene/slot/B-fence acceptance identity\n");
  return true;
}
}  // namespace

int main() {
  return RunHistoryIdentityContracts() && RunDispatchDescriptionSmoke() ? 0 : 1;
}
