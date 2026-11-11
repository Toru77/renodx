#pragma once
// Shared by world_pt_diffuse, world_pt_sharc_update and world_pt_sharc_resolve (world/pt/pt_rt12.hpp). The including
// file defines SHARC_UPDATE first.
#ifndef SHARC_UPDATE
#error "define SHARC_UPDATE before world_pt_common.hlsli"
#endif
#define RT12_TLAS_REGISTER t29
#define RT12_DYNAMIC_VERTICES_REGISTER t30
#include "world_rt12_trace.hlsli"
#include "world_rtxdi_surface.hlsli"
#include "world_rt_lamps.hlsli"
// SHARC configuration (compile time, identical in the three passes): OpenRemix sharc.glsl:9-19.
#define SHARC_ENABLE_RESPONSIVE_LIGHTING 0
#define SHARC_MATERIAL_DEMODULATION 1
#define SHARC_ENABLE_SH_ENCODING 1
#define SHARC_ENABLE_CACHE_RESAMPLING 1
#define SHARC_SAMPLE_NUM_THRESHOLD 3
#define SHARC_PROPAGATION_DEPTH 3
#define SHARC_RESAMPLING_DEPTH_MIN 2
#include "../sharc/vendor/include/SharcCommon.h"
Texture2D<float> g_pt_depth : register(t31);
Texture2D<uint4> g_pt_normal : register(t32);
Texture2D<uint4> g_pt_scene : register(t33);
StructuredBuffer<LightParam> g_pt_lights : register(t34);
StructuredBuffer<uint> g_pt_light_types : register(t35);
RWTexture2D<float4> g_pt_output : register(u0);
RWStructuredBuffer<uint64_t> g_pt_sharc_hash : register(u1);
RWStructuredBuffer<SharcAccumulationData> g_pt_sharc_accumulation : register(u2);
RWStructuredBuffer<SharcPackedData> g_pt_sharc_resolved : register(u3);
cbuffer PtParams : register(b0) {  // order = params[] in pt_rt12.hpp (32 dwords)
  uint2 g_size; uint g_frame; uint g_samples; uint g_max_bounces; uint g_roulette; uint g_roulette_min_bounce;
  uint g_lamp_count; uint g_sun_casters_only; uint g_dynamic_on; float g_ray_max; float g_env_bias; float g_char_bias;
  float g_lamp_clearance; float g_sky_intensity; uint g_sharc_enabled;
  uint g_sharc_capacity; uint g_sharc_downscale; uint g_sharc_window; uint g_sharc_stale_frames;
  float g_sharc_scene_scale; float3 g_sharc_camera;
  float3 g_sharc_camera_prev; uint g_sharc_mode;
  uint g_pad0; uint g_pad1; uint g_pad2; uint g_pad3;
};
uint PtHash(uint x) { x ^= x >> 16; x *= 0x7feb352du; x ^= x >> 15; x *= 0x846ca68bu; return x ^ (x >> 16); }
float PtRandom(inout uint s) { s = PtHash(s + 0x9e3779b9u); return float(s >> 8) * (1.0 / 16777216.0); }
float3 PtHitAlbedo(WorldTraceHit hit) { return float3(0.5, 0.5, 0.5); }  // M3 replaces this with the material cache
static float3 g_pt_sun_color;
static float3 g_pt_sun_dir;
static float3 g_pt_probe[9];
// Loads the scene rows, the sun and the ambient probe, and reconstructs the pixel's primary surface.
bool PtLoadSurface(uint2 pixel, out RtxdiSurfaceResult surface) {
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
  surface = RtxdiReconstructSurface(g_pt_depth.Load(int3(pixel, 0)), g_pt_normal.Load(int3(pixel, 0)), pixel, g_size, scene_rows);
  if ((surface.flags & RTXDI_SURFACE_FLAG_VALID) == 0u) return false;
  g_pt_sun_color = asfloat(g_pt_scene.Load(int3(25, 0, 0)).xyz);
  g_pt_sun_dir = normalize(-asfloat(g_pt_scene.Load(int3(26, 0, 0)).xyz));
  [unroll]
  for (uint i = 0u; i < 9u; ++i) g_pt_probe[i] = asfloat(g_pt_scene.Load(int3(37 + i, 0, 0)).xyz);
  return true;
}
float3 PtSky(float3 dir) {
  const float3 sky = g_pt_probe[0] + g_pt_probe[1] * dir.x + g_pt_probe[2] * dir.y + g_pt_probe[3] * dir.z + g_pt_probe[4] * (dir.z * dir.x)
                   + g_pt_probe[5] * (dir.y * dir.z) + g_pt_probe[6] * (dir.y * dir.x) + g_pt_probe[7] * (3 * dir.z * dir.z - 1)
                   + g_pt_probe[8] * (dir.x * dir.x - dir.y * dir.y);
  return g_sky_intensity * max(sky, 0);
}
float3 PtCosineDirection(float3 n, inout uint rng) {
  const float u1 = PtRandom(rng);
  const float u2 = PtRandom(rng);
  const float3 up = abs(n.z) < 0.999 ? float3(0, 0, 1) : float3(1, 0, 0);
  const float3 t = normalize(cross(up, n));
  const float3 bt = cross(n, t);
  return normalize(t * (sqrt(u1) * cos(6.2831853 * u2)) + bt * (sqrt(u1) * sin(6.2831853 * u2)) + n * sqrt(1.0 - u1));
}
// Light arriving at a bounce hit: the sun with a shadow ray and one lamp sampled uniformly. The caller multiplies by albedo.
float3 PtHitLight(float3 position, float3 nh, inout uint rng) {
  WorldTraceCounters counters;
  const WorldCameraView sun_view = {false, 1.0, 0u, g_sun_casters_only != 0u ? INSTANCE_SHADOW_CASTER : 0u};
  const WorldCameraView lamp_view = {false, 1.0, 0u, 0u};
  const float3 q = position + nh * g_env_bias;
  float3 sun = float3(0, 0, 0);
  const float ndl = saturate(dot(nh, g_pt_sun_dir));
  if (ndl > 0.0 && TraceWorldRayDxr(q, g_pt_sun_dir, 1e-3, g_ray_max, true, sun_view, counters).prim == 0xFFFFFFFFu) {
    sun = g_pt_sun_color * ndl;
  }
  float3 lamp = float3(0, 0, 0);
  if (g_lamp_count > 0u) {
    const uint i = min(uint(PtRandom(rng) * g_lamp_count), g_lamp_count - 1u);
    const uint type = g_pt_light_types[i];
    if (type == RTGI_LAMP_TYPE_POINT || type == RTGI_LAMP_TYPE_SPOT) {
      const LightParam lp = g_pt_lights[i];
      const float3 to = lp.pos - position;
      const float3 l = normalize(to);
      const float e = LampDistanceAttenuation(lp, to) * (type == RTGI_LAMP_TYPE_SPOT ? LampSpotCone(lp, l) : 1.0)
                    * max(lp.translucency, dot(nh, l));
      const float reach = length(to) - g_lamp_clearance;
      const bool vis = reach <= 1e-3 || TraceWorldRayDxr(q, l, 1e-3, reach, true, lamp_view, counters).prim == 0xFFFFFFFFu;
      if (e > 0.0 && vis) lamp = max(lp.color, 0.0) * e * g_lamp_count;
    }
  }
  return sun + lamp;
}
SharcParameters PtSharcParameters() {
  SharcParameters p;
  p.hashGridParameters.cameraPosition = g_sharc_camera;
  p.hashGridParameters.logarithmBase = 2;
  p.hashGridParameters.sceneScale = g_sharc_scene_scale;
  p.hashGridParameters.levelBias = 0;
  p.hashGridData.capacity = g_sharc_capacity;
  p.hashGridData.hashEntriesBuffer = g_pt_sharc_hash;
  p.radianceScale = 1000;
  p.accumulationBuffer = g_pt_sharc_accumulation;
  p.resolvedBuffer = g_pt_sharc_resolved;
  return p;
}
// A bounce hit's cache data (OpenRemix pathtrace.comp:1097-1124): placed slightly above the surface, toward the ray.
SharcHitData PtSharcHit(float3 position, float3 nh, float3 dir, float3 albedo) {
  SharcHitData data;
  data.positionWorld = position + nh * (1e-3 * length(position - g_sharc_camera) + 1e-4);
  data.normalWorld = nh;
  data.radianceDirectionWorld = -dir;
  data.radianceDirectionWeight = 0;
  data.materialDemodulation = max(albedo, float3(0.05, 0.05, 0.05));
  return data;
}
