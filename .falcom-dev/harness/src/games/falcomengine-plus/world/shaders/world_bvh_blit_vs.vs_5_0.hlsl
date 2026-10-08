// world_bvh_blit_vs.vs_5_0.hlsl - full-screen triangle for the debug view.

struct VSOutput
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

VSOutput main(uint vertex_id : SV_VertexID)
{
    VSOutput output;
    const float2 corner = float2((vertex_id << 1) & 2, vertex_id & 2);
    output.uv = corner;
    output.position = float4(corner * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);
    return output;
}
