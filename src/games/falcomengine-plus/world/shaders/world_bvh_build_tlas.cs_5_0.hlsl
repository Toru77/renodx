// world_bvh_build_tlas.cs_5_0.hlsl - single-group LBVH build over the active
// region instances.

#include "world_bvh_build.hlsli"

StructuredBuffer<BVHLeafGPU> g_leaves : register(t0);
RWStructuredBuffer<BVHNodeGPU> g_nodes : register(u0);

cbuffer cb_build : register(b13)
{
    uint g_leaf_count;
    uint g_spare0;
    uint g_spare1;
    uint g_spare2;
};

[numthreads(64, 1, 1)]
void main(uint3 dtid : SV_DispatchThreadID)
{
    if (dtid.x != 0u) return;
    BuildLbvh(g_leaves, 0u, g_leaf_count, g_nodes, 0u);
}
