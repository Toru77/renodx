// ── Pass 3: Reconstruction (gather) ──
// Per-pixel gather of N jittered samples along dominant blur direction.
// Implements the full Guertin et al. 2013 algorithm with:
//   • Dual-direction sampling (vmax for even i, vc for odd i)
//   • Halton-jittered sample positions
//   • Relative depth-comparison (foreground/background disambiguation)
//   • Feature-aware velocity-direction weighting
//   • Center-weight preservation (Section 4.3)
//   • Stochastic tile-boundary jitter (Section 4.2)
//
// Input:  t0 = scene color (R16G16B16A16_FLOAT, full-res)
//         t1 = neighbormax velocity (R16G16_FLOAT, tile-res)
//         t2 = per-pixel velocity (R16G16_FLOAT, full-res)
//         t3 = view-space depth (R32_FLOAT, full-res)
// Output: u0 = blurred color (R16G16B16A16_FLOAT, full-res)
//
// SPDX-License-Identifier: MIT

#include "motion_blur_common.hlsl"

Texture2D<float4>   g_srcColor    : register(t0);
Texture2D<float2>   g_srcNeighborMax : register(t1);
Texture2D<float2>   g_srcVelocity : register(t2);
Texture2D<float>    g_srcDepth    : register(t3);
Texture3D<float2>   g_isfastNoise : register(t4);  // IS-FAST blue noise (128×128×32, RG8)

RWTexture2D<float4> g_outColor    : register(u0);

cbuffer cb_motion_blur : register(b13)
{
    float MB_enabled;           // c[0]
    float MB_sample_count;      // c[1] = N
    float MB_tile_size;         // c[2] = r
    float MB_inv_tile_size;     // c[3]
    float MB_strength;          // c[4]
    float MB_jitter_h;          // c[5] = h
    float MB_center_k;          // c[6] = k
    float MB_threshold_g;       // c[7] = g
    float MB_screen_w;          // c[8] — output/display width
    float MB_screen_h;          // c[9] — output/display height
    float MB_tile_grid_w;       // c[10]
    float MB_tile_grid_h;       // c[11]
    float MB_depth_w;           // c[12] — internal/depth buffer width
    float MB_depth_h;           // c[13] — internal/depth buffer height
    float MB_noise_type;        // c[14] — 0=Halton, 1=IS-FAST
    float MB_frame_index;       // c[15] — for IS-FAST animation
    float MB_velocity_scale;    // c[16] — velocity multiplier (unused here)
    float MB_sample_spread;     // c[17] — multiply sample distance along blur direction
    float MB_velocity_max;      // c[18] — velocity clamp (unused here)
    float MB_out_w;             // c[19] — full output width (for scaling working-res→full-res color reads)
    float MB_out_h;             // c[20] — full output height
    float MB_adaptive;          // c[21] — 0=off, 1=adaptive sample count
    float MB_debug_view;        // c[22] — 0=off, 1=velocity visualization
};

// Sample depth at internal resolution from output-resolution coordinates
float SampleDepth(uint2 pix)
{
    float2 uv = (float2(pix) + 0.5) / float2(MB_screen_w, MB_screen_h);
    uint2 dp = uint2(uv * float2(MB_depth_w, MB_depth_h));
    dp = min(dp, uint2((uint)MB_depth_w - 1, (uint)MB_depth_h - 1));
    return g_srcDepth[dp];
}

// Sample scene color at full output resolution from working-resolution coordinates.
// In half-res mode the dispatch runs at half the T0 texture dimensions, so pixel
// coordinates must be scaled up by MB_out_w / MB_screen_w before loading.
float4 SampleColor(uint2 pix)
{
    float2 scale = float2(MB_out_w / MB_screen_w, MB_out_h / MB_screen_h);
    int2 fp = int2(float2(pix) * scale + 0.5);
    fp = clamp(fp, int2(0,0), int2((uint)MB_out_w - 1, (uint)MB_out_h - 1));
    return g_srcColor[fp];
}

// Get jitter seed — Halton (deterministic) or IS-FAST (blue noise)
float2 GetJitter(uint2 pix)
{
    if (MB_noise_type > 0.5f) {
        // IS-FAST blue noise: 128×128×32 RG8 texture, point-wrap sampled
        float3 coord = float3(
            (pix.x & 127) / 128.0,
            (pix.y & 127) / 128.0,
            fmod(MB_frame_index * 0.125f, 1.0f)  // 8 frames per cycle
        );
        return g_isfastNoise.Load(int4((uint)(coord.x * 128), (uint)(coord.y * 128), (uint)(coord.z * 32), 0));
    }
    return HaltonJitter(pix);
}

// Stochastic tile offset for jittering the NeighborMax lookup near tile edges
float2 TileEdgeJitterOffset(uint2 pix)
{
    float2 r = float2(MB_tile_size, MB_tile_size);
    float2 uv = (float2(pix) + 0.5) / r;
    float2 fracPart = frac(uv);
    // Linear falloff: jitter more near tile edges (within 10% of tile border)
    float2 edgeDist = min(fracPart, 1.0 - fracPart) / 0.1;
    float2 jitterAmount = saturate(1.0 - edgeDist);
    // Use Halton for jitter direction
    float2 j = GetJitter(pix);
    j = j * 2.0 - 1.0;  // [-1, 1]
    return j * jitterAmount;
}

// Blend two normalized vectors (rnmix from paper)
float2 RNMix(float2 a, float2 b, float t)
{
    float2 blended = lerp(a, b, t);
    return SafeNorm(blended);
}

[numthreads(8, 8, 1)]
void main(uint2 pix : SV_DispatchThreadID)
{
    uint w = (uint)MB_screen_w;
    uint h = (uint)MB_screen_h;
    if (pix.x >= w || pix.y >= h) return;

    // ── Debug view: velocity visualization ──
    // Encode direction as R/G (horizontal+/vertical+), magnitude as brightness.
    // Gray = static. Useful for comparing against raw RTV4 game MV in devkit.
    if (MB_debug_view > 0.5f) {
        float2 vel = g_srcVelocity[pix];
        float scale = 0.02f;  // 1/50 px — maps 50px velocity to full color range
        float3 viz = float3(vel * scale * 0.5f + 0.5f, 0.0f);
        // Clamp to [0,1] range and saturate
        viz = saturate(viz);
        // Gray screen-tint for static pixels (velocity < 0.5px)
        if (length(vel) < 0.5f) viz = float3(0.15f, 0.15f, 0.15f);
        if (MB_out_w > MB_screen_w * 1.5f)
            g_outColor[pix] = float4(viz, 1.0f);
        else
            g_outColor[pix] = float4(viz, 1.0f);
        return;
    }

    uint N = (uint)MB_sample_count;
    int r = (int)MB_tile_size;
    float hParam = MB_jitter_h;
    float kParam = MB_center_k;
    float gParam = MB_threshold_g;
    float strength = MB_strength;

    // ── Step 1: Sample NeighborMax with stochastic tile jitter ──
    float2 jitter = TileEdgeJitterOffset(pix);
    float2 tileUV = (float2(pix) + 0.5 + jitter) / float2(r, r);
    float2 tileFrac = frac(tileUV);

    // Bilinear manual sample of neighbormax (nearest-neighbor is fine too,
    // but bilinear reduces tile-boundary artifacts)
    int2 tileBase = int2(floor(tileUV));
    float2 tileGrid = float2(MB_tile_grid_w, MB_tile_grid_h);
    int2 t00 = clamp(tileBase + int2(0,0), int2(0,0), int2(tileGrid - 1));
    int2 t10 = clamp(tileBase + int2(1,0), int2(0,0), int2(tileGrid - 1));
    int2 t01 = clamp(tileBase + int2(0,1), int2(0,0), int2(tileGrid - 1));
    int2 t11 = clamp(tileBase + int2(1,1), int2(0,0), int2(tileGrid - 1));

    float2 v00 = g_srcNeighborMax[t00];
    float2 v10 = g_srcNeighborMax[t10];
    float2 v01 = g_srcNeighborMax[t01];
    float2 v11 = g_srcNeighborMax[t11];

    // Pick the one with the largest magnitude (more conservative than bilinear blend)
    float len00 = dot(v00, v00);
    float len10 = dot(v10, v10);
    float len01 = dot(v01, v01);
    float len11 = dot(v11, v11);
    float2 vmax = v00;
    float bestLen = len00;
    if (len10 > bestLen) { vmax = v10; bestLen = len10; }
    if (len01 > bestLen) { vmax = v01; bestLen = len01; }
    if (len11 > bestLen) { vmax = v11; bestLen = len11; }

    // ── Step 2: Early-out if no significant motion ──
    if (length(vmax) * MB_sample_spread < 0.5) {
        if (MB_out_w > MB_screen_w * 1.5f)
            g_outColor[pix] = float4(SampleColor(pix).rgb, 0.0f);  // alpha=0 → upsample uses original T0
        else
            g_outColor[pix] = SampleColor(pix);
        return;
    }

    // Adaptive sample count: use fewer samples for slow motion
    if (MB_adaptive > 0.5f) {
        float vLen = length(vmax);
        if      (vLen < 2.0f)  N = max(4u,  N / 6u);
        else if (vLen < 10.0f) N = max(8u,  N / 3u);
        else if (vLen < 30.0f) N = max(12u, N / 2u);
    }

    // ── Step 3: Per-pixel velocity and center-composite direction ──
    float2 vp = g_srcVelocity[pix];
    float2 wn = SafeNorm(vmax);
    float2 wp = float2(-wn.y, wn.x);  // perpendicular to vmax
    // Flip wp to point toward vp (not away)
    if (dot(wp, vp) < 0.0) wp = -wp;
    // Blend vp toward wp as velocity magnitude decreases (Section 4.1)
    float vpLen = length(vp);
    float blendT = saturate((vpLen - 0.5) / gParam);
    float2 vc = RNMix(wp, SafeNorm(vp), blendT);

    // ── Step 4: Read depth at pixel center ──
    float depthPix = SampleDepth(pix);

    // ── Step 5: Center weight (Section 4.3) ──
    float totalWeight = (float)N / max(kParam * length(vc), 0.001);
    float4 result = SampleColor(pix) * totalWeight;

    // ── Step 6: Jitter seed and sample loop ──
    float2 jitSeed = GetJitter(pix);
    float jh = jitSeed.x * hParam;  // combined jitter

    [loop]
    for (uint i = 0; i < N; ++i) {
        // Tapered sample position within [-1, 1], jittered (paper Appendix A)
        float t = (float(i) + jh + 1.0) / (float(N) + 1.0);
        t = t * 2.0 - 1.0;  // [-1, 1]

        // Choose direction: vmax for even i, vc for odd i (50/50 split)
        float2 dir = (i & 1u) ? vc : wn;
        float T = t * length(vmax);
        int2 samplePos = int2(pix) + int2(t * dir * length(vmax) * MB_sample_spread + 0.5);
        samplePos = clamp(samplePos, int2(0,0), int2(w-1, h-1));

        // Read color, velocity, depth at sample point
        float4 colorSample = SampleColor(samplePos);
        float2 vs = g_srcVelocity[samplePos];
        float depthSample = SampleDepth(samplePos);

        // Foreground/background classification
        float f = zCompare(depthPix, depthSample);  // sample is in front of pixel
        float b = zCompare(depthSample, depthPix);  // sample is behind pixel

        // Velocity-aware directional weights (Section 4.1)
        float wA = dot(vc, dir);              // pixel's composite direction vs sample direction
        float wB = dot(SafeNorm(vs), dir);    // sample's velocity direction vs sample direction

        // Three phenomenological cases (paper Section 3/4.1)
        float weight =
            f * cone(T, 1.0 / max(length(vs), 0.001)) * max(wB, 0.0) +
            b * cone(T, 1.0 / max(vpLen, 0.001)) * max(wA, 0.0) +
            cylinder(T, min(length(vs), vpLen)) * max(wA, wB) * 2.0;

        weight = max(weight, 0.0);

        totalWeight += weight;
        result += colorSample * weight;
    }

    // ── Step 7: Normalize and blend with original ──
    result = (totalWeight > 0.001) ? result / totalWeight : SampleColor(pix);
    if (MB_out_w > MB_screen_w * 1.5f)
        g_outColor[pix] = float4(result.rgb, 1.0f);  // alpha=1 → upsample applies blur at this pixel
    else
        g_outColor[pix] = lerp(SampleColor(pix), result, strength);
}
