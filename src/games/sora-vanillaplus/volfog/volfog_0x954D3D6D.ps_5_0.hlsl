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

cbuffer cb_volume_fog : register(b7)
{
  float volumeLightBrightness_g : packoffset(c0);
  float volumeDensity_g : packoffset(c0.y);
  float volumeCameraFarOverMaxFar_g : packoffset(c0.z);
  float volumeCameraFarClip_g : packoffset(c0.w);
  float volumeNearOverFarClip_g : packoffset(c1);
  float volumeNearDistance_g : packoffset(c1.y);
  float volumeFarDistance_g : packoffset(c1.z);
  uint volumeShapeCount_g : packoffset(c1.w);
  float4 volumeColor_g : packoffset(c2);
  float2 voulumeLightTileSizeInv_g : packoffset(c3);
  float combineAlpha_g : packoffset(c3.z);
  float temporalRatio_g : packoffset(c3.w);
  float2 prevScaling_g : packoffset(c4);
  float2 prevUVClamp_g : packoffset(c4.z);
  float volumeNearFadeInv_g : packoffset(c5);
  float volumeNearFadeValue_g : packoffset(c5.y);
  float densityScale_g : packoffset(c5.w);
}

SamplerState samLinear_s : register(s0);
SamplerState samPoint_s : register(s1);
Texture2D<float4> colorTexture : register(t0);
Texture2D<float4> depthTexture : register(t1);
Texture3D<float4> volumeScatter : register(t2);


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_Position0,
  float4 v1 : TEXCOORD0,
  out float4 o0 : SV_Target0)
{
  float4 r0,r1;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.x = depthTexture.SampleLevel(samPoint_s, v1.xy, 0).x;
  r0.y = 1;
  r0.z = dot(projInv_g._m22_m32, r0.xy);
  r0.x = dot(projInv_g._m23_m33, r0.xy);
  r0.x = r0.z / r0.x;
  r0.x = -r0.x / volumeCameraFarClip_g;
  r0.x = r0.x * volumeCameraFarOverMaxFar_g + -volumeNearOverFarClip_g;
  r0.y = -volumeNearOverFarClip_g + 1;
  r0.z = r0.x / r0.y;
  r0.xy = v1.xy;
  r0.xyzw = volumeScatter.SampleLevel(samLinear_s, r0.xyz, 0).xyzw;
  r1.xyzw = colorTexture.SampleLevel(samPoint_s, v1.xy, 0).xyzw;
  r0.xyz = r1.xyz * r0.www + r0.xyz;
  r0.xyz = r0.xyz + -r1.xyz;
  o0.xyz = combineAlpha_g * r0.xyz + r1.xyz;
  o0.w = r1.w;
  return;
}