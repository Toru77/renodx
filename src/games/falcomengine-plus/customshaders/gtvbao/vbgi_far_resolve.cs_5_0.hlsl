// falcomengine-plus VBGI far field - resolve: joint raw = near raw + L x W (to the VBGI denoise), far raw = L x W.
// Alpha copies the near raw GI alpha (< 0 = not traced). Debug views: 36 far term; 38 reservoir (M/history, W, nrm.w).
#include "vbgi_far_pass.hlsli"
[numthreads(8, 8, 1)]
void main(uint2 p : SV_DispatchThreadID)
{
  uint w, h;
  g_farOut0.GetDimensions(w, h);
  if (p.x >= w || p.y >= h) return;
  const float4 nr = g_farNearRaw.Load(int3(p, 0));
  const float4 s0 = g_farIn0.Load(int3(p, 0));
  const float4 s1 = g_farIn1.Load(int3(p, 0));
  const float3 far_gi = (nr.a >= 0.0f && s0.w > 0.0f) ? min(s1.rgb * (s0.w * GTVBAO_far_intensity), GTVBAO_radiance_clamp) : float3(0.0f, 0.0f, 0.0f);
  g_farOut0[p] = float4(nr.rgb + far_gi, nr.a);   // joint raw GI -> the existing VBGI denoise chain
  g_farOut1[p] = float4(far_gi, nr.a);            // far raw GI -> debug and the M4 far share
  if ((int)GTVBAO_debug_mode == 36) GTVBAO_WriteDebug(g_farDebug, p, float4(far_gi, 1.0f));
  if ((int)GTVBAO_debug_mode == 38) GTVBAO_WriteDebug(g_farDebug, p, float4(saturate(s1.a / max(GTVBAO_far_history_length * GTVBAO_far_candidates, 1.0f)), saturate(s0.w), g_farIn2.Load(int3(p, 0)).w, 1.0f));
}
