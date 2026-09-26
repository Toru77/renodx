// Motion Blur — shared kernel and helpers.
//
// Algorithm: Guertin, McGuire & Nowrouzezahrai, "A Fast and Stable
// Feature-Aware Motion Blur Filter" (NVIDIA TR NVR-2013-003, 2013). Paper text
// is kept at src/games/falcomengine-plus/reference/motionblur/.
//
// ── UNITS (read this before changing any constant) ───────────────────────────
// The paper states every length in PIXELS at a fixed reference resolution
// (Section 5 uses {N, r, t, k, h, g} = {25, 40, 1, 40, 0.95, 1.5}). We keep all
// math in UV space so the filter is resolution independent (Section 3), which
// means every pixel-denominated constant must be converted:
//
//   paper r  -> tileUV = r  / MB_REF_H
//   paper h  -> hUV    = h  / MB_REF_H
//   paper g  -> gUV    = g  / MB_REF_H
//   paper 0.5 (early-out / Eq. 1 threshold) -> minVelUV = 0.5 / MB_REF_H
//   paper k  -> kUV    = k  * MB_REF_H   (k is a DIVISOR of a length, so it
//                                          scales the other way)
//
// Derivation of the k flip: the paper computes totalWeight = N / (k * |vc|)
// with |vc| in pixels. |vc| in UV is |vc|_px / MB_REF_H, so
//   N / (k_uv * |vc|_uv) == N / (k * |vc|_px)
//   => k_uv * |vc|_px / MB_REF_H == k * |vc|_px
//   => k_uv == k * MB_REF_H
//
// paper t (Section 4.2 tile-edge falloff) is in TILES, not pixels, so it is
// used unscaled. See MBNeighborTile.
//
// A 4K player therefore sees the same blur length, the same tile grid, the same
// tile-boundary behaviour and the same sample count as a 1080p player; only the
// pixel count the gather runs over changes.
//
// ── MOTION VECTORS ──────────────────────────────────────────────────────────
// Source: the game's motion texture, bound as t3 of the TAA draw. Encoding is
// raw float2 PIXELS of that texture, prevPixel - curPixel + jitterDiff. The
// frame-to-frame jitter delta is ALREADY inside the value (every geometry
// shader adds jitterDiff_g before writing the target), so no jitter term is
// subtracted here — doing so would double-compensate. Same rule as the custom
// TAA, see taa/taa_common.hlsli.
//
// The velocity texture re-expresses this in UV (pixels / texture dims) so
// every downstream stage is resolution free. mb_intensity multiplies the
// velocity and therefore behaves as an exposure/shutter control; 0 is an exact
// no-op because the gather early-outs on zero velocity.

#ifndef FALCOMENGINE_PLUS_MOTION_BLUR_COMMON_HLSLI_
#define FALCOMENGINE_PLUS_MOTION_BLUR_COMMON_HLSLI_

#include "../shared.h"

// Reference height for all pixel-denominated paper constants (see header).
#define MB_REF_H 1080.0

// ── vector helpers ──
float2 MBNorm(float2 v) {
  float lenSq = dot(v, v);
  return (lenSq > 1e-16) ? v * rsqrt(lenSq) : float2(0.0, 0.0);
}

// Paper "rnmix": linear interpolation followed by normalization.
float2 MBRNMix(float2 a, float2 b, float t) { return MBNorm(lerp(a, b, t)); }

// Paper "cone" (McGuire et al. 2012): linear falloff of half-width `halfWidth`.
float MBCone(float t, float halfWidth) {
  return max(0.0, 1.0 - abs(t) / max(halfWidth, 1e-6));
}

// Paper "cylinder" (McGuire et al. 2012): quadratic falloff of half-width.
float MBCylinder(float t, float halfWidth) {
  float w = max(halfWidth, 1e-6);
  float x = t / w;
  return max(0.0, 1.0 - x * x);
}

// Paper Section 5 relative depth test, scene independent by construction:
//   zCompare[za,zb] = min(max(0, 1 - |za-zb| / min(za,zb)), 1)
// Fed LINEAR view depth, which is what makes the relative form meaningful; a
// raw hardware depth buffer collapses min(za,zb) at distance and the term
// saturates. mb_depth_tolerance widens the soft transition (the paper's form
// has no tolerance control).
float MBZCompare(float za, float zb) {
  float denom = max(min(za, zb), 1e-4);
  return saturate(1.0 - abs(za - zb) / denom)
       * saturate(shader_injection_data.mb_depth_tolerance);
}

// ── per-pixel jitter (paper Section 4.5 deterministic Halton) ──
float MBHalton2(uint index) {
  float result = 0.0;
  float f = 0.5f;
  uint i = index;
  while (i > 0u) {
    result += f * float(i & 1u);
    i >>= 1;
    f *= 0.5;
  }
  return result;
}

float MBHalton3(uint index) {
  float result = 0.0;
  float f = 1.0f / 3.0f;
  uint i = index;
  while (i > 0u) {
    result += f * float(i % 3u);
    i /= 3u;
    f /= 3.0f;
  }
  return result;
}

float2 MBJitterHalton(int2 p) {
  // Unique, well-mixed index per pixel; the +1 keeps index 0 out of the loop.
  uint base = uint(p.x) + uint(p.y) * 65537u + 1u;
  return float2(MBHalton2(base), MBHalton3(base));
}

// ── game motion -> UV velocity ──
// Source: the game's motion texture (t3 of the TAA draw). Encoding, established
// from the geometry shaders that write it (e.g. sora2nd/foliage/
// staticfoliage_0xF1EC53A8:201-207):
//
//   motion.xy = (prevPixel - curPixel) + jitterDiff_g     [raw float2, pixels]
//
// The jitter delta is already inside the value; nothing is subtracted here, and
// this stays true no matter how many consumers read it.
//
// The conversion runs at the point of use rather than in a full-resolution
// intermediate. It is a few ALU against a load the consumer was already
// issuing, so the fetch count is unchanged, and it removes a 29.5 MB per-frame
// write at 1440p plus a dispatch. That write was large enough to evict the
// gather's working set from L2 and force its 25 taps per pixel out to DRAM.
//
// The result is TRUE UV velocity: shutter scaling is deliberately NOT applied
// here. mb_intensity is an exposure control, so folding it into the velocity
// would make the gather's fixed minVelocityUV threshold a function of the
// slider -- below a point the filter switches off completely instead of
// blurring less -- and it would leak the gain into the per-pixel weighting
// terms (wB, 1/sampleLength, min(sampleLength, centerLength)), where the
// engine's per-frame jitter delta then dominates and the result crawls. The
// gather applies the shutter to the integration domain instead, where it
// belongs.
float2 MBGameMotionToUV(float2 motionPx) {
  float2 dims = max(float2(shader_injection_data.mb_motion_w,
                            shader_injection_data.mb_motion_h), float2(1.0, 1.0));
  float2 velocity = motionPx / dims;
  // Paper Section 3: the sample domain is 1D along vmax and the tile is r, so
  // clamping |v| at r is what preserves the "a pixel is at most influenced by
  // its 1-ring neighbouring tiles" property the whole filter relies on.
  float maxLength = max(shader_injection_data.mb_tile_uv, 1e-8);
  float len = length(velocity);
  return (len > maxLength) ? (velocity * (maxLength / len)) : velocity;
}

// ── Section 4.2 stochastic on-axis NeighborMax lookup ──
// The paper jitters a single tile lookup (never into a diagonal tile) with a
// linear falloff whose slope t is the tunable. Because the tile grid is axis
// aligned, an x offset only changes the tile column and a y offset only the
// tile row, so per-axis offsets cannot reach a diagonal neighbour. This trades
// banding for noise near tile borders, which is the point of the technique;
// the paper explicitly rejects interpolating between neighbourhood velocities
// because "the blur seems to wave and often highlights, as opposed to mask,
// tile boundaries".
//
// Returns an integer texel and is read with Load rather than SampleLevel. Every
// other read in this chain is already a point Load, and the filter must not
// depend on a sampler descriptor being valid: truncation of a non-negative
// coordinate is floor, which is exactly the texel a point sampler would have
// selected, so this is identical when a sampler is present and correct when one
// is absent.
int2 MBNeighborTile(float2 uv, float2 jitter) {
  float2 tiles = max(float2(shader_injection_data.mb_tiles_x,
                            shader_injection_data.mb_tiles_y), 1.0);
  float2 pos = uv * tiles;
  float2 distToBorder = min(pos, tiles - pos);
  float2 falloff = saturate(1.0 - distToBorder / max(shader_injection_data.mb_neighbor_t, 1e-3));
  float2 tile = pos + (jitter * 2.0 - 1.0) * falloff;
  return clamp(int2(tile), int2(0, 0), int2(tiles) - 1);
}

// ── adaptive sample ladder ──
// Buckets rather than a continuous ratio because the count derives from
// tile-level NeighborMax and is therefore strongly spatially coherent across the
// grid. It is not guaranteed uniform within a tile: the Section 4.2 stochastic
// border lookup can split a tile boundary between two buckets, so warp
// divergence is limited and clustered, not per pixel.
int MBSampleBucket(float vmaxRatio) {
  if (vmaxRatio < 0.20f) return 1;
  if (vmaxRatio < 0.40f) return 2;
  if (vmaxRatio < 0.60f) return 3;
  if (vmaxRatio < 0.80f) return 4;
  if (vmaxRatio < 1.00f) return 5;
  return 6;
}

// maxSamples is a CEILING: every rung is clamped to it and the top rung IS it.
// Returning max(maxSamples, 20) instead would let a ceiling of 8 emit 20 taps,
// which inverts the setting. Pinning maxSamples to 4 clamps every rung to 4,
// which is how a fixed low tap count is requested.
uint MBSampleCount(int bucket, uint maxSamples) {
  uint n = 4u;
  if (bucket == 2) n = 8u;
  else if (bucket == 3) n = 12u;
  else if (bucket == 4) n = 16u;
  else if (bucket == 5) n = 20u;
  else if (bucket >= 6) n = maxSamples;
  return clamp(n, 4u, max(maxSamples, 4u));
}

// Black = early-out, then one distinct hue per rung, so the distribution of
// gather cost is readable at a glance rather than as subtle grey steps.
float3 MBBucketColor(int bucket) {
  if (bucket <= 0) return float3(0.00, 0.00, 0.00);
  if (bucket == 1) return float3(0.10, 0.10, 0.55);
  if (bucket == 2) return float3(0.10, 0.55, 0.55);
  if (bucket == 3) return float3(0.15, 0.65, 0.15);
  if (bucket == 4) return float3(0.75, 0.75, 0.10);
  if (bucket == 5) return float3(0.85, 0.45, 0.05);
  return float3(0.85, 0.10, 0.10);
}

#endif  // FALCOMENGINE_PLUS_MOTION_BLUR_COMMON_HLSLI_
