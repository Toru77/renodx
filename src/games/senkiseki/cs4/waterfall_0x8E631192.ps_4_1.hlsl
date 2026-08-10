// ---- Created with 3Dmigoto v1.3.16 on Thu Jul 03 01:20:28 2025
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
  float FogRatio : packoffset(c76.z) = {1};
  float2 DuDvMapImageSize : packoffset(c77) = {256,256};
  float2 DuDvScale : packoffset(c77.z) = {4,4};
  float BloomIntensity : packoffset(c78) = {1};
  float MaskEps : packoffset(c78.y);
  float4 PointLightParams : packoffset(c79) = {0,2,1,1};
  float4 PointLightColor : packoffset(c80) = {1,0,0,0};
}

SamplerState LinearWrapSamplerState_s : register(s0);
SamplerState LinearClampSamplerState_s : register(s1);
SamplerState DiffuseMapSamplerSampler_s : register(s2);
SamplerState DiffuseMap2SamplerSampler_s : register(s3);
SamplerState NormalMap2SamplerSampler_s : register(s4);
SamplerState DuDvMapSamplerSampler_s : register(s5);
Texture2D<float4> LowResDepthTexture : register(t0);
Texture2D<float4> DiffuseMapSampler : register(t1);
Texture2D<float4> DiffuseMap2Sampler : register(t2);
Texture2D<float4> NormalMap2Sampler : register(t3);
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
  float4 v6 : TEXCOORD5,
  float3 v7 : TEXCOORD6,
  out float4 o0 : SV_TARGET0)
{
  float4 r0,r1,r2,r3,r4,r5,r6,r7;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.xy = DuDvMapSampler.Sample(DuDvMapSamplerSampler_s, v6.xy).xy;
  r0.xy = r0.xy * float2(2,2) + float2(-1,-1);
  r0.zw = DuDvScale.xy / DuDvMapImageSize.xy;
  r1.xy = r0.xy * r0.zw;
  r0.xyzw = r0.xyxy * r0.zwzw + float4(v3.x, v3.y, w3.x, w3.y);
  r2.xyzw = DiffuseMapSampler.Sample(DiffuseMapSamplerSampler_s, r0.xy).xyzw;
  r3.w = v1.w * r2.w;
  r0.x = r2.w * v1.w + -0.00400000019;
  r0.x = cmp(r0.x < 0);
  if (r0.x != 0) discard;
  r0.xy = v0.xy / scene.ViewportWidthHeight.xy;
  r1.zw = v0.ww * r0.xy;
  r4.xy = scene.ViewportWidthHeight.xy / DuDvMapImageSize.xy;
  r1.xy = r1.xy * r4.xy + r1.zw;
  r4.xyzw = DiffuseMap2Sampler.Sample(DiffuseMap2SamplerSampler_s, r0.zw).xyzw;
  r4.xyzw = UVaMUvColor.xyzw * r4.xyzw;
  r1.z = v1.w * r4.w;
  r2.xyz = r4.xyz * r1.zzz + r2.xyz;
  r1.w = dot(v5.xyz, v5.xyz);
  r1.w = rsqrt(r1.w);
  r4.xyz = v5.xyz * r1.www;
  r5.xyz = NormalMap2Sampler.Sample(NormalMap2SamplerSampler_s, r0.zw).xyz;
  r5.xyz = r5.xyz * float3(2,2,2) + float3(-1,-1,-1);
  r0.w = dot(v7.xyz, v7.xyz);
  r0.w = rsqrt(r0.w);
  r6.xyz = v7.xyz * r0.www;
  r7.xyz = r6.yzx * r4.zxy;
  r7.xyz = r4.yzx * r6.zxy + -r7.xyz;
  r0.z = cmp(r0.z < 0);
  r0.z = r0.z ? -1 : 1;
  r0.z = r5.x * r0.z;
  r5.xyw = r7.xyz * r5.yyy;
  r5.xyw = r0.zzz * r6.xyz + r5.xyw;
  r5.xyz = r5.zzz * r4.xyz + r5.xyw;
  r0.z = dot(r5.xyz, r5.xyz);
  r0.z = rsqrt(r0.z);
  r5.xyz = r5.xyz * r0.zzz + -r4.xyz;
  r4.xyz = r1.zzz * r5.xyz + r4.xyz;
  r0.z = dot(r4.xyz, r4.xyz);
  r0.z = rsqrt(r0.z);
  r4.xyz = r4.xyz * r0.zzz;
  r5.xyz = scene.EyePosition.xyz + -v4.xyz;
  r0.z = dot(r5.xyz, r5.xyz);
  r0.z = rsqrt(r0.z);
  r6.xyz = r5.xyz * r0.zzz;
  r0.w = saturate(dot(r4.xyz, r6.xyz));
  // r0.w = (dot(r4.xyz, r6.xyz));
  r1.z = dot(Light0.m_direction.xyz, r4.xyz);
  r1.z = r1.z * 0.5 + 0.5;
  r1.z = r1.z * r1.z;
  r7.xyz = Light0.m_colorIntensity.xyz * r1.zzz;
  r5.xyz = r5.xyz * r0.zzz + Light0.m_direction.xyz;
  r0.z = dot(r5.xyz, r5.xyz);
  r0.z = rsqrt(r0.z);
  r5.xyz = r5.xyz * r0.zzz;
  r0.z = saturate(dot(r4.xyz, r5.xyz));
  // r0.z = (dot(r4.xyz, r5.xyz));
  // r0.z = log2(r0.z);
  // r0.z = SpecularPower * r0.z;
  // r0.z = exp2(r0.z);
  r0.z = renodx::math::SafePow(r0.z, SpecularPower);
  r0.z = min(1, r0.z);
  r0.z = Shininess * r0.z;
  r5.xyz = min(float3(1.5,1.5,1.5), r7.xyz);
  r5.xyz = Light0.m_colorIntensity.xyz * r0.zzz + r5.xyz;
  r0.z = 1 + -r0.w;
  // r0.z = log2(r0.z);
  // r0.w = RimLitPower * r0.z;
  // r0.w = exp2(r0.w);
  r0.w = renodx::math::SafePow(r0.w, RimLitPower);
  r0.w = RimLitIntensity * r0.w;
  r7.xyz = RimLitColor.xyz * r0.www;
  r5.xyz = r7.xyz * Light0.m_colorIntensity.xyz + r5.xyz;
  r5.xyz = min(RimLightClampFactor, r5.xyz);
  r0.w = dot(-r6.xyz, r4.xyz);
  r0.w = r0.w + r0.w;
  r4.xyz = r4.xyz * -r0.www + -r6.xyz;
  r6.x = dot(r4.xyz, scene.View._m00_m10_m20);
  r6.y = dot(r4.xyz, scene.View._m01_m11_m21);
  r1.xy = r6.xy * scene.AlphaTestDirection + r1.xy;
  r0.xy = r0.xy * v0.ww + -r1.xy;
  r0.xy = scene.AdditionalShadowOffset * r0.xy + r1.xy;
  r0.xy = r0.xy / v0.ww;
  r0.xyw = RefractionTexture.Sample(LinearClampSamplerState_s, r0.xy).xyz;
  r1.x = PointLightParams.w * r3.w;
  r1.yzw = r2.xyz + -r0.xyw;
  r0.xyw = r1.xxx * r1.yzw + r0.xyw;
  r1.xyz = v1.xyz * r5.xyz;
  r2.xyz = r1.xyz * r0.xyw;
  r1.w = 1 + -PointLightParams.w;
  r1.w = scene.MiscParameters5.z * r1.w;
  r0.xyw = -r0.xyw * r1.xyz + r0.xyw;
  r3.xyz = r1.www * r0.xyw + r2.xyz;
  r1.xyzw = GameMaterialDiffuse.xyzw * r3.xyzw;
  r0.x = PointLightColor.x * r0.z;
  r0.x = exp2(r0.x);
  r0.x = -1 + r0.x;
  r0.x = PointLightColor.y * r0.x + 1;
  r0.xyz = GameMaterialEmission.xyz * r0.xxx + r1.xyz;
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
  r0.xyz = r0.www * -r0.xyz + r0.xyz;
  // r0.w = dot(r0.xyz, float3(0.298999995,0.587000012,0.114));
  r0.w = calculateLuminanceSRGB(r0.xyz);
  r1.xyz = r0.www * scene.MonotoneMul.xyz + scene.MonotoneAdd.xyz;
  r1.xyz = r1.xyz + -r0.xyz;
  o0.xyz = GameMaterialMonotone * r1.xyz + r0.xyz;
  o0.w = r1.w;

  o0 = max(o0, 0.f);
  return;
}