// ── Pass 1: TileMax ──
// Reduces the full-resolution velocity buffer to tile resolution.
// For each r×r pixel tile, finds the velocity vector with the largest magnitude.
// Uses a single-pass reduction with shared memory (each thread loads stride-wise).
//
// Input:  t0 = velocity buffer (R16G16_FLOAT, full-res)
// Output: u0 = tilemax buffer (R16G16_FLOAT, ceil(w/r) × ceil(h/r))
//
// SPDX-License-Identifier: MIT

#include "motion_blur_common.hlsl"

Texture2D<float2> g_srcVelocity : register(t0);
RWTexture2D<float2> g_outTileMax : register(u0);

// Push constants — see common.hlsl for slot definitions
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

groupshared float gs_mag[256];
groupshared float2 gs_vec[256];

[numthreads(256, 1, 1)]
void main(uint3 dt : SV_DispatchThreadID, uint3 gid : SV_GroupID, uint gi : SV_GroupIndex)
{
    uint tileX = gid.x;
    uint tileY = gid.y;
    uint r = (uint)MB_tile_size;
    uint tileBaseX = tileX * r;
    uint tileBaseY = tileY * r;
    uint tileW = min(r, (uint)MB_screen_w - tileBaseX);
    uint tileH = min(r, (uint)MB_screen_h - tileBaseY);
    uint tileSize = tileW * tileH;

    // Each thread loads multiple pixels in a stride over the tile
    float maxLenSq = 0.0;
    float2 bestVec = float2(0, 0);
    uint i = gi;
    while (i < tileSize) {
        uint px = tileBaseX + i % tileW;
        uint py = tileBaseY + i / tileW;
        float2 v = g_srcVelocity[uint2(px, py)];
        float lenSq = dot(v, v);
        if (lenSq > maxLenSq) {
            maxLenSq = lenSq;
            bestVec = v;
        }
        i += 256;
    }

    gs_mag[gi] = maxLenSq;
    gs_vec[gi] = bestVec;
    GroupMemoryBarrierWithGroupSync();

    // Shared-memory reduction (size 256 → 1)
    [unroll]
    for (uint s = 128; s > 0; s >>= 1) {
        if (gi < s) {
            if (gs_mag[gi + s] > gs_mag[gi]) {
                gs_mag[gi] = gs_mag[gi + s];
                gs_vec[gi] = gs_vec[gi + s];
            }
        }
        GroupMemoryBarrierWithGroupSync();
    }

    if (gi == 0) {
        g_outTileMax[uint2(tileX, tileY)] = gs_vec[0];
    }
}
