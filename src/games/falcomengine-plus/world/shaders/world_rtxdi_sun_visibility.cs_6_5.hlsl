#define RT12_TLAS_REGISTER t29
#define RT12_DYNAMIC_VERTICES_REGISTER t30
#include "world_rt12_trace.hlsli"
#include "world_rtxdi_surface.hlsli"

Texture2D<float> g_rtxdi_depth : register(t31);
Texture2D<uint4> g_rtxdi_normal : register(t32);
Texture2D<uint4> g_rtxdi_scene : register(t33);

struct RtxdiSunResult {
  uint status;
  float distance;
  uint reason;
  uint flags;
};

RWStructuredBuffer<RtxdiSunResult> g_rtxdi_sun_results : register(u0);

cbuffer RtxdiSunParams : register(b0) {
  uint2 g_dimensions;
  float g_max_distance;
  float g_environment_bias;
  float g_character_bias;
  float3 g_region_min;
  float g_region_size;
  uint g_scene_resources_valid;
  uint g_coverage_valid;
};

bool RtxdiSunCoverageEnd(float3 origin, float3 direction, float max_distance, out float coverage_end) {
  coverage_end = 0.0;
  if (any(origin < g_region_min) || any(origin > g_region_min + g_region_size)) return false;

  float exit_distance = max_distance;
  for (uint axis = 0u; axis < 3u; ++axis) {
    if (abs(direction[axis]) <= 1e-8) continue;
    const float boundary = (direction[axis] > 0.0 ? g_region_min[axis] + g_region_size : g_region_min[axis]);
    exit_distance = min(exit_distance, (boundary - origin[axis]) / direction[axis]);
  }
  coverage_end = max(exit_distance, 0.0);
  return isfinite(coverage_end) && coverage_end > 0.0;
}

[numthreads(8, 8, 1)]
void main(uint3 dispatch_id : SV_DispatchThreadID) {
  if (any(dispatch_id.xy >= g_dimensions)) return;

  RtxdiSunResult result = (RtxdiSunResult)0;
  result.status = RTXDI_SUN_STATUS_UNKNOWN;
  result.reason = RTXDI_SUN_REASON_INVALID_SURFACE;
  const uint2 pixel = dispatch_id.xy;
  const uint4 mrt0 = g_rtxdi_normal.Load(int3(pixel, 0));
  uint4 scene_rows[RTXDI_SURFACE_SCENE_ROW_COUNT];
  [unroll]
  for (uint row = 0u; row < 8u; ++row) {
    scene_rows[row] = g_rtxdi_scene.Load(int3(row, 0, 0));
  }
  [unroll]
  for (uint row = 0u; row < 4u; ++row) {
    scene_rows[RTXDI_SURFACE_INVERSE_VIEW_PROJECTION_ROW + row] =
        g_rtxdi_scene.Load(int3(RTXDI_SURFACE_INVERSE_VIEW_PROJECTION_ROW + row, 0, 0));
  }
  scene_rows[RTXDI_SURFACE_SUN_RESERVED_ROW] = g_rtxdi_scene.Load(int3(RTXDI_SURFACE_SUN_RESERVED_ROW, 0, 0));
  scene_rows[RTXDI_SURFACE_RESOLUTION_SCALING_ROW] = g_rtxdi_scene.Load(int3(RTXDI_SURFACE_RESOLUTION_SCALING_ROW, 0, 0));
  const RtxdiSurfaceResult surface = RtxdiReconstructSurface(g_rtxdi_depth.Load(int3(pixel, 0)), mrt0, pixel,
                                                              g_dimensions, scene_rows);
  if ((surface.flags & RTXDI_SURFACE_FLAG_VALID) == 0u) {
    g_rtxdi_sun_results[pixel.y * g_dimensions.x + pixel.x] = result;
    return;
  }
  if (g_scene_resources_valid == 0u) {
    result.reason = RTXDI_SUN_REASON_INVALID_SCENE;
    g_rtxdi_sun_results[pixel.y * g_dimensions.x + pixel.x] = result;
    return;
  }

  const float3 light_direction = -asfloat(scene_rows[RTXDI_SURFACE_SUN_RESERVED_ROW].xyz);
  const float direction_length_squared = dot(light_direction, light_direction);
  if (!all(isfinite(light_direction)) || !isfinite(direction_length_squared) || direction_length_squared <= 1e-12
      || !isfinite(g_max_distance) || g_max_distance <= 0.0 || !isfinite(g_region_size) || g_region_size <= 0.0) {
    result.reason = RTXDI_SUN_REASON_INVALID_RAY;
    g_rtxdi_sun_results[pixel.y * g_dimensions.x + pixel.x] = result;
    return;
  }

  const float3 sun_direction = light_direction * rsqrt(direction_length_squared);
  const bool character = (surface.flags & RTXDI_SURFACE_FLAG_CHARACTER) != 0u;
  const float normal_bias = character ? g_character_bias : g_environment_bias;
  if (!isfinite(normal_bias)) {
    result.reason = RTXDI_SUN_REASON_INVALID_RAY;
    g_rtxdi_sun_results[pixel.y * g_dimensions.x + pixel.x] = result;
    return;
  }
  const float bias_sign = (dot(surface.shading_normal, sun_direction) >= 0.0 ? 1.0 : -1.0);
  const float3 origin = surface.world_position + surface.shading_normal * (normal_bias * bias_sign);
  float coverage_end = 0.0;
  bool complete_segment = false;
  if (g_coverage_valid != 0u && !RtxdiSunCoverageEnd(origin, sun_direction, g_max_distance, coverage_end)) {
    result.reason = RTXDI_SUN_REASON_COVERAGE_MISS;
    result.flags = (character ? RTXDI_SUN_FLAG_CHARACTER : 0u);
    g_rtxdi_sun_results[pixel.y * g_dimensions.x + pixel.x] = result;
    return;
  }
  if (g_coverage_valid == 0u) {
    coverage_end = g_max_distance;
  } else {
    complete_segment = coverage_end >= g_max_distance;
  }

  WorldCameraView view;
  view.hide = false;
  view.floor_value = 0.0;
  view.skip_flags = 0u;
  view.require_flags = 0u;
  g_rt12_dynamic_mask = RT12_MASK_DYNAMIC;
  WorldTraceCounters counters;
  const WorldTraceHit hit = TraceWorldRayDxr(origin, sun_direction, 0.0, coverage_end, true, view, counters);
  result.flags = (character ? RTXDI_SUN_FLAG_CHARACTER : 0u);
  if (counters.invalid_refs != 0u) {
    result.reason = RTXDI_SUN_REASON_INVALID_ALPHA;
  } else if (counters.hits != 0u) {
    result.status = RTXDI_SUN_STATUS_BLOCKED;
    result.distance = hit.t;
    result.reason = RTXDI_SUN_REASON_NONE;
  } else if (complete_segment) {
    result.status = RTXDI_SUN_STATUS_VISIBLE;
    result.distance = g_max_distance;
    result.reason = RTXDI_SUN_REASON_NONE;
  } else {
    result.reason = RTXDI_SUN_REASON_TRUNCATED;
    result.flags |= RTXDI_SUN_FLAG_TRUNCATED;
  }
  g_rtxdi_sun_results[pixel.y * g_dimensions.x + pixel.x] = result;
}
