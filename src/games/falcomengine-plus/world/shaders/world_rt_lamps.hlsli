// Game lamp layouts and falloff (Sora 2nd lighting 0xCA3D8596) and the RTGI lamp type bits, shared by the RTGI and RT Local Shadows shaders.
#define RTGI_LAMP_TYPE_POINT 1u
#define RTGI_LAMP_TYPE_SPOT 2u

// The game's LightParam (sora2nd/lighting/lighting_0xCA3D8596.ps_5_0.hlsl): 112 bytes. The full layout sets the stride.
struct LightParam
{
    float3 pos;
    float radius;
    float3 color;
    float radiusInv;
    float3 charaColor;
    float attenuation;
    float3 vec;
    float spotAngleInv;
    float attenuationAngle;
    float specularIntensity;
    float specularGlossiness;
    float scatterAnisotropy;
    float3 scatterColor;
    float scatterDensity;
    float translucency;
    int shadowmapIndex;
    float scatterOffset;
    float userParam;
};

// The game's LightIndexData (lighting_0xCA3D8596.ps_5_0.hlsl): 576 bytes per tile cell.
struct LightIndexData
{
    int pointLightIndices[63];     // Offset:    0
    uint pointLightCount;          // Offset:  252
    int spotLightIndices[63];      // Offset:  256
    uint spotLightCount;           // Offset:  508
    int lightProbeIndices[14];     // Offset:  512
    uint lightProbeCount;          // Offset:  568
    float tileDepthInv;            // Offset:  572
};

// The game's lamp distance attenuation for the vector from the shaded point to the lamp (lighting_0xCA3D8596.ps_5_0.hlsl lines 1297-1307).
float LampDistanceAttenuation(LightParam lamp, float3 to_lamp)
{
    return max(0.0, 1.0 - pow(abs(length(to_lamp) * lamp.radiusInv), lamp.attenuation));
}

// The game's spot cone factor for the unit direction from the shaded point to the lamp (lighting lines 1387-1400).
float LampSpotCone(LightParam lamp, float3 lamp_dir)
{
    return max(0.0, 1.0 - pow((1.0 - max(0.0, dot(lamp_dir, lamp.vec))) * lamp.spotAngleInv, lamp.attenuationAngle));
}

// The game's point and spot lamp light on a character pixel: lamp.charaColor x distance attenuation x spot cone x
// max(translucency, N.L), each class weighted by its visibility. The tile cell is the one the Sora 2nd environment
// path computes (lighting_0xCA3D8596.ps_5_0.hlsl lines 829-840). A class that is off adds nothing.
float3 CharacterLampLight(StructuredBuffer<LightParam> lights, StructuredBuffer<LightIndexData> indices, float2 tile_size_inv,
                          float2 sv_position, float view_depth, float3 position, float3 n,
                          bool points_on, bool spots_on, float point_vis, float spot_vis)
{
    const uint2 tile = (uint2)(tile_size_inv * sv_position);
    const uint row = tile.y << 5;
    const uint slice = (uint)clamp(indices[tile.x + row].tileDepthInv * -view_depth, 0.0, 7.0);
    const uint cell = min(4607u, slice * 576u + row + tile.x);
    const uint points = points_on ? min(63u, indices[cell].pointLightCount) : 0u;
    const uint spots = spots_on ? min(63u, indices[cell].spotLightCount) : 0u;
    float3 lamp_sum = float3(0.0, 0.0, 0.0);
    for (uint i = 0u; i < points + spots; ++i)
    {
        const bool spot = i >= points;
        const LightParam lamp = lights[spot ? indices[cell].spotLightIndices[i - points] : indices[cell].pointLightIndices[i]];
        if (lamp.radius <= 0.0)
            continue;
        const float3 to_lamp = lamp.pos - position;
        const float3 lamp_dir = normalize(to_lamp);
        const float att = LampDistanceAttenuation(lamp, to_lamp) * (spot ? LampSpotCone(lamp, lamp_dir) : 1.0);
        if (att <= 0.0)
            continue;
        lamp_sum += lamp.charaColor * (att * max(lamp.translucency, dot(n, lamp_dir)) * (spot ? spot_vis : point_vis));
    }
    return lamp_sum;
}

// The game's DeferredParam (144 bytes). dynamicLightIntensity is byte offset 76 (.w of mid); the flag word is byte 136 (tail[3].z, read with asuint).
struct GameDeferredParam
{
    float4 head[4];
    float4 mid;
    float4 tail[4];
};
