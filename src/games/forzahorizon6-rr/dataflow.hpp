/*
 * Copyright (C) 2026 Carlos Lopez, speedlemur
 * SPDX-License-Identifier: MIT
 *
 * forzahorizon6-rr: RTGI dataflow capture.
 *
 * Resolves the descriptor-table SRV/UAV bindings of every compute dispatch
 * (the game only runs ~70 distinct shader hashes) into resource handles +
 * formats, so the producer/consumer graph and any ping-ponged history
 * buffers can be read out of the report. The DevKit cannot resolve bindings
 * for these game dispatches (only for our root-descriptor guide pass), and
 * the pass map only tracks hashes.
 *
 * Capture policy: once per hash per frame (the present hook clocks frames).
 * Up to three distinct binding variants are kept per hash; variants change
 * exactly when a pass reads/writes a double-buffered (ping-pong) resource,
 * and the report prints the changed handles as a compact diff, which is how
 * history buffers are identified.
 *
 * Resolution paths per descriptor slot:
 *  1. The shared descriptor-table mirror (utils::descriptor): update/copy
 *     events replicate heap slot contents. Covers every slot written via
 *     Create*View and copied slots (the source view handle is replicated).
 *  2. Direct: rebuild the slot's CPU descriptor address from the bound heap
 *     (on D3D12 the heap handle is the raw ID3D12DescriptorHeap) and resolve
 *     through ReShade's view registry. Covers heaps the mirror never saw but
 *     whose views were created on the device. Copied-to slots are not in the
 *     registry, so path 1 stays primary for those.
 *
 * Note: D3D12 root signatures with static samplers report their tables as
 * 'descriptor_table_with_static_samplers' (not 'descriptor_table'), so both
 * param forms are walked; ranges come from the pipeline layout util's stable
 * per-param copy.
 *
 * The report always carries per-stage counters plus a one-shot inventory of
 * an early dispatch so a single session can pinpoint any break in the chain
 * without a debugger attached.
 */

#pragma once

#include <windows.h>
#include <d3d12.h>

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include <include/reshade.hpp>

#include "../../utils/data.hpp"
#include "../../utils/descriptor.hpp"
#include "../../utils/pipeline_layout.hpp"
#include "../../utils/shader.hpp"
#include "pass_map.hpp"

namespace dataflow {

// Off by default: the capture queries ReShade's resource registries for every
// dispatch, and those per-frame registry calls were the suspected source of
// the repeated ReShade64 access-violation crashes. Enable explicitly for one
// diagnostics round (settings switch), then disable again.
inline std::atomic_bool enabled = false;
inline std::atomic<uint32_t> capture_counter = 0;
inline std::atomic<uint64_t> frame_index{0};

// --- diagnostics counters (monotonic; read by the report) ------------------

inline std::atomic<uint64_t> stat_dispatches{0};      // CaptureDispatch entries
inline std::atomic<uint64_t> stat_no_shader_state{0}; // no shader state for the cmd list
inline std::atomic<uint64_t> stat_no_compute_state{0};
inline std::atomic<uint64_t> stat_throttled{0};       // same hash already captured this frame
inline std::atomic<uint64_t> stat_hashed{0};          // dispatch with a resolved shader hash
inline std::atomic<uint64_t> stat_no_desc_data{0};    // descriptor util data missing
inline std::atomic<uint64_t> stat_no_cmd_data{0};     // our bind tracking missing
inline std::atomic<uint64_t> stat_no_layout{0};       // pipeline layout data missing
inline std::atomic<uint64_t> stat_binds{0};           // compute table binds (our handler)
inline std::atomic<uint64_t> stat_tables{0};          // non-zero tables walked
inline std::atomic<uint64_t> stat_ranges{0};          // SRV/UAV ranges walked
inline std::atomic<uint64_t> stat_heap_miss{0};       // range whose heap is absent from the mirror
inline std::atomic<uint64_t> stat_slot_mirror{0};     // slots resolved via the mirror
inline std::atomic<uint64_t> stat_slot_direct{0};     // slots resolved via the raw-descriptor path
inline std::atomic<uint64_t> stat_slot_miss{0};       // slots neither path could resolve
inline std::atomic<uint64_t> stat_av_caught{0};       // SEH faults swallowed in registry calls
inline std::atomic<uint64_t> stat_unresolved{0};      // dispatches with no shader hash
inline std::atomic<uint64_t> stat_captures{0};

inline std::mutex diag_mutex;
inline std::string diag_first;  // inventory of an early dispatch
inline std::atomic<uint32_t> diag_attempts{0};

// Sparse sample set for unbounded global arrays. Indices come from the
// decompiled passes and the runtime mapping: 21 = raw RT signal texture
// (sp37, read by F42B7F05 and the GI resolves), 30/31/33/34/35 = per-tile
// scheduler masks (read by the worklist classifier 0x4C29B60A). Sampling
// this reduced set keeps every pass under the report cap while still
// resolving the persistent entries the convergence work cares about.
inline constexpr uint32_t kGlobalArrayProbeIndices[] = {21u, 30u, 31u, 33u, 34u, 35u};

struct __declspec(uuid("8e2f4a91-5b7c-4d3e-9f10-a1b2c3d4e5f6")) CommandListData {
  std::mutex mutex;
  reshade::api::pipeline_layout compute_layout = {0};
  std::vector<reshade::api::descriptor_table> compute_tables;
};

struct Binding {
  std::string label;   // e.g. "t22.sp36"
  uint64_t handle = 0;
  std::string desc;    // e.g. "[640x360 f10]" or "[buf 3686400B]"
};

struct Capture {
  uint32_t counter = 0;
  uint32_t changes = 0;             // distinct binding variants observed
  std::vector<std::vector<Binding>> variants;  // up to 3, most recent last
};

inline std::mutex captures_mutex;
inline std::unordered_map<uint32_t, Capture> captures;
// Dispatches whose shader hash is unresolved (tracked as hash 0 by the pass
// map; e.g. pipelines created outside the hooks) are captured keyed by
// pipeline handle instead, so their bindings can still be read from the
// report.
inline std::unordered_map<uint64_t, Capture> unresolved_captures;

inline std::mutex throttle_mutex;
inline std::unordered_map<uint64_t, uint64_t> last_capture_frame;

inline bool IsUavRange(reshade::api::descriptor_type type) {
  switch (type) {
    case reshade::api::descriptor_type::texture_unordered_access_view:
    case reshade::api::descriptor_type::buffer_unordered_access_view:
      return true;
    default:
      return false;
  }
}

inline bool IsSrvRange(reshade::api::descriptor_type type) {
  switch (type) {
    case reshade::api::descriptor_type::texture_shader_resource_view:
    case reshade::api::descriptor_type::buffer_shader_resource_view:
    case reshade::api::descriptor_type::sampler_with_resource_view:
      return true;
    default:
      return false;
  }
}

inline void OnBindDescriptorTables(
    reshade::api::command_list* cmd_list, reshade::api::shader_stage stages,
    reshade::api::pipeline_layout layout, uint32_t first, uint32_t count,
    const reshade::api::descriptor_table* tables) {
  if ((static_cast<uint32_t>(stages) & static_cast<uint32_t>(reshade::api::shader_stage::compute)) == 0u) {
    return;
  }
  if (!enabled.load()) return;
  stat_binds.fetch_add(1, std::memory_order_relaxed);
  CommandListData* data = nullptr;
  renodx::utils::data::CreateOrGet(cmd_list, data);
  const std::unique_lock lock(data->mutex);
  data->compute_layout = layout;
  if (data->compute_tables.size() < first + count) {
    data->compute_tables.resize(first + count);
  }
  for (uint32_t i = 0; i < count; ++i) {
    data->compute_tables[first + i] = tables[i];
  }
}

inline void OnResetCommandList(reshade::api::command_list* cmd_list) {
  auto* data = renodx::utils::data::Get<CommandListData>(cmd_list);
  if (data == nullptr) return;
  const std::unique_lock lock(data->mutex);
  data->compute_layout = {0};
  data->compute_tables.clear();
}

inline void OnDestroyCommandList(reshade::api::command_list* cmd_list) {
  renodx::utils::data::Delete<CommandListData>(cmd_list);
}

// Rebuilds the slot's CPU descriptor address from the bound heap (D3D12 heap
// handles are raw ID3D12DescriptorHeap pointers) and resolves it through
// ReShade's view registry. Only destinations of device Create*View calls are
// registered there; copied-to slots resolve through the table mirror instead.
inline reshade::api::resource ResolveSlotDirect(
    reshade::api::device* device, reshade::api::descriptor_heap heap,
    uint32_t slot_index) {
  reshade::api::resource resolved = {0};
#if defined(_MSC_VER)
  __try {
    auto* native_device =
        reinterpret_cast<ID3D12Device*>(device->get_native());
    auto* native_heap =
        reinterpret_cast<ID3D12DescriptorHeap*>(heap.handle);
    if (native_device != nullptr && native_heap != nullptr) {
      const D3D12_DESCRIPTOR_HEAP_TYPE heap_type = native_heap->GetDesc().Type;
      const uint32_t increment =
          native_device->GetDescriptorHandleIncrementSize(heap_type);
      D3D12_CPU_DESCRIPTOR_HANDLE cpu =
          native_heap->GetCPUDescriptorHandleForHeapStart();
      cpu.ptr += static_cast<SIZE_T>(slot_index) * increment;
      resolved =
          device->get_resource_from_view(reshade::api::resource_view{cpu.ptr});
    }
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    stat_av_caught.fetch_add(1, std::memory_order_relaxed);
    resolved = {0};
  }
#endif
  return resolved;
}

// The capture resolves thousands of descriptor slots per second; between
// resolving a slot and querying the registry entry behind it the game can
// destroy or recycle the object, and a stale entry faults inside ReShade.
// Every registry call the capture makes is therefore SEH-guarded: a fault is
// swallowed, counted (av_caught in the report) and treated as a miss instead
// of taking the process down.
inline reshade::api::resource SafeGetResourceFromView(
    reshade::api::device* device, reshade::api::resource_view view) {
  reshade::api::resource resource = {0};
#if defined(_MSC_VER)
  __try {
    resource = device->get_resource_from_view(view);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    stat_av_caught.fetch_add(1, std::memory_order_relaxed);
    resource = {0};
  }
#else
  resource = device->get_resource_from_view(view);
#endif
  return resource;
}

inline uint32_t SafeGetHeapOffset(
    reshade::api::device* device, reshade::api::descriptor_table table,
    uint32_t binding, reshade::api::descriptor_heap* heap) {
  *heap = {0};
  uint32_t offset = 0;
#if defined(_MSC_VER)
  __try {
    device->get_descriptor_heap_offset(table, binding, 0, heap, &offset);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    stat_av_caught.fetch_add(1, std::memory_order_relaxed);
    *heap = {0};
    offset = 0;
  }
#else
  device->get_descriptor_heap_offset(table, binding, 0, heap, &offset);
#endif
  return offset;
}

inline reshade::api::resource_desc SafeGetResourceDesc(
    reshade::api::device* device, reshade::api::resource resource) {
  reshade::api::resource_desc desc = {};
#if defined(_MSC_VER)
  __try {
    desc = device->get_resource_desc(resource);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    stat_av_caught.fetch_add(1, std::memory_order_relaxed);
    desc = {};
  }
#else
  desc = device->get_resource_desc(resource);
#endif
  return desc;
}

inline void CaptureDispatch(reshade::api::command_list* cmd_list) {
  if (!enabled.load()) return;
  stat_dispatches.fetch_add(1, std::memory_order_relaxed);
  auto* shader_state = renodx::utils::shader::GetCurrentState(cmd_list);
  if (shader_state == nullptr) {
    stat_no_shader_state.fetch_add(1, std::memory_order_relaxed);
    return;
  }
  auto* compute_state = renodx::utils::shader::GetCurrentComputeState(shader_state);
  if (compute_state == nullptr) {
    stat_no_compute_state.fetch_add(1, std::memory_order_relaxed);
    return;
  }
  const uint32_t hash = renodx::utils::shader::GetCurrentShaderHash(compute_state);
  const uint64_t pipeline_handle = compute_state->pipeline.handle;
  if (hash == 0u && pipeline_handle == 0u) {
    stat_no_compute_state.fetch_add(1, std::memory_order_relaxed);
    return;
  }
  if (hash != 0u) {
    stat_hashed.fetch_add(1, std::memory_order_relaxed);
  } else {
    // No hash: the pipeline was created outside the hooks (pass map shows a
    // stable set of these per frame). Capture anyway, keyed by pipeline
    // handle, so their bindings are still visible in the report.
    stat_unresolved.fetch_add(1, std::memory_order_relaxed);
  }

  // Once per hash (or once per unresolved pipeline) per frame keeps the walk
  // bounded while still sampling every frame boundary, so double-buffered
  // bindings show up as alternating variants.
  const uint64_t capture_key =
      hash != 0u ? static_cast<uint64_t>(hash) : (1ull << 32u) | pipeline_handle;
  const uint64_t frame = frame_index.load(std::memory_order_relaxed);
  {
    const std::lock_guard lock(throttle_mutex);
    const auto it = last_capture_frame.find(capture_key);
    if (it != last_capture_frame.end() && it->second == frame) {
      stat_throttled.fetch_add(1, std::memory_order_relaxed);
      return;
    }
    last_capture_frame[capture_key] = frame;
  }

  auto* device = cmd_list->get_device();
  auto* descriptor_data = renodx::utils::data::Get<renodx::utils::descriptor::DeviceData>(device);
  auto* data = renodx::utils::data::Get<CommandListData>(cmd_list);
  if (device == nullptr || descriptor_data == nullptr) {
    stat_no_desc_data.fetch_add(1, std::memory_order_relaxed);
    return;
  }
  if (data == nullptr) {
    stat_no_cmd_data.fetch_add(1, std::memory_order_relaxed);
    return;
  }

  const std::unique_lock data_lock(data->mutex);
  const auto* layout_data = renodx::utils::pipeline_layout::GetPipelineLayoutData(data->compute_layout);
  if (layout_data == nullptr) {
    stat_no_layout.fetch_add(1, std::memory_order_relaxed);
    return;
  }

  std::vector<Binding> bindings;
  bindings.reserve(40);
  {
    const std::unique_lock descriptor_lock(descriptor_data->mutex);

    // One-shot inventory: dump the layout/table/heap shape of an early
    // dispatch into the report so any walk break is self-explanatory.
    if (diag_attempts.fetch_add(1, std::memory_order_relaxed) < 3u) {
      std::ostringstream d;
      d << "    hash=0x" << std::hex << std::uppercase << hash << std::dec
        << " layout=0x" << std::hex << data->compute_layout.handle << std::dec
        << " params=" << layout_data->params.size()
        << " tables=" << data->compute_tables.size()
        << " heaps=" << descriptor_data->heaps.size();
      uint64_t heap_slots = 0;
      for (const auto& [heap_handle, slots] : descriptor_data->heaps) {
        (void)heap_handle;
        heap_slots += slots.size();
      }
      d << " heap_slots=" << heap_slots << "\n";
      const size_t report_params =
          layout_data->params.size() < 10u ? layout_data->params.size() : 10u;
      for (size_t p = 0; p < report_params; ++p) {
        const auto& param = layout_data->params[p];
        const uint64_t table_handle =
            p < data->compute_tables.size() ? data->compute_tables[p].handle : 0u;
        d << "    p" << p << ": type=" << static_cast<uint32_t>(param.type)
          << " table=0x" << std::hex << std::uppercase << table_handle << std::dec;
        const bool is_table =
            param.type == reshade::api::pipeline_layout_param_type::descriptor_table
            || param.type == reshade::api::pipeline_layout_param_type::descriptor_table_with_static_samplers;
        if (is_table && p < layout_data->ranges.size()) {
          const auto& ranges = layout_data->ranges[p];
          d << " ranges=" << ranges.size();
          const size_t report_ranges = ranges.size() < 3u ? ranges.size() : 3u;
          for (size_t r = 0; r < report_ranges; ++r) {
            const auto& range = ranges[r];
            d << " r" << r << "{t=" << static_cast<uint32_t>(range.type)
              << " b=" << range.binding << " n=" << range.count
              << " reg=" << range.dx_register_index
              << " sp=" << range.dx_register_space << "}";
          }
        }
        d << "\n";
      }
      const std::lock_guard lock(diag_mutex);
      diag_first = d.str();
    }

    const size_t param_count = layout_data->params.size();
    for (size_t p = 0; p < param_count && p < data->compute_tables.size() && bindings.size() < 40; ++p) {
      const auto& param = layout_data->params[p];
      // D3D12 root signatures with static samplers surface their tables as
      // 'descriptor_table_with_static_samplers'; both forms carry the ranges
      // in the layout util's stable per-param range copy.
      const bool is_table =
          param.type == reshade::api::pipeline_layout_param_type::descriptor_table
          || param.type == reshade::api::pipeline_layout_param_type::descriptor_table_with_static_samplers;
      if (!is_table || p >= layout_data->ranges.size()) continue;
      const auto table = data->compute_tables[p];
      if (table.handle == 0u) continue;
      stat_tables.fetch_add(1, std::memory_order_relaxed);
      const auto& ranges = layout_data->ranges[p];
      for (size_t j = 0; j < ranges.size() && bindings.size() < 40; ++j) {
        const auto& range = ranges[j];
        const bool is_srv = IsSrvRange(range.type);
        const bool is_uav = IsUavRange(range.type);
        if (!is_srv && !is_uav) continue;
        if (range.count == 0u) continue;
        stat_ranges.fetch_add(1, std::memory_order_relaxed);
        reshade::api::descriptor_heap heap = {0};
        const uint32_t base_offset = SafeGetHeapOffset(device, table, range.binding, &heap);
        const auto heap_pair = descriptor_data->heaps.find(heap.handle);
        const auto* slots = heap_pair == descriptor_data->heaps.end() ? nullptr : &heap_pair->second;
        if (slots == nullptr) {
          stat_heap_miss.fetch_add(1, std::memory_order_relaxed);
        }
        const auto record_slot = [&](uint32_t slot_index) {
          reshade::api::resource resource = {0};
          if (slots != nullptr && slot_index < slots->size()) {
            const auto& slot = (*slots)[slot_index];
            if (slot.HasResourceView() && slot.resource_view.handle != 0u) {
              resource = SafeGetResourceFromView(device, slot.resource_view);
            }
          }
          const bool via_mirror = resource.handle != 0u;
          if (!via_mirror) {
            resource = ResolveSlotDirect(device, heap, slot_index);
          }
          if (resource.handle == 0u) {
            stat_slot_miss.fetch_add(1, std::memory_order_relaxed);
            return;
          }
          if (via_mirror) {
            stat_slot_mirror.fetch_add(1, std::memory_order_relaxed);
          } else {
            stat_slot_direct.fetch_add(1, std::memory_order_relaxed);
          }
          const auto desc = SafeGetResourceDesc(device, resource);
          const uint32_t reg = range.dx_register_index + (slot_index - base_offset);
          Binding binding;
          {
            char label[32] = {};
            snprintf(
                label, sizeof label, "%s%u.sp%u",
                is_uav ? "u" : "t", reg, range.dx_register_space);
            binding.label = label;
          }
          binding.handle = resource.handle;
          {
            char description[64] = {};
            if (desc.type == reshade::api::resource_type::buffer) {
              snprintf(
                  description, sizeof description, "[buf %lluB]",
                  static_cast<unsigned long long>(desc.buffer.size));
            } else {
              snprintf(
                  description, sizeof description, "[%ux%u f%u]",
                  desc.texture.width, desc.texture.height,
                  static_cast<uint32_t>(desc.texture.format));
            }
            binding.desc = description;
          }
          bindings.push_back(std::move(binding));
        };
        if (range.count == UINT32_MAX) {
          // Unbounded ranges are the engine's persistent global arrays (raw RT
          // signal, per-tile scheduler masks, probe state). Probe the known
          // persistent slot indices instead of the first N so the report shows
          // which passes hold them as SRV/UAV. The walk is bounds-checked and
          // SEH-guarded, so reading an unpadded table is safe.
          for (uint32_t index : kGlobalArrayProbeIndices) {
            if (bindings.size() >= 40) break;
            record_slot(base_offset + index);
          }
        } else {
          for (uint32_t i = 0; i < range.count && bindings.size() < 40; ++i) {
            record_slot(base_offset + i);
          }
        }
      }
    }
  }
  if (bindings.empty()) return;

  const std::lock_guard lock(captures_mutex);
  Capture& capture =
      hash != 0u ? captures[hash] : unresolved_captures[pipeline_handle];
  capture.counter = capture_counter.fetch_add(1);
  stat_captures.fetch_add(1, std::memory_order_relaxed);
  bool known = false;
  for (const auto& variant : capture.variants) {
    if (variant.size() != bindings.size()) continue;
    bool same = true;
    for (size_t k = 0; k < bindings.size(); ++k) {
      const auto& old = variant[k];
      const auto& current = bindings[k];
      if (old.label != current.label || old.handle != current.handle || old.desc != current.desc) {
        same = false;
        break;
      }
    }
    if (same) {
      known = true;
      break;
    }
  }
  if (!known) {
    ++capture.changes;
    if (capture.variants.size() >= 3u) {
      capture.variants.erase(capture.variants.begin());
    }
    capture.variants.push_back(std::move(bindings));
  }
}

inline bool OnDispatch(
    reshade::api::command_list* cmd_list, uint32_t, uint32_t, uint32_t) {
  CaptureDispatch(cmd_list);
  return false;
}

inline bool OnDispatchIndirect(
    reshade::api::command_list* cmd_list, reshade::api::indirect_command type,
    reshade::api::resource, uint64_t, uint32_t, uint32_t) {
  if (type == reshade::api::indirect_command::draw
      || type == reshade::api::indirect_command::draw_indexed) {
    return false;
  }
  CaptureDispatch(cmd_list);
  return false;
}

inline void OnPresent(
    reshade::api::command_queue*, reshade::api::swapchain*, const reshade::api::rect*,
    const reshade::api::rect*, uint32_t, const reshade::api::rect*) {
  frame_index.fetch_add(1, std::memory_order_relaxed);
}

inline void Use(DWORD fdw_reason) {
  // This must be set before descriptor::Use snapshots it into shared data.
  renodx::utils::descriptor::trace_descriptor_tables = true;
  renodx::utils::descriptor::Use(fdw_reason);
  renodx::utils::shader::Use(fdw_reason);
  renodx::utils::pipeline_layout::Use(fdw_reason);
  switch (fdw_reason) {
    case DLL_PROCESS_ATTACH:
      reshade::register_event<reshade::addon_event::bind_descriptor_tables>(OnBindDescriptorTables);
      reshade::register_event<reshade::addon_event::reset_command_list>(OnResetCommandList);
      reshade::register_event<reshade::addon_event::destroy_command_list>(OnDestroyCommandList);
      reshade::register_event<reshade::addon_event::dispatch>(OnDispatch);
      reshade::register_event<reshade::addon_event::draw_or_dispatch_indirect>(OnDispatchIndirect);
      reshade::register_event<reshade::addon_event::present>(OnPresent);
      break;
    case DLL_PROCESS_DETACH:
      reshade::unregister_event<reshade::addon_event::bind_descriptor_tables>(OnBindDescriptorTables);
      reshade::unregister_event<reshade::addon_event::reset_command_list>(OnResetCommandList);
      reshade::unregister_event<reshade::addon_event::destroy_command_list>(OnDestroyCommandList);
      reshade::unregister_event<reshade::addon_event::dispatch>(OnDispatch);
      reshade::unregister_event<reshade::addon_event::draw_or_dispatch_indirect>(OnDispatchIndirect);
      reshade::unregister_event<reshade::addon_event::present>(OnPresent);
      break;
  }
}

// Formats the changed handles between an older variant and the current one:
// alternating handles are the ping-pong/history resources.
inline std::string BuildVariantDiff(
    const std::vector<Binding>& old_bindings,
    const std::vector<Binding>& new_bindings) {
  std::stringstream d;
  for (const auto& current : new_bindings) {
    const Binding* old = nullptr;
    for (const auto& candidate : old_bindings) {
      if (candidate.label == current.label) {
        old = &candidate;
        break;
      }
    }
    if (old == nullptr) {
      d << "+" << current.label << " ";
    } else if (old->handle != current.handle) {
      char text[128] = {};
      snprintf(
          text, sizeof text, "%s 0x%llX->0x%llX%s ",
          current.label.c_str(),
          static_cast<unsigned long long>(old->handle),
          static_cast<unsigned long long>(current.handle),
          current.desc.c_str());
      d << text;
    }
  }
  for (const auto& old : old_bindings) {
    bool dropped = true;
    for (const auto& current : new_bindings) {
      if (current.label == old.label) {
        dropped = false;
        break;
      }
    }
    if (dropped) d << "-" << old.label << " ";
  }
  return d.str();
}

// Appends one capture (current variant bindings + alt diffs) to the report.
inline void AppendCapture(
    std::stringstream& s, const Capture& capture, const char* header) {
  s << header;
  for (const auto& binding : capture.variants.back()) {
    char text[128] = {};
    snprintf(
        text, sizeof text, "%s=0x%llX%s ",
        binding.label.c_str(),
        static_cast<unsigned long long>(binding.handle),
        binding.desc.c_str());
    s << text;
  }
  s << "\n";
  const std::vector<Binding>& current = capture.variants.back();
  for (size_t v = 0; v + 1 < capture.variants.size(); ++v) {
    const std::string diff = BuildVariantDiff(capture.variants[v], current);
    if (!diff.empty()) {
      s << "    alt" << v << ": " << diff << "\n";
    }
  }
}

inline std::string BuildReportSection() {
  std::stringstream s;
  s << "\n[dataflow]\n";
  s << "  counters: dispatch=" << stat_dispatches.load()
    << " bind=" << stat_binds.load()
    << " hashed=" << stat_hashed.load()
    << " throttled=" << stat_throttled.load()
    << " captures=" << stat_captures.load()
    << " unresolved=" << stat_unresolved.load()
    << " frames=" << frame_index.load() << "\n";
  s << "  filter: no_state=" << stat_no_shader_state.load()
    << " no_compute=" << stat_no_compute_state.load()
    << " no_desc_data=" << stat_no_desc_data.load()
    << " no_cmd_data=" << stat_no_cmd_data.load()
    << " no_layout=" << stat_no_layout.load() << "\n";
  s << "  walk: tables=" << stat_tables.load()
    << " ranges=" << stat_ranges.load()
    << " heap_miss=" << stat_heap_miss.load()
    << " mirror=" << stat_slot_mirror.load()
    << " direct=" << stat_slot_direct.load()
    << " miss=" << stat_slot_miss.load()
    << " av_caught=" << stat_av_caught.load() << "\n";
  {
    const std::lock_guard lock(diag_mutex);
    if (!diag_first.empty()) {
      s << "  early dispatch:\n" << diag_first;
    }
  }
  {
    const std::lock_guard lock(captures_mutex);
    if (captures.empty() && unresolved_captures.empty()) {
      s << "  no dispatch captured yet\n";
      return s.str();
    }
    std::vector<uint32_t> order;
    order.reserve(captures.size());
    for (const auto& [hash, capture] : captures) {
      (void)capture;
      order.push_back(hash);
    }
    std::sort(order.begin(), order.end());
    for (uint32_t hash : order) {
      const auto& capture = captures.find(hash)->second;
      if (capture.variants.empty()) continue;
      const char* name = pass_map::KnownName(hash);
      char header[128] = {};
      snprintf(
          header, sizeof header, "  0x%08X %s(#%u, v%u): ",
          hash, name != nullptr ? name : "", capture.counter,
          static_cast<uint32_t>(capture.variants.size()));
      AppendCapture(s, capture, header);
    }
    if (!unresolved_captures.empty()) {
      std::vector<uint64_t> handles;
      handles.reserve(unresolved_captures.size());
      for (const auto& [handle, capture] : unresolved_captures) {
        (void)capture;
        handles.push_back(handle);
      }
      std::sort(handles.begin(), handles.end());
      s << "  unresolved pipelines (bindings by pipeline handle):\n";
      for (uint64_t handle : handles) {
        const auto& capture = unresolved_captures.find(handle)->second;
        if (capture.variants.empty()) continue;
        char header[128] = {};
        snprintf(
            header, sizeof header, "   pipeline=0x%llX (#%u, v%u): ",
            static_cast<unsigned long long>(handle), capture.counter,
            static_cast<uint32_t>(capture.variants.size()));
        AppendCapture(s, capture, header);
      }
    }
  }
  return s.str();
}

}  // namespace dataflow
