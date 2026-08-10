// ---- Created with 3Dmigoto v1.3.16 on Thu Jul 03 21:38:30 2025
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
  float3 ShadowColorShift : packoffset(c74.y) = {0.100000001,0.0199999996,0.0199999996};
  float Shininess : packoffset(c75) = {0.5};
  float SpecularPower : packoffset(c75.y) = {50};
  float3 RimLitColor : packoffset(c76) = {1,1,1};
  float RimLitIntensity : packoffset(c76.w) = {4};
  float RimLitPower : packoffset(c77) = {2};
  float RimLightClampFactor : packoffset(c77.y) = {2};
  float CubeMapIntensity : packoffset(c77.z) = {1};
  float CubeMapFresnel : packoffset(c77.w) = {0};
  float2 DuDvMapImageSize : packoffset(c78) = {256,256};
  float2 DuDvScale : packoffset(c78.z) = {4,4};
  float BloomIntensity : packoffset(c79) = {1};
  float MaskEps : packoffset(c79.y);
  float4 PointLightParams : packoffset(c80) = {0,2,1,1};
  float4 PointLightColor : packoffset(c81) = {1,0,0,0};
}

SamplerState LinearWrapSamplerState_s : register(s0);
SamplerState LinearClampSamplerState_s : register(s1);
SamplerState DiffuseMapSamplerSampler_s : register(s2);
SamplerState NormalMapSamplerSampler_s : register(s3);
SamplerState DiffuseMap2SamplerSampler_s : register(s4);
SamplerState NormalMap2SamplerSampler_s : register(s5);
SamplerState DuDvMapSamplerSampler_s : register(s6);
Texture2D<float4> LowResDepthTexture : register(t0);
Texture2D<float4> DiffuseMapSampler : register(t1);
Texture2D<float4> NormalMapSampler : register(t2);
Texture2D<float4> DiffuseMap2Sampler : register(t3);
Texture2D<float4> NormalMap2Sampler : register(t4);
TextureCube<float4> CubeMapSampler : register(t5);
Texture2D<float4> DuDvMapSampler : register(t6);
Texture2D<float4> RefractionTexture : register(t7);


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
  r1.z = r2.w * v1.w + -0.00400000019;
  r1.z = cmp(r1.z < 0);
  if (r1.z != 0) discard;
  r1.zw = v0.xy / scene.ViewportWidthHeight.xy;
  r4.xy = v0.ww * r1.zw;
  r4.zw = scene.ViewportWidthHeight.xy / DuDvMapImageSize.xy;
  r1.xy = r1.xy * r4.zw + r4.xy;
  r4.xyzw = DiffuseMap2Sampler.Sample(DiffuseMap2SamplerSampler_s, r0.zw).xyzw;
  r2.w = UVaMUvColor.w * r4.w;
  r2.w = v1.w * r2.w;
  r4.xyz = r4.xyz * UVaMUvColor.xyz + -r2.xyz;
  r2.xyz = r2.www * r4.xyz + r2.xyz;
  r4.xyz = NormalMapSampler.Sample(NormalMapSamplerSampler_s, r0.xy).xyz;
  r4.xyz = r4.xyz * float3(2,2,2) + float3(-1,-1,-1);
  r0.y = dot(v7.xyz, v7.xyz);
  r0.y = rsqrt(r0.y);
  r5.xyz = v7.xyz * r0.yyy;
  r0.y = dot(v5.xyz, v5.xyz);
  r0.y = rsqrt(r0.y);
  r6.xyz = v5.xyz * r0.yyy;
  r7.xyz = r6.zxy * r5.yzx;
  r7.xyz = r6.yzx * r5.zxy + -r7.xyz;
  r0.xy = cmp(r0.xz < float2(0,0));
  r0.xy = r0.xy ? float2(-1,-1) : float2(1,1);
  r0.x = r4.x * r0.x;
  r4.xyw = r7.xyz * r4.yyy;
  r4.xyw = r0.xxx * r5.xyz + r4.xyw;
  r4.xyz = r4.zzz * r6.xyz + r4.xyw;
  r0.x = dot(r4.xyz, r4.xyz);
  r0.x = rsqrt(r0.x);
  r4.xyz = r4.xyz * r0.xxx;
  r0.xzw = NormalMap2Sampler.Sample(NormalMap2SamplerSampler_s, r0.zw).xyz;
  r0.xzw = r0.xzw * float3(2,2,2) + float3(-1,-1,-1);
  r0.x = r0.x * r0.y;
  r7.xyz = r0.zzz * r7.xyz;
  r0.xyz = r0.xxx * r5.xyz + r7.xyz;
  r0.xyz = r0.www * r6.xyz + r0.xyz;
  r0.w = dot(r0.xyz, r0.xyz);
  r0.w = rsqrt(r0.w);
  r0.xyz = r0.xyz * r0.www + -r4.xyz;
  r0.xyz = r2.www * r0.xyz + r4.xyz;
  r0.w = dot(r0.xyz, r0.xyz);
  r0.w = rsqrt(r0.w);
  r0.xyz = r0.xyz * r0.www;
  r4.xyz = scene.EyePosition.xyz + -v4.xyz;
  r2.w = dot(r4.xyz, r4.xyz);
  r2.w = rsqrt(r2.w);
  r5.xyz = r4.xyz * r2.www;
  r0.w = dot(r0.xyz, r5.xyz);
  r4.w = cmp(r0.w < 0);
  r0.xyzw = r4.wwww ? -r0.xyzw : r0.xyzw;
  r4.w = dot(-r5.xyz, r0.xyz);
  r4.w = r4.w + r4.w;
  r5.xyz = r0.xyz * -r4.www + -r5.xyz;
  r5.xyz = CubeMapSampler.Sample(LinearClampSamplerState_s, r5.xyz).xyz;
  r4.w = max(0, r0.w);
  r4.w = -r4.w * CubeMapFresnel + 1;
  r4.w = CubeMapIntensity * r4.w;
  r5.xyz = r5.xyz + -r2.xyz;
  r2.xyz = r4.www * r5.xyz + r2.xyz;
  r4.w = dot(Light0.m_direction.xyz, r0.xyz);
  r4.w = r4.w * 0.5 + 0.5;
  r4.w = r4.w * r4.w;
  r4.xyz = r4.xyz * r2.www + Light0.m_direction.xyz;
  r2.w = dot(r4.xyz, r4.xyz);
  r2.w = rsqrt(r2.w);
  r4.xyz = r4.xyz * r2.www;
  r0.x = saturate(dot(r0.xyz, r4.xyz));
  // r0.x = (dot(r0.xyz, r4.xyz));
  // r0.x = log2(r0.x);
  // r0.x = SpecularPower * r0.x;
  // r0.x = exp2(r0.x);
  r0.x = renodx::math::SafePow(r0.x, SpecularPower);
  r0.x = min(1, r0.x);
  r0.x = Shininess * r0.x;
  r4.xyz = Light0.m_colorIntensity.xyz * r4.www + scene.GlobalAmbientColor.xyz;
  r4.xyz = min(float3(1.5,1.5,1.5), r4.xyz);
  r0.xyz = Light0.m_colorIntensity.xyz * r0.xxx + r4.xyz;
  r0.w = 1 + -abs(r0.w);
  r0.w = max(0, r0.w);
  // r0.w = log2(r0.w);
  // r2.w = RimLitPower * r0.w;
  // r2.w = exp2(r2.w);
  r2.w = renodx::math::SafePow(r0.w, RimLitPower);
  r2.w = RimLitIntensity * r2.w;
  r4.xyz = RimLitColor.xyz * r2.www;
  r0.xyz = r4.xyz * Light0.m_colorIntensity.xyz + r0.xyz;
  r0.xyz = min(RimLightClampFactor, r0.xyz);
  r1.zw = r1.zw * v0.ww + -r1.xy;
  r1.xy = scene.AdditionalShadowOffset * r1.zw + r1.xy;
  r1.xy = r1.xy / v0.ww;
  r1.xyz = RefractionTexture.Sample(LinearClampSamplerState_s, r1.xy).xyz;
  r2.xyz = r2.xyz + -r1.xyz;
  r1.xyz = r3.www * r2.xyz + r1.xyz;
  r2.xyz = Light0.m_colorIntensity.xyz + Light0.m_colorIntensity.xyz;
  r2.xyz = max(float3(1,1,1), r2.xyz);
  r4.xyz = min(float3(1,1,1), r0.xyz);
  r4.xyz = float3(1,1,1) + -r4.xyz;
  r4.xyz = ShadowColorShift.xyz * r4.xyz;
  r0.xyz = r4.xyz * r2.xyz + r0.xyz;
  r0.xyz = v1.xyz * r0.xyz;
  r3.xyz = r1.xyz * r0.xyz;
  r1.xyzw = GameMaterialDiffuse.xyzw * r3.xyzw;
  r0.x = PointLightColor.x * r0.w;
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
  r1.xyz = scene.FogColor.xyz + -r0.xyz;
  r0.xyz = r0.www * r1.xyz + r0.xyz;
  // r0.w = dot(r0.xyz, float3(0.298999995,0.587000012,0.114));
  r0.w = calculateLuminanceSRGB(r0.xyz);
  r1.xyz = r0.www * scene.MonotoneMul.xyz + scene.MonotoneAdd.xyz;
  r1.xyz = r1.xyz + -r0.xyz;
  o0.xyz = GameMaterialMonotone * r1.xyz + r0.xyz;
  o0.w = r1.w;

  // o0 = max(o0, 0.f);
  o0.rgb = clamp(o0.rgb, 0.f, shader_injection.safe_clamp);;
  return;
}