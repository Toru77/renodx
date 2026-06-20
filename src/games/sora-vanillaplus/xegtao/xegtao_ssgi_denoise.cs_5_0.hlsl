///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// sora-vanillaplus SSGI — Denoise pass
//
// Simple 3x3 edge-aware spatial blur for the SSGI GI output.
// Uses depth and normal edges to preserve geometry boundaries.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "xegtao_common.hlsl"

Texture2D<float4>      g_srcGI             : register(t0);
Texture2D<lpfloat>     g_srcWorkingDepth   : register(t1);
SamplerState           g_samplerPointClamp : register(s0);
RWTexture2D<float4>    g_outDenoised       : register(u0);

// Denoise push constants: c[24] = blur_strength, c[25] = depth_sensitivity
// defaults: blur_strength=1.0, depth_sensitivity=1.0

float ComputeEdgeWeight(float centerDepth, float neighborDepth, float sensitivity)
{
  float diff = abs(centerDepth - neighborDepth);
  return exp(-diff * sensitivity * 10.0);
}

[numthreads(XE_GTAO_NUMTHREADS_X, XE_GTAO_NUMTHREADS_Y, 1)]
void main(uint2 pixCoord : SV_DispatchThreadID)
{
  uint width, height;
  g_srcGI.GetDimensions(width, height);
  if (pixCoord.x >= width || pixCoord.y >= height) return;

  float4 centerGI = g_srcGI.Load(int3(pixCoord, 0));

  GTAOConstants consts = BuildGTAOConstants(uint2(width, height));

  // Read push constants
  float blurStrength  = xegtao_copyback_preserve_yzw;   // c[24]
  float depthSensitivity = xegtao_isfast_passes;        // c[25]
  blurStrength = max(0.0, blurStrength);

  float2 uv = (float2(pixCoord) + 0.5) * consts.ViewportPixelSize;
  float centerDepth = g_srcWorkingDepth.SampleLevel(g_samplerPointClamp, uv, 0);

  float4 sum = centerGI * (1.0 + blurStrength);
  float weightSum = 1.0 + blurStrength;

  // 8 neighbors
  const int2 offsets[8] = {
    int2(-1,-1), int2(0,-1), int2(1,-1),
    int2(-1, 0),            int2(1, 0),
    int2(-1, 1), int2(0, 1), int2(1, 1)
  };
  const float diagWeight = 0.7071; // 1/sqrt(2) for diagonal neighbors

  for (uint i = 0; i < 8; ++i)
  {
    int2 nc = int2(pixCoord) + offsets[i];
    nc = clamp(nc, int2(0, 0), int2(width - 1, height - 1));
    float4 neighborGI = g_srcGI.Load(int3(nc, 0));

    float2 nuv = (float2(nc) + 0.5) * consts.ViewportPixelSize;
    float neighborDepth = g_srcWorkingDepth.SampleLevel(g_samplerPointClamp, nuv, 0);

    float edgeW = ComputeEdgeWeight(centerDepth, neighborDepth, depthSensitivity);
    float distW = (abs(offsets[i].x) + abs(offsets[i].y) == 2) ? diagWeight : 1.0;
    float w = edgeW * distW * blurStrength;

    sum += neighborGI * w;
    weightSum += w;
  }

  g_outDenoised[pixCoord] = sum / max(weightSum, 0.001);
}
