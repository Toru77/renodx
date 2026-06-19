///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// sora-vanillaplus XeGTAO — Pass 2: Main GTAO (Low quality — 8 spp)
//
// Kai-style: builds GTAOConstants in-shader.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "xegtao_common.hlsl"

Texture2D<lpfloat>         g_srcWorkingDepth    : register(t0);
SamplerState               g_samplerPointClamp  : register(s0);
RWTexture2D<uint>          g_outWorkingAOTerm   : register(u0);
RWTexture2D<float>         g_outWorkingEdges    : register(u1);

static lpfloat2 SpatioTemporalNoise(uint2 pixCoord, uint temporalIndex)
{
  uint index = HilbertIndex(pixCoord.x % XE_HILBERT_WIDTH, pixCoord.y % XE_HILBERT_WIDTH);
  index += 288u * (temporalIndex % 64u);
  return lpfloat2(
      frac(0.5 + (float)index * 0.75487766624669276005),
      frac(0.5 + (float)index * 0.5698402909980532659114));
}

static lpfloat3 ComputeDepthNormal(uint2 pixCoord, float2 normalizedScreenPos,
                                    lpfloat viewspaceZ, lpfloat pixLZ, lpfloat pixRZ,
                                    lpfloat pixTZ, lpfloat pixBZ, lpfloat4 edgesLRTB,
                                    GTAOConstants consts)
{
  float3 CENTER = XeGTAO_ComputeViewspacePosition(normalizedScreenPos, viewspaceZ, consts);
  float3 LEFT   = XeGTAO_ComputeViewspacePosition(
      normalizedScreenPos + float2(-1, 0) * consts.ViewportPixelSize, pixLZ, consts);
  float3 RIGHT  = XeGTAO_ComputeViewspacePosition(
      normalizedScreenPos + float2( 1, 0) * consts.ViewportPixelSize, pixRZ, consts);
  float3 TOP    = XeGTAO_ComputeViewspacePosition(
      normalizedScreenPos + float2( 0,-1) * consts.ViewportPixelSize, pixTZ, consts);
  float3 BOTTOM = XeGTAO_ComputeViewspacePosition(
      normalizedScreenPos + float2( 0, 1) * consts.ViewportPixelSize, pixBZ, consts);
  return (lpfloat3)XeGTAO_CalculateNormal(edgesLRTB, CENTER, LEFT, RIGHT, TOP, BOTTOM);
}

[numthreads(XE_GTAO_NUMTHREADS_X, XE_GTAO_NUMTHREADS_Y, 1)]
void main(uint2 pixCoord : SV_DispatchThreadID)
{
  uint width;
  uint height;
  g_srcWorkingDepth.GetDimensions(width, height);

  if (pixCoord.x >= width || pixCoord.y >= height) return;

  GTAOConstants consts = BuildGTAOConstants(uint2(width, height));

  uint noise_idx = consts.NoiseIndex < 0 ? 0u : (uint)consts.NoiseIndex;
  lpfloat2 noise = SpatioTemporalNoise(pixCoord, noise_idx);
  float2 screenPos = (float2(pixCoord) + 0.5f) * consts.ViewportPixelSize;

  lpfloat4 gatherUL = g_srcWorkingDepth.GatherRed(g_samplerPointClamp, float2(pixCoord) * consts.ViewportPixelSize);
  lpfloat4 gatherBR = g_srcWorkingDepth.GatherRed(g_samplerPointClamp, float2(pixCoord) * consts.ViewportPixelSize, int2(1, 1));
  lpfloat z = gatherUL.y, l = gatherUL.x, t = gatherUL.z, r = gatherBR.z, b = gatherBR.x;
  lpfloat4 edges = XeGTAO_CalculateEdges(z, l, r, t, b);

  XeGTAO_MainPass(pixCoord, (lpfloat)2, (lpfloat)2, noise,
      ComputeDepthNormal(pixCoord, screenPos, z, l, r, t, b, edges, consts),
      consts,
      g_srcWorkingDepth, g_samplerPointClamp,
      g_outWorkingAOTerm, g_outWorkingEdges);
}
