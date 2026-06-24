///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// sora-vanillaplus XeGTAO — Pass 3: Denoise (intermediate pass)
//
// Kai-style: builds GTAOConstants in-shader.
// Extended to denoise GI alongside AO when XE_GTAO_COMPUTE_GI is defined.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define XE_GTAO_COMPUTE_GI
#include "xegtao_common.hlsl"

Texture2D<uint>    g_srcWorkingAOTerm : register(t0);
Texture2D<lpfloat> g_srcWorkingEdges  : register(t1);
Texture2D<float4>  g_srcRawGI          : register(t2);  // raw GI from main pass
SamplerState       g_samplerPointClamp : register(s0);
RWTexture2D<uint>  g_outFinalAOTerm   : register(u0);
RWTexture2D<float4> g_outGI            : register(u1);

// ── GI denoise helpers ──
float XeGTAO_DenoiseGI_EdgeWeight(float centerDepth, float neighborDepth)
{
    float diff = abs(centerDepth - neighborDepth);
    return exp(-diff * 10.0);
}

void XeGTAO_DenoiseGI(uint2 pixCoordBase, GTAOConstants consts,
    Texture2D<float4> srcGI, Texture2D<lpfloat> srcDepth,
    SamplerState samp, RWTexture2D<float4> outGI)
{
    uint w, h;
    srcGI.GetDimensions(w, h);

    for (int side = 0; side < 2; side++)
    {
        int2 pixCoord = int2(pixCoordBase.x + side, pixCoordBase.y);
        if (pixCoord.x >= w || pixCoord.y >= h) continue;

    float2 uv = (float2(pixCoord) + 0.5) * consts.ViewportPixelSize;
    float4 centerGI = srcGI.Load(int3(pixCoord, 0));
    float centerDepth = srcDepth.SampleLevel(samp, uv, 0);

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
        float neighborDepth = srcDepth.SampleLevel(samp, nuv, 0);

        float depthW = XeGTAO_DenoiseGI_EdgeWeight(centerDepth, neighborDepth);
        float colorDiff = length(neighborGI.rgb - centerGI.rgb) / max(length(centerGI.rgb), 0.001);
        float colorW = exp(-colorDiff * 2.0);
        float w = depthW * colorW;

        sum += neighborGI * w;
        weightSum += w;
    }

    outGI[pixCoord] = sum / max(weightSum, 0.001);
    }
}

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

  // Denoise GI using depth + color edge weights.
  if (g_gi_enabled > 0.5f)
  {
      XeGTAO_DenoiseGI(dt * uint2(2, 1), consts,
          g_srcRawGI, g_srcWorkingEdges, g_samplerPointClamp,
          g_outGI);
  }
}
