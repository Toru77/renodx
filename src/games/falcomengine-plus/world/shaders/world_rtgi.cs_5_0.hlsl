// world_rtgi.cs_5_0.hlsl - RTGI trace (world/docs/ROADMAP.md, RTGI).
//
// One thread per trace pixel. Trace Resolution sets the block side d (1, 2 or 4): trace pixel tp stands for the d x d
// block at tp * d and reads the game depth and MRT normal at its representative pixel min(tp * d + d / 2, size - 1).
// Cosine-hemisphere closest-hit rays through the live BVH (world_bvh_trace.hlsli; characters at t10-t13 when on,
// the P2b records at t19 are). Each hit is classified as on screen (the game depth at its projected pixel is the hit),
// hidden on screen or off screen. Each hit gets a radiance. On screen with a valid copy: the previous frame's lighting
// output at the hit's previous-frame pixel (t20, prevViewProj_g). Otherwise: RTGI_FALLBACK_ALBEDO * (sun * N.L * a sun
// shadow ray + Ambient Contribution * the game's probe ambient). The raw GI (u2) is the mean sample radiance times the
// distance fade; the composite (M4) multiplies it by the receiver's G-buffer colour. Counters (u1) and debug views
// 1 to 3, 8 and 9 (u0). On screen (O1, Sora 2nd): the sun and lamp parts of the looked-up bounce are scaled by Sun
// Contribution and Dynamic Light Contribution, estimated from this frame's G-buffer (t25-t28).
// One bounce (M4): the copy holds last frame's applied bounce, so with RTGI_FLAG_SUBTRACT_PREVIOUS each lookup subtracts
// the previous final GI (t21) x the G-buffer colour at the hit (t22) x last frame's GI Intensity (c5.z), clamped at 0.
// Lamps (M4): with RTGI_FLAG_DYNAMIC_LIGHTS, a fallback hit adds the point and spot diffuse of the lamps uploaded
// this frame
// (t23) whose type bit (t24, world_rtgi_lights.cs_5_0.hlsl) is set. No shadow rays.
// Camera: the game scene CBV (b0). Noise: IS-FAST (t18, Load only) or IGN, offset by 64 px from RTAO's.
// cb_scene: the shared offsets are named below; prevViewProj_g moves between lighting variants (Sora 1st c74,
// Sora 2nd c75, Kai c85; rtgi_state.hpp RtgiPrevViewProjRegister), so its four registers are read from the
// g_scene_rows window at the register in c5.w. Reads past the end of a smaller scene buffer return 0 (D3D11 rule;
// the debug layer may warn that the buffer is smaller than declared).

#define WORLD_TRACE_DYNAMIC_INSTANCES
#define WORLD_TRACE_DYNAMIC_SLOT t19
#define WORLD_TRACE_DYNAMIC_SKIP_HIDDEN
#include "world_bvh_trace.hlsli"
#include "../../include/mrt_normal.hlsli"
#include "world_rt_lamps.hlsli"

// The game's LightParam (sora2nd/lighting/lighting_0xCA3D8596.ps_5_0.hlsl): 112 bytes. The full layout sets the stride.
struct LightParam
{
    float3 pos;
    float radius;
    float3 color;
    float radiusInv;
    float3 charaColor;
    float attenuation;
    float3 vec;
    float spotAngleInv;
    float attenuationAngle;
    float specularIntensity;
    float specularGlossiness;
    float scatterAnisotropy;
    float3 scatterColor;
    float scatterDensity;
    float translucency;
    int shadowmapIndex;
    float scatterOffset;
    float userParam;
};

// The game's DeferredParam (144 bytes). Only dynamicLightIntensity (byte offset 76 = .w of mid) is read.
struct GameDeferredParam
{
    float4 head[4];
    float4 mid;
    float4 tail[4];
};

Texture2D<float4> g_game_depth : register(t9);
Texture2D<uint4> g_mrt_normal : register(t17);
Texture3D<float2> g_isfast_noise : register(t18);
Texture2D<float4> g_rtgi_radiance : register(t20);
Texture2D<float4> g_rtgi_prev_final : register(t21);  // last frame's final GI (one-bounce correction)
Texture2D<float4> g_rtgi_albedo : register(t22);      // this frame's G-buffer colour, the lighting draw's colorTexture
StructuredBuffer<LightParam> g_rtgi_lamps : register(t23);  // Sora 2nd dynamicLights_g (Off-screen Dynamic Lights)
StructuredBuffer<uint> g_rtgi_lamp_types : register(t24);  // world_rtgi_lights: bit 1 point, bit 2 spot, per lamp
Texture2D<uint4> g_mrt1 : register(t25);                    // G-buffer mrtTexture1 (Sora 2nd): sun diffuse factor in .w
Texture2D<float4> g_shadow_mask : register(t26);            // screen shadow map (Sora 2nd shadowTexture)
Texture2D<uint2> g_mrt2 : register(t27);                    // G-buffer mrtTexture2 (Sora 2nd): material index in .x
StructuredBuffer<GameDeferredParam> g_rtgi_deferred : register(t28);  // Sora 2nd deferredParams_g

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
    float3 sceneShadowColor_g : packoffset(c27);
    float4 lightProbe_g[9] : packoffset(c37);
    float4 g_scene_rows[46] : packoffset(c50);  // scene registers c50..c95 (per-variant prevViewProj_g)
};
#define RTGI_SCENE_ROWS_FIRST 50u

// Push constants (rtgi.hpp fills these; 32 floats).
cbuffer cb_rtgi : register(b12)
{
    float4 g_rtgi_params : packoffset(c0);    // effective ray max, normal bias, spp, trace divisor d
    float4 g_rtgi_sampling : packoffset(c1);  // isfast flag (1 loaded), slice base, seed, debug mode
    float4 g_rtgi_fade : packoffset(c2);      // fade start_eff, fade end_eff, flags, P2b record count (w, uint bits)
    float4 g_rtgi_region : packoffset(c3);    // region min xyz, region size
    float4 g_rtgi_size : packoffset(c4);      // trace width, trace height, full width, full height
    float4 g_rtgi_shading : packoffset(c5);   // ambient contribution, radiance clamp (0 = off), last frame's applied GI Intensity, prevViewProj_g register (per variant)
    float4 g_rtgi_sun : packoffset(c6); // Sun Shadow Distance (m), characters (t10-t13), Sun Contribution, unused
    float4 g_rtgi_dynamic : packoffset(c7);  // Dynamic Light Contribution (x), Feedback Strength (y, effective), Extra Bounce Rays (z, Real Extra Rays only), unused
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
#define RTGI_STAT_LUMINANCE_SUM 17u  // sum of min(luminance, RTGI_LUMINANCE_STAT_CAP) x 10, low word
#define RTGI_STAT_LUMINANCE_MAX 18u
#define RTGI_STAT_CLAMPED 19u
#define RTGI_STAT_LUMINANCE_SUM_HI 23u  // high word of the 64-bit luminance sum (carry from index 17)
#define RTGI_LUMINANCE_STAT_CAP 1000.0  // per-sample cap of the luminance statistic only (not of the GI)
#define RTGI_STAT_TRACE_COUNT 20u
#define RTGI_FLAG_RADIANCE_VALID 1u
#define RTGI_FLAG_SUBTRACT_PREVIOUS 2u
#define RTGI_FLAG_SUN_CASTERS_ONLY 4u
#define RTGI_FLAG_FALLBACK_NO_ALBEDO 8u
#define RTGI_FLAG_DYNAMIC_LIGHTS 16u
#define RTGI_FLAG_GAME_SUN 32u
#define RTGI_FLAG_FEEDBACK 64u
#define RTGI_FLAG_REAL_RAYS 128u
#define RTGI_LAMP_COUNT 1024u
#define RTGI_FALLBACK_ALBEDO 0.5

// The G-buffer colour at a screen UV (this frame's lighting colorTexture).
float3 BounceSurfaceAt(float2 uv)
{
    uint aw;
    uint ah;
    g_rtgi_albedo.GetDimensions(aw, ah);
    return g_rtgi_albedo.Load(int3(min(uint2(uv * float2(aw, ah)), uint2(aw, ah) - 1u), 0)).rgb;
}

// O1 sun term at a screen UV: the game's sun lerp (lighting lines 1124-1126 and 1219-1221) minus its shadow floor,
// times the surface colour. k = mrtTexture1.w / 255 (line 782) x the screen shadow (line 770),
// floor = min(1, sceneShadowColor_g) (lines 140-141; the material shadowColor is not added). Cloud shadow, the
// shadow UV clamp, the penumbra tint, Hammon and contact shadows are not applied.
float3 SunGiAt(float2 uv, float3 surface)
{
    uint mw;
    uint mh;
    g_mrt1.GetDimensions(mw, mh);
    const float k = (float)g_mrt1.Load(int3(min(uint2(uv * float2(mw, mh)), uint2(mw, mh) - 1u), 0)).w / 255.0;
    uint sw;
    uint sh;
    g_shadow_mask.GetDimensions(sw, sh);
    const float shadow = g_shadow_mask.Load(int3(min(uint2(uv * float2(sw, sh)), uint2(sw, sh) - 1u), 0)).x;
    const float3 floor_term = min(float3(1.0, 1.0, 1.0), sceneShadowColor_g);
    return surface * lightColor_g * (1.0 - floor_term) * (k * shadow);
}

// Sum of the game's point and spot diffuse of the lamps with a type bit (lighting lines 1505-1664 without the Hammon
// correction and shadow maps). The caller applies the albedo and the lamp contribution.
float3 LampDiffuseSum(float3 position, float3 n)
{
    float3 lamp_sum = float3(0.0, 0.0, 0.0);
    for (uint li = 0u; li < RTGI_LAMP_COUNT; ++li)
    {
        const uint lamp_type = g_rtgi_lamp_types[li];
        const LightParam lamp = g_rtgi_lamps[li];
        if (lamp.radius <= 0.0 || lamp_type == 0u)
        {
            continue;
        }
        const float3 to_lamp = lamp.pos - position;
        const float lamp_att = max(0.0,
            1.0 - pow(abs(length(to_lamp) * lamp.radiusInv), lamp.attenuation));
        const float3 lamp_dir = normalize(to_lamp);
        const float lamp_ndl = max(lamp.translucency, dot(n, lamp_dir));
        if ((lamp_type & RTGI_LAMP_TYPE_POINT) != 0u)
        {
            lamp_sum += lamp.color * lamp_att * lamp_ndl;
        }
        if ((lamp_type & RTGI_LAMP_TYPE_SPOT) != 0u)
        {
            const float lamp_cone = max(0.0,
                1.0 - pow((1.0 - max(0.0, dot(lamp_dir, lamp.vec))) * lamp.spotAngleInv,
                          lamp.attenuationAngle));
            lamp_sum += lamp.color * lamp_att * lamp_cone * lamp_ndl;
        }
    }
    return lamp_sum;
}

groupshared uint gs_stats[RTGI_STAT_TRACE_COUNT];

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID, uint3 gid : SV_GroupThreadID)
{
    // Characters (t10-t13): c6.y objects, 0 unless Trace characters is on. P2b record count: c2.w (uint bits).
    g_trace_dynamic_count = (uint)g_rtgi_sun.y;
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
        const float3 N = DecodeFalcomMrtNormalAsGame(g_mrt_normal.Load(int3(px, 0)).xy);
        if (any(resolutionScaling_g != float2(1.0, 1.0)))
        {
            InterlockedAdd(gs_stats[RTGI_STAT_SCALED], 1u);
            if (debug_mode == 8)
            {
                debug_color = float3(0.5, 0.5, 0.5);
            }
        }
        else if (depth <= 0.0)
        {
            InterlockedAdd(gs_stats[RTGI_STAT_SKY], 1u);
            if (debug_mode == 8)
            {
                debug_color = float3(0.0, 0.0, 0.5);
            }
        }
        else if (dot(N, N) < 0.5)
        {
            InterlockedAdd(gs_stats[RTGI_STAT_NORMAL], 1u);
            if (debug_mode == 8)
            {
                debug_color = float3(1.0, 0.0, 1.0);
            }
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
                if (debug_mode == 8)
                {
                    debug_color = float3(0.0, 0.5, 0.0);
                }
            }
            else if (distance_to_camera >= g_rtgi_fade.y)
            {
                InterlockedAdd(gs_stats[RTGI_STAT_BEYOND_FADE], 1u);
                traced = true;  // Valid GI (0): no rays beyond Fade End; the debug views keep the not-traced colour.
            }
            else
            {
                traced = true;
                const uint flags = (uint)g_rtgi_fade.z;
                const uint prev_row = (uint)g_rtgi_shading.w - RTGI_SCENE_ROWS_FIRST;
                uint lookups = 0u;
                uint sun_rays = 0u;
                uint sun_visible_count = 0u;
                uint luminance_sum = 0u;
                uint luminance_max = 0u;
                uint clamped = 0u;
                float3 gi_sum = float3(0.0, 0.0, 0.0);
                const float3 origin = P + N * g_rtgi_params.y;
                // Real Extra Rays: bounces b = 1 .. bounce_rays follow the ray from the previous hit (at most 2).
                const uint bounce_rays = (flags & RTGI_FLAG_REAL_RAYS) != 0u ? min((uint)g_rtgi_dynamic.z, 2u) : 0u;
                // GI rays skip surfaces the game camera does not show (shadow-only casters, near-faded surfaces), as RTAO.
                const WorldCameraView view = {true, 1.0, 0u, 0u};
                uint hits = 0u;
                uint on_screen = 0u;
                uint occluded = 0u;
                uint off_screen = 0u;
                uint distance_dm = 0u;
                uint invalid_refs = 0u;
                uint stack_overflow = 0u;
                float t_sum = 0.0;
                float min_t = ray_max;
                for (uint i = 0u; i < spp; ++i)
                {
                    float3 ray_origin = origin;
                    float3 ray_N = N;
                    float3 throughput = float3(1.0, 1.0, 1.0);
                    float3 extra = float3(0.0, 0.0, 0.0);
                    float3 radiance_0 = float3(0.0, 0.0, 0.0);
                    for (uint b = 0u; b <= bounce_rays; ++b)
                    {
                        const float3 up = abs(ray_N.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
                        const float3 T = normalize(cross(up, ray_N));
                        const float3 B = cross(ray_N, T);
                        // Bounce b draws its own noise: the sample index is offset by b x spp.
                        float2 u;
                        if (g_rtgi_sampling.x > 0.5)
                        {
                            u = g_isfast_noise.Load(int4((tp.x + 64u) % 128u, (tp.y + 64u) % 128u,
                                ((uint)g_rtgi_sampling.y + i + b * spp) % 32u, 0));
                        }
                        else
                        {
                            const float2 q = float2(tp) + g_rtgi_sampling.z + 13.0 + float(i + b * spp) * 7.0;
                            u = frac(52.9829189 * frac(float2(dot(q, float2(0.06711056, 0.00583715)), dot(q + 47.0, float2(0.06711056, 0.00583715)))));
                        }
                        const float r = sqrt(u.x);
                        const float phi = 6.2831853 * u.y;
                        const float3 dir = T * (r * cos(phi)) + B * (r * sin(phi)) + ray_N * sqrt(max(0.0, 1.0 - u.x));

                        WorldTraceCounters counters;
                        const WorldTraceHit hit = TraceWorldClosest(ray_origin, dir, 1e-3, ray_max, view, counters);
                        invalid_refs += counters.invalid_refs;
                        stack_overflow += counters.stack_overflow;
                        if (hit.prim == 0xFFFFFFFFu)
                        {
                            // A miss ends the sample: a later bounce that misses adds no light.
                            break;
                        }
                        // Hit counters (debug views 1, 2 and 8) describe the first bounce only.
                        if (b == 0u)
                        {
                            hits += 1u;
                            t_sum += hit.t;
                            min_t = min(min_t, hit.t);
                            distance_dm += (uint)(hit.t * 10.0);
                        }
                        // On screen: the hit projects inside the image and the game depth there is the hit (2% or 5 cm, the
                        // RTAO discovery tolerance). Hidden on screen: it projects inside but another surface is in front.
                        const float hit_distance = length(hit.position - camera);
                        const float4 clip = mul(float4(hit.position, 1.0), viewProj_g);
                        const float2 hit_uv = float2(clip.x / clip.w * 0.5 + 0.5, 0.5 - clip.y / clip.w * 0.5);
                        bool visible = false;
                        if (clip.w <= 1e-6 || any(hit_uv < 0.0) || any(hit_uv >= 1.0))
                        {
                            if (b == 0u)
                            {
                                off_screen += 1u;
                            }
                        }
                        else
                        {
                            const uint2 q = min(uint2(hit_uv * float2(full_size)), full_size - 1u);
                            const float q_depth = g_game_depth.Load(int3(q, 0)).x;
                            const float2 q_uv = (float2(q) + 0.5) / float2(full_size);
                            const float4 q_h = mul(float4(q_uv.x * 2.0 - 1.0, 1.0 - q_uv.y * 2.0, q_depth, 1.0), viewProjInv_g);
                            visible = q_depth > 0.0
                                && abs(length(q_h.xyz / q_h.w - camera) - hit_distance) <= max(0.02 * hit_distance, 0.05);
                            if (b == 0u)
                            {
                                on_screen += visible ? 1u : 0u;
                                occluded += visible ? 0u : 1u;
                            }
                        }
                        const float3 n = dot(hit.normal, dir) > 0.0 ? -hit.normal : hit.normal;
                        float3 radiance = float3(0.0, 0.0, 0.0);
                        bool looked_up = false;
                        const float fallback_albedo = (flags & RTGI_FLAG_FALLBACK_NO_ALBEDO) != 0u ? 1.0 : RTGI_FALLBACK_ALBEDO;
                        if (visible && (flags & RTGI_FLAG_RADIANCE_VALID) != 0u)
                        {
                            // The copy is last frame's lighting output: look the hit up where the previous camera saw it.
                            // = mul(float4(p, 1), prevViewProj_g): cbuffer matrices are column-major, so register r + j is column j.
                            const float4 hit_h = float4(hit.position, 1.0);
                            const float4 prev_clip = float4(dot(hit_h, g_scene_rows[prev_row]), dot(hit_h, g_scene_rows[prev_row + 1u]),
                                                            dot(hit_h, g_scene_rows[prev_row + 2u]), dot(hit_h, g_scene_rows[prev_row + 3u]));
                            const float2 prev_uv = float2(prev_clip.x / prev_clip.w * 0.5 + 0.5, 0.5 - prev_clip.y / prev_clip.w * 0.5);
                            if (prev_clip.w > 1e-6 && all(prev_uv >= 0.0) && all(prev_uv < 1.0))
                            {
                                uint rw;
                                uint rh;
                                g_rtgi_radiance.GetDimensions(rw, rh);
                                radiance = g_rtgi_radiance.Load(int3(min(uint2(prev_uv * float2(rw, rh)), uint2(rw, rh) - 1u), 0)).rgb;
                                if ((flags & RTGI_FLAG_SUBTRACT_PREVIOUS) != 0u)
                                {
                                    // What the composite added here last frame: final GI at the previous pixel x colour at the hit.
                                    uint fw;
                                    uint fh;
                                    g_rtgi_prev_final.GetDimensions(fw, fh);
                                    uint aw;
                                    uint ah;
                                    g_rtgi_albedo.GetDimensions(aw, ah);
                                    const float3 applied = g_rtgi_prev_final.Load(int3(min(uint2(prev_uv * float2(fw, fh)), uint2(fw, fh) - 1u), 0)).rgb
                                        * g_rtgi_albedo.Load(int3(min(uint2(hit_uv * float2(aw, ah)), uint2(aw, ah) - 1u), 0)).rgb
                                        * g_rtgi_shading.z;
                                    radiance = max(radiance - applied, 0.0);
                                    if ((flags & RTGI_FLAG_FEEDBACK) != 0u)
                                    {
                                        // Extra Bounces: keep the feedback share of the subtracted bounce, so it builds up over frames.
                                        radiance += g_rtgi_dynamic.y * applied;
                                    }
                                }
                                // O1: scale this on-screen bounce's sun and lamp parts (Sun / Dynamic Light Contribution).
                                // Null views (not Sora 2nd, or no G-buffer colour) read 0, so nothing changes there.
                                if (g_rtgi_sun.z != 1.0 || g_rtgi_dynamic.x != 1.0)
                                {
                                    const float3 surface = BounceSurfaceAt(hit_uv);
                                    if (g_rtgi_sun.z != 1.0)
                                    {
                                        radiance += (g_rtgi_sun.z - 1.0) * SunGiAt(hit_uv, surface);
                                    }
                                    if (g_rtgi_dynamic.x != 1.0)
                                    {
                                        uint mw;
                                        uint mh;
                                        g_mrt2.GetDimensions(mw, mh);
                                        const uint2 material_px = min(uint2(hit_uv * float2(mw, mh)), uint2(mw, mh) - 1u);
                                        const uint material_index = min(0x4e1fu, g_mrt2.Load(int3(material_px, 0)).x);
                                        const float dynamic_light_intensity = g_rtgi_deferred[material_index].mid.w;
                                        radiance += (g_rtgi_dynamic.x - 1.0) * surface * dynamic_light_intensity
                                            * LampDiffuseSum(hit.position, n);
                                    }
                                }
                                looked_up = true;
                                lookups += 1u;
                            }
                        }
                        if (!looked_up)
                        {
                            // Hidden, off screen or no copy: sun with a shadow ray (shadow-only casters block it: hide off) plus
                            // the game's probe ambient (lighting_0xCA3D8596 lines 827-841) and, with RTGI_FLAG_DYNAMIC_LIGHTS, the
                            // lamps, times a grey albedo (RTGI_FALLBACK_ALBEDO; 1.0 with RTGI_FLAG_FALLBACK_NO_ALBEDO).
                            const float3 sun_dir = normalize(-lightDirection_g);
                            const float ndl = saturate(dot(n, sun_dir));
                            float sun = 0.0;
                            if (ndl > 0.0)
                            {
                                const WorldCameraView sun_view = {false, 1.0, 0u,
                                    ((flags & RTGI_FLAG_SUN_CASTERS_ONLY) != 0u ? INSTANCE_SHADOW_CASTER : 0u)};
                                WorldTraceCounters sun_counters;
                                const WorldTraceHit sun_hit = TraceWorldAny(hit.position + n * g_rtgi_params.y, sun_dir, 1e-3, g_rtgi_sun.x, sun_view, sun_counters);
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
                            float3 sun_light = lightColor_g * sun * g_rtgi_sun.z;
                            if ((flags & RTGI_FLAG_GAME_SUN) != 0u)
                            {
                                // The game's shadow floor (min(1, sceneShadowColor_g) x sun colour) is not scaled by Sun
                                // Contribution (g_rtgi_sun.z); only the lit part (1 - floor) is.
                                const float3 shadow_floor = min(float3(1.0, 1.0, 1.0), sceneShadowColor_g);
                                sun_light = lightColor_g
                                    * (shadow_floor + (float3(1.0, 1.0, 1.0) - shadow_floor) * sun * g_rtgi_sun.z);
                            }
                            radiance = fallback_albedo * (sun_light + g_rtgi_shading.x * ambient);
                            if ((flags & RTGI_FLAG_DYNAMIC_LIGHTS) != 0u)
                            {
                                radiance += fallback_albedo * g_rtgi_dynamic.x * LampDiffuseSum(hit.position, n);
                            }
                        }
                        if (b == 0u)
                        {
                            radiance_0 = radiance;
                        }
                        else
                        {
                            extra += throughput * radiance;
                        }
                        throughput *= (visible ? BounceSurfaceAt(hit_uv)
                            : float3(fallback_albedo, fallback_albedo, fallback_albedo));
                        ray_origin = hit.position + n * g_rtgi_params.y;
                        ray_N = n;
                    }
                    float3 radiance = radiance_0 + extra;
                    if (any(isnan(radiance)) || any(isinf(radiance)))
                    {
                        radiance = float3(0.0, 0.0, 0.0);
                    }
                    radiance = max(radiance, 0.0);
                    const float luminance = dot(radiance, float3(0.2126, 0.7152, 0.0722));
                    luminance_sum += (uint)(min(luminance, RTGI_LUMINANCE_STAT_CAP) * 10.0);
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
                else if (debug_mode == 8)
                {
                    // Pixel classes: the normal faces away from the camera; a hit within 1 cm; neither.
                    if (dot(N, camera - P) < 0.0)
                    {
                        debug_color = float3(1.0, 1.0, 0.0);
                    }
                    else if (hits > 0u && min_t < 0.01)
                    {
                        debug_color = float3(0.0, 1.0, 1.0);
                    }
                    else
                    {
                        debug_color = float3(0.0, 0.0, 0.0);
                    }
                }
                else if (debug_mode == 9)
                {
                    // O1 sun term of each pixel's own surface, shown as rgb / (1 + rgb); no Sun Contribution applied.
                    const float2 pixel_uv = (float2(px) + 0.5) / float2(full_size);
                    const float3 sun_estimate = SunGiAt(pixel_uv, BounceSurfaceAt(pixel_uv));
                    debug_color = sun_estimate / (1.0 + sun_estimate);
                }
            }
        }
        InterlockedAdd(gs_stats[RTGI_STAT_PIXELS], 1u);
        if ((debug_mode >= 1 && debug_mode <= 3) || debug_mode == 8 || debug_mode == 9)
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
        // a: 1 = GI value valid (traced or beyond Fade End), -1 = no surface or outside the region.
        g_rtgi_raw[tp] = traced ? float4(gi, 1.0) : float4(0.0, 0.0, 0.0, -1.0);
    }
    GroupMemoryBarrierWithGroupSync();
    if (group_index < RTGI_STAT_TRACE_COUNT)
    {
        if (group_index == RTGI_STAT_LUMINANCE_MAX)
        {
            InterlockedMax(g_rtgi_stats[group_index], gs_stats[group_index]);
        }
        else if (group_index == RTGI_STAT_LUMINANCE_SUM)
        {
            // 64-bit sum: carry into the high word when the low word wraps.
            uint before;
            InterlockedAdd(g_rtgi_stats[RTGI_STAT_LUMINANCE_SUM], gs_stats[group_index], before);
            if (before > 0xFFFFFFFFu - gs_stats[group_index])
            {
                InterlockedAdd(g_rtgi_stats[RTGI_STAT_LUMINANCE_SUM_HI], 1u);
            }
        }
        else
        {
            InterlockedAdd(g_rtgi_stats[group_index], gs_stats[group_index]);
        }
    }
}
