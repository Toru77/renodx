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
};

struct SplitBrdf {
  float demodulatedDiffuse;
  float3 specular;
};

float CalcLuminance(float3 c) { return dot(c, float3(0.2126, 0.7152, 0.0722)); }
float square(float v) { return v * v; }
float Lambert(float3 normal, float3 lightIncident) { return max(0.0, -dot(normal, lightIncident)); }
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
  s.flags = 1u | ((rs.flags & RTXDI_SURFACE_FLAG_CHARACTER) != 0u ? 2u : 0u);
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
  s.material.specularF0 = m.f0;
#if defined(PT_WATER_SURFACE)
  s.material.roughness = saturate(g_water_roughness * g_refl_roughness_scale);
#else
  const float mrt_scale = prev ? (g_pt_prev_mrt1.Load(int3(pixel, 0)).y & 255u) * (1.0 / 255.0)
                               : (g_pt_mrt1.Load(int3(pixel, 0)).y & 255u) * (1.0 / 255.0);
  const bool water_rule = (g_refl_flags & 8u) != 0u && (normal_flags & 2u) != 0u;
  s.material.roughness = water_rule ? saturate(g_water_roughness * g_refl_roughness_scale)
                                    : saturate(m.roughness * mrt_scale * g_refl_roughness_scale);
#endif
  s.diffuseProbability = PtSurfaceDiffuseProbability(s);
  return s;
}

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
  s.material.specularF0 = m.f0;
  s.material.roughness = saturate(m.roughness);
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
  const float ndl = saturate(dot(s.normal, dir));
  const float diffusePdf = ndl / 3.14159265;
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
  if (isDelta) {
    return Schlick_Fresnel(s.material.specularF0, saturate(dot(l, s.normal))) / max(1.0 - s.diffuseProbability, 1e-4);
  }
  const float3 diffuse = s.material.diffuseAlbedo * Lambert(s.normal, -l);
  const float3 specular = s.material.roughness >= kMinRoughness
      ? GGX_times_NdotL(s.viewDir, l, s.normal, s.material.roughness, s.material.specularF0) : 0.0;
  return diffuse + specular;
}
bool RAB_SurfaceImportanceSampleBsdf(RAB_Surface s, inout RTXDI_RandomSamplerState rng, out float3 dir,
                                     out RTXDI_BrdfRaySampleProperties props) {
  const float rnd = RTXDI_GetNextRandom(rng);
  const float2 u = float2(RTXDI_GetNextRandom(rng), RTXDI_GetNextRandom(rng));
  props = RTXDI_DefaultBrdfRaySampleProperties();
  if (rnd < s.diffuseProbability) {
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
float3 RAB_GetReflectedBsdfRadianceForSurface(float3 location, float3 radiance, RAB_Surface s) {
  const float3 l = normalize(location - s.worldPos);
  return radiance * RAB_SurfaceEvaluateBsdfTimesNoL(s, l, false);
}
float RAB_GetPTSampleTargetPdfForSurface(float3 location, float3 radiance, RAB_Surface s) {
  return RTXDI_Luminance(RAB_GetReflectedBsdfRadianceForSurface(location, radiance, s));
}
float RAB_GetLightSampleTargetPdfForSurface(RAB_LightSample sample, RAB_Surface s) {
  return RTXDI_Luminance(RAB_GetReflectedBsdfRadianceForSurface(sample.position, sample.radiance, s));
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
  return abs(a.roughness - b.roughness) < 0.2 && CalcLuminance(abs(a.diffuseAlbedo - b.diffuseAlbedo)) < 0.2;
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
    s.solidAnglePdf = 1.0;
    s.radiance = max(g_pt_sun_color, 0.0);
    s.position = surface.worldPos + g_pt_sun_dir * kPtFarLightDistance;
    return s;
  }
  const LightParam lp = g_pt_lights[info.index];
  const float3 to = lp.pos - surface.worldPos;
  const float d = length(to);
  const float3 l = d > 1e-6 ? to / d : surface.normal;
  float3 color = max((surface.flags & 2u) != 0u ? lp.charaColor : lp.color, 0.0);
  color *= LampDistanceAttenuation(lp, to);
  if (info.type == LAMP_TYPE_SPOT) color *= LampSpotCone(lp, l);
  s.radiance = color;
  s.position = lp.pos;
  s.solidAnglePdf = 1.0;
  return s;
}
float RAB_GetConservativeVisibility(RAB_Surface surface, float3 light_position) {
  WorldTraceCounters counters;
  WorldCameraView view = {false, 1.0, 0u, 0u};
  const float3 bias = (surface.flags & 2u) != 0u ? g_char_bias : g_env_bias;
  const float3 origin = surface.worldPos + surface.normal * bias;
  const float3 to = light_position - origin;
  const float distance = length(to);
  if (distance <= 1e-6) return 1.0;
  const float3 dir = to / distance;
  const bool sun = (surface.flags & 4u) != 0u;
  float reach = distance - (sun ? 0.0 : g_lamp_clearance);
  if (sun) {
    view.require_flags = g_sun_casters_only != 0u ? INSTANCE_SHADOW_CASTER : 0u;
    reach = g_ray_max;
  }
  if (reach <= 1e-3) return 1.0;
  g_rt12_dynamic_mask = g_dynamic_on != 0u ? RT12_MASK_DYNAMIC : 0u;
  return TraceWorldRayDxr(origin, dir, 1e-3, reach, true, view, counters).prim == 0xFFFFFFFFu ? 1.0 : 0.0;
}
float RAB_GetConservativeVisibility(RAB_Surface surface, RAB_LightSample sample) {
  if (sample.sun) surface.flags |= 4u;
  return RAB_GetConservativeVisibility(surface, sample.position);
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

SplitBrdf EvaluateBrdfPT(RAB_Surface s, float3 n, float3 v, float3 l) {
  SplitBrdf brdf = (SplitBrdf)0;
  brdf.demodulatedDiffuse = Lambert(s.normal, -l);
  brdf.specular = GGX_times_NdotL(v, l, s.normal, max(s.material.roughness, kMinRoughness), s.material.specularF0);
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

// NEE at a path vertex: the game's ReGIR pool or uniform local lights, plus the sun.
void PtSampleNee(RAB_Surface surface, inout RTXDI_RandomSamplerState rng, out float3 radiance_from_lights,
                 out RTXDI_SampledLightData sampled_light_data, out float nee_pdf, out float scatter_pdf,
                 out RAB_LightSample light_sample) {
  radiance_from_lights = 0.0;
  sampled_light_data = RTXDI_SampledLightData_CreateInvalidData();
  nee_pdf = 0.0;
  scatter_pdf = 0.0;
  light_sample = RAB_EmptyLightSample();

  RTXDI_DIReservoir reservoir = RTXDI_EmptyDIReservoir();
  RAB_LightSample local_sample = RAB_EmptyLightSample();
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
      const RAB_LightSample sample = RAB_SamplePolymorphicLight(info, surface, uv);
      const float target_pdf = RTXDI_Luminance(RAB_GetReflectedBsdfRadianceForSurface(sample.position, sample.radiance, surface));
      const float inv_source_pdf = 1.0 / max(source_pdf * count, 1e-8);
      const uint packed_uv = uint(saturate(uv.x) * 0xffff) | (uint(saturate(uv.y) * 0xffff) << 16);
      if (RTXDI_StreamSample(reservoir, index, packed_uv, RTXDI_GetNextRandom(rng), target_pdf, inv_source_pdf)) {
        local_sample = sample;
      }
    }
    RTXDI_FinalizeResampling(reservoir, 1.0, 1.0);
    reservoir.M = 1.0;
  }

  RAB_LightSample infinite_sample = RAB_EmptyLightSample();
  const RTXDI_DIReservoir infinite = RTXDI_SampleInfiniteLights(rng, surface, g_restir.nee.numInfiniteLightSamples,
                                                               g_restir.light_buffer.infiniteLightBufferRegion, infinite_sample);
  if (RTXDI_CombineDIReservoirs(reservoir, infinite, RTXDI_GetNextRandom(rng), infinite.targetPdf)) {
    light_sample = infinite_sample;
  } else if (local_sample.solidAnglePdf > 0.0) {
    light_sample = local_sample;
  }
  RTXDI_FinalizeResampling(reservoir, 1.0, 1.0);
  reservoir.M = 1.0;
  if (reservoir.weightSum <= 0.0 || light_sample.solidAnglePdf <= 0.0) return;

  const float3 l = normalize(light_sample.position - surface.worldPos);
  surface.flags |= light_sample.sun ? 4u : 0u;
  radiance_from_lights = RAB_GetReflectedBsdfRadianceForSurface(light_sample.position, light_sample.radiance, surface);
  radiance_from_lights *= RAB_GetConservativeVisibility(surface, light_sample.position);
  radiance_from_lights *= RTXDI_GetDIReservoirInvPdf(reservoir) / light_sample.solidAnglePdf;
  RTXDI_SampledLightData_SetLightData(sampled_light_data, reservoir.lightData & RTXDI_DIReservoir_LightIndexMask);
  sampled_light_data.uvData = reservoir.uvData;
  nee_pdf = 1.0 / reservoir.weightSum;
  scatter_pdf = RAB_SurfaceEvaluateBrdfPdf(surface, l);
}

#endif  // FALCOM_WORLD_PT_BRIDGE_HLSLI
