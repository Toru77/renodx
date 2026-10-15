#pragma once

// Phase 0 placement probe (diagnostic only). A snapshot of the static pool instances near the camera is saved
// (in memory and as text), and a later snapshot is compared with it: exact matches, near matches (jitter),
// moved placements, a rebase by a common offset, or a plain mismatch. Read-only on the pool.
//
// Lock rule: g_placement.mutex is a leaf lock. The camera and the pool are read before any file I/O.

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <map>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "bvh_pool.hpp"

namespace falcom_world::bvh {

inline constexpr float kPlacementRadius = 96.f;             // Chebyshev m from the camera
inline constexpr size_t kPlacementCap = 6000u;              // records kept, nearest first
inline constexpr uint64_t kPlacementPairBudget = 4000000u;  // pair evaluations of one compare (worst case)
inline constexpr uint64_t kPlacementBudgetStages = 4u;      // pair-scanning stages after exact (near, rebase x2, moved)
inline constexpr size_t kPlacementMinComparable = 20u;
inline constexpr float kPlacementPassFraction = 0.99f;      // exact share of N for PASS (and near+exact share for WARN)
inline constexpr float kPlacementOnlyFraction = 0.01f;      // PASS: only-A and only-B each at most this share of their side
inline constexpr float kPlacementBucketMeters = 0.002f;     // rebase detector: delta buckets
inline constexpr float kPlacementDeltaTolerance = 1e-3f;    // rebase detector: per-axis window around d*
inline constexpr size_t kPlacementRebaseMinLeftover = 10u;  // P: leftover instances with an equal-basis partner
inline constexpr float kPlacementRebaseMinDelta = 1e-3f;    // |d*| must exceed this

struct PlacementRecord {
  uint64_t signature = 0u;    // WorldMesh::signature
  uint64_t matrix_hash = 0u;  // MatrixHash of the kPoolWorldFloats matrix
  uint32_t vs_hash = 0u;
  bool alpha = false;
  float matrix[kPoolWorldFloats] = {};
  float bounds[6] = {};  // bounds_min, bounds_max
  float distance = 0.f;  // Chebyshev m from the camera at the snapshot
};

struct PlacementSnapshot {
  float camera[3] = {};
  uint32_t frame = 0u;
  std::string build;
  bool truncated = false;
  float radius = kPlacementRadius;  // effective radius: the distance of the last kept record when truncated
  uint64_t mesh_queue = 0u;         // pool fill at the snapshot
  uint64_t mesh_in_flight = 0u;
  uint64_t admitted = 0u;
  std::string switches;
  std::vector<PlacementRecord> records;
};

// Read-only. Camera first (it takes g_state.mutex), then the pool under its lock.
inline void TakePlacementSnapshot(PlacementSnapshot* out) {
  *out = PlacementSnapshot{};
  out->build = kWorldBuildId;
  PoolCameraPosition(out->camera);
  out->frame = g_state.frame.load();
  std::vector<std::pair<float, size_t>> candidates;
  {
    std::lock_guard<std::mutex> lock(g_pool.mutex);
    for (size_t i = 0; i < g_pool.instances.size(); ++i) {
      const WorldInstance& instance = g_pool.instances[i];
      if (instance.dynamic || instance.mesh_id >= g_pool.meshes.size()) continue;
      const float distance = PoolChebyshevDistance(out->camera, instance.bounds_min, instance.bounds_max);
      if (distance <= kPlacementRadius) candidates.emplace_back(distance, i);
    }
    std::stable_sort(candidates.begin(), candidates.end(),
                     [](const std::pair<float, size_t>& a, const std::pair<float, size_t>& b) { return a.first < b.first; });
    if (candidates.size() > kPlacementCap) {
      out->truncated = true;
      candidates.resize(kPlacementCap);
      out->radius = candidates.back().first;
    }
    out->records.reserve(candidates.size());
    for (const auto& [distance, index] : candidates) {
      const WorldInstance& instance = g_pool.instances[index];
      const WorldMesh& mesh = g_pool.meshes[instance.mesh_id];
      PlacementRecord& record = out->records.emplace_back();
      record.signature = mesh.signature;
      record.matrix_hash = MatrixHash(instance.matrix, kPoolWorldFloats);
      record.vs_hash = instance.source_vs_hash;
      record.alpha = mesh.alpha;
      std::memcpy(record.matrix, instance.matrix, sizeof(record.matrix));
      std::memcpy(record.bounds, instance.bounds_min, sizeof(float) * 3u);
      std::memcpy(record.bounds + 3, instance.bounds_max, sizeof(float) * 3u);
      record.distance = distance;
    }
    out->mesh_queue = static_cast<uint64_t>(g_pool.stats.mesh_queue);
    out->mesh_in_flight = static_cast<uint64_t>(g_pool.stats.mesh_in_flight);
    out->admitted = g_pool.instances.size();
    std::ostringstream switches;
    switches << "alpha_foliage=" << g_pool.alpha_foliage.load() << " follow_moving=" << g_pool.follow_moving.load()
             << " dynamic_per_frame=" << g_pool.dynamic_per_frame.load() << " exclude_moving=" << g_pool.exclude_moving.load()
             << " legacy_scale=" << g_pool.legacy_scale.load() << " alpha_wind_opaque=" << g_pool.alpha_wind_opaque.load()
             << " alpha_wind_sway=" << g_pool.alpha_wind_sway.load();
    out->switches = switches.str();
  }
}

// Bit-exact text: floats and hashes as hex bits.
inline std::string PlacementHexBits(float value) {
  uint32_t bits = 0u;
  std::memcpy(&bits, &value, sizeof(bits));
  char text[9];
  std::snprintf(text, sizeof(text), "%08x", bits);
  return text;
}

inline bool PlacementParseBits(const std::string& token, uint32_t* out) {
  if (token.empty()) return false;
  char* end = nullptr;
  const unsigned long value = std::strtoul(token.c_str(), &end, 16);
  if (end == nullptr || *end != '\0' || value > UINT32_MAX) return false;
  *out = static_cast<uint32_t>(value);
  return true;
}

inline bool PlacementParseFloat(const std::string& token, float* out) {
  uint32_t bits = 0u;
  if (!PlacementParseBits(token, &bits)) return false;
  std::memcpy(out, &bits, sizeof(*out));
  return true;
}

inline bool PlacementParseHash64(const std::string& token, uint64_t* out) {
  if (token.empty()) return false;
  char* end = nullptr;
  const unsigned long long value = std::strtoull(token.c_str(), &end, 16);
  if (end == nullptr || *end != '\0') return false;
  *out = static_cast<uint64_t>(value);
  return true;
}

inline bool WritePlacementFile(const std::filesystem::path& path, const PlacementSnapshot& snapshot) {
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  if (!out) return false;
  out << "placement-snapshot 1\n";
  out << "build " << snapshot.build << "\n";
  out << "frame " << snapshot.frame << "\n";
  out << "camera " << PlacementHexBits(snapshot.camera[0]) << " " << PlacementHexBits(snapshot.camera[1]) << " "
      << PlacementHexBits(snapshot.camera[2]) << "\n";
  out << "truncated " << (snapshot.truncated ? 1 : 0) << "\n";
  out << "radius " << PlacementHexBits(snapshot.radius) << "\n";
  out << "pool " << snapshot.mesh_queue << " " << snapshot.mesh_in_flight << " " << snapshot.admitted << "\n";
  out << "switches " << snapshot.switches << "\n";
  out << "count " << snapshot.records.size() << "\n";
  for (const PlacementRecord& record : snapshot.records) {
    char head[64];
    std::snprintf(head, sizeof(head), "%016llx %016llx %08x", static_cast<unsigned long long>(record.signature),
                  static_cast<unsigned long long>(record.matrix_hash), record.vs_hash);
    out << "rec " << head << " " << (record.alpha ? 1 : 0) << " " << PlacementHexBits(record.distance);
    for (const float value : record.matrix) out << " " << PlacementHexBits(value);
    for (const float value : record.bounds) out << " " << PlacementHexBits(value);
    out << "\n";
  }
  out.flush();
  return static_cast<bool>(out);
}

// Reads what WritePlacementFile wrote. On a bad line `error` names the line number and the field.
inline bool ReadPlacementFile(const std::filesystem::path& path, PlacementSnapshot* out, std::string* error) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    *error = "cannot open " + path.string();
    return false;
  }
  PlacementSnapshot snapshot;
  std::string line;
  size_t number = 0u;
  size_t expected = 0u;
  bool counted = false;
  const auto fail = [&](const char* what) {
    *error = "line " + std::to_string(number) + ": " + what;
    return false;
  };
  while (std::getline(in, line)) {
    number += 1u;
    if (!line.empty() && line.back() == '\r') line.pop_back();
    std::istringstream fields(line);
    std::string key;
    fields >> key;
    if (number == 1u) {
      std::string version;
      fields >> version;
      if (key != "placement-snapshot" || version != "1") return fail("not a placement snapshot (version 1)");
    } else if (key == "build") {
      std::getline(fields, snapshot.build);
      if (!snapshot.build.empty() && snapshot.build.front() == ' ') snapshot.build.erase(0u, 1u);
    } else if (key == "frame") {
      if (!(fields >> snapshot.frame)) return fail("bad frame");
    } else if (key == "camera") {
      std::string x, y, z;
      if (!(fields >> x >> y >> z) || !PlacementParseFloat(x, &snapshot.camera[0]) || !PlacementParseFloat(y, &snapshot.camera[1])
          || !PlacementParseFloat(z, &snapshot.camera[2])) {
        return fail("bad camera");
      }
    } else if (key == "truncated") {
      int flag = 0;
      if (!(fields >> flag)) return fail("bad truncated flag");
      snapshot.truncated = flag != 0;
    } else if (key == "radius") {
      std::string token;
      if (!(fields >> token) || !PlacementParseFloat(token, &snapshot.radius)) return fail("bad radius");
    } else if (key == "pool") {
      if (!(fields >> snapshot.mesh_queue >> snapshot.mesh_in_flight >> snapshot.admitted)) return fail("bad pool fill");
    } else if (key == "switches") {
      std::getline(fields, snapshot.switches);
      if (!snapshot.switches.empty() && snapshot.switches.front() == ' ') snapshot.switches.erase(0u, 1u);
    } else if (key == "count") {
      if (!(fields >> expected)) return fail("bad count");
      counted = true;
    } else if (key == "rec") {
      PlacementRecord record;
      std::string signature, hash, vs, alpha, distance;
      if (!(fields >> signature >> hash >> vs >> alpha >> distance) || !PlacementParseHash64(signature, &record.signature)
          || !PlacementParseHash64(hash, &record.matrix_hash) || !PlacementParseBits(vs, &record.vs_hash)
          || (alpha != "0" && alpha != "1") || !PlacementParseFloat(distance, &record.distance)) {
        return fail("bad record head");
      }
      record.alpha = alpha == "1";
      for (float& value : record.matrix) {
        std::string token;
        if (!(fields >> token) || !PlacementParseFloat(token, &value)) return fail("bad record matrix");
      }
      for (float& value : record.bounds) {
        std::string token;
        if (!(fields >> token) || !PlacementParseFloat(token, &value)) return fail("bad record bounds");
      }
      snapshot.records.push_back(record);
    } else {
      return fail("unknown line");
    }
  }
  if (!counted || snapshot.records.size() != expected) {
    *error = "record count " + std::to_string(snapshot.records.size()) + " does not match the count line";
    return false;
  }
  *out = std::move(snapshot);
  return true;
}

struct PlacementCompare {
  std::string verdict;             // the verdict line (also the last line of lines)
  std::vector<std::string> lines;  // the report
};

inline float PlacementTranslationGap(const PlacementRecord& a, const PlacementRecord& b) {
  const float dx = a.matrix[3] - b.matrix[3];
  const float dy = a.matrix[7] - b.matrix[7];
  const float dz = a.matrix[11] - b.matrix[11];
  return std::sqrt(dx * dx + dy * dy + dz * dz);
}

// Largest difference of the basis elements (the translation column excluded).
inline float PlacementBasisGap(const PlacementRecord& a, const PlacementRecord& b) {
  float gap = 0.f;
  for (uint32_t i = 0; i < kPoolWorldFloats; ++i) {
    if (i == 3u || i == 7u || i == 11u) continue;
    gap = (std::max)(gap, std::fabs(a.matrix[i] - b.matrix[i]));
  }
  return gap;
}

// Pure. Exact: same signature and hash, and the 12 floats bitwise equal (the hash alone is not trusted).
// Near: same signature, nearest translation, not moved (MeasurePoolMotion). Moved: the rest, nearest translation.
// Rebase: leftovers with an equal basis whose translation differences share one offset d* (2 mm buckets).
// A record counts only when it lies within the common radius min(radiusA, radiusB) of BOTH cameras, each distance
// measured from that snapshot's own camera, so a moved camera does not create mismatches.
inline PlacementCompare ComparePlacementSnapshots(const PlacementSnapshot& a, const PlacementSnapshot& b) {
  PlacementCompare out;
  std::vector<const PlacementRecord*> in_a;
  std::vector<const PlacementRecord*> in_b;
  size_t alpha_a = 0u;
  size_t alpha_b = 0u;
  size_t outside_a = 0u;
  size_t outside_b = 0u;
  const float common_radius = (std::min)(a.radius, b.radius);
  const auto within_both = [&](const PlacementRecord& record) {
    return PoolChebyshevDistance(a.camera, record.bounds, record.bounds + 3) <= common_radius
           && PoolChebyshevDistance(b.camera, record.bounds, record.bounds + 3) <= common_radius;
  };
  for (const PlacementRecord& record : a.records) {
    if (record.alpha) {
      alpha_a += 1u;
    } else if (!within_both(record)) {
      outside_a += 1u;
    } else {
      in_a.push_back(&record);
    }
  }
  for (const PlacementRecord& record : b.records) {
    if (record.alpha) {
      alpha_b += 1u;
    } else if (!within_both(record)) {
      outside_b += 1u;
    } else {
      in_b.push_back(&record);
    }
  }
  const size_t ca = in_a.size();
  const size_t cb = in_b.size();
  const size_t n = (std::min)(ca, cb);

  // Exact: multiset of (signature, hash) with the bitwise guard.
  std::map<std::pair<uint64_t, uint64_t>, std::vector<size_t>> by_key;
  for (size_t j = 0u; j < cb; ++j) by_key[{in_b[j]->signature, in_b[j]->matrix_hash}].push_back(j);
  std::vector<bool> used_a(ca, false);
  std::vector<bool> used_b(cb, false);
  size_t exact = 0u;
  for (size_t i = 0u; i < ca; ++i) {
    const auto found = by_key.find({in_a[i]->signature, in_a[i]->matrix_hash});
    if (found == by_key.end()) continue;
    for (const size_t j : found->second) {
      if (used_b[j] || std::memcmp(in_a[i]->matrix, in_b[j]->matrix, sizeof(in_a[i]->matrix)) != 0) continue;
      used_a[i] = true;
      used_b[j] = true;
      exact += 1u;
      break;
    }
  }

  // Leftovers by signature, and the worst-case pair evaluations of the stages after exact.
  std::map<uint64_t, std::vector<size_t>> left_a;
  std::map<uint64_t, std::vector<size_t>> left_b;
  for (size_t i = 0u; i < ca; ++i) {
    if (!used_a[i]) left_a[in_a[i]->signature].push_back(i);
  }
  for (size_t j = 0u; j < cb; ++j) {
    if (!used_b[j]) left_b[in_b[j]->signature].push_back(j);
  }
  uint64_t pairs = 0u;
  for (const auto& [signature, list] : left_a) {
    const auto other = left_b.find(signature);
    if (other != left_b.end()) pairs += static_cast<uint64_t>(list.size()) * other->second.size();
  }
  const bool over_budget = kPlacementBudgetStages * pairs > kPlacementPairBudget;
  // The rebase detector runs only when the exact share is below PASS: a lattice that is 99% exact is never a rebase.
  const bool rebase_due = static_cast<float>(exact) < kPlacementPassFraction * static_cast<float>(n);

  size_t near_matches = 0u;
  size_t moved = 0u;
  std::vector<float> moved_gaps;
  uint64_t rebase_pairs = 0u;
  size_t rebase_p = 0u;
  size_t rebase_s = 0u;
  float delta_star[3] = {};
  bool rebase_fires = false;
  if (!over_budget) {
    // Near: nearest unused partner of the same signature that does not count as moved.
    for (auto& [signature, list] : left_a) {
      const auto other = left_b.find(signature);
      if (other == left_b.end()) continue;
      for (const size_t i : list) {
        float best = 0.f;
        size_t best_j = SIZE_MAX;
        for (const size_t j : other->second) {
          if (used_b[j]) continue;
          const float gap = PlacementTranslationGap(*in_a[i], *in_b[j]);
          if (best_j == SIZE_MAX || gap < best) {
            best = gap;
            best_j = j;
          }
        }
        if (best_j == SIZE_MAX) continue;
        if (MeasurePoolMotion(in_a[i]->matrix, in_b[best_j]->matrix).moved) continue;
        used_a[i] = true;
        used_b[best_j] = true;
        near_matches += 1u;
      }
    }

    // Rebase: leftovers with an equal basis. Delta = tB - tA, bucketed; d* = mean of the top bucket.
    struct Bucket {
      size_t count = 0u;
      float sum[3] = {};
    };
    std::map<std::array<int64_t, 3>, Bucket> buckets;
    for (const auto& [signature, list] : left_a) {
      const auto other = left_b.find(signature);
      if (other == left_b.end()) continue;
      for (const size_t i : list) {
        if (used_a[i]) continue;
        for (const size_t j : other->second) {
          if (used_b[j]) continue;
          rebase_pairs += 1u;
          if (PlacementBasisGap(*in_a[i], *in_b[j]) > kPoolMotionMovedBasis) continue;
          std::array<int64_t, 3> key{};
          float delta[3] = {};
          for (int k = 0; k < 3; ++k) {
            delta[k] = in_b[j]->matrix[3 + 4 * k] - in_a[i]->matrix[3 + 4 * k];
            key[k] = static_cast<int64_t>(std::llround(delta[k] / kPlacementBucketMeters));
          }
          Bucket& bucket = buckets[key];
          bucket.count += 1u;
          for (int k = 0; k < 3; ++k) bucket.sum[k] += delta[k];
        }
      }
    }
    const Bucket* top = nullptr;
    for (const auto& [key, bucket] : buckets) {
      if (top == nullptr || bucket.count > top->count) top = &bucket;
    }
    if (top != nullptr) {
      for (int k = 0; k < 3; ++k) delta_star[k] = top->sum[k] / static_cast<float>(top->count);
      for (const auto& [signature, list] : left_a) {
        const auto other = left_b.find(signature);
        if (other == left_b.end()) continue;
        for (const size_t i : list) {
          if (used_a[i]) continue;
          bool any = false;
          bool star = false;
          for (const size_t j : other->second) {
            if (used_b[j] || PlacementBasisGap(*in_a[i], *in_b[j]) > kPoolMotionMovedBasis) continue;
            any = true;
            bool near_star = true;
            for (int k = 0; k < 3; ++k) {
              const float delta = in_b[j]->matrix[3 + 4 * k] - in_a[i]->matrix[3 + 4 * k];
              if (std::fabs(delta - delta_star[k]) > kPlacementDeltaTolerance) near_star = false;
            }
            star = star || near_star;
          }
          if (any) rebase_p += 1u;
          if (star) rebase_s += 1u;
        }
      }
      const float length = std::sqrt(delta_star[0] * delta_star[0] + delta_star[1] * delta_star[1] + delta_star[2] * delta_star[2]);
      rebase_fires = rebase_due && rebase_p >= kPlacementRebaseMinLeftover && rebase_s * 2u >= rebase_p && length > kPlacementRebaseMinDelta;
    }

    // Moved: the rest, nearest unused partner of the same signature.
    for (auto& [signature, list] : left_a) {
      const auto other = left_b.find(signature);
      for (const size_t i : list) {
        if (used_a[i]) continue;
        size_t best_j = SIZE_MAX;
        float best = 0.f;
        if (other != left_b.end()) {
          for (const size_t j : other->second) {
            if (used_b[j]) continue;
            const float gap = PlacementTranslationGap(*in_a[i], *in_b[j]);
            if (best_j == SIZE_MAX || gap < best) {
              best = gap;
              best_j = j;
            }
          }
        }
        if (best_j == SIZE_MAX) continue;
        used_a[i] = true;
        used_b[best_j] = true;
        moved += 1u;
        moved_gaps.push_back(best);
      }
    }
  }
  size_t only_a = 0u;
  size_t only_b = 0u;
  for (size_t i = 0u; i < ca; ++i) {
    if (!used_a[i]) only_a += 1u;
  }
  for (size_t j = 0u; j < cb; ++j) {
    if (!used_b[j]) only_b += 1u;
  }

  float median = 0.f;
  float max_gap = 0.f;
  if (!moved_gaps.empty()) {
    std::sort(moved_gaps.begin(), moved_gaps.end());
    const size_t mid = moved_gaps.size() / 2u;
    median = moved_gaps.size() % 2u != 0u ? moved_gaps[mid] : 0.5f * (moved_gaps[mid - 1u] + moved_gaps[mid]);
    max_gap = moved_gaps.back();
  }

  const auto percent = [](size_t part, size_t whole) {
    std::ostringstream s;
    s << std::fixed << std::setprecision(1) << (whole == 0u ? 0.0 : 100.0 * static_cast<double>(part) / static_cast<double>(whole)) << "%";
    return s.str();
  };
  const bool exact_pass = static_cast<float>(exact) >= kPlacementPassFraction * static_cast<float>(n);
  const bool only_small = static_cast<float>(only_a) <= kPlacementOnlyFraction * static_cast<float>(ca)
                          && static_cast<float>(only_b) <= kPlacementOnlyFraction * static_cast<float>(cb);
  const std::string only = "only A " + percent(only_a, ca) + ", only B " + percent(only_b, cb);
  std::ostringstream verdict;
  if (over_budget) {
    verdict << "INCONCLUSIVE (budget)";
  } else if (n < kPlacementMinComparable) {
    verdict << "INCONCLUSIVE " << n << " comparable instances (need " << kPlacementMinComparable << ")";
  } else if (rebase_fires) {
    verdict << "FAIL rebase-by-delta (" << delta_star[0] << ", " << delta_star[1] << ", " << delta_star[2] << ") m";
  } else if (exact_pass && only_small) {
    verdict << "PASS exact-match " << percent(exact, n) << " (" << exact << " of " << n << "), " << only;
  } else if (static_cast<float>(exact + near_matches) >= kPlacementPassFraction * static_cast<float>(n)) {
    verdict << "WARN not PASS: exact " << percent(exact, n) << " (" << exact << " of " << n << "), near " << near_matches << ", "
            << only;
  } else {
    verdict << "FAIL mismatch: exact " << percent(exact, n) << " (" << exact << " of " << n << "), moved " << moved
            << " (median " << median << " m, max " << max_gap << " m), only A " << percent(only_a, ca) << ", only B "
            << percent(only_b, cb);
  }
  if (b.mesh_queue + b.mesh_in_flight != 0u) {
    verdict << ". B was taken while " << (b.mesh_queue + b.mesh_in_flight) << " meshes were queued, retry after settle";
  }
  out.verdict = verdict.str();

  std::ostringstream text;
  text << "Placement compare: A f" << a.frame << " / B f" << b.frame << ", radius A " << a.radius << " m, B " << b.radius
       << " m (truncated A " << (a.truncated ? 1 : 0) << ", B " << (b.truncated ? 1 : 0) << "), camera A to B "
       << PoolChebyshevDistance(a.camera, b.camera, b.camera) << " m";
  out.lines.push_back(text.str());
  text.str("");
  text << "Inside both radii: A " << ca << ", B " << cb << " (N " << n << "); alpha excluded A " << alpha_a << ", B " << alpha_b
       << "; outside other radius, expected: A " << outside_a << ", B " << outside_b;
  out.lines.push_back(text.str());
  text.str("");
  text << "Exact " << exact << ", near " << near_matches << ", moved " << moved << " (median " << median << " m, max " << max_gap
       << " m), only A " << only_a << ", only B " << only_b << "; pair budget " << (over_budget ? "exceeded" : "ok");
  out.lines.push_back(text.str());
  text.str("");
  text << "Rebase detector: equal-basis leftover pairs " << rebase_pairs << ", P " << rebase_p << ", S " << rebase_s << ", d* ("
       << delta_star[0] << ", " << delta_star[1] << ", " << delta_star[2] << ") m, "
       << (rebase_due ? (rebase_fires ? "fired" : "not fired") : "skipped (exact share at PASS)");
  out.lines.push_back(text.str());
  out.lines.push_back("Verdict: " + out.verdict);
  return out;
}

// In memory: snapshot A and the last report lines (shown in the panel). Leaf lock.
struct PlacementState {
  std::mutex mutex;
  PlacementSnapshot a;
  bool a_valid = false;
  std::vector<std::string> lines;
};

inline PlacementState g_placement;

}  // namespace falcom_world::bvh
