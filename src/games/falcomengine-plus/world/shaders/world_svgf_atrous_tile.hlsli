// world_svgf_atrous_tile.hlsli - SVGF a-trous with the taps staged in group-shared memory (reference/svgf/optimizing_svgf.md
// Sec. 3.3). Output bit-identical to world_svgf_atrous when the scaled step equals SVGF_TILE_STEP.

#include "world_svgf.hlsli"

Texture2D<float4> g_input : register(t0);
Texture2D<float4> g_history_guide : register(t1);
Texture2D<float2> g_history_normal : register(t2);

RWTexture2D<float4> g_out : register(u0);

#define SVGF_TILE_SIDE (8 + 4 * SVGF_TILE_STEP)
#define SVGF_TILE_COUNT (SVGF_TILE_SIDE * SVGF_TILE_SIDE)

groupshared float4 gs_colour[SVGF_TILE_COUNT];
groupshared float gs_z[SVGF_TILE_COUNT];
groupshared float2 gs_normal[SVGF_TILE_COUNT];
groupshared uint gs_active;

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID, uint3 gtid : SV_GroupThreadID, uint3 gid : SV_GroupID)
{
    const uint lin = gtid.y * 8u + gtid.x;
    const int2 p = int2(id.xy);
    const int2 origin = int2(gid.xy) * 8 - 2 * SVGF_TILE_STEP;
    const bool inside = SvgfInside(p);
    const bool scaled = !all(resolutionScaling_g == float2(1.0, 1.0));
    const float4 c = inside ? g_input[p] : float4(0.0, 0.0, 0.0, -1.0);
    if (lin == 0u) gs_active = 0u;
    GroupMemoryBarrierWithGroupSync();
    if (inside && c.a >= 0.0 && !scaled) InterlockedOr(gs_active, 1u);
    GroupMemoryBarrierWithGroupSync();
    if (gs_active != 0u)
    {
        for (uint t = lin; t < SVGF_TILE_COUNT; t += 64u)
        {
            const int2 q = origin + int2(t % SVGF_TILE_SIDE, t / SVGF_TILE_SIDE);
            if (SvgfInside(q))
            {
                gs_colour[t] = g_input[q];
                gs_z[t] = g_history_guide[q].x;
                gs_normal[t] = g_history_normal[q];
            }
            else
            {
                gs_colour[t] = float4(0.0, 0.0, 0.0, 0.0);
                gs_z[t] = 0.0;
                gs_normal[t] = float2(0.0, 0.0);
            }
        }
    }
    GroupMemoryBarrierWithGroupSync();
    if (!inside) return;
    if (c.a < 0.0 || scaled)
    {
        g_out[p] = c;
        return;
    }
    const float4 gp = g_history_guide[p];
    const float3 np = OctDecode(g_history_normal[p]);
    const float lp = SvgfLuminance(c.rgb);

    // Eq. 6 driver: 3x3 [1 2 1] (x) [1 2 1] / 16 prefilter of the input variance at step 1, valid taps only.
    float gv_sum = 0.0;
    float gv_w = 0.0;
    for (int j = -1; j <= 1; ++j)
    {
        for (int i = -1; i <= 1; ++i)
        {
            const int2 q = p + int2(i, j);
            if (!SvgfInside(q)) continue;
            const uint ti = uint(q.y - origin.y) * SVGF_TILE_SIDE + uint(q.x - origin.x);
            const float4 cq = gs_colour[ti];
            // The centre is never skipped by the guide gate; other taps keep the skip rules.
            if (!(i == 0 && j == 0) && (cq.a < 0.0 || gs_z[ti] <= 0.0)) continue;
            const float g = (i == 0 ? 2.0 : 1.0) * (j == 0 ? 2.0 : 1.0);
            gv_sum += g * cq.a;
            gv_w += g;
        }
    }
    // Guard: with the centre included gv_w > 0; the centre's own variance is the fallback.
    const float gv_mean = gv_w > 0.0 ? gv_sum / gv_w : c.a;
    const float sigma_l = s_temporal.w * sqrt(max(0.0, gv_mean));

    // Sec. 4.3 kernel footprint: 5x5 taps at the scaled iteration spacing.
    const int step = (int)s_params.w * (int)clamp(round(s_spatial.w), 1.0, 4.0);
    float sum_hw = 0.0;
    float sum_hw_sq_var = 0.0;
    float3 sum_c = float3(0.0, 0.0, 0.0);
    for (int j = -2; j <= 2; ++j)
    {
        for (int i = -2; i <= 2; ++i)
        {
            const int2 q = p + int2(i, j) * step;
            if (!SvgfInside(q)) continue;
            const uint ti = uint(q.y - origin.y) * SVGF_TILE_SIDE + uint(q.x - origin.x);
            const float4 cq = gs_colour[ti];
            const float4 gq = float4(gs_z[ti], 0.0, 0.0, 0.0);
            // The centre is never skipped by the guide gate; other taps keep the skip rules.
            const bool centre = i == 0 && j == 0;
            if (!centre && (cq.a < 0.0 || gq.x <= 0.0)) continue;
            const float2 offset = float2(i, j) * float(step);
            const float h = kSvgfKernel[abs(i)] * kSvgfKernel[abs(j)];
            // Eq. 2: the centre has weight 1 (w_z = w_n = w_l = 1), so only h(0,0) enters Eq. 1 and Eq. 3 for it.
            const float w_z = centre ? 1.0 : SvgfDepthWeight(gp, gq, offset, s_spatial.x);
            const float w_n = centre ? 1.0 : SvgfNormalWeight(np, OctDecode(gs_normal[ti]), s_spatial.y);
            const float w_l = centre ? 1.0 : exp(-abs(lp - SvgfLuminance(cq.rgb)) / (sigma_l + kSvgfLuminanceEpsilon));
            const float hw = h * w_z * w_n * w_l;
            sum_c += hw * cq.rgb;
            sum_hw += hw;
            sum_hw_sq_var += hw * hw * cq.a;
        }
    }
    // Guard: a zero weight sum returns the centre value.
    if (sum_hw <= 0.0)
    {
        g_out[p] = c;
        return;
    }
    // Eq. 1 colour, Eq. 3 variance (the centre is included with its own weight 1 and kernel h(0,0)).
    g_out[p] = float4(sum_c / sum_hw, sum_hw_sq_var / (sum_hw * sum_hw));
}
