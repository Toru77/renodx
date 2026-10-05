// world_bvh_raster_vs.vs_5_0.hlsl - M2b pool triangle reconstruction.
//
// One draw per mesh group: vertex_id indexes the mesh vertex inside the pool
// vertex buffer, instance_id indexes the group's instance list. No input
// assembler buffers are used; all data comes from the pool SRVs.

struct WorldInstanceGPU
{
    float4 header;         // x = mesh_id, y = source
    float4 world[4];
    float4 inverse_world[4];
    float4 bounds_min;
    float4 bounds_max;
};

struct WorldMeshGPU
{
    float4 header;         // x = vertex_offset, y = vertex_count, z = index_offset, w = index_count
    float4 bbox_min;
    float4 bbox_max;
    float4 build;          // x = blas_node_offset, y = blas_root, z = leaf_offset, w = leaf_count
};

StructuredBuffer<float4> g_vertices : register(t0);
StructuredBuffer<WorldMeshGPU> g_meshes : register(t1);
StructuredBuffer<WorldInstanceGPU> g_instances : register(t2);
StructuredBuffer<uint> g_grouped_active : register(t3);
StructuredBuffer<uint> g_indices : register(t4);

cbuffer cb_raster : register(b13)
{
    float4x4 g_view_proj;
    uint g_mode;
    uint g_mesh_id;
    uint g_group_offset;
    uint g_spare0;
};

struct VSOutput
{
    float4 position : SV_Position;
    float3 world : TEXCOORD0;
    nointerpolation uint instance_id : TEXCOORD1;
    nointerpolation uint mesh_id : TEXCOORD2;
};

VSOutput main(uint vertex_id : SV_VertexID, uint instance_id : SV_InstanceID)
{
    VSOutput output;
    const WorldMeshGPU mesh = g_meshes[g_mesh_id];
    const uint resolved_instance = g_grouped_active[g_group_offset + instance_id];
    const WorldInstanceGPU instance = g_instances[resolved_instance];
    // Reproduce indexed geometry through SV_VertexID: vertex_id walks the
    // mesh's index range, not its unique vertex list.
    const uint index = g_indices[(uint)mesh.header.z + vertex_id];
    const float3 position = g_vertices[(uint)mesh.header.x + index].xyz;
    const float4 p = float4(position, 1.0);
    const float3 world = float3(
        dot(p, instance.world[0]),
        dot(p, instance.world[1]),
        dot(p, instance.world[2]));
    output.position = mul(float4(world, 1.0), g_view_proj);
    output.world = world;
    output.instance_id = resolved_instance;
    output.mesh_id = g_mesh_id;
    return output;
}
