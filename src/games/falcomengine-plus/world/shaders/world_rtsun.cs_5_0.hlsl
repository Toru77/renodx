// world_rtsun.cs_5_0.hlsl - RT Sun Shadows trace (world/docs/ROADMAP.md, RT Sun Shadows R3a).
//
// One thread per trace pixel: a d x d block of full-size pixels (d = trace divisor, c2.w); the representative pixel is the
// block centre. Environment pixels inside the RT cascade range (RtSunWeight > 0) and the BVH region
// shoot shadow rays toward the sun (a disk of Light Size, IS-FAST or IGN) through the live BVH; character casters block
// them (characters are not receivers here, their pixels are skipped and counted). Writes x shadow x w, z w and a 1 traced /
// -1 not traced (u2), the debug views 1-2 (u0) and counters (u1). The image is not changed by this pass.

// P2b records at t19 (t17 is the MRT normal here); character casters are traced (shadow-only casters included, so hidden
// records are not skipped).
#define WORLD_TRACE_DYNAMIC_INSTANCES
#define WORLD_TRACE_DYNAMIC_SLOT t19
#include "world_bvh_trace.hlsli"
#include "../../include/mrt_normal.hlsli"
#include "../../include/rt_sun_range.hlsli"

Texture2D<float4> g_game_depth : register(t9);
Texture2D<uint4> g_mrt_normal : register(t17);
Texture3D<float2> g_isfast_noise : register(t18);

RWTexture2D<float4> g_rtsun_debug : register(u0);
RWStructuredBuffer<uint> g_rtsun_stats : register(u1);
RWTexture2D<float4> g_rtsun_raw : register(u2);

// Scene constants (b0), the game's layout from the Sora 2nd lighting shader (lighting_0xCA3D8596).
cbuffer cb_scene : register(b0)
{
    float4x4 view_g : packoffset(c0);
    float4x4 viewInv_g : packoffset(c4);
    float4x4 viewProjInv_g : packoffset(c20);
    float3 lightDirection_g : packoffset(c26);
    int shadowmapCascadeCount_g : packoffset(c27.w);
    float2 resolutionScaling_g : packoffset(c49);
    float shadowFadeNear_g : packoffset(c50.w);
    float3 shadowSplitDistance_g : packoffset(c57);
    float shadowFadeRangeInv_g : packoffset(c57.w);
};

// Push constants (rtsun.hpp fills these; 20 floats).
cbuffer cb_rtsun : register(b12)
{
    float4 g_rtsun_params : packoffset(c0);   // Max Ray Distance (m), normal bias (m), samples per pixel, effective RT cascades
    float4 g_rtsun_sampling : packoffset(c1); // IS-FAST flag (1 loaded), slice base, IGN seed, debug mode
    float4 g_rtsun_sun : packoffset(c2);      // light size (degrees), character object count, P2b record count (z, uint bits), trace divisor (w)
    float4 g_rtsun_region : packoffset(c3);   // BVH region min xyz, region size
    float4 g_rtsun_size : packoffset(c4);     // full width, full height, region margin (m; kPoolRegionRayMargin), 0
    float2 g_rtsun_character : packoffset(c5); // Receive on characters (x, 1 on), Character Normal Bias (y, m)
};

#define RTSUN_STAT_PIXELS 0u
#define RTSUN_STAT_TRACED 1u
#define RTSUN_STAT_SKY 2u
#define RTSUN_STAT_NORMAL 3u
#define RTSUN_STAT_REGION 4u
#define RTSUN_STAT_BEYOND_RANGE 5u
#define RTSUN_STAT_SCALED 6u
#define RTSUN_STAT_CHARACTER 7u
#define RTSUN_STAT_RAYS 8u
#define RTSUN_STAT_RAYS_BLOCKED 9u
#define RTSUN_STAT_CHARACTER_RAYS 10u
#define RTSUN_STAT_SELF_BLOCKED 11u
#define RTSUN_STAT_INVALID_REFS 12u
#define RTSUN_STAT_STACK_OVERFLOW 13u
#define RTSUN_STAT_CHARACTERS_TRACED 14u
#define RTSUN_STAT_CHARACTERS_SELF_BLOCKED 15u
#define RTSUN_STAT_GATE_SCALING_X 16u
#define RTSUN_STAT_GATE_SCALING_Y 17u
#define RTSUN_STAT_GATE_CASCADE_COUNT 18u
#define RTSUN_STAT_GATE_CAMERA_X 19u
#define RTSUN_STAT_COUNT 32u

groupshared uint gs_stats[RTSUN_STAT_COUNT];

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID, uint3 gid : SV_GroupThreadID)
{
    g_trace_dynamic_count = (uint)g_rtsun_sun.y;
    g_trace_dynamic_instance_count = asuint(g_rtsun_sun.z);
    const uint group_index = gid.y * 8u + gid.x;
    for (uint s = group_index; s < RTSUN_STAT_COUNT; s += 64u) gs_stats[s] = 0u;
    GroupMemoryBarrierWithGroupSync();

    const uint d = max((uint)g_rtsun_sun.w, 1u);
    const uint2 full = uint2(g_rtsun_size.xy);
    const uint2 trace = (full + d - 1u) / d;
    if (all(id.xy < trace))
    {
        // Representative pixel: the block centre min(tp * d + d / 2, full - 1) (px = tp at Full).
        const uint2 tp = id.xy;
        const uint2 px = min(tp * d + d / 2u, full - 1u);
        const int debug_mode = (int)g_rtsun_sampling.w;
        float4 raw = float4(0.0, 0.0, 0.0, -1.0);
        float3 cls = float3(0.0, 0.0, 0.0);

        const float depth = g_game_depth.Load(int3(px, 0)).x;
        const uint4 mrt0 = g_mrt_normal.Load(int3(px, 0));
        const float3 N = DecodeFalcomMrtNormalAsGame(mrt0.xy);

        if (any(resolutionScaling_g != float2(1.0, 1.0)))
        {
            InterlockedAdd(gs_stats[RTSUN_STAT_SCALED], 1u);
            cls = float3(0.5, 0.5, 0.5);
        }
        else if (depth <= 0.0)
        {
            InterlockedAdd(gs_stats[RTSUN_STAT_SKY], 1u);
            cls = float3(0.0, 0.0, 0.5);
        }
        else if (dot(N, N) < 0.5)
        {
            InterlockedAdd(gs_stats[RTSUN_STAT_NORMAL], 1u);
            cls = float3(1.0, 0.0, 1.0);
        }
        else if ((mrt0.w & 8u) == 0u && g_rtsun_character.x < 0.5)
        {
            // Character pixel with Receive on characters off: not traced. Its raw term stays not traced.
            InterlockedAdd(gs_stats[RTSUN_STAT_CHARACTER], 1u);
            cls = float3(0.5, 0.5, 0.0);
        }
        else
        {
            const float2 uv = (float2(px) + 0.5) / g_rtsun_size.xy;
            const float4 world_h = mul(float4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, depth, 1.0), viewProjInv_g);
            const float3 P = world_h.xyz / world_h.w;
            const float ray_max = g_rtsun_params.x;
            const float region_margin = g_rtsun_size.z;
            const float3 region_lo = g_rtsun_region.xyz + region_margin;
            const float3 region_hi = g_rtsun_region.xyz + g_rtsun_region.w - region_margin;
            const float3 camera = viewInv_g._m30_m31_m32;
            const float dist = length(P - camera);
            const float w = RtSunWeight(dist, (uint)g_rtsun_params.w, shadowmapCascadeCount_g, shadowSplitDistance_g, shadowFadeNear_g, shadowFadeRangeInv_g);
            if (!(all(P >= region_lo) && all(P <= region_hi)))
            {
                InterlockedAdd(gs_stats[RTSUN_STAT_REGION], 1u);
                cls = float3(0.0, 0.5, 0.0);
            }
            else if (w <= 0.0)
            {
                InterlockedAdd(gs_stats[RTSUN_STAT_BEYOND_RANGE], 1u);
                raw = float4(0.0, 0.0, 0.0, 1.0);
                cls = float3(0.25, 0.0, 0.25);
            }
            else
            {
                InterlockedAdd(gs_stats[RTSUN_STAT_TRACED], 1u);
                // Sun direction toward the light (lightDirection_g is the direction the light travels).
                const float3 sun = normalize(-lightDirection_g);
                const float ndl = dot(N, sun);
                const bool receiver_character = (mrt0.w & 8u) == 0u;
                const float normal_bias = receiver_character ? g_rtsun_character.y : g_rtsun_params.y;
                const float ray_bias = normal_bias * (ndl > 0.0 ? 1.0 : -1.0);
                const float3 origin = P + N * ray_bias;
                const float3 up = abs(sun.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
                const float3 T = normalize(cross(up, sun));
                const float3 B = cross(sun, T);
                const float disk = tan(radians(g_rtsun_sun.x * 0.5));
                const uint spp = max((uint)g_rtsun_params.z, 1u);

                uint visible = 0u;
                uint rays = 0u;
                uint blocked_rays = 0u;
                uint character_rays = 0u;
                uint self_blocked = 0u;
                uint characters_self_blocked = 0u;
                uint invalid = 0u;
                uint overflow = 0u;
                [loop] for (uint smp = 0u; smp < spp; ++smp)
                {
                    float2 u;
                    if (g_rtsun_sampling.x > 0.5)
                    {
                        u = g_isfast_noise.Load(int4(tp.x % 128u, tp.y % 128u, ((uint)g_rtsun_sampling.y + smp) % 32u, 0));
                    }
                    else
                    {
                        const float2 q = float2(tp) + g_rtsun_sampling.z + float(smp) * 7.0;
                        u = TraceIgnNoise2(q);
                    }
                    const float r = disk * sqrt(u.x);
                    const float phi = 6.2831853 * u.y;
                    const float3 dir = normalize(sun + (T * cos(phi) + B * sin(phi)) * r);
                    const WorldCameraView view = {false, 1.0, 0u, 0u};
                    WorldTraceCounters counters;
                    const WorldTraceHit hit = TraceWorldAny(origin, dir, 1e-3, ray_max, view, counters);
                    rays++;
                    invalid += counters.invalid_refs;
                    overflow += counters.stack_overflow;
                    if (hit.prim != 0xFFFFFFFFu)
                    {
                        blocked_rays++;
                        if ((hit.instance & DYNAMIC_INSTANCE_FLAG) != 0u) character_rays++;
                        if (hit.t < 0.1)
                        {
                            self_blocked++;
                            if (receiver_character) characters_self_blocked++;
                        }
                    }
                    else
                    {
                        visible++;
                    }
                }

                const float shadow = 1.0 - float(visible) / float(spp);
                raw = float4(shadow * w, 0.0, w, 1.0);
                cls = float3(1.0, 1.0, 1.0);

                InterlockedAdd(gs_stats[RTSUN_STAT_RAYS], rays);
                InterlockedAdd(gs_stats[RTSUN_STAT_RAYS_BLOCKED], blocked_rays);
                InterlockedAdd(gs_stats[RTSUN_STAT_CHARACTER_RAYS], character_rays);
                InterlockedAdd(gs_stats[RTSUN_STAT_SELF_BLOCKED], self_blocked);
                if (receiver_character)
                {
                    InterlockedAdd(gs_stats[RTSUN_STAT_CHARACTERS_TRACED], 1u);
                    InterlockedAdd(gs_stats[RTSUN_STAT_CHARACTERS_SELF_BLOCKED], characters_self_blocked);
                }
                InterlockedAdd(gs_stats[RTSUN_STAT_INVALID_REFS], invalid);
                InterlockedAdd(gs_stats[RTSUN_STAT_STACK_OVERFLOW], overflow);
            }
        }
        InterlockedAdd(gs_stats[RTSUN_STAT_PIXELS], 1u);
        g_rtsun_raw[tp] = raw;
        if (debug_mode == 1 || debug_mode == 2)
        {
            // The debug texture is full size: the trace pixel fills its d x d block.
            const float3 dbg = debug_mode == 1 ? (raw.a > 0.0 ? raw.xxx : float3(0.0, 0.0, 0.0)) : cls;
            for (uint by = 0u; by < d; ++by)
            {
                for (uint bx = 0u; bx < d; ++bx)
                {
                    const uint2 block_px = tp * d + uint2(bx, by);
                    if (all(block_px < full)) g_rtsun_debug[block_px] = float4(dbg, 1.0);
                }
            }
        }
    }

    GroupMemoryBarrierWithGroupSync();
    for (uint s = group_index; s < RTSUN_STAT_COUNT; s += 64u)
    {
        if (gs_stats[s] != 0u) InterlockedAdd(g_rtsun_stats[s], gs_stats[s]);
    }

    if (all(id.xy == uint2(0u, 0u)))
    {
        g_rtsun_stats[RTSUN_STAT_GATE_SCALING_X] = asuint(resolutionScaling_g.x);
        g_rtsun_stats[RTSUN_STAT_GATE_SCALING_Y] = asuint(resolutionScaling_g.y);
        g_rtsun_stats[RTSUN_STAT_GATE_CASCADE_COUNT] = asuint(shadowmapCascadeCount_g);
        g_rtsun_stats[RTSUN_STAT_GATE_CAMERA_X] = asuint(viewInv_g._m30);
    }
}
