#pragma once

#include "../rtxdi/rtxdi_diag_layout.h"

StructuredBuffer<RtxdiLightTableEntry> g_rtxdi_light_table : register(t46);
StructuredBuffer<RtxdiLightTableEntry> g_rtxdi_prev_light_table : register(t47);
StructuredBuffer<uint> g_rtxdi_previous_to_current : register(t48);
StructuredBuffer<uint> g_rtxdi_current_to_previous : register(t49);

static uint g_rtxdi_surface_diag = RTXDI_DIAG_SURFACE_REASON_NONE;

struct RAB_Surface {
  float3 world_position;
  float3 shading_normal;
  float3 view_direction;
  float linear_view_depth;
  float material_glossiness;
  uint flags;
  uint material;
  uint2 pixel;
  uint tile_cell;
  uint tile_valid;
  uint previous;
  uint has_local_specular;
};

struct RAB_LightInfo {
  LightParam lamp;
  uint light_index;
  uint type;
  uint valid;
};

struct RAB_LightSample {
  float3 position;
  float3 shadow_target;
  float3 direction;
  float distance;
  float3 radiance;
  uint light_index;
  uint type;
  float specular_intensity;
  float specular_glossiness;
  float translucency;
  uint valid;
};

RAB_Surface RAB_EmptySurface() { return (RAB_Surface)0; }
bool RAB_IsSurfaceValid(RAB_Surface surface) { return (surface.flags & RTXDI_SURFACE_FLAG_VALID) != 0u; }
float3 RAB_GetSurfaceWorldPos(RAB_Surface surface) { return surface.world_position; }
float3 RAB_GetSurfaceNormal(RAB_Surface surface) { return surface.shading_normal; }
float RAB_GetSurfaceLinearDepth(RAB_Surface surface) { return surface.linear_view_depth; }
uint RAB_GetMaterial(RAB_Surface surface) { return surface.material; }
bool RAB_AreMaterialsSimilar(uint a, uint b) { return a == b; }
RAB_LightInfo RAB_EmptyLightInfo() { return (RAB_LightInfo)0; }
RAB_LightSample RAB_EmptyLightSample() { return (RAB_LightSample)0; }

bool RtxdiGetLightTileCell(uint2 pixel, float linear_depth, bool previous, out uint cell) {
  cell = 0u;
  const uint tile_count = (previous ? g_prev_tile_count : g_tile_count);
  if (tile_count == 0u || !isfinite(linear_depth) || linear_depth <= 0.0) return false;
  const uint2 tile_size_inv_bits = (previous ? g_rtxdi_prev_scene.Load(int3(RTXDI_SURFACE_LIGHT_TILE_SIZE_ROW, 0, 0))
                                             : g_rtxdi_scene.Load(int3(RTXDI_SURFACE_LIGHT_TILE_SIZE_ROW, 0, 0))).xy;
  const float2 tile_size_inv = asfloat(tile_size_inv_bits);
  if (!all(isfinite(tile_size_inv)) || any(tile_size_inv <= 0.0)) return false;
  const uint2 tile = uint2(tile_size_inv * (float2(pixel) + 0.5));
  if (tile.x >= 32u || tile.y >= 18u) return false;
  const uint base = tile.x + (tile.y << 5u);
  if (base >= tile_count) return false;
  const float tile_depth_inv = (previous ? g_rtxdi_prev_tiles[base].tileDepthInv : g_rtxdi_tiles[base].tileDepthInv);
  if (!isfinite(tile_depth_inv) || tile_depth_inv < 0.0) return false;
  const uint slice = (uint)clamp(tile_depth_inv * linear_depth, 0.0, 7.0);
  cell = min(4607u, slice * 576u + base);
  return cell < tile_count;
}

bool RtxdiTileContainsLight(uint tile_cell, uint light_index, uint type, bool previous) {
  LightIndexData tile;
  if (previous) tile = g_rtxdi_prev_tiles[tile_cell];
  else tile = g_rtxdi_tiles[tile_cell];
  if (type == RTGI_LAMP_TYPE_POINT) {
    const uint count = min(tile.pointLightCount, 63u);
    [loop]
    for (uint i = 0u; i < count; ++i) {
      if (tile.pointLightIndices[i] >= 0 && uint(tile.pointLightIndices[i]) == light_index) return true;
    }
  } else if (type == RTGI_LAMP_TYPE_SPOT) {
    const uint count = min(tile.spotLightCount, 63u);
    [loop]
    for (uint i = 0u; i < count; ++i) {
      if (tile.spotLightIndices[i] >= 0 && uint(tile.spotLightIndices[i]) == light_index) return true;
    }
  }
  return false;
}

RAB_Surface RtxdiMakeRABSurface(RtxdiSurfaceResult surface, uint2 pixel, bool previous) {
  RAB_Surface result = RAB_EmptySurface();
  if ((surface.flags & RTXDI_SURFACE_FLAG_VALID) == 0u) return result;
  const bool character = (surface.flags & RTXDI_SURFACE_FLAG_CHARACTER) != 0u;
  result.world_position = surface.world_position;
  result.shading_normal = surface.shading_normal;
  result.view_direction = surface.view_direction;
  result.linear_view_depth = surface.linear_view_depth;
  result.flags = surface.flags;
  result.material = character ? 0x80000000u : 0u;
  result.pixel = pixel;
  result.previous = previous ? 1u : 0u;
  result.tile_valid = RtxdiGetLightTileCell(pixel, surface.linear_view_depth, previous, result.tile_cell) ? 1u : 0u;
  if (character) return result;

  if ((previous ? g_prev_deferred_count : g_deferred_count) == 0u) {
    if (!previous) g_rtxdi_surface_diag = RTXDI_DIAG_SURFACE_REASON_NO_DEFERRED;
    return RAB_EmptySurface();
  }
  uint material_count, material_stride;
  if (previous) g_rtxdi_prev_deferred.GetDimensions(material_count, material_stride);
  else g_rtxdi_deferred.GetDimensions(material_count, material_stride);
  const uint material_index = (previous ? g_rtxdi_prev_mrt2.Load(int3(pixel, 0)).x : g_rtxdi_mrt2.Load(int3(pixel, 0)).x);
  if (material_index >= (previous ? g_prev_deferred_count : g_deferred_count) || material_index >= material_count) {
    if (!previous) g_rtxdi_surface_diag = RTXDI_DIAG_SURFACE_REASON_MATERIAL_OOB;
    return RAB_EmptySurface();
  }
  GameDeferredParam deferred;
  if (previous) deferred = g_rtxdi_prev_deferred[material_index];
  else deferred = g_rtxdi_deferred[material_index];
  const uint packed_gloss = (previous ? g_rtxdi_prev_mrt2.Load(int3(pixel, 0)).y : g_rtxdi_mrt2.Load(int3(pixel, 0)).y);
  const float gloss_x = (packed_gloss & 31u) / 31.0;
  const float gloss_y = ((packed_gloss >> 5u) & 31u) / 31.0;
  const float material_glossiness = lerp(lerp(deferred.mid.x, deferred.mid.y, gloss_x), deferred.mid.z, gloss_y);
  if (!all(isfinite(deferred.mid.xyz)) || !isfinite(material_glossiness)) {
    if (!previous) g_rtxdi_surface_diag = RTXDI_DIAG_SURFACE_REASON_MATERIAL_NONFINITE;
    return RAB_EmptySurface();
  }
  result.material_glossiness = material_glossiness;
  result.material = material_index;
  result.has_local_specular = (asuint(deferred.tail[3].z) & 2u) != 0u ? 1u : 0u;
  return result;
}

int2 RAB_ClampSamplePositionIntoView(int2 pixel, bool previous) {
  if (previous && g_history_valid == 0u) return int2(-1, -1);
  return clamp(pixel, int2(0, 0), int2(g_dimensions) - 1);
}

RAB_Surface RAB_GetGBufferSurface(int2 pixel, bool previous) {
  if ((previous && g_history_valid == 0u) || any(pixel < 0) || any(pixel >= int2(g_dimensions))) return RAB_EmptySurface();
  const uint2 dimensions = g_dimensions;
  uint4 scene_rows[RTXDI_SURFACE_SCENE_ROW_COUNT] = (uint4[RTXDI_SURFACE_SCENE_ROW_COUNT])0;
  [unroll]
  for (uint row = 0u; row < 8u; ++row) scene_rows[row] = (previous ? g_rtxdi_prev_scene.Load(int3(row, 0, 0)) : g_rtxdi_scene.Load(int3(row, 0, 0)));
  [unroll]
  for (uint row = 0u; row < 4u; ++row) {
    scene_rows[RTXDI_SURFACE_INVERSE_VIEW_PROJECTION_ROW + row] =
        (previous ? g_rtxdi_prev_scene.Load(int3(RTXDI_SURFACE_INVERSE_VIEW_PROJECTION_ROW + row, 0, 0)) : g_rtxdi_scene.Load(int3(RTXDI_SURFACE_INVERSE_VIEW_PROJECTION_ROW + row, 0, 0)));
  }
  scene_rows[RTXDI_SURFACE_SUN_RESERVED_ROW] = (previous ? g_rtxdi_prev_scene.Load(int3(RTXDI_SURFACE_SUN_RESERVED_ROW, 0, 0)) : g_rtxdi_scene.Load(int3(RTXDI_SURFACE_SUN_RESERVED_ROW, 0, 0)));
  scene_rows[RTXDI_SURFACE_RESOLUTION_SCALING_ROW] = (previous ? g_rtxdi_prev_scene.Load(int3(RTXDI_SURFACE_RESOLUTION_SCALING_ROW, 0, 0)) : g_rtxdi_scene.Load(int3(RTXDI_SURFACE_RESOLUTION_SCALING_ROW, 0, 0)));
  const uint2 pixel_u = uint2(pixel);
  const RtxdiSurfaceResult reconstructed = RtxdiReconstructSurface(
      (previous ? g_rtxdi_prev_depth.Load(int3(pixel_u, 0)) : g_rtxdi_depth.Load(int3(pixel_u, 0))),
      (previous ? g_rtxdi_prev_normal.Load(int3(pixel_u, 0)) : g_rtxdi_normal.Load(int3(pixel_u, 0))), pixel_u, dimensions, scene_rows);
  if (!previous) g_rtxdi_surface_diag = reconstructed.rejection;
  return RtxdiMakeRABSurface(reconstructed, pixel_u, previous);
}

RAB_LightInfo RAB_LoadLightInfo(uint light_index, bool previous) {
  RAB_LightInfo info = RAB_EmptyLightInfo();
  uint light_count;
  uint stride;
  if (previous) g_rtxdi_prev_lights.GetDimensions(light_count, stride);
  else g_rtxdi_lights.GetDimensions(light_count, stride);
  uint type_count;
  if (previous) g_rtxdi_prev_light_table.GetDimensions(type_count, stride);
  else g_rtxdi_light_table.GetDimensions(type_count, stride);
#if defined(RTXDI_DIAG_INITIAL_PASS)
  RtxdiDiagAdd(RTXDI_DIAG_CAND_TRIED, previous ? 0u : 1u);
#endif
  if (light_index >= (previous ? g_prev_light_count : g_light_count) || light_index >= light_count || light_index >= type_count) return info;

  const uint type = (previous ? g_rtxdi_prev_light_table[light_index].type : g_rtxdi_light_table[light_index].type);
#if defined(RTXDI_DIAG_INITIAL_PASS)
  RtxdiDiagAdd(RTXDI_DIAG_CAND_TYPE_NONE, !previous && type == 0u);
  RtxdiDiagAdd(RTXDI_DIAG_CAND_TYPE_BOTH, !previous && type == (RTGI_LAMP_TYPE_POINT | RTGI_LAMP_TYPE_SPOT));
#endif
  if (type != RTGI_LAMP_TYPE_POINT && type != RTGI_LAMP_TYPE_SPOT) return info;
  LightParam lamp;
  if (previous) lamp = g_rtxdi_prev_lights[light_index];
  else lamp = g_rtxdi_lights[light_index];
  if (!all(isfinite(float4(lamp.pos, lamp.radius))) || !all(isfinite(float4(lamp.color, lamp.radiusInv)))
      || !all(isfinite(float4(lamp.vec, lamp.spotAngleInv))) || !isfinite(lamp.attenuation)
      || !isfinite(lamp.attenuationAngle) || !isfinite(lamp.specularIntensity) || !isfinite(lamp.specularGlossiness)
      || !isfinite(lamp.translucency) || lamp.radius <= 0.0 || lamp.radiusInv <= 0.0) {
#if defined(RTXDI_DIAG_INITIAL_PASS)
    RtxdiDiagAdd(RTXDI_DIAG_CAND_LAMP_INVALID, previous ? 0u : 1u);
#endif
    return info;
  }
  if (type == RTGI_LAMP_TYPE_SPOT && (lamp.spotAngleInv <= 0.0 || lamp.attenuationAngle < 0.0
                                      || dot(lamp.vec, lamp.vec) <= 1e-12)) {
#if defined(RTXDI_DIAG_INITIAL_PASS)
    RtxdiDiagAdd(RTXDI_DIAG_CAND_LAMP_INVALID, previous ? 0u : 1u);
#endif
    return info;
  }
  info.lamp = lamp;
  info.light_index = light_index;
  info.type = type;
  info.valid = 1u;
#if defined(RTXDI_DIAG_INITIAL_PASS)
  RtxdiDiagAdd(RTXDI_DIAG_CAND_INFO_VALID, previous ? 0u : 1u);
#endif
  return info;
}

int RAB_TranslateLightIndex(uint light_index, bool previous) {
#if defined(RTXDI_TEMPORAL_DI_PASS)
  if ((g_frame_flags & RTXDI_FRAME_FLAG_STABLE_LIGHT_REMAP) != 0u) {
    if (g_history_valid == 0u) return -1;
    uint map_count;
    uint map_stride;
    if (previous) g_rtxdi_current_to_previous.GetDimensions(map_count, map_stride);
    else g_rtxdi_previous_to_current.GetDimensions(map_count, map_stride);
    const uint source_count = previous ? g_light_count : g_prev_light_count;
    if (light_index >= source_count || light_index >= map_count) return -1;
    const uint mapped_index = previous ? g_rtxdi_current_to_previous[light_index] : g_rtxdi_previous_to_current[light_index];
    const uint destination_count = previous ? g_prev_light_count : g_light_count;
    if (mapped_index == RTXDI_LIGHT_TABLE_INVALID_INDEX || mapped_index >= destination_count) return -1;
    const RAB_LightInfo source_light = RAB_LoadLightInfo(light_index, !previous);
    const RAB_LightInfo destination_light = RAB_LoadLightInfo(mapped_index, previous);
    if (source_light.valid == 0u || destination_light.valid == 0u || source_light.type != destination_light.type) return -1;
    return int(mapped_index);
  }
#endif
  const RAB_LightInfo source_light = RAB_LoadLightInfo(light_index, previous);
  const RAB_LightInfo destination_light = RAB_LoadLightInfo(light_index, !previous);
  if (source_light.valid == 0u || destination_light.valid == 0u || source_light.type != destination_light.type) return -1;
  const LightParam a = source_light.lamp;
  const LightParam b = destination_light.lamp;
  if (any(asuint(float4(a.pos, a.radius)) != asuint(float4(b.pos, b.radius)))
      || any(asuint(float4(a.color, a.radiusInv)) != asuint(float4(b.color, b.radiusInv)))
      || any(asuint(float4(a.charaColor, a.attenuation)) != asuint(float4(b.charaColor, b.attenuation)))
      || any(asuint(float4(a.vec, a.spotAngleInv)) != asuint(float4(b.vec, b.spotAngleInv)))
      || any(asuint(float4(a.attenuationAngle, a.specularIntensity, a.specularGlossiness, a.scatterAnisotropy))
             != asuint(float4(b.attenuationAngle, b.specularIntensity, b.specularGlossiness, b.scatterAnisotropy)))
      || any(asuint(float4(a.scatterColor, a.scatterDensity)) != asuint(float4(b.scatterColor, b.scatterDensity)))
      || asuint(a.translucency) != asuint(b.translucency) || a.shadowmapIndex != b.shadowmapIndex
      || asuint(a.scatterOffset) != asuint(b.scatterOffset) || asuint(a.userParam) != asuint(b.userParam)) return -1;
  return int(light_index);
}

RAB_LightSample RAB_SamplePolymorphicLight(RAB_LightInfo info, RAB_Surface surface, float2 uv) {
  RAB_LightSample sample = RAB_EmptyLightSample();
  if (info.valid == 0u || !RAB_IsSurfaceValid(surface)) return sample;
  const float3 delta = info.lamp.pos - surface.world_position;
  const float distance_squared = dot(delta, delta);
  if (!isfinite(distance_squared) || distance_squared <= 1e-12) return sample;
  const float distance = sqrt(distance_squared);
  const float3 direction = delta / distance;
  float spot = 1.0;
  if (info.type == RTGI_LAMP_TYPE_SPOT) spot = LampSpotCone(info.lamp, direction);
  const float attenuation = LampDistanceAttenuation(info.lamp, delta);
  const float cosine = max(0.0, dot(surface.shading_normal, direction));
  const bool character = (surface.flags & RTXDI_SURFACE_FLAG_CHARACTER) != 0u;
  const float3 lamp_color = character ? info.lamp.charaColor : info.lamp.color;
  const float3 radiance = max(lamp_color, 0.0) * (attenuation * spot);
  if (!all(isfinite(lamp_color)) || !isfinite(attenuation) || !isfinite(spot) || !isfinite(cosine)
      || !all(isfinite(radiance))) return sample;
  sample.position = info.lamp.pos;
  sample.shadow_target = info.lamp.pos;
  if (g_local_light_size > 0.0) {
    const float3 up = abs(direction.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
    const float3 T = normalize(cross(up, direction));
    const float3 B = cross(direction, T);
    const float r = g_local_light_size * sqrt(uv.x);
    const float phi = 6.2831853 * uv.y;
    sample.shadow_target += (T * cos(phi) + B * sin(phi)) * r;
  }
  sample.direction = direction;
  sample.distance = distance;
  sample.radiance = radiance;
  sample.light_index = info.light_index;
  sample.type = info.type;
  sample.specular_intensity = max(info.lamp.specularIntensity, 0.0);
  sample.specular_glossiness = info.lamp.specularGlossiness;
  sample.translucency = max(info.lamp.translucency, 0.0);
  sample.valid = 1u;
  return sample;
}

float RAB_GetLightSampleTargetPdfForSurface(RAB_LightSample sample, RAB_Surface surface) {
#if defined(RTXDI_DIAG_INITIAL_PASS)
  if ((g_frame_flags & RTXDI_FRAME_FLAG_DIAGNOSTICS) != 0u) {
    const bool diag_sample = sample.valid != 0u && RAB_IsSurfaceValid(surface);
    const bool diag_tile = diag_sample && surface.tile_valid != 0u;
    RtxdiDiagAdd(RTXDI_DIAG_CAND_SAMPLE_VALID, diag_sample);
    RtxdiDiagAdd(RTXDI_DIAG_CAND_TILE_VALID, diag_tile);
    RtxdiDiagAdd(RTXDI_DIAG_CAND_IN_TILE,
                 diag_tile && RtxdiTileContainsLight(surface.tile_cell, sample.light_index, sample.type, surface.previous != 0u));
    RtxdiDiagAdd(RTXDI_DIAG_CAND_RADIANCE_ZERO,
                 diag_tile && RtxdiTileContainsLight(surface.tile_cell, sample.light_index, sample.type, surface.previous != 0u)
                         && all(sample.radiance <= 0.0) ? 1u : 0u);
  }
#endif
  if (sample.valid == 0u || !RAB_IsSurfaceValid(surface) || surface.tile_valid == 0u
      || !RtxdiTileContainsLight(surface.tile_cell, sample.light_index, sample.type, surface.previous != 0u)) return 0.0;
  const float lit = max(max(0.0, dot(surface.shading_normal, sample.direction)), sample.translucency);
  const float3 diffuse = sample.radiance * lit;
  const float3 half_sum = sample.direction + surface.view_direction;
  const float half_length_sq = dot(half_sum, half_sum);
  const float specular_cosine = (half_length_sq > 1e-12 ? saturate(dot(surface.shading_normal, half_sum * rsqrt(half_length_sq))) : 0.0);
  const float specular_exponent = max(0.001, sample.specular_glossiness * surface.material_glossiness);
  const float specular_lobe = pow(specular_cosine, specular_exponent);
  const float3 specular = (surface.has_local_specular != 0u)
                              ? sample.radiance * (sample.specular_intensity * lit * specular_lobe)
                              : 0.0;
  if (!all(isfinite(diffuse)) || !all(isfinite(specular))) return 0.0;
  float target = dot(diffuse + specular, float3(0.2126, 0.7152, 0.0722));
  if (surface.has_local_specular != 0u && lit > 0.0) {
    // Preserve proposal support for grazing GGX lobes; this floor is never emitted as raw energy.
    target = max(target, dot(sample.radiance, float3(0.2126, 0.7152, 0.0722)) * sample.specular_intensity * lit * 1e-6);
  }
  const float target_pdf = (isfinite(target) ? max(target, 0.0) : 0.0);
#if defined(RTXDI_DIAG_INITIAL_PASS)
  if ((g_frame_flags & RTXDI_FRAME_FLAG_DIAGNOSTICS) != 0u) RtxdiDiagAdd(RTXDI_DIAG_CAND_TARGET_NONZERO, target_pdf > 0.0);
#endif
  return target_pdf;
}

float RAB_LightSampleSolidAnglePdf(RAB_LightSample sample) { return 0.0; }
bool RAB_IsAnalyticLightSample(RAB_LightSample sample) { return true; }
void RAB_GetLightDirDistance(RAB_Surface surface, RAB_LightSample sample, out float3 direction, out float distance) {
  direction = sample.direction;
  distance = sample.distance;
}
float RAB_SurfaceEvaluateBrdfPdf(RAB_Surface surface, float3 direction) { return 0.0; }
bool RAB_SurfaceImportanceSampleBrdf(RAB_Surface surface, inout RTXDI_RandomSamplerState rng, out float3 direction) {
  direction = 0.0;
  return false;
}

bool RAB_TraceRayForLocalLight(float3 origin, float3 direction, float tmin, float tmax, out uint index, out float2 uv) {
  index = 0u;
  uv = 0.0;
  return false;
}
float RAB_EvaluateLocalLightSourcePdf(uint index) { return 0.0; }
float RAB_EvaluateEnvironmentMapSamplingPdf(float3 direction) { return 0.0; }
float2 RAB_GetEnvironmentMapRandXYFromDir(float3 direction) { return 0.0; }
bool RAB_GetConservativeVisibility(RAB_Surface surface, RAB_LightSample sample) { return false; }

bool RtxdiDiCoverageEnd(float3 origin, float3 direction, float max_distance, float3 region_min, float region_size,
                        out float coverage_end) {
  coverage_end = 0.0;
  if (any(origin < region_min) || any(origin > region_min + region_size)) return false;
  float exit_distance = max_distance;
  for (uint axis = 0u; axis < 3u; ++axis) {
    if (abs(direction[axis]) <= 1e-8) continue;
    const float boundary = (direction[axis] > 0.0 ? region_min[axis] + region_size : region_min[axis]);
    exit_distance = min(exit_distance, (boundary - origin[axis]) / direction[axis]);
  }
  coverage_end = max(exit_distance, 0.0);
  return isfinite(coverage_end) && coverage_end > 0.0;
}

RtxdiDiRawResult RtxdiTraceLocalLight(RAB_Surface surface, RAB_LightSample sample, uint scene_valid, uint coverage_valid,
                                     float environment_bias, float character_bias, float endpoint_clearance,
                                     float3 region_min, float region_size, float inverse_pdf) {
  RtxdiDiRawResult result = (RtxdiDiRawResult)0;
  result.selected_light_index = 0xffffffffu;
  result.status = RTXDI_DI_STATUS_UNKNOWN;
  result.reason = RTXDI_DI_REASON_INVALID_SAMPLE;
  if (sample.valid != 0u) {
    result.selected_light_index = sample.light_index;
    result.selected_light_type = sample.type;
    if (isfinite(inverse_pdf) && inverse_pdf > 0.0) result.inverse_pdf = inverse_pdf;
  }
  if (!isfinite(inverse_pdf) || inverse_pdf <= 0.0) return result;
  if (scene_valid == 0u) {
    result.reason = RTXDI_DI_REASON_INVALID_SCENE;
    return result;
  }
  const float3 delta = sample.position - surface.world_position;
  const float distance_squared = dot(delta, delta);
  if (sample.valid == 0u || !all(isfinite(delta)) || !isfinite(distance_squared) || distance_squared <= 1e-12
      || !isfinite(endpoint_clearance) || endpoint_clearance < 0.0 || !isfinite(region_size) || region_size <= 0.0) return result;
  const float distance = sqrt(distance_squared);
  const float3 direction = delta / distance;
  // Optional local light clamp: scales the published weight only, never the stored reservoir.
  if (g_local_light_clamp > 0.0) {
    const float lum = dot(sample.radiance * max(max(0.0, dot(surface.shading_normal, direction)), sample.translucency) * inverse_pdf, float3(0.2126, 0.7152, 0.0722));
    if (isfinite(lum) && lum > g_local_light_clamp) {
      inverse_pdf *= g_local_light_clamp / lum;
      result.inverse_pdf = inverse_pdf;
    }
  }
  const float3 shadow_delta = sample.shadow_target - surface.world_position;
  const float shadow_distance_squared = dot(shadow_delta, shadow_delta);
  if (!all(isfinite(shadow_delta)) || !isfinite(shadow_distance_squared) || shadow_distance_squared <= 1e-12) return result;
  float shadow_distance = sqrt(shadow_distance_squared);
  float3 shadow_direction = shadow_delta / shadow_distance;
  const bool character = (surface.flags & RTXDI_SURFACE_FLAG_CHARACTER) != 0u;
  const float bias = character ? character_bias : environment_bias;
  if (!isfinite(bias)) return result;
  const float bias_sign = (dot(surface.shading_normal, shadow_direction) >= 0.0 ? 1.0 : -1.0);
  float3 origin = surface.world_position + surface.shading_normal * (bias * bias_sign);
  if (character && g_character_self_softening > 0.0) {
    const float3 outward = surface.shading_normal * bias_sign;
    float3 lateral = (sample.shadow_target - sample.position) * min(g_character_self_softening / shadow_distance, 1.0);
    lateral -= outward * min(dot(lateral, outward), 0.0);
    origin += lateral;
    const float3 shifted_delta = sample.shadow_target - (surface.world_position + lateral);
    const float shifted_distance_squared = dot(shifted_delta, shifted_delta);
    if (!all(isfinite(shifted_delta)) || !isfinite(shifted_distance_squared) || shifted_distance_squared <= 1e-12) return result;
    shadow_distance = sqrt(shifted_distance_squared);
    shadow_direction = shifted_delta / shadow_distance;
  }
  if (shadow_distance <= endpoint_clearance) return RtxdiDiMakeVisibleRawResult(sample.radiance * max(max(0.0, dot(surface.shading_normal, direction)), sample.translucency) * inverse_pdf, distance, character ? 1u : 0u, sample.light_index, sample.type, inverse_pdf);
  const float endpoint = max(shadow_distance - endpoint_clearance, 0.0);
  float coverage_end = 0.0;
  bool complete_segment = false;
  if (coverage_valid != 0u && !RtxdiDiCoverageEnd(origin, shadow_direction, endpoint, region_min, region_size, coverage_end)) {
    result.reason = RTXDI_DI_REASON_COVERAGE_MISS;
    return result;
  }
  if (coverage_valid == 0u) coverage_end = endpoint;
  else complete_segment = coverage_end >= endpoint;
  WorldCameraView view;
  view.hide = false;
  view.floor_value = 0.0;
  view.skip_flags = 0u;
  view.require_flags = 0u;
  g_rt12_dynamic_mask = RT12_MASK_DYNAMIC;
  WorldTraceCounters counters;
  const WorldTraceHit hit = TraceWorldRayDxr(origin, shadow_direction, 0.0, coverage_end, true, view, counters);
  if (counters.invalid_refs != 0u) result.reason = RTXDI_DI_REASON_INVALID_ALPHA;
  else if (counters.hits != 0u) {
    result.status = RTXDI_DI_STATUS_BLOCKED;
    result.distance = hit.t;
    result.reason = RTXDI_DI_REASON_NONE;
  } else if (complete_segment) {
    // This is pre-material local diffuse; later shading applies base color and dynamic intensity once.
    const float lit = max(max(0.0, dot(surface.shading_normal, direction)), sample.translucency);
    const float3 raw_diffuse = sample.radiance * lit * inverse_pdf;
    result = RtxdiDiMakeVisibleRawResult(raw_diffuse, distance, character ? 1u : 0u, sample.light_index,
                                        sample.type, inverse_pdf);
  } else result.reason = RTXDI_DI_REASON_TRUNCATED;
  return result;
}

