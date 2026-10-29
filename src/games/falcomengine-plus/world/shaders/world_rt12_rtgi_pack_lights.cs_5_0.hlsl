// world_rt12_rtgi_pack_lights.cs_5_0.hlsl - M2 DXR RTGI lighting inputs (rt12_backend.hpp RunRtgi). The mrt1, shadow and mrt2
// textures are copied at their own sizes. The deferred params, the lamps and the lamp types are copied into carriers:
// deferred texel t = y * 4096 + x holds parameter t / 9 as float4 (t % 9); lamp texel x holds lamp x / 7 as words
// (x % 7) of its LightParam. Each carrier is written only below its source's element count.
#include "world_rt_lamps.hlsli"

Texture2D<uint4> g_mrt1 : register(t0);
Texture2D<float4> g_shadow_mask : register(t1);
Texture2D<uint2> g_mrt2 : register(t2);
StructuredBuffer<GameDeferredParam> g_deferred : register(t3);
StructuredBuffer<LightParam> g_lamps : register(t4);
StructuredBuffer<uint> g_lamp_types : register(t5);
RWTexture2D<uint4> g_pack_mrt1 : register(u0);
RWTexture2D<float> g_pack_shadow : register(u1);
RWTexture2D<uint2> g_pack_mrt2 : register(u2);
RWTexture2D<uint4> g_pack_deferred : register(u3);
RWTexture2D<uint4> g_pack_lamps : register(u4);
RWTexture2D<uint> g_pack_lamp_types : register(u5);

// The 28 words of a LightParam in declaration order as 7 texels (raw bits).
uint4 LampTexel(LightParam lamp, uint k)
{
    switch (k)
    {
    case 0u: return asuint(float4(lamp.pos, lamp.radius));
    case 1u: return asuint(float4(lamp.color, lamp.radiusInv));
    case 2u: return asuint(float4(lamp.charaColor, lamp.attenuation));
    case 3u: return asuint(float4(lamp.vec, lamp.spotAngleInv));
    case 4u: return asuint(float4(lamp.attenuationAngle, lamp.specularIntensity, lamp.specularGlossiness, lamp.scatterAnisotropy));
    case 5u: return asuint(float4(lamp.scatterColor, lamp.scatterDensity));
    default: return uint4(asuint(lamp.translucency), (uint)lamp.shadowmapIndex, asuint(lamp.scatterOffset), asuint(lamp.userParam));
    }
}

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    uint2 size;
    g_mrt1.GetDimensions(size.x, size.y);
    if (all(id.xy < size)) g_pack_mrt1[id.xy] = g_mrt1.Load(int3(id.xy, 0));
    g_shadow_mask.GetDimensions(size.x, size.y);
    if (all(id.xy < size)) g_pack_shadow[id.xy] = g_shadow_mask.Load(int3(id.xy, 0)).x;
    g_mrt2.GetDimensions(size.x, size.y);
    if (all(id.xy < size)) g_pack_mrt2[id.xy] = g_mrt2.Load(int3(id.xy, 0));

    const uint texel = id.y * 4096u + id.x;
    uint deferred_count;
    uint deferred_stride;
    g_deferred.GetDimensions(deferred_count, deferred_stride);
    if (id.x < 4096u && id.y < 44u && texel < deferred_count * 9u)
    {
        const GameDeferredParam p = g_deferred[texel / 9u];
        const uint k = texel % 9u;
        float4 v = p.mid;
        if (k < 4u) v = p.head[k];
        else if (k > 4u) v = p.tail[k - 5u];
        g_pack_deferred[id.xy] = asuint(v);
    }

    uint lamp_count;
    uint lamp_stride;
    g_lamps.GetDimensions(lamp_count, lamp_stride);
    if (id.y == 0u && id.x < 7168u && id.x < lamp_count * 7u)
    {
        g_pack_lamps[uint2(id.x, 0u)] = LampTexel(g_lamps[id.x / 7u], id.x % 7u);
    }

    uint lamp_type_count;
    uint lamp_type_stride;
    g_lamp_types.GetDimensions(lamp_type_count, lamp_type_stride);
    if (id.y == 0u && id.x < 1024u && id.x < lamp_type_count) g_pack_lamp_types[uint2(id.x, 0u)] = g_lamp_types[id.x];
}
