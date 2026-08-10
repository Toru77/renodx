// ---- Created with 3Dmigoto v1.3.16 on Sun Jul 27 15:25:36 2025
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
  float Shininess : packoffset(c74.y) = {0.5};
  float SpecularPower : packoffset(c74.z) = {50};
  float2 DuDvMapImageSize : packoffset(c75) = {256,256};
  float2 DuDvScale : packoffset(c75.z) = {4,4};
  float BloomIntensity : packoffset(c76) = {1};
  float MaskEps : packoffset(c76.y);
  float4 PointLightParams : packoffset(c77) = {0,2,1,1};
  float4 PointLightColor : packoffset(c78) = {1,0,0,0};
}

SamplerState LinearWrapSamplerState_s : register(s0);
SamplerState LinearClampSamplerState_s : register(s1);
SamplerState DiffuseMapSamplerSampler_s : register(s2);
SamplerState NormalMapSamplerSampler_s : register(s3);
SamplerState SpecularMapSamplerSampler_s : register(s4);
SamplerState DuDvMapSamplerSampler_s : register(s5);
Texture2D<float4> LowResDepthTexture : register(t0);
Texture2D<float4> DiffuseMapSampler : register(t1);
Texture2D<float4> NormalMapSampler : register(t2);
Texture2D<float4> SpecularMapSampler : register(t3);
Texture2D<float4> DuDvMapSampler : register(t4);
Texture2D<float4> RefractionTexture : register(t5);


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_POSITION0,
  float4 v1 : COLOR0,
  float4 v2 : COLOR1,
  float2 v3 : TEXCOORD0,
  float2 w3 : TEXCOORD5,
  float4 v4 : TEXCOORD1,
  float4 v5 : TEXCOORD4,
  float3 v6 : TEXCOORD6,
  out float4 o0 : SV_TARGET0)
{
  float4 r0,r1,r2,r3,r4,r5,r6,r7;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.xy = DuDvMapSampler.Sample(DuDvMapSamplerSampler_s, w3.xy).xy;
  r0.xy = r0.xy * float2(2,2) + float2(-1,-1);
  r0.zw = DuDvScale.xy / DuDvMapImageSize.xy;
  r1.xy = r0.xy * r0.zw;
  r0.xy = r0.xy * r0.zw + v3.xy;
  r2.xyzw = DiffuseMapSampler.Sample(DiffuseMapSamplerSampler_s, r0.xy).xyzw;
  r3.w = v1.w * r2.w;
  r0.z = r2.w * v1.w + -0.00400000019;
  r0.z = cmp(r0.z < 0);
  if (r0.z != 0) discard;
  r0.zw = v0.xy / scene.ViewportWidthHeight.xy;
  r1.zw = v0.ww * r0.zw;
  r4.xy = scene.ViewportWidthHeight.xy / DuDvMapImageSize.xy;
  r1.xy = r1.xy * r4.xy + r1.zw;
  r1.z = SpecularMapSampler.Sample(SpecularMapSamplerSampler_s, v3.xy).x;
  r4.xyz = NormalMapSampler.Sample(NormalMapSamplerSampler_s, r0.xy).xyz;
  r4.xyz = r4.xyz * float3(2,2,2) + float3(-1,-1,-1);
  r0.y = dot(v6.xyz, v6.xyz);
  r0.y = rsqrt(r0.y);
  r5.xyz = v6.xyz * r0.yyy;
  r0.y = dot(v5.xyz, v5.xyz);
  r0.y = rsqrt(r0.y);
  r6.xyz = v5.xyz * r0.yyy;
  r7.xyz = r6.zxy * r5.yzx;
  r7.xyz = r6.yzx * r5.zxy + -r7.xyz;
  r0.x = cmp(r0.x < 0);
  r0.x = r0.x ? -1 : 1;
  r0.x = r4.x * r0.x;
  r4.xyw = r7.xyz * r4.yyy;
  r4.xyw = r0.xxx * r5.xyz + r4.xyw;
  r4.xyz = r4.zzz * r6.xyz + r4.xyw;
  r0.x = dot(r4.xyz, r4.xyz);
  r0.x = rsqrt(r0.x);
  r4.xyz = r4.xyz * r0.xxx;
  r5.xyz = scene.EyePosition.xyz + -v4.xyz;
  r0.x = dot(r5.xyz, r5.xyz);
  r0.x = rsqrt(r0.x);
  r6.xyz = r5.xyz * r0.xxx;
  r0.y = saturate(dot(r4.xyz, r6.xyz));
  r1.z = Shininess * r1.z;
  r1.w = dot(Light0.m_direction.xyz, r4.xyz);
  r1.w = r1.w * 0.5 + 0.5;
  r1.w = r1.w * r1.w;
  r5.xyz = r5.xyz * r0.xxx + scene.FakeRimLightDir.xyz;
  r0.x = dot(r5.xyz, r5.xyz);
  r0.x = rsqrt(r0.x);
  r5.xyz = r5.xyz * r0.xxx;
  r0.x = saturate(dot(r4.xyz, r5.xyz));
  r0.x = log2(r0.x);
  r0.x = SpecularPower * r0.x;
  r0.x = exp2(r0.x);
  r0.x = min(1, r0.x);
  r0.x = r0.x * r1.z;
  r4.xyz = Light0.m_colorIntensity.xyz * r1.www + scene.GlobalAmbientColor.xyz;
  r4.xyz = min(float3(1.5,1.5,1.5), r4.xyz);
  r4.xyz = Light0.m_colorIntensity.xyz * r0.xxx + r4.xyz;
  r0.xz = r0.zw * v0.ww + -r1.xy;
  r0.xz = scene.AdditionalShadowOffset * r0.xz + r1.xy;
  r0.xz = r0.xz / v0.ww;
  r0.xzw = RefractionTexture.Sample(LinearClampSamplerState_s, r0.xz).xyz;
  r1.xyz = r2.xyz + -r0.xzw;
  r0.xzw = r3.www * r1.xyz + r0.xzw;
  r1.xyz = v1.xyz * r4.xyz;
  r3.xyz = r1.xyz * r0.xzw;
  r1.xyzw = GameMaterialDiffuse.xyzw * r3.xyzw;
  r0.x = 1 + -r0.y;
  // r0.x = log2(r0.x);
  // r0.x = PointLightColor.x * r0.x;
  // r0.x = exp2(r0.x);
  r0.x = renodx::math::SafePow(r0.x, PointLightColor.x);
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
  r1.xyz = scene.FogColor.xyz + -r0.xyz;
  r0.xyz = r0.www * r1.xyz + r0.xyz;
  r0.w = calculateLuminanceSRGB(r0.rgb);
  r1.xyz = r0.www * scene.MonotoneMul.xyz + scene.MonotoneAdd.xyz;
  r1.xyz = r1.xyz + -r0.xyz;
  o0.xyz = GameMaterialMonotone * r1.xyz + r0.xyz;
  o0.w = r1.w;

  o0.rgb = clamp(o0.rgb, 0.f, shader_injection.safe_clamp);;
  return;
}