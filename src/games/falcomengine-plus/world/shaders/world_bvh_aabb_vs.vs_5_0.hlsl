// world_bvh_aabb_vs.vs_5_0.hlsl - M2b instanced instance-AABB wireframe.
//
// One draw covers every active instance: vertex_id selects a corner of a unit
// box edge, instance_id selects the active instance whose bounds expand it.

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
    float4 header;
    float4 bbox_min;
    float4 bbox_max;
    float4 build;
};

StructuredBuffer<float4> g_vertices : register(t0);
StructuredBuffer<WorldMeshGPU> g_meshes : register(t1);
StructuredBuffer<WorldInstanceGPU> g_instances : register(t2);
StructuredBuffer<uint> g_active : register(t3);

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

static const uint kEdges[12][2] = {
    {0, 1}, {2, 3}, {4, 5}, {6, 7},
    {0, 2}, {1, 3}, {4, 6}, {5, 7},
    {0, 4}, {1, 5}, {2, 6}, {3, 7},
};

VSOutput main(uint vertex_id : SV_VertexID, uint instance_id : SV_InstanceID)
{
    VSOutput output;
    const uint edge = min(vertex_id / 2u, 11u);
    const uint corner = kEdges[edge][vertex_id % 2u];
    const uint resolved_instance = g_active[instance_id];
    const WorldInstanceGPU instance = g_instances[resolved_instance];
    const float3 bounds_min = instance.bounds_min.xyz;
    const float3 bounds_max = instance.bounds_max.xyz;
    const float3 world = float3(
        (corner & 1u) ? bounds_max.x : bounds_min.x,
        (corner & 2u) ? bounds_max.y : bounds_min.y,
        (corner & 4u) ? bounds_max.z : bounds_min.z);
    output.position = mul(float4(world, 1.0), g_view_proj);
    output.world = world;
    output.instance_id = resolved_instance;
    output.mesh_id = 0u;
    return output;
}
