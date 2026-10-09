// world_rtao_filter.cs_5_0.hlsl - RTAO spatial filter (RTAO D5, world/docs/ROADMAP.md).
//
// Denoises the final AO texel (AO = texel / 255, 0..1): the r32_uint texel that pass A (Temporal off) or pass B
// (Temporal on) wrote. One dispatch per pass; the chain comes from MakeFilterPlan (rtao_state.hpp).
// Separable (f_size.w = 0): one horizontal (f_pass.x = 0) or vertical (1) 1D pass. A-trous (f_size.w = 1): one 2D pass.
// Edge stops shared by both kernels: the depth plane distance (game depth through viewProjInv_g, as pass B) and the
// MRT normal dot. Load only, no samplers. The last pass (f_pass.w = 1) writes the uint output (u0) and accumulates
// the stats; in debug mode 8 it writes the filter change instead of the AO.

#include "../../include/mrt_normal.hlsli"

#define RTAO_FILTER_STAT_BASE 68u
#define RTAO_FILTER_STAT_PIXELS 0u
#define RTAO_FILTER_STAT_CHANGE_SUM 1u
#define RTAO_FILTER_STAT_CHANGED_GT1LSB 2u
#define RTAO_FILTER_STAT_COUNT 3u
#define kFilterPlaneSigma 0.02
#define kFilterNormalPower 16.0
#define kFilterMinSigma 0.5
#define kFilterChangeScale 8.0

Texture2D<float4> g_game_depth : register(t0);
Texture2D<uint4> g_mrt_normal : register(t1);
Texture2D<uint> g_in_uint : register(t2);    // input AO texel (f_flags.x = 1)
Texture2D<float> g_in_float : register(t3);  // input AO intermediate, 0..1 (f_flags.x = 0)
Texture2D<uint> g_original : register(t4);   // unfiltered AO (A): change statistic and debug mode 8

RWTexture2D<uint> g_out_uint : register(u0);
RWTexture2D<float> g_out_float : register(u1);
RWStructuredBuffer<uint> g_filter_stats : register(u2);

cbuffer cb_scene : register(b0)
{
    float4x4 viewInv_g : packoffset(c4);
    float4x4 viewProjInv_g : packoffset(c20);
    float2 resolutionScaling_g : packoffset(c49);
};

// Push constants (rtao.hpp fills these; 12 floats).
cbuffer cb_rtao_filter : register(b12)
{
    float4 f_size : packoffset(c0);   // width, height, radius, kind (0 separable, 1 a-trous)
    float4 f_pass : packoffset(c1);   // direction (0 horizontal, 1 vertical, 2 2D), step, quality (0 Low, 1 Medium, 2 High), last pass
    float4 f_flags : packoffset(c2);  // input is uint, output is uint, debug mode (8 = filter change), unused
};

float3 FilterPosition(int2 q, float depth)
{
    const float2 uv = (float2(q) + 0.5) / f_size.xy;
    const float4 h = mul(float4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, depth, 1.0), viewProjInv_g);
    return h.xyz / h.w;
}

// Edge weight of tap q against the centre (Pc, Nc, dist_c): 0 for a sky tap or an invalid normal.
float FilterEdgeWeight(int2 q, float3 Pc, float3 Nc, float dist_c)
{
    const float depth = g_game_depth.Load(int3(q, 0)).x;
    const float3 Nt = DecodeFalcomMrtNormal(g_mrt_normal.Load(int3(q, 0)).xy);
    if (depth <= 0.0 || dot(Nt, Nt) < 0.5) return 0.0;
    const float plane = dot(Nc, FilterPosition(q, depth) - Pc) / (kFilterPlaneSigma * dist_c);
    return exp(-0.5 * plane * plane) * pow(saturate(dot(Nc, Nt)), kFilterNormalPower);
}

float FilterInput(int2 q)
{
    return f_flags.x > 0.5 ? g_in_uint[q] / 255.0 : g_in_float[q];
}

// One tap: kernel weight times edge weight (the centre tap has edge weight 1). Taps outside the image are skipped.
void FilterTap(int2 q, float kernel_w, bool centre, float3 Pc, float3 Nc, float dist_c, inout float sum_w, inout float sum_v)
{
    if (any(q < 0) || any(q >= int2(f_size.xy))) return;
    const float w = kernel_w * (centre ? 1.0 : FilterEdgeWeight(q, Pc, Nc, dist_c));
    sum_w += w;
    sum_v += w * FilterInput(q);
}

// A-trous 1D weight at offset k (in steps): Medium and High B3 spline (1, 4, 6, 4, 1) / 16 for k = -2..2;
// Low (1, 6, 1) / 8 at k = -2, 0, 2 (only even k are visited).
float FilterAtrousAxis(int k, int quality)
{
    if (quality == 0) return k == 0 ? 0.75 : 0.125;
    static const float b3[5] = {1.0 / 16.0, 4.0 / 16.0, 6.0 / 16.0, 4.0 / 16.0, 1.0 / 16.0};
    return b3[k + 2];
}

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    const int2 px = int2(id.xy);
    if (any(px >= int2(f_size.xy))) return;

    const bool last = f_pass.w > 0.5;
    const int radius = (int)f_size.z;
    const int step = (int)f_pass.y;
    const int quality = (int)f_pass.z;
    const int direction = (int)f_pass.x;

    const float depth = g_game_depth.Load(int3(px, 0)).x;
    const float3 Nc = DecodeFalcomMrtNormal(g_mrt_normal.Load(int3(px, 0)).xy);
    const float value = FilterInput(px);
    const bool copy = depth <= 0.0 || dot(Nc, Nc) < 0.5 || any(resolutionScaling_g != float2(1.0, 1.0));

    float result = value;
    if (!copy)
    {
        const float3 Pc = FilterPosition(px, depth);
        const float dist_c = length(Pc - float3(viewInv_g._m30, viewInv_g._m31, viewInv_g._m32));
        float sum_w = 0.0;
        float sum_v = 0.0;
        if ((int)f_size.w == 0)
        {
            // Separable: Gaussian in the offset (pixels), sigma = max(radius / 2, kFilterMinSigma), taps at k * stride.
            const int half_taps = radius / step;
            const float sigma = radius * 0.5 > kFilterMinSigma ? radius * 0.5 : kFilterMinSigma;
            for (int k = -half_taps; k <= half_taps; ++k)
            {
                const float d = (float)(k * step) / sigma;
                const int2 offset = direction == 0 ? int2(k * step, 0) : int2(0, k * step);
                FilterTap(px + offset, exp(-0.5 * d * d), k == 0, Pc, Nc, dist_c, sum_w, sum_v);
            }
        }
        else
        {
            // A-trous: one 2D pass, offsets k * step per axis.
            for (int j = -2; j <= 2; ++j)
            {
                for (int i = -2; i <= 2; ++i)
                {
                    if (quality == 0 && (i % 2 != 0 || j % 2 != 0)) continue;
                    const float kernel_w = FilterAtrousAxis(i, quality) * FilterAtrousAxis(j, quality);
                    FilterTap(px + int2(i, j) * step, kernel_w, i == 0 && j == 0, Pc, Nc, dist_c, sum_w, sum_v);
                }
            }
        }
        result = sum_w > 0.0 ? sum_v / sum_w : value;
    }

    if (last)
    {
        const float original = g_original[px] / 255.0;
        if (!copy)
        {
            const float change = abs(result - original);
            InterlockedAdd(g_filter_stats[RTAO_FILTER_STAT_BASE + RTAO_FILTER_STAT_PIXELS], 1u);
            InterlockedAdd(g_filter_stats[RTAO_FILTER_STAT_BASE + RTAO_FILTER_STAT_CHANGE_SUM], (uint)(change * 1000.0));
            if (change > 1.0 / 255.0) InterlockedAdd(g_filter_stats[RTAO_FILTER_STAT_BASE + RTAO_FILTER_STAT_CHANGED_GT1LSB], 1u);
        }
        if ((int)f_flags.z == 8)
        {
            result = saturate(abs(result - original) * kFilterChangeScale);
        }
    }

    if (f_flags.y > 0.5)
    {
        g_out_uint[px] = min(255u, (uint)round(saturate(result) * 255.0));
    }
    else
    {
        g_out_float[px] = saturate(result);
    }
}
