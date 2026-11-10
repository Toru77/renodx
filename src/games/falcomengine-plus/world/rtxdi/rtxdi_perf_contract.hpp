#pragma once

#include <atomic>
#include <cmath>
#include <cstdint>
#include <limits>

namespace falcom_world::rtxdi {

struct GpuPerfIdentity {
  uint64_t gather_epoch = 0u;
  uint64_t owner_generation = 0u;
  uint64_t transfer_configuration = 0u;
  uint64_t di_configuration = 0u;
  uint64_t settings_identity = 0u;
  uint64_t accepted_fence = 0u;
  uint32_t frame = UINT32_MAX;
  uint32_t width = 0u;
  uint32_t height = 0u;
  uint8_t valid_mask = 0u;
};

inline bool CanStartGpuTiming(bool gather, uint64_t timestamp_frequency, bool slot_pending) {
  return gather && timestamp_frequency != 0u && !slot_pending;
}

inline bool ShouldCountAcceptedGpuSubmission(bool gather, bool epoch_current, uint64_t signal,
                                             uint64_t no_signal) {
  return gather && epoch_current && signal != 0u && signal != no_signal;
}

inline bool CountCompletedGpuTimingBatch(bool has_valid_interval, uint32_t* completed_batches) {
  if (!has_valid_interval || completed_batches == nullptr || *completed_batches == UINT32_MAX) return false;
  ++*completed_batches;
  return true;
}

inline void RecordGpuTimingSkipEvent(std::atomic_uint64_t* skip_events) {
  if (skip_events != nullptr) skip_events->fetch_add(1u, std::memory_order_relaxed);
}

inline bool ClaimCompletedGpuTiming(bool* pending, bool completion_available, uint64_t fence, uint64_t completed) {
  if (pending == nullptr || !*pending || !completion_available || fence == 0u || fence > completed) return false;
  *pending = false;
  return true;
}

inline bool MatchesGpuPerfIdentity(const GpuPerfIdentity& sample, const GpuPerfIdentity& current) {
  return sample.gather_epoch != 0u && sample.gather_epoch == current.gather_epoch
         && sample.owner_generation != 0u && sample.owner_generation == current.owner_generation
         && sample.transfer_configuration == current.transfer_configuration
         && sample.di_configuration == current.di_configuration
         && sample.settings_identity == current.settings_identity && sample.accepted_fence != 0u
         && sample.accepted_fence == current.accepted_fence && sample.frame == current.frame
         && sample.width != 0u && sample.width == current.width && sample.height != 0u
         && sample.height == current.height && sample.valid_mask != 0u
         && sample.valid_mask == current.valid_mask;
}

inline bool DecodeGpuTimestampPair(uint64_t begin, uint64_t end, uint64_t frequency, double* milliseconds) {
  if (milliseconds == nullptr || frequency == 0u || end <= begin) return false;
  const double value = static_cast<double>(end - begin) * 1000.0 / static_cast<double>(frequency);
  if (!std::isfinite(value) || value < 0.0) return false;
  *milliseconds = value;
  return true;
}

inline bool AccumulateGpuSample(double milliseconds, double* sum, uint32_t* samples) {
  if (sum == nullptr || samples == nullptr || !std::isfinite(milliseconds) || milliseconds < 0.0
      || !std::isfinite(*sum) || *sum < 0.0 || *samples == UINT32_MAX) return false;
  const double next = *sum + milliseconds;
  if (!std::isfinite(next)) return false;
  *sum = next;
  ++*samples;
  return true;
}

}  // namespace falcom_world::rtxdi
