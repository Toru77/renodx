#include "./common.hlsl"
#include "../../shaders/canvas.hlsl"

struct LutParameters
{
    float4 scale;
    uint   texIndex;
    uint3  pad;
};

Texture2D<float4> srv2DTextures[65536] : register(t0, space1);
Texture3D<float4> srv3DTextures[65536] : register(t0, space2);

cbuffer cbDefaultSamplerIndices : register(b127)
{
    uint4 g_defaultSamplerStateIndices[4] : packoffset(c0.x);
};

cbuffer cbPostprocessParameters : register(b5)
{
    float4 ps_pixel_size                     : packoffset(c0.x);
    float4 ps_scale                          : packoffset(c1.x);
    float4 ps_intensity                      : packoffset(c2.x);
    float4 psScreenSize                      : packoffset(c3.x);
    float4 psPreGradeTonemapAndPaperWhite    : packoffset(c4.x);
    float4 psHdrDebugParams                  : packoffset(c5.x);
    float4 psHdrDebugColor                   : packoffset(c6.x);
    float4 psViewportBounds                  : packoffset(c7.x);
};

cbuffer cbPostprocessSource : register(b3)
{
    uint source0Tex2dIndex : packoffset(c0.x);
    uint source1Tex2dIndex : packoffset(c0.y);
    uint source2Tex2dIndex : packoffset(c0.z);
    uint source3Tex2dIndex : packoffset(c0.w);
    uint source4Tex2dIndex : packoffset(c1.x);
    uint source5Tex2dIndex : packoffset(c1.y);
    uint source6Tex2dIndex : packoffset(c1.z);
    uint source7Tex2dIndex : packoffset(c1.w);
};

cbuffer cbPostprocessColorGradingParameters : register(b2)
{
    LutParameters psLutParameters[8]        : packoffset(c0.x);
    float4 psHueSaturationMatrix[3]         : packoffset(c16.x);
    float4 psGamma                          : packoffset(c19.x);
    float  psLutLerp                        : packoffset(c20.x);
    float  psLutLerpDv                      : packoffset(c20.y);
    float  psSdrGamma                       : packoffset(c20.z);
    float  psPaperWhite                     : packoffset(c20.w);
    float  psBrightnessAdjustment           : packoffset(c21.x);
    float3 psPad                            : packoffset(c21.y);
    float3 psColorblindTransform[3]         : packoffset(c22.x);
};

SamplerState g_bindlessSamplerState[256] : register(s0, space1);
SamplerState g_staticSampler_TrilinearClamp : register(s20);

uint firstbithigh_msb(int value)  { return (value == 0) ? 0xFFFFFFFFu : (31u - firstbithigh(value)); }
uint firstbithigh_msb(uint value) { return (value == 0) ? 0xFFFFFFFFu : (31u - firstbithigh(value)); }

uint ResolveBindlessIndex(uint rawIndex, int selectFallback)
{
    int  heapBits   = (int)rawIndex & (-1048576);
    bool isSentinel = (heapBits == (-1018167296));
    int  localIndex = (int)rawIndex & 1048575;
    return select(isSentinel, localIndex, selectFallback);
}

float4 main(
    precise noperspective float4 SV_Position : SV_Position,
    linear float4 TEXCOORD : TEXCOORD
) : SV_Target
{
    float4 SV_Target;

    uint source0TexIndex = ResolveBindlessIndex(source0Tex2dIndex, 17); 
    int  source0SamplerIdxClamped = (int)min((uint)(g_defaultSamplerStateIndices[0].x), (uint)255); 
    uint source0SamplerIdx = (uint)source0SamplerIdxClamped + 0u; // _21
    float4 source0Sample = srv2DTextures[source0TexIndex].Sample(g_bindlessSamplerState[source0SamplerIdx], float2(TEXCOORD.x, TEXCOORD.y)); // _23

    float3 source0 = min(source0Sample.rgb, 65504.0f); 

    uint source1TexIndex = ResolveBindlessIndex(source1Tex2dIndex, 17); 
    int  source1SamplerIdxClamped = (int)min((uint)(g_defaultSamplerStateIndices[0].z), (uint)255); // _40
    uint source1SamplerIdx = (uint)source1SamplerIdxClamped + 0u; // _41
    float4 source1Sample = srv2DTextures[source1TexIndex].Sample(g_bindlessSamplerState[source1SamplerIdx], float2(TEXCOORD.x, TEXCOORD.y)); // _43

    float3 sum = source1Sample.rgb + source0;

    sum = renodx::color::pq::Encode(sum, 200.f);

    uint lutTexIndex = ResolveBindlessIndex(psLutParameters[5].texIndex, 18);

    float3 lutCoord0 = sum + -0.5f;
    float3 lutCoord1 = psLutParameters[5].scale.xyz * lutCoord0;
    float3 lutCoord = lutCoord1 + 0.5f;


    float4 lutSample = srv3DTextures[lutTexIndex].SampleLevel(g_staticSampler_TrilinearClamp, lutCoord, 0.0f); // _153

    lutSample.rgb = renodx::color::pq::Decode(lutSample.rgb, 200.f);
    lutSample.rgb = renodx::color::pq::Encode(lutSample.rgb, RENODX_DIFFUSE_WHITE_NITS);


    float radialDistSq = dot(float2(TEXCOORD.z, TEXCOORD.w), float2(TEXCOORD.z, TEXCOORD.w)); // _157
    float radialDist    = sqrt(radialDistSq);

    float radialScaled = ps_scale.x * radialDist;
    float radialBiased = radialScaled + ps_scale.y;
    float radialSat     = saturate(radialBiased);
    float radialWeighted = radialSat * ps_scale.z;
    float blendFactor     = ps_scale.w + radialWeighted;

    float3 final = blendFactor * lutSample.rgb;

    SV_Target.rgb = final;

    //renodx::canvas::Context debug_canvas = renodx::canvas::CreateContext(
    //    SV_Position.xy,
    //    float2(24.f, 24.f),
    //    float2(20.f, 30.f),
    //    SV_Target.rgb,
    //    1.f,
    //    float3(1.f, 1.f, 1.f),
    //    1.f,
    //    1.f);

    //renodx::canvas::DrawText(debug_canvas, 'p', 'r', 'e', ' ', 'z', ':', ' ');
    //renodx::canvas::DrawFloat(debug_canvas, psPreGradeTonemapAndPaperWhite.z, 4.f, 6.f, false, false);

    //SV_Target.rgb = renodx::canvas::GetOutput(debug_canvas).rgb;
    return SV_Target;
}
