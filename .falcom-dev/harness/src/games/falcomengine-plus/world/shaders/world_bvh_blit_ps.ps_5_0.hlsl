// world_bvh_blit_ps.ps_5_0.hlsl - present-time display of the BVH debug texture.

Texture2D<float4> g_debug : register(t0);
SamplerState g_sampler : register(s0);

float4 main(float4 position : SV_Position, float2 uv : TEXCOORD0) : SV_Target
{
    return float4(g_debug.Sample(g_sampler, uv).rgb, 1.0);
}
