// M2b probe: one ray against one triangle at z = 0 (rt12_probe.hpp stage 12).
RaytracingAccelerationStructure g_scene : register(t0);
RWStructuredBuffer<uint> g_out : register(u0);

[numthreads(1, 1, 1)]
void main() {
  RayDesc ray;
  ray.Origin = float3(0.0, 0.0, -1.0);
  ray.Direction = float3(0.0, 0.0, 1.0);
  ray.TMin = 0.0;
  ray.TMax = 10.0;
  RayQuery<RAY_FLAG_FORCE_OPAQUE | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH> query;
  query.TraceRayInline(g_scene, RAY_FLAG_NONE, 0xFF, ray);
  query.Proceed();
  const uint status = query.CommittedStatus();
  const bool hit = status == COMMITTED_TRIANGLE_HIT;
  const float2 bary = hit ? query.CommittedTriangleBarycentrics() : float2(0.0, 0.0);
  g_out[0] = status;
  g_out[1] = hit ? asuint(query.CommittedRayT()) : 0u;
  g_out[2] = asuint(bary.x);
  g_out[3] = asuint(bary.y);
}
