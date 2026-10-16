#pragma once

// "Gather performance metrics" (Performance tab). Session only, default OFF. While OFF no stopwatch reads the clock,
// no timing is written and no timing is summed: PerfNow() returns the epoch and PoolCpuTimer does nothing.

#include <atomic>
#include <chrono>
#include <cstdint>

namespace falcom_world::bvh {

struct PerfGate {
  std::atomic_bool gather{false};
  std::atomic_bool clear_pending{false};  // set when gather goes OFF: the next present clears what was gathered
  std::atomic_uint64_t present_world_us{0u};  // cumulative CPU time of OnWorldPresent (bracket)
  std::atomic_uint64_t present_bvh_us{0u};    // cumulative CPU time of OnWorldPresentBvh (bracket)
  std::atomic_uint64_t alpha_sync_us{0u};     // SyncLiveAlpha CPU time of the last present
  std::atomic_uint64_t rtao_record_us{0u};    // rtao::Dispatch CPU time (command recording)
  std::atomic_uint64_t deform_update_us{0u};  // UpdateDeformLive CPU time of the last present
};

inline PerfGate g_perf_gate;

inline std::chrono::steady_clock::time_point PerfNow() {
  return g_perf_gate.gather.load(std::memory_order_relaxed) ? std::chrono::steady_clock::now() : std::chrono::steady_clock::time_point{};
}

}  // namespace falcom_world::bvh
