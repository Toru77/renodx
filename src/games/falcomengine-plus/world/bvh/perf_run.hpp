#pragma once

// Performance tab and performance test (diagnostic only). The tab shows the cost of each present (now and the mean of
// the last kPerfTabWindow presents); the test is a 30 s wall-clock window whose costs are written to perf_*.json. Frame
// time is the interval between two successive presents (SamplePerfPresent). Every cost is an average per present, so two
// runs with different present counts compare fairly. The window also keeps the largest value per row (PerfMaxima)
// and, for the perf file, a per-present series; both are gather-only. Nothing is gathered while "Gather performance metrics" is off
// (perf_gate.hpp): no counter is read and no sample is written.
//
// Lock rule: g_perf.mutex is a leaf lock. It is never held together with g_cache_diag.mutex or g_pool.mutex. The pool
// lock is taken (briefly, for the counters) only while a run is active or a start is pending. File I/O and the result
// line are written after every lock is released.

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iterator>
#include <limits>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

#include "cache_diag.hpp"

namespace falcom_world::bvh {

inline constexpr double kPerfWindowSeconds = 30.0;

// Where a cost counts. InPresent: runs inside a present bracket (with DrawTime, the unattributed row and the hook outer row it
// makes the CPU total). DrawTime: runs at draw time, outside both brackets. Bracket: the two present handlers (their sum minus the
// InPresent rows is the unattributed CPU). Subset: BVH phases, parts of the rows above (never summed). Gpu: ms per
// measured frame (summed into the GPU total).
enum PerfRole : uint8_t { kPerfInPresent, kPerfDrawTime, kPerfBracket, kPerfSubset, kPerfGpu };

struct PerfCostInfo {
  const char* name;   // file key
  bool cumulative;    // the source keeps a running sum (a delta per present); otherwise the value of this present
  PerfRole role;
  bool counted;       // has an event counter (events, avg_per_event, rate in the file)
  const char* phase;  // Subset: the phase name; nullptr otherwise
};

// Cost entries, in the order of the tab.
enum PerfCost : size_t {
  kPerfTlasRebuild,
  kPerfTlasSkip,
  kPerfTlasApply,
  kPerfTlasRefit,
  kPerfBlasBuild,
  kPerfMeshCapture,
  kPerfPoolDrainExclCapture,
  kPerfP2bExpandCpu,
  kPerfAlphaSyncCpu,
  kPerfDeformUpdateCpu,
  kPerfPoolHooks,
  kPerfDeformHookCpu,
  kPerfRtaoRecordCpu,
  kPerfRtgiRecordCpu,
  kPerfRtlsRecordCpu,
  kPerfRtsunRecordCpu,
  kPerfPresentWorldCpu,
  kPerfPresentBvhCpu,
  kPerfGpuTrace,
  kPerfGpuAlphaBlit,
  kPerfGpuP2bExpand,
  kPerfGpuDeformRefit,
  kPerfGpuRtao,
  kPerfGpuRtaoFilter,
  kPerfGpuRtgi,
  kPerfGpuRtgiDenoise,
  kPerfGpuRtls,
  kPerfGpuRtlsDenoise,
  kPerfGpuRtsun,
  kPerfGpuRtsunDenoise,
  kPerfRebuildPhase0,
  kPerfRebuildPhase1,
  kPerfRebuildPhase2,
  kPerfRebuildPhase3,
  kPerfRebuildPhase4,
  kPerfRefitPhase0,
  kPerfRefitPhase1,
  kPerfRefitPhase2,
  kPerfRefitPhase3,
  kPerfSlackPhase0,
  kPerfSlackPhase1,
  kPerfSlackPhase2,
  kPerfSlackPhase3,
  kPerfDxrTlasGpu,
  kPerfDxrRecordsGpu,
  kPerfDxrCharactersGpu,
  kPerfGcBlasBuildGpu,
  kPerfDxrRtgiGpu,
  kPerfBlockRow,  // the block rows (PerfBlock order, perf_gate.hpp) follow: the first 12 are InPresent (disjoint), the rest Subset
  kPerfCostCount = kPerfBlockRow + kPerfBlockCount
};
inline constexpr PerfCostInfo kPerfCostInfo[kPerfCostCount] = {
    {"tlas_rebuild", true, kPerfInPresent, true, nullptr},
    {"tlas_skip", true, kPerfInPresent, true, nullptr},
    {"tlas_apply", true, kPerfInPresent, true, nullptr},
    {"tlas_refit", true, kPerfInPresent, true, nullptr},
    {"blas_build", true, kPerfInPresent, true, nullptr},
    {"mesh_capture", false, kPerfInPresent, false, nullptr},
    {"pool_drain_excl_capture", false, kPerfInPresent, false, nullptr},
    {"p2b_expand_cpu", true, kPerfInPresent, false, nullptr},
    {"alpha_sync_cpu", true, kPerfInPresent, false, nullptr},
    {"deform_update_cpu", true, kPerfInPresent, false, nullptr},
    {"pool_hooks", false, kPerfDrawTime, false, nullptr},
    {"deform_hook_cpu", false, kPerfDrawTime, false, nullptr},
    {"rtao_record_cpu", true, kPerfDrawTime, false, nullptr},
    {"rtgi_record_cpu", true, kPerfDrawTime, false, nullptr},
    {"rtls_record_cpu", true, kPerfDrawTime, false, nullptr},
    {"rtsun_record_cpu", true, kPerfDrawTime, false, nullptr},
    {"present_world_cpu", true, kPerfBracket, false, nullptr},
    {"present_bvh_cpu", true, kPerfBracket, false, nullptr},
    {"gpu_trace", true, kPerfGpu, true, nullptr},
    {"gpu_alpha_blit", true, kPerfGpu, true, nullptr},
    {"gpu_p2b_expand", true, kPerfGpu, true, nullptr},
    {"gpu_deform_refit", true, kPerfGpu, true, nullptr},
    {"gpu_rtao", true, kPerfGpu, true, nullptr},
    {"gpu_rtao_filter", true, kPerfGpu, true, nullptr},
    {"gpu_rtgi", true, kPerfGpu, true, nullptr},
    {"gpu_rtgi_denoise", true, kPerfGpu, true, nullptr},
    {"gpu_rtls", true, kPerfGpu, true, nullptr},
    {"gpu_rtls_denoise", true, kPerfGpu, true, nullptr},
    {"gpu_rtsun", true, kPerfGpu, true, nullptr},
    {"gpu_rtsun_denoise", true, kPerfGpu, true, nullptr},
    {"rebuild_phase_0", true, kPerfSubset, false, kCachePhaseNames[0]},
    {"rebuild_phase_1", true, kPerfSubset, false, kCachePhaseNames[1]},
    {"rebuild_phase_2", true, kPerfSubset, false, kCachePhaseNames[2]},
    {"rebuild_phase_3", true, kPerfSubset, false, kCachePhaseNames[3]},
    {"rebuild_phase_4", true, kPerfSubset, false, kCachePhaseNames[4]},
    {"refit_phase_0", true, kPerfSubset, false, "copy"},
    {"refit_phase_1", true, kPerfSubset, false, "descriptors"},
    {"refit_phase_2", true, kPerfSubset, false, "nodes+validate"},
    {"refit_phase_3", true, kPerfSubset, false, "upload"},
    {"slack_phase_0", true, kPerfSubset, false, "diff"},
    {"slack_phase_1", true, kPerfSubset, false, "ops"},
    {"slack_phase_2", true, kPerfSubset, false, "refit+validate"},
    {"slack_phase_3", true, kPerfSubset, false, "upload"},
    {"dxr_tlas_gpu", true, kPerfSubset, false, nullptr},
    {"dxr_records_gpu", true, kPerfSubset, false, nullptr},
    {"dxr_characters_gpu", true, kPerfSubset, false, nullptr},
    {"gc_blas_build_gpu", true, kPerfSubset, false, nullptr},
    {"dxr_rtgi_gpu", true, kPerfSubset, false, nullptr},
    // Blocks (PerfBlock order). InPresent blocks are disjoint: they are in the InPresent sum, so they reduce the unattributed CPU.
    {"perf_sample", true, kPerfInPresent, false, nullptr},
    {"sync_live_meshes", true, kPerfInPresent, false, nullptr},
    {"rebuild_noop_tests", true, kPerfInPresent, false, nullptr},
    {"record_tlas_rebuild", true, kPerfInPresent, false, nullptr},
    {"gpu_check", true, kPerfInPresent, false, nullptr},
    {"cache_diag", true, kPerfInPresent, false, nullptr},
    {"rtao_stats", true, kPerfInPresent, false, nullptr},
    {"debug_pass", true, kPerfInPresent, false, nullptr},
    {"resolve_indirect", true, kPerfInPresent, false, nullptr},
    {"p2b_record", true, kPerfInPresent, false, nullptr},
    {"p2b_diag", true, kPerfInPresent, false, nullptr},
    {"deform_probe", true, kPerfInPresent, false, nullptr},
    // Subset blocks: parts of the rows above, never summed.
    {"rebuild_call", true, kPerfSubset, false, nullptr},
    {"cache_tiers", true, kPerfSubset, false, nullptr},
    {"drain_compact", true, kPerfSubset, false, nullptr},
    {"drain_resolve", true, kPerfSubset, false, nullptr},
    {"drain_prune", true, kPerfSubset, false, nullptr},
    {"drain_retire_orphans", true, kPerfSubset, false, nullptr},
    {"drain_pool_stats", true, kPerfSubset, false, nullptr},
    {"follow_scan", true, kPerfSubset, false, nullptr},
    {"compact_observations", true, kPerfSubset, false, nullptr},
    {"compact_instances", true, kPerfSubset, false, nullptr},
    {"compact_rearm", true, kPerfSubset, false, nullptr},
    {"orphan_scan", true, kPerfSubset, false, nullptr},
    {"orphan_newest", true, kPerfSubset, false, nullptr},
    {"draw_hook_total", true, kPerfSubset, false, nullptr},
    {"draw_hook_record", true, kPerfSubset, false, nullptr},
    {"deform_probe_draw", true, kPerfSubset, false, nullptr},
    {"cb_hooks", true, kPerfSubset, false, nullptr},
    {"bind_hook", true, kPerfSubset, false, nullptr},
    {"pool_write_hooks", true, kPerfSubset, false, nullptr},
    {"rtao_inline", true, kPerfSubset, false, nullptr},
    {"lighting_rt", true, kPerfSubset, false, nullptr},
    {"rt12_signal_flush", true, kPerfSubset, false, nullptr},
    {"rt12_submit", true, kPerfSubset, false, nullptr},
    {"rt12_wait_b", true, kPerfSubset, false, nullptr},
    {"seg_to_rt", true, kPerfSubset, false, nullptr},
    {"seg_after_rt", true, kPerfSubset, false, nullptr},
    {"seg_present", true, kPerfSubset, false, nullptr},
    {"draw_hook_indirect_record", true, kPerfSubset, false, nullptr},
    {"record_shader_state", true, kPerfSubset, false, nullptr},
    {"record_render_state", true, kPerfSubset, false, nullptr},
    {"record_skin_layout", true, kPerfSubset, false, nullptr},
    {"record_scene_query", true, kPerfSubset, false, nullptr},
    {"record_rest", true, kPerfSubset, false, nullptr},
    {"pool_gate", true, kPerfSubset, false, nullptr},
    {"pool_lock", true, kPerfSubset, false, nullptr},
    {"pool_copy", true, kPerfSubset, false, nullptr},
    {"pool_gc", true, kPerfSubset, false, nullptr},
    {"pool_tail", true, kPerfSubset, false, nullptr},
    {"pool_ind_gate", true, kPerfSubset, false, nullptr},
    {"pool_ind_lock", true, kPerfSubset, false, nullptr},
    {"pool_ind_tail", true, kPerfSubset, false, nullptr},
    {"pool_ind_copy", true, kPerfSubset, false, nullptr},
    {"pool_ind_gc", true, kPerfSubset, false, nullptr},
    {"gc_present", true, kPerfSubset, false, nullptr},
    {"rt12_present", true, kPerfSubset, false, nullptr},
    {"draw_hook_pre", true, kPerfSubset, false, nullptr},
    {"rtao_post", true, kPerfSubset, false, nullptr},
    {"rt12_pass_a", true, kPerfSubset, false, nullptr},
    {"rt12_d3d11_passes", true, kPerfSubset, false, nullptr},
    {"rt12_record", true, kPerfSubset, false, nullptr},
    {"rt12_resolve", true, kPerfSubset, false, nullptr},
};
inline constexpr bool PerfInTotal(const PerfCostInfo& info) {
  return info.role == kPerfInPresent || info.role == kPerfDrawTime || info.role == kPerfGpu;
}

enum PerfGauge : size_t { kPerfGaugeTlasInstances, kPerfGaugeTlasDynamic, kPerfGaugePoolInstances, kPerfGaugePoolMeshes, kPerfGaugeSlackHoles, kPerfGaugeHookDraws, kPerfGaugeHookDrawsIndirect, kPerfGaugeHookDrawsUnconsumed, kPerfGaugeRecordMismatchRtv0, kPerfGaugeRecordMismatchDsv, kPerfGaugeRecordMismatchTopology, kPerfGaugeRecordMismatchInputLayout, kPerfGaugeRecordMismatchHasViewport, kPerfGaugeRecordMismatchViewport, kPerfGaugeRecordMismatchBlendEnable, kPerfGaugeRecordMismatchDepthEnable, kPerfGaugeRecordMismatchDepthWrite, kPerfGaugeRecordMismatchCullMode, kPerfGaugeRecordMismatchFrontCounterClockwise, kPerfGaugePoolReduced, kPerfGaugeBindCalls, kPerfGaugeBindSrvResolves, kPerfGaugeCount };
inline constexpr const char* kPerfGaugeNames[kPerfGaugeCount] = {"tlas_instances", "tlas_dynamic_instances", "pool_instances", "pool_meshes", "slack_free_holes", "hook_draws", "hook_draws_indirect", "hook_draws_unconsumed", "record_mismatch_rtv0", "record_mismatch_dsv", "record_mismatch_topology", "record_mismatch_input_layout", "record_mismatch_has_viewport", "record_mismatch_viewport", "record_mismatch_blend_enable", "record_mismatch_depth_enable", "record_mismatch_depth_write", "record_mismatch_cull_mode", "record_mismatch_front_counter_clockwise", "pool_reduced", "bind_calls", "bind_srv_resolves"};

struct PerfCounters {
  double cost[kPerfCostCount] = {};
  uint64_t events[kPerfCostCount] = {};  // counted costs and GPU rows: rebuilds, skips, applies, refits, builds, GPU samples
  double gauge[kPerfGaugeCount] = {};
};

// Largest value of each row, of the frame interval and of the totals (NaN: no value). Row maxima are per present (Gpu rows:
// per measured frame). The totals are the largest per-sample totals (PerfTotals), not sums of row maxima.
struct PerfMaxima {
  double row[kPerfCostCount];
  double frame = std::numeric_limits<double>::quiet_NaN();
  double unattr = std::numeric_limits<double>::quiet_NaN();
  double cpu = std::numeric_limits<double>::quiet_NaN();
  double gpu = std::numeric_limits<double>::quiet_NaN();
  PerfMaxima() { std::fill(std::begin(row), std::end(row), std::numeric_limits<double>::quiet_NaN()); }
};

// Per-present series of a run (ms; frame_ms is the interval to the previous present). Only the first kPerfSeriesCapacity
// presents are kept (the perf file's p99 is over the same presents); the counts and maxima cover the whole window.
inline constexpr size_t kPerfSeriesCapacity = 8192;
struct PerfSeries {
  std::vector<double> frame_ms;
  std::vector<double> cpu_total;
  std::vector<double> tlas_rebuild;
  std::vector<double> tlas_refit;
  std::vector<double> blocks[kPerfBlockCount];  // every block row (ms per present), the same length as frame_ms
};

// One present, for the worst-present breakdown: its frame interval and its CPU rows (ms). Fixed size, no heap.
struct PerfSpike {
  bool valid = false;
  uint64_t index = 0u;  // the series index of the present (0 in the tab)
  uint32_t frame = 0u;  // the frame of the sample (0 in the tab)
  double frame_ms = std::numeric_limits<double>::quiet_NaN();
  double cpu_total = std::numeric_limits<double>::quiet_NaN();
  double unattr = std::numeric_limits<double>::quiet_NaN();
  double unattr_raw = std::numeric_limits<double>::quiet_NaN();
  double bracket = std::numeric_limits<double>::quiet_NaN();  // the two present handlers
  double value[kPerfCostCount] = {};
};

// The Ray Tracing switches that decide what enters the BVH, in the order of the panel rows (world_bvh.hpp). Kept apart from
// CacheSwitches so the cache dump does not change.
inline constexpr size_t kPerfInclusionCount = 7;
inline constexpr const char* kPerfInclusionNames[kPerfInclusionCount] = {"alpha_foliage", "foliage_motion", "deforming_meshes", "follow_moving",
                                                                         "per_frame_p2b", "exclude_moving", "scan_indirect"};
using PerfInclusion = std::array<bool, kPerfInclusionCount>;

// What one Ray Tracing setting costs, from the rows above (spike plan, section 1). The rows of one setting are disjoint from
// those of another, but not from the totals: follow_scan lies inside pool_drain_excl_capture, and resolve_indirect has a GPU wait.
// cpu and gpu are the timed rows (kPerfCostCount: unused slot); context rows are shared with other work (averages only). untimed
// names the work with no clock read; needs_ab is the A/B run that measures the rest.
inline constexpr size_t kPerfSettingCount = 5;
struct PerfSetting {
  const char* key;
  size_t switch_first;  // index in PerfInclusion
  size_t switch_count;
  size_t cpu[4];
  size_t gpu[2];
  size_t context[2];
  const char* untimed;
  const char* needs_ab;
};
inline constexpr PerfSetting kPerfSettings[kPerfSettingCount] = {
    {"alpha_foliage", 0, 1, {kPerfAlphaSyncCpu, kPerfCostCount, kPerfCostCount, kPerfCostCount}, {kPerfGpuAlphaBlit, kPerfCostCount},
     {kPerfCostCount, kPerfCostCount},
     "admission and UV/atlas capture at draw time (inside pool_hooks); alpha instances' share of tlas_rebuild, tlas_refit, gpu_trace, gpu_rtao (any-hit)",
     "needs A/B: Alpha foliage OFF vs ON"},
    {"foliage_motion", 1, 1, {kPerfCostCount, kPerfCostCount, kPerfCostCount, kPerfCostCount}, {kPerfCostCount, kPerfCostCount},
     {kPerfTlasRefit, kPerfRefitPhase0},
     "wind sway maths per instance inside the refit copy loop and the rebuild, and the wind pass (no per-instance clock read)",
     "needs A/B: Foliage motion OFF vs ON; compare tlas_refit, refit_phase_0"},
    {"deforming_meshes", 2, 1, {kPerfDeformHookCpu, kPerfDeformUpdateCpu, kPerfBlockRow + kPerfBlockDeformProbe, kPerfCostCount},
     {kPerfGpuDeformRefit, kPerfCostCount}, {kPerfCostCount, kPerfCostCount},
     "deforming instances' share of tlas_* and of gpu_trace / gpu_rtao",
     "needs A/B: Deforming meshes OFF (RTAO OFF too, it needs them)"},
    {"follow_moving", 3, 3, {kPerfBlockRow + kPerfBlockFollowScan, kPerfP2bExpandCpu, kPerfBlockRow + kPerfBlockP2bRecord, kPerfCostCount},
     {kPerfGpuP2bExpand, kPerfCostCount}, {kPerfCostCount, kPerfCostCount},
     "exclude-moving admission and TLAS size effect; P2b TLAS exclusion savings; trace cost of expanded draws; follow/P2b capture at draw time (inside pool_hooks)",
     "needs A/B: each of the three OFF vs ON"},
    {"scan_indirect", 6, 1, {kPerfBlockRow + kPerfBlockResolveIndirect, kPerfCostCount, kPerfCostCount, kPerfCostCount},
     {kPerfCostCount, kPerfCostCount}, {kPerfCostCount, kPerfCostCount},
     "indirect capture staging at draw time (inside pool_hooks)",
     "needs A/B: Scan indirect OFF vs ON"},
};

// Sums of one setting's rows: cpu over the CPU rows, gpu over the Gpu rows that have a value. NaN when there are none.
struct PerfSettingValue {
  double cpu = std::numeric_limits<double>::quiet_NaN();
  double gpu = std::numeric_limits<double>::quiet_NaN();
};
inline PerfSettingValue PerfSettingSums(const double* values, const PerfSetting& setting) {
  PerfSettingValue sums;
  double cpu = 0.0, gpu = 0.0;
  bool cpu_any = false, gpu_any = false;
  for (size_t row : setting.cpu) {
    if (row == kPerfCostCount) continue;
    cpu += values[row];
    cpu_any = true;
  }
  for (size_t row : setting.gpu) {
    if (row == kPerfCostCount || std::isnan(values[row])) continue;
    gpu += values[row];
    gpu_any = true;
  }
  if (cpu_any) sums.cpu = cpu;
  if (gpu_any) sums.gpu = gpu;
  return sums;
}

struct PerfRunState {
  std::string label;  // raw label as typed
  std::chrono::steady_clock::time_point t0{};
  std::chrono::steady_clock::time_point last{};
  uint32_t start_frame = 0u;
  uint32_t end_frame = 0u;
  uint32_t serial = 0u;  // PoolState::episode_serial at the start: a change aborts the run
  uint64_t presents = 0u;  // presents after the start present
  double frame_ms_sum = 0.0;
  uint64_t hitches_over_25ms = 0u;    // frame intervals over 25 ms
  uint64_t hitches_over_33_3ms = 0u;  // frame intervals over 33.3 ms
  uint64_t block_over_2ms[kPerfBlockCount] = {};  // presents whose block value was over 2 ms (the perf file's attribution)
  double elapsed_s = 0.0;  // for the panel status line
  CacheSwitches start_switches;
  CacheSwitches end_switches;
  bool stable = true;  // no switch changed during the window
  PerfCounters prev;   // counters at the last present
  PerfCounters total;  // sums over the window
  PerfMaxima max;      // largest per present over the window
  PerfSpike spike_frame;   // the present with the largest frame interval
  PerfSpike spike_unattr;  // the present with the largest raw unattributed CPU (before the clamp)
  PerfInclusion start_inclusion{};
  PerfInclusion end_inclusion{};
  bool inclusion_stable = true;                     // no inclusion switch changed during the window
  PerfSettingValue setting_max[kPerfSettingCount];  // largest per present of each setting (sum of its rows)
  PerfSeries series;
  std::string abort_reason;  // empty: the window completed
};

// GPU timers the Performance tab sums: cumulative ms and samples read at each present (SamplePerfPresent).
struct PerfTimerSample {
  double ms_sum = 0.0;
  uint32_t samples = 0u;
};
struct PerfExtraGpu {
  PerfTimerSample rtao, rtao_filter, rtgi, rtgi_denoise, rtls, rtls_denoise, rtsun, rtsun_denoise, deform_refit;
  double dxr_tlas_ms = 0.0, dxr_records_ms = 0.0, dxr_characters_ms = 0.0, gc_blas_ms = 0.0, dxr_rtgi_ms = 0.0;  // D3D12 timestamp sums (rt12 panels); inside gpu_rtao, so Subset rows
};

// Performance tab: the last kPerfTabWindow presents. NaN means "--" (no value in the window).
inline constexpr size_t kPerfTabWindow = 30;
struct PerfTabSample {
  double value[kPerfCostCount] = {};   // this present (a cumulative cost: its delta)
  bool measured[kPerfCostCount] = {};  // Gpu rows: a sample was read in this present
  double frame_ms = std::numeric_limits<double>::quiet_NaN();  // interval to the previous present (not in the totals)
};
struct PerfTabView {
  double now[kPerfCostCount];  // newest value (Gpu rows: newest measured)
  double avg[kPerfCostCount];  // mean over the window (Gpu rows: over the measured presents)
  double unattr_now = std::numeric_limits<double>::quiet_NaN();
  double unattr_avg = std::numeric_limits<double>::quiet_NaN();
  double unattr_raw_now = std::numeric_limits<double>::quiet_NaN();  // before the clamp at zero
  double unattr_raw_avg = std::numeric_limits<double>::quiet_NaN();
  double cpu_now = std::numeric_limits<double>::quiet_NaN();  // DrawTime + InPresent rows + unattributed
  double cpu_avg = std::numeric_limits<double>::quiet_NaN();
  double hook_outer_now = std::numeric_limits<double>::quiet_NaN();
  double hook_outer_avg = std::numeric_limits<double>::quiet_NaN();
  double gpu_now = std::numeric_limits<double>::quiet_NaN();  // sum of the Gpu rows that have a value
  double gpu_avg = std::numeric_limits<double>::quiet_NaN();
  double frame_now = std::numeric_limits<double>::quiet_NaN();  // frame time: newest interval, and the mean over the window
  double frame_avg = std::numeric_limits<double>::quiet_NaN();
  PerfMaxima max;  // largest over the window (Gpu rows: over the measured presents)
  PerfSpike spike;        // the present with the largest frame interval in the window (valid false: none)
  size_t spike_age = 0u;  // presents since it (0: the newest)
  PerfTabView() {
    std::fill(std::begin(now), std::end(now), std::numeric_limits<double>::quiet_NaN());
    std::fill(std::begin(avg), std::end(avg), std::numeric_limits<double>::quiet_NaN());
  }
};
struct PerfTabState {
  std::array<PerfTabSample, kPerfTabWindow> ring{};
  size_t next = 0u;     // slot of the next sample
  size_t count = 0u;    // samples in the window
  bool primed = false;  // the first present only records prev
  PerfCounters prev;
  PerfTabView view;
};

struct PerfState {
  std::atomic_bool start_requested{false};  // set by the button, taken by the next present
  std::atomic_bool active{false};           // read by the panel and by Reset
  std::mutex mutex;                         // leaf lock: the run, the tab window and its view
  std::string pending_label;  // label typed when the start was requested
  PerfRunState run;           // the current or the last run
  PerfTabState tab;
};

// The totals of one row array (NaN: no value for a Gpu row). Unattributed CPU is the two present brackets minus the InPresent
// rows (raw, then clamped at zero). CPU is the DrawTime and InPresent rows plus the clamped unattributed CPU and the hook outer CPU
// (the draw hook and lighting RT time outside their DrawTime rows, and the bind, cb and pool-write hooks). GPU is the sum
// of the Gpu rows that have a value.
struct PerfTotalsValue {
  double unattr_raw = std::numeric_limits<double>::quiet_NaN();
  double unattr = std::numeric_limits<double>::quiet_NaN();
  double hook_outer = std::numeric_limits<double>::quiet_NaN();
  double cpu = std::numeric_limits<double>::quiet_NaN();
  double gpu = std::numeric_limits<double>::quiet_NaN();
};
inline PerfTotalsValue PerfTotals(const double* values) {
  double in = 0.0, bracket = 0.0, draw = 0.0, gpu = 0.0;
  bool gpu_any = false;
  for (size_t i = 0; i < kPerfCostCount; ++i) {
    const PerfRole role = kPerfCostInfo[i].role;
    if (role == kPerfInPresent) {
      in += values[i];
    } else if (role == kPerfDrawTime) {
      draw += values[i];
    } else if (role == kPerfBracket) {
      bracket += values[i];
    } else if (role == kPerfGpu && !std::isnan(values[i])) {
      gpu += values[i];
      gpu_any = true;
    }
  }
  PerfTotalsValue totals;
  totals.unattr_raw = bracket - in;
  totals.unattr = std::max(0.0, totals.unattr_raw);
  // The draw hook and the lighting RT block enclose DrawTime rows: only their time outside those rows is added (clamped at
  // zero; the enclosing and the inner clocks are separate reads). The bind, cb and pool-write hooks are in no other row.
  const double draw_hook_outer = values[kPerfBlockRow + kPerfBlockDrawHookTotal] - values[kPerfPoolHooks] - values[kPerfDeformHookCpu];
  const double lighting_rt_outer = values[kPerfBlockRow + kPerfBlockLightingRt] - values[kPerfRtaoRecordCpu] - values[kPerfRtgiRecordCpu]
                                   - values[kPerfRtlsRecordCpu] - values[kPerfRtsunRecordCpu];
  totals.hook_outer = std::max(0.0, draw_hook_outer) + std::max(0.0, lighting_rt_outer) + values[kPerfBlockRow + kPerfBlockBindHook]
                      + values[kPerfBlockRow + kPerfBlockCbHooks] + values[kPerfBlockRow + kPerfBlockPoolWriteHooks];
  totals.cpu = draw + in + totals.unattr + totals.hook_outer;
  totals.gpu = gpu_any ? gpu : std::numeric_limits<double>::quiet_NaN();
  return totals;
}

// One present's values: a cumulative cost is its delta from prev, the others are this present's value; a Gpu row without a
// sample in this present is NaN (not measured).
inline PerfTabSample MakePerfSample(const PerfCounters& now, const PerfCounters& prev, double frame_ms) {
  PerfTabSample sample;
  sample.frame_ms = frame_ms;
  for (size_t i = 0; i < kPerfCostCount; ++i) {
    const double cost = kPerfCostInfo[i].cumulative && now.cost[i] >= prev.cost[i] ? now.cost[i] - prev.cost[i] : now.cost[i];
    if (kPerfCostInfo[i].role != kPerfGpu) {
      sample.value[i] = cost;
      continue;
    }
    const uint64_t gained = now.events[i] >= prev.events[i] ? now.events[i] - prev.events[i] : now.events[i];
    sample.measured[i] = gained != 0u;
    sample.value[i] = gained != 0u ? cost / static_cast<double>(gained) : std::numeric_limits<double>::quiet_NaN();
  }
  return sample;
}

// The present's frame and CPU rows as a spike record (index and frame are the run's; the tab passes 0).
inline PerfSpike MakePerfSpike(const PerfTabSample& s, uint64_t index, uint32_t frame) {
  PerfSpike spike;
  spike.valid = true;
  spike.index = index;
  spike.frame = frame;
  spike.frame_ms = s.frame_ms;
  const PerfTotalsValue totals = PerfTotals(s.value);
  spike.cpu_total = totals.cpu;
  spike.unattr = totals.unattr;
  spike.unattr_raw = totals.unattr_raw;
  spike.bracket = s.value[kPerfPresentWorldCpu] + s.value[kPerfPresentBvhCpu];
  std::copy(std::begin(s.value), std::end(s.value), std::begin(spike.value));
  return spike;
}

// Keeps the largest value that is not NaN (NaN: none yet).
inline void MaxInto(double* max, double value) {
  if (!std::isnan(value) && (std::isnan(*max) || value > *max)) *max = value;
}

inline PerfState g_perf;

inline double PerfMsOf(const std::atomic_uint64_t& us) { return static_cast<double>(us.load(std::memory_order_relaxed)) / 1000000.0; }

// Counters at one present. Caller holds g_pool.mutex (tlas_timing, the expand sum and the admitted count).
inline PerfCounters ReadPerfCounters(const BvhDeviceData* data, const PerfExtraGpu& extra = {}) {
  const LiveBvhStats& live = data->live;
  const PoolTlasTiming& timing = g_pool.tlas_timing;
  PerfCounters c;
  c.cost[kPerfTlasRebuild] = timing.rebuild_ms_sum;
  c.events[kPerfTlasRebuild] = timing.rebuilds;
  c.cost[kPerfTlasSkip] = live.tlas_skip_ms_sum;
  c.events[kPerfTlasSkip] = live.tlas_skips;
  c.cost[kPerfTlasApply] = live.slack_apply_ms_sum;
  c.events[kPerfTlasApply] = live.slack_applies - live.slack_zero_op_applies;
  c.cost[kPerfTlasRefit] = timing.refit_ms_sum;
  c.events[kPerfTlasRefit] = timing.refits;
  c.cost[kPerfBlasBuild] = live.blas_build_ms_sum;
  c.events[kPerfBlasBuild] = live.blas_builds;
  c.cost[kPerfMeshCapture] = PerfMsOf(g_pool.cpu_capture_frame_us);
  c.cost[kPerfPoolDrainExclCapture] = PerfMsOf(g_pool.cpu_drain_frame_us) - PerfMsOf(g_pool.cpu_capture_frame_us);  // capture runs inside the drain
  c.cost[kPerfP2bExpandCpu] = g_pool.stats.dynamic_expand_cpu_ms_sum;
  c.cost[kPerfAlphaSyncCpu] = PerfMsOf(g_perf_gate.alpha_sync_us);
  c.cost[kPerfDeformUpdateCpu] = PerfMsOf(g_perf_gate.deform_update_us);
  c.cost[kPerfPoolHooks] = PerfMsOf(g_pool.cpu_hook_frame_us);
  c.cost[kPerfDeformHookCpu] = PerfMsOf(g_deform_live.cpu_frame_us);
  c.cost[kPerfRtaoRecordCpu] = PerfMsOf(g_perf_gate.rtao_record_us);
  c.cost[kPerfRtgiRecordCpu] = PerfMsOf(g_perf_gate.rtgi_record_us);
  c.cost[kPerfRtlsRecordCpu] = PerfMsOf(g_perf_gate.rtls_record_us);
  c.cost[kPerfRtsunRecordCpu] = PerfMsOf(g_perf_gate.rtsun_record_us);
  c.cost[kPerfPresentWorldCpu] = PerfMsOf(g_perf_gate.present_world_us);
  c.cost[kPerfPresentBvhCpu] = PerfMsOf(g_perf_gate.present_bvh_us);
  c.cost[kPerfGpuTrace] = data->trace_timer.ms_sum;
  c.events[kPerfGpuTrace] = data->trace_timer.samples;
  c.cost[kPerfGpuAlphaBlit] = data->alpha.timer.ms_sum;
  c.events[kPerfGpuAlphaBlit] = data->alpha.timer.samples;
  c.cost[kPerfGpuP2bExpand] = data->dynamic.timer.ms_sum;
  c.events[kPerfGpuP2bExpand] = data->dynamic.timer.samples;
  c.cost[kPerfGpuDeformRefit] = extra.deform_refit.ms_sum;
  c.events[kPerfGpuDeformRefit] = extra.deform_refit.samples;
  c.cost[kPerfGpuRtao] = extra.rtao.ms_sum;
  c.events[kPerfGpuRtao] = extra.rtao.samples;
  c.cost[kPerfGpuRtaoFilter] = extra.rtao_filter.ms_sum;
  c.events[kPerfGpuRtaoFilter] = extra.rtao_filter.samples;
  c.cost[kPerfGpuRtgi] = extra.rtgi.ms_sum;
  c.events[kPerfGpuRtgi] = extra.rtgi.samples;
  c.cost[kPerfGpuRtgiDenoise] = extra.rtgi_denoise.ms_sum;
  c.events[kPerfGpuRtgiDenoise] = extra.rtgi_denoise.samples;
  c.cost[kPerfGpuRtls] = extra.rtls.ms_sum;
  c.events[kPerfGpuRtls] = extra.rtls.samples;
  c.cost[kPerfGpuRtlsDenoise] = extra.rtls_denoise.ms_sum;
  c.events[kPerfGpuRtlsDenoise] = extra.rtls_denoise.samples;
  c.cost[kPerfGpuRtsun] = extra.rtsun.ms_sum;
  c.events[kPerfGpuRtsun] = extra.rtsun.samples;
  c.cost[kPerfGpuRtsunDenoise] = extra.rtsun_denoise.ms_sum;
  c.events[kPerfGpuRtsunDenoise] = extra.rtsun_denoise.samples;
  c.cost[kPerfDxrTlasGpu] = extra.dxr_tlas_ms;
  c.cost[kPerfDxrRecordsGpu] = extra.dxr_records_ms;
  c.cost[kPerfDxrCharactersGpu] = extra.dxr_characters_ms;
  c.cost[kPerfGcBlasBuildGpu] = extra.gc_blas_ms;
  c.cost[kPerfDxrRtgiGpu] = extra.dxr_rtgi_ms;
  for (size_t p = 0; p < 5u; ++p) c.cost[kPerfRebuildPhase0 + p] = timing.rebuild_phase_ms_sum[p];
  for (size_t p = 0; p < 4u; ++p) c.cost[kPerfRefitPhase0 + p] = timing.refit_phase_ms_sum[p];
  for (size_t p = 0; p < 4u; ++p) c.cost[kPerfSlackPhase0 + p] = live.slack_phase_ms_sum[p];
  for (size_t k = 0; k < kPerfBlockCount; ++k) c.cost[kPerfBlockRow + k] = PerfMsOf(g_perf_gate.block_us[k]);
  c.gauge[kPerfGaugeTlasInstances] = live.tlas_instances;
  c.gauge[kPerfGaugeTlasDynamic] = static_cast<double>(data->tlas_info.size() - data->tlas_dynamic_first);  // the dynamic suffix
  c.gauge[kPerfGaugePoolInstances] = static_cast<double>(g_pool.instances.size());
  c.gauge[kPerfGaugePoolMeshes] = live.pool_meshes;
  c.gauge[kPerfGaugeSlackHoles] = live.slack_free_holes;
  c.gauge[kPerfGaugeHookDraws] = static_cast<double>(g_perf_gate.hook_draws_frame.load(std::memory_order_relaxed));
  c.gauge[kPerfGaugeHookDrawsIndirect] = static_cast<double>(g_perf_gate.hook_draws_indirect_frame.load(std::memory_order_relaxed));
  c.gauge[kPerfGaugeHookDrawsUnconsumed] = static_cast<double>(g_perf_gate.hook_draws_unconsumed_frame.load(std::memory_order_relaxed));
  c.gauge[kPerfGaugeRecordMismatchRtv0] = static_cast<double>(g_perf_gate.record_mismatch_rtv0_frame.load(std::memory_order_relaxed));
  c.gauge[kPerfGaugeRecordMismatchDsv] = static_cast<double>(g_perf_gate.record_mismatch_dsv_frame.load(std::memory_order_relaxed));
  c.gauge[kPerfGaugeRecordMismatchTopology] = static_cast<double>(g_perf_gate.record_mismatch_topology_frame.load(std::memory_order_relaxed));
  c.gauge[kPerfGaugeRecordMismatchInputLayout] = static_cast<double>(g_perf_gate.record_mismatch_input_layout_frame.load(std::memory_order_relaxed));
  c.gauge[kPerfGaugeRecordMismatchHasViewport] = static_cast<double>(g_perf_gate.record_mismatch_has_viewport_frame.load(std::memory_order_relaxed));
  c.gauge[kPerfGaugeRecordMismatchViewport] = static_cast<double>(g_perf_gate.record_mismatch_viewport_frame.load(std::memory_order_relaxed));
  c.gauge[kPerfGaugeRecordMismatchBlendEnable] = static_cast<double>(g_perf_gate.record_mismatch_blend_enable_frame.load(std::memory_order_relaxed));
  c.gauge[kPerfGaugeRecordMismatchDepthEnable] = static_cast<double>(g_perf_gate.record_mismatch_depth_enable_frame.load(std::memory_order_relaxed));
  c.gauge[kPerfGaugeRecordMismatchDepthWrite] = static_cast<double>(g_perf_gate.record_mismatch_depth_write_frame.load(std::memory_order_relaxed));
  c.gauge[kPerfGaugeRecordMismatchCullMode] = static_cast<double>(g_perf_gate.record_mismatch_cull_mode_frame.load(std::memory_order_relaxed));
  c.gauge[kPerfGaugeRecordMismatchFrontCounterClockwise] = static_cast<double>(g_perf_gate.record_mismatch_front_counter_clockwise_frame.load(std::memory_order_relaxed));
  c.gauge[kPerfGaugePoolReduced] = g_gpu_capture.pool_reduced.load(std::memory_order_relaxed) ? 1.0 : 0.0;
  c.gauge[kPerfGaugeBindCalls] = static_cast<double>(g_perf_gate.bind_calls_frame.load(std::memory_order_relaxed));
  c.gauge[kPerfGaugeBindSrvResolves] = static_cast<double>(g_perf_gate.bind_srv_resolves_frame.load(std::memory_order_relaxed));
  return c;
}

// The inclusion switches now (relaxed atomic reads; no lock, no clock).
inline PerfInclusion ReadPerfInclusion() {
  return PerfInclusion{g_pool.alpha_foliage.load(std::memory_order_relaxed), g_pool.alpha_wind_sway.load(std::memory_order_relaxed),
                       g_deform_live.enabled.load(std::memory_order_relaxed), g_pool.follow_moving.load(std::memory_order_relaxed),
                       g_pool.dynamic_per_frame.load(std::memory_order_relaxed), g_pool.exclude_moving.load(std::memory_order_relaxed),
                       g_pool.scan_indirect.load(std::memory_order_relaxed)};
}

// One value object of the perf file: {"value": x, "unit": "..."}; NaN or infinite values are written as null.
inline void WritePerfValue(std::ostringstream* out, double value, const char* unit) {
  *out << "{\"value\": ";
  if (std::isfinite(value)) {
    *out << value;
  } else {
    *out << "null";
  }
  *out << ", \"unit\": \"" << unit << "\"}";
}

// A series: an array of numbers, NaN or infinite values written as null.
inline void WritePerfArray(std::ostringstream* out, const std::vector<double>& values) {
  *out << "[";
  for (size_t i = 0; i < values.size(); ++i) {
    if (i != 0u) *out << ", ";
    if (std::isfinite(values[i])) {
      *out << values[i];
    } else {
      *out << "null";
    }
  }
  *out << "]";
}

// A worst present: its frame, its CPU rows and the block rows (ms); null when there is none.
inline void WritePerfSpike(std::ostringstream* out, const PerfSpike& spike) {
  if (!spike.valid) {
    *out << "null";
    return;
  }
  double blocks_in_present = 0.0;  // the blocks that run inside the present brackets (not subsets)
  for (size_t k = 0; k < kPerfBlockRebuildCall; ++k) blocks_in_present += spike.value[kPerfBlockRow + k];
  *out << "{\"index\": ";
  WritePerfValue(out, static_cast<double>(spike.index), "count");
  *out << ", \"in_series\": " << (spike.index < kPerfSeriesCapacity ? "true" : "false") << ", \"frame\": ";
  WritePerfValue(out, spike.frame, "frame");
  *out << ", \"frame_ms\": ";
  WritePerfValue(out, spike.frame_ms, "ms");
  *out << ", \"cpu_total\": ";
  WritePerfValue(out, spike.cpu_total, "ms");
  *out << ", \"unattributed_ms\": ";
  WritePerfValue(out, spike.unattr, "ms");
  *out << ", \"unattributed_raw_ms\": ";
  WritePerfValue(out, spike.unattr_raw, "ms");
  *out << ", \"handlers_ms\": ";
  WritePerfValue(out, spike.bracket, "ms");
  *out << ", \"blocks_in_present_ms\": ";
  WritePerfValue(out, blocks_in_present, "ms");
  *out << ", \"blocks\": {";
  for (size_t k = 0; k < kPerfBlockCount; ++k) {
    *out << (k != 0u ? ", " : "") << "\"" << kPerfCostInfo[kPerfBlockRow + k].name << "\": ";
    WritePerfValue(out, spike.value[kPerfBlockRow + k], "ms");
  }
  *out << "}}";
}

// Writes the perf file in PoolOutputDir()/diag and returns its path.
inline std::filesystem::path WritePerfFile(const PerfRunState& run) {
  const double nan = std::numeric_limits<double>::quiet_NaN();
  const auto when = std::chrono::system_clock::now();
  const std::string label = SanitizeDiagLabel(run.label);
  const std::filesystem::path path = DiagFilePath("perf", label, when);
  const double presents = static_cast<double>(run.presents);
  const double actual_s = std::chrono::duration<double>(run.last - run.t0).count();
  const double avg_ms = run.presents != 0u ? run.frame_ms_sum / presents : nan;
  std::vector<double> sorted = run.series.frame_ms;  // p99 (nearest rank) over the series
  double p99 = nan;
  if (!sorted.empty()) {
    const size_t rank = (sorted.size() * 99u + 99u) / 100u;
    std::sort(sorted.begin(), sorted.end());
    p99 = sorted[rank - 1u];
  }
  std::ostringstream out;
  out << "{\n  \"schema\": 1,\n  \"kind\": \"performance_test\",\n  \"build\": \"" << PoolJsonEscape(kWorldBuildId) << "\",\n";
  out << "  \"label\": \"" << PoolJsonEscape(label) << "\",\n  \"label_raw\": \"" << PoolJsonEscape(run.label) << "\",\n";
  out << "  \"timestamp\": \"" << DiagTimestamp(when) << "\",\n";
  out << "  \"perf_gather\": " << (g_perf_gate.gather.load(std::memory_order_relaxed) ? "true" : "false") << ",\n";
  const char* reduce_reason = "";
  {
    std::lock_guard lock(g_gpu_capture.mutex);
    reduce_reason = g_gpu_capture.pool_reduce.full_reason;
  }
  out << "  \"gc_trace\": " << (g_gpu_capture.enabled.load(std::memory_order_relaxed) && g_gpu_capture.trace.load(std::memory_order_relaxed) ? "true" : "false") << ",\n";
  out << "  \"pool_reduce_full_reason_at_end\": \"" << PoolJsonEscape(reduce_reason) << "\",\n";
  out << "  \"sample_alignment\": \"work of present N, sampled at the start of present N+1\",\n";
  out << "  \"status\": \"" << (run.abort_reason.empty() ? "complete" : "aborted") << "\",\n  \"abort_reason\": \"" << PoolJsonEscape(run.abort_reason)
      << "\",\n";
  out << "  \"start_frame\": ";
  WritePerfValue(&out, run.start_frame, "frame");
  out << ",\n  \"end_frame\": ";
  WritePerfValue(&out, run.end_frame, "frame");
  out << ",\n  \"window\": {\"requested_s\": ";
  WritePerfValue(&out, kPerfWindowSeconds, "s");
  out << ", \"actual_s\": ";
  WritePerfValue(&out, actual_s, "s");
  out << ", \"presents\": ";
  WritePerfValue(&out, presents, "count");
  out << "},\n  \"frame_time\": {\"avg\": ";
  WritePerfValue(&out, avg_ms, "ms");
  out << ", \"avg_fps\": ";
  WritePerfValue(&out, 1000.0 / avg_ms, "fps");
  out << ", \"max\": ";
  WritePerfValue(&out, run.max.frame, "ms");
  out << ", \"p99\": ";
  WritePerfValue(&out, p99, "ms");
  out << ", \"hitches_over_25ms\": ";
  WritePerfValue(&out, static_cast<double>(run.hitches_over_25ms), "count");
  out << ", \"hitches_over_33_3ms\": ";
  WritePerfValue(&out, static_cast<double>(run.hitches_over_33_3ms), "count");
  out << "},\n  \"switches\": {\"start\": ";
  WriteSwitchesJson(&out, run.start_switches);
  out << ", \"end\": ";
  WriteSwitchesJson(&out, run.end_switches);
  out << ", \"stable\": " << (run.stable ? "true" : "false") << "},\n  \"stats\": {";
  double gpu_avg_sum = 0.0;
  bool gpu_measured = false;
  for (size_t i = 0; i < kPerfCostCount; ++i) {
    const PerfCostInfo& info = kPerfCostInfo[i];
    const double total = run.total.cost[i];
    const double events = static_cast<double>(run.total.events[i]);
    out << (i != 0u ? ", " : "") << "\"" << info.name << "\": {";
    if (info.role == kPerfGpu) {
      out << "\"basis\": \"" << (run.total.events[i] != 0u ? "measured" : "not measured") << "\", \"avg_per_measured_frame\": ";
      WritePerfValue(&out, events != 0.0 ? total / events : nan, "ms/measured frame");
      out << ", \"max_per_measured_frame\": ";
      WritePerfValue(&out, run.max.row[i], "ms/measured frame");
      out << ", \"total\": ";
      WritePerfValue(&out, total, "ms");
      out << ", \"samples\": ";
      WritePerfValue(&out, events, "count");
      if (events != 0.0) {
        gpu_avg_sum += total / events;
        gpu_measured = true;
      }
    } else {
      out << "\"avg_per_present\": ";
      WritePerfValue(&out, run.presents != 0u ? total / presents : nan, "ms/present");
      out << ", \"max_per_present\": ";
      WritePerfValue(&out, run.max.row[i], "ms/present");
      out << ", \"total\": ";
      WritePerfValue(&out, total, "ms");
      if (info.counted) {
        out << ", \"events\": ";
        WritePerfValue(&out, events, "count");
        out << ", \"avg_per_event\": ";
        WritePerfValue(&out, events != 0.0 ? total / events : nan, "ms/event");
        out << ", \"rate\": ";
        WritePerfValue(&out, actual_s > 0.0 ? events / actual_s : nan, "count/s");
      }
    }
    out << ", \"in_total\": " << (PerfInTotal(info) ? "true" : "false");
    if (info.phase != nullptr) out << ", \"phase\": \"" << info.phase << "\"";
    out << "}";
  }
  // Legacy keys (in_total false): the drain and the deform CPU as they were before the split.
  const double pool_drain = run.total.cost[kPerfPoolDrainExclCapture] + run.total.cost[kPerfMeshCapture];
  out << ", \"pool_drain\": {\"avg_per_present\": ";
  WritePerfValue(&out, run.presents != 0u ? pool_drain / presents : nan, "ms/present");
  out << ", \"total\": ";
  WritePerfValue(&out, pool_drain, "ms");
  out << ", \"in_total\": false}";
  const double deform_cpu = run.total.cost[kPerfDeformHookCpu] + run.total.cost[kPerfDeformUpdateCpu];
  out << ", \"deform_cpu\": {\"avg_per_present\": ";
  WritePerfValue(&out, run.presents != 0u ? deform_cpu / presents : nan, "ms/present");
  out << ", \"total\": ";
  WritePerfValue(&out, deform_cpu, "ms");
  out << ", \"in_total\": false}";
  // Unattributed CPU: the two present brackets minus the rows that run inside them, clamped at zero.
  const PerfTotalsValue totals = PerfTotals(run.total.cost);
  out << ", \"unattributed_cpu\": {\"avg_per_present\": ";
  WritePerfValue(&out, run.presents != 0u ? totals.unattr / presents : nan, "ms/present");
  out << ", \"max_per_present\": ";
  WritePerfValue(&out, run.max.unattr, "ms/present");
  out << ", \"total\": ";
  WritePerfValue(&out, totals.unattr, "ms");
  out << ", \"clamped\": " << (totals.unattr_raw < -0.01 * presents ? "true" : "false") << ", \"raw_total\": ";
  WritePerfValue(&out, totals.unattr_raw, "ms");
  out << ", \"in_total\": true}";
  out << ", \"hook_outer_cpu\": {\"avg_per_present\": ";
  WritePerfValue(&out, run.presents != 0u ? totals.hook_outer / presents : nan, "ms/present");
  out << ", \"total\": ";
  WritePerfValue(&out, totals.hook_outer, "ms");
  out << ", \"in_total\": true}";
  const double cpu_total = totals.cpu;
  out << ", \"cpu_total\": {\"avg_per_present\": ";
  WritePerfValue(&out, run.presents != 0u ? cpu_total / presents : nan, "ms/present");
  out << ", \"max_per_present\": ";
  WritePerfValue(&out, run.max.cpu, "ms/present");
  out << ", \"total\": ";
  WritePerfValue(&out, cpu_total, "ms");
  out << ", \"in_total\": false}";
  out << ", \"gpu_total\": {\"avg_per_measured_frame\": ";
  WritePerfValue(&out, gpu_measured ? gpu_avg_sum : nan, "ms/measured frame");
  out << ", \"max_per_measured_frame\": ";
  WritePerfValue(&out, run.max.gpu, "ms/measured frame");
  out << ", \"in_total\": false}";
  out << "},\n  \"gauges\": {";
  for (size_t i = 0; i < kPerfGaugeCount; ++i) {
    out << (i != 0u ? ", " : "") << "\"" << kPerfGaugeNames[i] << "\": {\"avg\": ";
    WritePerfValue(&out, run.presents != 0u ? run.total.gauge[i] / presents : nan, "count");
    out << "}";
  }
  // Blocks: the largest and the mean per present, and the presents over 2 ms. Disjoint blocks are in the unattributed sum; subsets are not.
  out << "},\n  \"attribution\": {\"threshold_ms\": ";
  WritePerfValue(&out, 2.0, "ms");
  out << ", \"blocks\": {";
  for (size_t k = 0; k < kPerfBlockCount; ++k) {
    const size_t row = kPerfBlockRow + k;
    out << (k != 0u ? ", " : "") << "\"" << kPerfCostInfo[row].name << "\": {\"max_per_present\": ";
    WritePerfValue(&out, run.max.row[row], "ms/present");
    out << ", \"avg_per_present\": ";
    WritePerfValue(&out, run.presents != 0u ? run.total.cost[row] / presents : nan, "ms/present");
    out << ", \"presents_over_2ms\": ";
    WritePerfValue(&out, static_cast<double>(run.block_over_2ms[k]), "count");
    out << ", \"role\": \"" << (kPerfCostInfo[row].role == kPerfSubset ? "subset" : "disjoint") << "\"}";
  }
  out << "}, \"worst_present\": ";
  WritePerfSpike(&out, run.spike_frame);
  out << ", \"worst_unattributed_present\": ";
  WritePerfSpike(&out, run.spike_unattr);
  out << "}";
  // Settings: the average per present (CPU rows) or per measured frame (Gpu rows) over the run, and the largest per present.
  double setting_avg[kPerfCostCount];
  for (size_t i = 0; i < kPerfCostCount; ++i) {
    const double events = static_cast<double>(run.total.events[i]);
    if (kPerfCostInfo[i].role == kPerfGpu) {
      setting_avg[i] = events != 0.0 ? run.total.cost[i] / events : nan;
    } else {
      setting_avg[i] = run.presents != 0u ? run.total.cost[i] / presents : nan;
    }
  }
  out << ",\n  \"settings_cost\": {\"basis\": \"sum of the timed rows of one setting; settings do not add up to cpu_total; follow_scan is inside "
         "pool_drain_excl_capture; resolve_indirect includes a GPU wait\", \"inclusion_switches\": {\"start\": {";
  for (size_t i = 0; i < kPerfInclusionCount; ++i) out << (i != 0u ? ", " : "") << "\"" << kPerfInclusionNames[i] << "\": " << (run.start_inclusion[i] ? "true" : "false");
  out << "}, \"end\": {";
  for (size_t i = 0; i < kPerfInclusionCount; ++i) out << (i != 0u ? ", " : "") << "\"" << kPerfInclusionNames[i] << "\": " << (run.end_inclusion[i] ? "true" : "false");
  out << "}, \"stable\": " << (run.inclusion_stable ? "true" : "false") << "}, \"settings\": {";
  for (size_t s = 0; s < kPerfSettingCount; ++s) {
    const PerfSetting& setting = kPerfSettings[s];
    const PerfSettingValue avg = PerfSettingSums(setting_avg, setting);
    out << (s != 0u ? ", " : "") << "\"" << setting.key << "\": {\"switch_keys\": [";
    for (size_t j = 0; j < setting.switch_count; ++j) out << (j != 0u ? ", " : "") << "\"" << kPerfInclusionNames[setting.switch_first + j] << "\"";
    out << "], \"live_parts\": [";
    bool first = true;
    for (size_t j = 0; j < 6; ++j) {
      const size_t row = j < 4 ? setting.cpu[j] : setting.gpu[j - 4];
      if (row == kPerfCostCount) continue;
      out << (first ? "" : ", ") << "\"" << kPerfCostInfo[row].name << "\"";
      first = false;
    }
    out << "], \"live_cpu\": {\"avg_per_present\": ";
    WritePerfValue(&out, avg.cpu, "ms/present");
    out << ", \"max_per_present\": ";
    WritePerfValue(&out, run.setting_max[s].cpu, "ms/present");
    out << "}, \"live_gpu\": {\"avg_per_measured_frame\": ";
    WritePerfValue(&out, avg.gpu, "ms/measured frame");
    out << ", \"max_per_measured_frame\": ";
    WritePerfValue(&out, run.setting_max[s].gpu, "ms/measured frame");
    out << "}, \"context_rows\": {";
    first = true;
    for (size_t row : setting.context) {
      if (row == kPerfCostCount) continue;
      out << (first ? "" : ", ") << "\"" << kPerfCostInfo[row].name << "\": ";
      WritePerfValue(&out, setting_avg[row], "ms/present");
      first = false;
    }
    out << "}, \"untimed\": \"" << setting.untimed << "\", \"needs_ab\": \"" << setting.needs_ab << "\"}";
  }
  // Per present, in ms (the frame interval for frame_ms); null where no value.
  out << "}},\n  \"series\": {\"capacity\": " << kPerfSeriesCapacity << ", \"length\": " << run.series.frame_ms.size() << ", \"frame_ms\": ";
  WritePerfArray(&out, run.series.frame_ms);
  out << ", \"cpu_total\": ";
  WritePerfArray(&out, run.series.cpu_total);
  out << ", \"tlas_rebuild\": ";
  WritePerfArray(&out, run.series.tlas_rebuild);
  out << ", \"tlas_refit\": ";
  WritePerfArray(&out, run.series.tlas_refit);
  out << ", \"blocks_unit\": \"ms/present; index i pairs with frame_ms[i]\", \"blocks\": {";
  for (size_t k = 0; k < kPerfBlockCount; ++k) {
    out << (k != 0u ? ", " : "") << "\"" << kPerfCostInfo[kPerfBlockRow + k].name << "\": ";
    WritePerfArray(&out, run.series.blocks[k]);
  }
  out << "}}\n}\n";
  std::string text = out.str();
  renodx::utils::path::WriteTextFile(path, text);
  return path;
}

// Gathering was switched OFF: clears what it gathered (CPU sums, maxima, last values, phase sums, GPU timer statistics,
// cache-diag timings) and the tab window. The RTAO, RTGI and RTLS timers are cleared by rtao::ClearRtaoPerfGathered,
// rtgi::ClearRtgiPerfGathered and OnWorldPresentBvh (RTLS). Runs on the present
// thread, one lock at a time; no graphics call is made under a lock.
inline void ClearPerfGathered(BvhDeviceData* data) {
  g_pool.cpu_hook_us.store(0u);
  g_pool.cpu_drain_us.store(0u);
  g_pool.cpu_capture_us.store(0u);
  g_pool.cpu_hook_frame_us.store(0u);
  g_pool.cpu_drain_frame_us.store(0u);
  g_pool.cpu_capture_frame_us.store(0u);
  g_perf_gate.present_world_us.store(0u);
  g_perf_gate.present_bvh_us.store(0u);
  g_perf_gate.alpha_sync_us.store(0u);
  g_perf_gate.rtao_record_us.store(0u);
  g_perf_gate.rtgi_record_us.store(0u);
  g_perf_gate.rtls_record_us.store(0u);
  g_perf_gate.deform_update_us.store(0u);
  for (std::atomic_uint64_t& us : g_perf_gate.block_us) us.store(0u);
  g_perf_gate.hook_draws.store(0u);
  g_perf_gate.hook_draws_frame.store(0u);
  g_perf_gate.hook_draws_indirect.store(0u);
  g_perf_gate.hook_draws_indirect_frame.store(0u);
  g_perf_gate.hook_draws_unconsumed.store(0u);
  g_perf_gate.hook_draws_unconsumed_frame.store(0u);
  g_perf_gate.bind_calls.store(0u);
  g_perf_gate.bind_calls_frame.store(0u);
  g_perf_gate.bind_srv_resolves.store(0u);
  g_perf_gate.bind_srv_resolves_frame.store(0u);
  g_perf_gate.record_mismatch_rtv0.store(0u);
  g_perf_gate.record_mismatch_rtv0_frame.store(0u);
  g_perf_gate.record_mismatch_dsv.store(0u);
  g_perf_gate.record_mismatch_dsv_frame.store(0u);
  g_perf_gate.record_mismatch_topology.store(0u);
  g_perf_gate.record_mismatch_topology_frame.store(0u);
  g_perf_gate.record_mismatch_input_layout.store(0u);
  g_perf_gate.record_mismatch_input_layout_frame.store(0u);
  g_perf_gate.record_mismatch_has_viewport.store(0u);
  g_perf_gate.record_mismatch_has_viewport_frame.store(0u);
  g_perf_gate.record_mismatch_viewport.store(0u);
  g_perf_gate.record_mismatch_viewport_frame.store(0u);
  g_perf_gate.record_mismatch_blend_enable.store(0u);
  g_perf_gate.record_mismatch_blend_enable_frame.store(0u);
  g_perf_gate.record_mismatch_depth_enable.store(0u);
  g_perf_gate.record_mismatch_depth_enable_frame.store(0u);
  g_perf_gate.record_mismatch_depth_write.store(0u);
  g_perf_gate.record_mismatch_depth_write_frame.store(0u);
  g_perf_gate.record_mismatch_cull_mode.store(0u);
  g_perf_gate.record_mismatch_cull_mode_frame.store(0u);
  g_perf_gate.record_mismatch_front_counter_clockwise.store(0u);
  g_perf_gate.record_mismatch_front_counter_clockwise_frame.store(0u);
  g_deform_live.cpu_us.store(0u);
  g_deform_live.cpu_frame_us.store(0u);
  {
    std::lock_guard<std::mutex> lock(g_pool.mutex);
    PoolTlasTiming& timing = g_pool.tlas_timing;
    timing.rebuild_ms_last = 0.f;
    timing.rebuild_ms_max = 0.f;
    timing.rebuild_ms_sum = 0.f;
    timing.refit_ms_last = 0.f;
    timing.refit_ms_max = 0.f;
    timing.refit_ms_sum = 0.f;
    timing.after_drain_ms_last = 0.f;
    timing.after_drain_ms_max = 0.f;
    timing.after_drain_ms_sum = 0.f;
    std::fill(std::begin(timing.rebuild_ms_by_cause), std::end(timing.rebuild_ms_by_cause), 0.f);
    std::fill(std::begin(timing.refit_phase_ms), std::end(timing.refit_phase_ms), 0.f);
    std::fill(std::begin(timing.rebuild_phase_ms_sum), std::end(timing.rebuild_phase_ms_sum), 0.f);
    std::fill(std::begin(timing.refit_phase_ms_sum), std::end(timing.refit_phase_ms_sum), 0.f);
    g_pool.stats.dynamic_expand_cpu_ms = 0.f;
    g_pool.stats.dynamic_expand_cpu_ms_sum = 0.f;
    g_pool.stats.dynamic_expand_gpu_ms = 0.f;
    g_pool.alpha_gpu.blit_ms = -1.f;
    g_pool.alpha_gpu.trace_ms = -1.f;
    if (data != nullptr) {
      LiveBvhStats& live = data->live;
      live.tlas_skip_ms_sum = 0.f;
      live.tlas_skip_ms_max = 0.f;
      live.slack_apply_ms_sum = 0.f;
      live.slack_apply_ms_max = 0.f;
      std::fill(std::begin(live.slack_phase_ms), std::end(live.slack_phase_ms), 0.f);
      std::fill(std::begin(live.slack_phase_ms_sum), std::end(live.slack_phase_ms_sum), 0.f);
      live.mesh_ms_last = 0.f;
      live.mesh_ms_max = 0.f;
      live.tlas_ms_last = 0.f;
      live.tlas_ms_max = 0.f;
      live.blas_build_ms_sum = 0.f;
      live.blas_build_ms_max = 0.f;
      live.refit_ms_last = 0.f;
      live.refit_ms_max = 0.f;
    }
  }
  if (data != nullptr) {
    ClearGpuTimerStats(&data->trace_timer);
    ClearGpuTimerStats(&data->alpha.timer);
    ClearGpuTimerStats(&data->dynamic.timer);
  }
  {
    std::lock_guard<std::mutex> lock(g_deform_live.mutex);
    ClearGpuTimerStats(&g_deform_live.refit_timer);
  }
  {
    std::lock_guard<std::mutex> lock(g_cache_diag.mutex);
    CacheDiagData& d = g_cache_diag.data;
    d.tlas.ms_sum = 0.f;
    d.tlas.ms_max = 0.f;
    d.tlas.noop_ms = 0.f;
    d.tlas.skip_ms = 0.f;
    std::fill(std::begin(d.tlas.phase_last), std::end(d.tlas.phase_last), 0.f);
    std::fill(std::begin(d.tlas.phase_sum), std::end(d.tlas.phase_sum), 0.f);
    std::fill(std::begin(d.tlas.phase_max), std::end(d.tlas.phase_max), 0.f);
    const auto clear_work = [](CacheWork* work) {
      work->blas_ms = 0.f;
      work->tlas_ms = 0.f;
      work->capture_ms = 0.f;
    };
    clear_work(&d.work);
    clear_work(&d.prev);
    clear_work(&d.episode.work);
    clear_work(&d.episode.t80_work);
    d.episode.t80_ms = 0.f;
    for (CacheSample& sample : d.episode.samples) {
      sample.ms = 0.f;
      clear_work(&sample.work);
    }
    d.frame_ms_last = 0.f;
    d.frame_ms_sum = 0.0;
    d.frame_intervals = 0u;
    d.last_present = {};
  }
  std::lock_guard<std::mutex> lock(g_perf.mutex);
  g_perf.tab = PerfTabState{};
}

// Called by SamplePerfPresent at every present, before the Collect early return of UpdateCacheDiag (a run works with Collect off).
// Refuses a start (result line says why) while gathering is off, while a run is active, while a map change is swept, or
// during the refill after a map change, pool reset or boot (kCacheSettlePresents). A map change or pool reset during a
// run aborts it, and so does switching gathering off.
inline void UpdatePerfRun(const BvhDeviceData* data, uint32_t frame, std::chrono::steady_clock::time_point now,
                          const CacheSwitches& switches, const PerfExtraGpu& extra = {}) {
  const bool gather = g_perf_gate.gather.load(std::memory_order_relaxed);
  const bool requested = g_perf.start_requested.load(std::memory_order_relaxed) && g_perf.start_requested.exchange(false, std::memory_order_relaxed);
  bool running = g_perf.active.load(std::memory_order_relaxed);
  if (!requested && !running) return;
  uint32_t serial = 0u;
  uint32_t episode_start = 0u;
  uint32_t map_change = 0u;
  PerfCounters counters;
  {
    std::lock_guard<std::mutex> lock(g_pool.mutex);
    serial = g_pool.episode_serial;
    episode_start = g_pool.episode_start_frame;
    map_change = g_pool.map_change_frame;
    if (gather) counters = ReadPerfCounters(data, extra);
  }
  std::string result;
  bool finished = false;
  PerfRunState done;
  {
    std::lock_guard<std::mutex> lock(g_perf.mutex);
    PerfRunState& run = g_perf.run;
    if (running) {
      if (!gather) {
        run.abort_reason = "Gather performance metrics switched off at f" + std::to_string(frame);
        finished = true;
        running = false;
      } else if (serial != run.serial) {
        run.abort_reason = "map change or pool reset at f" + std::to_string(frame);
        finished = true;
        running = false;  // a start in this present is judged on its own
      } else {
        run.presents += 1u;
        const double interval_ms = std::chrono::duration<double, std::milli>(now - run.last).count();
        const PerfTabSample sample = MakePerfSample(counters, run.prev, interval_ms);
        run.frame_ms_sum += interval_ms;
        run.last = now;
        run.elapsed_s = std::chrono::duration<double>(now - run.t0).count();
        run.end_switches = switches;
        run.stable = run.stable && switches == run.start_switches;
        run.end_inclusion = ReadPerfInclusion();
        run.inclusion_stable = run.inclusion_stable && run.end_inclusion == run.start_inclusion;
        for (size_t i = 0; i < kPerfCostCount; ++i) {
          if (kPerfCostInfo[i].cumulative) {
            run.total.cost[i] += counters.cost[i] >= run.prev.cost[i] ? counters.cost[i] - run.prev.cost[i] : counters.cost[i];
            run.total.events[i] += counters.events[i] >= run.prev.events[i] ? counters.events[i] - run.prev.events[i] : counters.events[i];
          } else {
            run.total.cost[i] += counters.cost[i];
          }
        }
        for (size_t i = 0; i < kPerfGaugeCount; ++i) run.total.gauge[i] += counters.gauge[i];
        const PerfTotalsValue totals = PerfTotals(sample.value);
        for (size_t i = 0; i < kPerfCostCount; ++i) MaxInto(&run.max.row[i], sample.value[i]);
        MaxInto(&run.max.frame, interval_ms);
        MaxInto(&run.max.unattr, totals.unattr);
        MaxInto(&run.max.cpu, totals.cpu);
        MaxInto(&run.max.gpu, totals.gpu);
        if (!run.spike_frame.valid || interval_ms > run.spike_frame.frame_ms) run.spike_frame = MakePerfSpike(sample, run.presents - 1u, frame);
        if (!run.spike_unattr.valid || totals.unattr_raw > run.spike_unattr.unattr_raw) run.spike_unattr = MakePerfSpike(sample, run.presents - 1u, frame);
        for (size_t s = 0; s < kPerfSettingCount; ++s) {
          const PerfSettingValue sums = PerfSettingSums(sample.value, kPerfSettings[s]);
          MaxInto(&run.setting_max[s].cpu, sums.cpu);
          MaxInto(&run.setting_max[s].gpu, sums.gpu);
        }
        if (interval_ms > 25.0) run.hitches_over_25ms += 1u;
        if (interval_ms > 33.3) run.hitches_over_33_3ms += 1u;
        for (size_t k = 0; k < kPerfBlockCount; ++k) {
          if (sample.value[kPerfBlockRow + k] > 2.0) run.block_over_2ms[k] += 1u;
        }
        if (run.series.frame_ms.size() < kPerfSeriesCapacity) {
          run.series.frame_ms.push_back(interval_ms);
          run.series.cpu_total.push_back(totals.cpu);
          run.series.tlas_rebuild.push_back(sample.value[kPerfTlasRebuild]);
          run.series.tlas_refit.push_back(sample.value[kPerfTlasRefit]);
          for (size_t k = 0; k < kPerfBlockCount; ++k) run.series.blocks[k].push_back(sample.value[kPerfBlockRow + k]);
        }
        run.prev = counters;
        finished = run.elapsed_s >= kPerfWindowSeconds;
      }
      if (finished) {
        run.end_frame = frame;
        done = run;
        g_perf.active.store(false, std::memory_order_relaxed);
      }
    }
    if (requested) {
      const uint32_t settle_end = episode_start + kCacheSettlePresents;
      if (!gather) {
        result = "Performance test refused: Gather performance metrics is off (switch it on in the Performance tab)";
      } else if (running) {
        result = "Performance test refused: a run is already active";
      } else if (map_change != 0u) {
        result = "Performance test refused: a map change is being swept (since f" + std::to_string(map_change)
                 + "); press again after the sweep (a few seconds)";
      } else if (frame >= episode_start && frame < settle_end) {
        result = "Performance test refused: the pool is refilling after a map change, pool reset or boot (episode from f"
                 + std::to_string(episode_start) + "); press again from f" + std::to_string(settle_end);
      } else {
        run = PerfRunState{};
        run.series.frame_ms.reserve(kPerfSeriesCapacity);
        run.series.cpu_total.reserve(kPerfSeriesCapacity);
        run.series.tlas_rebuild.reserve(kPerfSeriesCapacity);
        run.series.tlas_refit.reserve(kPerfSeriesCapacity);
        for (std::vector<double>& block : run.series.blocks) block.reserve(kPerfSeriesCapacity);
        run.label = g_perf.pending_label;
        run.t0 = now;
        run.last = now;
        run.start_frame = frame;
        run.serial = serial;
        run.start_switches = switches;
        run.end_switches = switches;
        run.start_inclusion = ReadPerfInclusion();
        run.end_inclusion = run.start_inclusion;
        run.prev = counters;
        g_perf.active.store(true, std::memory_order_relaxed);
        result = "Performance test started at f" + std::to_string(frame) + " (30 s, wall clock)";
      }
    }
  }
  if (finished) {
    const std::filesystem::path path = WritePerfFile(done);
    std::error_code ec;
    const uintmax_t bytes = std::filesystem::file_size(path, ec);
    const std::string text = "Performance test " + std::string(done.abort_reason.empty() ? "done" : "aborted (" + done.abort_reason + ")") + ": wrote "
                             + path.string() + " (" + std::to_string(ec ? 0u : bytes) + " bytes)";
    result = result.empty() ? text : text + "; " + result;
  }
  if (!result.empty()) {
    std::lock_guard<std::mutex> lock(g_cache_diag.mutex);
    g_cache_diag.data.last_result = result;
  }
}

// Every present (SamplePerfPresent, gathering on). Cumulative costs are deltas, the others are this present's value. A
// Gpu row is the cost per sample of the present when it gained samples. Unattributed CPU is the two present brackets
// minus the InPresent rows, clamped at zero; the CPU total is the DrawTime and InPresent rows plus the unattributed
// value, so it equals the sum of the rows shown. The first present only primes prev.
inline void UpdatePerfTab(const PerfCounters& now, double frame_ms = std::numeric_limits<double>::quiet_NaN()) {
  std::lock_guard<std::mutex> lock(g_perf.mutex);
  PerfTabState& tab = g_perf.tab;
  if (!tab.primed) {
    tab.prev = now;
    tab.primed = true;
    return;
  }
  tab.ring[tab.next] = MakePerfSample(now, tab.prev, frame_ms);
  tab.prev = now;
  tab.next = (tab.next + 1u) % kPerfTabWindow;
  tab.count = std::min(tab.count + 1u, kPerfTabWindow);

  PerfTabView& view = tab.view;
  double sums[kPerfCostCount] = {};
  size_t counts[kPerfCostCount] = {};
  for (size_t i = 0; i < kPerfCostCount; ++i) view.now[i] = std::numeric_limits<double>::quiet_NaN();
  view.frame_now = std::numeric_limits<double>::quiet_NaN();
  view.max = PerfMaxima{};
  view.spike_age = 0u;
  double frame_sum = 0.0;
  size_t frame_count = 0u;
  const PerfTabSample* spike = nullptr;
  for (size_t k = 0; k < tab.count; ++k) {  // newest first
    const PerfTabSample& s = tab.ring[(tab.next + kPerfTabWindow - 1u - k) % kPerfTabWindow];
    if (!std::isnan(s.frame_ms)) {
      if (frame_count == 0u) view.frame_now = s.frame_ms;
      frame_sum += s.frame_ms;
      frame_count += 1u;
      MaxInto(&view.max.frame, s.frame_ms);
      if (spike == nullptr || s.frame_ms > spike->frame_ms) {
        spike = &s;
        view.spike_age = k;
      }
    }
    for (size_t i = 0; i < kPerfCostCount; ++i) {
      if (kPerfCostInfo[i].role == kPerfGpu && !s.measured[i]) continue;
      if (counts[i] == 0u) view.now[i] = s.value[i];
      sums[i] += s.value[i];
      counts[i] += 1u;
      MaxInto(&view.max.row[i], s.value[i]);
    }
    const PerfTotalsValue totals = PerfTotals(s.value);
    MaxInto(&view.max.unattr, totals.unattr);
    MaxInto(&view.max.cpu, totals.cpu);
    MaxInto(&view.max.gpu, totals.gpu);
  }
  for (size_t i = 0; i < kPerfCostCount; ++i) {
    view.avg[i] = counts[i] != 0u ? sums[i] / static_cast<double>(counts[i]) : std::numeric_limits<double>::quiet_NaN();
  }
  view.frame_avg = frame_count != 0u ? frame_sum / static_cast<double>(frame_count) : std::numeric_limits<double>::quiet_NaN();
  const PerfTotalsValue now_totals = PerfTotals(view.now);
  const PerfTotalsValue avg_totals = PerfTotals(view.avg);
  view.unattr_raw_now = now_totals.unattr_raw;
  view.unattr_now = now_totals.unattr;
  view.cpu_now = now_totals.cpu;
  view.hook_outer_now = now_totals.hook_outer;
  view.gpu_now = now_totals.gpu;
  view.unattr_raw_avg = avg_totals.unattr_raw;
  view.unattr_avg = avg_totals.unattr;
  view.cpu_avg = avg_totals.cpu;
  view.hook_outer_avg = avg_totals.hook_outer;
  view.gpu_avg = avg_totals.gpu;
  view.spike = spike != nullptr ? MakePerfSpike(*spike, 0u, 0u) : PerfSpike{};
}

// The panel asks for a run; the next present decides (see UpdatePerfRun).
inline void RequestPerfRun(const std::string& raw_label) {
  {
    std::lock_guard<std::mutex> lock(g_perf.mutex);
    g_perf.pending_label = raw_label;
  }
  g_perf.start_requested.store(true, std::memory_order_relaxed);
}

}  // namespace falcom_world::bvh
