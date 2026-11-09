#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

#include "rtxdi_diagnostics.hpp"
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

struct SceneCpuMirror {
  uintptr_t device = 0u;
  uint64_t buffer = 0u, offset = 0u, size = 0u;
  uint64_t binding_sequence = 0u, write_sequence = 0u, last_read_sequence = 0u;
  uint64_t binding_frame = UINT64_MAX, write_frame = UINT64_MAX;
  std::vector<uint8_t> bytes;
  std::vector<uint8_t> coverage;
  const uint8_t* mapped = nullptr;
  uint64_t mapped_offset = 0u, mapped_size = 0u;
  bool valid = false, immutable_initial = false;
};

struct SceneCpuMirrorRegistry {
  std::mutex mutex;
  std::vector<SceneCpuMirror> mirrors;
};

inline SceneCpuMirrorRegistry& GetSceneCpuMirrorRegistry() {
  static SceneCpuMirrorRegistry registry;
  return registry;
}

inline SceneCpuMirror* FindSceneCpuMirror(std::vector<SceneCpuMirror>& mirrors, uintptr_t device, uint64_t buffer) {
  for (auto& mirror : mirrors) {
    if (mirror.device == device && mirror.buffer == buffer) return &mirror;
  }
  return nullptr;
}

inline void RegisterSceneCpuMirrorRange(uintptr_t device, uint64_t buffer, uint64_t offset, uint64_t size,
                                        uint64_t frame, uint64_t binding_sequence) {
  if (device == 0u || buffer == 0u || size != RTXDI_SURFACE_SCENE_ROW_COUNT * 16u
      || offset > UINT64_MAX - size || binding_sequence == 0u) return;
  auto& registry = GetSceneCpuMirrorRegistry();
  std::lock_guard lock(registry.mutex);
  SceneCpuMirror* mirror = FindSceneCpuMirror(registry.mirrors, device, buffer);
  if (mirror == nullptr) {
    registry.mirrors.push_back({.device = device, .buffer = buffer});
    mirror = &registry.mirrors.back();
  }
  if (mirror->offset != offset || mirror->size != size || mirror->bytes.size() != size
      || mirror->coverage.size() != size) {
    mirror->offset = offset;
    mirror->size = size;
    mirror->bytes.assign(static_cast<size_t>(size), 0u);
    mirror->coverage.assign(static_cast<size_t>(size), 0u);
    mirror->write_sequence = 0u;
    mirror->last_read_sequence = 0u;
    mirror->write_frame = UINT64_MAX;
    mirror->mapped = nullptr;
    mirror->valid = mirror->immutable_initial = false;
  }
  mirror->binding_frame = frame;
  mirror->binding_sequence = binding_sequence;
}

inline void MirrorSceneCpuWriteLocked(SceneCpuMirror* mirror, const void* data, uint64_t write_offset,
                                      uint64_t write_size, uint64_t frame) {
  if (data == nullptr || write_size == 0u || write_offset > UINT64_MAX - write_size) return;
  if (mirror == nullptr || mirror->size == 0u) return;
  if (mirror->immutable_initial) {
    std::fill(mirror->coverage.begin(), mirror->coverage.end(), 0u);
    mirror->valid = false;
    mirror->immutable_initial = false;
  }
  const uint64_t write_end = write_offset + write_size;
  const uint64_t mirror_end = mirror->offset + mirror->size;
  const uint64_t begin = (std::max)(write_offset, mirror->offset);
  const uint64_t end = (std::min)(write_end, mirror_end);
  if (begin >= end) return;
  const uint64_t destination_offset = begin - mirror->offset;
  const uint64_t source_offset = begin - write_offset;
  const size_t count = static_cast<size_t>(end - begin);
  std::memcpy(mirror->bytes.data() + destination_offset,
              static_cast<const uint8_t*>(data) + source_offset, count);
  std::fill(mirror->coverage.begin() + static_cast<std::ptrdiff_t>(destination_offset),
            mirror->coverage.begin() + static_cast<std::ptrdiff_t>(destination_offset + count), 1u);
  if (mirror->write_sequence != UINT64_MAX) ++mirror->write_sequence;
  mirror->write_frame = frame;
  mirror->valid = std::all_of(mirror->coverage.begin(), mirror->coverage.end(), [](uint8_t value) { return value != 0u; });
}

inline void SeedSceneCpuMirrorInitial(uintptr_t device, uint64_t buffer, uint64_t offset, uint64_t size,
                                      const void* data) {
  if (data == nullptr || size != RTXDI_SURFACE_SCENE_ROW_COUNT * 16u || offset > UINT64_MAX - size) return;
  auto& registry = GetSceneCpuMirrorRegistry();
  std::lock_guard lock(registry.mutex);
  SceneCpuMirror* mirror = FindSceneCpuMirror(registry.mirrors, device, buffer);
  if (mirror == nullptr || mirror->offset != offset || mirror->size != size) return;
  std::memcpy(mirror->bytes.data(), data, static_cast<size_t>(size));
  std::fill(mirror->coverage.begin(), mirror->coverage.end(), 1u);
  mirror->write_sequence = (std::max)(mirror->write_sequence, uint64_t{1u});
  mirror->last_read_sequence = 0u;
  mirror->write_frame = UINT64_MAX;
  mirror->valid = true;
  mirror->immutable_initial = true;
}

inline void MirrorSceneCpuWrite(uintptr_t device, uint64_t buffer, const void* data, uint64_t write_offset,
                                uint64_t write_size, uint64_t frame) {
  auto& registry = GetSceneCpuMirrorRegistry();
  std::lock_guard lock(registry.mutex);
  MirrorSceneCpuWriteLocked(FindSceneCpuMirror(registry.mirrors, device, buffer), data, write_offset, write_size, frame);
}

inline void BeginSceneCpuMap(uintptr_t device, uint64_t buffer, uint64_t offset, uint64_t size, const void* data) {
  auto& registry = GetSceneCpuMirrorRegistry();
  std::lock_guard lock(registry.mutex);
  SceneCpuMirror* mirror = FindSceneCpuMirror(registry.mirrors, device, buffer);
  if (mirror == nullptr) return;
  mirror->mapped = static_cast<const uint8_t*>(data);
  mirror->mapped_offset = offset;
  mirror->mapped_size = size;
}

inline void FinishSceneCpuMap(uintptr_t device, uint64_t buffer, uint64_t frame) {
  auto& registry = GetSceneCpuMirrorRegistry();
  std::lock_guard lock(registry.mutex);
  SceneCpuMirror* mirror = FindSceneCpuMirror(registry.mirrors, device, buffer);
  if (mirror == nullptr || mirror->mapped == nullptr) return;
  const uint8_t* data = mirror->mapped;
  const uint64_t offset = mirror->mapped_offset;
  const uint64_t size = mirror->mapped_size;
  mirror->mapped = nullptr;
  mirror->mapped_offset = 0u;
  mirror->mapped_size = 0u;
  const uint64_t write_size = size == UINT64_MAX ? mirror->offset + mirror->size : size;
  if (size != UINT64_MAX || offset == 0u) MirrorSceneCpuWriteLocked(mirror, data, offset, write_size, frame);
}

inline bool ReadSceneCpuMirror(uintptr_t device, uint64_t buffer, uint64_t offset, uint64_t size,
                               uint64_t frame, uint64_t binding_sequence, void* output,
                               uint64_t* write_sequence, uint64_t* write_frame) {
  if (output == nullptr || size == 0u) return false;
  auto& registry = GetSceneCpuMirrorRegistry();
  std::lock_guard lock(registry.mutex);
  SceneCpuMirror* mirror = FindSceneCpuMirror(registry.mirrors, device, buffer);
  if (mirror == nullptr || !mirror->valid || mirror->offset != offset || mirror->size != size
      || mirror->binding_frame != frame || mirror->binding_sequence != binding_sequence || mirror->write_sequence == 0u
      || (!mirror->immutable_initial && mirror->write_sequence <= mirror->last_read_sequence)) return false;
  std::memcpy(output, mirror->bytes.data(), static_cast<size_t>(size));
  if (write_sequence != nullptr) *write_sequence = mirror->write_sequence;
  if (write_frame != nullptr) *write_frame = mirror->write_frame;
  if (!mirror->immutable_initial) {
    mirror->last_read_sequence = mirror->write_sequence;
    std::fill(mirror->coverage.begin(), mirror->coverage.end(), 0u);
    mirror->valid = false;
  }
  return true;
}

inline void InvalidateSceneCpuMirror(uintptr_t device, uint64_t buffer) {
  auto& registry = GetSceneCpuMirrorRegistry();
  std::lock_guard lock(registry.mutex);
  SceneCpuMirror* mirror = FindSceneCpuMirror(registry.mirrors, device, buffer);
  if (mirror != nullptr) {
    mirror->valid = false;
    mirror->mapped = nullptr;
    mirror->offset = 0u;
    mirror->size = 0u;
    mirror->bytes.clear();
    mirror->coverage.clear();
    mirror->write_sequence = 0u;
    mirror->last_read_sequence = 0u;
    mirror->write_frame = UINT64_MAX;
    mirror->immutable_initial = false;
  }
}

inline void ResetSceneCpuMirrorsForDevice(uintptr_t device) {
  auto& registry = GetSceneCpuMirrorRegistry();
  std::lock_guard lock(registry.mutex);
  std::erase_if(registry.mirrors, [&](const SceneCpuMirror& mirror) { return mirror.device == device; });
}

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

struct SceneBindingEvidence {
  SceneRangeSnapshot promoted = {};
  SceneRangeSnapshot current = {};
  uint64_t buffer_bytes = 0u;
  uint64_t normalized_size = 0u;
  DiagnosticReason reason = DiagnosticReason::None;
  bool normalized_valid = false;
};

struct SceneRangeEvidence {
  SceneBindingEvidence binding = {};
  uint64_t expected_buffer = 0u;
  bool entry_found = false;
  bool resource_desc_valid = false;
  bool resource_buffer = false;
};

// Call with a coherent tracker snapshot, then recheck the native buffer before recording a pack.
inline SceneBindingStatus CheckPromotedSceneBinding(const SceneRangeSnapshot& promoted, const SceneRangeSnapshot& current,
                                                    uint64_t frame, uintptr_t command, uint64_t buffer_bytes,
                                                    SceneBindingEvidence* evidence = nullptr) {
  uint64_t current_size = 0u;
  if (evidence != nullptr) {
    evidence->promoted = promoted;
    evidence->current = current;
    evidence->buffer_bytes = buffer_bytes;
  }
  const auto binding_reason = [&](const SceneRangeSnapshot& binding) {
    if (!binding.known) return DiagnosticReason::UnknownBinding;
    if (!binding.live) return DiagnosticReason::NonliveBinding;
    if (binding.frame != frame) return DiagnosticReason::StaleFrame;
    if (binding.command == 0u || binding.command != command) return DiagnosticReason::CommandMismatch;
    return DiagnosticReason::MissingEntry;
  };
  if (!CanPromoteSceneBinding(promoted, frame, command, true)) {
    if (evidence != nullptr) evidence->reason = binding_reason(promoted);
    return SceneBindingStatus::SceneBindingChanged;
  }
  if (!CanPromoteSceneBinding(current, frame, command, true)) {
    if (evidence != nullptr) evidence->reason = binding_reason(current);
    return SceneBindingStatus::SceneBindingChanged;
  }
  if (promoted.sequence != current.sequence) {
    if (evidence != nullptr) evidence->reason = DiagnosticReason::SequenceMismatch;
    return SceneBindingStatus::SceneBindingChanged;
  }
  if (promoted.reset_generation != current.reset_generation) {
    if (evidence != nullptr) evidence->reason = DiagnosticReason::ResetMismatch;
    return SceneBindingStatus::SceneBindingChanged;
  }
  if (promoted.buffer != current.buffer) {
    if (evidence != nullptr) evidence->reason = DiagnosticReason::BufferMismatch;
    return SceneBindingStatus::SceneBindingChanged;
  }
  if (promoted.offset != current.offset) {
    if (evidence != nullptr) evidence->reason = DiagnosticReason::OffsetMismatch;
    return SceneBindingStatus::SceneBindingChanged;
  }
  if (!NormalizeSceneInputRange(buffer_bytes, current.offset, current.size, &current_size)) {
    if (evidence != nullptr) {
      evidence->reason = DiagnosticReason::InvalidRange;
      evidence->normalized_valid = false;
    }
    return SceneBindingStatus::SceneBindingChanged;
  }
  if (evidence != nullptr) {
    evidence->normalized_valid = true;
    evidence->normalized_size = current_size;
  }
  if (promoted.size != current_size) {
    if (evidence != nullptr) evidence->reason = DiagnosticReason::NormalizedSizeMismatch;
    return SceneBindingStatus::SceneBindingChanged;
  }
  if (evidence != nullptr) evidence->reason = DiagnosticReason::None;
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
