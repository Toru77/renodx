// world_rt12_records.cs_6_5.hlsl - M5: one D3D12_RAYTRACING_INSTANCE_DESC per P2b record (rt12_backend.hpp RunRtaoPassA
// 13.1c) and the record itself into the instance table at g_table_base + r (InstanceID), so the RTAO trace reads it like
// a static instance. The packing is transcribed on the CPU as rt12::Rt12RecordDescWords (harness TestRt12Records).
// A record that is invalid (header.w < 0.5), whose slot is out of range or has no D3D12 BLAS gets mask 0 and a null
// BLAS address (an inactive instance); otherwise the mask follows MakeRt12InstanceFields: not camera-visible -> 4,
// foliage or alpha-tested -> 2, else 1 (RTAO's rays never include 4: the compute SKIP_HIDDEN rule).
#include "world_bvh_types.hlsli"

Texture2D<uint4> g_records : register(t0);
StructuredBuffer<uint2> g_blas_addresses : register(t1);  // per live-store slot, (0, 0) = no BLAS
RWByteAddressBuffer g_descs : register(u0);
RWStructuredBuffer<WorldInstanceGPU> g_instances : register(u1);

cbuffer cb_records : register(b0)
{
    uint g_record_count;
    uint g_table_base;  // S: the static instance array size (InstanceID of record r = S + r)
    uint g_desc_base;   // descs before the records (static + characters)
    uint g_slot_count;  // entries in g_blas_addresses
};

float4 RecordTexel(uint r, uint j)
{
    const uint t = r * 12u + j;
    return asfloat(g_records.Load(int3(t % 4096u, t / 4096u, 0)));
}

[numthreads(64, 1, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    const uint r = id.x;
    if (r >= g_record_count) return;
    WorldInstanceGPU record;
    record.header = RecordTexel(r, 0u);
    [unroll] for (uint k = 0u; k < 4u; ++k)
    {
        record.world[k] = RecordTexel(r, 1u + k);
        record.inverse_world[k] = RecordTexel(r, 5u + k);
    }
    record.bounds_min = RecordTexel(r, 9u);
    record.bounds_max = RecordTexel(r, 10u);
    record.visibility = RecordTexel(r, 11u);
    g_instances[g_table_base + r] = record;

    const uint slot = (uint)record.header.x;
    const bool valid = record.header.w >= 0.5;
    const uint2 address = (valid && slot < g_slot_count) ? g_blas_addresses[slot] : uint2(0u, 0u);
    const bool active = address.x != 0u || address.y != 0u;
    const uint flags = (uint)(record.visibility.z + 0.5);
    uint mask = (flags & 2u) == 0u ? 4u : ((flags & 24u) != 0u ? 2u : 1u);
    if (!active) mask = 0u;
    const bool opaque = (uint)(record.header.z + 0.5) == 0u;
    const uint instance_flags = 0x1u | (opaque ? 0x4u : 0x8u);
    const uint offset = (g_desc_base + r) * 64u;
    g_descs.Store4(offset, asuint(record.world[0]));
    g_descs.Store4(offset + 16u, asuint(record.world[1]));
    g_descs.Store4(offset + 32u, asuint(record.world[2]));
    g_descs.Store4(offset + 48u, uint4((g_table_base + r) | (mask << 24u), instance_flags << 24u, address.x, address.y));
}
