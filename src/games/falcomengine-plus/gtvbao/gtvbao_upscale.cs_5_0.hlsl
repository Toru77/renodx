///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// falcomengine-plus GTVBAO — Half-res upscale (4-TAP JOINT)
//
// Per full-res pixel, the four surrounding half-res texels are combined with
// ordinary bilinear spatial weights multiplied by geometry-aware terms
// (plane/depth compatibility, normal compatibility), normalized, with shared
// weights for AO.
//
//   weight = bilinearWeight * exp(-|dot(Nf, Plow-Pf)| * sigma)
//                             * pow(saturate(dot(Nf, Nlow)), power)
//
// Geometry mapping (half->full block centers, odd-safe point loads).
// Requires pre-decoded full-res normals (normal_prep), so the CPU always runs
// the normal-prep pass before this dispatch when the à-trous chain is off.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "gtvbao_common.hlsl"

Texture2D<uint>      g_srcHalfAO      : register(t0);
Texture2D<float>     g_srcFullDepth   : register(t2);
Texture2D<uint4>     g_srcFullMrt     : register(t3);
Texture2D<float4>    g_srcFullNormal  : register(t4);
SamplerState         g_samplerPointClamp : register(s0);
RWTexture2D<uint>    g_outFullAO      : register(u0);

// Depth-derived full-res normal at a full texel (fallback when pre-decoded
// normals are unavailable, e.g. no MRT capture).
float3 Tap4DepthNormal(int2 ftc, int2 fullDims, float2 fullPx, GTAOConstants consts) {
  int2 fmax = max(fullDims - 1, int2(0, 0));
  float z = g_srcFullDepth.Load(int3(ftc, 0));
  float l = g_srcFullDepth.Load(int3(clamp(ftc + int2(-1, 0), int2(0, 0), fmax), 0));
  float r = g_srcFullDepth.Load(int3(clamp(ftc + int2( 1, 0), int2(0, 0), fmax), 0));
  float t = g_srcFullDepth.Load(int3(clamp(ftc + int2( 0,-1), int2(0, 0), fmax), 0));
  float b = g_srcFullDepth.Load(int3(clamp(ftc + int2( 0, 1), int2(0, 0), fmax), 0));
  float2 uC = (float2(ftc) + 0.5) * fullPx;
  float3 C = GTVBAO_ComputeViewspacePosition(uC, z, consts);
  float3 L = GTVBAO_ComputeViewspacePosition(uC + float2(-1, 0) * fullPx, l, consts);
  float3 R = GTVBAO_ComputeViewspacePosition(uC + float2( 1, 0) * fullPx, r, consts);
  float3 T = GTVBAO_ComputeViewspacePosition(uC + float2( 0,-1) * fullPx, t, consts);
  float3 B = GTVBAO_ComputeViewspacePosition(uC + float2( 0, 1) * fullPx, b, consts);
  return (float3)GTVBAO_CalculateNormal(
      GTVBAO_CalculateEdges((lpfloat)z, (lpfloat)l, (lpfloat)r, (lpfloat)t, (lpfloat)b),
      C, L, R, T, B);
}

[numthreads(GT_VBAO_NUMTHREADS_X, GT_VBAO_NUMTHREADS_Y, 1)]
void main(uint2 dt : SV_DispatchThreadID) {
  uint fullW, fullH;
  g_outFullAO.GetDimensions(fullW, fullH);
  if (dt.x >= fullW || dt.y >= fullH) return;

  uint halfW, halfH;
  g_srcHalfAO.GetDimensions(halfW, halfH);
  halfW = max(halfW, 1u); halfH = max(halfH, 1u);
  int2 halfDims = int2(halfW, halfH);
  int2 fullDims = int2(max(fullW, 1u), max(fullH, 1u));
  int2 fmax = max(fullDims - 1, int2(0, 0));
  int2 hmax = max(halfDims - 1, int2(0, 0));

  GTAOConstants fconsts = BuildGTAOConstants(uint2(fullW, fullH));
  float2 fullPx = 1.0 / max(float2(fullDims), float2(1, 1));

  int2 ft = int2(dt);
  int2 ftc = clamp(ft, int2(0, 0), fmax);

  // Full-res geometry at this pixel (unit normals by construction).
  float fz = g_srcFullDepth.Load(int3(ftc, 0));
  float2 fuv = (float2(ftc) + 0.5) * fullPx;
  float3 Pf = GTVBAO_ComputeViewspacePosition(fuv, fz, fconsts);
  float3 Nf;
  if (GTVBAO_mrt_normal_available > 0.5f) {
    Nf = g_srcFullNormal.Load(int3(ftc, 0)).xyz;
  } else {
    Nf = Tap4DepthNormal(ftc, fullDims, fullPx, fconsts);
  }

  float planeSigma = max(GTVBAO_upscale_plane_sigma, 0.001f);
  float normalPower = clamp(GTVBAO_upscale_normal_power, 1.0f, 64.0f);

  // Continuous half-res position; four bilinear neighbors with ordinary
  // bilinear weights.
  float2 hpos = (float2(ft) + 0.5) * 0.5f - 0.5f;
  int2 hbase = clamp(int2(floor(hpos)), int2(0, 0), hmax);
  float2 hfrac = saturate(hpos - float2(hbase));
  int2 hc0 = clamp(int2(floor(hpos + 0.5f)), int2(0, 0), hmax);

  float aoSum = 0.0f, wSum = 0.0f;
  float nearestAO = (float)g_srcHalfAO.Load(int3(hc0, 0)) * (1.0f / 255.0f);

  [unroll]
  for (int by = 0; by <= 1; ++by) {
    [unroll]
    for (int bx = 0; bx <= 1; ++bx) {
      int2 htc = clamp(hbase + int2(bx, by), int2(0, 0), hmax);
      float bw = ((bx == 0) ? (1.0f - hfrac.x) : hfrac.x)
               * ((by == 0) ? (1.0f - hfrac.y) : hfrac.y);
      float aoLow = (float)g_srcHalfAO.Load(int3(htc, 0)) * (1.0f / 255.0f);

      // Geometry representative: full-res block center for this half tap.
      int2 fcc = GTVBAO_HalfToFullCenter(htc, fullDims);
      float lz = g_srcFullDepth.Load(int3(fcc, 0));
      float3 Plow = GTVBAO_ComputeViewspacePosition((float2(fcc) + 0.5) * fullPx, lz, fconsts);
      float3 Nlow;
      if (GTVBAO_mrt_normal_available > 0.5f) {
        Nlow = g_srcFullNormal.Load(int3(fcc, 0)).xyz;
      } else {
        Nlow = Tap4DepthNormal(fcc, fullDims, fullPx, fconsts);
      }

      // planeD is already 0 when Nf is the zero placeholder, so depthW comes
      // out neutral on its own. The normal term must be forced neutral too,
      // otherwise pow(0, power) rejects every tap and the whole reconstruction
      // collapses onto the nearest-tap fallback.
      float planeD = abs(dot(Nf, Plow - Pf));
      float depthW = exp(-planeD * planeSigma);
      float normalW = (FalcomNormalValid(Nf) && FalcomNormalValid(Nlow))
                    ? pow(saturate(dot(Nf, Nlow)), normalPower) : 1.0;
      // No floor on the geometry term: when every tap is rejected the weighted
      // sum collapses to the nearest-tap fallback, which is correct here —
      // bilinear across taps that straddle a depth discontinuity would bleed
      // AO over the silhouette instead.
      float w = bw * depthW * normalW;

      aoSum += aoLow * w;
      wSum += w;
    }
  }

  float aoOut;
  if (wSum > 1e-5f) {
    aoOut = aoSum / wSum;
  } else {
    // Fallback hierarchy: weighted -> nearest half tap -> (hc0 == center tap).
    aoOut = nearestAO;
  }
  aoOut = saturate(aoOut);

  g_outFullAO[dt] = (uint)(aoOut * 255.0f + 0.5f);
}
