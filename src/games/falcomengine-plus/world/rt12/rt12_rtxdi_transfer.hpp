#pragma once

#include <algorithm>
#include <bit>
#include <chrono>
#include <cmath>
#include <numbers>

#include "../rr/rr_state.hpp"
#include "../game_lights.hpp"
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
  rtxdi::EffectiveRtxdiSettings effective_settings = {};
  int32_t scene_slot = -1;
  uint64_t scene_fence = 0u;
  bool local = false;
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
    if (reinterpret_cast<ID3D11DeviceContext*>(cmd_list->get_native()) != s.ctx11.Get()) {  // NOLINT(performance-no-int-to-ptr)
      forced_diagnostic_reason = rtxdi::DiagnosticReason::CommandMismatch;
      return finish();
    }
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
    slot.regir_source_identity = {};
    bool perf_timing = false;
    if (perf_gather && slot.perf_pending) {
      rtxdi::RecordGpuTimingSkipEvent(&bvh::g_perf_gate.rtxdi_timing_skip_events);
    }
    slot.output_b_fence = 0u;
    slot.output_c_fence = 0u;
    result.stage = rtxdi::InputStage::Allocate;
    if (transfer.owner_generation != 0u && transfer.owner_generation != s.generation) {
      for (Rt12DiTransferSlot& transfer_slot : transfer.slots) transfer_slot.regir_source_identity = {};
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
    if (!slot.allocator || slot.width != in.width || slot.height != in.height || slot.local != in.local
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
             }) {
          if (shared->srv11.handle) {
            device->destroy_resource_view(shared->srv11);
          }
          if (shared->uav11.handle) {
            device->destroy_resource_view(shared->uav11);
          }
        }
        denoise::DestroyComputePipeline(device, candidate.pack_inputs);
        denoise::DestroyComputePipeline(device, candidate.pack_records);
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
        ok = make_buffer(uint64_t{RTXDI_INPUT_MAX_LIGHTS} * sizeof(uint32_t), &candidate.light_types)
             && make_buffer(uint64_t{RTXDI_LIGHT_TABLE_CAPACITY} * sizeof(RtxdiLightTableEntry), &candidate.light_table)
             && make_buffer(uint64_t{RTXDI_LIGHT_TABLE_CAPACITY} * sizeof(uint32_t), &candidate.previous_to_current)
             && make_buffer(uint64_t{RTXDI_LIGHT_TABLE_CAPACITY} * sizeof(uint32_t), &candidate.current_to_previous);
        if (ok) {
          allocation_error = CreateRt12Buffer(s.device12.Get(), D3D12_HEAP_TYPE_UPLOAD,
                                              uint64_t{RTXDI_LIGHT_TABLE_CAPACITY} * 2u * sizeof(uint32_t),
                                              D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_FLAG_NONE,
                                              candidate.light_map_reset_upload);
          ok = SUCCEEDED(allocation_error);
          if (ok) {
            void* reset_data = nullptr;
            allocation_error = candidate.light_map_reset_upload->Map(0u, nullptr, &reset_data);
            ok = SUCCEEDED(allocation_error) && reset_data != nullptr;
            if (ok) {
              std::fill_n(static_cast<uint32_t*>(reset_data), RTXDI_LIGHT_TABLE_CAPACITY * 2u,
                          RTXDI_LIGHT_TABLE_INVALID_INDEX);
              candidate.light_map_reset_upload->Unmap(0u, nullptr);
            }
          }
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
        heap.NumDescriptors = 58u;
        allocation_error = s.device12->CreateDescriptorHeap(&heap, IID_PPV_ARGS(&candidate.di_descriptors));
        ok = SUCCEEDED(allocation_error);
      }
#if defined(__world_rtxdi_pack_inputs_EMBED_FILE) && defined(__world_rtxdi_pack_records_EMBED_FILE)
      if (ok) {
        ok = denoise::EnsureComputePipeline(device, &candidate.pack_inputs, {.srv_count = 6u, .uav_count = 7u, .sampler_count = 0u, .push_count = 5u, .code = __world_rtxdi_pack_inputs, .name = "rtxdi input pack"});
      }
      if (ok && in.local) {
        ok = denoise::EnsureComputePipeline(device, &candidate.pack_records, {.srv_count = 3u, .uav_count = 3u, .sampler_count = 0u, .push_count = 4u, .code = __world_rtxdi_pack_records, .name = "rtxdi record pack"});
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
           }) {
        if (shared->srv11.handle) {
          device->destroy_resource_view(shared->srv11);
        }
        if (shared->uav11.handle) {
          device->destroy_resource_view(shared->uav11);
        }
      }
      denoise::DestroyComputePipeline(device, slot.pack_inputs);
      denoise::DestroyComputePipeline(device, slot.pack_records);
      candidate.width = in.width;
      candidate.height = in.height;
      candidate.local = in.local;
      candidate.motion_available = motion_available;
      candidate.tile_count = 0u;
      slot = std::move(candidate);
      candidate_guard.release();
      for (Rt12DiTransferSlot& transfer_slot : transfer.slots) transfer_slot.regir_source_identity = {};
      transfer.owner_generation = s.generation;
      ++transfer.configuration_generation;
    }
    if (rtxdi::CanStartGpuTiming(perf_gather
                                     && perf_epoch == bvh::g_perf_gate.gather_epoch.load(std::memory_order_relaxed),
                                 s.timestamp_frequency, slot.perf_pending)) {
      if (!slot.perf_queries || !slot.perf_readback) {
        D3D12_QUERY_HEAP_DESC query_desc = {};
        query_desc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
        query_desc.Count = 2u * kRt12DiPerfSamples;
        D3D12_HEAP_PROPERTIES readback_heap = {};
        readback_heap.Type = D3D12_HEAP_TYPE_READBACK;
        D3D12_RESOURCE_DESC readback_desc = {};
        readback_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        readback_desc.Width = 16u * kRt12DiPerfSamples;
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
    const auto clear_tables = [&](const denoise::ComputePipeline& pipeline, uint32_t srv_count, uint32_t uav_count) {
      const reshade::api::buffer_range null_cb = {};
      const reshade::api::resource_view null_views[7] = {};
      const reshade::api::descriptor_table_update updates[3] = {
          {pipeline.cbv_table, 0, 0, 1u, DT::constant_buffer, &null_cb},
          {pipeline.srv_table, 0, 0, srv_count, DT::shader_resource_view, null_views},
          {pipeline.uav_table, 0, 0, uav_count, DT::unordered_access_view, null_views},
      };
      device->update_descriptor_tables(3u, updates);
    };
    const auto bind = [&](const denoise::ComputePipeline& pipeline, uint32_t srv_count, const reshade::api::resource_view* srvs,
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
    compute::NullComputeSlots(cmd_list, 6u, 7u, 0u);
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
      compute::NullComputeSlots(cmd_list, 3u, 3u, 0u);
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

inline rtxdi::DiStatus DispatchLightTable(reshade::api::device* device, reshade::api::command_list* cmd_list,
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
  const uint32_t slot_index = static_cast<uint32_t>(input.transfer_slot);
  Rt12DiTransferSlot& slot = transfer.slots[slot_index];
  slot.regir_source_identity = {};
  const bool perf_timing = bvh::g_perf_gate.gather.load(std::memory_order_relaxed)
                           && slot.perf_pending
                           && slot.perf_gather_epoch == bvh::g_perf_gate.gather_epoch.load(std::memory_order_relaxed)
                           && slot.perf_owner_generation == s.generation
                           && slot.perf_frame == in.frame && slot.perf_width == in.width && slot.perf_height == in.height
                           && slot.perf_settings_identity == in.effective_settings.identity;
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
                              .stable_light_remap = in.effective_settings.stable_light_remap,
                              .spatial_enabled = in.effective_settings.spatial_enabled,
                              .relax_selected = in.effective_settings.relax_diffuse,
                              .scene_slot = in.scene_slot,
                              .transfer_slot = input.transfer_slot,
                              .output_frame = slot.output_frame,
                              .output_width = slot.output_width,
                              .output_height = slot.output_height,
                              .output_owner_generation = slot.output_owner_generation,
                              .output_configuration_generation = slot.output_configuration_generation,
                              .output_fence_b = slot.output_b_fence,
                              .output_fence_c = slot.output_c_fence,
                              .output_scene_slot = slot.output_scene_slot,
                              .output_transfer_slot = slot.output_transfer_slot,
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
  if (s.di.settings_identity != in.effective_settings.identity || !slot.local
      || slot.width != in.width || slot.height != in.height || !slot.di_allocator
      || !slot.di_descriptors || !slot.light_types || !slot.light_table || !slot.previous_to_current
      || !slot.current_to_previous || !slot.light_map_reset_upload) {
    slot.light_table_status = RTXDI_LIGHT_TABLE_STATUS_RESOURCE_FAILED;
    slot.light_table_reason = RTXDI_LIGHT_TABLE_STATUS_RESOURCE_FAILED;
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
  slot.light_table_source = input.layout.lights_source;
  slot.light_table_truncated = input.layout.lights_truncated;
  slot.light_table_remap_enabled = in.effective_settings.stable_light_remap;
  Rt12DiTransferSlot* previous_slot = nullptr;
  bool history_valid = false;
  if (s.di.history_valid && s.di.history_transfer_slot >= 0 && s.di.history_transfer_slot < static_cast<int32_t>(kRt12Slots)) {
    Rt12DiTransferSlot& candidate = transfer.slots[static_cast<uint32_t>(s.di.history_transfer_slot)];
    const bool previous_inputs = candidate.local && candidate.width == in.width && candidate.height == in.height
                                 && candidate.depth.texture12 && candidate.normal.texture12 && candidate.scene.texture12
                                 && candidate.color.texture12 && candidate.mrt2.texture12 && candidate.deferred_buffer
                                 && candidate.lights_buffer && candidate.light_types && candidate.light_table;
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
    if (!slot.light_readback
        && FAILED(CreateRt12Buffer(s.device12.Get(), D3D12_HEAP_TYPE_READBACK,
                                   uint64_t{RTXDI_LIGHT_TABLE_CAPACITY} * sizeof(game_lights::LightParamCpu),
                                   D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_FLAG_NONE, slot.light_readback))) {
      slot.light_readback.Reset();
    }
    diag_armed = transfer.diag_zero && slot.diag_counters && slot.diag_readback;
  }

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
  if (!transfer.light_table_root || !transfer.light_table_pso) {
    D3D12_DESCRIPTOR_RANGE ranges[2] = {};
    ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    ranges[0].NumDescriptors = 5u;
    ranges[0].OffsetInDescriptorsFromTableStart = 0u;
    ranges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    ranges[1].NumDescriptors = 4u;
    ranges[1].OffsetInDescriptorsFromTableStart = 5u;
    D3D12_ROOT_PARAMETER params[2] = {};
    params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    params[0].Constants.Num32BitValues = 4u;
    params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    params[1].DescriptorTable.NumDescriptorRanges = 2u;
    params[1].DescriptorTable.pDescriptorRanges = ranges;
    D3D12_ROOT_SIGNATURE_DESC desc = {};
    desc.NumParameters = 2u;
    desc.pParameters = params;
    ComPtr<ID3DBlob> blob, error;
    ComPtr<ID3D12RootSignature> root;
    ComPtr<ID3D12PipelineState> pso;
    HRESULT table_hr = s.serialize(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &error);
    if (SUCCEEDED(table_hr)) {
      table_hr = s.device12->CreateRootSignature(0u, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&root));
    }
#if defined(__world_light_table_EMBED_FILE)
    if (SUCCEEDED(table_hr)) {
      D3D12_COMPUTE_PIPELINE_STATE_DESC pipeline = {};
      pipeline.pRootSignature = root.Get();
      pipeline.CS = {__world_light_table.data(), __world_light_table.size()};
      table_hr = s.device12->CreateComputePipelineState(&pipeline, IID_PPV_ARGS(&pso));
    }
#else
    table_hr = E_FAIL;
#endif
    if (FAILED(table_hr)) {
      slot.light_table_status = RTXDI_LIGHT_TABLE_STATUS_PIPELINE_FAILED;
      slot.light_table_reason = RTXDI_LIGHT_TABLE_STATUS_PIPELINE_FAILED;
      return publish(rtxdi::DiStatus::Failed, table_hr);
    }
    transfer.light_table_root = std::move(root);
    transfer.light_table_pso = std::move(pso);
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
  if (SUCCEEDED(hr)) hr = transfer.di_list->Reset(slot.di_allocator.Get(), transfer.classify_pso.Get());
  if (FAILED(hr)) return publish(rtxdi::DiStatus::Failed, hr);
  if (perf_timing) transfer.di_list->EndQuery(slot.perf_queries.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 2u);


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
  write_buffer_srv(33u, slot.tiles_buffer.Get(), RTXDI_INPUT_TILE_STRIDE, tile_count);
  write_buffer_uav(34u, slot.light_types.Get(), sizeof(uint32_t), RTXDI_INPUT_MAX_LIGHTS);
  Rt12DiTransferSlot& history_slot = (previous_slot ? *previous_slot : slot);
  D3D12_UNORDERED_ACCESS_VIEW_DESC diag_view = {};
  diag_view.Format = DXGI_FORMAT_R32_TYPELESS;
  diag_view.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
  diag_view.Buffer.NumElements = RTXDI_DIAG_COUNTER_COUNT;
  diag_view.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
  native->CreateUnorderedAccessView(diag_armed ? slot.diag_counters.Get() : nullptr, nullptr, &diag_view, cpu_at(39u));
  write_buffer_srv(49u, slot.lights_buffer.Get(), RTXDI_INPUT_LIGHT_STRIDE, light_count);
  write_buffer_srv(50u, slot.light_types.Get(), sizeof(uint32_t), RTXDI_LIGHT_TABLE_CAPACITY);
  write_buffer_srv(51u, history_slot.lights_buffer.Get(), RTXDI_INPUT_LIGHT_STRIDE, history_slot.light_count);
  write_buffer_srv(52u, history_slot.light_table.Get(), sizeof(RtxdiLightTableEntry), RTXDI_LIGHT_TABLE_CAPACITY);
  write_buffer_srv(53u, slot.light_table.Get(), sizeof(RtxdiLightTableEntry), RTXDI_LIGHT_TABLE_CAPACITY);
  write_buffer_uav(54u, slot.light_table.Get(), sizeof(RtxdiLightTableEntry), RTXDI_LIGHT_TABLE_CAPACITY);
  write_buffer_uav(55u, slot.previous_to_current.Get(), sizeof(uint32_t), RTXDI_LIGHT_TABLE_CAPACITY);
  write_buffer_uav(56u, slot.current_to_previous.Get(), sizeof(uint32_t), RTXDI_LIGHT_TABLE_CAPACITY);
  write_buffer_uav(57u, diag_armed ? slot.diag_counters.Get() : nullptr, sizeof(uint32_t), RTXDI_DIAG_COUNTER_COUNT);

  ID3D12DescriptorHeap* heaps[] = {slot.di_descriptors.Get()};
  transfer.di_list->SetDescriptorHeaps(1u, heaps);
  std::vector<D3D12_RESOURCE_BARRIER> barriers;
  barriers.reserve(48u);
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
  UavBarrier(barriers, slot.light_types.Get());
  Transition(barriers, slot.lights_buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
  Transition(barriers, slot.light_types.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
  Transition(barriers, slot.light_table.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
  Transition(barriers, slot.previous_to_current.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
  Transition(barriers, slot.current_to_previous.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
  if (history_valid && &history_slot != &slot) {
    Transition(barriers, history_slot.lights_buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    Transition(barriers, history_slot.light_table.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
  }
  transfer.di_list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
  transfer.di_list->CopyBufferRegion(slot.previous_to_current.Get(), 0u, slot.light_map_reset_upload.Get(), 0u,
                                     uint64_t{RTXDI_LIGHT_TABLE_CAPACITY} * sizeof(uint32_t));
  transfer.di_list->CopyBufferRegion(slot.current_to_previous.Get(), 0u, slot.light_map_reset_upload.Get(),
                                     uint64_t{RTXDI_LIGHT_TABLE_CAPACITY} * sizeof(uint32_t),
                                     uint64_t{RTXDI_LIGHT_TABLE_CAPACITY} * sizeof(uint32_t));
  barriers.clear();
  Transition(barriers, slot.previous_to_current.Get(), D3D12_RESOURCE_STATE_COPY_DEST,
             D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
  Transition(barriers, slot.current_to_previous.Get(), D3D12_RESOURCE_STATE_COPY_DEST,
             D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
  transfer.di_list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
  transfer.di_list->SetComputeRootSignature(transfer.light_table_root.Get());
  transfer.di_list->SetPipelineState(transfer.light_table_pso.Get());
  D3D12_GPU_DESCRIPTOR_HANDLE light_table_gpu = slot.di_descriptors->GetGPUDescriptorHandleForHeapStart();
  light_table_gpu.ptr += static_cast<UINT64>(49u) * s.descriptor_size;
  transfer.di_list->SetComputeRootDescriptorTable(1u, light_table_gpu);
  uint32_t light_table_constants[4] = {light_count, history_valid ? history_slot.light_count : 0u, history_valid ? 1u : 0u, 0u};
  transfer.di_list->SetComputeRoot32BitConstants(0u, 4u, light_table_constants, 0u);
  if (light_count != 0u) transfer.di_list->Dispatch((light_count + 63u) / 64u, 1u, 1u);
  barriers.clear();
  UavBarrier(barriers, slot.light_table.Get());
  Transition(barriers, slot.light_table.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
  transfer.di_list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
  light_table_constants[3] = 1u;
  transfer.di_list->SetComputeRoot32BitConstants(0u, 4u, light_table_constants, 0u);
  const uint32_t light_table_dispatch_count = (std::max)(light_count, history_valid ? history_slot.light_count : 0u);
  if (light_table_dispatch_count != 0u) transfer.di_list->Dispatch((light_table_dispatch_count + 63u) / 64u, 1u, 1u);
  barriers.clear();
  UavBarrier(barriers, slot.previous_to_current.Get());
  UavBarrier(barriers, slot.current_to_previous.Get());
  Transition(barriers, slot.previous_to_current.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
             D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
  Transition(barriers, slot.current_to_previous.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
             D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
  transfer.di_list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
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
  }
  hr = transfer.di_list->Close();
  if (FAILED(hr)) {
    slot.light_table_status = RTXDI_LIGHT_TABLE_STATUS_DISPATCH_FAILED;
    slot.light_table_reason = RTXDI_LIGHT_TABLE_STATUS_DISPATCH_FAILED;
    return publish(rtxdi::DiStatus::Failed, hr);
  }

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
    slot.light_table_status = RTXDI_LIGHT_TABLE_STATUS_DISPATCH_FAILED;
    slot.light_table_reason = RTXDI_LIGHT_TABLE_STATUS_DISPATCH_FAILED;
    transfer.input_frame = UINT32_MAX;
    if (rtxdi::DiPostSubmitSignalDisposition(true, false) == rtxdi::DiSignalDisposition::QuarantineSession) {
      FailSession(s, Rt12Reason::SubmitFailed, "rtxdi initial DI signal");
      return publish(rtxdi::DiStatus::Quarantined, E_FAIL);
    }
    return publish(rtxdi::DiStatus::Failed, E_FAIL);
  }
  s.unfenced_execution = false;
  slot.di_fence_value = di_fence;
  slot.regir_source_identity = {.owner_generation = s.generation,
                                .transfer_configuration = transfer.configuration_generation,
                                .table_fence = di_fence,
                                .frame = in.frame,
                                .transfer_slot = slot_index,
                                .light_count = light_count};
  slot.light_table_status = slot.light_table_truncated != 0u ? RTXDI_LIGHT_TABLE_STATUS_TRUNCATED
                               : !history_valid              ? RTXDI_LIGHT_TABLE_STATUS_NO_HISTORY
                               : light_count == 0u            ? RTXDI_LIGHT_TABLE_STATUS_ZERO_LIGHTS
                                                              : RTXDI_LIGHT_TABLE_STATUS_READY;
  slot.light_table_reason = slot.light_table_status;
  if (diag_armed) {
    slot.diag_pending = true;
    slot.diag_fence = di_fence;
    slot.diag_frame = in.frame;
    slot.diag_snapshot = {.frame = in.frame, .status = slot.light_table_status, .reason = slot.light_table_reason,
                          .source_count = slot.light_table_source, .copied_count = slot.light_count,
                          .truncated_count = slot.light_table_truncated, .fence = di_fence,
                          .remap_enabled = in.effective_settings.stable_light_remap};
    slot.diag_epoch = diagnostic_epoch;
    slot.diag_flags = history_valid ? RTXDI_DIAG_SAMPLE_HISTORY : 0u;
    transfer.diag_next_frame = in.frame + 30u;
  }
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
  return publish(light_count == 0u ? rtxdi::DiStatus::ZeroLights : rtxdi::DiStatus::Submitted, 0, di_fence);
}

}  // namespace falcom_world::rt12
