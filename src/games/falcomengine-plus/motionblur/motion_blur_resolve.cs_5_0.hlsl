// Motion Blur — camera / object velocity separation.
//
// The game writes ONE motion texture and it is the sum of two different things:
//   * camera motion, from prevViewProj_g vs the current view-projection, and
//   * object motion, from the object's own previous world matrix,
// plus the frame-to-frame jitter delta. A camera pan therefore reports the same
// full-screen velocity as a fast-moving object, and the filter cannot tell them
// apart. That matters for two reasons: the user cannot ask for object blur
// without also getting camera blur, and a camera pan drives tiles onto the
// half-resolution path for motion that is not really the scene's.
//
// The separation is a per-pixel reprojection. It cannot live in TileMax, because
// TileMax is a REDUCTION (one velocity out per tile) and the reprojection needs
// this pixel's own depth. So it gets its own pass, and it feeds the rest of the
// chain a single blended motion vector, leaving tilemax2 / neighbormax / vmax /
// the sample ladder / the early-out / the half-res routing all untouched.
//
// ── WHY THIS REPRODUCES THE GAME'S OWN MATH ──────────────────────────────────
// This is not an independent derivation. sora2nd/foliage/staticfoliage_
// 0xF1EC53A8.ps_5_0.hlsl:201-207 computes, for STATIC foliage, exactly the
// camera-only term:
//
//   r0.xy = v5.xy / v5.ww                              // previous clip
//   r0.xy = r0.xy * float2(0.5,-0.5) + float2(0.5,0.5)  // NDC -> UV
//   r0.xy = vpSize_g.xy * r0.xy                        // -> pixels
//   r0.zw = v6.xy / v6.ww                              // current clip
//   r0.zw = r0.zw * float2(0.5,-0.5) + float2(0.5,0.5)
//   r0.xy = r0.zw * vpSize_g.xy + -r0.xy               // pixels: cur - prev
//   o4.xy = jitterDiff_g.xy + r0.xy                    // + jitter delta
//
// For static geometry the object term is zero, so that expression IS camera
// velocity. Here the previous clip position comes from prevViewProj_g applied to
// the world point reconstructed from THIS pixel, which is the same quantity for a
// static surface, and the NDC->UV step and the jitter add are copied verbatim.
//
// ── MATRIX CONVENTION — READ BEFORE TOUCHING THE TWO mul() CALLS ──────────────
// The engine's matrices are ROW-MAJOR and are applied as ROW-VECTOR products, so
// every transform is mul(VECTOR, MATRIX). Getting this backwards is silent and
// catastrophic: it produced a whirlpool at screen centre with the whole frame
// blurring into it. The authority is the game's own reprojection at
// sora2nd/ssr/ssr2_0x17F931DE:156-164, which divides the xy by the result of
// dotting with _m03_m13_m23_m33 -- _mRC selects a COLUMN, so that is column 3,
// i.e. w = sum_j v_j * M[j][3] = [v * M]. See the comment at the call site.
//
// The unprojection uses the engine's own viewProjInv_g (c20) rather than
// composing viewInv_g (c4) with projInv_g (c12). Composing would require knowing
// the product order for a layout whose convention is exactly the thing being
// gotten wrong; the product is already supplied.
//
// ── WHY DEFAULTS ARE EXACT ───────────────────────────────────────────────────
//   out = camera*cw + object*ow,  object = game - camera
// With cw == ow == 1 this is camera + (game - camera) == game, bit for bit, so
// the CPU skips this pass entirely and binds the game motion directly. The
// pre-separation image is then reproduced without this shader running at all.
// This also holds in EVERY configuration, not just at 1: the two channels are
// exact complements ALGEBRAICALLY, and that is a trap worth stating plainly: it
// does NOT mean the two are independently safe. objectPx = gamePx - camPx means
// every error in the camera term is inherited by the object term at FULL size, with
// the sign flipped. An unbounded camera error on even a few percent of the frame
// therefore becomes unbounded object motion, and TileMax's per-tile max then
// amplifies a single bad pixel into a fully blurred tile. That is precisely how a
// garbage sky reconstruction leaked camera motion into Object Only. The camera
// term must be validated on its own; see the self-validation note at the call site.
//
// ── JITTER ───────────────────────────────────────────────────────────────────
// jitterDiff_g is added because the game adds it, and because object = game -
// camera subtracts two terms carrying the SAME jitter, so the jitter cancels in
// the object component regardless of whether prevViewProj_g is jitter-inclusive.
// It therefore only affects how much is ATTRIBUTED to camera, never object
// correctness. It is sub-pixel by construction (it is a per-frame jitter delta),
// and the gather's early-out is 0.5px, so a misattribution sits below the
// threshold where it could be visible. A still camera and scene with the Object
// Velocity view is the readout: it must be black.

#include "motion_blur_common.hlsli"

cbuffer cb_scene : register(b0)
{
  // proj_g and view_g are used ONLY to cross-check the reconstruction against the
  // gather's own depth linearization, which is the formula proven at
  // motion_blur_gather.cs_5_0.hlsl:133-134. Same matrices, same scalars, so the
  // two routes to a view-space distance are directly comparable.
  float4x4 view_g        : packoffset(c0);
  float4x4 proj_g        : packoffset(c8);
  // The ENGINE's own screen->world inverse (staticfoliage_0xF1EC53A8:10). Use it
  // rather than composing viewInv_g (c4) with projInv_g (c12): the composition
  // order is a guess about a row-major layout, and the product is already here.
  float4x4 viewProjInv_g : packoffset(c20);
  // Sora 2nd offsets, from the game's own staticfoliage shader lines 50-51.
  float4x4 prevViewProj_g : packoffset(c75);
  float2  jitterDiff_g    : packoffset(c79);
};

Texture2D<float2>   g_srcMotion : register(t0);
Texture2D<float>    g_srcDepth  : register(t1);
RWTexture2D<float4> g_outMotion : register(u0);

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
    g_outMotion[p] = float4(0.0, 0.0, 0.0, 0.0);
    return;
  }

  const float2 uv = (float2(p) + 0.5) / float2(motionDims);
  const int2 depthDims = max(int2(shader_injection_data.mb_depth_w,
                                  shader_injection_data.mb_depth_h), int2(1, 1));
  const float depth = g_srcDepth[clamp(int2(uv * float2(depthDims)),
                                       int2(0, 0), depthDims - 1)];

  // Screen -> NDC. The y flip is the exact inverse of the game's
  // `ndc * float2(0.5,-0.5) + 0.5` step above, and matches the convention the
  // motion buffer is written in.
  //
  // ── mul(VECTOR, MATRIX), NOT mul(matrix, vector) ────────────────────────────
  // The engine stores its matrices ROW-MAJOR and applies them as row-vector
  // products. The proof is the game's own reprojection at
  // sora2nd/ssr/ssr2_0x17F931DE:156-164:
  //
  //   r1.x = dot(r0.xyzw, ssrPrevViewProj_g._m00_m10_m20_m30);
  //   r1.y = dot(r0.xyzw, ssrPrevViewProj_g._m01_m11_m21_m31);
  //   r0.x = dot(r0.xyzw, ssrPrevViewProj_g._m03_m13_m23_m33);
  //   r0.xy = r1.xy / r0.xx;                       // <- that one is the divide by w
  //
  // _mRC selects a COLUMN, so _m03_m13_m23_m33 is column 3, and dividing by it
  // makes it w = sum_j v_j * M[j][3]. That is [v * M], not [M * v].
  //
  // Writing mul(matrix, vector) transposes the projection. w stays finite while
  // the xy terms collapse toward a point, so prevUV collapses toward screen
  // centre and every pixel receives a motion vector pointing inward with
  // magnitude proportional to its distance from the centre. That produced a
  // whirlpool in the middle of the frame with everything blurring into it, in
  // BOTH the camera and the object component -- the object view inherits the exact
  // negation via objectPx = gamePx - camPx, which is why the two matched.
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
  // position. The resulting camera motion is finite but meaningless, and
  // objPx = gamePx - camPx turned that into enormous OBJECT motion, which
  // TileMax's per-tile max then amplified into visible blur during a camera pan
  // with Object Only. For exactly those pixels addC - depth underflows, so zLin
  // goes non-finite and the pixel is rejected.
  float mulC = -proj_g[3][2];
  float addC =  proj_g[2][2];
  // Same sign correction the gather applies (motion_blur_gather.cs_5_0.hlsl:135).
  // Without it zLin comes out NEGATIVE for this projection and the zLin > 0 test
  // below would reject every pixel, silently disabling object blur entirely.
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
  // generous and cannot cost legitimate object blur, while a degenerate
  // reconstruction is off by orders of magnitude and cannot survive it.
  const bool consistent = isfinite(zLin) && (zLin > 0.0) && isfinite(zRec)
                       && (zRec > zLin * 0.5) && (zRec < zLin * 2.0);

  // A rejected pixel falls back to camPx = gamePx, objPx = 0 below, which is the
  // correct answer for sky: it has no object motion, and in Camera Only it still
  // blurs with the pan.
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
    // objectPx = gamePx - camPx that inflation lands in the OBJECT channel at full
    // size. Both default to the current behaviour, so leaving them alone changes
    // nothing.
    const float sign = shader_injection_data.mb_camera_sign >= 0.0f ? 1.0f : -1.0f;
    const float jitter = shader_injection_data.mb_camera_jitter > 0.5f ? 1.0f : 0.0f;
    camPx = (uv - prevUV) * sign * float2(motionDims) + jitterDiff_g * jitter;
    objPx = gamePx - camPx;
  } else {
    camPx = gamePx;
    objPx = float2(0.0, 0.0);
  }

  // ── Direction is fixed; the channels own the LENGTH ────────────────────────
  // Summing the two channel vectors gave each label control of the streak's
  // DIRECTION rather than its length, so the numbers read backwards. On a follow
  // camera cam and obj very largely cancel, which is why the character looks
  // sharp at 1/1: the streak there is the small TRUE total. |cam| on its own is
  // large. Dropping Object to 0 therefore swapped the streak onto the longer
  // camera vector and produced MORE smear -- the exact opposite of the label.
  //
  // Here the streak always follows gamePx = camPx + objPx, the real screen
  // motion, and each channel contributes only its share of the LENGTH. No slider
  // can point the streak somewhere the surface is not actually going.
  //
  // At both weights 1 the ratio is exactly 1 (a/b over max(a+b, eps), with
  // a+b >= eps whenever gamePx is non-zero) and gamePx * 1 == gamePx, so the
  // pre-separation result is reproduced bit for bit and the CPU can still skip
  // this pass entirely.
  const float camLen = length(camPx);
  const float objLen = length(objPx);
  const float contrib = camLen * max(shader_injection_data.mb_camera_weight, 0.0)
                       + objLen * max(shader_injection_data.mb_object_weight, 0.0);
  const float gain = contrib / max(camLen + objLen, 1e-6);
  const float2 blended = gamePx * gain;
  // .xy is what the tile chain reduces; .zw carries camera-only so the gather's
  // velocity views can read the components without a second reduction.
  g_outMotion[p] = float4(blended, camPx);
}
