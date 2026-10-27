// world_rt12_pack.cs_5_0.hlsl - M3 DXR RTAO inputs (rt12_backend.hpp RunRtaoPassA). Per trace pixel px: the depth and
// the MRT normal at its representative pixel (the rule of world_rtao.cs_5_0.hlsl TracePixel) into shared textures;
// thread (0, 0) also writes the cb_scene rows world_rtao reads: texels 0..3 viewInv_g rows, 4..7 viewProjInv_g rows,
// 8 resolutionScaling_g (xy).
Texture2D<float4> g_game_depth : register(t9);
Texture2D<uint4> g_mrt_normal : register(t17);
RWTexture2D<uint> g_pack_depth : register(u0);
RWTexture2D<uint4> g_pack_normal : register(u1);
RWTexture2D<uint4> g_pack_scene : register(u2);

cbuffer cb_scene : register(b0)
{
    float4x4 viewInv_g : packoffset(c4);
    float4x4 viewProjInv_g : packoffset(c20);
    float2 resolutionScaling_g : packoffset(c49);
};

cbuffer cb_rtao : register(b12)
{
    float4 g_rtao_params : packoffset(c0);
    float4 g_rtao_sampling : packoffset(c1);
    float4 g_rtao_fade : packoffset(c2);
    float4 g_rtao_region : packoffset(c3);
    float4 g_rtao_size : packoffset(c4);
    float4 g_rtao_trace : packoffset(c5);
};

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    const uint trace_d = max((uint)g_rtao_trace.x, 1u);
    const uint2 full_size = uint2(g_rtao_size.xy);
    const uint2 trace_size = (full_size + trace_d - 1u) / trace_d;
    if (id.x == 0u && id.y == 0u)
    {
        [unroll] for (uint r = 0u; r < 4u; ++r)
        {
            g_pack_scene[uint2(r, 0u)] = asuint(viewInv_g[r]);
            g_pack_scene[uint2(4u + r, 0u)] = asuint(viewProjInv_g[r]);
        }
        g_pack_scene[uint2(8u, 0u)] = uint4(asuint(resolutionScaling_g), 0u, 0u);
    }
    if (any(id.xy >= trace_size)) return;
    const uint2 rep = min(id.xy * trace_d + trace_d / 2u, full_size - 1u);
    g_pack_depth[id.xy] = asuint(g_game_depth.Load(int3(rep, 0)).x);
    g_pack_normal[id.xy] = g_mrt_normal.Load(int3(rep, 0));
}
