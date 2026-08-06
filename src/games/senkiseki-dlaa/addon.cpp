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
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <deque>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <deps/imgui/imgui.h>
#include <include/reshade.hpp>
#include <embed/shaders.h>

#include "../../mods/shader.hpp"
#include "../../utils/settings.hpp"
#include "../../utils/shader.hpp"
#include "../../utils/path.hpp"
#include "./shared.h"
#include "./dlss/dlss.hpp"
#include "./dxbc_patch.hpp"

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
    .dlaa_exclude_effects = 0.f,
    .dlaa_phase0_logging = 0.f,
};

// ── Phase B isolation toggles (addon-side floats, NOT in ShaderInjectData —
// adding fields there would resize the injected cbuffer and break the PS). ──
static float g_phaseb_enabled = 1.f;  // master: run the generic skinned-VS patcher
static float g_phaseb_outline = 1.f;  // include the GameEdgeParameters outline offset
static float g_phaseb_no_bind = 0.f;  // smoke-test: patched VS reads only the game's own t0/b0
                                      // (no addon t#/b# binds -> no leak, no extra SRV/CB read)
static float g_phaseb_minimal = 0.f;  // purest isolation: injected block reads NO resources at
                                      // all — prevClip = game's cb0[10..13] x float4(v0,1)
static float g_phaseb_no_output = 0.f;  // crash-bisection: injected block runs but adds NO
                                        // prevClip output register (PS v7 undefined again)
static float g_phaseb_constant = 0.f;   // crash-bisection: minimal but o7 = (0,0,0,1) constant,
                                        // zero reads at all
static float g_phaseb_dump = 0.f;     // evidence capture: write patched/original blobs + drawtrace to renodx-dev/dump/phaseb/

// ── Descriptor table helpers ──
constexpr uint32_t kTableParamCount = 6u;
using DTSet = std::array<reshade::api::descriptor_table, kTableParamCount>;

static void FreeTables(reshade::api::device* dev, DTSet& t) {
  for (auto& tb : t) { if (tb.handle) { dev->free_descriptor_table(tb); tb = {}; } }
}

// ── Log dedup / rate limiting ─────────────────────────────────────────────
// Hot diagnostics used to spam ReShade.log: per-draw/per-upload lines replay
// on every device re-create, and under a crash loop the whole frame sequence
// logs again and again. All high-frequency sites go through LogThrottled
// instead of raw log::message:
//   * the first `first` occurrences are logged verbatim (context matters);
//   * afterwards at most one line per `interval` occurrences, each annotated
//     with how many repeats were suppressed since the last emit ("(+N suppressed)");
//     interval == 0 disables the periodic re-log (hard cap at `first`).
// `key` is a compile-time site string so call sites never collide. State is
// process-global (survives device re-creates), so each process's output per
// site stays bounded no matter how often the GPU/device resets.
struct LogThrottleState {
  uint32_t count = 0;
  uint32_t last_emit = 0;
  uint32_t suppressed = 0;
};
static void LogThrottled(const char* key, reshade::log::level level,
                         uint32_t first, uint32_t interval, const char* fmt, ...) {
  static std::mutex mtx;
  static std::unordered_map<std::string, LogThrottleState> state;
  bool emit;
  uint32_t suppressed = 0u;
  {
    std::lock_guard<std::mutex> lock(mtx);
    auto& t = state[key];
    ++t.count;
    emit = (t.count <= first) || (interval > 0u && (t.count - t.last_emit) >= interval);
    if (!emit) {
      ++t.suppressed;
      return;
    }
    suppressed = t.suppressed;
    t.suppressed = 0u;
    t.last_emit = t.count;
  }
  char buf[512];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  if (suppressed != 0u) {
    size_t n = strlen(buf);
    snprintf(buf + n, sizeof(buf) - n, " (+%u suppressed)", suppressed);
  }
  reshade::log::message(level, buf);
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

  // Effect/particle exclusion mask (invalid-MV opt-out for DLAA)
  reshade::api::resource effect_mask_texture = {};
  reshade::api::resource_view effect_mask_srv = {};
  reshade::api::resource_view effect_mask_rtv = {};
  reshade::api::resource_view last_rtvs[8] = {};  // last bound RT set (for mask re-bind)
  uint32_t last_rtv_count = 0;
  reshade::api::resource_view last_dsv = {};
  bool effect_mask_cleared_this_frame = false;

  // Per-object motion target (r32g32b32a32_float): the patched char PS writes
  // prevNDC + valid-flag to an APPENDED RTV (o3), so we never touch the game's
  // 8-bit MRT2 (which is an encoded-depth target — the old 0x0E8BC215 overwrote
  // it and corrupted whatever reads it). Appended only during character draws
  // whose VS outputs prevClip (TEXCOORD5).
  reshade::api::resource motion_texture = {};
  reshade::api::resource_view motion_srv = {};
  reshade::api::resource_view motion_rtv = {};
  bool motion_cleared_this_frame = false;

  // ── Generic DXBC patcher (Phase B): per-object MVs WITHOUT per-shader HLSL ──
  // On create_pipeline we run PatchSkinnedVertexShader on any skinned VS that
  // has no hand-written replacement. The patched VS re-skins with a prev-bone
  // SRV (t#) and multiplies by the addon's prevVP cbuffer (b#), emitting
  // prevClip to a new TEXCOORD. The slots are chosen per-shader (free ones), so
  // we record them here and look them up at draw time by the patched hash.
  struct PatchedVsInfo {
    uint32_t new_hash = 0u;          // CRC32 of the bytecode the pipeline now holds
    uint32_t prev_vp_cb_slot = 1u;   // prevVP cbuffer slot (dcl_constantbuffer cb#[4])
    uint32_t prev_bone_t_slot = 1u;  // prev-bone StructuredBuffer SRV slot
    uint32_t bone_game_slot = 0u;    // game's own bone StructuredBuffer slot (usually t0)
    uint32_t texcoord_index = 5u;    // prevClip TEXCOORD semantic index
    uint32_t output_reg = 0u;        // prevClip output register
    bool needs_no_binding = false;   // true: patched VS reads only game's t0/b0 (DLAAPhaseBNoBind)
  };
  // Current (patched) hash -> info, used at draw time (GetCurrentVertexShaderHash
  // returns the patched hash because renodx didn't make this replacement).
  std::unordered_map<uint32_t, PatchedVsInfo> patched_vs_by_new_hash;
  // Skip-guard at create_pipeline: both the ORIGINAL hash (VS re-created from
  // the game's original bytecode) and the NEW hash (CreateInputLayout re-fires
  // with the already-patched blob) are recorded so we never double-patch.
  std::unordered_set<uint32_t> patched_vs_hashes;
  // Dynamic 64-byte native cbuffer holding prev_view_proj (the unjittered
  // previous-frame ViewProjection), bound to patched VSs' prevVP slot at draw.
  ID3D11Buffer* prev_vp_cb = nullptr;
  // Addon-owned stride-64 StructuredBuffer (identity placeholder) bound to
  // patched VSs' prev-bone t# slot at draw. We bind OUR buffer — never the
  // game's slot-0 VS SRV (for some passes slot 0 is a texture, and feeding a
  // texture to ld_structured t#,64 is a descriptor-kind mismatch that TDRs).
  ID3D11Buffer* prev_bones_buffer = nullptr;
  ID3D11ShaderResourceView* prev_bones_srv = nullptr;

  // ── Phase D: per-character prev-bone twins (real per-object MVs) ──
  // The game uploads a per-character bone StructuredBuffer (stride 64) at VS t0
  // every frame. For each distinct bone-buffer handle we keep an addon-owned
  // same-size structured buffer + SRV (the "twin"). At present we
  // CopyResource(game buffer -> twin), so the NEXT frame's patched VS reads the
  // PREVIOUS frame's bone matrices at its prev-bone t# slot -> prevClip
  // reflects real animation/limb motion, not just camera motion. Keyed ONLY by
  // the bone-buffer handle (no VS hash). Gated by DLAAPerObjectMotion = Prev-Bone.
  struct PrevBoneTwin {
    ID3D11Buffer* twin_buffer = nullptr;   // addon-owned structured buffer (copy dest)
    ID3D11ShaderResourceView* twin_srv = nullptr;
    ID3D11Buffer* game_buffer = nullptr;   // game's bone buffer (copy source, AddRef'd)
    uint64_t handle = 0u;                  // game bone-buffer handle (key)
    uint32_t size = 0u;
    uint32_t stride = 0u;
    uint64_t last_seen_frame = 0u;
  };
  std::vector<PrevBoneTwin> prev_bone_twins;
  ID3D11ShaderResourceView* last_bound_twin_srv = nullptr;  // twin bound this draw (for restore)

  // ── Phase 0 prev-pose probe state (separate DLAAPhase0Logging toggle) ──
  // Last vertex-stage t0 SRV push = the game's bone StructuredBuffer. The draw
  // hook correlates it with the current per-object VS hash to tell skinned vs
  // World-only VSs, detect one buffer shared across characters, and (new) read
  // the bound vertex buffer's BindFlags to tell GPU-skinned (UAV) from
  // CPU-skinned buffers — the Luma-style prev-vertex capture only works if the
  // char vertex buffers are UAV-flagged.
  reshade::api::resource_view last_bone_srv = {};  // bone StructuredBuffer SRV at VS t0
  reshade::api::resource last_bone_res = {};       // its underlying resource
  uint32_t last_bone_size = 0u;                    // buffer byte size
  uint32_t last_bone_stride = 0u;                  // element stride (64 for float4x4)
  // Per per-object VS hash -> bone buffer handle seen (0 = none = World-based).
  // Reveals handle reuse across characters and which VSs are truly skinned.
  std::unordered_map<uint32_t, uint64_t> p0_vs_bone_handle;
  // Per per-object VS hash -> vertex-buffer BindFlags (bitfield). A UAV bit
  // (D3D11_BIND_UNORDERED_ACCESS = 0x40) means the game GPU-skins into this
  // buffer, so a Luma-style prev-vertex capture is feasible for that mesh.
  std::unordered_map<uint32_t, uint32_t> p0_vs_vb_bind_flags;

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
  // Global jitter (Jitter Method = Global): unjittered ViewProjection captured
  // from the game's b0 _Globals upload, plus the last VP write state so the
  // per-draw bind-time write only happens when the jitter state actually flips.
  std::array<float, 16> globals_unjittered_vp = {};
  bool globals_vp_captured = false;
  bool globals_last_vp_jittered = false;
  // Per-buffer record of the UNJITTERED VP captured from each _Globals upload.
  // Used by the draw-time effect un-jitter so an effect draw reverts to its OWN
  // buffer's unjittered VP (never a stale one from another buffer).
  struct GlobalsBufferRec {
    reshade::api::resource res = {};
    std::array<float, 16> unjittered_vp = {};
    bool depth_only = false;  // character depth/shadow pass buffer: keep VP UNJITTERED
  };
  std::vector<GlobalsBufferRec> globals_recs;
  uint32_t globals_patch_count = 0;  // _Globals uploads captured this frame (diag)
  uint32_t globals_map_count = 0;    // large buffers mapped this frame (diag)
  reshade::api::resource last_b0_buffer = {};  // last b0 CB bound (global jitter writes VP here at draw time)
  bool in_own_upload = false;  // recursion guard: we're re-issuing a jittered _Globals upload
  uint32_t frame_index = 0u;
  bool resources_created = false;
  // Crash capture ring buffer: every draw is recorded in memory (bounded); when
  // the GPU dies (velocity pipeline creation fails with DEVICE_REMOVED, or a
  // present fails), the ring is flushed to drawtrace.log. This captures the
  // EXACT draws around the fault — the old 4000-draw file window closed before
  // the async GPU fault surfaced, missing the faulting draw entirely.
  std::deque<std::string> crash_ring;
  bool crash_ring_dumped = false;
  // Save/restore of the patched VS's extra t#/b# slots: the patched VS reads an
  // addon structured SRV (prev-bones) + cbuffer (prevVP) at slots that were
  // "free" for the char VSs, but leaving them bound leaks our structured SRV
  // into LATER draws whose VS may read that slot as a Texture2D -> GPU fault
  // (TDR). We save the game's prior bindings here and restore them at the next
  // draw (only if our placeholder is still bound, i.e. the game didn't rebind).
  ID3D11ShaderResourceView* prev_bone_saved_srv = nullptr;
  ID3D11Buffer* prev_vp_saved_cb = nullptr;
  uint32_t prev_bone_slot = 0u;
  uint32_t prev_vp_slot = 0u;
  bool patched_bind_restore_pending = false;

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
  dv(d->effect_mask_srv); dv(d->effect_mask_rtv); dr(d->effect_mask_texture);
  dv(d->motion_srv); dv(d->motion_rtv); dr(d->motion_texture);
  d->last_rtv_count = 0; d->last_dsv = {};
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
  if (d->prev_vp_cb) { d->prev_vp_cb->Release(); d->prev_vp_cb = nullptr; }
  if (d->prev_bones_srv) { d->prev_bones_srv->Release(); d->prev_bones_srv = nullptr; }
  if (d->prev_bones_buffer) { d->prev_bones_buffer->Release(); d->prev_bones_buffer = nullptr; }
  for (auto& t : d->prev_bone_twins) {
    if (t.twin_srv) t.twin_srv->Release();
    if (t.twin_buffer) t.twin_buffer->Release();
    if (t.game_buffer) t.game_buffer->Release();
  }
  d->prev_bone_twins.clear();
  d->last_bound_twin_srv = nullptr;
  if (d->prev_bone_saved_srv) { d->prev_bone_saved_srv->Release(); d->prev_bone_saved_srv = nullptr; }
  if (d->prev_vp_saved_cb) { d->prev_vp_saved_cb->Release(); d->prev_vp_saved_cb = nullptr; }
  d->prev_bone_slot = 0u;
  d->prev_vp_slot = 0u;
  d->patched_bind_restore_pending = false;
  d->patched_vs_by_new_hash.clear();
  d->patched_vs_hashes.clear();
  d->resources_created = false;
}

// ── Create velocity pipeline ──
// ── Velocity pipeline (motion_velocity.cs_5_0) ──
// (FlushCrashRing + MaybeTraceCrashWindow are defined below the draw hooks;
// CreateVelocityPipeline calls FlushCrashRing when the device is found dead.)
static void FlushCrashRing(DeviceData* d);

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
  DR mask_r    = {0,2,0,1, DS::all_compute, 1, DT::texture_shader_resource_view};                    // t2 (effect mask)
  DR uav_r     = {0,0,0,1, DS::all_compute, 1, DT::texture_unordered_access_view};
  reshade::api::constant_range pc_range = {};
  pc_range.binding = 0;
  pc_range.dx_register_index = 13;
  pc_range.count = 48;  // matches motion_velocity.cs_5_0.hlsl cbuffer (12 float4s)
  pc_range.visibility = DS::all_compute;

  P params[7];
  params[0].type = PT::descriptor_table; params[0].descriptor_table = {1, &sampler_r};
  params[1].type = PT::descriptor_table; params[1].descriptor_table = {1, &cbv_r};
  params[2].type = PT::descriptor_table; params[2].descriptor_table = {1, &srv_r};
  params[3].type = PT::descriptor_table; params[3].descriptor_table = {1, &motion_r};
  params[4].type = PT::descriptor_table; params[4].descriptor_table = {1, &uav_r};
  params[5].type = PT::descriptor_table; params[5].descriptor_table = {1, &mask_r};
  params[6].type = PT::push_constants;   params[6].push_constants = pc_range;

  if (!dev->create_pipeline_layout(7, params, &d->velocity_layout)) {
    // Unconditional step diagnostics: the dispatch never ran because this (or a
    // later step) fails — the log must say which one.
    LogThrottled("vel-step-layout", reshade::log::level::warning, 3u, 60u,
                 "[DLAA] veloc: create_pipeline_layout FAILED (pc=13,count=48)");
    return false;
  }
  for (uint32_t i = 0; i < 6u; ++i)
    if (!dev->allocate_descriptor_table(d->velocity_layout, i, &d->velocity_tables[i])) {
      LogThrottled("vel-step-table", reshade::log::level::warning, 3u, 60u,
                   "[DLAA] veloc: allocate_descriptor_table %u FAILED", i);
      return false;
    }

  reshade::api::shader_desc cs_desc = {};
  cs_desc.code = __motion_velocity.data();
  cs_desc.code_size = __motion_velocity.size();
  cs_desc.entry_point = "main";
  reshade::api::pipeline_subobject so = {reshade::api::pipeline_subobject_type::compute_shader, 1, &cs_desc};
  const bool pipe_ok = dev->create_pipeline(d->velocity_layout, 1, &so, &d->velocity_pipeline);
  if (!pipe_ok) {
    LogThrottled("vel-step-pipe", reshade::log::level::warning, 3u, 60u,
                 "[DLAA] veloc: create_pipeline(CS) FAILED (code_size=%zu)", __motion_velocity.size());
    // create_pipeline(CS) FAILED means the device was ALREADY removed by an
    // async GPU fault — the ring buffer holds the exact draws around that fault.
    FlushCrashRing(d);
  }
  return pipe_ok;
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

// ── Effect/particle exclusion mask ──
// Effect passes (particle, transparent, water, etc.) write 1.0 into a full-res
// mask via an appended render target. The velocity compute reads it and writes
// an OFF-SCREEN motion vector there, so DLSS falls back to the current input
// frame (no temporal history) — kills shimmer/ghosting on content DLSS can't
// resolve (particles, transparency).
static const std::array<uint32_t, 3> EFFECT_PS_HASHES = {
    0xD589DF82u,  // particle (0x7D3553A7)
    0x720FE34Cu,  // water/particle (0x8AFF0B4F)
    0xC1BF7F2Eu,  // effect (0x795F3AD3)
    // NOTE: transparent textures (0x728F5ED1 / VS 0xC8FE8FC4) are deliberately
    // NOT excluded. They are static world content that shares the camera's
    // motion with the background, so the velocity compute gives them correct
    // camera MVs and DLAA resolves them cleanly. Excluding them made the masked
    // region show the RAW JITTERED background (single sample, no temporal
    // accumulation) -> everything behind the texture shook/jittered.
};

static bool IsEffectPs(uint32_t hash) {
  for (uint32_t h : EFFECT_PS_HASHES) if (h == hash) return true;
  return false;
}

// VSs paired with the excluded effect PSs — these must NOT apply the DLAA
// rasterization jitter (they render as a native current-frame fallback).
static const std::array<uint32_t, 4> EFFECT_VS_HASHES = {
    0x7D3553A7u,  // particle
    0x8AFF0B4Fu,  // water/particle
    0x795F3AD3u,  // world-space effect
    0xC8FE8FC4u,  // transparent texture
};

static bool IsEffectVs(uint32_t hash) {
  for (uint32_t h : EFFECT_VS_HASHES) if (h == hash) return true;
  return false;
}

// Character depth/shadow-casting VSs (skinned, output only SV_POSITION; write
// the character's depth into the MAIN depth buffer). These read
// scene.ViewProjection (c10) which the global source patch jitters — but their
// shadow consumer compares against an UNJITTERED projection, so a jittered
// depth pass makes the character shadow move/shake (8px under the jitter test).
// Per-VS mode leaves them unjittered and shadows are stable, so in global mode
// we mark their buffers and keep them UNJITTERED (skip the source patch).
static DeviceData::GlobalsBufferRec* FindGlobalsRec(DeviceData* d, reshade::api::resource res);  // defined below
static const std::array<uint32_t, 4> DEPTH_VS_HASHES = {
    0xAA8821ECu,  // character shadow (user-reported)
    0x78F969DDu,  // character depth
    0xA1D82AA3u,  // character depth
    0x600D64CCu,  // character depth
};

static bool IsDepthVs(uint32_t hash) {
  for (uint32_t h : DEPTH_VS_HASHES) if (h == hash) return true;
  return false;
}

// Mark the current b0 _Globals buffer as a character-depth-pass buffer (VP must
// stay UNJITTERED). Called at draw time when the bound VS is a depth VS.
static void MarkDepthBuffer(DeviceData* d, reshade::api::resource res) {
  if (!d || !res.handle) return;
  auto* rec = FindGlobalsRec(d, res);
  if (!rec) {
    if (d->globals_recs.size() < 128) {
      d->globals_recs.push_back({});
      d->globals_recs.back().res = res;
      d->globals_recs.back().depth_only = true;
    }
    return;
  }
  rec->depth_only = true;
}

// Create the full-res effect mask (r16g16_float, RT + SRV).
static bool EnsureEffectMask(reshade::api::device* dev, DeviceData* d, uint32_t w, uint32_t h) {
  if (d->effect_mask_texture.handle) return true;
  if (!dev || !d) return false;
  reshade::api::resource_desc rd = {};
  rd.type = reshade::api::resource_type::texture_2d;
  rd.texture = {w, h, 1, 1, reshade::api::format::r16g16_float, 1};
  rd.heap = reshade::api::memory_heap::gpu_only;
  rd.usage = reshade::api::resource_usage::render_target | reshade::api::resource_usage::shader_resource;
  if (!dev->create_resource(rd, nullptr, reshade::api::resource_usage::render_target, &d->effect_mask_texture)) return false;
  reshade::api::resource_view_desc rv(reshade::api::resource_view_type::texture_2d, reshade::api::format::r16g16_float, 0, 1, 0, 1);
  dev->create_resource_view(d->effect_mask_texture, reshade::api::resource_usage::render_target, rv, &d->effect_mask_rtv);
  dev->create_resource_view(d->effect_mask_texture, reshade::api::resource_usage::shader_resource, rv, &d->effect_mask_srv);
  return d->effect_mask_rtv.handle && d->effect_mask_srv.handle;
}

// Append the effect-mask RT to effect passes (called right before their draw).
static void MaybeAppendEffectMask(reshade::api::command_list* cmd_list, DeviceData* d) {
  if (shader_injection.dlaa_exclude_effects < 0.5f) return;
  if (!cmd_list || !d || !d->effect_mask_rtv.handle || d->last_rtv_count == 0u) return;
  auto* state = renodx::utils::shader::GetCurrentState(cmd_list);
  uint32_t phash = state ? renodx::utils::shader::GetCurrentPixelShaderHash(state) : 0u;
  if (!IsEffectPs(phash)) return;
  auto* dev = cmd_list->get_device();
  if (!dev || !d->last_rtvs[0].handle) return;
  // D3D11 requires all bound RTs to share identical dimensions — only append
  // the mask to full-res passes. Low-res passes (e.g. particles rendered into
  // a 341x341 buffer) are skipped rather than breaking the draw.
  reshade::api::resource_desc mr = dev->get_resource_desc(d->effect_mask_texture);
  reshade::api::resource_desc rd = dev->get_resource_desc(dev->get_resource_from_view(d->last_rtvs[0]));
  if (rd.type != reshade::api::resource_type::texture_2d) return;
  if (rd.texture.width != mr.texture.width || rd.texture.height != mr.texture.height) return;
  // Clear once per frame before the first effect pass.
  if (!d->effect_mask_cleared_this_frame) {
    const float black[4] = {0.f, 0.f, 0.f, 0.f};
    cmd_list->clear_render_target_view(d->effect_mask_rtv, black);
    d->effect_mask_cleared_this_frame = true;
  }
  reshade::api::resource_view rts[9];
  uint32_t n = 0;
  for (uint32_t i = 0; i < d->last_rtv_count && n < 8u; ++i)
    if (d->last_rtvs[i].handle) rts[n++] = d->last_rtvs[i];
  if (n == 0u || n >= 8u) return;
  // The effect PS writes the mask to SV_TARGET1, so the mask RT must sit at
  // RTV index 1 — NOT appended at the end (that only aligned for single-RT
  // passes; multi-RT effect passes never wrote the mask and stayed in DLAA).
  // Shift the pass's own RT1..N down one slot to make room for it.
  for (uint32_t i = n; i > 1; --i) rts[i] = rts[i - 1];
  rts[1] = d->effect_mask_rtv;
  ++n;
  cmd_list->bind_render_targets_and_depth_stencil(n, rts, d->last_dsv);
}

// ── Per-object motion (Stage 1): dedicated 16-bit target ──
// VSs whose replacement outputs prevClip in TEXCOORD5 (o7/o8). All the skinned
// character-part VSs are patched to emit prevClip; the paired PS must be
// patched to write o3/SV_TARGET3 (otherwise that part falls back to camera
// motion). Full list = the character's mesh parts (hair, skin, clothing, eyes,
// outline, face).
static const std::array<uint32_t, 24> PER_OBJECT_MOTION_VS_HASHES = {
    0x0D5DABC6u,  // main skinned character (face)
    0xB2F338C8u,  // skinned
    0xB1C24E2Au,  // skinned
    0xBCB30859u,  // skinned
    0x5C1A50E5u,  // hair
    0xB5759643u,  // skinned
    0x4A030C25u,  // clothing
    0x3641D444u,  // eyeball
    0xF426BC1Cu,  // skinned
    0xC8F5D77Bu,  // skin
    0x38656EB3u,  // skinned
    0xF8C9B92Du,  // clothing
    0x5E5AE3FBu,  // character outline
    0xB662509Au,  // clothing
    0x0045297Du,  // skinned (draw 39)
    0x59001D8Eu,  // skinned (draw 40)
    0x63C867BAu,  // skinned (draw 41)
    0xB0A80DEFu,  // skinned outline (draw 42)
    0x1DF2E2BBu,  // skinned (draw 45)
    0x38BCCCA0u,  // skinned (draw 52)
    0x6285DCF3u,  // skinned (draw 73)
    0xFC588329u,  // skinned (draw 80)
    0x5AA04209u,  // skinned (draw 84)
    0x835760A3u,  // skinned outline (draw 85)
};

static bool IsPerObjectMotionVs(uint32_t hash) {
  for (uint32_t h : PER_OBJECT_MOTION_VS_HASHES) if (h == hash) return true;
  return false;
}

// Create the full-res per-object motion target (r32g32b32a32_float, RT + SRV).
// 32-bit: the 16-bit target quantized prevNDC to ~0.6px steps at 1440p, which
// flickered frame-to-frame under jitter (shifting rasterization coverage made
// the quantized value step) -> DLAA accumulated the MV noise as shake. Full
// float32 removes the quantization entirely (matches the camera depth path).
static bool EnsureMotionTarget(reshade::api::device* dev, DeviceData* d, uint32_t w, uint32_t h) {
  if (d->motion_texture.handle) return true;
  if (!dev || !d) return false;
  reshade::api::resource_desc rd = {};
  rd.type = reshade::api::resource_type::texture_2d;
  rd.texture = {w, h, 1, 1, reshade::api::format::r32g32b32a32_float, 1};
  rd.heap = reshade::api::memory_heap::gpu_only;
  rd.usage = reshade::api::resource_usage::render_target | reshade::api::resource_usage::shader_resource;
  if (!dev->create_resource(rd, nullptr, reshade::api::resource_usage::render_target, &d->motion_texture)) return false;
  reshade::api::resource_view_desc rv(reshade::api::resource_view_type::texture_2d, reshade::api::format::r32g32b32a32_float, 0, 1, 0, 1);
  dev->create_resource_view(d->motion_texture, reshade::api::resource_usage::render_target, rv, &d->motion_rtv);
  dev->create_resource_view(d->motion_texture, reshade::api::resource_usage::shader_resource, rv, &d->motion_srv);
  return d->motion_rtv.handle && d->motion_srv.handle;
}

// Append the per-object motion RTV to character G-buffer draws (the patched PS
// writes o3/SV_TARGET3 = prevNDC + valid flag). The RTV goes at the END (index
// n), so the game's RT0..2 (color, normal, encoded-depth) are untouched — no
// corruption. 32-bit float: full prevNDC precision (no jitter-induced
// quantization flicker like the old 16-bit target). Cleared once per frame to a
// zero-flag sentinel.
static void MaybeAppendMotionRtv(reshade::api::command_list* cmd_list, DeviceData* d) {
  if (shader_injection.dlaa_per_object_motion < 0.5f) return;
  // G-buffer only: require the game's color/normal/depth MRT set (>=3 RTs).
  // Depth/shadow/effect passes bind fewer RTs and must never get the appended
  // motion RTV (the generic patcher now touches many more VSs than the old
  // 24-hash list, so this guard matters).
  if (!cmd_list || !d || !d->motion_rtv.handle || d->last_rtv_count < 3u) return;
  auto* state = renodx::utils::shader::GetCurrentState(cmd_list);
  uint32_t vh = state ? renodx::utils::shader::GetCurrentVertexShaderHash(state) : 0u;
  // Gate on BOTH:
  //  (a) VSs the generic in-place patcher modified (patched NEW hashes), and
  //  (b) the ORIGINAL per-object char VS hashes as a fallback gate (covers the
  //      face 0x0D5DABC6 and any char VS that draws with its original hash).
  //      All skinned char VSs are generic-patched (they are NOT in
  //      custom_shaders — the boot-HLSL VS replacements don't emit prevClip,
  //      so listing them there made their paired PSs read garbage v7).
  if (!d->patched_vs_by_new_hash.contains(vh) && !IsPerObjectMotionVs(vh)) return;
  auto* dev = cmd_list->get_device();
  if (!dev || !d->last_rtvs[0].handle) return;
  // D3D11 requires all bound RTs to share identical dimensions — only append
  // the target to full-res passes.
  reshade::api::resource_desc mr = dev->get_resource_desc(d->motion_texture);
  reshade::api::resource_desc rd = dev->get_resource_desc(dev->get_resource_from_view(d->last_rtvs[0]));
  if (rd.type != reshade::api::resource_type::texture_2d) return;
  if (rd.texture.width != mr.texture.width || rd.texture.height != mr.texture.height) return;
  // Clear once per frame (flag=0 invalid; the patched PS writes flag=1).
  if (!d->motion_cleared_this_frame) {
    const float clear[4] = {0.f, 0.f, 0.f, 0.f};
    cmd_list->clear_render_target_view(d->motion_rtv, clear);
    d->motion_cleared_this_frame = true;
  }
  reshade::api::resource_view rts[9];
  uint32_t n = 0;
  for (uint32_t i = 0; i < d->last_rtv_count && n < 8u; ++i)
    if (d->last_rtvs[i].handle) rts[n++] = d->last_rtvs[i];
  if (n == 0u || n >= 8u) return;
  rts[n] = d->motion_rtv;
  ++n;
  cmd_list->bind_render_targets_and_depth_stencil(n, rts, d->last_dsv);
}

// ── Phase B runtime ──
// Lazily create the native 64-byte dynamic cbuffer that holds the UNJITTERED
// previous-frame ViewProjection for patched VSs' prevVP slot. vs_4_1 has only
// 14 cbuffer slots (b0..b13), so the patcher picks a FREE slot per shader
// (typically b1); we bind this buffer there at draw time.
static bool EnsurePrevVpCb(reshade::api::device* dev, DeviceData* d) {
  if (!dev || !d) return false;
  if (d->prev_vp_cb) return true;
  auto* nd = reinterpret_cast<ID3D11Device*>(dev->get_native());
  if (!nd) return false;
  D3D11_BUFFER_DESC bd = {};
  bd.ByteWidth = 64u;  // 4 x float4 (prevViewProj)
  bd.Usage = D3D11_USAGE_DYNAMIC;
  bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
  bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
  if (FAILED(nd->CreateBuffer(&bd, nullptr, &d->prev_vp_cb))) return false;
  return d->prev_vp_cb != nullptr;
}

// Lazily create the addon-owned prev-bone StructuredBuffer + SRV (stride 64,
// like the game's bone buffers). The patched VS reads it at its t# slot with
// ld_structured t#,64, so a VALID structured buffer must ALWAYS be bound there.
// We bind OUR identity placeholder instead of re-binding the game's slot-0 VS
// SRV: for some passes slot 0 is NOT a bone buffer (e.g. a texture), and
// feeding a texture to ld_structured is a descriptor-kind mismatch that TDRs
// the GPU. Phase D will copy real previous-frame bones into this buffer.
static bool EnsurePrevBonesSrv(reshade::api::device* dev, DeviceData* d) {
  if (!dev || !d) return false;
  if (d->prev_bones_srv) return true;
  auto* nd = reinterpret_cast<ID3D11Device*>(dev->get_native());
  if (!nd) return false;
  // 256 bones of identity (stride 64; the game's skinning uses the 3 columns at
  // bytes 0/16/32 and the patcher drops the 4th). Out-of-bounds reads clamp to
  // zero in D3D11, so a small buffer is safe.
  std::vector<float> identity(256u * 16u, 0.f);
  for (uint32_t b = 0u; b < 256u; ++b) {
    identity[b * 16u + 0u] = 1.f;   // row0 (byte 0)  -> pos.x
    identity[b * 16u + 5u] = 1.f;   // row1 (byte 16) -> pos.y
    identity[b * 16u + 10u] = 1.f;  // row2 (byte 32) -> pos.z
    identity[b * 16u + 15u] = 1.f;  // row3 (byte 48, unused by the patcher)
  }
  D3D11_BUFFER_DESC bd = {};
  bd.ByteWidth = (UINT)(identity.size() * sizeof(float));
  bd.Usage = D3D11_USAGE_DEFAULT;
  bd.BindFlags = D3D11_BIND_SHADER_RESOURCE;
  bd.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
  bd.StructureByteStride = 64u;
  D3D11_SUBRESOURCE_DATA init = {identity.data(), 0u, 0u};
  if (FAILED(nd->CreateBuffer(&bd, &init, &d->prev_bones_buffer))) return false;
  D3D11_SHADER_RESOURCE_VIEW_DESC sd = {};
  sd.Format = DXGI_FORMAT_UNKNOWN;
  sd.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
  sd.BufferEx.FirstElement = 0u;
  sd.BufferEx.NumElements = 256u;
  if (FAILED(nd->CreateShaderResourceView(d->prev_bones_buffer, &sd, &d->prev_bones_srv))) {
    d->prev_bones_buffer->Release();
    d->prev_bones_buffer = nullptr;
    return false;
  }
  return d->prev_bones_srv != nullptr;
}

// ── Phase D: prev-bone twin registry ──
// Find (or create) the addon-owned twin for the game bone-buffer behind the
// given SRV. Keyed ONLY by the game's bone-buffer handle (no VS hash). The twin
// is a same-size/same-stride structured buffer + SRV that we CopyResource into
// at present, so the next frame's patched VS reads the PREVIOUS frame's bones.
// Returns the twin's SRV (or nullptr if the buffer isn't a stride-64 structured
// buffer / on allocation failure). Bounded (LRU evicts the least-recently-used).
static ID3D11ShaderResourceView* EnsurePrevBoneTwin(reshade::api::device* dev, DeviceData* d,
                                                    ID3D11ShaderResourceView* game_srv) {
  if (!dev || !d || !game_srv) return nullptr;
  ID3D11Resource* res = nullptr;
  game_srv->GetResource(&res);
  if (!res) return nullptr;
  D3D11_RESOURCE_DIMENSION dim = D3D11_RESOURCE_DIMENSION_UNKNOWN;
  res->GetType(&dim);
  if (dim != D3D11_RESOURCE_DIMENSION_BUFFER) {
    res->Release();
    return nullptr;
  }
  auto* gb = static_cast<ID3D11Buffer*>(res);
  D3D11_BUFFER_DESC gd = {};
  gb->GetDesc(&gd);
  if (!(gd.MiscFlags & D3D11_RESOURCE_MISC_BUFFER_STRUCTURED) || gd.StructureByteStride != 64u) {
    res->Release();
    return nullptr;
  }
  // Key by the native buffer pointer (stable per game bone buffer).
  const uint64_t key = (uint64_t)(uintptr_t)gb;
  // Existing twin?
  for (auto& t : d->prev_bone_twins) {
    if (t.handle == key) {
      t.last_seen_frame = d->frame_index;
      res->Release();
      return t.twin_srv;
    }
  }
  // LRU evict when at capacity.
  if (d->prev_bone_twins.size() >= 64u) {
    uint64_t oldest = UINT64_MAX;
    size_t oldest_idx = 0u;
    for (size_t i = 0u; i < d->prev_bone_twins.size(); ++i) {
      if (d->prev_bone_twins[i].last_seen_frame < oldest) {
        oldest = d->prev_bone_twins[i].last_seen_frame;
        oldest_idx = i;
      }
    }
    auto& o = d->prev_bone_twins[oldest_idx];
    if (o.twin_srv) o.twin_srv->Release();
    if (o.twin_buffer) o.twin_buffer->Release();
    if (o.game_buffer) o.game_buffer->Release();
    d->prev_bone_twins.erase(d->prev_bone_twins.begin() + oldest_idx);
  }
  // Create the twin (DEFAULT structured buffer, same size/stride + SRV).
  auto* nd = reinterpret_cast<ID3D11Device*>(dev->get_native());
  if (!nd) { res->Release(); return nullptr; }
  DeviceData::PrevBoneTwin t;
  D3D11_BUFFER_DESC bd = {};
  bd.ByteWidth = gd.ByteWidth;
  bd.Usage = D3D11_USAGE_DEFAULT;
  bd.BindFlags = D3D11_BIND_SHADER_RESOURCE;
  bd.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
  bd.StructureByteStride = 64u;
  if (FAILED(nd->CreateBuffer(&bd, nullptr, &t.twin_buffer))) { res->Release(); return nullptr; }
  D3D11_SHADER_RESOURCE_VIEW_DESC sd = {};
  sd.Format = DXGI_FORMAT_UNKNOWN;
  sd.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
  sd.BufferEx.FirstElement = 0u;
  sd.BufferEx.NumElements = gd.ByteWidth / 64u;
  if (FAILED(nd->CreateShaderResourceView(t.twin_buffer, &sd, &t.twin_srv))) {
    t.twin_buffer->Release();
    res->Release();
    return nullptr;
  }
  gb->AddRef();  // keep the copy source alive across frames
  t.game_buffer = gb;
  t.handle = key;
  t.size = gd.ByteWidth;
  t.stride = 64u;
  t.last_seen_frame = d->frame_index;
  d->prev_bone_twins.push_back(t);
  res->Release();
  return t.twin_srv;
}

// ── Phase D: frame-end prev-bone capture ──
// CopyResource(game bone buffer -> twin) for every registered bone handle, so
// the next frame's patched VS re-skins with the PREVIOUS frame's bones. Called
// at present (after all draws of frame N). Gated by DLAAPerObjectMotion =
// Prev-Bone (>= 1.5). Uses the native immediate context.
static void CapturePrevBones(reshade::api::command_list* cmd_list, DeviceData* d) {
  if (!cmd_list || !d) return;
  if (shader_injection.dlaa_per_object_motion < 1.5f) return;  // Prev-Bone option only
  if (d->prev_bone_twins.empty()) return;
  auto* ctx = reinterpret_cast<ID3D11DeviceContext*>(cmd_list->get_native());
  if (!ctx) return;
  for (auto& t : d->prev_bone_twins) {
    if (t.twin_buffer && t.game_buffer) ctx->CopyResource(t.twin_buffer, t.game_buffer);
  }
}

// (Crash-window trace now lives in MaybeTraceCrashWindow below the draw hooks.)
// Forward decl: evidence dump folder resolver (defined with the dump helpers).
static const std::string& GetPhasebDir();

// Draw-time binding for patched skinned VSs: bind the CURRENT bone buffer to
// the patched VS's prev-bone t# slot and the addon's prevVP cbuffer to its b#
// slot. B.4 smoke test uses t# = CURRENT bones (identity prev, reproduces the
// old current-pose motion); Phase D replaces it with true per-bone-buffer
// prev twins. Uses the native context (bind_shader_resource_views is not in
// the public command_list API).
// Crash-window trace, INDEPENDENT of the per-object-motion toggle (the crash
// reproduces with it off too, so the trace must not be gated by it). Runs for
// every draw whose current VS is a patched one, when evidence capture is on.
// Crash capture: record every draw into an in-memory ring buffer (bounded so
// memory stays flat). The ring is dumped to drawtrace.log exactly when the GPU
// device dies, so the LAST lines name the faulting draw. Runs for every draw
// when DLAAPhaseBDump is on, INDEPENDENT of the per-object-motion toggle (the
// crash reproduces with it off too, so the trace must not be gated by it).
static void MaybeTraceCrashWindow(reshade::api::command_list* cmd_list, DeviceData* d) {
  if (g_phaseb_dump < 0.5f || !cmd_list || !d) return;
  auto* state = renodx::utils::shader::GetCurrentState(cmd_list);
  uint32_t vh = state ? renodx::utils::shader::GetCurrentVertexShaderHash(state) : 0u;
  uint32_t ph = state ? renodx::utils::shader::GetCurrentPixelShaderHash(state) : 0u;
  bool patched = d->patched_vs_by_new_hash.contains(vh);
  char line[160];
  snprintf(line, sizeof(line), "f=%u vs=0x%08X ps=0x%08X rtvs=%u%s%s",
           d->frame_index, vh, ph, d->last_rtv_count,
           patched ? " PATCHED" : "",
           d->motion_rtv.handle ? " motionRTV=1" : "");
  d->crash_ring.push_back(line);
  if (d->crash_ring.size() > 2048u) d->crash_ring.pop_front();
  // Unbounded patched-draw log: the fault (minimal mode is stateless) is at a
  // PATCHED draw, so the LAST line here before the crash names it, even if the
  // ring overflows with thousands of post-fault draws before device-removal is
  // detected. Appended every patched draw, never capped.
  if (patched) {
    char ppath[1024];
    snprintf(ppath, sizeof(ppath), "%s/patched.log", GetPhasebDir().c_str());
    FILE* pf = nullptr;
    fopen_s(&pf, ppath, "a");
    if (pf) {
      fprintf(pf, "f=%u vs=0x%08X ps=0x%08X rtvs=%u%s\n", d->frame_index, vh, ph,
              d->last_rtv_count, d->motion_rtv.handle ? " motionRTV=1" : "");
      fclose(pf);
    }
  }
}

// Dump the crash ring to drawtrace.log (once per device lifetime). Called when
// the GPU is detected dead (velocity pipeline creation fails) or at present
// failure. Overwrites the file so each run's capture is clean.
static void FlushCrashRing(DeviceData* d) {
  if (!d || d->crash_ring_dumped || d->crash_ring.empty()) return;
  d->crash_ring_dumped = true;
  char path[1024];
  snprintf(path, sizeof(path), "%s/drawtrace.log", GetPhasebDir().c_str());
  FILE* f = nullptr;
  fopen_s(&f, path, "w");
  if (f) {
    for (const auto& line : d->crash_ring)
      if (fputs(line.c_str(), f) >= 0 && fputc('\n', f) == EOF) break;
    fclose(f);
  }
  d->crash_ring.clear();
}

// Restore the game's original VS t#/b# bindings that MaybeBindPatchedVs
// overwrote on the previous patched draw. Without this, the patched VS's extra
// structured-bone SRV stays bound and a LATER draw whose VS reads that slot as
// a Texture2D faults the GPU (TDR) — the post-FXAA crash signature. Only
// restores if our placeholder is still bound at the slot (the game may have
// rebound it for its own use, in which case the leak is already gone). Must run
// BEFORE MaybeBindPatchedVs in the draw hook.
static void MaybeRestorePatchedBinds(reshade::api::command_list* cmd_list, DeviceData* d) {
  if (!d || !d->patched_bind_restore_pending) return;
  auto* ctx = reinterpret_cast<ID3D11DeviceContext*>(cmd_list->get_native());
  if (!ctx) return;
  ID3D11ShaderResourceView* rb_t = nullptr;
  ID3D11Buffer* rb_b = nullptr;
  ctx->VSGetShaderResources(d->prev_bone_slot, 1u, &rb_t);
  ctx->VSGetConstantBuffers(d->prev_vp_slot, 1u, &rb_b);
  const bool t_is_ours = (rb_t == d->prev_bones_srv) || (rb_t == d->last_bound_twin_srv);
  const bool b_is_ours = (rb_b == d->prev_vp_cb);
  if (t_is_ours) ctx->VSSetShaderResources(d->prev_bone_slot, 1u, &d->prev_bone_saved_srv);
  if (b_is_ours) {
    ID3D11Buffer* cb = d->prev_vp_saved_cb;
    ctx->VSSetConstantBuffers(d->prev_vp_slot, 1u, &cb);
  }
  if (rb_t) rb_t->Release();
  if (rb_b) rb_b->Release();
  if (d->prev_bone_saved_srv) { d->prev_bone_saved_srv->Release(); d->prev_bone_saved_srv = nullptr; }
  if (d->prev_vp_saved_cb) { d->prev_vp_saved_cb->Release(); d->prev_vp_saved_cb = nullptr; }
  d->prev_bone_slot = 0u;
  d->prev_vp_slot = 0u;
  d->last_bound_twin_srv = nullptr;
  d->patched_bind_restore_pending = false;
}

// Draw-time binding for patched skinned VSs: bind the addon-owned placeholder
// prev-bone buffer to the patched VS's prev-bone t# slot and the addon's
// prevVP cbuffer to its b# slot. UNCONDITIONAL (not gated by the per-object-
// motion toggle): the generic
// patcher compiles the prev-bone/prevVP reads into EVERY skinned VS, so the VS
// must always be fed valid resources — reading an unbound/stale t1 or b1 at
// draw time is a GPU fault (TDR). prev_vp_cb used to be created lazily at
// present, so frame 0's first character draw had an UNBOUND b1 (this crashed
// in BOTH toggle modes). The toggle only controls whether the motion data is
// USED (MaybeAppendMotionRtv + the velocity compute), not whether the VS gets
// its inputs. B.4 smoke test uses t# = an addon-owned identity placeholder
// (safe always-bind, no dependency on the game's slot-0 SRV); Phase D
// replaces it with true per-bone-buffer prev twins. Uses the native context
// (bind_shader_resource_views is not in the public command_list API).
static void MaybeBindPatchedVs(reshade::api::command_list* cmd_list, DeviceData* d) {
  if (!cmd_list || !d) return;
  auto* state = renodx::utils::shader::GetCurrentState(cmd_list);
  uint32_t vh = state ? renodx::utils::shader::GetCurrentVertexShaderHash(state) : 0u;
  auto it = d->patched_vs_by_new_hash.find(vh);
  if (it == d->patched_vs_by_new_hash.end()) return;
  // DLAAPhaseBNoBind (crash-bisection) mode: the patched VS reads ONLY the
  // game's own t0 bones + b0 ViewProjection — it declares no addon cb#/t# and
  // reads none, so binding anything here would only risk clobbering the game's
  // slots. Skip entirely (nothing to bind, nothing to leak, nothing to restore).
  if (it->second.needs_no_binding) {
    if (shader_injection.dlaa_debug_logging > 0.5f)
      LogThrottled("phaseB-nobind", reshade::log::level::info, 40u, 500u,
                   "[DLAA] phaseB: patched vs=0x%08X NO-BIND (game t0/b0 only) tex%u o%u",
                   vh, it->second.texcoord_index, it->second.output_reg);
    return;
  }
  // Ensure the prevVP cbuffer exists BEFORE binding (frame 0's first draw
  // happens before the first present, which was the old lazy-creation point).
  if (!d->prev_vp_cb) EnsurePrevVpCb(cmd_list->get_device(), d);
  auto* ctx = reinterpret_cast<ID3D11DeviceContext*>(cmd_list->get_native());
  if (!ctx) return;
  // Prev-bone SRV at t#: ALWAYS bind the addon-owned stride-64 structured
  // placeholder (identity). Phase D (Per-Object Motion = Prev-Bone) replaces it
  // with the PREVIOUS-frame twin of the game's CURRENT bone buffer, so the
  // patched VS re-skins with last frame's bones -> real limb motion. We bind OUR
  // buffer — never the game's slot-0 VS SRV directly — for some passes slot 0 is
  // not a bone buffer (e.g. a texture), and feeding a texture to the patched
  // VS's ld_structured t#,64 is a descriptor-kind mismatch that TDRs the GPU.
  if (!EnsurePrevBonesSrv(cmd_list->get_device(), d)) return;
  ID3D11ShaderResourceView* bone_srv = d->prev_bones_srv;
  d->last_bound_twin_srv = nullptr;
  if (shader_injection.dlaa_per_object_motion >= 1.5f) {
    // Read the game's CURRENT bone SRV at its own slot (the patcher records
    // bone_game_slot, usually t0) and bind its prev-frame twin. Only ever bind
    // our own twin SRV (never the game's SRV itself) so the ld_structured
    // descriptor kind is always valid.
    ID3D11ShaderResourceView* game_srv = nullptr;
    ctx->VSGetShaderResources(it->second.bone_game_slot, 1u, &game_srv);
    if (game_srv) {
      ID3D11ShaderResourceView* twin_srv =
          EnsurePrevBoneTwin(cmd_list->get_device(), d, game_srv);
      game_srv->Release();
      if (twin_srv) {
        bone_srv = twin_srv;
        d->last_bound_twin_srv = twin_srv;
      }
    }
  }
  // Save the game's current bindings at these slots BEFORE overwriting them:
  // MaybeRestorePatchedBinds (next draw) puts them back so our extra structured
  // SRV / prevVP CB never leak into later draws that read the slot as a
  // Texture2D/other type (that descriptor-kind mismatch TDRs the GPU).
  ID3D11ShaderResourceView* cur_t = nullptr;
  ID3D11Buffer* cur_b = nullptr;
  ctx->VSGetShaderResources(it->second.prev_bone_t_slot, 1u, &cur_t);
  ctx->VSGetConstantBuffers(it->second.prev_vp_cb_slot, 1u, &cur_b);
  if (d->prev_bone_saved_srv) d->prev_bone_saved_srv->Release();
  if (d->prev_vp_saved_cb) d->prev_vp_saved_cb->Release();
  d->prev_bone_saved_srv = cur_t;  // takes the ref
  d->prev_vp_saved_cb = cur_b;     // takes the ref
  d->prev_bone_slot = it->second.prev_bone_t_slot;
  d->prev_vp_slot = it->second.prev_vp_cb_slot;
  d->patched_bind_restore_pending = true;
  ctx->VSSetShaderResources(it->second.prev_bone_t_slot, 1u, &bone_srv);
  // PrevVP cbuffer at b#.
  if (d->prev_vp_cb) {
    ID3D11Buffer* cbs[1] = {d->prev_vp_cb};
    ctx->VSSetConstantBuffers(it->second.prev_vp_cb_slot, 1u, cbs);
  }
  if (shader_injection.dlaa_debug_logging > 0.5f) {
    LogThrottled("phaseB-bind", reshade::log::level::info, 40u, 500u,
                 "[DLAA] phaseB: bind patched vs=0x%08X cb%u=%s t%u=%s tex%u o%u",
                 vh, it->second.prev_vp_cb_slot, d->prev_vp_cb ? "prevVP" : "NONE",
                 it->second.prev_bone_t_slot, d->prev_bones_srv ? "prevBones" : "NONE",
                 it->second.texcoord_index, it->second.output_reg);
    // Read the binds back to confirm they recorded on THIS context. If the game
    // draws on a different (deferred) context, they read back NULL/OTHER and the
    // patched VS executes with an unbound t#/b# -> GPU fault (TDR).
    ID3D11ShaderResourceView* rb_srv = nullptr;
    ID3D11Buffer* rb_cb = nullptr;
    ctx->VSGetShaderResources(it->second.prev_bone_t_slot, 1u, &rb_srv);
    ctx->VSGetConstantBuffers(it->second.prev_vp_cb_slot, 1u, &rb_cb);
    LogThrottled("bind-verify", reshade::log::level::info, 40u, 500u,
                 "[DLAA] bind verify vs=0x%08X t%u=%s b%u=%s",
                 vh, it->second.prev_bone_t_slot,
                 (rb_srv == bone_srv) ? "OK" : (rb_srv ? "OTHER" : "NULL"),
                 it->second.prev_vp_cb_slot,
                 (rb_cb == d->prev_vp_cb) ? "OK" : (rb_cb ? "OTHER" : "NULL"));
    if (rb_srv) rb_srv->Release();
    if (rb_cb) rb_cb->Release();
  }
}

// ── Diagnostics: log the first ~20 effect draws (VS/PS hashes + exclusion flag).
// Tells us whether 0xC8FE8FC4-type draws are being masked (PS in EFFECT_PS_HASHES)
// and whether the exclude toggle is 1 at draw time (VS gate should fire).
static void MaybeLogEffectDraw(reshade::api::command_list* cmd_list, DeviceData* d) {
  if (!cmd_list || !d) return;
  auto* state = renodx::utils::shader::GetCurrentState(cmd_list);
  if (!state) return;
  uint32_t vhash = renodx::utils::shader::GetCurrentVertexShaderHash(state);
  uint32_t phash = renodx::utils::shader::GetCurrentPixelShaderHash(state);
  if (!IsEffectVs(vhash) && !IsEffectPs(phash) && vhash != 0xDFE5A75Du) return;
  LogThrottled("effect-draw", reshade::log::level::info, 20u, 0u,
               "[DLAA] effect draw: vs=0x%08X ps=0x%08X exclude=%d rtvs=%u",
               vhash, phash, (int)(shader_injection.dlaa_exclude_effects > 0.5f), d->last_rtv_count);
}

// Per-draw jitter control: effect draws (PS in EFFECT_PS_HASHES) are excluded
// from DLAA via the mask and render as a native current-frame fallback, so they
// must NOT receive the rasterization jitter (it would just shimmer). This runs
// BEFORE renodx's injection push (our draw hook is registered first), so the
// jitter offsets the VS reads THIS draw are exactly what we set here. Regular
// geometry is restored to the frame's canonical jitter (d->jitter_x/y).
// This is PS-driven (not VS-driven) so shared VSs like 0xDFE5A75D keep jitter
// in their geometry pass and lose it only in their effect/transparent pass.
static void ApplyPerDrawJitter(reshade::api::command_list* cmd_list, DeviceData* d) {
  if (shader_injection.dlaa_jitter_method > 0.5f) {
    // Global method: per-VS injection offsets stay 0 (the VP patch handles jitter).
    shader_injection.jitter_offset_x = 0.f;
    shader_injection.jitter_offset_y = 0.f;
    return;
  }
  auto* state = renodx::utils::shader::GetCurrentState(cmd_list);
  uint32_t phash = state ? renodx::utils::shader::GetCurrentPixelShaderHash(state) : 0u;
  if (IsEffectPs(phash)) {
    shader_injection.jitter_offset_x = 0.f;
    shader_injection.jitter_offset_y = 0.f;
  } else {
    shader_injection.jitter_offset_x = d->jitter_x;
    shader_injection.jitter_offset_y = d->jitter_y;
  }
}

// Global jitter: draw-time VP correction.
// Scene geometry is jittered reliably at the SOURCE (OnUpdateBufferRegion
// patches every _Globals upload), so scene draws need no draw-time write — and
// writing here could clobber the correctly-patched VP with a stale captured one
// from a different buffer. Draw-time only handles:
//   * effect draws (excluded from DLAA): revert to THIS buffer's own unjittered
//     VP so effects don't shimmer/move;
//   * untracked buffers (upload missed jitter, e.g. Map-updated): fallback write.
static DeviceData::GlobalsBufferRec* FindGlobalsRec(DeviceData* d, reshade::api::resource res);  // defined below
static void MaybeWriteGlobalsVp(reshade::api::command_list* cmd_list, DeviceData* d) {
  if (!cmd_list || !d) return;
  if (shader_injection.dlaa_jitter_method < 0.5f) return;
  if (shader_injection.dlaa_jitter_enabled < 0.5f && shader_injection.dlaa_jitter_test < 0.5f) return;
  // Gate-failure diagnostics: periodic (first few, then every 250th) so we can
  // see WHY the draw-time write doesn't fire in-game even after the start-menu
  // consumed the first-slot probes.
  if (!d->globals_vp_captured || !d->last_b0_buffer.handle) {
    if (shader_injection.dlaa_debug_logging > 0.5f) {
      LogThrottled("draw-skip-cap", reshade::log::level::info, 5u, 250u,
                   "[DLAA] global: draw SKIP (captured=%d last_b0=0x%llX)",
                   (int)d->globals_vp_captured, (unsigned long long)d->last_b0_buffer.handle);
    }
    return;
  }
  auto* dev = cmd_list->get_device();
  if (!dev) return;
  auto rd = dev->get_resource_desc(d->last_b0_buffer);
  auto* state = renodx::utils::shader::GetCurrentState(cmd_list);
  uint32_t vh = state ? renodx::utils::shader::GetCurrentVertexShaderHash(state) : 0u;
  // ── Character depth/shadow pass: mark its buffer (VP stays UNJITTERED) and
  // never write jitter into it. Per-VS mode leaves these unjittered and the
  // shadow is stable; the global source patch jitters c10 on ALL buffers, which
  // made the character shadow move — so we un-jitter this buffer. ──
  if (IsDepthVs(vh)) {
    MarkDepthBuffer(d, d->last_b0_buffer);
    if (shader_injection.dlaa_debug_logging > 0.5f) {
      LogThrottled("depth-vs", reshade::log::level::info, 5u, 250u,
                   "[DLAA] global: DEPTH VS 0x%08X buffer=0x%llX size=%llu (unjittered)",
                   vh, (unsigned long long)d->last_b0_buffer.handle,
                   (unsigned long long)(rd.type == reshade::api::resource_type::buffer ? rd.buffer.size : 0ull));
    }
    return;  // depth pass reads the buffer's UNJITTERED VP (source patch skips it)
  }
  if (rd.type != reshade::api::resource_type::buffer || rd.buffer.size < 768ull) {
    if (shader_injection.dlaa_debug_logging > 0.5f) {
      LogThrottled("draw-skip-small", reshade::log::level::info, 20u, 250u,
                   "[DLAA] global: draw SKIP small buffer=0x%llX size=%llu vs=0x%08X",
                   (unsigned long long)d->last_b0_buffer.handle,
                   (unsigned long long)(rd.type == reshade::api::resource_type::buffer ? rd.buffer.size : 0ull),
                   vh);
    }
    return;
  }
  uint32_t phash = state ? renodx::utils::shader::GetCurrentPixelShaderHash(state) : 0u;
  bool want_jittered = !IsEffectPs(phash);
  // Scene draw on a tracked (source-patched) buffer: already jittered, leave it.
  auto* rec = FindGlobalsRec(d, d->last_b0_buffer);
  if (want_jittered && rec) return;
  // Choose the VP to write: this buffer's own unjittered VP when known (effect
  // un-jitter), else the global captured VP (jittered for untracked scene
  // fallback, unjittered for untracked effect fallback).
  std::array<float, 16> vp = rec ? rec->unjittered_vp : d->globals_unjittered_vp;
  if (want_jittered && !rec) {
    for (int i = 0; i < 4; ++i) {
      vp[i] += d->jitter_x * vp[12 + i];
      vp[4 + i] += d->jitter_y * vp[12 + i];
    }
  }
  dev->update_buffer_region(vp.data(), d->last_b0_buffer, 160ull, 64ull);
  if (shader_injection.dlaa_debug_logging > 0.5f) {
    LogThrottled("vp-write", reshade::log::level::info, 5u, 250u,
                 "[DLAA] global: draw VP write buffer=0x%llX size=%llu jittered=%d ps=0x%08X",
                 (unsigned long long)d->last_b0_buffer.handle,
                 (unsigned long long)rd.buffer.size, (int)want_jittered, phash);
  }
}

// Phase 0 prev-pose probe (defined later; forward decl so the draw hooks call it).
static void Phase0ProbeDraw(reshade::api::command_list* cmd_list, DeviceData* d);

// Draw hooks: append the mask RT right before effect draws (the PS is
// guaranteed to be bound here, unlike at the RT-bind event).
static bool OnDrawMaskHook(reshade::api::command_list* cmd_list, uint32_t, uint32_t, uint32_t, uint32_t) {
  auto* dev = cmd_list->get_device();
  if (!dev) return false;
  auto* d = dev->get_private_data<DeviceData>();
  if (d) {
    MaybeRestorePatchedBinds(cmd_list, d);
    Phase0ProbeDraw(cmd_list, d);
    MaybeTraceCrashWindow(cmd_list, d);
    ApplyPerDrawJitter(cmd_list, d);
    MaybeWriteGlobalsVp(cmd_list, d);
    MaybeLogEffectDraw(cmd_list, d);
    MaybeAppendEffectMask(cmd_list, d);
    MaybeBindPatchedVs(cmd_list, d);
    MaybeAppendMotionRtv(cmd_list, d);
  }
  return false;
}
static bool OnDrawMaskHookIndexed(reshade::api::command_list* cmd_list, uint32_t, uint32_t, uint32_t, int32_t, uint32_t) {
  auto* dev = cmd_list->get_device();
  if (!dev) return false;
  auto* d = dev->get_private_data<DeviceData>();
  if (d) {
    MaybeRestorePatchedBinds(cmd_list, d);
    Phase0ProbeDraw(cmd_list, d);
    MaybeTraceCrashWindow(cmd_list, d);
    ApplyPerDrawJitter(cmd_list, d);
    MaybeWriteGlobalsVp(cmd_list, d);
    MaybeLogEffectDraw(cmd_list, d);
    MaybeAppendEffectMask(cmd_list, d);
    MaybeBindPatchedVs(cmd_list, d);
    MaybeAppendMotionRtv(cmd_list, d);
  }
  return false;
}

// ── Event: capture RTV0 (FXAA output target) ──
static void OnBindRenderTargets(
    reshade::api::command_list* cmd_list, uint32_t count,
    const reshade::api::resource_view* rtvs, reshade::api::resource_view dsv) {
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
  // Track the bound RT set for the effect-mask re-bind (skip our own echo).
  bool has_mask = false;
  for (uint32_t i = 0; i < count && i < 8u; ++i)
    if (d->effect_mask_rtv.handle && rtvs[i] == d->effect_mask_rtv) has_mask = true;
  if (!has_mask) {
    d->last_rtv_count = std::min<uint32_t>(count, 8u);
    for (uint32_t i = 0; i < d->last_rtv_count; ++i) d->last_rtvs[i] = rtvs[i];
    d->last_dsv = dsv;
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

  // Global jitter method: the staged copy captured the JITTERED VP (we patch
  // b0 at bind time). Prefer the exact unjittered VP captured from the game's
  // upload so the velocity reprojection stays jitter-free.
  if (shader_injection.dlaa_jitter_method > 0.5f) {
    if (d->globals_vp_captured) {
      view_proj = d->globals_unjittered_vp;
    } else {
      for (int i = 0; i < 4; ++i) {
        view_proj[i] -= d->jitter_x * view_proj[12 + i];
        view_proj[4 + i] -= d->jitter_y * view_proj[12 + i];
      }
    }
  }

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
static const std::array<uint32_t, 51> SCENE_GEOMETRY_VS_HASHES = {
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
    // Remaining char-part VSs from the 39-86 sweep (also scene geometry for the
    // b0 camera-matrix capture gate).
    0x0045297Du,  // draw 39
    0x59001D8Eu,  // draw 40
    0x63C867BAu,  // draw 41
    0xB0A80DEFu,  // draw 42 (outline)
    0x1DF2E2BBu,  // draw 45
    0x38BCCCA0u,  // draw 52
    0x6285DCF3u,  // draw 73
    0xFC588329u,  // draw 80
    0x5AA04209u,  // draw 84
    0x835760A3u,  // draw 85 (outline)
};

static bool IsSceneGeometryVs(uint32_t hash) {
  for (uint32_t h : SCENE_GEOMETRY_VS_HASHES) {
    if (h == hash) return true;
  }
  return false;
}

// ── Phase 0 prev-pose probe (separate DLAAPhase0Logging toggle) ──
// Logs the game's vertex-stage t0 SRV pushes — expected to be the per-character
// bone StructuredBuffer (stride 64 = float4x4). Answers the prev-pose design
// questions without the general Debug Logging spam:
//   1. Does the bone SRV arrive at VS t0? (handle, size, stride)
//   2. Which per-object VS hashes are skinned (see a bone buffer) vs World-only?
//   3. Is a single bone buffer REUSED across multiple characters / VS hashes?
//   4. Are there vertex-stage SRV pushes at binding > 0 (would conflict with
//      our planned prev-bone t1 slot)?
// Each distinct (stage, binding, VS hash, resource) combo logs once.
static void Phase0ProbeVertexSrv(reshade::api::device* dev, reshade::api::command_list* cmd_list,
                                 reshade::api::shader_stage stage,
                                 const reshade::api::descriptor_table_update& update,
                                 DeviceData* d) {
  if (shader_injection.dlaa_phase0_logging < 0.5f) return;
  if ((stage & reshade::api::shader_stage::vertex) != reshade::api::shader_stage::vertex) return;
  if (update.type != reshade::api::descriptor_type::texture_shader_resource_view) return;
  if (update.count < 1 || !update.descriptors) return;
  auto* views = static_cast<const reshade::api::resource_view*>(update.descriptors);
  if (!views[0].handle) return;

  auto* state = renodx::utils::shader::GetCurrentState(cmd_list);
  uint32_t vhash = state ? renodx::utils::shader::GetCurrentVertexShaderHash(state) : 0u;

  // Any vertex-stage SRV at binding > 0 would collide with our planned t1 slot.
  if (update.binding > 0u) {
    static std::unordered_set<uint64_t> logged;
    uint64_t key = ((uint64_t)update.binding << 32) | vhash;
    if (logged.insert(key).second) {
      char buf[160];
      snprintf(buf, sizeof(buf),
               "[P0] VS t%u SRV push (perobj=%d) vs=0x%08X — would conflict with prev-bone t1",
               update.binding, (int)IsPerObjectMotionVs(vhash), vhash);
      reshade::log::message(reshade::log::level::info, buf);
    }
    return;
  }

  // Binding 0: the per-draw bone StructuredBuffer (or World-only VS with no SRV).
  auto res = dev->get_resource_from_view(views[0]);
  auto rd = dev->get_resource_desc(res);
  uint64_t size = (rd.type == reshade::api::resource_type::buffer) ? rd.buffer.size : 0ull;
  uint32_t stride = (rd.type == reshade::api::resource_type::buffer) ? rd.buffer.stride : 0u;

  // Remember the most recent bone buffer for the draw-time correlation.
  d->last_bone_srv = views[0];
  d->last_bone_res = res;
  d->last_bone_size = (uint32_t)size;
  d->last_bone_stride = stride;

  // Track per per-object VS hash which bone buffer it used (0 = none seen yet).
  if (IsPerObjectMotionVs(vhash)) {
    auto& seen_handle = d->p0_vs_bone_handle[vhash];
    if (seen_handle == 0u) seen_handle = res.handle;
  }

  // Distinct (vhash, bone-handle) combos can be unbounded (per-frame dynamic
  // bone buffers) — cap the total so the per-object probe can't flood the log.
  LogThrottled("p0-srv0", reshade::log::level::info, 30u, 300u,
               "[P0] VS t0 SRV push vs=0x%08X perobj=%d bone=0x%llX size=%llu stride=%u",
               vhash, (int)IsPerObjectMotionVs(vhash),
               (unsigned long long)res.handle, (unsigned long long)size, stride);
}

// Draw-time Phase 0 correlation: for each per-object VS draw, report which bone
// buffer (if any) was bound — tells skinned vs World-only VSs, surfaces buffer
// reuse across characters, and (NEW) reads the bound VERTEX BUFFER's BindFlags
// via the native D3D11 context so we can see whether the char meshes are
// GPU-skinned (D3D11_BIND_UNORDERED_ACCESS = 0x40). GPU-skinned vertex buffers
// are what make the Luma-style prev-vertex capture (CachedSkinVertices +
// SV_VertexID) feasible — no bone-buffer twin needed.
static void Phase0ProbeDraw(reshade::api::command_list* cmd_list, DeviceData* d) {
  if (shader_injection.dlaa_phase0_logging < 0.5f) return;
  if (!cmd_list || !d) return;
  auto* state = renodx::utils::shader::GetCurrentState(cmd_list);
  if (!state) return;
  uint32_t vhash = renodx::utils::shader::GetCurrentVertexShaderHash(state);
  if (!IsPerObjectMotionVs(vhash)) return;
  uint32_t phash = renodx::utils::shader::GetCurrentPixelShaderHash(state);

  // Skinned = a bone SRV was seen for this VS; World-only = none (no entry).
  auto it = d->p0_vs_bone_handle.find(vhash);
  uint64_t bone = (it != d->p0_vs_bone_handle.end()) ? it->second : 0ull;

  // NEW: read the bound vertex buffer (slot 0) and its BindFlags. Only valid on
  // the native immediate context.
  uint32_t vb_bind_flags = 0u;
  {
    auto* native_ctx = reinterpret_cast<ID3D11DeviceContext*>(cmd_list->get_native());
    if (native_ctx) {
      ID3D11Buffer* vb = nullptr;
      UINT vb_stride = 0, vb_offset = 0;
      native_ctx->IAGetVertexBuffers(0, 1, &vb, &vb_stride, &vb_offset);
      if (vb) {
        D3D11_BUFFER_DESC bd = {};
        vb->GetDesc(&bd);
        vb_bind_flags = bd.BindFlags;
        vb->Release();
        d->p0_vs_vb_bind_flags[vhash] = bd.BindFlags;
      }
    }
  }
  bool gpu_skinned = (vb_bind_flags & D3D11_BIND_UNORDERED_ACCESS) != 0;

  LogThrottled("p0-draw", reshade::log::level::info, 30u, 300u,
               "[P0] per-object draw vs=0x%08X ps=0x%08X bone=0x%llX size=%u stride=%u vbBindFlags=0x%X %s%s",
               vhash, phash, (unsigned long long)bone,
               d->last_bone_size, d->last_bone_stride, vb_bind_flags,
               bone ? "skinned" : "WORLD-only (no bone SRV seen)",
               gpu_skinned ? " GPU-SKINNED(UAV)" : "");
}

// Per-frame Phase 0 summary: distinct bone buffers + vertex-buffer BindFlags
// per per-object VS. Rate-limited (every ~60 frames) so one-shot logs stand out.
static void Phase0ProbeFrameSummary(DeviceData* d) {
  if (shader_injection.dlaa_phase0_logging < 0.5f) return;
  if (!d) return;
  static uint32_t summary_counter = 0;
  if (++summary_counter % 60u != 1u) return;
  std::unordered_set<uint64_t> handles;
  uint32_t skinned = 0, world_only = 0, gpu_skinned = 0, cpu_skinned = 0;
  for (auto& [vh, h] : d->p0_vs_bone_handle) {
    if (h) { handles.insert(h); skinned++; }
    else world_only++;
  }
  for (auto& [vh, flags] : d->p0_vs_vb_bind_flags) {
    if (flags & D3D11_BIND_UNORDERED_ACCESS) gpu_skinned++;
    else cpu_skinned++;
  }
  char buf[300];
  snprintf(buf, sizeof(buf),
           "[P0] summary: per-object VSs=%zu skinned=%u world=%u distinctBoneBuffers=%zu vbUAV(gpu-skin)=%u vbNonUAV=%zu",
           d->p0_vs_bone_handle.size(), skinned, world_only, handles.size(),
           gpu_skinned, d->p0_vs_vb_bind_flags.size() - gpu_skinned);
  reshade::log::message(reshade::log::level::info, buf);
}

// ── Event: capture all needed descriptors (falcomengine-plus pattern: type → binding → hash) ──
static void OnPushDescriptorsCapture(
    reshade::api::command_list* cmd_list, reshade::api::shader_stage stage, reshade::api::pipeline_layout,
    uint32_t param_index, const reshade::api::descriptor_table_update& update) {
  auto* dev = cmd_list->get_device();
  if (!dev) return;
  auto* d = dev->get_private_data<DeviceData>();
  if (!d) return;

  // Phase 0 prev-pose probe: log vertex-stage SRV pushes (bone buffer) under
  // the SEPARATE DLAAPhase0Logging toggle (never the general debug logging).
  Phase0ProbeVertexSrv(dev, cmd_list, stage, update, d);

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
            LogThrottled("depth-push", reshade::log::level::info, 25u, 0u,
                         "[DLAA] Depth push: hash=0x%08X binding=%u fmt=%d %ux%u",
                         hash, update.binding, fmt,
                         (int)rd.texture.width, (int)rd.texture.height);
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
        uint32_t phash = cbv_ss ? renodx::utils::shader::GetCurrentPixelShaderHash(cbv_ss) : 0u;
        if (IsSceneGeometryVs(vhash)) {
          d->captured_scene_cbv = *cbv;
          d->captured_scene_cbv_valid = true;
          // Queue the camera-matrix staging copy EARLY so the Map at FXAA
          // (ReadSceneMatrices) doesn't stall the GPU pipeline mid-frame.
          if (shader_injection.dlaa_enabled > 0.5f && !d->scene_cbv_copy_issued) {
            d->scene_cbv_copy_issued = IssueSceneCbvCopy(cmd_list, d);
          }
        }
        // Track the last b0 buffer bound for the VERTEX stage (the VS reads the
        // ViewProjection from it). PS-stage b0 binds (material cbuffers) must
        // NOT overwrite this, or the draw-time write would target the wrong
        // (often small) buffer.
        if ((stage & reshade::api::shader_stage::vertex) == reshade::api::shader_stage::vertex) {
          d->last_b0_buffer = cbv->buffer;
          if (shader_injection.dlaa_debug_logging > 0.5f) {
            auto brd = dev->get_resource_desc(cbv->buffer);
            LogThrottled("b0-vs-bind", reshade::log::level::info, 5u, 250u,
                         "[DLAA] global: b0 VS bind buffer=0x%llX size=%llu",
                         (unsigned long long)cbv->buffer.handle,
                         (unsigned long long)(brd.type == reshade::api::resource_type::buffer ? brd.buffer.size : 0ull));
          }
        }
      }
    }
  }
}

// ── Global jitter: capture the game's unjittered ViewProjection ──
// When Jitter Method = Global, the addon jitters the SHARED scene.ViewProjection
// (b0 _Globals, c10 = bytes 160..208) instead of each replaced VS. The game
// re-uploads the full unjittered _Globals per object, so we read-only capture
// the unjittered VP here (before the upload lands) and re-issue the upload with
// the jittered VP. This works for EVERY scene VS permutation.
static DeviceData::GlobalsBufferRec* FindGlobalsRec(DeviceData* d, reshade::api::resource res) {
  for (auto& rec : d->globals_recs)
    if (rec.res.handle == res.handle) return &rec;
  return nullptr;
}

static bool OnUpdateBufferRegion(reshade::api::device* dev, const void* data,
                                 reshade::api::resource dest, uint64_t dest_offset, uint64_t size) {
  auto* d = dev ? dev->get_private_data<DeviceData>() : nullptr;
  // Recursion guard: our own re-issued upload below must land unchanged.
  if (d && d->in_own_upload) return false;
  // ── PROBE: log the first few large-buffer uploads (before any gates) so we
  // can see whether the game uploads b0 via UpdateSubresource and with what
  // offsets/sizes. ──
  if (dev && dest.handle && shader_injection.dlaa_debug_logging > 0.5f) {
    auto prd = dev->get_resource_desc(dest);
    if (prd.type == reshade::api::resource_type::buffer && prd.buffer.size >= 768ull) {
      LogThrottled("probe-upd", reshade::log::level::info, 10u, 0u,
                   "[DLAA] probe: upd buffer=0x%llX bufsize=%llu off=%llu len=%llu method=%d",
                   (unsigned long long)dest.handle, (unsigned long long)prd.buffer.size,
                   (unsigned long long)dest_offset, (unsigned long long)size,
                   (int)(shader_injection.dlaa_jitter_method > 0.5f));
    }
  }
  if (!dev || !data || !dest.handle) return false;
  if (shader_injection.dlaa_jitter_method < 0.5f) return false;
  if (shader_injection.dlaa_jitter_enabled < 0.5f && shader_injection.dlaa_jitter_test < 0.5f) return false;
  if (!d) return false;
  // Must reach the VP region (c10, bytes 160..208) — excludes small
  // post/shadow cbuffers and our own 64-byte VP writes (size < 208).
  if (dest_offset > 160ull || size < 208ull) return false;
  auto rd = dev->get_resource_desc(dest);
  if (rd.type != reshade::api::resource_type::buffer || rd.buffer.size < 768ull) return false;
  // Full-buffer (null-box) updates report UINT64_MAX: clamp to the buffer size.
  if (size == UINT64_MAX) size = rd.buffer.size;
  if (size < 208ull) return false;
  // (captured_scene_cbv gate removed: it was tied to the 41-VS hash list and
  // could miss the other per-material _Globals variants; the size + sanity
  // checks and the globals_buffers set cover all of them.)
  // Sanity: reject an empty/zero matrix. NOTE: D3D perspective projections have
  // m33 == 0, so we must NOT require m33 != 0 — that rejected every VP.
  const float* vp0 = reinterpret_cast<const float*>(
      static_cast<const uint8_t*>(data) + (size_t)(160ull - dest_offset));
  float vp_acc = 0.f;
  for (int i = 0; i < 16; ++i) vp_acc += std::fabs(vp0[i]);
  if (vp_acc < 1e-3f) return false;
  // Save the game's UNJITTERED ViewProjection for the velocity compute.
  memcpy(d->globals_unjittered_vp.data(), vp0, 64);
  d->globals_vp_captured = true;
  // Per-buffer record: this buffer's own unjittered VP (used by the draw-time
  // effect un-jitter so we never write a stale VP from another buffer).
  auto* rec = FindGlobalsRec(d, dest);
  if (!rec) {
    if (d->globals_recs.size() < 128) {
      d->globals_recs.push_back({});
      d->globals_recs.back().res = dest;
      d->globals_recs.back().unjittered_vp = d->globals_unjittered_vp;
    }
  } else {
    rec->unjittered_vp = d->globals_unjittered_vp;
  }
  // ── Character depth/shadow pass buffers: keep VP UNJITTERED ──
  // The depth VSs (DEPTH_VS_HASHES) write the character's depth into the main
  // depth buffer via scene.ViewProjection. Its shadow consumer compares against
  // the UNJITTERED projection, so jittering c10 here made the character shadow
  // move/shake (per-VS mode leaves it unjittered and shadows are stable). The
  // buffer is marked at draw time (MarkDepthBuffer); capture the unjittered VP
  // but let the game's original upload stand unchanged.
  if (rec && rec->depth_only) {
    if (shader_injection.dlaa_debug_logging > 0.5f) {
      LogThrottled("depth-unjit", reshade::log::level::info, 5u, 250u,
                   "[DLAA] global: depth buffer unjittered buffer=0x%llX size=%llu",
                   (unsigned long long)dest.handle, (unsigned long long)rd.buffer.size);
    }
    return false;  // let the game's original (unjittered) upload proceed
  }
  ++d->globals_patch_count;
  // ── SOURCE-LEVEL PATCH (reliable jitter) ──
  // We are on the immediate context right before the game's UpdateSubresource
  // lands. Copy the game's data, jitter the VP region, then BLOCK the original
  // upload and re-issue ours. The buffer now permanently contains the jittered
  // VP from the moment of upload — no dependency on draw-time writes, buffer
  // bind order, or UpdateSubresource-on-bound-buffer behavior. World (c44+) and
  // every other per-object field are copied verbatim.
  if (size > 2048ull) return false;  // _Globals are 848-1376B; skip anything larger
  uint8_t patched[2048];
  memcpy(patched, data, (size_t)size);
  float* vpd = reinterpret_cast<float*>(patched + (size_t)(160ull - dest_offset));
  const float jx = d->jitter_x, jy = d->jitter_y;
  for (int i = 0; i < 4; ++i) {
    vpd[i] += jx * vpd[12 + i];
    vpd[4 + i] += jy * vpd[12 + i];
  }
  if (shader_injection.dlaa_debug_logging > 0.5f) {
    LogThrottled("source-patch", reshade::log::level::info, 5u, 1000u,
                 "[DLAA] global: SOURCE PATCH buffer=0x%llX size=%llu jx=%.4f jy=%.4f",
                 (unsigned long long)dest.handle, (unsigned long long)size, jx, jy);
  }
  d->in_own_upload = true;
  dev->update_buffer_region(patched, dest, dest_offset, size);
  d->in_own_upload = false;
  return true;  // block the game's original (unjittered) upload
}

// ── PROBE: log the first few large-buffer Map calls. If the game uploads b0 via
// Map/Unmap instead of UpdateSubresource, update_buffer_region never fires and
// we need Map-based interception instead. ──
static void OnMapBufferRegionProbe(reshade::api::device* dev, reshade::api::resource res,
                                   uint64_t offset, uint64_t size, reshade::api::map_access access,
                                   void** data) {
  if (!dev || !res.handle || !data) return;
  if (shader_injection.dlaa_debug_logging < 0.5f) return;
  auto rd = dev->get_resource_desc(res);
  if (rd.type != reshade::api::resource_type::buffer || rd.buffer.size < 768ull) return;
  auto* d = dev->get_private_data<DeviceData>();
  if (d) ++d->globals_map_count;
  LogThrottled("probe-map", reshade::log::level::info, 10u, 250u,
               "[DLAA] probe: map buffer=0x%llX bufsize=%llu off=%llu len=%llu",
               (unsigned long long)res.handle, (unsigned long long)rd.buffer.size,
               (unsigned long long)offset, (unsigned long long)size);
}

// ── Jitter update ──
static void UpdateJitter(DeviceData* d) {
  if (!d) return;
  if (shader_injection.dlaa_jitter_test > 0.5f) {
    // Jitter Test: apply a large FIXED 8px horizontal projection shift so the
    // rasterization-level jitter is plainly visible.
    d->jitter_x = 8.f * 2.f / d->viewport_w;
    d->jitter_y = 0.f;
  } else if (shader_injection.dlaa_jitter_enabled < 0.5f) {
    d->jitter_x = 0.f;
    d->jitter_y = 0.f;
  } else {
    uint32_t f = d->frame_index;
    d->jitter_x = (Halton(f + 1u, 2u) - 0.5f) * 2.f / d->viewport_w;
    d->jitter_y = (Halton(f + 1u, 3u) - 0.5f) * 2.f / d->viewport_h;
  }
  if (shader_injection.dlaa_jitter_method > 0.5f) {
    // Global method: the shared ViewProjection patch is the jitter source; keep
    // the per-VS injection offsets at 0 so replaced VSs don't double-jitter.
    shader_injection.jitter_offset_x = 0.f;
    shader_injection.jitter_offset_y = 0.f;
  } else {
    shader_injection.jitter_offset_x = d->jitter_x;
    shader_injection.jitter_offset_y = d->jitter_y;
  }
}

// ── Projection jitter: two methods ──
// 1) PER VS (default): jitter is added directly in the replaced scene-geometry
//    VSs (boot/0xHASH.vs_4_1.hlsl): o0.x += DLAA_JITTER_X * o0.w after the
//    ViewProjection multiply. Requires each new scene's VS permutations to be
//    patched (see SCENE_GEOMETRY_VS_HASHES).
// 2) GLOBAL (Jitter Method = Global): the addon patches the SHARED
//    scene.ViewProjection (b0 _Globals, c10 = bytes 160..208) in place at bind
//    time (row0 += jx*row3, row1 += jy*row3). Every scene object jitters
//    regardless of VS hash — no per-scene shader gathering. The unjittered VP
//    is captured from the game's upload (OnUpdateBufferRegion) so velocity stays
//    jitter-free, and effect draws (excluded from DLAA) get the UNJITTERED VP.
// The old shared-PROXY approach is unworkable: replacing b0 with one proxy gives
// every object the first object's World matrix (b0 is per-object, World at c44);
// per-draw readbacks also killed performance. The global method only writes the
// VP region (bytes 160..208) into the game's own buffer — World is untouched.

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
  c[43] = shader_injection.dlaa_exclude_effects;  // params2.w — mask effects out of DLAA
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
      LogThrottled("vel-created", reshade::log::level::info, 1u, 100u,
                   "[DLAA] Velocity texture created: %ux%u fmt=%d", w, h, (int)vel_fmt);
  }

  if (!d->velocity_pipeline.handle && !CreateVelocityPipeline(dev, d)) {
    // Unconditional (not debug-gated): a crash log must always show why the
    // velocity dispatch didn't run.
    LogThrottled("vel-pipe-fail", reshade::log::level::warning, 3u, 60u,
                 "[DLAA] veloc: pipeline create FAILED");
    return false;
  }

  // Effect mask (r16g16_float) for the DLAA opt-out of particles/effects.
  EnsureEffectMask(dev, d, w, h);
  // Per-object motion target (32-bit float) for character MVs.
  EnsureMotionTarget(dev, d, w, h);

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

  // DLAA evaluate (only when all resources ready + NGX supported). The gate and
  // matrices status are logged UNCONDITIONALLY so a crash log always shows why
  // the velocity dispatch does or doesn't run.
  bool dlaa_ok = false;
  const bool dlss_res_ready = senkiseki3::dlss::ngx.supported
      && d->captured_color_res.handle && d->captured_rtv0_res.handle
      && d->captured_depth_res.handle;
  if (!dlss_res_ready) {
    LogThrottled("vel-gate", reshade::log::level::info, 3u, 60u,
                 "[DLAA] veloc: gate fail (ngx=%d color=%d rtv0=%d depth=%d)",
                 (int)senkiseki3::dlss::ngx.supported,
                 d->captured_color_res.handle ? 1 : 0,
                 d->captured_rtv0_res.handle ? 1 : 0,
                 d->captured_depth_res.handle ? 1 : 0);
  } else {
    // Read camera matrices from the game's _Globals CBV (depth-projection velocity)
    auto* cl = reinterpret_cast<ID3D11DeviceContext*>(cmd_list->get_native());
    ReadSceneMatrices(dev, d, cl);
    LogThrottled("vel-mv", reshade::log::level::info, 3u, 60u,
                 "[DLAA] veloc: matrices_valid=%d", (int)d->matrices_valid);

    if (d->matrices_valid) {
      // Pass: Velocity compute
      cmd_list->bind_pipeline(AC, d->velocity_pipeline);

      // Per-object motion source: our dedicated 16-bit target (Stage 1) when
      // available, else the legacy game MRT2 capture (8-bit, noisy).
      reshade::api::resource_view motion_src = d->motion_srv;
      if (shader_injection.dlaa_per_object_motion < 0.5f || !motion_src.handle)
        motion_src = d->captured_motion_srv;
      // ── Velocity dispatch: null-descriptor guard + bound-state diagnostics ──
      // The first dispatch here was TDRing the GPU despite all bindings looking
      // valid. Guards: skip on a null descriptor (never dispatch on a partial
      // set), and log the exact bound state + OM (RTV/DSV) state so a repeat
      // names the bad descriptor instead of guessing.
      reshade::api::descriptor_table_update u[6] = {
        { d->velocity_tables[0], 0, 0, 1, reshade::api::descriptor_type::sampler, &d->point_sampler },
        { d->velocity_tables[1], 0, 0, 1, reshade::api::descriptor_type::constant_buffer, &d->captured_scene_cbv },
        { d->velocity_tables[2], 0, 0, 1, reshade::api::descriptor_type::texture_shader_resource_view, &d->captured_depth_srv },
        { d->velocity_tables[3], 0, 0, 1, reshade::api::descriptor_type::texture_shader_resource_view, &motion_src },
        { d->velocity_tables[4], 0, 0, 1, reshade::api::descriptor_type::texture_unordered_access_view, &d->velocity_uav },
        { d->velocity_tables[5], 0, 0, 1, reshade::api::descriptor_type::texture_shader_resource_view, &d->effect_mask_srv },
      };
      if (!d->point_sampler.handle || !d->captured_scene_cbv.buffer.handle ||
          !d->captured_depth_srv.handle || !motion_src.handle ||
          !d->velocity_uav.handle || !d->effect_mask_srv.handle) {
        if (shader_injection.dlaa_debug_logging > 0.5f)
          LogThrottled("veloc-skip", reshade::log::level::warning, 3u, 60u,
                       "[DLAA] veloc: SKIP dispatch (null descriptor)");
        return false;
      }
      if (shader_injection.dlaa_debug_logging > 0.5f) {
        auto vdstr = [&](reshade::api::resource_view v) {
          if (!v.handle) return std::string("null");
          auto rd = dev->get_resource_desc(dev->get_resource_from_view(v));
          char b[80];
          snprintf(b, sizeof(b), "%ux%u fmt=%d", (int)rd.texture.width,
                   (int)rd.texture.height, (int)rd.texture.format);
          return std::string(b);
        };
        ID3D11RenderTargetView* om_rtvs[8] = {};
        ID3D11DepthStencilView* om_dsv = nullptr;
        UINT om_num = 0;
        cl->OMGetRenderTargets(8, om_rtvs, &om_dsv);
        ID3D11Resource* dsv_res = nullptr;
        if (om_dsv) om_dsv->GetResource(&dsv_res);
        ID3D11Buffer* cs_cb = nullptr;
        cl->CSGetConstantBuffers(13, 1, &cs_cb);
        D3D11_BUFFER_DESC cbd = {};
        if (cs_cb) cs_cb->GetDesc(&cbd);
        LogThrottled("veloc-dispatch", reshade::log::level::info, 3u, 60u,
                     "[DLAA] veloc: dispatch %ux%u depth=%s motion=%s mask=%s out=%s "
                     "cbv=0x%llX/%uB b13=%uB omRtv=%u omDsv=%s dsvIsDepth=%d",
                     w, h, vdstr(d->captured_depth_srv).c_str(), vdstr(motion_src).c_str(),
                     vdstr(d->effect_mask_srv).c_str(), vdstr(d->velocity_uav).c_str(),
                     (unsigned long long)d->captured_scene_cbv.buffer.handle,
                     (unsigned)(d->captured_scene_cbv.size), cbd.ByteWidth, om_num,
                     om_dsv ? "yes" : "no",
                     (dsv_res && dsv_res == reinterpret_cast<ID3D11Resource*>(d->captured_depth_res.handle)) ? 1 : 0);
        for (UINT i = 0; i < om_num; ++i)
          if (om_rtvs[i]) om_rtvs[i]->Release();
        if (om_dsv) om_dsv->Release();
        if (dsv_res) dsv_res->Release();
        if (cs_cb) cs_cb->Release();
      }

      dev->update_descriptor_tables(6, u);

      reshade::api::descriptor_table tables[6] = {
          d->velocity_tables[0], d->velocity_tables[1], d->velocity_tables[2],
          d->velocity_tables[3], d->velocity_tables[4], d->velocity_tables[5]};
      cmd_list->bind_descriptor_tables(CS, d->velocity_layout, 0, 6, tables);

      auto pc = BuildVelocityPC(d);
      cmd_list->push_constants(CS, d->velocity_layout, 6, 0, 48, pc.data());
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
            LogThrottled("mvdbg-srv-fail", reshade::log::level::warning, 1u, 120u,
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
          LogThrottled("ngx-srv-fail", reshade::log::level::warning, 1u, 120u,
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
        .key = "DLAAJitterMethod", .binding = &shader_injection.dlaa_jitter_method,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f, .label = "Jitter Method", .section = "Antialiasing",
        .tooltip = "Per VS: jitter is added inside each replaced scene-geometry vertex shader (each new scene's VS permutations must be patched). Global: the shared camera ViewProjection in _Globals is patched once, jittering every scene object regardless of VS hash (no per-scene shader gathering).",
        .labels = {"Per VS", "Global"},
        .is_enabled = []{ return shader_injection.dlaa_enabled > 0.5f; },
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
        .key = "DLAAExcludeEffects", .binding = &shader_injection.dlaa_exclude_effects,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f, .label = "Exclude Effects from DLAA", .section = "Antialiasing",
        .tooltip = "Masks particles/effects out of DLAA: they get an off-screen motion vector so DLSS falls back to the current frame (no temporal shimmer/ghosting).",
        .labels = {"Off","On"},
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
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f, .label = "Per-Object Motion", .section = "Antialiasing",
        .tooltip = "Camera Only: camera reprojection MVs only. Per-Object: patched char VSs emit prevClip (current-pose re-skin -> camera-relative MVs). Prev-Bone: patched char VSs re-skin with the PREVIOUS frame's per-character bone matrices -> true limb/animation MVs. Requires Phase B bind mode (NoBind/Minimal off) and a restart.",
        .labels = {"Camera Only","Per-Object","Prev-Bone"},
        .is_enabled = []{ return shader_injection.dlaa_enabled > 0.5f; },
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
    new renodx::utils::settings::Setting{
        .key = "DLAAPhase0Logging", .binding = &shader_injection.dlaa_phase0_logging,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f, .label = "Phase 0 Probe Logging", .section = "Antialiasing",
        .tooltip = "Prev-pose feasibility probe (bone-buffer SRV pushes at vertex t0, per-object VS skinned/world detection, buffer handle reuse, and vertex-buffer BindFlags to detect GPU-skinned UAV buffers for the Luma-style prev-vertex capture). Kept SEPARATE from Debug Logging so bone logs aren't buried in general spam.",
        .labels = {"Off","On"},
    },
    // Phase B generic-patch isolation (crash bisection).
    new renodx::utils::settings::Setting{
        .key = "DLAAPhaseB", .binding = &g_phaseb_enabled,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f, .label = "Phase B Generic VS Patch", .section = "Antialiasing",
        .tooltip = "Generic DXBC patcher: adds prev-bone re-skin + prevVP to every skinned VS at pipeline creation (per-object motion MVs without per-shader HLSL).",
        .labels = {"Off","On"},
    },
    new renodx::utils::settings::Setting{
        .key = "DLAAPhaseBOutline", .binding = &g_phaseb_outline,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f, .label = "Phase B Outline Offset", .section = "Antialiasing",
        .tooltip = "Replicate the GameEdgeParameters outline offset in the patched prevClip. Disable to isolate crashes to the outline block.",
        .labels = {"Off","On"},
    },
    new renodx::utils::settings::Setting{
        .key = "DLAAPhaseBNoBind", .binding = &g_phaseb_no_bind,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f, .label = "Phase B No-Bind (Game t0/b0 Only)", .section = "Antialiasing",
        .tooltip = "CRASH BISECTION: patch the VS to read ONLY the game's own t0 bones and b0 ViewProjection, so the patched draw needs NO addon t#/b# bindings (no leak, no extra SRV/CB read). prevClip == current clip (motion ~0) — correct smoke-test baseline. Requires restart to take effect (D3D11 pipeline cache).",
        .labels = {"Off","On"},
    },
    new renodx::utils::settings::Setting{
        .key = "DLAAPhaseBMinimal", .binding = &g_phaseb_minimal,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f, .label = "Phase B Minimal (No Reads)", .section = "Antialiasing",
        .tooltip = "CRASH BISECTION (purest): the injected block reads NO resources at all — prevClip = the game's own b0 ViewProjection x float4(v0,1), no re-skin, no outline. Isolates the output register / OSGN / dcl_temps machinery from every new resource read. Requires restart (D3D11 pipeline cache).",
        .labels = {"Off","On"},
    },
    new renodx::utils::settings::Setting{
        .key = "DLAAPhaseBNoOutput", .binding = &g_phaseb_no_output,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f, .label = "Phase B No Output Register", .section = "Antialiasing",
        .tooltip = "CRASH BISECTION: the injected block runs (reads v0 + cb0[10..13]) but adds NO prevClip output register — the PS's v7 goes undefined again. If the crash STOPS, the extra output register / OSGN append is the cause. Requires restart.",
        .labels = {"Off","On"},
    },
    new renodx::utils::settings::Setting{
        .key = "DLAAPhaseBConstant", .binding = &g_phaseb_constant,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f, .label = "Phase B Constant Output", .section = "Antialiasing",
        .tooltip = "CRASH BISECTION: minimal, but the new output register is written with a CONSTANT (0,0,0,1) — zero reads at all. If it still crashes, the mere existence of the output register faults; if it doesn't, the cb0 read / v7 data matters. Requires restart.",
        .labels = {"Off","On"},
    },
    new renodx::utils::settings::Setting{
        .key = "DLAAPhaseBDump", .binding = &g_phaseb_dump,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f, .label = "Phase B Evidence Dump", .section = "Antialiasing",
        .tooltip = "Write every patched VS's original + patched bytecode (0x<hash>.cso / 0x<new>.patched.cso) and a per-draw crash trace (drawtrace.log) to renodx-dev/dump/phaseb/. The LAST drawtrace line before a GPU TDR identifies the crashing draw.",
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
    // NOTE: ALL SKINNED char VSs (face 0x0D5DABC6, hair, eyes, skin, clothing,
    // outline, etc.) are handled by the GENERIC DXBC patcher (OnCreatePipeline)
    // and must NOT be listed here: the patcher's skip-guard
    // (`if (custom_shaders.contains(hash)) continue;`) would bypass them and
    // leave their paired hand-patched PSs reading an undefined v7/prevClip
    // (TEXCOORD5) -> garbage motion lines on those meshes. The boot-HLSL
    // replacements in this list only add geometry jitter and do NOT emit
    // prevClip. Unskinned scene-geometry VSs below stay custom-replaced (the
    // generic patcher only touches skinned VSs — those have no
    // BLENDINDICES/BLENDWEIGHTS).
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
    CustomShaderEntry(0x77355EED),  // forest impostor billboard (sky)
    CustomShaderEntry(0xC8FE8FC4),  // transparent texture
    CustomShaderEntry(0x7D3553A7),  // particle
    // Character G-buffer PS (paired with the main skinned char VS 0x0D5DABC6):
    // writes per-object prevNDC into o3 (SV_TARGET3) of the appended 16-bit
    // motion RTV — o0/o1/o2 (game MRTs) are untouched. The old 0x0E8BC215
    // overwrote MRT2 (o2 = encoded depth) which corrupted other objects.
    CustomShaderEntry(0xFEA2B509),
    // Other character-part G-buffer PSs — same o3 per-object-motion patch.
    CustomShaderEntry(0x159A34A3),  // pairs with 0xB662509A
    CustomShaderEntry(0x1682CB9B),  // pairs with 0xF8C9B92D
    CustomShaderEntry(0x41C27C38),  // pairs with 0xC8F5D77B
    CustomShaderEntry(0xBE1FC4AC),  // pairs with 0x3641D444
    CustomShaderEntry(0x59F0F50E),  // pairs with 0x4A030C25
    CustomShaderEntry(0xA4DC2F84),  // pairs with 0x5C1A50E5
    CustomShaderEntry(0x7542CBC4),  // pairs with 0xB1C24E2A
    CustomShaderEntry(0x0E03514A),  // pairs with 0xB2F338C8
    CustomShaderEntry(0x1DE48D94),  // pairs with 0x5E5AE3FB (outline)
    CustomShaderEntry(0x2029B1A4),  // pairs with 0xF426BC1C
    CustomShaderEntry(0x87986FAA),  // pairs with 0xBCB30859
    CustomShaderEntry(0x2B3C9980),  // pairs with 0x0045297D
    CustomShaderEntry(0x08F6C8F5),  // pairs with 0x59001D8E
    CustomShaderEntry(0x38BA428E),  // pairs with 0x63C867BA
    CustomShaderEntry(0xE14A638B),  // pairs with 0xB0A80DEF (outline)
    CustomShaderEntry(0x43C1531A),  // pairs with 0x1DF2E2BB
    CustomShaderEntry(0xF280500B),  // pairs with 0x38BCCCA0
    CustomShaderEntry(0xC04F07D9),  // pairs with 0x6285DCF3
    CustomShaderEntry(0xEA7B3E27),  // pairs with 0xFC588329
    CustomShaderEntry(0xDD5A9D03),  // pairs with 0x5AA04209
    CustomShaderEntry(0x055CFB63),  // pairs with 0x835760A3 (outline)
    // Effect PS replacements: write SV_TARGET1 = 1.0 into the appended
    // effect-mask RT (Exclude Effects toggle) so the velocity compute can
    // opt these pixels out of DLAA (invalid-MV -> current-frame fallback).
    CustomShaderEntry(0xD589DF82),  // particle PS (mask writer)
    CustomShaderEntry(0x720FE34C),  // water/particle PS (mask writer)
    CustomShaderEntry(0xC1BF7F2E),  // effect PS (mask writer)
    // (0x728F5ED1 transparent PS is NOT registered: it must stay jittered +
    //  DLAA'd — static world content with correct camera MVs. Excluding it made
    //  the background behind it shake.)
    // Pre-FXAA composite: pure pixel-center passthrough (no UV-shift jitter).
    // The geometry jitter passes through unchanged to the buffer RunDLAA feeds
    // to DLSS.
    CustomShaderEntry(0xE8C7EBA2),
};

// Resolve the evidence folder once: <ReShadeBase>/renodx-dev/dump/phaseb/ — the
// SAME absolute base the DevKit uses (renodx::utils::path::GetOutputPath). A
// relative path would resolve against the process CWD, which Steam sets to the
// game ROOT (F:\...\Cold Steel III\) not bin/x64 — that's why the earlier
// dumps never appeared where expected even though patching was happening.
static const std::string& GetPhasebDir() {
  static const std::string dir = [] {
    std::error_code ec;
    auto p = renodx::utils::path::GetOutputSubdirectory("dump") / "phaseb";
    std::filesystem::create_directories(p, ec);  // fopen does NOT create dirs
    return p.string();
  }();
  return dir;
}

// Write a raw shader blob to <ReShadeBase>/renodx-dev/dump/phaseb/ (for
// offline inspection when the GPU TDRs during load before the DevKit flushes).
// Gated behind the DLAAPhaseBDump setting.
static void WritePhasebDump(uint32_t hash, const void* code, size_t size, const char* suffix) {
  if (g_phaseb_dump < 0.5f) return;
  if (!code || size == 0u) return;
  char path[1024];
  snprintf(path, sizeof(path), "%s/0x%08X%s.cso", GetPhasebDir().c_str(), hash, suffix);
  FILE* f = nullptr;
  fopen_s(&f, path, "wb");
  if (f) { fwrite(code, 1, size, f); fclose(f); }
}

// (All-draw crash-window tracing is implemented in MaybeTraceCrashWindow near
// the draw hooks; per-draw lines are appended there.)

// ── Generic DXBC patcher hook ──
// Runs on create_pipeline. For D3D11 the event fires once per stage
// (CreateVertexShader / CreatePixelShader / CreateInputLayout), each with a
// single shader subobject. We register BEFORE mods::shader::Use so we always
// see the ORIGINAL bytecode: the skip-guard ("hash in custom_shaders") only
// works on originals — after renodx swaps in a hand-written replacement the
// hash no longer matches its key. renodx runs after us, so its own
// replacements still fire for the shaders we skipped.
static bool OnCreatePipeline(
    reshade::api::device* device,
    reshade::api::pipeline_layout layout,
    uint32_t subobject_count,
    const reshade::api::pipeline_subobject* subobjects) {
  if (!device) return false;
  auto* d = device->get_private_data<DeviceData>();
  if (!d) return false;
  // Master toggle. Note: D3D11 caches pipelines, so toggling this ON later
  // won't re-create already-created VSs (a restart is needed after a change).
  if (g_phaseb_enabled < 0.5f) return false;
  // Always patch (not gated by the per-object-motion toggle): D3D11 caches
  // pipelines, so a toggle-on later would never re-create an unpatched VS. An
  // unbound prevVP/prev-bone slot only affects the unused extra output.
  bool changed = false;
  for (uint32_t i = 0; i < subobject_count; ++i) {
    const auto& sub = subobjects[i];
    if (sub.type != reshade::api::pipeline_subobject_type::vertex_shader) continue;
    if (sub.count != 1u) continue;
    auto* desc = static_cast<reshade::api::shader_desc*>(sub.data);
    if (!desc || desc->code_size == 0u) continue;
    const uint32_t hash = renodx::utils::hash::ComputeCRC32(
        static_cast<const uint8_t*>(desc->code), desc->code_size);
    // Skip already-patched (re-fire at CreateInputLayout with our blob) and
    // hand-replaced shaders (renodx handles those with its own blobs).
    if (d->patched_vs_hashes.contains(hash)) continue;
    if (custom_shaders.contains(hash)) continue;

    std::vector<std::byte> blob;
    blob.resize(desc->code_size);
    std::memcpy(blob.data(), desc->code, desc->code_size);
    senkiseki3::dxbc::PatchInfo info;
    uint32_t new_hash = 0u;
    senkiseki3::dxbc::PatchOptions options;
    options.enable_outline = g_phaseb_outline >= 0.5f;
    options.use_game_resources_only = g_phaseb_no_bind >= 0.5f;
    options.minimal_patch = g_phaseb_minimal >= 0.5f;
    options.test_no_output = g_phaseb_no_output >= 0.5f;
    options.test_constant_output = g_phaseb_constant >= 0.5f;
    if (!senkiseki3::dxbc::PatchSkinnedVertexShader(blob, &new_hash, &info, options)) continue;
    if (new_hash == 0u || new_hash == hash) continue;

    // Evidence capture (DLAAPhaseBDump on): original + patched blobs, written
    // BEFORE the driver sees them so a TDR can't lose the evidence.
    WritePhasebDump(hash, desc->code, desc->code_size, "");
    WritePhasebDump(new_hash, blob.data(), blob.size(), ".patched");

    desc->code = malloc(blob.size());
    if (!desc->code) continue;
    std::memcpy(const_cast<void*>(desc->code), blob.data(), blob.size());
    desc->code_size = blob.size();

    DeviceData::PatchedVsInfo pvi;
    pvi.new_hash = new_hash;
    pvi.prev_vp_cb_slot = info.prev_vp_cb_slot;
    pvi.prev_bone_t_slot = info.prev_bone_t_slot;
    pvi.texcoord_index = info.texcoord_index;
    pvi.output_reg = info.output_reg;
    pvi.needs_no_binding = info.needs_no_binding;
    pvi.bone_game_slot = info.bone_game_slot;
    d->patched_vs_by_new_hash[new_hash] = pvi;
    d->patched_vs_hashes.insert(hash);     // original
    d->patched_vs_hashes.insert(new_hash); // patched
    changed = true;

    char buf[224];
    snprintf(buf, sizeof(buf),
             "[DLAA] phaseB: patched VS 0x%08X -> 0x%08X (%u B) cb%u t%u TEXCOORD%u o%u outline=%d noBind=%d minimal=%d noOutput=%d const=%d",
             hash, new_hash, (uint32_t)blob.size(), info.prev_vp_cb_slot,
             info.prev_bone_t_slot, info.texcoord_index, info.output_reg,
             (int)info.outline_applied, (int)info.needs_no_binding,
             (int)options.minimal_patch, (int)options.test_no_output,
             (int)options.test_constant_output);
    reshade::log::message(reshade::log::level::info, buf);
  }
  return changed;
}

}  // namespace

extern "C" __declspec(dllexport) constexpr const char* NAME = "Senkiseki DLAA";
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
      reshade::register_event<reshade::addon_event::update_buffer_region>(OnUpdateBufferRegion);
      reshade::register_event<reshade::addon_event::map_buffer_region>(OnMapBufferRegionProbe);
      reshade::register_event<reshade::addon_event::draw>(OnDrawMaskHook);
      reshade::register_event<reshade::addon_event::draw_indexed>(OnDrawMaskHookIndexed);
      // Phase B: generic skinned-VS DXBC patcher. Registered BEFORE
      // renodx::mods::shader::Use below so we see the ORIGINAL bytecode (the
      // skip-guard relies on it — after renodx swaps in a replacement the hash
      // no longer matches custom_shaders' keys).
      reshade::register_event<reshade::addon_event::create_pipeline>(OnCreatePipeline);

      reshade::register_event<reshade::addon_event::present>(
          [](reshade::api::command_queue* queue, reshade::api::swapchain* swapchain,
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
          // Effect mask is cleared at the first effect pass each frame.
          d->effect_mask_cleared_this_frame = false;
          // Per-object motion target cleared at the first char draw each frame.
          d->motion_cleared_this_frame = false;
          // Jitter is computed once per frame at PRESENT time, before the next frame's
          // composite (0xE8C7EBA2) draws. Both the composite UV shift and the NGX jitter
          // offsets then read the SAME stored value -> rendered jitter == reported jitter.
          // (Calling it in RunDLAA caused a 1-frame mismatch: the composite used J_{N-1}
          // while NGX got J_N, so DLSS could never align its temporal history.)
          UpdateJitter(d);
          // Phase 0 prev-pose probe: once-per-frame summary (separate toggle).
          Phase0ProbeFrameSummary(d);
          // Phase B: refresh the patched VSs' prevVP cbuffer with the UNJITTERED
          // previous-frame ViewProjection. ReadSceneMatrices set
          // shader_injection.prev_view_proj to the CURRENT frame's VP during
          // this frame's scene-CBV capture; the patched VSs read it on the NEXT
          // frame as prevViewProj — exactly matching the hand-patched shaders.
          if (shader_injection.dlaa_per_object_motion >= 0.5f &&
              EnsurePrevVpCb(dev, d) && d->matrices_valid) {
            auto* pcmd = queue->get_immediate_command_list();
            if (pcmd) {
              auto* pctx = reinterpret_cast<ID3D11DeviceContext*>(pcmd->get_native());
              D3D11_MAPPED_SUBRESOURCE m = {};
              if (pctx && SUCCEEDED(pctx->Map(d->prev_vp_cb, 0, D3D11_MAP_WRITE_DISCARD, 0, &m))) {
                std::memcpy(m.pData, shader_injection.prev_view_proj, 64);
                pctx->Unmap(d->prev_vp_cb, 0);
              }
            }
          }
          // Phase D: copy each registered game bone buffer into its prev twin so
          // the next frame's patched VS re-skins with the PREVIOUS frame's bones.
          // Gated by DLAAPerObjectMotion = Prev-Bone (handled inside CapturePrevBones).
          {
            auto* pcmd = queue->get_immediate_command_list();
            if (pcmd) CapturePrevBones(pcmd, d);
          }
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
        // Per-frame global-jitter summary: _Globals uploads patched vs large
        // buffers mapped this frame — tells us mid-game whether the scene
        // _Globals is uploaded via UpdateSubresource or Map/Unmap.
        if (d && shader_injection.dlaa_debug_logging > 0.5f && fc % 90 == 0) {
          char gbuf[128];
          snprintf(gbuf, sizeof(gbuf), "[DLAA] global: frame %d patched=%u mapped=%u",
                   fc, d->globals_patch_count, d->globals_map_count);
          reshade::log::message(reshade::log::level::info, gbuf);
        }
        if (d) { d->globals_patch_count = 0; d->globals_map_count = 0; }
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
      reshade::unregister_event<reshade::addon_event::draw>(OnDrawMaskHook);
      reshade::unregister_event<reshade::addon_event::draw_indexed>(OnDrawMaskHookIndexed);
      reshade::unregister_event<reshade::addon_event::map_buffer_region>(OnMapBufferRegionProbe);
      reshade::unregister_event<reshade::addon_event::update_buffer_region>(OnUpdateBufferRegion);
      reshade::unregister_event<reshade::addon_event::create_pipeline>(OnCreatePipeline);
      reshade::unregister_addon(h_module);
      break;
  }
  renodx::utils::settings::Use(fdw_reason, &settings);
  renodx::mods::shader::Use(fdw_reason, custom_shaders, &shader_injection);
  // The tail below runs for EVERY fdw_reason (DLL_THREAD_ATTACH included), so
  // the banner must be gated to process attach — otherwise it prints once per
  // spawned thread and bloats the log (observed: dozens/hundreds of copies
  // from many thread IDs at startup).
  if (fdw_reason == DLL_PROCESS_ATTACH) {
    // Log the Phase B evidence-capture state + resolved dump path so the next
    // run's log CONFIRMS whether DLAAPhaseBDump is actually on and where the
    // files go (the path bug hid this before).
    char ibuf[640];
    snprintf(ibuf, sizeof(ibuf), "[DLAA] phaseB dump=%s enabled=%s outline=%s path=%s",
             g_phaseb_dump >= 0.5f ? "ON" : "OFF",
             g_phaseb_enabled >= 0.5f ? "ON" : "OFF",
             g_phaseb_outline >= 0.5f ? "ON" : "OFF",
             GetPhasebDir().c_str());
    reshade::log::message(reshade::log::level::info, ibuf);
  }
  return TRUE;
}
