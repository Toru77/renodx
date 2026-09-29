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
        uint mrtW, mrtH;
        g_srcMrtNormal.GetDimensions(mrtW, mrtH);
        // GTVBAO_MrtTexel: same mapping the main pass and GI path use, so the
        // mask marks the pixel whose normal is actually read. Texel-space
        // coordinates, matching the original arithmetic exactly.
        int2 mrtTC = GTVBAO_MrtTexel((float2(p) + 0.5) * (float2(mrtW, mrtH) / float2(max(maskW, 1u), max(maskH, 1u))),
                                     float2(mrtW, mrtH));
        uint4 _mrtV = g_srcMrtNormal.Load(int3(mrtTC, 0));
        uint _mrtC = (GTVBAO_foliage_channel_mode < 0.5f) ? _mrtV.w : _mrtV.z;
        if (_mrtC & 0x8000u) {
            g_outFoliageMask[p] = 1u;
            return;
        }
    }
    g_outFoliageMask[p] = 0u;
}
