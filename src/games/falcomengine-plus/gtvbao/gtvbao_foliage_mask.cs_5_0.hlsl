///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// GTVBAO foliage mask pre-pass — reads MRT normal, checks bit 15, writes a binary mask to u0.
// The mask is consumed by the main pass (t4) per depth sample to prevent foliage from casting AO.
// Sora only; Kai uses o1.w for normal data (bit 15 reserved by other fields).
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define GT_VBAO_USE_DEFAULT_CONSTANTS 0
#define GT_VBAO_USE_HALF_FLOAT_PRECISION 0
#define GT_VBAO_USE_BITMASK 1

#include "gtvbao_common.hlsl"

Texture2D<lpfloat>  g_srcWorkingDepth : register(t0);
Texture2D<uint4>    g_srcMrtNormal    : register(t1);
RWTexture2D<uint>    g_outFoliageMask  : register(u0);

SamplerState g_samplerPointClamp : register(s0);

[numthreads(GT_VBAO_NUMTHREADS_X, GT_VBAO_NUMTHREADS_Y, 1)]
void main(uint2 p : SV_DispatchThreadID)
{
    uint maskW, maskH;
g_outFoliageMask.GetDimensions(maskW, maskH);
if (p.x >= maskW || p.y >= maskH) return;
    if (GTVBAO_exclude_foliage > 0.5f) {
        uint mrtW, mrtH, fullW, fullH;
        g_srcMrtNormal.GetDimensions(mrtW, mrtH);
        g_srcWorkingDepth.GetDimensions(fullW, fullH);
        // Same predicate the main pass uses, so the mask marks a half texel
        // exactly when the pixel detection would mark it.
        if (GTVBAO_FoliageMarked(g_srcMrtNormal, p, uint2(maskW, maskH), uint2(fullW, fullH),
                                 uint2(mrtW, mrtH), GTVBAO_foliage_channel_mode >= 0.5f,
                                 GTVBAO_resolution > 0.5f)) {
            g_outFoliageMask[p] = 1u;
            return;
        }
    }
    g_outFoliageMask[p] = 0u;
}
