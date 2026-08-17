// ── Motion Vector Generation: Depth Reprojection + Per-Object MV ──
// Compute shader (cs_5_0) that generates R16G16_FLOAT velocity.
//
// Two sources, COMBINED per-pixel (full MV = camera + object):
//   1. Per-object motion (dynamic objects): normally the dedicated Phase E
//      motion target at t1 holds the raw OBJECT-ONLY delta. Legacy RT2 mode
//      uses the packed fallback described below.
//      G-buffer MRT2 at t1 holds the
//      OBJECT-ONLY motion delta prevNDC-curNDC (both skins projected with the
//      CURRENT ViewProjection, so camera motion is NOT included) encoded in
//      o2.zw ([0,1], y-up) by the modified char G-buffer VS. o2.w != 0 marks a
//      valid entry. The object delta (px) is ADDED to the camera-path prevPx.
//   2. Camera motion (depth reprojection): unproject depth (t0) with the
//      current inverse view-projection, reproject with the previous
//      view-projection, and take the screen-space delta. Always runs; the
//      per-object delta is added on top for dynamic-object pixels.
//
// Input:  t0 = depth texture (R32_FLOAT or R32_TYPELESS)
//         t1 = motion source (dedicated r32g32b32a32_float in Phase E, or the
//              legacy game's r8g8b8a8_unorm RT2): the packer
//              PS fills o2.rgb with the 24-bit per-object encode E and o2.w=1
//              (alpha) as its validity flag where it wrote (the character);
//              background pixels carry the game's real packed depth with
//              alpha=0 and are ignored via the alpha gate (no addon swap:
//              Fix 1 keeps the game's G-buffer intact).
//         t2 = effect/particle mask (r16g16_float, 1.0 = excluded from DLAA)
// Output: u0 = velocity buffer (R16G16_FLOAT, screen px, y-down)
//
// Push constants (b13, 56 floats = 14 float4s):
//   c[0..3]  = prevViewProjection (4x4 row-major)
//   c[4..7]  = curViewProjInverse (4x4 row-major)
//   c[8]     = VP_WIDTH, VP_HEIGHT, VELOCITY_SCALE, DEBUG_VIEW
//   c[9]     = JITTER_X(NDC), JITTER_Y(NDC), PER_OBJECT_MOTION, ZERO_MV
//   c[10]    = MV_THRESHOLD(px), reserved, reserved, EXCLUDE_EFFECTS
//   c[11]    = OBJECT_MV_THRESHOLD(px), reserved, reserved, reserved
//   c[12]    = OBJECT_DEPTH_EPS, OBJECT_DEPTH_TEST, FULL_MV_MODE, reserved
//   c[13]    = STATIC_CAMERA_SC, OBJECT_DELTA_DIRECT, MAX_VP_DELTA, (unused)
//
// SPDX-License-Identifier: MIT

Texture2D<float> g_srcDepth : register(t0);
Texture2D<float4> g_srcMotion : register(t1);
Texture2D<float4> g_srcEffectMask : register(t2);
RWTexture2D<float2> g_outVelocity : register(u0);

cbuffer cb_push : register(b13) {
  float4x4 prevViewProj;
  float4x4 curViewProjInv;
  float4 params0;   // x=vp_w, y=vp_h, z=velocity_scale, w=debug_view
  float4 params1;   // x=jitter_x(NDC), y=jitter_y(NDC), z=per_object_motion, w=zero_mv
  float4 params2;   // x=mv_threshold(px), w=exclude_effects
  float4 params3;   // x=mv_threshold_object(px)
  float4 params4;   // x=object_depth_epsilon, y=object_depth_test, z=full_mv_mode
  float4 params5;   // x=static_camera_shortcircuit, y=object_delta_direct,
                    // z=max_vp_delta (|prevVP-curVP| max; < 1e-4 => camera still),
                    // w=dedicated Phase E source
};

[numthreads(8, 8, 1)]
void main(uint2 pix : SV_DispatchThreadID)
{
  uint w = (uint)params0.x;
  uint h = (uint)params0.y;
  if (pix.x >= w || pix.y >= h) return;

  // Current pixel center in screen space (y-down)
  float2 curPx = float2(pix) + 0.5;

  // Content shift of the current frame's sub-pixel jitter (px, y-down): the
  // jittered render shows content from the unjittered position (p - jitterPx).
  float2 jitterPx = float2(params1.x * params0.x * 0.5f, -params1.y * params0.y * 0.5f);
  // The UNJITTERED current pixel. The depth is rasterized at the JITTERED grid,
  // so depth[p] belongs to the content at unjittered p - jitterPx. Unprojecting
  // at the jittered pixel with the unjittered matrices would inject the frame's
  // jitter into the MVs (vel = vel_true + jitterPx) whenever the camera moves —
  // DLSS then applies the jitter offset again -> misaligned history -> shimmer,
  // worst at low FPS (few temporal samples to average it out). Use the
  // unjittered pixel so the round trip is exact for BOTH static (vel = 0) and
  // moving (vel = vel_true), for jittered AND unjittered depth alike.
  float2 curPxU = curPx - jitterPx;

  // Previous-frame screen position (y-down). Default = current pixel (no motion).
  float2 prevPx = curPxU;
  float2 objectDeltaPx = float2(0.0, 0.0);
  bool hasObjectMotion = false;

  // ── Per-object motion (dynamic objects, from the modified VS) ──
  // The patched VS encodes the OBJECT-ONLY motion delta prevNDC-curNDC (both
  // skins projected with the CURRENT ViewProjection, so the camera component is
  // NOT included — immune to prevVP pairing/staleness that made the old full-MV
  // delta a huge ~-0.9 NDC rigid-body offset) as a 24-bit code
  // E = (Ix*4096 + Iy)/16777216 into TEXCOORD10.zw, and the GAME's UNPATCHED
  // G-buffer PS packs E into o2 (RT2) as 3 bytes: o2 = (byte0/256, byte1/256,
  // byte2, 1). t1 is the GAME's real RT2 (r8g8b8a8_unorm) — no swap (Fix 1):
  // the game's G-buffer stays intact. FIX 1c re-centers the encode to E =
  // delta + 0.75 (S=1) so a still character sits at byte0=192 — bit-exact
  // through the 8-bit RT2 (the old 0.5 center hit the +-1-LSB byte0=128 band
  // and floored every still per-object delta at ~+-1.25..2.2px).
  // Decode: code = byte0*65536 + byte1*256 + byte2; Ix = code/4096, Iy = code
  // mod 4096; delta = ((Ix/4096)-0.5)/S with S = 4.0 (±0.125 NDC = ±160px at
  // 2560w; 0.078px/step quantization — 8x finer than the old ±1.0 NDC range,
  // which was the main per-pixel noise / DLAA detail-smear source).
  // The camera component of the object's motion is added by the depth
  // reprojection below (per-object pixels ADD the object delta to the camera
  // prevPx, so the full MV = camera + object).
  //
  // FIX 1b (legacy RT2 alpha validity gate): the GAME's RT2 is a FULL-SCREEN target — it
  // holds the per-object encode E only where the packer PS wrote it (o2.w=1 ->
  // alpha=1, the character) and REAL packed depth everywhere else (alpha=0,
  // background). Reading the whole RT2 as per-object decoded the background
  // depth into fake 200-360px MVs (the environment-wide ghost). The packer
  // writes o2.w=1 unconditionally, so alpha IS the encode's own validity flag:
  // gate per-object on it — only character pixels are per-object, the
  // background falls through to the camera path.
  if (params1.z > 0.5f) {
    float4 mrt = g_srcMotion.Load(int3(pix, 0));
    if (params5.w > 0.5f) {
      // Phase E: the dedicated R32G32B32A32 target receives the raw
      // object-only delta from the existing TEXCOORD10.xy carrier. Alpha is
      // cleared to zero and written as one by the patched PS, so it is a
      // genuine validity bit here (unlike the game's shared RT2 alpha).
      if (mrt.a > 0.5f && max(abs(mrt.x), abs(mrt.y)) < 4.0f) {
        objectDeltaPx = float2(mrt.x * w * 0.5f, -mrt.y * h * 0.5f);
        if (max(abs(objectDeltaPx.x), abs(objectDeltaPx.y)) < 1000.0f)
          hasObjectMotion = true;
        // ── Object-depth visibility test (DLAAPhaseObjectDepthTest) ──
        // The patched Phase E PS stores the object's clip z/w (the SAME surface
        // depth the rasterizer wrote to the depth buffer) in mrt.z. If it
        // matches the captured scene depth (within epsilon), this object IS the
        // visible surface and its MV wins. If it differs, the object is occluded
        // by a later-drawn surface — fall back to the camera (depth-reprojected)
        // MV, which is correct for whatever is actually visible.
        if (params4.y > 0.5f && hasObjectMotion) {
          float sceneDepth = g_srcDepth.Load(int3(pix, 0));
          if (abs(mrt.z - sceneDepth) > params4.x)
            hasObjectMotion = false;
        }
      }
    } else {
      float code = round(mrt.r * 256.0f) * 65536.0f +
                   round(mrt.g * 256.0f) * 256.0f +
                   round(mrt.b * 256.0f);
      if (code > 0.5f && mrt.a > 0.5f) {
        float ix = floor(code / 4096.0f);
        float iy = code - ix * 4096.0f;
      // Decode the object delta (FIX 1c re-center). VS encode is now
      // E = clamp(delta + 0.75, 0, 1) with S = 1.0 for BOTH modes; Ix =
      // round_z(E*4096) -> delta = Ix/4096 - 0.75. (The old center 0.5 sat a
      // still character on byte0=128, the 8-bit RT2's +-1-LSB rounding band,
      // which floored every small per-object delta at ~+-1.25..2.2px.)
      float vx = (ix / 4096.0f) - 0.75f;
      float vy = (iy / 4096.0f) - 0.75f;
      // Robustness: with the re-centered S=1 encode the decoded delta ranges
      // up to +-0.75 NDC (+-960px) before the clamp-to-0/1 falls back, so
      // stale/garbage prev-bone content that survives the CPU-side staleness
      // guard could reach a few hundred px. This check remains as a final
      // safety net; values beyond 1000px fall back to the camera path.
        if (max(abs(vx) * w * 0.5f, abs(vy) * h * 0.5f) < 1000.0f) {
          objectDeltaPx.x = vx * w * 0.5f;
          objectDeltaPx.y = -vy * h * 0.5f;
          hasObjectMotion = true;
        }
      }
    }

    // Full-MV carrier coordinates must be in the same unjittered space as
    // curPxU below. With the default Global jitter method, the game's current
    // b0 ViewProjection has J_current applied when the patched VS reconstructs
    // curClip, whereas our prevVP cbuffer deliberately remains unjittered.
    // Thus the raw carrier is (prev - current - J_current). Restore
    // J_current here so a still character has exactly zero object motion and a
    // moving character keeps its real small per-frame delta. Per-VS jitter is
    // applied after this injected reconstruction, so it does not need this
    // correction.
    if (hasObjectMotion && params4.z > 0.5f)
      objectDeltaPx += jitterPx;
  }

  // ── Camera motion (depth reprojection) — ALWAYS. Per-object pixels ADD the
  // object delta on top, so their full MV = camera + object (consistent with
  // the background). ──
  {
    // ROOT-CAUSE FIX 1 (DLAAPhaseStaticCameraSC, params5.x): when the
    // ViewProjection is unchanged (params5.z = max |prevVP-curVP| < 1e-4), the
    // unproject/reproject round trip is a mathematical identity that only
    // injects float precision noise (~0.01-0.02px on the near character — the
    // yellow/purple HSV spots). Skip it: prevPx stays curPxU -> static content
    // gets EXACTLY zero velocity at the source, no threshold needed.
    if (!(params5.x > 0.5f && params5.z < 0.0001f)) {
      float depth = g_srcDepth.Load(int3(pix, 0));

      // NDC (y-up) from the UNJITTERED pixel position
      float2 ndc = (curPxU / float2(w, h)) * 2.0 - 1.0;
      ndc.y = -ndc.y;

      // Unproject to world space
      float4 clipPos = float4(ndc, depth, 1.0);
      float4 worldPos = mul(clipPos, curViewProjInv);
      if (abs(worldPos.w) > 1e-6) {
        worldPos /= worldPos.w;

        // Reproject to previous screen
        float4 prevClip = mul(worldPos, prevViewProj);
        if (abs(prevClip.w) > 1e-5) {
          float2 prevNDC = prevClip.xy / prevClip.w;
          prevPx.x = (prevNDC.x * 0.5 + 0.5) * w;
          prevPx.y = (0.5 - prevNDC.y * 0.5) * h;  // y-up NDC -> y-down px
        }
      }
    }
    if (hasObjectMotion) {
      // ROOT-CAUSE FIX 2 (DLAAPhaseObjectDeltaDirect, params5.y): trust the
      // EXACT per-object delta instead of the round-22 confidence blend that
      // routes small deltas onto the noisy camera path. Full-MV: the delta IS
      // the complete screen-space motion (camera included) -> prevPx = curPxU
      // + objDelta. Object-only: relative delta added to the camera path.
      // The user-controlled per-object deadband rejects only as much subtle
      // motion as desired. It also gates the final per-object MV below, so a
      // single slider governs this direct-path cutoff end-to-end.
      if (params5.y > 0.5f) {
        float objLen = length(objectDeltaPx);
        if (objLen < params3.x) objectDeltaPx = float2(0.0, 0.0);
        if (params4.z > 0.5f) {
          prevPx = curPxU + objectDeltaPx;
        } else {
          prevPx += objectDeltaPx;
        }
      } else {
        // ── Object-motion confidence blend (round 22): ──
        // The character's per-object delta includes its idle/root-bob motion (a
        // few px to ~12px, oscillating frame-to-frame even when standing "still"
        // — the "arrows fire wildly at still" symptom). Those small per-object
        // MVs are NOT depth-consistent with the camera path, so DLSS rejects the
        // character's history -> no accumulation -> the whole character shakes,
        // jitter-independent, in every per-object variant. Fix: blend toward the
        // depth-consistent camera-path prevPx (which DLSS accepts) when the
        // per-object delta is SMALL, and use the per-object delta only when the
        // motion is substantial (real walking/limbs). objLen <= 12px -> camera
        // path (stable at still — covers the whole idle range); >= 20px -> full
        // per-object (real motion; walking deltas are 90-180px).
        const float kObjConfLowPx = 12.0f, kObjConfHighPx = 20.0f;
        float objLen = length(objectDeltaPx);
        float w = saturate((objLen - kObjConfLowPx) / (kObjConfHighPx - kObjConfLowPx));
        if (params4.z > 0.5f) {
        // Full-MV mode: the carrier has already had the Global-jitter term
        // removed above, so this is the complete jitter-free screen-space
        // motion (prevBone x prevVP - curBone x curVP).
        prevPx = lerp(prevPx, curPxU + objectDeltaPx, w);
        } else {
          // Object-only: add the (confidence-scaled) object delta onto the
          // depth-reprojected camera path (full MV = camera + object).
          prevPx += objectDeltaPx * w;
        }
      }
    }
  }

  // Velocity in output-res pixel units (y-down), jitter-free (curPxU, the
  // camera path, and the object delta are all unjittered).
  //
  // Jitter compensation:
  //  - Depth-reprojection path: the unproject/reproject runs with the UNJITTERED
  //    matrices at the UNJITTERED pixel (curPxU), so it is an exact identity for
  //    static content (vel = 0) AND yields the correct jitter-free MV for moving
  //    content (vel = vel_true).
  //  - Per-object path: now built on the jitter-free camera path + jitter-free
  //    object delta, so it needs NO jitter subtraction (the old per-object
  //    curPx-based full-MV path picked up the content shift and subtracted it).
  // DLSS receives the same jitter via InJitterOffset (MVJittered=0).
  float2 vel = (curPxU - prevPx) * params0.z;
  // Zero-MV A/B: force all motion vectors to 0 (isolates whether MVs help or hurt).
  if (params1.w > 0.5f) vel = float2(0.0, 0.0);
  vel = -vel;
  // MV Threshold: zero out sub-pixel noise (static dots) that poison history.
  // Camera (depth-reprojection) MVs use params2.x (DLAAMVThreshold);
  // per-object / Prev-Bone MVs use params3.x (Per-Object Motion Deadband).
  float mvThresh = hasObjectMotion ? params3.x : params2.x;
  if (length(vel) < mvThresh) vel = float2(0.0, 0.0);

  // Exclude effects/particles from DLAA: an off-screen motion vector makes DLSS
  // fall back to the current input frame (no temporal history), which prevents
  // shimmer/ghosting on content DLSS can't resolve. The effect PSs write 1.0
  // into the mask (t2) during their passes.
  if (params2.w > 0.5f && g_srcEffectMask.Load(int3(pix, 0)).x > 0.5f) vel = float2(100000.0, 100000.0);

  // Guard against garbage from invalid matrices/depth (e.g. startup frames)
  if (!isfinite(vel.x) || !isfinite(vel.y)) vel = 0;

  // Always write the raw velocity; the separate mv_debug compute converts it
  // to a readable display (HSV / arrows / magnitude / reprojection).
  g_outVelocity[pix] = vel;
}
