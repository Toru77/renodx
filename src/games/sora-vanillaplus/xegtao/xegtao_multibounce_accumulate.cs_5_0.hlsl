///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// sora-vanillaplus XeGTAO — Multi-bounce light buffer accumulation
//
// Adds previous frame's denoised GI to the HDR color buffer, creating
// an accumulated light buffer that enables multi-bounce GI feedback.
// Runs before the main pass when multi-bounce is enabled.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "xegtao_common.hlsl"

Texture2D<float4>  g_srcColor      : register(t0);  // HDR pre-lighting color texture
Texture2D<float4>  g_srcPreviousGI : register(t1);  // Previous frame's denoised GI
SamplerState       g_samplerPoint  : register(s0);
RWTexture2D<float4> g_outAccumulated : register(u0);

[numthreads(8, 8, 1)]
void main(uint2 dt : SV_DispatchThreadID)
{
    uint w, h;
    g_srcColor.GetDimensions(w, h);
    if (dt.x >= w || dt.y >= h) return;

    float4 color  = g_srcColor.Load(int3(dt, 0));
    float4 prevGI = g_srcPreviousGI.Load(int3(dt, 0));

    // Apply saturation to previous GI: lerp between grayscale and full color.
    float prevLuma = dot(prevGI.rgb, float3(0.299, 0.587, 0.114));
    prevGI.rgb = lerp(prevLuma.xxx, prevGI.rgb, g_gi_multibounce_saturation);

    // Boost previous GI by multi-bounce strength to bridge the scale gap
    // between GI (0-2) and HDR direct light (0-50+).
    // Default 1.0 = natural feedback; increase for stronger multi-bounce.
    g_outAccumulated[dt] = color + prevGI * g_gi_multibounce_strength;
}
