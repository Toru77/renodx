#define SHARC_UPDATE 0
#include "world_pt_common.hlsli"
[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID) {
  if (any(id.xy >= g_size)) return;
  RtxdiSurfaceResult surface;
  if (!PtLoadSurface(id.xy, surface)) { g_pt_output[id.xy] = 0; return; }
  g_rt12_dynamic_mask = g_dynamic_on != 0u ? RT12_MASK_DYNAMIC : 0u;
  WorldTraceCounters pt_counters;  // required out argument of TraceWorldRayDxr; discarded
  const WorldCameraView bounce_view = {true, 1.0, 0u, 0u};
  const SharcParameters sharc = PtSharcParameters();
  uint rng = PtHash(id.x + id.y * g_size.x + PtHash(g_frame));
  float3 L = float3(0, 0, 0);
  for (uint sample = 0u; sample < g_samples; ++sample) {
    float3 p = surface.world_position;
    float3 n = surface.shading_normal;
    float3 T = float3(1, 1, 1);
    float bias = (surface.flags & RTXDI_SURFACE_FLAG_CHARACTER) != 0u ? g_char_bias : g_env_bias;
    for (uint b = 0u; b < g_max_bounces; ++b) {
      const float3 dir = PtCosineDirection(n, rng);
      const WorldTraceHit hit = TraceWorldRayDxr(p + n * bias, dir, 1e-3, g_ray_max, false, bounce_view, pt_counters);
      if (hit.prim == 0xFFFFFFFFu) { L += T * PtSky(dir); break; }
      const float3 nh = dot(hit.normal, dir) > 0.0 ? -hit.normal : hit.normal;
      const float3 albedo = PtHitAlbedo(hit);
      if (g_sharc_enabled != 0u) {
        const SharcHitData data = PtSharcHit(hit.position, nh, dir, albedo);
        const float voxel = HashGridGetVoxelSize(HashGridGetLevel(data.positionWorld, sharc.hashGridParameters), sharc.hashGridParameters);
        float3 cached;
        if (hit.t > voxel * 1.732050808 && SharcGetCachedRadiance(sharc, data, cached, false) && all(isfinite(cached))) {
          L += T * max(cached, 0);
          break;
        }
      }
      L += T * albedo * PtHitLight(hit.position, nh, rng);
      T *= albedo;
      if (g_roulette != 0u && b + 1u >= g_roulette_min_bounce) {
        const float pr = max(0.05, pow(saturate(max(T.x, max(T.y, T.z))), 0.25));
        if (PtRandom(rng) >= pr) break;
        T /= pr;
      }
      p = hit.position;
      n = nh;
      bias = g_env_bias;
    }
  }
  float3 A = L / float(g_samples);
  if (any(isnan(A)) || any(isinf(A))) A = float3(0, 0, 0);
  g_pt_output[id.xy] = float4(max(A, 0), 1);
}
