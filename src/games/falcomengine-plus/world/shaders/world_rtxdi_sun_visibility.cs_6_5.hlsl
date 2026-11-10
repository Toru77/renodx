#define RT12_TLAS_REGISTER t29
#define RT12_DYNAMIC_VERTICES_REGISTER t30
#define RTXDI_ENABLE_PRESAMPLING 0
#include "world_rt12_trace.hlsli"
#include "world_rtxdi_surface.hlsli"

Texture2D<float> g_rtxdi_depth : register(t31);
Texture2D<uint4> g_rtxdi_normal : register(t32);
Texture2D<uint4> g_rtxdi_scene : register(t33);
Texture2D<uint4> g_rtxdi_sun_noise : register(t61);

struct RtxdiSunResult {
  uint status;
  float distance;
  uint reason;
  uint flags;
};

uint RtxdiSunVisibilityWord(RtxdiSunResult r) {
  return r.status | (((r.flags >> RTXDI_SUN_LAYER_SHIFT) & RTXDI_SUN_LAYER_MASK) << RTXDI_SUN_LAYER_SHIFT)
      | (r.status == RTXDI_SUN_STATUS_VISIBLE
             ? (((r.flags >> RTXDI_SUN_WIND_OCCLUSION_SHIFT) & RTXDI_SUN_WIND_OCCLUSION_MASK) << RTXDI_SUN_WIND_OCCLUSION_SHIFT)
             : 0u);
}

RWStructuredBuffer<RtxdiSunResult> g_rtxdi_sun_results : register(u0);
RWTexture2D<uint> g_rtxdi_sun_visibility : register(u1);

cbuffer RtxdiSunParams : register(b0) {
  uint2 g_dimensions;
  float g_max_distance;
  float g_environment_bias;
  float g_character_bias;
  float3 g_region_min;
  float g_region_size;
  uint g_scene_resources_valid;
  uint g_coverage_valid;
  uint g_frame_index;
  uint g_require_flags;
  uint g_trace_mode;
  float g_sun_disk_tan;
  uint g_noise_mode;
  uint g_noise_slice;
  float g_foliage_transmission;
  float g_wind_transmission;
  float3 g_foliage_tint;
  float g_wind_softening;
};

#include "world_rtxdi_sun_adapter.hlsli"
#include <Rtxdi/DI/InitialSampling.hlsli>

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
  uint2 pixel = dispatch_id.xy;
  if (g_trace_mode == RTXDI_SUN_TRACE_HALF) pixel *= 2u;
  if (any(pixel >= g_dimensions)) return;
  if (g_trace_mode == RTXDI_SUN_TRACE_RESOLVE && all((pixel & 1u) == 0u)) return;

  RtxdiSunResult result = (RtxdiSunResult)0;
  result.status = RTXDI_SUN_STATUS_UNKNOWN;
  result.reason = RTXDI_SUN_REASON_INVALID_SURFACE;
  const uint output_index = pixel.y * g_dimensions.x + pixel.x;
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
  scene_rows[RTXDI_SURFACE_SUN_RESERVED_ROW - 1u] = g_rtxdi_scene.Load(int3(RTXDI_SURFACE_SUN_RESERVED_ROW - 1u, 0, 0));
  scene_rows[RTXDI_SURFACE_SUN_RESERVED_ROW] = g_rtxdi_scene.Load(int3(RTXDI_SURFACE_SUN_RESERVED_ROW, 0, 0));
  scene_rows[RTXDI_SURFACE_RESOLUTION_SCALING_ROW] = g_rtxdi_scene.Load(int3(RTXDI_SURFACE_RESOLUTION_SCALING_ROW, 0, 0));
  const RtxdiSurfaceResult surface = RtxdiReconstructSurface(g_rtxdi_depth.Load(int3(pixel, 0)), mrt0, pixel,
                                                              g_dimensions, scene_rows);
  if ((surface.flags & RTXDI_SURFACE_FLAG_VALID) == 0u) {
    g_rtxdi_sun_results[output_index] = result;
    g_rtxdi_sun_visibility[pixel] = RTXDI_SUN_STATUS_UNKNOWN;
    return;
  }
  if (g_scene_resources_valid == 0u) {
    result.reason = RTXDI_SUN_REASON_INVALID_SCENE;
    g_rtxdi_sun_results[output_index] = result;
    g_rtxdi_sun_visibility[pixel] = RTXDI_SUN_STATUS_UNKNOWN;
    return;
  }

  const float3 light_direction = -asfloat(scene_rows[RTXDI_SURFACE_SUN_RESERVED_ROW].xyz);
  const float direction_length_squared = dot(light_direction, light_direction);
  const float3 light_radiance = asfloat(scene_rows[RTXDI_SURFACE_SUN_RESERVED_ROW - 1u].xyz);
  if (!all(isfinite(light_direction)) || !isfinite(direction_length_squared) || direction_length_squared <= 1e-12
      || !all(isfinite(light_radiance)) || all(light_radiance <= 0.0)
      || !isfinite(g_max_distance) || g_max_distance <= 0.0 || !isfinite(g_region_size) || g_region_size <= 0.0) {
    result.reason = RTXDI_SUN_REASON_INVALID_RAY;
    g_rtxdi_sun_results[output_index] = result;
    g_rtxdi_sun_visibility[pixel] = RTXDI_SUN_STATUS_UNKNOWN;
    return;
  }

  RAB_Surface rab_surface = (RAB_Surface)0;
  rab_surface.world_position = surface.world_position;
  rab_surface.shading_normal = surface.shading_normal;
  rab_surface.view_direction = surface.view_direction;
  rab_surface.linear_view_depth = surface.linear_view_depth;
  rab_surface.flags = surface.flags;
  RTXDI_RandomSamplerState rng = RTXDI_InitRandomSampler(pixel, g_frame_index, 0u);
  RAB_LightSample selected_sun = (RAB_LightSample)0;
  const RTXDI_LightBufferRegion infinite_region = {0u, 1u, 0u, 0u};
  const RTXDI_DIReservoir sun_reservoir = RTXDI_SampleInfiniteLights(rng, rab_surface, 1u, infinite_region, selected_sun);
  const float inverse_pdf = RTXDI_GetDIReservoirInvPdf(sun_reservoir);
  if (!RTXDI_IsValidDIReservoir(sun_reservoir) || selected_sun.valid == 0u || !isfinite(inverse_pdf)
      || abs(inverse_pdf - 1.0) > 1e-5) {
    result.reason = RTXDI_SUN_REASON_INVALID_RAY;
    g_rtxdi_sun_results[output_index] = result;
    g_rtxdi_sun_visibility[pixel] = RTXDI_SUN_STATUS_UNKNOWN;
    return;
  }

  float3 sun_direction = selected_sun.direction;
  if (g_sun_disk_tan > 0.0) {
    float2 u;
    if (g_noise_mode != 0u) {
      u = asfloat(g_rtxdi_sun_noise.Load(int3(pixel.x % 128u, pixel.y % 128u + 128u * g_noise_slice, 0)).xy);
    } else {
      u = float2(RTXDI_GetNextRandom(rng), RTXDI_GetNextRandom(rng));
    }
    const float3 up = abs(sun_direction.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
    const float3 T = normalize(cross(up, sun_direction));
    const float3 B = cross(sun_direction, T);
    const float r = g_sun_disk_tan * sqrt(u.x);
    const float phi = 6.2831853 * u.y;
    sun_direction = normalize(sun_direction + (T * cos(phi) + B * sin(phi)) * r);
  }
  result.flags |= RTXDI_SUN_FLAG_UNIT_INVERSE_PDF;
  const bool character = (surface.flags & RTXDI_SURFACE_FLAG_CHARACTER) != 0u;
  if (g_trace_mode == RTXDI_SUN_TRACE_RESOLVE) {
    const uint2 base = pixel & ~1u;
    bool found = false;
    uint source_index = 0u;
    float best_plane_distance = 0.0;
    for (uint offset_y = 0u; offset_y < 4u; offset_y += 2u) {
      for (uint offset_x = 0u; offset_x < 4u; offset_x += 2u) {
        const uint2 candidate = base + uint2(offset_x, offset_y);
        if (any(candidate >= g_dimensions)) continue;
        if ((offset_x != 0u && (pixel.x & 1u) == 0u) || (offset_y != 0u && (pixel.y & 1u) == 0u)) continue;
        const uint candidate_index = candidate.y * g_dimensions.x + candidate.x;
        const RtxdiSunResult candidate_result = g_rtxdi_sun_results[candidate_index];
        if (candidate_result.status != RTXDI_SUN_STATUS_VISIBLE && candidate_result.status != RTXDI_SUN_STATUS_BLOCKED) continue;
        if (((candidate_result.flags & RTXDI_SUN_FLAG_CHARACTER) != 0u) != character) continue;
        const RtxdiSurfaceResult candidate_surface = RtxdiReconstructSurface(
            g_rtxdi_depth.Load(int3(candidate, 0)), g_rtxdi_normal.Load(int3(candidate, 0)), candidate, g_dimensions, scene_rows);
        if ((candidate_surface.flags & RTXDI_SURFACE_FLAG_VALID) == 0u) continue;
        const float plane_distance = abs(dot(candidate_surface.world_position - surface.world_position, surface.shading_normal));
        if (plane_distance > 0.02 * surface.linear_view_depth || dot(candidate_surface.shading_normal, surface.shading_normal) < 0.8) continue;
        if (!found || plane_distance < best_plane_distance) {
          found = true;
          best_plane_distance = plane_distance;
          source_index = candidate_index;
        }
      }
    }
    if (found) {
      g_rtxdi_sun_results[output_index] = g_rtxdi_sun_results[source_index];
      g_rtxdi_sun_visibility[pixel] = RtxdiSunVisibilityWord(g_rtxdi_sun_results[source_index]);
      return;
    }
  }
  const float normal_bias = character ? g_character_bias : g_environment_bias;
  if (!isfinite(normal_bias)) {
    result.reason = RTXDI_SUN_REASON_INVALID_RAY;
    g_rtxdi_sun_results[output_index] = result;
    g_rtxdi_sun_visibility[pixel] = RTXDI_SUN_STATUS_UNKNOWN;
    return;
  }
  const float bias_sign = (dot(surface.shading_normal, sun_direction) >= 0.0 ? 1.0 : -1.0);
  const float3 origin = surface.world_position + surface.shading_normal * (normal_bias * bias_sign);
  float coverage_end = 0.0;
  bool complete_segment = false;
  if (g_coverage_valid != 0u && !RtxdiSunCoverageEnd(origin, sun_direction, g_max_distance, coverage_end)) {
    result.reason = RTXDI_SUN_REASON_COVERAGE_MISS;
    result.flags |= (character ? RTXDI_SUN_FLAG_CHARACTER : 0u);
    g_rtxdi_sun_results[output_index] = result;
    g_rtxdi_sun_visibility[pixel] = RTXDI_SUN_STATUS_UNKNOWN;
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
  view.require_flags = g_require_flags;
  g_rt12_dynamic_mask = RT12_MASK_DYNAMIC;
  g_rt12_foliage_transmission = g_foliage_transmission;
  g_rt12_foliage_layers = 0u;
  g_rt12_wind_transmission = g_wind_transmission;
  g_rt12_wind_softening = g_wind_softening;
  g_rt12_wind_layers = 0u;
  WorldTraceCounters counters;
  const WorldTraceHit hit = TraceWorldRayDxr(origin, sun_direction, 0.0, coverage_end, true, view, counters);
  result.flags |= (character ? RTXDI_SUN_FLAG_CHARACTER : 0u);
  if (counters.invalid_refs != 0u) {
    result.reason = RTXDI_SUN_REASON_INVALID_ALPHA;
  } else if (counters.hits != 0u) {
    result.status = RTXDI_SUN_STATUS_BLOCKED;
    result.distance = hit.t;
    result.reason = RTXDI_SUN_REASON_NONE;
    if ((hit.instance & DYNAMIC_INSTANCE_FLAG) != 0u) {
      result.flags |= RTXDI_SUN_BLOCKER_CHARACTER;
    } else {
      const uint blocker_visibility = (uint)(g_trace_instances[hit.instance].visibility.z + 0.5);
      result.flags |= ((blocker_visibility & INSTANCE_FOLIAGE) != 0u ? RTXDI_SUN_BLOCKER_FOLIAGE : 0u);
      result.flags |= ((blocker_visibility & INSTANCE_ALPHA_TESTED) != 0u ? RTXDI_SUN_BLOCKER_ALPHA : 0u);
      result.flags |= ((blocker_visibility & INSTANCE_CAMERA_VISIBLE) == 0u ? RTXDI_SUN_BLOCKER_SHADOW_ONLY : 0u);
    }
  } else if (complete_segment) {
    result.status = RTXDI_SUN_STATUS_VISIBLE;
    result.distance = g_max_distance;
    result.reason = RTXDI_SUN_REASON_NONE;
    result.flags |= min(g_rt12_foliage_layers, RTXDI_SUN_LAYER_MASK) << RTXDI_SUN_LAYER_SHIFT;
    result.flags |= min(g_rt12_wind_occlusion, RTXDI_SUN_WIND_OCCLUSION_MASK) << RTXDI_SUN_WIND_OCCLUSION_SHIFT;
  } else {
    result.reason = RTXDI_SUN_REASON_TRUNCATED;
    result.flags |= RTXDI_SUN_FLAG_TRUNCATED;
  }
  g_rtxdi_sun_results[output_index] = result;
  g_rtxdi_sun_visibility[pixel] = RtxdiSunVisibilityWord(result);
}
