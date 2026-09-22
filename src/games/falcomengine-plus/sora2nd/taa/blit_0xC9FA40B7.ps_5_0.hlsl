// Custom temporal-upsampling blit for Sora 2nd (game hash 0xC9FA40B7).
//
// v1 TAAU (temporal upsampling), Sora2nd only. Replaces the vanilla bilinear
// blit, which merely resamples the input-resolution TAA output onto the
// output-resolution target. Vanilla file sora2nd/taa/blit_0xC9FA40B7.ps_5_0.hlsl
// stays untouched as reference.
//
// Pipeline position and contracts:
// - Input t0: input-resolution TAA output, no HUD (verified in DevKit).
// - Output o0: output-resolution target, alpha forced to 1 (vanilla contract).
//   UI renders in later draws and is never touched here.
// - The single render target CANNOT hold both the sharpened display image and
//   the unsharpened accumulation, so the latter is written to histUAV (owned
//   output-resolution history, bound via push in on_draw). Sharpened pixels
//   therefore never enter history — same isolation principle as the former
//   RCAS compute stage, without its extra dispatch and copies.
//
// v1 scope (deliberate):
// - Input TAA temporal accumulation is BYPASSED while the upscaler runs (see
//   taa_bypass_accumulation): this blit consumes the raw jittered low-res
//   current frame, and the output-resolution history below is the SOLE
//   temporal accumulator. No stacked double accumulation.
// - Spatial reconstruction: Full Catmull-Rom (existing validated code path,
//   exact integer loads). No EASU in v1.
// - Output history: Full Catmull-Rom (survey: bilinear history resampling
//   accumulates blur; Catmull-Rom is the standard remedy).
// - Validation: centered 3x3 input-space footprint (see below) with the
//   UNCHANGED k-DOP/AABB clip mathematics.
// - Confidence beta = saturate(maxCombinedBicubicWeight): exact sample -> 1,
//   worst fractional position -> ~0.316. Literature-faithful, no remap, no UI,
//   no taps (weights already computed). updateWeight = (1-feedback)*beta.
// - Feedback/overshoot/silhouette/detail machinery: shared uniforms, reused
//   verbatim (motion scale rescaled by the render ratio, see below).
// - RCAS fused after accumulation via rcas_common.hlsli (single RCAS
//   implementation shared with the legacy compute pass). Ring taps are
//   center-motion-reprojected bilinear history: in converged static areas
//   history == accumulation (near-exact); in motion the limiter contains the
//   mismatch, and the temporally-filtered ring actively suppresses shimmer.
//   A full 5x per-pixel accumulation ring would cost ~1.5ms and is deferred
//   as an upgrade path, not v1.
//
// Coordinate/scale conventions:
// - v1 carries output UVs; q maps to input UV by the dims ratio (shared
//   render-target origin assumption, same as the vanilla blit it replaces).
// - Motion vectors are input-texel units (established); scaled linearly by
//   (outDims/inDims) into output texels. The frame-to-frame jitter delta
//   rides inside the vectors, so NO explicit jitter term is added here
//   (same no-double-compensation rule as custom TAA).
// - Output history lives in stable output UV space (outPrevScale = 1).
//   Resets: upscaler enable transition + output-dims change. Input-res
//   changes need no reset (footprint/stride/beta all derive per-frame).
// - At 100% render scale this degenerates to native-res temporal
//   accumulation + RCAS (no special case needed).

#include "../../taa/taa_common.hlsli"
#include "../../rcas/rcas_common.hlsli"

// Game bindings (preserved from the vanilla blit).
SamplerState smpl_s : register(s0);  // game sampler (kept for binding parity; custom taps use integer loads)
Texture2D<float4> tex : register(t0);  // input-resolution TAA output, no HUD
// Pushed bindings (bound via push in on_draw; the game binds nothing here).
Texture2D<float4> motionTex : register(t10);  // game motion buffer, input-texel units
Texture2D<float4> depthTex : register(t11);   // input-resolution depth (or white fallback when unavailable)
Texture2D<float4> histTex : register(t12);    // owned output-resolution history SRV
RWTexture2D<float4> histUAV : register(u1);   // owned output-resolution history UAV (unsharpened store)
// Game TAA constant buffer (cb_taa), re-pushed by C++ onto this draw: the
// identical buffer_range the Sora2nd TAA draw bound at b2, same frame. Only
// c0.xy (current-frame jitter, input pixels — sibling of the pixel-unit
// jitterDiff_g added to motion targets) is read. Declared here under its own
// name; the game's blit-time b2 (if any) is irrelevant to this replacement.
cbuffer cb_taa_cap : register(b2) {
  float2 jitter_g : packoffset(c0);
  float2 taaTexelSize_g : packoffset(c0.z);
  float4 taaBlend_g : packoffset(c1);
  float4 taaPrev_g : packoffset(c2);
};
// Game frame constant buffer (cb_scene), re-pushed by C++ onto this draw at
// the private b8 slot (b13 is the framework injection and must stay
// untouched): the identical buffer_range the game bound, same frame. Only
// c8.z (projection X offset) and c9.z (projection Y offset) are read.
cbuffer cb_frame : register(b8) {
  float4 projRowX : packoffset(c8);
  float4 projRowY : packoffset(c9);
};

// Exact bilinear from integer loads (no sampler dependence).
float3 BilinearTap(Texture2D<float4> t, float2 uv, float2 texelSize, int2 maxT) {
  float2 tc = clamp(uv, float2(0.0, 0.0), float2(1.0, 1.0)) / texelSize - 0.5;
  float2 i0 = floor(tc);
  float2 f = tc - i0;
  int2 t00 = clamp(int2(i0), int2(0, 0), maxT);
  int2 t10 = clamp(int2(i0) + int2(1, 0), int2(0, 0), maxT);
  int2 t01 = clamp(int2(i0) + int2(0, 1), int2(0, 0), maxT);
  int2 t11 = clamp(int2(i0) + int2(1, 1), int2(0, 0), maxT);
  float3 c00 = t.Load(int3(t00, 0)).xyz;
  float3 c10 = t.Load(int3(t10, 0)).xyz;
  float3 c01 = t.Load(int3(t01, 0)).xyz;
  float3 c11 = t.Load(int3(t11, 0)).xyz;
  return lerp(lerp(c00, c10, f.x), lerp(c01, c11, f.x), f.y);
}

void main(
    float4 v0 : SV_Position0,
    float2 v1 : TEXCOORD0,
    out float4 o0 : SV_Target0) {
  float2 uv_out = v1.xy;
  int2 pixel = int2(v0.xy);
  float inW = shader_injection_data.upsample_inW;
  float inH = shader_injection_data.upsample_inH;
  float outW = shader_injection_data.upsample_outW;
  float outH = shader_injection_data.upsample_outH;

  // Defensive: degenerate dims -> vanilla copy behavior (never black).
  if (inW < 64.0 || inH < 64.0 || outW < 64.0 || outH < 64.0) {
    float4 fallback = tex.SampleLevel(smpl_s, v1.xy, 0);
    o0 = float4(fallback.xyz, 1.0);
    return;
  }
  float rx = outW / inW;
  float ry = outH / inH;
  float ratioAvg = (rx + ry) * 0.5;
  float2 texelIn = float2(1.0 / inW, 1.0 / inH);
  float2 texelOut = float2(1.0 / outW, 1.0 / outH);
  int2 maxIn = int2((int)(inW + 0.5) - 1, (int)(inH + 0.5) - 1);
  int2 maxOut = int2((int)(outW + 0.5) - 1, (int)(outH + 0.5) - 1);
  // Input UV equals output UV: both span the full image [0,1], so no scaling
  // applies here. (A pixel-unit factor would sample only the top-left
  // inW/outW sub-region and stretch it fullscreen.) Render-scale footprint
  // and motion conversions below still use the dims ratio where units demand
  // it (texel offsets, motion vectors); this line must stay unscaled.
  float2 q = uv_out;

  // --- Spatial bicubic resolve of the jittered low-res current frame ---
  // Exact integer loads (sampler-independent) + analytic weights. The weights
  // additionally feed beta below at zero tap cost.
  float2 tc = q / texelIn - 0.5;
  float2 i0 = floor(tc);
  float2 f = tc - i0;
  float4 wx = CustomTAA_CubicWeights(f.x);
  float4 wy = CustomTAA_CubicWeights(f.y);
  float3 current = float3(0.0, 0.0, 0.0);
  float wmax = 0.0;
  // Raw maximum sample weight feeding the Raw Max confidence experiment
  // below. Tracks wmax (fixed path, then jitter path on success); mode 0
  // never reads it, so Normalized Max behavior is value-identical.
  float wmaxRaw = 0.0;
  [unroll]
  for (int j = 0; j < 4; ++j) {
    [unroll]
    for (int i = 0; i < 4; ++i) {
      int2 t = int2(i0) + int2(i - 1, j - 1);
      t = clamp(t, int2(0, 0), maxIn);
      float w = wx[i] * wy[j];
      current += w * tex.Load(int3(t, 0)).xyz;
      wmax = max(wmax, w);
    }
  }
  // Reconstruction filter option (UpscalerSpatialBilinear): replace ONLY the
  // reconstructed color with bilinear. Weights/wmax/beta, history,
  // reprojection, validation, feedback, RCAS and lifecycle stay identical.
  if (shader_injection_data.upsampler_spatial_bilinear > 0.5) {
    current = BilinearTap(tex, q, texelIn, maxIn);
  }
  wmaxRaw = wmax;  // fixed-path raw max (fallback value when the jitter path is invalid)
  // --- Jitter-aware reconstruction (experiment, UpscalerJitterReconstruction) ---
  // OFF (default): skipped by the uniform branch below; current/wmax above are
  //   untouched, so the baseline path is bit-for-bit identical.
  // ON: current is reconstructed from raw low-res samples at jittered
  //   output-space positions. Kernel selected by Jitter Reconstruction Filter:
  //   Box = overlap-area weights (proven reference); Gaussian = scale-aware
  //   Gaussian on the SAME jittered sample lattice (only the weights differ).
  //   Catmull-Rom above still executes but its result is discarded whenever
  //   the jitter-path accumulation below is valid.
  // Conventions (derived, not guessed):
  // - jitter_g is input pixels (sibling of the pixel-unit jitterDiff_g added
  //   to every motion target); clamped to +-1px (standard TAA jitter bound).
  // - Sign is MINUS: projection-offset jitter shifts the rendered image by +J
  //   pixels, so texel (i,j) imaged stable-scene (i+0.5)-J, i.e. its footprint
  //   in the stable output grid shifts by -jOut where jOut = J*(rx,ry).
  //   SIGN TEST (option b): if ON converges smeared/divergent across jitter
  //   phases while OFF is stable, flip ONLY the "- jOut" expressions below to
  //   "+ jOut" and repeat the same static-scene A/B. Generic softness alone
  //   is NOT a sign failure. Do not touch beta, feedback, validation, radius,
  //   or weights as part of that test.
  if (shader_injection_data.upsampler_jitter_reconstruction > 0.5
      && shader_injection_data.upsampler_jitter_live > 0.5) {
    float2 jitterIn = clamp(jitter_g, float2(-1.0, -1.0), float2(1.0, 1.0));
    if (shader_injection_data.upsampler_jitter_units > 0.5) {
      // NDC->Input Pixels diagnostic: reinterpret b2 jitter_g as NDC offsets
      // scaled by render dims. Applies to the baseline only; the projection
      // source override below still wins when selected (that experiment is
      // untouched). Everything after jitterIn is unchanged.
      float2 jitterInNDC = jitter_g * float2(inW * 0.5, inH * 0.5);
      jitterIn = clamp(jitterInNDC, float2(-1.0, -1.0), float2(1.0, 1.0));
    }
    if (shader_injection_data.upsampler_jitter_source > 0.5) {
      // Projection jitter source (experiment): p8_2/p9_2 are NDC offsets, so
      // image shift in input pixels is offset*dim/2 per axis. Y is negated:
      // D3D NDC +Y points up while our texel convention increases downward.
      // Per-axis signs are runtime-falsifiable (smear-while-baseline-stable
      // on one axis => flip only that axis); see the SIGN TEST note above.
      float2 projJitter = float2(projRowX.z * inW * 0.5, -projRowY.z * inH * 0.5);
      jitterIn = clamp(projJitter, float2(-1.0, -1.0), float2(1.0, 1.0));
    }
    float2 jOut = jitterIn * float2(rx, ry);
    // Anchor: input-continuous texel position imaging this output pixel center.
    float2 anchorC = (float2(pixel) + float2(0.5, 0.5) + jOut) / float2(rx, ry) - float2(0.5, 0.5);
    int2 anchorT = int2(floor(anchorC + float2(0.5, 0.5)));
    int filtMode = (int)(shader_injection_data.upsampler_jitter_reconstruction_filter + 0.5);
    if (filtMode < 1) {
    // --- Box kernel (proven reference; equations unchanged) ---
    // Each low-res input texel is one sample with output-space footprint
    // [i*rx - jOut.x, (i+1)*rx - jOut.x] (same for y). Overlap of that
    // footprint with the output pixel's unit box is the sample weight.
    // Radius covers footprint-half + pixel-half + jitter bound, in input
    // texels, clamped to the static loop bound. rx=2 -> 2 (5x5), rx=1 -> 2,
    // rx~0.67 -> 3 (7x7).
    float minRatio = min(rx, ry);
    int radius = (int)ceil(0.5 + 1.0 + 0.5 / max(minRatio, 1e-4));
    radius = clamp(radius, 1, 3);
    float px0x = float(pixel.x);
    float px1x = px0x + 1.0;
    float px0y = float(pixel.y);
    float px1y = px0y + 1.0;
    float3 jbAcc = float3(0.0, 0.0, 0.0);
    float jbSum = 0.0;
    float jbMax = 0.0;
    [unroll]
    for (int jboy = -3; jboy <= 3; ++jboy) {
      [unroll]
      for (int jbox = -3; jbox <= 3; ++jbox) {
        if (abs(jbox) > radius || abs(jboy) > radius) continue;
        int2 tIdx = clamp(anchorT + int2(jbox, jboy), int2(0, 0), maxIn);
        float sx0 = float(anchorT.x + jbox) * rx - jOut.x;
        float sx1 = sx0 + rx;
        float sy0 = float(anchorT.y + jboy) * ry - jOut.y;
        float sy1 = sy0 + ry;
        float oxw = max(0.0, min(sx1, px1x) - max(sx0, px0x));
        float oyw = max(0.0, min(sy1, px1y) - max(sy0, px0y));
        float jbW = oxw * oyw;
        if (jbW > 0.0) {
          float3 s = tex.Load(int3(tIdx, 0)).xyz;
          jbAcc += jbW * s;
          jbSum += jbW;
          jbMax = max(jbMax, jbW);
        }
      }
    }
    // Invalid (degenerate/poisoned) accumulation keeps the fixed-path current
    // and beta above — never black.
    if (jbSum > 1e-6 && isfinite(dot(jbAcc, jbAcc))) {
      current = jbAcc / jbSum;
      wmax = jbMax / jbSum;
      wmaxRaw = jbMax;
    }
    } else {
    // --- Gaussian kernel (experiment; same jittered lattice as Box) ---
    // Sample center (output px) of texel (i,j) is the center of its box
    // footprint above: ((i+0.5) - J) * ratio — identical -jOut convention.
    // Weight = 2D Gaussian of the center's distance to the output-pixel
    // center, sigma widening as input resolution drops. Raw Load() texels
    // only; no filtered-value chaining, so sample-space behavior is kept.
    float sigmaX = max(0.5, 0.5 * rx);
    float sigmaY = max(0.5, 0.5 * ry);
    float invVarX = 1.0 / max(sigmaX * sigmaX, 1e-6);
    float invVarY = 1.0 / max(sigmaY * sigmaY, 1e-6);
    // ~3-sigma support from the larger axis, clamped to the static loop
    // bound. rx=1 -> 2 (5x5), rx=1.5/2 -> 3 (7x7).
    int gradius = clamp((int)ceil(3.0 * max(sigmaX, sigmaY)), 1, 3);
    float jgPcx = float(pixel.x) + 0.5;
    float jgPcy = float(pixel.y) + 0.5;
    float3 jgAcc = float3(0.0, 0.0, 0.0);
    float jgSum = 0.0;
    float jgMax = 0.0;
    [unroll]
    for (int jgy = -3; jgy <= 3; ++jgy) {
      [unroll]
      for (int jgx = -3; jgx <= 3; ++jgx) {
        if (abs(jgx) > gradius || abs(jgy) > gradius) continue;
        int2 jgT = clamp(anchorT + int2(jgx, jgy), int2(0, 0), maxIn);
        float jgScx = (float(anchorT.x + jgx) + 0.5) * rx - jOut.x;
        float jgScy = (float(anchorT.y + jgy) + 0.5) * ry - jOut.y;
        float jgDx = jgScx - jgPcx;
        float jgDy = jgScy - jgPcy;
        float jgW = exp(-0.5 * (jgDx * jgDx * invVarX + jgDy * jgDy * invVarY));
        float3 jgS = tex.Load(int3(jgT, 0)).xyz;
        jgAcc += jgW * jgS;
        jgSum += jgW;
        jgMax = max(jgMax, jgW);
      }
    }
    // Same fallback contract as Box: degenerate/poisoned accumulation keeps
    // the fixed-path current and beta — never black.
    if (jgSum > 1e-6 && isfinite(dot(jgAcc, jgAcc))) {
      current = jgAcc / jgSum;
      wmax = jgMax / jgSum;
      wmaxRaw = jgMax;
    }
    }
  }
  // Reconstruction confidence: exact sample -> 1, worst fractional corner ->
  // ~0.316. Literature-faithful (no remap): strongly-supported pixels update
  // history firmly, interpolated pixels mostly retain it.
  float beta = saturate(wmax);
  if (shader_injection_data.upsampler_reconstruction_confidence > 0.5) {
    // Raw Max experiment: raw maximum sample weight, unclamped, unremapped.
    // Mode 0 above is untouched (Normalized Max, byte-identical behavior).
    beta = wmaxRaw;
  }

  // --- Depth pick + motion (input scale, closest-depth convention) ---
  int2 qc = clamp(int2(i0) + int2(1, 1), int2(0, 0), maxIn);
  float centerDepth = depthTex.Load(int3(qc, 0)).x;
  float bestDepth = centerDepth;
  float minDepth = centerDepth;
  float2 bestOffset = float2(0.0, 0.0);
  const float2 kOffsets[4] = {
    float2(-1.0, -1.0), float2(1.0, -1.0),
    float2(-1.0, 1.0), float2(1.0, 1.0),
  };
  [unroll]
  for (int di = 0; di < 4; ++di) {
    int2 tapT = clamp(qc + int2(kOffsets[di]), int2(0, 0), maxIn);
    float tapDepth = depthTex.Load(int3(tapT, 0)).x;
    if (tapDepth >= bestDepth) {
      bestDepth = tapDepth;
      bestOffset = kOffsets[di];
    }
    minDepth = min(minDepth, tapDepth);
  }
  float depthSpread = (bestDepth - minDepth) / max(bestDepth, 1e-4);
  float2 motion_in = float2(0.0, 0.0);
  if (shader_injection_data.upsampler_motion_live > 0.5) {
    float2 motionUV = saturate(q + bestOffset * texelIn);
    motion_in = BilinearTap(motionTex, motionUV, texelIn, maxIn).xy;
  }
  float2 motion_out = motion_in * float2(rx, ry);
  float motionPixels_out = length(motion_out);
  // Output history lives in stable output UV space (outPrevScale = 1);
  // display-mode changes reset via dims change (C++ side).
  float2 historyUV_out = saturate(uv_out + motion_out * texelOut);

  // --- RCAS strength incl. motion mask (shared uniforms, output-px rescale)
  // Threshold/range transfer from input-px tuning by the render ratio.
  float rcasCon = clamp(shader_injection_data.rcas_strength, 0.0, 1.0);
  {
    float s = clamp(shader_injection_data.rcas_strength, 0.0, 1.0);
    float mOn = (shader_injection_data.rcas_motion_on > 0.5
                 && shader_injection_data.upsampler_motion_live > 0.5) ? 1.0 : 0.0;
    float mT = 0.0;
    if (mOn > 0.5) {
      float th = max(shader_injection_data.rcas_motion_threshold, 0.0) * ratioAvg;
      float rg = max(shader_injection_data.rcas_motion_range, 0.25) * ratioAvg;
      mT = saturate((motionPixels_out - th) / max(rg, 1e-4));
      mT = pow(mT, clamp(shader_injection_data.rcas_motion_response, 0.5, 3.0));
    }
    rcasCon = lerp(s, clamp(shader_injection_data.rcas_motion_strength, 0.0, 1.0), mOn * mT);
  }
  float denoiseOn = (shader_injection_data.rcas_denoise > 0.5) ? 1.0 : 0.0;

  // --- First frame / reset: spatial-only resolve, seed history unsharpened.
  if (shader_injection_data.upsampler_history_valid < 0.5) {
    histUAV[pixel] = float4(clamp(current, 0.0, 65472.0), 1.0);
    float3 s0 = RCASSharpen(current, current, current, current, current, rcasCon, denoiseOn);
    o0 = float4(clamp(s0, 0.0, 65472.0), 1.0);
    return;
  }

  // --- Output-resolution history, Full Catmull-Rom (integer loads) ---
  float2 htc = historyUV_out / texelOut - 0.5;
  float2 hi0 = floor(htc);
  float2 hf = htc - hi0;
  float4 hwx = CustomTAA_CubicWeights(hf.x);
  float4 hwy = CustomTAA_CubicWeights(hf.y);
  float3 history = float3(0.0, 0.0, 0.0);
  [unroll]
  for (int hj = 0; hj < 4; ++hj) {
    [unroll]
    for (int hi = 0; hi < 4; ++hi) {
      int2 t = int2(hi0) + int2(hi - 1, hj - 1);
      t = clamp(t, int2(0, 0), maxOut);
      history += (hwx[hi] * hwy[hj]) * histTex.Load(int3(t, 0)).xyz;
    }
  }
  if (!isfinite(dot(history, history))) {
    history = current;
  }

  // --- Validation + feedback + accumulation (sole path; all diagnostics removed) ---
  float3 result;
  {

  // --- Validation footprint: integer-texel centered 3x3 around the
  // reconstruction anchor (9 distinct taps, symmetric). Covers the
  // Catmull-Rom support relevant to the current sample; k-DOP math itself
  // untouched.
  int2 qb = int2(floor(tc + 0.5));
  float3 vsamp[9];
  float3 vMin = float3(1e9, 1e9, 1e9);
  float3 vMax = float3(-1e9, -1e9, -1e9);
  int vi = 0;
  [unroll]
  for (int oy = -1; oy <= 1; ++oy) {
    [unroll]
    for (int ox = -1; ox <= 1; ++ox) {
      float2 np = float2(qb) + float2((float)ox, (float)oy);
      int2 idx = clamp(int2(floor(np + 0.5)), int2(0, 0), maxIn);
      float3 s = tex.Load(int3(idx, 0)).xyz;
      vsamp[vi++] = s;
      vMin = min(vMin, s);
      vMax = max(vMax, s);
    }
  }
  float clipT = 1.0;
  float overshootR = 0.0;
  int clipMode = (int)(shader_injection_data.custom_taa_clip_mode + 0.5);
  // Accept History diagnostic (TAAU History Validation = 1): skip only the
  // clip verdict — history keeps the reprojected sample and overshootR stays
  // 0, so the overshoot penalty releases through the unchanged formula below
  // while motion/silhouette/detail feedback runs verbatim. Mode 0 executes
  // the identical clip path as before.
  if (shader_injection_data.upsampler_history_validation < 0.5) {
  if (clipMode == CUSTOM_TAA_CLIP_KDOP) {
    int setSel = (int)(shader_injection_data.custom_taa_kdop_axes + 0.5);
    history = CustomTAA_ClipKDOP(
        current, history, vsamp, setSel,
        shader_injection_data.custom_taa_kdop_epsilon, clipT, overshootR);
  } else if (clipMode == CUSTOM_TAA_CLIP_AABB) {
    history = CustomTAA_ClipToExtents(current, history, vMin, vMax, clipT, overshootR);
  }
  }

  // --- Feedback chain (shared uniforms; motion scale rescaled to output px
  // so input-px tuning transfers: same physical response at any ratio) ---
  float effScale = shader_injection_data.custom_taa_motion_scale / max(ratioAvg, 1e-4);
  float motionWeight = saturate(motionPixels_out * effScale);
  if (shader_injection_data.custom_taa_squared_motion_response > 0.5) {
    motionWeight = motionWeight * motionWeight;
  }
  float adaptiveFeedback = lerp(shader_injection_data.custom_taa_static_feedback,
                                shader_injection_data.custom_taa_dynamic_feedback,
                                motionWeight);
  float softK = max(shader_injection_data.custom_taa_overshoot_softness, 1e-4);
  float overshootScale = 1.0 / (1.0 + (overshootR / softK) * (overshootR / softK));
  float disocc = 1.0 - motionWeight * saturate(depthSpread * shader_injection_data.custom_taa_silhouette_rejection);
  float detailGate = 0.0;
  {
    float detailRange = max(vMax.x - vMin.x, max(vMax.y - vMin.y, vMax.z - vMin.z));
    float refLuma = max(dot(current, float3(0.299, 0.587, 0.114)), 1e-2);
    detailGate = saturate((detailRange / refLuma) * shader_injection_data.custom_taa_detail_restore);
  }
  float restoreGate = detailGate * overshootScale;
  float feedback = clamp(adaptiveFeedback * overshootScale * disocc, 0.0, 0.99);
  feedback = lerp(feedback, shader_injection_data.custom_taa_static_feedback, saturate(restoreGate));
  float detailTarget = shader_injection_data.custom_taa_detail_target;
  if (detailTarget > shader_injection_data.custom_taa_static_feedback) {
    float boostGate = detailGate * overshootScale * disocc * (1.0 - motionWeight);
    feedback = lerp(feedback, min(detailTarget, 0.99), saturate(boostGate));
  }
  // Confidence-weighted accumulation: strongly-supported pixels update firmly,
  // interpolated pixels mostly retain history.
  float updateWeight = (1.0 - feedback) * beta;
  result = lerp(history, current, updateWeight);
  }  // end validation + feedback + accumulation block

  // --- Store unsharpened accumulation, then fused RCAS ---
  // Ring taps are center-motion-reprojected bilinear history (NOT full second
  // accumulations: ~16 loads vs ~5x pipeline cost). Converged static areas
  // satisfy history == accumulation (near-exact); in motion the limiter
  // contains mismatch while the temporally-filtered ring suppresses shimmer.
  // Full per-pixel ring accumulation remains a documented upgrade path.
  histUAV[pixel] = float4(clamp(result, 0.0, 65472.0), 1.0);
  if (shader_injection_data.upsampler_history_diagnostic > 0.5) {
    // History Only diagnostic: display the validated reprojected history the
    // resolver already uses. History write above, feedback, validation, beta,
    // reconstruction and RCAS math all ran unchanged; only o0 is redirected.
    // Temporary diagnostic; do not keep.
    o0 = float4(clamp(history, 0.0, 65472.0), 1.0);
    return;
  }
  float3 ringN = BilinearTap(histTex, saturate(uv_out + float2(0.0, -1.0) * texelOut + motion_out * texelOut), texelOut, maxOut);
  float3 ringW = BilinearTap(histTex, saturate(uv_out + float2(-1.0, 0.0) * texelOut + motion_out * texelOut), texelOut, maxOut);
  float3 ringE = BilinearTap(histTex, saturate(uv_out + float2(1.0, 0.0) * texelOut + motion_out * texelOut), texelOut, maxOut);
  float3 ringS = BilinearTap(histTex, saturate(uv_out + float2(0.0, 1.0) * texelOut + motion_out * texelOut), texelOut, maxOut);
  float3 sharpened = RCASSharpen(ringN, ringW, result, ringE, ringS, rcasCon, denoiseOn);
  o0 = float4(clamp(sharpened, 0.0, 65472.0), 1.0);
}
