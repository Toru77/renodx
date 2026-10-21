// world_rtao_upsample.cs_5_0.hlsl - RTAO trace upsample to full size (world/docs/ROADMAP.md, RTAO trace resolution).
//
// One thread per full pixel, dispatched only at Trace Resolution Half or Quarter, after pass A wrote the trace-size
// raw AO (t0; -1 = not traced). Joint bilateral upsample: the 2x2 trace pixels around the pixel position in trace
// space, each weighted by its bilinear weight times FilterPlaneNormalWeight (world_rt_denoise.hlsli) between this
// pixel and the tap representative pixel min(tq * d + d / 2, size - 1), as the RTGI upsample does. Taps with -1 are
// ignored. Writes the full-size raw AO (u1, r16_float, -1 = invalid) when output_raw is set (Temporal runs), else the
// 0..255 texel (u0, 255 = neutral), as pass A does at Full. No surface at this pixel, or no valid tap: invalid.

#include "../../include/mrt_normal.hlsli"
#include "world_rt_denoise.hlsli"

Texture2D<float> g_trace_raw : register(t0);
Texture2D<float4> g_game_depth : register(t1);
Texture2D<uint4> g_mrt_normal : register(t2);

RWTexture2D<uint> g_rtao_ao : register(u0);
RWTexture2D<float> g_rtao_raw : register(u1);

cbuffer cb_scene : register(b0)
{
    float4x4 viewInv_g : packoffset(c4);
    float4x4 viewProjInv_g : packoffset(c20);
    float2 resolutionScaling_g : packoffset(c49);
};

// Push constants (rtao.hpp fills these; 8 floats).
cbuffer cb_rtao_upsample : register(b12)
{
    float4 u_size : packoffset(c0);    // full width, full height, trace width, trace height
    float4 u_params : packoffset(c1);  // trace divisor d, output_raw (1 = u1 raw AO, 0 = u0 texel)
};

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    const uint2 full_size = uint2(u_size.xy);
    if (any(id.xy >= full_size)) return;
    const uint2 px = id.xy;
    const int2 trace_size = int2(u_size.zw);
    const uint d = max((uint)u_params.x, 1u);

    const float depth = g_game_depth.Load(int3(px, 0)).x;
    const float3 N = DecodeFalcomMrtNormalAsGame(g_mrt_normal.Load(int3(px, 0)).xy);
    const bool surface = depth > 0.0 && dot(N, N) >= 0.5 && all(resolutionScaling_g == float2(1.0, 1.0));
    float ao = -1.0;
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
        float sum_v = 0.0;
        [unroll]
        for (int t = 0; t < 4; ++t)
        {
            const int2 offset = int2(t & 1, t >> 1);
            const float w = (offset.x == 1 ? f.x : 1.0 - f.x) * (offset.y == 1 ? f.y : 1.0 - f.y);
            const int2 tq = clamp(base + offset, int2(0, 0), trace_size - 1);
            const float v = g_trace_raw[tq];
            if (v < 0.0) continue;
            const uint2 rep = min(uint2(tq) * d + d / 2u, full_size - 1u);
            const float rep_depth = g_game_depth.Load(int3(rep, 0)).x;
            const float3 Nt = DecodeFalcomMrtNormalAsGame(g_mrt_normal.Load(int3(rep, 0)).xy);
            if (rep_depth <= 0.0 || dot(Nt, Nt) < 0.5) continue;
            const float2 rep_uv = (float2(rep) + 0.5) / float2(full_size);
            const float4 rep_h = mul(float4(rep_uv.x * 2.0 - 1.0, 1.0 - rep_uv.y * 2.0, rep_depth, 1.0), viewProjInv_g);
            const float wt = w * FilterPlaneNormalWeight(Pc, N, dist_c, rep_h.xyz / rep_h.w, Nt);
            sum_w += wt;
            sum_v += wt * v;
        }
        if (sum_w > 1e-6)
        {
            ao = sum_v / sum_w;
        }
    }
    if (u_params.y > 0.5)
    {
        g_rtao_raw[px] = ao;
    }
    else
    {
        g_rtao_ao[px] = ao < 0.0 ? 255u : min(255u, (uint)round(saturate(ao) * 255.0));
    }
}
