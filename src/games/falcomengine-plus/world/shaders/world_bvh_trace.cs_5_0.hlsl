// world_bvh_trace.cs_5_0.hlsl - M4 primary-ray trace debug pass.
//
// Modes: 7 BVH Trace (Shaded), 8 BVH Trace (Instance ID), 9 Depth Compare.
// One ray per pixel from the camera origin toward the reversed-Z far plane
// (z = 0). Per-group counters are reduced through groupshared memory, then
// written to the stats buffer with one atomic set per group.
//
// Depth Compare reconstructs the game's surface for the same pixel from the
// lighting pass depth (t9, reversed-Z device depth) and the same inverse
// view-projection the ray uses, then compares the distance along the ray with
// the BVH hit distance:
//   match       |bvh - game| <= max(2% of game distance, 5 cm)
//   missing     game surface in front of the BVH hit, or no BVH hit
//               (characters, foliage, alpha-tested and anything not admitted)
//   extra       BVH hit in front of the game surface (geometry the game does
//               not show there: ghosts, stale or wrongly placed instances)
//   extra_sky   BVH hit where the game shows sky
//   far         game surface (or sky-pixel BVH hit) beyond the compare range,
//               where the region TLAS is not complete; not judged
//   sky         both empty
//   no_depth    no current-frame game depth bound
//
// Every view traces from the camera, so what the game camera shows applies
// (world_bvh_trace.hlsli): camera_hidden counts pixels whose nearest BVH
// surface the game camera does not show, and g_camera_view.x hides those
// surfaces (the ray continues behind them).

#include "world_bvh_trace.hlsli"

Texture2D<float4> g_game_depth : register(t9);

RWTexture2D<float4> g_trace_output : register(u0);
RWStructuredBuffer<uint> g_trace_stats : register(u1);

cbuffer cb_trace : register(b13)
{
    float4x4 g_view_proj_inv;
    float4 g_camera_position;
    uint g_mode;
    uint g_width;
    uint g_height;
    float g_compare_range;
    float4 g_depth_rect;  // valid game depth area in texels: x, y, width, height
    float4 g_inspect;     // x, y: pixel whose hit is reported; z > 0.5: report on
    float4 g_camera_view; // x > 0.5: hide what the game camera does not show; y: disableMapObjNearFade_g
    uint4 g_dynamic;      // x: deforming objects bound at t11 (deform_live.hpp)
};

#define TRACE_STATS_COUNT 15u
#define STAT_RAYS 0u
#define STAT_HITS 1u
#define STAT_MISSES 2u
#define STAT_INVALID_REFS 3u
#define STAT_STACK_OVERFLOW 4u
#define STAT_TRIANGLE_TESTS 5u
#define STAT_MAX_STACK_DEPTH 6u
#define STAT_CMP_MATCH 7u
#define STAT_CMP_MISSING 8u
#define STAT_CMP_EXTRA 9u
#define STAT_CMP_EXTRA_SKY 10u
#define STAT_CMP_FAR 11u
#define STAT_CMP_SKY 12u
#define STAT_CMP_NO_DEPTH 13u
#define STAT_CAMERA_HIDDEN 14u
// One pixel reports its hit after the counters:
// flags (1 traced, 2 hit, 4 hidden from the game camera), instance, mesh, prim, t, position xyz, compare class.
#define TRACE_INSPECT_BASE 15u

#define TRACE_MODE_SHADED 7u
#define TRACE_MODE_INSTANCE 8u
#define TRACE_MODE_DEPTH_COMPARE 9u

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

float Lambert(WorldTraceHit hit, float3 direction)
{
    // Camera-facing geometric normal.
    const float3 normal = dot(hit.normal, direction) > 0.0 ? -hit.normal : hit.normal;
    return 0.35 + 0.65 * saturate(dot(normal, kDebugLightDirection));
}

// Classifies one pixel and returns its debug color; `stat` receives the
// STAT_CMP_* counter to increment.
float3 CompareWithGameDepth(float2 uv, float2 ndc, float3 origin, float3 direction, WorldTraceHit hit, out uint stat)
{
    uint depth_width = 0u;
    uint depth_height = 0u;
    g_game_depth.GetDimensions(depth_width, depth_height);
    if (depth_width == 0u || depth_height == 0u || g_depth_rect.z <= 0.0 || g_depth_rect.w <= 0.0)
    {
        stat = STAT_CMP_NO_DEPTH;
        return float3(0.5, 0.0, 0.5);
    }

    const float2 texel = g_depth_rect.xy + uv * g_depth_rect.zw;
    const int2 pixel = clamp(int2(texel), int2(0, 0), int2(int(depth_width) - 1, int(depth_height) - 1));
    const float device_depth = g_game_depth.Load(int3(pixel, 0)).x;

    const bool bvh_hit = hit.prim != 0xFFFFFFFFu;
    const float bvh_distance = bvh_hit ? length(hit.position - origin) : 0.0;
    const float shade = bvh_hit ? Lambert(hit, direction) : 1.0;

    // Reversed-Z: the far plane (sky) is device depth 0.
    if (!(device_depth > 0.0))
    {
        if (!bvh_hit)
        {
            stat = STAT_CMP_SKY;
            return float3(0.02, 0.02, 0.05);
        }
        if (bvh_distance > g_compare_range)
        {
            stat = STAT_CMP_FAR;
            return float3(0.25, 0.25, 0.25) * shade;
        }
        stat = STAT_CMP_EXTRA_SKY;
        return float3(1.0, 0.55, 0.0) * shade;
    }

    const float4 game_h = mul(float4(ndc, device_depth, 1.0), g_view_proj_inv);
    const float3 game_position = game_h.xyz / game_h.w;
    const float game_distance = length(game_position - origin);
    if (game_distance > g_compare_range)
    {
        stat = STAT_CMP_FAR;
        return float3(0.25, 0.25, 0.25) * shade;
    }
    if (!bvh_hit)
    {
        stat = STAT_CMP_MISSING;
        return float3(0.1, 0.3, 1.0);
    }

    const float difference = bvh_distance - game_distance;
    const float tolerance = max(0.02 * game_distance, 0.05);
    if (abs(difference) <= tolerance)
    {
        stat = STAT_CMP_MATCH;
        return float3(0.2, 0.9, 0.25) * shade;
    }
    if (difference < 0.0)
    {
        stat = STAT_CMP_EXTRA;
        return float3(1.0, 0.1, 0.1) * shade;
    }
    stat = STAT_CMP_MISSING;
    return float3(0.1, 0.3, 1.0) * shade;
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

        g_trace_dynamic_count = g_dynamic.x;
        WorldCameraView view;
        view.hide = g_camera_view.x > 0.5;
        view.floor_value = g_camera_view.y;
        WorldTraceCounters counters;
        const WorldTraceHit hit = TraceWorldClosest(origin, direction, 0.001, 1e30, view, counters);

        float3 color = float3(0.02, 0.02, 0.03);
        uint compare_stat = 0xFFFFFFFFu;
        if (g_mode == TRACE_MODE_DEPTH_COMPARE)
        {
            uint stat = STAT_CMP_NO_DEPTH;
            color = CompareWithGameDepth(uv, ndc, origin, direction, hit, stat);
            InterlockedAdd(gs_stats[stat], 1u);
            compare_stat = stat;
        }
        else if (hit.prim != 0xFFFFFFFFu)
        {
            if (g_mode == TRACE_MODE_SHADED)
            {
                color = 0.85 * Lambert(hit, direction);
            }
            else
            {
                color = IdColor(hit.instance);
            }
        }
        g_trace_output[dtid.xy] = float4(color, 1.0);

        // Inspect: a single thread writes these slots, plain stores.
        if (g_inspect.z > 0.5 && dtid.x == (uint)g_inspect.x && dtid.y == (uint)g_inspect.y)
        {
            g_trace_stats[TRACE_INSPECT_BASE + 0u] = (hit.prim != 0xFFFFFFFFu ? 3u : 1u) | (counters.camera_hidden != 0u ? 4u : 0u);
            g_trace_stats[TRACE_INSPECT_BASE + 1u] = hit.instance;
            g_trace_stats[TRACE_INSPECT_BASE + 2u] = hit.mesh;
            g_trace_stats[TRACE_INSPECT_BASE + 3u] = hit.prim;
            g_trace_stats[TRACE_INSPECT_BASE + 4u] = asuint(hit.t);
            g_trace_stats[TRACE_INSPECT_BASE + 5u] = asuint(hit.position.x);
            g_trace_stats[TRACE_INSPECT_BASE + 6u] = asuint(hit.position.y);
            g_trace_stats[TRACE_INSPECT_BASE + 7u] = asuint(hit.position.z);
            g_trace_stats[TRACE_INSPECT_BASE + 8u] = compare_stat;
        }

        InterlockedAdd(gs_stats[STAT_RAYS], 1u);
        InterlockedAdd(gs_stats[STAT_HITS], counters.hits);
        InterlockedAdd(gs_stats[STAT_MISSES], counters.misses);
        InterlockedAdd(gs_stats[STAT_INVALID_REFS], counters.invalid_refs);
        InterlockedAdd(gs_stats[STAT_STACK_OVERFLOW], counters.stack_overflow);
        InterlockedAdd(gs_stats[STAT_TRIANGLE_TESTS], counters.triangle_tests);
        InterlockedMax(gs_stats[STAT_MAX_STACK_DEPTH], counters.max_stack_depth);
        InterlockedAdd(gs_stats[STAT_CAMERA_HIDDEN], counters.camera_hidden);
    }

    GroupMemoryBarrierWithGroupSync();
    if (group_index == 0u)
    {
        [unroll]
        for (uint i = 0u; i < TRACE_STATS_COUNT; ++i)
        {
            if (i == STAT_MAX_STACK_DEPTH) InterlockedMax(g_trace_stats[i], gs_stats[i]);
            else InterlockedAdd(g_trace_stats[i], gs_stats[i]);
        }
    }
}
