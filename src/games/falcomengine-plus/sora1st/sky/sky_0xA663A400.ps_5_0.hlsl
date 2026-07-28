// ---- Created with 3Dmigoto v1.4.1 on Mon Jul 27 15:27:10 2026

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

cbuffer cb_post_sky : register(b6)
{
  float3 incomingLight_g : packoffset(c0);
  float3 scatteringR_g : packoffset(c1);
  float3 scatteringM_g : packoffset(c2);
  float3 extinctionR_g : packoffset(c3);
  float3 extinctionM_g : packoffset(c4);
  float3 densityScaleHeight_g : packoffset(c5);
  float skyHorizonBottomLimit_g : packoffset(c5.w);
  float3 sunDirection_g : packoffset(c6);
  float skyHorizonTopLimit_g : packoffset(c6.w);
  float mieG_g : packoffset(c7);
  float distanceScale_g : packoffset(c7.y);
  float planetRadius_g : packoffset(c7.z);
  float atmosphereHeight_g : packoffset(c7.w);
  float sunIntensity_g : packoffset(c8);
  float skyLutNearOverFarClip_g : packoffset(c8.y);
  float skyLutCameraFarClip_g : packoffset(c8.z);
  float skyBrightness_g : packoffset(c8.w);
}

cbuffer cb_post_sky2 : register(b3)
{
  float4x4 skyViewProjInv_g : packoffset(c0);
}

SamplerState samLinear_s : register(s0);
Texture3D<float4> skyLUT : register(t0);
Texture3D<float2> skyLUT2 : register(t1);


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_Position0,
  float4 v1 : TEXCOORD0,
  out float4 o0 : SV_Target0,
  out float2 o4 : SV_TARGET4)
{
  float4 r0,r1,r2,r3;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.x = -mieG_g * mieG_g + 1;
  r0.x = 3 * r0.x;
  r0.yz = mieG_g * mieG_g + float2(2,1);
  r0.y = r0.y + r0.y;
  r0.x = r0.x / r0.y;
  r1.xy = v1.zw * float2(2,-2) + float2(-1,1);
  r1.zw = float2(1,1);
  r2.x = dot(r1.xyz, skyViewProjInv_g._m00_m10_m30);
  r2.y = dot(r1.xyz, skyViewProjInv_g._m01_m11_m31);
  r2.z = dot(r1.xyz, skyViewProjInv_g._m02_m12_m32);
  r0.y = dot(r1.xyz, skyViewProjInv_g._m03_m13_m33);
  r1.xyz = r2.xyz / r0.yyy;
  r0.y = dot(r1.xyz, r1.xyz);
  r0.y = rsqrt(r0.y);
  r2.xyz = r1.xyz * r0.yyy;
  r0.y = dot(r2.xyz, -sunDirection_g.xyz);
  r0.w = max(0.0500000007, abs(r2.y));
  r2.y = r0.w * 0.5 + 0.5;
  r0.w = dot(r0.yy, mieG_g);
  r0.z = r0.z + -r0.w;
  r0.z = log2(r0.z);
  r0.xz = float2(0.0795774683,1.5) * r0.xz;
  r0.z = exp2(r0.z);
  r0.w = r0.y * r0.y + 1;
  r0.z = r0.w / r0.z;
  r0.x = r0.x * r0.z;
  r2.z = viewInv_g._m31 / atmosphereHeight_g;
  r2.x = sunDirection_g.y * -0.5 + 0.5;
  r3.yz = skyLUT2.SampleLevel(samLinear_s, r2.xyz, 0).xy;
  r2.xyzw = skyLUT.SampleLevel(samLinear_s, r2.xyz, 0).xyzw;
  r3.x = r2.w;
  r0.xzw = r3.xyz * r0.xxx;
  r0.xzw = scatteringM_g.xyz * r0.xzw;
  r2.w = r0.y * 0.5 + 1.39999998;
  r0.y = -r0.y * 1.96000004 + 1.9604001;
  r0.y = log2(r0.y);
  r0.y = 1.5 * r0.y;
  r0.y = exp2(r0.y);
  r0.y = 12.566371 * r0.y;
  r0.y = 0.000399999233 / r0.y;
  r0.y = 0.00300000003 * r0.y;
  r3.xyz = r3.xyz * r0.yyy;
  r3.xyz = sunIntensity_g * r3.xyz;
  r0.y = 0.0636619776 * r2.w;
  r2.xyz = r2.xyz * r0.yyy;
  r0.xyz = r2.xyz * scatteringR_g.xyz + r0.xzw;
  r0.xyz = r0.xyz * incomingLight_g.xyz + r3.xyz;
  r0.xyz = skyBrightness_g * r0.xyz;
  o0.xyz = max(float3(0,0,0), r0.xyz);
  o0.w = 1;
  r0.x = dot(r1.xyzw, viewProj_g._m00_m10_m20_m30);
  r0.y = dot(r1.xyzw, viewProj_g._m01_m11_m21_m31);
  r0.z = dot(r1.xyzw, viewProj_g._m03_m13_m23_m33);
  r0.xy = r0.xy / r0.zz;
  r0.xy = r0.xy * float2(0.5,-0.5) + float2(0.5,0.5);
  r0.xy = vpSize_g.xy * r0.xy;
  r2.x = dot(r1.xyzw, prevViewProj_g._m00_m10_m20_m30);
  r2.y = dot(r1.xyzw, prevViewProj_g._m01_m11_m21_m31);
  r0.z = dot(r1.xyzw, prevViewProj_g._m03_m13_m23_m33);
  r0.zw = r2.xy / r0.zz;
  r0.zw = r0.zw * float2(0.5,-0.5) + float2(0.5,0.5);
  r0.xy = r0.zw * vpSize_g.xy + -r0.xy;
  o4.xy = jitterDiff_g.xy + r0.xy;
  return;
}