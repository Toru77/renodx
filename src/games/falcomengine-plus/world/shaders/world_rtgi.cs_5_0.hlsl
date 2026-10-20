// world_rtgi.cs_5_0.hlsl - RTGI trace (world/docs/ROADMAP.md, RTGI).
//
// One thread per trace pixel. Trace Resolution sets the block side d (1, 2 or 4): trace pixel tp stands for the d x d
// block at tp * d and reads the game depth and MRT normal at its representative pixel min(tp * d + d / 2, size - 1).
// Cosine-hemisphere closest-hit rays through the live BVH (world_bvh_trace.hlsli; deforming objects are not traced,
// the P2b records at t19 are). Each hit is classified as on screen (the game depth at its projected pixel is the hit),
// hidden on screen or off screen. Each hit gets a radiance. On screen with a valid copy: the previous frame's lighting
// output at the hit's previous-frame pixel (t20, prevViewProj_g). Otherwise: RTGI_FALLBACK_ALBEDO * (sun * N.L * a sun
// shadow ray + Ambient Contribution * the game's probe ambient). The raw GI (u2) is the mean sample radiance times the
// distance fade; the composite (M4) multiplies it by the receiver's G-buffer colour. Counters (u1) and debug views 1 to 3 (u0).
// Camera: the game scene CBV (b0). Noise: IS-FAST (t18, Load only) or IGN, offset by 64 px from RTAO's.

#define WORLD_TRACE_DYNAMIC_INSTANCES
#define WORLD_TRACE_DYNAMIC_SLOT t19
#define WORLD_TRACE_DYNAMIC_SKIP_HIDDEN
#include "world_bvh_trace.hlsli"
#include "../../include/mrt_normal.hlsli"

Texture2D<float4> g_game_depth : register(t9);
Texture2D<uint4> g_mrt_normal : register(t17);
Texture3D<float2> g_isfast_noise : register(t18);
Texture2D<float4> g_rtgi_radiance : register(t20);

RWTexture2D<float4> g_rtgi_debug : register(u0);
RWStructuredBuffer<uint> g_rtgi_stats : register(u1);
RWTexture2D<float4> g_rtgi_raw : register(u2);

cbuffer cb_scene : register(b0)
{
    float4x4 viewInv_g : packoffset(c4);
    float4x4 viewProj_g : packoffset(c16);
    float4x4 viewProjInv_g : packoffset(c20);
    float2 resolutionScaling_g : packoffset(c49);
    float3 lightColor_g : packoffset(c25);
    float3 lightDirection_g : packoffset(c26);
    float4 lightProbe_g[9] : packoffset(c37);
    float4x4 prevViewProj_g : packoffset(c75);
};

// Push constants (rtgi.hpp fills these; 24 floats).
cbuffer cb_rtgi : register(b12)
{
    float4 g_rtgi_params : packoffset(c0);    // effective ray max, normal bias, spp, trace divisor d
    float4 g_rtgi_sampling : packoffset(c1);  // isfast flag (1 loaded), slice base, seed, debug mode
    float4 g_rtgi_fade : packoffset(c2);      // fade start_eff, fade end_eff, flags, P2b record count (w, uint bits)
    float4 g_rtgi_region : packoffset(c3);    // region min xyz, region size
    float4 g_rtgi_size : packoffset(c4);      // trace width, trace height, full width, full height
    float4 g_rtgi_shading : packoffset(c5);   // reserved for the hit shading (M2)
};

#define RTGI_STAT_RAYS 0u
#define RTGI_STAT_HITS 1u
#define RTGI_STAT_PIXELS 2u
#define RTGI_STAT_SKY 3u
#define RTGI_STAT_NORMAL 4u
#define RTGI_STAT_REGION 5u
#define RTGI_STAT_BEYOND_FADE 6u
#define RTGI_STAT_INVALID_REFS 7u
#define RTGI_STAT_STACK_OVERFLOW 8u
#define RTGI_STAT_SCALED 9u
#define RTGI_STAT_HIT_ON_SCREEN 10u
#define RTGI_STAT_HIT_OCCLUDED 11u
#define RTGI_STAT_HIT_OFF_SCREEN 12u
#define RTGI_STAT_HIT_DISTANCE_DM 13u
#define RTGI_STAT_RADIANCE_LOOKUPS 14u
#define RTGI_STAT_SUN_RAYS 15u
#define RTGI_STAT_SUN_VISIBLE 16u
#define RTGI_STAT_LUMINANCE_SUM 17u
#define RTGI_STAT_LUMINANCE_MAX 18u
#define RTGI_STAT_CLAMPED 19u
#define RTGI_STAT_TRACE_COUNT 20u
#define RTGI_FLAG_RADIANCE_VALID 1u
#define RTGI_SUN_RAY_MAX 64.0
#define RTGI_FALLBACK_ALBEDO 0.5

groupshared uint gs_stats[RTGI_STAT_TRACE_COUNT];

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID, uint3 gid : SV_GroupThreadID)
{
    // Deforming objects (characters, water) are not traced: t10-t13 are null. P2b record count: c2.w (uint bits).
    g_trace_dynamic_count = 0u;
    g_trace_dynamic_instance_count = asuint(g_rtgi_fade.w);
    const uint group_index = gid.y * 8u + gid.x;
    if (group_index < RTGI_STAT_TRACE_COUNT) gs_stats[group_index] = 0u;
    GroupMemoryBarrierWithGroupSync();
    if (id.x < (uint)g_rtgi_size.x && id.y < (uint)g_rtgi_size.y)
    {
        const uint2 tp = id.xy;
        const uint d = max((uint)g_rtgi_params.w, 1u);
        const uint2 full_size = uint2(g_rtgi_size.zw);
        const uint2 px = min(tp * d + d / 2u, full_size - 1u);
        const float ray_max = g_rtgi_params.x;
        const uint spp = max((uint)g_rtgi_params.z, 1u);
        const int debug_mode = (int)g_rtgi_sampling.w;

        // Not traced (sky, invalid normal, scaled, outside the region, beyond Fade End): dark purple in every debug view.
        float3 debug_color = float3(0.25, 0.0, 0.25);
        bool traced = false;
        float3 gi = float3(0.0, 0.0, 0.0);
        const float depth = g_game_depth.Load(int3(px, 0)).x;
        const float3 N = DecodeFalcomMrtNormal(g_mrt_normal.Load(int3(px, 0)).xy);
        if (any(resolutionScaling_g != float2(1.0, 1.0)))
        {
            InterlockedAdd(gs_stats[RTGI_STAT_SCALED], 1u);
        }
        else if (depth <= 0.0)
        {
            InterlockedAdd(gs_stats[RTGI_STAT_SKY], 1u);
        }
        else if (dot(N, N) < 0.5)
        {
            InterlockedAdd(gs_stats[RTGI_STAT_NORMAL], 1u);
        }
        else
        {
            const float2 uv = (float2(px) + 0.5) / float2(full_size);
            const float4 world_h = mul(float4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, depth, 1.0), viewProjInv_g);
            const float3 P = world_h.xyz / world_h.w;
            const float3 region_lo = g_rtgi_region.xyz + ray_max;
            const float3 region_hi = g_rtgi_region.xyz + g_rtgi_region.w - ray_max;
            const float3 camera = float3(viewInv_g._m30, viewInv_g._m31, viewInv_g._m32);
            const float distance_to_camera = length(P - camera);
            if (!(all(P >= region_lo) && all(P <= region_hi)))
            {
                InterlockedAdd(gs_stats[RTGI_STAT_REGION], 1u);
            }
            else if (distance_to_camera >= g_rtgi_fade.y)
            {
                InterlockedAdd(gs_stats[RTGI_STAT_BEYOND_FADE], 1u);
                traced = true;
            }
            else
            {
                traced = true;
                const uint flags = (uint)g_rtgi_fade.z;
                uint lookups = 0u;
                uint sun_rays = 0u;
                uint sun_visible_count = 0u;
                uint luminance_sum = 0u;
                uint luminance_max = 0u;
                uint clamped = 0u;
                float3 gi_sum = float3(0.0, 0.0, 0.0);
                const float3 origin = P + N * g_rtgi_params.y;
                const float3 up = abs(N.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
                const float3 T = normalize(cross(up, N));
                const float3 B = cross(N, T);
                // GI rays skip surfaces the game camera does not show (shadow-only casters, near-faded surfaces), as RTAO.
                const WorldCameraView view = {true, 1.0, 0u};
                uint hits = 0u;
                uint on_screen = 0u;
                uint occluded = 0u;
                uint off_screen = 0u;
                uint distance_dm = 0u;
                uint invalid_refs = 0u;
                uint stack_overflow = 0u;
                float t_sum = 0.0;
                for (uint i = 0u; i < spp; ++i)
                {
                    float2 u;
                    if (g_rtgi_sampling.x > 0.5)
                    {
                        u = g_isfast_noise.Load(int4((tp.x + 64u) % 128u, (tp.y + 64u) % 128u, ((uint)g_rtgi_sampling.y + i) % 32u, 0));
                    }
                    else
                    {
                        const float2 q = float2(tp) + g_rtgi_sampling.z + 13.0 + float(i) * 7.0;
                        u = frac(52.9829189 * frac(float2(dot(q, float2(0.06711056, 0.00583715)), dot(q + 47.0, float2(0.06711056, 0.00583715)))));
                    }
                    const float r = sqrt(u.x);
                    const float phi = 6.2831853 * u.y;
                    const float3 dir = T * (r * cos(phi)) + B * (r * sin(phi)) + N * sqrt(max(0.0, 1.0 - u.x));

                    WorldTraceCounters counters;
                    const WorldTraceHit hit = TraceWorldClosest(origin, dir, 1e-3, ray_max, view, counters);
                    invalid_refs += counters.invalid_refs;
                    stack_overflow += counters.stack_overflow;
                    if (hit.prim == 0xFFFFFFFFu)
                    {
                        continue;
                    }
                    hits += 1u;
                    t_sum += hit.t;
                    distance_dm += (uint)(hit.t * 10.0);
                    // On screen: the hit projects inside the image and the game depth there is the hit (2% or 5 cm, the
                    // RTAO discovery tolerance). Hidden on screen: it projects inside but another surface is in front.
                    const float hit_distance = length(hit.position - camera);
                    const float4 clip = mul(float4(hit.position, 1.0), viewProj_g);
                    const float2 hit_uv = float2(clip.x / clip.w * 0.5 + 0.5, 0.5 - clip.y / clip.w * 0.5);
                    bool visible = false;
                    if (clip.w <= 1e-6 || any(hit_uv < 0.0) || any(hit_uv >= 1.0))
                    {
                        off_screen += 1u;
                    }
                    else
                    {
                        const uint2 q = min(uint2(hit_uv * float2(full_size)), full_size - 1u);
                        const float q_depth = g_game_depth.Load(int3(q, 0)).x;
                        const float2 q_uv = (float2(q) + 0.5) / float2(full_size);
                        const float4 q_h = mul(float4(q_uv.x * 2.0 - 1.0, 1.0 - q_uv.y * 2.0, q_depth, 1.0), viewProjInv_g);
                        visible = q_depth > 0.0
                            && abs(length(q_h.xyz / q_h.w - camera) - hit_distance) <= max(0.02 * hit_distance, 0.05);
                        if (visible)
                        {
                            on_screen += 1u;
                        }
                        else
                        {
                            occluded += 1u;
                        }
                    }
                    float3 radiance = float3(0.0, 0.0, 0.0);
                    bool looked_up = false;
                    if (visible && (flags & RTGI_FLAG_RADIANCE_VALID) != 0u)
                    {
                        // The copy is last frame's lighting output: look the hit up where the previous camera saw it.
                        const float4 prev_clip = mul(float4(hit.position, 1.0), prevViewProj_g);
                        const float2 prev_uv = float2(prev_clip.x / prev_clip.w * 0.5 + 0.5, 0.5 - prev_clip.y / prev_clip.w * 0.5);
                        if (prev_clip.w > 1e-6 && all(prev_uv >= 0.0) && all(prev_uv < 1.0))
                        {
                            uint rw;
                            uint rh;
                            g_rtgi_radiance.GetDimensions(rw, rh);
                            radiance = g_rtgi_radiance.Load(int3(min(uint2(prev_uv * float2(rw, rh)), uint2(rw, rh) - 1u), 0)).rgb;
                            looked_up = true;
                            lookups += 1u;
                        }
                    }
                    if (!looked_up)
                    {
                        // Hidden, off screen or no copy: sun with a shadow ray (shadow-only casters block it: hide off) plus
                        // the game's probe ambient (lighting_0xCA3D8596 lines 827-841), times a grey albedo.
                        const float3 n = dot(hit.normal, dir) > 0.0 ? -hit.normal : hit.normal;
                        const float3 sun_dir = normalize(-lightDirection_g);
                        const float ndl = saturate(dot(n, sun_dir));
                        float sun = 0.0;
                        if (ndl > 0.0)
                        {
                            const WorldCameraView sun_view = {false, 1.0, 0u};
                            WorldTraceCounters sun_counters;
                            const WorldTraceHit sun_hit = TraceWorldAny(hit.position + n * g_rtgi_params.y, sun_dir, 1e-3, RTGI_SUN_RAY_MAX, sun_view, sun_counters);
                            sun_rays += 1u;
                            invalid_refs += sun_counters.invalid_refs;
                            stack_overflow += sun_counters.stack_overflow;
                            if (sun_hit.prim == 0xFFFFFFFFu)
                            {
                                sun = ndl;
                                sun_visible_count += 1u;
                            }
                        }
                        const float3 ambient = max(float3(0.0, 0.0, 0.0),
                            lightProbe_g[0].xyz + lightProbe_g[1].xyz * n.x + lightProbe_g[2].xyz * n.y + lightProbe_g[3].xyz * n.z
                            + lightProbe_g[4].xyz * (n.z * n.x) + lightProbe_g[5].xyz * (n.y * n.z) + lightProbe_g[6].xyz * (n.y * n.x)
                            + lightProbe_g[7].xyz * (3.0 * n.z * n.z - 1.0) + lightProbe_g[8].xyz * (n.x * n.x - n.y * n.y));
                        radiance = RTGI_FALLBACK_ALBEDO * (lightColor_g * sun + g_rtgi_shading.x * ambient);
                    }
                    if (any(isnan(radiance)) || any(isinf(radiance)))
                    {
                        radiance = float3(0.0, 0.0, 0.0);
                    }
                    radiance = max(radiance, 0.0);
                    const float luminance = dot(radiance, float3(0.2126, 0.7152, 0.0722));
                    luminance_sum += (uint)(min(luminance, 1e6) * 100.0);
                    luminance_max = max(luminance_max, asuint(luminance));
                    if (g_rtgi_shading.y > 0.0 && luminance > g_rtgi_shading.y)
                    {
                        radiance *= g_rtgi_shading.y / luminance;
                        clamped += 1u;
                    }
                    gi_sum += radiance;
                }
                InterlockedAdd(gs_stats[RTGI_STAT_RAYS], spp);
                InterlockedAdd(gs_stats[RTGI_STAT_HITS], hits);
                InterlockedAdd(gs_stats[RTGI_STAT_HIT_ON_SCREEN], on_screen);
                InterlockedAdd(gs_stats[RTGI_STAT_HIT_OCCLUDED], occluded);
                InterlockedAdd(gs_stats[RTGI_STAT_HIT_OFF_SCREEN], off_screen);
                InterlockedAdd(gs_stats[RTGI_STAT_HIT_DISTANCE_DM], distance_dm);
                InterlockedAdd(gs_stats[RTGI_STAT_INVALID_REFS], invalid_refs);
                InterlockedAdd(gs_stats[RTGI_STAT_STACK_OVERFLOW], stack_overflow);
                InterlockedAdd(gs_stats[RTGI_STAT_RADIANCE_LOOKUPS], lookups);
                InterlockedAdd(gs_stats[RTGI_STAT_SUN_RAYS], sun_rays);
                InterlockedAdd(gs_stats[RTGI_STAT_SUN_VISIBLE], sun_visible_count);
                InterlockedAdd(gs_stats[RTGI_STAT_LUMINANCE_SUM], luminance_sum);
                InterlockedMax(gs_stats[RTGI_STAT_LUMINANCE_MAX], luminance_max);
                InterlockedAdd(gs_stats[RTGI_STAT_CLAMPED], clamped);
                const float fade = saturate((g_rtgi_fade.y - distance_to_camera) / max(g_rtgi_fade.y - g_rtgi_fade.x, 1e-4));
                gi = gi_sum / (float)spp * fade;
                const float mean_t = hits > 0u ? t_sum / (float)hits / ray_max : 0.0;
                if (debug_mode == 1)
                {
                    debug_color = float3(off_screen, on_screen, occluded) / (float)spp;
                }
                else if (debug_mode == 2)
                {
                    debug_color = float3(mean_t, mean_t, mean_t);
                }
                else if (debug_mode == 3)
                {
                    debug_color = gi / (1.0 + gi);
                }
            }
        }
        InterlockedAdd(gs_stats[RTGI_STAT_PIXELS], 1u);
        if (debug_mode >= 1 && debug_mode <= 3)
        {
            for (uint dy = 0u; dy < d; ++dy)
            {
                for (uint dx = 0u; dx < d; ++dx)
                {
                    const uint2 block_px = tp * d + uint2(dx, dy);
                    if (all(block_px < full_size))
                    {
                        g_rtgi_debug[block_px] = float4(debug_color, 1.0);
                    }
                }
            }
        }
        g_rtgi_raw[tp] = traced ? float4(gi, 1.0) : float4(0.0, 0.0, 0.0, -1.0);
    }
    GroupMemoryBarrierWithGroupSync();
    if (group_index < RTGI_STAT_TRACE_COUNT)
    {
        if (group_index == RTGI_STAT_LUMINANCE_MAX)
        {
            InterlockedMax(g_rtgi_stats[group_index], gs_stats[group_index]);
        }
        else
        {
            InterlockedAdd(g_rtgi_stats[group_index], gs_stats[group_index]);
        }
    }
}
