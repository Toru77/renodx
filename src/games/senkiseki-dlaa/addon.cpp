/*
 * Copyright (C) 2026
 * SPDX-License-Identifier: MIT
 *
 * Senkiseki3 DLAA — Replaces FXAA with NVIDIA DLAA (NGX SDK).
 * Features: camera jitter via VS injection, per-object motion vectors,
 *           depth-reprojection velocity compute, debug visualization.
 */

#define ImTextureID ImU64
#define DEBUG_LEVEL_0

#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

#include <deps/imgui/imgui.h>
#include <include/reshade.hpp>
#include <embed/shaders.h>

#include "../../mods/shader.hpp"
#include "../../utils/settings.hpp"
#include "../../utils/shader.hpp"
#include "./shared.h"
#include "./dlss/dlss.hpp"

namespace {

ShaderInjectData shader_injection = {
    .dlaa_enabled = 0.f, .dlaa_preset = 0.f,
    .dlaa_jitter_enabled = 0.f, .dlaa_jitter_sign = 0.f,
    .dlaa_jitter_test = 0.f,
    .dlaa_force_reset = 0.f, .dlaa_zero_mv = 0.f,
    .dlaa_mv_direction = 0.f, .dlaa_mv_threshold = 0.f,
    .dlaa_depth_source = 0.f,
    .dlaa_per_object_motion = 0.f,
    .dlaa_velocity_scale = 1.f, .dlaa_debug_view = 0.f,
    .dlaa_debug_scale = 50.f,
    .dlaa_debug_logging = 0.f,
    .dlaa_flag_is_hdr = 0.f, .dlaa_flag_depth_inverted = 1.f,
    .dlaa_flag_auto_exposure = 0.f,
    .jitter_offset_x = 0.f, .jitter_offset_y = 0.f,
    .dlaa_velocity_format = 0.f,
};

// ── Descriptor table helpers ──
constexpr uint32_t kTableParamCount = 5u;
using DTSet = std::array<reshade::api::descriptor_table, kTableParamCount>;

static void FreeTables(reshade::api::device* dev, DTSet& t) {
  for (auto& tb : t) { if (tb.handle) { dev->free_descriptor_table(tb); tb = {}; } }
}

// ── Per-device state ──
struct __declspec(uuid("b1a2c3d4-e5f6-7890-abcd-ef1234567890")) DeviceData {
  reshade::api::resource velocity_texture = {};
  reshade::api::resource_view velocity_srv = {};
  reshade::api::resource_view velocity_uav = {};
  reshade::api::format velocity_format = {};  // r16g16_float / r32g32_float (A/B toggle)
  reshade::api::pipeline_layout velocity_layout = {};
  reshade::api::pipeline velocity_pipeline = {};
  DTSet velocity_tables = {};

  reshade::api::buffer_range captured_scene_cbv = {};  // _Globals cbuffer at b0
  bool captured_scene_cbv_valid = false;
  reshade::api::resource_view captured_depth_srv = {};
  reshade::api::resource captured_depth_res = {};
  uint32_t depth_source_hash = 0u;  // PS hash that last pushed the captured depth (source identity)
  bool depth_primary_captured = false;  // a perspective (non-linear) depth was captured this frame
  reshade::api::resource_view captured_color_srv = {};
  reshade::api::resource captured_color_res = {};
  reshade::api::resource captured_rtv0_res = {};
  // Per-object motion: char G-buffer MRT2 (o2.zw = prevNDC, y-up UV)
  reshade::api::resource_view captured_motion_srv = {};
  reshade::api::resource captured_motion_res = {};
  reshade::api::resource last_rtv2_candidate = {};

  // Camera matrices for depth-projection velocity (read from _Globals CBV)
  std::array<float, 16> prev_view_proj = {};
  std::array<float, 16> curr_view_proj = {};
  std::array<float, 16> curr_view_proj_inv = {};
  bool matrices_valid = false;
  ID3D11Buffer* scene_cbv_staging = nullptr;
  uint32_t scene_cbv_staging_size = 0u;
  bool scene_cbv_copy_issued = false;  // a camera-matrix staging copy is queued this frame
  float viewport_w = 2560.f, viewport_h = 1440.f;
  float swapchain_w = 2560.f, swapchain_h = 1440.f;
  float jitter_x = 0.f, jitter_y = 0.f;
  uint32_t frame_index = 0u;
  bool resources_created = false;

  reshade::api::sampler point_sampler = {};

  // Previous-frame color scratch (reprojection debug mode)
  reshade::api::resource prev_color_texture = {};
  reshade::api::resource_view prev_color_srv = {};

  // Native MV-debug resources (bypass ReShade pipeline creation; display via NGX output)
  ID3D11ComputeShader* dbg_cs = nullptr;
  ID3D11UnorderedAccessView* dbg_out_uav = nullptr;
  ID3D11ShaderResourceView* dbg_vel_srv = nullptr;
  ID3D11ShaderResourceView* dbg_color_srv = nullptr;
  ID3D11ShaderResourceView* dbg_prev_srv = nullptr;
  ID3D11SamplerState* dbg_linear_sampler = nullptr;
  ID3D11Buffer* dbg_cb = nullptr;
};

// ── Halton jitter ──
static float Halton(uint32_t n, uint32_t base) {
  float r = 0.f, inv = 1.f / (float)base, f = 1.f;
  while (n) { f *= inv; r += f * (float)(n % base); n /= base; }
  return r;
}

// ── Destroy ──
static void Destroy(reshade::api::device* dev, DeviceData* d) {
  if (!dev || !d) return;
  auto dr = [&](reshade::api::resource& r) { if(r.handle) dev->destroy_resource(r); r={}; };
  auto dv = [&](reshade::api::resource_view& v) { if(v.handle) dev->destroy_resource_view(v); v={}; };
  auto dp = [&](reshade::api::pipeline& p) { if(p.handle) dev->destroy_pipeline(p); p={}; };
  auto dl = [&](reshade::api::pipeline_layout& l) { if(l.handle) dev->destroy_pipeline_layout(l); l={}; };

  dv(d->velocity_srv); dv(d->velocity_uav); dr(d->velocity_texture);
  dp(d->velocity_pipeline); dl(d->velocity_layout); FreeTables(dev, d->velocity_tables);
  d->captured_scene_cbv = {}; d->captured_scene_cbv_valid = false;
  dv(d->captured_depth_srv); dv(d->captured_color_srv);
  d->captured_depth_res = {}; d->captured_color_res = {};
  if (d->captured_motion_srv.handle) dev->destroy_resource_view(d->captured_motion_srv);
  d->captured_motion_srv = {}; d->captured_motion_res = {};
  d->last_rtv2_candidate = {};
  if (d->scene_cbv_staging) { d->scene_cbv_staging->Release(); d->scene_cbv_staging = nullptr; }
  d->scene_cbv_staging_size = 0u;
  d->scene_cbv_copy_issued = false;
  d->matrices_valid = false;
  if (d->point_sampler.handle) dev->destroy_sampler(d->point_sampler);
  d->point_sampler = {};
  dv(d->prev_color_srv); dr(d->prev_color_texture);
  if (d->dbg_cs) { d->dbg_cs->Release(); d->dbg_cs = nullptr; }
  if (d->dbg_out_uav) { d->dbg_out_uav->Release(); d->dbg_out_uav = nullptr; }
  if (d->dbg_vel_srv) { d->dbg_vel_srv->Release(); d->dbg_vel_srv = nullptr; }
  if (d->dbg_color_srv) { d->dbg_color_srv->Release(); d->dbg_color_srv = nullptr; }
  if (d->dbg_prev_srv) { d->dbg_prev_srv->Release(); d->dbg_prev_srv = nullptr; }
  if (d->dbg_linear_sampler) { d->dbg_linear_sampler->Release(); d->dbg_linear_sampler = nullptr; }
  if (d->dbg_cb) { d->dbg_cb->Release(); d->dbg_cb = nullptr; }
  d->resources_created = false;
}

// ── Create velocity pipeline ──
static bool CreateVelocityPipeline(reshade::api::device* dev, DeviceData* d) {
  if (!dev || !d || d->velocity_pipeline.handle) return true;

  reshade::api::sampler_desc sd = {};
  sd.filter = reshade::api::filter_mode::min_mag_mip_point;
  sd.address_u = sd.address_v = sd.address_w = reshade::api::texture_address_mode::clamp;
  dev->create_sampler(sd, &d->point_sampler);

  using DS = reshade::api::shader_stage;
  using DT = reshade::api::descriptor_type;
  using DR = reshade::api::descriptor_range;
  using P  = reshade::api::pipeline_layout_param;
  using PT = reshade::api::pipeline_layout_param_type;

  DR sampler_r = {0,0,0,1, DS::all_compute, 1, DT::sampler};
  DR cbv_r     = {0,0,0,1, DS::all_compute, 1, DT::constant_buffer};
  DR srv_r     = {0,0,0,1, DS::all_compute, 1, DT::texture_shader_resource_view};                    // t0 (depth)
  DR motion_r  = {0,1,0,1, DS::all_compute, 1, DT::texture_shader_resource_view};                    // t1 (per-object MV)
  DR uav_r     = {0,0,0,1, DS::all_compute, 1, DT::texture_unordered_access_view};
  reshade::api::constant_range pc_range = {};
  pc_range.binding = 0;
  pc_range.dx_register_index = 13;
  pc_range.count = 48;  // matches motion_velocity.cs_5_0.hlsl cbuffer (12 float4s)
  pc_range.visibility = DS::all_compute;

  P params[6];
  params[0].type = PT::descriptor_table; params[0].descriptor_table = {1, &sampler_r};
  params[1].type = PT::descriptor_table; params[1].descriptor_table = {1, &cbv_r};
  params[2].type = PT::descriptor_table; params[2].descriptor_table = {1, &srv_r};
  params[3].type = PT::descriptor_table; params[3].descriptor_table = {1, &motion_r};
  params[4].type = PT::descriptor_table; params[4].descriptor_table = {1, &uav_r};
  params[5].type = PT::push_constants;   params[5].push_constants = pc_range;

  if (!dev->create_pipeline_layout(6, params, &d->velocity_layout)) return false;
  for (uint32_t i = 0; i < 5u; ++i)
    if (!dev->allocate_descriptor_table(d->velocity_layout, i, &d->velocity_tables[i])) return false;

  reshade::api::shader_desc cs_desc = {};
  cs_desc.code = __motion_velocity.data();
  cs_desc.code_size = __motion_velocity.size();
  cs_desc.entry_point = "main";
  reshade::api::pipeline_subobject so = {reshade::api::pipeline_subobject_type::compute_shader, 1, &cs_desc};
  return dev->create_pipeline(d->velocity_layout, 1, &so, &d->velocity_pipeline);
}

// ── Native MV-debug resources ──
// Robust path: bypasses ReShade pipeline/descriptor-table creation and displays
// through the NGX output texture using the same native SRV bind as the DLAA
// output (which is proven to render on screen).
static bool EnsureDebugNative(reshade::api::device* dev, DeviceData* d) {
  auto* nd = reinterpret_cast<ID3D11Device*>(dev->get_native());
  if (!nd || !d) return false;

  if (!d->dbg_cs) {
    if (FAILED(nd->CreateComputeShader(__mv_debug.data(), static_cast<SIZE_T>(__mv_debug.size()), nullptr, &d->dbg_cs)))
      return false;
  }
  if (!d->dbg_out_uav) {
    D3D11_UNORDERED_ACCESS_VIEW_DESC uavd = {};
    uavd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    uavd.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
    uavd.Texture2D.MipSlice = 0;
    if (FAILED(nd->CreateUnorderedAccessView(senkiseki3::dlss::ngx.output_texture.Get(), &uavd, &d->dbg_out_uav)))
      return false;
  }
  if (!d->dbg_vel_srv) {
    D3D11_SHADER_RESOURCE_VIEW_DESC srvd = {};
    D3D11_TEXTURE2D_DESC vtd = {};
    reinterpret_cast<ID3D11Texture2D*>(d->velocity_texture.handle)->GetDesc(&vtd);
    srvd.Format = vtd.Format;  // r16g16_float or r32g32_float (follows the toggle)
    srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvd.Texture2D.MipLevels = 1;
    if (FAILED(nd->CreateShaderResourceView(reinterpret_cast<ID3D11Resource*>(d->velocity_texture.handle), &srvd, &d->dbg_vel_srv)))
      return false;
  }
  if (!d->dbg_color_srv) {
    D3D11_SHADER_RESOURCE_VIEW_DESC srvd = {};
    D3D11_TEXTURE2D_DESC td = {};
    reinterpret_cast<ID3D11Texture2D*>(d->captured_color_res.handle)->GetDesc(&td);
    srvd.Format = td.Format;
    srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvd.Texture2D.MipLevels = 1;
    if (FAILED(nd->CreateShaderResourceView(reinterpret_cast<ID3D11Resource*>(d->captured_color_res.handle), &srvd, &d->dbg_color_srv)))
      return false;
  }
  if (!d->dbg_prev_srv) {
    D3D11_SHADER_RESOURCE_VIEW_DESC srvd = {};
    srvd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvd.Texture2D.MipLevels = 1;
    if (FAILED(nd->CreateShaderResourceView(reinterpret_cast<ID3D11Resource*>(d->prev_color_texture.handle), &srvd, &d->dbg_prev_srv)))
      return false;
  }
  if (!d->dbg_linear_sampler) {
    D3D11_SAMPLER_DESC sd = {};
    sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sd.AddressU = sd.AddressV = sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    if (FAILED(nd->CreateSamplerState(&sd, &d->dbg_linear_sampler)))
      return false;
  }
  if (!d->dbg_cb) {
    D3D11_BUFFER_DESC bd = {};
    bd.ByteWidth = 16;
    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    if (FAILED(nd->CreateBuffer(&bd, nullptr, &d->dbg_cb)))
      return false;
  }
  return true;
}

// Ensure the previous-frame color scratch (R8G8B8A8 at viewport size).
static bool EnsurePrevColorTexture(reshade::api::device* dev, DeviceData* d, uint32_t w, uint32_t h) {
  if (!dev || !d || w == 0u || h == 0u) return false;
  if (d->prev_color_texture.handle) {
    auto rd = dev->get_resource_desc(d->prev_color_texture);
    if (rd.texture.width == w && rd.texture.height == h) return true;
    dev->destroy_resource_view(d->prev_color_srv); d->prev_color_srv = {};
    dev->destroy_resource(d->prev_color_texture); d->prev_color_texture = {};
  }
  reshade::api::resource_desc rd = {};
  rd.type = reshade::api::resource_type::texture_2d;
  rd.texture = {w, h, 1, 1, reshade::api::format::r8g8b8a8_unorm, 1};
  rd.heap = reshade::api::memory_heap::gpu_only;
  rd.usage = reshade::api::resource_usage::shader_resource;
  dev->create_resource(rd, nullptr, reshade::api::resource_usage::shader_resource, &d->prev_color_texture);
  reshade::api::resource_view_desc vd(reshade::api::resource_view_type::texture_2d, reshade::api::format::r8g8b8a8_unorm, 0, 1, 0, 1);
  dev->create_resource_view(d->prev_color_texture, reshade::api::resource_usage::shader_resource, vd, &d->prev_color_srv);
  return d->prev_color_texture.handle != 0u && d->prev_color_srv.handle != 0u;
}

// ── Event: capture RTV0 (FXAA output target) ──
static void OnBindRenderTargets(
    reshade::api::command_list* cmd_list, uint32_t count,
    const reshade::api::resource_view* rtvs, reshade::api::resource_view) {
  if (count < 1 || !rtvs || !rtvs[0].handle) return;
  auto* dev = cmd_list->get_device();
  if (!dev) return;
  auto* d = dev->get_private_data<DeviceData>();
  if (!d) return;
  d->captured_rtv0_res = dev->get_resource_from_view(rtvs[0]);
  // Candidate per-object motion MRT (RTV2) — adopted when char G-buffer draws
  if (count >= 3u && rtvs[2].handle) {
    d->last_rtv2_candidate = dev->get_resource_from_view(rtvs[2]);
  }
}

// ── Matrix helpers (row-major 4x4) ──
static void MulMat4(const float* a, const float* b, float* out) {
  for (int r = 0; r < 4; ++r) {
    for (int c = 0; c < 4; ++c) {
      float s = 0.f;
      for (int k = 0; k < 4; ++k) s += a[r * 4 + k] * b[k * 4 + c];
      out[r * 4 + c] = s;
    }
  }
}

// Gauss-Jordan 4x4 inverse (row-major). Returns false if singular.
static bool InvertMat4(const float* m, float* out) {
  float a[16], inv[16];
  for (int i = 0; i < 16; ++i) { a[i] = m[i]; inv[i] = (i % 5 == 0) ? 1.f : 0.f; }
  for (int col = 0; col < 4; ++col) {
    int pivot = col;
    float best = std::fabs(a[col * 4 + col]);
    for (int r = col + 1; r < 4; ++r) {
      float v = std::fabs(a[r * 4 + col]);
      if (v > best) { best = v; pivot = r; }
    }
    if (best < 1e-12f) return false;
    if (pivot != col) {
      for (int c = 0; c < 4; ++c) {
        std::swap(a[pivot * 4 + c], a[col * 4 + c]);
        std::swap(inv[pivot * 4 + c], inv[col * 4 + c]);
      }
    }
    const float d = a[col * 4 + col];
    for (int c = 0; c < 4; ++c) {
      a[col * 4 + c] /= d;
      inv[col * 4 + c] /= d;
    }
    for (int r = 0; r < 4; ++r) {
      if (r == col) continue;
      const float f = a[r * 4 + col];
      if (f == 0.f) continue;
      for (int c = 0; c < 4; ++c) {
        a[r * 4 + c] -= f * a[col * 4 + c];
        inv[r * 4 + c] -= f * inv[col * 4 + c];
      }
    }
  }
  for (int i = 0; i < 16; ++i) out[i] = inv[i];
  return true;
}

// Queue the GPU copy of the camera matrices (c10..c21) into the staging buffer
// EARLY — at the hash-gated scene b0 capture — so the Map in ReadSceneMatrices
// (at FXAA, end of frame) finds the copy already complete and does NOT drain the
// GPU pipeline. D3D11_MAP_READ on a just-copied staging buffer forces a full
// pipeline flush mid-frame (GPU utilization dropped to ~76% with copy-at-FXAA);
// issuing the copy at the first scene draw hides it behind the frame's GPU work.
static bool IssueSceneCbvCopy(reshade::api::command_list* cmd_list, DeviceData* d) {
  if (!cmd_list || !d || !d->captured_scene_cbv_valid) return false;
  auto* dev = cmd_list->get_device();
  if (!dev) return false;
  auto* cb = reinterpret_cast<ID3D11Buffer*>(d->captured_scene_cbv.buffer.handle);
  if (!cb) return false;

  // Read window must cover c10..c21 = 352 bytes.
  uint64_t range_size = d->captured_scene_cbv.size;
  if (range_size < 352ull) range_size = 352ull;
  if (range_size > 65536ull) range_size = 65536ull;
  auto bdesc = dev->get_resource_desc(d->captured_scene_cbv.buffer);
  if (bdesc.buffer.size > 0) {
    const uint64_t offset = d->captured_scene_cbv.offset;
    const uint64_t max_read = (offset < bdesc.buffer.size) ? (bdesc.buffer.size - offset) : 0u;
    if (range_size > max_read) range_size = max_read;
  }
  if (range_size < 352ull) return false;  // can't reach the matrices
  const uint32_t need = static_cast<uint32_t>(range_size);

  if (!d->scene_cbv_staging || d->scene_cbv_staging_size < need) {
    if (d->scene_cbv_staging) { d->scene_cbv_staging->Release(); d->scene_cbv_staging = nullptr; }
    D3D11_BUFFER_DESC bd = {};
    bd.ByteWidth = need;
    bd.Usage = D3D11_USAGE_STAGING;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    auto* nd = reinterpret_cast<ID3D11Device*>(dev->get_native());
    if (!nd || FAILED(nd->CreateBuffer(&bd, nullptr, &d->scene_cbv_staging)) || !d->scene_cbv_staging) return false;
    d->scene_cbv_staging_size = need;
  }

  auto* ctx = reinterpret_cast<ID3D11DeviceContext*>(cmd_list->get_native());
  if (!ctx) return false;
  D3D11_BOX box = {};
  box.left = static_cast<UINT>(d->captured_scene_cbv.offset);
  box.right = box.left + need;
  box.bottom = 1;
  box.back = 1;
  ctx->CopySubresourceRegion(d->scene_cbv_staging, 0, 0, 0, 0, cb, 0, &box);
  return true;
}

// Read the game's _Globals camera matrices (ViewProjection c10, ViewInverse c14,
// ProjectionInverse c18) via a staging copy of the captured scene CBV, then:
//   prev_view_proj     = last frame's ViewProjection
//   curr_view_proj     = this frame's ViewProjection
//   curr_view_proj_inv = ProjectionInverse * ViewInverse (= inverse(ViewProjection))
static bool ReadSceneMatrices(reshade::api::device* dev, DeviceData* d, ID3D11DeviceContext* ctx) {
  if (!dev || !d || !ctx || !d->captured_scene_cbv_valid) return false;
  auto* cb = reinterpret_cast<ID3D11Buffer*>(d->captured_scene_cbv.buffer.handle);
  if (!cb) return false;

  // Clamp the read window (must cover c10..c21 = 352 bytes).
  uint64_t range_size = d->captured_scene_cbv.size;
  if (range_size < 352ull) range_size = 352ull;
  if (range_size > 65536ull) range_size = 65536ull;
  auto bdesc = dev->get_resource_desc(d->captured_scene_cbv.buffer);
  if (bdesc.buffer.size > 0) {
    const uint64_t offset = d->captured_scene_cbv.offset;
    const uint64_t max_read = (offset < bdesc.buffer.size) ? (bdesc.buffer.size - offset) : 0u;
    if (range_size > max_read) range_size = max_read;
  }
  if (range_size < 352ull) return false;  // can't reach the matrices
  const uint32_t need = static_cast<uint32_t>(range_size);

  if (!d->scene_cbv_staging || d->scene_cbv_staging_size < need) {
    if (d->scene_cbv_staging) { d->scene_cbv_staging->Release(); d->scene_cbv_staging = nullptr; }
    D3D11_BUFFER_DESC bd = {};
    bd.ByteWidth = need;
    bd.Usage = D3D11_USAGE_STAGING;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    auto* nd = reinterpret_cast<ID3D11Device*>(dev->get_native());
    if (!nd || FAILED(nd->CreateBuffer(&bd, nullptr, &d->scene_cbv_staging)) || !d->scene_cbv_staging) return false;
    d->scene_cbv_staging_size = need;
  }

  if (!d->scene_cbv_copy_issued) {
    // Fallback: no early copy was queued this frame (e.g. DLAA toggled on
    // mid-frame). This path stalls the pipeline; the early copy in
    // OnPushDescriptorsCapture is the normal path.
    D3D11_BOX box = {};
    box.left = static_cast<UINT>(d->captured_scene_cbv.offset);
    box.right = box.left + need;
    box.bottom = 1;
    box.back = 1;
    ctx->CopySubresourceRegion(d->scene_cbv_staging, 0, 0, 0, 0, cb, 0, &box);
  }
  d->scene_cbv_copy_issued = false;  // consumed this frame

  D3D11_MAPPED_SUBRESOURCE mapped = {};
  if (FAILED(ctx->Map(d->scene_cbv_staging, 0, D3D11_MAP_READ, 0, &mapped))) return false;
  const float* data = static_cast<const float*>(mapped.pData);
  std::array<float, 16> view_proj, view_inv, proj_inv;
  memcpy(view_proj.data(), data + 160 / 4, 64);  // c10 ViewProjection
  memcpy(view_inv.data(),  data + 224 / 4, 64);  // c14 ViewInverse
  memcpy(proj_inv.data(),  data + 288 / 4, 64);  // c18 ProjectionInverse
  ctx->Unmap(d->scene_cbv_staging, 0);

  float sum = 0.f;
  for (float v : view_proj) sum += v;
  if (sum == 0.f) return false;  // uninitialized / zeroed matrix

  d->prev_view_proj = d->curr_view_proj;
  d->curr_view_proj = view_proj;
  // Invert the ACTUAL ViewProjection (c10) directly instead of composing the
  // game's separate ViewInverse/ProjectionInverse (c14/c18). If those stored
  // inverses don't exactly match the forward VP, every pixel gets a
  // depth-dependent reprojection error -> the static radial MV field (ghosting).
  // Inverting the real VP makes the unproject->reproject round trip exact, so a
  // static camera yields ~zero velocity on static content.
  if (!InvertMat4(view_proj.data(), d->curr_view_proj_inv.data())) return false;
  d->matrices_valid = true;
  // Inject the current frame's ViewProjection for the VS per-object path
  // (the replaced VS consumes it on the NEXT frame as prevViewProjection).
  for (int i = 0; i < 16; ++i) shader_injection.prev_view_proj[i] = d->curr_view_proj[i];
  return true;
}

// (Re)create the SRV for the per-object motion MRT (char G-buffer RTV2).
static void UpdateMotionSrv(reshade::api::device* dev, DeviceData* d, reshade::api::resource res) {
  if (!dev || !d || !res.handle) return;
  if (d->captured_motion_res.handle == res.handle && d->captured_motion_srv.handle) return;
  if (d->captured_motion_srv.handle) {
    dev->destroy_resource_view(d->captured_motion_srv);
    d->captured_motion_srv = {};
  }
  d->captured_motion_res = res;
  auto rd = dev->get_resource_desc(res);
  reshade::api::resource_view_desc vd(reshade::api::resource_view_type::texture_2d, rd.texture.format, 0, 1, 0, 1);
  dev->create_resource_view(res, reshade::api::resource_usage::shader_resource, vd, &d->captured_motion_srv);
}

// ── Scene-geometry vertex shader set (hash-gated) ──
// These are the VSs that rasterize the 3D scene into the 3-MRT G-buffer
// (captured live via the devkit; see tmp/senkiseki3/dump). Each one transforms
// with scene.ViewProjection (cb0 c10) from the per-object _Globals, so the
// jitter is applied directly in each replaced VS (boot/0xHASH.vs_4_1.hlsl).
//
// NOTE: b0 is a PER-OBJECT cbuffer (contains World at c44) — that is why the
// proxy-cbuffer jitter approach is unworkable (a shared proxy would give every
// object the first object's World matrix). Jitter must live in the VS.
static const std::array<uint32_t, 41> SCENE_GEOMETRY_VS_HASHES = {
    0x37F1DE22u,  // world/terrain (primary)
    0xCBF171E5u,  // world
    0xDF1D933Fu,  // world
    0x9BB882F5u,  // terrain tiles
    0x43ED1D83u,  // world
    0xC1F80CF6u,  // world
    0x8BB470CEu,  // world
    0x4E107313u,  // world (COLOR input)
    0x09394015u,  // unskinned characters/NPCs
    0x0D5DABC6u,  // skinned characters/NPCs
    0xB2F338C8u,  // skinned
    0xB1C24E2Au,  // skinned
    0x5C1A50E5u,  // character hair
    0x4A030C25u,  // character clothing
    0x3641D444u,  // eyeball
    0xC8F5D77Bu,  // character skin
    0xF8C9B92Du,  // character clothing
    0xB662509Au,  // character clothing
    // Foliage/world VSs discovered in the foliage scene:
    0x29513853u,  // foliage (main)
    0xED3D1A43u,  // foliage
    0x7D5282A3u,  // scene/world
    0x066E7DFBu,  // scene/world
    0x714E4C33u,  // scene/world
    0x7A711F41u,  // scene/world
    0x09BD12FAu,  // scene/world
    0x030AD345u,  // scene/world
    0x8913640Au,  // scene/world
    0x2DC04A66u,  // scene/world (G-buffer, 7 SRVs)
    0x34AA271Fu,  // scene/world (G-buffer)
    0xDFE5A75Du,  // scene/world (G-buffer)
    0x8ED5035Bu,  // scene/world (G-buffer)
    0x97E9A1ECu,  // scene/world (G-buffer)
    0x4D37FA49u,  // scene/world (G-buffer)
    0x9596CBC1u,  // scene/world (G-buffer)
    0xE4C6D6F4u,  // scene/world (G-buffer)
    0x8AFF0B4Fu,  // world-space effect (water/particle, RT=1)
    0x795F3AD3u,  // world-space effect (RT=1)
    0x5E5AE3FBu,  // character outline
    0x77355EEDu,  // forest impostor billboard (sky)
    0xC8FE8FC4u,  // transparent texture (world-space)
    0x7D3553A7u,  // particle (world-space)
};

static bool IsSceneGeometryVs(uint32_t hash) {
  for (uint32_t h : SCENE_GEOMETRY_VS_HASHES) {
    if (h == hash) return true;
  }
  return false;
}

// ── Event: capture all needed descriptors (falcomengine-plus pattern: type → binding → hash) ──
static void OnPushDescriptorsCapture(
    reshade::api::command_list* cmd_list, reshade::api::shader_stage stage, reshade::api::pipeline_layout,
    uint32_t param_index, const reshade::api::descriptor_table_update& update) {
  auto* dev = cmd_list->get_device();
  if (!dev) return;
  auto* d = dev->get_private_data<DeviceData>();
  if (!d) return;

  // ── SRV captures (color & depth) ──
  if (update.type == reshade::api::descriptor_type::texture_shader_resource_view) {
    auto* views = static_cast<const reshade::api::resource_view*>(update.descriptors);

    // Per-object motion MRT: adopt the latest RTV2 when the char G-buffer draws
    auto* motion_ss = renodx::utils::shader::GetCurrentState(cmd_list);
    uint32_t motion_hash = motion_ss ? renodx::utils::shader::GetCurrentPixelShaderHash(motion_ss) : 0u;
    if (motion_hash == 0x0E8BC215u) {
      UpdateMotionSrv(dev, d, d->last_rtv2_candidate);
    }

    // Diagnostic: sample first 5 pushes at bindings 0-4 to locate depth
    if (shader_injection.dlaa_debug_logging > 0.5f
        && update.binding <= 4u && update.count >= 1 && views[0].handle != 0u) {
      static int diag_count[5] = {};
      if (diag_count[update.binding] < 5) {
        auto* ss = renodx::utils::shader::GetCurrentState(cmd_list);
        uint32_t hash = ss ? renodx::utils::shader::GetCurrentPixelShaderHash(ss) : 0u;
        diag_count[update.binding]++;
        char buf[128];
        snprintf(buf, sizeof(buf), "[DLAA] Diag: t%u pushed, hash=0x%08X",
                 update.binding, hash);
        reshade::log::message(reshade::log::level::info, buf);
      }
    }

    // Color: FXAA (0x96BB8CFF) or pre-FXAA composite (0xE8C7EBA2) at t0
    if (update.binding == 0u && update.count >= 1 && views[0].handle != 0u) {
      auto* ss = renodx::utils::shader::GetCurrentState(cmd_list);
      if (ss) {
        uint32_t hash = renodx::utils::shader::GetCurrentPixelShaderHash(ss);
        if (hash == 0x96BB8CFFu || hash == 0xE8C7EBA2u) {
          d->captured_color_srv = views[0];
          d->captured_color_res = dev->get_resource_from_view(views[0]);
        }
      }
    }

    // Depth: full-res depth-format texture at t0 or t1. PREFER the perspective
    // depth-stencil family (D32=40, R24G8_TYPELESS=44, D24S8=45, R24_UNORM_X8=46)
    // over R32/R16 variants (41/53/54/56), which are usually LINEAR/processed depth
    // that cannot be used as clip-space Z/W (shows all-white in the Depth view).
    if ((update.binding == 0u || update.binding == 1u)
        && update.count >= 1 && views[0].handle != 0u) {
      auto res = dev->get_resource_from_view(views[0]);
      if (res.handle) {
        auto rd = dev->get_resource_desc(res);
        int fmt = (int)rd.texture.format;
        bool is_primary = (fmt == 40 || fmt == 44 || fmt == 45 || fmt == 46);
        bool is_fallback = (fmt == 41 || fmt == 53 || fmt == 54 || fmt == 56);
        if ((is_primary || is_fallback)
            && (float)rd.texture.width == d->swapchain_w
            && (float)rd.texture.height == d->swapchain_h) {
          auto* ss = renodx::utils::shader::GetCurrentState(cmd_list);
          uint32_t hash = ss ? renodx::utils::shader::GetCurrentPixelShaderHash(ss) : 0u;
          // Depth-source scan: log each distinct full-res depth pass once (debug).
          if (shader_injection.dlaa_debug_logging > 0.5f) {
            static int depth_scan_count = 0;
            if (depth_scan_count < 25) {
              depth_scan_count++;
              char dbuf[128];
              snprintf(dbuf, sizeof(dbuf), "[DLAA] Depth push: hash=0x%08X binding=%u fmt=%d %ux%u",
                       hash, update.binding, fmt,
                       (int)rd.texture.width, (int)rd.texture.height);
              reshade::log::message(reshade::log::level::info, dbuf);
            }
          }
          // Optional hash gate to a selected depth pass.
          bool src_match = true;
          if (shader_injection.dlaa_depth_source >= 0.5f) {
            uint32_t want = 0;
            switch ((int)shader_injection.dlaa_depth_source) {
              case 1: want = 0x0E83E74Eu; break;
              case 2: want = 0x55D61207u; break;
              case 3: want = 0x322E20D4u; break;
              default: want = 0u; break;
            }
            src_match = (want == 0u) || (hash == want);
          }
          // Prefer perspective depth; only use a linear/processed one if no
          // perspective depth has been captured this frame.
          if (src_match && (is_primary || !d->depth_primary_captured)) {
            d->captured_depth_srv = views[0];
            d->captured_depth_res = res;
            d->viewport_w = (float)rd.texture.width;
            d->viewport_h = (float)rd.texture.height;
            d->depth_source_hash = hash;
            if (is_primary) d->depth_primary_captured = true;
          }
        }
      }
    }
  }

  // ── CBV capture (b0 _Globals) ──
  // Hash-gated: only capture when a scene-geometry VS is bound, so
  // captured_scene_cbv reliably points at the camera _Globals (not the post
  // cbuffer, shadow matrices, or per-effect buffers).
  if (update.type == reshade::api::descriptor_type::constant_buffer) {
    if (update.binding == 0u && update.count >= 1) {
      auto* cbv = static_cast<const reshade::api::buffer_range*>(update.descriptors);
      if (cbv->buffer.handle) {
        auto* cbv_ss = renodx::utils::shader::GetCurrentState(cmd_list);
        uint32_t vhash = cbv_ss ? renodx::utils::shader::GetCurrentVertexShaderHash(cbv_ss) : 0u;
        if (IsSceneGeometryVs(vhash)) {
          d->captured_scene_cbv = *cbv;
          d->captured_scene_cbv_valid = true;
          // Queue the camera-matrix staging copy EARLY so the Map at FXAA
          // (ReadSceneMatrices) doesn't stall the GPU pipeline mid-frame.
          if (shader_injection.dlaa_enabled > 0.5f && !d->scene_cbv_copy_issued) {
            d->scene_cbv_copy_issued = IssueSceneCbvCopy(cmd_list, d);
          }
        }
      }
    }
  }
}

// ── Jitter update ──
static void UpdateJitter(DeviceData* d) {
  if (!d) return;
  if (shader_injection.dlaa_jitter_test > 0.5f) {
    // Jitter Test: apply a large FIXED 8px horizontal projection shift so the
    // rasterization-level jitter is plainly visible. With DLAA OFF + Jitter ON,
    // the whole image should visibly jump 8px (a REAL rasterization shift, not a
    // post-process translation) — verifies the proxy projection jitter reaches
    // the geometry with no fullscreen filtering.
    float jx = 8.f * 2.f / d->viewport_w;
    d->jitter_x = jx; d->jitter_y = 0.f;
    shader_injection.jitter_offset_x = jx;
    shader_injection.jitter_offset_y = 0.f;
    return;
  }
  if (shader_injection.dlaa_jitter_enabled < 0.5f) {
    d->jitter_x = 0.f; d->jitter_y = 0.f;
    shader_injection.jitter_offset_x = 0.f;
    shader_injection.jitter_offset_y = 0.f;
    return;
  }
  uint32_t f = d->frame_index;
  float jx = (Halton(f + 1u, 2u) - 0.5f) * 2.f / d->viewport_w;
  float jy = (Halton(f + 1u, 3u) - 0.5f) * 2.f / d->viewport_h;
  d->jitter_x = jx; d->jitter_y = jy;
  shader_injection.jitter_offset_x = jx;
  shader_injection.jitter_offset_y = jy;
}

// ── Projection jitter: REMOVED ──
// The old proxy-cbuffer approach (copy _Globals -> patch c10 -> re-bind at b0)
// was unworkable: b0 is a PER-OBJECT cbuffer (World at c44), so a shared proxy
// would give every object the first object's World matrix, and per-draw
// readbacks killed performance. Jitter is now applied directly in the replaced
// scene-geometry VSs (boot/0xHASH.vs_4_1.hlsl): o0.x += jitter_x * o0.w after
// the ViewProjection multiply. See SCENE_GEOMETRY_VS_HASHES.

// ── Velocity push constants ──
// Push constants (b13, 48 floats = 12 float4s) matching motion_velocity.cs_5_0.hlsl:
//   c[0..15] = prevViewProj, c[16..31] = curViewProjInv,
//   c[32..35] = params0 (vp_w, vp_h, velocity_scale, debug_view),
//   c[36..39] = params1 (jitter_x, jitter_y, per_object_motion, zero_mv),
//   c[40..43] = params2 (mv_threshold, mv_direction, 0, 0)
static std::array<float, 48> BuildVelocityPC(DeviceData* d) {
  std::array<float, 48> c = {};
  if (!d) return c;
  memcpy(&c[0],  d->prev_view_proj.data(), 64);
  memcpy(&c[16], d->curr_view_proj_inv.data(), 64);
  c[32] = d->viewport_w;  c[33] = d->viewport_h;
  c[34] = shader_injection.dlaa_velocity_scale;
  c[35] = shader_injection.dlaa_debug_view;
  c[36] = d->jitter_x;    c[37] = d->jitter_y;
  c[38] = shader_injection.dlaa_per_object_motion;
  c[39] = shader_injection.dlaa_zero_mv;
  c[40] = shader_injection.dlaa_mv_threshold;
  c[41] = shader_injection.dlaa_mv_direction;
  return c;
}

// ── DLAA dispatch ──
static bool RunDLAA(reshade::api::command_list* cmd_list) {
  auto* dev = cmd_list->get_device();
  if (!dev) return false;
  auto* d = dev->get_private_data<DeviceData>();
  if (!d) return false;

  if (!d->captured_depth_srv.handle || !d->captured_scene_cbv_valid || !d->captured_color_srv.handle) {
    static int missing_log_count = 0;
    if (shader_injection.dlaa_debug_logging > 0.5f && ++missing_log_count % 300 == 0) {
      std::string miss = "[DLAA] Missing:";
      if (!d->captured_depth_srv.handle) miss += " depth";
      if (!d->captured_scene_cbv_valid) miss += " cbv";
      if (!d->captured_color_srv.handle) miss += " color";
      miss += " — skipping";
      reshade::log::message(reshade::log::level::warning, miss.c_str());
    }
    return false;
  }

  uint32_t w = (uint32_t)d->viewport_w, h = (uint32_t)d->viewport_h;
  if (w < 64u) w = 64u; if (h < 64u) h = 64u;

  // Motion-vector texture format (A/B): r16g16_float (default) or r32g32_float.
  // Recreated on toggle change so the switch is instant mid-session.
  const auto vel_fmt = (shader_injection.dlaa_velocity_format > 0.5f)
                           ? reshade::api::format::r32g32_float
                           : reshade::api::format::r16g16_float;
  if (d->velocity_texture.handle && d->velocity_format != vel_fmt) {
    if (shader_injection.dlaa_debug_logging > 0.5f)
      reshade::log::message(reshade::log::level::info, "[DLAA] Velocity format change: recreating MV texture");
    if (d->velocity_srv.handle) dev->destroy_resource_view(d->velocity_srv);
    if (d->velocity_uav.handle) dev->destroy_resource_view(d->velocity_uav);
    dev->destroy_resource(d->velocity_texture);
    d->velocity_texture = {}; d->velocity_srv = {}; d->velocity_uav = {};
    d->velocity_format = {}; d->resources_created = false;
    if (d->dbg_vel_srv) { d->dbg_vel_srv->Release(); d->dbg_vel_srv = nullptr; }
  }
  if (!d->velocity_texture.handle) {
    reshade::api::resource_desc rd = {};
    rd.type = reshade::api::resource_type::texture_2d;
    rd.texture = {w, h, 1, 1, vel_fmt, 1};
    rd.heap = reshade::api::memory_heap::gpu_only;
    rd.usage = reshade::api::resource_usage::shader_resource | reshade::api::resource_usage::unordered_access;
    dev->create_resource(rd, nullptr, reshade::api::resource_usage::shader_resource, &d->velocity_texture);
    reshade::api::resource_view_desc vd(reshade::api::resource_view_type::texture_2d, vel_fmt, 0,1,0,1);
    dev->create_resource_view(d->velocity_texture, reshade::api::resource_usage::shader_resource, vd, &d->velocity_srv);
    dev->create_resource_view(d->velocity_texture, reshade::api::resource_usage::unordered_access, vd, &d->velocity_uav);
    d->velocity_format = vel_fmt;
    d->resources_created = true;
    if (shader_injection.dlaa_debug_logging > 0.5f)
      reshade::log::message(reshade::log::level::info, (std::string("[DLAA] Velocity texture created: ") + std::to_string(w) + "x" + std::to_string(h) + " fmt=" + std::to_string((int)vel_fmt)).c_str());
  }

  if (!d->velocity_pipeline.handle && !CreateVelocityPipeline(dev, d)) return false;

  const auto UA = reshade::api::resource_usage::unordered_access;
  const auto SR = reshade::api::resource_usage::shader_resource;
  const auto CS = reshade::api::shader_stage::all_compute;
  const auto AC = reshade::api::pipeline_stage::all_compute;

  // Jitter is computed ONCE PER FRAME at present time (see present handler), so the
  // composite UV shift and the NGX jitter offsets both read the SAME stored value
  // (rendered jitter == reported jitter).

  // Sync dlss.hpp globals from settings (preset & enable gates)
  senkiseki3::dlss::dlss_enabled = shader_injection.dlaa_enabled;
  senkiseki3::dlss::dlss_render_preset = shader_injection.dlaa_preset;
  // MVs are jitter-subtracted in the velocity shader, so MVJittered stays off.
  senkiseki3::dlss::dlss_motion_vectors_jittered = 0.f;
  senkiseki3::dlss::dlss_debug_logging = shader_injection.dlaa_debug_logging;
  senkiseki3::dlss::dlss_flag_is_hdr = shader_injection.dlaa_flag_is_hdr;
  senkiseki3::dlss::dlss_flag_depth_inverted = shader_injection.dlaa_flag_depth_inverted;
  senkiseki3::dlss::dlss_flag_auto_exposure = shader_injection.dlaa_flag_auto_exposure;

  // DLAA evaluate (only when all resources ready + NGX supported)
  bool dlaa_ok = false;
  if (senkiseki3::dlss::ngx.supported && d->captured_color_res.handle
      && d->captured_rtv0_res.handle && d->captured_depth_res.handle) {

    // Read camera matrices from the game's _Globals CBV (depth-projection velocity)
    auto* cl = reinterpret_cast<ID3D11DeviceContext*>(cmd_list->get_native());
    ReadSceneMatrices(dev, d, cl);

    if (d->matrices_valid) {
      // Pass: Velocity compute
      cmd_list->bind_pipeline(AC, d->velocity_pipeline);

      reshade::api::descriptor_table_update u[5] = {
        { d->velocity_tables[0], 0, 0, 1, reshade::api::descriptor_type::sampler, &d->point_sampler },
        { d->velocity_tables[1], 0, 0, 1, reshade::api::descriptor_type::constant_buffer, &d->captured_scene_cbv },
        { d->velocity_tables[2], 0, 0, 1, reshade::api::descriptor_type::texture_shader_resource_view, &d->captured_depth_srv },
        { d->velocity_tables[3], 0, 0, 1, reshade::api::descriptor_type::texture_shader_resource_view, &d->captured_motion_srv },
        { d->velocity_tables[4], 0, 0, 1, reshade::api::descriptor_type::texture_unordered_access_view, &d->velocity_uav },
      };
      dev->update_descriptor_tables(5, u);

      reshade::api::descriptor_table tables[5] = {
          d->velocity_tables[0], d->velocity_tables[1], d->velocity_tables[2],
          d->velocity_tables[3], d->velocity_tables[4]};
      cmd_list->bind_descriptor_tables(CS, d->velocity_layout, 0, 5, tables);

      auto pc = BuildVelocityPC(d);
      cmd_list->push_constants(CS, d->velocity_layout, 5, 0, 48, pc.data());
      cmd_list->dispatch((w + 7) / 8, (h + 7) / 8, 1);
      cmd_list->barrier(d->velocity_texture, UA, SR);

      // Unbind compute UAVs (D3D11 keeps them bound across passes)
      ID3D11UnorderedAccessView* null_uavs[4] = {};
      cl->CSSetUnorderedAccessViews(0, 4, null_uavs, nullptr);
      ID3D11ShaderResourceView* null_srvs[4] = {};
      cl->CSSetShaderResources(0, 4, null_srvs);
      cl->CSSetShader(nullptr, nullptr, 0);

      auto* src = reinterpret_cast<ID3D11Resource*>(d->captured_color_res.handle);
      auto* mv  = reinterpret_cast<ID3D11Resource*>(d->velocity_texture.handle);
      auto* dep = reinterpret_cast<ID3D11Resource*>(d->captured_depth_res.handle);
      auto* ngx_out = reinterpret_cast<ID3D11Resource*>(
          senkiseki3::dlss::ngx.output_texture.Get());
      if (cl && src && mv && dep && ngx_out) {
        if (shader_injection.dlaa_debug_view >= 1.f) {
          // MV debug screen (modes: 1=HSV, 2=Arrows, 3=Magnitude, 4=Reprojection).
          // Fully native path: dispatch writes into the NGX output texture and
          // the display uses the same native SRV binding as the working DLAA path.
          if (!EnsurePrevColorTexture(dev, d, w, h)) return false;
          if (!EnsureDebugNative(dev, d)) {
            reshade::log::message(reshade::log::level::error,
              "[DLAA] MV debug native setup failed");
            return false;
          }

          static int dbg_mode_log = 0;
          if (dbg_mode_log < 3 && shader_injection.dlaa_debug_logging > 0.5f) {
            dbg_mode_log++;
            char buf[96];
            snprintf(buf, sizeof(buf), "[DLAA] MV debug mode=%d scale=%.0f",
                     (int)shader_injection.dlaa_debug_view, shader_injection.dlaa_debug_scale);
            reshade::log::message(reshade::log::level::info, buf);
          }

          D3D11_MAPPED_SUBRESOURCE map = {};
          if (SUCCEEDED(cl->Map(d->dbg_cb, 0, D3D11_MAP_WRITE_DISCARD, 0, &map))) {
            float* p = static_cast<float*>(map.pData);
            p[0] = shader_injection.dlaa_debug_view;
            p[1] = shader_injection.dlaa_debug_scale;
            p[2] = 0.f; p[3] = 0.f;
            cl->Unmap(d->dbg_cb, 0);
          }
          cl->CSSetConstantBuffers(13, 1, &d->dbg_cb);
          ID3D11ShaderResourceView* dbg_depth_srv =
              d->captured_depth_srv.handle
              ? reinterpret_cast<ID3D11ShaderResourceView*>(d->captured_depth_srv.handle)
              : nullptr;
          ID3D11ShaderResourceView* dbg_srvs[4] = { d->dbg_vel_srv, d->dbg_color_srv, d->dbg_prev_srv, dbg_depth_srv };
          cl->CSSetShaderResources(0, 4, dbg_srvs);
          cl->CSSetSamplers(0, 1, &d->dbg_linear_sampler);
          ID3D11UnorderedAccessView* dbg_uavs[1] = { d->dbg_out_uav };
          cl->CSSetUnorderedAccessViews(0, 1, dbg_uavs, nullptr);
          cl->CSSetShader(d->dbg_cs, nullptr, 0);
          cl->Dispatch((w + 7) / 8, (h + 7) / 8, 1);

          ID3D11UnorderedAccessView* null_uav_dbg = nullptr;
          cl->CSSetUnorderedAccessViews(0, 1, &null_uav_dbg, nullptr);
          ID3D11ShaderResourceView* null_srvs_dbg[4] = {};
          cl->CSSetShaderResources(0, 4, null_srvs_dbg);
          cl->CSSetShader(nullptr, nullptr, 0);

          // Display: native SRV of the NGX output texture -> PS t0 (proven path).
          ID3D11ShaderResourceView* dbg_srv = nullptr;
          D3D11_SHADER_RESOURCE_VIEW_DESC dvd = {};
          dvd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
          dvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
          dvd.Texture2D.MipLevels = 1;
          if (SUCCEEDED(senkiseki3::dlss::ngx.device->CreateShaderResourceView(
                  senkiseki3::dlss::ngx.output_texture.Get(), &dvd, &dbg_srv))) {
            cl->PSSetShaderResources(0, 1, &dbg_srv);
            dbg_srv->Release();
            dlaa_ok = true;
          } else {
            reshade::log::message(reshade::log::level::warning,
              "[DLAA] Failed to create SRV for MV debug view");
          }
        } else {
          // NGX jitter must match the rendered screen-space jitter (pixels, y-down).
          // The VS adds +JITTER_Y to clip Y (y-up), which shifts content by -JY on
          // screen Y, so the Y offset is negated here.
          float jitter_px_x = d->jitter_x * d->viewport_w * 0.5f;
          float jitter_px_y = -d->jitter_y * d->viewport_h * 0.5f;
          // A/B the NGX jitter sign convention (NVIDIA ships a debug hotkey for this):
          // 0 = FlipBoth, 1 = FlipX, 2 = FlipY, 3 = Current.
          const int jsign = (int)shader_injection.dlaa_jitter_sign;
          if (jsign != 3) {
            if (jsign == 0 || jsign == 1) jitter_px_x = -jitter_px_x;
            if (jsign == 0 || jsign == 2) jitter_px_y = -jitter_px_y;
          }
          // Force Reset A/B: makes InReset=1 every frame (no history). Does NOT
          // recreate the feature, so it isolates temporal accumulation.
          if (shader_injection.dlaa_force_reset > 0.5f)
            senkiseki3::dlss::ngx.reset = true;
          // Periodic diagnostic: confirms jitter is non-zero, NGX accumulation
          // (reset) is stable, captured color is stable (fmt/dims churn would
          // recreate the feature every frame -> no accumulation), and the game's
          // ViewProjection is stable when the camera is static (a large vpd while
          // "standing still" means the game bakes per-frame camera movement/jitter
          // into its matrices, so MVs are legitimately non-zero).
          static int jitter_log_count = 0;
          if (shader_injection.dlaa_debug_logging > 0.5f && (++jitter_log_count % 60 == 0)) {
            char buf[192];
            int cw = 0, ch = 0, cfmt = 0;
            if (d->captured_color_res.handle) {
              auto cdesc = dev->get_resource_desc(d->captured_color_res);
              cw = (int)cdesc.texture.width; ch = (int)cdesc.texture.height;
              cfmt = (int)cdesc.texture.format;
            }
            float vp_delta = 0.f;
            for (int i = 0; i < 16; ++i)
              vp_delta += std::fabs(d->curr_view_proj[i] - d->prev_view_proj[i]);
            int mw = 0, mh = 0, mfmt = 0;
            if (d->captured_motion_res.handle) {
              auto mdesc = dev->get_resource_desc(d->captured_motion_res);
              mw = (int)mdesc.texture.width; mh = (int)mdesc.texture.height;
              mfmt = (int)mdesc.texture.format;
            }
            int dw = 0, dh = 0, dfmt = 0;
            if (d->captured_depth_res.handle) {
              auto ddesc = dev->get_resource_desc(d->captured_depth_res);
              dw = (int)ddesc.texture.width; dh = (int)ddesc.texture.height;
              dfmt = (int)ddesc.texture.format;
            }
            snprintf(buf, sizeof(buf),
              "[DLAA] f=%u px=(%.3f,%.3f) reset=%d feature=%s color=%ux%u fmt=%d vpd=%.4f po=%d motion=%ux%u fmt=%d depth=%ux%u fmt=%d src=0x%08X",
              d->frame_index,
              d->jitter_x * d->viewport_w * 0.5f, -d->jitter_y * d->viewport_h * 0.5f,
              senkiseki3::dlss::ngx.reset ? 1 : 0,
              senkiseki3::dlss::ngx.feature ? "yes" : "no",
              cw, ch, cfmt, vp_delta,
              (int)shader_injection.dlaa_per_object_motion, mw, mh, mfmt,
              dw, dh, dfmt, d->depth_source_hash);
            reshade::log::message(reshade::log::level::info, buf);
          }
          dlaa_ok = senkiseki3::dlss::EvaluateDLSS(cl, src, ngx_out, mv, dep,
                       jitter_px_x, jitter_px_y, 1.f, 1.f);
        }
      }
      // Replace t0 with DLAA output SRV (falcomengine-plus pattern).
      // In MV debug mode the velocity SRV is already bound to t0 above.
      if (dlaa_ok && shader_injection.dlaa_debug_view <= 0.5f) {
        D3D11_TEXTURE2D_DESC ngx_desc;
        senkiseki3::dlss::ngx.output_texture->GetDesc(&ngx_desc);
        static int fmt_log = 0;
        if (shader_injection.dlaa_debug_logging > 0.5f && ++fmt_log <= 2) {
          char buf[96];
          snprintf(buf, sizeof(buf), "[DLAA] NGX output: %ux%u fmt=%d",
                   ngx_desc.Width, ngx_desc.Height, (int)ngx_desc.Format);
          reshade::log::message(reshade::log::level::info, buf);
        }

        ID3D11ShaderResourceView* dlaa_srv = nullptr;
        D3D11_SHADER_RESOURCE_VIEW_DESC srvd = {};
        srvd.Format = ngx_desc.Format;
        srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvd.Texture2D.MipLevels = 1;
        senkiseki3::dlss::ngx.device->CreateShaderResourceView(
            senkiseki3::dlss::ngx.output_texture.Get(), &srvd, &dlaa_srv);
        if (dlaa_srv) {
          cl->PSSetShaderResources(0, 1, &dlaa_srv);
          dlaa_srv->Release();
        } else {
          reshade::log::message(reshade::log::level::warning,
            "[DLAA] Failed to create SRV for NGX output");
        }
      }
    }
  }

  // Keep a previous-frame color copy for the reprojection debug mode.
  if (shader_injection.dlaa_debug_view >= 1.f && d->captured_color_res.handle) {
    auto* cl2 = reinterpret_cast<ID3D11DeviceContext*>(cmd_list->get_native());
    if (cl2 && d->prev_color_texture.handle) {
      auto* prev_native = reinterpret_cast<ID3D11Resource*>(d->prev_color_texture.handle);
      auto* cur_native = reinterpret_cast<ID3D11Resource*>(d->captured_color_res.handle);
      cl2->CopyResource(prev_native, cur_native);
    }
  }

  d->frame_index++;
  return false;  // Never skip FXAA — DLAA replaces t0, FXAA composites to RTV0
}

// ── Settings ──
renodx::utils::settings::Settings settings = {
    new renodx::utils::settings::Setting{
        .key = "DLAAEnabled", .binding = &shader_injection.dlaa_enabled,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f, .label = "DLAA", .section = "Antialiasing",
        .tooltip = "Replace FXAA with NVIDIA DLAA. Requires nvngx_dlss.dll.", .labels = {"FXAA", "DLAA"},
    },
    new renodx::utils::settings::Setting{
        .key = "DLAAPreset", .binding = &shader_injection.dlaa_preset,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f, .label = "DLSS Preset", .section = "Antialiasing",
        .labels = {"Default","F-CNN","J-T1","K-T1","L-T2","M-T2"},
        .is_enabled = []{ return shader_injection.dlaa_enabled > 0.5f; },
    },
    new renodx::utils::settings::Setting{
        .key = "DLAAJitter", .binding = &shader_injection.dlaa_jitter_enabled,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f, .label = "Camera Jitter", .section = "Antialiasing",
        .labels = {"Off","On"}, .is_enabled = []{ return shader_injection.dlaa_enabled > 0.5f; },
    },
    new renodx::utils::settings::Setting{
        .key = "DLAAJitterSign", .binding = &shader_injection.dlaa_jitter_sign,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f, .label = "Jitter Sign", .section = "Antialiasing",
        .labels = {"FlipBoth","FlipX","FlipY","Current"},
        .is_enabled = []{ return shader_injection.dlaa_enabled > 0.5f; },
    },
    new renodx::utils::settings::Setting{
        .key = "DLAAJitterTest", .binding = &shader_injection.dlaa_jitter_test,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f, .label = "Jitter Test (8px)", .section = "Antialiasing",
        .tooltip = "Applies a fixed 8px viewport shift. With DLAA OFF + Jitter ON, the whole image must visibly jump 8px (real rasterization shift) to confirm viewport jitter works.",
        .labels = {"Off","On"},
        .is_enabled = []{ return true; },
    },
    new renodx::utils::settings::Setting{
        .key = "DLAAZeroMV", .binding = &shader_injection.dlaa_zero_mv,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f, .label = "Zero Motion Vectors", .section = "Antialiasing",
        .tooltip = "Forces all motion vectors to 0. A/B: if image looks similar, MVs aren't helping; if worse, MVs contribute useful info.",
        .labels = {"Off","On"},
        .is_enabled = []{ return shader_injection.dlaa_enabled > 0.5f; },
    },
    new renodx::utils::settings::Setting{
        .key = "DLAAMVDirection", .binding = &shader_injection.dlaa_mv_direction,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f, .label = "MV Direction Flip", .section = "Antialiasing",
        .tooltip = "Flips motion vector sign (previous-current instead of current-previous). A/B: if flipping fixes ghosting, the convention was wrong.",
        .labels = {"Off","Flip"},
        .is_enabled = []{ return shader_injection.dlaa_enabled > 0.5f; },
    },
    new renodx::utils::settings::Setting{
        .key = "DLAAMVThreshold", .binding = &shader_injection.dlaa_mv_threshold,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 0.f, .label = "MV Threshold (px)", .section = "Antialiasing",
        .tooltip = "Zeros motion vectors below this magnitude. Kills static sub-pixel MV noise that poisons history.",
        .min = 0.f, .max = 5.f, .format = "%.2f",
        .is_enabled = []{ return shader_injection.dlaa_enabled > 0.5f; },
    },
    new renodx::utils::settings::Setting{
        .key = "DLAAVelocityFormat", .binding = &shader_injection.dlaa_velocity_format,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f, .label = "32-bit Motion Vectors", .section = "Antialiasing",
        .tooltip = "MV texture precision A/B: r16g16_float (16-bit, default) vs r32g32_float (32-bit). 32-bit costs a little bandwidth; 16-bit is already exact for pixel-space MVs up to 2048px.",
        .labels = {"16-bit (r16g16)","32-bit (r32g32)"},
        .is_enabled = []{ return shader_injection.dlaa_enabled > 0.5f; },
    },
    new renodx::utils::settings::Setting{
        .key = "DLAADepthSource", .binding = &shader_injection.dlaa_depth_source,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f, .label = "Depth Source", .section = "Antialiasing",
        .tooltip = "Which pass's depth DLAA uses. Auto = last full-res depth push (may be wrong). Pick the one that shows a real depth map in MV Debug=Depth. See scan log.",
        .labels = {"Auto","0x0E83E74E","0x55D61207","0x322E20D4"},
        .is_enabled = []{ return shader_injection.dlaa_enabled > 0.5f; },
    },
    new renodx::utils::settings::Setting{
        .key = "DLAAForceReset", .binding = &shader_injection.dlaa_force_reset,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f, .label = "Force Reset (No History)", .section = "Antialiasing",
        .tooltip = "Forces NGX InReset=1 every frame (no temporal accumulation). A/B test: if identical to Off, history is already disabled.",
        .labels = {"Off","On"},
        .is_enabled = []{ return shader_injection.dlaa_enabled > 0.5f; },
    },
    new renodx::utils::settings::Setting{
        .key = "DLAAPerObjectMotion", .binding = &shader_injection.dlaa_per_object_motion,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f, .label = "Per-Object Motion", .section = "Antialiasing",
        .labels = {"Camera Only","Per-Object"}, .is_enabled = []{ return shader_injection.dlaa_enabled > 0.5f; },
    },
    new renodx::utils::settings::Setting{
        .key = "DLAAVelocityScale", .binding = &shader_injection.dlaa_velocity_scale,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 1.f, .label = "Velocity Scale", .section = "Antialiasing",
        .min = 0.1f, .max = 5.f, .format = "%.2f",
        .is_enabled = []{ return shader_injection.dlaa_enabled > 0.5f; },
    },
    new renodx::utils::settings::Setting{
        .key = "DLAADebugView", .binding = &shader_injection.dlaa_debug_view,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f, .label = "MV Debug", .section = "Antialiasing",
        .labels = {"Off","HSV","Arrows","Magnitude","Reproj","Depth"},
        .is_enabled = []{ return shader_injection.dlaa_enabled > 0.5f; },
    },
    new renodx::utils::settings::Setting{
        .key = "DLAADebugScale", .binding = &shader_injection.dlaa_debug_scale,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 50.f, .label = "MV Debug Scale", .section = "Antialiasing",
        .min = 1.f, .max = 200.f, .format = "%.0f",
        .is_enabled = []{ return shader_injection.dlaa_enabled > 0.5f && shader_injection.dlaa_debug_view >= 1.f; },
    },
    new renodx::utils::settings::Setting{
        .key = "DLAAFlagIsHDR", .binding = &shader_injection.dlaa_flag_is_hdr,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f, .label = "Flag: HDR Input", .section = "Antialiasing",
        .labels = {"Off","On"}, .is_enabled = []{ return shader_injection.dlaa_enabled > 0.5f; },
    },
    new renodx::utils::settings::Setting{
        .key = "DLAAFlagDepthInverted", .binding = &shader_injection.dlaa_flag_depth_inverted,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f, .label = "Flag: Depth Inverted", .section = "Antialiasing",
        .labels = {"Off","On"}, .is_enabled = []{ return shader_injection.dlaa_enabled > 0.5f; },
    },
    new renodx::utils::settings::Setting{
        .key = "DLAAFlagAutoExposure", .binding = &shader_injection.dlaa_flag_auto_exposure,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f, .label = "Flag: Auto Exposure", .section = "Antialiasing",
        .labels = {"Off","On"}, .is_enabled = []{ return shader_injection.dlaa_enabled > 0.5f; },
    },
    new renodx::utils::settings::Setting{
        .key = "DLAADebugLogging", .binding = &shader_injection.dlaa_debug_logging,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f, .label = "Debug Logging", .section = "Antialiasing",
        .labels = {"Off","On"},
    },
};

// ── Draw hook: FXAA replacement ──
// DLAA output replaces t0 content; FXAA reads it and composites to RTV0
static bool OnBeforeFxaaDraw(reshade::api::command_list* cmd_list) {
  if (shader_injection.dlaa_enabled < 0.5f) return true;
  RunDLAA(cmd_list);
  return true;  // Always let FXAA run — it composites our DLAA'd t0 to RTV0
}

renodx::mods::shader::CustomShaders custom_shaders = {
    {
        0x96BB8CFFu,
        renodx::mods::shader::CustomShader{
            .crc32 = 0x96BB8CFFu,
            .on_draw = OnBeforeFxaaDraw,
        },
    },
    // ── Scene-geometry VS replacements (hash-gated camera jitter) ──
    // Each replaced VS adds the per-frame sub-pixel jitter to SV_Position after
    // the ViewProjection multiply (o0.x += DLAA_JITTER_X * o0.w). This is the
    // ONLY jitter source — no proxy cbuffer, no composite UV shift. The jitter
    // offsets come from the addon's b13 injection (0 when disabled), so the
    // 8px jitter-test toggle works without DLAA being enabled.
    CustomShaderEntry(0x37F1DE22),  // world/terrain (primary)
    CustomShaderEntry(0xCBF171E5),  // world
    CustomShaderEntry(0xDF1D933F),  // world
    CustomShaderEntry(0x9BB882F5),  // terrain tiles
    CustomShaderEntry(0x43ED1D83),  // world
    CustomShaderEntry(0xC1F80CF6),  // world
    CustomShaderEntry(0x8BB470CE),  // world
    CustomShaderEntry(0x4E107313),  // world (COLOR input)
    CustomShaderEntry(0x09394015),  // unskinned characters/NPCs
    CustomShaderEntry(0xB2F338C8),  // skinned
    CustomShaderEntry(0xB1C24E2A),  // skinned
    // Skinned character/NPC VS: geometry jitter + outputs prev clip-space
    // position (o7) for per-object motion vectors. Prev clip is computed with
    // the unjittered prevViewProj (b13), so the MV stays jitter-free.
    CustomShaderEntry(0x0D5DABC6),
    // Additional character-mesh VSs (hair, eyes, skin, clothing) — same
    // geometry jitter; discovered in the character-heavy scene.
    CustomShaderEntry(0x5C1A50E5),  // hair
    CustomShaderEntry(0x4A030C25),  // clothing
    CustomShaderEntry(0x3641D444),  // eyeball
    CustomShaderEntry(0xC8F5D77B),  // skin
    CustomShaderEntry(0xF8C9B92D),  // clothing
    CustomShaderEntry(0xB662509A),  // clothing
    // Foliage & world VSs discovered in the foliage scene (same geometry jitter).
    CustomShaderEntry(0x29513853),  // foliage (main)
    CustomShaderEntry(0xED3D1A43),  // foliage
    CustomShaderEntry(0x7D5282A3),  // scene/world
    CustomShaderEntry(0x066E7DFB),  // scene/world
    CustomShaderEntry(0x714E4C33),  // scene/world
    CustomShaderEntry(0x7A711F41),  // scene/world
    CustomShaderEntry(0x09BD12FA),  // scene/world
    CustomShaderEntry(0x030AD345),  // scene/world
    CustomShaderEntry(0x8913640A),  // scene/world
    // Second batch of scene/world & world-space effect VSs from the same scene.
    CustomShaderEntry(0x2DC04A66),
    CustomShaderEntry(0x34AA271F),
    CustomShaderEntry(0xDFE5A75D),
    CustomShaderEntry(0x8ED5035B),
    CustomShaderEntry(0x97E9A1EC),
    CustomShaderEntry(0x4D37FA49),
    CustomShaderEntry(0x9596CBC1),
    CustomShaderEntry(0xE4C6D6F4),
    CustomShaderEntry(0x8AFF0B4F),  // water/particle
    CustomShaderEntry(0x795F3AD3),  // world-space effect
    CustomShaderEntry(0x5E5AE3FB),  // character outline
    CustomShaderEntry(0x77355EED),  // forest impostor billboard (sky)
    CustomShaderEntry(0xC8FE8FC4),  // transparent texture
    CustomShaderEntry(0x7D3553A7),  // particle
    // Character G-buffer PS: encodes per-object prevNDC into MRT2 (o2.zw) so
    // the velocity compute can build MVs for skinned characters.
    CustomShaderEntry(0x0E8BC215),
    // Pre-FXAA composite: pure pixel-center passthrough (no UV-shift jitter).
    // The geometry jitter passes through unchanged to the buffer RunDLAA feeds
    // to DLSS.
    CustomShaderEntry(0xE8C7EBA2),
};

}  // namespace

extern "C" __declspec(dllexport) constexpr const char* NAME = "Senkiseki3 DLAA";
extern "C" __declspec(dllexport) constexpr const char* DESCRIPTION =
    "NVIDIA DLAA for Senkiseki3. Requires nvngx_dlss.dll.";

BOOL APIENTRY DllMain(HMODULE h_module, DWORD fdw_reason, LPVOID) {
  switch (fdw_reason) {
    case DLL_PROCESS_ATTACH:
      if (!reshade::register_addon(h_module)) return FALSE;
      renodx::utils::settings::use_presets = false;
      renodx::mods::shader::force_pipeline_cloning = true;
      renodx::mods::shader::allow_multiple_push_constants = true;
      renodx::mods::shader::expected_constant_buffer_index = 13;
      renodx::mods::shader::expected_constant_buffer_space = 0;

      reshade::register_event<reshade::addon_event::bind_render_targets_and_depth_stencil>(OnBindRenderTargets);
      reshade::register_event<reshade::addon_event::push_descriptors>(OnPushDescriptorsCapture);

      reshade::register_event<reshade::addon_event::present>(
          [](reshade::api::command_queue*, reshade::api::swapchain* swapchain,
             const reshade::api::rect*, const reshade::api::rect*, uint32_t, const reshade::api::rect*) {
        auto* dev = swapchain->get_device();
        if (!dev) return;
        // Always track output resolution for depth validation
        auto* d = dev->get_private_data<DeviceData>();
        if (d) {
          reshade::api::resource_desc bbd = dev->get_resource_desc(swapchain->get_current_back_buffer());
          d->swapchain_w = (float)bbd.texture.width;
          d->swapchain_h = (float)bbd.texture.height;
          // Reset the per-frame depth-source preference (captures happen during the
          // next frame, before RunDLAA at FXAA).
          d->depth_primary_captured = false;
          // The early scene-CBV staging copy is issued once per frame (at the
          // first scene-geometry b0 push) and consumed at FXAA.
          d->scene_cbv_copy_issued = false;
          // Jitter is computed once per frame at PRESENT time, before the next frame's
          // composite (0xE8C7EBA2) draws. Both the composite UV shift and the NGX jitter
          // offsets then read the SAME stored value -> rendered jitter == reported jitter.
          // (Calling it in RunDLAA caused a 1-frame mismatch: the composite used J_{N-1}
          // while NGX got J_N, so DLSS could never align its temporal history.)
          UpdateJitter(d);
        }
        if (shader_injection.dlaa_enabled < 0.5f) return;
        if (!senkiseki3::dlss::ngx.initialized && !senkiseki3::dlss::ngx.init_failed) {
          if (shader_injection.dlaa_debug_logging > 0.5f)
            reshade::log::message(reshade::log::level::info, "[DLAA] Attempting NGX init...");
          auto* nd = reinterpret_cast<ID3D11Device*>(dev->get_native());
          bool ok = senkiseki3::dlss::InitDLSS(nd, (uint32_t)d->swapchain_w, (uint32_t)d->swapchain_h);
          if (!ok)
            reshade::log::message(reshade::log::level::error, "[DLAA] NGX init failed");
          else if (shader_injection.dlaa_debug_logging > 0.5f)
            reshade::log::message(reshade::log::level::info, "[DLAA] NGX init succeeded");
        }
        static int fc = 0;
        if (shader_injection.dlaa_debug_logging > 0.5f && (++fc <= 3 || (fc % 300 == 0)))
          reshade::log::message(reshade::log::level::info,
            (std::string("[DLAA] Present frame ") + std::to_string(fc)).c_str());
      });

      reshade::log::message(reshade::log::level::info, "[Senkiseki3 DLAA] Addon loaded");

      reshade::register_event<reshade::addon_event::init_device>([](reshade::api::device* dev) {
        dev->create_private_data<DeviceData>();
      });
      reshade::register_event<reshade::addon_event::destroy_device>([](reshade::api::device* dev) {
        auto* d = dev->get_private_data<DeviceData>();
        if (d) { Destroy(dev, d); dev->destroy_private_data<DeviceData>(); }
      });
      break;
    case DLL_PROCESS_DETACH:
      senkiseki3::dlss::ShutdownDLSS();
      reshade::unregister_addon(h_module);
      break;
  }
  renodx::utils::settings::Use(fdw_reason, &settings);
  renodx::mods::shader::Use(fdw_reason, custom_shaders, &shader_injection);
  return TRUE;
}
