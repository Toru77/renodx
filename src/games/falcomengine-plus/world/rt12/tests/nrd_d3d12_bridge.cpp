#include "../rt12_nrd.hpp"

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

struct NrdNativePipelines {
  ComPtr<ID3D12Device> device;
  ComPtr<ID3D12RootSignature> root_signature;
  std::vector<ComPtr<ID3D12PipelineState>> pipelines;

  bool Create(ID3D12Device* input_device, const nrd::InstanceDesc& instance_desc) {
    device = input_device;
    std::array<D3D12_DESCRIPTOR_RANGE, 2> ranges = {};
    ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    ranges[0].NumDescriptors = instance_desc.descriptorPoolDesc.perSetTexturesMaxNum;
    ranges[0].BaseShaderRegister = instance_desc.resourcesBaseRegisterIndex;
    ranges[0].RegisterSpace = instance_desc.resourcesSpaceIndex;
    ranges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
    ranges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    ranges[1].NumDescriptors = instance_desc.descriptorPoolDesc.perSetStorageTexturesMaxNum;
    ranges[1].BaseShaderRegister = instance_desc.resourcesBaseRegisterIndex;
    ranges[1].RegisterSpace = instance_desc.resourcesSpaceIndex;
    ranges[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    std::array<D3D12_ROOT_PARAMETER, 3> parameters = {};
    parameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    parameters[0].Descriptor.ShaderRegister = instance_desc.constantBufferRegisterIndex;
    parameters[0].Descriptor.RegisterSpace = instance_desc.constantBufferAndSamplersSpaceIndex;
    parameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    for (uint32_t index = 0; index < 2; ++index) {
      parameters[index + 1u].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
      parameters[index + 1u].DescriptorTable.NumDescriptorRanges = 1u;
      parameters[index + 1u].DescriptorTable.pDescriptorRanges = &ranges[index];
      parameters[index + 1u].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    }

    std::array<D3D12_STATIC_SAMPLER_DESC, static_cast<uint32_t>(nrd::Sampler::MAX_NUM)> samplers = {};
    for (uint32_t index = 0; index < samplers.size(); ++index) {
      auto& sampler = samplers[index];
      sampler.Filter = instance_desc.samplers[index] == nrd::Sampler::NEAREST_CLAMP
                           ? D3D12_FILTER_MIN_MAG_MIP_POINT
                           : D3D12_FILTER_MIN_MAG_MIP_LINEAR;
      sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
      sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
      sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
      sampler.MipLODBias = 0.0f;
      sampler.MaxAnisotropy = 1u;
      sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
      sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
      sampler.MinLOD = 0.0f;
      sampler.MaxLOD = D3D12_FLOAT32_MAX;
      sampler.ShaderRegister = instance_desc.samplersBaseRegisterIndex + index;
      sampler.RegisterSpace = instance_desc.constantBufferAndSamplersSpaceIndex;
      sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    }

    D3D12_ROOT_SIGNATURE_DESC root_desc = {};
    root_desc.NumParameters = static_cast<UINT>(parameters.size());
    root_desc.pParameters = parameters.data();
    root_desc.NumStaticSamplers = static_cast<UINT>(samplers.size());
    root_desc.pStaticSamplers = samplers.data();
    root_desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> errors;
    if (!Check(D3D12SerializeRootSignature(&root_desc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &errors),
               "serialize root signature")) {
      if (errors) std::fprintf(stderr, "%s\n", static_cast<const char*>(errors->GetBufferPointer()));
      return false;
    }
    if (!Check(device->CreateRootSignature(0u, signature->GetBufferPointer(), signature->GetBufferSize(),
                                           IID_PPV_ARGS(&root_signature)),
               "create root signature")) return false;

    pipelines.resize(instance_desc.pipelinesNum);
    for (uint32_t index = 0; index < pipelines.size(); ++index) {
      const auto& source = instance_desc.pipelines[index].computeShaderDXIL;
      if (source.bytecode == nullptr || source.size == 0u) {
        std::fprintf(stderr, "FAIL NRD missing DXIL pipeline bytecode at %u\n", index);
        return false;
      }
      D3D12_COMPUTE_PIPELINE_STATE_DESC pipeline_desc = {};
      pipeline_desc.pRootSignature = root_signature.Get();
      pipeline_desc.CS = {source.bytecode, static_cast<SIZE_T>(source.size)};
      if (!Check(device->CreateComputePipelineState(&pipeline_desc, IID_PPV_ARGS(&pipelines[index])),
                 instance_desc.pipelines[index].shaderIdentifier)) return false;
    }
    return true;
  }
};

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
    case nrd::ResourceType::IN_VIEWZ:                  return DXGI_FORMAT_R16_FLOAT;
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
    case DXGI_FORMAT_R11G11B10_FLOAT:    return 4u;
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
  return Check(device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc,
                                               D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                               IID_PPV_ARGS(output->GetAddressOf())),
               "create NRD upload buffer");
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
                         NrdNativePipelines* native_pipelines, falcom_world::rt12::Rt12NrdResourcePools* pools,
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
  NrdNativePipelines native_pipelines;
  if (!native_pipelines.Create(device.Get(), *instance_desc)) return false;
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
  uint64_t fence_value = 0u;
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
  std::printf("PASS NRD D3D12 InfoQueue clean across four submissions\n");
  std::printf("PASS NRD size recreation 13x9 -> 7x5 after completed slot fence\n");
  return true;
}

bool RunHistoryIdentityContracts() {
  using falcom_world::rt12::Rt12NrdCanContinueHistory;
  using falcom_world::rt12::Rt12NrdHistoryIdentity;
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
  return true;
}
}  // namespace

int main() {
  return RunHistoryIdentityContracts() && RunDispatchDescriptionSmoke() ? 0 : 1;
}
