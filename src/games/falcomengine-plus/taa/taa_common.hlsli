// Custom TAA shared core (Sora 1st / 2nd).
//
// From-scratch temporal accumulation. The dumped vanilla TAA files under
// sora1st/taa and sora2nd/taa are REFERENCE ONLY and stay untouched; this
// file re-implements only the verified external interface (resource bindings
// + coordinate convention) and builds an independent algorithm on top:
//
//   reproject (closest-depth motion convention)
//     -> history reconstruction (bilinear / adaptive bicubic / full bicubic)
//     -> validation (none / RGB AABB / k-DOP segment clip)
//     -> fixed-feedback accumulation with first-frame history reset
//
// Paper mapping:
// - Survey (Yang et al.): pipeline skeleton (reproject -> reconstruct ->
//   validate -> accumulate). Architecture only, no code taken.
// - Lin et al. high-order interpolation: bicubic history reconstruction as
//   bilinear + higher-order terms; edge D-terms gate the interior taps
//   (adaptive ~= paper's square-C12 vs full square-C16 structure).
// - k-DOP clipping (Ikkala et al., CC0 supplementary): polytope segment clip
//   with the paper's precomputed axis sets (see kAxes entries below).
//
// TAA-0 must prove reprojection first: fixed feedback, bilinear, no clip.
// Jitter: no explicit jitter-delta correction. Game motion vectors already
// contain the frame-to-frame jitter delta (see CustomTAA_Reproject notes).

#ifndef FALCOMENGINE_PLUS_TAA_COMMON_HLSLI_
#define FALCOMENGINE_PLUS_TAA_COMMON_HLSLI_

#include "../shared.h"

// Must match shared.h Custom TAA fields.
#define CUSTOM_TAA_FILTER_BILINEAR 0
#define CUSTOM_TAA_FILTER_ADAPTIVE 1
#define CUSTOM_TAA_FILTER_FULL 2
#define CUSTOM_TAA_CLIP_NONE 0
#define CUSTOM_TAA_CLIP_AABB 1
#define CUSTOM_TAA_CLIP_KDOP 2
#define CUSTOM_TAA_AXES_16 0
#define CUSTOM_TAA_AXES_22 1
#define CUSTOM_TAA_AXES_32 2
#define CUSTOM_TAA_DEBUG_NORMAL 0
#define CUSTOM_TAA_DEBUG_HISTORY 1
#define CUSTOM_TAA_DEBUG_CLIP 2
#define CUSTOM_TAA_DEBUG_DIFF_ADAPTIVE 3
#define CUSTOM_TAA_DEBUG_DIFF_FULL 4
// Diff views use a fixed-gain relative normalized difference (diagnostic
// only): diff / max(|A|, |B|, 1e-4), max channel, x8. Stable across runs,
// no extra user variable while the implementation is being validated.
// Caveat: D_min = 0 makes the adaptive gate maximally permissive, not a
// literal every-pixel-uses-Full guarantee — near-zero cubic corrections can
// still naturally equal bilinear. Compare overall spatial patterns.

// ---------------------------------------------------------------------------
// Verified external interface (coordinate convention, not algorithm).
//
// The game exposes: t0 color, t1 depth, t2 history, t3 motion (texel units),
// b2 texelSize/prevResolutionScale. TAA-0 reproduces the motion-lookup
// convention (max-depth tap among center + 4 diagonal neighbors selects the
// motion UV) as the initial reference. This convention is owned by the game
// interface; once TAA-0 proves accumulation, it can be inspected/replaced
// independently of the temporal algorithm.
// ---------------------------------------------------------------------------
float2 CustomTAA_Reproject(
    float2 uv,
    Texture2D<float4> depthTexture, SamplerState samPoint,
    Texture2D<float4> motionTexture, SamplerState samLinear,
    float2 texelSize, float2 prevResolutionScale,
    out float motionPixels) {
  float centerDepth = depthTexture.SampleLevel(samPoint, uv, 0).x;
  float2 bestOffset = float2(0.0, 0.0);
  float bestDepth = centerDepth;
  // Diagonal taps, matching the game's 5-tap footprint.
  const float2 kOffsets[4] = {
    float2(-1.0, -1.0), float2(1.0, -1.0),
    float2(-1.0, 1.0), float2(1.0, 1.0),
  };
  [unroll]
  for (int i = 0; i < 4; ++i) {
    float2 tapUV = saturate(uv + kOffsets[i] * texelSize);
    float tapDepth = depthTexture.SampleLevel(samPoint, tapUV, 0).x;
    if (tapDepth >= bestDepth) {
      bestDepth = tapDepth;
      bestOffset = kOffsets[i];
    }
  }
  float2 motionUV = saturate(uv + bestOffset * texelSize);
  float2 motionTexels = motionTexture.SampleLevel(samLinear, motionUV, 0).xy;
  // No explicit jitter-delta correction, by evidence (not assumption):
  // - Game motion vectors already carry the frame-to-frame jitter delta.
  //   E.g. sora1st foliage (staticfoliage_0x39F91AE8:246-252) computes
  //   (prevClip - curClip) in pixels and adds jitterDiff_g before writing
  //   the motion target; the same jitterDiff_g + motion pattern appears in
  //   the sky and all foliage shaders. The TAA cbuffer's jitter_g field is
  //   therefore vestigial: the historyUV formula below already reprojects
  //   through jittered motion, and an extra +/-jitter term here would
  //   double-compensate.
  // - Runtime corroboration: feedback=0 shows raw upstream jitter while
  //   feedback=0.9 converges stably, which a missing jitter delta would not.
  // Motion magnitude in pixels/frame (texture values are texel units).
  // Jitter-inclusive: a static camera still reports roughly 0-1px from the
  // frame-to-frame jitter delta alone. TAA-5 feedback scaling must therefore
  // stay small enough not to punish subpixel jitter as "motion".
  motionPixels = length(motionTexels);
  float2 historyUV = uv * prevResolutionScale + motionTexels * texelSize;
  return historyUV;
}

// ---------------------------------------------------------------------------
// History reconstruction.
// ---------------------------------------------------------------------------
float3 CustomTAA_SampleBilinear(
    float2 historyUV,
    Texture2D<float4> historyTexture, SamplerState samLinear) {
  return historyTexture.SampleLevel(samLinear, saturate(historyUV), 0).xyz;
}

// Uniform Catmull-Rom weights. w(0) = (0,1,0,0), w(1) = (0,0,1,0).
float4 CustomTAA_CubicWeights(float t) {
  float t2 = t * t;
  float t3 = t2 * t;
  return float4(
      -0.5 * t3 + t2 - 0.5 * t,
      1.5 * t3 - 2.5 * t2 + 1.0,
      -1.5 * t3 + 2.0 * t2 + 0.5 * t,
      0.5 * t3 - 0.5 * t2);
}

float2 CustomTAA_TexelToUV(int2 texel, float2 texelSize) {
  return (float2(texel) + 0.5) * texelSize;
}

// Full bicubic (16 point taps). Reference for adaptive/full comparison;
// BOP-folded optimization is TAA-4 work.
float3 CustomTAA_SampleBicubicFull(
    float2 historyUV, float2 texelSize,
    Texture2D<float4> historyTexture, SamplerState samPoint) {
  float2 clampedUV = saturate(historyUV);
  float2 tc = clampedUV / texelSize - 0.5;
  float2 i0 = floor(tc);
  float2 f = tc - i0;
  float4 wx = CustomTAA_CubicWeights(f.x);
  float4 wy = CustomTAA_CubicWeights(f.y);
  float3 sum = float3(0.0, 0.0, 0.0);
  [unroll]
  for (int j = 0; j < 4; ++j) {
    [unroll]
    for (int i = 0; i < 4; ++i) {
      int2 texel = int2(i0) + int2(i - 1, j - 1);
      float3 tap = historyTexture.SampleLevel(
                               samPoint, CustomTAA_TexelToUV(texel, texelSize), 0)
                       .xyz;
      sum += (wx[i] * wy[j]) * tap;
    }
  }
  return sum;
}

// Adaptive high-order: bilinear base + edge D-term gate (paper's adaptive
// structure: edge-only work ~= square-C12, interior taps only when needed).
// Edge D-terms use the Catmull-Rom t=0.5 midpoint deviation:
//   D = 0.0625 * ((P0 + P1) - (Pm1 + P2))
// i.e. how far the cubic midpoint sits from the linear midpoint. When every
// edge D-term is below D_min, cubic correction is skipped and bilinear is
// returned (saves the 4 corner taps + correction math).
float3 CustomTAA_SampleAdaptive(
    float2 historyUV, float2 texelSize, float dmin,
    Texture2D<float4> historyTexture, SamplerState samPoint, SamplerState samLinear) {
  float3 bilinear = CustomTAA_SampleBilinear(historyUV, historyTexture, samLinear);
  float2 clampedUV = saturate(historyUV);
  float2 tc = clampedUV / texelSize - 0.5;
  int2 b = int2(floor(tc));

  // Central rows (y = b.y, b.y+1): deviation along x.
  float3 rowA0 = historyTexture.SampleLevel(samPoint, CustomTAA_TexelToUV(b + int2(0, 0), texelSize), 0).xyz;
  float3 rowA1 = historyTexture.SampleLevel(samPoint, CustomTAA_TexelToUV(b + int2(1, 0), texelSize), 0).xyz;
  float3 rowAm1 = historyTexture.SampleLevel(samPoint, CustomTAA_TexelToUV(b + int2(-1, 0), texelSize), 0).xyz;
  float3 rowA2 = historyTexture.SampleLevel(samPoint, CustomTAA_TexelToUV(b + int2(2, 0), texelSize), 0).xyz;
  float3 rowB0 = historyTexture.SampleLevel(samPoint, CustomTAA_TexelToUV(b + int2(0, 1), texelSize), 0).xyz;
  float3 rowB1 = historyTexture.SampleLevel(samPoint, CustomTAA_TexelToUV(b + int2(1, 1), texelSize), 0).xyz;
  float3 rowBm1 = historyTexture.SampleLevel(samPoint, CustomTAA_TexelToUV(b + int2(-1, 1), texelSize), 0).xyz;
  float3 rowB2 = historyTexture.SampleLevel(samPoint, CustomTAA_TexelToUV(b + int2(2, 1), texelSize), 0).xyz;
  // Central columns (x = b.x, b.x+1): deviation along y.
  float3 colA0 = historyTexture.SampleLevel(samPoint, CustomTAA_TexelToUV(b + int2(0, 0), texelSize), 0).xyz;
  float3 colA1 = historyTexture.SampleLevel(samPoint, CustomTAA_TexelToUV(b + int2(0, 1), texelSize), 0).xyz;
  float3 colAm1 = historyTexture.SampleLevel(samPoint, CustomTAA_TexelToUV(b + int2(0, -1), texelSize), 0).xyz;
  float3 colA2 = historyTexture.SampleLevel(samPoint, CustomTAA_TexelToUV(b + int2(0, 2), texelSize), 0).xyz;
  float3 colB0 = historyTexture.SampleLevel(samPoint, CustomTAA_TexelToUV(b + int2(1, 0), texelSize), 0).xyz;
  float3 colB1 = historyTexture.SampleLevel(samPoint, CustomTAA_TexelToUV(b + int2(1, 1), texelSize), 0).xyz;
  float3 colB1m = historyTexture.SampleLevel(samPoint, CustomTAA_TexelToUV(b + int2(1, -1), texelSize), 0).xyz;
  float3 colB2 = historyTexture.SampleLevel(samPoint, CustomTAA_TexelToUV(b + int2(1, 2), texelSize), 0).xyz;

  float3 dRowA = 0.0625 * ((rowA0 + rowA1) - (rowAm1 + rowA2));
  float3 dRowB = 0.0625 * ((rowB0 + rowB1) - (rowBm1 + rowB2));
  float3 dColA = 0.0625 * ((colA0 + colA1) - (colAm1 + colA2));
  float3 dColB = 0.0625 * ((colB0 + colB1) - (colB1m + colB2));
  float dMax = max(max(max(abs(dRowA.x), abs(dRowA.y)), abs(dRowA.z)),
                   max(max(max(abs(dRowB.x), abs(dRowB.y)), abs(dRowB.z)),
                       max(max(max(abs(dColA.x), abs(dColA.y)), abs(dColA.z)),
                           max(abs(dColB.x), max(abs(dColB.y), abs(dColB.z))))));
  if (dMax < dmin) {
    return bilinear;
  }
  return CustomTAA_SampleBicubicFull(historyUV, texelSize, historyTexture, samPoint);
}

// ---------------------------------------------------------------------------
// History validation: segment clip of (current -> history) against the
// neighborhood polytope. Shared slab march; AABB uses the 3 basis axes,
// k-DOP adds the paper's oriented axes.
// ---------------------------------------------------------------------------
// Full polytope clip returning the clipped color + parametric t.
// Also reports overshootR (same normalized definition as the k-DOP variant)
// so the feedback-softening experiment behaves consistently in AABB mode.
float3 CustomTAA_ClipToExtents(float3 cur, float3 prev,
                               float3 minC, float3 maxC, out float clipT, out float overshootR) {
  float3 dir = prev - cur;
  float nearT = 0.0;
  float farT = 1.0;
  overshootR = 0.0;
  [unroll]
  for (int a = 0; a < 3; ++a) {
    float outside = max(minC[a] - prev[a], prev[a] - maxC[a]);
    if (outside > 0.0) {
      overshootR = max(overshootR, outside / ((maxC[a] - minC[a]) + 1e-3));
    }
    float origin = cur[a];
    float d = dir[a];
    if (abs(d) < 1e-8) {
      if (origin < minC[a] || origin > maxC[a]) {
        clipT = 0.0;
        return cur;
      }
      continue;
    }
    float invD = 1.0 / d;
    float t0 = (minC[a] - origin) * invD;
    float t1 = (maxC[a] - origin) * invD;
    nearT = max(nearT, min(t0, t1));
    farT = min(farT, max(t0, t1));
  }
  if (nearT > farT) {
    clipT = 0.0;
    return cur;
  }
  float t = clamp(nearT > 0.0 ? nearT : farT, 0.0, 1.0);
  clipT = t;
  return cur + t * dir;
}

// k-DOP axis sets from the paper supplementary (CC0). First three entries of
// the general sets are XYZ, so k-DOP gracefully contains the AABB.
static const float3 kCustomTAA_Axes32[16] = {
  float3(1, 0, 0), float3(0, 1, 0), float3(0, 0, 1),
  float3(0.820081, 0.456727, -0.344773), float3(0.540295, 0.829202, 0.143195),
  float3(0.255800, 0.841084, -0.476597), float3(-0.406935, -0.389062, 0.826459),
  float3(-0.826708, -0.382923, -0.412219), float3(0.260942, -0.577482, 0.773578),
  float3(0.254398, 0.637821, 0.726957), float3(0.310900, -0.728083, -0.610930),
  float3(0.798513, -0.556827, -0.228738), float3(0.673383, -0.163602, -0.720964),
  float3(-0.813922, 0.369658, -0.448201), float3(0.477650, -0.853722, 0.207384),
  float3(-0.554854, -0.041550, -0.830910),
};

// Paper's lightweight 22-DOP set (more zeroed entries; cheaper dots).
// Padded to 16 with zeroes; entries 11-15 are never read (axisCount = 11).
static const float3 kCustomTAA_Axes22[16] = {
  float3(1, 0, 0), float3(0, 1, 0), float3(0, 0, 1),
  float3(0.000000, 0.664104, -0.747640), float3(-0.656496, -0.656139, 0.372149),
  float3(-0.664251, 0.000000, -0.747509), float3(-0.664672, 0.000000, 0.747135),
  float3(0.000000, 0.664287, 0.747478), float3(0.657415, -0.657637, -0.367857),
  float3(-0.656571, -0.657954, -0.368796), float3(0.656800, -0.655635, 0.372501),
  float3(0, 0, 0), float3(0, 0, 0), float3(0, 0, 0), float3(0, 0, 0), float3(0, 0, 0),
};

int CustomTAA_KDOPAxisCount(int setSel) {
  if (setSel == CUSTOM_TAA_AXES_32) return 16;
  if (setSel == CUSTOM_TAA_AXES_22) return 11;
  return 8;
}

// k-DOP segment clip (paper's kdop_clipping ported to HLSL): project the 3x3
// neighborhood onto each axis, slab-march the cur->prev segment.
// Also reports overshootR: how far the RAW history sits outside the padded
// hull, normalized per axis by (extentSize + 1e-3) and maximized over axes.
// Dimensionless and HDR-scale-aware: r~0 means "inside/plausible", r>>1 means
// "genuine change". Used to soften feedback (TAA-5 experiment), never to move
// the clip boundary itself.
float3 CustomTAA_ClipKDOP(float3 cur, float3 prev, float3 neighborhood[9],
                          int setSel, float epsilon, out float clipT, out float overshootR) {
  float3 dir = prev - cur;
  float nearT = -1e9;
  float farT = 1e9;
  overshootR = 0.0;
  int axisCount = CustomTAA_KDOPAxisCount(setSel);
  [unroll]
  for (int a = 0; a < 16; ++a) {
    if (a >= axisCount) continue;
    // 16-DOP default: first 8 axes of the general 32-DOP set (general-purpose;
    // scene-optimized grass/asphalt 16-DOP sets are TAA-4 tuning candidates).
    float3 axis = (setSel == CUSTOM_TAA_AXES_22) ? kCustomTAA_Axes22[a] : kCustomTAA_Axes32[a];
    float eMin = 1e9;
    float eMax = -1e9;
    [unroll]
    for (int n = 0; n < 9; ++n) {
      float t = dot(neighborhood[n], axis);
      eMin = min(t, eMin);
      eMax = max(t, eMax);
    }
    eMin -= epsilon;
    eMax += epsilon;
    float prevProj = dot(prev, axis);
    float outside = max(eMin - prevProj, prevProj - eMax);
    if (outside > 0.0) {
      overshootR = max(overshootR, outside / ((eMax - eMin) + 1e-3));
    }
    float denom = dot(dir, axis);
    if (abs(denom) < 1e-8) {
      float p = dot(cur, axis);
      if (p < eMin || p > eMax) {
        clipT = 0.0;
        return cur;
      }
      continue;
    }
    float invDir = 1.0 / denom;
    float projPos = dot(cur, axis);
    float t0 = (eMin - projPos) * invDir;
    float t1 = (eMax - projPos) * invDir;
    nearT = max(nearT, min(t0, t1));
    farT = min(farT, max(t0, t1));
  }
  if (nearT <= farT && (nearT > 0.0 || farT > 0.0)) {
    float t = clamp(nearT > 0.0 ? nearT : farT, 0.0, 1.0);
    clipT = t;
    return cur + t * dir;
  }
  clipT = 0.0;
  return cur;
}

#endif  // FALCOMENGINE_PLUS_TAA_COMMON_HLSLI_
