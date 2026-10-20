#pragma once

// world_rt_denoise.hlsli - denoiser math shared by the RTAO passes (world_rtao_temporal, world_rtao_filter) and the RTGI
// passes (world_rtgi_temporal, world_rtgi_filter, world_rtgi_upsample): one implementation of the temporal accept
// rules, the history clamp and the spatial edge stops. Pure functions; no resources or constant buffers.

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

// History clamp: value clamped to the neighbourhood mean +/- k * sigma from the sum s1, the sum of squares s2 and the count n.
float ClampToNeighbourhood(float value, float s1, float s2, float n, float k)
{
    const float mean = s1 / n;
    const float sigma = sqrt(max(s2 / n - mean * mean, 0.0));
    return clamp(value, mean - k * sigma, mean + k * sigma);
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
