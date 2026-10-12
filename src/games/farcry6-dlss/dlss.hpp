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
// Calibration multiplier for the decoded motion vectors.
inline float dlss_mv_scale = 1.f;
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

// A destroyed resource takes every fingerprint that named it with it. Without
// this, a renderer rebuild leaves fingerprints pointing at freed resources and
// NGX reads them.
inline void OnDestroyResource(reshade::api::device* device, reshade::api::resource resource) {
  if (device == nullptr || resource.handle == 0u) return;
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
  // Auto: native-size inputs run DLAA, smaller inputs run Quality upscaling.
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

  const float pc[8] = {static_cast<float>(width), static_cast<float>(height), dlss_mv_scale, 0.f,
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
  const uint32_t target_width = input_width;
  const uint32_t target_height = input_height;
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

  // NGX output format follows the TAA target. Anything else keeps vanilla TAA.
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

    if (!CopyThroughTaaTarget(cmd_list, command_list, output_target, ngx.output_texture.Get(),
                              reshade::api::resource_usage::unordered_access)) {
      NoteFail(kFailPlaceholder);
      return false;
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

  // Jitter stays zero: the game's own jitter lives in its scene passes' constant
  // buffers, and nothing identifies it from here yet, so passing zero is the only
  // honest option. Inventing a value from a heuristic is what broke the image in
  // earlier builds.
  last_render_width.store(input_width);
  last_render_height.store(input_height);
  const float jitter_x = 0.f;
  const float jitter_y = 0.f;

  NVSDK_NGX_D3D12_DLSS_Eval_Params eval = {};
  eval.Feature.pInColor = color;
  eval.Feature.pInOutput = ngx.output_texture.Get();
  eval.pInDepth = depth;
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
               "Far Cry 6 DLSS: eval hash=0x%08X in=%ux%u target=%ux%u quality=%s preset=%s"
               " jit=(%.3f,%.3f) mv_src=%ux%u flags=0x%X reset=%d",
               hash, input_width, input_height, target_width, target_height,
               GetPerfQualityName(ngx.perf_quality), GetRenderPresetName(ngx.render_preset),
               jitter_x, jitter_y, motion_src_width, motion_src_height, feature_flags, eval.InReset);
      reshade::log::message(reshade::log::level::info, buf);
    }
  }

  const NVSDK_NGX_Result result = NGX_D3D12_EVALUATE_DLSS_EXT(command_list, ngx.feature, ngx.runtime_parameters, &eval);
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
    ++deploy_report.evals;
    if (eval.InReset != 0) ++deploy_report.resets;
  }

  // Evaluate-only mode: the whole chain ran, but the game's own TAA draw is
  // allowed to produce the image. Isolates our GPU work from the substitution.
  if (dlss_output_mode >= 0.5f) {
    return false;
  }

  // DLSS wrote the target; move it into the TAA output so the game keeps
  // running its own pipeline on top. The bypass above uses the same call.
  CopyThroughTaaTarget(cmd_list, command_list, output_target, ngx.output_texture.Get(),
                       reshade::api::resource_usage::unordered_access);

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
      "  quality / preset : %s / %s   flags=0x%X\n"
      "  evals/resets/bypass : %llu / %llu / %llu\n"
      "  jitter ngx px    : (%.4f, %.4f)   game value not wired, zero is passed\n"
      "  mv scale         : %.3f   mv jittered=%s   depth inverted=%s\n"
      "  hdr / restore    : %s / %s\n",
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
      static_cast<unsigned>(report.flags),
      static_cast<unsigned long long>(report.evals),
      static_cast<unsigned long long>(report.resets),
      static_cast<unsigned long long>(report.bypass),
      static_cast<double>(report.jitter_x), static_cast<double>(report.jitter_y),
      static_cast<double>(dlss_mv_scale),
      dlss_motion_vectors_jittered != 0.f ? "on" : "off",
      dlss_depth_inverted != 0.f ? "on" : "off",
      dlss_flag_is_hdr != 0.f ? "on" : "off",
      dlss_restore_state != 0.f ? "on" : "off");
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
  // Jitter: reported as what is actually passed, never as a guess. The game's own
  // value is not read yet, so this says so rather than implying it was found.
  {
    char line[256];
    snprintf(line, sizeof(line),
             "  jitter           : zero (game's own value not wired yet)\n");
    out += line;
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
  if (auto* data = cmd_list->get_private_data<CommandListData>()) data->ps_dirty = true;
}

inline void OnBindDescriptorTables(
    reshade::api::command_list* cmd_list, reshade::api::shader_stage stages,
    reshade::api::pipeline_layout, uint32_t first, uint32_t count,
    const reshade::api::descriptor_table* tables) {
  if (dlss_enabled == 0.f || !any_nvidia.load()) return;
  if (!renodx::utils::bitwise::HasFlag(stages, reshade::api::shader_stage::pixel)) return;
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
  NoteDrawForDepth(cmd_list);
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

inline void OnPresent(
    reshade::api::command_queue* queue, reshade::api::swapchain*,
    const reshade::api::rect*, const reshade::api::rect*,
    uint32_t, const reshade::api::rect*) {
  ++stat_presents;
  if (queue != nullptr) SelectSceneDepth(queue->get_device());
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
      reshade::register_event<reshade::addon_event::bind_render_targets_and_depth_stencil>(OnBindRenderTargetsAndDepthStencil);
      reshade::register_event<reshade::addon_event::draw>(OnDraw);
      reshade::register_event<reshade::addon_event::draw_indexed>(OnDrawIndexed);
      reshade::register_event<reshade::addon_event::present>(OnPresent);
      break;
    case DLL_PROCESS_DETACH:
      if (!attached) return;
      attached = false;
      reshade::unregister_event<reshade::addon_event::present>(OnPresent);
      reshade::unregister_event<reshade::addon_event::draw_indexed>(OnDrawIndexed);
      reshade::unregister_event<reshade::addon_event::draw>(OnDraw);
      reshade::unregister_event<reshade::addon_event::bind_render_targets_and_depth_stencil>(OnBindRenderTargetsAndDepthStencil);
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
