// ---- Created with 3Dmigoto v1.4.1 on Fri Mar  6 12:44:42 2026

#include "./kai-vanillaplus.h"

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
  float gameTime_g : packoffset(c26.w);
  float3 sceneShadowColor_g : packoffset(c27);
  float chrLightIntensity_g : packoffset(c27.w);
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
  float disableMapObjNearFade_g : packoffset(c32.w);
  float4x4 ditherMtx_g : packoffset(c33);
  float4 lightProbe_g[9] : packoffset(c37);
  float3 chrLightDir_g : packoffset(c46);
  float windForce_g : packoffset(c46.w);
  float4 mapColor_g : packoffset(c47);
  float4 clipPlane_g : packoffset(c48);
  float2 resolutionScaling_g : packoffset(c49);
  float2 shadowSplitDistance_g : packoffset(c49.z);
  float4x4 shadowMtx_g[3] : packoffset(c50);
  float shadowFadeNear_g : packoffset(c62);
  float shadowFadeRangeInv_g : packoffset(c62.y);
  float2 invShadowSize_g : packoffset(c62.z);
  float4 frustumPlanes_g[6] : packoffset(c63);
  float4x4 prevView_g : packoffset(c69);
  float4x4 prevViewInv_g : packoffset(c73);
  float4x4 prevProj_g : packoffset(c77);
  float4x4 prevProjInv_g : packoffset(c81);
  float4x4 prevViewProj_g : packoffset(c85);
  float4x4 prevViewProjInv_g : packoffset(c89);
  float2 motionJitterOffset_g : packoffset(c93);
  float2 curJitterOffset_g : packoffset(c93.z);
  float prevSceneTime_g : packoffset(c94);
  uint enableMotionVectors_g : packoffset(c94.y);
  float prevWindWaveTime_g : packoffset(c94.z);
  float padding : packoffset(c94.w);
}

cbuffer cb_dof : register(b2)
{
  float2 uv_clamp : packoffset(c0);
  float cocMaxRadius : packoffset(c0.z);
  float nearZ : packoffset(c0.w);
  float farZ : packoffset(c1);
  float invNearFade : packoffset(c1.y);
  float invFarFade : packoffset(c1.z);
  float nearFadeExp : packoffset(c1.w);
  float farFadeExp : packoffset(c2);
}

SamplerState samPoint_s : register(s0);
SamplerState samLinear_s : register(s1);
Texture2D<float4> colorTexture : register(t0);
Texture2D<float4> depthTexture : register(t1);


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_Position0,
  float4 v1 : TEXCOORD0,
  out float4 o0 : SV_Target0)
{
  float4 r0;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.x = depthTexture.SampleLevel(samPoint_s, v1.xy, 0).x;
  r0.y = 1;
  r0.z = dot(projInv_g._m22_m32, r0.xy);
  r0.x = dot(projInv_g._m23_m33, r0.xy);
  r0.x = r0.z / r0.x;
  r0.y = -farZ + -r0.x;
  r0.y = invFarFade * r0.y;
  r0.y = min(1, r0.y);
  r0.y = log2(r0.y);
  r0.y = farFadeExp * r0.y;
  r0.y = exp2(r0.y);
  r0.z = cmp(farZ < -r0.x);
  r0.y = r0.z ? r0.y : 0;
  r0.z = nearZ + r0.x;
  r0.x = cmp(-r0.x < nearZ);
  r0.z = invNearFade * r0.z;
  r0.z = min(1, r0.z);
  r0.z = log2(r0.z);
  r0.z = nearFadeExp * r0.z;
  r0.z = exp2(r0.z);
  o0.w = r0.x ? -r0.z : r0.y;
  if (sss_injection_data.dof_mode > 0.5f) {
    float near_coc = saturate(max(0.0, -o0.w) * max(sss_injection_data.dof_near_scale, 0.0));
    float far_coc = saturate(max(0.0, o0.w) * max(sss_injection_data.dof_far_scale, 0.0));
    float coc_curve = max(sss_injection_data.dof_coc_curve, 0.01);
    near_coc = pow(near_coc, coc_curve);
    far_coc = pow(far_coc, coc_curve);
    o0.w = (o0.w < 0.0) ? -near_coc : far_coc;
  }
  r0.xyz = colorTexture.SampleLevel(samLinear_s, v1.xy, 0).xyz;
  o0.xyz = r0.xyz;
  return;
}
