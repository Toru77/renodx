// world_rtgi_upsample.cs_5_0.hlsl - RTGI upsample to the depth size (world/docs/ROADMAP.md, RTGI M3b).
//
// One thread per full pixel. Algorithm 0 preserves the existing 2x2 joint-bilateral upsample. Algorithms 1 and 2 use
// the same 2x2 taps for bilinear and nearest-valid reconstruction; algorithm 3 uses a 4x4 joint-bilateral footprint.
// Every tap is checked against its representative full-resolution surface. With d = 1, every algorithm uses the
// existing pass-through path. Writes the final GI (u0, r11g11b10_float; 0 where there is no surface or valid tap) and
// debug views 6 and 7 (u1). RTGI_UPSAMPLE_AO_BYTE writes the AO byte to r32_uint; no surface or valid tap = 255.

#include "../../include/mrt_normal.hlsli"
#include "world_rt_denoise.hlsli"

Texture2D<float4> g_game_depth : register(t0);
Texture2D<uint4> g_mrt_normal : register(t1);
Texture2D<float4> g_in : register(t2);
Texture2D<float4> g_albedo : register(t3);  // the lighting draw's colorTexture (G-buffer colour), for Debug 7

#if defined(RTGI_UPSAMPLE_AO_BYTE)
RWTexture2D<uint> g_final : register(u0);
#else
RWTexture2D<float4> g_final : register(u0);
#endif
RWTexture2D<float4> g_rtgi_debug : register(u1);

cbuffer cb_scene : register(b0)
{
    float4x4 viewInv_g : packoffset(c4);
    float4x4 viewProjInv_g : packoffset(c20);
    float2 resolutionScaling_g : packoffset(c49);
};

// Push constants (rtgi.hpp fills these; 16 floats).
cbuffer cb_rtgi_upsample : register(b12)
{
    float4 u_size : packoffset(c0);      // full width, full height, trace width, trace height
    float4 u_params : packoffset(c1);    // trace divisor d, debug mode, GI Intensity, Character GI Intensity (Debug 7)
    float4 u_extra : packoffset(c2);     // c2: GI Saturation, algorithm, depth-plane sigma, normal edge power
    float4 u_upscale : packoffset(c3);   // c3: bilinear weight sharpness, nearest-valid fallback, unused, unused
};

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    const uint2 full_size = uint2(u_size.xy);
    if (any(id.xy >= full_size)) return;
    const uint2 px = id.xy;
    const int2 trace_size = int2(u_size.zw);
    const uint d = max((uint)u_params.x, 1u);
    const int debug_mode = (int)u_params.y;
    const int requested_algorithm = clamp((int)u_extra.y, 0, 3);
    const int algorithm = d == 1u ? 0 : requested_algorithm;
    const float plane_sigma = max(u_extra.z, 1e-6);
    const float normal_power = max(u_extra.w, 0.0);
    const float bilinear_sharpness = max(u_upscale.x, 0.01);
    const bool nearest_fallback = u_upscale.y > 0.5;

    const float depth = g_game_depth.Load(int3(px, 0)).x;
    const float3 N = DecodeFalcomMrtNormalAsGame(g_mrt_normal.Load(int3(px, 0)).xy);
    const bool surface = depth > 0.0 && dot(N, N) >= 0.5 && all(resolutionScaling_g == float2(1.0, 1.0));
#if defined(RTGI_UPSAMPLE_AO_BYTE)
    float3 gi = float3(1.0, 1.0, 1.0);  // AO: no surface or no valid tap = no occlusion
#else
    float3 gi = float3(0.0, 0.0, 0.0);
#endif
    bool valid = false;
#if defined(RTGI_UPSAMPLE_FINAL)
    if (surface)
    {
        const float4 v = g_in[px];
        if (v.a >= 0.0) gi = v.rgb;
    }
#else
    if (surface)
    {
        const float2 uv = (float2(px) + 0.5) / float2(full_size);
        const float4 world_h = mul(float4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, depth, 1.0), viewProjInv_g);
        const float3 Pc = world_h.xyz / world_h.w;
        const float dist_c = length(Pc - float3(viewInv_g._m30, viewInv_g._m31, viewInv_g._m32));
        // Position in trace space: trace pixel tq is centred on its representative pixel tq * d + d / 2.
        const float2 lp = (float2(px) - float(d / 2u)) / float(d);
        const int2 base = (int2)floor(lp);
        const float2 f = lp - floor(lp);
        float sum_w = 0.0;
        float3 sum_v = float3(0.0, 0.0, 0.0);
#if defined(RTGI_UPSAMPLE_NEAREST_FALLBACK)
        // GTVBAO and GTVBGI use this only when their fallback setting is on and all bilateral weights are edge-stopped.
        float best_w = -1.0;
        float3 best_v = float3(0.0, 0.0, 0.0);
#endif
        float best_joint_w = -1.0;
        float3 best_joint_v = float3(0.0, 0.0, 0.0);
        if (algorithm == 3)
        {
            [unroll]
            for (int y = 0; y < 4; ++y)
            {
                [unroll]
                for (int x = 0; x < 4; ++x)
                {
                    const int2 candidate = base + int2(x - 1, y - 1);
                    const float2 delta = float2(candidate) - lp;
                    const float w = exp(-0.5 * dot(delta, delta));
                    const int2 tq = clamp(candidate, int2(0, 0), trace_size - 1);
                    const float4 v = g_in[tq];
                    if (v.a < 0.0) continue;
                    const uint2 rep = RtTraceRepresentativePixel(uint2(tq), d, full_size);
                    const float rep_depth = g_game_depth.Load(int3(rep, 0)).x;
                    const float3 Nt = DecodeFalcomMrtNormalAsGame(g_mrt_normal.Load(int3(rep, 0)).xy);
                    if (RtUpsampleTapRejected(rep_depth, Nt)) continue;
#if defined(RTGI_UPSAMPLE_NEAREST_FALLBACK)
                    if (w > best_w)
                    {
                        best_w = w;
                        best_v = v.rgb;
                    }
#endif
                    const float2 rep_uv = (float2(rep) + 0.5) / float2(full_size);
                    const float4 rep_h = mul(float4(rep_uv.x * 2.0 - 1.0, 1.0 - rep_uv.y * 2.0, rep_depth, 1.0), viewProjInv_g);
                    const float wt = w * FilterPlaneNormalWeight(Pc, N, dist_c, rep_h.xyz / rep_h.w, Nt, plane_sigma, normal_power);
                    sum_w += wt;
                    sum_v += wt * v.rgb;
                }
            }
        }
        else
        {
            [unroll]
            for (int t = 0; t < 4; ++t)
            {
#if defined(RTGI_UPSAMPLE_AO_BYTE) && defined(RTGI_UPSAMPLE_NEAREST_FALLBACK)
                // d = 1: f = 0, so only tap 0 has a weight (1). Once used, later zero-weight taps cannot replace it.
                if (d == 1u && best_w >= 0.0) continue;
#endif
                const int2 offset = int2(t & 1, t >> 1);
                const float w = (offset.x == 1 ? f.x : 1.0 - f.x) * (offset.y == 1 ? f.y : 1.0 - f.y);
                const int2 tq = clamp(base + offset, int2(0, 0), trace_size - 1);
                const float4 v = g_in[tq];
                if (v.a < 0.0) continue;
                const uint2 rep = RtTraceRepresentativePixel(uint2(tq), d, full_size);
                const float rep_depth = g_game_depth.Load(int3(rep, 0)).x;
                const float3 Nt = DecodeFalcomMrtNormalAsGame(g_mrt_normal.Load(int3(rep, 0)).xy);
                if (RtUpsampleTapRejected(rep_depth, Nt)) continue;
#if defined(RTGI_UPSAMPLE_NEAREST_FALLBACK)
                if (w > best_w)
                {
                    best_w = w;
                    best_v = v.rgb;
                }
#endif
                float sample_w = w;
                if (algorithm != 0 && bilinear_sharpness != 1.0) sample_w = pow(max(w, 0.0), bilinear_sharpness);
                if (algorithm == 1)
                {
                    sum_w += sample_w;
                    sum_v += sample_w * v.rgb;
                    continue;
                }
                const float2 rep_uv = (float2(rep) + 0.5) / float2(full_size);
                const float4 rep_h = mul(float4(rep_uv.x * 2.0 - 1.0, 1.0 - rep_uv.y * 2.0, rep_depth, 1.0), viewProjInv_g);
                const float edge_weight = FilterPlaneNormalWeight(Pc, N, dist_c, rep_h.xyz / rep_h.w, Nt, plane_sigma, normal_power);
                if (algorithm == 2)
                {
                    const float joint_w = sample_w * edge_weight;
                    if (joint_w > best_joint_w)
                    {
                        best_joint_w = joint_w;
                        best_joint_v = v.rgb;
                    }
                }
                else
                {
                    const float wt = w * edge_weight;
                    sum_w += wt;
                    sum_v += wt * v.rgb;
                }
            }
        }
        if (algorithm == 2 && best_joint_w > 1e-6)
        {
            gi = best_joint_v;
            valid = true;
        }
        else if (sum_w > 1e-6)
        {
            gi = sum_v / sum_w;
            valid = true;
        }
#if defined(RTGI_UPSAMPLE_NEAREST_FALLBACK)
        else if (algorithm != 1 && nearest_fallback && best_w >= 0.0)
        {
            gi = best_v;
            valid = true;
        }
#endif
    }
#endif
#if defined(RTGI_UPSAMPLE_SVGF_INPUT)
    g_final[px] = float4(gi, valid ? 1.0 : -1.0);
    return;
#endif
    if (u_extra.x != 1.0)
    {
        float luma = dot(gi, float3(0.2126, 0.7152, 0.0722));
        gi = max(0.0, luma + (gi - luma) * u_extra.x);
    }
#if defined(RTGI_UPSAMPLE_AO_BYTE)
    g_final[px] = (uint)(saturate(gi.r) * 255.0 + 0.5);
#else
    g_final[px] = float4(gi, 1.0);
#endif
    if (debug_mode == 6)
    {
        // Not traced (no surface): dark purple, as in the other debug views.
        g_rtgi_debug[px] = float4(surface ? gi / (1.0 + gi) : float3(0.25, 0.0, 0.25), 1.0);
    }
    else if (debug_mode == 7)
    {
        uint aw;
        uint ah;
        g_albedo.GetDimensions(aw, ah);
        const uint flags = g_mrt_normal.Load(int3(px, 0)).w;
        const float3 applied = gi
            * g_albedo.Load(int3(min(uint2((float2(px) + 0.5) / float2(full_size) * float2(aw, ah)), uint2(aw, ah) - 1u), 0)).rgb
            * u_params.z * ((flags & 8u) == 0u && (flags & 1u) != 0u ? saturate(u_params.w) : 1.0);
        // No surface, or no colour captured this frame (t3 null, width 0): dark purple, as in the other debug views.
        g_rtgi_debug[px] = float4(surface && aw > 0u ? applied / (1.0 + applied) : float3(0.25, 0.0, 0.25), 1.0);
    }
}
