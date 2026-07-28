// ── Pass 2: NeighborMax ──
// Dilates tile velocities by taking the 1-ring maximum neighborhood.
// Includes direction-aware off-axis culling (Section 4.4): diagonal tiles
// are only included if their velocity direction points toward the center tile.
//
// Input:  t0 = tilemax buffer (R16G16_FLOAT, ceil(w/r) × ceil(h/r))
// Output: u0 = neighbormax buffer (R16G16_FLOAT, same dimensions)
//
// SPDX-License-Identifier: MIT

#include "motion_blur_common.hlsl"

Texture2D<float2> g_srcTileMax : register(t0);
RWTexture2D<float2> g_outNeighborMax : register(u0);

cbuffer cb_motion_blur : register(b13)
{
    float MB_enabled;           // c[0]
    float MB_sample_count;      // c[1]
    float MB_tile_size;         // c[2] = r
    float MB_inv_tile_size;     // c[3]
    float MB_strength;          // c[4]
    float MB_jitter_h;          // c[5]
    float MB_center_k;          // c[6]
    float MB_threshold_g;       // c[7]
    float MB_screen_w;          // c[8]
    float MB_screen_h;          // c[9]
    float MB_tile_grid_w;       // c[10]
    float MB_tile_grid_h;       // c[11]
    float MB_depth_w;           // c[12] — unused here
    float MB_depth_h;           // c[13]
    float MB_noise_type;        // c[14]
    float MB_frame_index;       // c[15]
    float MB_velocity_scale;    // c[16]
    float MB_sample_spread;     // c[17]
    float MB_velocity_max;      // c[18]
    float MB_out_w;             // c[19]
    float MB_out_h;             // c[20]
    float MB_adaptive;          // c[21] — unused here
    float MB_debug_view;        // c[22] — 0=off, 1=velocity visualization
};

// Check whether a diagonal tile's velocity would affect the center tile.
bool DiagonalAffectsTile(float2 diagVel, float2 centerToDiag)
{
    // The diagonal tile blurs toward its own velocity direction.
    // If that direction points toward the center tile, it affects us.
    float2 dir = SafeNorm(diagVel);
    float2 toCenter = -centerToDiag;
    return dot(dir, SafeNorm(toCenter)) > 0.0;
}

[numthreads(8, 8, 1)]
void main(uint2 pix : SV_DispatchThreadID)
{
    uint tw = (uint)MB_tile_grid_w;
    uint th = (uint)MB_tile_grid_h;
    if (pix.x >= tw || pix.y >= th) return;

    float2 centerVal = g_srcTileMax[pix];
    float bestLenSq = dot(centerVal, centerVal);
    float2 bestVec = centerVal;

    // 1-ring: 8 neighbors
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            if (dx == 0 && dy == 0) continue;
            int2 n = int2(pix) + int2(dx, dy);
            if (n.x < 0 || n.x >= (int)tw || n.y < 0 || n.y >= (int)th) continue;

            // Diagonal culling (Section 4.4): only include diagonal tiles
            // if their velocity points toward this tile.
            if (abs(dx) == 1 && abs(dy) == 1) {
                float2 diagVel = g_srcTileMax[n];
                if (!DiagonalAffectsTile(diagVel, float2(dx, dy)))
                    continue;
            }

            float2 nv = g_srcTileMax[n];
            float lenSq = dot(nv, nv);
            if (lenSq > bestLenSq) {
                bestLenSq = lenSq;
                bestVec = nv;
            }
        }
    }

    g_outNeighborMax[pix] = bestVec;
}
