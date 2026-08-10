// ---- Created with 3Dmigoto v1.3.16 on Tue Jul 15 19:37:33 2025
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
  bool PhyreMaterialSwitches : packoffset(c48.y);
  float4x4 World : packoffset(c49);
  float4x4 WorldViewProjection : packoffset(c53);
  float4x4 WorldViewProjectionPrev : packoffset(c57);
  float GlobalTexcoordFactor : packoffset(c61);

  struct
  {
    float3 m_direction;
    float3 m_colorIntensity;
  } Light0 : packoffset(c62);

  float GameMaterialID : packoffset(c63.w) = {0};
  float4 GameMaterialDiffuse : packoffset(c64) = {1,1,1,1};
  float4 GameMaterialEmission : packoffset(c65) = {0,0,0,0};
  float GameMaterialMonotone : packoffset(c66) = {0};
  float4 GameMaterialTexcoord : packoffset(c67) = {0,0,1,1};
  float4 GameDitherParams : packoffset(c68) = {0,0,0,0};
  float4 UVaMUvColor : packoffset(c69) = {1,1,1,1};
  float4 UVaProjTexcoord : packoffset(c70) = {0,0,1,1};
  float4 UVaMUvTexcoord : packoffset(c71) = {0,0,1,1};
  float4 UVaMUv2Texcoord : packoffset(c72) = {0,0,1,1};
  float4 UVaDuDvTexcoord : packoffset(c73) = {0,0,1,1};
  float AlphaThreshold : packoffset(c74) = {0.5};
  float2 WindyGrassDirection : packoffset(c74.y) = {0,0};
  float WindyGrassSpeed : packoffset(c74.w) = {0.100000001};
  float WindyGrassHomogenity : packoffset(c75) = {2};
  float WindyGrassScale : packoffset(c75.y) = {1};
  float BloomIntensity : packoffset(c75.z) = {1};
  float MaskEps : packoffset(c75.w);
  float4 PointLightParams : packoffset(c76) = {0,2,1,1};
  float4 PointLightColor : packoffset(c77) = {1,0,0,0};
}

SamplerState LinearWrapSamplerState_s : register(s0);
SamplerState PointWrapSamplerState_s : register(s1);
SamplerState DiffuseMapSamplerSampler_s : register(s2);
Texture2D<float4> DitherNoiseTexture : register(t0);
Texture2D<float4> LowResDepthTexture : register(t1);
Texture2D<float4> DiffuseMapSampler : register(t2);


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_POSITION0,
  float4 v1 : COLOR0,
  float4 v2 : COLOR1,
  float4 v3 : TEXCOORD0,
  float4 v4 : TEXCOORD1,
  float4 v5 : TEXCOORD4,
  float4 v6 : TEXCOORD9,
  uint v7 : SV_SampleIndex0,
  out float4 o0 : SV_TARGET0)
{
  const float4 icb[] = { { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0.250000, 0.250000, 0, 0},
                              { -0.250000, -0.250000, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { -0.125000, -0.375000, 0, 0},
                              { 0.375000, -0.125000, 0, 0},
                              { -0.375000, 0.125000, 0, 0},
                              { 0.125000, 0.375000, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0.062500, -0.187500, 0, 0},
                              { -0.062500, 0.187500, 0, 0},
                              { 0.312500, 0.062500, 0, 0},
                              { -0.187500, -0.312500, 0, 0},
                              { -0.312500, 0.312500, 0, 0},
                              { -0.437500, -0.062500, 0, 0},
                              { 0.187500, 0.437500, 0, 0},
                              { 0.437500, -0.437500, 0, 0} };
  float4 r0,r1,r2;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.xy = float2(0.25,0.25) * v0.xy;
  r0.x = DitherNoiseTexture.SampleLevel(PointWrapSamplerState_s, r0.xy, 0).x;
  r0.x = v6.x + -r0.x;
  r0.x = cmp(r0.x < 0);
  if (r0.x != 0) discard;
  r0.xyz = -scene.UserClipPlane2.xyz + v4.xyz;
  r0.w = dot(r0.xyz, r0.xyz);
  r0.w = rsqrt(r0.w);
  r0.xyz = r0.xyz * r0.www;
  r0.x = dot(scene.UserClipPlane.xyz, r0.xyz);
  r0.x = cmp(r0.x < 0);
  if (r0.x != 0) discard;
  r0.x = (int)scene.DuranteSettings.x & 1;
  if (r0.x != 0) {
    r0.xy = ddx(v3.xy);
    r0.zw = ddy(v3.xy);
    r1.x = (uint)scene.DuranteSettings.y << 3;
    r1.x = (int)r1.x + (int)v7.x;
    r0.zw = icb[r1.x+0].yy * r0.zw;
    r0.xy = icb[r1.x+0].xx * r0.xy + r0.zw;
    r0.xy = v3.xy + r0.xy;
  } else {
    r0.xy = v3.xy;
  }
  r0.xyzw = DiffuseMapSampler.Sample(DiffuseMapSamplerSampler_s, r0.xy).xyzw;
  r1.x = AlphaThreshold * v1.w;
  r0.w = r0.w * v1.w + -r1.x;
  r0.w = cmp(r0.w < 0);
  if (r0.w != 0) discard;
  r0.w = dot(v5.xyz, v5.xyz);
  r0.w = rsqrt(r0.w);
  r1.xyz = v5.xyz * r0.www;
  r2.xyz = scene.EyePosition.xyz + -v4.xyz;
  r0.w = dot(r2.xyz, r2.xyz);
  r0.w = rsqrt(r0.w);
  r2.xyz = r2.xyz * r0.www;
  r0.w = dot(r1.xyz, r2.xyz);
  r1.x = dot(Light0.m_direction.xyz, r1.xyz);
  r1.x = r1.x * 0.5 + 0.5;
  r1.x = r1.x * r1.x;
  r1.xyz = Light0.m_colorIntensity.xyz * r1.xxx + scene.GlobalAmbientColor.xyz;
  // r1.xyz = min(float3(1.5,1.5,1.5), r1.xyz);
  r1.xyz = v1.xyz * r1.xyz;
  r0.xyz = r1.xyz * r0.xyz;
  r0.w = 1 + -abs(r0.w);
  r0.w = max(0, r0.w);
  // r0.w = log2(r0.w);
  // r0.w = PointLightColor.x * r0.w;
  // r0.w = exp2(r0.w);
  r0.w = renodx::math::SafePow(r0.w, PointLightColor.x);
  r0.w = -1 + r0.w;
  r0.w = PointLightColor.y * r0.w + 1;
  r1.xyz = GameMaterialEmission.xyz * r0.www;
  r0.xyz = r0.xyz * GameMaterialDiffuse.xyz + r1.xyz;
  r0.w = cmp(0 < scene.MiscParameters6.w);
  if (r0.w != 0) {
    r1.xy = GlobalTexcoordFactor * scene.MiscParameters6.xy;
    r1.xz = r1.xy * float2(30,30) + v4.xz;
    r1.y = v4.y;
    r1.xyz = scene.MiscParameters6.zzz * r1.xyz;
    r2.x = LowResDepthTexture.SampleLevel(LinearWrapSamplerState_s, r1.xy, 0).x;
    r2.y = LowResDepthTexture.SampleLevel(LinearWrapSamplerState_s, r1.xz, 0).x;
    r2.z = LowResDepthTexture.SampleLevel(LinearWrapSamplerState_s, r1.yz, 0).x;
    r0.w = dot(r2.xyz, float3(0.333299994,0.333299994,0.333299994));
    r0.w = v2.w * r0.w;
    r0.w = -r0.w * scene.MiscParameters6.w + v2.w;
    r0.w = max(0, r0.w);
  } else {
    r0.w = v2.w;
  }
  r1.xyz = scene.FogColor.xyz + -r0.xyz;
  r0.xyz = r0.www * r1.xyz + r0.xyz;
  r0.w = -r0.w * 0.5 + 1;
  r0.w = r0.w * r0.w;
  r0.w = PointLightParams.z * r0.w;
  r1.x = calculateLuminanceSRGB(r0.xyz);
  r1.xyz = r1.xxx * scene.MonotoneMul.xyz + scene.MonotoneAdd.xyz;
  r1.xyz = r1.xyz + -r0.xyz;
  r0.xyz = GameMaterialMonotone * r1.xyz + r0.xyz;
  r1.xyz = BloomIntensity * r0.xyz;
  r1.x = calculateLuminanceSRGB(r1.xyz);
  r1.x = -scene.MiscParameters2.z + r1.x;
  r1.x = max(0, r1.x);
  r1.x = 0.5 * r1.x;
  r1.x = min(1, r1.x);
  o0.w = r1.x * r0.w;
  o0.xyz = r0.xyz;

  
  return;
}