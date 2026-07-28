#include "./common.hlsl"


struct CompositeToDisplayConstants
{
    float4 nits;
    float4 uiScale;
    float4 huePreserveLerp;
    uint   presentSurfaceBindlessIndex;
    uint   uiSurfaceBindlessIndex;
    uint   _unused_;
    uint   linearComposite;
    float3 pipelineToOutputColorSpace[3];
};

Texture2D<float4> srv2DTextures[65536] : register(t0, space1);

cbuffer cbCasConstants : register(b1)
{
    uint4 casConstants1        : packoffset(c0.x);
    uint  texIndexInputTexture : packoffset(c1.x);
    uint  pad[3]                : packoffset(c2.x);
};

cbuffer cbCompositeToDisplayConstants : register(b0)
{
    CompositeToDisplayConstants cbCompositeToDisplayConstants : packoffset(c0.x);
};

SamplerState g_staticSampler_TrilinearClamp : register(s20);

uint firstbithigh_msb(int value)  { return (value == 0) ? 0xFFFFFFFFu : (31u - firstbithigh(value)); }
uint firstbithigh_msb(uint value) { return (value == 0) ? 0xFFFFFFFFu : (31u - firstbithigh(value)); }

uint ResolveBindlessIndex(uint rawIndex)
{
    int  heapBits   = (int)rawIndex & (-1048576);
    bool isSentinel = (heapBits == (-1018167296));
    int  localIndex = (int)rawIndex & 1048575;
    return select(isSentinel, localIndex, 17);
}

float4 main(
    precise noperspective float4 SV_Position : SV_Position,
    linear float2 TEXCOORD : TEXCOORD
) : SV_Target
{
    float4 SV_Target;

    uint px = uint(SV_Position.x);
    uint py = uint(SV_Position.y);

    float sharpenAmount = asfloat(casConstants1.x);
    float sharpenLimit  = asfloat(casConstants1.y);

    uint inputTexIndex = ResolveBindlessIndex(texIndexInputTexture);
    float4 upSample = srv2DTextures[inputTexIndex + 0u].Load(int3(px, py + (uint)(-1), 0));
    float4 leftSample = srv2DTextures[inputTexIndex + 0u].Load(int3(px + (uint)(-1), py, 0));
    float4 centerSample = srv2DTextures[inputTexIndex + 0u].Load(int3(px, py, 0));
    float4 rightSample = srv2DTextures[inputTexIndex + 0u].Load(int3(px + 1u, py, 0));
    float4 downSample = srv2DTextures[inputTexIndex + 0u].Load(int3(px, py + 1u, 0));

    float maxAlpha0 = max(upSample.w, leftSample.w);
    float maxAlpha1 = max(maxAlpha0, rightSample.w);
    float maxAlpha2 = max(maxAlpha1, downSample.w);
    float maxAlphaScaled = maxAlpha2 * 0.800000011920929f;
    float maxAlphaFinal = max(maxAlphaScaled, centerSample.w);
    float alphaFade = 1.0f - maxAlphaFinal;
    float alphaFadeAmount = alphaFade * sharpenAmount;

    float minG0 = min(centerSample.y, rightSample.y);
    float minG1 = min(leftSample.y, minG0);
    float minG2 = min(upSample.y, downSample.y);
    float minG = min(minG1, minG2);

    float maxG0 = max(centerSample.y, rightSample.y);
    float maxG1 = max(leftSample.y, maxG0);
    float maxG2 = max(upSample.y, downSample.y);
    float maxG = max(maxG1, maxG2);

    int maxGAsInt = asint(maxG);
    uint rcpApproxBits = 2129690299u - (uint)maxGAsInt;
    float rcpApprox = asfloat(rcpApproxBits);
    float oneMinusMaxG = 1.0f - maxG;
    float contrastTerm = min(minG, oneMinusMaxG);
    float sharpenWeightRaw = rcpApprox * contrastTerm;
    float sharpenWeightSat = saturate(sharpenWeightRaw);
    float sharpenWeight = sharpenWeightSat * sharpenWeightSat;

    float3 lap = (centerSample.rgb * 4.0f) - upSample.rgb - leftSample.rgb - rightSample.rgb - downSample.rgb;
    float fadeLimitAmount = alphaFadeAmount * sharpenLimit;
    float3 perChannelAmount = float3(
        select(lap.x > 0.0f, fadeLimitAmount, alphaFadeAmount),
        select(lap.y > 0.0f, fadeLimitAmount, alphaFadeAmount),
        select(lap.z > 0.0f, fadeLimitAmount, alphaFadeAmount));
    float3 sceneColor = saturate(centerSample.rgb + (perChannelAmount * lap * sharpenWeight));

    float peak = RENODX_PEAK_WHITE_NITS / RENODX_DIFFUSE_WHITE_NITS;

    sceneColor = renodx::color::pq::DecodeSafe(sceneColor, RENODX_DIFFUSE_WHITE_NITS);

    sceneColor = renodx::color::correct::GammaSafe(sceneColor);

    sceneColor = NeutwoStockmanSharpeLMS(sceneColor, peak, 100.f);
    
    sceneColor = renodx::color::pq::EncodeSafe(sceneColor, RENODX_DIFFUSE_WHITE_NITS);

    bool linearCompositeIsZero = (cbCompositeToDisplayConstants.linearComposite == 0);

    uint uiTexIndexResolved = ResolveBindlessIndex(cbCompositeToDisplayConstants.uiSurfaceBindlessIndex);

    float uCentered = TEXCOORD.x + -0.5f;
    float vCentered = TEXCOORD.y + -0.5f;
    float uScaled = cbCompositeToDisplayConstants.uiScale.x * uCentered;
    float vScaled = cbCompositeToDisplayConstants.uiScale.y * vCentered;
    float uiU = uScaled + 0.5f;
    float uiV = vScaled + 0.5f;
    uint uiTexIndex = uiTexIndexResolved + 0u;

    float3 composited;

    if (!linearCompositeIsZero)
    {
        float4 uiSample = srv2DTextures[uiTexIndex].Sample(g_staticSampler_TrilinearClamp, float2(uiU, uiV));

        float uiInvAlpha = 1.0f - uiSample.w;
        float3 blend = uiInvAlpha * sceneColor;

        float3 uiNits = uiSample.rgb * cbCompositeToDisplayConstants.nits.y;

        composited = blend + uiNits;
        //composited = max(composited, 0);
    }
    else
    {
        float3 sceneLinear = renodx::color::pq::Decode(sceneColor, RENODX_GRAPHICS_WHITE_NITS);

        float4 uiSample = srv2DTextures[uiTexIndex].Sample(g_staticSampler_TrilinearClamp, float2(uiU, uiV));

        float uiInvAlpha = 1.0f - uiSample.w;
        float3 blend = uiInvAlpha * sceneLinear;

        float3 uiNits = uiSample.rgb * cbCompositeToDisplayConstants.nits.y;

        composited = blend + uiNits;
        //composited = max(composited, 0);
    }

    composited = renodx::color::bt2020::from::BT709(composited);

    float3 finalColor = renodx::color::pq::Encode(composited, RENODX_GRAPHICS_WHITE_NITS);

    SV_Target.rgb = finalColor;
    SV_Target.w = 1.0f;
    return SV_Target;
}
