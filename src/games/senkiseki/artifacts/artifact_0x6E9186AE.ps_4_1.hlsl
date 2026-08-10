// ---- Created with 3Dmigoto v1.3.16 on Fri Jul 25 18:48:23 2025
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
  float Shininess : packoffset(c84) = {0.5};
  float SpecularPower : packoffset(c84.y) = {50};
  float BlendMulScale3 : packoffset(c84.z) = {1};
  float2 ProjectionScale : packoffset(c85) = {1,1};
  float BloomIntensity : packoffset(c85.z) = {1};
  float MaskEps : packoffset(c85.w);
  float4 PointLightParams : packoffset(c86) = {0,2,1,1};
  float4 PointLightColor : packoffset(c87) = {1,0,0,0};
}

SamplerState LinearWrapSamplerState_s : register(s0);
SamplerState PointClampSamplerState_s : register(s1);
SamplerState DiffuseMapSamplerSampler_s : register(s3);
SamplerState NormalMapSamplerSampler_s : register(s4);
SamplerState SpecularMapSamplerSampler_s : register(s5);
SamplerState DiffuseMap2SamplerSampler_s : register(s6);
SamplerState SpecularMap2SamplerSampler_s : register(s7);
SamplerState NormalMap2SamplerSampler_s : register(s8);
SamplerState DiffuseMap3SamplerSampler_s : register(s9);
SamplerComparisonState LinearClampCmpSamplerState_s : register(s2);
Texture2D<float4> LowResDepthTexture : register(t0);
Texture2D<float4> LightShadowMap0 : register(t1);
Texture2D<float4> DiffuseMapSampler : register(t2);
Texture2D<float4> NormalMapSampler : register(t3);
Texture2D<float4> SpecularMapSampler : register(t4);
Texture2D<float4> DiffuseMap2Sampler : register(t5);
Texture2D<float4> SpecularMap2Sampler : register(t6);
Texture2D<float4> NormalMap2Sampler : register(t7);
Texture2D<float4> DiffuseMap3Sampler : register(t8);
Texture2D<float4> ProjectionMapSampler : register(t9);


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_POSITION0,
  float4 v1 : COLOR0,
  float4 v2 : COLOR1,
  float2 v3 : TEXCOORD0,
  float2 w3 : TEXCOORD2,
  float4 v4 : TEXCOORD1,
  float2 v5 : TEXCOORD3,
  float2 w5 : TEXCOORD7,
  float4 v6 : TEXCOORD4,
  float4 v7 : TEXCOORD6,
  float4 v8 : TEXCOORD10,
  out float4 o0 : SV_TARGET0,
  out float4 o1 : SV_TARGET1,
  out float4 o2 : SV_TARGET2)
{
  float4 r0,r1,r2,r3,r4,r5,r6,r7,r8;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.xyz = DiffuseMapSampler.Sample(DiffuseMapSamplerSampler_s, v3.xy).xyz;
  r1.xyzw = DiffuseMap2Sampler.Sample(DiffuseMap2SamplerSampler_s, w3.xy).xyzw;
  r0.w = UVaMUvColor.w * r1.w;
  r0.w = v1.w * r0.w;
  r1.xyz = r1.xyz * UVaMUvColor.xyz + -r0.xyz;
  r0.xyz = r0.www * r1.xyz + r0.xyz;
  r1.xyz = DiffuseMap3Sampler.Sample(DiffuseMap3SamplerSampler_s, w5.xy).xyz;
  r1.w = cmp(LightShadow0.m_splitDistances.y >= v4.w);
  if (r1.w != 0) {
    r2.xyz = Light0.m_direction.xyz * float3(0.0500000007,0.0500000007,0.0500000007) + v4.xyz;
    r2.xyz = v6.xyz * float3(0.0500000007,0.0500000007,0.0500000007) + r2.xyz;
    r1.w = cmp(v4.w < LightShadow0.m_splitDistances.x);
    if (r1.w != 0) {
      r2.w = 1;
      r3.x = dot(r2.xyzw, LightShadow0.m_split0Transform._m00_m10_m20_m30);
      r3.y = dot(r2.xyzw, LightShadow0.m_split0Transform._m01_m11_m21_m31);
      r3.z = dot(r2.xyzw, LightShadow0.m_split0Transform._m02_m12_m22_m32);
      r1.w = 25 / LightShadow0.m_splitDistances.y;
      r3.w = (int)scene.DuranteSettings.x & 16;
      if (r3.w != 0) {
        r4.xy = float2(0.000937499979,0.00187499996) * r1.ww;
        r3.w = -6 + r3.z;
        r4.xy = r4.xy * r3.ww;
        r4.xy = r4.xy / r3.zz;
        r3.w = dot(v0.xy, float2(0.0671105608,0.00583714992));
        r3.w = frac(r3.w);
        r3.w = 52.9829178 * r3.w;
        r3.w = frac(r3.w);
        r3.w = 6.28318548 * r3.w;
        r4.zw = float2(0,0);
        r5.x = 0;
        while (true) {
          r5.y = cmp((int)r5.x >= 16);
          if (r5.y != 0) break;
          r5.y = (int)r5.x;
          r5.z = 0.5 + r5.y;
          r5.z = sqrt(r5.z);
          r5.z = 0.25 * r5.z;
          r5.y = r5.y * 2.4000001 + r3.w;
          sincos(r5.y, r6.x, r7.x);
          r7.x = r7.x * r5.z;
          r7.y = r6.x * r5.z;
          r5.yz = r7.xy * r4.xy + r3.xy;
          r5.y = LightShadowMap0.SampleLevel(PointClampSamplerState_s, r5.yz, 0).x;
          r5.z = cmp(r5.y < r3.z);
          r6.y = r5.y + r4.w;
          r6.x = 1 + r4.z;
          r4.zw = r5.zz ? r6.xy : r4.zw;
          r5.x = (int)r5.x + 1;
        }
        r4.x = cmp(r4.z >= 1);
        if (r4.x != 0) {
          r4.xy = float2(0.000624999986,0.00124999997) * r1.ww;
          r4.z = r4.w / r4.z;
          r4.z = -r4.z + r3.z;
          r4.z = min(0.0700000003, r4.z);
          r4.z = 60 * r4.z;
          r4.xy = r4.zz * r4.xy;
          LightShadowMap0.GetDimensions(0, fDest.x, fDest.y, fDest.z);
          r4.zw = fDest.xy;
          r5.x = 0.5 / r4.z;
          r5.y = 1 / r4.w;
          r4.xy = max(r5.xy, r4.xy);
          r4.zw = float2(0,0);
          while (true) {
            r5.x = cmp((int)r4.w >= 16);
            if (r5.x != 0) break;
            r5.x = (int)r4.w;
            r5.y = 0.5 + r5.x;
            r5.y = sqrt(r5.y);
            r5.y = 0.25 * r5.y;
            r5.x = r5.x * 2.4000001 + r3.w;
            sincos(r5.x, r5.x, r6.x);
            r6.x = r6.x * r5.y;
            r6.y = r5.y * r5.x;
            r5.xy = r6.xy * r4.xy;
            r5.xy = r5.xy * float2(3,3) + r3.xy;
            r5.x = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r5.xy, r3.z).x;
            r4.z = r5.x + r4.z;
            r4.w = (int)r4.w + 1;
          }
          r3.w = 0.0625 * r4.z;
        } else {
          r3.w = 1;
        }
      } else {
        r4.x = (int)scene.DuranteSettings.x & 8;
        if (r4.x != 0) {
          r4.xy = float2(0.000375000003,0.000750000007) * r1.ww;
          r4.z = dot(v0.xy, float2(0.0671105608,0.00583714992));
          r4.z = frac(r4.z);
          r4.z = 52.9829178 * r4.z;
          r4.z = frac(r4.z);
          r4.z = 6.28318548 * r4.z;
          r4.w = 0;
          r5.x = 0;
          while (true) {
            r5.y = cmp((int)r5.x >= 16);
            if (r5.y != 0) break;
            r5.y = (int)r5.x;
            r5.z = 0.5 + r5.y;
            r5.z = sqrt(r5.z);
            r5.z = 0.25 * r5.z;
            r5.y = r5.y * 2.4000001 + r4.z;
            sincos(r5.y, r6.x, r7.x);
            r7.x = r7.x * r5.z;
            r7.y = r6.x * r5.z;
            r5.yz = r7.xy * r4.xy;
            r5.yz = r5.yz * float2(3,3) + r3.xy;
            r5.y = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r5.yz, r3.z).x;
            r4.w = r5.y + r4.w;
            r5.x = (int)r5.x + 1;
          }
          r3.w = 0.0625 * r4.w;
        } else {
          r4.x = dot(r2.xyzw, LightShadow0.m_split0Transform._m03_m13_m23_m33);
          r4.y = (int)scene.DuranteSettings.x & 4;
          if (r4.y != 0) {
            r4.yz = scene.MiscParameters4.xy * r4.xx;
            r4.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r3.xy, r3.z).x;
            r5.xy = -r4.yz * r1.ww;
            r5.z = 0;
            r5.xyz = r5.xyz + r3.xyz;
            r5.x = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r5.xy, r5.z).x;
            r5.x = 0.0625 * r5.x;
            r4.w = r4.w * 0.5 + r5.x;
            r5.x = r4.y * r1.w;
            r5.y = -r4.z * r1.w;
            r5.z = 0;
            r6.xyz = r5.xyz + r3.xyz;
            r5.x = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r6.xy, r6.z).x;
            r4.w = r5.x * 0.0625 + r4.w;
            r6.x = -r4.y * r1.w;
            r6.y = r4.z * r1.w;
            r6.z = 0;
            r7.xyz = r6.xyz + r3.xyz;
            r5.x = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r7.xy, r7.z).x;
            r4.w = r5.x * 0.0625 + r4.w;
            r7.xy = r4.yz * r1.ww;
            r7.z = 0;
            r8.xyz = r7.xyz + r3.xyz;
            r4.y = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r8.xy, r8.z).x;
            r4.y = r4.y * 0.0625 + r4.w;
            r5.xyz = r5.zyz + r3.xyz;
            r4.z = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r5.xy, r5.z).x;
            r4.y = r4.z * 0.0625 + r4.y;
            r5.xyz = r6.xzz + r3.xyz;
            r4.z = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r5.xy, r5.z).x;
            r4.y = r4.z * 0.0625 + r4.y;
            r5.xyz = r7.xzz + r3.xyz;
            r4.z = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r5.xy, r5.z).x;
            r4.y = r4.z * 0.0625 + r4.y;
            r5.xyz = r7.zyz + r3.xyz;
            r4.z = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r5.xy, r5.z).x;
            r3.w = r4.z * 0.0625 + r4.y;
          } else {
            r4.y = (int)scene.DuranteSettings.x & 2;
            if (r4.y != 0) {
              r4.xy = scene.MiscParameters4.xy * r4.xx;
              r4.z = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r3.xy, r3.z).x;
              r5.xy = -r4.xy * r1.ww;
              r5.z = 0;
              r5.xyz = r5.xyz + r3.xyz;
              r4.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r5.xy, r5.z).x;
              r4.w = scene.MiscParameters5.y * r4.w;
              r4.z = r4.z * scene.MiscParameters5.x + r4.w;
              r5.x = r4.x * r1.w;
              r5.y = -r4.y * r1.w;
              r5.z = 0;
              r5.xyz = r5.xyz + r3.xyz;
              r4.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r5.xy, r5.z).x;
              r4.z = r4.w * scene.MiscParameters5.y + r4.z;
              r5.x = -r4.x * r1.w;
              r5.y = r4.y * r1.w;
              r5.z = 0;
              r5.xyz = r5.xyz + r3.xyz;
              r4.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r5.xy, r5.z).x;
              r4.z = r4.w * scene.MiscParameters5.y + r4.z;
              r5.xy = r4.xy * r1.ww;
              r5.z = 0;
              r4.xyw = r5.xyz + r3.xyz;
              r1.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r4.xy, r4.w).x;
              r3.w = r1.w * scene.MiscParameters5.y + r4.z;
            } else {
              r3.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r3.xy, r3.z).x;
            }
          }
        }
      }
    } else {
      r2.w = 1;
      r3.x = dot(r2.xyzw, LightShadow0.m_split1Transform._m00_m10_m20_m30);
      r3.y = dot(r2.xyzw, LightShadow0.m_split1Transform._m01_m11_m21_m31);
      r3.z = dot(r2.xyzw, LightShadow0.m_split1Transform._m02_m12_m22_m32);
      r1.w = 10 / LightShadow0.m_splitDistances.y;
      r4.x = (int)scene.DuranteSettings.x & 16;
      if (r4.x != 0) {
        r4.xy = float2(0.000937499979,0.00187499996) * r1.ww;
        r4.z = -6 + r3.z;
        r4.xy = r4.xy * r4.zz;
        r4.xy = r4.xy / r3.zz;
        r4.z = dot(v0.xy, float2(0.0671105608,0.00583714992));
        r4.z = frac(r4.z);
        r4.z = 52.9829178 * r4.z;
        r4.z = frac(r4.z);
        r4.z = 6.28318548 * r4.z;
        r5.xy = float2(0,0);
        r4.w = 0;
        while (true) {
          r5.z = cmp((int)r4.w >= 16);
          if (r5.z != 0) break;
          r5.z = (int)r4.w;
          r5.w = 0.5 + r5.z;
          r5.w = sqrt(r5.w);
          r5.w = 0.25 * r5.w;
          r5.z = r5.z * 2.4000001 + r4.z;
          sincos(r5.z, r6.x, r7.x);
          r7.x = r7.x * r5.w;
          r7.y = r6.x * r5.w;
          r5.zw = r7.xy * r4.xy + r3.xy;
          r5.z = LightShadowMap0.SampleLevel(PointClampSamplerState_s, r5.zw, 0).x;
          r5.w = cmp(r5.z < r3.z);
          r6.y = r5.y + r5.z;
          r6.x = 1 + r5.x;
          r5.xy = r5.ww ? r6.xy : r5.xy;
          r4.w = (int)r4.w + 1;
        }
        r4.x = cmp(r5.x >= 1);
        if (r4.x != 0) {
          r4.xy = float2(0.000624999986,0.00124999997) * r1.ww;
          r4.w = r5.y / r5.x;
          r4.w = -r4.w + r3.z;
          r4.w = min(0.0700000003, r4.w);
          r4.w = 60 * r4.w;
          r4.xy = r4.ww * r4.xy;
          LightShadowMap0.GetDimensions(0, fDest.x, fDest.y, fDest.z);
          r5.xy = fDest.xy;
          r6.x = 0.5 / r5.x;
          r6.y = 1 / r5.y;
          r4.xy = max(r6.xy, r4.xy);
          r4.w = 0;
          r5.x = 0;
          while (true) {
            r5.y = cmp((int)r5.x >= 16);
            if (r5.y != 0) break;
            r5.y = (int)r5.x;
            r5.z = 0.5 + r5.y;
            r5.z = sqrt(r5.z);
            r5.z = 0.25 * r5.z;
            r5.y = r5.y * 2.4000001 + r4.z;
            sincos(r5.y, r6.x, r7.x);
            r7.x = r7.x * r5.z;
            r7.y = r6.x * r5.z;
            r5.yz = r7.xy * r4.xy;
            r5.yz = r5.yz * float2(3,3) + r3.xy;
            r5.y = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r5.yz, r3.z).x;
            r4.w = r5.y + r4.w;
            r5.x = (int)r5.x + 1;
          }
          r3.w = 0.0625 * r4.w;
        } else {
          r3.w = 1;
        }
      } else {
        r4.x = (int)scene.DuranteSettings.x & 8;
        if (r4.x != 0) {
          r4.xy = float2(0.000375000003,0.000750000007) * r1.ww;
          r4.z = dot(v0.xy, float2(0.0671105608,0.00583714992));
          r4.z = frac(r4.z);
          r4.z = 52.9829178 * r4.z;
          r4.z = frac(r4.z);
          r4.z = 6.28318548 * r4.z;
          r4.w = 0;
          r5.x = 0;
          while (true) {
            r5.y = cmp((int)r5.x >= 16);
            if (r5.y != 0) break;
            r5.y = (int)r5.x;
            r5.z = 0.5 + r5.y;
            r5.z = sqrt(r5.z);
            r5.z = 0.25 * r5.z;
            r5.y = r5.y * 2.4000001 + r4.z;
            sincos(r5.y, r6.x, r7.x);
            r7.x = r7.x * r5.z;
            r7.y = r6.x * r5.z;
            r5.yz = r7.xy * r4.xy;
            r5.yz = r5.yz * float2(3,3) + r3.xy;
            r5.y = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r5.yz, r3.z).x;
            r4.w = r5.y + r4.w;
            r5.x = (int)r5.x + 1;
          }
          r3.w = 0.0625 * r4.w;
        } else {
          r2.x = dot(r2.xyzw, LightShadow0.m_split1Transform._m03_m13_m23_m33);
          r2.y = (int)scene.DuranteSettings.x & 4;
          if (r2.y != 0) {
            r2.yz = scene.MiscParameters4.xy * r2.xx;
            r2.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r3.xy, r3.z).x;
            r4.xy = -r2.yz * r1.ww;
            r4.z = 0;
            r5.xyz = r4.xyz + r3.xyz;
            r4.y = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r5.xy, r5.z).x;
            r4.y = 0.0625 * r4.y;
            r2.w = r2.w * 0.5 + r4.y;
            r5.x = r2.y * r1.w;
            r5.y = -r2.z * r1.w;
            r5.z = 0;
            r6.xyz = r5.xyz + r3.xyz;
            r4.y = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r6.xy, r6.z).x;
            r2.w = r4.y * 0.0625 + r2.w;
            r6.x = -r2.y * r1.w;
            r6.y = r2.z * r1.w;
            r6.z = 0;
            r7.xyz = r6.xyz + r3.xyz;
            r4.y = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r7.xy, r7.z).x;
            r2.w = r4.y * 0.0625 + r2.w;
            r7.xy = r2.yz * r1.ww;
            r7.z = 0;
            r8.xyz = r7.xyz + r3.xyz;
            r2.y = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r8.xy, r8.z).x;
            r2.y = r2.y * 0.0625 + r2.w;
            r5.xyz = r5.zyz + r3.xyz;
            r2.z = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r5.xy, r5.z).x;
            r2.y = r2.z * 0.0625 + r2.y;
            r4.xyz = r4.xzz + r3.xyz;
            r2.z = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r4.xy, r4.z).x;
            r2.y = r2.z * 0.0625 + r2.y;
            r4.xyz = r7.xzz + r3.xyz;
            r2.z = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r4.xy, r4.z).x;
            r2.y = r2.z * 0.0625 + r2.y;
            r4.xyz = r6.zyz + r3.xyz;
            r2.z = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r4.xy, r4.z).x;
            r3.w = r2.z * 0.0625 + r2.y;
          } else {
            r2.y = (int)scene.DuranteSettings.x & 2;
            if (r2.y != 0) {
              r2.xy = scene.MiscParameters4.xy * r2.xx;
              r2.z = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r3.xy, r3.z).x;
              r4.xy = -r2.xy * r1.ww;
              r4.z = 0;
              r4.xyz = r4.xyz + r3.xyz;
              r2.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r4.xy, r4.z).x;
              r2.w = scene.MiscParameters5.y * r2.w;
              r2.z = r2.z * scene.MiscParameters5.x + r2.w;
              r4.x = r2.x * r1.w;
              r4.y = -r2.y * r1.w;
              r4.z = 0;
              r4.xyz = r4.xyz + r3.xyz;
              r2.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r4.xy, r4.z).x;
              r2.z = r2.w * scene.MiscParameters5.y + r2.z;
              r4.x = -r2.x * r1.w;
              r4.y = r2.y * r1.w;
              r4.z = 0;
              r4.xyz = r4.xyz + r3.xyz;
              r2.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r4.xy, r4.z).x;
              r2.z = r2.w * scene.MiscParameters5.y + r2.z;
              r4.xy = r2.xy * r1.ww;
              r4.z = 0;
              r2.xyw = r4.xyz + r3.xyz;
              r1.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r2.xy, r2.w).x;
              r3.w = r1.w * scene.MiscParameters5.y + r2.z;
            } else {
              r3.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r3.xy, r3.z).x;
            }
          }
        }
      }
    }
  } else {
    r3.w = 1;
  }
  r1.w = saturate(v4.w / LightShadow0.m_splitDistances.y);
  r2.x = r1.w * r1.w;
  r2.y = -scene.MiscParameters2.y + v4.y;
  r2.y = scene.MiscParameters2.x * abs(r2.y);
  r2.y = min(1, r2.y);
  r2.z = r2.y * r2.y;
  r2.y = r2.y * r2.z;
  r2.z = cmp(0 >= scene.MiscParameters2.x);
  r2.y = r2.z ? 0 : r2.y;
  r1.w = r1.w * r2.x + r2.y;
  r2.x = dot(v6.xyz, Light0.m_direction.xyz);
  r2.x = r2.x * 0.5 + 0.5;
  r2.y = 0.400000006 * scene.MiscParameters4.w;
  r2.z = max(0, -v6.y);
  r2.y = -r2.y * r2.z + 0.400000006;
  r2.z = cmp(r2.y < r2.x);
  r2.x = r2.x + -r2.y;
  r2.y = 1 + -r2.y;
  r2.x = r2.x / r2.y;
  r2.x = r2.z ? r2.x : 0;
  r2.x = 1 + -r2.x;
  r2.y = r2.x * r2.x;
  r1.w = r2.x * r2.y + r1.w;
  r1.w = min(1, r1.w);
  r1.w = r3.w + r1.w;
  r1.w = min(1, r1.w);
  r2.xy = ProjectionMapSampler.Sample(LinearWrapSamplerState_s, v5.xy).xw;
  r2.x = -r2.x * r2.y + 1;
  r1.w = min(r2.x, r1.w);
  r2.x = 1 + -r1.w;
  r2.y = 1 + -scene.MiscParameters2.w;
  r1.w = r2.x * r2.y + r1.w;
  r2.x = 1 + -r1.w;
  r2.y = 1 + -GameMaterialEmission.w;
  r1.w = r2.x * r2.y + r1.w;
  r2.x = SpecularMapSampler.Sample(SpecularMapSamplerSampler_s, v3.xy).x;
  r2.y = SpecularMap2Sampler.Sample(SpecularMap2SamplerSampler_s, w3.xy).x;
  r2.y = r2.y + -r2.x;
  r2.x = r0.w * r2.y + r2.x;
  r2.yzw = NormalMapSampler.Sample(NormalMapSamplerSampler_s, v3.xy).xyz;
  r2.yzw = r2.yzw * float3(2,2,2) + float3(-1,-1,-1);
  r3.x = dot(v7.xyz, v7.xyz);
  r3.x = rsqrt(r3.x);
  r3.xyz = v7.xyz * r3.xxx;
  r3.w = dot(v6.xyz, v6.xyz);
  r3.w = rsqrt(r3.w);
  r4.xyz = v6.xyz * r3.www;
  r5.xyz = r4.zxy * r3.yzx;
  r5.xyz = r4.yzx * r3.zxy + -r5.xyz;
  r6.xy = cmp(v3.xy < float2(0,0));
  r6.xy = r6.xy ? float2(-1,-1) : float2(1,1);
  r2.y = r6.x * r2.y;
  r6.xzw = r5.xyz * r2.zzz;
  r6.xzw = r2.yyy * r3.xyz + r6.xzw;
  r2.yzw = r2.www * r4.xyz + r6.xzw;
  r3.w = dot(r2.yzw, r2.yzw);
  r3.w = rsqrt(r3.w);
  r2.yzw = r3.www * r2.yzw;
  r6.xzw = NormalMap2Sampler.Sample(NormalMap2SamplerSampler_s, w3.xy).xyz;
  r6.xzw = r6.xzw * float3(2,2,2) + float3(-1,-1,-1);
  r3.w = r6.x * r6.y;
  r5.xyz = r6.zzz * r5.xyz;
  r3.xyz = r3.www * r3.xyz + r5.xyz;
  r3.xyz = r6.www * r4.xyz + r3.xyz;
  r3.w = dot(r3.xyz, r3.xyz);
  r3.w = rsqrt(r3.w);
  r3.xyz = r3.xyz * r3.www + -r2.yzw;
  r2.yzw = r0.www * r3.xyz + r2.yzw;
  r0.w = dot(r2.yzw, r2.yzw);
  r0.w = rsqrt(r0.w);
  r2.yzw = r2.yzw * r0.www;
  r3.xyz = scene.EyePosition.xyz + -v4.xyz;
  r0.w = dot(r3.xyz, r3.xyz);
  r0.w = rsqrt(r0.w);
  r4.xyz = r3.xyz * r0.www;
  r3.w = saturate(dot(r2.yzw, r4.xyz));
  r2.x = Shininess * r2.x;
  r4.x = dot(Light0.m_direction.xyz, r2.yzw);
  r4.x = r4.x * 0.5 + 0.5;
  r4.x = r4.x * r4.x;
  r3.xyz = r3.xyz * r0.www + Light0.m_direction.xyz;
  r0.w = dot(r3.xyz, r3.xyz);
  r0.w = rsqrt(r0.w);
  r3.xyz = r3.xyz * r0.www;
  r0.w = saturate(dot(r2.yzw, r3.xyz));
  r0.w = log2(r0.w);
  r0.w = SpecularPower * r0.w;
  r0.w = exp2(r0.w);
  r0.w = min(1, r0.w);
  r0.w = r0.w * r2.x;
  r3.xyz = Light0.m_colorIntensity.xyz * r4.xxx + scene.GlobalAmbientColor.xyz;
  r3.xyz = min(float3(1.5,1.5,1.5), r3.xyz);
  r3.xyz = Light0.m_colorIntensity.xyz * r0.www + r3.xyz;
  r0.w = 1 + -r1.w;
  r4.xyz = r3.xyz * scene.MiscParameters1.xyz + -r3.xyz;
  r3.xyz = r0.www * r4.xyz + r3.xyz;
  r0.xyz = r1.xyz * r0.xyz;
  r0.xyz = BlendMulScale3 * r0.xyz;
  r1.xyz = v1.xyz * r3.xyz;
  r0.xyz = r1.xyz * r0.xyz;
  r0.w = 1 + -r3.w;
  r0.w = log2(r0.w);
  r0.w = PointLightColor.x * r0.w;
  r0.w = exp2(r0.w);
  r0.w = -1 + r0.w;
  r0.w = PointLightColor.y * r0.w + 1;
  r1.xyz = GameMaterialEmission.xyz * r0.www;
  r0.xyz = r0.xyz * GameMaterialDiffuse.xyz + r1.xyz;
  r0.w = cmp(0 < scene.MiscParameters6.w);
  if (r0.w != 0) {
    r1.xy = GlobalTexcoordFactor * scene.MiscParameters6.xy;
    r1.xz = r1.xy * float2(30,30) + v4.xz;
    r1.y = v4.y;
    r1.xyz = scene.MiscParameters6.zzz * r1.xyz;
    r3.x = LowResDepthTexture.SampleLevel(LinearWrapSamplerState_s, r1.xy, 0).x;
    r3.y = LowResDepthTexture.SampleLevel(LinearWrapSamplerState_s, r1.xz, 0).x;
    r3.z = LowResDepthTexture.SampleLevel(LinearWrapSamplerState_s, r1.yz, 0).x;
    r0.w = dot(r3.xyz, float3(0.333299994,0.333299994,0.333299994));
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
  r1.x = calculateLuminanceSRGB(r0.xyz);
  r1.xyz = r1.xxx * scene.MonotoneMul.xyz + scene.MonotoneAdd.xyz;
  r1.xyz = r1.xyz + -r0.xyz;
  r0.xyz = GameMaterialMonotone * r1.xyz + r0.xyz;
  r1.xyz = BloomIntensity * r0.xyz;
  r1.x = calculateLuminanceSRGB(r1.xyz);
  r1.x = -scene.MiscParameters2.z + r1.x;
  r1.x = max(0, r1.x);
  r1.x = 0.5 * r1.x;
  r1.x = min(1, r1.x);
  o0.w = r1.x * r0.w;
  o1.xyz = r2.yzw * float3(0.5,0.5,0.5) + float3(0.5,0.5,0.5);
  o1.w = 0.466666698 + MaskEps;
  r0.w = v8.z / v8.w;
  r1.x = 256 * r0.w;
  r1.x = trunc(r1.x);
  r0.w = r0.w * 256 + -r1.x;
  r1.w = 256 * r0.w;
  r1.y = trunc(r1.w);
  r1.z = r0.w * 256 + -r1.y;
  o2.xyz = float3(0.00390625,0.00390625,1) * r1.xyz;
  o0.xyz = r0.xyz;
  o2.w = MaskEps;

  o0.rgb = clamp(o0.rgb, 0.f, shader_injection.safe_clamp);;
  o1 = saturate(o1);
  o2 = saturate(o2);
  return;
}