// ── Pass 0: Velocity buffer generation from depth + camera matrices ──
// Dispatched at output resolution. Depth is at internal resolution —
// sampled via UV scaling for correct coordinate mapping.
//
// Input:  t0 = depth texture (R32_FLOAT, internal-res)
// Output: u0 = velocity buffer (R16G16_FLOAT, output-res)
//
// SPDX-License-Identifier: MIT

#include "motion_blur_common.hlsl"

Texture2D<float> g_srcDepth : register(t0);
RWTexture2D<float2> g_outVelocity : register(u0);

cbuffer cb_scene : register(b0)
{
  float4x4 view_g          : packoffset(c0);
  float4x4 viewInv_g       : packoffset(c4);
  float4x4 proj_g          : packoffset(c8);
  float4x4 projInv_g       : packoffset(c12);
  float4x4 viewProj_g      : packoffset(c16);
  float4x4 viewProjInv_g   : packoffset(c20);
  float2 vpSize_g          : packoffset(c24);
  float2 invVPSize_g       : packoffset(c24.z);
#ifdef RENODX_KAI
  float4x4 prevViewProj_g  : packoffset(c85);
#else
  float4x4 prevViewProj_g  : packoffset(c74);
#endif
};

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
    float MB_screen_w;          // c[8] — output/display width
    float MB_screen_h;          // c[9] — output/display height
    float MB_tile_grid_w;       // c[10]
    float MB_tile_grid_h;       // c[11]
    float MB_depth_w;           // c[12] — internal/depth buffer width
    float MB_depth_h;           // c[13] — internal/depth buffer height
    float MB_noise_type;        // c[14] — 0=Halton, 1=IS-FAST
    float MB_frame_index;       // c[15] — frame counter
    float MB_velocity_scale;    // c[16] — multiply motion vectors
    float MB_sample_spread;     // c[17] — sample spread (unused here)
    float MB_velocity_max;      // c[18] — max velocity soft-clamp (0=off)
    float MB_out_w;             // c[19] — full output width
    float MB_out_h;             // c[20] — full output height
    float MB_adaptive;          // c[21] — adaptive sampling (unused here)
    float MB_debug_view;        // c[22] — 0=off, 1=velocity visualization
};

float LinearizeDepth(float d)
{
    float z_ndc = d * 2.0 - 1.0;
    return proj_g[3][2] / (z_ndc - proj_g[2][2]);
}

[numthreads(8, 8, 1)]
void main(uint2 pix : SV_DispatchThreadID)
{
    uint w = (uint)MB_screen_w;
    uint h = (uint)MB_screen_h;
    if (pix.x >= w || pix.y >= h) return;

    // UV at output resolution → sample depth at internal resolution
    float2 uv = (float2(pix) + 0.5) / float2(w, h);
    uint2 depthPix = uint2(uv * float2(MB_depth_w, MB_depth_h));
    depthPix = min(depthPix, uint2((uint)MB_depth_w - 1, (uint)MB_depth_h - 1));
    float depthRaw = g_srcDepth[depthPix];

    // NDC from output-res UV
    float2 ndc = uv * 2.0 - float2(1.0, 1.0);
    ndc.y = -ndc.y;
    float4 clipPos = float4(ndc, depthRaw, 1.0);

    float4 worldPos = mul(clipPos, viewProjInv_g);
    worldPos /= worldPos.w;

    float4 prevClip = mul(worldPos, prevViewProj_g);
    float2 prevNDC = prevClip.xy / prevClip.w;

    // Motion vector in output-res pixel units
    float2 motion = (ndc - prevNDC) * float2(w, h) * 0.5;

    float2 vel = motion * MB_velocity_scale;
    if (MB_velocity_max > 0.5) {
        float len = length(vel);
        if (len > 0.001) {
            float clamped = len / (1.0 + len / MB_velocity_max);
            vel *= clamped / len;
        }
    }
    g_outVelocity[pix] = vel;
}
