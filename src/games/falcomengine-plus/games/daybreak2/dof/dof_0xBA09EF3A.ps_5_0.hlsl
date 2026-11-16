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

cbuffer cb_local : register(b2)
{
  float4 offsetsAndWeights[15] : packoffset(c0);
  float nearZ : packoffset(c15);
  float farZ : packoffset(c15.y);
  float invNearFade : packoffset(c15.z);
  float invFarFade : packoffset(c15.w);
  float nearFadeExp : packoffset(c16);
  float farFadeExp : packoffset(c16.y);
}

SamplerState samPoint_s : register(s1);
Texture2D<float4> colorTexture : register(t0);
Texture2D<float4> depthTexture : register(t1);


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_Position0,
  float4 v1 : TEXCOORD0,
  out float4 o0 : SV_Target0)
{
  float4 r0,r1,r2;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.x = depthTexture.SampleLevel(samPoint_s, v1.xy, 0).x;
  r0.yw = float2(1,1);
  r1.x = dot(projInv_g._m22_m32, r0.xy);
  r0.x = dot(projInv_g._m23_m33, r0.xy);
  r0.x = r1.x / r0.x;
  r1.xyzw = float4(0,0,0,0);
  r0.y = 0;
  while (true) {
    r2.x = cmp((uint)r0.y >= 15);
    if (r2.x != 0) break;
    r2.xy = offsetsAndWeights[r0.y].xy + v1.xy;
    r0.z = depthTexture.SampleLevel(samPoint_s, r2.xy, 0).x;
    r2.x = dot(projInv_g._m22_m32, r0.zw);
    r0.z = dot(projInv_g._m23_m33, r0.zw);
    r0.z = r2.x / r0.z;
    r0.z = min(-r0.x, -r0.z);
    r2.x = cmp(r0.z < nearZ);
    r2.y = nearZ + -r0.z;
    r2.y = invNearFade * r2.y;
    r2.y = min(1, r2.y);
    r2.y = log2(r2.y);
    r2.y = nearFadeExp * r2.y;
    r2.y = exp2(r2.y);
    r2.z = cmp(farZ < r0.z);
    r0.z = -farZ + r0.z;
    r0.z = invFarFade * r0.z;
    r0.z = min(1, r0.z);
    r0.z = log2(r0.z);
    r0.z = farFadeExp * r0.z;
    r0.z = exp2(r0.z);
    r0.z = r2.z ? r0.z : 0;
    r0.z = r2.x ? r2.y : r0.z;
    r2.xy = offsetsAndWeights[r0.y].xy * r0.zz + v1.xy;
    r2.xyzw = colorTexture.SampleLevel(samPoint_s, r2.xy, 0).xyzw;
    r1.xyzw = r2.xyzw * offsetsAndWeights[r0.y].zzzz + r1.xyzw;
    r0.y = (int)r0.y + 1;
  }
  o0.xyzw = r1.xyzw;
  return;
}