// 0x328E747D - output: color buffer passthrough (gamma disabled)
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

// 3Dmigoto declarations
#define cmp -

void main(
  float4 v0 : SV_POSITION0,
  float2 v1 : TEXCOORD0,
  out float4 o0 : SV_TARGET0)
{
  float4 r0;
  float4 r1 = ColorBuffer.SampleLevel(LinearClampSampler_s, v1.xy, 0);

  if (shader_injection.tone_map_type == 0.f) {
    // Vanilla: apply gamma curve
    r0.x = 1.10000002 * GammaParameters.x;
    r1.xyz = saturate(r1.xyz);
    o0.w = r1.w;
    r0.yzw = log2(r1.xyz);
    r0.xyz = r0.xxx * r0.yzw;
    o0.xyz = exp2(r0.xyz);
    return;
  }

  // RenoDX: pass through linear data
  o0 = r1; 
  return;
}