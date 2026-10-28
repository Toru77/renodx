#pragma once

// G3 instance set: the instance slices of the GPU-cache meshes, shared by world_gc_insert and world_gc_compact.
// The C++ mirrors (GcInstanceDrawGPU, GcSetEntryGPU, kGcCounter*, GcPoseHash) are in bvh/gpu_capture.hpp.

#include "world_bvh_types.hlsli"
#include "world_instance_math.hlsli"

#define GC_MODE_CLEAR 0u   // kGcMode* (gpu_capture.hpp)
#define GC_MODE_REHASH 1u
#define GC_MODE_MARK 2u
#define GC_MODE_INSERT 3u
#define GC_COUNTER_SET 0u  // kGcCounter* (gpu_capture.hpp)
#define GC_COUNTER_INSERTED 1u
#define GC_COUNTER_HITS 2u
#define GC_COUNTER_KEPT 3u
#define GC_COUNTER_AGED 4u
#define GC_COUNTER_DROPPED_EPOCH 5u
#define GC_COUNTER_DROPPED_MOVING 6u
#define GC_COUNTER_MOVING_RECORDS 7u
#define GC_COUNTER_MOVING_HELD 8u
#define GC_COUNTER_INVALID 9u
#define GC_COUNTER_STALE_EPOCH 10u
#define GC_COUNTER_SET_OVERFLOW 11u
#define GC_COUNTER_REHASH_OVERFLOW 12u
#define GC_COUNTER_MARKED 13u
#define GC_COUNTER_SIGHTINGS 14u
#define GC_PASS_CAMERA 1u   // kPoolPass* (bvh_pool.hpp)
#define GC_PASS_LIGHT 8u
#define GC_PASS_ALPHA 16u
#define GC_PASS_WIND 32u
#define GC_MOVING_METERS 1e-5        // kPoolMovingMeters
#define GC_MOVING_BASIS 1e-5         // kPoolMovingBasis
#define GC_MOVING_ULP 4.76837158e-7  // 2^-21 (PoolSightingMoves)
#define GC_POSE_TRANSLATION_SCALE 256.0  // kGcPoseTranslationScale
#define GC_POSE_BASIS_SCALE 4096.0       // kGcPoseBasisScale

struct GcInstanceDraw  // gpu_capture.hpp GcInstanceDrawGPU (32 bytes)
{
    uint arena_float4;
    uint count;
    uint slot;
    uint epoch;
    uint pass_bits;
    uint3 pad;
};
struct GcSetEntry  // gpu_capture.hpp GcSetEntryGPU (64 bytes)
{
    uint slot;
    uint epoch;
    uint pose_lo;
    uint pose_hi;
    float4 world[3];
};

ByteAddressBuffer g_gc_arena : register(t0);                  // instance slices (160 bytes per element)
StructuredBuffer<GcInstanceDraw> g_gc_draws : register(t1);
StructuredBuffer<uint> g_gc_epochs : register(t2);            // per slot
RWStructuredBuffer<uint> g_gc_meta : register(u0);            // per set index: tag, last_seen, flags, 0
RWStructuredBuffer<GcSetEntry> g_gc_entries : register(u1);
RWStructuredBuffer<WorldInstanceGPU> g_gc_records : register(u2);
RWStructuredBuffer<uint> g_gc_counters : register(u3);
RWStructuredBuffer<uint> g_gc_moving : register(u4);          // per slot: last present + 1 with a moving camera sighting

cbuffer cb_gc_instances : register(b0)
{
    uint4 g_gi_c0;  // mode, frame, src_base, dst_base
    uint4 g_gi_c1;  // draw_count, capacity, asuint(min_scale), asuint(max_scale)
    uint4 g_gi_c2;  // asuint(max_position), hold_frames, prune_age, max_probes
    uint4 g_gi_c3;  // moving_capacity, 0, 0, 0
};

// D4: 0 inserted, 1 found, 2 no free probe. The winner of the CAS writes the entry; every finder raises last_seen and ORs flags.
uint GcSetInsert(uint base, uint capacity, uint max_probes, uint pose_lo, uint pose_hi, uint last_seen, uint flags, GcSetEntry data)
{
    const uint tag = pose_hi | 1u;
    [loop] for (uint p = 0u; p < max_probes; ++p)
    {
        const uint index = base + ((pose_lo + p) & (capacity - 1u));
        uint prior = 0u;
        InterlockedCompareExchange(g_gc_meta[index * 4u], 0u, tag, prior);
        if (prior != 0u && prior != tag) continue;
        if (prior == 0u) g_gc_entries[index] = data;
        InterlockedMax(g_gc_meta[index * 4u + 1u], last_seen);
        InterlockedOr(g_gc_meta[index * 4u + 2u], flags);
        return prior == 0u ? 0u : 1u;
    }
    return 2u;
}

// D8: header (slot, 0, 0, 1); bounds are the translation point (no mesh box on this path; DXR uses the BLAS).
WorldInstanceGPU GcMakeRecord(uint slot, float4 r0, float4 r1, float4 r2, float4 inverse_rows[3], uint flags)
{
    WorldInstanceGPU record;
    record.header = float4((float)slot, 0.0, 0.0, 1.0);
    record.world[0] = r0;
    record.world[1] = r1;
    record.world[2] = r2;
    record.world[3] = float4(0.0, 0.0, 0.0, 1.0);
    record.inverse_world[0] = inverse_rows[0];
    record.inverse_world[1] = inverse_rows[1];
    record.inverse_world[2] = inverse_rows[2];
    record.inverse_world[3] = float4(0.0, 0.0, 0.0, 1.0);
    const float4 translation = float4(r0.w, r1.w, r2.w, 0.0);
    record.bounds_min = translation;
    record.bounds_max = translation;
    record.visibility = float4(0.0, 0.0, (float)flags, 0.0);
    return record;
}
