// ---- Created with 3Dmigoto v1.3.16 on Tue Aug 26 00:12:42 2025
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
  float4x4 WorldViewProjection : packoffset(c48);
  float GlobalTexcoordFactor : packoffset(c52);

  struct
  {
    float3 m_direction;
    float3 m_colorIntensity;
  } Light0 : packoffset(c53);


  struct
  {
    float4x4 m_split0Transform;
    float4x4 m_split1Transform;
    float4 m_splitDistances;
  } LightShadow0 : packoffset(c55);

  float GameMaterialID : packoffset(c64) = {0};
  float4 GameMaterialDiffuse : packoffset(c65) = {1,1,1,1};
  float4 GameMaterialEmission : packoffset(c66) = {0,0,0,0};
  float GameMaterialMonotone : packoffset(c67) = {0};
  float4 GameMaterialTexcoord : packoffset(c68) = {0,0,1,1};
  float4 GameDitherParams : packoffset(c69) = {0,0,0,0};
  float4 UVaMUvColor : packoffset(c70) = {1,1,1,1};
  float4 UVaProjTexcoord : packoffset(c71) = {0,0,1,1};
  float4 UVaMUvTexcoord : packoffset(c72) = {0,0,1,1};
  float4 UVaMUv2Texcoord : packoffset(c73) = {0,0,1,1};
  float4 UVaDuDvTexcoord : packoffset(c74) = {0,0,1,1};
  float BloomIntensity : packoffset(c75) = {1};
  float4 PointLightParams : packoffset(c76) = {0,0,0,0};
  float4 PointLightColor : packoffset(c77) = {0,0,0,0};
}

SamplerState PointClampSamplerState_s : register(s0);
SamplerState DiffuseMapSamplerSampler_s : register(s2);
SamplerComparisonState LinearClampCmpSamplerState_s : register(s1);
Texture2D<float4> LightShadowMap0 : register(t0);
Texture2D<float4> DiffuseMapSampler : register(t1);


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_POSITION0,
  float4 v1 : COLOR0,
  float4 v2 : COLOR1,
  float4 v3 : TEXCOORD0,
  float4 v4 : TEXCOORD1,
  float4 v5 : TEXCOORD4,
  float4 v6 : TEXCOORD10,
  out float4 o0 : SV_TARGET0,
  out float4 o1 : SV_TARGET1,
  out float4 o2 : SV_TARGET2)
{
  float4 r0,r1,r2,r3,r4,r5,r6,r7;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.xyz = DiffuseMapSampler.Sample(DiffuseMapSamplerSampler_s, v3.xy).xyz;
  r0.w = cmp(LightShadow0.m_splitDistances.y >= v4.w);
  if (r0.w != 0) {
    r1.xyz = Light0.m_direction.xyz * float3(0.0500000007,0.0500000007,0.0500000007) + v4.xyz;
    r1.xyz = v5.xyz * float3(0.0500000007,0.0500000007,0.0500000007) + r1.xyz;
    r2.x = cmp(v4.w < LightShadow0.m_splitDistances.x);
    if (r2.x != 0) {
      r1.w = 1;
      r2.x = dot(r1.xyzw, LightShadow0.m_split0Transform._m00_m10_m20_m30);
      r2.y = dot(r1.xyzw, LightShadow0.m_split0Transform._m01_m11_m21_m31);
      r2.z = dot(r1.xyzw, LightShadow0.m_split0Transform._m02_m12_m22_m32);
      r2.w = (int)DuranteSettings.x & 2;
      if (r2.w != 0) {
        r2.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r2.xy, r2.z).x;
      } else {
        r3.x = 25 / LightShadow0.m_splitDistances.y;
        r3.y = (int)DuranteSettings.x & 16;
        if (r3.y != 0) {
          r3.yz = float2(0.000937499979,0.00187499996) * r3.xx;
          r3.w = -6 + r2.z;
          r3.yz = r3.yz * r3.ww;
          r3.yz = r3.yz / r2.zz;
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
            r5.xy = r7.xy * r3.yz + r2.xy;
            r4.w = LightShadowMap0.SampleLevel(PointClampSamplerState_s, r5.xy, 0).x;
            r5.x = cmp(r4.w < r2.z);
            r6.y = r4.y + r4.w;
            r6.x = 1 + r4.x;
            r4.xy = r5.xx ? r6.xy : r4.xy;
            r4.z = (int)r4.z + 1;
          }
          r3.y = cmp(r4.x >= 1);
          if (r3.y != 0) {
            r3.yz = float2(0.000624999986,0.00124999997) * r3.xx;
            r4.x = r4.y / r4.x;
            r4.x = -r4.x + r2.z;
            r4.x = min(0.0700000003, r4.x);
            r4.x = 60 * r4.x;
            r3.yz = r4.xx * r3.yz;
            LightShadowMap0.GetDimensions(0, fDest.x, fDest.y, fDest.z);
            r4.xy = fDest.xy;
            r4.xy = float2(0.5,0.5) / r4.xy;
            r3.yz = max(r4.xy, r3.yz);
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
              r4.zw = r6.xy * r3.yz;
              r4.zw = r4.zw * float2(3,3) + r2.xy;
              r4.z = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r4.zw, r2.z).x;
              r4.x = r4.x + r4.z;
              r4.y = (int)r4.y + 1;
            }
            r2.w = 0.0625 * r4.x;
          } else {
            r2.w = 1;
          }
        } else {
          r3.y = (int)DuranteSettings.x & 8;
          if (r3.y != 0) {
            r3.yz = float2(0.000375000003,0.000750000007) * r3.xx;
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
              r4.zw = r6.xy * r3.yz;
              r4.zw = r4.zw * float2(3,3) + r2.xy;
              r4.z = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r4.zw, r2.z).x;
              r4.x = r4.x + r4.z;
              r4.y = (int)r4.y + 1;
            }
            r2.w = 0.0625 * r4.x;
          } else {
            r3.y = dot(r1.xyzw, LightShadow0.m_split0Transform._m03_m13_m23_m33);
            r3.z = (int)DuranteSettings.x & 4;
            if (r3.z != 0) {
              r3.zw = scene.MiscParameters4.xy * r3.yy;
              r4.x = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r2.xy, r2.z).x;
              r5.xy = -r3.zw * r3.xx;
              r5.z = 0;
              r4.yzw = r5.xyz + r2.xyz;
              r4.y = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r4.yz, r4.w).x;
              r4.y = 0.0625 * r4.y;
              r4.x = r4.x * 0.5 + r4.y;
              r6.x = r3.z * r3.x;
              r6.y = -r3.w * r3.x;
              r6.z = 0;
              r4.yzw = r6.xyz + r2.xyz;
              r4.y = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r4.yz, r4.w).x;
              r4.x = r4.y * 0.0625 + r4.x;
              r7.x = -r3.z * r3.x;
              r7.y = r3.w * r3.x;
              r7.z = 0;
              r4.yzw = r7.xyz + r2.xyz;
              r4.y = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r4.yz, r4.w).x;
              r4.x = r4.y * 0.0625 + r4.x;
              r7.xy = r3.zw * r3.xx;
              r7.z = 0;
              r4.yzw = r7.xyz + r2.xyz;
              r3.z = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r4.yz, r4.w).x;
              r3.z = r3.z * 0.0625 + r4.x;
              r4.xyz = r6.zyz + r2.xyz;
              r3.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r4.xy, r4.z).x;
              r3.z = r3.w * 0.0625 + r3.z;
              r4.xyz = r5.xzz + r2.xyz;
              r3.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r4.xy, r4.z).x;
              r3.z = r3.w * 0.0625 + r3.z;
              r4.xyz = r6.xzz + r2.xyz;
              r3.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r4.xy, r4.z).x;
              r3.z = r3.w * 0.0625 + r3.z;
              r4.xyz = r7.zyz + r2.xyz;
              r3.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r4.xy, r4.z).x;
              r2.w = r3.w * 0.0625 + r3.z;
            } else {
              r3.yz = scene.MiscParameters4.xy * r3.yy;
              r3.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r2.xy, r2.z).x;
              r4.xy = -r3.yz * r3.xx;
              r4.z = 0;
              r4.xyz = r4.xyz + r2.xyz;
              r4.x = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r4.xy, r4.z).x;
              r4.x = scene.MiscParameters5.y * r4.x;
              r3.w = r3.w * scene.MiscParameters5.x + r4.x;
              r4.x = r3.y * r3.x;
              r4.y = -r3.z * r3.x;
              r4.z = 0;
              r4.xyz = r4.xyz + r2.xyz;
              r4.x = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r4.xy, r4.z).x;
              r3.w = r4.x * scene.MiscParameters5.y + r3.w;
              r4.x = -r3.y * r3.x;
              r4.y = r3.z * r3.x;
              r4.z = 0;
              r4.xyz = r4.xyz + r2.xyz;
              r4.x = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r4.xy, r4.z).x;
              r3.w = r4.x * scene.MiscParameters5.y + r3.w;
              r3.xy = r3.yz * r3.xx;
              r3.z = 0;
              r2.xyz = r3.xyz + r2.xyz;
              r2.x = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r2.xy, r2.z).x;
              r2.w = r2.x * scene.MiscParameters5.y + r3.w;
            }
          }
        }
      }
    } else {
      r1.w = 1;
      r2.x = dot(r1.xyzw, LightShadow0.m_split1Transform._m00_m10_m20_m30);
      r2.y = dot(r1.xyzw, LightShadow0.m_split1Transform._m01_m11_m21_m31);
      r2.z = dot(r1.xyzw, LightShadow0.m_split1Transform._m02_m12_m22_m32);
      r3.x = (int)DuranteSettings.x & 2;
      if (r3.x != 0) {
        r2.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r2.xy, r2.z).x;
      } else {
        r3.x = 10 / LightShadow0.m_splitDistances.y;
        r3.y = (int)DuranteSettings.x & 16;
        if (r3.y != 0) {
          r3.yz = float2(0.000937499979,0.00187499996) * r3.xx;
          r3.w = -6 + r2.z;
          r3.yz = r3.yz * r3.ww;
          r3.yz = r3.yz / r2.zz;
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
            r5.xy = r7.xy * r3.yz + r2.xy;
            r4.w = LightShadowMap0.SampleLevel(PointClampSamplerState_s, r5.xy, 0).x;
            r5.x = cmp(r4.w < r2.z);
            r6.y = r4.y + r4.w;
            r6.x = 1 + r4.x;
            r4.xy = r5.xx ? r6.xy : r4.xy;
            r4.z = (int)r4.z + 1;
          }
          r3.y = cmp(r4.x >= 1);
          if (r3.y != 0) {
            r3.yz = float2(0.000624999986,0.00124999997) * r3.xx;
            r4.x = r4.y / r4.x;
            r4.x = -r4.x + r2.z;
            r4.x = min(0.0700000003, r4.x);
            r4.x = 60 * r4.x;
            r3.yz = r4.xx * r3.yz;
            LightShadowMap0.GetDimensions(0, fDest.x, fDest.y, fDest.z);
            r4.xy = fDest.xy;
            r4.xy = float2(0.5,0.5) / r4.xy;
            r3.yz = max(r4.xy, r3.yz);
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
              r4.zw = r6.xy * r3.yz;
              r4.zw = r4.zw * float2(3,3) + r2.xy;
              r4.z = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r4.zw, r2.z).x;
              r4.x = r4.x + r4.z;
              r4.y = (int)r4.y + 1;
            }
            r2.w = 0.0625 * r4.x;
          } else {
            r2.w = 1;
          }
        } else {
          r3.y = (int)DuranteSettings.x & 8;
          if (r3.y != 0) {
            r3.yz = float2(0.000375000003,0.000750000007) * r3.xx;
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
              r4.zw = r6.xy * r3.yz;
              r4.zw = r4.zw * float2(3,3) + r2.xy;
              r4.z = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r4.zw, r2.z).x;
              r4.x = r4.x + r4.z;
              r4.y = (int)r4.y + 1;
            }
            r2.w = 0.0625 * r4.x;
          } else {
            r1.x = dot(r1.xyzw, LightShadow0.m_split1Transform._m03_m13_m23_m33);
            r1.y = (int)DuranteSettings.x & 4;
            if (r1.y != 0) {
              r1.yz = scene.MiscParameters4.xy * r1.xx;
              r1.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r2.xy, r2.z).x;
              r4.xy = -r1.yz * r3.xx;
              r4.z = 0;
              r3.yzw = r4.xyz + r2.xyz;
              r3.y = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r3.yz, r3.w).x;
              r3.y = 0.0625 * r3.y;
              r1.w = r1.w * 0.5 + r3.y;
              r4.x = r1.y * r3.x;
              r4.y = -r1.z * r3.x;
              r4.z = 0;
              r3.yzw = r4.xyz + r2.xyz;
              r3.y = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r3.yz, r3.w).x;
              r1.w = r3.y * 0.0625 + r1.w;
              r5.x = -r1.y * r3.x;
              r5.y = r1.z * r3.x;
              r5.z = 0;
              r3.yzw = r5.xyz + r2.xyz;
              r3.y = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r3.yz, r3.w).x;
              r1.w = r3.y * 0.0625 + r1.w;
              r6.xy = r1.yz * r3.xx;
              r6.z = 0;
              r3.yzw = r6.xyz + r2.xyz;
              r1.y = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r3.yz, r3.w).x;
              r1.y = r1.y * 0.0625 + r1.w;
              r3.yzw = r4.zyz + r2.xyz;
              r1.z = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r3.yz, r3.w).x;
              r1.y = r1.z * 0.0625 + r1.y;
              r3.yzw = r5.xzz + r2.xyz;
              r1.z = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r3.yz, r3.w).x;
              r1.y = r1.z * 0.0625 + r1.y;
              r3.yzw = r4.xzz + r2.xyz;
              r1.z = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r3.yz, r3.w).x;
              r1.y = r1.z * 0.0625 + r1.y;
              r3.yzw = r5.zyz + r2.xyz;
              r1.z = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r3.yz, r3.w).x;
              r2.w = r1.z * 0.0625 + r1.y;
            } else {
              r1.xy = scene.MiscParameters4.xy * r1.xx;
              r1.z = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r2.xy, r2.z).x;
              r4.xy = -r1.xy * r3.xx;
              r4.z = 0;
              r3.yzw = r4.xyz + r2.xyz;
              r1.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r3.yz, r3.w).x;
              r1.w = scene.MiscParameters5.y * r1.w;
              r1.z = r1.z * scene.MiscParameters5.x + r1.w;
              r4.x = r1.x * r3.x;
              r4.y = -r1.y * r3.x;
              r4.z = 0;
              r3.yzw = r4.xyz + r2.xyz;
              r1.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r3.yz, r3.w).x;
              r1.z = r1.w * scene.MiscParameters5.y + r1.z;
              r4.x = -r1.x * r3.x;
              r4.y = r1.y * r3.x;
              r4.z = 0;
              r3.yzw = r4.xyz + r2.xyz;
              r1.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r3.yz, r3.w).x;
              r1.z = r1.w * scene.MiscParameters5.y + r1.z;
              r3.xy = r1.xy * r3.xx;
              r3.z = 0;
              r1.xyw = r3.xyz + r2.xyz;
              r1.x = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r1.xy, r1.w).x;
              r2.w = r1.x * scene.MiscParameters5.y + r1.z;
            }
          }
        }
      }
    }
  } else {
    r2.w = 1;
  }
  r1.x = cmp(v4.w < LightShadow0.m_splitDistances.x);
  r1.y = -LightShadow0.m_splitDistances.x + v4.w;
  r1.z = LightShadow0.m_splitDistances.y + -LightShadow0.m_splitDistances.x;
  r1.y = saturate(r1.y / r1.z);
  r1.x = r1.x ? 0 : r1.y;
  r1.y = r1.x * r1.x;
  r1.x = r1.x * r1.y;
  r0.w = r0.w ? r1.x : 1;
  r1.x = -scene.MiscParameters2.y + v4.y;
  r1.x = scene.MiscParameters2.x * abs(r1.x);
  r1.x = min(1, r1.x);
  r1.y = r1.x * r1.x;
  r1.x = r1.x * r1.y;
  r1.y = cmp(0 >= scene.MiscParameters2.x);
  r1.x = r1.y ? 0 : r1.x;
  r0.w = r1.x + r0.w;
  r1.x = dot(v5.xyz, Light0.m_direction.xyz);
  r1.x = r1.x * 0.5 + 0.5;
  r1.y = 0.400000006 * scene.MiscParameters4.w;
  r1.z = max(0, -v5.y);
  r1.y = -r1.y * r1.z + 0.400000006;
  r1.z = cmp(r1.y < r1.x);
  r1.x = r1.x + -r1.y;
  r1.y = 1 + -r1.y;
  r1.x = r1.x / r1.y;
  r1.x = r1.z ? r1.x : 0;
  r1.x = 1 + -r1.x;
  r1.y = r1.x * r1.x;
  r0.w = r1.x * r1.y + r0.w;
  r0.w = min(1, r0.w);
  r0.w = r2.w + r0.w;
  r0.w = min(1, r0.w);
  r1.x = 1 + -r0.w;
  r1.y = 1 + -scene.MiscParameters2.w;
  r0.w = r1.x * r1.y + r0.w;
  r1.x = 1 + -r0.w;
  r1.y = 1 + -GameMaterialEmission.w;
  r0.w = r1.x * r1.y + r0.w;
  r1.x = dot(v5.xyz, v5.xyz);
  r1.x = rsqrt(r1.x);
  r1.xyz = v5.xyz * r1.xxx;
  r2.xyz = max(float3(1,1,1), scene.GlobalAmbientColor.xyz);
  // r2.xyz = min(float3(1.5,1.5,1.5), r2.xyz);
  r0.w = 1 + -r0.w;
  r3.xyz = r2.xyz * scene.MiscParameters1.xyz + -r2.xyz;
  r2.xyz = r0.www * r3.xyz + r2.xyz;
  r2.xyz = v1.xyz * r2.xyz;
  r0.xyz = r2.xyz * r0.xyz;
  r0.xyz = r0.xyz * GameMaterialDiffuse.xyz + GameMaterialEmission.xyz;
  r0.xyz = v2.xyz + r0.xyz;
  r0.w = calculateLuminanceSRGB(r0.rgb);
  // r0.w = dot(r0.xyz, float3(0.298999995,0.587000012,0.114));
  r2.xyz = r0.www * scene.MonotoneMul.xyz + scene.MonotoneAdd.xyz;
  // r2.xyz = r2.xyz + -r0.xyz;
  // r0.xyz = GameMaterialMonotone * r2.xyz + r0.xyz;
  r0.rgb = lerp(r0.rgb, r2.rgb, GameMaterialMonotone);
  r2.xyz = BloomIntensity * r0.xyz;
  r0.w = calculateLuminanceSRGB(r2.rgb);
  // r0.w = dot(r2.xyz, float3(0.298999995,0.587000012,0.114));
  r0.w = -scene.MiscParameters2.z + r0.w;
  r0.w = max(0, r0.w);
  r0.w = 0.5 * r0.w;
  o0.w = min(1, r0.w);
  o1.xyz = r1.xyz * float3(0.5,0.5,0.5) + float3(0.5,0.5,0.5);
  r0.w = v6.z / v6.w;
  r1.x = 256 * r0.w;
  r1.x = trunc(r1.x);
  r0.w = r0.w * 256 + -r1.x;
  r1.w = 256 * r0.w;
  r1.y = trunc(r1.w);
  r1.z = r0.w * 256 + -r1.y;
  o2.xyz = float3(0.00390625,0.00390625,1) * r1.xyz;
  o0.xyz = r0.xyz;
  o1.w = 0.75;
  o2.w = 0;
  return;
}