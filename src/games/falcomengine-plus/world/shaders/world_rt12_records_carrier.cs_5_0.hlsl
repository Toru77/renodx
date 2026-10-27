// world_rt12_records_carrier.cs_5_0.hlsl - M5: the P2b records (bvh_live.hpp ExpandLiveDynamic, WorldInstanceGPU) into
// the shared RGBA32_UINT carrier for D3D12 (rt12_backend.hpp): record r, float4 j (0..11) at texel t = 12 r + j,
// (t % 4096, t / 4096).
#include "world_bvh_types.hlsli"

StructuredBuffer<WorldInstanceGPU> g_records : register(t19);
RWTexture2D<uint4> g_carrier : register(u0);

cbuffer cb_records_carrier : register(b12)
{
    uint4 g_records_params : packoffset(c0);  // x = record count
};

void Put(uint t, float4 value) { g_carrier[uint2(t % 4096u, t / 4096u)] = asuint(value); }

[numthreads(64, 1, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    const uint r = id.x;
    if (r >= g_records_params.x) return;
    const WorldInstanceGPU record = g_records[r];
    const uint t = r * 12u;
    Put(t + 0u, record.header);
    [unroll] for (uint k = 0u; k < 4u; ++k)
    {
        Put(t + 1u + k, record.world[k]);
        Put(t + 5u + k, record.inverse_world[k]);
    }
    Put(t + 9u, record.bounds_min);
    Put(t + 10u, record.bounds_max);
    Put(t + 11u, record.visibility);
}
