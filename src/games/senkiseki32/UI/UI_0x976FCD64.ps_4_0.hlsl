// ---- Created with 3Dmigoto v1.4.1 on Thu Feb 12 12:18:39 2026
#include "../shared.h"

cbuffer _Globals : register(b0)
{
  float3 scene_EyePosition : packoffset(c0);
  float4x4 scene_View : packoffset(c1);
  float4x4 scene_ViewProjection : packoffset(c5);
  float3 scene_GlobalAmbientColor : packoffset(c9);
  float scene_GlobalTexcoordFactor : packoffset(c9.w);
  float3 scene_FakeRimLightDir : packoffset(c10);
  float4 scene_MiscParameters2 : packoffset(c11);
  float scene_AdditionalShadowOffset : packoffset(c12);
  float4 scene_cameraNearFarParameters : packoffset(c13);
  float4x4 World : packoffset(c14);
  float4x4 WorldViewProjection : packoffset(c18);
  float4x4 WorldViewInverse : packoffset(c22);
  float CameraAspectRatio : packoffset(c26);
  float4 inputColor : packoffset(c27) = {1,1,1,1};
  float4 inputSpecular : packoffset(c28) = {0,0,0,0};
  float4 inputUVShift : packoffset(c29) = {1,1,0,0};
  float inputAlphaThreshold : packoffset(c30) = {0};
  float4 inputCenter : packoffset(c31) = {0,0,0,0};
  float2 inputUVtraspose : packoffset(c32) = {1,0};
  float4 inputShaderParam : packoffset(c33) = {0,0,0,0};
  float2 inputScreenOffset : packoffset(c34) = {0,0};
  float inputDepth : packoffset(c34.z) = {0};
  float4 inputUVShiftMT : packoffset(c35) = {1,1,0,0};
  float3 inputNearFadeClip : packoffset(c36) = {0,0,1};
  float2 inputCameraNearFarParams : packoffset(c37) = {512,1.00010002};
  float4 MonotoneMul : packoffset(c38) = {1,1,1,1};
  float4 MonotoneAdd : packoffset(c39) = {0,0,0,0};
}

SamplerState LinearClampSampler_s : register(s0);
Texture2D<float4> TextureSampler : register(t0);


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_POSITION0,
  float4 v1 : COLOR0,
  float2 v2 : TEXCOORD0,
  out float4 o0 : SV_TARGET0)
{
  float4 r0,r1;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.xyzw = TextureSampler.Sample(LinearClampSampler_s, v2.xy).xyzw;
  r1.x = r0.w * v1.w + -inputAlphaThreshold;
  r0.xyzw = v1.xyzw * r0.xyzw;
  r1.x = cmp(r1.x < 0);
  if (r1.x != 0) discard;
  o0.xyz = inputSpecular.xyz * inputSpecular.www + r0.xyz;
  o0.w = r0.w;
  o0 = saturate(o0);
  return;
}