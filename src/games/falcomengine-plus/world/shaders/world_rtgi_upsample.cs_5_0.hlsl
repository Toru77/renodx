// world_rtgi_upsample.cs_5_0.hlsl - RTGI upsample to the depth size (world/docs/ROADMAP.md, RTGI M3b).
//
// One thread per full pixel. Joint bilateral upsample of the trace-resolution GI (t2; a <= 0 = not traced): the 2x2
// trace pixels around the pixel's position in trace space, each weighted by its bilinear weight times
// FilterPlaneNormalWeight (world_rt_denoise.hlsli) between this pixel and the tap's representative pixel
// min(tq * d + d / 2, size - 1). With d = 1 it is a pass-through. Writes the final GI (u0, r11g11b10_float; 0 where
// there is no surface or no valid tap) and debug views 6 and 7 (u1). View 7 (Applied GI) is what the Sora 2nd composite
// adds: final GI x the G-buffer colour (t3) x GI Intensity, with Character GI Intensity where MRT0.w has bit 3 clear and
// bit 0 set (the lighting shader's csIsCharacter and csCharacterShading).

#include "../../include/mrt_normal.hlsli"
#include "world_rt_denoise.hlsli"

Texture2D<float4> g_game_depth : register(t0);
Texture2D<uint4> g_mrt_normal : register(t1);
Texture2D<float4> g_in : register(t2);
Texture2D<float4> g_albedo : register(t3);  // the lighting draw's colorTexture (G-buffer colour), for Debug 7

RWTexture2D<float4> g_final : register(u0);
RWTexture2D<float4> g_rtgi_debug : register(u1);

cbuffer cb_scene : register(b0)
{
    float4x4 viewInv_g : packoffset(c4);
    float4x4 viewProjInv_g : packoffset(c20);
    float2 resolutionScaling_g : packoffset(c49);
};

// Push constants (rtgi.hpp fills these; 12 floats).
cbuffer cb_rtgi_upsample : register(b12)
{
    float4 u_size : packoffset(c0);      // full width, full height, trace width, trace height
    float4 u_params : packoffset(c1);    // trace divisor d, debug mode, GI Intensity, Character GI Intensity (Debug 7)
    float4 u_reserved : packoffset(c2);  // unused
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

    const float depth = g_game_depth.Load(int3(px, 0)).x;
    const float3 N = DecodeFalcomMrtNormal(g_mrt_normal.Load(int3(px, 0)).xy);
    const bool surface = depth > 0.0 && dot(N, N) >= 0.5 && all(resolutionScaling_g == float2(1.0, 1.0));
    float3 gi = float3(0.0, 0.0, 0.0);
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
        [unroll]
        for (int t = 0; t < 4; ++t)
        {
            const int2 offset = int2(t & 1, t >> 1);
            const float w = (offset.x == 1 ? f.x : 1.0 - f.x) * (offset.y == 1 ? f.y : 1.0 - f.y);
            const int2 tq = clamp(base + offset, int2(0, 0), trace_size - 1);
            const float4 v = g_in[tq];
            if (v.a <= 0.0) continue;
            const uint2 rep = min(uint2(tq) * d + d / 2u, full_size - 1u);
            const float rep_depth = g_game_depth.Load(int3(rep, 0)).x;
            const float3 Nt = DecodeFalcomMrtNormal(g_mrt_normal.Load(int3(rep, 0)).xy);
            if (rep_depth <= 0.0 || dot(Nt, Nt) < 0.5) continue;
            const float2 rep_uv = (float2(rep) + 0.5) / float2(full_size);
            const float4 rep_h = mul(float4(rep_uv.x * 2.0 - 1.0, 1.0 - rep_uv.y * 2.0, rep_depth, 1.0), viewProjInv_g);
            const float wt = w * FilterPlaneNormalWeight(Pc, N, dist_c, rep_h.xyz / rep_h.w, Nt);
            sum_w += wt;
            sum_v += wt * v.rgb;
        }
        if (sum_w > 1e-6)
        {
            gi = sum_v / sum_w;
        }
    }
    g_final[px] = float4(gi, 1.0);
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
