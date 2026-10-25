///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// falcomengine-plus GTVBAO — Pass 3: Denoise (intermediate pass)
//
// Kai-style: builds GTAOConstants in-shader.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "gtvbao_common.hlsl"

// Register layout must match gtvbao_denoise_last.cs_5_0.hlsl — both run on the
// shared denoise_layout, whose host table is
//   t0 AO, t1 edges, t2 raw GI, t3 (unused), t4 depth MIP0, t5 MRT normal.
Texture2D<uint>    g_srcWorkingAOTerm : register(t0);
Texture2D<lpfloat> g_srcWorkingEdges  : register(t1);
Texture2D<uint>    g_srcHistoryAO      : register(t3);  // unused (spatial only)
Texture2D<float>   g_srcDepth          : register(t4);  // viewspace depth MIP0
Texture2D<uint4>   g_mrtNormalTexture  : register(t5);  // unused (spatial only)
SamplerState       g_samplerPointClamp : register(s0);
RWTexture2D<uint>  g_outFinalAOTerm   : register(u0);

[numthreads(GT_VBAO_NUMTHREADS_X, GT_VBAO_NUMTHREADS_Y, 1)]
void main(uint2 dt : SV_DispatchThreadID)
{
  uint width;
  uint height;
  g_srcWorkingAOTerm.GetDimensions(width, height);

  GTAOConstants consts = BuildGTAOConstants(uint2(width, height));

  // ── 5×5 edge-aware spatial denoiser (spatial-only; Poisson removed) ──
  GTVBAO_Denoise(dt * uint2(2, 1), consts,
      g_srcWorkingAOTerm, g_srcWorkingEdges, g_samplerPointClamp,
      g_outFinalAOTerm, false);
}
