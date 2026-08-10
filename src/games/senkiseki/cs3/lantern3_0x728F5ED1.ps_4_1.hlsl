// ---- Created with 3Dmigoto v1.3.16 on Mon Aug 25 23:57:06 2025
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
  bool PhyreMaterialSwitches : packoffset(c43.y);
  float4x4 World : packoffset(c44);
  float GlobalTexcoordFactor : packoffset(c48);
  float GameMaterialID : packoffset(c48.y) = {0};
  float4 GameMaterialDiffuse : packoffset(c49) = {1,1,1,1};
  float4 GameMaterialEmission : packoffset(c50) = {0,0,0,0};
  float GameMaterialMonotone : packoffset(c51) = {0};
  float4 GameMaterialTexcoord : packoffset(c52) = {0,0,1,1};
  float4 GameDitherParams : packoffset(c53) = {0,0,0,0};
  float4 UVaMUvColor : packoffset(c54) = {1,1,1,1};
  float4 UVaProjTexcoord : packoffset(c55) = {0,0,1,1};
  float4 UVaMUvTexcoord : packoffset(c56) = {0,0,1,1};
  float4 UVaMUv2Texcoord : packoffset(c57) = {0,0,1,1};
  float4 UVaDuDvTexcoord : packoffset(c58) = {0,0,1,1};
  float AlphaThreshold : packoffset(c59) = {0.5};
  float BloomIntensity : packoffset(c59.y) = {1};
  float4 PointLightParams : packoffset(c60) = {0,0,0,0};
  float4 PointLightColor : packoffset(c61) = {0,0,0,0};
}

SamplerState DiffuseMapSamplerSampler_s : register(s0);
Texture2D<float4> DiffuseMapSampler : register(t0);


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_POSITION0,
  float4 v1 : COLOR0,
  float4 v2 : COLOR1,
  float4 v3 : TEXCOORD0,
  float4 v4 : TEXCOORD1,
  float3 v5 : TEXCOORD4,
  out float4 o0 : SV_TARGET0)
{
  float4 r0,r1;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.xyzw = DiffuseMapSampler.Sample(DiffuseMapSamplerSampler_s, v3.xy).xyzw;
  r1.x = r0.w * v1.w + -0.00400000019;
  r1.x = cmp(r1.x < 0);
  if (r1.x != 0) discard;
  r1.xyz = max(float3(1,1,1), scene.GlobalAmbientColor.xyz);
  // r1.xyz = min(float3(1.5,1.5,1.5), r1.xyz);
  r1.xyz = v1.xyz * r1.xyz;
  r0.xyz = r1.xyz * r0.xyz;
  r0.w = v1.w * r0.w;
  r0.w = GameMaterialDiffuse.w * r0.w;
  o0.w = r0.w;
  r0.xyz = r0.xyz * GameMaterialDiffuse.xyz + GameMaterialEmission.xyz;
  r0.xyz = v2.xyz + r0.xyz;
  // r0.w = dot(r0.xyz, float3(0.298999995,0.587000012,0.114));
  r0.w = calculateLuminanceSRGB(r0.rgb);
  r1.xyz = r0.www * scene.MonotoneMul.xyz + scene.MonotoneAdd.xyz;
  // r1.xyz = r1.xyz + -r0.xyz;
  // o0.xyz = GameMaterialMonotone * r1.xyz + r0.xyz;
  // o0.rgb = GameMaterialMonotone * (r1.rgb - r0.rgb) + r0.rgb;
  // o0.rgb = (1 - GameMaterialMonotone) * r0.rgb + GameMaterialMonotone * r1.rgb;
  o0.rgb = lerp(r0.rgb, r1.rgb, GameMaterialMonotone);
  return;
}