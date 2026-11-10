#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <mutex>
#include <shared_mutex>
#include <string>

namespace falcom_world::rtxdi {

inline constexpr uint32_t kDiagnosticTransitionCapacity = 64u;
inline constexpr uint32_t kDiagnosticFailureCapacity = 16u;
inline constexpr uint32_t kDiagnosticReasonCapacity = 32u;
inline constexpr uint32_t kDiagnosticIdentityCapacity = 128u;
inline constexpr uint32_t kDiagnosticReportCapacity = 65536u;

enum DiagnosticAvailability : uint32_t {
  DiagnosticFrameAvailable = 1u << 0u,
  DiagnosticStatesAvailable = 1u << 1u,
  DiagnosticInputIdentityAvailable = 1u << 2u,
  DiagnosticDimensionsAvailable = 1u << 3u,
  DiagnosticSceneFenceAvailable = 1u << 4u,
  DiagnosticInputFenceBAvailable = 1u << 5u,
  DiagnosticConsumerFenceCAvailable = 1u << 6u,
  DiagnosticSettingsAvailable = 1u << 7u,
  DiagnosticRrStateAvailable = 1u << 8u,
  DiagnosticNrdStatusAvailable = 1u << 9u,
};

enum class DiagnosticNrdBackend : uint32_t { NotCompiled,
                                             NotInitialized,
                                             Ready,
                                             Failed };
enum class DiagnosticNrdPolicy : uint32_t { RawSelected,
                                            RayReconstructionSuppressed,
                                            NoSdk,
                                            BackendUnavailable,
                                            RelaxSelected };

enum class DiagnosticStage : uint16_t {
  None,
  ScenePromotion,
  InputValidate,
  InputAllocate,
  InputSignalA,
  InputRecord,
  InputSubmit,
  InputWaitB,
  DiPreflight,
  SunPreflight,
  NrdInitialize,
  NrdSubmit,
  SlotConsumer,
  LightingBind,
  LightingDraw,
};

enum class DiagnosticReason : uint16_t {
  None,
  MissingEntry,
  NonliveBinding,
  UnknownBinding,
  StaleFrame,
  CommandMismatch,
  SequenceMismatch,
  ResetMismatch,
  BufferMismatch,
  OffsetMismatch,
  NormalizedSizeMismatch,
  InvalidResource,
  InvalidRange,
  ContextQueryFailed,
  NativeB0Mismatch,
  MissingInput,
  Busy,
  PipelineCreateFailed,
  ResourceCreateFailed,
  SignalFailed,
  SubmitFailed,
  WaitFailed,
  ConfigurationMismatch,
  Quarantined,
  CallbackObserved,
};

enum class DiagnosticOperation : uint16_t {
  None,
  Promote,
  Validate,
  Allocate,
  Pack,
  SignalA,
  Execute,
  WaitB,
  Dispatch,
  BindLocal,
  BindSun,
  LightingCallback,
  SignalC,
  Retire,
};

enum class DiagnosticInputKind : uint8_t {
  None,
  Depth,
  Normal,
  Color,
  Mrt1,
  Mrt2,
  Motion,
  Lights,
  Tiles,
  Deferred,
  Layout,
};

enum class DiagnosticValidationCode : uint8_t {
  None,
  InvalidTextureView,
  InvalidStructuredView,
  LayoutRejected,
  OptionalFallback,
};

enum class DiagnosticStructuredFailure : uint16_t {
  None,
  NullArgument,
  WrongViewDimension,
  WrongFormat,
  ResourceUnavailable,
  ResourceNotBuffer,
  MissingStructuredFlag,
  StructureStrideMismatch,
  ElementBoundsInvalid,
};

struct DiagnosticEvent {
  uint64_t capture_epoch = 0u;
  uint64_t event_sequence = 0u;
  uint64_t owner_generation = 0u;
  uint64_t frame = UINT64_MAX;
  uint64_t capture_frame = UINT64_MAX;
  uint64_t settings_identity = 0u;
  uint64_t effective_identity = 0u;
  uint64_t fence = 0u;
  uint64_t value0 = 0u;
  uint64_t value1 = 0u;
  uint64_t buffer_bytes = 0u;
  uint64_t normalized_size = 0u;
  uint64_t native_buffer_bytes = 0u;
  uint64_t native_offset_bytes = 0u;
  uint64_t native_count_bytes = 0u;
  uint32_t available_rows = 0u;
  uint32_t required_rows = 0u;
  uint32_t copied_rows = 0u;
  uint32_t native_bind_flags = 0u;
  std::array<uint64_t, 4> identity_tokens = {};
  int32_t hresult = 0;
  int32_t scene_slot = -1;
  int32_t transfer_slot = -1;
  uint32_t flags = 0u;
  uint32_t first_constant = 0u;
  uint32_t constant_count = 0u;
  DiagnosticStage stage = DiagnosticStage::None;
  DiagnosticReason reason = DiagnosticReason::None;
  DiagnosticOperation operation = DiagnosticOperation::None;
  DiagnosticInputKind input_kind = DiagnosticInputKind::None;
  DiagnosticValidationCode validation_code = DiagnosticValidationCode::None;
  uint16_t input_status = UINT16_MAX;
  uint16_t input_stage = UINT16_MAX;
};

struct DiagnosticFailure {
  DiagnosticEvent first = {};
  uint64_t count = 0u;
  uint64_t first_frame = UINT64_MAX;
  uint64_t latest_frame = UINT64_MAX;
  bool occupied = false;
};

struct DiagnosticReasonCount {
  DiagnosticReason reason = DiagnosticReason::None;
  uint64_t count = 0u;
  uint64_t first_frame = UINT64_MAX;
  uint64_t latest_frame = UINT64_MAX;
};

struct DiagnosticIdentity {
  uint64_t native_identity = 0u;
  uint64_t destruction_generation = 0u;
  uint64_t token = 0u;
  bool occupied = false;
};

struct DiagnosticSummary {
  uint64_t build_id = 0u;
  uint64_t requested_settings = 0u;
  uint64_t effective_settings = 0u;
  uint64_t configuration_generation = 0u;
  uint64_t n1_configuration = 0u;
  uint64_t nrd_configuration = 0u;
  uint64_t owner_generation = 0u;
  uint64_t frame = UINT64_MAX;
  uint64_t capture_frame = UINT64_MAX;
  uint64_t scene_age = UINT64_MAX;
  uint64_t input_age = UINT64_MAX;
  uint64_t di_age = UINT64_MAX;
  uint64_t sun_age = UINT64_MAX;
  uint64_t nrd_age = UINT64_MAX;
  uint64_t scene_fence = 0u;
  uint64_t input_fence_b = 0u;
  uint64_t consumer_fence_c = 0u;
  uint64_t attempts = 0u;
  uint64_t submissions = 0u;
  uint64_t skips = 0u;
  uint64_t current_status_reports_failure = 0u;
  uint64_t bindings = 0u;
  uint64_t matching_draw_callbacks = 0u;
  uint64_t pending_slots = 0u;
  uint64_t busy_slots = 0u;
  uint64_t quarantined_slots = 0u;
  uint64_t retired_slots = 0u;
  uint64_t shader_hash = 0u;
  uint64_t pipeline_layout = 0u;
  uint64_t resource_bytes = 0u;
  uint64_t resource_count = 0u;
  uint32_t width = 0u;
  uint32_t height = 0u;
  uint32_t scene_slot = UINT32_MAX;
  uint32_t transfer_slot = UINT32_MAX;
  uint32_t resource_format = UINT32_MAX;
  uint32_t resource_stride = 0u;
  uint32_t resource_elements = 0u;
  uint32_t runtime_gates = 0u;
  uint32_t rr_requested = 0u;
  uint32_t rr_owns_denoise = 0u;
  uint32_t denoiser_reason = 0u;
  uint32_t camera_qualification = 0u;
  uint32_t camera_reset_reason = 0u;
  uint32_t scene_status = 0u;
  uint32_t input_status = 0u;
  uint32_t input_stage = 0u;
  uint32_t di_status = 0u;
  uint32_t sun_status = 0u;
  uint32_t nrd_status = 0u;
  uint32_t nrd_compiled = 0u;
  uint32_t nrd_failure_stage = 0u;
  uint32_t nrd_policy = static_cast<uint32_t>(DiagnosticNrdPolicy::RawSelected);
  uint32_t nrd_output_staged = 0u;
  uint32_t publication_status = 0u;
  uint32_t binding_status = 0u;
  int32_t last_hresult = 0;
  uint32_t requested_candidates = 0u;
  uint32_t effective_candidates = 0u;
  uint32_t requested_spatial_samples = 0u;
  uint32_t effective_spatial_samples = 0u;
  uint32_t requested_denoiser = 0u;
  uint32_t effective_denoiser = 0u;
  uint32_t sdk_version = 0u;
  uint32_t nrd_version = 0u;
  uint32_t api_kind = 0u;
  uint32_t dxr_tier = 0u;
  uint32_t devkit_version = 0u;
  uint32_t adapter_vendor = 0u;
  uint32_t adapter_device = 0u;
  uint32_t debug_requested_view = 0u;
  uint32_t debug_effective_view = 0u;
  uint32_t debug_source_ready = 0u;
  uint32_t debug_rejection_reason = 0u;
  uint32_t debug_observed_frame = UINT32_MAX;
  uint32_t debug_width = 0u;
  uint32_t debug_height = 0u;
  uint32_t debug_scene_slot = UINT32_MAX;
  uint32_t debug_transfer_slot = UINT32_MAX;
  uint32_t debug_superseded_by_bvh = 0u;
  uint64_t debug_owner_generation = 0u;
  uint64_t debug_transfer_configuration = 0u;
  uint64_t debug_di_configuration = 0u;
  uint64_t debug_n1_configuration = 0u;
  uint32_t available_mask = 0u;
};

struct DiagnosticSnapshot {
  uint64_t capture_epoch = 0u;
  uint64_t next_event_sequence = 1u;
  std::array<DiagnosticEvent, kDiagnosticTransitionCapacity> transitions = {};
  std::array<DiagnosticFailure, kDiagnosticFailureCapacity> first_failures = {};
  std::array<DiagnosticReasonCount, kDiagnosticReasonCapacity> reason_counts = {};
  std::array<DiagnosticIdentity, kDiagnosticIdentityCapacity> identities = {};
  DiagnosticSummary latest = {};
  uint64_t transition_count = 0u;
  uint64_t transition_overwritten = 0u;
  uint64_t failure_records_dropped = 0u;
  uint64_t untracked_identities = 0u;
  uint32_t transition_head = 0u;
  uint32_t transition_size = 0u;
  uint32_t failure_size = 0u;
  uint32_t reason_size = 0u;
  uint32_t identity_size = 0u;
};

struct DiagnosticState {
  std::shared_mutex mutex;
  DiagnosticSnapshot snapshot = {};
  std::atomic<uint64_t> capture_control = 0u;
};

static_assert(sizeof(DiagnosticSnapshot) <= 256u * 1024u);

inline DiagnosticState& GetDiagnosticState() {
  static DiagnosticState state;
  return state;
}

inline uint64_t DiagnosticCaptureEpoch(const DiagnosticState& state) {
  return state.capture_control.load(std::memory_order_acquire) >> 1u;
}

inline bool DiagnosticCaptureEnabled(const DiagnosticState& state) {
  return (state.capture_control.load(std::memory_order_acquire) & 1u) != 0u;
}

inline uint64_t BeginDiagnosticEpoch(DiagnosticState& state, bool enabled) {
  std::unique_lock lock(state.mutex);
  uint64_t current = state.capture_control.load(std::memory_order_relaxed);
  const uint64_t next = (((current >> 1u) + 1u) << 1u) | (enabled ? 1u : 0u);
  state.capture_control.store(next, std::memory_order_release);
  state.snapshot = {};
  state.snapshot.capture_epoch = next >> 1u;
  return next >> 1u;
}

inline void SetDiagnosticCapture(DiagnosticState& state, bool enabled) {
  std::unique_lock lock(state.mutex);
  const uint64_t current = state.capture_control.load(std::memory_order_relaxed);
  if (((current & 1u) != 0u) == enabled) return;
  const uint64_t next = (((current >> 1u) + 1u) << 1u) | (enabled ? 1u : 0u);
  state.capture_control.store(next, std::memory_order_release);
  if (enabled) {
    state.snapshot = {};
    state.snapshot.capture_epoch = next >> 1u;
  }
}

inline void ClearDiagnosticCapture(DiagnosticState& state) {
  BeginDiagnosticEpoch(state, DiagnosticCaptureEnabled(state));
}

inline bool SameDiagnosticTransition(const DiagnosticEvent& a, const DiagnosticEvent& b) {
  return a.owner_generation == b.owner_generation && a.stage == b.stage && a.reason == b.reason
         && a.operation == b.operation && a.scene_slot == b.scene_slot && a.transfer_slot == b.transfer_slot
         && a.settings_identity == b.settings_identity && a.effective_identity == b.effective_identity
         && a.hresult == b.hresult && a.value0 == b.value0 && a.value1 == b.value1
         && a.buffer_bytes == b.buffer_bytes && a.normalized_size == b.normalized_size
         && a.native_buffer_bytes == b.native_buffer_bytes
         && a.native_offset_bytes == b.native_offset_bytes && a.native_count_bytes == b.native_count_bytes
         && a.available_rows == b.available_rows && a.required_rows == b.required_rows
         && a.copied_rows == b.copied_rows && a.native_bind_flags == b.native_bind_flags
         && a.identity_tokens == b.identity_tokens && a.flags == b.flags
         && a.first_constant == b.first_constant && a.constant_count == b.constant_count
         && a.input_kind == b.input_kind && a.validation_code == b.validation_code
         && a.input_status == b.input_status && a.input_stage == b.input_stage;
}

inline void RecordDiagnostic(DiagnosticState& state, uint64_t epoch, DiagnosticEvent event) {
  const uint64_t control = state.capture_control.load(std::memory_order_acquire);
  if (control != ((epoch << 1u) | 1u)) return;
  std::unique_lock lock(state.mutex);
  if (state.capture_control.load(std::memory_order_acquire) != control) return;
  auto& snapshot = state.snapshot;
  if (snapshot.capture_epoch != epoch) return;
  event.capture_epoch = epoch;
  event.event_sequence = snapshot.next_event_sequence;
  if (snapshot.next_event_sequence != UINT64_MAX) ++snapshot.next_event_sequence;

  if (event.reason != DiagnosticReason::None) {
    bool counted = false;
    for (uint32_t i = 0u; i < snapshot.reason_size; ++i) {
      auto& count = snapshot.reason_counts[i];
      if (count.reason != event.reason) continue;
      if (count.count != UINT64_MAX) ++count.count;
      count.latest_frame = event.frame;
      counted = true;
      break;
    }
    if (!counted && snapshot.reason_size < snapshot.reason_counts.size()) {
      auto& count = snapshot.reason_counts[snapshot.reason_size++];
      count = {.reason = event.reason, .count = 1u, .first_frame = event.frame, .latest_frame = event.frame};
    } else if (!counted) {
      if (snapshot.failure_records_dropped != UINT64_MAX) ++snapshot.failure_records_dropped;
    }

    DiagnosticFailure* failure = nullptr;
    for (auto& candidate : snapshot.first_failures) {
      if (candidate.occupied && candidate.first.stage == event.stage && candidate.first.reason == event.reason) {
        failure = &candidate;
        break;
      }
      if (failure == nullptr && !candidate.occupied) failure = &candidate;
    }
    if (failure != nullptr && !failure->occupied) {
      failure->occupied = true;
      failure->first = event;
      failure->count = 1u;
      failure->first_frame = failure->latest_frame = event.frame;
      if (snapshot.failure_size < snapshot.first_failures.size()) ++snapshot.failure_size;
    } else if (failure != nullptr) {
      if (failure->count != UINT64_MAX) ++failure->count;
      failure->latest_frame = event.frame;
    } else if (snapshot.failure_records_dropped != UINT64_MAX) {
      ++snapshot.failure_records_dropped;
    }
  }

  bool duplicate = false;
  if (snapshot.transition_size != 0u) {
    const uint32_t last = (snapshot.transition_head + snapshot.transitions.size() - 1u) % snapshot.transitions.size();
    duplicate = SameDiagnosticTransition(snapshot.transitions[last], event);
  }
  if (!duplicate) {
    snapshot.transitions[snapshot.transition_head] = event;
    snapshot.transition_head = (snapshot.transition_head + 1u) % snapshot.transitions.size();
    if (snapshot.transition_size < snapshot.transitions.size()) {
      ++snapshot.transition_size;
    } else if (snapshot.transition_overwritten != UINT64_MAX) {
      ++snapshot.transition_overwritten;
    }
    if (snapshot.transition_count != UINT64_MAX) ++snapshot.transition_count;
  }
  snapshot.latest.frame = event.frame;
  snapshot.latest.capture_frame = event.capture_frame;
  snapshot.latest.owner_generation = event.owner_generation;
  snapshot.latest.requested_settings = event.settings_identity;
  snapshot.latest.effective_settings = event.effective_identity;
  snapshot.latest.last_hresult = event.hresult;
  if ((event.operation == DiagnosticOperation::BindLocal || event.operation == DiagnosticOperation::BindSun)
      && (event.flags & 1u) != 0u && snapshot.latest.bindings != UINT64_MAX) {
    ++snapshot.latest.bindings;
  }
  if (event.operation == DiagnosticOperation::LightingCallback && (event.flags & 2u) != 0u
      && snapshot.latest.matching_draw_callbacks != UINT64_MAX) {
    ++snapshot.latest.matching_draw_callbacks;
  }
}

inline void UpdateDiagnosticSummary(DiagnosticState& state, uint64_t epoch, const DiagnosticSummary& summary) {
  if (!DiagnosticCaptureEnabled(state) || DiagnosticCaptureEpoch(state) != epoch) return;
  std::unique_lock lock(state.mutex);
  if (state.capture_control.load(std::memory_order_acquire) != ((epoch << 1u) | 1u)
      || state.snapshot.capture_epoch != epoch) return;
  const uint64_t bindings = state.snapshot.latest.bindings;
  const uint64_t matching_draw_callbacks = state.snapshot.latest.matching_draw_callbacks;
  state.snapshot.latest = summary;
  state.snapshot.latest.bindings = bindings;
  state.snapshot.latest.matching_draw_callbacks = matching_draw_callbacks;
}

inline uint64_t DiagnosticIdentityToken(DiagnosticState& state, uint64_t epoch, uint64_t identity,
                                        uint64_t destruction_generation) {
  if (identity == 0u || !DiagnosticCaptureEnabled(state) || DiagnosticCaptureEpoch(state) != epoch) return 0u;
  std::unique_lock lock(state.mutex);
  if (state.capture_control.load(std::memory_order_acquire) != ((epoch << 1u) | 1u)
      || state.snapshot.capture_epoch != epoch) return 0u;
  auto& snapshot = state.snapshot;
  for (const auto& token : snapshot.identities) {
    if (token.occupied && token.native_identity == identity
        && token.destruction_generation == destruction_generation) return token.token;
  }
  if (snapshot.identity_size >= snapshot.identities.size()) {
    if (snapshot.untracked_identities != UINT64_MAX) ++snapshot.untracked_identities;
    return 0u;
  }
  const uint64_t token_value = uint64_t{snapshot.identity_size} + 1u;
  snapshot.identities[snapshot.identity_size++] = {.native_identity = identity,
                                                   .destruction_generation = destruction_generation,
                                                   .token = token_value,
                                                   .occupied = true};
  return token_value;
}

inline DiagnosticSnapshot CopyDiagnosticSnapshot(DiagnosticState& state) {
  std::shared_lock lock(state.mutex);
  return state.snapshot;
}

inline std::string SerializeDiagnosticReport(const DiagnosticSnapshot& snapshot) {
  std::array<char, kDiagnosticReportCapacity> buffer = {};
  size_t size = 0u;
  bool truncated = false;
  auto append = [&](const char* format, auto... values) {
    if (size >= buffer.size()) {
      truncated = true;
      return;
    }
    const int written = std::snprintf(buffer.data() + size, buffer.size() - size, format, values...);
    if (written < 0) {
      truncated = true;
      return;
    }
    if (static_cast<size_t>(written) >= buffer.size() - size) {
      size = buffer.size() - 1u;
      truncated = true;
      return;
    }
    size += static_cast<size_t>(written);
  };
  append("RTXDI diagnostic report\nschema=1 epoch=%llu transitions=%u total=%llu overwritten=%llu failures=%u dropped=%llu identities=%u untracked=%llu\n",
         static_cast<unsigned long long>(snapshot.capture_epoch), snapshot.transition_size,
         static_cast<unsigned long long>(snapshot.transition_count),
         static_cast<unsigned long long>(snapshot.transition_overwritten), snapshot.failure_size,
         static_cast<unsigned long long>(snapshot.failure_records_dropped), snapshot.identity_size,
         static_cast<unsigned long long>(snapshot.untracked_identities));
  append("codes stage=0:None,1:ScenePromotion,2:InputValidate,3:InputAllocate,4:InputSignalA,5:InputRecord,6:InputSubmit,7:InputWaitB,8:DiPreflight,9:SunPreflight,10:NrdInitialize,11:NrdSubmit,12:SlotConsumer,13:LightingBind,14:LightingDraw\n");
  append("codes reason=0:None,1:MissingEntry,2:NonliveBinding,3:UnknownBinding,4:StaleFrame,5:CommandMismatch,6:SequenceMismatch,7:ResetMismatch,8:BufferMismatch,9:OffsetMismatch,10:NormalizedSizeMismatch,11:InvalidResource,12:InvalidRange,13:ContextQueryFailed,14:NativeB0Mismatch,15:MissingInput,16:Busy,17:PipelineCreateFailed,18:ResourceCreateFailed,19:SignalFailed,20:SubmitFailed,21:WaitFailed,22:ConfigurationMismatch,23:Quarantined,24:CallbackObserved\n");
  append("codes operation=0:None,1:Promote,2:Validate,3:Allocate,4:Pack,5:SignalA,6:Execute,7:WaitB,8:Dispatch,9:BindLocal,10:BindSun,11:LightingCallback,12:SignalC,13:Retire\n");
  append("codes input=0:None,1:Depth,2:Normal,3:Color,4:Mrt1,5:Mrt2,6:Motion,7:Lights,8:Tiles,9:Deferred,10:Layout; validation=0:None,1:InvalidTextureView,2:InvalidStructuredView,3:LayoutRejected,4:OptionalFallback\n");
  append("codes structured_failure=0:None,1:NullArgument,2:WrongViewDimension,3:WrongFormat,4:ResourceUnavailable,5:ResourceNotBuffer,6:MissingStructuredFlag,7:StructureStrideMismatch,8:ElementBoundsInvalid; evidence=1:srv_desc,2:get_resource,4:buffer_type,8:buffer_desc,16:bufferex_flags,32:computed_bounds,64:bounds_valid,128:attempted\n");
  append("codes input_status=0:Off,1:AlreadyAttempted,2:WaitingForScene,3:MissingInput,4:SceneBindingChanged,5:Busy,6:AllocationFailed,7:SubmitFailed,8:Quarantined,9:Ready; input_stage=0:None,1:Validate,2:Allocate,3:Pack,4:SignalA,5:Record,6:Order,7:Submit,8:WaitB,9:Ready; 65535=unavailable\n");
  const auto& summary = snapshot.latest;
  append("summary frame=%llu capture_frame=%llu owner=%llu settings=%llu effective=%llu config=%llu n1=unavailable nrd_config=unavailable size=%ux%u scene_slot=%u transfer_slot=%u\n",
         static_cast<unsigned long long>(summary.frame), static_cast<unsigned long long>(summary.capture_frame),
         static_cast<unsigned long long>(summary.owner_generation), static_cast<unsigned long long>(summary.requested_settings),
         static_cast<unsigned long long>(summary.effective_settings), static_cast<unsigned long long>(summary.configuration_generation),
         summary.width, summary.height, summary.scene_slot, summary.transfer_slot);
  append("states scene=%u input=%u stage=%u di=%u sun=%u nrd_backend=%u nrd_failure_stage=%u nrd_policy=%u nrd_diffuse_staged=%u publication=%u binding=%u rr_requested=%u rr_owner=%u denoiser=%u\n",
         summary.scene_status, summary.input_status, summary.input_stage, summary.di_status, summary.sun_status,
         summary.nrd_status, summary.nrd_failure_stage, summary.nrd_policy, summary.nrd_output_staged,
         summary.publication_status, summary.binding_status, summary.rr_requested,
         summary.rr_owns_denoise, summary.denoiser_reason);
  append("debug requested=%u effective=%u source_ready=%u reason=%u frame=%u owner=%llu transfer_config=%llu di_config=%llu n1_config=",
         summary.debug_requested_view, summary.debug_effective_view, summary.debug_source_ready,
         summary.debug_rejection_reason, summary.debug_observed_frame,
         static_cast<unsigned long long>(summary.debug_owner_generation),
         static_cast<unsigned long long>(summary.debug_transfer_configuration),
         static_cast<unsigned long long>(summary.debug_di_configuration));
  if (summary.debug_n1_configuration == 0u) {
    append("unavailable");
  } else {
    append("%llu", static_cast<unsigned long long>(summary.debug_n1_configuration));
  }
  append(" scene_slot=%u transfer_slot=%u size=%ux%u bvh_supersedes=%u\n",
         summary.debug_scene_slot, summary.debug_transfer_slot, summary.debug_width, summary.debug_height,
         summary.debug_superseded_by_bvh);
  append("debug_reason=0:None,1:BackendUnavailable,2:NotProduced,3:NoSdk,4:RrSuppressed,5:WaitingInput,6:StaleIdentity,7:InvalidResource,8:BvhSuperseded,9:ProducerDisabled\n");
  append("fences scene=%llu A=unavailable B_value=%llu C_value=%llu completion_knowledge=unavailable ages scene=%llu input=%llu di=%llu sun=%llu nrd=%llu\n",
         static_cast<unsigned long long>(summary.scene_fence),
         static_cast<unsigned long long>(summary.input_fence_b), static_cast<unsigned long long>(summary.consumer_fence_c),
         static_cast<unsigned long long>(summary.scene_age),
         static_cast<unsigned long long>(summary.input_age), static_cast<unsigned long long>(summary.di_age),
         static_cast<unsigned long long>(summary.sun_age), static_cast<unsigned long long>(summary.nrd_age));
  append("counts scene_attempts=%llu scene_submissions=%llu scene_skips=%llu current_status_reports_failure=%llu output_bind_events=%llu matching_draw_callbacks=%llu slot_counts=unavailable\n",
         static_cast<unsigned long long>(summary.attempts), static_cast<unsigned long long>(summary.submissions),
         static_cast<unsigned long long>(summary.skips),
         static_cast<unsigned long long>(summary.current_status_reports_failure),
         static_cast<unsigned long long>(summary.bindings), static_cast<unsigned long long>(summary.matching_draw_callbacks));
  append("current_status_reports_failure is only scene Failed or input SubmitFailed in the latest summary; false does not clear failure history\n");
  append("resource bytes=unavailable count=unavailable format=unavailable stride=unavailable elements=unavailable shader=unavailable layout=unavailable available=0x%08x hresult=%d gates=0x%08x sdk=unavailable nrd_compiled=%u api=unavailable dxr=unavailable devkit=unavailable adapter=unavailable\n",
         summary.available_mask, summary.last_hresult, summary.runtime_gates, summary.nrd_compiled);
  append("availability bits: frame=0 states=1 input_identity=2 dimensions=3 scene_fence=4 B_fence_value=5 C_fence_value=6 settings=7 rr=8 nrd_status=9; unlisted fields unavailable\n");
  append("nrd_backend=0:NotCompiled,1:NotInitialized,2:Ready,3:Failed; nrd_failure_stage=0:None,1:SDKInstance,2:D3D12Backend; nrd_policy=0:RawSelected,1:RayReconstructionSuppressed,2:NoSdk,3:BackendUnavailable,4:RelaxSelected; RR suppression applies to diffuse denoising, raw local/sun lighting remains independent; staged is private producer evidence, not per-pixel application\n");
  append("settings candidates=%u/%u spatial=%u/%u denoiser=%u/%u camera=unavailable reset=unavailable age_sentinel=%llu\n",
         summary.requested_candidates, summary.effective_candidates, summary.requested_spatial_samples,
         summary.effective_spatial_samples, summary.requested_denoiser, summary.effective_denoiser,
         static_cast<unsigned long long>(UINT64_MAX));
  append("reasons:\n");
  for (uint32_t i = 0u; i < snapshot.reason_size; ++i) {
    const auto& reason = snapshot.reason_counts[i];
    append("  code=%u count=%llu first=%llu latest=%llu\n", static_cast<uint32_t>(reason.reason),
           static_cast<unsigned long long>(reason.count), static_cast<unsigned long long>(reason.first_frame),
           static_cast<unsigned long long>(reason.latest_frame));
  }
  append("first_failures:\n");
  for (const auto& failure : snapshot.first_failures) {
    if (!failure.occupied) continue;
    const auto& event = failure.first;
    append("  stage=%u reason=%u op=%u input=%u validation=%u input_status=%u input_stage=%u count=%llu first=%llu latest=%llu owner=%llu frame=%llu capture=%llu hr=%d scene=%d transfer=%d settings=%llu effective=%llu tokens=%llu,%llu,%llu,%llu flags=0x%08x range=%llu,%llu buffer=%llu normalized=%llu native_bytes=%llu native_range=%u,%u native_window=%llu,%llu bind_flags=0x%08x rows=%u/%u copied=%u\n",
           static_cast<uint32_t>(event.stage), static_cast<uint32_t>(event.reason), static_cast<uint32_t>(event.operation),
           static_cast<uint32_t>(event.input_kind), static_cast<uint32_t>(event.validation_code),
           static_cast<uint32_t>(event.input_status), static_cast<uint32_t>(event.input_stage),
           static_cast<unsigned long long>(failure.count), static_cast<unsigned long long>(failure.first_frame),
           static_cast<unsigned long long>(failure.latest_frame), static_cast<unsigned long long>(event.owner_generation),
           static_cast<unsigned long long>(event.frame), static_cast<unsigned long long>(event.capture_frame), event.hresult,
           event.scene_slot, event.transfer_slot, static_cast<unsigned long long>(event.settings_identity),
           static_cast<unsigned long long>(event.effective_identity), static_cast<unsigned long long>(event.identity_tokens[0]),
           static_cast<unsigned long long>(event.identity_tokens[1]), static_cast<unsigned long long>(event.identity_tokens[2]),
           static_cast<unsigned long long>(event.identity_tokens[3]), event.flags,
           static_cast<unsigned long long>(event.value0), static_cast<unsigned long long>(event.value1),
           static_cast<unsigned long long>(event.buffer_bytes), static_cast<unsigned long long>(event.normalized_size),
           static_cast<unsigned long long>(event.native_buffer_bytes),
           event.first_constant, event.constant_count,
           static_cast<unsigned long long>(event.native_offset_bytes),
           static_cast<unsigned long long>(event.native_count_bytes), event.native_bind_flags,
           event.available_rows, event.required_rows, event.copied_rows);
    if ((event.flags & 0x80u) != 0u) {
      append("    structured failure=%u evidence=0x%08x stride=%u source_register=",
             event.flags >> 16u, event.flags & 0x7fu, static_cast<uint32_t>(event.normalized_size));
      if (event.required_rows == UINT32_MAX) {
        append("unavailable");
      } else {
        append("t%u", event.required_rows);
      }
      append(" dimension=%u format=%u first=%u count=%u bufferex_flags=0x%08x resource_token=%llu bytes=%llu bind=0x%08x misc=0x%08x structure_stride=%u offset=%llu length=%llu\n",
             static_cast<uint32_t>(event.native_offset_bytes), static_cast<uint32_t>(event.native_count_bytes),
             event.first_constant, event.constant_count, event.copied_rows,
             static_cast<unsigned long long>(event.identity_tokens[0]),
             static_cast<unsigned long long>(event.buffer_bytes), event.native_bind_flags,
             static_cast<uint32_t>(event.normalized_size >> 32u), event.available_rows,
             static_cast<unsigned long long>(event.value0), static_cast<unsigned long long>(event.value1));
    }
  }
  append("recent_transitions oldest_to_newest:\n");
  const uint32_t first = (snapshot.transition_head + snapshot.transitions.size() - snapshot.transition_size)
                         % snapshot.transitions.size();
  for (uint32_t i = 0u; i < snapshot.transition_size; ++i) {
    const auto& event = snapshot.transitions[(first + i) % snapshot.transitions.size()];
    append("  seq=%llu stage=%u reason=%u op=%u input=%u validation=%u input_status=%u input_stage=%u owner=%llu frame=%llu capture=%llu hr=%d scene=%d transfer=%d fence=%llu tokens=%llu,%llu,%llu,%llu flags=0x%08x range=%llu,%llu buffer=%llu normalized=%llu native_bytes=%llu native_range=%u,%u native_window=%llu,%llu bind_flags=0x%08x rows=%u/%u copied=%u\n",
           static_cast<unsigned long long>(event.event_sequence), static_cast<uint32_t>(event.stage),
           static_cast<uint32_t>(event.reason), static_cast<uint32_t>(event.operation),
           static_cast<uint32_t>(event.input_kind), static_cast<uint32_t>(event.validation_code),
           static_cast<uint32_t>(event.input_status), static_cast<uint32_t>(event.input_stage),
           static_cast<unsigned long long>(event.owner_generation), static_cast<unsigned long long>(event.frame),
           static_cast<unsigned long long>(event.capture_frame), event.hresult, event.scene_slot, event.transfer_slot,
           static_cast<unsigned long long>(event.fence), static_cast<unsigned long long>(event.identity_tokens[0]),
           static_cast<unsigned long long>(event.identity_tokens[1]), static_cast<unsigned long long>(event.identity_tokens[2]),
           static_cast<unsigned long long>(event.identity_tokens[3]), event.flags,
           static_cast<unsigned long long>(event.value0), static_cast<unsigned long long>(event.value1),
           static_cast<unsigned long long>(event.buffer_bytes), static_cast<unsigned long long>(event.normalized_size),
           static_cast<unsigned long long>(event.native_buffer_bytes), event.first_constant, event.constant_count,
           static_cast<unsigned long long>(event.native_offset_bytes),
           static_cast<unsigned long long>(event.native_count_bytes), event.native_bind_flags,
           event.available_rows, event.required_rows, event.copied_rows);
    if ((event.flags & 0x80u) != 0u) {
      append("    structured failure=%u evidence=0x%08x stride=%u source_register=",
             event.flags >> 16u, event.flags & 0x7fu, static_cast<uint32_t>(event.normalized_size));
      if (event.required_rows == UINT32_MAX) {
        append("unavailable");
      } else {
        append("t%u", event.required_rows);
      }
      append(" dimension=%u format=%u first=%u count=%u bufferex_flags=0x%08x resource_token=%llu bytes=%llu bind=0x%08x misc=0x%08x structure_stride=%u offset=%llu length=%llu\n",
             static_cast<uint32_t>(event.native_offset_bytes), static_cast<uint32_t>(event.native_count_bytes),
             event.first_constant, event.constant_count, event.copied_rows,
             static_cast<unsigned long long>(event.identity_tokens[0]),
             static_cast<unsigned long long>(event.buffer_bytes), event.native_bind_flags,
             static_cast<uint32_t>(event.normalized_size >> 32u), event.available_rows,
             static_cast<unsigned long long>(event.value0), static_cast<unsigned long long>(event.value1));
    }
  }
  if (truncated || size >= buffer.size() - 1u) {
    constexpr char summary_text[] = "\n[report truncated at fixed 65536-byte limit]\n";
    const size_t summary_size = sizeof(summary_text) - 1u;
    size = (std::min)(size, buffer.size() - summary_size - 1u);
    std::copy(summary_text, summary_text + summary_size, buffer.begin() + static_cast<std::ptrdiff_t>(size));
    size += summary_size;
  }
  return std::string(buffer.data(), size);
}

}  // namespace falcom_world::rtxdi
