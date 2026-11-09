#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>

#include "rtxdi_input_layout.h"
#include "rtxdi_surface_layout.h"

namespace falcom_world::rtxdi {

enum class SceneCaptureSource { None,
                                Push,
                                DescriptorTable };
struct SceneRangeSnapshot {
  uint64_t buffer = 0u, offset = 0u, size = 0u, frame = UINT64_MAX;
  uintptr_t command = 0u;
  SceneCaptureSource source = SceneCaptureSource::None;
  bool live = false;
  bool known = false;
  uint64_t sequence = 0u;
  uint64_t reset_generation = 0u;
};
struct SceneRangeCapture {
  std::shared_mutex mutex;
  std::unordered_map<uintptr_t, SceneRangeSnapshot> bindings;
  std::unordered_map<uintptr_t, uint64_t> reset_generations;
  SceneRangeSnapshot snapshot;
  uint64_t sequence = 0u;
};

inline void TrackSceneBinding(SceneRangeCapture* tracker, SceneRangeSnapshot binding) {
  std::unique_lock lock(tracker->mutex);
  binding.sequence = ++tracker->sequence;
  binding.reset_generation = tracker->reset_generations[binding.command];
  tracker->bindings[binding.command] = binding;
  if (tracker->snapshot.command == binding.command) tracker->snapshot.live = false;
}

inline void ResetSceneBinding(SceneRangeCapture* tracker, uintptr_t command) {
  std::unique_lock lock(tracker->mutex);
  ++tracker->reset_generations[command];
  ++tracker->sequence;
  tracker->bindings.erase(command);
  if (tracker->snapshot.command == command) tracker->snapshot.live = false;
}

inline void InvalidateSceneBuffer(SceneRangeCapture* tracker, uint64_t buffer) {
  std::unique_lock lock(tracker->mutex);
  if (tracker->snapshot.buffer == buffer) tracker->snapshot.live = false;
  for (auto& [command, binding] : tracker->bindings) {
    if (binding.buffer == buffer) {
      binding.live = false;
      binding.sequence = ++tracker->sequence;
    }
  }
}

struct TextureInputExtent {
  uint32_t width = 0u, height = 0u, mip_levels = 0u, mip = 0u, samples = 0u, array_size = 0u;
  bool texture_2d_view = false;
};

inline bool ValidTextureInput(const TextureInputExtent& input, uint32_t expected_width, uint32_t expected_height) {
  if (!input.texture_2d_view || input.samples != 1u || input.array_size != 1u || input.width == 0u || input.height == 0u
      || input.mip >= input.mip_levels || input.mip >= 32u) return false;
  return (std::max)(input.width >> input.mip, 1u) == expected_width
         && (std::max)(input.height >> input.mip, 1u) == expected_height;
}

struct StructuredInputRange {
  uint64_t buffer_bytes = 0u;
  uint32_t first_element = 0u;
  uint32_t element_count = 0u;
  uint32_t stride = 0u;
};

inline bool ValidStructuredInput(const StructuredInputRange& input, uint32_t expected_stride) {
  if (input.stride != expected_stride || expected_stride == 0u) return false;
  const uint64_t offset = uint64_t{input.first_element} * expected_stride;
  const uint64_t bytes = uint64_t{input.element_count} * expected_stride;
  return offset <= input.buffer_bytes && bytes <= input.buffer_bytes - offset;
}

inline bool NormalizeSceneInputRange(uint64_t buffer_bytes, uint64_t offset, uint64_t range_bytes, uint64_t* normalized_bytes) {
  if (normalized_bytes == nullptr || offset > buffer_bytes || (offset % 256u) != 0u) return false;
  const uint64_t available = buffer_bytes - offset;
  if (range_bytes == UINT64_MAX) {
    range_bytes = available;
  } else if ((range_bytes % 256u) != 0u || range_bytes > available) {
    return false;
  }
  if (range_bytes < RTXDI_INPUT_SCENE_ROWS * 16u) return false;
  *normalized_bytes = range_bytes;
  return true;
}

inline bool ValidSceneInputRange(uint64_t buffer_bytes, uint64_t offset, uint64_t range_bytes) {
  uint64_t normalized = 0u;
  return NormalizeSceneInputRange(buffer_bytes, offset, range_bytes, &normalized);
}

struct SceneDescriptorRange {
  uint32_t binding = 0u, register_index = 0u, register_space = 0u, count = 0u, array_size = 1u;
  bool constant_buffer = false, pixel_visible = false;
};

enum class SceneRangeMapping { Unrelated,
                               Mapped,
                               Unsupported };
inline SceneRangeMapping MapSceneDescriptorRange(const SceneDescriptorRange& range, uint32_t target, uint32_t* table_binding) {
  if (!range.constant_buffer || !range.pixel_visible || range.register_space != 0u
      || target < range.register_index || target - range.register_index >= range.count) return SceneRangeMapping::Unrelated;
  const uint64_t binding = uint64_t{range.binding} + (target - range.register_index);
  if (table_binding == nullptr || range.array_size != 1u || binding > UINT32_MAX) return SceneRangeMapping::Unsupported;
  *table_binding = static_cast<uint32_t>(binding);
  return SceneRangeMapping::Mapped;
}

inline bool SceneDescriptorIndex(uint32_t binding, uint32_t count, uint32_t target, uint32_t* index) {
  if (index == nullptr || target < binding || target - binding >= count) return false;
  *index = target - binding;
  return true;
}

inline bool SceneCaptureCurrent(uint64_t captured_frame, uint64_t frame, uintptr_t captured_command, uintptr_t command, bool live) {
  return live && captured_frame == frame && captured_command != 0u && captured_command == command;
}

inline bool CanPromoteSceneBinding(const SceneRangeSnapshot& binding, uint64_t frame, uintptr_t command, bool lighting_shader) {
  return lighting_shader && binding.known && SceneCaptureCurrent(binding.frame, frame, binding.command, command, binding.live);
}

enum class SceneBindingStatus { Ready,
                                SceneBindingChanged };

// Call with a coherent tracker snapshot, then recheck the native buffer before recording a pack.
inline SceneBindingStatus CheckPromotedSceneBinding(const SceneRangeSnapshot& promoted, const SceneRangeSnapshot& current,
                                                    uint64_t frame, uintptr_t command, uint64_t buffer_bytes) {
  uint64_t current_size = 0u;
  if (!CanPromoteSceneBinding(promoted, frame, command, true) || !CanPromoteSceneBinding(current, frame, command, true)
      || promoted.sequence != current.sequence || promoted.reset_generation != current.reset_generation
      || promoted.buffer != current.buffer || promoted.offset != current.offset
      || !NormalizeSceneInputRange(buffer_bytes, current.offset, current.size, &current_size)
      || promoted.size != current_size) {
    return SceneBindingStatus::SceneBindingChanged;
  }
  return SceneBindingStatus::Ready;
}

inline bool TransferSlotAvailable(uint64_t slot_fence, uint64_t completed_fence) {
  return slot_fence <= completed_fence;
}

inline bool TransferAttemptDuplicate(uint32_t attempted_frame, uint64_t attempted_generation,
                                     uint32_t frame, uint64_t generation) {
  return attempted_frame == frame && attempted_generation == generation;
}

enum class SunVisibilityStatus : uint32_t { Unknown = RTXDI_SUN_STATUS_UNKNOWN,
                                            Visible = RTXDI_SUN_STATUS_VISIBLE,
                                            Blocked = RTXDI_SUN_STATUS_BLOCKED };

inline bool SunVisibilityRequested(uint32_t request_mask) { return (request_mask & 2u) != 0u; }

inline bool ValidSceneLease(int32_t current_slot, uint32_t current_frame, int32_t leased_slot, uint32_t leased_frame,
                            uint64_t current_fence, uint64_t leased_fence) {
  return current_slot >= 0 && current_slot == leased_slot && current_frame == leased_frame
         && current_fence != 0u && current_fence == leased_fence;
}

inline bool SceneCoverageSnapshotMatches(int32_t current_slot, uint32_t current_frame, uint64_t current_generation,
                                          uint64_t current_fence, uint64_t current_gc_version, int32_t snapshot_slot,
                                          uint32_t snapshot_frame, uint64_t snapshot_generation,
                                          uint64_t snapshot_fence, uint64_t snapshot_gc_version) {
  return current_slot >= 0 && current_slot == snapshot_slot && current_frame == snapshot_frame
         && current_generation == snapshot_generation && current_fence != 0u && current_fence == snapshot_fence
         && current_gc_version == snapshot_gc_version;
}

inline bool CheckedSunResultBytes(uint32_t width, uint32_t height, uint64_t* bytes) {
  if (bytes == nullptr || width == 0u || height == 0u) return false;
  const uint64_t pixels = uint64_t{width} * height;
  if (pixels > UINT32_MAX || pixels > UINT64_MAX / RTXDI_SUN_RESULT_STRIDE) return false;
  *bytes = pixels * RTXDI_SUN_RESULT_STRIDE;
  return true;
}

inline SunVisibilityStatus ResolveSunVisibility(bool hit, float hit_distance, bool alpha_metadata_valid,
                                                float requested_distance, float covered_distance) {
  if (!alpha_metadata_valid || !std::isfinite(requested_distance) || requested_distance <= 0.f
      || !std::isfinite(covered_distance) || covered_distance <= 0.f) return SunVisibilityStatus::Unknown;
  if (hit) {
    if (!std::isfinite(hit_distance) || hit_distance < 0.f || hit_distance > covered_distance) {
      return SunVisibilityStatus::Unknown;
    }
    return SunVisibilityStatus::Blocked;
  }
  return (covered_distance >= requested_distance ? SunVisibilityStatus::Visible : SunVisibilityStatus::Unknown);
}

inline bool MakeInputLayout(uint32_t width, uint32_t height, bool motion_available, bool local_requested,
                            uint32_t lights, uint32_t tiles, uint32_t deferred, RtxdiInputLayout* out) {
  if (out == nullptr || width == 0u || height == 0u) return false;
  if (local_requested && (tiles > RTXDI_INPUT_MAX_TILES || deferred > RTXDI_INPUT_MAX_DEFERRED)) return false;
  *out = {};
  out->width = width;
  out->height = height;
  out->motion_available = motion_available;
  out->flags = local_requested;
  if (local_requested) {
    out->lights_source = lights;
    out->lights_copied = (std::min)(lights, RTXDI_INPUT_MAX_LIGHTS);
    out->lights_truncated = lights - out->lights_copied;
    out->tiles_source = out->tiles_copied = tiles;
    out->deferred_source = out->deferred_copied = deferred;
  }
  return true;
}

}  // namespace falcom_world::rtxdi
