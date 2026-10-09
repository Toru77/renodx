// world_rtao.cs_5_0.hlsl - RTAO round 1 (world/docs/ROADMAP.md, RTAO).
//
// One thread per pixel over the game depth size, dispatched inline in the lighting
// hook. Cosine-hemisphere rays are traced through the live BVH (world_bvh_trace.hlsli
// with dynamic count 0: characters and water do not occlude in round 1). The AO is
// written to u0 as a 0..255 texel (255 = neutral). Per-group counters are reduced in
// groupshared memory and added to u1 (RTAO_STAT_*).
//
// Camera: the game scene CBV bound as b0 (viewInv_g, viewProjInv_g), not g_state.camera.
// Sampling: IS-FAST blue noise (t18, Load only, 128x128x32), or IGN when it is not loaded.

#include "world_bvh_trace.hlsli"
#include "../../include/mrt_normal.hlsli"

Texture2D<float4> g_game_depth : register(t9);
Texture2D<uint4> g_mrt_normal : register(t17);
Texture3D<float2> g_isfast_noise : register(t18);

RWTexture2D<uint> g_rtao_ao : register(u0);
RWStructuredBuffer<uint> g_rtao_stats : register(u1);
// Raw AO for the temporal pass (round 2): valid pixels hold the trace AO, neutral-invalid pixels hold -1.
RWTexture2D<float> g_rtao_raw : register(u2);

cbuffer cb_scene : register(b0)
{
    float4x4 viewInv_g : packoffset(c4);
    float4x4 viewProjInv_g : packoffset(c20);
    float2 resolutionScaling_g : packoffset(c49);
};

// Push constants (rtao.hpp fills these; 20 floats).
cbuffer cb_rtao : register(b12)
{
    float4 g_rtao_params : packoffset(c0);    // radius, ray_max, strength, normal_bias
    float4 g_rtao_sampling : packoffset(c1);  // spp, isfast flag (1 loaded), slice base, seed
    float4 g_rtao_fade : packoffset(c2);      // fade start_eff, fade end_eff, 0, 0
    float4 g_rtao_region : packoffset(c3);    // region min xyz, region size
    float4 g_rtao_size : packoffset(c4);      // width, height, dynamic count (0), output_raw (1 = write u2, not u0)
};

#define RTAO_STAT_RAYS 0u
#define RTAO_STAT_HITS 1u
#define RTAO_STAT_PIXELS 2u
#define RTAO_STAT_AO_SUM 3u
#define RTAO_STAT_SKY 4u
#define RTAO_STAT_NORMAL 5u
#define RTAO_STAT_REGION 6u
#define RTAO_STAT_INVALID_REFS 7u
#define RTAO_STAT_STACK_OVERFLOW 8u
#define RTAO_STAT_SCALED 9u
#define RTAO_STAT_COUNT 10u

#define RTAO_DISC_BASE 16u
#define RTAO_DISC_COUNT 8u

groupshared uint gs_stats[RTAO_STAT_COUNT];
groupshared uint gs_disc[RTAO_DISC_COUNT];

void TracePixel(uint2 px)
{
    const float ray_max = g_rtao_params.y;
    const float radius = min(g_rtao_params.x, ray_max);
    const uint spp = max((uint)g_rtao_sampling.x, 1u);
    const float3 region_min = g_rtao_region.xyz;
    const float region_size = g_rtao_region.w;

    float ao = 1.0;
    bool traced = false;
    const float depth = g_game_depth.Load(int3(px, 0)).x;
    const float3 N = DecodeFalcomMrtNormal(g_mrt_normal.Load(int3(px, 0)).xy);

    if (any(resolutionScaling_g != float2(1.0, 1.0)))
    {
        InterlockedAdd(gs_stats[RTAO_STAT_SCALED], 1u);
    }
    else if (depth <= 0.0)
    {
        InterlockedAdd(gs_stats[RTAO_STAT_SKY], 1u);
    }
    else if (dot(N, N) < 0.5)
    {
        InterlockedAdd(gs_stats[RTAO_STAT_NORMAL], 1u);
    }
    else
    {
        const float2 uv = (float2(px) + 0.5) / g_rtao_size.xy;
        const float4 world_h = mul(float4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, depth, 1.0), viewProjInv_g);
        const float3 P = world_h.xyz / world_h.w;
        const float3 region_lo = region_min + ray_max;
        const float3 region_hi = region_min + region_size - ray_max;
        if (!(all(P >= region_lo) && all(P <= region_hi)))
        {
            InterlockedAdd(gs_stats[RTAO_STAT_REGION], 1u);
        }
        else
        {
            traced = true;
            const float3 camera = float3(viewInv_g._m30, viewInv_g._m31, viewInv_g._m32);
            const float distance_to_camera = length(P - camera);
            // Two-Sided discovery (diagnostic, g_rtao_fade.z = 1): one primary ray camera -> P. Only a hit at the
            // G-buffer distance (2% or 5 cm) counts: winding back x normal toward camera x alpha-tested.
            if (g_rtao_fade.z > 0.5)
            {
                const float3 view_dir = (P - camera) / distance_to_camera;
                WorldCameraView disc_view = {true, 1.0};
                WorldTraceCounters disc_counters;
                const WorldTraceHit disc_hit = TraceWorldClosest(camera, view_dir, 1e-3, 1.05 * distance_to_camera, disc_view, disc_counters);
                if (disc_hit.prim != 0xFFFFFFFFu && abs(disc_hit.t - distance_to_camera) <= max(0.02 * distance_to_camera, 0.05))
                {
                    const uint back = disc_hit.facing > 0.0 ? 1u : 0u;
                    const uint toward = dot(N, -view_dir) > 0.0 ? 1u : 0u;
                    const uint alpha = disc_hit.material != 0u ? 1u : 0u;
                    InterlockedAdd(gs_disc[back * 4u + toward * 2u + alpha], 1u);
                }
            }
            const float3 origin = P + N * g_rtao_params.w;
            const float3 up = abs(N.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
            const float3 T = normalize(cross(up, N));
            const float3 B = cross(N, T);
            const WorldCameraView view = {true, 1.0};

            float occ = 0.0;
            uint rays = 0u;
            uint hits = 0u;
            uint invalid_refs = 0u;
            uint stack_overflow = 0u;
            for (uint i = 0u; i < spp; ++i)
            {
                float2 u;
                if (g_rtao_sampling.y > 0.5)
                {
                    u = g_isfast_noise.Load(int4(px.x % 128u, px.y % 128u, ((uint)g_rtao_sampling.z + i) % 32u, 0));
                }
                else
                {
                    const float seed = g_rtao_sampling.w;
                    const float2 q = float2(px) + seed + float(i) * 7.0;
                    u = frac(52.9829189 * frac(float2(dot(q, float2(0.06711056, 0.00583715)), dot(q + 47.0, float2(0.06711056, 0.00583715)))));
                }
                const float r = sqrt(u.x);
                const float phi = 6.2831853 * u.y;
                const float3 dir = T * (r * cos(phi)) + B * (r * sin(phi)) + N * sqrt(max(0.0, 1.0 - u.x));

                WorldTraceCounters counters;
                const WorldTraceHit hit = TraceWorldClosest(origin, dir, 1e-3, ray_max, view, counters);
                rays += 1u;
                invalid_refs += counters.invalid_refs;
                stack_overflow += counters.stack_overflow;
                if (hit.prim != 0xFFFFFFFFu)
                {
                    hits += 1u;
                    occ += 1.0 - smoothstep(radius, ray_max, hit.t);
                }
            }
            occ /= (float)spp;

            const float fade_start = g_rtao_fade.x;
            const float fade_end = g_rtao_fade.y;
            const float fade = saturate((fade_end - distance_to_camera) / max(fade_end - fade_start, 1e-4));
            ao = lerp(1.0, pow(saturate(1.0 - occ), g_rtao_params.z), fade);

            InterlockedAdd(gs_stats[RTAO_STAT_RAYS], rays);
            InterlockedAdd(gs_stats[RTAO_STAT_HITS], hits);
            InterlockedAdd(gs_stats[RTAO_STAT_INVALID_REFS], invalid_refs);
            InterlockedAdd(gs_stats[RTAO_STAT_STACK_OVERFLOW], stack_overflow);
        }
    }

    const uint texel = min(255u, (uint)round(saturate(ao) * 255.0));
    InterlockedAdd(gs_stats[RTAO_STAT_PIXELS], 1u);
    InterlockedAdd(gs_stats[RTAO_STAT_AO_SUM], texel);
    if (g_rtao_size.w > 0.5) g_rtao_raw[px] = traced ? ao : -1.0;
    else g_rtao_ao[px] = texel;
}

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID, uint3 gid : SV_GroupThreadID)
{
    // Round 1: no deforming objects in the RTAO trace (t10-t13 are null).
    g_trace_dynamic_count = (uint)g_rtao_size.z;
    const uint group_index = gid.y * 8u + gid.x;
    if (group_index < RTAO_STAT_COUNT) gs_stats[group_index] = 0u;
    if (group_index < RTAO_DISC_COUNT) gs_disc[group_index] = 0u;
    GroupMemoryBarrierWithGroupSync();

    if (id.x < (uint)g_rtao_size.x && id.y < (uint)g_rtao_size.y) TracePixel(id.xy);

    GroupMemoryBarrierWithGroupSync();
    if (group_index < RTAO_STAT_COUNT) InterlockedAdd(g_rtao_stats[group_index], gs_stats[group_index]);
    if (g_rtao_fade.z > 0.5 && group_index < RTAO_DISC_COUNT) InterlockedAdd(g_rtao_stats[RTAO_DISC_BASE + group_index], gs_disc[group_index]);
}
