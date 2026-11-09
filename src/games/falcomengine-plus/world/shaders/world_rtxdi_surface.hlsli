#pragma once

#include "../../include/mrt_normal.hlsli"
#include "../rtxdi/rtxdi_surface_layout.h"

struct RtxdiSurfaceResult {
  float3 world_position;
  float3 shading_normal;
  float3 view_direction;
  float linear_view_depth;
  uint flags;
  uint rejection;
};

RtxdiSurfaceResult RtxdiRejectSurface(uint reason) {
  RtxdiSurfaceResult result = (RtxdiSurfaceResult)0;
  result.rejection = reason;
  return result;
}

RtxdiSurfaceResult RtxdiReconstructSurface(float raw_depth, uint4 mrt0, uint2 pixel, uint2 internal_size,
                                          uint4 scene_rows[RTXDI_SURFACE_SCENE_ROW_COUNT]) {
  if (any(internal_size == 0u)) return RtxdiRejectSurface(RTXDI_SURFACE_REJECT_DIMENSIONS);
  if (any(pixel >= internal_size)) return RtxdiRejectSurface(RTXDI_SURFACE_REJECT_PIXEL);

  const float2 resolution_scaling = asfloat(scene_rows[RTXDI_SURFACE_RESOLUTION_SCALING_ROW].xy);
  if (!all(isfinite(resolution_scaling)) || any(resolution_scaling != 1.0)) {
    return RtxdiRejectSurface(RTXDI_SURFACE_REJECT_SCALING);
  }
  if (!isfinite(raw_depth) || raw_depth <= 0.0 || raw_depth > 1.0) {
    return RtxdiRejectSurface(RTXDI_SURFACE_REJECT_DEPTH);
  }

  const float3 shading_normal = DecodeFalcomMrtNormalAsGame(mrt0.xy);
  if (!all(isfinite(shading_normal)) || dot(shading_normal, shading_normal) < 0.5) {
    return RtxdiRejectSurface(RTXDI_SURFACE_REJECT_NORMAL);
  }

  const float2 uv = (float2(pixel) + 0.5) / float2(internal_size);
  const float4 clip_position = float4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, raw_depth, 1.0);
  const float4x4 inverse_view_projection = float4x4(
      asfloat(scene_rows[RTXDI_SURFACE_INVERSE_VIEW_PROJECTION_ROW + 0u]),
      asfloat(scene_rows[RTXDI_SURFACE_INVERSE_VIEW_PROJECTION_ROW + 1u]),
      asfloat(scene_rows[RTXDI_SURFACE_INVERSE_VIEW_PROJECTION_ROW + 2u]),
      asfloat(scene_rows[RTXDI_SURFACE_INVERSE_VIEW_PROJECTION_ROW + 3u]));
  const float4 world_h = mul(clip_position, inverse_view_projection);
  if (!isfinite(world_h.w) || abs(world_h.w) <= 1e-8) {
    return RtxdiRejectSurface(RTXDI_SURFACE_REJECT_HOMOGENEOUS_W);
  }
  if (!all(isfinite(world_h.xyz))) return RtxdiRejectSurface(RTXDI_SURFACE_REJECT_POSITION);
  const float3 world_position = world_h.xyz / world_h.w;
  if (!all(isfinite(world_position))) return RtxdiRejectSurface(RTXDI_SURFACE_REJECT_POSITION);

  const float4x4 view = float4x4(asfloat(scene_rows[RTXDI_SURFACE_VIEW_ROW + 0u]),
                                asfloat(scene_rows[RTXDI_SURFACE_VIEW_ROW + 1u]),
                                asfloat(scene_rows[RTXDI_SURFACE_VIEW_ROW + 2u]),
                                asfloat(scene_rows[RTXDI_SURFACE_VIEW_ROW + 3u]));
  const float3 camera = asfloat(scene_rows[RTXDI_SURFACE_INVERSE_VIEW_ROW + 3u]).xyz;
  const float3 to_camera = camera - world_position;
  const float to_camera_length_squared = dot(to_camera, to_camera);
  if (!all(isfinite(camera)) || !isfinite(to_camera_length_squared) || to_camera_length_squared <= 1e-12) {
    return RtxdiRejectSurface(RTXDI_SURFACE_REJECT_VIEW_DIRECTION);
  }
  const float3 view_direction = to_camera * rsqrt(to_camera_length_squared);
  if (!all(isfinite(view_direction))) return RtxdiRejectSurface(RTXDI_SURFACE_REJECT_VIEW_DIRECTION);

  const float linear_view_depth = -mul(float4(world_position, 1.0), view).z;
  if (!isfinite(linear_view_depth) || linear_view_depth <= 0.0) {
    return RtxdiRejectSurface(RTXDI_SURFACE_REJECT_VIEW_DEPTH);
  }

  RtxdiSurfaceResult result;
  result.world_position = world_position;
  result.shading_normal = shading_normal;
  result.view_direction = view_direction;
  result.linear_view_depth = linear_view_depth;
  result.flags = RTXDI_SURFACE_FLAG_VALID;
  if ((mrt0.w & 8u) == 0u) result.flags |= RTXDI_SURFACE_FLAG_CHARACTER;
  result.rejection = RTXDI_SURFACE_REJECT_NONE;
  return result;
}
