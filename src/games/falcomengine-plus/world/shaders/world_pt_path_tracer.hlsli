#pragma once
// This software contains source code provided by NVIDIA Corporation.
// Port of NVIDIA FullSample LightingPasses/RtxdiApplicationBridge/PathTracer/RAB_PathTracer.hlsli. PSR, debug path
// recording and reordering are not ported; SHARC terminal radiance, the game's NEE and the water hit test replace them.
#ifndef FALCOM_WORLD_PT_PATH_TRACER_HLSLI
#define FALCOM_WORLD_PT_PATH_TRACER_HLSLI

#include "world_pt_bridge.hlsli"
#include <Rtxdi/PT/PathTracerContext.hlsli>
#include <Rtxdi/PT/PathTracerRandomContext.hlsli>

bool IsEmissiveSamplingEnabled() { return g_sharc_enabled != 0u; }
bool IsNeeSamplingEnabled() { return true; }

template<typename PTContextType>
bool ShouldRunRussianRoulette(inout RTXDI_PathTracerContext<PTContextType> ctx) {
  return g_roulette != 0u && ctx.GetBounceDepth() - 1u >= g_roulette_min_bounce;
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
  if (PtSharcTerminal(surface.worldPos, surface.normal, -ctx.GetContinuationRay().Direction, ctx.GetTraceResult().t,
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
               const RAB_Surface prevSurface, uint2 pixel) {
  const RAB_RayPayload payload = ctx.GetTraceResult();
  const RayDesc ray = ctx.GetContinuationRay();
  const RAB_Surface surface = PtHitSurface(payload, ray.Direction);
  ctx.RecordPathIntersection(surface);

  if (ctx.IsPathTerminated()) return;

#if defined(PT_WATER_SURFACE)
  if (ctx.GetBounceDepth() == 2u && PtWaterHit(pixel, payload.position, surface.normal, surface.viewDir)) {
    ctx.RecordEnvironmentMapLightSample(PtSky(reflect(-surface.viewDir, surface.normal)), prevSurface,
                                        ptRandContext.initialRandomSamplerState);
    ctx.SetPathTermination(true);
    return;
  }
#endif

  const RTXDI_BrdfRaySample brs = ctx.GetBrdfRaySample();
  float3 cached = 0.0;
  const bool terminal = PtSharcTerminal(surface.worldPos, surface.normal, -ray.Direction, payload.t,
                                        sqrt(max(prevSurface.material.roughness, 0.0)), brs.properties.IsSpecular(),
                                        surface.material.diffuseAlbedo, cached);
#if defined(RTXDI_RESTIR_PT_INITIAL_SAMPLING)
  if (terminal) {
    PtCount(25u);
    ctx.RecordEmissiveLightSample(cached, prevSurface, ptRandContext.initialRandomSamplerState);
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
                scatter_pdf, light_sample);
    if (nee_pdf != 0.0) {
      ctx.RecordNeeLightSample(sampled_light_data, radiance_from_lights, nee_pdf, scatter_pdf, light_sample,
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
    brs.brdfTimesNoL = RAB_SurfaceEvaluateBsdfTimesNoL(surface, dir, props.IsDelta());
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

template<typename PTContextType>
void RAB_PathTrace(inout RTXDI_PathTracerContext<PTContextType> ctx, inout RTXDI_PathTracerRandomContext rng,
                   inout RAB_PathTracerUserData ud) {
  while (ctx.GetBounceDepth() <= ctx.GetMaxPathBounce()) {
    ctx.BeginPathState();
    if (!PtDetermineOutgoingRay(ctx, rng)) break;
    const RAB_Surface prev = ctx.GetIntersectionSurface();
    ctx.SetTraceResult(PtTraceNextBounce(ctx.GetContinuationRay()));
    if (RAB_IsValidHit(ctx.GetTraceResult())) {
      HandleHit(ctx, rng, prev, ud.pixel);
      if (ctx.IsPathTerminated()) break;
    } else {
      ctx.RecordPathRadianceMiss(rng.initialRandomSamplerState);
      ctx.RecordEnvironmentMapLightSample(RAB_GetEnvironmentRadiance(ctx.GetContinuationRay().Direction), prev,
                                          rng.initialRandomSamplerState);
      break;
    }
    ctx.IncreaseBounceDepth();
  }
}

#endif  // FALCOM_WORLD_PT_PATH_TRACER_HLSLI
