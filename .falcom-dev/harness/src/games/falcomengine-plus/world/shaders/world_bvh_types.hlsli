#pragma once

// world_bvh_types.hlsli - canonical GPU layout read by the trace shaders. The
// BVH is built on the CPU (bvh_build.hpp, bvh_live.hpp); the C++ counterparts
// in bvh_resources.hpp/bvh_build.hpp/deform_live.hpp are size-asserted against these strides:
//   DynamicObjectGPU 32 bytes
//   WorldMeshGPU     64 bytes
//   WorldInstanceGPU 192 bytes
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
    float4 visibility;     // x = near-fade start (m), y = near-fade 1/range, z = INSTANCE_* flags, w = 0
};

// WorldInstanceGPU.visibility.z flags (bvh_resources.hpp kInstance*).
#define INSTANCE_NEAR_FADE 1u       // camera rays apply the near fade
#define INSTANCE_CAMERA_VISIBLE 2u  // drawn by a camera vertex shader
#define INSTANCE_SHADOW_CASTER 4u   // drawn by a light vertex shader (shadow maps)

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

// A deforming mesh captured by stream output this frame (deform_live.hpp
// DynamicObjectGPU, 32 bytes): its BLAS in the dynamic arenas, refit every
// frame; its triangles are three world-space float3 each, from vertex_base.
struct DynamicObjectGPU
{
    uint node_offset;
    uint leaf_offset;
    uint node_count;
    uint vertex_base;
    uint triangle_count;
    uint refit_offset;
    uint level_offset;   // level_count + 1 level starts, relative to refit_offset
    uint level_count;
};

#define DYNAMIC_INSTANCE_FLAG 0x80000000u  // hit.instance of a dynamic object
#define DYNAMIC_MESH_MARKER 0xFFFFFFFEu    // hit.mesh of a dynamic object
