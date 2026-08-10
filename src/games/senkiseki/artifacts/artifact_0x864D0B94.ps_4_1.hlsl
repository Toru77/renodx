// ---- Created with 3Dmigoto v1.3.16 on Thu Jul 03 19:51:59 2025
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
  float BlendMulScale2 : packoffset(c87.z) = {1};
  float2 DuDvMapImageSize : packoffset(c88) = {256,256};
  float2 DuDvScale : packoffset(c88.z) = {4,4};
  float BloomIntensity : packoffset(c89) = {1};
  float MaskEps : packoffset(c89.y);
  float4 PointLightParams : packoffset(c90) = {0,2,1,1};
  float4 PointLightColor : packoffset(c91) = {1,0,0,0};
}

SamplerState LinearClampSamplerState_s : register(s0);
SamplerState PointWrapSamplerState_s : register(s1);
SamplerState PointClampSamplerState_s : register(s2);
SamplerState DiffuseMapSamplerSampler_s : register(s4);
SamplerState NormalMapSamplerSampler_s : register(s5);
SamplerState DiffuseMap2SamplerSampler_s : register(s6);
SamplerState DuDvMapSamplerSampler_s : register(s7);
SamplerComparisonState LinearClampCmpSamplerState_s : register(s3);
Texture2D<float4> DitherNoiseTexture : register(t0);
Texture2D<float4> LightShadowMap0 : register(t1);
Texture2D<float4> DiffuseMapSampler : register(t2);
Texture2D<float4> NormalMapSampler : register(t3);
Texture2D<float4> DiffuseMap2Sampler : register(t4);
Texture2D<float4> DuDvMapSampler : register(t5);
Texture2D<float4> RefractionTexture : register(t6);


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
  float4 v7 : TEXCOORD6,
  float4 v8 : TEXCOORD9,
  float4 v9 : TEXCOORD10,
  out float4 o0 : SV_TARGET0,
  out float4 o1 : SV_TARGET1,
  out float4 o2 : SV_TARGET2)
{
  float4 r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.xy = float2(0.25,0.25) * v0.xy;
  r0.x = DitherNoiseTexture.SampleLevel(PointWrapSamplerState_s, r0.xy, 0).x;
  r0.x = v8.y + -r0.x;
  r0.y = cmp(0 < v8.z);
  r0.z = cmp(v8.z < 0);
  r0.y = (int)-r0.y + (int)r0.z;
  r0.y = cmp((int)r0.y < 0);
  r0.x = r0.y ? -r0.x : r0.x;
  r0.x = cmp(r0.x < 0);
  if (r0.x != 0) discard;
  r0.xy = DuDvMapSampler.Sample(DuDvMapSamplerSampler_s, v6.xy).xy;
  r0.xy = r0.xy * float2(2,2) + float2(-1,-1);
  r0.zw = DuDvScale.xy / DuDvMapImageSize.xy;
  r1.xy = r0.xy * r0.zw;
  r0.xyzw = r0.xyxy * r0.zwzw + float4(v3.x, v3.y, w3.x, w3.y);
  r2.xyzw = DiffuseMapSampler.Sample(DiffuseMapSamplerSampler_s, r0.xy).xyzw;
  r1.z = v1.w * r2.w;
  r3.xy = v0.xy / scene.ViewportWidthHeight.xy;
  r3.zw = v0.ww * r3.xy;
  r4.xy = scene.ViewportWidthHeight.xy / DuDvMapImageSize.xy;
  r1.xy = r1.xy * r4.xy + r3.zw;
  r4.xyzw = DiffuseMap2Sampler.Sample(DiffuseMap2SamplerSampler_s, r0.zw).xyzw;
  r4.xyzw = UVaMUvColor.xyzw * r4.xyzw;
  r0.z = v1.w * r4.w;
  r4.xyz = r4.xyz * r2.xyz;
  r4.xyz = r4.xyz * BlendMulScale2 + -r2.xyz;
  r2.xyz = r0.zzz * r4.xyz + r2.xyz;
  r0.z = cmp(LightShadow0.m_splitDistances.y >= v4.w);
  if (r0.z != 0) {
    r4.xyz = Light0.m_direction.xyz * float3(0.0500000007,0.0500000007,0.0500000007) + v4.xyz;
    r4.xyz = v5.xyz * float3(0.0500000007,0.0500000007,0.0500000007) + r4.xyz;
    r0.z = cmp(v4.w < LightShadow0.m_splitDistances.x);
    if (r0.z != 0) {
      r4.w = 1;
      r5.x = dot(r4.xyzw, LightShadow0.m_split0Transform._m00_m10_m20_m30);
      r5.y = dot(r4.xyzw, LightShadow0.m_split0Transform._m01_m11_m21_m31);
      r5.z = dot(r4.xyzw, LightShadow0.m_split0Transform._m02_m12_m22_m32);
      r0.z = 25 / LightShadow0.m_splitDistances.y;
      r0.w = (int)scene.DuranteSettings.x & 16;
      if (r0.w != 0) {
        r3.zw = float2(0.000937499979,0.00187499996) * r0.zz;
        r0.w = -6 + r5.z;
        r3.zw = r3.zw * r0.ww;
        r3.zw = r3.zw / r5.zz;
        r0.w = dot(v0.xy, float2(0.0671105608,0.00583714992));
        r0.w = frac(r0.w);
        r0.w = 52.9829178 * r0.w;
        r0.w = frac(r0.w);
        r0.w = 6.28318548 * r0.w;
        r6.xy = float2(0,0);
        r1.w = 0;
        while (true) {
          r2.w = cmp((int)r1.w >= 16);
          if (r2.w != 0) break;
          r2.w = (int)r1.w;
          r5.w = 0.5 + r2.w;
          r5.w = sqrt(r5.w);
          r5.w = 0.25 * r5.w;
          r2.w = r2.w * 2.4000001 + r0.w;
          sincos(r2.w, r7.x, r8.x);
          r8.x = r8.x * r5.w;
          r8.y = r7.x * r5.w;
          r6.zw = r8.xy * r3.zw + r5.xy;
          r2.w = LightShadowMap0.SampleLevel(PointClampSamplerState_s, r6.zw, 0).x;
          r5.w = cmp(r2.w < r5.z);
          r7.y = r6.y + r2.w;
          r7.x = 1 + r6.x;
          r6.xy = r5.ww ? r7.xy : r6.xy;
          r1.w = (int)r1.w + 1;
        }
        r1.w = cmp(r6.x >= 1);
        if (r1.w != 0) {
          r3.zw = float2(0.000624999986,0.00124999997) * r0.zz;
          r1.w = r6.y / r6.x;
          r1.w = r5.z + -r1.w;
          r1.w = min(0.0700000003, r1.w);
          r1.w = 60 * r1.w;
          r3.zw = r1.ww * r3.zw;
          LightShadowMap0.GetDimensions(0, fDest.x, fDest.y, fDest.z);
          r6.xy = fDest.xy;
          r7.x = 0.5 / r6.x;
          r7.y = 1 / r6.y;
          r3.zw = max(r7.xy, r3.zw);
          r1.w = 0;
          r2.w = 0;
          while (true) {
            r5.w = cmp((int)r2.w >= 16);
            if (r5.w != 0) break;
            r5.w = (int)r2.w;
            r6.x = 0.5 + r5.w;
            r6.x = sqrt(r6.x);
            r6.x = 0.25 * r6.x;
            r5.w = r5.w * 2.4000001 + r0.w;
            sincos(r5.w, r7.x, r8.x);
            r8.x = r8.x * r6.x;
            r8.y = r7.x * r6.x;
            r6.xy = r8.xy * r3.zw;
            r6.xy = r6.xy * float2(3,3) + r5.xy;
            r5.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r6.xy, r5.z).x;
            r1.w = r5.w + r1.w;
            r2.w = (int)r2.w + 1;
          }
          r0.w = 0.0625 * r1.w;
        } else {
          r0.w = 1;
        }
      } else {
        r1.w = (int)scene.DuranteSettings.x & 8;
        if (r1.w != 0) {
          r3.zw = float2(0.000375000003,0.000750000007) * r0.zz;
          r1.w = dot(v0.xy, float2(0.0671105608,0.00583714992));
          r1.w = frac(r1.w);
          r1.w = 52.9829178 * r1.w;
          r1.w = frac(r1.w);
          r1.w = 6.28318548 * r1.w;
          r2.w = 0;
          r5.w = 0;
          while (true) {
            r6.x = cmp((int)r5.w >= 16);
            if (r6.x != 0) break;
            r6.x = (int)r5.w;
            r6.y = 0.5 + r6.x;
            r6.y = sqrt(r6.y);
            r6.y = 0.25 * r6.y;
            r6.x = r6.x * 2.4000001 + r1.w;
            sincos(r6.x, r6.x, r7.x);
            r7.x = r7.x * r6.y;
            r7.y = r6.y * r6.x;
            r6.xy = r7.xy * r3.zw;
            r6.xy = r6.xy * float2(3,3) + r5.xy;
            r6.x = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r6.xy, r5.z).x;
            r2.w = r6.x + r2.w;
            r5.w = (int)r5.w + 1;
          }
          r0.w = 0.0625 * r2.w;
        } else {
          r1.w = dot(r4.xyzw, LightShadow0.m_split0Transform._m03_m13_m23_m33);
          r2.w = (int)scene.DuranteSettings.x & 4;
          if (r2.w != 0) {
            r3.zw = scene.MiscParameters4.xy * r1.ww;
            r2.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r5.xy, r5.z).x;
            r6.xy = -r3.zw * r0.zz;
            r6.z = 0;
            r7.xyz = r6.xyz + r5.xyz;
            r5.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r7.xy, r7.z).x;
            r5.w = 0.0625 * r5.w;
            r2.w = r2.w * 0.5 + r5.w;
            r7.x = r3.z * r0.z;
            r7.y = -r3.w * r0.z;
            r7.z = 0;
            r8.xyz = r7.xyz + r5.xyz;
            r5.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r8.xy, r8.z).x;
            r2.w = r5.w * 0.0625 + r2.w;
            r8.x = -r3.z * r0.z;
            r8.y = r3.w * r0.z;
            r8.z = 0;
            r9.xyz = r8.xyz + r5.xyz;
            r5.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r9.xy, r9.z).x;
            r2.w = r5.w * 0.0625 + r2.w;
            r9.xy = r3.zw * r0.zz;
            r9.z = 0;
            r10.xyz = r9.xyz + r5.xyz;
            r3.z = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r10.xy, r10.z).x;
            r2.w = r3.z * 0.0625 + r2.w;
            r6.xyz = r6.zyz + r5.xyz;
            r3.z = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r6.xy, r6.z).x;
            r2.w = r3.z * 0.0625 + r2.w;
            r6.xyz = r8.xzz + r5.xyz;
            r3.z = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r6.xy, r6.z).x;
            r2.w = r3.z * 0.0625 + r2.w;
            r6.xyz = r7.xzz + r5.xyz;
            r3.z = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r6.xy, r6.z).x;
            r2.w = r3.z * 0.0625 + r2.w;
            r6.xyz = r9.zyz + r5.xyz;
            r3.z = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r6.xy, r6.z).x;
            r0.w = r3.z * 0.0625 + r2.w;
          } else {
            r2.w = (int)scene.DuranteSettings.x & 2;
            if (r2.w != 0) {
              r3.zw = scene.MiscParameters4.xy * r1.ww;
              r1.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r5.xy, r5.z).x;
              r6.xy = -r3.zw * r0.zz;
              r6.z = 0;
              r6.xyz = r6.xyz + r5.xyz;
              r2.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r6.xy, r6.z).x;
              r2.w = scene.MiscParameters5.y * r2.w;
              r1.w = r1.w * scene.MiscParameters5.x + r2.w;
              r6.x = r3.z * r0.z;
              r6.y = -r3.w * r0.z;
              r6.z = 0;
              r6.xyz = r6.xyz + r5.xyz;
              r2.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r6.xy, r6.z).x;
              r1.w = r2.w * scene.MiscParameters5.y + r1.w;
              r6.x = -r3.z * r0.z;
              r6.y = r3.w * r0.z;
              r6.z = 0;
              r6.xyz = r6.xyz + r5.xyz;
              r2.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r6.xy, r6.z).x;
              r1.w = r2.w * scene.MiscParameters5.y + r1.w;
              r6.xy = r3.zw * r0.zz;
              r6.z = 0;
              r6.xyz = r6.xyz + r5.xyz;
              r0.z = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r6.xy, r6.z).x;
              r0.w = r0.z * scene.MiscParameters5.y + r1.w;
            } else {
              r0.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r5.xy, r5.z).x;
            }
          }
        }
      }
    } else {
      r4.w = 1;
      r5.x = dot(r4.xyzw, LightShadow0.m_split1Transform._m00_m10_m20_m30);
      r5.y = dot(r4.xyzw, LightShadow0.m_split1Transform._m01_m11_m21_m31);
      r5.z = dot(r4.xyzw, LightShadow0.m_split1Transform._m02_m12_m22_m32);
      r0.z = 10 / LightShadow0.m_splitDistances.y;
      r1.w = (int)scene.DuranteSettings.x & 16;
      if (r1.w != 0) {
        r3.zw = float2(0.000937499979,0.00187499996) * r0.zz;
        r1.w = -6 + r5.z;
        r3.zw = r3.zw * r1.ww;
        r3.zw = r3.zw / r5.zz;
        r1.w = dot(v0.xy, float2(0.0671105608,0.00583714992));
        r1.w = frac(r1.w);
        r1.w = 52.9829178 * r1.w;
        r1.w = frac(r1.w);
        r1.w = 6.28318548 * r1.w;
        r6.xy = float2(0,0);
        r2.w = 0;
        while (true) {
          r5.w = cmp((int)r2.w >= 16);
          if (r5.w != 0) break;
          r5.w = (int)r2.w;
          r6.z = 0.5 + r5.w;
          r6.z = sqrt(r6.z);
          r6.z = 0.25 * r6.z;
          r5.w = r5.w * 2.4000001 + r1.w;
          sincos(r5.w, r7.x, r8.x);
          r8.x = r8.x * r6.z;
          r8.y = r7.x * r6.z;
          r6.zw = r8.xy * r3.zw + r5.xy;
          r5.w = LightShadowMap0.SampleLevel(PointClampSamplerState_s, r6.zw, 0).x;
          r6.z = cmp(r5.w < r5.z);
          r7.y = r6.y + r5.w;
          r7.x = 1 + r6.x;
          r6.xy = r6.zz ? r7.xy : r6.xy;
          r2.w = (int)r2.w + 1;
        }
        r2.w = cmp(r6.x >= 1);
        if (r2.w != 0) {
          r3.zw = float2(0.000624999986,0.00124999997) * r0.zz;
          r2.w = r6.y / r6.x;
          r2.w = r5.z + -r2.w;
          r2.w = min(0.0700000003, r2.w);
          r2.w = 60 * r2.w;
          r3.zw = r2.ww * r3.zw;
          LightShadowMap0.GetDimensions(0, fDest.x, fDest.y, fDest.z);
          r6.xy = fDest.xy;
          r7.x = 0.5 / r6.x;
          r7.y = 1 / r6.y;
          r3.zw = max(r7.xy, r3.zw);
          r2.w = 0;
          r5.w = 0;
          while (true) {
            r6.x = cmp((int)r5.w >= 16);
            if (r6.x != 0) break;
            r6.x = (int)r5.w;
            r6.y = 0.5 + r6.x;
            r6.y = sqrt(r6.y);
            r6.y = 0.25 * r6.y;
            r6.x = r6.x * 2.4000001 + r1.w;
            sincos(r6.x, r6.x, r7.x);
            r7.x = r7.x * r6.y;
            r7.y = r6.y * r6.x;
            r6.xy = r7.xy * r3.zw;
            r6.xy = r6.xy * float2(3,3) + r5.xy;
            r6.x = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r6.xy, r5.z).x;
            r2.w = r6.x + r2.w;
            r5.w = (int)r5.w + 1;
          }
          r0.w = 0.0625 * r2.w;
        } else {
          r0.w = 1;
        }
      } else {
        r1.w = (int)scene.DuranteSettings.x & 8;
        if (r1.w != 0) {
          r3.zw = float2(0.000375000003,0.000750000007) * r0.zz;
          r1.w = dot(v0.xy, float2(0.0671105608,0.00583714992));
          r1.w = frac(r1.w);
          r1.w = 52.9829178 * r1.w;
          r1.w = frac(r1.w);
          r1.w = 6.28318548 * r1.w;
          r2.w = 0;
          r5.w = 0;
          while (true) {
            r6.x = cmp((int)r5.w >= 16);
            if (r6.x != 0) break;
            r6.x = (int)r5.w;
            r6.y = 0.5 + r6.x;
            r6.y = sqrt(r6.y);
            r6.y = 0.25 * r6.y;
            r6.x = r6.x * 2.4000001 + r1.w;
            sincos(r6.x, r6.x, r7.x);
            r7.x = r7.x * r6.y;
            r7.y = r6.y * r6.x;
            r6.xy = r7.xy * r3.zw;
            r6.xy = r6.xy * float2(3,3) + r5.xy;
            r6.x = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r6.xy, r5.z).x;
            r2.w = r6.x + r2.w;
            r5.w = (int)r5.w + 1;
          }
          r0.w = 0.0625 * r2.w;
        } else {
          r1.w = dot(r4.xyzw, LightShadow0.m_split1Transform._m03_m13_m23_m33);
          r2.w = (int)scene.DuranteSettings.x & 4;
          if (r2.w != 0) {
            r3.zw = scene.MiscParameters4.xy * r1.ww;
            r2.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r5.xy, r5.z).x;
            r4.xy = -r3.zw * r0.zz;
            r4.z = 0;
            r4.xyz = r5.xyz + r4.xyz;
            r4.x = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r4.xy, r4.z).x;
            r4.x = 0.0625 * r4.x;
            r2.w = r2.w * 0.5 + r4.x;
            r4.x = r3.z * r0.z;
            r4.y = -r3.w * r0.z;
            r4.z = 0;
            r6.xyz = r5.xyz + r4.xyz;
            r4.x = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r6.xy, r6.z).x;
            r2.w = r4.x * 0.0625 + r2.w;
            r6.x = -r3.z * r0.z;
            r6.y = r3.w * r0.z;
            r6.z = 0;
            r7.xyz = r6.xyz + r5.xyz;
            r4.x = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r7.xy, r7.z).x;
            r2.w = r4.x * 0.0625 + r2.w;
            r7.xy = r3.zw * r0.zz;
            r7.z = 0;
            r8.xyz = r7.xyz + r5.xyz;
            r3.z = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r8.xy, r8.z).x;
            r2.w = r3.z * 0.0625 + r2.w;
            r4.xyz = r5.xyz + r4.zyz;
            r3.z = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r4.xy, r4.z).x;
            r2.w = r3.z * 0.0625 + r2.w;
            r4.xyz = r6.xzz + r5.xyz;
            r3.z = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r4.xy, r4.z).x;
            r2.w = r3.z * 0.0625 + r2.w;
            r4.xyz = r7.xzz + r5.xyz;
            r3.z = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r4.xy, r4.z).x;
            r2.w = r3.z * 0.0625 + r2.w;
            r4.xyz = r7.zyz + r5.xyz;
            r3.z = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r4.xy, r4.z).x;
            r0.w = r3.z * 0.0625 + r2.w;
          } else {
            r2.w = (int)scene.DuranteSettings.x & 2;
            if (r2.w != 0) {
              r3.zw = scene.MiscParameters4.xy * r1.ww;
              r1.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r5.xy, r5.z).x;
              r4.xy = -r3.zw * r0.zz;
              r4.z = 0;
              r4.xyz = r5.xyz + r4.xyz;
              r2.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r4.xy, r4.z).x;
              r2.w = scene.MiscParameters5.y * r2.w;
              r1.w = r1.w * scene.MiscParameters5.x + r2.w;
              r4.x = r3.z * r0.z;
              r4.y = -r3.w * r0.z;
              r4.z = 0;
              r4.xyz = r5.xyz + r4.xyz;
              r2.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r4.xy, r4.z).x;
              r1.w = r2.w * scene.MiscParameters5.y + r1.w;
              r4.x = -r3.z * r0.z;
              r4.y = r3.w * r0.z;
              r4.z = 0;
              r4.xyz = r5.xyz + r4.xyz;
              r2.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r4.xy, r4.z).x;
              r1.w = r2.w * scene.MiscParameters5.y + r1.w;
              r4.xy = r3.zw * r0.zz;
              r4.z = 0;
              r4.xyz = r5.xyz + r4.xyz;
              r0.z = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r4.xy, r4.z).x;
              r0.w = r0.z * scene.MiscParameters5.y + r1.w;
            } else {
              r0.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r5.xy, r5.z).x;
            }
          }
        }
      }
    }
  } else {
    r0.w = 1;
  }
  r0.z = saturate(v4.w / LightShadow0.m_splitDistances.y);
  r1.w = r0.z * r0.z;
  r2.w = -scene.MiscParameters2.y + v4.y;
  r2.w = scene.MiscParameters2.x * abs(r2.w);
  r2.w = min(1, r2.w);
  r3.z = r2.w * r2.w;
  r2.w = r3.z * r2.w;
  r3.z = cmp(0 >= scene.MiscParameters2.x);
  r2.w = r3.z ? 0 : r2.w;
  r0.z = r0.z * r1.w + r2.w;
  r1.w = dot(v5.xyz, Light0.m_direction.xyz);
  r1.w = r1.w * 0.5 + 0.5;
  r2.w = 0.400000006 * scene.MiscParameters4.w;
  r3.z = max(0, -v5.y);
  r2.w = -r2.w * r3.z + 0.400000006;
  r3.z = cmp(r2.w < r1.w);
  r1.w = -r2.w + r1.w;
  r2.w = 1 + -r2.w;
  r1.w = r1.w / r2.w;
  r1.w = r3.z ? r1.w : 0;
  r1.w = 1 + -r1.w;
  r2.w = r1.w * r1.w;
  r0.z = r1.w * r2.w + r0.z;
  r0.z = min(1, r0.z);
  r0.z = r0.w + r0.z;
  r0.z = min(1, r0.z);
  r0.z = 1 + r0.z;
  r0.w = 0.5 * r0.z;
  r0.z = -r0.z * 0.5 + 1;
  r1.w = 1 + -scene.MiscParameters2.w;
  r0.z = r0.z * r1.w + r0.w;
  r0.w = 1 + -r0.z;
  r1.w = 1 + -GameMaterialEmission.w;
  r0.z = r0.w * r1.w + r0.z;
  r0.w = r0.z * r0.z;
  r4.xyz = Light0.m_colorIntensity.xyz * r0.www;
  r5.xyz = NormalMapSampler.Sample(NormalMapSamplerSampler_s, r0.xy).xyz;
  r5.xyz = r5.xyz * float3(2,2,2) + float3(-1,-1,-1);
  r0.y = dot(v7.xyz, v7.xyz);
  r0.y = rsqrt(r0.y);
  r6.xyz = v7.xyz * r0.yyy;
  r0.y = dot(v5.xyz, v5.xyz);
  r0.y = rsqrt(r0.y);
  r7.xyz = v5.xyz * r0.yyy;
  r8.xyz = r7.zxy * r6.yzx;
  r8.xyz = r7.yzx * r6.zxy + -r8.xyz;
  r0.x = cmp(r0.x < 0);
  r0.x = r0.x ? -1 : 1;
  r0.x = r5.x * r0.x;
  r5.xyw = r8.xyz * r5.yyy;
  r0.xyw = r0.xxx * r6.xyz + r5.xyw;
  r0.xyw = r5.zzz * r7.xyz + r0.xyw;
  r1.w = dot(r0.xyw, r0.xyw);
  r1.w = rsqrt(r1.w);
  r0.xyw = r1.www * r0.xyw;
  r5.xyz = scene.EyePosition.xyz + -v4.xyz;
  r1.w = dot(r5.xyz, r5.xyz);
  r1.w = rsqrt(r1.w);
  r6.xyz = r5.xyz * r1.www;
  r2.w = saturate(dot(r0.xyw, r6.xyz));
  r3.z = dot(Light0.m_direction.xyz, r0.xyw);
  r3.z = r3.z * 0.5 + 0.5;
  r3.z = r3.z * r3.z;
  r6.xyz = SpecularColor.xyz * Light0.m_colorIntensity.xyz;
  r5.xyz = r5.xyz * r1.www + Light0.m_direction.xyz;
  r1.w = dot(r5.xyz, r5.xyz);
  r1.w = rsqrt(r1.w);
  r5.xyz = r5.xyz * r1.www;
  r1.w = saturate(dot(r0.xyw, r5.xyz));
  // r1.w = log2(r1.w);
  // r1.w = SpecularPower * r1.w;
  // r1.w = exp2(r1.w);
  r1.w = renodx::math::SafePow(r1.w, SpecularPower);
  r1.w = min(1, r1.w);
  r1.w = Shininess * r1.w;
  r5.xyz = Light0.m_colorIntensity.xyz * r3.zzz + scene.GlobalAmbientColor.xyz;
  r5.xyz = min(float3(1.5,1.5,1.5), r5.xyz);
  r5.xyz = r6.xyz * r1.www + r5.xyz;
  r0.z = 1 + -r0.z;
  r6.xyz = r5.xyz * scene.MiscParameters1.xyz + -r5.xyz;
  r5.xyz = r0.zzz * r6.xyz + r5.xyz;
  r0.z = 1 + -r2.w;
  // r0.z = log2(r0.z);
  // r1.w = RimLitPower * r0.z;
  // r1.w = exp2(r1.w);
  r1.w = renodx::math::SafePow(r0.z, RimLitPower);
  r1.w = RimLitIntensity * r1.w;
  r6.xyz = RimLitColor.xyz * r1.www;
  r5.xyz = r6.xyz * r4.xyz + r5.xyz;
  r5.xyz = min(RimLightClampFactor, r5.xyz);
  r3.xy = r3.xy * v0.ww + -r1.xy;
  r1.xy = scene.AdditionalShadowOffset * r3.xy + r1.xy;
  r1.xy = r1.xy / v0.ww;
  r1.xyw = RefractionTexture.Sample(LinearClampSamplerState_s, r1.xy).xyz;
  r2.xyz = r2.xyz + -r1.xyw;
  r1.xyz = r1.zzz * r2.xyz + r1.xyw;
  r2.xyz = r4.xyz + r4.xyz;
  r2.xyz = max(float3(1,1,1), r2.xyz);
  r3.xyz = min(float3(1,1,1), r5.xyz);
  r3.xyz = float3(1,1,1) + -r3.xyz;
  r3.xyz = ShadowColorShift.xyz * r3.xyz;
  r2.xyz = r3.xyz * r2.xyz + r5.xyz;
  r2.xyz = v1.xyz * r2.xyz;
  r1.xyz = r2.xyz * r1.xyz;
  r0.z = PointLightColor.x * r0.z;
  r0.z = exp2(r0.z);
  r0.z = -1 + r0.z;
  r0.z = PointLightColor.y * r0.z + 1;
  r2.xyz = GameMaterialEmission.xyz * r0.zzz;
  r1.xyz = r1.xyz * GameMaterialDiffuse.xyz + r2.xyz;
  // r0.z = dot(r1.xyz, float3(0.298999995, 0.587000012, 0.114));
  r0.z = calculateLuminanceSRGB(r1.xyz);
  r2.xyz = r0.zzz * scene.MonotoneMul.xyz + scene.MonotoneAdd.xyz;
  r2.xyz = r2.xyz + -r1.xyz;
  r1.xyz = GameMaterialMonotone * r2.xyz + r1.xyz;
  r2.xyz = BloomIntensity * r1.xyz;
  // r0.z = dot(r2.xyz, float3(0.298999995, 0.587000012, 0.114));
  r0.z = calculateLuminanceSRGB(r2.xyz);
  r0.z = -scene.MiscParameters2.z + r0.z;
  r0.z = max(0, r0.z);
  r0.z = 0.5 * r0.z;
  r0.z = min(1, r0.z);
  o0.w = PointLightParams.z * r0.z;
  o1.xyz = r0.xyw * float3(0.5,0.5,0.5) + float3(0.5,0.5,0.5);
  o1.w = 0.466666698 + MaskEps;
  r0.x = v9.z / v9.w;
  r0.y = 256 * r0.x;
  r2.x = trunc(r0.y);
  r0.x = r0.x * 256 + -r2.x;
  r0.y = 256 * r0.x;
  r2.y = trunc(r0.y);
  r2.z = r0.x * 256 + -r2.y;
  o2.xyz = float3(0.00390625,0.00390625,1) * r2.xyz;
  o0.xyz = r1.xyz;
  o2.w = MaskEps;

  o0.rgb = clamp(o0.rgb, 0.f, shader_injection.safe_clamp);;
  return;
}