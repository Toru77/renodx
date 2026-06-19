///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// sora-vanillaplus XeGTAO — Pass 3: Denoise (intermediate pass)
//
// Kai-style: builds GTAOConstants in-shader.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "xegtao_common.hlsl"

Texture2D<uint>    g_srcWorkingAOTerm : register(t0);
Texture2D<lpfloat> g_srcWorkingEdges  : register(t1);
SamplerState       g_samplerPointClamp : register(s0);
RWTexture2D<uint>  g_outFinalAOTerm   : register(u0);

[numthreads(XE_GTAO_NUMTHREADS_X, XE_GTAO_NUMTHREADS_Y, 1)]
void main(uint2 dt : SV_DispatchThreadID)
{
  uint width;
  uint height;
  g_srcWorkingAOTerm.GetDimensions(width, height);

  GTAOConstants consts = BuildGTAOConstants(uint2(width, height));

  XeGTAO_Denoise(dt * uint2(2, 1), consts,
      g_srcWorkingAOTerm, g_srcWorkingEdges, g_samplerPointClamp,
      g_outFinalAOTerm, false);
}
