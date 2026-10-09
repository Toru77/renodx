// world_rtao_temporal.cs_5_0.hlsl - RTAO round 2, pass B: temporal accumulation (world/docs/ROADMAP.md, RTAO).
//
// One thread per pixel over the AO size, after pass A (world_rtao.cs_5_0.hlsl). Reads the raw AO (t0; -1 =
// neutral-invalid pixel), the game depth (t1), the MRT normal (t2), the motion (t3, texel units: the TAA
// interface) and the history (t4: R accumulated AO, G distance from camera, BA octahedral normal). Writes the AO
// texel (u0, r32_uint), the stats (u1, RTAO_TSTAT_* at index 10..15) and the new history (u2).
// Reprojection is CustomTAA_Reproject (taa/taa_common.hlsli): one implementation.

#include "../../taa/taa_common.hlsli"
#include "../../include/mrt_normal.hlsli"

Texture2D<float> g_raw_ao : register(t0);
Texture2D<float4> g_game_depth : register(t1);
Texture2D<uint4> g_mrt_normal : register(t2);
Texture2D<float4> g_motion : register(t3);
Texture2D<float4> g_history_read : register(t4);

RWTexture2D<uint> g_rtao_ao : register(u0);
RWStructuredBuffer<uint> g_rtao_stats : register(u1);
RWTexture2D<float4> g_history_write : register(u2);

SamplerState g_samp_point : register(s0);
SamplerState g_samp_linear : register(s1);

cbuffer cb_scene : register(b0)
{
    float4x4 viewInv_g : packoffset(c4);
    float4x4 viewProjInv_g : packoffset(c20);
};

// Push constants (rtao.hpp fills these; 20 floats).
cbuffer cb_rtao_temporal : register(b12)
{
    float4 g_t_params : packoffset(c0);   // history weight, depth rejection, normal rejection, history clamp (sigma)
    float4 g_t_texel : packoffset(c1);    // texel size xy, prevResolutionScale xy (1, 1)
    float4 g_t_size : packoffset(c2);     // width, height, debug mode (0 normal, 1 raw, 2 accumulated, 3 confidence), history valid (0 = reset)
    float4 g_t_reserved0 : packoffset(c3);
    float4 g_t_reserved1 : packoffset(c4);
};

#define RTAO_TSTAT_BASE 10u
#define RTAO_TSTAT_PIXELS 0u
#define RTAO_TSTAT_VALID_TAPS 1u
#define RTAO_TSTAT_REJECTED_DEPTH 2u
#define RTAO_TSTAT_REJECTED_NORMAL 3u
#define RTAO_TSTAT_OUT_OF_BOUNDS 4u
#define RTAO_TSTAT_RESET_PIXELS 5u
#define RTAO_TSTAT_COUNT 6u

groupshared uint gs_tstats[RTAO_TSTAT_COUNT];

// Octahedral normal encoding for the history (BA channels).
float2 OctEncode(float3 n)
{
    n /= (abs(n.x) + abs(n.y) + abs(n.z));
    float2 e = n.z >= 0.0 ? n.xy : (1.0 - abs(n.yx)) * sign(n.xy);
    return e;
}

float3 OctDecode(float2 e)
{
    float3 v = float3(e.x, e.y, 1.0 - abs(e.x) - abs(e.y));
    if (v.z < 0.0) v.xy = (1.0 - abs(v.yx)) * sign(v.xy);
    return normalize(v);
}

void TemporalPixel(uint2 px)
{
    const float2 size = g_t_size.xy;
    const float raw = g_raw_ao[px];
    const float history_weight = g_t_params.x;
    const float depth_rejection = g_t_params.y;
    const float normal_rejection = g_t_params.z;
    const float clamp_sigma = g_t_params.w;
    const int debug_mode = (int)g_t_size.z;
    const bool history_valid_frame = g_t_size.w > 0.5;

    InterlockedAdd(gs_tstats[RTAO_TSTAT_PIXELS], 1u);
    if (raw < 0.0)
    {
        // Neutral-invalid pixel (sky, scaled, outside region, invalid normal): history invalid, neutral AO.
        g_history_write[px] = float4(0.0, 0.0, 0.0, 0.0);
        g_rtao_ao[px] = 255u;
        InterlockedAdd(gs_tstats[RTAO_TSTAT_RESET_PIXELS], 1u);
        return;
    }

    const float depth = g_game_depth.Load(int3(px, 0)).x;
    const float3 N = DecodeFalcomMrtNormal(g_mrt_normal.Load(int3(px, 0)).xy);
    const float2 uv = (float2(px) + 0.5) / size;
    const float4 world_h = mul(float4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, depth, 1.0), viewProjInv_g);
    const float3 P = world_h.xyz / world_h.w;
    const float3 camera = float3(viewInv_g._m30, viewInv_g._m31, viewInv_g._m32);
    const float dist_cur = length(P - camera);

    float motion_px = 0.0;
    float depth_spread = 0.0;
    const float2 hist_uv = CustomTAA_Reproject(
        uv, g_game_depth, g_samp_point, g_motion, g_samp_linear,
        g_t_texel.xy, g_t_texel.zw, motion_px, depth_spread);
    const bool in_bounds = all(hist_uv >= 0.0) && all(hist_uv <= 1.0);
    if (!in_bounds) InterlockedAdd(gs_tstats[RTAO_TSTAT_OUT_OF_BOUNDS], 1u);

    // 2x2 bilinear history taps, each multiplied by the depth and normal accept flags, then renormalised.
    const float2 hp = hist_uv * size - 0.5;
    const int2 base = (int2)floor(hp);
    const float2 f = hp - floor(hp);
    float weight_total = 0.0;
    float weight_accepted = 0.0;
    float history_sum = 0.0;
    uint accepted_taps = 0u;
    [unroll]
    for (int t = 0; t < 4; ++t)
    {
        const int2 offset = int2(t & 1, t >> 1);
        const float w = (offset.x == 1 ? f.x : 1.0 - f.x) * (offset.y == 1 ? f.y : 1.0 - f.y);
        const int2 tap = clamp(base + offset, int2(0, 0), int2(size) - 1);
        weight_total += w;
        if (!in_bounds) continue;
        const float4 h = g_history_read.Load(int3(tap, 0));
        if (h.g <= 0.0) continue;  // invalid history (never written or reset)
        const bool depth_ok = abs(h.g - dist_cur) / max(dist_cur, 1e-4) <= depth_rejection;
        if (!depth_ok)
        {
            InterlockedAdd(gs_tstats[RTAO_TSTAT_REJECTED_DEPTH], 1u);
            continue;
        }
        const bool normal_ok = dot(OctDecode(h.ba), N) >= normal_rejection;
        if (!normal_ok)
        {
            InterlockedAdd(gs_tstats[RTAO_TSTAT_REJECTED_NORMAL], 1u);
            continue;
        }
        weight_accepted += w;
        history_sum += w * h.r;
        accepted_taps += 1u;
    }
    InterlockedAdd(gs_tstats[RTAO_TSTAT_VALID_TAPS], accepted_taps);

    // Renormalise; below epsilon the history counts as rejected.
    const float valid_fraction = (history_valid_frame && weight_total > 0.0 && weight_accepted > 1e-4)
        ? saturate(weight_accepted / weight_total) : 0.0;
    float history_ao = valid_fraction > 0.0 ? history_sum / weight_accepted : raw;
    if (!history_valid_frame) InterlockedAdd(gs_tstats[RTAO_TSTAT_RESET_PIXELS], 1u);

    // History clamp: 3x3 neighbourhood of the raw AO (valid pixels only), mean +/- k * sigma. k = 0 = off.
    if (clamp_sigma > 0.0 && valid_fraction > 0.0)
    {
        float s1 = 0.0, s2 = 0.0, n = 0.0;
        [unroll]
        for (int dy = -1; dy <= 1; ++dy)
        {
            [unroll]
            for (int dx = -1; dx <= 1; ++dx)
            {
                const int2 q = clamp(int2(px) + int2(dx, dy), int2(0, 0), int2(size) - 1);
                const float v = g_raw_ao[q];
                if (v < 0.0) continue;
                s1 += v;
                s2 += v * v;
                n += 1.0;
            }
        }
        const float mean = s1 / n;
        const float sigma = sqrt(max(s2 / n - mean * mean, 0.0));
        history_ao = clamp(history_ao, mean - clamp_sigma * sigma, mean + clamp_sigma * sigma);
    }

    const float alpha = history_weight * valid_fraction;
    const float ao = lerp(raw, history_ao, alpha);
    g_history_write[px] = float4(ao, dist_cur, OctEncode(N));

    // Debug modes write the chosen value as the texel (history is written normally above).
    float texel_value = ao;
    if (debug_mode == 1) texel_value = raw;
    else if (debug_mode == 2) texel_value = valid_fraction > 0.0 ? history_ao : 1.0;
    else if (debug_mode == 3) texel_value = valid_fraction;
    g_rtao_ao[px] = min(255u, (uint)round(saturate(texel_value) * 255.0));
}

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID, uint3 gid : SV_GroupThreadID)
{
    const uint group_index = gid.y * 8u + gid.x;
    if (group_index < RTAO_TSTAT_COUNT) gs_tstats[group_index] = 0u;
    GroupMemoryBarrierWithGroupSync();

    if (id.x < (uint)g_t_size.x && id.y < (uint)g_t_size.y) TemporalPixel(id.xy);

    GroupMemoryBarrierWithGroupSync();
    if (group_index < RTAO_TSTAT_COUNT) InterlockedAdd(g_rtao_stats[RTAO_TSTAT_BASE + group_index], gs_tstats[group_index]);
}
