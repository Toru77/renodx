// world_rtls.cs_5_0.hlsl - RT Local Shadows trace (world/docs/ROADMAP.md, RT Local Shadows M2).
//
// One thread per trace pixel. For the game's point and spot lamps of the pixel's tile cell (Sora 2nd lighting
// 0xCA3D8596 lines 829-840), shadow rays toward the lamp (a disk of Light Size, IS-FAST or IGN) through the live BVH.
// Writes x point shadow, y spot shadow (diffuse-weighted share of the kind's lamp light that is blocked), z the distance
// fade, a 1 traced / -1 not traced (u2), the debug views 1-5 (u0) and counters (u1).

// P2b records at t19 (t17 is the MRT normal here); camera-hidden records are skipped (RTAO views hide them).
#define WORLD_TRACE_DYNAMIC_INSTANCES
#define WORLD_TRACE_DYNAMIC_SLOT t19
#define WORLD_TRACE_DYNAMIC_SKIP_HIDDEN
#include "world_bvh_trace.hlsli"
#include "../../include/mrt_normal.hlsli"
#include "world_rt_lamps.hlsli"

Texture2D<float4> g_game_depth : register(t9);
Texture2D<uint4> g_mrt_normal : register(t17);
Texture3D<float2> g_isfast_noise : register(t18);
StructuredBuffer<LightParam> g_rtls_lights : register(t20);
StructuredBuffer<LightIndexData> g_rtls_light_indices : register(t21);

RWTexture2D<float4> g_rtls_debug : register(u0);
RWStructuredBuffer<uint> g_rtls_stats : register(u1);
RWTexture2D<float4> g_rtls_raw : register(u2);

cbuffer cb_scene : register(b0)
{
    float4x4 view_g : packoffset(c0);
    float4x4 viewInv_g : packoffset(c4);
    float4x4 viewProjInv_g : packoffset(c20);
    float2 lightTileSizeInv_g : packoffset(c29);
    float2 resolutionScaling_g : packoffset(c49);
};

// Push constants (rtls.hpp fills these; 28 floats).
cbuffer cb_rtls : register(b12)
{
    float4 g_rtls_params : packoffset(c0);   // effective ray max (m), normal bias (m), samples per light, trace divisor (1 in M2)
    float4 g_rtls_sampling : packoffset(c1); // IS-FAST flag (1 loaded), slice base, IGN seed, debug mode
    float4 g_rtls_fade : packoffset(c2);     // fade start_eff, fade end_eff, kind bits (RTLS_KIND_* as a float), P2b record count (w, uint bits)
    float4 g_rtls_region : packoffset(c3);   // region min xyz, region size
    float4 g_rtls_size : packoffset(c4);     // trace width, trace height, full width, full height
    float4 g_rtls_light : packoffset(c5);    // light size (m), light clearance (m), max point lights, lamp count
    float4 g_rtls_extra : packoffset(c6);    // character object count, 0, 0, 0
};

#define RTLS_STAT_PIXELS 0u
#define RTLS_STAT_TRACED 1u
#define RTLS_STAT_SKY 2u
#define RTLS_STAT_NORMAL 3u
#define RTLS_STAT_REGION 4u
#define RTLS_STAT_BEYOND_FADE 5u
#define RTLS_STAT_SCALED 6u
#define RTLS_STAT_CHARACTER 7u
#define RTLS_STAT_LIGHTS_CONSIDERED 8u
#define RTLS_STAT_LIGHTS_TRACED 9u
#define RTLS_STAT_POINTS_OVER_BUDGET 10u
#define RTLS_STAT_SPOTS_OVER_CAP 11u
#define RTLS_STAT_LIGHTS_BACKLIT 12u
#define RTLS_STAT_RAYS 13u
#define RTLS_STAT_RAYS_BLOCKED 14u
#define RTLS_STAT_RAYS_NEAR_LIGHT 15u
#define RTLS_STAT_RAYS_CLIPPED 16u
#define RTLS_STAT_INVALID_REFS 17u
#define RTLS_STAT_STACK_OVERFLOW 18u
#define RTLS_STAT_INDEX_OUT_OF_RANGE 19u
#define RTLS_STAT_PER_LIGHT_RAYS 32u
#define RTLS_STAT_PER_LIGHT_NEAR 64u
#define RTLS_STAT_COUNT 96u
#define RTLS_KIND_POINT 1u
#define RTLS_KIND_SPOT 2u
#define RTLS_KIND_CHARACTERS 4u
#define RTLS_STAT_CHARACTER_TRACED 24u
#define RTLS_STAT_CHARACTER_SELF_BLOCKED 25u
#define RTLS_MAX_POINTS 10u
#define RTLS_MAX_SPOTS 8u
#define RTLS_PER_LIGHT_COUNT 32u
#define RTLS_ID_INDEX_MASK 0x3FFu
#define RTLS_ID_BACKLIT 0x400u
#define RTLS_NEAR_LIGHT_BAND 0.5

groupshared uint gs_stats[RTLS_STAT_COUNT];

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID, uint3 gid : SV_GroupThreadID)
{
    // Characters (t10-t13) while Trace characters is on; count 0 otherwise.
    g_trace_dynamic_count = (uint)g_rtls_extra.x;
    g_trace_dynamic_instance_count = asuint(g_rtls_fade.w);
    const uint group_index = gid.y * 8u + gid.x;
    for (uint s = group_index; s < RTLS_STAT_COUNT; s += 64u) gs_stats[s] = 0u;
    GroupMemoryBarrierWithGroupSync();

    if (all(id.xy < uint2(g_rtls_size.xy)))
    {
        // Representative pixel: the block centre min(tp * d + d / 2, size - 1) (px = tp at Full).
        const uint2 tp = id.xy;
        const uint d = max((uint)g_rtls_params.w, 1u);
        const uint2 full = uint2(g_rtls_size.zw);
        const uint2 px = min(tp * d + d / 2u, full - 1u);
        const int debug_mode = (int)g_rtls_sampling.w;
        float4 raw = float4(0.0, 0.0, 0.0, -1.0);
        float3 dbg = float3(0.25, 0.0, 0.25);
        float3 cls = float3(0.25, 0.0, 0.25);

        const float depth = g_game_depth.Load(int3(px, 0)).x;
        const uint4 mrt0 = g_mrt_normal.Load(int3(px, 0));
        const float3 N = DecodeFalcomMrtNormalAsGame(mrt0.xy);

        if (any(resolutionScaling_g != float2(1.0, 1.0)))
        {
            InterlockedAdd(gs_stats[RTLS_STAT_SCALED], 1u);
            cls = float3(0.5, 0.5, 0.5);
        }
        else if (depth <= 0.0)
        {
            InterlockedAdd(gs_stats[RTLS_STAT_SKY], 1u);
            cls = float3(0.0, 0.0, 0.5);
        }
        else if (dot(N, N) < 0.5)
        {
            InterlockedAdd(gs_stats[RTLS_STAT_NORMAL], 1u);
            cls = float3(1.0, 0.0, 1.0);
        }
        else if ((mrt0.w & 8u) == 0u && ((uint)g_rtls_fade.z & RTLS_KIND_CHARACTERS) == 0u)
        {
            InterlockedAdd(gs_stats[RTLS_STAT_CHARACTER], 1u);
            cls = float3(0.5, 0.5, 0.0);
        }
        else
        {
            const float2 uv = (float2(px) + 0.5) / float2(full);
            const float4 world_h = mul(float4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, depth, 1.0), viewProjInv_g);
            const float3 P = world_h.xyz / world_h.w;
            const float ray_max = g_rtls_params.x;
            const float3 region_lo = g_rtls_region.xyz + ray_max;
            const float3 region_hi = g_rtls_region.xyz + g_rtls_region.w - ray_max;
            const float3 camera = viewInv_g._m30_m31_m32;
            const float dist = length(P - camera);
            if (!(all(P >= region_lo) && all(P <= region_hi)))
            {
                InterlockedAdd(gs_stats[RTLS_STAT_REGION], 1u);
                cls = float3(0.0, 0.5, 0.0);
            }
            else if (dist >= g_rtls_fade.y)
            {
                InterlockedAdd(gs_stats[RTLS_STAT_BEYOND_FADE], 1u);
                raw = float4(0.0, 0.0, 0.0, 1.0);
                dbg = float3(0.0, 0.0, 0.0);
            }
            else
            {
                InterlockedAdd(gs_stats[RTLS_STAT_TRACED], 1u);
                // Character pixel (Receive on characters): lamps are weighted by charaColor, rays get the character bias.
                const bool character = (mrt0.w & 8u) == 0u;
                if (character) InterlockedAdd(gs_stats[RTLS_STAT_CHARACTER_TRACED], 1u);

                // Cell index: the lighting shader's expression, kept word for word for the harness text check.
                const float view_z = dot(view_g._m02_m12_m22_m32, float4(P, 1.0));
                const uint2 tile = (uint2)(lightTileSizeInv_g * (float2(px) + 0.5));
                const uint row = tile.y << 5;
                const uint slice = (uint)clamp(g_rtls_light_indices[tile.x + row].tileDepthInv * -view_z, 0.0, 7.0);
                const uint cell = min(4607u, slice * 576u + row + tile.x);

                // Candidates: every point and spot lamp of the cell that lights P, in one loop over both lists. Points keep
                // the RTLS_MAX_POINTS strongest (descending); spots keep the first RTLS_MAX_SPOTS in list order.
                const uint kinds = (uint)g_rtls_fade.z;
                const uint lamp_count = (uint)g_rtls_light.w;
                const uint list_points = (kinds & RTLS_KIND_POINT) != 0u ? min(63u, g_rtls_light_indices[cell].pointLightCount) : 0u;
                const uint list_spots = (kinds & RTLS_KIND_SPOT) != 0u ? min(63u, g_rtls_light_indices[cell].spotLightCount) : 0u;
                float top_pw[RTLS_MAX_POINTS] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
                uint top_pi[RTLS_MAX_POINTS] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
                float top_sw[RTLS_MAX_SPOTS] = {0, 0, 0, 0, 0, 0, 0, 0};
                uint top_si[RTLS_MAX_SPOTS] = {0, 0, 0, 0, 0, 0, 0, 0};
                float point_sum = 0.0;
                float spot_sum = 0.0;
                uint point_n = 0u;
                uint spot_n = 0u;
                uint considered = 0u;
                uint out_of_range = 0u;
                [loop] for (uint i = 0u; i < list_points + list_spots; ++i)
                {
                    const bool spot = i >= list_points;
                    const uint li = spot ? (uint)g_rtls_light_indices[cell].spotLightIndices[i - list_points]
                                         : (uint)g_rtls_light_indices[cell].pointLightIndices[i];
                    if (li >= lamp_count)
                    {
                        out_of_range++;
                        continue;
                    }
                    // Diffuse weight as the game's loops weight the lamp: luminance of the colour x falloff x [cone] x
                    // max(NdotL, translucency); 0 when it does not light P. Back-lit lamps (NdotL <= 0, lit through
                    // translucency) are traced from the back side.
                    const LightParam lamp = g_rtls_lights[li];
                    if (lamp.radius <= 0.0) continue;
                    const float3 to_lamp = lamp.pos - P;
                    const float3 dir = normalize(to_lamp);
                    float att = LampDistanceAttenuation(lamp, to_lamp);
                    if (spot) att *= LampSpotCone(lamp, dir);
                    if (att <= 0.0) continue;
                    const float ndl = dot(N, dir);
                    float w = dot(character ? lamp.charaColor : lamp.color, float3(0.2126, 0.7152, 0.0722)) * att * max(ndl, lamp.translucency);
                    if (w <= 0.0) continue;
                    uint id = li | (ndl <= 0.0 ? RTLS_ID_BACKLIT : 0u);
                    considered++;
                    if (spot)
                    {
                        spot_sum += w;
                        if (spot_n < RTLS_MAX_SPOTS)
                        {
                            top_sw[spot_n] = w;
                            top_si[spot_n] = id;
                        }
                        spot_n++;
                    }
                    else
                    {
                        point_sum += w;
                        point_n++;
                        // Keep the RTLS_MAX_POINTS largest weights in descending order (empty slots hold weight 0).
                        [unroll] for (uint j = 0u; j < RTLS_MAX_POINTS; ++j)
                        {
                            if (w > top_pw[j])
                            {
                                const float tw = top_pw[j];
                                const uint ti = top_pi[j];
                                top_pw[j] = w;
                                top_pi[j] = id;
                                w = tw;
                                id = ti;
                            }
                        }
                    }
                }

                // Counts: spots all up to the cap, points up to the budget (and RTLS_MAX_POINTS).
                const uint spot_traced = min(spot_n, RTLS_MAX_SPOTS);
                const uint point_traced = min(point_n, min((uint)g_rtls_light.z, RTLS_MAX_POINTS));
                const uint spots_over = spot_n - spot_traced;
                const uint points_over = point_n - point_traced;

                // Trace loop: one trace call site for points and spots.
                const float ray_bias = g_rtls_params.y + (character ? g_rtls_extra.y : 0.0);
                const float light_size = g_rtls_light.x;
                const float clearance = g_rtls_light.y;
                const uint spp = max((uint)g_rtls_params.z, 1u);
                float point_shadow = 0.0;
                float spot_shadow = 0.0;
                uint traced_lights = 0u;
                uint lights_backlit = 0u;
                uint rays = 0u;
                uint blocked_rays = 0u;
                uint near_rays = 0u;
                uint clipped_rays = 0u;
                uint invalid = 0u;
                uint overflow = 0u;
                uint self_blocked = 0u;
                [loop] for (uint k = 0u; k < spot_traced + point_traced; ++k)
                {
                    const bool spot = k < spot_traced;
                    const uint slot = spot ? k : k - spot_traced;
                    const float w = spot ? top_sw[slot] : top_pw[slot];
                    const uint lid = spot ? top_si[slot] : top_pi[slot];
                    const uint li = lid & RTLS_ID_INDEX_MASK;
                    const bool backlit = (lid & RTLS_ID_BACKLIT) != 0u;
                    if (backlit) lights_backlit++;
                    const LightParam lamp = g_rtls_lights[li];

                    const float3 origin = P + N * (backlit ? -ray_bias : ray_bias);
                    const float3 axis = normalize(lamp.pos - origin);
                    const float3 up = abs(axis.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
                    const float3 T = normalize(cross(up, axis));
                    const float3 B = cross(axis, T);

                    uint lamp_visible = 0u;
                    for (uint smp = 0u; smp < spp; ++smp)
                    {
                        const uint sample_index = k * spp + smp;
                        float2 u;
                        if (g_rtls_sampling.x > 0.5)
                        {
                            u = g_isfast_noise.Load(int4(tp.x % 128u, tp.y % 128u, ((uint)g_rtls_sampling.y + sample_index) % 32u, 0));
                        }
                        else
                        {
                            const float2 q = float2(tp) + g_rtls_sampling.z + float(sample_index) * 7.0;
                            u = TraceIgnNoise2(q);
                        }
                        const float r = light_size * sqrt(u.x);
                        const float phi = 6.2831853 * u.y;
                        const float3 target = lamp.pos + T * (r * cos(phi)) + B * (r * sin(phi));
                        const float3 to_t = target - origin;
                        const float len = length(to_t);
                        const float reach = len - clearance;
                        if (reach <= 1e-3)
                        {
                            lamp_visible++;
                            continue;
                        }
                        const bool clipped = reach > ray_max;
                        const float t_max = clipped ? ray_max : reach;
                        const WorldCameraView view = {false, 1.0, 0u, 0u};
                        WorldTraceCounters counters;
                        const WorldTraceHit hit = TraceWorldAny(origin, to_t / len, 1e-3, t_max, view, counters);
                        rays++;
                        invalid += counters.invalid_refs;
                        overflow += counters.stack_overflow;
                        if (clipped) clipped_rays++;
                        const bool blocked = hit.prim != 0xFFFFFFFFu;
                        if (blocked) blocked_rays++;
                        else lamp_visible++;
                        const bool near = blocked && !clipped && hit.t > t_max - RTLS_NEAR_LIGHT_BAND;
                        if (near) near_rays++;
                        if (character && blocked && hit.t < 0.1) self_blocked++;
                        if (li < RTLS_PER_LIGHT_COUNT)
                        {
                            InterlockedAdd(gs_stats[RTLS_STAT_PER_LIGHT_RAYS + li], 1u);
                            if (near) InterlockedAdd(gs_stats[RTLS_STAT_PER_LIGHT_NEAR + li], 1u);
                        }
                    }
                    const float visible = float(lamp_visible) / float(spp);
                    if (spot) spot_shadow += w * (1.0 - visible);
                    else point_shadow += w * (1.0 - visible);
                    traced_lights++;
                }

                const float fade_start = g_rtls_fade.x;
                const float fade_end = g_rtls_fade.y;
                const float f = saturate((fade_end - dist) / max(fade_end - fade_start, 1e-4));
                raw = float4((point_sum > 0.0 ? point_shadow / point_sum : 0.0) * f,
                             (spot_sum > 0.0 ? spot_shadow / spot_sum : 0.0) * f, f, 1.0);

                InterlockedAdd(gs_stats[RTLS_STAT_LIGHTS_CONSIDERED], considered);
                InterlockedAdd(gs_stats[RTLS_STAT_LIGHTS_TRACED], traced_lights);
                InterlockedAdd(gs_stats[RTLS_STAT_POINTS_OVER_BUDGET], points_over);
                InterlockedAdd(gs_stats[RTLS_STAT_SPOTS_OVER_CAP], spots_over);
                InterlockedAdd(gs_stats[RTLS_STAT_LIGHTS_BACKLIT], lights_backlit);
                InterlockedAdd(gs_stats[RTLS_STAT_RAYS], rays);
                InterlockedAdd(gs_stats[RTLS_STAT_RAYS_BLOCKED], blocked_rays);
                InterlockedAdd(gs_stats[RTLS_STAT_RAYS_NEAR_LIGHT], near_rays);
                InterlockedAdd(gs_stats[RTLS_STAT_RAYS_CLIPPED], clipped_rays);
                InterlockedAdd(gs_stats[RTLS_STAT_INVALID_REFS], invalid);
                InterlockedAdd(gs_stats[RTLS_STAT_STACK_OVERFLOW], overflow);
                InterlockedAdd(gs_stats[RTLS_STAT_INDEX_OUT_OF_RANGE], out_of_range);
                InterlockedAdd(gs_stats[RTLS_STAT_CHARACTER_SELF_BLOCKED], self_blocked);

                cls = character ? float3(1.0, 0.5, 0.0) : (considered > 0u ? float3(1.0, 1.0, 1.0) : float3(0.0, 0.0, 0.0));
                if (debug_mode == 1) dbg = raw.xyz;
                else if (debug_mode == 2) dbg = (min(considered, 10u) / 10.0).xxx;
                else if (debug_mode == 3) dbg = float3(min(points_over + spots_over, 4u) / 4.0, min(traced_lights, 10u) / 10.0, 0.0);
                else if (debug_mode == 4) dbg = rays > 0u ? float3(float(near_rays) / float(rays), float(blocked_rays) / float(rays), 0.0) : float3(0.0, 0.0, 0.0);
            }
        }
        InterlockedAdd(gs_stats[RTLS_STAT_PIXELS], 1u);
        g_rtls_raw[tp] = raw;
        if (debug_mode >= 1 && debug_mode <= 5)
        {
            // The debug texture is full size: the trace pixel fills its d x d block.
            for (uint by = 0u; by < d; ++by)
            {
                for (uint bx = 0u; bx < d; ++bx)
                {
                    const uint2 block_px = tp * d + uint2(bx, by);
                    if (all(block_px < full)) g_rtls_debug[block_px] = float4(debug_mode == 5 ? cls : dbg, 1.0);
                }
            }
        }
    }

    GroupMemoryBarrierWithGroupSync();
    for (uint s = group_index; s < RTLS_STAT_COUNT; s += 64u)
    {
        if (gs_stats[s] != 0u) InterlockedAdd(g_rtls_stats[s], gs_stats[s]);
    }
}
