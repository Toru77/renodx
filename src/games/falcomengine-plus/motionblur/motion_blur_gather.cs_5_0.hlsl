// Motion Blur P3 — the Guertin gather.
//
// Paper Appendix A, implemented in UV space (see motion_blur_common.hlsli for
// the unit derivation). What the paper contributes over single-velocity
// filtering, and what is present here:
//
//   * multi-direction sampling: samples alternate between the tile dominant
//     velocity vmax and the per-pixel composite direction vc, so pixels whose
//     own motion differs from the neighbourhood still contribute (Section 4.1)
//   * vc = rnmix(vp_perp, norm(vp), (|vp| - 0.5)/g), i.e. as a pixel's own
//     velocity fades out it is replaced by the direction perpendicular to
//     vmax rather than wasting its sample budget (Eq. 1)
//   * feature-aware weights: each of the three phenomenological cases is
//     additionally weighted by the dot product between the sampling direction
//     and the velocity direction at the sample (Section 4.1)
//   * the mid-point sample is NOT discarded and the centre weight is
//     normalised, so thin features survive and the weight does not drift with
//     the sample count (Section 4.3)
//   * stochastic on-axis tile lookup near tile borders (Section 4.2), which
//     MBNeighborTile resolves
//   * deterministic Halton jitter of the integration domain (Section 4.5)
//
// Both inputs are the game's own textures, read directly: motion at t1 and depth
// at t3, with no full-resolution intermediate in between. Depth is linearized
// inline because cb_scene is already bound at b0 for every stage of this chain;
// that costs one reciprocal per sample against 25 texture fetches already being
// issued, and it removes a 14.7 MB per-frame write at 1440p. The gather
// point-samples either way, so the result is identical.
//
// Deliberately excluded, both sanctioned by the paper itself:
//   * TileVariance sample distribution (Section 4.1) — the paper disables it
//     in its own Section 5 results
//   * the (px+py)&1 FXAA luminance hint (Section 4.5) — this filter runs after
//     FXAA, so there is no downstream edge detector to consume it
//
// Output alpha is the CENTRE sample's alpha, so the gather never disturbs
// whatever the source encodes there.

#include "motion_blur_common.hlsli"

cbuffer cb_scene : register(b0)
{
  float4x4 view_g    : packoffset(c0);
  float4x4 viewInv_g : packoffset(c4);
  float4x4 proj_g    : packoffset(c8);
  float4x4 projInv_g : packoffset(c12);
};

Texture2D<float4>    g_srcColor       : register(t0);
// float4, not float2: motion_blur_resolve writes .xy = camera/object blend and
// .zw = camera-only. TileMax still declares float2 over the same resource and
// takes .xy. When the resolve pass is skipped this is the game's own motion
// texture, where .zw is unused.
Texture2D<float4>    g_srcMotion      : register(t1);
Texture2D<float2>    g_srcNeighborMax : register(t2);
// Single-channel, matching the game's other depth readers (gtvbao, ssrr2,
// lighting). Declaring this float2 made `addC - g_srcDepth[texel]` a float2
// silently truncated to .x, which is the same number but is a real type
// mismatch the compiler rightly flagged.
Texture2D<float>     g_srcDepth       : register(t3);
Texture3D<float2>    g_isfastNoise    : register(t4);
// The game's UNRESOLVED motion, bound beside the resolved one. Read only by the
// Camera/Object Velocity views, which return before the tap loop, so the filter
// itself never issues a load against it.
Texture2D<float2>    g_srcGameMotion  : register(t5);
RWTexture2D<float4>  g_outColor       : register(u0);

// Point lookup: a tap is a real pixel, not a filtered one. Also keeps depth
// reads off silhouette interpolations.
float SampleLinearDepth(float2 uv, int2 dims, float mulC, float addC) {
  int2 texel = clamp(int2(uv * float2(dims)), int2(0, 0), dims - 1);
  float denom = addC - g_srcDepth[texel];
  float z = (abs(denom) > 1e-8) ? (mulC / denom) : 0.0;
  z = max(z, 0.0);
  // Cleared/sky depth linearizes to 0 or garbage depending on the projection;
  // clamping to a finite positive distance keeps zCompare well defined.
  if (!isfinite(z)) z = 0.0;
  return min(z, 1.0e9);
}

float2 SampleJitter(int2 p) {
  if (shader_injection_data.mb_jitter_source > 0.5 && shader_injection_data.mb_jitter_ready > 0.5) {
    // IS-FAST blue-noise volume: temporally varying, so the residual sampling
    // noise reads far less than a static per-pixel Halton pattern. This matters
    // more than usual here because motion blur is applied ON TOP of depth of
    // field, so the paper's "DoF blurs our noise" benefit is unavailable.
    // mb_jitter_ready is the runtime availability flag, so the user's choice in
    // mb_jitter_source is never overwritten.
    int3 c = int3(int(p.x) & 127, int(p.y) & 127, int(shader_injection_data.mb_frame_index) & 31);
    return frac(g_isfastNoise.Load(int4(c, 0)));
  }
  return MBJitterHalton(p);
}

[numthreads(8, 8, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID) {
  int2 p = int2(dispatchThreadID.xy);
  int workingW = (int)shader_injection_data.mb_working_w;
  int workingH = (int)shader_injection_data.mb_working_h;
  if (workingW <= 0 || workingH <= 0) return;
  if (p.x >= workingW || p.y >= workingH) return;

  int2 depthDims = max(int2(shader_injection_data.mb_depth_w, shader_injection_data.mb_depth_h), int2(1, 1));
  int2 motionDims = max(int2(shader_injection_data.mb_motion_w, shader_injection_data.mb_motion_h), int2(1, 1));
  int2 tiles = max(int2(shader_injection_data.mb_tiles_x,
                        shader_injection_data.mb_tiles_y), int2(1, 1));
  float tileUV = max(shader_injection_data.mb_tile_uv, 1e-6);
  bool motionValid = shader_injection_data.mb_motion_valid > 0.5;

  // Loop invariants, hoisted explicitly rather than left to the compiler. The
  // per-pixel motion weight is mandatory: dropping it was measured to cost
  // visible foreground bleeding, so it is no longer a quality/perf trade.
  const bool localWeights = shader_injection_data.mb_local_velocity_weights > 0.5;
  // Off drops the per-tap depth fetch and both cone terms, leaving the cylinder
  // term alone. Uniform across the dispatch, so the branch costs no divergence.
  const bool depthTest = shader_injection_data.mb_depth_test > 0.5;
  // Split-resolution routing. Inactive when Half Resolution is off, in which case
  // this one full-res dispatch owns all motion and the chain is the same four
  // dispatches as before. When it is on, the shader is dispatched TWICE with
  // mb_gather_side flipped: side 0 owns short motion at full res, side 1 owns
  // long motion at half res.
  const bool splitActive = shader_injection_data.mb_halfres > 0.5;
  const bool ownsLong = shader_injection_data.mb_gather_side > 0.5;
  const float halfResUV = max(shader_injection_data.mb_halfres_px, 0.0) / MB_REF_H;
  const float2 workingF = float2(workingW, workingH);
  const int2 workingMax = int2(workingW - 1, workingH - 1);
  const float2 motionF = float2(motionDims);
  const int2 motionMax = motionDims - 1;

  // Paper pixel constants expressed in UV (see motion_blur_common.hlsli).
  // jitterUV carries mb_frame_scale so its RATIO to |vmax| -- which is what
  // actually determines the look -- stays constant across framerates. |vmax|
  // arrives from neighbormax already scaled, so it must not be scaled again.
  float minVelocityUV = 0.5 / MB_REF_H;
  float jitterUV = clamp(shader_injection_data.mb_jitter_h, 0.0, 4.0) / MB_REF_H
                 * max(shader_injection_data.mb_frame_scale, 0.0);
  float gUV = max(shader_injection_data.mb_min_velocity_g, 1e-4) / MB_REF_H;
  float kUV = max(shader_injection_data.mb_center_weight_k, 1e-3) * MB_REF_H;

  // Depth linearization constants, hoisted out of the sample loop.
  float mulC = -proj_g[3][2];
  float addC =  proj_g[2][2];
  if (mulC * addC < 0.0) addC = -addC;

  float2 uv = (float2(p) + 0.5) / float2(workingW, workingH);
  float4 centerSample = g_srcColor[p];

  int debugView = (int)(shader_injection_data.mb_debug_view + 0.5);

  // One routing decision, shared by the Half Res Detect view and the reject
  // below. Guarded so the full-resolution path -- the default -- does not pay for
  // a fetch it has never paid for: the decision is only needed when the split is
  // active or when that view is being drawn, and both are uniform across the
  // dispatch, so the branch costs nothing.
  const bool needRouting = splitActive || (debugView == 7);
  float2 routingVMax = float2(0.0, 0.0);
  if (needRouting) {
    routingVMax = motionValid ? g_srcNeighborMax[MBRoutingTile(uv)] : float2(0.0, 0.0);
  }
  const bool longMotion = length(routingVMax) >= halfResUV;

  if (debugView == 7) {
    // Must come BEFORE the reject below, which discards the pixels this dispatch
    // does not own and would never reach a debug view otherwise.
    //
    // Grey marks a tile that will not blur at all (below the 0.5px early-out), so
    // "slow but full res" is distinguishable from "not moving". That test uses the
    // ROUTING vmax rather than the jittered filter vmax: the partition is per-tile,
    // the two differ by at most one tile, and using the routing value avoids the
    // 3D noise fetch the jittered lookup would otherwise need here.
    const bool blursAtAll = motionValid && (length(routingVMax) > minVelocityUV);
    float3 detect = !splitActive ? float3(0.0, 0.0, 1.0)       // split disabled
                  : !blursAtAll  ? float3(0.25, 0.25, 0.25)  // too slow to blur
                  : longMotion   ? float3(1.0, 0.0, 0.0)       // half resolution
                                 : float3(0.0, 1.0, 0.0);      // full resolution
    g_outColor[p] = float4(detect, 1.0);
    return;
  }

  // Routing happens here, before the noise load, so a pixel the other gather owns
  // costs two fetches (colour + routing) instead of three.
  if (splitActive && (longMotion != ownsLong)) {
    // Not this dispatch's motion class. Always write, and write the untouched
    // source, so that a pixel the composite misjudges at a tile boundary shows
    // an unblurred frame rather than stale data from a previous one.
    g_outColor[p] = centerSample;
    return;
  }

  float2 jitter = SampleJitter(p);

  int2 neighborTile = MBNeighborTile(uv, jitter);
  // An unavailable motion buffer must not be allowed to read the 1x1 fallback
  // stand-in, which would look like a uniform velocity everywhere. Zeroing vmax
  // makes every pixel take the early-out below, i.e. a passthrough.
  //
  // Point Load, not SampleLevel: this is the filter's only input, and it must
  // not depend on a sampler's descriptor being valid.
  float2 vmax = motionValid ? g_srcNeighborMax[neighborTile] : float2(0.0, 0.0);
  float vmaxLength = length(vmax);
  const bool blurred = motionValid && (vmaxLength > minVelocityUV);

  // Adaptive ladder. N comes from the bucket, bounded by Max Samples. Keyed to
  // |vmax| in absolute UV, so it does not move when Max Radius does.
  int bucket = blurred ? MBSampleBucket(vmaxLength) : 0;

  // ---- debug views (no filter output; alpha 1) ----
  if (debugView == 6) {
    g_outColor[p] = float4(MBBucketColor(bucket), 1.0);
    return;
  }
  if (debugView == 1) {
    // Gated on motionValid, which reflects the BOUND view, never on `blurred`.
    // `blurred` is a filter outcome: gating a motion diagnostic on it makes a
    // TileMax/NeighborMax fault look exactly like a dead motion buffer, which is
    // precisely the misdiagnosis this view caused once already.
    float2 centerVelocity = motionValid
        ? MBGameMotionToUV(g_srcMotion[clamp(int2(uv * float2(motionDims)), 0, motionDims - 1)].xy)
        : float2(0.0, 0.0);
    float centerLength = length(centerVelocity);
    float2 dir = (centerLength > 1e-8) ? centerVelocity / centerLength : float2(0.0, 0.0);
    g_outColor[p] = float4(float3(saturate(centerLength / tileUV), dir * 0.5 + 0.5), 1.0);
    return;
  }
  if (debugView == 2) {
    float2 dir = (vmaxLength > 1e-8) ? vmax / vmaxLength : float2(0.0, 0.0);
    g_outColor[p] = float4(float3(saturate(vmaxLength / tileUV), dir * 0.5 + 0.5), 1.0);
    return;
  }
  if (debugView == 3) {
    // 1 unit of the neighbormax grid is 1 texel, so the distance to the
    // nearest tile border is directly readable in texels.
    float2 pos = frac(uv * float2(tiles));
    float2 distToBorder = min(pos, 1.0 - pos);
    float edge = 1.0 - smoothstep(0.0, 1.5, min(distToBorder.x, distToBorder.y));
    g_outColor[p] = float4(edge.xxx, 1.0);
    return;
  }
  if (debugView == 4) {
    g_outColor[p] = float4(saturate(SampleLinearDepth(uv, depthDims, mulC, addC) * 0.01).xxx, 1.0);
    return;
  }
  if (debugView == 5) {
    g_outColor[p] = float4(saturate(vmaxLength / tileUV).xxx, 1.0);
    return;
  }
  if (debugView == 8 || debugView == 9) {
    // 8 = camera only, 9 = object only. The CPU forces the resolve pass on for
    // these two, so .zw really is camera velocity and not whatever the game
    // texture happened to carry there.
    //
    // Object is the DIFFERENCE against the game's own motion, which is what makes
    // it the decisive test: the jitter delta is present in both terms, so it
    // cancels here and the view must read pure black for a static camera AND a
    // static scene. A structured ~0.5-1px residual means the resolve pass is
    // adding jitterDiff_g on top of a prevViewProj_g that already carries it.
    float2 px = float2(0.0, 0.0);
    if (motionValid) {
      const int2 mtexel = clamp(int2(uv * float2(motionDims)), 0, motionDims - 1);
      const float2 cameraPx = g_srcMotion[mtexel].zw;
      px = (debugView == 8) ? cameraPx : (g_srcGameMotion[mtexel] - cameraPx);
    }
    float2 vel = MBGameMotionToUV(px);
    float len = length(vel);
    float2 dir = (len > 1e-8) ? vel / len : float2(0.0, 0.0);
    g_outColor[p] = float4(float3(saturate(len / tileUV), dir * 0.5 + 0.5), 1.0);
    return;
  }

  if (debugView == 10) {
    // Object Residual: on a STATIC scene the object channel must be zero, because
    // the game's motion is then entirely camera motion. So this view is a pure
    // error readout for the camera term -- objPx = gamePx - camPx means every
    // camera error lands here at FULL size, negated, rather than being averaged
    // away. A clean mirror of the Camera Velocity view in this one means the
    // camera estimate is inverted or mis-scaled, not that object motion exists.
    //
    // Full scale is minVelocityUV, the gather's own 0.5px early-out, so this asks
    // exactly the question the filter asks: anything bright here WILL blur. Black
    // means it will not. That makes the Camera Direction / Camera Jitter
    // combination self-determining without judging hues.
    float2 px = float2(0.0, 0.0);
    if (motionValid) {
      const int2 mtexel = clamp(int2(uv * float2(motionDims)), 0, motionDims - 1);
      px = g_srcGameMotion[mtexel] - g_srcMotion[mtexel].zw;
    }
    g_outColor[p] = float4(saturate(length(MBGameMotionToUV(px))
                                   / max(minVelocityUV, 1e-8)).xxx, 1.0);
    return;
  }

  // Paper early-out: nothing is moving here, or the motion buffer is unusable.
  if (!blurred) {
    g_outColor[p] = centerSample;
    return;
  }

  // ---- Section 4.1: two sampling directions ----
  float2 wn = vmax / vmaxLength;
    float2 centerVelocity = MBGameMotionToUV(
        g_srcMotion[clamp(int2(uv * float2(motionDims)), 0, motionDims - 1)].xy);
  float centerLength = length(centerVelocity);
  float2 wp = float2(-wn.y, wn.x);
  if (dot(wp, centerVelocity) < 0.0) wp = -wp;  // Appendix A sign flip
  float2 vc = MBRNMix(wp, MBNorm(centerVelocity), saturate((centerLength - minVelocityUV) / gUV));

  // Appendix A seeds the accumulator with
  //     totalWeight = N / (k * |vc|),  result = color[p] * totalWeight
  // where the pseudocode's `vc` is V[p], the PER-PIXEL velocity, not the
  // composite direction (that one is `wc`). The centre sample is therefore a
  // vanishing fraction of the total and its job is only to anchor the
  // degenerate all-weights-equal case.
  //
  // The denominator is floored at the paper's own minimum-velocity threshold.
  // Eq. 1 already treats a pixel below that as having no velocity of its own, so
  // without the floor a pixel reading exactly zero would make N/(k*0) enormous
  // and pin itself to its original colour, killing the very transparency effect
  // Section 4.2 is about.
  //
  // NOTE: paper Section 4.3 describes the centre weight as `w_p = |v|*k + N/k`
  // and argues the single-velocity form was under-weighted, which does not
  // reconcile numerically with the Appendix A expression above (they differ by
  // orders of magnitude). Appendix A is the executable specification and is
  // what is implemented; if thin features ghost in practice, this is the line
  // to revisit against the original PDF.
  // Adaptive ladder: the neighbourhood velocity already says how much smear this
  // pixel needs, so taps scale with it instead of being flat. `maxSamples` is a
  // CEILING, not the count. The [loop] attribute below is load-bearing: it stops
  // the compiler unrolling a now-dynamic bound, and divergence stays limited
  // because buckets are tile-coherent (see MBSampleBucket).
  const int maxSamples = clamp((int)(shader_injection_data.mb_sample_count + 0.5), 4, 48);
  const int sampleCount = (int)MBSampleCount(bucket, (uint)maxSamples);
  float totalWeight = (float)sampleCount
                    / max(kUV * max(centerLength, minVelocityUV), 1e-8);
  float3 result = centerSample.rgb * totalWeight;
  // Only fetched when the depth test will actually consume it. MBZCompare is
  // multiplied by mb_depth_tolerance, so a very low tolerance already scales both
  // cone terms down to near nothing while still paying one depth load per tap.
  const float centerDepth = depthTest ? SampleLinearDepth(uv, depthDims, mulC, addC) : 0.0;

  // Section 4.5: Halton-jittered stratified integration. h extends the domain
  // slightly past |vmax| (the paper's "larger maximum jitter value").
  //
  // r bounds the TILE search, not the integration length: the paper's domain is
  // |vmax| + h. Clamping the scaled domain to tileUV is what made the Intensity
  // slider inert, because |v| is already clamped to tileUV, so any motion at or
  // above Max Radius saturated the product. Clamp to ONE TILE instead, which is
  // the actual constraint the 1-ring property imposes.
  float maxTileUV = 1.0 / float(max(tiles.y, 1));
  float maxT = min((vmaxLength + jitterUV) * max(shader_injection_data.mb_intensity, 0.0), maxTileUV);

  [loop]
  for (int i = 0; i < sampleCount; ++i) {
    // lerp, not mix: mix is a pixel-stage intrinsic and this is a compute shader.
    float t = lerp(-1.0, 1.0, ((float)i + jitter.x + 1.0) / (float)(sampleCount + 1));
    float T = t * maxT;
    float2 d = (i & 1) ? vc : wn;  // even samples follow vmax, odd follow vc

    float2 sampleUV = clamp(uv + T * d, 0.0, 1.0);
    // wB is the paper's local-velocity term: it asks whether the motion AT THIS
    // TAP agrees with the sampling direction, which is what stops foreground
    // bleeding across a depth edge. Measured to be necessary for image quality,
    // so it stays on by default and the toggle is only a future optimisation hook.
    float wA = dot(vc, d);
    float wB = wA;
    float sampleLength = centerLength;
    if (localWeights) {
      int2 motionTexel = clamp(int2(sampleUV * motionF), int2(0, 0), motionMax);
        float2 sampleVelocity = MBGameMotionToUV(g_srcMotion[motionTexel].xy);
      wB = dot(MBNorm(sampleVelocity), d);
      sampleLength = length(sampleVelocity);
    }

    // The three phenomenological cases, each additionally weighted by how well
    // the local velocity direction agrees with the sampling direction.
    //
    // MBConeByLength takes the velocity directly: the old form built 1/L and then
    // divided by it, spending two divides per cone term for a product.
    float weight = MBCylinder(T, min(sampleLength, centerLength)) * max(wA, wB) * 2.0;
    if (depthTest) {
      // Fore/background classification relative to p. zCompare is symmetric, so f
      // and b coincide; the paper's distinction is carried by the two cone
      // half-widths and by wA vs wB above.
      float depthAgree = MBZCompare(centerDepth, SampleLinearDepth(sampleUV, depthDims, mulC, addC));
      weight += depthAgree * MBConeByLength(T, sampleLength) * max(wB, 0.0)
              + depthAgree * MBConeByLength(T, centerLength) * max(wA, 0.0);
    }

    // Colour is the most expensive fetch in the loop (8 B against 4 B for motion
    // and depth), so it is deferred until the weight is known. A tap rejected by
    // the depth test has weight 0 and contributes nothing, and across a
    // high-contrast edge that is a large share of them. Identical result for
    // finite colour, and strictly safer than before: the unconditional form
    // evaluated colour * 0, which is NaN for a non-finite texel and would poison
    // the whole pixel. Divergence limits the win, since a warp with a mix of
    // accepted and rejected lanes still pays for the fetch.
    weight = max(weight, 0.0);
    if (weight > 0.0) {
      totalWeight += weight;
      result += g_srcColor[clamp(int2(sampleUV * workingF), int2(0, 0), workingMax)].rgb * weight;
    }
  }

  result = (totalWeight > 1e-6) ? result / totalWeight : centerSample.rgb;
  // Every dispatched pixel is written, so the two gathers together always cover
  // the frame and the composite never reads a stale texel.
  g_outColor[p] = float4(clamp(result, 0.0, 65472.0), centerSample.a);
}
