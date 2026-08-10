// ---- Created with 3Dmigoto v1.3.16 on Wed Jul 02 06:47:15 2025
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


  struct
  {
    float4x4 m_split0Transform;
    float4x4 m_split1Transform;
    float4 m_splitDistances;
  } LightShadow0 : packoffset(c64);

  float GameMaterialID : packoffset(c73) = {0};
  float4 GameMaterialDiffuse : packoffset(c74) = {1,1,1,1};
  float4 GameMaterialEmission : packoffset(c75) = {0,0,0,0};
  float GameMaterialMonotone : packoffset(c76) = {0};
  float4 GameMaterialTexcoord : packoffset(c77) = {0,0,1,1};
  float4 GameDitherParams : packoffset(c78) = {0,0,0,0};
  float4 UVaMUvColor : packoffset(c79) = {1,1,1,1};
  float4 UVaProjTexcoord : packoffset(c80) = {0,0,1,1};
  float4 UVaMUvTexcoord : packoffset(c81) = {0,0,1,1};
  float4 UVaMUv2Texcoord : packoffset(c82) = {0,0,1,1};
  float4 UVaDuDvTexcoord : packoffset(c83) = {0,0,1,1};
  float3 ShadowColorShift : packoffset(c84) = {0.100000001,0.0199999996,0.0199999996};
  float Shininess : packoffset(c84.w) = {0.5};
  float SpecularPower : packoffset(c85) = {50};
  float3 SpecularColor : packoffset(c85.y) = {1,1,1};
  float3 RimLitColor : packoffset(c86) = {1,1,1};
  float RimLitIntensity : packoffset(c86.w) = {4};
  float RimLitPower : packoffset(c87) = {2};
  float RimLightClampFactor : packoffset(c87.y) = {2};
  float FogRatio : packoffset(c87.z) = {1};
  float2 DuDvMapImageSize : packoffset(c88) = {256,256};
  float2 DuDvScale : packoffset(c88.z) = {4,4};
  float BloomIntensity : packoffset(c89) = {1};
  float ReflectionIntensity : packoffset(c89.y) = {0};
  float ReflectionFresnel : packoffset(c89.z) = {0};
  float MaskEps : packoffset(c89.w);
  float4 PointLightParams : packoffset(c90) = {0,2,1,1};
  float4 PointLightColor : packoffset(c91) = {1,0,0,0};
}

SamplerState LinearWrapSamplerState_s : register(s0);
SamplerState LinearClampSamplerState_s : register(s1);
SamplerState PointClampSamplerState_s : register(s2);
SamplerState DiffuseMapSamplerSampler_s : register(s4);
SamplerState NormalMapSamplerSampler_s : register(s5);
SamplerState DuDvMapSamplerSampler_s : register(s6);
SamplerComparisonState LinearClampCmpSamplerState_s : register(s3);
Texture2D<float4> LowResDepthTexture : register(t0);
Texture2D<float4> LightShadowMap0 : register(t1);
Texture2D<float4> DiffuseMapSampler : register(t2);
Texture2D<float4> NormalMapSampler : register(t3);
Texture2D<float4> DuDvMapSampler : register(t4);
Texture2D<float4> ReflectionTexture : register(t5);
Texture2D<float4> RefractionTexture : register(t6);


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
  float4 v6 : TEXCOORD6,
  float4 v7 : TEXCOORD10,
  out float4 o0 : SV_TARGET0,
  out float4 o1 : SV_TARGET1,
  out float4 o2 : SV_TARGET2)
{
  float4 r0,r1,r2,r3,r4,r5,r6,r7,r8;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.xy = DuDvMapSampler.Sample(DuDvMapSamplerSampler_s, w3.xy).xy;
  r0.xy = r0.xy * float2(2,2) + float2(-1,-1);
  r0.zw = DuDvScale.xy / DuDvMapImageSize.xy;
  r1.xy = r0.xy * r0.zw;
  r0.xy = r0.xy * r0.zw + v3.xy;
  r2.xyzw = DiffuseMapSampler.Sample(DiffuseMapSamplerSampler_s, r0.xy).xyzw;
  r0.z = v1.w * r2.w;
  r1.zw = v0.xy / scene.ViewportWidthHeight.xy;
  r3.xy = v0.ww * r1.zw;
  r3.zw = scene.ViewportWidthHeight.xy / DuDvMapImageSize.xy;
  r1.xy = r1.xy * r3.zw + r3.xy;
  r0.w = cmp(LightShadow0.m_splitDistances.y >= v4.w);
  if (r0.w != 0) {
    r3.xyz = Light0.m_direction.xyz * float3(0.0500000007,0.0500000007,0.0500000007) + v4.xyz;
    r3.xyz = v5.xyz * float3(0.0500000007,0.0500000007,0.0500000007) + r3.xyz;
    r0.w = cmp(v4.w < LightShadow0.m_splitDistances.x);
    if (r0.w != 0) {
      r3.w = 1;
      r4.x = dot(r3.xyzw, LightShadow0.m_split0Transform._m00_m10_m20_m30);
      r4.y = dot(r3.xyzw, LightShadow0.m_split0Transform._m01_m11_m21_m31);
      r4.z = dot(r3.xyzw, LightShadow0.m_split0Transform._m02_m12_m22_m32);
      r0.w = 25 / LightShadow0.m_splitDistances.y;
      r2.w = (int)scene.DuranteSettings.x & 16;
      if (r2.w != 0) {
        r5.xy = float2(0.000937499979,0.00187499996) * r0.ww;
        r2.w = -6 + r4.z;
        r5.xy = r5.xy * r2.ww;
        r5.xy = r5.xy / r4.zz;
        r2.w = dot(v0.xy, float2(0.0671105608,0.00583714992));
        r2.w = frac(r2.w);
        r2.w = 52.9829178 * r2.w;
        r2.w = frac(r2.w);
        r2.w = 6.28318548 * r2.w;
        r5.zw = float2(0,0);
        r4.w = 0;
        while (true) {
          r6.x = cmp((int)r4.w >= 16);
          if (r6.x != 0) break;
          r6.x = (int)r4.w;
          r6.y = 0.5 + r6.x;
          r6.y = sqrt(r6.y);
          r6.y = 0.25 * r6.y;
          r6.x = r6.x * 2.4000001 + r2.w;
          sincos(r6.x, r6.x, r7.x);
          r7.x = r7.x * r6.y;
          r7.y = r6.y * r6.x;
          r6.xy = r7.xy * r5.xy + r4.xy;
          r6.x = LightShadowMap0.SampleLevel(PointClampSamplerState_s, r6.xy, 0).x;
          r6.y = cmp(r6.x < r4.z);
          r7.y = r6.x + r5.w;
          r7.x = 1 + r5.z;
          r5.zw = r6.yy ? r7.xy : r5.zw;
          r4.w = (int)r4.w + 1;
        }
        r4.w = cmp(r5.z >= 1);
        if (r4.w != 0) {
          r5.xy = float2(0.000624999986,0.00124999997) * r0.ww;
          r4.w = r5.w / r5.z;
          r4.w = r4.z + -r4.w;
          r4.w = min(0.0700000003, r4.w);
          r4.w = 60 * r4.w;
          r5.xy = r4.ww * r5.xy;
          LightShadowMap0.GetDimensions(0, fDest.x, fDest.y, fDest.z);
          r5.zw = fDest.xy;
          r5.zw = float2(0.5,0.5) / r5.zw;
          r5.xy = max(r5.zw, r5.xy);
          r4.w = 0;
          r5.z = 0;
          while (true) {
            r5.w = cmp((int)r5.z >= 16);
            if (r5.w != 0) break;
            r5.w = (int)r5.z;
            r6.x = 0.5 + r5.w;
            r6.x = sqrt(r6.x);
            r6.x = 0.25 * r6.x;
            r5.w = r5.w * 2.4000001 + r2.w;
            sincos(r5.w, r7.x, r8.x);
            r8.x = r8.x * r6.x;
            r8.y = r7.x * r6.x;
            r6.xy = r8.xy * r5.xy;
            r6.xy = r6.xy * float2(3,3) + r4.xy;
            r5.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r6.xy, r4.z).x;
            r4.w = r5.w + r4.w;
            r5.z = (int)r5.z + 1;
          }
          r2.w = 0.0625 * r4.w;
        } else {
          r2.w = 1;
        }
      } else {
        r4.w = (int)scene.DuranteSettings.x & 8;
        if (r4.w != 0) {
          r5.xy = float2(0.000375000003,0.000750000007) * r0.ww;
          r4.w = dot(v0.xy, float2(0.0671105608,0.00583714992));
          r4.w = frac(r4.w);
          r4.w = 52.9829178 * r4.w;
          r4.w = frac(r4.w);
          r4.w = 6.28318548 * r4.w;
          r5.zw = float2(0,0);
          while (true) {
            r6.x = cmp((int)r5.w >= 16);
            if (r6.x != 0) break;
            r6.x = (int)r5.w;
            r6.y = 0.5 + r6.x;
            r6.y = sqrt(r6.y);
            r6.y = 0.25 * r6.y;
            r6.x = r6.x * 2.4000001 + r4.w;
            sincos(r6.x, r6.x, r7.x);
            r7.x = r7.x * r6.y;
            r7.y = r6.y * r6.x;
            r6.xy = r7.xy * r5.xy;
            r6.xy = r6.xy * float2(3,3) + r4.xy;
            r6.x = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r6.xy, r4.z).x;
            r5.z = r6.x + r5.z;
            r5.w = (int)r5.w + 1;
          }
          r2.w = 0.0625 * r5.z;
        } else {
          r4.w = dot(r3.xyzw, LightShadow0.m_split0Transform._m03_m13_m23_m33);
          r5.x = (int)scene.DuranteSettings.x & 4;
          if (r5.x != 0) {
            r5.xy = scene.MiscParameters4.xy * r4.ww;
            r5.z = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r4.xy, r4.z).x;
            r6.xy = -r5.xy * r0.ww;
            r6.z = 0;
            r6.xyz = r6.xyz + r4.xyz;
            r5.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r6.xy, r6.z).x;
            r5.w = 0.0625 * r5.w;
            r5.z = r5.z * 0.5 + r5.w;
            r6.x = r5.x * r0.w;
            r6.y = -r5.y * r0.w;
            r6.z = 0;
            r7.xyz = r6.xyz + r4.xyz;
            r5.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r7.xy, r7.z).x;
            r5.z = r5.w * 0.0625 + r5.z;
            r7.x = -r5.x * r0.w;
            r7.y = r5.y * r0.w;
            r7.z = 0;
            r8.xyz = r7.xyz + r4.xyz;
            r5.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r8.xy, r8.z).x;
            r5.z = r5.w * 0.0625 + r5.z;
            r8.xy = r5.xy * r0.ww;
            r8.z = 0;
            r5.xyw = r8.xyz + r4.xyz;
            r5.x = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r5.xy, r5.w).x;
            r5.x = r5.x * 0.0625 + r5.z;
            r5.yzw = r6.zyz + r4.xyz;
            r5.y = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r5.yz, r5.w).x;
            r5.x = r5.y * 0.0625 + r5.x;
            r5.yzw = r7.xzz + r4.xyz;
            r5.y = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r5.yz, r5.w).x;
            r5.x = r5.y * 0.0625 + r5.x;
            r5.yzw = r6.xzz + r4.xyz;
            r5.y = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r5.yz, r5.w).x;
            r5.x = r5.y * 0.0625 + r5.x;
            r5.yzw = r8.zyz + r4.xyz;
            r5.y = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r5.yz, r5.w).x;
            r2.w = r5.y * 0.0625 + r5.x;
          } else {
            r5.x = (int)scene.DuranteSettings.x & 2;
            if (r5.x != 0) {
              r5.xy = scene.MiscParameters4.xy * r4.ww;
              r4.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r4.xy, r4.z).x;
              r6.xy = -r5.xy * r0.ww;
              r6.z = 0;
              r6.xyz = r6.xyz + r4.xyz;
              r5.z = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r6.xy, r6.z).x;
              r5.z = scene.MiscParameters5.y * r5.z;
              r4.w = r4.w * scene.MiscParameters5.x + r5.z;
              r6.x = r5.x * r0.w;
              r6.y = -r5.y * r0.w;
              r6.z = 0;
              r6.xyz = r6.xyz + r4.xyz;
              r5.z = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r6.xy, r6.z).x;
              r4.w = r5.z * scene.MiscParameters5.y + r4.w;
              r6.x = -r5.x * r0.w;
              r6.y = r5.y * r0.w;
              r6.z = 0;
              r6.xyz = r6.xyz + r4.xyz;
              r5.z = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r6.xy, r6.z).x;
              r4.w = r5.z * scene.MiscParameters5.y + r4.w;
              r5.xy = r5.xy * r0.ww;
              r5.z = 0;
              r5.xyz = r5.xyz + r4.xyz;
              r0.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r5.xy, r5.z).x;
              r2.w = r0.w * scene.MiscParameters5.y + r4.w;
            } else {
              r2.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r4.xy, r4.z).x;
            }
          }
        }
      }
    } else {
      r3.w = 1;
      r4.x = dot(r3.xyzw, LightShadow0.m_split1Transform._m00_m10_m20_m30);
      r4.y = dot(r3.xyzw, LightShadow0.m_split1Transform._m01_m11_m21_m31);
      r4.z = dot(r3.xyzw, LightShadow0.m_split1Transform._m02_m12_m22_m32);
      r0.w = 10 / LightShadow0.m_splitDistances.y;
      r4.w = (int)scene.DuranteSettings.x & 16;
      if (r4.w != 0) {
        r5.xy = float2(0.000937499979,0.00187499996) * r0.ww;
        r4.w = -6 + r4.z;
        r5.xy = r5.xy * r4.ww;
        r5.xy = r5.xy / r4.zz;
        r4.w = dot(v0.xy, float2(0.0671105608,0.00583714992));
        r4.w = frac(r4.w);
        r4.w = 52.9829178 * r4.w;
        r4.w = frac(r4.w);
        r4.w = 6.28318548 * r4.w;
        r5.zw = float2(0,0);
        r6.x = 0;
        while (true) {
          r6.y = cmp((int)r6.x >= 16);
          if (r6.y != 0) break;
          r6.y = (int)r6.x;
          r6.z = 0.5 + r6.y;
          r6.z = sqrt(r6.z);
          r6.z = 0.25 * r6.z;
          r6.y = r6.y * 2.4000001 + r4.w;
          sincos(r6.y, r7.x, r8.x);
          r8.x = r8.x * r6.z;
          r8.y = r7.x * r6.z;
          r6.yz = r8.xy * r5.xy + r4.xy;
          r6.y = LightShadowMap0.SampleLevel(PointClampSamplerState_s, r6.yz, 0).x;
          r6.z = cmp(r6.y < r4.z);
          r7.y = r6.y + r5.w;
          r7.x = 1 + r5.z;
          r5.zw = r6.zz ? r7.xy : r5.zw;
          r6.x = (int)r6.x + 1;
        }
        r5.x = cmp(r5.z >= 1);
        if (r5.x != 0) {
          r5.xy = float2(0.000624999986,0.00124999997) * r0.ww;
          r5.z = r5.w / r5.z;
          r5.z = -r5.z + r4.z;
          r5.z = min(0.0700000003, r5.z);
          r5.z = 60 * r5.z;
          r5.xy = r5.zz * r5.xy;
          LightShadowMap0.GetDimensions(0, fDest.x, fDest.y, fDest.z);
          r5.zw = fDest.xy;
          r5.zw = float2(0.5,0.5) / r5.zw;
          r5.xy = max(r5.zw, r5.xy);
          r5.zw = float2(0,0);
          while (true) {
            r6.x = cmp((int)r5.w >= 16);
            if (r6.x != 0) break;
            r6.x = (int)r5.w;
            r6.y = 0.5 + r6.x;
            r6.y = sqrt(r6.y);
            r6.y = 0.25 * r6.y;
            r6.x = r6.x * 2.4000001 + r4.w;
            sincos(r6.x, r6.x, r7.x);
            r7.x = r7.x * r6.y;
            r7.y = r6.y * r6.x;
            r6.xy = r7.xy * r5.xy;
            r6.xy = r6.xy * float2(3,3) + r4.xy;
            r6.x = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r6.xy, r4.z).x;
            r5.z = r6.x + r5.z;
            r5.w = (int)r5.w + 1;
          }
          r2.w = 0.0625 * r5.z;
        } else {
          r2.w = 1;
        }
      } else {
        r4.w = (int)scene.DuranteSettings.x & 8;
        if (r4.w != 0) {
          r5.xy = float2(0.000375000003,0.000750000007) * r0.ww;
          r4.w = dot(v0.xy, float2(0.0671105608,0.00583714992));
          r4.w = frac(r4.w);
          r4.w = 52.9829178 * r4.w;
          r4.w = frac(r4.w);
          r4.w = 6.28318548 * r4.w;
          r5.zw = float2(0,0);
          while (true) {
            r6.x = cmp((int)r5.w >= 16);
            if (r6.x != 0) break;
            r6.x = (int)r5.w;
            r6.y = 0.5 + r6.x;
            r6.y = sqrt(r6.y);
            r6.y = 0.25 * r6.y;
            r6.x = r6.x * 2.4000001 + r4.w;
            sincos(r6.x, r6.x, r7.x);
            r7.x = r7.x * r6.y;
            r7.y = r6.y * r6.x;
            r6.xy = r7.xy * r5.xy;
            r6.xy = r6.xy * float2(3,3) + r4.xy;
            r6.x = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r6.xy, r4.z).x;
            r5.z = r6.x + r5.z;
            r5.w = (int)r5.w + 1;
          }
          r2.w = 0.0625 * r5.z;
        } else {
          r3.x = dot(r3.xyzw, LightShadow0.m_split1Transform._m03_m13_m23_m33);
          r3.y = (int)scene.DuranteSettings.x & 4;
          if (r3.y != 0) {
            r3.yz = scene.MiscParameters4.xy * r3.xx;
            r3.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r4.xy, r4.z).x;
            r5.xy = -r3.yz * r0.ww;
            r5.z = 0;
            r6.xyz = r5.xyz + r4.xyz;
            r4.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r6.xy, r6.z).x;
            r4.w = 0.0625 * r4.w;
            r3.w = r3.w * 0.5 + r4.w;
            r6.x = r3.y * r0.w;
            r6.y = -r3.z * r0.w;
            r6.z = 0;
            r7.xyz = r6.xyz + r4.xyz;
            r4.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r7.xy, r7.z).x;
            r3.w = r4.w * 0.0625 + r3.w;
            r7.x = -r3.y * r0.w;
            r7.y = r3.z * r0.w;
            r7.z = 0;
            r7.xyz = r7.xyz + r4.xyz;
            r4.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r7.xy, r7.z).x;
            r3.w = r4.w * 0.0625 + r3.w;
            r7.xy = r3.yz * r0.ww;
            r7.z = 0;
            r8.xyz = r7.xyz + r4.xyz;
            r3.y = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r8.xy, r8.z).x;
            r3.y = r3.y * 0.0625 + r3.w;
            r8.xyz = r6.zyz + r4.xyz;
            r3.z = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r8.xy, r8.z).x;
            r3.y = r3.z * 0.0625 + r3.y;
            r5.xyz = r5.xzz + r4.xyz;
            r3.z = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r5.xy, r5.z).x;
            r3.y = r3.z * 0.0625 + r3.y;
            r5.xyz = r6.xzz + r4.xyz;
            r3.z = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r5.xy, r5.z).x;
            r3.y = r3.z * 0.0625 + r3.y;
            r5.xyz = r7.zyz + r4.xyz;
            r3.z = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r5.xy, r5.z).x;
            r2.w = r3.z * 0.0625 + r3.y;
          } else {
            r3.y = (int)scene.DuranteSettings.x & 2;
            if (r3.y != 0) {
              r3.xy = scene.MiscParameters4.xy * r3.xx;
              r3.z = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r4.xy, r4.z).x;
              r5.xy = -r3.xy * r0.ww;
              r5.z = 0;
              r5.xyz = r5.xyz + r4.xyz;
              r3.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r5.xy, r5.z).x;
              r3.w = scene.MiscParameters5.y * r3.w;
              r3.z = r3.z * scene.MiscParameters5.x + r3.w;
              r5.x = r3.x * r0.w;
              r5.y = -r3.y * r0.w;
              r5.z = 0;
              r5.xyz = r5.xyz + r4.xyz;
              r3.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r5.xy, r5.z).x;
              r3.z = r3.w * scene.MiscParameters5.y + r3.z;
              r5.x = -r3.x * r0.w;
              r5.y = r3.y * r0.w;
              r5.z = 0;
              r5.xyz = r5.xyz + r4.xyz;
              r3.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r5.xy, r5.z).x;
              r3.z = r3.w * scene.MiscParameters5.y + r3.z;
              r5.xy = r3.xy * r0.ww;
              r5.z = 0;
              r3.xyw = r5.xyz + r4.xyz;
              r0.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r3.xy, r3.w).x;
              r2.w = r0.w * scene.MiscParameters5.y + r3.z;
            } else {
              r2.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r4.xy, r4.z).x;
            }
          }
        }
      }
    }
  } else {
    r2.w = 1;
  }
  r0.w = saturate(v4.w / LightShadow0.m_splitDistances.y);
  r3.x = r0.w * r0.w;
  r3.y = -scene.MiscParameters2.y + v4.y;
  r3.y = scene.MiscParameters2.x * abs(r3.y);
  r3.y = min(1, r3.y);
  r3.z = r3.y * r3.y;
  r3.y = r3.y * r3.z;
  r3.z = cmp(0 >= scene.MiscParameters2.x);
  r3.y = r3.z ? 0 : r3.y;
  r0.w = r0.w * r3.x + r3.y;
  r3.x = dot(v5.xyz, Light0.m_direction.xyz);
  r3.x = r3.x * 0.5 + 0.5;
  r3.y = 0.400000006 * scene.MiscParameters4.w;
  r3.z = max(0, -v5.y);
  r3.y = -r3.y * r3.z + 0.400000006;
  r3.z = cmp(r3.y < r3.x);
  r3.x = r3.x + -r3.y;
  r3.y = 1 + -r3.y;
  r3.x = r3.x / r3.y;
  r3.x = r3.z ? r3.x : 0;
  r3.x = 1 + -r3.x;
  r3.y = r3.x * r3.x;
  r0.w = r3.x * r3.y + r0.w;
  r0.w = min(1, r0.w);
  r0.w = r2.w + r0.w;
  r0.w = min(1, r0.w);
  r0.w = 1 + r0.w;
  r2.w = 0.5 * r0.w;
  r0.w = -r0.w * 0.5 + 1;
  r3.x = 1 + -scene.MiscParameters2.w;
  r0.w = r0.w * r3.x + r2.w;
  r2.w = 1 + -r0.w;
  r3.x = 1 + -GameMaterialEmission.w;
  r0.w = r2.w * r3.x + r0.w;
  r2.w = r0.w * r0.w;
  r3.xyz = Light0.m_colorIntensity.xyz * r2.www;
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
  // r0.y = (dot(r4.xyz, r6.xyz));
  r2.w = dot(Light0.m_direction.xyz, r4.xyz);
  r2.w = r2.w * 0.5 + 0.5;
  r2.w = r2.w * r2.w;
  r6.xzw = SpecularColor.xyz * Light0.m_colorIntensity.xyz;
  r5.xyz = r5.xyz * r0.xxx + scene.FakeRimLightDir.xyz;
  r0.x = dot(r5.xyz, r5.xyz);
  r0.x = rsqrt(r0.x);
  r5.xyz = r5.xyz * r0.xxx;
  r0.x = saturate(dot(r4.xyz, r5.xyz));
  // r0.x = (dot(r4.xyz, r5.xyz));
  // r0.x = log2(r0.x);
  // r0.x = SpecularPower * r0.x;
  // r0.x = exp2(r0.x);
  r0.x = renodx::math::SafePow(r0.x, SpecularPower);
  r0.x = min(1, r0.x);
  r0.x = Shininess * r0.x;
  r5.xyz = Light0.m_colorIntensity.xyz * r2.www + scene.GlobalAmbientColor.xyz;
  r5.xyz = min(float3(1.5,1.5,1.5), r5.xyz);
  r5.xyz = r6.xzw * r0.xxx + r5.xyz;
  r0.x = 1 + -r0.w;
  r6.xzw = r5.xyz * scene.MiscParameters1.xyz + -r5.xyz;
  r5.xyz = r0.xxx * r6.xzw + r5.xyz;
  r0.x = 1 + -r0.y;
  float l = r0.x;
  // r0.x = log2(r0.x);
  // r0.y = RimLitPower * r0.x;
  // r0.y = exp2(r0.y);
  r0.y = renodx::math::SafePow(l, RimLitPower);
  r0.y = RimLitIntensity * r0.y;
  r6.xzw = RimLitColor.xyz * r0.yyy;
  r5.xyz = r6.xzw * r3.xyz + r5.xyz;
  r5.xyz = min(RimLightClampFactor, r5.xyz);
  r0.yw = r1.zw * v0.ww + -r1.xy;
  r0.yw = scene.AdditionalShadowOffset * r0.yw + r1.xy;
  r0.yw = r0.yw / v0.ww;
  r6.xzw = RefractionTexture.Sample(LinearClampSamplerState_s, r0.yw).xyz;
  r2.xyz = -r6.xzw + r2.xyz;
  r0.yzw = r0.zzz * r2.xyz + r6.xzw;
  r1.xy = r1.xy / v0.ww;
  r1.z = 1 + -r1.y;
  r1.xyz = ReflectionTexture.Sample(LinearClampSamplerState_s, r1.xz).xyz;
  r1.w = max(0, r6.y);
  r1.w = -r1.w * ReflectionFresnel + 1;
  r2.x = r1.w * r1.w;
  r1.w = r2.x * r1.w;
  r1.w = ReflectionIntensity * r1.w;
  r1.xyz = r1.xyz + -r0.yzw;
  r0.yzw = r1.www * r1.xyz + r0.yzw;
  r1.xyz = r3.xyz + r3.xyz;
  r1.xyz = max(float3(1,1,1), r1.xyz);
  r2.xyz = min(float3(1,1,1), r5.xyz);
  r2.xyz = float3(1,1,1) + -r2.xyz;
  r2.xyz = ShadowColorShift.xyz * r2.xyz;
  r1.xyz = r2.xyz * r1.xyz + r5.xyz;
  r1.xyz = v1.xyz * r1.xyz;
  r0.yzw = r1.xyz * r0.yzw;
  // r0.x = PointLightColor.x * r0.x;
  // r0.x = exp2(r0.x);
  r0.x = renodx::math::SafePow(l, PointLightColor.x);
  r0.x = -1 + r0.x;
  r0.x = PointLightColor.y * r0.x + 1;
  r1.xyz = GameMaterialEmission.xyz * r0.xxx;
  r0.xyz = r0.yzw * GameMaterialDiffuse.xyz + r1.xyz;
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
  r0.w = -r0.w * 0.5 + 1;
  r0.w = r0.w * r0.w;
  r0.w = PointLightParams.z * r0.w;
  // r1.x = dot(r0.xyz, float3(0.298999995, 0.587000012, 0.114));
  r1.x = calculateLuminanceSRGB(r0.xyz);
  r1.xyz = r1.xxx * scene.MonotoneMul.xyz + scene.MonotoneAdd.xyz;
  r1.xyz = r1.xyz + -r0.xyz;
  r0.xyz = GameMaterialMonotone * r1.xyz + r0.xyz;
  r1.xyz = BloomIntensity * r0.xyz;
  // r1.x = dot(r1.xyz, float3(0.298999995,0.587000012,0.114));
  r1.x = calculateLuminanceSRGB(r1.xyz);
  r1.x = -scene.MiscParameters2.z + r1.x;
  r1.x = max(0, r1.x);
  r1.x = 0.5 * r1.x;
  r1.x = min(1, r1.x);
  o0.w = r1.x * r0.w;
  o1.xyz = r4.xyz * float3(0.5,0.5,0.5) + float3(0.5,0.5,0.5);
  o1.w = 0.466666698 + MaskEps;
  r0.w = v7.z / v7.w;
  r1.x = 256 * r0.w;
  r1.x = trunc(r1.x);
  r0.w = r0.w * 256 + -r1.x;
  r1.w = 256 * r0.w;
  r1.y = trunc(r1.w);
  r1.z = r0.w * 256 + -r1.y;
  o2.xyz = float3(0.00390625,0.00390625,1) * r1.xyz;
  o0.xyz = r0.xyz;
  o2.w = MaskEps;

  o0 = max(o0, 0.f);
  o1 = max(o1, 0.f);
  o2 = max(o2, 0.f);
  return;
}