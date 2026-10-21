// world_rtgi_filter.cs_5_0.hlsl - RTGI spatial filter (world/docs/ROADMAP.md, RTGI M3b).
//
// The RTAO spatial filter (world_rtao_filter.cs_5_0.hlsl) on an rgb value at trace resolution, before the upsample.
// One dispatch per pass of the MakeFilterPlan chain (rtao_state.hpp). Separable (f_size.w = 0): one horizontal
// (f_pass.x = 0) or vertical (1) 1D pass with f_pass.z taps at stride 1, Gaussian, sigma = FilterSeparableSigma(radius).
// A-trous (f_size.w = 1): one 2D pass of f_pass.z x f_pass.z taps at spacing f_pass.y, binomial weights. Edge stops:
// FilterPlaneNormalWeight (world_rt_denoise.hlsli) on the depth plane and MRT normal at each trace pixel's
// representative pixel min(tq * d + d / 2, size - 1). Input t2 (a <= 0 = not traced: copied, and skipped as a tap),
// output u0. Load only, no samplers.

#include "../../include/mrt_normal.hlsli"
#include "world_rt_denoise.hlsli"

Texture2D<float4> g_game_depth : register(t0);
Texture2D<uint4> g_mrt_normal : register(t1);
Texture2D<float4> g_in : register(t2);

RWTexture2D<float4> g_out : register(u0);

cbuffer cb_scene : register(b0)
{
    float4x4 viewInv_g : packoffset(c4);
    float4x4 viewProjInv_g : packoffset(c20);
    float2 resolutionScaling_g : packoffset(c49);
};

// Push constants (rtgi.hpp fills these; 12 floats).
cbuffer cb_rtgi_filter : register(b12)
{
    float4 f_size : packoffset(c0);  // trace width, trace height, radius, kind (0 separable, 1 a-trous)
    float4 f_pass : packoffset(c1);  // direction (0 horizontal, 1 vertical, 2 2D), step, taps (odd), last pass
    float4 f_full : packoffset(c2);  // full width, full height, trace divisor d, unused
};

// World position and MRT normal of trace pixel tq at its representative pixel; false for sky or an invalid normal.
bool RepresentativeSurface(int2 tq, out float3 P, out float3 N)
{
    const uint d = max((uint)f_full.z, 1u);
    const uint2 full_size = uint2(f_full.xy);
    const uint2 px = min(uint2(tq) * d + d / 2u, full_size - 1u);
    const float depth = g_game_depth.Load(int3(px, 0)).x;
    N = DecodeFalcomMrtNormalAsGame(g_mrt_normal.Load(int3(px, 0)).xy);
    const float2 uv = (float2(px) + 0.5) / float2(full_size);
    const float4 h = mul(float4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, depth, 1.0), viewProjInv_g);
    P = h.xyz / h.w;
    return depth > 0.0 && dot(N, N) >= 0.5;
}

// One tap: kernel weight times the edge stop (1 at the centre). Taps outside the image or not traced are skipped.
void FilterTap(int2 q, float kernel_w, bool centre, float3 Pc, float3 Nc, float dist_c, inout float sum_w, inout float3 sum_v)
{
    if (any(q < 0) || any(q >= int2(f_size.xy))) return;
    const float4 t = g_in[q];
    if (t.a <= 0.0) return;
    float edge = 1.0;
    if (!centre)
    {
        float3 Pt;
        float3 Nt;
        edge = RepresentativeSurface(q, Pt, Nt) ? FilterPlaneNormalWeight(Pc, Nc, dist_c, Pt, Nt) : 0.0;
    }
    sum_w += kernel_w * edge;
    sum_v += kernel_w * edge * t.rgb;
}

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    const int2 px = int2(id.xy);
    if (any(px >= int2(f_size.xy))) return;

    const int radius = (int)f_size.z;
    const int step = (int)f_pass.y;
    const int taps = (int)f_pass.z;  // odd: 3..17 separable, 3..9 a-trous (the plan clamps and rounds up)
    const int half_taps = (taps - 1) / 2;
    const int direction = (int)f_pass.x;

    const float4 centre = g_in[px];
    float3 Pc;
    float3 Nc;
    const bool surface = RepresentativeSurface(px, Pc, Nc);
    float4 result = centre;
    if (centre.a > 0.0 && surface && all(resolutionScaling_g == float2(1.0, 1.0)))
    {
        const float dist_c = length(Pc - float3(viewInv_g._m30, viewInv_g._m31, viewInv_g._m32));
        float sum_w = 0.0;
        float3 sum_v = float3(0.0, 0.0, 0.0);
        if ((int)f_size.w == 0)
        {
            const float sigma = FilterSeparableSigma(radius);
            for (int k = -half_taps; k <= half_taps; ++k)
            {
                const int2 offset = direction == 0 ? int2(k, 0) : int2(0, k);
                FilterTap(px + offset, FilterGaussianWeight(k, sigma), k == 0, Pc, Nc, dist_c, sum_w, sum_v);
            }
        }
        else
        {
            // A-trous binomial row (1 2 1, 1 4 6 4 1, ...), integer recurrence (the row of world_rtao_filter.cs_5_0.hlsl).
            float wk[9];
            int c = 1;
            wk[0] = (float)c;
            for (int t = 0; t < taps - 1; ++t)
            {
                c = c * (taps - 1 - t) / (t + 1);
                wk[t + 1] = (float)c;
            }
            for (int j = -half_taps; j <= half_taps; ++j)
            {
                for (int i = -half_taps; i <= half_taps; ++i)
                {
                    FilterTap(px + int2(i, j) * step, wk[i + half_taps] * wk[j + half_taps], i == 0 && j == 0, Pc, Nc, dist_c, sum_w, sum_v);
                }
            }
        }
        if (sum_w > 0.0)
        {
            result = float4(sum_v / sum_w, 1.0);
        }
    }
    g_out[px] = result;
}
