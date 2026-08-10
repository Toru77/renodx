// ---- Created with 3Dmigoto v1.3.16 on Tue Sep 16 02:23:46 2025
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
  float3 RimLitColor : packoffset(c74.y) = {1,1,1};
  float RimLitIntensity : packoffset(c75) = {4};
  float RimLitPower : packoffset(c75.y) = {2};
  float RimLightClampFactor : packoffset(c75.z) = {2};
  float BlendMulScale3 : packoffset(c75.w) = {1};
  float2 DuDvMapImageSize : packoffset(c76) = {256,256};
  float2 DuDvScale : packoffset(c76.z) = {4,4};
  float2 WindyGrassDirection : packoffset(c77) = {0,0};
  float WindyGrassSpeed : packoffset(c77.z) = {0.100000001};
  float WindyGrassHomogenity : packoffset(c77.w) = {2};
  float WindyGrassScale : packoffset(c78) = {1};
  float BloomIntensity : packoffset(c78.y) = {1};
  float MaskEps : packoffset(c78.z);
  float4 PointLightParams : packoffset(c79) = {0,2,1,1};
  float4 PointLightColor : packoffset(c80) = {1,0,0,0};
}

SamplerState LinearClampSamplerState_s : register(s0);
SamplerState DiffuseMapSamplerSampler_s : register(s1);
SamplerState DiffuseMap2SamplerSampler_s : register(s2);
SamplerState DiffuseMap3SamplerSampler_s : register(s3);
SamplerState DuDvMapSamplerSampler_s : register(s4);
Texture2D<float4> DiffuseMapSampler : register(t0);
Texture2D<float4> DiffuseMap2Sampler : register(t1);
Texture2D<float4> DiffuseMap3Sampler : register(t2);
Texture2D<float4> DuDvMapSampler : register(t3);
Texture2D<float4> RefractionTexture : register(t4);


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
  float4 r0,r1,r2,r3;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.xy = DuDvMapSampler.Sample(DuDvMapSamplerSampler_s, v6.xy).xy;
  r0.xy = r0.xy * float2(2,2) + float2(-1,-1);
  r0.zw = DuDvScale.xy / DuDvMapImageSize.xy;

  float4 coord = float4(v3.x, v3.y, w3.x, w3.y);
  r1.xyzw = r0.xyxy * r0.zwzw + coord;
  r2.xyzw = DiffuseMapSampler.Sample(DiffuseMapSamplerSampler_s, r1.xy).xyzw;
  r1.xyzw = DiffuseMap2Sampler.Sample(DiffuseMap2SamplerSampler_s, r1.zw).xyzw;
  r3.x = r2.w * v1.w + -0.00400000019;
  r3.x = cmp(r3.x < 0);
  if (r3.x != 0) discard;
  r3.xy = r0.xy * r0.zw + w6.xy;
  r0.xy = r0.xy * r0.zw;
  r3.xyzw = DiffuseMap3Sampler.Sample(DiffuseMap3SamplerSampler_s, r3.xy).xyzw;
  r0.z = UVaMUvColor.w * r1.w;
  r1.xyz = r1.xyz * UVaMUvColor.xyz + -r2.xyz;
  r0.z = v1.w * r0.z;
  r1.xyz = r0.zzz * r1.xyz + r2.xyz;
  r0.z = v1.w * r2.w;
  r2.xyz = r1.xyz * r3.xyz;
  r0.w = v1.w * r3.w;
  r2.xyz = r2.xyz * BlendMulScale3 + -r1.xyz;
  r1.xyz = r0.www * r2.xyz + r1.xyz;
  r2.xy = scene.ViewportWidthHeight.xy / DuDvMapImageSize.xy;
  r2.zw = v0.xy / scene.ViewportWidthHeight.xy;
  r3.xy = v0.ww * r2.zw;
  r0.xy = r0.xy * r2.xy + r3.xy;
  r2.xy = r2.zw * v0.ww + -r0.xy;
  r0.xy = scene.AdditionalShadowOffset * r2.xy + r0.xy;
  r0.xy = r0.xy / v0.ww;
  r0.xyw = RefractionTexture.Sample(LinearClampSamplerState_s, r0.xy).xyz;
  r1.xyz = r1.xyz + -r0.xyw;
  r2.xyz = scene.EyePosition.xyz + -v4.xyz;
  r1.w = dot(r2.xyz, r2.xyz);
  r1.w = rsqrt(r1.w);
  r2.xyz = r2.xyz * r1.www;
  r1.w = dot(v5.xyz, v5.xyz);
  r1.w = rsqrt(r1.w);
  r3.xyz = v5.xyz * r1.www;
  r1.w = saturate(dot(r3.xyz, r2.xyz));
  r1.w = 1 + -r1.w;
  r1.w = log2(r1.w);
  r2.x = RimLitPower * r1.w;
  r1.w = PointLightColor.x * r1.w;
  r1.w = exp2(r1.w);
  r1.w = -1 + r1.w;
  r1.w = PointLightColor.y * r1.w + 1;
  r2.x = exp2(r2.x);
  r2.x = -r2.x * RimLitIntensity + 1;
  r2.w = r2.x * r0.z;
  r0.xyz = r2.www * r1.xyz + r0.xyw;
  r0.w = max(Light0.m_colorIntensity.x, Light0.m_colorIntensity.y);
  r1.x = max(0.00100000005, Light0.m_colorIntensity.z);
  r0.w = max(r1.x, r0.w);
  r1.xyz = Light0.m_colorIntensity.xyz / r0.www;
  r1.xyz = min(float3(1.5,1.5,1.5), r1.xyz);
  r1.xyz = v1.xyz * r1.xyz;
  r2.xyz = r1.xyz * r0.xyz;
  r0.xyzw = GameMaterialDiffuse.xyzw * r2.xyzw;
  r0.xyz = GameMaterialEmission.xyz * r1.www + r0.xyz;
  o0.w = r0.w;
  // r0.w = dot(r0.xyz, float3(0.298999995,0.587000012,0.114));
  r0.w = calculateLuminanceSRGB(r0.rgb);
  r1.xyz = r0.www * scene.MonotoneMul.xyz + scene.MonotoneAdd.xyz;
  r1.xyz = r1.xyz + -r0.xyz;
  o0.xyz = GameMaterialMonotone * r1.xyz + r0.xyz;

  o0.w = max(o0.w, 0.f);
  return;
}