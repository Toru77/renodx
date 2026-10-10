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

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include <include/reshade.hpp>

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
    // resolve-denoise/
    {0x209AB6A4, "resolve-4tap"},
    {0x596D3E8F, "resolve-spatial"},
    {0x0B33C6D8, "resolve-bilateral"},
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

constexpr size_t kMaxOrder = 48;
constexpr size_t kMaxCandidates = 32;
constexpr size_t kMaxTop = 16;
constexpr size_t kMaxCumulativeEntries = 4096;

struct FrameSnapshot {
  uint64_t frames_total = 0;
  uint64_t frame_index = 0;
  uint64_t dispatches = 0;
  uint64_t indirect = 0;
  std::vector<std::pair<uint32_t, uint32_t>> top;        // hash, count (desc)
  std::vector<uint32_t> order;                            // first-seen distinct
  std::vector<std::pair<uint32_t, uint32_t>> candidates;  // known shader, count
};

inline std::mutex state_mutex;
inline std::unordered_map<uint32_t, uint32_t> frame_counts;
inline std::vector<uint32_t> frame_order;
inline uint64_t frame_dispatches = 0;
inline uint64_t frame_indirect = 0;
inline std::unordered_map<uint32_t, uint64_t> cumulative_counts;
inline uint64_t cumulative_dispatches = 0;
inline uint64_t frames_seen = 0;
inline FrameSnapshot last_frame;

inline void Record(uint32_t hash, bool indirect) {
  const std::lock_guard lock(state_mutex);
  ++frame_dispatches;
  if (indirect) ++frame_indirect;
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

inline void Snapshot() {
  const std::lock_guard lock(state_mutex);
  FrameSnapshot snap;
  snap.frames_total = ++frames_seen;
  snap.frame_index = frames_seen;
  snap.dispatches = frame_dispatches;
  snap.indirect = frame_indirect;
  snap.order = frame_order;

  std::vector<std::pair<uint32_t, uint32_t>> entries(frame_counts.begin(), frame_counts.end());
  const size_t take = std::min(kMaxTop, entries.size());
  std::partial_sort(
      entries.begin(), entries.begin() + static_cast<ptrdiff_t>(take), entries.end(),
      [](const auto& a, const auto& b) { return a.second > b.second; });
  snap.top.assign(entries.begin(), entries.begin() + static_cast<ptrdiff_t>(take));

  for (const auto& known : kKnownShaders) {
    if (snap.candidates.size() >= kMaxCandidates) break;
    const auto it = frame_counts.find(known.hash);
    if (it == frame_counts.end() || it->second == 0) continue;
    snap.candidates.emplace_back(known.hash, it->second);
  }

  last_frame = std::move(snap);
  frame_counts.clear();
  frame_order.clear();
  frame_dispatches = 0;
  frame_indirect = 0;
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

inline void Reset() {
  const std::lock_guard lock(state_mutex);
  frame_counts.clear();
  frame_order.clear();
  frame_dispatches = 0;
  frame_indirect = 0;
  cumulative_counts.clear();
  cumulative_dispatches = 0;
  frames_seen = 0;
  last_frame = {};
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
    << " dispatch(es), " << snap.indirect << " indirect\n";
  if (!snap.candidates.empty()) {
    s << "  candidates (last frame):";
    for (const auto& [hash, count] : snap.candidates) {
      s << " " << KnownName(hash) << "x" << count;
    }
    s << "\n";
  }
  s << "  first-seen order (" << snap.order.size() << "):\n";
  for (uint32_t hash : snap.order) {
    s << "    " << ShaderLabel(hash) << "\n";
  }
  s << "  top dispatches (last frame):\n";
  for (const auto& [hash, count] : snap.top) {
    s << "    " << count << "x " << ShaderLabel(hash) << "\n";
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
  if (shader_state == nullptr) return;
  auto* compute_state = renodx::utils::shader::GetCurrentComputeState(shader_state);
  Record(renodx::utils::shader::GetCurrentShaderHash(compute_state), indirect);
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
      reshade::register_event<reshade::addon_event::present>(OnPresent);
      break;
    case DLL_PROCESS_DETACH:
      reshade::unregister_event<reshade::addon_event::dispatch>(OnDispatch);
      reshade::unregister_event<reshade::addon_event::draw_or_dispatch_indirect>(
          OnDispatchIndirect);
      reshade::unregister_event<reshade::addon_event::present>(OnPresent);
      break;
  }
}

}  // namespace pass_map
