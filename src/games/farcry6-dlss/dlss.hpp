/*
 * Copyright (C) 2026
 * SPDX-License-Identifier: MIT
 *
 * Far Cry 6 DLSS — replaces the game's TAA passes with NVIDIA DLSS Super
 * Resolution (D3D12 NGX path).
 *
 * Target passes (reference HLSL in antialiasing/, never injected):
 *   0xFF0AF25B — main TAA (single render target)
 *   0x4AD8172A — pause-menu TAA variant (two render targets; the second
 *                carries alpha/coverage history and is left untouched)
 *
 * TAA resource mapping (derived from the reference shader decode):
 *   t0 (float2) — packed current-frame motion vectors (custom piecewise
 *                 encoding, UV units scaled by 0.1). Decoded on the GPU by
 *                 dlss_mv_unpack.cs_6_0 into pixel-unit RG16F MVs for NGX.
 *   t1 (float2) — previous-frame MVs, used by TAA only for its consistency
 *                 check. Not consumed by DLSS.
 *   t2 (float4) — current scene color (DLSS pInColor).
 *   t3/t4       — TAA history (DLSS keeps its own history; not consumed).
 *   t5/t6 (float) — single-channel buffers sampled at current/reprojected
 *                 UVs. Depth candidate for DLSS pInDepth (selectable).
 *   b0 c011     — TAA blend knobs. The full b0 range is also scanned for a
 *                 per-frame varying small-magnitude float pair to recover the
 *                 game's render jitter for NGX (falls back to zero).
 */

#pragma once

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <include/reshade.hpp>
#include <nvsdk_ngx.h>
#include <nvsdk_ngx_helpers.h>
#include <wrl/client.h>

#include "../../utils/bitwise.hpp"
#include "../../utils/pipeline_layout.hpp"
#include "../../utils/resource.hpp"
#include "../../utils/shader.hpp"

namespace farcry6_dlss::dlss {

static constexpr uint32_t kTaaMainHash = 0xFF0AF25Bu;
static constexpr uint32_t kTaaPauseHash = 0x4AD8172Au;
static constexpr uint32_t kTrackedTableCount = 32u;
static constexpr uint32_t kTrackedCbvCount = 16u;
static constexpr uint32_t kTrackedSrvCount = 16u;
static constexpr uint32_t kTrackedRtvCount = 8u;

// Game TAA slots (space0) consumed by DLSS.
static constexpr uint32_t kSlotPackedMv = 0u;  // t0: packed motion vectors
static constexpr uint32_t kSlotColor = 2u;     // t2: current scene color

struct __declspec(uuid("7f3e2a1c-9b4d-4e8f-a6c2-d5f1b0e98341")) CommandListData {
  std::array<reshade::api::descriptor_table, kTrackedTableCount> descriptor_tables = {};
  std::array<reshade::api::buffer_range, kTrackedCbvCount> constant_buffers = {};
  std::array<reshade::api::resource_view, kTrackedSrvCount> push_srvs = {};
  std::array<reshade::api::resource_view, kTrackedRtvCount> rtvs = {};
  reshade::api::resource_view dsv = {};
  // Memoized pixel-shader hash: refreshed once per pipeline bind instead of
  // once per draw (binds are ~100x rarer than draws at Dunia draw counts).
  uint32_t cached_ps_hash = 0u;
  bool ps_hash_valid = false;
  bool ps_dirty = true;
};

// ── Settings globals (written by addon.cpp settings system) ──
inline float dlss_enabled = 1.f;
// 0=Auto, 1=DLAA, 2=Quality, 3=Balanced, 4=Performance, 5=UltraPerformance
inline float dlss_mode = 0.f;
// DLSS preset: 0=Default, 1=F, 2=J, 3=K, 4=L, 5=M
inline float dlss_render_preset = 0.f;
inline float dlss_motion_vectors_jittered = 1.f;
inline float dlss_depth_inverted = 0.f;
// 0=t5 (current-UV buffer), 1=t6 (reprojected-UV buffer)
inline float dlss_depth_source = 0.f;
// 0=Zero (safe), 1=Auto (scan b0 for the game's jitter, fall back to zero)
inline float dlss_jitter_source = 1.f;
// Calibration multiplier for the unpacked motion vectors.
inline float dlss_mv_scale = 1.f;
inline float dlss_debug_logging = 0.f;
// Experimental fallback: run DLSS even when some TAA inputs don't resolve,
// using zero-motion / far-plane stand-ins. Image will be softer than TAA.
// Pipeline testing only; default off (unresolved inputs keep vanilla TAA).
inline float dlss_allow_fallback = 0.f;

struct InstanceData {
  Microsoft::WRL::ComPtr<ID3D12Device> device;
  Microsoft::WRL::ComPtr<ID3D12Resource> output_texture;
  NVSDK_NGX_Parameter* capability_parameters = nullptr;
  NVSDK_NGX_Parameter* runtime_parameters = nullptr;
  NVSDK_NGX_Handle* feature = nullptr;
  uint32_t width = 0u;   // target (output) width
  uint32_t height = 0u;  // target (output) height
  uint32_t input_width = 0u;
  uint32_t input_height = 0u;
  DXGI_FORMAT output_format = DXGI_FORMAT_UNKNOWN;
  int requested_render_preset = -1;
  int render_preset = -1;
  int perf_quality = -1;
  int feature_flags = 0;
  bool initialized = false;
  bool supported = false;
  bool init_failed = false;
  bool create_failed = false;
  bool eval_failed = false;
  bool reset = true;
  bool logged_success = false;
  bool logged_jitter = false;
};

inline InstanceData ngx;

// ── MV-unpack pass resources (decodes t0 packing to RG16F pixel MVs) ──
struct UnpackData {
  reshade::api::pipeline_layout layout = {};
  reshade::api::descriptor_table tables[2] = {};
  reshade::api::pipeline pipeline = {};
  reshade::api::resource texture = {};
  reshade::api::resource_view srv = {};
  reshade::api::resource_view uav = {};
  uint32_t width = 0u;
  uint32_t height = 0u;
  bool layout_ready = false;
};

inline UnpackData unpack;
inline reshade::api::device* unpack_device = nullptr;

// ── Fallback stand-in resources (experimental degraded mode) ──
// Zero-motion MVs (RG16F) and far-plane depth (R32F), sized to the DLSS input.
// Well-formed by construction, so they bypass the sanity guards that fence
// stale journal entries. Created on demand, released with the device.
struct FallbackData {
  reshade::api::resource mv_texture = {};
  reshade::api::resource depth_texture = {};
  uint32_t width = 0u;
  uint32_t height = 0u;
};

inline FallbackData fallback;

inline void ReleaseFallback(reshade::api::device* device) {
  if (device == nullptr) return;
  if (fallback.mv_texture.handle) device->destroy_resource(fallback.mv_texture);
  if (fallback.depth_texture.handle) device->destroy_resource(fallback.depth_texture);
  fallback = {};
}

inline bool EnsureFallback(reshade::api::device* device, uint32_t width, uint32_t height) {
  if (device == nullptr || width == 0u || height == 0u) return false;
  if (fallback.mv_texture.handle != 0u && fallback.width == width && fallback.height == height) return true;
  ReleaseFallback(device);
  // Explicit initial data (GPU memory is not zero-filled): zero motion and
  // far-plane depth. See below.
  reshade::api::resource_desc mv_rd = {};
  mv_rd.type = reshade::api::resource_type::texture_2d;
  mv_rd.texture = {width, height, 1, 1, reshade::api::format::r16g16_float, 1};
  mv_rd.heap = reshade::api::memory_heap::gpu_only;
  mv_rd.usage = reshade::api::resource_usage::shader_resource;
  reshade::api::resource_desc dep_rd = {};
  dep_rd.type = reshade::api::resource_type::texture_2d;
  dep_rd.texture = {width, height, 1, 1, reshade::api::format::r32_float, 1};
  dep_rd.heap = reshade::api::memory_heap::gpu_only;
  dep_rd.usage = reshade::api::resource_usage::shader_resource;
  // Zero motion (0.0f) initial data: "no motion anywhere" signal.
  std::vector<uint16_t> zero_mv(static_cast<size_t>(width) * height * 2u, 0u);
  reshade::api::subresource_data mv_init = {};
  mv_init.data = zero_mv.data();
  mv_init.row_pitch = static_cast<uint32_t>(static_cast<size_t>(width) * 2u * sizeof(uint16_t));
  mv_init.slice_pitch = static_cast<uint32_t>(zero_mv.size() * sizeof(uint16_t));
  // Far plane (1.0f) initial data for depth: minimal disocclusion influence.
  std::vector<float> far_plane(static_cast<size_t>(width) * height, 1.0f);
  reshade::api::subresource_data dep_init = {};
  dep_init.data = far_plane.data();
  dep_init.row_pitch = static_cast<uint32_t>(static_cast<size_t>(width) * sizeof(float));
  dep_init.slice_pitch = static_cast<uint32_t>(far_plane.size() * sizeof(float));
  if (!device->create_resource(mv_rd, &mv_init, reshade::api::resource_usage::shader_resource, &fallback.mv_texture)
      || fallback.mv_texture.handle == 0u) {
    ReleaseFallback(device);
    return false;
  }
  if (!device->create_resource(dep_rd, &dep_init, reshade::api::resource_usage::shader_resource, &fallback.depth_texture)
      || fallback.depth_texture.handle == 0u) {
    ReleaseFallback(device);
    return false;
  }
  fallback.width = width;
  fallback.height = height;
  return true;
}

// ── Jitter auto-detect state (scans b0 for a per-frame varying pair) ──
struct JitterState {
  std::vector<float> prev;
  int candidate = -1;
  int streak = 0;
  bool adopted = false;
  float x = 0.f;
  float y = 0.f;
  bool logged_adopt = false;
};

inline JitterState jitter_state;

// ── Multi-adapter device tracking ──
// The engine may expose non-NVIDIA devices (e.g. Microsoft Basic Render
// Driver, vendor 0x1414) alongside the real GPU. Support is per-device and
// sticky: a non-NVIDIA init must never disable an NVIDIA device.
inline std::mutex device_mutex;
inline std::unordered_set<uint64_t> nvidia_devices;
inline std::atomic<bool> any_nvidia{false};
inline std::string device_summary = "none seen";
inline bool attached = false;

// ── Failure codes for the always-visible status line ──
static constexpr int kFailNone = 0;
static constexpr int kFailDevice = 1;
static constexpr int kFailB0 = 2;
static constexpr int kFailResolve = 3;
static constexpr int kFailSize = 4;
static constexpr int kFailFormat = 5;
static constexpr int kFailNgxInit = 6;
static constexpr int kFailFeature = 7;
static constexpr int kFailUnpack = 8;
static constexpr int kFailEval = 9;

inline const char* FailCodeText(int code) {
  switch (code) {
    case kFailNone: return "none yet";
    case kFailDevice: return "no NVIDIA device";
    case kFailB0: return "b0 constants not captured";
    case kFailResolve: return "TAA resources not resolved (learning)";
    case kFailSize: return "input/target size mismatch";
    case kFailFormat: return "unsupported TAA target format";
    case kFailNgxInit: return "NGX init failed (see log)";
    case kFailFeature: return "NGX feature creation failed (see log)";
    case kFailUnpack: return "MV unpack failed (see log)";
    case kFailEval: return "NGX evaluation failed (see log)";
    default: return "unknown";
  }
}

// ── Runtime counters (atomics; drawn by the status UI) ──
inline std::atomic<uint64_t> stat_taa_draws{0};
inline std::atomic<uint64_t> stat_dlss_ok{0};
inline std::atomic<int> stat_fail_code{kFailNone};
inline std::atomic<int> stat_resolve_fail_streak{0};
inline std::atomic<bool> resolved_once{false};
inline std::atomic<int> full_capture_frames{0};
inline std::atomic<uint64_t> stat_presents{0};
inline std::atomic<uint64_t> last_arm_present{0};
static constexpr uint64_t kRearmCooldownPresents = 600u;

inline void NoteFail(int code) { stat_fail_code.store(code); }
inline void NoteSuccess() {
  ++stat_dlss_ok;
  stat_resolve_fail_streak.store(0);
  stat_fail_code.store(kFailNone);
  // First success switches the journal from attach-time record-all to
  // targeted (learned-table) recording. Boot entries are KEPT (heap writes
  // may be one-time); the interest gate only limits what gets added.
  resolved_once.store(true);
}

// ── Scoped TAA descriptor capture ──
// The game binds TAA inputs through descriptor tables. Instead of the global
// descriptor-table mirror (known FPS killer), we enumerate every layout
// (table, binding) candidate for each slot of interest at TAA draws and copy
// ONLY overlapping updates. Layout ranges carry no descriptor types, so
// b0/t0-style (register 0, space 0) collisions are disambiguated by the
// recorded update type. Push-bound slots need no capture at all.
static constexpr uint32_t kLearnedSlots = 5u;  // b0, t0, t2, t5, t6
static constexpr uint32_t kMaxCandidates = 3u;
// Last-TAA-draw per-slot resolve results, for the Status readout only.
// 0=miss, 1=real journal/push entry, 2=fallback dummy. Written by the
// resolvers / EvaluateDLSS, read by the overlay.
inline std::atomic<int> slot_hit[kLearnedSlots];
inline bool logged_fallback = false;
static constexpr uint32_t kMaxInterestTables = 32u;

inline uint32_t LearnIndexForSlot(uint32_t slot, bool is_cbv) {
  if (is_cbv) return (slot == 0u) ? 0u : UINT32_MAX;
  switch (slot) {
    case 0u: return 1u;
    case 2u: return 2u;
    case 5u: return 3u;
    case 6u: return 4u;
    default: return UINT32_MAX;
  }
}

struct LearnedCandidates {
  uint64_t heaps[kMaxCandidates] = {};
  uint32_t offsets[kMaxCandidates] = {};
  uint32_t count = 0u;
  bool is_push = false;
};
inline LearnedCandidates learned[kLearnedSlots];
// Lock-free interest set for the update/copy hooks (rebuilt after each
// learn): decomposed (heap, offset) pairs so range overlap is O(interest)
// with zero hashing on the hot path.
inline std::atomic<uint64_t> interest_heaps[kMaxInterestTables];
inline std::atomic<uint32_t> interest_offs[kMaxInterestTables];
inline std::atomic<uint32_t> interest_count{0u};

// Raw-handle -> (heap, base-offset) cache (declared early: ClearCapture
// below resets it). See RawCacheLookup for the consistency protocol.
static constexpr uint32_t kRawCacheSize = 128u;
inline std::atomic<uint64_t> raw_cache_raw[kRawCacheSize];
inline std::atomic<uint64_t> raw_cache_heap[kRawCacheSize];
inline std::atomic<uint32_t> raw_cache_base[kRawCacheSize];

struct RecordedDesc {
  reshade::api::descriptor_type type = static_cast<reshade::api::descriptor_type>(0);
  bool has_type = false;
  reshade::api::resource_view view = {};
  reshade::api::buffer_range cbv = {};
  bool has_view = false;
  bool has_cbv = false;
};
// Attach-time descriptor journal. Records from process attach (so boot-time
// heap writes are captured), pre-reserved and hard-capped: no ReShade calls,
// no vector resizes, tiny mutex critical sections on the hot path. After the
// first successful TAA resolve the journal is cleared and only targeted
// (learned-table) updates are recorded.
static constexpr size_t kSeenReserve = 262144u;
static constexpr size_t kSeenCap = 262144u;
inline std::mutex capture_mutex;
inline std::unordered_map<uint64_t, RecordedDesc> seen;
inline bool seen_reserved = false;
inline std::atomic<size_t> seen_count{0};
inline bool logged_seen_cap = false;
// Native device owning the journaled heaps (0 = unset). Set on first insert;
// a *different* native at init time means fresh heap objects, so the journal
// is wiped. Same-native re-init (wrapper churn) keeps it.
inline std::atomic<uint64_t> capture_device_native{0};
inline std::atomic<uint64_t> last_evict_present{0};
// Last input/output native handles: any change forces an NGX history reset
// (kills history poisoning when heap slots recycle across resources).
inline uint64_t last_in_color = 0u;
inline uint64_t last_in_mv = 0u;
inline uint64_t last_in_depth = 0u;
inline uint64_t last_in_output = 0u;
inline bool logged_size_guard = false;

// Known NVIDIA device natives for lock-free draw gating (≤4 devices).
inline std::atomic<uint64_t> nvidia_devs[4];

// Every device ReShade initializes (any vendor), for fail-open journal
// filtering. Heap handles are native-unique, so foreign heaps can never
// collide with TAA keys — while a strict NVIDIA-only filter here would
// silently discard writes on any unexpected wrapper (total blindness). Draws
// themselves stay strictly NVIDIA-gated.
inline std::atomic<uint64_t> seen_devs[8];

inline void NoteSeenDevice(uint64_t native) {
  if (native == 0u) return;
  for (uint32_t i = 0u; i < 8u; ++i) {
    if (seen_devs[i].load() == native) return;
  }
  for (uint32_t i = 0u; i < 8u; ++i) {
    uint64_t empty = 0u;
    if (seen_devs[i].compare_exchange_strong(empty, native)) return;
  }
}

inline void ForgetSeenDevice(uint64_t native) {
  if (native == 0u) return;
  for (uint32_t i = 0u; i < 8u; ++i) {
    if (seen_devs[i].load() == native) seen_devs[i].store(0u);
  }
}

inline bool IsSeenDevice(uint64_t native) {
  if (native == 0u) return false;
  for (uint32_t i = 0u; i < 8u; ++i) {
    if (seen_devs[i].load() == native) return true;
  }
  return false;
}

inline bool IsKnownNvidiaNative(uint64_t native) {
  if (native == 0u) return false;
  for (uint32_t i = 0u; i < 4u; ++i) {
    if (nvidia_devs[i].load() == native) return true;
  }
  return false;
}

inline void AddNvidiaNative(uint64_t native) {
  if (native == 0u || IsKnownNvidiaNative(native)) return;
  for (uint32_t i = 0u; i < 4u; ++i) {
    uint64_t empty = 0u;
    if (nvidia_devs[i].compare_exchange_strong(empty, native)) return;
  }
}

inline void RemoveNvidiaNative(uint64_t native) {
  if (native == 0u) return;
  for (uint32_t i = 0u; i < 4u; ++i) {
    if (nvidia_devs[i].load() == native) nvidia_devs[i].store(0u);
  }
}

inline void ClearCapture() {
  for (auto& l : learned) l = {};
  for (uint32_t i = 0u; i < kMaxInterestTables; ++i) {
    interest_heaps[i].store(0u);
    interest_offs[i].store(0u);
  }
  interest_count.store(0u);
  seen.clear();
  seen_reserved = false;
  capture_device_native.store(0u);
  for (uint32_t i = 0u; i < kRawCacheSize; ++i) raw_cache_raw[i].store(0u);
  full_capture_frames.store(0);
  resolved_once.store(false);
}

inline uint64_t HeapKey(uint64_t heap, uint32_t offset) {
  return heap ^ (static_cast<uint64_t>(offset) * 0x9E3779B97F4A7C15ull);
}

// Canonicalizes a (table, binding) pair to the backing (heap, offset).
// Required because ReShade reports native view-creation writes as
// {0xF000..|CPUptr}/binding 0 while binds use raw GPU handles: the two forms
// only meet in (heap, offset) space (see get_descriptor_heap_offset, which
// handles both). Returns false for unbound/unknown tables (skip gracefully).
inline bool CanonicalBinding(
    reshade::api::device* device,
    reshade::api::descriptor_table table,
    uint32_t binding,
    uint64_t& heap,
    uint32_t& offset) {
  heap = 0u;
  offset = 0u;
  if (device == nullptr || table.handle == 0u) return false;
  reshade::api::descriptor_heap h = {};
  uint32_t off = 0u;
  device->get_descriptor_heap_offset(table, binding, 0u, &h, &off);
  if (h.handle == 0u) return false;
  heap = h.handle;
  offset = off;
  return true;
}

// Raw-handle -> (heap, base-offset) cache. Steady-state updates hit here
// with a few atomic loads and zero ReShade calls; a miss costs exactly one
// canonical query and then populates the slot. Slots are validated by raw
// match (hash collisions overwrite and self-heal via re-query). The writer
// zeroes raw first so torn triples can only read as misses, never corrupt.
inline uint32_t RawSlot(uint64_t raw) {
  uint64_t h = raw ^ (raw >> 33);
  h *= 0x9E3779B97F4A7C15ull;
  return static_cast<uint32_t>((h ^ (h >> 29)) % kRawCacheSize);
}

inline bool RawCacheLookup(uint64_t raw, uint64_t& heap, uint32_t& base) {
  if (raw == 0u) return false;
  const uint32_t i = RawSlot(raw);
  const uint64_t r1 = raw_cache_raw[i].load();
  if (r1 != raw) return false;
  heap = raw_cache_heap[i].load();
  base = raw_cache_base[i].load();
  return raw_cache_raw[i].load() == r1 && heap != 0u;
}

inline void RawCacheStore(uint64_t raw, uint64_t heap, uint32_t base) {
  if (raw == 0u || heap == 0u) return;
  const uint32_t i = RawSlot(raw);
  raw_cache_raw[i].store(0u);
  raw_cache_heap[i].store(heap);
  raw_cache_base[i].store(base);
  raw_cache_raw[i].store(raw);
}

// Cached canonicalization: hot path is lock-free and call-free.
inline bool CanonicalBindingCached(
    reshade::api::device* device,
    reshade::api::descriptor_table table,
    uint32_t binding,
    uint64_t& heap,
    uint32_t& offset) {
  heap = 0u;
  offset = 0u;
  if (table.handle == 0u) return false;
  uint64_t h = 0u;
  uint32_t b = 0u;
  if (RawCacheLookup(table.handle, h, b)) {
    heap = h;
    offset = b + binding;
    return true;
  }
  if (!CanonicalBinding(device, table, 0u, h, b)) return false;
  RawCacheStore(table.handle, h, b);
  heap = h;
  offset = b + binding;
  return true;
}

inline void RebuildInterest() {
  uint32_t n = 0u;
  for (uint32_t i = 0u; i < kLearnedSlots && n < kMaxInterestTables; ++i) {
    for (uint32_t k = 0u; k < learned[i].count && n < kMaxInterestTables; ++k) {
      const uint64_t h = learned[i].heaps[k];
      const uint32_t o = learned[i].offsets[k];
      if (h == 0u) continue;
      bool dup = false;
      for (uint32_t j = 0u; j < n; ++j) {
        if (interest_heaps[j].load() == h && interest_offs[j].load() == o) {
          dup = true;
          break;
        }
      }
      if (dup) continue;
      interest_heaps[n].store(h);
      interest_offs[n].store(o);
      ++n;
    }
  }
  interest_count.store(n);
}

inline bool logged_taa_draw_detected = false;
inline bool logged_missing_constants = false;
inline bool logged_missing_resources = false;
inline bool logged_unsupported_output = false;
inline bool logged_b0_map_failed = false;
inline std::chrono::steady_clock::time_point last_present_time{};

inline CommandListData* Get(reshade::api::command_list* cmd_list) {
  if (cmd_list == nullptr) return nullptr;
  auto* data = cmd_list->get_private_data<CommandListData>();
  return data != nullptr ? data : cmd_list->create_private_data<CommandListData>();
}

inline std::wstring GetProcessDirectory() {
  std::array<wchar_t, MAX_PATH> path = {};
  const DWORD length = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
  if (length == 0u || length >= path.size()) return L".";
  std::wstring result(path.data(), length);
  const auto separator = result.find_last_of(L"\\/");
  return separator == std::wstring::npos ? L"." : result.substr(0u, separator);
}

inline const char* ResultToString(NVSDK_NGX_Result result) {
  switch (result) {
    case NVSDK_NGX_Result_Success: return "Success";
    case NVSDK_NGX_Result_FAIL_FeatureNotSupported: return "FeatureNotSupported";
    case NVSDK_NGX_Result_FAIL_PlatformError: return "PlatformError";
    case NVSDK_NGX_Result_FAIL_FeatureAlreadyExists: return "FeatureAlreadyExists";
    case NVSDK_NGX_Result_FAIL_FeatureNotFound: return "FeatureNotFound";
    case NVSDK_NGX_Result_FAIL_InvalidParameter: return "InvalidParameter";
    case NVSDK_NGX_Result_FAIL_ScratchBufferTooSmall: return "ScratchBufferTooSmall";
    case NVSDK_NGX_Result_FAIL_NotInitialized: return "NotInitialized";
    case NVSDK_NGX_Result_FAIL_UnsupportedInputFormat: return "UnsupportedInputFormat";
    case NVSDK_NGX_Result_FAIL_RWFlagMissing: return "RWFlagMissing";
    case NVSDK_NGX_Result_FAIL_MissingInput: return "MissingInput";
    case NVSDK_NGX_Result_FAIL_UnableToInitializeFeature: return "UnableToInitializeFeature";
    case NVSDK_NGX_Result_FAIL_OutOfDate: return "OutOfDate";
    case NVSDK_NGX_Result_FAIL_OutOfGPUMemory: return "OutOfGPUMemory";
    case NVSDK_NGX_Result_FAIL_UnsupportedFormat: return "UnsupportedFormat";
    case NVSDK_NGX_Result_FAIL_UnableToWriteToAppDataPath: return "UnableToWriteToAppDataPath";
    case NVSDK_NGX_Result_FAIL_UnsupportedParameter: return "UnsupportedParameter";
    case NVSDK_NGX_Result_FAIL_Denied: return "Denied";
    case NVSDK_NGX_Result_FAIL_NotImplemented: return "NotImplemented";
    default: return "Unknown";
  }
}

inline bool IsSupported() { return any_nvidia.load() && (!ngx.initialized || ngx.supported); }

inline std::string DeviceSummary() {
  std::lock_guard<std::mutex> lock(device_mutex);
  return device_summary;
}

inline void ReleaseFeatureHandleOnly() {
  if (ngx.feature != nullptr) {
    NVSDK_NGX_D3D12_ReleaseFeature(ngx.feature);
    ngx.feature = nullptr;
  }
  if (ngx.runtime_parameters != nullptr) {
    NVSDK_NGX_D3D12_DestroyParameters(ngx.runtime_parameters);
    ngx.runtime_parameters = nullptr;
  }
  ngx.requested_render_preset = -1;
  ngx.render_preset = -1;
  ngx.perf_quality = -1;
  ngx.feature_flags = 0;
  ngx.create_failed = false;
  ngx.eval_failed = false;
  ngx.reset = true;
  ngx.logged_success = false;
  ngx.logged_jitter = false;
}

inline void ReleaseFeature() {
  ReleaseFeatureHandleOnly();
  ngx.output_texture.Reset();
  ngx.width = 0u;
  ngx.height = 0u;
  ngx.input_width = 0u;
  ngx.input_height = 0u;
  ngx.output_format = DXGI_FORMAT_UNKNOWN;
}

inline void ReleaseUnpack(reshade::api::device* device) {
  if (device == nullptr) return;
  if (unpack.uav.handle) device->destroy_resource_view(unpack.uav);
  if (unpack.srv.handle) device->destroy_resource_view(unpack.srv);
  if (unpack.texture.handle) device->destroy_resource(unpack.texture);
  if (unpack.pipeline.handle) device->destroy_pipeline(unpack.pipeline);
  for (auto& table : unpack.tables) {
    if (table.handle) device->free_descriptor_table(table);
  }
  if (unpack.layout.handle) device->destroy_pipeline_layout(unpack.layout);
  unpack = {};
}

inline void ReleaseNgx() {
  if (unpack_device != nullptr) {
    ReleaseUnpack(unpack_device);
    ReleaseFallback(unpack_device);
    unpack_device = nullptr;
  }
  ReleaseFeature();
  if (ngx.capability_parameters != nullptr) {
    NVSDK_NGX_D3D12_DestroyParameters(ngx.capability_parameters);
    ngx.capability_parameters = nullptr;
  }
  if (ngx.initialized) {
    NVSDK_NGX_D3D12_Shutdown1(ngx.device.Get());
  }
  ngx.device.Reset();
  ngx.initialized = false;
  ngx.supported = false;
  ngx.init_failed = false;
  jitter_state = {};
  stat_resolve_fail_streak.store(0);
  logged_taa_draw_detected = false;
  logged_missing_constants = false;
  logged_missing_resources = false;
  logged_unsupported_output = false;
  logged_b0_map_failed = false;
}

inline bool EnsureNgxInitialized(reshade::api::device* device) {
  if (ngx.initialized) return ngx.supported;
  if (ngx.init_failed || device == nullptr || device->get_api() != reshade::api::device_api::d3d12) return false;

  auto* native_device = reinterpret_cast<ID3D12Device*>(device->get_native());
  if (native_device == nullptr) return false;

  const std::wstring process_directory = GetProcessDirectory();
  wchar_t* feature_paths[] = {const_cast<wchar_t*>(process_directory.c_str())};
  NVSDK_NGX_FeatureCommonInfo feature_info = {};
  feature_info.PathListInfo.Length = 1u;
  feature_info.PathListInfo.Path = feature_paths;

  const NVSDK_NGX_Result init_result = NVSDK_NGX_D3D12_Init_with_ProjectID(
      "a3f4c8e1-7b2d-4f6a-9c1e-5d8b0f2a6c4e",
      NVSDK_NGX_ENGINE_TYPE_CUSTOM,
      "1.0",
      process_directory.c_str(),
      native_device,
      &feature_info,
      NVSDK_NGX_Version_API);
  if (NVSDK_NGX_FAILED(init_result)) {
    ngx.init_failed = true;
    std::stringstream s;
    s << "Far Cry 6 DLSS: NGX init failed: " << ResultToString(init_result)
      << " (0x" << std::hex << static_cast<uint32_t>(init_result) << ")";
    reshade::log::message(reshade::log::level::error, s.str().c_str());
    return false;
  }

  const NVSDK_NGX_Result params_result = NVSDK_NGX_D3D12_GetCapabilityParameters(&ngx.capability_parameters);
  if (NVSDK_NGX_FAILED(params_result) || ngx.capability_parameters == nullptr) {
    ngx.init_failed = true;
    std::stringstream s;
    s << "Far Cry 6 DLSS: NGX capability parameters failed: " << ResultToString(params_result)
      << " (0x" << std::hex << static_cast<uint32_t>(params_result) << ")";
    reshade::log::message(reshade::log::level::error, s.str().c_str());
    NVSDK_NGX_D3D12_Shutdown1(native_device);
    return false;
  }

  int super_sampling_available = 0;
  ngx.capability_parameters->Get(NVSDK_NGX_EParameter_SuperSampling_Available, &super_sampling_available);
  ngx.supported = super_sampling_available > 0;
  if (!ngx.supported) {
    reshade::log::message(reshade::log::level::warning, "Far Cry 6 DLSS: NGX reports DLSS/DLAA is not supported");
  }

  ngx.device = native_device;
  ngx.initialized = true;
  unpack_device = device;
  reshade::log::message(reshade::log::level::info, "Far Cry 6 DLSS: NGX initialized");
  return ngx.supported;
}

inline bool EnsureOutputTexture(ID3D12Device* device, uint32_t width, uint32_t height, DXGI_FORMAT format) {
  if (device == nullptr || width == 0u || height == 0u || format == DXGI_FORMAT_UNKNOWN) return false;
  if (ngx.output_texture != nullptr && ngx.width == width && ngx.height == height && ngx.output_format == format) return true;

  ReleaseFeature();

  D3D12_HEAP_PROPERTIES heap_props = {};
  heap_props.Type = D3D12_HEAP_TYPE_DEFAULT;
  heap_props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
  heap_props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
  heap_props.CreationNodeMask = 1u;
  heap_props.VisibleNodeMask = 1u;

  D3D12_RESOURCE_DESC desc = {};
  desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
  desc.Alignment = 0u;
  desc.Width = width;
  desc.Height = height;
  desc.DepthOrArraySize = 1u;
  desc.MipLevels = 1u;
  desc.Format = format;
  desc.SampleDesc.Count = 1u;
  desc.SampleDesc.Quality = 0u;
  desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
  desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS | D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

  const HRESULT hr = device->CreateCommittedResource(
      &heap_props,
      D3D12_HEAP_FLAG_NONE,
      &desc,
      D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
      nullptr,
      IID_PPV_ARGS(ngx.output_texture.ReleaseAndGetAddressOf()));
  if (FAILED(hr)) {
    std::stringstream s;
    s << "Far Cry 6 DLSS: output texture creation failed: 0x" << std::hex << static_cast<uint32_t>(hr);
    reshade::log::message(reshade::log::level::error, s.str().c_str());
    return false;
  }

  ngx.width = width;
  ngx.height = height;
  ngx.output_format = format;
  return true;
}

inline int GetRenderPresetValue() {
  switch (static_cast<int>(dlss_render_preset)) {
    case 1: return static_cast<int>(NVSDK_NGX_DLSS_Hint_Render_Preset_F);
    case 2: return static_cast<int>(NVSDK_NGX_DLSS_Hint_Render_Preset_J);
    case 3: return static_cast<int>(NVSDK_NGX_DLSS_Hint_Render_Preset_K);
    case 4: return static_cast<int>(NVSDK_NGX_DLSS_Hint_Render_Preset_L);
    case 5: return static_cast<int>(NVSDK_NGX_DLSS_Hint_Render_Preset_M);
    default: return static_cast<int>(NVSDK_NGX_DLSS_Hint_Render_Preset_Default);
  }
}

inline int GetFeatureFlags() {
  // Full-resolution packed MVs are decoded to pixel units by the unpack pass,
  // so MVLowRes must NOT be set. The scene is rendered jittered, so MVs span
  // jittered frames (MVJittered, toggleable for A/B).
  int flags = 0;
  if (dlss_depth_inverted != 0.f) flags |= NVSDK_NGX_DLSS_Feature_Flags_DepthInverted;
  if (dlss_motion_vectors_jittered != 0.f) flags |= NVSDK_NGX_DLSS_Feature_Flags_MVJittered;
  return flags;
}

inline const char* GetRenderPresetName(int preset) {
  switch (preset) {
    case static_cast<int>(NVSDK_NGX_DLSS_Hint_Render_Preset_F): return "F";
    case static_cast<int>(NVSDK_NGX_DLSS_Hint_Render_Preset_J): return "J";
    case static_cast<int>(NVSDK_NGX_DLSS_Hint_Render_Preset_K): return "K";
    case static_cast<int>(NVSDK_NGX_DLSS_Hint_Render_Preset_L): return "L";
    case static_cast<int>(NVSDK_NGX_DLSS_Hint_Render_Preset_M): return "M";
    default: return "Default";
  }
}

inline const char* GetPerfQualityName(int quality) {
  switch (quality) {
    case static_cast<int>(NVSDK_NGX_PerfQuality_Value_DLAA): return "DLAA";
    case static_cast<int>(NVSDK_NGX_PerfQuality_Value_UltraQuality): return "UltraQuality";
    case static_cast<int>(NVSDK_NGX_PerfQuality_Value_MaxQuality): return "Quality";
    case static_cast<int>(NVSDK_NGX_PerfQuality_Value_Balanced): return "Balanced";
    case static_cast<int>(NVSDK_NGX_PerfQuality_Value_MaxPerf): return "Performance";
    case static_cast<int>(NVSDK_NGX_PerfQuality_Value_UltraPerformance): return "UltraPerformance";
    default: return "Unknown";
  }
}

inline int GetPerfQualityValue(uint32_t input_width, uint32_t input_height, uint32_t target_width, uint32_t target_height) {
  const int mode = static_cast<int>(dlss_mode);
  if (mode == 1) return static_cast<int>(NVSDK_NGX_PerfQuality_Value_DLAA);
  if (mode == 2) return static_cast<int>(NVSDK_NGX_PerfQuality_Value_MaxQuality);
  if (mode == 3) return static_cast<int>(NVSDK_NGX_PerfQuality_Value_Balanced);
  if (mode == 4) return static_cast<int>(NVSDK_NGX_PerfQuality_Value_MaxPerf);
  if (mode == 5) return static_cast<int>(NVSDK_NGX_PerfQuality_Value_UltraPerformance);
  // Auto: native-size inputs run DLAA; smaller inputs (engine render scaling)
  // run Quality upscaling to the display-size target.
  if (input_width == target_width && input_height == target_height) {
    return static_cast<int>(NVSDK_NGX_PerfQuality_Value_DLAA);
  }
  return static_cast<int>(NVSDK_NGX_PerfQuality_Value_MaxQuality);
}

inline void SetRenderPresetParameters(NVSDK_NGX_Parameter* parameters, int render_preset) {
  if (parameters == nullptr) return;
  NVSDK_NGX_Parameter_SetUI(parameters, NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_DLAA, render_preset);
  NVSDK_NGX_Parameter_SetUI(parameters, NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Quality, render_preset);
  NVSDK_NGX_Parameter_SetUI(parameters, NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Balanced, render_preset);
  NVSDK_NGX_Parameter_SetUI(parameters, NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Performance, render_preset);
  NVSDK_NGX_Parameter_SetUI(parameters, NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_UltraPerformance, render_preset);
  NVSDK_NGX_Parameter_SetUI(parameters, NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_UltraQuality, render_preset);
}

inline bool EnsureFeature(
    ID3D12GraphicsCommandList* command_list,
    uint32_t input_width, uint32_t input_height,
    uint32_t target_width, uint32_t target_height,
    int render_preset, int feature_flags, int perf_quality) {
  if (command_list == nullptr || ngx.capability_parameters == nullptr || ngx.output_texture == nullptr) return false;
  if (ngx.feature != nullptr && ngx.runtime_parameters != nullptr
      && ngx.input_width == input_width && ngx.input_height == input_height
      && ngx.width == target_width && ngx.height == target_height
      && ngx.requested_render_preset == render_preset
      && ngx.perf_quality == perf_quality
      && ngx.feature_flags == feature_flags) return true;
  if (ngx.feature != nullptr) ReleaseFeatureHandleOnly();
  if (ngx.create_failed) return false;

  const NVSDK_NGX_Result params_result = NVSDK_NGX_D3D12_AllocateParameters(&ngx.runtime_parameters);
  if (NVSDK_NGX_FAILED(params_result) || ngx.runtime_parameters == nullptr) {
    ngx.create_failed = true;
    std::stringstream s;
    s << "Far Cry 6 DLSS: runtime parameters allocation failed: " << ResultToString(params_result)
      << " (0x" << std::hex << static_cast<uint32_t>(params_result) << ")";
    reshade::log::message(reshade::log::level::error, s.str().c_str());
    return false;
  }

  NVSDK_NGX_DLSS_Create_Params params = {};
  params.Feature.InWidth = input_width;
  params.Feature.InHeight = input_height;
  params.Feature.InTargetWidth = target_width;
  params.Feature.InTargetHeight = target_height;
  params.Feature.InPerfQualityValue = static_cast<NVSDK_NGX_PerfQuality_Value>(perf_quality);
  params.InFeatureCreateFlags = feature_flags;
  params.InEnableOutputSubrects = false;

  SetRenderPresetParameters(ngx.runtime_parameters, render_preset);

  int actual_render_preset = render_preset;
  int actual_perf_quality = perf_quality;
  NVSDK_NGX_Result result = NGX_D3D12_CREATE_DLSS_EXT(command_list, 1u, 1u, &ngx.feature, ngx.runtime_parameters, &params);
  if (NVSDK_NGX_FAILED(result)) {
    actual_render_preset = static_cast<int>(NVSDK_NGX_DLSS_Hint_Render_Preset_Default);
    actual_perf_quality = static_cast<int>(NVSDK_NGX_PerfQuality_Value_DLAA);
    params.Feature.InPerfQualityValue = static_cast<NVSDK_NGX_PerfQuality_Value>(actual_perf_quality);
    SetRenderPresetParameters(ngx.runtime_parameters, actual_render_preset);
    result = NGX_D3D12_CREATE_DLSS_EXT(command_list, 1u, 1u, &ngx.feature, ngx.runtime_parameters, &params);
  }

  if (NVSDK_NGX_FAILED(result) || ngx.feature == nullptr) {
    ngx.create_failed = true;
    if (ngx.runtime_parameters != nullptr) {
      NVSDK_NGX_D3D12_DestroyParameters(ngx.runtime_parameters);
      ngx.runtime_parameters = nullptr;
    }
    ngx.requested_render_preset = -1;
    ngx.render_preset = -1;
    ngx.perf_quality = -1;
    std::stringstream s;
    s << "Far Cry 6 DLSS: feature creation failed: " << ResultToString(result)
      << " (0x" << std::hex << static_cast<uint32_t>(result) << ")";
    reshade::log::message(reshade::log::level::error, s.str().c_str());
    return false;
  }

  ngx.requested_render_preset = render_preset;
  ngx.render_preset = actual_render_preset;
  ngx.perf_quality = actual_perf_quality;
  ngx.feature_flags = feature_flags;
  ngx.input_width = input_width;
  ngx.input_height = input_height;
  ngx.reset = true;

  std::stringstream s;
  s << "Far Cry 6 DLSS: feature created in=" << input_width << "x" << input_height
    << " target=" << target_width << "x" << target_height
    << " quality=" << GetPerfQualityName(ngx.perf_quality)
    << " preset=" << GetRenderPresetName(ngx.render_preset)
    << " flags=" << feature_flags;
  reshade::log::message(reshade::log::level::info, s.str().c_str());
  return true;
}

// ── MV-unpack pass (ReShade API compute, mirrors senkiseki velocity setup) ──

inline bool EnsureUnpackLayout(reshade::api::device* device) {
  if (unpack.layout_ready) return unpack.layout.handle != 0u;
  unpack.layout_ready = true;
  if (device == nullptr) return false;

  using DS = reshade::api::shader_stage;
  using DT = reshade::api::descriptor_type;
  using DR = reshade::api::descriptor_range;
  using P = reshade::api::pipeline_layout_param;
  using PT = reshade::api::pipeline_layout_param_type;

  DR srv_r = {0, 0, 0, 1, DS::all_compute, 1, DT::texture_shader_resource_view};  // t0 (packed MV)
  DR uav_r = {0, 0, 0, 1, DS::all_compute, 1, DT::texture_unordered_access_view};  // u0 (pixel MV)
  reshade::api::constant_range pc_range = {};
  pc_range.binding = 0;
  pc_range.dx_register_index = 0;
  pc_range.dx_register_space = 0;
  pc_range.count = 4;  // float4: width, height, mv_scale, unused
  pc_range.visibility = DS::all_compute;

  P params[3];
  params[0].type = PT::descriptor_table;
  params[0].descriptor_table = {1, &srv_r};
  params[1].type = PT::descriptor_table;
  params[1].descriptor_table = {1, &uav_r};
  params[2].type = PT::push_constants;
  params[2].push_constants = pc_range;

  if (!device->create_pipeline_layout(3, params, &unpack.layout)) {
    reshade::log::message(reshade::log::level::warning, "Far Cry 6 DLSS: unpack pipeline layout creation failed");
    return false;
  }
  for (uint32_t i = 0u; i < 2u; ++i) {
    if (!device->allocate_descriptor_table(unpack.layout, i, &unpack.tables[i])) {
      reshade::log::message(reshade::log::level::warning, "Far Cry 6 DLSS: unpack descriptor table allocation failed");
      return false;
    }
  }
  return true;
}

inline bool EnsureUnpackResources(reshade::api::device* device, uint32_t width, uint32_t height) {
  if (device == nullptr || width == 0u || height == 0u) return false;
  if (unpack.texture.handle != 0u && unpack.width == width && unpack.height == height) return unpack.pipeline.handle != 0u;
  if (unpack_device != device) {
    // Statics are single-device; a device change releases everything.
    ReleaseUnpack(unpack_device);
    unpack_device = device;
  } else {
    if (unpack.texture.handle) {
      if (unpack.uav.handle) device->destroy_resource_view(unpack.uav);
      if (unpack.srv.handle) device->destroy_resource_view(unpack.srv);
      device->destroy_resource(unpack.texture);
      unpack.texture = {};
      unpack.srv = {};
      unpack.uav = {};
    }
    if (unpack.pipeline.handle) {
      device->destroy_pipeline(unpack.pipeline);
      unpack.pipeline = {};
    }
  }
  unpack.width = 0u;
  unpack.height = 0u;

  if (!EnsureUnpackLayout(device)) return false;

  reshade::api::resource_desc rd = {};
  rd.type = reshade::api::resource_type::texture_2d;
  rd.texture = {width, height, 1, 1, reshade::api::format::r16g16_float, 1};
  rd.heap = reshade::api::memory_heap::gpu_only;
  rd.usage = reshade::api::resource_usage::shader_resource | reshade::api::resource_usage::unordered_access;
  if (!device->create_resource(rd, nullptr, reshade::api::resource_usage::shader_resource, &unpack.texture)
      || unpack.texture.handle == 0u) {
    reshade::log::message(reshade::log::level::warning, "Far Cry 6 DLSS: unpack texture creation failed");
    return false;
  }
  reshade::api::resource_view_desc vd(reshade::api::resource_view_type::texture_2d, reshade::api::format::r16g16_float, 0, 1, 0, 1);
  if (!device->create_resource_view(unpack.texture, reshade::api::resource_usage::shader_resource, vd, &unpack.srv)
      || !device->create_resource_view(unpack.texture, reshade::api::resource_usage::unordered_access, vd, &unpack.uav)) {
    reshade::log::message(reshade::log::level::warning, "Far Cry 6 DLSS: unpack view creation failed");
    return false;
  }

  reshade::api::shader_desc cs_desc = {};
  cs_desc.code = __dlss_mv_unpack.data();
  cs_desc.code_size = __dlss_mv_unpack.size();
  cs_desc.entry_point = "main";
  reshade::api::pipeline_subobject so = {reshade::api::pipeline_subobject_type::compute_shader, 1, &cs_desc};
  if (!device->create_pipeline(unpack.layout, 1, &so, &unpack.pipeline) || unpack.pipeline.handle == 0u) {
    std::stringstream s;
    s << "Far Cry 6 DLSS: unpack pipeline creation failed (code_size=" << __dlss_mv_unpack.size() << ")";
    reshade::log::message(reshade::log::level::warning, s.str().c_str());
    return false;
  }

  unpack.width = width;
  unpack.height = height;
  return true;
}

inline bool DispatchUnpack(
    reshade::api::device* device,
    reshade::api::command_list* cmd_list,
    reshade::api::resource_view packed_mv_srv,
    uint32_t width, uint32_t height) {
  if (!EnsureUnpackResources(device, width, height)) return false;
  if (packed_mv_srv.handle == 0u || unpack.srv.handle == 0u || unpack.uav.handle == 0u) return false;

  const auto AC = reshade::api::pipeline_stage::all_compute;
  const auto CS = reshade::api::shader_stage::all_compute;
  cmd_list->bind_pipeline(AC, unpack.pipeline);

  reshade::api::descriptor_table_update updates[2] = {
      {unpack.tables[0], 0, 0, 1, reshade::api::descriptor_type::texture_shader_resource_view, &packed_mv_srv},
      {unpack.tables[1], 0, 0, 1, reshade::api::descriptor_type::texture_unordered_access_view, &unpack.uav},
  };
  device->update_descriptor_tables(2, updates);
  cmd_list->bind_descriptor_tables(CS, unpack.layout, 0, 2, unpack.tables);

  const float pc[4] = {static_cast<float>(width), static_cast<float>(height), dlss_mv_scale, 0.f};
  cmd_list->push_constants(CS, unpack.layout, 2, 0, 4, pc);
  cmd_list->dispatch((width + 7u) / 8u, (height + 7u) / 8u, 1);
  cmd_list->barrier(unpack.texture, reshade::api::resource_usage::unordered_access, reshade::api::resource_usage::shader_resource);
  return true;
}

// ── Descriptor resolution (explicit space0 slots, cf. haloinfinite) ──

inline bool ResolveRegister(
    reshade::api::pipeline_layout layout,
    uint32_t layout_param,
    const reshade::api::descriptor_table_update& update,
    uint32_t descriptor_index,
    uint32_t& dx_register_index,
    uint32_t& dx_register_space) {
  const auto* layout_data = renodx::utils::pipeline_layout::GetPipelineLayoutData(layout);
  if (layout_data == nullptr || layout_param >= layout_data->params.size()) return false;

  const auto& param = layout_data->params[layout_param];
  const uint32_t binding = update.binding + descriptor_index;
  switch (param.type) {
    case reshade::api::pipeline_layout_param_type::push_descriptors:
      dx_register_index = param.push_descriptors.dx_register_index + binding;
      dx_register_space = param.push_descriptors.dx_register_space;
      return true;
    case reshade::api::pipeline_layout_param_type::descriptor_table:
    case reshade::api::pipeline_layout_param_type::push_descriptors_with_ranges:
    case reshade::api::pipeline_layout_param_type::push_descriptors_with_static_samplers:
    case reshade::api::pipeline_layout_param_type::descriptor_table_with_static_samplers:
      if (layout_param >= layout_data->ranges.size()) return false;
      for (const auto& range : layout_data->ranges[layout_param]) {
        const bool in_range = binding >= range.binding && (range.count == UINT32_MAX || binding < range.binding + range.count);
        if (!in_range) continue;
        dx_register_index = range.dx_register_index + (binding - range.binding);
        dx_register_space = range.dx_register_space;
        return true;
      }
      return false;
    default:
      return false;
  }
}

// Enumerates ALL (layout_param, binding) pairs whose range covers
// (reg, space). Ranges carry no descriptor types, so callers must
// disambiguate b0/t0-style collisions via recorded update types.
inline uint32_t FindAllTablesForRegister(
    reshade::api::pipeline_layout layout,
    uint32_t dx_register_index,
    uint32_t dx_register_space,
    uint32_t* out_params,
    uint32_t* out_bindings,
    uint32_t max_out) {
  uint32_t n = 0u;
  const auto* layout_data = renodx::utils::pipeline_layout::GetPipelineLayoutData(layout);
  if (layout_data == nullptr || out_params == nullptr || out_bindings == nullptr || max_out == 0u) return 0u;
  for (uint32_t param_index = 0u; param_index < layout_data->ranges.size() && n < max_out; ++param_index) {
    // Skip push-descriptor params (no table to capture; pushes are tracked).
    if (param_index < layout_data->params.size()
        && layout_data->params[param_index].type == reshade::api::pipeline_layout_param_type::push_descriptors) {
      continue;
    }
    for (const auto& range : layout_data->ranges[param_index]) {
      if (range.dx_register_space != dx_register_space) continue;
      if (dx_register_index < range.dx_register_index) continue;
      const uint32_t offset = dx_register_index - range.dx_register_index;
      if (range.count != UINT32_MAX && offset >= range.count) continue;
      out_params[n] = param_index;
      out_bindings[n] = range.binding + offset;
      if (++n >= max_out) return n;
    }
  }
  return n;
}

inline bool CurrentPixelLayout(reshade::api::command_list* cmd_list, reshade::api::pipeline_layout& layout) {
  layout = {};
  if (cmd_list == nullptr) return false;
  auto* shader_state = renodx::utils::shader::GetCurrentState(cmd_list);
  if (shader_state == nullptr) return false;
  auto* pixel_state = renodx::utils::shader::GetCurrentPixelState(shader_state);
  renodx::utils::shader::PopulateStageState(pixel_state);
  if (pixel_state == nullptr || pixel_state->pipeline_details == nullptr) return false;
  layout = pixel_state->pipeline_details->layout;
  return layout.handle != 0u;
}

// Records the current (table, binding) candidates of every slot of interest
// at a TAA draw so the update hook knows what to listen for. Push-bound
// slots are marked push (no capture needed). Cheap: a few layout scans 1-2x
// per frame.
inline void LearnTaaBindings(reshade::api::command_list* cmd_list, const CommandListData* data) {
  if (cmd_list == nullptr || data == nullptr) return;
  reshade::api::pipeline_layout layout = {};
  if (!CurrentPixelLayout(cmd_list, layout)) return;

  const uint32_t slots[kLearnedSlots] = {0u, 0u, 2u, 5u, 6u};
  const bool is_cbv[kLearnedSlots] = {true, false, false, false, false};
  for (uint32_t i = 0u; i < kLearnedSlots; ++i) {
    const uint32_t slot = slots[i];
    learned[i].is_push = false;
    if (!is_cbv[i] && slot < kTrackedSrvCount && data->push_srvs[slot].handle != 0u) {
      learned[i].is_push = true;
      learned[i].count = 0u;
      continue;
    }
    if (is_cbv[i] && data->constant_buffers[slot].buffer.handle != 0u) {
      learned[i].is_push = true;
      learned[i].count = 0u;
      continue;
    }
    uint32_t params[kMaxCandidates] = {};
    uint32_t bindings[kMaxCandidates] = {};
    const uint32_t n = FindAllTablesForRegister(layout, slot, 0u, params, bindings, kMaxCandidates);
    uint32_t kept = 0u;
    for (uint32_t k = 0u; k < n && kept < kMaxCandidates; ++k) {
      if (params[k] >= data->descriptor_tables.size()) continue;
      const auto table = data->descriptor_tables[params[k]];
      if (table.handle == 0u) continue;
      uint64_t heap = 0u;
      uint32_t offset = 0u;
      if (!CanonicalBindingCached(cmd_list->get_device(), table, bindings[k], heap, offset)) continue;
      learned[i].heaps[kept] = heap;
      learned[i].offsets[kept] = offset;
      ++kept;
    }
    learned[i].count = kept;
  }
  RebuildInterest();
}

inline bool IsSrvType(reshade::api::descriptor_type type) {
  return type == reshade::api::descriptor_type::texture_shader_resource_view
      || type == reshade::api::descriptor_type::buffer_shader_resource_view;
}

inline reshade::api::resource_view ResolveSlotSrv(
    reshade::api::command_list* cmd_list,
    const CommandListData* data,
    uint32_t slot) {
  if (cmd_list == nullptr || data == nullptr || slot >= kTrackedSrvCount) return {0};
  const uint32_t li = LearnIndexForSlot(slot, false);
  if (li != UINT32_MAX && data->push_srvs[slot].handle != 0u) {
    slot_hit[li].store(1);
    return data->push_srvs[slot];
  }
  if (li == UINT32_MAX) return {0};

  // Journal lookup (boot-time writes included), type-checked. Candidates are
  // canonicalized so native-creation writes and GPU binds meet.
  reshade::api::pipeline_layout layout = {};
  if (CurrentPixelLayout(cmd_list, layout)) {
    uint32_t params[kMaxCandidates] = {};
    uint32_t bindings[kMaxCandidates] = {};
    const uint32_t n = FindAllTablesForRegister(layout, slot, 0u, params, bindings, kMaxCandidates);
    auto* device = cmd_list->get_device();
    std::lock_guard<std::mutex> lock(capture_mutex);
    for (uint32_t k = 0u; k < n; ++k) {
      if (params[k] >= data->descriptor_tables.size()) continue;
      const auto table = data->descriptor_tables[params[k]];
      if (table.handle == 0u) continue;
      uint64_t heap = 0u;
      uint32_t offset = 0u;
      if (!CanonicalBindingCached(device, table, bindings[k], heap, offset)) continue;
      const auto rit = seen.find(HeapKey(heap, offset));
      if (rit != seen.end() && rit->second.has_view && rit->second.has_type && IsSrvType(rit->second.type)) {
        slot_hit[li].store(1);
        return rit->second.view;
      }
    }
  }
  // Miss: (re-)learn from the current bindings so the update hook is armed
  // for the next frame. This frame falls back to vanilla TAA.
  slot_hit[li].store(0);
  LearnTaaBindings(cmd_list, data);
  return {0};
}

inline bool ResolveB0(
    reshade::api::command_list* cmd_list,
    const CommandListData* data,
    reshade::api::buffer_range& out) {
  out = {};
  if (data == nullptr) return false;
  if (data->constant_buffers[0].buffer.handle != 0u) {
    out = data->constant_buffers[0];
    slot_hit[0].store(1);
    return true;
  }
  if (cmd_list == nullptr) return false;
  reshade::api::pipeline_layout layout = {};
  if (CurrentPixelLayout(cmd_list, layout)) {
    uint32_t params[kMaxCandidates] = {};
    uint32_t bindings[kMaxCandidates] = {};
    const uint32_t n = FindAllTablesForRegister(layout, 0u, 0u, params, bindings, kMaxCandidates);
    auto* device = cmd_list->get_device();
    std::lock_guard<std::mutex> lock(capture_mutex);
    for (uint32_t k = 0u; k < n; ++k) {
      if (params[k] >= data->descriptor_tables.size()) continue;
      const auto table = data->descriptor_tables[params[k]];
      if (table.handle == 0u) continue;
      uint64_t heap = 0u;
      uint32_t offset = 0u;
      if (!CanonicalBindingCached(device, table, bindings[k], heap, offset)) continue;
      const auto rit = seen.find(HeapKey(heap, offset));
      if (rit != seen.end() && rit->second.has_cbv
          && rit->second.has_type
          && rit->second.type == reshade::api::descriptor_type::constant_buffer) {
        out = rit->second.cbv;
        slot_hit[0].store(1);
        return true;
      }
    }
  }
  slot_hit[0].store(0);
  LearnTaaBindings(cmd_list, data);
  return false;
}

// Evicts non-learned journal entries to make room for learned traffic.
// Caller must hold capture_mutex. Throttled to one pass per 300 presents so
// effect-churn refills cannot turn it into a per-frame O(n) scan.
inline void EvictNonLearnedLocked() {
  if (stat_presents.load() - last_evict_present.load() < 300u) return;
  last_evict_present.store(stat_presents.load());
  uint64_t keep[kMaxInterestTables] = {};
  uint32_t n = interest_count.load();
  if (n > kMaxInterestTables) n = kMaxInterestTables;
  for (uint32_t j = 0u; j < n; ++j) {
    keep[j] = HeapKey(interest_heaps[j].load(), interest_offs[j].load());
  }
  for (auto it = seen.begin(); it != seen.end();) {
    bool learned = false;
    for (uint32_t j = 0u; j < n; ++j) {
      if (it->first == keep[j]) {
        learned = true;
        break;
      }
    }
    if (learned) {
      ++it;
    } else {
      it = seen.erase(it);
    }
  }
  seen_count.store(seen.size());
}

// Own update hook: from attach until the first resolve, records every SRV /
// CBV write into the capped journal (captures boot-time heap writes). After
// the first resolve, only updates overlapping learned TAA heap ranges are
// recorded. Each update is canonicalized once to (heap, offset) so native
// view-creation writes ({CPUptr}/binding 0) and GPU binds meet in one key
// space. No vector resizes, tiny mutex critical sections only on record.
inline bool OnUpdateDescriptorTables(
    reshade::api::device* device,
    uint32_t count,
    const reshade::api::descriptor_table_update* updates) {
  if (device == nullptr || count == 0u || updates == nullptr) return false;
  // Fail-open journal filter: accept writes from any initialized device.
  // Heap handles are native-unique so foreign heaps cannot collide; a strict
  // NVIDIA-only filter here risks silently discarding the writes we need.
  if (!IsSeenDevice(device->get_native())) return false;
  const bool full = full_capture_frames.load() > 0;
  const bool targeted = resolved_once.load() && !full;
  if (targeted && interest_count.load() == 0u) return false;

  for (uint32_t u = 0u; u < count; ++u) {
    const auto& update = updates[u];
    if (update.table.handle == 0u || update.count == 0u) continue;
    const bool is_cbv = update.type == reshade::api::descriptor_type::constant_buffer;
    const bool is_srv = IsSrvType(update.type);
    const bool is_sampler_srv = update.type == reshade::api::descriptor_type::sampler_with_resource_view;
    if (!is_cbv && !is_srv && !is_sampler_srv) continue;

    uint64_t heap = 0u;
    uint32_t base = 0u;
    // Cached canonicalization: steady-state hits cost a few atomic loads.
    // array_offset folds into the base (ReShade asserts it 0 for queries).
    bool have_base = RawCacheLookup(update.table.handle, heap, base);
    if (!have_base) {
      // Unknown raw handle: exactly one canonical query classifies it (then
      // cached). This is the only per-update query.
      if (!CanonicalBinding(device, update.table, 0u, heap, base)) continue;
      RawCacheStore(update.table.handle, heap, base);
    }
    base += update.array_offset + update.binding;

    uint32_t overlap_ks[kMaxInterestTables] = {};
    uint32_t overlap_n = 0u;
    if (targeted) {
      // O(interest) range gate: each interest entry yields at most one k.
      // No lock is taken when nothing overlaps.
      const uint32_t n = interest_count.load();
      for (uint32_t j = 0u; j < n && j < kMaxInterestTables && overlap_n < kMaxInterestTables; ++j) {
        if (interest_heaps[j].load() != heap) continue;
        const uint32_t o = interest_offs[j].load();
        if (o < base || o >= base + update.count) continue;
        overlap_ks[overlap_n++] = o - base;
      }
      if (overlap_n == 0u) continue;
    } else if (seen_count.load() >= kSeenCap) {
      continue;  // journal full; re-arm path clears it if resolve keeps failing
    }

    std::lock_guard<std::mutex> lock(capture_mutex);
    if (!seen_reserved) {
      seen.reserve(kSeenReserve);
      seen_reserved = true;
    }
    if (capture_device_native.load() == 0u) capture_device_native.store(device->get_native());
    // Targeted mode records only overlapping ks (learned traffic bypasses
    // caps unconditionally); otherwise every descriptor in the update.
    const uint32_t iters = targeted ? overlap_n : update.count;
    for (uint32_t ii = 0u; ii < iters; ++ii) {
      const uint32_t k = targeted ? overlap_ks[ii] : ii;
      const uint32_t off = base + k;
      const uint64_t key = HeapKey(heap, off);
      if (seen.size() >= kSeenCap && seen.find(key) == seen.end()) {
        if (targeted) {
          // Learned traffic always lands: evict non-learned entries to make
          // room (throttled; see EvictNonLearnedLocked). Cap-full blindness
          // for TAA inputs is structurally impossible after this point.
          EvictNonLearnedLocked();
          if (seen.size() >= kSeenCap && seen.find(key) == seen.end()) continue;
        } else {
          if (full) {
            // Full-window re-arm bypasses the cap (bounded to 5 frames).
          } else {
            if (!logged_seen_cap) {
              logged_seen_cap = true;
              reshade::log::message(reshade::log::level::warning,
                                    "Far Cry 6 DLSS: descriptor journal full before first resolve; "
                                    "TAA inputs may need a re-arm (see log)");
            }
            continue;
          }
        }
      }
      auto& rec = seen[key];
      rec.type = update.type;
      rec.has_type = true;
      if (is_cbv) {
        rec.cbv = static_cast<const reshade::api::buffer_range*>(update.descriptors)[k];
        rec.has_cbv = rec.cbv.buffer.handle != 0u;
      } else if (is_srv) {
        rec.view = static_cast<const reshade::api::resource_view*>(update.descriptors)[k];
        rec.has_view = rec.view.handle != 0u;
      } else {
        rec.view = static_cast<const reshade::api::sampler_with_resource_view*>(update.descriptors)[k].view;
        rec.has_view = rec.view.handle != 0u;
      }
    }
    seen_count.store(seen.size());
  }
  return false;
}

inline ID3D12Resource* GetNativeResource(reshade::api::device* device, reshade::api::resource_view view) {
  if (device == nullptr || view.handle == 0u) return nullptr;
  const auto resource = renodx::utils::resource::GetResourceFromView(device, view);
  if (resource.handle == 0u) return nullptr;
  return reinterpret_cast<ID3D12Resource*>(resource.handle);
}

// Reads up to max_floats of the b0 constant buffer (for jitter auto-detect).
inline bool ReadB0Floats(const reshade::api::buffer_range& buffer_range, std::vector<float>& out, uint32_t max_floats) {
  out.clear();
  if (buffer_range.buffer.handle == 0u || max_floats == 0u) return false;
  auto* resource = reinterpret_cast<ID3D12Resource*>(buffer_range.buffer.handle);
  if (resource == nullptr) return false;

  const uint64_t offset = buffer_range.offset == UINT64_MAX ? 0u : buffer_range.offset;
  const uint64_t bytes = static_cast<uint64_t>(max_floats) * sizeof(float);
  D3D12_RANGE read_range = {static_cast<SIZE_T>(offset), static_cast<SIZE_T>(offset + bytes)};
  void* mapped = nullptr;
  if (FAILED(resource->Map(0u, &read_range, &mapped)) || mapped == nullptr) return false;
  out.assign(static_cast<const float*>(mapped), static_cast<const float*>(mapped) + max_floats);
  D3D12_RANGE written_range = {0u, 0u};
  resource->Unmap(0u, &written_range);
  return true;
}

// Scans b0 for a per-frame varying small-magnitude float pair (the game's
// render jitter). Requires the same pair to win N consecutive frames before
// adoption; otherwise jitter stays zero. Never fabricates: on any doubt the
// output is (0,0) and the log says why.
inline void UpdateJitterEstimate(const std::vector<float>& current) {
  auto& js = jitter_state;
  if (dlss_jitter_source == 0.f) {
    js.x = 0.f;
    js.y = 0.f;
    return;
  }
  if (current.empty() || js.prev.size() != current.size()) {
    js.prev = current;
    js.candidate = -1;
    js.streak = 0;
    return;
  }
  int best = -1;
  float best_mag = 0.f;
  for (size_t i = 0u; i + 1u < current.size(); ++i) {
    const float dx = current[i] - js.prev[i];
    const float dy = current[i + 1u] - js.prev[i + 1u];
    const float ax = dx < 0.f ? -dx : dx;
    const float ay = dy < 0.f ? -dy : dy;
    if (ax < 1e-7f && ay < 1e-7f) continue;          // static this frame
    if (ax > 0.05f || ay > 0.05f) continue;          // too large for jitter
    const float cx = current[i] < 0.f ? -current[i] : current[i];
    const float cy = current[i + 1u] < 0.f ? -current[i + 1u] : current[i + 1u];
    if (cx > 2.f || cy > 2.f) continue;              // not a small offset
    const float mag = ax + ay;
    if (best < 0 || mag < best_mag) {
      best = static_cast<int>(i);
      best_mag = mag;
    }
  }
  js.prev = current;
  if (best < 0) return;  // no varying pair this frame; keep previous estimate
  if (best == js.candidate) {
    ++js.streak;
  } else {
    js.candidate = best;
    js.streak = 0;
  }
  if (js.streak >= 30 && !js.adopted) {
    js.adopted = true;
  }
  if (js.adopted && js.candidate >= 0
      && static_cast<size_t>(js.candidate + 1u) < current.size()) {
    js.x = current[static_cast<size_t>(js.candidate)];
    js.y = current[static_cast<size_t>(js.candidate + 1u)];
    if (!js.logged_adopt) {
      js.logged_adopt = true;
      char buf[192];
      snprintf(buf, sizeof(buf),
               "Far Cry 6 DLSS: jitter auto-detect adopted b0[%d]=(%.6f, %.6f)",
               js.candidate, js.x, js.y);
      reshade::log::message(reshade::log::level::info, buf);
    }
  }
}

inline bool IsTaaDraw(reshade::api::command_list* cmd_list, CommandListData*& data, uint32_t& hash) {
  auto* shader_state = renodx::utils::shader::GetCurrentState(cmd_list);
  if (shader_state == nullptr) return false;
  hash = renodx::utils::shader::GetCurrentPixelShaderHash(shader_state);
  if (hash != kTaaMainHash && hash != kTaaPauseHash) return false;
  data = Get(cmd_list);
  return data != nullptr && data->rtvs[0].handle != 0u;
}

// Draw-time TAA check with memoized PS hash: the state lookup runs at most
// once per pipeline bind (see OnBindPipeline), not once per draw.
inline bool CheckTaaDraw(reshade::api::command_list* cmd_list, CommandListData*& data, uint32_t& hash) {
  data = nullptr;
  hash = 0u;
  if (cmd_list == nullptr) return false;
  auto* list_data = cmd_list->get_private_data<CommandListData>();
  if (list_data == nullptr || list_data->ps_dirty || !list_data->ps_hash_valid) {
    if (!IsTaaDraw(cmd_list, data, hash)) return false;
    if (data != nullptr) {
      data->cached_ps_hash = hash;
      data->ps_hash_valid = true;
      data->ps_dirty = false;
    }
    return true;
  }
  hash = list_data->cached_ps_hash;
  if (hash != kTaaMainHash && hash != kTaaPauseHash) return false;
  data = list_data;
  return data->rtvs[0].handle != 0u;
}

// Shared resolve-failure path: streak accounting, duty-cycle-limited
// re-arm, one-shot log. Returns false (vanilla TAA) always.
inline bool FailResolveResources(ID3D12Resource* color, ID3D12Resource* packed_motion, ID3D12Resource* depth,
                                 ID3D12Resource* output_target) {
  NoteFail(kFailResolve);
  // Resolve misses are expected while learning (first frames, heap churn).
  // After a sustained streak, arm the time-boxed full-record fallback for a
  // few frames. Re-arms are duty-cycle limited (600 presents) and logged, so
  // a pathological table-churn case cannot become a permanent hitch cycle.
  // If the journal hit its cap while still unresolved, drop it first: the
  // re-arm window then re-observes the live heap state.
  const int streak = stat_resolve_fail_streak.fetch_add(1) + 1;
  const uint64_t presents = stat_presents.load();
  if (streak % 30 == 0 && full_capture_frames.load() == 0
      && presents - last_arm_present.load() > kRearmCooldownPresents) {
    last_arm_present.store(presents);
    {
      std::lock_guard<std::mutex> lock(capture_mutex);
      if (seen.size() >= kSeenCap) {
        seen.clear();
        seen_count.store(0);
      }
    }
    full_capture_frames.store(5);
    {
      std::lock_guard<std::mutex> lock(capture_mutex);
      char buf[192];
      snprintf(buf, sizeof(buf),
               "Far Cry 6 DLSS: arming 5-frame full descriptor record to locate TAA inputs (journal=%llu)",
               static_cast<unsigned long long>(seen.size()));
      reshade::log::message(reshade::log::level::info, buf);
    }
  }
  if (!logged_missing_resources) {
    logged_missing_resources = true;
    std::stringstream s;
    s << "Far Cry 6 DLSS: TAA draw found, but resources failed to resolve"
      << " color=" << reinterpret_cast<void*>(color)
      << " packed_mv=" << reinterpret_cast<void*>(packed_motion)
      << " depth=" << reinterpret_cast<void*>(depth)
      << " rtv0=" << reinterpret_cast<void*>(output_target);
    reshade::log::message(reshade::log::level::warning, s.str().c_str());
  }
  return false;
}

inline bool EvaluateDLSS(reshade::api::command_list* cmd_list, const CommandListData* data, uint32_t hash) {
  if (cmd_list == nullptr || data == nullptr || dlss_enabled == 0.f || !IsSupported()) return false;
  auto* device = cmd_list->get_device();
  if (device == nullptr || device->get_api() != reshade::api::device_api::d3d12) return false;

  auto* command_list = reinterpret_cast<ID3D12GraphicsCommandList*>(cmd_list->get_native());
  if (command_list == nullptr) return false;
  if (ngx.eval_failed) return false;
  for (auto& hit : slot_hit) hit.store(0);  // resolvers set per-slot results below
  {
    // Per-device gate: WARP/other-adapter command lists keep vanilla TAA.
    std::lock_guard<std::mutex> lock(device_mutex);
    if (nvidia_devices.find(device->get_native()) == nvidia_devices.end()) {
      NoteFail(kFailDevice);
      return false;
    }
  }

  // b0 feeds the jitter scan only. Without it jitter stays zero; the draw
  // proceeds only in experimental fallback mode, else vanilla TAA.
  reshade::api::buffer_range b0_range = {};
  const bool b0_ok = ResolveB0(cmd_list, data, b0_range);
  if (!b0_ok) {
    if (dlss_allow_fallback == 0.f) {
      NoteFail(kFailB0);
      if (!logged_missing_constants) {
        logged_missing_constants = true;
        reshade::log::message(reshade::log::level::warning, "Far Cry 6 DLSS: TAA draw found, but b0 constants were not captured");
      }
      return false;
    }
  }

  const uint32_t depth_slot = (dlss_depth_source > 0.5f) ? 6u : 5u;
  const auto color_view = ResolveSlotSrv(cmd_list, data, kSlotColor);
  const auto motion_view = ResolveSlotSrv(cmd_list, data, kSlotPackedMv);
  const auto depth_view = ResolveSlotSrv(cmd_list, data, depth_slot);
  auto* color = GetNativeResource(device, color_view);
  auto* packed_motion = GetNativeResource(device, motion_view);
  auto* depth = GetNativeResource(device, depth_view);
  auto* output_target = GetNativeResource(device, data->rtvs[0]);
  // Color and the output target are mandatory: DLSS cannot run without them,
  // in any mode.
  if (color == nullptr || output_target == nullptr) {
    return FailResolveResources(color, packed_motion, depth, output_target);
  }
  const bool mv_real = packed_motion != nullptr;
  const bool depth_real = depth != nullptr;
  if ((!mv_real || !depth_real) && dlss_allow_fallback == 0.f) {
    return FailResolveResources(color, packed_motion, depth, output_target);
  }

  const D3D12_RESOURCE_DESC color_desc = color->GetDesc();
  const D3D12_RESOURCE_DESC output_desc = output_target->GetDesc();
  const uint32_t input_width = static_cast<uint32_t>(color_desc.Width);
  const uint32_t input_height = color_desc.Height;
  const uint32_t target_width = static_cast<uint32_t>(output_desc.Width);
  const uint32_t target_height = output_desc.Height;
  if (input_width == 0u || input_height == 0u || target_width == 0u || target_height == 0u) {
    NoteFail(kFailSize);
    return false;
  }
  if (input_width > target_width || input_height > target_height) {
    // Never upscale downwards; leave the game's TAA in place.
    NoteFail(kFailSize);
    return false;
  }
  // Sanity: reject absurd inputs from stale journal entries on recycled heap
  // slots (e.g. tiny effect/system textures). DLSS ratios top out at 3x per
  // axis (Ultra Performance); DLAA requires exact equality.
  {
    bool sane = input_width >= 256u && input_height >= 256u && target_width >= 256u && target_height >= 256u
        && target_width <= 7680u && target_height <= 4320u
        && target_width <= input_width * 4u && target_height <= input_height * 4u;
    if (sane) {
      const int mode = static_cast<int>(dlss_mode);
      if (mode == 1 && (input_width != target_width || input_height != target_height)) sane = false;
    }
    if (!sane) {
      NoteFail(kFailSize);
      if (!logged_size_guard) {
        logged_size_guard = true;
        char buf[192];
        snprintf(buf, sizeof(buf),
                 "Far Cry 6 DLSS: rejecting implausible sizes in=%ux%u target=%ux%u (stale journal entry?)",
                 input_width, input_height, target_width, target_height);
        reshade::log::message(reshade::log::level::warning, buf);
      }
      return false;
    }
  }

  // Experimental fallback: stand-ins for unresolvable MV/depth. Reached
  // only when color+output resolved and the fallback setting is on.
  const bool use_mv_dummy = !mv_real;
  const bool use_depth_dummy = !depth_real;
  if (use_mv_dummy || use_depth_dummy) {
    if (!EnsureFallback(device, input_width, input_height)) {
      NoteFail(kFailResolve);
      return false;
    }
    if (use_mv_dummy) slot_hit[LearnIndexForSlot(kSlotPackedMv, false)].store(2);
    if (use_depth_dummy) slot_hit[LearnIndexForSlot(depth_slot, false)].store(2);
    if (!logged_fallback) {
      logged_fallback = true;
      std::stringstream s;
      s << "Far Cry 6 DLSS: experimental fallback engaged (mv_dummy=" << (use_mv_dummy ? 1 : 0)
        << " depth_dummy=" << (use_depth_dummy ? 1 : 0)
        << "): image will be softer than TAA until real inputs resolve";
      reshade::log::message(reshade::log::level::info, s.str().c_str());
    }
  }

  // NGX output format follows the TAA target. Only formats NGX accepts as
  // DLSS output are usable; anything else keeps vanilla TAA (no regression).
  DXGI_FORMAT output_format = DXGI_FORMAT_UNKNOWN;
  switch (output_desc.Format) {
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
      output_format = DXGI_FORMAT_R8G8B8A8_UNORM;
      break;
    case DXGI_FORMAT_R16G16B16A16_FLOAT:
      output_format = DXGI_FORMAT_R16G16B16A16_FLOAT;
      break;
    case DXGI_FORMAT_R11G11B10_FLOAT:
      output_format = DXGI_FORMAT_R11G11B10_FLOAT;
      break;
    default:
      break;
  }
  if (output_format == DXGI_FORMAT_UNKNOWN) {
    NoteFail(kFailFormat);
    if (!logged_unsupported_output) {
      logged_unsupported_output = true;
      std::stringstream s;
      s << "Far Cry 6 DLSS: unsupported TAA target format " << static_cast<int>(output_desc.Format)
        << ", keeping vanilla TAA";
      reshade::log::message(reshade::log::level::warning, s.str().c_str());
    }
    return false;
  }

  if (!EnsureNgxInitialized(device)) {
    NoteFail(kFailNgxInit);
    return false;
  }
  if (!EnsureOutputTexture(ngx.device.Get(), target_width, target_height, output_format)) return false;

  // Effective MV/depth: real journal entries, or fallback stand-ins.
  // Zeroed MVs are not jittered: report that honestly to NGX.
  ID3D12Resource* motion_vectors = nullptr;
  ID3D12Resource* depth_res = depth;
  int feature_flags = GetFeatureFlags();
  if (use_mv_dummy) {
    motion_vectors = reinterpret_cast<ID3D12Resource*>(fallback.mv_texture.handle);
    feature_flags &= ~NVSDK_NGX_DLSS_Feature_Flags_MVJittered;
  } else {
    if (!DispatchUnpack(device, cmd_list, motion_view, input_width, input_height)) {
      NoteFail(kFailUnpack);
      return false;
    }
    motion_vectors = unpack.texture.handle != 0u
        ? reinterpret_cast<ID3D12Resource*>(unpack.texture.handle)
        : nullptr;
  }
  if (use_depth_dummy) {
    depth_res = reinterpret_cast<ID3D12Resource*>(fallback.depth_texture.handle);
  }
  if (motion_vectors == nullptr || depth_res == nullptr) return false;

  // Any input/output swap (heap-slot recycling, resizes) invalidates DLSS
  // history: force a reset instead of poisoning it with a stale frame.
  {
    const uint64_t handles[4] = {
        reinterpret_cast<uint64_t>(color), reinterpret_cast<uint64_t>(motion_vectors),
        reinterpret_cast<uint64_t>(depth_res), reinterpret_cast<uint64_t>(output_target)};
    if (handles[0] != last_in_color || handles[1] != last_in_mv || handles[2] != last_in_depth
        || handles[3] != last_in_output) {
      ngx.reset = true;
      last_in_color = handles[0];
      last_in_mv = handles[1];
      last_in_depth = handles[2];
      last_in_output = handles[3];
    }
  }

  const int render_preset = GetRenderPresetValue();
  const int perf_quality = GetPerfQualityValue(input_width, input_height, target_width, target_height);
  if (!EnsureFeature(command_list, input_width, input_height, target_width, target_height,
                     render_preset, feature_flags, perf_quality)) {
    NoteFail(kFailFeature);
    return false;
  }

  // Jitter: game estimate (b0 scan) or zero. Without b0 (fallback mode) it
  // stays zero. Convert a small-magnitude estimate as UV (x resolution); a
  // pixel-scale estimate passes through.
  float jitter_x = 0.f;
  float jitter_y = 0.f;
  if (b0_ok && dlss_jitter_source > 0.5f) {
    std::vector<float> b0;
    if (ReadB0Floats(b0_range, b0, 512u)) {
      UpdateJitterEstimate(b0);
      const float ex = jitter_state.adopted ? jitter_state.x : 0.f;
      const float ey = jitter_state.adopted ? jitter_state.y : 0.f;
      const float ax = ex < 0.f ? -ex : ex;
      const float ay = ey < 0.f ? -ey : ey;
      jitter_x = (ax < 0.05f) ? ex * static_cast<float>(input_width) : ex;
      jitter_y = (ay < 0.05f) ? ey * static_cast<float>(input_height) : ey;
    } else if (!logged_b0_map_failed) {
      logged_b0_map_failed = true;
      reshade::log::message(reshade::log::level::warning, "Far Cry 6 DLSS: b0 buffer map failed, jitter stays zero");
    }
  }

  NVSDK_NGX_D3D12_DLSS_Eval_Params eval = {};
  eval.Feature.pInColor = color;
  eval.Feature.pInOutput = ngx.output_texture.Get();
  eval.pInDepth = depth_res;
  eval.pInMotionVectors = motion_vectors;
  eval.InJitterOffsetX = jitter_x;
  eval.InJitterOffsetY = jitter_y;
  eval.InRenderSubrectDimensions.Width = input_width;
  eval.InRenderSubrectDimensions.Height = input_height;
  eval.InReset = ngx.reset ? 1 : 0;
  eval.InMVScaleX = 1.f;
  eval.InMVScaleY = 1.f;
  eval.InPreExposure = 1.f;
  eval.InExposureScale = 1.f;

  if (dlss_debug_logging != 0.f) {
    static int eval_log_count = 0;
    if (++eval_log_count % 60 == 0) {
      char buf[384];
      snprintf(buf, sizeof(buf),
               "Far Cry 6 DLSS: eval hash=0x%08X in=%ux%u target=%ux%u quality=%s preset=%s jit=(%.3f,%.3f) mvscale=%.2f flags=0x%X reset=%d",
               hash, input_width, input_height, target_width, target_height,
               GetPerfQualityName(ngx.perf_quality), GetRenderPresetName(ngx.render_preset),
               jitter_x, jitter_y, dlss_mv_scale, feature_flags, eval.InReset);
      reshade::log::message(reshade::log::level::info, buf);
    }
  }

  const NVSDK_NGX_Result result = NGX_D3D12_EVALUATE_DLSS_EXT(command_list, ngx.feature, ngx.runtime_parameters, &eval);
  if (NVSDK_NGX_FAILED(result)) {
    ngx.eval_failed = true;
    NoteFail(kFailEval);
    std::stringstream s;
    s << "Far Cry 6 DLSS: evaluation failed: " << ResultToString(result)
      << " (0x" << std::hex << static_cast<uint32_t>(result) << ")";
    reshade::log::message(reshade::log::level::error, s.str().c_str());
    return false;
  }
  ngx.reset = false;
  NoteSuccess();

  D3D12_RESOURCE_BARRIER barriers[2] = {};
  barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  barriers[0].Transition.pResource = ngx.output_texture.Get();
  barriers[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
  barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
  barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  barriers[1].Transition.pResource = output_target;
  barriers[1].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
  barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
  command_list->ResourceBarrier(2u, barriers);
  command_list->CopyResource(output_target, ngx.output_texture.Get());
  std::swap(barriers[0].Transition.StateBefore, barriers[0].Transition.StateAfter);
  std::swap(barriers[1].Transition.StateBefore, barriers[1].Transition.StateAfter);
  command_list->ResourceBarrier(2u, barriers);

  if (!ngx.logged_success) {
    ngx.logged_success = true;
    std::stringstream s;
    s << "Far Cry 6 DLSS: replacing TAA 0x" << std::hex << hash << std::dec
      << " at in=" << input_width << "x" << input_height
      << " target=" << target_width << "x" << target_height;
    reshade::log::message(reshade::log::level::info, s.str().c_str());
  }
  if (!ngx.logged_jitter) {
    ngx.logged_jitter = true;
    std::stringstream s;
    s << "Far Cry 6 DLSS: jitter ngx_pixels=(" << eval.InJitterOffsetX << ", " << eval.InJitterOffsetY << ")"
      << " mv_jittered=" << (dlss_motion_vectors_jittered != 0.f ? "on" : "off")
      << " depth_slot=" << ((dlss_depth_source > 0.5f) ? "t6" : "t5")
      << " reset=" << eval.InReset;
    reshade::log::message(reshade::log::level::info, s.str().c_str());
  }
  if (hash == kTaaPauseHash && dlss_debug_logging != 0.f) {
    static int pause_log_count = 0;
    if (++pause_log_count % 60 == 0) {
      reshade::log::message(reshade::log::level::info, "Far Cry 6 DLSS: pause TAA replaced (RTV1 alpha history left untouched)");
    }
  }
  // NOTE: the pause variant writes a second target (alpha/coverage history).
  // DLSS replaces RTV0; RTV1 is intentionally left untouched so downstream
  // passes keep reading a valid (if one frame older at pause entry) buffer.
  return true;
}

inline void OnInitCommandList(reshade::api::command_list* cmd_list) { cmd_list->create_private_data<CommandListData>(); }
inline void OnDestroyCommandList(reshade::api::command_list* cmd_list) { cmd_list->destroy_private_data<CommandListData>(); }
inline void OnResetCommandList(reshade::api::command_list* cmd_list) {
  if (auto* data = cmd_list->get_private_data<CommandListData>()) *data = {};
}

// Any pipeline bind may change the pixel shader: mark the memoized hash
// dirty (refreshed lazily at the next draw). Never creates private data.
inline void OnBindPipeline(
    reshade::api::command_list* cmd_list, reshade::api::pipeline_stage, reshade::api::pipeline) {
  if (cmd_list == nullptr) return;
  if (auto* data = cmd_list->get_private_data<CommandListData>()) data->ps_dirty = true;
}

inline void OnBindRenderTargetsAndDepthStencil(
    reshade::api::command_list* cmd_list, uint32_t count,
    const reshade::api::resource_view* rtvs, reshade::api::resource_view dsv) {
  if (dlss_enabled == 0.f || !any_nvidia.load()) return;
  auto* data = Get(cmd_list);
  if (data == nullptr) return;
  data->rtvs = {};
  for (uint32_t i = 0u; i < count && i < kTrackedRtvCount; ++i) data->rtvs[i] = rtvs != nullptr ? rtvs[i] : reshade::api::resource_view{};
  data->dsv = dsv;
}

inline void OnPushDescriptors(
    reshade::api::command_list* cmd_list, reshade::api::shader_stage stages,
    reshade::api::pipeline_layout layout, uint32_t layout_param,
    const reshade::api::descriptor_table_update& update) {
  if (dlss_enabled == 0.f || !any_nvidia.load()) return;
  if (!renodx::utils::bitwise::HasFlag(stages, reshade::api::shader_stage::pixel)) return;
  auto* data = Get(cmd_list);
  if (data == nullptr) return;
  for (uint32_t i = 0u; i < update.count; ++i) {
    uint32_t dx_register_index = 0u;
    uint32_t dx_register_space = 0u;
    if (!ResolveRegister(layout, layout_param, update, i, dx_register_index, dx_register_space)) continue;
    if (dx_register_space != 0u) continue;
    if (update.type == reshade::api::descriptor_type::constant_buffer) {
      if (dx_register_index >= kTrackedCbvCount) continue;
      data->constant_buffers[dx_register_index] = static_cast<const reshade::api::buffer_range*>(update.descriptors)[i];
    } else if (update.type == reshade::api::descriptor_type::texture_shader_resource_view) {
      if (dx_register_index >= kTrackedSrvCount) continue;
      data->push_srvs[dx_register_index] = static_cast<const reshade::api::resource_view*>(update.descriptors)[i];
    }
  }
}

inline void OnBindDescriptorTables(
    reshade::api::command_list* cmd_list, reshade::api::shader_stage stages,
    reshade::api::pipeline_layout, uint32_t first, uint32_t count,
    const reshade::api::descriptor_table* tables) {
  if (dlss_enabled == 0.f || !any_nvidia.load()) return;
  if (!renodx::utils::bitwise::HasFlag(stages, reshade::api::shader_stage::pixel)) return;
  auto* data = Get(cmd_list);
  if (data == nullptr || tables == nullptr) return;
  for (uint32_t i = 0u; i < count && first + i < kTrackedTableCount; ++i) data->descriptor_tables[first + i] = tables[i];
}

inline bool OnDraw(reshade::api::command_list* cmd_list, uint32_t, uint32_t, uint32_t, uint32_t) {
  // Master gate first: disabled (or no NVIDIA device) costs ~nothing and
  // keeps no per-draw state lookups.
  if (dlss_enabled == 0.f || !any_nvidia.load()) return false;
  CommandListData* data = nullptr;
  uint32_t hash = 0u;
  if (!CheckTaaDraw(cmd_list, data, hash)) return false;
  ++stat_taa_draws;
  if (!logged_taa_draw_detected) {
    logged_taa_draw_detected = true;
    std::stringstream s;
    s << "Far Cry 6 DLSS: detected TAA draw 0x" << std::hex << hash;
    reshade::log::message(reshade::log::level::info, s.str().c_str());
  }
  return EvaluateDLSS(cmd_list, data, hash);
}

inline bool OnDrawIndexed(reshade::api::command_list* cmd_list, uint32_t, uint32_t, uint32_t, int32_t, uint32_t) {
  if (dlss_enabled == 0.f || !any_nvidia.load()) return false;
  CommandListData* data = nullptr;
  uint32_t hash = 0u;
  if (!CheckTaaDraw(cmd_list, data, hash)) return false;
  ++stat_taa_draws;
  if (!logged_taa_draw_detected) {
    logged_taa_draw_detected = true;
    std::stringstream s;
    s << "Far Cry 6 DLSS: detected TAA indexed draw 0x" << std::hex << hash;
    reshade::log::message(reshade::log::level::info, s.str().c_str());
  }
  return EvaluateDLSS(cmd_list, data, hash);
}

inline void OnInitDevice(reshade::api::device* device) {
  if (device == nullptr) return;
  NoteSeenDevice(device->get_native());
  {
    // Fresh heap objects deserve a fresh journal — but only on a genuinely
    // different native device. Same-native re-init (wrapper churn) keeps it.
    const uint64_t native = device->get_native();
    const uint64_t owner = capture_device_native.load();
    if (owner != 0u && owner != native) {
      std::lock_guard<std::mutex> lock(capture_mutex);
      ClearCapture();
      char buf[192];
      snprintf(buf, sizeof(buf),
               "Far Cry 6 DLSS: capture switched devices (0x%llX -> 0x%llX), journal reset",
               static_cast<unsigned long long>(owner), static_cast<unsigned long long>(native));
      reshade::log::message(reshade::log::level::info, buf);
    }
  }
  int vendor_id = 0;
  const bool is_nvidia = device->get_property(reshade::api::device_properties::vendor_id, &vendor_id) && vendor_id == 0x10de;
  {
    std::lock_guard<std::mutex> lock(device_mutex);
    if (is_nvidia) {
      nvidia_devices.insert(device->get_native());
      AddNvidiaNative(device->get_native());
      char buf[128];
      snprintf(buf, sizeof(buf), "nvidia+%zu", nvidia_devices.size());
      device_summary = buf;
    } else {
      char buf[160];
      snprintf(buf, sizeof(buf), "non-nvidia (vendor=0x%X)%s",
               static_cast<unsigned>(vendor_id),
               any_nvidia.load() ? ", nvidia also present" : "");
      if (!any_nvidia.load()) device_summary = buf;
    }
    any_nvidia.store(!nvidia_devices.empty());
  }
  std::stringstream s;
  if (is_nvidia) {
    s << "Far Cry 6 DLSS: NVIDIA device detected (" << device->get_native() << ")";
    reshade::log::message(reshade::log::level::info, s.str().c_str());
  } else {
    s << "Far Cry 6 DLSS: ignoring non-NVIDIA device, vendor=0x" << std::hex << vendor_id;
    reshade::log::message(reshade::log::level::info, s.str().c_str());
  }
}

inline void OnDestroyDevice(reshade::api::device* device) {
  if (device != nullptr) {
    bool was_ours = false;
    {
      std::lock_guard<std::mutex> lock(device_mutex);
      was_ours = nvidia_devices.erase(device->get_native()) > 0u;
      any_nvidia.store(!nvidia_devices.empty());
      if (nvidia_devices.empty()) device_summary = "none seen";
    }
    if (was_ours) ReleaseNgx();
    RemoveNvidiaNative(device->get_native());
    ForgetSeenDevice(device->get_native());
    // Rare by definition. The journal is intentionally NOT wiped here: if the
    // same native re-initializes (wrapper churn) its heaps are still alive.
    // A genuinely different native wipes on its init (see OnInitDevice).
    reshade::log::message(reshade::log::level::info, "Far Cry 6 DLSS: device destroyed");
  }
}

inline void OnPresent(
    reshade::api::command_queue*, reshade::api::swapchain*,
    const reshade::api::rect*, const reshade::api::rect*,
    uint32_t, const reshade::api::rect*) {
  // Present boundary timestamp (frame pacing bookkeeping for future use).
  last_present_time = std::chrono::steady_clock::now();
  ++stat_presents;
  // Tick down the time-boxed full-record fallback. The journal itself is NOT
  // cleared here: boot-time writes must persist until (and after) resolve.
  int frames = full_capture_frames.load();
  if (frames > 0) full_capture_frames.store(frames - 1);
}

inline void Use(DWORD fdw_reason) {
  // NOTE on copy_descriptor_tables: deliberately UNSUBSCRIBED (measured).
  // A prior build subscribed with an overlap-gated handler; FPS regressed
  // and no copy-fed heap ever converted to a deployment, so the subscription
  // bought cost without value. The update journal is the sole capture path.
  // utils::shader::Use attaches the pipeline/state tracking our draw
  // identification and layout resolution read (GetCurrentState,
  // GetCurrentPixelShaderHash, PopulateStageState). No replacements are
  // registered (tracking only). Owned here deliberately: without it the
  // addon only works when another addon (e.g. the HDR mod) attaches the
  // same tracker first.
  renodx::utils::shader::Use(fdw_reason);
  renodx::utils::pipeline_layout::Use(fdw_reason);
  renodx::utils::resource::Use(fdw_reason);

  switch (fdw_reason) {
    case DLL_PROCESS_ATTACH:
      if (attached) return;
      attached = true;
      last_present_time = std::chrono::steady_clock::now();
      reshade::register_event<reshade::addon_event::init_device>(OnInitDevice);
      reshade::register_event<reshade::addon_event::destroy_device>(OnDestroyDevice);
      reshade::register_event<reshade::addon_event::init_command_list>(OnInitCommandList);
      reshade::register_event<reshade::addon_event::destroy_command_list>(OnDestroyCommandList);
      reshade::register_event<reshade::addon_event::reset_command_list>(OnResetCommandList);
      reshade::register_event<reshade::addon_event::bind_render_targets_and_depth_stencil>(OnBindRenderTargetsAndDepthStencil);
      reshade::register_event<reshade::addon_event::push_descriptors>(OnPushDescriptors);
      reshade::register_event<reshade::addon_event::bind_descriptor_tables>(OnBindDescriptorTables);
      reshade::register_event<reshade::addon_event::update_descriptor_tables>(OnUpdateDescriptorTables);
      reshade::register_event<reshade::addon_event::bind_pipeline>(OnBindPipeline);
      reshade::register_event<reshade::addon_event::draw>(OnDraw);
      reshade::register_event<reshade::addon_event::draw_indexed>(OnDrawIndexed);
      reshade::register_event<reshade::addon_event::present>(OnPresent);
      break;
    case DLL_PROCESS_DETACH:
      if (!attached) return;
      attached = false;
      reshade::unregister_event<reshade::addon_event::init_device>(OnInitDevice);
      reshade::unregister_event<reshade::addon_event::destroy_device>(OnDestroyDevice);
      reshade::unregister_event<reshade::addon_event::init_command_list>(OnInitCommandList);
      reshade::unregister_event<reshade::addon_event::destroy_command_list>(OnDestroyCommandList);
      reshade::unregister_event<reshade::addon_event::reset_command_list>(OnResetCommandList);
      reshade::unregister_event<reshade::addon_event::bind_render_targets_and_depth_stencil>(OnBindRenderTargetsAndDepthStencil);
      reshade::unregister_event<reshade::addon_event::push_descriptors>(OnPushDescriptors);
      reshade::unregister_event<reshade::addon_event::bind_descriptor_tables>(OnBindDescriptorTables);
      reshade::unregister_event<reshade::addon_event::update_descriptor_tables>(OnUpdateDescriptorTables);
      reshade::unregister_event<reshade::addon_event::bind_pipeline>(OnBindPipeline);
      reshade::unregister_event<reshade::addon_event::draw>(OnDraw);
      reshade::unregister_event<reshade::addon_event::draw_indexed>(OnDrawIndexed);
      reshade::unregister_event<reshade::addon_event::present>(OnPresent);
      ReleaseNgx();
      break;
  }
}

}  // namespace farcry6_dlss::dlss
