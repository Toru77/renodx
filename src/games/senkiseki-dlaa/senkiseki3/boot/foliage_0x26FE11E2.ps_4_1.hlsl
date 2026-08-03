// ---- Created with 3Dmigoto v1.4.1 on Fri Jul 31 21:37:45 2026

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
  float AlphaThreshold : packoffset(c75) = {0.5};
  float3 ShadowColorShift : packoffset(c75.y) = {0.100000001,0.0199999996,0.0199999996};
  float Shininess : packoffset(c76) = {0.5};
  float SpecularPower : packoffset(c76.y) = {50};
  float2 WindyGrassDirection : packoffset(c76.z) = {0,0};
  float WindyGrassSpeed : packoffset(c77) = {0.100000001};
  float WindyGrassHomogenity : packoffset(c77.y) = {2};
  float WindyGrassScale : packoffset(c77.z) = {1};
  float BloomIntensity : packoffset(c77.w) = {1};
  float4 PointLightParams : packoffset(c78) = {0,0,0,0};
  float4 PointLightColor : packoffset(c79) = {0,0,0,0};
}

SamplerState PointWrapSamplerState_s : register(s0);
SamplerState PointClampSamplerState_s : register(s1);
SamplerState DiffuseMapSamplerSampler_s : register(s3);
SamplerState SpecularMapSamplerSampler_s : register(s4);
SamplerComparisonState LinearClampCmpSamplerState_s : register(s2);
Texture2D<float4> DitherNoiseTexture : register(t0);
Texture2D<float4> LightShadowMap0 : register(t1);
Texture2D<float4> DiffuseMapSampler : register(t2);
Texture2D<float4> SpecularMapSampler : register(t3);


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_POSITION0,
  float4 v1 : COLOR0,
  float4 v2 : COLOR1,
  float4 v3 : TEXCOORD0,
  float4 v4 : TEXCOORD1,
  float4 v5 : TEXCOORD4,
  float4 v6 : TEXCOORD9,
  float4 v7 : TEXCOORD10,
  uint v8 : SV_SampleIndex0,
  out float4 o0 : SV_TARGET0,
  out float4 o1 : SV_TARGET1,
  out float4 o2 : SV_TARGET2)
{
  const float4 icb[] = { { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0.250000, 0.250000, 0, 0},
                              { -0.250000, -0.250000, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { -0.125000, -0.375000, 0, 0},
                              { 0.375000, -0.125000, 0, 0},
                              { -0.375000, 0.125000, 0, 0},
                              { 0.125000, 0.375000, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0, 0, 0, 0},
                              { 0.062500, -0.187500, 0, 0},
                              { -0.062500, 0.187500, 0, 0},
                              { 0.312500, 0.062500, 0, 0},
                              { -0.187500, -0.312500, 0, 0},
                              { -0.312500, 0.312500, 0, 0},
                              { -0.437500, -0.062500, 0, 0},
                              { 0.187500, 0.437500, 0, 0},
                              { 0.437500, -0.437500, 0, 0} };
  float4 r0,r1,r2,r3,r4,r5,r6,r7;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.xy = float2(0.25,0.25) * v0.xy;
  r0.x = DitherNoiseTexture.Sample(PointWrapSamplerState_s, r0.xy).x;
  r0.x = v6.x + -r0.x;
  r0.x = cmp(r0.x < 0);
  if (r0.x != 0) discard;
  r0.x = (int)DuranteSettings.x & 1;
  if (r0.x != 0) {
    r0.xy = ddx(v3.xy);
    r0.zw = ddy(v3.xy);
    r1.x = (uint)DuranteSettings.y << 3;
    r1.x = (int)r1.x + (int)v8.x;
    r0.zw = icb[r1.x+0].yy * r0.zw;
    r0.xy = icb[r1.x+0].xx * r0.xy + r0.zw;
    r0.xy = v3.xy + r0.xy;
  } else {
    r0.xy = v3.xy;
  }
  r1.xyzw = DiffuseMapSampler.Sample(DiffuseMapSamplerSampler_s, r0.xy).xyzw;
  r0.z = AlphaThreshold * v1.w;
  r0.z = r1.w * v1.w + -r0.z;
  r0.z = cmp(r0.z < 0);
  if (r0.z != 0) discard;
  r0.z = cmp(LightShadow0.m_splitDistances.y >= v4.w);
  if (r0.z != 0) {
    r2.xyz = Light0.m_direction.xyz * float3(0.0500000007,0.0500000007,0.0500000007) + v4.xyz;
    r2.xyz = v5.xyz * float3(0.0500000007,0.0500000007,0.0500000007) + r2.xyz;
    r0.w = cmp(v4.w < LightShadow0.m_splitDistances.x);
    if (r0.w != 0) {
      r2.w = 1;
      r3.x = dot(r2.xyzw, LightShadow0.m_split0Transform._m00_m10_m20_m30);
      r3.y = dot(r2.xyzw, LightShadow0.m_split0Transform._m01_m11_m21_m31);
      r3.z = dot(r2.xyzw, LightShadow0.m_split0Transform._m02_m12_m22_m32);
      r0.w = (int)DuranteSettings.x & 2;
      if (r0.w != 0) {
        r0.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r3.xy, r3.z).x;
      } else {
        r1.w = 25 / LightShadow0.m_splitDistances.y;
        r3.w = (int)DuranteSettings.x & 16;
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
            r4.zw = float2(0.5,0.5) / r4.zw;
            r4.xy = max(r4.zw, r4.xy);
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
            r0.w = 0.0625 * r4.z;
          } else {
            r0.w = 1;
          }
        } else {
          r3.w = (int)DuranteSettings.x & 8;
          if (r3.w != 0) {
            r4.xy = float2(0.000375000003,0.000750000007) * r1.ww;
            r3.w = dot(v0.xy, float2(0.0671105608,0.00583714992));
            r3.w = frac(r3.w);
            r3.w = 52.9829178 * r3.w;
            r3.w = frac(r3.w);
            r3.w = 6.28318548 * r3.w;
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
            r0.w = 0.0625 * r4.z;
          } else {
            r3.w = dot(r2.xyzw, LightShadow0.m_split0Transform._m03_m13_m23_m33);
            r4.x = (int)DuranteSettings.x & 4;
            if (r4.x != 0) {
              r4.xy = scene.MiscParameters4.xy * r3.ww;
              r4.z = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r3.xy, r3.z).x;
              r5.xy = -r4.xy * r1.ww;
              r5.z = 0;
              r5.xyz = r5.xyz + r3.xyz;
              r4.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r5.xy, r5.z).x;
              r4.w = 0.0625 * r4.w;
              r4.z = r4.z * 0.5 + r4.w;
              r5.x = r4.x * r1.w;
              r5.y = -r4.y * r1.w;
              r5.z = 0;
              r6.xyz = r5.xyz + r3.xyz;
              r4.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r6.xy, r6.z).x;
              r4.z = r4.w * 0.0625 + r4.z;
              r6.x = -r4.x * r1.w;
              r6.y = r4.y * r1.w;
              r6.z = 0;
              r7.xyz = r6.xyz + r3.xyz;
              r4.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r7.xy, r7.z).x;
              r4.z = r4.w * 0.0625 + r4.z;
              r7.xy = r4.xy * r1.ww;
              r7.z = 0;
              r4.xyw = r7.xyz + r3.xyz;
              r4.x = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r4.xy, r4.w).x;
              r4.x = r4.x * 0.0625 + r4.z;
              r4.yzw = r5.zyz + r3.xyz;
              r4.y = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r4.yz, r4.w).x;
              r4.x = r4.y * 0.0625 + r4.x;
              r4.yzw = r6.xzz + r3.xyz;
              r4.y = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r4.yz, r4.w).x;
              r4.x = r4.y * 0.0625 + r4.x;
              r4.yzw = r7.xzz + r3.xyz;
              r4.y = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r4.yz, r4.w).x;
              r4.x = r4.y * 0.0625 + r4.x;
              r4.yzw = r7.zyz + r3.xyz;
              r4.y = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r4.yz, r4.w).x;
              r0.w = r4.y * 0.0625 + r4.x;
            } else {
              r4.xy = scene.MiscParameters4.xy * r3.ww;
              r3.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r3.xy, r3.z).x;
              r5.xy = -r4.xy * r1.ww;
              r5.z = 0;
              r5.xyz = r5.xyz + r3.xyz;
              r4.z = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r5.xy, r5.z).x;
              r4.z = scene.MiscParameters5.y * r4.z;
              r3.w = r3.w * scene.MiscParameters5.x + r4.z;
              r5.x = r4.x * r1.w;
              r5.y = -r4.y * r1.w;
              r5.z = 0;
              r5.xyz = r5.xyz + r3.xyz;
              r4.z = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r5.xy, r5.z).x;
              r3.w = r4.z * scene.MiscParameters5.y + r3.w;
              r5.x = -r4.x * r1.w;
              r5.y = r4.y * r1.w;
              r5.z = 0;
              r5.xyz = r5.xyz + r3.xyz;
              r4.z = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r5.xy, r5.z).x;
              r3.w = r4.z * scene.MiscParameters5.y + r3.w;
              r4.xy = r4.xy * r1.ww;
              r4.z = 0;
              r3.xyz = r4.xyz + r3.xyz;
              r1.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r3.xy, r3.z).x;
              r0.w = r1.w * scene.MiscParameters5.y + r3.w;
            }
          }
        }
      }
    } else {
      r2.w = 1;
      r3.x = dot(r2.xyzw, LightShadow0.m_split1Transform._m00_m10_m20_m30);
      r3.y = dot(r2.xyzw, LightShadow0.m_split1Transform._m01_m11_m21_m31);
      r3.z = dot(r2.xyzw, LightShadow0.m_split1Transform._m02_m12_m22_m32);
      r1.w = (int)DuranteSettings.x & 2;
      if (r1.w != 0) {
        r0.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r3.xy, r3.z).x;
      } else {
        r1.w = 10 / LightShadow0.m_splitDistances.y;
        r3.w = (int)DuranteSettings.x & 16;
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
            r4.zw = float2(0.5,0.5) / r4.zw;
            r4.xy = max(r4.zw, r4.xy);
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
            r0.w = 0.0625 * r4.z;
          } else {
            r0.w = 1;
          }
        } else {
          r3.w = (int)DuranteSettings.x & 8;
          if (r3.w != 0) {
            r4.xy = float2(0.000375000003,0.000750000007) * r1.ww;
            r3.w = dot(v0.xy, float2(0.0671105608,0.00583714992));
            r3.w = frac(r3.w);
            r3.w = 52.9829178 * r3.w;
            r3.w = frac(r3.w);
            r3.w = 6.28318548 * r3.w;
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
            r0.w = 0.0625 * r4.z;
          } else {
            r2.x = dot(r2.xyzw, LightShadow0.m_split1Transform._m03_m13_m23_m33);
            r2.y = (int)DuranteSettings.x & 4;
            if (r2.y != 0) {
              r2.yz = scene.MiscParameters4.xy * r2.xx;
              r2.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r3.xy, r3.z).x;
              r4.xy = -r2.yz * r1.ww;
              r4.z = 0;
              r4.xyz = r4.xyz + r3.xyz;
              r3.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r4.xy, r4.z).x;
              r3.w = 0.0625 * r3.w;
              r2.w = r2.w * 0.5 + r3.w;
              r4.x = r2.y * r1.w;
              r4.y = -r2.z * r1.w;
              r4.z = 0;
              r5.xyz = r4.xyz + r3.xyz;
              r3.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r5.xy, r5.z).x;
              r2.w = r3.w * 0.0625 + r2.w;
              r5.x = -r2.y * r1.w;
              r5.y = r2.z * r1.w;
              r5.z = 0;
              r6.xyz = r5.xyz + r3.xyz;
              r3.w = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r6.xy, r6.z).x;
              r2.w = r3.w * 0.0625 + r2.w;
              r6.xy = r2.yz * r1.ww;
              r6.z = 0;
              r7.xyz = r6.xyz + r3.xyz;
              r2.y = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r7.xy, r7.z).x;
              r2.y = r2.y * 0.0625 + r2.w;
              r4.xyz = r4.zyz + r3.xyz;
              r2.z = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r4.xy, r4.z).x;
              r2.y = r2.z * 0.0625 + r2.y;
              r4.xyz = r5.xzz + r3.xyz;
              r2.z = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r4.xy, r4.z).x;
              r2.y = r2.z * 0.0625 + r2.y;
              r4.xyz = r6.xzz + r3.xyz;
              r2.z = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r4.xy, r4.z).x;
              r2.y = r2.z * 0.0625 + r2.y;
              r4.xyz = r6.zyz + r3.xyz;
              r2.z = LightShadowMap0.SampleCmpLevelZero(LinearClampCmpSamplerState_s, r4.xy, r4.z).x;
              r0.w = r2.z * 0.0625 + r2.y;
            } else {
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
              r0.w = r1.w * scene.MiscParameters5.y + r2.z;
            }
          }
        }
      }
    }
  } else {
    r0.w = 1;
  }
  r1.w = cmp(v4.w < LightShadow0.m_splitDistances.x);
  r2.x = -LightShadow0.m_splitDistances.x + v4.w;
  r2.y = LightShadow0.m_splitDistances.y + -LightShadow0.m_splitDistances.x;
  r2.x = saturate(r2.x / r2.y);
  r1.w = r1.w ? 0 : r2.x;
  r2.x = r1.w * r1.w;
  r1.w = r2.x * r1.w;
  r0.z = r0.z ? r1.w : 1;
  r1.w = -scene.MiscParameters2.y + v4.y;
  r1.w = scene.MiscParameters2.x * abs(r1.w);
  r1.w = min(1, r1.w);
  r2.x = r1.w * r1.w;
  r1.w = r2.x * r1.w;
  r2.x = cmp(0 >= scene.MiscParameters2.x);
  r1.w = r2.x ? 0 : r1.w;
  r0.z = r1.w + r0.z;
  r1.w = dot(v5.xyz, Light0.m_direction.xyz);
  r1.w = r1.w * 0.5 + 0.5;
  r2.x = 0.400000006 * scene.MiscParameters4.w;
  r2.y = max(0, -v5.y);
  r2.x = -r2.x * r2.y + 0.400000006;
  r2.y = cmp(r2.x < r1.w);
  r1.w = -r2.x + r1.w;
  r2.x = 1 + -r2.x;
  r1.w = r1.w / r2.x;
  r1.w = r2.y ? r1.w : 0;
  r1.w = 1 + -r1.w;
  r2.x = r1.w * r1.w;
  r0.z = r1.w * r2.x + r0.z;
  r0.z = min(1, r0.z);
  r0.z = r0.w + r0.z;
  r0.z = min(1, r0.z);
  r0.w = 1 + -r0.z;
  r1.w = 1 + -scene.MiscParameters2.w;
  r0.z = r0.w * r1.w + r0.z;
  r0.w = 1 + -r0.z;
  r1.w = 1 + -GameMaterialEmission.w;
  r0.z = r0.w * r1.w + r0.z;
  r0.x = SpecularMapSampler.Sample(SpecularMapSamplerSampler_s, r0.xy).x;
  r0.y = 1 + r0.x;
  r0.y = 0.5 * r0.y;
  r2.xyz = Light0.m_colorIntensity.xyz * r0.yyy;
  r0.y = r0.z * r0.z;
  r2.xyz = r2.xyz * r0.yyy;
  r0.y = dot(v5.xyz, v5.xyz);
  r0.y = rsqrt(r0.y);
  r3.xyz = v5.xyz * r0.yyy;
  r4.xyz = scene.EyePosition.xyz + -v4.xyz;
  r0.y = dot(r4.xyz, r4.xyz);
  r0.y = rsqrt(r0.y);
  r0.w = dot(Light0.m_direction.xyz, r3.xyz);
  r0.w = r0.w * 0.5 + 0.5;
  r4.xyz = r4.xyz * r0.yyy + Light0.m_direction.xyz;
  r0.y = dot(r4.xyz, r4.xyz);
  r0.y = rsqrt(r0.y);
  r4.xyz = r4.xyz * r0.yyy;
  r0.y = saturate(dot(r3.xyz, r4.xyz));
  r0.y = log2(r0.y);
  // ASM: mul r0.xy, r0.xyxx, cb0[76].xyxx  (r0.x *= Shininess, r0.y *= SpecularPower)
  r0.xy = float2(Shininess, SpecularPower) * r0.xy;
  r0.y = exp2(r0.y);
  r0.y = min(1, r0.y);
  r0.xw = r0.yw * r0.xw;
  r4.xyz = Light0.m_colorIntensity.xyz * r0.www + scene.GlobalAmbientColor.xyz;
  r4.xyz = min(float3(1.5,1.5,1.5), r4.xyz);
  r0.xyw = Light0.m_colorIntensity.xyz * r0.xxx + r4.xyz;
  r0.z = 1 + -r0.z;
  r4.xyz = r0.xyw * scene.MiscParameters1.xyz + -r0.xyw;
  r0.xyz = r0.zzz * r4.xyz + r0.xyw;
  r2.xyz = r2.xyz + r2.xyz;
  r2.xyz = max(float3(1,1,1), r2.xyz);
  r4.xyz = min(float3(1,1,1), r0.xyz);
  r4.xyz = float3(1,1,1) + -r4.xyz;
  r4.xyz = ShadowColorShift.xyz * r4.xyz;
  r0.xyz = r4.xyz * r2.xyz + r0.xyz;
  r0.xyz = v1.xyz * r0.xyz;
  r0.xyz = r1.xyz * r0.xyz;
  r0.xyz = r0.xyz * GameMaterialDiffuse.xyz + GameMaterialEmission.xyz;
  r0.xyz = v2.xyz + r0.xyz;
  r1.xyz = scene.FogColor.xyz + -r0.xyz;
  r0.xyz = v2.www * r1.xyz + r0.xyz;
  r0.w = dot(r0.xyz, float3(0.298999995,0.587000012,0.114));
  r1.xyz = r0.www * scene.MonotoneMul.xyz + scene.MonotoneAdd.xyz;
  r1.xyz = r1.xyz + -r0.xyz;
  r0.xyz = GameMaterialMonotone * r1.xyz + r0.xyz;
  r1.xyz = BloomIntensity * r0.xyz;
  r0.w = dot(r1.xyz, float3(0.298999995,0.587000012,0.114));
  r0.w = -scene.MiscParameters2.z + r0.w;
  r0.w = max(0, r0.w);
  r0.w = 0.5 * r0.w;
  o0.w = min(1, r0.w);
  o1.xyz = r3.xyz * float3(0.5,0.5,0.5) + float3(0.5,0.5,0.5);
  r0.w = v7.z / v7.w;
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