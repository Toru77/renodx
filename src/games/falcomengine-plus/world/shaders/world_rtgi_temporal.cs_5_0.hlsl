// world_rtgi_temporal.cs_5_0.hlsl - RTGI temporal accumulation (world/docs/ROADMAP.md, RTGI M3b).
//
// One thread per trace pixel, after the RTGI trace (world_rtgi.cs_5_0.hlsl). The algorithm is RTAO pass B's
// (world_rtao_temporal.cs_5_0.hlsl) on an rgb value at trace resolution, through world_rt_denoise.hlsli: reprojection
// with CustomTAA_Reproject at the representative pixel min(tp * d + d / 2, size - 1), 2x2 bilinear history taps with the
// depth test and the soft normal weight, renormalised, clamped per channel to the 3x3 raw neighbourhood (History Clamp
// sigma, 0 = off), blended by History Weight times the valid fraction. Reads the raw GI (t0; a < 0 = not traced), the
// game depth (t1), the MRT normal (t2), the motion (t3, RTV4) and the history (t4 colour: rgb GI, a distance, 0 =
// invalid; t5 octahedral normal). Writes the new history (u0 colour, u2 normal), which is also this pass's output,
// counters (u1, RTGI_STAT_*) and debug views 4 and 5 (u3).

#include "../../taa/taa_common.hlsli"
#include "../../include/mrt_normal.hlsli"
#include "world_rt_denoise.hlsli"

Texture2D<float4> g_raw_gi : register(t0);
Texture2D<float4> g_game_depth : register(t1);
Texture2D<uint4> g_mrt_normal : register(t2);
Texture2D<float4> g_motion : register(t3);
Texture2D<float4> g_history_color : register(t4);
Texture2D<float2> g_history_normal : register(t5);

RWTexture2D<float4> g_history_color_write : register(u0);
RWStructuredBuffer<uint> g_rtgi_stats : register(u1);
RWTexture2D<float2> g_history_normal_write : register(u2);
RWTexture2D<float4> g_rtgi_debug : register(u3);

SamplerState g_samp_point : register(s0);
SamplerState g_samp_linear : register(s1);

cbuffer cb_scene : register(b0)
{
    float4x4 viewInv_g : packoffset(c4);
    float4x4 viewProjInv_g : packoffset(c20);
};

// Push constants (rtgi.hpp fills these; 20 floats).
cbuffer cb_rtgi_temporal : register(b12)
{
    float4 g_t_params : packoffset(c0);    // history weight, depth rejection, normal rejection, history clamp (sigma)
    float4 g_t_texel : packoffset(c1);     // full-res texel size xy, prevResolutionScale xy (1, 1)
    float4 g_t_size : packoffset(c2);      // trace width, trace height, debug mode, history valid (0 = reset)
    float4 g_t_full : packoffset(c3);      // full width, full height, trace divisor d, unused
    float4 g_t_reserved : packoffset(c4);  // unused
};

#define RTGI_STAT_TEMPORAL_PIXELS 20u
#define RTGI_STAT_NO_HISTORY 21u
#define RTGI_STAT_RESET_PIXELS 22u
#define RTGI_TSTAT_COUNT 3u

groupshared uint gs_tstats[RTGI_TSTAT_COUNT];

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID, uint3 gid : SV_GroupThreadID)
{
    const uint group_index = gid.y * 8u + gid.x;
    if (group_index < RTGI_TSTAT_COUNT) gs_tstats[group_index] = 0u;
    GroupMemoryBarrierWithGroupSync();
    if (id.x < (uint)g_t_size.x && id.y < (uint)g_t_size.y)
    {
        const uint2 tp = id.xy;
        const int2 trace_size = int2(g_t_size.xy);
        const uint2 full_size = uint2(g_t_full.xy);
        const uint d = max((uint)g_t_full.z, 1u);
        const uint2 px = min(tp * d + d / 2u, full_size - 1u);
        const int debug_mode = (int)g_t_size.z;
        const float4 raw = g_raw_gi[tp];
        // Not traced: dark purple in the debug views.
        float3 debug_color = float3(0.25, 0.0, 0.25);
        if (raw.a < 0.0)
        {
            // Not traced (sky, invalid normal, scaled, outside the region): history invalid (distance 0).
            g_history_color_write[tp] = float4(0.0, 0.0, 0.0, 0.0);
            g_history_normal_write[tp] = float2(0.0, 0.0);
        }
        else
        {
            InterlockedAdd(gs_tstats[0], 1u);
            const bool history_valid_frame = g_t_size.w > 0.5;
            const float depth = g_game_depth.Load(int3(px, 0)).x;
            const float3 N = DecodeFalcomMrtNormalAsGame(g_mrt_normal.Load(int3(px, 0)).xy);
            const float2 uv = (float2(px) + 0.5) / float2(full_size);
            const float4 world_h = mul(float4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, depth, 1.0), viewProjInv_g);
            const float3 P = world_h.xyz / world_h.w;
            const float dist_cur = length(P - float3(viewInv_g._m30, viewInv_g._m31, viewInv_g._m32));
            float motion_px = 0.0;
            float depth_spread = 0.0;
            const float2 hist_uv = CustomTAA_Reproject(
                uv, g_game_depth, g_samp_point, g_motion, g_samp_linear, g_t_texel.xy, g_t_texel.zw, motion_px, depth_spread);
            const bool in_bounds = all(hist_uv >= 0.0) && all(hist_uv <= 1.0);
            // 2x2 bilinear history taps at trace resolution, each weighted by the depth test and the soft normal weight.
            const float2 hp = hist_uv * float2(trace_size) - 0.5;
            const int2 base = (int2)floor(hp);
            const float2 f = hp - floor(hp);
            float weight_total = 0.0;
            float weight_accepted = 0.0;
            float3 history_sum = float3(0.0, 0.0, 0.0);
            [unroll]
            for (int t = 0; t < 4; ++t)
            {
                const int2 offset = int2(t & 1, t >> 1);
                const float w = (offset.x == 1 ? f.x : 1.0 - f.x) * (offset.y == 1 ? f.y : 1.0 - f.y);
                const int2 tap = clamp(base + offset, int2(0, 0), trace_size - 1);
                weight_total += w;
                if (!in_bounds) continue;
                const float4 h = g_history_color.Load(int3(tap, 0));
                if (h.a <= 0.0) continue;  // invalid history (never written or reset)
                if (!TemporalDepthAccept(h.a, dist_cur, g_t_params.y)) continue;
                const float wn = TemporalNormalWeight(dot(OctDecode(g_history_normal.Load(int3(tap, 0))), N), g_t_params.z);
                weight_accepted += w * wn;
                history_sum += w * wn * h.rgb;
            }
            const float valid_fraction = (history_valid_frame && weight_total > 0.0 && weight_accepted > 1e-4)
                ? saturate(weight_accepted / weight_total) : 0.0;
            float3 history = valid_fraction > 0.0 ? history_sum / weight_accepted : raw.rgb;
            if (!history_valid_frame) InterlockedAdd(gs_tstats[RTGI_STAT_RESET_PIXELS - RTGI_STAT_TEMPORAL_PIXELS], 1u);
            if (valid_fraction <= 0.0) InterlockedAdd(gs_tstats[RTGI_STAT_NO_HISTORY - RTGI_STAT_TEMPORAL_PIXELS], 1u);
            // History clamp, per channel: the 3x3 neighbourhood of the raw GI (traced pixels only), mean +/- k * sigma.
            if (g_t_params.w > 0.0 && valid_fraction > 0.0)
            {
                float3 s1 = float3(0.0, 0.0, 0.0);
                float3 s2 = float3(0.0, 0.0, 0.0);
                float n = 0.0;
                [unroll]
                for (int dy = -1; dy <= 1; ++dy)
                {
                    [unroll]
                    for (int dx = -1; dx <= 1; ++dx)
                    {
                        const int2 q = clamp(int2(tp) + int2(dx, dy), int2(0, 0), trace_size - 1);
                        const float4 v = g_raw_gi[q];
                        if (v.a < 0.0) continue;
                        s1 += v.rgb;
                        s2 += v.rgb * v.rgb;
                        n += 1.0;
                    }
                }
                history.r = ClampToNeighbourhood(history.r, s1.r, s2.r, n, g_t_params.w);
                history.g = ClampToNeighbourhood(history.g, s1.g, s2.g, n, g_t_params.w);
                history.b = ClampToNeighbourhood(history.b, s1.b, s2.b, n, g_t_params.w);
            }
            const float3 gi = lerp(raw.rgb, history, g_t_params.x * valid_fraction);
            g_history_color_write[tp] = float4(gi, dist_cur);
            g_history_normal_write[tp] = OctEncode(N);
            if (debug_mode == 4)
            {
                debug_color = gi / (1.0 + gi);
            }
            else if (debug_mode == 5)
            {
                debug_color = float3(valid_fraction, valid_fraction, valid_fraction);
            }
        }
        if (debug_mode == 4 || debug_mode == 5)
        {
            for (uint by = 0u; by < d; ++by)
            {
                for (uint bx = 0u; bx < d; ++bx)
                {
                    const uint2 block_px = tp * d + uint2(bx, by);
                    if (all(block_px < full_size))
                    {
                        g_rtgi_debug[block_px] = float4(debug_color, 1.0);
                    }
                }
            }
        }
    }
    GroupMemoryBarrierWithGroupSync();
    if (group_index < RTGI_TSTAT_COUNT) InterlockedAdd(g_rtgi_stats[RTGI_STAT_TEMPORAL_PIXELS + group_index], gs_tstats[group_index]);
}
