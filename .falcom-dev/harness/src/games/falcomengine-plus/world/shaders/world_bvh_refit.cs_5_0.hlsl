// world_bvh_refit.cs_5_0.hlsl - refits the BLAS of deforming meshes.
//
// One thread group per object captured this frame (deform_live.hpp). Its
// nodes are listed by depth, deepest level first (BuildRefitOrder); level by
// level, a leaf node takes the bounds of its triangle (three world-space float3
// from this frame's stream-output capture) and an internal node the union of
// its children, which the previous level finished. The tree structure never
// changes. Mirrors deform_live.hpp RefitDynamicNodes.
//
// The level loop has a fixed trip count so the group barrier is in uniform
// flow control; objects with fewer levels idle through the rest.

#include "world_bvh_types.hlsli"

#define REFIT_GROUP_SIZE 64u
#define REFIT_MAX_LEVELS 64u

Buffer<float> g_vertices : register(t0);
StructuredBuffer<DynamicObjectGPU> g_objects : register(t1);
StructuredBuffer<BVHLeafGPU> g_leaves : register(t2);
StructuredBuffer<uint> g_refit : register(t3);
RWStructuredBuffer<BVHNodeGPU> g_nodes : register(u0);

cbuffer cb_refit : register(b13)
{
    uint4 g_refit_params;  // x: objects
};

[numthreads(64, 1, 1)]
void main(uint3 group : SV_GroupID, uint thread_index : SV_GroupIndex)
{
    const uint object_index = group.x;
    const bool active = object_index < g_refit_params.x;
    DynamicObjectGPU object = (DynamicObjectGPU)0;
    if (active) object = g_objects[object_index];
    const uint leaf_count = (object.node_count + 1u) / 2u;

    [loop]
    for (uint level = 0u; level < REFIT_MAX_LEVELS; ++level)
    {
        if (active && level < object.level_count)
        {
            const uint begin = g_refit[object.refit_offset + object.level_offset + level];
            const uint end = g_refit[object.refit_offset + object.level_offset + level + 1u];
            for (uint entry = begin + thread_index; entry < end; entry += REFIT_GROUP_SIZE)
            {
                const uint local = g_refit[object.refit_offset + entry];
                if (local >= object.node_count) continue;
                BVHNodeGPU node = g_nodes[object.node_offset + local];
                float3 lo;
                float3 hi;
                if ((node.child_or_leaf & BVH_LEAF_FLAG) != 0u)
                {
                    const uint leaf_index = node.child_or_leaf & 0x7FFFFFFFu;
                    if (leaf_index >= leaf_count) continue;
                    const uint prim = g_leaves[object.leaf_offset + leaf_index].prim;
                    if (prim >= object.triangle_count) continue;
                    const uint base = (object.vertex_base + prim * 3u) * 3u;
                    const float3 a = float3(g_vertices[base + 0u], g_vertices[base + 1u], g_vertices[base + 2u]);
                    const float3 b = float3(g_vertices[base + 3u], g_vertices[base + 4u], g_vertices[base + 5u]);
                    const float3 c = float3(g_vertices[base + 6u], g_vertices[base + 7u], g_vertices[base + 8u]);
                    lo = min(a, min(b, c));
                    hi = max(a, max(b, c));
                }
                else
                {
                    if (node.child_or_leaf >= object.node_count || node.sibling_or_right >= object.node_count) continue;
                    const BVHNodeGPU left = g_nodes[object.node_offset + node.child_or_leaf];
                    const BVHNodeGPU right = g_nodes[object.node_offset + node.sibling_or_right];
                    lo = min(left.bounds_min, right.bounds_min);
                    hi = max(left.bounds_max, right.bounds_max);
                }
                node.bounds_min = lo;
                node.bounds_max = hi;
                g_nodes[object.node_offset + local] = node;
            }
        }
        DeviceMemoryBarrierWithGroupSync();
    }
}
