#ifndef FALCOM_WORLD_RT_SHADOW_RULES_HLSLI_
#define FALCOM_WORLD_RT_SHADOW_RULES_HLSLI_
// This software contains source code provided by NVIDIA Corporation.
#include "../rtxdi/rtxdi_surface_layout.h"
// One authoritative copy of the world-traced sun shadow rules (foliage, wind, blocker classification, tint and
// self-softening). Shared by the RTXDI sun pass, the local-light bridge and PT final shading. Functions only:
// every setting is a parameter and the caller owns the views and globals used by TraceWorldRayDxr.

struct Rt12ShadowResult {
  uint status;
  uint reason;
  float distance;
  uint flags;
};

// Visibility word consumed by the kept RTXDI debug view (world_rtxdi_debug.hlsli). Port of RtxdiSunVisibilityWord.
uint Rt12SunVisibilityWord(uint status, uint flags) {
  return status | (((flags >> RTXDI_SUN_LAYER_SHIFT) & RTXDI_SUN_LAYER_MASK) << RTXDI_SUN_LAYER_SHIFT)
      | (status == RTXDI_SUN_STATUS_VISIBLE
             ? (((flags >> RTXDI_SUN_WIND_OCCLUSION_SHIFT) & RTXDI_SUN_WIND_OCCLUSION_MASK) << RTXDI_SUN_WIND_OCCLUSION_SHIFT)
             : 0u);
}

// Character self-softening: shift the shadow origin laterally, then remove the inward component (M15/M16).
float3 Rt12SelfSofteningOffset(float3 outward, float3 lateral, float self_softening) {
  if (self_softening <= 0.0) return 0.0;
  lateral *= self_softening;
  return lateral - outward * min(dot(lateral, outward), 0.0);
}

// Exit of the prepared scene AABB along the ray; the segment beyond it has no coverage (M3.1b).
bool Rt12ShadowCoverageEnd(
    float3 origin, float3 direction, float max_distance, float3 region_min, float region_size, out float coverage_end) {
  coverage_end = 0.0;
  if (any(origin < region_min) || any(origin > region_min + region_size)) return false;
  float exit_distance = max_distance;
  for (uint axis = 0u; axis < 3u; ++axis) {
    if (abs(direction[axis]) <= 1e-8) continue;
    const float boundary = direction[axis] > 0.0 ? region_min[axis] + region_size : region_min[axis];
    exit_distance = min(exit_distance, (boundary - origin[axis]) / direction[axis]);
  }
  coverage_end = max(exit_distance, 0.0);
  return isfinite(coverage_end) && coverage_end > 0.0;
}

// Core sun ray: blocker classification, foliage/wind transmission, the wind second ray and the tint.
Rt12ShadowResult Rt12TraceSunShadow(
    float3 origin, float3 direction, float coverage_end, bool complete_segment, bool character, bool include_dynamic,
    uint require_flags, float max_distance, float foliage_transmission, float wind_transmission, float wind_softening,
    float3 tint, float2 wind_u, float3 tangent, float3 bitangent, out float transmittance, out float3 transmitted_tint) {
  Rt12ShadowResult result = (Rt12ShadowResult)0;
  result.status = RTXDI_SUN_STATUS_UNKNOWN;
  result.reason = RTXDI_SUN_REASON_NONE;
  transmittance = 1.0;
  transmitted_tint = 1.0;

  WorldCameraView view;
  view.hide = false;
  view.floor_value = 0.0;
  view.skip_flags = 0u;
  view.require_flags = require_flags;
  g_rt12_dynamic_mask = include_dynamic ? RT12_MASK_DYNAMIC : 0u;
  g_rt12_foliage_transmission = foliage_transmission;
  g_rt12_foliage_layers = 0u;
  g_rt12_wind_transmission = wind_transmission;
  g_rt12_wind_mode = (wind_transmission > 0.0 && wind_softening > 0.0) ? RT12_WIND_SKIP : RT12_WIND_COUNT;
  g_rt12_wind_layers = 0u;
  WorldTraceCounters counters;
  const WorldTraceHit hit = TraceWorldRayDxr(origin, direction, 0.0, coverage_end, true, view, counters);
  result.flags |= (character ? RTXDI_SUN_FLAG_CHARACTER : 0u);
  if (counters.invalid_refs != 0u) {
    result.reason = RTXDI_SUN_REASON_INVALID_ALPHA;
  } else if (counters.hits != 0u) {
    result.status = RTXDI_SUN_STATUS_BLOCKED;
    result.distance = hit.t;
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
    result.distance = max_distance;
    result.flags |= min(g_rt12_foliage_layers, RTXDI_SUN_LAYER_MASK) << RTXDI_SUN_LAYER_SHIFT;
    if (g_rt12_wind_mode == RT12_WIND_SKIP) {
      const float wind_radius = wind_softening * sqrt(wind_u.x);
      const float wind_phi = 6.2831853 * wind_u.y;
      g_rt12_wind_mode = RT12_WIND_ONLY;
      g_rt12_wind_layers = 0u;
      WorldTraceCounters wind_counters;
      TraceWorldRayDxr(origin + (tangent * cos(wind_phi) + bitangent * sin(wind_phi)) * wind_radius, direction, 0.0,
                       coverage_end, true, view, wind_counters);
    }
    result.flags |= min(g_rt12_wind_occlusion, RTXDI_SUN_WIND_OCCLUSION_MASK) << RTXDI_SUN_WIND_OCCLUSION_SHIFT;
    const uint layers = (result.flags >> RTXDI_SUN_LAYER_SHIFT) & RTXDI_SUN_LAYER_MASK;
    const uint wind_occlusion = (result.flags >> RTXDI_SUN_WIND_OCCLUSION_SHIFT) & RTXDI_SUN_WIND_OCCLUSION_MASK;
    transmittance = (layers == 0u ? 1.0 : pow(saturate(foliage_transmission), layers)) * (1.0 - wind_occlusion / 255.0);
    transmitted_tint = lerp(float3(1.0, 1.0, 1.0), tint, saturate(float(layers) / float(max(RTXDI_SUN_LAYER_MASK, 1u))));
  } else {
    result.reason = RTXDI_SUN_REASON_TRUNCATED;
    result.flags |= RTXDI_SUN_FLAG_TRUNCATED;
  }
  return result;
}

#endif  // FALCOM_WORLD_RT_SHADOW_RULES_HLSLI_
