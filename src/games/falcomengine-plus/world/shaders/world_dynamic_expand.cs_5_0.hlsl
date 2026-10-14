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

StructuredBuffer<float4> g_dyn_arena : register(t0);
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

// CheckPoolWorld (bvh_pool.hpp): finite; each row length >= 1e-6 (not zero scale) and within [min, max); each
// translation within max_position; no two rows nearly parallel (|cos| < 0.995).
bool DynamicWorldPlausible(float4 r0, float4 r1, float4 r2)
{
    if (!all(isfinite(r0)) || !all(isfinite(r1)) || !all(isfinite(r2))) return false;
    const float3 rows[3] = {r0.xyz, r1.xyz, r2.xyz};
    const float3 translation = float3(r0.w, r1.w, r2.w);
    if (!all(abs(translation) <= g_max_position)) return false;
    float lengths[3];
    for (uint r = 0; r < 3; ++r)
    {
        lengths[r] = length(rows[r]);
        if (!(lengths[r] >= 1e-6) || !(lengths[r] >= g_min_scale) || !(lengths[r] < g_max_scale)) return false;
    }
    for (uint a = 0; a < 3; ++a)
    {
        for (uint b = a + 1; b < 3; ++b)
        {
            if (!(abs(dot(rows[a], rows[b]) / (lengths[a] * lengths[b])) < 0.995)) return false;
        }
    }
    return true;
}

[numthreads(DYNAMIC_THREADS, 1, 1)]
void main(uint3 group : SV_GroupID, uint thread : SV_GroupIndex)
{
    if (group.x >= g_draw_count) return;
    const DynamicDrawGPU draw = g_dyn_draws[group.x];
    const bool draw_valid = (draw.info.w & 1u) != 0u;
    uint count = draw.info.y;
    if (draw.extra.x != 0u) count = min(count, asuint(g_dyn_arena[draw.extra.y].y));  // indirect: InstanceCount
    for (uint i = thread; i < draw.info.y; i += DYNAMIC_THREADS)
    {
        float4 rows[3] = {float4(0.0, 0.0, 0.0, 0.0), float4(0.0, 0.0, 0.0, 0.0), float4(0.0, 0.0, 0.0, 0.0)};
        const bool live = draw_valid && i < count;
        if (live)
        {
            const uint element = draw.info.x + i * DYNAMIC_ELEMENT_FLOAT4;
            rows[0] = g_dyn_arena[element];
            rows[1] = g_dyn_arena[element + 1u];
            rows[2] = g_dyn_arena[element + 2u];
        }
        const float3 a0 = rows[0].xyz;
        const float3 a1 = rows[1].xyz;
        const float3 a2 = rows[2].xyz;
        // Columns of the adjugate: rows of the inverse are (c0.k, c1.k, c2.k) / det.
        const float3 c0 = cross(a1, a2);
        const float3 c1 = cross(a2, a0);
        const float3 c2 = cross(a0, a1);
        const float det = dot(a0, c0);
        const bool valid = live && abs(det) >= 1e-12 && DynamicWorldPlausible(rows[0], rows[1], rows[2]);
        const float3 translation = float3(rows[0].w, rows[1].w, rows[2].w);

        // header.z = 0: alpha-tested meshes are never recorded (RecordPoolDynamicDraw), so every record is not alpha-tested.
        WorldInstanceGPU record;
        record.header = float4((float)draw.extra.z, 0.0, 0.0, valid ? 1.0 : 0.0);
        record.world[0] = rows[0];
        record.world[1] = rows[1];
        record.world[2] = rows[2];
        record.world[3] = float4(0.0, 0.0, 0.0, 1.0);
        float4 inverse_rows[3];
        for (uint r = 0; r < 3; ++r)
        {
            const float3 inverse_row = float3(c0[r], c1[r], c2[r]) / det;
            inverse_rows[r] = float4(inverse_row, -dot(inverse_row, translation));
        }
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
