// ---- Created with 3Dmigoto v1.4.1 on Sun Mar 22 00:32:12 2026

cbuffer cb_scene : register(b0)
{
  float4x4 view_g : packoffset(c0);
  float4x4 viewInv_g : packoffset(c4);
  float4x4 proj_g : packoffset(c8);
  float4x4 projInv_g : packoffset(c12);
  float4x4 viewProj_g : packoffset(c16);
  float4x4 viewProjInv_g : packoffset(c20);
  float2 vpSize_g : packoffset(c24);
  float2 invVPSize_g : packoffset(c24.z);
  float3 lightColor_g : packoffset(c25);
  float disableMapObjNearFade_g : packoffset(c25.w);
  float3 lightDirection_g : packoffset(c26);
  float gameTime_g : packoffset(c26.w);
  float3 sceneShadowColor_g : packoffset(c27);
  int shadowmapCascadeCount_g : packoffset(c27.w);
  float3 windDirection_g : packoffset(c28);
  float sceneTime_g : packoffset(c28.w);
  float2 lightTileSizeInv_g : packoffset(c29);
  float fogNearDistance_g : packoffset(c29.z);
  float fogFadeRangeInv_g : packoffset(c29.w);
  float3 fogColor_g : packoffset(c30);
  float fogIntensity_g : packoffset(c30.w);
  float fogHeight_g : packoffset(c31);
  float fogHeightRangeInv_g : packoffset(c31.y);
  float windWaveTime_g : packoffset(c31.z);
  float windWaveFrequency_g : packoffset(c31.w);
  uint localLightProbeCount_g : packoffset(c32);
  float lightSpecularGlossiness_g : packoffset(c32.y);
  float lightSpecularIntensity_g : packoffset(c32.z);
  float localShadowResolutionInv_g : packoffset(c32.w);
  float4x4 ditherMtx_g : packoffset(c33);
  float4 lightProbe_g[9] : packoffset(c37);
  float3 chrLightDir_g : packoffset(c46);
  float windForce_g : packoffset(c46.w);
  float4 mapColor_g : packoffset(c47);
  float4 clipPlane_g : packoffset(c48);
  float2 resolutionScaling_g : packoffset(c49);
  float2 invShadowSize_g : packoffset(c49.z);
  float3 chrShadowColor_g : packoffset(c50);
  float shadowFadeNear_g : packoffset(c50.w);
  float4 frustumPlanes_g[6] : packoffset(c51);
  float3 shadowSplitDistance_g : packoffset(c57);
  float shadowFadeRangeInv_g : packoffset(c57.w);
  float4x4 shadowMtx_g[4] : packoffset(c58);
  float4x4 prevViewProj_g : packoffset(c74);
  float2 jitterDiff_g : packoffset(c78);
}

SamplerState samPoint_s : register(s0);
SamplerComparisonState SmplShadow_s : register(s13);
Texture2D<float4> depthTexture : register(t0);
Texture2DArray<float4> shadowMaps : register(t16);


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_Position0,
  float4 v1 : TEXCOORD0,
  out float4 o0 : SV_Target0)
{
  const float4 icb[] = { { 1.000000, 0, 0, 0},
                              { 0, 1.000000, 0, 0},
                              { 0, 0, 1.000000, 0},
                              { 0, 0, 0, 1.000000},
                              { -0.840520, -0.073954, 0, 0},
                              { -0.326235, -0.405830, 0, 0},
                              { -0.698464, 0.457259, 0, 0},
                              { -0.203356, 0.620585, 0, 0},
                              { 0.963450, -0.194353, 0, 0},
                              { 0.473434, -0.480026, 0, 0},
                              { 0.519454, 0.767034, 0, 0},
                              { 0.185461, -0.894523, 0, 0},
                              { 0.507351, 0.064963, 0, 0},
                              { -0.321932, 0.595435, 0, 0} };
  float4 r0,r1,r2,r3,r4;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.z = depthTexture.SampleLevel(samPoint_s, v1.xy, 0).x;
  r0.xy = v1.zw * float2(2,-2) + float2(-1,1);
  r0.w = 1;
  r1.x = dot(r0.xyzw, viewProjInv_g._m00_m10_m20_m30);
  r1.y = dot(r0.xyzw, viewProjInv_g._m01_m11_m21_m31);
  r1.z = dot(r0.xyzw, viewProjInv_g._m02_m12_m22_m32);
  r1.w = dot(r0.xyzw, viewProjInv_g._m03_m13_m23_m33);
  r0.xyzw = r1.xyzw / r1.wwww;
  r1.x = -viewInv_g._m30;
  r1.y = -viewInv_g._m31;
  r1.z = -viewInv_g._m32;
  r1.xyz = r1.xyz + r0.xyz;
  r1.x = dot(r1.xyz, r1.xyz);
  r1.x = sqrt(r1.x);
  r1.y = cmp(4 == shadowmapCascadeCount_g);
  r1.z = shadowSplitDistance_g.z + -5;
  r1.z = cmp(r1.z < r1.x);
  r1.y = r1.z ? r1.y : 0;
  if (r1.y != 0) {
    r2.x = dot(r0.xyzw, shadowMtx_g[3]._m00_m10_m20_m30);
    r2.y = dot(r0.xyzw, shadowMtx_g[3]._m01_m11_m21_m31);
    r2.z = dot(r0.xyzw, shadowMtx_g[3]._m02_m12_m22_m32);
    r1.y = dot(r0.xyzw, shadowMtx_g[3]._m03_m13_m23_m33);
    r1.yzw = r2.xyz / r1.yyy;
    r2.xy = cmp(r1.yz < float2(0,0));
    r2.zw = cmp(float2(1,1) < r1.yz);
    r2.x = (int)r2.z | (int)r2.x;
    r2.x = (int)r2.y | (int)r2.x;
    r2.x = (int)r2.w | (int)r2.x;
    if (r2.x != 0) {
      r2.x = 1;
    } else {
      r3.z = 3;
      r2.yz = float2(0,0);
      while (true) {
        r2.w = cmp((int)r2.z >= 10);
        if (r2.w != 0) break;
        r3.xy = saturate(icb[r2.z+4].xy * invShadowSize_g.xy + r1.yz);
        r2.w = shadowMaps.SampleCmpLevelZero(SmplShadow_s, r3.xyz, r1.w).x;
        r2.y = r2.y + r2.w;
        r2.z = (int)r2.z + 1;
      }
      r2.x = 0.100000001 * r2.y;
    }
    r1.y = cmp(r1.x < shadowSplitDistance_g.z);
    if (r1.y != 0) {
      r3.x = dot(r0.xyzw, shadowMtx_g[2]._m00_m10_m20_m30);
      r3.y = dot(r0.xyzw, shadowMtx_g[2]._m01_m11_m21_m31);
      r3.z = dot(r0.xyzw, shadowMtx_g[2]._m02_m12_m22_m32);
      r1.y = dot(r0.xyzw, shadowMtx_g[2]._m03_m13_m23_m33);
      r1.yzw = r3.xyz / r1.yyy;
      r3.z = 2;
      r2.yz = float2(0,0);
      while (true) {
        r2.w = cmp((int)r2.z >= 10);
        if (r2.w != 0) break;
        r3.xy = saturate(icb[r2.z+4].xy * invShadowSize_g.xy + r1.yz);
        r2.w = shadowMaps.SampleCmpLevelZero(SmplShadow_s, r3.xyz, r1.w).x;
        r2.y = r2.y + r2.w;
        r2.z = (int)r2.z + 1;
      }
      r1.y = shadowSplitDistance_g.z + -r1.x;
      r1.y = 0.200000003 * r1.y;
      r1.z = r2.y * 0.100000001 + -r2.x;
      r2.x = r1.y * r1.z + r2.x;
    }
  } else {
    r1.y = shadowSplitDistance_g.y + -5;
    r1.y = cmp(r1.y < r1.x);
    if (r1.y != 0) {
      r3.x = dot(r0.xyzw, shadowMtx_g[2]._m00_m10_m20_m30);
      r3.y = dot(r0.xyzw, shadowMtx_g[2]._m01_m11_m21_m31);
      r3.z = dot(r0.xyzw, shadowMtx_g[2]._m02_m12_m22_m32);
      r1.y = dot(r0.xyzw, shadowMtx_g[2]._m03_m13_m23_m33);
      r1.yzw = r3.xyz / r1.yyy;
      r2.yz = cmp(r1.yz < float2(0,0));
      r3.xy = cmp(float2(1,1) < r1.yz);
      r2.y = (int)r2.y | (int)r3.x;
      r2.y = (int)r2.z | (int)r2.y;
      r2.y = (int)r3.y | (int)r2.y;
      if (r2.y != 0) {
        r2.x = 1;
      } else {
        r3.z = 2;
        r2.yz = float2(0,0);
        while (true) {
          r2.w = cmp((int)r2.z >= 10);
          if (r2.w != 0) break;
          r3.xy = saturate(icb[r2.z+4].xy * invShadowSize_g.xy + r1.yz);
          r2.w = shadowMaps.SampleCmpLevelZero(SmplShadow_s, r3.xyz, r1.w).x;
          r2.y = r2.y + r2.w;
          r2.z = (int)r2.z + 1;
        }
        r2.x = 0.100000001 * r2.y;
      }
      r1.y = cmp(r1.x < shadowSplitDistance_g.y);
      if (r1.y != 0) {
        r3.x = dot(r0.xyzw, shadowMtx_g[1]._m00_m10_m20_m30);
        r3.y = dot(r0.xyzw, shadowMtx_g[1]._m01_m11_m21_m31);
        r3.z = dot(r0.xyzw, shadowMtx_g[1]._m02_m12_m22_m32);
        r1.y = dot(r0.xyzw, shadowMtx_g[1]._m03_m13_m23_m33);
        r1.yzw = r3.xyz / r1.yyy;
        r2.yz = invShadowSize_g.xy * float2(1.125,1.125);
        r3.z = 1;
        r2.w = 0;
        r3.w = 0;
        while (true) {
          r4.x = cmp((int)r3.w >= 10);
          if (r4.x != 0) break;
          r3.xy = saturate(icb[r3.w+4].xy * r2.yz + r1.yz);
          r3.x = shadowMaps.SampleCmpLevelZero(SmplShadow_s, r3.xyz, r1.w).x;
          r2.w = r3.x + r2.w;
          r3.w = (int)r3.w + 1;
        }
        r1.y = shadowSplitDistance_g.y + -r1.x;
        r1.y = 0.200000003 * r1.y;
        r1.z = r2.w * 0.100000001 + -r2.x;
        r2.x = r1.y * r1.z + r2.x;
      }
    } else {
      r1.y = cmp(r1.x < shadowSplitDistance_g.x);
      r2.yzw = r1.yyy ? float3(0,0,0) : float3(1.40129846e-45,5.60519386e-45,1);
      r3.x = dot(r0.xyzw, shadowMtx_g[r2.z/4]._m00_m10_m20_m30);
      r3.y = dot(r0.xyzw, shadowMtx_g[r2.z/4]._m01_m11_m21_m31);
      r3.z = dot(r0.xyzw, shadowMtx_g[r2.z/4]._m02_m12_m22_m32);
      r1.z = dot(r0.xyzw, shadowMtx_g[r2.z/4]._m03_m13_m23_m33);
      r3.xyz = r3.xyz / r1.zzz;
      r1.z = dot(float2(1.25,1.125), icb[r2.y+0].xy);
      r1.zw = invShadowSize_g.xy * r1.zz;
      r3.w = 0;
      r4.x = 0;
      while (true) {
        r4.y = cmp((int)r4.x >= 10);
        if (r4.y != 0) break;
        r2.yz = saturate(icb[r4.x+4].xy * r1.zw + r3.xy);
        r2.y = shadowMaps.SampleCmpLevelZero(SmplShadow_s, r2.yzw, r3.z).x;
        r3.w = r3.w + r2.y;
        r4.x = (int)r4.x + 1;
      }
      r2.x = 0.100000001 * r3.w;
      r1.z = shadowSplitDistance_g.x + -5;
      r1.z = cmp(r1.z < r1.x);
      r1.y = r1.z ? r1.y : 0;
      if (r1.y != 0) {
        r3.x = dot(r0.xyzw, shadowMtx_g[1]._m00_m10_m20_m30);
        r3.y = dot(r0.xyzw, shadowMtx_g[1]._m01_m11_m21_m31);
        r3.z = dot(r0.xyzw, shadowMtx_g[1]._m02_m12_m22_m32);
        r0.x = dot(r0.xyzw, shadowMtx_g[1]._m03_m13_m23_m33);
        r0.xyz = r3.xyz / r0.xxx;
        r1.yz = invShadowSize_g.xy * float2(1.125,1.125);
        r3.z = 1;
        r0.w = 0;
        r1.w = 0;
        while (true) {
          r2.y = cmp((int)r1.w >= 10);
          if (r2.y != 0) break;
          r3.xy = saturate(icb[r1.w+4].xy * r1.yz + r0.xy);
          r2.y = shadowMaps.SampleCmpLevelZero(SmplShadow_s, r3.xyz, r0.z).x;
          r0.w = r2.y + r0.w;
          r1.w = (int)r1.w + 1;
        }
        r0.y = shadowSplitDistance_g.x + -r1.x;
        r0.xy = float2(0.100000001,0.200000003) * r0.wy;
        r0.z = r3.w * 0.100000001 + -r0.x;
        r2.x = r0.y * r0.z + r0.x;
      }
    }
  }
  r0.x = -shadowFadeNear_g + r1.x;
  r0.x = saturate(shadowFadeRangeInv_g * r0.x);
  r0.y = 1 + -r2.x;
  o0.xyzw = r0.xxxx * r0.yyyy + r2.xxxx;
  return;
}