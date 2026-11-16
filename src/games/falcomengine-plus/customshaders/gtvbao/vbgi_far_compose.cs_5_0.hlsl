// Compose this frame's near GI with temporally accumulated far GI at trace resolution.
#include "vbgi_far_pass.hlsli"
[numthreads(8, 8, 1)]
void main(uint2 p : SV_DispatchThreadID)
{
  uint w, h;
  g_farOut0.GetDimensions(w, h);
  if (p.x >= w || p.y >= h) return;
  const float4 near_gi = g_farNearRaw.Load(int3(p, 0));
  const float4 far_gi = g_farIn0.Load(int3(p, 0));
  g_farOut0[p] = float4(near_gi.rgb + (far_gi.a >= 0.0f ? far_gi.rgb : float3(0.0f, 0.0f, 0.0f)), near_gi.a);
}
