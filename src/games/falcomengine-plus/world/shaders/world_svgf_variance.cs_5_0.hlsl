// world_svgf_variance.cs_5_0.hlsl - SVGF spatial variance estimate (world/docs/ROADMAP.md, SVGF M1). Paper Sec. 4.2
// (reference/svgf/svgf_preprint.md).
//
// Input: the integrated colour and temporal variance (t0, a = sigma'^2; a < 0 = not traced), the moments (t1, mu1', mu2',
// h), the guide (t2) and the normal (t3), all at the current frame. Output: u0 (the a-trous iteration 0 input) with the
// colour passed through and the variance sigma^2. Where the history is shorter than 4 frames (h < 4) the variance is the
// 7x7 spatial estimate with depth and normal weights (Eq. 4, 5 with offset q - p; no luminance term, plan default).
// Otherwise the temporal variance is kept. Debug view 11 (u1).

#include "world_svgf.hlsli"

Texture2D<float4> g_integrated : register(t0);
Texture2D<float4> g_history_moments : register(t1);
Texture2D<float4> g_history_guide : register(t2);
Texture2D<float2> g_history_normal : register(t3);

RWTexture2D<float4> g_atrous_write : register(u0);
RWTexture2D<float4> g_rtgi_debug : register(u1);

#define SVGF_VAR_TILE_SIDE 14

groupshared float gs_a[196];
groupshared float gs_z[196];
groupshared float2 gs_normal[196];
groupshared float2 gs_m[196];
groupshared uint gs_active;

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID, uint3 gtid : SV_GroupThreadID, uint3 gid : SV_GroupID)
{
    const uint lin = gtid.y * 8u + gtid.x;
    const int2 p = int2(id.xy);
    const int2 origin = int2(gid.xy) * 8 - 3;
    const bool inside = SvgfInside(p);
    const float4 c = inside ? g_integrated[p] : float4(0.0, 0.0, 0.0, -1.0);
    const float4 mp = inside ? g_history_moments[p] : float4(0.0, 0.0, 0.0, 0.0);
    if (lin == 0u) gs_active = 0u;
    GroupMemoryBarrierWithGroupSync();
    if (inside && c.a >= 0.0 && mp.z < kSvgfVarianceHistory) InterlockedOr(gs_active, 1u);
    GroupMemoryBarrierWithGroupSync();
    if (gs_active != 0u)
    {
        for (uint t = lin; t < 196u; t += 64u)
        {
            const int2 q = origin + int2(t % SVGF_VAR_TILE_SIDE, t / SVGF_VAR_TILE_SIDE);
            if (SvgfInside(q))
            {
                gs_a[t] = g_integrated[q].a;
                gs_z[t] = g_history_guide[q].x;
                gs_normal[t] = g_history_normal[q];
                gs_m[t] = g_history_moments[q].xy;
            }
        }
    }
    GroupMemoryBarrierWithGroupSync();
    if (!inside) return;
    const int debug_mode = (int)s_params.y;
    if (c.a < 0.0)
    {
        // Not traced: copied through, dark purple in the debug view.
        g_atrous_write[p] = c;
        if (debug_mode == 11) SvgfWriteDebugBlock(g_rtgi_debug, uint2(p), float3(0.25, 0.0, 0.25));
        return;
    }
    const float4 gp = g_history_guide[p];
    const float3 np = OctDecode(g_history_normal[p]);
    float sigma2 = c.a;  // h >= 4: the temporal variance
    if (mp.z < kSvgfVarianceHistory)
    {
        // Sec. 4.2: 7x7 bilateral estimate with depth and normal weights, centre weight 1. Taps outside the image,
        // not traced (a < 0) or without a surface (z <= 0) are skipped.
        float sum_w = 0.0;
        float sum_m1 = 0.0;
        float sum_m2 = 0.0;
        for (int j = -3; j <= 3; ++j)
        {
            for (int i = -3; i <= 3; ++i)
            {
                const int2 q = p + int2(i, j);
                if (!SvgfInside(q)) continue;
                const uint ti = uint(q.y - origin.y) * SVGF_VAR_TILE_SIDE + uint(q.x - origin.x);
                const float4 cq = float4(0.0, 0.0, 0.0, gs_a[ti]);
                const float4 gq = float4(gs_z[ti], 0.0, 0.0, 0.0);
                // The centre is never skipped by the guide gate (it is traced: a >= 0); other taps keep the skip rules.
                const bool centre = i == 0 && j == 0;
                if (!centre && (cq.a < 0.0 || gq.x <= 0.0)) continue;
                // Non-paper detail: Falcor also uses a luminance term and a 4/h boost (SVGFFilterMoments.ps.slang.txt:67,91-100,120).
                // The plan default follows the paper's depth and normal weights only.
                const float w = (i == 0 && j == 0)
                    ? 1.0
                    : SvgfDepthWeight(gp, gq, float2(i, j), kSvgfSigmaZ) * SvgfNormalWeight(np, OctDecode(gs_normal[ti]), kSvgfSigmaN);
                const float4 mq = float4(gs_m[ti], 0.0, 0.0);
                sum_w += w;
                sum_m1 += w * mq.x;
                sum_m2 += w * mq.y;
            }
        }
        // Guard: no weight sum (cannot happen with the centre included) keeps the centre's temporal variance.
        if (sum_w > 0.0)
        {
            const float mbar1 = sum_m1 / sum_w;
            const float mbar2 = sum_m2 / sum_w;
            sigma2 = max(0.0, mbar2 - mbar1 * mbar1);
        }
    }
    g_atrous_write[p] = float4(c.rgb, sigma2);
    if (debug_mode == 11)
    {
        // Debug 11: sigma / (1 + sigma) grey, blue set to 1 where h < 4.
        const float s = sigma2 / (1.0 + sigma2);
        const float blue = mp.z < kSvgfVarianceHistory ? 1.0 : s;
        SvgfWriteDebugBlock(g_rtgi_debug, uint2(p), float3(s, s, blue));
    }
}
