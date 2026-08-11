// ── Motion Vector Generation: Depth Reprojection + Per-Object MV ──
// Compute shader (cs_5_0) that generates R16G16_FLOAT velocity.
//
// Two sources, COMBINED per-pixel (full MV = camera + object):
//   1. Per-object motion (dynamic objects): G-buffer MRT2 at t1 holds the
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
//         t1 = G-buffer MRT2 (per-object prevNDC in o2.zw, y-up UV)
//         t2 = effect/particle mask (r16g16_float, 1.0 = excluded from DLAA)
// Output: u0 = velocity buffer (R16G16_FLOAT, screen px, y-down)
//
// Push constants (b13, 48 floats = 12 float4s):
//   c[0..3]  = prevViewProjection (4x4 row-major)
//   c[4..7]  = curViewProjInverse (4x4 row-major)
//   c[8]     = VP_WIDTH, VP_HEIGHT, VELOCITY_SCALE, DEBUG_VIEW
//   c[9]     = JITTER_X(NDC), JITTER_Y(NDC), PER_OBJECT_MOTION, ZERO_MV
//   c[10]    = MV_THRESHOLD(px), MV_DIRECTION(flip), MV_MODE, EXCLUDE_EFFECTS
//              MV_MODE: 0=MVJittered=0 no per-object subtract (B), 1=MVJittered=0
//              subtract (A, current), 2=MVJittered=1 whole buffer jittered (C)
//   c[11]    = OBJECT_MV_THRESHOLD(px) — per-object/Prev-Bone MVs only
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
  float4 params2;   // x=mv_threshold(px), y=mv_direction(flip), z=mv_mode, w=exclude_effects
  float4 params3;   // x=mv_threshold_object(px), y=prev_jitter_x(NDC), z=prev_jitter_y(NDC), w=jitter_in_mv
  float4 params4;   // x=depth_sample_unjit, yzw=unused
};

[numthreads(8, 8, 1)]
void main(uint2 pix : SV_DispatchThreadID)
{
  uint w = (uint)params0.x;
  uint h = (uint)params0.y;
  if (pix.x >= w || pix.y >= h) return;

  // DLAAPhaseSynthMV (params4.y): synthetic MV test — bypass depth reprojection
  // entirely and write an ANALYTIC global MV, so we can establish the correct
  // jitter-delta MV + sign in the reproj debug view without DLSS/depth/matrices.
  //   1 = zero everywhere (Test A)
  //   2 = Jcur - Jprev  (pixel-space, y-down)
  //   3 = Jprev - Jcur  (flipped sign)
  if (params4.y > 0.5f) {
    float2 synth = 0.0;
    if (params4.y > 1.5f) {
      // Jcur - Jprev in y-down px: x = (jx - prevJx)*w/2, y = -(jy - prevJy)*h/2
      synth.x = (params1.x - params3.y) * params0.x * 0.5f;
      synth.y = -(params1.y - params3.z) * params0.y * 0.5f;
      if (params4.y > 2.5f) synth = -synth;
    }
    g_outVelocity[pix] = synth;
    return;
  }

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

  // ── Per-object motion (dynamic objects, from modified VS + 32-bit target) ──
  // The patched VS encodes the OBJECT-ONLY motion delta prevNDC-curNDC (both
  // skins projected with the CURRENT ViewProjection, so the camera component is
  // NOT included — immune to prevVP pairing/staleness that made the old full-MV
  // delta a huge ~-0.9 NDC rigid-body offset) as a 24-bit code
  // E = (Ix*4096 + Iy)/16777216 into TEXCOORD10.zw, and the GAME's UNPATCHED
  // G-buffer PS packs E into o2 (RT2) as 3 bytes: o2 = (byte0/256, byte1/256,
  // byte2, 1). We swap RT2 for our r32g32b32a32_float target.
  // Decode: code = byte0*65536 + byte1*256 + byte2; Ix = code/4096, Iy = code
  // mod 4096; vx = Ix/4096*2-1, vy = Iy/4096*2-1 (±1.0 NDC = ±1280px at 2560w).
  // The camera component of the object's motion is added by the depth
  // reprojection below (per-object pixels ADD the object delta to the camera
  // prevPx, so the full MV = camera + object).
  if (params1.z > 0.5f) {
    float4 mrt = g_srcMotion.Load(int3(pix, 0));
    float code = round(mrt.r * 256.0f) * 65536.0f +
                 round(mrt.g * 256.0f) * 256.0f +
                 round(mrt.b * 256.0f);
    if (code > 0.5f) {
      float ix = floor(code / 4096.0f);
      float iy = code - ix * 4096.0f;
      float vx = (ix / 4096.0f) * 2.0f - 1.0f;    // ×0.5 encode: ±1.0 NDC
      float vy = (iy / 4096.0f) * 2.0f - 1.0f;
      // Robustness: only trust object-only MVs within a sane magnitude. Values
      // beyond 1000px mean the prev-bone snapshot was stale/mixed or the motion
      // is absurd — falling back to the camera path keeps the character
      // consistent (no jitter).
      if (max(abs(vx) * w * 0.5f, abs(vy) * h * 0.5f) < 1000.0f) {
        objectDeltaPx.x = vx * w * 0.5f;
        objectDeltaPx.y = -vy * h * 0.5f;
        hasObjectMotion = true;
      }
    }
  }

  // ── Camera motion (depth reprojection) — ALWAYS. Per-object pixels ADD the
  // object delta on top, so their full MV = camera + object (consistent with
  // the background). ──
  {
    // DLAAPhaseDepthSampleUnjit (params4.x): the depth buffer is UNJITTERED
    // (our depth_only fix), so depth[pix] belongs to the content at UNJITTERED
    // position pix, NOT at curPxU = pix - jitterPx. Sampling at pix therefore
    // misreads the depth by up to half a pixel whenever jitter is on, injecting
    // a per-frame MV error DLSS can't compensate. Sample at the UNJITTERED pixel
    // (curPxU, rounded to the nearest texel) so depth and position agree.
    int2 depthPix = int2(pix);
    if (params4.x > 0.5f) {
      depthPix = int2(round(curPxU));
      depthPix = clamp(depthPix, int2(0, 0), int2((int)w - 1, (int)h - 1));
    }
    float depth = g_srcDepth.Load(int3(depthPix, 0));

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
    if (hasObjectMotion) {
      prevPx += objectDeltaPx;
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
  // params2.z = MV jitter mode (DLAAPhaseMVJittered / DLAAPhaseMVComp):
  //   0/1 = MVJittered=0 (Tests A/B): the buffer is jitter-free — no per-object
  //       subtraction is needed anymore (they are camera-path based).
  //   2   = MVJittered=1 (Test C): add the current jitter so the whole buffer
  //       is consistently jittered and DLSS removes it internally.
  if (params2.z > 1.5f) {
    vel.x += params1.x * params0.x * 0.5f;
    vel.y -= params1.y * params0.y * 0.5f;
  }
  // DIAGNOSTIC (DLAAPhaseJitterInMV, params3.w): this DLSS runtime IGNORES the
  // NGX jitter offset (report 0 vs real = identical), so the per-frame jitter
  // reads as unresolvable sub-pixel motion -> shimmer, worst at low FPS. Bake the
  // jitter DELTA (current - previous frame, px) into the MVs so DLSS's MV-based
  // reprojection aligns the jittered history. Base MVs must be jitter-free for
  // this to be exact: camera via curPxU, per-object via the Test A subtraction
  // above (params2.z ~ 1). At static: vel = jitterCur - jitterPrev, which maps the
  // content at the current jittered pixel back to its previous jittered position.
  if (params3.w > 0.5f) {
    vel.x += (params1.x - params3.y) * params0.x * 0.5f;   // (jx - prevJx) * w/2
    vel.y -= (params1.y - params3.z) * params0.y * 0.5f;   // -(jy - prevJy) * h/2
  }
  // Zero-MV A/B: force all motion vectors to 0 (isolates whether MVs help or hurt).
  if (params1.w > 0.5f) vel = float2(0.0, 0.0);
  // MV Direction A/B: flip sign (previous-current instead of current-previous).
  if (params2.y > 0.5f) vel = -vel;
  // MV Threshold: zero out sub-pixel noise (static dots) that poison history.
  // Camera (depth-reprojection) MVs use params2.x (DLAAMVThreshold);
  // per-object / Prev-Bone MVs use params3.x (DLAAPerObjectMVThreshold).
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
