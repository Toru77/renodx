///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// falcomengine-plus GTVBAO — Half-res upscale NEAREST (temporary A/B diagnostic)
//
// Minimal counterpart to gtvbao_upscale.cs: point-samples the half-resolution
// denoised AO/GI straight to full resolution. No 5x5 loop, no spatial/plane/
// normal weights, no exp/pow in the measurement path.
//
// Same bindings as the 5x5 upscale (upscale_layout: 5 SRV + 3 UAV) so it plugs
// into the same descriptor tables; only the pipeline object differs.
// t0 = half AO denoised (R32U, final domain), t1 = half GI denoised (RGBA16F),
// t2/t3/t4 = bound but UNREAD (kept for layout parity) ->
// u0 = full AO, u1 = full GI, u2 = full debug.
//
// halfTexel = floor(fullTexel / 2), clamped (odd-safe).
// Debug views mirror gtvbao_upscale.cs numbering/display mapping for A/B
// comparability, but only execute when diagnostics are requested (mode 0
// writes nothing extra: 2 loads + 2 stores).
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "gtvbao_common.hlsl"

Texture2D<uint>      g_srcHalfAO      : register(t0);
Texture2D<float4>    g_srcHalfGI      : register(t1);
Texture2D<float>     g_srcFullDepth   : register(t2);
Texture2D<uint4>     g_srcFullMrt     : register(t3);
Texture2D<float4>    g_srcFullNormal  : register(t4);
RWTexture2D<uint>    g_outFullAO      : register(u0);
RWTexture2D<float4>  g_outFullGI      : register(u1);
RWTexture2D<float4>  g_outFullDebug   : register(u2);

[numthreads(GT_VBAO_NUMTHREADS_X, GT_VBAO_NUMTHREADS_Y, 1)]
void main(uint2 dt : SV_DispatchThreadID) {
  uint fullW, fullH;
  g_outFullAO.GetDimensions(fullW, fullH);
  if (dt.x >= fullW || dt.y >= fullH) return;

  uint halfW, halfH;
  g_srcHalfAO.GetDimensions(halfW, halfH);
  halfW = max(halfW, 1u); halfH = max(halfH, 1u);

  int2 htc = min(int2(dt) / 2, int2(halfW - 1, halfH - 1));
  float ao = (float)g_srcHalfAO.Load(int3(htc, 0)) * (1.0f / 255.0f);
  float4 gi = g_srcHalfGI.Load(int3(htc, 0));

  g_outFullAO[dt] = (uint)(saturate(ao) * 255.0f + 0.5f);
  g_outFullGI[dt] = gi;

  // Diagnostics only (skipped entirely in measurement mode).
  if (GTVBAO_upscale_debug > 0.5f) {
    int dbg = (int)GTVBAO_upscale_debug;
    float4 dbgOut = float4(0, 0, 0, 0);
    if (dbg >= 1 && dbg <= 3) {
      float av = pow(saturate(ao), 3.0f);
      dbgOut = float4(av, av, av, 1);
    } else if (dbg >= 4 && dbg <= 6) {
      // No weights exist in Nearest; flat mid-gray marks these views N/A.
      dbgOut = float4(0.5f, 0.5f, 0.5f, 1);
    } else if (dbg >= 7) {
      dbgOut = float4(saturate(gi.rgb * 4.0f), 1);
    }
    g_outFullDebug[dt] = dbgOut;
  }
}
