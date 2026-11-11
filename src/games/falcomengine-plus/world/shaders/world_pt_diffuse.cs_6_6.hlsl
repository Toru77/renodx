#define RT12_TLAS_REGISTER t29
#define RT12_DYNAMIC_VERTICES_REGISTER t30
#include "world_rt12_trace.hlsli"
#include "world_rtxdi_surface.hlsli"
#include "world_rt_lamps.hlsli"
Texture2D<float> g_pt_depth : register(t31);
Texture2D<uint4> g_pt_normal : register(t32);
Texture2D<uint4> g_pt_scene : register(t33);
StructuredBuffer<LightParam> g_pt_lights : register(t34);
StructuredBuffer<uint> g_pt_light_types : register(t35);
RWTexture2D<float4> g_pt_output : register(u0);
cbuffer PtParams : register(b0) {  // order = params[] in pt_rt12.hpp
  uint2 g_size; uint g_frame; uint g_samples; uint g_max_bounces; uint g_roulette; uint g_roulette_min_bounce;
  uint g_lamp_count; uint g_sun_casters_only; uint g_dynamic_on; float g_ray_max; float g_env_bias; float g_char_bias;
  float g_lamp_clearance; float g_sky_intensity; uint g_pad;
};
uint PtHash(uint x) { x ^= x >> 16; x *= 0x7feb352du; x ^= x >> 15; x *= 0x846ca68bu; return x ^ (x >> 16); }
float PtRandom(inout uint s) { s = PtHash(s + 0x9e3779b9u); return float(s >> 8) * (1.0 / 16777216.0); }
float3 PtHitAlbedo(WorldTraceHit hit) { return float3(0.5, 0.5, 0.5); }  // M3 replaces this with the material cache
[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID) {
  if (any(id.xy >= g_size)) return;
  uint4 scene_rows[RTXDI_SURFACE_SCENE_ROW_COUNT];
  [unroll]
  for (uint row = 0u; row < 8u; ++row) {
    scene_rows[row] = g_pt_scene.Load(int3(row, 0, 0));
  }
  [unroll]
  for (uint row = 0u; row < 4u; ++row) {
    scene_rows[RTXDI_SURFACE_INVERSE_VIEW_PROJECTION_ROW + row] =
        g_pt_scene.Load(int3(RTXDI_SURFACE_INVERSE_VIEW_PROJECTION_ROW + row, 0, 0));
  }
  scene_rows[RTXDI_SURFACE_SUN_RESERVED_ROW - 1u] = g_pt_scene.Load(int3(RTXDI_SURFACE_SUN_RESERVED_ROW - 1u, 0, 0));
  scene_rows[RTXDI_SURFACE_SUN_RESERVED_ROW] = g_pt_scene.Load(int3(RTXDI_SURFACE_SUN_RESERVED_ROW, 0, 0));
  scene_rows[RTXDI_SURFACE_RESOLUTION_SCALING_ROW] = g_pt_scene.Load(int3(RTXDI_SURFACE_RESOLUTION_SCALING_ROW, 0, 0));
  const RtxdiSurfaceResult surface = RtxdiReconstructSurface(g_pt_depth.Load(int3(id.xy, 0)), g_pt_normal.Load(int3(id.xy, 0)), id.xy, g_size, scene_rows);
  if ((surface.flags & RTXDI_SURFACE_FLAG_VALID) == 0u) { g_pt_output[id.xy] = 0; return; }
  const float3 sun_color = asfloat(g_pt_scene.Load(int3(25, 0, 0)).xyz);
  const float3 sun_dir = normalize(-asfloat(g_pt_scene.Load(int3(26, 0, 0)).xyz));
  float3 probe[9];
  [unroll]
  for (uint i = 0u; i < 9u; ++i) probe[i] = asfloat(g_pt_scene.Load(int3(37 + i, 0, 0)).xyz);
  g_rt12_dynamic_mask = g_dynamic_on != 0u ? RT12_MASK_DYNAMIC : 0u;
  WorldTraceCounters pt_counters;  // required out argument of TraceWorldRayDxr; discarded
  const WorldCameraView bounce_view = {true, 1.0, 0u, 0u};
  const WorldCameraView sun_view = {false, 1.0, 0u, g_sun_casters_only != 0u ? INSTANCE_SHADOW_CASTER : 0u};
  const WorldCameraView lamp_view = {false, 1.0, 0u, 0u};
  uint rng = PtHash(id.x + id.y * g_size.x + PtHash(g_frame));
  float3 L = float3(0, 0, 0);
  for (uint sample = 0u; sample < g_samples; ++sample) {
    float3 p = surface.world_position;
    float3 n = surface.shading_normal;
    float3 T = float3(1, 1, 1);
    float bias = (surface.flags & RTXDI_SURFACE_FLAG_CHARACTER) != 0u ? g_char_bias : g_env_bias;
    for (uint b = 0u; b < g_max_bounces; ++b) {
      const float u1 = PtRandom(rng);
      const float u2 = PtRandom(rng);
      const float3 up = abs(n.z) < 0.999 ? float3(0, 0, 1) : float3(1, 0, 0);
      const float3 t = normalize(cross(up, n));
      const float3 bt = cross(n, t);
      const float3 dir = normalize(t * (sqrt(u1) * cos(6.2831853 * u2)) + bt * (sqrt(u1) * sin(6.2831853 * u2)) + n * sqrt(1.0 - u1));
      const WorldTraceHit hit = TraceWorldRayDxr(p + n * bias, dir, 1e-3, g_ray_max, false, bounce_view, pt_counters);
      if (hit.prim == 0xFFFFFFFFu) {
        const float3 sky = probe[0] + probe[1] * dir.x + probe[2] * dir.y + probe[3] * dir.z + probe[4] * (dir.z * dir.x)
                         + probe[5] * (dir.y * dir.z) + probe[6] * (dir.y * dir.x) + probe[7] * (3 * dir.z * dir.z - 1)
                         + probe[8] * (dir.x * dir.x - dir.y * dir.y);
        L += T * g_sky_intensity * max(sky, 0);
        break;
      }
      const float3 nh = dot(hit.normal, dir) > 0.0 ? -hit.normal : hit.normal;
      const float3 albedo = PtHitAlbedo(hit);
      const float3 q = hit.position + nh * g_env_bias;
      float3 sun = float3(0, 0, 0);
      const float ndl = saturate(dot(nh, sun_dir));
      if (ndl > 0.0 && TraceWorldRayDxr(q, sun_dir, 1e-3, g_ray_max, true, sun_view, pt_counters).prim == 0xFFFFFFFFu) {
        sun = sun_color * ndl;
      }
      float3 lamp = float3(0, 0, 0);
      if (g_lamp_count > 0u) {
        const uint i = min(uint(PtRandom(rng) * g_lamp_count), g_lamp_count - 1u);
        const uint type = g_pt_light_types[i];
        if (type == RTGI_LAMP_TYPE_POINT || type == RTGI_LAMP_TYPE_SPOT) {
          const LightParam lp = g_pt_lights[i];
          const float3 to = lp.pos - hit.position;
          const float3 l = normalize(to);
          const float e = LampDistanceAttenuation(lp, to) * (type == RTGI_LAMP_TYPE_SPOT ? LampSpotCone(lp, l) : 1.0)
                        * max(lp.translucency, dot(nh, l));
          const float reach = length(to) - g_lamp_clearance;
          const bool vis = reach <= 1e-3 || TraceWorldRayDxr(q, l, 1e-3, reach, true, lamp_view, pt_counters).prim == 0xFFFFFFFFu;
          if (e > 0.0 && vis) lamp = max(lp.color, 0.0) * e * g_lamp_count;
        }
      }
      L += T * albedo * (sun + lamp);
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
