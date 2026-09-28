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

// ── GI denoise helpers ──
float GTVBAO_DenoiseGI_EdgeWeight(float centerDepth, float neighborDepth)
{
    float diff = abs(centerDepth - neighborDepth);
    return exp(-diff * 10.0);
}

void GTVBAO_DenoiseGI(uint2 pixCoordBase, GTAOConstants consts,
    Texture2D<float4> srcGI, Texture2D<lpfloat> srcEdges, Texture2D<float> srcDepth,
    bool useRealDepth, SamplerState samp, RWTexture2D<float4> outGI)
{
    uint w, h;
    srcGI.GetDimensions(w, h);

    // GTVBAO_gi_depth_binding: the packed-edge buffer is a depth-discontinuity
    // map in [0,1], so using it as the edge-stop source makes that stop a weak
    // uniform blur rather than a real depth guard. Both sources are passed in
    // and the sampled scalar is selected here — the ?: operator will not unify
    // two Texture2D objects whose component types are nominally distinct.
    for (int side = 0; side < 2; side++)
    {
        int2 pixCoord = int2(pixCoordBase.x + side, pixCoordBase.y);
        if (pixCoord.x >= w || pixCoord.y >= h) continue;

    float2 uv = (float2(pixCoord) + 0.5) * consts.ViewportPixelSize;
    float4 centerGI = srcGI.Load(int3(pixCoord, 0));
    float centerDepth = useRealDepth
        ? srcDepth.SampleLevel(samp, uv, 0)
        : srcEdges.SampleLevel(samp, uv, 0);

    float4 sum = centerGI;
    float weightSum = 1.0;

    const int2 offsets[8] = {
        int2(-1,-1), int2(0,-1), int2(1,-1),
        int2(-1, 0),            int2(1, 0),
        int2(-1, 1), int2(0, 1), int2(1, 1)
    };

    [unroll]
    for (uint i = 0; i < 8; ++i)
    {
        int2 nc = clamp(pixCoord + offsets[i], int2(0,0), int2(w-1, h-1));
        float4 neighborGI = srcGI.Load(int3(nc, 0));
        float2 nuv = (float2(nc) + 0.5) * consts.ViewportPixelSize;
        float neighborDepth = useRealDepth
            ? srcDepth.SampleLevel(samp, nuv, 0)
            : srcEdges.SampleLevel(samp, nuv, 0);

        float depthW = GTVBAO_DenoiseGI_EdgeWeight(centerDepth, neighborDepth);
        float colorDiff = length(neighborGI.rgb - centerGI.rgb) / max(length(centerGI.rgb), 0.001);
        float colorW = exp(-colorDiff * 2.0);
        float w = depthW * colorW;

        sum += neighborGI * w;
        weightSum += w;
    }

    outGI[pixCoord] = sum / max(weightSum, 0.001);
    }
}

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

  // GI always uses original 3×3 bilateral
  if (g_gi_enabled > 0.5f)
  {
      GTVBAO_DenoiseGI(dt * uint2(2, 1), consts,
          g_srcRawGI, g_srcWorkingEdges, g_srcDepth,
          GTVBAO_gi_depth_binding > 0.5f,
          g_samplerPointClamp, g_outGI);
  }
}
