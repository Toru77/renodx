/*
 * Copyright (C) 2026
 * SPDX-License-Identifier: MIT
 *
 * Far Cry 6 DLSS — replaces the game's TAA pass with NVIDIA DLSS Super
 * Resolution (D3D12 / NGX path).
 *
 * ── Verified against live DevKit captures ──
 *
 * Passes:
 *   0xFF0AF25B   TAA, one render target, sized at the input resolution
 *   0x4AD8172A   pause-menu TAA variant, two render targets; RTV1 carries
 *                alpha/coverage history and is left untouched
 *   0x0207BCBE   upscale, sampling the TAA output as t0 and writing the
 *                display-resolution target. This is the pass DLSS *upscaling*
 *                has to replace; the TAA pass itself is 1:1 (DLAA only).
 *
 * TAA inputs, all space0, slot -> content:
 *   t0/t1   RG8,  half the input resolution   packed motion vectors
 *   t2      FP16, input resolution            current color
 *   t3/t4   FP16, input resolution            history
 *   t5/t6   R8,   input resolution            depth candidates
 *   b0      CBV, 512 bytes                    knobs; the offset moves every
 *                                             frame inside one large buffer
 *
 * How the inputs are found:
 *   The engine assembles its per-frame descriptor tables with CopyDescriptors,
 *   so the bound slots are filled by copies and by nothing else. Measured: with
 *   only writes recorded, every TAA slot reported zero recorded descriptors in
 *   the heap it was bound from, while 25,880 writes landed in other heaps — and
 *   the engine then issues ~176M copy calls a session (one descriptor each,
 *   ~33,000 per frame) rebuilding per-frame tables whose positions advance every
 *   frame. Watching those copies is possible but costs frame time; watching
 *   their *result* needs no copies at all:
 *
 *     - A descriptor's bytes are a pure function of the view it holds, and
 *       CopyDescriptors copies them verbatim — so the same view has the same
 *       bytes in the pool slot where it was created and in every frame slot it
 *       is copied into. That is what makes them usable as a key.
 *     - Descriptor writes (CreateShaderResourceView / CreateConstantBufferView /
 *       CreateUnorderedAccessView arrive as update_descriptor_tables) are the
 *       moment the view is known, so the bytes at the written slot are recorded
 *       there, keyed to the view.
 *     - At the draw, the bytes at the bound slot are read and looked up. No copy
 *       subscription, no mirror, no position learning, and no assumption about
 *       how the engine rotates its tables.
 *
 *   The bytes are compared, never interpreted. Interpreting them was tried and
 *   does not work: they are driver-internal, not resource addresses — measured,
 *   the first eight bytes of live SRV descriptors are not even 4-byte aligned,
 *   and two different textures shared their low 32 bits. ReShade's descriptor
 *   size matches the driver's increment, so the slot read was the slot intended.
 *
 *   Layout ranges carry a descriptor type, and it must be filtered on: sampler
 *   ranges live in space 0 at register 0 too, and otherwise displace the real
 *   table from the candidate list.
 *
 *   The packed motion vectors are half the input resolution, so the unpack pass
 *   indexes that buffer at its own size. Reading it at input-resolution
 *   coordinates runs past the end of the texture, where D3D returns 0, which
 *   decodes to roughly -0.99 - a ~126px motion vector over three quarters of
 *   the frame.
 *
 *   t5/t6 are NOT depth. The reference shader uses them as an anti-ghosting
 *   mask (max of a current-UV and a reprojected-UV sample, subtracted from a
 *   velocity-driven blend factor), so feeding them to NGX as pInDepth gives
 *   DLSS a signal with the wrong meaning. A real depth source is still open.
 */

#pragma once

#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <include/reshade.hpp>
#include <nvsdk_ngx.h>
#include <nvsdk_ngx_helpers.h>
#include <wrl/client.h>

#include "../../utils/bitwise.hpp"
#include "../../utils/build_info.hpp"
#include "../../utils/pipeline_layout.hpp"
#include "../../utils/shader.hpp"

namespace farcry6_dlss::dlss {

static constexpr uint32_t kTaaMainHash = 0xFF0AF25Bu;
static constexpr uint32_t kTaaPauseHash = 0x4AD8172Au;

static constexpr uint32_t kTrackedTableCount = 32u;
static constexpr uint32_t kTrackedRtvCount = 4u;

// TAA slots consumed by DLSS (space0).
static constexpr uint32_t kSlotPackedMv = 0u;  // t0: packed motion vectors
static constexpr uint32_t kSlotColor = 2u;     // t2: current scene color
static constexpr uint32_t kSlotDepthA = 5u;    // t5: depth candidate
static constexpr uint32_t kSlotDepthB = 6u;    // t6: depth candidate
static constexpr uint32_t kSlotCbv = 0u;       // b0: TAA knobs

// ── Settings (written by the settings system in addon.cpp) ──
inline float dlss_enabled = 1.f;
// 0=Auto, 1=DLAA, 2=Quality, 3=Balanced, 4=Performance, 5=UltraPerformance
inline float dlss_mode = 0.f;
// 0=Default, 1=F, 2=J, 3=K, 4=L, 5=M
inline float dlss_render_preset = 0.f;
// 0=Zero, 1=read the jitter the game applies (not wired yet: still zero)
inline float dlss_jitter_source = 0.f;
// What feeds NGX as depth, for isolating a rendering fault:
//   0 = Scene depth   the game's own depth buffer, identified by how it is bound
//   1 = t5 mask       the TAA anti-ghosting mask, which is not depth
//   2 = t6 mask       likewise
//   3 = Placeholder   far plane, so nothing occludes anything
inline float dlss_depth_source = 0.f;
// What feeds NGX as colour: 0 = the game's scene colour, 1 = flat mid grey.
inline float dlss_color_source = 0.f;
// What feeds NGX as motion: 0 = decoded game motion, 1 = zero motion.
inline float dlss_motion_source = 0.f;
// Writes a known-good image through the same output plumbing with NGX not
// involved at all, so artifacts that survive this are ours and not DLSS's.
inline float dlss_bypass_ngx = 0.f;
inline float dlss_motion_vectors_jittered = 1.f;
inline float dlss_depth_inverted = 0.f;
// Declares the colour input as HDR, i.e. as values above 1.0. Off preserves the
// behaviour of every previous build; A/B it rather than assume.
inline float dlss_flag_is_hdr = 0.f;
// Re-applies the game's descriptor heaps and root signature after each NGX
// evaluation. NGX dispatches on the native command list, behind ReShade's hooks,
// and the descriptor heap binding is the piece of state it cannot avoid
// disturbing that graphics and compute share.
inline float dlss_restore_state = 1.f;
// Transitions the scene depth into a shader-readable state for the read and back
// afterwards. The engine leaves it in a depth-stencil state, in which a shader
// resource read returns zeros.
inline float dlss_transition_depth = 1.f;
// Calibration multiplier for the decoded motion vectors.
inline float dlss_mv_scale = 1.f;
// Motion vector sign: a flipped sign misreprojects exactly as badly as a wrong
// scale, and there was no way to test it - the scale range has no negatives.
inline float dlss_mv_flip = 0.f;
// Reset the history on every evaluation. If the image is identical with this on, the
// history was already being discarded every frame; if it gets worse, history is
// accumulating and the fault is in what it accumulates from.
inline float dlss_reset_every_frame = 0.f;
// Engines commonly store the offset to undo rather than the one applied - the same
// trap the motion vectors had. A wrong sign doubles the reprojection error instead
// of correcting it, which is a visible failure rather than a subtle one.
inline float dlss_jitter_flip = 0.f;
inline float dlss_debug_logging = 0.f;
// What DLSS is allowed to replace, for isolating a rendering fault:
//   0 = Replace         skip the TAA draw and copy DLSS output into its target
//   1 = Evaluate only   run the whole chain, keep the game's own TAA output
//   2 = Resolve only    resolve inputs and report; no unpack, no eval
inline float dlss_output_mode = 0.f;

// ── Status ──
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
static constexpr int kFailPlaceholder = 10;

inline const char* FailCodeText(int code) {
  switch (code) {
    case kFailNone: return "ok";
    case kFailDevice: return "no NVIDIA device";
    case kFailB0: return "b0 constants not resolved";
    case kFailResolve: return "TAA inputs not resolved";
    case kFailSize: return "input/target size mismatch";
    case kFailFormat: return "unsupported TAA target format";
    case kFailNgxInit: return "NGX init failed (see log)";
    case kFailFeature: return "NGX feature creation failed (see log)";
    case kFailUnpack: return "MV unpack failed (see log)";
    case kFailEval: return "NGX evaluation failed (see log)";
    case kFailPlaceholder: return "input placeholder creation failed";
    default: return "unknown";
  }
}

// The single "state" code is the most recent failure, and a later success
// overwrites it — so an intermittent rejection looks like "ok". Counted per
// reason, an intermittent rejection is a number.
static constexpr uint32_t kFailCodeCount = 11u;
inline std::atomic<uint64_t> stat_fail_counts[kFailCodeCount];

inline std::atomic<uint64_t> stat_taa_draws{0};
inline std::atomic<uint64_t> stat_dlss_ok{0};
inline std::atomic<uint64_t> stat_presents{0};
inline std::atomic<int> stat_fail_code{kFailNone};
// Per-slot resolve result of the last TAA draw: 0=miss, 1=real, read by the UI.
inline std::atomic<int> slot_hit[5];

inline void NoteFail(int code) {
  stat_fail_code.store(code);
  if (code >= 0 && static_cast<uint32_t>(code) < kFailCodeCount) {
    stat_fail_counts[code].fetch_add(1u, std::memory_order_relaxed);
  }
}
inline void NoteSuccess() {
  ++stat_dlss_ok;
  stat_fail_code.store(kFailNone);
}

// ── Device tracking ──
// The engine exposes more than one device (a WARP adapter alongside the real
// GPU), so support is per-device and sticky: a non-NVIDIA init must never
// disable an NVIDIA one.
inline std::mutex device_mutex;
inline std::unordered_set<uint64_t> nvidia_devices;
inline std::atomic<bool> any_nvidia{false};
inline std::string device_summary = "none seen";
inline bool attached = false;

// Lock-free copy of the NVIDIA natives, for the draw path.
inline std::atomic<uint64_t> nvidia_devs[4];

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

inline std::string DeviceSummary() {
  std::lock_guard<std::mutex> lock(device_mutex);
  return device_summary;
}

// ── Descriptor fingerprints ──
// A descriptor's bytes are a pure function of the view it holds, and
// CopyDescriptors copies them verbatim — so the same view has the same bytes in
// the pool slot where it was created and in every frame slot it is later copied
// into. Those bytes cannot be *interpreted*: they are driver-internal, and the
// probe proved it (not even 4-byte aligned, and two different textures shared
// their low 32 bits). They can be *compared*, and that is all this needs.
//
// Record them where the view is known — the write events, tens of thousands per
// session — and look them up where the view is needed, at the draw. No copy
// subscription, no mirror, no position learning, and no assumption about the
// engine's table rotation, because the frame slot's bytes are the pool slot's
// bytes by construction.
// The key is the whole descriptor slot, not a prefix of it. A prefix was tried
// and collided for 27% of the pool: the resource address a texture view points
// at is not in the first bytes — the upper half of the first 16 was byte-for-byte
// identical across four different textures. Bytes past the driver's increment
// stay zero, so the key is deterministic.
static constexpr uint32_t kFingerprintBytes = 64u;
static constexpr uint32_t kFingerprintWords = kFingerprintBytes / sizeof(uint64_t);

struct DescriptorFingerprint {
  std::array<uint64_t, kFingerprintWords> words = {};
};

inline bool operator==(const DescriptorFingerprint& lhs, const DescriptorFingerprint& rhs) {
  return lhs.words == rhs.words;
}

struct DescriptorFingerprintHash {
  size_t operator()(const DescriptorFingerprint& key) const {
    uint64_t hash = 0x9E3779B97F4A7C15ull;
    for (const uint64_t word : key.words) {
      hash ^= word + 0x9E3779B97F4A7C15ull + (hash << 6u) + (hash >> 2u);
    }
    return static_cast<size_t>(hash);
  }
};

struct RecordedDescriptor {
  reshade::api::descriptor_type type = reshade::api::descriptor_type::sampler;
  // Resolved at record time, deliberately. A ReShade resource_view handle is the
  // CPU address of the descriptor slot the view was created at, and the engine
  // reuses those slots — so a stored handle can later name a *different* view's
  // resource. That is what produced a motion-vector slot resolving to a
  // 1024x1024 texture. The resource and the view's format are what the draw
  // actually needs, and they cannot go stale.
  reshade::api::resource resource = {};
  reshade::api::format format = reshade::api::format::unknown;
  // Captured at record time for the same reason the resource is: querying a
  // destroyed resource later returns an empty description, which is
  // indistinguishable from a slot genuinely changing resource.
  uint32_t width = 0u;
  uint32_t height = 0u;
  reshade::api::buffer_range range = {};
  bool has_resource = false;
  bool has_range = false;
};

// The first genuine collision, kept for the report: two records that produce the
// same key while resolving to different resources. Their bytes are identical by
// definition, so what matters is *what* differs.
struct FingerprintCollision {
  bool captured = false;
  uint32_t byte_count = 0u;
  uint64_t first_resource = 0u;
  uint64_t second_resource = 0u;
  bool same_resource = false;
};

inline std::mutex fingerprint_mutex;
inline std::unordered_map<DescriptorFingerprint, RecordedDescriptor, DescriptorFingerprintHash> fingerprints;
// Reverse index: which fingerprints name a given resource. Needed because a
// stored resource handle outlives its resource — a renderer rebuild (a
// fullscreen transition does this) destroys render targets, and a fingerprint
// left pointing at one would hand NGX a freed resource.
inline std::unordered_multimap<uint64_t, DescriptorFingerprint> fingerprints_by_resource;
inline FingerprintCollision fingerprint_collision;
inline std::atomic<uint32_t> descriptor_stride_bytes{0};
inline std::atomic<uint64_t> stat_fingerprints{0};
inline std::atomic<uint64_t> stat_fingerprint_collisions{0};
inline std::atomic<uint64_t> stat_fingerprint_misses{0};
inline std::atomic<uint64_t> stat_fingerprint_invalidated{0};

// "The same view" is the same resource and the same view format. It is not the
// view handle: that is a slot address, and comparing slot addresses made the same
// view created at two slots look like two different views.
inline bool SameRecordedView(const RecordedDescriptor& lhs, const RecordedDescriptor& rhs) {
  if (lhs.has_resource != rhs.has_resource || lhs.has_range != rhs.has_range) return false;
  if (lhs.has_range) {
    return lhs.range.buffer.handle == rhs.range.buffer.handle && lhs.range.offset == rhs.range.offset;
  }
  return lhs.resource.handle == rhs.resource.handle && lhs.format == rhs.format;
}

inline void RecordFingerprint(const DescriptorFingerprint& key, const RecordedDescriptor& record) {
  const std::lock_guard<std::mutex> lock(fingerprint_mutex);
  const auto it = fingerprints.find(key);
  if (it == fingerprints.end()) {
    fingerprints.emplace(key, record);
    if (record.has_resource) fingerprints_by_resource.emplace(record.resource.handle, key);
    stat_fingerprints.store(fingerprints.size());
    return;
  }
  if (SameRecordedView(it->second, record)) return;  // the same view, seen again
  stat_fingerprint_collisions.fetch_add(1u, std::memory_order_relaxed);
  if (!fingerprint_collision.captured) {
    fingerprint_collision.captured = true;
    fingerprint_collision.byte_count = descriptor_stride_bytes.load();
    fingerprint_collision.first_resource = it->second.resource.handle;
    fingerprint_collision.second_resource = record.resource.handle;
    fingerprint_collision.same_resource = it->second.resource.handle != 0u
        && it->second.resource.handle == record.resource.handle;
  }
}

// Canonicalizes a (table, binding) pair to the backing (heap, offset). Both the
// CPU-form handles ReShade reports for view creation and the GPU-form handles
// binds use land in this space.
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

// Reads the bytes a bound slot actually holds. This is the one place the addon
// touches descriptor memory, and it never looks at the contents — only copies
// them out to be compared.
inline bool ReadSlotFingerprint(
    reshade::api::device* device,
    reshade::api::descriptor_table table,
    uint32_t binding,
    DescriptorFingerprint& out) {
  out = {};
  if (device == nullptr || table.handle == 0u) return false;
  if (device->get_api() != reshade::api::device_api::d3d12) return false;

  uint64_t heap = 0u;
  uint32_t offset = 0u;
  if (!CanonicalBinding(device, table, binding, heap, offset)) return false;
  auto* native_heap = reinterpret_cast<ID3D12DescriptorHeap*>(heap);
  auto* native_device = reinterpret_cast<ID3D12Device*>(device->get_native());
  if (native_heap == nullptr || native_device == nullptr) return false;

  // Heap type, bounds and CPU base are checked; shader visibility is NOT,
  // because it is not relevant here and demanding it was a bug: the engine
  // creates its views in a non-shader-visible staging heap and copies them into
  // the frame tables afterwards. That flag controls GPU visibility only — a heap
  // descriptors are written into is CPU-addressable by definition, which is the
  // only property this read needs.
  const D3D12_DESCRIPTOR_HEAP_DESC heap_desc = native_heap->GetDesc();
  if (heap_desc.Type != D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) return false;
  if (offset >= heap_desc.NumDescriptors) return false;

  const D3D12_CPU_DESCRIPTOR_HANDLE base = native_heap->GetCPUDescriptorHandleForHeapStart();
  if (base.ptr == 0) return false;
  const UINT stride = native_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
  if (stride == 0u) return false;
  descriptor_stride_bytes.store(stride);
  const uint32_t readable = std::min<uint32_t>(stride, kFingerprintBytes);
  const auto* slot = reinterpret_cast<const uint8_t*>(base.ptr) + static_cast<size_t>(offset) * stride;
  std::memcpy(out.words.data(), slot, readable);
  return true;
}

inline bool LookupFingerprint(const DescriptorFingerprint& key, RecordedDescriptor& out) {
  const std::lock_guard<std::mutex> lock(fingerprint_mutex);
  const auto it = fingerprints.find(key);
  if (it == fingerprints.end()) return false;
  out = it->second;
  return true;
}

inline uint64_t CountFingerprints() {
  const std::lock_guard<std::mutex> lock(fingerprint_mutex);
  return fingerprints.size();
}

inline FingerprintCollision GetFingerprintCollision() {
  const std::lock_guard<std::mutex> lock(fingerprint_mutex);
  return fingerprint_collision;
}

// ── Scene depth read state ──
// The scene depth is what the scene renders *into*: the engine leaves it in a
// depth-stencil state, and a shader resource read of a resource in a depth state
// is invalid and returns zeros — which is what a black depth view in DLSS's own
// debug overlay looks like. NGX reads it as a shader resource, so it has to be
// transitioned for the read and put back into the state the engine's own tracking
// believes it is in: the engine writes its next barrier against that belief.
//
// Only the handful of resources we ever feed NGX as depth are tracked, so the
// cost is a fixed number of compares per barrier the engine issues, plus one
// store when it is one of ours.
static constexpr uint32_t kTrackedDepthSlots = 4u;
inline std::atomic<uint64_t> tracked_depth_resources[kTrackedDepthSlots];
inline std::atomic<uint32_t> tracked_depth_states[kTrackedDepthSlots];
inline std::atomic<uint32_t> tracked_depth_next{0u};
inline std::atomic<uint64_t> stat_depth_transitions{0u};

inline void OnBarrier(
    reshade::api::command_list*, uint32_t count,
    const reshade::api::resource* resources,
    const reshade::api::resource_usage*,
    const reshade::api::resource_usage* new_states) {
  if (count == 0u || resources == nullptr || new_states == nullptr) return;
  bool any_known = false;
  for (uint32_t slot = 0u; slot < kTrackedDepthSlots && !any_known; ++slot) {
    if (tracked_depth_resources[slot].load(std::memory_order_relaxed) != 0u) any_known = true;
  }
  if (!any_known) return;
  for (uint32_t i = 0u; i < count; ++i) {
    if (resources[i].handle == 0u) continue;
    for (uint32_t slot = 0u; slot < kTrackedDepthSlots; ++slot) {
      if (tracked_depth_resources[slot].load(std::memory_order_relaxed) != resources[i].handle) continue;
      tracked_depth_states[slot].store(
          static_cast<uint32_t>(new_states[i]), std::memory_order_relaxed);
    }
  }
}

// Registers a resource whose state we care about. A depth buffer the engine
// ping-pongs still keeps its state this way, instead of being unknown every frame.
// A resource is created in a state the engine chooses, and a texture it only ever
// uses as a UAV may never be transitioned afterwards - so creation is the only
// place its state is ever stated. That is still the engine's own answer.
inline std::mutex initial_state_mutex;
inline std::unordered_map<uint64_t, uint32_t> initial_states;

inline void OnInitResource(
    reshade::api::device*, const reshade::api::resource_desc&, const reshade::api::subresource_data*,
    reshade::api::resource_usage initial_state, reshade::api::resource resource) {
  if (resource.handle == 0u) return;
  const std::lock_guard<std::mutex> lock(initial_state_mutex);
  if (initial_states.size() > 8192u) initial_states.clear();
  initial_states[resource.handle] = static_cast<uint32_t>(initial_state);
}

inline void TrackDepthResource(uint64_t resource) {
  if (resource == 0u) return;
  for (uint32_t slot = 0u; slot < kTrackedDepthSlots; ++slot) {
    if (tracked_depth_resources[slot].load(std::memory_order_relaxed) == resource) return;
  }
  uint32_t initial = static_cast<uint32_t>(reshade::api::resource_usage::undefined);
  {
    const std::lock_guard<std::mutex> lock(initial_state_mutex);
    const auto it = initial_states.find(resource);
    if (it != initial_states.end()) initial = it->second;
  }
  const uint32_t slot = tracked_depth_next.fetch_add(1u, std::memory_order_relaxed) % kTrackedDepthSlots;
  tracked_depth_resources[slot].store(resource, std::memory_order_relaxed);
  tracked_depth_states[slot].store(initial, std::memory_order_relaxed);
}

// The state the engine last put a tracked depth in. Unknown means it has not been
// transitioned since it was selected; it is bound as a depth-stencil view, so a
// write state is what it will be.
inline reshade::api::resource_usage TrackedDepthState(uint64_t resource) {
  for (uint32_t slot = 0u; slot < kTrackedDepthSlots; ++slot) {
    if (tracked_depth_resources[slot].load(std::memory_order_relaxed) != resource) continue;
    return static_cast<reshade::api::resource_usage>(
        tracked_depth_states[slot].load(std::memory_order_relaxed));
  }
  return reshade::api::resource_usage::undefined;
}

// Names every usage bit that is set, since the states that matter here are
// combinations: an engine keeping a depth buffer readable in compute while it is
// still the depth target reports depth-write|shader-resource-non-pixel, and a
// single-name switch would call that "other".
inline std::string ResourceStateName(reshade::api::resource_usage state) {
  struct Bit {
    uint32_t value;
    const char* name;
  };
  static const Bit kBits[] = {
      {static_cast<uint32_t>(reshade::api::resource_usage::depth_stencil_write), "depth-write"},
      {static_cast<uint32_t>(reshade::api::resource_usage::depth_stencil_read), "depth-read"},
      {static_cast<uint32_t>(reshade::api::resource_usage::shader_resource_pixel), "shader-resource-pixel"},
      {static_cast<uint32_t>(reshade::api::resource_usage::shader_resource_non_pixel), "shader-resource-non-pixel"},
      {static_cast<uint32_t>(reshade::api::resource_usage::render_target), "render-target"},
      {static_cast<uint32_t>(reshade::api::resource_usage::unordered_access), "unordered-access"},
      {static_cast<uint32_t>(reshade::api::resource_usage::copy_source), "copy-source"},
      {static_cast<uint32_t>(reshade::api::resource_usage::copy_dest), "copy-dest"},
      {static_cast<uint32_t>(reshade::api::resource_usage::vertex_buffer), "vertex-buffer"},
      {static_cast<uint32_t>(reshade::api::resource_usage::index_buffer), "index-buffer"},
      {static_cast<uint32_t>(reshade::api::resource_usage::constant_buffer), "constant-buffer"},
      {static_cast<uint32_t>(reshade::api::resource_usage::indirect_argument), "indirect-argument"},
      {static_cast<uint32_t>(reshade::api::resource_usage::stream_output), "stream-output"},
      {static_cast<uint32_t>(reshade::api::resource_usage::acceleration_structure), "acceleration-structure"},
  };
  const uint32_t bits = static_cast<uint32_t>(state);
  if (bits == 0u) return "undefined";
  std::string out;
  uint32_t remaining = bits;
  for (const Bit& bit : kBits) {
    if ((bits & bit.value) == 0u) continue;
    if (!out.empty()) out += '|';
    out += bit.name;
    remaining &= ~bit.value;
  }
  if (remaining != 0u) {
    char tail[32];
    snprintf(tail, sizeof(tail), "%s0x%X", out.empty() ? "" : "|", remaining);
    out += tail;
  }
  return out;
}

// A destroyed resource takes every fingerprint that named it with it. Without
// this, a renderer rebuild leaves fingerprints pointing at freed resources and
// NGX reads them.
// The constant ring is an upload buffer, so it can be read directly. Mapping it once
// and reading from the pointer turns a probe into a memcpy instead of a Map/Unmap
// pair, which is what makes it affordable to probe *every* write rather than a
// capped few. The cap was the whole problem: the camera's buffer is one of hundreds
// written per frame, so it was reached on about two percent of frames, and the
// jitter was therefore almost always zero.
struct MappedRing {
  uint64_t handle = 0u;
  ID3D12Resource* resource = nullptr;
  const uint8_t* data = nullptr;
  uint64_t size = 0u;
};

inline MappedRing mapped_ring;
inline std::mutex mapped_ring_mutex;

inline const uint8_t* MappedRingData(uint64_t handle) {
  // Fast path without the lock: the ring is one long-lived buffer, so this hits
  // almost always, and a mutex per constant-buffer write was measurable.
  if (handle != 0u && mapped_ring.handle == handle && mapped_ring.data != nullptr) return mapped_ring.data;
  const std::lock_guard<std::mutex> lock(mapped_ring_mutex);
  if (handle == 0u) return nullptr;
  if (mapped_ring.handle == handle && mapped_ring.data != nullptr) return mapped_ring.data;
  // A different buffer: drop the old mapping without unmapping it, since that
  // resource is on its way out (the engine owns it) and Unmap on a destroyed
  // resource is not something to do.
  if (mapped_ring.resource != nullptr) mapped_ring = {};
  auto* resource = reinterpret_cast<ID3D12Resource*>(handle);
  if (resource == nullptr) return nullptr;
  void* mapped = nullptr;
  // A null range declares that the whole resource may be read, which is the case.
  if (FAILED(resource->Map(0u, nullptr, &mapped)) || mapped == nullptr) return nullptr;
  mapped_ring.handle = handle;
  mapped_ring.resource = resource;
  mapped_ring.data = static_cast<const uint8_t*>(mapped);
  mapped_ring.size = resource->GetDesc().Width;
  return mapped_ring.data;
}


inline void OnDestroyResource(reshade::api::device* device, reshade::api::resource resource) {
  if (device == nullptr || resource.handle == 0u) return;
  {
    const std::lock_guard<std::mutex> lock(initial_state_mutex);
    initial_states.erase(resource.handle);
  }
  if (mapped_ring.handle == resource.handle) mapped_ring = {};
  // A recycled handle must not inherit the state of the resource that used to
  // live at that address.
  for (uint32_t slot = 0u; slot < kTrackedDepthSlots; ++slot) {
    if (tracked_depth_resources[slot].load(std::memory_order_relaxed) != resource.handle) continue;
    tracked_depth_resources[slot].store(0u, std::memory_order_relaxed);
    tracked_depth_states[slot].store(
        static_cast<uint32_t>(reshade::api::resource_usage::undefined), std::memory_order_relaxed);
  }
  const std::lock_guard<std::mutex> lock(fingerprint_mutex);
  const auto range = fingerprints_by_resource.equal_range(resource.handle);
  uint64_t removed = 0u;
  for (auto it = range.first; it != range.second; ++it) {
    removed += fingerprints.erase(it->second);
  }
  fingerprints_by_resource.erase(range.first, range.second);
  if (removed != 0u) {
    stat_fingerprints.store(fingerprints.size());
    stat_fingerprint_invalidated.fetch_add(removed, std::memory_order_relaxed);
  }
}

// Descriptor writes. CreateShaderResourceView / CreateConstantBufferView /
// CreateUnorderedAccessView are the only D3D12 entry points ReShade turns into
// update_descriptor_tables, and they are the moment the view is known.
// ── Jitter, read from the engine's own projection ──
// A renderer jitters by shifting normalised-device coordinates, and a shift in NDC
// has to be proportional to view depth: with clip.x = m00*x + m02*z and
// clip.w = m32*z, the shift is m02/m32. A perspective projection leaves m02 at
// zero unless something deliberately put a value there, which makes (m02, m12) the
// one place in the frame where the jitter exists as data. The value is therefore
// read, never guessed from its size or movement, and the buffer it was read from
// has to keep the same shape before the value is used.
//
// Buffers are read when the engine writes their descriptor, while the data is
// fresh, and the one that matched is re-read once per frame afterwards. Probing is
// capped per frame and stops entirely once a projection is in hand.
struct ProjectionJitter {
  bool valid = false;
  bool transposed = false;   // storage order of the matching matrix
  uint64_t buffer = 0u;
  uint64_t offset = 0u;
  uint32_t size = 0u;
  float base_xx = 0.f;       // [0][0]
  float base_yy = 0.f;       // [1][1]
  float base_zz = 0.f;       // [2][2]
  float base_zw = 0.f;       // [2][3]
  float ndc_x = 0.f;
  float ndc_y = 0.f;
  float pixels_x = 0.f;
  float pixels_y = 0.f;
  float last_pixels_x = 0.f;  // to notice a buffer that stopped changing: stale
  float last_pixels_y = 0.f;
  uint32_t frozen_frames = 0u;
  bool matched_this_frame = false;  // probing found it in this frame
  uint32_t hint_size = 0u;          // its constant buffer's declared size
  uint64_t relative_offset = 0u;    // where the matrix sits inside that buffer
  uint64_t probes = 0u;
  uint64_t adoptions = 0u;
  uint64_t losses = 0u;
};

inline ProjectionJitter projection_jitter;
static constexpr uint32_t kProjectionProbesPerFrame = 4u;
static constexpr uint32_t kProjectionMaxBytes = 65536u;
inline uint32_t projection_probes_left = kProjectionProbesPerFrame;
// A full scan reads and walks a whole constant buffer; when it finds nothing it is
// not worth repeating every frame, so attempts are spaced out.
inline uint32_t projection_scan_cooldown = 0u;
// Aspect ratio of the render extent, set once per frame from the evaluation. Zero
// disables the aspect test rather than accepting every matrix.
inline float projection_aspect_xx = 0.f;

// Tests sixteen consecutive floats as a perspective projection in either storage
// order and, if the shape matches, returns the two entries that can only be a
// deliberate NDC shift.
inline bool MatchProjection(
    const float* m, bool transposed, float& out_ndc_x, float& out_ndc_y,
    float& out_base_xx, float& out_base_yy, float& out_base_zz, float& out_base_zw) {  const auto at = [&](uint32_t row, uint32_t column) {
    return m[transposed ? (column * 4u + row) : (row * 4u + column)];
  };
  const auto near_zero = [](float value, float epsilon) { return fabsf(value) <= epsilon; };

  // The w row of a perspective projection: nothing, the depth sign, nothing.
  // Every test below is a rejection, and every comparison against NaN is false, so
  // a buffer of NaNs would pass all of them. Finiteness comes first.
  for (uint32_t i = 0u; i < 16u; ++i) {
    if (!std::isfinite(m[i])) return false;
  }
  if (!near_zero(at(3u, 0u), 1e-5f) || !near_zero(at(3u, 1u), 1e-5f) || !near_zero(at(3u, 3u), 1e-5f)) return false;
  if (fabsf(fabsf(at(3u, 2u)) - 1.0f) > 1e-3f) return false;
  // The x and y rows carry their focal scale and, apart from the jitter, nothing.
  if (!near_zero(at(0u, 1u), 1e-5f) || !near_zero(at(0u, 3u), 1e-5f)) return false;
  if (!near_zero(at(1u, 0u), 1e-5f) || !near_zero(at(1u, 3u), 1e-5f)) return false;

  const float xx = at(0u, 0u);
  const float yy = at(1u, 1u);
  const float zz = at(2u, 2u);
  const float zw = at(2u, 3u);
  if (fabsf(xx) < 0.01f || fabsf(yy) < 0.01f) return false;
  // [2][2] is deliberately not required: an infinite-far projection leaves it at
  // zero, and that is what a modern renderer uses. The near/far term below is the
  // part every projection has.
  (void)zz;
  // A projection maps depth onto something: without a near/far term at [2][3] this
  // is a rotation or an identity with a translation - both of which share the
  // (0, 0, +/-1, 0) pattern above and are common in the same buffers (the minimap's
  // own transform matched here before this test existed).
  if (fabsf(zw) < 1e-4f) return false;

  // The focal scales are in proportion to the aspect ratio, which is what makes
  // this a projection of this frame's viewport rather than any other matrix.
  if (projection_aspect_xx > 0.01f) {
    const float ratio = fabsf(yy / xx);
    if (fabsf(ratio - projection_aspect_xx) > 0.05f) return false;
  }

  const float w = at(3u, 2u);
  const float nx = at(0u, 2u) / w;
  const float ny = at(1u, 2u) / w;
  // A jitter is sub-pixel; anything larger is not one.
  if (fabsf(nx) > 0.002f || fabsf(ny) > 0.002f) return false;

  out_ndc_x = nx;
  out_ndc_y = ny;
  out_base_xx = xx;
  out_base_yy = yy;
  out_base_zz = zz;
  out_base_zw = zw;
  return true;
}

// Reads one constant buffer and reports whether it holds a perspective projection.
inline bool ReadProjectionFromBuffer(const reshade::api::buffer_range& range, ProjectionJitter& out) {
  out = {};
  if (range.buffer.handle == 0u) return false;
  // Read from the mapping rather than mapping again: a second Map on a resource
  // this code already holds open fails, which is what silently stopped the
  // discovery from ever finding anything.
  const uint8_t* mapped = MappedRingData(range.buffer.handle);
  if (mapped == nullptr) return false;
  uint64_t begin = range.offset == UINT64_MAX ? 0u : range.offset;
  if (begin >= mapped_ring.size) return false;
  uint64_t bytes = range.size == UINT64_MAX ? 0u : range.size;
  if (bytes == 0u || bytes > kProjectionMaxBytes) bytes = kProjectionMaxBytes;
  if (begin + bytes > mapped_ring.size) bytes = mapped_ring.size - begin;
  if (bytes < 16u * sizeof(float)) return false;
  const auto* floats = reinterpret_cast<const float*>(mapped + begin);
  const uint32_t count = static_cast<uint32_t>(bytes / sizeof(float));

  for (uint32_t block = 0u; block + 16u <= count; ++block) {
    const float* m = floats + static_cast<size_t>(block) * 4u;
    for (uint32_t order = 0u; order < 2u; ++order) {
      float nx = 0.f, ny = 0.f, xx = 0.f, yy = 0.f, zz = 0.f, zw = 0.f;
      if (!MatchProjection(m, order != 0u, nx, ny, xx, yy, zz, zw)) continue;
      out.valid = true;
      out.transposed = order != 0u;
      out.buffer = range.buffer.handle;
      out.offset = begin + static_cast<uint64_t>(block) * 4u * sizeof(float);
      out.relative_offset = out.offset - begin;
      out.size = static_cast<uint32_t>(bytes);
      out.base_xx = xx;
      out.base_yy = yy;
      out.base_zz = zz;
      out.base_zw = zw;
      out.ndc_x = nx;
      out.ndc_y = ny;
      return true;
    }
  }
  return false;
}

// Called when the engine writes a constant buffer descriptor: the projection is
// recognised while its data is fresh, instead of scanning the ring for something
// that looks like a matrix.
inline void ProbeConstantBufferForProjection(const reshade::api::buffer_range& range) {
  if (range.buffer.handle == 0u || range.size == UINT64_MAX) return;
  // Without an aspect to compare against, an orthographic projection - a shadow
  // matrix, say - matches the shape test and would be adopted as the camera.
  if (projection_aspect_xx <= 0.01f) return;
  if (range.size < 16u * sizeof(float) || range.size > kProjectionMaxBytes) return;

  if (projection_jitter.hint_size != 0u && !projection_jitter.matched_this_frame) {
    // Its size is known but this frame has not found it yet: only writes of that
    // size are candidates, and of those only the sixteen floats the matrix occupies
    // are read - which is what makes it affordable to check every one of them.
    // The base is deliberately not required to match here. It legitimately moves
    // when the field of view changes - a scope, sights - and requiring it froze the
    // record at the first FOV captured, rejecting every frame afterwards. What is
    // required is the shape, the aspect, and later the sub-pixel magnitude; a wrong
    // match is almost always a static projection, whose jitter is zero anyway.
    if (range.size == projection_jitter.hint_size) {
      const uint8_t* data = MappedRingData(range.buffer.handle);
      if (data != nullptr && range.offset != UINT64_MAX) {
        const uint64_t at = range.offset + projection_jitter.relative_offset;
        if (at + 16u * sizeof(float) <= mapped_ring.size) {
          float m[16] = {};
          std::memcpy(m, data + at, sizeof(m));
          float nx = 0.f, ny = 0.f, xx = 0.f, yy = 0.f, zz = 0.f, zw = 0.f;
          if (MatchProjection(m, projection_jitter.transposed, nx, ny, xx, yy, zz, zw)) {
            projection_jitter.ndc_x = nx;
            projection_jitter.ndc_y = ny;
            projection_jitter.base_xx = xx;
            projection_jitter.base_yy = yy;
            projection_jitter.matched_this_frame = true;
            ++projection_jitter.probes;
            return;
          }
        }
      }
    }
    // Nothing matched at the known place, so the record is re-found from scratch.
  }

  if (projection_jitter.matched_this_frame || projection_probes_left == 0u) return;
  // Spacing out scans only makes sense once there is an identity to keep: while the
  // camera's buffer has never been found, every frame is used to look for it, and
  // only after a find is lost is the expensive path throttled.
  if (projection_jitter.hint_size != 0u) {
    if (projection_scan_cooldown > 0u) {
      --projection_scan_cooldown;
      return;
    }
    projection_scan_cooldown = 60u;
  }
  --projection_probes_left;
  ProjectionJitter found;
  if (!ReadProjectionFromBuffer(range, found)) return;
  found.adoptions = projection_jitter.adoptions + 1u;
  found.losses = projection_jitter.losses;
  found.probes = projection_jitter.probes;
  found.hint_size = projection_jitter.hint_size != 0u
      ? projection_jitter.hint_size
      : static_cast<uint32_t>(range.size);
  found.matched_this_frame = true;
  projection_jitter = found;
}

// Re-reads the buffer the projection was found in. The engine rewrites it every
// frame, so this is what keeps the value current; a buffer that no longer has the
// same shape, or that stops changing at all (a stale slot), is dropped and found
// again from the descriptors the engine writes next.
// ── Jitter: read the camera's projection every frame ──
// The constant ring relocates its contents every frame, so nothing that remembers
// *where* the matrix was can be trusted - that assumption has failed in four
// different forms now (offset, size hint, probe budget, scan cooldown). What is
// stable is the *search*: one comparison per float4 over a bounded window costs
// tens of microseconds, which is affordable every single frame and removes the
// entire class of scheduling failures. It is also the design that first produced a
// real jitter value - (-0.2134, 0.1706) pixels - before the gates were added.
// ── Jitter: the engine's own value, wherever it keeps it ──
// The projection matrix was proven not to carry it: with a non-zero requirement in
// place, not one projection in the window matched. That leaves a per-frame uniform
// consumed by the scene shaders - a float2 that is sub-pixel in magnitude and
// *changes every frame*. Constants do not move, and this does, so the test is not
// the value's size but its behaviour and its reach: the same tiny pair, appearing in
// more than one constant buffer of the same frame, and different the next frame.
struct JitterCandidate {
  float a = 0.f;
  float b = 0.f;
  uint32_t slots = 0u;      // distinct buffers it appeared in this frame
  uint32_t reach = 0u;      // how many buffers the winner reached
  uint64_t slot_mask = 0u;  // which ones, so repeats in one buffer do not count twice
};
inline JitterCandidate jitter_candidates[32];
inline uint32_t jitter_candidate_count = 0u;
inline float jitter_last_adopted_a = 0.f;
inline float jitter_last_adopted_b = 0.f;
inline uint32_t jitter_adopted_age = 0u;
// The last few adopted values, so that one report shows whether both components of
// the pair actually move over time or whether only one of them does.
inline float jitter_history[4][2] = {};
inline uint32_t jitter_history_next = 0u;
// Values adopted recently. A jitter never repeats a value, so a candidate whose value
// has just been used is skipped. That kills a two-buffer alternation, which otherwise
// wins simply by being first in the ring, and leaves whichever candidate keeps
// producing values the engine has never used before.
inline float jitter_recent[128][2] = {};
inline uint32_t jitter_recent_next = 0u;
inline uint32_t jitter_recent_count = 0u;

// A van der Corput value in [0,1): the basis of a Halton pair, which is what renderers
// jitter with. The synthetic option feeds one on purpose, so the image can be seen with
// a correct-shaped sequence and compared against whatever the search finds.
inline float HaltonValue(uint32_t index, uint32_t base) {
  float result = 0.0f;
  float fraction = 1.0f / static_cast<float>(base);
  uint32_t i = index;
  while (i > 0u) {
    result += fraction * static_cast<float>(i % base);
    i /= base;
    fraction /= static_cast<float>(base);
  }
  return result;
}

// ── Jitter: the engine's cycled table ──
// The projection matrix was proven not to carry it, and a Halton-like search never
// could: the engine's jitter is a finite table of offsets, cycled. That changes what is
// being looked for and what qualifies.
//
// Collection is hashed by the pair's bits, quantised so that the same value written by
// two different shaders groups together. Exact bits do not, and that alone kept the
// two-buffer test from ever being satisfied.
static constexpr uint32_t kCandidateSlots = 1024u;

struct CandidateSlot {
  uint64_t key = 0u;
  uint64_t mask = 0u;
  uint32_t slots = 0u;
  bool used = false;
};

inline CandidateSlot candidate_slots[kCandidateSlots];

inline CandidateSlot* FindCandidateSlot(uint64_t key) {
  uint32_t index = static_cast<uint32_t>((key * 0x9E3779B97F4A7C15ull) >> 40u) & (kCandidateSlots - 1u);
  for (uint32_t probe = 0u; probe < 24u; ++probe) {
    CandidateSlot& slot = candidate_slots[(index + probe) & (kCandidateSlots - 1u)];
    if (!slot.used) return &slot;
    if (slot.key == key) return &slot;
  }
  return nullptr;
}

// Once a handful of table entries are known in order, the sequence is *predicted*
// rather than searched for. Searching walks four megabytes of constant ring - that was
// the stutter - and yielded a value only on the frames where it saw the current entry.
// Predicting costs nothing, yields a value every frame, and is what the engine does.
inline uint32_t jitter_predict_cursor = 0u;
inline uint32_t jitter_predict_ready = 0u;
inline uint32_t jitter_scan_cooldown = 0u;
// Where the sliced walk of the window has reached, in absolute bytes, and whether the
// candidate table has been cleared for this pass. The table is cleared once per full
// pass, not once per frame: the reach test needs to see the whole ring's buffers, and
// clearing per frame is what forced the entire window to be walked every frame.
inline uint64_t jitter_slice_cursor = 0u;
inline bool jitter_slice_started = false;

inline void RefreshProjectionJitter(
    const reshade::api::buffer_range& range, uint32_t render_width, uint32_t render_height) {
  auto& js = projection_jitter;
  ++js.probes;
  js.valid = false;

  // The common path: one entry further along the table.
  if (jitter_predict_ready != 0u && jitter_recent_count >= 4u && jitter_scan_cooldown > 0u) {
    jitter_predict_cursor = (jitter_predict_cursor + 1u) % jitter_recent_count;
    const float px = jitter_recent[jitter_predict_cursor][0];
    const float py = jitter_recent[jitter_predict_cursor][1];
    if (std::isfinite(px) && std::isfinite(py) && fabsf(px) <= 0.5f && fabsf(py) <= 0.5f) {
      js.valid = true;
      js.pixels_x = px;
      js.pixels_y = py;
      js.ndc_x = px;
      js.ndc_y = py;
      js.matched_this_frame = true;
      --jitter_scan_cooldown;
      return;
    }
    jitter_predict_ready = 0u;
  }

  // Alignment. Before the table is known this runs occasionally rather than every
  // frame, so a failed search cannot cost frames; afterwards it runs once every 32 to
  // confirm the sequence or re-find it.
  if (jitter_scan_cooldown > 0u) {
    --jitter_scan_cooldown;
    return;
  }
  jitter_scan_cooldown = jitter_predict_ready != 0u ? 32u : 4u;

  if (range.buffer.handle == 0u || range.offset == UINT64_MAX) return;
  if (render_width == 0u || render_height == 0u) return;
  const uint8_t* data = MappedRingData(range.buffer.handle);
  if (data == nullptr) return;

  const uint64_t end = (range.offset + 512u) > mapped_ring.size ? mapped_ring.size : range.offset + 512u;
  const uint64_t window = 4u * 1024u * 1024u;
  const uint64_t begin = end > window ? end - window : 0u;
  if (end <= begin + 16u) return;

  if (!jitter_slice_started || jitter_slice_cursor < begin || jitter_slice_cursor >= end) {
    jitter_slice_cursor = begin;
    jitter_slice_started = true;
    for (uint32_t s = 0u; s < kCandidateSlots; ++s) candidate_slots[s] = CandidateSlot{};
  }
  // A slice per frame rather than the whole window: 512 KB is a fraction of a
  // millisecond, where the full walk was one to two.
  const uint64_t slice_bytes = 512u * 1024u;
  uint64_t slice_begin = jitter_slice_cursor;
  uint64_t slice_end = slice_begin + slice_bytes;
  if (slice_end > end) slice_end = end;
  jitter_slice_cursor = (slice_end >= end) ? begin : slice_end;

  const float pixel_limit = 0.5f;
  const float ndc_limit_x = 1.0f / static_cast<float>(render_width);
  const float ndc_limit_y = 1.0f / static_cast<float>(render_height);

  const float* floats = reinterpret_cast<const float*>(data + slice_begin);
  const uint32_t floats_count = static_cast<uint32_t>((slice_end - slice_begin) / sizeof(float));
  for (uint32_t i = 0u; i + 4u <= floats_count; i += 4u) {
    for (uint32_t pair = 0u; pair < 2u; ++pair) {
      const uint32_t at = i + pair * 2u;
      const float a = floats[at];
      const float b = floats[at + 1u];
      if (!std::isfinite(a) || !std::isfinite(b)) continue;
      const bool pixel_like = fabsf(a) <= pixel_limit && fabsf(b) <= pixel_limit;
      const bool ndc_like = fabsf(a) <= ndc_limit_x && fabsf(b) <= ndc_limit_y;
      if (!pixel_like && !ndc_like) continue;
      if (fabsf(a) + fabsf(b) < 0.002f) continue;
      uint32_t bits_a = 0u, bits_b = 0u;
      std::memcpy(&bits_a, &a, sizeof(bits_a));
      std::memcpy(&bits_b, &b, sizeof(bits_b));
      bits_a &= 0xFFFFF000u;
      bits_b &= 0xFFFFF000u;
      CandidateSlot* slot = FindCandidateSlot((static_cast<uint64_t>(bits_a) << 32u) | bits_b);
      if (slot == nullptr) continue;
      const uint64_t bit = 1ull << (((slice_begin + static_cast<uint64_t>(at) * sizeof(float)) / 512u) % 64u);
      if ((slot->mask & bit) == 0u) {
        slot->mask |= bit;
        ++slot->slots;
      }
    }
  }

  // A value qualifies by repeating one adopted earlier - the ring is the only window -
  // and by not matching anything from the last few frames, which rejects a constant and
  // a two-value alternation. Until the ring has filled once, any shared value is taken.
  uint32_t best_slot = UINT32_MAX;
  for (uint32_t s = 0u; s < kCandidateSlots; ++s) {
    if (!candidate_slots[s].used || candidate_slots[s].slots < 2u) continue;
    uint32_t bits_a = static_cast<uint32_t>(candidate_slots[s].key >> 32u);
    uint32_t bits_b = static_cast<uint32_t>(candidate_slots[s].key & 0xFFFFFFFFu);
    float a = 0.f, b = 0.f;
    std::memcpy(&a, &bits_a, sizeof(a));
    std::memcpy(&b, &bits_b, sizeof(b));
    bool immediate = false;
    bool returns = false;
    for (uint32_t r = 0u; r < jitter_recent_count; ++r) {
      const uint32_t index = (jitter_recent_next + 128u - 1u - r) % 128u;
      if (fabsf(jitter_recent[index][0] - a) > 1e-4f
          || fabsf(jitter_recent[index][1] - b) > 1e-4f) {
        continue;
      }
      if (r < 4u) {
        immediate = true;
        break;
      }
      returns = true;
      break;
    }
    if (immediate) continue;
    if (jitter_recent_count >= 128u && !returns) continue;
    best_slot = s;
    break;
  }
  if (best_slot == UINT32_MAX) return;

  uint32_t bits_a = static_cast<uint32_t>(candidate_slots[best_slot].key >> 32u);
  uint32_t bits_b = static_cast<uint32_t>(candidate_slots[best_slot].key & 0xFFFFFFFFu);
  float best_a = 0.f, best_b = 0.f;
  std::memcpy(&best_a, &bits_a, sizeof(best_a));
  std::memcpy(&best_b, &bits_b, sizeof(best_b));
  js.size = candidate_slots[best_slot].slots;

  const float pixels_x = fabsf(best_a) <= pixel_limit
      ? best_a
      : best_a * static_cast<float>(render_width) * 0.5f;
  const float pixels_y = fabsf(best_b) <= pixel_limit
      ? -best_b
      : -best_b * static_cast<float>(render_height) * 0.5f;
  if (!std::isfinite(pixels_x) || !std::isfinite(pixels_y)
      || fabsf(pixels_x) > 0.5f || fabsf(pixels_y) > 0.5f) return;

  ++js.adoptions;
  jitter_history[jitter_history_next][0] = pixels_x;
  jitter_history[jitter_history_next][1] = pixels_y;
  jitter_history_next = (jitter_history_next + 1u) % 4u;

  // Append to the sequence unless it repeats the previous entry, which would put a
  // duplicate in the table and make every prediction after it wrong.
  const uint32_t last = jitter_recent_count == 0u ? 0u : (jitter_recent_next + 128u - 1u) % 128u;
  if (jitter_recent_count == 0u
      || fabsf(jitter_recent[last][0] - best_a) > 1e-4f
      || fabsf(jitter_recent[last][1] - best_b) > 1e-4f) {
    jitter_recent[jitter_recent_next][0] = best_a;
    jitter_recent[jitter_recent_next][1] = best_b;
    jitter_predict_cursor = jitter_recent_next;
    jitter_recent_next = (jitter_recent_next + 1u) % 128u;
    if (jitter_recent_count < 128u) ++jitter_recent_count;
    jitter_predict_ready = 1u;
  }

  js.valid = true;
  js.base_xx = best_a;
  js.base_yy = best_b;
  js.ndc_x = best_a;
  js.ndc_y = best_b;
  js.pixels_x = pixels_x;
  js.pixels_y = pixels_y;
  js.matched_this_frame = true;
}

inline bool OnUpdateDescriptorTables(
    reshade::api::device* device,
    uint32_t count,
    const reshade::api::descriptor_table_update* updates) {
  if (dlss_enabled == 0.f || !any_nvidia.load()) return false;
  if (device == nullptr || count == 0u || updates == nullptr) return false;
  if (device->get_api() != reshade::api::device_api::d3d12) return false;

  for (uint32_t u = 0u; u < count; ++u) {
    const auto& update = updates[u];
    if (update.table.handle == 0u || update.count == 0u) continue;
    const bool is_cbv = update.type == reshade::api::descriptor_type::constant_buffer;
    const bool is_view = update.type == reshade::api::descriptor_type::texture_shader_resource_view
        || update.type == reshade::api::descriptor_type::buffer_shader_resource_view
        || update.type == reshade::api::descriptor_type::texture_unordered_access_view
        || update.type == reshade::api::descriptor_type::buffer_unordered_access_view
        || update.type == reshade::api::descriptor_type::sampler_with_resource_view;
    if (!is_cbv && !is_view) continue;

    for (uint32_t k = 0u; k < update.count; ++k) {
      RecordedDescriptor record;
      record.type = update.type;
      if (is_cbv) {
        record.range = static_cast<const reshade::api::buffer_range*>(update.descriptors)[k];
        record.has_range = record.range.buffer.handle != 0u;
        if (!record.has_range) continue;
        ProbeConstantBufferForProjection(record.range);
      } else {
        const reshade::api::resource_view view =
            update.type == reshade::api::descriptor_type::sampler_with_resource_view
                ? static_cast<const reshade::api::sampler_with_resource_view*>(update.descriptors)[k].view
                : static_cast<const reshade::api::resource_view*>(update.descriptors)[k];
        if (view.handle == 0u) continue;
        // Resolve now, while this slot still holds this view.
        record.resource = device->get_resource_from_view(view);
        if (record.resource.handle == 0u) continue;
        record.format = device->get_resource_view_desc(view).format;
        const reshade::api::resource_desc resource_desc = device->get_resource_desc(record.resource);
        if (resource_desc.type == reshade::api::resource_type::texture_2d) {
          record.width = resource_desc.texture.width;
          record.height = resource_desc.texture.height;
        }
        record.has_resource = true;
      }

      DescriptorFingerprint key;
      if (!ReadSlotFingerprint(device, update.table, update.binding + k, key)) continue;
      RecordFingerprint(key, record);
    }
  }
  return false;
}

inline bool IsSrvDescriptorType(reshade::api::descriptor_type type) {
  return type == reshade::api::descriptor_type::texture_shader_resource_view
      || type == reshade::api::descriptor_type::buffer_shader_resource_view;
}
// ── Layout lookup ──

// ── Layout lookup ──
inline bool CurrentPixelLayout(reshade::api::command_list* cmd_list, reshade::api::pipeline_layout& layout) {
  layout = {};
  if (cmd_list == nullptr) return false;
  auto* shader_state = renodx::utils::shader::GetCurrentState(cmd_list);
  if (shader_state == nullptr) return false;
  auto* pixel_state = renodx::utils::shader::GetCurrentPixelState(shader_state);
  if (pixel_state == nullptr) return false;
  renodx::utils::shader::PopulateStageState(pixel_state);
  if (pixel_state->pipeline_details == nullptr) return false;
  layout = pixel_state->pipeline_details->layout;
  return layout.handle != 0u;
}

// Enumerates the (layout_param, binding) pairs whose range covers
// (register, space) AND matches the wanted descriptor type. The type filter is
// what keeps sampler ranges (space 0, register 0) from displacing the real
// table, and it removes every cross-type probe from the read path.
inline uint32_t FindTablesForRegister(
    reshade::api::pipeline_layout layout,
    uint32_t dx_register_index,
    uint32_t dx_register_space,
    reshade::api::descriptor_type wanted,
    uint32_t* out_params,
    uint32_t* out_bindings,
    uint32_t max_out) {
  uint32_t n = 0u;
  const auto* layout_data = renodx::utils::pipeline_layout::GetPipelineLayoutData(layout);
  if (layout_data == nullptr || out_params == nullptr || out_bindings == nullptr || max_out == 0u) return 0u;
  for (uint32_t param_index = 0u; param_index < layout_data->ranges.size() && n < max_out; ++param_index) {
    // Push-descriptor params have no table to read.
    if (param_index < layout_data->params.size()
        && layout_data->params[param_index].type == reshade::api::pipeline_layout_param_type::push_descriptors) {
      continue;
    }
    for (const auto& range : layout_data->ranges[param_index]) {
      if (range.type != wanted) continue;
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

// ── Per-command-list state ──
struct __declspec(uuid("7f3e2a1c-9b4d-4e8f-a6c2-d5f1b0e98341")) CommandListData {
  std::array<reshade::api::descriptor_table, kTrackedTableCount> descriptor_tables = {};
  std::array<reshade::api::resource_view, kTrackedRtvCount> rtvs = {};
  // Depth attribution: draws are counted against whichever depth-stencil was
  // bound when they were issued, and flushed into the shared map at every bind.
  reshade::api::resource dsv = {};
  uint32_t dsv_draws = 0u;
  // Memoized pixel-shader hash: refreshed per pipeline bind instead of per draw.
  uint32_t cached_ps_hash = 0u;
  bool ps_hash_valid = false;
  bool ps_dirty = true;
  // Same for the scene probe: its hashes are read once per bind, not per draw, so
  // the probe costs two comparisons on the draws it does not care about.
  uint32_t cached_scene_vs_hash = 0u;
  uint32_t cached_scene_ps_hash = 0u;
  bool scene_hash_valid = false;
};

inline CommandListData* Get(reshade::api::command_list* cmd_list) {
  if (cmd_list == nullptr) return nullptr;
  auto* data = cmd_list->get_private_data<CommandListData>();
  return data != nullptr ? data : cmd_list->create_private_data<CommandListData>();
}

// ── View cache ──
// The unpack pass needs a ReShade view of the motion resource. It is created
// from the resource we resolved, never from a recorded handle, and reused:
// creating one per draw would leak a view every frame.
static constexpr uint32_t kViewCacheSize = 8u;
inline std::mutex view_cache_mutex;
inline reshade::api::resource_view view_cache[kViewCacheSize] = {};
inline reshade::api::resource view_cache_resource[kViewCacheSize] = {};
inline reshade::api::format view_cache_format[kViewCacheSize] = {};

inline void ReleaseViewCache(reshade::api::device* device) {
  if (device == nullptr) return;
  const std::lock_guard<std::mutex> lock(view_cache_mutex);
  for (uint32_t i = 0u; i < kViewCacheSize; ++i) {
    if (view_cache[i].handle == 0u) continue;
    device->destroy_resource_view(view_cache[i]);
    view_cache[i] = {};
    view_cache_resource[i] = {};
    view_cache_format[i] = reshade::api::format::unknown;
  }
}

inline reshade::api::resource_view GetOrCreateTextureView(
    reshade::api::device* device, reshade::api::resource resource, reshade::api::format format) {
  if (device == nullptr || resource.handle == 0u || format == reshade::api::format::unknown) return {};
  const std::lock_guard<std::mutex> lock(view_cache_mutex);
  uint32_t empty = kViewCacheSize;
  for (uint32_t i = 0u; i < kViewCacheSize; ++i) {
    if (view_cache[i].handle == 0u) {
      if (empty == kViewCacheSize) empty = i;
      continue;
    }
    if (view_cache_resource[i].handle == resource.handle && view_cache_format[i] == format) {
      return view_cache[i];
    }
  }
  if (empty == kViewCacheSize) empty = 0u;  // cache full: recycle slot 0
  if (view_cache[empty].handle != 0u) device->destroy_resource_view(view_cache[empty]);
  reshade::api::resource_view_desc view_desc(
      reshade::api::resource_view_type::texture_2d, format, 0, 1, 0, 1);
  reshade::api::resource_view view = {};
  if (!device->create_resource_view(resource, reshade::api::resource_usage::shader_resource, view_desc, &view)
      || view.handle == 0u) {
    return {};
  }
  view_cache[empty] = view;
  view_cache_resource[empty] = resource;
  view_cache_format[empty] = format;
  return view;
}

// ── Descriptor probe ──
// One-shot, report-only. On a miss the bytes read at the bound slot are kept
// next to a count of recorded fingerprints that share the low half, so a failure
// says *how* it failed — different identity, or only the upper half differing —
// instead of just that it failed.
// A few frames of history per slot, because the interesting failure here is an
// alternation: the motion slot flipping between two resources too fast to read
// off a live panel. The report carries the last few observations so both appear.
static constexpr uint32_t kProbeHistory = 4u;

struct ProbeSample {
  uint64_t resource = 0u;
  uint32_t width = 0u;
  uint32_t height = 0u;
  reshade::api::format format = reshade::api::format::unknown;
};

struct ProbeSlot {
  bool searched = false;
  bool found = false;
  uint64_t low = 0u;
  uint64_t high = 0u;
  uint32_t observations = 0u;
  uint32_t mismatches = 0u;
  ProbeSample history[kProbeHistory] = {};
};
inline ProbeSlot probe_slots[4];  // t0, t2, t5, t6
inline std::mutex probe_mutex;

inline const char* ProbeSlotName(uint32_t index) {
  switch (index) {
    case 0u: return "t0";
    case 1u: return "t2";
    case 2u: return "t5";
    default: return "t6";
  }
}

inline void RecordProbe(
    reshade::api::device*, uint32_t probe_index, const DescriptorFingerprint& key,
    const RecordedDescriptor* record, bool resolved) {
  if (probe_index >= 4u) return;
  const std::lock_guard<std::mutex> lock(probe_mutex);
  ProbeSlot& slot = probe_slots[probe_index];
  if (!slot.searched) {
    slot.searched = true;
    slot.found = resolved;
    slot.low = key.words[0];
    slot.high = key.words[1];
  }
  ++slot.observations;
  if (!resolved || record == nullptr || !record->has_resource) return;
  // Shift the ring: oldest first, newest last.
  for (uint32_t i = 0u; i + 1u < kProbeHistory; ++i) slot.history[i] = slot.history[i + 1u];
  ProbeSample& newest = slot.history[kProbeHistory - 1u];
  const ProbeSample previous = newest;
  newest.resource = record->resource.handle;
  newest.width = record->width;
  newest.height = record->height;
  newest.format = record->format;
  if (previous.resource != 0u && (previous.width != newest.width || previous.height != newest.height
                                  || previous.format != newest.format)) {
    ++slot.mismatches;
  }
}

// Probe lines, shared by the Status readout and the Copy Report: the report has
// to carry everything needed to diagnose a run, because the game's log is not a
// practical channel.
inline void AppendProbeLines(std::stringstream& s) {
  const std::lock_guard<std::mutex> lock(probe_mutex);
  for (uint32_t i = 0u; i < 4u; ++i) {
    const ProbeSlot& slot = probe_slots[i];
    s << "  " << ProbeSlotName(i) << "   : ";
    if (!slot.searched) {
      s << "slot not reached\n";
      continue;
    }
    s << (slot.found ? "resolved" : "MISS")
      << " bytes=0x" << std::hex << slot.high << ":" << slot.low << std::dec;
    if (!slot.found) s << " no-entry";
    if (slot.mismatches != 0u) {
      s << " MISMATCHES=" << slot.mismatches << "/" << slot.observations;
    }
    s << "\n";
    // The last few frames, so an alternation is visible in one paste.
    bool any = false;
    for (uint32_t k = 0u; k < kProbeHistory; ++k) {
      const ProbeSample& sample = slot.history[k];
      if (sample.resource == 0u) continue;
      if (!any) {
        s << "         recent:";
        any = true;
      }
      s << " 0x" << std::hex << sample.resource << std::dec
        << " " << sample.width << "x" << sample.height
        << " fmt=" << static_cast<int>(sample.format);
    }
    if (any) s << "\n";
  }
}

inline std::string BuildProbeReport() {
  std::stringstream s;
  AppendProbeLines(s);
  return s.str();
}

// ── Input placeholders ──
// Each DLSS input can be replaced by a well-formed stand-in, so a rendering fault
// can be attributed to one input by substitution instead of by inference. The
// stand-ins are valid textures of the right size and format, so NGX never
// receives anything malformed - only something with neutral content:
//   motion  zero motion (and MVJittered is dropped to match)
//   depth   far plane, i.e. nothing occludes anything
//   colour  flat mid grey
struct PlaceholderData {
  reshade::api::resource motion = {};
  reshade::api::resource depth = {};
  reshade::api::resource color = {};
  uint32_t width = 0u;
  uint32_t height = 0u;
  bool depth_inverted = false;
};

inline PlaceholderData placeholders;
inline std::mutex placeholder_mutex;
inline reshade::api::device* placeholder_device = nullptr;

inline void DestroyPlaceholders(reshade::api::device* device) {
  if (device == nullptr) return;
  if (placeholders.motion.handle) device->destroy_resource(placeholders.motion);
  if (placeholders.depth.handle) device->destroy_resource(placeholders.depth);
  if (placeholders.color.handle) device->destroy_resource(placeholders.color);
  placeholders = {};
}

inline void ReleasePlaceholders(reshade::api::device* device) {
  const std::lock_guard<std::mutex> lock(placeholder_mutex);
  DestroyPlaceholders(device);
  if (placeholder_device == device) placeholder_device = nullptr;
}

inline bool EnsurePlaceholders(reshade::api::device* device, uint32_t width, uint32_t height) {
  if (device == nullptr || width == 0u || height == 0u) return false;
  const bool want_inverted = dlss_depth_inverted != 0.f;
  const std::lock_guard<std::mutex> lock(placeholder_mutex);
  if (placeholder_device != device) {
    // Statics are single-device; a device change releases everything.
    DestroyPlaceholders(placeholder_device);
    placeholder_device = device;
  }
  if (placeholders.motion.handle != 0u && placeholders.color.handle != 0u
      && placeholders.depth.handle != 0u
      && placeholders.width == width && placeholders.height == height
      && placeholders.depth_inverted == want_inverted) {
    return true;
  }
  DestroyPlaceholders(device);

  const size_t pixels = static_cast<size_t>(width) * height;

  reshade::api::resource_desc motion_desc = {};
  motion_desc.type = reshade::api::resource_type::texture_2d;
  motion_desc.texture = {width, height, 1, 1, reshade::api::format::r16g16_float, 1};
  motion_desc.heap = reshade::api::memory_heap::gpu_only;
  motion_desc.usage = reshade::api::resource_usage::shader_resource | reshade::api::resource_usage::copy_source;
  std::vector<uint16_t> zero_motion(pixels * 2u, 0u);
  reshade::api::subresource_data motion_init = {};
  motion_init.data = zero_motion.data();
  motion_init.row_pitch = width * 2u * sizeof(uint16_t);
  motion_init.slice_pitch = static_cast<uint32_t>(zero_motion.size() * sizeof(uint16_t));
  if (!device->create_resource(motion_desc, &motion_init, reshade::api::resource_usage::shader_resource,
                               &placeholders.motion)
      || placeholders.motion.handle == 0u) {
    DestroyPlaceholders(device);
    return false;
  }

  // Reverse-Z stores the far plane as zero, so the stand-in follows the setting:
  // a "far" that is actually near would mark every pixel disoccluded.
  reshade::api::resource_desc depth_desc = {};
  depth_desc.type = reshade::api::resource_type::texture_2d;
  depth_desc.texture = {width, height, 1, 1, reshade::api::format::r32_float, 1};
  depth_desc.heap = reshade::api::memory_heap::gpu_only;
  depth_desc.usage = reshade::api::resource_usage::shader_resource;
  std::vector<float> far_plane(pixels, want_inverted ? 0.0f : 1.0f);
  reshade::api::subresource_data depth_init = {};
  depth_init.data = far_plane.data();
  depth_init.row_pitch = width * sizeof(float);
  depth_init.slice_pitch = static_cast<uint32_t>(pixels * sizeof(float));
  if (!device->create_resource(depth_desc, &depth_init, reshade::api::resource_usage::shader_resource,
                               &placeholders.depth)
      || placeholders.depth.handle == 0u) {
    DestroyPlaceholders(device);
    return false;
  }

  // Flat mid grey in linear space: bright enough that artifacts are unmistakable,
  // neutral enough that nothing in it can be mistaken for scene content.
  reshade::api::resource_desc color_desc = {};
  color_desc.type = reshade::api::resource_type::texture_2d;
  color_desc.texture = {width, height, 1, 1, reshade::api::format::r16g16b16a16_float, 1};
  color_desc.heap = reshade::api::memory_heap::gpu_only;
  color_desc.usage = reshade::api::resource_usage::shader_resource | reshade::api::resource_usage::copy_source;
  std::vector<uint16_t> grey(pixels * 4u);
  for (size_t i = 0u; i < pixels; ++i) {
    grey[i * 4u + 0u] = 0x3800u;  // 0.5 in FP16
    grey[i * 4u + 1u] = 0x3800u;
    grey[i * 4u + 2u] = 0x3800u;
    grey[i * 4u + 3u] = 0x3C00u;  // 1.0
  }
  reshade::api::subresource_data color_init = {};
  color_init.data = grey.data();
  color_init.row_pitch = width * 4u * sizeof(uint16_t);
  color_init.slice_pitch = static_cast<uint32_t>(grey.size() * sizeof(uint16_t));
  if (!device->create_resource(color_desc, &color_init, reshade::api::resource_usage::shader_resource,
                               &placeholders.color)
      || placeholders.color.handle == 0u) {
    DestroyPlaceholders(device);
    return false;
  }

  placeholders.width = width;
  placeholders.height = height;
  placeholders.depth_inverted = want_inverted;
  return true;
}

// ── Scene depth ──
// DLSS compares depth between frames to find disocclusions, so it needs the
// *scene* depth — not t5/t6, which the reference shader uses as an anti-ghosting
// mask whose producer merely reads depth to build it. Feeding DLSS a mask makes
// it fire its disocclusion logic in the wrong places and keep the resulting
// history, which is what accumulating artifacts look like.
//
// DevKit's draw details carry no depth-stencil field, so the depth is identified
// by how much work is issued against it: the geometry pass draws thousands of
// times with the scene depth bound, while every effect draws a handful. Binds are
// the wrong measure — the engine binds render targets per *pass*, not per draw, so
// every depth looks equally bound — so draws are attributed to whichever
// depth-stencil was bound when they were issued. A per-command-list counter does
// that for free: it increments in the draw handler and is flushed into the shared
// map whenever the render targets change.
inline std::mutex depth_mutex;
struct DepthCandidate {
  uint32_t draws = 0u;
  uint32_t width = 0u;
  uint32_t height = 0u;
};
inline std::unordered_map<uint64_t, DepthCandidate> depth_candidates;
inline uint64_t scene_depth_resource = 0u;
inline uint32_t scene_depth_draws = 0u;
inline uint32_t scene_depth_width = 0u;
inline uint32_t scene_depth_height = 0u;
inline reshade::api::format scene_depth_format = reshade::api::format::unknown;
// Render size of the last TAA target. Used to exclude depth candidates that
// cannot be the scene depth: the previous build ranked purely by draw count and
// picked a 1536x1536 D16 shadow cascade, which the geometry passes draw into more
// often than the main pass.
inline std::atomic<uint32_t> last_render_width{0u};
inline std::atomic<uint32_t> last_render_height{0u};


inline void FlushDepthDraws(reshade::api::device* device, CommandListData* data) {
  if (data == nullptr || data->dsv.handle == 0u) return;
  if (data->dsv_draws == 0u) return;
  const uint64_t resource = data->dsv.handle;
  const uint32_t draws = data->dsv_draws;
  data->dsv_draws = 0u;

  const std::lock_guard<std::mutex> lock(depth_mutex);
  DepthCandidate& candidate = depth_candidates[resource];
  if (candidate.width == 0u && device != nullptr) {
    const reshade::api::resource_desc desc = device->get_resource_desc(reshade::api::resource{resource});
    if (desc.type == reshade::api::resource_type::texture_2d) {
      candidate.width = desc.texture.width;
      candidate.height = desc.texture.height;
    }
  }
  candidate.draws += draws;
}

// Called at the frame boundary: among the depths drawn into at the render
// resolution, the one with the most draws becomes the depth for the next frame.
inline void SelectSceneDepth(reshade::api::device* device) {
  if (device == nullptr) return;
  const uint32_t want_width = last_render_width.load();
  const uint32_t want_height = last_render_height.load();
  uint64_t best = 0u;
  uint32_t best_count = 0u;
  {
    const std::lock_guard<std::mutex> lock(depth_mutex);
    for (const auto& entry : depth_candidates) {
      if (want_width != 0u && (entry.second.width != want_width || entry.second.height != want_height)) continue;
      if (entry.second.draws > best_count) {
        best = entry.first;
        best_count = entry.second.draws;
      }
    }
    depth_candidates.clear();
  }
  // A handful of draws is a full-screen effect's depth, not the scene's.
  if (best == 0u || best_count < 100u) return;
  const reshade::api::resource_desc desc = device->get_resource_desc(reshade::api::resource{best});
  if (desc.type != reshade::api::resource_type::texture_2d) return;
  scene_depth_resource = best;
  scene_depth_draws = best_count;
  scene_depth_width = desc.texture.width;
  scene_depth_height = desc.texture.height;
  scene_depth_format = desc.texture.format;
  // Its state has to be known to read it as a shader resource. Registering it
  // here, at the frame boundary, means the engine's transitions during the next
  // frame are recorded before NGX needs them.
  TrackDepthResource(best);
}

// ── Jitter: read from the pass that applies it ──
// The projection test above is triggered by constant-buffer writes — a frame has
// hundreds — with a budget of four per frame. Measured over sessions, that reached
// the scene's own view constants on almost no frame, which is why every version of
// a search for the value either missed it, cost frames, or both.
//
// The pass that cannot avoid the jitter is the one that rasterises the scene: every
// vertex of the frame is offset by it, so the value is in that pass's view constants
// by construction rather than by chance. Those passes are known by identity — the
// hashes below came from a live capture, and each draws four render targets at the
// render resolution with the camera's constant buffer at b0 — so the read happens at
// the draws that must carry the jitter, not wherever a scan happens to land.
inline constexpr uint32_t kSceneShaders[] = {
    0xA38182D8u, 0xD632E3BDu,  // terrain
    0x949755FAu, 0x4D19C52Du,  // props
    0x459A3A3Eu, 0xF61BFFFFu,  // packed scene geometry
    0x2CF40AE1u, 0xF78435B7u,  // scene materials
    0xABDFA78Eu, 0xE15F674Au,
    0x1BC1D314u, 0x48D5181Cu,
};
static constexpr uint32_t kSceneShaderCount = sizeof(kSceneShaders) / sizeof(kSceneShaders[0]);
// A handful of draws per frame: one is enough once the pass is known, and the budget
// is what keeps a frame whose first candidates are shadow passes from costing
// anything.
static constexpr uint32_t kSceneProbeAttemptsPerFrame = 6u;
static constexpr uint64_t kSceneProbeMaxBytes = 8192u;
inline uint32_t scene_probe_attempts_left = 0u;
inline uint32_t scene_attempts_made = 0u;
inline bool scene_projected_this_frame = false;

// One reading, plus the previous one, so movement can be told from a constant: the
// jitter changes every frame, a static offset never does.
struct SceneJitter {
  bool valid = false;
  bool from_projection = false;
  uint32_t shader = 0u;
  uint32_t stage = 0u;   // 0 = vertex, 1 = pixel
  uint32_t cb = 0u;
  uint32_t offset = 0u;  // byte offset of the value inside the buffer view
  uint32_t order = 0u;   // storage order of the matching matrix
  float raw_a = 0.f;
  float raw_b = 0.f;
  float pixels_x = 0.f;
  float pixels_y = 0.f;
  // The matrix's own previous reading, so movement is measured against the matrix
  // and never against whatever value the source currently holds.
  float previous_a = 0.f;
  float previous_b = 0.f;
  uint64_t reads = 0u;
  uint64_t moved = 0u;
  // The frame it was last read on. A reading that stops being refreshed must not
  // stay in hand: a stale jitter is a wrong jitter.
  uint64_t updated_frame = 0u;
};
inline SceneJitter scene_jitter;
inline uint64_t frame_counter = 0u;

// Where the matrix was found inside the buffer, relative to the buffer's start. The
// ring relocates the contents every frame, so only the relative position is stable
// — and it is stable enough to read one block instead of scanning the buffer.
inline uint32_t scene_latch_cb = UINT32_MAX;
inline uint32_t scene_latch_offset = 0u;
inline uint32_t scene_latch_order = 0u;

// Sub-pixel pairs in the same buffers, kept per (buffer, offset). They are the
// fallback when no projection carries the offset, and their counters are the
// evidence that decides which pair is the jitter: it moves every frame, a constant
// never does.
static constexpr uint32_t kScenePairCount = 12u;

struct ScenePair {
  bool used = false;
  uint32_t cb = 0u;
  uint32_t offset = 0u;
  float a = 0.f;
  float b = 0.f;
  uint32_t reads = 0u;
  uint32_t moved = 0u;
};
inline ScenePair scene_pairs[kScenePairCount];

inline void NoteScenePair(uint32_t cb, uint32_t offset, float a, float b) {
  ScenePair* slot = nullptr;
  for (ScenePair& pair : scene_pairs) {
    if (pair.used && pair.cb == cb && pair.offset == offset) {
      slot = &pair;
      break;
    }
    if (!pair.used && slot == nullptr) slot = &pair;
  }
  if (slot == nullptr) return;
  if (!slot->used) {
    *slot = ScenePair{};
    slot->used = true;
    slot->cb = cb;
    slot->offset = offset;
  }
  if (slot->reads != 0u
      && (fabsf(slot->a - a) > 1e-6f || fabsf(slot->b - b) > 1e-6f)) {
    ++slot->moved;
  }
  slot->a = a;
  slot->b = b;
  ++slot->reads;
}

// A pair's units are not labelled in the buffer. A value that cannot be NDC is read
// as pixels; one that can only be NDC is converted the same way the projection is.
inline void PairToPixels(
    float a, float b, uint32_t width, uint32_t height, float& out_x, float& out_y) {
  out_x = 0.f;
  out_y = 0.f;
  if (width == 0u || height == 0u) return;
  // One pixel of NDC is 2/width, so anything inside that on its axis is NDC.
  if (fabsf(a) <= 2.0f / static_cast<float>(width)
      && fabsf(b) <= 2.0f / static_cast<float>(height)) {
    out_x = a * static_cast<float>(width) * 0.5f;
    out_y = -b * static_cast<float>(height) * 0.5f;
    return;
  }
  out_x = a;
  out_y = -b;
}

// Reads one constant buffer of a scene draw: the projection that carries the offset
// first, and the sub-pixel pairs only while no projection has been found.
inline void ProbeSceneConstantBuffer(
    const reshade::api::buffer_range& range, uint32_t cb, uint32_t shader, uint32_t stage) {
  if (range.buffer.handle == 0u || range.offset == UINT64_MAX) return;
  const uint8_t* mapped = MappedRingData(range.buffer.handle);
  if (mapped == nullptr) return;
  const uint64_t begin = range.offset;
  if (begin >= mapped_ring.size) return;
  uint64_t bytes = range.size == UINT64_MAX ? 0u : range.size;
  if (bytes == 0u || bytes > kSceneProbeMaxBytes) bytes = kSceneProbeMaxBytes;
  if (begin + bytes > mapped_ring.size) bytes = mapped_ring.size - begin;
  if (bytes < 16u * sizeof(float)) return;
  const auto* floats = reinterpret_cast<const float*>(mapped + begin);
  const uint32_t count = static_cast<uint32_t>(bytes / sizeof(float));

  uint32_t block = UINT32_MAX;
  uint32_t order = 0u;
  float ndc_x = 0.f;
  float ndc_y = 0.f;
  if (scene_latch_cb == cb && scene_latch_offset + 16u * sizeof(float) <= bytes) {
    // Latched: one block, sixteen floats, read instead of scanned.
    const float* m = reinterpret_cast<const float*>(mapped + begin + scene_latch_offset);
    float xx = 0.f, yy = 0.f, zz = 0.f, zw = 0.f;
    if (MatchProjection(m, scene_latch_order != 0u, ndc_x, ndc_y, xx, yy, zz, zw)) {
      block = static_cast<uint32_t>(scene_latch_offset / (4u * sizeof(float)));
      order = scene_latch_order;
    } else {
      scene_latch_cb = UINT32_MAX;  // the layout moved: find it again
    }
  }
  if (block == UINT32_MAX && scene_latch_cb == UINT32_MAX) {
    // A scan only happens while nothing is latched, and it prefers the projection
    // that carries an offset: an unjittered one proves nothing about the jitter, and
    // a frame's constants hold several matrices.
    float best = -1.0f;
    for (uint32_t candidate = 0u; candidate + 16u <= count; ++candidate) {
      const float* m = floats + static_cast<size_t>(candidate) * 4u;
      for (uint32_t candidate_order = 0u; candidate_order < 2u; ++candidate_order) {
        float nx = 0.f, ny = 0.f, xx = 0.f, yy = 0.f, zz = 0.f, zw = 0.f;
        if (!MatchProjection(m, candidate_order != 0u, nx, ny, xx, yy, zz, zw)) continue;
        const float magnitude = fabsf(nx) + fabsf(ny);
        if (magnitude <= best) continue;
        best = magnitude;
        block = candidate;
        order = candidate_order;
        ndc_x = nx;
        ndc_y = ny;
      }
    }
    if (block != UINT32_MAX) {
      scene_latch_cb = cb;
      scene_latch_offset = block * 4u * sizeof(float);
      scene_latch_order = order;
    }
  }
  if (block == UINT32_MAX) {
    // Pairs are recorded while no matrix is latched — that is the fallback — and
    // beside the latched matrix itself, because a projection that never carries an
    // offset is exactly the case that needs one.
    if (scene_attempts_made > 1u) return;
    if (scene_latch_cb != UINT32_MAX && scene_latch_cb != cb) return;
    for (uint32_t i = 0u; i + 1u < count; ++i) {
      const float a = floats[i];
      const float b = floats[i + 1u];
      if (!std::isfinite(a) || !std::isfinite(b)) continue;
      if (fabsf(a) > 0.5f || fabsf(b) > 0.5f) continue;
      if (fabsf(a) + fabsf(b) < 2e-4f) continue;
      NoteScenePair(cb, static_cast<uint32_t>(i * sizeof(float)), a, b);
    }
    return;
  }

  // The matrix's own NDC offset, and the two entries it is stored in, so the report
  // carries what the buffer says and not only what was derived from it.
  const float* m = floats + static_cast<size_t>(block) * 4u;
  const float raw_a = m[order != 0u ? 8u : 2u];
  const float raw_b = m[order != 0u ? 9u : 6u];
  const uint32_t width = last_render_width.load();
  const uint32_t height = last_render_height.load();
  const bool moved = scene_jitter.reads != 0u
      && (fabsf(scene_jitter.previous_a - raw_a) > 1e-7f
          || fabsf(scene_jitter.previous_b - raw_b) > 1e-7f);
  scene_jitter.previous_a = raw_a;
  scene_jitter.previous_b = raw_b;
  scene_jitter.valid = true;
  scene_jitter.from_projection = true;
  scene_jitter.shader = shader;
  scene_jitter.stage = stage;
  scene_jitter.cb = cb;
  scene_jitter.offset = block * 4u * sizeof(float);
  scene_jitter.order = order;
  scene_jitter.raw_a = raw_a;
  scene_jitter.raw_b = raw_b;
  scene_jitter.pixels_x = ndc_x * static_cast<float>(width) * 0.5f;
  scene_jitter.pixels_y = -ndc_y * static_cast<float>(height) * 0.5f;
  scene_jitter.updated_frame = frame_counter;
  ++scene_jitter.reads;
  if (moved) ++scene_jitter.moved;
  scene_projected_this_frame = true;
}

// The entry point from the draw hooks: identifies the scene passes and reads their
// constant buffers, at a handful of draws per frame.
inline void ProbeSceneDraw(reshade::api::command_list* cmd_list) {
  if (scene_probe_attempts_left == 0u || scene_projected_this_frame) return;
  if (cmd_list == nullptr) return;
  auto* data = Get(cmd_list);
  if (data == nullptr) return;
  if (!data->scene_hash_valid) {
    auto* shader_state = renodx::utils::shader::GetCurrentState(cmd_list);
    if (shader_state == nullptr) return;
    data->cached_scene_vs_hash = renodx::utils::shader::GetCurrentVertexShaderHash(shader_state);
    data->cached_scene_ps_hash = renodx::utils::shader::GetCurrentPixelShaderHash(shader_state);
    data->scene_hash_valid = true;
  }
  const uint32_t vertex_hash = data->cached_scene_vs_hash;
  const uint32_t pixel_hash = data->cached_scene_ps_hash;
  uint32_t shader = 0u;
  uint32_t stage = 0u;
  for (uint32_t i = 0u; i + 1u < kSceneShaderCount; i += 2u) {
    const uint32_t first = kSceneShaders[i];
    const uint32_t second = kSceneShaders[i + 1u];
    if (vertex_hash != 0u && (vertex_hash == first || vertex_hash == second)) {
      shader = vertex_hash;
      stage = 0u;
      break;
    }
    if (pixel_hash != 0u && (pixel_hash == first || pixel_hash == second)) {
      shader = pixel_hash;
      stage = 1u;
      break;
    }
  }
  if (shader == 0u) return;
  --scene_probe_attempts_left;
  ++scene_attempts_made;
  reshade::api::pipeline_layout layout = {};
  if (!CurrentPixelLayout(cmd_list, layout)) return;
  auto* device = cmd_list->get_device();
  if (device == nullptr) return;
  // The camera's constants sit at b0 in every scene pass measured; the registers
  // after it are per-material, and they are checked too so a pass that moves the
  // camera's block is still read. One camera buffer per frame is enough.
  for (uint32_t reg = 0u; reg < 4u && !scene_projected_this_frame; ++reg) {
    uint32_t params[2] = {};
    uint32_t bindings[2] = {};
    const uint32_t n = FindTablesForRegister(
        layout, reg, 0u, reshade::api::descriptor_type::constant_buffer, params, bindings, 2u);
    for (uint32_t k = 0u; k < n; ++k) {
      if (params[k] >= data->descriptor_tables.size()) continue;
      const auto table = data->descriptor_tables[params[k]];
      if (table.handle == 0u) continue;
      DescriptorFingerprint key;
      if (!ReadSlotFingerprint(device, table, bindings[k], key)) continue;
      RecordedDescriptor record;
      if (!LookupFingerprint(key, record)) continue;
      if (!record.has_range || record.type != reshade::api::descriptor_type::constant_buffer) continue;
      ProbeSceneConstantBuffer(record.range, reg, shader, stage);
      if (scene_projected_this_frame) break;
    }
  }
  // A matrix that never moves is not the jitter: after thirty readings with no
  // change, the offset is being applied elsewhere and the sub-pixel pair that does
  // move is the source. If the matrix starts moving, it takes the source back.
  const bool want_pair = !scene_jitter.from_projection
      || (scene_jitter.moved == 0u && scene_jitter.reads >= 30u);
  if (!want_pair) return;
  const ScenePair* best = nullptr;
  for (const ScenePair& pair : scene_pairs) {
    if (!pair.used || pair.moved == 0u) continue;
    if (best == nullptr
        || static_cast<uint64_t>(pair.moved) * best->reads
            > static_cast<uint64_t>(best->moved) * pair.reads) {
      best = &pair;
    }
  }
  if (best == nullptr) return;
  scene_jitter.valid = true;
  scene_jitter.from_projection = false;
  scene_jitter.shader = shader;
  scene_jitter.stage = stage;
  scene_jitter.cb = best->cb;
  scene_jitter.offset = best->offset;
  scene_jitter.order = 0u;
  scene_jitter.raw_a = best->a;
  scene_jitter.raw_b = best->b;
  PairToPixels(best->a, best->b, last_render_width.load(), last_render_height.load(),
               scene_jitter.pixels_x, scene_jitter.pixels_y);
  scene_jitter.updated_frame = frame_counter;
}

// ── Resolvers ──
struct SlotInput {
  reshade::api::resource resource = {};
  reshade::api::format format = reshade::api::format::unknown;
};

inline bool ResolveSlot(
    reshade::api::command_list* cmd_list,
    const CommandListData* data,
    uint32_t slot,
    uint32_t probe_index,
    SlotInput& out) {
  out = {};
  if (cmd_list == nullptr || data == nullptr) return false;
  const uint32_t li = (slot == kSlotPackedMv) ? 1u : (slot == kSlotColor) ? 2u : (slot == kSlotDepthA) ? 3u : 4u;

  reshade::api::pipeline_layout layout = {};
  if (!CurrentPixelLayout(cmd_list, layout)) {
    slot_hit[li].store(0);
    return false;
  }
  uint32_t params[4] = {};
  uint32_t bindings[4] = {};
  const uint32_t n = FindTablesForRegister(
      layout, slot, 0u, reshade::api::descriptor_type::texture_shader_resource_view, params, bindings, 4u);
  auto* device = cmd_list->get_device();
  for (uint32_t k = 0u; k < n; ++k) {
    if (params[k] >= data->descriptor_tables.size()) continue;
    const auto table = data->descriptor_tables[params[k]];
    if (table.handle == 0u) continue;
    DescriptorFingerprint key;
    if (!ReadSlotFingerprint(device, table, bindings[k], key)) continue;
    RecordedDescriptor record;
    const bool resolved = LookupFingerprint(key, record);
    RecordProbe(device, probe_index, key, resolved ? &record : nullptr, resolved);
    if (!resolved) {
      stat_fingerprint_misses.fetch_add(1u, std::memory_order_relaxed);
      continue;
    }
    if (!record.has_resource || !IsSrvDescriptorType(record.type)) continue;
    const reshade::api::resource_desc desc = device->get_resource_desc(record.resource);
    if (desc.type != reshade::api::resource_type::texture_2d) continue;
    out.resource = record.resource;
    out.format = record.format != reshade::api::format::unknown ? record.format : desc.texture.format;
    slot_hit[li].store(1);
    return true;
  }
  slot_hit[li].store(0);
  return false;
}

inline bool ResolveB0(
    reshade::api::command_list* cmd_list,
    const CommandListData* data,
    reshade::api::buffer_range& out) {
  out = {};
  if (cmd_list == nullptr || data == nullptr) return false;
  reshade::api::pipeline_layout layout = {};
  if (!CurrentPixelLayout(cmd_list, layout)) {
    slot_hit[0].store(0);
    return false;
  }
  uint32_t params[4] = {};
  uint32_t bindings[4] = {};
  const uint32_t n = FindTablesForRegister(
      layout, kSlotCbv, 0u, reshade::api::descriptor_type::constant_buffer, params, bindings, 4u);
  auto* device = cmd_list->get_device();
  for (uint32_t k = 0u; k < n; ++k) {
    if (params[k] >= data->descriptor_tables.size()) continue;
    const auto table = data->descriptor_tables[params[k]];
    if (table.handle == 0u) continue;
    DescriptorFingerprint key;
    if (!ReadSlotFingerprint(device, table, bindings[k], key)) continue;
    RecordedDescriptor record;
    if (!LookupFingerprint(key, record)) continue;
    if (!record.has_range || record.type != reshade::api::descriptor_type::constant_buffer) continue;
    // The recorded range carries the view's own byte offset, so the engine's
    // per-frame slot inside one large buffer needs no address arithmetic.
    out = record.range;
    slot_hit[0].store(1);
    return true;
  }
  slot_hit[0].store(0);
  return false;
}
// ── NGX ──
// NGX reports its own diagnostics through a callback the app supplies at init,
// and the SDK documentation points at those logs when something fails. They are
// captured into a small ring instead of the game log: the log is unreadable at
// this scale, and these are exactly the lines the report should carry — the
// runtime's own version, what it made of the parameters it was given, and any
// internal complaint. The callback can be called from any thread, concurrently.
struct NgxLog {
  std::mutex mutex;
  std::array<std::string, 24> lines;
  uint32_t next = 0u;      // where the next line is written
  uint32_t kept = 0u;      // lines currently in the ring
  std::string previous;
  std::atomic<uint64_t> seen{0u};     // messages accepted
  std::atomic<uint64_t> repeats{0u};  // identical consecutive messages collapsed
};

inline NgxLog ngx_log;

inline void NVSDK_CONV OnNgxLog(
    const char* message, NVSDK_NGX_Logging_Level level, NVSDK_NGX_Feature /*source*/) {
  if (message == nullptr) return;
  std::string text(message);
  if (text.size() > 200u) text.resize(200u);
  const std::lock_guard<std::mutex> lock(ngx_log.mutex);
  // NGX repeats the same line every frame in some paths; collapse those runs so
  // the ring holds distinct information rather than one message 24 times.
  if (ngx_log.kept > 0u && text == ngx_log.previous) {
    ngx_log.repeats.fetch_add(1u, std::memory_order_relaxed);
    return;
  }
  ngx_log.previous = text;
  text.insert(0u, level == NVSDK_NGX_LOGGING_LEVEL_VERBOSE ? "v|" : "i|");
  ngx_log.lines[ngx_log.next] = text;
  ngx_log.next = (ngx_log.next + 1u) % static_cast<uint32_t>(ngx_log.lines.size());
  if (ngx_log.kept < ngx_log.lines.size()) ++ngx_log.kept;
  ngx_log.seen.fetch_add(1u, std::memory_order_relaxed);
}

struct InstanceData {
  Microsoft::WRL::ComPtr<ID3D12Device> device;
  Microsoft::WRL::ComPtr<ID3D12Resource> output_texture;
  NVSDK_NGX_Parameter* capability_parameters = nullptr;
  NVSDK_NGX_Parameter* runtime_parameters = nullptr;
  NVSDK_NGX_Handle* feature = nullptr;
  uint32_t width = 0u;
  uint32_t height = 0u;
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
    default: return "Unknown";
  }
}

inline bool IsSupported() { return any_nvidia.load() && (!ngx.initialized || ngx.supported); }

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

inline void ReleaseNgx() {
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
  // Fingerprints are deliberately kept across device churn: they describe views,
  // and the pool they come from is written at level load.
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
  // Ask NGX for its diagnostics rather than relying on the game log: these land
  // in the report, which is where every other diagnostic for this addon lives.
  feature_info.LoggingInfo.LoggingCallback = &OnNgxLog;
  feature_info.LoggingInfo.MinimumLoggingLevel = NVSDK_NGX_LOGGING_LEVEL_ON;
  feature_info.LoggingInfo.DisableOtherLoggingSinks = false;

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
  // The unpack pass decodes packed MVs to pixel units, so MVLowRes must NOT be
  // set. The scene is rendered jittered, so MVs span jittered frames.
  int flags = 0;
  if (dlss_depth_inverted != 0.f) flags |= NVSDK_NGX_DLSS_Feature_Flags_DepthInverted;
  if (dlss_flag_is_hdr != 0.f) flags |= NVSDK_NGX_DLSS_Feature_Flags_IsHDR;
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
  if (input_width == 0u || input_height == 0u || input_width == target_width) {
    return static_cast<int>(NVSDK_NGX_PerfQuality_Value_DLAA);
  }
  // Each quality name is one fixed ratio, and DLSS derives the extent it works at
  // from it: asking for the wrong one makes it compute and write at a different
  // size than the resource it has been given. The closest ratio wins, measured in
  // log space so 1.5x and 2x are equidistant from 1.7x rather than 1.7x being nearer.
  const float ratio = static_cast<float>(target_width) / static_cast<float>(input_width);
  struct QualityRatio {
    float factor;
    NVSDK_NGX_PerfQuality_Value value;
  };
  static const QualityRatio kRatios[] = {
      {1.5f, NVSDK_NGX_PerfQuality_Value_MaxQuality},
      {1.7f, NVSDK_NGX_PerfQuality_Value_Balanced},
      {2.0f, NVSDK_NGX_PerfQuality_Value_MaxPerf},
      {3.0f, NVSDK_NGX_PerfQuality_Value_UltraPerformance},
  };
  float best_delta = 1e9f;
  int best = static_cast<int>(NVSDK_NGX_PerfQuality_Value_MaxQuality);
  for (const QualityRatio& candidate : kRatios) {
    const float delta = fabsf(logf(ratio) - logf(candidate.factor));
    if (delta >= best_delta) continue;
    best_delta = delta;
    best = static_cast<int>(candidate.value);
  }
  return best;
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
      && ngx.feature_flags == feature_flags) {
    return true;
  }
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

// ── MV unpack (decodes the packed t0 to input-resolution RG16F pixel MVs) ──
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
  pc_range.count = 8;  // float4 + float4: output size, scale, source size
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
  reshade::api::resource_view_desc vd(
      reshade::api::resource_view_type::texture_2d, reshade::api::format::r16g16_float, 0, 1, 0, 1);
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
    uint32_t width, uint32_t height, uint32_t src_width, uint32_t src_height) {
  if (!EnsureUnpackResources(device, width, height)) return false;
  if (packed_mv_srv.handle == 0u || unpack.srv.handle == 0u || unpack.uav.handle == 0u) return false;
  // The packed buffer is half the input resolution; the output is input
  // resolution so NGX gets MVs matching the color buffer.
  if (src_width == 0u) src_width = width;
  if (src_height == 0u) src_height = height;

  const auto AC = reshade::api::pipeline_stage::all_compute;
  const auto CS = reshade::api::shader_stage::all_compute;
  cmd_list->bind_pipeline(AC, unpack.pipeline);

  reshade::api::descriptor_table_update updates[2] = {
      {unpack.tables[0], 0, 0, 1, reshade::api::descriptor_type::texture_shader_resource_view, &packed_mv_srv},
      {unpack.tables[1], 0, 0, 1, reshade::api::descriptor_type::texture_unordered_access_view, &unpack.uav},
  };
  device->update_descriptor_tables(2, updates);
  cmd_list->bind_descriptor_tables(CS, unpack.layout, 0, 2, unpack.tables);

  const float pc[8] = {static_cast<float>(width), static_cast<float>(height),
                       dlss_mv_scale * (dlss_mv_flip != 0.f ? -1.f : 1.f), 0.f,
                       static_cast<float>(src_width), static_cast<float>(src_height), 0.f, 0.f};
  cmd_list->push_constants(CS, unpack.layout, 2, 0, 8, pc);
  cmd_list->dispatch((width + 7u) / 8u, (height + 7u) / 8u, 1);
  cmd_list->barrier(unpack.texture, reshade::api::resource_usage::unordered_access, reshade::api::resource_usage::shader_resource);
  return true;
}

// ── Draw identification ──
inline bool logged_taa_draw_detected = false;
inline bool logged_missing_constants = false;
inline bool logged_missing_inputs = false;
inline bool logged_unsupported_output = false;
inline bool logged_bypass_format = false;
inline bool logged_size_guard = false;

// Last deployment snapshot, for the Status readout and the report.
struct DeployReport {
  uint32_t input_width = 0u;
  uint32_t input_height = 0u;
  uint32_t target_width = 0u;
  uint32_t target_height = 0u;
  uint32_t mv_width = 0u;
  uint32_t mv_height = 0u;
  uint32_t depth_width = 0u;
  uint32_t depth_height = 0u;
  int quality = -1;
  int preset = -1;
  int flags = 0;
  float jitter_x = 0.f;
  float jitter_y = 0.f;
  uint32_t depth_state_before = 0u;
  bool depth_transitioned = false;
  uint64_t evals = 0u;
  uint64_t resets = 0u;
  uint64_t bypass = 0u;
};
inline std::mutex report_mutex;
inline DeployReport deploy_report;
// Where in the engine's per-draw constant ring the TAA's own b0 slot sat, in
// bytes. Context for the jitter work: the scene passes' slots, which carry the
// game's jitter, sit at a fixed distance from it.
inline std::atomic<uint64_t> last_b0_offset{0u};
// Packed input/target dimensions: a change is the only thing that legitimately
// invalidates DLSS history. The engine ping-pongs its color and history
// textures every frame, so resetting on handle changes would restart DLSS on
// every frame and destroy the temporal accumulation.
inline uint64_t last_dims = 0u;

inline bool IsTaaDraw(reshade::api::command_list* cmd_list, CommandListData*& data, uint32_t& hash) {
  auto* shader_state = renodx::utils::shader::GetCurrentState(cmd_list);
  if (shader_state == nullptr) return false;
  hash = renodx::utils::shader::GetCurrentPixelShaderHash(shader_state);
  if (hash != kTaaMainHash && hash != kTaaPauseHash) return false;
  data = Get(cmd_list);
  return data != nullptr && data->rtvs[0].handle != 0u;
}

// Draw-time TAA check with a memoized PS hash: the state lookup runs at most
// once per pipeline bind, not once per draw.
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

inline ID3D12Resource* GetNativeResource(reshade::api::device* device, reshade::api::resource_view view) {
  if (device == nullptr || view.handle == 0u) return nullptr;
  const auto resource = device->get_resource_from_view(view);
  if (resource.handle == 0u) return nullptr;
  return reinterpret_cast<ID3D12Resource*>(resource.handle);
}

// Shared failure path. Returns false (vanilla TAA) always.
inline bool FailInputs(
    ID3D12Resource* color, ID3D12Resource* motion, ID3D12Resource* depth, ID3D12Resource* target) {
  NoteFail(kFailResolve);
  if (!logged_missing_inputs) {
    logged_missing_inputs = true;
    std::stringstream s;
    s << "Far Cry 6 DLSS: TAA draw found, but inputs did not resolve"
      << " color=" << reinterpret_cast<void*>(color)
      << " motion=" << reinterpret_cast<void*>(motion)
      << " depth=" << reinterpret_cast<void*>(depth)
      << " target=" << reinterpret_cast<void*>(target);
    reshade::log::message(reshade::log::level::warning, s.str().c_str());
  }
  return false;
}

// Copies src into the TAA target with explicit barriers on both, restoring both
// afterwards: the target goes back to being the render target it was, and src
// back to the state it arrived in. Used by the deployment path and by the
// bypass, so both exercise identical plumbing.
inline bool CopyThroughTaaTarget(
    reshade::api::command_list* cmd_list,
    ID3D12GraphicsCommandList* command_list,
    ID3D12Resource* output_target,
    ID3D12Resource* src,
    reshade::api::resource_usage src_state) {
  if (cmd_list == nullptr || command_list == nullptr || output_target == nullptr || src == nullptr) return false;
  const reshade::api::resource resources[2] = {
      reshade::api::resource{reinterpret_cast<uint64_t>(src)},
      reshade::api::resource{reinterpret_cast<uint64_t>(output_target)}};
  const reshade::api::resource_usage before[2] = {
      src_state, reshade::api::resource_usage::render_target};
  const reshade::api::resource_usage after[2] = {
      reshade::api::resource_usage::copy_source, reshade::api::resource_usage::copy_dest};
  cmd_list->barrier(2u, resources, before, after);
  command_list->CopyResource(output_target, src);
  cmd_list->barrier(2u, resources, after, before);
  return true;
}

// Re-applies the descriptor heaps and graphics root signature the game had bound.
// NGX reaches the native command list directly, so its dispatches bind their own
// heap without passing through ReShade's hooks: ReShade still believes the game's
// heap is current, which is what makes this call effective — ReShade re-applies
// it precisely because it thinks nothing changed. Root parameters and root
// signatures are per stage and survive, but the descriptor heap binding is shared
// between compute and graphics, so the game's next draw would read its
// descriptors out of NGX's heap. Passing no tables forces the heap and root
// signature update without touching the root descriptor tables themselves.
inline void RestoreGameDescriptorState(reshade::api::command_list* cmd_list) {
  if (cmd_list == nullptr) return;
  reshade::api::pipeline_layout layout = {};
  if (!CurrentPixelLayout(cmd_list, layout) || layout.handle == 0u) return;
  cmd_list->bind_descriptor_tables(
      reshade::api::shader_stage::all_graphics, layout, 0u, 0u, nullptr);
}

// ── FSR1 as the upscaler ──
// With FSR1 on, the game upscales in its own pass: a compute dispatch that reads
// the internal-resolution image and writes the display-resolution one, with the
// minimap and UI composited afterwards instead of inside it. That is the place for
// DLSS to upscale - the input is still the scene's own aliased colour, and the
// game's later stages keep reading exactly what they expect.
inline constexpr uint32_t kFsr1EasuHash = 0x4E927DB0u;
inline std::atomic<bool> fsr1_active{false};   // seen last frame, so this one too
inline bool fsr1_seen_this_frame = false;
inline std::atomic<uint32_t> fsr1_target_width{0u};
inline std::atomic<uint32_t> fsr1_target_height{0u};
inline std::atomic<uint32_t> fsr1_target_format{0u};
inline std::atomic<uint64_t> fsr1_output_resource{0u};
inline std::atomic<uint64_t> stat_fsr1_replaced{0u};
inline std::atomic<uint64_t> stat_fsr1_misses{0u};
// Set when the evaluation wrote the game's upscale resource this frame, which is
// the only case in which the game's own dispatch may be taken out.
inline std::atomic<bool> fsr1_wrote_this_frame{false};

// The upscaler's output is the display-resolution UAV it dispatches to. Which slot
// it sits in is not assumed: the bindings are walked and the largest target wins,
// which is the display-resolution image rather than any scratch buffer.
inline bool ResolveDispatchOutput(
    reshade::api::command_list* cmd_list,
    const CommandListData* data,
    uint32_t expected_width, uint32_t expected_height,
    reshade::api::resource& out_resource,
    uint32_t& out_width, uint32_t& out_height, uint32_t& out_format) {
  out_resource = {};
  out_width = 0u;
  out_height = 0u;
  out_format = 0u;
  auto* device = cmd_list != nullptr ? cmd_list->get_device() : nullptr;
  if (device == nullptr || data == nullptr) return false;
  auto* shader_state = renodx::utils::shader::GetCurrentState(cmd_list);
  if (shader_state == nullptr) return false;
  auto* compute_state = renodx::utils::shader::GetCurrentComputeState(shader_state);
  if (compute_state == nullptr) return false;
  renodx::utils::shader::PopulateStageState(compute_state);
  if (compute_state->pipeline_details == nullptr) return false;
  const reshade::api::pipeline_layout layout = compute_state->pipeline_details->layout;
  if (layout.handle == 0u) return false;

  uint32_t params[8] = {};
  uint32_t bindings[8] = {};
  const uint32_t count = FindTablesForRegister(
      layout, 0u, 0u, reshade::api::descriptor_type::texture_unordered_access_view, params, bindings, 8u);
  uint64_t best_area = 0u;
  for (uint32_t i = 0u; i < count; ++i) {
    if (params[i] >= data->descriptor_tables.size()) continue;
    const auto table = data->descriptor_tables[params[i]];
    if (table.handle == 0u) continue;
    DescriptorFingerprint key;
    if (!ReadSlotFingerprint(device, table, bindings[i], key)) continue;
    RecordedDescriptor record;
    if (!LookupFingerprint(key, record)) continue;
    if (!record.has_resource) continue;
    const reshade::api::resource_desc desc = device->get_resource_desc(record.resource);
    if (desc.type != reshade::api::resource_type::texture_2d) continue;
    // The target has to be exactly the image DLSS produced. A descriptor slot is
    // recycled and its bytes can name a different resource entirely, and copying
    // into a 16x16 one is an invalid call that takes the device with it - so the
    // size is the test, not being the largest binding here.
    // Identifying the upscaler's output must not depend on the size this frame is
    // currently writing, or the two can never meet: the target is recognised by
    // being a plain 2D texture larger than the render extent, in the render's own
    // aspect, in a format that can be written.
    if (desc.texture.width <= expected_width || desc.texture.height <= expected_height) continue;
    if (desc.texture.levels != 1u || desc.texture.depth_or_layers != 1u || desc.texture.samples != 1u) continue;
    if (static_cast<uint64_t>(desc.texture.width) * expected_height
        != static_cast<uint64_t>(desc.texture.height) * expected_width) {
      continue;
    }
    const auto candidate_format = static_cast<DXGI_FORMAT>(desc.texture.format);
    if (candidate_format != DXGI_FORMAT_R8G8B8A8_UNORM
        && candidate_format != DXGI_FORMAT_R16G16B16A16_FLOAT
        && candidate_format != DXGI_FORMAT_R11G11B10_FLOAT) {
      continue;
    }
    const uint64_t area = static_cast<uint64_t>(desc.texture.width) * desc.texture.height;
    if (area < best_area) continue;
    best_area = area;
    out_resource = record.resource;
    out_width = desc.texture.width;
    out_height = desc.texture.height;
    out_format = static_cast<uint32_t>(desc.texture.format);
  }
  return out_resource.handle != 0u;
}

// Replaces the upscaler dispatch: DLSS has already produced the display-resolution
// image this frame, so it is moved into the resource the dispatch would have
// written and the dispatch itself is prevented.
inline bool OnDispatch(reshade::api::command_list* cmd_list, uint32_t, uint32_t, uint32_t) {
  if (dlss_enabled == 0.f || !any_nvidia.load()) return false;
  if (cmd_list == nullptr) return false;
  auto* shader_state = renodx::utils::shader::GetCurrentState(cmd_list);
  if (shader_state == nullptr) return false;
  if (renodx::utils::shader::GetCurrentComputeShaderHash(shader_state) != kFsr1EasuHash) return false;
  // Seeing the dispatch is what tells the frame that the game has an upscaler to
  // hand over - without this the evaluation never targets the display resolution
  // and the dispatch is never prevented, each waiting on the other.
  fsr1_seen_this_frame = true;
  auto* data = Get(cmd_list);
  if (data == nullptr) return false;

  const uint32_t render_width = last_render_width.load();
  const uint32_t render_height = last_render_height.load();
  reshade::api::resource output = {};
  uint32_t width = 0u, height = 0u, format = 0u;
  if (render_width == 0u || render_height == 0u
      || !ResolveDispatchOutput(cmd_list, data, render_width, render_height, output, width, height, format)) {
    stat_fsr1_misses.fetch_add(1u, std::memory_order_relaxed);
    return false;
  }
  TrackDepthResource(output.handle);
  // What the upscaler writes is what the next frame's evaluation targets: the size
  // can only be learned from here, and it must not be matched against this frame's
  // output size or the two can never meet.
  fsr1_target_width.store(width);
  fsr1_target_height.store(height);
  fsr1_target_format.store(format);
  fsr1_output_resource.store(output.handle);
  // NGX writes into this resource directly, at the evaluation point, so there is
  // nothing to copy: this only takes the game's dispatch out of the way once that
  // write has actually happened. A frame where it did not keeps the game's upscale.
  if (!fsr1_wrote_this_frame.load()) return false;
  stat_fsr1_replaced.fetch_add(1u, std::memory_order_relaxed);
  return true;
}

inline bool EvaluateDLSS(reshade::api::command_list* cmd_list, const CommandListData* data, uint32_t hash) {
  if (cmd_list == nullptr || data == nullptr || dlss_enabled == 0.f || !IsSupported()) return false;
  auto* device = cmd_list->get_device();
  if (device == nullptr || device->get_api() != reshade::api::device_api::d3d12) return false;

  auto* command_list = reinterpret_cast<ID3D12GraphicsCommandList*>(cmd_list->get_native());
  if (command_list == nullptr) return false;
  if (ngx.eval_failed) return false;

  for (auto& hit : slot_hit) hit.store(0);

  // Per-device gate: a non-NVIDIA device's command lists keep vanilla TAA.
  if (!IsKnownNvidiaNative(device->get_native())) {
    NoteFail(kFailDevice);
    return false;
  }

  // All four inputs are resolved every draw: both depth candidates are read so
  // the probe always sees the full set, and the setting picks which one feeds
  // NGX.
  SlotInput color_in;
  SlotInput motion_in;
  SlotInput depth_a_in;
  SlotInput depth_b_in;
  const bool color_ok = ResolveSlot(cmd_list, data, kSlotColor, 1u, color_in);
  const bool motion_ok = ResolveSlot(cmd_list, data, kSlotPackedMv, 0u, motion_in);
  const bool depth_a_ok = ResolveSlot(cmd_list, data, kSlotDepthA, 2u, depth_a_in);
  const bool depth_b_ok = ResolveSlot(cmd_list, data, kSlotDepthB, 3u, depth_b_in);
  reshade::api::buffer_range b0_range = {};
  const bool b0_ok = ResolveB0(cmd_list, data, b0_range);
  if (b0_ok && b0_range.offset != UINT64_MAX) last_b0_offset.store(b0_range.offset);
  if (!b0_ok && !logged_missing_constants) {
    logged_missing_constants = true;
    reshade::log::message(reshade::log::level::warning, "Far Cry 6 DLSS: TAA draw found, but b0 was not resolved");
  }

  // Input substitution. Both depth candidates are always resolved so the probe
  // keeps reporting the full set; the settings then decide what actually reaches
  // NGX. Substituting an input replaces it with a well-formed stand-in, so a
  // substituted input can never fail resolution.
  const bool use_depth_placeholder = dlss_depth_source >= 2.5f;
  const bool use_depth_mask = !use_depth_placeholder && dlss_depth_source > 0.5f;
  const bool use_depth_mask_b = use_depth_mask && dlss_depth_source > 1.5f;
  const bool mask_ok = use_depth_mask_b ? depth_b_ok : depth_a_ok;
  const bool use_color_placeholder = dlss_color_source >= 0.5f;
  const bool use_motion_placeholder = dlss_motion_source >= 0.5f;
  const bool use_bypass = dlss_bypass_ngx != 0.f;

  auto* output_target = GetNativeResource(device, data->rtvs[0]);
  if (output_target == nullptr) {
    return FailInputs(nullptr, nullptr, nullptr, output_target);
  }
  const D3D12_RESOURCE_DESC taa_desc = output_target->GetDesc();
  const uint32_t taa_width = static_cast<uint32_t>(taa_desc.Width);
  const uint32_t taa_height = taa_desc.Height;

  if ((use_color_placeholder || use_motion_placeholder || use_depth_placeholder || use_bypass)
      && !EnsurePlaceholders(device, taa_width, taa_height)) {
    NoteFail(kFailPlaceholder);
    return FailInputs(nullptr, nullptr, nullptr, output_target);
  }

  auto* color = use_color_placeholder
      ? reinterpret_cast<ID3D12Resource*>(placeholders.color.handle)
      : (color_ok ? reinterpret_cast<ID3D12Resource*>(color_in.resource.handle) : nullptr);
  auto* motion = use_motion_placeholder
      ? reinterpret_cast<ID3D12Resource*>(placeholders.motion.handle)
      : (motion_ok ? reinterpret_cast<ID3D12Resource*>(motion_in.resource.handle) : nullptr);

  // Scene depth: only usable when it matches this frame's render size.
  ID3D12Resource* scene_depth = nullptr;
  if (!use_depth_mask && !use_depth_placeholder && scene_depth_resource != 0u) {
    const reshade::api::resource_desc scene_desc =
        device->get_resource_desc(reshade::api::resource{scene_depth_resource});
    if (scene_desc.type == reshade::api::resource_type::texture_2d
        && scene_desc.texture.width == taa_width && scene_desc.texture.height == taa_height) {
      scene_depth = reinterpret_cast<ID3D12Resource*>(scene_depth_resource);
    }
  }
  SlotInput depth_in = use_depth_mask ? (use_depth_mask_b ? depth_b_in : depth_a_in) : SlotInput{};
  auto* depth = use_depth_placeholder
      ? reinterpret_cast<ID3D12Resource*>(placeholders.depth.handle)
      : (use_depth_mask ? reinterpret_cast<ID3D12Resource*>(depth_in.resource.handle) : scene_depth);
  const bool depth_ok = use_depth_placeholder
      || (use_depth_mask ? mask_ok : (scene_depth != nullptr));

  if (!use_bypass && color == nullptr) {
    return FailInputs(color, motion, depth, output_target);
  }
  if (!use_bypass && (motion == nullptr || !depth_ok || depth == nullptr)) {
    return FailInputs(color, motion, depth, output_target);
  }

  // Resolve-only mode: inputs are resolved and reported, the game's own TAA
  // keeps producing the image. Isolates our resolution work from our GPU work.
  // Bypass always writes, so it takes precedence over this.
  if (!use_bypass && dlss_output_mode >= 1.5f) {
    stat_fail_code.store(kFailNone);
    return false;
  }

  const D3D12_RESOURCE_DESC color_desc = color != nullptr ? color->GetDesc() : D3D12_RESOURCE_DESC{};
  const D3D12_RESOURCE_DESC motion_desc_early = motion != nullptr ? motion->GetDesc() : D3D12_RESOURCE_DESC{};
  const D3D12_RESOURCE_DESC depth_desc_early = depth != nullptr ? depth->GetDesc() : D3D12_RESOURCE_DESC{};
  const D3D12_RESOURCE_DESC output_desc = output_target->GetDesc();
  // The input size comes from the TAA target, not from whatever the colour slot
  // resolved to: the target is the render resolution by definition, so a wrong
  // colour resolution is caught below instead of silently becoming the input.
  const uint32_t input_width = static_cast<uint32_t>(output_desc.Width);
  const uint32_t input_height = output_desc.Height;
  // With the game's own upscaler running, DLSS *is* the upscaler: its output is the
  // display-resolution image the upscaler would have written, at the size the last
  // replaced dispatch reported.
  const uint64_t fsr1_target_handle = fsr1_output_resource.load();
  // The resource is written by NGX directly, so it may only be used once the
  // engine has stated a state for it: an unknown state means no write this frame.
  const reshade::api::resource_usage fsr1_target_state = fsr1_target_handle != 0u
      ? TrackedDepthState(fsr1_target_handle)
      : reshade::api::resource_usage::undefined;
  const bool fsr1_mode = fsr1_active.load() && fsr1_target_width.load() != 0u
      && fsr1_target_handle != 0u
      && fsr1_target_state != reshade::api::resource_usage::undefined;
  const reshade::api::resource fsr1_target_resource{fsr1_target_handle};
  const bool fsr1_needs_write_transition = fsr1_mode
      && (static_cast<uint32_t>(fsr1_target_state)
          & static_cast<uint32_t>(reshade::api::resource_usage::unordered_access)) == 0u;
  const uint32_t target_width = fsr1_mode ? fsr1_target_width.load() : input_width;
  const uint32_t target_height = fsr1_mode ? fsr1_target_height.load() : input_height;
  if (input_width == 0u || input_height == 0u) {
    NoteFail(kFailSize);
    return false;
  }
  // Every input must belong to this render resolution: colour and depth at the
  // input size, motion vectors at half of it (or the same size). This is the
  // check that rejects a slot which resolved to an unrelated texture — the
  // 1024x1024 motion resource that produced the accumulating artifacts. A
  // substituted input is a texture we made at this exact size, and the bypass
  // touches no game input at all.
  if (!use_bypass) {
    const bool color_matches = use_color_placeholder
        || (color_desc.Width == input_width && color_desc.Height == input_height);
    const bool depth_matches = use_depth_placeholder
        || (depth_desc_early.Width == input_width && depth_desc_early.Height == input_height);
    const bool motion_matches = use_motion_placeholder
        || (motion_desc_early.Width == input_width / 2u && motion_desc_early.Height == input_height / 2u)
        || (motion_desc_early.Width == input_width && motion_desc_early.Height == input_height);
    if (!color_matches || !motion_matches || (use_depth_mask && !depth_matches)) {
      NoteFail(kFailSize);
      if (!logged_size_guard) {
        logged_size_guard = true;
        char buf[256];
        snprintf(buf, sizeof(buf),
                 "Far Cry 6 DLSS: input size mismatch color=%ux%u motion=%ux%u depth=%ux%u target=%ux%u",
                 static_cast<uint32_t>(color_desc.Width), color_desc.Height,
                 static_cast<uint32_t>(motion_desc_early.Width), motion_desc_early.Height,
                 static_cast<uint32_t>(depth_desc_early.Width), depth_desc_early.Height,
                 input_width, input_height);
        reshade::log::message(reshade::log::level::warning, buf);
      }
      return false;
    }
  }
  // DLSS ratios top out at 3x per axis (Ultra Performance); DLAA requires
  // equality. Reject anything outside that rather than feed NGX nonsense.
  {
    bool sane = input_width >= 256u && input_height >= 256u && target_width >= 256u && target_height >= 256u
        && target_width <= 7680u && target_height <= 4320u
        && target_width <= input_width * 4u && target_height <= input_height * 4u;
    if (sane && static_cast<int>(dlss_mode) == 1
        && (input_width != target_width || input_height != target_height)) {
      sane = false;
    }
    if (!sane) {
      NoteFail(kFailSize);
      if (!logged_size_guard) {
        logged_size_guard = true;
        char buf[192];
        snprintf(buf, sizeof(buf),
                 "Far Cry 6 DLSS: rejecting implausible sizes in=%ux%u target=%ux%u",
                 input_width, input_height, target_width, target_height);
        reshade::log::message(reshade::log::level::warning, buf);
      }
      return false;
    }
  }
  // DLSS ratios top out at 3x per axis (Ultra Performance); DLAA requires
  // equality. Reject anything outside that rather than feed NGX nonsense.
  {
    bool sane = input_width >= 256u && input_height >= 256u && target_width >= 256u && target_height >= 256u
        && target_width <= 7680u && target_height <= 4320u
        && target_width <= input_width * 4u && target_height <= input_height * 4u;
    if (sane && static_cast<int>(dlss_mode) == 1
        && (input_width != target_width || input_height != target_height)) {
      sane = false;
    }
    if (!sane) {
      NoteFail(kFailSize);
      if (!logged_size_guard) {
        logged_size_guard = true;
        char buf[192];
        snprintf(buf, sizeof(buf),
                 "Far Cry 6 DLSS: rejecting implausible sizes in=%ux%u target=%ux%u",
                 input_width, input_height, target_width, target_height);
        reshade::log::message(reshade::log::level::warning, buf);
      }
      return false;
    }
  }

  // Published before anything can return early: the depth selection filters its
  // candidates by this size, and the projection test needs the aspect. Leaving them
  // to the evaluation meant that when the evaluation stopped running, the depth
  // selection fell back to ranking by draw count and picked a shadow cascade.
  last_render_width.store(input_width);
  last_render_height.store(input_height);
  projection_aspect_xx = input_height != 0u
      ? static_cast<float>(input_width) / static_cast<float>(input_height)
      : 0.f;

  // NGX output format follows whatever the output resource will be: the TAA target
  // normally, the upscaler's target when DLSS is doing the upscaling.
  const DXGI_FORMAT wanted_format = (fsr1_mode && fsr1_target_format.load() != 0u)
      ? static_cast<DXGI_FORMAT>(fsr1_target_format.load())
      : output_desc.Format;
  DXGI_FORMAT output_format = DXGI_FORMAT_UNKNOWN;
  switch (wanted_format) {
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

  // Bypass: a known-good image (flat grey) written through the same transitions
  // and copy the DLSS path uses, with NGX not involved at all. Artifacts that
  // survive this are ours — the barrier/copy plumbing — and not DLSS's, and the
  // game's own TAA draw is skipped so nothing overwrites what we wrote.
  if (use_bypass) {
    if (!EnsureOutputTexture(reinterpret_cast<ID3D12Device*>(device->get_native()),
                             target_width, target_height, output_format)) {
      NoteFail(kFailPlaceholder);
      return false;
    }
    auto* grey = reinterpret_cast<ID3D12Resource*>(placeholders.color.handle);
    if (grey == nullptr || ngx.output_texture == nullptr) {
      NoteFail(kFailPlaceholder);
      return false;
    }
    // The stand-in is FP16, which is also the format of the TAA target in this
    // game; the copy itself requires the two to be identical, so a target of any
    // other format is reported instead of being copied into incorrectly.
    if (output_format != DXGI_FORMAT_R16G16B16A16_FLOAT) {
      NoteFail(kFailPlaceholder);
      if (!logged_bypass_format) {
        logged_bypass_format = true;
        std::stringstream s;
        s << "Far Cry 6 DLSS: bypass needs a 16-bit float TAA target, this one is format "
          << static_cast<int>(output_desc.Format) << "; skipping";
        reshade::log::message(reshade::log::level::warning, s.str().c_str());
      }
      return false;
    }
    const reshade::api::resource fill_resources[2] = {
        reshade::api::resource{reinterpret_cast<uint64_t>(grey)},
        reshade::api::resource{reinterpret_cast<uint64_t>(ngx.output_texture.Get())}};
    const reshade::api::resource_usage fill_before[2] = {
        reshade::api::resource_usage::shader_resource, reshade::api::resource_usage::unordered_access};
    const reshade::api::resource_usage fill_after[2] = {
        reshade::api::resource_usage::copy_source, reshade::api::resource_usage::copy_dest};
    cmd_list->barrier(2u, fill_resources, fill_before, fill_after);
    command_list->CopyResource(ngx.output_texture.Get(), grey);
    cmd_list->barrier(2u, fill_resources, fill_after, fill_before);

    // With the game's upscaler active, DLSS's texture is display-resolution and the
    // TAA target is not, so there is no correct destination here: the image is left
    // to the game's own upscaler in that configuration rather than copied wrongly.
    if (!fsr1_active.load()) {
      if (!CopyThroughTaaTarget(cmd_list, command_list, output_target, ngx.output_texture.Get(),
                                reshade::api::resource_usage::unordered_access)) {
        NoteFail(kFailPlaceholder);
        return false;
      }
    }
    {
      const std::lock_guard<std::mutex> lock(report_mutex);
      deploy_report.input_width = input_width;
      deploy_report.input_height = input_height;
      deploy_report.target_width = target_width;
      deploy_report.target_height = target_height;
      ++deploy_report.bypass;
    }
    // Leaving bypass resumes DLSS with the history it had before, which describes
    // a different input path; restart it instead.
    ngx.reset = true;
    stat_fail_code.store(kFailNone);
    return true;
  }

  if (!EnsureNgxInitialized(device)) {
    NoteFail(kFailNgxInit);
    return false;
  }
  if (!EnsureOutputTexture(ngx.device.Get(), target_width, target_height, output_format)) return false;

  // Without the game's upscaler there is nowhere for the evaluation's output to go:
  // it writes a display-resolution target, and the internal-resolution TAA target
  // is about to be written by the draw itself - which now runs. The inputs are
  // resolved above, so the report keeps reporting them either way, and nothing is
  // dispatched to the GPU for a result the game would immediately overwrite.
  if (!use_bypass && (!fsr1_active.load() || fsr1_target_width.load() == 0u)) {
    stat_fail_code.store(kFailNone);
    return false;
  }

  const uint32_t motion_src_width = use_motion_placeholder
      ? placeholders.width
      : static_cast<uint32_t>(motion->GetDesc().Width);
  const uint32_t motion_src_height = use_motion_placeholder
      ? placeholders.height
      : static_cast<uint32_t>(motion->GetDesc().Height);
  ID3D12Resource* motion_vectors = nullptr;
  if (use_motion_placeholder) {
    // A substituted motion input is used as-is: it is already pixel-space zero
    // motion at the input resolution, so the unpack has nothing to decode.
    motion_vectors = reinterpret_cast<ID3D12Resource*>(placeholders.motion.handle);
  } else {
    // Our own view of the resolved resource: a recorded handle would name whichever
    // view last occupied that slot, not the one this draw bound.
    const reshade::api::resource_view motion_view =
        GetOrCreateTextureView(device, motion_in.resource, motion_in.format);
    if (motion_view.handle == 0u) {
      NoteFail(kFailUnpack);
      return false;
    }
    if (!DispatchUnpack(device, cmd_list, motion_view, input_width, input_height,
                        motion_src_width, motion_src_height)) {
      NoteFail(kFailUnpack);
      return false;
    }
    motion_vectors = unpack.texture.handle != 0u
        ? reinterpret_cast<ID3D12Resource*>(unpack.texture.handle)
        : nullptr;
  }
  if (motion_vectors == nullptr) return false;

  // Reset DLSS history only on a structural change.
  {
    const uint64_t dims = (static_cast<uint64_t>(input_width) << 48)
                        | (static_cast<uint64_t>(input_height) << 32)
                        | (static_cast<uint64_t>(target_width) << 16)
                        | static_cast<uint64_t>(target_height);
    if (dims != last_dims) {
      last_dims = dims;
      ngx.reset = true;
    }
  }

  const int render_preset = GetRenderPresetValue();
  const int perf_quality = GetPerfQualityValue(input_width, input_height, target_width, target_height);
  // Zero motion does not span jittered frames, so the flag is dropped to match
  // the substitute rather than left describing motion that is no longer there.
  int feature_flags = GetFeatureFlags();
  if (use_motion_placeholder) feature_flags &= ~NVSDK_NGX_DLSS_Feature_Flags_MVJittered;
  if (!EnsureFeature(command_list, input_width, input_height, target_width, target_height,
                     render_preset, feature_flags, perf_quality)) {
    NoteFail(kFailFeature);
    return false;
  }

  // Jitter: read from the scene's own view constants, at the draws that apply it.
  // The ring-wide fallback only runs when that produced nothing and the value is
  // actually being consumed: it is the expensive path, and it is a fallback.
  last_render_width.store(input_width);
  last_render_height.store(input_height);
  // A reading counts only while the scene probe refreshed it this frame: a stale
  // offset is a wrong offset, and zero is the safe value.
  const bool scene_fresh = scene_jitter.valid && scene_jitter.updated_frame == frame_counter;
  if (!scene_fresh && dlss_jitter_source > 0.5f && dlss_jitter_source < 1.5f) {
    RefreshProjectionJitter(b0_range, input_width, input_height);
  }
  float jitter_x = 0.f;
  float jitter_y = 0.f;
  // A sub-pixel offset that cycles through a handful of phases: if feeding this
  // changes the image, DLSS is consuming the jitter and the real value is worth
  // wiring; if it changes nothing, the jitter is not reaching its reprojection at
  // all and neither is the history.
  if (dlss_jitter_source >= 1.5f) {
    // A finite cycled pattern: the first 21 values of a Halton(2,3) sequence, wrapped.
    // That is the shape a renderer's own jitter table has - a fixed set of offsets
    // reused every cycle - so the debug overlay holds a steady set of points instead
    // of filling the way a never-repeating sequence does.
    static uint32_t pattern_index = 0u;
    const uint32_t step = (pattern_index++ % 21u) + 1u;
    jitter_x = HaltonValue(step, 2u) - 0.5f;
    jitter_y = HaltonValue(step, 3u) - 0.5f;
  } else if (dlss_jitter_source > 0.5f && scene_fresh) {
    const float jitter_sign = dlss_jitter_flip != 0.f ? -1.f : 1.f;
    jitter_x = scene_jitter.pixels_x * jitter_sign;
    jitter_y = scene_jitter.pixels_y * jitter_sign;
  } else if (dlss_jitter_source > 0.5f && projection_jitter.valid) {
    const float jitter_sign = dlss_jitter_flip != 0.f ? -1.f : 1.f;
    jitter_x = projection_jitter.pixels_x * jitter_sign;
    jitter_y = projection_jitter.pixels_y * jitter_sign;
  }
  // Nothing that is not a finite number reaches NGX, and nothing outside
  // sub-pixel range either: a jitter is a sub-pixel offset, so a larger value can
  // only be a read error.
  if (!std::isfinite(jitter_x) || !std::isfinite(jitter_y)
      || fabsf(jitter_x) > 0.5f || fabsf(jitter_y) > 0.5f) {
    jitter_x = 0.f;
    jitter_y = 0.f;
  }

  NVSDK_NGX_D3D12_DLSS_Eval_Params eval = {};
  eval.Feature.pInColor = color;
  eval.Feature.pInOutput = fsr1_mode
      ? reinterpret_cast<ID3D12Resource*>(fsr1_target_handle)
      : ngx.output_texture.Get();
  eval.pInDepth = depth;
  eval.pInMotionVectors = motion_vectors;
  eval.InJitterOffsetX = jitter_x;
  eval.InJitterOffsetY = jitter_y;
  eval.InRenderSubrectDimensions.Width = input_width;
  eval.InRenderSubrectDimensions.Height = input_height;
  eval.InReset = (ngx.reset || dlss_reset_every_frame != 0.f) ? 1 : 0;
  eval.InMVScaleX = 1.f;
  eval.InMVScaleY = 1.f;
  eval.InPreExposure = 1.f;
  eval.InExposureScale = 1.f;

  if (dlss_debug_logging != 0.f) {
    static int eval_log_count = 0;
    if (++eval_log_count % 60 == 0) {
      char buf[384];
      snprintf(buf, sizeof(buf),
               "Far Cry 6 DLSS: eval hash=0x%08X in=%ux%u target=%ux%u quality=%s preset=%s"
               " jit=(%.3f,%.3f) mv_src=%ux%u flags=0x%X reset=%d",
               hash, input_width, input_height, target_width, target_height,
               GetPerfQualityName(ngx.perf_quality), GetRenderPresetName(ngx.render_preset),
               jitter_x, jitter_y, motion_src_width, motion_src_height, feature_flags, eval.InReset);
      reshade::log::message(reshade::log::level::info, buf);
    }
  }

  // NGX reads the depth as a shader resource in a compute dispatch, so the depth
  // has to be in a non-pixel shader-readable state for that read to return data.
  // The engine leaves it in a depth-stencil state, where the read returns zeros —
  // a black depth view in DLSS's own debug overlay. It is put back into the state
  // the engine's tracking believes it is in afterwards, because the engine writes
  // its next barrier against that belief and not against what actually happened.
  const reshade::api::resource depth_resource{scene_depth_resource};
  reshade::api::resource_usage depth_state = reshade::api::resource_usage::undefined;
  if (!use_depth_mask && !use_depth_placeholder && scene_depth != nullptr) {
    depth_state = TrackedDepthState(scene_depth_resource);
    // Bound as a depth-stencil view, so that is what it is while the tracker has
    // not seen a barrier for it yet.
    if (depth_state == reshade::api::resource_usage::undefined) {
      depth_state = reshade::api::resource_usage::depth_stencil_write;
    }
  }
  const bool depth_needs_read_transition = dlss_transition_depth != 0.f
      && depth_state != reshade::api::resource_usage::undefined
      && (static_cast<uint32_t>(depth_state)
          & static_cast<uint32_t>(reshade::api::resource_usage::shader_resource_non_pixel)) == 0u;
  if (depth_needs_read_transition) {
    cmd_list->barrier(depth_resource, depth_state, reshade::api::resource_usage::shader_resource_non_pixel);
    stat_depth_transitions.fetch_add(1u, std::memory_order_relaxed);
  }
  // NGX writes the upscaler's own resource, which it can only do while that
  // resource is writable.
  if (fsr1_needs_write_transition) {
    cmd_list->barrier(fsr1_target_resource, fsr1_target_state, reshade::api::resource_usage::unordered_access);
  }

  const NVSDK_NGX_Result result = NGX_D3D12_EVALUATE_DLSS_EXT(command_list, ngx.feature, ngx.runtime_parameters, &eval);
  if (depth_needs_read_transition) {
    cmd_list->barrier(depth_resource, reshade::api::resource_usage::shader_resource_non_pixel, depth_state);
  }
  if (fsr1_needs_write_transition) {
    cmd_list->barrier(fsr1_target_resource, reshade::api::resource_usage::unordered_access, fsr1_target_state);
  }
  // Restored before anything else: the evaluation dispatched on the native
  // command list, and the game's own commands follow ours on this same list.
  if (dlss_restore_state != 0.f) RestoreGameDescriptorState(cmd_list);
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
  // The game's dispatch is only taken out for frames DLSS actually wrote.
  if (fsr1_mode) fsr1_wrote_this_frame.store(true);

  {
    const std::lock_guard<std::mutex> lock(report_mutex);
    deploy_report.input_width = input_width;
    deploy_report.input_height = input_height;
    deploy_report.target_width = target_width;
    deploy_report.target_height = target_height;
    deploy_report.mv_width = motion_src_width;
    deploy_report.mv_height = motion_src_height;
    deploy_report.depth_width = static_cast<uint32_t>(depth->GetDesc().Width);
    deploy_report.depth_height = static_cast<uint32_t>(depth->GetDesc().Height);
    deploy_report.quality = ngx.perf_quality;
    deploy_report.preset = ngx.render_preset;
    deploy_report.flags = feature_flags;
    deploy_report.jitter_x = jitter_x;
    deploy_report.jitter_y = jitter_y;
    deploy_report.depth_state_before = static_cast<uint32_t>(depth_state);
    deploy_report.depth_transitioned = depth_needs_read_transition;
    ++deploy_report.evals;
    if (eval.InReset != 0) ++deploy_report.resets;
  }

  // Evaluate-only mode: the whole chain ran, but the game's own TAA draw is
  // allowed to produce the image. Isolates our GPU work from the substitution.
  if (dlss_output_mode >= 0.5f) {
    return false;
  }

  // DLSS wrote the target; move it into the TAA output so the game keeps
  // running its own pipeline on top. With the game's upscaler doing that step,
  // the dispatch moves DLSS's output into the resource it would have written
  // instead, so nothing is copied here.
  if (!fsr1_mode) {
    CopyThroughTaaTarget(cmd_list, command_list, output_target, ngx.output_texture.Get(),
                         reshade::api::resource_usage::unordered_access);
  }

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
      << " depth=" << (use_depth_placeholder ? "placeholder"
          : (use_depth_mask ? (use_depth_mask_b ? "t6 mask" : "t5 mask") : "scene"))
      << " reset=" << eval.InReset;
    reshade::log::message(reshade::log::level::info, s.str().c_str());
  }
  // NOTE: the pause variant writes a second target (alpha/coverage history).
  // DLSS replaces RTV0; RTV1 is left untouched so downstream passes keep
  // reading a valid buffer.
  return true;
}

// ── Report ──
inline std::string BuildDebugReport() {
  const DeployReport report = [&] {
    const std::lock_guard<std::mutex> lock(report_mutex);
    return deploy_report;
  }();
  char buf[4096];
  snprintf(buf, sizeof(buf),
      "Far Cry 6 DLSS report\n"
      "  addon version    : %s\n"
      "  devices          : %s\n"
      "  draws / replaced : %llu / %llu\n"
      "  state            : %s\n"
      "  presents         : %llu\n"
      "  inputs           : colour=%s motion=%s depth=%s bypass=%s\n"
      "  fingerprints     : %llu recorded, %llu collisions, %llu misses, %llu invalidated, stride=%u bytes\n"
      "  slots b0/t0/t2/t5/t6 : %d/%d/%d/%d/%d  (0 miss, 1 resolved)\n"
      "  probe (recorded descriptor lookup)\n"
      "%s"
      "  last eval        : in=%ux%u target=%ux%u mv=%ux%u depth=%ux%u\n"
      "  quality / preset : %s / %s (hint=%d)   flags=0x%X\n"
      "  evals/resets/bypass : %llu / %llu / %llu\n"
      "  jitter ngx px    : (%.4f, %.4f)\n"
      "  mv scale / flip  : %.3f / %s   mv jittered=%s   depth inverted=%s   reset-every-frame=%s\n"
      "  hdr / restore    : %s / %s\n"
      "  depth transition : %s\n"
      "  upscaler         : %s   replaced=%llu misses=%llu target=%ux%u fmt=%u res=0x%llX\n",
      std::string(renodx::build_info::kBuildVersion).c_str(),
      DeviceSummary().c_str(),
      static_cast<unsigned long long>(stat_taa_draws.load()),
      static_cast<unsigned long long>(stat_dlss_ok.load()),
      FailCodeText(stat_fail_code.load()),
      static_cast<unsigned long long>(stat_presents.load()),
      dlss_color_source >= 0.5f ? "placeholder (grey)" : "game (t2)",
      dlss_motion_source >= 0.5f ? "placeholder (zero)" : "game (decoded)",
      dlss_depth_source >= 2.5f ? "placeholder (far)"
          : (dlss_depth_source > 1.5f ? "t6 (mask)" : (dlss_depth_source > 0.5f ? "t5 (mask)" : "scene depth")),
      dlss_bypass_ngx != 0.f ? "on" : "off",
      CountFingerprints(),
      static_cast<unsigned long long>(stat_fingerprint_collisions.load()),
      static_cast<unsigned long long>(stat_fingerprint_misses.load()),
      static_cast<unsigned long long>(stat_fingerprint_invalidated.load()),
      descriptor_stride_bytes.load(),
      slot_hit[0].load(), slot_hit[1].load(), slot_hit[2].load(), slot_hit[3].load(), slot_hit[4].load(),
      BuildProbeReport().c_str(),
      report.input_width, report.input_height, report.target_width, report.target_height,
      report.mv_width, report.mv_height, report.depth_width, report.depth_height,
      GetPerfQualityName(report.quality), GetRenderPresetName(report.preset),
      GetRenderPresetValue(),
      static_cast<unsigned>(report.flags),
      static_cast<unsigned long long>(report.evals),
      static_cast<unsigned long long>(report.resets),
      static_cast<unsigned long long>(report.bypass),
      static_cast<double>(report.jitter_x), static_cast<double>(report.jitter_y),
      static_cast<double>(dlss_mv_scale),
      dlss_mv_flip != 0.f ? "flipped" : "normal",
      dlss_motion_vectors_jittered != 0.f ? "on" : "off",
      dlss_depth_inverted != 0.f ? "on" : "off",
      dlss_reset_every_frame != 0.f ? "on" : "off",
      dlss_flag_is_hdr != 0.f ? "on" : "off",
      dlss_restore_state != 0.f ? "on" : "off",
      dlss_transition_depth != 0.f ? "on" : "off",
      fsr1_active.load() ? "game upscaler (DLSS upscales there)" : "game (DLSS at the TAA, internal res)",
      static_cast<unsigned long long>(stat_fsr1_replaced.load()),
      static_cast<unsigned long long>(stat_fsr1_misses.load()),
      fsr1_target_width.load(), fsr1_target_height.load(), fsr1_target_format.load(),
      static_cast<unsigned long long>(fsr1_output_resource.load()));
  std::string out(buf);
  // What actually feeds NGX as depth, and how it was found: the scene depth is
  // identified by usage, so its resource, size, format and bind count are the
  // evidence that the right texture was picked rather than a half-res effect's.
  {
    char line[256];
    if (scene_depth_resource != 0u) {
      snprintf(line, sizeof(line),
               "  scene depth      : 0x%llX %ux%u fmt=%d bound by %u draws\n",
               static_cast<unsigned long long>(scene_depth_resource),
               scene_depth_width, scene_depth_height, static_cast<int>(scene_depth_format),
               scene_depth_draws);
    } else {
      snprintf(line, sizeof(line), "  scene depth      : not identified\n");
    }
    out += line;
  }
  // The TAA's own constant slot, in the engine's per-draw ring.
  {
    char line[128];
    snprintf(line, sizeof(line), "  taa b0 slot      : %llu\n",
             static_cast<unsigned long long>(last_b0_offset.load()));
    out += line;
  }
  // What state the depth was in when NGX read it, and whether that needed a
  // transition. A depth buffer in a depth-stencil state reads as zeros, which is
  // silent everywhere except DLSS's own debug overlay.
  {
    char line[320];
    const auto before = static_cast<reshade::api::resource_usage>(report.depth_state_before);
    if (report.depth_state_before == 0u) {
      snprintf(line, sizeof(line), "  depth read state : n/a (placeholder or mask depth)\n");
    } else if (report.depth_transitioned) {
      snprintf(line, sizeof(line), "  depth read state : %s -> shader-resource-non-pixel (restored)\n",
               ResourceStateName(before).c_str());
    } else {
      snprintf(line, sizeof(line), "  depth read state : readable as-is (%s)\n",
               ResourceStateName(before).c_str());
    }
    out += line;
  }
  {
    char line[160];
    snprintf(line, sizeof(line), "  depth transitions: %llu\n",
             static_cast<unsigned long long>(stat_depth_transitions.load()));
    out += line;
  }
  // Jitter: what the projection read produced, and why it did or did not produce
  // anything. The value DLSS was handed is on the line above.
  {
    char line[320];
  // The last few adopted values: one paste shows whether both components of the
  // pair move over time, or whether only one of them does.
  {
    char line[288];
    snprintf(line, sizeof(line),
             "  jitter last      : %.4f,%.4f  %.4f,%.4f  %.4f,%.4f  %.4f,%.4f\n",
             static_cast<double>(jitter_history[0][0]), static_cast<double>(jitter_history[0][1]),
             static_cast<double>(jitter_history[1][0]), static_cast<double>(jitter_history[1][1]),
             static_cast<double>(jitter_history[2][0]), static_cast<double>(jitter_history[2][1]),
             static_cast<double>(jitter_history[3][0]), static_cast<double>(jitter_history[3][1]));
    out += line;
  }
    const auto& js = projection_jitter;
    if (dlss_jitter_source <= 0.5f) {
      snprintf(line, sizeof(line), "  jitter source    : zero (manual)\n");
    } else if (dlss_jitter_source >= 1.5f) {
      snprintf(line, sizeof(line),
               "  jitter source    : synthetic test ramp (px=(%.2f, %.2f))\n",
               static_cast<double>(js.pixels_x), static_cast<double>(js.pixels_y));
    } else if (scene_jitter.valid) {
      snprintf(line, sizeof(line),
               "  jitter source    : scene view constants (%s cb%u+0x%X)\n",
               scene_jitter.from_projection ? "projection" : "pair",
               scene_jitter.cb, scene_jitter.offset);
    } else if (!js.valid) {
      snprintf(line, sizeof(line),
               "  jitter source    : shared sub-pixel pair, none in hand (%llu adopted, %llu lost)\n",
               static_cast<unsigned long long>(js.adoptions), static_cast<unsigned long long>(js.losses));
    } else {
      snprintf(line, sizeof(line),
               "  jitter source    : pair at %llu+%u%s, raw=(%.6f, %.6f) -> px=(%.4f, %.4f), scans=%llu\n",
               static_cast<unsigned long long>(js.offset), js.size, js.transposed ? " (transposed)" : "",
               static_cast<double>(js.ndc_x), static_cast<double>(js.ndc_y),
               static_cast<double>(js.pixels_x), static_cast<double>(js.pixels_y),
               static_cast<unsigned long long>(js.probes));
    }
    out += line;
  }
  {
    char line[256];
    const auto& js = projection_jitter;
    snprintf(line, sizeof(line), "  projection base  : [0][0]=%.6f [1][1]=%.6f [2][2]=%.6f [2][3]=%.6f\n",
             static_cast<double>(js.base_xx), static_cast<double>(js.base_yy),
             static_cast<double>(js.base_zz), static_cast<double>(js.base_zw));
    out += line;
  }
  // Where the scene's own view constants were read. This is the source the
  // extraction uses, and the movement counters are the evidence that the value
  // found moves — a jitter changes every frame and a constant never does.
  {
    char line[512];
    if (!scene_jitter.valid) {
      snprintf(line, sizeof(line),
               "  scene jitter     : none read (probes left=%u, latch=%s)\n",
               scene_probe_attempts_left,
               scene_latch_cb == UINT32_MAX ? "none" : "held");
    } else {
      uint64_t moved = scene_jitter.moved;
      uint64_t reads = scene_jitter.reads;
      if (!scene_jitter.from_projection) {
        for (const ScenePair& pair : scene_pairs) {
          if (!pair.used || pair.cb != scene_jitter.cb || pair.offset != scene_jitter.offset) continue;
          moved = pair.moved;
          reads = pair.reads;
          break;
        }
      }
      snprintf(line, sizeof(line),
               "  scene jitter     : hash=0x%08X %s cb%u+0x%X %s raw=(%.6f, %.6f) -> px=(%.4f, %.4f) moved=%llu/%llu%s\n",
               scene_jitter.shader,
               scene_jitter.stage == 0u ? "vs" : "ps",
               scene_jitter.cb, scene_jitter.offset,
               scene_jitter.from_projection
                   ? (scene_jitter.order != 0u ? "proj(transposed)" : "proj")
                   : "pair",
               static_cast<double>(scene_jitter.raw_a), static_cast<double>(scene_jitter.raw_b),
               static_cast<double>(scene_jitter.pixels_x), static_cast<double>(scene_jitter.pixels_y),
               static_cast<unsigned long long>(moved),
               static_cast<unsigned long long>(reads),
               scene_jitter.updated_frame == frame_counter ? "" : "  STALE");
    }
    out += line;
    std::string pairs;
    for (const ScenePair& pair : scene_pairs) {
      if (!pair.used || pair.reads == 0u) continue;
      if (pairs.size() > 260u) break;  // the report is not a dump: six is plenty
      char part[96];
      snprintf(part, sizeof(part), "%scb%u+0x%X=(%.5f,%.5f) moved=%u/%u",
               pairs.empty() ? "" : "  ", pair.cb, pair.offset,
               static_cast<double>(pair.a), static_cast<double>(pair.b),
               pair.moved, pair.reads);
      pairs += part;
    }
    out += "  scene pairs      : ";
    out += pairs.empty() ? "none" : pairs;
    out += "\n";
  }
  // NGX's own words, oldest first. It is the component we cannot see into, and it
  // is the one that knows the runtime version and whether it disliked anything.
  {
    const std::lock_guard<std::mutex> lock(ngx_log.mutex);
    char header[160];
    snprintf(header, sizeof(header), "  ngx log          : %llu lines, %llu repeats dropped, %u kept\n",
             static_cast<unsigned long long>(ngx_log.seen.load()),
             static_cast<unsigned long long>(ngx_log.repeats.load()),
             ngx_log.kept);
    out += header;
    const uint32_t size = static_cast<uint32_t>(ngx_log.lines.size());
    const uint32_t first = ngx_log.kept < size ? 0u : ngx_log.next;
    for (uint32_t i = 0u; i < ngx_log.kept; ++i) {
      out += "    ";
      out += ngx_log.lines[(first + i) % size];
      out += "\n";
    }
  }
  // Intermittent rejections, per reason: a later success overwrites the single
  // state code, so without these a half-deploying session reads as "ok".
  {
    std::string failures;
    for (uint32_t i = 1u; i < kFailCodeCount; ++i) {
      const uint64_t count = stat_fail_counts[i].load();
      if (count == 0u) continue;
      char part[64];
      snprintf(part, sizeof(part), "%s%s=%llu", failures.empty() ? "" : " ",
               FailCodeText(static_cast<int>(i)), static_cast<unsigned long long>(count));
      failures += part;
    }
    out += "  failures         : ";
    out += failures.empty() ? "none" : failures;
    out += "\n";
  }
  // A collision means two records produced the same key bytes while resolving to
  // different resources. Their bytes are equal by definition, so what has to be
  // reported is what differs.
  const FingerprintCollision collision = GetFingerprintCollision();
  if (collision.captured) {
    char line[256];
    snprintf(line, sizeof(line),
             "  collision        : %u-byte key, resources 0x%llX vs 0x%llX%s\n",
             collision.byte_count,
             static_cast<unsigned long long>(collision.first_resource),
             static_cast<unsigned long long>(collision.second_resource),
             collision.same_resource ? " (same resource, different view)" : " (different resources)");
    out += line;
  }
  return out;
}

// ── Event handlers ──
inline void OnInitCommandList(reshade::api::command_list* cmd_list) { cmd_list->create_private_data<CommandListData>(); }
inline void OnDestroyCommandList(reshade::api::command_list* cmd_list) { cmd_list->destroy_private_data<CommandListData>(); }

// Any pipeline bind may change the pixel shader: mark the memoized hash dirty.
// Never creates private data.
inline void OnBindPipeline(
    reshade::api::command_list* cmd_list, reshade::api::pipeline_stage, reshade::api::pipeline) {
  if (cmd_list == nullptr) return;
  if (auto* data = cmd_list->get_private_data<CommandListData>()) {
    data->ps_dirty = true;
    data->scene_hash_valid = false;
  }
}

inline void OnBindDescriptorTables(
    reshade::api::command_list* cmd_list, reshade::api::shader_stage stages,
    reshade::api::pipeline_layout, uint32_t first, uint32_t count,
    const reshade::api::descriptor_table* tables) {
  if (dlss_enabled == 0.f || !any_nvidia.load()) return;
  // Tables are recorded for every stage, not just the pixel stage: the compute
  // dispatch that upscales binds its output through one, and a table left
  // unrecorded is indistinguishable from a table that resolved to nothing.
  (void)stages;
  if (cmd_list == nullptr || tables == nullptr) return;
  auto* data = Get(cmd_list);
  if (data == nullptr) return;
  for (uint32_t i = 0u; i < count && first + i < kTrackedTableCount; ++i) {
    data->descriptor_tables[first + i] = tables[i];
  }
}

inline void OnBindRenderTargetsAndDepthStencil(
    reshade::api::command_list* cmd_list, uint32_t count,
    const reshade::api::resource_view* rtvs, reshade::api::resource_view dsv) {
  if (dlss_enabled == 0.f || !any_nvidia.load()) return;
  if (cmd_list == nullptr) return;
  auto* data = Get(cmd_list);
  if (data == nullptr) return;
  // Attribute the draws issued since the last bind, then switch to the new depth.
  auto* device = cmd_list->get_device();
  FlushDepthDraws(device, data);
  data->dsv = (device != nullptr && dsv.handle != 0u) ? device->get_resource_from_view(dsv) : reshade::api::resource{};
  data->dsv_draws = 0u;
  data->rtvs = {};
  for (uint32_t i = 0u; i < count && i < kTrackedRtvCount; ++i) {
    data->rtvs[i] = rtvs != nullptr ? rtvs[i] : reshade::api::resource_view{};
  }
}

// Draws are attributed to the bound depth-stencil here, because that is the only
// place the count can be kept without a lock: the flush happens at the next bind.
inline void NoteDrawForDepth(reshade::api::command_list* cmd_list) {
  if (cmd_list == nullptr) return;
  auto* data = cmd_list->get_private_data<CommandListData>();
  if (data != nullptr && data->dsv.handle != 0u) ++data->dsv_draws;
}

inline bool OnDraw(reshade::api::command_list* cmd_list, uint32_t, uint32_t, uint32_t, uint32_t) {
  if (dlss_enabled == 0.f || !any_nvidia.load()) return false;
  NoteDrawForDepth(cmd_list);
  CommandListData* data = nullptr;
  uint32_t hash = 0u;
  if (!CheckTaaDraw(cmd_list, data, hash)) {
    // The scene's own view constants are read at its geometry draws, which is where
    // the jitter is applied and therefore where it exists as data.
    ProbeSceneDraw(cmd_list);
    return false;
  }
  ++stat_taa_draws;
  if (!logged_taa_draw_detected) {
    logged_taa_draw_detected = true;
    std::stringstream s;
    s << "Far Cry 6 DLSS: detected TAA draw 0x" << std::hex << hash;
    reshade::log::message(reshade::log::level::info, s.str().c_str());
  }
  // The game's TAA is left to run - DLSS no longer replaces it, only the game's
  // upscaler. The evaluation happens at this draw because this is where the
  // scene's colour, depth and motion vectors are bound; the draw itself is not
  // taken out, so the game's pipeline is complete whether or not DLSS deploys.
  EvaluateDLSS(cmd_list, data, hash);
  return false;
}

inline bool OnDrawIndexed(reshade::api::command_list* cmd_list, uint32_t, uint32_t, uint32_t, int32_t, uint32_t) {
  if (dlss_enabled == 0.f || !any_nvidia.load()) return false;
  NoteDrawForDepth(cmd_list);
  CommandListData* data = nullptr;
  uint32_t hash = 0u;
  if (!CheckTaaDraw(cmd_list, data, hash)) {
    // The scene's own view constants are read at its geometry draws, which is where
    // the jitter is applied and therefore where it exists as data.
    ProbeSceneDraw(cmd_list);
    return false;
  }
  ++stat_taa_draws;
  if (!logged_taa_draw_detected) {
    logged_taa_draw_detected = true;
    std::stringstream s;
    s << "Far Cry 6 DLSS: detected TAA draw 0x" << std::hex << hash;
    reshade::log::message(reshade::log::level::info, s.str().c_str());
  }
  // The game's TAA is left to run - DLSS no longer replaces it, only the game's
  // upscaler. The evaluation happens at this draw because this is where the
  // scene's colour, depth and motion vectors are bound; the draw itself is not
  // taken out, so the game's pipeline is complete whether or not DLSS deploys.
  EvaluateDLSS(cmd_list, data, hash);
  return false;
}

inline void OnPresent(
    reshade::api::command_queue* queue, reshade::api::swapchain*,
    const reshade::api::rect*, const reshade::api::rect*,
    uint32_t, const reshade::api::rect*) {
  ++stat_presents;
  if (queue != nullptr) SelectSceneDepth(queue->get_device());
  // Whether the game's own upscaler runs is decided by what the last frame showed:
  // with it, DLSS deploys at the upscaler; without it, at the TAA draw.
  fsr1_active.store(fsr1_seen_this_frame);
  fsr1_seen_this_frame = false;
  fsr1_wrote_this_frame.store(false);
  projection_jitter.matched_this_frame = false;
  // The scene probe budget belongs to the frame, like the write probes'.
  scene_probe_attempts_left = kSceneProbeAttemptsPerFrame;
  scene_attempts_made = 0u;
  scene_projected_this_frame = false;
  ++frame_counter;
  // The probe budget belongs to the frame, not to the evaluation: resetting it at the
  // TAA draw meant every batch was spent on constant buffers written after that draw,
  // and the camera's own buffer - written in the frame setup, before it - was never
  // inside one. From here the budget is spent on the frame's first writes, which is
  // where per-view data lives.
  projection_probes_left = kProjectionProbesPerFrame;
}

inline void OnInitDevice(reshade::api::device* device) {
  if (device == nullptr) return;
  int vendor_id = 0;
  const bool is_nvidia = device->get_property(reshade::api::device_properties::vendor_id, &vendor_id)
                      && vendor_id == 0x10de;
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
  if (device == nullptr) return;
  bool was_ours = false;
  {
    std::lock_guard<std::mutex> lock(device_mutex);
    was_ours = nvidia_devices.erase(device->get_native()) > 0u;
    any_nvidia.store(!nvidia_devices.empty());
    if (nvidia_devices.empty()) device_summary = "none seen";
  }
  if (was_ours) {
    ReleaseNgx();
    ReleaseUnpack(unpack_device);
    unpack_device = nullptr;
    ReleasePlaceholders(device);
    ReleaseViewCache(device);
  }
  RemoveNvidiaNative(device->get_native());
  reshade::log::message(reshade::log::level::info, "Far Cry 6 DLSS: device destroyed");
}

inline void Use(DWORD fdw_reason) {
  // utils::shader attaches the pipeline/state tracking this addon reads
  // (GetCurrentState, GetCurrentPixelShaderHash, PopulateStageState) and pulls
  // utils::pipeline_layout with it.
  //
  // Descriptor writes are recorded (update_descriptor_tables). Descriptor
  // *copies* are deliberately not subscribed: the engine issues ~117M of them a
  // session and ReShade builds the copy array before the event reaches us, which
  // is where the earlier per-frame cost came from.
  renodx::utils::shader::Use(fdw_reason);
  renodx::utils::pipeline_layout::Use(fdw_reason);

  switch (fdw_reason) {
    case DLL_PROCESS_ATTACH:
      if (attached) return;
      attached = true;
      reshade::register_event<reshade::addon_event::init_device>(OnInitDevice);
      reshade::register_event<reshade::addon_event::destroy_device>(OnDestroyDevice);
      reshade::register_event<reshade::addon_event::update_descriptor_tables>(OnUpdateDescriptorTables);
      reshade::register_event<reshade::addon_event::destroy_resource>(OnDestroyResource);
      reshade::register_event<reshade::addon_event::init_command_list>(OnInitCommandList);
      reshade::register_event<reshade::addon_event::destroy_command_list>(OnDestroyCommandList);
      reshade::register_event<reshade::addon_event::bind_pipeline>(OnBindPipeline);
      reshade::register_event<reshade::addon_event::bind_descriptor_tables>(OnBindDescriptorTables);
      reshade::register_event<reshade::addon_event::barrier>(OnBarrier);
      reshade::register_event<reshade::addon_event::bind_render_targets_and_depth_stencil>(OnBindRenderTargetsAndDepthStencil);
      reshade::register_event<reshade::addon_event::draw>(OnDraw);
      reshade::register_event<reshade::addon_event::draw_indexed>(OnDrawIndexed);
      reshade::register_event<reshade::addon_event::dispatch>(OnDispatch);
      reshade::register_event<reshade::addon_event::init_resource>(OnInitResource);
      reshade::register_event<reshade::addon_event::present>(OnPresent);
      break;
    case DLL_PROCESS_DETACH:
      if (!attached) return;
      attached = false;
      reshade::unregister_event<reshade::addon_event::present>(OnPresent);
      reshade::unregister_event<reshade::addon_event::init_resource>(OnInitResource);
      reshade::unregister_event<reshade::addon_event::dispatch>(OnDispatch);
      reshade::unregister_event<reshade::addon_event::draw_indexed>(OnDrawIndexed);
      reshade::unregister_event<reshade::addon_event::draw>(OnDraw);
      reshade::unregister_event<reshade::addon_event::bind_render_targets_and_depth_stencil>(OnBindRenderTargetsAndDepthStencil);
      reshade::unregister_event<reshade::addon_event::barrier>(OnBarrier);
      reshade::unregister_event<reshade::addon_event::bind_descriptor_tables>(OnBindDescriptorTables);
      reshade::unregister_event<reshade::addon_event::bind_pipeline>(OnBindPipeline);
      reshade::unregister_event<reshade::addon_event::destroy_command_list>(OnDestroyCommandList);
      reshade::unregister_event<reshade::addon_event::init_command_list>(OnInitCommandList);
      reshade::unregister_event<reshade::addon_event::destroy_resource>(OnDestroyResource);
      reshade::unregister_event<reshade::addon_event::update_descriptor_tables>(OnUpdateDescriptorTables);
      reshade::unregister_event<reshade::addon_event::destroy_device>(OnDestroyDevice);
      reshade::unregister_event<reshade::addon_event::init_device>(OnInitDevice);
      {
        reshade::api::device* device = unpack_device != nullptr ? unpack_device : placeholder_device;
        unpack_device = nullptr;
        ReleaseUnpack(device);
        ReleasePlaceholders(device);
        ReleaseViewCache(device);
      }
      ReleaseNgx();
      break;
  }
}

}  // namespace farcry6_dlss::dlss
