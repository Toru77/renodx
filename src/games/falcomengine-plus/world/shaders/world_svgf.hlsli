#pragma once

// world_svgf.hlsli - SVGF (Schied et al., HPG 2017, "Spatiotemporal Variance-Guided Filtering"; the paper is
// reference/svgf/svgf_preprint.md) for the shared denoise chain (RTGI, GTVBGI, RT Local Shadows, RT Sun). Three compute
// passes include this file: world_svgf_reproject (Sec. 4.1 temporal accumulation and the guide data), world_svgf_variance
// (Sec. 4.2 spatial variance while the history is short) and world_svgf_atrous (Sec. 4.3 a-trous iteration, Eq. 1 to 3,
// with the Sec. 4.4 edge stops, Eq. 4 to 6). "Non-paper" marks a value the paper does not give. No mesh ID, no MRT0 .w
// read and no class or flag test in any SVGF pass.

#include "../../taa/taa_common.hlsli"
#include "../../include/mrt_normal.hlsli"
#include "world_rt_denoise.hlsli"

cbuffer cb_scene : register(b0)
{
    float4x4 view_g : packoffset(c0);
    float4x4 viewInv_g : packoffset(c4);
    float4x4 viewProjInv_g : packoffset(c20);
    float2 resolutionScaling_g : packoffset(c49);
};

// Push constants (SvgfPushConstants in rtgi_resources.hpp, 16 floats).
cbuffer cb_svgf : register(b12)
{
    float4 s_size : packoffset(c0);      // trace width, trace height, full width, full height
    float4 s_params : packoffset(c1);    // trace divisor d, debug mode (RTGI numbering), history valid (1, 0 = reset), a-trous step 2^i (0 otherwise)
    float4 s_temporal : packoffset(c2);  // alpha = 1 - History Weight, depth rejection, normal rejection, sigma_l
    float4 s_luminance : packoffset(c3); // luminance weights rgb, unused
};

static const float kSvgfSigmaZ = 1.0;                  // Sec. 4.4: sigma_z = 1
static const float kSvgfSigmaN = 128.0;                // Sec. 4.4: sigma_n = 128
static const float kSvgfMaxHistory = 32.0;             // non-paper: history length cap
static const float kSvgfDepthEpsilonScale = 1e-3;      // non-paper: eps_z = this x z_p (plan default; Falcor has no eps)
static const float kSvgfLuminanceEpsilon = 1e-6;       // non-paper: eps_l (plan default; Falcor uses 1e-10 under the sqrt)
static const float kSvgfVarianceHistory = 4.0;         // Sec. 4.2: history below 4 frames uses the spatial estimate
// Sec. 4.3: h(k) = H[|kx|] * H[|ky|] with H = {3/8, 1/4, 1/16} (the paper's kernel h = [1/16, 1/4, 3/8, 1/4, 1/16]).
static const float kSvgfKernel[3] = {0.375, 0.25, 0.0625};

// Luminance l = dot(rgb, Lw) with the weights of the owner (push c3).
float SvgfLuminance(float3 c)
{
    return dot(c, s_luminance.rgb);
}

// Eq. 4: depth weight of tap q (guide gq) against centre p (guide gp); offset = q - p in trace pixels. Guide: (z, dz/dx,
// dz/dy, dist). The slope is the dot product of the screen-space gradient with the offset (Sec. 4.4 local linear model).
// Non-paper: the dot product and the relative eps follow the plan; Falcor uses length(offset) and no eps.
float SvgfDepthWeight(float4 gp, float4 gq, float2 offset)
{
    const float slope = abs(dot(gp.yz, offset));
    return exp(-abs(gp.x - gq.x) / (kSvgfSigmaZ * slope + kSvgfDepthEpsilonScale * gp.x));
}

// Eq. 5: normal weight, cosine term with sigma_n.
float SvgfNormalWeight(float3 np, float3 nq)
{
    return pow(max(0.0, dot(np, nq)), kSvgfSigmaN);
}

// Trace pixel inside the trace image.
bool SvgfInside(int2 q)
{
    return all(q >= 0) && all(q < int2(s_size.xy));
}

// Writes a traced pixel's colour to its d x d block of the full-size debug view.
void SvgfWriteDebugBlock(RWTexture2D<float4> debug, uint2 tp, float3 colour)
{
    const uint d = max((uint)s_params.x, 1u);
    const uint2 full_size = uint2(s_size.zw);
    for (uint by = 0u; by < d; ++by)
    {
        for (uint bx = 0u; bx < d; ++bx)
        {
            const uint2 block_px = tp * d + uint2(bx, by);
            if (all(block_px < full_size))
            {
                debug[block_px] = float4(colour, 1.0);
            }
        }
    }
}
