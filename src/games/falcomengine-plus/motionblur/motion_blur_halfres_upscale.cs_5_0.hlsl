// ── Half-res upsample: working res → output res ──
// Bilinear upsample from half (or internal) resolution to full display resolution,
// then composites the upsampled blur with the original full-res T0.
// Used when Half Resolution mode is enabled.
//
// Input:  t0 = blurred output (R16G16B16A16_FLOAT, working-res)
//         t1 = original scene color (R16G16B16A16_FLOAT, output-res)
// Output: u0 = composited result (R16G16B16A16_FLOAT, output-res)
//
// SPDX-License-Identifier: MIT

Texture2D<float4> g_srcBlur : register(t0);
Texture2D<float4> g_srcOriginal : register(t1);
RWTexture2D<float4> g_outFullres : register(u0);

cbuffer cb_motion_blur : register(b13)
{
    float MB_enabled;
    float MB_sample_count;
    float MB_tile_size;
    float MB_inv_tile_size;
    float MB_strength;
    float MB_jitter_h;
    float MB_center_k;
    float MB_threshold_g;
    float MB_screen_w;          // c[8] — working/source width
    float MB_screen_h;          // c[9] — working/source height
    float MB_tile_grid_w;       // c[10]
    float MB_tile_grid_h;       // c[11]
    float MB_depth_w;           // c[12]
    float MB_depth_h;           // c[13]
    float MB_noise_type;        // c[14]
    float MB_frame_index;       // c[15]
    float MB_velocity_scale;    // c[16]
    float MB_sample_spread;     // c[17]
    float MB_velocity_max;      // c[18]
    float MB_out_w;             // c[19] — output/dest width
    float MB_out_h;             // c[20] — output/dest height
    float MB_adaptive;          // c[21] — unused here
    float MB_debug_view;        // c[22] — 0=off, 1=velocity visualization
};

float4 SampleBilinear(float2 uv, uint2 srcSize)
{
    float2 coord = uv * float2(srcSize) - 0.5;
    int2 base = int2(floor(coord));
    float2 frac = coord - float2(base);
    int2 c00 = clamp(base + int2(0,0), int2(0,0), int2(srcSize) - 1);
    int2 c10 = clamp(base + int2(1,0), int2(0,0), int2(srcSize) - 1);
    int2 c01 = clamp(base + int2(0,1), int2(0,0), int2(srcSize) - 1);
    int2 c11 = clamp(base + int2(1,1), int2(0,0), int2(srcSize) - 1);
    return lerp(lerp(g_srcBlur[c00], g_srcBlur[c10], frac.x),
                lerp(g_srcBlur[c01], g_srcBlur[c11], frac.x), frac.y);
}

[numthreads(8, 8, 1)]
void main(uint2 pix : SV_DispatchThreadID)
{
    uint ow = (uint)MB_out_w;
    uint oh = (uint)MB_out_h;
    if (pix.x >= ow || pix.y >= oh) return;
    float2 uv = (float2(pix) + 0.5) / float2(ow, oh);
    uint2 srcSize = uint2((uint)MB_screen_w, (uint)MB_screen_h);
    float4 blurred = SampleBilinear(uv, srcSize);
    float blurAmount = MB_strength * blurred.a;
    g_outFullres[pix] = lerp(g_srcOriginal[pix], blurred, blurAmount);
}
