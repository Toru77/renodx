// FalcomSSRBlurCS.cs_5_0.hlsl — Falcom Engine+ separable bilateral blur for SSR.
// Blurs the SSR result (rgb = color, a = confidence) in one axis per dispatch.
// Horizontal pass: raw -> blur_h. Vertical pass: blur_h -> blur.
// sigma <= 0.01 acts as identity (sharp/raw SSR).
// Gaussian spatial weighting x confidence-gated color accumulation x
// screen-depth bilateral rejection (relative linearized view distance) x
// center-preserving confidence. Both passes reference ORIGINAL scene depth.
// Input : t0 source (raw or blur_h), t1 captured scene depth (HW depth, SSR grid),
//         b0 cb_scene (proj for depth unpack, same convention as SSR march),
//         b13 { sigma, horizontal }, s0 point clamp
// Output: u0 dest (blur_h or blur)

cbuffer cb_scene : register(b0)
{
    float4x4 proj_g : packoffset(c8);
};

cbuffer cb_blur : register(b13)
{
    float g_sigma;
    float g_horizontal;
    float g_symmetricWeights;  // 0 = legacy tap loop (default), 1 = symmetric-pair loop (A/B test)
};

Texture2D<float4>  g_inTex : register(t0);
Texture2D<float>   g_depthTex : register(t1);
SamplerState       g_pointClamp : register(s0);

RWTexture2D<float4> g_outTex : register(u0);

static const float SSR_BLUR_FLT_MAX = 3.402823466e+38;
static const float kDepthRelTol = 0.25;  // relative view-distance bilateral tolerance (no UI yet)
static const float kConfEpsilon = 0.01;  // minimum confidence for color donors

// ── Depth linearization (identical math to FalcomSSRCS; handles standard and
//    reversed Z by sign guard; gives positive view-space distance). ──
void GetDepthUnpackConsts(out float mul_c, out float add_c)
{
    mul_c = -proj_g[3][2];
    add_c =  proj_g[2][2];
    if (mul_c * add_c < 0.0) add_c = -add_c;
}

float LinearizeDepth(float ndc_depth, float mul_c, float add_c)
{
    float denom = add_c - ndc_depth;
    float z = (abs(denom) > 1e-8) ? (mul_c / denom) : 0.0;
    z = max(z, 0.0);
    return isfinite(z) && z > 0.0 ? z : SSR_BLUR_FLT_MAX;
}

// SSR depth validity convention (matches FalcomSSRCS sky test).
bool IsSceneDepthValid(float hw_depth)
{
    return hw_depth > 0.0 && hw_depth < 1.0;
}

[numthreads(8, 8, 1)]
void main(uint3 dtid : SV_DispatchThreadID)
{
    uint w, h;
    g_outTex.GetDimensions(w, h);
    if (dtid.x >= w || dtid.y >= h) return;

    const int2 px = int2(dtid.xy);

    if (g_sigma <= 0.01) {
        g_outTex[px] = g_inTex.Load(int3(px, 0));
        return;
    }

    const int radius = min((int)ceil(3.0 * g_sigma), 16);
    const float sigma = max(g_sigma, 1e-3);
    const float invSigma2 = 0.5 / (sigma * sigma);

    // Center reference: own confidence stays authoritative; own linearized
    // depth is the bilateral reference (original scene depth, both passes).
    // Depth-unpack constants are uniform for the whole dispatch; derive once
    // instead of per depth sample (identical values, less ALU).
    float blurUnpackMul, blurUnpackAdd;
    GetDepthUnpackConsts(blurUnpackMul, blurUnpackAdd);
    float centerConf = g_inTex.Load(int3(px, 0)).a;
    float centerRaw = g_depthTex.Load(int3(px, 0));
    bool centerValid = IsSceneDepthValid(centerRaw);
    float centerLin = centerValid ? LinearizeDepth(centerRaw, blurUnpackMul, blurUnpackAdd) : 0.0;

    // Confidence-gated color accumulation (valid donors only) x Gaussian x
    // depth-bilateral weight; confidence accumulation stays depth-independent
    // so unrelated neighbors cannot collapse the center's confidence.
    float3 colorNum = 0.0;
    float colorDen = 0.0;
    float confNum = 0.0;
    float confDen = 0.0;
    if (g_symmetricWeights > 0.5) {
        // A/B path: center tap once, then symmetric pairs sharing one exp().
        // Same tap set and encounter order as the legacy loop below.
        {
            float4 sample = g_inTex.Load(int3(px, 0));
            float conf = sample.a;
            if (conf > kConfEpsilon) {
                colorNum += sample.rgb * conf;
                colorDen += conf;
            }
            confNum += conf;
            confDen += 1.0;
        }
        for (int dd = 1; dd <= radius; ++dd) {
            float wgt = exp(-float(dd) * float(dd) * invSigma2);
            for (int s = -1; s <= 1; s += 2) {
                int2 tap = (g_horizontal > 0.5)
                    ? int2(px.x + dd * s, px.y)
                    : int2(px.x, px.y + dd * s);
                tap = clamp(tap, int2(0, 0), int2(w, h) - int2(1, 1));
                float4 sample = g_inTex.Load(int3(tap, 0));
                float conf = sample.a;
                float depthW = 1.0;
                if (centerValid) {
                    float tapRaw = g_depthTex.Load(int3(tap, 0));
                    if (!IsSceneDepthValid(tapRaw)) {
                        depthW = 0.0;  // sky tap: never a donor
                    } else {
                        float tapLin = LinearizeDepth(tapRaw, blurUnpackMul, blurUnpackAdd);
                        float relDiff = abs(tapLin - centerLin) / max(centerLin, 1e-4);
                        float depthRatio = relDiff / kDepthRelTol;
                        depthW = exp(-depthRatio * depthRatio);
                    }
                }
                if (conf > kConfEpsilon) {
                    float effW = wgt * depthW;
                    colorNum += sample.rgb * conf * effW;
                    colorDen += conf * effW;
                }
                confNum += conf * wgt;
                confDen += wgt;
            }
        }
    } else {
    for (int d = -radius; d <= radius; ++d) {
        float wgt = exp(-float(d) * float(d) * invSigma2);
        int2 tap = (g_horizontal > 0.5)
            ? int2(px.x + d, px.y)
            : int2(px.x, px.y + d);
        tap = clamp(tap, int2(0, 0), int2(w, h) - int2(1, 1));
        float4 sample = g_inTex.Load(int3(tap, 0));
        float conf = sample.a;
        float depthW = 1.0;
        if (centerValid) {
            float tapRaw = g_depthTex.Load(int3(tap, 0));
            if (!IsSceneDepthValid(tapRaw)) {
                depthW = 0.0;  // sky tap: never a donor
            } else {
                float tapLin = LinearizeDepth(tapRaw, blurUnpackMul, blurUnpackAdd);
                float relDiff = abs(tapLin - centerLin) / max(centerLin, 1e-4);
                float depthRatio = relDiff / kDepthRelTol;
                depthW = exp(-depthRatio * depthRatio);
            }
        }
        if (conf > kConfEpsilon) {
            float effW = wgt * depthW;
            colorNum += sample.rgb * conf * effW;
            colorDen += conf * effW;
        }
        confNum += conf * wgt;
        confDen += wgt;
    }
    }
    float meanConf = confNum / max(confDen, 1e-4);
    float finalConfidence = max(centerConf, meanConf);
    float3 finalColor = colorNum / max(colorDen, 1e-4);
    g_outTex[px] = float4(max(0.0, finalColor), finalConfidence);
}
