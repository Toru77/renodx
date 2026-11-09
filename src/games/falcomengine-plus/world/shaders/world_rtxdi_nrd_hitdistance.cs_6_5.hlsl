// N3b: private first-bounce diffuse hit distance for the experimental NRD path.
// The N1 selected-light alpha is deliberately ignored. This pass traces the
// closest alpha-accepted scene hit over a finite world-space segment.
#define RT12_TLAS_REGISTER t29
#define RT12_DYNAMIC_VERTICES_REGISTER t30
#include "world_rt12_trace.hlsli"
#include "world_rtxdi_surface.hlsli"
#include "world_rtxdi_nrd_hitdistance_sampling.hlsli"
#include "world_rtxdi_nrd_hitdistance_coverage.hlsli"
#include "../rtxdi/rtxdi_di_layout.h"

Texture2D<float> g_n3b_depth : register(t31);
Texture2D<uint4> g_n3b_normal : register(t32);
Texture2D<uint4> g_n3b_scene : register(t33);
Texture2D<uint> g_n3b_n1_status : register(t34);

RWStructuredBuffer<float> g_n3b_hit_distance : register(u0);
RWStructuredBuffer<uint> g_n3b_hit_validity : register(u1);

cbuffer NrdHitDistanceParams : register(b0) {
  uint2 g_n3b_dimensions;
  uint g_n3b_frame;
  uint g_n3b_scene_valid;
  uint g_n3b_coverage_complete;
  float g_n3b_environment_bias;
  float g_n3b_character_bias;
  float g_n3b_max_distance;
  float3 g_n3b_coverage_min;
  float g_n3b_coverage_size;
};

[numthreads(8, 8, 1)]
void main(uint3 dispatch_id : SV_DispatchThreadID) {
  if (any(dispatch_id.xy >= g_n3b_dimensions)) return;
  const uint2 pixel = dispatch_id.xy;
  const uint pixel_index = pixel.y * g_n3b_dimensions.x + pixel.x;
  g_n3b_hit_distance[pixel_index] = 0.0;
  g_n3b_hit_validity[pixel_index] = 0u;

  if (g_n3b_scene_valid == 0u || g_n3b_coverage_complete == 0u
      || !isfinite(g_n3b_max_distance) || g_n3b_max_distance <= 0.0 || g_n3b_max_distance > 65504.0
      || !isfinite(g_n3b_environment_bias) || !isfinite(g_n3b_character_bias)
      || !isfinite(g_n3b_coverage_size) || g_n3b_coverage_size <= 0.0) return;

  uint4 scene_rows[RTXDI_SURFACE_SCENE_ROW_COUNT];
  [unroll]
  for (uint row = 0u; row < 8u; ++row) {
    scene_rows[row] = g_n3b_scene.Load(int3(row, 0, 0));
  }
  [unroll]
  for (uint row = 0u; row < 4u; ++row) {
    scene_rows[RTXDI_SURFACE_INVERSE_VIEW_PROJECTION_ROW + row] =
        g_n3b_scene.Load(int3(RTXDI_SURFACE_INVERSE_VIEW_PROJECTION_ROW + row, 0, 0));
  }
  scene_rows[RTXDI_SURFACE_SUN_RESERVED_ROW - 1u] =
      g_n3b_scene.Load(int3(RTXDI_SURFACE_SUN_RESERVED_ROW - 1u, 0, 0));
  scene_rows[RTXDI_SURFACE_SUN_RESERVED_ROW] = g_n3b_scene.Load(int3(RTXDI_SURFACE_SUN_RESERVED_ROW, 0, 0));
  scene_rows[RTXDI_SURFACE_RESOLUTION_SCALING_ROW] =
      g_n3b_scene.Load(int3(RTXDI_SURFACE_RESOLUTION_SCALING_ROW, 0, 0));

  const RtxdiSurfaceResult surface = RtxdiReconstructSurface(
      g_n3b_depth.Load(int3(pixel, 0)), g_n3b_normal.Load(int3(pixel, 0)), pixel, g_n3b_dimensions, scene_rows);
  if ((surface.flags & RTXDI_SURFACE_FLAG_VALID) == 0u) return;
  const uint n1_status = g_n3b_n1_status.Load(int3(pixel, 0));
  const uint n1_code = n1_status & 0x7u;
  const bool n1_known = (n1_status & RTXDI_N1_STATUS_KNOWN_BIT) != 0u
                        && (n1_code == RTXDI_N1_STATUS_VISIBLE || n1_code == RTXDI_N1_STATUS_BLOCKED);
  const bool character = (surface.flags & RTXDI_SURFACE_FLAG_CHARACTER) != 0u;
  const bool status_character = (n1_status & RTXDI_N1_STATUS_CHARACTER_BIT) != 0u;
  if (!n1_known || character != status_character) return;

  uint rng = NrdHitDistanceHash(pixel_index ^ NrdHitDistanceHash(g_n3b_frame + 0xA511E9B3u));
  const float3 ray_direction = NrdCosineHemisphere(surface.shading_normal, rng);
  const float normal_bias = character ? g_n3b_character_bias : g_n3b_environment_bias;
  const float bias_sign = dot(surface.shading_normal, ray_direction) >= 0.0 ? 1.0 : -1.0;
  const float3 ray_origin = surface.world_position + surface.shading_normal * (normal_bias * bias_sign);
  if (!all(isfinite(ray_origin)) || !all(isfinite(ray_direction))
      || abs(dot(ray_direction, ray_direction) - 1.0) > 1.0e-3) return;
  float coverage_end = 0.0;
  if (!NrdHitDistanceCoverageEnd(ray_origin, ray_direction, g_n3b_max_distance, g_n3b_coverage_min,
                                 g_n3b_coverage_size, coverage_end)) return;
  WorldCameraView view;
  view.hide = false;
  view.floor_value = 0.0;
  view.skip_flags = 0u;
  view.require_flags = 0u;
  // Include current-frame deforming characters and all shadow-only casters.
  // Dynamic TLAS instances use the scene lease's character snapshot and vertex table.
  g_rt12_dynamic_mask = RT12_MASK_DYNAMIC;
  WorldTraceCounters counters;
  const WorldTraceHit hit = TraceWorldRayDxr(ray_origin, ray_direction, 0.0, coverage_end, false, view, counters);
  if (counters.invalid_refs != 0u || counters.stack_overflow != 0u) return;

  const bool hit_valid = counters.hits != 0u;
  const float edge_tolerance = max(1.0e-4, coverage_end * 1.0e-5);
  if (hit_valid && hit.t >= coverage_end - edge_tolerance) return;
  const bool full_range_covered = NrdHitDistanceFullyCovered(coverage_end, g_n3b_max_distance);
  if (!hit_valid && !full_range_covered) return;
  const float distance = hit_valid ? hit.t : g_n3b_max_distance;
  if (!isfinite(distance) || distance <= 0.0 || distance > 65504.0) return;

  g_n3b_hit_distance[pixel_index] = distance;
  // bit 0: valid distance; bit 1: character receiver; bit 2: certified miss.
  g_n3b_hit_validity[pixel_index] = 1u | (character ? 2u : 0u) | (hit_valid ? 0u : 4u);
}
