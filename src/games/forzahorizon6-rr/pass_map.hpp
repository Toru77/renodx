/*
 * Copyright (C) 2026 Carlos Lopez, speedlemur
 * SPDX-License-Identifier: MIT
 *
 * forzahorizon6-rr: compute-pass map diagnostics.
 *
 * Counts compute dispatches per frame (direct + indirect) and records the
 * first-seen shader-hash order, so FH6's RT resolve/denoise chain can be
 * identified live. Everything lands in the overlay panel and the Copy Report.
 */

#pragma once

#include <windows.h>
#include <d3d12.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include <include/reshade.hpp>

#include "../../utils/hash.hpp"
#include "../../utils/shader.hpp"

namespace pass_map {

// --- candidates from the shader dump inventory (shaders/README.md) --------

struct KnownShader {
  uint32_t hash;
  const char* name;
};

// clang-format off
inline const KnownShader kKnownShaders[] = {
    // dxr-trace/ — 16 inline-RayQuery reflection traces (clusters A-D)
    {0x12886018, "trace-A0"}, {0x2A602F59, "trace-A1"}, {0x44D4E258, "trace-A2"},
    {0x489E04F8, "trace-A3"}, {0x50067031, "trace-A4"}, {0xD4B4DB6E, "trace-A5"},
    {0x1A7101EC, "trace-B0"}, {0x2740BA98, "trace-B1"}, {0x43453554, "trace-B2"},
    {0x5408243D, "trace-B3"}, {0x7BD3C79E, "trace-B4"}, {0xD844655D, "trace-B5"},
    {0x490914DC, "trace-C0"}, {0x5CF02E66, "trace-C1"},
    {0xBA7D5A3F, "trace-D0"}, {0xBF9D9135, "trace-D1"},
    // resolve-denoise/ (the two lower entries are RT-quality permutations of
    // the spatial filter and the reconstruction stage; see denoise.hpp)
    {0x209AB6A4, "resolve-4tap"},
    {0x596D3E8F, "resolve-spatial"},
    {0x0B33C6D8, "resolve-bilateral"},
    {0x4DAF8A48, "resolve-spatial-hi"},
    {0x14FA42AB, "resolve-apply-hi"},
    // gbuffer-prep/
    {0xBF794558, "gbuffer-prep"},
    // ssgi/
    {0x8E8540A0, "ssgi-march"},
    // gi-probes/
    {0xBEB68B69, "probe-acc-a"}, {0x1324EF7E, "probe-acc-b"},
    {0xD9CDA0AC, "probe-blend"}, {0xC026B375, "probe-list"},
    {0x9848AF45, "probe-visibility"},
    // infra/
    {0x16478515, "hiz-pyramid"}, {0x7A3FD6D7, "worklist-compact"},
    {0x71D374B7, "indirect-args"}, {0x5175B738, "counter-clear"},
    // potential/
    {0x04C1F302, "cs-probe-cand"}, {0x0E548F62, "cs-vis-cand"},
};
// clang-format on

inline const char* KnownName(uint32_t hash) {
  for (const auto& entry : kKnownShaders) {
    if (entry.hash == hash) return entry.name;
  }
  return nullptr;
}

inline std::string ShaderLabel(uint32_t hash) {
  char buffer[24] = {};
  snprintf(buffer, sizeof buffer, "0x%08X", hash);
  const char* name = KnownName(hash);
  if (name == nullptr) return buffer;
  return std::string(name) + " (" + buffer + ")";
}

// --- state ----------------------------------------------------------------

constexpr size_t kMaxOrder = 96;
constexpr size_t kMaxCandidates = 32;
constexpr size_t kMaxTop = 16;
constexpr size_t kMaxTopIndirectUnknown = 8;
constexpr size_t kMaxCumulativeEntries = 4096;

struct CandidateCount {
  uint32_t hash = 0;
  uint32_t total = 0;
  uint32_t indirect = 0;
};

struct FrameSnapshot {
  uint64_t frames_total = 0;
  uint64_t frame_index = 0;
  uint64_t dispatches = 0;
  uint64_t indirect = 0;
  uint64_t zero_hash = 0;  // unresolved dispatches (last frame)
  uint64_t fallback = 0;   // resolved via our own pipeline map (last frame)
  uint64_t blob = 0;       // resolved via PSO blob extraction (last frame)
  std::vector<std::pair<uint32_t, uint32_t>> top;  // hash, count (desc)
  std::vector<std::pair<uint32_t, uint32_t>> top_indirect_unknown;
  std::vector<uint32_t> order;             // first-seen distinct
  std::vector<CandidateCount> candidates;  // known shader counts
  std::vector<std::pair<uint64_t, uint32_t>> unresolved_handles;  // handle, count
};

inline std::mutex state_mutex;
inline std::unordered_map<uint32_t, uint32_t> frame_counts;
inline std::unordered_map<uint32_t, uint32_t> frame_indirect_counts;
inline std::vector<uint32_t> frame_order;
inline uint64_t frame_dispatches = 0;
inline uint64_t frame_indirect = 0;
inline uint64_t frame_zero_hash = 0;
inline uint64_t frame_fallback = 0;
inline uint64_t frame_blob = 0;
inline std::unordered_map<uint64_t, uint32_t> frame_unresolved_handles;
inline std::unordered_map<uint32_t, uint64_t> cumulative_counts;
inline std::unordered_map<uint32_t, uint64_t> cumulative_candidate_counts;
inline uint64_t cumulative_dispatches = 0;
inline uint64_t cumulative_zero_hash = 0;
inline uint64_t cumulative_fallback = 0;
inline uint64_t frames_seen = 0;
inline FrameSnapshot last_frame;

// Independent pipeline -> shader-hash tracker. The shader utils resolve hashes
// through their own details map; a few dispatches per frame come back as hash 0
// there. Computing the CRC32 ourselves from the compute-shader subobject at
// init_pipeline gives us a second, independent path so nothing stays unnamed.
inline std::mutex pipeline_mutex;
inline std::unordered_map<uint64_t, uint32_t> pipeline_hashes;  // pipeline.handle -> hash
inline uint64_t pipelines_tracked = 0;

inline void OnInitPipeline(
    reshade::api::device*, reshade::api::pipeline_layout, uint32_t subobject_count,
    const reshade::api::pipeline_subobject* subobjects, reshade::api::pipeline pipeline) {
  if (pipeline.handle == 0u || subobjects == nullptr) return;
  for (uint32_t i = 0; i < subobject_count; ++i) {
    if (subobjects[i].type != reshade::api::pipeline_subobject_type::compute_shader) continue;
    const auto* desc = static_cast<const reshade::api::shader_desc*>(subobjects[i].data);
    if (desc == nullptr || desc->code == nullptr || desc->code_size == 0) break;
    const uint32_t hash = renodx::utils::hash::ComputeCRC32(
        static_cast<const uint8_t*>(desc->code), desc->code_size);
    if (hash == 0) break;
    const std::lock_guard lock(pipeline_mutex);
    pipeline_hashes[pipeline.handle] = hash;
    ++pipelines_tracked;
    break;
  }
}

inline void OnDestroyPipeline(reshade::api::device*, reshade::api::pipeline pipeline) {
  if (pipeline.handle == 0u) return;
  const std::lock_guard lock(pipeline_mutex);
  pipeline_hashes.erase(pipeline.handle);
}

// Some dispatches bind pipelines whose creation never reached our hooks
// (SL/NGX internals, or anything holding a raw device). Identify those from
// the GPU side: pull the PSO's cached blob, scan it for DXBC containers, and
// CRC32 them with the same convention as the dump filenames.
inline std::unordered_map<uint64_t, uint32_t> unresolved_pipeline_ids;  // handle -> hash (0 = tried, no luck)
inline uint64_t blob_identified = 0;
inline uint32_t frame_blob_attempts = 0;
constexpr uint32_t kMaxBlobAttemptsPerFrame = 16;

inline uint32_t IdentifyPipelineViaBlob(uint64_t pipeline_handle) {
  if (pipeline_handle == 0) return 0;
  uint32_t known_hash = 0;
  uint32_t fallback_hash = 0;
#if defined(_MSC_VER)
  __try {
    auto* pso = reinterpret_cast<ID3D12PipelineState*>(pipeline_handle);
    ID3DBlob* blob = nullptr;
    if (FAILED(pso->GetCachedBlob(&blob)) || blob == nullptr) return 0;
    const auto* bytes = static_cast<const uint8_t*>(blob->GetBufferPointer());
    const size_t size = blob->GetBufferSize();
    if (bytes != nullptr && size >= 32) {
      for (size_t offset = 0; offset + 32 <= size; ++offset) {
        if (bytes[offset] != 'D' || bytes[offset + 1] != 'X' || bytes[offset + 2] != 'B'
            || bytes[offset + 3] != 'C') {
          continue;
        }
        uint32_t container_size = 0;
        memcpy(&container_size, bytes + offset + 12, sizeof(container_size));
        if (container_size < 64 || container_size > size - offset) continue;
        const uint32_t hash = renodx::utils::hash::ComputeCRC32(bytes + offset, container_size);
        if (hash == 0) continue;
        if (KnownName(hash) != nullptr) {
          known_hash = hash;
          break;
        }
        if (fallback_hash == 0) fallback_hash = hash;
      }
    }
    blob->Release();
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    known_hash = 0;
    fallback_hash = 0;
  }
#endif
  return known_hash != 0 ? known_hash : fallback_hash;
}

inline uint32_t ResolveUnresolvedPipeline(uint64_t pipeline_handle) {
  if (pipeline_handle == 0) return 0;
  {
    const std::lock_guard lock(pipeline_mutex);
    const auto it = unresolved_pipeline_ids.find(pipeline_handle);
    if (it != unresolved_pipeline_ids.end()) return it->second;
    if (frame_blob_attempts >= kMaxBlobAttemptsPerFrame) return 0;
    ++frame_blob_attempts;
  }
  const uint32_t hash = IdentifyPipelineViaBlob(pipeline_handle);
  const std::lock_guard lock(pipeline_mutex);
  unresolved_pipeline_ids[pipeline_handle] = hash;  // failures cached too (stable set)
  if (hash != 0) ++blob_identified;
  return hash;
}

inline void Record(uint32_t hash, bool indirect, bool via_fallback = false, bool via_blob = false) {
  const std::lock_guard lock(state_mutex);
  ++frame_dispatches;
  if (indirect) ++frame_indirect;
  if (via_fallback || via_blob) {
    ++frame_fallback;
    ++cumulative_fallback;
  }
  if (via_blob) ++frame_blob;
  ++cumulative_dispatches;
  {
    const auto it = cumulative_counts.find(hash);
    if (it != cumulative_counts.end()) {
      ++it->second;
    } else if (cumulative_counts.size() < kMaxCumulativeEntries) {
      cumulative_counts.emplace(hash, 1);
    }
  }
  ++frame_counts[hash];
  if (indirect) ++frame_indirect_counts[hash];
  if (KnownName(hash) != nullptr) ++cumulative_candidate_counts[hash];
  if (frame_order.size() < kMaxOrder) {
    bool seen = false;
    for (uint32_t existing : frame_order) {
      if (existing == hash) {
        seen = true;
        break;
      }
    }
    if (!seen) frame_order.push_back(hash);
  }
}

// Dispatches whose shader hash we could not resolve (untracked pipeline).
inline void RecordUnresolved(bool indirect, uint64_t pipeline_handle) {
  const std::lock_guard lock(state_mutex);
  ++frame_dispatches;
  if (indirect) ++frame_indirect;
  ++cumulative_dispatches;
  ++frame_zero_hash;
  ++cumulative_zero_hash;
  if (pipeline_handle != 0) ++frame_unresolved_handles[pipeline_handle];
}

inline void Snapshot() {
  const std::lock_guard lock(state_mutex);
  FrameSnapshot snap;
  snap.frames_total = ++frames_seen;
  snap.frame_index = frames_seen;
  snap.dispatches = frame_dispatches;
  snap.indirect = frame_indirect;
  snap.zero_hash = frame_zero_hash;
  snap.fallback = frame_fallback;
  snap.blob = frame_blob;
  snap.order = frame_order;

  if (!frame_unresolved_handles.empty()) {
    std::vector<std::pair<uint64_t, uint32_t>> handles(
        frame_unresolved_handles.begin(), frame_unresolved_handles.end());
    const size_t handle_take = std::min<size_t>(4, handles.size());
    std::partial_sort(
        handles.begin(), handles.begin() + static_cast<ptrdiff_t>(handle_take), handles.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });
    handles.resize(handle_take);
    snap.unresolved_handles = std::move(handles);
  }

  std::vector<std::pair<uint32_t, uint32_t>> entries(frame_counts.begin(), frame_counts.end());
  const size_t take = std::min(kMaxTop, entries.size());
  std::partial_sort(
      entries.begin(), entries.begin() + static_cast<ptrdiff_t>(take), entries.end(),
      [](const auto& a, const auto& b) { return a.second > b.second; });
  snap.top.assign(entries.begin(), entries.begin() + static_cast<ptrdiff_t>(take));

  std::vector<std::pair<uint32_t, uint32_t>> indirect_unknown;
  for (const auto& [hash, count] : frame_indirect_counts) {
    if (KnownName(hash) != nullptr) continue;
    indirect_unknown.emplace_back(hash, count);
  }
  const size_t indirect_take = std::min(kMaxTopIndirectUnknown, indirect_unknown.size());
  std::partial_sort(
      indirect_unknown.begin(), indirect_unknown.begin() + static_cast<ptrdiff_t>(indirect_take),
      indirect_unknown.end(), [](const auto& a, const auto& b) { return a.second > b.second; });
  snap.top_indirect_unknown.assign(
      indirect_unknown.begin(), indirect_unknown.begin() + static_cast<ptrdiff_t>(indirect_take));

  for (const auto& known : kKnownShaders) {
    if (snap.candidates.size() >= kMaxCandidates) break;
    const auto total_it = frame_counts.find(known.hash);
    if (total_it == frame_counts.end() || total_it->second == 0) continue;
    CandidateCount candidate{};
    candidate.hash = known.hash;
    candidate.total = total_it->second;
    const auto indirect_it = frame_indirect_counts.find(known.hash);
    if (indirect_it != frame_indirect_counts.end()) candidate.indirect = indirect_it->second;
    snap.candidates.push_back(candidate);
  }

  last_frame = std::move(snap);
  frame_counts.clear();
  frame_indirect_counts.clear();
  frame_order.clear();
  frame_dispatches = 0;
  frame_indirect = 0;
  frame_zero_hash = 0;
  frame_fallback = 0;
  frame_blob = 0;
  frame_unresolved_handles.clear();
  {
    const std::lock_guard pipeline_lock(pipeline_mutex);
    frame_blob_attempts = 0;
  }
}

inline FrameSnapshot Capture() {
  const std::lock_guard lock(state_mutex);
  return last_frame;
}

inline uint64_t CumulativeDispatches() {
  const std::lock_guard lock(state_mutex);
  return cumulative_dispatches;
}

inline std::vector<std::pair<uint32_t, uint64_t>> CumulativeTop(size_t count) {
  const std::lock_guard lock(state_mutex);
  std::vector<std::pair<uint32_t, uint64_t>> entries(
      cumulative_counts.begin(), cumulative_counts.end());
  const size_t take = std::min(count, entries.size());
  std::partial_sort(
      entries.begin(), entries.begin() + static_cast<ptrdiff_t>(take), entries.end(),
      [](const auto& a, const auto& b) { return a.second > b.second; });
  entries.resize(take);
  return entries;
}

inline uint64_t CumulativeZeroHash() {
  const std::lock_guard lock(state_mutex);
  return cumulative_zero_hash;
}

inline uint64_t CumulativeFallback() {
  const std::lock_guard lock(state_mutex);
  return cumulative_fallback;
}

inline uint64_t PipelinesTracked() {
  const std::lock_guard lock(pipeline_mutex);
  return pipelines_tracked;
}

inline uint64_t CumulativeBlobIdentified() {
  const std::lock_guard lock(pipeline_mutex);
  return blob_identified;
}

inline std::vector<std::pair<uint32_t, uint64_t>> CumulativeCandidates() {
  const std::lock_guard lock(state_mutex);
  std::vector<std::pair<uint32_t, uint64_t>> out;
  for (const auto& known : kKnownShaders) {
    const auto it = cumulative_candidate_counts.find(known.hash);
    if (it == cumulative_candidate_counts.end() || it->second == 0) continue;
    out.emplace_back(known.hash, it->second);
  }
  return out;
}

inline void Reset() {
  const std::lock_guard lock(state_mutex);
  frame_counts.clear();
  frame_indirect_counts.clear();
  frame_order.clear();
  frame_dispatches = 0;
  frame_indirect = 0;
  frame_zero_hash = 0;
  frame_fallback = 0;
  frame_blob = 0;
  frame_unresolved_handles.clear();
  cumulative_counts.clear();
  cumulative_candidate_counts.clear();
  cumulative_dispatches = 0;
  cumulative_zero_hash = 0;
  cumulative_fallback = 0;
  frames_seen = 0;
  last_frame = {};
  {
    const std::lock_guard pipeline_lock(pipeline_mutex);
    blob_identified = 0;
    frame_blob_attempts = 0;
  }
}

// --- report ---------------------------------------------------------------

inline std::string BuildReportSection() {
  const FrameSnapshot snap = Capture();
  std::stringstream s;
  s << "\n[RT pass map]\n";
  if (snap.frames_total == 0) {
    s << "  no presents captured yet\n";
    return s.str();
  }
  s << "  frame " << snap.frame_index << " of " << snap.frames_total << ": " << snap.dispatches
    << " dispatch(es), " << snap.indirect << " indirect; unresolved (hash 0): " << snap.zero_hash
    << " (cumulative " << CumulativeZeroHash() << "); fallback: " << snap.fallback
    << " (cumulative " << CumulativeFallback() << "; blob-identified " << CumulativeBlobIdentified()
    << ", pipelines tracked " << PipelinesTracked() << ")\n";
  if (!snap.unresolved_handles.empty()) {
    s << "  unresolved pipeline handles (last frame):";
    for (const auto& [handle, count] : snap.unresolved_handles) {
      char buffer[32] = {};
      snprintf(buffer, sizeof buffer, " 0x%llX", static_cast<unsigned long long>(handle));
      s << buffer << " x" << count;
    }
    s << "\n";
  }
  if (!snap.candidates.empty()) {
    s << "  candidates (last frame):";
    for (const auto& candidate : snap.candidates) {
      s << " " << KnownName(candidate.hash) << "x" << candidate.total;
      if (candidate.indirect > 0) s << " (i" << candidate.indirect << ")";
    }
    s << "\n";
  }
  if (!snap.top_indirect_unknown.empty()) {
    s << "  top indirect, not curated (last frame):\n";
    for (const auto& [hash, count] : snap.top_indirect_unknown) {
      s << "    " << count << "x " << ShaderLabel(hash) << "\n";
    }
  }
  s << "  first-seen order (" << snap.order.size() << "):\n";
  for (uint32_t hash : snap.order) {
    s << "    " << ShaderLabel(hash) << "\n";
  }
  s << "  top dispatches (last frame):\n";
  for (const auto& [hash, count] : snap.top) {
    s << "    " << count << "x " << ShaderLabel(hash) << "\n";
  }
  const auto cumulative_candidates = CumulativeCandidates();
  if (!cumulative_candidates.empty()) {
    s << "  candidates (cumulative):";
    for (const auto& [hash, count] : cumulative_candidates) {
      s << " " << KnownName(hash) << "x" << count;
    }
    s << "\n";
  }
  s << "  cumulative: " << CumulativeDispatches() << " dispatch(es)\n";
  for (const auto& [hash, count] : CumulativeTop(8)) {
    s << "    " << count << "x " << ShaderLabel(hash) << "\n";
  }
  return s.str();
}

// --- hooks ----------------------------------------------------------------

inline void RecordCurrent(reshade::api::command_list* cmd_list, bool indirect) {
  auto* shader_state = renodx::utils::shader::GetCurrentState(cmd_list);
  if (shader_state == nullptr) {
    RecordUnresolved(indirect, 0);
    return;
  }
  auto* compute_state = renodx::utils::shader::GetCurrentComputeState(shader_state);
  const uint64_t pipeline_handle =
      compute_state != nullptr ? compute_state->pipeline.handle : 0u;
  uint32_t hash = renodx::utils::shader::GetCurrentShaderHash(compute_state);
  bool via_fallback = false;
  bool via_blob = false;
  if (hash == 0 && pipeline_handle != 0u) {
    {
      const std::lock_guard lock(pipeline_mutex);
      const auto it = pipeline_hashes.find(pipeline_handle);
      if (it != pipeline_hashes.end()) {
        hash = it->second;
        via_fallback = true;
      }
    }
    if (hash == 0) {
      hash = ResolveUnresolvedPipeline(pipeline_handle);
      via_blob = hash != 0;
    }
  }
  if (hash == 0) {
    RecordUnresolved(indirect, pipeline_handle);
    return;
  }
  Record(hash, indirect, via_fallback, via_blob);
}

inline bool OnDispatch(reshade::api::command_list* cmd_list, uint32_t, uint32_t, uint32_t) {
  RecordCurrent(cmd_list, false);
  return false;
}

inline bool OnDispatchIndirect(
    reshade::api::command_list* cmd_list, reshade::api::indirect_command type,
    reshade::api::resource, uint64_t, uint32_t, uint32_t) {
  if (type == reshade::api::indirect_command::draw
      || type == reshade::api::indirect_command::draw_indexed) {
    return false;
  }
  RecordCurrent(cmd_list, true);
  return false;
}

inline void OnPresent(
    reshade::api::command_queue*, reshade::api::swapchain*, const reshade::api::rect*,
    const reshade::api::rect*, uint32_t, const reshade::api::rect*) {
  Snapshot();
}

inline void Use(DWORD fdw_reason) {
  renodx::utils::shader::Use(fdw_reason);
  switch (fdw_reason) {
    case DLL_PROCESS_ATTACH:
      reshade::register_event<reshade::addon_event::dispatch>(OnDispatch);
      reshade::register_event<reshade::addon_event::draw_or_dispatch_indirect>(OnDispatchIndirect);
      reshade::register_event<reshade::addon_event::init_pipeline>(OnInitPipeline);
      reshade::register_event<reshade::addon_event::destroy_pipeline>(OnDestroyPipeline);
      reshade::register_event<reshade::addon_event::present>(OnPresent);
      break;
    case DLL_PROCESS_DETACH:
      reshade::unregister_event<reshade::addon_event::dispatch>(OnDispatch);
      reshade::unregister_event<reshade::addon_event::draw_or_dispatch_indirect>(
          OnDispatchIndirect);
      reshade::unregister_event<reshade::addon_event::init_pipeline>(OnInitPipeline);
      reshade::unregister_event<reshade::addon_event::destroy_pipeline>(OnDestroyPipeline);
      reshade::unregister_event<reshade::addon_event::present>(OnPresent);
      break;
  }
}

}  // namespace pass_map
