// ---- Created with 3Dmigoto v1.3.16 on Wed Sep 03 17:47:08 2025
#include "../cs4/common.hlsl"
// Cedric's craft01 effect

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
  float4 UVaMUvColor : packoffset(c68) = {1,1,1,1};
  float4 UVaProjTexcoord : packoffset(c69) = {0,0,1,1};
  float4 UVaMUvTexcoord : packoffset(c70) = {0,0,1,1};
  float4 UVaMUv2Texcoord : packoffset(c71) = {0,0,1,1};
  float4 UVaDuDvTexcoord : packoffset(c72) = {0,0,1,1};
  float AlphaThreshold : packoffset(c73) = {0.5};
  float3 RimLitColor : packoffset(c73.y) = {1,1,1};
  float RimLitIntensity : packoffset(c74) = {4};
  float RimLitPower : packoffset(c74.y) = {2};
  float RimLightClampFactor : packoffset(c74.z) = {2};
  float BloomIntensity : packoffset(c74.w) = {0.699999988};
  float4 GameEdgeParameters : packoffset(c75) = {1,1,1,0.00300000003};
  float MaskEps : packoffset(c76);
  float4 PointLightParams : packoffset(c77) = {0,2,1,1};
  float4 PointLightColor : packoffset(c78) = {1,0,0,0};
}

SamplerState LinearWrapSamplerState_s : register(s0);
SamplerState LinearClampSamplerState_s : register(s1);
SamplerState DiffuseMapSamplerSampler_s : register(s2);
SamplerState DiffuseMap2SamplerSampler_s : register(s3);
Texture2D<float4> LowResDepthTexture : register(t0);
Texture2D<float4> DiffuseMapSampler : register(t1);
Texture2D<float4> DiffuseMap2Sampler : register(t2);
Texture2D<float4> RefractionTexture : register(t3);


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_POSITION0,
  float4 v1 : COLOR0,
  float4 v2 : COLOR1,
  float2 v3 : TEXCOORD0,
  float2 w3 : TEXCOORD2,
  float4 v4 : TEXCOORD1,
  float3 v5 : TEXCOORD4,
  out float4 o0 : SV_TARGET0)
{
  float4 r0,r1,r2,r3,r4,r5,r6,r7;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.xyzw = DiffuseMapSampler.Sample(DiffuseMapSamplerSampler_s, v3.xy).xyzw;
  r1.x = v1.w * r0.w;
  r0.w = r0.w * v1.w + -0.00400000019;
  r0.w = cmp(r0.w < 0);
  if (r0.w != 0) discard;
  r1.yz = v0.xy / scene.ViewportWidthHeight.xy;
  r2.xy = v0.ww * r1.yz;
  r3.xyzw = DiffuseMap2Sampler.Sample(DiffuseMap2SamplerSampler_s, w3.xy).xyzw;
  r3.xyzw = UVaMUvColor.xyzw * r3.xyzw;
  r0.w = r3.w * r1.x;
  r1.x = dot(v5.xyz, v5.xyz);
  r1.x = rsqrt(r1.x);
  r4.xyz = v5.xyz * r1.xxx;
  r5.xyz = scene.EyePosition.xyz + -v4.xyz;
  r1.x = dot(r5.xyz, r5.xyz);
  r1.x = rsqrt(r1.x);
  r5.xyz = r5.xyz * r1.xxx;
  r1.x = saturate(dot(r4.xyz, r5.xyz));
  r1.w = max(Light0.m_colorIntensity.x, Light0.m_colorIntensity.y);
  r2.z = max(0.00100000005, Light0.m_colorIntensity.z);
  r1.w = max(r2.z, r1.w);
  r6.xyz = Light0.m_colorIntensity.xyz / r1.www;
  r6.xyz = min(float3(1.5,1.5,1.5), r6.xyz);
  r1.x = 1 + -r1.x;
  r1.x = log2(r1.x);
  r1.w = RimLitPower * r1.x;
  r1.w = exp2(r1.w);
  r1.w = -r1.w * RimLitIntensity + 1;
  r7.w = r1.w * r0.w;
  r6.xyz = max(scene.GlobalAmbientColor.xyz, r6.xyz);
  r0.w = dot(-r5.xyz, r4.xyz);
  r0.w = r0.w + r0.w;
  r4.xyz = r4.xyz * -r0.www + -r5.xyz;
  r5.x = dot(r4.xyz, scene.View._m00_m10_m20);
  r5.y = dot(r4.xyz, scene.View._m01_m11_m21);
  r2.xy = r5.xy * scene.AlphaTestDirection + r2.xy;
  r1.yz = r1.yz * v0.ww + -r2.xy;
  r1.yz = scene.AdditionalShadowOffset * r1.yz + r2.xy;
  r1.yz = r1.yz / v0.ww;
  r1.yzw = RefractionTexture.Sample(LinearClampSamplerState_s, r1.yz).xyz;
  r0.w = PointLightParams.w * r7.w;
  r0.xyz = r0.xyz * r3.xyz + -r1.yzw;
  r0.xyz = r0.www * r0.xyz + r1.yzw;
  r1.yzw = v1.xyz * r6.xyz;
  r2.xyz = r1.yzw * r0.xyz;
  r0.w = 1 + -PointLightParams.w;
  r0.w = scene.MiscParameters5.z * r0.w;
  r0.xyz = -r0.xyz * r1.yzw + r0.xyz;
  r7.xyz = r0.www * r0.xyz + r2.xyz;
  r0.xyzw = GameMaterialDiffuse.xyzw * r7.xyzw;
  r1.x = PointLightColor.x * r1.x;
  r1.x = exp2(r1.x);
  r1.x = -1 + r1.x;
  r1.x = PointLightColor.y * r1.x + 1;
  r0.xyz = GameMaterialEmission.xyz * r1.xxx + r0.xyz;
  r1.x = cmp(0 < scene.MiscParameters6.w);
  if (r1.x != 0) {
    r1.xy = GlobalTexcoordFactor * scene.MiscParameters6.xy;
    r1.xz = r1.xy * float2(30,30) + v4.xz;
    r1.y = v4.y;
    r1.xyz = scene.MiscParameters6.zzz * r1.xyz;
    r2.x = LowResDepthTexture.SampleLevel(LinearWrapSamplerState_s, r1.xy, 0).x;
    r2.y = LowResDepthTexture.SampleLevel(LinearWrapSamplerState_s, r1.xz, 0).x;
    r2.z = LowResDepthTexture.SampleLevel(LinearWrapSamplerState_s, r1.yz, 0).x;
    r1.x = dot(r2.xyz, float3(0.333299994,0.333299994,0.333299994));
    r1.x = v2.w * r1.x;
    r1.x = -r1.x * scene.MiscParameters6.w + v2.w;
    r1.x = max(0, r1.x);
  } else {
    r1.x = v2.w;
  }
  r1.yzw = scene.FogColor.xyz + -r0.xyz;
  r0.xyz = r1.xxx * r1.yzw + r0.xyz;
  // r1.x = dot(r0.xyz, float3(0.298999995,0.587000012,0.114));
  r1.x = calculateLuminanceSRGB(r0.rgb);
  r1.xyz = r1.xxx * scene.MonotoneMul.xyz + scene.MonotoneAdd.xyz;
  r1.xyz = r1.xyz + -r0.xyz;
  o0.xyz = GameMaterialMonotone * r1.xyz + r0.xyz;
  o0.w = r0.w;

  o0.w = max(o0.w, 0.f);
  return;
}