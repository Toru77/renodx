// falcomengine-plus VBGI far field - temporal reservoir reuse (M2). Biased ReSTIR GI: this frame's candidate merged with
// last frame's temporal reservoir at the reprojected texel, 1/M normalization, M capped at GTVBAO_far_history_length x GTVBAO_far_candidates.
// The reused sender's radiance is re-read from this frame's radiance texture when it is on screen and unoccluded; the
// pair must pass VbgiFarUsable and the near-segment check. nrm.w of the output = 1 where the radiance was refreshed.
#include "vbgi_far_pass.hlsli"
static const float kVbgiFarTemporalEdge = 0.1f;
[numthreads(8, 8, 1)]
void main(uint2 p : SV_DispatchThreadID)
{
  uint w, h;
  g_farOut0.GetDimensions(w, h);
  if (p.x >= w || p.y >= h) return;
  const GTAOConstants consts = BuildGTAOConstants(uint2(w, h));
  const float radius = consts.EffectRadius * consts.RadiusMultiplier;
  const float4 c0 = g_farIn0.Load(int3(p, 0));
  const float4 c1 = g_farIn1.Load(int3(p, 0));
  const float4 c2 = g_farIn2.Load(int3(p, 0));
  VbgiFarReservoir r = (VbgiFarReservoir)0;
  VbgiFarMerge(r, c0.xyz, c1.rgb, c2.xyz, VbgiFarLuminance(c1.rgb), c0.w, c1.a, VbgiFarRand(p, 0u));
  float refreshed = 0.0f;
  float3 P, N;
  float2 uv;
  if (GTVBAO_temporal_history_valid > 0.5f && VbgiFarSurface(int2(p), consts, P, N, uv))
  {
    uint dw, dh;
    g_farNdcDepth.GetDimensions(dw, dh);
    float motion_px, depth_spread;
    const float2 hist_uv = CustomTAA_Reproject(uv, g_farNdcDepth, g_farSamp, g_farMotion, g_farSamp,
                                               1.0f / float2(dw, dh), float2(1.0f, 1.0f), motion_px, depth_spread);
    if (all(hist_uv >= 0.0f) && all(hist_uv <= 1.0f) && depth_spread <= kVbgiFarTemporalEdge)
    {
      const int3 q = int3(min(uint2(hist_uv * float2(w, h)), uint2(w, h) - 1u), 0);
      const float4 h0 = g_farPrev0.Load(q);
      float4 h1 = g_farPrev1.Load(q);
      const float4 h2 = g_farPrev2.Load(q);
      const float3 S = VbgiFarWorldToView(h0.xyz);
      if (h0.w > 0.0f && FalcomNormalValid(h2.xyz) && S.z > 0.0f)
      {
        const float2 su = VbgiFarViewToUv(S, consts);
        if (all(su >= 0.0f) && all(su <= 1.0f) && abs(g_farDepth.SampleLevel(g_farSamp, su, 0.0f).x - S.z) <= 0.05f * S.z)
        {
          h1.rgb = g_srcRadiance.SampleLevel(g_farSamp, su, 0.0f).rgb;
          refreshed = 1.0f;
        }
        float cosR, cosS, d2;
        if (VbgiFarUsable(P, N, S, TransformNormalToView(h2.xyz), radius, cosR, cosS, d2)
            && VbgiFarNearClear(P, S, radius, consts.BitmaskThickness, uv, consts, g_farDepth, g_farSamp, VbgiFarRand(p, 2u)))
          VbgiFarMerge(r, h0.xyz, h1.rgb, h2.xyz, VbgiFarLuminance(h1.rgb), h0.w,
                       min(h1.a, max(GTVBAO_far_history_length * GTVBAO_far_candidates, 1.0f)), VbgiFarRand(p, 1u));
      }
    }
  }
  g_farOut0[p] = float4(r.pos, VbgiFarFinalW(r));
  g_farOut1[p] = float4(r.rad, r.M);
  g_farOut2[p] = float4(r.nrm, refreshed);
}
