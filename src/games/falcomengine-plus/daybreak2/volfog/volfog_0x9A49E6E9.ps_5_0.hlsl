// ---- Created with 3Dmigoto v1.4.1 on Mon Jul  6 22:15:04 2026

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
  float ldotvXZ_g : packoffset(c25.w);
  float3 lightDirection_g : packoffset(c26);
  float shadowSplitDistance_g : packoffset(c26.w);
  float4x4 shadowMtx_g[2] : packoffset(c27);
  float2 invShadowSize_g : packoffset(c35);
  float shadowFadeNear_g : packoffset(c35.z);
  float shadowFadeRangeInv_g : packoffset(c35.w);
  float3 sceneShadowColor_g : packoffset(c36);
  float gameTime_g : packoffset(c36.w);
  float3 windDirection_g : packoffset(c37);
  uint collisionCount_g : packoffset(c37.w);
  float lightTileWidthInv_g : packoffset(c38);
  float lightTileHeightInv_g : packoffset(c38.y);
  float fogNearDistance_g : packoffset(c38.z);
  float fogFadeRangeInv_g : packoffset(c38.w);
  float3 fogColor_g : packoffset(c39);
  float fogIntensity_g : packoffset(c39.w);
  float fogHeight_g : packoffset(c40);
  float fogHeightRangeInv_g : packoffset(c40.y);
  float windWaveTime_g : packoffset(c40.z);
  float windWaveFrequency_g : packoffset(c40.w);
  uint localLightProbeCount_g : packoffset(c41);
  float lightSpecularGlossiness_g : packoffset(c41.y);
  float lightSpecularIntensity_g : packoffset(c41.z);
  float lightTileDepthInv_g : packoffset(c41.w);
  float4x4 ditherMtx_g : packoffset(c42);
  float4 lightProbe_g[9] : packoffset(c46);
  float4x4 farShadowMtx_g : packoffset(c55);
  float3 chrLightDir_g : packoffset(c59);
  float shadowDistance_g : packoffset(c59.w);
  float resolutionScaling_g : packoffset(c60);
  float sceneTime_g : packoffset(c60.y);
  float windForce_g : packoffset(c60.z);
  float disableMapObjNearFade_g : packoffset(c60.w);
  float4 mapColor_g : packoffset(c61);
  float4 clipPlane_g : packoffset(c62);
  float shadowZeroCascadeUVMult_g : packoffset(c63);
  float prevSceneTime_g : packoffset(c63.y);
  float prevWindWaveTime_g : packoffset(c63.z);
  uint enableMotionVectors_g : packoffset(c63.w);
  float4x4 prevView_g : packoffset(c64);
  float4x4 prevViewInv_g : packoffset(c68);
  float4x4 prevProj_g : packoffset(c72);
  float4x4 prevProjInv_g : packoffset(c76);
  float4x4 prevViewProj_g : packoffset(c80);
  float4x4 prevViewProjInv_g : packoffset(c84);
  float2 motionJitterOffset_g : packoffset(c88);
  float2 curJitterOffset_g : packoffset(c88.z);
}

cbuffer ConstantBuffer : register(b6)
{
  float3 incomingLight_g : packoffset(c0);
  float3 scatteringR_g : packoffset(c1);
  float3 scatteringM_g : packoffset(c2);
  float3 extinctionR_g : packoffset(c3);
  float3 extinctionM_g : packoffset(c4);
  float3 densityScaleHeight_g : packoffset(c5);
  float3 sunDirection_g : packoffset(c6);
  float mieG_g : packoffset(c7);
  float distanceScale_g : packoffset(c7.y);
  float planetRadius_g : packoffset(c7.z);
  float atmosphereHeight_g : packoffset(c7.w);
  float sunIntensity_g : packoffset(c8);
  float volumeNearOverFarClip_g : packoffset(c8.y);
  float volumeCameraFarClip_g : packoffset(c8.z);
  float cloudCoverage_g : packoffset(c9);
  float cloudThickness_g : packoffset(c9.y);
  uint cloudRaySteps_g : packoffset(c9.z);
  float cloudLightIntensity_g : packoffset(c9.w);
  float cloudDistance_g : packoffset(c10);
  float cloudFadeRangeInv_g : packoffset(c10.y);
  float cloudHeight_g : packoffset(c10.z);
  float3 cloudColor_g : packoffset(c11);
}

SamplerState samPoint_s : register(s0);
SamplerState samLinear_s : register(s1);
Texture2D<float4> colorTexture : register(t0);
Texture2D<float4> depthTexture : register(t1);
Texture2D<float4> cloudsTexture : register(t2);
Texture3D<float4> inscatterLUT : register(t3);
Texture3D<float4> extinctionLUT : register(t4);


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_Position0,
  float4 v1 : TEXCOORD0,
  out float4 o0 : SV_Target0,
  out float o1 : SV_Target1)
{
  float4 r0,r1,r2;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.xyzw = colorTexture.SampleLevel(samPoint_s, v1.xy, 0).xyzw;
  r1.x = depthTexture.SampleLevel(samPoint_s, v1.xy, 0).x;
  r1.z = cmp(0 < r1.x);
  if (r1.z != 0) {
    r1.y = 1;
    r1.z = dot(projInv_g._m22_m32, r1.xy);
    r1.x = dot(projInv_g._m23_m33, r1.xy);
    r1.x = r1.z / r1.x;
    r1.x = -r1.x / volumeCameraFarClip_g;
    r1.x = -volumeNearOverFarClip_g + r1.x;
    r1.y = -volumeNearOverFarClip_g + 1;
    r1.z = r1.x / r1.y;
    r1.xy = v1.zw;
    r2.xyz = inscatterLUT.SampleLevel(samLinear_s, r1.xyz, 0).xyz;
    r1.xyz = extinctionLUT.SampleLevel(samLinear_s, r1.xyz, 0).xyz;
    r1.xyz = r1.xyz + r2.xyz;
    r0.xyz = r1.xyz * r0.xyz;
    o1.x = 0;
  } else {
    r1.xyzw = cloudsTexture.SampleLevel(samLinear_s, v1.xy, 0).xyzw;
    r1.xyz = r1.xyz + -r0.xyz;
    r0.xyz = r1.www * r1.xyz + r0.xyz;
    r1.xw = invVPSize_g.xy;
    r1.yz = float2(0,0);
    r2.xyzw = v1.xyxy + -r1.xyzw;
    r2.x = depthTexture.SampleLevel(samPoint_s, r2.xy, 0).x;
    r1.xyzw = v1.xyxy + r1.xyzw;
    r1.x = depthTexture.SampleLevel(samPoint_s, r1.xy, 0).x;
    r1.y = depthTexture.SampleLevel(samPoint_s, r2.zw, 0).x;
    r1.z = depthTexture.SampleLevel(samPoint_s, r1.zw, 0).x;
    r1.w = cmp(r2.x == 0.000000);
    r1.xy = cmp(r1.xy == float2(0,0));
    r1.x = r1.x ? r1.w : 0;
    r1.x = r1.y ? r1.x : 0;
    r1.y = cmp(r1.z == 0.000000);
    r1.x = r1.y ? r1.x : 0;
    o1.x = r1.x ? 1.000000 : 0;
  }
  o0.xyzw = r0.xyzw;
  return;
}