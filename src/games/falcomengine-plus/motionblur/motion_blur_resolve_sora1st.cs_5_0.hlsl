// Motion Blur — camera-length weighting over the game's motion buffer. SORA 1ST.
//
// ── READ THIS FIRST: THIS FILE IS A CLONE, AND THAT IS THE POINT ──────────────
// It is a byte-for-byte copy of motion_blur_resolve.cs_5_0.hlsl except for TWO
// cbuffer offsets, and it must stay that way. Every other line — the row-vector
// convention, the depth self-validation, the camera-length blend, the camera-cut
// test, the pre-tile clamp — is load-bearing and was arrived at the hard way. A
// hand-rewritten second variant is how the row-vector convention got transposed in
// the first place.
//
// If a fix ever lands in the Sora 2nd resolve, port it here verbatim. If the two
// files ever differ by more than the offsets and the comments describing them, that
// is a bug, not an optimisation.
//
// ── WHY TWO VARIANTS AT ALL ───────────────────────────────────────────────────
// HLSL packoffset() is a compile-time constant, so the scene cbuffer layout cannot
// be a push constant. The two games disagree:
//
//   field              Sora 1st   Sora 2nd
//   prevViewProj_g     c74        c75
//   jitterDiff_g       c78        c79
//
// Sourced by diffing the cbuffer declarations of the two games' own GBuffer
// shaders, which are the authority:
//
//   sora1st/foliage/staticfoliage_0x39F91AE8.ps_5_0.hlsl  -> c74 / c78
//   sora2nd/foliage/staticfoliage_0xF1EC53A8.ps_5_0.hlsl  -> c75 / c79
//
// c0 through c31 are IDENTICAL in both, which is why view_g, proj_g and
// viewProjInv_g are declared with the same offsets here as there, and why the
// gather and tilemax need no variant at all — the gather only reads c0..c12.
//
// These offsets are read from a GBuffer shader but consumed at the TONEMAP, a
// different point in the frame. That the Sora 2nd copy has been correct all along
// is the evidence that the layout is stable from GBuffer to tonemap for this engine
// family. The Object Residual debug view is the readout for the Sora 1st copy: it
// must be BLACK on a static scene with the camera moving, because object motion is
// then zero by definition and any lit residual is a wrong offset. A lit residual
// here means c74 is wrong, not that object motion exists.
//
// Everything below the cbuffer is shared rationale, reproduced in full rather than
// pointed at, so that this file can be read on its own.

#include "motion_blur_common.hlsli"

cbuffer cb_scene : register(b0)
{
  // proj_g and view_g are used ONLY to cross-check the reconstruction against the
  // gather's own depth linearization, which is the formula proven at
  // motion_blur_gather.cs_5_0.hlsl:133-134. Same matrices, same scalars, so the
  // two routes to a view-space distance are directly comparable. c0 and c8 are
  // identical in both games.
  float4x4 view_g        : packoffset(c0);
  float4x4 proj_g        : packoffset(c8);
  // The ENGINE's own screen->world inverse (staticfoliage_0x39F91AE8:10 for this
  // game). Use it rather than composing viewInv_g (c4) with projInv_g (c12): the
  // composition order is a guess about a row-major layout, and the product is
  // already supplied. c20 is identical in both games.
  float4x4 viewProjInv_g : packoffset(c20);
  // ── THE ONLY TWO LINES THAT DIFFER FROM THE SORA 2ND RESOLVE ───────────────
  // Sora 1st offsets, from the game's own staticfoliage shader
  // 0x39F91AE8. Sora 2nd uses c75 and c79 for the same two fields.
  float4x4 prevViewProj_g : packoffset(c74);
  float2  jitterDiff_g    : packoffset(c78);
};

Texture2D<float2>   g_srcMotion : register(t0);
Texture2D<float>    g_srcDepth  : register(t1);
#if defined(MB_RESOLVE_WIDE)
// Wide variant, bound only while a camera-term debug view (8/9) is active: the
// RGBA16F surface is what can carry the camera-only term in .zw. See the wrapper
// files and the addon's resolve-format selection.
RWTexture2D<float4> g_outMotion : register(u0);
#else
// Shipping variant. The filter reads only .xy, so the surface is RG16F and this
// declaration is exactly the two channels it writes. Halves the bytes per texel
// on the texture the gather fetches once per tap, and stores the identical half
// values the wide surface would have held. Debug views 8/9 read .zw and force
// the wide variant instead.
RWTexture2D<float2> g_outMotion : register(u0);
#endif

[numthreads(8, 8, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID) {
  const int2 motionDims = max(int2(shader_injection_data.mb_motion_w,
                                   shader_injection_data.mb_motion_h), int2(1, 1));
  const int2 p = int2(dispatchThreadID.xy);
  if (p.x >= motionDims.x || p.y >= motionDims.y) return;

  const float2 gamePx = g_srcMotion[p];

  // A dead motion buffer must not be interpreted as "the camera did not move";
  // the gather early-outs on zero motion, which is the correct passthrough.
  if (shader_injection_data.mb_motion_valid < 0.5) {
    // Scalar broadcast, so the early-out is correct on both the narrow and the
    // wide declaration without an implicit truncation warning.
    g_outMotion[p] = 0.0;
    return;
  }

  // ── CAMERA-CUT REJECTION ────────────────────────────────────────────────────
  // A teleport or a cutscene transition replaces the view transform in a single
  // frame. The game still writes motion vectors as though the camera had moved
  // continuously, so the entire frame smears toward the warp point. UE's velocity
  // pass suppresses velocity for one frame when it detects this; this is the same
  // behaviour, and it is here rather than in the gather because the gather has
  // already lost the information by the time it runs.
  //
  // The test is on the CAMERA TRANSFORM, not on the motion texture. That
  // distinction is the whole point: the game's motion buffer mixes camera and
  // object motion, so neither a magnitude threshold nor a "did the average change
  // abruptly" test on it can separate a fast whip from a cut. mul(view_g, proj_g)
  // is the CURRENT view-projection in the same convention as prevViewProj_g, so
  // comparing the two measures only how far the view moved this frame -- a moving
  // object cannot trigger it, and neither can a smooth pan.
  //
  // On the unvalidated prevViewProj offset: this is the ONE place relying on it is
  // safe, and the reason is asymmetric. Reconstructing exact velocity needs the
  // offset to be precisely right, and getting it wrong was catastrophic. A
  // magnitude THRESHOLD does not: a wrong mul() convention yields a bounded, smooth
  // displacement, a real cut yields one orders of magnitude larger. The test
  // survives the first and catches the second.
  //
  // Note that the depth consistency gate below does NOT catch this. A cut produces
  // enormous but internally self-consistent values, so `consistent` is satisfied
  // and `projectable` passes. That is exactly the hole this fills.
  //
  // The result is uniform across the dispatch, so the compiler hoists it, and it
  // runs before the depth fetch so a cut costs one matrix evaluation and no loads.
  bool cut = false;
  if (shader_injection_data.mb_camera_cut > 0.5f) {
    const float limitPx = max(shader_injection_data.mb_camera_cut_px, 1.0f);
    const float4x4 curViewProj = mul(view_g, proj_g);
    // Five probes at a nominal depth: the centre catches a pure translation, the
    // corners catch rotation and FOV change. Deliberately not perspective-
    // corrected points -- both projections are evaluated the same way, so the
    // comparison stays like-for-like and the measure remains valid.
    const float2 probes[5] = {float2(0.0, 0.0), float2(-0.5, -0.5), float2(0.5, -0.5),
                              float2(-0.5, 0.5), float2(0.5, 0.5)};
    [loop]
    for (int i = 0; i < 5; ++i) {
      const float4 probe = float4(probes[i], 0.5, 1.0);
      const float4 cur = mul(probe, curViewProj);
      const float4 prv = mul(probe, prevViewProj_g);
      // A degenerate probe means the transform is not usable at all. Treat it as a
      // cut: a false positive costs one sharp frame, a false negative costs a
      // full-screen smear, and the two are not worth trading equally.
      if (!(abs(cur.w) > 1e-6) || !(abs(prv.w) > 1e-6)
          || !all(isfinite(cur.xy)) || !all(isfinite(prv.xy))) {
        cut = true;
        break;
      }
      const float2 curUV = (cur.xy / cur.w) * float2(0.5, -0.5) + 0.5;
      const float2 prvUV = (prv.xy / prv.w) * float2(0.5, -0.5) + 0.5;
      if (length((curUV - prvUV) * MB_REF_H) > limitPx) {
        cut = true;
        break;
      }
    }
  }
  if (cut) {
    // Scalar broadcast, so the early-out is correct on both the narrow and the
    // wide declaration without an implicit truncation warning.
    g_outMotion[p] = 0.0;
    return;
  }

  const float2 uv = (float2(p) + 0.5) / float2(motionDims);
  const int2 depthDims = max(int2(shader_injection_data.mb_depth_w,
                                  shader_injection_data.mb_depth_h), int2(1, 1));
  const float depth = g_srcDepth[clamp(int2(uv * float2(depthDims)),
                                       int2(0, 0), depthDims - 1)];

  // Screen -> NDC. The y flip is the exact inverse of the game's
  // `ndc * float2(0.5,-0.5) + 0.5` step, and matches the convention the motion
  // buffer is written in.
  //
  // ── mul(VECTOR, MATRIX), NOT mul(matrix, vector) ────────────────────────────
  // The engine stores its matrices ROW-MAJOR and applies them as row-vector
  // products. The proof is the game's own reprojection, which divides the xy by
  // the result of dotting with _m03_m13_m23_m33 -- _mRC selects a COLUMN, so that
  // is column 3, i.e. w = sum_j v_j * M[j][3] = [v * M].
  //
  //   r1.x = dot(r0.xyzw, ssrPrevViewProj_g._m00_m10_m20_m30);
  //   r1.y = dot(r0.xyzw, ssrPrevViewProj_g._m01_m11_m21_m31);
  //   r0.x = dot(r0.xyzw, ssrPrevViewProj_g._m03_m13_m23_m33);
  //   r0.xy = r1.xy / r0.xx;                       // <- that one is the divide by w
  //
  // Writing mul(matrix, vector) transposes the projection. w stays finite while
  // the xy terms collapse toward a point, so prevUV collapses toward screen
  // centre and every pixel receives a motion vector pointing inward with
  // magnitude proportional to its distance from the centre. That produced a
  // whirlpool in the middle of the frame with everything blurring into it.
  const float2 ndc = uv * float2(2.0, -2.0) + float2(-1.0, 1.0);

  // ── SELF-VALIDATION: cross-check against the gather's own linearization ──────
  // The gather converts raw depth to a view-space distance with exactly this
  // formula (motion_blur_gather.cs_5_0.hlsl:133-134) and is proven there. We
  // reconstruct the same distance by a second, independent route and require the
  // two to agree. Nothing here is a guess about a constant buffer layout, and a
  // reconstruction that is numerically finite but geometrically meaningless cannot
  // pass.
  //
  // This exists because "did we divide by zero" is the WRONG question. The engine
  // uses an INFINITE FAR reversed-Z projection, so a sky or cleared pixel at
  // depth 0 is a legal point AT INFINITY: w lands around 1e-7, clears any small
  // epsilon, and world.xyz / w then amplifies float error into an enormous
  // position. For exactly those pixels addC - depth underflows, so zLin goes
  // non-finite and the pixel is rejected.
  float mulC = -proj_g[3][2];
  float addC =  proj_g[2][2];
  // Same sign correction the gather applies (motion_blur_gather.cs_5_0.hlsl:135).
  // Without it zLin comes out NEGATIVE for this projection and the zLin > 0 test
  // below would reject every pixel, silently disabling the camera term entirely.
  if (mulC * addC < 0.0) addC = -addC;
  const float zLin = mulC / (addC - depth);

  const float4 world = mul(float4(ndc, depth, 1.0), viewProjInv_g);
  const bool solvable = (abs(world.w) > 1e-6) && all(isfinite(world.xyz));
  const float3 worldPos = solvable ? (world.xyz / world.w) : float3(0.0, 0.0, 0.0);

  // View-space depth of the reconstructed point, to compare against zLin. abs()
  // covers the handedness: the gather clamps its linearized depth positive, so
  // the sign of view-space z is a convention this check must not depend on.
  const float zRec = solvable ? abs(mul(float4(worldPos, 1.0), view_g).z) : 0.0;
  // A valid surface agrees with zLin to roughly 1e-3, so a 2x band is enormously
  // generous and cannot cost legitimate camera blur, while a degenerate
  // reconstruction is off by orders of magnitude and cannot survive it.
  const bool consistent = isfinite(zLin) && (zLin > 0.0) && isfinite(zRec)
                       && (zRec > zLin * 0.5) && (zRec < zLin * 2.0);

  // A rejected pixel falls back to camPx = gamePx, objPx = 0 below, which is the
  // correct answer for sky: it has no object motion, and the streak still follows
  // the pan.
  const float4 prevClip = mul(float4(worldPos, 1.0), prevViewProj_g);
  const bool projectable = consistent
                        && (abs(prevClip.w) > 1e-6) && all(isfinite(prevClip.xy));

  float2 camPx;
  float2 objPx;
  if (projectable) {
    const float2 prevUV = (prevClip.xy / prevClip.w) * float2(0.5, -0.5) + 0.5;
    // mb_camera_sign resolves which of the game's unlabelled v5/v6 varyings is the
    // PREVIOUS clip position, which is an inference from arithmetic order and not
    // something the repo states. It multiplies the difference only; the jitter
    // delta is added with the same sign either way, because the game adds it to
    // whatever signed difference its convention produces.
    //
    // mb_camera_jitter covers the double-count question: if prevViewProj_g already
    // carries the jitter, adding jitterDiff_g inflates the camera term, and since
    // objPx = gamePx - camPx that inflation lands there at full size. Both default
    // to the current behaviour, so leaving them alone changes nothing.
    const float sign = shader_injection_data.mb_camera_sign >= 0.0f ? 1.0f : -1.0f;
    const float jitter = shader_injection_data.mb_camera_jitter > 0.5f ? 1.0f : 0.0f;
    camPx = (uv - prevUV) * sign * float2(motionDims) + jitterDiff_g * jitter;
    objPx = gamePx - camPx;
  } else {
    camPx = gamePx;
    objPx = float2(0.0, 0.0);
  }

  // ── LENGTH is the camera's share; DIRECTION is the true screen motion ───────
  // The streak always follows gamePx = camPx + objPx, the real screen motion, so
  // no setting can point it somewhere the surface is not going. What the pass
  // limits is the LENGTH, by the camera's fraction of the total magnitude.
  //
  // Summing the two vectors instead would have let each term steer the streak's
  // DIRECTION, so on a follow camera where cam and obj very largely cancel the
  // numbers read backwards. Fixing the direction to gamePx and scaling only the
  // magnitude is what makes "drop the object motion" actually shorten the streak.
  const float camLen = length(camPx);
  const float objLen = length(objPx);
  const float gain = camLen / max(camLen + objLen, 1e-6);
  const float2 blended = gamePx * gain;

  // ── VELOCITY CLAMP, BEFORE THE TILE REDUCTION ───────────────────────────────
  // UE orders this velocity flatten -> clamp -> tile reduction. Clamping only in
  // the gather is too late to matter: TileMax is a per-tile MAXIMUM, so a single
  // garbage pixel seizes its entire tile's velocity. Clamping here, at the last
  // point before TileMax, bounds one bad pixel to one tile of plausible motion.
  //
  // The limit is Max Radius, the SAME bound the gather applied, expressed in the
  // same units by dividing the reference-frame limit back out through
  // mb_frame_scale, so this RELOCATES the clamp rather than tightening the image.
  // The gather clamps the length of the UV-space vector, where x is normalised by
  // the motion width and y by its height, so the equivalent limit in pixels uses
  // the MAGNITUDE of both dimensions. Using one dimension would clamp tighter
  // horizontally than vertically and silently change the image.
  const float maxPx = shader_injection_data.mb_tile_uv
                    * length(float2(motionDims))
                    / max(shader_injection_data.mb_frame_scale, 1e-4);
  const float blendedLen = length(blended);
  const float2 clamped = (blendedLen > maxPx && blendedLen > 1e-6f)
                       ? blended * (maxPx / blendedLen)
                       : blended;
  // .xy is what the tile chain reduces; .zw carries camera-only so the gather's
  // velocity views can read the components without a second reduction. .zw is
  // deliberately NOT clamped: the Camera Velocity view exists to show the camera
  // term, and clamping it here would hide exactly the garbage it is for. The
  // shipping (narrow) variant has no .zw to write.
#if defined(MB_RESOLVE_WIDE)
  g_outMotion[p] = float4(clamped, camPx);
#else
  g_outMotion[p] = clamped;
#endif
}
