#pragma once
// Shared by the unified PT passes (world/pt/pt_rt12.hpp). The including file defines PT_COMMON_PASS first.
#ifndef PT_COMMON_PASS
#error "define PT_COMMON_PASS before world_pt_common.hlsli"
#endif
// This software contains source code provided by NVIDIA Corporation.
#define RT12_TLAS_REGISTER t29
#define RT12_DYNAMIC_VERTICES_REGISTER t30
#include "world_rt12_trace.hlsli"
#include "world_rt_shadow_rules.hlsli"
#include "world_rtxdi_surface.hlsli"
#include "world_regir_shared.hlsli"
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
StructuredBuffer<RtxdiLightTableEntry> g_pt_light_table : register(t35);
Texture2D<float4> g_pt_color : register(t36);     // G-buffer albedo (transfer slot color)
Texture2D<uint4> g_pt_mrt1 : register(t37);       // .z & 255: specular scale
Texture2D<uint4> g_pt_mrt2 : register(t38);       // .x: deferred material index
StructuredBuffer<GameDeferredParam> g_pt_deferred : register(t39);
Texture2D<uint4> g_pt_noise : register(t40);
StructuredBuffer<PtReGirHeader> g_pt_regir_header : register(t41);
StructuredBuffer<PtReGirCell> g_pt_regir_cells : register(t42);
RWStructuredBuffer<uint64_t> g_pt_material_keys : register(u4);
globallycoherent RWStructuredBuffer<uint> g_pt_material_values : register(u5);
RWTexture2D<float4> g_pt_diffuse : register(u0);
RWStructuredBuffer<uint64_t> g_pt_sharc_hash : register(u1);
RWStructuredBuffer<SharcAccumulationData> g_pt_sharc_accumulation : register(u2);
RWStructuredBuffer<SharcPackedData> g_pt_sharc_resolved : register(u3);
RWStructuredBuffer<uint> g_pt_counters : register(u6);  // GPU diagnostics: 0 exact G-buffer, 1 cache hit, 2 grey miss (PtHitMaterial)
RWTexture2D<float4> g_pt_specular : register(u7);  // traced reflection (rgb), mean hit distance (a)
#ifndef RTXDI_ENABLE_PRESAMPLING
#define RTXDI_ENABLE_PRESAMPLING 0
#endif
#include "../pt/pt_restir_layout.h"
RWStructuredBuffer<RTXDI_PackedPTReservoir> g_pt_reservoirs : register(u8);
#define RTXDI_PT_RESERVOIR_BUFFER g_pt_reservoirs
Buffer<float2> g_pt_neighbors : register(t43);
#define RTXDI_NEIGHBOR_OFFSETS_BUFFER g_pt_neighbors
Texture2D<float> g_pt_prev_depth : register(t44);
Texture2D<uint4> g_pt_prev_normal : register(t45);
Texture2D<uint4> g_pt_prev_scene : register(t46);
Texture2D<float4> g_pt_prev_color : register(t47);
Texture2D<uint4> g_pt_prev_mrt1 : register(t48);
Texture2D<uint4> g_pt_prev_mrt2 : register(t49);
StructuredBuffer<GameDeferredParam> g_pt_prev_deferred : register(t50);
StructuredBuffer<LightParam> g_pt_prev_lights : register(t51);
StructuredBuffer<RtxdiLightTableEntry> g_pt_prev_light_table : register(t52);
Texture2D<float4> g_pt_motion : register(t53);
StructuredBuffer<uint> g_pt_previous_to_current : register(t54);
StructuredBuffer<uint> g_pt_current_to_previous : register(t55);
RWTexture2D<uint> g_pt_sample_id : register(u9);
RWTexture2D<float2> g_pt_duplication_map : register(u10);
RWTexture2D<float> g_pt_smoothed_duplication : register(u11);
RWTexture2D<float4> g_pt_neighbor_selection_gbuffer : register(u12);
RWByteAddressBuffer g_pt_neighbor_selection : register(u13);
Texture2D<float> g_pt_prev_smoothed_duplication : register(t56);
StructuredBuffer<LightIndexData> g_pt_tiles : register(t57);
RWTexture2D<float4> g_pt_debug_value : register(u14);
RWTexture2D<uint> g_pt_debug_status : register(u15);
RWTexture2D<uint4> g_pt_sun_detail : register(u16);
RWTexture2D<float4> g_pt_sun_visibility : register(u17);
RWTexture2D<float4> g_pt_initial_color : register(u18);
#define RTXDI_PT_SAMPLE_ID_TEXTURE g_pt_sample_id
#define RTXDI_PT_DUPLICATION_MAP g_pt_duplication_map
#define RTXDI_PT_SMOOTHED_DUPLICATION_MAP g_pt_smoothed_duplication
#define RTXDI_PT_PREV_SMOOTHED_DUPLICATION_MAP g_pt_prev_smoothed_duplication
#define RTXDI_SPATIAL_NEIGHBOR_SELECTION_BUFFER g_pt_neighbor_selection
ConstantBuffer<PtRestirConstants> g_restir : register(b1);
// g_refl_enabled: 1 while PT reflection is on (formerly the reflection sample count; A14.1).
cbuffer PtParams : register(b0) {  // order = params[] in pt_rt12.hpp (48 dwords)
  uint2 g_size; uint g_frame; uint g_samples; uint g_max_bounces; uint g_roulette; uint g_roulette_min_bounce;
  uint g_lamp_count; uint g_sun_casters_only; uint g_dynamic_on; float g_ray_max; float g_env_bias; float g_char_bias;
  float g_lamp_clearance; float g_sky_intensity; uint g_sharc_enabled;
  uint g_sharc_capacity; uint g_sharc_downscale; uint g_sharc_window; uint g_sharc_stale_frames;
  float g_sharc_scene_scale; float3 g_sharc_camera;
  float3 g_sharc_camera_prev; uint g_sharc_mode;
  uint g_mat_enabled; uint g_mat_capacity; uint g_mat_rate; float g_mat_cell_scale;
  uint g_mat_deferred_count; float g_mat_char_roughness; float g_mat_char_f0; uint g_mat_mode;
  float g_sharc_radiance_scale; uint g_refl_enabled; float g_refl_intensity; float g_refl_roughness_scale;
  float g_refl_sample_clamp; uint g_refl_flags;  // output clamp (0 = off); flags: 1 = cap to one, 2 = reflection uses SHARC, 4 = SHARC footprint test, 8 = water bed test, 32 = sparse reflections enabled
  float g_water_roughness; float g_gi_clamp;  // base roughness of water reflections; GI luminance clamp (0 = off)
  uint g_sharc_firefly_clamp_enabled; float g_sharc_firefly_clamp;  // SHARC update clamp on (1) / off (0); limit as a multiple of the cell mean (OpenRemix sharcFireflyClamp)
  uint g_sparse_levels; uint g_sparse_enabled;
  uint g_regir_enabled;
};
uint PtHash(uint x) { x ^= x >> 16; x *= 0x7feb352du; x ^= x >> 15; x *= 0x846ca68bu; return x ^ (x >> 16); }
bool PtSparseSelected(uint2 pixel) { return uint(asfloat(g_pt_noise.Load(int3((pixel.x + 53u) % 128u, (pixel.y + 91u) % 128u + 128u * (g_frame % 32u), 0)).x) * 256.0) < g_sparse_levels; }
float PtRandom(inout uint s) { s = PtHash(s + 0x9e3779b9u); return float(s >> 8) * (1.0 / 16777216.0); }
static float3 g_pt_sun_color;
static float3 g_pt_sun_dir;
static float3 g_pt_probe[9];
static float4x4 g_pt_view_proj;
static float4x4 g_pt_inv_view_proj;
struct PtMaterial { float3 albedo; float roughness; float3 f0; float spec_scale; bool character; bool reflective; };
// Reconstructs one frame's primary surface without touching the current frame's SHARC/material globals.
bool PtReconstructSurface(Texture2D<float> depth, Texture2D<uint4> normal, Texture2D<uint4> scene, uint2 pixel,
                          out RtxdiSurfaceResult surface) {
  uint4 scene_rows[RTXDI_SURFACE_SCENE_ROW_COUNT];
  [unroll]
  for (uint row = 0u; row < 8u; ++row) {
    scene_rows[row] = scene.Load(int3(row, 0, 0));
  }
  [unroll]
  for (uint row = 0u; row < 4u; ++row) {
    scene_rows[RTXDI_SURFACE_INVERSE_VIEW_PROJECTION_ROW + row] =
        scene.Load(int3(RTXDI_SURFACE_INVERSE_VIEW_PROJECTION_ROW + row, 0, 0));
  }
  scene_rows[RTXDI_SURFACE_SUN_RESERVED_ROW - 1u] = scene.Load(int3(RTXDI_SURFACE_SUN_RESERVED_ROW - 1u, 0, 0));
  scene_rows[RTXDI_SURFACE_SUN_RESERVED_ROW] = scene.Load(int3(RTXDI_SURFACE_SUN_RESERVED_ROW, 0, 0));
  scene_rows[RTXDI_SURFACE_RESOLUTION_SCALING_ROW] = scene.Load(int3(RTXDI_SURFACE_RESOLUTION_SCALING_ROW, 0, 0));
  surface = RtxdiReconstructSurface(depth.Load(int3(pixel, 0)), normal.Load(int3(pixel, 0)), pixel, g_size, scene_rows);
  return (surface.flags & RTXDI_SURFACE_FLAG_VALID) != 0u;
}
float3 PtSceneCamera(Texture2D<uint4> scene) {
  return float3(asfloat(scene.Load(int3(RTXDI_SURFACE_INVERSE_VIEW_ROW + 0u, 0, 0)).w),
                asfloat(scene.Load(int3(RTXDI_SURFACE_INVERSE_VIEW_ROW + 1u, 0, 0)).w),
                asfloat(scene.Load(int3(RTXDI_SURFACE_INVERSE_VIEW_ROW + 2u, 0, 0)).w));
}
// Loads the scene rows, the sun and the ambient probe, and reconstructs the pixel's primary surface.
bool PtLoadSurface(uint2 pixel, out RtxdiSurfaceResult surface) {
  if (!PtReconstructSurface(g_pt_depth, g_pt_normal, g_pt_scene, pixel, surface)) return false;
  g_pt_sun_color = asfloat(g_pt_scene.Load(int3(25, 0, 0)).xyz);
  g_pt_sun_dir = normalize(-asfloat(g_pt_scene.Load(int3(26, 0, 0)).xyz));
  [unroll]
  for (uint i = 0u; i < 9u; ++i) g_pt_probe[i] = asfloat(g_pt_scene.Load(int3(37 + i, 0, 0)).xyz);
g_pt_view_proj = float4x4(asfloat(g_pt_scene.Load(int3(16, 0, 0))), asfloat(g_pt_scene.Load(int3(17, 0, 0))),
                          asfloat(g_pt_scene.Load(int3(18, 0, 0))), asfloat(g_pt_scene.Load(int3(19, 0, 0))));
g_pt_inv_view_proj = float4x4(asfloat(g_pt_scene.Load(int3(20, 0, 0))), asfloat(g_pt_scene.Load(int3(21, 0, 0))),
                              asfloat(g_pt_scene.Load(int3(22, 0, 0))), asfloat(g_pt_scene.Load(int3(23, 0, 0))));
// cb_scene is column-major: mul(M_registers, v), as world_rtxdi_surface.hlsli:41-47.
  return true;
}
float3 PtSky(float3 dir) {
  const float3 sky = g_pt_probe[0] + g_pt_probe[1] * dir.x + g_pt_probe[2] * dir.y + g_pt_probe[3] * dir.z + g_pt_probe[4] * (dir.z * dir.x)
                   + g_pt_probe[5] * (dir.y * dir.z) + g_pt_probe[6] * (dir.y * dir.x) + g_pt_probe[7] * (3 * dir.z * dir.z - 1)
                   + g_pt_probe[8] * (dir.x * dir.x - dir.y * dir.y);
  return g_sky_intensity * max(sky, 0);
}
// Scales c down to luminance limit, keeping hue (OpenRemix fireflies.glsl:6-12). limit 0 = off.
float3 PtClampLuminance(float3 c, float limit) {
  const float lum = dot(c, float3(0.2126, 0.7152, 0.0722));
  return limit > 0.0 && lum > limit ? c * (limit / lum) : c;
}
void PtCount(uint index) { uint previous; InterlockedAdd(g_pt_counters[index], 1u, previous); }
SharcParameters PtSharcParameters() {
  SharcParameters p;
  p.hashGridParameters.cameraPosition = g_sharc_camera;
  p.hashGridParameters.logarithmBase = 2;
  p.hashGridParameters.sceneScale = g_sharc_scene_scale;
  p.hashGridParameters.levelBias = 0;
  p.hashGridData.capacity = g_sharc_capacity;
  p.hashGridData.hashEntriesBuffer = g_pt_sharc_hash;
  p.radianceScale = g_sharc_radiance_scale;
  p.accumulationBuffer = g_pt_sharc_accumulation;
  p.resolvedBuffer = g_pt_sharc_resolved;
  return p;
}
// A cache cell's position: slightly above the surface (OpenRemix pathtrace.comp:1097-1103). Shared by SHARC and the material cache.
float3 PtCachePosition(float3 position, float3 n) { return position + n * (1e-3 * length(position - g_sharc_camera) + 1e-4); }
// Material-cache key normal: the dominant axis with its sign, so shading-normal noise does not split one face. Cell positions keep the real normal.
float3 PtMaterialKeyNormal(float3 n) {
  const float3 a = abs(n);
  const uint axis = (a.x >= a.y && a.x >= a.z) ? 0u : (a.y >= a.z ? 1u : 2u);
  const float3 codes[6] = {float3(1, 1, 1), float3(-1, 1, 1), float3(1, -1, 1), float3(1, 1, -1), float3(-1, -1, 1), float3(-1, 1, -1)};
  return codes[axis * 2u + (n[axis] < 0.0 ? 1u : 0u)];
}
HashGridParameters PtMaterialGrid() {
  HashGridParameters p;
  p.cameraPosition = g_sharc_camera; p.logarithmBase = 2; p.sceneScale = g_mat_cell_scale; p.levelBias = 0;
  return p;
}
HashGridData PtMaterialData() { HashGridData d; d.capacity = g_mat_capacity; d.hashEntriesBuffer = g_pt_material_keys; return d; }
// The G-buffer material of one pixel (lighting :334, :994-1016; world_rtxdi_reconstruct :155, :166, :177).
PtMaterial PtGBufferMaterialFrom(Texture2D<float4> color, Texture2D<uint4> normal, Texture2D<uint4> mrt1,
                                 Texture2D<uint4> mrt2, StructuredBuffer<GameDeferredParam> deferred,
                                 uint deferred_count, uint2 pixel) {
  PtMaterial m;
  m.albedo = saturate(color.Load(int3(pixel, 0)).rgb);
  m.character = (normal.Load(int3(pixel, 0)).w & 8u) == 0u;
  m.roughness = g_mat_char_roughness; m.f0 = g_mat_char_f0; m.spec_scale = 1.0; m.reflective = false;
  if (!m.character) {
    m.spec_scale = (mrt1.Load(int3(pixel, 0)).z & 255u) * (1.0 / 255.0);
    const uint index = mrt2.Load(int3(pixel, 0)).x;
    if (index < deferred_count) {
      const GameDeferredParam d = deferred[index];
      if (all(isfinite(float4(d.head[1].xyz, d.tail[0].z)))) {
        m.roughness = saturate(d.tail[0].z); m.f0 = saturate(d.head[1].xyz);
        m.reflective = (asuint(d.tail[3].z) & 36u) == 4u;
      }
    }
  }
  return m;
}
PtMaterial PtGBufferMaterial(uint2 pixel) {
  return PtGBufferMaterialFrom(g_pt_color, g_pt_normal, g_pt_mrt1, g_pt_mrt2, g_pt_deferred, g_mat_deferred_count, pixel);
}
uint4 PtPackMaterial(PtMaterial m) {
  return uint4(f32tof16(m.albedo.r) | (f32tof16(m.albedo.g) << 16), f32tof16(m.albedo.b) | (f32tof16(m.roughness) << 16),
               f32tof16(m.f0.r) | (f32tof16(m.f0.g) << 16),
               f32tof16(m.f0.b) | (uint(saturate(m.spec_scale) * 255.0 + 0.5) << 16)
                | ((1u | (m.character ? 2u : 0u) | (m.reflective ? 4u : 0u)) << 24));
}
PtMaterial PtUnpackMaterial(uint4 v) {
  PtMaterial m;
  m.albedo = float3(f16tof32(v.x), f16tof32(v.x >> 16), f16tof32(v.y)); m.roughness = f16tof32(v.y >> 16);
  m.f0 = float3(f16tof32(v.z), f16tof32(v.z >> 16), f16tof32(v.w)); m.spec_scale = ((v.w >> 16) & 255u) * (1.0 / 255.0);
  m.character = ((v.w >> 24) & 2u) != 0u;
  m.reflective = ((v.w >> 24) & 4u) != 0u;
  return m;
}
uint4 PtReadMaterial(uint index) {
  const uint base = index * 4u;
  return uint4(g_pt_material_values[base], g_pt_material_values[base + 1u], g_pt_material_values[base + 2u], g_pt_material_values[base + 3u]);
}
// The material at a bounce hit: the G-buffer when the hit is the visible surface on screen, else the cache, else grey 0.5.
PtMaterial PtHitMaterial(float3 position, float3 nh) {
  PtMaterial m;
  m.albedo = float3(0.5, 0.5, 0.5); m.roughness = 1.0; m.f0 = float3(0.04, 0.04, 0.04); m.spec_scale = 1.0; m.character = false; m.reflective = false;
  if (g_mat_enabled == 0u) return m;
  const float4 clip = mul(g_pt_view_proj, float4(position, 1.0));
  if (clip.w > 1e-6) {
    const float2 uv = float2(clip.x / clip.w * 0.5 + 0.5, 0.5 - clip.y / clip.w * 0.5);
    if (all(uv >= 0.0) && all(uv < 1.0)) {
      const uint2 pixel = min(uint2(uv * float2(g_size)), g_size - 1u);
      const float depth = g_pt_depth.Load(int3(pixel, 0));
      const float2 center = (float2(pixel) + 0.5) / float2(g_size);
      const float4 world_h = mul(g_pt_inv_view_proj, float4(center.x * 2.0 - 1.0, 1.0 - center.y * 2.0, depth, 1.0));
      if (depth > 0.0 && depth <= 1.0 && abs(world_h.w) > 1e-8
          && length(world_h.xyz / world_h.w - position) <= 0.05 + 0.02 * length(position - g_sharc_camera)) {
        PtCount(0u);
        return PtGBufferMaterial(pixel);
      }
    }
  }
  uint64_t key;
  const uint index = HashGridFindEntry(PtMaterialData(), PtCachePosition(position, nh), PtMaterialKeyNormal(nh), PtMaterialGrid(), key);
  if (index != HASH_GRID_INVALID_CACHE_INDEX && ((PtReadMaterial(index).w >> 24) & 1u) != 0u) {
    PtCount(1u);
    return PtUnpackMaterial(PtReadMaterial(index));
  }
  PtCount(2u);
  return m;
}
// A bounce hit's cache data (OpenRemix pathtrace.comp:1097-1124): placed slightly above the surface, toward the ray.
SharcHitData PtSharcHit(float3 position, float3 nh, float3 dir, float3 albedo) {
  SharcHitData data;
  data.positionWorld = PtCachePosition(position, nh);
  data.normalWorld = nh;
  data.radianceDirectionWorld = -dir;
  data.radianceDirectionWeight = 0;
  data.materialDemodulation = max(albedo, float3(0.05, 0.05, 0.05));
  return data;
}
