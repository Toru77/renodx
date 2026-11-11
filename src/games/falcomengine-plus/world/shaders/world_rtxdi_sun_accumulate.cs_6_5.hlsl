#include "world_rtxdi_surface.hlsli"
#include <Rtxdi/Utils/Math.hlsli>

struct RtxdiSunResult {
  uint status;
  float distance;
  uint reason;
  uint flags;
};

Texture2D<float> g_depth : register(t0);
Texture2D<uint4> g_normal : register(t1);
Texture2D<uint4> g_scene : register(t2);
Texture2D<float4> g_motion : register(t3);
Texture2D<float> g_prev_depth : register(t4);
Texture2D<uint4> g_prev_normal : register(t5);
Texture2D<uint4> g_prev_scene : register(t6);
Texture2D<float2> g_prev_history : register(t7);
RWStructuredBuffer<RtxdiSunResult> g_sun_results : register(u0);
RWTexture2D<uint> g_sun_visibility : register(u1);
RWTexture2D<float2> g_history : register(u2);

cbuffer RtxdiSunAccumulateParams : register(b0) {
  uint2 g_dimensions;
  uint g_history_valid;
  uint g_accumulation_frames;
  float g_foliage_transmission;
};

// History tests use the RTXDI SDK temporal defaults (relative view depth, normal cosine).
static const float kSunHistoryDepthThreshold = 0.1;
static const float kSunHistoryNormalThreshold = 0.5;

// This frame's sun visibility of one pixel, from its ray result; false for UNKNOWN.
bool RtxdiSunInstantVisibility(uint2 pixel, out float visibility, out bool character) {
  const RtxdiSunResult result = g_sun_results[pixel.y * g_dimensions.x + pixel.x];
  character = (result.flags & RTXDI_SUN_FLAG_CHARACTER) != 0u;
  visibility = result.status == RTXDI_SUN_STATUS_VISIBLE
                   ? RtxdiSunVisibleFraction((result.flags >> RTXDI_SUN_LAYER_SHIFT) & RTXDI_SUN_LAYER_MASK,
                                             (result.flags >> RTXDI_SUN_WIND_OCCLUSION_SHIFT) & RTXDI_SUN_WIND_OCCLUSION_MASK,
                                             g_foliage_transmission)
                   : 0.0;
  return result.status == RTXDI_SUN_STATUS_VISIBLE || result.status == RTXDI_SUN_STATUS_BLOCKED;
}

[numthreads(8, 8, 1)]
void main(uint3 dispatch_id : SV_DispatchThreadID) {
  const uint2 pixel = dispatch_id.xy;
  if (any(pixel >= g_dimensions)) return;
  float current = 0.0;
  bool character = false;
  if (!RtxdiSunInstantVisibility(pixel, current, character)) {
    // UNKNOWN keeps its raw word and resets this pixel's history.
    g_history[pixel] = float2(0.0, 0.0);
    return;
  }
  float neighborhood_min = current;
  float neighborhood_max = current;
  for (int offset_y = -1; offset_y <= 1; ++offset_y) {
    for (int offset_x = -1; offset_x <= 1; ++offset_x) {
      const int2 neighbor = int2(pixel) + int2(offset_x, offset_y);
      if ((offset_x == 0 && offset_y == 0) || any(neighbor < 0) || any(neighbor >= int2(g_dimensions))) continue;
      float neighbor_visibility = 0.0;
      bool neighbor_character = false;
      if (!RtxdiSunInstantVisibility(uint2(neighbor), neighbor_visibility, neighbor_character)
          || neighbor_character != character) continue;
      neighborhood_min = min(neighborhood_min, neighbor_visibility);
      neighborhood_max = max(neighborhood_max, neighbor_visibility);
    }
  }

  float history_length = 0.0;
  float history = current;
  const float2 previous_position = float2(pixel) + g_motion.Load(int3(pixel, 0)).xy + 0.5;
  if (g_history_valid != 0u && all(isfinite(previous_position)) && all(previous_position >= 0.0)
      && all(previous_position < float2(g_dimensions))) {
    const uint2 previous_pixel = uint2(previous_position);
    uint4 rows[RTXDI_SURFACE_SCENE_ROW_COUNT];
    uint4 previous_rows[RTXDI_SURFACE_SCENE_ROW_COUNT];
    [unroll]
    for (uint row = 0u; row < 8u; ++row) {
      rows[row] = g_scene.Load(int3(row, 0, 0));
      previous_rows[row] = g_prev_scene.Load(int3(row, 0, 0));
    }
    [unroll]
    for (uint row = RTXDI_SURFACE_INVERSE_VIEW_PROJECTION_ROW; row < RTXDI_SURFACE_INVERSE_VIEW_PROJECTION_ROW + 4u; ++row) {
      rows[row] = g_scene.Load(int3(row, 0, 0));
      previous_rows[row] = g_prev_scene.Load(int3(row, 0, 0));
    }
    rows[RTXDI_SURFACE_RESOLUTION_SCALING_ROW] = g_scene.Load(int3(RTXDI_SURFACE_RESOLUTION_SCALING_ROW, 0, 0));
    previous_rows[RTXDI_SURFACE_RESOLUTION_SCALING_ROW] = g_prev_scene.Load(int3(RTXDI_SURFACE_RESOLUTION_SCALING_ROW, 0, 0));
    const RtxdiSurfaceResult surface = RtxdiReconstructSurface(g_depth.Load(int3(pixel, 0)), g_normal.Load(int3(pixel, 0)),
                                                                pixel, g_dimensions, rows);
    const RtxdiSurfaceResult previous = RtxdiReconstructSurface(g_prev_depth.Load(int3(previous_pixel, 0)),
                                                                 g_prev_normal.Load(int3(previous_pixel, 0)),
                                                                 previous_pixel, g_dimensions, previous_rows);
    const float4x4 previous_view = float4x4(asfloat(previous_rows[RTXDI_SURFACE_VIEW_ROW + 0u]),
                                            asfloat(previous_rows[RTXDI_SURFACE_VIEW_ROW + 1u]),
                                            asfloat(previous_rows[RTXDI_SURFACE_VIEW_ROW + 2u]),
                                            asfloat(previous_rows[RTXDI_SURFACE_VIEW_ROW + 3u]));
    const float expected_depth = -mul(previous_view, float4(surface.world_position, 1.0)).z;
    const float2 stored = g_prev_history.Load(int3(previous_pixel, 0));
    if ((surface.flags & RTXDI_SURFACE_FLAG_VALID) != 0u && (previous.flags & RTXDI_SURFACE_FLAG_VALID) != 0u
        && (surface.flags & RTXDI_SURFACE_FLAG_CHARACTER) == (previous.flags & RTXDI_SURFACE_FLAG_CHARACTER)
        && isfinite(expected_depth) && expected_depth > 0.0
        && RTXDI_IsValidNeighbor(surface.shading_normal, previous.shading_normal, expected_depth,
                                 previous.linear_view_depth, kSunHistoryNormalThreshold, kSunHistoryDepthThreshold)
        && all(isfinite(stored)) && stored.y >= 1.0) {
      history_length = stored.y;
      history = clamp(stored.x, neighborhood_min, neighborhood_max);
    }
  }
  const float accumulated_length = min(history_length + 1.0, 32.0);
  const float filtered = lerp(history, current, max(1.0 / accumulated_length, 1.0 / float(g_accumulation_frames)));
  g_history[pixel] = float2(filtered, accumulated_length);
  // Without usable history the raw word stays, so a reset pixel shows exactly the unfiltered result.
  if (history_length == 0.0) return;
  g_sun_visibility[pixel] = RTXDI_SUN_STATUS_FILTERED
                            | (g_sun_visibility[pixel] & (RTXDI_SUN_LAYER_MASK << RTXDI_SUN_LAYER_SHIFT))
                            | ((uint)round(saturate(1.0 - filtered) * 255.0) << RTXDI_SUN_WIND_OCCLUSION_SHIFT);
}
