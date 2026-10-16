#pragma once

// "Gather performance metrics" (Performance tab). Session only, default OFF. While OFF no stopwatch reads the clock,
// no timing is written and no timing is summed: PerfNow() returns the epoch and PoolCpuTimer does nothing.

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>

namespace falcom_world::bvh {

// Stopwatch blocks (the block rows of perf_run.hpp, same order). Blocks 0 to 11 run inside a present bracket and are disjoint
// from each other and from the InPresent rows (they reduce the unattributed CPU). Blocks 12 to 19 are subsets of the rows
// above (parts of a row or of a block, never summed).
enum PerfBlock : size_t {
  kPerfBlockPerfSample,         // SamplePerfPresent (counted in the next sample)
  kPerfBlockSyncMeshes,         // SyncLiveMeshes, minus the BLAS builds inside it
  kPerfBlockRebuildNoop,        // the no-op tests of RebuildLiveTlas (cache diagnostics)
  kPerfBlockRecordTlas,         // RecordCacheTlasRebuild (after the TLAS timing)
  kPerfBlockGpuCheck,           // CheckLiveBvhOnGpu (on request)
  kPerfBlockCacheDiag,          // UpdateCacheDiag
  kPerfBlockRtaoStats,          // MaybeCaptureRtaoStats (on request)
  kPerfBlockDebugPass,          // RunBvhDebugPass (while a debug view is on)
  kPerfBlockResolveIndirect,    // ResolveIndirectDraws (includes the GPU wait)
  kPerfBlockP2bRecord,          // ExpandLiveDynamic after its gate
  kPerfBlockP2bDiag,            // RunP2bDiagnostics (on request)
  kPerfBlockDeformProbe,        // DrainDeformProbe after its gate
  kPerfBlockRebuildCall,        // RebuildLiveTlas, whole
  kPerfBlockCacheTiers,         // ComputeCacheTiers (sampled presents)
  kPerfBlockDrainCompact,       // CompactPool and the Apply*Switch calls
  kPerfBlockDrainResolve,       // ResolvePoolSlot
  kPerfBlockDrainPrune,         // PrunePool
  kPerfBlockDrainRetireOrphans, // RetirePoolOrphans
  kPerfBlockDrainPoolStats,     // UpdatePoolStats
  kPerfBlockFollowScan,         // the follow scan of FollowPoolMovingInstance (per moving sighting)
  kPerfBlockCount
};

struct PerfGate {
  std::atomic_bool gather{false};
  std::atomic_bool clear_pending{false};  // set when gather goes OFF: the next present clears what was gathered
  std::atomic_uint64_t present_world_us{0u};  // cumulative CPU time of OnWorldPresent (bracket)
  std::atomic_uint64_t present_bvh_us{0u};    // cumulative CPU time of OnWorldPresentBvh (bracket)
  std::atomic_uint64_t alpha_sync_us{0u};     // SyncLiveAlpha CPU time of the last present
  std::atomic_uint64_t rtao_record_us{0u};    // rtao::Dispatch CPU time (command recording)
  std::atomic_uint64_t deform_update_us{0u};  // UpdateDeformLive CPU time of the last present
  std::atomic_uint64_t block_us[kPerfBlockCount]{};  // CPU time of each PerfBlock (cumulative; written by PoolCpuTimer or AddPerfBlockMs)
};

inline PerfGate g_perf_gate;

inline std::chrono::steady_clock::time_point PerfNow() {
  return g_perf_gate.gather.load(std::memory_order_relaxed) ? std::chrono::steady_clock::now() : std::chrono::steady_clock::time_point{};
}

// Adds a block time that was measured with PerfNow, in ms (for a value that is not a plain scope). Nothing while gathering is off.
inline void AddPerfBlockMs(PerfBlock block, double ms) {
  if (g_perf_gate.gather.load(std::memory_order_relaxed)) {
    g_perf_gate.block_us[block].fetch_add(static_cast<uint64_t>((std::max)(ms, 0.0) * 1000.0), std::memory_order_relaxed);
  }
}

}  // namespace falcom_world::bvh
