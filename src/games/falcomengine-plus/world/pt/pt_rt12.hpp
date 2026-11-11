#pragma once
// Path-traced diffuse GI on DXR (M1): one D3D12 submission after the frame's RTXDI submissions. Reads the RTXDI transfer
// slot (depth, MRT0, scene rows, lamps) and the scene TLAS; writes one RGBA16F output per PT slot for the lighting draw (t47).
#include <algorithm>
#include <bit>
#include <vector>
#include "../debug/crash_log.hpp"
#include "../rtao/rtao.hpp"  // include order only: defines rtao::NullComputeSlots, which rt12_backend.hpp and rt12_rtxdi_transfer.hpp call; M1 uses no RTAO behaviour (moves to a neutral header in M6)
#include "../rt12/rt12_backend.hpp"
#include "../rt12/rt12_rtxdi_transfer.hpp"
#include "pt_state.hpp"
namespace falcom_world::rt12 {
inline reshade::api::resource_view RunPtTrace(reshade::api::device* device, reshade::api::command_list* cmd_list,
                                              int32_t transfer_slot, bool lamps) {
  using Microsoft::WRL::ComPtr;
  const auto fallback = [](pt::PtReason reason) { pt::SetPtReason(reason); return reshade::api::resource_view{0u}; };
  // 1. Gates.
  if (!g_rt12_enabled.load(std::memory_order_relaxed)) return fallback(pt::PtReason::DxrOff);
  if (!g_rt12 || g_rt12->device != device) return fallback(pt::PtReason::NotCreated);
  Rt12State& s = *g_rt12;
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
  Rt12DiTransferSlot& input = s.di_transfer.slots[static_cast<uint32_t>(transfer_slot)];
  Rt12Slot& scene = s.lighting_slots[static_cast<uint32_t>(s.scene_slot)];
  const auto fail = [&s, &fallback](const char* stage, HRESULT hr) {
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
  const bool lamps_on = lamps && input.light_count != 0u && input.lights_buffer && input.light_types;
  ID3D12Resource* lights = lamps_on ? input.lights_buffer.Get() : nullptr;
  ID3D12Resource* light_types = lamps_on ? input.light_types.Get() : nullptr;
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
    // Table: t0-t3 | t14-t16 | t29 | t30 | t31-t35 | u0, in descriptor order 0..14.
    const UINT counts[6] = {4u, 3u, 1u, 1u, 5u, 1u};
    const UINT bases[6] = {0u, 14u, 29u, 30u, 31u, 0u};
    const UINT offsets[6] = {0u, 4u, 7u, 8u, 9u, 14u};
    D3D12_DESCRIPTOR_RANGE ranges[6] = {};
    for (uint32_t i = 0u; i < 6u; ++i) {
      ranges[i].RangeType = i == 5u ? D3D12_DESCRIPTOR_RANGE_TYPE_UAV : D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
      ranges[i].NumDescriptors = counts[i];
      ranges[i].BaseShaderRegister = bases[i];
      ranges[i].OffsetInDescriptorsFromTableStart = offsets[i];
    }
    D3D12_ROOT_PARAMETER params[2] = {};
    params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    params[0].Constants.Num32BitValues = kRt12PtConstants;
    params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    params[1].DescriptorTable.NumDescriptorRanges = 6u;
    params[1].DescriptorTable.pDescriptorRanges = ranges;
    D3D12_ROOT_SIGNATURE_DESC root_desc = {};
    root_desc.NumParameters = 2u;
    root_desc.pParameters = params;
    ComPtr<ID3DBlob> blob, error;
    hr = s.serialize(&root_desc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &error);
    if (FAILED(hr)) return fail("pt root serialize", hr);
    hr = s.device12->CreateRootSignature(0u, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&r.root));
    if (FAILED(hr)) return fail("pt root", hr);
#if defined(__world_pt_diffuse_EMBED_FILE)
    D3D12_COMPUTE_PIPELINE_STATE_DESC pso_desc = {};
    pso_desc.pRootSignature = r.root.Get();
    pso_desc.CS = {__world_pt_diffuse.data(), __world_pt_diffuse.size()};
    hr = s.device12->CreateComputePipelineState(&pso_desc, IID_PPV_ARGS(&r.pso));
    if (FAILED(hr)) return fail("pt pso", hr);
#else
    return fail("pt shader not embedded", E_FAIL);
#endif
    D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
    heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heap_desc.NumDescriptors = kRt12Slots * kRt12PtDescriptors;
    heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    hr = s.device12->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&r.heap));
    if (FAILED(hr)) return fail("pt heap", hr);
    r.created = true;
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
      renodx::utils::log::w("[world-rt12] pt stuck: fence at ", Rt12CompletedValue(s), ", slot submitted ", pt_slot.fence_value);
      DisableMirror();
      return fallback(pt::PtReason::Stuck);
    }
  }
  // 4. This slot's output at the input size (a replaced texture is retired with this submission's fence).
  const size_t retired_begin = s.retired.size();
  const uint32_t w = input.width;
  const uint32_t h = input.height;
  Rt12Shared& out = r.output[k];
  if (!out.texture11 || out.width != w || out.height != h) {
    RetireShared(s, out, s.last_signal);
    if (FAILED(CreateRt12Shared(s, device, DXGI_FORMAT_R16G16B16A16_FLOAT, w, h, 1u, true, &out))) {
      RetireShared(s, out, s.last_signal);
      return fallback(pt::PtReason::TargetFailed);
    }
  }
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
  // 6. Record: descriptors, barriers, trace.
  ID3D12GraphicsCommandList4* list = r.list.Get();
  if (FAILED(pt_slot.allocator->Reset()) || FAILED(list->Reset(pt_slot.allocator.Get(), nullptr))) {
    return submit_fail(Rt12Reason::SubmitFailed, "pt reset");
  }
  ID3D12Device5* device12 = s.device12.Get();
  const uint32_t base = k * kRt12PtDescriptors;
  const auto cpu_at = [&r, &s](uint32_t index) {
    D3D12_CPU_DESCRIPTOR_HANDLE handle = r.heap->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += static_cast<SIZE_T>(index) * s.descriptor_size;
    return handle;
  };
  const auto set_srv = [&](uint32_t index, ID3D12Resource* resource, const D3D12_SHADER_RESOURCE_VIEW_DESC& view) {
    device12->CreateShaderResourceView(resource, &view, cpu_at(base + index));
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
  device12->CreateShaderResourceView(nullptr, &tlas_srv, cpu_at(base + 7u));
  set_srv(8u, dynamic_vertices, Rt12BufferSrv(16u, dynamic_vertices ? uint64_t{kRt12CarrierWidth} * kRt12CarrierHeight : 1u));
  set_srv(9u, input.depth.texture12.Get(), Rt12TextureSrv(DXGI_FORMAT_R32_FLOAT));
  set_srv(10u, input.normal.texture12.Get(), Rt12TextureSrv(DXGI_FORMAT_R32G32B32A32_UINT));
  set_srv(11u, input.scene.texture12.Get(), Rt12TextureSrv(DXGI_FORMAT_R32G32B32A32_UINT));
  set_srv(12u, lights, Rt12BufferSrv(RTXDI_INPUT_LIGHT_STRIDE, lamps_on ? input.light_count : 1u));
  set_srv(13u, light_types, Rt12BufferSrv(4u, RTXDI_INPUT_MAX_LIGHTS));
  const D3D12_UNORDERED_ACCESS_VIEW_DESC out_uav = Rt12TextureUav(DXGI_FORMAT_R16G16B16A16_FLOAT);
  device12->CreateUnorderedAccessView(out.texture12.Get(), nullptr, &out_uav, cpu_at(base + 14u));
  D3D12_GPU_DESCRIPTOR_HANDLE table = r.heap->GetGPUDescriptorHandleForHeapStart();
  table.ptr += static_cast<UINT64>(base) * s.descriptor_size;
  ID3D12Resource* const reads[] = {vertices, indices, mesh_table, scene.instance_table.Get(), uv_arena, s.atlas.texture12.Get(),
                                   s.materials.Get(), dynamic_vertices, input.depth.texture12.Get(), input.normal.texture12.Get(),
                                   input.scene.texture12.Get(), lights, light_types};
  std::vector<D3D12_RESOURCE_BARRIER>& barriers = pt_slot.barrier_scratch;
  barriers.clear();
  for (ID3D12Resource* resource : reads) {
    if (resource) Transition(barriers, resource, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
  }
  Transition(barriers, out.texture12.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
  list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
  const uint32_t params[kRt12PtConstants] = {
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
      0u};
  ID3D12DescriptorHeap* heaps[] = {r.heap.Get()};
  list->SetDescriptorHeaps(1u, heaps);
  list->SetComputeRootSignature(r.root.Get());
  list->SetPipelineState(r.pso.Get());
  list->SetComputeRoot32BitConstants(0u, kRt12PtConstants, params, 0u);
  list->SetComputeRootDescriptorTable(1u, table);
  list->Dispatch((w + 7u) / 8u, (h + 7u) / 8u, 1u);
  barriers.clear();
  Transition(barriers, out.texture12.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON);
  for (ID3D12Resource* resource : reads) {
    if (resource) Transition(barriers, resource, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COMMON);
  }
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
  scene.fence_value = (std::max)(scene.fence_value, b);
  input.fence_value = (std::max)(input.fence_value, b);
  for (size_t i = retired_begin; i < s.retired.size(); ++i) s.retired[i].fence_value = b;
  NoteRt12Signal(s, b);
  RecordRt12Evidence(s, GpuEvidenceEvent::Rt12SignalB, GpuEvidenceSource::Pt);
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
  // 9. Panel.
  {
    std::lock_guard lock(pt::g_pt_panel_mutex);
    pt::g_pt_panel.reason = pt::PtReason::Tracing;
    ++pt::g_pt_panel.frames[static_cast<uint32_t>(pt::PtReason::Tracing)];
    pt::g_pt_panel.lamps = lamps_on ? input.light_count : 0u;
    pt::g_pt_panel.width = w;
    pt::g_pt_panel.height = h;
  }
  return out.srv11;
}
}  // namespace falcom_world::rt12
