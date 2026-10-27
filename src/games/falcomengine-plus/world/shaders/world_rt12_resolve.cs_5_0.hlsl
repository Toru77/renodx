// world_rt12_resolve.cs_5_0.hlsl - M3: the DXR pass A outputs (shared textures) into pass A's own targets: the raw AO
// (u2, output_raw) or the AO texel (u0), at the trace size; group (0, 0) adds the 24 stats into the RTAO stats buffer.
Texture2D<uint> g_rt12_texel : register(t0);
Texture2D<float> g_rt12_raw : register(t1);
Texture2D<uint> g_rt12_stats : register(t2);
RWTexture2D<uint> g_rtao_ao : register(u0);
RWStructuredBuffer<uint> g_rtao_stats : register(u1);
RWTexture2D<float> g_rtao_raw : register(u2);

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
void main(uint3 id : SV_DispatchThreadID, uint3 gid : SV_GroupThreadID, uint3 group : SV_GroupID)
{
    const uint trace_d = max((uint)g_rtao_trace.x, 1u);
    const uint2 trace_size = (uint2(g_rtao_size.xy) + trace_d - 1u) / trace_d;
    if (group.x == 0u && group.y == 0u)
    {
        const uint i = gid.y * 8u + gid.x;
        if (i < 24u) InterlockedAdd(g_rtao_stats[i], g_rt12_stats.Load(int3(i, 0, 0)));
    }
    if (any(id.xy >= trace_size)) return;
    if (g_rtao_size.w > 0.5) g_rtao_raw[id.xy] = g_rt12_raw.Load(int3(id.xy, 0));
    else g_rtao_ao[id.xy] = g_rt12_texel.Load(int3(id.xy, 0));
}
