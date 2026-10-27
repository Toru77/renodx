// world_rt12_noise.cs_5_0.hlsl - M3: the IS-FAST noise (128 x 128 x 32) into a shared 2D texture (128 x 4096,
// slice s at rows 128 s .. 128 s + 127; xy = asuint of the two values) for world_rt12_rtao.cs_6_5.hlsl.
Texture3D<float2> g_isfast_noise : register(t18);
RWTexture2D<uint4> g_rt12_noise : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    const float2 value = g_isfast_noise.Load(int4(id.x, id.y % 128u, id.y / 128u, 0));
    g_rt12_noise[id.xy] = uint4(asuint(value), 0u, 0u);
}
