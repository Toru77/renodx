// world_gc_insert.cs_5_0.hlsl - G3: the instance set of this present (gpu_capture.hpp GcPoseHash, PoolSightingMoves,
// CheckPoolWorld). Modes in cb_gc_instances: clear and rehash run one thread per set index; mark and insert run one
// group per draw of this present (direct and indirect draws, instance slices and indirect args from the G3 arena).
// The C++ reference for every rule is named at its use; the harness (test_gpu_p2b TestGcInstanceShaders) checks them.

#include "world_gc_instance.hlsli"

[numthreads(64, 1, 1)]
void main(uint3 group : SV_GroupID, uint thread : SV_GroupIndex)
{
    const uint mode = g_gi_c0.x;
    const uint frame = g_gi_c0.y;
    const uint src = g_gi_c0.z;
    const uint dst = g_gi_c0.w;
    const uint draw_count = g_gi_c1.x;
    const uint capacity = g_gi_c1.y;
    const float min_scale = asfloat(g_gi_c1.z);
    const float max_scale = asfloat(g_gi_c1.w);
    const float max_position = asfloat(g_gi_c2.x);
    const uint hold = g_gi_c2.y;
    const uint prune_age = g_gi_c2.z;
    const uint max_probes = g_gi_c2.w;
    const uint moving_capacity = g_gi_c3.x;

    if (mode == GC_MODE_CLEAR)
    {
        const uint i = group.x * 64u + thread;
        if (i >= capacity) return;
        for (uint k = 0u; k < 4u; ++k) g_gc_meta[(dst + i) * 4u + k] = 0u;
        return;
    }

    if (mode == GC_MODE_REHASH)
    {
        const uint i = group.x * 64u + thread;
        if (i >= capacity) return;
        const uint m = (src + i) * 4u;
        const uint tag = g_gc_meta[m];
        if (tag == 0u) return;
        const uint last_seen = g_gc_meta[m + 1u];
        const GcSetEntry entry = g_gc_entries[src + i];
        if (frame - last_seen > prune_age)
        {
            InterlockedAdd(g_gc_counters[GC_COUNTER_AGED], 1u);
            return;
        }
        if (entry.epoch != g_gc_epochs[entry.slot])
        {
            InterlockedAdd(g_gc_counters[GC_COUNTER_DROPPED_EPOCH], 1u);
            return;
        }
        const uint moved = g_gc_moving[entry.slot];
        if (moved != 0u && frame + 1u - moved <= hold)
        {
            InterlockedAdd(g_gc_counters[GC_COUNTER_DROPPED_MOVING], 1u);
            return;
        }
        const uint result = GcSetInsert(dst, capacity, max_probes, entry.pose_lo, entry.pose_hi, last_seen, g_gc_meta[m + 2u], entry);
        if (result == 2u)
        {
            InterlockedAdd(g_gc_counters[GC_COUNTER_REHASH_OVERFLOW], 1u);
        }
        else
        {
            InterlockedAdd(g_gc_counters[GC_COUNTER_KEPT], 1u);
        }
        return;
    }

    if (group.x >= draw_count) return;
    const GcInstanceDraw draw = g_gc_draws[group.x];
    uint flags = 0u;
    if ((draw.pass_bits & GC_PASS_CAMERA) != 0u) flags |= 2u;  // kInstanceCameraVisible
    if ((draw.pass_bits & GC_PASS_LIGHT) != 0u) flags |= 4u;   // kInstanceShadowCaster
    if ((draw.pass_bits & GC_PASS_WIND) != 0u) flags |= 8u;    // kInstanceFoliage
    if ((draw.pass_bits & GC_PASS_ALPHA) != 0u) flags |= 16u;  // kInstanceAlphaTested

    // G3b: an indirect draw takes its instance count from the args copied at the draw (index count, instance count, first index,
    // base vertex), with the CPU resolve rules (bvh_pool.hpp, indirect resolve): empty first, then the learned range, then the window.
    uint count = draw.count;
    if (draw.args_float4 != GC_NO_ARGS)
    {
        const uint4 args = g_gc_arena.Load4(draw.args_float4 * 16u);
        const bool report = mode == GC_MODE_INSERT && thread == 0u;
        if (args.x == 0u || args.y == 0u)
        {
            if (report) InterlockedAdd(g_gc_counters[GC_COUNTER_INDIRECT_EMPTY], 1u);
            return;
        }
        if (args.x != draw.index_count || args.z != draw.first_index || asint(args.w) != draw.vertex_offset)
        {
            if (report) InterlockedAdd(g_gc_counters[GC_COUNTER_INDIRECT_MISMATCH], 1u);
            return;
        }
        if (report && args.y > draw.count) InterlockedAdd(g_gc_counters[GC_COUNTER_INDIRECT_TRUNCATED], 1u);
        count = min(args.y, draw.count);
    }

    for (uint i = thread; i < count; i += 64u)
    {
        const uint element = draw.arena_float4 + i * 10u;
        float world[12];
        float prev[12];
        [unroll] for (uint k = 0u; k < 12u; ++k)
        {
            world[k] = asfloat(g_gc_arena.Load((element * 4u + k) * 4u));
            prev[k] = asfloat(g_gc_arena.Load((element * 4u + 12u + k) * 4u));
        }

        // PoolPrevWorldFilled and PoolSightingMoves (bvh_pool.hpp), on the camera sightings only.
        bool moves = false;
        if ((draw.pass_bits & GC_PASS_CAMERA) != 0u)
        {
            bool identity = true;
            bool filled = false;
            bool finite = true;
            float scale = 1.0;
            [unroll] for (uint n = 0u; n < 12u; ++n)
            {
                const float expected = (n == 0u || n == 5u || n == 10u) ? 1.0 : 0.0;
                if (prev[n] != expected) identity = false;
                if (prev[n] != 0.0) filled = true;
                if (!isfinite(prev[n]) || !isfinite(world[n])) finite = false;
                if (n != 3u && n != 7u && n != 11u) scale = max(scale, abs(world[n]));
            }
            if (!identity && filled && finite)
            {
                [unroll] for (uint n = 0u; n < 12u; ++n)
                {
                    const float difference = abs(world[n] - prev[n]);
                    if (n == 3u || n == 7u || n == 11u)
                    {
                        const float magnitude = max(abs(world[n]), abs(prev[n]));
                        if (difference > max(GC_MOVING_METERS, magnitude * GC_MOVING_ULP)) moves = true;
                    }
                    else if (difference > GC_MOVING_BASIS * scale)
                    {
                        moves = true;
                    }
                }
            }
        }

        if (mode == GC_MODE_MARK)
        {
            if (moves)
            {
                InterlockedMax(g_gc_moving[draw.slot], frame + 1u);
                InterlockedAdd(g_gc_counters[GC_COUNTER_MARKED], 1u);
            }
            continue;
        }

        InterlockedAdd(g_gc_counters[GC_COUNTER_SIGHTINGS], 1u);
        if (draw.epoch != g_gc_epochs[draw.slot])
        {
            InterlockedAdd(g_gc_counters[GC_COUNTER_STALE_EPOCH], 1u);
            continue;
        }

        const float4 w0 = float4(world[0], world[1], world[2], world[3]);
        const float4 w1 = float4(world[4], world[5], world[6], world[7]);
        const float4 w2 = float4(world[8], world[9], world[10], world[11]);
        float4 inverse_rows[3];
        const float det = WorldAffineInverse(w0, w1, w2, inverse_rows);
        if (!(abs(det) >= 1e-12) || !WorldRowsPlausible(w0, w1, w2, min_scale, max_scale, max_position))
        {
            InterlockedAdd(g_gc_counters[GC_COUNTER_INVALID], 1u);
            continue;
        }

        const uint moved = g_gc_moving[draw.slot];
        const bool held = moved != 0u && frame + 1u - moved <= hold;
        if (moves || held)
        {
            if (!moves) InterlockedAdd(g_gc_counters[GC_COUNTER_MOVING_HELD], 1u);
            uint record_index = 0u;
            InterlockedAdd(g_gc_counters[GC_COUNTER_MOVING_RECORDS], 1u, record_index);
            if (record_index < moving_capacity)
            {
                g_gc_records[capacity + record_index] = GcMakeRecord(draw.slot, w0, w1, w2, inverse_rows, flags);
            }
            continue;
        }

        // D5 pose key: the slot leads, then the 12 world floats on the grid (round half to even, as C++ nearbyint).
        uint words[13];
        words[0] = draw.slot;
        [unroll] for (uint n = 0u; n < 12u; ++n)
        {
            const float scale_factor = (n == 3u || n == 7u || n == 11u) ? GC_POSE_TRANSLATION_SCALE : GC_POSE_BASIS_SCALE;
            words[n + 1u] = asuint((int)round(world[n] * scale_factor));
        }
        uint pose_lo = 2166136261u;
        uint pose_hi = 0x9747B28Cu;
        [unroll] for (uint w = 0u; w < 13u; ++w)
        {
            pose_lo = (pose_lo ^ words[w]) * 16777619u;
            pose_hi = (pose_hi ^ words[w]) * 0x5BD1E995u;
            pose_hi ^= pose_hi >> 15u;
        }

        pose_lo ^= pose_lo >> 16u;
        pose_lo *= 0x85EBCA6Bu;
        pose_lo ^= pose_lo >> 13u;
        pose_lo *= 0xC2B2AE35u;
        pose_lo ^= pose_lo >> 16u;
        pose_hi ^= pose_hi >> 16u;
        pose_hi *= 0x85EBCA6Bu;
        pose_hi ^= pose_hi >> 13u;
        pose_hi *= 0xC2B2AE35u;
        pose_hi ^= pose_hi >> 16u;

        GcSetEntry entry;
        entry.slot = draw.slot;
        entry.epoch = draw.epoch;
        entry.pose_lo = pose_lo;
        entry.pose_hi = pose_hi;
        entry.world[0] = w0;
        entry.world[1] = w1;
        entry.world[2] = w2;
        const uint result = GcSetInsert(dst, capacity, max_probes, pose_lo, pose_hi, frame, flags, entry);
        if (result == 0u)
        {
            InterlockedAdd(g_gc_counters[GC_COUNTER_INSERTED], 1u);
        }
        else if (result == 1u)
        {
            InterlockedAdd(g_gc_counters[GC_COUNTER_HITS], 1u);
        }
        else
        {
            InterlockedAdd(g_gc_counters[GC_COUNTER_SET_OVERFLOW], 1u);
        }
    }
}
