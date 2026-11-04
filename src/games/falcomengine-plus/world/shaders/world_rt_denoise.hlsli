#pragma once

// world_rt_denoise.hlsli - denoiser math shared by the SVGF passes (world_svgf.hlsli), the upsample (world_rtgi_upsample) and the GTVBAO main pass (gtvbao_main, upsample skip).
// Pure functions; no resources or constant buffers.

// Soft normal weight: a history tap counts fully at dot >= the Normal Rejection value and falls linearly to 0 over this width.
static const float kNormalRampWidth = 0.4;
#define kFilterPlaneSigma 0.02
#define kFilterNormalPower 16.0
#define kFilterMinSigma 0.5

// Octahedral normal encoding for the history.
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

// History tap depth test: the relative distance difference is at most depth_rejection.
bool TemporalDepthAccept(float history_dist, float dist_cur, float depth_rejection)
{
    return abs(history_dist - dist_cur) / max(dist_cur, 1e-4) <= depth_rejection;
}

// History tap normal weight (see kNormalRampWidth).
float TemporalNormalWeight(float tap_dot, float normal_rejection)
{
    return saturate((tap_dot - (normal_rejection - kNormalRampWidth)) / kNormalRampWidth);
}

// Spatial edge stop of a tap (Pt, Nt) against the centre (Pc, Nc, dist_c): depth-plane distance and normal dot.
float FilterPlaneNormalWeight(float3 Pc, float3 Nc, float dist_c, float3 Pt, float3 Nt)
{
    const float plane = dot(Nc, Pt - Pc) / (kFilterPlaneSigma * dist_c);
    return exp(-0.5 * plane * plane) * pow(saturate(dot(Nc, Nt)), kFilterNormalPower);
}

// Separable Gaussian sigma (pixels) of a Filter Radius: radius / 2, at least kFilterMinSigma.
float FilterSeparableSigma(float radius)
{
    return radius * 0.5 > kFilterMinSigma ? radius * 0.5 : kFilterMinSigma;
}

// Separable Gaussian weight of offset k.
float FilterGaussianWeight(int k, float sigma)
{
    const float d = (float)k / sigma;
    return exp(-0.5 * d * d);
}

// Representative full-res pixel of trace pixel tq at divisor d: the centre of its d x d block, clamped to the image.
uint2 RtTraceRepresentativePixel(uint2 tq, uint d, uint2 full_size) { return min(tq * d + d / 2u, full_size - 1u); }

// The upsample's tap rejection at the tap's representative pixel. A NaN normal is not rejected (both compares are false).
bool RtUpsampleTapRejected(float rep_depth, float3 rep_normal) { return rep_depth <= 0.0 || dot(rep_normal, rep_normal) < 0.5; }
