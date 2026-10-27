// world_rt12_deform_unpack.cs_6_5.hlsl - M4: the carrier (world_rt12_carrier.cs_5_0.hlsl) into the D3D12 float4 vertex
// buffer the character BLAS are built from, at the same vertex index. A vertex with a non-finite component gets a NaN x,
// which makes its triangles inactive in DXR (the compute test never hits such a triangle either).
Texture2D<uint4> g_carrier : register(t0);
RWStructuredBuffer<float4> g_vertices : register(u0);

cbuffer cb_unpack : register(b0)
{
    uint g_vertex_count;
};

[numthreads(64, 1, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= g_vertex_count) return;
    const float3 p = asfloat(g_carrier.Load(int3(id.x % 4096u, id.x / 4096u, 0)).xyz);
    g_vertices[id.x] = all(isfinite(p)) ? float4(p, 1.0) : float4(asfloat(0x7FC00000u), 0.0, 0.0, 1.0);
}
