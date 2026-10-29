// world_rt12_rtgi.cs_6_5.hlsl - M2: RTGI on D3D12 (DXR 1.1 inline ray queries, rt12_backend.hpp RunRtgi).
// The per-pixel code is world_rtgi.cs_5_0.hlsl, included with RTGI_DXR: its inputs are the shared copies made by
// world_rt12_rtgi_pack.cs_5_0.hlsl (the trace-size MRT normal, read at the representative pixel by dividing by the trace
// divisor) and its rays go through TraceWorldRayDxr. Characters are traced as dynamic objects (t30) when g_rtgi_sun.y > 0.
#define RTGI_DXR
#define RT12_TLAS_REGISTER t29
#include "world_rt12_trace.hlsli"

Texture2D<uint4> g_rt12_normal : register(t17);
Texture2D<uint4> g_rt12_noise : register(t18);
RWTexture2D<uint> g_rt12_rtgi_stats : register(u1);

static uint g_rt12_d = 1u;

struct Rt12Normal
{
    uint unused;
    uint4 Load(int3 p) { return g_rt12_normal.Load(int3(p.xy / (int)g_rt12_d, 0)); }
};
struct Rt12Noise
{
    uint unused;
    float2 Load(int4 p) { return asfloat(g_rt12_noise.Load(int3(p.x, p.y + 128 * p.z, 0)).xy); }
};
static Rt12Normal g_mrt_normal;
static Rt12Noise g_isfast_noise;

WorldTraceHit RtgiTraceClosestDxr(float3 origin, float3 direction, float t_min, float t_max, WorldCameraView view,
                                  out WorldTraceCounters counters)
{
    return TraceWorldRayDxr(origin, direction, t_min, t_max, false, view, counters);
}

WorldTraceHit RtgiTraceAnyDxr(float3 origin, float3 direction, float t_min, float t_max, WorldCameraView view,
                              out WorldTraceCounters counters)
{
    return TraceWorldRayDxr(origin, direction, t_min, t_max, true, view, counters);
}

#define RTGI_TRACE_CLOSEST RtgiTraceClosestDxr
#define RTGI_TRACE_ANY RtgiTraceAnyDxr
#define RTGI_STATS_AT(i) g_rt12_rtgi_stats[uint2((i), 0u)]
#define RTGI_MAIN_PROLOGUE g_rt12_d = max((uint)g_rtgi_params.w, 1u); g_rt12_dynamic_mask = (uint)g_rtgi_sun.y > 0u ? RT12_MASK_DYNAMIC : 0u;
#include "world_rtgi.cs_5_0.hlsl"
