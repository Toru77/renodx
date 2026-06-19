///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// sora-vanillaplus XeGTAO — Pass 2: Main GTAO (High quality)
//
// Kai-style: builds GTAOConstants in-shader, gets quality from push_constants.
// Quality is determined by xegtao_quality push constant, not hardcoded slices.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "xegtao_common.hlsl"

Texture2D<lpfloat>     g_srcWorkingDepth   : register(t0);
SamplerState           g_samplerPointClamp : register(s0);

RWTexture2D<uint>          g_outWorkingAOTerm : register(u0);
RWTexture2D<float>         g_outWorkingEdges  : register(u1);

static lpfloat2 SpatioTemporalNoise(uint2 p, uint t)
{
  uint i = HilbertIndex(p.x % XE_HILBERT_WIDTH, p.y % XE_HILBERT_WIDTH) + 288u * (t % 64u);
  return lpfloat2(frac(0.5 + (float)i * 0.75487766624669276005),
                  frac(0.5 + (float)i * 0.5698402909980532659114));
}

static lpfloat3 DepthNormal(uint2 p, float2 u, lpfloat z, lpfloat l, lpfloat r, lpfloat t, lpfloat b, lpfloat4 e, GTAOConstants consts)
{
  float3 C = XeGTAO_ComputeViewspacePosition(u, z, consts);
  float3 L = XeGTAO_ComputeViewspacePosition(u + float2(-1, 0) * consts.ViewportPixelSize, l, consts);
  float3 R = XeGTAO_ComputeViewspacePosition(u + float2(1, 0) * consts.ViewportPixelSize, r, consts);
  float3 T = XeGTAO_ComputeViewspacePosition(u + float2(0, -1) * consts.ViewportPixelSize, t, consts);
  float3 B = XeGTAO_ComputeViewspacePosition(u + float2(0, 1) * consts.ViewportPixelSize, b, consts);
  return (lpfloat3)XeGTAO_CalculateNormal(e, C, L, R, T, B);
}

[numthreads(XE_GTAO_NUMTHREADS_X, XE_GTAO_NUMTHREADS_Y, 1)]
void main(uint2 p : SV_DispatchThreadID)
{
  uint width;
  uint height;
  g_srcWorkingDepth.GetDimensions(width, height);

  if (p.x >= width || p.y >= height) return;

  GTAOConstants consts = BuildGTAOConstants(uint2(width, height));

  uint noise_idx = consts.NoiseIndex < 0 ? 0u : (uint)consts.NoiseIndex;
  lpfloat2 n = SpatioTemporalNoise(p, noise_idx);
  float2 u = (float2(p) + 0.5f) * consts.ViewportPixelSize;

  lpfloat4 ul = g_srcWorkingDepth.GatherRed(g_samplerPointClamp, float2(p) * consts.ViewportPixelSize);
  lpfloat4 br = g_srcWorkingDepth.GatherRed(g_samplerPointClamp, float2(p) * consts.ViewportPixelSize, int2(1, 1));
  lpfloat z = ul.y, l = ul.x, t = ul.z, r = br.z, b = br.x;
  lpfloat4 e = XeGTAO_CalculateEdges(z, l, r, t, b);

  // Quality from push constant: map to slice/step counts
  lpfloat slice_count = 3.0;
  lpfloat steps_per_slice = 3.0;
  uint q = (uint)round(clamp(xegtao_quality, 0.0, 3.0));
  if (q == 0u)      { slice_count = 2.0; steps_per_slice = 2.0; }  // Low
  else if (q == 1u) { slice_count = 2.0; steps_per_slice = 3.0; }  // Medium
  else if (q == 2u) { slice_count = 3.0; steps_per_slice = 3.0; }  // High
  else              { slice_count = 4.0; steps_per_slice = 4.0; }  // Ultra

  XeGTAO_MainPass(p, slice_count, steps_per_slice, n,
      DepthNormal(p, u, z, l, r, t, b, e, consts),
      consts,
      g_srcWorkingDepth, g_samplerPointClamp,
      g_outWorkingAOTerm, g_outWorkingEdges);
}
