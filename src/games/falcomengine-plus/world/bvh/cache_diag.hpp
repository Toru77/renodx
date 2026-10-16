#pragma once

// Phase 0 cache diagnostics (diagnostic only: no rendering or TLAS behaviour change).
//
// Where the pool's instances sit (distance tiers from the camera), what the pool
// and the resident GPU store hold per tier, which TLAS rebuilds follow a change of
// the instance set, the BLAS build cost, the lighting gap, and how long a map
// change or a pool reset takes to refill the TLAS with fresh instances (an
// episode). Every result is shown in the panel, copied to the clipboard and
// written to a new file in the diag folder (cache_diag_<time>[_<label>].json).
//
// Lock rule: g_cache_diag.mutex is a leaf lock. Nothing else is taken under it;
// the pool and camera are read before it is taken, and file I/O happens outside.

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <iterator>
#include <limits>
#include <numeric>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "bvh_pool.hpp"
#include "bvh_resources.hpp"
#include "deform_live.hpp"

namespace falcom_world::bvh {

// Chebyshev distance from the camera: T0 < 128 m, T1 < 256 m, T2 >= 256 m (inside the region).
inline constexpr float kCacheTierEdges[2] = {128.f, 256.f};
inline constexpr float kCacheHistogramEdges[9] = {16.f, 32.f, 64.f, 96.f, 128.f, 192.f, 256.f, 384.f, 512.f};
inline constexpr size_t kCacheHistogramBuckets = 10u;  // the last bucket is open (>= 512 m)
inline constexpr const char* kCacheHistogramLabels[kCacheHistogramBuckets] = {
    "<16", "16-32", "32-64", "64-96", "96-128", "128-192", "192-256", "256-384", "384-512", ">=512"};
// Object-space extent (largest bbox side, m) of a mesh and its ratio to the distance (largest instance side / max(distance, 1 m)).
inline constexpr float kCacheExtentEdges[8] = {0.25f, 0.5f, 1.f, 2.f, 4.f, 8.f, 16.f, 32.f};
inline constexpr size_t kCacheExtentBuckets = 9u;
inline constexpr const char* kCacheExtentLabels[kCacheExtentBuckets] = {
    "<0.25", "0.25-0.5", "0.5-1", "1-2", "2-4", "4-8", "8-16", "16-32", ">=32"};
inline constexpr float kCacheRatioEdges[4] = {0.0025f, 0.005f, 0.01f, 0.02f};
inline constexpr size_t kCacheRatioBuckets = 5u;
inline constexpr const char* kCacheRatioLabels[kCacheRatioBuckets] = {"<0.0025", "0.0025-0.005", "0.005-0.01", "0.01-0.02", ">=0.02"};
inline constexpr const char* kCacheGapLabels[kPoolResightGapBuckets] = {"<120", "120-600", "600-1800", "1800-3600", ">=3600"};
inline constexpr size_t kCacheLabelMax = 40u;           // file name label: bytes kept after sanitising
inline constexpr uint32_t kCacheSampleInterval = 30u;   // presents between tier computations
inline constexpr uint32_t kCacheSettlePresents = 300u;  // presents without a TLAS rebuild that settle an episode
inline constexpr size_t kCacheMaxSamples = 256u;        // TLAS rebuild samples kept per episode
inline constexpr double kCacheT80Fraction = 0.8;         // t80: the first sample with this share of the final fresh count
inline constexpr size_t kCacheTierOutside = 3u;         // tiers: T0, T1, T2, outside region, no instance
inline constexpr size_t kCacheTierNone = 4u;
inline constexpr size_t kCacheTierCount = 5u;
inline constexpr const char* kCacheTierNames[kCacheTierCount] = {
    "T0 <128 m", "T1 <256 m", "T2 >=256 m (in region)", "outside region", "no instance"};
inline constexpr size_t kCacheTlasTierNone = 3u;  // TLAS rebuild tiers: T0, T1, T2, none
inline constexpr size_t kCacheTlasTierCount = 4u;
inline constexpr float kCacheLatencyEdges[4] = {3.f, 9.f, 31.f, 121.f};  // presents: 0-2, 3-8, 9-30, 31-120, >120
inline constexpr size_t kCacheLatencyBuckets = 5u;
inline constexpr const char* kCacheLatencyLabels[kCacheLatencyBuckets] = {"0-2", "3-8", "9-30", "31-120", ">120"};
inline constexpr size_t kCachePhaseCount = 5u;
inline constexpr const char* kCachePhaseNames[kCachePhaseCount] = {
    "snapshot+lock", "filter+descriptors", "sort+build+validate", "buffer create", "buffer destroy"};
inline constexpr const char* kCacheReasonNames[] = {
    "first build", "store changed", "pool changed", "visibility changed", "region moved", "alpha switch", "sway switch",
    "per-frame switch", "flagged meshes", "refit quality", "refit failed", "slack switch", "slack maintenance", "slack structural",
    "slack exhausted", "slack invalid", "slack upload"};
inline constexpr size_t kCacheReasonCount = std::size(kCacheReasonNames);  // one more counter: other

// Tier of an instance by its Chebyshev distance to the camera.
inline size_t CacheInstanceTier(float distance, bool in_region) {
  if (!in_region) return kCacheTierOutside;
  if (distance < kCacheTierEdges[0]) return 0u;
  return distance < kCacheTierEdges[1] ? 1u : 2u;
}

// Bucket of a value: the number of edges <= value (an edge value goes to the upper bucket); edge_count + 1 buckets.
inline size_t CacheBucket(float value, const float* edges, size_t edge_count) {
  size_t bucket = 0u;
  while (bucket < edge_count && value >= edges[bucket]) ++bucket;
  return bucket;
}

// One tier row. Instances count per instance; meshes count once, in the tier of the nearest instance.
struct CacheTier {
  uint64_t instances = 0u;
  uint64_t instance_tris = 0u;
  uint64_t meshes = 0u;
  uint64_t pool_tris = 0u;
  uint64_t pool_bytes = 0u;  // positions * 12 + indices * 4 + uvs * 8
  uint64_t resident_meshes = 0u;
  uint64_t resident_tris = 0u;   // BLAS leaves of the live slot
  uint64_t resident_bytes = 0u;  // LiveSlotBytes of the live slot
  uint64_t extent[kCacheExtentBuckets] = {};    // meshes, in the tier of their nearest instance
  uint64_t ratio[kCacheRatioBuckets] = {};      // instances
  uint64_t ratio_tris[kCacheRatioBuckets] = {};  // instance triangles
};

// A resident mesh (live slot), by uid, built outside the pool lock.
struct CacheResident {
  uint64_t tris = 0u;
  uint64_t bytes = 0u;
};

struct CacheTiers {
  CacheTier tiers[kCacheTierCount];
  uint64_t histogram[kCacheHistogramBuckets] = {};
  uint64_t instances = 0u;  // pool totals
  uint64_t meshes = 0u;
  uint64_t pool_tris = 0u;
  uint64_t pool_bytes = 0u;
  uint64_t sum_instances = 0u;  // tier sums
  uint64_t sum_meshes = 0u;
  uint64_t sum_tris = 0u;
  uint64_t sum_bytes = 0u;
  bool self_check = false;  // tier sums equal the pool totals
  bool hist_check[kCacheTierCount] = {};  // per tier: extent sum = meshes, ratio sums = instances and instance tris
};

// Caller holds g_pool.mutex. `camera` is the pool camera (zeros when unknown); `resident` by mesh uid.
inline void ComputeCacheTiers(const std::vector<WorldInstance>& instances, const std::vector<WorldMesh>& meshes,
                              const float* camera, const PoolRegion& region,
                              const std::unordered_map<uint64_t, CacheResident>& resident, CacheTiers* out) {
  *out = CacheTiers{};
  std::vector<float> nearest(meshes.size(), std::numeric_limits<float>::infinity());
  std::vector<size_t> nearest_tier(meshes.size(), kCacheTierNone);
  for (const WorldInstance& instance : instances) {
    if (instance.mesh_id >= meshes.size()) continue;
    const WorldMesh& mesh = meshes[instance.mesh_id];
    const float distance = PoolChebyshevDistance(camera, instance.bounds_min, instance.bounds_max);
    const size_t tier = CacheInstanceTier(distance, PoolInstanceInRegion(instance, region));
    out->tiers[tier].instances += 1u;
    out->tiers[tier].instance_tris += mesh.triangle_count;
    out->histogram[CacheBucket(distance, kCacheHistogramEdges, std::size(kCacheHistogramEdges))] += 1u;
    float side = 0.f;
    for (int i = 0; i < 3; ++i) side = std::max(side, instance.bounds_max[i] - instance.bounds_min[i]);
    const size_t ratio = CacheBucket(side / std::max(distance, 1.f), kCacheRatioEdges, std::size(kCacheRatioEdges));
    out->tiers[tier].ratio[ratio] += 1u;
    out->tiers[tier].ratio_tris[ratio] += mesh.triangle_count;
    if (distance < nearest[instance.mesh_id]) {
      nearest[instance.mesh_id] = distance;
      nearest_tier[instance.mesh_id] = tier;
    }
  }
  for (size_t m = 0; m < meshes.size(); ++m) {
    const WorldMesh& mesh = meshes[m];
    const uint64_t bytes = mesh.positions.size() * 12u + mesh.indices.size() * 4u + mesh.uvs.size() * 8u;
    CacheTier& row = out->tiers[nearest_tier[m]];
    row.meshes += 1u;
    row.pool_tris += mesh.triangle_count;
    row.pool_bytes += bytes;
    float extent = 0.f;
    for (int i = 0; i < 3; ++i) extent = std::max(extent, mesh.bbox_max[i] - mesh.bbox_min[i]);
    row.extent[CacheBucket(extent, kCacheExtentEdges, std::size(kCacheExtentEdges))] += 1u;
    const auto found = resident.find(mesh.uid);
    if (found != resident.end()) {
      row.resident_meshes += 1u;
      row.resident_tris += found->second.tris;
      row.resident_bytes += found->second.bytes;
    }
    out->meshes += 1u;
    out->pool_tris += mesh.triangle_count;
    out->pool_bytes += bytes;
  }
  out->instances = instances.size();
  for (const CacheTier& row : out->tiers) {
    out->sum_instances += row.instances;
    out->sum_meshes += row.meshes;
    out->sum_tris += row.pool_tris;
    out->sum_bytes += row.pool_bytes;
  }
  out->self_check = out->sum_instances == out->instances && out->sum_meshes == out->meshes
                    && out->sum_tris == out->pool_tris && out->sum_bytes == out->pool_bytes;
  for (size_t k = 0; k < kCacheTierCount; ++k) {
    const CacheTier& row = out->tiers[k];
    out->hist_check[k] = std::accumulate(std::begin(row.extent), std::end(row.extent), uint64_t{0}) == row.meshes
                         && std::accumulate(std::begin(row.ratio), std::end(row.ratio), uint64_t{0}) == row.instances
                         && std::accumulate(std::begin(row.ratio_tris), std::end(row.ratio_tris), uint64_t{0}) == row.instance_tris;
  }
}

// What one TLAS rebuild changed, by instance id. Wind instances are left out (their pose is swayed every present).
struct CacheTlasDiff {
  uint64_t added = 0u;
  uint64_t removed = 0u;
  uint64_t moved = 0u;               // same id, another matrix
  float nearest_changed = -1.f;      // Chebyshev m from the camera; -1 = none
  float nearest_add_remove = -1.f;
};

// Index of a rebuild reason in kCacheReasonNames; kCacheReasonCount for any other reason.
inline size_t CacheReasonIndex(const char* reason) {
  for (size_t r = 0; r < kCacheReasonCount; ++r) {
    if (std::strcmp(reason, kCacheReasonNames[r]) == 0) return r;
  }
  return kCacheReasonCount;
}

// Pure: `before` and `after` are TLAS snapshots (any order).
inline CacheTlasDiff DiffTlasInstances(const std::vector<LiveTlasInstance>& before,
                                       const std::vector<LiveTlasInstance>& after, const float* camera) {
  std::vector<const LiveTlasInstance*> old_list;
  std::vector<const LiveTlasInstance*> new_list;
  for (const LiveTlasInstance& instance : before) {
    if (!instance.wind && instance.id != 0u) old_list.push_back(&instance);  // id 0: a slack hole
  }
  for (const LiveTlasInstance& instance : after) {
    if (!instance.wind && instance.id != 0u) new_list.push_back(&instance);
  }
  const auto by_id = [](const LiveTlasInstance* a, const LiveTlasInstance* b) { return a->id < b->id; };
  std::sort(old_list.begin(), old_list.end(), by_id);
  std::sort(new_list.begin(), new_list.end(), by_id);
  const auto distance = [&](const LiveTlasInstance* instance) {
    return PoolChebyshevDistance(camera, instance->bounds_min, instance->bounds_max);
  };
  CacheTlasDiff diff;
  const auto nearer = [](float current, float value) { return current < 0.f || value < current ? value : current; };
  size_t i = 0u;
  size_t j = 0u;
  while (i < old_list.size() || j < new_list.size()) {
    if (j == new_list.size() || (i < old_list.size() && old_list[i]->id < new_list[j]->id)) {
      diff.removed += 1u;
      diff.nearest_add_remove = nearer(diff.nearest_add_remove, distance(old_list[i]));
      i += 1u;
    } else if (i == old_list.size() || new_list[j]->id < old_list[i]->id) {
      diff.added += 1u;
      diff.nearest_add_remove = nearer(diff.nearest_add_remove, distance(new_list[j]));
      j += 1u;
    } else {
      if (MatrixHash(old_list[i]->matrix, kPoolWorldFloats) != MatrixHash(new_list[j]->matrix, kPoolWorldFloats)) {
        diff.moved += 1u;
        diff.nearest_changed = nearer(diff.nearest_changed, distance(new_list[j]));
      }
      i += 1u;
      j += 1u;
    }
  }
  return diff;
}

// Pure: ids of the non-zero instances of `after` that `before` does not hold (wind included), ascending.
inline std::vector<uint64_t> CacheEnteredIds(const std::vector<LiveTlasInstance>& before, const std::vector<LiveTlasInstance>& after) {
  std::vector<uint64_t> old_ids;
  for (const LiveTlasInstance& instance : before) {
    if (instance.id != 0u) old_ids.push_back(instance.id);
  }
  std::sort(old_ids.begin(), old_ids.end());
  std::vector<uint64_t> entered;
  for (const LiveTlasInstance& instance : after) {
    if (instance.id != 0u && !std::binary_search(old_ids.begin(), old_ids.end(), instance.id)) entered.push_back(instance.id);
  }
  std::sort(entered.begin(), entered.end());
  return entered;
}

struct CacheTlasStats {
  uint64_t rebuilds = 0u;  // rebuilds diffed (a TLAS that is not empty)
  uint64_t empty = 0u;     // rebuilds that left the TLAS empty (not diffed)
  uint64_t added = 0u;
  uint64_t removed = 0u;
  uint64_t moved = 0u;
  float nearest_changed_m = -1.f;     // over the session, -1 = none
  float nearest_add_remove_m = -1.f;
  uint64_t by_tier[kCacheTlasTierCount] = {};  // rebuilds by tier of the nearest add or remove
  float ms_sum = 0.f;  // TLAS rebuild CPU time (since reset)
  float ms_max = 0.f;
  float phase_last[kCachePhaseCount] = {};  // TLAS rebuild phases (ms)
  float phase_sum[kCachePhaseCount] = {};
  float phase_max[kCachePhaseCount] = {};
  uint64_t noop_full = 0u;         // rebuilds whose snapshot equals the TLAS (descriptors and dynamic poses)
  uint64_t noop_static = 0u;       // rebuilds whose static prefix equals the TLAS
  uint64_t noop_by_cause[3] = {};  // full no-ops by cause: retire, admit, other
  float noop_ms = 0.f;             // TLAS rebuild CPU time of the full no-ops
  uint64_t skips = 0u;             // rebuilds skipped: the snapshot equals the TLAS (skip_noop)
  float skip_ms = 0.f;
  uint64_t skips_by_cause[3] = {};
  uint64_t by_reason[kCacheReasonCount + 1u] = {};
  uint64_t applies = 0u;  // in-place slack updates (not rebuilds)
  uint64_t entered = 0u;                              // first TLAS entries (instances stamped)
  uint64_t admit_latency[kCacheLatencyBuckets] = {};  // presents from admission to the first TLAS entry
  uint64_t seen_latency[kCacheLatencyBuckets] = {};   // presents from the first sighting to the first TLAS entry
};

// Cost of the work done while an episode (or the session since reset) runs: BLAS builds, TLAS rebuilds and mesh capture CPU.
struct CacheWork {
  float blas_ms = 0.f;
  float tlas_ms = 0.f;
  float capture_ms = 0.f;
  uint64_t blas_builds = 0u;
  uint64_t tlas_rebuilds = 0u;
  uint64_t capture_meshes = 0u;
  uint64_t capture_wait_sum = 0u;  // presents from queue to applied
};

struct CacheSample {
  uint32_t frame = 0u;
  float ms = 0.f;              // since the episode start
  uint64_t tlas_instances = 0u;
  uint64_t tlas_fresh = 0u;    // TLAS instances last seen at or after the episode's fresh frame
  uint64_t pool_admitted = 0u;
  CacheWork work;  // episode work at this sample
};

struct CacheEpisode {
  bool open = false;
  bool ever = false;
  const char* state = "none";  // open, settled, interrupted
  const char* reason = "";
  uint32_t serial = 0u;        // PoolState::episode_serial it is for
  uint32_t start_frame = 0u;
  uint32_t fresh_from = 0u;
  std::chrono::steady_clock::time_point start_time{};
  uint32_t quiet_presents = 0u;  // presents since the last TLAS rebuild of the episode
  std::vector<CacheSample> samples;
  bool t80_valid = false;
  uint32_t t80_frames = 0u;
  float t80_ms = 0.f;
  uint64_t final_fresh = 0u;
  CacheWork work;      // episode work so far
  CacheWork t80_work;  // episode work at t80 (valid with t80_valid)
};

// Ends an episode. t80 = the first sample whose fresh count reaches kCacheT80Fraction of the final one (the last sample).
// Samples taken before the start frame are never kept, so a stale start is not credited.
inline void FinishCacheEpisode(CacheEpisode* episode, const char* state) {
  episode->open = false;
  episode->state = state;
  episode->t80_valid = false;
  episode->final_fresh = episode->samples.empty() ? 0u : episode->samples.back().tlas_fresh;
  if (episode->final_fresh == 0u) return;
  for (const CacheSample& sample : episode->samples) {
    if (static_cast<double>(sample.tlas_fresh) < kCacheT80Fraction * static_cast<double>(episode->final_fresh)) continue;
    episode->t80_valid = true;
    episode->t80_frames = sample.frame - episode->start_frame;
    episode->t80_ms = sample.ms;
    episode->t80_work = sample.work;
    return;
  }
}

// Lighting gap: a present is dark when the lighting pass depth is not the one of this frame's camera.
struct CacheLighting {
  uint64_t presents = 0u;
  uint64_t dark_presents = 0u;
  uint64_t dark_runs = 0u;
  uint64_t longest_dark = 0u;
  uint64_t longest_lit = 0u;
  uint64_t run = 0u;  // length of the current run
  bool dark = false;  // state of the last present
  bool known = false;
};

inline void AddCacheLightingSample(CacheLighting* lighting, bool dark) {
  if (!lighting->known || dark != lighting->dark) {
    if (dark) lighting->dark_runs += 1u;
    lighting->run = 0u;
  }
  lighting->run += 1u;
  lighting->known = true;
  lighting->dark = dark;
  lighting->presents += 1u;
  if (dark) {
    lighting->dark_presents += 1u;
    lighting->longest_dark = std::max(lighting->longest_dark, lighting->run);
  } else {
    lighting->longest_lit = std::max(lighting->longest_lit, lighting->run);
  }
}

struct CacheDiagData {
  uint64_t presents = 0u;
  bool tiers_valid = false;
  uint32_t tiers_frame = 0u;  // frame of the last tier computation
  CacheTiers tiers;
  uint32_t serial_seen = 0u;  // PoolState::episode_serial at the last present
  CacheTlasStats tlas;
  CacheEpisode episode;
  CacheLighting lighting;
  PoolResightCounters resight;  // pool re-sightings, copied at each present
  uint64_t resight_tracked = 0u;  // retirements held by the tracker
  CacheWork work;                 // since reset
  CacheWork prev;                 // cumulative BLAS and capture counters at the last present (cleared by reset)
  std::string last_path;
  uint64_t last_bytes = 0u;
  std::string last_result;  // the last panel action's result line
  bool dump_armed = false;  // Dump was pressed once: the next press writes the file (after a present computed the tiers)
  std::chrono::steady_clock::time_point last_present{};  // kept across a reset: the frame interval continues
  float frame_ms_last = 0.f;  // interval between the last two presents
  double frame_ms_sum = 0.0;  // since reset
  uint64_t frame_intervals = 0u;
};

struct CacheDiagState {
  std::atomic_bool enabled{true};
  std::atomic_bool reset_requested{false};        // the counters are cleared by the present (UpdateCacheDiag)
  std::atomic_bool force_tiers_requested{false};  // the tiers are computed by the present (UpdateCacheDiag)
  std::mutex mutex;
  CacheDiagData data;
};

inline CacheDiagState g_cache_diag;

// Clears the counters of the report. The episode serial seen and "ever" are kept, so a
// reset does not start an episode for the pool's existing instances.
inline void ResetCacheDiagCounters() {
  std::lock_guard<std::mutex> lock(g_cache_diag.mutex);
  CacheDiagData fresh;
  fresh.serial_seen = g_cache_diag.data.serial_seen;
  fresh.episode.ever = g_cache_diag.data.episode.ever;
  fresh.presents = g_cache_diag.data.presents;
  fresh.last_path = g_cache_diag.data.last_path;
  fresh.last_bytes = g_cache_diag.data.last_bytes;
  fresh.last_result = g_cache_diag.data.last_result;
  fresh.last_present = g_cache_diag.data.last_present;
  g_cache_diag.data = std::move(fresh);
}

// Diagnostics of one TLAS rebuild, filled by RebuildLiveTlas: phases, no-op test, cause (retire, admit, other) and reason.
struct CacheRebuildExtra {
  float phase_ms[kCachePhaseCount] = {};
  bool noop_full = false;
  bool noop_static = false;
  bool applied = false;  // an in-place slack update: counted as an apply, not a rebuild
  uint32_t cause = 2u;
  const char* reason = "";
};

// Caller holds no lock. `before` is the TLAS this build replaces (moved out before the replacement);
// `after` is the TLAS in place now. `replaced` is false for a failed build, which counts nothing.
inline void RecordCacheTlasRebuild(const std::vector<LiveTlasInstance>& before, const std::vector<LiveTlasInstance>& after,
                                   bool replaced, uint32_t frame, float ms, const CacheRebuildExtra& extra = {}) {
  if (!replaced || !g_cache_diag.enabled.load(std::memory_order_relaxed)) return;
  const PoolCpuTimer record_timer{&g_perf_gate.block_us[kPerfBlockRecordTlas]};
  float camera[3] = {};
  PoolCameraPosition(camera);
  const CacheTlasDiff diff = after.empty() ? CacheTlasDiff{} : DiffTlasInstances(before, after, camera);
  // Entry latency: the first entry of each instance is stamped under the pool lock (a re-entry after a region move is not counted).
  std::vector<size_t> admit_bucket;
  std::vector<size_t> seen_bucket;
  {
    const std::vector<uint64_t> entered = CacheEnteredIds(before, after);
    std::lock_guard<std::mutex> lock(g_pool.mutex);
    for (const uint64_t id : entered) {
      const auto it = std::lower_bound(g_pool.instances.begin(), g_pool.instances.end(), id,
                                       [](const WorldInstance& instance, uint64_t value) { return instance.id < value; });
      if (it == g_pool.instances.end() || it->id != id || it->tlas_entry_frame != 0u) continue;
      it->tlas_entry_frame = frame;
      admit_bucket.push_back(CacheBucket(static_cast<float>(frame >= it->admit_frame ? frame - it->admit_frame : 0u), kCacheLatencyEdges, std::size(kCacheLatencyEdges)));
      seen_bucket.push_back(CacheBucket(static_cast<float>(frame >= it->first_frame ? frame - it->first_frame : 0u), kCacheLatencyEdges, std::size(kCacheLatencyEdges)));
    }
  }
  bool open = false;
  uint32_t fresh_from = 0u;
  {
    std::lock_guard<std::mutex> lock(g_cache_diag.mutex);
    open = g_cache_diag.data.episode.open;
    fresh_from = g_cache_diag.data.episode.fresh_from;
  }
  uint64_t fresh = 0u;
  uint64_t admitted = 0u;
  if (open) {
    std::unordered_set<uint64_t> fresh_ids;
    {
      std::lock_guard<std::mutex> lock(g_pool.mutex);
      admitted = g_pool.instances.size();
      for (const WorldInstance& instance : g_pool.instances) {
        if (PoolInstanceLastSeen(instance) >= fresh_from) fresh_ids.insert(instance.id);
      }
    }
    for (const LiveTlasInstance& instance : after) {
      if (fresh_ids.count(instance.id) != 0u) fresh += 1u;
    }
  }
  std::lock_guard<std::mutex> lock(g_cache_diag.mutex);
  CacheDiagData& d = g_cache_diag.data;
  for (size_t k = 0; k < admit_bucket.size(); ++k) {
    d.tlas.entered += 1u;
    d.tlas.admit_latency[admit_bucket[k]] += 1u;
    d.tlas.seen_latency[seen_bucket[k]] += 1u;
  }
  if (extra.applied) {
    d.tlas.applies += 1u;
    d.tlas.added += diff.added;
    d.tlas.removed += diff.removed;
    d.tlas.moved += diff.moved;
    return;
  }
  d.tlas.ms_sum += ms;
  d.tlas.ms_max = std::max(d.tlas.ms_max, ms);
  d.work.tlas_ms += ms;
  d.work.tlas_rebuilds += 1u;
  for (size_t p = 0; p < kCachePhaseCount; ++p) {
    d.tlas.phase_last[p] = extra.phase_ms[p];
    d.tlas.phase_sum[p] += extra.phase_ms[p];
    d.tlas.phase_max[p] = std::max(d.tlas.phase_max[p], extra.phase_ms[p]);
  }
  d.tlas.by_reason[CacheReasonIndex(extra.reason)] += 1u;
  if (extra.noop_full) {
    d.tlas.noop_full += 1u;
    d.tlas.noop_by_cause[extra.cause] += 1u;
    d.tlas.noop_ms += ms;
  }
  if (extra.noop_static) d.tlas.noop_static += 1u;
  if (after.empty()) {
    d.tlas.empty += 1u;
  } else {
    d.tlas.rebuilds += 1u;
    d.tlas.added += diff.added;
    d.tlas.removed += diff.removed;
    d.tlas.moved += diff.moved;
    if (diff.nearest_changed >= 0.f && (d.tlas.nearest_changed_m < 0.f || diff.nearest_changed < d.tlas.nearest_changed_m)) {
      d.tlas.nearest_changed_m = diff.nearest_changed;
    }
    const size_t tier = diff.nearest_add_remove >= 0.f ? CacheInstanceTier(diff.nearest_add_remove, true) : kCacheTlasTierNone;
    d.tlas.by_tier[tier] += 1u;
    if (diff.nearest_add_remove >= 0.f
        && (d.tlas.nearest_add_remove_m < 0.f || diff.nearest_add_remove < d.tlas.nearest_add_remove_m)) {
      d.tlas.nearest_add_remove_m = diff.nearest_add_remove;
    }
  }
  if (!d.episode.open) return;
  d.episode.quiet_presents = 0u;
  if (frame < d.episode.start_frame) return;
  d.episode.work.tlas_ms += ms;
  d.episode.work.tlas_rebuilds += 1u;
  if (d.episode.samples.size() >= kCacheMaxSamples) return;
  CacheSample& sample = d.episode.samples.emplace_back();
  sample.frame = frame;
  sample.ms = std::chrono::duration<float, std::milli>(std::chrono::steady_clock::now() - d.episode.start_time).count();
  sample.tlas_instances = after.size();
  sample.tlas_fresh = fresh;
  sample.pool_admitted = admitted;
  sample.work = d.episode.work;
}

// Caller holds no lock. A rebuild skipped because its snapshot equals the TLAS (nothing built, uploaded or destroyed).
inline void RecordCacheTlasSkip(float ms, uint32_t cause) {
  if (!g_cache_diag.enabled.load(std::memory_order_relaxed)) return;
  std::lock_guard<std::mutex> lock(g_cache_diag.mutex);
  CacheTlasStats& t = g_cache_diag.data.tlas;
  t.skips += 1u;
  t.skip_ms += ms;
  t.skips_by_cause[cause] += 1u;
}

template <typename... Args>
inline std::string CacheFormat(const char* format, Args... args) {
  const int size = std::snprintf(nullptr, 0, format, args...);
  std::string text(static_cast<size_t>(size > 0 ? size : 0), '\0');
  if (size > 0) std::snprintf(text.data(), text.size() + 1u, format, args...);
  return text;
}

// TLAS rebuild phases (ms): average, max and last per phase.
inline std::string CacheTlasPhaseLine(const CacheDiagData& d) {
  const CacheTlasStats& t = d.tlas;
  const CacheWork& w = d.work;
  std::string phases = "TLAS rebuild phases (ms) avg/max/last: ";
  for (size_t p = 0; p < kCachePhaseCount; ++p) {
    phases += CacheFormat("%s%s %.3f/%.3f/%.3f", p != 0u ? ", " : "", kCachePhaseNames[p],
                          w.tlas_rebuilds != 0u ? static_cast<double>(t.phase_sum[p]) / static_cast<double>(w.tlas_rebuilds) : 0.0,
                          static_cast<double>(t.phase_max[p]), static_cast<double>(t.phase_last[p]));
  }
  return phases;
}

// The report: clipboard text and the "lines" key of the cache diagnostics file.
inline std::vector<std::string> CacheDiagLines(const BvhDeviceData* data) {
  CacheDiagData d;
  {
    std::lock_guard<std::mutex> lock(g_cache_diag.mutex);
    d = g_cache_diag.data;
  }
  std::vector<std::string> lines;
  lines.push_back(CacheFormat("Cache diagnostics (%s): %llu presents, tiers %s%s", kWorldBuildId, d.presents,
                              d.tiers_valid ? CacheFormat("at f%u (every %u presents or on dump)", d.tiers_frame, kCacheSampleInterval).c_str()
                                            : "not computed yet",
                              g_perf_gate.gather.load(std::memory_order_relaxed) ? "" : " (performance metrics OFF: timings read 0)"));
  if (d.tiers_valid) {
    for (size_t t = 0; t < kCacheTierCount; ++t) {
      const CacheTier& row = d.tiers.tiers[t];
      lines.push_back(CacheFormat("%s: instances %llu (%llu tris), meshes %llu (%llu tris, %.2f MB), resident %llu meshes (%llu tris, %.2f MB)",
                                  kCacheTierNames[t], row.instances, row.instance_tris, row.meshes, row.pool_tris,
                                  static_cast<double>(row.pool_bytes) / 1048576.0, row.resident_meshes, row.resident_tris,
                                  static_cast<double>(row.resident_bytes) / 1048576.0));
    }
    if (d.tiers.self_check) {
      lines.push_back(CacheFormat("Self-check: PASS (tier sums = pool totals: %llu instances, %llu meshes, %llu tris, %llu bytes)",
                                  d.tiers.instances, d.tiers.meshes, d.tiers.pool_tris, d.tiers.pool_bytes));
    } else {
      lines.push_back(CacheFormat("Self-check: FAIL (tier sums %llu instances, %llu meshes, %llu tris, %llu bytes; pool totals %llu, %llu, %llu, %llu)",
                                  d.tiers.sum_instances, d.tiers.sum_meshes, d.tiers.sum_tris, d.tiers.sum_bytes,
                                  d.tiers.instances, d.tiers.meshes, d.tiers.pool_tris, d.tiers.pool_bytes));
    }
    std::string histogram = "Instance distance (m): ";
    for (size_t b = 0; b < kCacheHistogramBuckets; ++b) {
      histogram += CacheFormat("%s%s %llu", b != 0u ? ", " : "", kCacheHistogramLabels[b], d.tiers.histogram[b]);
    }
    lines.push_back(histogram);
  }
  const CacheTlasStats& t = d.tlas;
  lines.push_back(CacheFormat(
      "TLAS changes: rebuilds %llu (empty %llu), added %llu, removed %llu, moved %llu, nearest changed %s, nearest add/remove %s; by tier of nearest add/remove: T0 %llu, T1 %llu, T2 %llu, none %llu",
      t.rebuilds, t.empty, t.added, t.removed, t.moved,
      t.nearest_changed_m < 0.f ? "-" : CacheFormat("%.1f m", t.nearest_changed_m).c_str(),
      t.nearest_add_remove_m < 0.f ? "-" : CacheFormat("%.1f m", t.nearest_add_remove_m).c_str(), t.by_tier[0],
      t.by_tier[1], t.by_tier[2], t.by_tier[kCacheTlasTierNone]));
  const CacheWork& w = d.work;
  lines.push_back(CacheTlasPhaseLine(d));

  lines.push_back(CacheFormat(
      "TLAS no-op rebuilds: full %llu, static %llu (%.1f %% of rebuilds), by cause retire %llu, admit %llu, other %llu, %.2f ms spent; skipped %llu (%.2f ms; by cause retire %llu, admit %llu, other %llu); in-place applies %llu",
      t.noop_full, t.noop_static, w.tlas_rebuilds != 0u ? 100.0 * static_cast<double>(t.noop_static) / static_cast<double>(w.tlas_rebuilds) : 0.0,
      t.noop_by_cause[0], t.noop_by_cause[1], t.noop_by_cause[2], static_cast<double>(t.noop_ms), t.skips, static_cast<double>(t.skip_ms),
      t.skips_by_cause[0], t.skips_by_cause[1], t.skips_by_cause[2], t.applies));
  std::string latency = "TLAS entry latency (presents) admit->TLAS ";
  std::string seen = "; first seen->TLAS ";
  for (size_t b = 0; b < kCacheLatencyBuckets; ++b) {
    latency += CacheFormat("%s%s %llu", b != 0u ? ", " : "", kCacheLatencyLabels[b], t.admit_latency[b]);
    seen += CacheFormat("%s%s %llu", b != 0u ? ", " : "", kCacheLatencyLabels[b], t.seen_latency[b]);
  }
  lines.push_back(latency + seen + CacheFormat("; entered %llu", t.entered));
  std::string reasons = "TLAS rebuild reasons: ";
  for (size_t r = 0; r <= kCacheReasonCount; ++r) {
    reasons += CacheFormat("%s%s %llu", r != 0u ? ", " : "", r < kCacheReasonCount ? kCacheReasonNames[r] : "other", t.by_reason[r]);
  }
  lines.push_back(reasons);
  const PoolResightCounters& r = d.resight;
  lines.push_back(CacheFormat(
      "Re-sightings: retired %llu (retainable %llu, %llu tris), new meshes %llu, re-sighted %llu (%.1f %%), retainable %llu; near misses: count mismatch %llu (%.1f %%), drift %llu; tracked %llu (dropped %llu, expired %llu)",
      r.retired, r.retired_retainable, r.retired_tris, r.new_meshes, r.resights,
      r.new_meshes != 0u ? 100.0 * r.resights / r.new_meshes : 0.0, r.resights_retainable, r.count_mismatch,
      r.resights != 0u ? 100.0 * r.count_mismatch / r.resights : 0.0, r.drift_near_misses, d.resight_tracked, r.retired_dropped, r.expired));
  std::string gap = "Re-sighting gap (presents since retirement): ";
  for (size_t b = 0; b < kPoolResightGapBuckets; ++b) gap += CacheFormat("%s%s %llu", b != 0u ? ", " : "", kCacheGapLabels[b], r.gap[b]);
  lines.push_back(gap + CacheFormat("; within hold (<1800) %llu (%.1f %% of new meshes); triangles not rebuilt (counts equal) %llu",
                                    r.gap[0] + r.gap[1] + r.gap[2], r.new_meshes != 0u ? 100.0 * (r.gap[0] + r.gap[1] + r.gap[2]) / r.new_meshes : 0.0,
                                    r.revivable_tris));
  lines.push_back(CacheFormat(
      "Time since reset: TLAS rebuilds %llu, avg %.2f ms, max %.2f ms; mesh capture CPU %.2f ms over %llu meshes (decode + apply; GPU read-back wait not CPU), queue-to-applied wait avg %.1f, max %llu presents",
      w.tlas_rebuilds, w.tlas_rebuilds != 0u ? static_cast<double>(t.ms_sum) / static_cast<double>(w.tlas_rebuilds) : 0.0,
      static_cast<double>(t.ms_max), static_cast<double>(w.capture_ms), r.capture_meshes,
      r.capture_meshes != 0u ? static_cast<double>(r.capture_wait_sum) / static_cast<double>(r.capture_meshes) : 0.0, r.capture_wait_max));
  if (d.tiers_valid) {
    for (size_t k = 0; k < kCacheTlasTierNone; ++k) {
      const CacheTier& row = d.tiers.tiers[k];
      std::string size = CacheFormat("T%zu size: mesh extent (m) ", k);
      for (size_t b = 0; b < kCacheExtentBuckets; ++b) size += CacheFormat("%s%s %llu", b != 0u ? ", " : "", kCacheExtentLabels[b], row.extent[b]);
      size += "; extent/distance ";
      for (size_t b = 0; b < kCacheRatioBuckets; ++b) size += CacheFormat("%s%s %llu", b != 0u ? ", " : "", kCacheRatioLabels[b], row.ratio[b]);
      size += " (tris ";
      for (size_t b = 0; b < kCacheRatioBuckets; ++b) size += CacheFormat("%s%llu", b != 0u ? ", " : "", row.ratio_tris[b]);
      lines.push_back(size + ")");
    }
    const bool hist_pass = std::all_of(std::begin(d.tiers.hist_check), std::end(d.tiers.hist_check), [](bool pass) { return pass; });
    lines.push_back(std::string("Histogram self-check: ") + (hist_pass ? "PASS" : "FAIL"));
  }
  if (std::strcmp(d.episode.state, "none") != 0) {
    const CacheWork& ew = d.episode.work;
    const double t80_ms = static_cast<double>(d.episode.t80_ms);
    const CacheWork& tw = d.episode.t80_work;
    const std::string at_t80 = d.episode.t80_valid
        ? CacheFormat("at t80 (+%u frames / %.1f ms): BLAS %.2f ms (%.1f %%), TLAS %.2f ms (%.1f %%), capture %.2f ms (%.1f %%)",
                      d.episode.t80_frames, t80_ms, static_cast<double>(tw.blas_ms), 100.0 * tw.blas_ms / t80_ms, static_cast<double>(tw.tlas_ms),
                      100.0 * tw.tlas_ms / t80_ms, static_cast<double>(tw.capture_ms), 100.0 * tw.capture_ms / t80_ms)
        : std::string("at t80 not reached");
    lines.push_back(CacheFormat(
        "Episode work (so far): BLAS %.2f ms (%llu builds), TLAS %.2f ms (%llu rebuilds), capture CPU %.2f ms (%llu meshes, wait avg %.1f presents); %s",
        static_cast<double>(ew.blas_ms), ew.blas_builds, static_cast<double>(ew.tlas_ms), ew.tlas_rebuilds, static_cast<double>(ew.capture_ms),
        ew.capture_meshes, ew.capture_meshes != 0u ? static_cast<double>(ew.capture_wait_sum) / static_cast<double>(ew.capture_meshes) : 0.0,
        at_t80.c_str()));
  }
  if (data != nullptr) {
    const LiveBvhStats& live = data->live;
    lines.push_back(CacheFormat("BLAS builds %llu (%llu triangles built), %.2f ms avg, %.2f ms max (since reset)", live.blas_builds,
                                live.blas_built_triangles,
                                live.blas_builds != 0u ? static_cast<double>(live.blas_build_ms_sum) / static_cast<double>(live.blas_builds) : 0.0,
                                static_cast<double>(live.blas_build_ms_max)));
  }
  const CacheEpisode& e = d.episode;
  if (!e.ever) {
    lines.push_back("Episode: none yet (starts at a map change, a pool reset or the first admitted instance)");
  } else if (std::strcmp(e.state, "none") == 0) {
    lines.push_back("Episode: none since reset");
  } else if (e.open) {
    const unsigned long long fresh_now = e.samples.empty() ? 0ull : static_cast<unsigned long long>(e.samples.back().tlas_fresh);
    lines.push_back(CacheFormat("Episode: %s (%s, serial %u, start f%u, fresh from f%u): open: t80 pending, fresh now %llu; %llu TLAS samples",
                                e.state, e.reason, e.serial, e.start_frame, e.fresh_from, fresh_now,
                                static_cast<unsigned long long>(e.samples.size())));
  } else {
    const std::string t80 = e.t80_valid ? CacheFormat("+%u frames / %.1f ms", e.t80_frames, static_cast<double>(e.t80_ms))
                                        : std::string("not reached");
    lines.push_back(CacheFormat("Episode: %s (%s, serial %u, start f%u, fresh from f%u): t80 %s; fresh %llu of final; %llu TLAS samples",
                                e.state, e.reason, e.serial, e.start_frame, e.fresh_from, t80.c_str(), e.final_fresh,
                                static_cast<unsigned long long>(e.samples.size())));
  }
  const CacheLighting& light = d.lighting;
  lines.push_back(CacheFormat("Lighting gap: %s now; dark %llu of %llu presents, dark runs %llu, longest dark %llu, longest lit %llu presents",
                              !light.known ? "no data" : light.dark ? "DARK" : "lit", light.dark_presents, light.presents,
                              light.dark_runs, light.longest_dark, light.longest_lit));
  lines.push_back(d.last_path.empty() ? std::string("Last file: none written in this session")
                                      : CacheFormat("Last file: %s (%llu bytes)", d.last_path.c_str(), d.last_bytes));
  return lines;
}

// Switches of the diagnostics as they were when a file was written or a perf run started.
struct CacheSwitches {
  bool collect = false;
  bool skip_noop = false;
  bool slack = false;
  bool live_bvh = false;
  bool operator==(const CacheSwitches& other) const {
    return collect == other.collect && skip_noop == other.skip_noop && slack == other.slack && live_bvh == other.live_bvh;
  }
};

inline void WriteSwitchesJson(std::ostringstream* out, const CacheSwitches& s) {
  *out << "{\"collect_cache_diagnostics\": " << (s.collect ? "true" : "false") << ", \"skip_noop_tlas\": " << (s.skip_noop ? "true" : "false")
       << ", \"slack_tlas\": " << (s.slack ? "true" : "false") << ", \"live_bvh\": " << (s.live_bvh ? "true" : "false") << "}";
}

// Every error counter of the report (the panel's Errors line and the dump's errors block). Locks: g_pool.mutex and
// g_deform_live.mutex are taken one after the other, never nested; the caller holds no lock.
struct CacheErrors {
  uint64_t meshes = 0u;
  uint64_t total = 0u;
  struct Readback {
    uint64_t map_failures = 0u;
    uint64_t staging_failures = 0u;
    uint64_t base_mismatch = 0u;
  } readback;
  struct Bvh {
    uint64_t blas_invalid = 0u;
    uint64_t tlas_invalid = 0u;
    uint64_t upload_failures = 0u;
    uint64_t refit_failures = 0u;
    uint64_t invalid_refs = 0u;
    uint64_t stack_overflow = 0u;
  } bvh;
  struct Deform {
    uint64_t failed = 0u;
    uint64_t resource_failures = 0u;
    uint64_t map_failures = 0u;
  } deform;
};

inline CacheErrors CollectCacheErrors(const BvhDeviceData* data) {
  CacheErrors e;
  {
    std::lock_guard<std::mutex> lock(g_pool.mutex);
    UpdatePoolStats();
    e.meshes = g_pool.stats.mesh_failures;
    e.readback.map_failures = g_pool.stats.map_failures;
    e.readback.staging_failures = g_pool.stats.staging_failures;
    e.readback.base_mismatch = g_pool.stats.base_mismatch;
  }
  {
    std::lock_guard<std::mutex> lock(g_deform_live.mutex);
    e.deform.failed = g_deform_live.stats.failed;
    e.deform.resource_failures = g_deform_live.stats.resource_failures;
    e.deform.map_failures = g_deform_live.stats.map_failures;
  }
  e.bvh.blas_invalid = data->live.blas_invalid;
  e.bvh.tlas_invalid = data->live.tlas_invalid;
  e.bvh.upload_failures = data->live.upload_failures;
  e.bvh.refit_failures = data->live.refit_failures;
  e.bvh.invalid_refs = data->trace_stats.invalid_refs;
  e.bvh.stack_overflow = data->trace_stats.stack_overflow;
  e.total = e.meshes + e.readback.map_failures + e.readback.staging_failures + e.readback.base_mismatch + e.bvh.blas_invalid
            + e.bvh.tlas_invalid + e.bvh.upload_failures + e.bvh.refit_failures + e.bvh.invalid_refs + e.bvh.stack_overflow
            + e.deform.failed + e.deform.resource_failures + e.deform.map_failures;
  return e;
}

// File name label: [A-Za-z0-9._-] kept, every other byte becomes '_', runs collapsed, trimmed of "_.-" at both ends, at most kCacheLabelMax bytes.
inline std::string SanitizeDiagLabel(const std::string& raw) {
  std::string text;
  for (const char c : raw) {
    const bool keep = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '.' || c == '-' || c == '_';
    const char mapped = keep ? c : '_';
    if (mapped == '_' && !text.empty() && text.back() == '_') continue;
    text += mapped;
  }
  const size_t first = text.find_first_not_of("_.-");
  text = first == std::string::npos ? std::string() : text.substr(first, text.find_last_not_of("_.-") - first + 1);
  if (text.size() > kCacheLabelMax) text.resize(kCacheLabelMax);
  return text;
}

inline std::string DiagTimestamp(std::chrono::system_clock::time_point when) {
  const std::time_t time = std::chrono::system_clock::to_time_t(when);
  std::tm local{};
  localtime_s(&local, &time);
  char text[32] = {};
  std::strftime(text, sizeof(text), "%Y%m%d-%H%M%S", &local);
  return text;
}

// A new file in PoolOutputDir()/diag: <prefix>_<timestamp>[_<label>].json, with -2, -3, ... when the name is taken. Never overwrites.
inline std::filesystem::path DiagFilePath(const char* prefix, const std::string& label, std::chrono::system_clock::time_point when) {
  const std::filesystem::path folder = PoolOutputDir() / "diag";
  std::error_code ec;
  std::filesystem::create_directories(folder, ec);
  const std::string stem = std::string(prefix) + "_" + DiagTimestamp(when) + (label.empty() ? std::string() : "_" + label);
  std::filesystem::path path = folder / (stem + ".json");
  for (int n = 2; std::filesystem::exists(path, ec); ++n) path = folder / (stem + "-" + std::to_string(n) + ".json");
  return path;
}

// A new cache diagnostics file (a separate file per dump). Returns its path; the caller confirms the size.
inline std::filesystem::path WriteCacheDiagFile(const BvhDeviceData* data, uint32_t frame, const std::string& raw_label,
                                                const CacheSwitches& switches) {
  const std::vector<std::string> lines = CacheDiagLines(data);
  const CacheErrors errors = CollectCacheErrors(data);
  CacheDiagData d;
  {
    std::lock_guard<std::mutex> lock(g_cache_diag.mutex);
    d = g_cache_diag.data;
  }
  const std::string label = SanitizeDiagLabel(raw_label);
  const auto when = std::chrono::system_clock::now();
  const std::filesystem::path path = DiagFilePath("cache_diag", label, when);
  std::ostringstream out;
  out << "{\n  \"schema\": 4,\n  \"build\": \"" << PoolJsonEscape(kWorldBuildId) << "\",\n  \"frame\": " << frame << ",\n";
  out << "  \"label\": \"" << PoolJsonEscape(label) << "\",\n  \"label_raw\": \"" << PoolJsonEscape(raw_label) << "\",\n";
  out << "  \"timestamp\": \"" << DiagTimestamp(when) << "\",\n  \"presents\": " << d.presents << ",\n";
  out << "  \"tiers_valid\": " << (d.tiers_valid ? "true" : "false") << ", \"tiers_frame\": " << d.tiers_frame << ",\n";
  out << "  \"perf_gather\": " << (g_perf_gate.gather.load(std::memory_order_relaxed) ? "true" : "false") << ",\n";
  out << "  \"frame_ms\": {\"last\": " << PoolJsonFloat{d.frame_ms_last} << ", \"avg\": "
      << PoolJsonFloat{d.frame_intervals != 0u ? static_cast<float>(d.frame_ms_sum / static_cast<double>(d.frame_intervals)) : -1.f}
      << ", \"intervals\": " << d.frame_intervals << "},\n";
  out << "  \"switches\": ";
  WriteSwitchesJson(&out, switches);
  out << ",\n  \"lines\": [";
  for (size_t i = 0; i < lines.size(); ++i) out << (i != 0u ? ", " : "") << "\"" << PoolJsonEscape(lines[i]) << "\"";

  const auto uint_array = [&out](const uint64_t* values, size_t count) {
    out << "[";
    for (size_t i = 0; i < count; ++i) out << (i != 0u ? ", " : "") << values[i];
    out << "]";
  };
  const auto label_array = [&out](const char* const* labels, size_t count) {
    out << "[";
    for (size_t i = 0; i < count; ++i) out << (i != 0u ? ", " : "") << "\"" << labels[i] << "\"";
    out << "]";
  };
  const auto work_json = [&out](const CacheWork& w) {
    out << "{\"blas_ms\": " << PoolJsonFloat{w.blas_ms} << ", \"blas_builds\": " << w.blas_builds << ", \"tlas_ms\": " << PoolJsonFloat{w.tlas_ms}
        << ", \"tlas_rebuilds\": " << w.tlas_rebuilds << ", \"capture_ms\": " << PoolJsonFloat{w.capture_ms}
        << ", \"capture_meshes\": " << w.capture_meshes << ", \"capture_wait_sum\": " << w.capture_wait_sum << "}";
  };
  const PoolResightCounters& r = d.resight;
  out << "],\n  \"resight\": {\"retired\": " << r.retired << ", \"retired_retainable\": " << r.retired_retainable
      << ", \"retired_tris\": " << r.retired_tris << ", \"retired_dropped\": " << r.retired_dropped << ", \"expired\": " << r.expired
      << ", \"new_meshes\": " << r.new_meshes << ", \"new_tris\": " << r.new_tris << ", \"resights\": " << r.resights
      << ", \"resights_retainable\": " << r.resights_retainable << ", \"count_mismatch\": " << r.count_mismatch
      << ", \"revivable_tris\": " << r.revivable_tris << ", \"drift_near_misses\": " << r.drift_near_misses << ", \"tracked\": " << d.resight_tracked
      << ", \"gap\": [";
  for (size_t b = 0; b < kPoolResightGapBuckets; ++b) {
    out << (b != 0u ? ", " : "") << "{\"bucket\": \"" << kCacheGapLabels[b] << "\", \"count\": " << r.gap[b] << "}";
  }
  out << "]},\n  \"capture\": {\"cpu_ms\": " << PoolJsonFloat{d.work.capture_ms} << ", \"meshes\": " << r.capture_meshes
      << ", \"wait_sum\": " << r.capture_wait_sum << ", \"wait_max\": " << r.capture_wait_max << "},\n  \"extent_buckets\": ";
  label_array(kCacheExtentLabels, kCacheExtentBuckets);
  out << ",\n  \"ratio_buckets\": ";
  label_array(kCacheRatioLabels, kCacheRatioBuckets);
  out << ",\n  \"tiers\": [";
  for (size_t t = 0; t < kCacheTierCount; ++t) {
    const CacheTier& row = d.tiers.tiers[t];
    out << (t != 0u ? ", " : "") << "{\"tier\": \"" << PoolJsonEscape(kCacheTierNames[t]) << "\", \"instances\": " << row.instances
        << ", \"instance_tris\": " << row.instance_tris << ", \"meshes\": " << row.meshes << ", \"pool_tris\": " << row.pool_tris
        << ", \"pool_bytes\": " << row.pool_bytes << ", \"resident_meshes\": " << row.resident_meshes
        << ", \"resident_tris\": " << row.resident_tris << ", \"resident_bytes\": " << row.resident_bytes << ", \"extent\": ";
    uint_array(row.extent, kCacheExtentBuckets);
    out << ", \"ratio\": ";
    uint_array(row.ratio, kCacheRatioBuckets);
    out << ", \"ratio_tris\": ";
    uint_array(row.ratio_tris, kCacheRatioBuckets);
    out << ", \"hist_check\": " << (d.tiers.hist_check[t] ? "true" : "false") << "}";
  }
  out << "],\n  \"self_check\": " << (d.tiers.self_check ? "true" : "false") << ",\n  \"histogram\": [";
  for (size_t b = 0; b < kCacheHistogramBuckets; ++b) {
    out << (b != 0u ? ", " : "") << "{\"bucket\": \"" << kCacheHistogramLabels[b] << "\", \"count\": " << d.tiers.histogram[b] << "}";
  }
  out << "],\n  \"blas\": {";
  if (data != nullptr) {
    out << "\"builds\": " << data->live.blas_builds << ", \"triangles\": " << data->live.blas_built_triangles
        << ", \"ms_sum\": " << PoolJsonFloat{data->live.blas_build_ms_sum} << ", \"ms_max\": " << PoolJsonFloat{data->live.blas_build_ms_max};
  }
  const CacheTlasStats& t = d.tlas;
  const auto float_array = [&out](const float* values, size_t count) {
    out << "[";
    for (size_t i = 0; i < count; ++i) out << (i != 0u ? ", " : "") << PoolJsonFloat{values[i]};
    out << "]";
  };
  out << "},\n  \"tlas\": {\"rebuilds\": " << t.rebuilds << ", \"empty\": " << t.empty << ", \"added\": " << t.added
      << ", \"removed\": " << t.removed << ", \"moved\": " << t.moved << ", \"nearest_changed_m\": " << PoolJsonFloat{t.nearest_changed_m}
      << ", \"nearest_add_remove_m\": " << PoolJsonFloat{t.nearest_add_remove_m} << ", \"by_tier\": [" << t.by_tier[0] << ", "
      << t.by_tier[1] << ", " << t.by_tier[2] << ", " << t.by_tier[kCacheTlasTierNone] << "], \"ms_sum\": " << PoolJsonFloat{t.ms_sum}
      << ", \"ms_max\": " << PoolJsonFloat{t.ms_max} << ",\n    \"phase_ms\": {\"names\": [";
  for (size_t p = 0; p < kCachePhaseCount; ++p) out << (p != 0u ? ", " : "") << "\"" << kCachePhaseNames[p] << "\"";
  out << "], \"last\": ";
  float_array(t.phase_last, kCachePhaseCount);
  out << ", \"sum\": ";
  float_array(t.phase_sum, kCachePhaseCount);
  out << ", \"max\": ";
  float_array(t.phase_max, kCachePhaseCount);
  out << "},\n    \"noop\": {\"full\": " << t.noop_full << ", \"static\": " << t.noop_static << ", \"by_cause\": [" << t.noop_by_cause[0]
      << ", " << t.noop_by_cause[1] << ", " << t.noop_by_cause[2] << "], \"ms\": " << PoolJsonFloat{t.noop_ms} << ", \"skips\": " << t.skips << ", \"skip_ms\": " << PoolJsonFloat{t.skip_ms}
      << ", \"skips_by_cause\": [" << t.skips_by_cause[0] << ", " << t.skips_by_cause[1] << ", " << t.skips_by_cause[2]
      << "], \"applies\": " << t.applies << "},\n    \"reasons\": {";
  for (size_t r = 0; r <= kCacheReasonCount; ++r) {
    out << (r != 0u ? ", " : "") << "\"" << (r < kCacheReasonCount ? kCacheReasonNames[r] : "other") << "\": " << t.by_reason[r];
  }
  out << "}},\n";
  const CacheEpisode& e = d.episode;
  out << "  \"episode\": {\"state\": \"" << e.state << "\", \"reason\": \"" << e.reason << "\", \"serial\": " << e.serial
      << ", \"start_frame\": " << e.start_frame << ", \"fresh_from\": " << e.fresh_from << ", \"t80_valid\": " << (e.t80_valid ? "true" : "false")
      << ", \"t80_frames\": " << e.t80_frames << ", \"t80_ms\": " << PoolJsonFloat{e.t80_ms} << ", \"final_fresh\": " << e.final_fresh << ", \"work\": ";
  work_json(e.work);
  out << ", \"t80_work\": ";
  work_json(e.t80_work);
  out << ", \"samples\": [";
  for (size_t s = 0; s < e.samples.size(); ++s) {
    const CacheSample& sample = e.samples[s];
    out << (s != 0u ? ", " : "") << "{\"frame_offset\": " << (sample.frame - e.start_frame) << ", \"ms_offset\": " << PoolJsonFloat{sample.ms}
        << ", \"tlas_instances\": " << sample.tlas_instances << ", \"tlas_fresh\": " << sample.tlas_fresh
        << ", \"pool_admitted\": " << sample.pool_admitted << ", \"work\": ";
    work_json(sample.work);
    out << "}";
  }
  const CacheLighting& light = d.lighting;
  out << "]},\n  \"entry_latency\": {\"entered\": " << t.entered << ", \"buckets\": [";
  for (size_t b = 0; b < kCacheLatencyBuckets; ++b) out << (b != 0u ? ", " : "") << "\"" << kCacheLatencyLabels[b] << "\"";
  out << "], \"admit\": ";
  uint_array(t.admit_latency, kCacheLatencyBuckets);
  out << ", \"seen\": ";
  uint_array(t.seen_latency, kCacheLatencyBuckets);
  out << "},\n  \"lighting\": {\"presents\": " << light.presents << ", \"dark_presents\": " << light.dark_presents
      << ", \"dark_runs\": " << light.dark_runs << ", \"longest_dark\": " << light.longest_dark << ", \"longest_lit\": " << light.longest_lit
      << "}";
  const GpuTimer& trace = data->trace_timer;
  const LiveBvhStats& live = data->live;
  out << ",\n  \"slack\": {\"applies\": " << live.slack_applies << ", \"apply_ms_sum\": " << PoolJsonFloat{live.slack_apply_ms_sum}
      << ", \"apply_ms_max\": " << PoolJsonFloat{live.slack_apply_ms_max} << ", \"added\": " << live.slack_added
      << ", \"removed\": " << live.slack_removed << ", \"changed\": " << live.slack_changed << ", \"exhausted\": " << live.slack_exhausted
      << ", \"structural\": " << live.slack_structural << ", \"maintenance\": " << live.slack_maintenance
      << ", \"free_holes\": " << live.slack_free_holes << ", \"phase_ms\": [" << PoolJsonFloat{live.slack_phase_ms[0]} << ", "
      << PoolJsonFloat{live.slack_phase_ms[1]} << ", " << PoolJsonFloat{live.slack_phase_ms[2]} << ", " << PoolJsonFloat{live.slack_phase_ms[3]} << "]}";
  out << ",\n  \"gpu\": {\"trace\": {\"last_ms\": " << PoolJsonFloat{trace.last_ms} << ", \"avg_ms\": "
      << PoolJsonFloat{trace.samples != 0u ? static_cast<float>(trace.ms_sum / trace.samples) : -1.f} << ", \"max_ms\": " << PoolJsonFloat{trace.ms_max}
      << ", \"samples\": " << trace.samples << ", \"skipped\": " << trace.skipped << "}}";
  out << ",\n  \"gpu_check\": {\"checked\": " << (live.gpu_checked ? "true" : "false") << ", \"ok\": " << (live.gpu_ok ? "true" : "false")
      << ", \"frame\": " << live.gpu_check_frame << ", \"result\": \"" << PoolJsonEscape(live.gpu_result) << "\"}";
  out << ",\n  \"live\": {\"tlas_instances\": " << live.tlas_instances << ", \"tlas_rebuilds\": " << live.tlas_rebuilds
      << ", \"tlas_reason\": \"" << PoolJsonEscape(live.tlas_reason) << "\", \"tlas_frame\": " << live.tlas_frame
      << ", \"refit_lag_last\": " << live.refit_lag_last << ", \"refit_lag_max\": " << live.refit_lag_max
      << ", \"refit_rest_fast\": " << live.refit_rest_fast << ", \"refit_rest_slow\": " << live.refit_rest_slow
      << ", \"snapshot_reused\": " << live.snapshot_reused << ", \"snapshot_fresh\": " << live.snapshot_fresh
      << ", \"mesh_ms\": {\"last\": " << PoolJsonFloat{live.mesh_ms_last} << ", \"max\": " << PoolJsonFloat{live.mesh_ms_max}
      << "}, \"tlas_skips\": " << live.tlas_skips << ", \"skip_ms\": {\"sum\": " << PoolJsonFloat{live.tlas_skip_ms_sum}
      << ", \"max\": " << PoolJsonFloat{live.tlas_skip_ms_max} << "}}";
  out << ",\n  \"errors\": {\"total\": " << errors.total << ", \"meshes\": " << errors.meshes << ", \"readback\": {\"map_failures\": "
      << errors.readback.map_failures << ", \"staging_failures\": " << errors.readback.staging_failures << ", \"base_mismatch\": "
      << errors.readback.base_mismatch << "}, \"bvh\": {\"blas_invalid\": " << errors.bvh.blas_invalid << ", \"tlas_invalid\": " << errors.bvh.tlas_invalid
      << ", \"upload_failures\": " << errors.bvh.upload_failures << ", \"refit_failures\": " << errors.bvh.refit_failures
      << ", \"invalid_refs\": " << errors.bvh.invalid_refs << ", \"stack_overflow\": " << errors.bvh.stack_overflow
      << "}, \"deform\": {\"failed\": " << errors.deform.failed << ", \"resource_failures\": " << errors.deform.resource_failures
      << ", \"map_failures\": " << errors.deform.map_failures << "}}";
  out << ",\n  \"work\": ";
  work_json(d.work);
  out << "\n}\n";
  std::string text = out.str();
  renodx::utils::path::WriteTextFile(path, text);
  return path;

}

}  // namespace falcom_world::bvh
