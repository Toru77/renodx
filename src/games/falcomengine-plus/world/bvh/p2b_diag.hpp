#pragma once

// P2b diagnostics ("Diagnostics (all)" in the BVH panel). The button only arms; the first present after
// the expansion has a flagged draw and a trace stats readback (or 120 presents later) writes
// world_p2b_diag.json and world_p2b_summary.txt, then the pool and deform dumps and the manifest.
// The analysis is BuildP2bDiagJson, a pure function over P2bDiagInput, so the harness checks its verdicts
// on synthetic inputs. The runner takes the snapshot under g_pool.mutex (CPU only), releases it, then does
// the readbacks and the one probe copy (GPU work, never under a module mutex).

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <mutex>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "../../../../utils/log.hpp"
#include "../../../../utils/path.hpp"
#include "../../../../utils/scene.hpp"
#include "../world_state.hpp"
#include "bvh_live.hpp"
#include "bvh_trace.hpp"
#include "deform_live.hpp"

namespace falcom_world::bvh {

inline constexpr uint32_t kP2bDiagWaitFrames = 120u;
inline constexpr uint64_t kP2bDiagArenaReadBytes = 64u * 1024u;
inline constexpr uint32_t kP2bDiagRecordsRead = 64u;
inline constexpr uint32_t kP2bDiagTableRead = 16u;
inline constexpr uint64_t kP2bDiagProbeBytes = 2u * kPoolInstanceStride;  // two 160-byte slices

struct P2bDiagState {
  std::atomic_bool requested{false};
  std::atomic<uint32_t> armed_frame{0u};
  std::atomic<uint32_t> written_frame{0u};
  std::atomic<uint32_t> state{0u};  // 0 idle, 1 armed, 2 written, 3 failed
  std::mutex mutex;
  std::string verdict_hint;         // guarded by mutex
};

inline P2bDiagState g_p2b_diag;

// The P2b counters of the pool and the trace, as the diagnostics read them.
struct P2bDiagCounters {
  bool ready = false;
  uint64_t expand_presents = 0u;
  uint64_t expand_empty = 0u;
  uint64_t expand_failed = 0u;
  uint32_t instance_count = 0u;
  uint32_t trace_count = 0u;  // records bound to the last trace dispatch
  uint32_t trace_frame = 0u;
  uint32_t trace_stats_frame = 0u;  // g_state.frame at the stats readback that filled trace_stats
  uint32_t tlas_per_frame = 0u;
  uint32_t tlas_partition = 0u;
  uint32_t tlas_skipped = 0u;
  uint64_t flagged_total = 0u;
  uint64_t flagged_resident = 0u;
  bool trace_valid = false;
  uint32_t p2b_tested = 0u;
  uint32_t p2b_skipped = 0u;
  uint32_t p2b_box_hits = 0u;
  uint32_t p2b_wins = 0u;
};

struct P2bDiagPoolPoint {
  uint64_t id = 0u;
  float position[3] = {};
};

struct P2bDiagFlagged {
  uint64_t uid = 0u;
  uint32_t slot = 0u;
  bool resident = false;
  uint32_t pool_instances = 0u;
};

// Everything BuildP2bDiagJson reads. Arena, records and table are the readbacks (empty when not read).
struct P2bDiagInput {
  std::string build;
  uint32_t generated_frame = 0u;
  uint32_t armed_frame = 0u;
  std::vector<std::pair<const char*, bool>> switches;
  P2bDiagCounters counters;
  std::vector<PoolDynamicDraw> draws;      // the last present's draws (CPU copies)
  std::vector<DynamicDrawGPU> table;       // the draw table uploaded for the last present
  bool table_readback_ok = false;
  bool table_matches = false;              // the first table entries read back equal table
  bool readback_ok = false;
  std::string readback_reason;
  std::string arena_reason = "ok";   // "ok" or why the arena was not read this present
  std::string records_reason = "ok";
  std::string table_reason = "ok";
  bool fresh_trace = false;          // the trace stats come from a dispatch at or after the armed frame
  bool timed_out = false;            // kP2bDiagWaitFrames presents passed without a fresh trace
  std::vector<float> arena;                // arena floats as read (float4 index = element * 4 floats)
  std::vector<WorldInstanceGPU> records;   // the first records read back
  std::vector<DynamicDrawGPU> table_readback;
  bool probe_ran = false;
  bool probe_tail_read = false;   // the arena-tail readback succeeded
  bool probe_plain_read = false;  // the raw control buffer (same stride-0 layout as the arena) readback succeeded
  bool probe_arena_tail_landed = false;
  bool probe_plain_landed = false;
  std::string probe_reason;
  std::vector<P2bDiagPoolPoint> pool;
  std::vector<P2bDiagFlagged> flagged;
  bool ray_valid = false;
  float ray_origin[3] = {};
  float ray_direction[3] = {};
};

struct P2bDiagSummary {
  uint32_t records_valid = 0u;
  uint32_t records_invalid = 0u;
  uint32_t arena_elements_checked = 0u;
  uint32_t arena_elements_zero = 0u;
  uint32_t draws_with_zero_arena = 0u;
  uint32_t gpu_cpu_validity_mismatch = 0u;
  float nearest_pool_dist_median = 0.f;
};

// The analysis of one snapshot (the verdict table in the plan). Returns the JSON; verdict_hint and
// summary_lines (ASCII, for world_p2b_summary.txt and the log) are optional.
inline std::string BuildP2bDiagJson(const P2bDiagInput& in, std::string* verdict_hint,
                                    std::vector<std::string>* summary_lines, P2bDiagSummary* summary_out = nullptr) {
  constexpr size_t kElementCap = 64u;
  constexpr size_t kRecordCap = 32u;
  const float ray_origin[3] = {in.ray_origin[0], in.ray_origin[1], in.ray_origin[2]};
  const float ray_direction[3] = {in.ray_direction[0], in.ray_direction[1], in.ray_direction[2]};
  const size_t arena_floats = in.arena.size();

  struct Element {
    uint32_t draw = 0u;
    uint32_t index = 0u;
    uint32_t arena_f4 = 0u;
    bool readable = false;
    float world[12] = {};
    bool zero = false;
    bool finite = false;
    PoolMatrixReject reject = PoolMatrixReject::None;
    uint64_t nearest_id = 0u;
    float nearest_dist = 0.f;
  };
  std::vector<Element> elements;
  for (size_t d = 0u; d < in.table.size() && elements.size() < kElementCap; ++d) {
    const DynamicDrawGPU& draw = in.table[d];
    for (uint32_t k = 0u; k < draw.info[1] && elements.size() < kElementCap; ++k) {
      Element e;
      e.draw = static_cast<uint32_t>(d);
      e.index = k;
      e.arena_f4 = draw.info[0] + k * (kPoolInstanceStride / 16u);
      const size_t base = static_cast<size_t>(e.arena_f4) * 4u;
      e.readable = base + 12u <= arena_floats;
      if (e.readable) {
        std::memcpy(e.world, in.arena.data() + base, sizeof(e.world));
        e.zero = true;
        e.finite = true;
        for (const float value : e.world) {
          e.zero = e.zero && value == 0.f;
          e.finite = e.finite && std::isfinite(value);
        }
        e.reject = e.finite ? CheckPoolWorld(e.world) : PoolMatrixReject::NonFinite;
        float best = std::numeric_limits<float>::max();
        for (const P2bDiagPoolPoint& point : in.pool) {
          const float dx = e.world[3] - point.position[0];
          const float dy = e.world[7] - point.position[1];
          const float dz = e.world[11] - point.position[2];
          const float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
          if (dist < best) {
            best = dist;
            e.nearest_id = point.id;
          }
        }
        e.nearest_dist = best < std::numeric_limits<float>::max() ? best : 0.f;
      }
      elements.push_back(e);
    }
  }

  P2bDiagSummary s;
  std::vector<float> nearest;
  for (const Element& e : elements) {
    if (!e.readable) continue;
    s.arena_elements_checked += 1u;
    s.arena_elements_zero += e.zero ? 1u : 0u;
    if (e.finite && !in.pool.empty()) nearest.push_back(e.nearest_dist);
  }
  for (size_t d = 0u; d < in.table.size(); ++d) {
    bool any = false;
    bool all_zero = true;
    for (const Element& e : elements) {
      if (e.draw != d || !e.readable) continue;
      any = true;
      all_zero = all_zero && e.zero;
    }
    s.draws_with_zero_arena += (any && all_zero) ? 1u : 0u;
  }
  if (!nearest.empty()) {
    std::nth_element(nearest.begin(), nearest.begin() + nearest.size() / 2u, nearest.end());
    s.nearest_pool_dist_median = nearest[nearest.size() / 2u];
  }

  // Per record: the draw that writes it, the CPU reference of its validity, inverse and bounds.
  struct RecordCheck {
    bool covered = false;
    bool expected = false;
    bool ray_hit = false;
    float ray_entry = 0.f;
    float inverse_err = 0.f;
    float bounds_err = 0.f;
    uint32_t draw = 0u;
    uint32_t slot = 0u;
    uint32_t flags = 0u;
    bool args_ok = false;
  };
  std::vector<RecordCheck> checks(in.records.size());
  bool expected_invalid_gpu = false;  // a covered record CPU accepts that the GPU rejected
  bool large_error = false;           // a covered valid record with an inverse or bounds error beyond tolerance
  bool box_crossed = false;           // a valid record whose box the inspect ray enters
  for (size_t r = 0u; r < in.records.size(); ++r) {
    const WorldInstanceGPU& rec = in.records[r];
    RecordCheck& check = checks[r];
    const bool valid_gpu = rec.header[3] > 0.5f;
    for (size_t d = 0u; d < in.table.size() && !check.covered; ++d) {
      const DynamicDrawGPU& draw = in.table[d];
      if (r < draw.info[2] || r >= static_cast<uint64_t>(draw.info[2]) + draw.info[1]) continue;
      check.covered = true;
      check.draw = static_cast<uint32_t>(d);
      check.slot = draw.extra[2];
      check.flags = draw.info[3] & 6u;
      const uint32_t i = static_cast<uint32_t>(r - draw.info[2]);
      uint32_t count = draw.info[1];
      if (draw.extra[0] != 0u) {
        const size_t args = static_cast<size_t>(draw.extra[1]) * 4u + 1u;
        if (args < arena_floats) {
          uint32_t instance_count = 0u;
          std::memcpy(&instance_count, &in.arena[args], sizeof(uint32_t));
          count = (std::min)(count, instance_count);
          check.args_ok = true;
        }
      }
      const bool live = (draw.info[3] & 1u) != 0u && i < count;
      const size_t base = static_cast<size_t>(draw.info[0] + i * (kPoolInstanceStride / 16u)) * 4u;
      if (live && base + 12u <= arena_floats) {
        float matrix[12] = {};
        std::memcpy(matrix, in.arena.data() + base, sizeof(matrix));
        const float det = matrix[0] * (matrix[5] * matrix[10] - matrix[6] * matrix[9])
                          - matrix[1] * (matrix[4] * matrix[10] - matrix[6] * matrix[8])
                          + matrix[2] * (matrix[4] * matrix[9] - matrix[5] * matrix[8]);
        check.expected = std::isfinite(det) && std::fabs(det) >= 1e-12f && CheckPoolWorld(matrix) == PoolMatrixReject::None;
        if (check.expected) {
          float inverse[16] = {};
          ComputeMatrixInverse(matrix, inverse);
          for (int row = 0; row < 3; ++row) {
            for (int col = 0; col < 4; ++col) {
              const float want = inverse[row * 4 + col];
              const float got = rec.inverse_world[row * 4 + col];
              const float err = std::fabs(got - want) / (std::max)(1.f, std::fabs(want));
              if (std::isfinite(err)) check.inverse_err = (std::max)(check.inverse_err, err);
              else check.inverse_err = std::numeric_limits<float>::infinity();
            }
          }
          float lo[3] = {1e30f, 1e30f, 1e30f};
          float hi[3] = {-1e30f, -1e30f, -1e30f};
          for (int corner = 0; corner < 8; ++corner) {
            const float p[3] = {(corner & 1) ? draw.bbox_max[0] : draw.bbox_min[0], (corner & 2) ? draw.bbox_max[1] : draw.bbox_min[1],
                                (corner & 4) ? draw.bbox_max[2] : draw.bbox_min[2]};
            float w[3];
            TransformInst4x3RowDot(matrix, p[0], p[1], p[2], w);
            for (int k = 0; k < 3; ++k) {
              lo[k] = (std::min)(lo[k], w[k]);
              hi[k] = (std::max)(hi[k], w[k]);
            }
          }
          for (int k = 0; k < 3; ++k) {
            check.bounds_err = (std::max)(check.bounds_err, std::fabs(rec.bounds_min[k] - lo[k]));
            check.bounds_err = (std::max)(check.bounds_err, std::fabs(rec.bounds_max[k] - hi[k]));
          }
        }
      }
      if (check.covered && check.expected != valid_gpu) s.gpu_cpu_validity_mismatch += 1u;
    }
    if (valid_gpu) s.records_valid += 1u;
    else s.records_invalid += 1u;
    if (check.covered && check.expected && !valid_gpu) expected_invalid_gpu = true;
    if (check.covered && check.expected && valid_gpu && (check.inverse_err > 1e-3f || check.bounds_err > 1e-2f)) large_error = true;
    if (in.ray_valid) check.ray_hit = RayBoxEntry(ray_origin, ray_direction, rec.bounds_min, rec.bounds_max, &check.ray_entry);
    if (valid_gpu && check.ray_hit) box_crossed = true;
  }
  if (summary_out != nullptr) *summary_out = s;

  // Verdict: the first matching row of the plan's table.
  const bool arena_zero_all = s.arena_elements_checked != 0u && s.arena_elements_zero == s.arena_elements_checked;
  bool arena_bad = false;  // a non-zero element far from the pool or non-finite, or an indirect draw with no instances
  for (const Element& e : elements) {
    if (e.readable && !e.zero && (!e.finite || e.nearest_dist > 50.f)) arena_bad = true;
  }
  for (const DynamicDrawGPU& draw : in.table) {
    if (draw.extra[0] == 0u) continue;
    const size_t args = static_cast<size_t>(draw.extra[1]) * 4u + 1u;
    if (args < arena_floats) {
      uint32_t instance_count = 0u;
      std::memcpy(&instance_count, &in.arena[args], sizeof(uint32_t));
      arena_bad = arena_bad || instance_count == 0u;
    }
  }
  const bool trace = in.fresh_trace && in.counters.trace_valid;
  const uint32_t presents = static_cast<uint32_t>(in.counters.expand_presents);
  const bool records_seen = !in.records.empty();
  const uint32_t tested = in.counters.p2b_tested;
  const uint32_t skipped = in.counters.p2b_skipped;
  const bool probe_read = in.probe_tail_read && in.probe_plain_read;
  const char* verdict = in.fresh_trace ? "open" : "open (no fresh trace)";
  if (probe_read && !in.probe_arena_tail_landed && in.probe_plain_landed) {
    verdict = "H1b: structured-160 to structured-16 copy dropped";  // pre-fix signature (the old structured arena), not expected now
  } else if (probe_read && in.probe_arena_tail_landed && arena_zero_all && !in.table.empty()) {
    verdict = "H1c: wrong source or offset";
  } else if (arena_bad) {
    verdict = "H1d: wrong base or stride or stale buffer";
  } else if (expected_invalid_gpu || large_error) {
    verdict = "H2: expansion shader or constants";
  } else if (in.table_readback_ok && !in.table_matches) {
    verdict = "H2: draw upload";
  } else if (trace && records_seen && box_crossed && (tested == 0u || in.counters.trace_count == 0u)) {
    verdict = "H3a: count path or loop not reached";
  } else if (trace && tested > 0u && skipped > 0u
             && static_cast<uint64_t>(skipped) * 10u >= (static_cast<uint64_t>(tested) + skipped) * 9u) {
    verdict = "H3b: t17 reads zeros or mesh_id out of range";
  } else if (trace && in.counters.p2b_box_hits == 0u && box_crossed) {
    verdict = "H3c: bounds or ray convention";
  } else if (trace && in.counters.p2b_box_hits > 0u && in.counters.p2b_wins == 0u) {
    verdict = "H3d: BLAS, slot or visibility";
  } else if ((presents >= 2u && in.counters.expand_empty * 2u > presents)
             || (in.fresh_trace && in.counters.trace_count != in.counters.instance_count)) {
    verdict = "H4: second present or timing";
  }
  if (verdict_hint != nullptr) *verdict_hint = verdict;

  // World_p2b_diag.json.
  std::ostringstream out;
  out << "{\n  \"schema\": 1,\n  \"build\": \"" << in.build << "\",\n  \"generated_frame\": " << in.generated_frame
      << ",\n  \"armed_frame\": " << in.armed_frame << ",\n  \"verdict_hint\": \"" << verdict << "\",\n";
  out << "  \"switches\": {";
  for (size_t i = 0u; i < in.switches.size(); ++i) {
    out << (i != 0u ? ", " : "") << "\"" << in.switches[i].first << "\": " << (in.switches[i].second ? "true" : "false");
  }
  out << "},\n";
  const P2bDiagCounters& c = in.counters;
  out << "  \"expand\": {\"ready\": " << (c.ready ? "true" : "false") << ", \"presents\": " << c.expand_presents
      << ", \"empty_presents\": " << c.expand_empty << ", \"failed_presents\": " << c.expand_failed
      << ", \"instance_count\": " << c.instance_count << ", \"draw_count\": " << in.table.size()
      << ", \"camera_draws\": " << std::count_if(in.table.begin(), in.table.end(), [](const DynamicDrawGPU& d) { return (d.info[3] & 2u) != 0u; })
      << ", \"shadow_draws\": " << std::count_if(in.table.begin(), in.table.end(), [](const DynamicDrawGPU& d) { return (d.info[3] & 4u) != 0u; })
      << "},\n";
  out << "  \"trace\": {\"valid\": " << (c.trace_valid ? "true" : "false") << ", \"p2b_tested\": " << c.p2b_tested
      << ", \"p2b_skipped\": " << c.p2b_skipped << ", \"p2b_box_hits\": " << c.p2b_box_hits << ", \"p2b_wins\": " << c.p2b_wins
      << ", \"trace_instance_count\": " << c.trace_count << ", \"trace_frame\": " << c.trace_frame
      << ", \"trace_stats_frame\": " << c.trace_stats_frame << ", \"fresh\": "
      << (in.fresh_trace ? "true" : "false") << ", \"armed_frame\": " << in.armed_frame << ", \"timed_out\": "
      << (in.timed_out ? "true" : "false") << "},\n";
  out << "  \"tlas\": {\"per_frame\": " << c.tlas_per_frame << ", \"dynamic_partition\": " << c.tlas_partition
      << ", \"flagged_skipped\": " << c.tlas_skipped << "},\n";
  out << "  \"flagged_meshes\": [";
  for (size_t i = 0u; i < in.flagged.size(); ++i) {
    const P2bDiagFlagged& f = in.flagged[i];
    out << (i != 0u ? ", " : "") << "{\"uid\": " << f.uid << ", \"slot\": " << f.slot << ", \"resident\": "
        << (f.resident ? "true" : "false") << ", \"pool_instances\": " << f.pool_instances << "}";
  }
  out << "],\n";
  out << "  \"draw_count\": " << in.table.size() << ", \"record_count\": " << in.records.size() << ",\n";
  out << "  \"readback\": {\"ok\": " << (in.readback_ok ? "true" : "false") << ", \"reason\": \"" << in.readback_reason
      << "\", \"table_matches\": " << (in.table_matches ? "true" : "false") << ", \"arena_bytes\": " << in.arena.size() * sizeof(float)
      << ", \"records_read\": " << in.records.size() << ", \"table_read\": " << in.table_readback.size()
      << ", \"arena_reason\": \"" << in.arena_reason << "\", \"records_reason\": \"" << in.records_reason
      << "\", \"table_reason\": \"" << in.table_reason << "\"},\n";
  out << "  \"probe\": {\"ran\": " << (in.probe_ran ? "true" : "false") << ", \"arena_tail_landed\": "
      << (in.probe_arena_tail_landed ? "true" : "false") << ", \"plain_landed\": " << (in.probe_plain_landed ? "true" : "false")
      << ", \"reason\": \"" << in.probe_reason << "\"},\n";
  out << "  \"draws\": [";
  for (size_t i = 0u; i < in.table.size(); ++i) {
    const DynamicDrawGPU& draw = in.table[i];
    const PoolDynamicDraw* pool_draw = i < in.draws.size() ? &in.draws[i] : nullptr;
    out << (i != 0u ? ", " : "") << "{\"mesh_key\": " << (pool_draw != nullptr ? pool_draw->mesh_key : 0u)
        << ", \"vs\": " << (pool_draw != nullptr ? pool_draw->vs_hash : 0u)
        << ", \"pass\": " << (pool_draw != nullptr ? static_cast<uint32_t>(pool_draw->pass) : 0u)
        << ", \"indirect\": " << (draw.extra[0] != 0u ? "true" : "false") << ", \"arena_off\": " << (draw.info[0] * 16u)
        << ", \"count\": " << draw.info[1] << ", \"out\": " << draw.info[2] << ", \"slot\": " << draw.extra[2]
        << ", \"resident\": " << ((draw.info[3] & 1u) != 0u ? "true" : "false")
        << ", \"table_matches\": " << (in.table_matches && i < in.table_readback.size() ? "true" : "false") << "}";
  }
  out << "],\n  \"elements\": [";
  for (size_t i = 0u; i < elements.size(); ++i) {
    const Element& e = elements[i];
    out << (i != 0u ? ", " : "") << "{\"draw\": " << e.draw << ", \"index\": " << e.index << ", \"readable\": "
        << (e.readable ? "true" : "false") << ", \"arena_zero\": " << (e.zero ? "true" : "false") << ", \"finite\": "
        << (e.finite ? "true" : "false") << ", \"world\": [";
    for (size_t k = 0u; k < 12u; ++k) out << (k != 0u ? ", " : "") << PoolJsonFloat{e.world[k]};
    out << "], \"cpu_reject\": \"" << PoolMatrixRejectName(e.reject) << "\", \"nearest_pool_id\": " << e.nearest_id
        << ", \"nearest_pool_dist_m\": " << PoolJsonFloat{e.nearest_dist} << "}";
  }
  out << "],\n  \"records\": [";
  for (size_t r = 0u; r < in.records.size() && r < kRecordCap; ++r) {
    const WorldInstanceGPU& rec = in.records[r];
    const RecordCheck& check = checks[r];
    out << (r != 0u ? ", " : "") << "{\"index\": " << r << ", \"valid_gpu\": " << (rec.header[3] > 0.5f ? "true" : "false")
        << ", \"mesh_slot\": " << PoolJsonFloat{rec.header[0]} << ", \"flags\": " << PoolJsonFloat{rec.visibility[2]}
        << ", \"translation\": [" << PoolJsonFloat{rec.world[3]} << ", " << PoolJsonFloat{rec.world[7]} << ", "
        << PoolJsonFloat{rec.world[11]} << "], \"bounds_min\": [" << PoolJsonFloat{rec.bounds_min[0]} << ", "
        << PoolJsonFloat{rec.bounds_min[1]} << ", " << PoolJsonFloat{rec.bounds_min[2]} << "], \"bounds_max\": ["
        << PoolJsonFloat{rec.bounds_max[0]} << ", " << PoolJsonFloat{rec.bounds_max[1]} << ", " << PoolJsonFloat{rec.bounds_max[2]}
        << "], \"cpu_expected_valid\": " << (check.expected ? "true" : "false") << ", \"covered\": "
        << (check.covered ? "true" : "false") << ", \"cpu_inverse_max_err\": " << PoolJsonFloat{check.inverse_err}
        << ", \"cpu_bounds_max_err\": " << PoolJsonFloat{check.bounds_err} << ", \"ray_entry\": "
        << (check.ray_hit ? "" : "null");
    if (check.ray_hit) out << PoolJsonFloat{check.ray_entry};
    out << "}";
  }
  out << "],\n  \"summary\": {\"records_valid\": " << s.records_valid << ", \"records_invalid\": " << s.records_invalid
      << ", \"arena_elements_checked\": " << s.arena_elements_checked << ", \"arena_elements_zero\": " << s.arena_elements_zero
      << ", \"draws_with_zero_arena\": " << s.draws_with_zero_arena << ", \"gpu_cpu_validity_mismatch\": "
      << s.gpu_cpu_validity_mismatch << ", \"nearest_pool_dist_median\": " << PoolJsonFloat{s.nearest_pool_dist_median}
      << ", \"verdict_hint\": \"" << verdict << "\"}\n}\n";

  if (summary_lines != nullptr) {
    summary_lines->clear();
    summary_lines->push_back(std::string("build ") + in.build);
    std::string switches = "switches:";
    for (const auto& item : in.switches) switches += std::string(" ") + item.first + (item.second ? "=on" : "=off");
    summary_lines->push_back(switches);
    summary_lines->push_back("expand: presents " + std::to_string(c.expand_presents) + ", empty " + std::to_string(c.expand_empty)
                             + ", failed " + std::to_string(c.expand_failed) + ", ready " + (c.ready ? "yes" : "no")
                             + ", records " + std::to_string(c.instance_count) + ", draws " + std::to_string(in.table.size()));
    summary_lines->push_back("trace: valid " + std::string(c.trace_valid ? "yes" : "no") + ", p2b tested " + std::to_string(c.p2b_tested)
                             + ", skipped " + std::to_string(c.p2b_skipped) + ", box hits " + std::to_string(c.p2b_box_hits)
                             + ", wins " + std::to_string(c.p2b_wins) + ", trace records " + std::to_string(c.trace_count));
    summary_lines->push_back(std::string("probe: ran ") + (in.probe_ran ? "yes" : "no") + ", arena tail landed "
                             + (in.probe_arena_tail_landed ? "yes" : "no") + ", plain landed " + (in.probe_plain_landed ? "yes" : "no"));
    summary_lines->push_back("records: valid " + std::to_string(s.records_valid) + ", invalid " + std::to_string(s.records_invalid)
                             + ", gpu-cpu mismatch " + std::to_string(s.gpu_cpu_validity_mismatch));
    summary_lines->push_back("arena: elements " + std::to_string(s.arena_elements_checked) + ", zero " + std::to_string(s.arena_elements_zero)
                             + ", draws zero " + std::to_string(s.draws_with_zero_arena) + ", nearest pool median "
                             + std::to_string(s.nearest_pool_dist_median) + " m");
    summary_lines->push_back(std::string("readback: ") + (in.readback_ok ? std::string("ok") : "not ok (" + in.readback_reason + ")")
                             + ", table " + (in.table_matches ? "matches" : "differs"));
    summary_lines->push_back("verdict_hint: " + std::string(verdict));
  }
  return out.str();
}

// Per present (OnWorldPresent, after ExpandLiveDynamic and RefitLiveTlasAfterDrain). Waits for a present with
// a flagged draw and a trace stats readback, or kP2bDiagWaitFrames presents, then writes the diagnostics.
inline void RunP2bDiagnostics(reshade::api::device* device, reshade::api::command_queue* queue) {
  if (device == nullptr || queue == nullptr || !g_p2b_diag.requested.load(std::memory_order_relaxed)) return;
  const uint32_t frame = g_state.frame.load();
  const uint32_t armed = g_p2b_diag.armed_frame.load();
  BvhDeviceData* data = GetBvhDeviceData(device);
  P2bDiagInput in;
  in.build = kWorldBuildId;
  in.generated_frame = frame;
  in.armed_frame = armed;
  uint64_t arena_used = 0u;
  uint32_t instance_count = 0u;
  reshade::api::resource arena = {0u};
  reshade::api::resource instances = {0u};
  reshade::api::resource table_buffer = {0u};
  bool timed_out = false;
  {
    std::lock_guard<std::mutex> lock(g_pool.mutex);
    const PoolStats& stats = g_pool.stats;
    const bool fresh = data != nullptr && data->trace_stats.frame >= armed && data->trace_stats.valid;
    const bool present_ready = fresh && !data->dynamic.last_table.empty();
    if (!present_ready) {
      if (frame < armed + kP2bDiagWaitFrames) return;
      timed_out = true;
    }
    in.fresh_trace = fresh;
    in.timed_out = timed_out;
    P2bDiagCounters& c = in.counters;
    c.ready = stats.dynamic_ready;
    c.expand_presents = stats.dynamic_expand_presents;
    c.expand_empty = stats.dynamic_expand_empty;
    c.expand_failed = stats.dynamic_expand_failed;
    c.instance_count = stats.dynamic_instance_count;
    c.trace_count = stats.dynamic_trace_count;
    c.trace_frame = stats.dynamic_trace_frame;
    c.tlas_per_frame = stats.dynamic_tlas_per_frame;
    c.tlas_partition = stats.dynamic_tlas_partition;
    c.tlas_skipped = stats.dynamic_tlas_p2b_skipped;
    c.flagged_total = stats.dynamic_flagged_total;
    c.flagged_resident = stats.dynamic_flagged_resident;
    arena_used = stats.dynamic_arena_bytes;
    in.switches = {{"dynamic_per_frame", g_pool.dynamic_per_frame.load()},
                   {"alpha_foliage", g_pool.alpha_foliage.load()},
                   {"alpha_wind_sway", g_pool.alpha_wind_sway.load()},
                   {"exclude_moving", g_pool.exclude_moving.load()},
                   {"live_on", g_pool.live_on.load()},
                   {"legacy_scale", g_pool.legacy_scale.load()}};
    for (const WorldInstance& instance : g_pool.instances) {
      in.pool.push_back({instance.id, {instance.matrix[3], instance.matrix[7], instance.matrix[11]}});
    }
    std::vector<uint32_t> instances_per_mesh(g_pool.meshes.size(), 0u);
    for (const WorldInstance& instance : g_pool.instances) {
      if (instance.mesh_id < instances_per_mesh.size()) instances_per_mesh[instance.mesh_id] += 1u;
    }
    for (uint32_t m = 0u; m < g_pool.meshes.size(); ++m) {
      const WorldMesh& mesh = g_pool.meshes[m];
      if (!mesh.dynamic) continue;
      P2bDiagFlagged flagged;
      flagged.uid = mesh.uid;
      flagged.pool_instances = instances_per_mesh[m];
      if (data != nullptr) {
        const auto slot = data->slot_by_uid.find(mesh.uid);
        if (slot != data->slot_by_uid.end()) {
          flagged.slot = slot->second;
          flagged.resident = slot->second < data->slots.size() && data->slots[slot->second].live;
        }
      }
      in.flagged.push_back(flagged);
    }
    if (data != nullptr) {
      c.trace_valid = data->trace_stats.valid;
      c.trace_stats_frame = data->trace_stats.frame;
      c.p2b_tested = data->trace_stats.p2b_tested;
      c.p2b_skipped = data->trace_stats.p2b_skipped;
      c.p2b_box_hits = data->trace_stats.p2b_box_hits;
      c.p2b_wins = data->trace_stats.p2b_wins;
      in.draws = data->dynamic.last_draws;
      in.table = data->dynamic.last_table;
      in.ray_valid = data->inspect.ray_valid;
      std::copy(data->inspect.ray_origin.begin(), data->inspect.ray_origin.end(), in.ray_origin);
      std::copy(data->inspect.ray_direction.begin(), data->inspect.ray_direction.end(), in.ray_direction);
      instance_count = data->dynamic.instance_count;
      arena = data->dynamic.arena;
      instances = data->dynamic.instances;
      table_buffer = data->dynamic.draws;
    }
  }

  // GPU work from here on: no module mutex is held.
  std::string reason;
  if (timed_out) reason += "timed out after " + std::to_string(kP2bDiagWaitFrames) + " presents (no trace stats of a flagged draw); ";
  std::vector<uint8_t> bytes;
  const uint64_t arena_bytes = (std::min)(arena_used, kP2bDiagArenaReadBytes);
  if (arena_bytes == 0u) {
    in.arena_reason = "skipped: arena bytes 0 this present";
  } else if (arena.handle == 0u || !renodx::utils::scene::ReadbackBuffer(device, queue, arena, 0u, arena_bytes, &bytes) || bytes.size() != arena_bytes) {
    in.arena_reason = arena.handle == 0u ? "skipped: no arena buffer" : "readback failed";
    reason += "arena not read; ";
  } else {
    in.arena.resize(arena_bytes / sizeof(float));
    std::memcpy(in.arena.data(), bytes.data(), arena_bytes);
  }
  const uint32_t records_read = (std::min)(instance_count, kP2bDiagRecordsRead);
  const uint64_t record_bytes = static_cast<uint64_t>(records_read) * sizeof(WorldInstanceGPU);
  if (records_read == 0u) {
    in.records_reason = "skipped: no instances";
  } else if (instances.handle == 0u || !renodx::utils::scene::ReadbackBuffer(device, queue, instances, 0u, record_bytes, &bytes) || bytes.size() != record_bytes) {
    in.records_reason = instances.handle == 0u ? "skipped: no instance buffer" : "readback failed";
    reason += "records not read; ";
  } else {
    in.records.resize(records_read);
    std::memcpy(in.records.data(), bytes.data(), record_bytes);
  }
  in.table_matches = true;
  in.table_readback_ok = true;
  const size_t table_read = (std::min)(in.table.size(), static_cast<size_t>(kP2bDiagTableRead));
  const uint64_t table_bytes = static_cast<uint64_t>(table_read) * sizeof(DynamicDrawGPU);
  if (table_read == 0u) {
    in.table_reason = "skipped: no draws";
  } else if (table_buffer.handle == 0u || !renodx::utils::scene::ReadbackBuffer(device, queue, table_buffer, 0u, table_bytes, &bytes) || bytes.size() != table_bytes) {
    in.table_reason = table_buffer.handle == 0u ? "skipped: no draw table buffer" : "readback failed";
    in.table_readback_ok = false;
    reason += "draw table not read; ";
  } else {
    in.table_readback.resize(table_read);
    std::memcpy(in.table_readback.data(), bytes.data(), table_bytes);
    in.table_matches = std::memcmp(in.table_readback.data(), in.table.data(), table_bytes) == 0;
  }

  // The probe: the only GPU write of this path. Pattern copies into the arena tail (the arena is a raw stride-0 buffer, as the
  // control is) and into a stride-0 control buffer, both from a stride-160 source; read back and compared.
  if (arena.handle != 0u) {
    in.probe_ran = true;
    std::vector<uint8_t> pattern(kP2bDiagProbeBytes);
    for (size_t i = 0u; i < pattern.size(); ++i) pattern[i] = static_cast<uint8_t>(0xA0u + (i % 0x40u));
    std::vector<uint8_t> zeros(kP2bDiagProbeBytes, 0u);
    reshade::api::resource probe_source = {0u};
    reshade::api::resource_view probe_view = {0u};
    reshade::api::resource control = {0u};
    const bool source_created = CreatePoolBuffer(device, pattern.data(), kP2bDiagProbeBytes, kPoolInstanceStride, &probe_source, &probe_view);
    reshade::api::resource_desc control_desc = {};
    control_desc.type = reshade::api::resource_type::buffer;
    control_desc.buffer.size = kP2bDiagProbeBytes;
    control_desc.heap = reshade::api::memory_heap::gpu_only;
    control_desc.usage = reshade::api::resource_usage::copy_source | reshade::api::resource_usage::copy_dest;
    reshade::api::subresource_data control_init = {zeros.data(), 0u, 0u};
    const bool control_created = device->create_resource(control_desc, &control_init, reshade::api::resource_usage::copy_dest, &control);
    if (!source_created || !control_created) {
      in.probe_reason = "probe buffers not created";
    } else {
      auto* cmd_list = queue->get_immediate_command_list();
      cmd_list->barrier(arena, reshade::api::resource_usage::shader_resource, reshade::api::resource_usage::copy_dest);
      cmd_list->copy_buffer_region(probe_source, 0u, arena, kPoolDynamicArenaBytes - kP2bDiagProbeBytes, kP2bDiagProbeBytes);
      cmd_list->copy_buffer_region(probe_source, 0u, control, 0u, kP2bDiagProbeBytes);
      cmd_list->barrier(arena, reshade::api::resource_usage::copy_dest, reshade::api::resource_usage::shader_resource);
      std::vector<uint8_t> tail;
      std::vector<uint8_t> plain;
      if (renodx::utils::scene::ReadbackBuffer(device, queue, arena, kPoolDynamicArenaBytes - kP2bDiagProbeBytes, kP2bDiagProbeBytes, &tail)
          && tail.size() == kP2bDiagProbeBytes) {
        in.probe_tail_read = true;
        in.probe_arena_tail_landed = tail == pattern;
      } else {
        in.probe_reason = "tail readback failed";
      }
      if (renodx::utils::scene::ReadbackBuffer(device, queue, control, 0u, kP2bDiagProbeBytes, &plain) && plain.size() == kP2bDiagProbeBytes) {
        in.probe_plain_read = true;
        in.probe_plain_landed = plain == pattern;
      } else {
        in.probe_reason = in.probe_tail_read ? "plain readback failed" : in.probe_reason + "; plain readback failed";
      }
      if (!in.probe_tail_read || !in.probe_plain_read) reason += in.probe_reason + "; ";
    }
    DestroyBuffer(device, &probe_view, &probe_source);
    if (control.handle != 0u) device->destroy_resource(control);
  } else {
    in.probe_reason = "no arena (live BVH off or GPU objects not created)";
  }

  in.readback_ok = reason.empty();
  in.readback_reason = reason;
  std::string verdict;
  std::vector<std::string> lines;
  std::string json = BuildP2bDiagJson(in, &verdict, &lines);
  std::string summary;
  for (const std::string& line : lines) summary += line + "\n";
  renodx::utils::path::WriteTextFile(PoolOutputDir() / "world_p2b_diag.json", json);
  renodx::utils::path::WriteTextFile(PoolOutputDir() / "world_p2b_summary.txt", summary);
  for (const std::string& line : lines) renodx::utils::log::i("[world-bvh] p2b diag: ", line);
  {
    std::lock_guard<std::mutex> lock(g_p2b_diag.mutex);
    g_p2b_diag.verdict_hint = verdict;
  }
  g_p2b_diag.written_frame.store(frame);
  g_p2b_diag.state.store(in.readback_ok ? 2u : 3u);
  g_p2b_diag.requested.store(false);
  DumpDeformLive();
  DumpWorldPool(g_bvh_trace.hide_camera_hidden.load(std::memory_order_relaxed));
}

}  // namespace falcom_world::bvh
