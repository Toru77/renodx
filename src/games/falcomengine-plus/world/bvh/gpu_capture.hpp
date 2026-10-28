// Included inside falcom_world::bvh (from bvh_pool.hpp); this file has no namespace line.
// GPU geometry cache (G2), draw side: per draw key, the GPU-to-GPU copies of the index, vertex and UV ranges that
// the present side decodes into the carrier. Validation only: nothing traces this geometry. The present side (D3D11
// passes, D3D12 copies, BLAS, the check) is in rt12/rt12_capture.hpp. The shaders are transcribed by the pure
// functions below, and the harness tests them against the CPU decode.
#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <map>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

inline constexpr uint64_t kGcStagingBytes = 16ull << 20;
inline constexpr uint32_t kGcJobsPerPresent = kPoolMeshCopiesPerFrame;  // range jobs and decode jobs, each
inline constexpr uint32_t kGcReadbackSlots = 3u;                         // range results read two presents later
inline constexpr size_t kGcMaxEntries = kPoolMaxMeshes;
inline constexpr uint32_t kGcMaxRewrites = 3u;
inline constexpr uint32_t kGcCarrierWidth = 4096u;
inline constexpr uint32_t kGcCarrierHeight = 512u;
inline constexpr uint64_t kGcCarrierTexels = uint64_t{kGcCarrierWidth} * kGcCarrierHeight;
inline constexpr uint64_t kGcBlasTrianglesPerPresent = 1ull << 20;
inline constexpr uint64_t kGcGeometryBytesMax = 256ull << 20;
inline constexpr uint64_t kGcBlasBytesMax = 512ull << 20;
inline constexpr uint32_t kGcCheckMeshes = 64u;
// Format codes of the decode shader (world_gc_decode.cs_5_0.hlsl GC_FORMAT_*).
inline constexpr uint32_t kGcFormatNone = 0u, kGcFormatF32x2 = 1u, kGcFormatF32x3 = 2u, kGcFormatF32x4 = 3u,
                          kGcFormatF16x2 = 4u, kGcFormatF16x4 = 5u;
inline constexpr uint32_t kGcNoUv = 0xFFFFFFFFu;  // GcDecodeParams uv_offset: the draw has no UV
// G3: the GPU instance set (rt12/rt12_capture.hpp step 7b; world_gc_insert / world_gc_compact.cs_5_0.hlsl).
inline constexpr uint32_t kGcSetCapacity = 65536u;               // set entries per half (power of two); set records at [0, n)
inline constexpr uint32_t kGcMovingRecords = 8192u;              // per-present records of moving sightings, at [kGcSetCapacity, +n)
inline constexpr uint32_t kGcSetMaxProbes = 32u;
inline constexpr uint64_t kGcInstanceArenaBytes = 16ull << 20;    // instance slices copied at the draws of one present
inline constexpr uint32_t kGcInstanceDrawsPerPresent = 16384u;
inline constexpr float kGcPoseTranslationScale = 256.f;            // pose key grid: 1/256 m
inline constexpr float kGcPoseBasisScale = 4096.f;                 // and 1/4096 per basis element
inline constexpr uint32_t kGcCompareCpuSamples = 256u, kGcCompareGpuSamples = 32u;
// Modes of world_gc_insert.cs_5_0.hlsl (GC_MODE_*); dispatched in the order clear, mark, rehash, insert.
inline constexpr uint32_t kGcModeClear = 0u, kGcModeRehash = 1u, kGcModeMark = 2u, kGcModeInsert = 3u;
// The 16 counters of one present (world_gc_instance.hlsli GC_COUNTER_*).
inline constexpr uint32_t kGcCounterSet = 0u, kGcCounterInserted = 1u, kGcCounterHits = 2u, kGcCounterKept = 3u,
                          kGcCounterAged = 4u, kGcCounterDroppedEpoch = 5u, kGcCounterDroppedMoving = 6u,
                          kGcCounterMovingRecords = 7u, kGcCounterMovingHeld = 8u, kGcCounterInvalid = 9u,
                          kGcCounterStaleEpoch = 10u, kGcCounterSetOverflow = 11u, kGcCounterRehashOverflow = 12u,
                          kGcCounterMarked = 13u, kGcCounterSightings = 14u, kGcCounterCount = 16u;
static_assert((kGcSetCapacity & (kGcSetCapacity - 1u)) == 0u, "the set probe mask needs a power of two");
static_assert(kGcMaxEntries < (1u << 24), "a slot must be exact in a float (record header.x)");

inline uint32_t GcFormatCode(reshade::api::format format) {
  switch (format) {
    case reshade::api::format::r32g32_float: return kGcFormatF32x2;
    case reshade::api::format::r32g32b32_float: return kGcFormatF32x3;
    case reshade::api::format::r32g32b32a32_float: return kGcFormatF32x4;
    case reshade::api::format::r16g16_float: return kGcFormatF16x2;
    case reshade::api::format::r16g16b16a16_float: return kGcFormatF16x4;
    default: return kGcFormatNone;
  }
}

enum class GcPhase : uint8_t { Range, RangeWait, Data, DataWait, Done, Refused };

struct GcEntry {
  uint64_t mesh_key = 0u;
  uint32_t generation = 0u;  // new value at every creation or reset: stale jobs and results are dropped
  GcPhase phase = GcPhase::Range;
  const char* reason = "";
  DrawRecord draw;
  int32_t pos_offset = 0;
  uint32_t pos_format = kGcFormatNone;
  int32_t uv_offset = -1;  // in the VB (uv.buffer 0) or in the UV stream (uv.buffer set): -1 = no UV
  uint32_t uv_format = kGcFormatNone;
  PoolUvStream uv;
  uint32_t min_vertex = 0u, max_vertex = 0u;
  uint32_t rewrites = 0u;
  uint32_t frame = 0u;
  uint32_t slot = 0u;  // G3 mesh slot (D1)
};

struct GcJob {
  uint64_t mesh_key = 0u;
  uint32_t generation = 0u;
  uint32_t index_byte = 0u;   // staging byte address of the first wanted byte (skip included)
  uint32_t vertex_byte = 0u;  // the vertex at min_vertex
  uint32_t uv_byte = 0u;      // the UV stream copy's first byte, or vertex_byte for a UV in the VB
};

struct GcDecoded {
  uint64_t mesh_key = 0u;
  uint32_t generation = 0u;
  uint32_t vertex_count = 0u;
  uint32_t index_count = 0u;
  bool has_uv = false;
  uint32_t vtx_texel = 0u;
  uint32_t idx_texel = 0u;
  uint32_t uv_texel = 0u;
  uint32_t vb_usage = 0u, vb_flags = 0u, ib_usage = 0u, ib_flags = 0u, pos_format = 0u, index_size = 0u;  // the check split
};

// One direct draw of a GPU-cache mesh in this present (world_gc_instance.hlsli GcInstanceDraw).
struct GcInstanceDrawGPU {
  uint32_t arena_float4 = 0u;  // its instance slice in the instance arena, in float4 units
  uint32_t count = 0u;
  uint32_t slot = 0u;
  uint32_t epoch = 0u;         // slot_epoch[slot] at the draw
  uint32_t pass_bits = 0u;     // kPoolPass* of the draw
  uint32_t pad[3] = {};
};
static_assert(sizeof(GcInstanceDrawGPU) == 32u, "GcInstanceDrawGPU must match world_gc_instance.hlsli");
// One entry of the instance set (world_gc_instance.hlsli GcSetEntry); tag, last_seen and flags are in the meta buffer.
struct GcSetEntryGPU {
  uint32_t slot = 0u, epoch = 0u, pose_lo = 0u, pose_hi = 0u;
  float world[12] = {};
};
static_assert(sizeof(GcSetEntryGPU) == 64u, "GcSetEntryGPU must match world_gc_instance.hlsli");
// The instance slice of a direct draw (OnPoolScanDraw).
struct GcInstanceSource {
  reshade::api::resource buffer = {0u};  // t15
  int32_t base = 0;                      // CPU b1 base
  uint32_t count = 0u;
  uint8_t pass = 0u;                     // kPoolPass* bits
};

struct GcStats {
  uint64_t created = 0u, range_jobs = 0u, decode_jobs = 0u, decoded = 0u, deferred_skips = 0u, staging_full = 0u;
  uint64_t jobs_cap = 0u, cap_attempts = 0u, carrier_busy = 0u, carrier_full = 0u, stale_jobs = 0u, invalidated_write = 0u;
  uint64_t uv_stream_mismatch = 0u;  // a Data-phase draw bound another UV stream buffer than the entry's (skipped)
  uint64_t invalidated_destroy = 0u, resets_pool = 0u, resets_fragmented = 0u;
  uint64_t readback_busy = 0u;  // range jobs returned to Range: their readback slot was unread
  uint64_t inst_draws = 0u, inst_instances = 0u, inst_no_geometry = 0u, inst_deferred = 0u, inst_arena_full = 0u, inst_draws_cap = 0u;  // G3 draw side (D2)
  std::map<std::string, uint64_t> refused;
  std::map<std::array<uint32_t, 4>, uint64_t> formats;  // pos code, uv code, index size, stride
  std::map<std::array<uint32_t, 4>, uint64_t> buffers;  // vb usage, vb flags, ib usage, ib flags
  std::map<std::array<uint32_t, 4>, uint64_t> rewritten_by_buffer;
  uint32_t largest_vertex_range = 0u, largest_index_count = 0u;
};

struct GpuCaptureState {
  std::atomic<bool> enabled{false};  // the panel switch (session only, not saved)
  std::atomic<bool> active{false};   // set by the present side once its objects exist; every hook returns on !active
  std::mutex mutex;
  reshade::api::resource staging = {0u};  // D3D11 raw buffer, kGcStagingBytes
  uint64_t staging_used = 0u;             // bytes reserved in this present's copies
  uint32_t next_generation = 1u;
  std::unordered_map<uint64_t, GcEntry> entries;
  std::vector<GcJob> range_jobs, decode_jobs;
  std::vector<uint64_t> invalidated;  // keys whose D3D12 geometry must be dropped at the next present
  GcStats stats;
  // G3 (D1, D2).
  reshade::api::resource instance_arena = {0u};  // D3D11 raw buffer, kGcInstanceArenaBytes
  uint64_t instance_arena_used = 0u;
  std::vector<GcInstanceDrawGPU> instance_draws;
  uint32_t next_slot = 0u;
  std::vector<uint32_t> free_slots;
  std::vector<uint32_t> slot_epoch = std::vector<uint32_t>(kGcMaxEntries, 0u);
  std::vector<uint64_t> slot_key = std::vector<uint64_t>(kGcMaxEntries, 0u);
  bool epochs_dirty = true;
};

inline GpuCaptureState g_gpu_capture;

// Caller holds g_gpu_capture.mutex. The slot's set entries and recorded draws become stale on the GPU (D1).
inline void BumpGcSlotEpoch(uint32_t slot) {
  g_gpu_capture.slot_epoch[slot] += 1u;
  g_gpu_capture.epochs_dirty = true;
}

// Caller holds g_gpu_capture.mutex.
inline void RefuseGpuCaptureEntry(GcEntry& entry, const char* reason) {
  entry.phase = GcPhase::Refused;
  entry.reason = reason;
  g_gpu_capture.stats.refused[reason] += 1u;
  BumpGcSlotEpoch(entry.slot);
}

// The game wrote or destroyed a buffer of these keys (called from the pool's hooks, after its lock is released).
inline void InvalidateGpuCaptureKeys(const std::vector<uint64_t>& keys, bool destroyed) {
  if (!g_gpu_capture.active.load(std::memory_order_relaxed)) return;
  GpuCaptureState& g = g_gpu_capture;
  std::lock_guard lock(g.mutex);
  for (const uint64_t mesh_key : keys) {
    const auto it = g.entries.find(mesh_key);
    if (it == g.entries.end()) continue;
    GcEntry& entry = it->second;
    const bool was_refused = entry.phase == GcPhase::Refused;
    if (destroyed) {
      BumpGcSlotEpoch(entry.slot);
      g.free_slots.push_back(entry.slot);
      g.slot_key[entry.slot] = 0u;
      g.entries.erase(it);
      g.stats.invalidated_destroy += 1u;
    } else {
      if (was_refused) continue;
      g.stats.invalidated_write += 1u;
      g.stats.rewritten_by_buffer[{entry.draw.vb_usage, entry.draw.vb_flags, entry.draw.ib_usage, entry.draw.ib_flags}] += 1u;
      entry.rewrites += 1u;
      BumpGcSlotEpoch(entry.slot);
      if (entry.rewrites >= kGcMaxRewrites) {
        RefuseGpuCaptureEntry(entry, "rewritten");
      } else {
        entry.phase = GcPhase::Range;
        entry.generation = g.next_generation++;
      }
    }
    if (!was_refused) g.invalidated.push_back(mesh_key);
  }
}

// Every entry is dropped. `why` is "pool reset" or "fragmented" (counted).
inline void ResetGpuCaptureEntries(const char* why) {
  GpuCaptureState& g = g_gpu_capture;
  std::lock_guard lock(g.mutex);
  for (const auto& [mesh_key, entry] : g.entries) {
    g.invalidated.push_back(mesh_key);
    BumpGcSlotEpoch(entry.slot);
  }
  g.entries.clear();
  g.range_jobs.clear();
  g.decode_jobs.clear();
  g.instance_draws.clear();
  g.instance_arena_used = 0u;
  g.next_slot = 0u;
  g.free_slots.clear();
  std::fill(g.slot_key.begin(), g.slot_key.end(), 0u);
  if (std::strcmp(why, "pool reset") == 0) g.stats.resets_pool += 1u;
  if (std::strcmp(why, "fragmented") == 0) g.stats.resets_fragmented += 1u;
}

// Called from OnPoolScanDraw after its lock is released, for a direct draw whose buffers the pool tracks for this key.
// Returns the copies issued. Copies only on the immediate context (a deferred draw serves none).
// With `instances`, a draw of a Done entry also copies its instance slice into the G3 arena (D2).
inline uint32_t GpuCaptureOnDraw(reshade::api::command_list* cmd_list, const DrawRecord& draw, uint64_t mesh_key,
                                 const PoolUvStream& uv, bool deferred, const GcInstanceSource* instances = nullptr) {
  if (!g_gpu_capture.active.load(std::memory_order_relaxed) || mesh_key == 0u) return 0u;
  GpuCaptureState& g = g_gpu_capture;
  std::array<PoolCopyCommand, 4> commands;
  uint32_t count = 0u;
  {
    std::lock_guard lock(g.mutex);
    if (g.staging.handle == 0u) return 0u;  // torn down between the flag and the lock
    // A copy of [begin, end) of `source` into staging: 16-aligned offset, 4-aligned copy. Returns false when it does not fit.
    const auto reserve = [&](reshade::api::resource source, uint64_t size, uint64_t begin, uint64_t end, uint64_t* cursor,
                             uint32_t* byte, PoolCopyCommand* command) {
      const uint64_t offset = (*cursor + 15u) & ~uint64_t{15u};
      const uint64_t aligned_begin = begin & ~uint64_t{3u};
      const uint64_t copied = (std::min)((end + 3u) & ~uint64_t{3u}, size) - aligned_begin;
      if (offset + copied > kGcStagingBytes) {
        g.stats.staging_full += 1u;
        return false;
      }
      *cursor = offset + copied;
      *byte = static_cast<uint32_t>(offset + (begin - aligned_begin));
      *command = {source, aligned_begin, g.staging, offset, copied};
      return true;
    };

    auto it = g.entries.find(mesh_key);
    if (it == g.entries.end()) {
      if (g.entries.size() >= kGcMaxEntries) {
        g.stats.cap_attempts += 1u;
        return 0u;
      }
      GcEntry created;
      created.mesh_key = mesh_key;
      created.generation = g.next_generation++;
      created.draw = draw;
      created.frame = g_state.frame.load();
      if (g.free_slots.empty()) {
        created.slot = g.next_slot++;
      } else {
        created.slot = g.free_slots.back();
        g.free_slots.pop_back();
      }
      g.slot_key[created.slot] = mesh_key;
      reshade::api::format pos_format = reshade::api::format::unknown;
      reshade::api::format layout_uv_format = reshade::api::format::unknown;
      int32_t layout_uv_offset = -1;
      const char* refusal = ResolvePoolMeshLayout(draw, &created.pos_offset, &pos_format, &layout_uv_offset, &layout_uv_format);
      const bool stream = uv.buffer.handle != 0u;
      const bool in_vb_uv = !stream && layout_uv_offset >= 0;
      if (refusal == nullptr && stream) {
        created.uv = uv;
        created.uv_offset = static_cast<int32_t>(uv.element_offset);
        created.uv_format = GcFormatCode(uv.format);
      } else if (refusal == nullptr && in_vb_uv) {
        created.uv_offset = layout_uv_offset;
        created.uv_format = GcFormatCode(layout_uv_format);
      }
      created.pos_format = GcFormatCode(pos_format);
      const uint32_t written = static_cast<uint32_t>(reshade::api::resource_usage::unordered_access)
                               | static_cast<uint32_t>(reshade::api::resource_usage::stream_output);
      const uint32_t dynamic = static_cast<uint32_t>(reshade::api::resource_flags::dynamic);
      const bool has_uv = stream || in_vb_uv;
      if (refusal == nullptr && ((draw.vb_usage | draw.ib_usage) & written) != 0u) refusal = "gpu_written";
      if (refusal == nullptr && ((draw.vb_flags | draw.ib_flags) & dynamic) != 0u) refusal = "cpu_dynamic";
      if (refusal == nullptr && created.pos_format == kGcFormatNone) refusal = "format";
      if (refusal == nullptr && has_uv && created.uv_format == kGcFormatNone) refusal = "uv_format";
      const uint32_t uv_code = has_uv ? created.uv_format : kGcFormatNone;
      g.stats.created += 1u;
      g.stats.formats[{created.pos_format, uv_code, draw.index_size, draw.vb_stride}] += 1u;
      g.stats.buffers[{draw.vb_usage, draw.vb_flags, draw.ib_usage, draw.ib_flags}] += 1u;
      created.phase = GcPhase::Range;
      if (refusal != nullptr) RefuseGpuCaptureEntry(created, refusal);
      it = g.entries.emplace(mesh_key, std::move(created)).first;
    }
    GcEntry& entry = it->second;
    if (deferred && (entry.phase == GcPhase::Range || entry.phase == GcPhase::Data)) {
      g.stats.deferred_skips += 1u;
      return 0u;
    }
    if (entry.phase == GcPhase::Range) {
      const uint64_t begin = draw.ib_offset + uint64_t{draw.first_index} * draw.index_size;
      const uint64_t end = begin + uint64_t{draw.index_count} * draw.index_size;
      if (end > draw.ib_size) {
        RefuseGpuCaptureEntry(entry, "range");
      } else if (g.range_jobs.size() >= kGcJobsPerPresent) {
        g.stats.jobs_cap += 1u;
      } else {
        uint64_t cursor = g.staging_used;
        uint32_t index_byte = 0u;
        PoolCopyCommand copy;
        if (reserve(draw.ib, draw.ib_size, begin, end, &cursor, &index_byte, &copy)) {
          g.staging_used = cursor;
          commands[count++] = copy;
          g.range_jobs.push_back({mesh_key, entry.generation, index_byte, 0u, 0u});
          entry.phase = GcPhase::RangeWait;
          g.stats.range_jobs += 1u;
        }
      }
    } else if (entry.phase == GcPhase::Data) {
      // The UV stream copy is served only by a draw that binds the same stream buffer as the entry.
      const bool stream = entry.uv.buffer.handle != 0u;
      if (stream && uv.buffer.handle != entry.uv.buffer.handle) {
        g.stats.uv_stream_mismatch += 1u;
      } else {
        const uint64_t min_vertex = entry.min_vertex;
        const uint64_t max_vertex = uint64_t{entry.max_vertex} + 1u;
        const uint64_t ib_begin = draw.ib_offset + uint64_t{draw.first_index} * draw.index_size;
        const uint64_t ib_end = ib_begin + uint64_t{draw.index_count} * draw.index_size;
        const uint64_t vb_begin = draw.vb_offset + min_vertex * draw.vb_stride;
        const uint64_t vb_end = draw.vb_offset + max_vertex * draw.vb_stride;
        const uint64_t uv_begin = stream ? entry.uv.offset + min_vertex * entry.uv.stride : 0u;
        const uint64_t uv_end = stream ? entry.uv.offset + max_vertex * entry.uv.stride : 0u;
        if (ib_end > draw.ib_size || vb_end > draw.vb_size || (stream && uv_end > entry.uv.size)) {
          RefuseGpuCaptureEntry(entry, "range");
        } else if (g.decode_jobs.size() >= kGcJobsPerPresent) {
          g.stats.jobs_cap += 1u;
        } else {
          uint64_t cursor = g.staging_used;
          uint32_t index_byte = 0u, vertex_byte = 0u, uv_byte = 0u;
          PoolCopyCommand copies[3];
          bool fits = reserve(draw.ib, draw.ib_size, ib_begin, ib_end, &cursor, &index_byte, &copies[0])
                      && reserve(draw.vb, draw.vb_size, vb_begin, vb_end, &cursor, &vertex_byte, &copies[1]);
          if (fits && stream) fits = reserve(entry.uv.buffer, entry.uv.size, uv_begin, uv_end, &cursor, &uv_byte, &copies[2]);
          if (fits) {
            g.staging_used = cursor;
            const uint32_t copy_count = stream ? 3u : 2u;
            for (uint32_t i = 0u; i < copy_count; ++i) commands[count++] = copies[i];
            if (!stream) uv_byte = vertex_byte;
            g.decode_jobs.push_back({mesh_key, entry.generation, index_byte, vertex_byte, uv_byte});
            entry.phase = GcPhase::DataWait;
            g.stats.decode_jobs += 1u;
          }
        }
      }
    }
    if (instances != nullptr) {
      const uint64_t bytes = uint64_t{instances->count} * kPoolInstanceStride;
      if (entry.phase != GcPhase::Done) {
        g.stats.inst_no_geometry += 1u;
      } else if (deferred) {
        g.stats.inst_deferred += 1u;
      } else if (g.instance_draws.size() >= kGcInstanceDrawsPerPresent) {
        g.stats.inst_draws_cap += 1u;
      } else if (g.instance_arena_used + bytes > kGcInstanceArenaBytes) {
        g.stats.inst_arena_full += 1u;
      } else {
        g.instance_draws.push_back({.arena_float4 = static_cast<uint32_t>(g.instance_arena_used / 16u),
                                    .count = instances->count,
                                    .slot = entry.slot,
                                    .epoch = g.slot_epoch[entry.slot],
                                    .pass_bits = instances->pass});
        commands[count++] = {instances->buffer, static_cast<uint64_t>(instances->base) * kPoolInstanceStride, g.instance_arena,
                             g.instance_arena_used, bytes};
        g.instance_arena_used += bytes;
        g.stats.inst_draws += 1u;
        g.stats.inst_instances += instances->count;
      }
    }
  }
  IssuePoolCopies(cmd_list, commands.data(), count);
  return count;
}

// The pure transcriptions of the two shaders (world_gc_range.cs_5_0.hlsl and world_gc_decode.cs_5_0.hlsl must match them).

// Dword at `address` (the byte address rounded down to 4) and the next one; words past `staging_size` read as 0.
inline uint32_t GcLoadU32(const uint8_t* staging, uint64_t staging_size, uint32_t address) {
  const uint64_t aligned = address & ~uint64_t{3u};
  const uint32_t shift = (address & 3u) * 8u;
  uint32_t w0 = 0u, w1 = 0u;
  if (aligned + 4u <= staging_size) std::memcpy(&w0, staging + aligned, sizeof(w0));
  if (aligned + 8u <= staging_size) std::memcpy(&w1, staging + aligned + 4u, sizeof(w1));
  return shift == 0u ? w0 : (w0 >> shift) | (w1 << (32u - shift));
}

// The same steps as renodx::utils::scene::HalfToFloat, including its subnormal branch (kept on purpose: the CPU pool
// is the reference, see the ROADMAP G2 reference).
inline float GcHalfToFloat(uint32_t bits16) {
  const uint32_t sign = (bits16 & 0x8000u) << 16u;
  uint32_t exp = (bits16 >> 10u) & 0x1Fu;
  uint32_t mantissa = bits16 & 0x3FFu;
  uint32_t f32 = 0u;
  if (exp == 0u) {
    if (mantissa == 0u) {
      f32 = sign;
    } else {
      exp = 1u;
      while ((mantissa & 0x400u) == 0u) {
        mantissa <<= 1u;
        exp++;
      }
      mantissa &= 0x3FFu;
      f32 = sign | ((exp + 112u) << 23u) | (mantissa << 13u);
    }
  } else if (exp == 31u) {
    f32 = sign | 0x7F800000u | (mantissa << 13u);
  } else {
    f32 = sign | ((exp + 112u) << 23u) | (mantissa << 13u);
  }
  float out = 0.f;
  std::memcpy(&out, &f32, sizeof(out));
  return out;
}

// out = {0, 0, 0, 1}, then the code's components at `address`.
inline void GcDecodeAttribute(const uint8_t* staging, uint64_t size, uint32_t address, uint32_t code, float out[4]) {
  out[0] = 0.f;
  out[1] = 0.f;
  out[2] = 0.f;
  out[3] = 1.f;
  const bool half = code == kGcFormatF16x2 || code == kGcFormatF16x4;
  const uint32_t count = (code == kGcFormatF32x2 || code == kGcFormatF16x2) ? 2u : (code == kGcFormatF32x3 ? 3u : 4u);
  for (uint32_t k = 0u; k < count; ++k) {
    if (half) {
      out[k] = GcHalfToFloat(GcLoadU32(staging, size, address + 2u * k) & 0xFFFFu);
    } else {
      const uint32_t bits = GcLoadU32(staging, size, address + 4u * k);
      std::memcpy(&out[k], &bits, sizeof(float));
    }
  }
}

// out = {min of the absolute indices, max, invalid count, 0}; min is stored as ~min so that it is an InterlockedMax.
inline void GcRangeJob(const uint8_t* staging, uint64_t size, uint32_t index_byte, uint32_t index_size, uint32_t index_count,
                       int32_t base_vertex, uint32_t out[4]) {
  out[0] = 0u;
  out[1] = 0u;
  out[2] = 0u;
  out[3] = 0u;
  for (uint32_t i = 0u; i < index_count; ++i) {
    uint32_t raw = GcLoadU32(staging, size, index_byte + i * index_size);
    if (index_size == 2u) raw &= 0xFFFFu;
    const int64_t absolute = static_cast<int64_t>(raw) + base_vertex;
    if (raw >= 0x80000000u || absolute < 0) {
      out[2] += 1u;
      continue;
    }
    const uint32_t abs_index = static_cast<uint32_t>(absolute);
    out[0] = (std::max)(out[0], ~abs_index);
    out[1] = (std::max)(out[1], abs_index);
  }
}

// The 16 push-constant words of the decode pass, in this order.
struct GcDecodeParams {
  uint32_t index_byte = 0u, index_size = 0u, index_count = 0u;
  int32_t base_vertex = 0;
  uint32_t vertex_byte = 0u, stride = 0u, pos_offset = 0u, pos_format = 0u;
  uint32_t uv_byte = 0u, uv_stride = 0u, uv_offset = kGcNoUv, uv_format = 0u;
  uint32_t vertex_count = 0u, min_vertex = 0u, vtx_texel = 0u, idx_texel = 0u;
};
static_assert(sizeof(GcDecodeParams) == 64u);

// Texels of one job in the carrier: vertices, index quads, and UV pairs when the job has UVs.
inline uint32_t GcJobTexels(uint32_t vertex_count, uint32_t index_count, bool has_uv) {
  return vertex_count + (index_count + 3u) / 4u + (has_uv ? (vertex_count + 1u) / 2u : 0u);
}

// Writes the job's texels into `carrier` (4 words per texel, texel n at word 4n). Texel n of the carrier is at
// (n % kGcCarrierWidth, n / kGcCarrierWidth) on the GPU.
inline void GcDecodeJob(const uint8_t* staging, uint64_t size, const GcDecodeParams& p, uint32_t* carrier) {
  const uint32_t index_texels = (p.index_count + 3u) / 4u;
  const uint32_t uv_texel = p.idx_texel + index_texels;
  const bool has_uv = p.uv_offset != kGcNoUv;
  const uint32_t loops = (std::max)(p.vertex_count, index_texels);
  for (uint32_t t = 0u; t < loops; ++t) {
    if (t < p.vertex_count) {
      float position[4] = {};
      GcDecodeAttribute(staging, size, p.vertex_byte + t * p.stride + p.pos_offset, p.pos_format, position);
      const float xyzw[4] = {position[0], position[1], position[2], 1.f};
      std::memcpy(carrier + size_t{p.vtx_texel + t} * 4u, xyzw, sizeof(xyzw));
    }
    if (t < index_texels) {
      uint32_t* texel = carrier + size_t{p.idx_texel + t} * 4u;
      for (uint32_t k = 0u; k < 4u; ++k) {
        const uint32_t i = 4u * t + k;
        uint32_t relative = 0u;
        if (i < p.index_count) {
          uint32_t raw = GcLoadU32(staging, size, p.index_byte + i * p.index_size);
          if (p.index_size == 2u) raw &= 0xFFFFu;
          relative = raw + static_cast<uint32_t>(p.base_vertex) - p.min_vertex;
          if (relative >= p.vertex_count) relative = 0u;
        }
        texel[k] = relative;
      }
    }
    if (has_uv && t < (p.vertex_count + 1u) / 2u) {
      uint32_t* texel = carrier + size_t{uv_texel + t} * 4u;
      for (uint32_t k = 0u; k < 2u; ++k) {
        const uint32_t vertex = 2u * t + k;
        float uv[4] = {};
        if (vertex < p.vertex_count) {
          GcDecodeAttribute(staging, size, p.uv_byte + vertex * p.uv_stride + p.uv_offset, p.uv_format, uv);
        }
        std::memcpy(texel + 2u * k, uv, 2u * sizeof(float));
      }
    }
  }
}

// The largest prefix of `triangles` whose sum is within `budget`; at least one when there is any (a mesh over the
// budget is built alone). 0 for none.
inline uint32_t GcBlasBudgetCount(const std::vector<uint32_t>& triangles, uint64_t budget) {
  if (triangles.empty()) return 0u;
  uint64_t sum = 0u;
  uint32_t count = 0u;
  for (const uint32_t triangle_count : triangles) {
    if (sum + triangle_count > budget) break;
    sum += triangle_count;
    count += 1u;
  }
  return (std::max)(count, 1u);
}

struct GcCompareResult {
  uint32_t triangles = 0u;
  uint32_t mismatched = 0u;
  int64_t first = -1;
  float gpu[9] = {};
  float cpu[9] = {};
  bool uv_compared = false;
  uint32_t uv_mismatched = 0u;
  bool uv_size_mismatch = false;
};

// Bitwise per triangle, in object space (the CPU decode remaps vertices, so indices are not compared).
inline GcCompareResult GcCompareTriangles(const float* gpu_vertices, uint32_t gpu_vertex_count, const uint32_t* gpu_indices,
                                          uint32_t gpu_index_count, const float* gpu_uvs,
                                          const std::vector<std::array<float, 3>>& cpu_positions,
                                          const std::vector<uint32_t>& cpu_indices,
                                          const std::vector<std::array<float, 2>>& cpu_uvs) {
  GcCompareResult result;
  const uint32_t gpu_triangles = gpu_index_count / 3u;
  const uint32_t cpu_triangles = static_cast<uint32_t>(cpu_indices.size() / 3u);
  result.triangles = (std::max)(gpu_triangles, cpu_triangles);
  if (gpu_triangles != cpu_triangles) {
    result.mismatched = result.triangles;
    result.first = 0;
    return result;
  }
  result.uv_size_mismatch = gpu_uvs != nullptr && cpu_uvs.size() != cpu_positions.size();
  result.uv_compared = gpu_uvs != nullptr && !result.uv_size_mismatch;
  const auto same_bits = [](const float* a, const float* b, size_t count) { return std::memcmp(a, b, count * sizeof(float)) == 0; };
  for (uint32_t t = 0u; t < gpu_triangles; ++t) {
    bool bad = false;
    bool uv_bad = false;
    float gpu[9] = {};
    float cpu[9] = {};
    for (uint32_t j = 0u; j < 3u && !bad; ++j) {
      const uint32_t gpu_index = gpu_indices[3u * t + j];
      const uint32_t cpu_index = cpu_indices[3u * t + j];
      if (gpu_index >= gpu_vertex_count || cpu_index >= cpu_positions.size()) {
        bad = true;
        break;
      }
      std::memcpy(gpu + 3u * j, gpu_vertices + size_t{gpu_index} * 4u, 3u * sizeof(float));
      std::memcpy(cpu + 3u * j, cpu_positions[cpu_index].data(), 3u * sizeof(float));
      if (!same_bits(gpu + 3u * j, cpu + 3u * j, 3u)) bad = true;
      if (result.uv_compared && !same_bits(gpu_uvs + size_t{gpu_index} * 2u, cpu_uvs[cpu_index].data(), 2u)) uv_bad = true;
    }
    if (uv_bad) result.uv_mismatched += 1u;
    if (!bad && !uv_bad) continue;
    result.mismatched += 1u;
    if (result.first < 0) {
      result.first = static_cast<int64_t>(t);
      std::memcpy(result.gpu, gpu, sizeof(gpu));
      std::memcpy(result.cpu, cpu, sizeof(cpu));
    }
  }
  return result;
}

// Key of a pose in the GPU instance set (D5): the 12 world floats on a grid (translation 1/256 m, basis 1/4096; round half
// to even, as HLSL round), FNV-style words with a leading word (the GPU mesh slot; the compare uses the CPU mesh id).
// out[0] picks the probe start, out[1] | 1 is the tag. world_gc_insert.cs_5_0.hlsl must match.
inline void GcPoseHash(uint32_t lead, const float* world, uint32_t out[2]) {
  uint32_t h1 = 2166136261u;
  uint32_t h2 = 0x9747B28Cu;
  const auto mix = [&](uint32_t word) {
    h1 = (h1 ^ word) * 16777619u;
    h2 = (h2 ^ word) * 0x5BD1E995u;
    h2 ^= h2 >> 15u;
  };
  mix(lead);
  for (uint32_t i = 0u; i < 12u; ++i) {
    const float scale = (i == 3u || i == 7u || i == 11u) ? kGcPoseTranslationScale : kGcPoseBasisScale;
    mix(static_cast<uint32_t>(static_cast<int32_t>(std::nearbyint(world[i] * scale))));
  }
  h1 ^= h1 >> 16u;
  h1 *= 0x85EBCA6Bu;
  h1 ^= h1 >> 13u;
  h1 *= 0xC2B2AE35u;
  h1 ^= h1 >> 16u;
  h2 ^= h2 >> 16u;
  h2 *= 0x85EBCA6Bu;
  h2 ^= h2 >> 13u;
  h2 *= 0xC2B2AE35u;
  h2 ^= h2 >> 16u;
  out[0] = h1;
  out[1] = h2;
}

struct GcCompareGpuInstance {
  uint64_t mesh_key = 0u;
  int64_t mesh_id = -1;             // CPU mesh of the key, -1 none
  bool cpu_alpha_conflict = false;  // that CPU mesh has an alpha conflict (P-2)
  float world[12] = {};
  uint32_t flags = 0u;              // kInstance* (record visibility.z)
};
struct GcCompareCpuInstance {
  uint64_t mesh_key = 0u;
  uint32_t mesh_id = 0u;
  float world[12] = {};
  bool dynamic = false;        // follow instance (moving object)
  bool recently_seen = false;  // observed within kPoolPruneAge frames
  bool alpha = false;
  uint32_t vs_hash = 0u;
};
struct GcInstanceCompare {
  struct CpuOnly {
    uint64_t mesh_key = 0u;
    uint32_t mesh_id = 0u;
    float translation[3] = {};
    std::string reason;
    bool alpha = false;
    uint32_t vs_hash = 0u;
  };
  struct GpuOnly {
    uint64_t mesh_key = 0u;
    int64_t mesh_id = -1;
    float translation[3] = {};
    uint32_t flags = 0u;
  };
  bool run = false;
  uint64_t set_count = 0u, moving_records = 0u, gpu_records = 0u, cpu_instances = 0u, matched = 0u, gpu_only = 0u;
  uint64_t gpu_only_alpha = 0u, gpu_only_alpha_conflict = 0u, gpu_no_cpu_mesh = 0u, gpu_dup_content = 0u, cpu_dup = 0u;
  uint64_t cpu_only = 0u, timeouts = 0u;
  double wait_ms = 0.0;
  std::map<std::string, uint64_t> cpu_only_by_reason;
  std::vector<CpuOnly> cpu_only_samples;  // up to kGcCompareCpuSamples
  std::vector<GpuOnly> gpu_only_samples;  // up to kGcCompareGpuSamples
};

// D10. `entry_reason`: per CPU mesh key with a G2 entry, "pending", "done" or "refused:<reason>" (absent = "no_entry").
inline GcInstanceCompare GcCompareInstances(const std::vector<GcCompareGpuInstance>& gpu,
                                            const std::vector<GcCompareCpuInstance>& cpu,
                                            const std::unordered_map<uint64_t, std::string>& entry_reason) {
  GcInstanceCompare result;
  result.run = true;
  result.gpu_records = gpu.size();
  result.cpu_instances = cpu.size();
  const auto key_of = [](uint32_t mesh_id, const float* world) {
    uint32_t hash[2] = {};
    GcPoseHash(mesh_id, world, hash);
    return (uint64_t{hash[0]} << 32u) | hash[1];
  };
  std::unordered_set<uint64_t> gpu_keys;
  for (const GcCompareGpuInstance& instance : gpu) {
    if (instance.mesh_id < 0) {
      result.gpu_no_cpu_mesh += 1u;
      continue;
    }
    if (!gpu_keys.insert(key_of(static_cast<uint32_t>(instance.mesh_id), instance.world)).second) result.gpu_dup_content += 1u;
  }
  std::unordered_set<uint64_t> cpu_keys;
  for (const GcCompareCpuInstance& instance : cpu) {
    const uint64_t key = key_of(instance.mesh_id, instance.world);
    if (!cpu_keys.insert(key).second) {
      result.cpu_dup += 1u;
      continue;
    }
    if (gpu_keys.count(key) != 0u) {
      result.matched += 1u;
      continue;
    }
    std::string reason = "dynamic";
    if (!instance.dynamic) {
      const auto it = entry_reason.find(instance.mesh_key);
      reason = (it == entry_reason.end() ? std::string("no_entry") : it->second);
      if (reason == "done" && !instance.recently_seen) reason = "done_unseen";
    }
    result.cpu_only += 1u;
    result.cpu_only_by_reason[reason] += 1u;
    if (result.cpu_only_samples.size() < kGcCompareCpuSamples) {
      result.cpu_only_samples.push_back({.mesh_key = instance.mesh_key, .mesh_id = instance.mesh_id,
                                         .translation = {instance.world[3], instance.world[7], instance.world[11]},
                                         .reason = reason, .alpha = instance.alpha, .vs_hash = instance.vs_hash});
    }
  }
  std::unordered_set<uint64_t> reported;
  for (const GcCompareGpuInstance& instance : gpu) {
    if (instance.mesh_id < 0) continue;
    const uint64_t key = key_of(static_cast<uint32_t>(instance.mesh_id), instance.world);
    if (cpu_keys.count(key) != 0u || !reported.insert(key).second) continue;
    result.gpu_only += 1u;
    if ((instance.flags & 16u) != 0u) result.gpu_only_alpha += 1u;  // kInstanceAlphaTested (bvh_resources.hpp)
    if (instance.cpu_alpha_conflict) result.gpu_only_alpha_conflict += 1u;
    if (result.gpu_only_samples.size() < kGcCompareGpuSamples) {
      result.gpu_only_samples.push_back({.mesh_key = instance.mesh_key, .mesh_id = instance.mesh_id,
                                         .translation = {instance.world[3], instance.world[7], instance.world[11]},
                                         .flags = instance.flags});
    }
  }
  return result;
}
