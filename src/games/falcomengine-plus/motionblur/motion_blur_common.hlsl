// ── Motion blur shared helpers ──
// Camera-motion blur based on Guertin et al. 2013
// "A Fast and Stable Feature-Aware Motion Blur Filter"
//
// SPDX-License-Identifier: MIT

#ifndef MOTION_BLUR_COMMON_HLSL_
#define MOTION_BLUR_COMMON_HLSL_

// ── Push constants at b13 (shared with GTVBAO and other passes) ──
// These are written by addon.cpp in the same cbuffer slot.
// c[0]  = motion_blur_enabled       // 0=off, 1=on
// c[1]  = motion_blur_sample_count  // N — number of gather samples
// c[2]  = motion_blur_tile_size     // r — tile size in pixels
// c[3]  = motion_blur_inv_tile_size // 1.0 / r
// c[4]  = motion_blur_strength      // 0–1 blend factor
// c[5]  = motion_blur_jitter_h      // 0.95 (Halton jitter max offset in pixels)
// c[6]  = motion_blur_center_k      // 40 — center-weight normalization divisor
// c[7]  = motion_blur_threshold_g   // 1.5 — velocity-threshold for perpendicular blend
// c[8]  = motion_blur_screen_w      // screen width in pixels
// c[9]  = motion_blur_screen_h      // screen height in pixels
// c[10] = motion_blur_tile_grid_w   // ceil(w/r)
// c[11] = motion_blur_tile_grid_h   // ceil(h/r)

// ── 2D Halton sequence (bases 2, 3) ──
float Halton2(uint i)
{
    float r = 0.0;
    float f = 0.5;
    uint n = i;
    while (n > 0) {
        r += (n & 1u) * f;
        n >>= 1;
        f *= 0.5;
    }
    return r;
}

float Halton3(uint i)
{
    float r = 0.0;
    float f = 1.0 / 3.0;
    uint n = i;
    while (n > 0) {
        r += (n % 3u) * f;
        n /= 3u;
        f /= 3.0;
    }
    return r;
}

// Returns a jitter offset in [0, 1) for pixel p
float2 HaltonJitter(uint2 p)
{
    uint base = p.x + p.y * 65537u + 1u;  // per-pixel hash
    return float2(Halton2(base), Halton3(base));
}

// ── Relative depth comparison (Guertin Section 3 / Appendix A) ──
// Returns 1 when za is in front of zb, 0 when behind, smooth transition in between.
// Works with positive view-space depths (lower = closer to camera).
float zCompare(float za, float zb)
{
    float denom = max(min(za, zb), 0.001);
    return saturate(1.0 - abs(za - zb) / denom);
}

// ── Cone filter weight ──
// Cone-shaped falloff with half-width = halfWidth.
// T is the signed distance from the sample to the pixel center (in pixels).
float cone(float T, float halfWidth)
{
    return max(0.0, 1.0 - abs(T) / max(halfWidth, 0.001));
}

// ── Cylinder filter weight ──
// Uniform weight within halfWidth, zero outside.
float cylinder(float T, float halfWidth)
{
    return step(abs(T), halfWidth);
}

// ── Normalize with safe fallback ──
float2 SafeNorm(float2 v)
{
    float len2 = dot(v, v);
    return (len2 > 1e-8) ? v * rsqrt(len2) : float2(0, 0);
}

#endif  // MOTION_BLUR_COMMON_HLSL_
