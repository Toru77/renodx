// falcomengine-plus VBGI far field - spatial reuse (M3). Merges this texel's temporal reservoir (or candidate) with
// kVbgiFarSpatialNeighbors random neighbours' inside GTVBAO_far_spatial_radius trace texels (biased ReSTIR GI: geometric
// test of 25 degrees and 5% depth, VbgiFarUsable + near-segment check, 1/M). Neighbour W carries the reconnection
// Jacobian in the cosine-weighted measure (receiver cosine ratio x ReSTIR GI eq. 11 solid-angle ratio), rejected
// outside [0.1, 10]. nrm.w of the output = fraction of accepted neighbours. Not kept across frames.
#include "vbgi_far_pass.hlsli"
static const uint kVbgiFarSpatialNeighbors = 3u;
[numthreads(8, 8, 1)]
void main(uint2 p : SV_DispatchThreadID)
{
  uint w, h;
  g_farOut0.GetDimensions(w, h);
  if (p.x >= w || p.y >= h) return;
  const GTAOConstants consts = BuildGTAOConstants(uint2(w, h));
  const float radius = consts.EffectRadius * consts.RadiusMultiplier;
  const float4 o0 = g_farIn0.Load(int3(p, 0));
  const float4 o1 = g_farIn1.Load(int3(p, 0));
  const float4 o2 = g_farIn2.Load(int3(p, 0));
  VbgiFarReservoir r = (VbgiFarReservoir)0;
  VbgiFarMerge(r, o0.xyz, o1.rgb, o2.xyz, VbgiFarLuminance(o1.rgb), o0.w, o1.a, VbgiFarRand(p, 10u));
  float accepted = 0.0f;
  float3 P, N;
  float2 uv;
  if (VbgiFarSurface(int2(p), consts, P, N, uv))
  {
    [loop] for (uint k = 0u; k < kVbgiFarSpatialNeighbors; ++k)
    {
      float sn, cs;
      sincos(6.28318548f * VbgiFarRand(p, 20u + 2u * k), sn, cs);
      const float2 off = float2(cs, sn) * (GTVBAO_far_spatial_radius * sqrt(VbgiFarRand(p, 21u + 2u * k)));
      const int2 q = clamp(int2(p) + int2(round(off)), int2(0, 0), int2(w, h) - 1);
      if (all(q == int2(p))) continue;
      float3 Pq, Nq;
      float2 uvq;
      if (!VbgiFarSurface(q, consts, Pq, Nq, uvq) || dot(N, Nq) < 0.906f || abs(Pq.z - P.z) > 0.05f * P.z) continue;
      const float4 n0 = g_farIn0.Load(int3(q, 0));
      const float4 n1 = g_farIn1.Load(int3(q, 0));
      const float4 n2 = g_farIn2.Load(int3(q, 0));
      float phat = 0.0f;
      float jac = 0.0f;
      if (n0.w > 0.0f && FalcomNormalValid(n2.xyz))
      {
        const float3 S = VbgiFarWorldToView(n0.xyz);
        const float3 NS = TransformNormalToView(n2.xyz);
        float cosR, cosS, d2, cosRq, cosSq, d2q;
        if (VbgiFarUsable(P, N, S, NS, radius, cosR, cosS, d2) && VbgiFarUsable(Pq, Nq, S, NS, 0.0f, cosRq, cosSq, d2q))
        {
          jac = (cosR / cosRq) * (cosS / cosSq) * (d2q / d2);
          if (jac >= 0.1f && jac <= 10.0f
              && VbgiFarNearClear(P, S, radius, consts.BitmaskThickness, uv, consts, g_farDepth, g_farSamp, VbgiFarRand(p, 30u + k)))
          {
            phat = VbgiFarLuminance(n1.rgb);
            accepted += 1.0f;
          }
        }
      }
      VbgiFarMerge(r, n0.xyz, n1.rgb, n2.xyz, phat, n0.w * jac, n1.a, VbgiFarRand(p, 11u + k));
    }
  }
  g_farOut0[p] = float4(r.pos, VbgiFarFinalW(r));
  g_farOut1[p] = float4(r.rad, r.M);
  g_farOut2[p] = float4(r.nrm, accepted / (float)kVbgiFarSpatialNeighbors);
}
