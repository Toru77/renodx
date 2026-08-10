// ---- Created with 3Dmigoto v1.3.16 on Wed Sep 03 19:18:02 2025
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
  float SphereMapIntensity : packoffset(c76.w) = {1};
  float2 DuDvMapImageSize : packoffset(c77) = {256,256};
  float2 DuDvScale : packoffset(c77.z) = {4,4};
  float BloomIntensity : packoffset(c78) = {0.699999988};
  float4 GameEdgeParameters : packoffset(c79) = {1,1,1,0.00300000003};
  float MaskEps : packoffset(c80);
  float4 PointLightParams : packoffset(c81) = {0,2,1,1};
  float4 PointLightColor : packoffset(c82) = {1,0,0,0};
}

SamplerState LinearWrapSamplerState_s : register(s0);
SamplerState LinearClampSamplerState_s : register(s1);
SamplerState DiffuseMapSamplerSampler_s : register(s2);
SamplerState NormalMapSamplerSampler_s : register(s3);
SamplerState DiffuseMap2SamplerSampler_s : register(s4);
SamplerState DiffuseMap3SamplerSampler_s : register(s5);
SamplerState DuDvMapSamplerSampler_s : register(s6);
Texture2D<float4> LowResDepthTexture : register(t0);
Texture2D<float4> DiffuseMapSampler : register(t1);
Texture2D<float4> NormalMapSampler : register(t2);
Texture2D<float4> DiffuseMap2Sampler : register(t3);
Texture2D<float4> DiffuseMap3Sampler : register(t4);
Texture2D<float4> CartoonMapSampler : register(t5);
Texture2D<float4> SphereMapSampler : register(t6);
Texture2D<float4> DuDvMapSampler : register(t7);
Texture2D<float4> RefractionTexture : register(t8);


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
  float3 v7 : TEXCOORD6,
  out float4 o0 : SV_TARGET0)
{
  float4 r0,r1,r2,r3,r4,r5;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.xy = DuDvMapSampler.Sample(DuDvMapSamplerSampler_s, v6.xy).xy;
  r0.xy = r0.xy * float2(2,2) + float2(-1,-1);
  r0.zw = DuDvScale.xy / DuDvMapImageSize.xy;
  r1.xy = r0.xy * r0.zw;

  float4 coord = float4(v3.x, v3.y, w3.x, w3.y);
  r2.xyzw = r0.xyxy * r0.zwzw + coord.xyzw;
  r3.xyzw = DiffuseMapSampler.Sample(DiffuseMapSamplerSampler_s, r2.xy).xyzw;
  r4.w = v1.w * r3.w;
  r1.z = r3.w * v1.w + -0.00400000019;
  r1.z = cmp(r1.z < 0);
  if (r1.z != 0) discard;
  r1.zw = v0.xy / scene.ViewportWidthHeight.xy;
  r5.xy = v0.ww * r1.zw;
  r5.zw = scene.ViewportWidthHeight.xy / DuDvMapImageSize.xy;
  r1.xy = r1.xy * r5.zw + r5.xy;
  r5.xyzw = DiffuseMap2Sampler.Sample(DiffuseMap2SamplerSampler_s, r2.zw).xyzw;
  r5.xyzw = UVaMUvColor.xyzw * r5.xyzw;
  r4.xyz = r3.xyz;
  r3.xyzw = r5.xyzw * r4.xyzw;
  r0.xy = r0.xy * r0.zw + w6.xy;
  r0.xyzw = DiffuseMap3Sampler.Sample(DiffuseMap3SamplerSampler_s, r0.xy).xyzw;
  r0.w = v1.w * r0.w;
  r0.xyz = r0.xyz * r0.www + r3.xyz;
  r2.yzw = NormalMapSampler.Sample(NormalMapSamplerSampler_s, r2.xy).xyz;
  r2.yzw = r2.yzw * float3(2,2,2) + float3(-1,-1,-1);
  r0.w = dot(v7.xyz, v7.xyz);
  r0.w = rsqrt(r0.w);
  r3.xyz = v7.xyz * r0.www;
  r0.w = dot(v5.xyz, v5.xyz);
  r0.w = rsqrt(r0.w);
  r4.xyz = v5.xyz * r0.www;
  r5.xyz = r4.zxy * r3.yzx;
  r5.xyz = r4.yzx * r3.zxy + -r5.xyz;
  r0.w = cmp(r2.x < 0);
  r0.w = r0.w ? -1 : 1;
  r0.w = r2.y * r0.w;
  r2.xyz = r5.xyz * r2.zzz;
  r2.xyz = r0.www * r3.xyz + r2.xyz;
  r2.xyz = r2.www * r4.xyz + r2.xyz;
  r0.w = dot(r2.xyz, r2.xyz);
  r0.w = rsqrt(r0.w);
  r2.xyz = r2.xyz * r0.www;
  r3.xyz = scene.EyePosition.xyz + -v4.xyz;
  r0.w = dot(r3.xyz, r3.xyz);
  r0.w = rsqrt(r0.w);
  r3.xyz = r3.xyz * r0.www;
  r0.w = saturate(dot(r2.xyz, r3.xyz));
  r3.x = dot(r2.xyz, scene.View._m00_m10_m20);
  r3.y = dot(r2.xyz, scene.View._m01_m11_m21);
  r3.xy = r3.xy * float2(0.5,0.5) + float2(0.5,0.5);
  r3.xyz = SphereMapSampler.Sample(LinearClampSamplerState_s, r3.xy).xyz;
  r2.x = dot(LightDirForChar.xyz, r2.xyz);
  r2.x = r2.x * 0.5 + 0.5;
  r2.y = 0;
  r2.x = CartoonMapSampler.SampleLevel(LinearClampSamplerState_s, r2.xy, 0).x;
  r2.xyz = Light0.m_colorIntensity.xyz * r2.xxx + scene.GlobalAmbientColor.xyz;
  r2.xyz = min(float3(1.5,1.5,1.5), r2.xyz);
  r0.w = 1 + -r0.w;
  r0.w = log2(r0.w);
  r2.w = RimLitPower * r0.w;
  r2.w = exp2(r2.w);
  r2.w = -r2.w * RimLitIntensity + 1;
  r4.w = r3.w * r2.w;
  r1.zw = r1.zw * v0.ww + -r1.xy;
  r1.xy = scene.AdditionalShadowOffset * r1.zw + r1.xy;
  r1.xy = r1.xy / v0.ww;
  r1.xyz = RefractionTexture.Sample(LinearClampSamplerState_s, r1.xy).xyz;
  r0.xyz = -r1.xyz + r0.xyz;
  r0.xyz = r4.www * r0.xyz + r1.xyz;
  r1.xyz = Light0.m_colorIntensity.xyz + Light0.m_colorIntensity.xyz;
  r1.xyz = max(float3(1,1,1), r1.xyz);
  r5.xyz = min(float3(1,1,1), r2.xyz);
  r5.xyz = float3(1,1,1) + -r5.xyz;
  r5.xyz = ShadowColorShift.xyz * r5.xyz;
  r1.xyz = r5.xyz * r1.xyz + r2.xyz;
  r2.xyz = SphereMapIntensity * r3.xyz;
  r0.xyz = r2.xyz * v1.xyz + r0.xyz;
  r1.xyz = v1.xyz * r1.xyz;
  r4.xyz = r1.xyz * r0.xyz;
  r1.xyzw = GameMaterialDiffuse.xyzw * r4.xyzw;
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
  r0.w = calculateLuminanceSRGB(r0.w);
  r1.xyz = r0.www * scene.MonotoneMul.xyz + scene.MonotoneAdd.xyz;
  r1.xyz = r1.xyz + -r0.xyz;
  o0.xyz = GameMaterialMonotone * r1.xyz + r0.xyz;
  o0.w = r1.w;

  o0.w = max(o0.w, 0.f);
  return;
}