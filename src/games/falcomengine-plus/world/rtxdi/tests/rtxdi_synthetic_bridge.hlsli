// Synthetic API stubs for compilation only; these do not describe Falcom rendering.
#include <Rtxdi/RtxdiParameters.h>
#include <Rtxdi/Utils/RandomSamplerState.hlsli>
struct RAB_Surface { float value; };
struct RAB_Material { float value; };
struct RAB_LightInfo { float value; };
struct RAB_LightSample { float value; };
RAB_Surface RAB_EmptySurface() { return (RAB_Surface)0; }
RAB_Material RAB_GetMaterial(RAB_Surface s) { return (RAB_Material)0; }
bool RAB_AreMaterialsSimilar(RAB_Material a, RAB_Material b) { return true; }
bool RAB_IsSurfaceValid(RAB_Surface s) { return true; }
float3 RAB_GetSurfaceWorldPos(RAB_Surface s) { return 0; }
float3 RAB_GetSurfaceNormal(RAB_Surface s) { return float3(0, 0, 1); }
float RAB_GetSurfaceLinearDepth(RAB_Surface s) { return 1; }
RAB_Surface RAB_GetGBufferSurface(int2 p, bool previous) { return (RAB_Surface)0; }
int2 RAB_ClampSamplePositionIntoView(int2 p, bool previous) { return p; }
RAB_LightInfo RAB_EmptyLightInfo() { return (RAB_LightInfo)0; }
RAB_LightInfo RAB_LoadLightInfo(uint index, bool previous) { return (RAB_LightInfo)0; }
int RAB_TranslateLightIndex(uint index, bool previous) { return int(index); }
RAB_LightSample RAB_EmptyLightSample() { return (RAB_LightSample)0; }
RAB_LightSample RAB_SamplePolymorphicLight(RAB_LightInfo light, RAB_Surface surface, float2 uv) { return (RAB_LightSample)0; }
float RAB_GetLightSampleTargetPdfForSurface(RAB_LightSample light, RAB_Surface surface) { return 1; }
float RAB_LightSampleSolidAnglePdf(RAB_LightSample light) { return 1; }
bool RAB_IsAnalyticLightSample(RAB_LightSample light) { return true; }
void RAB_GetLightDirDistance(RAB_Surface surface, RAB_LightSample light, out float3 direction, out float distance) { direction = 0; distance = 1; }
float RAB_SurfaceEvaluateBrdfPdf(RAB_Surface surface, float3 direction) { return 1; }
bool RAB_SurfaceImportanceSampleBrdf(RAB_Surface surface, inout RTXDI_RandomSamplerState rng, out float3 direction) { direction = 0; return false; }
bool RAB_TraceRayForLocalLight(float3 origin, float3 direction, float tmin, float tmax, out uint index, out float2 uv) { index = 0; uv = 0; return false; }
float RAB_EvaluateLocalLightSourcePdf(uint index) { return 1; }
float RAB_EvaluateEnvironmentMapSamplingPdf(float3 direction) { return 1; }
float2 RAB_GetEnvironmentMapRandXYFromDir(float3 direction) { return 0; }
bool RAB_GetConservativeVisibility(RAB_Surface surface, RAB_LightSample light) { return true; }
bool RAB_GetTemporalConservativeVisibility(RAB_Surface current, RAB_Surface previous, RAB_LightSample light) { return true; }

