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
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
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
    .dlaa_hdr_decode = 0.f,
    .dlaa_hdr_inject = 0.f,
    .dlaa_hdr_float_out = 0.f,
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
static float g_phaseb_jitter_only = 1.f;  // camera path (default ON): generic VS patch ONLY
                                          // adds the SV_Position jitter — no prevClip output,
                                          // no prev-bone re-skin, no prevVP, no outline, no
                                          // OSGN append. Under GLOBAL jitter it's inert (b13=0).
// ── Phase E crash-bisection toggles (mirror Phase B's; isolate what the
// NVIDIA driver chokes on in the patched PS) ──
static float g_phasee_constant = 0.f;  // add ONLY the o3 output, constant write — no input
                                       // decl, no SV_SampleIndex relocation, no vN read
static float g_phasee_no_read = 0.f;   // input decl + SV relocation, but body never reads vN
static float g_phasee_no_math = 0.f;   // input decl + SV relocation, mov o3,vN (no div/mad)
static float g_phasee_no_new_output = 0.f;  // full input+math, but prevNDC -> EXISTING
                                            // o{max}.zw, NO new output / OSGN append
// ── Shake-isolation diagnostics (addon-side floats, NOT in ShaderInjectData) ──
static float g_phase_freeze_jitter = 0.f;  // DIAGNOSTIC: fix jitter at a CONSTANT value every frame (Halton disabled).
                                          // If the Prev-Bone shake disappears with a fixed jitter, the error is the
                                          // frame-to-frame jitter variation (compensation/timing mismatch), not the MVs.
static float g_phase_freeze_report = 0.f;  // DIAGNOSTIC (DLAAPhaseFreezeReport): render the normal (Halton) jitter
                                          // but report a CONSTANT jitter to DLSS. If the shake disappears, DLSS was
                                          // reacting to the VARYING report (fix the offset); if it still shakes, DLSS
                                          // ignores the report and the RENDERED variation is the culprit.
static float g_phase_freeze_render = 0.f;  // DIAGNOSTIC (DLAAPhaseFreezeRender): mirror — render a CONSTANT jitter
                                          // but report the normal (Halton) jitter. If it shakes, DLSS consumes the
                                          // report; if it stays stable, DLSS ignores it.
static float g_phase_mv_comp = 1.f;        // DIAGNOSTIC: per-object jitter subtraction in the velocity shader A/B.
                                          // 1 = subtract the frame's jitter from per-object MVs (current behavior).
                                          // 0 = don't compensate. If the character STOPS shaking with comp OFF, the
                                          // character was never jittered (we were over-subtracting); if it still shakes,
                                          // the character IS jittered and compensation isn't the cause.
static float g_phase_mv_jittered = 0.f;   // DIAGNOSTIC (Test C): feed JITTERED MVs + MVJittered=1 so DLSS removes the
                                          // jitter internally. The velocity shader then ADDS the jitter to the camera path
                                          // (it is jitter-free by construction) to keep the whole buffer consistently jittered.
static float g_phase_jitter_depth = 0.f;  // DIAGNOSTIC (DLAAPhaseJitterDepth): jitter the depth pass too,
                                          // so DLSS sees color AND depth in the same jitter state (Test B).
                                          // Default OFF = depth stays the game's native UNJITTERED depth
                                          // (Test A). Only the character SHADOW VS stays unjittered (its
                                          // consumer compares against the unjittered projection).
static float g_phase_jitter_color = 1.f;  // DIAGNOSTIC (DLAAPhaseJitterColor): master color-jitter toggle.
                                          // Default ON = color rendered with the jittered VP (normal DLAA).
                                          // OFF = color renders UNJITTERED and 0 jitter is reported to DLSS
                                          // (baseline: unjittered color + unjittered depth + report 0).
static float g_phase_mv_threshold_object = 0.f;  // per-object / Prev-Bone MV threshold (px). DLAAMVThreshold now gates
                                                 // ONLY camera (depth-reprojection) MVs; this gates the per-object path.
                                                 // Addon-side (NOT in ShaderInjectData: growing that struct resizes the
                                                 // injected b13 cbuffer and breaks the boot PSs). Consumed only by
                                                 // BuildVelocityPC -> velocity compute params3.x.
static float g_jitter_scale = 1.f;  // DIAGNOSTIC (DLAAJitterScale): 0=0.5x, 1=1x exact, 2=2x — scales the NGX jitter
                                    // offset magnitude reported to DLSS WITHOUT changing the rendered jitter. If the
                                    // 30 FPS static jitter converges at 0.5x or 2x, the reported magnitude doesn't
                                    // match the applied one. Addon-side only (not in ShaderInjectData).
static float g_jitter_in_mv = 0.f;  // DIAGNOSTIC (DLAAPhaseJitterInMV): bake the jitter DELTA (current - previous
                                    // frame) into the MVs. This DLSS runtime IGNORES the NGX jitter offset (report 0
                                    // vs real = identical), so the per-frame jitter reads as unresolvable sub-pixel
                                    // motion -> shimmer, worst at low FPS. DLSS uses the MVs directly, so adding the
                                    // delta makes its reprojection align the jittered history. Pairs with Test A.
static float g_jitter_decouple = 0.f;  // DIAGNOSTIC (DLAAPhaseReportOnly): render UNJITTERED but report the Halton
                                       // jitter to DLSS. The DEFINITIVE test of whether DLSS uses the offset: if the
                                       // output changes when this is on (DLSS told jitter that isn't rendered), DLSS
                                       // consumes the report; if it looks identical to jitter-off, DLSS ignores it.
static float g_mv_scale_mode = 0.f;  // DIAGNOSTIC (DLAAMVScale): A/B the DLSS InMVScale parameter (converts the
                                     // velocity texture to pixel space) WITHOUT touching the MV shader. 0 = 1.0
                                     // (MVs already pixel-space, SDK default), 1 = 1/W,1/H (1.0 = full screen),
                                     // 2 = 2/W,2/H (NDC, 1.0 = half screen).
static float g_phase_depth_sample_unjit = 0.f;  // DIAGNOSTIC (DLAAPhaseDepthSampleUnjit): sample the depth at the
                                                // UNJITTERED pixel (curPxU) instead of the jittered pixel (pix) in the
                                                // velocity camera path. With an UNJITTERED depth buffer (depth_only),
                                                // depth[pix] belongs to content at unjittered position pix, NOT at
                                                // curPxU = pix - jitterPx — so sampling at pix misreads the depth by
                                                // up to half a pixel whenever jitter is on, injecting a per-frame MV
                                                // error that DLSS can't compensate (shimmer, worst at 30 FPS).
static float g_phase_synth_jitter_mv = 0.f;  // DIAGNOSTIC (DLAAPhaseSynthMV): synthetic MV test. Bypasses depth
                                             // reprojection and writes an ANALYTIC global MV so we can establish the
                                             // correct jitter-delta MV + sign in the reprojection debug view (no DLSS,
                                             // no depth, no matrices). 0=normal, 1=zero everywhere, 2=Jcur-Jprev,
                                             // 3=Jprev-Jcur. At static+jitter, whichever sign makes the reproj view
                                             // black is the sign DLSS needs (fed via JitterInMV + report 0).

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
  // Phase E: same skip-guard for the generic per-object-motion PS patcher
  // (PatchPerObjectPixelShader). The patched PS needs NO draw-time binding — it
  // reads prevClip (TEXCOORD5) via normal VS->PS semantic linkage and writes
  // the appended o3/SV_TARGET3.
  std::unordered_set<uint32_t> patched_ps_hashes;
  // Dynamic 64-byte native cbuffer holding prev_view_proj (the unjittered
  // previous-frame ViewProjection), bound to patched VSs' prevVP slot at draw.
  ID3D11Buffer* prev_vp_cb = nullptr;
  // Addon-owned stride-64 StructuredBuffer (identity placeholder) bound to
  // patched VSs' prev-bone t# slot at draw. We bind OUR buffer — never the
  // game's slot-0 VS SRV (for some passes slot 0 is a texture, and feeding a
  // texture to ld_structured t#,64 is a descriptor-kind mismatch that TDRs).
  ID3D11Buffer* prev_bones_buffer = nullptr;
  ID3D11ShaderResourceView* prev_bones_srv = nullptr;

  // ── Phase D: per-instance prev-bone snapshots (real per-object MVs) ──
  // The game uploads a per-character bone StructuredBuffer (stride 64) at VS t0
  // every frame. For each CHARACTER INSTANCE we keep an addon-owned cur/prev
  // pair. At DRAW TIME (when the game has JUST bound this frame's bone buffer
  // at its t0 slot) we CopyResource(game buffer -> cur), and the patched VS
  // binds PREV (cur promoted to prev at present), so it re-skins with the
  // PREVIOUS frame's bones -> prevClip reflects real animation/limb motion.
  //
  // Keyed by CHARACTER INSTANCE (per-object b0 cbuffer handle + VB slot0
  // handle), NOT by VS hash — a single VS is SHARED across many characters
  // (eyeballs, skin, clothing), so a shader key would mix their bones. And NOT
  // by the bone-buffer handle: the game rotates a RING of bone buffers (frame N
  // writes buffer[N%4]), so a handle-keyed twin is 3-4 frames stale. Capturing
  // at draw time keyed by the stable instance is ring-agnostic.
  struct PrevBoneSnap {
    ID3D11Buffer* cur_buffer = nullptr;    // addon-owned structured buffer (this frame's bones)
    ID3D11ShaderResourceView* cur_srv = nullptr;
    ID3D11Buffer* prev_buffer = nullptr;   // addon-owned structured buffer (prev frame's bones)
    ID3D11ShaderResourceView* prev_srv = nullptr;
    uint32_t size = 0u;                    // byte size (== game bone buffer)
    uint64_t last_draw_frame = 0u;         // frame of the last draw-time capture
  };
  std::unordered_map<uint64_t, PrevBoneSnap> prev_bone_snaps;  // key = instance composite
  ID3D11ShaderResourceView* last_bound_twin_srv = nullptr;  // prev snap bound this draw (for restore)

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
  // ── Frame-pairing diagnostic ──
  // The velocity reprojection must pair depth_N with VP_N (and reproject through
  // prevVP = VP_{N-1}). If the captured depth or VP belongs to a different frame
  // (N-1/N-2), the error scales with the per-frame displacement -> invisible at
  // 170 FPS (tiny motion) but glaring at 30 FPS (large motion). Stamps record
  // which frame_index each input was captured on + the resource identity, so we
  // can prove/disprove the N / N-1 pairing directly.
  uint32_t depth_capture_frame = 0u;      // frame_index when captured_depth_res was last set
  uint64_t depth_capture_res_handle = 0u;  // resource handle at depth capture (ping-pong detection)
  uint32_t vp_read_frame = 0u;            // frame_index when ReadSceneMatrices captured curr VP
  uint64_t vp_cbv_handle = 0u;            // _Globals buffer handle the VP was read from
  uint32_t prev_vp_frame = 0u;            // frame_index the current prev_view_proj was captured on
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
  float jitter_x = 0.f, jitter_y = 0.f;             // RENDER jitter (what the source patch applies)
  float report_jitter_x = 0.f, report_jitter_y = 0.f;  // REPORT jitter (what NGX is told; = render unless decoupled)
  float prev_jitter_x = 0.f, prev_jitter_y = 0.f;  // previous frame's jitter (for MV jitter-delta baking)
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
  uint32_t evals_this_frame = 0u;  // EvaluateDLSS calls this frame (diag: must be 1 per presented frame)
  std::chrono::steady_clock::time_point last_present_time{};  // for DLSS InFrameTimeDeltaInMsec
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

  // ── Hang watchdog (scene-3 loading-screen freeze) ──
  // A background thread writes a heartbeat to watchdog.log every 250 ms while
  // the render thread keeps `watchdog_progress` stamped at every interesting
  // spot (pipeline patch start/end, draw hook enter/done, present). When the
  // game freezes, the render thread stops advancing the string, so the LAST
  // repeated heartbeat names the exact spot — splitting the two failure modes
  // we keep going back and forth on:
  //   * stuck after "create_pipeline phaseE patched PS 0x..->0x.."
  //       => blocked inside the driver's CreatePixelShader => driver COMPILE
  //          hang on that patched shader pair (no CPU assertion can catch it)
  //   * stuck after "draw done vs=0x.. ps=0x.."
  //       => blocked inside the driver's Draw/DrawIndexed => GPU EXECUTION
  //          hang on that patched draw
  std::mutex watchdog_mutex;
  char watchdog_progress[512] = "init";  // FIXED buffer: zero heap allocation inside the
                                         // lock scope, so a hung/corrupt heap can never
                                         // deadlock the watchdog against the render thread
  std::atomic<bool> watchdog_running{false};
  std::atomic<uint64_t> watchdog_ping{0};  // bumped every heartbeat; proves the watchdog
                                           // thread is alive even when the progress stalls
  std::thread watchdog_thread;

  reshade::api::sampler point_sampler = {};

  // Previous-frame color scratch (reprojection debug mode)
  reshade::api::resource prev_color_texture = {};
  reshade::api::resource_view prev_color_srv = {};

  // Native MV-debug resources (bypass ReShade pipeline creation; display via NGX output)
  ID3D11ComputeShader* dbg_cs = nullptr;
  ID3D11UnorderedAccessView* dbg_out_uav = nullptr;
  DXGI_FORMAT dbg_out_uav_fmt = DXGI_FORMAT_UNKNOWN;  // ngx format the UAV was created for
  ID3D11ShaderResourceView* dbg_vel_srv = nullptr;
  ID3D11ShaderResourceView* dbg_color_srv = nullptr;
  ID3D11ShaderResourceView* dbg_prev_srv = nullptr;
  ID3D11SamplerState* dbg_linear_sampler = nullptr;
  ID3D11Buffer* dbg_cb = nullptr;

  // ── HDR-mod compatibility (Phase 2): DLSS input/output color conversion ──
  // Decode the sRGB/PQ composite to linear before DLSS (DLSS always expects
  // linear input), then re-encode the DLSS output to the encoding the HDR chain
  // expects (0x9DB02646 SignPow + swapchain proxy sRGB-decode -> PQ).
  ID3D11ComputeShader* hdr_conv_cs = nullptr;
  ID3D11Buffer* hdr_conv_cb = nullptr;
  ID3D11Texture2D* linear_scratch = nullptr;     // r16g16b16a16_float (DLSS input)
  ID3D11ShaderResourceView* linear_scratch_srv = nullptr;
  ID3D11UnorderedAccessView* linear_scratch_uav = nullptr;
  ID3D11Texture2D* encoded_scratch = nullptr;    // r8g8b8a8_unorm (re-encoded DLSS output)
  ID3D11ShaderResourceView* encoded_scratch_srv = nullptr;
  ID3D11UnorderedAccessView* encoded_scratch_uav = nullptr;
  ID3D11Texture2D* ngx_dump_staging = nullptr;   // CPU-readable copy of NGX output (luma diag)
  uint32_t hdr_scratch_w = 0u, hdr_scratch_h = 0u;
  bool hdr_detected = false;                     // _renodx-senkiseki.addon64 loaded (Phase 3 auto-default)
  bool dlaa_ran_this_frame = false;              // DLSS bound t0 at a final_blending draw this frame

  // ── Robust DLSS color source (Phase 3 fix) ──
  // The game's PS t0 at the FXAA draw IS the composite. Read it live so the
  // DLSS input is immune to the event capture's failure modes under the HDR mod
  // (hash-0 "Pipeline not found" skip, or our own NGX-output t0 bind re-firing
  // the capture hook). Native AddRef'd pointers.
  ID3D11ShaderResourceView* live_color_srv = nullptr;
  ID3D11Resource* live_color_res = nullptr;
  uint32_t color_capture_frame = 0u;             // last successful event color capture
  ID3D11Texture2D* color_dump_staging = nullptr; // CPU readback for the color-source diag
};

// ── Hang watchdog (full definitions live below WritePhasebDump; forward
// declarations so the draw hooks + pipeline patcher + present can stamp the
// current spot).
static void WatchdogSet(DeviceData* d, const char* fmt, ...);
static void WatchdogStampDraw(reshade::api::command_list* cmd_list, DeviceData* d, const char* tag);
static void WatchdogStart(DeviceData* d);
static void WatchdogStop(DeviceData* d);

// ── Halton jitter ──
static float Halton(uint32_t n, uint32_t base) {
  float r = 0.f, inv = 1.f / (float)base, f = 1.f;
  while (n) { f *= inv; r += f * (float)(n % base); n /= base; }
  return r;
}

// ── Destroy ──
static void Destroy(reshade::api::device* dev, DeviceData* d) {
  if (!dev || !d) return;
  WatchdogStop(d);  // join the heartbeat thread before tearing down DeviceData
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
  if (d->hdr_conv_cs) { d->hdr_conv_cs->Release(); d->hdr_conv_cs = nullptr; }
  if (d->hdr_conv_cb) { d->hdr_conv_cb->Release(); d->hdr_conv_cb = nullptr; }
  if (d->linear_scratch_uav) { d->linear_scratch_uav->Release(); d->linear_scratch_uav = nullptr; }
  if (d->linear_scratch_srv) { d->linear_scratch_srv->Release(); d->linear_scratch_srv = nullptr; }
  if (d->linear_scratch) { d->linear_scratch->Release(); d->linear_scratch = nullptr; }
  if (d->encoded_scratch_uav) { d->encoded_scratch_uav->Release(); d->encoded_scratch_uav = nullptr; }
  if (d->encoded_scratch_srv) { d->encoded_scratch_srv->Release(); d->encoded_scratch_srv = nullptr; }
  if (d->encoded_scratch) { d->encoded_scratch->Release(); d->encoded_scratch = nullptr; }
  if (d->ngx_dump_staging) { d->ngx_dump_staging->Release(); d->ngx_dump_staging = nullptr; }
  if (d->color_dump_staging) { d->color_dump_staging->Release(); d->color_dump_staging = nullptr; }
  if (d->live_color_srv) { d->live_color_srv->Release(); d->live_color_srv = nullptr; }
  if (d->live_color_res) { d->live_color_res->Release(); d->live_color_res = nullptr; }
  d->color_capture_frame = 0u;
  d->hdr_scratch_w = 0u; d->hdr_scratch_h = 0u;
  if (d->prev_vp_cb) { d->prev_vp_cb->Release(); d->prev_vp_cb = nullptr; }
  if (d->prev_bones_srv) { d->prev_bones_srv->Release(); d->prev_bones_srv = nullptr; }
  if (d->prev_bones_buffer) { d->prev_bones_buffer->Release(); d->prev_bones_buffer = nullptr; }
  for (auto& [key, snap] : d->prev_bone_snaps) {
    if (snap.cur_srv) snap.cur_srv->Release();
    if (snap.cur_buffer) snap.cur_buffer->Release();
    if (snap.prev_srv) snap.prev_srv->Release();
    if (snap.prev_buffer) snap.prev_buffer->Release();
  }
  d->prev_bone_snaps.clear();
  d->last_bound_twin_srv = nullptr;
  if (d->prev_bone_saved_srv) { d->prev_bone_saved_srv->Release(); d->prev_bone_saved_srv = nullptr; }
  if (d->prev_vp_saved_cb) { d->prev_vp_saved_cb->Release(); d->prev_vp_saved_cb = nullptr; }
  d->prev_bone_slot = 0u;
  d->prev_vp_slot = 0u;
  d->patched_bind_restore_pending = false;
  d->patched_vs_by_new_hash.clear();
  d->patched_vs_hashes.clear();
  d->patched_ps_hashes.clear();
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
  pc_range.count = 52;  // matches motion_velocity.cs_5_0.hlsl cbuffer (13 float4s)
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
  if (!d->dbg_out_uav || d->dbg_out_uav_fmt != senkiseki3::dlss::ngx.output_format) {
    // Recreate when the NGX output format changes (float-output toggle).
    if (d->dbg_out_uav) { d->dbg_out_uav->Release(); d->dbg_out_uav = nullptr; }
    D3D11_UNORDERED_ACCESS_VIEW_DESC uavd = {};
    uavd.Format = senkiseki3::dlss::ngx.output_format;
    uavd.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
    uavd.Texture2D.MipSlice = 0;
    if (FAILED(nd->CreateUnorderedAccessView(senkiseki3::dlss::ngx.output_texture.Get(), &uavd, &d->dbg_out_uav)))
      return false;
    d->dbg_out_uav_fmt = senkiseki3::dlss::ngx.output_format;
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

// ── HDR-mod compatibility (Phase 2): DLSS input/output color conversion ──
// The senkiseki HDR mod encodes the composite as sRGB (processAndToneMap ->
// srgb::EncodeSafe) while DLSS always expects LINEAR input regardless of the
// IsHDR flag, so DLSS outputs dark/black content which the HDR swapchain proxy
// (sRGB decode -> PQ) amplifies to a pure black screen. These passes decode the
// captured composite to a linear scratch (DLSS input) and re-encode the DLSS
// output back to the encoding the HDR chain expects (0x9DB02646 SignPow + proxy).
static bool EnsureHdrConversion(reshade::api::device* dev, DeviceData* d, uint32_t w, uint32_t h) {
  auto* nd = reinterpret_cast<ID3D11Device*>(dev->get_native());
  if (!nd || !d || w == 0u || h == 0u) return false;

  if (!d->hdr_conv_cs) {
    if (FAILED(nd->CreateComputeShader(__hdr_convert.data(), static_cast<SIZE_T>(__hdr_convert.size()), nullptr, &d->hdr_conv_cs)))
      return false;
  }
  if (!d->hdr_conv_cb) {
    D3D11_BUFFER_DESC bd = {};
    bd.ByteWidth = 16;
    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    if (FAILED(nd->CreateBuffer(&bd, nullptr, &d->hdr_conv_cb))) return false;
  }
  const bool size_changed = (d->hdr_scratch_w != w || d->hdr_scratch_h != h);
  if (d->linear_scratch && size_changed) {
    if (d->linear_scratch_uav) { d->linear_scratch_uav->Release(); d->linear_scratch_uav = nullptr; }
    if (d->linear_scratch_srv) { d->linear_scratch_srv->Release(); d->linear_scratch_srv = nullptr; }
    if (d->linear_scratch) { d->linear_scratch->Release(); d->linear_scratch = nullptr; }
  }
  if (!d->linear_scratch) {
    D3D11_TEXTURE2D_DESC td = {};
    td.Width = w; td.Height = h; td.MipLevels = 1; td.ArraySize = 1;
    td.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
    if (FAILED(nd->CreateTexture2D(&td, nullptr, &d->linear_scratch))) return false;
    D3D11_SHADER_RESOURCE_VIEW_DESC srvd = {};
    srvd.Format = td.Format; srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D; srvd.Texture2D.MipLevels = 1;
    D3D11_UNORDERED_ACCESS_VIEW_DESC uavd = {};
    uavd.Format = td.Format; uavd.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D; uavd.Texture2D.MipSlice = 0;
    if (FAILED(nd->CreateShaderResourceView(d->linear_scratch, &srvd, &d->linear_scratch_srv))) return false;
    if (FAILED(nd->CreateUnorderedAccessView(d->linear_scratch, &uavd, &d->linear_scratch_uav))) return false;
  }
  if (d->encoded_scratch && size_changed) {
    if (d->encoded_scratch_uav) { d->encoded_scratch_uav->Release(); d->encoded_scratch_uav = nullptr; }
    if (d->encoded_scratch_srv) { d->encoded_scratch_srv->Release(); d->encoded_scratch_srv = nullptr; }
    if (d->encoded_scratch) { d->encoded_scratch->Release(); d->encoded_scratch = nullptr; }
  }
  if (!d->encoded_scratch) {
    D3D11_TEXTURE2D_DESC td = {};
    td.Width = w; td.Height = h; td.MipLevels = 1; td.ArraySize = 1;
    td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
    if (FAILED(nd->CreateTexture2D(&td, nullptr, &d->encoded_scratch))) return false;
    D3D11_SHADER_RESOURCE_VIEW_DESC srvd = {};
    srvd.Format = td.Format; srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D; srvd.Texture2D.MipLevels = 1;
    D3D11_UNORDERED_ACCESS_VIEW_DESC uavd = {};
    uavd.Format = td.Format; uavd.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D; uavd.Texture2D.MipSlice = 0;
    if (FAILED(nd->CreateShaderResourceView(d->encoded_scratch, &srvd, &d->encoded_scratch_srv))) return false;
    if (FAILED(nd->CreateUnorderedAccessView(d->encoded_scratch, &uavd, &d->encoded_scratch_uav))) return false;
  }
  // CPU staging for the NGX-output luma diagnostic — match the ngx output
  // format (r8g8b8a8, or r16g16b16a16_float when DLAAHdrFloatOut is on).
  {
    DXGI_FORMAT ngx_fmt = senkiseki3::dlss::ngx.output_format;
    if (ngx_fmt == DXGI_FORMAT_UNKNOWN) ngx_fmt = DXGI_FORMAT_R8G8B8A8_UNORM;
    if (d->ngx_dump_staging) {
      D3D11_TEXTURE2D_DESC td = {};
      d->ngx_dump_staging->GetDesc(&td);
      if (td.Format != ngx_fmt || td.Width != w || td.Height != h) {
        d->ngx_dump_staging->Release(); d->ngx_dump_staging = nullptr;
      }
    }
    if (!d->ngx_dump_staging) {
      D3D11_TEXTURE2D_DESC td = {};
      td.Width = w; td.Height = h; td.MipLevels = 1; td.ArraySize = 1;
      td.Format = ngx_fmt;
      td.SampleDesc.Count = 1;
      td.Usage = D3D11_USAGE_STAGING;
      td.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
      if (FAILED(nd->CreateTexture2D(&td, nullptr, &d->ngx_dump_staging))) return false;
    }
  }
  d->hdr_scratch_w = w; d->hdr_scratch_h = h;
  return true;
}

// Run one color-conversion compute pass on the native context (mode: 0=sRGB
// decode, 1=sRGB encode, 2=PQ decode, 3=PQ encode).
static bool RunHdrConvertPass(ID3D11DeviceContext* cl, DeviceData* d, int mode,
                              ID3D11ShaderResourceView* src_srv,
                              ID3D11UnorderedAccessView* dst_uav,
                              uint32_t w, uint32_t h) {
  if (!cl || !d || !d->hdr_conv_cs || !d->hdr_conv_cb || !src_srv || !dst_uav) return false;
  D3D11_MAPPED_SUBRESOURCE map = {};
  if (FAILED(cl->Map(d->hdr_conv_cb, 0, D3D11_MAP_WRITE_DISCARD, 0, &map))) return false;
  float* p = static_cast<float*>(map.pData);
  p[0] = (float)mode; p[1] = 0.f; p[2] = 0.f; p[3] = 0.f;
  cl->Unmap(d->hdr_conv_cb, 0);
  ID3D11ShaderResourceView* srvs[1] = { src_srv };
  ID3D11UnorderedAccessView* uavs[1] = { dst_uav };
  cl->CSSetShaderResources(0, 1, srvs);
  cl->CSSetUnorderedAccessViews(0, 1, uavs, nullptr);
  cl->CSSetConstantBuffers(13, 1, &d->hdr_conv_cb);
  cl->CSSetShader(d->hdr_conv_cs, nullptr, 0);
  cl->Dispatch((w + 7) / 8, (h + 7) / 8, 1);
  ID3D11UnorderedAccessView* null_uav = nullptr;
  cl->CSSetUnorderedAccessViews(0, 1, &null_uav, nullptr);
  ID3D11ShaderResourceView* null_srv[1] = {};
  cl->CSSetShaderResources(0, 1, null_srv);
  cl->CSSetShader(nullptr, nullptr, 0);
  return true;
}

// IEEE 754 half -> float (for the r16g16b16a16_float NGX output luma dump).
static float HalfToFloat(uint16_t h) {
  const uint32_t sign = (uint32_t)(h & 0x8000u) << 16;
  uint32_t exp = (h >> 10) & 0x1Fu;
  uint32_t man = h & 0x3FFu;
  uint32_t f;
  if (exp == 0u) {
    if (man == 0u) {
      f = sign;
    } else {
      exp = 127u - 15u + 1u;
      while ((man & 0x400u) == 0u) { man <<= 1u; --exp; }
      man &= 0x3FFu;
      f = sign | (exp << 23) | (man << 13);
    }
  } else if (exp == 31u) {
    f = sign | 0x7F800000u | (man << 13);
  } else {
    f = sign | ((exp - 15u + 127u) << 23) | (man << 13);
  }
  float out = 0.f;
  std::memcpy(&out, &f, sizeof(out));
  return out;
}

// DIAGNOSTIC: copy the NGX output to a CPU staging texture and log min/avg/max
// RGB luma (throttled ~1 Hz by the caller). Distinguishes "DLSS wrote PURE 0
// (eval/config failure)" from "DLSS wrote DARK-but-nonzero content". Handles
// both r8g8b8a8 and r16g16b16a16_float NGX output. GPU stall acceptable at 1 Hz.
static void LogNgxOutputLuma(ID3D11DeviceContext* cl, DeviceData* d) {
  if (!cl || !d || !d->ngx_dump_staging) return;
  auto* ngx_out = senkiseki3::dlss::ngx.output_texture.Get();
  if (!ngx_out) return;
  cl->CopyResource(d->ngx_dump_staging, ngx_out);
  D3D11_MAPPED_SUBRESOURCE map = {};
  if (FAILED(cl->Map(d->ngx_dump_staging, 0, D3D11_MAP_READ, 0, &map))) return;
  D3D11_TEXTURE2D_DESC td = {};
  d->ngx_dump_staging->GetDesc(&td);
  const bool is_float = (td.Format == DXGI_FORMAT_R16G16B16A16_FLOAT);
  auto* px = static_cast<const uint8_t*>(map.pData);
  double sum = 0.0;
  float mn = 1e9f, mx = -1e9f;
  uint64_t samples = 0;
  const uint32_t stride = map.RowPitch;
  for (uint32_t y = 0; y < d->hdr_scratch_h; y += 8) {
    const uint8_t* row = px + (size_t)y * stride;
    for (uint32_t x = 0; x < d->hdr_scratch_w; x += 8) {
      float r, g, b;
      if (is_float) {
        const uint16_t* p = reinterpret_cast<const uint16_t*>(row + (size_t)x * 8u);
        r = HalfToFloat(p[0]); g = HalfToFloat(p[1]); b = HalfToFloat(p[2]);
      } else {
        r = row[x * 4 + 0] * (1.f / 255.f);
        g = row[x * 4 + 1] * (1.f / 255.f);
        b = row[x * 4 + 2] * (1.f / 255.f);
      }
      float l = (r + g + b) * (1.f / 3.f);
      sum += l; mn = std::min(mn, l); mx = std::max(mx, l);
      samples++;
    }
  }
  cl->Unmap(d->ngx_dump_staging, 0);
  if (samples == 0) return;
  float avg = (float)(sum / (double)samples);
  char buf[128];
  snprintf(buf, sizeof(buf), "[DLAA] NGX output luma: min=%.3f avg=%.3f max=%.3f (min=0 => DLSS wrote pure black)",
           mn, avg, mx);
  reshade::log::message(reshade::log::level::info, buf);
}

// Ensure the CPU readback staging for the DLSS color-source diagnostic.
static bool EnsureColorDumpStaging(reshade::api::device* dev, DeviceData* d, uint32_t w, uint32_t h) {
  auto* nd = reinterpret_cast<ID3D11Device*>(dev->get_native());
  if (!nd || !d || w == 0u || h == 0u) return false;
  if (d->color_dump_staging) {
    D3D11_TEXTURE2D_DESC td = {};
    d->color_dump_staging->GetDesc(&td);
    if (td.Width == w && td.Height == h) return true;
    d->color_dump_staging->Release(); d->color_dump_staging = nullptr;
  }
  D3D11_TEXTURE2D_DESC td = {};
  td.Width = w; td.Height = h; td.MipLevels = 1; td.ArraySize = 1;
  td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  td.SampleDesc.Count = 1;
  td.Usage = D3D11_USAGE_STAGING;
  td.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
  return SUCCEEDED(nd->CreateTexture2D(&td, nullptr, &d->color_dump_staging));
}

// DIAGNOSTIC: report the ACTUAL color source handed to DLSS — live t0 vs event
// capture, content min/avg/max luma (CPU readback), capture staleness, and the
// FXAA draw's resolved PS hash (0 => "Pipeline not found" capture skip).
static void LogColorSource(reshade::api::command_list* cmd_list, DeviceData* d,
                           ID3D11Resource* color_res, bool using_live) {
  auto* dev = cmd_list->get_device();
  auto* cl = reinterpret_cast<ID3D11DeviceContext*>(cmd_list->get_native());
  if (!dev || !d || !cl || !color_res) return;

  D3D11_TEXTURE2D_DESC td = {};
  static_cast<ID3D11Texture2D*>(color_res)->GetDesc(&td);

  float mn = 0.f, avg = 0.f, mx = 0.f;
  bool have_luma = false;
  if (td.Format == DXGI_FORMAT_R8G8B8A8_UNORM && EnsureColorDumpStaging(dev, d, td.Width, td.Height)) {
    cl->CopyResource(d->color_dump_staging, color_res);
    D3D11_MAPPED_SUBRESOURCE map = {};
    if (SUCCEEDED(cl->Map(d->color_dump_staging, 0, D3D11_MAP_READ, 0, &map))) {
      auto* px = static_cast<const uint8_t*>(map.pData);
      double sum = 0.0; float mnv = 1e9f, mxv = -1e9f; uint64_t n = 0;
      for (uint32_t y = 0; y < td.Height; y += 8) {
        const uint8_t* row = px + (size_t)y * map.RowPitch;
        for (uint32_t x = 0; x < td.Width; x += 8) {
          float l = (row[x * 4 + 0] + row[x * 4 + 1] + row[x * 4 + 2]) * (1.f / (3.f * 255.f));
          sum += l; mnv = std::min(mnv, l); mxv = std::max(mxv, l); n++;
        }
      }
      cl->Unmap(d->color_dump_staging, 0);
      if (n) { mn = mnv; mx = mxv; avg = (float)(sum / (double)n); have_luma = true; }
    }
  }

  auto* ss = renodx::utils::shader::GetCurrentState(cmd_list);
  uint32_t hash = ss ? renodx::utils::shader::GetCurrentPixelShaderHash(ss) : 0u;
  uint32_t stale = d->frame_index >= d->color_capture_frame ? d->frame_index - d->color_capture_frame : 0u;
  char buf[200];
  if (have_luma) {
    snprintf(buf, sizeof(buf),
      "[DLAA] color src: %s res=0x%llX fmt=%d %ux%u luma=(%.3f,%.3f,%.3f) capStale=%u fxaaHash=0x%X",
      using_live ? "LIVE" : "CAPTURE", (unsigned long long)color_res,
      (int)td.Format, (unsigned)td.Width, (unsigned)td.Height, mn, avg, mx, stale, hash);
  } else {
    snprintf(buf, sizeof(buf),
      "[DLAA] color src: %s res=0x%llX fmt=%d %ux%u (luma readback skipped) capStale=%u fxaaHash=0x%X",
      using_live ? "LIVE" : "CAPTURE", (unsigned long long)color_res,
      (int)td.Format, (unsigned)td.Width, (unsigned)td.Height, stale, hash);
  }
  reshade::log::message(reshade::log::level::info, buf);
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
static const std::array<uint32_t, 5> DEPTH_VS_HASHES = {
    0xAA8821ECu,  // character shadow (user-reported)
    0xAB857156u,  // hat shadow (draw 1) — was missing, shadow shook in global jitter
    0x78F969DDu,  // character depth
    0xA1D82AA3u,  // character depth
    0x600D64CCu,  // character depth
};

static bool IsDepthVs(uint32_t hash) {
  // DLAAPhaseJitterDepth (diagnostic): jitter the character's MAIN depth pass
  // too so DLSS sees color AND depth in the same jitter state (friend's red
  // flag #1). Only the character SHADOW VS (0xAA8821EC) stays unjittered — its
  // consumer compares against the unjittered projection. Default (off): all
  // depth VSs stay unjittered.
  if (g_phase_jitter_depth > 0.5f) {
    return hash == 0xAA8821ECu;
  }
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
  auto* state = renodx::utils::shader::GetCurrentState(cmd_list);
  const uint32_t vh = state ? renodx::utils::shader::GetCurrentVertexShaderHash(state) : 0u;
  const uint32_t ph = state ? renodx::utils::shader::GetCurrentPixelShaderHash(state) : 0u;
  // Diagnostic: the white part of the eye shows background in HSV (no per-object
  // motion). Capture EVERY draw whose VS is per-object OR whose PS is one of our
  // patched outputs, with the bound RTV count, so we can see why the white part
  // is excluded (most likely: drawn in a pass with <3 RTs, or with an unpatched
  // PS). DLAADebugLogging must be on.
  const bool is_po_vs = d->patched_vs_by_new_hash.contains(vh) || IsPerObjectMotionVs(vh);
  const bool is_patched_ps = d->patched_ps_hashes.contains(ph);
  if (shader_injection.dlaa_debug_logging > 0.5f && (is_po_vs || is_patched_ps))
    LogThrottled("motion-draw", reshade::log::level::info, 200u, 100u,
                 "[DLAA] motion: draw vs=0x%08X ps=0x%08X rtvs=%u poVS=%d patchedPS=%d",
                 vh, ph, d->last_rtv_count, (int)is_po_vs, (int)is_patched_ps);
  // G-buffer only: require the game's color/normal/depth MRT set (>=3 RTs).
  // Depth/shadow/effect passes bind fewer RTs and must never get the appended
  // motion RTV (the generic patcher now touches many more VSs than the old
  // 24-hash list, so this guard matters).
  if (!cmd_list || !d || !d->motion_rtv.handle || d->last_rtv_count < 3u) return;
  // Gate on BOTH:
  //  (a) VSs the generic in-place patcher modified (patched NEW hashes), and
  //  (b) the ORIGINAL per-object char VS hashes as a fallback gate (covers the
  //      face 0x0D5DABC6 and any char VS that draws with its original hash).
  //      All skinned char VSs are generic-patched (they are NOT in
  //      custom_shaders — the boot-HLSL VS replacements don't emit prevClip,
  //      so listing them there made their paired PSs read garbage v7).
  if (!is_po_vs) {
    // Diagnostic: log near-miss draws (per-object PS but unrecognized VS) so we
    // can see why a part (e.g. the white of the eye) may not get per-object
    // motion. Only log when the PS is one of our patched outputs, to avoid noise.
    if (shader_injection.dlaa_debug_logging > 0.5f && d->patched_ps_hashes.contains(ph))
      LogThrottled("motion-gate", reshade::log::level::info, 40u, 250u,
                   "[DLAA] motion: SKIP (VS not per-object) vs=0x%08X ps=0x%08X rtvs=%u",
                   vh, ph, d->last_rtv_count);
    return;
  }
  auto* dev = cmd_list->get_device();
  if (!dev || !d->last_rtvs[0].handle) return;
  // D3D11 requires all bound RTs to share identical dimensions — only append
  // the target to full-res passes.
  reshade::api::resource_desc mr = dev->get_resource_desc(d->motion_texture);
  reshade::api::resource_desc rd = dev->get_resource_desc(dev->get_resource_from_view(d->last_rtvs[0]));
  if (rd.type != reshade::api::resource_type::texture_2d) return;
  if (rd.texture.width != mr.texture.width || rd.texture.height != mr.texture.height) return;
  if (shader_injection.dlaa_debug_logging > 0.5f)
    LogThrottled("motion-append", reshade::log::level::info, 60u, 200u,
                 "[DLAA] motion: APPEND vs=0x%08X ps=0x%08X rtvs=%u",
                 vh, ph, d->last_rtv_count);
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

// ── Phase D: per-instance prev-bone snapshot registry ──
// Find (or create) the addon-owned cur/prev snapshot pair for a character
// INSTANCE (key = composite of the per-object b0 cbuffer handle + VB slot0
// handle). Each snap holds a cur buffer (draw-time capture of THIS frame's
// bones) and a prev buffer (promoted from cur at present; bound to the patched
// VS so it re-skins with the PREVIOUS frame's bones). Returns the snap, or
// nullptr on allocation failure. Bounded (LRU evicts the least-recently-used).
static DeviceData::PrevBoneSnap* EnsurePrevBoneSnap(reshade::api::device* dev, DeviceData* d,
                                                    uint64_t key, uint32_t bytes) {
  if (!dev || !d || key == 0u || bytes == 0u) return nullptr;
  // Existing snap for this instance, same size?
  auto it = d->prev_bone_snaps.find(key);
  if (it != d->prev_bone_snaps.end()) {
    if (it->second.size == bytes) return &it->second;
    // Size changed (rare): release and recreate below.
    if (it->second.cur_srv) it->second.cur_srv->Release();
    if (it->second.cur_buffer) it->second.cur_buffer->Release();
    if (it->second.prev_srv) it->second.prev_srv->Release();
    if (it->second.prev_buffer) it->second.prev_buffer->Release();
    d->prev_bone_snaps.erase(it);
  }
  // LRU evict when at capacity.
  if (d->prev_bone_snaps.size() >= 128u) {
    uint64_t oldest = UINT64_MAX;
    uint64_t oldest_key = 0u;
    for (auto& [k, s] : d->prev_bone_snaps) {
      if (s.last_draw_frame < oldest) {
        oldest = s.last_draw_frame;
        oldest_key = k;
      }
    }
    auto& o = d->prev_bone_snaps[oldest_key];
    if (o.cur_srv) o.cur_srv->Release();
    if (o.cur_buffer) o.cur_buffer->Release();
    if (o.prev_srv) o.prev_srv->Release();
    if (o.prev_buffer) o.prev_buffer->Release();
    d->prev_bone_snaps.erase(oldest_key);
  }
  // Create the cur/prev pair (DEFAULT structured buffers, same size + SRVs).
  // Both start as identity bone matrices so frame 0's prev is the safe bind-pose
  // placeholder (never uninitialized garbage bones in the first draw).
  auto* nd = reinterpret_cast<ID3D11Device*>(dev->get_native());
  if (!nd) return nullptr;
  DeviceData::PrevBoneSnap snap;
  auto create_pair = [&](ID3D11Buffer** buf, ID3D11ShaderResourceView** srv) -> bool {
    D3D11_BUFFER_DESC bd = {};
    bd.ByteWidth = bytes;
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    bd.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    bd.StructureByteStride = 64u;
    // Identity bone matrices (bytes/64 elements, each = 4x4 identity).
    std::vector<uint8_t> identity(bytes, 0);
    for (uint32_t i = 0; i < bytes / 64u; ++i) {
      float* m = reinterpret_cast<float*>(identity.data() + (size_t)i * 64u);
      m[0] = m[5] = m[10] = m[15] = 1.f;
    }
    D3D11_SUBRESOURCE_DATA init = {};
    init.pSysMem = identity.data();
    init.SysMemPitch = bytes;
    init.SysMemSlicePitch = 0u;
    if (FAILED(nd->CreateBuffer(&bd, &init, buf))) return false;
    D3D11_SHADER_RESOURCE_VIEW_DESC sd = {};
    sd.Format = DXGI_FORMAT_UNKNOWN;
    sd.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
    sd.BufferEx.FirstElement = 0u;
    sd.BufferEx.NumElements = bytes / 64u;
    if (FAILED(nd->CreateShaderResourceView(*buf, &sd, srv))) {
      (*buf)->Release();
      *buf = nullptr;
      return false;
    }
    return true;
  };
  if (!create_pair(&snap.cur_buffer, &snap.cur_srv)) return nullptr;
  if (!create_pair(&snap.prev_buffer, &snap.prev_srv)) return nullptr;
  snap.size = bytes;
  snap.last_draw_frame = 0u;
  d->prev_bone_snaps.emplace(key, snap);
  return &d->prev_bone_snaps[key];
}

// ── Phase D: frame-end prev-bone promotion ──
// CopyResource(cur -> prev) for every per-instance snapshot, so the next
// frame's patched VS re-skins with the PREVIOUS frame's bones (the cur captured
// at draw time this frame). Called at present (after all draws of frame N).
// Gated by DLAAPerObjectMotion = Prev-Bone (>= 1.5). Uses the native immediate
// context.
static void CapturePrevBones(reshade::api::command_list* cmd_list, DeviceData* d) {
  if (!cmd_list || !d) return;
  if (shader_injection.dlaa_per_object_motion < 1.5f) return;  // Prev-Bone option only
  if (d->prev_bone_snaps.empty()) return;
  auto* ctx = reinterpret_cast<ID3D11DeviceContext*>(cmd_list->get_native());
  if (!ctx) return;
  for (auto& [key, snap] : d->prev_bone_snaps) {
    if (snap.prev_buffer && snap.cur_buffer) ctx->CopyResource(snap.prev_buffer, snap.cur_buffer);
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
    // Draw-time per-INSTANCE prev-bone capture. At this point the game has JUST
    // bound THIS frame's bone buffer at bone_game_slot (usually t0) — that
    // buffer holds frame N's bones RIGHT NOW. We CopyResource it into this
    // instance's "cur" snapshot and bind the instance's "prev" (promoted from
    // cur at present) so the patched VS re-skins with frame N-1's bones.
    //
    // Key = CHARACTER INSTANCE (per-object b0 cbuffer + VB slot0 handle), NOT
    // the VS hash (a VS is SHARED across many characters — eyeballs/skin/
    // clothing — so a shader key would mix their bones) and NOT the bone handle
    // (the game rotates a RING of bone buffers, frame N writes buffer[N%4], so
    // a handle-keyed twin is 3-4 frames stale). Capturing at draw time keyed by
    // the stable instance is ring-agnostic. Only ever bind our own snapshot SRV
    // (never the game's SRV itself) so the ld_structured descriptor kind is
    // always valid.
    ID3D11Buffer* b0 = nullptr;
    ctx->VSGetConstantBuffers(0u, 1u, &b0);
    ID3D11Buffer* vb0 = nullptr;
    UINT vb_stride = 0u, vb_offset = 0u;
    ctx->IAGetVertexBuffers(0u, 1u, &vb0, &vb_stride, &vb_offset);
    if (b0 && vb0) {
      // Boost-style hash_combine: heap pointers' entropy is mostly in the low
      // bits, so a naive (b0<<32)^vb0 would collide; this mixes both fully.
      const uint64_t b0k = (uint64_t)(uintptr_t)b0;
      const uint64_t vbk = (uint64_t)(uintptr_t)vb0;
      const uint64_t key = b0k ^ (vbk + 0x9e3779b97f4a7c15ull + (b0k << 6) + (b0k >> 2));
      ID3D11ShaderResourceView* game_srv = nullptr;
      ctx->VSGetShaderResources(it->second.bone_game_slot, 1u, &game_srv);
      if (game_srv) {
        ID3D11Resource* res = nullptr;
        game_srv->GetResource(&res);
        if (res) {
          D3D11_RESOURCE_DIMENSION dim = D3D11_RESOURCE_DIMENSION_UNKNOWN;
          res->GetType(&dim);
          if (dim == D3D11_RESOURCE_DIMENSION_BUFFER) {
            auto* gb = static_cast<ID3D11Buffer*>(res);
            D3D11_BUFFER_DESC gd = {};
            gb->GetDesc(&gd);
            if ((gd.MiscFlags & D3D11_RESOURCE_MISC_BUFFER_STRUCTURED) && gd.StructureByteStride == 64u) {
              DeviceData::PrevBoneSnap* snap =
                  EnsurePrevBoneSnap(cmd_list->get_device(), d, key, gd.ByteWidth);
              if (snap) {
                // Capture this frame's bones into cur ONCE per frame per
                // instance (multiple draws of the same instance/VS in one frame
                // share one snapshot — the game binds the same pose for all).
                if (snap->last_draw_frame != d->frame_index) {
                  ctx->CopyResource(snap->cur_buffer, gb);
                  snap->last_draw_frame = d->frame_index;
                }
                bone_srv = snap->prev_srv;
                d->last_bound_twin_srv = snap->prev_srv;
              }
            }
          }
          res->Release();
        }
        game_srv->Release();
      }
    }
    if (b0) b0->Release();
    if (vb0) vb0->Release();
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

// The game's native depth is the R24G8 output res we auto-check (captured_depth_res).
// A draw WRITES that depth when its bound depth-stencil IS that same resource
// AND no color RT is bound (a true depth-only pass — the game fills the R24G8
// in its own depth pass, then color draws later). The game's own depth is NOT
// jittered, so the source patch must leave this buffer's VP unjittered
// (depth_only) — DLSS then gets the native unjittered depth while the color
// passes keep the jittered VP. Color passes that merely depth-test against the
// R24G8 have color RTs bound, so they never match here and stay jittered.
static bool IsDepthWriteDraw(reshade::api::command_list* cmd_list, DeviceData* d) {
  // DLAAPhaseJitterDepth ON: we WANT the world depth pass jittered (Test B:
  // color AND depth in the same jitter state) — so do NOT mark it depth_only.
  // Default OFF: mark it depth_only so the source patch leaves the VP unjittered
  // and DLSS gets the game's native unjittered depth (Test A).
  if (g_phase_jitter_depth > 0.5f) return false;
  if (!cmd_list || !d) return false;
  if (!d->captured_depth_res.handle) return false;
  auto* ctx = reinterpret_cast<ID3D11DeviceContext*>(cmd_list->get_native());
  if (!ctx) return false;
  ID3D11RenderTargetView* rtvs[8] = {};
  ID3D11DepthStencilView* dsv = nullptr;
  UINT num_rtvs = 8;
  ctx->OMGetRenderTargets(num_rtvs, rtvs, &dsv);
  if (!dsv) {
    for (UINT i = 0; i < num_rtvs; ++i) if (rtvs[i]) rtvs[i]->Release();
    return false;
  }
  // Depth-only pass: no color target bound.
  bool no_color = (num_rtvs == 0u);
  ID3D11Resource* res = nullptr;
  dsv->GetResource(&res);
  bool match = false;
  if (res) {
    match = no_color && (reinterpret_cast<uintptr_t>(res) == d->captured_depth_res.handle);
    res->Release();
  }
  dsv->Release();
  for (UINT i = 0; i < num_rtvs; ++i) if (rtvs[i]) rtvs[i]->Release();
  return match;
}

static void MaybeWriteGlobalsVp(reshade::api::command_list* cmd_list, DeviceData* d) {
  if (!cmd_list || !d) return;
  if (shader_injection.dlaa_jitter_method < 0.5f) return;
  if (shader_injection.dlaa_jitter_enabled < 0.5f && shader_injection.dlaa_jitter_test < 0.5f) return;
  // Gate-failure diagnostics: periodic (first few, then every 250th) so we can
  // see WHY the draw-time write doesn't fire in-game even after the start-menu
  // consumed the first-slot probes.
  // DLAAPhaseJitterColor OFF: nothing is jittered at the source, so there is
  // nothing to correct at draw time (no effect un-jitter, no fallback write).
  if (g_phase_jitter_color < 0.5f) return;
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
  // ── Depth passes: keep the VP UNJITTERED so DLSS gets the game's native
  // depth. Two signals identify a depth-writing draw:
  //   * IsDepthVs(vh): known depth/shadow VSs (character draws + shadow maps
  //     whose consumers compare against the unjittered projection);
  //   * the bound DSV IS the captured R24G8 (captured_depth_res): the WORLD
  //     depth pass. The game's own depth is NOT jittered, so the source patch
  //     must NOT jitter this buffer's VP — mark it depth_only here, then the
  //     source patch (OnUpdateBufferRegion) lets the original upload stand.
  //     Color passes don't bind the depth buffer as DSV, so they keep the
  //     jittered VP.
  if (IsDepthVs(vh) || IsDepthWriteDraw(cmd_list, d)) {
    MarkDepthBuffer(d, d->last_b0_buffer);
    if (shader_injection.dlaa_debug_logging > 0.5f) {
      LogThrottled("depth-vs", reshade::log::level::info, 5u, 250u,
                   "[DLAA] global: DEPTH draw vs=0x%08X buffer=0x%llX size=%llu (unjittered)",
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
  // EXCEPT depth_only buffers: those are the character's shared b0 (marked by
  // the depth/shadow pass), whose upload the source patch left UNJITTERED — so
  // the G-buffer draw must jitter it here, or the character rasterizes unjittered
  // while the per-object velocity path subtracts jitter -> MV error -> shake.
  auto* rec = FindGlobalsRec(d, d->last_b0_buffer);
  if (want_jittered && rec && !rec->depth_only) return;
  // Choose the VP to write: this buffer's own unjittered VP when known (effect
  // un-jitter / depth_only G-buffer jitter), else the global captured VP
  // (jittered for untracked scene fallback, unjittered for untracked effect
  // fallback).
  std::array<float, 16> vp = rec ? rec->unjittered_vp : d->globals_unjittered_vp;
  if (want_jittered) {
    for (int i = 0; i < 4; ++i) {
      vp[i] += d->jitter_x * vp[12 + i];
      vp[4 + i] += d->jitter_y * vp[12 + i];
    }
  }
  dev->update_buffer_region(vp.data(), d->last_b0_buffer, 160ull, 64ull);
  if (shader_injection.dlaa_debug_logging > 0.5f) {
    LogThrottled("vp-write", reshade::log::level::info, 5u, 250u,
                 "[DLAA] global: draw VP write buffer=0x%llX size=%llu jittered=%d depth_only=%d ps=0x%08X",
                 (unsigned long long)d->last_b0_buffer.handle,
                 (unsigned long long)rd.buffer.size, (int)want_jittered,
                 (int)(rec && rec->depth_only), phash);
  }
}

// Phase 0 prev-pose probe (defined later; forward decl so the draw hooks call it).
static void Phase0ProbeDraw(reshade::api::command_list* cmd_list, DeviceData* d);

// Draw hooks: append the mask RT right before effect draws (the PS is
// guaranteed to be bound here, unlike at the RT-bind event).
// SCENE-3 TDR diagnostic: log the REAL output-merger state (render-target
// formats + sample counts + DSV format/samples) on every draw whose VS or PS is
// one of our patched outputs. The drawtrace only logs the tracked BIND count;
// if scene 3's char draws bind a 4th RTV at slot 3 (a game target our o3 write
// would collide with) or multisampled targets, this reveals it directly.
// Queries the NATIVE D3D11 context (the real state, not our tracking).
// Gated on DLAADebugLogging.
static void MaybeLogOmState(reshade::api::command_list* cmd_list, DeviceData* d) {
  if (shader_injection.dlaa_debug_logging <= 0.5f || !cmd_list || !d) return;
  auto* state = renodx::utils::shader::GetCurrentState(cmd_list);
  const uint32_t vh = state ? renodx::utils::shader::GetCurrentVertexShaderHash(state) : 0u;
  const uint32_t ph = state ? renodx::utils::shader::GetCurrentPixelShaderHash(state) : 0u;
  const bool is_po_vs = d->patched_vs_by_new_hash.contains(vh) || IsPerObjectMotionVs(vh);
  const bool is_patched_ps = d->patched_ps_hashes.contains(ph);
  if (!is_po_vs && !is_patched_ps) return;
  auto* ctx = reinterpret_cast<ID3D11DeviceContext*>(cmd_list->get_native());
  if (!ctx) return;
  ID3D11RenderTargetView* rtvs[8] = {};
  ID3D11DepthStencilView* dsv = nullptr;
  ctx->OMGetRenderTargets(8, rtvs, &dsv);  // fills up to 8; count non-null below
  UINT count = 0;
  for (UINT i = 0u; i < 8u; ++i) if (rtvs[i]) ++count;
  char line[512];
  int pos = snprintf(line, sizeof(line), "[DLAA] OM vs=0x%08X ps=0x%08X realRtvs=%u",
                     vh, ph, count);
  for (UINT i = 0; i < 8u && i < count && pos > 0 && pos < (int)sizeof(line); ++i) {
    if (!rtvs[i]) continue;
    D3D11_RENDER_TARGET_VIEW_DESC rd = {};
    rtvs[i]->GetDesc(&rd);
    UINT samples = 1;
    ID3D11Resource* res = nullptr;
    rtvs[i]->GetResource(&res);
    if (res) {
      D3D11_RESOURCE_DIMENSION dim;
      res->GetType(&dim);
      if (dim == D3D11_RESOURCE_DIMENSION_TEXTURE2D) {
        D3D11_TEXTURE2D_DESC td = {};
        static_cast<ID3D11Texture2D*>(res)->GetDesc(&td);
        samples = td.SampleDesc.Count;
      }
      res->Release();
    }
    pos += snprintf(line + pos, sizeof(line) - (size_t)pos, " r%d=fmt%d ms%d",
                    (int)i, (int)rd.Format, (int)samples);
  }
  if (dsv) {
    D3D11_DEPTH_STENCIL_VIEW_DESC dd = {};
    dsv->GetDesc(&dd);
    UINT dsv_samples = 1;
    ID3D11Resource* res = nullptr;
    dsv->GetResource(&res);
    if (res) {
      D3D11_RESOURCE_DIMENSION dim;
      res->GetType(&dim);
      if (dim == D3D11_RESOURCE_DIMENSION_TEXTURE2D) {
        D3D11_TEXTURE2D_DESC td = {};
        static_cast<ID3D11Texture2D*>(res)->GetDesc(&td);
        dsv_samples = td.SampleDesc.Count;
      }
      res->Release();
    }
    pos += snprintf(line + pos, sizeof(line) - (size_t)pos, " dsv=fmt%d ms%d",
                    (int)dd.Format, (int)dsv_samples);
  }
  for (UINT i = 0; i < 8u && i < count; ++i) if (rtvs[i]) rtvs[i]->Release();
  if (dsv) dsv->Release();
  LogThrottled("om-state", reshade::log::level::info, 60u, 250u, "%s", line);
}

static bool OnDrawMaskHook(reshade::api::command_list* cmd_list, uint32_t, uint32_t, uint32_t, uint32_t) {
  auto* dev = cmd_list->get_device();
  if (!dev) return false;
  auto* d = dev->get_private_data<DeviceData>();
  if (d) {
    WatchdogStampDraw(cmd_list, d, "enter");
    MaybeRestorePatchedBinds(cmd_list, d);
    Phase0ProbeDraw(cmd_list, d);
    MaybeTraceCrashWindow(cmd_list, d);
    MaybeLogOmState(cmd_list, d);
    ApplyPerDrawJitter(cmd_list, d);
    MaybeWriteGlobalsVp(cmd_list, d);
    MaybeLogEffectDraw(cmd_list, d);
    MaybeAppendEffectMask(cmd_list, d);
    MaybeBindPatchedVs(cmd_list, d);
    MaybeAppendMotionRtv(cmd_list, d);
    WatchdogStampDraw(cmd_list, d, "done");
  }
  return false;
}
static bool OnDrawMaskHookIndexed(reshade::api::command_list* cmd_list, uint32_t, uint32_t, uint32_t, int32_t, uint32_t) {
  auto* dev = cmd_list->get_device();
  if (!dev) return false;
  auto* d = dev->get_private_data<DeviceData>();
  if (d) {
    WatchdogStampDraw(cmd_list, d, "enter");
    MaybeRestorePatchedBinds(cmd_list, d);
    Phase0ProbeDraw(cmd_list, d);
    MaybeTraceCrashWindow(cmd_list, d);
    MaybeLogOmState(cmd_list, d);
    ApplyPerDrawJitter(cmd_list, d);
    MaybeWriteGlobalsVp(cmd_list, d);
    MaybeLogEffectDraw(cmd_list, d);
    MaybeAppendEffectMask(cmd_list, d);
    MaybeBindPatchedVs(cmd_list, d);
    MaybeAppendMotionRtv(cmd_list, d);
    WatchdogStampDraw(cmd_list, d, "done");
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
  // Frame-pairing stamp: this frame's VP belongs to frame_index (now); the
  // velocity reprojection uses it as the CURRENT VP and the previous stamp's
  // VP as prevViewProj — so prev_view_proj must be exactly one frame older.
  d->prev_vp_frame = d->vp_read_frame;
  d->vp_read_frame = d->frame_index;
  d->vp_cbv_handle = d->captured_scene_cbv.buffer.handle;
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
  uint64_t vb_handle = 0ull, b0_handle = 0ull;
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
        vb_handle = (uint64_t)(uintptr_t)vb;
        vb->Release();
        d->p0_vs_vb_bind_flags[vhash] = bd.BindFlags;
      }
      // Per-object b0 cbuffer (World at c44) = the per-instance key's upper
      // half. Verify it is STABLE per character across frames (while the bone
      // handle rotates through the ring) and DISTINCT across characters that
      // share a VS — the premise of the draw-time snapshot key.
      ID3D11Buffer* b0 = nullptr;
      native_ctx->VSGetConstantBuffers(0, 1, &b0);
      if (b0) {
        b0_handle = (uint64_t)(uintptr_t)b0;
        b0->Release();
      }
    }
  }
  bool gpu_skinned = (vb_bind_flags & D3D11_BIND_UNORDERED_ACCESS) != 0;
  // Same hash_combine as the draw-time snapshot key in MaybeBindPatchedVs.
  uint64_t inst_key = b0_handle ^ (vb_handle + 0x9e3779b97f4a7c15ull + (b0_handle << 6) + (b0_handle >> 2));

  LogThrottled("p0-draw", reshade::log::level::info, 30u, 300u,
               "[P0] per-object draw vs=0x%08X ps=0x%08X bone=0x%llX size=%u stride=%u vbBindFlags=0x%X b0=0x%llX vb0=0x%llX instKey=0x%llX %s%s",
               vhash, phash, (unsigned long long)bone,
               d->last_bone_size, d->last_bone_stride, vb_bind_flags,
               (unsigned long long)b0_handle, (unsigned long long)vb_handle,
               (unsigned long long)inst_key,
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

    // Color: FXAA (0x96BB8CFF) or pre-FXAA composite (0xE8C7EBA2) at t0.
    // POISON GUARD (Phase 3): never accept our own NGX-output bind — RunDLAA's
    // native PSSetShaderResources(0, ngx_out) is intercepted by ReShade and
    // re-fires this hook with the FXAA draw's hash, which would overwrite the
    // captured composite with the (black) DLSS output. RunDLAA prefers the LIVE
    // t0 (PSGetShaderResources); this event capture is the fallback + feeds the
    // reprojection debug view.
    if (update.binding == 0u && update.count >= 1 && views[0].handle != 0u) {
      auto res = dev->get_resource_from_view(views[0]);
      bool is_ngx_out = senkiseki3::dlss::ngx.output_texture &&
          res.handle == reinterpret_cast<uintptr_t>(senkiseki3::dlss::ngx.output_texture.Get());
      if (res.handle && !is_ngx_out) {
        auto* ss = renodx::utils::shader::GetCurrentState(cmd_list);
        if (ss) {
          uint32_t hash = renodx::utils::shader::GetCurrentPixelShaderHash(ss);
          if (hash == 0x96BB8CFFu || hash == 0xE8C7EBA2u) {
            d->captured_color_srv = views[0];
            d->captured_color_res = res;
            d->color_capture_frame = d->frame_index;
          }
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
            // Frame-pairing stamp: which frame this depth was captured on + the
            // resource identity. The velocity reprojection unprojects this depth
            // with curr VP and reprojects with prev VP — if the depth is from a
            // different frame than those matrices, the MV error scales with the
            // per-frame displacement (bad at 30 FPS, invisible at 170 FPS).
            d->depth_capture_frame = d->frame_index;
            d->depth_capture_res_handle = res.handle;
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
          if (shader_injection.dlaa_enabled > 1.5f && !d->scene_cbv_copy_issued) {
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
  // DLAAPhaseJitterColor OFF: skip the source patch for ALL buffers (color
  // renders unjittered — baseline). The VP capture above still ran, so the
  // velocity compute keeps the game's native matrices.
  if (g_phase_jitter_color < 0.5f) return false;
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
  // Compute the jitter VALUE (NDC) this frame should use.
  float rjx = 0.f, rjy = 0.f;
  if (shader_injection.dlaa_jitter_test > 0.5f) {
    // Jitter Test: apply a large FIXED 8px horizontal projection shift so the
    // rasterization-level jitter is plainly visible.
    rjx = 8.f * 2.f / d->viewport_w; rjy = 0.f;
  } else if (g_phase_freeze_jitter > 0.5f) {
    // Freeze: CONSTANT sub-pixel jitter every frame (Halton disabled).
    rjx = 0.25f * 2.f / d->viewport_w;
    rjy = -0.25f * 2.f / d->viewport_h;
  } else if (shader_injection.dlaa_jitter_enabled < 0.5f) {
    rjx = 0.f; rjy = 0.f;
  } else {
    uint32_t f = d->frame_index;
    rjx = (Halton(f + 1u, 2u) - 0.5f) * 2.f / d->viewport_w;
    rjy = (Halton(f + 1u, 3u) - 0.5f) * 2.f / d->viewport_h;
  }
  // The frozen sub-pixel value used by the freeze diagnostics (matches
  // DLAAPhaseFreezeJitter's constant).
  const float frjx = 0.25f * 2.f / d->viewport_w;
  const float frjy = -0.25f * 2.f / d->viewport_h;
  // Render vs report jitter, decoupled by the A/B toggles. Default: both = the
  // computed value. DLAAPhaseFreezeReport freezes only the report; DLAAPhase-
  // FreezeRender freezes only the render.
  float render_jx = rjx, render_jy = rjy;
  float report_jx = rjx, report_jy = rjy;
  if (g_phase_freeze_report > 0.5f) { report_jx = frjx; report_jy = frjy; }
  if (g_phase_freeze_render > 0.5f) { render_jx = frjx; render_jy = frjy; }
  // DLAAPhaseJitterColor OFF: baseline — render UNJITTERED and report 0 jitter
  // (takes precedence over Report-Only, which is a color-jittered diagnostic).
  if (g_phase_jitter_color < 0.5f) {
    d->jitter_x = 0.f; d->jitter_y = 0.f;
    d->report_jitter_x = 0.f; d->report_jitter_y = 0.f;
  } else if (g_jitter_decouple > 0.5f) {
    // DIAGNOSTIC (DLAAPhaseReportOnly): render UNJITTERED but report the Halton
    // jitter to DLSS. If the output changes vs plain jitter-off, DLSS consumes
    // the report; if identical, DLSS ignores the reported offset entirely.
    d->jitter_x = 0.f; d->jitter_y = 0.f;
    d->report_jitter_x = report_jx; d->report_jitter_y = report_jy;
  } else {
    d->jitter_x = render_jx; d->jitter_y = render_jy;
    d->report_jitter_x = report_jx; d->report_jitter_y = report_jy;
  }
  // DLAAPhaseJitterInMV: the jitter DELTA is baked into the MVs (velocity shader
  // params3.w), so DLSS must NOT also apply the reported offset — that would
  // double-count the jitter (the original JitterInMV fullscreen-shake failure).
  // Force the report to 0: the correct combo is delta-in-MV + report 0.
  if (g_jitter_in_mv > 0.5f) {
    d->report_jitter_x = 0.f; d->report_jitter_y = 0.f;
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
// Push constants (b13, 52 floats = 13 float4s) matching motion_velocity.cs_5_0.hlsl:
//   c[0..15] = prevViewProj, c[16..31] = curViewProjInv,
//   c[32..35] = params0 (vp_w, vp_h, velocity_scale, debug_view),
//   c[36..39] = params1 (jitter_x, jitter_y, per_object_motion, zero_mv),
//   c[40..43] = params2 (mv_threshold, mv_direction, mv_mode, exclude_effects),
//   c[44..47] = params3 (mv_threshold_object, prev_jitter_x, prev_jitter_y, jitter_in_mv),
//   c[48..51] = params4 (depth_sample_unjit, 0, 0, 0)
static std::array<float, 52> BuildVelocityPC(DeviceData* d) {
  std::array<float, 52> c = {};
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
  // params2.z = MV jitter mode:
  //   0 = MVJittered=0 + no per-object subtraction (Test B)
  //   1 = MVJittered=0 + subtract per-object jitter (Test A, current)
  //   2 = MVJittered=1 + no subtract; camera path adds jitter (Test C)
  c[42] = (g_phase_mv_jittered > 0.5f) ? 2.f : g_phase_mv_comp;
  c[43] = shader_injection.dlaa_exclude_effects;  // params2.w — mask effects out of DLAA
  c[44] = g_phase_mv_threshold_object;            // params3.x — per-object/Prev-Bone MVs only
  c[45] = d->prev_jitter_x;                       // params3.y — previous frame's jitter X (NDC)
  c[46] = d->prev_jitter_y;                       // params3.z — previous frame's jitter Y (NDC)
  c[47] = g_jitter_in_mv;                         // params3.w — bake jitter delta into MVs (diag)
  c[48] = g_phase_depth_sample_unjit;              // params4.x — sample depth at unjittered pixel (diag)
  c[49] = g_phase_synth_jitter_mv;                 // params4.y — synthetic jitter MV test (diag)
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
  // 3-way toggle: 0=Off, 1=FXAA (game luma FXAA, no DLSS), 2=DLAA.
  senkiseki3::dlss::dlss_enabled = (shader_injection.dlaa_enabled > 1.5f) ? 1.f : 0.f;
  senkiseki3::dlss::dlss_render_preset = shader_injection.dlaa_preset;
  // MVJittered A/B/C (DLAAPhaseMVJittered): off = MVs are jitter-subtracted in
  // the velocity shader (MVJittered=0, Tests A/B); on = MVs are fed JITTERED and
  // DLSS removes the jitter internally (MVJittered=1, Test C).
  senkiseki3::dlss::dlss_motion_vectors_jittered = (g_phase_mv_jittered > 0.5f) ? 1.f : 0.f;
  senkiseki3::dlss::dlss_debug_logging = shader_injection.dlaa_debug_logging;
  senkiseki3::dlss::dlss_flag_is_hdr = shader_injection.dlaa_flag_is_hdr;
  senkiseki3::dlss::dlss_flag_depth_inverted = shader_injection.dlaa_flag_depth_inverted;
  senkiseki3::dlss::dlss_flag_auto_exposure = shader_injection.dlaa_flag_auto_exposure;
  // DLSS output format: r16g16b16a16_float when the toggle is on, so the HDR
  // mod's final_blending tone maps UNCLAMPED values (8-bit UNORM clamps
  // highlights before the tone map -> clipping/banding).
  senkiseki3::dlss::dlss_output_format = (shader_injection.dlaa_hdr_float_out > 0.5f)
      ? DXGI_FORMAT_R16G16B16A16_FLOAT : DXGI_FORMAT_R8G8B8A8_UNORM;

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
      // HDR decode path (Phase 2): when DLAAHdrDecode is on, this holds the
      // re-encoded DLSS output SRV to bind at PS t0 (instead of raw NGX output).
      ID3D11ShaderResourceView* hdr_t0_srv = nullptr;

      // ── Phase 3 fix: robust DLSS color source ──
      // The game's PS t0 at this FXAA draw IS the composite. Read it live so we
      // are immune to the event capture's failure modes under the HDR mod
      // (hash-0 "Pipeline not found" skip, or our own NGX-output t0 bind
      // re-firing the capture hook and poisoning captured_color).
      if (d->live_color_srv) { d->live_color_srv->Release(); d->live_color_srv = nullptr; }
      if (d->live_color_res) { d->live_color_res->Release(); d->live_color_res = nullptr; }
      ID3D11ShaderResourceView* live_t0 = nullptr;
      cl->PSGetShaderResources(0, 1, &live_t0);
      if (live_t0) {
        ID3D11Resource* live_res = nullptr;
        live_t0->GetResource(&live_res);
        if (live_res) {
          D3D11_TEXTURE2D_DESC td = {};
          static_cast<ID3D11Texture2D*>(live_res)->GetDesc(&td);
          bool is_ngx_out = senkiseki3::dlss::ngx.output_texture &&
              live_res == senkiseki3::dlss::ngx.output_texture.Get();
          if (td.Width == w && td.Height == h && !is_ngx_out) {
            d->live_color_srv = live_t0; d->live_color_srv->AddRef();
            d->live_color_res = live_res; d->live_color_res->AddRef();
          }
          live_res->Release();
        }
        live_t0->Release();
      }
      // DIAG (Phase 3): confirm the actual color source handed to DLSS — live vs
      // event capture, content luma, capture staleness, and the FXAA draw's
      // resolved PS hash (0 => "Pipeline not found" capture skip).
      if (shader_injection.dlaa_debug_logging > 0.5f) {
        static int color_src_log = 0;
        if (++color_src_log % 60 == 0) {
          ID3D11Resource* diag_res = d->live_color_res
              ? d->live_color_res
              : reinterpret_cast<ID3D11Resource*>(d->captured_color_res.handle);
          LogColorSource(cmd_list, d, diag_res, d->live_color_res != nullptr);
        }
      }

      // Pass: Velocity compute
      cmd_list->bind_pipeline(AC, d->velocity_pipeline);

      // Motion-vector source for the velocity compute. The shader only samples
      // t1 (g_srcMotion) when per_object_motion is on (params1.z > 0.5); in
      // camera-only mode the texture is NEVER read, but the dispatch still
      // requires a NON-NULL descriptor. ALWAYS bind the dedicated 32-bit target
      // (created by EnsureMotionTarget above) and only fall back to the legacy
      // game-MRT2 capture if that ever fails.
      //
      // NOTE: do NOT select the legacy capture in camera mode. It was populated
      // from the original char G-buffer PS hash 0x0E8BC215, which Phase E's
      // generic PS patcher now modifies (changing its hash) — so the capture
      // gate never fires, captured_motion_srv stays null, and the dispatch was
      // skipped -> DLAA never ran in camera-only mode (no overlay).
      reshade::api::resource_view motion_src = d->motion_srv;
      if (!motion_src.handle) motion_src = d->captured_motion_srv;
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
      cmd_list->push_constants(CS, d->velocity_layout, 6, 0, 52, pc.data());
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
          D3D11_TEXTURE2D_DESC ngx_dvd_td = {};
          senkiseki3::dlss::ngx.output_texture->GetDesc(&ngx_dvd_td);
          D3D11_SHADER_RESOURCE_VIEW_DESC dvd = {};
          dvd.Format = ngx_dvd_td.Format;
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
          // screen Y, so the Y offset is negated here. Uses report_jitter (== render
          // jitter unless DLAAPhaseReportOnly decouples them).
          float jitter_px_x = d->report_jitter_x * d->viewport_w * 0.5f;
          float jitter_px_y = -d->report_jitter_y * d->viewport_h * 0.5f;
          // A/B the NGX jitter sign convention (NVIDIA ships a debug hotkey for this):
          // 0 = FlipBoth, 1 = FlipX, 2 = FlipY, 3 = Current.
          const int jsign = (int)shader_injection.dlaa_jitter_sign;
          if (jsign != 3) {
            if (jsign == 0 || jsign == 1) jitter_px_x = -jitter_px_x;
            if (jsign == 0 || jsign == 2) jitter_px_y = -jitter_px_y;
          }
          // DIAGNOSTIC (DLAAJitterScale): scale the reported NGX jitter magnitude
          // without touching the rendered jitter. If the 30 FPS static jitter
          // converges at 0.5x or 2x, the reported magnitude doesn't match the applied.
          // 0 = report 0 jitter (frames ARE jittered, DLSS told none): if visually
          // IDENTICAL to 1x, DLSS is ignoring the reported jitter entirely.
          {
            float jscale = 1.0f;
            if (g_jitter_scale < 0.5f) jscale = 0.0f;        // report 0
            else if (g_jitter_scale < 1.5f) jscale = 0.5f;   // 0.5x
            else if (g_jitter_scale < 2.5f) jscale = 1.0f;   // 1x (exact)
            else jscale = 2.0f;                              // 2x
            jitter_px_x *= jscale;
            jitter_px_y *= jscale;
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
            char buf[256];
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
              "[DLAA] f=%u px=(%.3f,%.3f) jpx=(%.3f,%.3f) s=%.0f reset=%d feature=%s color=%ux%u fmt=%d vpd=%.4f po=%d motion=%ux%u fmt=%d depth=%ux%u fmt=%d src=0x%08X dcf=%u vpf=%u pvf=%u dh=0x%llX cbv=0x%llX",
              d->frame_index,
              d->jitter_x * d->viewport_w * 0.5f, -d->jitter_y * d->viewport_h * 0.5f,
              jitter_px_x, jitter_px_y, g_jitter_scale,
              senkiseki3::dlss::ngx.reset ? 1 : 0,
              senkiseki3::dlss::ngx.feature ? "yes" : "no",
              cw, ch, cfmt, vp_delta,
              (int)shader_injection.dlaa_per_object_motion, mw, mh, mfmt,
              dw, dh, dfmt, d->depth_source_hash,
              d->depth_capture_frame, d->vp_read_frame, d->prev_vp_frame,
              (unsigned long long)d->depth_capture_res_handle,
              (unsigned long long)d->vp_cbv_handle);
            reshade::log::message(reshade::log::level::info, buf);
          }
          // DIAGNOSTIC (DLAAMVScale): A/B ONLY the DLSS MV_Scale parameter.
          // The SDK header says InMVScale converts the velocity texture TO pixel
          // space, so 1.0 assumes the texture is already pixel-space; 1/W,1/H
          // assumes 1.0 = full screen; 2/W,2/H assumes NDC (1.0 = half screen).
          // The MV shader's values are NOT modified, so this is a clean A/B of
          // the DLSS scale convention alone.
          float mv_scale_x = 1.f, mv_scale_y = 1.f;
          switch ((int)g_mv_scale_mode) {
            case 1: mv_scale_x = 1.f / (float)w; mv_scale_y = 1.f / (float)h; break;
            case 2: mv_scale_x = 2.f / (float)w; mv_scale_y = 2.f / (float)h; break;
            default: break;  // 1.0 (pixel-space, current)
          }
          // HDR-mod compatibility (Phase 2): DLSS always expects LINEAR input
          // regardless of the IsHDR flag (IsHDR only tweaks exposure/tonemap).
          // With the senkiseki HDR mod the composite is sRGB-encoded, so DLSS
          // sees gamma-compressed values as linear -> dark output that the HDR
          // swapchain proxy (sRGB decode -> PQ) amplifies to pure black.
          // DLAAHdrDecode: 0=Off (unchanged), 1=sRGB, 2=PQ — decodes the input
          // to a linear scratch for DLSS and re-encodes the output for t0.
          // ── Phase 3 fix: use the LIVE game t0 (composite) as the DLSS color
          // source. The event capture can fail/poison under the HDR mod (hash-0
          // "Pipeline not found" skip, or our own NGX-output t0 bind re-firing
          // the capture hook), so prefer what the game ACTUALLY bound here.
          ID3D11Resource* color_src_res = d->live_color_res
              ? d->live_color_res
              : reinterpret_cast<ID3D11Resource*>(d->captured_color_res.handle);
          ID3D11ShaderResourceView* color_src_srv = d->live_color_srv
              ? d->live_color_srv
              : reinterpret_cast<ID3D11ShaderResourceView*>(d->captured_color_srv.handle);
          const int hdr_decode_mode = (int)shader_injection.dlaa_hdr_decode;
          if (hdr_decode_mode >= 1 && hdr_decode_mode <= 2) {
            if (EnsureHdrConversion(dev, d, w, h) && color_src_srv) {
              int dec = (hdr_decode_mode == 1) ? 0 : 2;  // sRGB decode / PQ decode
              if (RunHdrConvertPass(cl, d, dec, color_src_srv, d->linear_scratch_uav, w, h)) {
                src = reinterpret_cast<ID3D11Resource*>(d->linear_scratch);
              }
            }
          } else {
            src = color_src_res;
          }
          dlaa_ok = senkiseki3::dlss::EvaluateDLSS(cl, src, ngx_out, mv, dep,
                       jitter_px_x, jitter_px_y, mv_scale_x, mv_scale_y);
          // The DLSS output texture may have been recreated by the eval (e.g.
          // the float-output toggle changed its format) — refresh the pointer.
          ngx_out = reinterpret_cast<ID3D11Resource*>(senkiseki3::dlss::ngx.output_texture.Get());
          if (dlaa_ok) {
            ++d->evals_this_frame;  // diag: count evals/present (must be 1)
            // Re-encode the DLSS linear output into the HDR chain's encoding
            // (mode 1 = sRGB, 3 = PQ) and bind THAT at PS t0.
            if (hdr_decode_mode >= 1 && hdr_decode_mode <= 2 && d->encoded_scratch_uav) {
              int enc = (hdr_decode_mode == 1) ? 1 : 3;
              ID3D11ShaderResourceView* ngx_srv = nullptr;
              D3D11_SHADER_RESOURCE_VIEW_DESC nsv = {};
              D3D11_TEXTURE2D_DESC ntd = {};
              senkiseki3::dlss::ngx.output_texture->GetDesc(&ntd);
              nsv.Format = ntd.Format;
              nsv.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
              nsv.Texture2D.MipLevels = 1;
              if (SUCCEEDED(senkiseki3::dlss::ngx.device->CreateShaderResourceView(ngx_out, &nsv, &ngx_srv))) {
                if (RunHdrConvertPass(cl, d, enc, ngx_srv, d->encoded_scratch_uav, w, h)) {
                  hdr_t0_srv = d->encoded_scratch_srv;
                }
                ngx_srv->Release();
              }
            }
            // DIAGNOSTIC: NGX output luma (pure 0 vs dark content) ~1 Hz.
            static int luma_log_count = 0;
            if (shader_injection.dlaa_debug_logging > 0.5f && (++luma_log_count % 60 == 0)) {
              LogNgxOutputLuma(cl, d);
            }
          }
        }
      }
      // Replace t0 with DLAA output SRV (falcomengine-plus pattern).
      // In MV debug mode the velocity SRV is already bound to t0 above.
      if (dlaa_ok && shader_injection.dlaa_debug_view <= 0.5f) {
        // HDR decode path: bind the re-encoded scratch (already holds the DLSS
        // output in the HDR chain's sRGB/PQ encoding). Otherwise bind the raw
        // NGX output texture (non-HDR / DLAAHdrDecode=Off, unchanged behavior).
        ID3D11ShaderResourceView* dlaa_srv = hdr_t0_srv;
        bool release_srv = false;
        if (dlaa_srv == nullptr) {
          D3D11_TEXTURE2D_DESC ngx_desc;
          senkiseki3::dlss::ngx.output_texture->GetDesc(&ngx_desc);
          static int fmt_log = 0;
          if (shader_injection.dlaa_debug_logging > 0.5f && ++fmt_log <= 2) {
            char buf[96];
            snprintf(buf, sizeof(buf), "[DLAA] NGX output: %ux%u fmt=%d",
                     ngx_desc.Width, ngx_desc.Height, (int)ngx_desc.Format);
            reshade::log::message(reshade::log::level::info, buf);
          }
          D3D11_SHADER_RESOURCE_VIEW_DESC srvd = {};
          srvd.Format = ngx_desc.Format;
          srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
          srvd.Texture2D.MipLevels = 1;
          senkiseki3::dlss::ngx.device->CreateShaderResourceView(
              senkiseki3::dlss::ngx.output_texture.Get(), &srvd, &dlaa_srv);
          release_srv = (dlaa_srv != nullptr);
        }
        if (dlaa_srv) {
          cl->PSSetShaderResources(0, 1, &dlaa_srv);
          if (release_srv) dlaa_srv->Release();
        } else {
          LogThrottled("ngx-srv-fail", reshade::log::level::warning, 1u, 120u,
                       "[DLAA] Failed to create SRV for NGX output");
        }
      }
    }
  }

  // Keep a previous-frame color copy for the reprojection debug mode.
  if (shader_injection.dlaa_debug_view >= 1.f && (d->live_color_res || d->captured_color_res.handle)) {
    auto* cl2 = reinterpret_cast<ID3D11DeviceContext*>(cmd_list->get_native());
    if (cl2 && d->prev_color_texture.handle) {
      auto* prev_native = reinterpret_cast<ID3D11Resource*>(d->prev_color_texture.handle);
      auto* cur_native = d->live_color_res
          ? d->live_color_res
          : reinterpret_cast<ID3D11Resource*>(d->captured_color_res.handle);
      cl2->CopyResource(prev_native, cur_native);
    }
  }

  d->frame_index++;
  return dlaa_ok;  // true = DLSS/debug output was bound at t0 on this draw
}

// ── Settings ──
renodx::utils::settings::Settings settings = {
    new renodx::utils::settings::Setting{
        .key = "DLAAEnabled", .binding = &shader_injection.dlaa_enabled,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 1.f, .label = "Anti-Aliasing", .section = "Antialiasing",
        .tooltip = "Off: no anti-aliasing (FXAA replaced by a passthrough). FXAA: the game's original luma FXAA. DLAA: NVIDIA DLAA (requires nvngx_dlss.dll).", .labels = {"Off", "FXAA", "DLAA"},
    },
    new renodx::utils::settings::Setting{
        .key = "DLAAPreset", .binding = &shader_injection.dlaa_preset,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f, .label = "DLSS Preset", .section = "Antialiasing",
        .labels = {"Default","F-CNN","J-T1","K-T1","L-T2","M-T2"},
        .is_enabled = []{ return shader_injection.dlaa_enabled > 1.5f; },
    },
    new renodx::utils::settings::Setting{
        .key = "DLAAJitter", .binding = &shader_injection.dlaa_jitter_enabled,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f, .label = "Camera Jitter", .section = "Antialiasing",
        .labels = {"Off","On"}, .is_enabled = []{ return shader_injection.dlaa_enabled > 1.5f; },
    },
    new renodx::utils::settings::Setting{
        .key = "DLAAJitterSign", .binding = &shader_injection.dlaa_jitter_sign,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f, .label = "Jitter Sign", .section = "Antialiasing",
        .labels = {"FlipBoth","FlipX","FlipY","Current"},
        .is_enabled = []{ return shader_injection.dlaa_enabled > 1.5f; },
    },
    new renodx::utils::settings::Setting{
        .key = "DLAAJitterScale", .binding = &g_jitter_scale,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 2.f, .label = "Jitter Report (diag)", .section = "Antialiasing",
        .tooltip = "DIAGNOSTIC: scales the NGX jitter offset reported to DLSS WITHOUT changing the rendered jitter. 'None' reports 0 (frames ARE still jittered) — if that looks identical to 1x, DLSS ignores the reported jitter entirely. 0.5x/2x test the magnitude.",
        .labels = {"None (report 0)","0.5x","1x (exact)","2x"},
        .is_enabled = []{ return shader_injection.dlaa_enabled > 1.5f; },
    },
    new renodx::utils::settings::Setting{
        .key = "DLAAPhaseJitterInMV", .binding = &g_jitter_in_mv,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f, .label = "Phase: Jitter In MV (diag)", .section = "Antialiasing",
        .tooltip = "ROOT-CAUSE FIX (diagnostic): the per-frame rendered jitter is NOT being compensated via the reported offset (Freeze Report = no change), so it reads as sub-pixel motion -> shimmer, worst at low FPS. The velocity shader bakes the jitter DELTA (current - previous frame) into the MVs, and the reported jitter is forced to 0 so DLSS does NOT double-count. This routes the jitter through the MVs, which DLSS demonstrably consumes. Use with Jitter Color = On, Jitter Depth = Off, Report = 1x.",
        .labels = {"Off","On"},
        .is_enabled = []{ return shader_injection.dlaa_enabled > 1.5f; },
    },
    new renodx::utils::settings::Setting{
        .key = "DLAAPhaseReportOnly", .binding = &g_jitter_decouple,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f, .label = "Phase: Report-Only Jitter (diag)", .section = "Antialiasing",
        .tooltip = "DEFINITIVE A/B of whether DLSS consumes the reported jitter offset: renders the frame UNJITTERED but still reports the Halton jitter to DLSS. If the image changes vs plain jitter-off, DLSS uses the offset; if it looks identical, DLSS ignores the reported offset entirely. Log shows px=0 (rendered) vs jpx!=0 (reported).",
        .labels = {"Off","On"},
        .is_enabled = []{ return shader_injection.dlaa_enabled > 1.5f; },
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
        .is_enabled = []{ return shader_injection.dlaa_enabled > 1.5f; },
    },
    new renodx::utils::settings::Setting{
        .key = "DLAAZeroMV", .binding = &shader_injection.dlaa_zero_mv,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f, .label = "Zero Motion Vectors", .section = "Antialiasing",
        .tooltip = "Forces all motion vectors to 0. A/B: if image looks similar, MVs aren't helping; if worse, MVs contribute useful info.",
        .labels = {"Off","On"},
        .is_enabled = []{ return shader_injection.dlaa_enabled > 1.5f; },
    },
    new renodx::utils::settings::Setting{
        .key = "DLAAMVDirection", .binding = &shader_injection.dlaa_mv_direction,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f, .label = "MV Direction Flip", .section = "Antialiasing",
        .tooltip = "Flips motion vector sign (previous-current instead of current-previous). A/B: if flipping fixes ghosting, the convention was wrong.",
        .labels = {"Off","Flip"},
        .is_enabled = []{ return shader_injection.dlaa_enabled > 1.5f; },
    },
    new renodx::utils::settings::Setting{
        .key = "DLAAMVThreshold", .binding = &shader_injection.dlaa_mv_threshold,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 0.f, .label = "MV Threshold (px)", .section = "Antialiasing",
        .tooltip = "Zeros CAMERA motion vectors below this magnitude. Kills static sub-pixel MV noise that poisons history. Per-object / Prev-Bone MVs have their own threshold: DLAAPerObjectMVThreshold.",
        .min = 0.f, .max = 5.f, .format = "%.2f",
        .is_enabled = []{ return shader_injection.dlaa_enabled > 1.5f; },
    },
    new renodx::utils::settings::Setting{
        .key = "DLAAPerObjectMVThreshold", .binding = &g_phase_mv_threshold_object,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 0.f, .label = "Per-Object MV Threshold (px)", .section = "Antialiasing",
        .tooltip = "Zeros PER-OBJECT / Prev-Bone motion vectors below this magnitude (dynamic objects: character limbs, hair, etc.). The camera path keeps its own DLAAMVThreshold. Leave 0 to keep all per-object MVs (including small noise).",
        .min = 0.f, .max = 5.f, .format = "%.2f",
        .is_enabled = []{ return shader_injection.dlaa_enabled > 1.5f; },
    },
    new renodx::utils::settings::Setting{
        .key = "DLAAVelocityFormat", .binding = &shader_injection.dlaa_velocity_format,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f, .label = "32-bit Motion Vectors", .section = "Antialiasing",
        .tooltip = "MV texture precision A/B: r16g16_float (16-bit, default) vs r32g32_float (32-bit). 32-bit costs a little bandwidth; 16-bit is already exact for pixel-space MVs up to 2048px.",
        .labels = {"16-bit (r16g16)","32-bit (r32g32)"},
        .is_enabled = []{ return shader_injection.dlaa_enabled > 1.5f; },
    },
    new renodx::utils::settings::Setting{
        .key = "DLAAExcludeEffects", .binding = &shader_injection.dlaa_exclude_effects,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f, .label = "Exclude Effects from DLAA", .section = "Antialiasing",
        .tooltip = "Masks particles/effects out of DLAA: they get an off-screen motion vector so DLSS falls back to the current frame (no temporal shimmer/ghosting).",
        .labels = {"Off","On"},
        .is_enabled = []{ return shader_injection.dlaa_enabled > 1.5f; },
    },
    new renodx::utils::settings::Setting{
        .key = "DLAADepthSource", .binding = &shader_injection.dlaa_depth_source,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f, .label = "Depth Source", .section = "Antialiasing",
        .tooltip = "Which pass's depth DLAA uses. Auto = last full-res depth push (may be wrong). Pick the one that shows a real depth map in MV Debug=Depth. See scan log.",
        .labels = {"Auto","0x0E83E74E","0x55D61207","0x322E20D4"},
        .is_enabled = []{ return shader_injection.dlaa_enabled > 1.5f; },
    },
    new renodx::utils::settings::Setting{
        .key = "DLAAForceReset", .binding = &shader_injection.dlaa_force_reset,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f, .label = "Force Reset (No History)", .section = "Antialiasing",
        .tooltip = "Forces NGX InReset=1 every frame (no temporal accumulation). A/B test: if identical to Off, history is already disabled.",
        .labels = {"Off","On"},
        .is_enabled = []{ return shader_injection.dlaa_enabled > 1.5f; },
    },
    new renodx::utils::settings::Setting{
        .key = "DLAAPerObjectMotion", .binding = &shader_injection.dlaa_per_object_motion,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f, .label = "Per-Object Motion", .section = "Antialiasing",
        .tooltip = "Camera Only: camera reprojection MVs only. Per-Object: patched char VSs emit prevClip (current-pose re-skin -> camera-relative MVs). Prev-Bone: patched char VSs re-skin with the PREVIOUS frame's per-character bone matrices -> true limb/animation MVs. Requires Phase B bind mode (NoBind/Minimal off) and a restart.",
        .labels = {"Camera Only","Per-Object","Prev-Bone"},
        .is_enabled = []{ return shader_injection.dlaa_enabled > 1.5f; },
    },
    new renodx::utils::settings::Setting{
        .key = "DLAAVelocityScale", .binding = &shader_injection.dlaa_velocity_scale,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 1.f, .label = "Velocity Scale", .section = "Antialiasing",
        .min = 0.1f, .max = 5.f, .format = "%.2f",
        .is_enabled = []{ return shader_injection.dlaa_enabled > 1.5f; },
    },
    new renodx::utils::settings::Setting{
        .key = "DLAAMVScale", .binding = &g_mv_scale_mode,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f, .label = "MV Scale (diag)", .section = "Antialiasing",
        .tooltip = "A/B: the DLSS InMVScale parameter (converts the velocity texture to pixel space) WITHOUT changing the MV shader. 1.0 = MVs already pixel-space (SDK default). 1/W,1/H = normalized (1.0 = full screen). 2/W,2/H = NDC (1.0 = half screen). If the 30 FPS jitter converges at one of these, the MV scale convention was the bug.",
        .labels = {"1.0 (px)","1/W, 1/H","2/W, 2/H"},
        .is_enabled = []{ return shader_injection.dlaa_enabled > 1.5f; },
    },
    new renodx::utils::settings::Setting{
        .key = "DLAAPhaseDepthSampleUnjit", .binding = &g_phase_depth_sample_unjit,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f, .label = "Phase: Sample Depth Unjittered (diag)", .section = "Antialiasing",
        .tooltip = "A/B: sample the velocity camera path's depth at the UNJITTERED pixel (curPxU) instead of the jittered pixel (pix). With an unjittered depth buffer, depth[pix] belongs to content at unjittered position pix, not at curPxU = pix - jitterPx — sampling at pix misreads the depth by up to half a pixel whenever jitter is on, injecting a per-frame MV error DLSS can't compensate (shimmer, worst at 30 FPS). On = sample at curPxU.",
        .labels = {"Off (depth[pix])","On (depth[curPxU])"},
        .is_enabled = []{ return shader_injection.dlaa_enabled > 1.5f; },
    },
    new renodx::utils::settings::Setting{
        .key = "DLAAPhaseSynthMV", .binding = &g_phase_synth_jitter_mv,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f, .label = "Phase: Synthetic Jitter MV (diag)", .section = "Antialiasing",
        .tooltip = "ESTABLISH THE SIGN: bypasses depth reprojection and writes an ANALYTIC global MV (no DLSS, no depth, no matrices) so the reproj debug view proves the correct jitter-delta MV + sign. At 30 FPS static + jitter ON, whichever of Jcur-Jprev / Jprev-Jcur makes the reproj view BLACK is the sign DLSS needs (then feed it via JitterInMV + report 0).",
        .labels = {"Off","Zero (Test A)","Jcur-Jprev","Jprev-Jcur"},
        .is_enabled = []{ return shader_injection.dlaa_enabled > 1.5f; },
    },
    new renodx::utils::settings::Setting{
        .key = "DLAADebugView", .binding = &shader_injection.dlaa_debug_view,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f, .label = "MV Debug", .section = "Antialiasing",
        .labels = {"Off","HSV","Arrows","Magnitude","Reproj","Depth"},
        .is_enabled = []{ return shader_injection.dlaa_enabled > 1.5f; },
    },
    new renodx::utils::settings::Setting{
        .key = "DLAADebugScale", .binding = &shader_injection.dlaa_debug_scale,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 50.f, .label = "MV Debug Scale", .section = "Antialiasing",
        .min = 1.f, .max = 200.f, .format = "%.0f",
        .is_enabled = []{ return shader_injection.dlaa_enabled > 1.5f && shader_injection.dlaa_debug_view >= 1.f; },
    },
    new renodx::utils::settings::Setting{
        .key = "DLAAFlagIsHDR", .binding = &shader_injection.dlaa_flag_is_hdr,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f, .label = "Flag: HDR Input", .section = "Antialiasing",
        .labels = {"Off","On"}, .is_enabled = []{ return shader_injection.dlaa_enabled > 1.5f; },
    },
    new renodx::utils::settings::Setting{
        .key = "DLAAFlagDepthInverted", .binding = &shader_injection.dlaa_flag_depth_inverted,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f, .label = "Flag: Depth Inverted", .section = "Antialiasing",
        .labels = {"Off","On"}, .is_enabled = []{ return shader_injection.dlaa_enabled > 1.5f; },
    },
    new renodx::utils::settings::Setting{
        .key = "DLAAFlagAutoExposure", .binding = &shader_injection.dlaa_flag_auto_exposure,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f, .label = "Flag: Auto Exposure", .section = "Antialiasing",
        .labels = {"Off","On"}, .is_enabled = []{ return shader_injection.dlaa_enabled > 1.5f; },
    },
    new renodx::utils::settings::Setting{
        .key = "DLAAHdrDecode", .binding = &shader_injection.dlaa_hdr_decode,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f, .label = "HDR Decode (diag)", .section = "Antialiasing",
        .tooltip = "HDR-mod compatibility A/B (COMPOSITE path only): converts the DLSS input/output color encoding. Off = feed DLSS the composite as-is. sRGB = decode composite sRGB->linear before DLSS and re-encode the DLSS output linear->sRGB. PQ = same with ST.2084 PQ. The Pre-ToneMap inject path (DLAAHdrInject) does NOT use this.",
        .labels = {"Off","sRGB","PQ"},
        .is_enabled = []{ return shader_injection.dlaa_enabled > 1.5f; },
    },
    new renodx::utils::settings::Setting{
        .key = "DLAAHdrInject", .binding = &shader_injection.dlaa_hdr_inject,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f, .label = "HDR Pre-ToneMap Inject", .section = "Antialiasing",
        .tooltip = "Where DLAA runs the DLSS pass. Auto: runs at the final_blending draw (raw untonemapped scene) when the HDR mod (_renodx-senkiseki.addon64) is loaded, so the HDR mod tone maps the DLAA'd image itself. Pre-ToneMap: force that path. Composite: run DLSS on the tone-mapped composite at FXAA (old path — SDR-capped with the HDR mod).",
        .labels = {"Auto","Pre-ToneMap","Composite"},
        .is_enabled = []{ return shader_injection.dlaa_enabled > 1.5f; },
    },
    new renodx::utils::settings::Setting{
        .key = "DLAAHdrFloatOut", .binding = &shader_injection.dlaa_hdr_float_out,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f, .label = "HDR: Float DLSS Output", .section = "Antialiasing",
        .tooltip = "Makes the DLSS output texture r16g16b16a16_float instead of r8g8b8a8. The 8-bit UNORM output clamps highlight values before the HDR mod's tone map can recover them (clipping/banding on the Pre-ToneMap path). Float preserves the range through the tone map. Recreates the DLSS feature once on toggle.",
        .labels = {"Off (8-bit)","On (r16 float)"},
        .is_enabled = []{ return shader_injection.dlaa_enabled > 1.5f; },
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
        .key = "DLAAPhaseBJitterOnly", .binding = &g_phaseb_jitter_only,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f, .label = "Phase B Jitter-Only", .section = "Antialiasing",
        .tooltip = "CAMERA PATH (default ON): the generic VS patch injects ONLY the SV_Position camera jitter (b13) — no prevClip output, no prev-bone re-skin, no prevVP, no outline, no OSGN append. Under the GLOBAL jitter method the b13 offset is 0, so this is an inert safety patch. Turn OFF to restore the per-object motion machinery. Requires restart.",
        .labels = {"Off","On"},
    },
    new renodx::utils::settings::Setting{
        .key = "DLAAPhaseBOutline", .binding = &g_phaseb_outline,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f, .label = "Phase B Outline Offset", .section = "Antialiasing",
        .tooltip = "Replicate the GameEdgeParameters outline offset in the patched prevClip. Now gated on ACTUAL shader usage (most char VSs declare it but never read it, so this is normally a no-op) — only genuinely-outlined shaders get the offset.",
        .labels = {"Off","On"},
    },
    // ── Shake-isolation diagnostics ──
    new renodx::utils::settings::Setting{
        .key = "DLAAPhaseFreezeJitter", .binding = &g_phase_freeze_jitter,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f, .label = "Phase: Freeze Jitter", .section = "Antialiasing",
        .tooltip = "DIAGNOSTIC: replace the Halton sequence with a CONSTANT sub-pixel jitter (+0.25/-0.25px) every frame. If the Prev-Bone shake disappears with a fixed jitter, the error is the frame-to-frame jitter VARIATION (compensation/timing mismatch), not the MVs themselves.",
        .labels = {"Off","On"},
    },
    new renodx::utils::settings::Setting{
        .key = "DLAAPhaseFreezeReport", .binding = &g_phase_freeze_report,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f, .label = "Phase: Freeze Report (diag)", .section = "Antialiasing",
        .tooltip = "A/B: render the NORMAL (Halton) jitter but report a CONSTANT jitter to DLSS. If the shake DISAPPEARS, DLSS was reacting to the VARYING report (so the offset we feed is wrong); if it STILL shakes, DLSS ignores the report and the RENDERED variation is the culprit.",
        .labels = {"Off","On"},
    },
    new renodx::utils::settings::Setting{
        .key = "DLAAPhaseFreezeRender", .binding = &g_phase_freeze_render,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f, .label = "Phase: Freeze Render (diag)", .section = "Antialiasing",
        .tooltip = "A/B (mirror of Freeze Report): render a CONSTANT jitter but report the normal (Halton) jitter. If it SHAKES, DLSS consumes the report; if it STAYS STABLE, DLSS ignores it.",
        .labels = {"Off","On"},
    },
    new renodx::utils::settings::Setting{
        .key = "DLAAPhaseMVComp", .binding = &g_phase_mv_comp,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f, .label = "Phase: MV Jitter Comp", .section = "Antialiasing",
        .tooltip = "DIAGNOSTIC: per-object jitter subtraction in the velocity shader A/B (Test A/B with MVJittered off). Off: if the character STOPS shaking, the character was never jittered (we were over-subtracting); if it still shakes, the character IS jittered and compensation isn't the cause.",
        .labels = {"Off","On"},
    },
    new renodx::utils::settings::Setting{
        .key = "DLAAPhaseMVJittered", .binding = &g_phase_mv_jittered,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f, .label = "Phase: MVJittered=1 (Test C)", .section = "Antialiasing",
        .tooltip = "DIAGNOSTIC (Test C): feed JITTERED MVs and set MVJittered=1 so DLSS removes the jitter internally (instead of subtracting in the shader). The camera path then ADDS the jitter to stay consistent. Use with Phase: MV Jitter Comp = Off.",
        .labels = {"Off","On"},
    },
    new renodx::utils::settings::Setting{
        .key = "DLAAPhaseJitterDepth", .binding = &g_phase_jitter_depth,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f, .label = "Phase: Jitter Depth (diag)", .section = "Antialiasing",
        .tooltip = "A/B: jitter the DEPTH pass too, so DLSS's color and depth are in the SAME jitter state (Test B). Off (default) = depth stays the game's native UNJITTERED depth (Test A). On = the world + character depth passes are jittered with the color. Only the shadow VS stays unjittered. Watch for the character shadow moving.",
        .labels = {"Off (Test A)","On (Test B)"},
    },
    new renodx::utils::settings::Setting{
        .key = "DLAAPhaseJitterColor", .binding = &g_phase_jitter_color,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f, .label = "Phase: Jitter Color (diag)", .section = "Antialiasing",
        .tooltip = "A/B: master color-jitter toggle. On (default) = color rendered with the jittered VP (normal DLAA). Off = color renders UNJITTERED and 0 jitter is reported to DLSS — the clean baseline (unjittered color + unjittered depth + report 0).",
        .labels = {"Off (Baseline)","On"},
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
        .key = "DLAAPhaseEConstant", .binding = &g_phasee_constant,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f, .label = "Phase E Constant Output", .section = "Antialiasing",
        .tooltip = "CRASH BISECTION: Phase E adds ONLY the o3 output register, written with a CONSTANT (0.5,0.5,0,1) — no new input decl, no SV_SampleIndex relocation, no vN read. If it still hangs, the mere appended output register faults; if not, the input/relocation/read matters. Requires restart.",
        .labels = {"Off","On"},
    },
    new renodx::utils::settings::Setting{
        .key = "DLAAPhaseENoRead", .binding = &g_phasee_no_read,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f, .label = "Phase E No Input Read", .section = "Antialiasing",
        .tooltip = "CRASH BISECTION: normal input decl + SV relocation, but the injected body writes a CONSTANT — never reads vN. Isolates the vN data path from the declaration/relocation machinery. Requires restart.",
        .labels = {"Off","On"},
    },
    new renodx::utils::settings::Setting{
        .key = "DLAAPhaseENoMath", .binding = &g_phasee_no_math,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f, .label = "Phase E No Math", .section = "Antialiasing",
        .tooltip = "CRASH BISECTION: normal input decl + SV relocation, body = mov o3, vN straight through (no max/div/mad). Isolates the divide/mad math from the read. Requires restart.",
        .labels = {"Off","On"},
    },
    new renodx::utils::settings::Setting{
        .key = "DLAAPhaseENoNewOutput", .binding = &g_phasee_no_new_output,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f, .label = "Phase E No New Output (o2.zw)", .section = "Antialiasing",
        .tooltip = "CRASH BISECTION: full input decl + SV relocation + full div/mad math, but prevNDC is written into the EXISTING max output's .zw (o2.zw for 3-RT) instead of appending a NEW SV_TARGET output — NO OSGN append, NO dcl_output. Every crashing test so far appended a new output; if this is safe, the appended output register was the trigger. Requires restart.",
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
static bool HdrFinalPathActive(reshade::api::command_list* cmd_list);  // defined below

static bool OnBeforeFxaaDraw(reshade::api::command_list* cmd_list) {
  if (shader_injection.dlaa_enabled < 1.5f) return true;  // DLAA only (mode 2)
  // HDR pre-tone-map path (DLAAHdrInject): DLSS already ran at the final_blending
  // draw, so the FXAA replacement just passthroughs the tone-mapped composite to
  // RTV0. Fallback: if no final_blending draw ran DLSS this frame (e.g. a final
  // variant we don't hook), run DLSS here on the composite as before.
  if (HdrFinalPathActive(cmd_list)) {
    auto* dev = cmd_list->get_device();
    auto* d = dev ? dev->get_private_data<DeviceData>() : nullptr;
    if (d && d->dlaa_ran_this_frame) return true;
  }
  RunDLAA(cmd_list);
  return true;  // Always let FXAA run — it composites our DLAA'd t0 to RTV0
}

// ── HDR pre-tone-map path (DLAAHdrInject) ──
// Runs DLAA at the FINAL_BLENDING draw instead of FXAA. The live PS t0 at that
// draw is the game's UNTONEMAPPED raw scene (final_blending's ColorBuffer) —
// DLSS processes it and RunDLAA rebinds t0 to the DLSS output, so the HDR mod's
// OWN final_blending (which runs right after this on_draw) tone maps the DLAA'd
// image. No renodx/HDR settings are read or replicated: if the HDR mod author
// changes the tone map or nits, we never know or care.
static bool HdrFinalPathActive(reshade::api::command_list* cmd_list) {
  const int mode = (int)shader_injection.dlaa_hdr_inject;
  if (mode == 1) return true;   // Force Pre-ToneMap
  if (mode == 2) return false;  // Force Composite
  // Auto: active when the HDR mod is loaded.
  auto* dev = cmd_list ? cmd_list->get_device() : nullptr;
  auto* d = dev ? dev->get_private_data<DeviceData>() : nullptr;
  return d && d->hdr_detected;
}

static bool OnBeforeFinalBlendingDraw(reshade::api::command_list* cmd_list) {
  if (shader_injection.dlaa_enabled < 1.5f) return true;  // DLAA only (mode 2)
  if (!HdrFinalPathActive(cmd_list)) return true;          // pre-tone-map path off
  auto* dev = cmd_list->get_device();
  auto* d = dev ? dev->get_private_data<DeviceData>() : nullptr;
  if (!d) return true;
  d->dlaa_ran_this_frame = RunDLAA(cmd_list);
  return true;  // never skip — the HDR mod's final_blending must tone-map t0
}

// All final_blending hashes the HDR mod replaces (CS4 + CS3 variants). DLSS runs
// at these draws so it sees the raw pre-tone-map scene, and the HDR mod's own
// final_blending (running after our on_draw) tone maps the DLSS output.
static constexpr std::array<uint32_t, 138> FINAL_BLENDING_HASHES = {
    // CS4 finals
    0x46A727D9u, 0x2BF0C94Bu, 0x3E08A3A6u, 0x2A8422DEu, 0x49107B8Fu, 0x1C7DCC30u,
    0x3541804Au, 0xFD245ECCu, 0x93DEF816u, 0x0469E6D6u, 0x16DA1605u, 0xC2D07E63u,
    0x1ABB15C9u, 0x228096A4u, 0x2005E90Cu, 0x2F2D2517u, 0x30064E3Du, 0x38B0F676u,
    0x46589838u, 0x4C17DC8Cu, 0x4D291F04u, 0x4E97BECAu, 0x4F6D55D7u, 0x5F75A7B5u,
    0x6417B671u, 0x575568A5u, 0x68F73C03u, 0x71DC9089u, 0x762A5B0Eu, 0x7DD4CB97u,
    0x7E245ADCu, 0x83446FEFu, 0x84648647u, 0x8589D7AFu, 0x8A4275AAu, 0x8F06D84Cu,
    0xA07D8921u, 0x94889944u, 0x9BF2734Cu, 0xA01BCC76u, 0xA20ECB4Bu, 0xA210474Cu,
    0xA4560071u, 0xA6A642ECu, 0xA716393Fu, 0xA7C2CED9u, 0xAC7DDDACu, 0xB1E0A90Du,
    0xB41DEC9Bu, 0xB573287Eu, 0xB59D548Au, 0xB5A7008Bu, 0xB936A99Bu, 0xBB0DEC2Au,
    0xBEE3AF35u, 0xCC00713Eu, 0xCF4D44BCu, 0xD01F775Cu, 0xD0D6FE14u, 0xDA5C467Fu,
    0xDBC91624u, 0xDECB5EF1u, 0xE1D7A2B6u, 0xE65E731Du, 0xEC63410Fu, 0xED6722DCu,
    0xED849BD1u, 0xF0566A0Fu, 0xFCBCF123u,
    // CS3 finals
    0x6B574B6Eu, 0xABF4E009u, 0xB4406452u, 0x95F02C1Du, 0x26217A30u, 0x46DC6C58u,
    0x5A5A4C5Au, 0x322E20D4u, 0x00C4E2A6u, 0x0161438Cu, 0x01E1FF0Fu, 0x02E4863Eu,
    0x0EBE33ECu, 0x11CAE0E6u, 0x1486065Du, 0x160A4895u, 0x167A12F5u, 0x1D403F97u,
    0x1EDECECCu, 0x215168ABu, 0x2594FB2Cu, 0x269DDBDFu, 0x281F2814u, 0x31E727B4u,
    0x3415A1BEu, 0x34EAE8B6u, 0x38BB9A49u, 0x3FB18938u, 0x513CF99Fu, 0x517BF7B2u,
    0x51DB8AC5u, 0x52DBC392u, 0x53B8577Cu, 0x5A1C72D5u, 0x5BBE9F57u, 0x67F0BD68u,
    0x6C550D4Fu, 0x78F1AC05u, 0x884D23A7u, 0x8923A287u, 0x8BDD1E5Du, 0x90180FAFu,
    0x9655B4B2u, 0x97EB039Cu, 0x9B0A1C60u, 0xA4945FF3u, 0xA4D9C6FDu, 0xA671C250u,
    0xA9F09088u, 0xABE9BEE1u, 0xAC84E828u, 0xACFCDFC2u, 0xB32BC083u, 0xB3BF88B3u,
    0xB78778F5u, 0xBA62ACD7u, 0xC2DFA434u, 0xCED8E8A8u, 0xCFF51135u, 0xD7BC302Fu,
    0xD8F91B51u, 0xE0E54773u, 0xE13AAD9Bu, 0xE2FC9C22u, 0xE52B3C27u, 0xE742F2C3u,
    0xEC64A31Au, 0xEFAC375Cu, 0xEFC9A329u,
};

// ── FXAA replacement (3-way AA toggle) ──
// The replacement shader (0x96BB8CFF) SELF-GATES on shader_injection.dlaa_enabled
// (b13): Off (0) → passthrough, FXAA (1) → RGB-luma FXAA 3.11, DLAA (2) →
// passthrough. No on_replace gate is needed — the shader always runs and picks
// its behavior from the toggle. OnBeforeFxaaDraw still runs DLAA first in DLAA
// mode so t0 holds the DLAA output for the passthrough to copy.
// Build the custom-shader table: the FXAA 3-way replacement + effect-mask +
// composite entries, PLUS every final_blending hash with an on_draw hook that
// runs DLSS on the raw pre-tone-map scene (HDR pre-tone-map path). The final
// entries carry NO replacement code, so the HDR mod's own final_blending
// replacement is used and tone maps the DLSS output.
static renodx::mods::shader::CustomShaders BuildCustomShaders() {
  renodx::mods::shader::CustomShaders cs = {
      {
          0x96BB8CFFu,
          renodx::mods::shader::CustomShader{
              .crc32 = 0x96BB8CFFu,
              .code = __0x96BB8CFF,
              .on_draw = OnBeforeFxaaDraw,
          },
      },
      // ── Phase E: per-object-motion PSs are NO LONGER hand-replaced ──
      // The 22 char G-buffer PSs (0xFEA2B509, 0x159A34A3, 0x1682CB9B, ...,
      // 0x055CFB63) were removed from custom_shaders: the generic DXBC patcher
      // (OnCreatePipeline -> PatchPerObjectPixelShader) now appends the
      // TEXCOORD5 input + SV_TARGET3 output to ANY G-buffer PS, writing
      // prevNDC+valid-flag to the appended 32-bit motion RTV. No hashes, no
      // per-shader HLSL — same as the VS patcher. (The old boot PS .hlsl files
      // remain on disk as reference but are no longer injected.)
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
  for (uint32_t h : FINAL_BLENDING_HASHES) {
    if (cs.contains(h)) continue;
    renodx::mods::shader::CustomShader s = {};
    s.crc32 = h;
    s.on_draw = OnBeforeFinalBlendingDraw;
    cs.emplace(h, s);
  }
  return cs;
}

renodx::mods::shader::CustomShaders custom_shaders = BuildCustomShaders();

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

// ── Hang watchdog: heartbeat thread ──
// Writes one line to watchdog.log every 250 ms while DLAAPhaseBDump is ON. The
// render thread keeps `watchdog_progress` stamped at every interesting spot
// (pipeline patch start/end, draw hook enter/done, present). When the game
// freezes the render thread stops advancing it, so the repeated tail of the
// heartbeat file shows exactly where it stopped.
static std::string WatchdogPath() {
  std::error_code ec;
  auto p = renodx::utils::path::GetOutputSubdirectory("dump") / "phaseb";
  std::filesystem::create_directories(p, ec);
  return (p / "watchdog.log").string();
}

static void WatchdogSet(DeviceData* d, const char* fmt, ...) {
  if (!d || !d->watchdog_running.load()) return;
  char buf[512];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  buf[sizeof(buf) - 1u] = '\0';
  std::lock_guard<std::mutex> lock(d->watchdog_mutex);
  strncpy_s(d->watchdog_progress, buf, _TRUNCATE);  // fixed copy — no allocation in lock scope
  d->watchdog_progress[sizeof(d->watchdog_progress) - 1u] = '\0';
}

static void WatchdogStampDraw(reshade::api::command_list* cmd_list, DeviceData* d, const char* tag) {
  if (!d || !d->watchdog_running.load()) return;
  auto* state = renodx::utils::shader::GetCurrentState(cmd_list);
  uint32_t vh = state ? renodx::utils::shader::GetCurrentVertexShaderHash(state) : 0u;
  uint32_t ph = state ? renodx::utils::shader::GetCurrentPixelShaderHash(state) : 0u;
  WatchdogSet(d, "draw %s vs=0x%08X ps=0x%08X", tag, vh, ph);
}

static void WatchdogStart(DeviceData* d) {
  if (!d || d->watchdog_running.exchange(true)) return;
  const std::string path = WatchdogPath();
  d->watchdog_thread = std::thread([d, path]() {
    while (d->watchdog_running.load()) {
      char prog[512];
      {
        std::lock_guard<std::mutex> lock(d->watchdog_mutex);
        strncpy_s(prog, d->watchdog_progress, _TRUNCATE);  // fixed copy — no allocation
        prog[sizeof(prog) - 1u] = '\0';
      }
      const uint64_t ping = d->watchdog_ping.fetch_add(1u) + 1u;
      if (g_phaseb_dump >= 0.5f) {
        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        FILE* f = nullptr;
        fopen_s(&f, path.c_str(), "a");
        if (f) {
          fprintf(f, "t=%lld ping=%llu %s\n", (long long)ms,
                  (unsigned long long)ping, prog);
          fclose(f);
        }
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }
  });
}

static void WatchdogStop(DeviceData* d) {
  if (!d) return;
  d->watchdog_running.store(false);
  if (d->watchdog_thread.joinable()) d->watchdog_thread.join();
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
  WatchdogSet(d, "create_pipeline n=%u", subobject_count);
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
    options.jitter_only = g_phaseb_jitter_only >= 0.5f;
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
             "[DLAA] phaseB: patched VS 0x%08X -> 0x%08X (%u B) cb%u t%u TEXCOORD%u o%u outline=%d noBind=%d minimal=%d noOutput=%d const=%d jitterOnly=%d",
             hash, new_hash, (uint32_t)blob.size(), info.prev_vp_cb_slot,
             info.prev_bone_t_slot, info.texcoord_index, info.output_reg,
             (int)info.outline_applied, (int)info.needs_no_binding,
             (int)options.minimal_patch, (int)options.test_no_output,
             (int)options.test_constant_output, (int)options.jitter_only);
    reshade::log::message(reshade::log::level::info, buf);
    WatchdogSet(d, "create_pipeline phaseB patched VS 0x%08X -> 0x%08X", hash, new_hash);
  }

  // ── Phase E: generic per-object-motion PS patcher ──
  // Runs on the pixel_shader subobject (create_pipeline fires once per stage;
  // the PS is a separate subobject from the VS). Appends a TEXCOORD input
  // (prevClip, at the SAME index the paired VS writes it) + an SV_TARGET
  // output (the appended motion RTV, at max_out+1) to any G-buffer PS (>=3
  // SV_TARGET outputs) and injects the prevNDC->o3 write before ret. The PS
  // needs NO draw-time binding: it reads prevClip via normal VS->PS semantic
  // linkage and writes the appended o3; MaybeAppendMotionRtv only binds the
  // motion RTV on char draws, so o3 is discarded elsewhere. Skip-guard:
  // hand-replaced shaders (effect masks, FXAA, composite) and already-patched
  // hashes.
  //
  // The PS prevClip input semantic must EXACTLY match the index Phase B chose
  // for the paired VS (D3D11 links by semantic name+index). If we picked the
  // PS's own first-free index, it could differ from the VS's (the game's VS
  // may output TEXCOORDs the PS doesn't consume, e.g. eye VS 0x215C68A3
  // outputs TEXCOORD5 while PS 0x0E03514A does not read it) -> the PS would
  // read the game's own TEXCOORD as prevClip -> garbage MVs -> ghosting. Look
  // up the VS subobject in this same create_pipeline call and reuse its index.
  uint32_t vs_texcoord_index = 0u;
  for (uint32_t i = 0; i < subobject_count; ++i) {
    const auto& sub = subobjects[i];
    if (sub.type != reshade::api::pipeline_subobject_type::vertex_shader) continue;
    if (sub.count != 1u) continue;
    auto* vsd = static_cast<reshade::api::shader_desc*>(sub.data);
    if (!vsd || vsd->code_size == 0u) continue;
    const uint32_t vhash = renodx::utils::hash::ComputeCRC32(
        static_cast<const uint8_t*>(vsd->code), vsd->code_size);
    auto vit = d->patched_vs_by_new_hash.find(vhash);
    if (vit != d->patched_vs_by_new_hash.end()) {
      vs_texcoord_index = vit->second.texcoord_index;
    }
    break;
  }
  for (uint32_t i = 0; i < subobject_count; ++i) {
    const auto& sub = subobjects[i];
    if (sub.type != reshade::api::pipeline_subobject_type::pixel_shader) continue;
    if (sub.count != 1u) continue;
    auto* desc = static_cast<reshade::api::shader_desc*>(sub.data);
    if (!desc || desc->code_size == 0u) continue;
    const uint32_t hash = renodx::utils::hash::ComputeCRC32(
        static_cast<const uint8_t*>(desc->code), desc->code_size);
    if (d->patched_ps_hashes.contains(hash)) continue;
    if (custom_shaders.contains(hash)) continue;

    std::vector<std::byte> blob;
    blob.resize(desc->code_size);
    std::memcpy(blob.data(), desc->code, desc->code_size);
    uint32_t new_hash = 0u;
    senkiseki3::dxbc::PixelShaderPatchOptions ps_options;
    ps_options.constant_output = g_phasee_constant >= 0.5f;
    ps_options.no_read = g_phasee_no_read >= 0.5f;
    ps_options.no_math = g_phasee_no_math >= 0.5f;
    ps_options.no_new_output = g_phasee_no_new_output >= 0.5f;
    if (!senkiseki3::dxbc::PatchPerObjectPixelShader(blob, &new_hash, vs_texcoord_index, ps_options)) continue;
    if (new_hash == 0u || new_hash == hash) continue;

    // Evidence capture (DLAAPhaseBDump on): original + patched PS blobs, written
    // BEFORE the driver sees them so a TDR can't lose the evidence.
    WritePhasebDump(hash, desc->code, desc->code_size, ".ps");
    WritePhasebDump(new_hash, blob.data(), blob.size(), ".ps.patched");

    desc->code = malloc(blob.size());
    if (!desc->code) continue;
    std::memcpy(const_cast<void*>(desc->code), blob.data(), blob.size());
    desc->code_size = blob.size();

    d->patched_ps_hashes.insert(hash);     // original
    d->patched_ps_hashes.insert(new_hash); // patched
    changed = true;

    char buf[192];
    snprintf(buf, sizeof(buf), "[DLAA] phaseE: patched PS 0x%08X -> 0x%08X (%u B) const=%d noRead=%d noMath=%d noNewOut=%d",
             hash, new_hash, (uint32_t)blob.size(),
             (int)ps_options.constant_output, (int)ps_options.no_read, (int)ps_options.no_math,
             (int)ps_options.no_new_output);
    reshade::log::message(reshade::log::level::info, buf);
    WatchdogSet(d, "create_pipeline phaseE patched PS 0x%08X -> 0x%08X", hash, new_hash);
  }
  WatchdogSet(d, "create_pipeline done");
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
          WatchdogSet(d, "present f=%u", d->frame_index);
          // Measure frame time for DLSS InFrameTimeDeltaInMsec (motion-speed scaling).
          const auto now = std::chrono::steady_clock::now();
          if (d->last_present_time.time_since_epoch().count() != 0) {
            const double dt_ms = std::chrono::duration<double, std::milli>(
                now - d->last_present_time).count();
            senkiseki3::dlss::dlss_frame_time_delta_ms = (float)dt_ms;
          }
          d->last_present_time = now;
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
          // HDR pre-tone-map path: DLSS ran at a final_blending draw this frame?
          d->dlaa_ran_this_frame = false;
          // Jitter is computed once per frame at PRESENT time, before the next frame's
          // composite (0xE8C7EBA2) draws. Both the composite UV shift and the NGX jitter
          // offsets then read the SAME stored value -> rendered jitter == reported jitter.
          // (Calling it in RunDLAA caused a 1-frame mismatch: the composite used J_{N-1}
          // while NGX got J_N, so DLSS could never align its temporal history.)
          // Save the PREVIOUS frame's jitter (the value about to be overwritten) for the
          // velocity shader's jitter-delta baking (DLAAPhaseJitterInMV).
          d->prev_jitter_x = d->jitter_x;
          d->prev_jitter_y = d->jitter_y;
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
        if (shader_injection.dlaa_enabled < 1.5f) return;  // NGX only in DLAA mode (2)
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
        // DIAGNOSTIC: how many EvaluateDLSS calls happened in the frame that just
        // presented. Must be 1 — if >1, FXAA/RunDLAA fires multiple times per frame
        // and DLSS's internal frame counter advances faster than the presented frames.
        if (d && shader_injection.dlaa_debug_logging > 0.5f && fc % 30 == 0) {
          char fbuf[128];
          snprintf(fbuf, sizeof(fbuf), "[DLAA] freq f=%u evals=%u",
                   d->frame_index, d->evals_this_frame);
          reshade::log::message(reshade::log::level::info, fbuf);
        }
        if (d) d->evals_this_frame = 0;
      });

      reshade::log::message(reshade::log::level::info, "[Senkiseki3 DLAA] Addon loaded");

      reshade::register_event<reshade::addon_event::init_device>([](reshade::api::device* dev) {
        auto* d = dev->create_private_data<DeviceData>();
        // HDR-mod detection (Phase 3): the senkiseki HDR addon ships as
        // _renodx-senkiseki.addon64. When present, DLSS input/output needs the
        // sRGB linearize path. Logged for confirmation; auto-defaulting comes
        // after the Phase 2 A/B verifies DLAAHdrDecode=1 fixes the black screen.
        d->hdr_detected = GetModuleHandleA("_renodx-senkiseki.addon64") != nullptr;
        reshade::log::message(reshade::log::level::info,
          d->hdr_detected ? "[DLAA] HDR mod detected: _renodx-senkiseki.addon64"
                          : "[DLAA] HDR mod not detected");
        WatchdogStart(d);
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
