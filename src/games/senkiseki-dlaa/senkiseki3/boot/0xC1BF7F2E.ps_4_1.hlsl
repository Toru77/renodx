// ---- Created with 3Dmigoto v1.4.1 on Wed Aug  5 19:10:02 2026

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

  float GameMaterialID : packoffset(c60) = {0};
  float4 GameMaterialDiffuse : packoffset(c61) = {1,1,1,1};
  float4 GameMaterialEmission : packoffset(c62) = {0,0,0,0};
  float GameMaterialMonotone : packoffset(c63) = {0};
  float4 GameMaterialTexcoord : packoffset(c64) = {0,0,1,1};
  float4 GameDitherParams : packoffset(c65) = {0,0,0,0};
  float4 UVaMUvColor : packoffset(c66) = {1,1,1,1};
  float4 UVaProjTexcoord : packoffset(c67) = {0,0,1,1};
  float4 UVaMUvTexcoord : packoffset(c68) = {0,0,1,1};
  float4 UVaMUv2Texcoord : packoffset(c69) = {0,0,1,1};
  float4 UVaDuDvTexcoord : packoffset(c70) = {0,0,1,1};
  float AlphaThreshold : packoffset(c71) = {0.5};
  float Shininess : packoffset(c71.y) = {0.5};
  float SpecularPower : packoffset(c71.z) = {50};
  float BloomIntensity : packoffset(c71.w) = {1};
  float4 PointLightParams : packoffset(c72) = {0,0,0,0};
  float4 PointLightColor : packoffset(c73) = {0,0,0,0};
}

SamplerState PointClampSamplerState_s : register(s0);
SamplerState DiffuseMapSamplerSampler_s : register(s2);
SamplerState SpecularMapSamplerSampler_s : register(s3);
SamplerState DiffuseMap2SamplerSampler_s : register(s4);
SamplerComparisonState LinearClampCmpSamplerState_s : register(s1);
Texture2D<float4> LightShadowMap0 : register(t0);
Texture2D<float4> DiffuseMapSampler : register(t1);
Texture2D<float4> SpecularMapSampler : register(t2);
Texture2D<float4> DiffuseMap2Sampler : register(t3);


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
  out float4 o0 : SV_TARGET0,
  out float4 o1 : SV_TARGET1)
{
  float4 r0,r1,r2,r3,r4,r5,r6,r7;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.xyzw = DiffuseMapSampler.Sample(DiffuseMapSamplerSampler_s, v3.xy).xyzw;
  r1.x = v1.w * r0.w;
  r0.w = r0.w * v1.w + -0.00400000019;
  r0.w = cmp(r0.w < 0);
  o1 = float4(1,1,1,1);  // DLAA: effect/particle mask
  if (r0.w != 0) discard;
  r2.xyzw = DiffuseMap2Sampler.Sample(DiffuseMap2SamplerSampler_s, w3.xy).xyzw;
  r2.xyzw = UVaMUvColor.xyzw * r2.xyzw;
  r0.w = v1.w * r2.w;
  r0.xyz = r2.xyz * r0.www + r0.xyz;
  r0.w = cmp(LightShadow0.m_splitDistances.y >= v4.w);
  if (r0.w != 0) {
    r1.yzw = Light0.m_direction.xyz * float3(0.0500000007,0.0500000007,0.0500000007) + v4.xyz;
    r2.xyz = v5.xyz * float3(0.0500000007,0.0500000007,0.0500000007) + r1.yzw;
    r1.y = cmp(v4.w < LightShadow0.m_splitDistances.x);
    if (r1.y != 0) {
      r2.w = 1;
      r3.x = dot(r2.xyzw, LightShadow0.m_split0Transform._m00_m10_m20_m30);
      r3.y = dot(r2.xyzw, LightShadow0.m_split0Transform._m01_m11_m21_m31);
      r3.z = dot(r2.xyzw, LightShadow0.m_split0Transform._m02_m12_m22_m32);
      r1.y = (int)DuranteSettings.x & 2;
      if (r1.y != 0) {
        r1.y = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r3.xy, r3.z).x;
      } else {
        r1.z = 25 / LightShadow0.m_splitDistances.y;
        r1.w = (int)DuranteSettings.x & 16;
        if (r1.w != 0) {
          r4.xy = float2(0.000937499979,0.00187499996) * r1.zz;
          r1.w = -6 + r3.z;
          r4.xy = r4.xy * r1.ww;
          r4.xy = r4.xy / r3.zz;
          r1.w = dot(v0.xy, float2(0.0671105608,0.00583714992));
          r1.w = frac(r1.w);
          r1.w = 52.9829178 * r1.w;
          r1.w = frac(r1.w);
          r1.w = 6.28318548 * r1.w;
          r4.zw = float2(0,0);
          r3.w = 0;
          while (true) {
            r5.x = cmp((int)r3.w >= 16);
            if (r5.x != 0) break;
            r5.x = (int)r3.w;
            r5.y = 0.5 + r5.x;
            r5.y = sqrt(r5.y);
            r5.y = 0.25 * r5.y;
            r5.x = r5.x * 2.4000001 + r1.w;
            sincos(r5.x, r5.x, r6.x);
            r6.x = r6.x * r5.y;
            r6.y = r5.y * r5.x;
            r5.xy = r6.xy * r4.xy + r3.xy;
            r5.x = LightShadowMap0.SampleLevel(PointClampSamplerState_s, r5.xy, 0).x;
            r5.y = cmp(r5.x < r3.z);
            r6.y = r5.x + r4.w;
            r6.x = 1 + r4.z;
            r4.zw = r5.yy ? r6.xy : r4.zw;
            r3.w = (int)r3.w + 1;
          }
          r3.w = cmp(r4.z >= 1);
          if (r3.w != 0) {
            r4.xy = float2(0.000624999986,0.00124999997) * r1.zz;
            r3.w = r4.w / r4.z;
            r3.w = r3.z + -r3.w;
            r3.w = min(0.0700000003, r3.w);
            r3.w = 60 * r3.w;
            r4.xy = r3.ww * r4.xy;
            LightShadowMap0.GetDimensions(0, fDest.x, fDest.y, fDest.z);
            r4.zw = fDest.xy;
            r4.zw = float2(0.5,0.5) / r4.zw;
            r4.xy = max(r4.zw, r4.xy);
            r3.w = 0;
            r4.z = 0;
            while (true) {
              r4.w = cmp((int)r4.z >= 16);
              if (r4.w != 0) break;
              r4.w = (int)r4.z;
              r5.x = 0.5 + r4.w;
              r5.x = sqrt(r5.x);
              r5.x = 0.25 * r5.x;
              r4.w = r4.w * 2.4000001 + r1.w;
              sincos(r4.w, r6.x, r7.x);
              r7.x = r7.x * r5.x;
              r7.y = r6.x * r5.x;
              r5.xy = r7.xy * r4.xy;
              r5.xy = r5.xy * float2(3,3) + r3.xy;
              r4.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r5.xy, r3.z).x;
              r3.w = r4.w + r3.w;
              r4.z = (int)r4.z + 1;
            }
            r1.y = 0.0625 * r3.w;
          } else {
            r1.y = 1;
          }
        } else {
          r1.w = (int)DuranteSettings.x & 8;
          if (r1.w != 0) {
            r4.xy = float2(0.000375000003,0.000750000007) * r1.zz;
            r1.w = dot(v0.xy, float2(0.0671105608,0.00583714992));
            r1.w = frac(r1.w);
            r1.w = 52.9829178 * r1.w;
            r1.w = frac(r1.w);
            r1.w = 6.28318548 * r1.w;
            r3.w = 0;
            r4.z = 0;
            while (true) {
              r4.w = cmp((int)r4.z >= 16);
              if (r4.w != 0) break;
              r4.w = (int)r4.z;
              r5.x = 0.5 + r4.w;
              r5.x = sqrt(r5.x);
              r5.x = 0.25 * r5.x;
              r4.w = r4.w * 2.4000001 + r1.w;
              sincos(r4.w, r6.x, r7.x);
              r7.x = r7.x * r5.x;
              r7.y = r6.x * r5.x;
              r5.xy = r7.xy * r4.xy;
              r5.xy = r5.xy * float2(3,3) + r3.xy;
              r4.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r5.xy, r3.z).x;
              r3.w = r4.w + r3.w;
              r4.z = (int)r4.z + 1;
            }
            r1.y = 0.0625 * r3.w;
          } else {
            r1.w = dot(r2.xyzw, LightShadow0.m_split0Transform._m03_m13_m23_m33);
            r3.w = (int)DuranteSettings.x & 4;
            if (r3.w != 0) {
              r4.xy = scene.MiscParameters4.xy * r1.ww;
              r3.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r3.xy, r3.z).x;
              r5.xy = -r4.xy * r1.zz;
              r5.z = 0;
              r5.xyz = r5.xyz + r3.xyz;
              r4.z = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r5.xy, r5.z).x;
              r4.z = 0.0625 * r4.z;
              r3.w = r3.w * 0.5 + r4.z;
              r5.x = r4.x * r1.z;
              r5.y = -r4.y * r1.z;
              r5.z = 0;
              r6.xyz = r5.xyz + r3.xyz;
              r4.z = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r6.xy, r6.z).x;
              r3.w = r4.z * 0.0625 + r3.w;
              r6.x = -r4.x * r1.z;
              r6.y = r4.y * r1.z;
              r6.z = 0;
              r7.xyz = r6.xyz + r3.xyz;
              r4.z = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r7.xy, r7.z).x;
              r3.w = r4.z * 0.0625 + r3.w;
              r4.xy = r4.xy * r1.zz;
              r4.z = 0;
              r7.xyz = r4.xyz + r3.xyz;
              r4.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r7.xy, r7.z).x;
              r3.w = r4.w * 0.0625 + r3.w;
              r5.xyz = r5.zyz + r3.xyz;
              r4.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r5.xy, r5.z).x;
              r3.w = r4.w * 0.0625 + r3.w;
              r5.xyz = r6.xzz + r3.xyz;
              r4.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r5.xy, r5.z).x;
              r3.w = r4.w * 0.0625 + r3.w;
              r5.xyz = r4.xzz + r3.xyz;
              r4.x = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r5.xy, r5.z).x;
              r3.w = r4.x * 0.0625 + r3.w;
              r4.xyz = r4.zyz + r3.xyz;
              r4.x = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r4.xy, r4.z).x;
              r1.y = r4.x * 0.0625 + r3.w;
            } else {
              r4.xy = scene.MiscParameters4.xy * r1.ww;
              r1.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r3.xy, r3.z).x;
              r5.xy = -r4.xy * r1.zz;
              r5.z = 0;
              r5.xyz = r5.xyz + r3.xyz;
              r3.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r5.xy, r5.z).x;
              r3.w = scene.MiscParameters5.y * r3.w;
              r1.w = r1.w * scene.MiscParameters5.x + r3.w;
              r5.x = r4.x * r1.z;
              r5.y = -r4.y * r1.z;
              r5.z = 0;
              r5.xyz = r5.xyz + r3.xyz;
              r3.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r5.xy, r5.z).x;
              r1.w = r3.w * scene.MiscParameters5.y + r1.w;
              r5.x = -r4.x * r1.z;
              r5.y = r4.y * r1.z;
              r5.z = 0;
              r5.xyz = r5.xyz + r3.xyz;
              r3.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r5.xy, r5.z).x;
              r1.w = r3.w * scene.MiscParameters5.y + r1.w;
              r4.xy = r4.xy * r1.zz;
              r4.z = 0;
              r3.xyz = r4.xyz + r3.xyz;
              r1.z = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r3.xy, r3.z).x;
              r1.y = r1.z * scene.MiscParameters5.y + r1.w;
            }
          }
        }
      }
    } else {
      r2.w = 1;
      r3.x = dot(r2.xyzw, LightShadow0.m_split1Transform._m00_m10_m20_m30);
      r3.y = dot(r2.xyzw, LightShadow0.m_split1Transform._m01_m11_m21_m31);
      r3.z = dot(r2.xyzw, LightShadow0.m_split1Transform._m02_m12_m22_m32);
      r1.z = (int)DuranteSettings.x & 2;
      if (r1.z != 0) {
        r1.y = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r3.xy, r3.z).x;
      } else {
        r1.z = 10 / LightShadow0.m_splitDistances.y;
        r1.w = (int)DuranteSettings.x & 16;
        if (r1.w != 0) {
          r4.xy = float2(0.000937499979,0.00187499996) * r1.zz;
          r1.w = -6 + r3.z;
          r4.xy = r4.xy * r1.ww;
          r4.xy = r4.xy / r3.zz;
          r1.w = dot(v0.xy, float2(0.0671105608,0.00583714992));
          r1.w = frac(r1.w);
          r1.w = 52.9829178 * r1.w;
          r1.w = frac(r1.w);
          r1.w = 6.28318548 * r1.w;
          r4.zw = float2(0,0);
          r3.w = 0;
          while (true) {
            r5.x = cmp((int)r3.w >= 16);
            if (r5.x != 0) break;
            r5.x = (int)r3.w;
            r5.y = 0.5 + r5.x;
            r5.y = sqrt(r5.y);
            r5.y = 0.25 * r5.y;
            r5.x = r5.x * 2.4000001 + r1.w;
            sincos(r5.x, r5.x, r6.x);
            r6.x = r6.x * r5.y;
            r6.y = r5.y * r5.x;
            r5.xy = r6.xy * r4.xy + r3.xy;
            r5.x = LightShadowMap0.SampleLevel(PointClampSamplerState_s, r5.xy, 0).x;
            r5.y = cmp(r5.x < r3.z);
            r6.y = r5.x + r4.w;
            r6.x = 1 + r4.z;
            r4.zw = r5.yy ? r6.xy : r4.zw;
            r3.w = (int)r3.w + 1;
          }
          r3.w = cmp(r4.z >= 1);
          if (r3.w != 0) {
            r4.xy = float2(0.000624999986,0.00124999997) * r1.zz;
            r3.w = r4.w / r4.z;
            r3.w = r3.z + -r3.w;
            r3.w = min(0.0700000003, r3.w);
            r3.w = 60 * r3.w;
            r4.xy = r3.ww * r4.xy;
            LightShadowMap0.GetDimensions(0, fDest.x, fDest.y, fDest.z);
            r4.zw = fDest.xy;
            r4.zw = float2(0.5,0.5) / r4.zw;
            r4.xy = max(r4.zw, r4.xy);
            r3.w = 0;
            r4.z = 0;
            while (true) {
              r4.w = cmp((int)r4.z >= 16);
              if (r4.w != 0) break;
              r4.w = (int)r4.z;
              r5.x = 0.5 + r4.w;
              r5.x = sqrt(r5.x);
              r5.x = 0.25 * r5.x;
              r4.w = r4.w * 2.4000001 + r1.w;
              sincos(r4.w, r6.x, r7.x);
              r7.x = r7.x * r5.x;
              r7.y = r6.x * r5.x;
              r5.xy = r7.xy * r4.xy;
              r5.xy = r5.xy * float2(3,3) + r3.xy;
              r4.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r5.xy, r3.z).x;
              r3.w = r4.w + r3.w;
              r4.z = (int)r4.z + 1;
            }
            r1.y = 0.0625 * r3.w;
          } else {
            r1.y = 1;
          }
        } else {
          r1.w = (int)DuranteSettings.x & 8;
          if (r1.w != 0) {
            r4.xy = float2(0.000375000003,0.000750000007) * r1.zz;
            r1.w = dot(v0.xy, float2(0.0671105608,0.00583714992));
            r1.w = frac(r1.w);
            r1.w = 52.9829178 * r1.w;
            r1.w = frac(r1.w);
            r1.w = 6.28318548 * r1.w;
            r3.w = 0;
            r4.z = 0;
            while (true) {
              r4.w = cmp((int)r4.z >= 16);
              if (r4.w != 0) break;
              r4.w = (int)r4.z;
              r5.x = 0.5 + r4.w;
              r5.x = sqrt(r5.x);
              r5.x = 0.25 * r5.x;
              r4.w = r4.w * 2.4000001 + r1.w;
              sincos(r4.w, r6.x, r7.x);
              r7.x = r7.x * r5.x;
              r7.y = r6.x * r5.x;
              r5.xy = r7.xy * r4.xy;
              r5.xy = r5.xy * float2(3,3) + r3.xy;
              r4.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r5.xy, r3.z).x;
              r3.w = r4.w + r3.w;
              r4.z = (int)r4.z + 1;
            }
            r1.y = 0.0625 * r3.w;
          } else {
            r1.w = dot(r2.xyzw, LightShadow0.m_split1Transform._m03_m13_m23_m33);
            r2.x = (int)DuranteSettings.x & 4;
            if (r2.x != 0) {
              r2.xy = scene.MiscParameters4.xy * r1.ww;
              r2.z = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r3.xy, r3.z).x;
              r4.xy = -r2.xy * r1.zz;
              r4.z = 0;
              r4.xyz = r4.xyz + r3.xyz;
              r2.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r4.xy, r4.z).x;
              r2.w = 0.0625 * r2.w;
              r2.z = r2.z * 0.5 + r2.w;
              r4.x = r2.x * r1.z;
              r4.y = -r2.y * r1.z;
              r4.z = 0;
              r5.xyz = r4.xyz + r3.xyz;
              r2.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r5.xy, r5.z).x;
              r2.z = r2.w * 0.0625 + r2.z;
              r5.x = -r2.x * r1.z;
              r5.y = r2.y * r1.z;
              r5.z = 0;
              r6.xyz = r5.xyz + r3.xyz;
              r2.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r6.xy, r6.z).x;
              r2.z = r2.w * 0.0625 + r2.z;
              r6.xy = r2.xy * r1.zz;
              r6.z = 0;
              r2.xyw = r6.xyz + r3.xyz;
              r2.x = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r2.xy, r2.w).x;
              r2.x = r2.x * 0.0625 + r2.z;
              r2.yzw = r4.zyz + r3.xyz;
              r2.y = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r2.yz, r2.w).x;
              r2.x = r2.y * 0.0625 + r2.x;
              r2.yzw = r5.xzz + r3.xyz;
              r2.y = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r2.yz, r2.w).x;
              r2.x = r2.y * 0.0625 + r2.x;
              r2.yzw = r6.xzz + r3.xyz;
              r2.y = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r2.yz, r2.w).x;
              r2.x = r2.y * 0.0625 + r2.x;
              r2.yzw = r5.zyz + r3.xyz;
              r2.y = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r2.yz, r2.w).x;
              r1.y = r2.y * 0.0625 + r2.x;
            } else {
              r2.xy = scene.MiscParameters4.xy * r1.ww;
              r1.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r3.xy, r3.z).x;
              r4.xy = -r2.xy * r1.zz;
              r4.z = 0;
              r4.xyz = r4.xyz + r3.xyz;
              r2.z = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r4.xy, r4.z).x;
              r2.z = scene.MiscParameters5.y * r2.z;
              r1.w = r1.w * scene.MiscParameters5.x + r2.z;
              r4.x = r2.x * r1.z;
              r4.y = -r2.y * r1.z;
              r4.z = 0;
              r4.xyz = r4.xyz + r3.xyz;
              r2.z = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r4.xy, r4.z).x;
              r1.w = r2.z * scene.MiscParameters5.y + r1.w;
              r4.x = -r2.x * r1.z;
              r4.y = r2.y * r1.z;
              r4.z = 0;
              r4.xyz = r4.xyz + r3.xyz;
              r2.z = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r4.xy, r4.z).x;
              r1.w = r2.z * scene.MiscParameters5.y + r1.w;
              r2.xy = r2.xy * r1.zz;
              r2.z = 0;
              r2.xyz = r3.xyz + r2.xyz;
              r1.z = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r2.xy, r2.z).x;
              r1.y = r1.z * scene.MiscParameters5.y + r1.w;
            }
          }
        }
      }
    }
  } else {
    r1.y = 1;
  }
  r1.z = cmp(v4.w < LightShadow0.m_splitDistances.x);
  r1.w = -LightShadow0.m_splitDistances.x + v4.w;
  r2.x = LightShadow0.m_splitDistances.y + -LightShadow0.m_splitDistances.x;
  r1.w = saturate(r1.w / r2.x);
  r1.z = r1.z ? 0 : r1.w;
  r1.w = r1.z * r1.z;
  r1.z = r1.z * r1.w;
  r0.w = r0.w ? r1.z : 1;
  r1.z = -scene.MiscParameters2.y + v4.y;
  r1.z = scene.MiscParameters2.x * abs(r1.z);
  r1.z = min(1, r1.z);
  r1.w = r1.z * r1.z;
  r1.z = r1.z * r1.w;
  r1.w = cmp(0 >= scene.MiscParameters2.x);
  r1.z = r1.w ? 0 : r1.z;
  r0.w = r1.z + r0.w;
  r1.z = dot(v5.xyz, Light0.m_direction.xyz);
  r1.z = r1.z * 0.5 + 0.5;
  r1.w = 0.400000006 * scene.MiscParameters4.w;
  r2.x = max(0, -v5.y);
  r1.w = -r1.w * r2.x + 0.400000006;
  r2.x = cmp(r1.w < r1.z);
  r1.z = r1.z + -r1.w;
  r1.w = 1 + -r1.w;
  r1.z = r1.z / r1.w;
  r1.z = r2.x ? r1.z : 0;
  r1.z = 1 + -r1.z;
  r1.w = r1.z * r1.z;
  r0.w = r1.z * r1.w + r0.w;
  r0.w = min(1, r0.w);
  r0.w = r1.y + r0.w;
  r0.w = min(1, r0.w);
  r1.y = 1 + -r0.w;
  r1.z = 1 + -scene.MiscParameters2.w;
  r0.w = r1.y * r1.z + r0.w;
  r1.y = 1 + -r0.w;
  r1.z = 1 + -GameMaterialEmission.w;
  r0.w = r1.y * r1.z + r0.w;
  r1.y = SpecularMapSampler.Sample(SpecularMapSamplerSampler_s, v3.xy).x;
  r1.z = dot(v5.xyz, v5.xyz);
  r1.z = rsqrt(r1.z);
  r2.xyz = v5.xyz * r1.zzz;
  r3.xyz = scene.EyePosition.xyz + -v4.xyz;
  r1.z = dot(r3.xyz, r3.xyz);
  r1.z = rsqrt(r1.z);
  r1.w = dot(Light0.m_direction.xyz, r2.xyz);
  r1.w = r1.w * 0.5 + 0.5;
  r1.w = r1.w * r1.w;
  r4.xyz = Light0.m_colorIntensity.xyz * r1.www;
  r3.xyz = r3.xyz * r1.zzz + Light0.m_direction.xyz;
  r1.z = dot(r3.xyz, r3.xyz);
  r1.z = rsqrt(r1.z);
  r3.xyz = r3.xyz * r1.zzz;
  r1.z = saturate(dot(r2.xyz, r3.xyz));
  r1.z = log2(r1.z);
  r1.yz = Shininess * r1.yz;
  r1.z = exp2(r1.z);
  r1.z = min(1, r1.z);
  r1.y = r1.z * r1.y;
  r2.xyz = min(float3(1.5,1.5,1.5), r4.xyz);
  r1.yzw = Light0.m_colorIntensity.xyz * r1.yyy + r2.xyz;
  r0.w = 1 + -r0.w;
  r2.xyz = r1.yzw * scene.MiscParameters1.xyz + -r1.yzw;
  r1.yzw = r0.www * r2.xyz + r1.yzw;
  r1.yzw = v1.xyz * r1.yzw;
  r0.xyz = r1.yzw * r0.xyz;
  r0.w = GameMaterialDiffuse.w * r1.x;
  r0.xyz = r0.xyz * GameMaterialDiffuse.xyz + GameMaterialEmission.xyz;
  r0.xyz = v2.xyz + r0.xyz;
  r1.x = dot(r0.xyz, float3(0.298999995,0.587000012,0.114));
  r1.xyz = r1.xxx * scene.MonotoneMul.xyz + scene.MonotoneAdd.xyz;
  r1.xyz = r1.xyz + -r0.xyz;
  o0.xyz = GameMaterialMonotone * r1.xyz + r0.xyz;
  o0.w = r0.w;
  return;
}
