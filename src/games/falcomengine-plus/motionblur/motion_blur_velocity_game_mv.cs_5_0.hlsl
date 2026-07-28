// ── Pass 0 (game-mv variant): Velocity from game's motion vectors ──
// Reads the game's resolved motion vector texture (RTV4 from sky shader)
// instead of recomputing from depth + camera matrices.
// Format: o4.xy = (prevPixelPos - currPixelPos) + jitterDiff  (pixel units, R16G16)
//
// Input:  t0 = game motion vectors (R16G16_FLOAT, at game internal resolution)
// Output: u0 = velocity buffer (R16G16_FLOAT, at working/output resolution)
//
// The game MV texture may be at a different resolution than the output,
// so we use UV-based coordinate remapping (like the depth shader).
// MB_depth_w/h carry the game MV texture dimensions (set from captured_mv_width/height).
//
// SPDX-License-Identifier: MIT

Texture2D<float2> g_gameMotionVectors : register(t0);
RWTexture2D<float2> g_outVelocity : register(u0);

cbuffer cb_motion_blur : register(b13)
{
    float MB_enabled;           // c[0]
    float MB_sample_count;      // c[1]
    float MB_tile_size;         // c[2]
    float MB_inv_tile_size;     // c[3]
    float MB_strength;          // c[4]
    float MB_jitter_h;          // c[5]
    float MB_center_k;          // c[6]
    float MB_threshold_g;       // c[7]
    float MB_screen_w;          // c[8]  — working/output width
    float MB_screen_h;          // c[9]  — working/output height
    float MB_tile_grid_w;       // c[10]
    float MB_tile_grid_h;       // c[11]
    float MB_depth_w;           // c[12] — game MV texture width
    float MB_depth_h;           // c[13] — game MV texture height
    float MB_noise_type;        // c[14]
    float MB_frame_index;       // c[15]
    float MB_velocity_scale;    // c[16]
    float MB_sample_spread;     // c[17]
    float MB_velocity_max;      // c[18]
    float MB_out_w;             // c[19]
    float MB_out_h;             // c[20]
    float MB_adaptive;          // c[21]
    float MB_debug_view;        // c[22] — 0=off, 1=velocity visualization
};

[numthreads(8, 8, 1)]
void main(uint2 pix : SV_DispatchThreadID)
{
    uint w = (uint)MB_screen_w;
    uint h = (uint)MB_screen_h;
    if (pix.x >= w || pix.y >= h) return;

    // UV-based coordinate remapping: working-res pix → game-MV-res texel
    float2 uv = (float2(pix) + 0.5) / float2(w, h);
    uint2 mvPix = uint2(uv * float2(MB_depth_w, MB_depth_h));
    mvPix = min(mvPix, uint2((uint)MB_depth_w - 1, (uint)MB_depth_h - 1));
    float2 mv = g_gameMotionVectors[mvPix];

    // Game format: prevPixel - currPixel + jitter
    // We need currPixel - prevPixel (current→previous direction for blur)
    float2 vel = -mv * MB_velocity_scale;

    // Scale from game-MV pixel units to working-res pixel units
    vel *= float2(MB_screen_w / MB_depth_w, MB_screen_h / MB_depth_h);

    if (MB_velocity_max > 0.5) {
        float len = length(vel);
        if (len > 0.001) {
            float clamped = len / (1.0 + len / MB_velocity_max);
            vel *= clamped / len;
        }
    }
    g_outVelocity[pix] = vel;
}
