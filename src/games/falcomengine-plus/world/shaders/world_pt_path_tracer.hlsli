#pragma once
// This software contains source code provided by NVIDIA Corporation.
// Port of NVIDIA FullSample LightingPasses/RtxdiApplicationBridge/PathTracer/RAB_PathTracer.hlsli. PSR, debug path
// recording and reordering are not ported; SHARC terminal radiance, the game's NEE and the water hit test replace them.
#ifndef FALCOM_WORLD_PT_PATH_TRACER_HLSLI
#define FALCOM_WORLD_PT_PATH_TRACER_HLSLI

#include "world_pt_bridge.hlsli"
#include "../rtxdi/rtxdi_debug_layout.h"
#include <Rtxdi/PT/PathTracerContext.hlsli>
#include <Rtxdi/PT/PathTracerRandomContext.hlsli>

bool IsEmissiveSamplingEnabled() { return g_sharc_enabled != 0u; }
bool IsNeeSamplingEnabled() { return true; }

float3 PtGiClampLe(float3 le, float3 throughput) {
  if (g_restir.gi_clamp_per_bounce == 0u) return le;
  return le * min(1.0, g_gi_clamp / max(CalcLuminance(le * throughput), 1e-6));
}

template<typename PTContextType>
bool ShouldRunRussianRoulette(inout RTXDI_PathTracerContext<PTContextType> ctx) {
  return g_roulette != 0u && ctx.GetBounceDepth() >= g_roulette_min_bounce + 2u;
}

float CalculateRussianRouletteContinuationProbability(float3 throughput) {
  return max(0.05, pow(saturate(max(throughput.r, max(throughput.g, throughput.b))), 0.25));
}

RayDesc PtContinuationRay(float3 n, float3 p, float3 dir) {
  RayDesc ray = (RayDesc)0;
  ray.Origin = p + (dot(dir, n) > 0.0 ? n : -n) * 1e-3;
  ray.Direction = dir;
  ray.TMin = 1e-3;
  ray.TMax = g_ray_max;
  return ray;
}

RAB_RayPayload PtTraceNextBounce(RayDesc ray) {
  RAB_RayPayload payload = (RAB_RayPayload)0;
  payload.prim = 0xFFFFFFFFu;
  payload.t = g_ray_max;
  g_rt12_dynamic_mask = g_dynamic_on != 0u ? RT12_MASK_DYNAMIC : 0u;
  WorldTraceCounters counters;
  const WorldCameraView bounce_view = {true, 1.0, 0u, 0u};
  const WorldTraceHit hit = TraceWorldRayDxr(ray.Origin, ray.Direction, 1e-3, g_ray_max, false, bounce_view, counters);
  if (hit.prim != 0xFFFFFFFFu) {
    payload.prim = hit.prim;
    payload.position = hit.position;
    payload.normal = hit.normal;
    payload.t = hit.t;
  }
  return payload;
}

template<typename PTContextType>
bool ShouldSampleEmissiveSurfaces(const RTXDI_PathTracerContext<PTContextType> ctx, RAB_Surface prevSurface) {
  return ctx.ShouldSampleEmissiveSurfaces() && g_sharc_enabled != 0u;
}

template<typename PTContextType>
float3 SampleEmissiveLightFromSurface(const RTXDI_PathTracerContext<PTContextType> ctx) {
  const RAB_Surface surface = ctx.GetIntersectionSurface();
  RAB_Surface prev = ctx.GetRcPrevSurface();
  const RTXDI_BrdfRaySample brs = ctx.GetBrdfRaySample();
  float3 cached = 0.0;
  if (ctx.GetBounceDepth() >= g_restir.sharc_min_bounce + 1u
      && PtSharcTerminal(surface.worldPos, surface.normal, -ctx.GetContinuationRay().Direction, ctx.GetTraceResult().t,
                         sqrt(max(prev.material.roughness, 0.0)), brs.properties.IsSpecular(), surface.material.diffuseAlbedo, cached)) {
    return cached;
  }
  return 0.0;
}

bool PtWaterHit(uint2 pixel, float3 hit_position, float3 n, float3 v) {
#if defined(PT_WATER_SURFACE)
  const float4 clip = mul(g_pt_view_proj, float4(hit_position, 1.0));
  if (clip.w > 1e-6) {
    const float2 huv = float2(clip.x / clip.w * 0.5 + 0.5, 0.5 - clip.y / clip.w * 0.5);
    if (all(huv >= 0.0) && all(huv < 1.0)
        && (g_pt_normal.Load(int3(min(uint2(huv * float2(g_size)), g_size - 1u), 0)).w & 2u) != 0u) {
      return true;
    }
  }
#endif
  return false;
}

template<typename PTContextType>
bool ShouldSampleNeeLights(const RTXDI_PathTracerContext<PTContextType> ctx) {
  return IsNeeSamplingEnabled() && ctx.ShouldSampleNee()
         && ctx.GetIntersectionSurface().material.roughness >= kMinRoughness;
}

template<typename PTContextType>
void HandleHit(inout RTXDI_PathTracerContext<PTContextType> ctx, inout RTXDI_PathTracerRandomContext ptRandContext,
               const RAB_Surface prevSurface, inout RAB_PathTracerUserData ud, uint2 pixel) {
  const RAB_RayPayload payload = ctx.GetTraceResult();
  const RayDesc ray = ctx.GetContinuationRay();
  const RAB_Surface surface = PtHitSurface(payload, ray.Direction);
  ctx.RecordPathIntersection(surface);

  if (ctx.IsPathTerminated()) return;

#if defined(PT_WATER_SURFACE)
  if (ctx.GetBounceDepth() == 2u && PtWaterHit(pixel, payload.position, surface.normal, surface.viewDir)) {
    ctx.RecordEnvironmentMapLightSample(PtGiClampLe(PtSky(reflect(-surface.viewDir, surface.normal)),
                                                     ctx.GetPathThroughput()), prevSurface,
                                        ptRandContext.initialRandomSamplerState);
    ctx.SetPathTermination(true);
    return;
  }
#endif

  const RTXDI_BrdfRaySample brs = ctx.GetBrdfRaySample();
  float3 cached = 0.0;
  const bool terminal = ctx.GetBounceDepth() >= g_restir.sharc_min_bounce + 1u
      && PtSharcTerminal(surface.worldPos, surface.normal, -ray.Direction, payload.t,
                         sqrt(max(prevSurface.material.roughness, 0.0)), brs.properties.IsSpecular(),
                         surface.material.diffuseAlbedo, cached);
#if defined(RTXDI_RESTIR_PT_INITIAL_SAMPLING)
#if defined(PT_WATER_SURFACE)
  const bool sharc_debug_hit = false;  // water never writes the PT debug views or their counters
#else
  // A21: the first path tree that reaches a hit writes the view; status 0 (SHARC) or 3 (Material) means still unset.
  const bool sharc_debug_hit = ctx.GetBounceDepth() == 2u && g_restir.debug_view == PT_DEBUG_VIEW_SHARC
                               && ud.debug_status == 0u;
  if (sharc_debug_hit) {
    // PT SHARC view (A18): 1 terminated by the cache, 2 cached but rejected by the terminal rule, 3 miss, 4 SHARC off.
    ud.debug_status = 4u;
    if (g_sharc_enabled != 0u) {
      const SharcParameters sharc = PtSharcParameters();
      const SharcHitData data = PtSharcHit(surface.worldPos, surface.normal, -ray.Direction, surface.material.diffuseAlbedo);
      HashGridKey key;
      const uint cell = HashGridFindEntry(sharc.hashGridData, data.positionWorld, data.normalWorld,
                                          sharc.hashGridParameters, key);
      const float cell_samples = cell != HASH_GRID_INVALID_CACHE_INDEX
          ? SharcGetVoxelData(sharc.resolvedBuffer, cell).accumulatedSampleNum : 0.0;
      ud.debug_word = uint4(cell, HashGridGetLevel(data.positionWorld, sharc.hashGridParameters), uint(cell_samples), 0u);
      float3 lookup = 0.0;
      if (terminal) {
        ud.debug_status = 1u;
        ud.debug_value = float4(cached, 1.0);
      } else if (SharcGetCachedRadiance(sharc, data, lookup, false) && all(isfinite(lookup))) {
        ud.debug_status = 2u;
        ud.debug_value = float4(max(lookup, 0.0), 1.0);
      } else {
        ud.debug_status = 3u;
      }
    }
  }
  if (ctx.GetBounceDepth() == 2u && g_restir.debug_view == PT_DEBUG_VIEW_MATERIAL && ud.debug_status == 3u) {
    // PT Material view (A18.2): 0 exact G-buffer, 1 world cache, 2 grey miss, 3 sky; word.y holds reflective/character bits.
    const RAB_Surface hit = ctx.GetIntersectionSurface();
    ud.debug_status = hit.material_source & 3u;
    ud.debug_value = float4(hit.material.diffuseAlbedo, hit.material.roughness);
    ud.debug_word = uint4(hit.material_index, hit.material_source >> 2, 0u, 0u);
  }
#endif
  if (terminal) {
    PtCount(25u);
    const float3 le = PtGiClampLe(cached, ctx.GetPathThroughput());
    ud.color_sum += le * ctx.GetPathThroughput();
    if (sharc_debug_hit) {
      InterlockedAdd(g_pt_counters[39u], uint(min(CalcLuminance(le * ctx.GetPathThroughput()), 65535.0) * 16.0));
    }
    ctx.RecordEmissiveLightSample(le, prevSurface, ptRandContext.initialRandomSamplerState);
    ctx.SetPathTermination(true);
    return;
  }
#else
  if (terminal && ctx.GetBounceDepth() < ctx.GetMaxPathBounce()) {
    PtCount(27u);
    ctx.MultiplyPathThroughput(0.0);
  }
#endif

  if (ShouldSampleNeeLights(ctx)) {
    float3 radiance_from_lights = 0.0;
    RTXDI_SampledLightData sampled_light_data = RTXDI_SampledLightData_CreateInvalidData();
    RAB_LightSample light_sample = RAB_EmptyLightSample();
    float nee_pdf = 0.0;
    float scatter_pdf = 0.0;
    PtSampleNee(surface, ptRandContext.initialRandomSamplerState, radiance_from_lights, sampled_light_data, nee_pdf,
                scatter_pdf, light_sample, true);
    if (nee_pdf != 0.0) {
      const float3 le = PtGiClampLe(radiance_from_lights, ctx.GetPathThroughput());
#if defined(RTXDI_RESTIR_PT_INITIAL_SAMPLING)
      ud.color_sum += le * ctx.GetPathThroughput();
      if (sharc_debug_hit) {
        InterlockedAdd(g_pt_counters[40u], uint(min(CalcLuminance(le * ctx.GetPathThroughput()), 65535.0) * 16.0));
      }
#endif
      ctx.RecordNeeLightSample(sampled_light_data, le, nee_pdf, scatter_pdf, light_sample,
                               ptRandContext.initialRandomSamplerState);
    }
  }
}

RTXDI_BrdfRaySample PtImportanceSampleBrdf(RAB_Surface surface, inout RTXDI_RandomSamplerState rng) {
  RTXDI_BrdfRaySample brs = RTXDI_EmptyBrdfRaySample();
  RTXDI_BrdfRaySampleProperties props = RTXDI_DefaultBrdfRaySampleProperties();
  float3 dir = 0.0;
  if (RAB_SurfaceImportanceSampleBsdf(surface, rng, dir, props)) {
    brs.outDirection = dir;
    brs.properties = props;
    brs.outPdf = RAB_SurfaceEvaluateBsdfPdf(surface, dir, props);
    brs.brdfTimesNoL = (surface.flags & 16u) != 0u  // A18: reflective diffuse-only continuation
        ? surface.material.diffuseAlbedo * Lambert(surface.normal, -dir)
        : RAB_SurfaceEvaluateBsdfTimesNoL(surface, dir, props.IsDelta());
  }
  return brs;
}

template<typename PTContextType>
bool PtDetermineOutgoingRay(inout RTXDI_PathTracerContext<PTContextType> ctx,
                            inout RTXDI_PathTracerRandomContext rng) {
  if (ctx.GetBounceDepth() != 1u) {
    ctx.SetBrdfRaySample(PtImportanceSampleBrdf(ctx.GetIntersectionSurface(), rng.replayRandomSamplerState));
    if (!ctx.ValidContinuationRayBrdfOverPdf()) return false;
    ctx.MultiplyPathThroughput(ctx.GetContinuationRayBrdfOverPdf());
    if (ShouldRunRussianRoulette(ctx)) {
      const float probability = CalculateRussianRouletteContinuationProbability(ctx.GetPathThroughput());
      if (RTXDI_GetNextRandom(rng.initialRandomSamplerState) > probability) return false;
      ctx.MultiplyPathThroughput(1.0 / probability);
      ctx.RecordRussianRouletteProbability(probability);
    }
    ctx.SetContinuationRay(PtContinuationRay(RAB_GetSurfaceGeoNormal(ctx.GetIntersectionSurface()),
                                             RAB_GetSurfaceWorldPos(ctx.GetIntersectionSurface()),
                                             ctx.GetBrdfRaySample().outDirection));
  }
  return ctx.AnalyzePathReconnectibilityBeforeTrace();
}

#if defined(RTXDI_RESTIR_PT_INITIAL_SAMPLING) && !defined(PT_WATER_SURFACE)
template<typename PTContextType>
void PtPrimaryNee(inout RTXDI_PathTracerContext<PTContextType> ctx, inout RTXDI_PathTracerRandomContext rng,
                  inout RAB_PathTracerUserData ud) {
  if (g_restir.direct_enabled == 0u) return;
  const RAB_Surface surface = ctx.GetIntersectionSurface();
  if (!RAB_IsSurfaceValid(surface)) return;
  uint cell = 0u;
  const bool tile_valid = PtGetLightTileCell(surface, cell);
  const RTXDI_InitialSamplingMisData mis_data = RTXDI_ComputeInitialSamplingMisData(g_restir.nee);
  RTXDI_DIReservoir local_reservoir = RTXDI_EmptyDIReservoir();
  RAB_LightSample local_sample = RAB_EmptyLightSample();
  // Candidates come from this pixel's game tile cell: the only lights with a non-zero target
  // (world_rtxdi_initial_di.cs_6_5.hlsl:132-167). Characters take lamps from the final pass's lamp-only estimator in
  // both character modes (A22 C1).
  if (tile_valid && g_restir.nee.numLocalLightSamples != 0u && !surface.material.character) {
    const LightIndexData tile = g_pt_tiles[cell];
    const uint point_count = min(tile.pointLightCount, 63u);
    const uint spot_count = min(tile.spotLightCount, 63u);
    const uint entry_count = point_count + spot_count;
    uint usable_count = 0u;
    for (uint entry = 0u; entry < entry_count; ++entry) {
      const bool point_entry = entry < point_count;
      const int listed = point_entry ? tile.pointLightIndices[entry] : tile.spotLightIndices[entry - point_count];
      usable_count += PtTileEntryUsable(listed, point_entry) ? 1u : 0u;
    }
    if (usable_count != 0u) {
      for (uint candidate = 0u; candidate < g_restir.nee.numLocalLightSamples; ++candidate) {
        const uint selected_rank = min(uint(RTXDI_GetNextRandom(rng.initialRandomSamplerState) * float(usable_count)),
                                       usable_count - 1u);
        uint light_index = g_lamp_count;
        uint rank = 0u;
        for (uint entry = 0u; entry < entry_count; ++entry) {
          const bool point_entry = entry < point_count;
          const int listed = point_entry ? tile.pointLightIndices[entry] : tile.spotLightIndices[entry - point_count];
          const bool usable = PtTileEntryUsable(listed, point_entry);
          if (usable && rank == selected_rank) light_index = uint(listed);
          rank += usable ? 1u : 0u;
        }
        const RAB_LightInfo light_info = RAB_LoadLightInfo(light_index, false);
        float2 uv;
        if (g_restir.isfast_light_jitter != 0u) {
          uv = asfloat(g_pt_noise.Load(int3(surface.pixel.x % 128u, surface.pixel.y % 128u + 128u * (g_frame % 32u), 0)).xy);
        } else {
          uv = RTXDI_RandomlySelectLocalLightUV(rng.initialRandomSamplerState);
        }
        RTXDI_StreamLocalLightAtUVIntoReservoir(rng.initialRandomSamplerState, mis_data, surface,
            g_restir.nee.brdfCutoff, mis_data.localLightMisWeight, light_index, uv, float(usable_count),
            light_info, local_reservoir, local_sample);
      }
      RTXDI_FinalizeResampling(local_reservoir, 1.0, mis_data.numMisSamples);
      local_reservoir.M = 1;
    }
  }
  RTXDI_DIReservoir sun_reservoir = RTXDI_EmptyDIReservoir();
  RAB_LightSample sun_sample = RAB_EmptyLightSample();
  // Game-toon characters take their sun from the game lighting and its shading-time RT visibility, so only local lamps are
  // streamed here.
  if (!(surface.material.character && g_restir.char_toon != 0u)) {
    // One infinite-light sample (InitialSampling.hlsli:375-399), keeping the IS-FAST uv source.
    const RAB_LightInfo sun_info = RAB_LoadLightInfo(g_lamp_count, false);
    if (sun_info.index != RTXDI_LIGHT_TABLE_INVALID_INDEX) {
      const float2 sun_uv = (g_restir.isfast_light_jitter != 0u)
          ? asfloat(g_pt_noise.Load(int3(surface.pixel.x % 128u, surface.pixel.y % 128u + 128u * (g_frame % 32u), 0)).xy)
          : RTXDI_RandomlySelectInfiniteLightUV(rng.initialRandomSamplerState);
      RTXDI_StreamInfiniteLightAtUVIntoReservoir(rng.initialRandomSamplerState, sun_info, surface, g_lamp_count, sun_uv,
                                                 1.0, sun_reservoir, sun_sample);
      RTXDI_FinalizeResampling(sun_reservoir, 1.0, sun_reservoir.M);
      sun_reservoir.M = 1;
    }
  }
  RAB_LightSample light_sample = RAB_EmptyLightSample();
  const RTXDI_DIReservoir reservoir =
      PtCombineLocalAndSun(local_reservoir, local_sample, sun_reservoir, sun_sample, rng.initialRandomSamplerState, light_sample);
  if (reservoir.weightSum <= 0.0 || light_sample.solidAnglePdf <= 0.0) return;
  const uint light_index = reservoir.lightData & RTXDI_DIReservoir_LightIndexMask;
  RAB_Surface visible_surface = surface;
  visible_surface.flags |= light_sample.sun ? 4u : 0u;
  float3 radiance_from_lights = RAB_GetReflectedBsdfRadianceForSurface(light_sample.position, light_sample.radiance, visible_surface);
  radiance_from_lights *= PtLightVisibility(visible_surface, light_sample.position);
  radiance_from_lights *= RTXDI_GetDIReservoirInvPdf(reservoir) / light_sample.solidAnglePdf;
  if (all(radiance_from_lights <= 0.0)) return;
  RTXDI_SampledLightData sampled_light_data = RTXDI_SampledLightData_CreateInvalidData();
  RTXDI_SampledLightData_SetLightData(sampled_light_data, light_index);
  sampled_light_data.uvData = reservoir.uvData;
  // Lin 2026 §6.1: primary NEE as a d = 2 path in the unified reservoir; same stream as RecordNeeLightSample case 3
  // (InitialSamplingPathTracerContext.hlsli:196-247) one vertex earlier.
  ud.color_sum += radiance_from_lights * ctx.GetPathThroughput();
  ctx.RISStreamPathSample(2u, radiance_from_lights, rng.initialRandomSamplerState, 2u, true, 1.0 / reservoir.weightSum,
                          float3(1.f / 0.f, asfloat(sampled_light_data.lightData), asfloat(sampled_light_data.uvData)),
                          RAB_LightSampleSolidAnglePdf(light_sample));
  PtCount(light_sample.sun ? 31u : 30u);
}
#endif

template<typename PTContextType>
void RAB_PathTrace(inout RTXDI_PathTracerContext<PTContextType> ctx, inout RTXDI_PathTracerRandomContext rng,
                   inout RAB_PathTracerUserData ud) {
#if defined(RTXDI_RESTIR_PT_INITIAL_SAMPLING) && !defined(PT_WATER_SURFACE)
  PtPrimaryNee(ctx, rng, ud);
#endif
  while (ctx.GetBounceDepth() <= ctx.GetMaxPathBounce()) {
    ctx.BeginPathState();
    if (!PtDetermineOutgoingRay(ctx, rng)) break;
    const RAB_Surface prev = ctx.GetIntersectionSurface();
    ctx.SetTraceResult(PtTraceNextBounce(ctx.GetContinuationRay()));
#if defined(RTXDI_RESTIR_PT_INITIAL_SAMPLING) && !defined(PT_WATER_SURFACE)
    if (ctx.GetBounceDepth() == 2u) {
      PtCount(33u);
      if (RAB_IsValidHit(ctx.GetTraceResult())) PtCount(34u);
    }
#endif
    if (RAB_IsValidHit(ctx.GetTraceResult())) {
      HandleHit(ctx, rng, prev, ud, ud.pixel);
      if (ctx.IsPathTerminated()) break;
    } else {
      ctx.RecordPathRadianceMiss(rng.initialRandomSamplerState);
      const float3 le = PtGiClampLe(RAB_GetEnvironmentRadiance(ctx.GetContinuationRay().Direction), ctx.GetPathThroughput());
#if defined(RTXDI_RESTIR_PT_INITIAL_SAMPLING)
      ud.color_sum += le * ctx.GetPathThroughput();
#endif
      ctx.RecordEnvironmentMapLightSample(le, prev, rng.initialRandomSamplerState);
      break;
    }
    ctx.IncreaseBounceDepth();
  }
}

#endif  // FALCOM_WORLD_PT_PATH_TRACER_HLSLI
