// world_svgf_atrous.cs_5_0.hlsl - SVGF a-trous wavelet iteration (world/docs/ROADMAP.md, SVGF M1). Paper Sec. 4.3 Eq. 1 to 3
// and the Sec. 4.4 edge stops Eq. 4 to 6 (reference/svgf/svgf_preprint.md).
//
// One iteration i with step s = 2^i (params.w) times the radius scale. Input t0 (rgb colour, a = variance; a < 0 = not traced), guide t1, normal
// t2 of the history (current frame). Output u0. The centre is copied through when not traced or when the resolution
// scaling is not 1 (as the RTAO and RTGI filters).
//   Eq. 1: C'(p) = sum(h w C(q)) / sum(h w)
//   Eq. 2: w = w_z w_n w_l, centre weight 1
//   Eq. 3: Var'(p) = sum(h^2 w^2 Var(q)) / (sum(h w))^2
//   Eq. 4: w_z = depth weight (SvgfDepthWeight), Eq. 5: w_n = normal weight (SvgfNormalWeight)
//   Eq. 6: w_l = exp(-|l_p - l_q| / (sigma_l sqrt(g3x3(Var)(p)) + eps_l)), with the 3x3 [1 2 1] (x) [1 2 1] / 16 prefilter of
//   the input variance at step 1 that drives only w_l (Sec. 4.4). Non-paper: eps_l = 1e-6 (plan default; Falcor uses
//   1e-10 under the sqrt, SVGFAtrous.ps.slang.txt:96).

#include "world_svgf.hlsli"

Texture2D<float4> g_input : register(t0);
Texture2D<float4> g_history_guide : register(t1);
Texture2D<float2> g_history_normal : register(t2);

RWTexture2D<float4> g_out : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    if (!SvgfInside(int2(id.xy))) return;
    const int2 p = int2(id.xy);
    const float4 c = g_input[p];
    if (c.a < 0.0 || !all(resolutionScaling_g == float2(1.0, 1.0)))
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
            const float4 cq = g_input[q];
            // The centre is never skipped by the guide gate; other taps keep the skip rules.
            if (!(i == 0 && j == 0) && (cq.a < 0.0 || g_history_guide[q].x <= 0.0)) continue;
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
            const float4 cq = g_input[q];
            const float4 gq = g_history_guide[q];
            // The centre is never skipped by the guide gate; other taps keep the skip rules.
            const bool centre = i == 0 && j == 0;
            if (!centre && (cq.a < 0.0 || gq.x <= 0.0)) continue;
            const float2 offset = float2(i, j) * float(step);
            const float h = kSvgfKernel[abs(i)] * kSvgfKernel[abs(j)];
            // Eq. 2: the centre has weight 1 (w_z = w_n = w_l = 1), so only h(0,0) enters Eq. 1 and Eq. 3 for it.
            const float w_z = centre ? 1.0 : SvgfDepthWeight(gp, gq, offset, s_spatial.x);
            const float w_n = centre ? 1.0 : SvgfNormalWeight(np, OctDecode(g_history_normal[q]), s_spatial.y);
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
