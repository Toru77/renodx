// ---- Created with 3Dmigoto v1.3.16 on Wed Jul 16 13:22:37 2025
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

  float PerMaterialMainLightClampFactor : packoffset(c73) = {1.5};
  float3 LightDirForChar : packoffset(c73.y);
  float GameMaterialID : packoffset(c74) = {0};
  float4 GameMaterialDiffuse : packoffset(c75) = {1,1,1,1};
  float4 GameMaterialEmission : packoffset(c76) = {0,0,0,0};
  float GameMaterialMonotone : packoffset(c77) = {0};
  float4 GameMaterialTexcoord : packoffset(c78) = {0,0,1,1};
  float4 UVaMUvColor : packoffset(c79) = {1,1,1,1};
  float4 UVaProjTexcoord : packoffset(c80) = {0,0,1,1};
  float4 UVaMUvTexcoord : packoffset(c81) = {0,0,1,1};
  float4 UVaMUv2Texcoord : packoffset(c82) = {0,0,1,1};
  float4 UVaDuDvTexcoord : packoffset(c83) = {0,0,1,1};
  float AlphaThreshold : packoffset(c84) = {0.5};
  float3 ShadowColorShift : packoffset(c84.y) = {0.100000001,0.0199999996,0.0199999996};
  float3 RimLitColor : packoffset(c85) = {1,1,1};
  float RimLitIntensity : packoffset(c85.w) = {4};
  float RimLitPower : packoffset(c86) = {2};
  float RimLightClampFactor : packoffset(c86.y) = {2};
  float ShadowReceiveOffset : packoffset(c86.z) = {0.600000024};
  float BloomIntensity : packoffset(c86.w) = {0.699999988};
  float4 GameEdgeParameters : packoffset(c87) = {1,1,1,0.00300000003};
  float MaskEps : packoffset(c88);
  float4 PointLightParams : packoffset(c89) = {0,2,1,1};
  float4 PointLightColor : packoffset(c90) = {1,0,0,0};
}

SamplerState LinearWrapSamplerState_s : register(s0);
SamplerState LinearClampSamplerState_s : register(s1);
SamplerState PointClampSamplerState_s : register(s2);
SamplerState DiffuseMapSamplerSampler_s : register(s4);
SamplerState NormalMapSamplerSampler_s : register(s5);
SamplerComparisonState LinearClampCmpSamplerState_s : register(s3);
Texture2D<float4> LowResDepthTexture : register(t0);
Texture2D<float4> LightShadowMap0 : register(t1);
Texture2D<float4> DiffuseMapSampler : register(t2);
Texture2D<float4> NormalMapSampler : register(t3);
Texture2D<float4> CartoonMapSampler : register(t4);


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_POSITION0,
  float4 v1 : COLOR0,
  float4 v2 : COLOR1,
  float4 v3 : TEXCOORD0,
  float4 v4 : TEXCOORD1,
  float4 v5 : TEXCOORD4,
  float3 v6 : TEXCOORD6,
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
  r0.w = cmp(LightShadow0.m_splitDistances.y >= v4.w);
  if (r0.w != 0) {
    r1.yzw = Light0.m_direction.xyz * ShadowReceiveOffset + v4.xyz;
    r2.xyz = v5.xyz * float3(0.0500000007,0.0500000007,0.0500000007) + r1.yzw;
    r0.w = cmp(v4.w < LightShadow0.m_splitDistances.x);
    if (r0.w != 0) {
      r2.w = 1;
      r3.x = dot(r2.xyzw, LightShadow0.m_split0Transform._m00_m10_m20_m30);
      r3.y = dot(r2.xyzw, LightShadow0.m_split0Transform._m01_m11_m21_m31);
      r3.z = dot(r2.xyzw, LightShadow0.m_split0Transform._m02_m12_m22_m32);
      r0.w = 25 / LightShadow0.m_splitDistances.y;
      r1.y = (int)scene.DuranteSettings.x & 16;
      if (r1.y != 0) {
        r1.yz = float2(0.000937499979,0.00187499996) * r0.ww;
        r1.w = -6 + r3.z;
        r1.yz = r1.yz * r1.ww;
        r1.yz = r1.yz / r3.zz;
        r1.w = dot(v0.xy, float2(0.0671105608,0.00583714992));
        r1.w = frac(r1.w);
        r1.w = 52.9829178 * r1.w;
        r1.w = frac(r1.w);
        r1.w = 6.28318548 * r1.w;
        r4.xy = float2(0,0);
        r3.w = 0;
        while (true) {
          r4.z = cmp((int)r3.w >= 16);
          if (r4.z != 0) break;
          r4.z = (int)r3.w;
          r4.w = 0.5 + r4.z;
          r4.w = sqrt(r4.w);
          r4.w = 0.25 * r4.w;
          r4.z = r4.z * 2.4000001 + r1.w;
          sincos(r4.z, r5.x, r6.x);
          r6.x = r6.x * r4.w;
          r6.y = r5.x * r4.w;
          r4.zw = r6.xy * r1.yz + r3.xy;
          r4.z = LightShadowMap0.SampleLevel(PointClampSamplerState_s, r4.zw, 0).x;
          r4.w = cmp(r4.z < r3.z);
          r5.y = r4.y + r4.z;
          r5.x = 1 + r4.x;
          r4.xy = r4.ww ? r5.xy : r4.xy;
          r3.w = (int)r3.w + 1;
        }
        r1.y = cmp(r4.x >= 1);
        if (r1.y != 0) {
          r1.yz = float2(0.000624999986,0.00124999997) * r0.ww;
          r3.w = r4.y / r4.x;
          r3.w = r3.z + -r3.w;
          r3.w = min(0.0700000003, r3.w);
          r3.w = 60 * r3.w;
          r1.yz = r3.ww * r1.yz;
          LightShadowMap0.GetDimensions(0, fDest.x, fDest.y, fDest.z);
          r4.xy = fDest.xy;
          r4.xy = float2(0.5,0.5) / r4.xy;
          r1.yz = max(r4.xy, r1.yz);
          r3.w = 0;
          r4.x = 0;
          while (true) {
            r4.y = cmp((int)r4.x >= 16);
            if (r4.y != 0) break;
            r4.y = (int)r4.x;
            r4.z = 0.5 + r4.y;
            r4.z = sqrt(r4.z);
            r4.z = 0.25 * r4.z;
            r4.y = r4.y * 2.4000001 + r1.w;
            sincos(r4.y, r5.x, r6.x);
            r6.x = r6.x * r4.z;
            r6.y = r5.x * r4.z;
            r4.yz = r6.xy * r1.yz;
            r4.yz = r4.yz * float2(3,3) + r3.xy;
            r4.y = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r4.yz, r3.z).x;
            r3.w = r4.y + r3.w;
            r4.x = (int)r4.x + 1;
          }
          r1.y = 0.0625 * r3.w;
        } else {
          r1.y = 1;
        }
      } else {
        r1.z = (int)scene.DuranteSettings.x & 8;
        if (r1.z != 0) {
          r1.zw = float2(0.000375000003,0.000750000007) * r0.ww;
          r3.w = dot(v0.xy, float2(0.0671105608,0.00583714992));
          r3.w = frac(r3.w);
          r3.w = 52.9829178 * r3.w;
          r3.w = frac(r3.w);
          r3.w = 6.28318548 * r3.w;
          r4.xy = float2(0,0);
          while (true) {
            r4.z = cmp((int)r4.y >= 16);
            if (r4.z != 0) break;
            r4.z = (int)r4.y;
            r4.w = 0.5 + r4.z;
            r4.w = sqrt(r4.w);
            r4.w = 0.25 * r4.w;
            r4.z = r4.z * 2.4000001 + r3.w;
            sincos(r4.z, r5.x, r6.x);
            r6.x = r6.x * r4.w;
            r6.y = r5.x * r4.w;
            r4.zw = r6.xy * r1.zw;
            r4.zw = r4.zw * float2(3,3) + r3.xy;
            r4.z = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r4.zw, r3.z).x;
            r4.x = r4.x + r4.z;
            r4.y = (int)r4.y + 1;
          }
          r1.y = 0.0625 * r4.x;
        } else {
          r1.z = dot(r2.xyzw, LightShadow0.m_split0Transform._m03_m13_m23_m33);
          r1.w = (int)scene.DuranteSettings.x & 4;
          if (r1.w != 0) {
            r4.xy = scene.MiscParameters4.xy * r1.zz;
            r1.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r3.xy, r3.z).x;
            r5.xy = -r4.xy * r0.ww;
            r5.z = 0;
            r6.xyz = r5.xyz + r3.xyz;
            r3.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r6.xy, r6.z).x;
            r3.w = 0.0625 * r3.w;
            r1.w = r1.w * 0.5 + r3.w;
            r6.x = r4.x * r0.w;
            r6.y = -r4.y * r0.w;
            r6.z = 0;
            r6.xyz = r6.xyz + r3.xyz;
            r3.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r6.xy, r6.z).x;
            r1.w = r3.w * 0.0625 + r1.w;
            r6.x = -r4.x * r0.w;
            r6.y = r4.y * r0.w;
            r6.z = 0;
            r7.xyz = r6.xyz + r3.xyz;
            r3.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r7.xy, r7.z).x;
            r1.w = r3.w * 0.0625 + r1.w;
            r4.xy = r4.xy * r0.ww;
            r4.z = 0;
            r7.xyz = r4.xyz + r3.xyz;
            r3.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r7.xy, r7.z).x;
            r1.w = r3.w * 0.0625 + r1.w;
            r5.xyz = r5.zyz + r3.xyz;
            r3.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r5.xy, r5.z).x;
            r1.w = r3.w * 0.0625 + r1.w;
            r5.xyz = r6.xzz + r3.xyz;
            r3.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r5.xy, r5.z).x;
            r1.w = r3.w * 0.0625 + r1.w;
            r4.xyz = r4.xzz + r3.xyz;
            r3.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r4.xy, r4.z).x;
            r1.w = r3.w * 0.0625 + r1.w;
            r4.xyz = r6.zyz + r3.xyz;
            r3.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r4.xy, r4.z).x;
            r1.y = r3.w * 0.0625 + r1.w;
          } else {
            r1.w = (int)scene.DuranteSettings.x & 2;
            if (r1.w != 0) {
              r1.zw = scene.MiscParameters4.xy * r1.zz;
              r3.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r3.xy, r3.z).x;
              r4.xy = -r1.zw * r0.ww;
              r4.z = 0;
              r4.xyz = r4.xyz + r3.xyz;
              r4.x = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r4.xy, r4.z).x;
              r4.x = scene.MiscParameters5.y * r4.x;
              r3.w = r3.w * scene.MiscParameters5.x + r4.x;
              r4.x = r1.z * r0.w;
              r4.y = -r1.w * r0.w;
              r4.z = 0;
              r4.xyz = r4.xyz + r3.xyz;
              r4.x = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r4.xy, r4.z).x;
              r3.w = r4.x * scene.MiscParameters5.y + r3.w;
              r4.x = -r1.z * r0.w;
              r4.y = r1.w * r0.w;
              r4.z = 0;
              r4.xyz = r4.xyz + r3.xyz;
              r4.x = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r4.xy, r4.z).x;
              r3.w = r4.x * scene.MiscParameters5.y + r3.w;
              r4.xy = r1.zw * r0.ww;
              r4.z = 0;
              r4.xyz = r4.xyz + r3.xyz;
              r0.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r4.xy, r4.z).x;
              r1.y = r0.w * scene.MiscParameters5.y + r3.w;
            } else {
              r1.y = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r3.xy, r3.z).x;
            }
          }
        }
      }
    } else {
      r2.w = 1;
      r3.x = dot(r2.xyzw, LightShadow0.m_split1Transform._m00_m10_m20_m30);
      r3.y = dot(r2.xyzw, LightShadow0.m_split1Transform._m01_m11_m21_m31);
      r3.z = dot(r2.xyzw, LightShadow0.m_split1Transform._m02_m12_m22_m32);
      r0.w = 10 / LightShadow0.m_splitDistances.y;
      r1.z = (int)scene.DuranteSettings.x & 16;
      if (r1.z != 0) {
        r1.zw = float2(0.000937499979,0.00187499996) * r0.ww;
        r3.w = -6 + r3.z;
        r1.zw = r3.ww * r1.zw;
        r1.zw = r1.zw / r3.zz;
        r3.w = dot(v0.xy, float2(0.0671105608,0.00583714992));
        r3.w = frac(r3.w);
        r3.w = 52.9829178 * r3.w;
        r3.w = frac(r3.w);
        r3.w = 6.28318548 * r3.w;
        r4.xyz = float3(0,0,0);
        while (true) {
          r4.w = cmp((int)r4.z >= 16);
          if (r4.w != 0) break;
          r4.w = (int)r4.z;
          r5.x = 0.5 + r4.w;
          r5.x = sqrt(r5.x);
          r5.x = 0.25 * r5.x;
          r4.w = r4.w * 2.4000001 + r3.w;
          sincos(r4.w, r6.x, r7.x);
          r7.x = r7.x * r5.x;
          r7.y = r6.x * r5.x;
          r5.xy = r7.xy * r1.zw + r3.xy;
          r4.w = LightShadowMap0.SampleLevel(PointClampSamplerState_s, r5.xy, 0).x;
          r5.x = cmp(r4.w < r3.z);
          r6.y = r4.y + r4.w;
          r6.x = 1 + r4.x;
          r4.xy = r5.xx ? r6.xy : r4.xy;
          r4.z = (int)r4.z + 1;
        }
        r1.z = cmp(r4.x >= 1);
        if (r1.z != 0) {
          r1.zw = float2(0.000624999986,0.00124999997) * r0.ww;
          r4.x = r4.y / r4.x;
          r4.x = -r4.x + r3.z;
          r4.x = min(0.0700000003, r4.x);
          r4.x = 60 * r4.x;
          r1.zw = r4.xx * r1.zw;
          LightShadowMap0.GetDimensions(0, fDest.x, fDest.y, fDest.z);
          r4.xy = fDest.xy;
          r4.xy = float2(0.5,0.5) / r4.xy;
          r1.zw = max(r4.xy, r1.zw);
          r4.xy = float2(0,0);
          while (true) {
            r4.z = cmp((int)r4.y >= 16);
            if (r4.z != 0) break;
            r4.z = (int)r4.y;
            r4.w = 0.5 + r4.z;
            r4.w = sqrt(r4.w);
            r4.w = 0.25 * r4.w;
            r4.z = r4.z * 2.4000001 + r3.w;
            sincos(r4.z, r5.x, r6.x);
            r6.x = r6.x * r4.w;
            r6.y = r5.x * r4.w;
            r4.zw = r6.xy * r1.zw;
            r4.zw = r4.zw * float2(3,3) + r3.xy;
            r4.z = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r4.zw, r3.z).x;
            r4.x = r4.x + r4.z;
            r4.y = (int)r4.y + 1;
          }
          r1.y = 0.0625 * r4.x;
        } else {
          r1.y = 1;
        }
      } else {
        r1.z = (int)scene.DuranteSettings.x & 8;
        if (r1.z != 0) {
          r1.zw = float2(0.000375000003,0.000750000007) * r0.ww;
          r3.w = dot(v0.xy, float2(0.0671105608,0.00583714992));
          r3.w = frac(r3.w);
          r3.w = 52.9829178 * r3.w;
          r3.w = frac(r3.w);
          r3.w = 6.28318548 * r3.w;
          r4.xy = float2(0,0);
          while (true) {
            r4.z = cmp((int)r4.y >= 16);
            if (r4.z != 0) break;
            r4.z = (int)r4.y;
            r4.w = 0.5 + r4.z;
            r4.w = sqrt(r4.w);
            r4.w = 0.25 * r4.w;
            r4.z = r4.z * 2.4000001 + r3.w;
            sincos(r4.z, r5.x, r6.x);
            r6.x = r6.x * r4.w;
            r6.y = r5.x * r4.w;
            r4.zw = r6.xy * r1.zw;
            r4.zw = r4.zw * float2(3,3) + r3.xy;
            r4.z = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r4.zw, r3.z).x;
            r4.x = r4.x + r4.z;
            r4.y = (int)r4.y + 1;
          }
          r1.y = 0.0625 * r4.x;
        } else {
          r1.z = dot(r2.xyzw, LightShadow0.m_split1Transform._m03_m13_m23_m33);
          r1.w = (int)scene.DuranteSettings.x & 4;
          if (r1.w != 0) {
            r2.xy = scene.MiscParameters4.xy * r1.zz;
            r1.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r3.xy, r3.z).x;
            r4.xy = -r2.xy * r0.ww;
            r4.z = 0;
            r4.xyz = r4.xyz + r3.xyz;
            r2.z = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r4.xy, r4.z).x;
            r2.z = 0.0625 * r2.z;
            r1.w = r1.w * 0.5 + r2.z;
            r4.x = r2.x * r0.w;
            r4.y = -r2.y * r0.w;
            r4.z = 0;
            r5.xyz = r4.xyz + r3.xyz;
            r2.z = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r5.xy, r5.z).x;
            r1.w = r2.z * 0.0625 + r1.w;
            r5.x = -r2.x * r0.w;
            r5.y = r2.y * r0.w;
            r5.z = 0;
            r6.xyz = r5.xyz + r3.xyz;
            r2.z = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r6.xy, r6.z).x;
            r1.w = r2.z * 0.0625 + r1.w;
            r2.xy = r2.xy * r0.ww;
            r2.z = 0;
            r6.xyz = r3.xyz + r2.xyz;
            r2.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r6.xy, r6.z).x;
            r1.w = r2.w * 0.0625 + r1.w;
            r4.xyz = r4.zyz + r3.xyz;
            r2.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r4.xy, r4.z).x;
            r1.w = r2.w * 0.0625 + r1.w;
            r4.xyz = r5.xzz + r3.xyz;
            r2.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r4.xy, r4.z).x;
            r1.w = r2.w * 0.0625 + r1.w;
            r4.xyz = r3.xyz + r2.xzz;
            r2.x = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r4.xy, r4.z).x;
            r1.w = r2.x * 0.0625 + r1.w;
            r2.xyz = r3.xyz + r2.zyz;
            r2.x = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r2.xy, r2.z).x;
            r1.y = r2.x * 0.0625 + r1.w;
          } else {
            r1.w = (int)scene.DuranteSettings.x & 2;
            if (r1.w != 0) {
              r1.zw = scene.MiscParameters4.xy * r1.zz;
              r2.x = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r3.xy, r3.z).x;
              r4.xy = -r1.zw * r0.ww;
              r4.z = 0;
              r2.yzw = r4.xyz + r3.xyz;
              r2.y = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r2.yz, r2.w).x;
              r2.y = scene.MiscParameters5.y * r2.y;
              r2.x = r2.x * scene.MiscParameters5.x + r2.y;
              r4.x = r1.z * r0.w;
              r4.y = -r1.w * r0.w;
              r4.z = 0;
              r2.yzw = r4.xyz + r3.xyz;
              r2.y = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r2.yz, r2.w).x;
              r2.x = r2.y * scene.MiscParameters5.y + r2.x;
              r4.x = -r1.z * r0.w;
              r4.y = r1.w * r0.w;
              r4.z = 0;
              r2.yzw = r4.xyz + r3.xyz;
              r2.y = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r2.yz, r2.w).x;
              r2.x = r2.y * scene.MiscParameters5.y + r2.x;
              r4.xy = r1.zw * r0.ww;
              r4.z = 0;
              r2.yzw = r4.xyz + r3.xyz;
              r0.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r2.yz, r2.w).x;
              r1.y = r0.w * scene.MiscParameters5.y + r2.x;
            } else {
              r1.y = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r3.xy, r3.z).x;
            }
          }
        }
      }
    }
  } else {
    r1.y = 1;
  }
  r0.w = saturate(v4.w / LightShadow0.m_splitDistances.y);
  r1.z = r0.w * r0.w;
  r1.w = -scene.MiscParameters2.y + v4.y;
  r1.w = scene.MiscParameters2.x * abs(r1.w);
  r1.w = min(1, r1.w);
  r2.x = r1.w * r1.w;
  r1.w = r2.x * r1.w;
  r2.x = cmp(0 >= scene.MiscParameters2.x);
  r1.w = r2.x ? 0 : r1.w;
  r0.w = r0.w * r1.z + r1.w;
  r1.z = dot(v5.xyz, Light0.m_direction.xyz);
  r1.z = r1.z * 0.5 + 0.5;
  r1.w = r1.z * r1.z;
  r1.z = -r1.z * r1.w + 1;
  r0.w = r1.z * r1.z + r0.w;
  r0.w = min(1, r0.w);
  r0.w = r1.y + r0.w;
  r0.w = min(1, r0.w);
  r1.y = 1 + -r0.w;
  r1.z = 1 + -scene.MiscParameters2.w;
  r0.w = r1.y * r1.z + r0.w;
  r1.y = 1 + -r0.w;
  r1.z = 1 + -GameMaterialEmission.w;
  r0.w = r1.y * r1.z + r0.w;
  r1.y = r0.w * r0.w;
  r1.yzw = Light0.m_colorIntensity.xyz * r1.yyy;
  r2.xyz = NormalMapSampler.Sample(NormalMapSamplerSampler_s, v3.xy).xyz;
  r2.xyz = r2.xyz * float3(2,2,2) + float3(-1,-1,-1);
  r2.w = dot(v6.xyz, v6.xyz);
  r2.w = rsqrt(r2.w);
  r3.xyz = v6.xyz * r2.www;
  r2.w = dot(v5.xyz, v5.xyz);
  r2.w = rsqrt(r2.w);
  r4.xyz = v5.xyz * r2.www;
  r5.xyz = r4.zxy * r3.yzx;
  r5.xyz = r4.yzx * r3.zxy + -r5.xyz;
  r2.w = cmp(v3.x < 0);
  r2.w = r2.w ? -1 : 1;
  r2.x = r2.x * r2.w;
  r5.xyz = r5.xyz * r2.yyy;
  r2.xyw = r2.xxx * r3.xyz + r5.xyz;
  r2.xyz = r2.zzz * r4.xyz + r2.xyw;
  r2.w = dot(r2.xyz, r2.xyz);
  r2.w = rsqrt(r2.w);
  r2.xyz = r2.xyz * r2.www;
  r3.xyz = scene.EyePosition.xyz + -v4.xyz;
  r3.w = dot(r3.xyz, r3.xyz);
  r3.w = rsqrt(r3.w);
  r3.xyz = r3.xyz * r3.www;
  r2.w = dot(r2.xyz, r3.xyz);
  r3.x = cmp(r2.w < 0);
  r2.xyzw = r3.xxxx ? -r2.xyzw : r2.xyzw;
  r2.x = dot(LightDirForChar.xyz, r2.xyz);
  r2.x = r2.x * 0.5 + 0.5;
  r2.y = 0;
  r2.x = CartoonMapSampler.SampleLevel(LinearClampSamplerState_s, r2.xy, 0).x;
  r2.xyz = Light0.m_colorIntensity.xyz * r2.xxx + scene.GlobalAmbientColor.xyz;
  r2.xyz = min(PerMaterialMainLightClampFactor, r2.xyz);
  r0.w = 1 + -r0.w;
  r3.xyz = r2.xyz * scene.MiscParameters1.xyz + -r2.xyz;
  r2.xyz = r0.www * r3.xyz + r2.xyz;
  r0.w = 1 + -abs(r2.w);
  r0.w = max(0, r0.w);
  float l = r0.w;
  // r0.w = log2(r0.w);
  // r2.w = RimLitPower * r0.w;
  // r2.w = exp2(r2.w);
  r2.w = renodx::math::SafePow(l, RimLitPower);

  r2.w = -r2.w * RimLitIntensity + 1;
  r3.w = r2.w * r1.x;
  r1.xyz = r1.yzw + r1.yzw;
  r1.xyz = max(float3(1,1,1), r1.xyz);
  r4.xyz = min(float3(1,1,1), r2.xyz);
  r4.xyz = float3(1,1,1) + -r4.xyz;
  r4.xyz = ShadowColorShift.xyz * r4.xyz;
  r1.xyz = r4.xyz * r1.xyz + r2.xyz;
  r1.xyz = v1.xyz * r1.xyz;
  r3.xyz = r1.xyz * r0.xyz;
  r1.xyzw = GameMaterialDiffuse.xyzw * r3.xyzw;
  // r0.x = PointLightColor.x * r0.w;
  // r0.x = exp2(r0.x);
  r0.x = renodx::math::SafePow(l, PointLightColor.x);
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
  r0.w = calculateLuminanceSRGB(r0.xyz);
  r1.xyz = r0.www * scene.MonotoneMul.xyz + scene.MonotoneAdd.xyz;
  r1.xyz = r1.xyz + -r0.xyz;
  o0.xyz = GameMaterialMonotone * r1.xyz + r0.xyz;
  o0.w = r1.w;

  o0.w = max(o0.w, 0.f);
  // o1 = saturate(o1);
  // o2 = saturate(o2);
  return;
}