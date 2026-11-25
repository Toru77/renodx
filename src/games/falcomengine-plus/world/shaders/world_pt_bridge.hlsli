#pragma once
// This software contains source code provided by NVIDIA Corporation.
// RAB bridge for RTXDI ReSTIR PT. The BRDF, light and MIS functions follow NVIDIA FullSample's
// LightingPasses/RtxdiApplicationBridge and ShadingHelpers, with the game's world-tracing inputs.
#ifndef FALCOM_WORLD_PT_BRIDGE_HLSLI
#define FALCOM_WORLD_PT_BRIDGE_HLSLI

#include "world_pt_common.hlsli"
#include <Rtxdi/Utils/RandomSamplerState.hlsli>
#include <Rtxdi/Utils/BrdfRaySample.hlsli>
#include <Rtxdi/DI/Reservoir.hlsli>
#include <Rtxdi/Utils/Color.hlsli>
#include <Rtxdi/Utils/SampledLightData.hlsli>
#include <Rtxdi/Utils/Checkerboard.hlsli>
#include <Rtxdi/PT/Reservoir.hlsli>

static const float kMinRoughness = 0.03f;
#define RAB_DISTANT_LIGHT_DISTANCE 100000.0f
static const float kPtFarLightDistance = 100000.0f;

struct RAB_Material {
  float3 diffuseAlbedo;
  float3 specularF0;
  float roughness;
  float3 emissiveColor;
  bool character;
};

struct RAB_Surface {
  float3 worldPos;
  float3 viewDir;
  float3 normal;
  float3 geoNormal;
  float viewDepth;
  float diffuseProbability;
  RAB_Material material;
  uint flags;
  uint2 pixel;
  uint material_source;
  uint material_index;
  float lampScale;
};

struct RAB_LightInfo {
  uint index;
  uint type;
};

struct RAB_LightSample {
  float3 position;
  float3 radiance;
  float solidAnglePdf;
  bool analytic;
  bool sun;
};

struct RAB_RayPayload {
  uint prim;
  float3 position;
  float3 normal;
  float t;
};

struct RAB_PathTracerUserData {
  uint16_t pathType;
  uint2 pixel;
  float3 color_sum;
  // A18 initial-pass debug views: first-hit (bounce depth 2) facts, written to u14-u16 by world_pt_initial.
  uint debug_status;
  float4 debug_value;
  uint4 debug_word;
};

struct SplitBrdf {
  float demodulatedDiffuse;
  float3 specular;
};

float CalcLuminance(float3 c) { return dot(c, float3(0.2126, 0.7152, 0.0722)); }
float square(float v) { return v * v; }
float Lambert(float3 normal, float3 lightIncident) { return max(0.0, -dot(normal, lightIncident)) / 3.14159265; }
void ConstructONB(float3 normal, out float3 tangent, out float3 bitangent) {
  const float sign = normal.z >= 0.0 ? 1.0 : -1.0;
  const float a = -1.0 / (sign + normal.z);
  const float b = normal.x * normal.y * a;
  tangent = float3(1.0 + sign * normal.x * normal.x * a, sign * b, -sign * normal.x);
  bitangent = float3(b, sign + normal.y * normal.y * a, -normal.y);
}
float3 Schlick_Fresnel(float3 f0, float vdh) { return f0 + (1.0 - f0) * pow(max(1.0 - vdh, 0.0), 5.0); }
float G_SmithOverNdotV(float roughness, float ndv, float ndl) {
  const float alpha = square(roughness);
  const float g1 = ndv * sqrt(square(alpha) + (1.0 - square(alpha)) * square(ndl));
  const float g2 = ndl * sqrt(square(alpha) + (1.0 - square(alpha)) * square(ndv));
  return 2.0 * ndl / max(g1 + g2, 1e-6);
}
float3 GGX_times_NdotL(float3 v, float3 l, float3 n, float roughness, float3 f0) {
  const float3 h = normalize(l + v);
  const float ndl = saturate(dot(n, l));
  if (ndl <= 0.0) return 0.0;
  const float vdh = saturate(dot(v, h));
  const float ndh = saturate(dot(n, h));
  const float alpha = square(roughness);
  const float d = square(alpha) / (3.14159265 * square(square(ndh) * square(alpha) + (1.0 - square(ndh))));
  return Schlick_Fresnel(f0, vdh) * (d * G_SmithOverNdotV(roughness, saturate(dot(n, v)), ndl) / 4.0);
}
// Verbatim from FullSample HelperFunctions.hlsli:40-55.
#define PT_LIGHT_SAMPLING_EPSILON 1e-10  // FullSample PolymorphicLight.hlsli:20.
/*https://graphics.pixar.com/library/OrthonormalB/paper.pdf*/
void BranchlessONB(in float3 n, out float3 b1, out float3 b2)
{
    float sign = n.z >= 0.0f ? 1.0f : -1.0f;
    float a = -1.0f / (sign + n.z);
    float b = n.x * n.y * a;
    b1 = float3(1.0f + sign * n.x * n.x * a, sign * b, -sign * n.x);
    b2 = float3(b, sign + n.y * n.y * a, -n.y);
}

float3 SphericalDirection(float sinTheta, float cosTheta, float sinPhi, float cosPhi, float3 x, float3 y, float3 z)
{
    return sinTheta * cosPhi * x + sinTheta * sinPhi * y + cosTheta * z;
}
float2 SampleDisk(float2 random) {
  const float angle = 2.0 * 3.14159265 * random.x;
  return float2(cos(angle), sin(angle)) * sqrt(random.y);
}
float3 SampleCosHemisphere(float2 random, out float solidAnglePdf) {
  const float elevation = sqrt(saturate(1.0 - random.y));
  solidAnglePdf = elevation / 3.14159265;
  return float3(SampleDisk(random), elevation);
}
float3 ImportanceSampleGGX_VNDF(float2 random, float roughness, float3 ve, float ndf_trim) {
  const float alpha = square(roughness);
  const float3 vh = normalize(float3(alpha * ve.x, alpha * ve.y, ve.z));
  const float lensq = square(vh.x) + square(vh.y);
  const float3 t1 = lensq > 0.0 ? float3(-vh.y, vh.x, 0.0) * (1.0 / sqrt(lensq)) : float3(1.0, 0.0, 0.0);
  const float3 t2 = cross(vh, t1);
  const float r = sqrt(random.x * ndf_trim);
  const float phi = 2.0 * 3.14159265 * random.y;
  const float t1v = r * cos(phi);
  const float s = 0.5 * (1.0 + vh.z);
  const float t2v = (1.0 - s) * sqrt(1.0 - square(t1v)) + s * r * sin(phi);
  const float3 nh = t1v * t1 + t2v * t2 + sqrt(max(0.0, 1.0 - square(t1v) - square(t2v))) * vh;
  return float3(alpha * nh.x, alpha * nh.y, max(0.0, nh.z));
}
float ImportanceSampleGGX_VNDF_PDF(float roughness, float3 n, float3 v, float3 l) {
  const float3 h = normalize(l + v);
  const float ndh = saturate(dot(n, h));
  const float vdh = saturate(dot(v, h));
  const float alpha = square(roughness);
  const float d = square(alpha) / (3.14159265 * square(square(ndh) * square(alpha) + (1.0 - square(ndh))));
  return vdh > 0.0 ? d / (4.0 * vdh) : 0.0;
}

RAB_Surface PtEmptySurface() {
  RAB_Surface s = (RAB_Surface)0;
  s.viewDepth = -1.0;
  return s;
}
RAB_Surface RAB_EmptySurface() { return PtEmptySurface(); }
bool RAB_IsSurfaceValid(RAB_Surface s) { return (s.flags & 1u) != 0u; }
float3 RAB_GetSurfaceWorldPos(RAB_Surface s) { return s.worldPos; }
void RAB_SetSurfaceWorldPos(inout RAB_Surface s, float3 p) { s.worldPos = p; }
float3 RAB_GetSurfaceNormal(RAB_Surface s) { return s.normal; }
void RAB_SetSurfaceNormal(inout RAB_Surface s, float3 n) { s.normal = n; }
float3 RAB_GetSurfaceGeoNormal(RAB_Surface s) { return s.geoNormal; }
float3 RAB_GetSurfaceViewDir(RAB_Surface s) { return s.viewDir; }
float RAB_GetSurfaceLinearDepth(RAB_Surface s) { return s.viewDepth; }
float RAB_GetSurfaceRoughness(RAB_Surface s) { return s.material.roughness; }
RAB_Material RAB_GetMaterial(RAB_Surface s) { return s.material; }
float3 RAB_GetEmissiveColor(RAB_Material m) { return m.emissiveColor; }
float3 RAB_GetLightDirDistance(RAB_Surface s, RAB_LightSample sample, out float3 dir, out float distance) {
  float3 to = sample.position - s.worldPos;
  distance = length(to);
  dir = distance > 1e-6 ? to / distance : s.normal;
  return dir;
}
float3 RAB_LightSamplePosition(RAB_LightSample s) { return s.position; }
float3 RAB_LightSampleRadiance(RAB_LightSample s) { return s.radiance; }
float RAB_LightSampleSolidAnglePdf(RAB_LightSample s) { return s.solidAnglePdf; }
bool RAB_IsAnalyticLightSample(RAB_LightSample s) { return s.analytic; }
bool RAB_IsValidHit(RAB_RayPayload p) { return p.prim != 0xFFFFFFFFu; }
float RAB_RayPayloadGetCommittedHitT(RAB_RayPayload p) { return p.t; }
bool RAB_RayPayloadIsFrontFace(RAB_RayPayload p) { return p.t < 1e30; }

float PtSurfaceDiffuseProbability(RAB_Surface s) {
  if ((s.flags & 32u) != 0u) return 1.0;
  const float diffuse = CalcLuminance(s.material.diffuseAlbedo);
  const float specular = CalcLuminance(Schlick_Fresnel(s.material.specularF0, saturate(dot(s.viewDir, s.normal))));
  const float sum = diffuse + specular;
  return sum < 1e-7 ? 1.0 : diffuse / sum;
}

RAB_Surface RAB_GetGBufferSurface(uint2 pixel, bool prev) {
  if (any(pixel >= g_size) || (prev && g_restir.history_valid == 0u)) return PtEmptySurface();
  RtxdiSurfaceResult rs;
  if (prev ? !PtReconstructSurface(g_pt_prev_depth, g_pt_prev_normal, g_pt_prev_scene, pixel, rs)
           : !PtLoadSurface(pixel, rs)) return PtEmptySurface();
  const uint normal_flags = prev ? g_pt_prev_normal.Load(int3(pixel, 0)).w : g_pt_normal.Load(int3(pixel, 0)).w;
#if defined(PT_WATER_SURFACE)
  if ((normal_flags & 2u) == 0u) return PtEmptySurface();
#endif
  RAB_Surface s = (RAB_Surface)0;
  s.flags = 1u | 8u | ((rs.flags & RTXDI_SURFACE_FLAG_CHARACTER) != 0u ? 2u : 0u);
  s.pixel = pixel;
  s.worldPos = rs.world_position;
  s.viewDepth = rs.linear_view_depth;
  s.normal = rs.shading_normal;
  s.geoNormal = rs.shading_normal;
  s.viewDir = rs.view_direction;
  PtMaterial m;
  if (prev) {
    m = PtGBufferMaterialFrom(g_pt_prev_color, g_pt_prev_normal, g_pt_prev_mrt1, g_pt_prev_mrt2,
                              g_pt_prev_deferred, g_restir.prev_deferred_count, pixel);
  } else {
    m = PtGBufferMaterial(pixel);
  }
  s.material.diffuseAlbedo = m.albedo;
#if defined(PT_WATER_SURFACE)
  // A18: water is a pure mirror; the reflected value is the incoming radiance and the game's water shader applies its
  // own Fresnel. m is the main G-buffer pixel under the water, so its F0 is not the water's.
  s.material.specularF0 = 1.0;
#else
  // Game specular scale (mrt1.z/255) gates the specular lobe, as the game lighting does; spec_scale 0 means no reflection (A15).
  // Game reflection rule (lighting_0xCA3D8596 :898,:968): cubemap reflection only where the deferred flag has 4 and not 32 (A17).
  s.material.specularF0 = (m.character || m.reflective) ? m.f0 * m.spec_scale : 0.0;
  // A18 design A: reflective environment primaries continue the path diffuse-only; their reflection is traced in final
  // shading. Light sampling keeps the full BSDF, so direct highlights stay in the reservoir.
  if (m.reflective) s.flags |= 16u;
  // A19: F0 0 still leaves Schlick's grazing term, so non-reflective environment surfaces drop the specular lobe (32u).
  if (!m.character && !m.reflective) s.flags |= 32u;
#endif
  s.material.character = m.character;
  s.lampScale = m.character ? 1.0 : m.lamp_scale;
#if defined(PT_WATER_SURFACE)
  s.material.diffuseAlbedo = 0.0;
  s.material.roughness = saturate(g_water_roughness);
#else
  const float mrt_scale = prev ? (g_pt_prev_mrt1.Load(int3(pixel, 0)).y & 255u) * (1.0 / 255.0)
                               : (g_pt_mrt1.Load(int3(pixel, 0)).y & 255u) * (1.0 / 255.0);
  const bool water_rule = (g_refl_flags & 8u) != 0u && (normal_flags & 2u) != 0u;
  s.material.roughness = m.character ? saturate(m.roughness)
      : (water_rule ? saturate(g_water_roughness * g_refl_roughness_scale)
                    : saturate(m.roughness * mrt_scale * g_refl_roughness_scale));
#endif
  s.diffuseProbability = PtSurfaceDiffuseProbability(s);
  return s;
}

#if defined(PT_WATER_SURFACE)
// Camera-only reprojection for the SSR-time water surface (RTV4 has no water motion), as the SSR shaders do with the
// previous view-projection (README "Motion vectors versus camera reprojection").
float2 PtWaterCameraMotion(uint2 pixel, float3 world_position) {
  const float4x4 previous_view_projection = float4x4(
      asfloat(g_pt_prev_scene.Load(int3(16u, 0, 0))), asfloat(g_pt_prev_scene.Load(int3(17u, 0, 0))),
      asfloat(g_pt_prev_scene.Load(int3(18u, 0, 0))), asfloat(g_pt_prev_scene.Load(int3(19u, 0, 0))));
  const float4 clip = mul(previous_view_projection, float4(world_position, 1.0));
  const float2 uv = float2(clip.x / clip.w * 0.5 + 0.5, 0.5 - clip.y / clip.w * 0.5);
  return uv * float2(g_size) - 0.5 - float2(pixel);
}
#endif

int2 RAB_ClampSamplePositionIntoView(int2 pixel, bool previous) {
  if (previous && g_restir.history_valid == 0u) return int2(-1, -1);
  return clamp(pixel, int2(0, 0), int2(g_size) - 1);
}
// Duplication map count (RestirPT.md "Duplication-Based History Reduction"), as in FullSample RAB_Buffers.hlsli.
uint RAB_GetDuplicationMapCount(int2 pixel) {
  if (any(pixel < 0) || any(pixel >= int2(g_size))) return 0u;
  return uint(round(saturate(RTXDI_PT_DUPLICATION_MAP[pixel].x) * 255.0));
}
// Compatibility-guided neighbour surfaces: the dedicated G-buffer stores normal + view depth, as in FullSample
// RAB_NeighborSelection.hlsli. World position is reconstructed from the current frame's scene matrices.
bool RAB_GetNeighborSelectionSurface(int2 pixel, out float3 worldNormal, out float3 worldPos, out float viewDepth) {
  worldNormal = 0.0;
  worldPos = 0.0;
  viewDepth = 0.0;
  if (any(pixel < 0) || any(pixel >= int2(g_size))) return false;
  const float4 data = g_pt_neighbor_selection_gbuffer[pixel];
  worldNormal = data.xyz;
  viewDepth = data.w;
  if (!isfinite(viewDepth) || viewDepth <= 0.0 || dot(worldNormal, worldNormal) < 0.5) return false;
  float4x4 view = (float4x4)0;
  float4x4 inv_view_proj = (float4x4)0;
  [unroll]
  for (uint row = 0u; row < 4u; ++row) {
    view[row] = asfloat(g_pt_scene.Load(int3(RTXDI_SURFACE_VIEW_ROW + row, 0, 0)));
    inv_view_proj[row] = asfloat(g_pt_scene.Load(int3(RTXDI_SURFACE_INVERSE_VIEW_PROJECTION_ROW + row, 0, 0)));
  }
  const float2 uv = (float2(pixel) + 0.5) / float2(g_size);
  const float2 ndc = float2(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0);
  const float4 far_h = mul(inv_view_proj, float4(ndc, 1.0, 1.0));
  if (!isfinite(far_h.w) || abs(far_h.w) <= 1e-8) return false;
  const float3 camera = PtSceneCamera(g_pt_scene);
  const float3 ray_direction = normalize(far_h.xyz / far_h.w - camera);
  const float ray_depth = dot(ray_direction, -normalize(view[2].xyz));
  if (!isfinite(ray_depth) || ray_depth <= 1e-6) return false;
  worldPos = camera + ray_direction * (viewDepth / ray_depth);
  return all(isfinite(worldPos));
}
void RAB_LastBounceDenoiserCallback(float3 lightPos, RAB_Surface surface, inout RAB_PathTracerUserData ptud) {}
void RAB_ReconnectionDenoiserCallback(RTXDI_PTReservoir reservoir, RAB_Surface surface, inout RAB_PathTracerUserData ptud) {}

RAB_Surface PtHitSurface(float3 position, float3 normal, float distance, float3 dir) {
  RAB_Surface s = (RAB_Surface)0;
  s.flags = 1u;
  s.worldPos = position;
  s.viewDepth = distance;
  s.normal = dot(normal, dir) > 0.0 ? -normal : normal;
  s.geoNormal = s.normal;
  s.viewDir = -dir;
  const PtMaterial m = PtHitMaterial(position, s.normal);
  s.material.diffuseAlbedo = m.albedo;
  s.material.specularF0 = (m.character || m.reflective) ? m.f0 * m.spec_scale : 0.0;
  if (!m.character && !m.reflective) s.flags |= 32u;
  s.material.roughness = saturate(m.roughness);
  s.material.character = m.character;
  s.lampScale = 1.0;
  s.material_source = m.debug_source;
  s.material_index = m.debug_index;
  s.diffuseProbability = PtSurfaceDiffuseProbability(s);
  return s;
}
RAB_Surface PtHitSurface(WorldTraceHit hit, float3 dir) {
  return PtHitSurface(hit.position, hit.normal, hit.t, dir);
}
RAB_Surface PtHitSurface(RAB_RayPayload payload, float3 dir) {
  return PtHitSurface(payload.position, payload.normal, payload.t, dir);
}

float3 WorldToTangent(RAB_Surface s, float3 w) {
  float3 t, b;
  ConstructONB(s.normal, t, b);
  return float3(dot(b, w), dot(t, w), dot(s.normal, w));
}
float3 TangentToWorld(RAB_Surface s, float3 h) {
  float3 t, b;
  ConstructONB(s.normal, t, b);
  return b * h.x + t * h.y + s.normal * h.z;
}
float RAB_SurfaceEvaluateBrdfPdf(RAB_Surface s, float3 dir) {
  const float ndl = saturate(dot(s.normal, dir));
  const float diffusePdf = ndl / 3.14159265;
  const float specularPdf = ImportanceSampleGGX_VNDF_PDF(max(s.material.roughness, kMinRoughness), s.normal, s.viewDir, dir);
  return ndl > 0.0 ? lerp(specularPdf, diffusePdf, s.diffuseProbability) : 0.0;
}
float RAB_SurfaceEvaluateBsdfPdf(RAB_Surface s, float3 dir, RTXDI_BrdfRaySampleProperties props) {
  // A15: FullSample's mixture of a delta "pdf" with a continuous pdf over-weights mirrors when p_d is large.
  if (props.IsDelta()) return saturate(dot(s.normal, dir)) > 0.0 ? 1.0 : 0.0;  // A15: delta weight = F/(1-p_d), the lobe-selection estimator
  const float ndl = saturate(dot(s.normal, dir));
  const float diffusePdf = ndl / 3.14159265;
  if ((s.flags & 48u) != 0u) return diffusePdf;  // A18 reflective diffuse-only continuation (16u), A19 no specular lobe (32u)
  float specularPdf = 0.0;
  if (s.material.roughness >= kMinRoughness) {
    specularPdf = ImportanceSampleGGX_VNDF_PDF(max(s.material.roughness, kMinRoughness), s.normal, s.viewDir, dir);
  } else if (props.IsDelta()) {
    specularPdf = 1.0;
  }
  return ndl > 0.0 ? lerp(specularPdf, diffusePdf, s.diffuseProbability) : 0.0;
}
float3 RAB_SurfaceEvaluateBsdfTimesNoL(RAB_Surface s, float3 l, bool isDelta) {
  if (dot(l, s.geoNormal) <= 0.0) return 0.0;
  const bool specular_lobe = (s.flags & 32u) == 0u;
  if (isDelta) {
    return specular_lobe
        ? Schlick_Fresnel(s.material.specularF0, saturate(dot(l, s.normal))) / max(1.0 - s.diffuseProbability, 1e-4) : 0.0;
  }
  const float3 diffuse = s.material.diffuseAlbedo * Lambert(s.normal, -l);
  const float3 specular = specular_lobe && s.material.roughness >= kMinRoughness
      ? GGX_times_NdotL(s.viewDir, l, s.normal, s.material.roughness, s.material.specularF0) : 0.0;
  return diffuse + specular;
}
bool RAB_SurfaceImportanceSampleBsdf(RAB_Surface s, inout RTXDI_RandomSamplerState rng, out float3 dir,
                                     out RTXDI_BrdfRaySampleProperties props) {
  const float rnd = RTXDI_GetNextRandom(rng);
  const float2 u = float2(RTXDI_GetNextRandom(rng), RTXDI_GetNextRandom(rng));
  props = RTXDI_DefaultBrdfRaySampleProperties();
  if ((s.flags & 48u) != 0u || rnd < s.diffuseProbability) {
    float pdf;
    dir = TangentToWorld(s, SampleCosHemisphere(u, pdf));
    props.SetDiffuse();
  } else {
    props.SetSpecular();
    if (s.material.roughness < kMinRoughness) {
      dir = reflect(-s.viewDir, s.normal);
      props.SetDelta();
    } else {
      const float3 ve = normalize(WorldToTangent(s, s.viewDir));
      dir = reflect(-s.viewDir, TangentToWorld(s, normalize(ImportanceSampleGGX_VNDF(u, max(s.material.roughness, kMinRoughness), ve, 1.0))));
    }
  }
  return dot(s.normal, dir) > 0.0;
}
// A tile-list entry is a candidate when it names a light of the list's type (world_rtxdi_initial_di.cs_6_5.hlsl:79-82);
// returns its light index, or -1 when it is not a candidate.
int PtTileEntry(LightIndexData tile, uint entry, uint point_count) {
  const bool point_entry = entry < point_count;
  const int listed = point_entry ? tile.pointLightIndices[entry] : tile.spotLightIndices[entry - point_count];
  return (listed >= 0 && uint(listed) < g_lamp_count
          && g_pt_light_table[uint(listed)].type == (point_entry ? LAMP_TYPE_POINT : LAMP_TYPE_SPOT)) ? listed : -1;
}
// Number of candidate entries in a tile cell; entries [0, point_count) are point lights, the rest spot lights.
uint PtTileUsableEntries(LightIndexData tile, out uint point_count, out uint entry_count) {
  point_count = min(tile.pointLightCount, 63u);
  entry_count = point_count + min(tile.spotLightCount, 63u);
  uint usable_count = 0u;
  for (uint entry = 0u; entry < entry_count; ++entry) usable_count += PtTileEntry(tile, entry, point_count) >= 0 ? 1u : 0u;
  return usable_count;
}
// Exact per-pixel game tile cell (copied from world_rtxdi_light_bridge.hlsli:59-76; the DI bridge is removed in M7).
bool PtGetLightTileCell(RAB_Surface surface, out uint cell) {
  cell = 0u;
  if (g_restir.tile_count == 0u || !isfinite(surface.viewDepth) || surface.viewDepth <= 0.0) return false;
  const float2 tile_size_inv = asfloat(g_pt_scene.Load(int3(RTXDI_SURFACE_LIGHT_TILE_SIZE_ROW, 0, 0)).xy);
  if (!all(isfinite(tile_size_inv)) || any(tile_size_inv <= 0.0)) return false;
  const uint2 tile = uint2(tile_size_inv * (float2(surface.pixel) + 0.5));
  if (tile.x >= 32u || tile.y >= 18u) return false;
  const uint base = tile.x + (tile.y << 5u);
  if (base >= g_restir.tile_count) return false;
  const float tile_depth_inv = g_pt_tiles[base].tileDepthInv;
  if (!isfinite(tile_depth_inv) || tile_depth_inv < 0.0) return false;
  const uint slice = (uint)clamp(tile_depth_inv * surface.viewDepth, 0.0, 7.0);
  cell = min(4607u, slice * 576u + base);
  return cell < g_restir.tile_count;
}

float3 RAB_GetReflectedBsdfRadianceForSurface(float3 location, float3 radiance, RAB_Surface s) {
  const float3 l = normalize(location - s.worldPos);
  return radiance * RAB_SurfaceEvaluateBsdfTimesNoL(s, l, false);
}
float RAB_GetPTSampleTargetPdfForSurface(float3 location, float3 radiance, RAB_Surface s) {
  if ((s.flags & 16u) != 0u) {  // A18: reconnections at a reflective primary carry the diffuse continuation only
    return RTXDI_Luminance(radiance * s.material.diffuseAlbedo * Lambert(s.normal, -normalize(location - s.worldPos)));
  }
  return RTXDI_Luminance(RAB_GetReflectedBsdfRadianceForSurface(location, radiance, s));
}
float RAB_GetLightSampleTargetPdfForSurface(RAB_LightSample sample, RAB_Surface s) {
  if (sample.solidAnglePdf <= 0.0) return 0.0;
  return RTXDI_Luminance(RAB_GetReflectedBsdfRadianceForSurface(sample.position, sample.radiance, s)) / sample.solidAnglePdf;
}
bool RAB_SurfaceImportanceSampleBrdf(RAB_Surface s, inout RTXDI_RandomSamplerState rng, out float3 dir) {
  RTXDI_BrdfRaySampleProperties props = RTXDI_DefaultBrdfRaySampleProperties();
  return RAB_SurfaceImportanceSampleBsdf(s, rng, dir, props);
}
bool RAB_TraceRayForLocalLight(float3 origin, float3 dir, float t_min, float t_max, out uint light_index, out float2 uv) {
  light_index = RTXDI_INVALID_LIGHT_INDEX;
  uv = 0.0;
  return false;
}
bool RAB_AreMaterialsSimilar(RAB_Material a, RAB_Material b) {
  return a.character == b.character && abs(a.roughness - b.roughness) < 0.2
      && CalcLuminance(abs(a.diffuseAlbedo - b.diffuseAlbedo)) < 0.2;
}

RAB_LightInfo RAB_EmptyLightInfo() {
  RAB_LightInfo info = (RAB_LightInfo)0;
  info.index = RTXDI_LIGHT_TABLE_INVALID_INDEX;
  return info;
}
RAB_LightInfo RAB_LoadLightInfo(uint index, bool prev) {
  RAB_LightInfo info = RAB_EmptyLightInfo();
  const uint lamp_count = prev ? g_restir.prev_light_count : g_lamp_count;
  if (index < lamp_count) {
    const uint type = prev ? g_pt_prev_light_table[index].type : g_pt_light_table[index].type;
    if (type == LAMP_TYPE_POINT || type == LAMP_TYPE_SPOT) {
      LightParam lp;
      if (prev) lp = g_pt_prev_lights[index];
      else lp = g_pt_lights[index];
      if (all(isfinite(float3(lp.pos.x, lp.pos.y, lp.pos.z))) && all(isfinite(lp.color))
          && lp.radius > 0.0) {
        info.index = index;
        info.type = type;
      }
    }
  } else if (index == lamp_count) {
    info.index = index;
    info.type = 2u;  // infinite directional light
  }
  return info;
}
RAB_LightSample RAB_EmptyLightSample() {
  RAB_LightSample s = (RAB_LightSample)0;
  s.position = float3(0, 0, 0);
  s.radiance = float3(0, 0, 0);
  s.analytic = true;
  return s;
}
RAB_LightSample RAB_SamplePolymorphicLight(RAB_LightInfo info, RAB_Surface surface, float2 uv) {
  RAB_LightSample s = RAB_EmptyLightSample();
  if (info.index == RTXDI_LIGHT_TABLE_INVALID_INDEX) return s;
  if (info.index == g_lamp_count) {
    s.sun = true;
    float3 direction = g_pt_sun_dir;
    const float angular_radius = g_restir.sun_angular_radius;
    if (angular_radius > 0.0) {
      // DirectionalLight::CalcSample (PolymorphicLight.hlsli:596-603), not normalized; our direction points toward the sun.
      float sin_half, cos_half;
      sincos(angular_radius, sin_half, cos_half);
      float3 t, b;
      BranchlessONB(direction, t, b);
      const float2 disk = SampleDisk(uv);
      const float3 sample_direction = direction + t * (disk.x * sin_half) + b * (disk.y * sin_half);
      s.solidAnglePdf = 1.0 / (2.0 * 3.14159265 * (1.0 - cos_half));
      direction = sample_direction;
    } else {
      s.solidAnglePdf = 1.0;
    }
    // Game lights are irradiance-normalized (lighting_0xCA3D8596: diffuse = albedo x colour x N.L, no 1/pi). The FullSample
    // Lambert BRDF divides by pi, and an area light spreads that over its solid angle: radiance = pi x colour / Omega,
    // where 1 / Omega = solidAnglePdf (delta lights use pdf 1).
    s.radiance = max(g_pt_sun_color, 0.0) * (3.14159265 * s.solidAnglePdf);
    s.position = surface.worldPos + direction * kPtFarLightDistance;
    return s;
  }
  const LightParam lp = g_pt_lights[info.index];
  float3 center = lp.pos;
  const float radius = g_restir.lamp_radius;
  float3 to = center - surface.worldPos;
  float d = length(to);
  float3 l = d > 1e-6 ? to / d : surface.normal;
  if (radius > 0.0) {
    // SphereLight::CalcSample (PolymorphicLight.hlsli:131-187) with random = uv and viewer = surface.worldPos.
    // FullSample assumes the viewer is never inside the sphere (PolymorphicLight.hlsli:120-121); keep the point
    // convention there so receivers near a large lamp still get its light.
    if (dot(to, to) <= radius * radius) {
      center = lp.pos;
      s.solidAnglePdf = 1.0;
    } else {
      const float light_distance2 = dot(to, to);
      const float radius2 = radius * radius;
      const float sinThetaMax2 = radius2 / light_distance2;
      const float cosThetaMax = sqrt(max(0.0, 1.0 - sinThetaMax2));
      const float phi = 2.0 * 3.14159265 * uv.x;
      const float cosTheta = lerp(cosThetaMax, 1.0, uv.y);
      const float sinTheta = sqrt(max(0.0, 1.0 - cosTheta * cosTheta));
      const float ds = d * cosTheta - sqrt(max(PT_LIGHT_SAMPLING_EPSILON, radius2 - light_distance2 * sinTheta * sinTheta));
      const float cosAlpha = (light_distance2 + radius2 - ds * ds) / (2.0 * d * radius);
      const float sinAlpha = sqrt(max(0.0, 1.0 - cosAlpha * cosAlpha));
      const float3 normal = normalize(to);
      float3 tangent, bitangent;
      BranchlessONB(normal, tangent, bitangent);
      float sinPhi, cosPhi;
      sincos(phi, sinPhi, cosPhi);
      const float3 radiusVector = SphericalDirection(sinAlpha, cosAlpha, sinPhi, cosPhi, -tangent, -bitangent, -normal);
      center = lp.pos + radius * radiusVector;
      s.solidAnglePdf = 1.0 / (2.0 * 3.14159265 * (1.0 - cosThetaMax));
    }
    to = center - surface.worldPos;
    d = length(to);
    l = d > 1e-6 ? to / d : surface.normal;
  } else {
    s.solidAnglePdf = 1.0;
  }
  float3 color = max(surface.material.character ? lp.charaColor * g_restir.char_lamp_intensity : lp.color, 0.0);
  color *= LampDistanceAttenuation(lp, to) * surface.lampScale;
  if (info.type == LAMP_TYPE_SPOT) color *= LampSpotCone(lp, l);
  if (g_restir.local_clamp_enabled != 0u) color = min(color, float3(g_restir.local_clamp, g_restir.local_clamp, g_restir.local_clamp));
  s.radiance = color * (3.14159265 * s.solidAnglePdf) * g_restir.lamp_intensity;
  s.position = center;
  return s;
}
float PtLightVisibility(RAB_Surface surface, float3 light_position) {
  WorldTraceCounters counters;
  WorldCameraView view = {false, 1.0, 0u, 0u};
  const bool primary = (surface.flags & 8u) != 0u;
  const bool character = (surface.flags & 2u) != 0u;
  const bool sun = (surface.flags & 4u) != 0u;
  float bias;
  if (primary && sun) bias = character ? g_restir.sun_char_normal_bias : g_restir.sun_normal_bias;
  else if (primary) bias = character ? g_restir.local_char_normal_bias : g_restir.local_normal_bias;
  else bias = character ? g_char_bias : g_env_bias;
  const float bias_sign = dot(surface.normal, normalize(light_position - surface.worldPos)) >= 0.0 ? 1.0 : -1.0;
  const float3 outward = surface.normal * bias_sign;
  float3 origin = surface.worldPos + outward * bias;
  if (primary && character) {
    const float3 to_light = light_position - surface.worldPos;
    const float distance_to_light = length(to_light);
    const float self_softening = sun ? g_restir.sun_char_self_softening : g_restir.local_char_self_softening;
    const float3 lateral_dir = normalize(to_light - outward * dot(to_light, outward) + float3(1e-5, 0.0, 0.0));
    origin += Rt12SelfSofteningOffset(outward, lateral_dir, min(self_softening, distance_to_light * 0.5));
  }
  const float3 to = light_position - origin;
  const float distance = length(to);
  if (distance <= 1e-6) return 1.0;
  const float3 dir = to / distance;
  float reach = distance - (sun ? 0.0 : (primary ? g_restir.local_clearance : g_lamp_clearance));
  if (sun) {
    view.require_flags = primary && g_restir.sun_casters_only != 0u ? INSTANCE_SHADOW_CASTER : 0u;
    if (primary) view.skip_flags = INSTANCE_FOLIAGE;
    reach = primary ? min(g_restir.sun_max_distance, g_ray_max) : g_ray_max;
  }
  if (reach <= 1e-3) return 1.0;
  g_rt12_dynamic_mask = g_dynamic_on != 0u ? RT12_MASK_DYNAMIC : 0u;
  return TraceWorldRayDxr(origin, dir, 1e-3, reach, true, view, counters).prim == 0xFFFFFFFFu ? 1.0 : 0.0;
}
float RAB_GetConservativeVisibility(RAB_Surface surface, RAB_LightSample sample) {
  if (sample.sun) surface.flags |= 4u;
  return PtLightVisibility(surface, sample.position);
}
// Reconnection visibility (FullSample RAB_VisibilityTest.hlsli:19-30): tight segment to a surface vertex; no lamp
// clearance, light biases or self-softening (those are light-shadow rules, PtLightVisibility).
float RAB_GetConservativeVisibility(RAB_Surface surface, float3 sample_position) {
  WorldTraceCounters counters;
  const WorldCameraView view = {false, 1.0, 0u, 0u};
  const float3 to = sample_position - surface.worldPos;
  const float distance = length(to);
  if (distance <= 0.003) return 1.0;
  const float offset = 0.001;
  const float3 origin = surface.worldPos + surface.geoNormal * ((dot(surface.geoNormal, to) >= 0.0 ? 1.0 : -1.0) * g_env_bias);
  g_rt12_dynamic_mask = g_dynamic_on != 0u ? RT12_MASK_DYNAMIC : 0u;
  return TraceWorldRayDxr(origin, to / distance, offset, max(offset, distance - offset * 2.0), true, view, counters).prim
             == 0xFFFFFFFFu ? 1.0 : 0.0;
}
float RAB_GetTemporalConservativeVisibility(RAB_Surface surface, float3 light_position, float3 prev_light_position) {
  return RAB_GetConservativeVisibility(surface, light_position);
}
float3 RAB_GetEnvironmentRadiance(float3 dir) { return PtSky(dir); }
float RAB_GetMISWeightForNEE(uint lightIndex, RAB_LightSample lightSample, float3 surfacePos, float scatterPdf) { return 1.0; }
float RAB_GetMISWeightForNEE(uint lightIndex, RAB_LightSample lightSample, float3 lightDirection, float invPartialJacobian, float scatterPdf) { return 1.0; }
float RAB_GetMISWeightForNEELight(uint lightIndex, RAB_LightSample lightSample, float3 surfacePos, float scatterPdf) { return 1.0; }
float GetMISWeightForNEELight(uint lightIndex, RAB_LightSample lightSample, float3 surfacePos, float scatterPdf) { return 1.0; }
float RAB_GetMISWeightForEnvironmentMap(float3 dir, RAB_Surface surface, RTXDI_BrdfRaySample brs) { return 1.0; }
float GetMISWeightForEnvironmentMap(float3 dir, RAB_Surface surface, RTXDI_BrdfRaySample brs) { return 1.0; }
float RAB_GetMISWeightForEmissiveSurface(RAB_RayPayload payload, RAB_Surface surface, RTXDI_BrdfRaySample brs) { return 1.0; }
float GetMISWeightForEmissiveSurface(RAB_RayPayload payload, RAB_Surface surface, RTXDI_BrdfRaySample brs) { return 1.0; }

RAB_PathTracerUserData RAB_EmptyPathTracerUserData() {
  RAB_PathTracerUserData data = (RAB_PathTracerUserData)0;
  return data;
}
void RAB_PathTracerUserDataSetPathType(inout RAB_PathTracerUserData data, uint16_t type) { data.pathType = type; }

float RAB_EvaluateLocalLightSourcePdf(uint lightIndex) { return 1.0 / float(max(g_lamp_count, 1u)); }
float RAB_EvaluateEnvironmentMapSamplingPdf(float3 dir) { return 0.0; }
float2 RAB_GetEnvironmentMapRandXYFromDir(float3 dir) { return float2(0.5, 0.5); }
int RAB_TranslateLightIndex(uint lightIndex, bool currentToPrevious) {
  const bool source_prev = !currentToPrevious;
  const uint source_count = source_prev ? g_restir.prev_light_count : g_lamp_count;
  const uint destination_count = source_prev ? g_lamp_count : g_restir.prev_light_count;
  if (lightIndex == source_count) {
    const RAB_LightInfo destination = RAB_LoadLightInfo(destination_count, !source_prev);
    return destination.type == 2u ? int(destination_count) : -1;
  }
  if (lightIndex >= source_count) return -1;
  if (g_restir.stable_light_remap != 0u) {
    if (g_restir.history_valid == 0u) return -1;
    const uint mapped = source_prev ? g_pt_previous_to_current[lightIndex] : g_pt_current_to_previous[lightIndex];
    if (mapped == RTXDI_LIGHT_TABLE_INVALID_INDEX || mapped >= destination_count) return -1;
    const RAB_LightInfo source_light = RAB_LoadLightInfo(lightIndex, source_prev);
    const RAB_LightInfo destination_light = RAB_LoadLightInfo(mapped, !source_prev);
    if (source_light.type == 0u || destination_light.type == 0u || source_light.type != destination_light.type) return -1;
    return int(mapped);
  }
  const RAB_LightInfo source_light = RAB_LoadLightInfo(lightIndex, source_prev);
  const RAB_LightInfo destination_light = RAB_LoadLightInfo(lightIndex, !source_prev);
  if (source_light.type == 0u || destination_light.type == 0u || source_light.type != destination_light.type) return -1;
  LightParam a, b;
  if (source_prev) {
    a = g_pt_prev_lights[lightIndex];
    b = g_pt_lights[lightIndex];
  } else {
    a = g_pt_lights[lightIndex];
    b = g_pt_prev_lights[lightIndex];
  }
  if (any(asuint(float4(a.pos, a.radius)) != asuint(float4(b.pos, b.radius)))
      || any(asuint(float4(a.color, a.radiusInv)) != asuint(float4(b.color, b.radiusInv)))
      || any(asuint(float4(a.charaColor, a.attenuation)) != asuint(float4(b.charaColor, b.attenuation)))
      || any(asuint(float4(a.vec, a.spotAngleInv)) != asuint(float4(b.vec, b.spotAngleInv)))
      || any(asuint(float4(a.attenuationAngle, a.specularIntensity, a.specularGlossiness, a.scatterAnisotropy))
             != asuint(float4(b.attenuationAngle, b.specularIntensity, b.specularGlossiness, b.scatterAnisotropy)))
      || any(asuint(float4(a.scatterColor, a.scatterDensity)) != asuint(float4(b.scatterColor, b.scatterDensity)))
      || asuint(a.translucency) != asuint(b.translucency) || a.shadowmapIndex != b.shadowmapIndex
      || asuint(a.scatterOffset) != asuint(b.scatterOffset) || asuint(a.userParam) != asuint(b.userParam)) return -1;
  return int(lightIndex);
}
float RAB_GetLightTargetPdfForVolume(float3 pos, RAB_LightSample sample) { return 0.0; }

SplitBrdf EvaluateBrdfPT(RAB_Surface s, float3 n, float3 v, float3 l, bool isDelta) {
  SplitBrdf brdf = (SplitBrdf)0;
  const bool specular_lobe = (s.flags & 32u) == 0u;
  if (isDelta) {
    if (specular_lobe) {
      brdf.specular = Schlick_Fresnel(s.material.specularF0, saturate(dot(l, n))) / max(1.0 - s.diffuseProbability, 1e-4);
    }
    return brdf;
  }
  brdf.demodulatedDiffuse = Lambert(n, -l);
  brdf.specular = specular_lobe && s.material.roughness >= kMinRoughness
      ? GGX_times_NdotL(v, l, n, s.material.roughness, s.material.specularF0) : 0.0;
  return brdf;
}
float3 DemodulateSpecular(float3 f0, float3 specular) { return specular / max(0.01, f0); }

// SHARC terminal radiance (A1.3): current GI and reflection early-out rules. cache is isotropic.
bool PtSharcTerminal(float3 position, float3 nh, float3 dir, float hit_t, float prev_alpha, bool specular_lobe,
                     float3 albedo, out float3 cached) {
  cached = 0.0;
  if (g_sharc_enabled == 0u) return false;
  const SharcHitData data = PtSharcHit(position, nh, dir, albedo);
  const float voxel = HashGridGetVoxelSize(HashGridGetLevel(data.positionWorld, PtSharcParameters().hashGridParameters),
                                           PtSharcParameters().hashGridParameters);
  if (specular_lobe) {
    if ((g_refl_flags & 2u) == 0u) return false;
    const float spread = min(1.0, 1.41421356 * prev_alpha / sqrt(max(1.0 - prev_alpha * prev_alpha, 1e-6)));
    if ((g_refl_flags & 4u) != 0u && hit_t * spread <= voxel * 1.732050808) return false;
  } else if (hit_t <= voxel * 1.732050808) {
    return false;
  }
  if (!SharcGetCachedRadiance(PtSharcParameters(), data, cached, false) || !all(isfinite(cached))) return false;
  cached = max(cached, 0.0);
  return true;
}

#include <Rtxdi/DI/InitialSampling.hlsli>

// RTXDI_SampleLightsForSurface combination (DI/InitialSampling.hlsli:637-660): local and sun reservoirs into an empty state.
RTXDI_DIReservoir PtCombineLocalAndSun(RTXDI_DIReservoir local, RAB_LightSample local_sample, RTXDI_DIReservoir sun,
                                       RAB_LightSample sun_sample, inout RTXDI_RandomSamplerState rng,
                                       out RAB_LightSample selected) {
  RTXDI_DIReservoir state = RTXDI_EmptyDIReservoir();
  RTXDI_CombineDIReservoirs(state, local, 0.5, local.targetPdf);
  const bool select_sun = RTXDI_CombineDIReservoirs(state, sun, RTXDI_GetNextRandom(rng), sun.targetPdf);
  RTXDI_FinalizeResampling(state, 1.0, 1.0);
  state.M = 1;
  selected = local_sample;
  if (select_sun) selected = sun_sample;
  return state;
}

// NEE at a path vertex: the game's ReGIR pool or uniform local lights, plus the sun.
void PtSampleNee(RAB_Surface surface, inout RTXDI_RandomSamplerState rng, out float3 radiance_from_lights,
                 out RTXDI_SampledLightData sampled_light_data, out float nee_pdf, out float scatter_pdf,
                 out RAB_LightSample light_sample, bool sample_sun) {
  radiance_from_lights = 0.0;
  sampled_light_data = RTXDI_SampledLightData_CreateInvalidData();
  nee_pdf = 0.0;
  scatter_pdf = 0.0;
  light_sample = RAB_EmptyLightSample();

  RTXDI_DIReservoir local_reservoir = RTXDI_EmptyDIReservoir();
  RAB_LightSample local_sample = RAB_EmptyLightSample();
  const RTXDI_InitialSamplingMisData mis_data = RTXDI_ComputeInitialSamplingMisData(g_restir.nee);
  const uint count = g_restir.nee.numLocalLightSamples;
  if (g_lamp_count > 0u && count > 0u) {
    const bool regir_eligible = g_regir_enabled != 0u && g_lamp_count <= RTXDI_LIGHT_TABLE_CAPACITY;
    if (regir_eligible) PtCount(9u);
    bool pool_valid = false;
    uint fallback_reason = regir_eligible ? 0u : 1u;
    PtReGirCell pool = (PtReGirCell)0;
    int3 key;
    if (regir_eligible && PtReGirCellKey(surface.worldPos, key)) {
      PtCount(10u);
      uint header_count, header_stride, cell_count, cell_stride;
      g_pt_regir_header.GetDimensions(header_count, header_stride);
      g_pt_regir_cells.GetDimensions(cell_count, cell_stride);
      if (header_count != 0u && g_pt_regir_header[0].valid != 0u
          && g_pt_regir_header[0].cell_count <= min(cell_count, PT_REGIR_MAX_CELLS)) {
        bool matched = false;
        for (uint cell_index = 0u; cell_index < g_pt_regir_header[0].cell_count; ++cell_index) {
          const PtReGirCell candidate = g_pt_regir_cells[cell_index];
          if (all(candidate.key == key) && candidate.valid != 0u) {
            matched = true;
            PtCount(11u);
            pool = candidate;
            pool_valid = true;
            for (uint slot = 0u; slot < PT_REGIR_SLOTS; ++slot) {
              if (pool.slots[slot] >= g_lamp_count) {
                pool_valid = false;
                fallback_reason = 5u;
                PtCount(14u);
              }
            }
            break;
          }
        }
        if (!matched) PtCount(12u);
      } else {
        fallback_reason = 4u;
        PtCount(13u);
      }
      if (fallback_reason == 0u && !pool_valid) fallback_reason = 3u;
    } else if (regir_eligible) {
      fallback_reason = 2u;
    }
    PtReGirSelection selection = (PtReGirSelection)0;
    if (pool_valid) {
      const uint next_rng = PtHash(PtHash(RTXDI_GetNextRandom(rng) * 16777216.0));
      selection = PtReGirSelect(next_rng >> 8, g_lamp_count, pool.slots);
    }
    if (selection.valid != 0u) {
      PtCount(15u);
    } else {
      if (fallback_reason == 0u) fallback_reason = 3u;
      PtCount(15u + fallback_reason);
    }
    for (uint i = 0u; i < count; ++i) {
      uint index;
      float source_pdf;
      if (selection.valid != 0u) {
        index = selection.light_index;
        source_pdf = selection.pmf;
      } else {
        index = min(uint(RTXDI_GetNextRandom(rng) * g_lamp_count), g_lamp_count - 1u);
        source_pdf = 1.0 / g_lamp_count;
      }
      const float2 uv = float2(RTXDI_GetNextRandom(rng), RTXDI_GetNextRandom(rng));
      const RAB_LightInfo info = RAB_LoadLightInfo(index, false);
      RTXDI_StreamLocalLightAtUVIntoReservoir(rng, mis_data, surface, g_restir.nee.brdfCutoff, mis_data.localLightMisWeight,
                                              index, uv, 1.0 / source_pdf, info, local_reservoir, local_sample);
    }
    RTXDI_FinalizeResampling(local_reservoir, 1.0, mis_data.numMisSamples);
    local_reservoir.M = 1.0;
  }

  RAB_LightSample sun_sample = RAB_EmptyLightSample();
  RTXDI_DIReservoir sun_reservoir = RTXDI_EmptyDIReservoir();
  if (sample_sun) {
    sun_reservoir = RTXDI_SampleInfiniteLights(rng, surface, g_restir.nee.numInfiniteLightSamples,
                                               g_restir.light_buffer.infiniteLightBufferRegion, sun_sample);
  }
  light_sample = RAB_EmptyLightSample();
  const RTXDI_DIReservoir reservoir =
      PtCombineLocalAndSun(local_reservoir, local_sample, sun_reservoir, sun_sample, rng, light_sample);
  if (reservoir.weightSum <= 0.0 || light_sample.solidAnglePdf <= 0.0) return;

  const float3 l = normalize(light_sample.position - surface.worldPos);
  surface.flags |= light_sample.sun ? 4u : 0u;
  radiance_from_lights = RAB_GetReflectedBsdfRadianceForSurface(light_sample.position, light_sample.radiance, surface);
  radiance_from_lights *= PtLightVisibility(surface, light_sample.position);
  radiance_from_lights *= RTXDI_GetDIReservoirInvPdf(reservoir) / light_sample.solidAnglePdf;
  RTXDI_SampledLightData_SetLightData(sampled_light_data, reservoir.lightData & RTXDI_DIReservoir_LightIndexMask);
  sampled_light_data.uvData = reservoir.uvData;
  nee_pdf = 1.0 / reservoir.weightSum;
  scatter_pdf = RAB_SurfaceEvaluateBrdfPdf(surface, l);
}

// The game's lamp response (lighting_0xCA3D8596 :1378-1442) summed over this pixel's tile cell, for environment and
// characters: colour x attenuation x cone x max(N.L, translucency) from each lamp centre. One lamp, picked in proportion
// to its response, traces one shadow ray to a sphere sample, so the estimate is bounded by the unshadowed sum (A24a).
float3 PtGameLamp(RAB_Surface surface, inout RTXDI_RandomSamplerState rng) {
  uint cell = 0u;
  if (!PtGetLightTileCell(surface, cell)) return 0.0;
  const LightIndexData tile = g_pt_tiles[cell];
  uint point_count, entry_count;
  if (PtTileUsableEntries(tile, point_count, entry_count) == 0u) return 0.0;
  float3 selected_response = 0.0;
  uint selected_index = 0u;
  float response_sum = 0.0;
  for (uint entry = 0u; entry < entry_count; ++entry) {
    const int listed = PtTileEntry(tile, entry, point_count);
    if (listed < 0) continue;
    const LightParam lp = g_pt_lights[uint(listed)];
    const float3 to_c = lp.pos - surface.worldPos;
    const float3 l_c = normalize(to_c);
    float3 color = max(surface.material.character
        ? lp.charaColor * g_restir.char_lamp_intensity
        : lp.color * (g_restir.lamp_material_scale != 0u ? surface.lampScale : 1.0), 0.0);
    color *= LampDistanceAttenuation(lp, to_c);
    if (g_pt_light_table[uint(listed)].type == LAMP_TYPE_SPOT) color *= LampSpotCone(lp, l_c);
    if (g_restir.local_clamp_enabled != 0u) color = min(color, float3(g_restir.local_clamp, g_restir.local_clamp, g_restir.local_clamp));
    const float3 response = color * max(max(dot(surface.normal, l_c), lp.translucency * g_restir.lamp_wrap_scale), 0.0)
        * g_restir.lamp_intensity;
    const float weight = RTXDI_Luminance(response);
    response_sum += weight;
    if (weight > 0.0 && RTXDI_GetNextRandom(rng) * response_sum < weight) {
      selected_response = response;
      selected_index = uint(listed);
    }
  }
  if (response_sum <= 0.0) return 0.0;
  const float2 uv = float2(RTXDI_GetNextRandom(rng), RTXDI_GetNextRandom(rng));
  const RAB_LightSample light_sample = RAB_SamplePolymorphicLight(RAB_LoadLightInfo(selected_index, false), surface, uv);
  return selected_response * (response_sum / RTXDI_Luminance(selected_response))
         * PtLightVisibility(surface, light_sample.position);
}

#endif  // FALCOM_WORLD_PT_BRIDGE_HLSLI
