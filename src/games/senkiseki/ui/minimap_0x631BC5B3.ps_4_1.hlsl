// ---- Created with 3Dmigoto v1.3.16 on Fri Jun 06 16:40:11 2025

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

  float4x4 WorldViewProjection : packoffset(c48);
  float4x4 World : packoffset(c52);
  float3 lookPosition : packoffset(c56) = {0,0,0};
  float2 inputHeight : packoffset(c57) = {3,8};
  float inputColorShiftHeight : packoffset(c57.z) = {50};
  float inputAlphaThreshold : packoffset(c57.w) = {0.00999999978};
}

SamplerState LinearClampSamplerState_s : register(s0);
Texture2D<float4> MinimapTextureSampler : register(t0);


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

  r0.xyzw = MinimapTextureSampler.Sample(LinearClampSamplerState_s, v2.xy).xyzw;
  r1.x = r0.w * v1.w + -inputAlphaThreshold;
  r0.xyzw = v1.xyzw * r0.xyzw;
  o0.xyzw = r0.xyzw;
  r0.x = cmp(r1.x < 0);
  if (r0.x != 0) discard;

  return;
}