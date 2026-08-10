// ---- Created with 3Dmigoto v1.3.16 on Sat Jun 07 19:28:12 2025
// rising nova
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

  float3 LightDirForChar : packoffset(c64);
  float GameMaterialID : packoffset(c64.w) = {0};
  float4 GameMaterialDiffuse : packoffset(c65) = {1,1,1,1};
  float4 GameMaterialEmission : packoffset(c66) = {0,0,0,0};
  float GameMaterialMonotone : packoffset(c67) = {0};
  float4 GameMaterialTexcoord : packoffset(c68) = {0,0,1,1};
  float4 UVaMUvColor : packoffset(c69) = {1,1,1,1};
  float4 UVaProjTexcoord : packoffset(c70) = {0,0,1,1};
  float4 UVaMUvTexcoord : packoffset(c71) = {0,0,1,1};
  float4 UVaMUv2Texcoord : packoffset(c72) = {0,0,1,1};
  float4 UVaDuDvTexcoord : packoffset(c73) = {0,0,1,1};
  float AlphaThreshold : packoffset(c74) = {0.5};
  float3 ShadowColorShift : packoffset(c74.y) = {0.100000001,0.0199999996,0.0199999996};
  float3 RimLitColor : packoffset(c75) = {1,1,1};
  float RimLitIntensity : packoffset(c75.w) = {4};
  float RimLitPower : packoffset(c76) = {2};
  float RimLightClampFactor : packoffset(c76.y) = {2};
  float ShadowReceiveOffset : packoffset(c76.z) = {0.600000024};
  float2 DuDvMapImageSize : packoffset(c77) = {256,256};
  float2 DuDvScale : packoffset(c77.z) = {4,4};
  float BloomIntensity : packoffset(c78) = {0.699999988};
  float GlareIntensity : packoffset(c78.y) = {1};
  float4 GameEdgeParameters : packoffset(c79) = {1,1,1,0.00300000003};
  float MaskEps : packoffset(c80);
  float4 PointLightParams : packoffset(c81) = {0,2,1,1};
  float4 PointLightColor : packoffset(c82) = {1,0,0,0};
}

SamplerState LinearClampSamplerState_s : register(s0);
SamplerState DiffuseMapSamplerSampler_s : register(s1);
SamplerState DiffuseMap2SamplerSampler_s : register(s2);
SamplerState DiffuseMap3SamplerSampler_s : register(s3);
SamplerState DuDvMapSamplerSampler_s : register(s4);
Texture2D<float4> DiffuseMapSampler : register(t0);
Texture2D<float4> DiffuseMap2Sampler : register(t1);
Texture2D<float4> DiffuseMap3Sampler : register(t2);
Texture2D<float4> CartoonMapSampler : register(t3);
Texture2D<float4> DuDvMapSampler : register(t4);
Texture2D<float4> RefractionTexture : register(t5);


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_POSITION0,
  float4 v1 : COLOR0,
  float4 v2 : COLOR1,
  float2 v3 : TEXCOORD0,
  float2 w3 : TEXCOORD2,
  float4 v4 : TEXCOORD1,
  float4 v5 : TEXCOORD4,
  float2 v6 : TEXCOORD5,
  float2 w6 : TEXCOORD7,
  out float4 o0 : SV_TARGET0)
{
  float4 r0,r1,r2,r3,r4,r5;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.xy = DuDvMapSampler.Sample(DuDvMapSamplerSampler_s, v6.xy).xy;
  r0.xy = r0.xy * float2(2,2) + float2(-1,-1);
  r0.zw = DuDvScale.xy / DuDvMapImageSize.xy;
  r1.xy = r0.xy * r0.zw + v3.xy;
  r1.zw = r0.xy * r0.zw + w3.xy;
  r2.xyzw = DiffuseMapSampler.Sample(DiffuseMapSamplerSampler_s, r1.xy).xyzw;
  r1.xyzw = DiffuseMap2Sampler.Sample(DiffuseMap2SamplerSampler_s, r1.zw).xyzw;
  r1.xyzw = UVaMUvColor.xyzw * r1.xyzw;
  r3.x = r2.w * v1.w + -0.00400000019;
  r3.x = cmp(r3.x < 0);
  if (r3.x != 0) discard;
  r3.xy = r0.xy * r0.zw + w6.xy;
  r0.xy = r0.xy * r0.zw;
  r3.xyzw = DiffuseMap3Sampler.Sample(DiffuseMap3SamplerSampler_s, r3.xy).xyzw;
  r0.z = v1.w * r3.w;
  r0.w = v1.w * r1.w;
  r1.xyz = r1.xyz * r0.www + r2.xyz;
  r0.w = v1.w * r2.w;
  r1.xyz = r3.xyz * r0.zzz + r1.xyz;
  r2.xy = scene.ViewportWidthHeight.xy / DuDvMapImageSize.xy;
  r2.zw = v0.xy / scene.ViewportWidthHeight.xy;
  r3.xy = v0.ww * r2.zw;
  r0.xy = r0.xy * r2.xy + r3.xy;
  r3.xyz = scene.EyePosition.xyz + -v4.xyz;
  r0.z = dot(r3.xyz, r3.xyz);
  r0.z = rsqrt(r0.z);
  r3.xyz = r3.xyz * r0.zzz;
  r0.z = dot(v5.xyz, v5.xyz);
  r0.z = rsqrt(r0.z);
  r4.xyz = v5.xyz * r0.zzz;
  r0.z = dot(-r3.xyz, r4.xyz);
  r0.z = r0.z + r0.z;
  r5.xyz = r4.xyz * -r0.zzz + -r3.xyz;
  r0.z = saturate(dot(r4.xyz, r3.xyz));
  // r0.z = (dot(r4.xyz, r3.xyz));
  r1.w = dot(LightDirForChar.xyz, r4.xyz);
  r2.x = r1.w * 0.5 + 0.5;
  r0.z = 1 + -r0.z;
  float l = r0.z;
  r0.z = log2(r0.z);
  r3.x = dot(r5.xyz, scene.View._m00_m10_m20);
  r3.y = dot(r5.xyz, scene.View._m01_m11_m21);
  r0.xy = r3.xy * scene.AlphaTestDirection + r0.xy;
  r2.zw = r2.zw * v0.ww + -r0.xy;
  r0.xy = scene.AdditionalShadowOffset * r2.zw + r0.xy;
  r0.xy = r0.xy / v0.ww;
  r3.xyz = RefractionTexture.Sample(LinearClampSamplerState_s, r0.xy).xyz;
  r1.xyz = -r3.xyz + r1.xyz;
  // r0.x = RimLitPower * r0.z;
  // r0.y = PointLightColor.x * r0.z;
  // r0.y = exp2(r0.y);
  r0.y = renodx::math::SafePow(l, PointLightColor.x);
  r0.y = -1 + r0.y;
  r0.y = PointLightColor.y * r0.y + 1;
  // r0.x = exp2(r0.x);
  r0.x = renodx::math::SafePow(l, RimLitPower);
  r0.x = -r0.x * RimLitIntensity + 1;
  r4.w = r0.w * r0.x;
  r0.x = PointLightParams.w * r4.w;
  r0.xzw = r0.xxx * r1.xyz + r3.xyz;
  r2.y = 0;
  r1.x = CartoonMapSampler.SampleLevel(LinearClampSamplerState_s, r2.xy, 0).x;
  r1.xyz = Light0.m_colorIntensity.xyz * r1.xxx + scene.GlobalAmbientColor.xyz;
  // r1.xyz = min(float3(1.5,1.5,1.5), r1.xyz);
  r2.xyz = min(float3(1,1,1), r1.xyz);
  r2.xyz = float3(1,1,1) + -r2.xyz;
  r2.xyz = ShadowColorShift.xyz * r2.xyz;
  r3.xyz = Light0.m_colorIntensity.xyz + Light0.m_colorIntensity.xyz;
  r3.xyz = max(float3(1,1,1), r3.xyz);
  r1.xyz = r2.xyz * r3.xyz + r1.xyz;
  r1.xyz = v1.xyz * r1.xyz;
  r2.xyz = r1.xyz * r0.xzw;
  r0.xzw = -r0.xzw * r1.xyz + r0.xzw;
  r1.x = 1 + -PointLightParams.w;
  r1.x = scene.MiscParameters5.z * r1.x;
  r4.xyz = r1.xxx * r0.xzw + r2.xyz;
  r1.xyzw = GameMaterialDiffuse.xyzw * r4.xyzw;
  r0.xyz = GameMaterialEmission.xyz * r0.yyy + r1.xyz;
  o0.w = r1.w;
  // r0.w = dot(r0.xyz, float3(0.298999995,0.587000012,0.114));
  r0.w = calculateLuminanceSRGB(r0.xyz);
  r1.xyz = r0.www * scene.MonotoneMul.xyz + scene.MonotoneAdd.xyz;
  r1.xyz = r1.xyz + -r0.xyz;
  o0.xyz = GameMaterialMonotone * r1.xyz + r0.xyz;

  o0 = max(o0, 0.f);
  // o0.rgb = clamp(o0.rgb, 0.f, RENODX_PEAK_WHITE_NITS / RENODX_DIFFUSE_WHITE_NITS);
  return;
}