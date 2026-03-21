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

cbuffer cb_local : register(b2)
{
  int2 textureSize_g : packoffset(c0);
  float2 aoSize_g : packoffset(c0.z);
  float charaAOStrength_g : packoffset(c1);
  float charaAOCutOff_g : packoffset(c1.y);
  int mapAOSampleCount_g : packoffset(c1.z);
  float mapAOIntensity_g : packoffset(c1.w);
  float3 mapAOColor_g : packoffset(c2);
  float mapAOFadeBeginDistance_g : packoffset(c2.w);
  float mapAOFadeRangeInv_g : packoffset(c3);
  float mapAOLimit_g : packoffset(c3.y);
  float mapAOBias_g : packoffset(c3.z);
  float mapAODistMaxInv_g : packoffset(c3.w);
  float2 texelSize_g : packoffset(c4);
  float2 prevAoScaling_g : packoffset(c4.z);
  float4 sphere_g[16] : packoffset(c5);
}

cbuffer cb_local2 : register(b3)
{
  float3 rayMarchShadowDir_g : packoffset(c0);
  float4x4 ssaoPrevViewProj_g : packoffset(c1);
}

SamplerState samPoint_s : register(s0);
SamplerState samLinear_s : register(s1);
Texture2D<float4> depthTexture : register(t0);
Texture2D<uint4> mrtTexture0 : register(t1);
Texture2D<float4> intensityMap : register(t2);
Texture2D<float4> prevTexture : register(t3);


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
                              { 0, 0, 0, 1.000000} };
  float4 r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.z = depthTexture.SampleLevel(samLinear_s, v1.xy, 0).x;
  mrtTexture0.GetDimensions(0, fDest.x, fDest.y, fDest.z);
  r1.xy = fDest.xy;
  r1.zw = float2(0.25,0.25) / r1.xy;
  r1.zw = v1.xy + r1.zw;
  r1.xy = r1.zw * r1.xy;
  r1.xy = (int2)r1.xy;
  r1.zw = float2(0,0);
  r1.xyzw = mrtTexture0.Load(r1.xyz).xyzw;
  r0.xy = v1.zw * float2(2,-2) + float2(-1,1);
  r0.w = 1;
  r2.x = dot(r0.xyzw, projInv_g._m00_m10_m20_m30);
  r2.y = dot(r0.xyzw, projInv_g._m01_m11_m21_m31);
  r2.z = dot(r0.xyzw, projInv_g._m02_m12_m22_m32);
  r2.w = dot(r0.xyzw, projInv_g._m03_m13_m23_m33);
  r2.xyz = r2.xyz / r2.www;
  r3.x = (int)r1.w & 1;
  if (r3.x != 0) {
    r1.xy = (uint2)r1.xy;
    r4.zw = r1.xy * float2(3.05180438e-05,3.05180438e-05) + float2(-1,-1);
    r1.x = 3.14159274 * r4.z;
    sincos(r1.x, r1.x, r5.x);
    r1.y = -r4.w * r4.w + 1;
    r1.y = sqrt(r1.y);
    r4.x = r5.x * r1.y;
    r4.y = r1.x * r1.y;
    r1.x = dot(v1.zw, float2(12.9898005,78.2330017));
    r1.x = sin(r1.x);
    r1.x = 43758.5469 * r1.x;
    r1.x = frac(r1.x);
    r3.yzw = float3(5.39830017,5.44269991,6.93709993) * r1.xxx;
    r3.yzw = frac(r3.yzw);
    r5.xyz = float3(21.5351009,14.3136997,15.3219004) + r3.yzw;
    r1.x = dot(r3.zwy, r5.xyz);
    r3.yzw = r3.yzw + r1.xxx;
    r3.yzw = r3.yyz * r3.wzw;
    r3.yzw = float3(95.4337006,97.5970001,93.8365021) * r3.yzw;
    r3.yzw = frac(r3.yzw);
    r3.yzw = r3.yzw * float3(2,2,2) + float3(-1,-1,-1);
    r1.x = dot(r3.yzw, r3.yzw);
    r1.x = rsqrt(r1.x);
    r3.yzw = r3.yzw * r1.xxx;
    r1.x = charaAOStrength_g + -charaAOCutOff_g;
    r1.x = 1 / r1.x;
    r5.xw = float2(0,1);
    r1.y = 0;
    r4.z = 0;
    while (true) {
      r6.x = cmp((int)r4.z >= 10);
      if (r6.x != 0) break;
      r6.x = dot(sphere_g[r4.z].xyz, r3.yzw);
      r6.x = r6.x + r6.x;
      r6.xyz = r3.yzw * -r6.xxx + sphere_g[r4.z].xyz;
      r6.z = dot(r6.xyz, r4.xyw);
      r6.w = cmp(0 < r6.z);
      r6.z = cmp(r6.z < 0);
      r6.z = (int)-r6.w + (int)r6.z;
      r6.z = (int)r6.z;
      r6.xy = r6.zz * r6.xy;
      r6.xy = aoSize_g.xy * r6.xy;
      r6.xy = r6.xy * float2(1,-1) + v1.zw;
      r6.xy = resolutionScaling_g.xy * r6.xy;
      r5.z = depthTexture.SampleLevel(samPoint_s, r6.xy, 0).x;
      r6.x = dot(projInv_g._m22_m32, r5.zw);
      r5.z = dot(projInv_g._m23_m33, r5.zw);
      r5.z = r6.x / r5.z;
      r5.z = r5.z + -r2.z;
      r6.x = cmp(r5.z >= charaAOCutOff_g);
      r6.x = r6.x ? 1.000000 : 0;
      r5.z = -charaAOCutOff_g + r5.z;
      r5.z = saturate(r5.z * r1.x);
      r6.y = r5.z * -2 + 3;
      r5.z = r5.z * r5.z;
      r5.z = -r6.y * r5.z + 1;
      r1.y = r6.x * r5.z + r1.y;
      r4.z = (int)r4.z + 1;
    }
    r5.y = 0.100000001 * r1.y;
    r2.w = 1;
    r6.x = dot(r2.xyzw, viewInv_g._m00_m10_m20_m30);
    r6.y = dot(r2.xyzw, viewInv_g._m01_m11_m21_m31);
    r6.z = dot(r2.xyzw, viewInv_g._m02_m12_m22_m32);
    r7.x = viewInv_g._m20;
    r7.y = viewInv_g._m21;
    r7.z = viewInv_g._m22;
    r1.x = dot(r4.xyw, r7.xyz);
    r1.xy = abs(r1.xx) * float2(-0.00249999994,0.00150000001) + float2(0.00749999983,0.000500000024);
    r3.yzw = r4.xyw * r1.xxx + r6.xyz;
    r4.xyz = rayMarchShadowDir_g.xyz * r1.yyy;
    r6.w = 1;
    r1.xy = float2(0,0);
    while (true) {
      r2.w = cmp((int)r1.y >= 10);
      if (r2.w != 0) break;
      r2.w = (int)r1.y;
      r6.xyz = r4.xyz * r2.www + r3.yzw;
      r7.x = dot(r6.xyzw, viewProj_g._m00_m10_m20_m30);
      r7.y = dot(r6.xyzw, viewProj_g._m01_m11_m21_m31);
      r7.z = dot(r6.xyzw, viewProj_g._m02_m12_m22_m32);
      r2.w = dot(r6.xyzw, viewProj_g._m03_m13_m23_m33);
      r6.xyz = r7.xyz / r2.www;
      r7.xy = r6.xy * float2(0.5,0.5) + float2(0.5,0.5);
      r7.z = 1 + -r7.y;
      r5.zw = resolutionScaling_g.xy * r7.xz;
      r2.w = depthTexture.SampleLevel(samPoint_s, r5.zw, 0).x;
      r2.w = r2.w + -r6.z;
      r2.w = saturate(400000 * r2.w);
      r1.x = r2.w * 0.25 + r1.x;
      r1.y = (int)r1.y + 1;
    }
    r1.x = 1 + -r1.x;
    r1.x = max(0, r1.x);
    r3.yz = float2(0.5,0.5) + -v1.zw;
    r1.y = max(abs(r3.y), abs(r3.z));
    r1.y = -0.449999988 + r1.y;
    r1.y = max(0, r1.y);
    r1.y = 20 * r1.y;
    r2.w = 1 + -r1.x;
    r4.z = r1.y * r2.w + r1.x;
  } else {
    if (mapAOSampleCount_g != 0) {
      r1.w = (uint)r1.z >> 8;
      r1.xy = (int2)r1.zw & int2(255,255);
      r1.xy = (uint2)r1.xy;
      r1.zw = r1.xy * float2(0.00784313772,0.00784313772) + float2(-1,-1);
      r1.z = 3.14159274 * r1.z;
      sincos(r1.z, r6.x, r7.x);
      r1.z = -r1.w * r1.w + 1;
      r1.z = sqrt(r1.z);
      r1.x = r7.x * r1.z;
      r1.y = r6.x * r1.z;
      r3.yz = (int2)textureSize_g.xy;
      r3.yz = v1.zw * r3.yz;
      r3.yz = float2(0.25,0.25) * r3.yz;
      r5.zw = cmp(r3.yz >= -r3.yz);
      r3.yz = frac(abs(r3.yz));
      r3.yz = r5.zw ? r3.yz : -r3.yz;
      r3.yz = float2(4,4) * r3.yz;
      r3.yz = (int2)r3.yz;
      r6.x = dot(ditherMtx_g._m00_m10_m20_m30, icb[r3.y+0].xyzw);
      r6.y = dot(ditherMtx_g._m01_m11_m21_m31, icb[r3.y+0].xyzw);
      r6.z = dot(ditherMtx_g._m02_m12_m22_m32, icb[r3.y+0].xyzw);
      r6.w = dot(ditherMtx_g._m03_m13_m23_m33, icb[r3.y+0].xyzw);
      r1.z = dot(r6.xyzw, icb[r3.z+0].xyzw);
      r2.w = -10 + -r2.z;
      r3.yz = saturate(float2(0.00999999978,0.00249999994) * r2.ww);
      r2.w = r3.y * 0.197500005 + 0.00249999994;
      r3.y = saturate(-0.100000001 * r2.z);
      r3.y = r3.y * 0.189999998 + 0.00999999978;
      r3.w = 2 + -r3.y;
      r3.y = r3.z * r3.w + r3.y;
      r3.z = mapAOSampleCount_g;
      r6.y = v1.z;
      r6.z = -v1.z;
      r7.w = 1;
      r8.w = 1;
      r5.zw = float2(1,0);
      r3.w = 0;
      r4.w = r1.z;
      while (true) {
        r6.w = cmp((int)r5.w >= mapAOSampleCount_g);
        if (r6.w != 0) break;
        r6.x = v1.w + r4.w;
        r6.w = dot(float2(78.2330017,12.9898005), r6.xy);
        r6.w = sin(r6.w);
        r6.w = 43758.5469 * r6.w;
        r6.w = frac(r6.w);
        r9.z = r6.w * 2 + -1;
        r6.x = dot(float2(78.2330017,12.9898005), r6.xz);
        r6.x = sin(r6.x);
        r6.x = 43758.5469 * r6.x;
        r6.x = frac(r6.x);
        r6.x = 6.28318548 * r6.x;
        sincos(r6.x, r6.x, r10.x);
        r6.w = -r9.z * r9.z + 1;
        r6.w = sqrt(r6.w);
        r10.y = r6.x;
        r9.xy = r10.xy * r6.ww;
        r6.x = 1 + r4.w;
        r6.w = r6.x / r3.z;
        r6.w = sqrt(r6.w);
        r6.w = r6.w * r3.y;
        r9.xyz = r9.xyz * r6.www;
        r6.w = dot(-r1.xyw, r9.xyz);
        r6.w = cmp(r6.w < 0);
        r9.xyz = r6.www ? r9.xyz : -r9.xyz;
        r7.xyz = r9.xyz + r2.xyz;
        r9.x = dot(r7.xyzw, proj_g._m00_m10_m20_m30);
        r9.y = dot(r7.xyzw, proj_g._m01_m11_m21_m31);
        r6.w = dot(r7.xyzw, proj_g._m03_m13_m23_m33);
        r7.xy = r9.xy / r6.ww;
        r7.xy = r7.xy * float2(0.5,0.5) + float2(0.5,0.5);
        r6.w = 1 + -r7.y;
        r9.x = cmp(r7.x < 0);
        r9.y = cmp(r6.w < 0);
        r9.x = (int)r9.y | (int)r9.x;
        r9.y = cmp(1 < r7.x);
        r9.x = (int)r9.y | (int)r9.x;
        r6.w = cmp(1 < r6.w);
        r6.w = (int)r6.w | (int)r9.x;
        if (r6.w != 0) {
          r9.y = (int)r5.w + 1;
          r9.x = r5.z;
          r4.w = r6.x;
          r5.zw = r9.xy;
          continue;
        }
        r7.z = 1 + -r7.y;
        r9.xy = resolutionScaling_g.xy * r7.xz;
        r6.w = intensityMap.SampleLevel(samLinear_s, r9.xy, 0).x;
        r5.z = min(r6.w, r5.z);
        r8.z = depthTexture.SampleLevel(samLinear_s, r9.xy, 0).x;
        r8.xy = r7.xz * float2(2,-2) + float2(-1,1);
        r7.x = dot(r8.xyzw, projInv_g._m00_m10_m20_m30);
        r7.y = dot(r8.xyzw, projInv_g._m01_m11_m21_m31);
        r7.z = dot(r8.xyzw, projInv_g._m02_m12_m22_m32);
        r6.w = dot(r8.xyzw, projInv_g._m03_m13_m23_m33);
        r7.xyz = r7.xyz / r6.www;
        r7.xyz = r7.xyz + -r2.xyz;
        r6.w = dot(r7.xyz, r1.xyw);
        r6.w = r6.w + -r2.w;
        r6.w = max(0, r6.w);
        r7.x = dot(r7.xyz, r7.xyz);
        r7.y = cmp(r7.x == 0.000000);
        r7.y = r7.y ? 1.000000 : 0;
        r7.x = r7.x + r7.y;
        r6.w = r6.w / r7.x;
        r3.w = r6.w + r3.w;
        r5.w = (int)r5.w + 1;
        r4.w = r6.x;
      }
      r1.xy = resolutionScaling_g.xy * v1.zw;
      r1.x = intensityMap.SampleLevel(samLinear_s, r1.xy, 0).x;
      r1.x = min(r5.z, r1.x);
      r1.x = r3.y * r1.x;
      r1.x = r3.w * r1.x;
      r1.x = mapAOIntensity_g * r1.x;
      r1.x = r1.x / r3.z;
      r5.x = min(mapAOLimit_g, r1.x);
    } else {
      r5.x = 0;
    }
    r5.y = 0;
    r4.z = 1;
  }
  r4.xy = float2(1,1) + -r5.xy;
  r1.x = dot(r0.xyzw, viewProjInv_g._m00_m10_m20_m30);
  r1.y = dot(r0.xyzw, viewProjInv_g._m01_m11_m21_m31);
  r1.z = dot(r0.xyzw, viewProjInv_g._m02_m12_m22_m32);
  r1.w = dot(r0.xyzw, viewProjInv_g._m03_m13_m23_m33);
  r0.xyzw = r1.xyzw / r1.wwww;
  r1.x = dot(r0.xyzw, ssaoPrevViewProj_g._m00_m10_m20_m30);
  r1.y = dot(r0.xyzw, ssaoPrevViewProj_g._m01_m11_m21_m31);
  r0.x = dot(r0.xyzw, ssaoPrevViewProj_g._m03_m13_m23_m33);
  r0.xy = r1.xy / r0.xx;
  r0.xy = r0.xy * float2(0.5,0.5) + float2(0.5,0.5);
  r0.z = 1 + -r0.y;
  r0.xy = resolutionScaling_g.xy * r0.xz;
  r0.xy = prevAoScaling_g.xy * r0.xy;
  r0.xyz = prevTexture.SampleLevel(samLinear_s, r0.xy, 0).xyz;
  r1.xyz = r4.xyz + -r0.xyz;
  o0.xyz = r1.xyz * float3(0.25,0.25,1) + r0.xyz;
  o0.w = r3.x ? 1 : 0;
  return;
}