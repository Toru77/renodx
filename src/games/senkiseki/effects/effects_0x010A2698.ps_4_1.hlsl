// ---- Created with 3Dmigoto v1.3.16 on Fri Jun 06 22:18:00 2025
#include "../shared.h"
#include "../cs4/common.hlsl"
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

  bool PhyreContextSwitches : packoffset(c48);
  float4x4 World : packoffset(c49);
  float4x4 WorldView : packoffset(c53);
  float4x4 WorldViewProjection : packoffset(c57);
  float4x4 WorldViewInverse : packoffset(c61);
  float4x4 WorldViewProjectionInverse : packoffset(c65);
  float4x4 WorldInverse : packoffset(c69);
  float CameraAspectRatio : packoffset(c73);
  float4 inputColor : packoffset(c74) = {1,1,1,1};
  float4 inputSpecular : packoffset(c75) = {0,0,0,0};
  float inputAlphaThreshold : packoffset(c76) = {0};
  float4 inputCenter : packoffset(c77) = {0,0,0,0};
  float4 inputUVShift : packoffset(c78) = {1,1,0,0};
  float4 inputUVShift1 : packoffset(c79) = {1,1,0,0};
  float4 inputUVShift2 : packoffset(c80) = {1,1,0,0};
  float2 inputUVtraspose : packoffset(c81) = {1,0};
  float2 inputUVtraspose1 : packoffset(c81.z) = {1,0};
  float2 inputUVtraspose2 : packoffset(c82) = {1,0};
  float4 inputShaderParam : packoffset(c83) = {0,0,0,0};
  float2 inputDUDVParam1 : packoffset(c84) = {1,1};
  float2 inputScreenOffset : packoffset(c84.z) = {0,0};
  float inputDepth : packoffset(c85) = {0};
  float2 inputSoftCheckDepthParam : packoffset(c85.y) = {10,0};
  float3 inputNearFadeClip : packoffset(c86) = {0,0,1};
  float inputDownscaleFactor : packoffset(c86.w) = {1};
  float2 inputDepthBufferSize : packoffset(c87) = {960,544};
  float2 inputDistortion : packoffset(c87.z) = {0,0};
  float3 inputBlurParam : packoffset(c88) = {0,0,0};
  float4 inputMonotoneMul : packoffset(c89) = {1,1,1,1};
  float4 inputMonotoneAdd : packoffset(c90) = {0,0,0,0};
  float4 inputInvProjXY : packoffset(c91) = {0,0,0,0};
  float4 GameDitherParams : packoffset(c92) = {0,0,0,0};
}

SamplerState VariableSamplerState_s : register(s0);
SamplerState VariableSamplerState1_s : register(s1);
Texture2D<float4> TextureSampler : register(t0);
Texture2D<float4> Texture1Sampler : register(t1);


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_POSITION0,
  float4 v1 : COLOR0,
  float4 v2 : COLOR1,
  float2 v3 : TEXCOORD0,
  float2 w3 : TEXCOORD1,
  float2 v4 : TEXCOORD2,
  out float4 o0 : SV_TARGET0)
{
  float4 r0,r1;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.xy = Texture1Sampler.Sample(VariableSamplerState1_s, w3.xy).xy;
  r0.xy = r0.xy * float2(2,2) + float2(-1,-1);
  r0.zw = float2(1,1) / scene.ViewportWidthHeight.xy;
  r1.x = v0.x * r0.z;
  r1.z = -v0.y * r0.w + 1;
  r0.xy = r0.xy * inputDUDVParam1.xy + r1.xz;
  r0.xyzw = TextureSampler.Sample(VariableSamplerState_s, r0.xy).xyzw;
  r1.x = r0.w * v1.w + -inputAlphaThreshold;
  r0.xyzw = v1.xyzw * r0.xyzw;
  r1.x = cmp(r1.x < 0);
  if (r1.x != 0) discard;
  o0.xyz = v2.xyz * v2.www + r0.xyz;
  o0.w = r0.w;

  o0 = max(o0, 0.f);
  return;
}