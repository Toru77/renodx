// world_rt12_carrier.cs_5_0.hlsl - M4: deform arena vertices (float3, typed R32_FLOAT SRV) into the shared RGBA32_UINT
// carrier for D3D12 (rt12_backend.hpp): vertex v at texel (v % 4096, v / 4096), xyz = asuint, w = 0.
Buffer<float> g_arena : register(t10);
RWTexture2D<uint4> g_carrier : register(u0);

cbuffer cb_carrier : register(b12)
{
    uint4 g_carrier_params : packoffset(c0);  // x = vertex count
};

[numthreads(64, 1, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    const uint v = id.y * 65535u * 64u + id.x;
    if (v >= g_carrier_params.x) return;
    const uint3 p = uint3(asuint(g_arena[v * 3u]), asuint(g_arena[v * 3u + 1u]), asuint(g_arena[v * 3u + 2u]));
    g_carrier[uint2(v % 4096u, v / 4096u)] = uint4(p, 0u);
}
