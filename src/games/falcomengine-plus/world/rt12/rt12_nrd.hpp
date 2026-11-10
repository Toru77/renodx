#pragma once

#include <NRD.h>

#include <d3d12.h>
#include <wrl/client.h>

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

#include "../rtxdi/rtxdi_camera_qualification.hpp"

namespace falcom_world::rt12 {

using Microsoft::WRL::ComPtr;

struct Rt12NrdTexture {
  ComPtr<ID3D12Resource> resource;
  DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
  D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON;
  uint32_t width = 0;
  uint32_t height = 0;
  bool unordered_access_written = false;
};

struct Rt12NrdCameraCapture {
  std::array<uint8_t, 1536u> scene_rows = {};
  uint64_t resource = 0u, offset = 0u, binding_sequence = 0u;
  uint64_t write_sequence = 0u, write_frame = UINT64_MAX, owner_generation = 0u;
  uint32_t capture_frame = UINT32_MAX;
  bool valid = false;
};

struct Rt12NrdProbeConstants {
  uint32_t resource_size[2] = {};
  float current_jitter_pixels[2] = {};
  float previous_jitter_pixels[2] = {};
  float motion_correction_pixels[2][2] = {};
  float padding[2] = {};
  float current_inverse_view_projection[16] = {};
  float previous_inverse_view_projection[16] = {};
  float current_view_projection_non_jittered[16] = {};
  float previous_view_projection_non_jittered[16] = {};
  float previous_view[16] = {};
};
static_assert(offsetof(Rt12NrdProbeConstants, resource_size) == 0u);
static_assert(offsetof(Rt12NrdProbeConstants, current_jitter_pixels) == 8u);
static_assert(offsetof(Rt12NrdProbeConstants, previous_jitter_pixels) == 16u);
static_assert(offsetof(Rt12NrdProbeConstants, motion_correction_pixels) == 24u);
static_assert(offsetof(Rt12NrdProbeConstants, padding) == 40u);
static_assert(offsetof(Rt12NrdProbeConstants, current_inverse_view_projection) == 48u);
static_assert(offsetof(Rt12NrdProbeConstants, previous_inverse_view_projection) == 112u);
static_assert(offsetof(Rt12NrdProbeConstants, current_view_projection_non_jittered) == 176u);
static_assert(offsetof(Rt12NrdProbeConstants, previous_view_projection_non_jittered) == 240u);
static_assert(offsetof(Rt12NrdProbeConstants, previous_view) == 304u);
static_assert(sizeof(Rt12NrdProbeConstants) == 368u);

struct Rt12NrdSlot {
  ComPtr<ID3D12CommandAllocator> allocator;
  ComPtr<ID3D12GraphicsCommandList> command_list;
  ComPtr<ID3D12DescriptorHeap> descriptors;
  ComPtr<ID3D12Resource> constants;
  ComPtr<ID3D12Resource> diffuse_readback;
  ComPtr<ID3D12Resource> specular_readback;
  ComPtr<ID3D12Resource> probe_constants;
  ComPtr<ID3D12Resource> probe_results;
  ComPtr<ID3D12Resource> probe_readback;
  ComPtr<ID3D12Resource> probe_timestamp_readback;
  ComPtr<ID3D12QueryHeap> probe_queries;
  ComPtr<ID3D12DescriptorHeap> dispatch_descriptors;
  ComPtr<ID3D12Resource> dispatch_constants;
  ComPtr<ID3D12DescriptorHeap> prep_descriptors;
  ComPtr<ID3D12DescriptorHeap> publication_descriptors;
  ComPtr<ID3D12Resource> publication_constants;
  ComPtr<ID3D12DescriptorHeap> debug_distance_descriptors;
  ComPtr<ID3D12DescriptorHeap> hitdistance_descriptors;
  ComPtr<ID3D12Resource> prep_constants;
  ComPtr<ID3D12Resource> prep_diffuse;
  ComPtr<ID3D12Resource> prep_specular;
  ComPtr<ID3D12Resource> prep_normal_roughness;
  ComPtr<ID3D12Resource> prep_viewz;
  ComPtr<ID3D12Resource> prep_mv;
  ComPtr<ID3D12Resource> hit_distance;
  ComPtr<ID3D12Resource> hit_validity;
  uint32_t hitdistance_width = 0u;
  uint32_t hitdistance_height = 0u;
  std::vector<ComPtr<ID3D12Resource>> staging;
  std::array<Rt12NrdTexture, static_cast<uint32_t>(nrd::ResourceType::MAX_NUM)> resources;
  Rt12NrdCameraCapture camera;
  rtxdi::CameraCandidate camera_candidate;
  rtxdi::CameraQualification qualification;
  uint64_t probe_fence = 0u;
  double probe_gpu_microseconds = -1.0;
  double probe_record_cpu_microseconds = -1.0;
  double probe_readback_cpu_microseconds = -1.0;
  double nrd_gpu_microseconds = -1.0;
  uint32_t probe_frame = UINT32_MAX;
  uint32_t dispatch_count = 0u;
  uint32_t width = 0u, height = 0u;
  bool probe_pending = false;
  bool dispatch_ready = false;
  bool diffuse_output_valid = false;
  bool diffuse_publication_recorded = false;
  uint64_t fence_value = 0;
};

struct Rt12NrdDiffusePublicationIdentity {
  uint64_t owner_generation = 0u;
  uint64_t transfer_configuration = 0u;
  uint64_t di_configuration = 0u;
  uint64_t n1_configuration = 0u;
  uint64_t nrd_configuration = 0u;
  uint64_t producer_fence_b = 0u;
  uint32_t frame = UINT32_MAX;
  uint32_t width = 0u;
  uint32_t height = 0u;
  int32_t scene_slot = -1;
  int32_t transfer_slot = -1;
  bool accepted = false;
};

inline uint64_t Rt12NrdN1ConfigurationIdentity(uint32_t version, const std::array<float, 7>& brdf_config) {
  uint64_t identity = 1469598103934665603ull ^ version;
  for (float value : brdf_config) {
    identity ^= std::bit_cast<uint32_t>(value);
    identity *= 1099511628211ull;
  }
  return identity == 0u ? 1u : identity;
}

inline uint64_t Rt12NrdConfigurationIdentity(uint64_t transfer_configuration, uint64_t di_configuration,
                                             uint64_t settings_identity, uint32_t motion_correction_index) {
  uint64_t identity = (transfer_configuration ^ (di_configuration * 0x9e3779b97f4a7c15ull)
                       ^ (settings_identity * 0xd6e8feb86659fd93ull)
                       ^ (uint64_t{motion_correction_index} << 32u) ^ 1u)
                      ^ 1469598103934665603ull;
  identity *= 1099511628211ull;
  return identity == 0u ? 1u : identity;
}

inline bool Rt12NrdDiffusePublicationMatches(const Rt12NrdDiffusePublicationIdentity& identity,
                                             uint32_t frame, uint64_t owner_generation,
                                             uint64_t transfer_configuration, uint64_t di_configuration,
                                             uint64_t n1_configuration, uint64_t nrd_configuration,
                                             uint32_t width, uint32_t height, int32_t scene_slot,
                                             int32_t transfer_slot, uint64_t producer_fence_b) {
  return identity.accepted && identity.frame == frame && identity.owner_generation == owner_generation
         && identity.transfer_configuration == transfer_configuration && identity.di_configuration == di_configuration
         && identity.n1_configuration == n1_configuration && identity.nrd_configuration == nrd_configuration
         && identity.width == width && identity.height == height && identity.scene_slot == scene_slot
         && identity.transfer_slot == transfer_slot && identity.producer_fence_b != 0u
         && identity.producer_fence_b == producer_fence_b;
}

inline bool Rt12NrdCreateDiffusePublicationPipeline(ID3D12Device* device, const void* shader_bytecode,
                                                    size_t shader_size,
                                                    Microsoft::WRL::ComPtr<ID3D12RootSignature>* root,
                                                    Microsoft::WRL::ComPtr<ID3D12PipelineState>* pipeline) {
  if (device == nullptr || shader_bytecode == nullptr || shader_size == 0u || root == nullptr || pipeline == nullptr)
    return false;
  D3D12_DESCRIPTOR_RANGE ranges[2] = {};
  ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
  ranges[0].NumDescriptors = 4u;
  ranges[0].BaseShaderRegister = 0u;
  ranges[0].OffsetInDescriptorsFromTableStart = 0u;
  ranges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
  ranges[1].NumDescriptors = 1u;
  ranges[1].BaseShaderRegister = 0u;
  ranges[1].OffsetInDescriptorsFromTableStart = 0u;
  D3D12_ROOT_PARAMETER parameters[3] = {};
  parameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
  parameters[0].Constants.ShaderRegister = 0u;
  parameters[0].Constants.Num32BitValues = 3u;
  parameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
  parameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
  parameters[1].DescriptorTable.NumDescriptorRanges = 1u;
  parameters[1].DescriptorTable.pDescriptorRanges = &ranges[0];
  parameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
  parameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
  parameters[2].DescriptorTable.NumDescriptorRanges = 1u;
  parameters[2].DescriptorTable.pDescriptorRanges = &ranges[1];
  parameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
  D3D12_ROOT_SIGNATURE_DESC signature = {};
  signature.NumParameters = static_cast<UINT>(std::size(parameters));
  signature.pParameters = parameters;
  ComPtr<ID3DBlob> blob, errors;
  if (FAILED(D3D12SerializeRootSignature(&signature, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &errors))) return false;
  ComPtr<ID3D12RootSignature> candidate_root;
  ComPtr<ID3D12PipelineState> candidate_pipeline;
  if (FAILED(device->CreateRootSignature(0u, blob->GetBufferPointer(), blob->GetBufferSize(),
                                         IID_PPV_ARGS(&candidate_root)))) return false;
  D3D12_COMPUTE_PIPELINE_STATE_DESC pipeline_desc = {};
  pipeline_desc.pRootSignature = candidate_root.Get();
  pipeline_desc.CS = {shader_bytecode, shader_size};
  if (FAILED(device->CreateComputePipelineState(&pipeline_desc, IID_PPV_ARGS(&candidate_pipeline)))) return false;
  *root = std::move(candidate_root);
  *pipeline = std::move(candidate_pipeline);
  return true;
}

struct Rt12NrdResourcePools {
  std::vector<Rt12NrdTexture> permanent;
  std::vector<Rt12NrdTexture> transient;
  uint64_t bytes = 0;
  uint32_t width = 0;
  uint32_t height = 0;
};

struct Rt12NrdHistoryIdentity {
  uint64_t owner = 0;
  uint64_t generation = 0;
  uint64_t configuration = 0;
  uint64_t frame = 0;
  uint32_t width = 0;
  uint32_t height = 0;
  bool valid = false;
};

inline bool Rt12NrdCanContinueHistory(const Rt12NrdHistoryIdentity& previous,
                                      const Rt12NrdHistoryIdentity& current) {
  return previous.valid && current.valid && previous.owner == current.owner
         && previous.generation == current.generation && previous.configuration == current.configuration
         && previous.width == current.width && previous.height == current.height
         && previous.frame != UINT64_MAX && current.frame == previous.frame + 1u;
}

inline bool Rt12NrdSubmissionAllowed(bool rr_requested, bool rr_owns_denoise, bool relax_selected) {
  return relax_selected && !rr_requested && !rr_owns_denoise;
}

struct Rt12NrdPipelineSet {
  ComPtr<ID3D12RootSignature> root_signature;
  std::vector<ComPtr<ID3D12PipelineState>> pipelines;

  bool Create(ID3D12Device* device, const nrd::InstanceDesc& instance_desc,
              PFN_D3D12_SERIALIZE_ROOT_SIGNATURE serialize) {
    if (device == nullptr || serialize == nullptr || instance_desc.pipelinesNum == 0u
        || instance_desc.samplersNum != static_cast<uint32_t>(nrd::Sampler::MAX_NUM)) return false;
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
    for (uint32_t index = 0; index < 2u; ++index) {
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
      sampler.MaxAnisotropy = 1u;
      sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
      sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
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
    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> errors;
    HRESULT hr = serialize(&root_desc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &errors);
    if (FAILED(hr) || signature == nullptr) return false;
    ComPtr<ID3D12RootSignature> candidate_root;
    hr = device->CreateRootSignature(0u, signature->GetBufferPointer(), signature->GetBufferSize(),
                                     IID_PPV_ARGS(&candidate_root));
    if (FAILED(hr)) return false;
    std::vector<ComPtr<ID3D12PipelineState>> candidate_pipelines(instance_desc.pipelinesNum);
    for (uint32_t index = 0; index < candidate_pipelines.size(); ++index) {
      const auto& source = instance_desc.pipelines[index].computeShaderDXIL;
      if (source.bytecode == nullptr || source.size == 0u) return false;
      D3D12_COMPUTE_PIPELINE_STATE_DESC pipeline_desc = {};
      pipeline_desc.pRootSignature = candidate_root.Get();
      pipeline_desc.CS = {source.bytecode, static_cast<SIZE_T>(source.size)};
      hr = device->CreateComputePipelineState(&pipeline_desc, IID_PPV_ARGS(&candidate_pipelines[index]));
      if (FAILED(hr)) return false;
    }
    root_signature = std::move(candidate_root);
    pipelines = std::move(candidate_pipelines);
    return true;
  }

  void Reset() {
    pipelines.clear();
    root_signature.Reset();
  }
};

inline Rt12NrdTexture* Rt12NrdFindTexture(const nrd::ResourceDesc& resource,
                                          Rt12NrdResourcePools* pools, Rt12NrdSlot* slot) {
  if (pools == nullptr || slot == nullptr) return nullptr;
  if (resource.type == nrd::ResourceType::PERMANENT_POOL) {
    return resource.indexInPool < pools->permanent.size() ? &pools->permanent[resource.indexInPool] : nullptr;
  }
  if (resource.type == nrd::ResourceType::TRANSIENT_POOL) {
    return resource.indexInPool < pools->transient.size() ? &pools->transient[resource.indexInPool] : nullptr;
  }
  const uint32_t index = static_cast<uint32_t>(resource.type);
  if (index >= static_cast<uint32_t>(nrd::ResourceType::TRANSIENT_POOL)) return nullptr;
  Rt12NrdTexture& texture = slot->resources[index];
  return texture.resource ? &texture : nullptr;
}

inline bool Rt12NrdRecordDiffusePublication(ID3D12Device* device, ID3D12GraphicsCommandList* list,
                                            ID3D12RootSignature* root, ID3D12PipelineState* pipeline,
                                            ID3D12DescriptorHeap* descriptors, ID3D12Resource* constants,
                                            Rt12NrdTexture* relax_diffuse, Rt12NrdTexture* prepared_viewz,
                                            Rt12NrdTexture* n1_status, Rt12NrdTexture* n3b_hit_validity,
                                            Rt12NrdTexture* publication, uint32_t width, uint32_t height) {
  if (device == nullptr || list == nullptr || root == nullptr || pipeline == nullptr || descriptors == nullptr
      || constants == nullptr || relax_diffuse == nullptr || prepared_viewz == nullptr || n1_status == nullptr
      || n3b_hit_validity == nullptr || publication == nullptr || width == 0u || height == 0u
      || uint64_t{width} * height > UINT32_MAX || !relax_diffuse->resource || !prepared_viewz->resource
      || !n1_status->resource || !n3b_hit_validity->resource || !publication->resource
      || relax_diffuse->width != width || relax_diffuse->height != height || prepared_viewz->width != width
      || prepared_viewz->height != height || n1_status->width != width || n1_status->height != height
      || publication->width != width || publication->height != height
      || relax_diffuse->format != DXGI_FORMAT_R16G16B16A16_FLOAT
      || prepared_viewz->format != DXGI_FORMAT_R32_FLOAT || n1_status->format != DXGI_FORMAT_R32_UINT
      || publication->format != DXGI_FORMAT_R16G16B16A16_FLOAT
      || n3b_hit_validity->resource->GetDesc().Dimension != D3D12_RESOURCE_DIMENSION_BUFFER
      || n3b_hit_validity->resource->GetDesc().Width < uint64_t{width} * height * sizeof(uint32_t)) return false;

  const uint32_t descriptor_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
  const D3D12_CPU_DESCRIPTOR_HANDLE heap_cpu = descriptors->GetCPUDescriptorHandleForHeapStart();
  const D3D12_GPU_DESCRIPTOR_HANDLE heap_gpu = descriptors->GetGPUDescriptorHandleForHeapStart();
  if (constants->GetDesc().Width < 12u) return false;
  void* mapped = nullptr;
  const D3D12_RANGE no_read = {0u, 0u};
  if (FAILED(constants->Map(0u, &no_read, &mapped))) return false;
  const std::array<uint32_t, 3u> publication_constants = {width, height, std::bit_cast<uint32_t>(500000.0f)};
  std::memcpy(mapped, publication_constants.data(), sizeof(publication_constants));
  constants->Unmap(0u, nullptr);

  const std::array<Rt12NrdTexture*, 4u> inputs = {relax_diffuse, prepared_viewz, n1_status, n3b_hit_validity};
  std::array<D3D12_RESOURCE_STATES, 4u> input_restore_states = {};
  const std::array<DXGI_FORMAT, 4u> input_formats = {DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_R32_FLOAT,
                                                     DXGI_FORMAT_R32_UINT, DXGI_FORMAT_UNKNOWN};
  std::array<D3D12_RESOURCE_BARRIER, 16u> barriers = {};
  uint32_t barrier_count = 0u;
  for (uint32_t index = 0u; index < inputs.size(); ++index) {
    Rt12NrdTexture& input = *inputs[index];
    input_restore_states[index] = input.state;
    if (input.state == D3D12_RESOURCE_STATE_UNORDERED_ACCESS && input.unordered_access_written) {
      D3D12_RESOURCE_BARRIER& barrier = barriers[barrier_count++];
      barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
      barrier.UAV.pResource = input.resource.Get();
      input.unordered_access_written = false;
    }
    if (input.state != D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
        && input.state != D3D12_RESOURCE_STATE_GENERIC_READ) {
      D3D12_RESOURCE_BARRIER& barrier = barriers[barrier_count++];
      barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
      barrier.Transition.pResource = input.resource.Get();
      barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
      barrier.Transition.StateBefore = input.state;
      barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
      input.state = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    }
    D3D12_SHADER_RESOURCE_VIEW_DESC view = {};
    view.Format = input_formats[index];
    view.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    if (index == 3u) {
      view.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
      view.Buffer.NumElements = width * height;
      view.Buffer.StructureByteStride = sizeof(uint32_t);
    } else {
      view.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
      view.Texture2D.MipLevels = 1u;
    }
    device->CreateShaderResourceView(input.resource.Get(), &view,
                                     {heap_cpu.ptr + static_cast<SIZE_T>(index) * descriptor_size});
  }
  if (publication->state != D3D12_RESOURCE_STATE_UNORDERED_ACCESS) {
    D3D12_RESOURCE_BARRIER& barrier = barriers[barrier_count++];
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = publication->resource.Get();
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = publication->state;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    publication->state = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
  }
  D3D12_UNORDERED_ACCESS_VIEW_DESC output_view = {};
  output_view.Format = publication->format;
  output_view.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
  device->CreateUnorderedAccessView(publication->resource.Get(), nullptr, &output_view,
                                    {heap_cpu.ptr + static_cast<SIZE_T>(4u) * descriptor_size});
  if (barrier_count != 0u) list->ResourceBarrier(barrier_count, barriers.data());

  ID3D12DescriptorHeap* heaps[] = {descriptors};
  list->SetDescriptorHeaps(1u, heaps);
  list->SetComputeRootSignature(root);
  list->SetPipelineState(pipeline);
  list->SetComputeRoot32BitConstants(0u, static_cast<UINT>(publication_constants.size()),
                                     publication_constants.data(), 0u);
  list->SetComputeRootDescriptorTable(1u, heap_gpu);
  list->SetComputeRootDescriptorTable(2u, {heap_gpu.ptr + static_cast<UINT64>(4u) * descriptor_size});
  list->Dispatch((width + 7u) / 8u, (height + 7u) / 8u, 1u);
  publication->unordered_access_written = true;

  barriers = {};
  barrier_count = 0u;
  D3D12_RESOURCE_BARRIER& publication_uav = barriers[barrier_count++];
  publication_uav.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
  publication_uav.UAV.pResource = publication->resource.Get();
  publication->unordered_access_written = false;
  D3D12_RESOURCE_BARRIER& publication_common = barriers[barrier_count++];
  publication_common.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  publication_common.Transition.pResource = publication->resource.Get();
  publication_common.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  publication_common.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
  publication_common.Transition.StateAfter = D3D12_RESOURCE_STATE_COMMON;
  publication->state = D3D12_RESOURCE_STATE_COMMON;
  for (uint32_t index = 0u; index < inputs.size(); ++index) {
    Rt12NrdTexture& input = *inputs[index];
    const D3D12_RESOURCE_STATES restore_state = input_restore_states[index];
    if (input.state == restore_state) continue;
    D3D12_RESOURCE_BARRIER& barrier = barriers[barrier_count++];
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = input.resource.Get();
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = input.state;
    barrier.Transition.StateAfter = restore_state;
    input.state = restore_state;
  }
  list->ResourceBarrier(barrier_count, barriers.data());
  return true;
}

inline bool Rt12NrdRecordDispatches(ID3D12Device* device, ID3D12GraphicsCommandList* list,
                                    const nrd::InstanceDesc& instance_desc,
                                    const nrd::DispatchDesc* dispatches, uint32_t dispatches_num,
                                    Rt12NrdPipelineSet* pipelines, Rt12NrdResourcePools* pools,
                                    Rt12NrdSlot* slot, uint32_t descriptor_stride) {
  if (device == nullptr || list == nullptr || dispatches == nullptr || dispatches_num == 0u
      || pipelines == nullptr || pools == nullptr || slot == nullptr || slot->dispatch_descriptors == nullptr
      || slot->dispatch_constants == nullptr) return false;
  const uint32_t srv_count = instance_desc.descriptorPoolDesc.perSetTexturesMaxNum;
  const uint32_t uav_count = instance_desc.descriptorPoolDesc.perSetStorageTexturesMaxNum;
  const uint32_t descriptor_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
  const uint64_t constant_stride = (instance_desc.constantBufferMaxDataSize + 255u) & ~uint64_t{255u};
  if (dispatches_num > slot->dispatch_count || constant_stride == 0u
      || constant_stride * dispatches_num > slot->dispatch_constants->GetDesc().Width) return false;
  void* mapped = nullptr;
  const D3D12_RANGE no_read = {0u, 0u};
  if (FAILED(slot->dispatch_constants->Map(0u, &no_read, &mapped))) return false;
  auto* constant_bytes = static_cast<uint8_t*>(mapped);
  std::memset(constant_bytes, 0, static_cast<size_t>(constant_stride * dispatches_num));
  uint32_t previous_constant = UINT32_MAX;
  const D3D12_CPU_DESCRIPTOR_HANDLE heap_cpu = slot->dispatch_descriptors->GetCPUDescriptorHandleForHeapStart();
  const D3D12_GPU_DESCRIPTOR_HANDLE heap_gpu = slot->dispatch_descriptors->GetGPUDescriptorHandleForHeapStart();
  for (uint32_t dispatch_index = 0u; dispatch_index < dispatches_num; ++dispatch_index) {
    const nrd::DispatchDesc& dispatch = dispatches[dispatch_index];
    if (dispatch.pipelineIndex >= pipelines->pipelines.size() || dispatch.resources == nullptr
        || dispatch.gridWidth == 0u || dispatch.gridHeight == 0u || dispatch.resourcesNum > srv_count + uav_count) {
      slot->dispatch_constants->Unmap(0u, nullptr);
      return false;
    }
    uint8_t* constants = constant_bytes + static_cast<size_t>(dispatch_index) * constant_stride;
    if (!dispatch.constantBufferDataMatchesPreviousDispatch) {
      if (dispatch.constantBufferDataSize > constant_stride
          || (dispatch.constantBufferDataSize != 0u && dispatch.constantBufferData == nullptr)) {
        slot->dispatch_constants->Unmap(0u, nullptr);
        return false;
      }
      if (dispatch.constantBufferDataSize != 0u) {
        std::memcpy(constants, dispatch.constantBufferData, dispatch.constantBufferDataSize);
      }
      previous_constant = dispatch_index;
    } else if (previous_constant == UINT32_MAX) {
      slot->dispatch_constants->Unmap(0u, nullptr);
      return false;
    } else {
      std::memcpy(constants, constant_bytes + static_cast<size_t>(previous_constant) * constant_stride,
                  static_cast<size_t>(constant_stride));
      previous_constant = dispatch_index;
    }
    const D3D12_CPU_DESCRIPTOR_HANDLE dispatch_cpu = {
        heap_cpu.ptr + static_cast<SIZE_T>(dispatch_index) * descriptor_stride * descriptor_size};
    const D3D12_GPU_DESCRIPTOR_HANDLE dispatch_gpu = {
        heap_gpu.ptr + static_cast<UINT64>(dispatch_index) * descriptor_stride * descriptor_size};
    for (uint32_t index = 0u; index < srv_count; ++index) {
      D3D12_SHADER_RESOURCE_VIEW_DESC null_view = {};
      null_view.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
      null_view.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
      null_view.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
      null_view.Texture2D.MipLevels = 1u;
      device->CreateShaderResourceView(nullptr, &null_view,
                                       {dispatch_cpu.ptr + static_cast<SIZE_T>(index) * descriptor_size});
    }
    for (uint32_t index = 0u; index < uav_count; ++index) {
      D3D12_UNORDERED_ACCESS_VIEW_DESC null_view = {};
      null_view.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
      null_view.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
      device->CreateUnorderedAccessView(nullptr, nullptr, &null_view,
                                        {dispatch_cpu.ptr + static_cast<SIZE_T>(srv_count + index) * descriptor_size});
    }
    std::array<D3D12_RESOURCE_BARRIER, 64u> barriers = {};
    uint32_t barrier_count = 0u, srv_index = 0u, uav_index = 0u;
    for (uint32_t resource_index = 0u; resource_index < dispatch.resourcesNum; ++resource_index) {
      const nrd::ResourceDesc& resource_desc = dispatch.resources[resource_index];
      Rt12NrdTexture* texture = Rt12NrdFindTexture(resource_desc, pools, slot);
      if (texture == nullptr) {
        slot->dispatch_constants->Unmap(0u, nullptr);
        return false;
      }
      const bool is_srv = resource_desc.descriptorType == nrd::DescriptorType::TEXTURE;
      if (!is_srv && resource_desc.descriptorType != nrd::DescriptorType::STORAGE_TEXTURE) {
        slot->dispatch_constants->Unmap(0u, nullptr);
        return false;
      }
      if (barrier_count == barriers.size()) {
        slot->dispatch_constants->Unmap(0u, nullptr);
        return false;
      }
      if (texture->state != (is_srv ? D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE : D3D12_RESOURCE_STATE_UNORDERED_ACCESS)) {
        auto& barrier = barriers[barrier_count++];
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = texture->resource.Get();
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrier.Transition.StateBefore = texture->state;
        barrier.Transition.StateAfter = is_srv ? D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
                                               : D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        texture->state = barrier.Transition.StateAfter;
        texture->unordered_access_written = false;
      } else if (!is_srv && texture->unordered_access_written) {
        auto& barrier = barriers[barrier_count++];
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        barrier.UAV.pResource = texture->resource.Get();
        texture->unordered_access_written = false;
      }
      D3D12_CPU_DESCRIPTOR_HANDLE descriptor = {};
      if (is_srv) {
        if (srv_index >= srv_count) {
          slot->dispatch_constants->Unmap(0u, nullptr);
          return false;
        }
        descriptor.ptr = dispatch_cpu.ptr + static_cast<SIZE_T>(srv_index++) * descriptor_size;
        D3D12_SHADER_RESOURCE_VIEW_DESC view = {};
        view.Format = texture->format;
        view.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        view.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        view.Texture2D.MipLevels = 1u;
        device->CreateShaderResourceView(texture->resource.Get(), &view, descriptor);
      } else {
        if (uav_index >= uav_count) {
          slot->dispatch_constants->Unmap(0u, nullptr);
          return false;
        }
        descriptor.ptr = dispatch_cpu.ptr + static_cast<SIZE_T>(srv_count + uav_index++) * descriptor_size;
        D3D12_UNORDERED_ACCESS_VIEW_DESC view = {};
        view.Format = texture->format;
        view.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
        device->CreateUnorderedAccessView(texture->resource.Get(), nullptr, &view, descriptor);
        texture->unordered_access_written = true;
      }
    }
    if (barrier_count != 0u) list->ResourceBarrier(barrier_count, barriers.data());
    ID3D12DescriptorHeap* heaps[] = {slot->dispatch_descriptors.Get()};
    list->SetDescriptorHeaps(1u, heaps);
    list->SetComputeRootSignature(pipelines->root_signature.Get());
    list->SetComputeRootConstantBufferView(0u, slot->dispatch_constants->GetGPUVirtualAddress()
                                                   + static_cast<uint64_t>(dispatch_index) * constant_stride);
    list->SetComputeRootDescriptorTable(1u, dispatch_gpu);
    list->SetComputeRootDescriptorTable(2u, {dispatch_gpu.ptr + static_cast<UINT64>(srv_count) * descriptor_size});
    list->SetPipelineState(pipelines->pipelines[dispatch.pipelineIndex].Get());
    list->Dispatch(dispatch.gridWidth, dispatch.gridHeight, 1u);
  }
  slot->dispatch_constants->Unmap(0u, nullptr);
  std::array<D3D12_RESOURCE_BARRIER, 64u> barriers = {};
  uint32_t barrier_count = 0u;
  for (Rt12NrdTexture& texture : slot->resources) {
    if (!texture.resource || texture.state == D3D12_RESOURCE_STATE_COMMON) continue;
    if (barrier_count == barriers.size()) return false;
    auto& barrier = barriers[barrier_count++];
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = texture.resource.Get();
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = texture.state;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COMMON;
    texture.state = D3D12_RESOURCE_STATE_COMMON;
    texture.unordered_access_written = false;
  }
  for (Rt12NrdTexture& texture : pools->permanent) {
    if (!texture.resource || texture.state == D3D12_RESOURCE_STATE_COMMON) continue;
    if (barrier_count == barriers.size()) return false;
    auto& barrier = barriers[barrier_count++];
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = texture.resource.Get();
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = texture.state;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COMMON;
    texture.state = D3D12_RESOURCE_STATE_COMMON;
    texture.unordered_access_written = false;
  }
  for (Rt12NrdTexture& texture : pools->transient) {
    if (!texture.resource || texture.state == D3D12_RESOURCE_STATE_COMMON) continue;
    if (barrier_count == barriers.size()) return false;
    auto& barrier = barriers[barrier_count++];
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = texture.resource.Get();
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = texture.state;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COMMON;
    texture.state = D3D12_RESOURCE_STATE_COMMON;
    texture.unordered_access_written = false;
  }
  if (barrier_count != 0u) list->ResourceBarrier(barrier_count, barriers.data());
  return true;
}

class Rt12NrdGeneration {
 public:
  Rt12NrdGeneration() = default;
  Rt12NrdGeneration(const Rt12NrdGeneration&) = delete;
  Rt12NrdGeneration& operator=(const Rt12NrdGeneration&) = delete;

  ~Rt12NrdGeneration() { Reset(); }

  bool CreateRelaxDiffuseSpecular() {
    Reset();
    const nrd::LibraryDesc* library = nrd::GetLibraryDesc();
    if (library == nullptr || library->versionMajor != NRD_VERSION_MAJOR || library->versionMinor != NRD_VERSION_MINOR
        || library->versionBuild != NRD_VERSION_BUILD || library->normalEncoding != nrd::NormalEncoding::R10_G10_B10_A2_UNORM
        || library->roughnessEncoding != nrd::RoughnessEncoding::LINEAR) {
      return false;
    }

    const nrd::DenoiserDesc denoiser = {1u, nrd::Denoiser::RELAX_DIFFUSE_SPECULAR};
    const nrd::InstanceCreationDesc creation = {{}, &denoiser, 1u};
    if (nrd::CreateInstance(creation, instance_) != nrd::Result::SUCCESS || instance_ == nullptr) {
      Reset();
      return false;
    }

    return true;
  }

  bool CreateD3D12Backend(ID3D12Device* device, PFN_D3D12_SERIALIZE_ROOT_SIGNATURE serialize) {
    if (instance_ == nullptr) return false;
    const nrd::InstanceDesc* instance_desc = nrd::GetInstanceDesc(*instance_);
    return instance_desc != nullptr && pipelines.Create(device, *instance_desc, serialize);
  }

  bool CreatePools(ID3D12Device* device, uint32_t width, uint32_t height) {
    if (instance_ == nullptr || device == nullptr || width == 0u || height == 0u) return false;
    const nrd::InstanceDesc* instance_desc = nrd::GetInstanceDesc(*instance_);
    if (instance_desc == nullptr) return false;
    if (pools.width == width && pools.height == height
        && pools.permanent.size() == instance_desc->permanentPoolSize
        && pools.transient.size() == instance_desc->transientPoolSize) return true;
    Rt12NrdResourcePools candidate;
    candidate.width = width;
    candidate.height = height;
    candidate.permanent.resize(instance_desc->permanentPoolSize);
    candidate.transient.resize(instance_desc->transientPoolSize);
    const auto create_pool = [&](const nrd::TextureDesc& source, Rt12NrdTexture* target) {
      if (source.downsampleFactor == 0u) return false;
      const uint64_t resource_width = (uint64_t{width} + source.downsampleFactor - 1u) / source.downsampleFactor;
      const uint64_t resource_height = (uint64_t{height} + source.downsampleFactor - 1u) / source.downsampleFactor;
      if (resource_width == 0u || resource_height == 0u || resource_width > UINT32_MAX || resource_height > UINT32_MAX) return false;
      DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
      switch (source.format) {
        case nrd::Format::R8_UNORM:             format = DXGI_FORMAT_R8_UNORM; break;
        case nrd::Format::RGBA8_UNORM:          format = DXGI_FORMAT_R8G8B8A8_UNORM; break;
        case nrd::Format::R16_SFLOAT:           format = DXGI_FORMAT_R16_FLOAT; break;
        case nrd::Format::RG16_SFLOAT:          format = DXGI_FORMAT_R16G16_FLOAT; break;
        case nrd::Format::RGBA16_SFLOAT:        format = DXGI_FORMAT_R16G16B16A16_FLOAT; break;
        case nrd::Format::R32_SFLOAT:           format = DXGI_FORMAT_R32_FLOAT; break;
        case nrd::Format::R11_G11_B10_UFLOAT:   format = DXGI_FORMAT_R11G11B10_FLOAT; break;
        case nrd::Format::RGBA8_UINT:           format = DXGI_FORMAT_R8G8B8A8_UINT; break;
        case nrd::Format::R10_G10_B10_A2_UNORM: format = DXGI_FORMAT_R10G10B10A2_UNORM; break;
        default:                                return false;
      }
      D3D12_RESOURCE_DESC desc = {};
      desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
      desc.Width = resource_width;
      desc.Height = static_cast<UINT>(resource_height);
      desc.DepthOrArraySize = 1u;
      desc.MipLevels = 1u;
      desc.Format = format;
      desc.SampleDesc.Count = 1u;
      desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
      desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
      D3D12_HEAP_PROPERTIES heap = {};
      heap.Type = D3D12_HEAP_TYPE_DEFAULT;
      heap.CreationNodeMask = 1u;
      heap.VisibleNodeMask = 1u;
      if (FAILED(device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc,
                                                 D3D12_RESOURCE_STATE_COMMON, nullptr,
                                                 IID_PPV_ARGS(&target->resource)))) return false;
      target->format = format;
      target->width = static_cast<uint32_t>(resource_width);
      target->height = static_cast<uint32_t>(resource_height);
      target->state = D3D12_RESOURCE_STATE_COMMON;
      target->unordered_access_written = false;
      candidate.bytes += device->GetResourceAllocationInfo(0u, 1u, &desc).SizeInBytes;
      return true;
    };
    for (uint32_t index = 0u; index < candidate.permanent.size(); ++index) {
      if (!create_pool(instance_desc->permanentPool[index], &candidate.permanent[index])) return false;
    }
    for (uint32_t index = 0u; index < candidate.transient.size(); ++index) {
      if (!create_pool(instance_desc->transientPool[index], &candidate.transient[index])) return false;
    }
    pools = std::move(candidate);
    return true;
  }

  void Reset() {
    pipelines.Reset();
    pools = {};
    history = {};
    if (instance_ != nullptr) {
      nrd::DestroyInstance(*instance_);
      instance_ = nullptr;
    }
  }

  nrd::Instance* Instance() const { return instance_; }

  Rt12NrdPipelineSet pipelines;
  Rt12NrdResourcePools pools;
  Rt12NrdHistoryIdentity history;
  rtxdi::CameraQualification qualification;

 private:
  nrd::Instance* instance_ = nullptr;
};

}  // namespace falcom_world::rt12
