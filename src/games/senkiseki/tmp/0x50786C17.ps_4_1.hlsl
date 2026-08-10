// ---- Created with 3Dmigoto v1.3.16 on Fri Sep 05 02:33:48 2025
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

  struct
  {
    float3 m_direction;
    float3 m_colorIntensity;
  } Light0 : packoffset(c49);


  struct
  {
    float4x4 m_split0Transform;
    float4x4 m_split1Transform;
    float4 m_splitDistances;
  } LightShadow0 : packoffset(c51);

  float PerMaterialMainLightClampFactor : packoffset(c60) = {1.5};
  float3 LightDirForChar : packoffset(c60.y);
  float GameMaterialID : packoffset(c61) = {0};
  float4 GameMaterialDiffuse : packoffset(c62) = {1,1,1,1};
  float4 GameMaterialEmission : packoffset(c63) = {0,0,0,0};
  float GameMaterialMonotone : packoffset(c64) = {0};
  float4 GameMaterialTexcoord : packoffset(c65) = {0,0,1,1};
  float4 UVaMUvColor : packoffset(c66) = {1,1,1,1};
  float4 UVaProjTexcoord : packoffset(c67) = {0,0,1,1};
  float4 UVaMUvTexcoord : packoffset(c68) = {0,0,1,1};
  float4 UVaMUv2Texcoord : packoffset(c69) = {0,0,1,1};
  float4 UVaDuDvTexcoord : packoffset(c70) = {0,0,1,1};
  float AlphaThreshold : packoffset(c71) = {0.5};
  float3 ShadowColorShift : packoffset(c71.y) = {0.100000001,0.0199999996,0.0199999996};
  float3 RimLitColor : packoffset(c72) = {1,1,1};
  float RimLitIntensity : packoffset(c72.w) = {4};
  float RimLitPower : packoffset(c73) = {2};
  float RimLightClampFactor : packoffset(c73.y) = {2};
  float ShadowReceiveOffset : packoffset(c73.z) = {0.600000024};
  float BloomIntensity : packoffset(c73.w) = {0.699999988};
  float4 GameEdgeParameters : packoffset(c74) = {1,1,1,0.00300000003};
  float4 OutlineColorFactor : packoffset(c75) = {1,1,1,1};
  float4 PointLightParams : packoffset(c76) = {0,0,0,0};
  float4 PointLightColor : packoffset(c77) = {0,0,0,0};
}

SamplerState LinearClampSamplerState_s : register(s0);
SamplerState DiffuseMapSamplerSampler_s : register(s1);
SamplerState NormalMapSamplerSampler_s : register(s2);
SamplerState DiffuseMap2SamplerSampler_s : register(s3);
Texture2D<float4> DiffuseMapSampler : register(t0);
Texture2D<float4> NormalMapSampler : register(t1);
Texture2D<float4> DiffuseMap2Sampler : register(t2);
Texture2D<float4> CartoonMapSampler : register(t3);


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
  float3 v6 : TEXCOORD6,
  out float4 o0 : SV_TARGET0)
{
  float4 r0,r1,r2,r3,r4;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.xyzw = DiffuseMapSampler.Sample(DiffuseMapSamplerSampler_s, v3.xy).xyzw;

  r1.x = r0.w * v1.w + -0.00400000019;
  r1.x = cmp(r1.x < 0);
  if (r1.x != 0) discard;

  r1.xyzw = DiffuseMap2Sampler.Sample(DiffuseMap2SamplerSampler_s, w3.xy).xyzw;
  r1.w = UVaMUvColor.w * r1.w;
  r1.xyz = r1.xyz * UVaMUvColor.xyz + -r0.xyz;
  r1.w = v1.w * r1.w;
  r0.xyz = r1.www * r1.xyz + r0.xyz;
  r0.w = v1.w * r0.w;
  r0.w = GameMaterialDiffuse.w * r0.w;
  o0.w = r0.w;
  r0.w = dot(v6.xyz, v6.xyz);
  r0.w = rsqrt(r0.w);
  r1.xyz = v6.xyz * r0.www;
  r0.w = dot(v5.xyz, v5.xyz);
  r0.w = rsqrt(r0.w);
  r2.xyz = v5.xyz * r0.www;
  r3.xyz = r2.zxy * r1.yzx;
  r3.xyz = r2.yzx * r1.zxy + -r3.xyz;
  r4.xyz = NormalMapSampler.Sample(NormalMapSamplerSampler_s, v3.xy).xyz;
  r4.xyz = r4.xyz * float3(2,2,2) + float3(-1,-1,-1);
  r3.xyz = r4.yyy * r3.xyz;
  r0.w = cmp(v3.x < 0);
  r0.w = r0.w ? -r4.x : r4.x;
  r1.xyz = r0.www * r1.xyz + r3.xyz;
  r1.xyz = r4.zzz * r2.xyz + r1.xyz;
  r0.w = dot(r1.xyz, r1.xyz);
  r0.w = rsqrt(r0.w);
  r1.xyz = r1.xyz * r0.www;
  r2.xyz = scene.EyePosition.xyz + -v4.xyz;
  r0.w = dot(r2.xyz, r2.xyz);
  r0.w = rsqrt(r0.w);
  r2.xyz = r2.xyz * r0.www;
  r0.w = saturate(dot(r1.xyz, r2.xyz));
  r1.x = dot(LightDirForChar.xyz, r1.xyz);
  r1.x = r1.x * 0.5 + 0.5;
  r0.w = 1 + -r0.w;
  r0.w = log2(r0.w);
  r0.w = RimLitPower * r0.w;
  r0.w = exp2(r0.w);
  r0.w = RimLitIntensity * r0.w;
  r2.xyz = RimLitColor.xyz * r0.www;
  r1.y = 0;
  r0.w = CartoonMapSampler.SampleLevel(LinearClampSamplerState_s, r1.xy, 0).x;
  r0.w = r0.w * 0.550000012 + 0.550000012;
  r0.w = min(PerMaterialMainLightClampFactor, r0.w);
  r1.xyz = r2.xyz * float3(0.550000012,0.550000012,0.550000012) + r0.www;
  r1.xyz = min(RimLightClampFactor, r1.xyz);
  r2.xyz = min(float3(1,1,1), r1.xyz);
  r2.xyz = float3(1,1,1) + -r2.xyz;
  r2.xyz = ShadowColorShift.xyz * r2.xyz;
  r1.xyz = r2.xyz * float3(1.10000002,1.10000002,1.10000002) + r1.xyz;
  r1.xyz = v1.xyz * r1.xyz;
  r0.xyz = r1.xyz * r0.xyz;
  r0.xyz = r0.xyz * GameMaterialDiffuse.xyz + GameMaterialEmission.xyz;
  r0.xyz = v2.xyz + r0.xyz;
  // r0.w = dot(r0.xyz, float3(0.298999995,0.587000012,0.114));
  r0.w = calculateLuminanceSRGB(r0.rgb);
  r1.xyz = r0.www * scene.MonotoneMul.xyz + scene.MonotoneAdd.xyz;
  r1.xyz = r1.xyz + -r0.xyz;
  o0.xyz = GameMaterialMonotone * r1.xyz + r0.xyz;
  
  return;
}