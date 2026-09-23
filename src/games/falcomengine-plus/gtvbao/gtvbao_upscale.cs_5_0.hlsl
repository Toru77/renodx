///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// falcomengine-plus GTVBAO — Half-res upscale (joint bilateral reconstruction)
//
// Reconstructs full-resolution AO + GI from half-resolution denoised AO/GI
// using full-resolution geometry guides (depth MIP0 + pre-decoded normals).
//
// 5x5 half-res neighborhood per full-res pixel:
//   weight = spatial * exp(-|dot(Nf, Plow - Pf)| * planeSigma)
//                   * pow(saturate(dot(Nf, Nlow)), normalPower)
// AO_full = sum(AO*w)/sum(w), GI_full = sum(GI*w)/sum(w) (shared weights).
// Fallback: nearest half tap when sum(w) ~= 0. Geometry lookups are
// point/Load-based block centers (odd-safe); AO/GI signal uses plain loads.
//
// Bindings (upscale_layout): t0 = half AO denoised (R32U, final domain),
// t1 = half GI denoised (RGBA16F), t2 = full depth MIPs (R32F),
// t3 = full MRT normal (uint4, unused here — kept for layout parity/debug),
// t4 = full pre-decoded normals (RGBA16F) -> u0 = full AO, u1 = full GI,
// u2 = full debug.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "gtvbao_common.hlsl"

Texture2D<uint>      g_srcHalfAO      : register(t0);
Texture2D<float4>    g_srcHalfGI      : register(t1);
Texture2D<float>     g_srcFullDepth   : register(t2);
Texture2D<uint4>     g_srcFullMrt     : register(t3);
Texture2D<float4>    g_srcFullNormal  : register(t4);
SamplerState         g_samplerPointClamp : register(s0);
RWTexture2D<uint>    g_outFullAO      : register(u0);
RWTexture2D<float4>  g_outFullGI      : register(u1);
RWTexture2D<float4>  g_outFullDebug   : register(u2);

float3 UpscaleSafeNormalize(float3 v, float3 fallback) {
  float len2 = dot(v, v);
  return (len2 < 1e-5) ? fallback : v * rsqrt(len2);
}

// Depth-derived full-res normal at a full texel (fallback when pre-decoded
// normals are unavailable, e.g. no MRT capture).
float3 UpscaleDepthNormal(int2 ftc, int2 fullDims, float2 fullPx, GTAOConstants consts) {
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

  // Full-res constants (pixel size only used for position reconstruction).
  GTAOConstants fconsts = BuildGTAOConstants(uint2(fullW, fullH));
  float2 fullPx = 1.0 / max(float2(fullDims), float2(1, 1));

  int2 ft = int2(dt);
  int2 ftc = clamp(ft, int2(0, 0), fmax);

  // Full-res geometry at this pixel.
  float fz = g_srcFullDepth.Load(int3(ftc, 0));
  float2 fuv = (float2(ftc) + 0.5) * fullPx;
  float3 Pf = GTVBAO_ComputeViewspacePosition(fuv, fz, fconsts);
  float3 Nf;
  if (GTVBAO_mrt_normal_available > 0.5f) {
    Nf = g_srcFullNormal.Load(int3(ftc, 0)).xyz;
    Nf = UpscaleSafeNormalize(Nf, float3(0, 0, 1));
  } else {
    Nf = UpscaleDepthNormal(ftc, fullDims, fullPx, fconsts);
  }

  float planeSigma = max(GTVBAO_upscale_plane_sigma, 0.001f);
  float normalPower = clamp(GTVBAO_upscale_normal_power, 1.0f, 64.0f);

  // Exact half-res position of this full pixel; 5x5 taps around its center.
  float2 hpos = (float2(ft) + 0.5) * 0.5f - 0.5f;
  int2 hc0 = clamp(int2(floor(hpos + 0.5f)), int2(0, 0), hmax);

  float aoSum = 0.0f, wSum = 0.0f;
  float4 giSum = float4(0, 0, 0, 0);
  float dwAcc = 0.0f, nwAcc = 0.0f;
  float nearestAO = (float)g_srcHalfAO.Load(int3(hc0, 0)) * (1.0f / 255.0f);
  float4 nearestGI = g_srcHalfGI.Load(int3(hc0, 0));

  [unroll]
  for (int dy = -2; dy <= 2; ++dy) {
    [unroll]
    for (int dx = -2; dx <= 2; ++dx) {
      int2 htc = clamp(hc0 + int2(dx, dy), int2(0, 0), hmax);
      float aoLow = (float)g_srcHalfAO.Load(int3(htc, 0)) * (1.0f / 255.0f);
      float4 giLow = g_srcHalfGI.Load(int3(htc, 0));

      // Geometry representative: full-res block center for this half tap.
      int2 fcc = GTVBAO_HalfToFullCenter(htc, fullDims);
      float lz = g_srcFullDepth.Load(int3(fcc, 0));
      float3 Plow = GTVBAO_ComputeViewspacePosition((float2(fcc) + 0.5) * fullPx, lz, fconsts);
      float3 Nlow;
      if (GTVBAO_mrt_normal_available > 0.5f) {
        Nlow = UpscaleSafeNormalize(g_srcFullNormal.Load(int3(fcc, 0)).xyz, Nf);
      } else {
        Nlow = UpscaleDepthNormal(fcc, fullDims, fullPx, fconsts);
      }

      float2 d2 = float2(htc) - hpos;
      float spatialW = exp(-dot(d2, d2) * 0.5f);
      float planeD = abs(dot(Nf, Plow - Pf));
      float depthW = exp(-planeD * planeSigma);
      float normalW = pow(saturate(dot(Nf, Nlow)), normalPower);
      float w = spatialW * depthW * normalW;

      aoSum += aoLow * w;
      giSum += giLow * w;
      wSum += w;
      dwAcc += depthW;
      nwAcc += normalW;
    }
  }

  float aoOut, wAvg = wSum / 25.0f;
  float4 giOut;
  if (wSum > 1e-5f) {
    aoOut = aoSum / wSum;
    giOut = giSum / wSum;
  } else {
    // Fallback hierarchy: weighted -> nearest valid half tap.
    aoOut = nearestAO;
    giOut = nearestGI;
  }
  aoOut = saturate(aoOut);

  g_outFullAO[dt] = (uint)(aoOut * 255.0f + 0.5f);
  g_outFullGI[dt] = giOut;

  // ── Diagnostics (GTVBAO_upscale_debug) ──
  int dbg = (int)GTVBAO_upscale_debug;
  float4 dbgOut = float4(0, 0, 0, 0);
  if (dbg == 1 || dbg == 2) {
    dbgOut = float4(nearestAO, nearestAO, nearestAO, 1);
  } else if (dbg == 3) {
    dbgOut = float4(aoOut, aoOut, aoOut, 1);
  } else if (dbg == 4) {
    float d = saturate(dwAcc / 25.0f);
    dbgOut = float4(d, 0.5f * (1.0f - d), 1.0f - d, 1);
  } else if (dbg == 5) {
    float d = saturate(nwAcc / 25.0f);
    dbgOut = float4(d, d, d, 1);
  } else if (dbg == 6) {
    dbgOut = float4(wAvg, wAvg, wAvg, 1);
  } else if (dbg == 7 || dbg == 8) {
    dbgOut = float4(saturate(nearestGI.rgb), 1);
  } else if (dbg == 9) {
    dbgOut = float4(saturate(giOut.rgb), 1);
  }
  g_outFullDebug[dt] = dbgOut;
}
