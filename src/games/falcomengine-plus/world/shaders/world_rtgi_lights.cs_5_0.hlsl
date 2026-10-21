// world_rtgi_lights.cs_5_0.hlsl - RTGI lamp types (M4, world/docs/ROADMAP.md, Off-screen Dynamic Lights).
//
// One thread per tile cell of the game's lightIndices_g (Sora 2nd lighting 0xCA3D8596, bound at t0; 4608 cells of 576
// bytes, 64 threads per group). Each cell lists the lamps the game lights in that tile: pointLightIndices and
// spotLightIndices, each read up to 63 entries as the game caps them. Every listed lamp below the lamp count sets
// bit 1 (point) or bit 2 (spot) in u0, one uint per lamp, with InterlockedOr. The type comes from these lists only,
// never from spotAngleInv (a point lamp can carry spot values). The host zeroes u0 before the dispatch (rtgi.hpp).
// The lamp count (b12 c0.x) is the lamp buffer's entry count, capped at 1024 by the host.

#include "world_rt_lamps.hlsli"

struct LightIndexData
{
    int pointLightIndices[63];     // Offset:    0
    uint pointLightCount;          // Offset:  252
    int spotLightIndices[63];      // Offset:  256
    uint spotLightCount;           // Offset:  508
    int lightProbeIndices[14];     // Offset:  512
    uint lightProbeCount;          // Offset:  568
    float tileDepthInv;            // Offset:  572
};

StructuredBuffer<LightIndexData> g_light_indices : register(t0);
RWStructuredBuffer<uint> g_lamp_types : register(u0);

cbuffer cb_rtgi_lamps : register(b12)
{
    float g_lamp_count : packoffset(c0);
};

#define RTGI_LAMP_TILE_LIST_CAP 63u
#define RTGI_LAMP_TILE_CELLS 4608u  // cells of lightIndices_g (kRtgiLightTileCount in rtgi_resources.hpp)

[numthreads(64, 1, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= RTGI_LAMP_TILE_CELLS) return;
    const LightIndexData tile = g_light_indices[id.x];
    const uint point_count = min(tile.pointLightCount, RTGI_LAMP_TILE_LIST_CAP);
    for (uint i = 0u; i < point_count; ++i)
    {
        const uint lamp = (uint)tile.pointLightIndices[i];
        if (lamp < (uint)g_lamp_count)
        {
            InterlockedOr(g_lamp_types[lamp], RTGI_LAMP_TYPE_POINT);
        }
    }
    const uint spot_count = min(tile.spotLightCount, RTGI_LAMP_TILE_LIST_CAP);
    for (uint j = 0u; j < spot_count; ++j)
    {
        const uint lamp = (uint)tile.spotLightIndices[j];
        if (lamp < (uint)g_lamp_count)
        {
            InterlockedOr(g_lamp_types[lamp], RTGI_LAMP_TYPE_SPOT);
        }
    }
}
