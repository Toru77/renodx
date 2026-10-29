#pragma once

// "Gather performance metrics" (Performance tab). Session only, default OFF. While OFF no stopwatch reads the clock,
// no timing is written and no timing is summed: PerfNow() returns the epoch and PoolCpuTimer does nothing.

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>

namespace falcom_world::bvh {

// Stopwatch blocks (the block rows of perf_run.hpp, same order). Blocks 0 to 11 run inside a present bracket and are disjoint
// from each other and from the InPresent rows (they reduce the unattributed CPU). Blocks 12 and up are subsets of the rows
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
  kPerfBlockCompactObservations,  // CompactPool: observations of invalidated keys
  kPerfBlockCompactInstances,     // CompactPool: instance removal and admitted keys
  kPerfBlockCompactRearm,         // CompactPool: re-arm of observations removed through another key
  kPerfBlockOrphanScan,           // RetirePoolOrphans: candidate scan
  kPerfBlockOrphanNewest,         // PoolNewestSeenByKey (RetirePoolOrphans)
  kPerfBlockDrawHookTotal,      // WorldDrawCallback, whole (direct and indirect draws)
  kPerfBlockDrawHookRecord,     // WorldDrawCallback: DrawRecord fill to CommitDrawRecord (direct draws)
  kPerfBlockDeformProbeDraw,    // OnDeformProbeDraw and OnDeformProbeIndirectDraw calls
  kPerfBlockCbHooks,            // cb tracker map, unmap and update hooks
  kPerfBlockBindHook,           // OnPushDescriptorsWorld
  kPerfBlockPoolWriteHooks,     // NotePoolBufferWrite
  kPerfBlockRtaoInline,         // RunRtaoInline, whole (rtao_record_cpu is inside)
  kPerfBlockLightingRt,         // the RT block of the lighting hook (snapshot, RTAO, RTLS, RT sun, RTGI, restore)
  kPerfBlockRt12SignalFlush,    // RunRtaoPassA: fence11 signal A and ctx11 Flush
  kPerfBlockRt12Submit,         // RunRtaoPassA: D3D12 waits, ExecuteCommandLists, signal B
  kPerfBlockRt12WaitB,          // RunRtaoPassA: D3D11 wait on fence B
  kPerfBlockSegToRt,            // wall clock: finish_present to the lighting RT block (or to the present, no RT block)
  kPerfBlockSegAfterRt,         // wall clock: end of the lighting RT block to OnWorldPresentBvh
  kPerfBlockSegPresent,         // wall clock: OnWorldPresentBvh to finish_present
  kPerfBlockDrawHookIndirectRecord,  // WorldDrawCallback: DrawRecord fill of indirect draws (FillCommonDrawRecord to FillDrawBindingMasks)
  kPerfBlockRecordShader,            // FillCommonDrawRecord: shader state read
  kPerfBlockRecordRender,            // FillCommonDrawRecord: render state read and pipeline lookups
  kPerfBlockRecordSkin,              // FillCommonDrawRecord: LayoutHasSkinInputs
  kPerfBlockRecordScene,             // FillCommonDrawRecord: scene read and QueryBufferInfo
  kPerfBlockRecordRest,              // WorldDrawCallback direct: candidate, FillDrawBindingMasks and CommitDrawRecord
  kPerfBlockPoolGate,                // pool draw hook: GatePoolDraw and wind read
  kPerfBlockPoolLock,                // pool draw hook: bookkeeping under the pool lock
  kPerfBlockPoolCopy,                // pool draw hook: IssuePoolCopies and LogPoolMeshCopies
  kPerfBlockPoolGc,                  // pool draw hook: GPU capture note
  kPerfBlockPoolTail,                // pool draw hook: P2b slice copy and alpha source capture, after the lock
  kPerfBlockPoolIndGate,             // pool indirect hook: GatePoolDraw, wind read and stats, alpha read, slice, args size
  kPerfBlockPoolIndLock,             // pool indirect hook: sub-draw bookkeeping under the pool lock
  kPerfBlockPoolIndTail,             // pool indirect hook: P2b copies and alpha source capture, after the lock
  kPerfBlockPoolIndCopy,             // pool indirect hook: IssuePoolCopies and LogPoolMeshCopies
  kPerfBlockPoolIndGc,               // pool indirect hook: GPU capture of the sub-draws and its stats
  kPerfBlockGcPresent,     // rt12::OnGcPresent, whole (OnWorldPresentBvh)
  kPerfBlockRt12Present,   // rt12::OnPresent, whole (OnWorldPresentBvh)
  kPerfBlockDrawHookPre,   // WorldDrawCallback direct: hook start to record start
  kPerfBlockRtaoPost,      // rtao::Dispatch after pass A: upsample, pass B, filter, table nulls
  kPerfBlockRt12PassA,     // rt12::RunRtaoPassA, whole
  kPerfBlockRt12D3d11Passes,  // RunRtaoPassA steps 10-11: D3D11 noise copy and passes
  kPerfBlockRt12Record,    // RunRtaoPassA step 13: D3D12 recording to Close
  kPerfBlockRt12Resolve,   // RunRtaoPassA step 15: D3D11 resolve
  kPerfBlockCount
};

struct PerfGate {
  std::atomic_bool gather{false};
  std::atomic_bool clear_pending{false};  // set when gather goes OFF: the next present clears what was gathered
  // The CPU time counters are nanoseconds (PoolCpuTimer); the _us suffix predates the unit.
  std::atomic_uint64_t present_world_us{0u};  // cumulative CPU time of OnWorldPresent (bracket)
  std::atomic_uint64_t present_bvh_us{0u};    // cumulative CPU time of OnWorldPresentBvh (bracket)
  std::atomic_uint64_t alpha_sync_us{0u};     // SyncLiveAlpha CPU time of the last present
  std::atomic_uint64_t rtao_record_us{0u};    // rtao::Dispatch CPU time (command recording)
  std::atomic_uint64_t rtgi_record_us{0u};    // rtgi::Dispatch CPU time (command recording)
  std::atomic_uint64_t rtls_record_us{0u};    // rtls::Dispatch CPU time (command recording)
  std::atomic_uint64_t rtsun_record_us{0u};   // rtsun::Dispatch CPU time (command recording)
  std::atomic_uint64_t deform_update_us{0u};  // UpdateDeformLive CPU time of the last present
  std::atomic_uint64_t block_us[kPerfBlockCount]{};  // CPU time of each PerfBlock (cumulative nanoseconds; written by PoolCpuTimer or AddPerfBlockMs)
  std::atomic_uint64_t hook_draws{0u};        // draws that reached the pool path of WorldDrawCallback (gather only)
  std::atomic_uint64_t hook_draws_frame{0u};  // hook_draws of the last frame (moved by SamplePerfPresent)
  std::atomic_uint64_t hook_draws_indirect{0u};  // indirect draws that reached WorldDrawCallback (gather only)
  std::atomic_uint64_t hook_draws_indirect_frame{0u};  // hook_draws_indirect of the last frame (moved by SamplePerfPresent)
  std::atomic_uint64_t hook_draws_unconsumed{0u};  // direct draws skipped by the consumed rule (WorldDrawCallback; count only while gathering)
  std::atomic_uint64_t hook_draws_unconsumed_frame{0u};  // hook_draws_unconsumed of the last frame (moved by SamplePerfPresent)
  std::atomic_uint64_t bind_calls{0u};  // push_descriptors calls past WorldDrawHookNeeded (gather only)
  std::atomic_uint64_t bind_calls_frame{0u};  // bind_calls of the last frame (moved by SamplePerfPresent)
  std::atomic_uint64_t bind_srv_resolves{0u};  // get_resource_from_view calls in OnPushDescriptorsWorld (gather only)
  std::atomic_uint64_t bind_srv_resolves_frame{0u};  // bind_srv_resolves of the last frame (moved by SamplePerfPresent)
  std::atomic_uint64_t record_native_seq{0u};  // draws seen by the native-record parity gate (gather only; every 64th also builds the framework state)
  std::atomic_uint64_t record_mismatch_rtv0{0u};  // rtv0 differs between the native read and the framework state (gather only; every 64th draw)
  std::atomic_uint64_t record_mismatch_rtv0_frame{0u};  // record_mismatch_rtv0 of the last frame (moved by SamplePerfPresent)
  std::atomic_uint64_t record_mismatch_dsv{0u};
  std::atomic_uint64_t record_mismatch_dsv_frame{0u};  // record_mismatch_dsv of the last frame (moved by SamplePerfPresent)
  std::atomic_uint64_t record_mismatch_topology{0u};
  std::atomic_uint64_t record_mismatch_topology_frame{0u};  // record_mismatch_topology of the last frame (moved by SamplePerfPresent)
  std::atomic_uint64_t record_mismatch_input_layout{0u};
  std::atomic_uint64_t record_mismatch_input_layout_frame{0u};  // record_mismatch_input_layout of the last frame (moved by SamplePerfPresent)
  std::atomic_uint64_t record_mismatch_has_viewport{0u};
  std::atomic_uint64_t record_mismatch_has_viewport_frame{0u};  // record_mismatch_has_viewport of the last frame (moved by SamplePerfPresent)
  std::atomic_uint64_t record_mismatch_viewport{0u};
  std::atomic_uint64_t record_mismatch_viewport_frame{0u};  // record_mismatch_viewport of the last frame (moved by SamplePerfPresent)
  std::atomic_uint64_t record_mismatch_blend_enable{0u};
  std::atomic_uint64_t record_mismatch_blend_enable_frame{0u};  // record_mismatch_blend_enable of the last frame (moved by SamplePerfPresent)
  std::atomic_uint64_t record_mismatch_depth_enable{0u};
  std::atomic_uint64_t record_mismatch_depth_enable_frame{0u};  // record_mismatch_depth_enable of the last frame (moved by SamplePerfPresent)
  std::atomic_uint64_t record_mismatch_depth_write{0u};
  std::atomic_uint64_t record_mismatch_depth_write_frame{0u};  // record_mismatch_depth_write of the last frame (moved by SamplePerfPresent)
  std::atomic_uint64_t record_mismatch_cull_mode{0u};
  std::atomic_uint64_t record_mismatch_cull_mode_frame{0u};  // record_mismatch_cull_mode of the last frame (moved by SamplePerfPresent)
  std::atomic_uint64_t record_mismatch_front_counter_clockwise{0u};
  std::atomic_uint64_t record_mismatch_front_counter_clockwise_frame{0u};  // record_mismatch_front_counter_clockwise of the last frame (moved by SamplePerfPresent)
  // Frame segment marks: render thread only (the immediate-list lighting hook, the present and finish_present handlers).
  std::chrono::steady_clock::time_point frame_start{};    // finish_present of the previous frame
  std::chrono::steady_clock::time_point rt_end{};         // end of the last immediate-list lighting RT block this frame
  std::chrono::steady_clock::time_point present_start{};  // start of OnWorldPresentBvh
  bool rt_seen = false;                                    // an immediate-list lighting RT block ran this frame
};

inline PerfGate g_perf_gate;

inline std::chrono::steady_clock::time_point PerfNow() {
  return g_perf_gate.gather.load(std::memory_order_relaxed) ? std::chrono::steady_clock::now() : std::chrono::steady_clock::time_point{};
}

// Adds a block time that was measured with PerfNow, in ms (for a value that is not a plain scope). Nothing while gathering is off.
inline void AddPerfBlockMs(PerfBlock block, double ms) {
  if (g_perf_gate.gather.load(std::memory_order_relaxed)) {
    g_perf_gate.block_us[block].fetch_add(static_cast<uint64_t>((std::max)(ms, 0.0) * 1000000.0), std::memory_order_relaxed);
  }
}

// Adds the CPU time of its scope to *total_us in nanoseconds (steady_clock). The *_us counter names predate the unit.
struct PoolCpuTimer {
  std::atomic_uint64_t* total_us;
  const bool armed = g_perf_gate.gather.load(std::memory_order_relaxed);
  std::chrono::steady_clock::time_point start = armed ? std::chrono::steady_clock::now() : std::chrono::steady_clock::time_point{};
  ~PoolCpuTimer() {
    if (!armed) return;
    total_us->fetch_add(static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
                                                  std::chrono::steady_clock::now() - start).count()),
                        std::memory_order_relaxed);
  }
};

}  // namespace falcom_world::bvh
