// ---- Created with 3Dmigoto v1.3.16 on Wed Jul 02 06:47:15 2025
#include "../shared.h"
#include "./common.hlsl"
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
  float Shininess : packoffset(c74.y) = {0.5};
  float SpecularPower : packoffset(c74.z) = {50};
  float3 RimLitColor : packoffset(c75) = {1,1,1};
  float RimLitIntensity : packoffset(c75.w) = {4};
  float RimLitPower : packoffset(c76) = {2};
  float RimLightClampFactor : packoffset(c76.y) = {2};
  float2 WindyGrassDirection : packoffset(c76.z) = {0,0};
  float WindyGrassSpeed : packoffset(c77) = {0.100000001};
  float WindyGrassHomogenity : packoffset(c77.y) = {2};
  float WindyGrassScale : packoffset(c77.z) = {1};
  float BloomIntensity : packoffset(c77.w) = {1};
  float MaskEps : packoffset(c78);
  float4 PointLightParams : packoffset(c79) = {0,2,1,1};
  float4 PointLightColor : packoffset(c80) = {1,0,0,0};
}

SamplerState LinearWrapSamplerState_s : register(s0);
SamplerState PointWrapSamplerState_s : register(s1);
SamplerState DiffuseMapSamplerSampler_s : register(s2);
SamplerState NormalMapSamplerSampler_s : register(s3);
Texture2D<float4> DitherNoiseTexture : register(t0);
Texture2D<float4> LowResDepthTexture : register(t1);
Texture2D<float4> DiffuseMapSampler : register(t2);
Texture2D<float4> NormalMapSampler : register(t3);


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_POSITION0,
  float4 v1 : COLOR0,
  float4 v2 : COLOR1,
  float4 v3 : TEXCOORD0,
  float4 v4 : TEXCOORD1,
  float4 v5 : TEXCOORD4,
  float4 v6 : TEXCOORD6,
  float4 v7 : TEXCOORD9,
  float4 v8 : TEXCOORD10,
  uint v9 : SV_SampleIndex0,
  out float4 o0 : SV_TARGET0,
  out float4 o1 : SV_TARGET1,
  out float4 o2 : SV_TARGET2)
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
  float4 r0,r1,r2,r3,r4;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.xy = float2(0.25,0.25) * v0.xy;
  r0.x = DitherNoiseTexture.SampleLevel(PointWrapSamplerState_s, r0.xy, 0).x;
  r0.x = v7.x + -r0.x;
  r0.x = cmp(r0.x < 0);
  if (r0.x != 0) discard;
  r0.x = (int)scene.DuranteSettings.x & 1;
  if (r0.x != 0) {
    r0.xy = ddx(v3.xy);
    r0.zw = ddy(v3.xy);
    r1.x = (uint)scene.DuranteSettings.y << 3;
    r1.x = (int)r1.x + (int)v9.x;
    r0.zw = icb[r1.x+0].yy * r0.zw;
    r0.xy = icb[r1.x+0].xx * r0.xy + r0.zw;
    r0.xy = v3.xy + r0.xy;
  } else {
    r0.xy = v3.xy;
  }
  r1.xyzw = DiffuseMapSampler.Sample(DiffuseMapSamplerSampler_s, r0.xy).xyzw;
  r0.z = AlphaThreshold * v1.w;
  r0.z = r1.w * v1.w + -r0.z;
  r0.z = cmp(r0.z < 0);
  if (r0.z != 0) discard;
  r0.yzw = NormalMapSampler.Sample(NormalMapSamplerSampler_s, r0.xy).xyz;
  r0.yzw = r0.yzw * float3(2,2,2) + float3(-1,-1,-1);
  r1.w = dot(v6.xyz, v6.xyz);
  r1.w = rsqrt(r1.w);
  r2.xyz = v6.xyz * r1.www;
  r1.w = dot(v5.xyz, v5.xyz);
  r1.w = rsqrt(r1.w);
  r3.xyz = v5.xyz * r1.www;
  r4.xyz = r3.zxy * r2.yzx;
  r4.xyz = r3.yzx * r2.zxy + -r4.xyz;
  r0.x = cmp(r0.x < 0);
  r0.x = r0.x ? -1 : 1;
  r0.x = r0.y * r0.x;
  r4.xyz = r4.xyz * r0.zzz;
  r0.xyz = r0.xxx * r2.xyz + r4.xyz;
  r0.xyz = r0.www * r3.xyz + r0.xyz;
  r0.w = dot(r0.xyz, r0.xyz);
  r0.w = rsqrt(r0.w);
  r0.xyz = r0.xyz * r0.www;
  r2.xyz = scene.EyePosition.xyz + -v4.xyz;
  r0.w = dot(r2.xyz, r2.xyz);
  r0.w = rsqrt(r0.w);
  r3.xyz = r2.xyz * r0.www;
  r1.w = saturate(dot(r0.xyz, r3.xyz));
  // r1.w = (dot(r0.xyz, r3.xyz));
  r2.w = dot(Light0.m_direction.xyz, r0.xyz);
  r2.w = r2.w * 0.5 + 0.5;
  r2.w = r2.w * r2.w;
  r2.xyz = r2.xyz * r0.www + scene.FakeRimLightDir.xyz;
  r0.w = dot(r2.xyz, r2.xyz);
  r0.w = rsqrt(r0.w);
  r2.xyz = r2.xyz * r0.www;
  r0.w = saturate(dot(r0.xyz, r2.xyz));
  // r0.w = (dot(r0.xyz, r2.xyz));
  // r0.w = log2(r0.w);
  // r0.w = SpecularPower * r0.w;
  // r0.w = exp2(r0.w);
  r0.w = renodx::math::SafePow(r0.w, SpecularPower);
  r0.w = min(1, r0.w);
  r0.w = Shininess * r0.w;
  r2.xyz = Light0.m_colorIntensity.xyz * r2.www + scene.GlobalAmbientColor.xyz;
  r2.xyz = min(float3(1.5,1.5,1.5), r2.xyz);
  r2.xyz = Light0.m_colorIntensity.xyz * r0.www + r2.xyz;
  r0.w = 1 + -r1.w;
  // r0.w = log2(r0.w);
  // r1.w = RimLitPower * r0.w;
  // r1.w = exp2(r1.w);
  r1.w = renodx::math::SafePow(r0.w, RimLitPower);
  r1.w = RimLitIntensity * r1.w;
  r3.xyz = RimLitColor.xyz * r1.www;
  r2.xyz = r3.xyz * Light0.m_colorIntensity.xyz + r2.xyz;
  r2.xyz = min(RimLightClampFactor, r2.xyz);
  r2.xyz = v1.xyz * r2.xyz;
  r1.xyz = r2.xyz * r1.xyz;
  r0.w = PointLightColor.x * r0.w;
  r0.w = exp2(r0.w);
  r0.w = -1 + r0.w;
  r0.w = PointLightColor.y * r0.w + 1;
  r2.xyz = GameMaterialEmission.xyz * r0.www;
  r1.xyz = r1.xyz * GameMaterialDiffuse.xyz + r2.xyz;
  r0.w = cmp(0 < scene.MiscParameters6.w);
  if (r0.w != 0) {
    r2.xy = GlobalTexcoordFactor * scene.MiscParameters6.xy;
    r2.xz = r2.xy * float2(30,30) + v4.xz;
    r2.y = v4.y;
    r2.xyz = scene.MiscParameters6.zzz * r2.xyz;
    r3.x = LowResDepthTexture.SampleLevel(LinearWrapSamplerState_s, r2.xy, 0).x;
    r3.y = LowResDepthTexture.SampleLevel(LinearWrapSamplerState_s, r2.xz, 0).x;
    r3.z = LowResDepthTexture.SampleLevel(LinearWrapSamplerState_s, r2.yz, 0).x;
    r0.w = dot(r3.xyz, float3(0.333299994,0.333299994,0.333299994));
    r0.w = v2.w * r0.w;
    r0.w = -r0.w * scene.MiscParameters6.w + v2.w;
    r0.w = max(0, r0.w);
  } else {
    r0.w = v2.w;
  }
  r2.xyz = scene.FogColor.xyz + -r1.xyz;
  r1.xyz = r0.www * r2.xyz + r1.xyz;
  r0.w = -r0.w * 0.5 + 1;
  r0.w = r0.w * r0.w;
  r0.w = PointLightParams.z * r0.w;
  // r1.w = dot(r1.xyz, float3(0.298999995,0.587000012,0.114));
  r1.w = calculateLuminanceSRGB(r1.xyz);
  r2.xyz = r1.www * scene.MonotoneMul.xyz + scene.MonotoneAdd.xyz;
  r2.xyz = r2.xyz + -r1.xyz;
  r1.xyz = GameMaterialMonotone * r2.xyz + r1.xyz;
  r2.xyz = BloomIntensity * r1.xyz;
  // r1.w = dot(r2.xyz, float3(0.298999995,0.587000012,0.114));
  r1.w = calculateLuminanceSRGB(r2.xyz);
  r1.w = -scene.MiscParameters2.z + r1.w;
  r1.w = max(0, r1.w);
  r1.w = 0.5 * r1.w;
  r1.w = min(1, r1.w);
  o0.w = r1.w * r0.w;
  o1.xyz = r0.xyz * float3(0.5,0.5,0.5) + float3(0.5,0.5,0.5);
  o1.w = 0.466666698 + MaskEps;
  r0.x = v8.z / v8.w;
  r0.y = 256 * r0.x;
  r2.x = trunc(r0.y);
  r0.x = r0.x * 256 + -r2.x;
  r0.y = 256 * r0.x;
  r2.y = trunc(r0.y);
  r2.z = r0.x * 256 + -r2.y;
  o2.xyz = float3(0.00390625,0.00390625,1) * r2.xyz;
  o0.xyz = r1.xyz;
  o2.w = MaskEps;
  return;
}