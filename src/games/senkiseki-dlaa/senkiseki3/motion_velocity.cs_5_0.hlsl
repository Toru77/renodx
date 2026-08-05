// ── Motion Vector Generation: Depth Reprojection + Per-Object MV ──
// Compute shader (cs_5_0) that generates R16G16_FLOAT velocity.
//
// Two sources (per-pixel, object motion wins):
//   1. Per-object motion (dynamic objects): G-buffer MRT2 at t1 holds the
//      previous-frame NDC encoded in o2.zw ([0,1] UV, y-up) by the modified
//      char G-buffer shader (0x0E8BC215). o2.w != 0 marks a valid entry.
//   2. Camera motion (depth reprojection): unproject depth (t0) with the
//      current inverse view-projection, reproject with the previous
//      view-projection, and take the screen-space delta.
//
// Input:  t0 = depth texture (R32_FLOAT or R32_TYPELESS)
//         t1 = G-buffer MRT2 (per-object prevNDC in o2.zw, y-up UV)
//         t2 = effect/particle mask (r16g16_float, 1.0 = excluded from DLAA)
// Output: u0 = velocity buffer (R16G16_FLOAT, screen px, y-down)
//
// Push constants (b13, 32 floats = 8 float4s):
//   c[0..3]  = prevViewProjection (4x4 row-major)
//   c[4..7]  = curViewProjInverse (4x4 row-major)
//   c[8]     = VP_WIDTH, VP_HEIGHT, VELOCITY_SCALE, DEBUG_VIEW
//   c[9]     = JITTER_X(NDC), JITTER_Y(NDC), PER_OBJECT_MOTION, ZERO_MV
//   c[10]    = MV_THRESHOLD(px), MV_DIRECTION(flip), 0, EXCLUDE_EFFECTS
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
  float4 params2;   // x=mv_threshold(px), y=mv_direction(flip), z=0, w=0
};

[numthreads(8, 8, 1)]
void main(uint2 pix : SV_DispatchThreadID)
{
  uint w = (uint)params0.x;
  uint h = (uint)params0.y;
  if (pix.x >= w || pix.y >= h) return;

  // Current pixel center in screen space (y-down)
  float2 curPx = float2(pix) + 0.5;

  // Previous-frame screen position (y-down)
  float2 prevPx = curPx;
  bool hasObjectMotion = false;

  // ── Per-object motion (dynamic objects, from modified VS + G-buffer) ──
  if (params1.z > 0.5f) {
    float4 mrt = g_srcMotion.Load(int3(pix, 0));
    // o2.w != 0 marks a pixel with per-object prevNDC (y-up UV in zw)
    if (abs(mrt.w) > 0.001f) {
      prevPx.x = mrt.z * w;
      prevPx.y = (1.0 - mrt.w) * h;  // flip y-up UV -> y-down screen px
      hasObjectMotion = true;
    }
  }

  // ── Camera motion (depth reprojection) ──
  if (!hasObjectMotion) {
    float depth = g_srcDepth.Load(int3(pix, 0));

    // NDC (y-up) from pixel position
    float2 ndc = (curPx / float2(w, h)) * 2.0 - 1.0;
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

  // Velocity in output-res pixel units (y-down).
  //
  // Jitter compensation:
  //  - Depth-reprojection path: ALREADY jitter-free. Unproject/reproject uses
  //    the UNJITTERED matrices, which is an identity round trip for any depth,
  //    so the geometry jitter (SV_Position shift in the replaced scene VSs)
  //    cancels even though the depth is rasterized at the jittered grid. Do NOT
  //    compensate here (it would inject the jitter into static-environment MVs).
  //  - Per-object path: curPx is the jittered pixel, but the VS prevClip is
  //    unjittered NDC, so the MV picks up the content shift
  //    (+jitter_x*w/2, -jitter_y*h/2 px). Subtract it there only.
  // DLSS receives the same jitter via InJitterOffset (MVJittered=0).
  float2 vel = (curPx - prevPx) * params0.z;
  if (hasObjectMotion) {
    vel.x -= params1.x * params0.x * 0.5f;
    vel.y += params1.y * params0.y * 0.5f;
  }
  // Zero-MV A/B: force all motion vectors to 0 (isolates whether MVs help or hurt).
  if (params1.w > 0.5f) vel = float2(0.0, 0.0);
  // MV Direction A/B: flip sign (previous-current instead of current-previous).
  if (params2.y > 0.5f) vel = -vel;
  // MV Threshold: zero out sub-pixel noise (static dots) that poison history.
  if (length(vel) < params2.x) vel = float2(0.0, 0.0);

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
