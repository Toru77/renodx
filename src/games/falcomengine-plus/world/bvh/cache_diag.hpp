#pragma once

// Phase 0 cache diagnostics (diagnostic only: no rendering or TLAS behaviour change).
//
// Where the pool's instances sit (distance tiers from the camera), what the pool
// and the resident GPU store hold per tier, which TLAS rebuilds follow a change of
// the instance set, the BLAS build cost, the lighting gap, and how long a map
// change or a pool reset takes to refill the TLAS with fresh instances (an
// episode). Every result is shown in the panel, copied to the clipboard and
// written to world_cache_diag.json.
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
#include <filesystem>
#include <limits>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "bvh_pool.hpp"
#include "bvh_resources.hpp"

namespace falcom_world::bvh {

// Chebyshev distance from the camera: T0 < 128 m, T1 < 256 m, T2 >= 256 m (inside the region).
inline constexpr float kCacheTierEdges[2] = {128.f, 256.f};
inline constexpr float kCacheHistogramEdges[9] = {16.f, 32.f, 64.f, 96.f, 128.f, 192.f, 256.f, 384.f, 512.f};
inline constexpr size_t kCacheHistogramBuckets = 10u;  // the last bucket is open (>= 512 m)
inline constexpr const char* kCacheHistogramLabels[kCacheHistogramBuckets] = {
    "<16", "16-32", "32-64", "64-96", "96-128", "128-192", "192-256", "256-384", "384-512", ">=512"};
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

// Tier of an instance by its Chebyshev distance to the camera.
inline size_t CacheInstanceTier(float distance, bool in_region) {
  if (!in_region) return kCacheTierOutside;
  if (distance < kCacheTierEdges[0]) return 0u;
  return distance < kCacheTierEdges[1] ? 1u : 2u;
}

inline size_t CacheHistogramBucket(float distance) {
  size_t bucket = 0u;
  while (bucket + 1u < kCacheHistogramBuckets && distance >= kCacheHistogramEdges[bucket]) ++bucket;
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
    out->histogram[CacheHistogramBucket(distance)] += 1u;
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
}

// What one TLAS rebuild changed, by instance id. Wind instances are left out (their pose is swayed every present).
struct CacheTlasDiff {
  uint64_t added = 0u;
  uint64_t removed = 0u;
  uint64_t moved = 0u;               // same id, another matrix
  float nearest_changed = -1.f;      // Chebyshev m from the camera; -1 = none
  float nearest_add_remove = -1.f;
};

// Pure: `before` and `after` are TLAS snapshots (any order).
inline CacheTlasDiff DiffTlasInstances(const std::vector<LiveTlasInstance>& before,
                                       const std::vector<LiveTlasInstance>& after, const float* camera) {
  std::vector<const LiveTlasInstance*> old_list;
  std::vector<const LiveTlasInstance*> new_list;
  for (const LiveTlasInstance& instance : before) {
    if (!instance.wind) old_list.push_back(&instance);
  }
  for (const LiveTlasInstance& instance : after) {
    if (!instance.wind) new_list.push_back(&instance);
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

struct CacheTlasStats {
  uint64_t rebuilds = 0u;  // rebuilds diffed (a TLAS that is not empty)
  uint64_t empty = 0u;     // rebuilds that left the TLAS empty (not diffed)
  uint64_t added = 0u;
  uint64_t removed = 0u;
  uint64_t moved = 0u;
  float nearest_changed_m = -1.f;     // over the session, -1 = none
  float nearest_add_remove_m = -1.f;
  uint64_t by_tier[kCacheTlasTierCount] = {};  // rebuilds by tier of the nearest add or remove
};

struct CacheSample {
  uint32_t frame = 0u;
  float ms = 0.f;              // since the episode start
  uint64_t tlas_instances = 0u;
  uint64_t tlas_fresh = 0u;    // TLAS instances last seen at or after the episode's fresh frame
  uint64_t pool_admitted = 0u;
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
  std::string last_path;
  uint64_t last_bytes = 0u;
  std::string last_result;  // the last panel action's result line
  bool dump_armed = false;  // Dump was pressed once: the next press writes the file (after a present computed the tiers)
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
  g_cache_diag.data = std::move(fresh);
}

// Caller holds no lock. `before` is the TLAS this build replaces (moved out before the replacement);
// `after` is the TLAS in place now. `replaced` is false for a failed build, which counts nothing.
inline void RecordCacheTlasRebuild(const std::vector<LiveTlasInstance>& before, const std::vector<LiveTlasInstance>& after,
                                   bool replaced, uint32_t frame) {
  if (!replaced || !g_cache_diag.enabled.load(std::memory_order_relaxed)) return;
  float camera[3] = {};
  PoolCameraPosition(camera);
  const CacheTlasDiff diff = after.empty() ? CacheTlasDiff{} : DiffTlasInstances(before, after, camera);
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
  if (frame < d.episode.start_frame || d.episode.samples.size() >= kCacheMaxSamples) return;
  CacheSample& sample = d.episode.samples.emplace_back();
  sample.frame = frame;
  sample.ms = std::chrono::duration<float, std::milli>(std::chrono::steady_clock::now() - d.episode.start_time).count();
  sample.tlas_instances = after.size();
  sample.tlas_fresh = fresh;
  sample.pool_admitted = admitted;
}

template <typename... Args>
inline std::string CacheFormat(const char* format, Args... args) {
  const int size = std::snprintf(nullptr, 0, format, args...);
  std::string text(static_cast<size_t>(size > 0 ? size : 0), '\0');
  if (size > 0) std::snprintf(text.data(), text.size() + 1u, format, args...);
  return text;
}

// The report: panel lines, clipboard text and the "lines" key of world_cache_diag.json.
inline std::vector<std::string> CacheDiagLines(const BvhDeviceData* data) {
  CacheDiagData d;
  {
    std::lock_guard<std::mutex> lock(g_cache_diag.mutex);
    d = g_cache_diag.data;
  }
  std::vector<std::string> lines;
  lines.push_back(CacheFormat("Cache diagnostics (%s): %llu presents, tiers %s", kWorldBuildId, d.presents,
                              d.tiers_valid ? CacheFormat("at f%u (every %u presents or on dump)", d.tiers_frame, kCacheSampleInterval).c_str()
                                            : "not computed yet"));
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

// world_cache_diag.json in the pool folder. Returns its path; the caller confirms the size.
inline std::filesystem::path WriteCacheDiagFile(const BvhDeviceData* data, uint32_t frame) {
  const std::vector<std::string> lines = CacheDiagLines(data);
  CacheDiagData d;
  {
    std::lock_guard<std::mutex> lock(g_cache_diag.mutex);
    d = g_cache_diag.data;
  }
  std::ostringstream out;
  out << "{\n  \"schema\": 1,\n  \"build\": \"" << PoolJsonEscape(kWorldBuildId) << "\",\n  \"frame\": " << frame << ",\n";
  out << "  \"lines\": [";
  for (size_t i = 0; i < lines.size(); ++i) out << (i != 0u ? ", " : "") << "\"" << PoolJsonEscape(lines[i]) << "\"";
  out << "],\n  \"tiers\": [";
  for (size_t t = 0; t < kCacheTierCount; ++t) {
    const CacheTier& row = d.tiers.tiers[t];
    out << (t != 0u ? ", " : "") << "{\"tier\": \"" << PoolJsonEscape(kCacheTierNames[t]) << "\", \"instances\": " << row.instances
        << ", \"instance_tris\": " << row.instance_tris << ", \"meshes\": " << row.meshes << ", \"pool_tris\": " << row.pool_tris
        << ", \"pool_bytes\": " << row.pool_bytes << ", \"resident_meshes\": " << row.resident_meshes
        << ", \"resident_tris\": " << row.resident_tris << ", \"resident_bytes\": " << row.resident_bytes << "}";
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
  out << "},\n  \"tlas\": {\"rebuilds\": " << t.rebuilds << ", \"empty\": " << t.empty << ", \"added\": " << t.added
      << ", \"removed\": " << t.removed << ", \"moved\": " << t.moved << ", \"nearest_changed_m\": " << PoolJsonFloat{t.nearest_changed_m}
      << ", \"nearest_add_remove_m\": " << PoolJsonFloat{t.nearest_add_remove_m} << ", \"by_tier\": [" << t.by_tier[0] << ", "
      << t.by_tier[1] << ", " << t.by_tier[2] << ", " << t.by_tier[kCacheTlasTierNone] << "]},\n";
  const CacheEpisode& e = d.episode;
  out << "  \"episode\": {\"state\": \"" << e.state << "\", \"reason\": \"" << e.reason << "\", \"serial\": " << e.serial
      << ", \"start_frame\": " << e.start_frame << ", \"fresh_from\": " << e.fresh_from << ", \"t80_valid\": " << (e.t80_valid ? "true" : "false")
      << ", \"t80_frames\": " << e.t80_frames << ", \"t80_ms\": " << PoolJsonFloat{e.t80_ms} << ", \"final_fresh\": " << e.final_fresh
      << ", \"samples\": [";
  for (size_t s = 0; s < e.samples.size(); ++s) {
    const CacheSample& sample = e.samples[s];
    out << (s != 0u ? ", " : "") << "{\"frame_offset\": " << (sample.frame - e.start_frame) << ", \"ms_offset\": " << PoolJsonFloat{sample.ms}
        << ", \"tlas_instances\": " << sample.tlas_instances << ", \"tlas_fresh\": " << sample.tlas_fresh
        << ", \"pool_admitted\": " << sample.pool_admitted << "}";
  }
  const CacheLighting& light = d.lighting;
  out << "]},\n  \"lighting\": {\"presents\": " << light.presents << ", \"dark_presents\": " << light.dark_presents
      << ", \"dark_runs\": " << light.dark_runs << ", \"longest_dark\": " << light.longest_dark << ", \"longest_lit\": " << light.longest_lit
      << "}\n}\n";
  const std::filesystem::path path = PoolOutputDir() / "world_cache_diag.json";
  std::string text = out.str();
  std::error_code ec;
  std::filesystem::remove(path, ec);  // a failed write must not leave the previous file behind
  renodx::utils::path::WriteTextFile(path, text);
  return path;
}

}  // namespace falcom_world::bvh
