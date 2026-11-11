#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <shared_mutex>

#include "rtxdi_debug_layout.h"
#include "rtxdi_diagnostics.hpp"

namespace falcom_world::rtxdi {

inline float g_local_requested = 0.f;
inline float g_sun_requested = 0.f;
inline float g_quality_preset = 1.f;
inline float g_local_candidates = 8.f;
inline float g_temporal_reuse = 1.f;
inline float g_spatial_reuse = 1.f;
inline float g_spatial_samples = 4.f;
inline float g_spatial_disocclusion = 4.f;
inline float g_spatial_radius = 4.f;
inline float g_history_length = 20.f;
inline float g_temporal_depth_threshold = 0.1f;
inline float g_temporal_normal_threshold = 0.5f;
inline float g_spatial_depth_threshold = 0.05f;
inline float g_spatial_normal_threshold = 0.9f;
inline float g_boiling_filter = 1.f;
inline float g_boiling_filter_strength = 0.2f;
inline float g_diffuse_denoiser = 1.f;
inline float g_debug_logging = 0.f;
inline float g_debug_view = static_cast<float>(RTXDI_DEBUG_VIEW_OFF);
inline float g_sun_detail_view = static_cast<float>(RTXDI_SUN_DETAIL_STATUS);
inline float g_relax_diffuse_history = 30.f;
inline float g_relax_fast_history = 6.f;
inline float g_relax_prepass_radius = 30.f;
inline float g_relax_atrous_iterations = 5.f;
inline float g_local_normal_bias = 0.02f;
inline float g_local_character_normal_bias = 0.02f;
inline float g_local_light_clearance = 0.2f;
inline float g_sun_normal_bias = 0.02f;
inline float g_sun_character_normal_bias = 0.02f;
inline float g_sun_casters_only = 1.f;
inline float g_sun_shadow_darkening = 1.f;
inline float g_sun_character_shadow_darkening = 1.f;
inline float g_sun_trace_resolution = 0.f;
inline float g_sun_light_size = 0.f;
inline float g_sun_isfast = 1.f;
inline float g_sun_foliage_transmission = 0.f;
inline float g_sun_wind_transmission = 0.5f;
inline float g_sun_wind_softening = 0.5f;
inline float g_sun_character_self_softening = 0.f;
inline float g_sun_accumulation = 1.f;
inline float g_sun_accumulation_frames = 8.f;
inline float g_sun_foliage_tint_r = 1.f;
inline float g_sun_foliage_tint_g = 1.f;
inline float g_sun_foliage_tint_b = 1.f;
inline float g_local_shadow_darkening = 1.f;
inline float g_local_character_shadow_darkening = 1.f;
inline float g_local_character_intensity = 1.f;
inline float g_local_character_fallback = 1.f;
inline float g_local_light_size = 0.f;
inline float g_local_isfast = 1.f;
inline float g_local_character_self_softening = 0.f;
inline float FiniteClamped(float value, float fallback, float minimum, float maximum) { return std::clamp(std::isfinite(value) ? value : fallback, minimum, maximum); }
inline bool SunAccumulationRequested() { return g_sun_accumulation > 0.5f && FiniteClamped(g_sun_light_size, 0.f, 0.f, 5.f) > 0.f; }
inline bool g_applying_quality_preset = false;
inline uint32_t SceneRequestMask() { return (g_local_requested > 0.5f ? 1u : 0u) | (g_sun_requested > 0.5f ? 2u : 0u); }
inline bool SceneRequested() { return SceneRequestMask() != 0u; }

struct DebugSnapshot {
  uint32_t requested_view = RTXDI_DEBUG_VIEW_OFF;
  uint32_t effective_view = RTXDI_DEBUG_VIEW_OFF;
  uint32_t sun_detail_view = RTXDI_SUN_DETAIL_STATUS;
  bool sun_detail_source_ready = false;
  uint32_t source_status = 0u;
  uint32_t rejection_reason = 0u;
  uint32_t frame = UINT32_MAX;
  uint32_t width = 0u, height = 0u;
  uint64_t owner_generation = 0u;
  uint64_t transfer_configuration = 0u;
  uint64_t di_configuration = 0u;
  uint64_t n1_configuration = 0u;
  int32_t scene_slot = -1, transfer_slot = -1;
  bool source_ready = false;
  bool distance_source = false;
  bool superseded_by_bvh = false;
};
struct DebugState {
  std::shared_mutex mutex;
  DebugSnapshot snapshot;
};
inline DebugState g_debug;

enum class SceneStatus { Off,
                         DxrOff,
                         NotCreated,
                         NoDevice,
                         DeferredContext,
                         Guarded,
                         MissingSceneInput,
                         SceneBindingChanged,
                         GeometryNotReady,
                         Reused,
                         Submitted,
                         Busy,
                         Failed,
                         Quarantined };
struct SceneSnapshot {
  SceneStatus status = SceneStatus::Off;
  uint32_t request_mask = 0u;
  uint32_t attempted_frame = UINT32_MAX;
  uint64_t attempted_generation = 0u;
  uint32_t prepared_frame = UINT32_MAX;
  int32_t prepared_slot = -1;
  uint64_t prepared_fence = 0u;
  uint64_t prepared_generation = 0u;
  uint64_t attempts = 0u, submissions = 0u, reuses = 0u, skips = 0u;
};
struct SceneState {
  std::shared_mutex mutex;
  SceneSnapshot snapshot;
};
enum class InputStatus { Off,
                         AlreadyAttempted,
                         WaitingForScene,
                         MissingInput,
                         SceneBindingChanged,
                         Busy,
                         AllocationFailed,
                         SubmitFailed,
                         Quarantined,
                         Ready };
enum class InputStage { None,
                        Validate,
                        Allocate,
                        Pack,
                        SignalA,
                        Record,
                        Order,
                        Submit,
                        WaitB,
                        Ready };
inline const char* InputStatusName(InputStatus status) {
  switch (status) {
    case InputStatus::Off:                 return "off";
    case InputStatus::AlreadyAttempted:    return "already attempted";
    case InputStatus::WaitingForScene:     return "waiting for scene";
    case InputStatus::MissingInput:        return "missing input";
    case InputStatus::SceneBindingChanged: return "scene binding changed";
    case InputStatus::Busy:                return "busy";
    case InputStatus::AllocationFailed:    return "allocation failed";
    case InputStatus::SubmitFailed:        return "submit failed";
    case InputStatus::Quarantined:         return "quarantined";
    case InputStatus::Ready:               return "ready";
  }
  return "unknown";
}
struct InputSnapshot {
  InputStatus status = InputStatus::Off;
  InputStage stage = InputStage::None;
  int32_t error = 0;
  uint32_t attempted_frame = UINT32_MAX, ready_frame = UINT32_MAX;
  uint32_t width = 0u, height = 0u;
  uint32_t lights_source = 0u, lights_copied = 0u, lights_truncated = 0u;
  uint32_t tiles_source = 0u, tiles_copied = 0u, deferred_source = 0u, deferred_copied = 0u;
  uint64_t owner_generation = 0u, configuration_generation = 0u, fence = 0u, scene_fence = 0u;
  uint64_t carrier_bytes = 0u, record_bytes = 0u;
  int32_t scene_slot = -1;
  int32_t transfer_slot = -1;
  bool motion_available = false;
};
struct InputState {
  std::shared_mutex mutex;
  InputSnapshot snapshot;
};
inline InputState g_input;

enum class DiStatus { Off,
                      WaitingInput,
                      ZeroLights,
                      Busy,
                      Submitted,
                      Failed,
                      Quarantined };
struct DiSnapshot {
  DiStatus status = DiStatus::Off;
  uint32_t frame = UINT32_MAX;
  uint64_t configuration_generation = 0u;
  uint64_t settings_identity = 0u;
  uint64_t fence = 0u;
  uint32_t local_candidates = 0u;
  uint32_t spatial_samples = 0u;
  uint32_t spatial_disocclusion = 0u;
  uint32_t history_length = 0u;
  bool temporal_enabled = false;
  bool spatial_enabled = false;
  bool relax_selected = false;
  int32_t scene_slot = -1;
  int32_t transfer_slot = -1;
  bool spatial_ran = false;
  bool temporal_ran = false;
  bool history_valid = false;
  uint32_t temporal_source_index = UINT32_MAX;
  uint32_t temporal_output_index = UINT32_MAX;
  uint32_t spatial_source_index = UINT32_MAX;
  uint32_t spatial_output_index = UINT32_MAX;
  uint32_t output_frame = UINT32_MAX, output_width = 0u, output_height = 0u;
  uint64_t output_owner_generation = 0u, output_configuration_generation = 0u;
  uint64_t output_fence_b = 0u, output_fence_c = 0u;
  int32_t output_scene_slot = -1, output_transfer_slot = -1;
  bool output_staged = false;
#if defined(FALCOM_NRD_ENABLED)
  bool nrd_diffuse_staged = false;
  bool nrd_diffuse_bound = false;
  bool nrd_diffuse_lighting_draw_seen = false;
  uint32_t nrd_diffuse_frame = UINT32_MAX;
  int32_t nrd_diffuse_transfer_slot = -1;
  uint64_t nrd_diffuse_fence_b = 0u;
  NrdGateSnapshot nrd_gate = {};
#endif
  bool output_bound = false;
  uint32_t output_bound_frame = UINT32_MAX;
  int32_t output_bound_transfer_slot = -1;
  uint64_t output_bound_fence_b = 0u;
  bool lighting_draw_seen = false;
  int32_t error = 0;
};
struct DiState {
  std::shared_mutex mutex;
  DiSnapshot snapshot;
};
inline DiState g_di;

enum class SunStatus { Off,
                       WaitingInput,
                       Staged,
                       Failed,
                       Quarantined };
struct SunSnapshot {
  SunStatus status = SunStatus::Off;
  uint32_t frame = UINT32_MAX, width = 0u, height = 0u;
  uint64_t owner_generation = 0u, configuration_generation = 0u;
  uint64_t di_configuration_generation = 0u;
  uint64_t fence_b = 0u, fence_c = 0u;
  int32_t scene_slot = -1, transfer_slot = -1;
  bool output_bound = false;
  uint32_t output_bound_frame = UINT32_MAX;
  int32_t output_bound_transfer_slot = -1;
  uint64_t output_bound_fence_b = 0u;
  bool lighting_draw_seen = false;
  bool accumulated = false;
  bool accumulation_history = false;
  int32_t error = 0;
};
inline bool MatchesStagedSunIdentity(const SunSnapshot& snapshot, uint32_t frame, uint64_t owner_generation,
                                     uint64_t configuration_generation, uint64_t di_configuration_generation) {
  return snapshot.status != SunStatus::Staged
         || (snapshot.frame == frame && snapshot.owner_generation == owner_generation
             && snapshot.configuration_generation == configuration_generation
             && snapshot.di_configuration_generation == di_configuration_generation);
}
inline bool MatchesStagedSunSlotIdentity(const SunSnapshot& snapshot, uint32_t frame, uint64_t owner_generation,
                                         uint64_t configuration_generation, uint64_t di_configuration_generation,
                                         uint64_t slot_configuration_generation) {
  return MatchesStagedSunIdentity(snapshot, frame, owner_generation, configuration_generation,
                                  di_configuration_generation)
         && (snapshot.status != SunStatus::Staged
             || snapshot.configuration_generation == slot_configuration_generation);
}
enum class SunAttemptAction { Off,
                              PreserveDuplicate,
                              Invalidate };
inline void TransitionSunAttempt(SunSnapshot* snapshot, uint32_t frame, SunAttemptAction action) {
  switch (action) {
    case SunAttemptAction::Off:
      *snapshot = {.status = SunStatus::Off, .frame = frame};
      break;
    case SunAttemptAction::PreserveDuplicate:
      break;
    case SunAttemptAction::Invalidate:
      *snapshot = {.status = SunStatus::WaitingInput, .frame = frame};
      break;
  }
}
struct SunState {
  std::shared_mutex mutex;
  SunSnapshot snapshot;
};
inline SunState g_sun;

inline const char* SunStatusName(SunStatus status) {
  switch (status) {
    case SunStatus::Off:          return "off";
    case SunStatus::WaitingInput: return "waiting input";
    case SunStatus::Staged:       return "staged";
    case SunStatus::Failed:       return "failed";
    case SunStatus::Quarantined:  return "quarantined";
  }
  return "unknown";
}

inline const char* DiStatusName(DiStatus status) {
  switch (status) {
    case DiStatus::Off:          return "off";
    case DiStatus::WaitingInput: return "waiting input";
    case DiStatus::ZeroLights:   return "zero lights";
    case DiStatus::Busy:         return "busy";
    case DiStatus::Submitted:    return "submitted";
    case DiStatus::Failed:       return "failed";
    case DiStatus::Quarantined:  return "quarantined";
  }
  return "unknown";
}

inline void RefreshInputOffState(InputSnapshot* state, uint32_t frame) {
  if (state->status == InputStatus::Ready && state->ready_frame == frame) return;
  *state = {};
}

inline SceneState g_scene;
inline void RefreshOffState(SceneSnapshot* state, uint32_t frame) {
  if (state->prepared_frame == frame && (state->status == SceneStatus::Submitted || state->status == SceneStatus::Reused)) return;
  state->status = SceneStatus::Off;
  state->request_mask = 0u;
  state->prepared_frame = UINT32_MAX;
  state->prepared_slot = -1;
  state->prepared_fence = 0u;
  state->prepared_generation = 0u;
  if (state->attempted_frame != frame) {
    state->attempted_frame = UINT32_MAX;
    state->attempted_generation = 0u;
  }
}
inline bool IsDuplicate(const SceneSnapshot& state, uint32_t frame, uint64_t generation) {
  return state.attempted_frame == frame && state.attempted_generation == generation;
}
inline bool PreserveAttemptForNonImmediate(const SceneSnapshot& state, uint32_t frame, bool requested, bool deferred) {
  return (!requested || deferred) && state.attempted_frame == frame;
}
struct SceneReuseInputs {
  bool healthy = false, slot_valid = false, tlas_valid = false, gc = false, scene_gc = false;
  bool noise_pending = false;
  uint32_t frame = 0u, scene_frame = UINT32_MAX, records = 0u, scene_records = 0u, characters = 0u, scene_characters = 0u;
  uint64_t generation = 0u, scene_generation = 0u, snapshot = 0u, scene_snapshot = 0u, gc_version = 0u, scene_gc_version = 0u;
};
inline bool CanReuseScene(const SceneReuseInputs& in) {
  return in.healthy && in.slot_valid && in.tlas_valid && in.frame == in.scene_frame && in.generation == in.scene_generation
         && in.gc == in.scene_gc && (!in.gc || (in.gc_version != 0u && in.gc_version == in.scene_gc_version))
         && in.records == in.scene_records && in.characters == in.scene_characters
         && in.snapshot != 0u && in.snapshot == in.scene_snapshot && !in.noise_pending;
}
inline const char* SceneStatusName(SceneStatus status) {
  switch (status) {
    case SceneStatus::Off:                 return "off";
    case SceneStatus::DxrOff:              return "DXR off";
    case SceneStatus::NotCreated:          return "RT12 not created";
    case SceneStatus::NoDevice:            return "no device";
    case SceneStatus::DeferredContext:     return "deferred context";
    case SceneStatus::Guarded:             return "guarded";
    case SceneStatus::MissingSceneInput:   return "missing scene input";
    case SceneStatus::SceneBindingChanged: return "scene binding changed";
    case SceneStatus::GeometryNotReady:    return "geometry not ready";
    case SceneStatus::Reused:              return "reused";
    case SceneStatus::Submitted:           return "submitted";
    case SceneStatus::Busy:                return "busy";
    case SceneStatus::Failed:              return "failed";
    case SceneStatus::Quarantined:         return "quarantined";
  }
  return "unknown";
}
}  // namespace falcom_world::rtxdi
