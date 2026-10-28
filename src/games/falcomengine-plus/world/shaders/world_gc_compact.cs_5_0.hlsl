// world_gc_compact.cs_5_0.hlsl - G3/G4: writes every live entry of the dst half of the instance set, and this present's
// staged moving records, as WorldInstanceGPU records at [0, out), out counted in GC_COUNTER_OUT (GC_COUNTER_SET still
// counts the set records). One thread per set index (cb_gc_instances: dst base, capacity, moving capacity).

#include "world_gc_instance.hlsli"

[numthreads(64, 1, 1)]
void main(uint3 group : SV_GroupID, uint thread : SV_GroupIndex)
{
    const uint capacity = g_gi_c1.y;
    const uint i = group.x * 64u + thread;
    const uint moving = min(g_gc_counters[GC_COUNTER_MOVING_RECORDS], g_gi_c3.x);
    if (i < moving)
    {
        uint moving_index = 0u;
        InterlockedAdd(g_gc_counters[GC_COUNTER_OUT], 1u, moving_index);
        g_gc_records[moving_index] = g_gc_records[capacity + g_gi_c3.x + i];
    }
    if (i >= capacity) return;
    const uint index = g_gi_c0.w + i;
    const uint m = index * 4u;
    if (g_gc_meta[m] == 0u) return;
    const GcSetEntry entry = g_gc_entries[index];
    float4 inverse_rows[3];
    WorldAffineInverse(entry.world[0], entry.world[1], entry.world[2], inverse_rows);
    InterlockedAdd(g_gc_counters[GC_COUNTER_SET], 1u);
    uint record_index = 0u;
    InterlockedAdd(g_gc_counters[GC_COUNTER_OUT], 1u, record_index);
    g_gc_records[record_index] = GcMakeRecord(entry.slot, entry.world[0], entry.world[1], entry.world[2], inverse_rows, g_gc_meta[m + 2u], false);
}
