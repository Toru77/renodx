#define SHARC_UPDATE 1
#include "world_pt_common.hlsli"
// SHARC update (OpenRemix pathtrace.comp:1656-1744 updateCache): one path per downscale cell, from the primary surface.
[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID) {
  const uint ds = max(g_sharc_downscale, 1u);
  const uint phase = g_frame % (ds * ds);
  const uint2 pixel = id.xy * ds + uint2(phase % ds, phase / ds);
  if (any(pixel >= g_size)) return;
  RtxdiSurfaceResult surface;
  if (!PtLoadSurface(pixel, surface)) return;
  g_rt12_dynamic_mask = g_dynamic_on != 0u ? RT12_MASK_DYNAMIC : 0u;
  WorldTraceCounters pt_counters;
  const WorldCameraView bounce_view = {true, 1.0, 0u, 0u};
  const SharcParameters sharc = PtSharcParameters();
  SharcState state;
  SharcInit(state);
  uint rng = PtHash(pixel.x + pixel.y * g_size.x + PtHash(g_frame)) ^ 0x53484152u;
  float3 p = surface.world_position;
  float3 n = surface.shading_normal;
  float bias = (surface.flags & RTXDI_SURFACE_FLAG_CHARACTER) != 0u ? g_char_bias : g_env_bias;
  const uint max_bounces = min(g_max_bounces, 8u);
  for (uint b = 0u; b < max_bounces; ++b) {
    const float3 dir = PtCosineDirection(n, rng);
    const WorldTraceHit hit = TraceWorldRayDxr(p + n * bias, dir, 1e-3, g_ray_max, false, bounce_view, pt_counters);
    if (hit.prim == 0xFFFFFFFFu) { SharcUpdateMiss(sharc, state, PtSky(dir)); break; }
    const float3 nh = dot(hit.normal, dir) > 0.0 ? -hit.normal : hit.normal;
    const float3 albedo = PtHitMaterial(hit.position, nh).albedo;
    float3 direct = albedo * PtHitLight(hit.position, nh, rng);
    if (any(isnan(direct)) || any(isinf(direct))) direct = float3(0, 0, 0);
    if (!SharcUpdateHit(sharc, state, PtSharcHit(hit.position, nh, dir, albedo), direct, PtRandom(rng))) break;
    SharcSetThroughput(state, albedo);
    p = hit.position;
    n = nh;
    bias = g_env_bias;
  }
}
