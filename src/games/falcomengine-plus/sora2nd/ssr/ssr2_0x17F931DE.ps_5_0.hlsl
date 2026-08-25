// ---- Created with 3Dmigoto v1.4.1 on Fri Aug 21 11:50:58 2026

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
  float fogExp_g : packoffset(c32);
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
  float2 cloudShadowOffset_g : packoffset(c74);
  float cloudShadowScale_g : packoffset(c74.z);
  float4x4 prevViewProj_g : packoffset(c75);
  float2 jitterDiff_g : packoffset(c79);
  float4 shadowBlurRadius_g : packoffset(c80);
}

cbuffer cb_ssr : register(b2)
{
  uint maxRayCount_g : packoffset(c0);
  float rayLength_g : packoffset(c0.y);
  float2 prevResolutionScaling_g : packoffset(c0.z);
  float2 texelSize_g : packoffset(c1);
  float2 uvClamp_g : packoffset(c1.z);
  float4x4 ssrPrevViewProj_g : packoffset(c2);
}

SamplerState samLinear_s : register(s0);
SamplerState samPoint_s : register(s1);
Texture2D<float4> ssrTexture : register(t0);
Texture2D<float4> historyTexture : register(t1);
Texture2D<float4> depthTexture : register(t2);


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_Position0,
  float4 v1 : TEXCOORD0,
  out float4 o0 : SV_Target0)
{
  float4 r0,r1,r2,r3,r4,r5;
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
  r1.x = dot(r0.xyzw, ssrPrevViewProj_g._m00_m10_m20_m30);
  r1.y = dot(r0.xyzw, ssrPrevViewProj_g._m01_m11_m21_m31);
  r0.x = dot(r0.xyzw, ssrPrevViewProj_g._m03_m13_m23_m33);
  r0.xy = r1.xy / r0.xx;
  r0.xy = r0.xy * float2(0.5,0.5) + float2(0.5,0.5);
  r0.z = 1 + -r0.y;
  r0.xy = resolutionScaling_g.xy * r0.xz;
  r0.xy = prevResolutionScaling_g.xy * r0.xy;
  r0.xyzw = historyTexture.SampleLevel(samLinear_s, r0.xy, 0).xyzw;
  r1.xy = saturate(-texelSize_g.xy * float2(0.5,0.5) + v1.xy);
  r1.xyzw = ssrTexture.SampleLevel(samLinear_s, r1.xy, 0).xyzw;
  r2.xy = saturate(texelSize_g.xy * float2(0.5,0.5) + v1.xy);
  r2.xyzw = ssrTexture.SampleLevel(samLinear_s, r2.xy, 0).xyzw;
  r3.xyzw = r2.xyzw + r1.xyzw;
  r4.xyzw = ssrTexture.SampleLevel(samLinear_s, v1.xy, 0).xyzw;
  r5.xyzw = r4.xyzw + r4.xyzw;
  r3.xyzw = r3.xyzw * float4(4,4,4,4) + -r5.xyzw;
  r5.xyzw = -r3.xyzw * float4(0.166666999,0.166666999,0.166666999,0.166666999) + r4.xyzw;
  r4.xyzw = r5.xyzw * float4(1.08731282,1.08731282,1.08731282,1.08731282) + r4.xyzw;
  r4.xyzw = max(float4(0,0,0,0), r4.xyzw);
  r4.xyzw = min(float4(65472,65472,65472,65472), r4.xyzw);
  r5.x = dot(r4.xyz, float3(0.298999995,0.587000012,0.114));
  r5.x = r5.x * r4.w;
  r3.xyzw = r4.xyzw + r3.xyzw;
  r3.xyzw = float4(0.142857,0.142857,0.142857,0.142857) * r3.xyzw;
  r3.x = dot(r3.xyz, float3(0.298999995,0.587000012,0.114));
  r3.x = r3.x * r3.w + -r5.x;
  r5.xyzw = min(r2.xyzw, r1.xyzw);
  r1.xyzw = max(r2.xyzw, r1.xyzw);
  r1.xyzw = abs(r3.xxxx) * float4(4,4,4,4) + r1.xyzw;
  r2.xyzw = -abs(r3.xxxx) * float4(4,4,4,4) + r5.xyzw;
  r3.xyzw = r2.xyzw + r1.xyzw;
  r1.xyzw = -r2.xyzw + r1.xyzw;
  r1.xyzw = float4(0.5,0.5,0.5,0.5) * r1.xyzw;
  r0.xyzw = -r3.xyzw * float4(0.5,0.5,0.5,0.5) + r0.xyzw;
  r2.xyzw = float4(0.5,0.5,0.5,0.5) * r3.xyzw;
  r3.xyzw = float4(9.99999975e-05,9.99999975e-05,9.99999975e-05,9.99999975e-05) + r0.xyzw;
  r1.xyzw = r1.xyzw / r3.xyzw;
  r1.y = min(abs(r1.y), abs(r1.z));
  r1.x = min(abs(r1.x), r1.y);
  r1.x = min(r1.x, abs(r1.w));
  r1.x = min(1, r1.x);
  r0.xyzw = r0.xyzw * r1.xxxx + r2.xyzw;
  r0.xyzw = r0.xyzw + -r4.xyzw;
  r0.xyzw = r0.xyzw * float4(0.899999976,0.899999976,0.899999976,0.899999976) + r4.xyzw;
  r0.xyzw = max(float4(0,0,0,0), r0.xyzw);
  o0.xyzw = min(float4(65472,65472,65472,65472), r0.xyzw);
  return;
}