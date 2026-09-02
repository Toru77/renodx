// FalcomSSRBlurCS.cs_5_0.hlsl — Falcom Engine+ generic separable Gaussian blur for SSR.
// Blurs the SSR result (rgb = color, a = confidence) in one axis per dispatch.
// Horizontal pass: raw -> blur_h. Vertical pass: blur_h -> blur.
// sigma <= 0.01 acts as identity (sharp/raw SSR).
// Input : t0 source (raw or blur_h), b13 { sigma, horizontal }, s0 point clamp
// Output: u0 dest (blur_h or blur)

cbuffer cb_blur : register(b13)
{
    float g_sigma;
    float g_horizontal;
};

Texture2D<float4>  g_inTex : register(t0);
SamplerState       g_pointClamp : register(s0);

RWTexture2D<float4> g_outTex : register(u0);

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

    // Confidence-weighted accumulation: color contributes only where SSR has
    // valid confidence (a), so SSR color never bleeds into low/miss regions.
    float3 accumColor = 0.0;
    float accumConfidence = 0.0;
    float totalWeight = 0.0;
    for (int d = -radius; d <= radius; ++d) {
        float wgt = exp(-float(d) * float(d) * invSigma2);
        int2 tap = (g_horizontal > 0.5)
            ? int2(px.x + d, px.y)
            : int2(px.x, px.y + d);
        tap = clamp(tap, int2(0, 0), int2(w, h) - int2(1, 1));
        float4 sample = g_inTex.Load(int3(tap, 0));
        float conf = sample.a;
        accumColor += sample.rgb * conf * wgt;
        accumConfidence += conf * wgt;
        totalWeight += wgt;
    }
    float finalConfidence = accumConfidence / max(totalWeight, 1e-4);
    float3 finalColor = accumColor / max(accumConfidence, 1e-4);
    g_outTex[px] = float4(max(0.0, finalColor), finalConfidence);
}