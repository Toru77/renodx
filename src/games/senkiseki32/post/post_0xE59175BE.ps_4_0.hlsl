// 0xE59175BE - post with glow, fade texture
#include "../shared.h"
#include "../common.hlsl"
cbuffer _Globals : register(b0)
{
  float3 scene_EyePosition : packoffset(c0);
  float4x4 scene_View : packoffset(c1);
  float4x4 scene_ViewProjection : packoffset(c5);
  float3 scene_GlobalAmbientColor : packoffset(c9);
  float scene_GlobalTexcoordFactor : packoffset(c9.w);
  float4 scene_FogRangeParameters : packoffset(c10);
  float3 scene_FogColor : packoffset(c11);
  float3 scene_FakeRimLightDir : packoffset(c12);
  float4 scene_MiscParameters2 : packoffset(c13);
  float scene_AdditionalShadowOffset : packoffset(c14);
  float4 scene_cameraNearFarParameters : packoffset(c15);
  float4 FilterColor : packoffset(c16) = {1,1,1,1};
  float4 FadingColor : packoffset(c17) = {1,1,1,1};
  float4 MonotoneMul : packoffset(c18) = {1,1,1,1};
  float4 MonotoneAdd : packoffset(c19) = {0,0,0,0};
  float4 GlowIntensity : packoffset(c20) = {1,1,1,1};
  float4 ToneFactor : packoffset(c21) = {1,1,1,1};
  float4 UvScaleBias : packoffset(c22) = {1,1,0,0};
  float4 GaussianBlurParams : packoffset(c23) = {0,0,0,0};
  float4 DofParams : packoffset(c24) = {0,0,0,0};
  float4 GammaParameters : packoffset(c25) = {1,1,1,0};
  float4 WhirlPinchParams : packoffset(c26) = {0,0,0,0};
  float4 UVWarpParams : packoffset(c27) = {0,0,0,0};
  float4 MotionBlurParams : packoffset(c28) = {0,0,0,0};
  float GlobalTexcoordFactor : packoffset(c29);
}

SamplerState LinearClampSampler_s : register(s0);
Texture2D<float4> ColorBuffer : register(t0);
Texture2D<float4> GlareBuffer : register(t1);
Texture2D<float4> FadingTexture : register(t2);


// 3Dmigoto declarations
#define cmp -

float3 ApplySmartFadeBlendSrgb(float3 sceneSrgb, float3 fadeSrgb, float fadeAmount)
{
  float3 sceneLinear = renodx::color::srgb::DecodeSafe(sceneSrgb);
  float3 blendedLinear = _MultiplyBlendSrgb(sceneLinear, fadeSrgb, fadeAmount);
  return renodx::color::srgb::EncodeSafe(blendedLinear);
}


void main(
  float4 v0 : SV_POSITION0,
  float2 v1 : TEXCOORD0,
  float2 w1 : TEXCOORD1,
  out float4 o0 : SV_TARGET0)
{
  float3 glare = GlareBuffer.SampleLevel(LinearClampSampler_s, v1.xy, 0).rgb;
  float3 color = ColorBuffer.SampleLevel(LinearClampSampler_s, v1.xy, 0).rgb;
  float4 fadeTex = FadingTexture.SampleLevel(LinearClampSampler_s, v1.xy, 0);

  // Match vanilla math exactly in SDR/gamma space.
  float3 scene = color * ToneFactor.xxx + glare * GlowIntensity.w;
  if (shader_injection.debug_disable_fading == 0.f) {
    float fadeMask = FadingColor.w * fadeTex.a;
    float3 fadeTarget = fadeTex.rgb * FadingColor.rgb;
    scene = ApplySmartFadeBlendSrgb(scene, fadeTarget, fadeMask);
  }

  if (shader_injection.tone_map_type == 0.f) {
    o0.rgb = scene;
  } else {
    // Preserve SDR transition behavior, then apply HDR tonemapping.
    float3 linearScene = renodx::color::srgb::DecodeSafe(scene);
    o0.rgb = _Tonemap(linearScene);
    o0.rgb = renodx::draw::RenderIntermediatePass(o0.rgb);
  }
  o0.a = 1.0;
}
