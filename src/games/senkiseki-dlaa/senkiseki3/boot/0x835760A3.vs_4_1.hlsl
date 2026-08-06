// ---- Created with 3Dmigoto v1.4.1 on Thu Aug  6 13:26:28 2026

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
  float AlphaThreshold : packoffset(c59) = {0.5};
  float3 ShadowColorShift : packoffset(c59.y) = {0.100000001,0.0199999996,0.0199999996};
  float3 RimLitColor : packoffset(c60) = {1,1,1};
  float RimLitIntensity : packoffset(c60.w) = {4};
  float RimLitPower : packoffset(c61) = {2};
  float RimLightClampFactor : packoffset(c61.y) = {2};
  float ShadowReceiveOffset : packoffset(c61.z) = {0.600000024};
  float BloomIntensity : packoffset(c61.w) = {0.699999988};
  float4 GameEdgeParameters : packoffset(c62) = {1,1,1,0.00300000003};
  float4 OutlineColorFactor : packoffset(c63) = {1,1,1,1};
  float4 PointLightParams : packoffset(c64) = {0,0,0,0};
  float4 PointLightColor : packoffset(c65) = {0,0,0,0};
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
  out float4 o0 : SV_POSITION0,
  out float4 o1 : COLOR0,
  out float4 o2 : COLOR1,
  out float4 o3 : TEXCOORD0,
  out float4 o4 : TEXCOORD1,
  out float4 o5 : TEXCOORD9,
  out float4 o6 : TEXCOORD10,
  out float4 o7 : TEXCOORD5)  // DLAA: prevClip
{
  float4 r0,r1,r2,r3;
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
  r0.w = dot(r0.xyzw, scene.ViewProjection._m03_m13_m23_m33);
  r0.w = max(0.300000012, r0.w);
  r0.w = min(1, r0.w);
  r1.xyz = BoneTransformConstantBuffer[v3.y]._m00_m10_m20;
  r1.x = dot(v1.xyz, r1.xyz);
  r2.xyz = BoneTransformConstantBuffer[v3.y]._m01_m11_m21;
  r1.y = dot(v1.xyz, r2.xyz);
  r2.xyz = BoneTransformConstantBuffer[v3.y]._m02_m12_m22;
  r1.z = dot(v1.xyz, r2.xyz);
  r1.xyz = v4.yyy * r1.xyz;
  r2.xyz = BoneTransformConstantBuffer[v3.x]._m00_m10_m20;
  r2.x = dot(v1.xyz, r2.xyz);
  r3.xyz = BoneTransformConstantBuffer[v3.x]._m01_m11_m21;
  r2.y = dot(v1.xyz, r3.xyz);
  r3.xyz = BoneTransformConstantBuffer[v3.x]._m02_m12_m22;
  r2.z = dot(v1.xyz, r3.xyz);
  r1.xyz = r2.xyz * v4.xxx + r1.xyz;
  r2.xyz = BoneTransformConstantBuffer[v3.z]._m00_m10_m20;
  r2.x = dot(v1.xyz, r2.xyz);
  r3.xyz = BoneTransformConstantBuffer[v3.z]._m01_m11_m21;
  r2.y = dot(v1.xyz, r3.xyz);
  r3.xyz = BoneTransformConstantBuffer[v3.z]._m02_m12_m22;
  r2.z = dot(v1.xyz, r3.xyz);
  r1.xyz = r2.xyz * v4.zzz + r1.xyz;
  r2.xyz = BoneTransformConstantBuffer[v3.w]._m00_m10_m20;
  r2.x = dot(v1.xyz, r2.xyz);
  r3.xyz = BoneTransformConstantBuffer[v3.w]._m01_m11_m21;
  r2.y = dot(v1.xyz, r3.xyz);
  r3.xyz = BoneTransformConstantBuffer[v3.w]._m02_m12_m22;
  r2.z = dot(v1.xyz, r3.xyz);
  r1.xyz = r2.xyz * v4.www + r1.xyz;
  r1.w = dot(r1.xyz, r1.xyz);
  r1.w = rsqrt(r1.w);
  r1.xyz = r1.xyz * r1.www;
  r2.xyz = GameEdgeParameters.www * r1.xyz;
  o5.xyz = r1.xyz;
  r0.xyz = r2.xyz * r0.www + r0.xyz;
  r1.xyz = -scene.EyePosition.xyz + r0.xyz;
  r0.w = dot(r1.xyz, r1.xyz);
  r0.w = rsqrt(r0.w);
  r1.xyz = r1.xyz * r0.www;
  r0.w = 0.00100000005 + GameEdgeParameters.w;
  r0.xyz = r1.xyz * r0.www + r0.xyz;
  r0.w = 1;
  r1.x = dot(r0.xyzw, scene.ViewProjection._m00_m10_m20_m30);
  r1.y = dot(r0.xyzw, scene.ViewProjection._m01_m11_m21_m31);
  r1.z = dot(r0.xyzw, scene.ViewProjection._m02_m12_m22_m32);
  r1.w = dot(r0.xyzw, scene.ViewProjection._m03_m13_m23_m33);
  // DLAA: per-object motion (prev clip-space position from prevViewProj x current pose).
  // Injected prev VP comes from the addon b13 cbuffer (Senkiseki3 has no c74).
  if (DLAA_PER_OBJECT_MOTION > 0.5f) {
    float4x4 prevViewProjection = float4x4(
        shader_injection_data.prev_view_proj[0],  shader_injection_data.prev_view_proj[1],  shader_injection_data.prev_view_proj[2],  shader_injection_data.prev_view_proj[3],
        shader_injection_data.prev_view_proj[4],  shader_injection_data.prev_view_proj[5],  shader_injection_data.prev_view_proj[6],  shader_injection_data.prev_view_proj[7],
        shader_injection_data.prev_view_proj[8],  shader_injection_data.prev_view_proj[9],  shader_injection_data.prev_view_proj[10], shader_injection_data.prev_view_proj[11],
        shader_injection_data.prev_view_proj[12], shader_injection_data.prev_view_proj[13], shader_injection_data.prev_view_proj[14], shader_injection_data.prev_view_proj[15]);
    float4 prevClip;
    float4 wp = float4(r0.xyz, 1);
    prevClip.x = dot(wp.xyzw, prevViewProjection._m00_m10_m20_m30);
    prevClip.y = dot(wp.xyzw, prevViewProjection._m01_m11_m21_m31);
    prevClip.z = dot(wp.xyzw, prevViewProjection._m02_m12_m22_m32);
    prevClip.w = dot(wp.xyzw, prevViewProjection._m03_m13_m23_m33);
    o7 = prevClip;
  } else {
    o7 = float4(0, 0, 0, 0);
  }
  r0.w = dot(r0.xyzw, scene.View._m02_m12_m22_m32);
  r0.w = -scene.FogRangeParameters.x + -r0.w;
  r0.w = saturate(scene.FogRangeParameters.z * r0.w);
  o0.xyzw = r1.xyzw;
  o6.xyzw = r1.xyzw;
  o1.xyz = OutlineColorFactor.xyz * GameEdgeParameters.xyz;
  o1.w = 1;
  r1.x = -scene.MiscParameters3.x + r0.y;
  o4.xyz = r0.xyz;
  r0.x = saturate(scene.MiscParameters3.y * r1.x);
  r0.y = scene.MiscParameters3.z + r0.w;
  r0.y = min(1, r0.y);
  r0.x = r0.x * r0.y + r0.w;
  r0.x = min(1, r0.x);
  o2.w = scene.FogRangeParameters.w * r0.x;
  o2.xyz = float3(0,0,0);
  o3.xy = v2.xy;
  o4.w = 0;
  return;
}