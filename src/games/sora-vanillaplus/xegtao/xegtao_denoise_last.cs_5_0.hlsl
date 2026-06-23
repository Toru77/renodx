///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// sora-vanillaplus XeGTAO — Pass 3: Denoise (final pass)
//
// Kai-style: builds GTAOConstants in-shader.
// Extended to denoise GI alongside AO when XE_GTAO_COMPUTE_GI is defined.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define XE_GTAO_COMPUTE_GI
#include "xegtao_common.hlsl"

Texture2D<uint>    g_srcWorkingAOTerm : register(t0);
Texture2D<lpfloat> g_srcWorkingEdges  : register(t1);
Texture2D<float4>  g_srcRawGI          : register(t2);  // raw GI from main pass
Texture2D<uint>    g_srcHistoryAO      : register(t3);  // previous frame denoised AO
Texture2D<float>   g_srcDepth          : register(t4);  // viewspace depth MIP0 for reprojection
SamplerState       g_samplerPointClamp : register(s0);
RWTexture2D<uint>  g_outFinalAOTerm   : register(u0);
RWTexture2D<float4> g_outGI            : register(u1);
RWTexture2D<uint>  g_outHistoryAO     : register(u2);  // write history for next frame

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
      g_outFinalAOTerm, true);

  // ── Spatio-Temporal blend ──
  if (xegtao_denoiser_type > 0.5f) {
    float2 vpSize = float2(width, height);
    float2 invVPSize = 1.0.xx / vpSize;

    [unroll]
    for (int side = 0; side < 2; side++) {
      int2 pixCoord = int2(dt.x * 2 + side, dt.y);
      if (pixCoord.x >= (int)width || pixCoord.y >= (int)height) continue;

      // Read spatial result
      uint spatialPacked = g_outFinalAOTerm[pixCoord];
      float spatialAO = float(spatialPacked) / 255.0;

      // Current UV
      float2 uv = (float2(pixCoord) + 0.5) * invVPSize;

      // Camera reprojection — reconstruct viewspace position from depth,
      // transform to previous frame's clip space via world space.
      float viewZ = -g_srcDepth.SampleLevel(g_samplerPointClamp, uv, 0);
      float3 viewPos = XeGTAO_ComputeViewspacePosition(uv, viewZ, consts);
      float4 worldPos = mul(float4(viewPos, 1.0), viewInv_g);
      float4 prevClipPos = mul(worldPos, prevViewProj_g);
      float2 prevNDC = prevClipPos.xy / prevClipPos.w;
      float2 prevUV = prevNDC * float2(-0.5, 0.5) + 0.5;
      float historyWeight = xegtao_temporal_blend;

      // Reject history when reprojected UV falls off-screen
      if (any(prevUV < 0.0 || prevUV > 1.0)) historyWeight = 0.0;

      // Sample history AO
      uint hw, hh;
      g_srcHistoryAO.GetDimensions(hw, hh);
      int2 historyCoord = int2(saturate(prevUV) * float2(hw, hh));
      uint historyPacked = g_srcHistoryAO.Load(int3(historyCoord, 0));
      float historyAO = float(historyPacked) / 255.0;

      // Guard against uninitialized history on first frame
      if (historyAO < 0.001) historyWeight = 0.0;

      // Blend
      float blendedAO = lerp(spatialAO, historyAO, historyWeight);

      // Write output and history
      uint blendedPacked = uint(saturate(blendedAO) * 255.0 + 0.5);
      g_outFinalAOTerm[pixCoord] = blendedPacked;
      g_outHistoryAO[pixCoord] = blendedPacked;
    }
  }

  // Denoise GI using depth + color edge weights.
  if (g_gi_enabled > 0.5f)
  {
      XeGTAO_DenoiseGI(dt * uint2(2, 1), consts,
          g_srcRawGI, g_srcWorkingEdges, g_samplerPointClamp,
          g_outGI);
  }
}
