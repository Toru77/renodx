/*
 * Copyright (C) 2026 Carlos Lopez, speedlemur
 * SPDX-License-Identifier: MIT
 *
 * forzahorizon6-rr: real DLSS-RR guide buffers (M3a, experimental).
 *
 * Captures the game's live G-buffer render targets and generates the RR guide
 * buffers from them:
 *   - NormalRoughness: decoded from the packed normal target (R32_UINT, the
 *     12+12-bit packing used by the game's own resolve chain) plus a
 *     roughness candidate from the material target, by our own compute pass
 *     (fh6_guide_nr.cs_6_6.hlsl) dispatched on the game's command list right
 *     before the RR evaluate.
 *   - Albedo: linearized copy written by the same pass (DLSS-RR requires
 *     linear albedo; the captured G-buffer target is sRGB and unsupported).
 *   - SpecularAlbedo: generated in the same pass from albedo (approximation;
 *     no first-class specular target has been confirmed live yet).
 *
 * Capture is keyed on the live render-target FORMAT SIGNATURE, not on shader
 * hashes — the writer shaders are recompiled per game build/quality tier and
 * their hashes drift (this was the initial "captures 0" cause). Verified live
 * from a garage snapshot via DevKit:
 *   RT2 r8g8b8a8_uint  = material bits
 *   RT3 r32_uint       = packed normal bits
 *   RT4 r8g8b8a8_srgb  = albedo (image-confirmed: car paint)
 *
 * Everything fails closed: any missing capture, format mismatch or pipeline
 * failure keeps the placeholder guides in charge for that frame.
 */

#pragma once

#include <windows.h>
#include <d3d12.h>

#include <array>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <map>
#include <mutex>
#include <sstream>
#include <string>

#include <include/reshade.hpp>
#include <embed/shaders.h>

#include "../../utils/shader.hpp"

#pragma comment(lib, "d3d12.lib")

namespace guides {

// Live switches (settings-driven, read every evaluate).
inline std::atomic<bool> enabled = false;             // real guides on/off
inline std::atomic<bool> normals_view_space = true;   // world -> view transform
inline std::atomic<int32_t> roughness_source = 0;     // 0 = material.y, 1 = material.z, 2 = constant

// Diagnostics for the report.
inline std::atomic<uint32_t> captures = 0;
inline std::atomic<bool> signature_seen = false;
inline std::atomic<bool> nr_pipeline_ready = false;
inline std::atomic<uint32_t> nr_dispatches = 0;
inline std::atomic<bool> last_ready = false;

// Stage reporting: 0 = off, 1 = waiting capture, 2 = texture failed,
// 3 = pipeline failed, 4 = dispatching.
inline std::atomic<int32_t> last_stage = 0;
inline std::atomic<long> last_hr = 0;

inline const char* StageName(int32_t stage) {
  switch (stage) {
    case 0: return "off";
    case 1: return "waiting for G-buffer capture";
    case 2: return "guide texture creation failed";
    case 3: return "guide pipeline creation failed";
    case 4: return "dispatching";
    default: return "unknown";
  }
}

inline void SetStage(int32_t stage, long hr) {
  const int32_t previous = last_stage.exchange(stage);
  last_hr.store(hr);
  if (previous == stage) return;
  std::stringstream s;
  s << "guides: " << StageName(stage);
  if (hr != 0) {
    char buffer[64] = {};
    snprintf(buffer, sizeof buffer, " (hr=0x%08lX)", static_cast<unsigned long>(hr));
    s << buffer;
  }
  reshade::log::message(reshade::log::level::info, s.str().c_str());
}

struct CapturedTexture {
  std::atomic<uint64_t> handle{0};
  std::atomic<uint32_t> format{0};
  std::atomic<uint32_t> width{0};
  std::atomic<uint32_t> height{0};
  std::atomic<uint32_t> changes{0};
};

inline CapturedTexture captured_albedo;    // RT4 r8g8b8a8_srgb
inline CapturedTexture captured_material;  // RT2 r8g8b8a8_uint
inline CapturedTexture captured_normal;    // RT3 r32_uint

inline void StoreCapture(
    reshade::api::device* device, reshade::api::resource resource,
    CapturedTexture& out, const char* name) {
  auto* native = reinterpret_cast<ID3D12Resource*>(resource.handle);
  if (native == nullptr) return;
  const D3D12_RESOURCE_DESC native_desc = native->GetDesc();
  if (out.handle.exchange(resource.handle) != resource.handle) {
    // Per-frame double-buffered G-buffer sets make the handle flip often;
    // only the first few changes are log-worthy.
    if (out.changes.fetch_add(1) < 4) {
      std::stringstream s;
      s << "guides: captured " << name << " 0x" << std::hex
        << static_cast<unsigned long long>(resource.handle) << std::dec
        << " fmt=" << static_cast<uint32_t>(native_desc.Format) << " "
        << native_desc.Width << "x" << native_desc.Height;
      reshade::log::message(reshade::log::level::info, s.str().c_str());
    }
  }
  out.format.store(static_cast<uint32_t>(native_desc.Format));
  out.width.store(static_cast<uint32_t>(native_desc.Width));
  out.height.store(native_desc.Height);
}

// Capture the live G-buffer from a bound render-target list, identified by
// its format signature. Accepts both event shapes (bind + render pass).
inline void CaptureFromViews(
    reshade::api::command_list* cmd_list, const reshade::api::resource_view* views,
    uint32_t count) {
  if (!enabled.load() || views == nullptr || count < 8) return;
  auto* device = cmd_list->get_device();
  if (device == nullptr) return;

  const auto resolve = [&](uint32_t index) -> reshade::api::resource {
    if (views[index].handle == 0u) return {0};
    return device->get_resource_from_view(views[index]);
  };
  const reshade::api::resource albedo = resolve(4);
  const reshade::api::resource material = resolve(2);
  const reshade::api::resource normal = resolve(3);
  if (albedo.handle == 0u || material.handle == 0u || normal.handle == 0u) return;

  const reshade::api::resource_desc albedo_desc = device->get_resource_desc(albedo);
  const reshade::api::resource_desc material_desc = device->get_resource_desc(material);
  const reshade::api::resource_desc normal_desc = device->get_resource_desc(normal);
  if (albedo_desc.type != reshade::api::resource_type::texture_2d
      || material_desc.type != reshade::api::resource_type::texture_2d
      || normal_desc.type != reshade::api::resource_type::texture_2d) {
    return;
  }
  if (albedo_desc.texture.width < 320u || albedo_desc.texture.height < 180u) return;
  if (albedo_desc.texture.width != material_desc.texture.width
      || albedo_desc.texture.height != material_desc.texture.height
      || albedo_desc.texture.width != normal_desc.texture.width
      || albedo_desc.texture.height != normal_desc.texture.height) {
    return;
  }
  if (albedo_desc.texture.format != reshade::api::format::r8g8b8a8_unorm_srgb
      || material_desc.texture.format != reshade::api::format::r8g8b8a8_uint
      || normal_desc.texture.format != reshade::api::format::r32_uint) {
    return;
  }

  signature_seen.store(true);
  StoreCapture(device, albedo, captured_albedo, "albedo(rt4)");
  StoreCapture(device, material, captured_material, "material(rt2)");
  StoreCapture(device, normal, captured_normal, "normal(rt3)");
  captures.fetch_add(1);
}

inline void OnBindRenderTargetsAndDepthStencil(
    reshade::api::command_list* cmd_list, uint32_t count,
    const reshade::api::resource_view* rtvs, reshade::api::resource_view dsv) {
  (void)dsv;
  CaptureFromViews(cmd_list, rtvs, count);
}

inline void OnBeginRenderPass(
    reshade::api::command_list* cmd_list, uint32_t count,
    const reshade::api::render_pass_render_target_desc* rts,
    const reshade::api::render_pass_depth_stencil_desc* ds) {
  (void)ds;
  if (rts == nullptr || count < 8) return;
  std::array<reshade::api::resource_view, 8> views{};
  const uint32_t n = count < 8u ? count : 8u;
  for (uint32_t i = 0; i < n; ++i) views[i] = rts[i].view;
  CaptureFromViews(cmd_list, views.data(), n);
}

// --- NormalRoughness + specular guide pass (native D3D12) ------------------

struct GuidePass {
  ID3D12Resource* normal_roughness = nullptr;
  D3D12_RESOURCE_STATES normal_roughness_state = D3D12_RESOURCE_STATE_COMMON;
  ID3D12Resource* specular = nullptr;
  D3D12_RESOURCE_STATES specular_state = D3D12_RESOURCE_STATE_COMMON;
  ID3D12Resource* albedo_linear = nullptr;
  D3D12_RESOURCE_STATES albedo_linear_state = D3D12_RESOURCE_STATE_COMMON;
  uint32_t width = 0;
  uint32_t height = 0;
  ID3D12RootSignature* root_signature = nullptr;
  ID3D12PipelineState* pso = nullptr;
  // Typed texture access is not allowed through root descriptors, so all six
  // bindings live in a shader-visible descriptor heap (validated offline on
  // WARP: only descriptor tables satisfy these resource declarations).
  // Because the game double-buffers its G-buffer, descriptors are written
  // once per distinct resource set into 6-slot groups and never rewritten,
  // which keeps in-flight frames safe.
  ID3D12DescriptorHeap* heap = nullptr;
  UINT heap_increment = 0;
  uint32_t next_group = 0;
  // (normal, material, albedo, nr, spec, albedo_linear) -> group index
  std::map<std::array<uint64_t, 6>, uint32_t> groups;
  bool init_failed = false;
};

inline GuidePass guide_pass;
inline std::atomic<long> last_texture_hr{0};
inline std::atomic<long> last_pipeline_hr{0};

inline bool EnsureGuideTextures(ID3D12Device* device, uint32_t width, uint32_t height) {
  if (guide_pass.normal_roughness != nullptr && guide_pass.width == width
      && guide_pass.height == height) {
    return true;
  }
  if (guide_pass.normal_roughness != nullptr) {
    guide_pass.normal_roughness->Release();
    guide_pass.normal_roughness = nullptr;
    guide_pass.normal_roughness_state = D3D12_RESOURCE_STATE_COMMON;
  }
  if (guide_pass.specular != nullptr) {
    guide_pass.specular->Release();
    guide_pass.specular = nullptr;
    guide_pass.specular_state = D3D12_RESOURCE_STATE_COMMON;
  }
  if (guide_pass.albedo_linear != nullptr) {
    guide_pass.albedo_linear->Release();
    guide_pass.albedo_linear = nullptr;
    guide_pass.albedo_linear_state = D3D12_RESOURCE_STATE_COMMON;
  }

  D3D12_HEAP_PROPERTIES heap{};
  heap.Type = D3D12_HEAP_TYPE_DEFAULT;

  D3D12_RESOURCE_DESC nr_desc{};
  nr_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
  nr_desc.Width = width;
  nr_desc.Height = height;
  nr_desc.DepthOrArraySize = 1;
  nr_desc.MipLevels = 1;
  nr_desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
  nr_desc.SampleDesc.Count = 1;
  nr_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
  const HRESULT nr_hr = device->CreateCommittedResource(
      &heap, D3D12_HEAP_FLAG_NONE, &nr_desc, D3D12_RESOURCE_STATE_COMMON,
      nullptr, IID_PPV_ARGS(&guide_pass.normal_roughness));
  if (FAILED(nr_hr) || guide_pass.normal_roughness == nullptr) {
    guide_pass.normal_roughness = nullptr;
    last_texture_hr.store(nr_hr);
    return false;
  }

  D3D12_RESOURCE_DESC spec_desc{};
  spec_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
  spec_desc.Width = width;
  spec_desc.Height = height;
  spec_desc.DepthOrArraySize = 1;
  spec_desc.MipLevels = 1;
  spec_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  spec_desc.SampleDesc.Count = 1;
  spec_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
  const HRESULT spec_hr = device->CreateCommittedResource(
      &heap, D3D12_HEAP_FLAG_NONE, &spec_desc, D3D12_RESOURCE_STATE_COMMON,
      nullptr, IID_PPV_ARGS(&guide_pass.specular));
  if (FAILED(spec_hr) || guide_pass.specular == nullptr) {
    last_texture_hr.store(spec_hr);
    guide_pass.normal_roughness->Release();
    guide_pass.normal_roughness = nullptr;
    guide_pass.specular = nullptr;
    return false;
  }

  D3D12_RESOURCE_DESC alb_desc{};
  alb_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
  alb_desc.Width = width;
  alb_desc.Height = height;
  alb_desc.DepthOrArraySize = 1;
  alb_desc.MipLevels = 1;
  alb_desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
  alb_desc.SampleDesc.Count = 1;
  alb_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
  const HRESULT alb_hr = device->CreateCommittedResource(
      &heap, D3D12_HEAP_FLAG_NONE, &alb_desc, D3D12_RESOURCE_STATE_COMMON,
      nullptr, IID_PPV_ARGS(&guide_pass.albedo_linear));
  if (FAILED(alb_hr) || guide_pass.albedo_linear == nullptr) {
    last_texture_hr.store(alb_hr);
    guide_pass.normal_roughness->Release();
    guide_pass.normal_roughness = nullptr;
    guide_pass.specular->Release();
    guide_pass.specular = nullptr;
    guide_pass.albedo_linear = nullptr;
    return false;
  }

  guide_pass.width = width;
  guide_pass.height = height;
  guide_pass.normal_roughness_state = D3D12_RESOURCE_STATE_COMMON;
  guide_pass.specular_state = D3D12_RESOURCE_STATE_COMMON;
  guide_pass.albedo_linear_state = D3D12_RESOURCE_STATE_COMMON;

  if (guide_pass.heap == nullptr) {
    D3D12_DESCRIPTOR_HEAP_DESC heap_desc{};
    heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heap_desc.NumDescriptors = 64;  // 10 groups of 6 descriptors
    heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    const HRESULT heap_hr = device->CreateDescriptorHeap(
        &heap_desc, IID_PPV_ARGS(&guide_pass.heap));
    if (FAILED(heap_hr) || guide_pass.heap == nullptr) {
      guide_pass.heap = nullptr;
      last_texture_hr.store(heap_hr);
      return false;
    }
    guide_pass.heap_increment = device->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    guide_pass.next_group = 0;
    guide_pass.groups.clear();
  }
  return true;
}

inline bool EnsureGuidePipeline(ID3D12Device* device) {
  if (guide_pass.pso != nullptr && guide_pass.root_signature != nullptr) return true;
  if (guide_pass.init_failed) return false;

  D3D12_DESCRIPTOR_RANGE ranges[2] = {};
  ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
  ranges[0].NumDescriptors = 3;  // t0..t2
  ranges[0].BaseShaderRegister = 0;
  ranges[0].RegisterSpace = 0;
  ranges[0].OffsetInDescriptorsFromTableStart = 0;
  ranges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
  ranges[1].NumDescriptors = 3;  // u0..u2
  ranges[1].BaseShaderRegister = 0;
  ranges[1].RegisterSpace = 0;
  ranges[1].OffsetInDescriptorsFromTableStart = 3;

  D3D12_ROOT_PARAMETER params[2] = {};
  params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
  params[0].Constants.ShaderRegister = 0;
  params[0].Constants.RegisterSpace = 0;
  params[0].Constants.Num32BitValues = 16;
  params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
  params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
  params[1].DescriptorTable.NumDescriptorRanges = 2;
  params[1].DescriptorTable.pDescriptorRanges = ranges;
  params[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

  D3D12_ROOT_SIGNATURE_DESC root_desc{};
  root_desc.NumParameters = 2;
  root_desc.pParameters = params;
  root_desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

  ID3DBlob* signature = nullptr;
  ID3DBlob* error = nullptr;
  const HRESULT serialize = D3D12SerializeRootSignature(
      &root_desc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
  if (error != nullptr) error->Release();
  if (FAILED(serialize) || signature == nullptr) {
    guide_pass.init_failed = true;
    last_pipeline_hr.store(serialize);
    return false;
  }
  const HRESULT created = device->CreateRootSignature(
      0, signature->GetBufferPointer(), signature->GetBufferSize(),
      IID_PPV_ARGS(&guide_pass.root_signature));
  signature->Release();
  if (FAILED(created) || guide_pass.root_signature == nullptr) {
    guide_pass.init_failed = true;
    last_pipeline_hr.store(created);
    return false;
  }

  D3D12_COMPUTE_PIPELINE_STATE_DESC pso_desc{};
  pso_desc.pRootSignature = guide_pass.root_signature;
  pso_desc.CS.pShaderBytecode = __fh6_guide_nr.data();
  pso_desc.CS.BytecodeLength = __fh6_guide_nr.size();
  const HRESULT pso_created = device->CreateComputePipelineState(
      &pso_desc, IID_PPV_ARGS(&guide_pass.pso));
  if (FAILED(pso_created) || guide_pass.pso == nullptr) {
    guide_pass.init_failed = true;
    last_pipeline_hr.store(pso_created);
    return false;
  }
  nr_pipeline_ready.store(true);
  return true;
}

struct ReadyGuides {
  bool real = false;
  void* normal_roughness = nullptr;
  void* albedo = nullptr;
  uint32_t albedo_format = 0;
  void* specular = nullptr;
  uint32_t specular_format = 0;
};

inline D3D12_RESOURCE_BARRIER TransitionBarrier(
    ID3D12Resource* resource, D3D12_RESOURCE_STATES before,
    D3D12_RESOURCE_STATES after) {
  D3D12_RESOURCE_BARRIER barrier{};
  barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  barrier.Transition.pResource = resource;
  barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  barrier.Transition.StateBefore = before;
  barrier.Transition.StateAfter = after;
  return barrier;
}

// Called from the evaluate hook before the guide tags are built. Dispatches
// the guide pass on the game's command list and returns the live guide
// resources, or a not-ready result so placeholders stay in charge.
inline ReadyGuides PrepareEvaluate(
    ID3D12Device* device, ID3D12GraphicsCommandList* cmd_list, uint32_t width,
    uint32_t height, const float world_to_view_rows[3][4]) {
  ReadyGuides ready{};
  last_ready.store(false);
  if (!enabled.load() || device == nullptr || cmd_list == nullptr || width == 0
      || height == 0) {
    SetStage(0, 0);
    return ready;
  }

  const uint64_t albedo = captured_albedo.handle.load();
  const uint64_t material = captured_material.handle.load();
  const uint64_t normal = captured_normal.handle.load();
  if (normal == 0 || albedo == 0) {
    SetStage(1, 0);
    return ready;  // G-buffer targets not captured yet
  }

  if (!EnsureGuideTextures(device, width, height)) {
    SetStage(2, last_texture_hr.load());
    return ready;
  }
  if (!EnsureGuidePipeline(device)) {
    SetStage(3, last_pipeline_hr.load());
    return ready;
  }
  SetStage(4, 0);

  auto* gbuffer_albedo = reinterpret_cast<ID3D12Resource*>(albedo);
  auto* gbuffer_material = reinterpret_cast<ID3D12Resource*>(material);
  auto* gbuffer_normal = reinterpret_cast<ID3D12Resource*>(normal);

  uint32_t flags = 0;
  if (normals_view_space.load()) flags |= 1u;
  const int32_t source = roughness_source.load();
  if ((source == 0 || source == 1) && material != 0) {
    flags |= 2u;
    if (source == 1) flags |= 4u;
  }
  flags |= 16u;  // roughness = 1 - field/127 (writer fields are gloss-like)

  const D3D12_RESOURCE_STATES srv_state = static_cast<D3D12_RESOURCE_STATES>(
      D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
      | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

  D3D12_RESOURCE_BARRIER barriers[3] = {};
  uint32_t barrier_count = 0;
  if (guide_pass.normal_roughness_state != D3D12_RESOURCE_STATE_UNORDERED_ACCESS) {
    barriers[barrier_count++] = TransitionBarrier(
        guide_pass.normal_roughness, guide_pass.normal_roughness_state,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
  }
  if (guide_pass.specular_state != D3D12_RESOURCE_STATE_UNORDERED_ACCESS) {
    barriers[barrier_count++] = TransitionBarrier(
        guide_pass.specular, guide_pass.specular_state,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
  }
  if (guide_pass.albedo_linear_state != D3D12_RESOURCE_STATE_UNORDERED_ACCESS) {
    barriers[barrier_count++] = TransitionBarrier(
        guide_pass.albedo_linear, guide_pass.albedo_linear_state,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
  }
  cmd_list->ResourceBarrier(barrier_count, barriers);

  // Descriptor group: (normal, material, albedo, NR, spec, albedo_linear) -> 6
  // consecutive heap slots. Groups are written once and never rewritten, so
  // double-buffered G-buffer sets simply get their own groups.
  const std::array<uint64_t, 6> group_key = {
      normal,
      material,
      albedo,
      reinterpret_cast<uint64_t>(guide_pass.normal_roughness),
      reinterpret_cast<uint64_t>(guide_pass.specular),
      reinterpret_cast<uint64_t>(guide_pass.albedo_linear),
  };
  uint32_t group = 0;
  const auto existing = guide_pass.groups.find(group_key);
  if (existing != guide_pass.groups.end()) {
    group = existing->second;
  } else {
    group = guide_pass.next_group;
    guide_pass.next_group = (guide_pass.next_group + 1u) % 10u;  // 64/6 slots
    guide_pass.groups[group_key] = group;
    const D3D12_CPU_DESCRIPTOR_HANDLE heap_base =
        guide_pass.heap->GetCPUDescriptorHandleForHeapStart();
    const auto slot = [&](uint32_t index) {
      D3D12_CPU_DESCRIPTOR_HANDLE handle = heap_base;
      handle.ptr += static_cast<SIZE_T>(guide_pass.heap_increment) * (group * 6u + index);
      return handle;
    };
    device->CreateShaderResourceView(gbuffer_normal, nullptr, slot(0));
    if (material != 0) {
      device->CreateShaderResourceView(gbuffer_material, nullptr, slot(1));
    }
    device->CreateShaderResourceView(gbuffer_albedo, nullptr, slot(2));
    D3D12_UNORDERED_ACCESS_VIEW_DESC nr_uav{};
    nr_uav.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    nr_uav.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    device->CreateUnorderedAccessView(guide_pass.normal_roughness, nullptr, &nr_uav, slot(3));
    D3D12_UNORDERED_ACCESS_VIEW_DESC spec_uav{};
    spec_uav.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    spec_uav.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    device->CreateUnorderedAccessView(guide_pass.specular, nullptr, &spec_uav, slot(4));
    D3D12_UNORDERED_ACCESS_VIEW_DESC alb_uav{};
    alb_uav.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    alb_uav.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    device->CreateUnorderedAccessView(guide_pass.albedo_linear, nullptr, &alb_uav, slot(5));
  }

  cmd_list->SetComputeRootSignature(guide_pass.root_signature);
  cmd_list->SetPipelineState(guide_pass.pso);
  const uint32_t params[4] = {
      width, height, flags, 500u,  // constant roughness fallback = 0.5
  };
  cmd_list->SetComputeRoot32BitConstants(0, 4, params, 0);
  cmd_list->SetComputeRoot32BitConstants(0, 12, world_to_view_rows, 4);
  ID3D12DescriptorHeap* heaps[1] = {guide_pass.heap};
  cmd_list->SetDescriptorHeaps(1, heaps);
  D3D12_GPU_DESCRIPTOR_HANDLE table =
      guide_pass.heap->GetGPUDescriptorHandleForHeapStart();
  table.ptr += static_cast<UINT64>(guide_pass.heap_increment) * (group * 6u);
  cmd_list->SetComputeRootDescriptorTable(1, table);
  cmd_list->Dispatch((width + 7u) / 8u, (height + 7u) / 8u, 1u);

  D3D12_RESOURCE_BARRIER after[6] = {};
  after[0].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
  after[0].UAV.pResource = guide_pass.normal_roughness;
  after[1].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
  after[1].UAV.pResource = guide_pass.specular;
  after[2].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
  after[2].UAV.pResource = guide_pass.albedo_linear;
  after[3] = TransitionBarrier(
      guide_pass.normal_roughness, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, srv_state);
  after[4] = TransitionBarrier(
      guide_pass.specular, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, srv_state);
  after[5] = TransitionBarrier(
      guide_pass.albedo_linear, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, srv_state);
  cmd_list->ResourceBarrier(6, after);
  guide_pass.normal_roughness_state = srv_state;
  guide_pass.specular_state = srv_state;
  guide_pass.albedo_linear_state = srv_state;

  nr_dispatches.fetch_add(1);
  ready.real = true;
  ready.normal_roughness = guide_pass.normal_roughness;
  ready.albedo = guide_pass.albedo_linear;
  ready.albedo_format = static_cast<uint32_t>(DXGI_FORMAT_R16G16B16A16_FLOAT);
  ready.specular = guide_pass.specular;
  ready.specular_format = static_cast<uint32_t>(DXGI_FORMAT_R8G8B8A8_UNORM);
  last_ready.store(true);
  return ready;
}

inline void Use(DWORD fdw_reason) {
  renodx::utils::shader::Use(fdw_reason);
  switch (fdw_reason) {
    case DLL_PROCESS_ATTACH:
      reshade::register_event<reshade::addon_event::bind_render_targets_and_depth_stencil>(
          OnBindRenderTargetsAndDepthStencil);
      reshade::register_event<reshade::addon_event::begin_render_pass>(OnBeginRenderPass);
      break;
    case DLL_PROCESS_DETACH:
      reshade::unregister_event<reshade::addon_event::bind_render_targets_and_depth_stencil>(
          OnBindRenderTargetsAndDepthStencil);
      reshade::unregister_event<reshade::addon_event::begin_render_pass>(OnBeginRenderPass);
      break;
  }
}

inline std::string StatusLine() {
  std::stringstream s;
  s << "RR guides: " << (enabled.load() ? "real" : "placeholder");
  if (enabled.load()) {
    s << (last_ready.load() ? " (active)" : " (waiting for G-buffer capture)");
    s << " captures " << captures.load();
  }
  return s.str();
}

inline void AppendCaptured(std::stringstream& s, const char* name, const CapturedTexture& texture) {
  char buffer[160] = {};
  const uint64_t handle = texture.handle.load();
  if (handle == 0) {
    snprintf(buffer, sizeof buffer, "  %s: not captured\n", name);
    s << buffer;
    return;
  }
  snprintf(
      buffer, sizeof buffer, "  %s: 0x%llX fmt=%u %ux%u (changes %u)\n", name,
      static_cast<unsigned long long>(handle), texture.format.load(),
      texture.width.load(), texture.height.load(), texture.changes.load());
  s << buffer;
}

inline std::string BuildReportSection() {
  std::stringstream s;
  s << "\n[RT guides]\n";
  s << "  setting: " << (enabled.load() ? "real (experimental)" : "placeholder") << "\n";
  s << "  G-buffer signature seen: " << (signature_seen.load() ? "yes" : "no")
    << " (captures " << captures.load() << ")\n";
  AppendCaptured(s, "albedo  (rt4)", captured_albedo);
  AppendCaptured(s, "material(rt2)", captured_material);
  AppendCaptured(s, "normal  (rt3)", captured_normal);
  s << "  guide pass: " << (nr_pipeline_ready.load() ? "ready" : "not ready");
  if (guide_pass.width != 0) s << " (" << guide_pass.width << "x" << guide_pass.height << ")";
  s << ", dispatches " << nr_dispatches.load() << "\n";
  s << "  textures: NR=RGBA16F albedo=RGBA16F(linear) spec=RGBA8\n";
  s << "  stage: " << StageName(last_stage.load());
  if (last_hr.load() != 0) {
    char buffer[48] = {};
    snprintf(buffer, sizeof buffer, " (hr=0x%08lX)", static_cast<unsigned long>(last_hr.load()));
    s << buffer;
  }
  s << "\n";
  s << "  last evaluate: " << (last_ready.load() ? "real guides" : "placeholders") << "\n";
  s << "  flags: view normals " << (normals_view_space.load() ? "on" : "off")
    << ", roughness source " << roughness_source.load() << "\n";
  return s.str();
}

}  // namespace guides
