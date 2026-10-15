#pragma once

// Performance test (diagnostic only): a 30 s wall-clock window of presents with the cost counters read at each present.
// Frame time is the interval between two successive presents (UpdateCacheDiag). Every cost is an average per present,
// so two runs with different present counts compare fairly. Only always-on counters are read (no Collect switch needed).
//
// Lock rule: g_perf.mutex is a leaf lock. It is never held together with g_cache_diag.mutex or g_pool.mutex. The pool
// lock is taken (briefly, for the counters) only while a run is active or a start is pending. File I/O and the result
// line are written after every lock is released.

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
  kPerfMeshCapture,
  kPerfPoolHooks,
  kPerfPoolDrain,
  kPerfDeformCpu,
  kPerfCostCount
};
inline constexpr const char* kPerfCostNames[kPerfCostCount] = {"tlas_rebuild", "tlas_skip", "tlas_apply", "tlas_refit", "blas_build",
                                                               "gpu_trace",   "mesh_capture", "pool_hooks", "pool_drain", "deform_cpu"};
inline constexpr bool kPerfCostCumulative[kPerfCostCount] = {true, true, true, true, true, true, false, false, false, false};

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

struct PerfState {
  std::atomic_bool start_requested{false};  // set by the button, taken by the next present
  std::atomic_bool active{false};           // read by the panel and by Reset
  std::mutex mutex;
  std::string pending_label;  // label typed when the start was requested
  PerfRunState run;           // the current or the last run
};

inline PerfState g_perf;

// Counters at one present. Caller holds g_pool.mutex (tlas_timing and the admitted count).
inline PerfCounters ReadPerfCounters(const BvhDeviceData* data, uint64_t capture_us) {
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
    if (i == kPerfGpuTrace) {
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
                          const CacheSwitches& switches) {
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
    counters = ReadPerfCounters(data, capture_us);
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

// The panel asks for a run; the next present decides (see UpdatePerfRun).
inline void RequestPerfRun(const std::string& raw_label) {
  {
    std::lock_guard<std::mutex> lock(g_perf.mutex);
    g_perf.pending_label = raw_label;
  }
  g_perf.start_requested.store(true, std::memory_order_relaxed);
}

}  // namespace falcom_world::bvh
