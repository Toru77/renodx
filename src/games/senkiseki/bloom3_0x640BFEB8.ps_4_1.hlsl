// ---- Created with 3Dmigoto v1.3.16 on Fri Jun 06 16:56:24 2025
#include "cs4/common.hlsl"
#include "shared.h"

cbuffer _Globals : register(b0)
{

  struct
  {
    float3 EyePosition;
    float3 EyeDirection;
    float4x4 View;
    float4x4 Projection;
    float4x4 ViewProjection;
    float4x4 ViewInverse;
    float4x4 ProjectionInverse;
    float4x4 ViewProjectionPrev;
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
    float4 MiscParameters6;
    float3 MiscParameters7;
    float3 light2_colorIntensity;
    float4 light2_attenuation;
    uint4 DuranteSettings;
  } scene : packoffset(c0);

  float MaskEps : packoffset(c48);
  bool PhyreContextSwitches : packoffset(c48.y);
  float4 CommonParams : packoffset(c49) = {0.00052083336,0.00092592591,1.77777779,0};
  float4 FilterColor : packoffset(c50) = {1,1,1,1};
  float4 FadingColor : packoffset(c51) = {1,1,1,1};
  float4 _MonotoneMul : packoffset(c52) = {1,1,1,1};
  float4 _MonotoneAdd : packoffset(c53) = {0,0,0,0};
  float4 GlowIntensity : packoffset(c54) = {1,1,1,1};
  float4 GodrayParams : packoffset(c55) = {0,0,0,0};
  float4 GodrayParams2 : packoffset(c56) = {0,0,0,0};
  float4 GodrayColor : packoffset(c57) = {0,0,0,1};
  float4 SSAOParams : packoffset(c58) = {1,0,1,30};
  float4 SSRParams : packoffset(c59) = {5,0.300000012,15,1024};
  float4 ToneFactor : packoffset(c60) = {1,1,1,1};
  float4 UvScaleBias : packoffset(c61) = {1,1,0,0};
  float4 GaussianBlurParams : packoffset(c62) = {0,0,0,0};
  float4 DofParams : packoffset(c63) = {0,0,0,0};
  float4 GammaParameters : packoffset(c64) = {1,1,1,0};
  float4 NoiseParams : packoffset(c65) = {0,0,0,0};
  float4 WhirlPinchParams : packoffset(c66) = {0,0,0,0};
  float4 UVWarpParams : packoffset(c67) = {0,0,0,0};
  float4 MotionBlurParams : packoffset(c68) = {0,0,0,0};
  float GlobalTexcoordFactor : packoffset(c69);
}

SamplerState LinearClampSamplerState_s : register(s0);
Texture2D<float4> ColorBuffer : register(t0);
Texture2D<float4> GlareBuffer : register(t1);


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_POSITION0,
  float2 v1 : TEXCOORD0,
  float2 w1 : TEXCOORD7,
  float4 v2 : TEXCOORD1,
  float4 v3 : TEXCOORD2,
  out float4 o0 : SV_TARGET0)
{
  float4 r0,r1,r2;
  uint4 bitmask, uiDest;
  float4 fDest;

  float gaussian_blur_w = GaussianBlurParams.w;
  float gaussian_blur_z = GaussianBlurParams.z;

  r0.xyzw = ColorBuffer.SampleLevel(LinearClampSamplerState_s, v2.xy, 0).xyzw;
  r0 = processBloomBuffer(r0);
  r0.xyzw = float4(0.100000001,0.100000001,0.100000001,0.100000001) * r0.xyzw;
  r1.xyzw = ColorBuffer.SampleLevel(LinearClampSamplerState_s, v1.xy, 0).xyzw;
  r1 = processBloomBuffer(r1);
  r0.xyzw = r1.xyzw * float4(0.400000006,0.400000006,0.400000006,0.400000006) + r0.xyzw;
  r1.xyzw = ColorBuffer.SampleLevel(LinearClampSamplerState_s, v2.zw, 0).xyzw;
  r1 = processBloomBuffer(r1);
  r0.xyzw = r1.xyzw * float4(0.200000003,0.200000003,0.200000003,0.200000003) + r0.xyzw;
  r1.xyzw = ColorBuffer.SampleLevel(LinearClampSamplerState_s, v3.xy, 0).xyzw;
  r1 = processBloomBuffer(r1);
  r0.xyzw = r1.xyzw * float4(0.200000003,0.200000003,0.200000003,0.200000003) + r0.xyzw;
  r1.xyzw = ColorBuffer.SampleLevel(LinearClampSamplerState_s, v3.zw, 0).xyzw;
  r1 = processBloomBuffer(r1);
  r0.xyzw = r1.xyzw * float4(0.100000001,0.100000001,0.100000001,0.100000001) + r0.xyzw;
  r1.x = gaussian_blur_w * r0.w;
  r2.xyzw = GlareBuffer.SampleLevel(LinearClampSamplerState_s, w1.xy, 0).xyzw;
  r2.xyz = r2.xyz * r1.xxx;

  r0.xyzw = r2.xyzw + r0.xyzw;
  o0.xyzw = gaussian_blur_z * r0.xyzw;

  o0.rgb = max(o0.rgb, 0.f);

  // o0.rgb = clamp(o0.rgb, 0.f, shader_injection.safe_clamp);
  
  
  return;
}