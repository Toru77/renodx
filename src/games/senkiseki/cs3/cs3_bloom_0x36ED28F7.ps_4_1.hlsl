// ---- Created with 3Dmigoto v1.3.16 on Sat Jun 07 06:17:25 2025
#include "../shared.h"
#include "../cs4/common.hlsl"
cbuffer _Globals : register(b0)
{
  uint4 DuranteSettings : packoffset(c0);

  struct
  {
    float3 EyePosition;
    float4x4 View;
    float4x4 Projection;
    float4x4 ViewProjection;
    float4x4 ViewInverse;
    float4x4 ProjectionInverse;
    float2 cameraNearFar;
    float cameraNearTimesFar;
    float cameraFarMinusNear;
    float cameraFarMinusNearInv;
    float2 ViewportWidthHeight;
    float2 screenWidthHeightInv;
    float3 GlobalAmbientColor;
    float Time;
    float3 FakeRimLightDir;
    float3 FogColor;
    float4 FogRangeParameters;
    float3 MiscParameters1;
    float4 MiscParameters2;
    float3 MonotoneMul;
    float3 MonotoneAdd;
    float3 UserClipPlane2;
    float4 UserClipPlane;
    float4 MiscParameters3;
    float AdditionalShadowOffset;
    float AlphaTestDirection;
    float4 MiscParameters4;
    float3 MiscParameters5;
    float4 light1_attenuation;
    float3 light2_position;
    float3 light2_colorIntensity;
    float4 light2_attenuation;
  } scene : packoffset(c1);

  bool PhyreContextSwitches : packoffset(c43);
  float4 FilterColor : packoffset(c44) = {1,1,1,1};
  float4 FadingColor : packoffset(c45) = {1,1,1,1};
  float4 _MonotoneMul : packoffset(c46) = {1,1,1,1};
  float4 _MonotoneAdd : packoffset(c47) = {0,0,0,0};
  float4 GlowIntensity : packoffset(c48) = {1,1,1,1};
  float4 GodrayParams : packoffset(c49) = {0,0,0,0};
  float4 GodrayParams2 : packoffset(c50) = {0,0,0,0};
  float4 GodrayColor : packoffset(c51) = {0,0,0,1};
  float4 SSRParams : packoffset(c52) = {5,0.300000012,15,1024};
  float4 ToneFactor : packoffset(c53) = {1,1,1,1};
  float4 UvScaleBias : packoffset(c54) = {1,1,0,0};
  float4 GaussianBlurParams : packoffset(c55) = {0,0,0,0};
  float4 DofParams : packoffset(c56) = {0,0,0,0};
  float4 DofParams2 : packoffset(c57) = {0,0,0,0};
  float4 GammaParameters : packoffset(c58) = {1,1,1,0};
  float4 NoiseParams : packoffset(c59) = {0,0,0,0};
  float4 WhirlPinchParams : packoffset(c60) = {0,0,0,0};
  float4 UVWarpParams : packoffset(c61) = {0,0,0,0};
  float4 MotionBlurParams : packoffset(c62) = {0,0,0,0};
  float GlobalTexcoordFactor : packoffset(c63);
}

SamplerState LinearClampSamplerState_s : register(s0);
Texture2D<float4> ColorBuffer : register(t0);


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_POSITION0,
  float2 v1 : TEXCOORD0,
  out float4 o0 : SV_TARGET0)
{
  float4 r0,r1,r2;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.xy = -GaussianBlurParams.xy + v1.xy;
  r0.xyzw = ColorBuffer.SampleLevel(LinearClampSamplerState_s, r0.xy, 0).xyzw;
  r0 = processBloomBuffer(r0);
  r0.xyzw = GaussianBlurParams.wwww * r0.xyzw;
  r1.xyzw = ColorBuffer.SampleLevel(LinearClampSamplerState_s, v1.xy, 0).xyzw;
  r1 = processBloomBuffer(r1);
  r0.xyzw = r1.xyzw * GaussianBlurParams.zzzz + r0.xyzw;
  r1.xyzw = GaussianBlurParams.xyxy * float4(1, -1, -1, 1) + v1.xyxy;
  r2.xyzw = ColorBuffer.SampleLevel(LinearClampSamplerState_s, r1.xy, 0).xyzw;
  r2 = processBloomBuffer(r2);
  r1.xyzw = ColorBuffer.SampleLevel(LinearClampSamplerState_s, r1.zw, 0).xyzw;
  r1 = processBloomBuffer(r1);
  r0.xyzw = r2.xyzw * GaussianBlurParams.wwww + r0.xyzw;
  r0.xyzw = r1.xyzw * GaussianBlurParams.wwww + r0.xyzw;
  r1.xy = GaussianBlurParams.xy + v1.xy;
  r1.xyzw = ColorBuffer.SampleLevel(LinearClampSamplerState_s, r1.xy, 0).xyzw;
  r1 = processBloomBuffer(r1);
  o0.xyzw = r1.xyzw;
  o0.rgb = max(o0.rgb, 0.f);
  // o0.rgb = clamp(o0.rgb, 0.f, shader_injection.safe_clamp);
    
  return;
}