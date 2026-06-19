///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// sora-vanillaplus XeGTAO — Pass 2: Main GTAO (Medium quality — 12 spp)
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

static lpfloat3 ComputeDepthNormal(uint2 pixCoord, float2 nsp,
                                    lpfloat vz, lpfloat lz, lpfloat rz,
                                    lpfloat tz, lpfloat bz, lpfloat4 edges,
                                    GTAOConstants consts)
{
  float3 C = XeGTAO_ComputeViewspacePosition(nsp, vz, consts);
  float3 L = XeGTAO_ComputeViewspacePosition(nsp + float2(-1, 0) * consts.ViewportPixelSize, lz, consts);
  float3 R = XeGTAO_ComputeViewspacePosition(nsp + float2( 1, 0) * consts.ViewportPixelSize, rz, consts);
  float3 T = XeGTAO_ComputeViewspacePosition(nsp + float2( 0,-1) * consts.ViewportPixelSize, tz, consts);
  float3 B = XeGTAO_ComputeViewspacePosition(nsp + float2( 0, 1) * consts.ViewportPixelSize, bz, consts);
  return (lpfloat3)XeGTAO_CalculateNormal(edges, C, L, R, T, B);
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
  float2 nsp = (float2(pixCoord) + 0.5f) * consts.ViewportPixelSize;

  lpfloat4 ul = g_srcWorkingDepth.GatherRed(g_samplerPointClamp, float2(pixCoord) * consts.ViewportPixelSize);
  lpfloat4 br = g_srcWorkingDepth.GatherRed(g_samplerPointClamp, float2(pixCoord) * consts.ViewportPixelSize, int2(1, 1));
  lpfloat vz = ul.y, lz = ul.x, tz = ul.z, rz = br.z, bz = br.x;
  lpfloat4 edges = XeGTAO_CalculateEdges(vz, lz, rz, tz, bz);

  XeGTAO_MainPass(pixCoord, (lpfloat)2, (lpfloat)3, noise,
      ComputeDepthNormal(pixCoord, nsp, vz, lz, rz, tz, bz, edges, consts),
      consts,
      g_srcWorkingDepth, g_samplerPointClamp,
      g_outWorkingAOTerm, g_outWorkingEdges);
}
