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

// Paper "cone" (McGuire et al. 2012): linear falloff over a domain of half-width
// `halfWidth`, i.e. max(0, 1 - |t|/halfWidth).
//
// Every call site passed halfWidth as 1/L for a velocity L, which cost a divide to
// build the reciprocal and a second divide to apply it. Since
//     1 - |t| / (1/L)  ==  1 - |t| * L
// the length is taken directly and the pair of divides collapses to a multiply.
// Two of these per tap is 2x16 divides per pixel saved at the default tap count.
float MBConeByLength(float t, float len) {
  return max(0.0, 1.0 - abs(t) * len);
}

// Paper "cylinder" (McGuire et al. 2012): quadratic falloff of half-width.
// Unlike the cone this is already one divide (t/L squared), so there is nothing
// to reclaim without changing the arithmetic.
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
//
// mb_frame_scale IS applied here, and this is the single place it is applied.
// Motion vectors are per-frame displacements, so a camera crossing the same
// point covers half as many pixels at 120 fps as at 60. Every length in this
// filter is derived from that vector -- the streak, the ladder buckets, the
// early-out, and the half-resolution split -- so they would all move with
// framerate. Scaling once, here, puts TileMax, NeighborMax and the gather's
// per-pixel reads into the same reference-frame units, and every downstream
// threshold becomes framerate-independent for free. Scaling it anywhere else
// would double-apply it: vmax arrives from neighbormax already converted.
float2 MBGameMotionToUV(float2 motionPx) {
  float2 dims = max(float2(shader_injection_data.mb_motion_w,
                            shader_injection_data.mb_motion_h), float2(1.0, 1.0));
  float2 velocity = motionPx / dims * max(shader_injection_data.mb_frame_scale, 0.0);
  // Paper Section 3: the sample domain is 1D along vmax and the tile is r, so
  // clamping |v| at r is what preserves the "a pixel is at most influenced by
  // its 1-ring neighbouring tiles" property the whole filter relies on.
  float maxLength = max(shader_injection_data.mb_tile_uv, 1e-8);
  float len = length(velocity);
  return (len > maxLength) ? (velocity * (maxLength / len)) : velocity;
}

// ── split-resolution routing ──
// Which resolution owns a pixel. Deliberately JITTER-FREE: every pass that makes
// this decision must reach the same answer, and the filter's own lookup below is
// jittered, so routing cannot reuse it. It works in UV, so it is resolution
// independent and the full-res and half-res gathers agree on the same tile even
// though their pixel indices, and therefore their Halton/IS-FAST jitter, differ.
//
// Per-tile rather than per-pixel is what makes the agreement possible, and it
// also means a gather rejects whole 8x8 groups with no intra-warp divergence. The
// cost is that the resolution switch is quantised to the tile grid.
int2 MBRoutingTile(float2 uv) {
  float2 tiles = max(float2(shader_injection_data.mb_tiles_x,
                            shader_injection_data.mb_tiles_y), 1.0);
  return clamp(int2(uv * tiles), int2(0, 0), int2(tiles) - 1);
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
//
// The count is keyed to |vmax| in 1080-REFERENCE PIXELS, deliberately NOT as a
// fraction of the tile radius. Keying it to tileUV coupled the tap count to an
// unrelated setting: dropping Max Radius from 40 to 24 moved the thresholds from
// 8/16/24/32/40 px down to 4.8/9.6/14.4/19.2/24, which pushed an entire moving
// scene onto the 12-16 tap rungs and cancelled most of the ladder's saving. How
// many taps a pixel needs is a function of how much smear is happening there,
// nothing else.
int MBSampleBucket(float vmaxUV) {
  if (vmaxUV < 1.5f / MB_REF_H) return 1;
  if (vmaxUV < 3.0f / MB_REF_H) return 2;
  if (vmaxUV < 6.0f / MB_REF_H) return 3;
  if (vmaxUV < 10.0f / MB_REF_H) return 4;
  if (vmaxUV < 16.0f / MB_REF_H) return 5;
  return 6;
}

// maxSamples is a CEILING: every rung is clamped to it and the top rung IS it.
// Returning max(maxSamples, 20) instead would let a ceiling of 8 emit 20 taps,
// which inverts the setting.
//
// The ceiling arrives from the Quality preset as 12/16/20/24, and the rungs below
// are 4/6/8/12/16, so the presets interact with the ladder differently: at High
// and Ultra every rung is distinct, while at Low (12) the top three rungs all
// clamp to 12 and "very fast" stops being distinguishable from "extremely fast".
// That is the intended cost of the Low preset, not a fault -- it is the only tier
// where the ladder stops discriminating. Pinning the ceiling to 4 would clamp
// every rung to 4, but no preset goes that low, so there is no longer a way to
// force a fixed 4-tap blur from the UI.
//
// NOTE: with a per-pixel bound a warp runs to its longest member, so the realised
// saving is bounded by divergence rather than by the mean. Rungs are tile-coherent
// and 8x8 groups sit well inside a tile, so that is expected to be mild, but the
// ladder's payoff is not strictly linear in the average tap count.
uint MBSampleCount(int bucket, uint maxSamples) {
  uint n = 4u;
  if (bucket == 2) n = 6u;
  else if (bucket == 3) n = 8u;
  else if (bucket == 4) n = 12u;
  else if (bucket == 5) n = 16u;
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
