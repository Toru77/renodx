#pragma once

#include <bit>

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
  int32_t scene_slot = -1;
  uint64_t scene_fence = 0u;
  bool local = false, sun = false;
};

struct Rt12DiInputResult {
  rtxdi::InputStatus status = rtxdi::InputStatus::MissingInput;
  rtxdi::InputStage stage = rtxdi::InputStage::Validate;
  HRESULT error = S_OK;
  RtxdiInputLayout layout = {};
  uint64_t fence = 0u;
  int32_t transfer_slot = -1;
};

enum class Rt12DiTextureKind { Float,
                               Motion,
                               Uint4,
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
    case Rt12DiTextureKind::Uint2:
      format_ok = desc.Format == DXGI_FORMAT_R16G16_UINT || desc.Format == DXGI_FORMAT_R32G32_UINT;
      break;
  }
  return format_ok && rtxdi::ValidateTextureView(view, width, height, desc.Format);
}

// Scene CBV identity is checked against the current binding before each stage.
inline bool CurrentDiSceneRange(reshade::api::device* device, reshade::api::command_list* cmd_list,
                                const Rt12DiInputViews& in, ID3D11Buffer* expected_scene, reshade::api::buffer_range* range) {
  if (!device || !cmd_list || !in.scene_tracker || !range) return false;
  const uintptr_t command = reinterpret_cast<uintptr_t>(cmd_list);
  rtxdi::SceneRangeSnapshot promoted, binding;
  {
    std::shared_lock lock(in.scene_tracker->mutex);
    promoted = in.scene_tracker->snapshot;
    const auto found = in.scene_tracker->bindings.find(command);
    if (found == in.scene_tracker->bindings.end()) return false;
    binding = found->second;
  }
  if (!promoted.live || !binding.live || promoted.buffer == 0u
      || reinterpret_cast<uint64_t>(expected_scene) != promoted.buffer) return false;
  const auto desc = device->get_resource_desc(reshade::api::resource{promoted.buffer});
  if (desc.type != reshade::api::resource_type::buffer
      || rtxdi::CheckPromotedSceneBinding(promoted, binding, in.capture_frame, command, desc.buffer.size)
             != rtxdi::SceneBindingStatus::Ready
      || !rtxdi::ValidateSceneBuffer(reinterpret_cast<ID3D11Buffer*>(promoted.buffer), promoted.offset, RTXDI_INPUT_SCENE_ROWS * 16u)) return false;  // NOLINT(performance-no-int-to-ptr)
  {
    std::shared_lock lock(in.scene_tracker->mutex);
    const auto found = in.scene_tracker->bindings.find(command);
    if (found == in.scene_tracker->bindings.end()
        || rtxdi::CheckPromotedSceneBinding(in.scene_tracker->snapshot, found->second, in.capture_frame, command, desc.buffer.size)
               != rtxdi::SceneBindingStatus::Ready
        || in.scene_tracker->snapshot.sequence != promoted.sequence) return false;
  }
  *range = {reshade::api::resource{promoted.buffer}, promoted.offset, RTXDI_INPUT_SCENE_ROWS * 16u};
  return true;
}

inline bool ValidateBoundDiScene(ID3D11DeviceContext* context, const reshade::api::buffer_range& range) {
  if (!context || range.buffer.handle == 0u) return false;
  Microsoft::WRL::ComPtr<ID3D11DeviceContext1> context1;
  if (FAILED(context->QueryInterface(IID_PPV_ARGS(&context1)))) return false;
  Microsoft::WRL::ComPtr<ID3D11Buffer> buffer;
  UINT first = 0u, count = 0u;
  context1->PSGetConstantBuffers1(0u, 1u, &buffer, &first, &count);
  return reinterpret_cast<uint64_t>(buffer.Get()) == range.buffer.handle
         && uint64_t{first} * 16u == range.offset && count >= RTXDI_INPUT_SCENE_ROWS;
}

inline Rt12DiInputResult TransferRtxdiInputs(reshade::api::device* device, reshade::api::command_list* cmd_list,
                                             const Rt12DiInputViews& in) {
  using DS = reshade::api::shader_stage;
  using DT = reshade::api::descriptor_type;
  using Microsoft::WRL::ComPtr;
  Rt12DiInputResult result;
  try {
    if (!g_rt12 || g_rt12->device != device || !cmd_list) return result;
    Rt12State& s = *g_rt12;
    if (s.failed || s.stuck || s.unfenced_execution || in.scene_slot < 0
        || in.scene_slot >= static_cast<int32_t>(kRt12Slots)
        || !rtxdi::ValidSceneLease(s.scene_slot, s.scene_frame, in.scene_slot, in.frame, s.scene_fence, in.scene_fence)
        || (in.sun && !rtxdi::SceneCoverageSnapshotMatches(s.scene_slot, s.scene_frame, s.scene_generation, s.scene_fence, s.scene_gc_version, s.scene_coverage_slot, s.scene_coverage_frame, s.scene_coverage_generation, s.scene_coverage_fence, s.scene_coverage_gc_version))
        || reinterpret_cast<ID3D11DeviceContext*>(cmd_list->get_native()) != s.ctx11.Get()) return result;  // NOLINT(performance-no-int-to-ptr)
    auto& transfer = s.di_transfer;
    if (rtxdi::TransferAttemptDuplicate(transfer.attempted_frame, transfer.attempted_generation, in.frame, s.generation)) {
      result.status = rtxdi::InputStatus::AlreadyAttempted;
      return result;
    }
    transfer.attempted_frame = in.frame;
    transfer.attempted_generation = s.generation;
    transfer.input_frame = UINT32_MAX;
    if (!in.width || !in.height) return result;
    ComPtr<ID3D11Buffer> current_scene;
    s.ctx11->PSGetConstantBuffers(0u, 1u, &current_scene);
    reshade::api::buffer_range scene_range = {};
    if (!CurrentDiSceneRange(device, cmd_list, in, current_scene.Get(), &scene_range)
        || !ValidateBoundDiScene(s.ctx11.Get(), scene_range)) {
      result.status = rtxdi::InputStatus::SceneBindingChanged;
      return result;
    }
    if (!ValidateDiTexture(in.depth, in.width, in.height, Rt12DiTextureKind::Float)
        || !ValidateDiTexture(in.normal, in.width, in.height, Rt12DiTextureKind::Uint4)) return result;
    const bool motion_available = in.motion.handle != 0u
                                  && ValidateDiTexture(in.motion, in.width, in.height, Rt12DiTextureKind::Motion);
    if (in.local && (!ValidateDiTexture(in.color, in.width, in.height, Rt12DiTextureKind::Float) || !ValidateDiTexture(in.mrt1, in.width, in.height, Rt12DiTextureKind::Uint4) || !ValidateDiTexture(in.mrt2, in.width, in.height, Rt12DiTextureKind::Uint2))) return result;
    rtxdi::StructuredInputRange lights = {}, tiles = {}, deferred = {};
    if (in.local && ((in.lights.handle && !rtxdi::ValidateStructuredView(reinterpret_cast<ID3D11ShaderResourceView*>(in.lights.handle), RTXDI_INPUT_LIGHT_STRIDE, &lights)) || !rtxdi::ValidateStructuredView(reinterpret_cast<ID3D11ShaderResourceView*>(in.tiles.handle), RTXDI_INPUT_TILE_STRIDE, &tiles) || !rtxdi::ValidateStructuredView(reinterpret_cast<ID3D11ShaderResourceView*>(in.deferred.handle), RTXDI_INPUT_DEFERRED_STRIDE, &deferred))) return result;  // NOLINT(performance-no-int-to-ptr)
    if (!rtxdi::MakeInputLayout(in.width, in.height, motion_available, in.local, lights.element_count, tiles.element_count, deferred.element_count, &result.layout)) return result;
    const uint32_t k = transfer.next_slot;
    Rt12DiTransferSlot& slot = transfer.slots[k];
    if (!rtxdi::TransferSlotAvailable((std::max)(slot.fence_value, slot.di_fence_value), Rt12CompletedValue(s))) {
      result.status = rtxdi::InputStatus::Busy;
      return result;
    }
    const uint64_t consumer_completed = (s.fence11 ? s.fence11->GetCompletedValue() : 0u);
    if (!rtxdi::CanReuseDiOutput(slot.output_staged, slot.output_c_fence, consumer_completed)) {
      result.status = rtxdi::InputStatus::Busy;
      return result;
    }
    slot.output_staged = false;
    slot.output_b_fence = 0u;
    slot.output_c_fence = 0u;
    result.stage = rtxdi::InputStage::Allocate;
    if (transfer.owner_generation != 0u && transfer.owner_generation != s.generation) {
      s.failed = true;
      s.stuck = true;
      result.status = rtxdi::InputStatus::Quarantined;
      result.error = E_FAIL;
      UpdatePanel([](Rt12Panel& p) { p.stuck = true; });
      return result;
    }
    if (in.local && !EnsureRt12DiResources(s, true, in.width, in.height)) {
      result.status = rtxdi::InputStatus::AllocationFailed;
      result.error = E_FAIL;
      return result;
    }
    // Each slot is replaced only after its B fence completes.
    if (!slot.allocator || slot.width != in.width || slot.height != in.height || slot.local != in.local
        || slot.motion_available != motion_available) {
      if (transfer.configuration_generation == UINT64_MAX) {
        result.status = rtxdi::InputStatus::AllocationFailed;
        return result;
      }
      Rt12DiTransferSlot candidate;
      auto destroy_candidate = [&](Rt12DiTransferSlot*) {
        for (Rt12Shared* shared : {&candidate.depth, &candidate.normal, &candidate.scene, &candidate.color, &candidate.mrt1,
                                   &candidate.mrt2, &candidate.motion, &candidate.lights, &candidate.tiles, &candidate.deferred,
                                   &candidate.di_resolved}) {
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
      }
      if (ok && motion_available) ok = make_shared(&candidate.motion, DXGI_FORMAT_R32G32B32A32_FLOAT, in.width, in.height);
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
      heap.NumDescriptors = 35u;
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
        ok = rtgi::EnsureRtgiPipeline(device, &candidate.pack_inputs, {.srv_count = 6u, .uav_count = 7u, .sampler_count = 0u, .push_count = 4u, .code = __world_rtxdi_pack_inputs, .name = "rtxdi input pack"});
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
        return result;
      }
      for (Rt12Shared* shared : {&slot.depth, &slot.normal, &slot.scene, &slot.color, &slot.mrt1, &slot.mrt2, &slot.motion,
                                 &slot.lights, &slot.tiles, &slot.deferred, &slot.di_resolved}) {
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
      candidate.motion_available = motion_available;
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
        return result;
      }
      const HRESULT output_hr = CreateRt12Buffer(s.device12.Get(), D3D12_HEAP_TYPE_DEFAULT,
                                                 output_bytes, D3D12_RESOURCE_STATE_COMMON,
                                                 D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, slot.sun_results);
      if (FAILED(output_hr)) {
        result.status = rtxdi::InputStatus::AllocationFailed;
        result.error = output_hr;
        return result;
      }
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
        return result;
      }
      transfer.unpack_root = std::move(root);
      transfer.unpack_pso = std::move(pso);
    }
    if (in.sun && (!transfer.sun_root || !transfer.sun_pso)) {
      D3D12_DESCRIPTOR_RANGE ranges[6] = {};
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
      D3D12_ROOT_PARAMETER params[2] = {};
      params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
      params[0].Constants.Num32BitValues = 12u;
      params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
      params[1].DescriptorTable.NumDescriptorRanges = 6u;
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
        return result;
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
        return result;
      }
      transfer.list = std::move(list);
    }
    if (slot.fence_value != 0u) {
      try {
        if (!renodx::utils::gpu_fence::QueueWait(s.ctx11.Get(), s.fence12.get(), slot.fence_value)) {
          result.status = rtxdi::InputStatus::SubmitFailed;
          result.error = E_FAIL;
          return result;
        }
      } catch (...) {
        result.status = rtxdi::InputStatus::SubmitFailed;
        result.error = E_FAIL;
        return result;
      }
    }
    if (!CurrentDiSceneRange(device, cmd_list, in, current_scene.Get(), &scene_range)
        || !ValidateBoundDiScene(s.ctx11.Get(), scene_range)) {
      result.status = rtxdi::InputStatus::SceneBindingChanged;
      return result;
    }
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
    const uint32_t pixel_params[4] = {in.width, in.height, in.local ? 1u : 0u, motion_available ? 1u : 0u};
    cmd_list->push_constants(DS::all_compute, slot.pack_inputs.layout, 3u, 0u, 4u, pixel_params);
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
      return result;
    }
    if (!CurrentDiSceneRange(device, cmd_list, in, slot.source_scene.Get(), &scene_range)
        || !ValidateBoundDiScene(s.ctx11.Get(), scene_range)) {
      result.status = rtxdi::InputStatus::SceneBindingChanged;
      return result;
    }
    result.stage = rtxdi::InputStage::Record;
    HRESULT hr = slot.allocator->Reset();
    if (SUCCEEDED(hr)) {
      hr = transfer.list->Reset(slot.allocator.Get(), transfer.unpack_pso.Get());
    }
    if (FAILED(hr)) {
      result.status = rtxdi::InputStatus::SubmitFailed;
      result.error = hr;
      return result;
    }
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
      D3D12_UNORDERED_ACCESS_VIEW_DESC result_uav = {};
      result_uav.Format = DXGI_FORMAT_UNKNOWN;
      result_uav.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
      result_uav.Buffer.NumElements = static_cast<UINT>(pixels);
      result_uav.Buffer.StructureByteStride = RTXDI_SUN_RESULT_STRIDE;
      native->CreateUnorderedAccessView(slot.sun_results.Get(), nullptr, &result_uav, query_cpu(20u));

      std::vector<D3D12_RESOURCE_BARRIER> query_barriers;
      query_barriers.reserve(16u);
      for (ID3D12Resource* resource : {slot.depth.texture12.Get(), slot.normal.texture12.Get(), vertices, indices,
                                       mesh_table, scene.instance_table.Get(), uv_arena,
                                       s.atlas.texture12.Get(), s.materials.Get(), dynamic_vertices}) {
        if (resource) Transition(query_barriers, resource, D3D12_RESOURCE_STATE_COMMON,
                                 D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
      }
      Transition(query_barriers, slot.sun_results.Get(), D3D12_RESOURCE_STATE_COMMON,
                 D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
      transfer.list->ResourceBarrier(static_cast<UINT>(query_barriers.size()), query_barriers.data());
      ID3D12DescriptorHeap* query_heaps[] = {slot.descriptors.Get()};
      transfer.list->SetDescriptorHeaps(1u, query_heaps);
      transfer.list->SetComputeRootSignature(transfer.sun_root.Get());
      transfer.list->SetPipelineState(transfer.sun_pso.Get());
      const float maximum_distance = rtsun::g_rtsun_ray_max;
      const float environment_bias = rtsun::g_rtsun_normal_bias;
      const float character_bias = rtsun::g_rtsun_character_normal_bias;
      const uint32_t params[12] = {in.width, in.height, std::bit_cast<uint32_t>(maximum_distance),
                                   std::bit_cast<uint32_t>(environment_bias), std::bit_cast<uint32_t>(character_bias),
                                   std::bit_cast<uint32_t>(s.scene_coverage_min[0]),
                                   std::bit_cast<uint32_t>(s.scene_coverage_min[1]),
                                   std::bit_cast<uint32_t>(s.scene_coverage_min[2]),
                                   std::bit_cast<uint32_t>(s.scene_coverage_size),
                                   scene_resources_valid ? 1u : 0u, s.scene_coverage_valid ? 1u : 0u, 0u};
      transfer.list->SetComputeRoot32BitConstants(0u, 12u, params, 0u);
      auto query_gpu = slot.descriptors->GetGPUDescriptorHandleForHeapStart();
      query_gpu.ptr += static_cast<UINT64>(8u) * s.descriptor_size;
      transfer.list->SetComputeRootDescriptorTable(1u, query_gpu);
      transfer.list->Dispatch((in.width + 7u) / 8u, (in.height + 7u) / 8u, 1u);
      query_barriers.clear();
      UavBarrier(query_barriers, slot.sun_results.Get());
      for (ID3D12Resource* resource : {slot.depth.texture12.Get(), slot.normal.texture12.Get(), vertices, indices,
                                       mesh_table, scene.instance_table.Get(), uv_arena,
                                       s.atlas.texture12.Get(), s.materials.Get(), dynamic_vertices}) {
        if (resource) Transition(query_barriers, resource, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                                 D3D12_RESOURCE_STATE_COMMON);
      }
      Transition(query_barriers, slot.sun_results.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
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
    hr = transfer.list->Close();
    if (FAILED(hr)) {
      result.status = rtxdi::InputStatus::SubmitFailed;
      result.error = hr;
      return result;
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
      return result;
    }
    ID3D12CommandList* lists[] = {transfer.list.Get()};
    uint64_t b = renodx::utils::gpu_fence::NO_SIGNAL;
    result.stage = rtxdi::InputStage::Submit;
    s.unfenced_execution = true;
    try {
      s.queue12->ExecuteCommandLists(1u, lists);
      b = renodx::utils::gpu_fence::QueueSignal(s.queue12.Get(), s.fence12.get());
      if (b == renodx::utils::gpu_fence::NO_SIGNAL) throw 0;
      slot.fence_value = b;
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
      return result;
    }
    NoteRt12Signal(s, b);
    return result;
  } catch (...) {
    if (result.status != rtxdi::InputStatus::Ready && result.status != rtxdi::InputStatus::Quarantined) {
      result.status = rtxdi::InputStatus::SubmitFailed;
      result.error = E_FAIL;
    }
    return result;
  }
}

inline rtxdi::DiStatus DispatchRtxdiInitialDi(reshade::api::device* device, reshade::api::command_list* cmd_list,
                                              const Rt12DiInputViews& in, const Rt12DiInputResult& input) {
  using Microsoft::WRL::ComPtr;
  if (!g_rt12 || g_rt12->device != device || !cmd_list || !in.local || input.status != rtxdi::InputStatus::Ready
      || input.transfer_slot < 0 || input.transfer_slot >= static_cast<int32_t>(kRt12Slots)) {
    return rtxdi::DiStatus::WaitingInput;
  }
  Rt12State& s = *g_rt12;
  auto& transfer = s.di_transfer;
  const uint32_t slot_index = static_cast<uint32_t>(input.transfer_slot);
  Rt12DiTransferSlot& slot = transfer.slots[slot_index];
  bool spatial_ran = false;
  bool temporal_ran = false;
  bool history_valid = false;
  uint32_t spatial_source_index = UINT32_MAX;
  uint32_t spatial_output_index = UINT32_MAX;
  uint32_t temporal_source_index = UINT32_MAX;
  uint32_t temporal_output_index = UINT32_MAX;
  const auto publish = [&](rtxdi::DiStatus status, int32_t error = 0, uint64_t fence = 0u) {
    std::unique_lock lock(rtxdi::g_di.mutex);
    rtxdi::g_di.snapshot = {.status = status,
                            .frame = in.frame,
                            .configuration_generation = s.di.configuration_generation,
                            .fence = fence,
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
                            .output_bound = false,
                            .output_bound_frame = UINT32_MAX,
                            .output_bound_transfer_slot = -1,
                            .output_bound_fence_b = 0u,
                            .lighting_draw_seen = false,
                            .error = error};
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
      .reservoir_available = rtxdi::TransferSlotAvailable(s.di.last_use_fence, Rt12CompletedValue(s)),
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
  if (!s.di.context || !slot.local || slot.width != in.width || slot.height != in.height || !slot.di_allocator
      || !slot.di_descriptors || !slot.light_types || !slot.di_results || !slot.di_frame_upload) {
    return publish(rtxdi::DiStatus::Failed, E_FAIL);
  }
  ComPtr<ID3D11Buffer> current_scene;
  s.ctx11->PSGetConstantBuffers(0u, 1u, &current_scene);
  reshade::api::buffer_range scene_range = {};
  if (!CurrentDiSceneRange(device, cmd_list, in, current_scene.Get(), &scene_range)
      || !ValidateBoundDiScene(s.ctx11.Get(), scene_range)) {
    return publish(rtxdi::DiStatus::WaitingInput, E_FAIL);
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
  s.di.frame.parameters.lights.localLightBufferRegion = {0u, light_count, 0u, 0u};
  s.di.frame.parameters.lights.infiniteLightBufferRegion = {};
  s.di.frame.parameters.lights.environmentLightParams = {};
  if (!rtxdi::ValidTemporalSpatialBufferIndices(s.di.frame.parameters.di.bufferIndices)) {
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
    D3D12_DESCRIPTOR_RANGE ranges[9] = {};
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
    set_range(6u, D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 2u, 0u, 18u);
    set_range(7u, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 9u, 34u, 22u);
    set_range(8u, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2u, 43u, 31u);
    D3D12_ROOT_PARAMETER params[3] = {};
    params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    params[0].Constants.Num32BitValues = 17u;
    params[0].Constants.ShaderRegister = 0u;
    params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    params[1].Descriptor.ShaderRegister = 1u;
    params[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    params[2].DescriptorTable.NumDescriptorRanges = 9u;
    params[2].DescriptorTable.pDescriptorRanges = ranges;
    D3D12_ROOT_SIGNATURE_DESC desc = {};
    desc.NumParameters = 3u;
    desc.pParameters = params;
    ComPtr<ID3DBlob> blob, error;
    ComPtr<ID3D12RootSignature> root;
    ComPtr<ID3D12PipelineState> pso, temporal_pso, spatial_pso;
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
#else
    hr = E_FAIL;
#endif
    if (FAILED(hr)) return publish(rtxdi::DiStatus::Failed, hr);
    transfer.initial_di_root = std::move(root);
    transfer.initial_di_pso = std::move(pso);
    transfer.temporal_di_pso = std::move(temporal_pso);
    transfer.spatial_di_pso = std::move(spatial_pso);
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

  ID3D12DescriptorHeap* heaps[] = {slot.di_descriptors.Get()};
  transfer.di_list->SetDescriptorHeaps(1u, heaps);
  std::vector<D3D12_RESOURCE_BARRIER> barriers;
  barriers.reserve(32u);
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
                                   slot.mrt2.texture12.Get(), slot.deferred_buffer.Get(), dynamic_vertices, slot.depth.texture12.Get(),
                                   slot.normal.texture12.Get(), slot.scene.texture12.Get(), slot.motion.texture12.Get()}) {
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
  const uint32_t sun_params[17] = {in.width, in.height, scene_resources_valid ? 1u : 0u, s.scene_coverage_valid ? 1u : 0u,
                                   light_count, std::bit_cast<uint32_t>(rtsun::g_rtsun_normal_bias),
                                   std::bit_cast<uint32_t>(rtsun::g_rtsun_character_normal_bias),
                                   std::bit_cast<uint32_t>(0.001f),
                                   std::bit_cast<uint32_t>(s.scene_coverage_min[0]),
                                   std::bit_cast<uint32_t>(s.scene_coverage_min[1]),
                                   std::bit_cast<uint32_t>(s.scene_coverage_min[2]),
                                   std::bit_cast<uint32_t>(s.scene_coverage_size), input.layout.deferred_copied,
                                   history_slot.deferred_count, history_slot.light_count, tile_count, history_slot.tile_count};
  transfer.di_list->SetComputeRoot32BitConstants(0u, 17u, sun_params, 0u);
  transfer.di_list->SetComputeRootConstantBufferView(1u, slot.di_frame_upload->GetGPUVirtualAddress());
  transfer.di_list->SetComputeRootDescriptorTable(2u, slot.di_descriptors->GetGPUDescriptorHandleForHeapStart());
  transfer.di_list->Dispatch((in.width + 7u) / 8u, (in.height + 7u) / 8u, 1u);
  barriers.clear();
  UavBarrier(barriers, s.di.reservoirs.Get());
  transfer.di_list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
  if (history_valid) {
    transfer.di_list->SetPipelineState(transfer.temporal_di_pso.Get());
    transfer.di_list->Dispatch((in.width + 7u) / 8u, (in.height + 7u) / 8u, 1u);
    barriers.clear();
    UavBarrier(barriers, s.di.reservoirs.Get());
    transfer.di_list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
  }
  transfer.di_list->SetPipelineState(transfer.spatial_di_pso.Get());
  transfer.di_list->Dispatch((in.width + 7u) / 8u, (in.height + 7u) / 8u, 1u);
  barriers.clear();
  UavBarrier(barriers, s.di.reservoirs.Get());
  UavBarrier(barriers, slot.di_results.Get());
  UavBarrier(barriers, slot.di_resolved.texture12.Get());
  Transition(barriers, s.di.reservoirs.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON);
  Transition(barriers, slot.di_results.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON);
  Transition(barriers, slot.di_resolved.texture12.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON);
  for (ID3D12Resource* resource : {vertices, indices, mesh_table, scene.instance_table.Get(), uv_arena, s.atlas.texture12.Get(),
                                   s.materials.Get(), slot.lights_buffer.Get(), slot.light_types.Get(), slot.color.texture12.Get(),
                                   slot.mrt2.texture12.Get(), slot.deferred_buffer.Get(), dynamic_vertices, slot.depth.texture12.Get(),
                                   slot.normal.texture12.Get(), slot.scene.texture12.Get(), slot.motion.texture12.Get()}) {
    if (resource) Transition(barriers, resource, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COMMON);
  }
  Transition(barriers, slot.tiles_buffer.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COMMON);
  if (history_valid) {
    for (ID3D12Resource* resource : history_resources) {
      if (resource) Transition(barriers, resource, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COMMON);
    }
  }
  transfer.di_list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
  hr = transfer.di_list->Close();
  if (FAILED(hr)) return publish(rtxdi::DiStatus::Failed, hr);

  ID3D12CommandList* lists[] = {transfer.di_list.Get()};
  uint64_t di_fence = renodx::utils::gpu_fence::NO_SIGNAL;
  s.unfenced_execution = true;
  try {
    s.queue12->ExecuteCommandLists(1u, lists);
    di_fence = renodx::utils::gpu_fence::QueueSignal(s.queue12.Get(), s.fence12.get());
    if (di_fence == renodx::utils::gpu_fence::NO_SIGNAL) throw 0;
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
  temporal_ran = history_valid;
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
  spatial_ran = true;
  spatial_source_index = s.di.frame.parameters.di.bufferIndices.spatialResamplingInputBufferIndex;
  spatial_output_index = s.di.frame.parameters.di.bufferIndices.spatialResamplingOutputBufferIndex;
  return publish(light_count == 0u ? rtxdi::DiStatus::ZeroLights : rtxdi::DiStatus::Submitted, 0, di_fence);
}

}  // namespace falcom_world::rt12
