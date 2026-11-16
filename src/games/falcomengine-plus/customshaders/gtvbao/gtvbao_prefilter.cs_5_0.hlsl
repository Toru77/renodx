///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// falcomengine-plus GTVBAO — Pass 1: Depth Prefilter (MIP chain generation)
//
// Kai-style: builds GTAOConstants in-shader from game's scene CBV (b0)
// and push_constants (b13). Dispatched with ceil(w/16) × ceil(h/16) × 1.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "gtvbao_common.hlsl"

Texture2D<float>       g_srcRawDepth       : register(t0);
SamplerState           g_samplerPointClamp : register(s0);

RWTexture2D<lpfloat>   g_outDepthMIP0      : register(u0);
RWTexture2D<lpfloat>   g_outDepthMIP1      : register(u1);
RWTexture2D<lpfloat>   g_outDepthMIP2      : register(u2);
RWTexture2D<lpfloat>   g_outDepthMIP3      : register(u3);
RWTexture2D<lpfloat>   g_outDepthMIP4      : register(u4);

[numthreads(GT_VBAO_NUMTHREADS_X, GT_VBAO_NUMTHREADS_Y, 1)]
void main(
    uint2 dispatchThreadID : SV_DispatchThreadID,
    uint2 groupThreadID : SV_GroupThreadID)
{
  uint width;
  uint height;
  g_outDepthMIP0.GetDimensions(width, height);

  GTAOConstants consts = BuildGTAOConstants(uint2(width, height));

  GTVBAO_PrefilterDepths16x16(
      dispatchThreadID, groupThreadID, consts,
      g_srcRawDepth, g_samplerPointClamp,
      g_outDepthMIP0, g_outDepthMIP1, g_outDepthMIP2,
      g_outDepthMIP3, g_outDepthMIP4);
}
