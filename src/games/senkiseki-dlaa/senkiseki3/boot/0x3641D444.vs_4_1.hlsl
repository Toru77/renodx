// ---- Created with 3Dmigoto v1.4.1 on Wed Aug  5 17:51:14 2026

#include "../../shared.h"


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
  float PerMaterialMainLightClampFactor : packoffset(c48.y) = {1.5};
  float3 LightDirForChar : packoffset(c49);
  float GameMaterialID : packoffset(c49.w) = {0};
  float4 GameMaterialDiffuse : packoffset(c50) = {1,1,1,1};
  float4 GameMaterialEmission : packoffset(c51) = {0,0,0,0};
  float GameMaterialMonotone : packoffset(c52) = {0};
  float4 GameMaterialTexcoord : packoffset(c53) = {0,0,1,1};
  float4 UVaMUvColor : packoffset(c54) = {1,1,1,1};
  float4 UVaProjTexcoord : packoffset(c55) = {0,0,1,1};
  float4 UVaMUvTexcoord : packoffset(c56) = {0,0,1,1};
  float4 UVaMUv2Texcoord : packoffset(c57) = {0,0,1,1};
  float4 UVaDuDvTexcoord : packoffset(c58) = {0,0,1,1};
  float3 ShadowColorShift : packoffset(c59) = {0.100000001,0.0199999996,0.0199999996};
  float ShadowReceiveOffset : packoffset(c59.w) = {0.600000024};
  float SphereMapIntensity : packoffset(c60) = {1};
  float BloomIntensity : packoffset(c60.y) = {0.699999988};
  float4 GameEdgeParameters : packoffset(c61) = {1,1,1,0.00300000003};
  float4 PointLightParams : packoffset(c62) = {0,0,0,0};
  float4 PointLightColor : packoffset(c63) = {0,0,0,0};
}

StructuredBuffer<float4x4> BoneTransformConstantBuffer : register(t0);


// 3Dmigoto declarations
#define cmp -


void main(
  float3 v0 : POSITION0,
  float3 v1 : NORMAL0,
  float2 v2 : TEXCOORD0,
  uint4 v3 : BLENDINDICES0,
  float4 v4 : BLENDWEIGHTS0,
  float2 v5 : TEXCOORD3,
  out float4 o0 : SV_POSITION0,
  out float4 o1 : COLOR0,
  out float4 o2 : COLOR1,
  out float2 o3 : TEXCOORD0,
  out float2 p3 : TEXCOORD2,
  out float4 o4 : TEXCOORD1,
  out float4 o5 : TEXCOORD4,
  out float4 o6 : TEXCOORD10)
{
  float4 r0,r1,r2,r3,r4;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.xyzw = BoneTransformConstantBuffer[v3.y]._m00_m10_m20_m30;
  r1.xyz = v0.xyz;
  r1.w = 1;
  r0.x = dot(r1.xyzw, r0.xyzw);
  r2.xyzw = BoneTransformConstantBuffer[v3.y]._m01_m11_m21_m31;
  r0.y = dot(r1.xyzw, r2.xyzw);
  r2.xyzw = BoneTransformConstantBuffer[v3.y]._m02_m12_m22_m32;
  r0.z = dot(r1.xyzw, r2.xyzw);
  r0.xyz = v4.yyy * r0.xyz;
  r2.xyzw = BoneTransformConstantBuffer[v3.x]._m00_m10_m20_m30;
  r2.x = dot(r1.xyzw, r2.xyzw);
  r3.xyzw = BoneTransformConstantBuffer[v3.x]._m01_m11_m21_m31;
  r2.y = dot(r1.xyzw, r3.xyzw);
  r3.xyzw = BoneTransformConstantBuffer[v3.x]._m02_m12_m22_m32;
  r2.z = dot(r1.xyzw, r3.xyzw);
  r0.xyz = r2.xyz * v4.xxx + r0.xyz;
  r2.xyzw = BoneTransformConstantBuffer[v3.z]._m00_m10_m20_m30;
  r2.x = dot(r1.xyzw, r2.xyzw);
  r3.xyzw = BoneTransformConstantBuffer[v3.z]._m01_m11_m21_m31;
  r2.y = dot(r1.xyzw, r3.xyzw);
  r3.xyzw = BoneTransformConstantBuffer[v3.z]._m02_m12_m22_m32;
  r2.z = dot(r1.xyzw, r3.xyzw);
  r0.xyz = r2.xyz * v4.zzz + r0.xyz;
  r2.xyzw = BoneTransformConstantBuffer[v3.w]._m00_m10_m20_m30;
  r2.x = dot(r1.xyzw, r2.xyzw);
  r3.xyzw = BoneTransformConstantBuffer[v3.w]._m01_m11_m21_m31;
  r2.y = dot(r1.xyzw, r3.xyzw);
  r3.xyzw = BoneTransformConstantBuffer[v3.w]._m02_m12_m22_m32;
  r2.z = dot(r1.xyzw, r3.xyzw);
  r0.xyz = r2.xyz * v4.www + r0.xyz;
  r0.w = 1;
  r1.x = dot(r0.xyzw, scene.ViewProjection._m00_m10_m20_m30);
  r1.y = dot(r0.xyzw, scene.ViewProjection._m01_m11_m21_m31);
  r1.z = dot(r0.xyzw, scene.ViewProjection._m02_m12_m22_m32);
  r1.w = dot(r0.xyzw, scene.ViewProjection._m03_m13_m23_m33);
  r0.w = dot(r0.xyzw, scene.View._m02_m12_m22_m32);
  // DLAA: rasterization-level camera jitter (SV_Position sub-pixel shift).
  // Jitter offsets come from the addon b13 injection (0 when disabled).
  o0.x = r1.x + DLAA_JITTER_X * r1.w;
  o0.y = r1.y + DLAA_JITTER_Y * r1.w;
  o0.zw = r1.zw;
  o6.xyzw = r1.xyzw;
  o1.xyzw = float4(1,1,1,1);
  r1.xyz = scene.EyePosition.xyz + -r0.xyz;
  r1.w = dot(r1.xyz, r1.xyz);
  r1.w = rsqrt(r1.w);
  r1.xyz = r1.xyz * r1.www;
  r2.xyz = BoneTransformConstantBuffer[v3.y]._m00_m10_m20;
  r2.x = dot(v1.xyz, r2.xyz);
  r3.xyz = BoneTransformConstantBuffer[v3.y]._m01_m11_m21;
  r2.y = dot(v1.xyz, r3.xyz);
  r3.xyz = BoneTransformConstantBuffer[v3.y]._m02_m12_m22;
  r2.z = dot(v1.xyz, r3.xyz);
  r2.xyz = v4.yyy * r2.xyz;
  r3.xyz = BoneTransformConstantBuffer[v3.x]._m00_m10_m20;
  r3.x = dot(v1.xyz, r3.xyz);
  r4.xyz = BoneTransformConstantBuffer[v3.x]._m01_m11_m21;
  r3.y = dot(v1.xyz, r4.xyz);
  r4.xyz = BoneTransformConstantBuffer[v3.x]._m02_m12_m22;
  r3.z = dot(v1.xyz, r4.xyz);
  r2.xyz = r3.xyz * v4.xxx + r2.xyz;
  r3.xyz = BoneTransformConstantBuffer[v3.z]._m00_m10_m20;
  r3.x = dot(v1.xyz, r3.xyz);
  r4.xyz = BoneTransformConstantBuffer[v3.z]._m01_m11_m21;
  r3.y = dot(v1.xyz, r4.xyz);
  r4.xyz = BoneTransformConstantBuffer[v3.z]._m02_m12_m22;
  r3.z = dot(v1.xyz, r4.xyz);
  r2.xyz = r3.xyz * v4.zzz + r2.xyz;
  r3.xyz = BoneTransformConstantBuffer[v3.w]._m00_m10_m20;
  r3.x = dot(v1.xyz, r3.xyz);
  r4.xyz = BoneTransformConstantBuffer[v3.w]._m01_m11_m21;
  r3.y = dot(v1.xyz, r4.xyz);
  r4.xyz = BoneTransformConstantBuffer[v3.w]._m02_m12_m22;
  r3.z = dot(v1.xyz, r4.xyz);
  r2.xyz = r3.xyz * v4.www + r2.xyz;
  r1.w = dot(r2.xyz, r2.xyz);
  r1.w = rsqrt(r1.w);
  r2.xyz = r2.xyz * r1.www;
  r1.x = dot(r2.xyz, r1.xyz);
  o5.xyz = r2.xyz;
  r1.y = cmp(r1.x < 0);
  r1.x = r1.y ? -r1.x : r1.x;
  r1.x = 1 + -r1.x;
  r1.x = max(0, r1.x);
  r1.x = log2(r1.x);
  r1.y = max(1, PointLightColor.w);
  r1.x = r1.y * r1.x;
  r1.x = exp2(r1.x);
  o2.xyz = PointLightColor.xyz * r1.xxx;
  r1.x = -scene.MiscParameters3.x + r0.y;
  o4.xyz = r0.xyz;
  r0.x = saturate(scene.MiscParameters3.y * r1.x);
  r0.y = -scene.FogRangeParameters.x + -r0.w;
  o4.w = -r0.w;
  r0.y = saturate(scene.FogRangeParameters.z * r0.y);
  r0.z = scene.MiscParameters3.z + r0.y;
  r0.z = min(1, r0.z);
  r0.x = r0.x * r0.z + r0.y;
  r0.x = min(1, r0.x);
  o2.w = scene.FogRangeParameters.w * r0.x;
  o3.xy = v2.xy * GameMaterialTexcoord.zw + GameMaterialTexcoord.xy;
  p3.xy = v5.xy * UVaMUvTexcoord.zw + UVaMUvTexcoord.xy;
  return;
}
