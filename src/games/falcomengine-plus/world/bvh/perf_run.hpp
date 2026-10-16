#pragma once

// Performance tab and performance test (diagnostic only). The tab shows the cost of each present (now and the mean of
// the last kPerfTabWindow presents); the test is a 30 s wall-clock window whose costs are written to perf_*.json. Frame
// time is the interval between two successive presents (UpdateCacheDiag). Every cost is an average per present, so two
// runs with different present counts compare fairly. Nothing is gathered while "Gather performance metrics" is off
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

#include "cache_diag.hpp"

namespace falcom_world::bvh {

inline constexpr double kPerfWindowSeconds = 30.0;

// Where a cost counts. InPresent: runs inside a present bracket (with DrawTime and the unattributed row it makes the CPU
// total). DrawTime: runs at draw time, outside both brackets. Bracket: the two present handlers (their sum minus the
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
  kPerfPresentWorldCpu,
  kPerfPresentBvhCpu,
  kPerfGpuTrace,
  kPerfGpuAlphaBlit,
  kPerfGpuP2bExpand,
  kPerfGpuDeformRefit,
  kPerfGpuRtao,
  kPerfGpuRtaoFilter,
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
  kPerfCostCount
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
    {"present_world_cpu", true, kPerfBracket, false, nullptr},
    {"present_bvh_cpu", true, kPerfBracket, false, nullptr},
    {"gpu_trace", true, kPerfGpu, true, nullptr},
    {"gpu_alpha_blit", true, kPerfGpu, true, nullptr},
    {"gpu_p2b_expand", true, kPerfGpu, true, nullptr},
    {"gpu_deform_refit", true, kPerfGpu, true, nullptr},
    {"gpu_rtao", true, kPerfGpu, true, nullptr},
    {"gpu_rtao_filter", true, kPerfGpu, true, nullptr},
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
};
inline constexpr bool PerfInTotal(const PerfCostInfo& info) {
  return info.role == kPerfInPresent || info.role == kPerfDrawTime || info.role == kPerfGpu;
}

enum PerfGauge : size_t { kPerfGaugeTlasInstances, kPerfGaugePoolInstances, kPerfGaugePoolMeshes, kPerfGaugeSlackHoles, kPerfGaugeCount };
inline constexpr const char* kPerfGaugeNames[kPerfGaugeCount] = {"tlas_instances", "pool_instances", "pool_meshes", "slack_free_holes"};

struct PerfCounters {
  double cost[kPerfCostCount] = {};
  uint64_t events[kPerfCostCount] = {};  // counted costs and GPU rows: rebuilds, skips, applies, refits, builds, GPU samples
  double gauge[kPerfGaugeCount] = {};
};

struct PerfRunState {
  std::string label;  // raw label as typed
  std::chrono::steady_clock::time_point t0{};
  std::chrono::steady_clock::time_point last{};
  uint32_t start_frame = 0u;
  uint32_t end_frame = 0u;
  uint32_t serial = 0u;  // PoolState::episode_serial at the start: a change aborts the run
  uint64_t presents = 0u;  // presents after the start present
  double frame_ms_sum = 0.0;
  double elapsed_s = 0.0;  // for the panel status line
  CacheSwitches start_switches;
  CacheSwitches end_switches;
  bool stable = true;  // no switch changed during the window
  PerfCounters prev;   // counters at the last present
  PerfCounters total;  // sums over the window
  std::string abort_reason;  // empty: the window completed
};

// GPU timers the Performance tab sums: cumulative ms and samples read at each present (UpdateCacheDiag).
struct PerfTimerSample {
  double ms_sum = 0.0;
  uint32_t samples = 0u;
};
struct PerfExtraGpu {
  PerfTimerSample rtao, rtao_filter, deform_refit;
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
  double gpu_now = std::numeric_limits<double>::quiet_NaN();  // sum of the Gpu rows that have a value
  double gpu_avg = std::numeric_limits<double>::quiet_NaN();
  double frame_now = std::numeric_limits<double>::quiet_NaN();  // frame time: newest interval, and the mean over the window
  double frame_avg = std::numeric_limits<double>::quiet_NaN();
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

inline PerfState g_perf;

inline double PerfMsOf(const std::atomic_uint64_t& us) { return static_cast<double>(us.load(std::memory_order_relaxed)) / 1000.0; }

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
  for (size_t p = 0; p < 5u; ++p) c.cost[kPerfRebuildPhase0 + p] = timing.rebuild_phase_ms_sum[p];
  for (size_t p = 0; p < 4u; ++p) c.cost[kPerfRefitPhase0 + p] = timing.refit_phase_ms_sum[p];
  for (size_t p = 0; p < 4u; ++p) c.cost[kPerfSlackPhase0 + p] = live.slack_phase_ms_sum[p];
  c.gauge[kPerfGaugeTlasInstances] = live.tlas_instances;
  c.gauge[kPerfGaugePoolInstances] = static_cast<double>(g_pool.instances.size());
  c.gauge[kPerfGaugePoolMeshes] = live.pool_meshes;
  c.gauge[kPerfGaugeSlackHoles] = live.slack_free_holes;
  return c;
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

// Writes the perf file in PoolOutputDir()/diag and returns its path.
inline std::filesystem::path WritePerfFile(const PerfRunState& run) {
  const double nan = std::numeric_limits<double>::quiet_NaN();
  const auto when = std::chrono::system_clock::now();
  const std::string label = SanitizeDiagLabel(run.label);
  const std::filesystem::path path = DiagFilePath("perf", label, when);
  const double presents = static_cast<double>(run.presents);
  const double actual_s = std::chrono::duration<double>(run.last - run.t0).count();
  const double avg_ms = run.presents != 0u ? run.frame_ms_sum / presents : nan;
  std::ostringstream out;
  out << "{\n  \"schema\": 1,\n  \"kind\": \"performance_test\",\n  \"build\": \"" << PoolJsonEscape(kWorldBuildId) << "\",\n";
  out << "  \"label\": \"" << PoolJsonEscape(label) << "\",\n  \"label_raw\": \"" << PoolJsonEscape(run.label) << "\",\n";
  out << "  \"timestamp\": \"" << DiagTimestamp(when) << "\",\n";
  out << "  \"perf_gather\": " << (g_perf_gate.gather.load(std::memory_order_relaxed) ? "true" : "false") << ",\n";
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
  out << "},\n  \"switches\": {\"start\": ";
  WriteSwitchesJson(&out, run.start_switches);
  out << ", \"end\": ";
  WriteSwitchesJson(&out, run.end_switches);
  out << ", \"stable\": " << (run.stable ? "true" : "false") << "},\n  \"stats\": {";
  double in_total = 0.0, bracket_total = 0.0, draw_total = 0.0, gpu_avg_sum = 0.0;
  bool gpu_measured = false;
  for (size_t i = 0; i < kPerfCostCount; ++i) {
    const PerfCostInfo& info = kPerfCostInfo[i];
    const double total = run.total.cost[i];
    const double events = static_cast<double>(run.total.events[i]);
    out << (i != 0u ? ", " : "") << "\"" << info.name << "\": {";
    if (info.role == kPerfGpu) {
      out << "\"basis\": \"" << (run.total.events[i] != 0u ? "measured" : "not measured") << "\", \"avg_per_measured_frame\": ";
      WritePerfValue(&out, events != 0.0 ? total / events : nan, "ms/measured frame");
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
      if (info.role == kPerfInPresent) in_total += total;
      if (info.role == kPerfDrawTime) draw_total += total;
      if (info.role == kPerfBracket) bracket_total += total;
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
  const double unattr_raw = bracket_total - in_total;
  const double unattr_total = std::max(0.0, unattr_raw);
  out << ", \"unattributed_cpu\": {\"avg_per_present\": ";
  WritePerfValue(&out, run.presents != 0u ? unattr_total / presents : nan, "ms/present");
  out << ", \"total\": ";
  WritePerfValue(&out, unattr_total, "ms");
  out << ", \"clamped\": " << (unattr_raw < -0.01 * presents ? "true" : "false") << ", \"raw_total\": ";
  WritePerfValue(&out, unattr_raw, "ms");
  out << ", \"in_total\": true}";
  const double cpu_total = draw_total + in_total + unattr_total;
  out << ", \"cpu_total\": {\"avg_per_present\": ";
  WritePerfValue(&out, run.presents != 0u ? cpu_total / presents : nan, "ms/present");
  out << ", \"total\": ";
  WritePerfValue(&out, cpu_total, "ms");
  out << ", \"in_total\": false}";
  out << ", \"gpu_total\": {\"avg_per_measured_frame\": ";
  WritePerfValue(&out, gpu_measured ? gpu_avg_sum : nan, "ms/measured frame");
  out << ", \"in_total\": false}";
  out << "},\n  \"gauges\": {";
  for (size_t i = 0; i < kPerfGaugeCount; ++i) {
    out << (i != 0u ? ", " : "") << "\"" << kPerfGaugeNames[i] << "\": {\"avg\": ";
    WritePerfValue(&out, run.presents != 0u ? run.total.gauge[i] / presents : nan, "count");
    out << "}";
  }
  out << "}\n}\n";
  std::string text = out.str();
  renodx::utils::path::WriteTextFile(path, text);
  return path;
}

// Gathering was switched OFF: clears what it gathered (CPU sums, maxima, last values, phase sums, GPU timer statistics,
// cache-diag timings) and the tab window. The RTAO timers are cleared by rtao::ClearRtaoPerfGathered. Runs on the present
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
  g_perf_gate.deform_update_us.store(0u);
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

// Called by UpdateCacheDiag at every present, before its Collect early return (a run works with Collect off).
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
        run.frame_ms_sum += std::chrono::duration<double, std::milli>(now - run.last).count();
        run.last = now;
        run.elapsed_s = std::chrono::duration<double>(now - run.t0).count();
        run.end_switches = switches;
        run.stable = run.stable && switches == run.start_switches;
        for (size_t i = 0; i < kPerfCostCount; ++i) {
          if (kPerfCostInfo[i].cumulative) {
            run.total.cost[i] += counters.cost[i] >= run.prev.cost[i] ? counters.cost[i] - run.prev.cost[i] : counters.cost[i];
            run.total.events[i] += counters.events[i] >= run.prev.events[i] ? counters.events[i] - run.prev.events[i] : counters.events[i];
          } else {
            run.total.cost[i] += counters.cost[i];
          }
        }
        for (size_t i = 0; i < kPerfGaugeCount; ++i) run.total.gauge[i] += counters.gauge[i];
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
        run.label = g_perf.pending_label;
        run.t0 = now;
        run.last = now;
        run.start_frame = frame;
        run.serial = serial;
        run.start_switches = switches;
        run.end_switches = switches;
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

// Every present (UpdateCacheDiag, gathering on). Cumulative costs are deltas, the others are this present's value. A
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
  PerfTabSample sample;
  sample.frame_ms = frame_ms;
  for (size_t i = 0; i < kPerfCostCount; ++i) {
    const double cost = kPerfCostInfo[i].cumulative && now.cost[i] >= tab.prev.cost[i] ? now.cost[i] - tab.prev.cost[i] : now.cost[i];
    if (kPerfCostInfo[i].role != kPerfGpu) {
      sample.value[i] = cost;
      continue;
    }
    const uint64_t gained = now.events[i] >= tab.prev.events[i] ? now.events[i] - tab.prev.events[i] : now.events[i];
    if (gained != 0u) {
      sample.value[i] = cost / static_cast<double>(gained);
      sample.measured[i] = true;
    }
  }
  tab.prev = now;
  tab.ring[tab.next] = sample;
  tab.next = (tab.next + 1u) % kPerfTabWindow;
  tab.count = std::min(tab.count + 1u, kPerfTabWindow);

  PerfTabView& view = tab.view;
  double sums[kPerfCostCount] = {};
  size_t counts[kPerfCostCount] = {};
  for (size_t i = 0; i < kPerfCostCount; ++i) view.now[i] = std::numeric_limits<double>::quiet_NaN();
  view.frame_now = std::numeric_limits<double>::quiet_NaN();
  double frame_sum = 0.0;
  size_t frame_count = 0u;
  for (size_t k = 0; k < tab.count; ++k) {  // newest first
    const PerfTabSample& s = tab.ring[(tab.next + kPerfTabWindow - 1u - k) % kPerfTabWindow];
    if (!std::isnan(s.frame_ms)) {
      if (frame_count == 0u) view.frame_now = s.frame_ms;
      frame_sum += s.frame_ms;
      frame_count += 1u;
    }
    for (size_t i = 0; i < kPerfCostCount; ++i) {
      if (kPerfCostInfo[i].role == kPerfGpu && !s.measured[i]) continue;
      if (counts[i] == 0u) view.now[i] = s.value[i];
      sums[i] += s.value[i];
      counts[i] += 1u;
    }
  }
  for (size_t i = 0; i < kPerfCostCount; ++i) {
    view.avg[i] = counts[i] != 0u ? sums[i] / static_cast<double>(counts[i]) : std::numeric_limits<double>::quiet_NaN();
  }
  view.frame_avg = frame_count != 0u ? frame_sum / static_cast<double>(frame_count) : std::numeric_limits<double>::quiet_NaN();
  double in_now = 0.0, in_avg = 0.0, bracket_now = 0.0, bracket_avg = 0.0, cpu_now = 0.0, cpu_avg = 0.0;
  double gpu_now = 0.0, gpu_avg = 0.0;
  bool gpu_now_any = false, gpu_avg_any = false;
  for (size_t i = 0; i < kPerfCostCount; ++i) {
    const PerfRole role = kPerfCostInfo[i].role;
    if (role == kPerfInPresent) {
      in_now += view.now[i];
      in_avg += view.avg[i];
    } else if (role == kPerfBracket) {
      bracket_now += view.now[i];
      bracket_avg += view.avg[i];
    } else if (role == kPerfGpu) {
      if (!std::isnan(view.now[i])) {
        gpu_now += view.now[i];
        gpu_now_any = true;
      }
      if (!std::isnan(view.avg[i])) {
        gpu_avg += view.avg[i];
        gpu_avg_any = true;
      }
    }
    if (PerfInTotal(kPerfCostInfo[i]) && role != kPerfGpu) {
      cpu_now += view.now[i];
      cpu_avg += view.avg[i];
    }
  }
  view.unattr_raw_now = bracket_now - in_now;
  view.unattr_raw_avg = bracket_avg - in_avg;
  view.unattr_now = std::max(0.0, view.unattr_raw_now);
  view.unattr_avg = std::max(0.0, view.unattr_raw_avg);
  view.cpu_now = cpu_now + view.unattr_now;
  view.cpu_avg = cpu_avg + view.unattr_avg;
  view.gpu_now = gpu_now_any ? gpu_now : std::numeric_limits<double>::quiet_NaN();
  view.gpu_avg = gpu_avg_any ? gpu_avg : std::numeric_limits<double>::quiet_NaN();
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
