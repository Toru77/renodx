// world_gc_compact.cs_5_0.hlsl - G3: writes every live entry of the dst half of the instance set as a WorldInstanceGPU
// record at [0, n), n counted in GC_COUNTER_SET. One thread per set index (cb_gc_instances: dst base, capacity).
// The moving records of this present stay at [capacity, ...) and are not written here.

#include "world_gc_instance.hlsli"

[numthreads(64, 1, 1)]
void main(uint3 group : SV_GroupID, uint thread : SV_GroupIndex)
{
    const uint capacity = g_gi_c1.y;
    const uint i = group.x * 64u + thread;
    if (i >= capacity) return;
    const uint index = g_gi_c0.w + i;
    const uint m = index * 4u;
    if (g_gc_meta[m] == 0u) return;
    const GcSetEntry entry = g_gc_entries[index];
    float4 inverse_rows[3];
    WorldAffineInverse(entry.world[0], entry.world[1], entry.world[2], inverse_rows);
    uint record_index = 0u;
    InterlockedAdd(g_gc_counters[GC_COUNTER_SET], 1u, record_index);
    g_gc_records[record_index] = GcMakeRecord(entry.slot, entry.world[0], entry.world[1], entry.world[2], inverse_rows, g_gc_meta[m + 2u]);
}
