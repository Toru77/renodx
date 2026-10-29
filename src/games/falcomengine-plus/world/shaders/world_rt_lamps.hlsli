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

// The game's DeferredParam (144 bytes). Only dynamicLightIntensity (byte offset 76 = .w of mid) is read.
struct GameDeferredParam
{
    float4 head[4];
    float4 mid;
    float4 tail[4];
};
