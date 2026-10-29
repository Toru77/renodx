// world_rt12_rtgi_resolve.cs_5_0.hlsl - M2: the DXR RTGI stats (stats R32_UINT 32x1, texels 0..23) added into the RTGI stats
// buffer. Thread x < 24 adds its texel into the buffer entry of the same index.
Texture2D<uint> g_rt12_rtgi_stats : register(t0);
RWStructuredBuffer<uint> g_rtgi_stats_out : register(u0);

[numthreads(32, 1, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    if (id.x < 24u) InterlockedAdd(g_rtgi_stats_out[id.x], g_rt12_rtgi_stats.Load(int3(id.x, 0, 0)));
}
