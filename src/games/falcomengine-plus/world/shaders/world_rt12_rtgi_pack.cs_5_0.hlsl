// world_rt12_rtgi_pack.cs_5_0.hlsl - M2 DXR RTGI inputs (rt12_backend.hpp RunRtgi). Per trace pixel: the MRT normal at its
// representative pixel (the rule of world_rtgi.cs_5_0.hlsl) into a shared trace-size texture. The depth, the radiance,
// the previous final GI and the albedo are copied at their own sizes. Thread (0, 0) also writes the 96 cb_scene rows
// (c0..c95) as texels 0..95.
Texture2D<float4> g_game_depth : register(t0);
Texture2D<uint4> g_mrt_normal : register(t1);
Texture2D<float4> g_radiance_src : register(t2);
Texture2D<float4> g_prev_final_src : register(t3);
Texture2D<float4> g_albedo_src : register(t4);
RWTexture2D<float> g_pack_depth : register(u0);
RWTexture2D<uint4> g_pack_normal : register(u1);
RWTexture2D<float4> g_pack_radiance : register(u2);
RWTexture2D<float4> g_pack_prev_final : register(u3);
RWTexture2D<float4> g_pack_albedo : register(u4);
RWTexture2D<uint4> g_pack_scene : register(u5);

cbuffer cb_scene : register(b0)
{
    float4 g_scene_all[96] : packoffset(c0);
};

cbuffer cb_rtgi_pack : register(b12)
{
    float4 g_pack_params : packoffset(c0);  // x = trace divisor, y = trace width, z = trace height
};

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    if (id.x == 0u && id.y == 0u)
    {
        for (uint r = 0u; r < 96u; ++r) g_pack_scene[uint2(r, 0u)] = asuint(g_scene_all[r]);
    }
    uint2 depth_size;
    g_game_depth.GetDimensions(depth_size.x, depth_size.y);
    if (all(id.xy < depth_size)) g_pack_depth[id.xy] = g_game_depth.Load(int3(id.xy, 0)).x;
    const uint d = max((uint)g_pack_params.x, 1u);
    const uint2 trace_size = uint2(g_pack_params.yz);
    if (all(id.xy < trace_size))
    {
        const uint2 rep = min(id.xy * d + d / 2u, depth_size - 1u);
        g_pack_normal[id.xy] = g_mrt_normal.Load(int3(rep, 0));
    }
    uint2 size;
    g_radiance_src.GetDimensions(size.x, size.y);
    if (all(id.xy < size)) g_pack_radiance[id.xy] = g_radiance_src.Load(int3(id.xy, 0));
    g_prev_final_src.GetDimensions(size.x, size.y);
    if (all(id.xy < size)) g_pack_prev_final[id.xy] = g_prev_final_src.Load(int3(id.xy, 0));
    g_albedo_src.GetDimensions(size.x, size.y);
    if (all(id.xy < size)) g_pack_albedo[id.xy] = g_albedo_src.Load(int3(id.xy, 0));
}
