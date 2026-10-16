#pragma once

// Performance test (diagnostic only): a 30 s wall-clock window of presents with the cost counters read at each present.
// Frame time is the interval between two successive presents (UpdateCacheDiag). Every cost is an average per present,
// so two runs with different present counts compare fairly. Only always-on counters are read (no Collect switch needed).
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
#include <limits>
#include <mutex>
#include <sstream>
#include <string>

#include "cache_diag.hpp"

namespace falcom_world::bvh {

inline constexpr double kPerfWindowSeconds = 30.0;

// Cost entries. Cumulative: the source keeps a running sum (a delta per present). Per present: the value of this present.
enum PerfCost : size_t {
  kPerfTlasRebuild,
  kPerfTlasSkip,
  kPerfTlasApply,
  kPerfTlasRefit,
  kPerfBlasBuild,
  kPerfGpuTrace,
  kPerfGpuAlphaBlit,
  kPerfGpuP2bExpand,
  kPerfGpuDeformRefit,
  kPerfGpuRtao,
  kPerfGpuRtaoFilter,
  kPerfMeshCapture,
  kPerfPoolHooks,
  kPerfPoolDrain,
  kPerfDeformCpu,
  kPerfCostCount
};
inline constexpr const char* kPerfCostNames[kPerfCostCount] = {
    "tlas_rebuild", "tlas_skip", "tlas_apply", "tlas_refit", "blas_build", "gpu_trace", "gpu_alpha_blit", "gpu_p2b_expand",
    "gpu_deform_refit", "gpu_rtao", "gpu_rtao_filter", "mesh_capture", "pool_hooks", "pool_drain", "deform_cpu"};
inline constexpr bool kPerfCostCumulative[kPerfCostCount] = {true, true, true, true, true, true, true, true,
                                                             true, true, true, false, false, false, false};
// GPU entries: ms per sample (delta cost / delta samples). The others are CPU ms.
inline constexpr bool kPerfCostGpu[kPerfCostCount] = {false, false, false, false, false, true, true, true,
                                                      true, true, true, false, false, false, false};

enum PerfGauge : size_t { kPerfGaugeTlasInstances, kPerfGaugePoolInstances, kPerfGaugePoolMeshes, kPerfGaugeSlackHoles, kPerfGaugeCount };
inline constexpr const char* kPerfGaugeNames[kPerfGaugeCount] = {"tlas_instances", "pool_instances", "pool_meshes", "slack_free_holes"};

struct PerfCounters {
  double cost[kPerfCostCount] = {};
  uint64_t events[kPerfCostCount] = {};  // cumulative costs only: rebuilds, skips, applies, refits, builds, GPU samples
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

// Performance tab: the last kPerfTabWindow presents. NaN means "--" (no sample in the window).
inline constexpr size_t kPerfTabWindow = 30;
struct PerfTabSample {
  double cpu_ms = 0.0;
  double gpu_ms = 0.0;
  bool gpu_measured = false;
};
struct PerfTabView {
  double cpu_now = std::numeric_limits<double>::quiet_NaN();
  double cpu_avg = std::numeric_limits<double>::quiet_NaN();
  double gpu_now = std::numeric_limits<double>::quiet_NaN();
  double gpu_avg = std::numeric_limits<double>::quiet_NaN();
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

// Counters at one present. Caller holds g_pool.mutex (tlas_timing and the admitted count).
inline PerfCounters ReadPerfCounters(const BvhDeviceData* data, uint64_t capture_us, const PerfExtraGpu& extra = {}) {
  const LiveBvhStats& live = data->live;
  const PoolTlasTiming& timing = g_pool.tlas_timing;
  PerfCounters c;
  c.cost[kPerfTlasRebuild] = timing.rebuild_ms_sum;
  c.events[kPerfTlasRebuild] = timing.rebuilds;
  c.cost[kPerfTlasSkip] = live.tlas_skip_ms_sum;
  c.events[kPerfTlasSkip] = live.tlas_skips;
  c.cost[kPerfTlasApply] = live.slack_apply_ms_sum;
  c.events[kPerfTlasApply] = live.slack_applies;
  c.cost[kPerfTlasRefit] = timing.refit_ms_sum;
  c.events[kPerfTlasRefit] = timing.refits;
  c.cost[kPerfBlasBuild] = live.blas_build_ms_sum;
  c.events[kPerfBlasBuild] = live.blas_builds;
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
  c.cost[kPerfMeshCapture] = static_cast<double>(capture_us) / 1000.0;
  c.cost[kPerfPoolHooks] = static_cast<double>(g_pool.cpu_hook_frame_us.load(std::memory_order_relaxed)) / 1000.0;
  c.cost[kPerfPoolDrain] = static_cast<double>(g_pool.cpu_drain_frame_us.load(std::memory_order_relaxed)) / 1000.0;
  c.cost[kPerfDeformCpu] = static_cast<double>(g_deform_live.cpu_frame_us.load(std::memory_order_relaxed)) / 1000.0;
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
  for (size_t i = 0; i < kPerfCostCount; ++i) {
    const double total = run.total.cost[i];
    const double events = static_cast<double>(run.total.events[i]);
    out << (i != 0u ? ", " : "") << "\"" << kPerfCostNames[i] << "\": {";
    if (kPerfCostGpu[i]) {
      out << "\"basis\": \"" << (run.total.events[i] != 0u ? "measured" : "not measured") << "\", \"avg_per_measured_frame\": ";
      WritePerfValue(&out, events != 0.0 ? total / events : nan, "ms/measured frame");
      out << ", \"total\": ";
      WritePerfValue(&out, total, "ms");
      out << ", \"samples\": ";
      WritePerfValue(&out, events, "count");
    } else {
      out << "\"avg_per_present\": ";
      WritePerfValue(&out, run.presents != 0u ? total / presents : nan, "ms/present");
      out << ", \"total\": ";
      WritePerfValue(&out, total, "ms");
      if (kPerfCostCumulative[i]) {
        out << ", \"events\": ";
        WritePerfValue(&out, events, "count");
        out << ", \"avg_per_event\": ";
        WritePerfValue(&out, events != 0.0 ? total / events : nan, "ms/event");
        out << ", \"rate\": ";
        WritePerfValue(&out, actual_s > 0.0 ? events / actual_s : nan, "count/s");
      }
    }
    out << "}";
  }
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

// Called by UpdateCacheDiag at every present, before its Collect early return (a run works with Collect off).
// Refuses a start (result line says why) while a run is active, while a map change is swept, or during the refill
// after a map change, pool reset or boot (kCacheSettlePresents). A map change or pool reset during a run aborts it.
inline void UpdatePerfRun(const BvhDeviceData* data, uint32_t frame, uint64_t capture_us, std::chrono::steady_clock::time_point now,
                          const CacheSwitches& switches, const PerfExtraGpu& extra = {}) {
  const bool requested = g_perf.start_requested.exchange(false, std::memory_order_relaxed);
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
    counters = ReadPerfCounters(data, capture_us, extra);
  }
  std::string result;
  bool finished = false;
  PerfRunState done;
  {
    std::lock_guard<std::mutex> lock(g_perf.mutex);
    PerfRunState& run = g_perf.run;
    if (running) {
      if (serial != run.serial) {
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
          if (kPerfCostCumulative[i]) {
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
      if (running) {
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

// Every present (UpdateCacheDiag): CPU per present is the sum of the non-GPU costs (cumulative ones as deltas); GPU per
// present is the sum over the GPU timers that gained samples of delta cost / delta samples. The first present primes prev.
inline void UpdatePerfTab(const PerfCounters& now) {
  std::lock_guard<std::mutex> lock(g_perf.mutex);
  PerfTabState& tab = g_perf.tab;
  if (!tab.primed) {
    tab.prev = now;
    tab.primed = true;
    return;
  }
  PerfTabSample sample;
  for (size_t i = 0; i < kPerfCostCount; ++i) {
    const double cost = kPerfCostCumulative[i] && now.cost[i] >= tab.prev.cost[i] ? now.cost[i] - tab.prev.cost[i] : now.cost[i];
    if (!kPerfCostGpu[i]) {
      sample.cpu_ms += cost;
      continue;
    }
    const uint64_t gained = now.events[i] >= tab.prev.events[i] ? now.events[i] - tab.prev.events[i] : now.events[i];
    if (gained != 0u) {
      sample.gpu_ms += cost / static_cast<double>(gained);
      sample.gpu_measured = true;
    }
  }
  tab.prev = now;
  tab.ring[tab.next] = sample;
  tab.next = (tab.next + 1u) % kPerfTabWindow;
  tab.count = std::min(tab.count + 1u, kPerfTabWindow);

  double cpu_sum = 0.0, gpu_sum = 0.0;
  size_t gpu_count = 0u;
  tab.view.gpu_now = std::numeric_limits<double>::quiet_NaN();
  for (size_t k = 0; k < tab.count; ++k) {  // newest first
    const PerfTabSample& s = tab.ring[(tab.next + kPerfTabWindow - 1u - k) % kPerfTabWindow];
    cpu_sum += s.cpu_ms;
    if (!s.gpu_measured) continue;
    if (gpu_count == 0u) tab.view.gpu_now = s.gpu_ms;
    gpu_sum += s.gpu_ms;
    gpu_count += 1u;
  }
  tab.view.cpu_now = sample.cpu_ms;
  tab.view.cpu_avg = cpu_sum / static_cast<double>(tab.count);
  tab.view.gpu_avg = gpu_count != 0u ? gpu_sum / static_cast<double>(gpu_count) : std::numeric_limits<double>::quiet_NaN();
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
