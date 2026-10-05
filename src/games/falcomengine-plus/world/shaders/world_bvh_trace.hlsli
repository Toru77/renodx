#pragma once

// world_bvh_trace.hlsli - M4 read-only BVH traversal.
//
// TraceWorldClosest / TraceWorldAny trace a world-space ray through the region
// TLAS, transform it into each candidate mesh's local space with the stored
// inverse world matrix (row-dot convention; the affine transform preserves the
// ray parameterization, so hit t stays in world units), then walk the mesh
// BLAS and test triangles with Moller-Trumbore.
//
// All references are bounds-checked and clamped: malformed data increments
// invalid_refs instead of dereferencing out of range, and the stack never
// overflows (stack_overflow is counted instead). Per-thread counters are
// accumulated by the caller.

#include "world_bvh_types.hlsli"

#define TRACE_STACK_SIZE 64u

StructuredBuffer<float4> g_trace_vertices : register(t0);
StructuredBuffer<uint> g_trace_indices : register(t1);
StructuredBuffer<WorldMeshGPU> g_trace_meshes : register(t2);
StructuredBuffer<WorldInstanceGPU> g_trace_instances : register(t3);
StructuredBuffer<uint> g_trace_active : register(t4);
StructuredBuffer<BVHNodeGPU> g_trace_blas_nodes : register(t5);
StructuredBuffer<BVHLeafGPU> g_trace_blas_leaves : register(t6);
StructuredBuffer<BVHNodeGPU> g_trace_tlas_nodes : register(t7);
StructuredBuffer<BVHLeafGPU> g_trace_tlas_leaves : register(t8);

struct WorldTraceHit
{
    float t;
    uint instance;
    uint mesh;
    uint prim;
    float3 position;
    float3 normal;
};

struct WorldTraceCounters
{
    uint hits;
    uint misses;
    uint invalid_refs;
    uint stack_overflow;
    uint triangle_tests;
    uint max_stack_depth;
};

void TraceResetCounters(out WorldTraceCounters counters)
{
    counters.hits = 0u;
    counters.misses = 0u;
    counters.invalid_refs = 0u;
    counters.stack_overflow = 0u;
    counters.triangle_tests = 0u;
    counters.max_stack_depth = 0u;
}

bool AabbIntersect(
    float3 origin,
    float3 direction,
    float3 bounds_min,
    float3 bounds_max,
    float t_min,
    float t_max,
    out float t_near,
    out float t_far)
{
    const float3 inv_direction = 1.0 / direction;
    const float3 t0 = (bounds_min - origin) * inv_direction;
    const float3 t1 = (bounds_max - origin) * inv_direction;
    const float3 t_small = min(t0, t1);
    const float3 t_big = max(t0, t1);
    t_near = max(max(t_small.x, t_small.y), max(t_small.z, t_min));
    t_far = min(min(t_big.x, t_big.y), min(t_big.z, t_max));
    return t_near <= t_far;
}

bool IntersectTriangle(
    float3 origin,
    float3 direction,
    float3 v0,
    float3 v1,
    float3 v2,
    out float t,
    out float3 normal)
{
    t = 0.0;
    normal = float3(0.0, 1.0, 0.0);
    const float3 edge1 = v1 - v0;
    const float3 edge2 = v2 - v0;
    const float3 p = cross(direction, edge2);
    const float det = dot(edge1, p);
    if (abs(det) < 1e-12) return false;
    const float inv_det = 1.0 / det;
    const float3 tv = origin - v0;
    const float u = dot(tv, p) * inv_det;
    if (u < 0.0 || u > 1.0) return false;
    const float3 q = cross(tv, edge1);
    const float v = dot(direction, q) * inv_det;
    if (v < 0.0 || u + v > 1.0) return false;
    const float hit_t = dot(edge2, q) * inv_det;
    if (hit_t <= 0.0) return false;
    const float3 raw_normal = cross(edge1, edge2);
    const float normal_length = length(raw_normal);
    t = hit_t;
    if (normal_length > 1e-12) normal = raw_normal / normal_length;
    return true;
}

bool TraceBlas(
    uint mesh_id,
    float3 origin,
    float3 direction,
    float t_min,
    inout float t_max,
    inout WorldTraceCounters counters,
    out uint out_prim,
    out float3 out_normal)
{
    out_prim = 0xFFFFFFFFu;
    out_normal = float3(0.0, 0.0, 0.0);

    const WorldMeshGPU mesh = g_trace_meshes[mesh_id];
    const uint leaf_count = (uint)mesh.build.w;
    if (leaf_count == 0u) return false;
    const uint node_count = 2u * leaf_count - 1u;
    const uint node_base = (uint)mesh.build.x;
    const uint leaf_base = (uint)mesh.build.z;
    const uint vertex_offset = (uint)mesh.header.x;
    const uint vertex_count = (uint)mesh.header.y;
    const uint index_offset = (uint)mesh.header.z;
    const uint index_count = (uint)mesh.header.w;

    uint stack[TRACE_STACK_SIZE];
    uint stack_size = 0u;
    uint node_index = 0u;
    bool found = false;

    while (true)
    {
        if (node_index >= node_count)
        {
            counters.invalid_refs++;
        }
        else
        {
            const BVHNodeGPU node = g_trace_blas_nodes[node_base + node_index];
            float t_near;
            float t_far;
            if (AabbIntersect(origin, direction, node.bounds_min, node.bounds_max, t_min, t_max, t_near, t_far))
            {
                if ((node.child_or_leaf & BVH_LEAF_FLAG) != 0u)
                {
                    const uint leaf_index = node.child_or_leaf & 0x7FFFFFFFu;
                    if (leaf_index >= leaf_count)
                    {
                        counters.invalid_refs++;
                    }
                    else
                    {
                        const BVHLeafGPU leaf = g_trace_blas_leaves[leaf_base + leaf_index];
                        const uint prim = leaf.prim;
                        if (prim * 3u + 2u >= index_count)
                        {
                            counters.invalid_refs++;
                        }
                        else
                        {
                            const uint i0 = g_trace_indices[index_offset + prim * 3u + 0u];
                            const uint i1 = g_trace_indices[index_offset + prim * 3u + 1u];
                            const uint i2 = g_trace_indices[index_offset + prim * 3u + 2u];
                            if (i0 >= vertex_count || i1 >= vertex_count || i2 >= vertex_count)
                            {
                                counters.invalid_refs++;
                            }
                            else
                            {
                                const float3 v0 = g_trace_vertices[vertex_offset + i0].xyz;
                                const float3 v1 = g_trace_vertices[vertex_offset + i1].xyz;
                                const float3 v2 = g_trace_vertices[vertex_offset + i2].xyz;
                                float tri_t;
                                float3 tri_normal;
                                counters.triangle_tests++;
                                if (IntersectTriangle(origin, direction, v0, v1, v2, tri_t, tri_normal)
                                    && tri_t >= t_min && tri_t < t_max)
                                {
                                    t_max = tri_t;
                                    out_prim = prim;
                                    out_normal = tri_normal;
                                    found = true;
                                }
                            }
                        }
                    }
                }
                else
                {
                    const uint left = node.child_or_leaf;
                    const uint right = node.sibling_or_right;
                    if (left >= node_count || right >= node_count || left == right)
                    {
                        counters.invalid_refs++;
                    }
                    else if (stack_size < TRACE_STACK_SIZE)
                    {
                        stack[stack_size] = right;
                        stack_size++;
                        counters.max_stack_depth = max(counters.max_stack_depth, stack_size);
                        node_index = left;
                        continue;
                    }
                    else
                    {
                        counters.stack_overflow++;
                    }
                }
            }
        }
        if (stack_size == 0u) break;
        stack_size--;
        node_index = stack[stack_size];
    }
    return found;
}

WorldTraceHit TraceWorldRay(
    float3 origin,
    float3 direction,
    float t_min,
    float t_max,
    bool any_hit,
    out WorldTraceCounters counters)
{
    TraceResetCounters(counters);

    WorldTraceHit hit;
    hit.t = t_max;
    hit.instance = 0xFFFFFFFFu;
    hit.mesh = 0xFFFFFFFFu;
    hit.prim = 0xFFFFFFFFu;
    hit.position = float3(0.0, 0.0, 0.0);
    hit.normal = float3(0.0, 0.0, 0.0);

    uint tlas_leaf_count = 0u;
    uint tlas_stride = 0u;
    g_trace_tlas_leaves.GetDimensions(tlas_leaf_count, tlas_stride);
    if (tlas_leaf_count == 0u)
    {
        counters.misses = 1u;
        return hit;
    }
    const uint tlas_node_count = 2u * tlas_leaf_count - 1u;

    uint active_count = 0u;
    uint active_stride = 0u;
    g_trace_active.GetDimensions(active_count, active_stride);
    uint instance_count = 0u;
    uint instance_stride = 0u;
    g_trace_instances.GetDimensions(instance_count, instance_stride);
    uint mesh_count = 0u;
    uint mesh_stride = 0u;
    g_trace_meshes.GetDimensions(mesh_count, mesh_stride);

    float best_t = t_max;
    uint best_instance = 0xFFFFFFFFu;
    uint best_mesh = 0xFFFFFFFFu;
    uint best_prim = 0xFFFFFFFFu;
    float3 best_normal = float3(0.0, 0.0, 0.0);

    uint stack[TRACE_STACK_SIZE];
    uint stack_size = 0u;
    uint node_index = 0u;

    while (true)
    {
        if (node_index >= tlas_node_count)
        {
            counters.invalid_refs++;
        }
        else
        {
            const BVHNodeGPU node = g_trace_tlas_nodes[node_index];
            float t_near;
            float t_far;
            if (AabbIntersect(origin, direction, node.bounds_min, node.bounds_max, t_min, best_t, t_near, t_far))
            {
                if ((node.child_or_leaf & BVH_LEAF_FLAG) != 0u)
                {
                    const uint slot = node.child_or_leaf & 0x7FFFFFFFu;
                    if (slot >= tlas_leaf_count)
                    {
                        counters.invalid_refs++;
                    }
                    else
                    {
                        const BVHLeafGPU leaf = g_trace_tlas_leaves[slot];
                        const uint instance_slot = leaf.prim;
                        if (instance_slot >= active_count)
                        {
                            counters.invalid_refs++;
                        }
                        else
                        {
                            const uint resolved = g_trace_active[instance_slot];
                            if (resolved >= instance_count)
                            {
                                counters.invalid_refs++;
                            }
                            else
                            {
                                const WorldInstanceGPU instance = g_trace_instances[resolved];
                                const uint mesh_id = (uint)instance.header.x;
                                if (mesh_id >= mesh_count)
                                {
                                    counters.invalid_refs++;
                                }
                                else
                                {
                                    const float4 origin_h = float4(origin, 1.0);
                                    const float4 direction_h = float4(direction, 0.0);
                                    const float3 local_origin = float3(
                                        dot(origin_h, instance.inverse_world[0]),
                                        dot(origin_h, instance.inverse_world[1]),
                                        dot(origin_h, instance.inverse_world[2]));
                                    const float3 local_direction = float3(
                                        dot(direction_h, instance.inverse_world[0]),
                                        dot(direction_h, instance.inverse_world[1]),
                                        dot(direction_h, instance.inverse_world[2]));
                                    float local_best_t = best_t;
                                    uint local_prim = 0xFFFFFFFFu;
                                    float3 local_normal = float3(0.0, 0.0, 0.0);
                                    if (TraceBlas(mesh_id, local_origin, local_direction, t_min, local_best_t, counters, local_prim, local_normal))
                                    {
                                        best_t = local_best_t;
                                        best_instance = resolved;
                                        best_mesh = mesh_id;
                                        best_prim = local_prim;
                                        const float4 normal_h = float4(local_normal, 0.0);
                                        best_normal = normalize(float3(
                                            dot(normal_h, instance.world[0]),
                                            dot(normal_h, instance.world[1]),
                                            dot(normal_h, instance.world[2])));
                                        if (any_hit)
                                        {
                                            counters.hits = 1u;
                                            hit.t = best_t;
                                            hit.instance = best_instance;
                                            hit.mesh = best_mesh;
                                            hit.prim = best_prim;
                                            hit.position = origin + direction * best_t;
                                            hit.normal = best_normal;
                                            return hit;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                else
                {
                    const uint left = node.child_or_leaf;
                    const uint right = node.sibling_or_right;
                    if (left >= tlas_node_count || right >= tlas_node_count || left == right)
                    {
                        counters.invalid_refs++;
                    }
                    else if (stack_size < TRACE_STACK_SIZE)
                    {
                        stack[stack_size] = right;
                        stack_size++;
                        counters.max_stack_depth = max(counters.max_stack_depth, stack_size);
                        node_index = left;
                        continue;
                    }
                    else
                    {
                        counters.stack_overflow++;
                    }
                }
            }
        }
        if (stack_size == 0u) break;
        stack_size--;
        node_index = stack[stack_size];
    }

    if (best_prim == 0xFFFFFFFFu)
    {
        counters.misses = 1u;
        return hit;
    }
    counters.hits = 1u;
    hit.t = best_t;
    hit.instance = best_instance;
    hit.mesh = best_mesh;
    hit.prim = best_prim;
    hit.position = origin + direction * best_t;
    hit.normal = best_normal;
    return hit;
}

WorldTraceHit TraceWorldClosest(float3 origin, float3 direction, float t_min, float t_max, out WorldTraceCounters counters)
{
    return TraceWorldRay(origin, direction, t_min, t_max, false, counters);
}

WorldTraceHit TraceWorldAny(float3 origin, float3 direction, float t_min, float t_max, out WorldTraceCounters counters)
{
    return TraceWorldRay(origin, direction, t_min, t_max, true, counters);
}
