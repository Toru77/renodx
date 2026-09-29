///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// falcomengine-plus GTVBAO — Multi-bounce light buffer accumulation
//
// Adds previous frame's denoised GI to the HDR color buffer, creating
// an accumulated light buffer that enables multi-bounce GI feedback.
// Runs before the main pass when multi-bounce is enabled.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "gtvbao_common.hlsl"

// Softness of the per-pixel feedback gate, in units of the measured gain
// ratio (GI luminance over local direct-light luminance). Below this ratio a
// pixel is treated as having no meaningful bounce opportunity and receives a
// proportionally reduced share, which keeps the feedback free of the noise
// amplification a hard "always add the full fraction" would cause.
#define GTVBAO_MB_BOUNCE_KNEE 0.01

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

    // Apply max clamp to prevent over-brightening from feedback accumulation
    if (g_gi_multibounce_max_clamp > 0.0) {
      prevGI.rgb = min(prevGI.rgb, g_gi_multibounce_max_clamp);
    }

    // Multi-bounce feedback, normalised per pixel.
    //
    // The previous form injected the raw GI (`prevGI * strength`), but prevGI
    // has already been attenuated by everything the gather did to it: light
    // exposure, the newCount/32 sector weight, and both the receiver cosine
    // (NdotL) and the sample cosine (NsDotL). That product is a per-pixel gain
    // which swings from 0 (fully occluded, no new sectors) to ~0.15 (open and
    // facing). No single strength value can make the bounce visible on that
    // spread: low enough to be safe on the bright pixels does nothing on the
    // rest, and high enough to show there diverges on those.
    //
    // So measure the gain per pixel instead of assuming it, and rescale prevGI
    // so its luminance lands on an explicit target: the bounce fraction of
    // this pixel's OWN direct light. Every pixel then gets the same relative
    // bounce rather than a 30x spread that follows the geometry.
    //
    // The target is gated by ratio/(ratio+knee) so a pixel with no real bounce
    // opportunity injects nothing instead of amplifying noise by an unbounded
    // factor. That keeps two useful bounds:
    //   scale      <= bounceFrac / knee          (no unbounded amplification)
    //   added luma <= bounceFrac * luma(color)  (no unbounded accumulation)
    // Because the injected luminance is capped at a fraction of the CURRENT
    // frame's direct light -- which never contains the feedback -- the loop
    // cannot run away at any strength. The runaway that the source paper warns
    // about is structurally impossible here, which is also why the max clamp
    // can stay off by default.
    const float colorLuma = dot(color.rgb,  float3(0.299, 0.587, 0.114));
    // Recompute after the saturation lerp and the optional clamp above, so the
    // measured gain reflects the GI actually being injected.
    const float giLuma = dot(prevGI.rgb, float3(0.299, 0.587, 0.114));

    // Bounce fraction of the local direct light, trimmable by the existing
    // strength slider. Saturating caps the pair so an extreme strength cannot
    // push the total past doubling the local light.
    const float bounceFrac = saturate(g_gi_multibounce_bounce_fraction *
                                      max(g_gi_multibounce_strength, 0.0));

    const float ratio  = giLuma / max(colorLuma, 1e-6);
    const float gate   = ratio / (ratio + GTVBAO_MB_BOUNCE_KNEE);
    const float targetLuma = colorLuma * bounceFrac * gate;
    // Zero-GI pixels land here as 0 / eps = 0, so they inject exactly nothing.
    const float scale  = targetLuma / max(giLuma, 1e-6);

    g_outAccumulated[dt] = color + prevGI * scale;
}
