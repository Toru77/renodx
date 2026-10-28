// GPU geometry cache (G2), present side: the D3D11 index-range and decode passes into a carrier ring, the D3D12 copies
// into the cache's own arenas and one BLAS per mesh, the on-request check against the CPU pool, and the JSON dump.
// Validation only: nothing traces this geometry. The draw side (hooks, staging reservation, transcriptions) is in
// bvh/gpu_capture.hpp. Submission order and fences are as rt12_backend.hpp (D14).
// G3 adds the GPU instance set (step 7b) and the on-request instance compare (validation only, not traced).
#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <cstring>
#include <deque>
#include <fstream>
#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "rt12_backend.hpp"

namespace falcom_world::rt12 {

// Geometry bytes of a mesh in the arenas (16 B vertex, 4 B index, 8 B UV pair per vertex).
inline uint64_t GcMeshBytes(uint32_t vertex_count, uint32_t index_count, bool has_uv) {
  return uint64_t{vertex_count} * kRt12ArenaStride[kRt12ArenaVertices] + uint64_t{index_count} * kRt12ArenaStride[kRt12ArenaIndices]
         + (has_uv ? uint64_t{vertex_count} * kRt12ArenaStride[kRt12ArenaUvs] : 0u);
}

struct Rt12GcMesh {
  uint64_t v_off = 0u, i_off = 0u, u_off = 0u;  // element offsets in the arenas (16, 4 and 8 bytes)
  uint32_t vertex_count = 0u, index_count = 0u;
  bool has_uv = false;
  Microsoft::WRL::ComPtr<ID3D12Resource> blas;
  uint64_t blas_bytes = 0u;
  uint32_t vb_usage = 0u, vb_flags = 0u, ib_usage = 0u, ib_flags = 0u, pos_format = 0u, index_size = 0u;
};

// The last on-request check (D17). `split` counts the meshes and triangles with a mismatch, by buffer kind and format.
struct Rt12GcCheck {
  struct Sample {
    uint64_t key = 0u;
    uint32_t triangle = 0u;
    float gpu[9] = {};
    float cpu[9] = {};
  };
  bool run = false;
  uint64_t checked = 0u, triangles = 0u, mismatched_triangles = 0u, mismatched_meshes = 0u, cpu_absent = 0u;
  uint64_t uv_cpu_dropped = 0u, uv_size_mismatch = 0u, timeouts = 0u;
  std::map<std::string, uint64_t> cpu_failed;  // the pool's failure reason of a key with no CPU mesh
  double wait_ms = 0.0;
  std::map<std::array<uint32_t, 6>, std::array<uint64_t, 2>> split;  // vb usage, vb flags, ib usage, ib flags, pos code, index size
  std::vector<Sample> samples;                                        // up to 8
};

struct Rt12GcSettle {
  uint64_t meshes = 0u, presents = 0u;
  double ms = 0.0;
};

struct Rt12GcPanel {
  std::string state = "off";
  std::string failed_stage;
  HRESULT hr = S_OK;
  bool leaked = false;  // the state was leaked at an owner mismatch
  bvh::GcStats stats;
  uint64_t phase_counts[6] = {};  // by bvh::GcPhase
  uint64_t resident = 0u, pending = 0u, pool_meshes = 0u, blas_queue = 0u, presents = 0u;
  uint64_t arena_used[3] = {}, arena_capacity[3] = {};  // bytes
  uint64_t dead_bytes = 0u, geometry_bytes = 0u, geometry_peak = 0u, blas_bytes = 0u, blas_peak = 0u, blas_cap_waits = 0u;
  float build_ms_last = -1.f, decode_ms_last = -1.f;
  double build_ms_sum = 0.0, decode_ms_sum = 0.0;
  uint64_t build_ms_n = 0u, decode_ms_n = 0u;
  std::vector<Rt12GcSettle> settle;  // the last 8
  Rt12GcCheck check;
  uint32_t inst_counters[bvh::kGcCounterCount] = {};  // G3, the last read present
  uint64_t inst_readback_busy = 0u;
  float inst_ms_last = -1.f;
  double inst_ms_sum = 0.0;
  uint64_t inst_ms_n = 0u;
  bvh::GcInstanceCompare compare;
};

struct Rt12GcState {
  Rt12State* owner = nullptr;
  // D3D11: the index-range and decode passes (D7 to D9).
  reshade::api::resource staging = {0u}, results = {0u};
  reshade::api::resource readback[bvh::kGcReadbackSlots] = {};
  reshade::api::resource_view staging_srv = {0u}, results_uav = {0u};
  reshade::api::pipeline_layout layout = {0u};
  reshade::api::descriptor_table srv_table = {0u}, uav_table = {0u};
  reshade::api::pipeline range_pipeline = {0u}, decode_pipeline = {0u};
  bvh::GpuTimer timer;
  std::vector<bvh::GcJob> readback_jobs[bvh::kGcReadbackSlots];  // range jobs whose results are read once their query is done
  Microsoft::WRL::ComPtr<ID3D11Query> readback_done[bvh::kGcReadbackSlots];  // event query after each slot's copy
  uint32_t present = 0u;
  // The carrier ring (D10): one shared texture per present slot.
  Rt12Shared carriers[kRt12Slots];
  uint64_t carrier_fence[kRt12Slots] = {};
  uint32_t carrier_next = 0u;
  // D3D12 (D11 to D13): the copy source, the arenas, the slots and the BLAS.
  Microsoft::WRL::ComPtr<ID3D12Resource> linear;
  Rt12Arena arenas[3];
  uint64_t dead_bytes = 0u;
  Rt12Slot slots[kRt12Slots];
  Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> list;
  uint32_t gc_next = 0u;
  bool timed[kRt12Slots] = {};  // the slot's build timestamps are in query_readback
  Microsoft::WRL::ComPtr<ID3D12QueryHeap> queries;
  Microsoft::WRL::ComPtr<ID3D12Resource> query_readback;
  std::unordered_map<uint64_t, Rt12GcMesh> meshes;
  std::deque<uint64_t> build_queue;  // FIFO by mesh key
  uint64_t blas_bytes = 0u, geometry_peak = 0u, blas_peak = 0u, blas_cap_waits = 0u;
  float build_ms_last = -1.f;
  double build_ms_sum = 0.0;
  uint64_t build_ms_n = 0u;
  // Pending meshes over time (the settle line).
  uint64_t pending_peak = 0u;
  uint32_t pending_since = 0u;
  LARGE_INTEGER pending_qpc = {};
  std::vector<Rt12GcSettle> settle;
  Rt12GcCheck check;
  // G3 (D3, D4): the instance set, the D3D11 passes and the counter readback ring.
  reshade::api::resource inst_arena = {0u}, inst_draws = {0u}, inst_epochs = {0u}, inst_meta = {0u}, inst_entries = {0u};
  reshade::api::resource inst_records = {0u}, inst_counters_buffer = {0u}, inst_moving = {0u};
  reshade::api::resource inst_readback[bvh::kGcReadbackSlots] = {};
  reshade::api::resource_view inst_arena_srv = {0u}, inst_draws_srv = {0u}, inst_epochs_srv = {0u};
  reshade::api::resource_view inst_meta_uav = {0u}, inst_entries_uav = {0u}, inst_records_uav = {0u}, inst_counters_uav = {0u},
                              inst_moving_uav = {0u};
  reshade::api::pipeline_layout inst_layout = {0u};
  reshade::api::descriptor_table inst_srv_table = {0u}, inst_uav_table = {0u};
  reshade::api::pipeline insert_pipeline = {0u}, compact_pipeline = {0u};
  bvh::GpuTimer inst_timer;
  Microsoft::WRL::ComPtr<ID3D11Query> inst_done[bvh::kGcReadbackSlots];
  Microsoft::WRL::ComPtr<ID3D11Query> compare_done;
  bool inst_pending[bvh::kGcReadbackSlots] = {};
  uint32_t inst_counters[bvh::kGcCounterCount] = {};
  uint32_t inst_current = 0u;  // the half of the set that holds the last present's entries
  uint64_t inst_readback_busy = 0u;
  uint64_t inst_bytes = 0u;    // D3D11 bytes of the G3 objects (log and world_gc.json)
  bvh::GcInstanceCompare compare;
  bool failed = false;
  const char* failed_stage = "";
  HRESULT hr = S_OK;
};

inline std::mutex g_gc_panel_mutex;
inline Rt12GcPanel g_gc_panel;
inline std::unique_ptr<Rt12GcState> g_rt12_gc;
inline std::atomic<bool> g_gc_check_requested{false}, g_gc_dump_requested{false}, g_gc_compare_requested{false};

// A GC failure before any work is submitted: the D3D11 side is deactivated (the M5 path continues).
inline void FailGc(Rt12GcState& gc, const char* stage, HRESULT hr) {
  gc.failed = true;
  gc.failed_stage = stage;
  gc.hr = hr;
  bvh::g_gpu_capture.active.store(false, std::memory_order_relaxed);
  {
    std::lock_guard lock(bvh::g_gpu_capture.mutex);
    bvh::g_gpu_capture.staging = {0u};
    bvh::g_gpu_capture.staging_used = 0u;
    bvh::g_gpu_capture.range_jobs.clear();
    bvh::g_gpu_capture.decode_jobs.clear();
    bvh::g_gpu_capture.instance_arena = {0u};
    bvh::g_gpu_capture.instance_arena_used = 0u;
    bvh::g_gpu_capture.instance_draws.clear();
  }
  renodx::utils::log::w("[world-gc] failed at ", stage, " hr=", renodx::utils::log::AsHex(static_cast<uint32_t>(hr)));
}

// Stops the draw side. No graphics call: the objects it held are released by the caller or leaked.
inline void StopGcDrawSide() {
  bvh::g_gpu_capture.active.store(false, std::memory_order_relaxed);
  std::lock_guard lock(bvh::g_gpu_capture.mutex);
  bvh::g_gpu_capture.entries.clear();
  bvh::g_gpu_capture.range_jobs.clear();
  bvh::g_gpu_capture.decode_jobs.clear();
  bvh::g_gpu_capture.invalidated.clear();
  bvh::g_gpu_capture.staging = {0u};
  bvh::g_gpu_capture.staging_used = 0u;
  bvh::g_gpu_capture.instance_arena = {0u};
  bvh::g_gpu_capture.instance_arena_used = 0u;
  bvh::g_gpu_capture.instance_draws.clear();
  bvh::g_gpu_capture.next_slot = 0u;
  bvh::g_gpu_capture.free_slots.clear();
  std::fill(bvh::g_gpu_capture.slot_key.begin(), bvh::g_gpu_capture.slot_key.end(), 0u);
  std::fill(bvh::g_gpu_capture.slot_epoch.begin(), bvh::g_gpu_capture.slot_epoch.end(), 0u);
  bvh::g_gpu_capture.epochs_dirty = true;
}

// Stops the cache (D16 and D15 teardown). A stuck or missing DXR state leaks its objects on purpose.
inline void TeardownGc(reshade::api::device* device, const char* why) {
  StopGcDrawSide();
  if (!g_rt12_gc) return;
  if (!g_rt12 || g_rt12->stuck) {
    g_rt12_gc.release();  // leaked on purpose (the GPU may still touch it)
    renodx::utils::log::w("[world-gc] teardown (", why, "): leaked");
    return;
  }
  Rt12GcState& gc = *g_rt12_gc;
  Rt12State& s = *g_rt12;
  for (Rt12Shared& carrier : gc.carriers) RetireShared(s, carrier, s.last_signal);
  for (reshade::api::resource_view view : {gc.staging_srv, gc.results_uav, gc.inst_arena_srv, gc.inst_draws_srv, gc.inst_epochs_srv,
                                           gc.inst_meta_uav, gc.inst_entries_uav, gc.inst_records_uav, gc.inst_counters_uav,
                                           gc.inst_moving_uav}) {
    if (view.handle != 0u) device->destroy_resource_view(view);
  }
  for (reshade::api::resource resource : {gc.staging, gc.results, gc.readback[0], gc.readback[1], gc.readback[2], gc.inst_arena,
                                          gc.inst_draws, gc.inst_epochs, gc.inst_meta, gc.inst_entries, gc.inst_records,
                                          gc.inst_counters_buffer, gc.inst_moving, gc.inst_readback[0], gc.inst_readback[1],
                                          gc.inst_readback[2]}) {
    if (resource.handle != 0u) device->destroy_resource(resource);
  }
  if (gc.srv_table.handle != 0u) device->free_descriptor_table(gc.srv_table);
  if (gc.uav_table.handle != 0u) device->free_descriptor_table(gc.uav_table);
  if (gc.inst_srv_table.handle != 0u) device->free_descriptor_table(gc.inst_srv_table);
  if (gc.inst_uav_table.handle != 0u) device->free_descriptor_table(gc.inst_uav_table);
  if (gc.range_pipeline.handle != 0u) device->destroy_pipeline(gc.range_pipeline);
  if (gc.decode_pipeline.handle != 0u) device->destroy_pipeline(gc.decode_pipeline);
  if (gc.insert_pipeline.handle != 0u) device->destroy_pipeline(gc.insert_pipeline);
  if (gc.compact_pipeline.handle != 0u) device->destroy_pipeline(gc.compact_pipeline);
  if (gc.layout.handle != 0u) device->destroy_pipeline_layout(gc.layout);
  if (gc.inst_layout.handle != 0u) device->destroy_pipeline_layout(gc.inst_layout);
  bvh::DestroyGpuTimer(&gc.timer);
  bvh::DestroyGpuTimer(&gc.inst_timer);
  RetireObject(s, s.last_signal, gc.linear);
  for (Rt12Arena& arena : gc.arenas) RetireObject(s, s.last_signal, arena.buffer);
  for (auto& [mesh_key, mesh] : gc.meshes) RetireObject(s, s.last_signal, mesh.blas);
  for (Rt12Slot& slot : gc.slots) RetireObject(s, s.last_signal, slot.allocator);
  RetireObject(s, s.last_signal, gc.list);
  RetireObject(s, s.last_signal, gc.queries);
  RetireObject(s, s.last_signal, gc.query_readback);
  g_rt12_gc.reset();
  renodx::utils::log::i("[world-gc] teardown (", why, ")");
}

inline void DestroyGc(reshade::api::device* device) {
  if (g_rt12_gc && g_rt12 && g_rt12->device == device) TeardownGc(device, "device destroyed");
}

// Creates the D3D11 passes and the D3D12 objects (D7, D10, D11, D12 setup). Returns false and records the stage on failure.
inline bool CreateGc(reshade::api::device* device, Rt12State& s, Rt12GcState& gc) {
#if defined(_WIN32)
  using Microsoft::WRL::ComPtr;
  using RU = reshade::api::resource_usage;
  using DS = reshade::api::shader_stage;
  using DT = reshade::api::descriptor_type;
  using DR = reshade::api::descriptor_range;
  using P = reshade::api::pipeline_layout_param;
  gc.owner = &s;
  const auto fail = [&](const char* stage, HRESULT hr) {
    gc.failed = true;
    gc.failed_stage = stage;
    gc.hr = hr;
    renodx::utils::log::w("[world-gc] create failed at ", stage, " hr=", renodx::utils::log::AsHex(static_cast<uint32_t>(hr)));
    return false;
  };

  // D3D11: staging (D7), results, three readback slots, the tables and the pipelines.
  if (!device->create_resource(reshade::api::resource_desc(bvh::kGcStagingBytes, reshade::api::memory_heap::gpu_only,
                                                           RU::shader_resource | RU::copy_dest),
                               nullptr, RU::shader_resource, &gc.staging)) {
    return fail("staging", E_FAIL);
  }
  if (!device->create_resource_view(gc.staging, RU::shader_resource,
                                    reshade::api::resource_view_desc(reshade::api::resource_view_type::buffer,
                                                                     reshade::api::format::r32_typeless, 0, bvh::kGcStagingBytes / 4u),
                                    &gc.staging_srv)) {
    return fail("staging view", E_FAIL);
  }
  if (!bvh::CreatePoolBuffer(device, nullptr, bvh::kGcJobsPerPresent * 16u, 16u, &gc.results, &gc.results_uav,
                             RU::unordered_access | RU::copy_source | RU::copy_dest)) {
    return fail("results", E_FAIL);
  }
  for (reshade::api::resource& readback : gc.readback) {
    if (!device->create_resource(reshade::api::resource_desc(bvh::kGcJobsPerPresent * 16u, reshade::api::memory_heap::gpu_to_cpu,
                                                             RU::copy_dest),
                                 nullptr, RU::copy_dest, &readback)) {
      return fail("readback", E_FAIL);
    }
  }
  auto* native11 = reinterpret_cast<ID3D11Device*>(static_cast<uintptr_t>(device->get_native()));  // NOLINT(performance-no-int-to-ptr)
  const D3D11_QUERY_DESC event_desc = {D3D11_QUERY_EVENT, 0u};
  for (Microsoft::WRL::ComPtr<ID3D11Query>& query : gc.readback_done) {
    const HRESULT query_hr = native11->CreateQuery(&event_desc, &query);
    if (FAILED(query_hr)) return fail("readback query", query_hr);
  }
  const auto make_layout = [&](uint32_t srv_count, uint32_t uav_count, reshade::api::pipeline_layout* out) {
    DR srv_range = {0, 0, 0, srv_count, DS::all_compute, 1, DT::shader_resource_view};
    DR uav_range = {0, 0, 0, uav_count, DS::all_compute, 1, DT::unordered_access_view};
    reshade::api::constant_range push_range = {};
    push_range.binding = 0;
    push_range.dx_register_index = 0;
    push_range.dx_register_space = 0;
    push_range.count = 16;
    push_range.visibility = DS::all_compute;
    P params[3] = {};
    params[0].type = reshade::api::pipeline_layout_param_type::descriptor_table;
    params[0].descriptor_table.count = 1;
    params[0].descriptor_table.ranges = &srv_range;
    params[1].type = reshade::api::pipeline_layout_param_type::descriptor_table;
    params[1].descriptor_table.count = 1;
    params[1].descriptor_table.ranges = &uav_range;
    params[2].type = reshade::api::pipeline_layout_param_type::push_constants;
    params[2].push_constants = push_range;
    return device->create_pipeline_layout(3, params, out);
  };
  if (!make_layout(1u, 2u, &gc.layout)) return fail("layout", E_FAIL);
  if (!device->allocate_descriptor_table(gc.layout, 0, &gc.srv_table)
      || !device->allocate_descriptor_table(gc.layout, 1, &gc.uav_table)) {
    return fail("descriptor tables", E_FAIL);
  }
  const auto make = [&](reshade::api::pipeline_layout layout, auto& code, reshade::api::pipeline* out) {
    reshade::api::shader_desc shader = {};
    shader.code = code.data();
    shader.code_size = code.size();
    shader.entry_point = "main";
    reshade::api::pipeline_subobject subobject = {reshade::api::pipeline_subobject_type::compute_shader, 1, &shader};
    return device->create_pipeline(layout, 1, &subobject, out);
  };
#if defined(__world_gc_range_EMBED_FILE)
  if (!make(gc.layout, __world_gc_range, &gc.range_pipeline)) return fail("range pipeline", E_FAIL);
#else
  return fail("shader not embedded", E_FAIL);
#endif
#if defined(__world_gc_decode_EMBED_FILE)
  if (!make(gc.layout, __world_gc_decode, &gc.decode_pipeline)) return fail("decode pipeline", E_FAIL);
#else
  return fail("shader not embedded", E_FAIL);
#endif
  bvh::CreateGpuTimer(device, &gc.timer);

  // G3: the instance arena, the instance set (two halves), the draws, the epochs and moving table, the counters and their
  // readback ring (D3D11 only).
  if (!device->create_resource(reshade::api::resource_desc(bvh::kGcInstanceArenaBytes, reshade::api::memory_heap::gpu_only,
                                                           RU::shader_resource | RU::copy_dest),
                               nullptr, RU::shader_resource, &gc.inst_arena)) {
    return fail("instance arena", E_FAIL);
  }
  if (!device->create_resource_view(gc.inst_arena, RU::shader_resource,
                                    reshade::api::resource_view_desc(reshade::api::resource_view_type::buffer,
                                                                     reshade::api::format::r32_typeless, 0,
                                                                     bvh::kGcInstanceArenaBytes / 4u),
                                    &gc.inst_arena_srv)) {
    return fail("instance arena view", E_FAIL);
  }
  gc.inst_bytes += bvh::kGcInstanceArenaBytes;
  if (!bvh::CreatePoolBuffer(device, nullptr, bvh::kGcInstanceDrawsPerPresent * sizeof(bvh::GcInstanceDrawGPU),
                             sizeof(bvh::GcInstanceDrawGPU), &gc.inst_draws, &gc.inst_draws_srv, RU::shader_resource | RU::copy_dest)) {
    return fail("instance draws", E_FAIL);
  }
  gc.inst_bytes += bvh::kGcInstanceDrawsPerPresent * sizeof(bvh::GcInstanceDrawGPU);
  const std::vector<uint32_t> zero_slots(bvh::kGcMaxEntries, 0u);
  if (!bvh::CreatePoolBuffer(device, zero_slots.data(), bvh::kGcMaxEntries * 4u, 4u, &gc.inst_epochs, &gc.inst_epochs_srv,
                             RU::shader_resource | RU::copy_dest)) {
    return fail("instance epochs", E_FAIL);
  }
  if (!bvh::CreatePoolBuffer(device, zero_slots.data(), bvh::kGcMaxEntries * 4u, 4u, &gc.inst_moving, &gc.inst_moving_uav,
                             RU::unordered_access)) {
    return fail("instance moving", E_FAIL);
  }
  gc.inst_bytes += 2u * bvh::kGcMaxEntries * 4u;
  const std::vector<uint32_t> zero_meta(size_t{2} * bvh::kGcSetCapacity * 4u, 0u);
  if (!bvh::CreatePoolBuffer(device, zero_meta.data(), zero_meta.size() * 4u, 4u, &gc.inst_meta, &gc.inst_meta_uav,
                             RU::unordered_access)) {
    return fail("instance meta", E_FAIL);
  }
  gc.inst_bytes += zero_meta.size() * 4u;
  if (!bvh::CreatePoolBuffer(device, nullptr, size_t{2} * bvh::kGcSetCapacity * sizeof(bvh::GcSetEntryGPU),
                             sizeof(bvh::GcSetEntryGPU), &gc.inst_entries, &gc.inst_entries_uav, RU::unordered_access)) {
    return fail("instance entries", E_FAIL);
  }
  gc.inst_bytes += size_t{2} * bvh::kGcSetCapacity * sizeof(bvh::GcSetEntryGPU);
  if (!bvh::CreatePoolBuffer(device, nullptr, (bvh::kGcSetCapacity + bvh::kGcMovingRecords) * sizeof(bvh::WorldInstanceGPU),
                             sizeof(bvh::WorldInstanceGPU), &gc.inst_records, &gc.inst_records_uav,
                             RU::unordered_access | RU::copy_source)) {
    return fail("instance records", E_FAIL);
  }
  gc.inst_bytes += (bvh::kGcSetCapacity + bvh::kGcMovingRecords) * sizeof(bvh::WorldInstanceGPU);
  if (!bvh::CreatePoolBuffer(device, nullptr, bvh::kGcCounterCount * 4u, 4u, &gc.inst_counters_buffer, &gc.inst_counters_uav,
                             RU::unordered_access | RU::copy_source | RU::copy_dest)) {
    return fail("instance counters", E_FAIL);
  }
  gc.inst_bytes += bvh::kGcCounterCount * 4u;
  for (reshade::api::resource& readback : gc.inst_readback) {
    if (!device->create_resource(reshade::api::resource_desc(bvh::kGcCounterCount * 4u, reshade::api::memory_heap::gpu_to_cpu,
                                                             RU::copy_dest),
                                 nullptr, RU::copy_dest, &readback)) {
      return fail("instance readback", E_FAIL);
    }
    gc.inst_bytes += bvh::kGcCounterCount * 4u;
  }
  for (Microsoft::WRL::ComPtr<ID3D11Query>& query : gc.inst_done) {
    const HRESULT query_hr = native11->CreateQuery(&event_desc, &query);
    if (FAILED(query_hr)) return fail("instance query", query_hr);
  }
  const HRESULT compare_query_hr = native11->CreateQuery(&event_desc, &gc.compare_done);
  if (FAILED(compare_query_hr)) return fail("compare query", compare_query_hr);
  if (!make_layout(3u, 5u, &gc.inst_layout)) return fail("instance layout", E_FAIL);
  if (!device->allocate_descriptor_table(gc.inst_layout, 0, &gc.inst_srv_table)
      || !device->allocate_descriptor_table(gc.inst_layout, 1, &gc.inst_uav_table)) {
    return fail("instance descriptor tables", E_FAIL);
  }
#if defined(__world_gc_insert_EMBED_FILE)
  if (!make(gc.inst_layout, __world_gc_insert, &gc.insert_pipeline)) return fail("insert pipeline", E_FAIL);
#else
  return fail("shader not embedded", E_FAIL);
#endif
#if defined(__world_gc_compact_EMBED_FILE)
  if (!make(gc.inst_layout, __world_gc_compact, &gc.compact_pipeline)) return fail("compact pipeline", E_FAIL);
#else
  return fail("shader not embedded", E_FAIL);
#endif
  bvh::CreateGpuTimer(device, &gc.inst_timer);

  // The carrier ring (D10): R32G32B32A32_UINT, 4096 x 512 each.
  for (Rt12Shared& carrier : gc.carriers) {
    const HRESULT hr = CreateRt12Shared(s, device, DXGI_FORMAT_R32G32B32A32_UINT, bvh::kGcCarrierWidth, bvh::kGcCarrierHeight, 1u,
                                        true, &carrier);
    if (FAILED(hr)) return fail("carrier", hr);
  }

  // D3D12: the linear copy source (D11), the allocators, the list (created and closed), the timestamp heap.
  HRESULT hr = CreateRt12Buffer(s.device12.Get(), D3D12_HEAP_TYPE_DEFAULT, bvh::kGcCarrierTexels * 16u, D3D12_RESOURCE_STATE_COMMON,
                                D3D12_RESOURCE_FLAG_NONE, gc.linear);
  if (FAILED(hr)) return fail("linear", hr);
  for (Rt12Slot& slot : gc.slots) {
    hr = s.device12->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&slot.allocator));
    if (FAILED(hr)) return fail("gc allocator", hr);
  }
  hr = s.device12->CreateCommandList(0u, D3D12_COMMAND_LIST_TYPE_DIRECT, gc.slots[0].allocator.Get(), nullptr, IID_PPV_ARGS(&gc.list));
  if (FAILED(hr)) return fail("gc list", hr);
  hr = gc.list->Close();
  if (FAILED(hr)) return fail("gc list close", hr);
  const D3D12_QUERY_HEAP_DESC query_desc = {D3D12_QUERY_HEAP_TYPE_TIMESTAMP, kRt12Slots * 2u, 0u};
  hr = s.device12->CreateQueryHeap(&query_desc, IID_PPV_ARGS(&gc.queries));
  if (FAILED(hr)) return fail("gc query heap", hr);
  hr = CreateRt12Buffer(s.device12.Get(), D3D12_HEAP_TYPE_READBACK, kRt12Slots * 2u * 8u, D3D12_RESOURCE_STATE_COPY_DEST,
                        D3D12_RESOURCE_FLAG_NONE, gc.query_readback);
  if (FAILED(hr)) return fail("gc query readback", hr);

  // Publish the staging buffer to the draw side, then switch the hooks on.
  {
    std::lock_guard lock(bvh::g_gpu_capture.mutex);
    bvh::g_gpu_capture.staging = gc.staging;
    bvh::g_gpu_capture.staging_used = 0u;
    bvh::g_gpu_capture.instance_arena = gc.inst_arena;
    bvh::g_gpu_capture.instance_arena_used = 0u;
    bvh::g_gpu_capture.instance_draws.clear();
    bvh::g_gpu_capture.epochs_dirty = true;
  }
  bvh::g_gpu_capture.active.store(true, std::memory_order_relaxed);
  renodx::utils::log::i("[world-gc] active (staging 16 MB, carriers 3 x 32 MB, linear 32 MB, instance set ", gc.inst_bytes >> 20u, " MB)");
  return true;
#else
  (void)device;
  (void)s;
  (void)gc;
  return false;
#endif
}

// The on-request check (D17): up to kGcCheckMeshes resident meshes, copied back from D3D12 and compared with the CPU
// pool mesh of the same key, bitwise per triangle. The wait is bounded by kRt12TeardownMs.
inline void RunGcCheck(Rt12State& s, Rt12GcState& gc) {
  using Microsoft::WRL::ComPtr;
  const uint64_t completed = s.fence12->GetCompletedValue();
  const uint32_t k = gc.gc_next;
  if (gc.slots[k].fence_value > completed) {
    g_gc_check_requested.store(true, std::memory_order_relaxed);  // no free slot: retry at the next present
    return;
  }
  struct CheckCase {
    uint64_t key = 0u;
    const Rt12GcMesh* gpu = nullptr;
    std::vector<std::array<float, 3>> positions;
    std::vector<uint32_t> indices;
    std::vector<std::array<float, 2>> uvs;
    uint64_t vertex_dst = 0u, index_dst = 0u, uv_dst = 0u;  // readback byte offsets
  };
  std::vector<uint64_t> keys;
  for (const auto& [key, mesh] : gc.meshes) {
    if (mesh.blas) keys.push_back(key);
  }
  std::sort(keys.begin(), keys.end());
  if (keys.size() > bvh::kGcCheckMeshes) keys.resize(bvh::kGcCheckMeshes);
  std::vector<CheckCase> cases;
  uint64_t cpu_absent = 0u;
  std::map<std::string, uint64_t> cpu_failed;
  {
    std::lock_guard lock(bvh::g_pool.mutex);
    for (const uint64_t key : keys) {
      const auto it = bvh::g_pool.mesh_by_key.find(key);
      if (it == bvh::g_pool.mesh_by_key.end() || it->second >= bvh::g_pool.meshes.size()) {
        const auto failed = bvh::g_pool.failed_meshes.find(key);
        if (failed != bvh::g_pool.failed_meshes.end()) {
          cpu_failed[failed->second.reason] += 1u;
        } else {
          cpu_absent += 1u;
        }
        continue;
      }
      const bvh::WorldMesh& mesh = bvh::g_pool.meshes[it->second];
      CheckCase check_case;
      check_case.key = key;
      check_case.gpu = &gc.meshes.at(key);
      check_case.positions = mesh.positions;
      check_case.indices = mesh.indices;
      check_case.uvs = mesh.uvs;
      cases.push_back(std::move(check_case));
    }
  }
  Rt12GcCheck result;
  result.run = true;
  result.cpu_absent = cpu_absent;
  result.cpu_failed = cpu_failed;
  uint64_t total = 0u;
  for (CheckCase& check_case : cases) {
    const Rt12GcMesh& mesh = *check_case.gpu;
    check_case.vertex_dst = total;
    total += uint64_t{mesh.vertex_count} * 16u;
    check_case.index_dst = total;
    total += uint64_t{mesh.index_count} * 4u;
    check_case.uv_dst = total;
    if (mesh.has_uv) total += uint64_t{mesh.vertex_count} * 8u;
  }
  if (total == 0u) {
    gc.check = result;
    return;
  }

  // Copy the arena ranges into a readback buffer on the free slot, wait for it (bounded), then compare.
  ComPtr<ID3D12Resource> readback;
  HRESULT hr = CreateRt12Buffer(s.device12.Get(), D3D12_HEAP_TYPE_READBACK, total, D3D12_RESOURCE_STATE_COPY_DEST,
                                D3D12_RESOURCE_FLAG_NONE, readback);
  if (FAILED(hr)) {
    renodx::utils::log::w("[world-gc] check: readback buffer hr=", renodx::utils::log::AsHex(static_cast<uint32_t>(hr)));
    return;
  }
  Rt12Slot& slot = gc.slots[k];
  if (FAILED(slot.allocator->Reset()) || FAILED(gc.list->Reset(slot.allocator.Get(), nullptr))) {
    FailGc(gc, "check reset", E_FAIL);
    return;
  }
  std::vector<D3D12_RESOURCE_BARRIER> barriers;
  for (Rt12Arena& arena : gc.arenas) Transition(barriers, arena.buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
  gc.list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
  for (const CheckCase& check_case : cases) {
    const Rt12GcMesh& mesh = *check_case.gpu;
    gc.list->CopyBufferRegion(readback.Get(), check_case.vertex_dst, gc.arenas[kRt12ArenaVertices].buffer.Get(), mesh.v_off * 16u,
                              uint64_t{mesh.vertex_count} * 16u);
    gc.list->CopyBufferRegion(readback.Get(), check_case.index_dst, gc.arenas[kRt12ArenaIndices].buffer.Get(), mesh.i_off * 4u,
                              uint64_t{mesh.index_count} * 4u);
    if (mesh.has_uv) {
      gc.list->CopyBufferRegion(readback.Get(), check_case.uv_dst, gc.arenas[kRt12ArenaUvs].buffer.Get(), mesh.u_off * 8u,
                                uint64_t{mesh.vertex_count} * 8u);
    }
  }
  barriers.clear();
  for (Rt12Arena& arena : gc.arenas) Transition(barriers, arena.buffer.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COMMON);
  gc.list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
  if (FAILED(gc.list->Close())) {
    FailGc(gc, "check close", E_FAIL);
    return;
  }
  bool ordered = true;
  if (s.last_signal != 0u) {
    try {
      ordered = renodx::utils::gpu_fence::QueueWait(s.queue12.Get(), s.fence12.get(), s.last_signal);
    } catch (const std::exception&) {
      ordered = false;
    }
    if (!ordered) {
      FailSession(s, Rt12Reason::SubmitFailed, "gc check order wait");
      return;
    }
  }
  ID3D12CommandList* lists[] = {gc.list.Get()};
  s.queue12->ExecuteCommandLists(1u, lists);
  uint64_t value = renodx::utils::gpu_fence::NO_SIGNAL;
  try {
    value = renodx::utils::gpu_fence::QueueSignal(s.queue12.Get(), s.fence12.get());
  } catch (const std::exception&) {
    value = renodx::utils::gpu_fence::NO_SIGNAL;
  }
  if (value == renodx::utils::gpu_fence::NO_SIGNAL) {
    FailSession(s, Rt12Reason::SubmitFailed, "gc check signal");
    return;
  }
  slot.fence_value = value;
  gc.gc_next = (k + 1u) % kRt12Slots;
  s.last_signal = value;
  s.last_signal_qpc = QpcNow();

  const LARGE_INTEGER start = QpcNow();
  while (s.fence12->GetCompletedValue() < value && QpcMsSince(start) < static_cast<double>(kRt12TeardownMs)) Sleep(1);
  result.wait_ms = QpcMsSince(start);
  if (s.fence12->GetCompletedValue() < value) {
    result.timeouts = 1u;
    gc.check = result;
    RetireObject(s, value, readback);  // not read: the GPU may still write it
    renodx::utils::log::w("[world-gc] check: timeout after ", result.wait_ms, " ms");
    return;
  }

  void* mapped = nullptr;
  if (FAILED(readback->Map(0u, nullptr, &mapped))) {
    gc.check = result;
    RetireObject(s, value, readback);
    renodx::utils::log::w("[world-gc] check: map failed");
    return;
  }
  const uint8_t* data = static_cast<const uint8_t*>(mapped);
  for (const CheckCase& check_case : cases) {
    const Rt12GcMesh& mesh = *check_case.gpu;
    const bvh::GcCompareResult compare = bvh::GcCompareTriangles(
        reinterpret_cast<const float*>(data + check_case.vertex_dst), mesh.vertex_count,
        reinterpret_cast<const uint32_t*>(data + check_case.index_dst), mesh.index_count,
        mesh.has_uv ? reinterpret_cast<const float*>(data + check_case.uv_dst) : nullptr, check_case.positions, check_case.indices,
        check_case.uvs);
    result.checked += 1u;
    result.triangles += compare.triangles;
    if (mesh.has_uv && check_case.uvs.empty()) result.uv_cpu_dropped += 1u;
    if (compare.uv_size_mismatch) result.uv_size_mismatch += 1u;
    if (compare.mismatched == 0u) continue;
    result.mismatched_meshes += 1u;
    result.mismatched_triangles += compare.mismatched;
    auto& split = result.split[{mesh.vb_usage, mesh.vb_flags, mesh.ib_usage, mesh.ib_flags, mesh.pos_format, mesh.index_size}];
    split[0] += 1u;
    split[1] += compare.mismatched;
    if (result.samples.size() < 8u) {
      Rt12GcCheck::Sample sample;
      sample.key = check_case.key;
      sample.triangle = static_cast<uint32_t>(compare.first);
      std::memcpy(sample.gpu, compare.gpu, sizeof(sample.gpu));
      std::memcpy(sample.cpu, compare.cpu, sizeof(sample.cpu));
      result.samples.push_back(sample);
    }
  }
  readback->Unmap(0u, nullptr);
  RetireObject(s, value, readback);
  gc.check = result;
  renodx::utils::log::i("[world-gc] check: ", result.checked, " meshes, ", result.triangles, " triangles, ",
                        result.mismatched_triangles, " mismatched in ", result.mismatched_meshes, " meshes, cpu absent ",
                        result.cpu_absent, ", wait ", result.wait_ms, " ms");
}

// Writes world_gc_compare_f<present>.json (schema 1) with one compare result. A failed write is logged and ignored.
inline void DumpGcCompareJson(const bvh::GcInstanceCompare& cmp, uint32_t present, const uint32_t (&counters)[bvh::kGcCounterCount]) {
  const bool timed_out = cmp.timeouts > 0u;  // nothing was read or compared: every count is null
  const auto num = [timed_out](uint64_t value) { return timed_out ? std::string("null") : std::to_string(value); };
  const auto count_of = [&cmp](const char* reason) {
    const auto it = cmp.cpu_only_by_reason.find(reason);
    return it == cmp.cpu_only_by_reason.end() ? uint64_t{0u} : it->second;
  };
  uint64_t refused = 0u;
  for (const auto& [reason, count] : cmp.cpu_only_by_reason) {
    if (reason.rfind("refused:", 0) == 0) refused += count;
  }
  const uint64_t done = count_of("done");
  const uint64_t ratio_base = cmp.matched + done;
  const uint32_t moving_overflow = counters[bvh::kGcCounterMovingRecords] > bvh::kGcMovingRecords
                                       ? counters[bvh::kGcCounterMovingRecords] - bvh::kGcMovingRecords
                                       : 0u;
  std::ostringstream out;
  out << "{\n  \"schema\": 1,\n  \"present\": " << present << ",\n";
  out << "  \"set_count\": " << num(cmp.set_count) << ", \"moving_records\": " << num(cmp.moving_records) << ", \"gpu_records\": "
      << num(cmp.gpu_records) << ", \"cpu_instances\": " << num(cmp.cpu_instances) << ", \"matched\": " << num(cmp.matched) << ",\n";
  out << "  \"gpu_only\": {\"count\": " << num(cmp.gpu_only) << ", \"alpha\": " << num(cmp.gpu_only_alpha)
      << ", \"alpha_conflict\": " << num(cmp.gpu_only_alpha_conflict) << "},\n";
  out << "  \"gpu_no_cpu_mesh\": " << num(cmp.gpu_no_cpu_mesh) << ", \"gpu_dup_content\": " << num(cmp.gpu_dup_content)
      << ", \"cpu_dup\": " << num(cmp.cpu_dup) << ",\n";
  out << "  \"cpu_only\": {\"count\": " << num(cmp.cpu_only) << ", \"by_reason\": {\"done\": " << num(done)
      << ", \"done_unseen\": " << num(count_of("done_unseen")) << ", \"dynamic\": " << num(count_of("dynamic")) << ", \"pending\": "
      << num(count_of("pending")) << ", \"refused\": " << num(refused) << ", \"no_entry\": " << num(count_of("no_entry")) << "}},\n";
  out << "  \"match_ratio\": ";
  if (timed_out) {
    out << "null";
  } else {
    out << bvh::PoolJsonFloat{ratio_base > 0u ? static_cast<float>(cmp.matched) / static_cast<float>(ratio_base) : -1.0f};
  }
  out << ",\n";
  out << "  \"overflow\": {\"set\": " << num(counters[bvh::kGcCounterSetOverflow]) << ", \"rehash\": "
      << num(counters[bvh::kGcCounterRehashOverflow]) << ", \"moving\": " << num(moving_overflow) << "},\n";
  out << "  \"timeouts\": " << cmp.timeouts << ", \"wait_ms\": " << bvh::PoolJsonFloat{static_cast<float>(cmp.wait_ms)} << "\n}\n";
  const std::filesystem::path path = bvh::PoolOutputDir() / ("world_gc_compare_f" + std::to_string(present) + ".json");
  std::ofstream file(path);
  file << out.str();
  file.close();
  if (!file) renodx::utils::log::w("[world-gc] compare json write failed");
}

// On request (G3 D9, D10): reads the instance set of the last present back (bounded by kRt12TeardownMs) and counts matched,
// GPU-only and CPU-only instances against the CPU pool. Validation only: nothing traces the set.
inline void RunGcInstanceCompare(reshade::api::device* device, reshade::api::command_list* cmd_list, ID3D11DeviceContext* context11,
                                 Rt12GcState& gc) {
  using RU = reshade::api::resource_usage;
  const uint64_t records_bytes = uint64_t{bvh::kGcSetCapacity} * sizeof(bvh::WorldInstanceGPU);
  const uint64_t counters_bytes = uint64_t{bvh::kGcCounterCount} * 4u;
  reshade::api::resource readback = {0u};
  if (!device->create_resource(reshade::api::resource_desc(records_bytes + counters_bytes, reshade::api::memory_heap::gpu_to_cpu,
                                                           RU::copy_dest),
                               nullptr, RU::copy_dest, &readback)) {
    renodx::utils::log::w("[world-gc] compare: readback buffer failed");
    return;
  }
  cmd_list->barrier(gc.inst_records, RU::unordered_access, RU::copy_source);
  cmd_list->copy_buffer_region(gc.inst_records, 0u, readback, 0u, records_bytes);
  cmd_list->barrier(gc.inst_records, RU::copy_source, RU::unordered_access);
  cmd_list->copy_buffer_region(gc.inst_counters_buffer, 0u, readback, records_bytes, counters_bytes);
  context11->End(gc.compare_done.Get());
  context11->Flush();

  const LARGE_INTEGER start = QpcNow();
  bool finished = false;
  while (QpcMsSince(start) < static_cast<double>(kRt12TeardownMs)) {
    BOOL done = FALSE;
    if (context11->GetData(gc.compare_done.Get(), &done, sizeof(done), 0u) == S_OK && done) {
      finished = true;
      break;
    }
    Sleep(1);
  }
  bvh::GcInstanceCompare result;
  result.wait_ms = QpcMsSince(start);
  if (!finished) {
    result.run = true;
    result.timeouts = 1u;
    device->destroy_resource(readback);  // not read: the GPU may still write it
    gc.compare = result;
    renodx::utils::log::w("[world-gc] compare: timeout after ", result.wait_ms, " ms");
    const uint32_t no_counters[bvh::kGcCounterCount] = {};  // not read after a timeout
    DumpGcCompareJson(result, gc.present, no_counters);
    return;
  }

  void* mapped = nullptr;
  if (!device->map_buffer_region(readback, 0u, records_bytes + counters_bytes, reshade::api::map_access::read_only, &mapped)) {
    device->destroy_resource(readback);
    renodx::utils::log::w("[world-gc] compare: map failed");
    return;
  }
  const auto* records = static_cast<const bvh::WorldInstanceGPU*>(mapped);
  uint32_t counters[bvh::kGcCounterCount] = {};
  std::memcpy(counters, static_cast<const uint8_t*>(mapped) + records_bytes, sizeof(counters));
  const uint32_t set_count = (std::min)(counters[bvh::kGcCounterSet], bvh::kGcSetCapacity);
  std::vector<uint32_t> slots(set_count);
  std::vector<bvh::GcCompareGpuInstance> gpu(set_count);
  for (uint32_t r = 0u; r < set_count; ++r) {
    slots[r] = static_cast<uint32_t>(records[r].header[0]);
    std::memcpy(gpu[r].world, records[r].world, sizeof(gpu[r].world));
    gpu[r].flags = static_cast<uint32_t>(records[r].visibility[2]);
  }
  device->unmap_buffer_region(readback);
  device->destroy_resource(readback);

  {
    std::lock_guard lock(bvh::g_gpu_capture.mutex);
    for (uint32_t r = 0u; r < set_count; ++r) {
      gpu[r].mesh_key = slots[r] < bvh::kGcMaxEntries ? bvh::g_gpu_capture.slot_key[slots[r]] : 0u;
    }
  }
  std::vector<bvh::GcCompareCpuInstance> cpu;
  std::unordered_set<uint64_t> cpu_mesh_keys;
  {
    std::lock_guard lock(bvh::g_pool.mutex);
    for (bvh::GcCompareGpuInstance& instance : gpu) {
      const auto mesh_it = bvh::g_pool.mesh_by_key.find(instance.mesh_key);
      if (instance.mesh_key == 0u || mesh_it == bvh::g_pool.mesh_by_key.end() || mesh_it->second >= bvh::g_pool.meshes.size()) continue;
      instance.mesh_id = static_cast<int64_t>(mesh_it->second);
      instance.cpu_alpha_conflict = bvh::g_pool.meshes[mesh_it->second].alpha_state.conflict;
    }
    const uint32_t frame = g_state.frame.load();
    for (const bvh::WorldInstance& instance : bvh::g_pool.instances) {
      bvh::GcCompareCpuInstance cpu_instance;
      cpu_instance.mesh_key = instance.mesh_key;
      cpu_instance.mesh_id = instance.mesh_id;
      std::memcpy(cpu_instance.world, instance.matrix, sizeof(cpu_instance.world));
      cpu_instance.dynamic = instance.dynamic;
      cpu_instance.alpha = instance.mesh_id < bvh::g_pool.meshes.size() && bvh::g_pool.meshes[instance.mesh_id].alpha;
      cpu_instance.vs_hash = instance.source_vs_hash;
      const auto observed = bvh::g_pool.observations.find(
          bvh::InstanceKey{instance.mesh_key, MatrixHash(instance.matrix, bvh::kPoolWorldFloats)});
      cpu_instance.recently_seen = observed != bvh::g_pool.observations.end() && frame - observed->second.last_frame <= bvh::kPoolPruneAge;
      cpu_mesh_keys.insert(instance.mesh_key);
      cpu.push_back(cpu_instance);
    }
  }
  std::unordered_map<uint64_t, std::string> reasons;
  {
    std::lock_guard lock(bvh::g_gpu_capture.mutex);
    for (const uint64_t key : cpu_mesh_keys) {
      const auto entry_it = bvh::g_gpu_capture.entries.find(key);
      if (entry_it == bvh::g_gpu_capture.entries.end()) continue;
      const bvh::GcEntry& entry = entry_it->second;
      if (entry.phase == bvh::GcPhase::Done) {
        reasons[key] = "done";
      } else if (entry.phase == bvh::GcPhase::Refused) {
        reasons[key] = std::string("refused:") + entry.reason;
      } else {
        reasons[key] = "pending";
      }
    }
  }
  result = bvh::GcCompareInstances(gpu, cpu, reasons);
  result.wait_ms = QpcMsSince(start);
  result.set_count = set_count;
  result.moving_records = counters[bvh::kGcCounterMovingRecords];
  gc.compare = result;
  std::string reason_text;
  for (const auto& [reason, count] : result.cpu_only_by_reason) {
    reason_text += (reason_text.empty() ? "" : ",") + reason + ":" + std::to_string(count);
  }
  renodx::utils::log::i("[world-gc] compare: set ", result.set_count, ", cpu ", result.cpu_instances, ", matched ", result.matched,
                        ", gpu-only ", result.gpu_only, " (alpha ", result.gpu_only_alpha, ", conflict ", result.gpu_only_alpha_conflict,
                        "), no cpu mesh ", result.gpu_no_cpu_mesh, ", cpu-only ", result.cpu_only, " (", reason_text, "), wait ",
                        result.wait_ms, " ms");
  DumpGcCompareJson(result, gc.present, counters);
}

// Writes world_gc.json (schema 1) from a copy of the panel, the draw-side stats, and the resident meshes.
inline void DumpGcJson(const Rt12GcState& gc) {
  Rt12GcPanel p;
  {
    std::lock_guard lock(g_gc_panel_mutex);
    p = g_gc_panel;
  }
  uint64_t entries = 0u;
  for (const uint64_t count : p.phase_counts) entries += count;
  // Duplicates: resident meshes whose CPU mesh_id an earlier resident mesh (in key order) already has.
  uint64_t with_cpu = 0u, shared = 0u, dup_bytes = 0u, geometry_bytes = 0u;
  std::vector<uint64_t> keys;
  for (const auto& [key, mesh] : gc.meshes) keys.push_back(key);
  std::sort(keys.begin(), keys.end());
  {
    std::lock_guard lock(bvh::g_pool.mutex);
    std::unordered_set<uint32_t> seen;
    for (const uint64_t key : keys) {
      const Rt12GcMesh& mesh = gc.meshes.at(key);
      const uint64_t bytes = GcMeshBytes(mesh.vertex_count, mesh.index_count, mesh.has_uv);
      geometry_bytes += bytes;
      const auto it = bvh::g_pool.mesh_by_key.find(key);
      if (it == bvh::g_pool.mesh_by_key.end() || it->second >= bvh::g_pool.meshes.size()) continue;
      with_cpu += 1u;
      if (!seen.insert(bvh::g_pool.meshes[it->second].mesh_id).second) {
        shared += 1u;
        dup_bytes += bytes;
      }
    }
  }
  const double dup_ratio = geometry_bytes > 0u ? static_cast<double>(dup_bytes) / static_cast<double>(geometry_bytes) : 0.0;
  const auto format_name = [](uint32_t code) {
    switch (code) {
      case bvh::kGcFormatF32x2: return renodx::utils::scene::FormatToString(reshade::api::format::r32g32_float);
      case bvh::kGcFormatF32x3: return renodx::utils::scene::FormatToString(reshade::api::format::r32g32b32_float);
      case bvh::kGcFormatF32x4: return renodx::utils::scene::FormatToString(reshade::api::format::r32g32b32a32_float);
      case bvh::kGcFormatF16x2: return renodx::utils::scene::FormatToString(reshade::api::format::r16g16_float);
      case bvh::kGcFormatF16x4: return renodx::utils::scene::FormatToString(reshade::api::format::r16g16b16a16_float);
      default: return std::string("none");
    }
  };
  const Rt12GcCheck& check = p.check;
  std::ostringstream out;
  out << "{\n  \"schema\": 1,\n";
  out << "  \"state\": \"" << bvh::PoolJsonEscape(p.state) << "\",\n";
  out << "  \"failed_stage\": \"" << bvh::PoolJsonEscape(p.failed_stage) << "\",\n";
  out << "  \"hr\": " << static_cast<uint32_t>(p.hr) << ",\n";
  out << "  \"leaked\": " << (p.leaked ? "true" : "false") << ",\n";
  out << "  \"presents\": " << p.presents << ",\n";
  out << "  \"phases\": {\"range\": " << p.phase_counts[0] << ", \"range_wait\": " << p.phase_counts[1]
      << ", \"data\": " << p.phase_counts[2] << ", \"data_wait\": " << p.phase_counts[3] << ", \"done\": " << p.phase_counts[4]
      << ", \"refused\": " << p.phase_counts[5] << "},\n";
  out << "  \"resident\": " << p.resident << ", \"pending\": " << p.pending << ", \"pool_meshes\": " << p.pool_meshes
      << ", \"blas_queue\": " << p.blas_queue << ",\n";
  const bvh::GcStats& st = p.stats;
  out << "  \"counts\": {\"created\": " << st.created << ", \"range_jobs\": " << st.range_jobs << ", \"decode_jobs\": " << st.decode_jobs
      << ", \"decoded\": " << st.decoded << ", \"deferred_skips\": " << st.deferred_skips << ", \"staging_full_attempts\": " << st.staging_full
      << ", \"jobs_cap_attempts\": " << st.jobs_cap << ", \"cap_attempts\": " << st.cap_attempts << ", \"carrier_busy\": " << st.carrier_busy
      << ", \"carrier_full\": " << st.carrier_full << ", \"stale_jobs\": " << st.stale_jobs << ", \"invalidated_write\": " << st.invalidated_write
      << ", \"uv_stream_mismatch\": " << st.uv_stream_mismatch << ", \"invalidated_destroy\": " << st.invalidated_destroy
      << ", \"resets_pool\": " << st.resets_pool
      << ", \"resets_fragmented\": " << st.resets_fragmented << ", \"readback_busy\": " << st.readback_busy << "},\n";
  out << "  \"refused\": {";
  bool first = true;
  for (const auto& [reason, count] : st.refused) {
    out << (first ? "" : ", ") << "\"" << bvh::PoolJsonEscape(reason) << "\": " << count;
    first = false;
  }
  out << "},\n  \"formats\": [";
  first = true;
  for (const auto& [format_key, count] : st.formats) {
    out << (first ? "\n    " : ",\n    ") << "{\"pos\": \"" << format_name(format_key[0]) << "\", \"uv\": \""
        << format_name(format_key[1]) << "\", \"index_size\": " << format_key[2] << ", \"stride\": " << format_key[3]
        << ", \"count\": " << count << "}";
    first = false;
  }
  out << "],\n  \"buffers\": [";
  first = true;
  for (const auto& [buffer_key, count] : st.buffers) {
    out << (first ? "\n    " : ",\n    ") << "{\"vb\": \"" << bvh::PoolJsonEscape(bvh::PoolBufferText(buffer_key[0], buffer_key[1]))
        << "\", \"ib\": \"" << bvh::PoolJsonEscape(bvh::PoolBufferText(buffer_key[2], buffer_key[3])) << "\", \"count\": " << count << "}";
    first = false;
  }
  out << "],\n  \"rewritten_by_buffer\": [";
  first = true;
  for (const auto& [buffer_key, count] : st.rewritten_by_buffer) {
    out << (first ? "\n    " : ",\n    ") << "{\"vb\": \"" << bvh::PoolJsonEscape(bvh::PoolBufferText(buffer_key[0], buffer_key[1]))
        << "\", \"ib\": \"" << bvh::PoolJsonEscape(bvh::PoolBufferText(buffer_key[2], buffer_key[3])) << "\", \"count\": " << count << "}";
    first = false;
  }
  out << "],\n  \"largest\": {\"vertex_range\": " << st.largest_vertex_range << ", \"index_count\": " << st.largest_index_count << "},\n";
  out << "  \"memory\": {\"geometry_used_bytes\": " << (p.arena_used[0] + p.arena_used[1] + p.arena_used[2])
      << ", \"geometry_cap_bytes\": " << bvh::kGcGeometryBytesMax << ", \"geometry_live_bytes\": " << p.geometry_bytes
      << ", \"dead_bytes\": " << p.dead_bytes << ", \"arena_used_bytes\": [" << p.arena_used[0] << ", " << p.arena_used[1] << ", "
      << p.arena_used[2] << "], \"arena_capacity_bytes\": [" << p.arena_capacity[0] << ", " << p.arena_capacity[1] << ", "
      << p.arena_capacity[2] << "], \"blas_bytes\": " << p.blas_bytes << ", \"blas_cap_bytes\": " << bvh::kGcBlasBytesMax
      << ", \"geometry_peak_bytes\": " << p.geometry_peak << ", \"blas_peak_bytes\": " << p.blas_peak
      << ", \"blas_cap_waits\": " << p.blas_cap_waits << ", \"d3d11_bytes\": " << (bvh::kGcStagingBytes + 3u * 32ull * 1024ull * 1024ull)
      << ", \"linear_bytes\": " << (32ull * 1024ull * 1024ull) << "},\n";
  out << "  \"timing\": {\"build_gpu_ms_last\": " << bvh::PoolJsonFloat{p.build_ms_last} << ", \"build_gpu_ms_avg\": "
      << bvh::PoolJsonFloat{static_cast<float>(p.build_ms_n > 0u ? p.build_ms_sum / static_cast<double>(p.build_ms_n) : -1.0)}
      << ", \"build_gpu_ms_n\": " << p.build_ms_n << ", \"decode_gpu_ms_last\": " << bvh::PoolJsonFloat{p.decode_ms_last}
      << ", \"decode_gpu_ms_avg\": "
      << bvh::PoolJsonFloat{static_cast<float>(p.decode_ms_n > 0u ? p.decode_ms_sum / static_cast<double>(p.decode_ms_n) : -1.0)}
      << ", \"decode_gpu_ms_n\": " << p.decode_ms_n << "},\n";
  out << "  \"duplicates\": {\"resident_entries\": " << keys.size() << ", \"with_cpu_mesh\": " << with_cpu
      << ", \"shared_cpu_mesh\": " << shared << ", \"dup_bytes\": " << dup_bytes << ", \"geometry_bytes\": " << geometry_bytes
      << ", \"dup_ratio\": " << bvh::PoolJsonFloat{static_cast<float>(dup_ratio)} << "},\n";
  out << "  \"settle\": [";
  for (size_t i = 0u; i < p.settle.size(); ++i) {
    out << (i != 0u ? ", " : "") << "{\"meshes\": " << p.settle[i].meshes << ", \"presents\": " << p.settle[i].presents
        << ", \"ms\": " << bvh::PoolJsonFloat{static_cast<float>(p.settle[i].ms)} << "}";
  }
  out << "],\n  \"check\": {\"run\": " << (check.run ? "true" : "false") << ", \"checked\": " << check.checked
      << ", \"triangles\": " << check.triangles << ", \"mismatched_triangles\": " << check.mismatched_triangles
      << ", \"mismatched_meshes\": " << check.mismatched_meshes << ", \"cpu_absent\": " << check.cpu_absent
      << ", \"uv_cpu_dropped\": " << check.uv_cpu_dropped << ", \"uv_size_mismatch\": " << check.uv_size_mismatch
      << ", \"timeouts\": " << check.timeouts
      << ", \"wait_ms\": " << bvh::PoolJsonFloat{static_cast<float>(check.wait_ms)} << ",\n    \"cpu_failed\": {";
  first = true;
  for (const auto& [reason, count] : check.cpu_failed) {
    out << (first ? "" : ", ") << "\"" << bvh::PoolJsonEscape(reason) << "\": " << count;
    first = false;
  }
  out << "},\n    \"split\": [";
  first = true;
  for (const auto& [split_key, split] : check.split) {
    out << (first ? "" : ", ") << "{\"vb\": \"" << bvh::PoolJsonEscape(bvh::PoolBufferText(split_key[0], split_key[1]))
        << "\", \"ib\": \"" << bvh::PoolJsonEscape(bvh::PoolBufferText(split_key[2], split_key[3])) << "\", \"pos\": \""
        << format_name(split_key[4]) << "\", \"index_size\": " << split_key[5] << ", \"meshes\": " << split[0]
        << ", \"triangles\": " << split[1] << "}";
    first = false;
  }
  out << "],\n    \"samples\": [";
  for (size_t i = 0u; i < check.samples.size(); ++i) {
    const Rt12GcCheck::Sample& sample = check.samples[i];
    out << (i != 0u ? ", " : "") << "{\"key\": " << sample.key << ", \"triangle\": " << sample.triangle << ", \"gpu\": [";
    for (uint32_t k = 0u; k < 9u; ++k) out << (k != 0u ? ", " : "") << bvh::PoolJsonFloat{sample.gpu[k]};
    out << "], \"cpu\": [";
    for (uint32_t k = 0u; k < 9u; ++k) out << (k != 0u ? ", " : "") << bvh::PoolJsonFloat{sample.cpu[k]};
    out << "]}";
  }
  out << "]},\n  \"instances\": {\"capacity\": " << bvh::kGcSetCapacity << ", \"moving_capacity\": " << bvh::kGcMovingRecords
      << ",\n    \"counters\": {\"set\": " << p.inst_counters[bvh::kGcCounterSet]
      << ", \"inserted\": " << p.inst_counters[bvh::kGcCounterInserted] << ", \"hits\": " << p.inst_counters[bvh::kGcCounterHits]
      << ", \"kept\": " << p.inst_counters[bvh::kGcCounterKept] << ", \"aged\": " << p.inst_counters[bvh::kGcCounterAged]
      << ", \"dropped_epoch\": " << p.inst_counters[bvh::kGcCounterDroppedEpoch]
      << ", \"dropped_moving\": " << p.inst_counters[bvh::kGcCounterDroppedMoving]
      << ", \"moving_records\": " << p.inst_counters[bvh::kGcCounterMovingRecords]
      << ", \"moving_held\": " << p.inst_counters[bvh::kGcCounterMovingHeld] << ", \"invalid\": " << p.inst_counters[bvh::kGcCounterInvalid]
      << ", \"stale_epoch\": " << p.inst_counters[bvh::kGcCounterStaleEpoch]
      << ", \"set_overflow\": " << p.inst_counters[bvh::kGcCounterSetOverflow]
      << ", \"rehash_overflow\": " << p.inst_counters[bvh::kGcCounterRehashOverflow]
      << ", \"marked\": " << p.inst_counters[bvh::kGcCounterMarked] << ", \"sightings\": " << p.inst_counters[bvh::kGcCounterSightings]
      << "},\n    \"draw_side\": {\"draws\": " << p.stats.inst_draws << ", \"instances\": " << p.stats.inst_instances
      << ", \"no_geometry\": " << p.stats.inst_no_geometry << ", \"deferred\": " << p.stats.inst_deferred
      << ", \"arena_full\": " << p.stats.inst_arena_full << ", \"draws_cap\": " << p.stats.inst_draws_cap << "},\n"
      << "    \"readback_busy\": " << p.inst_readback_busy
      << ", \"gpu_ms_avg\": " << bvh::PoolJsonFloat{static_cast<float>(p.inst_ms_n > 0u ? p.inst_ms_sum / static_cast<double>(p.inst_ms_n) : -1.0)}
      << ", \"d3d11_bytes\": " << gc.inst_bytes << ",\n";
  const bvh::GcInstanceCompare& cmp = p.compare;
  out << "    \"compare\": {\"run\": " << (cmp.run ? "true" : "false") << ", \"set_count\": " << cmp.set_count
      << ", \"moving_records\": " << cmp.moving_records << ", \"gpu_records\": " << cmp.gpu_records
      << ", \"cpu_instances\": " << cmp.cpu_instances << ", \"matched\": " << cmp.matched << ", \"gpu_only\": " << cmp.gpu_only
      << ", \"gpu_only_alpha\": " << cmp.gpu_only_alpha << ", \"gpu_only_alpha_conflict\": " << cmp.gpu_only_alpha_conflict
      << ", \"gpu_no_cpu_mesh\": " << cmp.gpu_no_cpu_mesh << ", \"gpu_dup_content\": " << cmp.gpu_dup_content
      << ", \"cpu_dup\": " << cmp.cpu_dup << ", \"cpu_only\": " << cmp.cpu_only << ", \"timeouts\": " << cmp.timeouts
      << ", \"wait_ms\": " << bvh::PoolJsonFloat{static_cast<float>(cmp.wait_ms)} << ",\n      \"cpu_only_by_reason\": {";
  bool first_reason = true;
  for (const auto& [reason, count] : cmp.cpu_only_by_reason) {
    out << (first_reason ? "" : ", ") << "\"" << bvh::PoolJsonEscape(reason) << "\": " << count;
    first_reason = false;
  }
  out << "},\n      \"cpu_only_samples\": [";
  for (size_t i = 0u; i < cmp.cpu_only_samples.size(); ++i) {
    const bvh::GcInstanceCompare::CpuOnly& sample = cmp.cpu_only_samples[i];
    out << (i != 0u ? ", " : "") << "{\"mesh_key\": " << sample.mesh_key << ", \"mesh_id\": " << sample.mesh_id << ", \"translation\": ["
        << bvh::PoolJsonFloat{sample.translation[0]} << ", " << bvh::PoolJsonFloat{sample.translation[1]} << ", "
        << bvh::PoolJsonFloat{sample.translation[2]} << "], \"reason\": \"" << bvh::PoolJsonEscape(sample.reason)
        << "\", \"alpha\": " << (sample.alpha ? "true" : "false") << ", \"vs\": \"" << bvh::PoolJsonEscape(bvh::PoolHashText(sample.vs_hash))
        << "\"}";
  }
  out << "],\n      \"gpu_only_samples\": [";
  for (size_t i = 0u; i < cmp.gpu_only_samples.size(); ++i) {
    const bvh::GcInstanceCompare::GpuOnly& sample = cmp.gpu_only_samples[i];
    out << (i != 0u ? ", " : "") << "{\"mesh_key\": " << sample.mesh_key << ", \"mesh_id\": " << sample.mesh_id << ", \"translation\": ["
        << bvh::PoolJsonFloat{sample.translation[0]} << ", " << bvh::PoolJsonFloat{sample.translation[1]} << ", "
        << bvh::PoolJsonFloat{sample.translation[2]} << "], \"flags\": " << sample.flags << "}";
  }
  out << "]}}\n}\n";
  const std::filesystem::path path = bvh::PoolOutputDir() / "world_gc.json";
  std::string text = out.str();
  renodx::utils::path::WriteTextFile(path, text);
  std::string refused_text;
  for (const auto& [reason, count] : st.refused) {
    refused_text += (refused_text.empty() ? "" : ",") + reason + ":" + std::to_string(count);
  }
  renodx::utils::log::i("[world-gc] census: entries=", entries, " resident=", keys.size(), " pending=", p.pending,
                        " geometry_mb=", static_cast<double>(geometry_bytes) / (1024.0 * 1024.0),
                        " blas_mb=", static_cast<double>(p.blas_bytes) / (1024.0 * 1024.0), " dup_ratio=", dup_ratio,
                        " refused=", refused_text);
  renodx::utils::log::i("[world-gc] wrote world_gc.json");
}

// One present of the cache (D14 order: after UpdateLiveBvh, before rt12::OnPresent). Steps 1 to 12 of the G2 plan.
inline void OnGcPresent(reshade::api::device* device, reshade::api::command_queue* queue) {
#if defined(_WIN32)
  using Microsoft::WRL::ComPtr;
  using RU = reshade::api::resource_usage;
  using DS = reshade::api::shader_stage;
  using DT = reshade::api::descriptor_type;
  const bool switch_on = bvh::g_gpu_capture.enabled.load(std::memory_order_relaxed);
  const bool dxr_on = g_rt12_enabled.load(std::memory_order_relaxed);
  const bool dxr_ok = g_rt12 && g_rt12->device == device && !g_rt12->failed && !g_rt12->stuck;
  // The state belongs to an earlier DXR state whose device is gone: no graphics call on its objects. Leaked on purpose.
  if (g_rt12_gc && g_rt12 && g_rt12_gc->owner != g_rt12.get()) {
    StopGcDrawSide();
    g_rt12_gc.release();
    renodx::utils::log::w("[world-gc] teardown (owner mismatch): leaked");
    std::lock_guard lock(g_gc_panel_mutex);
    g_gc_panel.state = "off";
    g_gc_panel.leaked = true;
  }
  // 1. Effective only with the switch, DXR on and a healthy DXR state.
  if (!switch_on || !dxr_on || !dxr_ok) {
    if (g_rt12_gc) {
      const char* why = "dxr failed";
      if (!switch_on) {
        why = "switch off";
      } else if (!dxr_on) {
        why = "dxr off";
      }
      TeardownGc(device, why);
      std::lock_guard lock(g_gc_panel_mutex);
      g_gc_panel.state = "off";
    }
    return;
  }
  if (!g_rt12_gc) {
    g_rt12_gc = std::make_unique<Rt12GcState>();
    CreateGc(device, *g_rt12, *g_rt12_gc);  // the work starts at the next present
    return;
  }
  Rt12State& s = *g_rt12;
  Rt12GcState& gc = *g_rt12_gc;
  if (gc.failed) {
    std::lock_guard lock(g_gc_panel_mutex);
    g_gc_panel.state = "failed";
    g_gc_panel.failed_stage = gc.failed_stage;
    g_gc_panel.hr = gc.hr;
    return;
  }

  // 2. Completed work: the build timestamps of the finished GC slots, and the D3D11 decode time (read in Begin below).
  const uint64_t completed = s.fence12->GetCompletedValue();
  for (uint32_t k = 0u; k < kRt12Slots; ++k) {
    if (!gc.timed[k] || gc.slots[k].fence_value > completed) continue;
    gc.timed[k] = false;
    uint64_t t[2] = {};
    void* mapped = nullptr;
    const D3D12_RANGE read = {k * 16u, k * 16u + 16u};
    if (SUCCEEDED(gc.query_readback->Map(0u, &read, &mapped))) {
      std::memcpy(t, static_cast<const uint8_t*>(mapped) + k * 16u, sizeof(t));
      const D3D12_RANGE nothing = {0u, 0u};
      gc.query_readback->Unmap(0u, &nothing);
      gc.build_ms_last = TicksToMs(t[1] - t[0], s.timestamp_frequency);
      gc.build_ms_sum += gc.build_ms_last;
      gc.build_ms_n += 1u;
    }
  }

  // 3. Keys the game wrote or destroyed: their D3D12 meshes and BLAS go (retired with the last submission).
  const size_t retired_begin = s.retired.size();
  std::vector<uint64_t> invalidated;
  {
    std::lock_guard lock(bvh::g_gpu_capture.mutex);
    invalidated.swap(bvh::g_gpu_capture.invalidated);
  }
  for (const uint64_t mesh_key : invalidated) {
    const auto it = gc.meshes.find(mesh_key);
    if (it == gc.meshes.end()) continue;
    RetireObject(s, s.last_signal, it->second.blas);
    gc.blas_bytes -= it->second.blas_bytes;
    gc.dead_bytes += GcMeshBytes(it->second.vertex_count, it->second.index_count, it->second.has_uv);
    gc.meshes.erase(it);
    std::erase(gc.build_queue, mesh_key);
  }

  // 4. Fragmentation: when most of a large arena is dead, the cache starts over (the entries are reset too).
  uint64_t used_bytes = 0u;
  for (uint32_t a = 0u; a < 3u; ++a) used_bytes += gc.arenas[a].used * kRt12ArenaStride[a];
  if (gc.dead_bytes * 2u > used_bytes && used_bytes > (64ull << 20)) {
    for (auto& [mesh_key, mesh] : gc.meshes) RetireObject(s, s.last_signal, mesh.blas);
    gc.meshes.clear();
    gc.build_queue.clear();
    gc.blas_bytes = 0u;
    for (Rt12Arena& arena : gc.arenas) arena.used = 0u;
    gc.dead_bytes = 0u;
    bvh::ResetGpuCaptureEntries("fragmented");
    renodx::utils::log::i("[world-gc] reset (fragmented)");
  }

  // 5. Range results of each readback slot whose copy has finished, oldest first (mapped outside any lock). Results apply to
  // RangeWait entries of the same generation; the rest are stale.
  const bool diag = gc.present < 8u;
  if (diag) {
    renodx::utils::log::i("[world-gc] diag p", gc.present, " begin: invalidated=", invalidated.size(), " resident=", gc.meshes.size());
  }
  const uint32_t present = gc.present;
  gc.present += 1u;
  auto* immediate = queue->get_immediate_command_list();
  ID3D11DeviceContext* context11 = bvh::GpuTimerContext(immediate);
  if (context11 == nullptr) {
    FailGc(gc, "immediate context", E_FAIL);
    return;
  }
  uint32_t slots_read = 0u, slots_waiting = 0u;
  for (uint32_t age = 0u; age < bvh::kGcReadbackSlots; ++age) {
    const uint32_t slot = (present + age) % bvh::kGcReadbackSlots;
    if (gc.readback_jobs[slot].empty()) continue;
    BOOL done = FALSE;
    if (context11->GetData(gc.readback_done[slot].Get(), &done, sizeof(done), D3D11_ASYNC_GETDATA_DONOTFLUSH) != S_OK || !done) {
      slots_waiting += 1u;
      continue;
    }
    slots_read += 1u;
    std::vector<bvh::GcJob> jobs;
    jobs.swap(gc.readback_jobs[slot]);
    uint32_t results[bvh::kGcJobsPerPresent * 4u] = {};
    void* mapped = nullptr;
    if (!device->map_buffer_region(gc.readback[slot], 0u, sizeof(results), reshade::api::map_access::read_only, &mapped)) {
      FailGc(gc, "range readback map", E_FAIL);
      return;
    }
    std::memcpy(results, mapped, sizeof(results));
    device->unmap_buffer_region(gc.readback[slot]);
    std::lock_guard lock(bvh::g_gpu_capture.mutex);
    for (size_t j = 0u; j < jobs.size(); ++j) {
      const auto it = bvh::g_gpu_capture.entries.find(jobs[j].mesh_key);
      if (it == bvh::g_gpu_capture.entries.end() || it->second.generation != jobs[j].generation
          || it->second.phase != bvh::GcPhase::RangeWait) {
        bvh::g_gpu_capture.stats.stale_jobs += 1u;
        continue;
      }
      bvh::GcEntry& entry = it->second;
      const uint32_t* result = results + 4u * j;
      const uint32_t min_vertex = ~result[0];
      const uint32_t max_vertex = result[1];
      if (result[2] != 0u || min_vertex > max_vertex) {
        bvh::RefuseGpuCaptureEntry(entry, "bad_index");
        continue;
      }
      const uint64_t vertex_count = uint64_t{max_vertex} - min_vertex + 1u;
      const bool has_uv = entry.uv_offset >= 0;
      if (vertex_count * entry.draw.vb_stride > bvh::kPoolMeshSlotBytes
          || vertex_count > bvh::kGcCarrierTexels
          || bvh::GcJobTexels(static_cast<uint32_t>(vertex_count), entry.draw.index_count, has_uv) > bvh::kGcCarrierTexels) {
        bvh::RefuseGpuCaptureEntry(entry, "too_large");
        continue;
      }
      entry.min_vertex = min_vertex;
      entry.max_vertex = max_vertex;
      entry.phase = bvh::GcPhase::Data;
      bvh::g_gpu_capture.stats.largest_vertex_range = (std::max)(bvh::g_gpu_capture.stats.largest_vertex_range, static_cast<uint32_t>(vertex_count));
      bvh::g_gpu_capture.stats.largest_index_count = (std::max)(bvh::g_gpu_capture.stats.largest_index_count, entry.draw.index_count);
    }
  }
  if (diag) {
    renodx::utils::log::i("[world-gc] diag p", present, " readback: read=", slots_read, " waiting=", slots_waiting);
  }

  // 6. Work for this present: the carrier and slot must be free for decodes; the jobs that do not fit go back to Data.
  const bool slot_free = gc.slots[gc.gc_next].fence_value <= completed && gc.carrier_fence[gc.carrier_next] <= completed;
  const bool readback_free = gc.readback_jobs[present % bvh::kGcReadbackSlots].empty();
  std::vector<bvh::GcJob> range_dispatched;
  std::vector<std::array<uint32_t, 16>> range_words;
  std::vector<std::array<uint32_t, 16>> decode_words;
  std::vector<bvh::GcDecoded> decoded;
  uint32_t texels_used = 0u;
  std::vector<bvh::GcInstanceDrawGPU> inst_draws;
  std::vector<uint32_t> inst_epochs;
  {
    std::lock_guard lock(bvh::g_gpu_capture.mutex);
    std::vector<bvh::GcJob> range_jobs;
    std::vector<bvh::GcJob> decode_jobs;
    range_jobs.swap(bvh::g_gpu_capture.range_jobs);
    decode_jobs.swap(bvh::g_gpu_capture.decode_jobs);
    bvh::g_gpu_capture.staging_used = 0u;
    inst_draws.swap(bvh::g_gpu_capture.instance_draws);
    bvh::g_gpu_capture.instance_arena_used = 0u;
    if (bvh::g_gpu_capture.epochs_dirty) {
      inst_epochs = bvh::g_gpu_capture.slot_epoch;
      bvh::g_gpu_capture.epochs_dirty = false;
    }
    for (const bvh::GcJob& job : range_jobs) {
      const auto it = bvh::g_gpu_capture.entries.find(job.mesh_key);
      if (it == bvh::g_gpu_capture.entries.end() || it->second.generation != job.generation
          || it->second.phase != bvh::GcPhase::RangeWait) {
        bvh::g_gpu_capture.stats.stale_jobs += 1u;
        continue;
      }
      const bvh::GcEntry& entry = it->second;
      if (!readback_free) {
        it->second.phase = bvh::GcPhase::Range;
        bvh::g_gpu_capture.stats.readback_busy += 1u;
        continue;
      }
      std::array<uint32_t, 16> words = {};
      words[0] = job.index_byte;
      words[1] = entry.draw.index_size;
      words[2] = entry.draw.index_count;
      words[3] = static_cast<uint32_t>(entry.draw.vertex_offset);
      words[4] = static_cast<uint32_t>(range_words.size());  // c1.x: the result slot of this job
      range_words.push_back(words);
      range_dispatched.push_back(job);
    }
    for (const bvh::GcJob& job : decode_jobs) {
      const auto it = bvh::g_gpu_capture.entries.find(job.mesh_key);
      if (it == bvh::g_gpu_capture.entries.end() || it->second.generation != job.generation
          || it->second.phase != bvh::GcPhase::DataWait) {
        bvh::g_gpu_capture.stats.stale_jobs += 1u;
        continue;
      }
      bvh::GcEntry& entry = it->second;
      const bool has_uv = entry.uv_offset >= 0;
      const uint32_t vertex_count = entry.max_vertex - entry.min_vertex + 1u;
      const uint32_t index_count = entry.draw.index_count;
      const uint32_t texels = bvh::GcJobTexels(vertex_count, index_count, has_uv);
      if (!slot_free) {
        entry.phase = bvh::GcPhase::Data;
        bvh::g_gpu_capture.stats.carrier_busy += 1u;
        continue;
      }
      if (uint64_t{texels_used} + texels > bvh::kGcCarrierTexels) {
        entry.phase = bvh::GcPhase::Data;
        bvh::g_gpu_capture.stats.carrier_full += 1u;
        continue;
      }
      bvh::GcDecodeParams params;
      params.index_byte = job.index_byte;
      params.index_size = entry.draw.index_size;
      params.index_count = index_count;
      params.base_vertex = entry.draw.vertex_offset;
      params.vertex_byte = job.vertex_byte;
      params.stride = entry.draw.vb_stride;
      params.pos_offset = static_cast<uint32_t>(entry.pos_offset);
      params.pos_format = entry.pos_format;
      params.uv_byte = job.uv_byte;
      params.uv_stride = entry.uv.buffer.handle != 0u ? entry.uv.stride : entry.draw.vb_stride;
      params.uv_offset = has_uv ? static_cast<uint32_t>(entry.uv_offset) : bvh::kGcNoUv;
      params.uv_format = entry.uv_format;
      params.vertex_count = vertex_count;
      params.min_vertex = entry.min_vertex;
      params.vtx_texel = texels_used;
      params.idx_texel = texels_used + vertex_count;
      std::array<uint32_t, 16> words = {};
      std::memcpy(words.data(), &params, sizeof(params));
      decode_words.push_back(words);
      bvh::GcDecoded done;
      done.mesh_key = job.mesh_key;
      done.generation = job.generation;
      done.vertex_count = vertex_count;
      done.index_count = index_count;
      done.has_uv = has_uv;
      done.vtx_texel = params.vtx_texel;
      done.idx_texel = params.idx_texel;
      done.uv_texel = params.idx_texel + (index_count + 3u) / 4u;
      done.vb_usage = entry.draw.vb_usage;
      done.vb_flags = entry.draw.vb_flags;
      done.ib_usage = entry.draw.ib_usage;
      done.ib_flags = entry.draw.ib_flags;
      done.pos_format = entry.pos_format;
      done.index_size = entry.draw.index_size;
      decoded.push_back(done);
      entry.phase = bvh::GcPhase::Done;
      bvh::g_gpu_capture.stats.decoded += 1u;
      texels_used += texels;
    }
  }

  // 7. D3D11 passes on the immediate context: the range pass (results copied to the readback slot of this present) and
  // the decode pass into this present's carrier.
  if (diag) {
    renodx::utils::log::i("[world-gc] diag p", present, " passes: range=", range_words.size(), " decode=", decode_words.size(), " readback_free=", readback_free ? 1 : 0);
  }
  if (!range_words.empty() || !decode_words.empty()) {
    auto* cmd_list = immediate;
    if (cmd_list == nullptr) {
      FailGc(gc, "immediate command list", E_FAIL);
      return;
    }
    const reshade::api::descriptor_table tables[2] = {gc.srv_table, gc.uav_table};
    const reshade::api::resource_view null_views[2] = {{0u}, {0u}};
    const reshade::api::descriptor_table_update srv_update = {gc.srv_table, 0, 0, 1, DT::shader_resource_view, &gc.staging_srv};
    device->update_descriptor_tables(1, &srv_update);
    bvh::BeginGpuTimer(device, cmd_list, &gc.timer);
    if (!range_words.empty()) {
      uint32_t zeros[bvh::kGcJobsPerPresent * 4u] = {};
      bvh::WriteBufferRange(device, cmd_list, gc.results, 16u, 0u, zeros, sizeof(zeros), nullptr);
      cmd_list->barrier(gc.results, RU::copy_dest, RU::unordered_access);
      cmd_list->bind_pipeline(reshade::api::pipeline_stage::all_compute, gc.range_pipeline);
      const reshade::api::resource_view range_views[2] = {null_views[0], gc.results_uav};  // u0 unused, u1 the results
      const reshade::api::descriptor_table_update uav_update = {gc.uav_table, 0, 0, 2, DT::unordered_access_view, range_views};
      device->update_descriptor_tables(1, &uav_update);
      cmd_list->bind_descriptor_tables(DS::all_compute, gc.layout, 0, 2, tables);
      for (const std::array<uint32_t, 16>& words : range_words) {
        cmd_list->push_constants(DS::all_compute, gc.layout, 2, 0, 16, words.data());
        const uint32_t groups = (words[2] + 63u) / 64u;
        cmd_list->dispatch((std::min)(groups, 65535u), (groups + 65534u) / 65535u, 1u);
      }
      const reshade::api::descriptor_table_update uav_unbind = {gc.uav_table, 0, 0, 2, DT::unordered_access_view, null_views};
      device->update_descriptor_tables(1, &uav_unbind);
      cmd_list->bind_descriptor_tables(DS::all_compute, gc.layout, 1, 1, &gc.uav_table);
      cmd_list->barrier(gc.results, RU::unordered_access, RU::copy_source);
      cmd_list->copy_buffer_region(gc.results, 0u, gc.readback[present % bvh::kGcReadbackSlots], 0u, sizeof(zeros));
      context11->End(gc.readback_done[present % bvh::kGcReadbackSlots].Get());
      gc.readback_jobs[present % bvh::kGcReadbackSlots] = range_dispatched;
    }
    if (!decode_words.empty()) {
      cmd_list->bind_pipeline(reshade::api::pipeline_stage::all_compute, gc.decode_pipeline);
      const reshade::api::resource_view decode_views[2] = {gc.carriers[gc.carrier_next].uav11, null_views[0]};  // u0 the carrier
      const reshade::api::descriptor_table_update uav_update = {gc.uav_table, 0, 0, 2, DT::unordered_access_view, decode_views};
      device->update_descriptor_tables(1, &uav_update);
      cmd_list->bind_descriptor_tables(DS::all_compute, gc.layout, 0, 2, tables);
      for (const std::array<uint32_t, 16>& words : decode_words) {
        cmd_list->push_constants(DS::all_compute, gc.layout, 2, 0, 16, words.data());
        const uint32_t loops = (std::max)(words[12], (words[2] + 3u) / 4u);
        const uint32_t groups = (loops + 63u) / 64u;
        cmd_list->dispatch((std::min)(groups, 65535u), (groups + 65534u) / 65535u, 1u);
      }
      const reshade::api::descriptor_table_update uav_unbind = {gc.uav_table, 0, 0, 2, DT::unordered_access_view, null_views};
      device->update_descriptor_tables(1, &uav_unbind);
      cmd_list->bind_descriptor_tables(DS::all_compute, gc.layout, 1, 1, &gc.uav_table);
    }
    bvh::EndGpuTimer(cmd_list, &gc.timer);
    if (diag) {
      renodx::utils::log::i("[world-gc] diag p", present, " passes recorded");
    }
  }

  // 7b. G3: the instance set (D3) and its counters, read back by query (D3D11 only, no CPU wait).
  for (uint32_t age = 0u; age < bvh::kGcReadbackSlots; ++age) {
    const uint32_t slot = (present + age) % bvh::kGcReadbackSlots;
    if (!gc.inst_pending[slot]) continue;
    BOOL done = FALSE;
    if (context11->GetData(gc.inst_done[slot].Get(), &done, sizeof(done), D3D11_ASYNC_GETDATA_DONOTFLUSH) != S_OK || !done) continue;
    void* mapped = nullptr;
    if (!device->map_buffer_region(gc.inst_readback[slot], 0u, sizeof(gc.inst_counters), reshade::api::map_access::read_only, &mapped)) {
      FailGc(gc, "instance readback map", E_FAIL);
      return;
    }
    std::memcpy(gc.inst_counters, mapped, sizeof(gc.inst_counters));
    device->unmap_buffer_region(gc.inst_readback[slot]);
    gc.inst_pending[slot] = false;
  }
  auto* cmd_list = immediate;
  if (!inst_epochs.empty()) {
    bvh::WriteBufferRange(device, cmd_list, gc.inst_epochs, 4u, 0u, inst_epochs.data(), inst_epochs.size() * 4u, nullptr);
    cmd_list->barrier(gc.inst_epochs, RU::copy_dest, RU::shader_resource);
  }
  if (!inst_draws.empty()) {
    bvh::WriteBufferRange(device, cmd_list, gc.inst_draws, sizeof(bvh::GcInstanceDrawGPU), 0u, inst_draws.data(),
                          inst_draws.size() * sizeof(bvh::GcInstanceDrawGPU), nullptr);
    cmd_list->barrier(gc.inst_draws, RU::copy_dest, RU::shader_resource);
  }
  cmd_list->barrier(gc.inst_arena, RU::copy_dest, RU::shader_resource);
  const uint32_t zero_counters[bvh::kGcCounterCount] = {};
  bvh::WriteBufferRange(device, cmd_list, gc.inst_counters_buffer, 4u, 0u, zero_counters, sizeof(zero_counters), nullptr);
  cmd_list->barrier(gc.inst_counters_buffer, RU::copy_dest, RU::unordered_access);
  bvh::BeginGpuTimer(device, cmd_list, &gc.inst_timer);
  const reshade::api::resource_view inst_srvs[3] = {gc.inst_arena_srv, gc.inst_draws_srv, gc.inst_epochs_srv};
  const reshade::api::descriptor_table_update inst_srv_update = {gc.inst_srv_table, 0, 0, 3, DT::shader_resource_view, inst_srvs};
  device->update_descriptor_tables(1, &inst_srv_update);
  const reshade::api::resource_view inst_uavs[5] = {gc.inst_meta_uav, gc.inst_entries_uav, gc.inst_records_uav, gc.inst_counters_uav,
                                                    gc.inst_moving_uav};
  const reshade::api::descriptor_table_update inst_uav_update = {gc.inst_uav_table, 0, 0, 5, DT::unordered_access_view, inst_uavs};
  device->update_descriptor_tables(1, &inst_uav_update);
  const reshade::api::descriptor_table inst_tables[2] = {gc.inst_srv_table, gc.inst_uav_table};
  cmd_list->bind_pipeline(reshade::api::pipeline_stage::all_compute, gc.insert_pipeline);
  cmd_list->bind_descriptor_tables(DS::all_compute, gc.inst_layout, 0, 2, inst_tables);
  const uint32_t draw_count = static_cast<uint32_t>(inst_draws.size());
  const bool legacy = bvh::g_pool.legacy_scale.load(std::memory_order_relaxed);
  const float min_scale = legacy ? bvh::kPoolLegacyMinScale : bvh::kPoolMinScale;
  const float max_scale = legacy ? bvh::kPoolLegacyMaxScale : bvh::kPoolMaxScale;
  std::array<uint32_t, 16> words = {};
  words[1] = present;
  words[2] = gc.inst_current * bvh::kGcSetCapacity;
  words[3] = (1u - gc.inst_current) * bvh::kGcSetCapacity;
  words[4] = draw_count;
  words[5] = bvh::kGcSetCapacity;
  std::memcpy(&words[6], &min_scale, sizeof(float));
  std::memcpy(&words[7], &max_scale, sizeof(float));
  std::memcpy(&words[8], &bvh::kPoolMaxPosition, sizeof(float));
  words[9] = bvh::kPoolMovingHoldFrames;
  words[10] = bvh::kPoolPruneAge;
  words[11] = bvh::kGcSetMaxProbes;
  words[12] = bvh::kGcMovingRecords;
  const auto run = [&](uint32_t mode, uint32_t groups) {
    words[0] = mode;
    cmd_list->push_constants(DS::all_compute, gc.inst_layout, 2, 0, 16, words.data());
    cmd_list->dispatch(groups, 1u, 1u);
  };
  run(bvh::kGcModeClear, bvh::kGcSetCapacity / 64u);
  if (draw_count != 0u) run(bvh::kGcModeMark, draw_count);
  run(bvh::kGcModeRehash, bvh::kGcSetCapacity / 64u);
  if (draw_count != 0u) run(bvh::kGcModeInsert, draw_count);
  cmd_list->bind_pipeline(reshade::api::pipeline_stage::all_compute, gc.compact_pipeline);
  run(0u, bvh::kGcSetCapacity / 64u);
  const reshade::api::resource_view null_inst_uavs[5] = {{0u}, {0u}, {0u}, {0u}, {0u}};
  const reshade::api::descriptor_table_update inst_uav_unbind = {gc.inst_uav_table, 0, 0, 5, DT::unordered_access_view, null_inst_uavs};
  device->update_descriptor_tables(1, &inst_uav_unbind);
  cmd_list->bind_descriptor_tables(DS::all_compute, gc.inst_layout, 1, 1, &gc.inst_uav_table);
  bvh::EndGpuTimer(cmd_list, &gc.inst_timer);
  cmd_list->barrier(gc.inst_counters_buffer, RU::unordered_access, RU::copy_source);
  const uint32_t inst_slot = present % bvh::kGcReadbackSlots;
  if (gc.inst_pending[inst_slot]) {
    gc.inst_readback_busy += 1u;
  } else {
    cmd_list->copy_buffer_region(gc.inst_counters_buffer, 0u, gc.inst_readback[inst_slot], 0u, sizeof(gc.inst_counters));
    context11->End(gc.inst_done[inst_slot].Get());
    gc.inst_pending[inst_slot] = true;
  }
  gc.inst_current = 1u - gc.inst_current;

  // 8. The GC submission needs its slot free. Signal fence11 after the decode passes (the D3D12 side waits on it).
  const bool busy = gc.slots[gc.gc_next].fence_value > completed;
  const bool submit = (!decoded.empty() || !gc.build_queue.empty()) && !busy;
  uint64_t signal_a = renodx::utils::gpu_fence::NO_SIGNAL;
  if (submit && !decoded.empty()) {
    try {
      signal_a = renodx::utils::gpu_fence::QueueSignal(s.ctx11.Get(), s.fence11.get());
      s.ctx11->Flush();
    } catch (const std::exception&) {
      signal_a = renodx::utils::gpu_fence::NO_SIGNAL;
    }
    if (signal_a == renodx::utils::gpu_fence::NO_SIGNAL) {
      FailGc(gc, "fence11 signal", E_FAIL);
      return;
    }
  }

  // 9. Record the GC submission: placement of the decoded meshes, arena growth (SubmitPresent 9.3), the carrier copies
  // and the arena copies, and the BLAS builds under the triangle and byte budgets.
  struct Placed {
    bvh::GcDecoded decoded;
    uint64_t v_off = 0u, i_off = 0u, u_off = 0u;
  };
  struct BlasBuild {
    uint64_t key = 0u;
    D3D12_RAYTRACING_GEOMETRY_DESC geometry = {};
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
    uint64_t result_bytes = 0u, scratch_bytes = 0u, scratch_offset = 0u;
    ComPtr<ID3D12Resource> result;
  };
  std::vector<Placed> placed;
  uint64_t added[3] = {};
  if (submit) {
    std::lock_guard lock(bvh::g_gpu_capture.mutex);
    uint64_t total_used = 0u;
    for (uint32_t a = 0u; a < 3u; ++a) total_used += gc.arenas[a].used * kRt12ArenaStride[a];
    for (const bvh::GcDecoded& decoded_mesh : decoded) {
      const auto it = bvh::g_gpu_capture.entries.find(decoded_mesh.mesh_key);
      if (it == bvh::g_gpu_capture.entries.end() || it->second.generation != decoded_mesh.generation
          || it->second.phase != bvh::GcPhase::Done) {
        bvh::g_gpu_capture.stats.stale_jobs += 1u;
        continue;
      }
      const uint64_t bytes = GcMeshBytes(decoded_mesh.vertex_count, decoded_mesh.index_count, decoded_mesh.has_uv);
      if (total_used + bytes > bvh::kGcGeometryBytesMax) {
        bvh::RefuseGpuCaptureEntry(it->second, "vram_cap");
        continue;
      }
      placed.push_back({decoded_mesh, gc.arenas[0].used + added[0], gc.arenas[1].used + added[1], gc.arenas[2].used + added[2]});
      added[0] += decoded_mesh.vertex_count;
      added[1] += decoded_mesh.index_count;
      if (decoded_mesh.has_uv) added[2] += decoded_mesh.vertex_count;
      total_used += bytes;
    }
  }
  // Placed meshes enter the cache now; their BLAS follow from the build queue.
  for (const Placed& placed_mesh : placed) {
    const bvh::GcDecoded& source = placed_mesh.decoded;
    Rt12GcMesh& mesh = gc.meshes[source.mesh_key];
    mesh.v_off = placed_mesh.v_off;
    mesh.i_off = placed_mesh.i_off;
    mesh.u_off = placed_mesh.u_off;
    mesh.vertex_count = source.vertex_count;
    mesh.index_count = source.index_count;
    mesh.has_uv = source.has_uv;
    mesh.vb_usage = source.vb_usage;
    mesh.vb_flags = source.vb_flags;
    mesh.ib_usage = source.ib_usage;
    mesh.ib_flags = source.ib_flags;
    mesh.pos_format = source.pos_format;
    mesh.index_size = source.index_size;
    gc.build_queue.push_back(source.mesh_key);
  }

  const bool record = submit && !gc.build_queue.empty();
  if (record) {
    const uint32_t k = gc.gc_next;
    Rt12Slot& slot = gc.slots[k];
    if (FAILED(slot.allocator->Reset()) || FAILED(gc.list->Reset(slot.allocator.Get(), nullptr))) {
      FailGc(gc, "gc reset", E_FAIL);
      return;
    }
    gc.list->EndQuery(gc.queries.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 2u * k);

    // Arena growth: a new buffer of the needed size; the used part is copied from the old one below.
    ComPtr<ID3D12Resource> old_arena[3];
    uint64_t old_used[3] = {};
    bool grown[3] = {};
    for (uint32_t a = 0u; a < 3u; ++a) {
      Rt12Arena& arena = gc.arenas[a];
      const uint64_t stride = kRt12ArenaStride[a];
      const uint64_t needed = arena.used + added[a];
      if (needed <= arena.capacity) continue;
      const uint64_t capacity = (std::max)({needed, 2u * arena.capacity, kRt12ArenaMinBytes / stride});
      ComPtr<ID3D12Resource> buffer;
      if (FAILED(CreateRt12Buffer(s.device12.Get(), D3D12_HEAP_TYPE_DEFAULT, capacity * stride, D3D12_RESOURCE_STATE_COMMON,
                                  D3D12_RESOURCE_FLAG_NONE, buffer))) {
        FailGc(gc, "arena growth", E_FAIL);
        return;
      }
      old_arena[a] = arena.buffer;
      old_used[a] = arena.used;
      grown[a] = true;
      RetireObject(s, s.last_signal, arena.buffer);
      arena.buffer = buffer;
      arena.capacity = capacity;
    }
    bool written[3] = {};
    for (uint32_t a = 0u; a < 3u; ++a) written[a] = added[a] > 0u || (grown[a] && old_used[a] > 0u);

    // The BLAS plan: the first queued meshes within the triangle budget and the byte cap.
    std::vector<uint32_t> triangles;
    for (const uint64_t mesh_key : gc.build_queue) triangles.push_back(gc.meshes.at(mesh_key).index_count / 3u);
    const uint32_t build_count = bvh::GcBlasBudgetCount(triangles, bvh::kGcBlasTrianglesPerPresent);
    std::vector<BlasBuild> builds;
    uint64_t blas_planned = 0u;
    uint64_t scratch_total = 0u;
    const uint64_t vertex_va = gc.arenas[kRt12ArenaVertices].buffer->GetGPUVirtualAddress();
    const uint64_t index_va = gc.arenas[kRt12ArenaIndices].buffer->GetGPUVirtualAddress();
    for (uint32_t i = 0u; i < build_count; ++i) {
      const uint64_t mesh_key = gc.build_queue[i];
      const Rt12GcMesh& mesh = gc.meshes.at(mesh_key);
      BlasBuild build;
      build.key = mesh_key;
      build.geometry.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
      build.geometry.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;
      build.geometry.Triangles.VertexBuffer.StartAddress = vertex_va + mesh.v_off * 16u;
      build.geometry.Triangles.VertexBuffer.StrideInBytes = 16u;
      build.geometry.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
      build.geometry.Triangles.VertexCount = mesh.vertex_count;
      build.geometry.Triangles.IndexBuffer = index_va + mesh.i_off * 4u;
      build.geometry.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;
      build.geometry.Triangles.IndexCount = (mesh.index_count / 3u) * 3u;
      build.inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
      build.inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
      build.inputs.NumDescs = 1u;
      build.inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
      build.inputs.pGeometryDescs = &build.geometry;
      D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info = {};
      s.device12->GetRaytracingAccelerationStructurePrebuildInfo(&build.inputs, &info);
      build.result_bytes = Align256(info.ResultDataMaxSizeInBytes);
      build.scratch_bytes = Align256(info.ScratchDataSizeInBytes);
      if (gc.blas_bytes + blas_planned + build.result_bytes > bvh::kGcBlasBytesMax) {
        gc.blas_cap_waits += 1u;
        break;
      }
      blas_planned += build.result_bytes;
      build.scratch_offset = scratch_total;
      scratch_total += build.scratch_bytes;
      builds.push_back(build);
    }
    ComPtr<ID3D12Resource> scratch;
    if (!builds.empty()) {
      if (FAILED(CreateRt12Buffer(s.device12.Get(), D3D12_HEAP_TYPE_DEFAULT, scratch_total, D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                                  D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, scratch))) {
        FailGc(gc, "blas scratch", E_FAIL);
        return;
      }
    }
    for (BlasBuild& build : builds) {
      if (FAILED(CreateRt12Buffer(s.device12.Get(), D3D12_HEAP_TYPE_DEFAULT, build.result_bytes,
                                  D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                                  build.result))) {
        FailGc(gc, "blas buffer", E_FAIL);
        return;
      }
    }

    // Barrier batch 1, copies, barrier batch 2.
    const bool any_copy = !placed.empty();
    const bool blas_now = !builds.empty();
    std::vector<D3D12_RESOURCE_BARRIER> barriers;
    for (uint32_t a = 0u; a < 3u; ++a) {
      if (grown[a] && old_used[a] > 0u) Transition(barriers, old_arena[a].Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
      if (written[a]) Transition(barriers, gc.arenas[a].buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
    }
    if (any_copy) {
      Transition(barriers, gc.linear.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
      Transition(barriers, gc.carriers[gc.carrier_next].texture12.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
    }
    if (!barriers.empty()) gc.list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
    for (uint32_t a = 0u; a < 3u; ++a) {
      if (grown[a] && old_used[a] > 0u) {
        gc.list->CopyBufferRegion(gc.arenas[a].buffer.Get(), 0u, old_arena[a].Get(), 0u, old_used[a] * kRt12ArenaStride[a]);
      }
    }
    if (any_copy) {
      // The carrier rows that hold this present's texels (texel n at byte 16n of the linear buffer).
      const uint32_t rows = (texels_used + bvh::kGcCarrierWidth - 1u) / bvh::kGcCarrierWidth;
      D3D12_TEXTURE_COPY_LOCATION destination = {};
      destination.pResource = gc.linear.Get();
      destination.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
      destination.PlacedFootprint.Offset = 0u;
      destination.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R32G32B32A32_UINT;
      destination.PlacedFootprint.Footprint.Width = bvh::kGcCarrierWidth;
      destination.PlacedFootprint.Footprint.Height = rows;
      destination.PlacedFootprint.Footprint.Depth = 1u;
      destination.PlacedFootprint.Footprint.RowPitch = bvh::kGcCarrierWidth * 16u;
      D3D12_TEXTURE_COPY_LOCATION source = {};
      source.pResource = gc.carriers[gc.carrier_next].texture12.Get();
      source.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
      source.SubresourceIndex = 0u;
      const D3D12_BOX box = {0u, 0u, 0u, bvh::kGcCarrierWidth, rows, 1u};
      gc.list->CopyTextureRegion(&destination, 0u, 0u, 0u, &source, &box);
      barriers.clear();
      Transition(barriers, gc.linear.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COPY_SOURCE);
      gc.list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
      for (const Placed& placed_mesh : placed) {
        const bvh::GcDecoded& source_mesh = placed_mesh.decoded;
        gc.list->CopyBufferRegion(gc.arenas[kRt12ArenaVertices].buffer.Get(), placed_mesh.v_off * 16u, gc.linear.Get(),
                                  uint64_t{source_mesh.vtx_texel} * 16u, uint64_t{source_mesh.vertex_count} * 16u);
        gc.list->CopyBufferRegion(gc.arenas[kRt12ArenaIndices].buffer.Get(), placed_mesh.i_off * 4u, gc.linear.Get(),
                                  uint64_t{source_mesh.idx_texel} * 16u, uint64_t{source_mesh.index_count} * 4u);
        if (source_mesh.has_uv) {
          gc.list->CopyBufferRegion(gc.arenas[kRt12ArenaUvs].buffer.Get(), placed_mesh.u_off * 8u, gc.linear.Get(),
                                    uint64_t{source_mesh.uv_texel} * 16u, uint64_t{source_mesh.vertex_count} * 8u);
        }
      }
      for (uint32_t a = 0u; a < 3u; ++a) gc.arenas[a].used += added[a];
    }
    barriers.clear();
    if (any_copy) {
      Transition(barriers, gc.linear.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COMMON);
      Transition(barriers, gc.carriers[gc.carrier_next].texture12.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COMMON);
    }
    for (uint32_t a = 0u; a < 3u; ++a) {
      if (written[a]) {
        Transition(barriers, gc.arenas[a].buffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST,
                   (a < 2u && blas_now) ? D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE : D3D12_RESOURCE_STATE_COMMON);
      }
    }
    if (blas_now) {
      for (uint32_t a = 0u; a < 2u; ++a) {
        if (!written[a]) Transition(barriers, gc.arenas[a].buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
      }
    }
    if (!barriers.empty()) gc.list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());

    // BLAS builds (SubmitPresent 9.8): one scratch buffer, one range per build.
    if (blas_now) {
      for (BlasBuild& build : builds) {
        build.inputs.pGeometryDescs = &build.geometry;  // the vector is final: the pointer stays valid
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC desc = {};
        desc.DestAccelerationStructureData = build.result->GetGPUVirtualAddress();
        desc.Inputs = build.inputs;
        desc.ScratchAccelerationStructureData = scratch->GetGPUVirtualAddress() + build.scratch_offset;
        gc.list->BuildRaytracingAccelerationStructure(&desc, 0u, nullptr);
      }
      barriers.clear();
      UavBarrier(barriers, nullptr);
      Transition(barriers, gc.arenas[kRt12ArenaVertices].buffer.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COMMON);
      Transition(barriers, gc.arenas[kRt12ArenaIndices].buffer.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COMMON);
      gc.list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
      RetireObject(s, s.last_signal, scratch);
    }
    gc.list->EndQuery(gc.queries.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 2u * k + 1u);
    gc.list->ResolveQueryData(gc.queries.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 2u * k, 2u, gc.query_readback.Get(), 16ull * k);
    gc.timed[k] = true;
    if (FAILED(gc.list->Close())) {
      FailGc(gc, "gc close", E_FAIL);
      return;
    }

    // 10. Order and execute: fence11 (decode) when this submission has placed meshes, then the previous D3D12 submission.
    if (!placed.empty()) {
      bool ordered = false;
      try {
        ordered = renodx::utils::gpu_fence::QueueWait(s.queue12.Get(), s.fence11.get(), signal_a);
      } catch (const std::exception&) {
        ordered = false;
      }
      if (!ordered) {
        FailGc(gc, "fence11 wait", E_FAIL);
        return;
      }
    }
    if (s.last_signal != 0u) {
      bool ordered = false;
      try {
        ordered = renodx::utils::gpu_fence::QueueWait(s.queue12.Get(), s.fence12.get(), s.last_signal);
      } catch (const std::exception&) {
        ordered = false;
      }
      if (!ordered) {
        FailGc(gc, "gc order wait", E_FAIL);
        return;
      }
    }
    ID3D12CommandList* lists[] = {gc.list.Get()};
    s.queue12->ExecuteCommandLists(1u, lists);
    uint64_t value = renodx::utils::gpu_fence::NO_SIGNAL;
    try {
      value = renodx::utils::gpu_fence::QueueSignal(s.queue12.Get(), s.fence12.get());
    } catch (const std::exception&) {
      value = renodx::utils::gpu_fence::NO_SIGNAL;
    }
    if (value == renodx::utils::gpu_fence::NO_SIGNAL) {
      FailSession(s, Rt12Reason::SubmitFailed, "gc signal");
      return;
    }
    slot.fence_value = value;
    if (any_copy) {
      gc.carrier_fence[gc.carrier_next] = value;
      gc.carrier_next = (gc.carrier_next + 1u) % kRt12Slots;
    }
    gc.gc_next = (k + 1u) % kRt12Slots;
    for (size_t i = retired_begin; i < s.retired.size(); ++i) s.retired[i].fence_value = value;
    s.last_signal = value;
    s.last_signal_qpc = QpcNow();
    for (BlasBuild& build : builds) {
      Rt12GcMesh& mesh = gc.meshes.at(build.key);
      mesh.blas = build.result;
      mesh.blas_bytes = build.result_bytes;
      gc.blas_bytes += build.result_bytes;
    }
    gc.build_queue.erase(gc.build_queue.begin(), gc.build_queue.begin() + static_cast<std::ptrdiff_t>(builds.size()));
    gc.blas_peak = (std::max)(gc.blas_peak, gc.blas_bytes);
  }

  // 11. On-request check (on this present thread).
  if (g_gc_check_requested.exchange(false)) RunGcCheck(s, gc);
  if (g_gc_compare_requested.exchange(false)) RunGcInstanceCompare(device, immediate, context11, gc);

  // 12. Pending meshes (the settle line), the geometry and BLAS peaks, and the panel copy.
  uint64_t phase_counts[6] = {};
  bvh::GcStats stats;
  {
    std::lock_guard lock(bvh::g_gpu_capture.mutex);
    for (const auto& [mesh_key, entry] : bvh::g_gpu_capture.entries) phase_counts[static_cast<uint32_t>(entry.phase)] += 1u;
    stats = bvh::g_gpu_capture.stats;
  }
  const uint64_t pending = phase_counts[0] + phase_counts[1] + phase_counts[2] + phase_counts[3] + gc.build_queue.size();
  if (pending > 0u && gc.pending_peak == 0u) {
    gc.pending_since = present;
    gc.pending_qpc = QpcNow();
  }
  gc.pending_peak = (std::max)(gc.pending_peak, pending);
  if (pending == 0u) {
    if (gc.pending_peak >= bvh::kPoolMassRetireMeshes) {
      Rt12GcSettle settle;
      settle.meshes = gc.meshes.size();
      settle.presents = present + 1u - gc.pending_since;
      settle.ms = QpcMsSince(gc.pending_qpc);
      gc.settle.push_back(settle);
      if (gc.settle.size() > 8u) gc.settle.erase(gc.settle.begin());
      renodx::utils::log::i("[world-gc] settled: ", settle.meshes, " meshes resident after ", settle.presents, " presents (",
                            settle.ms, " ms)");
    }
    gc.pending_peak = 0u;
  }
  uint64_t live_bytes = 0u;
  for (const auto& [mesh_key, mesh] : gc.meshes) live_bytes += GcMeshBytes(mesh.vertex_count, mesh.index_count, mesh.has_uv);
  gc.geometry_peak = (std::max)(gc.geometry_peak, live_bytes);

  Rt12GcPanel panel;
  panel.state = "active";
  panel.stats = stats;
  for (size_t i = 0u; i < 6u; ++i) panel.phase_counts[i] = phase_counts[i];
  panel.resident = gc.meshes.size();
  panel.pending = pending;
  panel.blas_queue = gc.build_queue.size();
  panel.presents = gc.present;
  {
    std::lock_guard lock(bvh::g_pool.mutex);
    panel.pool_meshes = bvh::g_pool.meshes.size();
  }
  for (uint32_t a = 0u; a < 3u; ++a) {
    panel.arena_used[a] = gc.arenas[a].used * kRt12ArenaStride[a];
    panel.arena_capacity[a] = gc.arenas[a].capacity * kRt12ArenaStride[a];
  }
  panel.dead_bytes = gc.dead_bytes;
  panel.geometry_bytes = live_bytes;
  panel.geometry_peak = gc.geometry_peak;
  panel.blas_bytes = gc.blas_bytes;
  panel.blas_peak = gc.blas_peak;
  panel.blas_cap_waits = gc.blas_cap_waits;
  panel.build_ms_last = gc.build_ms_last;
  panel.build_ms_sum = gc.build_ms_sum;
  panel.build_ms_n = gc.build_ms_n;
  panel.decode_ms_last = gc.timer.last_ms;
  panel.decode_ms_sum = gc.timer.ms_sum;
  panel.decode_ms_n = gc.timer.samples;
  panel.settle = gc.settle;
  panel.check = gc.check;
  std::memcpy(panel.inst_counters, gc.inst_counters, sizeof(panel.inst_counters));
  panel.inst_readback_busy = gc.inst_readback_busy;
  panel.inst_ms_last = gc.inst_timer.last_ms;
  panel.inst_ms_sum = gc.inst_timer.ms_sum;
  panel.inst_ms_n = gc.inst_timer.samples;
  panel.compare = gc.compare;
  {
    std::lock_guard lock(g_gc_panel_mutex);
    g_gc_panel = std::move(panel);
  }
  if (diag) {
    renodx::utils::log::i("[world-gc] diag p", present, " end: submit=", submit ? 1 : 0, " placed=", placed.size(), " resident=", gc.meshes.size(), " pending=", pending);
  }
  if (g_gc_dump_requested.exchange(false)) DumpGcJson(gc);
#else
  (void)device;
  (void)queue;
#endif
}

}  // namespace falcom_world::rt12
