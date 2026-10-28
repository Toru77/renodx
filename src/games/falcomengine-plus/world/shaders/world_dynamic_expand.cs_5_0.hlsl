// world_dynamic_expand.cs_5_0.hlsl - P2b: expands the flagged draws of this present into WorldInstanceGPU
// records for the trace (bvh_live.hpp ExpandLiveDynamic). One thread group per draw; its records are
// [info.z, info.z + count), count = info.y (direct) or the args' instance count, capped at info.y (indirect).
//
// The matrix is the instance slice copied at the draw (row-major 3x4 in the first 48 bytes of each 160-byte
// element). A record is valid only when the draw's mesh is resident and the matrix passes CheckPoolWorld
// (bvh_pool.hpp, same tests in the same meaning) with a non-singular linear part; anything else is written
// with header.w = 0 and the trace skips it. The inverse is the closed-form affine inverse (rows of the inverse
// are the cofactor columns over the determinant); the bounds are the mesh's local box, transformed.

#include "world_bvh_types.hlsli"
#include "world_instance_math.hlsli"

ByteAddressBuffer g_dyn_arena : register(t0);
StructuredBuffer<DynamicDrawGPU> g_dyn_draws : register(t1);
RWStructuredBuffer<WorldInstanceGPU> g_dyn_instances : register(u0);

cbuffer cb_dynamic_expand : register(b0)
{
    uint g_draw_count;
    float g_min_scale;     // kPoolMinScale (kPoolLegacyMinScale while the legacy switch is on)
    float g_max_scale;     // kPoolMaxScale
    float g_max_position;  // kPoolMaxPosition
};

#define DYNAMIC_THREADS 64u
#define DYNAMIC_ELEMENT_FLOAT4 10u  // one instance slice element (kPoolInstanceStride = 160 bytes), in float4


[numthreads(DYNAMIC_THREADS, 1, 1)]
void main(uint3 group : SV_GroupID, uint thread : SV_GroupIndex)
{
    if (group.x >= g_draw_count) return;
    const DynamicDrawGPU draw = g_dyn_draws[group.x];
    const bool draw_valid = (draw.info.w & 1u) != 0u;
    uint count = draw.info.y;
    if (draw.extra.x != 0u) count = min(count, g_dyn_arena.Load(draw.extra.y * 16u + 4u));  // indirect: InstanceCount
    for (uint i = thread; i < draw.info.y; i += DYNAMIC_THREADS)
    {
        float4 rows[3] = {float4(0.0, 0.0, 0.0, 0.0), float4(0.0, 0.0, 0.0, 0.0), float4(0.0, 0.0, 0.0, 0.0)};
        const bool live = draw_valid && i < count;
        if (live)
        {
            const uint element = draw.info.x + i * DYNAMIC_ELEMENT_FLOAT4;
            rows[0] = asfloat(g_dyn_arena.Load4(element * 16u));
            rows[1] = asfloat(g_dyn_arena.Load4((element + 1u) * 16u));
            rows[2] = asfloat(g_dyn_arena.Load4((element + 2u) * 16u));
        }
        float4 inverse_rows[3];
        const float det = WorldAffineInverse(rows[0], rows[1], rows[2], inverse_rows);
        const bool valid = live && abs(det) >= 1e-12 && WorldRowsPlausible(rows[0], rows[1], rows[2], g_min_scale, g_max_scale, g_max_position);
        const float3 translation = float3(rows[0].w, rows[1].w, rows[2].w);

        // header.z = 0: alpha-tested meshes are never recorded (RecordPoolDynamicDraw), so every record is not alpha-tested.
        WorldInstanceGPU record;
        record.header = float4((float)draw.extra.z, 0.0, 0.0, valid ? 1.0 : 0.0);
        record.world[0] = rows[0];
        record.world[1] = rows[1];
        record.world[2] = rows[2];
        record.world[3] = float4(0.0, 0.0, 0.0, 1.0);
        record.inverse_world[0] = inverse_rows[0];
        record.inverse_world[1] = inverse_rows[1];
        record.inverse_world[2] = inverse_rows[2];
        record.inverse_world[3] = float4(0.0, 0.0, 0.0, 1.0);
        // Per axis: the translation plus the smaller (larger) of each box extent times its row element.
        const float3 box_min = draw.bbox_min.xyz;
        const float3 box_max = draw.bbox_max.xyz;
        const float3 e0 = rows[0].xyz * box_min;
        const float3 e1 = rows[0].xyz * box_max;
        const float3 f0 = rows[1].xyz * box_min;
        const float3 f1 = rows[1].xyz * box_max;
        const float3 g0 = rows[2].xyz * box_min;
        const float3 g1 = rows[2].xyz * box_max;
        const float3 lo = float3(
            translation.x + min(e0.x, e1.x) + min(e0.y, e1.y) + min(e0.z, e1.z),
            translation.y + min(f0.x, f1.x) + min(f0.y, f1.y) + min(f0.z, f1.z),
            translation.z + min(g0.x, g1.x) + min(g0.y, g1.y) + min(g0.z, g1.z));
        const float3 hi = float3(
            translation.x + max(e0.x, e1.x) + max(e0.y, e1.y) + max(e0.z, e1.z),
            translation.y + max(f0.x, f1.x) + max(f0.y, f1.y) + max(f0.z, f1.z),
            translation.z + max(g0.x, g1.x) + max(g0.y, g1.y) + max(g0.z, g1.z));
        record.bounds_min = float4(lo, 0.0);
        record.bounds_max = float4(hi, 0.0);
        record.visibility = float4(0.0, 0.0, (float)(draw.info.w & 6u), 0.0);
        g_dyn_instances[draw.info.z + i] = record;
    }
}
