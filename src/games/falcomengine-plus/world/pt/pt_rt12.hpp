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
#include "../rtao/rtao.hpp"  // include order only: defines rtao::NullComputeSlots, which rt12_backend.hpp and rt12_rtxdi_transfer.hpp call; M1 uses no RTAO behaviour (moves to a neutral header in M6)
#include "../rt12/rt12_backend.hpp"
#include "../rt12/rt12_rtxdi_transfer.hpp"
#include "pt_state.hpp"
#include "pt_diagnostics.hpp"
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
    // Table: t0-t3 | t14-t16 | t29 | t30 | t31-t35 | u0-u3 | t36-t39 | u4-u6, descriptor order 0..24.
    const UINT counts[8] = {4u, 3u, 1u, 1u, 5u, 4u, 4u, 3u};
    const UINT bases[8] = {0u, 14u, 29u, 30u, 31u, 0u, 36u, 4u};
    const UINT offsets[8] = {0u, 4u, 7u, 8u, 9u, 14u, 18u, 22u};
    D3D12_DESCRIPTOR_RANGE ranges[8] = {};
    for (uint32_t i = 0u; i < 8u; ++i) {
      ranges[i].RangeType = (i == 5u || i == 7u) ? D3D12_DESCRIPTOR_RANGE_TYPE_UAV : D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
      ranges[i].NumDescriptors = counts[i];
      ranges[i].BaseShaderRegister = bases[i];
      ranges[i].OffsetInDescriptorsFromTableStart = offsets[i];
    }
    D3D12_ROOT_PARAMETER params[2] = {};
    params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    params[0].Constants.Num32BitValues = kRt12PtConstants;
    params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    params[1].DescriptorTable.NumDescriptorRanges = 8u;
    params[1].DescriptorTable.pDescriptorRanges = ranges;
    D3D12_ROOT_SIGNATURE_DESC root_desc = {};
    root_desc.NumParameters = 2u;
    root_desc.pParameters = params;
    ComPtr<ID3DBlob> blob, error;
    hr = s.serialize(&root_desc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &error);
    if (FAILED(hr)) return fail("pt root serialize", hr);
    hr = s.device12->CreateRootSignature(0u, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&r.root));
    if (FAILED(hr)) return fail("pt root", hr);
#if defined(__world_pt_diffuse_EMBED_FILE) && defined(__world_pt_sharc_update_EMBED_FILE) && defined(__world_pt_sharc_resolve_EMBED_FILE) && defined(__world_pt_material_write_EMBED_FILE)
    D3D12_COMPUTE_PIPELINE_STATE_DESC pso_desc = {};
    pso_desc.pRootSignature = r.root.Get();
    pso_desc.CS = {__world_pt_diffuse.data(), __world_pt_diffuse.size()};
    hr = s.device12->CreateComputePipelineState(&pso_desc, IID_PPV_ARGS(&r.pso));
    if (FAILED(hr)) return fail("pt pso", hr);
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
    D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
    heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heap_desc.NumDescriptors = kRt12Slots * kRt12PtDescriptors;
    heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    hr = s.device12->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&r.heap));
    if (FAILED(hr)) return fail("pt heap", hr);
    for (uint32_t i = 0u; i < kRt12Slots; ++i) {
      hr = CreateRt12Buffer(s.device12.Get(), D3D12_HEAP_TYPE_DEFAULT, kRt12PtCounters * sizeof(uint32_t), D3D12_RESOURCE_STATE_COMMON,
                            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, r.counters[i]);
      if (FAILED(hr)) return fail("pt counters", hr);
      hr = CreateRt12Buffer(s.device12.Get(), D3D12_HEAP_TYPE_READBACK, kRt12PtCounters * sizeof(uint32_t), D3D12_RESOURCE_STATE_COPY_DEST,
                            D3D12_RESOURCE_FLAG_NONE, r.counter_readback[i]);
      if (FAILED(hr)) return fail("pt counter readback", hr);
    }
    hr = CreateRt12Buffer(s.device12.Get(), D3D12_HEAP_TYPE_UPLOAD, kRt12PtCounters * sizeof(uint32_t), D3D12_RESOURCE_STATE_GENERIC_READ,
                          D3D12_RESOURCE_FLAG_NONE, r.counter_zero);
    if (FAILED(hr)) return fail("pt counter zero", hr);
    void* zeros = nullptr;
    hr = r.counter_zero->Map(0u, nullptr, &zeros);
    if (FAILED(hr)) return fail("pt counter zero map", hr);
    std::fill_n(static_cast<uint32_t*>(zeros), kRt12PtCounters, 0u);
    r.counter_zero->Unmap(0u, nullptr);
    r.created = true;
  }
  // 2b. GPU counters: a slot's counters are read once its submission's fence is complete. Never waits.
  const uint64_t completed = Rt12CompletedValue(s);
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
      pt::PtLogWarn("[world-rt12] pt stuck: fence at " + std::to_string(Rt12CompletedValue(s)) + ", slot submitted "
                    + std::to_string(pt_slot.fence_value));
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
  ID3D12Resource* deferred = mat_on && input.deferred_count != 0u ? input.deferred_buffer.Get() : nullptr;
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
  const auto set_buffer_uav = [&](uint32_t index, ID3D12Resource* resource, uint32_t stride) {
    D3D12_UNORDERED_ACCESS_VIEW_DESC view = {};
    view.Format = DXGI_FORMAT_UNKNOWN;
    view.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    view.Buffer.NumElements = sharc_on ? r.sharc_capacity : 1u;
    view.Buffer.StructureByteStride = stride;
    device12->CreateUnorderedAccessView(resource, nullptr, &view, cpu_at(base + index));
  };
  set_buffer_uav(15u, sharc_on ? r.sharc_hash.Get() : nullptr, 8u);
  set_buffer_uav(16u, sharc_on ? r.sharc_accumulation.Get() : nullptr, pt::kPtSharcAccumulationStride);
  set_buffer_uav(17u, sharc_on ? r.sharc_resolved.Get() : nullptr, pt::kPtSharcResolvedStride);
  ID3D12Resource* const sharc_buffers[] = {sharc_on ? r.sharc_hash.Get() : nullptr, sharc_on ? r.sharc_accumulation.Get() : nullptr,
                                           sharc_on ? r.sharc_resolved.Get() : nullptr};
  set_srv(18u, mat_on ? input.color.texture12.Get() : nullptr, Rt12TextureSrv(DXGI_FORMAT_R32G32B32A32_FLOAT));
  set_srv(19u, mat_on ? input.mrt1.texture12.Get() : nullptr, Rt12TextureSrv(DXGI_FORMAT_R32G32B32A32_UINT));
  set_srv(20u, mat_on ? input.mrt2.texture12.Get() : nullptr, Rt12TextureSrv(DXGI_FORMAT_R32G32B32A32_UINT));
  set_srv(21u, deferred, Rt12BufferSrv(RTXDI_INPUT_DEFERRED_STRIDE, deferred ? input.deferred_count : 1u));
  const auto set_material_uav = [&](uint32_t index, ID3D12Resource* resource, uint32_t stride, uint32_t count) {
    D3D12_UNORDERED_ACCESS_VIEW_DESC view = {};
    view.Format = DXGI_FORMAT_UNKNOWN;
    view.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    view.Buffer.NumElements = count;
    view.Buffer.StructureByteStride = stride;
    device12->CreateUnorderedAccessView(resource, nullptr, &view, cpu_at(base + index));
  };
  set_material_uav(22u, mat_on ? r.material_keys.Get() : nullptr, 8u, mat_on ? r.material_capacity : 1u);
  set_material_uav(23u, mat_on ? r.material_values.Get() : nullptr, 4u, mat_on ? r.material_capacity * 4u : 1u);
  set_material_uav(24u, r.counters[k].Get(), 4u, kRt12PtCounters);
  ID3D12Resource* const material_buffers[] = {mat_on ? r.material_keys.Get() : nullptr, mat_on ? r.material_values.Get() : nullptr};
  ID3D12Resource* const material_reads[] = {mat_on ? input.color.texture12.Get() : nullptr, mat_on ? input.mrt1.texture12.Get() : nullptr,
                                            mat_on ? input.mrt2.texture12.Get() : nullptr, deferred};
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
  Transition(barriers, r.counters[k].Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
  list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
  list->CopyBufferRegion(r.counters[k].Get(), 0u, r.counter_zero.Get(), 0u, kRt12PtCounters * sizeof(uint32_t));
  barriers.clear();
  Transition(barriers, r.counters[k].Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
  list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
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
      0u, 0u, 0u, 0u, 0u};
  ID3D12DescriptorHeap* heaps[] = {r.heap.Get()};
  list->SetDescriptorHeaps(1u, heaps);
  list->SetComputeRootSignature(r.root.Get());
  list->SetPipelineState(r.pso.Get());
  list->SetComputeRoot32BitConstants(0u, kRt12PtConstants, params, 0u);
  list->SetComputeRootDescriptorTable(1u, table);
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
    const uint32_t cells = ((w + mat_rate - 1u) / mat_rate) * ((h + mat_rate - 1u) / mat_rate);
    list->Dispatch((cells + 127u) / 128u, 1u, 1u);
    material_barrier();
    list->SetPipelineState(r.pso.Get());
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
    list->SetPipelineState(r.pso.Get());
  }
  list->Dispatch((w + 7u) / 8u, (h + 7u) / 8u, 1u);
  barriers.clear();
  Transition(barriers, out.texture12.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON);
  for (ID3D12Resource* resource : sharc_buffers) {
    if (resource) {
      Transition(barriers, resource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON);
    }
  }
  for (ID3D12Resource* resource : material_buffers) {
    if (resource) Transition(barriers, resource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON);
  }
  for (ID3D12Resource* resource : material_reads) {
    if (resource) Transition(barriers, resource, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COMMON);
  }
  for (ID3D12Resource* resource : reads) {
    if (resource) Transition(barriers, resource, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COMMON);
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
  if (!r.counter_pending[k]) {
    r.counter_fence[k] = b;
    r.counter_frame[k] = frame;
    r.counter_pending[k] = true;
  }
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
  pt::SetPtReason(pt::PtReason::Tracing);
  {
    std::lock_guard lock(pt::g_pt_panel_mutex);
    pt::g_pt_panel.lamps = lamps_on ? input.light_count : 0u;
    pt::g_pt_panel.width = w;
    pt::g_pt_panel.height = h;
    pt::g_pt_panel.sharc_on = sharc_on;
    pt::g_pt_panel.sharc_capacity_log2 = sharc_on ? static_cast<uint32_t>(std::countr_zero(r.sharc_capacity)) : 0u;
    pt::g_pt_panel.sharc_downscale = sharc_on ? sharc_ds : 0u;
    pt::g_pt_panel.sharc_window = sharc_window;
    pt::g_pt_panel.mat_on = mat_on;
    pt::g_pt_panel.mat_capacity_log2 = mat_on ? static_cast<uint32_t>(std::countr_zero(r.material_capacity)) : 0u;
    pt::g_pt_panel.mat_rate = mat_on ? mat_rate : 0u;
  }
  return out.srv11;
}
}  // namespace falcom_world::rt12
