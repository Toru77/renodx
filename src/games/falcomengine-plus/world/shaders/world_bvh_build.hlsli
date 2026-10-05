// world_bvh_build.hlsli - shared top-down Morton LBVH construction.
//
// Leaves are already Morton-sorted on the CPU. The tree is built top-down:
// each range is split at the highest differing Morton bit; the larger half is
// deferred on a bounded stack while the smaller half is processed next, so the
// deferred range is at most half the parent and the stack depth is bounded by
// log2(leaf_count). Parents are always allocated before their children, which
// makes the bottom-up bounds pass a simple reverse index sweep.

#include "world_bvh_types.hlsli"

#define BVH_STACK_SIZE 64u

uint BvhSplitByMorton(StructuredBuffer<BVHLeafGPU> leaves, uint leaf_base, uint first, uint last)
{
    const uint first_code = leaves[leaf_base + first].code;
    const uint last_code = leaves[leaf_base + last].code;
    if (first_code == last_code)
    {
        return (first + last) / 2u;
    }
    const uint bit = firstbithigh(first_code ^ last_code);
    uint lo = first;
    uint hi = last;
    while (lo + 1u < hi)
    {
        const uint mid = (lo + hi) / 2u;
        if (((leaves[leaf_base + mid].code >> bit) & 1u) != 0u) hi = mid; else lo = mid;
    }
    return lo;
}

void BuildLbvh(
    StructuredBuffer<BVHLeafGPU> leaves,
    uint leaf_base,
    uint leaf_count,
    RWStructuredBuffer<BVHNodeGPU> nodes,
    uint node_base)
{
    if (leaf_count == 0u) return;

    uint stack_first[BVH_STACK_SIZE];
    uint stack_last[BVH_STACK_SIZE];
    uint stack_node[BVH_STACK_SIZE];
    uint top = 0u;
    uint node_count = 1u;
    uint first = 0u;
    uint last = leaf_count - 1u;
    uint node = 0u;

    while (true)
    {
        if (first == last)
        {
            const BVHLeafGPU leaf = leaves[leaf_base + first];
            BVHNodeGPU out_node;
            out_node.bounds_min = leaf.bounds_min.xyz;
            out_node.bounds_max = leaf.bounds_max.xyz;
            // Leaf nodes encode the leaf index within this tree's leaf range;
            // traversal resolves the primitive/instance through the leaf buffer.
            out_node.child_or_leaf = BVH_LEAF_FLAG | first;
            out_node.sibling_or_right = 0u;
            nodes[node_base + node] = out_node;
            if (top == 0u) break;
            top--;
            first = stack_first[top];
            last = stack_last[top];
            node = stack_node[top];
            continue;
        }

        const uint split = BvhSplitByMorton(leaves, leaf_base, first, last);
        const uint left = node_count++;
        const uint right = node_count++;
        BVHNodeGPU out_node;
        out_node.bounds_min = float3(1e30, 1e30, 1e30);
        out_node.bounds_max = float3(-1e30, -1e30, -1e30);
        out_node.child_or_leaf = left;
        out_node.sibling_or_right = right;
        nodes[node_base + node] = out_node;

        const uint left_size = split - first + 1u;
        const uint right_size = last - split;
        if (left_size >= right_size)
        {
            stack_first[top] = first;
            stack_last[top] = split;
            stack_node[top] = left;
            top++;
            first = split + 1u;
            node = right;
        }
        else
        {
            stack_first[top] = split + 1u;
            stack_last[top] = last;
            stack_node[top] = right;
            top++;
            last = split;
            node = left;
        }
    }

    // Parents are allocated before their children, so a reverse index sweep
    // computes every internal node's bounds from its already-finished children.
    for (int i = (int)node_count - 1; i >= 0; --i)
    {
        BVHNodeGPU out_node = nodes[node_base + (uint)i];
        if ((out_node.child_or_leaf & BVH_LEAF_FLAG) != 0u) continue;
        const BVHNodeGPU left_node = nodes[node_base + out_node.child_or_leaf];
        const BVHNodeGPU right_node = nodes[node_base + out_node.sibling_or_right];
        out_node.bounds_min = min(left_node.bounds_min, right_node.bounds_min);
        out_node.bounds_max = max(left_node.bounds_max, right_node.bounds_max);
        nodes[node_base + (uint)i] = out_node;
    }
}
