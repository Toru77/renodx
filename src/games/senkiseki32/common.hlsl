#include "./shared.h"
#include "./DICE.hlsl"
#include "./macleod_boynton.hlsli"

// --- SAFETY CLAMP (The Fix) ---
// This function cleans up "illegal" pixels before they hit the Tone Mapper.
float3 Sanitize(float3 color)
{
    // 1. Kill Negatives (Math errors)
    color = max(color, 0.0);

    // 2. Cap Infinities (Exploding Fire/Lights)
    // 250.0 is extremely bright (far brighter than the sun in most games),
    // but safe enough for RenoDX to handle without glitching.
    color = min(color, 250.0); 

    // 3. Kill NaNs (Not a Number errors)
    // If a pixel is "broken" (NaN), this forces it to Black (0) so it doesn't spread.
    if (any(isnan(color))) color = 0.0;

    return color;
}

// --- INTERNAL HELPERS ---

float3 _CorrectHueMacLeodBoynton(float3 color, float3 reference_color)
{
    if (RENODX_TONE_MAP_TYPE == 0.f) return color;
    if (RENODX_TONE_MAP_HUE_CORRECTION <= 0.f) return color;

    float hue_correction_strength = saturate(RENODX_TONE_MAP_HUE_CORRECTION);
    return CorrectHueAndPurityMBGated(
        color,
        reference_color,
        hue_correction_strength,
        0.5f,
        2.f,
        hue_correction_strength,
        1.f);
}

float3 _CombineScene(float3 color, float3 glare, float3 toneFactor, float glowIntensity)
{
    color = renodx::color::srgb::DecodeSafe(color);
    glare = renodx::color::srgb::DecodeSafe(glare);
    return (color * toneFactor) + (glare * glowIntensity);
}

float3 _MultiplyBlendSrgb(float3 sceneColor, float3 tintColorSrgb, float blendFactor)
{
    blendFactor = saturate(blendFactor);
    float3 tintColor = renodx::color::srgb::DecodeSafe(tintColorSrgb);
    return sceneColor + (sceneColor * tintColor * blendFactor);
}

float3 _SmartBlendFilter(float3 sceneColor, float3 filterColor, float mixFactor)
{
    if (shader_injection.debug_disable_filter != 0.f) return sceneColor;
    return _MultiplyBlendSrgb(sceneColor, filterColor, mixFactor);
}

float3 _SmartBlendFilterDof(float3 sceneColor, float3 filterColor, float mixFactor, float dofFactor)
{
    if (shader_injection.debug_disable_filter != 0.f) return sceneColor;
    dofFactor = saturate(dofFactor);
    return _MultiplyBlendSrgb(sceneColor, filterColor, mixFactor * dofFactor);
}

float3 _ApplyMonotone(float3 color, float4 monotoneMul, float4 monotoneAdd)
{
    float luma = dot(color, float3(0.299, 0.587, 0.114));
    float3 monoColor = luma * monotoneMul.rgb + monotoneAdd.rgb;
    return lerp(color, monoColor, monotoneMul.w);
}

float3 _Tonemap(float3 color)
{
    color = Sanitize(color);
    float3 hue_reference_color = renodx::tonemap::Reinhard(max(0.f, color));

    if (shader_injection.tone_map_type == 1.f) {
        color = DICEToneMap(color);
        return _CorrectHueMacLeodBoynton(color, hue_reference_color);
    }

    renodx::draw::Config draw_config = renodx::draw::BuildConfig();
    // Disable RenoDRT internal hue correction/shift so MB is the only hue path.
    draw_config.tone_map_hue_correction = 0.f;
    draw_config.tone_map_hue_shift = 0.f;
    color = renodx::draw::ToneMapPass(color, draw_config);
    return _CorrectHueMacLeodBoynton(color, hue_reference_color);
}

float3 _FadeColor(float3 color, float4 fadingColor)
{
    if (shader_injection.debug_disable_fading != 0.f) return color;
    float fadeAlpha = saturate(fadingColor.w);
    float3 fadeTarget = renodx::color::srgb::DecodeSafe(fadingColor.rgb);
    return lerp(color, fadeTarget, fadeAlpha);
}

float3 _FadeTex(float3 color, float3 fadingTexRGB, float fadingTexA, float4 fadingColor)
{
    if (shader_injection.debug_disable_fading != 0.f) return color;
    float fadeAlpha = saturate(fadingColor.w * fadingTexA);
    float3 fadeTarget = renodx::color::srgb::DecodeSafe(fadingTexRGB * fadingColor.rgb);
    return lerp(color, fadeTarget, fadeAlpha);
}

// --- PUBLIC VARIANTS ---

// No filter, no extras
float3 ApplyRenoDX_NoFilter(float3 color, float3 glare, float3 toneFactor, float glowIntensity)
{
    float3 scene = _CombineScene(color, glare, toneFactor, glowIntensity);
    scene = _Tonemap(scene);
    return renodx::draw::RenderIntermediatePass(scene);
}

// Filter only (smart blend)
float3 ApplyRenoDX(float3 color, float3 glare, float3 filterColor, float mixFactor, float3 toneFactor, float glowIntensity)
{
    float3 scene = _CombineScene(color, glare, toneFactor, glowIntensity);
    scene = _SmartBlendFilter(scene, filterColor, mixFactor);
    scene = _Tonemap(scene);
    return renodx::draw::RenderIntermediatePass(scene);
}

// Monotone only
float3 ApplyRenoDX_Mono(float3 color, float3 glare, float3 toneFactor, float glowIntensity,
                        float4 monotoneMul, float4 monotoneAdd)
{
    float3 scene = _CombineScene(color, glare, toneFactor, glowIntensity);
    scene = _ApplyMonotone(scene, monotoneMul, monotoneAdd);
    scene = _Tonemap(scene);
    return renodx::draw::RenderIntermediatePass(scene);
}

// Fade with constant color
float3 ApplyRenoDX_FadeColor(float3 color, float3 glare, float3 toneFactor, float glowIntensity,
                             float4 fadingColor)
{
    float3 scene = _CombineScene(color, glare, toneFactor, glowIntensity);
    scene = _FadeColor(scene, fadingColor);
    scene = _Tonemap(scene);
    return renodx::draw::RenderIntermediatePass(scene);
}

// Fade with texture
float3 ApplyRenoDX_FadeTex(float3 color, float3 glare, float3 toneFactor, float glowIntensity,
                           float3 fadingTexRGB, float fadingTexA, float4 fadingColor)
{
    float3 scene = _CombineScene(color, glare, toneFactor, glowIntensity);
    scene = _FadeTex(scene, fadingTexRGB, fadingTexA, fadingColor);
    scene = _Tonemap(scene);
    return renodx::draw::RenderIntermediatePass(scene);
}

// Monotone + fade color
float3 ApplyRenoDX_MonoFadeColor(float3 color, float3 glare, float3 toneFactor, float glowIntensity,
                                 float4 monotoneMul, float4 monotoneAdd, float4 fadingColor)
{
    float3 scene = _CombineScene(color, glare, toneFactor, glowIntensity);
    scene = _ApplyMonotone(scene, monotoneMul, monotoneAdd);
    scene = _FadeColor(scene, fadingColor);
    scene = _Tonemap(scene);
    return renodx::draw::RenderIntermediatePass(scene);
}

// Monotone + fade color (vanilla-faithful: combine & monotone in gamma space)
float3 ApplyRenoDX_MonoFadeColor_Vanilla(float3 color, float3 glare, float3 toneFactor, float glowIntensity,
                                         float4 monotoneMul, float4 monotoneAdd, float4 fadingColor)
{
    // Combine in gamma space (matching vanilla math)
    float3 scene = color * toneFactor + glare * glowIntensity;
    // Monotone in gamma space (luma coefficients expect gamma values)
    scene = _ApplyMonotone(scene, monotoneMul, monotoneAdd);
    // Decode to linear for HDR tonemapping
    scene = renodx::color::srgb::DecodeSafe(scene);
    scene = _Tonemap(scene);
    scene = _FadeColor(scene, fadingColor);
    return renodx::draw::RenderIntermediatePass(scene);
}

// Monotone + fade texture
float3 ApplyRenoDX_MonoFadeTex(float3 color, float3 glare, float3 toneFactor, float glowIntensity,
                               float4 monotoneMul, float4 monotoneAdd,
                               float3 fadingTexRGB, float fadingTexA, float4 fadingColor)
{
    float3 scene = _CombineScene(color, glare, toneFactor, glowIntensity);
    scene = _ApplyMonotone(scene, monotoneMul, monotoneAdd);
    scene = _FadeTex(scene, fadingTexRGB, fadingTexA, fadingColor);
    scene = _Tonemap(scene);
    return renodx::draw::RenderIntermediatePass(scene);
}

// Filter + monotone
float3 ApplyRenoDX_FilterMono(float3 color, float3 glare, float3 filterColor, float mixFactor,
                              float3 toneFactor, float glowIntensity,
                              float4 monotoneMul, float4 monotoneAdd)
{
    float3 scene = _CombineScene(color, glare, toneFactor, glowIntensity);
    scene = _SmartBlendFilter(scene, filterColor, mixFactor);
    scene = _ApplyMonotone(scene, monotoneMul, monotoneAdd);
    scene = _Tonemap(scene);
    return renodx::draw::RenderIntermediatePass(scene);
}

// Filter + fade color
float3 ApplyRenoDX_FilterFadeColor(float3 color, float3 glare, float3 filterColor, float mixFactor,
                                   float3 toneFactor, float glowIntensity, float4 fadingColor)
{
    float3 scene = _CombineScene(color, glare, toneFactor, glowIntensity);
    scene = _SmartBlendFilter(scene, filterColor, mixFactor);
    scene = _FadeColor(scene, fadingColor);
    scene = _Tonemap(scene);
    return renodx::draw::RenderIntermediatePass(scene);
}

// Filter + fade texture
float3 ApplyRenoDX_FilterFadeTex(float3 color, float3 glare, float3 filterColor, float mixFactor,
                                 float3 toneFactor, float glowIntensity,
                                 float3 fadingTexRGB, float fadingTexA, float4 fadingColor)
{
    float3 scene = _CombineScene(color, glare, toneFactor, glowIntensity);
    scene = _SmartBlendFilter(scene, filterColor, mixFactor);
    scene = _FadeTex(scene, fadingTexRGB, fadingTexA, fadingColor);
    scene = _Tonemap(scene);
    return renodx::draw::RenderIntermediatePass(scene);
}

// Filter + monotone + fade color
float3 ApplyRenoDX_FilterMonoFadeColor(float3 color, float3 glare, float3 filterColor, float mixFactor,
                                       float3 toneFactor, float glowIntensity,
                                       float4 monotoneMul, float4 monotoneAdd, float4 fadingColor)
{
    float3 scene = _CombineScene(color, glare, toneFactor, glowIntensity);
    scene = _SmartBlendFilter(scene, filterColor, mixFactor);
    scene = _ApplyMonotone(scene, monotoneMul, monotoneAdd);
    scene = _FadeColor(scene, fadingColor);
    scene = _Tonemap(scene);
    return renodx::draw::RenderIntermediatePass(scene);
}

// Filter + monotone + fade texture
float3 ApplyRenoDX_FilterMonoFadeTex(float3 color, float3 glare, float3 filterColor, float mixFactor,
                                     float3 toneFactor, float glowIntensity,
                                     float4 monotoneMul, float4 monotoneAdd,
                                     float3 fadingTexRGB, float fadingTexA, float4 fadingColor)
{
    float3 scene = _CombineScene(color, glare, toneFactor, glowIntensity);
    scene = _SmartBlendFilter(scene, filterColor, mixFactor);
    scene = _ApplyMonotone(scene, monotoneMul, monotoneAdd);
    scene = _FadeTex(scene, fadingTexRGB, fadingTexA, fadingColor);
    scene = _Tonemap(scene);
    return renodx::draw::RenderIntermediatePass(scene);
}

// Filter + DOF
float3 ApplyRenoDX_FilterDof(float3 color, float3 glare, float3 filterColor, float mixFactor,
                             float dofFactor, float3 toneFactor, float glowIntensity)
{
    float3 scene = _CombineScene(color, glare, toneFactor, glowIntensity);
    scene = _SmartBlendFilterDof(scene, filterColor, mixFactor, dofFactor);
    scene = _Tonemap(scene);
    return renodx::draw::RenderIntermediatePass(scene);
}

// Filter + DOF + fade texture (fade in gamma space, filter via smart blend)
float3 ApplyRenoDX_FilterDofFadeTex_Vanilla(float3 color, float3 glare, float3 filterColor, float filterAlpha,
                                            float dofFactor, float3 toneFactor, float glowIntensity,
                                            float3 fadingTexRGB, float fadingTexA, float4 fadingColor)
{
    float3 scene = color * toneFactor + glare * glowIntensity;
    if (shader_injection.debug_disable_fading == 0.f) {
        float3 fadeTarget = fadingTexRGB * fadingColor.rgb;
        float fadeAlpha = saturate(fadingColor.w * fadingTexA);
        scene = lerp(scene, fadeTarget, fadeAlpha);
    }
    // Decode to linear, then apply filter via proven smart blend
    scene = renodx::color::srgb::DecodeSafe(scene);
    scene = _SmartBlendFilterDof(scene, filterColor, filterAlpha, dofFactor);
    scene = _Tonemap(scene);
    return renodx::draw::RenderIntermediatePass(scene);
}


