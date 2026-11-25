#pragma once
// Path-traced diffuse GI on DXR (M1): one D3D12 submission after the frame's RTXDI submissions. Reads the RTXDI transfer
// slot (depth, MRT0, scene rows, lamps) and the scene TLAS; writes one RGBA16F output per PT slot for the lighting draw (t47).
#include <algorithm>
#include <bit>
#include <cmath>
#include <memory>
#include <string_view>
#include <vector>
#include "../debug/crash_log.hpp"
#include "../compute/compute.hpp"  // include order only: defines compute::NullComputeSlots, which rt12_backend.hpp and rt12_rtxdi_transfer.hpp call
#include "../rt12/rt12_backend.hpp"
#include "../rt12/rt12_rtxdi_transfer.hpp"
#include "../rr/rr_d3d12.hpp"
#include "pt_restir_layout.h"
#include "pt_state.hpp"
#include "pt_diagnostics.hpp"
namespace falcom_world::rt12 {
inline reshade::api::resource_view RunPtTrace(reshade::api::device* device, reshade::api::command_list* cmd_list,
                                              int32_t transfer_slot, bool lamps, reshade::api::resource_view isfast_view, uint32_t debug_view, uint32_t sun_detail_view,
                                               reshade::api::resource_view* reflection_view,
                                               reshade::api::resource_view* sun_visibility_view,
                                               reshade::api::resource_view* debug_views, uint32_t light_table_status) {
  *reflection_view = reshade::api::resource_view{0u};
  *sun_visibility_view = reshade::api::resource_view{0u};
  debug_views[0] = debug_views[1] = debug_views[2] = reshade::api::resource_view{0u};
  using Microsoft::WRL::ComPtr;
  const auto fallback = [](pt::PtReason reason) { pt::SetPtReason(reason); return reshade::api::resource_view{0u}; };
  // 1. Gates.
  if (!g_rt12_enabled.load(std::memory_order_relaxed)) return fallback(pt::PtReason::DxrOff);
  if (!g_rt12 || g_rt12->device != device) return fallback(pt::PtReason::NotCreated);
  Rt12State& s = *g_rt12;
  namespace di = falcom_world::rtxdi;
  if (s.unfenced_execution) return reshade::api::resource_view{0u};
  if (s.stuck) return fallback(pt::PtReason::Stuck);
  if (s.failed) return fallback(pt::PtReason::Failed);
  if (!s.shader_model_6_6) return fallback(pt::PtReason::NoShaderModel66);
  if (reinterpret_cast<ID3D11DeviceContext*>(cmd_list->get_native()) != s.ctx11.Get()) {  // NOLINT(performance-no-int-to-ptr)
    return fallback(pt::PtReason::DeferredContext);
  }
  const uint32_t frame = g_state.frame.load();
  if (s.scene_frame != frame || s.scene_slot < 0) return fallback(pt::PtReason::SceneMissing);
  if (transfer_slot < 0 || transfer_slot >= static_cast<int32_t>(kRt12Slots) || s.di_transfer.input_frame != frame
      || s.di_transfer.input_slot != transfer_slot) {
    return fallback(pt::PtReason::InputNotReady);
  }
  Rt12PtState& r = s.path_trace;
  r.water_frame = UINT32_MAX;  // invalidated by each main trace; published only after its own success
  r.water_frame_inputs = {};
  Rt12DiTransferSlot& input = s.di_transfer.slots[static_cast<uint32_t>(transfer_slot)];
  Rt12Slot& scene = s.lighting_slots[static_cast<uint32_t>(s.scene_slot)];
  const auto fail = [&s, &fallback](const char* stage, HRESULT hr) {
    const uint64_t capture_state = pt::g_pt_capture_state.load();
    if ((capture_state & 1u) != 0u) {
      pt::RecordPtEvent(g_state.frame.load(std::memory_order_relaxed),
                        std::string(stage) + " hr " + std::to_string(static_cast<uint32_t>(hr)), capture_state);
    }
    FailSession(s, Rt12Reason::CreateFailed, stage, hr);
    return fallback(pt::PtReason::Failed);
  };
  const auto submit_fail = [&s, &fallback](Rt12Reason reason, const char* stage) {
    FailSession(s, reason, stage);
    return fallback(pt::PtReason::SubmitFailed);
  };
  // Scene and input resources (the selection used by the RTXDI sun pass).
  const bool gc = s.scene_gc;
  ID3D12Resource* vertices = gc ? g_rt12_gc_scene.vertices : s.arenas[kRt12ArenaVertices].buffer.Get();
  const uint64_t vertex_count = gc ? g_rt12_gc_scene.vertex_count : s.arenas[kRt12ArenaVertices].used;
  ID3D12Resource* indices = gc ? g_rt12_gc_scene.indices : s.arenas[kRt12ArenaIndices].buffer.Get();
  const uint64_t index_count = gc ? g_rt12_gc_scene.index_count : s.arenas[kRt12ArenaIndices].used;
  ID3D12Resource* mesh_table = gc ? scene.gc_meshes.Get() : s.mesh_table.Get();
  const uint64_t mesh_count = gc ? bvh::kGcMaxEntries : s.mesh_count;
  ID3D12Resource* uv_arena = gc ? g_rt12_gc_scene.uvs : s.arenas[kRt12ArenaUvs].buffer.Get();
  const uint64_t uv_count = gc ? g_rt12_gc_scene.uv_count : s.arenas[kRt12ArenaUvs].used;
  ID3D12Resource* dynamic_vertices = s.scene_dynamic_count != 0u ? s.dynamic_vertices.Get() : nullptr;
  const bool lamps_on = lamps && input.light_count != 0u && input.lights_buffer && input.light_table;
  ID3D12Resource* lights = lamps_on ? input.lights_buffer.Get() : nullptr;
  ID3D12Resource* light_table = lamps_on ? input.light_table.Get() : nullptr;
  if (!scene.tlas || !scene.instance_table || !vertices || !indices || !mesh_table || !uv_arena || !s.atlas.texture12
      || !s.materials || (s.scene_dynamic_count != 0u && !dynamic_vertices) || !input.depth.texture12
      || !input.normal.texture12 || !input.scene.texture12) {
    return fallback(pt::PtReason::SceneMissing);
  }
  // 2. First use: allocators, list, root signature, PSO, descriptor heap.
  if (!r.created) {
    HRESULT hr = S_OK;
    for (Rt12Slot& slot : r.slots) {
      hr = s.device12->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&slot.allocator));
      if (FAILED(hr)) return fail("pt allocator", hr);
    }
    hr = s.device12->CreateCommandList(0u, D3D12_COMMAND_LIST_TYPE_DIRECT, r.slots[0].allocator.Get(), nullptr, IID_PPV_ARGS(&r.list));
    if (FAILED(hr)) return fail("pt list", hr);
    hr = r.list->Close();
    if (FAILED(hr)) return fail("pt close", hr);
    for (Rt12Slot& slot : r.water_slots) {
      hr = s.device12->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&slot.allocator));
      if (FAILED(hr)) return fail("pt water allocator", hr);
    }
    hr = s.device12->CreateCommandList(0u, D3D12_COMMAND_LIST_TYPE_DIRECT, r.water_slots[0].allocator.Get(), nullptr, IID_PPV_ARGS(&r.water_list));
    if (FAILED(hr)) return fail("pt water list", hr);
    hr = r.water_list->Close();
    if (FAILED(hr)) return fail("pt water close", hr);
    // Table (kRt12PtRanges): t0-t3 | t14-t16 | t29 | t30 | t31-t35 | u0-u3 | t36-t39 | u4-u7 | t40-t42 | u8 | t43-t55
    // | u10-u11 | u9 | u12-u13 | t56 | t57 | u14-u16 | u18.
    D3D12_DESCRIPTOR_RANGE ranges[std::size(kRt12PtRanges)] = {};
    for (uint32_t i = 0u; i < std::size(kRt12PtRanges); ++i) {
      const Rt12PtRange& range = kRt12PtRanges[i];
      ranges[i].RangeType = range.uav ? D3D12_DESCRIPTOR_RANGE_TYPE_UAV : D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
      ranges[i].NumDescriptors = range.count;
      ranges[i].BaseShaderRegister = range.base;
      ranges[i].OffsetInDescriptorsFromTableStart = range.offset;
    }
    D3D12_ROOT_PARAMETER params[3] = {};
    params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    params[0].Constants.Num32BitValues = kRt12PtConstants;
    params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    params[1].DescriptorTable.NumDescriptorRanges = static_cast<UINT>(std::size(kRt12PtRanges));
    params[1].DescriptorTable.pDescriptorRanges = ranges;
    params[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    params[2].Descriptor.ShaderRegister = 1u;
    D3D12_ROOT_SIGNATURE_DESC root_desc = {};
    root_desc.NumParameters = 3u;
    root_desc.pParameters = params;
    ComPtr<ID3DBlob> blob, error;
    hr = s.serialize(&root_desc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &error);
    if (FAILED(hr)) return fail("pt root serialize", hr);
    hr = s.device12->CreateRootSignature(0u, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&r.root));
    if (FAILED(hr)) return fail("pt root", hr);
#if defined(__world_pt_initial_EMBED_FILE) && defined(__world_pt_final_EMBED_FILE) && defined(__world_pt_temporal_EMBED_FILE) && defined(__world_pt_spatial_EMBED_FILE) && defined(__world_pt_duplication_EMBED_FILE) && defined(__world_pt_smoothed_duplication_EMBED_FILE) && defined(__world_pt_neighbor_selection_EMBED_FILE) && defined(__world_pt_water_initial_EMBED_FILE) && defined(__world_pt_water_temporal_EMBED_FILE) && defined(__world_pt_water_spatial_EMBED_FILE) && defined(__world_pt_water_neighbor_selection_EMBED_FILE) && defined(__world_pt_water_duplication_EMBED_FILE) && defined(__world_pt_water_smoothed_duplication_EMBED_FILE) && defined(__world_pt_water_final_EMBED_FILE) && defined(__world_pt_sharc_update_EMBED_FILE) && defined(__world_pt_sharc_resolve_EMBED_FILE) && defined(__world_pt_material_write_EMBED_FILE)
    D3D12_COMPUTE_PIPELINE_STATE_DESC pso_desc = {};
    pso_desc.pRootSignature = r.root.Get();
    pso_desc.CS = {__world_pt_initial.data(), __world_pt_initial.size()};
    hr = s.device12->CreateComputePipelineState(&pso_desc, IID_PPV_ARGS(&r.initial_pso));
    if (FAILED(hr)) return fail("pt initial pso", hr);
    pso_desc.CS = {__world_pt_temporal.data(), __world_pt_temporal.size()};
    hr = s.device12->CreateComputePipelineState(&pso_desc, IID_PPV_ARGS(&r.temporal_pso));
    if (FAILED(hr)) return fail("pt temporal pso", hr);
    pso_desc.CS = {__world_pt_spatial.data(), __world_pt_spatial.size()};
    hr = s.device12->CreateComputePipelineState(&pso_desc, IID_PPV_ARGS(&r.spatial_pso));
    if (FAILED(hr)) return fail("pt spatial pso", hr);
    pso_desc.CS = {__world_pt_duplication.data(), __world_pt_duplication.size()};
    hr = s.device12->CreateComputePipelineState(&pso_desc, IID_PPV_ARGS(&r.duplication_pso));
    if (FAILED(hr)) return fail("pt duplication pso", hr);
    pso_desc.CS = {__world_pt_smoothed_duplication.data(), __world_pt_smoothed_duplication.size()};
    hr = s.device12->CreateComputePipelineState(&pso_desc, IID_PPV_ARGS(&r.smoothed_duplication_pso));
    if (FAILED(hr)) return fail("pt smoothed duplication pso", hr);
    pso_desc.CS = {__world_pt_neighbor_selection.data(), __world_pt_neighbor_selection.size()};
    hr = s.device12->CreateComputePipelineState(&pso_desc, IID_PPV_ARGS(&r.neighbor_selection_pso));
    if (FAILED(hr)) return fail("pt neighbor selection pso", hr);
    pso_desc.CS = {__world_pt_final.data(), __world_pt_final.size()};
    hr = s.device12->CreateComputePipelineState(&pso_desc, IID_PPV_ARGS(&r.final_pso));
    if (FAILED(hr)) return fail("pt final pso", hr);
    pso_desc.CS = {__world_pt_water_initial.data(), __world_pt_water_initial.size()};
    hr = s.device12->CreateComputePipelineState(&pso_desc, IID_PPV_ARGS(&r.water_initial_pso));
    if (FAILED(hr)) return fail("pt water initial pso", hr);
    pso_desc.CS = {__world_pt_water_temporal.data(), __world_pt_water_temporal.size()};
    hr = s.device12->CreateComputePipelineState(&pso_desc, IID_PPV_ARGS(&r.water_temporal_pso));
    if (FAILED(hr)) return fail("pt water temporal pso", hr);
    pso_desc.CS = {__world_pt_water_spatial.data(), __world_pt_water_spatial.size()};
    hr = s.device12->CreateComputePipelineState(&pso_desc, IID_PPV_ARGS(&r.water_spatial_pso));
    if (FAILED(hr)) return fail("pt water spatial pso", hr);
    pso_desc.CS = {__world_pt_water_neighbor_selection.data(), __world_pt_water_neighbor_selection.size()};
    hr = s.device12->CreateComputePipelineState(&pso_desc, IID_PPV_ARGS(&r.water_neighbor_selection_pso));
    if (FAILED(hr)) return fail("pt water neighbor selection pso", hr);
    pso_desc.CS = {__world_pt_water_duplication.data(), __world_pt_water_duplication.size()};
    hr = s.device12->CreateComputePipelineState(&pso_desc, IID_PPV_ARGS(&r.water_duplication_pso));
    if (FAILED(hr)) return fail("pt water duplication pso", hr);
    pso_desc.CS = {__world_pt_water_smoothed_duplication.data(), __world_pt_water_smoothed_duplication.size()};
    hr = s.device12->CreateComputePipelineState(&pso_desc, IID_PPV_ARGS(&r.water_smoothed_duplication_pso));
    if (FAILED(hr)) return fail("pt water smoothed duplication pso", hr);
    pso_desc.CS = {__world_pt_water_final.data(), __world_pt_water_final.size()};
    hr = s.device12->CreateComputePipelineState(&pso_desc, IID_PPV_ARGS(&r.water_final_pso));
    if (FAILED(hr)) return fail("pt water final pso", hr);
    pso_desc.CS = {__world_pt_sharc_update.data(), __world_pt_sharc_update.size()};
    hr = s.device12->CreateComputePipelineState(&pso_desc, IID_PPV_ARGS(&r.sharc_update_pso));
    if (FAILED(hr)) return fail("pt sharc update pso", hr);
    pso_desc.CS = {__world_pt_sharc_resolve.data(), __world_pt_sharc_resolve.size()};
    hr = s.device12->CreateComputePipelineState(&pso_desc, IID_PPV_ARGS(&r.sharc_resolve_pso));
    if (FAILED(hr)) return fail("pt sharc resolve pso", hr);
    pso_desc.CS = {__world_pt_material_write.data(), __world_pt_material_write.size()};
    hr = s.device12->CreateComputePipelineState(&pso_desc, IID_PPV_ARGS(&r.material_write_pso));
    if (FAILED(hr)) return fail("pt material write pso", hr);
#else
    return fail("pt shader not embedded", E_FAIL);
#endif
    const bool regir_created = [&]() {
    D3D12_DESCRIPTOR_RANGE regir_ranges[2] = {};
    regir_ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    regir_ranges[0].NumDescriptors = 3u;
    regir_ranges[0].OffsetInDescriptorsFromTableStart = 0u;
    regir_ranges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    regir_ranges[1].NumDescriptors = 2u;
    regir_ranges[1].OffsetInDescriptorsFromTableStart = 3u;
    D3D12_ROOT_PARAMETER regir_params[2] = {};
    regir_params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    regir_params[0].Constants.Num32BitValues = kRt12ReGirConstants;
    regir_params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    regir_params[1].DescriptorTable.NumDescriptorRanges = 2u;
    regir_params[1].DescriptorTable.pDescriptorRanges = regir_ranges;
    D3D12_ROOT_SIGNATURE_DESC regir_desc = {};
    regir_desc.NumParameters = 2u;
    regir_desc.pParameters = regir_params;
    hr = s.serialize(&regir_desc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &error);
    if (FAILED(hr)) return false;
    hr = s.device12->CreateRootSignature(0u, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&r.regir_root));
    if (FAILED(hr)) return false;
#if defined(__world_regir_register_EMBED_FILE) && defined(__world_regir_weights_EMBED_FILE) && defined(__world_regir_build_EMBED_FILE)
    D3D12_COMPUTE_PIPELINE_STATE_DESC regir_pso_desc = {};
    regir_pso_desc.pRootSignature = r.regir_root.Get();
    regir_pso_desc.CS = {__world_regir_register.data(), __world_regir_register.size()};
    hr = s.device12->CreateComputePipelineState(&regir_pso_desc, IID_PPV_ARGS(&r.regir_register_pso));
    if (FAILED(hr)) return false;
    regir_pso_desc.CS = {__world_regir_weights.data(), __world_regir_weights.size()};
    hr = s.device12->CreateComputePipelineState(&regir_pso_desc, IID_PPV_ARGS(&r.regir_weights_pso));
    if (FAILED(hr)) return false;
    regir_pso_desc.CS = {__world_regir_build.data(), __world_regir_build.size()};
    hr = s.device12->CreateComputePipelineState(&regir_pso_desc, IID_PPV_ARGS(&r.regir_build_pso));
    if (FAILED(hr)) return false;
#else
    return false;
#endif
    D3D12_DESCRIPTOR_HEAP_DESC regir_heap_desc = {};
    regir_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    regir_heap_desc.NumDescriptors = kRt12Slots * kRt12ReGirPasses * kRt12ReGirDescriptors;
    regir_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    hr = s.device12->CreateDescriptorHeap(&regir_heap_desc, IID_PPV_ARGS(&r.regir_heap));
    if (FAILED(hr)) return false;
    for (uint32_t i = 0u; i < kRt12Slots; ++i) {
      hr = CreateRt12Buffer(s.device12.Get(), D3D12_HEAP_TYPE_DEFAULT, kRt12ReGirHeaderStride, D3D12_RESOURCE_STATE_COMMON,
                            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, r.regir_header[i]);
      if (FAILED(hr)) return false;
      hr = CreateRt12Buffer(s.device12.Get(), D3D12_HEAP_TYPE_DEFAULT, pt::kPtReGirMaxCells * kRt12ReGirCellStride, D3D12_RESOURCE_STATE_COMMON,
                            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, r.regir_cells[i]);
      if (FAILED(hr)) return false;
      hr = CreateRt12Buffer(s.device12.Get(), D3D12_HEAP_TYPE_DEFAULT,
                            pt::kPtReGirMaxCells * RTXDI_LIGHT_TABLE_CAPACITY * sizeof(float), D3D12_RESOURCE_STATE_COMMON,
                            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, r.regir_weights[i]);
      if (FAILED(hr)) return false;
      if (SUCCEEDED(CreateRt12Buffer(s.device12.Get(), D3D12_HEAP_TYPE_READBACK, kRt12ReGirHeaderStride,
                                     D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_FLAG_NONE, r.regir_header_readback[i]))) {
        if (FAILED(CreateRt12Buffer(s.device12.Get(), D3D12_HEAP_TYPE_READBACK, pt::kPtReGirMaxCells * kRt12ReGirCellStride,
                                    D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_FLAG_NONE, r.regir_cells_readback[i]))) {
          r.regir_header_readback[i].Reset();
        }
      }
    }
      return true;
    }();
    if (!regir_created) {
      r.regir_root.Reset();
      r.regir_register_pso.Reset();
      r.regir_weights_pso.Reset();
      r.regir_build_pso.Reset();
      r.regir_heap.Reset();
      for (uint32_t i = 0u; i < kRt12Slots; ++i) {
        r.regir_header[i].Reset();
        r.regir_cells[i].Reset();
        r.regir_weights[i].Reset();
        r.regir_header_readback[i].Reset();
        r.regir_cells_readback[i].Reset();
      }
    }
    D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
    heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heap_desc.NumDescriptors = 2u * kRt12Slots * kRt12PtDescriptors;  // main blocks, then water blocks
    heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    hr = s.device12->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&r.heap));
    if (FAILED(hr)) return fail("pt heap", hr);
    if (FAILED(CreateRt12Buffer(s.device12.Get(), D3D12_HEAP_TYPE_UPLOAD, rtxdi::kNeighborBytes,
                                D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_FLAG_NONE, r.neighbors))) {
      return fail("pt neighbors", E_FAIL);
    }
    void* neighbor_mapped = nullptr;
    if (FAILED(r.neighbors->Map(0u, nullptr, &neighbor_mapped))) return fail("pt neighbors map", E_FAIL);
    ::rtxdi::FillNeighborOffsetBuffer(static_cast<uint8_t*>(neighbor_mapped), rtxdi::kNeighborCount);
    r.neighbors->Unmap(0u, nullptr);
    for (uint32_t i = 0u; i < kRt12Slots; ++i) {
      hr = CreateRt12Buffer(s.device12.Get(), D3D12_HEAP_TYPE_UPLOAD, kPtRestirConstantsBytes, D3D12_RESOURCE_STATE_GENERIC_READ,
                            D3D12_RESOURCE_FLAG_NONE, r.constants[i]);
      if (FAILED(hr)) return fail("pt restir constants", hr);
      hr = CreateRt12Buffer(s.device12.Get(), D3D12_HEAP_TYPE_DEFAULT, kRt12PtCounters * sizeof(uint32_t), D3D12_RESOURCE_STATE_COMMON,
                            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, r.counters[i]);
      if (FAILED(hr)) return fail("pt counters", hr);
      hr = CreateRt12Buffer(s.device12.Get(), D3D12_HEAP_TYPE_READBACK, kRt12PtCounters * sizeof(uint32_t), D3D12_RESOURCE_STATE_COPY_DEST,
                            D3D12_RESOURCE_FLAG_NONE, r.counter_readback[i]);
      if (FAILED(hr)) return fail("pt counter readback", hr);
      hr = CreateRt12Buffer(s.device12.Get(), D3D12_HEAP_TYPE_DEFAULT, kRt12PtCounters * sizeof(uint32_t), D3D12_RESOURCE_STATE_COMMON,
                            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, r.water_counters[i]);
      if (FAILED(hr)) return fail("pt water counters", hr);
      hr = CreateRt12Buffer(s.device12.Get(), D3D12_HEAP_TYPE_READBACK, kRt12PtCounters * sizeof(uint32_t), D3D12_RESOURCE_STATE_COPY_DEST,
                            D3D12_RESOURCE_FLAG_NONE, r.water_counter_readback[i]);
      if (FAILED(hr)) return fail("pt water counter readback", hr);
    }
    hr = CreateRt12Buffer(s.device12.Get(), D3D12_HEAP_TYPE_UPLOAD, kRt12PtCounters * sizeof(uint32_t), D3D12_RESOURCE_STATE_GENERIC_READ,
                          D3D12_RESOURCE_FLAG_NONE, r.counter_zero);
    if (FAILED(hr)) return fail("pt counter zero", hr);
    void* zeros = nullptr;
    hr = r.counter_zero->Map(0u, nullptr, &zeros);
    if (FAILED(hr)) return fail("pt counter zero map", hr);
    std::fill_n(static_cast<uint32_t*>(zeros), kRt12PtCounters, 0u);
    r.counter_zero->Unmap(0u, nullptr);
    std::fill_n(r.debug_frame, kRt12Slots, UINT32_MAX);
    r.created = true;
  }
  // 2b. GPU counters: a slot's counters are read once its submission's fence is complete. Never waits.
  const uint64_t completed = Rt12CompletedValue(s);
  {
    std::lock_guard lock(pt::g_pt_panel_mutex);
    if (pt::g_pt_panel.regir_counts_generation != s.generation) pt::g_pt_panel.regir_counts_available = false;
    if (pt::g_pt_panel.regir_water_counts_owner_generation != pt::g_pt_panel.regir_owner_generation) {
      pt::g_pt_panel.regir_water_counts_available = false;
      pt::g_pt_panel.regir_water_counts_frame = 0u;
      pt::g_pt_panel.regir_water_counts_owner_generation = 0u;
    }
  }
  for (uint32_t i = 0u; i < kRt12Slots; ++i) {
    if (!r.regir_counts_pending[i] || r.regir_counts_fence[i] > completed) continue;
    void* header_data = nullptr;
    const D3D12_RANGE header_range = {0u, kRt12ReGirHeaderStride};
    if (FAILED(r.regir_header_readback[i]->Map(0u, &header_range, &header_data))) continue;
    void* cells_data = nullptr;
    const D3D12_RANGE cells_range = {0u, pt::kPtReGirMaxCells * kRt12ReGirCellStride};
    const HRESULT cells_result = r.regir_cells_readback[i]->Map(0u, &cells_range, &cells_data);
    if (SUCCEEDED(cells_result) && header_data && cells_data) {
      const auto* header_values = static_cast<const uint32_t*>(header_data);
      const auto* cell_values = static_cast<const uint32_t*>(cells_data);
      const uint32_t cell_count = (std::min)(header_values[0], pt::kPtReGirMaxCells);
      uint32_t valid_count = 0u;
      for (uint32_t cell = 0u; cell < cell_count; ++cell) {
        valid_count += cell_values[cell * (kRt12ReGirCellStride / sizeof(uint32_t)) + 3u] != 0u ? 1u : 0u;
      }
      std::lock_guard lock(pt::g_pt_panel_mutex);
      if (!pt::g_pt_panel.regir_counts_available || pt::g_pt_panel.regir_counts_generation != s.generation
          || r.regir_counts_fence[i] > pt::g_pt_panel.regir_counts_fence) {
        pt::g_pt_panel.regir_counts_available = true;
        pt::g_pt_panel.regir_counts_generation = s.generation;
        pt::g_pt_panel.regir_counts_frame = r.regir_counts_frame[i];
        pt::g_pt_panel.regir_counts_fence = r.regir_counts_fence[i];
        pt::g_pt_panel.regir_cell_count = header_values[0];
        pt::g_pt_panel.regir_rejected_count = header_values[1];
        pt::g_pt_panel.regir_overflow_count = header_values[2];
        pt::g_pt_panel.regir_valid_count = valid_count;
      }
      r.regir_counts_pending[i] = false;
    }
    const D3D12_RANGE no_writes = {0u, 0u};
    if (SUCCEEDED(cells_result)) r.regir_cells_readback[i]->Unmap(0u, &no_writes);
    r.regir_header_readback[i]->Unmap(0u, &no_writes);
  }
  for (uint32_t i = 0u; i < kRt12Slots; ++i) {
    if (!r.counter_pending[i] || r.counter_fence[i] > completed) continue;
    void* mapped = nullptr;
    if (FAILED(r.counter_readback[i]->Map(0u, nullptr, &mapped))) continue;
    if (mapped != nullptr) {
      const auto* values = static_cast<const uint32_t*>(mapped);
      {
        std::lock_guard lock(pt::g_pt_panel_mutex);
        const uint32_t stored = static_cast<uint32_t>(pt::g_pt_panel.gpu_frame);
        const bool newer = static_cast<int32_t>(r.counter_frame[i] - stored) >= 0;
        if (!pt::g_pt_panel.gpu_available || newer) {
          pt::g_pt_panel.gpu_exact = values[0];
          pt::g_pt_panel.gpu_hits = values[1];
          pt::g_pt_panel.gpu_misses = values[2];
          pt::g_pt_panel.gpu_lock_drops = values[3];
          pt::g_pt_panel.gpu_writes = values[4];
          pt::g_pt_panel.regir_eligible = values[9];
          pt::g_pt_panel.regir_lookups = values[10];
          pt::g_pt_panel.regir_matches = values[11];
          pt::g_pt_panel.regir_misses = values[12];
          pt::g_pt_panel.regir_invalid_headers = values[13];
          pt::g_pt_panel.regir_invalid_slots = values[14];
          pt::g_pt_panel.regir_selections = values[15];
          pt::g_pt_panel.regir_fallback_disabled = values[16];
          pt::g_pt_panel.regir_fallback_key = values[17];
          pt::g_pt_panel.regir_fallback_miss = values[18];
          pt::g_pt_panel.regir_fallback_header = values[19];
          pt::g_pt_panel.regir_fallback_slots = values[20];
          pt::g_pt_panel.gpu_initial_valid = values[21];
          pt::g_pt_panel.gpu_path_len2 = values[22];
          pt::g_pt_panel.gpu_path_len3plus = values[23];
          pt::g_pt_panel.gpu_nee_connected = values[24];
          pt::g_pt_panel.gpu_sharc_terminated = values[25];
          pt::g_pt_panel.gpu_sharc_terminal_rc = values[26];
          pt::g_pt_panel.gpu_sharc_replay_rejected = values[27];
          pt::g_pt_panel.temporal_selected = values[28];
          pt::g_pt_panel.spatial_selected = values[29];
          pt::g_pt_panel.gpu_d2_lamp = values[30];
          pt::g_pt_panel.gpu_d2_sun = values[31];
          pt::g_pt_panel.gpu_shade_transmitted = values[32];
          pt::g_pt_panel.gpu_ao_rays = values[33];
          pt::g_pt_panel.gpu_ao_occluded = values[34];
          pt::g_pt_panel.gpu_energy_len2 = values[35];
          pt::g_pt_panel.gpu_energy_len3 = values[36];
          pt::g_pt_panel.gpu_energy_len4_5 = values[37];
          pt::g_pt_panel.gpu_energy_len6plus = values[38];
          pt::g_pt_panel.gpu_sharc_first_hit_cached_lum = values[39];
          pt::g_pt_panel.gpu_first_hit_traced_lum = values[40];
          pt::g_pt_panel.sparse_sample_active = r.counter_sparse[i];
          pt::g_pt_panel.sparse_selected = values[7];
          pt::g_pt_panel.sparse_valid = values[8];
          pt::g_pt_panel.gpu_frame = static_cast<uint32_t>(r.counter_frame[i]);
          pt::g_pt_panel.gpu_available = true;
        }
      }
      r.counter_pending[i] = false;
    }
    r.counter_readback[i]->Unmap(0u, nullptr);
  }
  // 3. Slot wait: this slot's previous submission must be complete (a timeout is stuck).
  const uint32_t k = r.next;
  Rt12Slot& pt_slot = r.slots[k];
  if (pt_slot.fence_value > Rt12CompletedValue(s)) {
    const LARGE_INTEGER wait_start = QpcNow();
    const bool done = WaitRt12Fence(s, pt_slot.fence_value, kRt12StuckMs);
    const float wait_ms = static_cast<float>(QpcMsSince(wait_start));
    {
      std::lock_guard lock(pt::g_pt_panel_mutex);
      pt::g_pt_panel.waits += 1u;
      pt::g_pt_panel.wait_ms_max = (std::max)(pt::g_pt_panel.wait_ms_max, wait_ms);
    }
    if (!done) {
      s.stuck = true;
      const HRESULT removed = s.device12 ? s.device12->GetDeviceRemovedReason() : S_OK;
      pt::PtLogWarn("[world-rt12] pt stuck: fence at " + std::to_string(Rt12CompletedValue(s)) + ", slot submitted "
                    + std::to_string(pt_slot.fence_value) + " device_removed="
                    + renodx::utils::log::AsHex(static_cast<uint32_t>(removed)));
      DisableMirror();
      return fallback(pt::PtReason::Stuck);
    }
  }
  // 4. This slot's output at the input size (a replaced texture is retired with this submission's fence).
  const size_t retired_begin = s.retired.size();
  const uint32_t w = input.width;
  const uint32_t h = input.height;
  if ((!r.restir || r.restir_width != w || r.restir_height != h) && r.reservoirs_last_use <= Rt12CompletedValue(s)) {
    auto context = std::make_unique<::rtxdi::ReSTIRPTContext>(::rtxdi::ReSTIRPTStaticParameters{
        .RenderWidth = w, .RenderHeight = h, .CheckerboardSamplingMode = ::rtxdi::CheckerboardMode::Off});
    const uint64_t bytes = uint64_t{context->GetReservoirBufferParameters().reservoirArrayPitch}
                           * ::rtxdi::c_NumReSTIRPTReservoirBuffers * sizeof(RTXDI_PackedPTReservoir);
    RetireObject(s, s.last_signal, r.reservoirs);
    r.reservoirs.Reset();
    const HRESULT hr_reservoirs = CreateRt12Buffer(s.device12.Get(), D3D12_HEAP_TYPE_DEFAULT, bytes,
                                                   D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                                                   r.reservoirs);
    if (FAILED(hr_reservoirs)) return fail("pt reservoirs", hr_reservoirs);
    D3D12_HEAP_PROPERTIES heap = {};
    heap.Type = D3D12_HEAP_TYPE_DEFAULT;
    const auto make_texture = [&](DXGI_FORMAT format, ComPtr<ID3D12Resource>* out) {
      D3D12_RESOURCE_DESC desc = {};
      desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
      desc.Width = w;
      desc.Height = h;
      desc.DepthOrArraySize = 1u;
      desc.MipLevels = 1u;
      desc.Format = format;
      desc.SampleDesc.Count = 1u;
      desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
      desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
      return s.device12->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COMMON, nullptr,
                                                 IID_PPV_ARGS(out->ReleaseAndGetAddressOf()));
    };
    for (const auto& resource : {r.sample_id, r.duplication_map, r.smoothed_duplication[0], r.smoothed_duplication[1],
                                 r.neighbor_selection_gbuffer, r.neighbor_selection}) {
      RetireObject(s, s.last_signal, resource);
    }
    r.sample_id.Reset();
    r.duplication_map.Reset();
    r.smoothed_duplication[0].Reset();
    r.smoothed_duplication[1].Reset();
    r.neighbor_selection_gbuffer.Reset();
    r.neighbor_selection.Reset();
    HRESULT hr_maps = make_texture(DXGI_FORMAT_R32_UINT, &r.sample_id);
    if (FAILED(hr_maps)) return fail("pt sample id", hr_maps);
    hr_maps = make_texture(DXGI_FORMAT_R32G32_FLOAT, &r.duplication_map);
    if (FAILED(hr_maps)) return fail("pt duplication map", hr_maps);
    hr_maps = make_texture(DXGI_FORMAT_R32_FLOAT, &r.smoothed_duplication[0]);
    if (FAILED(hr_maps)) return fail("pt smoothed duplication", hr_maps);
    hr_maps = make_texture(DXGI_FORMAT_R32_FLOAT, &r.smoothed_duplication[1]);
    if (FAILED(hr_maps)) return fail("pt smoothed duplication prev", hr_maps);
    hr_maps = make_texture(DXGI_FORMAT_R16G16B16A16_FLOAT, &r.neighbor_selection_gbuffer);
    if (FAILED(hr_maps)) return fail("pt neighbor selection gbuffer", hr_maps);
    hr_maps = CreateRt12Buffer(s.device12.Get(), D3D12_HEAP_TYPE_DEFAULT, uint64_t{w} * h * 4u * 4u,
                               D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, r.neighbor_selection);
    if (FAILED(hr_maps)) return fail("pt neighbor selection buffer", hr_maps);
    uint64_t next_configuration = 0u;
    if (!rtxdi::NextDiConfigurationGeneration(r.configuration_generation, &next_configuration)) {
      return fail("pt configuration generation", E_FAIL);
    }
    r.configuration_generation = next_configuration;
    r.restir = std::move(context);
    r.restir_width = w;
    r.restir_height = h;
  }
  if (!r.restir || !r.reservoirs) return fallback(pt::PtReason::NotCreated);
  // A resize waits for the reservoirs' last use; until then skip the frame rather than dispatch at the new size.
  if (r.restir_width != w || r.restir_height != h) return fallback(pt::PtReason::NotCreated);
  Rt12Shared& out = r.output[k];
  if (!out.texture11 || out.width != w || out.height != h) {
    RetireShared(s, out, s.last_signal);
    if (FAILED(CreateRt12Shared(s, device, DXGI_FORMAT_R16G16B16A16_FLOAT, w, h, 1u, true, &out))) {
      RetireShared(s, out, s.last_signal);
      return fallback(pt::PtReason::TargetFailed);
    }
  }
  const bool refl_on = pt::g_pt_reflection_enabled > 0.5f && input.color.texture12 && input.mrt1.texture12 && input.mrt2.texture12;
  const bool water_on = pt::g_pt_reflection_water_surface > 0.5f && refl_on;
  const bool refl_target = refl_on || pt::g_restir_direct > 0.5f;
  if ((!water_on || !r.water_restir || r.water_restir_width != w || r.water_restir_height != h)
      && r.water_reservoirs_last_use <= completed) {
    RetireObject(s, s.last_signal, r.water_reservoirs);
    for (const auto& resource : {r.water_sample_id, r.water_duplication_map, r.water_smoothed_duplication[0],
                                 r.water_smoothed_duplication[1], r.water_neighbor_selection_gbuffer,
                                 r.water_neighbor_selection}) {
      RetireObject(s, s.last_signal, resource);
    }
    for (auto& constants : r.water_constants) RetireObject(s, s.last_signal, constants);
    r.water_reservoirs.Reset();
    r.water_sample_id.Reset();
    r.water_duplication_map.Reset();
    r.water_smoothed_duplication[0].Reset();
    r.water_smoothed_duplication[1].Reset();
    r.water_neighbor_selection_gbuffer.Reset();
    r.water_neighbor_selection.Reset();
    for (auto& constants : r.water_constants) constants.Reset();
    r.water_restir.reset();
    r.water_history_valid = false;
    r.water_history_frame = UINT32_MAX;
    r.water_history_transfer_slot = -1;
    if (water_on) {
      auto context = std::make_unique<::rtxdi::ReSTIRPTContext>(::rtxdi::ReSTIRPTStaticParameters{
          .RenderWidth = w, .RenderHeight = h, .CheckerboardSamplingMode = ::rtxdi::CheckerboardMode::Off});
      uint64_t next_configuration = 0u;
      if (!rtxdi::NextDiConfigurationGeneration(r.configuration_generation, &next_configuration)) {
        return fail("pt water configuration generation", E_FAIL);
      }
      r.configuration_generation = next_configuration;
      const uint64_t water_bytes = uint64_t{context->GetReservoirBufferParameters().reservoirArrayPitch}
                                   * ::rtxdi::c_NumReSTIRPTReservoirBuffers * sizeof(RTXDI_PackedPTReservoir);
      const HRESULT hr_water = CreateRt12Buffer(s.device12.Get(), D3D12_HEAP_TYPE_DEFAULT, water_bytes,
                                                D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                                                r.water_reservoirs);
      if (FAILED(hr_water)) return fail("pt water reservoirs", hr_water);
      D3D12_HEAP_PROPERTIES heap = {};
      heap.Type = D3D12_HEAP_TYPE_DEFAULT;
      const auto make_texture = [&](DXGI_FORMAT format, ComPtr<ID3D12Resource>* out) {
        D3D12_RESOURCE_DESC desc = {};
        desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        desc.Width = w;
        desc.Height = h;
        desc.DepthOrArraySize = 1u;
        desc.MipLevels = 1u;
        desc.Format = format;
        desc.SampleDesc.Count = 1u;
        desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        return s.device12->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COMMON, nullptr,
                                                   IID_PPV_ARGS(out->ReleaseAndGetAddressOf()));
      };
      HRESULT hr_maps = make_texture(DXGI_FORMAT_R32_UINT, &r.water_sample_id);
      if (FAILED(hr_maps)) return fail("pt water sample id", hr_maps);
      hr_maps = make_texture(DXGI_FORMAT_R32G32_FLOAT, &r.water_duplication_map);
      if (FAILED(hr_maps)) return fail("pt water duplication map", hr_maps);
      hr_maps = make_texture(DXGI_FORMAT_R32_FLOAT, &r.water_smoothed_duplication[0]);
      if (FAILED(hr_maps)) return fail("pt water smoothed duplication", hr_maps);
      hr_maps = make_texture(DXGI_FORMAT_R32_FLOAT, &r.water_smoothed_duplication[1]);
      if (FAILED(hr_maps)) return fail("pt water smoothed duplication prev", hr_maps);
      hr_maps = make_texture(DXGI_FORMAT_R16G16B16A16_FLOAT, &r.water_neighbor_selection_gbuffer);
      if (FAILED(hr_maps)) return fail("pt water neighbor selection gbuffer", hr_maps);
      hr_maps = CreateRt12Buffer(s.device12.Get(), D3D12_HEAP_TYPE_DEFAULT, uint64_t{w} * h * 4u * 4u,
                                 D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                                 r.water_neighbor_selection);
      if (FAILED(hr_maps)) return fail("pt water neighbor selection buffer", hr_maps);
      for (auto& constants : r.water_constants) {
        hr_maps = CreateRt12Buffer(s.device12.Get(), D3D12_HEAP_TYPE_UPLOAD, kPtRestirConstantsBytes,
                                   D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_FLAG_NONE, constants);
        if (FAILED(hr_maps)) return fail("pt water restir constants", hr_maps);
      }
      r.water_restir = std::move(context);
      r.water_restir_width = w;
      r.water_restir_height = h;
    }
  }
  if (water_on && !r.water_restir) return fallback(pt::PtReason::NotCreated);
  if (water_on && (r.water_restir_width != w || r.water_restir_height != h)) return fallback(pt::PtReason::NotCreated);
  Rt12Shared& refl = r.reflection[k];
  if (refl_target && (!refl.texture11 || refl.width != w || refl.height != h)) {
    RetireShared(s, refl, s.last_signal);
    if (FAILED(CreateRt12Shared(s, device, DXGI_FORMAT_R16G16B16A16_FLOAT, w, h, 1u, true, &refl))) {
      RetireShared(s, refl, s.last_signal);
      return fallback(pt::PtReason::TargetFailed);
    }
  }
  const bool game_sun_shadows = pt::g_restir_game_sun_shadows > 0.5f;
  Rt12Shared& sun_vis = r.sun_visibility[k];
  if (game_sun_shadows && (!sun_vis.texture11 || sun_vis.width != w || sun_vis.height != h)) {
    RetireShared(s, sun_vis, s.last_signal);
    if (FAILED(CreateRt12Shared(s, device, DXGI_FORMAT_R16G16B16A16_FLOAT, w, h, 1u, true, &sun_vis))) {
      RetireShared(s, sun_vis, s.last_signal);
      return fallback(pt::PtReason::TargetFailed);
    }
  } else if (!game_sun_shadows && sun_vis.texture11) {
    RetireShared(s, sun_vis, s.last_signal);
  }
  Rt12Shared& initial_color = r.initial_color[k];
  if (!initial_color.texture11 || initial_color.width != w || initial_color.height != h) {
    RetireShared(s, initial_color, s.last_signal);
    if (FAILED(CreateRt12Shared(s, device, DXGI_FORMAT_R16G16B16A16_FLOAT, w, h, 1u, true, &initial_color))) {
      RetireShared(s, initial_color, s.last_signal);
      return fallback(pt::PtReason::TargetFailed);
    }
  }
  const bool debug_on = debug_view != RTXDI_DEBUG_VIEW_OFF;
  Rt12Shared* debug_outputs[] = {&r.debug_value[k], &r.debug_status[k], &r.sun_detail[k]};
  const DXGI_FORMAT debug_formats[] = {DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_R32_UINT, DXGI_FORMAT_R32G32B32A32_UINT};
  for (uint32_t i = 0u; i < 3u; ++i) {
    Rt12Shared& shared = *debug_outputs[i];
    if (debug_on && (!shared.texture11 || shared.width != w || shared.height != h)) {
      RetireShared(s, shared, s.last_signal);
      if (FAILED(CreateRt12Shared(s, device, debug_formats[i], w, h, 1u, true, &shared))) {
        RetireShared(s, shared, s.last_signal);
        return fallback(pt::PtReason::TargetFailed);
      }
    } else if (!debug_on && shared.texture11) {
      RetireShared(s, shared, s.last_signal);
    }
  }
  if (!debug_on) {
    r.debug_frame[k] = UINT32_MAX;
    r.debug_view[k] = RTXDI_DEBUG_VIEW_OFF;
    r.sun_detail_view[k] = RTXDI_SUN_DETAIL_STATUS;
  }
  // 4b. SHARC (M2): clear triggers, buffers, update grid and resolve window (OpenRemix sharc.cpp:311-475).
  const uint32_t sharc_capacity_wanted = 1u << std::clamp(static_cast<uint32_t>(pt::g_pt_sharc_capacity_log2), 16u, 22u);
  if (r.sharc_failed_capacity != 0u && r.sharc_failed_capacity != sharc_capacity_wanted) {
    r.sharc_failed_capacity = 0u;
  }
  bool sharc_on = pt::g_pt_sharc_enabled > 0.5f && r.sharc_failed_capacity != sharc_capacity_wanted;
  float camera[3] = {};
  float light[6] = {};
  bool has_sun = false;
  {
    std::lock_guard lock(g_state.mutex);
    const CameraSnapshot& c = g_state.camera;
    if (c.valid) { camera[0] = c.view_inv[3]; camera[1] = c.view_inv[7]; camera[2] = c.view_inv[11]; }
    has_sun = c.valid && c.has_sun;
    if (has_sun) { std::copy_n(c.light_direction, 3, light); std::copy_n(c.light_color, 3, light + 3); }
  }
  uint32_t episode = 0u;
  { std::lock_guard lock(bvh::g_pool.mutex); episode = bvh::g_pool.episode_serial; }
  const char* clear_reason = nullptr;
  if (sharc_on && r.sharc_capacity != sharc_capacity_wanted) {
    RetireObject(s, s.last_signal, r.sharc_hash);
    RetireObject(s, s.last_signal, r.sharc_accumulation);
    RetireObject(s, s.last_signal, r.sharc_resolved);
    r.sharc_hash.Reset(); r.sharc_accumulation.Reset(); r.sharc_resolved.Reset(); r.sharc_capacity = 0u;
    const auto make = [&](uint64_t stride, ComPtr<ID3D12Resource>* out) {
      return CreateRt12Buffer(s.device12.Get(), D3D12_HEAP_TYPE_DEFAULT, uint64_t{sharc_capacity_wanted} * stride,
                              D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, *out);
    };
    if (SUCCEEDED(make(8u, std::addressof(r.sharc_hash))) && SUCCEEDED(make(pt::kPtSharcAccumulationStride, std::addressof(r.sharc_accumulation)))
        && SUCCEEDED(make(pt::kPtSharcResolvedStride, std::addressof(r.sharc_resolved)))) {
      r.sharc_capacity = sharc_capacity_wanted;
      clear_reason = "allocated";
    } else {
      r.sharc_hash.Reset(); r.sharc_accumulation.Reset(); r.sharc_resolved.Reset();
      r.sharc_failed_capacity = sharc_capacity_wanted;
      pt::PtLogWarn("[world-rt12] pt sharc buffers failed at capacity " + std::to_string(sharc_capacity_wanted) + "; SHARC off");
      sharc_on = false;
    }
  }
  const uint32_t sharc_scale = std::clamp(static_cast<uint32_t>(pt::g_pt_sharc_scene_scale), 1u, 200u);
  const uint32_t sharc_bounces = std::clamp(static_cast<uint32_t>(pt::g_pt_max_bounces), 1u, 16u);
  uint32_t sharc_ds = std::clamp(static_cast<uint32_t>(pt::g_pt_sharc_downscale), 1u, 16u);
  while (sharc_ds < 16u && uint64_t{(w + sharc_ds - 1u) / sharc_ds} * ((h + sharc_ds - 1u) / sharc_ds) > pt::kPtSharcMaxUpdatePaths) ++sharc_ds;
  uint32_t sharc_window = 0u;
  const uint32_t sharc_radiance_wanted = std::bit_cast<uint32_t>(static_cast<float>(rtxdi::FiniteClamped(pt::g_pt_sharc_radiance_scale, 100000.f, 1000.f, 1000000.f)));
  if (sharc_on) {
    if (r.sharc_last_frame == UINT32_MAX || frame - r.sharc_last_frame > pt::kPtSharcGapFrames) {
      if (!clear_reason) {
        clear_reason = "gap";
      }
    }
    if (episode != r.sharc_episode) {
      if (!clear_reason) {
        clear_reason = "map change";
      }
    }
    if (sharc_scale != r.sharc_scene_scale || sharc_bounces != r.sharc_bounces) {
      if (!clear_reason) {
        clear_reason = "settings";
      }
    }
    if (pt::g_pt_sharc_reset_request != r.sharc_reset_seen) {
      if (!clear_reason) {
        clear_reason = "user";
      }
    }
    if (sharc_radiance_wanted != r.sharc_radiance_seen) {
      if (!clear_reason) {
        clear_reason = "radiance";
      }
    }
    if (has_sun) {
      const float length = std::sqrt(light[0] * light[0] + light[1] * light[1] + light[2] * light[2]);
      for (int i = 0; i < 3; ++i) light[i] = length > 0.f ? light[i] / length : 0.f;
      float angle = 0.f, old_energy = 0.f, new_energy = 0.f, difference = 0.f;
      for (int i = 0; i < 3; ++i) angle += light[i] * r.sharc_light[i];
      for (int i = 3; i < 6; ++i) {
        old_energy += r.sharc_light[i];
        new_energy += light[i];
        difference += std::abs(light[i] - r.sharc_light[i]);
      }
      const float energy = (std::max)(old_energy, new_energy);
      if (r.sharc_light_valid && (difference > .25f * energy || (energy > 0.f && angle < .999f))) {
        if (!clear_reason) {
          clear_reason = "sun";
        }
      }
      const bool changed = r.sharc_light_valid && (difference > 1e-7f || angle < .9999999f);
      r.sharc_responsive_frames = changed ? 32u : (r.sharc_responsive_frames ? r.sharc_responsive_frames - 1u : 0u);
      std::copy_n(light, 6, r.sharc_light);
      r.sharc_light_valid = true;
    }
    if (clear_reason) {
      r.sharc_clear = true;
    }
    const uint32_t frames = std::clamp(static_cast<uint32_t>(pt::g_pt_sharc_accumulation_frames), 1u, 1024u);
    sharc_window = r.sharc_responsive_frames ? (std::min)(frames, 4u) : frames;
    r.sharc_episode = episode;
    r.sharc_scene_scale = sharc_scale;
    r.sharc_bounces = sharc_bounces;
    r.sharc_reset_seen = pt::g_pt_sharc_reset_request;
    r.sharc_radiance_seen = sharc_radiance_wanted;
    r.sharc_last_frame = frame;
  } else {
    r.sharc_last_frame = UINT32_MAX;  // the next frame with SHARC on clears it
  }
  // 4c. Material cache (M3): inputs, buffers, clear triggers (a sun change does not clear materials).
  const uint32_t mat_capacity_wanted = 1u << std::clamp(static_cast<uint32_t>(pt::g_pt_material_capacity_log2), 16u, 22u);
  if (r.material_failed_capacity != 0u && r.material_failed_capacity != mat_capacity_wanted) {
    r.material_failed_capacity = 0u;
  }
  bool mat_on = pt::g_pt_material_enabled > 0.5f && r.material_failed_capacity != mat_capacity_wanted
                && input.color.texture12 && input.mrt1.texture12 && input.mrt2.texture12;
  const char* mat_clear_reason = nullptr;
  if (mat_on && r.material_capacity != mat_capacity_wanted) {
    RetireObject(s, s.last_signal, r.material_keys);
    RetireObject(s, s.last_signal, r.material_values);
    r.material_keys.Reset();
    r.material_values.Reset();
    r.material_capacity = 0u;
    const auto make_material = [&](uint64_t stride, ComPtr<ID3D12Resource>* out) {
      return CreateRt12Buffer(s.device12.Get(), D3D12_HEAP_TYPE_DEFAULT, uint64_t{mat_capacity_wanted} * stride,
                              D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, *out);
    };
    if (SUCCEEDED(make_material(8u, std::addressof(r.material_keys)))
        && SUCCEEDED(make_material(pt::kPtMaterialValueStride, std::addressof(r.material_values)))) {
      r.material_capacity = mat_capacity_wanted;
      mat_clear_reason = "allocated";
    } else {
      r.material_keys.Reset();
      r.material_values.Reset();
      r.material_failed_capacity = mat_capacity_wanted;
      pt::PtLogWarn("[world-rt12] pt material buffers failed at capacity " + std::to_string(mat_capacity_wanted) + "; material cache off");
      mat_on = false;
    }
  }
  const uint32_t mat_rate = std::clamp(static_cast<uint32_t>(pt::g_pt_material_rate), 1u, 16u);
  const uint32_t mat_scale = std::clamp(static_cast<uint32_t>(pt::g_pt_material_cell_scale), 1u, 200u);
  const bool mat_scale_changed = mat_on && mat_scale != r.material_cell_scale;
  if (mat_on) {
    if (!mat_clear_reason && (r.material_last_frame == UINT32_MAX || frame - r.material_last_frame > pt::kPtSharcGapFrames)) {
      mat_clear_reason = "gap";
    }
    if (!mat_clear_reason && episode != r.material_episode) {
      mat_clear_reason = "map change";
    }
    if (!mat_clear_reason && mat_scale != r.material_cell_scale) {
      mat_clear_reason = "settings";
    }
    if (!mat_clear_reason && pt::g_pt_sharc_reset_request != r.material_reset_seen) {
      mat_clear_reason = "user";
    }
    if (mat_clear_reason) {
      r.material_clear = true;
    }
    r.material_episode = episode;
    r.material_cell_scale = mat_scale;
    r.material_reset_seen = pt::g_pt_sharc_reset_request;
    r.material_last_frame = frame;
  } else {
    r.material_last_frame = UINT32_MAX;
  }
  if (sharc_on && (mat_on != r.sharc_material_on || mat_scale_changed)) {
    r.sharc_clear = true;
    if (!clear_reason) {
      clear_reason = "material";
    }
  }
  r.sharc_material_on = mat_on;
  if (sharc_on && r.sharc_clear) ++r.cache_epoch;
  if (mat_on && r.material_clear) ++r.cache_epoch;
  const bool gbuf_on = mat_on || refl_on;  // the G-buffer material inputs (t36-t39) serve the material cache and the reflection
  ID3D12Resource* deferred = gbuf_on && input.deferred_count != 0u ? input.deferred_buffer.Get() : nullptr;
  float sharc_prev[3];
  std::copy_n(r.sharc_clear ? camera : r.sharc_prev_camera, 3, sharc_prev);  // sharc.cpp:473
  std::copy_n(camera, 3, r.sharc_prev_camera);
  // 5. Signal A (D3D11), flushed.
  uint64_t a = renodx::utils::gpu_fence::NO_SIGNAL;
  try {
    a = renodx::utils::gpu_fence::QueueSignal(s.ctx11.Get(), s.fence11.get());
    s.ctx11->Flush();
  } catch (const std::exception&) {
    a = renodx::utils::gpu_fence::NO_SIGNAL;
  }
  RecordRt12SignalA(s, a, GpuEvidenceSource::Pt);
  if (a == renodx::utils::gpu_fence::NO_SIGNAL) return submit_fail(Rt12Reason::FenceError, "pt fence11 signal");
  const auto pt_main_cpu_start = bvh::PerfNow();  // M0t: D3D12 record and submit only (no D3D11 flush, copies or waits)
  // 6. Record: descriptors, barriers, trace.
  ID3D12GraphicsCommandList4* list = r.list.Get();
  if (FAILED(pt_slot.allocator->Reset()) || FAILED(list->Reset(pt_slot.allocator.Get(), nullptr))) {
    return submit_fail(Rt12Reason::SubmitFailed, "pt reset");
  }
  const bool pt_perf_gather = bvh::g_perf_gate.gather.load(std::memory_order_relaxed);
  const uint64_t pt_perf_epoch = bvh::g_perf_gate.gather_epoch.load(std::memory_order_relaxed);
  bool pt_perf_timing = false;
  if (rtxdi::CanStartGpuTiming(pt_perf_gather, s.timestamp_frequency, r.perf_pending[k])) {
    if (!r.perf_queries || !r.perf_readback) {
      D3D12_QUERY_HEAP_DESC query_desc = {};
      query_desc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
      query_desc.Count = kRt12PtPerfTimestamps * kRt12PtPerfBlocks;
      if (FAILED(s.device12->CreateQueryHeap(&query_desc, IID_PPV_ARGS(&r.perf_queries)))
          || FAILED(CreateRt12Buffer(s.device12.Get(), D3D12_HEAP_TYPE_READBACK, 80ull * kRt12PtPerfBlocks,
                                     D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_FLAG_NONE, r.perf_readback))) {
        r.perf_queries.Reset();
        r.perf_readback.Reset();
      }
    }
    pt_perf_timing = r.perf_queries && r.perf_readback;
  }
  if (pt_perf_gather && s.timestamp_frequency == 0u) {
    rtxdi::RecordGpuTimingSkipEvent(&bvh::g_perf_gate.pt_timing_skip_events);
  } else if (pt_perf_gather && r.perf_pending[k]) {
    bvh::g_perf_gate.pt_timing_pending_frames.fetch_add(1u, std::memory_order_relaxed);
  } else if (pt_perf_gather && !pt_perf_timing) {
    rtxdi::RecordGpuTimingSkipEvent(&bvh::g_perf_gate.pt_timing_skip_events);  // query heap or readback allocation failed
  }
  ID3D12Device5* device12 = s.device12.Get();
  const uint32_t regir_slot = static_cast<uint32_t>(transfer_slot);
  if (pt_perf_timing) r.perf_valid_mask[k] = 0u;
  const pt::PtReGirIdentity source_identity = input.regir_source_identity;
  const bool regir_requested = pt::g_pt_regir_enabled > 0.5f;
  const bool regir_identity_matches = source_identity.owner_generation == s.generation
                        && source_identity.transfer_configuration == s.di_transfer.configuration_generation
                        && source_identity.table_fence != 0u && source_identity.frame == frame
                        && source_identity.transfer_slot == static_cast<uint32_t>(transfer_slot)
                        && source_identity.light_count == input.light_count && input.light_count <= RTXDI_LIGHT_TABLE_CAPACITY;
  const bool regir_slot_available = r.regir_last_use[regir_slot] <= Rt12CompletedValue(s);
  const bool regir_resources_available = r.regir_root && r.regir_register_pso && r.regir_weights_pso && r.regir_build_pso
                        && r.regir_heap && r.regir_header[regir_slot] && r.regir_cells[regir_slot] && r.regir_weights[regir_slot];
  const bool regir_on = regir_requested && lamps_on && regir_identity_matches && regir_slot_available && regir_resources_available;
  const bool regir_copy_counts = regir_on && !r.regir_counts_pending[regir_slot]
                                 && r.regir_header_readback[regir_slot] && r.regir_cells_readback[regir_slot];
  r.regir_identity[regir_slot] = {};
  const bool sparse_requested = pt::g_pt_sparse_enabled > 0.5f;
  pt::PtSparsePolicy sparse_policy = pt::ComputePtSparsePolicy(
      sparse_requested, pt::g_pt_sparse_adaptive > 0.5f, pt::g_pt_sparse_rate, pt::g_pt_sparse_exponent,
      pt::g_pt_sparse_quality, pt::g_pt_sparse_max_rate, w, h);
  if (sparse_policy.dense_reason == pt::PtSparseDenseReason::None) {
    if (!rr::RrRequested()) {
      sparse_policy.dense_reason = pt::PtSparseDenseReason::RrNotRequested;
    } else if (!rr::RrOwnsDenoise()) {
      sparse_policy.dense_reason = pt::PtSparseDenseReason::RrNotActive;
    } else if (isfast_view.handle == 0u || s.pt_noise_source != isfast_view.handle || !s.noise.texture12) {
      sparse_policy.dense_reason = pt::PtSparseDenseReason::NoiseUnavailable;
    }
  }
  const bool sparse_reflections_requested = pt::g_pt_sparse_reflections > 0.5f;
  if (sparse_policy.dense_reason == pt::PtSparseDenseReason::None && refl_on && !sparse_reflections_requested) {
    sparse_policy.dense_reason = pt::PtSparseDenseReason::ReflectionsDense;
  }
  const bool light_jitter_ready = isfast_view.handle != 0u && s.pt_noise_source == isfast_view.handle && s.noise.texture12;
  const bool sparse_effective = sparse_policy.dense_reason == pt::PtSparseDenseReason::None && sparse_policy.levels < 256u;
  const bool sparse_reflections_effective = sparse_effective && refl_on && sparse_reflections_requested;
  Rt12DiTransferSlot* previous_slot = nullptr;
  bool history_valid = false;
  const char* history_reason = "valid";
  if (!r.history_valid) history_reason = "no_history";
  else if (r.history_frame == UINT32_MAX || r.history_frame + 1u != frame) history_reason = "frame";
  else if (r.history_configuration_generation == 0u || r.history_configuration_generation != r.configuration_generation) history_reason = "configuration";
  else if (r.history_owner_generation == 0u || r.history_owner_generation != s.generation) history_reason = "owner";
  else if (r.history_width != w || r.history_height != h) history_reason = "size";
  else if (r.history_transfer_slot < 0 || r.history_transfer_slot >= static_cast<int32_t>(kRt12Slots)
           || r.history_transfer_slot == static_cast<int32_t>(transfer_slot)) history_reason = "transfer_slot";
  else if (r.history_cache_epoch != r.cache_epoch) history_reason = "cache_epoch";
  else if (r.history_sparse_levels != (sparse_effective ? sparse_policy.levels : 256u)
           || r.history_sparse_reflections_effective != sparse_reflections_effective) history_reason = "sparse";
  else if (!input.motion_available) history_reason = "motion";
  if (r.history_valid && r.history_transfer_slot >= 0 && r.history_transfer_slot < static_cast<int32_t>(kRt12Slots)
      && r.history_frame != UINT32_MAX && r.history_frame + 1u == frame) {
    Rt12DiTransferSlot& candidate = s.di_transfer.slots[static_cast<uint32_t>(r.history_transfer_slot)];
    const bool previous_inputs = candidate.width == w && candidate.height == h
        && candidate.depth.texture12 && candidate.normal.texture12 && candidate.scene.texture12
        && candidate.color.texture12 && candidate.mrt1.texture12 && candidate.mrt2.texture12
        && candidate.deferred_buffer && candidate.lights_buffer && candidate.light_table
        && candidate.motion.texture12 && candidate.motion_available;
    if (rtxdi::ValidDiHistoryIdentity({.previous_frame = r.history_frame,
                                       .current_frame = frame,
                                       .previous_width = r.history_width,
                                       .previous_height = r.history_height,
                                       .current_width = w,
                                       .current_height = h,
                                       .previous_transfer_slot = r.history_transfer_slot,
                                       .current_transfer_slot = static_cast<int32_t>(transfer_slot),
                                       .previous_owner_generation = r.history_owner_generation,
                                       .current_owner_generation = s.generation,
                                       .previous_configuration_generation = r.history_configuration_generation,
                                       .current_configuration_generation = r.configuration_generation,
                                       .motion_available = input.motion_available,
                                       .previous_inputs_available = previous_inputs})
        && r.history_cache_epoch == r.cache_epoch
        && r.history_sparse_levels == (sparse_effective ? sparse_policy.levels : 256u)
        && r.history_sparse_reflections_effective == sparse_reflections_effective) {
      previous_slot = &candidate;
      history_valid = true;
    } else {
      history_reason = !previous_inputs ? "missing_inputs" : (!input.motion_available ? "motion" : "identity");
    }
  }
  if (!history_valid) {
    r.history_frame = UINT32_MAX;
    r.history_width = 0u;
    r.history_height = 0u;
    r.history_transfer_slot = -1;
    r.history_owner_generation = 0u;
    r.history_configuration_generation = 0u;
    r.history_cache_epoch = 0u;
    r.history_sparse_levels = 256u;
    r.history_sparse_reflections_effective = false;
  }
  r.history_valid = history_valid;
  Rt12DiTransferSlot& history_slot = previous_slot ? *previous_slot : input;
  const uint32_t base = k * kRt12PtDescriptors;
  const auto cpu_at = [&r, &s](uint32_t index) {
    D3D12_CPU_DESCRIPTOR_HANDLE handle = r.heap->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += static_cast<SIZE_T>(index) * s.descriptor_size;
    return handle;
  };
  uint32_t layout_mismatch = 0u;
  const auto set_srv = [&](uint32_t index, ID3D12Resource* resource, const D3D12_SHADER_RESOURCE_VIEW_DESC& view) {
    if (Rt12PtDescriptorIsUav(index) != false) layout_mismatch = index + 1u;
    device12->CreateShaderResourceView(resource, &view, cpu_at(base + index));
    r.water_frame_inputs.srv_descs[index] = view;
    r.water_frame_inputs.resources[index] = resource;
  };
  set_srv(0u, vertices, Rt12BufferSrv(kRt12ArenaStride[kRt12ArenaVertices], vertex_count));
  set_srv(1u, indices, Rt12BufferSrv(kRt12ArenaStride[kRt12ArenaIndices], index_count));
  set_srv(2u, mesh_table, Rt12BufferSrv(sizeof(bvh::WorldMeshGPU), mesh_count));
  set_srv(3u, scene.instance_table.Get(), Rt12BufferSrv(sizeof(bvh::WorldInstanceGPU), scene.instance_count));
  set_srv(4u, uv_arena, Rt12BufferSrv(kRt12ArenaStride[kRt12ArenaUvs], uv_count));
  D3D12_SHADER_RESOURCE_VIEW_DESC atlas_srv = {};
  atlas_srv.Format = DXGI_FORMAT_R32_UINT;
  atlas_srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
  atlas_srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  atlas_srv.Texture2DArray.MipLevels = 1u;
  atlas_srv.Texture2DArray.ArraySize = kRt12AlphaSlices;
  set_srv(5u, s.atlas.texture12.Get(), atlas_srv);
  set_srv(6u, s.materials.Get(), Rt12BufferSrv(32u, kRt12AlphaSlices));
  D3D12_SHADER_RESOURCE_VIEW_DESC tlas_srv = {};
  tlas_srv.Format = DXGI_FORMAT_UNKNOWN;
  tlas_srv.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
  tlas_srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  tlas_srv.RaytracingAccelerationStructure.Location = scene.tlas->GetGPUVirtualAddress();
  if (Rt12PtDescriptorIsUav(7u) != false) layout_mismatch = 7u + 1u;
  device12->CreateShaderResourceView(nullptr, &tlas_srv, cpu_at(base + 7u));
  r.water_frame_inputs.srv_descs[7u] = tlas_srv;
  r.water_frame_inputs.tlas = scene.tlas;
  set_srv(8u, dynamic_vertices, Rt12BufferSrv(16u, dynamic_vertices ? uint64_t{kRt12CarrierWidth} * kRt12CarrierHeight : 1u));
  set_srv(9u, input.depth.texture12.Get(), Rt12TextureSrv(DXGI_FORMAT_R32_FLOAT));
  set_srv(10u, input.normal.texture12.Get(), Rt12TextureSrv(DXGI_FORMAT_R32G32B32A32_UINT));
  set_srv(11u, input.scene.texture12.Get(), Rt12TextureSrv(DXGI_FORMAT_R32G32B32A32_UINT));
  set_srv(12u, lights, Rt12BufferSrv(RTXDI_INPUT_LIGHT_STRIDE, lamps_on ? input.light_count : 1u));
  set_srv(13u, light_table, Rt12BufferSrv(sizeof(RtxdiLightTableEntry), RTXDI_LIGHT_TABLE_CAPACITY));
  const D3D12_UNORDERED_ACCESS_VIEW_DESC out_uav = Rt12TextureUav(DXGI_FORMAT_R16G16B16A16_FLOAT);
  if (Rt12PtDescriptorIsUav(14u) != true) layout_mismatch = 14u + 1u;
  device12->CreateUnorderedAccessView(out.texture12.Get(), nullptr, &out_uav, cpu_at(base + 14u));
  r.water_frame_inputs.uav_descs[14u] = out_uav;
  if (Rt12PtDescriptorIsUav(25u) != true) layout_mismatch = 25u + 1u;
  device12->CreateUnorderedAccessView(refl_target ? refl.texture12.Get() : nullptr, nullptr, &out_uav, cpu_at(base + 25u));
  r.water_frame_inputs.uav_descs[25u] = out_uav;
  const auto set_buffer_uav = [&](uint32_t index, ID3D12Resource* resource, uint32_t stride) {
    if (Rt12PtDescriptorIsUav(index) != true) layout_mismatch = index + 1u;
    D3D12_UNORDERED_ACCESS_VIEW_DESC view = {};
    view.Format = DXGI_FORMAT_UNKNOWN;
    view.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    view.Buffer.NumElements = sharc_on ? r.sharc_capacity : 1u;
    view.Buffer.StructureByteStride = stride;
    device12->CreateUnorderedAccessView(resource, nullptr, &view, cpu_at(base + index));
    r.water_frame_inputs.uav_descs[index] = view;
    r.water_frame_inputs.resources[index] = resource;
  };
  set_buffer_uav(15u, sharc_on ? r.sharc_hash.Get() : nullptr, 8u);
  set_buffer_uav(16u, sharc_on ? r.sharc_accumulation.Get() : nullptr, pt::kPtSharcAccumulationStride);
  set_buffer_uav(17u, sharc_on ? r.sharc_resolved.Get() : nullptr, pt::kPtSharcResolvedStride);
  ID3D12Resource* const sharc_buffers[] = {sharc_on ? r.sharc_hash.Get() : nullptr, sharc_on ? r.sharc_accumulation.Get() : nullptr,
                                           sharc_on ? r.sharc_resolved.Get() : nullptr};
  set_srv(18u, gbuf_on ? input.color.texture12.Get() : nullptr, Rt12TextureSrv(DXGI_FORMAT_R32G32B32A32_FLOAT));
  set_srv(19u, gbuf_on ? input.mrt1.texture12.Get() : nullptr, Rt12TextureSrv(DXGI_FORMAT_R32G32B32A32_UINT));
  set_srv(20u, gbuf_on ? input.mrt2.texture12.Get() : nullptr, Rt12TextureSrv(DXGI_FORMAT_R32G32B32A32_UINT));
  set_srv(21u, deferred, Rt12BufferSrv(RTXDI_INPUT_DEFERRED_STRIDE, deferred ? input.deferred_count : 1u));
  set_srv(26u, sparse_effective || light_jitter_ready ? s.noise.texture12.Get() : nullptr,
          Rt12TextureSrv(DXGI_FORMAT_R32G32B32A32_UINT));
  set_srv(27u, regir_on ? r.regir_header[regir_slot].Get() : nullptr, Rt12BufferSrv(kRt12ReGirHeaderStride, 1u));
  set_srv(28u, regir_on ? r.regir_cells[regir_slot].Get() : nullptr, Rt12BufferSrv(kRt12ReGirCellStride, pt::kPtReGirMaxCells));
  D3D12_SHADER_RESOURCE_VIEW_DESC neighbor_srv = {};
  neighbor_srv.Format = DXGI_FORMAT_R8G8_SNORM;
  neighbor_srv.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
  neighbor_srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  neighbor_srv.Buffer.NumElements = rtxdi::kNeighborCount;
  set_srv(30u, r.neighbors.Get(), neighbor_srv);
  set_srv(31u, history_valid ? history_slot.depth.texture12.Get() : nullptr, Rt12TextureSrv(DXGI_FORMAT_R32_FLOAT));
  set_srv(32u, history_valid ? history_slot.normal.texture12.Get() : nullptr, Rt12TextureSrv(DXGI_FORMAT_R32G32B32A32_UINT));
  set_srv(33u, history_valid ? history_slot.scene.texture12.Get() : nullptr, Rt12TextureSrv(DXGI_FORMAT_R32G32B32A32_UINT));
  set_srv(34u, history_valid ? history_slot.color.texture12.Get() : nullptr, Rt12TextureSrv(DXGI_FORMAT_R32G32B32A32_FLOAT));
  set_srv(35u, history_valid ? history_slot.mrt1.texture12.Get() : nullptr, Rt12TextureSrv(DXGI_FORMAT_R32G32B32A32_UINT));
  set_srv(36u, history_valid ? history_slot.mrt2.texture12.Get() : nullptr, Rt12TextureSrv(DXGI_FORMAT_R32G32B32A32_UINT));
  set_srv(37u, history_valid ? history_slot.deferred_buffer.Get() : nullptr, Rt12BufferSrv(RTXDI_INPUT_DEFERRED_STRIDE,
                                                                 history_slot.deferred_count ? history_slot.deferred_count : 1u));
  set_srv(38u, history_valid ? history_slot.lights_buffer.Get() : nullptr, Rt12BufferSrv(RTXDI_INPUT_LIGHT_STRIDE,
                                                               history_slot.light_count ? history_slot.light_count : 1u));
  set_srv(39u, history_valid ? history_slot.light_table.Get() : nullptr, Rt12BufferSrv(sizeof(RtxdiLightTableEntry), RTXDI_LIGHT_TABLE_CAPACITY));
  set_srv(40u, input.motion.texture12.Get(), Rt12TextureSrv(DXGI_FORMAT_R32G32B32A32_FLOAT));
  set_srv(41u, input.previous_to_current.Get(), Rt12BufferSrv(sizeof(uint32_t), RTXDI_LIGHT_TABLE_CAPACITY));
  set_srv(42u, input.current_to_previous.Get(), Rt12BufferSrv(sizeof(uint32_t), RTXDI_LIGHT_TABLE_CAPACITY));
  const auto set_texture_uav = [&](uint32_t index, ID3D12Resource* resource, DXGI_FORMAT format) {
    if (Rt12PtDescriptorIsUav(index) != true) layout_mismatch = index + 1u;
    D3D12_UNORDERED_ACCESS_VIEW_DESC view = Rt12TextureUav(format);
    device12->CreateUnorderedAccessView(resource, nullptr, &view, cpu_at(base + index));
    r.water_frame_inputs.uav_descs[index] = view;
    r.water_frame_inputs.resources[index] = resource;
  };
  set_texture_uav(43u, r.duplication_map.Get(), DXGI_FORMAT_R32G32_FLOAT);
  set_texture_uav(44u, r.smoothed_duplication[frame & 1u].Get(), DXGI_FORMAT_R32_FLOAT);
  set_texture_uav(45u, r.sample_id.Get(), DXGI_FORMAT_R32_UINT);
  set_texture_uav(46u, r.neighbor_selection_gbuffer.Get(), DXGI_FORMAT_R16G16B16A16_FLOAT);
  D3D12_UNORDERED_ACCESS_VIEW_DESC neighbor_uav = {};
  neighbor_uav.Format = DXGI_FORMAT_R32_TYPELESS;
  neighbor_uav.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
  neighbor_uav.Buffer.NumElements = static_cast<UINT>(uint64_t{w} * h * 4u);
  neighbor_uav.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
  if (Rt12PtDescriptorIsUav(47u) != true) layout_mismatch = 47u + 1u;
  device12->CreateUnorderedAccessView(r.neighbor_selection.Get(), nullptr, &neighbor_uav, cpu_at(base + 47u));
  r.water_frame_inputs.uav_descs[47u] = neighbor_uav;
  r.water_frame_inputs.resources[47u] = r.neighbor_selection;
  set_srv(48u, r.smoothed_duplication[(frame + 1u) & 1u].Get(), Rt12TextureSrv(DXGI_FORMAT_R32_FLOAT));
  set_srv(49u, lamps ? input.tiles_buffer.Get() : nullptr,
          Rt12BufferSrv(RTXDI_INPUT_TILE_STRIDE, lamps ? input.tile_count : 1u));
  ID3D12Resource* const debug_textures[] = {debug_on ? r.debug_value[k].texture12.Get() : nullptr,
                                            debug_on ? r.debug_status[k].texture12.Get() : nullptr,
                                            debug_on ? r.sun_detail[k].texture12.Get() : nullptr};
  for (uint32_t i = 0u; i < 3u; ++i) set_texture_uav(50u + i, debug_textures[i], debug_formats[i]);
  set_texture_uav(53u, game_sun_shadows ? sun_vis.texture12.Get() : nullptr, DXGI_FORMAT_R16G16B16A16_FLOAT);
  set_texture_uav(54u, initial_color.texture12.Get(), DXGI_FORMAT_R16G16B16A16_FLOAT);
  D3D12_UNORDERED_ACCESS_VIEW_DESC reservoir_uav = {};
  reservoir_uav.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
  reservoir_uav.Buffer.NumElements = r.restir->GetReservoirBufferParameters().reservoirArrayPitch
                                     * ::rtxdi::c_NumReSTIRPTReservoirBuffers;
  reservoir_uav.Buffer.StructureByteStride = sizeof(RTXDI_PackedPTReservoir);
  if (Rt12PtDescriptorIsUav(29u) != true) layout_mismatch = 29u + 1u;
  device12->CreateUnorderedAccessView(r.reservoirs.Get(), nullptr, &reservoir_uav, cpu_at(base + 29u));
  r.water_frame_inputs.uav_descs[29u] = reservoir_uav;
  r.water_frame_inputs.resources[29u] = r.reservoirs;
  const auto set_material_uav = [&](uint32_t index, ID3D12Resource* resource, uint32_t stride, uint32_t count) {
    if (Rt12PtDescriptorIsUav(index) != true) layout_mismatch = index + 1u;
    D3D12_UNORDERED_ACCESS_VIEW_DESC view = {};
    view.Format = DXGI_FORMAT_UNKNOWN;
    view.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    view.Buffer.NumElements = count;
    view.Buffer.StructureByteStride = stride;
    device12->CreateUnorderedAccessView(resource, nullptr, &view, cpu_at(base + index));
    r.water_frame_inputs.uav_descs[index] = view;
    r.water_frame_inputs.resources[index] = resource;
  };
  set_material_uav(22u, mat_on ? r.material_keys.Get() : nullptr, 8u, mat_on ? r.material_capacity : 1u);
  set_material_uav(23u, mat_on ? r.material_values.Get() : nullptr, 4u, mat_on ? r.material_capacity * 4u : 1u);
  set_material_uav(24u, r.counters[k].Get(), 4u, kRt12PtCounters);
  if (layout_mismatch != 0u) return fail("pt descriptor layout", static_cast<HRESULT>(0x80000000u | (layout_mismatch - 1u)));
  ID3D12Resource* const material_buffers[] = {mat_on ? r.material_keys.Get() : nullptr, mat_on ? r.material_values.Get() : nullptr};
  ID3D12Resource* const material_reads[] = {gbuf_on ? input.color.texture12.Get() : nullptr, gbuf_on ? input.mrt1.texture12.Get() : nullptr,
                                            gbuf_on ? input.mrt2.texture12.Get() : nullptr, deferred};
  D3D12_GPU_DESCRIPTOR_HANDLE table = r.heap->GetGPUDescriptorHandleForHeapStart();
  table.ptr += static_cast<UINT64>(base) * s.descriptor_size;
  ID3D12Resource* const reads[] = {vertices, indices, mesh_table, scene.instance_table.Get(), uv_arena, s.atlas.texture12.Get(),
                                   s.materials.Get(), dynamic_vertices, input.depth.texture12.Get(), input.normal.texture12.Get(),
                                   input.scene.texture12.Get(), lights, light_table};
  std::vector<D3D12_RESOURCE_BARRIER>& barriers = pt_slot.barrier_scratch;
  barriers.clear();
  for (ID3D12Resource* resource : reads) {
    if (resource) Transition(barriers, resource, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
  }
  if (previous_slot) {
    for (ID3D12Resource* resource : {history_slot.depth.texture12.Get(), history_slot.normal.texture12.Get(),
                                     history_slot.scene.texture12.Get(), history_slot.color.texture12.Get(),
                                     history_slot.mrt1.texture12.Get(), history_slot.mrt2.texture12.Get(),
                                     history_slot.deferred_buffer.Get(), history_slot.lights_buffer.Get(),
                                     history_slot.light_table.Get()}) {
      if (resource) Transition(barriers, resource, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    }
  }
  for (ID3D12Resource* resource : {input.motion.texture12.Get(), input.previous_to_current.Get(),
                                   input.current_to_previous.Get()}) {
    if (resource) Transition(barriers, resource, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
  }
  if (sparse_effective || light_jitter_ready) {
    Transition(barriers, s.noise.texture12.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
  }
  Transition(barriers, out.texture12.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
  if (refl_target) Transition(barriers, refl.texture12.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
  for (ID3D12Resource* resource : sharc_buffers) {
    if (resource) {
      Transition(barriers, resource, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    }
  }
  for (ID3D12Resource* resource : material_reads) {
    if (resource) Transition(barriers, resource, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
  }
  for (ID3D12Resource* resource : material_buffers) {
    if (resource) Transition(barriers, resource, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
  }
  Transition(barriers, r.reservoirs.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
  for (ID3D12Resource* resource : {r.sample_id.Get(), r.duplication_map.Get(), r.smoothed_duplication[frame & 1u].Get(),
                                   r.neighbor_selection_gbuffer.Get(), r.neighbor_selection.Get()}) {
    Transition(barriers, resource, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
  }
  Transition(barriers, r.smoothed_duplication[(frame + 1u) & 1u].Get(), D3D12_RESOURCE_STATE_COMMON,
             D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
  Transition(barriers, r.counters[k].Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
  list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
  list->CopyBufferRegion(r.counters[k].Get(), 0u, r.counter_zero.Get(), 0u, kRt12PtCounters * sizeof(uint32_t));
  barriers.clear();
  Transition(barriers, r.counters[k].Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
  list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
  if (regir_on) {
    const uint32_t regir_base = regir_slot * kRt12ReGirPasses * kRt12ReGirDescriptors;
    const auto regir_cpu = [&](uint32_t pass, uint32_t index) {
      D3D12_CPU_DESCRIPTOR_HANDLE handle = r.regir_heap->GetCPUDescriptorHandleForHeapStart();
      handle.ptr += static_cast<SIZE_T>(regir_base + pass * kRt12ReGirDescriptors + index) * s.descriptor_size;
      return handle;
    };
    const auto regir_srv = [&](uint32_t pass, uint32_t index, ID3D12Resource* resource, const D3D12_SHADER_RESOURCE_VIEW_DESC& view) {
      device12->CreateShaderResourceView(resource, &view, regir_cpu(pass, index));
    };
    const auto regir_uav = [&](uint32_t pass, uint32_t index, ID3D12Resource* resource, uint32_t stride, uint32_t count) {
      D3D12_UNORDERED_ACCESS_VIEW_DESC view = {};
      view.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
      view.Buffer.NumElements = count;
      view.Buffer.StructureByteStride = stride;
      device12->CreateUnorderedAccessView(resource, nullptr, &view, regir_cpu(pass, 3u + index));
    };
    ID3D12Resource* header = r.regir_header[regir_slot].Get();
    ID3D12Resource* cells = r.regir_cells[regir_slot].Get();
    ID3D12Resource* weights = r.regir_weights[regir_slot].Get();
    regir_srv(0u, 0u, input.depth.texture12.Get(), Rt12TextureSrv(DXGI_FORMAT_R32_FLOAT));
    regir_srv(0u, 1u, input.normal.texture12.Get(), Rt12TextureSrv(DXGI_FORMAT_R32G32B32A32_UINT));
    regir_srv(0u, 2u, input.scene.texture12.Get(), Rt12TextureSrv(DXGI_FORMAT_R32G32B32A32_UINT));
    regir_uav(0u, 0u, header, kRt12ReGirHeaderStride, 1u);
    regir_uav(0u, 1u, cells, kRt12ReGirCellStride, pt::kPtReGirMaxCells);
    regir_srv(1u, 0u, light_table, Rt12BufferSrv(sizeof(RtxdiLightTableEntry), RTXDI_LIGHT_TABLE_CAPACITY));
    regir_srv(1u, 1u, header, Rt12BufferSrv(kRt12ReGirHeaderStride, 1u));
    regir_srv(1u, 2u, cells, Rt12BufferSrv(kRt12ReGirCellStride, pt::kPtReGirMaxCells));
    regir_uav(1u, 0u, weights, sizeof(float), pt::kPtReGirMaxCells * RTXDI_LIGHT_TABLE_CAPACITY);
    regir_uav(1u, 1u, nullptr, sizeof(uint32_t), 1u);
    regir_srv(2u, 0u, header, Rt12BufferSrv(kRt12ReGirHeaderStride, 1u));
    regir_srv(2u, 1u, weights, Rt12BufferSrv(sizeof(float), pt::kPtReGirMaxCells * RTXDI_LIGHT_TABLE_CAPACITY));
    regir_srv(2u, 2u, nullptr, Rt12BufferSrv(sizeof(uint32_t), 1u));
    regir_uav(2u, 0u, cells, kRt12ReGirCellStride, pt::kPtReGirMaxCells);
    regir_uav(2u, 1u, nullptr, sizeof(uint32_t), 1u);
    barriers.clear();
    for (ID3D12Resource* resource : {header, cells, weights}) {
      Transition(barriers, resource, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    }
    list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
    ID3D12DescriptorHeap* regir_heaps[] = {r.regir_heap.Get()};
    list->SetDescriptorHeaps(1u, regir_heaps);
    list->SetComputeRootSignature(r.regir_root.Get());
    const auto regir_table = [&](uint32_t pass) {
      D3D12_GPU_DESCRIPTOR_HANDLE handle = r.regir_heap->GetGPUDescriptorHandleForHeapStart();
      handle.ptr += static_cast<UINT64>(regir_base + pass * kRt12ReGirDescriptors) * s.descriptor_size;
      list->SetComputeRootDescriptorTable(1u, handle);
    };
    D3D12_GPU_DESCRIPTOR_HANDLE header_gpu = r.regir_heap->GetGPUDescriptorHandleForHeapStart();
    header_gpu.ptr += static_cast<UINT64>(regir_base + 3u) * s.descriptor_size;
    const uint32_t clear_header[4] = {};
    list->ClearUnorderedAccessViewUint(header_gpu, regir_cpu(0u, 3u), header, clear_header, 0u, nullptr);
    barriers.clear();
    UavBarrier(barriers, header);
    list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
    const uint32_t register_params[] = {w, h};
    list->SetPipelineState(r.regir_register_pso.Get());
    list->SetComputeRoot32BitConstants(0u, kRt12ReGirConstants, register_params, 0u);
    regir_table(0u);
    if (pt_perf_timing) list->EndQuery(r.perf_queries.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 2u * kRt12PtPerfBlocks + 8u * k + 0u);
    list->Dispatch(1u, 1u, 1u);
    if (pt_perf_timing) {
      list->EndQuery(r.perf_queries.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 2u * kRt12PtPerfBlocks + 8u * k + 1u);
      r.perf_valid_mask[k] |= 1u;
    }
    barriers.clear();
    UavBarrier(barriers, header);
    UavBarrier(barriers, cells);
    Transition(barriers, header, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    Transition(barriers, cells, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
    const uint32_t build_params[] = {input.light_count, frame};
    list->SetPipelineState(r.regir_weights_pso.Get());
    list->SetComputeRoot32BitConstants(0u, kRt12ReGirConstants, build_params, 0u);
    regir_table(1u);
    if (pt_perf_timing) list->EndQuery(r.perf_queries.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 2u * kRt12PtPerfBlocks + 8u * k + 2u);
    list->Dispatch(RTXDI_LIGHT_TABLE_CAPACITY / 64u, pt::kPtReGirMaxCells, 1u);
    if (pt_perf_timing) {
      list->EndQuery(r.perf_queries.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 2u * kRt12PtPerfBlocks + 8u * k + 3u);
      r.perf_valid_mask[k] |= 2u;
    }
    barriers.clear();
    UavBarrier(barriers, weights);
    Transition(barriers, weights, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    Transition(barriers, cells, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
    list->SetPipelineState(r.regir_build_pso.Get());
    regir_table(2u);
    if (pt_perf_timing) list->EndQuery(r.perf_queries.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 2u * kRt12PtPerfBlocks + 8u * k + 4u);
    list->Dispatch((pt::kPtReGirMaxCells + 63u) / 64u, 1u, 1u);
    if (pt_perf_timing) {
      list->EndQuery(r.perf_queries.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 2u * kRt12PtPerfBlocks + 8u * k + 5u);
      r.perf_valid_mask[k] |= 4u;
    }
    barriers.clear();
    UavBarrier(barriers, cells);
    Transition(barriers, cells, D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
               regir_copy_counts ? D3D12_RESOURCE_STATE_COPY_SOURCE : D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    if (regir_copy_counts) Transition(barriers, header, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_SOURCE);
    list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
    if (regir_copy_counts) {
      list->CopyBufferRegion(r.regir_header_readback[regir_slot].Get(), 0u, header, 0u, kRt12ReGirHeaderStride);
      list->CopyBufferRegion(r.regir_cells_readback[regir_slot].Get(), 0u, cells, 0u, pt::kPtReGirMaxCells * kRt12ReGirCellStride);
      barriers.clear();
      Transition(barriers, header, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
      Transition(barriers, cells, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
      list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
    }
  }
  uint32_t params[kRt12PtConstants] = {
      w, h, frame,
      std::clamp(static_cast<uint32_t>(pt::g_pt_samples), 1u, 16u),
      std::clamp(static_cast<uint32_t>(pt::g_pt_max_bounces), 1u, 16u),
      pt::g_pt_russian_roulette > 0.5f ? 1u : 0u,
      std::clamp(static_cast<uint32_t>(pt::g_pt_roulette_min_bounce), 1u, 16u),
      lamps_on ? input.light_count : 0u,
      pt::g_pt_sun_casters > 0.5f ? 1u : 0u,
      dynamic_vertices ? 1u : 0u,
      std::bit_cast<uint32_t>(rtxdi::FiniteClamped(pt::g_pt_ray_max, 500.f, 1.f, 500.f)),
      std::bit_cast<uint32_t>(rtxdi::FiniteClamped(pt::g_pt_env_bias, 0.02f, 0.f, 0.5f)),
      std::bit_cast<uint32_t>(rtxdi::FiniteClamped(pt::g_pt_char_bias, 0.02f, 0.f, 0.5f)),
      std::bit_cast<uint32_t>(rtxdi::FiniteClamped(pt::g_pt_lamp_clearance, 0.2f, 0.f, 1.f)),
      std::bit_cast<uint32_t>(rtxdi::FiniteClamped(pt::g_pt_sky_intensity, 1.f, 0.f, 4.f)),
      sharc_on ? 1u : 0u,
      r.sharc_capacity, sharc_ds, sharc_window, std::clamp(static_cast<uint32_t>(pt::g_pt_sharc_stale_frames), 8u, 1024u),
      std::bit_cast<uint32_t>(static_cast<float>(sharc_scale)),
      std::bit_cast<uint32_t>(camera[0]), std::bit_cast<uint32_t>(camera[1]), std::bit_cast<uint32_t>(camera[2]),
      std::bit_cast<uint32_t>(sharc_prev[0]), std::bit_cast<uint32_t>(sharc_prev[1]), std::bit_cast<uint32_t>(sharc_prev[2]),
      0u,
      mat_on ? 1u : 0u, r.material_capacity, mat_rate, std::bit_cast<uint32_t>(static_cast<float>(mat_scale)),
      deferred ? input.deferred_count : 0u,
      std::bit_cast<uint32_t>(rtxdi::FiniteClamped(pt::g_pt_char_roughness, 0.5f, 0.05f, 1.f)),
      std::bit_cast<uint32_t>(rtxdi::FiniteClamped(pt::g_pt_char_f0, 0.04f, 0.f, 0.2f)),
      0u, sharc_radiance_wanted,
      refl_on ? 1u : 0u,  // g_refl_enabled
      std::bit_cast<uint32_t>(rtxdi::FiniteClamped(pt::g_pt_reflection_intensity, 1.f, 0.f, 2.f)),
      std::bit_cast<uint32_t>(rtxdi::FiniteClamped(pt::g_pt_reflection_roughness_scale, 1.f, 0.f, 2.f)),
      pt::g_pt_reflection_clamp_enabled > 0.5f ? std::bit_cast<uint32_t>(rtxdi::FiniteClamped(pt::g_pt_reflection_sample_clamp, 16.f, 1.f, 1000.f)) : 0u,
      (pt::g_pt_reflection_cap_to_one > 0.5f ? 1u : 0u) | (pt::g_pt_reflection_use_sharc > 0.5f ? 2u : 0u) |
          (pt::g_pt_reflection_footprint > 0.5f ? 4u : 0u) | (pt::g_pt_reflection_water_rule > 0.5f ? 8u : 0u) |
          (sparse_reflections_requested ? 32u : 0u),
      std::bit_cast<uint32_t>(rtxdi::FiniteClamped(pt::g_pt_reflection_water_roughness, 0.05f, 0.f, 1.f)),
      pt::g_pt_gi_clamp_enabled > 0.5f ? std::bit_cast<uint32_t>(rtxdi::FiniteClamped(pt::g_pt_gi_clamp, 16.f, 1.f, 1000.f)) : 0u,  // [42] water roughness, [43] GI clamp (0 = off)
      pt::g_pt_sharc_firefly_clamp_enabled > 0.5f ? 1u : 0u,  // [44] SHARC update clamp on, [45] its limit (multiple of the cell mean)
      std::bit_cast<uint32_t>(rtxdi::FiniteClamped(pt::g_pt_sharc_firefly_clamp, 10.f, 1.f, 100.f)),
      sparse_requested ? sparse_policy.levels : 0u, sparse_policy.dense_reason == pt::PtSparseDenseReason::None ? 1u : 0u,
      regir_on ? 1u : 0u, 0u, 0u, 0u};
  const bool rr = rr::RrRequested() && rr::RrOwnsDenoise();
  const uint32_t resampling_mode = (std::clamp)(static_cast<uint32_t>(pt::g_restir_resampling), 0u, 3u);
  r.restir->SetFrameIndex(frame);
  r.restir->SetResamplingMode(static_cast<::rtxdi::ReSTIRPT_ResamplingMode>(resampling_mode));
  r.restir->SetInitialSamplingParameters({.numInitialSamples = (std::clamp)(
      static_cast<uint32_t>(pt::g_pt_samples), 1u, 16u),
      .maxBounceDepth = static_cast<uint32_t>(pt::g_pt_max_bounces) + 1u,
      .maxRcVertexLength = static_cast<uint32_t>(std::clamp(pt::g_restir_max_rc_vertex_length, 2.f, 8.f))});
  RTXDI_PTTemporalResamplingParameters temporal = ::rtxdi::GetDefaultReSTIRPTTemporalResamplingParams(rr);
  temporal.depthThreshold = rtxdi::FiniteClamped(pt::g_restir_temporal_depth_threshold, 0.1f, 0.f, 1.f);
  temporal.normalThreshold = rtxdi::FiniteClamped(pt::g_restir_temporal_normal_threshold, 0.6f, 0.f, 1.f);
  temporal.enablePermutationSampling = pt::g_restir_permutation_sampling > 0.5f ? 1u : 0u;
  temporal.enableVisibilityBeforeCombine = pt::g_restir_temporal_visibility > 0.5f ? 1u : 0u;
  r.restir->SetTemporalResamplingParameters(temporal);
  RTXDI_PTSpatialResamplingParameters spatial = ::rtxdi::GetDefaultReSTIRPTSpatialResamplingParams(rr);
  spatial.numSpatialSamples = (std::clamp)(static_cast<uint32_t>(pt::g_restir_spatial_samples), 1u, 8u);
  spatial.numDisocclusionBoostSamples = (std::clamp)(static_cast<uint32_t>(pt::g_restir_disocclusion_boost_samples), 0u, 16u);
  spatial.samplingRadius = rtxdi::FiniteClamped(pt::g_restir_spatial_radius, 32.f, 1.f, 64.f);
  spatial.depthThreshold = rtxdi::FiniteClamped(pt::g_restir_spatial_depth_threshold, 0.1f, 0.f, 1.f);
  spatial.normalThreshold = rtxdi::FiniteClamped(pt::g_restir_spatial_normal_threshold, 0.6f, 0.f, 1.f);
  spatial.enableSpatialHeuristicMode = pt::g_restir_compatibility_neighbors > 0.5f ? 1u : 0u;
  r.restir->SetSpatialResamplingParameters(spatial);
  RTXDI_BoilingFilterParameters boiling = ::rtxdi::GetDefaultReSTIRPTBoilingFilterParams(rr);
  boiling.enableBoilingFilter = pt::g_restir_boiling_filter > 0.5f ? 1u : 0u;
  boiling.boilingFilterStrength = rtxdi::FiniteClamped(pt::g_restir_boiling_filter_strength, 0.2f, 0.f, 1.f);
  r.restir->SetBoilingFilterParameters(boiling);
  RTXDI_PTReconnectionParameters reconnection = ::rtxdi::GetDefaultReSTIRPTReconnectionParameters();
  reconnection.minConnectionFootprint = pt::g_restir_min_connection_footprint;
  reconnection.minConnectionFootprintSigma = pt::g_restir_min_connection_footprint_sigma;
  reconnection.minPdfRoughness = pt::g_restir_min_pdf_roughness;
  reconnection.minPdfRoughnessSigma = pt::g_restir_min_pdf_roughness_sigma;
  reconnection.roughnessThreshold = pt::g_restir_roughness_threshold;
  reconnection.distanceThreshold = pt::g_restir_distance_threshold;
  reconnection.reconnectionMode = pt::g_restir_reconnection_mode > 0.5f ? RTXDI_PTReconnectionMode::Footprint
                                                                       : RTXDI_PTReconnectionMode::FixedThreshold;
  r.restir->SetReconnectionParameters(reconnection);
  r.restir->SetHybridShiftParameters({.maxBounceDepth = static_cast<uint32_t>(pt::g_pt_max_bounces) + 1u,
                                      .maxRcVertexLength = static_cast<uint32_t>(std::clamp(pt::g_restir_max_rc_vertex_length, 2.f, 8.f))});
  RTXDI_PTDecorrelationParameters decorrelation = ::rtxdi::GetDefaultReSTIRPTDecorrelationParams(rr);
  decorrelation.decorrelationFactor = rtxdi::FiniteClamped(pt::g_restir_decorrelation_factor, 0.4f, 0.f, 1.f);
  r.restir->SetDecorrelationParameters(decorrelation);
  PtRestirConstants restir_constants = {};
  restir_constants.runtime.frameIndex = frame;
  restir_constants.light_buffer.localLightBufferRegion = {0u, lamps_on ? input.light_count : 0u, 0u, 0u};
  restir_constants.light_buffer.infiniteLightBufferRegion = {lamps_on ? input.light_count : 0u, 1u, 0u, 0u};
  restir_constants.light_buffer.environmentLightParams.lightPresent = 0u;
  restir_constants.nee = {.numLocalLightSamples = static_cast<uint32_t>(std::clamp(pt::g_restir_nee_local_samples, 0.f, 32.f)),
                          .numInfiniteLightSamples = 1u, .numEnvironmentSamples = 0u, .numBrdfSamples = 0u,
                          .brdfCutoff = 0.f, .brdfRayMinT = 0.f,
                          .localLightSamplingMode = ReSTIRDI_LocalLightSamplingMode::Uniform,
                          .enableInitialVisibility = 0u};
  restir_constants.restir.reservoirBuffer = r.restir->GetReservoirBufferParameters();
  restir_constants.restir.bufferIndices = r.restir->GetBufferIndices();
  restir_constants.restir.initialSampling = r.restir->GetInitialSamplingParameters();
  restir_constants.restir.decorrelation = r.restir->GetDecorrelationParameters();
  restir_constants.restir.reconnection = reconnection;
  restir_constants.restir.temporalResampling = r.restir->GetTemporalResamplingParameters();
  restir_constants.restir.hybridShift = r.restir->GetHybridShiftParameters();
  restir_constants.restir.boilingFilter = r.restir->GetBoilingFilterParameters();
  restir_constants.restir.spatialResampling = r.restir->GetSpatialResamplingParameters();
  restir_constants.prev_light_count = history_slot.light_count;
  restir_constants.prev_deferred_count = history_slot.deferred_count;
  restir_constants.stable_light_remap = pt::g_restir_stable_light_remap > 0.5f ? 1u : 0u;
  restir_constants.history_valid = history_valid ? 1u : 0u;
  const auto di_clamped = di::FiniteClamped;
  restir_constants.sun_angular_radius =
      di_clamped(di::g_sun_light_size, 0.f, 0.f, 5.f) * 0.5f * std::numbers::pi_v<float> / 180.f;
  restir_constants.lamp_radius = di_clamped(di::g_local_light_size, 0.f, 0.f, 5.f);
  restir_constants.isfast_light_jitter =
      (di::g_sun_isfast > 0.5f || di::g_local_isfast > 0.5f) && light_jitter_ready ? 1u : 0u;
  restir_constants.local_normal_bias = di_clamped(di::g_local_normal_bias, 0.02f, 0.f, 0.5f);
  restir_constants.local_char_normal_bias = di_clamped(di::g_local_character_normal_bias, 0.02f, 0.f, 0.5f);
  restir_constants.local_clearance = di_clamped(di::g_local_light_clearance, 0.2f, 0.f, 1.f);
  restir_constants.sun_normal_bias = di_clamped(di::g_sun_normal_bias, 0.02f, 0.f, 0.5f);
  restir_constants.sun_char_normal_bias = di_clamped(di::g_sun_character_normal_bias, 0.02f, 0.f, 0.5f);
  restir_constants.sun_max_distance = di_clamped(di::g_sun_max_ray_distance, 100.f, 1.f, 10000.f);
  restir_constants.sun_casters_only = di::g_sun_casters_only > 0.5f ? 1u : 0u;
  restir_constants.local_clamp = di_clamped(di::g_local_light_clamp, 16.f, 0.f, 1000.f);
  restir_constants.local_clamp_enabled = di::g_local_light_clamp_enabled > 0.5f ? 1u : 0u;
  restir_constants.char_lamp_intensity = di_clamped(di::g_local_character_intensity, 1.f, 0.f, 10.f);
  restir_constants.local_char_self_softening = di_clamped(di::g_local_character_self_softening, 0.f, 0.f, 10.f);
  restir_constants.sun_char_self_softening = di_clamped(di::g_sun_character_self_softening, 0.f, 0.f, 10.f);
  restir_constants.foliage_transmission = di_clamped(di::g_sun_foliage_transmission, 0.f, 0.f, 1.f);
  restir_constants.wind_transmission = di_clamped(di::g_sun_wind_transmission, 0.5f, 0.f, 1.f);
  restir_constants.wind_softening = di_clamped(di::g_sun_wind_softening, 0.5f, 0.f, 10.f);
  restir_constants.foliage_tint[0] = di_clamped(di::g_sun_foliage_tint_r, 1.f, 0.f, 1.f);
  restir_constants.foliage_tint[1] = di_clamped(di::g_sun_foliage_tint_g, 1.f, 0.f, 1.f);
  restir_constants.foliage_tint[2] = di_clamped(di::g_sun_foliage_tint_b, 1.f, 0.f, 1.f);
  restir_constants.sun_darkening = di_clamped(di::g_sun_shadow_darkening, 1.f, 0.f, 1.f);
  restir_constants.sun_char_darkening = di_clamped(di::g_sun_character_shadow_darkening, 1.f, 0.f, 1.f);
  restir_constants.pt_intensity = di_clamped(pt::g_pt_intensity, 1.f, 0.f, 2.f);
  restir_constants.pt_char_intensity = di_clamped(pt::g_pt_char_intensity, 1.f, 0.f, 1.f);
  restir_constants.direct_enabled = pt::g_restir_direct > 0.5f ? 1u : 0u;
  restir_constants.direct_clamp = pt::g_restir_direct_clamp_enabled > 0.5f
      ? di_clamped(pt::g_restir_direct_clamp, 16.f, 0.5f, 64.f) : 0.f;
  restir_constants.lamp_toon = di_clamped(pt::g_pt_lamp_toon, 1.f, 0.f, 1.f);
  restir_constants.lamp_wrap_scale = di_clamped(pt::g_pt_lamp_wrap_scale, 1.f, 0.f, 2.f);
  restir_constants.lamp_intensity = di_clamped(pt::g_pt_lamp_intensity, 1.f, 0.f, 4.f);
  restir_constants.lamp_material_scale = pt::g_pt_lamp_material_scale > 0.5f ? 1u : 0u;
  restir_constants.lamp_bands = static_cast<uint32_t>(di_clamped(pt::g_pt_lamp_bands, 0.f, 0.f, 8.f) + 0.5f);
  restir_constants.sun_toon = di_clamped(pt::g_pt_sun_toon, 1.f, 0.f, 1.f);
  restir_constants.char_sun_toon = di_clamped(pt::g_pt_char_sun_toon, 1.f, 0.f, 1.f);
  restir_constants.char_lamp_toon = di_clamped(pt::g_pt_char_lamp_toon, 1.f, 0.f, 1.f);
  restir_constants.sun_intensity = di_clamped(pt::g_pt_sun_intensity, 1.f, 0.f, 4.f);
  restir_constants.game_sun_shadows = game_sun_shadows ? 1u : 0u;
  restir_constants.char_interior_sun_gate = pt::g_restir_char_interior_sun_gate > 0.5f ? 1u : 0u;
  restir_constants.gi_clamp_per_bounce =
      (pt::g_pt_gi_clamp_mode > 0.5f && pt::g_pt_gi_clamp_enabled > 0.5f) ? 1u : 0u;
  restir_constants.sharc_min_bounce = std::clamp(static_cast<uint32_t>(pt::g_pt_sharc_min_bounce), 1u, 3u);
  restir_constants.resampling_none = resampling_mode == 0u ? 1u : 0u;
  restir_constants.tile_count = lamps ? input.tile_count : 0u;
  restir_constants.debug_view = debug_view;
  restir_constants.sun_detail_view = sun_detail_view;
  restir_constants.debug_channel = PT_DEBUG_CHANNEL_UCW;
  if (debug_view == PT_DEBUG_VIEW_RESTIR) {
    restir_constants.debug_channel = static_cast<uint32_t>(
        di_clamped(pt::g_pt_debug_channel, 0.f, 0.f, static_cast<float>(PT_DEBUG_CHANNEL_INITIAL_VS_REUSED)));
  } else if (debug_view == PT_DEBUG_VIEW_MATERIAL) {
    restir_constants.debug_channel = static_cast<uint32_t>(di_clamped(di::g_pt_material_channel, 3.f, 0.f, 4.f));
  }
  restir_constants.coverage_min[0] = s.scene_coverage_min[0];
  restir_constants.coverage_min[1] = s.scene_coverage_min[1];
  restir_constants.coverage_min[2] = s.scene_coverage_min[2];
  restir_constants.coverage_size = s.scene_coverage_size;
  restir_constants.coverage_valid = s.scene_coverage_valid ? 1u : 0u;
  r.debug_view[k] = debug_view;
  r.sun_detail_view[k] = sun_detail_view;
  if (debug_on) r.debug_frame[k] = frame;
  void* constants_data = nullptr;
  if (FAILED(r.constants[k]->Map(0u, nullptr, &constants_data))) return fail("pt constants map", E_FAIL);
  std::memcpy(constants_data, &restir_constants, sizeof(restir_constants));
  r.constants[k]->Unmap(0u, nullptr);
  ID3D12DescriptorHeap* heaps[] = {r.heap.Get()};
  list->SetDescriptorHeaps(1u, heaps);
  list->SetComputeRootSignature(r.root.Get());
  list->SetComputeRoot32BitConstants(0u, kRt12PtConstants, params, 0u);
  list->SetComputeRootConstantBufferView(2u, r.constants[k]->GetGPUVirtualAddress());
  list->SetComputeRootDescriptorTable(1u, table);
  list->SetPipelineState(r.initial_pso.Get());
  if (mat_on) {
    const auto material_barrier = [&]() {
      barriers.clear();
      for (ID3D12Resource* resource : material_buffers) UavBarrier(barriers, resource);
      list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
    };
    list->SetPipelineState(r.material_write_pso.Get());
    if (r.material_clear) {
      params[35] = 1u;
      list->SetComputeRoot32BitConstants(0u, kRt12PtConstants, params, 0u);
      list->Dispatch((r.material_capacity + 127u) / 128u, 1u, 1u);
      material_barrier();
      params[35] = 0u;
      list->SetComputeRoot32BitConstants(0u, kRt12PtConstants, params, 0u);
      r.material_clear = false;
      std::lock_guard lock(pt::g_pt_panel_mutex);
      ++pt::g_pt_panel.mat_clears;
      pt::g_pt_panel.mat_clear_reason = mat_clear_reason ? mat_clear_reason : "allocated";
    }
    // A22 M1: every 32 frames each unrefreshed entry ages one step and is evicted at 15 (about 480 frames off-screen).
    if (frame % 32u == 0u) {
      params[35] = 2u;
      list->SetComputeRoot32BitConstants(0u, kRt12PtConstants, params, 0u);
      list->Dispatch((r.material_capacity + 127u) / 128u, 1u, 1u);
      material_barrier();
      params[35] = 0u;
      list->SetComputeRoot32BitConstants(0u, kRt12PtConstants, params, 0u);
    }
    // A22.1: one thread per pixel, so missing cells are written the frame they appear.
    list->Dispatch((w * h + 127u) / 128u, 1u, 1u);
    material_barrier();
    list->SetPipelineState(r.initial_pso.Get());
  }
  if (sharc_on) {
    const auto uav_barrier = [&]() {
      barriers.clear();
      for (ID3D12Resource* resource : sharc_buffers) UavBarrier(barriers, resource);
      list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
    };
    const UINT entry_groups = (r.sharc_capacity + 127u) / 128u;
    list->SetPipelineState(r.sharc_resolve_pso.Get());
    if (r.sharc_clear) {
      params[27] = 1u;
      list->SetComputeRoot32BitConstants(0u, kRt12PtConstants, params, 0u);
      list->Dispatch(entry_groups, 1u, 1u);
      uav_barrier();
      params[27] = 0u;
      list->SetComputeRoot32BitConstants(0u, kRt12PtConstants, params, 0u);
      r.sharc_clear = false;
      std::lock_guard lock(pt::g_pt_panel_mutex);
      ++pt::g_pt_panel.sharc_clears;
      pt::g_pt_panel.sharc_clear_reason = clear_reason ? clear_reason : "allocated";
    }
    list->SetPipelineState(r.sharc_update_pso.Get());
    list->Dispatch(((w + sharc_ds - 1u) / sharc_ds + 7u) / 8u, ((h + sharc_ds - 1u) / sharc_ds + 7u) / 8u, 1u);
    uav_barrier();
    list->SetPipelineState(r.sharc_resolve_pso.Get());
    list->Dispatch(entry_groups, 1u, 1u);
    uav_barrier();
    list->SetPipelineState(r.initial_pso.Get());
  }
  if (pt_perf_timing) list->EndQuery(r.perf_queries.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 2u * k);
  if (pt_perf_timing) list->EndQuery(r.perf_queries.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 2u * kRt12PtPerfBlocks + 8u * k + 6u);
  list->Dispatch((w + 7u) / 8u, (h + 7u) / 8u, 1u);
  barriers.clear();
  UavBarrier(barriers, r.reservoirs.Get());
  UavBarrier(barriers, initial_color.texture12.Get());
  list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
  const bool has_temporal = resampling_mode == 1u || resampling_mode == 3u;
  const bool has_spatial = resampling_mode == 2u || resampling_mode == 3u;
  if (spatial.enableSpatialHeuristicMode && has_spatial) {
    barriers.clear();
    UavBarrier(barriers, r.neighbor_selection_gbuffer.Get());
    list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
    list->SetPipelineState(r.neighbor_selection_pso.Get());
    list->Dispatch((w + 7u) / 8u, (h + 7u) / 8u, 1u);
    barriers.clear();
    UavBarrier(barriers, r.neighbor_selection_gbuffer.Get());
    UavBarrier(barriers, r.neighbor_selection.Get());
    list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
  }
  if (has_temporal) {
    list->SetPipelineState(r.temporal_pso.Get());
    list->Dispatch((w + 7u) / 8u, (h + 7u) / 8u, 1u);
    barriers.clear();
    UavBarrier(barriers, r.reservoirs.Get());
    list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
  }
  if (has_spatial) {
    list->SetPipelineState(r.spatial_pso.Get());
    list->Dispatch((w + 7u) / 8u, (h + 7u) / 8u, 1u);
    barriers.clear();
    UavBarrier(barriers, r.reservoirs.Get());
    list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
  }
  const bool needs_smoothed = decorrelation.decorrelationMode == RTXDI_PTDecorrelationMode::Stagnancy
                              && decorrelation.decorrelationFactor > 0.f && has_temporal;
  if (::rtxdi::NeedsDuplicationMap(temporal, decorrelation) && has_temporal) {
    barriers.clear();
    UavBarrier(barriers, r.reservoirs.Get());
    UavBarrier(barriers, r.sample_id.Get());
    UavBarrier(barriers, r.duplication_map.Get());
    UavBarrier(barriers, r.smoothed_duplication[frame & 1u].Get());
    list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
    list->SetPipelineState(r.duplication_pso.Get());
    list->Dispatch((w + 15u) / 16u, (h + 15u) / 16u, 1u);
    if (needs_smoothed) {
      barriers.clear();
      UavBarrier(barriers, r.duplication_map.Get());
      list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
      list->SetPipelineState(r.smoothed_duplication_pso.Get());
      list->Dispatch((w + 15u) / 16u, (h + 15u) / 16u, 1u);
    }
    barriers.clear();
    UavBarrier(barriers, r.smoothed_duplication[frame & 1u].Get());
    list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
  }
  list->SetPipelineState(r.final_pso.Get());
  list->Dispatch((w + 7u) / 8u, (h + 7u) / 8u, 1u);
  if (pt_perf_timing) {
    list->EndQuery(r.perf_queries.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 2u * k + 1u);
    list->EndQuery(r.perf_queries.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 2u * kRt12PtPerfBlocks + 8u * k + 7u);
    r.perf_valid_mask[k] |= 8u;
    list->ResolveQueryData(r.perf_queries.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 2u * k, 2u, r.perf_readback.Get(), 16ull * k);
    for (uint32_t interval = 0u; interval < 4u; ++interval) {
      if ((r.perf_valid_mask[k] & (1u << interval)) == 0u) continue;
      const uint32_t query = 2u * kRt12PtPerfBlocks + 8u * k + interval * 2u;
      list->ResolveQueryData(r.perf_queries.Get(), D3D12_QUERY_TYPE_TIMESTAMP, query, 2u, r.perf_readback.Get(),
                             16ull * kRt12PtPerfBlocks + 64ull * k + 16ull * interval);
    }
  }
  barriers.clear();
  Transition(barriers, out.texture12.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON);
  if (refl_target) Transition(barriers, refl.texture12.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON);
  for (ID3D12Resource* resource : sharc_buffers) {
    if (resource) {
      Transition(barriers, resource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON);
    }
  }
  for (ID3D12Resource* resource : material_buffers) {
    if (resource) Transition(barriers, resource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON);
  }
  Transition(barriers, r.reservoirs.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON);
  for (ID3D12Resource* resource : {r.sample_id.Get(), r.duplication_map.Get(), r.smoothed_duplication[frame & 1u].Get(),
                                   r.neighbor_selection_gbuffer.Get(), r.neighbor_selection.Get()}) {
    Transition(barriers, resource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON);
  }
  Transition(barriers, r.smoothed_duplication[(frame + 1u) & 1u].Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
             D3D12_RESOURCE_STATE_COMMON);
  for (ID3D12Resource* resource : material_reads) {
    if (resource) Transition(barriers, resource, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COMMON);
  }
  for (ID3D12Resource* resource : reads) {
    if (resource) Transition(barriers, resource, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COMMON);
  }
  if (previous_slot) {
    for (ID3D12Resource* resource : {history_slot.depth.texture12.Get(), history_slot.normal.texture12.Get(),
                                     history_slot.scene.texture12.Get(), history_slot.color.texture12.Get(),
                                     history_slot.mrt1.texture12.Get(), history_slot.mrt2.texture12.Get(),
                                     history_slot.deferred_buffer.Get(), history_slot.lights_buffer.Get(),
                                     history_slot.light_table.Get()}) {
      if (resource) Transition(barriers, resource, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COMMON);
    }
  }
  for (ID3D12Resource* resource : {input.motion.texture12.Get(), input.previous_to_current.Get(),
                                   input.current_to_previous.Get()}) {
    if (resource) Transition(barriers, resource, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COMMON);
  }
  if (sparse_effective || light_jitter_ready) {
    Transition(barriers, s.noise.texture12.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COMMON);
  }
  if (regir_on) {
    for (ID3D12Resource* resource : {r.regir_header[regir_slot].Get(), r.regir_cells[regir_slot].Get(), r.regir_weights[regir_slot].Get()}) {
      Transition(barriers, resource, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COMMON);
    }
  }
  Transition(barriers, r.counters[k].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
  list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
  if (!r.counter_pending[k]) {
    list->CopyBufferRegion(r.counter_readback[k].Get(), 0u, r.counters[k].Get(), 0u, kRt12PtCounters * sizeof(uint32_t));
  }
  barriers.clear();
  Transition(barriers, r.counters[k].Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COMMON);
  list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
  if (FAILED(list->Close())) return submit_fail(Rt12Reason::SubmitFailed, "pt close");
  // 7. Submission: wait for A and the previous D3D12 work, execute, signal B.
  bool ordered = false;
  try {
    ordered = renodx::utils::gpu_fence::QueueWait(s.queue12.Get(), s.fence11.get(), a);
    if (ordered && s.last_signal != 0u) {
      ordered = renodx::utils::gpu_fence::QueueWait(s.queue12.Get(), s.fence12.get(), s.last_signal);
    }
  } catch (const std::exception&) {
    ordered = false;
  }
  if (!ordered) return submit_fail(Rt12Reason::SubmitFailed, "pt order wait");
  RecordRt12WaitA(s, a, GpuEvidenceSource::Pt);
  if (s.last_signal != 0u) RecordRt12WaitPriorB(s, s.last_signal, GpuEvidenceSource::Pt);
  ID3D12CommandList* lists[] = {list};
  BeginRt12Submission(s);
  s.unfenced_execution = true;  // stays true if no signal B follows (teardown then leaks instead of freeing in-use objects)
  s.queue12->ExecuteCommandLists(1u, lists);
  bvh::AddPerfBlockMs(bvh::kPerfBlockPtMainCpu, bvh::LiveElapsedMs(pt_main_cpu_start));
  RecordRt12Evidence(s, GpuEvidenceEvent::Rt12Submit, GpuEvidenceSource::Pt);
  RecordGpuWorkBreadcrumb(GpuWorkKind::Rt12, k);
  RecordGpuWorkBreadcrumb(GpuWorkKind::Pt, w);
  uint64_t b = renodx::utils::gpu_fence::NO_SIGNAL;
  try {
    b = renodx::utils::gpu_fence::QueueSignal(s.queue12.Get(), s.fence12.get());
  } catch (const std::exception&) {
    b = renodx::utils::gpu_fence::NO_SIGNAL;
  }
  if (b == renodx::utils::gpu_fence::NO_SIGNAL) {
    s.sync_evidence.signal_b_result = E_FAIL;
    RecordRt12Evidence(s, GpuEvidenceEvent::Rt12SignalB, GpuEvidenceSource::Pt);
    return submit_fail(Rt12Reason::SubmitFailed, "pt signal");
  }
  s.unfenced_execution = false;
  pt_slot.fence_value = b;
  if (pt_perf_timing) {
    r.perf_pending[k] = true;
    r.perf_fence[k] = b;
    r.perf_epoch[k] = pt_perf_epoch;
  }
  if (!r.counter_pending[k]) {
    r.counter_fence[k] = b;
    r.counter_frame[k] = frame;
    r.counter_sparse[k] = sparse_policy.dense_reason == pt::PtSparseDenseReason::None && sparse_policy.levels < 256u;
    r.counter_pending[k] = true;
  }
  scene.fence_value = (std::max)(scene.fence_value, b);
  input.fence_value = (std::max)(input.fence_value, b);
  if (regir_on) {
    r.regir_last_use[regir_slot] = b;
    r.regir_identity[regir_slot] = source_identity;
    if (regir_copy_counts) {
      r.regir_counts_fence[regir_slot] = b;
      r.regir_counts_frame[regir_slot] = frame;
      r.regir_counts_pending[regir_slot] = true;
    }
    input.di_fence_value = (std::max)(input.di_fence_value, b);
  }
  for (size_t i = retired_begin; i < s.retired.size(); ++i) s.retired[i].fence_value = b;
  NoteRt12Signal(s, b);
  RecordRt12Evidence(s, GpuEvidenceEvent::Rt12SignalB, GpuEvidenceSource::Pt);
  r.reservoirs_last_use = b;
  r.history_frame = frame;
  r.history_width = w;
  r.history_height = h;
  r.history_transfer_slot = static_cast<int32_t>(transfer_slot);
  r.history_owner_generation = s.generation;
  r.history_configuration_generation = r.configuration_generation;
  r.history_cache_epoch = r.cache_epoch;
  r.history_sparse_levels = sparse_policy.dense_reason == pt::PtSparseDenseReason::None ? sparse_policy.levels : 256u;
  r.history_sparse_reflections_effective = sparse_reflections_effective;
  r.history_valid = true;
  {
    std::lock_guard lock(pt::g_pt_panel_mutex);
    pt::g_pt_panel.history_valid = history_valid;
    pt::g_pt_panel.history_reason = history_reason;
  }
  r.next = (k + 1u) % kRt12Slots;
  // 8. D3D11 waits for B on the GPU before the lighting draw reads the output.
  bool waited = false;
  try {
    waited = renodx::utils::gpu_fence::QueueWait(s.ctx11.Get(), s.fence12.get(), b);
  } catch (const std::exception&) {
    waited = false;
  }
  if (!waited) return submit_fail(Rt12Reason::SubmitFailed, "pt d3d11 wait on fence B");
  RecordRt12WaitB(s, b, GpuEvidenceSource::Pt);
  const uint32_t sparse_effective_levels = sparse_policy.dense_reason == pt::PtSparseDenseReason::None ? sparse_policy.levels : 256u;
  if (r.sparse_effective != sparse_effective || r.sparse_effective_levels != sparse_effective_levels ||
      r.sparse_reflections_effective != sparse_reflections_effective) {
    if (const auto runtime = rr::SnapshotRrD3D12Runtime(); runtime != nullptr) {
      runtime->MarkHistoryReset();
      r.sparse_effective = sparse_effective;
      r.sparse_effective_levels = sparse_effective_levels;
      r.sparse_reflections_effective = sparse_reflections_effective;
    }
  }
  if (water_on) {  // publish the water block's inputs for RunPtWaterTrace of this frame
    bool water_history_valid = false;
    const char* water_history_reason = "valid";
    const uint32_t water_prev_slot = r.water_history_slot;
    const int32_t water_prev_transfer = r.water_history_transfer_slot;
    if (!r.water_history_valid) water_history_reason = "no_history";
    else if (r.water_history_frame == UINT32_MAX || r.water_history_frame + 1u != frame) water_history_reason = "frame";
    else if (r.water_history_configuration_generation == 0u
             || r.water_history_configuration_generation != r.configuration_generation) water_history_reason = "configuration";
    else if (r.water_history_owner_generation == 0u || r.water_history_owner_generation != s.generation) {
      water_history_reason = "owner";
    } else if (r.water_history_width != w || r.water_history_height != h) water_history_reason = "size";
    else if (water_prev_transfer < 0 || water_prev_transfer >= static_cast<int32_t>(kRt12Slots)
             || water_prev_transfer == static_cast<int32_t>(transfer_slot)) water_history_reason = "transfer_slot";
    else if (r.water_history_cache_epoch != r.cache_epoch) water_history_reason = "cache_epoch";
    else if (water_prev_slot == k) water_history_reason = "water_slot";
    else if (!r.water_depth[water_prev_slot].texture12 || !r.water_normal[water_prev_slot].texture12
             || r.water_depth[water_prev_slot].width != w || r.water_depth[water_prev_slot].height != h
             || r.water_normal[water_prev_slot].width != w || r.water_normal[water_prev_slot].height != h) {
      water_history_reason = "previous_copies";
    } else if (r.water_history_scene_slot >= kRt12Slots
               || !s.di_transfer.slots[r.water_history_scene_slot].scene.texture12) water_history_reason = "previous_scene";
    if (r.water_history_valid && water_prev_transfer >= 0 && water_prev_transfer < static_cast<int32_t>(kRt12Slots)
        && water_prev_slot < kRt12Slots && water_prev_slot != k && r.water_history_frame != UINT32_MAX
        && r.water_history_frame + 1u == frame && r.water_depth[water_prev_slot].texture12
        && r.water_normal[water_prev_slot].texture12 && r.water_history_scene_slot < kRt12Slots
        && s.di_transfer.slots[r.water_history_scene_slot].scene.texture12) {
      Rt12DiTransferSlot& candidate = s.di_transfer.slots[static_cast<uint32_t>(water_prev_transfer)];
      const bool previous_inputs = candidate.width == w && candidate.height == h
          && candidate.depth.texture12 && candidate.normal.texture12 && candidate.scene.texture12
          && candidate.color.texture12 && candidate.mrt1.texture12 && candidate.mrt2.texture12
          && candidate.deferred_buffer && candidate.lights_buffer && candidate.light_table
          && candidate.motion.texture12 && candidate.motion_available;
      if (rtxdi::ValidDiHistoryIdentity({.previous_frame = r.water_history_frame,
                                         .current_frame = frame,
                                         .previous_width = r.water_history_width,
                                         .previous_height = r.water_history_height,
                                         .current_width = w,
                                         .current_height = h,
                                         .previous_transfer_slot = water_prev_transfer,
                                         .current_transfer_slot = static_cast<int32_t>(transfer_slot),
                                         .previous_owner_generation = r.water_history_owner_generation,
                                         .current_owner_generation = s.generation,
                                         .previous_configuration_generation = r.water_history_configuration_generation,
                                         .current_configuration_generation = r.configuration_generation,
                                         .motion_available = true,
                                         .previous_inputs_available = previous_inputs})) {
        water_history_valid = true;
      } else {
        water_history_reason = !previous_inputs ? "missing_inputs" : (!input.motion_available ? "motion" : "identity");
      }
    }
    if (!water_history_valid) {
      r.water_history_frame = UINT32_MAX;
      r.water_history_width = 0u;
      r.water_history_height = 0u;
      r.water_history_transfer_slot = -1;
      r.water_history_owner_generation = 0u;
      r.water_history_configuration_generation = 0u;
      r.water_history_cache_epoch = 0u;
    }
    r.water_history_valid = water_history_valid;
    r.water_restir->SetFrameIndex(frame);
    r.water_restir->SetResamplingMode(static_cast<::rtxdi::ReSTIRPT_ResamplingMode>(resampling_mode));
    r.water_restir->SetInitialSamplingParameters({.numInitialSamples = (std::clamp)(
        static_cast<uint32_t>(pt::g_pt_samples), 1u, 16u),
        .maxBounceDepth = static_cast<uint32_t>(pt::g_pt_max_bounces) + 1u,
        .maxRcVertexLength = static_cast<uint32_t>(std::clamp(pt::g_restir_max_rc_vertex_length, 2.f, 8.f))});
    r.water_restir->SetTemporalResamplingParameters(temporal);
    r.water_restir->SetSpatialResamplingParameters(spatial);
    r.water_restir->SetBoilingFilterParameters(boiling);
    r.water_restir->SetReconnectionParameters(reconnection);
    r.water_restir->SetHybridShiftParameters({.maxBounceDepth = static_cast<uint32_t>(pt::g_pt_max_bounces) + 1u,
                                              .maxRcVertexLength = static_cast<uint32_t>(
                                                  std::clamp(pt::g_restir_max_rc_vertex_length, 2.f, 8.f))});
    r.water_restir->SetDecorrelationParameters(decorrelation);
    void* water_data = nullptr;
    if (SUCCEEDED(r.water_constants[k]->Map(0u, nullptr, &water_data))) {
      PtRestirConstants water_restir = restir_constants;
      water_restir.restir.reservoirBuffer = r.water_restir->GetReservoirBufferParameters();
      water_restir.restir.bufferIndices = r.water_restir->GetBufferIndices();
      water_restir.restir.initialSampling = r.water_restir->GetInitialSamplingParameters();
      water_restir.restir.decorrelation = r.water_restir->GetDecorrelationParameters();
      water_restir.restir.reconnection = r.water_restir->GetReconnectionParameters();
      water_restir.restir.temporalResampling = r.water_restir->GetTemporalResamplingParameters();
      water_restir.restir.hybridShift = r.water_restir->GetHybridShiftParameters();
      water_restir.restir.boilingFilter = r.water_restir->GetBoilingFilterParameters();
      water_restir.restir.spatialResampling = r.water_restir->GetSpatialResamplingParameters();
      water_restir.history_valid = water_history_valid ? 1u : 0u;
      std::memcpy(water_data, &water_restir, sizeof(water_restir));
      r.water_constants[k]->Unmap(0u, nullptr);
    }
    {
      std::lock_guard lock(pt::g_pt_panel_mutex);
      pt::g_pt_panel.water_history_valid = water_history_valid;
      pt::g_pt_panel.water_history_reason = water_history_reason;
    }
    std::copy_n(params, kRt12PtConstants, r.water_params);
    r.water_params[46] = 0u;
    r.water_params[47] = 0u;
    r.water_main_slot = k;
    r.water_input_slot = transfer_slot;
    r.water_scene_slot = s.scene_slot;
    r.water_frame = frame;
  }
  // 9. Panel.
  pt::SetPtReason(pt::PtReason::Tracing);
  {
    std::lock_guard lock(pt::g_pt_panel_mutex);
    pt::g_pt_panel.lamps = lamps_on ? input.light_count : 0u;
    pt::g_pt_panel.resampling_mode = resampling_mode;
    pt::g_pt_panel.direct_enabled = restir_constants.direct_enabled != 0u;
    pt::g_pt_panel.light_table_status = light_table_status;
    pt::g_pt_panel.debug_view = debug_view;
    pt::g_pt_panel.debug_channel = restir_constants.debug_channel;
    pt::g_pt_panel.width = w;
    pt::g_pt_panel.height = h;
    pt::g_pt_panel.sparse_policy = sparse_policy;
    pt::g_pt_panel.sparse_requested = sparse_requested;
    pt::g_pt_panel.sparse_reflections_requested = sparse_reflections_requested;
    pt::g_pt_panel.sparse_reflections_effective = sparse_reflections_effective;
    pt::g_pt_panel.sparse_adaptive = pt::g_pt_sparse_adaptive > 0.5f;
    pt::g_pt_panel.sharc_on = sharc_on;
    pt::g_pt_panel.sharc_capacity_log2 = sharc_on ? static_cast<uint32_t>(std::countr_zero(r.sharc_capacity)) : 0u;
    pt::g_pt_panel.sharc_downscale = sharc_on ? sharc_ds : 0u;
    pt::g_pt_panel.sharc_window = sharc_window;
    pt::g_pt_panel.mat_on = mat_on;
    pt::g_pt_panel.mat_capacity_log2 = mat_on ? static_cast<uint32_t>(std::countr_zero(r.material_capacity)) : 0u;
    pt::g_pt_panel.mat_rate = mat_on ? mat_rate : 0u;
    pt::g_pt_panel.refl_on = refl_on;
    pt::g_pt_panel.regir_requested = regir_requested;
    pt::g_pt_panel.regir_active = regir_on;
    pt::g_pt_panel.regir_fallback_reason = !regir_requested ? "off" : !lamps_on ? "no_lamps"
        : !regir_identity_matches ? "source_identity" : !regir_slot_available ? "slot_busy"
        : !regir_resources_available ? "resources" : "none";
    pt::g_pt_panel.regir_owner_generation = source_identity.owner_generation;
    pt::g_pt_panel.regir_transfer_configuration = source_identity.transfer_configuration;
    pt::g_pt_panel.regir_frame = source_identity.frame;
    pt::g_pt_panel.regir_transfer_slot = source_identity.transfer_slot;
    pt::g_pt_panel.regir_light_count = source_identity.light_count;
    pt::g_pt_panel.regir_source_fence = source_identity.table_fence;
    pt::g_pt_panel.regir_build_fence = regir_on ? b : 0u;
    pt::g_pt_panel.regir_last_use_fence = r.regir_last_use[regir_slot];
  }
  *reflection_view = refl_target ? refl.srv11 : reshade::api::resource_view{0u};
  *sun_visibility_view = game_sun_shadows ? sun_vis.srv11 : reshade::api::resource_view{0u};
  debug_views[0] = debug_on && r.debug_frame[k] == frame ? r.debug_value[k].srv11 : reshade::api::resource_view{0u};
  debug_views[1] = debug_on && r.debug_frame[k] == frame ? r.debug_status[k].srv11 : reshade::api::resource_view{0u};
  debug_views[2] = debug_on && r.debug_frame[k] == frame ? r.sun_detail[k].srv11 : reshade::api::resource_view{0u};
  return out.srv11;
}
// Water panel state when no water reflection is traced this draw: off, the reason, and no water counters.
inline void SetPtWaterOff(const char* reason) {
  std::lock_guard lock(pt::g_pt_panel_mutex);
  pt::g_pt_panel.water_on = false;
  pt::g_pt_panel.water_reason = reason;
  pt::g_pt_panel.gpu_water_pixels = 0u;
  pt::g_pt_panel.gpu_water_traces = 0u;
  pt::g_pt_panel.regir_water_counts_available = false;
  pt::g_pt_panel.regir_water_counts_frame = 0u;
  pt::g_pt_panel.regir_water_counts_owner_generation = 0u;
  pt::g_pt_panel.regir_water_submission_frame = UINT32_MAX;
  pt::g_pt_panel.regir_water_submission_owner_generation = 0u;
}
// Water reflection from the SSR surface (M4): traces the water pixels of the SSR draw, from copies of its depth and mrt0,
// into water_reflection[k] (read by ssr2). Uses the water block that the main trace of this frame published.
inline reshade::api::resource_view RunPtWaterTrace(reshade::api::device* device, reshade::api::command_list* cmd_list,
                                                   const reshade::api::resource_view& depth_srv,
                                                   const reshade::api::resource_view& mrt_srv) {
  using Microsoft::WRL::ComPtr;
  const auto fallback = [](const char* reason) {
    SetPtWaterOff(reason);
    return reshade::api::resource_view{0u};
  };
  // 1. Gates.
  if (!g_rt12 || g_rt12->device != device) return fallback("gate");
  Rt12State& s = *g_rt12;
  if (s.unfenced_execution || s.stuck || s.failed) return fallback("gate");
  if (reinterpret_cast<ID3D11DeviceContext*>(cmd_list->get_native()) != s.ctx11.Get()) {  // NOLINT(performance-no-int-to-ptr)
    return fallback("gate");
  }
  Rt12PtState& r = s.path_trace;
  const uint32_t frame = g_state.frame.load();
  if (!r.created || r.water_frame != frame) return fallback("gate");
  r.water_frame = UINT32_MAX;  // one water trace per published main trace
  const uint32_t k = r.water_main_slot;
  const uint32_t w = r.water_params[0];
  const uint32_t h = r.water_params[1];
  Rt12Slot& water_slot = r.water_slots[k];
  // The input and scene slots the main trace captured must still be the frame's current slots.
  if (s.di_transfer.input_frame != frame || s.di_transfer.input_slot != r.water_input_slot || s.scene_frame != frame
      || s.scene_slot != r.water_scene_slot) {
    return fallback("stale_slot");
  }
  Rt12DiTransferSlot& input = s.di_transfer.slots[static_cast<uint32_t>(r.water_input_slot)];
  Rt12Slot& scene = s.lighting_slots[static_cast<uint32_t>(r.water_scene_slot)];
  const uint32_t regir_slot = static_cast<uint32_t>(r.water_input_slot);
  const pt::PtReGirIdentity& regir_identity = r.regir_identity[regir_slot];
  const pt::PtReGirIdentity& source_identity = input.regir_source_identity;
  if (regir_identity.owner_generation != source_identity.owner_generation
      || regir_identity.transfer_configuration != source_identity.transfer_configuration
      || regir_identity.table_fence == 0u || regir_identity.table_fence != source_identity.table_fence
      || regir_identity.frame != source_identity.frame || regir_identity.transfer_slot != source_identity.transfer_slot
      || regir_identity.light_count != source_identity.light_count || source_identity.owner_generation != s.generation
      || source_identity.transfer_configuration != s.di_transfer.configuration_generation || source_identity.frame != frame) {
    r.water_params[48] = 0u;
  }
  const Rt12PtWaterFrame& in = r.water_frame_inputs;
  // Water counters: a completed readback is published once, in frame order; never waits.
  const uint64_t completed = Rt12CompletedValue(s);
  for (uint32_t i = 0u; i < kRt12Slots; ++i) {
    if (!r.water_counter_pending[i] || r.water_counter_fence[i] > completed) continue;
    void* mapped = nullptr;
    if (FAILED(r.water_counter_readback[i]->Map(0u, nullptr, &mapped))) {
      std::lock_guard lock(pt::g_pt_panel_mutex);
      pt::g_pt_panel.regir_water_counts_available = false;
      pt::g_pt_panel.regir_water_counts_frame = 0u;
      pt::g_pt_panel.regir_water_counts_owner_generation = 0u;
      continue;
    }
    if (mapped != nullptr) {
      const auto* values = static_cast<const uint32_t*>(mapped);
      {
        std::lock_guard lock(pt::g_pt_panel_mutex);
        const uint32_t readback_frame = pt::g_pt_panel.regir_water_pending_frame[i];
        const uint64_t owner_generation = pt::g_pt_panel.regir_water_pending_owner_generation[i];
        const uint32_t owner = pt::g_pt_panel.regir_water_pending_owner[i];
        const bool submission_matches = pt::g_pt_panel.regir_water_submission_frame == readback_frame
                                        && pt::g_pt_panel.regir_water_submission_owner_generation == owner_generation
                                        && owner == r.water_main_slot;
        const bool identity_matches = owner_generation != 0u && pt::g_pt_panel.regir_active
                                      && pt::g_pt_panel.regir_owner_generation == owner_generation;
        if (submission_matches && identity_matches && readback_frame == r.water_counter_frame[i]
            && static_cast<int32_t>(readback_frame - r.water_counter_published_frame) >= 0) {
          pt::g_pt_panel.gpu_water_pixels = values[5];
          pt::g_pt_panel.gpu_water_traces = values[6];
          pt::g_pt_panel.water_temporal_selected = values[28];
          pt::g_pt_panel.water_spatial_selected = values[29];
          pt::g_pt_panel.regir_water_counts_available = true;
          pt::g_pt_panel.regir_water_counts_frame = readback_frame;
          pt::g_pt_panel.regir_water_counts_owner_generation = owner_generation;
          pt::g_pt_panel.regir_water_eligible = values[9];
          pt::g_pt_panel.regir_water_lookups = values[10];
          pt::g_pt_panel.regir_water_matches = values[11];
          pt::g_pt_panel.regir_water_misses = values[12];
          pt::g_pt_panel.regir_water_invalid_headers = values[13];
          pt::g_pt_panel.regir_water_invalid_slots = values[14];
          pt::g_pt_panel.regir_water_selections = values[15];
          pt::g_pt_panel.regir_water_fallback_disabled = values[16];
          pt::g_pt_panel.regir_water_fallback_key = values[17];
          pt::g_pt_panel.regir_water_fallback_miss = values[18];
          pt::g_pt_panel.regir_water_fallback_header = values[19];
          pt::g_pt_panel.regir_water_fallback_slots = values[20];
          r.water_counter_published_frame = r.water_counter_frame[i];
        }
      }
      r.water_counter_pending[i] = false;
    }
    r.water_counter_readback[i]->Unmap(0u, nullptr);
  }
  // 2. Validation: the SSR depth and mrt0 must be same-size single-sample textures in the formats the water block reads.
  if (depth_srv.handle == 0u) return fallback("no_depth_capture");
  if (mrt_srv.handle == 0u) return fallback("no_mrt_capture");
  const reshade::api::resource depth_res = device->get_resource_from_view(depth_srv);
  const reshade::api::resource mrt_res = device->get_resource_from_view(mrt_srv);
  const reshade::api::resource_desc depth_desc = device->get_resource_desc(depth_res);
  const reshade::api::resource_desc mrt_desc = device->get_resource_desc(mrt_res);
  const DXGI_FORMAT depth_format = static_cast<DXGI_FORMAT>(depth_desc.texture.format);
  const DXGI_FORMAT mrt_format = static_cast<DXGI_FORMAT>(mrt_desc.texture.format);
  const DXGI_FORMAT depth_view_format = static_cast<DXGI_FORMAT>(device->get_resource_view_desc(depth_srv).format);
  const DXGI_FORMAT mrt_view_format = static_cast<DXGI_FORMAT>(device->get_resource_view_desc(mrt_srv).format);
  const bool depth_ok = (depth_format == DXGI_FORMAT_R32_FLOAT || depth_format == DXGI_FORMAT_R32_TYPELESS)
                        && depth_view_format == DXGI_FORMAT_R32_FLOAT;
  const bool mrt_ok = ((mrt_format == DXGI_FORMAT_R32G32B32A32_UINT || mrt_format == DXGI_FORMAT_R32G32B32A32_TYPELESS)
                       && mrt_view_format == DXGI_FORMAT_R32G32B32A32_UINT)
                      || (mrt_format == DXGI_FORMAT_R16G16B16A16_UINT && mrt_view_format == DXGI_FORMAT_R16G16B16A16_UINT);
  // Each rejection names its check; with debug logging on, the event ring keeps the formats and sizes of both sources.
  const auto reject = [&](const char* reason) {
    const uint64_t capture_state = pt::g_pt_capture_state.load();
    if ((capture_state & 1u) != 0u) {
      // Short fields, water size first so it survives the 160-character event limit: "<check> <w>x<h> d <fmt>/<view> t<type> <W>x<H> s<samples> m ...".
      pt::RecordPtEvent(g_state.frame.load(std::memory_order_relaxed),
                        std::string("water ") + reason + " " + std::to_string(w) + "x" + std::to_string(h) + " d "
                            + std::to_string(static_cast<uint32_t>(depth_format)) + "/"
                            + std::to_string(static_cast<uint32_t>(depth_view_format)) + " t"
                            + std::to_string(static_cast<uint32_t>(depth_desc.type)) + " " + std::to_string(depth_desc.texture.width) + "x"
                            + std::to_string(depth_desc.texture.height) + " s" + std::to_string(depth_desc.texture.samples) + " m "
                            + std::to_string(static_cast<uint32_t>(mrt_format)) + "/" + std::to_string(static_cast<uint32_t>(mrt_view_format))
                            + " t" + std::to_string(static_cast<uint32_t>(mrt_desc.type)) + " " + std::to_string(mrt_desc.texture.width) + "x"
                            + std::to_string(mrt_desc.texture.height) + " s" + std::to_string(mrt_desc.texture.samples),
                        capture_state);
    }
    return fallback(reason);
  };
  if (!depth_ok) return reject("depth_format");
  if (!mrt_ok) return reject("mrt_format");
  if (depth_desc.type != reshade::api::resource_type::texture_2d || mrt_desc.type != reshade::api::resource_type::texture_2d) {
    return reject("type");
  }
  if (depth_desc.texture.width != w || depth_desc.texture.height != h || mrt_desc.texture.width != w || mrt_desc.texture.height != h) {
    return reject("size");
  }
  if (depth_desc.texture.samples != 1u || mrt_desc.texture.samples != 1u) return reject("samples");
  const auto fail_water = [&s, &fallback](Rt12Reason reason, const char* stage, const char* water_reason) {
    FailSession(s, reason, stage);
    return fallback(water_reason);
  };
  // 3. Slot wait: this water slot's previous submission must be complete (a timeout is stuck).
  if (water_slot.fence_value > Rt12CompletedValue(s) && !WaitRt12Fence(s, water_slot.fence_value, kRt12StuckMs)) {
    s.stuck = true;
    pt::PtLogWarn("[world-rt12] pt water stuck: fence at " + std::to_string(Rt12CompletedValue(s)) + ", slot submitted "
                  + std::to_string(water_slot.fence_value));
    DisableMirror();
    return fallback("stuck");
  }
  if (!r.water_reservoirs || !r.water_restir) return fallback("resources");
  // 4. Copy targets and water reflection at the SSR draw's size (a replaced texture is retired with the last signal).
  if (r.water_depth_format != depth_format) {
    for (Rt12Shared& shared : r.water_depth) RetireShared(s, shared, s.last_signal);
    r.water_depth_format = depth_format;
  }
  if (r.water_normal_format != mrt_format) {
    for (Rt12Shared& shared : r.water_normal) RetireShared(s, shared, s.last_signal);
    r.water_normal_format = mrt_format;
  }
  Rt12Shared& depth_copy = r.water_depth[k];
  if (!depth_copy.texture11 || depth_copy.width != w || depth_copy.height != h) {
    RetireShared(s, depth_copy, s.last_signal);
    if (FAILED(CreateRt12Shared(s, device, depth_format, w, h, 1u, false, &depth_copy, depth_view_format))) {
      RetireShared(s, depth_copy, s.last_signal);
      return fallback("copy");
    }
  }
  Rt12Shared& normal_copy = r.water_normal[k];
  if (!normal_copy.texture11 || normal_copy.width != w || normal_copy.height != h) {
    RetireShared(s, normal_copy, s.last_signal);
    if (FAILED(CreateRt12Shared(s, device, mrt_format, w, h, 1u, false, &normal_copy, mrt_view_format))) {
      RetireShared(s, normal_copy, s.last_signal);
      return fallback("copy");
    }
  }
  Rt12Shared& refl = r.water_reflection[k];
  if (!refl.texture11 || refl.width != w || refl.height != h) {
    RetireShared(s, refl, s.last_signal);
    if (FAILED(CreateRt12Shared(s, device, DXGI_FORMAT_R16G16B16A16_FLOAT, w, h, 1u, true, &refl))) {
      RetireShared(s, refl, s.last_signal);
      return fallback("copy");
    }
  }
  // 5. Copy the SSR draw's depth and mrt0 (D3D11), then signal A, flushed. Single-mip, single-slice sources only.
  if (depth_desc.texture.levels != 1u || depth_desc.texture.depth_or_layers != 1u || mrt_desc.texture.levels != 1u
      || mrt_desc.texture.depth_or_layers != 1u) {
    return fallback("copy");
  }
  s.ctx11->CopyResource(depth_copy.texture11.Get(), reinterpret_cast<ID3D11Resource*>(depth_res.handle));  // NOLINT(performance-no-int-to-ptr)
  s.ctx11->CopyResource(normal_copy.texture11.Get(), reinterpret_cast<ID3D11Resource*>(mrt_res.handle));  // NOLINT(performance-no-int-to-ptr)
  uint64_t a = renodx::utils::gpu_fence::NO_SIGNAL;
  try {
    a = renodx::utils::gpu_fence::QueueSignal(s.ctx11.Get(), s.fence11.get());
    s.ctx11->Flush();
  } catch (const std::exception&) {
    a = renodx::utils::gpu_fence::NO_SIGNAL;
  }
  RecordRt12SignalA(s, a, GpuEvidenceSource::Pt);
  if (a == renodx::utils::gpu_fence::NO_SIGNAL) return fail_water(Rt12Reason::FenceError, "pt water fence11 signal", "submit");
  const auto water_cpu_start = bvh::PerfNow();  // M0t: D3D12 record and submit only (no D3D11 copies, flush or waits)
  // 6. Record: descriptors, barriers, counters, dispatch.
  ID3D12GraphicsCommandList4* list = r.water_list.Get();
  if (FAILED(water_slot.allocator->Reset()) || FAILED(list->Reset(water_slot.allocator.Get(), nullptr))) {
    return fail_water(Rt12Reason::SubmitFailed, "pt water reset", "record");
  }
  const uint32_t water_block = kRt12Slots + k;
  const bool water_perf_gather = bvh::g_perf_gate.gather.load(std::memory_order_relaxed);
  const uint64_t water_perf_epoch = bvh::g_perf_gate.gather_epoch.load(std::memory_order_relaxed);
  const bool water_perf_timing = rtxdi::CanStartGpuTiming(water_perf_gather, s.timestamp_frequency, r.perf_pending[water_block])
                                 && r.perf_queries && r.perf_readback;
  if (water_perf_timing) r.perf_valid_mask[water_block] = 0u;
  if (water_perf_gather && s.timestamp_frequency == 0u) {
    rtxdi::RecordGpuTimingSkipEvent(&bvh::g_perf_gate.pt_timing_skip_events);
  } else if (water_perf_gather && r.perf_pending[water_block]) {
    bvh::g_perf_gate.pt_timing_pending_frames.fetch_add(1u, std::memory_order_relaxed);
  } else if (water_perf_gather && !water_perf_timing) {
    rtxdi::RecordGpuTimingSkipEvent(&bvh::g_perf_gate.pt_timing_skip_events);  // query heap or readback not allocated
  }
  ID3D12Device5* device12 = s.device12.Get();
  const uint32_t base = (kRt12Slots + k) * kRt12PtDescriptors;
  const auto cpu_at = [&r, &s](uint32_t index) {
    D3D12_CPU_DESCRIPTOR_HANDLE handle = r.heap->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += static_cast<SIZE_T>(index) * s.descriptor_size;
    return handle;
  };
  // Water block (R3): every descriptor is written here, after this slot's fence. Null: 14, 16 and 54 (main output, SHARC
  // accumulation and initial colour, written only by the main trace). 15, 17, 22 and 23 are the cache reads (SHARC hash and
  // resolved, material keys and values); the water dispatch does not write them, since the material write and SHARC update
  // passes never run here.
  const uint32_t water_prev_slot = r.water_history_slot;
  const bool water_history = r.water_history_valid;
  for (uint32_t i = 0u; i < kRt12PtUsedDescriptors; ++i) {
    const D3D12_CPU_DESCRIPTOR_HANDLE handle = cpu_at(base + i);
    const bool uav_index = Rt12PtDescriptorIsUav(i);
    if (!uav_index) {
      ID3D12Resource* resource = in.resources[i].Get();
      D3D12_SHADER_RESOURCE_VIEW_DESC view = in.srv_descs[i];
      if (i == 26u || i == 40u) {
        resource = nullptr;  // sparse noise and RTV4 motion are not inputs to the water passes
      }
      if (i == 9u) {
        resource = depth_copy.texture12.Get();
        view = Rt12TextureSrv(depth_view_format);
      }
      if (i == 10u) {
        resource = normal_copy.texture12.Get();
        view = Rt12TextureSrv(mrt_view_format);
      }
      if (water_history && i == 31u) {
        resource = r.water_depth[water_prev_slot].texture12.Get();
        view = Rt12TextureSrv(depth_view_format);
      }
      if (water_history && i == 32u) {
        resource = r.water_normal[water_prev_slot].texture12.Get();
        view = Rt12TextureSrv(mrt_view_format);
      }
      if (water_history && i == 33u) {
        resource = s.di_transfer.slots[r.water_history_scene_slot].scene.texture12.Get();
        view = Rt12TextureSrv(DXGI_FORMAT_R32G32B32A32_UINT);
      }
      if (i == 48u) {
        resource = r.water_smoothed_duplication[(frame + 1u) & 1u].Get();
        view = Rt12TextureSrv(DXGI_FORMAT_R32_FLOAT);
      }
      if (view.ViewDimension == D3D12_SRV_DIMENSION_UNKNOWN) {
        FailSession(s, Rt12Reason::CreateFailed, "pt water descriptor", static_cast<HRESULT>(0x80000000u | i));
        return fallback("resources");
      }
      device12->CreateShaderResourceView(resource, &view, handle);
    } else {
      ID3D12Resource* resource = nullptr;
      D3D12_UNORDERED_ACCESS_VIEW_DESC view = in.uav_descs[i];
      if (i == 15u || i == 17u || i == 22u || i == 23u) resource = in.resources[i].Get();
      if (i == 24u) resource = r.water_counters[k].Get();
      if (i == 25u) resource = refl.texture12.Get();
      if (i == 29u) {
        resource = r.water_reservoirs.Get();
        view.Buffer.NumElements = r.water_restir->GetReservoirBufferParameters().reservoirArrayPitch
                                  * ::rtxdi::c_NumReSTIRPTReservoirBuffers;
      }
      if (i == 43u) resource = r.water_duplication_map.Get();
      if (i == 44u) resource = r.water_smoothed_duplication[frame & 1u].Get();
      if (i == 45u) resource = r.water_sample_id.Get();
      if (i == 46u) resource = r.water_neighbor_selection_gbuffer.Get();
      if (i == 47u) resource = r.water_neighbor_selection.Get();
      if (i >= 50u && i <= 54u) resource = nullptr;
      if (view.ViewDimension == D3D12_UAV_DIMENSION_UNKNOWN) {
        FailSession(s, Rt12Reason::CreateFailed, "pt water descriptor", static_cast<HRESULT>(0x80000000u | i));
        return fallback("resources");
      }
      device12->CreateUnorderedAccessView(resource, nullptr, &view, handle);
    }
  }
  // Counters: cleared before the dispatch, copied to the readback after it (only when no readback is pending).
  std::vector<D3D12_RESOURCE_BARRIER>& barriers = water_slot.barrier_scratch;
  barriers.clear();
  ID3D12Resource* const water_reads[] = {in.resources[0].Get(), in.resources[1].Get(), in.resources[2].Get(), in.resources[3].Get(),
                                         in.resources[4].Get(), in.resources[5].Get(), in.resources[6].Get(), in.resources[8].Get(),
                                         depth_copy.texture12.Get(), normal_copy.texture12.Get(), in.resources[11].Get(),
                                         in.resources[12].Get(), in.resources[13].Get(), in.resources[18].Get(),
                                         in.resources[19].Get(), in.resources[20].Get(), in.resources[21].Get(),
                                         r.water_params[48] ? in.resources[27].Get() : nullptr,
                                         r.water_params[48] ? in.resources[28].Get() : nullptr,
                                         water_history ? r.water_depth[water_prev_slot].texture12.Get() : nullptr,
                                         water_history ? r.water_normal[water_prev_slot].texture12.Get() : nullptr,
                                         water_history ? s.di_transfer.slots[r.water_history_scene_slot].scene.texture12.Get()
                                                       : nullptr,
                                         in.resources[34].Get(), in.resources[35].Get(), in.resources[36].Get(),
                                         in.resources[37].Get(), in.resources[38].Get(), in.resources[39].Get(),
                                         in.resources[41].Get(), in.resources[42].Get(),
                                         r.water_smoothed_duplication[(frame + 1u) & 1u].Get()};
  for (ID3D12Resource* resource : water_reads) {
    if (resource) Transition(barriers, resource, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
  }
  // Cache reads are UAV-typed descriptors, so they are in UAV state, as in the main trace.
  ID3D12Resource* const water_cache[] = {in.resources[15].Get(), in.resources[17].Get(), in.resources[22].Get(), in.resources[23].Get()};
  for (ID3D12Resource* resource : water_cache) {
    if (resource) Transition(barriers, resource, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
  }
  Transition(barriers, refl.texture12.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
  Transition(barriers, r.water_reservoirs.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
  for (ID3D12Resource* resource : {r.water_sample_id.Get(), r.water_duplication_map.Get(),
                                   r.water_smoothed_duplication[frame & 1u].Get(), r.water_neighbor_selection_gbuffer.Get(),
                                   r.water_neighbor_selection.Get()}) {
    Transition(barriers, resource, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
  }
  Transition(barriers, r.water_counters[k].Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
  list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
  list->CopyBufferRegion(r.water_counters[k].Get(), 0u, r.counter_zero.Get(), 0u, kRt12PtCounters * sizeof(uint32_t));
  barriers.clear();
  Transition(barriers, r.water_counters[k].Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
  list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
  // Dispatch: the full ReSTIR PT schedule on the water reservoir.
  const uint32_t water_resampling = static_cast<uint32_t>(r.water_restir->GetResamplingMode());
  const bool has_temporal = water_resampling == 1u || water_resampling == 3u;
  const bool has_spatial = water_resampling == 2u || water_resampling == 3u;
  const bool temporal_ran = has_temporal && water_history;
  const RTXDI_PTDecorrelationParameters water_decorrelation = r.water_restir->GetDecorrelationParameters();
  const RTXDI_PTTemporalResamplingParameters water_temporal = r.water_restir->GetTemporalResamplingParameters();
  const bool needs_smoothed = water_decorrelation.decorrelationMode == RTXDI_PTDecorrelationMode::Stagnancy
                              && water_decorrelation.decorrelationFactor > 0.f && temporal_ran;
  ID3D12DescriptorHeap* heaps[] = {r.heap.Get()};
  list->SetDescriptorHeaps(1u, heaps);
  list->SetComputeRootSignature(r.root.Get());
  list->SetComputeRoot32BitConstants(0u, kRt12PtConstants, r.water_params, 0u);
  list->SetComputeRootConstantBufferView(2u, r.water_constants[k]->GetGPUVirtualAddress());
  D3D12_GPU_DESCRIPTOR_HANDLE table = r.heap->GetGPUDescriptorHandleForHeapStart();
  table.ptr += static_cast<UINT64>(base) * s.descriptor_size;
  list->SetComputeRootDescriptorTable(1u, table);
  if (water_perf_timing) list->EndQuery(r.perf_queries.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 2u * water_block);
  if (water_perf_timing) list->EndQuery(r.perf_queries.Get(), D3D12_QUERY_TYPE_TIMESTAMP,
                                        2u * kRt12PtPerfBlocks + 8u * water_block + 6u);
  list->SetPipelineState(r.water_initial_pso.Get());
  list->Dispatch((w + 7u) / 8u, (h + 7u) / 8u, 1u);
  barriers.clear();
  UavBarrier(barriers, r.water_reservoirs.Get());
  list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
  if (r.water_restir->GetSpatialResamplingParameters().enableSpatialHeuristicMode && has_spatial) {
    barriers.clear();
    UavBarrier(barriers, r.water_neighbor_selection_gbuffer.Get());
    list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
    list->SetPipelineState(r.water_neighbor_selection_pso.Get());
    list->Dispatch((w + 7u) / 8u, (h + 7u) / 8u, 1u);
    barriers.clear();
    UavBarrier(barriers, r.water_neighbor_selection_gbuffer.Get());
    UavBarrier(barriers, r.water_neighbor_selection.Get());
    list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
  }
  if (temporal_ran) {
    list->SetPipelineState(r.water_temporal_pso.Get());
    list->Dispatch((w + 7u) / 8u, (h + 7u) / 8u, 1u);
    barriers.clear();
    UavBarrier(barriers, r.water_reservoirs.Get());
    list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
  }
  if (has_spatial) {
    list->SetPipelineState(r.water_spatial_pso.Get());
    list->Dispatch((w + 7u) / 8u, (h + 7u) / 8u, 1u);
    barriers.clear();
    UavBarrier(barriers, r.water_reservoirs.Get());
    list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
  }
  if (::rtxdi::NeedsDuplicationMap(water_temporal, water_decorrelation) && temporal_ran) {
    barriers.clear();
    UavBarrier(barriers, r.water_reservoirs.Get());
    UavBarrier(barriers, r.water_sample_id.Get());
    UavBarrier(barriers, r.water_duplication_map.Get());
    UavBarrier(barriers, r.water_smoothed_duplication[frame & 1u].Get());
    list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
    list->SetPipelineState(r.water_duplication_pso.Get());
    list->Dispatch((w + 15u) / 16u, (h + 15u) / 16u, 1u);
    if (needs_smoothed) {
      barriers.clear();
      UavBarrier(barriers, r.water_duplication_map.Get());
      list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
      list->SetPipelineState(r.water_smoothed_duplication_pso.Get());
      list->Dispatch((w + 15u) / 16u, (h + 15u) / 16u, 1u);
    }
    barriers.clear();
    UavBarrier(barriers, r.water_smoothed_duplication[frame & 1u].Get());
    list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
  }
  list->SetPipelineState(r.water_final_pso.Get());
  list->Dispatch((w + 7u) / 8u, (h + 7u) / 8u, 1u);
  if (water_perf_timing) {
    list->EndQuery(r.perf_queries.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 2u * water_block + 1u);
    list->EndQuery(r.perf_queries.Get(), D3D12_QUERY_TYPE_TIMESTAMP,
                   2u * kRt12PtPerfBlocks + 8u * water_block + 7u);
    r.perf_valid_mask[water_block] = 8u;
    list->ResolveQueryData(r.perf_queries.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 2u * water_block, 2u, r.perf_readback.Get(),
                           16ull * water_block);
    const uint32_t trace_query = 2u * kRt12PtPerfBlocks + 8u * water_block + 6u;
    list->ResolveQueryData(r.perf_queries.Get(), D3D12_QUERY_TYPE_TIMESTAMP, trace_query, 2u, r.perf_readback.Get(),
                           16ull * kRt12PtPerfBlocks + 64ull * water_block + 48ull);
  }
  barriers.clear();
  Transition(barriers, refl.texture12.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON);
  Transition(barriers, r.water_reservoirs.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON);
  for (ID3D12Resource* resource : {r.water_sample_id.Get(), r.water_duplication_map.Get(),
                                   r.water_smoothed_duplication[frame & 1u].Get(), r.water_neighbor_selection_gbuffer.Get(),
                                   r.water_neighbor_selection.Get()}) {
    Transition(barriers, resource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON);
  }
  for (ID3D12Resource* resource : water_cache) {
    if (resource) Transition(barriers, resource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON);
  }
  Transition(barriers, r.water_counters[k].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
  list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
  const bool counters_copied = !r.water_counter_pending[k];
  if (counters_copied) {
    list->CopyBufferRegion(r.water_counter_readback[k].Get(), 0u, r.water_counters[k].Get(), 0u, kRt12PtCounters * sizeof(uint32_t));
  }
  barriers.clear();
  Transition(barriers, r.water_counters[k].Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COMMON);
  for (ID3D12Resource* resource : water_reads) {
    if (resource) Transition(barriers, resource, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COMMON);
  }
  list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
  if (FAILED(list->Close())) return fail_water(Rt12Reason::SubmitFailed, "pt water close", "record");
  // 7. Submission: wait for A and the previous D3D12 work, execute, signal B.
  bool ordered = false;
  try {
    ordered = renodx::utils::gpu_fence::QueueWait(s.queue12.Get(), s.fence11.get(), a);
    if (ordered && s.last_signal != 0u) {
      ordered = renodx::utils::gpu_fence::QueueWait(s.queue12.Get(), s.fence12.get(), s.last_signal);
    }
  } catch (const std::exception&) {
    ordered = false;
  }
  if (!ordered) return fail_water(Rt12Reason::SubmitFailed, "pt water order wait", "submit");
  RecordRt12WaitA(s, a, GpuEvidenceSource::Pt);
  if (s.last_signal != 0u) RecordRt12WaitPriorB(s, s.last_signal, GpuEvidenceSource::Pt);
  ID3D12CommandList* lists[] = {list};
  BeginRt12Submission(s);
  s.unfenced_execution = true;  // stays true if no signal B follows (teardown then leaks instead of freeing in-use objects)
  s.queue12->ExecuteCommandLists(1u, lists);
  bvh::AddPerfBlockMs(bvh::kPerfBlockPtWaterCpu, bvh::LiveElapsedMs(water_cpu_start));
  RecordRt12Evidence(s, GpuEvidenceEvent::Rt12Submit, GpuEvidenceSource::Pt);
  RecordGpuWorkBreadcrumb(GpuWorkKind::Rt12, k);
  uint64_t b = renodx::utils::gpu_fence::NO_SIGNAL;
  try {
    b = renodx::utils::gpu_fence::QueueSignal(s.queue12.Get(), s.fence12.get());
  } catch (const std::exception&) {
    b = renodx::utils::gpu_fence::NO_SIGNAL;
  }
  if (b == renodx::utils::gpu_fence::NO_SIGNAL) {
    s.sync_evidence.signal_b_result = E_FAIL;
    RecordRt12Evidence(s, GpuEvidenceEvent::Rt12SignalB, GpuEvidenceSource::Pt);
    return fail_water(Rt12Reason::SubmitFailed, "pt water signal", "submit");
  }
  s.unfenced_execution = false;
  water_slot.fence_value = b;
  r.water_reservoirs_last_use = b;
  r.water_history_frame = frame;
  r.water_history_width = w;
  r.water_history_height = h;
  r.water_history_slot = k;
  r.water_history_transfer_slot = r.water_input_slot;
  r.water_history_scene_slot = static_cast<uint32_t>(r.water_history_transfer_slot);
  r.water_history_owner_generation = s.generation;
  r.water_history_configuration_generation = r.configuration_generation;
  r.water_history_cache_epoch = r.cache_epoch;
  if (water_perf_timing) {
    r.perf_pending[water_block] = true;
    r.perf_fence[water_block] = b;
    r.perf_epoch[water_block] = water_perf_epoch;
  }
  if (counters_copied) {
    r.water_counter_fence[k] = b;
    r.water_counter_frame[k] = frame;
    r.water_counter_pending[k] = true;
    std::lock_guard lock(pt::g_pt_panel_mutex);
    pt::g_pt_panel.regir_water_pending_frame[k] = frame;
    pt::g_pt_panel.regir_water_pending_owner_generation[k] = r.water_params[48] != 0u ? regir_identity.owner_generation : 0u;
    pt::g_pt_panel.regir_water_pending_owner[k] = r.water_params[48] != 0u ? regir_slot : UINT32_MAX;
    pt::g_pt_panel.regir_water_submission_frame = frame;
    pt::g_pt_panel.regir_water_submission_owner_generation = r.water_params[48] != 0u ? regir_identity.owner_generation : 0u;
    pt::g_pt_panel.regir_water_counts_available = false;
    pt::g_pt_panel.regir_water_counts_frame = 0u;
    pt::g_pt_panel.regir_water_counts_owner_generation = 0u;
  }
  scene.fence_value = (std::max)(scene.fence_value, b);
  input.fence_value = (std::max)(input.fence_value, b);
  if (r.water_params[48] != 0u) {
    r.regir_last_use[regir_slot] = (std::max)(r.regir_last_use[regir_slot], b);
    input.di_fence_value = (std::max)(input.di_fence_value, b);
    std::lock_guard lock(pt::g_pt_panel_mutex);
    if (pt::g_pt_panel.regir_active && pt::g_pt_panel.regir_owner_generation == regir_identity.owner_generation
        && pt::g_pt_panel.regir_source_fence == regir_identity.table_fence
        && pt::g_pt_panel.regir_frame == regir_identity.frame && pt::g_pt_panel.regir_transfer_slot == regir_slot) {
      pt::g_pt_panel.regir_last_use_fence = r.regir_last_use[regir_slot];
    }
  }
  // The submitted inputs stay alive until this submission's fence completes; the snapshot is then empty for the next trace.
  for (const ComPtr<ID3D12Resource>& resource : in.resources) RetireObject(s, b, resource);
  RetireObject(s, b, in.tlas);
  r.water_frame_inputs = {};
  NoteRt12Signal(s, b);
  RecordRt12Evidence(s, GpuEvidenceEvent::Rt12SignalB, GpuEvidenceSource::Pt);
  // 8. D3D11 waits for B on the GPU before ssr2 reads the water reflection.
  bool waited = false;
  try {
    waited = renodx::utils::gpu_fence::QueueWait(s.ctx11.Get(), s.fence12.get(), b);
  } catch (const std::exception&) {
    waited = false;
  }
  if (!waited) return fail_water(Rt12Reason::SubmitFailed, "pt water d3d11 wait on fence B", "submit");
  RecordRt12WaitB(s, b, GpuEvidenceSource::Pt);
  // 9. Panel.
  {
    std::lock_guard lock(pt::g_pt_panel_mutex);
    pt::g_pt_panel.water_on = true;
    pt::g_pt_panel.water_reason = "ok";
  }
  return refl.srv11;
}
}  // namespace falcom_world::rt12