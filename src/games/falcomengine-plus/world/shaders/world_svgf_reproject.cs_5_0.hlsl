// world_svgf_reproject.cs_5_0.hlsl - SVGF reprojection (world/docs/ROADMAP.md, SVGF M1). Paper Sec. 4.1 and the guide data
// of Sec. 4.2 and 4.4 (reference/svgf/svgf_preprint.md).
//
// One thread per trace pixel tp. Its representative full-res pixel is rep = min(tp * d + d / 2, full - 1). Writes the
// integrated colour C' and the temporal variance sigma'^2 (u0: the integrated target when the spatial filter is on, else
// history colour), the moments mu1', mu2', h (u2), the guide (u3: z, dz/dx, dz/dy, dist) and the octahedral normal (u4).
// Counters (u1, RTGI_STAT_*) and debug views 4 and 5 (u5). Not traced (raw.a < 0) writes a = -1 and zero guide data.

#include "world_svgf.hlsli"

Texture2D<float4> g_raw_gi : register(t0);
Texture2D<float4> g_game_depth : register(t1);
Texture2D<uint4> g_mrt_normal : register(t2);
Texture2D<float4> g_motion : register(t3);
Texture2D<float4> g_history_color : register(t4);
Texture2D<float4> g_history_moments : register(t5);
Texture2D<float4> g_history_guide : register(t6);
Texture2D<float2> g_history_normal : register(t7);

RWTexture2D<float4> g_colour_write : register(u0);
RWStructuredBuffer<uint> g_rtgi_stats : register(u1);
RWTexture2D<float4> g_moments_write : register(u2);
RWTexture2D<float4> g_guide_write : register(u3);
RWTexture2D<float2> g_normal_write : register(u4);
RWTexture2D<float4> g_rtgi_debug : register(u5);

SamplerState g_samp_point : register(s0);
SamplerState g_samp_linear : register(s1);

#define RTGI_STAT_TEMPORAL_PIXELS 20u
#define RTGI_STAT_NO_HISTORY 21u
#define RTGI_STAT_RESET_PIXELS 22u
#define RTGI_TSTAT_COUNT 3u

groupshared uint gs_tstats[RTGI_TSTAT_COUNT];

// Centre and its four neighbours, for the one-sided depth gradients of the guide.
static const int2 kNeighbour[5] = {int2(0, 0), int2(1, 0), int2(-1, 0), int2(0, 1), int2(0, -1)};

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID, uint3 gid : SV_GroupThreadID)
{
    const uint group_index = gid.y * 8u + gid.x;
    if (group_index < RTGI_TSTAT_COUNT) gs_tstats[group_index] = 0u;
    GroupMemoryBarrierWithGroupSync();
    if (id.x < (uint)s_size.x && id.y < (uint)s_size.y)
    {
        const int2 tp = int2(id.xy);
        const uint2 full_size = uint2(s_size.zw);
        const uint d = max((uint)s_params.x, 1u);
        const int debug_mode = (int)s_params.y;
        const bool history_valid_frame = s_params.z > 0.5;
        const float4 raw = g_raw_gi[tp];
        // Not traced: dark purple in the debug views.
        float3 debug_color = float3(0.25, 0.0, 0.25);
        if (raw.a < 0.0)
        {
            g_colour_write[tp] = float4(0.0, 0.0, 0.0, -1.0);
            g_moments_write[tp] = float4(0.0, 0.0, 0.0, 0.0);
            g_guide_write[tp] = float4(0.0, 0.0, 0.0, 0.0);
            g_normal_write[tp] = float2(0.0, 0.0);
        }
        else
        {
            InterlockedAdd(gs_tstats[0], 1u);

            // Linear view depth z of the centre and its neighbours at their representative pixels (a neighbour is valid
            // when inside the trace image and its depth is positive). Sec. 4.4 uses the screen-space gradient of z.
            float z[5];
            bool zv[5];
            float3 P0 = float3(0.0, 0.0, 0.0);
            float2 uv0 = float2(0.0, 0.0);
            uint2 rep0 = uint2(0u, 0u);
            [unroll]
            for (int k = 0; k < 5; ++k)
            {
                const int2 q = tp + kNeighbour[k];
                zv[k] = SvgfInside(q);
                const uint2 rq = min(uint2(max(q, int2(0, 0))) * d + d / 2u, full_size - 1u);
                const float depth_q = g_game_depth.Load(int3(rq, 0)).x;
                zv[k] = zv[k] && depth_q > 0.0;
                const float2 uv_q = (float2(rq) + 0.5) / float2(full_size);
                const float4 world_h = mul(float4(uv_q.x * 2.0 - 1.0, 1.0 - uv_q.y * 2.0, depth_q, 1.0), viewProjInv_g);
                const float3 Pq = world_h.xyz / world_h.w;
                // No surface (depth <= 0, sky or no depth): z = 0 (plan: guide z = 0 means no surface; skipped by the guide.x <= 0 rules).
                z[k] = depth_q > 0.0 ? abs(dot(view_g._m02_m12_m22_m32, float4(Pq, 1.0))) : 0.0;
                if (k == 0)
                {
                    P0 = Pq;
                    uv0 = uv_q;
                    rep0 = rq;
                }
            }
            // One-sided differences to tp -1 and tp +1 per axis: the one with the smaller magnitude; the only valid one; or 0.
            const float gx = (zv[1] && zv[2])
                ? (abs(z[1] - z[0]) < abs(z[0] - z[2]) ? z[1] - z[0] : z[0] - z[2])
                : (zv[1] ? z[1] - z[0] : (zv[2] ? z[0] - z[2] : 0.0));
            const float gy = (zv[3] && zv[4])
                ? (abs(z[3] - z[0]) < abs(z[0] - z[4]) ? z[3] - z[0] : z[0] - z[4])
                : (zv[3] ? z[3] - z[0] : (zv[4] ? z[0] - z[4] : 0.0));
            const float dist_cur = length(P0 - float3(viewInv_g._m30, viewInv_g._m31, viewInv_g._m32));
            const float3 N = DecodeFalcomMrtNormalAsGame(g_mrt_normal.Load(int3(rep0, 0)).xy);
            const float4 guide = zv[0] ? float4(z[0], gx, gy, dist_cur) : float4(0.0, 0.0, 0.0, 0.0);
            const float l = SvgfLuminance(raw.rgb);

            float3 col = raw.rgb;
            float mu1 = l;
            float mu2 = l * l;
            float h = 1.0;
            bool blended = false;
            if (history_valid_frame)
            {
                // Sec. 4.1: bilinear 2x2 history taps at the reprojected position (CustomTAA_Reproject as the RTGI pass).
                float motion_px = 0.0;
                float depth_spread = 0.0;
                const float2 hist_uv = CustomTAA_Reproject(
                    uv0, g_game_depth, g_samp_point, g_motion, g_samp_linear,
                    float2(1.0, 1.0) / float2(full_size), float2(1.0, 1.0), motion_px, depth_spread);
                // Trace position of the reprojected representative pixel: (hist_uv * full - 0.5 - d / 2) / d.
                const float2 hp = (hist_uv * float2(full_size) - 0.5 - float(d / 2u)) / float(d);
                const int2 base = (int2)floor(hp);
                const float2 f = hp - floor(hp);
                // A tap is consistent when it is inside the trace image, has a surface and history (z > 0, h > 0),
                // passes the depth test and the normal test. Inconsistent taps lose their weight D, which is spread
                // uniformly over the consistent ones (Sec. 4.1). Plan default: the project's depth and normal tests
                // (TemporalDepthAccept, binary normal dot); Falcor uses fwidth-based tests (SVGFReproject.ps.slang.txt:61-78)
                // and renormalises proportionally (line 114-138); the paper's uniform rule is kept here.
                float wt[4];
                bool ok[4];
                float3 tc[4];
                float3 tm[4];
                float D = 0.0;
                uint nc = 0u;
                [unroll]
                for (int t = 0; t < 4; ++t)
                {
                    const int2 q = base + int2(t & 1, t >> 1);
                    wt[t] = ((t & 1) == 1 ? f.x : 1.0 - f.x) * ((t >> 1) == 1 ? f.y : 1.0 - f.y);
                    tc[t] = float3(0.0, 0.0, 0.0);
                    tm[t] = float3(0.0, 0.0, 0.0);
                    ok[t] = SvgfInside(q);
                    if (ok[t])
                    {
                        const float4 hg = g_history_guide.Load(int3(q, 0));
                        const float4 hm = g_history_moments.Load(int3(q, 0));
                        const float3 hn = OctDecode(g_history_normal.Load(int3(q, 0)));
                        const float4 hc = g_history_color.Load(int3(q, 0));
                        ok[t] = hg.x > 0.0 && hm.z > 0.0 && SvgfFinite(hc) && SvgfFinite(hm) && TemporalDepthAccept(hg.w, dist_cur, s_temporal.y)
                            && dot(hn, N) >= s_temporal.z;
                        tc[t] = hc.rgb;
                        tm[t] = hm.xyz;
                    }
                    if (ok[t]) nc += 1u;
                    else D += wt[t];
                }
                float3 hist_col = float3(0.0, 0.0, 0.0);
                float3 hist_mom = float3(0.0, 0.0, 0.0);
                if (nc > 0u)
                {
                    [unroll]
                    for (int t = 0; t < 4; ++t)
                    {
                        if (ok[t])
                        {
                            const float w = wt[t] + D / float(nc);
                            hist_col += w * tc[t];
                            hist_mom += w * tm[t];
                        }
                    }
                    blended = true;
                }
                else
                {
                    // Sec. 4.1: no consistent 2x2 tap, so a 3x3 neighbourhood of the nearest history pixel (uniform mean).
                    const int2 c = (int2)floor(hp + 0.5);
                    uint n3 = 0u;
                    for (int j = -1; j <= 1; ++j)
                    {
                        for (int i = -1; i <= 1; ++i)
                        {
                            const int2 q = c + int2(i, j);
                            if (!SvgfInside(q)) continue;
                            const float4 hg = g_history_guide.Load(int3(q, 0));
                            const float4 hm = g_history_moments.Load(int3(q, 0));
                            const float3 hn = OctDecode(g_history_normal.Load(int3(q, 0)));
                            const float4 hc = g_history_color.Load(int3(q, 0));
                            if (hg.x > 0.0 && hm.z > 0.0 && SvgfFinite(hc) && SvgfFinite(hm) && TemporalDepthAccept(hg.w, dist_cur, s_temporal.y)
                                && dot(hn, N) >= s_temporal.z)
                            {
                                n3 += 1u;
                                hist_col += hc.rgb;
                                hist_mom += hm.xyz;
                            }
                        }
                    }
                    if (n3 > 0u)
                    {
                        hist_col /= float(n3);
                        hist_mom /= float(n3);
                        blended = true;
                    }
                }
                if (blended)
                {
                    // Sec. 4.1: h = min(h_prev + 1, 32) (non-paper cap), alpha = max(1 - History Weight, 1 / h) (non-paper
                    // floor; the paper gives alpha = 0.2), C' = alpha C + (1 - alpha) C_prev. The moments use the same alpha
                    // (plan default; Falcor uses a separate moments alpha, SVGFReproject.ps.slang.txt:222).
                    h = min(hist_mom.z + 1.0, kSvgfMaxHistory);
                    const float alpha = max(s_temporal.x, 1.0 / h);
                    col = alpha * raw.rgb + (1.0 - alpha) * hist_col;
                    mu1 = alpha * l + (1.0 - alpha) * hist_mom.x;
                    mu2 = alpha * l * l + (1.0 - alpha) * hist_mom.y;
                }
            }
            if (!blended)
            {
                // Sec. 4.1: disocclusion or reset, C' = C, the moments of this frame, h = 1.
                InterlockedAdd(gs_tstats[1], 1u);
            }
            if (!history_valid_frame)
            {
                InterlockedAdd(gs_tstats[2], 1u);
            }
            // Sec. 4.2: sigma'^2 = mu2' - mu1'^2, clamped at 0.
            const float variance = max(0.0, mu2 - mu1 * mu1);
            g_colour_write[tp] = float4(col, variance);
            g_moments_write[tp] = float4(mu1, mu2, h, 0.0);
            g_guide_write[tp] = guide;
            g_normal_write[tp] = OctEncode(N);
            if (debug_mode == 4)
            {
                debug_color = col / (1.0 + col);
            }
            else if (debug_mode == 5)
            {
                debug_color = float3(h, h, h) / kSvgfMaxHistory;
            }
        }
        if (debug_mode == 4 || debug_mode == 5)
        {
            SvgfWriteDebugBlock(g_rtgi_debug, uint2(tp), debug_color);
        }
    }
    GroupMemoryBarrierWithGroupSync();
    if (group_index < RTGI_TSTAT_COUNT) InterlockedAdd(g_rtgi_stats[RTGI_STAT_TEMPORAL_PIXELS + group_index], gs_tstats[group_index]);
}
