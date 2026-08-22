// ---- Created with 3Dmigoto v1.4.1 on Sat Aug 22 12:30:43 2026
#include "../../shared.h"
// DOF1 of Sora1st. DOF2 is shared with Sora2nd.

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
  if (shader_injection_data.dof_mode > 0.5f) {
    float near_coc = saturate(max(0.0, -o0.w) * max(shader_injection_data.dof_near_scale, 0.0));
    float far_coc = saturate(max(0.0, o0.w) * max(shader_injection_data.dof_far_scale, 0.0));
    float coc_curve = max(shader_injection_data.dof_coc_curve, 0.01);
    near_coc = pow(near_coc, coc_curve);
    far_coc = pow(far_coc, coc_curve);
    o0.w = (o0.w < 0.0) ? -near_coc : far_coc;
  }
  r0.xyz = colorTexture.SampleLevel(samLinear_s, v1.xy, 0).xyz;
  o0.xyz = r0.xyz;
  return;
}