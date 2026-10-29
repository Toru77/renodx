#pragma once

// RTGI on DXR (M2): the RTGI trace on the D3D12 side against pass A's scene (the TLAS, the instance table and the mirror
// arenas of scene_slot), fed by three D3D11 pack passes (world_rt12_rtgi_pack, _pack_lights, _resolve) into shared
// textures. Runs on the render thread, under the lock rule of rt12_backend.hpp.

#include <span>

#include "rt12_backend.hpp"

namespace falcom_world::rt12 {

// The RTGI shared textures (Rt12RtgiState::shared), by index.
enum Rt12RtgiShared : uint32_t {
  kRt12RtgiDepth,
  kRt12RtgiNormal,
  kRt12RtgiRadiance,
  kRt12RtgiPrevFinal,
  kRt12RtgiAlbedo,
  kRt12RtgiMrt1,
  kRt12RtgiShadow,
  kRt12RtgiMrt2,
  kRt12RtgiScene,
  kRt12RtgiLamps,
  kRt12RtgiLampTypes,
  kRt12RtgiDeferred,
  kRt12RtgiStats,
  kRt12RtgiRaw,
  kRt12RtgiDebug,
};

#if defined(__world_rt12_rtgi_pack_EMBED_FILE)
inline constexpr std::span<const std::uint8_t> kRt12RtgiPackShader = __world_rt12_rtgi_pack;
#else
inline constexpr std::span<const std::uint8_t> kRt12RtgiPackShader = {};
#endif
#if defined(__world_rt12_rtgi_pack_lights_EMBED_FILE)
inline constexpr std::span<const std::uint8_t> kRt12RtgiPackLightsShader = __world_rt12_rtgi_pack_lights;
#else
inline constexpr std::span<const std::uint8_t> kRt12RtgiPackLightsShader = {};
#endif
#if defined(__world_rt12_rtgi_resolve_EMBED_FILE)
inline constexpr std::span<const std::uint8_t> kRt12RtgiResolveShader = __world_rt12_rtgi_resolve;
#else
inline constexpr std::span<const std::uint8_t> kRt12RtgiResolveShader = {};
#endif

// The RTGI inputs of one lighting draw. A view with handle 0 is not bound this frame. lamp_count and deferred_count are the
// element counts of the game's dynamicLights_g and deferredParams_g (capped at 1024 and 0x4e20 here).
struct Rt12RtgiInputs {
  reshade::api::resource_view depth = {0u};
  reshade::api::resource_view mrt_normal = {0u};
  reshade::api::resource_view isfast = {0u};
  reshade::api::resource_view scene_cbv = {0u};
  reshade::api::resource_view radiance = {0u};
  reshade::api::resource_view prev_gi = {0u};
  reshade::api::resource_view albedo = {0u};
  reshade::api::resource_view lights = {0u};
  reshade::api::resource_view lamp_types = {0u};
  reshade::api::resource_view mrt1 = {0u};
  reshade::api::resource_view shadow = {0u};
  reshade::api::resource_view mrt2 = {0u};
  reshade::api::resource_view deferred = {0u};
  reshade::api::resource raw_texture = {0u};
  reshade::api::resource debug_texture = {0u};
  reshade::api::resource_view stats_uav = {0u};
  uint32_t trace_width = 0u;
  uint32_t trace_height = 0u;
  uint32_t full_width = 0u;
  uint32_t full_height = 0u;
  bool trace_characters = false;
  uint32_t lamp_count = 0u;
  uint32_t deferred_count = 0u;
};

// Counts a frame that does not trace RTGI, with its reason.
inline bool RtgiFallback(Rt12RtgiReason reason) {
  std::lock_guard lock(g_rt12_rtgi_panel_mutex);
  g_rt12_rtgi_panel.reason = reason;
  g_rt12_rtgi_panel.frames[static_cast<uint32_t>(reason)] += 1u;
  return false;
}

// Creates one RTGI shared texture at width x height; a size change retires the old one (its fence is set by the caller).
inline bool EnsureRtgiShared(Rt12State& s, reshade::api::device* device, Rt12Shared& shared, DXGI_FORMAT format, uint32_t width,
                             uint32_t height) {
  if (shared.texture11 && shared.width == width && shared.height == height) return true;
  RetireShared(s, shared, s.last_signal);
  return SUCCEEDED(CreateRt12Shared(s, device, format, width, height, 1u, true, &shared));
}

// RTGI trace (B). Returns false when no GI was traced this frame; the reason is in the RTGI panel.
inline bool RunRtgiTrace(reshade::api::device* device, reshade::api::command_list* cmd_list, const rtgi::RtgiPushConstants& push,
                         const Rt12RtgiInputs& in) {
#if defined(_WIN32)
  using Microsoft::WRL::ComPtr;
  using DS = reshade::api::shader_stage;
  using DT = reshade::api::descriptor_type;
  // 1. Gates.
  if (!g_rt12_enabled.load(std::memory_order_relaxed)) return RtgiFallback(Rt12RtgiReason::DxrOff);
  if (!g_rt12 || g_rt12->device != device) return RtgiFallback(Rt12RtgiReason::NotCreated);
  Rt12State& s = *g_rt12;
  if (s.stuck) return RtgiFallback(Rt12RtgiReason::Stuck);
  if (s.failed) return RtgiFallback(Rt12RtgiReason::Failed);
  if (reinterpret_cast<ID3D11DeviceContext*>(cmd_list->get_native()) != s.ctx11.Get()) {  // NOLINT(performance-no-int-to-ptr)
    return RtgiFallback(Rt12RtgiReason::ContextMismatch);
  }
  const uint32_t frame = g_state.frame.load();
  if (s.scene_frame != frame || s.scene_slot < 0) return RtgiFallback(Rt12RtgiReason::SceneMissing);
  if (in.isfast.handle != 0u && s.noise_source != in.isfast.handle) return RtgiFallback(Rt12RtgiReason::SceneMissing);
  Rt12RtgiState& r = s.rtgi;
  const auto fail = [&s](const char* stage, HRESULT hr) {
    FailSession(s, Rt12Reason::CreateFailed, stage, hr);
    return RtgiFallback(Rt12RtgiReason::Failed);
  };

  // 2. First use: allocators, list, root signature, PSO, heaps, queries, fixed buffers and fixed shared textures.
  if (!r.created) {
    HRESULT hr = S_OK;
    for (uint32_t k = 0u; k < kRt12Slots; ++k) {
      hr = s.device12->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&r.slots[k].allocator));
      if (FAILED(hr)) return fail("rtgi allocator", hr);
    }
    hr = s.device12->CreateCommandList(0u, D3D12_COMMAND_LIST_TYPE_DIRECT, r.slots[0].allocator.Get(), nullptr, IID_PPV_ARGS(&r.list));
    if (FAILED(hr)) return fail("rtgi list", hr);
    hr = r.list->Close();
    if (FAILED(hr)) return fail("rtgi close", hr);

    // Root signature (C4): root CBV b0 (scene rows), 32 constants at b12, one table of 25 descriptors in register order.
    D3D12_DESCRIPTOR_RANGE ranges[4] = {};
    const auto set_range = [](D3D12_DESCRIPTOR_RANGE& range, D3D12_DESCRIPTOR_RANGE_TYPE type, UINT count, UINT base) {
      range.RangeType = type;
      range.NumDescriptors = count;
      range.BaseShaderRegister = base;
      range.RegisterSpace = 0u;
      range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
    };
    set_range(ranges[0], D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4u, 0u);
    set_range(ranges[1], D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1u, 9u);
    set_range(ranges[2], D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 17u, 14u);
    set_range(ranges[3], D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 3u, 0u);
    D3D12_ROOT_PARAMETER parameters[3] = {};
    parameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    parameters[0].Descriptor.ShaderRegister = 0u;
    parameters[0].Descriptor.RegisterSpace = 0u;
    parameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    parameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    parameters[1].Constants.ShaderRegister = 12u;
    parameters[1].Constants.RegisterSpace = 0u;
    parameters[1].Constants.Num32BitValues = rtgi::kRtgiPushConstantCount;
    parameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    parameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    parameters[2].DescriptorTable.NumDescriptorRanges = 4u;
    parameters[2].DescriptorTable.pDescriptorRanges = ranges;
    parameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    D3D12_ROOT_SIGNATURE_DESC root_desc = {};
    root_desc.NumParameters = 3u;
    root_desc.pParameters = parameters;
    ComPtr<ID3DBlob> blob;
    ComPtr<ID3DBlob> error;
    hr = s.serialize(&root_desc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &error);
    if (FAILED(hr)) return fail("rtgi root serialize", hr);
    hr = s.device12->CreateRootSignature(0u, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&r.root));
    if (FAILED(hr)) return fail("rtgi root", hr);

#if defined(__world_rt12_rtgi_EMBED_FILE)
    D3D12_COMPUTE_PIPELINE_STATE_DESC pso_desc = {};
    pso_desc.pRootSignature = r.root.Get();
    pso_desc.CS.pShaderBytecode = __world_rt12_rtgi.data();
    pso_desc.CS.BytecodeLength = __world_rt12_rtgi.size();
    hr = s.device12->CreateComputePipelineState(&pso_desc, IID_PPV_ARGS(&r.pso));
    if (FAILED(hr)) return fail("rtgi pso", hr);
#else
    return fail("rtgi shader not embedded", E_FAIL);
#endif

    D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
    heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heap_desc.NumDescriptors = kRt12Slots * kRt12RtgiDescriptors;
    heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    hr = s.device12->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&r.heap));
    if (FAILED(hr)) return fail("rtgi heap", hr);
    heap_desc.NumDescriptors = 1u;
    heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    hr = s.device12->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&r.cpu_heap));
    if (FAILED(hr)) return fail("rtgi cpu heap", hr);

    D3D12_QUERY_HEAP_DESC query_desc = {};
    query_desc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
    query_desc.Count = kRt12Slots * kRt12RtgiTimestamps;
    hr = s.device12->CreateQueryHeap(&query_desc, IID_PPV_ARGS(&r.queries));
    if (FAILED(hr)) return fail("rtgi queries", hr);
    hr = CreateRt12Buffer(s.device12.Get(), D3D12_HEAP_TYPE_READBACK, kRt12Slots * kRt12RtgiTimestamps * 8u,
                          D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_FLAG_NONE, r.query_readback);
    if (FAILED(hr)) return fail("rtgi readback", hr);

    // The buffers hold the whole carrier (the copies are whole carriers); the SRVs count the game's elements.
    hr = CreateRt12Buffer(s.device12.Get(), D3D12_HEAP_TYPE_DEFAULT, kRt12RtgiSceneRows * 16u, D3D12_RESOURCE_STATE_COMMON,
                          D3D12_RESOURCE_FLAG_NONE, r.scene_cb);
    if (FAILED(hr)) return fail("rtgi scene buffer", hr);
    hr = CreateRt12Buffer(s.device12.Get(), D3D12_HEAP_TYPE_DEFAULT,
                          uint64_t{rtgi::kRtgiLampCount} * kRt12RtgiLampTexels * 16u, D3D12_RESOURCE_STATE_COMMON,
                          D3D12_RESOURCE_FLAG_NONE, r.lamps);
    if (FAILED(hr)) return fail("rtgi lamp buffer", hr);
    hr = CreateRt12Buffer(s.device12.Get(), D3D12_HEAP_TYPE_DEFAULT, uint64_t{rtgi::kRtgiLampCount} * 4u, D3D12_RESOURCE_STATE_COMMON,
                          D3D12_RESOURCE_FLAG_NONE, r.lamp_types);
    if (FAILED(hr)) return fail("rtgi lamp type buffer", hr);
    hr = CreateRt12Buffer(s.device12.Get(), D3D12_HEAP_TYPE_DEFAULT,
                          uint64_t{kRt12RtgiCarrierWidth} * kRt12RtgiDeferredRows * 16u, D3D12_RESOURCE_STATE_COMMON,
                          D3D12_RESOURCE_FLAG_NONE, r.deferred);
    if (FAILED(hr)) return fail("rtgi deferred buffer", hr);

    if (!EnsureRtgiShared(s, device, r.shared[kRt12RtgiScene], DXGI_FORMAT_R32G32B32A32_UINT, kRt12RtgiSceneRows, 1u)
        || !EnsureRtgiShared(s, device, r.shared[kRt12RtgiLamps], DXGI_FORMAT_R32G32B32A32_UINT,
                             kRt12RtgiLampTexels * rtgi::kRtgiLampCount, 1u)
        || !EnsureRtgiShared(s, device, r.shared[kRt12RtgiLampTypes], DXGI_FORMAT_R32_UINT, rtgi::kRtgiLampCount, 1u)
        || !EnsureRtgiShared(s, device, r.shared[kRt12RtgiDeferred], DXGI_FORMAT_R32G32B32A32_UINT, kRt12RtgiCarrierWidth,
                             kRt12RtgiDeferredRows)
        || !EnsureRtgiShared(s, device, r.shared[kRt12RtgiStats], DXGI_FORMAT_R32_UINT, kRt12StatsWidth, 1u)) {
      return fail("rtgi fixed textures", E_FAIL);
    }
    r.created = true;
  }

  // 3. The three D3D11 pipelines.
  const rtgi::RtgiPipelineDesc pack_desc = {.srv_count = 5u, .uav_count = 6u, .sampler_count = 0u, .push_count = 4u,
                                            .code = kRt12RtgiPackShader, .name = "rt12 rtgi pack"};
  const rtgi::RtgiPipelineDesc pack_lights_desc = {.srv_count = 6u, .uav_count = 6u, .sampler_count = 0u, .push_count = 4u,
                                                   .code = kRt12RtgiPackLightsShader, .name = "rt12 rtgi pack lights"};
  const rtgi::RtgiPipelineDesc resolve_desc = {.srv_count = 1u, .uav_count = 1u, .sampler_count = 0u, .push_count = 4u,
                                               .code = kRt12RtgiResolveShader, .name = "rt12 rtgi resolve"};
  if (!rtgi::EnsureRtgiPipeline(device, &r.pack, pack_desc) || !rtgi::EnsureRtgiPipeline(device, &r.pack_lights, pack_lights_desc)
      || !rtgi::EnsureRtgiPipeline(device, &r.resolve, resolve_desc)) {
    return RtgiFallback(Rt12RtgiReason::TargetFailed);
  }

  // 4. This frame's slot: its previous submission must be complete (a timeout is stuck).
  const uint32_t k = r.next;
  Rt12Slot& slot = r.slots[k];
  if (slot.fence_value > s.fence12->GetCompletedValue()) {
    const LARGE_INTEGER wait_start = QpcNow();
    const bool done = WaitRt12Fence(s, slot.fence_value, kRt12StuckMs);
    const float wait_ms = static_cast<float>(QpcMsSince(wait_start));
    {
      std::lock_guard lock(g_rt12_rtgi_panel_mutex);
      g_rt12_rtgi_panel.waits += 1u;
      g_rt12_rtgi_panel.wait_ms_max = std::max(g_rt12_rtgi_panel.wait_ms_max, wait_ms);
    }
    if (!done) {
      s.stuck = true;
      renodx::utils::log::w("[world-rt12] rtgi stuck: fence at ", s.fence12->GetCompletedValue(), ", slot submitted ", slot.fence_value);
      DisableMirror();
      return RtgiFallback(Rt12RtgiReason::Stuck);
    }
  }

  // 5. Per-frame shared textures by size (a replaced one is retired with this frame's fence in step 8).
  const size_t retired_begin = s.retired.size();
  const uint32_t tw = in.trace_width;
  const uint32_t th = in.trace_height;
  const uint32_t fw = in.full_width;
  const uint32_t fh = in.full_height;
  if (!EnsureRtgiShared(s, device, r.shared[kRt12RtgiDepth], DXGI_FORMAT_R32_FLOAT, fw, fh)
      || !EnsureRtgiShared(s, device, r.shared[kRt12RtgiNormal], DXGI_FORMAT_R32G32B32A32_UINT, tw, th)
      || !EnsureRtgiShared(s, device, r.shared[kRt12RtgiRaw], DXGI_FORMAT_R16G16B16A16_FLOAT, tw, th)
      || !EnsureRtgiShared(s, device, r.shared[kRt12RtgiDebug], DXGI_FORMAT_R8G8B8A8_UNORM, fw, fh)) {
    return RtgiFallback(Rt12RtgiReason::TargetFailed);
  }

  // Inputs packed this frame; the others are null in the descriptor table and skipped by the D3D11 passes.
  const bool sun = Rt12RtgiReadsSunInputs(push.sun[2], static_cast<int>(push.sampling[3]));
  const bool material = Rt12RtgiReadsMaterialInputs(push.extra[0]);
  const bool radiance_on = in.radiance.handle != 0u;
  const bool prev_on = in.prev_gi.handle != 0u;
  const bool albedo_on = in.albedo.handle != 0u;
  const bool lamps_on = in.lights.handle != 0u && in.lamp_types.handle != 0u;
  const bool mrt1_on = sun && in.mrt1.handle != 0u;
  const bool shadow_on = sun && in.shadow.handle != 0u;
  const bool mrt2_on = material && in.mrt2.handle != 0u;
  const bool deferred_on = material && in.deferred.handle != 0u;
  const bool pack_lights_on = mrt1_on || shadow_on || mrt2_on || deferred_on || lamps_on;
  const bool debug_on = in.debug_texture.handle != 0u;
  const bool characters_on = in.trace_characters && s.scene_dynamic_count > 0u && s.dynamic_vertices != nullptr;
  const uint32_t lamp_elements = std::min(in.lamp_count, rtgi::kRtgiLampCount);
  const uint32_t deferred_elements = std::min(in.deferred_count, kRt12RtgiDeferredMax);
  const auto pick = [](bool on, reshade::api::resource_view view) { return on ? view : reshade::api::resource_view{0u}; };
  // A packed source is shared at its own size (the RTAO ensure_target rule); an unpacked one is not ensured.
  const auto source_ensure = [&](bool on, reshade::api::resource_view view, uint32_t index, DXGI_FORMAT format, uint32_t& width,
                                 uint32_t& height) {
    if (!on) return true;
    const reshade::api::resource_desc desc = device->get_resource_desc(device->get_resource_from_view(view));
    width = desc.texture.width;
    height = desc.texture.height;
    return EnsureRtgiShared(s, device, r.shared[index], format, width, height);
  };
  uint32_t radiance_w = 0u, radiance_h = 0u, prev_w = 0u, prev_h = 0u, albedo_w = 0u, albedo_h = 0u;
  uint32_t mrt1_w = 0u, mrt1_h = 0u, shadow_w = 0u, shadow_h = 0u, mrt2_w = 0u, mrt2_h = 0u;
  if (!source_ensure(radiance_on, in.radiance, kRt12RtgiRadiance, DXGI_FORMAT_R16G16B16A16_FLOAT, radiance_w, radiance_h)
      || !source_ensure(prev_on, in.prev_gi, kRt12RtgiPrevFinal, DXGI_FORMAT_R16G16B16A16_FLOAT, prev_w, prev_h)
      || !source_ensure(albedo_on, in.albedo, kRt12RtgiAlbedo, DXGI_FORMAT_R16G16B16A16_FLOAT, albedo_w, albedo_h)
      || !source_ensure(mrt1_on, in.mrt1, kRt12RtgiMrt1, DXGI_FORMAT_R32G32B32A32_UINT, mrt1_w, mrt1_h)
      || !source_ensure(shadow_on, in.shadow, kRt12RtgiShadow, DXGI_FORMAT_R32_FLOAT, shadow_w, shadow_h)
      || !source_ensure(mrt2_on, in.mrt2, kRt12RtgiMrt2, DXGI_FORMAT_R32G32_UINT, mrt2_w, mrt2_h)) {
    return RtgiFallback(Rt12RtgiReason::TargetFailed);
  }
  // Pack A covers depth and the packed pack-A sources; pack lights covers the lamp and deferred carriers and the packed sources.
  const uint32_t pack_w = std::max({fw, radiance_w, prev_w, albedo_w});
  const uint32_t pack_h = std::max({fh, radiance_h, prev_h, albedo_h});
  const uint32_t lights_w = std::max({kRt12RtgiLampTexels * rtgi::kRtgiLampCount, kRt12RtgiCarrierWidth, mrt1_w, shadow_w, mrt2_w});
  const uint32_t lights_h = std::max({kRt12RtgiDeferredRows, mrt1_h, shadow_h, mrt2_h});

  // 6. D3D11: pack A, pack lights, then signal A (flushed).
  const auto bind_pass = [&](const rtgi::RtgiPipeline& p, uint32_t srv_count, const reshade::api::resource_view* srv_views,
                             uint32_t uav_count, const reshade::api::resource_view* uav_views) {
    const reshade::api::buffer_range scene_range = rtao::SceneCbvRange(in.scene_cbv);
    reshade::api::descriptor_table_update updates[3] = {
        {p.cbv_table, 0, 0, 1, DT::constant_buffer, &scene_range},
        {p.srv_table, 0, 0, srv_count, DT::shader_resource_view, srv_views},
        {p.uav_table, 0, 0, uav_count, DT::unordered_access_view, uav_views},
    };
    device->update_descriptor_tables(3, updates);
    cmd_list->bind_pipeline(reshade::api::pipeline_stage::all_compute, p.pipeline);
    const reshade::api::descriptor_table tables[3] = {p.cbv_table, p.srv_table, p.uav_table};
    cmd_list->bind_descriptor_tables(DS::all_compute, p.layout, 0, 3, tables);
  };
  // Nulls a pipeline's app-side SRV and UAV tables after its last dispatch (rtgi.hpp pattern).
  const auto null_tables = [device](const rtgi::RtgiPipeline& p, uint32_t srv_count, uint32_t uav_count) {
    const reshade::api::resource_view null_views[6] = {};
    reshade::api::descriptor_table_update updates[2] = {
        {p.srv_table, 0, 0, srv_count, DT::shader_resource_view, null_views},
        {p.uav_table, 0, 0, uav_count, DT::unordered_access_view, null_views},
    };
    device->update_descriptor_tables(2, updates);
  };
  {
    const reshade::api::resource_view srvs[5] = {in.depth, in.mrt_normal, pick(radiance_on, in.radiance), pick(prev_on, in.prev_gi),
                                                 pick(albedo_on, in.albedo)};
    const reshade::api::resource_view uavs[6] = {
        r.shared[kRt12RtgiDepth].uav11,
        r.shared[kRt12RtgiNormal].uav11,
        pick(radiance_on, r.shared[kRt12RtgiRadiance].uav11),
        pick(prev_on, r.shared[kRt12RtgiPrevFinal].uav11),
        pick(albedo_on, r.shared[kRt12RtgiAlbedo].uav11),
        r.shared[kRt12RtgiScene].uav11,
    };
    bind_pass(r.pack, 5u, srvs, 6u, uavs);
    const float pack_push[4] = {push.params[3], static_cast<float>(tw), static_cast<float>(th), 0.f};
    cmd_list->push_constants(DS::all_compute, r.pack.layout, 3, 0, 4, pack_push);
    cmd_list->dispatch((pack_w + 7u) / 8u, (pack_h + 7u) / 8u, 1u);
    rtao::NullComputeSlots(cmd_list, 5u, 6u, 0u);
    null_tables(r.pack, 5u, 6u);
  }
  if (pack_lights_on) {
    const reshade::api::resource_view srvs[6] = {pick(mrt1_on, in.mrt1), pick(shadow_on, in.shadow), pick(mrt2_on, in.mrt2),
                                                 pick(deferred_on, in.deferred), pick(lamps_on, in.lights),
                                                 pick(lamps_on, in.lamp_types)};
    const reshade::api::resource_view uavs[6] = {
        pick(mrt1_on, r.shared[kRt12RtgiMrt1].uav11),
        pick(shadow_on, r.shared[kRt12RtgiShadow].uav11),
        pick(mrt2_on, r.shared[kRt12RtgiMrt2].uav11),
        pick(deferred_on, r.shared[kRt12RtgiDeferred].uav11),
        pick(lamps_on, r.shared[kRt12RtgiLamps].uav11),
        pick(lamps_on, r.shared[kRt12RtgiLampTypes].uav11),
    };
    bind_pass(r.pack_lights, 6u, srvs, 6u, uavs);
    cmd_list->dispatch((lights_w + 7u) / 8u, (lights_h + 7u) / 8u, 1u);
    rtao::NullComputeSlots(cmd_list, 6u, 6u, 0u);
    null_tables(r.pack_lights, 6u, 6u);
  }
  uint64_t a = renodx::utils::gpu_fence::NO_SIGNAL;
  try {
    const bvh::PoolCpuTimer flush_timer{&bvh::g_perf_gate.block_us[bvh::kPerfBlockRt12SignalFlush]};
    a = renodx::utils::gpu_fence::QueueSignal(s.ctx11.Get(), s.fence11.get());
    s.ctx11->Flush();
  } catch (const std::exception&) {
    a = renodx::utils::gpu_fence::NO_SIGNAL;
  }
  if (a == renodx::utils::gpu_fence::NO_SIGNAL) {
    FailSession(s, Rt12Reason::FenceError, "rtgi fence11 signal");
    return RtgiFallback(Rt12RtgiReason::SubmitFailed);
  }

  // 7. D3D12 list: timestamp 0, the carrier copies, timestamp 1, the descriptors, the barriers, the trace, timestamp 2.
  ID3D12GraphicsCommandList4* list = r.list.Get();
  if (FAILED(slot.allocator->Reset()) || FAILED(list->Reset(slot.allocator.Get(), nullptr))) {
    FailSession(s, Rt12Reason::SubmitFailed, "rtgi reset");
    return RtgiFallback(Rt12RtgiReason::SubmitFailed);
  }
  const uint32_t base_q = k * kRt12RtgiTimestamps;
  const uint32_t base = k * kRt12RtgiDescriptors;
  list->EndQuery(r.queries.Get(), D3D12_QUERY_TYPE_TIMESTAMP, base_q);

  const auto copy_to_buffer = [list](Rt12Shared& carrier, ID3D12Resource* buffer, DXGI_FORMAT format, uint32_t width, uint32_t height,
                                     uint32_t texel_bytes) {
    D3D12_TEXTURE_COPY_LOCATION dst = {};
    dst.pResource = buffer;
    dst.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    dst.PlacedFootprint.Offset = 0u;
    dst.PlacedFootprint.Footprint.Format = format;
    dst.PlacedFootprint.Footprint.Width = width;
    dst.PlacedFootprint.Footprint.Height = height;
    dst.PlacedFootprint.Footprint.Depth = 1u;
    dst.PlacedFootprint.Footprint.RowPitch = width * texel_bytes;
    D3D12_TEXTURE_COPY_LOCATION src = {};
    src.pResource = carrier.texture12.Get();
    src.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    src.SubresourceIndex = 0u;
    list->CopyTextureRegion(&dst, 0u, 0u, 0u, &src, nullptr);
  };
  std::vector<D3D12_RESOURCE_BARRIER> barriers;
  Transition(barriers, r.shared[kRt12RtgiScene].texture12.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
  Transition(barriers, r.scene_cb.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
  if (lamps_on) {
    Transition(barriers, r.shared[kRt12RtgiLamps].texture12.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
    Transition(barriers, r.shared[kRt12RtgiLampTypes].texture12.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
    Transition(barriers, r.lamps.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
    Transition(barriers, r.lamp_types.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
  }
  if (deferred_on) {
    Transition(barriers, r.shared[kRt12RtgiDeferred].texture12.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
    Transition(barriers, r.deferred.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
  }
  list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
  copy_to_buffer(r.shared[kRt12RtgiScene], r.scene_cb.Get(), DXGI_FORMAT_R32G32B32A32_UINT, kRt12RtgiSceneRows, 1u, 16u);
  if (lamps_on) {
    copy_to_buffer(r.shared[kRt12RtgiLamps], r.lamps.Get(), DXGI_FORMAT_R32G32B32A32_UINT, kRt12RtgiLampTexels * rtgi::kRtgiLampCount,
                   1u, 16u);
    copy_to_buffer(r.shared[kRt12RtgiLampTypes], r.lamp_types.Get(), DXGI_FORMAT_R32_UINT, rtgi::kRtgiLampCount, 1u, 4u);
  }
  if (deferred_on) {
    copy_to_buffer(r.shared[kRt12RtgiDeferred], r.deferred.Get(), DXGI_FORMAT_R32G32B32A32_UINT, kRt12RtgiCarrierWidth,
                   kRt12RtgiDeferredRows, 16u);
  }
  list->EndQuery(r.queries.Get(), D3D12_QUERY_TYPE_TIMESTAMP, base_q + 1u);

  // The C4 descriptors of this slot: t0..t3, t9, t14..t30, u0..u2. Unbound inputs get a null descriptor.
  ID3D12Device5* device12 = s.device12.Get();
  const auto cpu_at = [&r, &s](uint32_t index) {
    D3D12_CPU_DESCRIPTOR_HANDLE handle = r.heap->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += static_cast<SIZE_T>(index) * s.descriptor_size;
    return handle;
  };
  const auto gpu_at = [&r, &s](uint32_t index) {
    D3D12_GPU_DESCRIPTOR_HANDLE handle = r.heap->GetGPUDescriptorHandleForHeapStart();
    handle.ptr += static_cast<UINT64>(index) * s.descriptor_size;
    return handle;
  };
  const auto set_srv = [&](uint32_t index, ID3D12Resource* resource, const D3D12_SHADER_RESOURCE_VIEW_DESC& view) {
    device12->CreateShaderResourceView(resource, &view, cpu_at(base + index));
  };
  const auto set_uav = [&](uint32_t index, ID3D12Resource* resource, const D3D12_UNORDERED_ACCESS_VIEW_DESC& view) {
    device12->CreateUnorderedAccessView(resource, nullptr, &view, cpu_at(base + index));
  };
  const auto shared_resource = [&r](bool on, uint32_t index) {
    return on ? r.shared[index].texture12.Get() : static_cast<ID3D12Resource*>(nullptr);
  };
  const Rt12Slot& scene = s.lighting_slots[static_cast<uint32_t>(s.scene_slot)];
  const bool dynamic_on = characters_on;
  set_srv(0u, s.arenas[kRt12ArenaVertices].buffer.Get(),
          Rt12BufferSrv(kRt12ArenaStride[kRt12ArenaVertices], s.arenas[kRt12ArenaVertices].used));
  set_srv(1u, s.arenas[kRt12ArenaIndices].buffer.Get(), Rt12BufferSrv(kRt12ArenaStride[kRt12ArenaIndices], s.arenas[kRt12ArenaIndices].used));
  set_srv(2u, s.mesh_table.Get(), Rt12BufferSrv(sizeof(bvh::WorldMeshGPU), s.mesh_count));
  set_srv(3u, scene.instance_table.Get(), Rt12BufferSrv(sizeof(bvh::WorldInstanceGPU), s.scene_instance_count));
  set_srv(4u, shared_resource(true, kRt12RtgiDepth), Rt12TextureSrv(DXGI_FORMAT_R32_FLOAT));
  set_srv(5u, s.arenas[kRt12ArenaUvs].buffer.Get(), Rt12BufferSrv(kRt12ArenaStride[kRt12ArenaUvs], s.arenas[kRt12ArenaUvs].used));
  D3D12_SHADER_RESOURCE_VIEW_DESC atlas_srv = {};
  atlas_srv.Format = DXGI_FORMAT_R32_UINT;
  atlas_srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
  atlas_srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  atlas_srv.Texture2DArray.MipLevels = 1u;
  atlas_srv.Texture2DArray.ArraySize = kRt12AlphaSlices;
  set_srv(6u, s.atlas.texture12.Get(), atlas_srv);
  set_srv(7u, s.materials.Get(), Rt12BufferSrv(32u, kRt12AlphaSlices));
  set_srv(8u, shared_resource(true, kRt12RtgiNormal), Rt12TextureSrv(DXGI_FORMAT_R32G32B32A32_UINT));
  set_srv(9u, s.noise.texture12.Get(), Rt12TextureSrv(DXGI_FORMAT_R32G32B32A32_UINT));
  set_srv(10u, nullptr, Rt12TextureSrv(DXGI_FORMAT_R32G32B32A32_UINT));  // t19: not read by RTGI
  set_srv(11u, shared_resource(radiance_on, kRt12RtgiRadiance), Rt12TextureSrv(DXGI_FORMAT_R16G16B16A16_FLOAT));
  set_srv(12u, shared_resource(prev_on, kRt12RtgiPrevFinal), Rt12TextureSrv(DXGI_FORMAT_R16G16B16A16_FLOAT));
  set_srv(13u, shared_resource(albedo_on, kRt12RtgiAlbedo), Rt12TextureSrv(DXGI_FORMAT_R16G16B16A16_FLOAT));
  // Lamps, lamp types and deferred: structured buffers on the D3D12 DEFAULT buffers (strides 112, 4 and 144).
  set_srv(14u, lamps_on ? r.lamps.Get() : nullptr, Rt12BufferSrv(kRt12RtgiLampTexels * 16u, lamp_elements));
  set_srv(15u, lamps_on ? r.lamp_types.Get() : nullptr, Rt12BufferSrv(4u, rtgi::kRtgiLampCount));
  set_srv(16u, shared_resource(mrt1_on, kRt12RtgiMrt1), Rt12TextureSrv(DXGI_FORMAT_R32G32B32A32_UINT));
  set_srv(17u, shared_resource(shadow_on, kRt12RtgiShadow), Rt12TextureSrv(DXGI_FORMAT_R32_FLOAT));
  set_srv(18u, shared_resource(mrt2_on, kRt12RtgiMrt2), Rt12TextureSrv(DXGI_FORMAT_R32G32_UINT));
  set_srv(19u, deferred_on ? r.deferred.Get() : nullptr, Rt12BufferSrv(kRt12RtgiDeferredTexels * 16u, deferred_elements));
  D3D12_SHADER_RESOURCE_VIEW_DESC tlas_srv = {};
  tlas_srv.Format = DXGI_FORMAT_UNKNOWN;
  tlas_srv.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
  tlas_srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  tlas_srv.RaytracingAccelerationStructure.Location = scene.tlas ? scene.tlas->GetGPUVirtualAddress() : 0u;
  set_srv(20u, nullptr, tlas_srv);
  set_srv(21u, dynamic_on ? s.dynamic_vertices.Get() : nullptr,
          Rt12BufferSrv(16u, uint64_t{kRt12CarrierWidth} * kRt12CarrierHeight));
  set_uav(22u, debug_on ? r.shared[kRt12RtgiDebug].texture12.Get() : nullptr, Rt12TextureUav(DXGI_FORMAT_R8G8B8A8_UNORM));
  set_uav(23u, r.shared[kRt12RtgiStats].texture12.Get(), Rt12TextureUav(DXGI_FORMAT_R32_UINT));
  set_uav(24u, r.shared[kRt12RtgiRaw].texture12.Get(), Rt12TextureUav(DXGI_FORMAT_R16G16B16A16_FLOAT));
  const D3D12_UNORDERED_ACCESS_VIEW_DESC stats_clear_uav = Rt12TextureUav(DXGI_FORMAT_R32_UINT);
  device12->CreateUnorderedAccessView(r.shared[kRt12RtgiStats].texture12.Get(), nullptr, &stats_clear_uav,
                                      r.cpu_heap->GetCPUDescriptorHandleForHeapStart());

  // Barriers into the read and write states, then the stats clear and the trace.
  const std::vector<ID3D12Resource*> reads = {
      s.arenas[kRt12ArenaVertices].buffer.Get(), s.arenas[kRt12ArenaIndices].buffer.Get(), s.arenas[kRt12ArenaUvs].buffer.Get(),
      s.mesh_table.Get(), scene.instance_table.Get(), s.materials.Get(), s.atlas.texture12.Get(), s.noise.texture12.Get(),
      shared_resource(true, kRt12RtgiDepth), shared_resource(true, kRt12RtgiNormal), shared_resource(radiance_on, kRt12RtgiRadiance),
      shared_resource(prev_on, kRt12RtgiPrevFinal), shared_resource(albedo_on, kRt12RtgiAlbedo), shared_resource(mrt1_on, kRt12RtgiMrt1),
      shared_resource(shadow_on, kRt12RtgiShadow), shared_resource(mrt2_on, kRt12RtgiMrt2),
      dynamic_on ? s.dynamic_vertices.Get() : nullptr,
  };
  const std::vector<ID3D12Resource*> writes = {
      r.shared[kRt12RtgiStats].texture12.Get(), r.shared[kRt12RtgiRaw].texture12.Get(), shared_resource(debug_on, kRt12RtgiDebug),
  };
  barriers.clear();
  for (ID3D12Resource* resource : reads) {
    if (resource) Transition(barriers, resource, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
  }
  for (ID3D12Resource* resource : writes) {
    if (resource) Transition(barriers, resource, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
  }
  Transition(barriers, r.scene_cb.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
  Transition(barriers, r.shared[kRt12RtgiScene].texture12.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COMMON);
  if (lamps_on) {
    Transition(barriers, r.shared[kRt12RtgiLamps].texture12.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COMMON);
    Transition(barriers, r.shared[kRt12RtgiLampTypes].texture12.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COMMON);
    Transition(barriers, r.lamps.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    Transition(barriers, r.lamp_types.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
  }
  if (deferred_on) {
    Transition(barriers, r.shared[kRt12RtgiDeferred].texture12.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COMMON);
    Transition(barriers, r.deferred.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
  }
  list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
  ID3D12DescriptorHeap* heaps[] = {r.heap.Get()};
  list->SetDescriptorHeaps(1u, heaps);
  const UINT zeros[4] = {0u, 0u, 0u, 0u};
  list->ClearUnorderedAccessViewUint(gpu_at(base + 23u), r.cpu_heap->GetCPUDescriptorHandleForHeapStart(),
                                     r.shared[kRt12RtgiStats].texture12.Get(), zeros, 0u, nullptr);
  barriers.clear();
  UavBarrier(barriers, r.shared[kRt12RtgiStats].texture12.Get());
  list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());

  // The trace: characters are traced (t30) only with this frame's character instances in the TLAS.
  rtgi::RtgiPushConstants gpu_push = push;
  gpu_push.sun[1] = (in.trace_characters && s.scene_dynamic_count > 0u) ? static_cast<float>(s.scene_dynamic_count) : 0.f;
  list->SetComputeRootSignature(r.root.Get());
  list->SetPipelineState(r.pso.Get());
  list->SetComputeRootConstantBufferView(0u, r.scene_cb->GetGPUVirtualAddress());
  list->SetComputeRoot32BitConstants(1u, rtgi::kRtgiPushConstantCount, &gpu_push, 0u);
  list->SetComputeRootDescriptorTable(2u, gpu_at(base));
  list->Dispatch((tw + 7u) / 8u, (th + 7u) / 8u, 1u);

  // Back to COMMON.
  barriers.clear();
  for (ID3D12Resource* resource : writes) {
    if (resource) Transition(barriers, resource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON);
  }
  for (ID3D12Resource* resource : reads) {
    if (resource) Transition(barriers, resource, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COMMON);
  }
  Transition(barriers, r.scene_cb.Get(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_COMMON);
  if (lamps_on) {
    Transition(barriers, r.lamps.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COMMON);
    Transition(barriers, r.lamp_types.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COMMON);
  }
  if (deferred_on) {
    Transition(barriers, r.deferred.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COMMON);
  }
  list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
  list->EndQuery(r.queries.Get(), D3D12_QUERY_TYPE_TIMESTAMP, base_q + 2u);
  list->ResolveQueryData(r.queries.Get(), D3D12_QUERY_TYPE_TIMESTAMP, base_q, kRt12RtgiTimestamps, r.query_readback.Get(),
                         static_cast<uint64_t>(base_q) * 8u);
  if (FAILED(list->Close())) {
    FailSession(s, Rt12Reason::SubmitFailed, "rtgi close");
    return RtgiFallback(Rt12RtgiReason::SubmitFailed);
  }

  // 8. Submission: wait for signal A and the previous submission, execute, signal B.
  bool ordered = false;
  try {
    ordered = renodx::utils::gpu_fence::QueueWait(s.queue12.Get(), s.fence11.get(), a);
    if (ordered && s.last_signal != 0u) {
      ordered = renodx::utils::gpu_fence::QueueWait(s.queue12.Get(), s.fence12.get(), s.last_signal);
    }
  } catch (const std::exception&) {
    ordered = false;
  }
  if (!ordered) {
    FailSession(s, Rt12Reason::SubmitFailed, "rtgi order wait");
    return RtgiFallback(Rt12RtgiReason::SubmitFailed);
  }
  ID3D12CommandList* lists[] = {list};
  s.queue12->ExecuteCommandLists(1u, lists);
  uint64_t b = renodx::utils::gpu_fence::NO_SIGNAL;
  try {
    b = renodx::utils::gpu_fence::QueueSignal(s.queue12.Get(), s.fence12.get());
  } catch (const std::exception&) {
    b = renodx::utils::gpu_fence::NO_SIGNAL;
  }
  if (b == renodx::utils::gpu_fence::NO_SIGNAL) {
    FailSession(s, Rt12Reason::SubmitFailed, "rtgi signal");
    return RtgiFallback(Rt12RtgiReason::SubmitFailed);
  }
  slot.fence_value = b;
  for (size_t i = retired_begin; i < s.retired.size(); ++i) s.retired[i].fence_value = b;
  s.last_signal = b;
  s.last_signal_qpc = QpcNow();
  slot.timestamps = true;
  r.next = (k + 1u) % kRt12Slots;

  // 9. D3D11 waits for B (GPU wait) before the copy back.
  bool waited = false;
  try {
    const bvh::PoolCpuTimer wait_timer{&bvh::g_perf_gate.block_us[bvh::kPerfBlockRt12WaitB]};
    waited = renodx::utils::gpu_fence::QueueWait(s.ctx11.Get(), s.fence12.get(), b);
  } catch (const std::exception&) {
    waited = false;
  }
  if (!waited) {
    FailSession(s, Rt12Reason::SubmitFailed, "rtgi d3d11 wait on fence B");
    return RtgiFallback(Rt12RtgiReason::SubmitFailed);
  }

  // 10. D3D11 copy back (raw, and the debug texture when set), then the stats resolve into the RTGI stats buffer.
  s.ctx11->CopyResource(reinterpret_cast<ID3D11Resource*>(in.raw_texture.handle),  // NOLINT(performance-no-int-to-ptr)
                        r.shared[kRt12RtgiRaw].texture11.Get());
  if (debug_on) {
    s.ctx11->CopyResource(reinterpret_cast<ID3D11Resource*>(in.debug_texture.handle),  // NOLINT(performance-no-int-to-ptr)
                          r.shared[kRt12RtgiDebug].texture11.Get());
  }
  {
    const reshade::api::resource_view srvs[1] = {r.shared[kRt12RtgiStats].srv11};
    const reshade::api::resource_view uavs[1] = {in.stats_uav};
    bind_pass(r.resolve, 1u, srvs, 1u, uavs);
    cmd_list->dispatch(1u, 1u, 1u);
    rtao::NullComputeSlots(cmd_list, 1u, 1u, 0u);
    null_tables(r.resolve, 1u, 1u);
  }

  // 11. Panel: reason Tracing and the traced frame count.
  std::lock_guard lock(g_rt12_rtgi_panel_mutex);
  g_rt12_rtgi_panel.reason = Rt12RtgiReason::Tracing;
  g_rt12_rtgi_panel.frames[static_cast<uint32_t>(Rt12RtgiReason::Tracing)] += 1u;
  g_rt12_rtgi_panel.characters = in.trace_characters ? s.scene_dynamic_count : 0u;
  return true;
#else
  return false;
#endif
}

}  // namespace falcom_world::rt12
