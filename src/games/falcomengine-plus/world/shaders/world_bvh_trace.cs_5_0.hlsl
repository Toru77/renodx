// world_bvh_trace.cs_5_0.hlsl - M4 primary-ray trace debug pass.
//
// Modes: 7 BVH Trace (Shaded), 8 BVH Trace (Instance ID).
// One ray per pixel from the camera origin toward the reversed-Z far plane
// (z = 0). Per-group counters are reduced through groupshared memory, then
// written to the stats buffer with one atomic set per group.

#include "world_bvh_trace.hlsli"

RWTexture2D<float4> g_trace_output : register(u0);
RWStructuredBuffer<uint> g_trace_stats : register(u1);

cbuffer cb_trace : register(b13)
{
    float4x4 g_view_proj_inv;
    float4 g_camera_position;
    uint g_mode;
    uint g_width;
    uint g_height;
    uint g_spare0;
};

#define TRACE_STATS_COUNT 7u

groupshared uint gs_stats[TRACE_STATS_COUNT];

static const float3 kDebugLightDirection = normalize(float3(0.4, 0.8, 0.45));

float3 HueToRgb(float hue)
{
    const float3 k = float3(1.0, 2.0 / 3.0, 1.0 / 3.0);
    const float3 p = abs(frac(hue + k) * 6.0 - 3.0);
    return saturate(p - 1.0);
}

float3 IdColor(uint id)
{
    return HueToRgb(frac(float(id) * 0.618034));
}

[numthreads(8, 8, 1)]
void main(uint3 dtid : SV_DispatchThreadID, uint group_index : SV_GroupIndex)
{
    if (group_index < TRACE_STATS_COUNT) gs_stats[group_index] = 0u;
    GroupMemoryBarrierWithGroupSync();

    const bool valid = dtid.x < g_width && dtid.y < g_height;
    if (valid)
    {
        const float2 uv = (float2(dtid.xy) + 0.5) / float2(g_width, g_height);
        const float2 ndc = float2(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0);
        const float4 far_h = mul(float4(ndc, 0.0, 1.0), g_view_proj_inv);
        const float3 far_point = far_h.xyz / far_h.w;
        const float3 origin = g_camera_position.xyz;
        const float3 direction = normalize(far_point - origin);

        WorldTraceCounters counters;
        const WorldTraceHit hit = TraceWorldClosest(origin, direction, 0.001, 1e30, counters);

        float3 color = float3(0.02, 0.02, 0.03);
        if (hit.prim != 0xFFFFFFFFu)
        {
            if (g_mode == 7u)
            {
                // Match the raster shaded mode with a camera-facing geometric normal.
                const float3 normal = dot(hit.normal, direction) > 0.0 ? -hit.normal : hit.normal;
                const float lambert = 0.35 + 0.65 * saturate(dot(normal, kDebugLightDirection));
                color = 0.85 * lambert;
            }
            else
            {
                color = IdColor(hit.instance);
            }
        }
        g_trace_output[dtid.xy] = float4(color, 1.0);

        InterlockedAdd(gs_stats[0], 1u);
        InterlockedAdd(gs_stats[1], counters.hits);
        InterlockedAdd(gs_stats[2], counters.misses);
        InterlockedAdd(gs_stats[3], counters.invalid_refs);
        InterlockedAdd(gs_stats[4], counters.stack_overflow);
        InterlockedAdd(gs_stats[5], counters.triangle_tests);
        InterlockedMax(gs_stats[6], counters.max_stack_depth);
    }

    GroupMemoryBarrierWithGroupSync();
    if (group_index == 0u)
    {
        [unroll]
        for (uint i = 0u; i < TRACE_STATS_COUNT; ++i)
        {
            if (i == 6u) InterlockedMax(g_trace_stats[i], gs_stats[i]);
            else InterlockedAdd(g_trace_stats[i], gs_stats[i]);
        }
    }
}
