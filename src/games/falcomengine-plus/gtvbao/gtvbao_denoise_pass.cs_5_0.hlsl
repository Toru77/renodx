///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// falcomengine-plus GTVBAO — Pass 3: Denoise (intermediate pass)
//
// Kai-style: builds GTAOConstants in-shader.
// Extended to denoise GI alongside AO when GT_VBAO_COMPUTE_GI is defined.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define GT_VBAO_COMPUTE_GI
#include "gtvbao_common.hlsl"

// Register layout must match gtvbao_denoise_last.cs_5_0.hlsl — both run on the
// shared denoise_layout, whose host table is
//   t0 AO, t1 edges, t2 raw GI, t3 (unused), t4 depth MIP0, t5 MRT normal.
Texture2D<uint>    g_srcWorkingAOTerm : register(t0);
Texture2D<lpfloat> g_srcWorkingEdges  : register(t1);
Texture2D<float4>  g_srcRawGI          : register(t2);  // raw GI from main pass
Texture2D<uint>    g_srcHistoryAO      : register(t3);  // unused (spatial only)
Texture2D<float>   g_srcDepth          : register(t4);  // viewspace depth MIP0
Texture2D<uint4>   g_mrtNormalTexture  : register(t5);  // unused (spatial only)
SamplerState       g_samplerPointClamp : register(s0);
RWTexture2D<uint>  g_outFinalAOTerm   : register(u0);
RWTexture2D<float4> g_outGI            : register(u1);

// ── GI denoise lives in gtvbao_common.hlsl (GTVBAO_DenoiseGI) so the
// intermediate and final denoise passes cannot drift apart. ──

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

  // GI always uses original 3×3 bilateral, edge-stopped on real view-space depth
  if (g_gi_enabled > 0.5f)
  {
      GTVBAO_DenoiseGI(dt * uint2(2, 1), consts,
          g_srcRawGI, g_srcDepth,
          g_samplerPointClamp, g_outGI);
  }
}
