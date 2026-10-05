// world_bvh_build_blas.cs_5_0.hlsl - one thread per mesh builds its LBAS.

#include "world_bvh_build.hlsli"

StructuredBuffer<WorldMeshGPU> g_meshes : register(t0);
StructuredBuffer<BVHLeafGPU> g_leaves : register(t1);
RWStructuredBuffer<BVHNodeGPU> g_nodes : register(u0);

cbuffer cb_build : register(b13)
{
    uint g_mesh_count;
    uint g_spare0;
    uint g_spare1;
    uint g_spare2;
};

[numthreads(64, 1, 1)]
void main(uint3 dtid : SV_DispatchThreadID)
{
    const uint mesh_id = dtid.x;
    if (mesh_id >= g_mesh_count) return;
    const WorldMeshGPU mesh = g_meshes[mesh_id];
    BuildLbvh(
        g_leaves,
        (uint)mesh.build.z,
        (uint)mesh.build.w,
        g_nodes,
        (uint)mesh.build.x);
}
