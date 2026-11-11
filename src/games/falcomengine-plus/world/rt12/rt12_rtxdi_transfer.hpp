#pragma once

#include <algorithm>
#include <bit>
#include <chrono>
#include <cmath>
#include <numbers>

#include "../rr/rr_state.hpp"
#include "../rtsun/rtsun_state.hpp"
#include "../rtxdi/rtxdi_di_contract.h"
#include "../rtxdi/rtxdi_di_layout.h"
#include "../rtxdi/rtxdi_input_d3d11.hpp"
#include "../rtxdi/rtxdi_surface_layout.h"
#include "rt12_backend.hpp"

namespace falcom_world::rt12 {

struct Rt12DiInputViews {
  reshade::api::resource_view depth = {}, normal = {}, color = {}, mrt1 = {}, mrt2 = {}, motion = {};
  reshade::api::resource_view lights = {}, tiles = {}, deferred = {};
  rtxdi::SceneRangeCapture* scene_tracker = nullptr;
  uint64_t capture_frame = UINT64_MAX;
  uint32_t frame = UINT32_MAX, width = 0u, height = 0u;
  uint32_t lights_register = UINT32_MAX, tiles_register = UINT32_MAX, deferred_register = UINT32_MAX;
  uint32_t effective_debug_view = RTXDI_DEBUG_VIEW_OFF;
  uint32_t sun_detail_view = RTXDI_SUN_DETAIL_STATUS;
  std::array<float, 7> n1_brdf_config = {};
  rtxdi::EffectiveRtxdiSettings effective_settings = {};
  int32_t scene_slot = -1;
  uint64_t scene_fence = 0u;
  bool local = false, sun = false;
};

struct Rt12DiInputResult {
  rtxdi::InputStatus status = rtxdi::InputStatus::MissingInput;
  rtxdi::InputStage stage = rtxdi::InputStage::Validate;
  HRESULT error = S_OK;
  rtxdi::DiagnosticInputKind validation_input = rtxdi::DiagnosticInputKind::None;
  rtxdi::DiagnosticValidationCode validation_code = rtxdi::DiagnosticValidationCode::None;
  RtxdiInputLayout layout = {};
  uint64_t fence = 0u;
  int32_t transfer_slot = -1;
};

enum class Rt12DiTextureKind { Float,
                               Motion,
                               Uint4,
                               Uint,
                               Uint2 };

inline bool ValidateDiTexture(reshade::api::resource_view source, uint32_t width, uint32_t height, Rt12DiTextureKind kind) {
  if (!source.handle) return false;
  auto* view = reinterpret_cast<ID3D11ShaderResourceView*>(source.handle);  // NOLINT(performance-no-int-to-ptr)
  D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
  view->GetDesc(&desc);
  bool format_ok = false;
  switch (kind) {
    case Rt12DiTextureKind::Float:
      format_ok = desc.Format == DXGI_FORMAT_R32_FLOAT || desc.Format == DXGI_FORMAT_R16_FLOAT
                  || desc.Format == DXGI_FORMAT_R16_UNORM || desc.Format == DXGI_FORMAT_R24_UNORM_X8_TYPELESS
                  || desc.Format == DXGI_FORMAT_R16G16_FLOAT || desc.Format == DXGI_FORMAT_R32G32_FLOAT || desc.Format == DXGI_FORMAT_R16G16B16A16_FLOAT
                  || desc.Format == DXGI_FORMAT_R32G32B32A32_FLOAT || desc.Format == DXGI_FORMAT_R8G8B8A8_UNORM
                  || desc.Format == DXGI_FORMAT_R11G11B10_FLOAT;
      break;
    case Rt12DiTextureKind::Motion:
      format_ok = desc.Format == DXGI_FORMAT_R16G16_FLOAT;
      break;
    case Rt12DiTextureKind::Uint4:
      format_ok = desc.Format == DXGI_FORMAT_R8G8B8A8_UINT || desc.Format == DXGI_FORMAT_R16G16B16A16_UINT
                  || desc.Format == DXGI_FORMAT_R32G32B32A32_UINT;
      break;
    case Rt12DiTextureKind::Uint:
      format_ok = desc.Format == DXGI_FORMAT_R32_UINT;
      break;
    case Rt12DiTextureKind::Uint2:
      format_ok = desc.Format == DXGI_FORMAT_R16G16_UINT || desc.Format == DXGI_FORMAT_R32G32_UINT;
      break;
  }
  return format_ok && rtxdi::ValidateTextureView(view, width, height, desc.Format);
}

// Scene CBV identity is checked against the current binding before each stage.
inline bool CurrentDiSceneRange(reshade::api::device* device, reshade::api::command_list* cmd_list,
                                const Rt12DiInputViews& in, ID3D11Buffer* expected_scene,
                                reshade::api::buffer_range* range,
                                rtxdi::SceneRangeEvidence* evidence = nullptr) {
  if (evidence != nullptr) *evidence = {};
  if (!device || !cmd_list || !in.scene_tracker || !range) {
    if (evidence != nullptr) evidence->binding.reason = rtxdi::DiagnosticReason::InvalidResource;
    return false;
  }
  const uintptr_t command = reinterpret_cast<uintptr_t>(cmd_list);
  rtxdi::SceneRangeSnapshot promoted, binding;
  {
    std::shared_lock lock(in.scene_tracker->mutex);
    promoted = in.scene_tracker->snapshot;
    const auto found = in.scene_tracker->bindings.find(command);
    if (found == in.scene_tracker->bindings.end()) {
      if (evidence != nullptr) {
        evidence->binding.promoted = promoted;
        evidence->binding.reason = rtxdi::DiagnosticReason::MissingEntry;
        evidence->expected_buffer = reinterpret_cast<uint64_t>(expected_scene);
      }
      return false;
    }
    binding = found->second;
  }
  if (evidence != nullptr) {
    evidence->entry_found = true;
    evidence->binding.promoted = promoted;
    evidence->binding.current = binding;
    evidence->expected_buffer = reinterpret_cast<uint64_t>(expected_scene);
    evidence->native_first_constant = promoted.native_first_constant;
    evidence->native_constant_count = promoted.native_constant_count;
  }
  if (!promoted.live || !binding.live || promoted.buffer == 0u
      || reinterpret_cast<uint64_t>(expected_scene) != promoted.buffer) {
    if (evidence != nullptr) {
      evidence->binding.reason = !promoted.live || !binding.live ? rtxdi::DiagnosticReason::NonliveBinding
                                 : promoted.buffer == 0u         ? rtxdi::DiagnosticReason::InvalidResource
                                                                 : rtxdi::DiagnosticReason::BufferMismatch;
    }
    return false;
  }
  const auto desc = device->get_resource_desc(reshade::api::resource{promoted.buffer});
  if (evidence != nullptr) {
    evidence->resource_desc_valid = desc.type == reshade::api::resource_type::buffer;
    evidence->resource_buffer = evidence->resource_desc_valid;
    evidence->binding.buffer_bytes = desc.buffer.size;
  }
  if (desc.type != reshade::api::resource_type::buffer) {
    if (evidence != nullptr) evidence->binding.reason = rtxdi::DiagnosticReason::InvalidResource;
    return false;
  }
  if (rtxdi::CheckPromotedSceneBinding(promoted, binding, in.capture_frame, command, desc.buffer.size,
                                       evidence != nullptr ? &evidence->binding : nullptr)
      != rtxdi::SceneBindingStatus::Ready) return false;
  if (!rtxdi::ValidateSceneBuffer(reinterpret_cast<ID3D11Buffer*>(promoted.buffer), promoted.offset,
                                  binding.size)) {
    if (evidence != nullptr) evidence->binding.reason = rtxdi::DiagnosticReason::InvalidRange;
    return false;
  }  // NOLINT(performance-no-int-to-ptr)
  {
    std::shared_lock lock(in.scene_tracker->mutex);
    const auto found = in.scene_tracker->bindings.find(command);
    if (found == in.scene_tracker->bindings.end()) {
      if (evidence != nullptr) evidence->binding.reason = rtxdi::DiagnosticReason::MissingEntry;
      return false;
    }
    if (rtxdi::CheckPromotedSceneBinding(in.scene_tracker->snapshot, found->second, in.capture_frame,
                                         command, desc.buffer.size,
                                         evidence != nullptr ? &evidence->binding : nullptr)
        != rtxdi::SceneBindingStatus::Ready) return false;
    if (in.scene_tracker->snapshot.sequence != promoted.sequence) {
      if (evidence != nullptr) evidence->binding.reason = rtxdi::DiagnosticReason::SequenceMismatch;
      return false;
    }
  }
  *range = {reshade::api::resource{promoted.buffer}, promoted.offset, promoted.size};
  return true;
}

struct DiSceneNativeBindingEvidence {
  uint64_t native_buffer = 0u;
  uint64_t native_offset_bytes = 0u;
  uint64_t native_count_bytes = 0u;
  uint32_t first_constant = 0u;
  uint32_t constant_count = 0u;
  uint32_t buffer_bytes = 0u;
  uint32_t bind_flags = 0u;
  HRESULT context_query_result = E_POINTER;
  bool context_query_available = false;
  bool buffer_available = false;
};

inline void RecordDiSceneEvidence(rtxdi::DiagnosticState& diagnostics, uint64_t epoch,
                                  rtxdi::DiagnosticStage stage, uint64_t owner_generation,
                                  uint64_t frame, int32_t scene_slot, int32_t transfer_slot,
                                  const rtxdi::SceneRangeEvidence& scene,
                                  const DiSceneNativeBindingEvidence& native, bool passed = false) {
  rtxdi::DiagnosticEvent event = {};
  event.owner_generation = owner_generation;
  event.frame = frame;
  event.capture_frame = scene.binding.current.frame;
  event.scene_slot = scene_slot;
  event.transfer_slot = transfer_slot;
  event.stage = stage;
  event.operation = rtxdi::DiagnosticOperation::Validate;
  event.reason = passed ? rtxdi::DiagnosticReason::None : scene.binding.reason;
  if (!passed && scene.binding.reason == rtxdi::DiagnosticReason::None) {
    event.reason = native.context_query_available ? rtxdi::DiagnosticReason::NativeB0Mismatch
                                                  : rtxdi::DiagnosticReason::ContextQueryFailed;
  }
  event.hresult = FAILED(native.context_query_result) ? native.context_query_result : 0;
  event.value0 = scene.binding.promoted.offset;
  event.value1 = scene.binding.promoted.size;
  event.buffer_bytes = scene.binding.buffer_bytes;
  event.normalized_size = scene.binding.normalized_size;
  event.native_buffer_bytes = native.buffer_bytes;
  event.native_offset_bytes = native.native_offset_bytes;
  event.native_count_bytes = native.native_count_bytes;
  event.native_bind_flags = native.bind_flags;
  event.available_rows = scene.binding.available_rows;
  event.required_rows = RTXDI_INPUT_SCENE_REQUIRED_ROWS;
  event.copied_rows = scene.binding.copied_rows;
  event.first_constant = native.first_constant;
  event.constant_count = native.constant_count;
  event.flags = (scene.entry_found ? 1u : 0u) | (scene.resource_desc_valid ? 2u : 0u)
                | (scene.binding.normalized_valid ? 4u : 0u) | (native.context_query_available ? 8u : 0u)
                | (native.buffer_available ? 16u : 0u)
                | (native.native_buffer == scene.expected_buffer ? 32u : 0u);
  event.identity_tokens[0] = rtxdi::DiagnosticIdentityToken(
      diagnostics, epoch, scene.binding.current.command, owner_generation);
  event.identity_tokens[1] = rtxdi::DiagnosticIdentityToken(
      diagnostics, epoch, scene.binding.promoted.buffer, scene.binding.promoted.reset_generation);
  event.identity_tokens[2] = rtxdi::DiagnosticIdentityToken(
      diagnostics, epoch, scene.binding.current.buffer, scene.binding.current.reset_generation);
  event.identity_tokens[3] = rtxdi::DiagnosticIdentityToken(
      diagnostics, epoch, native.native_buffer,
      scene.binding.current.reset_generation);
  rtxdi::RecordDiagnostic(diagnostics, epoch, event);
}

inline bool ValidateBoundDiScene(ID3D11DeviceContext* context, const reshade::api::buffer_range& range,
                                 rtxdi::SceneRangeEvidence* expected = nullptr,
                                 DiSceneNativeBindingEvidence* evidence = nullptr,
                                 rtxdi::SceneCopyWindow* copy_window = nullptr) {
  if (evidence != nullptr) *evidence = {};
  rtxdi::SceneB0BindingEvidence native;
  const bool valid = rtxdi::ValidateSceneB0Binding(
      context, reinterpret_cast<ID3D11Buffer*>(range.buffer.handle), range.offset,
      RTXDI_INPUT_SCENE_REQUIRED_ROWS, &native);  // NOLINT(performance-no-int-to-ptr)
  if (evidence != nullptr) {
    evidence->native_buffer = native.native_buffer;
    evidence->first_constant = native.first_constant;
    evidence->constant_count = native.constant_count;
    evidence->buffer_bytes = native.buffer_bytes;
    evidence->bind_flags = native.bind_flags;
    evidence->native_offset_bytes = uint64_t{native.first_constant} * 16u;
    evidence->native_count_bytes = native.constant_count == 4096u && evidence->native_offset_bytes <= native.buffer_bytes
                                       ? native.buffer_bytes - evidence->native_offset_bytes
                                       : uint64_t{native.constant_count} * 16u;
    evidence->context_query_result = native.context_query_result;
    evidence->context_query_available = native.context_query_available;
    evidence->buffer_available = native.buffer_available;
  }
  rtxdi::SceneCopyWindow window = {};
  const bool extent_matches = expected != nullptr && expected->resource_desc_valid
                              && expected->binding.buffer_bytes == native.buffer_bytes;
  const bool window_valid = expected != nullptr
                            && rtxdi::MakeSceneCopyWindow(native.buffer_bytes, range.offset, range.size,
                                                          native.first_constant, native.constant_count, &window);
  if (copy_window != nullptr) *copy_window = window;
  if (expected != nullptr) {
    expected->binding.available_rows = window.available_rows;
    expected->binding.copied_rows = window.copied_rows;
    if (!extent_matches) expected->binding.reason = rtxdi::DiagnosticReason::InvalidRange;
  }
  return valid && extent_matches && expected != nullptr && native.first_constant == expected->native_first_constant
         && native.constant_count == expected->native_constant_count && window_valid;
}

#if defined(FALCOM_NRD_ENABLED)
inline void CaptureNrdCameraRows(uintptr_t device, const Rt12DiInputViews& in,
                                 const reshade::api::buffer_range& range, uint32_t copied_rows,
                                 uint64_t owner_generation,
                                 Rt12NrdCameraCapture* camera) {
  if (camera == nullptr) return;
  *camera = {};
  const uint64_t copy_bytes = uint64_t{copied_rows} * 16u;
  if (device == 0u || in.scene_tracker == nullptr || copied_rows < RTXDI_INPUT_SCENE_REQUIRED_ROWS
      || copied_rows > RTXDI_INPUT_SCENE_ROWS) return;
  rtxdi::SceneRangeSnapshot promoted;
  {
    std::shared_lock lock(in.scene_tracker->mutex);
    promoted = in.scene_tracker->snapshot;
  }
  if (!promoted.live || !promoted.known || promoted.frame != in.capture_frame
      || promoted.buffer != range.buffer.handle || promoted.offset != range.offset
      || promoted.size != range.size || promoted.copied_rows != copied_rows || promoted.sequence == 0u) return;
  uint64_t write_sequence = 0u, write_frame = UINT64_MAX;
  if (!rtxdi::ReadSceneCpuMirror(device, range.buffer.handle, range.offset, copy_bytes, in.capture_frame,
                                 promoted.sequence, camera->scene_rows.data(), &write_sequence, &write_frame)) return;
  camera->resource = range.buffer.handle;
  camera->offset = range.offset;
  camera->binding_sequence = promoted.sequence;
  camera->write_sequence = write_sequence;
  camera->write_frame = write_frame;
  camera->owner_generation = owner_generation;
  camera->capture_frame = in.capture_frame;
  camera->valid = true;
}

inline bool EnsureNrdProbePipeline(Rt12State& state) {
  auto& transfer = state.di_transfer;
  if (transfer.nrd_probe_root && transfer.nrd_probe_pso) return true;
#if defined(__world_rtxdi_nrd_probe_EMBED_FILE)
  D3D12_DESCRIPTOR_RANGE ranges[2] = {};
  ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
  ranges[0].NumDescriptors = 5u;
  ranges[0].BaseShaderRegister = 0u;
  ranges[0].OffsetInDescriptorsFromTableStart = 0u;
  ranges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
  ranges[1].NumDescriptors = 1u;
  ranges[1].BaseShaderRegister = 0u;
  ranges[1].OffsetInDescriptorsFromTableStart = 0u;
  D3D12_ROOT_PARAMETER parameters[3] = {};
  parameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
  parameters[0].Descriptor.ShaderRegister = 0u;
  parameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
  parameters[1].DescriptorTable.NumDescriptorRanges = 1u;
  parameters[1].DescriptorTable.pDescriptorRanges = &ranges[0];
  parameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
  parameters[2].DescriptorTable.NumDescriptorRanges = 1u;
  parameters[2].DescriptorTable.pDescriptorRanges = &ranges[1];
  D3D12_ROOT_SIGNATURE_DESC desc = {};
  desc.NumParameters = 3u;
  desc.pParameters = parameters;
  ComPtr<ID3DBlob> blob, error;
  ComPtr<ID3D12RootSignature> root;
  ComPtr<ID3D12PipelineState> pso;
  HRESULT hr = state.serialize(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &error);
  if (SUCCEEDED(hr)) {
    hr = state.device12->CreateRootSignature(0u, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&root));
  }
  if (SUCCEEDED(hr)) {
    D3D12_COMPUTE_PIPELINE_STATE_DESC pipeline = {};
    pipeline.pRootSignature = root.Get();
    pipeline.CS = {__world_rtxdi_nrd_probe.data(), __world_rtxdi_nrd_probe.size()};
    hr = state.device12->CreateComputePipelineState(&pipeline, IID_PPV_ARGS(&pso));
  }
  if (FAILED(hr)) return false;
  transfer.nrd_probe_root = std::move(root);
  transfer.nrd_probe_pso = std::move(pso);
  return true;
#else
  return false;
#endif
}

inline bool EnsureNrdPrepPipeline(Rt12State& state) {
  auto& transfer = state.di_transfer;
  if (transfer.nrd_prep_root && transfer.nrd_prep_pso) return true;
#if defined(__world_rtxdi_nrd_prep_EMBED_FILE)
  D3D12_DESCRIPTOR_RANGE ranges[2] = {};
  ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
  ranges[0].NumDescriptors = 10u;
  ranges[0].BaseShaderRegister = 0u;
  ranges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
  ranges[1].NumDescriptors = 5u;
  ranges[1].BaseShaderRegister = 0u;
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
  D3D12_ROOT_SIGNATURE_DESC desc = {};
  desc.NumParameters = 3u;
  desc.pParameters = parameters;
  ComPtr<ID3DBlob> blob, error;
  ComPtr<ID3D12RootSignature> root;
  ComPtr<ID3D12PipelineState> pso;
  HRESULT hr = state.serialize(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &error);
  if (SUCCEEDED(hr)) {
    hr = state.device12->CreateRootSignature(0u, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&root));
  }
  if (SUCCEEDED(hr)) {
    D3D12_COMPUTE_PIPELINE_STATE_DESC pipeline = {};
    pipeline.pRootSignature = root.Get();
    pipeline.CS = {__world_rtxdi_nrd_prep.data(), __world_rtxdi_nrd_prep.size()};
    hr = state.device12->CreateComputePipelineState(&pipeline, IID_PPV_ARGS(&pso));
  }
  if (FAILED(hr)) return false;
  transfer.nrd_prep_root = std::move(root);
  transfer.nrd_prep_pso = std::move(pso);
  return true;
#else
  return false;
#endif
}

inline DXGI_FORMAT NrdResourceFormat(nrd::ResourceType type) {
  switch (type) {
    case nrd::ResourceType::IN_MV:                         return DXGI_FORMAT_R16G16_FLOAT;
    case nrd::ResourceType::IN_NORMAL_ROUGHNESS:           return DXGI_FORMAT_R10G10B10A2_UNORM;
    case nrd::ResourceType::IN_VIEWZ:                      return DXGI_FORMAT_R32_FLOAT;
    case nrd::ResourceType::IN_DIFF_RADIANCE_HITDIST:
    case nrd::ResourceType::IN_SPEC_RADIANCE_HITDIST:
    case nrd::ResourceType::OUT_DIFF_RADIANCE_HITDIST:
    case nrd::ResourceType::OUT_SPEC_RADIANCE_HITDIST:
    case nrd::ResourceType::IN_DIFF_SH0:
    case nrd::ResourceType::IN_DIFF_SH1:
    case nrd::ResourceType::IN_SPEC_SH0:
    case nrd::ResourceType::IN_SPEC_SH1:
    case nrd::ResourceType::OUT_DIFF_SH0:
    case nrd::ResourceType::OUT_DIFF_SH1:
    case nrd::ResourceType::OUT_SPEC_SH0:
    case nrd::ResourceType::OUT_SPEC_SH1:                  return DXGI_FORMAT_R16G16B16A16_FLOAT;
    case nrd::ResourceType::IN_DIFF_DIRECTION_HITDIST:     return DXGI_FORMAT_R8G8B8A8_UNORM;
    case nrd::ResourceType::IN_DIFF_CONFIDENCE:
    case nrd::ResourceType::IN_SPEC_CONFIDENCE:
    case nrd::ResourceType::IN_DISOCCLUSION_THRESHOLD_MIX:
    case nrd::ResourceType::IN_SIGNAL:
    case nrd::ResourceType::OUT_DIFF_HITDIST:
    case nrd::ResourceType::OUT_SPEC_HITDIST:              return DXGI_FORMAT_R8_UNORM;
    case nrd::ResourceType::IN_PENUMBRA:                   return DXGI_FORMAT_R16_FLOAT;
    case nrd::ResourceType::IN_TRANSLUCENCY:               return DXGI_FORMAT_R8G8B8A8_UNORM;
    case nrd::ResourceType::OUT_VALIDATION:                return DXGI_FORMAT_R8G8B8A8_UNORM;
    case nrd::ResourceType::OUT_SHADOW_TRANSLUCENCY:       return DXGI_FORMAT_R8G8B8A8_UNORM;
    default:                                               return DXGI_FORMAT_UNKNOWN;
  }
}

inline bool EnsureNrdDispatchResources(Rt12State& state, Rt12NrdSlot* slot, uint32_t width, uint32_t height,
                                       const nrd::DispatchDesc* dispatches, uint32_t dispatch_count) {
  if (slot == nullptr || !state.device12 || dispatches == nullptr || dispatch_count == 0u) return false;
  const nrd::InstanceDesc* instance = nrd::GetInstanceDesc(*state.di_transfer.nrd.Instance());
  if (instance == nullptr) return false;
  const bool recreate = slot->width != width || slot->height != height || slot->dispatch_count != dispatch_count
                        || !slot->dispatch_descriptors || !slot->dispatch_constants;
  if (recreate) {
    slot->resources = {};
    slot->dispatch_descriptors.Reset();
    slot->dispatch_constants.Reset();
    slot->prep_descriptors.Reset();
    slot->publication_descriptors.Reset();
    slot->publication_constants.Reset();
    slot->width = width;
    slot->height = height;
    slot->dispatch_count = dispatch_count;
    const uint32_t descriptor_stride = instance->descriptorPoolDesc.perSetTexturesMaxNum
                                       + instance->descriptorPoolDesc.perSetStorageTexturesMaxNum;
    if (dispatch_count > UINT32_MAX / descriptor_stride) return false;
    D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
    heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heap_desc.NumDescriptors = dispatch_count * descriptor_stride;
    heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    if (FAILED(state.device12->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&slot->dispatch_descriptors)))) return false;
    heap_desc.NumDescriptors = 15u;
    if (FAILED(state.device12->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&slot->prep_descriptors)))) return false;
    heap_desc.NumDescriptors = 5u;
    if (FAILED(state.device12->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&slot->publication_descriptors)))) return false;
    if (FAILED(CreateRt12Buffer(state.device12.Get(), D3D12_HEAP_TYPE_UPLOAD, 256u,
                                D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_FLAG_NONE,
                                slot->publication_constants))) return false;
    const uint64_t constant_stride = (instance->constantBufferMaxDataSize + 255u) & ~uint64_t{255u};
    if (constant_stride == 0u || constant_stride > UINT64_MAX / dispatch_count
        || FAILED(CreateRt12Buffer(state.device12.Get(), D3D12_HEAP_TYPE_UPLOAD, constant_stride * dispatch_count,
                                   D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_FLAG_NONE,
                                   slot->dispatch_constants))) return false;
  }
  for (uint32_t dispatch_index = 0u; dispatch_index < dispatch_count; ++dispatch_index) {
    for (uint32_t resource_index = 0u; resource_index < dispatches[dispatch_index].resourcesNum; ++resource_index) {
      const nrd::ResourceDesc& resource = dispatches[dispatch_index].resources[resource_index];
      const uint32_t resource_index_in_array = static_cast<uint32_t>(resource.type);
      if (resource_index_in_array >= static_cast<uint32_t>(nrd::ResourceType::TRANSIENT_POOL)) continue;
      Rt12NrdTexture& texture = slot->resources[resource_index_in_array];
      if (texture.resource) continue;
      const DXGI_FORMAT format = NrdResourceFormat(resource.type);
      if (format == DXGI_FORMAT_UNKNOWN) return false;
      D3D12_RESOURCE_DESC desc = {};
      desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
      desc.Width = width;
      desc.Height = height;
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
      if (FAILED(state.device12->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc,
                                                         D3D12_RESOURCE_STATE_COMMON, nullptr,
                                                         IID_PPV_ARGS(&texture.resource)))) return false;
      texture.format = format;
      texture.width = width;
      texture.height = height;
      texture.state = D3D12_RESOURCE_STATE_COMMON;
    }
  }
  const std::array<nrd::ResourceType, 7u> prep_resources = {
      nrd::ResourceType::IN_MV, nrd::ResourceType::IN_NORMAL_ROUGHNESS, nrd::ResourceType::IN_VIEWZ,
      nrd::ResourceType::IN_DIFF_RADIANCE_HITDIST, nrd::ResourceType::IN_SPEC_RADIANCE_HITDIST,
      nrd::ResourceType::OUT_DIFF_RADIANCE_HITDIST, nrd::ResourceType::OUT_SPEC_RADIANCE_HITDIST};
  for (nrd::ResourceType type : prep_resources) {
    Rt12NrdTexture& texture = slot->resources[static_cast<uint32_t>(type)];
    if (texture.resource) continue;
    const DXGI_FORMAT format = NrdResourceFormat(type);
    if (format == DXGI_FORMAT_UNKNOWN) return false;
    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Width = width;
    desc.Height = height;
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
    if (FAILED(state.device12->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc,
                                                       D3D12_RESOURCE_STATE_COMMON, nullptr,
                                                       IID_PPV_ARGS(&texture.resource)))) return false;
    texture.format = format;
    texture.width = width;
    texture.height = height;
    texture.state = D3D12_RESOURCE_STATE_COMMON;
  }
  return true;
}

#if defined(FALCOM_NRD_ENABLED)
inline bool RecordNrdDiffusePrep(Rt12State& state, Rt12DiTransferSlot& slot, const Rt12DiInputViews& input,
                                 uint32_t correction_index, bool history_valid) {
  if (!EnsureNrdPrepPipeline(state) || !slot.nrd.prep_descriptors || !slot.nrd.hit_distance || !slot.nrd.hit_validity
      || !slot.depth.texture12
      || !slot.normal.texture12 || !slot.scene.texture12 || !slot.motion.texture12 || !slot.mrt2.texture12
      || !slot.deferred_buffer || !slot.n1_diffuse.texture12 || !slot.n1_status.texture12) return false;
  auto& transfer = state.di_transfer;
  auto& nrd_slot = slot.nrd;
  ID3D12Device* device = state.device12.Get();
  ID3D12GraphicsCommandList* list = transfer.di_list.Get();
  const uint32_t descriptor_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
  const D3D12_CPU_DESCRIPTOR_HANDLE cpu = nrd_slot.prep_descriptors->GetCPUDescriptorHandleForHeapStart();
  const D3D12_GPU_DESCRIPTOR_HANDLE gpu = nrd_slot.prep_descriptors->GetGPUDescriptorHandleForHeapStart();
  const std::array<ID3D12Resource*, 10u> srvs = {slot.depth.texture12.Get(), slot.normal.texture12.Get(), slot.scene.texture12.Get(),
                                                 slot.motion.texture12.Get(), slot.mrt2.texture12.Get(), slot.deferred_buffer.Get(),
                                                 slot.n1_diffuse.texture12.Get(), slot.n1_status.texture12.Get(),
                                                 slot.nrd.hit_distance.Get(), slot.nrd.hit_validity.Get()};
  const std::array<DXGI_FORMAT, 10u> srv_formats = {DXGI_FORMAT_R32_FLOAT, DXGI_FORMAT_R32G32B32A32_UINT,
                                                    DXGI_FORMAT_R32G32B32A32_UINT, DXGI_FORMAT_R32G32B32A32_FLOAT,
                                                    DXGI_FORMAT_R32G32B32A32_UINT, DXGI_FORMAT_UNKNOWN,
                                                    DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_R32_UINT,
                                                    DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN};
  std::array<D3D12_RESOURCE_BARRIER, 18u> barriers = {};
  uint32_t barrier_count = 0u;
  for (uint32_t index = 0u; index < srvs.size(); ++index) {
    ID3D12Resource* resource = srvs[index];
    if (resource == nullptr) return false;
    if (index == 6u || index == 7u) {
      D3D12_RESOURCE_BARRIER& barrier = barriers[barrier_count++];
      barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
      barrier.Transition.pResource = resource;
      barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
      barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
      barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    } else if (index >= 8u) {
      D3D12_RESOURCE_BARRIER& barrier = barriers[barrier_count++];
      barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
      barrier.Transition.pResource = resource;
      barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
      barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
      barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    }
    D3D12_SHADER_RESOURCE_VIEW_DESC view = {};
    view.Format = srv_formats[index];
    view.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    if (index == 5u) {
      view.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
      view.Buffer.NumElements = std::max(1u, slot.deferred_count);
      view.Buffer.StructureByteStride = RTXDI_INPUT_DEFERRED_STRIDE;
    } else if (index < 8u) {
      view.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
      view.Texture2D.MipLevels = 1u;
    } else {
      view.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
      view.Buffer.NumElements = static_cast<UINT>(uint64_t{input.width} * input.height);
      view.Buffer.StructureByteStride = sizeof(uint32_t);
    }
    device->CreateShaderResourceView(resource, &view, {cpu.ptr + static_cast<SIZE_T>(index) * descriptor_size});
  }
  const std::array<nrd::ResourceType, 5u> uav_types = {nrd::ResourceType::IN_MV,
                                                       nrd::ResourceType::IN_NORMAL_ROUGHNESS,
                                                       nrd::ResourceType::IN_VIEWZ,
                                                       nrd::ResourceType::IN_DIFF_RADIANCE_HITDIST,
                                                       nrd::ResourceType::IN_SPEC_RADIANCE_HITDIST};
  const std::array<Rt12NrdTexture*, 5u> uavs = {
      &nrd_slot.resources[static_cast<uint32_t>(uav_types[0])], &nrd_slot.resources[static_cast<uint32_t>(uav_types[1])],
      &nrd_slot.resources[static_cast<uint32_t>(uav_types[2])], &nrd_slot.resources[static_cast<uint32_t>(uav_types[3])],
      &nrd_slot.resources[static_cast<uint32_t>(uav_types[4])]};
  for (uint32_t index = 0u; index < uavs.size(); ++index) {
    Rt12NrdTexture* texture = uavs[index];
    if (texture->resource == nullptr) return false;
    D3D12_RESOURCE_BARRIER& barrier = barriers[barrier_count++];
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = texture->resource.Get();
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = texture->state;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    texture->state = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    D3D12_UNORDERED_ACCESS_VIEW_DESC view = {};
    view.Format = texture->format;
    view.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    device->CreateUnorderedAccessView(texture->resource.Get(), nullptr, &view,
                                      {cpu.ptr + static_cast<SIZE_T>(10u + index) * descriptor_size});
  }
  if (barrier_count != 0u) list->ResourceBarrier(barrier_count, barriers.data());
  ID3D12DescriptorHeap* heaps[] = {nrd_slot.prep_descriptors.Get()};
  list->SetDescriptorHeaps(1u, heaps);
  list->SetComputeRootSignature(transfer.nrd_prep_root.Get());
  list->SetPipelineState(transfer.nrd_prep_pso.Get());
  const std::array<uint32_t, 10u> constants = {input.width, input.height, slot.deferred_count, correction_index,
                                               std::bit_cast<uint32_t>(nrd_slot.camera_candidate.motion_correction_candidates[0][0]),
                                               std::bit_cast<uint32_t>(nrd_slot.camera_candidate.motion_correction_candidates[0][1]),
                                               std::bit_cast<uint32_t>(nrd_slot.camera_candidate.motion_correction_candidates[1][0]),
                                               std::bit_cast<uint32_t>(nrd_slot.camera_candidate.motion_correction_candidates[1][1]),
                                               history_valid ? 1u : 0u, 0u};
  list->SetComputeRoot32BitConstants(0u, static_cast<UINT>(constants.size()), constants.data(), 0u);
  list->SetComputeRootDescriptorTable(1u, gpu);
  list->SetComputeRootDescriptorTable(2u, {gpu.ptr + static_cast<UINT64>(10u) * descriptor_size});
  list->Dispatch((input.width + 7u) / 8u, (input.height + 7u) / 8u, 1u);
  barriers = {};
  barrier_count = 0u;
  for (uint32_t index = 6u; index < 10u; ++index) {
    D3D12_RESOURCE_BARRIER& barrier = barriers[barrier_count++];
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = srvs[index];
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    barrier.Transition.StateAfter = index < 8u ? D3D12_RESOURCE_STATE_UNORDERED_ACCESS : D3D12_RESOURCE_STATE_COMMON;
  }
  list->ResourceBarrier(barrier_count, barriers.data());
  return true;
}

inline bool BuildNrdCommonSettings(const rtxdi::CameraCandidate& camera, uint32_t width, uint32_t height,
                                   uint32_t frame_index, bool history_valid, nrd::CommonSettings* settings) {
  if (settings == nullptr || width == 0u || height == 0u || width > UINT16_MAX || height > UINT16_MAX
      || !camera.matrices_valid || !camera.absolute_jitter_valid) return false;
  *settings = {};
  std::copy(camera.view_to_clip.begin(), camera.view_to_clip.end(), settings->viewToClipMatrix);
  std::copy(camera.view_to_clip_previous.begin(), camera.view_to_clip_previous.end(), settings->viewToClipMatrixPrev);
  std::copy(camera.world_to_view.begin(), camera.world_to_view.end(), settings->worldToViewMatrix);
  std::copy(camera.world_to_view_previous.begin(), camera.world_to_view_previous.end(), settings->worldToViewMatrixPrev);
  settings->motionVectorScale[0] = 1.0f;
  settings->motionVectorScale[1] = 1.0f;
  settings->motionVectorScale[2] = 0.0f;
  settings->cameraJitter[0] = camera.jitter_pixels[0] / static_cast<float>(width);
  settings->cameraJitter[1] = camera.jitter_pixels[1] / static_cast<float>(height);
  settings->cameraJitterPrev[0] = camera.jitter_pixels_previous[0] / static_cast<float>(width);
  settings->cameraJitterPrev[1] = camera.jitter_pixels_previous[1] / static_cast<float>(height);
  settings->resourceSize[0] = settings->resourceSizePrev[0] = static_cast<uint16_t>(width);
  settings->resourceSize[1] = settings->resourceSizePrev[1] = static_cast<uint16_t>(height);
  settings->rectSize[0] = settings->rectSizePrev[0] = static_cast<uint16_t>(width);
  settings->rectSize[1] = settings->rectSizePrev[1] = static_cast<uint16_t>(height);
  settings->frameIndex = frame_index;
  settings->accumulationMode = history_valid ? nrd::AccumulationMode::CONTINUE : nrd::AccumulationMode::RESTART;
  settings->isMotionVectorInWorldSpace = false;
  settings->denoisingRange = 500000.0f;
  return true;
}

inline bool RecordNrdDenoise(Rt12State& state, Rt12DiTransferSlot& slot, const Rt12DiInputViews& input,
                             const rtxdi::EffectiveRtxdiSettings& settings,
                             Rt12NrdHistoryIdentity* accepted_history) {
  auto& generation = state.di_transfer.nrd;
  if (generation.Instance() == nullptr || accepted_history == nullptr || input.width == 0u || input.height == 0u
      || !slot.nrd.camera_candidate.matrices_valid || !slot.nrd.camera_candidate.absolute_jitter_valid) return false;
  if (generation.pools.width != input.width || generation.pools.height != input.height) {
    const uint64_t completed = Rt12CompletedValue(state);
    if (completed == UINT64_MAX) return false;
    for (Rt12DiTransferSlot& pending : state.di_transfer.slots) {
      if (pending.nrd.fence_value > completed) return false;
    }
  }
  if (!generation.CreatePools(state.device12.Get(), input.width, input.height)) return false;
  nrd::CommonSettings common = {};
  const uint64_t configuration = state.di_transfer.configuration_generation
                                 ^ (uint64_t{slot.n1_configuration_version} << 32u)
                                 ^ state.di_transfer.nrd.qualification.motion_correction_index
                                 ^ settings.identity;
  uint64_t configuration_hash = configuration ^ 1469598103934665603ull;
  for (float value : input.n1_brdf_config) {
    configuration_hash ^= std::bit_cast<uint32_t>(value);
    configuration_hash *= 1099511628211ull;
  }
  const Rt12NrdHistoryIdentity current = {reinterpret_cast<uint64_t>(state.device), state.generation,
                                          configuration_hash, input.frame, input.width, input.height, true};
  const bool history_valid = Rt12NrdCanContinueHistory(generation.history, current)
                             && generation.qualification.qualified;
  const uint32_t frame_index = history_valid ? static_cast<uint32_t>(generation.history.frame + 1u) : 0u;
  if (!BuildNrdCommonSettings(slot.nrd.camera_candidate, input.width, input.height,
                              frame_index, history_valid, &common)
      || nrd::SetCommonSettings(*generation.Instance(), common) != nrd::Result::SUCCESS) return false;
  nrd::RelaxSettings relax = {};
  relax.hitDistanceReconstructionMode = nrd::HitDistanceReconstructionMode::OFF;
  relax.minMaterialForDiffuse = 0.0f;
  relax.minMaterialForSpecular = 0.0f;
  relax.diffuseMaxAccumulatedFrameNum = settings.relax_diffuse_history;
  relax.diffuseMaxFastAccumulatedFrameNum = settings.relax_fast_history;
  relax.diffusePrepassBlurRadius = static_cast<float>(settings.relax_prepass_radius);
  relax.atrousIterationNum = settings.relax_atrous_iterations;
  if (nrd::SetDenoiserSettings(*generation.Instance(), 1u, &relax) != nrd::Result::SUCCESS) return false;
  const nrd::Identifier identifier = 1u;
  const nrd::DispatchDesc* dispatches = nullptr;
  uint32_t dispatch_count = 0u;
  if (nrd::GetComputeDispatches(*generation.Instance(), &identifier, 1u, dispatches, dispatch_count)
          != nrd::Result::SUCCESS
      || dispatches == nullptr || dispatch_count == 0u
      || !EnsureNrdDispatchResources(state, &slot.nrd, input.width, input.height, dispatches, dispatch_count)) return false;
  const nrd::InstanceDesc* instance = nrd::GetInstanceDesc(*generation.Instance());
  if (instance == nullptr) return false;
  if (!RecordNrdDiffusePrep(state, slot, input, generation.qualification.motion_correction_index, history_valid)
      || !Rt12NrdRecordDispatches(state.device12.Get(), state.di_transfer.di_list.Get(), *instance,
                                  dispatches, dispatch_count, &generation.pipelines, &generation.pools,
                                  &slot.nrd,
                                  instance->descriptorPoolDesc.perSetTexturesMaxNum
                                      + instance->descriptorPoolDesc.perSetStorageTexturesMaxNum)) return false;
  *accepted_history = current;
  return true;
}

#endif

inline bool EnsureNrdProbeResources(Rt12State& state, Rt12NrdSlot* slot) {
  if (slot == nullptr || !state.device12) return false;
  constexpr uint64_t results_bytes = 32u * 18u * 2u * sizeof(uint32_t);
  Rt12NrdSlot candidate;
  if (FAILED(CreateRt12Buffer(state.device12.Get(), D3D12_HEAP_TYPE_UPLOAD, 512u,
                              D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_FLAG_NONE,
                              candidate.probe_constants))
      || FAILED(CreateRt12Buffer(state.device12.Get(), D3D12_HEAP_TYPE_DEFAULT, results_bytes,
                                 D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                                 candidate.probe_results))
      || FAILED(CreateRt12Buffer(state.device12.Get(), D3D12_HEAP_TYPE_READBACK, results_bytes,
                                 D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_FLAG_NONE,
                                 candidate.probe_readback))
      || FAILED(CreateRt12Buffer(state.device12.Get(), D3D12_HEAP_TYPE_READBACK, 2u * sizeof(uint64_t),
                                 D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_FLAG_NONE,
                                 candidate.probe_timestamp_readback))) return false;
  D3D12_QUERY_HEAP_DESC queries = {};
  queries.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
  queries.Count = 2u;
  if (FAILED(state.device12->CreateQueryHeap(&queries, IID_PPV_ARGS(&candidate.probe_queries)))) return false;
  slot->probe_constants = std::move(candidate.probe_constants);
  slot->probe_results = std::move(candidate.probe_results);
  slot->probe_readback = std::move(candidate.probe_readback);
  slot->probe_timestamp_readback = std::move(candidate.probe_timestamp_readback);
  slot->probe_queries = std::move(candidate.probe_queries);
  return true;
}

inline void ProcessNrdProbeReadbacks(Rt12State& state) {
  const auto start = std::chrono::steady_clock::now();
  const uint64_t completed = Rt12CompletedValue(state);
  if (completed == UINT64_MAX) {
    state.di_transfer.nrd.qualification = {};
    return;
  }
  std::array<Rt12DiTransferSlot*, kRt12Slots> ready = {};
  uint32_t ready_count = 0u;
  for (Rt12DiTransferSlot& slot : state.di_transfer.slots) {
    if (slot.nrd.probe_pending && slot.nrd.probe_fence <= completed) ready[ready_count++] = &slot;
  }
  std::sort(ready.begin(), ready.begin() + ready_count, [](const auto* a, const auto* b) {
    return a->nrd.probe_frame < b->nrd.probe_frame;
  });
  for (uint32_t ready_index = 0u; ready_index < ready_count; ++ready_index) {
    Rt12DiTransferSlot& slot = *ready[ready_index];
    std::array<std::array<float, 32u * 18u>, 2u> residuals = {};
    void* mapped = nullptr;
    const D3D12_RANGE read_range = {0u, 32u * 18u * 2u * sizeof(uint32_t)};
    if (!slot.nrd.probe_readback || FAILED(slot.nrd.probe_readback->Map(0u, &read_range, &mapped))) {
      slot.nrd.probe_pending = false;
      state.di_transfer.nrd.qualification.qualified = false;
      state.di_transfer.nrd.qualification.consecutive_passes = 0u;
      continue;
    }
    const auto* raw = static_cast<const uint32_t*>(mapped);
    for (uint32_t sample = 0u; sample < 32u * 18u; ++sample) {
      for (uint32_t candidate = 0u; candidate < 2u; ++candidate) {
        residuals[candidate][sample] = std::bit_cast<float>(raw[sample * 2u + candidate]);
      }
    }
    const D3D12_RANGE no_write = {0u, 0u};
    slot.nrd.probe_readback->Unmap(0u, &no_write);
    uint32_t selected_correction = UINT32_MAX;
    float selected_median = INFINITY;
    for (uint32_t candidate = 0u; candidate < 2u; ++candidate) {
      uint32_t count = 0u;
      uint32_t inliers = 0u;
      for (float error : residuals[candidate]) {
        if (!std::isfinite(error)) continue;
        residuals[candidate][count++] = error;
        if (error <= 1.0f) ++inliers;
      }
      if (count < 64u || inliers < 64u) continue;
      std::sort(residuals[candidate].begin(), residuals[candidate].begin() + count);
      const float median = (count & 1u) ? residuals[candidate][count / 2u]
                                        : 0.5f * (residuals[candidate][count / 2u - 1u] + residuals[candidate][count / 2u]);
      const uint32_t p90_index = ((count * 9u + 9u) / 10u) - 1u;
      if (median <= 0.35f && residuals[candidate][p90_index] <= 1.0f && median < selected_median) {
        selected_correction = candidate;
        selected_median = median;
      }
    }
    const bool candidate_valid = slot.nrd.camera.valid && slot.nrd.camera_candidate.matrices_valid
                                 && slot.nrd.camera_candidate.absolute_jitter_valid;
    rtxdi::ObserveCameraProbe(&state.di_transfer.nrd.qualification, slot.nrd.probe_frame,
                              slot.nrd.camera.owner_generation, state.di_transfer.configuration_generation,
                              slot.nrd.camera.resource, slot.width, slot.height, candidate_valid,
                              selected_correction != UINT32_MAX, selected_correction);
    void* timestamp_data = nullptr;
    const D3D12_RANGE timestamp_range = {0u, 2u * sizeof(uint64_t)};
    if (slot.nrd.probe_timestamp_readback
        && SUCCEEDED(slot.nrd.probe_timestamp_readback->Map(0u, &timestamp_range, &timestamp_data))) {
      const auto* timestamps = static_cast<const uint64_t*>(timestamp_data);
      uint64_t frequency = 0u;
      if (state.queue12 && SUCCEEDED(state.queue12->GetTimestampFrequency(&frequency)) && frequency != 0u) {
        slot.nrd.probe_gpu_microseconds = static_cast<double>(timestamps[1] - timestamps[0]) * 1.0e6 / frequency;
      }
      slot.nrd.probe_timestamp_readback->Unmap(0u, &no_write);
    }
    slot.nrd.probe_readback_cpu_microseconds = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - start).count();
    slot.nrd.probe_pending = false;
  }
}

inline bool RecordNrdCameraProbe(Rt12State& state, Rt12DiTransferSlot& current,
                                 Rt12DiTransferSlot& previous, const Rt12DiInputViews& input,
                                 const rtxdi::CameraCandidate& candidate) {
  if (!EnsureNrdProbePipeline(state) || !EnsureNrdProbeResources(state, &current.nrd)
      || current.di_descriptors == nullptr || current.width != input.width || current.height != input.height
      || !previous.depth.texture12 || !previous.normal.texture12 || previous.width != current.width
      || previous.height != current.height) return false;
  rtxdi::CameraMatrix current_inverse_view_projection = {}, previous_inverse_view_projection = {};
  if (!rtxdi::ReadSceneMatrix(current.nrd.camera.scene_rows.data(), 20u, &current_inverse_view_projection)
      || !rtxdi::ReadSceneMatrix(previous.nrd.camera.scene_rows.data(), 20u, &previous_inverse_view_projection)) return false;
  Rt12NrdProbeConstants constants = {};
  constants.resource_size[0] = input.width;
  constants.resource_size[1] = input.height;
  constants.current_jitter_pixels[0] = candidate.jitter_pixels[0];
  constants.current_jitter_pixels[1] = candidate.jitter_pixels[1];
  constants.previous_jitter_pixels[0] = candidate.jitter_pixels_previous[0];
  constants.previous_jitter_pixels[1] = candidate.jitter_pixels_previous[1];
  for (uint32_t index = 0u; index < 2u; ++index) {
    constants.motion_correction_pixels[index][0] = candidate.motion_correction_candidates[index][0];
    constants.motion_correction_pixels[index][1] = candidate.motion_correction_candidates[index][1];
  }
  std::copy(current_inverse_view_projection.begin(), current_inverse_view_projection.end(),
            constants.current_inverse_view_projection);
  std::copy(previous_inverse_view_projection.begin(), previous_inverse_view_projection.end(),
            constants.previous_inverse_view_projection);
  const rtxdi::CameraMatrix current_vp = rtxdi::MultiplyCameraMatrices(candidate.world_to_view, candidate.view_to_clip);
  const rtxdi::CameraMatrix previous_vp = rtxdi::MultiplyCameraMatrices(candidate.world_to_view_previous,
                                                                        candidate.view_to_clip_previous);
  std::copy(current_vp.begin(), current_vp.end(), constants.current_view_projection_non_jittered);
  std::copy(previous_vp.begin(), previous_vp.end(), constants.previous_view_projection_non_jittered);
  std::copy(candidate.world_to_view_previous.begin(), candidate.world_to_view_previous.end(), constants.previous_view);
  void* mapped = nullptr;
  const D3D12_RANGE no_read = {0u, 0u};
  if (FAILED(current.nrd.probe_constants->Map(0u, &no_read, &mapped))) return false;
  std::memset(mapped, 0, 512u);
  std::memcpy(mapped, &constants, sizeof(constants));
  current.nrd.probe_constants->Unmap(0u, nullptr);

  ID3D12Device5* device = state.device12.Get();
  const auto cpu_at = [&](uint32_t index) {
    D3D12_CPU_DESCRIPTOR_HANDLE handle = current.di_descriptors->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += static_cast<SIZE_T>(index) * state.descriptor_size;
    return handle;
  };
  const auto create_texture_srv = [&](uint32_t index, ID3D12Resource* resource, DXGI_FORMAT format) {
    D3D12_SHADER_RESOURCE_VIEW_DESC view = {};
    view.Format = format;
    view.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    view.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    view.Texture2D.MipLevels = 1u;
    device->CreateShaderResourceView(resource, &view, cpu_at(index));
  };
  create_texture_srv(0u, current.depth.texture12.Get(), DXGI_FORMAT_R32_FLOAT);
  create_texture_srv(1u, previous.depth.texture12.Get(), DXGI_FORMAT_R32_FLOAT);
  create_texture_srv(2u, current.motion.texture12.Get(), DXGI_FORMAT_R32G32B32A32_FLOAT);
  create_texture_srv(3u, current.normal.texture12.Get(), DXGI_FORMAT_R32G32B32A32_UINT);
  create_texture_srv(4u, previous.normal.texture12.Get(), DXGI_FORMAT_R32G32B32A32_UINT);
  D3D12_UNORDERED_ACCESS_VIEW_DESC output_view = {};
  output_view.Format = DXGI_FORMAT_UNKNOWN;
  output_view.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
  output_view.Buffer.NumElements = 32u * 18u * 2u;
  output_view.Buffer.StructureByteStride = sizeof(uint32_t);
  device->CreateUnorderedAccessView(current.nrd.probe_results.Get(), nullptr, &output_view, cpu_at(36u));

  D3D12_RESOURCE_BARRIER barrier = {};
  barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  barrier.Transition.pResource = current.nrd.probe_results.Get();
  barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
  barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
  state.di_transfer.di_list->ResourceBarrier(1u, &barrier);
  ID3D12DescriptorHeap* heaps[] = {current.di_descriptors.Get()};
  state.di_transfer.di_list->SetDescriptorHeaps(1u, heaps);
  state.di_transfer.di_list->SetComputeRootSignature(state.di_transfer.nrd_probe_root.Get());
  state.di_transfer.di_list->SetComputeRootConstantBufferView(0u, current.nrd.probe_constants->GetGPUVirtualAddress());
  D3D12_GPU_DESCRIPTOR_HANDLE srv_table = current.di_descriptors->GetGPUDescriptorHandleForHeapStart();
  state.di_transfer.di_list->SetComputeRootDescriptorTable(1u, srv_table);
  D3D12_GPU_DESCRIPTOR_HANDLE uav_table = srv_table;
  uav_table.ptr += static_cast<UINT64>(36u) * state.descriptor_size;
  state.di_transfer.di_list->SetComputeRootDescriptorTable(2u, uav_table);
  state.di_transfer.di_list->SetPipelineState(state.di_transfer.nrd_probe_pso.Get());
  state.di_transfer.di_list->EndQuery(current.nrd.probe_queries.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 0u);
  state.di_transfer.di_list->Dispatch(4u, 3u, 1u);
  state.di_transfer.di_list->EndQuery(current.nrd.probe_queries.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 1u);
  D3D12_RESOURCE_BARRIER barriers[2] = {};
  barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
  barriers[0].UAV.pResource = current.nrd.probe_results.Get();
  barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  barriers[1].Transition.pResource = current.nrd.probe_results.Get();
  barriers[1].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
  barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
  state.di_transfer.di_list->ResourceBarrier(2u, barriers);
  state.di_transfer.di_list->CopyBufferRegion(current.nrd.probe_readback.Get(), 0u,
                                              current.nrd.probe_results.Get(), 0u, 32u * 18u * 2u * sizeof(uint32_t));
  barrier = {};
  barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  barrier.Transition.pResource = current.nrd.probe_results.Get();
  barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
  barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COMMON;
  state.di_transfer.di_list->ResourceBarrier(1u, &barrier);
  state.di_transfer.di_list->ResolveQueryData(current.nrd.probe_queries.Get(), D3D12_QUERY_TYPE_TIMESTAMP,
                                              0u, 2u, current.nrd.probe_timestamp_readback.Get(), 0u);
  return true;
}
#endif

inline Rt12DiInputResult TransferRtxdiInputs(reshade::api::device* device, reshade::api::command_list* cmd_list,
                                             const Rt12DiInputViews& in) {
  const bvh::PoolCpuTimer cpu_timer{&bvh::g_perf_gate.rtxdi_input_cpu_us};
  using DS = reshade::api::shader_stage;
  using DT = reshade::api::descriptor_type;
  using Microsoft::WRL::ComPtr;
  Rt12DiInputResult result;
  auto& diagnostics = rtxdi::GetDiagnosticState();
  const bool diagnostic_enabled = rtxdi::DiagnosticCaptureEnabled(diagnostics);
  const bool perf_gather = bvh::g_perf_gate.gather.load(std::memory_order_relaxed);
  const uint64_t perf_epoch = perf_gather ? bvh::g_perf_gate.gather_epoch.load(std::memory_order_relaxed) : 0u;
  if (perf_gather) bvh::g_perf_gate.rtxdi_attempts.fetch_add(1u, std::memory_order_relaxed);
  const uint64_t diagnostic_epoch = diagnostic_enabled ? rtxdi::DiagnosticCaptureEpoch(diagnostics) : 0u;
  rtxdi::DiagnosticReason forced_diagnostic_reason = rtxdi::DiagnosticReason::None;
  rtxdi::StructuredViewEvidence structured_evidence = {};
  const auto finish = [&]() {
    if (perf_gather && bvh::g_perf_gate.gather.load(std::memory_order_relaxed)
        && bvh::g_perf_gate.gather_epoch.load(std::memory_order_relaxed) == perf_epoch) {
      if (result.status == rtxdi::InputStatus::Busy) {
        bvh::g_perf_gate.rtxdi_busy_skips.fetch_add(1u, std::memory_order_relaxed);
      } else if (result.status != rtxdi::InputStatus::Off && result.status != rtxdi::InputStatus::AlreadyAttempted) {
        bvh::g_perf_gate.rtxdi_rejections.fetch_add(1u, std::memory_order_relaxed);
      }
    }
    if (diagnostic_enabled) {
      rtxdi::DiagnosticEvent event = {};
      event.frame = in.frame;
      event.capture_frame = in.capture_frame;
      event.owner_generation = g_rt12 && g_rt12->device == device ? g_rt12->generation : 0u;
      event.settings_identity = in.effective_settings.identity;
      event.effective_identity = in.effective_settings.identity;
      event.fence = result.fence;
      event.scene_slot = in.scene_slot;
      event.transfer_slot = result.transfer_slot;
      event.hresult = result.error;
      event.input_kind = result.validation_input;
      event.validation_code = result.validation_code;
      event.input_status = static_cast<uint16_t>(result.status);
      event.input_stage = static_cast<uint16_t>(result.stage);
      if ((structured_evidence.evidence_flags & 0x80u) != 0u) {
        event.identity_tokens[0] = rtxdi::DiagnosticIdentityToken(
            diagnostics, diagnostic_epoch, structured_evidence.resource_identity, event.owner_generation);
        event.normalized_size = uint64_t{structured_evidence.expected_stride}
                                | (uint64_t{structured_evidence.misc_flags} << 32u);
        switch (result.validation_input) {
          case rtxdi::DiagnosticInputKind::Lights:   event.required_rows = in.lights_register; break;
          case rtxdi::DiagnosticInputKind::Tiles:    event.required_rows = in.tiles_register; break;
          case rtxdi::DiagnosticInputKind::Deferred: event.required_rows = in.deferred_register; break;
          default:                                   break;
        }
        event.flags = structured_evidence.evidence_flags | (static_cast<uint32_t>(structured_evidence.failure) << 16u);
        event.native_offset_bytes = structured_evidence.view_dimension;
        event.native_count_bytes = structured_evidence.format;
        event.first_constant = structured_evidence.first_element;
        event.constant_count = structured_evidence.element_count;
        event.copied_rows = structured_evidence.bufferex_flags;
        event.native_bind_flags = structured_evidence.bind_flags;
        event.available_rows = structured_evidence.structure_stride;
        event.buffer_bytes = structured_evidence.buffer_bytes;
        event.value0 = structured_evidence.element_offset;
        event.value1 = structured_evidence.element_bytes;
      }
      switch (result.stage) {
        case rtxdi::InputStage::None:
        case rtxdi::InputStage::Validate: event.stage = rtxdi::DiagnosticStage::InputValidate; break;
        case rtxdi::InputStage::Allocate: event.stage = rtxdi::DiagnosticStage::InputAllocate; break;
        case rtxdi::InputStage::Pack:     event.stage = rtxdi::DiagnosticStage::InputRecord; break;
        case rtxdi::InputStage::SignalA:  event.stage = rtxdi::DiagnosticStage::InputSignalA; break;
        case rtxdi::InputStage::Record:
        case rtxdi::InputStage::Order:    event.stage = rtxdi::DiagnosticStage::InputRecord; break;
        case rtxdi::InputStage::Submit:   event.stage = rtxdi::DiagnosticStage::InputSubmit; break;
        case rtxdi::InputStage::WaitB:
        case rtxdi::InputStage::Ready:    event.stage = rtxdi::DiagnosticStage::InputWaitB; break;
      }
      switch (result.status) {
        case rtxdi::InputStatus::Off:
        case rtxdi::InputStatus::AlreadyAttempted:
        case rtxdi::InputStatus::Ready:               event.reason = rtxdi::DiagnosticReason::None; break;
        case rtxdi::InputStatus::WaitingForScene:     event.reason = rtxdi::DiagnosticReason::MissingInput; break;
        case rtxdi::InputStatus::MissingInput:        event.reason = rtxdi::DiagnosticReason::MissingInput; break;
        case rtxdi::InputStatus::SceneBindingChanged: event.reason = rtxdi::DiagnosticReason::None; break;
        case rtxdi::InputStatus::Busy:                event.reason = rtxdi::DiagnosticReason::Busy; break;
        case rtxdi::InputStatus::AllocationFailed:    event.reason = rtxdi::DiagnosticReason::ResourceCreateFailed; break;
        case rtxdi::InputStatus::SubmitFailed:
          event.reason = result.stage == rtxdi::InputStage::SignalA ? rtxdi::DiagnosticReason::SignalFailed
                                                                    : rtxdi::DiagnosticReason::SubmitFailed;
          break;
        case rtxdi::InputStatus::Quarantined: event.reason = rtxdi::DiagnosticReason::Quarantined; break;
      }
      if (forced_diagnostic_reason != rtxdi::DiagnosticReason::None) event.reason = forced_diagnostic_reason;
      switch (result.stage) {
        case rtxdi::InputStage::None:
        case rtxdi::InputStage::Validate: event.operation = rtxdi::DiagnosticOperation::Validate; break;
        case rtxdi::InputStage::Allocate: event.operation = rtxdi::DiagnosticOperation::Allocate; break;
        case rtxdi::InputStage::Pack:     event.operation = rtxdi::DiagnosticOperation::Pack; break;
        case rtxdi::InputStage::SignalA:  event.operation = rtxdi::DiagnosticOperation::SignalA; break;
        case rtxdi::InputStage::Record:
        case rtxdi::InputStage::Order:    event.operation = rtxdi::DiagnosticOperation::Execute; break;
        case rtxdi::InputStage::Submit:   event.operation = rtxdi::DiagnosticOperation::Execute; break;
        case rtxdi::InputStage::WaitB:
        case rtxdi::InputStage::Ready:    event.operation = rtxdi::DiagnosticOperation::WaitB; break;
      }
      rtxdi::RecordDiagnostic(diagnostics, diagnostic_epoch, event);
    }
    return result;
  };
  try {
    if (!g_rt12 || g_rt12->device != device || !cmd_list) {
      result.status = rtxdi::InputStatus::MissingInput;
      return finish();
    }
    Rt12State& s = *g_rt12;
    if (s.failed || s.stuck || s.unfenced_execution) {
      forced_diagnostic_reason = rtxdi::DiagnosticReason::Quarantined;
      return finish();
    }
    if (in.scene_slot < 0 || in.scene_slot >= static_cast<int32_t>(kRt12Slots)) {
      forced_diagnostic_reason = rtxdi::DiagnosticReason::InvalidResource;
      return finish();
    }
    if (!rtxdi::ValidSceneLease(s.scene_slot, s.scene_frame, in.scene_slot, in.frame, s.scene_fence, in.scene_fence)) {
      forced_diagnostic_reason = rtxdi::DiagnosticReason::StaleFrame;
      return finish();
    }
    if (in.sun && !rtxdi::SceneCoverageSnapshotMatches(s.scene_slot, s.scene_frame, s.scene_generation, s.scene_fence, s.scene_gc_version, s.scene_coverage_slot, s.scene_coverage_frame, s.scene_coverage_generation, s.scene_coverage_fence, s.scene_coverage_gc_version)) {
      forced_diagnostic_reason = rtxdi::DiagnosticReason::InvalidRange;
      return finish();
    }
    if (reinterpret_cast<ID3D11DeviceContext*>(cmd_list->get_native()) != s.ctx11.Get()) {  // NOLINT(performance-no-int-to-ptr)
      forced_diagnostic_reason = rtxdi::DiagnosticReason::CommandMismatch;
      return finish();
    }
#if defined(FALCOM_NRD_ENABLED)
    ProcessNrdProbeReadbacks(s);
#endif
    auto& transfer = s.di_transfer;
    if (rtxdi::TransferAttemptDuplicate(transfer.attempted_frame, transfer.attempted_generation, in.frame, s.generation)) {
      result.status = rtxdi::InputStatus::AlreadyAttempted;
      return finish();
    }
    transfer.attempted_frame = in.frame;
    transfer.attempted_generation = s.generation;
    transfer.input_frame = UINT32_MAX;
    if (!in.width || !in.height) {
      forced_diagnostic_reason = rtxdi::DiagnosticReason::InvalidRange;
      return finish();
    }
    ComPtr<ID3D11Buffer> current_scene;
    s.ctx11->PSGetConstantBuffers(0u, 1u, &current_scene);
    reshade::api::buffer_range scene_range = {};
    rtxdi::SceneRangeEvidence scene_evidence = {};
    DiSceneNativeBindingEvidence native_evidence = {};
    const bool scene_range_valid = CurrentDiSceneRange(device, cmd_list, in, current_scene.Get(), &scene_range,
                                                       &scene_evidence);
    rtxdi::SceneCopyWindow scene_copy_window = {};
    const bool native_range_valid = ValidateBoundDiScene(s.ctx11.Get(), scene_range, &scene_evidence,
                                                         &native_evidence, &scene_copy_window);
    scene_evidence.binding.available_rows = scene_copy_window.available_rows;
    scene_evidence.binding.copied_rows = scene_copy_window.copied_rows;
    if (!scene_range_valid || !native_range_valid) {
      result.status = rtxdi::InputStatus::SceneBindingChanged;
      if (diagnostic_enabled) {
        RecordDiSceneEvidence(diagnostics, diagnostic_epoch, rtxdi::DiagnosticStage::InputValidate,
                              s.generation, in.frame, in.scene_slot, -1, scene_evidence, native_evidence);
      }
      return finish();
    }
    if (diagnostic_enabled) {
      RecordDiSceneEvidence(diagnostics, diagnostic_epoch, rtxdi::DiagnosticStage::InputValidate,
                            s.generation, in.frame, in.scene_slot, -1, scene_evidence, native_evidence, true);
    }
    if (!ValidateDiTexture(in.depth, in.width, in.height, Rt12DiTextureKind::Float)) {
      result.validation_input = rtxdi::DiagnosticInputKind::Depth;
      result.validation_code = rtxdi::DiagnosticValidationCode::InvalidTextureView;
      forced_diagnostic_reason = rtxdi::DiagnosticReason::InvalidResource;
      return finish();
    }
    if (!ValidateDiTexture(in.normal, in.width, in.height, Rt12DiTextureKind::Uint4)) {
      result.validation_input = rtxdi::DiagnosticInputKind::Normal;
      result.validation_code = rtxdi::DiagnosticValidationCode::InvalidTextureView;
      forced_diagnostic_reason = rtxdi::DiagnosticReason::InvalidResource;
      return finish();
    }
    const bool motion_valid = in.motion.handle != 0u
                              && ValidateDiTexture(in.motion, in.width, in.height, Rt12DiTextureKind::Motion);
    const bool motion_available = motion_valid;
    if (in.motion.handle && !motion_valid) {
      result.validation_input = rtxdi::DiagnosticInputKind::Motion;
      result.validation_code = rtxdi::DiagnosticValidationCode::OptionalFallback;
    }
    if (in.local && !ValidateDiTexture(in.color, in.width, in.height, Rt12DiTextureKind::Float)) {
      result.validation_input = rtxdi::DiagnosticInputKind::Color;
      result.validation_code = rtxdi::DiagnosticValidationCode::InvalidTextureView;
      forced_diagnostic_reason = rtxdi::DiagnosticReason::InvalidResource;
      return finish();
    }
    if (in.local && !ValidateDiTexture(in.mrt1, in.width, in.height, Rt12DiTextureKind::Uint4)) {
      result.validation_input = rtxdi::DiagnosticInputKind::Mrt1;
      result.validation_code = rtxdi::DiagnosticValidationCode::InvalidTextureView;
      forced_diagnostic_reason = rtxdi::DiagnosticReason::InvalidResource;
      return finish();
    }
    if (in.local && !ValidateDiTexture(in.mrt2, in.width, in.height, Rt12DiTextureKind::Uint2)) {
      result.validation_input = rtxdi::DiagnosticInputKind::Mrt2;
      result.validation_code = rtxdi::DiagnosticValidationCode::InvalidTextureView;
      forced_diagnostic_reason = rtxdi::DiagnosticReason::InvalidResource;
      return finish();
    }
    rtxdi::StructuredInputRange lights = {}, tiles = {}, deferred = {};
    if (in.local && in.lights.handle
        && !rtxdi::ValidateStructuredView(reinterpret_cast<ID3D11ShaderResourceView*>(in.lights.handle),
                                          RTXDI_INPUT_LIGHT_STRIDE, &lights,
                                          diagnostic_enabled ? &structured_evidence : nullptr)) {  // NOLINT(performance-no-int-to-ptr)
      result.validation_input = rtxdi::DiagnosticInputKind::Lights;
      result.validation_code = rtxdi::DiagnosticValidationCode::InvalidStructuredView;
      forced_diagnostic_reason = rtxdi::DiagnosticReason::InvalidResource;
      return finish();
    }
    if (in.local
        && !rtxdi::ValidateStructuredView(reinterpret_cast<ID3D11ShaderResourceView*>(in.tiles.handle),
                                          RTXDI_INPUT_TILE_STRIDE, &tiles,
                                          diagnostic_enabled ? &structured_evidence : nullptr)) {  // NOLINT(performance-no-int-to-ptr)
      result.validation_input = rtxdi::DiagnosticInputKind::Tiles;
      result.validation_code = rtxdi::DiagnosticValidationCode::InvalidStructuredView;
      forced_diagnostic_reason = rtxdi::DiagnosticReason::InvalidResource;
      return finish();
    }
    if (in.local
        && !rtxdi::ValidateStructuredView(reinterpret_cast<ID3D11ShaderResourceView*>(in.deferred.handle),
                                          RTXDI_INPUT_DEFERRED_STRIDE, &deferred,
                                          diagnostic_enabled ? &structured_evidence : nullptr)) {  // NOLINT(performance-no-int-to-ptr)
      result.validation_input = rtxdi::DiagnosticInputKind::Deferred;
      result.validation_code = rtxdi::DiagnosticValidationCode::InvalidStructuredView;
      forced_diagnostic_reason = rtxdi::DiagnosticReason::InvalidResource;
      return finish();
    }
    structured_evidence = {};
    if (!rtxdi::MakeInputLayout(in.width, in.height, motion_available, in.local, lights.element_count,
                                tiles.element_count, deferred.element_count, &result.layout)) {
      result.validation_input = rtxdi::DiagnosticInputKind::Layout;
      result.validation_code = rtxdi::DiagnosticValidationCode::LayoutRejected;
      forced_diagnostic_reason = rtxdi::DiagnosticReason::InvalidRange;
      return finish();
    }
    const uint32_t k = transfer.next_slot;
    Rt12DiTransferSlot& slot = transfer.slots[k];
    if (!rtxdi::TransferSlotAvailable((std::max)(slot.fence_value, slot.di_fence_value), Rt12CompletedValue(s))) {
      result.status = rtxdi::InputStatus::Busy;
      return finish();
    }
    const uint64_t consumer_completed = (s.fence11 ? s.fence11->GetCompletedValue() : 0u);
    if (!rtxdi::CanReuseDiOutput(slot.output_staged
#if defined(FALCOM_NRD_ENABLED)
                                     || slot.nrd_diffuse_staged
#endif
                                 ,
                                 slot.output_c_fence, consumer_completed)) {
      result.status = rtxdi::InputStatus::Busy;
      return finish();
    }
    bool perf_timing = false;
    if (perf_gather && slot.perf_pending) {
      rtxdi::RecordGpuTimingSkipEvent(&bvh::g_perf_gate.rtxdi_timing_skip_events);
    }
    if (diagnostic_enabled && (slot.output_staged
#if defined(FALCOM_NRD_ENABLED)
                               || slot.nrd_diffuse_staged
#endif
                               )) {
      rtxdi::RecordDiagnostic(diagnostics, diagnostic_epoch,
                              {.owner_generation = s.generation,
                               .frame = in.frame,
                               .capture_frame = slot.output_frame,
                               .fence = slot.output_c_fence,
                               .value0 = slot.output_b_fence,
                               .scene_slot = slot.output_scene_slot,
                               .transfer_slot = static_cast<int32_t>(k),
                               .stage = rtxdi::DiagnosticStage::SlotConsumer,
                               .reason = rtxdi::DiagnosticReason::None,
                               .operation = rtxdi::DiagnosticOperation::Retire});
    }
    slot.output_staged = false;
    slot.sun_detail_identity = {};
#if defined(FALCOM_NRD_ENABLED)
    slot.debug_distance_identity = {};
#endif
#if defined(FALCOM_NRD_ENABLED)
    slot.nrd_diffuse_staged = false;
    slot.nrd_diffuse_identity = {};
    slot.nrd.diffuse_publication_recorded = false;
#endif
    slot.n1_staged = false;
    slot.n1_configuration_version = RTXDI_N1_CONFIG_VERSION;
    slot.n1_brdf_config = in.n1_brdf_config;
    slot.output_b_fence = 0u;
    slot.output_c_fence = 0u;
    result.stage = rtxdi::InputStage::Allocate;
    if (transfer.owner_generation != 0u && transfer.owner_generation != s.generation) {
      s.failed = true;
      s.stuck = true;
      result.status = rtxdi::InputStatus::Quarantined;
      result.error = E_FAIL;
      UpdatePanel([](Rt12Panel& p) { p.stuck = true; });
      return finish();
    }
    if (in.local && !EnsureRt12DiResources(s, true, in.width, in.height, in.effective_settings)) {
      result.status = rtxdi::InputStatus::AllocationFailed;
      result.error = E_FAIL;
      return finish();
    }
    // Each slot is replaced only after its B fence completes.
    if (!slot.allocator || slot.width != in.width || slot.height != in.height || slot.local != in.local || slot.sun != in.sun
        || slot.motion_available != motion_available) {
      if (transfer.configuration_generation == UINT64_MAX) {
        result.status = rtxdi::InputStatus::AllocationFailed;
        return finish();
      }
      Rt12DiTransferSlot candidate;
      auto destroy_candidate = [&](Rt12DiTransferSlot*) {
        for (Rt12Shared* shared : {
                 &candidate.depth,
                 &candidate.normal,
                 &candidate.scene,
                 &candidate.color,
                 &candidate.mrt1,
                 &candidate.mrt2,
                 &candidate.motion,
                 &candidate.lights,
                 &candidate.tiles,
                 &candidate.deferred,
                 &candidate.di_resolved,
                 &candidate.sun_visibility,
                 &candidate.sun_detail,
                 &candidate.n1_diffuse,
                 &candidate.n1_specular,
                 &candidate.n1_status,
#if defined(FALCOM_NRD_ENABLED)
                 &candidate.nrd_diffuse_publication,
#endif
             }) {
          if (shared->srv11.handle) {
            device->destroy_resource_view(shared->srv11);
          }
          if (shared->uav11.handle) {
            device->destroy_resource_view(shared->uav11);
          }
        }
        rtgi::DestroyRtgiPipeline(device, candidate.pack_inputs);
        rtgi::DestroyRtgiPipeline(device, candidate.pack_records);
      };
      std::unique_ptr<Rt12DiTransferSlot, decltype(destroy_candidate)> candidate_guard(&candidate, destroy_candidate);
      HRESULT allocation_error = S_OK;
      const auto make_shared = [&](Rt12Shared* shared, DXGI_FORMAT format, uint32_t width, uint32_t height) {
        allocation_error = CreateRt12Shared(s, device, format, width, height, 1u, true, shared);
        return SUCCEEDED(allocation_error);
      };
      bool ok = make_shared(&candidate.depth, DXGI_FORMAT_R32_FLOAT, in.width, in.height)
                && make_shared(&candidate.normal, DXGI_FORMAT_R32G32B32A32_UINT, in.width, in.height)
                && make_shared(&candidate.scene, DXGI_FORMAT_R32G32B32A32_UINT, 96u, 1u);
      if (ok && in.local) {
        ok = make_shared(&candidate.color, DXGI_FORMAT_R32G32B32A32_FLOAT, in.width, in.height)
             && make_shared(&candidate.mrt1, DXGI_FORMAT_R32G32B32A32_UINT, in.width, in.height)
             // Shared RG32_UINT failed with E_INVALIDARG on the tested RTX 4070; RGBA32_UINT succeeds.
             && make_shared(&candidate.mrt2, DXGI_FORMAT_R32G32B32A32_UINT, in.width, in.height)
             && make_shared(&candidate.lights, DXGI_FORMAT_R32G32B32A32_UINT, 4096u, RTXDI_INPUT_LIGHT_ROWS)
             && make_shared(&candidate.tiles, DXGI_FORMAT_R32G32B32A32_UINT, 4096u, RTXDI_INPUT_TILE_ROWS)
             && make_shared(&candidate.deferred, DXGI_FORMAT_R32G32B32A32_UINT, 4096u, RTXDI_INPUT_DEFERRED_ROWS);
        if (ok) ok = make_shared(&candidate.di_resolved, DXGI_FORMAT_R32G32B32A32_UINT, in.width, in.height);
        if (ok) {
          // Private N1 reference outputs; they are not bound by the game lighting path.
          const HRESULT n1_diffuse_hr = CreateRt12Shared(s, device, DXGI_FORMAT_R16G16B16A16_FLOAT,
                                                         in.width, in.height, 1u, true, &candidate.n1_diffuse);
          const HRESULT n1_specular_hr = SUCCEEDED(n1_diffuse_hr)
                                             ? CreateRt12Shared(s, device, DXGI_FORMAT_R16G16B16A16_FLOAT,
                                                                in.width, in.height, 1u, true, &candidate.n1_specular)
                                             : n1_diffuse_hr;
          if (SUCCEEDED(n1_specular_hr)) {
            const HRESULT n1_status_hr = CreateRt12Shared(s, device, DXGI_FORMAT_R32_UINT,
                                                          in.width, in.height, 1u, true, &candidate.n1_status);
            candidate.n1_resources_ready = SUCCEEDED(n1_status_hr);
          }
        }
      }
      if (ok && motion_available) ok = make_shared(&candidate.motion, DXGI_FORMAT_R32G32B32A32_FLOAT, in.width, in.height);
      if (ok && in.sun) ok = make_shared(&candidate.sun_visibility, DXGI_FORMAT_R32_UINT, in.width, in.height);
#if defined(FALCOM_NRD_ENABLED)
      if (ok && in.local) {
        CreateRt12Shared(s, device, DXGI_FORMAT_R16G16B16A16_FLOAT, in.width, in.height, 1u, true,
                         &candidate.nrd_diffuse_publication);
      }
#endif
      const auto make_buffer = [&](uint64_t bytes, ComPtr<ID3D12Resource>* buffer) {
        allocation_error = CreateRt12Buffer(s.device12.Get(), D3D12_HEAP_TYPE_DEFAULT, bytes, D3D12_RESOURCE_STATE_COMMON,
                                            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, *buffer);
        return SUCCEEDED(allocation_error);
      };
      if (ok) {
        ok = make_buffer(1536u, &candidate.scene_buffer);
      }
      if (ok && in.local) {
        ok = make_buffer(uint64_t{RTXDI_INPUT_MAX_LIGHTS} * RTXDI_INPUT_LIGHT_STRIDE, &candidate.lights_buffer)
             && make_buffer(uint64_t{RTXDI_INPUT_MAX_TILES} * RTXDI_INPUT_TILE_STRIDE, &candidate.tiles_buffer)
             && make_buffer(uint64_t{RTXDI_INPUT_MAX_DEFERRED} * RTXDI_INPUT_DEFERRED_STRIDE, &candidate.deferred_buffer);
      }
      if (ok && in.local) {
        const uint64_t pixels = uint64_t{in.width} * in.height;
        ok = pixels <= UINT64_MAX / sizeof(RtxdiDiRawResult)
             && make_buffer(uint64_t{RTXDI_INPUT_MAX_LIGHTS} * sizeof(uint32_t), &candidate.light_types)
             && make_buffer(pixels * sizeof(RtxdiDiRawResult), &candidate.di_results);
        if (ok) {
          allocation_error = CreateRt12Buffer(s.device12.Get(), D3D12_HEAP_TYPE_UPLOAD,
                                              kRtxdiFrameCbvBytes, D3D12_RESOURCE_STATE_GENERIC_READ,
                                              D3D12_RESOURCE_FLAG_NONE, candidate.di_frame_upload);
          ok = SUCCEEDED(allocation_error);
        }
        if (ok) {
          allocation_error = s.device12->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                                                IID_PPV_ARGS(&candidate.di_allocator));
          ok = SUCCEEDED(allocation_error);
        }
      }
      if (ok) {
        allocation_error = s.device12->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&candidate.allocator));
        ok = SUCCEEDED(allocation_error);
      }
      D3D12_DESCRIPTOR_HEAP_DESC heap = {};
      heap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
      heap.NumDescriptors = 40u;
      heap.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
      if (ok) {
        allocation_error = s.device12->CreateDescriptorHeap(&heap, IID_PPV_ARGS(&candidate.descriptors));
        ok = SUCCEEDED(allocation_error);
      }
      if (ok && in.local) {
        allocation_error = s.device12->CreateDescriptorHeap(&heap, IID_PPV_ARGS(&candidate.di_descriptors));
        ok = SUCCEEDED(allocation_error);
      }
#if defined(__world_rtxdi_pack_inputs_EMBED_FILE) && defined(__world_rtxdi_pack_records_EMBED_FILE)
      if (ok) {
        ok = rtgi::EnsureRtgiPipeline(device, &candidate.pack_inputs, {.srv_count = 6u, .uav_count = 7u, .sampler_count = 0u, .push_count = 5u, .code = __world_rtxdi_pack_inputs, .name = "rtxdi input pack"});
      }
      if (ok && in.local) {
        ok = rtgi::EnsureRtgiPipeline(device, &candidate.pack_records, {.srv_count = 3u, .uav_count = 3u, .sampler_count = 0u, .push_count = 4u, .code = __world_rtxdi_pack_records, .name = "rtxdi record pack"});
      }
#else
      ok = false;
#endif
      if (!ok) {
        result.status = rtxdi::InputStatus::AllocationFailed;
        result.error = FAILED(allocation_error) ? allocation_error : E_FAIL;
        return finish();
      }
      for (Rt12Shared* shared : {
               &slot.depth,
               &slot.normal,
               &slot.scene,
               &slot.color,
               &slot.mrt1,
               &slot.mrt2,
               &slot.motion,
               &slot.lights,
               &slot.tiles,
               &slot.deferred,
               &slot.di_resolved,
               &slot.sun_visibility,
               &slot.sun_detail,
               &slot.n1_diffuse,
               &slot.n1_specular,
               &slot.n1_status,
#if defined(FALCOM_NRD_ENABLED)
               &slot.nrd_diffuse_publication,
#endif
           }) {
        if (shared->srv11.handle) {
          device->destroy_resource_view(shared->srv11);
        }
        if (shared->uav11.handle) {
          device->destroy_resource_view(shared->uav11);
        }
      }
      rtgi::DestroyRtgiPipeline(device, slot.pack_inputs);
      rtgi::DestroyRtgiPipeline(device, slot.pack_records);
      candidate.width = in.width;
      candidate.height = in.height;
      candidate.local = in.local;
      candidate.sun = in.sun;
      candidate.motion_available = motion_available;
      candidate.n1_configuration_version = 1u;
      candidate.n1_brdf_config = in.n1_brdf_config;
      candidate.tile_count = 0u;
      candidate.output_staged = false;
      slot = std::move(candidate);
      candidate_guard.release();
      transfer.owner_generation = s.generation;
      ++transfer.configuration_generation;
    }
    if (in.sun && !slot.sun_results) {
      uint64_t output_bytes = 0u;
      if (!rtxdi::CheckedSunResultBytes(in.width, in.height, &output_bytes)) {
        result.status = rtxdi::InputStatus::AllocationFailed;
        result.error = E_INVALIDARG;
        return finish();
      }
      const HRESULT output_hr = CreateRt12Buffer(s.device12.Get(), D3D12_HEAP_TYPE_DEFAULT,
                                                 output_bytes, D3D12_RESOURCE_STATE_COMMON,
                                                 D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, slot.sun_results);
      if (FAILED(output_hr)) {
        result.status = rtxdi::InputStatus::AllocationFailed;
        result.error = output_hr;
        return finish();
      }
    }
    const bool sun_detail_requested = in.sun
                                      && RtxdiSunDetailExportRequested(in.effective_debug_view, in.sun_detail_view);
    if (sun_detail_requested && (!slot.sun_detail.texture12 || slot.sun_detail.width != in.width
                                 || slot.sun_detail.height != in.height)) {
      Rt12Shared detail_candidate;
      if (SUCCEEDED(CreateRt12Shared(s, device, DXGI_FORMAT_R32G32B32A32_UINT, in.width, in.height, 1u, true,
                                     &detail_candidate))) {
        RetireShared(s, slot.sun_detail, slot.output_b_fence);
        slot.sun_detail = std::move(detail_candidate);
      }
    }
    if (rtxdi::CanStartGpuTiming(perf_gather
                                     && perf_epoch == bvh::g_perf_gate.gather_epoch.load(std::memory_order_relaxed),
                                 s.timestamp_frequency, slot.perf_pending)) {
      if (!slot.perf_queries || !slot.perf_readback) {
        D3D12_QUERY_HEAP_DESC query_desc = {};
        query_desc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
        query_desc.Count = 8u;
        D3D12_HEAP_PROPERTIES readback_heap = {};
        readback_heap.Type = D3D12_HEAP_TYPE_READBACK;
        D3D12_RESOURCE_DESC readback_desc = {};
        readback_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        readback_desc.Width = 64u;
        readback_desc.Height = 1u;
        readback_desc.DepthOrArraySize = 1u;
        readback_desc.MipLevels = 1u;
        readback_desc.SampleDesc.Count = 1u;
        readback_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        const HRESULT query_hr = s.device12->CreateQueryHeap(&query_desc, IID_PPV_ARGS(&slot.perf_queries));
        if (SUCCEEDED(query_hr)) {
          const HRESULT readback_hr = s.device12->CreateCommittedResource(
              &readback_heap, D3D12_HEAP_FLAG_NONE, &readback_desc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
              IID_PPV_ARGS(&slot.perf_readback));
          if (FAILED(readback_hr)) slot.perf_queries.Reset();
        }
      }
      perf_timing = slot.perf_queries && slot.perf_readback;
      if (perf_timing) {
        slot.perf_gather_epoch = perf_epoch;
        slot.perf_owner_generation = s.generation;
        slot.perf_transfer_configuration = transfer.configuration_generation;
        slot.perf_di_configuration = s.di.configuration_generation;
        slot.perf_settings_identity = in.effective_settings.identity;
        slot.perf_frame = in.frame;
        slot.perf_width = in.width;
        slot.perf_height = in.height;
        slot.perf_valid_mask = 0u;
        slot.perf_fence = 0u;
      } else {
        rtxdi::RecordGpuTimingSkipEvent(&bvh::g_perf_gate.rtxdi_timing_skip_events);
      }
    }
    if (perf_gather && s.timestamp_frequency == 0u) {
      rtxdi::RecordGpuTimingSkipEvent(&bvh::g_perf_gate.rtxdi_timing_skip_events);
    }
    if (!transfer.unpack_root || !transfer.unpack_pso) {
      D3D12_DESCRIPTOR_RANGE ranges[2] = {};
      ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
      ranges[0].NumDescriptors = 4u;
      ranges[0].OffsetInDescriptorsFromTableStart = 0u;
      ranges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
      ranges[1].NumDescriptors = 4u;
      ranges[1].OffsetInDescriptorsFromTableStart = 4u;
      D3D12_ROOT_PARAMETER params[2] = {};
      params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
      params[0].Constants.Num32BitValues = 4u;
      params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
      params[1].DescriptorTable.NumDescriptorRanges = 2u;
      params[1].DescriptorTable.pDescriptorRanges = ranges;
      D3D12_ROOT_SIGNATURE_DESC root_desc = {};
      root_desc.NumParameters = 2u;
      root_desc.pParameters = params;
      ComPtr<ID3DBlob> blob, error;
      ComPtr<ID3D12RootSignature> root;
      ComPtr<ID3D12PipelineState> pso;
      HRESULT hr = s.serialize(&root_desc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &error);
      if (SUCCEEDED(hr)) {
        hr = s.device12->CreateRootSignature(0u, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&root));
      }
#if defined(__world_rtxdi_unpack_records_EMBED_FILE)
      if (SUCCEEDED(hr)) {
        D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};
        desc.pRootSignature = root.Get();
        desc.CS = {__world_rtxdi_unpack_records.data(), __world_rtxdi_unpack_records.size()};
        hr = s.device12->CreateComputePipelineState(&desc, IID_PPV_ARGS(&pso));
      }
#else
      hr = E_FAIL;
#endif
      if (FAILED(hr)) {
        result.status = rtxdi::InputStatus::AllocationFailed;
        result.error = hr;
        return finish();
      }
      transfer.unpack_root = std::move(root);
      transfer.unpack_pso = std::move(pso);
    }
    if (in.sun && (!transfer.sun_root || !transfer.sun_pso)) {
      D3D12_DESCRIPTOR_RANGE ranges[8] = {};
      ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
      ranges[0].NumDescriptors = 4u;
      ranges[0].BaseShaderRegister = 0u;
      ranges[0].OffsetInDescriptorsFromTableStart = 0u;
      ranges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
      ranges[1].NumDescriptors = 3u;
      ranges[1].BaseShaderRegister = 14u;
      ranges[1].OffsetInDescriptorsFromTableStart = 4u;
      ranges[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
      ranges[2].NumDescriptors = 1u;
      ranges[2].BaseShaderRegister = 29u;
      ranges[2].OffsetInDescriptorsFromTableStart = 7u;
      ranges[3].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
      ranges[3].NumDescriptors = 1u;
      ranges[3].BaseShaderRegister = 30u;
      ranges[3].OffsetInDescriptorsFromTableStart = 8u;
      ranges[4].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
      ranges[4].NumDescriptors = 3u;
      ranges[4].BaseShaderRegister = 31u;
      ranges[4].OffsetInDescriptorsFromTableStart = 9u;
      ranges[5].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
      ranges[5].NumDescriptors = 1u;
      ranges[5].OffsetInDescriptorsFromTableStart = 12u;
      ranges[6].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
      ranges[6].NumDescriptors = 1u;
      ranges[6].BaseShaderRegister = 1u;
      ranges[6].OffsetInDescriptorsFromTableStart = 13u;
      ranges[7].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
      ranges[7].NumDescriptors = 1u;
      ranges[7].BaseShaderRegister = 61u;
      ranges[7].OffsetInDescriptorsFromTableStart = 16u;
      D3D12_ROOT_PARAMETER params[2] = {};
      params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
      params[0].Constants.Num32BitValues = 24u;
      params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
      params[1].DescriptorTable.NumDescriptorRanges = 8u;
      params[1].DescriptorTable.pDescriptorRanges = ranges;
      D3D12_ROOT_SIGNATURE_DESC root_desc = {};
      root_desc.NumParameters = 2u;
      root_desc.pParameters = params;
      ComPtr<ID3DBlob> blob, error;
      ComPtr<ID3D12RootSignature> root;
      ComPtr<ID3D12PipelineState> pso;
      HRESULT hr = s.serialize(&root_desc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &error);
      if (SUCCEEDED(hr)) {
        hr = s.device12->CreateRootSignature(0u, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&root));
      }
#if defined(__world_rtxdi_sun_visibility_EMBED_FILE)
      if (SUCCEEDED(hr)) {
        D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};
        desc.pRootSignature = root.Get();
        desc.CS = {__world_rtxdi_sun_visibility.data(), __world_rtxdi_sun_visibility.size()};
        hr = s.device12->CreateComputePipelineState(&desc, IID_PPV_ARGS(&pso));
      }
#else
      hr = E_FAIL;
#endif
      if (FAILED(hr)) {
        result.status = rtxdi::InputStatus::AllocationFailed;
        result.error = hr;
        return finish();
      }
      transfer.sun_root = std::move(root);
      transfer.sun_pso = std::move(pso);
    }
    if (!transfer.list) {
      ComPtr<ID3D12GraphicsCommandList4> list;
      HRESULT hr = s.device12->CreateCommandList(0u, D3D12_COMMAND_LIST_TYPE_DIRECT, slot.allocator.Get(), nullptr, IID_PPV_ARGS(&list));
      if (SUCCEEDED(hr)) {
        hr = list->Close();
      }
      if (FAILED(hr)) {
        result.status = rtxdi::InputStatus::AllocationFailed;
        result.error = hr;
        return finish();
      }
      transfer.list = std::move(list);
    }
    if (slot.fence_value != 0u) {
      try {
        if (!renodx::utils::gpu_fence::QueueWait(s.ctx11.Get(), s.fence12.get(), slot.fence_value)) {
          result.status = rtxdi::InputStatus::SubmitFailed;
          result.error = E_FAIL;
          return finish();
        }
      } catch (...) {
        result.status = rtxdi::InputStatus::SubmitFailed;
        result.error = E_FAIL;
        return finish();
      }
    }
    rtxdi::SceneRangeEvidence allocation_scene_evidence = {};
    DiSceneNativeBindingEvidence allocation_native_evidence = {};
    const bool allocation_scene_valid = CurrentDiSceneRange(
        device, cmd_list, in, current_scene.Get(), &scene_range, &allocation_scene_evidence);
    rtxdi::SceneCopyWindow allocation_copy_window = {};
    const bool allocation_native_valid = ValidateBoundDiScene(s.ctx11.Get(), scene_range, &allocation_scene_evidence,
                                                              &allocation_native_evidence, &allocation_copy_window);
    allocation_scene_evidence.binding.available_rows = allocation_copy_window.available_rows;
    allocation_scene_evidence.binding.copied_rows = allocation_copy_window.copied_rows;
    if (!allocation_scene_valid || !allocation_native_valid) {
      result.status = rtxdi::InputStatus::SceneBindingChanged;
      if (diagnostic_enabled) {
        RecordDiSceneEvidence(diagnostics, diagnostic_epoch, rtxdi::DiagnosticStage::InputAllocate,
                              s.generation, in.frame, in.scene_slot, static_cast<int32_t>(k),
                              allocation_scene_evidence, allocation_native_evidence);
      }
      return finish();
    }
    if (diagnostic_enabled) {
      RecordDiSceneEvidence(diagnostics, diagnostic_epoch, rtxdi::DiagnosticStage::InputAllocate,
                            s.generation, in.frame, in.scene_slot, static_cast<int32_t>(k),
                            allocation_scene_evidence, allocation_native_evidence, true);
    }
#if defined(FALCOM_NRD_ENABLED)
    CaptureNrdCameraRows(reinterpret_cast<uintptr_t>(device), in, scene_range, allocation_copy_window.copied_rows,
                         s.generation, &slot.nrd.camera);
#endif
    const reshade::api::resource_view retained[9] = {
        in.depth, in.normal, in.local ? in.color : reshade::api::resource_view{},
        in.local ? in.mrt1 : reshade::api::resource_view{}, in.local ? in.mrt2 : reshade::api::resource_view{},
        motion_available ? in.motion : reshade::api::resource_view{},
        in.local ? in.lights : reshade::api::resource_view{},
        in.local ? in.tiles : reshade::api::resource_view{}, in.local ? in.deferred : reshade::api::resource_view{}};
    for (size_t i = 0u; i < slot.source_views.size(); ++i) {
      slot.source_views[i] = reinterpret_cast<ID3D11ShaderResourceView*>(retained[i].handle);  // NOLINT(performance-no-int-to-ptr)
    }
    slot.source_scene = current_scene;
    current_scene.Reset();
    const auto clear_tables = [&](const rtgi::RtgiPipeline& pipeline, uint32_t srv_count, uint32_t uav_count) {
      const reshade::api::buffer_range null_cb = {};
      const reshade::api::resource_view null_views[7] = {};
      const reshade::api::descriptor_table_update updates[3] = {
          {pipeline.cbv_table, 0, 0, 1u, DT::constant_buffer, &null_cb},
          {pipeline.srv_table, 0, 0, srv_count, DT::shader_resource_view, null_views},
          {pipeline.uav_table, 0, 0, uav_count, DT::unordered_access_view, null_views},
      };
      device->update_descriptor_tables(3u, updates);
    };
    const auto bind = [&](const rtgi::RtgiPipeline& pipeline, uint32_t srv_count, const reshade::api::resource_view* srvs,
                          uint32_t uav_count, const reshade::api::resource_view* uavs) {
      const reshade::api::descriptor_table_update updates[3] = {
          {pipeline.cbv_table, 0, 0, 1, DT::constant_buffer, &scene_range},
          {pipeline.srv_table, 0, 0, srv_count, DT::shader_resource_view, srvs},
          {pipeline.uav_table, 0, 0, uav_count, DT::unordered_access_view, uavs},
      };
      device->update_descriptor_tables(3u, updates);
      cmd_list->bind_pipeline(reshade::api::pipeline_stage::all_compute, pipeline.pipeline);
      const reshade::api::descriptor_table tables[3] = {pipeline.cbv_table, pipeline.srv_table, pipeline.uav_table};
      cmd_list->bind_descriptor_tables(DS::all_compute, pipeline.layout, 0u, 3u, tables);
    };
    const reshade::api::resource_view pixels[6] = {retained[0], retained[1], retained[2], retained[3], retained[4], retained[5]};
    const reshade::api::resource_view packed[7] = {slot.depth.uav11, slot.normal.uav11, slot.color.uav11, slot.mrt1.uav11, slot.mrt2.uav11, slot.scene.uav11, slot.motion.uav11};
    bind(slot.pack_inputs, 6u, pixels, 7u, packed);
    const uint32_t pixel_params[5] = {in.width, in.height, in.local ? 1u : 0u, motion_available ? 1u : 0u,
                                      allocation_copy_window.copied_rows};
    cmd_list->push_constants(DS::all_compute, slot.pack_inputs.layout, 3u, 0u, 5u, pixel_params);
    cmd_list->dispatch((in.width + 7u) / 8u, (in.height + 7u) / 8u, 1u);
    rtao::NullComputeSlots(cmd_list, 6u, 7u, 0u);
    clear_tables(slot.pack_inputs, 6u, 7u);
    if (in.local) {
      const reshade::api::resource_view records[3] = {in.lights, in.tiles, in.deferred};
      const reshade::api::resource_view outputs[3] = {slot.lights.uav11, slot.tiles.uav11, slot.deferred.uav11};
      bind(slot.pack_records, 3u, records, 3u, outputs);
      const uint32_t counts[4] = {result.layout.lights_copied, result.layout.tiles_copied, result.layout.deferred_copied, 0u};
      cmd_list->push_constants(DS::all_compute, slot.pack_records.layout, 3u, 0u, 4u, counts);
      const uint32_t texels = (std::max)({counts[0] * 7u, counts[1] * 36u, counts[2] * 9u});
      if (texels) {
        cmd_list->dispatch((texels + 63u) / 64u, 1u, 1u);
      }
      rtao::NullComputeSlots(cmd_list, 3u, 3u, 0u);
      clear_tables(slot.pack_records, 3u, 3u);
    }
    result.stage = rtxdi::InputStage::SignalA;
    uint64_t a = renodx::utils::gpu_fence::NO_SIGNAL;
    try {
      a = renodx::utils::gpu_fence::QueueSignal(s.ctx11.Get(), s.fence11.get());
      s.ctx11->Flush();
    } catch (...) {
      a = renodx::utils::gpu_fence::NO_SIGNAL;
    }
    if (a == renodx::utils::gpu_fence::NO_SIGNAL) {
      transfer.input_frame = UINT32_MAX;
      FailSession(s, Rt12Reason::SubmitFailed, "rtxdi D3D11 signal A");
      result.status = rtxdi::InputStatus::SubmitFailed;
      result.error = E_FAIL;
      return finish();
    }
    rtxdi::SceneRangeEvidence signal_scene_evidence = {};
    DiSceneNativeBindingEvidence signal_native_evidence = {};
    const bool signal_scene_valid = CurrentDiSceneRange(
        device, cmd_list, in, slot.source_scene.Get(), &scene_range, &signal_scene_evidence);
    rtxdi::SceneCopyWindow signal_copy_window = {};
    const bool signal_native_valid = ValidateBoundDiScene(s.ctx11.Get(), scene_range, &signal_scene_evidence,
                                                          &signal_native_evidence, &signal_copy_window);
    signal_scene_evidence.binding.available_rows = signal_copy_window.available_rows;
    signal_scene_evidence.binding.copied_rows = signal_copy_window.copied_rows;
    if (!signal_scene_valid || !signal_native_valid) {
      result.status = rtxdi::InputStatus::SceneBindingChanged;
      if (diagnostic_enabled) {
        RecordDiSceneEvidence(diagnostics, diagnostic_epoch, rtxdi::DiagnosticStage::InputSignalA,
                              s.generation, in.frame, in.scene_slot, static_cast<int32_t>(k),
                              signal_scene_evidence, signal_native_evidence);
      }
      return finish();
    }
    if (diagnostic_enabled) {
      RecordDiSceneEvidence(diagnostics, diagnostic_epoch, rtxdi::DiagnosticStage::InputSignalA,
                            s.generation, in.frame, in.scene_slot, static_cast<int32_t>(k),
                            signal_scene_evidence, signal_native_evidence, true);
    }
    result.stage = rtxdi::InputStage::Record;
    HRESULT hr = slot.allocator->Reset();
    if (SUCCEEDED(hr)) {
      hr = transfer.list->Reset(slot.allocator.Get(), transfer.unpack_pso.Get());
    }
    if (FAILED(hr)) {
      result.status = rtxdi::InputStatus::SubmitFailed;
      result.error = hr;
      return finish();
    }
    if (perf_timing) transfer.list->EndQuery(slot.perf_queries.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 0u);
    ID3D12Device5* native = s.device12.Get();
    const auto cpu_at = [&](uint32_t index) {
      auto handle = slot.descriptors->GetCPUDescriptorHandleForHeapStart();
      handle.ptr += static_cast<SIZE_T>(index) * s.descriptor_size;
      return handle;
    };
    const D3D12_SHADER_RESOURCE_VIEW_DESC srv = Rt12TextureSrv(DXGI_FORMAT_R32G32B32A32_UINT);
    ID3D12Resource* textures[4] = {slot.scene.texture12.Get(), slot.lights.texture12.Get(), slot.tiles.texture12.Get(), slot.deferred.texture12.Get()};
    ID3D12Resource* buffers[4] = {slot.scene_buffer.Get(), slot.lights_buffer.Get(), slot.tiles_buffer.Get(), slot.deferred_buffer.Get()};
    const uint32_t words[4] = {1536u / 4u, RTXDI_INPUT_MAX_LIGHTS * RTXDI_INPUT_LIGHT_STRIDE / 4u,
                               RTXDI_INPUT_MAX_TILES * RTXDI_INPUT_TILE_STRIDE / 4u, RTXDI_INPUT_MAX_DEFERRED * RTXDI_INPUT_DEFERRED_STRIDE / 4u};
    for (uint32_t i = 0u; i < 4u; ++i) {
      native->CreateShaderResourceView(textures[i], &srv, cpu_at(i));
      D3D12_UNORDERED_ACCESS_VIEW_DESC uav = {};
      uav.Format = DXGI_FORMAT_R32_TYPELESS;
      uav.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
      uav.Buffer.NumElements = words[i];
      uav.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
      native->CreateUnorderedAccessView(buffers[i], nullptr, &uav, cpu_at(4u + i));
    }
    std::vector<D3D12_RESOURCE_BARRIER> barriers;
    barriers.reserve(16u);
    for (uint32_t i = 0u; i < 4u; ++i) {
      if (textures[i]) {
        Transition(barriers, textures[i], D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
      }
      if (buffers[i]) {
        Transition(barriers, buffers[i], D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
      }
    }
    transfer.list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
    ID3D12DescriptorHeap* heaps[] = {slot.descriptors.Get()};
    transfer.list->SetDescriptorHeaps(1u, heaps);
    transfer.list->SetComputeRootSignature(transfer.unpack_root.Get());
    transfer.list->SetPipelineState(transfer.unpack_pso.Get());
    const uint32_t counts[4] = {result.layout.lights_copied, result.layout.tiles_copied, result.layout.deferred_copied, 0u};
    transfer.list->SetComputeRoot32BitConstants(0u, 4u, counts, 0u);
    transfer.list->SetComputeRootDescriptorTable(1u, slot.descriptors->GetGPUDescriptorHandleForHeapStart());
    const uint32_t texels = (std::max)({96u, counts[0] * 7u, counts[1] * 36u, counts[2] * 9u});
    transfer.list->Dispatch((texels + 63u) / 64u, 1u, 1u);
    bool sun_detail_recorded = false;
    if (in.sun) {
      Rt12Slot& scene = s.lighting_slots[static_cast<uint32_t>(in.scene_slot)];
      const bool gc = s.scene_gc;
      ID3D12Resource* vertices = gc ? g_rt12_gc_scene.vertices : s.arenas[kRt12ArenaVertices].buffer.Get();
      const uint64_t vertex_count = gc ? g_rt12_gc_scene.vertex_count : s.arenas[kRt12ArenaVertices].used;
      ID3D12Resource* indices = gc ? g_rt12_gc_scene.indices : s.arenas[kRt12ArenaIndices].buffer.Get();
      const uint64_t index_count = gc ? g_rt12_gc_scene.index_count : s.arenas[kRt12ArenaIndices].used;
      ID3D12Resource* mesh_table = gc ? scene.gc_meshes.Get() : s.mesh_table.Get();
      const uint64_t mesh_count = gc ? bvh::kGcMaxEntries : s.mesh_count;
      ID3D12Resource* uv_arena = gc ? g_rt12_gc_scene.uvs : s.arenas[kRt12ArenaUvs].buffer.Get();
      const uint64_t uv_count = gc ? g_rt12_gc_scene.uv_count : s.arenas[kRt12ArenaUvs].used;
      ID3D12Resource* dynamic_vertices = s.scene_dynamic_count != 0u ? s.dynamic_vertices.Get() : nullptr;
      const bool scene_resources_valid = scene.tlas && scene.instance_table && vertices && indices && mesh_table
                                         && uv_arena && s.atlas.texture12 && s.materials
                                         && (s.scene_dynamic_count == 0u || dynamic_vertices);
      const uint64_t pixels = (uint64_t{in.width} * in.height);
      const auto query_cpu = [&](uint32_t index) {
        auto handle = slot.descriptors->GetCPUDescriptorHandleForHeapStart();
        handle.ptr += static_cast<SIZE_T>(index) * s.descriptor_size;
        return handle;
      };
      const auto query_buffer_srv = [&](uint32_t index, ID3D12Resource* resource, uint32_t stride, uint64_t count) {
        const D3D12_SHADER_RESOURCE_VIEW_DESC view = Rt12BufferSrv(stride, count);
        native->CreateShaderResourceView(resource, &view, query_cpu(index));
      };
      query_buffer_srv(8u, vertices, 16u, vertex_count);
      query_buffer_srv(9u, indices, kRt12ArenaStride[kRt12ArenaIndices], index_count);
      query_buffer_srv(10u, mesh_table, sizeof(bvh::WorldMeshGPU), mesh_count);
      query_buffer_srv(11u, scene.instance_table.Get(), sizeof(bvh::WorldInstanceGPU), scene.instance_count);
      query_buffer_srv(12u, uv_arena, kRt12ArenaStride[kRt12ArenaUvs], uv_count);
      D3D12_SHADER_RESOURCE_VIEW_DESC atlas_srv = {};
      atlas_srv.Format = DXGI_FORMAT_R32_UINT;
      atlas_srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
      atlas_srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
      atlas_srv.Texture2DArray.MipLevels = 1u;
      atlas_srv.Texture2DArray.ArraySize = kRt12AlphaSlices;
      native->CreateShaderResourceView(s.atlas.texture12.Get(), &atlas_srv, query_cpu(13u));
      query_buffer_srv(14u, s.materials.Get(), 32u, kRt12AlphaSlices);
      D3D12_SHADER_RESOURCE_VIEW_DESC tlas_srv = {};
      tlas_srv.Format = DXGI_FORMAT_UNKNOWN;
      tlas_srv.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
      tlas_srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
      tlas_srv.RaytracingAccelerationStructure.Location = (scene.tlas ? scene.tlas->GetGPUVirtualAddress() : 0u);
      native->CreateShaderResourceView(nullptr, &tlas_srv, query_cpu(15u));
      query_buffer_srv(16u, dynamic_vertices, 16u,
                       dynamic_vertices ? uint64_t{kRt12CarrierWidth} * kRt12CarrierHeight : 1u);
      const D3D12_SHADER_RESOURCE_VIEW_DESC depth_srv = Rt12TextureSrv(DXGI_FORMAT_R32_FLOAT);
      native->CreateShaderResourceView(slot.depth.texture12.Get(), &depth_srv, query_cpu(17u));
      native->CreateShaderResourceView(slot.normal.texture12.Get(), &srv, query_cpu(18u));
      native->CreateShaderResourceView(slot.scene.texture12.Get(), &srv, query_cpu(19u));
      native->CreateShaderResourceView(s.noise.texture12.Get(), &srv, query_cpu(24u));
      D3D12_UNORDERED_ACCESS_VIEW_DESC result_uav = {};
      result_uav.Format = DXGI_FORMAT_UNKNOWN;
      result_uav.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
      result_uav.Buffer.NumElements = static_cast<UINT>(pixels);
      result_uav.Buffer.StructureByteStride = RTXDI_SUN_RESULT_STRIDE;
      native->CreateUnorderedAccessView(slot.sun_results.Get(), nullptr, &result_uav, query_cpu(20u));
      D3D12_UNORDERED_ACCESS_VIEW_DESC visibility_uav = {};
      visibility_uav.Format = DXGI_FORMAT_R32_UINT;
      visibility_uav.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
      native->CreateUnorderedAccessView(slot.sun_visibility.texture12.Get(), nullptr, &visibility_uav, query_cpu(21u));

      std::vector<D3D12_RESOURCE_BARRIER> query_barriers;
      query_barriers.reserve(16u);
      for (ID3D12Resource* resource : {slot.depth.texture12.Get(), slot.normal.texture12.Get(), vertices, indices,
                                       mesh_table, scene.instance_table.Get(), uv_arena,
                                       s.atlas.texture12.Get(), s.materials.Get(), dynamic_vertices, s.noise.texture12.Get()}) {
        if (resource) Transition(query_barriers, resource, D3D12_RESOURCE_STATE_COMMON,
                                 D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
      }
      Transition(query_barriers, slot.sun_results.Get(), D3D12_RESOURCE_STATE_COMMON,
                 D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
      Transition(query_barriers, slot.sun_visibility.texture12.Get(), D3D12_RESOURCE_STATE_COMMON,
                 D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
      transfer.list->ResourceBarrier(static_cast<UINT>(query_barriers.size()), query_barriers.data());
      ID3D12DescriptorHeap* query_heaps[] = {slot.descriptors.Get()};
      transfer.list->SetDescriptorHeaps(1u, query_heaps);
      transfer.list->SetComputeRootSignature(transfer.sun_root.Get());
      transfer.list->SetPipelineState(transfer.sun_pso.Get());
      const float maximum_distance = rtsun::g_rtsun_ray_max;
      const float environment_bias = rtxdi::FiniteClamped(rtxdi::g_sun_normal_bias, 0.02f, 0.f, 0.5f);
      const float character_bias = rtxdi::FiniteClamped(rtxdi::g_sun_character_normal_bias, 0.02f, 0.f, 0.5f);
      const bool sun_half = rtxdi::FiniteClamped(rtxdi::g_sun_trace_resolution, 0.f, 0.f, 1.f) > 0.5f;
      const float sun_disk_tan = std::tan(rtxdi::FiniteClamped(rtxdi::g_sun_light_size, 0.f, 0.f, 5.f) * 0.5f * std::numbers::pi_v<float> / 180.f);
      const uint32_t params[24] = {in.width, in.height, std::bit_cast<uint32_t>(maximum_distance),
                                   std::bit_cast<uint32_t>(environment_bias), std::bit_cast<uint32_t>(character_bias),
                                   std::bit_cast<uint32_t>(s.scene_coverage_min[0]),
                                   std::bit_cast<uint32_t>(s.scene_coverage_min[1]),
                                   std::bit_cast<uint32_t>(s.scene_coverage_min[2]),
                                   std::bit_cast<uint32_t>(s.scene_coverage_size),
                                   scene_resources_valid ? 1u : 0u, s.scene_coverage_valid ? 1u : 0u, in.frame,
                                   rtxdi::g_sun_casters_only > 0.5f ? bvh::kInstanceShadowCaster : 0u,
                                   sun_half ? RTXDI_SUN_TRACE_HALF : RTXDI_SUN_TRACE_FULL,
                                   std::bit_cast<uint32_t>(sun_disk_tan),
                                   (rtxdi::g_sun_isfast > 0.5f && s.noise_source != 0u) ? 1u : 0u,
                                   in.frame % 32u,
                                   std::bit_cast<uint32_t>(rtxdi::FiniteClamped(rtxdi::g_sun_foliage_transmission, 0.f, 0.f, 1.f)),
                                   std::bit_cast<uint32_t>(rtxdi::FiniteClamped(rtxdi::g_sun_wind_transmission, 0.5f, 0.f, 1.f)), 0u,
                                   std::bit_cast<uint32_t>(rtxdi::FiniteClamped(rtxdi::g_sun_foliage_tint_r, 1.f, 0.f, 1.f)),
                                   std::bit_cast<uint32_t>(rtxdi::FiniteClamped(rtxdi::g_sun_foliage_tint_g, 1.f, 0.f, 1.f)),
                                   std::bit_cast<uint32_t>(rtxdi::FiniteClamped(rtxdi::g_sun_foliage_tint_b, 1.f, 0.f, 1.f)),
                                   std::bit_cast<uint32_t>(rtxdi::FiniteClamped(rtxdi::g_sun_wind_softening, 0.5f, 0.f, 4.f))};
      transfer.list->SetComputeRoot32BitConstants(0u, 24u, params, 0u);
      auto query_gpu = slot.descriptors->GetGPUDescriptorHandleForHeapStart();
      query_gpu.ptr += static_cast<UINT64>(8u) * s.descriptor_size;
      transfer.list->SetComputeRootDescriptorTable(1u, query_gpu);
      if (sun_half) {
        transfer.list->Dispatch(((in.width + 1u) / 2u + 7u) / 8u, ((in.height + 1u) / 2u + 7u) / 8u, 1u);
        query_barriers.clear();
        UavBarrier(query_barriers, slot.sun_results.Get());
        UavBarrier(query_barriers, slot.sun_visibility.texture12.Get());
        transfer.list->ResourceBarrier(static_cast<UINT>(query_barriers.size()), query_barriers.data());
        transfer.list->SetComputeRoot32BitConstant(0u, RTXDI_SUN_TRACE_RESOLVE, 13u);
      }
      transfer.list->Dispatch((in.width + 7u) / 8u, (in.height + 7u) / 8u, 1u);
      query_barriers.clear();
      UavBarrier(query_barriers, slot.sun_results.Get());
      UavBarrier(query_barriers, slot.sun_visibility.texture12.Get());
      if (sun_detail_requested && slot.sun_detail.texture12) {
#if defined(__world_rtxdi_sun_detail_EMBED_FILE)
        if (!transfer.sun_detail_root || !transfer.sun_detail_pso) {
          D3D12_DESCRIPTOR_RANGE detail_ranges[2] = {};
          detail_ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
          detail_ranges[0].NumDescriptors = 1u;
          detail_ranges[0].BaseShaderRegister = 0u;
          detail_ranges[0].OffsetInDescriptorsFromTableStart = 0u;
          detail_ranges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
          detail_ranges[1].NumDescriptors = 1u;
          detail_ranges[1].BaseShaderRegister = 0u;
          detail_ranges[1].OffsetInDescriptorsFromTableStart = 1u;
          D3D12_ROOT_PARAMETER detail_parameters[2] = {};
          detail_parameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
          detail_parameters[0].Constants.Num32BitValues = 3u;
          detail_parameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
          detail_parameters[1].DescriptorTable.NumDescriptorRanges = 2u;
          detail_parameters[1].DescriptorTable.pDescriptorRanges = detail_ranges;
          D3D12_ROOT_SIGNATURE_DESC detail_desc = {};
          detail_desc.NumParameters = static_cast<UINT>(std::size(detail_parameters));
          detail_desc.pParameters = detail_parameters;
          ComPtr<ID3DBlob> detail_blob, detail_errors;
          ComPtr<ID3D12RootSignature> detail_root;
          ComPtr<ID3D12PipelineState> detail_pso;
          HRESULT detail_hr = s.serialize(&detail_desc, D3D_ROOT_SIGNATURE_VERSION_1, &detail_blob, &detail_errors);
          if (SUCCEEDED(detail_hr)) {
            detail_hr = s.device12->CreateRootSignature(0u, detail_blob->GetBufferPointer(), detail_blob->GetBufferSize(),
                                                        IID_PPV_ARGS(&detail_root));
          }
          if (SUCCEEDED(detail_hr)) {
            D3D12_COMPUTE_PIPELINE_STATE_DESC detail_pipeline = {};
            detail_pipeline.pRootSignature = detail_root.Get();
            detail_pipeline.CS = {__world_rtxdi_sun_detail.data(), __world_rtxdi_sun_detail.size()};
            detail_hr = s.device12->CreateComputePipelineState(&detail_pipeline, IID_PPV_ARGS(&detail_pso));
          }
          if (SUCCEEDED(detail_hr)) {
            transfer.sun_detail_root = std::move(detail_root);
            transfer.sun_detail_pso = std::move(detail_pso);
          }
        }
        if (transfer.sun_detail_root && transfer.sun_detail_pso) {
          D3D12_SHADER_RESOURCE_VIEW_DESC detail_srv = {};
          detail_srv.Format = DXGI_FORMAT_UNKNOWN;
          detail_srv.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
          detail_srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
          detail_srv.Buffer.NumElements = static_cast<UINT>(pixels);
          detail_srv.Buffer.StructureByteStride = RTXDI_SUN_RESULT_STRIDE;
          native->CreateShaderResourceView(slot.sun_results.Get(), &detail_srv, query_cpu(22u));
          D3D12_UNORDERED_ACCESS_VIEW_DESC detail_uav = {};
          detail_uav.Format = DXGI_FORMAT_R32G32B32A32_UINT;
          detail_uav.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
          native->CreateUnorderedAccessView(slot.sun_detail.texture12.Get(), nullptr, &detail_uav, query_cpu(23u));
          std::array<D3D12_RESOURCE_BARRIER, 2u> detail_barriers = {};
          detail_barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
          detail_barriers[0].Transition.pResource = slot.sun_results.Get();
          detail_barriers[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
          detail_barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
          detail_barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
          detail_barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
          detail_barriers[1].Transition.pResource = slot.sun_detail.texture12.Get();
          detail_barriers[1].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
          detail_barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
          detail_barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
          transfer.list->ResourceBarrier(static_cast<UINT>(detail_barriers.size()), detail_barriers.data());
          transfer.list->SetComputeRootSignature(transfer.sun_detail_root.Get());
          transfer.list->SetPipelineState(transfer.sun_detail_pso.Get());
          const std::array<uint32_t, 3u> detail_constants = {in.width, in.height, static_cast<uint32_t>(pixels)};
          transfer.list->SetComputeRoot32BitConstants(0u, static_cast<UINT>(detail_constants.size()),
                                                      detail_constants.data(), 0u);
          auto detail_gpu = slot.descriptors->GetGPUDescriptorHandleForHeapStart();
          detail_gpu.ptr += static_cast<UINT64>(22u) * s.descriptor_size;
          transfer.list->SetComputeRootDescriptorTable(1u, detail_gpu);
          transfer.list->Dispatch((in.width + 7u) / 8u, (in.height + 7u) / 8u, 1u);
          detail_barriers = {};
          detail_barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
          detail_barriers[0].UAV.pResource = slot.sun_detail.texture12.Get();
          detail_barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
          detail_barriers[1].Transition.pResource = slot.sun_results.Get();
          detail_barriers[1].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
          detail_barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
          detail_barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_COMMON;
          transfer.list->ResourceBarrier(static_cast<UINT>(detail_barriers.size()), detail_barriers.data());
          detail_barriers = {};
          detail_barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
          detail_barriers[0].Transition.pResource = slot.sun_detail.texture12.Get();
          detail_barriers[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
          detail_barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
          detail_barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_COMMON;
          transfer.list->ResourceBarrier(1u, detail_barriers.data());
          sun_detail_recorded = true;
        }
#endif
      }
      for (ID3D12Resource* resource : {slot.depth.texture12.Get(), slot.normal.texture12.Get(), vertices, indices,
                                       mesh_table, scene.instance_table.Get(), uv_arena,
                                       s.atlas.texture12.Get(), s.materials.Get(), dynamic_vertices, s.noise.texture12.Get()}) {
        if (resource) Transition(query_barriers, resource, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                                 D3D12_RESOURCE_STATE_COMMON);
      }
      Transition(query_barriers, slot.sun_results.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                 D3D12_RESOURCE_STATE_COMMON);
      Transition(query_barriers, slot.sun_visibility.texture12.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                 D3D12_RESOURCE_STATE_COMMON);
      transfer.list->ResourceBarrier(static_cast<UINT>(query_barriers.size()), query_barriers.data());
    }
    barriers.clear();
    for (auto* buffer : buffers)
      if (buffer) {
        UavBarrier(barriers, buffer);
      }
    transfer.list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
    barriers.clear();
    for (uint32_t i = 0u; i < 4u; ++i) {
      if (textures[i]) {
        Transition(barriers, textures[i], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COMMON);
      }
      if (buffers[i]) {
        Transition(barriers, buffers[i], D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON);
      }
    }
    transfer.list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
    if (perf_timing) {
      transfer.list->EndQuery(slot.perf_queries.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 1u);
      transfer.list->ResolveQueryData(slot.perf_queries.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 0u, 2u,
                                      slot.perf_readback.Get(), 0u);
    }
    hr = transfer.list->Close();
    if (FAILED(hr)) {
      result.status = rtxdi::InputStatus::SubmitFailed;
      result.error = hr;
      return finish();
    }
    result.stage = rtxdi::InputStage::Order;
    bool ordered = false;
    try {
      ordered = renodx::utils::gpu_fence::QueueWait(s.queue12.Get(), s.fence11.get(), a);
      if (ordered && s.last_signal != 0u) {
        ordered = renodx::utils::gpu_fence::QueueWait(s.queue12.Get(), s.fence12.get(), s.last_signal);
      }
    } catch (...) {
      ordered = false;
    }
    if (!ordered) {
      result.status = rtxdi::InputStatus::SubmitFailed;
      result.error = E_FAIL;
      return finish();
    }
    ID3D12CommandList* lists[] = {transfer.list.Get()};
    uint64_t b = renodx::utils::gpu_fence::NO_SIGNAL;
    result.stage = rtxdi::InputStage::Submit;
    s.unfenced_execution = true;
    try {
      s.queue12->ExecuteCommandLists(1u, lists);
      b = renodx::utils::gpu_fence::QueueSignal(s.queue12.Get(), s.fence12.get());
      if (b == renodx::utils::gpu_fence::NO_SIGNAL) throw 0;
      if (rtxdi::ShouldCountAcceptedGpuSubmission(
              bvh::g_perf_gate.gather.load(std::memory_order_relaxed),
              bvh::g_perf_gate.gather_epoch.load(std::memory_order_relaxed) == perf_epoch, b,
              renodx::utils::gpu_fence::NO_SIGNAL)) {
        bvh::g_perf_gate.rtxdi_submissions.fetch_add(1u, std::memory_order_relaxed);
      }
      slot.fence_value = b;
      if (perf_timing) {
        slot.perf_pending = true;
        slot.perf_fence = b;
        slot.perf_valid_mask = 1u;
      }
      Rt12Slot& scene = s.lighting_slots[static_cast<uint32_t>(in.scene_slot)];
      scene.fence_value = (std::max)(scene.fence_value, b);
      s.last_signal = b;
      transfer.input_fence = b;
      result.fence = b;
      result.transfer_slot = static_cast<int32_t>(k);
      result.stage = rtxdi::InputStage::WaitB;
      if (!renodx::utils::gpu_fence::QueueWait(s.ctx11.Get(), s.fence12.get(), b)) throw 0;
      transfer.scene_fence = b;
      transfer.scene_slot = s.scene_slot;
      transfer.input_frame = in.frame;
      transfer.next_slot = (k + 1u) % kRt12Slots;
      transfer.input_slot = static_cast<int32_t>(k);
      slot.light_count = result.layout.lights_copied;
      slot.tile_count = result.layout.tiles_copied;
      slot.deferred_count = result.layout.deferred_copied;
      if (in.sun) {
        slot.output_b_fence = b;
        slot.output_c_fence = 0u;
        slot.output_owner_generation = s.generation;
        slot.output_configuration_generation = transfer.configuration_generation;
        slot.sun_configuration_generation = transfer.configuration_generation;
        slot.output_frame = in.frame;
        slot.output_width = in.width;
        slot.output_height = in.height;
        slot.output_scene_slot = in.scene_slot;
        slot.output_transfer_slot = static_cast<int32_t>(k);
        slot.output_staged = true;
        if (sun_detail_recorded) {
          slot.sun_detail_identity = {.source_version = 1u,
                                      .detail_view = in.sun_detail_view,
                                      .frame = in.frame,
                                      .width = in.width,
                                      .height = in.height,
                                      .scene_slot = in.scene_slot,
                                      .transfer_slot = static_cast<int32_t>(k),
                                      .owner_generation = s.generation,
                                      .transfer_configuration = transfer.configuration_generation,
                                      .di_configuration = s.di.configuration_generation,
                                      .settings_identity = in.effective_settings.identity,
                                      .sun_configuration = transfer.configuration_generation,
                                      .producer_fence_b = b,
                                      .recorded = true,
                                      .staged = true};
        }
      }
      result.fence = b;
      result.status = rtxdi::InputStatus::Ready;
      result.stage = rtxdi::InputStage::Ready;
      s.unfenced_execution = false;
    } catch (...) {
      transfer.input_frame = UINT32_MAX;
      s.failed = true;
      s.stuck = true;
      result.status = rtxdi::InputStatus::Quarantined;
      result.error = E_FAIL;
      UpdatePanel([](Rt12Panel& p) { p.stuck = true; });
      FailSession(s, Rt12Reason::SubmitFailed, "rtxdi input handoff");
      return finish();
    }
    NoteRt12Signal(s, b);
    return finish();
  } catch (...) {
    if (result.status != rtxdi::InputStatus::Ready && result.status != rtxdi::InputStatus::Quarantined) {
      result.status = rtxdi::InputStatus::SubmitFailed;
      result.error = E_FAIL;
    }
    return finish();
  }
}

inline rtxdi::DiStatus DispatchRtxdiInitialDi(reshade::api::device* device, reshade::api::command_list* cmd_list,
                                              const Rt12DiInputViews& in, const Rt12DiInputResult& input) {
  const bvh::PoolCpuTimer cpu_timer{&bvh::g_perf_gate.rtxdi_di_cpu_us};
  using Microsoft::WRL::ComPtr;
  auto& diagnostics = rtxdi::GetDiagnosticState();
  const bool diagnostic_enabled = rtxdi::DiagnosticCaptureEnabled(diagnostics);
  const uint64_t diagnostic_epoch = diagnostic_enabled ? rtxdi::DiagnosticCaptureEpoch(diagnostics) : 0u;
  const bool perf_gather = bvh::g_perf_gate.gather.load(std::memory_order_relaxed);
  const uint64_t perf_epoch = perf_gather ? bvh::g_perf_gate.gather_epoch.load(std::memory_order_relaxed) : 0u;
  if (perf_gather) bvh::g_perf_gate.rtxdi_attempts.fetch_add(1u, std::memory_order_relaxed);
  if (!g_rt12 || g_rt12->device != device || !cmd_list || !in.local || input.status != rtxdi::InputStatus::Ready
      || input.transfer_slot < 0 || input.transfer_slot >= static_cast<int32_t>(kRt12Slots)) {
    if (diagnostic_enabled) {
      rtxdi::RecordDiagnostic(diagnostics, diagnostic_epoch,
                              {.owner_generation = g_rt12 && g_rt12->device == device ? g_rt12->generation : 0u,
                               .frame = in.frame,
                               .capture_frame = in.capture_frame,
                               .settings_identity = in.effective_settings.identity,
                               .hresult = input.error,
                               .scene_slot = in.scene_slot,
                               .transfer_slot = input.transfer_slot,
                               .stage = rtxdi::DiagnosticStage::DiPreflight,
                               .reason = input.status == rtxdi::InputStatus::Busy ? rtxdi::DiagnosticReason::Busy
                                         : input.status == rtxdi::InputStatus::SceneBindingChanged
                                             ? rtxdi::DiagnosticReason::NativeB0Mismatch
                                             : rtxdi::DiagnosticReason::MissingInput,
                               .operation = rtxdi::DiagnosticOperation::Validate});
    }
    return rtxdi::DiStatus::WaitingInput;
  }
  Rt12State& s = *g_rt12;
  auto& transfer = s.di_transfer;
#if defined(FALCOM_NRD_ENABLED)
  ProcessNrdProbeReadbacks(s);
  const bool nrd_requested = Rt12NrdSubmissionAllowed(rr::RrRequested(), rr::RrOwnsDenoise(),
                                                      in.effective_settings.relax_diffuse);
  if (!nrd_requested) {
    transfer.nrd.qualification = {};
    transfer.nrd.history = {};
  }
#endif
  const uint32_t slot_index = static_cast<uint32_t>(input.transfer_slot);
  Rt12DiTransferSlot& slot = transfer.slots[slot_index];
  const bool perf_timing = bvh::g_perf_gate.gather.load(std::memory_order_relaxed)
                           && slot.perf_pending
                           && slot.perf_gather_epoch == bvh::g_perf_gate.gather_epoch.load(std::memory_order_relaxed)
                           && slot.perf_owner_generation == s.generation
                           && slot.perf_frame == in.frame && slot.perf_width == in.width && slot.perf_height == in.height
                           && slot.perf_settings_identity == in.effective_settings.identity;
  bool perf_distance_recorded = false;
  bool perf_nrd_recorded = false;
#if defined(FALCOM_NRD_ENABLED)
  slot.nrd.dispatch_ready = false;
  slot.nrd.diffuse_output_valid = false;
#endif
  bool spatial_ran = false;
  bool temporal_ran = false;
  bool history_valid = false;
#if defined(FALCOM_NRD_ENABLED)
  bool nrd_probe_recorded = false;
  bool nrd_dispatch_recorded = false;
  bool nrd_history_reset = false;
  Rt12NrdHistoryIdentity nrd_accepted_history = {};
  rtxdi::NrdGateSnapshot nrd_gate = {};
#endif
  uint32_t spatial_source_index = UINT32_MAX;
  uint32_t spatial_output_index = UINT32_MAX;
  uint32_t temporal_source_index = UINT32_MAX;
  uint32_t temporal_output_index = UINT32_MAX;
  const auto publish = [&](rtxdi::DiStatus status, int32_t error = 0, uint64_t fence = 0u) {
    if (perf_gather && bvh::g_perf_gate.gather.load(std::memory_order_relaxed)
        && bvh::g_perf_gate.gather_epoch.load(std::memory_order_relaxed) == perf_epoch) {
      if (status == rtxdi::DiStatus::Busy) {
        bvh::g_perf_gate.rtxdi_busy_skips.fetch_add(1u, std::memory_order_relaxed);
      } else if (status == rtxdi::DiStatus::Failed || status == rtxdi::DiStatus::Quarantined) {
        bvh::g_perf_gate.rtxdi_rejections.fetch_add(1u, std::memory_order_relaxed);
      }
    }
    {
      std::unique_lock lock(rtxdi::g_di.mutex);
      rtxdi::g_di.snapshot = {.status = status,
                              .frame = in.frame,
                              .configuration_generation = s.di.configuration_generation,
                              .settings_identity = s.di.settings_identity,
                              .fence = fence,
                              .local_candidates = in.effective_settings.local_candidates,
                              .spatial_samples = in.effective_settings.spatial_samples,
                              .spatial_disocclusion = in.effective_settings.spatial_disocclusion,
                              .history_length = in.effective_settings.history_length,
                              .temporal_enabled = in.effective_settings.temporal_enabled,
                              .spatial_enabled = in.effective_settings.spatial_enabled,
                              .relax_selected = in.effective_settings.relax_diffuse,
                              .scene_slot = in.scene_slot,
                              .transfer_slot = input.transfer_slot,
                              .spatial_ran = spatial_ran,
                              .temporal_ran = temporal_ran,
                              .history_valid = history_valid,
                              .temporal_source_index = temporal_source_index,
                              .temporal_output_index = temporal_output_index,
                              .spatial_source_index = spatial_source_index,
                              .spatial_output_index = spatial_output_index,
                              .output_frame = slot.output_frame,
                              .output_width = slot.output_width,
                              .output_height = slot.output_height,
                              .output_owner_generation = slot.output_owner_generation,
                              .output_configuration_generation = slot.output_configuration_generation,
                              .output_fence_b = slot.output_b_fence,
                              .output_fence_c = slot.output_c_fence,
                              .output_scene_slot = slot.output_scene_slot,
                              .output_transfer_slot = slot.output_transfer_slot,
                              .output_staged = slot.output_staged,
#if defined(FALCOM_NRD_ENABLED)
                              .nrd_diffuse_staged = slot.nrd_diffuse_staged,
                              .nrd_diffuse_bound = false,
                              .nrd_diffuse_lighting_draw_seen = false,
                              .nrd_diffuse_frame = slot.nrd_diffuse_staged ? slot.nrd_diffuse_identity.frame : UINT32_MAX,
                              .nrd_diffuse_transfer_slot = slot.nrd_diffuse_staged ? slot.nrd_diffuse_identity.transfer_slot : -1,
                              .nrd_diffuse_fence_b = slot.nrd_diffuse_staged ? slot.nrd_diffuse_identity.producer_fence_b : 0u,
                              .nrd_gate = nrd_gate,
#endif
                              .output_bound = false,
                              .output_bound_frame = UINT32_MAX,
                              .output_bound_transfer_slot = -1,
                              .output_bound_fence_b = 0u,
                              .lighting_draw_seen = false,
                              .error = error};
    }
    if (diagnostic_enabled) {
      rtxdi::DiagnosticReason reason = rtxdi::DiagnosticReason::None;
      switch (status) {
        case rtxdi::DiStatus::Off:
        case rtxdi::DiStatus::Submitted:    reason = rtxdi::DiagnosticReason::None; break;
        case rtxdi::DiStatus::WaitingInput: reason = rtxdi::DiagnosticReason::MissingInput; break;
        case rtxdi::DiStatus::ZeroLights:   reason = rtxdi::DiagnosticReason::MissingInput; break;
        case rtxdi::DiStatus::Busy:         reason = rtxdi::DiagnosticReason::Busy; break;
        case rtxdi::DiStatus::Failed:
          reason = error == E_INVALIDARG ? rtxdi::DiagnosticReason::ConfigurationMismatch
                                         : rtxdi::DiagnosticReason::SubmitFailed;
          break;
        case rtxdi::DiStatus::Quarantined: reason = rtxdi::DiagnosticReason::Quarantined; break;
      }
      rtxdi::RecordDiagnostic(diagnostics, diagnostic_epoch,
                              {.owner_generation = s.generation,
                               .frame = in.frame,
                               .capture_frame = in.capture_frame,
                               .settings_identity = in.effective_settings.identity,
                               .effective_identity = s.di.settings_identity,
                               .fence = fence,
                               .value0 = static_cast<uint64_t>(status),
                               .value1 = static_cast<uint64_t>(input.status),
                               .hresult = error,
                               .scene_slot = in.scene_slot,
                               .transfer_slot = input.transfer_slot,
                               .stage = rtxdi::DiagnosticStage::DiPreflight,
                               .reason = reason,
                               .operation = status == rtxdi::DiStatus::Submitted
                                                ? rtxdi::DiagnosticOperation::Dispatch
                                                : rtxdi::DiagnosticOperation::Validate});
    }
    return status;
  };
  if (s.failed || s.stuck || s.unfenced_execution) return publish(rtxdi::DiStatus::Quarantined, E_FAIL);
  const rtxdi::DiSubmissionGate gate = rtxdi::CheckDiSubmissionIdentity({
      .requested_frame = in.frame,
      .transfer_frame = transfer.input_frame,
      .requested_generation = s.generation,
      .owner_generation = transfer.owner_generation,
      .requested_scene_slot = in.scene_slot,
      .transfer_scene_slot = transfer.scene_slot,
      .requested_transfer_slot = input.transfer_slot,
      .transfer_slot = transfer.input_slot,
      .requested_fence = input.fence,
      .transfer_fence = transfer.input_fence,
      .requested_width = in.width,
      .requested_height = in.height,
      .config_width = s.di.config.width,
      .config_height = s.di.config.height,
      .local_requested = in.local,
      .input_ready = input.status == rtxdi::InputStatus::Ready,
      .transfer_available = slot.fence_value == input.fence,
      // Same-queue ordering and the reservoir UAV/transition barriers guard reuse, so presence is enough here.
      .reservoir_available = s.di.reservoirs != nullptr,
  });
  if (gate != rtxdi::DiSubmissionGate::Ready) {
    const rtxdi::DiStatus status = (gate == rtxdi::DiSubmissionGate::Busy                    ? rtxdi::DiStatus::Busy
                                    : gate == rtxdi::DiSubmissionGate::ConfigurationMismatch ? rtxdi::DiStatus::Failed
                                                                                             : rtxdi::DiStatus::WaitingInput);
    return publish(status, (gate == rtxdi::DiSubmissionGate::ConfigurationMismatch ? E_INVALIDARG : 0));
  }
  if (!rtxdi::ValidSceneLease(s.scene_slot, s.scene_frame, in.scene_slot, in.frame, s.scene_fence, in.scene_fence)
      || !rtxdi::SceneCoverageSnapshotMatches(
          s.scene_slot, s.scene_frame, s.scene_generation, s.scene_fence, s.scene_gc_version,
          s.scene_coverage_slot, s.scene_coverage_frame, s.scene_coverage_generation,
          s.scene_coverage_fence, s.scene_coverage_gc_version)
      || reinterpret_cast<ID3D11DeviceContext*>(cmd_list->get_native()) != s.ctx11.Get()) {  // NOLINT(performance-no-int-to-ptr)
    return publish(rtxdi::DiStatus::WaitingInput);
  }
  if (!s.di.context || s.di.settings_identity != in.effective_settings.identity || !slot.local
      || slot.width != in.width || slot.height != in.height || !slot.di_allocator
      || !slot.di_descriptors || !slot.light_types || !slot.di_results || !slot.di_frame_upload) {
    return publish(rtxdi::DiStatus::Failed, E_FAIL);
  }
  ComPtr<ID3D11Buffer> current_scene;
  s.ctx11->PSGetConstantBuffers(0u, 1u, &current_scene);
  reshade::api::buffer_range scene_range = {};
  rtxdi::SceneRangeEvidence scene_evidence = {};
  DiSceneNativeBindingEvidence native_evidence = {};
  const bool scene_range_valid = CurrentDiSceneRange(device, cmd_list, in, current_scene.Get(), &scene_range,
                                                     &scene_evidence);
  rtxdi::SceneCopyWindow scene_copy_window = {};
  const bool native_range_valid = ValidateBoundDiScene(s.ctx11.Get(), scene_range, &scene_evidence,
                                                       &native_evidence, &scene_copy_window);
  scene_evidence.binding.available_rows = scene_copy_window.available_rows;
  scene_evidence.binding.copied_rows = scene_copy_window.copied_rows;
  if (!scene_range_valid || !native_range_valid) {
    if (diagnostic_enabled) {
      RecordDiSceneEvidence(diagnostics, diagnostic_epoch, rtxdi::DiagnosticStage::DiPreflight,
                            s.generation, in.frame, in.scene_slot, input.transfer_slot,
                            scene_evidence, native_evidence);
    }
    return publish(rtxdi::DiStatus::WaitingInput, E_FAIL);
  }
  if (diagnostic_enabled) {
    RecordDiSceneEvidence(diagnostics, diagnostic_epoch, rtxdi::DiagnosticStage::DiPreflight,
                          s.generation, in.frame, in.scene_slot, input.transfer_slot,
                          scene_evidence, native_evidence, true);
  }

  const uint32_t light_count = input.layout.lights_copied;
  const uint32_t tile_count = input.layout.tiles_copied;
  Rt12DiTransferSlot* previous_slot = nullptr;
  if (s.di.history_valid && s.di.history_transfer_slot >= 0 && s.di.history_transfer_slot < static_cast<int32_t>(kRt12Slots)) {
    Rt12DiTransferSlot& candidate = transfer.slots[static_cast<uint32_t>(s.di.history_transfer_slot)];
    const bool previous_inputs = candidate.local && candidate.width == in.width && candidate.height == in.height
                                 && candidate.depth.texture12 && candidate.normal.texture12 && candidate.scene.texture12
                                 && candidate.color.texture12 && candidate.mrt2.texture12 && candidate.deferred_buffer
                                 && candidate.lights_buffer && candidate.light_types;
    if (rtxdi::ValidDiHistoryIdentity({.previous_frame = s.di.history_frame,
                                       .current_frame = in.frame,
                                       .previous_width = s.di.history_width,
                                       .previous_height = s.di.history_height,
                                       .current_width = in.width,
                                       .current_height = in.height,
                                       .previous_transfer_slot = s.di.history_transfer_slot,
                                       .current_transfer_slot = static_cast<int32_t>(slot_index),
                                       .previous_owner_generation = s.di.history_owner_generation,
                                       .current_owner_generation = s.generation,
                                       .previous_configuration_generation = s.di.history_configuration_generation,
                                       .current_configuration_generation = s.di.configuration_generation,
                                       .motion_available = input.layout.motion_available != 0u,
                                       .previous_inputs_available = previous_inputs})) {
      previous_slot = &candidate;
      history_valid = true;
    }
  }
  s.di.frame.parameters = rtxdi::MakeDiFrameParameters(s.di.config, *s.di.context);
  s.di.frame.parameters.width = in.width;
  s.di.frame.parameters.height = in.height;
  s.di.frame.parameters.history_valid = history_valid ? 1u : 0u;
  bool diag_armed = diagnostic_enabled && !slot.diag_pending && in.frame >= transfer.diag_next_frame;
  if (diag_armed) {
    if (!transfer.diag_zero) {
      void* zero_bytes = nullptr;
      const D3D12_RANGE no_reads = {0u, 0u};
      if (FAILED(CreateRt12Buffer(s.device12.Get(), D3D12_HEAP_TYPE_UPLOAD, RTXDI_DIAG_COUNTER_BYTES,
                                  D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_FLAG_NONE, transfer.diag_zero))
          || FAILED(transfer.diag_zero->Map(0u, &no_reads, &zero_bytes)) || zero_bytes == nullptr) {
        transfer.diag_zero.Reset();
      } else {
        std::memset(zero_bytes, 0, RTXDI_DIAG_COUNTER_BYTES);
        transfer.diag_zero->Unmap(0u, nullptr);
      }
    }
    if (!slot.diag_counters
        && FAILED(CreateRt12Buffer(s.device12.Get(), D3D12_HEAP_TYPE_DEFAULT, RTXDI_DIAG_COUNTER_BYTES,
                                   D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, slot.diag_counters))) {
      slot.diag_counters.Reset();
    }
    if (!slot.diag_readback
        && FAILED(CreateRt12Buffer(s.device12.Get(), D3D12_HEAP_TYPE_READBACK, RTXDI_DIAG_COUNTER_BYTES,
                                   D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_FLAG_NONE, slot.diag_readback))) {
      slot.diag_readback.Reset();
    }
    diag_armed = transfer.diag_zero && slot.diag_counters && slot.diag_readback;
  }
  if (diag_armed) s.di.frame.parameters.flags |= RTXDI_FRAME_FLAG_DIAGNOSTICS;
  s.di.frame.parameters.lights.localLightBufferRegion = {0u, light_count, 0u, 0u};
  s.di.frame.parameters.lights.infiniteLightBufferRegion = {};
  s.di.frame.parameters.lights.environmentLightParams = {};
  if (!rtxdi::ValidDiBufferIndices(s.di.frame.parameters.di.bufferIndices, in.effective_settings)) {
    return publish(rtxdi::DiStatus::Failed, E_FAIL);
  }
  temporal_source_index = s.di.frame.parameters.di.bufferIndices.temporalResamplingInputBufferIndex;
  temporal_output_index = s.di.frame.parameters.di.bufferIndices.temporalResamplingOutputBufferIndex;

  if (!transfer.classify_root || !transfer.classify_pso) {
    D3D12_DESCRIPTOR_RANGE ranges[2] = {};
    ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    ranges[0].NumDescriptors = 1u;
    ranges[0].BaseShaderRegister = 0u;
    ranges[0].OffsetInDescriptorsFromTableStart = 0u;
    ranges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    ranges[1].NumDescriptors = 1u;
    ranges[1].BaseShaderRegister = 0u;
    ranges[1].OffsetInDescriptorsFromTableStart = 1u;
    D3D12_ROOT_PARAMETER params[2] = {};
    params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    params[0].Constants.Num32BitValues = 4u;
    params[0].Constants.ShaderRegister = 12u;
    params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    params[1].DescriptorTable.NumDescriptorRanges = 2u;
    params[1].DescriptorTable.pDescriptorRanges = ranges;
    D3D12_ROOT_SIGNATURE_DESC desc = {};
    desc.NumParameters = 2u;
    desc.pParameters = params;
    ComPtr<ID3DBlob> blob, error;
    ComPtr<ID3D12RootSignature> root;
    ComPtr<ID3D12PipelineState> pso;
    HRESULT hr = s.serialize(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &error);
    if (SUCCEEDED(hr)) hr = s.device12->CreateRootSignature(0u, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&root));
#if defined(__world_rtxdi_classify_lights_EMBED_FILE)
    if (SUCCEEDED(hr)) {
      D3D12_COMPUTE_PIPELINE_STATE_DESC pipeline = {};
      pipeline.pRootSignature = root.Get();
      pipeline.CS = {__world_rtxdi_classify_lights.data(), __world_rtxdi_classify_lights.size()};
      hr = s.device12->CreateComputePipelineState(&pipeline, IID_PPV_ARGS(&pso));
    }
#else
    hr = E_FAIL;
#endif
    if (FAILED(hr)) return publish(rtxdi::DiStatus::Failed, hr);
    transfer.classify_root = std::move(root);
    transfer.classify_pso = std::move(pso);
  }
  if (!transfer.initial_di_root || !transfer.initial_di_pso || !transfer.temporal_di_pso || !transfer.spatial_di_pso) {
    D3D12_DESCRIPTOR_RANGE ranges[13] = {};
    const auto set_range = [&](uint32_t index, D3D12_DESCRIPTOR_RANGE_TYPE type, uint32_t count,
                               uint32_t shader_register, uint32_t offset) {
      ranges[index].RangeType = type;
      ranges[index].NumDescriptors = count;
      ranges[index].BaseShaderRegister = shader_register;
      ranges[index].OffsetInDescriptorsFromTableStart = offset;
    };
    set_range(0u, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4u, 0u, 0u);
    set_range(1u, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3u, 14u, 4u);
    set_range(2u, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 6u, 17u, 7u);
    set_range(3u, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1u, 29u, 13u);
    set_range(4u, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1u, 30u, 14u);
    set_range(5u, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3u, 31u, 15u);
    set_range(6u, D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 3u, 0u, 18u);
    set_range(7u, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 9u, 34u, 22u);
    set_range(8u, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2u, 43u, 31u);
    set_range(9u, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1u, 45u, 35u);
    set_range(10u, D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 3u, 3u, 36u);
    set_range(11u, D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1u, 6u, 39u);
    set_range(12u, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1u, 61u, 21u);
    D3D12_ROOT_PARAMETER params[4] = {};
    params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    params[0].Constants.Num32BitValues = 21u;
    params[0].Constants.ShaderRegister = 0u;
    params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    params[1].Descriptor.ShaderRegister = 1u;
    params[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    params[2].DescriptorTable.NumDescriptorRanges = 13u;
    params[2].DescriptorTable.pDescriptorRanges = ranges;
    params[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    params[3].Constants.Num32BitValues = 10u;
    params[3].Constants.ShaderRegister = 12u;
    D3D12_ROOT_SIGNATURE_DESC desc = {};
    desc.NumParameters = 4u;
    desc.pParameters = params;
    ComPtr<ID3DBlob> blob, error;
    ComPtr<ID3D12RootSignature> root;
    ComPtr<ID3D12PipelineState> pso, temporal_pso, spatial_pso, n1_resolve_pso;
    HRESULT hr = s.serialize(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &error);
    if (SUCCEEDED(hr)) hr = s.device12->CreateRootSignature(0u, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&root));
#if defined(__world_rtxdi_initial_di_EMBED_FILE) && defined(__world_rtxdi_temporal_di_EMBED_FILE) && defined(__world_rtxdi_spatial_di_EMBED_FILE)
    if (SUCCEEDED(hr)) {
      D3D12_COMPUTE_PIPELINE_STATE_DESC pipeline = {};
      pipeline.pRootSignature = root.Get();
      pipeline.CS = {__world_rtxdi_temporal_di.data(), __world_rtxdi_temporal_di.size()};
      hr = s.device12->CreateComputePipelineState(&pipeline, IID_PPV_ARGS(&temporal_pso));
    }
    if (SUCCEEDED(hr)) {
      D3D12_COMPUTE_PIPELINE_STATE_DESC pipeline = {};
      pipeline.pRootSignature = root.Get();
      pipeline.CS = {__world_rtxdi_initial_di.data(), __world_rtxdi_initial_di.size()};
      hr = s.device12->CreateComputePipelineState(&pipeline, IID_PPV_ARGS(&pso));
    }
    if (SUCCEEDED(hr)) {
      D3D12_COMPUTE_PIPELINE_STATE_DESC pipeline = {};
      pipeline.pRootSignature = root.Get();
      pipeline.CS = {__world_rtxdi_spatial_di.data(), __world_rtxdi_spatial_di.size()};
      hr = s.device12->CreateComputePipelineState(&pipeline, IID_PPV_ARGS(&spatial_pso));
    }
#if defined(__world_rtxdi_reconstruct_EMBED_FILE)
    if (SUCCEEDED(hr)) {
      D3D12_COMPUTE_PIPELINE_STATE_DESC pipeline = {};
      pipeline.pRootSignature = root.Get();
      pipeline.CS = {__world_rtxdi_reconstruct.data(), __world_rtxdi_reconstruct.size()};
      if (FAILED(s.device12->CreateComputePipelineState(&pipeline, IID_PPV_ARGS(&n1_resolve_pso)))) {
        n1_resolve_pso.Reset();
      }
    }
#endif
#else
    hr = E_FAIL;
#endif
    if (FAILED(hr)) return publish(rtxdi::DiStatus::Failed, hr);
    transfer.initial_di_root = std::move(root);
    transfer.initial_di_pso = std::move(pso);
    transfer.temporal_di_pso = std::move(temporal_pso);
    transfer.spatial_di_pso = std::move(spatial_pso);
    transfer.n1_resolve_pso = std::move(n1_resolve_pso);
  }

  if (!transfer.di_list) {
    ComPtr<ID3D12GraphicsCommandList4> list;
    HRESULT hr = s.device12->CreateCommandList(0u, D3D12_COMMAND_LIST_TYPE_DIRECT, slot.di_allocator.Get(),
                                               nullptr, IID_PPV_ARGS(&list));
    if (SUCCEEDED(hr)) hr = list->Close();
    if (FAILED(hr)) return publish(rtxdi::DiStatus::Failed, hr);
    transfer.di_list = std::move(list);
  }
  HRESULT hr = slot.di_allocator->Reset();
  if (SUCCEEDED(hr)) hr = transfer.di_list->Reset(slot.di_allocator.Get(), transfer.initial_di_pso.Get());
  if (FAILED(hr)) return publish(rtxdi::DiStatus::Failed, hr);
  if (perf_timing) transfer.di_list->EndQuery(slot.perf_queries.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 2u);

  void* frame_bytes = nullptr;
  const D3D12_RANGE no_reads = {0u, 0u};
  hr = slot.di_frame_upload->Map(0u, &no_reads, &frame_bytes);
  if (FAILED(hr) || !frame_bytes) return publish(rtxdi::DiStatus::Failed, FAILED(hr) ? hr : E_FAIL);
  std::memcpy(frame_bytes, &s.di.frame, sizeof(s.di.frame));
  slot.di_frame_upload->Unmap(0u, nullptr);

  ID3D12Device5* native = s.device12.Get();
  const auto cpu_at = [&](uint32_t index) {
    auto handle = slot.di_descriptors->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += static_cast<SIZE_T>(index) * s.descriptor_size;
    return handle;
  };
  const auto write_buffer_srv = [&](uint32_t index, ID3D12Resource* resource, uint32_t stride, uint64_t count) {
    const D3D12_SHADER_RESOURCE_VIEW_DESC view = Rt12BufferSrv(stride, count);
    native->CreateShaderResourceView(resource, &view, cpu_at(index));
  };
  const auto write_buffer_uav = [&](uint32_t index, ID3D12Resource* resource, uint32_t stride, uint64_t count) {
    D3D12_UNORDERED_ACCESS_VIEW_DESC view = {};
    view.Format = DXGI_FORMAT_UNKNOWN;
    view.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    view.Buffer.NumElements = static_cast<UINT>(count);
    view.Buffer.StructureByteStride = stride;
    native->CreateUnorderedAccessView(resource, nullptr, &view, cpu_at(index));
  };
  Rt12Slot& scene = s.lighting_slots[static_cast<uint32_t>(in.scene_slot)];
  const bool gc = s.scene_gc;
  ID3D12Resource* vertices = gc ? g_rt12_gc_scene.vertices : s.arenas[kRt12ArenaVertices].buffer.Get();
  const uint64_t vertex_count = gc ? g_rt12_gc_scene.vertex_count : s.arenas[kRt12ArenaVertices].used;
  ID3D12Resource* indices = gc ? g_rt12_gc_scene.indices : s.arenas[kRt12ArenaIndices].buffer.Get();
  const uint64_t index_count = gc ? g_rt12_gc_scene.index_count : s.arenas[kRt12ArenaIndices].used;
  ID3D12Resource* mesh_table = gc ? scene.gc_meshes.Get() : s.mesh_table.Get();
  const uint64_t mesh_count = gc ? bvh::kGcMaxEntries : s.mesh_count;
  ID3D12Resource* uv_arena = gc ? g_rt12_gc_scene.uvs : s.arenas[kRt12ArenaUvs].buffer.Get();
  const uint64_t uv_count = gc ? g_rt12_gc_scene.uv_count : s.arenas[kRt12ArenaUvs].used;
  ID3D12Resource* dynamic_vertices = s.scene_dynamic_count != 0u ? s.dynamic_vertices.Get() : nullptr;
  const bool scene_resources_valid = scene.tlas && scene.instance_table && vertices && indices && mesh_table && uv_arena
                                     && s.atlas.texture12 && s.materials
                                     && (s.scene_dynamic_count == 0u || dynamic_vertices);
  write_buffer_srv(0u, vertices, 16u, vertex_count);
  write_buffer_srv(1u, indices, kRt12ArenaStride[kRt12ArenaIndices], index_count);
  write_buffer_srv(2u, mesh_table, sizeof(bvh::WorldMeshGPU), mesh_count);
  write_buffer_srv(3u, scene.instance_table.Get(), sizeof(bvh::WorldInstanceGPU), scene.instance_count);
  write_buffer_srv(4u, uv_arena, kRt12ArenaStride[kRt12ArenaUvs], uv_count);
  D3D12_SHADER_RESOURCE_VIEW_DESC atlas_srv = {};
  atlas_srv.Format = DXGI_FORMAT_R32_UINT;
  atlas_srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
  atlas_srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  atlas_srv.Texture2DArray.MipLevels = 1u;
  atlas_srv.Texture2DArray.ArraySize = kRt12AlphaSlices;
  native->CreateShaderResourceView(s.atlas.texture12.Get(), &atlas_srv, cpu_at(5u));
  write_buffer_srv(6u, s.materials.Get(), 32u, kRt12AlphaSlices);
  write_buffer_srv(7u, slot.lights_buffer.Get(), RTXDI_INPUT_LIGHT_STRIDE, light_count);
  write_buffer_srv(8u, slot.light_types.Get(), sizeof(uint32_t), RTXDI_INPUT_MAX_LIGHTS);
  const D3D12_SHADER_RESOURCE_VIEW_DESC float4_texture = Rt12TextureSrv(DXGI_FORMAT_R32G32B32A32_FLOAT);
  const D3D12_SHADER_RESOURCE_VIEW_DESC uint4_texture = Rt12TextureSrv(DXGI_FORMAT_R32G32B32A32_UINT);
  native->CreateShaderResourceView(slot.color.texture12.Get(), &float4_texture, cpu_at(9u));
  native->CreateShaderResourceView(slot.mrt2.texture12.Get(), &uint4_texture, cpu_at(10u));
  write_buffer_srv(11u, slot.deferred_buffer.Get(), RTXDI_INPUT_DEFERRED_STRIDE, input.layout.deferred_copied);
  D3D12_SHADER_RESOURCE_VIEW_DESC tlas_srv = {};
  tlas_srv.Format = DXGI_FORMAT_UNKNOWN;
  tlas_srv.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
  tlas_srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  tlas_srv.RaytracingAccelerationStructure.Location = (scene.tlas ? scene.tlas->GetGPUVirtualAddress() : 0u);
  D3D12_SHADER_RESOURCE_VIEW_DESC neighbor_srv = {};
  neighbor_srv.Format = DXGI_FORMAT_R8G8_SNORM;
  neighbor_srv.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
  neighbor_srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  neighbor_srv.Buffer.NumElements = rtxdi::kNeighborCount;
  native->CreateShaderResourceView(s.di.neighbors.Get(), &neighbor_srv, cpu_at(12u));
  native->CreateShaderResourceView(nullptr, &tlas_srv, cpu_at(13u));
  write_buffer_srv(14u, dynamic_vertices, 16u, dynamic_vertices ? uint64_t{kRt12CarrierWidth} * kRt12CarrierHeight : 1u);
  const D3D12_SHADER_RESOURCE_VIEW_DESC depth_srv = Rt12TextureSrv(DXGI_FORMAT_R32_FLOAT);
  native->CreateShaderResourceView(slot.depth.texture12.Get(), &depth_srv, cpu_at(15u));
  native->CreateShaderResourceView(slot.normal.texture12.Get(), &uint4_texture, cpu_at(16u));
  native->CreateShaderResourceView(slot.scene.texture12.Get(), &uint4_texture, cpu_at(17u));
  const uint64_t reservoir_count = s.di.config.reservoir_bytes / sizeof(RTXDI_PackedDIReservoir);
  const uint64_t pixel_count = uint64_t{in.width} * in.height;
  write_buffer_uav(18u, s.di.reservoirs.Get(), sizeof(RTXDI_PackedDIReservoir), reservoir_count);
  write_buffer_uav(19u, slot.di_results.Get(), sizeof(RtxdiDiRawResult), pixel_count);
  D3D12_UNORDERED_ACCESS_VIEW_DESC resolved_uav = {};
  resolved_uav.Format = DXGI_FORMAT_R32G32B32A32_UINT;
  resolved_uav.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
  native->CreateUnorderedAccessView(slot.di_resolved.texture12.Get(), nullptr, &resolved_uav, cpu_at(20u));
  write_buffer_srv(33u, slot.tiles_buffer.Get(), RTXDI_INPUT_TILE_STRIDE, tile_count);
  write_buffer_uav(34u, slot.light_types.Get(), sizeof(uint32_t), RTXDI_INPUT_MAX_LIGHTS);
  const D3D12_SHADER_RESOURCE_VIEW_DESC motion_srv = Rt12TextureSrv(DXGI_FORMAT_R32G32B32A32_FLOAT);
  native->CreateShaderResourceView(slot.motion.texture12.Get(), &motion_srv, cpu_at(22u));
  Rt12DiTransferSlot& history_slot = (previous_slot ? *previous_slot : slot);
  native->CreateShaderResourceView(history_slot.depth.texture12.Get(), &depth_srv, cpu_at(23u));
  native->CreateShaderResourceView(history_slot.normal.texture12.Get(), &uint4_texture, cpu_at(24u));
  native->CreateShaderResourceView(history_slot.scene.texture12.Get(), &uint4_texture, cpu_at(25u));
  native->CreateShaderResourceView(history_slot.color.texture12.Get(), &float4_texture, cpu_at(26u));
  native->CreateShaderResourceView(history_slot.mrt2.texture12.Get(), &uint4_texture, cpu_at(27u));
  write_buffer_srv(28u, history_slot.deferred_buffer.Get(), RTXDI_INPUT_DEFERRED_STRIDE, history_slot.deferred_count);
  write_buffer_srv(29u, history_slot.lights_buffer.Get(), RTXDI_INPUT_LIGHT_STRIDE, history_slot.light_count);
  write_buffer_srv(30u, history_slot.light_types.Get(), sizeof(uint32_t), RTXDI_INPUT_MAX_LIGHTS);
  write_buffer_srv(31u, slot.tiles_buffer.Get(), RTXDI_INPUT_TILE_STRIDE, tile_count);
  write_buffer_srv(32u, history_slot.tiles_buffer.Get(), RTXDI_INPUT_TILE_STRIDE, history_slot.tile_count);
  const D3D12_SHADER_RESOURCE_VIEW_DESC mrt1_srv = Rt12TextureSrv(DXGI_FORMAT_R32G32B32A32_UINT);
  native->CreateShaderResourceView(slot.mrt1.texture12.Get(), &mrt1_srv, cpu_at(35u));
  const auto write_texture_uav = [&](uint32_t index, ID3D12Resource* resource, DXGI_FORMAT format) {
    D3D12_UNORDERED_ACCESS_VIEW_DESC view = {};
    view.Format = format;
    view.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    native->CreateUnorderedAccessView(resource, nullptr, &view, cpu_at(index));
  };
  const bool n1_ready = transfer.n1_resolve_pso && slot.n1_resources_ready && slot.n1_diffuse.texture12 && slot.n1_specular.texture12
                        && slot.n1_status.texture12;
  if (n1_ready) {
    write_texture_uav(36u, slot.n1_diffuse.texture12.Get(), DXGI_FORMAT_R16G16B16A16_FLOAT);
    write_texture_uav(37u, slot.n1_specular.texture12.Get(), DXGI_FORMAT_R16G16B16A16_FLOAT);
    write_texture_uav(38u, slot.n1_status.texture12.Get(), DXGI_FORMAT_R32_UINT);
  }
  D3D12_UNORDERED_ACCESS_VIEW_DESC diag_view = {};
  diag_view.Format = DXGI_FORMAT_R32_TYPELESS;
  diag_view.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
  diag_view.Buffer.NumElements = RTXDI_DIAG_COUNTER_COUNT;
  diag_view.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
  native->CreateUnorderedAccessView(diag_armed ? slot.diag_counters.Get() : nullptr, nullptr, &diag_view, cpu_at(39u));
  native->CreateShaderResourceView(s.noise.texture12.Get(), &uint4_texture, cpu_at(21u));

  ID3D12DescriptorHeap* heaps[] = {slot.di_descriptors.Get()};
  transfer.di_list->SetDescriptorHeaps(1u, heaps);
  std::vector<D3D12_RESOURCE_BARRIER> barriers;
  barriers.reserve(32u);
  if (diag_armed) {
    Transition(barriers, slot.diag_counters.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
    transfer.di_list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
    transfer.di_list->CopyBufferRegion(slot.diag_counters.Get(), 0u, transfer.diag_zero.Get(), 0u, RTXDI_DIAG_COUNTER_BYTES);
    barriers.clear();
    Transition(barriers, slot.diag_counters.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
  }
  Transition(barriers, slot.tiles_buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
  for (ID3D12Resource* resource : {slot.light_types.Get()}) {
    Transition(barriers, resource, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
  }
  transfer.di_list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
  transfer.di_list->SetComputeRootSignature(transfer.classify_root.Get());
  transfer.di_list->SetPipelineState(transfer.classify_pso.Get());
  const uint32_t classify_table_constants[4] = {light_count, tile_count, 0u, 0u};
  transfer.di_list->SetComputeRoot32BitConstants(0u, 4u, classify_table_constants, 0u);
  auto classify_gpu = slot.di_descriptors->GetGPUDescriptorHandleForHeapStart();
  classify_gpu.ptr += static_cast<UINT64>(33u) * s.descriptor_size;
  transfer.di_list->SetComputeRootDescriptorTable(1u, classify_gpu);
  if (light_count != 0u) transfer.di_list->Dispatch((light_count + 63u) / 64u, 1u, 1u);
  if (tile_count != 0u) {
    barriers.clear();
    UavBarrier(barriers, slot.light_types.Get());
    transfer.di_list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
    const uint32_t classify_tiles_constants[4] = {light_count, tile_count, 1u, 0u};
    transfer.di_list->SetComputeRoot32BitConstants(0u, 4u, classify_tiles_constants, 0u);
    transfer.di_list->Dispatch((tile_count + 63u) / 64u, 1u, 1u);
  }
  barriers.clear();
  ID3D12Resource* history_resources[] = {history_slot.depth.texture12.Get(), history_slot.normal.texture12.Get(),
                                         history_slot.scene.texture12.Get(), history_slot.color.texture12.Get(),
                                         history_slot.mrt2.texture12.Get(), history_slot.deferred_buffer.Get(),
                                         history_slot.lights_buffer.Get(), history_slot.light_types.Get(), history_slot.tiles_buffer.Get()};
  for (ID3D12Resource* resource : {vertices, indices, mesh_table, scene.instance_table.Get(), uv_arena, s.atlas.texture12.Get(),
                                   s.materials.Get(), slot.lights_buffer.Get(), slot.light_types.Get(), slot.color.texture12.Get(),
                                   slot.mrt1.texture12.Get(), slot.mrt2.texture12.Get(), slot.deferred_buffer.Get(), dynamic_vertices, slot.depth.texture12.Get(),
                                   slot.normal.texture12.Get(), slot.scene.texture12.Get(), slot.motion.texture12.Get(), s.noise.texture12.Get()}) {
    if (resource) Transition(barriers, resource, resource == slot.light_types.Get() ? D3D12_RESOURCE_STATE_UNORDERED_ACCESS : D3D12_RESOURCE_STATE_COMMON,
                             resource == slot.light_types.Get() ? D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
                                                                : D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
  }
  if (history_valid) {
    for (ID3D12Resource* resource : history_resources) {
      if (resource) Transition(barriers, resource, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    }
  }
  Transition(barriers, s.di.reservoirs.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
  Transition(barriers, slot.di_results.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
  Transition(barriers, slot.di_resolved.texture12.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
  transfer.di_list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
  transfer.di_list->SetComputeRootSignature(transfer.initial_di_root.Get());
  transfer.di_list->SetPipelineState(transfer.initial_di_pso.Get());
  const float local_light_size = rtxdi::FiniteClamped(rtxdi::g_local_light_size, 0.f, 0.f, 0.5f);
  const uint32_t sun_params[21] = {in.width, in.height, scene_resources_valid ? 1u : 0u, s.scene_coverage_valid ? 1u : 0u,
                                   light_count,
                                   std::bit_cast<uint32_t>(rtxdi::FiniteClamped(rtxdi::g_local_normal_bias, 0.02f, 0.f, 0.5f)),
                                   std::bit_cast<uint32_t>(rtxdi::FiniteClamped(rtxdi::g_local_character_normal_bias, 0.02f, 0.f, 0.5f)),
                                   std::bit_cast<uint32_t>(rtxdi::FiniteClamped(rtxdi::g_local_light_clearance, 0.2f, 0.f, 1.f)),
                                   std::bit_cast<uint32_t>(s.scene_coverage_min[0]),
                                   std::bit_cast<uint32_t>(s.scene_coverage_min[1]),
                                   std::bit_cast<uint32_t>(s.scene_coverage_min[2]),
                                   std::bit_cast<uint32_t>(s.scene_coverage_size), input.layout.deferred_copied,
                                   history_slot.deferred_count, history_slot.light_count, tile_count, history_slot.tile_count,
                                   std::bit_cast<uint32_t>(local_light_size),
                                   (rtxdi::g_local_isfast > 0.5f && s.noise_source != 0u && local_light_size > 0.f) ? 1u : 0u,
                                   in.frame % 32u, std::bit_cast<uint32_t>(rtxdi::FiniteClamped(rtxdi::g_local_character_self_softening, 0.f, 0.f, 2.f))};
  transfer.di_list->SetComputeRoot32BitConstants(0u, 21u, sun_params, 0u);
  transfer.di_list->SetComputeRootConstantBufferView(1u, slot.di_frame_upload->GetGPUVirtualAddress());
  transfer.di_list->SetComputeRootDescriptorTable(2u, slot.di_descriptors->GetGPUDescriptorHandleForHeapStart());
  transfer.di_list->Dispatch((in.width + 7u) / 8u, (in.height + 7u) / 8u, 1u);
  barriers.clear();
  UavBarrier(barriers, s.di.reservoirs.Get());
  transfer.di_list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
  if (history_valid && in.effective_settings.temporal_enabled) {
    transfer.di_list->SetPipelineState(transfer.temporal_di_pso.Get());
    transfer.di_list->Dispatch((in.width + 7u) / 8u, (in.height + 7u) / 8u, 1u);
    barriers.clear();
    UavBarrier(barriers, s.di.reservoirs.Get());
    transfer.di_list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
  }
  transfer.di_list->SetPipelineState(transfer.spatial_di_pso.Get());
  transfer.di_list->Dispatch((in.width + 7u) / 8u, (in.height + 7u) / 8u, 1u);
  if (n1_ready) {
    barriers.clear();
    UavBarrier(barriers, slot.di_results.Get());
    transfer.di_list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
    barriers.clear();
    Transition(barriers, slot.n1_diffuse.texture12.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    Transition(barriers, slot.n1_specular.texture12.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    Transition(barriers, slot.n1_status.texture12.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    transfer.di_list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
    transfer.di_list->SetPipelineState(transfer.n1_resolve_pso.Get());
    const float env_keep = 1.f - rtxdi::FiniteClamped(rtxdi::g_local_shadow_darkening, 1.f, 0.f, 1.f);
    const float char_keep = 1.f - rtxdi::FiniteClamped(rtxdi::g_local_character_shadow_darkening, 1.f, 0.f, 1.f);
    const uint32_t n1_constants[10] = {
        RTXDI_N1_CONFIG_VERSION,
        std::bit_cast<uint32_t>(in.n1_brdf_config[0]),
        std::bit_cast<uint32_t>(in.n1_brdf_config[1]),
        std::bit_cast<uint32_t>(in.n1_brdf_config[2]),
        std::bit_cast<uint32_t>(in.n1_brdf_config[3]),
        std::bit_cast<uint32_t>(in.n1_brdf_config[4]),
        std::bit_cast<uint32_t>(in.n1_brdf_config[5]),
        std::bit_cast<uint32_t>(in.n1_brdf_config[6]),
        std::bit_cast<uint32_t>(env_keep),
        std::bit_cast<uint32_t>(char_keep),
    };
    transfer.di_list->SetComputeRoot32BitConstants(3u, 10u, n1_constants, 0u);
    transfer.di_list->Dispatch((in.width + 7u) / 8u, (in.height + 7u) / 8u, 1u);
  }
#if defined(FALCOM_NRD_ENABLED)
  bool camera_candidate_valid = false;
  bool projection_compatible = false;
  const bool nrd_allowed = nrd_requested;
  nrd_gate.frame = in.frame;
  bool gate_open = true;
  const auto nrd_gate_link = [&](rtxdi::NrdGateLink link, bool pass, uint8_t reason) {
    if (!gate_open) return false;
    nrd_gate.status[static_cast<uint32_t>(link)] = pass ? rtxdi::kNrdGatePass : rtxdi::kNrdGateFail;
    nrd_gate.reason[static_cast<uint32_t>(link)] = pass ? 0u : reason;
    gate_open = pass;
    return pass;
  };
  nrd_gate_link(rtxdi::NrdGateLink::Requested, in.effective_settings.relax_diffuse, 1u);
  nrd_gate_link(rtxdi::NrdGateLink::RrFree, !rr::RrRequested() && !rr::RrOwnsDenoise(), rr::RrRequested() ? 1u : 2u);
  nrd_gate_link(rtxdi::NrdGateLink::N1Ready, n1_ready, !transfer.n1_resolve_pso ? 1u : 2u);
  nrd_gate_link(rtxdi::NrdGateLink::History, history_valid && previous_slot != nullptr, 1u);
  nrd_gate_link(rtxdi::NrdGateLink::Motion, input.layout.motion_available != 0u, 1u);
  if (gate_open) {
    nrd_gate.previous_capture_frame = previous_slot->nrd.camera.capture_frame;
    uint8_t previous_frame_reason = 0u;
    if (!slot.nrd.camera.valid || !previous_slot->nrd.camera.valid) {
      previous_frame_reason = 1u;
    } else if (slot.nrd.camera.capture_frame != in.frame || previous_slot->nrd.camera.capture_frame == UINT32_MAX
               || previous_slot->nrd.camera.capture_frame + 1u != in.frame) {
      previous_frame_reason = 2u;
    } else if (slot.nrd.camera.owner_generation != s.generation
               || previous_slot->nrd.camera.owner_generation != s.generation) {
      previous_frame_reason = 3u;
    } else if (previous_slot->di_fence_value == 0u || previous_slot->fence_value == 0u) {
      previous_frame_reason = 4u;
    }
    nrd_gate_link(rtxdi::NrdGateLink::PreviousFrame, previous_frame_reason == 0u, previous_frame_reason);
  }
  if (nrd_allowed && n1_ready && history_valid && previous_slot != nullptr && input.layout.motion_available != 0u
      && slot.nrd.camera.valid && previous_slot->nrd.camera.valid && slot.nrd.camera.capture_frame == in.frame
      && previous_slot->nrd.camera.capture_frame != UINT32_MAX
      && previous_slot->nrd.camera.capture_frame + 1u == in.frame
      && slot.nrd.camera.owner_generation == s.generation
      && previous_slot->nrd.camera.owner_generation == s.generation
      && previous_slot->di_fence_value != 0u && previous_slot->fence_value != 0u) {
    rtxdi::CameraCandidate camera_candidate;
    if (rtxdi::BuildCameraCandidate(slot.nrd.camera.scene_rows.data(), previous_slot->nrd.camera.scene_rows.data(),
                                    in.width, in.height, &camera_candidate)) {
      camera_candidate_valid = true;
      slot.nrd.camera_candidate = camera_candidate;
      const auto& qualification = transfer.nrd.qualification;
      projection_compatible = previous_slot != nullptr && previous_slot->nrd.camera_candidate.matrices_valid
                              && rtxdi::CameraMatricesNear(camera_candidate.view_to_clip,
                                                           previous_slot->nrd.camera_candidate.view_to_clip, 1.0e-4f);
      const bool qualification_matches = rtxdi::CameraQualificationMatches(
          qualification, s.generation, transfer.configuration_generation, slot.nrd.camera.resource,
          in.width, in.height, camera_candidate_valid, projection_compatible);
      if (!qualification_matches) {
        const auto probe_record_start = std::chrono::steady_clock::now();
        nrd_probe_recorded = RecordNrdCameraProbe(s, slot, *previous_slot, in, camera_candidate);
        slot.nrd.probe_record_cpu_microseconds = std::chrono::duration<double, std::micro>(
                                                     std::chrono::steady_clock::now() - probe_record_start)
                                                     .count();
      }
    }
  }
  const auto& camera_qualification = transfer.nrd.qualification;
  const bool keep_camera_qualified = rtxdi::CameraQualificationMatches(
      camera_qualification, s.generation, transfer.configuration_generation, slot.nrd.camera.resource,
      in.width, in.height, camera_candidate_valid, projection_compatible);
  if (!nrd_probe_recorded && !keep_camera_qualified) {
    rtxdi::ObserveCameraProbe(&transfer.nrd.qualification, in.frame, s.generation,
                              transfer.configuration_generation, slot.nrd.camera.resource, in.width, in.height,
                              camera_candidate_valid, false, UINT32_MAX);
  }
  nrd_gate.camera_passes = transfer.nrd.qualification.consecutive_passes;
  nrd_gate.camera_correction = transfer.nrd.qualification.motion_correction_index;
  uint8_t camera_reason = 5u;
  if (!camera_candidate_valid) {
    camera_reason = 1u;
  } else if (!projection_compatible) {
    camera_reason = 2u;
  } else if (!transfer.nrd.qualification.qualified) {
    camera_reason = 3u;
  } else if (transfer.nrd.qualification.motion_correction_index >= 2u) {
    camera_reason = 4u;
  }
  nrd_gate_link(rtxdi::NrdGateLink::Camera, keep_camera_qualified && transfer.nrd.qualification.motion_correction_index < 2u,
       camera_reason);
  if (nrd_allowed && n1_ready && keep_camera_qualified
      && transfer.nrd.qualification.motion_correction_index < 2u) {
    Rt12Slot& nrd_scene = s.lighting_slots[static_cast<uint32_t>(in.scene_slot)];
    Rt12NrdSlot& nrd_slot = slot.nrd;
    bool hitdistance_recorded = false;
    uint8_t hitdistance_reason = 1u;
    if (transfer.nrd_hitdistance_root && transfer.nrd_hitdistance_pso) {
      hitdistance_recorded = true;
    } else {
#if defined(__world_rtxdi_nrd_hitdistance_EMBED_FILE)
      D3D12_DESCRIPTOR_RANGE hitdistance_ranges[6] = {};
      hitdistance_ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
      hitdistance_ranges[0].NumDescriptors = 4u;
      hitdistance_ranges[0].BaseShaderRegister = 0u;
      hitdistance_ranges[0].OffsetInDescriptorsFromTableStart = 0u;
      hitdistance_ranges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
      hitdistance_ranges[1].NumDescriptors = 3u;
      hitdistance_ranges[1].BaseShaderRegister = 14u;
      hitdistance_ranges[1].OffsetInDescriptorsFromTableStart = 4u;
      hitdistance_ranges[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
      hitdistance_ranges[2].NumDescriptors = 1u;
      hitdistance_ranges[2].BaseShaderRegister = 29u;
      hitdistance_ranges[2].OffsetInDescriptorsFromTableStart = 7u;
      hitdistance_ranges[3].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
      hitdistance_ranges[3].NumDescriptors = 1u;
      hitdistance_ranges[3].BaseShaderRegister = 30u;
      hitdistance_ranges[3].OffsetInDescriptorsFromTableStart = 8u;
      hitdistance_ranges[4].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
      hitdistance_ranges[4].NumDescriptors = 4u;
      hitdistance_ranges[4].BaseShaderRegister = 31u;
      hitdistance_ranges[4].OffsetInDescriptorsFromTableStart = 9u;
      hitdistance_ranges[5].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
      hitdistance_ranges[5].NumDescriptors = 2u;
      hitdistance_ranges[5].BaseShaderRegister = 0u;
      hitdistance_ranges[5].OffsetInDescriptorsFromTableStart = 13u;
      D3D12_ROOT_PARAMETER hitdistance_parameters[3] = {};
      hitdistance_parameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
      hitdistance_parameters[0].Constants.ShaderRegister = 0u;
      hitdistance_parameters[0].Constants.Num32BitValues = 12u;
      hitdistance_parameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
      hitdistance_parameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
      hitdistance_parameters[1].DescriptorTable.NumDescriptorRanges = 5u;
      hitdistance_parameters[1].DescriptorTable.pDescriptorRanges = hitdistance_ranges;
      hitdistance_parameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
      hitdistance_parameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
      hitdistance_parameters[2].DescriptorTable.NumDescriptorRanges = 1u;
      hitdistance_parameters[2].DescriptorTable.pDescriptorRanges = &hitdistance_ranges[5];
      hitdistance_parameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
      D3D12_ROOT_SIGNATURE_DESC hitdistance_desc = {};
      hitdistance_desc.NumParameters = 3u;
      hitdistance_desc.pParameters = hitdistance_parameters;
      ComPtr<ID3DBlob> hitdistance_blob, hitdistance_errors;
      ComPtr<ID3D12RootSignature> hitdistance_root;
      ComPtr<ID3D12PipelineState> hitdistance_pso;
      HRESULT hitdistance_hr = s.serialize(&hitdistance_desc, D3D_ROOT_SIGNATURE_VERSION_1,
                                           &hitdistance_blob, &hitdistance_errors);
      if (SUCCEEDED(hitdistance_hr)) {
        hitdistance_hr = s.device12->CreateRootSignature(0u, hitdistance_blob->GetBufferPointer(),
                                                         hitdistance_blob->GetBufferSize(),
                                                         IID_PPV_ARGS(&hitdistance_root));
      }
      if (SUCCEEDED(hitdistance_hr)) {
        D3D12_COMPUTE_PIPELINE_STATE_DESC hitdistance_pipeline = {};
        hitdistance_pipeline.pRootSignature = hitdistance_root.Get();
        hitdistance_pipeline.CS = {__world_rtxdi_nrd_hitdistance.data(),
                                   __world_rtxdi_nrd_hitdistance.size()};
        hitdistance_hr = s.device12->CreateComputePipelineState(&hitdistance_pipeline,
                                                                IID_PPV_ARGS(&hitdistance_pso));
      }
      if (SUCCEEDED(hitdistance_hr)) {
        transfer.nrd_hitdistance_root = std::move(hitdistance_root);
        transfer.nrd_hitdistance_pso = std::move(hitdistance_pso);
        hitdistance_recorded = true;
      }
#endif
    }

    if (hitdistance_recorded && (!nrd_slot.hit_distance || !nrd_slot.hit_validity || !nrd_slot.hitdistance_descriptors || nrd_slot.hitdistance_width != in.width || nrd_slot.hitdistance_height != in.height)) {
      if (!s.device12 || in.width == 0u || in.height == 0u || uint64_t{in.width} * in.height > UINT32_MAX) {
        hitdistance_recorded = false;
        hitdistance_reason = 2u;
      } else {
        const uint64_t hitdistance_bytes = uint64_t{in.width} * in.height * sizeof(uint32_t);
        Rt12NrdSlot hitdistance_candidate;
        D3D12_DESCRIPTOR_HEAP_DESC hitdistance_heap = {};
        hitdistance_heap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        hitdistance_heap.NumDescriptors = 15u;
        hitdistance_heap.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        if (FAILED(CreateRt12Buffer(s.device12.Get(), D3D12_HEAP_TYPE_DEFAULT, hitdistance_bytes,
                                    D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                                    hitdistance_candidate.hit_distance))
            || FAILED(CreateRt12Buffer(s.device12.Get(), D3D12_HEAP_TYPE_DEFAULT, hitdistance_bytes,
                                       D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                                       hitdistance_candidate.hit_validity))
            || FAILED(s.device12->CreateDescriptorHeap(&hitdistance_heap,
                                                       IID_PPV_ARGS(&hitdistance_candidate.hitdistance_descriptors)))) {
          hitdistance_recorded = false;
          hitdistance_reason = 2u;
        } else {
          nrd_slot.hit_distance = std::move(hitdistance_candidate.hit_distance);
          nrd_slot.hit_validity = std::move(hitdistance_candidate.hit_validity);
          nrd_slot.hitdistance_descriptors = std::move(hitdistance_candidate.hitdistance_descriptors);
          nrd_slot.hitdistance_width = in.width;
          nrd_slot.hitdistance_height = in.height;
        }
      }
    }

    if (hitdistance_recorded && nrd_scene.tlas && nrd_scene.instance_table && slot.n1_status.texture12
        && vertices && indices && mesh_table && uv_arena && s.atlas.texture12 && s.materials
        && (s.scene_dynamic_count == 0u || dynamic_vertices)) {
      ID3D12Device* device12 = s.device12.Get();
      ID3D12GraphicsCommandList* di_list = transfer.di_list.Get();
      const uint32_t descriptor_stride = device12->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
      const D3D12_CPU_DESCRIPTOR_HANDLE cpu = nrd_slot.hitdistance_descriptors->GetCPUDescriptorHandleForHeapStart();
      const D3D12_GPU_DESCRIPTOR_HANDLE gpu = nrd_slot.hitdistance_descriptors->GetGPUDescriptorHandleForHeapStart();
      const auto cpu_at = [&](uint32_t index) {
        return D3D12_CPU_DESCRIPTOR_HANDLE{cpu.ptr + static_cast<SIZE_T>(index) * descriptor_stride};
      };
      const auto write_buffer_srv = [&](uint32_t index, ID3D12Resource* resource, uint32_t stride, uint64_t count) {
        const D3D12_SHADER_RESOURCE_VIEW_DESC view = Rt12BufferSrv(stride, count);
        device12->CreateShaderResourceView(resource, &view, cpu_at(index));
      };
      write_buffer_srv(0u, vertices, 16u, vertex_count);
      write_buffer_srv(1u, indices, kRt12ArenaStride[kRt12ArenaIndices], index_count);
      write_buffer_srv(2u, mesh_table, sizeof(bvh::WorldMeshGPU), mesh_count);
      write_buffer_srv(3u, nrd_scene.instance_table.Get(), sizeof(bvh::WorldInstanceGPU), nrd_scene.instance_count);
      write_buffer_srv(4u, uv_arena, kRt12ArenaStride[kRt12ArenaUvs], uv_count);
      D3D12_SHADER_RESOURCE_VIEW_DESC atlas_srv = {};
      atlas_srv.Format = DXGI_FORMAT_R32_UINT;
      atlas_srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
      atlas_srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
      atlas_srv.Texture2DArray.MipLevels = 1u;
      atlas_srv.Texture2DArray.ArraySize = kRt12AlphaSlices;
      device12->CreateShaderResourceView(s.atlas.texture12.Get(), &atlas_srv, cpu_at(5u));
      write_buffer_srv(6u, s.materials.Get(), 32u, kRt12AlphaSlices);
      D3D12_SHADER_RESOURCE_VIEW_DESC tlas_srv = {};
      tlas_srv.Format = DXGI_FORMAT_UNKNOWN;
      tlas_srv.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
      tlas_srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
      tlas_srv.RaytracingAccelerationStructure.Location = nrd_scene.tlas->GetGPUVirtualAddress();
      device12->CreateShaderResourceView(nullptr, &tlas_srv, cpu_at(7u));
      write_buffer_srv(8u, dynamic_vertices, 16u,
                       dynamic_vertices ? uint64_t{kRt12CarrierWidth} * kRt12CarrierHeight : 1u);
      const D3D12_SHADER_RESOURCE_VIEW_DESC depth_srv = Rt12TextureSrv(DXGI_FORMAT_R32_FLOAT);
      const D3D12_SHADER_RESOURCE_VIEW_DESC uint4_srv = Rt12TextureSrv(DXGI_FORMAT_R32G32B32A32_UINT);
      device12->CreateShaderResourceView(slot.depth.texture12.Get(), &depth_srv, cpu_at(9u));
      device12->CreateShaderResourceView(slot.normal.texture12.Get(), &uint4_srv, cpu_at(10u));
      device12->CreateShaderResourceView(slot.scene.texture12.Get(), &uint4_srv, cpu_at(11u));
      const D3D12_SHADER_RESOURCE_VIEW_DESC status_srv = Rt12TextureSrv(DXGI_FORMAT_R32_UINT);
      device12->CreateShaderResourceView(slot.n1_status.texture12.Get(), &status_srv, cpu_at(12u));
      D3D12_UNORDERED_ACCESS_VIEW_DESC distance_uav = {};
      distance_uav.Format = DXGI_FORMAT_UNKNOWN;
      distance_uav.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
      distance_uav.Buffer.NumElements = static_cast<UINT>(uint64_t{in.width} * in.height);
      distance_uav.Buffer.StructureByteStride = sizeof(float);
      device12->CreateUnorderedAccessView(nrd_slot.hit_distance.Get(), nullptr, &distance_uav, cpu_at(13u));
      distance_uav.Buffer.StructureByteStride = sizeof(uint32_t);
      device12->CreateUnorderedAccessView(nrd_slot.hit_validity.Get(), nullptr, &distance_uav, cpu_at(14u));

      std::array<D3D12_RESOURCE_BARRIER, 3u> hitdistance_barriers = {};
      hitdistance_barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
      hitdistance_barriers[0].Transition.pResource = slot.n1_status.texture12.Get();
      hitdistance_barriers[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
      hitdistance_barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
      hitdistance_barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
      for (uint32_t index = 0u; index < 2u; ++index) {
        D3D12_RESOURCE_BARRIER& barrier = hitdistance_barriers[index + 1u];
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = index == 0u ? nrd_slot.hit_distance.Get() : nrd_slot.hit_validity.Get();
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
      }
      di_list->ResourceBarrier(static_cast<UINT>(hitdistance_barriers.size()), hitdistance_barriers.data());
      ID3D12DescriptorHeap* hitdistance_heaps[] = {nrd_slot.hitdistance_descriptors.Get()};
      di_list->SetDescriptorHeaps(1u, hitdistance_heaps);
      di_list->SetComputeRootSignature(transfer.nrd_hitdistance_root.Get());
      di_list->SetPipelineState(transfer.nrd_hitdistance_pso.Get());
      const std::array<uint32_t, 12u> hitdistance_constants = {
          in.width, in.height, in.frame, scene_resources_valid ? 1u : 0u,
          s.scene_coverage_valid ? 1u : 0u,
          std::bit_cast<uint32_t>(rtxdi::FiniteClamped(rtxdi::g_local_normal_bias, 0.02f, 0.f, 0.5f)),
          std::bit_cast<uint32_t>(rtxdi::FiniteClamped(rtxdi::g_local_character_normal_bias, 0.02f, 0.f, 0.5f)),
          std::bit_cast<uint32_t>(65504.0f),
          std::bit_cast<uint32_t>(s.scene_coverage_min[0]),
          std::bit_cast<uint32_t>(s.scene_coverage_min[1]),
          std::bit_cast<uint32_t>(s.scene_coverage_min[2]),
          std::bit_cast<uint32_t>(s.scene_coverage_size)};
      di_list->SetComputeRoot32BitConstants(0u, static_cast<UINT>(hitdistance_constants.size()),
                                            hitdistance_constants.data(), 0u);
      di_list->SetComputeRootDescriptorTable(1u, gpu);
      di_list->SetComputeRootDescriptorTable(2u, {gpu.ptr + static_cast<UINT64>(13u) * descriptor_stride});
      if (perf_timing) di_list->EndQuery(slot.perf_queries.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 4u);
      di_list->Dispatch((in.width + 7u) / 8u, (in.height + 7u) / 8u, 1u);
      hitdistance_barriers = {};
      hitdistance_barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
      hitdistance_barriers[0].UAV.pResource = nrd_slot.hit_distance.Get();
      hitdistance_barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
      hitdistance_barriers[1].UAV.pResource = nrd_slot.hit_validity.Get();
      hitdistance_barriers[2].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
      hitdistance_barriers[2].Transition.pResource = slot.n1_status.texture12.Get();
      hitdistance_barriers[2].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
      hitdistance_barriers[2].Transition.StateBefore = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
      hitdistance_barriers[2].Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
      di_list->ResourceBarrier(static_cast<UINT>(hitdistance_barriers.size()), hitdistance_barriers.data());
      hitdistance_barriers = {};
      hitdistance_barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
      hitdistance_barriers[0].Transition.pResource = nrd_slot.hit_distance.Get();
      hitdistance_barriers[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
      hitdistance_barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
      hitdistance_barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_COMMON;
      hitdistance_barriers[1] = hitdistance_barriers[0];
      hitdistance_barriers[1].Transition.pResource = nrd_slot.hit_validity.Get();
      di_list->ResourceBarrier(2u, hitdistance_barriers.data());
      if (perf_timing) {
        di_list->EndQuery(slot.perf_queries.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 5u);
        perf_distance_recorded = true;
      }
    } else {
      hitdistance_recorded = false;
      hitdistance_reason = 3u;
    }
    const uint32_t debug_view = in.effective_debug_view;
    const bool distance_debug_requested = RtxdiDebugDistanceExportRequested(debug_view);
    slot.debug_distance_identity.recorded = false;
    slot.debug_distance_identity.staged = false;
    slot.debug_distance_identity.requested_view = debug_view;
    if (distance_debug_requested && hitdistance_recorded && nrd_slot.hit_distance && nrd_slot.hit_validity
        && slot.n1_status.texture12 && in.width != 0u && in.height != 0u
        && uint64_t{in.width} * in.height <= UINT32_MAX) {
#if defined(__world_rtxdi_debug_distance_EMBED_FILE)
      if (!transfer.debug_distance_root || !transfer.debug_distance_pso) {
        D3D12_DESCRIPTOR_RANGE debug_ranges[2] = {};
        debug_ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        debug_ranges[0].NumDescriptors = 3u;
        debug_ranges[0].BaseShaderRegister = 0u;
        debug_ranges[0].OffsetInDescriptorsFromTableStart = 0u;
        debug_ranges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
        debug_ranges[1].NumDescriptors = 1u;
        debug_ranges[1].BaseShaderRegister = 0u;
        debug_ranges[1].OffsetInDescriptorsFromTableStart = 0u;
        D3D12_ROOT_PARAMETER debug_parameters[3] = {};
        debug_parameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        debug_parameters[0].Constants.ShaderRegister = 0u;
        debug_parameters[0].Constants.Num32BitValues = 3u;
        debug_parameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        debug_parameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        debug_parameters[1].DescriptorTable.NumDescriptorRanges = 1u;
        debug_parameters[1].DescriptorTable.pDescriptorRanges = &debug_ranges[0];
        debug_parameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        debug_parameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        debug_parameters[2].DescriptorTable.NumDescriptorRanges = 1u;
        debug_parameters[2].DescriptorTable.pDescriptorRanges = &debug_ranges[1];
        debug_parameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        D3D12_ROOT_SIGNATURE_DESC debug_desc = {};
        debug_desc.NumParameters = static_cast<UINT>(std::size(debug_parameters));
        debug_desc.pParameters = debug_parameters;
        ComPtr<ID3DBlob> debug_blob, debug_errors;
        ComPtr<ID3D12RootSignature> debug_root;
        ComPtr<ID3D12PipelineState> debug_pso;
        HRESULT debug_hr = s.serialize(&debug_desc, D3D_ROOT_SIGNATURE_VERSION_1, &debug_blob, &debug_errors);
        if (SUCCEEDED(debug_hr)) {
          debug_hr = s.device12->CreateRootSignature(0u, debug_blob->GetBufferPointer(), debug_blob->GetBufferSize(),
                                                     IID_PPV_ARGS(&debug_root));
        }
        if (SUCCEEDED(debug_hr)) {
          D3D12_COMPUTE_PIPELINE_STATE_DESC debug_pipeline = {};
          debug_pipeline.pRootSignature = debug_root.Get();
          debug_pipeline.CS = {__world_rtxdi_debug_distance.data(), __world_rtxdi_debug_distance.size()};
          debug_hr = s.device12->CreateComputePipelineState(&debug_pipeline, IID_PPV_ARGS(&debug_pso));
        }
        if (SUCCEEDED(debug_hr)) {
          transfer.debug_distance_root = std::move(debug_root);
          transfer.debug_distance_pso = std::move(debug_pso);
        }
      }
      if (transfer.debug_distance_root && transfer.debug_distance_pso) {
        const uint64_t pixel_count = uint64_t{in.width} * in.height;
        if (!slot.nrd_debug_distance.texture12 || slot.nrd_debug_distance.width != in.width
            || slot.nrd_debug_distance.height != in.height) {
          Rt12Shared debug_candidate;
          const HRESULT debug_texture_hr = CreateRt12Shared(s, device, DXGI_FORMAT_R16G16B16A16_FLOAT,
                                                            in.width, in.height, 1u, true, &debug_candidate);
          if (SUCCEEDED(debug_texture_hr)) {
            RetireShared(s, slot.nrd_debug_distance, slot.output_b_fence);
            slot.nrd_debug_distance = std::move(debug_candidate);
          }
        }
        if (slot.nrd_debug_distance.texture12) {
          if (!nrd_slot.debug_distance_descriptors || nrd_slot.hitdistance_width != in.width
              || nrd_slot.hitdistance_height != in.height) {
            D3D12_DESCRIPTOR_HEAP_DESC debug_heap_desc = {};
            debug_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
            debug_heap_desc.NumDescriptors = 4u;
            debug_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
            ComPtr<ID3D12DescriptorHeap> debug_heap;
            if (SUCCEEDED(s.device12->CreateDescriptorHeap(&debug_heap_desc, IID_PPV_ARGS(&debug_heap)))) {
              nrd_slot.debug_distance_descriptors = std::move(debug_heap);
            }
          }
          if (nrd_slot.debug_distance_descriptors) {
            ID3D12Device* device12 = s.device12.Get();
            ID3D12GraphicsCommandList* di_list = transfer.di_list.Get();
            const UINT descriptor_stride = device12->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            const D3D12_CPU_DESCRIPTOR_HANDLE cpu = nrd_slot.debug_distance_descriptors->GetCPUDescriptorHandleForHeapStart();
            const D3D12_GPU_DESCRIPTOR_HANDLE gpu = nrd_slot.debug_distance_descriptors->GetGPUDescriptorHandleForHeapStart();
            const auto cpu_at = [&](uint32_t index) {
              return D3D12_CPU_DESCRIPTOR_HANDLE{cpu.ptr + static_cast<SIZE_T>(index) * descriptor_stride};
            };
            const D3D12_SHADER_RESOURCE_VIEW_DESC status_srv = Rt12TextureSrv(DXGI_FORMAT_R32_UINT);
            device12->CreateShaderResourceView(slot.n1_status.texture12.Get(), &status_srv, cpu_at(0u));
            D3D12_SHADER_RESOURCE_VIEW_DESC hit_srv = {};
            hit_srv.Format = DXGI_FORMAT_UNKNOWN;
            hit_srv.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
            hit_srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            hit_srv.Buffer.NumElements = static_cast<UINT>(pixel_count);
            hit_srv.Buffer.StructureByteStride = sizeof(float);
            device12->CreateShaderResourceView(nrd_slot.hit_distance.Get(), &hit_srv, cpu_at(1u));
            hit_srv.Buffer.StructureByteStride = sizeof(uint32_t);
            device12->CreateShaderResourceView(nrd_slot.hit_validity.Get(), &hit_srv, cpu_at(2u));
            D3D12_UNORDERED_ACCESS_VIEW_DESC debug_uav = {};
            debug_uav.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
            debug_uav.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
            device12->CreateUnorderedAccessView(slot.nrd_debug_distance.texture12.Get(), nullptr, &debug_uav, cpu_at(3u));

            std::array<D3D12_RESOURCE_BARRIER, 4u> debug_barriers = {};
            debug_barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            debug_barriers[0].Transition.pResource = slot.n1_status.texture12.Get();
            debug_barriers[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            debug_barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
            debug_barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
            for (uint32_t index = 0u; index < 2u; ++index) {
              D3D12_RESOURCE_BARRIER& barrier = debug_barriers[index + 1u];
              barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
              barrier.Transition.pResource = index == 0u ? nrd_slot.hit_distance.Get() : nrd_slot.hit_validity.Get();
              barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
              barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
              barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
            }
            debug_barriers[3].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            debug_barriers[3].Transition.pResource = slot.nrd_debug_distance.texture12.Get();
            debug_barriers[3].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            debug_barriers[3].Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
            debug_barriers[3].Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
            di_list->ResourceBarrier(static_cast<UINT>(debug_barriers.size()), debug_barriers.data());
            ID3D12DescriptorHeap* debug_heaps[] = {nrd_slot.debug_distance_descriptors.Get()};
            di_list->SetDescriptorHeaps(1u, debug_heaps);
            di_list->SetComputeRootSignature(transfer.debug_distance_root.Get());
            di_list->SetPipelineState(transfer.debug_distance_pso.Get());
            const std::array<uint32_t, 3u> debug_constants = {in.width, in.height, static_cast<uint32_t>(pixel_count)};
            di_list->SetComputeRoot32BitConstants(0u, static_cast<UINT>(debug_constants.size()), debug_constants.data(), 0u);
            di_list->SetComputeRootDescriptorTable(1u, gpu);
            di_list->SetComputeRootDescriptorTable(2u, {gpu.ptr + static_cast<UINT64>(3u) * descriptor_stride});
            di_list->Dispatch((in.width + 7u) / 8u, (in.height + 7u) / 8u, 1u);
            debug_barriers = {};
            debug_barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
            debug_barriers[0].UAV.pResource = slot.nrd_debug_distance.texture12.Get();
            debug_barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            debug_barriers[1].Transition.pResource = slot.n1_status.texture12.Get();
            debug_barriers[1].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            debug_barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
            debug_barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
            for (uint32_t index = 0u; index < 2u; ++index) {
              D3D12_RESOURCE_BARRIER& barrier = debug_barriers[index + 2u];
              barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
              barrier.Transition.pResource = index == 0u ? nrd_slot.hit_distance.Get() : nrd_slot.hit_validity.Get();
              barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
              barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
              barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COMMON;
            }
            di_list->ResourceBarrier(static_cast<UINT>(debug_barriers.size()), debug_barriers.data());
            debug_barriers = {};
            debug_barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            debug_barriers[0].Transition.pResource = slot.nrd_debug_distance.texture12.Get();
            debug_barriers[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            debug_barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
            debug_barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_COMMON;
            di_list->ResourceBarrier(1u, debug_barriers.data());
            slot.debug_distance_identity.recorded = true;
          }
        }
      }
#endif
    }
    if (perf_timing && hitdistance_recorded) {
      transfer.di_list->EndQuery(slot.perf_queries.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 6u);
    }
    nrd_gate_link(rtxdi::NrdGateLink::HitDistance, hitdistance_recorded, hitdistance_reason);
    nrd_dispatch_recorded = hitdistance_recorded
                            && RecordNrdDenoise(s, slot, in, in.effective_settings, &nrd_accepted_history);
    nrd_gate_link(rtxdi::NrdGateLink::Denoise, nrd_dispatch_recorded, 1u);
    slot.nrd.diffuse_publication_recorded = false;
    uint8_t publication_reason = 4u;
#if defined(__world_rtxdi_nrd_publish_EMBED_FILE)
    const bool publication_out_diff = slot.nrd.resources[static_cast<uint32_t>(nrd::ResourceType::OUT_DIFF_RADIANCE_HITDIST)].resource != nullptr;
    const bool publication_view_z = slot.nrd.resources[static_cast<uint32_t>(nrd::ResourceType::IN_VIEWZ)].resource != nullptr;
    publication_reason = !nrd_dispatch_recorded ? 1u
                         : !slot.nrd.hit_validity ? 5u
                         : !slot.nrd_diffuse_publication.texture12 ? 6u
                         : !publication_out_diff ? 7u
                         : !publication_view_z ? 8u
                         : 0u;
    if (publication_reason == 0u) {
      publication_reason = 2u;
      if (!transfer.nrd_publication_root || !transfer.nrd_publication_pso) {
        Rt12NrdCreateDiffusePublicationPipeline(s.device12.Get(), __world_rtxdi_nrd_publish.data(),
                                                __world_rtxdi_nrd_publish.size(), &transfer.nrd_publication_root,
                                                &transfer.nrd_publication_pso);
      }
      if (transfer.nrd_publication_root && transfer.nrd_publication_pso) {
        publication_reason = 3u;
        Rt12NrdTexture status_texture;
        status_texture.resource = slot.n1_status.texture12;
        status_texture.format = DXGI_FORMAT_R32_UINT;
        status_texture.width = in.width;
        status_texture.height = in.height;
        status_texture.state = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        status_texture.unordered_access_written = true;
        Rt12NrdTexture hit_validity;
        hit_validity.resource = slot.nrd.hit_validity;
        hit_validity.width = in.width;
        hit_validity.height = in.height;
        hit_validity.state = D3D12_RESOURCE_STATE_COMMON;
        Rt12NrdTexture publication;
        publication.resource = slot.nrd_diffuse_publication.texture12;
        publication.format = DXGI_FORMAT_R16G16B16A16_FLOAT;
        publication.width = in.width;
        publication.height = in.height;
        publication.state = D3D12_RESOURCE_STATE_COMMON;
        slot.nrd.diffuse_publication_recorded = Rt12NrdRecordDiffusePublication(
            s.device12.Get(), transfer.di_list.Get(), transfer.nrd_publication_root.Get(),
            transfer.nrd_publication_pso.Get(), slot.nrd.publication_descriptors.Get(),
            slot.nrd.publication_constants.Get(),
            &slot.nrd.resources[static_cast<uint32_t>(nrd::ResourceType::OUT_DIFF_RADIANCE_HITDIST)],
            &slot.nrd.resources[static_cast<uint32_t>(nrd::ResourceType::IN_VIEWZ)], &status_texture,
            &hit_validity, &publication, in.width, in.height);
      }
    }
    if (perf_timing && nrd_dispatch_recorded && slot.nrd.diffuse_publication_recorded) {
      transfer.di_list->EndQuery(slot.perf_queries.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 7u);
      perf_nrd_recorded = true;
    }
#endif
    nrd_gate_link(rtxdi::NrdGateLink::Publication, slot.nrd.diffuse_publication_recorded, publication_reason);
    nrd_history_reset = !nrd_dispatch_recorded;
  } else {
    slot.nrd.dispatch_ready = false;
    nrd_history_reset = true;
  }
#endif
  barriers.clear();
  UavBarrier(barriers, s.di.reservoirs.Get());
  UavBarrier(barriers, slot.di_results.Get());
  UavBarrier(barriers, slot.di_resolved.texture12.Get());
  if (n1_ready) {
    UavBarrier(barriers, slot.n1_diffuse.texture12.Get());
    UavBarrier(barriers, slot.n1_specular.texture12.Get());
    UavBarrier(barriers, slot.n1_status.texture12.Get());
    Transition(barriers, slot.n1_diffuse.texture12.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON);
    Transition(barriers, slot.n1_specular.texture12.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON);
    Transition(barriers, slot.n1_status.texture12.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON);
  }
  Transition(barriers, s.di.reservoirs.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON);
  Transition(barriers, slot.di_results.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON);
  Transition(barriers, slot.di_resolved.texture12.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON);
  for (ID3D12Resource* resource : {vertices, indices, mesh_table, scene.instance_table.Get(), uv_arena, s.atlas.texture12.Get(),
                                   s.materials.Get(), slot.lights_buffer.Get(), slot.light_types.Get(), slot.color.texture12.Get(),
                                   slot.mrt2.texture12.Get(), slot.deferred_buffer.Get(), dynamic_vertices, slot.depth.texture12.Get(),
                                   slot.normal.texture12.Get(), slot.scene.texture12.Get(), slot.motion.texture12.Get(), s.noise.texture12.Get()}) {
    if (resource) Transition(barriers, resource, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COMMON);
  }
  Transition(barriers, slot.tiles_buffer.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COMMON);
  if (history_valid) {
    for (ID3D12Resource* resource : history_resources) {
      if (resource) Transition(barriers, resource, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COMMON);
    }
  }
  if (diag_armed) Transition(barriers, slot.diag_counters.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
  transfer.di_list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
  if (diag_armed) {
    transfer.di_list->CopyBufferRegion(slot.diag_readback.Get(), 0u, slot.diag_counters.Get(), 0u, RTXDI_DIAG_COUNTER_BYTES);
    barriers.clear();
    Transition(barriers, slot.diag_counters.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COMMON);
    transfer.di_list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
  }
  if (perf_timing) {
    transfer.di_list->EndQuery(slot.perf_queries.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 3u);
    transfer.di_list->ResolveQueryData(slot.perf_queries.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 2u, 2u,
                                       slot.perf_readback.Get(), 16u);
    if (perf_distance_recorded) {
      transfer.di_list->ResolveQueryData(slot.perf_queries.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 4u, 2u,
                                         slot.perf_readback.Get(), 32u);
    }
    if (perf_nrd_recorded) {
      transfer.di_list->ResolveQueryData(slot.perf_queries.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 6u, 2u,
                                         slot.perf_readback.Get(), 48u);
    }
  }
  hr = transfer.di_list->Close();
  if (FAILED(hr)) return publish(rtxdi::DiStatus::Failed, hr);

  ID3D12CommandList* lists[] = {transfer.di_list.Get()};
  uint64_t di_fence = renodx::utils::gpu_fence::NO_SIGNAL;
  s.unfenced_execution = true;
  try {
    s.queue12->ExecuteCommandLists(1u, lists);
    di_fence = renodx::utils::gpu_fence::QueueSignal(s.queue12.Get(), s.fence12.get());
    if (di_fence == renodx::utils::gpu_fence::NO_SIGNAL) throw 0;
    if (rtxdi::ShouldCountAcceptedGpuSubmission(
            bvh::g_perf_gate.gather.load(std::memory_order_relaxed),
            bvh::g_perf_gate.gather_epoch.load(std::memory_order_relaxed) == perf_epoch, di_fence,
            renodx::utils::gpu_fence::NO_SIGNAL)) {
      bvh::g_perf_gate.rtxdi_submissions.fetch_add(1u, std::memory_order_relaxed);
    }
  } catch (...) {
    transfer.input_frame = UINT32_MAX;
    if (rtxdi::DiPostSubmitSignalDisposition(true, false) == rtxdi::DiSignalDisposition::QuarantineSession) {
      FailSession(s, Rt12Reason::SubmitFailed, "rtxdi initial DI signal");
      return publish(rtxdi::DiStatus::Quarantined, E_FAIL);
    }
    return publish(rtxdi::DiStatus::Failed, E_FAIL);
  }
  s.unfenced_execution = false;
  slot.di_fence_value = di_fence;
  if (perf_timing) {
    slot.perf_fence = di_fence;
    slot.perf_valid_mask |= 2u;
    if (perf_distance_recorded) slot.perf_valid_mask |= 4u;
    if (perf_nrd_recorded) slot.perf_valid_mask |= 8u;
  }
  if (diag_armed) {
    slot.diag_pending = true;
    slot.diag_fence = di_fence;
    slot.diag_frame = in.frame;
    slot.diag_epoch = diagnostic_epoch;
    slot.diag_flags = (n1_ready ? RTXDI_DIAG_SAMPLE_N1 : 0u) | (in.effective_settings.spatial_enabled ? RTXDI_DIAG_SAMPLE_SPATIAL : 0u)
                      | (history_valid && in.effective_settings.temporal_enabled ? RTXDI_DIAG_SAMPLE_TEMPORAL : 0u)
                      | (history_valid ? RTXDI_DIAG_SAMPLE_HISTORY : 0u);
    transfer.diag_next_frame = in.frame + 30u;
  }
#if defined(FALCOM_NRD_ENABLED)
  if (nrd_probe_recorded) {
    slot.nrd.probe_fence = di_fence;
    slot.nrd.probe_frame = in.frame;
    slot.nrd.probe_pending = true;
  } else {
    slot.nrd.probe_pending = false;
  }
  if (nrd_dispatch_recorded) {
    slot.nrd.dispatch_ready = true;
    transfer.nrd.history = nrd_accepted_history;
    slot.nrd.diffuse_output_valid = true;
    slot.nrd.fence_value = di_fence;
  } else if (nrd_history_reset) {
    transfer.nrd.history = {};
    slot.nrd.diffuse_output_valid = false;
  }
#endif
  if (previous_slot) previous_slot->di_fence_value = (std::max)(previous_slot->di_fence_value, di_fence);
  slot.fence_value = (std::max)(slot.fence_value, di_fence);
  scene.fence_value = (std::max)(scene.fence_value, di_fence);
  transfer.input_fence = di_fence;
  transfer.scene_fence = di_fence;
  s.last_signal = di_fence;
  NoteRt12Signal(s, di_fence);
  bool output_b_wait_accepted = false;
  try {
    output_b_wait_accepted = renodx::utils::gpu_fence::QueueWait(s.ctx11.Get(), s.fence12.get(), di_fence);
  } catch (...) {
    output_b_wait_accepted = false;
  }
  if (!output_b_wait_accepted) {
    transfer.input_frame = UINT32_MAX;
    FailSession(s, Rt12Reason::SubmitFailed, "rtxdi output signal B");
    return publish(rtxdi::DiStatus::Quarantined, E_FAIL, di_fence);
  }
  slot.output_b_fence = di_fence;
  slot.output_c_fence = 0u;
  slot.output_owner_generation = s.generation;
  slot.output_configuration_generation = s.di.configuration_generation;
  slot.output_frame = in.frame;
  slot.output_width = in.width;
  slot.output_height = in.height;
  slot.output_scene_slot = in.scene_slot;
  slot.output_transfer_slot = input.transfer_slot;
  slot.output_staged = true;
  slot.n1_staged = n1_ready;
#if defined(FALCOM_NRD_ENABLED)
  if (slot.debug_distance_identity.recorded) {
    slot.debug_distance_identity.frame = in.frame;
    slot.debug_distance_identity.width = in.width;
    slot.debug_distance_identity.height = in.height;
    slot.debug_distance_identity.n1_configuration_version = slot.n1_configuration_version;
    slot.debug_distance_identity.scene_slot = in.scene_slot;
    slot.debug_distance_identity.transfer_slot = input.transfer_slot;
    slot.debug_distance_identity.owner_generation = s.generation;
    slot.debug_distance_identity.transfer_configuration = transfer.configuration_generation;
    slot.debug_distance_identity.di_configuration = s.di.configuration_generation;
    slot.debug_distance_identity.settings_identity = in.effective_settings.identity;
    slot.debug_distance_identity.producer_fence_b = di_fence;
    slot.debug_distance_identity.n1_brdf_config = slot.n1_brdf_config;
    slot.debug_distance_identity.staged = true;
  }
  slot.nrd_diffuse_staged = nrd_dispatch_recorded && slot.nrd.diffuse_publication_recorded
                            && slot.nrd_diffuse_publication.srv11.handle != 0u;
  nrd_gate_link(rtxdi::NrdGateLink::Srv, slot.nrd_diffuse_publication.srv11.handle != 0u, 1u);
  slot.nrd_diffuse_identity = {};
  if (slot.nrd_diffuse_staged) {
    slot.nrd_diffuse_identity = {
        .owner_generation = s.generation,
        .transfer_configuration = transfer.configuration_generation,
        .di_configuration = s.di.configuration_generation,
        .n1_configuration = Rt12NrdN1ConfigurationIdentity(slot.n1_configuration_version, slot.n1_brdf_config),
        .nrd_configuration = Rt12NrdConfigurationIdentity(transfer.configuration_generation,
                                                          s.di.configuration_generation,
                                                          in.effective_settings.identity,
                                                          transfer.nrd.qualification.motion_correction_index),
        .producer_fence_b = di_fence,
        .frame = in.frame,
        .width = in.width,
        .height = in.height,
        .scene_slot = in.scene_slot,
        .transfer_slot = input.transfer_slot,
        .accepted = true,
    };
  }
#endif
  temporal_ran = history_valid && in.effective_settings.temporal_enabled;
  s.di.last_use_fence = di_fence;
  s.di.history_valid = true;
  s.di.history_frame = in.frame;
  s.di.history_width = in.width;
  s.di.history_height = in.height;
  s.di.history_transfer_slot = input.transfer_slot;
  s.di.history_scene_slot = in.scene_slot;
  s.di.history_owner_generation = s.generation;
  s.di.history_configuration_generation = s.di.configuration_generation;
  s.di.context->SetFrameIndex(s.di.context->GetFrameIndex() + 1u);
  spatial_ran = in.effective_settings.spatial_enabled;
  spatial_source_index = s.di.frame.parameters.di.bufferIndices.spatialResamplingInputBufferIndex;
  spatial_output_index = s.di.frame.parameters.di.bufferIndices.spatialResamplingOutputBufferIndex;
  return publish(light_count == 0u ? rtxdi::DiStatus::ZeroLights : rtxdi::DiStatus::Submitted, 0, di_fence);
}

}  // namespace falcom_world::rt12
