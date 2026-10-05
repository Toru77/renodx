#pragma once

// world_bvh_types.hlsli - canonical GPU layout shared by the BVH build and
// trace shaders. The C++ counterparts in bvh_resources.hpp/bvh_build.hpp are
// size-asserted against these strides:
//   WorldMeshGPU     64 bytes
//   WorldInstanceGPU 176 bytes
//   BVHNodeGPU       32 bytes
//   BVHLeafGPU       48 bytes

struct WorldMeshGPU
{
    float4 header;         // x = vertex_offset, y = vertex_count, z = index_offset, w = index_count
    float4 bbox_min;
    float4 bbox_max;
    float4 build;          // x = blas_node_offset, y = blas_root, z = leaf_offset, w = leaf_count
};

struct WorldInstanceGPU
{
    float4 header;         // x = mesh_id, y = source
    float4 world[4];
    float4 inverse_world[4];
    float4 bounds_min;
    float4 bounds_max;
};

struct BVHLeafGPU
{
    float4 bounds_min;
    float4 bounds_max;
    uint prim;
    uint code;
    uint pad0;
    uint pad1;
};

struct BVHNodeGPU
{
    float3 bounds_min;
    uint child_or_leaf;    // internal: left child node index; leaf: 0x80000000 | leaf index
    float3 bounds_max;
    uint sibling_or_right; // internal: right child node index
};

#define BVH_LEAF_FLAG 0x80000000u
