/*
 * Copyright (C) 2026
 * SPDX-License-Identifier: MIT
 */
#pragma once

#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <mutex>
#include <shared_mutex>
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

#include "../../../utils/descriptor.hpp"
#include "../../../utils/gpu_fence.hpp"
#include "../../../utils/minhook.hpp"
#include "../../../utils/pipeline_layout.hpp"
#include "../../../utils/shader.hpp"
#include "../../../utils/state.hpp"
#include "../shared.h"
#include "./streamline_runtime.hpp"
#include "./fg_camera.hpp"
#include "./fg_factory.hpp"
#include "./fg_mode.hpp"

namespace farcry6_dlss::dlss {

static constexpr uint32_t kTaaMainHash = 0xFF0AF25Bu;
static constexpr uint32_t kTaaPauseHash = 0x4AD8172Au;
static constexpr uint32_t kSceneUpscaleHash = 0x944489F5u;
static constexpr uint32_t kHdrOutputHash = 0x62A8D0C3u;
static constexpr uint32_t kScRgbOutputHash = 0x6439E80Au;
static constexpr uint32_t kSdrOutputHash = 0x6A24E345u;

static constexpr uint32_t kTrackedTableCount = 32u;
static constexpr uint32_t kTrackedRtvCount = 4u;

// TAA slots consumed by DLSS (space0).
static constexpr uint32_t kSlotPackedMv = 0u;  // t0: packed motion vectors
static constexpr uint32_t kSlotColor = 2u;     // t2: current scene color
static constexpr uint32_t kSlotCbv = 0u;       // b0: TAA knobs

inline float dlss_enabled = 1.f;
inline float dlss_rcas_strength = 0.5f;
inline float dlss_render_preset = 3.f;
inline const ShaderInjectData* output_settings = nullptr;
inline float fg_multiplier_setting = 0.f;  // 0=off, 1..5=generated frames
inline std::atomic<bool> fg_enable_requested{false};
inline std::atomic<uint32_t> fg_requested_generated{0u};
inline std::atomic<uint32_t> fg_applied_generated{0u};
inline uint32_t fg_pending_generated = 0u;
inline std::atomic<bool> fg_swapchain_transition{false};
inline std::atomic<int> fg_mode_status{0};
// Accessed under frame_api_mutex on the presenting thread.
inline farcry6_dlss::frame_generation::ModeState fg_mode;
inline reshade::api::swapchain* fg_mode_swapchain = nullptr;
inline bool fg_mode_present_outstanding = false;
inline bool fg_pending_retain_resources = false;
inline std::atomic<bool> fg_resources_retained{false};
inline std::atomic<uint64_t> fg_temporary_off_time_ms{0u};
inline uint64_t fg_last_missing_hudless_ms = 0u;
inline uint32_t fg_missing_hudless_restarts = 0u;

inline bool FgInputsEnabled() {
  return fg_enable_requested.load(std::memory_order_acquire)
      || fg_mode_status.load(std::memory_order_acquire) != 0;
}

inline bool SetFgMode(farcry6_dlss::frame_generation::ModeState::Mode mode, uint32_t generated_frames = 0u) {
  auto* set_options = farcry6_dlss::streamline::fg_set_options;
  if (set_options == nullptr) return false;
  sl::DLSSGOptions options = {};
  options.mode = (mode == farcry6_dlss::frame_generation::ModeState::Mode::on ? sl::DLSSGMode::eOn : sl::DLSSGMode::eOff);
  if (mode == farcry6_dlss::frame_generation::ModeState::Mode::on) {
    if (generated_frames == 0u) return false;
    options.numFramesToGenerate = generated_frames;
  }
  const bool retain_resources = mode == farcry6_dlss::frame_generation::ModeState::Mode::off
                             && fg_enable_requested.load(std::memory_order_acquire)
                             && FgInputsEnabled() && !fg_swapchain_transition.load(std::memory_order_acquire)
                             && farcry6_dlss::streamline::runtime.free_resources != nullptr;
  if (retain_resources) options.flags = sl::DLSSGFlags::eRetainResourcesWhenOff;
  const auto result = set_options(sl::ViewportHandle(0u), options);
  if (result != sl::Result::eOk) return false;
  fg_pending_retain_resources = retain_resources;
  fg_pending_generated = (mode == farcry6_dlss::frame_generation::ModeState::Mode::on ? generated_frames : 0u);
  if (mode == farcry6_dlss::frame_generation::ModeState::Mode::off) {
    fg_temporary_off_time_ms.store(retain_resources ? GetTickCount64() : 0u, std::memory_order_release);
  }
  fg_mode.OptionsAccepted(mode);
  fg_mode_status.store(mode == farcry6_dlss::frame_generation::ModeState::Mode::on ? 1 : 3, std::memory_order_relaxed);
  return true;
}
inline std::atomic<uint64_t> pause_taa_last_frame{UINT64_MAX};
// Per-slot resolve result of the last TAA draw: 0=miss, 1=real, read by the UI.

// ── Device tracking ──
// The engine exposes more than one device (a WARP adapter alongside the real
// GPU), so support is per-device and sticky: a non-NVIDIA init must never
// disable an NVIDIA one.
inline std::mutex device_mutex;
inline std::unordered_set<uint64_t> gpu_devices;
inline std::atomic<bool> any_gpu{false};
inline bool attached = false;

// Lock-free copy of the NVIDIA natives, for the draw path.
inline std::atomic<uint64_t> gpu_devs[4];
inline std::atomic<uint64_t> nvidia_devs[4];

inline bool IsKnownGpuNative(uint64_t native) {
  if (native == 0u) return false;
  for (uint32_t i = 0u; i < 4u; ++i) {
    if (gpu_devs[i].load() == native) return true;
  }
  return false;
}

inline void AddGpuNative(uint64_t native) {
  if (native == 0u || IsKnownGpuNative(native)) return;
  for (uint32_t i = 0u; i < 4u; ++i) {
    uint64_t empty = 0u;
    if (gpu_devs[i].compare_exchange_strong(empty, native)) return;
  }
}

inline void RemoveGpuNative(uint64_t native) {
  if (native == 0u) return;
  for (uint32_t i = 0u; i < 4u; ++i) {
    if (gpu_devs[i].load() == native) gpu_devs[i].store(0u);
    if (nvidia_devs[i].load() == native) nvidia_devs[i].store(0u);
  }
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

inline std::mutex fingerprint_mutex;
inline std::unordered_map<DescriptorFingerprint, RecordedDescriptor, DescriptorFingerprintHash> fingerprints;
// Reverse index: which fingerprints name a given resource. Needed because a
// stored resource handle outlives its resource — a renderer rebuild (a
// fullscreen transition does this) destroys render targets, and a fingerprint
// left pointing at one would hand NGX a freed resource.
inline std::unordered_multimap<uint64_t, DescriptorFingerprint> fingerprints_by_resource;
inline std::atomic<uint32_t> descriptor_stride_bytes{0};

inline void RecordFingerprint(const DescriptorFingerprint& key, const RecordedDescriptor& record) {
  const std::lock_guard<std::mutex> lock(fingerprint_mutex);
  if (fingerprints.emplace(key, record).second && record.has_resource) {
    fingerprints_by_resource.emplace(record.resource.handle, key);
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

struct DepthCandidate {
  uint32_t draws = 0u;
  uint32_t width = 0u;
  uint32_t height = 0u;
};
inline std::mutex depth_mutex;
inline std::unordered_map<uint64_t, DepthCandidate> depth_candidates;
inline uint64_t scene_depth_resource = 0u;
inline uint32_t scene_depth_width = 0u;
inline uint32_t scene_depth_height = 0u;
inline reshade::api::format scene_depth_format = reshade::api::format::unknown;
static constexpr uint32_t kViewCacheSize = 8u;
inline std::mutex view_cache_mutex;
inline reshade::api::resource_view view_cache[kViewCacheSize] = {};
inline reshade::api::resource view_cache_resource[kViewCacheSize] = {};
inline reshade::api::format view_cache_format[kViewCacheSize] = {};
inline std::atomic<uint64_t> game_upscale_output_resource{0u};

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
  {
    const std::lock_guard<std::mutex> lock(depth_mutex);
    depth_candidates.erase(resource.handle);
    if (scene_depth_resource == resource.handle) {
      scene_depth_resource = 0u;
      scene_depth_width = 0u;
      scene_depth_height = 0u;
      scene_depth_format = reshade::api::format::unknown;
    }
  }
  {
    const std::lock_guard<std::mutex> lock(view_cache_mutex);
    for (uint32_t i = 0u; i < kViewCacheSize; ++i) {
      if (view_cache_resource[i].handle != resource.handle) continue;
      if (view_cache[i].handle != 0u) device->destroy_resource_view(view_cache[i]);
      view_cache[i] = {};
      view_cache_resource[i] = {};
      view_cache_format[i] = reshade::api::format::unknown;
    }
  }
  uint64_t expected_game_upscale = resource.handle;
  game_upscale_output_resource.compare_exchange_strong(expected_game_upscale, 0u);
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

inline bool OnUpdateDescriptorTables(
    reshade::api::device* device,
    uint32_t count,
    const reshade::api::descriptor_table_update* updates) {

  if (dlss_enabled == 0.f || !any_gpu.load()) return false;
  if (device == nullptr || count == 0u || updates == nullptr) return false;
  if (device->get_api() != reshade::api::device_api::d3d12) return false;

  for (uint32_t u = 0u; u < count; ++u) {
    const auto& update = updates[u];
    if (update.table.handle == 0u || update.count == 0u) continue;
    const bool is_cbv = update.type == reshade::api::descriptor_type::constant_buffer;
    // The TAA and upscaler resources resolved by this addon are textures;
    // buffer views cannot supply those inputs or outputs.
    const bool is_view = update.type == reshade::api::descriptor_type::texture_shader_resource_view
        || update.type == reshade::api::descriptor_type::texture_unordered_access_view;

    if (!is_cbv && !is_view) continue;

    for (uint32_t k = 0u; k < update.count; ++k) {
      RecordedDescriptor record;
      record.type = update.type;
      if (is_cbv) {
        record.range = static_cast<const reshade::api::buffer_range*>(update.descriptors)[k];
        record.has_range = record.range.buffer.handle != 0u;
        if (!record.has_range) continue;
        // The projection scan is only used as a fallback for the scene jitter source.

      } else {
        const reshade::api::resource_view view =
            static_cast<const reshade::api::resource_view*>(update.descriptors)[k];
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
    uint32_t max_out,
    reshade::api::shader_stage stage = reshade::api::shader_stage::all) {
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
      if ((static_cast<uint32_t>(range.visibility) & static_cast<uint32_t>(stage)) == 0u) continue;
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
  reshade::api::resource coverage_pass_dsv = {};
  // Depth attribution: draws are counted against whichever depth-stencil was
  // bound when they were issued, and flushed into the shared map at every bind.
  reshade::api::resource dsv = {};
  bool in_render_pass = false;
  uint32_t dsv_draws = 0u;
  // Memoized pixel-shader hash: refreshed per pipeline bind instead of per draw.
  uint32_t cached_ps_hash = 0u;
  bool ps_hash_valid = false;
  bool ps_dirty = true;
  uint64_t dlss_taa_resource = 0u;
  uint64_t dlss_frame = UINT64_MAX;
  bool gpu_tracked = false;
};

inline CommandListData* Get(reshade::api::command_list* cmd_list) {
  if (cmd_list == nullptr) return nullptr;
  auto* data = cmd_list->get_private_data<CommandListData>();
  return data != nullptr ? data : cmd_list->create_private_data<CommandListData>();
}

// ReShade's execute callback runs before the native submission. A retirement
// fence is therefore only signalled later on the submitting thread at Present,
// after every recorded list which could reference the resource was reset.
struct GpuRetirementTicket {
  std::unordered_set<reshade::api::command_list*> pending_lists;
  reshade::api::command_queue* queue = nullptr;
  DWORD submit_thread = 0u;
  uint64_t fence_value = 0u;
  bool conflicting_submission = false;
  bool complete = false;
};
struct GpuRetirementState {
  std::shared_mutex mutex;
  std::unordered_set<reshade::api::command_list*> recorded_lists;
  std::vector<std::weak_ptr<GpuRetirementTicket>> tickets;
  std::unique_ptr<renodx::utils::gpu_fence::Fence> fence;
  reshade::api::command_queue* queue = nullptr;
  DWORD submit_thread = 0u;
  bool conflicting_submission = false;
  std::atomic<uint64_t> reclaimed{0u};
  uint64_t fence_failures = 0u;
};
inline GpuRetirementState gpu_retirement;

inline void TrackGpuCommands(reshade::api::command_list* cmd_list) {
  auto* data = Get(cmd_list);
  if (data == nullptr || data->gpu_tracked) return;
  std::unique_lock lock(gpu_retirement.mutex);
  gpu_retirement.recorded_lists.insert(cmd_list);
  data->gpu_tracked = true;
}

inline std::shared_ptr<GpuRetirementTicket> RetireGpuCommands(reshade::api::command_list* command_list = nullptr) {
  auto ticket = std::make_shared<GpuRetirementTicket>();
  std::unique_lock lock(gpu_retirement.mutex);
  if (command_list != nullptr) {
    if (gpu_retirement.recorded_lists.contains(command_list)) {
      ticket->pending_lists.insert(command_list);
    } else {
      // Without the recording list we cannot prove when its commands finish.
      ticket->conflicting_submission = true;
    }
  } else {
    ticket->pending_lists = gpu_retirement.recorded_lists;
    ticket->queue = gpu_retirement.queue;
    ticket->submit_thread = gpu_retirement.submit_thread;
    ticket->conflicting_submission = gpu_retirement.conflicting_submission;
  }
  gpu_retirement.tickets.emplace_back(ticket);
  return ticket;
}

// ── View cache ──
// The unpack pass needs a ReShade view of the motion resource. It is created
// from the resource we resolved, never from a recorded handle, and reused:
// creating one per draw would leak a view every frame.
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
  if (view_cache[empty].handle != 0u) {
    device->destroy_resource_view(view_cache[empty]);
    view_cache[empty] = {};
    view_cache_resource[empty] = {};
    view_cache_format[empty] = reshade::api::format::unknown;
  }
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
  reshade::api::resource_desc chosen = {};
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
    // A handful of draws is a full-screen effect's depth, not the scene's.
    if (best == 0u || best_count < 100u) return;
    const reshade::api::resource_desc desc = device->get_resource_desc(reshade::api::resource{best});
    if (desc.type != reshade::api::resource_type::texture_2d) return;
    chosen = desc;
    scene_depth_resource = best;
    scene_depth_width = desc.texture.width;
    scene_depth_height = desc.texture.height;
    scene_depth_format = desc.texture.format;
  }

  // Its state has to be known to read it as a shader resource. Registering it
  // here, at the frame boundary, means the engine's transitions during the next
  // frame are recorded before NGX needs them.
  TrackDepthResource(best);
}

inline uint64_t frame_counter = 0u;

struct SceneMatrixSample {
  uint64_t frame = 0u;
  uint32_t shader = 0u;
  uint32_t layout_param = 0u;
  uint32_t binding = 0u;
  uint64_t buffer = 0u;
  uint64_t buffer_offset = 0u;
  uint64_t buffer_size = 0u;
  uint32_t jitter_frame = 0u;
  uint32_t offscreen = 0u;
  float viewport[2] = {};
  float motion_vector_offsets[4] = {};
  float inverse_projection[16] = {};
  float inverse_view[16] = {};
  float projection[16] = {};
  float view[16] = {};
  float view_projection[16] = {};
  float matrix[16] = {};
  float previous_matrix[16] = {};
  float camera_position[3] = {};
  float previous_camera_position[3] = {};
  float camera_right[3] = {};
  float camera_up[3] = {};
  float camera_direction[3] = {};
  float temporal[2] = {};
  bool temporal_valid = false;
  DescriptorFingerprint fingerprint = {};
  bool fingerprint_valid = false;
};
inline std::mutex fg_camera_sample_mutex;
inline std::array<SceneMatrixSample, 4> fg_camera_samples = {};
struct FgCameraEvaluation {
  uint64_t frame = UINT64_MAX;
  float jitter_x = 0.f;
  float jitter_y = 0.f;
  bool depth_inverted = false;
  bool motion_jittered = false;
  bool reset = false;
  bool halton = false;
  // Identity checks only: these values do not retain GPU resources or prove
  // their contents are still valid at present.
  uint64_t color = 0u;
  uint64_t motion = 0u;
  uint64_t depth = 0u;
  uint32_t render_width = 0u;
  uint32_t render_height = 0u;
  uint32_t output_width = 0u;
  uint32_t output_height = 0u;
  bool handoff = false;
  bool camera_valid = false;
  sl::Constants camera = {};
  sl::FrameToken* token = nullptr;
  reshade::api::command_list* input_list = nullptr;
  reshade::api::command_queue* submit_queue = nullptr;
  bool conflicting_queue = false;
  uint32_t hdr_slot = UINT32_MAX;
  float hdr_brightness = 0.f;
  float tone_map_type = 0.f;
  float diffuse_white_nits = 0.f;
  float gamma_correction = 0.f;
  reshade::api::color_space output_color_space = reshade::api::color_space::unknown;
  uint64_t scene_output = 0u;
  bool hudless_tagged = false;
};
inline std::array<FgCameraEvaluation, 4> fg_camera_evaluations = {};
inline std::atomic<uint64_t> fg_camera_sample_frame{UINT64_MAX};
inline std::atomic<bool> fg_camera_capture_enabled{false};
inline std::atomic<uint64_t> fg_last_scene_handoff_frame{UINT64_MAX};
inline std::atomic<uint64_t> last_taa_draw_frame{UINT64_MAX};
inline std::atomic<uint64_t> last_upscaler_eval_frame{UINT64_MAX};
inline std::atomic<uint32_t> fg_stable_gameplay_frames{0u};
inline std::atomic<uint64_t> fg_swapchain_transitions{0u};
inline std::atomic<reshade::api::color_space> fg_output_color_space{reshade::api::color_space::unknown};
inline std::atomic<uint32_t> fg_output_format{0u};
inline std::atomic<uint64_t> fg_fullscreen_transitions{0u};
inline std::atomic<uint64_t> fg_present_frame{UINT64_MAX};
inline std::atomic<uint64_t> fg_successful_present_frame{UINT64_MAX};
inline std::atomic<uint64_t> fg_camera_capture_busy{0u};
inline std::atomic<uint64_t> fg_camera_valid_evals{0u};
inline std::atomic<uint64_t> fg_camera_unavailable_evals{0u};
inline std::atomic<uint64_t> fg_camera_missing_frame{0u};
inline std::atomic<uint64_t> fg_camera_invalid_data{0u};
inline std::atomic<uint64_t> fg_camera_eval_frame{UINT64_MAX};
inline std::atomic<uint64_t> fg_camera_present_valid{0u};
inline std::atomic<uint64_t> fg_camera_present_missing{0u};
inline std::atomic<uint64_t> fg_camera_present_invalid{0u};
inline std::atomic<uint64_t> fg_camera_present_frame{UINT64_MAX};
inline std::atomic<uint64_t> fg_frame_matched{0u};
inline std::atomic<uint64_t> fg_frame_missing_handoff{0u};
inline std::atomic<uint64_t> fg_frame_missing_hdr{0u};
inline std::atomic<uint64_t> fg_input_attempt_frame{UINT64_MAX};
inline std::atomic<uint64_t> fg_input_tagged{0u};
inline std::atomic<reshade::api::command_list*> fg_input_recording_list{nullptr};
inline std::atomic<uint64_t> fg_input_failed{0u};
inline std::atomic<uint64_t> fg_input_submitted{0u};
inline std::atomic<uint64_t> fg_input_queue_skips{0u};
inline std::atomic<uint64_t> fg_present_started{0u};
inline std::atomic<uint64_t> fg_present_finished{0u};
inline std::atomic<uint64_t> fg_present_unpaired{0u};
inline std::atomic<int> fg_input_last_result{0};
inline std::atomic<uint32_t> fg_state_max_generated{0u};
inline std::atomic<uint32_t> fg_complete_input_streak{0u};
inline std::atomic<bool> fg_completed_input_on_gpu{false};
inline std::atomic<uint64_t> fg_complete_input_frames{0u};
inline std::atomic<uint64_t> fg_hudless_converted{0u};
inline std::atomic<uint64_t> fg_hudless_tagged{0u};
inline std::atomic<uint64_t> fg_hudless_busy{0u};
inline std::atomic<uint64_t> fg_hudless_no_slot{0u};
inline std::atomic<uint64_t> fg_hudless_rejected{0u};
struct FgPendingPresent {
  sl::FrameToken* token = nullptr;
  reshade::api::command_queue* queue = nullptr;
  reshade::api::swapchain* swapchain = nullptr;
  bool hudless_tagged = false;
  uint64_t frame = UINT64_MAX;
};
inline FgPendingPresent fg_pending_present;
inline std::atomic<bool> fg_has_pending_present{false};

using CameraBuilder = void (*)(void*, void*, void*, void*, void*, void*, void*, const float*, uint32_t);
using CameraParametersWriter = void (*)(void*, void*, void*);
using AlternateCameraBuilder = void (*)(void*, void*, void*, void*, bool);
struct CameraBuilderState {
  std::shared_mutex mutex;
  CameraBuilder original = nullptr;
  CameraParametersWriter offsets_original = nullptr;
  AlternateCameraBuilder alternate_original = nullptr;
  bool offsets_hook_installed = false;
  enum class State { idle, unavailable, unsupported, install_failed, active } state = State::idle;
  uint64_t halton_frame = UINT64_MAX;
  float halton_pixels[2] = {};
  struct HaltonSample {
    uint64_t frame = UINT64_MAX;
    uint32_t width = 0u;
    uint32_t height = 0u;
    float pixels[2] = {};
  };
  std::array<HaltonSample, 8> halton_history = {};
};
inline CameraBuilderState camera_builder;
inline std::atomic<bool> camera_builder_motion_offsets_enabled{false};
inline std::atomic<bool> camera_builder_halton_enabled{false};
inline std::atomic<uint64_t> camera_builder_frame{0u};

// The pause shader can appear after the gameplay TAA draw was recorded. Keep
// the following present interval on the game's path too, and then resume DLSS.
inline bool PauseTaaRecentlySeen() {
  const uint64_t pause_frame = pause_taa_last_frame.load(std::memory_order_acquire);
  const uint64_t frame = camera_builder_frame.load(std::memory_order_acquire);
  return pause_frame != UINT64_MAX && (pause_frame == frame || (frame != 0u && pause_frame == frame - 1u));
}

inline void HookCameraBuilder(void* provider, void* view, void* previous_view, void* parameters,
                              void* arg5, void* arg6, void* arg7, const float* offset, uint32_t jitter_frame) {
  const bool use_halton = camera_builder_halton_enabled.load(std::memory_order_acquire);
  if (!use_halton) {
    camera_builder.original(provider, view, previous_view, parameters, arg5, arg6, arg7, offset, jitter_frame);
    return;
  }
  const uint32_t width = last_render_width.load();
  const uint32_t height = last_render_height.load();
  const float x = (offset != nullptr ? offset[0] : 0.f);
  const float y = (offset != nullptr ? offset[1] : 0.f);
  const bool candidate = width != 0u && height != 0u && jitter_frame <= 1u && std::isfinite(x) && std::isfinite(y)
      && std::abs(std::abs(x) * static_cast<float>(width) - 0.5f) < 0.01f
      && std::abs(std::abs(y) * static_cast<float>(height) - 0.5f) < 0.01f;
  const bool halton = candidate && use_halton;
  const uint64_t frame = camera_builder_frame.load(std::memory_order_acquire);
  const uint32_t step = static_cast<uint32_t>(frame % 16u) + 1u;
  const float replacement[2] = {
      -2.f * (HaltonValue(step, 2u) - 0.5f) / static_cast<float>(width != 0u ? width : 1u),
      2.f * (HaltonValue(step, 3u) - 0.5f) / static_cast<float>(height != 0u ? height : 1u),
  };
  camera_builder.original(provider, view, previous_view, parameters, arg5, arg6, arg7,
                               halton ? replacement : offset, jitter_frame);
  float projection[16] = {};
  if (halton && provider != nullptr) {
    std::memcpy(projection, static_cast<const uint8_t*>(provider) + 0xa0u, sizeof(projection));
  }
  const bool verified = halton && projection[14] == -1.f && projection[15] == 0.f
      && std::isfinite(projection[2]) && std::isfinite(projection[6])
      && std::abs(projection[2] * static_cast<float>(width)) <= 1.01f
      && std::abs(projection[6] * static_cast<float>(height)) <= 1.01f;
  if (verified) {
    std::unique_lock lock(camera_builder.mutex);
    camera_builder.halton_frame = frame;
    camera_builder.halton_pixels[0] = -projection[2] * static_cast<float>(width) * 0.5f;
    camera_builder.halton_pixels[1] = projection[6] * static_cast<float>(height) * 0.5f;
    auto& sample = camera_builder.halton_history[frame % camera_builder.halton_history.size()];
    sample.frame = frame;
    sample.width = width;
    sample.height = height;
    sample.pixels[0] = camera_builder.halton_pixels[0];
    sample.pixels[1] = camera_builder.halton_pixels[1];
  }
}

inline void HookAlternateCameraBuilder(void* provider, void* source, void* view, void* parameters, bool no_jitter) {
  auto* original = camera_builder.alternate_original;
  const uint32_t width = last_render_width.load();
  const uint32_t height = last_render_height.load();
  if (!camera_builder_halton_enabled.load(std::memory_order_acquire)
      || no_jitter || provider == nullptr || view == nullptr || width == 0u || height == 0u) {
    original(provider, source, view, parameters, no_jitter);
    return;
  }
  alignas(16) std::array<uint8_t, 0xd8> view_copy = {};
  std::memcpy(view_copy.data(), view, view_copy.size());
  uint32_t viewport[2] = {};
  float old_jitter[2] = {};
  std::memcpy(viewport, view_copy.data() + 0x20u, sizeof(viewport));
  std::memcpy(old_jitter, view_copy.data() + 0x68u, sizeof(old_jitter));
  if (viewport[0] != width || viewport[1] != height || !std::isfinite(old_jitter[0]) || !std::isfinite(old_jitter[1])
      || std::abs(std::abs(old_jitter[0]) * width - 0.5f) >= 0.01f
      || std::abs(std::abs(old_jitter[1]) * height - 0.5f) >= 0.01f) {
    original(provider, source, view, parameters, no_jitter);
    return;
  }
  const uint64_t frame = camera_builder_frame.load(std::memory_order_acquire);
  const uint32_t step = static_cast<uint32_t>(frame % 16u) + 1u;
  const float pixels[2] = {HaltonValue(step, 2u) - 0.5f, HaltonValue(step, 3u) - 0.5f};
  const float replacement[2] = {-2.f * pixels[0] / width, 2.f * pixels[1] / height};
  std::memcpy(view_copy.data() + 0x68u, replacement, sizeof(replacement));
  original(provider, source, view_copy.data(), parameters, no_jitter);
  float projection[16] = {};
  std::memcpy(projection, static_cast<const uint8_t*>(provider) + 0xa0u, sizeof(projection));
  const bool verified = projection[14] == -1.f && projection[15] == 0.f
      && std::abs(-projection[2] * width * 0.5f - pixels[0]) < 0.01f
      && std::abs(projection[6] * height * 0.5f - pixels[1]) < 0.01f;
  if (verified && camera_builder_motion_offsets_enabled.load(std::memory_order_acquire)) {
    float offsets[2] = {};
    auto* camera = static_cast<uint8_t*>(provider);
    std::memcpy(offsets, camera + 0x2e0u, sizeof(offsets));
    if (std::abs(std::abs(offsets[0]) - 0.25f) < 0.001f
        && std::abs(std::abs(offsets[1]) - 0.25f) < 0.001f) {
      std::memcpy(camera + 0x2e0u, pixels, sizeof(pixels));
    }
  }
}

// This writer copies the game's original two-phase offsets into the provider.
// Only modify a render-sized camera whose projection matches this frame's Halton sample.
inline void HookCameraParametersWriter(void* provider, void* view, void* parameters) {
  camera_builder.offsets_original(provider, view, parameters);
  if (provider == nullptr || !camera_builder_motion_offsets_enabled.load(std::memory_order_acquire)) return;
  const uint64_t frame = camera_builder_frame.load(std::memory_order_acquire);
  const uint32_t width = last_render_width.load();
  const uint32_t height = last_render_height.load();
  float projection[16] = {};
  float viewport[2] = {};
  float old_offsets[2] = {};
  auto* camera = static_cast<uint8_t*>(provider);
  std::memcpy(projection, camera + 0xa0u, sizeof(projection));
  std::memcpy(viewport, camera + 0x220u, sizeof(viewport));
  std::memcpy(old_offsets, camera + 0x2e0u, sizeof(old_offsets));
  std::unique_lock lock(camera_builder.mutex);
  const auto& sample = camera_builder.halton_history[frame % camera_builder.halton_history.size()];

  const bool recognized = width != 0u && height != 0u && sample.frame == frame
      && sample.width == width && sample.height == height
      && viewport[0] == static_cast<float>(width) && viewport[1] == static_cast<float>(height)
      && projection[14] == -1.f && projection[15] == 0.f
      && std::abs(-projection[2] * width * 0.5f - sample.pixels[0]) < 0.01f
      && std::abs(projection[6] * height * 0.5f - sample.pixels[1]) < 0.01f
      && std::abs(std::abs(old_offsets[0]) - 0.25f) < 0.001f
      && std::abs(std::abs(old_offsets[1]) - 0.25f) < 0.001f;
  if (recognized) {
    std::memcpy(camera + 0x2e0u, sample.pixels, sizeof(sample.pixels));
  }
  lock.unlock();
  if (recognized && fg_camera_capture_enabled.load(std::memory_order_acquire)
      && fg_camera_sample_frame.load(std::memory_order_relaxed) != frame) {
    SceneMatrixSample camera_sample;
    if (CaptureFrameGenerationCamera(camera + 0x20u, width, height, frame, &camera_sample)) {
      std::unique_lock capture_lock(fg_camera_sample_mutex, std::try_to_lock);
      if (capture_lock.owns_lock()) {
        fg_camera_samples[frame % fg_camera_samples.size()] = camera_sample;
        fg_camera_sample_frame.store(frame, std::memory_order_release);
      } else {
        fg_camera_capture_busy.fetch_add(1u, std::memory_order_relaxed);
      }
    }
  }
}

inline void StartCameraBuilder() {
  std::unique_lock lock(camera_builder.mutex);
  if (camera_builder.state == CameraBuilderState::State::active
      || camera_builder.state == CameraBuilderState::State::unsupported
      || camera_builder.state == CameraBuilderState::State::install_failed) return;
  const auto module = GetModuleHandleW(L"FC_m64d3d12.dll");
  if (module == nullptr) {
    camera_builder.state = CameraBuilderState::State::unavailable;
    return;
  }
  const auto* base = reinterpret_cast<const uint8_t*>(module);
  const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
  if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew <= 0 || dos->e_lfanew > 0x1000) {
    camera_builder.state = CameraBuilderState::State::unsupported;
    return;
  }
  const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(base + dos->e_lfanew);
  constexpr uint8_t signature[] = {0x48, 0x8b, 0xc4, 0x48, 0x89, 0x58, 0x10, 0x48,
                                   0x89, 0x70, 0x18, 0x48, 0x89, 0x78, 0x20};
  constexpr uint32_t rva = 0x8a7b90u;
  if (nt->Signature != IMAGE_NT_SIGNATURE || nt->FileHeader.TimeDateStamp != 0x6824d119u
      || nt->OptionalHeader.SizeOfImage != 0x1fb64000u || std::memcmp(base + rva, signature, sizeof(signature)) != 0) {
    camera_builder.state = CameraBuilderState::State::unsupported;
    return;
  }
  const auto target = reinterpret_cast<CameraBuilder>(const_cast<uint8_t*>(base + rva));
  try {
    renodx::utils::minhook::Initialize();
    const renodx::utils::minhook::Function hook(target, &HookCameraBuilder, &camera_builder.original);
    renodx::utils::minhook::Create(hook);
    HMODULE owner = nullptr;
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
                            reinterpret_cast<LPCWSTR>(&HookCameraBuilder), &owner)) {
      renodx::utils::minhook::Remove(hook);
      renodx::utils::minhook::Uninitialize();
      camera_builder.state = CameraBuilderState::State::install_failed;
      return;
    }
    renodx::utils::minhook::Enable(hook);
    constexpr uint32_t alternate_rva = 0x6cb5b0u;
    constexpr uint8_t alternate_signature[] = {0x40, 0x55, 0x53, 0x56, 0x57, 0x48, 0x8d, 0xac,
                                               0x24, 0x98, 0xfe, 0xff, 0xff, 0x48, 0x81, 0xec};
    if (std::memcmp(base + alternate_rva, alternate_signature, sizeof(alternate_signature)) == 0) {
      try {
        const auto alternate_target = reinterpret_cast<AlternateCameraBuilder>(const_cast<uint8_t*>(base + alternate_rva));
        const renodx::utils::minhook::Function alternate_hook(alternate_target, &HookAlternateCameraBuilder, &camera_builder.alternate_original);
        renodx::utils::minhook::Create(alternate_hook);
        renodx::utils::minhook::Enable(alternate_hook);
      } catch (const std::exception&) {
        // The first-person hook is optional.
      }
    }
    // A separate guarded hook handles the later offset copy; failure leaves
    // the established Halton camera hook active without changing offsets.
    constexpr uint32_t offsets_rva = 0x6cbe10u;
    constexpr uint8_t offsets_signature[] = {0x40, 0x53, 0x55, 0x56, 0x57, 0x41, 0x56, 0x48, 0x81, 0xec, 0x80, 0x00, 0x00, 0x00};
    if (std::memcmp(base + offsets_rva, offsets_signature, sizeof(offsets_signature)) == 0) {
      try {
        const auto offsets_target = reinterpret_cast<CameraParametersWriter>(const_cast<uint8_t*>(base + offsets_rva));
        const renodx::utils::minhook::Function offsets_hook(offsets_target, &HookCameraParametersWriter, &camera_builder.offsets_original);
        renodx::utils::minhook::Create(offsets_hook);
        renodx::utils::minhook::Enable(offsets_hook);
        camera_builder.offsets_hook_installed = true;
      } catch (const std::exception&) {
        camera_builder.offsets_hook_installed = false;
      }
    }
    camera_builder.state = CameraBuilderState::State::active;
  } catch (const std::exception&) {
    camera_builder.state = CameraBuilderState::State::install_failed;
  }
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
    SlotInput& out) {
  out = {};
  if (cmd_list == nullptr || data == nullptr) return false;

  reshade::api::pipeline_layout layout = {};
  if (!CurrentPixelLayout(cmd_list, layout)) {
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

    if (!resolved) {
      continue;
    }
    if (!record.has_resource || !IsSrvDescriptorType(record.type)) continue;
    const reshade::api::resource_desc desc = device->get_resource_desc(record.resource);
    if (desc.type != reshade::api::resource_type::texture_2d) continue;
    out.resource = record.resource;
    out.format = record.format != reshade::api::format::unknown ? record.format : desc.texture.format;
    return true;
  }
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
    return true;
  }
  return false;
}

// The HDR10 output shader's b1 c18.y controls the conversion needed for the
// HUD-less image. Capture the same value the game uses for this output draw.
inline std::atomic<uint64_t> fg_hdr_parameter_frame{UINT64_MAX};
inline std::atomic<float> fg_hdr_brightness{0.f};
inline std::atomic<uint64_t> fg_hdr_parameter_hits{0u};
inline std::atomic<uint64_t> fg_hdr_parameter_misses{0u};
inline void CaptureFgHdrOutputParameters(reshade::api::command_list* cmd_list) {
  auto* data = cmd_list->get_private_data<CommandListData>();
  reshade::api::pipeline_layout layout = {};
  if (data == nullptr || !CurrentPixelLayout(cmd_list, layout)) {
    fg_hdr_parameter_misses.fetch_add(1u, std::memory_order_relaxed);
    return;
  }
  uint32_t params[4] = {};
  uint32_t bindings[4] = {};
  const uint32_t count = FindTablesForRegister(layout, 1u, 0u, reshade::api::descriptor_type::constant_buffer, params, bindings, 4u);
  for (uint32_t i = 0u; i < count; ++i) {
    if (params[i] >= data->descriptor_tables.size()) continue;
    const auto table = data->descriptor_tables[params[i]];
    DescriptorFingerprint key;
    RecordedDescriptor record;
    if (table.handle == 0u || !ReadSlotFingerprint(cmd_list->get_device(), table, bindings[i], key)
        || !LookupFingerprint(key, record) || !record.has_range
        || record.type != reshade::api::descriptor_type::constant_buffer || record.range.offset == UINT64_MAX
        || (record.range.size != UINT64_MAX && record.range.size < 296u)) continue;
    auto* resource = reinterpret_cast<ID3D12Resource*>(record.range.buffer.handle);
    if (resource == nullptr) continue;
    const auto desc = resource->GetDesc();
    D3D12_HEAP_PROPERTIES heap = {};
    D3D12_HEAP_FLAGS flags = {};
    if (desc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER || record.range.offset > desc.Width
        || desc.Width - record.range.offset < 296u || FAILED(resource->GetHeapProperties(&heap, &flags))
        || heap.Type != D3D12_HEAP_TYPE_UPLOAD) continue;
    void* mapped = nullptr;
    const D3D12_RANGE read_range = {static_cast<SIZE_T>(record.range.offset + 292u),
                                    static_cast<SIZE_T>(record.range.offset + 296u)};
    if (FAILED(resource->Map(0u, &read_range, &mapped)) || mapped == nullptr) continue;
    float brightness = 0.f;
    std::memcpy(&brightness, static_cast<const uint8_t*>(mapped) + record.range.offset + 292u, sizeof(brightness));
    const D3D12_RANGE written = {0u, 0u};
    resource->Unmap(0u, &written);
    if (!std::isfinite(brightness) || brightness <= 0.f) continue;
    fg_hdr_brightness.store(brightness, std::memory_order_relaxed);
    fg_hdr_parameter_frame.store(frame_counter, std::memory_order_release);
    fg_hdr_parameter_hits.fetch_add(1u, std::memory_order_relaxed);
    return;
  }
  fg_hdr_parameter_misses.fetch_add(1u, std::memory_order_relaxed);
}
// ── NGX ──
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
};

inline InstanceData ngx;

// Retired allocations stay alive until their recorded lists and GPU fence finish.
static constexpr size_t kRetiredGenerationLimit = 8u;
struct RetiredNgxFeature {
  NVSDK_NGX_Handle* feature = nullptr;
  NVSDK_NGX_Parameter* parameters = nullptr;
  std::shared_ptr<GpuRetirementTicket> ticket;
};
struct RetiredNgxOutput {
  Microsoft::WRL::ComPtr<ID3D12Resource> texture;
  std::shared_ptr<GpuRetirementTicket> ticket;
};
inline std::vector<RetiredNgxFeature> retired_ngx_features;
inline std::vector<RetiredNgxOutput> retired_ngx_outputs;

// One capture owns its readback until the submitting queue has retired the copy.
inline std::wstring GetProcessDirectory() {
  std::array<wchar_t, MAX_PATH> path = {};
  const DWORD length = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
  if (length == 0u || length >= path.size()) return L".";
  std::wstring result(path.data(), length);
  const auto separator = result.find_last_of(L"\\/");
  return separator == std::wstring::npos ? L"." : result.substr(0u, separator);
}

inline bool OnCreateDevice(reshade::api::device_api api, uint32_t& api_version) {
  if (api == reshade::api::device_api::d3d12) {
    const uint32_t requested = static_cast<uint32_t>((std::clamp)(fg_multiplier_setting, 0.f, 5.f));
    fg_requested_generated.store(requested, std::memory_order_release);
    fg_enable_requested.store(requested != 0u, std::memory_order_release);
    farcry6_dlss::streamline::LoadRuntime(GetProcessDirectory());
    farcry6_dlss::streamline::StartBootstrap(GetProcessDirectory());
  }

  return false;
}

inline bool IsSupported() { return any_gpu.load(); }

inline void ReleaseFeatureHandleOnly(bool retire = false) {
  if (retire && (ngx.feature != nullptr || ngx.runtime_parameters != nullptr)) {
    retired_ngx_features.push_back({.feature = ngx.feature, .parameters = ngx.runtime_parameters, .ticket = RetireGpuCommands()});
    ngx.feature = nullptr;
    ngx.runtime_parameters = nullptr;
  } else {
    if (ngx.feature != nullptr) {
      NVSDK_NGX_D3D12_ReleaseFeature(ngx.feature);
      ngx.feature = nullptr;
    }
    if (ngx.runtime_parameters != nullptr) {
      NVSDK_NGX_D3D12_DestroyParameters(ngx.runtime_parameters);
      ngx.runtime_parameters = nullptr;
    }
  }
  ngx.requested_render_preset = -1;
  ngx.render_preset = -1;
  ngx.perf_quality = -1;
  ngx.feature_flags = 0;
  ngx.create_failed = false;
  ngx.eval_failed = false;
  ngx.reset = true;
}

inline void ReleaseFeature(bool retire = false) {
  ReleaseFeatureHandleOnly(retire);
  if (retire && ngx.output_texture != nullptr) {
    retired_ngx_outputs.push_back({.texture = std::move(ngx.output_texture), .ticket = RetireGpuCommands()});
  } else {
    ngx.output_texture.Reset();
  }
  ngx.width = 0u;
  ngx.height = 0u;
  ngx.input_width = 0u;
  ngx.input_height = 0u;
  ngx.output_format = DXGI_FORMAT_UNKNOWN;
}

inline void ReleaseNgx() {
  ReleaseFeature();
  for (const auto& retired : retired_ngx_features) {
    if (retired.feature != nullptr) NVSDK_NGX_D3D12_ReleaseFeature(retired.feature);
    if (retired.parameters != nullptr) NVSDK_NGX_D3D12_DestroyParameters(retired.parameters);
  }
  retired_ngx_features.clear();
  retired_ngx_outputs.clear();
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

    return false;
  }

  const NVSDK_NGX_Result params_result = NVSDK_NGX_D3D12_GetCapabilityParameters(&ngx.capability_parameters);
  if (NVSDK_NGX_FAILED(params_result) || ngx.capability_parameters == nullptr) {
    ngx.init_failed = true;

    NVSDK_NGX_D3D12_Shutdown1(native_device);
    return false;
  }

  int super_sampling_available = 0;
  ngx.capability_parameters->Get(NVSDK_NGX_EParameter_SuperSampling_Available, &super_sampling_available);
  ngx.supported = super_sampling_available > 0;

  ngx.device = native_device;
  ngx.initialized = true;

  return ngx.supported;
}

inline bool EnsureOutputTexture(ID3D12Device* device, uint32_t width, uint32_t height, DXGI_FORMAT format) {
  if (device == nullptr || width == 0u || height == 0u || format == DXGI_FORMAT_UNKNOWN) return false;
  if (ngx.output_texture != nullptr && ngx.width == width && ngx.height == height && ngx.output_format == format) return true;

  if ((ngx.output_texture != nullptr && retired_ngx_outputs.size() >= kRetiredGenerationLimit)
      || (ngx.feature != nullptr && retired_ngx_features.size() >= kRetiredGenerationLimit)) {
    return false;
  }
  ReleaseFeature(true);

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
  // Motion vectors use the render resolution.
  // MVLowRes describes texture dimensions, independently of vector units.
  return NVSDK_NGX_DLSS_Feature_Flags_MVLowRes
       | NVSDK_NGX_DLSS_Feature_Flags_DepthInverted
       | NVSDK_NGX_DLSS_Feature_Flags_IsHDR;
}

inline int GetPerfQualityValue(uint32_t input_width, uint32_t input_height, uint32_t target_width, uint32_t target_height) {
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
  if (ngx.feature != nullptr) {
    if (retired_ngx_features.size() >= kRetiredGenerationLimit) {
      return false;
    }
    ReleaseFeatureHandleOnly(true);
  }
  if (ngx.create_failed) return false;

  const NVSDK_NGX_Result params_result = NVSDK_NGX_D3D12_AllocateParameters(&ngx.runtime_parameters);
  if (NVSDK_NGX_FAILED(params_result) || ngx.runtime_parameters == nullptr) {
    ngx.create_failed = true;

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

    return false;
  }

  ngx.requested_render_preset = render_preset;
  ngx.render_preset = actual_render_preset;
  ngx.perf_quality = actual_perf_quality;
  ngx.feature_flags = feature_flags;
  ngx.input_width = input_width;
  ngx.input_height = input_height;
  ngx.reset = true;

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
struct RetiredUnpack {
  UnpackData data;
  std::shared_ptr<GpuRetirementTicket> ticket;
};
inline std::vector<RetiredUnpack> retired_unpack;
inline reshade::api::device* unpack_device = nullptr;

struct FgHudlessSlot {
  reshade::api::resource texture = {};
  reshade::api::resource_view source_view = {};
  reshade::api::resource_view target_view = {};
  reshade::api::descriptor_table tables[2] = {};
  uint64_t source = 0u;
  uint32_t width = 0u;
  uint32_t height = 0u;
  std::shared_ptr<GpuRetirementTicket> ticket;
};
struct FgHudlessState {
  std::shared_mutex mutex;
  reshade::api::device* device = nullptr;
  reshade::api::pipeline_layout layout = {};
  reshade::api::pipeline pipeline = {};
  bool pipeline_attempted = false;
  reshade::api::format output_format = reshade::api::format::unknown;
  uint32_t slot_limit = 0u;
  std::array<FgHudlessSlot, 12> slots = {};
};
inline FgHudlessState fg_hudless;
struct RcasSlot {
  reshade::api::resource texture = {};
  reshade::api::resource_view source_view = {};
  reshade::api::resource_view target_view = {};
  reshade::api::descriptor_table tables[2] = {};
  uint64_t source = 0u;
  uint32_t width = 0u;
  uint32_t height = 0u;
  std::shared_ptr<GpuRetirementTicket> ticket;
};
struct RcasState {
  std::shared_mutex mutex;
  reshade::api::device* device = nullptr;
  reshade::api::pipeline_layout layout = {};
  reshade::api::pipeline pipeline = {};
  bool pipeline_attempted = false;
  std::array<RcasSlot, 12> slots = {};
  std::atomic<uint64_t> dispatched{0u};
  std::atomic<uint64_t> skipped{0u};
};
inline RcasState rcas;
inline RcasState color_conversion;
inline void ReleaseColorConversion(reshade::api::device* device) {
  std::unique_lock lock(color_conversion.mutex);
  if (device == nullptr || color_conversion.device != device) return;
  for (auto& slot : color_conversion.slots) {
    if (slot.source_view.handle != 0u) device->destroy_resource_view(slot.source_view);
    if (slot.target_view.handle != 0u) device->destroy_resource_view(slot.target_view);
    if (slot.texture.handle != 0u) device->destroy_resource(slot.texture);
    for (auto& table : slot.tables) {
      if (table.handle != 0u) device->free_descriptor_table(table);
    }
    slot = {};
  }
  if (color_conversion.pipeline.handle != 0u) device->destroy_pipeline(color_conversion.pipeline);
  if (color_conversion.layout.handle != 0u) device->destroy_pipeline_layout(color_conversion.layout);
  color_conversion.pipeline = {};
  color_conversion.layout = {};
  color_conversion.pipeline_attempted = false;
  color_conversion.device = nullptr;
}
inline std::atomic<bool> fg_hudless_slots_allocated{false};
inline std::atomic<bool> fg_hudless_retire_requested{false};

inline void ReleaseFgHudless(reshade::api::device* device) {
  std::unique_lock lock(fg_hudless.mutex);
  if (device == nullptr || fg_hudless.device != device) return;
  for (auto& slot : fg_hudless.slots) {
    if (slot.source_view.handle) device->destroy_resource_view(slot.source_view);
    if (slot.target_view.handle) device->destroy_resource_view(slot.target_view);
    if (slot.texture.handle) device->destroy_resource(slot.texture);
    for (auto& table : slot.tables) {
      if (table.handle) device->free_descriptor_table(table);
    }
    slot = {};
  }
  if (fg_hudless.pipeline.handle) device->destroy_pipeline(fg_hudless.pipeline);
  if (fg_hudless.layout.handle) device->destroy_pipeline_layout(fg_hudless.layout);
  fg_hudless.device = nullptr;
  fg_hudless.pipeline = {};
  fg_hudless.layout = {};
  fg_hudless.pipeline_attempted = false;
  fg_hudless.output_format = reshade::api::format::unknown;
  fg_hudless.slot_limit = 0u;
  fg_hudless_slots_allocated.store(false, std::memory_order_release);
  fg_hudless_retire_requested.store(false, std::memory_order_release);
}

inline void ReleaseUnpack(reshade::api::device* device) {
  if (device == nullptr) return;
  retired_unpack.push_back({.data = unpack});
  for (auto& retired : retired_unpack) {
    auto& generation = retired.data;
    if (generation.uav.handle) device->destroy_resource_view(generation.uav);
    if (generation.srv.handle) device->destroy_resource_view(generation.srv);
    if (generation.texture.handle) device->destroy_resource(generation.texture);
    if (generation.pipeline.handle) device->destroy_pipeline(generation.pipeline);

    for (auto& table : generation.tables) {
      if (table.handle) device->free_descriptor_table(table);
    }
    if (generation.layout.handle) device->destroy_pipeline_layout(generation.layout);
  }
  retired_unpack.clear();
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

  DR srv_r = {0, 0, 0, 2, DS::all_compute, 1, DT::texture_shader_resource_view};  // t0 motion, t1 mask
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

    return false;
  }
  for (uint32_t i = 0u; i < 2u; ++i) {
    if (!device->allocate_descriptor_table(unpack.layout, i, &unpack.tables[i])) {

      return false;
    }
  }
  return true;
}

inline bool EnsureUnpackResources(reshade::api::device* device, uint32_t width, uint32_t height) {
  if (device == nullptr || width == 0u || height == 0u) return false;
  if (unpack.texture.handle != 0u && unpack.width == width && unpack.height == height) return unpack.pipeline.handle != 0u;
  if (unpack_device != device) {
    ReleaseUnpack(unpack_device);
    unpack_device = device;
  } else if (unpack.texture.handle != 0u || unpack.layout.handle != 0u) {
    if (retired_unpack.size() >= kRetiredGenerationLimit) {
      return false;
    }
    retired_unpack.push_back({.data = unpack, .ticket = RetireGpuCommands()});

    unpack = {};
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

    return false;
  }
  reshade::api::resource_view_desc vd(
      reshade::api::resource_view_type::texture_2d, reshade::api::format::r16g16_float, 0, 1, 0, 1);
  if (!device->create_resource_view(unpack.texture, reshade::api::resource_usage::shader_resource, vd, &unpack.srv)
      || !device->create_resource_view(unpack.texture, reshade::api::resource_usage::unordered_access, vd, &unpack.uav)) {

    return false;
  }

  reshade::api::shader_desc cs_desc = {};
  cs_desc.code = __dlss_mv_unpack.data();
  cs_desc.code_size = __dlss_mv_unpack.size();
  cs_desc.entry_point = "main";
  reshade::api::pipeline_subobject so = {reshade::api::pipeline_subobject_type::compute_shader, 1, &cs_desc};
  if (!device->create_pipeline(unpack.layout, 1, &so, &unpack.pipeline) || unpack.pipeline.handle == 0u) {

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
                       1.f, 0.f,
                       static_cast<float>(src_width), static_cast<float>(src_height), 0.f, 0.f};
  cmd_list->push_constants(CS, unpack.layout, 2, 0, 8, pc);
  cmd_list->barrier(unpack.texture, reshade::api::resource_usage::shader_resource, reshade::api::resource_usage::unordered_access);
  cmd_list->dispatch((width + 7u) / 8u, (height + 7u) / 8u, 1);
  cmd_list->barrier(unpack.texture, reshade::api::resource_usage::unordered_access, reshade::api::resource_usage::shader_resource);
  return true;
}

// ── Draw identification ──

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

// ── game upscale as the upscaler ──
// With game upscale on, the game upscales in its own pass: a compute dispatch that reads
// the internal-resolution image and writes the display-resolution one, with the
// minimap and UI composited afterwards instead of inside it. That is the place for
// DLSS to upscale - the input is still the scene's own aliased colour, and the
// game's later stages keep reading exactly what they expect.
inline constexpr uint32_t kGameUpscaleEasuHash = 0x4E927DB0u;
inline std::atomic<bool> game_upscale_active{false};   // seen last frame, so this one too
inline bool game_upscale_seen_this_frame = false;
inline std::atomic<uint32_t> display_width{0u};
inline std::atomic<uint32_t> display_height{0u};
inline std::atomic<uint32_t> game_upscale_target_width{0u};
inline std::atomic<uint32_t> game_upscale_target_height{0u};
inline std::atomic<uint32_t> game_upscale_target_format{0u};
// Set when the evaluation wrote the game's upscale resource this frame, which is
// the only case in which the game's own dispatch may be taken out.
inline std::atomic<bool> game_upscale_wrote_this_frame{false};

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

  if (dlss_enabled == 0.f || !any_gpu.load()) return false;
  if (cmd_list == nullptr) return false;
  auto* shader_state = renodx::utils::shader::GetCurrentState(cmd_list);
  if (shader_state == nullptr) return false;
  const uint32_t compute_hash = renodx::utils::shader::GetCurrentComputeShaderHash(shader_state);

  if (compute_hash != kGameUpscaleEasuHash) return false;
  // Seeing the dispatch is what tells the frame that the game has an upscaler to
  // hand over - without this the evaluation never targets the display resolution
  // and the dispatch is never prevented, each waiting on the other.
  game_upscale_seen_this_frame = true;
  auto* data = Get(cmd_list);
  if (data == nullptr) return false;

  const uint32_t render_width = last_render_width.load();
  const uint32_t render_height = last_render_height.load();
  reshade::api::resource output = {};
  uint32_t width = 0u, height = 0u, format = 0u;
  if (render_width == 0u || render_height == 0u
      || !ResolveDispatchOutput(cmd_list, data, render_width, render_height, output, width, height, format)) {
    return false;
  }
  TrackDepthResource(output.handle);
  // What the upscaler writes is what the next frame's evaluation targets: the size
  // can only be learned from here, and it must not be matched against this frame's
  // output size or the two can never meet.
  game_upscale_target_width.store(width);
  game_upscale_target_height.store(height);
  game_upscale_target_format.store(format);
  game_upscale_output_resource.store(output.handle);

  // NGX writes into this resource directly, at the evaluation point, so there is
  // nothing to copy: this only takes the game's dispatch out of the way once that
  // write has actually happened. A frame where it did not keeps the game's upscale.
  if (!game_upscale_wrote_this_frame.load() || PauseTaaRecentlySeen()) return false;
  return true;
}

inline bool EvaluateDLSS(reshade::api::command_list* cmd_list, const CommandListData* data, uint32_t hash) {

  if (cmd_list == nullptr || data == nullptr || dlss_enabled == 0.f || !IsSupported()) return false;
  auto* device = cmd_list->get_device();
  if (device == nullptr || device->get_api() != reshade::api::device_api::d3d12) return false;
    bool nvidia_device = false;
    for (const auto& native : nvidia_devs) {
      if (native.load(std::memory_order_relaxed) == device->get_native()) nvidia_device = true;
    }
    if (!nvidia_device) {

      return false;
    }

  auto* command_list = reinterpret_cast<ID3D12GraphicsCommandList*>(cmd_list->get_native());
  if (command_list == nullptr) return false;
  // The pause variant has a separate history output. Leave its TAA to the game
  // and resume DLSS with fresh history when gameplay returns.
  if (hash == kTaaPauseHash) {

    pause_taa_last_frame.store(camera_builder_frame.load(std::memory_order_acquire), std::memory_order_release);
    ngx.reset = true;
    return false;
  }
  if (PauseTaaRecentlySeen()) {
    ngx.reset = true;
    return false;
  }
  TrackGpuCommands(cmd_list);

  // Per-device gate: a non-NVIDIA device's command lists keep vanilla TAA.
  if (!IsKnownGpuNative(device->get_native())) {

    return false;
  }

  SlotInput color_in;
  SlotInput motion_in;
  const bool color_ok = ResolveSlot(cmd_list, data, kSlotColor, color_in);
  const bool motion_ok = ResolveSlot(cmd_list, data, kSlotPackedMv, motion_in);
  reshade::api::buffer_range b0_range = {};
  const bool b0_ok = ResolveB0(cmd_list, data, b0_range);
  float temporal_jitter[4] = {};
  bool temporal_jitter_valid = false;
  if (b0_ok && b0_range.offset != UINT64_MAX) {
    // FC_m64d3d12.dll registers JitterParameters at byte offset 0x60 in
    // CTemporalEffectsProvider. Record the TAA buffer bytes as a candidate;
    // their identity, units and frame association still require validation.
    constexpr uint64_t kTemporalJitterOffset = 0x60u;
    constexpr uint64_t kTemporalJitterBytes = 4u * sizeof(float);
    const uint8_t* mapped = MappedRingData(b0_range.buffer.handle);
    if (mapped != nullptr && b0_range.size >= kTemporalJitterOffset + kTemporalJitterBytes
        && b0_range.offset <= mapped_ring.size
        && mapped_ring.size - b0_range.offset >= kTemporalJitterOffset + kTemporalJitterBytes) {
      std::memcpy(temporal_jitter, mapped + b0_range.offset + kTemporalJitterOffset, kTemporalJitterBytes);
      temporal_jitter_valid = std::isfinite(temporal_jitter[0]) && std::isfinite(temporal_jitter[1])
          && temporal_jitter[0] >= 0.f && temporal_jitter[0] <= 1.f
          && temporal_jitter[1] >= 0.f && temporal_jitter[1] <= 1.f;
    }
  }

  auto* output_target = GetNativeResource(device, data->rtvs[0]);
  if (output_target == nullptr) {
    return false;
  }
  const D3D12_RESOURCE_DESC taa_desc = output_target->GetDesc();
  const uint32_t taa_width = static_cast<uint32_t>(taa_desc.Width);
  const uint32_t taa_height = taa_desc.Height;
  // Publish the new gameplay extent before resolving depth: depth selection
  // runs at Present and must be able to discover the new-size resource even
  // while the previous frame's depth is still selected.
  if (taa_width >= 256u && taa_height >= 256u && taa_width <= 7680u && taa_height <= 4320u) {
    const uint32_t previous_width = last_render_width.exchange(taa_width);
    const uint32_t previous_height = last_render_height.exchange(taa_height);
    if (previous_width != 0u && previous_height != 0u
        && (previous_width != taa_width || previous_height != taa_height)) {
      ngx.create_failed = false;
      ngx.eval_failed = false;
      ngx.reset = true;
    }
  }
  if (ngx.eval_failed) return false;

  auto* color = color_ok ? reinterpret_cast<ID3D12Resource*>(color_in.resource.handle) : nullptr;
  auto* motion = motion_ok ? reinterpret_cast<ID3D12Resource*>(motion_in.resource.handle) : nullptr;
  Microsoft::WRL::ComPtr<ID3D12Resource> scene_depth_owned;
  {
    const std::lock_guard<std::mutex> lock(depth_mutex);
    if (scene_depth_resource != 0u && scene_depth_width == taa_width && scene_depth_height == taa_height) {
      scene_depth_owned = reinterpret_cast<ID3D12Resource*>(scene_depth_resource);
    }
  }
  auto* scene_depth = scene_depth_owned.Get();
  auto* depth = scene_depth;
  if (color == nullptr || motion == nullptr || depth == nullptr) {
    return false;
  }

  const D3D12_RESOURCE_DESC color_desc = color->GetDesc();
  const D3D12_RESOURCE_DESC motion_desc_early = motion->GetDesc();
  const D3D12_RESOURCE_DESC output_desc = output_target->GetDesc();
  // The input size comes from the TAA target, not from whatever the colour slot
  // resolved to: the target is the render resolution by definition, so a wrong
  // colour resolution is caught below instead of silently becoming the input.
  const uint32_t input_width = static_cast<uint32_t>(output_desc.Width);
  const uint32_t input_height = output_desc.Height;
  // With the game's own upscaler running, DLSS *is* the upscaler: its output is the
  // display-resolution image the upscaler would have written, at the size the last
  // replaced dispatch reported.
  const uint64_t game_upscale_target_handle = game_upscale_output_resource.load();
  // The resource is written by NGX directly, so it may only be used once the
  // engine has stated a state for it: an unknown state means no write this frame.
  const reshade::api::resource_usage game_upscale_target_state = game_upscale_target_handle != 0u
      ? TrackedDepthState(game_upscale_target_handle)
      : reshade::api::resource_usage::undefined;
  const bool game_upscale_mode = game_upscale_active.load() && game_upscale_target_width.load() != 0u
      && game_upscale_target_handle != 0u
      && game_upscale_target_state != reshade::api::resource_usage::undefined;
  const reshade::api::resource game_upscale_target_resource{game_upscale_target_handle};
  const bool game_upscale_needs_write_transition = game_upscale_mode
      && (static_cast<uint32_t>(game_upscale_target_state)
          & static_cast<uint32_t>(reshade::api::resource_usage::unordered_access)) == 0u;
  const uint32_t target_width = game_upscale_mode ? game_upscale_target_width.load() : display_width.load();
  const uint32_t target_height = game_upscale_mode ? game_upscale_target_height.load() : display_height.load();

  if (input_width == 0u || input_height == 0u
      || (input_width > target_width || input_height > target_height)) {

    return false;
  }
  const bool color_matches = color_desc.Width == input_width && color_desc.Height == input_height;
  const bool motion_matches = (motion_desc_early.Width == input_width / 2u && motion_desc_early.Height == input_height / 2u)
      || (motion_desc_early.Width == input_width && motion_desc_early.Height == input_height);
  if (!color_matches || !motion_matches) {

    return false;
  }
  if (input_width < 256u || input_height < 256u || target_width < 256u || target_height < 256u
      || target_width > 7680u || target_height > 4320u
      || target_width > input_width * 4u || target_height > input_height * 4u) {

    return false;
  }

  // Published before anything can return early: the depth selection filters its
  // candidates by this size. Leaving them
  // to the evaluation meant that when the evaluation stopped running, the depth
  // selection fell back to ranking by draw count and picked a shadow cascade.
  last_render_width.store(input_width);
  last_render_height.store(input_height);
  // NGX output format follows whatever the output resource will be: the TAA target
  // normally, the upscaler's target when DLSS is doing the upscaling.
  const DXGI_FORMAT wanted_format = (game_upscale_mode && game_upscale_target_format.load() != 0u)
      ? static_cast<DXGI_FORMAT>(game_upscale_target_format.load())
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

    return false;
  }

  if (!game_upscale_mode && hash != kTaaMainHash) return false;

  if (!EnsureNgxInitialized(device)) {

    return false;
  }
  if (!EnsureOutputTexture(reinterpret_cast<ID3D12Device*>(device->get_native()), target_width, target_height, output_format)) return false;

  const uint32_t motion_src_width = static_cast<uint32_t>(motion->GetDesc().Width);
  const uint32_t motion_src_height = motion->GetDesc().Height;

  // Reset history on structural changes; interrupted evaluation is handled below.
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
  if (!EnsureFeature(command_list, input_width, input_height, target_width, target_height,
                     render_preset, feature_flags, perf_quality)) {

    return false;
  }

  last_render_width.store(input_width);
  last_render_height.store(input_height);
  float jitter_x = temporal_jitter_valid ? temporal_jitter[0] - 0.5f : 0.f;
  float jitter_y = temporal_jitter_valid ? temporal_jitter[1] - 0.5f : 0.f;
  bool builder_halton = false;
  if (camera_builder_halton_enabled.load(std::memory_order_acquire)) {
    std::shared_lock lock(camera_builder.mutex);
    builder_halton = camera_builder.halton_frame == frame_counter;
    if (builder_halton) {
      jitter_x = camera_builder.halton_pixels[0];
      jitter_y = camera_builder.halton_pixels[1];
    }
  }

  // Nothing that is not a finite number reaches NGX, and nothing outside
  // sub-pixel range either: a jitter is a sub-pixel offset, so a larger value can
  // only be a read error.
  if (!std::isfinite(jitter_x) || !std::isfinite(jitter_y)
      || fabsf(jitter_x) > 0.5f || fabsf(jitter_y) > 0.5f) {
    jitter_x = 0.f;
    jitter_y = 0.f;
  }

  const reshade::api::resource_view motion_view = GetOrCreateTextureView(device, motion_in.resource, motion_in.format);
  if (motion_view.handle == 0u || !DispatchUnpack(device, cmd_list, motion_view, input_width, input_height,
                                                   motion_src_width, motion_src_height)) {

    return false;
  }
  auto* motion_vectors = reinterpret_cast<ID3D12Resource*>(unpack.texture.handle);
  if (motion_vectors == nullptr) return false;

  static bool previous_builder_halton = false;
  static bool previous_motion_offsets = false;
  if (builder_halton != previous_builder_halton
      || camera_builder_motion_offsets_enabled.load(std::memory_order_acquire) != previous_motion_offsets) {
    ngx.reset = true;
    previous_builder_halton = builder_halton;
    previous_motion_offsets = camera_builder_motion_offsets_enabled.load(std::memory_order_acquire);
  }

  // Reserve both conversions before dispatching either one: a partial conversion
  // must never allow a linear BT.2020 output to reach the game's gamma pipeline.
  constexpr bool convert_color = true;
  std::unique_lock conversion_lock(color_conversion.mutex, std::defer_lock);
  RcasSlot* conversion_slots[2] = {};
  if (convert_color) {
    if (data->in_render_pass || color_desc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D
        || color_desc.Format != DXGI_FORMAT_R16G16B16A16_FLOAT || color_desc.DepthOrArraySize != 1u
        || color_desc.MipLevels != 1u || color_desc.SampleDesc.Count != 1u
        || !conversion_lock.try_lock()) return false;
    auto* output = (game_upscale_mode ? reinterpret_cast<ID3D12Resource*>(game_upscale_target_handle) : ngx.output_texture.Get());
    if (output == nullptr) return false;
    const auto output_desc = output->GetDesc();
    if (output_desc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D
        || output_desc.Format != DXGI_FORMAT_R16G16B16A16_FLOAT || output_desc.Width != target_width
        || output_desc.Height != target_height || output_desc.DepthOrArraySize != 1u
        || output_desc.MipLevels != 1u || output_desc.SampleDesc.Count != 1u) return false;
    if (color_conversion.device == nullptr) color_conversion.device = device;
    if (color_conversion.device != device) return false;
    if (!color_conversion.pipeline_attempted) {
      color_conversion.pipeline_attempted = true;
      using DS = reshade::api::shader_stage;
      using DT = reshade::api::descriptor_type;
      using PT = reshade::api::pipeline_layout_param_type;
      reshade::api::descriptor_range srv = {0, 0, 0, 1, DS::all_compute, 1, DT::texture_shader_resource_view};
      reshade::api::descriptor_range uav = {0, 0, 0, 1, DS::all_compute, 1, DT::texture_unordered_access_view};
      reshade::api::constant_range constants = {};
      constants.count = 3u;
      constants.visibility = DS::all_compute;
      reshade::api::pipeline_layout_param params[3] = {};
      params[0].type = PT::descriptor_table;
      params[0].descriptor_table = {1u, &srv};
      params[1].type = PT::descriptor_table;
      params[1].descriptor_table = {1u, &uav};
      params[2].type = PT::push_constants;
      params[2].push_constants = constants;
      if (device->create_pipeline_layout(3u, params, &color_conversion.layout)) {
        reshade::api::shader_desc shader = {};
        shader.code = __dlss_color_convert.data();
        shader.code_size = __dlss_color_convert.size();
        shader.entry_point = "main";
        const reshade::api::pipeline_subobject subobject = {reshade::api::pipeline_subobject_type::compute_shader, 1u, &shader};
        (void)device->create_pipeline(color_conversion.layout, 1u, &subobject, &color_conversion.pipeline);
      }
    }
    if (color_conversion.pipeline.handle == 0u) return false;
    {
      std::shared_lock retirement_lock(gpu_retirement.mutex);
      uint32_t found = 0u;
      for (auto& candidate : color_conversion.slots) {
        if (!candidate.ticket || candidate.ticket->complete) {
          conversion_slots[found++] = &candidate;
          if (found == 2u) break;
        }
      }
    }
    if (conversion_slots[1] == nullptr) return false;
    const reshade::api::resource_view_desc view_desc(
        reshade::api::resource_view_type::texture_2d, reshade::api::format::r16g16b16a16_float, 0u, 1u, 0u, 1u);
    for (uint32_t i = 0u; i < 2u; ++i) {
      auto& slot = *conversion_slots[i];
      slot.ticket.reset();
      auto* source = (i == 0u ? color : output);
      const uint64_t source_handle = reinterpret_cast<uint64_t>(source);
      const uint32_t width = (i == 0u ? input_width : target_width);
      const uint32_t height = (i == 0u ? input_height : target_height);
      if (slot.source != source_handle && slot.source_view.handle != 0u) {
        device->destroy_resource_view(slot.source_view);
        slot.source_view = {};
        slot.source = 0u;
      }
      if ((slot.width != width || slot.height != height) && slot.texture.handle != 0u) {
        if (slot.target_view.handle != 0u) device->destroy_resource_view(slot.target_view);
        device->destroy_resource(slot.texture);
        slot.texture = {};
        slot.target_view = {};
        slot.width = slot.height = 0u;
      }
      if (slot.source_view.handle == 0u
          && device->create_resource_view(reshade::api::resource{source_handle}, reshade::api::resource_usage::shader_resource,
                                          view_desc, &slot.source_view)) {
        slot.source = source_handle;
      }
      if (slot.texture.handle == 0u) {
        reshade::api::resource_desc desc = {};
        desc.type = reshade::api::resource_type::texture_2d;
        desc.texture = {width, height, 1u, 1u, reshade::api::format::r16g16b16a16_float, 1u};
        desc.heap = reshade::api::memory_heap::gpu_only;
        desc.usage = reshade::api::resource_usage::unordered_access | reshade::api::resource_usage::shader_resource | reshade::api::resource_usage::copy_source;
        if (device->create_resource(desc, nullptr, reshade::api::resource_usage::unordered_access, &slot.texture)) {
          slot.width = width;
          slot.height = height;
        }
      }
      if (slot.texture.handle != 0u && slot.target_view.handle == 0u) {
        (void)device->create_resource_view(slot.texture, reshade::api::resource_usage::unordered_access, view_desc, &slot.target_view);
      }
      for (uint32_t j = 0u; j < 2u; ++j) {
        if (slot.tables[j].handle == 0u) (void)device->allocate_descriptor_table(color_conversion.layout, j, &slot.tables[j]);
      }
      if (!slot.source_view.handle || !slot.target_view.handle || !slot.tables[0].handle || !slot.tables[1].handle) return false;
      const reshade::api::descriptor_table_update updates[] = {
          {slot.tables[0], 0u, 0u, 1u, reshade::api::descriptor_type::texture_shader_resource_view, &slot.source_view},
          {slot.tables[1], 0u, 0u, 1u, reshade::api::descriptor_type::texture_unordered_access_view, &slot.target_view},
      };
      device->update_descriptor_tables(2u, updates);
    }
    const auto stage = reshade::api::shader_stage::all_compute;
    cmd_list->bind_pipeline(reshade::api::pipeline_stage::all_compute, color_conversion.pipeline);
    cmd_list->bind_descriptor_tables(stage, color_conversion.layout, 0u, 2u, conversion_slots[0]->tables);
    const uint32_t constants[] = {input_width, input_height, 1u};
    cmd_list->push_constants(stage, color_conversion.layout, 2u, 0u, 3u, constants);
    cmd_list->dispatch((input_width + 7u) / 8u, (input_height + 7u) / 8u, 1u);
    cmd_list->barrier(conversion_slots[0]->texture, reshade::api::resource_usage::unordered_access, reshade::api::resource_usage::shader_resource);
    RestoreGameDescriptorState(cmd_list);
    conversion_slots[0]->ticket = RetireGpuCommands(cmd_list);
  }
  static bool previous_color_conversion = false;
  if (convert_color != previous_color_conversion) {
    ngx.reset = true;
    previous_color_conversion = convert_color;
  }
  NVSDK_NGX_D3D12_DLSS_Eval_Params eval = {};
  eval.Feature.pInColor = (convert_color ? reinterpret_cast<ID3D12Resource*>(conversion_slots[0]->texture.handle) : color);
  eval.Feature.pInOutput = game_upscale_mode
      ? reinterpret_cast<ID3D12Resource*>(game_upscale_target_handle)
      : ngx.output_texture.Get();
  eval.pInDepth = depth;
  eval.pInMotionVectors = motion_vectors;
  eval.InJitterOffsetX = jitter_x;
  eval.InJitterOffsetY = jitter_y;
  eval.InRenderSubrectDimensions.Width = input_width;
  eval.InRenderSubrectDimensions.Height = input_height;
  const uint64_t previous_eval_frame = last_upscaler_eval_frame.load(std::memory_order_relaxed);
  const bool interrupted_history = previous_eval_frame != UINT64_MAX && frame_counter > previous_eval_frame
                                && frame_counter - previous_eval_frame > 1u;
  eval.InReset = (ngx.reset || interrupted_history) ? 1 : 0;
  eval.InMVScaleX = 1.f;
  eval.InMVScaleY = 1.f;
  eval.InPreExposure = 1.f;
  eval.InExposureScale = 1.f;

  // NGX reads the depth as a shader resource in a compute dispatch, so the depth
  // has to be in a non-pixel shader-readable state for that read to return data.
  // The engine leaves it in a depth-stencil state, where the read returns zeros —
  // a black depth view in DLSS's own debug overlay. It is put back into the state
  // the engine's tracking believes it is in afterwards, because the engine writes
  // its next barrier against that belief and not against what actually happened.
  const reshade::api::resource depth_resource{reinterpret_cast<uint64_t>(scene_depth)};
  reshade::api::resource_usage depth_state = reshade::api::resource_usage::undefined;
  if (scene_depth != nullptr) {
    depth_state = TrackedDepthState(depth_resource.handle);
    // Bound as a depth-stencil view, so that is what it is while the tracker has
    // not seen a barrier for it yet.
    if (depth_state == reshade::api::resource_usage::undefined) {
      depth_state = reshade::api::resource_usage::depth_stencil_write;
    }
  }
  const bool depth_needs_read_transition = depth_state != reshade::api::resource_usage::undefined
      && (static_cast<uint32_t>(depth_state)
          & static_cast<uint32_t>(reshade::api::resource_usage::shader_resource_non_pixel)) == 0u;
  if (depth_needs_read_transition) {
    cmd_list->barrier(depth_resource, depth_state, reshade::api::resource_usage::shader_resource_non_pixel);
  }
  // NGX writes the upscaler's own resource, which it can only do while that
  // resource is writable.
  if (game_upscale_needs_write_transition) {
    cmd_list->barrier(game_upscale_target_resource, game_upscale_target_state, reshade::api::resource_usage::unordered_access);
  }

  const NVSDK_NGX_Result result = NGX_D3D12_EVALUATE_DLSS_EXT(command_list, ngx.feature, ngx.runtime_parameters, &eval);
  if (depth_needs_read_transition) {
    cmd_list->barrier(depth_resource, reshade::api::resource_usage::shader_resource_non_pixel, depth_state);
  }
  if (game_upscale_needs_write_transition) {
    cmd_list->barrier(game_upscale_target_resource, reshade::api::resource_usage::unordered_access, game_upscale_target_state);
  }
  // Restored before anything else: the evaluation dispatched on the native
  // command list, and the game's own commands follow ours on this same list.
  RestoreGameDescriptorState(cmd_list);
  if (convert_color) {
    cmd_list->barrier(conversion_slots[0]->texture, reshade::api::resource_usage::shader_resource, reshade::api::resource_usage::unordered_access);
    if (!NVSDK_NGX_FAILED(result)) {
      const auto stage = reshade::api::shader_stage::all_compute;
      const reshade::api::resource output_resource{reinterpret_cast<uint64_t>(eval.Feature.pInOutput)};
      const auto output_state = (game_upscale_mode ? game_upscale_target_state : reshade::api::resource_usage::unordered_access);
      if (output_state != reshade::api::resource_usage::shader_resource_non_pixel) {
        cmd_list->barrier(output_resource, output_state, reshade::api::resource_usage::shader_resource_non_pixel);
      }
      cmd_list->bind_pipeline(reshade::api::pipeline_stage::all_compute, color_conversion.pipeline);
      cmd_list->bind_descriptor_tables(stage, color_conversion.layout, 0u, 2u, conversion_slots[1]->tables);
      const uint32_t constants[] = {target_width, target_height, 0u};
      cmd_list->push_constants(stage, color_conversion.layout, 2u, 0u, 3u, constants);
      cmd_list->dispatch((target_width + 7u) / 8u, (target_height + 7u) / 8u, 1u);
      cmd_list->barrier(conversion_slots[1]->texture, reshade::api::resource_usage::unordered_access, reshade::api::resource_usage::copy_source);
      cmd_list->barrier(output_resource, reshade::api::resource_usage::shader_resource_non_pixel, reshade::api::resource_usage::copy_dest);
      command_list->CopyResource(eval.Feature.pInOutput, reinterpret_cast<ID3D12Resource*>(conversion_slots[1]->texture.handle));
      cmd_list->barrier(output_resource, reshade::api::resource_usage::copy_dest, output_state);
      cmd_list->barrier(conversion_slots[1]->texture, reshade::api::resource_usage::copy_source, reshade::api::resource_usage::unordered_access);
      RestoreGameDescriptorState(cmd_list);
      conversion_slots[1]->ticket = RetireGpuCommands(cmd_list);
    }
  }
  if (NVSDK_NGX_FAILED(result)) {
    ngx.eval_failed = true;

    return false;
  }
  ngx.reset = false;
  last_upscaler_eval_frame.store(frame_counter, std::memory_order_relaxed);
  auto* rcas_source = eval.Feature.pInOutput;
  const D3D12_RESOURCE_DESC rcas_source_desc = (rcas_source != nullptr ? rcas_source->GetDesc() : D3D12_RESOURCE_DESC{});
  if (dlss_rcas_strength > 0.f && rcas_source != nullptr
      && rcas_source_desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D
      && rcas_source_desc.Format == DXGI_FORMAT_R16G16B16A16_FLOAT
      && rcas_source_desc.Width == target_width && rcas_source_desc.Height == target_height
      && rcas_source_desc.DepthOrArraySize == 1u && rcas_source_desc.MipLevels == 1u && rcas_source_desc.SampleDesc.Count == 1u) {
    std::unique_lock lock(rcas.mutex, std::try_to_lock);
    if (lock.owns_lock()) {
      if (rcas.device == nullptr) rcas.device = device;
      if (rcas.device == device && !rcas.pipeline_attempted) {
        rcas.pipeline_attempted = true;
        using DS = reshade::api::shader_stage;
        using DT = reshade::api::descriptor_type;
        using PT = reshade::api::pipeline_layout_param_type;
        reshade::api::descriptor_range srv = {0, 0, 0, 1, DS::all_compute, 1, DT::texture_shader_resource_view};
        reshade::api::descriptor_range uav = {0, 0, 0, 1, DS::all_compute, 1, DT::texture_unordered_access_view};
        reshade::api::constant_range constants = {};
        constants.count = 3u;
        constants.visibility = DS::all_compute;
        reshade::api::pipeline_layout_param params[3] = {};
        params[0].type = PT::descriptor_table;
        params[0].descriptor_table = {1u, &srv};
        params[1].type = PT::descriptor_table;
        params[1].descriptor_table = {1u, &uav};
        params[2].type = PT::push_constants;
        params[2].push_constants = constants;
        if (device->create_pipeline_layout(3u, params, &rcas.layout)) {
          reshade::api::shader_desc shader = {};
          shader.code = __dlss_rcas.data();
          shader.code_size = __dlss_rcas.size();
          shader.entry_point = "main";
          const reshade::api::pipeline_subobject subobject = {reshade::api::pipeline_subobject_type::compute_shader, 1u, &shader};
          (void)device->create_pipeline(rcas.layout, 1u, &subobject, &rcas.pipeline);
        }
      }
      RcasSlot* slot = nullptr;
      if (rcas.device == device && rcas.pipeline.handle != 0u) {
        std::shared_lock retirement_lock(gpu_retirement.mutex);
        for (auto& candidate : rcas.slots) {
          if (!candidate.ticket || candidate.ticket->complete) {
            slot = &candidate;
            break;
          }
        }
      }
      if (slot != nullptr) {
        slot->ticket.reset();
        const uint64_t source = reinterpret_cast<uint64_t>(rcas_source);
        if (slot->source != source && slot->source_view.handle != 0u) {
          device->destroy_resource_view(slot->source_view);
          slot->source_view = {};
          slot->source = 0u;
        }
        if ((slot->width != target_width || slot->height != target_height) && slot->texture.handle != 0u) {
          if (slot->target_view.handle != 0u) device->destroy_resource_view(slot->target_view);
          device->destroy_resource(slot->texture);
          slot->texture = {};
          slot->target_view = {};
          slot->width = slot->height = 0u;
        }
        const reshade::api::resource_view_desc view_desc(
            reshade::api::resource_view_type::texture_2d, reshade::api::format::r16g16b16a16_float, 0u, 1u, 0u, 1u);
        if (slot->source_view.handle == 0u
            && device->create_resource_view(reshade::api::resource{source}, reshade::api::resource_usage::shader_resource,
                                            view_desc, &slot->source_view)) {
          slot->source = source;
        }
        if (slot->texture.handle == 0u) {
          reshade::api::resource_desc desc = {};
          desc.type = reshade::api::resource_type::texture_2d;
          desc.texture = {target_width, target_height, 1u, 1u, reshade::api::format::r16g16b16a16_float, 1u};
          desc.heap = reshade::api::memory_heap::gpu_only;
          desc.usage = reshade::api::resource_usage::unordered_access | reshade::api::resource_usage::copy_source;
          if (device->create_resource(desc, nullptr, reshade::api::resource_usage::unordered_access, &slot->texture)) {
            slot->width = target_width;
            slot->height = target_height;
          }
        }
        if (slot->texture.handle != 0u && slot->target_view.handle == 0u) {
          (void)device->create_resource_view(slot->texture, reshade::api::resource_usage::unordered_access, view_desc, &slot->target_view);
        }
        for (uint32_t i = 0u; i < 2u; ++i) {
          if (slot->tables[i].handle == 0u) (void)device->allocate_descriptor_table(rcas.layout, i, &slot->tables[i]);
        }
        if (slot->source_view.handle && slot->target_view.handle && slot->tables[0].handle && slot->tables[1].handle) {
          const reshade::api::descriptor_table_update updates[] = {
              {slot->tables[0], 0u, 0u, 1u, reshade::api::descriptor_type::texture_shader_resource_view, &slot->source_view},
              {slot->tables[1], 0u, 0u, 1u, reshade::api::descriptor_type::texture_unordered_access_view, &slot->target_view},
          };
          device->update_descriptor_tables(2u, updates);
          const auto stage = reshade::api::shader_stage::all_compute;
          cmd_list->bind_pipeline(reshade::api::pipeline_stage::all_compute, rcas.pipeline);
          cmd_list->bind_descriptor_tables(stage, rcas.layout, 0u, 2u, slot->tables);
          const struct { uint32_t width, height; float strength; } constants = {
              target_width, target_height, (std::clamp)(dlss_rcas_strength, 0.f, 1.f)};
          cmd_list->push_constants(stage, rcas.layout, 2u, 0u, 3u, &constants);
          const reshade::api::resource source_resource{source};
          const auto source_state = (game_upscale_mode ? game_upscale_target_state : reshade::api::resource_usage::unordered_access);
          if (source_state != reshade::api::resource_usage::shader_resource_non_pixel) {
            cmd_list->barrier(source_resource, source_state, reshade::api::resource_usage::shader_resource_non_pixel);
          }
          cmd_list->dispatch((target_width + 7u) / 8u, (target_height + 7u) / 8u, 1u);
          cmd_list->barrier(slot->texture, reshade::api::resource_usage::unordered_access, reshade::api::resource_usage::copy_source);
          cmd_list->barrier(source_resource, reshade::api::resource_usage::shader_resource_non_pixel, reshade::api::resource_usage::copy_dest);
          command_list->CopyResource(rcas_source, reinterpret_cast<ID3D12Resource*>(slot->texture.handle));
          cmd_list->barrier(source_resource, reshade::api::resource_usage::copy_dest, source_state);
          cmd_list->barrier(slot->texture, reshade::api::resource_usage::copy_source, reshade::api::resource_usage::unordered_access);
          RestoreGameDescriptorState(cmd_list);
          slot->ticket = RetireGpuCommands(cmd_list);
          rcas.dispatched.fetch_add(1u, std::memory_order_relaxed);
        } else {
          rcas.skipped.fetch_add(1u, std::memory_order_relaxed);
        }
      } else {
        rcas.skipped.fetch_add(1u, std::memory_order_relaxed);
      }
    } else {
      rcas.skipped.fetch_add(1u, std::memory_order_relaxed);
    }
  } else if (dlss_rcas_strength > 0.f) {
    rcas.skipped.fetch_add(1u, std::memory_order_relaxed);
  }
  // The game's dispatch is only taken out for frames DLSS actually wrote.
  if (game_upscale_mode) game_upscale_wrote_this_frame.store(true);

  const uint64_t fg_eval_frame = frame_counter;
  sl::Constants fg_camera = {};
  bool fg_camera_valid = false;
  if (fg_camera_capture_enabled.load(std::memory_order_acquire) && fg_eval_frame != 0u) {
    fg_camera_eval_frame.store(fg_eval_frame, std::memory_order_relaxed);
    const std::lock_guard<std::mutex> lock(fg_camera_sample_mutex);
    auto& evaluation = fg_camera_evaluations[fg_eval_frame % fg_camera_evaluations.size()];
    if (evaluation.frame == fg_eval_frame && evaluation.token != nullptr) {
      // Multiple evaluations in one CPU interval cannot safely share one FG token.
      evaluation.conflicting_queue = true;
    } else {
      evaluation = {
          .frame = fg_eval_frame,
          .jitter_x = jitter_x,
          .jitter_y = jitter_y,
          .depth_inverted = true,
          .motion_jittered = false,
          .reset = eval.InReset != 0,
          .halton = builder_halton,
          .color = reinterpret_cast<uint64_t>(ngx.output_texture.Get()),
          .motion = reinterpret_cast<uint64_t>(motion_vectors),
          .depth = reinterpret_cast<uint64_t>(depth),
          .render_width = input_width,
          .render_height = input_height,
          .output_width = target_width,
          .output_height = target_height,
      };
    }
    const auto& current = fg_camera_samples[fg_eval_frame % fg_camera_samples.size()];
    if (current.frame == fg_eval_frame) {
      fg_camera_valid = BuildFrameGenerationCameraFromProvider(current, jitter_x, jitter_y,
                                                               true, false,
                                                               eval.InReset != 0, &fg_camera);
      if (fg_camera_valid) {
        evaluation.camera = fg_camera;
        evaluation.camera_valid = true;
      }
      if (!fg_camera_valid) fg_camera_invalid_data.fetch_add(1u, std::memory_order_relaxed);
    } else {
      fg_camera_missing_frame.fetch_add(1u, std::memory_order_relaxed);
    }
  }

  if (fg_camera_capture_enabled.load(std::memory_order_relaxed)) {
    (fg_camera_valid ? fg_camera_valid_evals : fg_camera_unavailable_evals).fetch_add(1u, std::memory_order_relaxed);
  }

  // Both source buffers may be reused, so the tags must be volatile.
  // FG remains off, so Streamline does not need to retain their contents yet.
  if (FgInputsEnabled() && fg_camera_capture_enabled.load(std::memory_order_acquire) && fg_camera_valid
      && !game_upscale_mode
      && camera_builder_frame.load(std::memory_order_acquire) == fg_eval_frame
      && depth_state != reshade::api::resource_usage::undefined
      && fg_input_attempt_frame.exchange(fg_eval_frame, std::memory_order_relaxed) != fg_eval_frame) {
    std::lock_guard frame_lock(farcry6_dlss::streamline::frame_api_mutex);
    auto& streamline = farcry6_dlss::streamline::runtime;
    if (farcry6_dlss::streamline::factory_hook_active.load(std::memory_order_acquire)
        && !fg_swapchain_transition.load(std::memory_order_acquire)) {
      sl::FrameToken* token = nullptr;
      const uint32_t frame_index = static_cast<uint32_t>(fg_eval_frame);
      sl::Result fg_result = streamline.get_frame_token(token, &frame_index);
      if (fg_result == sl::Result::eOk && token == nullptr) fg_result = sl::Result::eErrorInvalidParameter;
      if (fg_result == sl::Result::eOk && token != nullptr) {
        const sl::ViewportHandle viewport(0u);
        fg_result = streamline.set_constants(fg_camera, *token, viewport);
        if (fg_result == sl::Result::eOk) {
          const auto read_state = reshade::api::resource_usage::shader_resource_non_pixel;
          if (depth_state != read_state) cmd_list->barrier(depth_resource, depth_state, read_state);
          sl::Resource depth_tag_resource(sl::ResourceType::eTex2d, depth, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
          sl::Resource motion_tag_resource(sl::ResourceType::eTex2d, motion_vectors,
                                           D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
          const sl::Extent extent{0u, 0u, input_width, input_height};
          sl::ResourceTag tags[] = {
              {&depth_tag_resource, sl::kBufferTypeDepth, sl::ResourceLifecycle::eOnlyValidNow, &extent},
              {&motion_tag_resource, sl::kBufferTypeMotionVectors, sl::ResourceLifecycle::eOnlyValidNow, &extent},
          };
          fg_result = streamline.set_tag(*token, viewport, tags, 2u, command_list);
          if (depth_state != read_state) cmd_list->barrier(depth_resource, read_state, depth_state);
          RestoreGameDescriptorState(cmd_list);
        }
      }
      fg_input_last_result.store(static_cast<int>(fg_result), std::memory_order_relaxed);
      if (fg_result == sl::Result::eOk && token != nullptr) {
        bool recorded = false;
        {
          const std::lock_guard<std::mutex> lock(fg_camera_sample_mutex);
          auto& evaluation = fg_camera_evaluations[fg_eval_frame % fg_camera_evaluations.size()];
          if (evaluation.frame == fg_eval_frame && camera_builder_frame.load(std::memory_order_acquire) == fg_eval_frame) {
            evaluation.token = token;
            evaluation.input_list = cmd_list;
            fg_input_recording_list.store(cmd_list, std::memory_order_release);
            fg_input_tagged.fetch_add(1u, std::memory_order_relaxed);
            recorded = true;
          }
        }
        if (!recorded) {
          const sl::ResourceTag clear_tags[] = {
              {nullptr, sl::kBufferTypeDepth, sl::ResourceLifecycle::eValidUntilPresent},
              {nullptr, sl::kBufferTypeMotionVectors, sl::ResourceLifecycle::eValidUntilPresent},
              {nullptr, sl::kBufferTypeHUDLessColor, sl::ResourceLifecycle::eValidUntilPresent},
          };
          (void)streamline.set_tag(*token, sl::ViewportHandle(0u), clear_tags, 3u, nullptr);
          fg_present_unpaired.fetch_add(1u, std::memory_order_relaxed);
        }
      } else {
        fg_input_failed.fetch_add(1u, std::memory_order_relaxed);
        if (token != nullptr) {
          const sl::ResourceTag clear_tags[] = {
              {nullptr, sl::kBufferTypeDepth, sl::ResourceLifecycle::eValidUntilPresent},
              {nullptr, sl::kBufferTypeMotionVectors, sl::ResourceLifecycle::eValidUntilPresent},
              {nullptr, sl::kBufferTypeHUDLessColor, sl::ResourceLifecycle::eValidUntilPresent},
          };
          (void)streamline.set_tag(*token, sl::ViewportHandle(0u), clear_tags, 3u, nullptr);
        }
      }
    }
  }

  // The full-resolution scene pass consumes our output on the same command list.
  // Keep the game TAA draw intact until that pass is reached.

  return true;
}

inline void OnExecuteCommandList(reshade::api::command_queue* queue, reshade::api::command_list* cmd_list) {
  if (queue != nullptr && cmd_list != nullptr
      && fg_input_recording_list.load(std::memory_order_acquire) == cmd_list) {
    std::unique_lock lock(fg_camera_sample_mutex, std::try_to_lock);
    if (lock.owns_lock()) {
      auto& evaluation = fg_camera_evaluations[camera_builder_frame.load(std::memory_order_acquire) % fg_camera_evaluations.size()];
      if (evaluation.frame == camera_builder_frame.load(std::memory_order_relaxed)
          && evaluation.token != nullptr && evaluation.input_list == cmd_list) {
        if (evaluation.submit_queue == nullptr) {
          evaluation.submit_queue = queue;
          fg_input_submitted.fetch_add(1u, std::memory_order_relaxed);
        } else if (evaluation.submit_queue != queue) {
          evaluation.conflicting_queue = true;
        }
      }
    } else {
      fg_input_queue_skips.fetch_add(1u, std::memory_order_relaxed);
    }
  }
  if (queue != nullptr && cmd_list != nullptr) {
    auto* data = cmd_list->get_private_data<CommandListData>();
    if (data != nullptr && data->dsv.handle != 0u && data->dsv_draws != 0u) {
      FlushDepthDraws(queue->get_device(), data);
    }
  }
  if (cmd_list != nullptr && cmd_list->get_private_data<CommandListData>() != nullptr
      && cmd_list->get_private_data<CommandListData>()->gpu_tracked) {
    std::unique_lock lock(gpu_retirement.mutex);
    if (gpu_retirement.recorded_lists.contains(cmd_list)) {
      const DWORD thread = GetCurrentThreadId();
      if (gpu_retirement.queue == nullptr) {
        gpu_retirement.queue = queue;
        gpu_retirement.submit_thread = thread;
      } else if (gpu_retirement.queue != queue || gpu_retirement.submit_thread != thread) {
        gpu_retirement.conflicting_submission = true;
      }
      for (auto& weak : gpu_retirement.tickets) {
        if (auto ticket = weak.lock(); ticket && ticket->pending_lists.contains(cmd_list)) {
          if (ticket->queue == nullptr) {
            ticket->queue = queue;
            ticket->submit_thread = thread;
          } else if (ticket->queue != queue || ticket->submit_thread != thread) {
            ticket->conflicting_submission = true;
          }
        }
      }
    }
  }

}

inline void OnResetCommandList(reshade::api::command_list* cmd_list) {
  if (auto* data = cmd_list->get_private_data<CommandListData>(); data != nullptr && data->gpu_tracked) {
    std::unique_lock lock(gpu_retirement.mutex);
    gpu_retirement.recorded_lists.erase(cmd_list);
    for (auto& weak : gpu_retirement.tickets) {
      if (auto ticket = weak.lock()) ticket->pending_lists.erase(cmd_list);
    }
    data->gpu_tracked = false;
  }
  if (auto* data = cmd_list->get_private_data<CommandListData>()) {
    data->in_render_pass = false;
    data->coverage_pass_dsv = {};
    data->dsv = {};
    data->dsv_draws = 0u;
  }
}

inline void OnInitCommandList(reshade::api::command_list* cmd_list) { cmd_list->create_private_data<CommandListData>(); }
inline void OnDestroyCommandList(reshade::api::command_list* cmd_list) {
  OnResetCommandList(cmd_list);
  cmd_list->destroy_private_data<CommandListData>();
}

inline void OnBeginRenderPass(reshade::api::command_list* cmd_list, uint32_t count,
                              const reshade::api::render_pass_render_target_desc* targets,
                              const reshade::api::render_pass_depth_stencil_desc* depth) {
  if (auto* data = Get(cmd_list)) {
    data->in_render_pass = true;
    data->coverage_pass_dsv = (depth != nullptr && depth->view.handle != 0u
                                  ? cmd_list->get_device()->get_resource_from_view(depth->view)
                                  : reshade::api::resource{});
  }
}

inline void OnEndRenderPass(reshade::api::command_list* cmd_list) {
  if (auto* data = Get(cmd_list)) {
    data->in_render_pass = false;
    data->coverage_pass_dsv = {};
  }
}

// Any pipeline bind may change the pixel shader: mark the memoized hash dirty.
// Never creates private data.
inline void OnBindPipeline(
    reshade::api::command_list* cmd_list, reshade::api::pipeline_stage, reshade::api::pipeline) {
  if (cmd_list == nullptr) return;
  if (auto* data = cmd_list->get_private_data<CommandListData>()) {
    data->ps_dirty = true;
  }
}

inline void OnBindDescriptorTables(
    reshade::api::command_list* cmd_list, reshade::api::shader_stage stages,
    reshade::api::pipeline_layout, uint32_t first, uint32_t count,
    const reshade::api::descriptor_table* tables) {
  if (dlss_enabled == 0.f || !any_gpu.load()) return;
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
  if (dlss_enabled == 0.f || !any_gpu.load()) return;
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

// Replace the scene upscale draw with the DLSS image. The
// original TAA draw remains available as a fallback if this exact handoff fails.
inline bool ReplaceSceneUpscale(reshade::api::command_list* cmd_list) {
  if (cmd_list == nullptr || PauseTaaRecentlySeen()) return false;
  auto* data = cmd_list->get_private_data<CommandListData>();
  if (data == nullptr || data->dlss_frame != frame_counter || data->dlss_taa_resource == 0u) return false;
  auto* shader_state = renodx::utils::shader::GetCurrentState(cmd_list);
  if (shader_state == nullptr || renodx::utils::shader::GetCurrentPixelShaderHash(shader_state) != kSceneUpscaleHash) return false;
  SlotInput input;
  if (!ResolveSlot(cmd_list, data, 0u, input) || input.resource.handle != data->dlss_taa_resource) {
    return false;
  }
  auto* target = GetNativeResource(cmd_list->get_device(), data->rtvs[0]);
  auto* source = ngx.output_texture.Get();
  if (target == nullptr || source == nullptr) {
    return false;
  }
  const auto dst = target->GetDesc();
  const auto src = source->GetDesc();
  if (dst.Dimension != src.Dimension || dst.Width != src.Width || dst.Height != src.Height
      || dst.Format != src.Format || dst.DepthOrArraySize != src.DepthOrArraySize
      || dst.MipLevels != src.MipLevels || dst.SampleDesc.Count != src.SampleDesc.Count) {
    return false;
  }
  auto* native = reinterpret_cast<ID3D12GraphicsCommandList*>(cmd_list->get_native());

  const bool copied = CopyThroughTaaTarget(cmd_list, native, target, source, reshade::api::resource_usage::unordered_access);

  if (!copied) return false;
  fg_last_scene_handoff_frame.store(frame_counter, std::memory_order_release);
  if (output_settings != nullptr && FgInputsEnabled() && fg_camera_capture_enabled.load(std::memory_order_acquire)
      && !fg_hudless_retire_requested.load(std::memory_order_acquire)
      && !data->in_render_pass && src.Format == DXGI_FORMAT_R16G16B16A16_FLOAT && src.Width <= UINT32_MAX) {
    const uint64_t frame = camera_builder_frame.load(std::memory_order_acquire);
    const uint64_t brightness_frame = fg_hdr_parameter_frame.load(std::memory_order_acquire);
    const float brightness = fg_hdr_brightness.load(std::memory_order_relaxed);
    const auto output_space = fg_output_color_space.load(std::memory_order_relaxed);
    const bool is_sdr = output_space == reshade::api::color_space::srgb_nonlinear;
    const bool is_hdr10 = output_space == reshade::api::color_space::hdr10_st2084;
    const bool is_scrgb = output_space == reshade::api::color_space::extended_srgb_linear
                       && fg_output_format.load(std::memory_order_relaxed) == static_cast<uint32_t>(reshade::api::format::r16g16b16a16_float);
    bool input_ready = false;
    {
      const std::lock_guard<std::mutex> lock(fg_camera_sample_mutex);
      const auto& evaluation = fg_camera_evaluations[frame % fg_camera_evaluations.size()];
      input_ready = evaluation.frame == frame && evaluation.token != nullptr && evaluation.input_list == cmd_list;
    }
    if (input_ready && (is_sdr || ((is_hdr10 || is_scrgb) && brightness_frame != UINT64_MAX
        && (brightness_frame == frame || brightness_frame + 1u == frame) && std::isfinite(brightness) && brightness > 0.f))) {
      std::unique_lock lock(fg_hudless.mutex, std::try_to_lock);
      if (lock.owns_lock()) {
        auto* device = cmd_list->get_device();
        if (is_scrgb && fg_hudless.pipeline_attempted
            && fg_hudless.output_format != reshade::api::format::r16g16b16a16_float) {
          fg_hudless_retire_requested.store(true, std::memory_order_release);
        } else {
          if (fg_hudless.device == nullptr) fg_hudless.device = device;
          if (fg_hudless.device == device && !fg_hudless.pipeline_attempted) {
            fg_hudless.pipeline_attempted = true;
            fg_hudless_slots_allocated.store(true, std::memory_order_release);
            D3D12_FEATURE_DATA_FORMAT_SUPPORT format_support = {};
            format_support.Format = DXGI_FORMAT_R10G10B10A2_UNORM;
            auto* native_device = reinterpret_cast<ID3D12Device*>(device->get_native());
            const bool packed_supported = !is_scrgb && native_device != nullptr
                && SUCCEEDED(native_device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &format_support, sizeof(format_support)))
                && (format_support.Support1 & D3D12_FORMAT_SUPPORT1_TYPED_UNORDERED_ACCESS_VIEW) != 0u
                && (format_support.Support2 & D3D12_FORMAT_SUPPORT2_UAV_TYPED_STORE) != 0u;
            fg_hudless.output_format = (packed_supported ? reshade::api::format::r10g10b10a2_unorm
                                                         : reshade::api::format::r16g16b16a16_float);
            fg_hudless.slot_limit = (packed_supported ? 12u : 6u);
            using DS = reshade::api::shader_stage;
            using DT = reshade::api::descriptor_type;
            using PT = reshade::api::pipeline_layout_param_type;
            reshade::api::descriptor_range srv = {0, 0, 0, 1, DS::all_compute, 1, DT::texture_shader_resource_view};
            reshade::api::descriptor_range uav = {0, 0, 0, 1, DS::all_compute, 1, DT::texture_unordered_access_view};
            reshade::api::constant_range constants = {};
            constants.count = 4u;
            constants.visibility = DS::all_compute;
            reshade::api::pipeline_layout_param params[3] = {};
            params[0].type = PT::descriptor_table;
            params[0].descriptor_table = {1u, &srv};
            params[1].type = PT::descriptor_table;
            params[1].descriptor_table = {1u, &uav};
            params[2].type = PT::push_constants;
            params[2].push_constants = constants;
            if (device->create_pipeline_layout(3u, params, &fg_hudless.layout)) {
              reshade::api::shader_desc shader = {};
              shader.code = __fg_hdr10_convert.data();
              shader.code_size = __fg_hdr10_convert.size();
              shader.entry_point = "main";
              const reshade::api::pipeline_subobject subobject = {reshade::api::pipeline_subobject_type::compute_shader, 1u, &shader};
              (void)device->create_pipeline(fg_hudless.layout, 1u, &subobject, &fg_hudless.pipeline);
            }
          }
          uint32_t slot_index = UINT32_MAX;
          if (fg_hudless.device == device && fg_hudless.pipeline.handle != 0u) {
            std::shared_lock retirement_lock(gpu_retirement.mutex);
            for (uint32_t i = 0u; i < fg_hudless.slot_limit; ++i) {
              if (!fg_hudless.slots[i].ticket || fg_hudless.slots[i].ticket->complete) {
                if (fg_hudless.slots[i].ticket) {
                  fg_completed_input_on_gpu.store(true, std::memory_order_release);
                }
                slot_index = i;
                break;
              }
            }
          }
          if (slot_index != UINT32_MAX) {
            auto& slot = fg_hudless.slots[slot_index];
            slot.ticket.reset();
            if (slot.source != reinterpret_cast<uint64_t>(source) && slot.source_view.handle) {
              device->destroy_resource_view(slot.source_view);
              slot.source_view = {};
              slot.source = 0u;
            }
            if ((slot.width != src.Width || slot.height != src.Height) && slot.texture.handle) {
              if (slot.target_view.handle) device->destroy_resource_view(slot.target_view);
              device->destroy_resource(slot.texture);
              slot.target_view = {};
              slot.texture = {};
              slot.width = slot.height = 0u;
            }
            const reshade::api::resource_view_desc source_view_desc(
                reshade::api::resource_view_type::texture_2d, reshade::api::format::r16g16b16a16_float, 0u, 1u, 0u, 1u);
            const reshade::api::resource_view_desc target_view_desc(
                reshade::api::resource_view_type::texture_2d, fg_hudless.output_format, 0u, 1u, 0u, 1u);
            if (slot.source_view.handle == 0u
                && device->create_resource_view(reshade::api::resource{reinterpret_cast<uint64_t>(source)},
                                                reshade::api::resource_usage::shader_resource, source_view_desc, &slot.source_view)) {
              slot.source = reinterpret_cast<uint64_t>(source);
            }
            if (slot.texture.handle == 0u) {
              reshade::api::resource_desc resource_desc = {};
              resource_desc.type = reshade::api::resource_type::texture_2d;
              resource_desc.texture = {static_cast<uint32_t>(src.Width), src.Height, 1u, 1u, fg_hudless.output_format, 1u};
              resource_desc.heap = reshade::api::memory_heap::gpu_only;
              resource_desc.usage = reshade::api::resource_usage::shader_resource | reshade::api::resource_usage::unordered_access;
              if (device->create_resource(resource_desc, nullptr, reshade::api::resource_usage::shader_resource, &slot.texture)) {
                slot.width = static_cast<uint32_t>(src.Width);
                slot.height = src.Height;
              }
            }
            if (slot.texture.handle && slot.target_view.handle == 0u) {
              (void)device->create_resource_view(slot.texture, reshade::api::resource_usage::unordered_access, target_view_desc, &slot.target_view);
            }
            for (uint32_t i = 0u; i < 2u; ++i) {
              if (slot.tables[i].handle == 0u) (void)device->allocate_descriptor_table(fg_hudless.layout, i, &slot.tables[i]);
            }
            if (slot.source_view.handle || slot.target_view.handle || slot.texture.handle
                || slot.tables[0].handle || slot.tables[1].handle) {
              fg_hudless_slots_allocated.store(true, std::memory_order_release);
            }
            if (slot.source_view.handle && slot.target_view.handle && slot.tables[0].handle && slot.tables[1].handle) {
              const reshade::api::descriptor_table_update updates[] = {
                  {slot.tables[0], 0u, 0u, 1u, reshade::api::descriptor_type::texture_shader_resource_view, &slot.source_view},
                  {slot.tables[1], 0u, 0u, 1u, reshade::api::descriptor_type::texture_unordered_access_view, &slot.target_view},
              };
              device->update_descriptor_tables(2u, updates);
              const auto stage = reshade::api::shader_stage::all_compute;
              cmd_list->bind_pipeline(reshade::api::pipeline_stage::all_compute, fg_hudless.pipeline);
              cmd_list->bind_descriptor_tables(stage, fg_hudless.layout, 0u, 2u, slot.tables);
              const float tone_map_type = output_settings->tone_map_type;
              const float diffuse_white_nits = output_settings->diffuse_white_nits;
              const float gamma_correction = output_settings->gamma_correction;
              // The conversion shader multiplies this value by two, like the game's output pass.
              const float output_brightness = (tone_map_type == 0.f ? brightness : diffuse_white_nits * 0.5f);
              const struct { uint32_t width, height; float brightness; uint32_t decode_mode; } constants = {
                  slot.width, slot.height, is_sdr ? 0.f : (is_scrgb ? -output_brightness : output_brightness),
                  tone_map_type == 0.f ? 0u : (gamma_correction != 0.f ? 1u : 2u)};
              cmd_list->push_constants(stage, fg_hudless.layout, 2u, 0u, 4u, &constants);
              const reshade::api::resource source_resource{reinterpret_cast<uint64_t>(source)};
              cmd_list->barrier(source_resource, reshade::api::resource_usage::unordered_access, reshade::api::resource_usage::shader_resource_non_pixel);
              cmd_list->barrier(slot.texture, reshade::api::resource_usage::shader_resource, reshade::api::resource_usage::unordered_access);
              cmd_list->dispatch((slot.width + 7u) / 8u, (slot.height + 7u) / 8u, 1u);
              cmd_list->barrier(slot.texture, reshade::api::resource_usage::unordered_access, reshade::api::resource_usage::shader_resource);
              cmd_list->barrier(source_resource, reshade::api::resource_usage::shader_resource_non_pixel, reshade::api::resource_usage::unordered_access);
              RestoreGameDescriptorState(cmd_list);
              slot.ticket = RetireGpuCommands(cmd_list);
              const std::lock_guard<std::mutex> sample_lock(fg_camera_sample_mutex);
              auto& evaluation = fg_camera_evaluations[frame % fg_camera_evaluations.size()];
              if (evaluation.frame == frame && evaluation.input_list == cmd_list) {
                evaluation.hdr_slot = slot_index;
                evaluation.hdr_brightness = (is_sdr ? 0.f : brightness);
                evaluation.tone_map_type = tone_map_type;
                evaluation.diffuse_white_nits = diffuse_white_nits;
                evaluation.gamma_correction = gamma_correction;
                evaluation.output_color_space = output_space;
                evaluation.scene_output = reinterpret_cast<uint64_t>(target);
                fg_hudless_converted.fetch_add(1u, std::memory_order_relaxed);
              }
            } else {
              fg_hudless_rejected.fetch_add(1u, std::memory_order_relaxed);
            }
          } else {
            fg_hudless_busy.fetch_add(1u, std::memory_order_relaxed);
            if (fg_hudless.device == device && fg_hudless.pipeline.handle != 0u) {
              fg_hudless_no_slot.fetch_add(1u, std::memory_order_relaxed);
            }
          }
        }
      } else {
        fg_hudless_busy.fetch_add(1u, std::memory_order_relaxed);
      }
    }
  }
  if (fg_camera_capture_enabled.load(std::memory_order_acquire)) {
    const std::lock_guard<std::mutex> lock(fg_camera_sample_mutex);
    auto& evaluation = fg_camera_evaluations[frame_counter % fg_camera_evaluations.size()];
    if (evaluation.frame == frame_counter && evaluation.color == reinterpret_cast<uint64_t>(source)) {
      evaluation.handoff = true;
    }
  }
  data->dlss_taa_resource = 0u;
  return true;
}

inline void TagFgHudless(reshade::api::command_list* cmd_list, uint32_t output_hash) {
  if (cmd_list == nullptr || !FgInputsEnabled() || !fg_camera_capture_enabled.load(std::memory_order_acquire)) return;
  const auto* data = cmd_list->get_private_data<CommandListData>();
  if (data == nullptr || data->in_render_pass) return;
  const uint64_t frame = camera_builder_frame.load(std::memory_order_acquire);
  FgCameraEvaluation evaluation = {};
  {
    const std::lock_guard<std::mutex> lock(fg_camera_sample_mutex);
    evaluation = fg_camera_evaluations[frame % fg_camera_evaluations.size()];
  }
  const bool is_sdr = evaluation.output_color_space == reshade::api::color_space::srgb_nonlinear;
  const bool is_hdr10 = evaluation.output_color_space == reshade::api::color_space::hdr10_st2084;
  const bool is_scrgb = evaluation.output_color_space == reshade::api::color_space::extended_srgb_linear;
  if (evaluation.frame != frame || evaluation.token == nullptr || evaluation.input_list != cmd_list
      || evaluation.hdr_slot >= fg_hudless.slots.size() || evaluation.hudless_tagged
      || evaluation.output_color_space != fg_output_color_space.load(std::memory_order_relaxed)
      || output_settings == nullptr || evaluation.tone_map_type != output_settings->tone_map_type
      || evaluation.diffuse_white_nits != output_settings->diffuse_white_nits
      || evaluation.gamma_correction != output_settings->gamma_correction
      || (!is_sdr && !is_hdr10 && !is_scrgb) || (is_sdr && output_hash != kSdrOutputHash)
      || ((is_hdr10 || is_scrgb) && ((is_hdr10 ? output_hash != kHdrOutputHash : output_hash != kScRgbOutputHash)
                       || fg_hdr_parameter_frame.load(std::memory_order_acquire) != frame
                       || fg_hdr_brightness.load(std::memory_order_relaxed) != evaluation.hdr_brightness))) {
    fg_hudless_rejected.fetch_add(1u, std::memory_order_relaxed);
    return;
  }
  if (is_sdr) {
    SlotInput input;
    if (!ResolveSlot(cmd_list, data, 0u, input) || input.resource.handle != evaluation.scene_output) {
      fg_hudless_rejected.fetch_add(1u, std::memory_order_relaxed);
      return;
    }
  }
  std::shared_lock lock(fg_hudless.mutex, std::try_to_lock);
  if (!lock.owns_lock()) {
    fg_hudless_busy.fetch_add(1u, std::memory_order_relaxed);
    return;
  }
  const auto& slot = fg_hudless.slots[evaluation.hdr_slot];
  if (slot.texture.handle == 0u || slot.width != evaluation.output_width || slot.height != evaluation.output_height) {
    fg_hudless_rejected.fetch_add(1u, std::memory_order_relaxed);
    return;
  }
  std::lock_guard frame_lock(farcry6_dlss::streamline::frame_api_mutex);
  if (!farcry6_dlss::streamline::factory_hook_active.load(std::memory_order_acquire)
      || fg_swapchain_transition.load(std::memory_order_acquire)) return;
  sl::Resource resource(sl::ResourceType::eTex2d, reinterpret_cast<void*>(slot.texture.handle),
                        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
  const sl::Extent extent{0u, 0u, slot.width, slot.height};
  const sl::ResourceTag tag(&resource, sl::kBufferTypeHUDLessColor, sl::ResourceLifecycle::eOnlyValidNow, &extent);
  const auto result = farcry6_dlss::streamline::runtime.set_tag(
      *evaluation.token, sl::ViewportHandle(0u), &tag, 1u,
      reinterpret_cast<ID3D12GraphicsCommandList*>(cmd_list->get_native()));
  RestoreGameDescriptorState(cmd_list);
  fg_input_last_result.store(static_cast<int>(result), std::memory_order_relaxed);
  if (result == sl::Result::eOk) {
    const std::lock_guard<std::mutex> sample_lock(fg_camera_sample_mutex);
    auto& current = fg_camera_evaluations[frame % fg_camera_evaluations.size()];
    if (current.frame == frame && current.token == evaluation.token && current.hdr_slot == evaluation.hdr_slot) {
      current.hudless_tagged = true;
      fg_hudless_tagged.fetch_add(1u, std::memory_order_relaxed);
    }
  } else {
    fg_input_failed.fetch_add(1u, std::memory_order_relaxed);
  }
}

inline bool OnDraw(reshade::api::command_list* cmd_list, uint32_t, uint32_t, uint32_t, uint32_t) {

  if (dlss_enabled == 0.f || !any_gpu.load()) return false;
  NoteDrawForDepth(cmd_list);
  if (ReplaceSceneUpscale(cmd_list)) return true;
  CommandListData* data = nullptr;
  uint32_t hash = 0u;
  if (!CheckTaaDraw(cmd_list, data, hash)) {
    if ((hash == kHdrOutputHash || hash == kScRgbOutputHash) && fg_camera_capture_enabled.load(std::memory_order_acquire)) {
      CaptureFgHdrOutputParameters(cmd_list);
      TagFgHudless(cmd_list, hash);
    } else if (hash == kSdrOutputHash && fg_camera_capture_enabled.load(std::memory_order_acquire)) {
      TagFgHudless(cmd_list, hash);
    }
    // The scene's own view constants are read at its geometry draws, which is where
    // the jitter is applied and therefore where it exists as data.

    return false;
  }
  last_taa_draw_frame.store(frame_counter, std::memory_order_relaxed);
  // Keep TAA running as a fallback. A successful evaluation is handed to the
  // matching scene upscale draw or the game's game upscale dispatch.
  data->dlss_taa_resource = 0u;
  if (EvaluateDLSS(cmd_list, data, hash) && !game_upscale_active.load()
      && hash == kTaaMainHash) {
    data->dlss_taa_resource = cmd_list->get_device()->get_resource_from_view(data->rtvs[0]).handle;
    data->dlss_frame = frame_counter;
  }
  return false;
}

inline bool OnDrawIndexed(reshade::api::command_list* cmd_list, uint32_t, uint32_t, uint32_t, int32_t, uint32_t) {

  if (dlss_enabled == 0.f || !any_gpu.load()) return false;
  NoteDrawForDepth(cmd_list);
  if (ReplaceSceneUpscale(cmd_list)) return true;
  CommandListData* data = nullptr;
  uint32_t hash = 0u;
  if (!CheckTaaDraw(cmd_list, data, hash)) {
    if ((hash == kHdrOutputHash || hash == kScRgbOutputHash) && fg_camera_capture_enabled.load(std::memory_order_acquire)) {
      CaptureFgHdrOutputParameters(cmd_list);
      TagFgHudless(cmd_list, hash);
    } else if (hash == kSdrOutputHash && fg_camera_capture_enabled.load(std::memory_order_acquire)) {
      TagFgHudless(cmd_list, hash);
    }
    // The scene's own view constants are read at its geometry draws, which is where
    // the jitter is applied and therefore where it exists as data.

    return false;
  }
  last_taa_draw_frame.store(frame_counter, std::memory_order_relaxed);
  // Keep TAA running as a fallback. A successful evaluation is handed to the
  // matching scene upscale draw or the game's game upscale dispatch.
  data->dlss_taa_resource = 0u;
  if (EvaluateDLSS(cmd_list, data, hash) && !game_upscale_active.load()
      && hash == kTaaMainHash) {
    data->dlss_taa_resource = cmd_list->get_device()->get_resource_from_view(data->rtvs[0]).handle;
    data->dlss_frame = frame_counter;
  }
  return false;
}

inline void OnPresent(
    reshade::api::command_queue* queue, reshade::api::swapchain* swapchain,
    const reshade::api::rect*, const reshade::api::rect*,
    uint32_t, const reshade::api::rect*) {

  if (queue != nullptr && swapchain != nullptr && queue->get_device() != nullptr
      && reinterpret_cast<void*>(queue->get_device()->get_native())
          == farcry6_dlss::streamline::bootstrap_device.load(std::memory_order_acquire)) {
    fg_output_color_space.store(swapchain->get_color_space(), std::memory_order_relaxed);
    const auto back_buffer = swapchain->get_current_back_buffer();
    if (back_buffer.handle != 0u) {
      fg_output_format.store(static_cast<uint32_t>(queue->get_device()->get_resource_desc(back_buffer).texture.format),
                             std::memory_order_relaxed);
    }
  }
  if (queue != nullptr) {
    {
      std::unique_lock lock(gpu_retirement.mutex);
      for (auto it = gpu_retirement.tickets.begin(); it != gpu_retirement.tickets.end();) {
        auto ticket = it->lock();
        if (!ticket) {
          it = gpu_retirement.tickets.erase(it);
          continue;
        }
        ++it;
        if (ticket->complete || !ticket->pending_lists.empty() || ticket->conflicting_submission) continue;
        // An unobserved submission cannot establish a safe fence point.
        if (ticket->queue == nullptr) continue;
        if (ticket->queue != queue || ticket->submit_thread != GetCurrentThreadId()) continue;
        try {
          if (gpu_retirement.fence == nullptr) {
            gpu_retirement.fence = std::make_unique<renodx::utils::gpu_fence::Fence>(queue->get_device());
          }
          if (ticket->fence_value == renodx::utils::gpu_fence::NO_SIGNAL) {
            ticket->fence_value = renodx::utils::gpu_fence::QueueSignal(queue, gpu_retirement.fence.get());
            if (ticket->fence_value == renodx::utils::gpu_fence::NO_SIGNAL) {
              ++gpu_retirement.fence_failures;
              continue;
            }
          }
          const uint64_t completed = gpu_retirement.fence->GetCompletedValue();
          ticket->complete = completed != UINT64_MAX && completed >= ticket->fence_value;
        } catch (const std::exception&) {
          ++gpu_retirement.fence_failures;
        }
      }
    }
    for (auto it = retired_ngx_features.begin(); it != retired_ngx_features.end();) {
      if (!it->ticket->complete) { ++it; continue; }
      if (it->feature != nullptr) NVSDK_NGX_D3D12_ReleaseFeature(it->feature);
      if (it->parameters != nullptr) NVSDK_NGX_D3D12_DestroyParameters(it->parameters);
      it = retired_ngx_features.erase(it);
      ++gpu_retirement.reclaimed;
    }
    for (auto it = retired_ngx_outputs.begin(); it != retired_ngx_outputs.end();) {
      if (!it->ticket->complete) { ++it; continue; }
      it = retired_ngx_outputs.erase(it);
      ++gpu_retirement.reclaimed;
    }
    for (auto it = retired_unpack.begin(); it != retired_unpack.end();) {
      if (!it->ticket->complete || unpack_device != queue->get_device()) { ++it; continue; }
      auto& generation = it->data;
      if (generation.uav.handle) unpack_device->destroy_resource_view(generation.uav);
      if (generation.srv.handle) unpack_device->destroy_resource_view(generation.srv);
      if (generation.texture.handle) unpack_device->destroy_resource(generation.texture);
      if (generation.pipeline.handle) unpack_device->destroy_pipeline(generation.pipeline);

      for (auto& table : generation.tables) {
        if (table.handle) unpack_device->free_descriptor_table(table);
      }
      if (generation.layout.handle) unpack_device->destroy_pipeline_layout(generation.layout);
      it = retired_unpack.erase(it);
      ++gpu_retirement.reclaimed;
    }
    if (!FgInputsEnabled() && fg_hudless_slots_allocated.load(std::memory_order_acquire)) {
      fg_hudless_retire_requested.store(true, std::memory_order_release);
    }
    if (fg_hudless_retire_requested.load(std::memory_order_acquire)) {
      std::unique_lock hudless_lock(fg_hudless.mutex, std::try_to_lock);
      if (hudless_lock.owns_lock() && (fg_hudless.device == nullptr || fg_hudless.device == queue->get_device())) {
        bool pending = false;
        for (auto& slot : fg_hudless.slots) {
          bool complete = !slot.ticket;
          if (slot.ticket) {
            std::shared_lock retirement_lock(gpu_retirement.mutex);
            complete = slot.ticket->complete;
          }
          if (!complete) {
            pending = true;
            continue;
          }
          auto* device = fg_hudless.device;
          if (device != nullptr) {
            if (slot.source_view.handle) device->destroy_resource_view(slot.source_view);
            if (slot.target_view.handle) device->destroy_resource_view(slot.target_view);
            if (slot.texture.handle) device->destroy_resource(slot.texture);
            for (auto& table : slot.tables) {
              if (table.handle) device->free_descriptor_table(table);
            }
          }
          slot = {};
        }
        if (!pending) {
          if (fg_hudless.device != nullptr) {
            if (fg_hudless.pipeline.handle) fg_hudless.device->destroy_pipeline(fg_hudless.pipeline);
            if (fg_hudless.layout.handle) fg_hudless.device->destroy_pipeline_layout(fg_hudless.layout);
          }
          fg_hudless.pipeline = {};
          fg_hudless.layout = {};
          fg_hudless.pipeline_attempted = false;
          fg_hudless.output_format = reshade::api::format::unknown;
          fg_hudless.slot_limit = 0u;
          fg_hudless.device = nullptr;
          fg_hudless_slots_allocated.store(false, std::memory_order_release);
          fg_hudless_retire_requested.store(false, std::memory_order_release);
        }
      }
    }
  }
  if (queue != nullptr && swapchain != nullptr) {
    const auto desc = queue->get_device()->get_resource_desc(swapchain->get_back_buffer(0));
    display_width.store(desc.texture.width);
    display_height.store(desc.texture.height);
  }
  if (queue != nullptr) SelectSceneDepth(queue->get_device());
  // Whether the game's own upscaler runs is decided by what the last frame showed:
  // with it, DLSS deploys at the upscaler; without it, at the TAA draw.
  game_upscale_active.store(game_upscale_seen_this_frame);
  game_upscale_seen_this_frame = false;
  game_upscale_wrote_this_frame.store(false);

  if (fg_camera_capture_enabled.load(std::memory_order_acquire)) {
    const std::lock_guard<std::mutex> lock(fg_camera_sample_mutex);
    auto& evaluation = fg_camera_evaluations[frame_counter % fg_camera_evaluations.size()];
    const auto& camera = fg_camera_samples[frame_counter % fg_camera_samples.size()];
    if (evaluation.frame == frame_counter) {
      fg_camera_present_frame.store(frame_counter, std::memory_order_relaxed);
      if (camera.frame != frame_counter) {
        fg_camera_present_missing.fetch_add(1u, std::memory_order_relaxed);
      } else {
        sl::Constants constants = {};
        const bool matching_jitter = !evaluation.halton
            || (std::abs(-camera.projection[2] * camera.viewport[0] * 0.5f - evaluation.jitter_x) < 0.01f
                && std::abs(camera.projection[6] * camera.viewport[1] * 0.5f - evaluation.jitter_y) < 0.01f);
        if (matching_jitter && BuildFrameGenerationCameraFromProvider(camera, evaluation.jitter_x, evaluation.jitter_y,
                                                   evaluation.depth_inverted, evaluation.motion_jittered,
                                                   evaluation.reset, &constants)) {
          evaluation.camera = constants;
          evaluation.camera_valid = true;
          fg_camera_present_valid.fetch_add(1u, std::memory_order_relaxed);
          if (!evaluation.handoff || evaluation.color == 0u || evaluation.motion == 0u || evaluation.depth == 0u
              || evaluation.render_width == 0u || evaluation.render_height == 0u
              || evaluation.output_width == 0u || evaluation.output_height == 0u) {
            fg_frame_missing_handoff.fetch_add(1u, std::memory_order_relaxed);
          } else if (evaluation.output_color_space == reshade::api::color_space::hdr10_st2084
                     && fg_hdr_parameter_frame.load(std::memory_order_acquire) != frame_counter) {
            fg_frame_missing_hdr.fetch_add(1u, std::memory_order_relaxed);
          } else {
            fg_frame_matched.fetch_add(1u, std::memory_order_relaxed);
          }
        } else {
          fg_camera_present_invalid.fetch_add(1u, std::memory_order_relaxed);
        }
      }
    }
  }
  FgCameraEvaluation present_input = {};
  if (FgInputsEnabled() || fg_input_recording_list.load(std::memory_order_acquire) != nullptr) {
    const std::lock_guard<std::mutex> lock(fg_camera_sample_mutex);
    const auto& evaluation = fg_camera_evaluations[frame_counter % fg_camera_evaluations.size()];
    if (evaluation.frame == frame_counter) present_input = evaluation;
    fg_input_recording_list.store(nullptr, std::memory_order_release);
  }
  if (present_input.token != nullptr || fg_has_pending_present.load(std::memory_order_acquire)
      || fg_resources_retained.load(std::memory_order_acquire)
      || fg_mode_status.load(std::memory_order_relaxed) != 0) {
    std::lock_guard frame_lock(farcry6_dlss::streamline::frame_api_mutex);
    if (farcry6_dlss::streamline::factory_hook_active.load(std::memory_order_acquire)) {
      auto& runtime = farcry6_dlss::streamline::runtime;
      const sl::ViewportHandle viewport(0u);
      if (fg_resources_retained.load(std::memory_order_relaxed) && fg_mode.ReadyForDisplayChange()
          && (!fg_enable_requested.load(std::memory_order_acquire) || !FgInputsEnabled()
              || fg_swapchain_transition.load(std::memory_order_acquire))
          && runtime.free_resources != nullptr
          && runtime.free_resources(sl::kFeatureDLSS_G, viewport) == sl::Result::eOk) {
        fg_resources_retained.store(false, std::memory_order_release);
        fg_temporary_off_time_ms.store(0u, std::memory_order_release);
      }
      const sl::ResourceTag clear_tags[] = {
          {nullptr, sl::kBufferTypeDepth, sl::ResourceLifecycle::eValidUntilPresent},
          {nullptr, sl::kBufferTypeMotionVectors, sl::ResourceLifecycle::eValidUntilPresent},
          {nullptr, sl::kBufferTypeHUDLessColor, sl::ResourceLifecycle::eValidUntilPresent},
      };
      if (fg_mode_present_outstanding && fg_mode_swapchain == swapchain) {
        fg_mode.PresentCompleted(false);
        fg_applied_generated.store(0u, std::memory_order_release);
        fg_pending_generated = 0u;
        fg_mode.applied = farcry6_dlss::frame_generation::ModeState::Mode::unknown;
        fg_mode_status.store(4, std::memory_order_relaxed);
        fg_enable_requested.store(false, std::memory_order_release);
        fg_mode_present_outstanding = false;
      }
      if (fg_pending_present.token != nullptr) {
        fg_mode.PresentCompleted(false);
        if (!fg_mode.ReadyForDisplayChange()) fg_mode_status.store(4, std::memory_order_relaxed);
        (void)runtime.set_tag(*fg_pending_present.token, viewport, clear_tags, 3u, nullptr);
        fg_pending_present = {};
        fg_has_pending_present.store(false, std::memory_order_release);
        fg_present_unpaired.fetch_add(1u, std::memory_order_relaxed);
        fg_complete_input_streak.store(0u, std::memory_order_relaxed);
      }
      if (present_input.token != nullptr) {
        if (queue != nullptr && swapchain != nullptr && present_input.handoff
            && present_input.submit_queue == queue && !present_input.conflicting_queue
            && fg_camera_capture_enabled.load(std::memory_order_acquire)
            && !PauseTaaRecentlySeen() && !fg_swapchain_transition.load(std::memory_order_acquire)
            && farcry6_dlss::streamline::pcl_set_marker != nullptr) {
          const auto result = farcry6_dlss::streamline::pcl_set_marker(sl::PCLMarker::ePresentStart, *present_input.token);
          fg_input_last_result.store(static_cast<int>(result), std::memory_order_relaxed);
          if (result == sl::Result::eOk) {
            fg_pending_present = {.token = present_input.token, .queue = queue, .swapchain = swapchain,
                                  .hudless_tagged = present_input.hudless_tagged, .frame = frame_counter};
            fg_has_pending_present.store(true, std::memory_order_release);
            fg_present_started.fetch_add(1u, std::memory_order_relaxed);
          } else {
            (void)runtime.set_tag(*present_input.token, viewport, clear_tags, 3u, nullptr);
            fg_input_failed.fetch_add(1u, std::memory_order_relaxed);
            fg_complete_input_streak.store(0u, std::memory_order_relaxed);
          }
        } else {
          (void)runtime.set_tag(*present_input.token, viewport, clear_tags, 3u, nullptr);
          fg_present_unpaired.fetch_add(1u, std::memory_order_relaxed);
          fg_complete_input_streak.store(0u, std::memory_order_relaxed);
        }
      }
      const bool usable = fg_has_pending_present.load(std::memory_order_acquire) && fg_pending_present.hudless_tagged
                       && FgInputsEnabled() && fg_enable_requested.load(std::memory_order_acquire)
                       && dlss_enabled != 0.f
                       && !fg_swapchain_transition.load(std::memory_order_acquire) && !PauseTaaRecentlySeen();
      if (!usable && !fg_mode.ReadyForDisplayChange()
          && !(fg_mode.has_pending && fg_mode.pending == farcry6_dlss::frame_generation::ModeState::Mode::off)) {
        const uint32_t reasons = (fg_has_pending_present.load(std::memory_order_acquire) ? 0u : 1u)
                               | (fg_pending_present.hudless_tagged ? 0u : 2u)
                               | (FgInputsEnabled() ? 0u : 4u)
                               | (fg_enable_requested.load(std::memory_order_acquire) ? 0u : 8u)
                               | (fg_swapchain_transition.load(std::memory_order_acquire) ? 16u : 0u)
                               | (PauseTaaRecentlySeen() ? 32u : 0u)
                               | (present_input.token == nullptr ? 64u : 0u)
                               | (!present_input.handoff ? 128u : 0u)
                               | (present_input.submit_queue != queue || present_input.conflicting_queue ? 256u : 0u)
                               | (!fg_camera_capture_enabled.load(std::memory_order_acquire) ? 512u : 0u);

        if (reasons == 2u) {
          const uint64_t now = GetTickCount64();
          fg_missing_hudless_restarts = (fg_last_missing_hudless_ms != 0u && now - fg_last_missing_hudless_ms < 5000u
                                             ? fg_missing_hudless_restarts + 1u : 1u);
          fg_last_missing_hudless_ms = now;
          if (fg_missing_hudless_restarts >= 3u) {
            fg_enable_requested.store(false, std::memory_order_release);
            fg_missing_hudless_restarts = 0u;
            fg_last_missing_hudless_ms = 0u;

          }
        }
        if (!SetFgMode(farcry6_dlss::frame_generation::ModeState::Mode::off)) {
          fg_mode_status.store(4, std::memory_order_relaxed);
          fg_enable_requested.store(false, std::memory_order_release);
        }
      }
      if (fg_mode_status.load(std::memory_order_relaxed) != 0 && fg_mode_swapchain == swapchain) {
        fg_mode_present_outstanding = true;
      }
    }
  }
  if (!FgInputsEnabled() || present_input.token == nullptr) {
    fg_complete_input_streak.store(0u, std::memory_order_relaxed);
  }
  const bool gameplay_handoff = fg_last_scene_handoff_frame.load(std::memory_order_acquire) == frame_counter
                             && !PauseTaaRecentlySeen() && frame_counter != 0u
                             && fg_successful_present_frame.load(std::memory_order_acquire) == frame_counter - 1u;
  if (frame_counter != 0u && dlss_enabled != 0.f && !game_upscale_active.load()
      && fg_last_scene_handoff_frame.load(std::memory_order_acquire) == frame_counter - 1u) {
    const uint32_t details = (last_taa_draw_frame.load(std::memory_order_relaxed) == frame_counter ? 1u : 0u)
                           | (last_upscaler_eval_frame.load(std::memory_order_relaxed) == frame_counter ? 2u : 0u);

  }
  const uint64_t temporary_off_time = fg_temporary_off_time_ms.load(std::memory_order_acquire);
  // A short missing-frame interruption can recover without the full startup delay.
  // Keep the window open after requesting on so capture stays enabled as it settles.
  const bool quick_recovery = fg_enable_requested.load(std::memory_order_acquire)
                           && temporary_off_time != 0u && GetTickCount64() - temporary_off_time < 1000u;
  const uint32_t stable_frames = (gameplay_handoff ? (std::min)(fg_stable_gameplay_frames.load() + 1u, 8u) : 0u);
  fg_stable_gameplay_frames.store(stable_frames, std::memory_order_relaxed);
  fg_present_frame.store(frame_counter, std::memory_order_release);
  ++frame_counter;
  camera_builder_frame.store(frame_counter, std::memory_order_release);
  const bool want_halton = dlss_enabled != 0.f && IsSupported();
  camera_builder_motion_offsets_enabled.store(want_halton, std::memory_order_release);
  if (want_halton) StartCameraBuilder();
  {
    std::shared_lock lock(camera_builder.mutex);
    camera_builder_halton_enabled.store(want_halton && camera_builder.state == CameraBuilderState::State::active,
                                        std::memory_order_release);
    fg_camera_capture_enabled.store(want_halton && FgInputsEnabled()
                                        && !fg_swapchain_transition.load(std::memory_order_acquire)
                                        && stable_frames >= (quick_recovery ? 1u : 8u) && camera_builder.state == CameraBuilderState::State::active
                                        && camera_builder.offsets_hook_installed
                                        && farcry6_dlss::streamline::factory_hook_active.load(std::memory_order_acquire),
                                    std::memory_order_release);
  }

}

inline void OnFgDestroySwapchain(reshade::api::swapchain* swapchain, bool) {
  if (!farcry6_dlss::streamline::factory_hook_active.load(std::memory_order_acquire)
      || swapchain == nullptr || swapchain->get_device() == nullptr
      || reinterpret_cast<void*>(swapchain->get_device()->get_native())
          != farcry6_dlss::streamline::bootstrap_device.load(std::memory_order_acquire)) return;

  fg_swapchain_transition.store(true, std::memory_order_release);
  fg_hudless_retire_requested.store(true, std::memory_order_release);
  fg_camera_capture_enabled.store(false, std::memory_order_release);
  fg_complete_input_streak.store(0u, std::memory_order_relaxed);
  fg_completed_input_on_gpu.store(false, std::memory_order_release);
  fg_stable_gameplay_frames.store(0u, std::memory_order_relaxed);
  fg_swapchain_transitions.fetch_add(1u, std::memory_order_relaxed);
}

inline bool OnFgFullscreenState(reshade::api::swapchain* swapchain, bool, void*) {
  if (!farcry6_dlss::streamline::factory_hook_active.load(std::memory_order_acquire)
      || swapchain == nullptr || swapchain->get_device() == nullptr
      || reinterpret_cast<void*>(swapchain->get_device()->get_native())
          != farcry6_dlss::streamline::bootstrap_device.load(std::memory_order_acquire)) return false;
  fg_camera_capture_enabled.store(false, std::memory_order_release);
  fg_complete_input_streak.store(0u, std::memory_order_relaxed);
  fg_completed_input_on_gpu.store(false, std::memory_order_release);
  fg_hudless_retire_requested.store(true, std::memory_order_release);
  fg_stable_gameplay_frames.store(0u, std::memory_order_relaxed);
  fg_successful_present_frame.store(UINT64_MAX, std::memory_order_release);
  fg_fullscreen_transitions.fetch_add(1u, std::memory_order_relaxed);
  return false;
}

inline void OnFgInitSwapchain(reshade::api::swapchain* swapchain, bool) {
  if (!farcry6_dlss::streamline::factory_hook_active.load(std::memory_order_acquire)
      || swapchain == nullptr || swapchain->get_device() == nullptr
      || reinterpret_cast<void*>(swapchain->get_device()->get_native())
          != farcry6_dlss::streamline::bootstrap_device.load(std::memory_order_acquire)) return;

  fg_last_scene_handoff_frame.store(UINT64_MAX, std::memory_order_release);
  fg_stable_gameplay_frames.store(0u, std::memory_order_relaxed);
  fg_swapchain_transition.store(false, std::memory_order_release);
}

inline void OnFinishPresent(reshade::api::command_queue* queue, reshade::api::swapchain* swapchain) {

  if (queue != nullptr && swapchain != nullptr && queue->get_device() != nullptr
      && reinterpret_cast<void*>(queue->get_device()->get_native())
          == farcry6_dlss::streamline::bootstrap_device.load(std::memory_order_acquire)) {
    fg_successful_present_frame.store(fg_present_frame.load(std::memory_order_acquire), std::memory_order_release);
  }
  if (!fg_has_pending_present.load(std::memory_order_acquire)
      && fg_mode_status.load(std::memory_order_relaxed) == 0) return;
  std::lock_guard frame_lock(farcry6_dlss::streamline::frame_api_mutex);
  if (!farcry6_dlss::streamline::factory_hook_active.load(std::memory_order_acquire)) return;
  if (fg_mode_swapchain == swapchain) fg_mode_present_outstanding = false;
  if (fg_mode.has_pending && fg_mode_swapchain == swapchain) {
    fg_mode.PresentCompleted(true);
    fg_applied_generated.store(fg_pending_generated, std::memory_order_release);
    fg_pending_generated = 0u;
    fg_resources_retained.store(fg_mode.ReadyForDisplayChange() && fg_pending_retain_resources, std::memory_order_release);
    fg_pending_retain_resources = false;
    fg_mode_status.store(fg_mode.ReadyForDisplayChange() ? 0 : 2, std::memory_order_relaxed);

    if (fg_mode.ReadyForDisplayChange() && farcry6_dlss::streamline::reflex_set_options != nullptr) {
      sl::ReflexOptions reflex = {};
      reflex.mode = sl::ReflexMode::eOff;
      (void)farcry6_dlss::streamline::reflex_set_options(reflex);
    }
  }
  if (fg_pending_present.token == nullptr || fg_pending_present.queue != queue
      || fg_pending_present.swapchain != swapchain) return;
  const auto result = farcry6_dlss::streamline::pcl_set_marker(sl::PCLMarker::ePresentEnd, *fg_pending_present.token);
  bool state_healthy = false;
  if (farcry6_dlss::streamline::fg_get_state != nullptr) {
    sl::DLSSGState state = {};
    const auto state_result = farcry6_dlss::streamline::fg_get_state(sl::ViewportHandle(0u), state, nullptr);
    if (state_result == sl::Result::eOk) {
      state_healthy = state.status == sl::DLSSGStatus::eOk;
      fg_state_max_generated.store(state.numFramesToGenerateMax, std::memory_order_relaxed);
    }
  }
  const sl::ResourceTag clear_tags[] = {
      {nullptr, sl::kBufferTypeDepth, sl::ResourceLifecycle::eValidUntilPresent},
      {nullptr, sl::kBufferTypeMotionVectors, sl::ResourceLifecycle::eValidUntilPresent},
      {nullptr, sl::kBufferTypeHUDLessColor, sl::ResourceLifecycle::eValidUntilPresent},
  };
  const auto clear_result = farcry6_dlss::streamline::runtime.set_tag(
      *fg_pending_present.token, sl::ViewportHandle(0u), clear_tags, 3u, nullptr);
  const bool complete_inputs = fg_pending_present.hudless_tagged && state_healthy
                            && result == sl::Result::eOk && clear_result == sl::Result::eOk
                            && FgInputsEnabled() && fg_camera_capture_enabled.load(std::memory_order_acquire)
                            && !fg_swapchain_transition.load(std::memory_order_acquire)
                            && !fg_hudless_retire_requested.load(std::memory_order_acquire);
  fg_pending_present = {};
  fg_has_pending_present.store(false, std::memory_order_release);
  if (result == sl::Result::eOk && clear_result == sl::Result::eOk) {
    fg_present_finished.fetch_add(1u, std::memory_order_relaxed);
  } else {
    fg_input_last_result.store(static_cast<int>(result != sl::Result::eOk ? result : clear_result), std::memory_order_relaxed);
    fg_input_failed.fetch_add(1u, std::memory_order_relaxed);
  }
  if (!state_healthy && fg_mode.applied == farcry6_dlss::frame_generation::ModeState::Mode::on) {
    fg_enable_requested.store(false, std::memory_order_release);
  }
  if (complete_inputs) {
    fg_complete_input_frames.fetch_add(1u, std::memory_order_relaxed);
    const uint32_t streak = fg_complete_input_streak.fetch_add(1u, std::memory_order_relaxed) + 1u;
    const uint64_t temporary_off_time = fg_temporary_off_time_ms.load(std::memory_order_acquire);
    const bool quick_recovery = fg_resources_retained.load(std::memory_order_acquire)
                             && temporary_off_time != 0u && GetTickCount64() - temporary_off_time < 1000u;
    const uint32_t generated_frames = (std::min)(fg_requested_generated.load(std::memory_order_acquire),
                                                 fg_state_max_generated.load(std::memory_order_relaxed));
    if (generated_frames != 0u && fg_enable_requested.load(std::memory_order_acquire)
        && fg_mode.applied == farcry6_dlss::frame_generation::ModeState::Mode::on && !fg_mode.has_pending
        && fg_applied_generated.load(std::memory_order_acquire) != generated_frames) {
      if (!SetFgMode(farcry6_dlss::frame_generation::ModeState::Mode::on, generated_frames)) {
        fg_enable_requested.store(false, std::memory_order_release);
      }
    }
    if (streak >= (quick_recovery ? 2u : 8u) && fg_enable_requested.load(std::memory_order_acquire)
        && fg_completed_input_on_gpu.load(std::memory_order_acquire)
        && generated_frames != 0u && dlss_enabled != 0.f
        && fg_mode.ReadyForDisplayChange() && farcry6_dlss::streamline::fg_set_options != nullptr
        && farcry6_dlss::streamline::reflex_set_options != nullptr
        && farcry6_dlss::streamline::bootstrap_fg_support.load() == 0
        && farcry6_dlss::streamline::bootstrap_reflex_support.load() == 0) {
      sl::ReflexOptions reflex = {};
      reflex.mode = sl::ReflexMode::eLowLatency;
      const auto reflex_result = farcry6_dlss::streamline::reflex_set_options(reflex);
      if (reflex_result != sl::Result::eOk || !SetFgMode(farcry6_dlss::frame_generation::ModeState::Mode::on, generated_frames)) {
        fg_enable_requested.store(false, std::memory_order_release);
        if (reflex_result == sl::Result::eOk) {
          reflex.mode = sl::ReflexMode::eOff;
          (void)farcry6_dlss::streamline::reflex_set_options(reflex);
        }
      } else {
        fg_mode_swapchain = swapchain;

      }
    }
  } else {
    fg_complete_input_streak.store(0u, std::memory_order_relaxed);
  }
}

inline void OnInitDevice(reshade::api::device* device) {
  if (device == nullptr) return;
  int vendor_id = 0;
  const bool is_nvidia = device->get_property(reshade::api::device_properties::vendor_id, &vendor_id)
                      && vendor_id == 0x10de;
  if (device->get_api() == reshade::api::device_api::d3d12) {
    if (is_nvidia) {
      farcry6_dlss::streamline::RegisterBootstrapDevice(reinterpret_cast<void*>(device->get_native()));
    } else {
      farcry6_dlss::streamline::StopBootstrap(nullptr);
    }
  }
  const bool supported_vendor = is_nvidia;
  {
    std::lock_guard<std::mutex> lock(device_mutex);
    if (supported_vendor && device->get_api() == reshade::api::device_api::d3d12) {
      gpu_devices.insert(device->get_native());
      AddGpuNative(device->get_native());
      if (is_nvidia) {
        for (auto& native : nvidia_devs) {
          uint64_t empty = 0u;
          if (native.compare_exchange_strong(empty, device->get_native())) break;
        }
      }
    }
    any_gpu.store(!gpu_devices.empty());
  }

}

inline void OnDestroyDevice(reshade::api::device* device) {
  if (device == nullptr) return;
  if (device->get_api() == reshade::api::device_api::d3d12
      && reinterpret_cast<void*>(device->get_native())
          == farcry6_dlss::streamline::bootstrap_device.load(std::memory_order_acquire)) {
    farcry6_dlss::streamline::StopBootstrap(reinterpret_cast<void*>(device->get_native()));
    {
      std::lock_guard frame_lock(farcry6_dlss::streamline::frame_api_mutex);
      fg_pending_present = {};
      fg_has_pending_present.store(false, std::memory_order_release);
      fg_mode = {};
      fg_mode_swapchain = nullptr;
      fg_mode_present_outstanding = false;
      fg_pending_retain_resources = false;
      fg_pending_generated = 0u;
      fg_applied_generated.store(0u, std::memory_order_release);
      fg_resources_retained.store(false, std::memory_order_release);
      fg_temporary_off_time_ms.store(0u, std::memory_order_release);
      fg_last_missing_hudless_ms = 0u;
      fg_missing_hudless_restarts = 0u;
      fg_mode_status.store(0, std::memory_order_relaxed);
      fg_enable_requested.store(false, std::memory_order_release);
    }
    fg_input_recording_list.store(nullptr, std::memory_order_release);
    fg_camera_capture_enabled.store(false, std::memory_order_release);
    fg_complete_input_streak.store(0u, std::memory_order_relaxed);
    fg_completed_input_on_gpu.store(false, std::memory_order_release);
    fg_swapchain_transition.store(false, std::memory_order_release);
    fg_output_color_space.store(reshade::api::color_space::unknown, std::memory_order_relaxed);
    fg_output_format.store(0u, std::memory_order_relaxed);
    fg_present_frame.store(UINT64_MAX, std::memory_order_release);
    fg_successful_present_frame.store(UINT64_MAX, std::memory_order_release);
    fg_hdr_parameter_frame.store(UINT64_MAX, std::memory_order_release);
    {
      std::lock_guard lock(fg_camera_sample_mutex);
      fg_camera_samples = {};
      fg_camera_evaluations = {};
      fg_camera_sample_frame.store(UINT64_MAX, std::memory_order_release);
      fg_last_scene_handoff_frame.store(UINT64_MAX, std::memory_order_release);
      last_taa_draw_frame.store(UINT64_MAX, std::memory_order_relaxed);
      last_upscaler_eval_frame.store(UINT64_MAX, std::memory_order_relaxed);
      fg_stable_gameplay_frames.store(0u, std::memory_order_relaxed);
    }
  }
  bool was_ours = false;
  {
    std::lock_guard<std::mutex> lock(device_mutex);
    was_ours = gpu_devices.erase(device->get_native()) > 0u;
    any_gpu.store(!gpu_devices.empty());
  }

  if (was_ours) {
    ReleaseFgHudless(device);
    {
      std::unique_lock lock(rcas.mutex);
      if (rcas.device == device) {
        for (auto& slot : rcas.slots) {
          if (slot.source_view.handle != 0u) device->destroy_resource_view(slot.source_view);
          if (slot.target_view.handle != 0u) device->destroy_resource_view(slot.target_view);
          if (slot.texture.handle != 0u) device->destroy_resource(slot.texture);
          for (auto& table : slot.tables) {
            if (table.handle != 0u) device->free_descriptor_table(table);
          }
          slot = {};
        }
        if (rcas.pipeline.handle != 0u) device->destroy_pipeline(rcas.pipeline);
        if (rcas.layout.handle != 0u) device->destroy_pipeline_layout(rcas.layout);
        rcas.pipeline = {};
        rcas.layout = {};
        rcas.pipeline_attempted = false;
        rcas.device = nullptr;
      }
    }
    ReleaseColorConversion(device);
    ReleaseNgx();
    ReleaseUnpack(unpack_device);
    unpack_device = nullptr;
    ReleaseViewCache(device);
    std::unique_lock lock(gpu_retirement.mutex);
    gpu_retirement.recorded_lists.clear();
    gpu_retirement.tickets.clear();
    gpu_retirement.fence.reset();
    gpu_retirement.queue = nullptr;
    gpu_retirement.submit_thread = 0u;
    gpu_retirement.conflicting_submission = false;
  }
  RemoveGpuNative(device->get_native());

}

inline void Use(DWORD fdw_reason, const ShaderInjectData* settings) {
  switch (fdw_reason) {
    case DLL_PROCESS_ATTACH:
      if (attached) return;
      attached = true;
      output_settings = settings;
      reshade::register_event<reshade::addon_event::create_device>(OnCreateDevice);
      reshade::register_event<reshade::addon_event::init_device>(OnInitDevice);
      reshade::register_event<reshade::addon_event::destroy_device>(OnDestroyDevice);
      reshade::register_event<reshade::addon_event::update_descriptor_tables>(OnUpdateDescriptorTables);
      reshade::register_event<reshade::addon_event::destroy_resource>(OnDestroyResource);
      reshade::register_event<reshade::addon_event::init_command_list>(OnInitCommandList);
      reshade::register_event<reshade::addon_event::reset_command_list>(OnResetCommandList);
      reshade::register_event<reshade::addon_event::execute_command_list>(OnExecuteCommandList);
      reshade::register_event<reshade::addon_event::destroy_command_list>(OnDestroyCommandList);
      reshade::register_event<reshade::addon_event::bind_pipeline>(OnBindPipeline);
      reshade::register_event<reshade::addon_event::bind_descriptor_tables>(OnBindDescriptorTables);
      reshade::register_event<reshade::addon_event::begin_render_pass>(OnBeginRenderPass);
      reshade::register_event<reshade::addon_event::end_render_pass>(OnEndRenderPass);
      reshade::register_event<reshade::addon_event::barrier>(OnBarrier);
      reshade::register_event<reshade::addon_event::bind_render_targets_and_depth_stencil>(OnBindRenderTargetsAndDepthStencil);
      reshade::register_event<reshade::addon_event::draw>(OnDraw);
      reshade::register_event<reshade::addon_event::draw_indexed>(OnDrawIndexed);
      reshade::register_event<reshade::addon_event::dispatch>(OnDispatch);
      reshade::register_event<reshade::addon_event::init_resource>(OnInitResource);
      reshade::register_event<reshade::addon_event::present>(OnPresent);
      reshade::register_event<reshade::addon_event::finish_present>(OnFinishPresent);
      reshade::register_event<reshade::addon_event::destroy_swapchain>(OnFgDestroySwapchain);
      reshade::register_event<reshade::addon_event::init_swapchain>(OnFgInitSwapchain);
      reshade::register_event<reshade::addon_event::set_fullscreen_state>(OnFgFullscreenState);
      break;
    case DLL_PROCESS_DETACH:

      if (!attached) return;
      attached = false;
      reshade::unregister_event<reshade::addon_event::present>(OnPresent);
      reshade::unregister_event<reshade::addon_event::finish_present>(OnFinishPresent);
      reshade::unregister_event<reshade::addon_event::destroy_swapchain>(OnFgDestroySwapchain);
      reshade::unregister_event<reshade::addon_event::init_swapchain>(OnFgInitSwapchain);
      reshade::unregister_event<reshade::addon_event::set_fullscreen_state>(OnFgFullscreenState);
      reshade::unregister_event<reshade::addon_event::init_resource>(OnInitResource);
      reshade::unregister_event<reshade::addon_event::dispatch>(OnDispatch);
      reshade::unregister_event<reshade::addon_event::draw_indexed>(OnDrawIndexed);
      reshade::unregister_event<reshade::addon_event::draw>(OnDraw);
      reshade::unregister_event<reshade::addon_event::bind_render_targets_and_depth_stencil>(OnBindRenderTargetsAndDepthStencil);
      reshade::unregister_event<reshade::addon_event::barrier>(OnBarrier);
      reshade::unregister_event<reshade::addon_event::bind_descriptor_tables>(OnBindDescriptorTables);
      reshade::unregister_event<reshade::addon_event::end_render_pass>(OnEndRenderPass);
      reshade::unregister_event<reshade::addon_event::begin_render_pass>(OnBeginRenderPass);
      reshade::unregister_event<reshade::addon_event::bind_pipeline>(OnBindPipeline);
      reshade::unregister_event<reshade::addon_event::execute_command_list>(OnExecuteCommandList);
      reshade::unregister_event<reshade::addon_event::reset_command_list>(OnResetCommandList);
      reshade::unregister_event<reshade::addon_event::destroy_command_list>(OnDestroyCommandList);
      reshade::unregister_event<reshade::addon_event::init_command_list>(OnInitCommandList);
      reshade::unregister_event<reshade::addon_event::destroy_resource>(OnDestroyResource);
      reshade::unregister_event<reshade::addon_event::update_descriptor_tables>(OnUpdateDescriptorTables);
      reshade::unregister_event<reshade::addon_event::destroy_device>(OnDestroyDevice);
      reshade::unregister_event<reshade::addon_event::init_device>(OnInitDevice);
      reshade::unregister_event<reshade::addon_event::create_device>(OnCreateDevice);
      {
        reshade::api::device* device = unpack_device != nullptr ? unpack_device
            : nullptr;
        ReleaseFgHudless(device);
        ReleaseColorConversion(color_conversion.device);
            unpack_device = nullptr;
        ReleaseUnpack(device);
            ReleaseViewCache(device);
      }
      ReleaseNgx();
      output_settings = nullptr;
      break;
  }
}

}  // namespace farcry6_dlss::dlss
