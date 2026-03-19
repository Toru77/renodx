// ---- Created with 3Dmigoto v1.4.1 on Wed Mar  4 19:23:33 2026

struct InstanceParam
{
    float4x4 world;                // Offset:    0
    float4 color;                  // Offset:   64
    float4 uv;                     // Offset:   80
    float4 param;                  // Offset:   96
    uint boneAddress;              // Offset:  112
    float3 param2;                 // Offset:  116
    float4x4 prevWorld;            // Offset:  128
};

cbuffer cb_tex_swizzle : register(b10)
{
  uint swizzle_flags_g : packoffset(c0);
}

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

cbuffer cb_local : register(b5)
{
  float2 uvScroll0_g : packoffset(c0);
  float2 uvScroll1_g : packoffset(c0.z);
  float2 uvScroll2_g : packoffset(c1);
  float emissive_g : packoffset(c1.z);
  float materialFogIntensity_g : packoffset(c1.w);
  float opacity_g : packoffset(c2);
  float translucency_g : packoffset(c2.y);
  float alphaTestThreshold_g : packoffset(c2.z);
  float shakeScale_g : packoffset(c2.w);
  float shakeSpeed_g : packoffset(c3);
  float shakeFlexibility_g : packoffset(c3.y);
  float shakeFreq_g : packoffset(c3.z);
  float shakeWindScale_g : packoffset(c3.w);
  float ssaoIntensity_g : packoffset(c4);
  float3 shadowColor_g : packoffset(c4.y);
  float glowShadowFadeRatio_g : packoffset(c5);
  float3 rimLightColor_g : packoffset(c5.y);
  float rimLightPower_g : packoffset(c6);
  float3 specularColor_g : packoffset(c6.y);
  float specularShadowFadeRatio_g : packoffset(c7);
  float rimIntensity_g : packoffset(c7.y);
  float dynamicLightIntensity_g : packoffset(c7.z);
  float fresnel0_g : packoffset(c7.w);
  float specularGlossiness0_g : packoffset(c8);
  float shadowCastOffset_g : packoffset(c8.y);
  float volumeFogInvalidity_g : packoffset(c8.z);
  uint materialID_g : packoffset(c8.w);
}

SamplerState Smpl0_s : register(s0);
Texture2D<float4> Tex0 : register(t0);
StructuredBuffer<InstanceParam> instances_g : register(t15);

#include "./kai-vanillaplus.h"

// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_Position0,
  float3 v1 : NORMAL0,
  nointerpolation uint4 v2 : TEXCOORD0,
  float4 v3 : TEXCOORD1,
  float4 v4 : TEXCOORD2,
  float4 v5 : TEXCOORD7,
  uint v6 : SV_IsFrontFace0,
  out float4 o0 : SV_Target0,
  out uint4 o1 : SV_Target1,
  out uint4 o2 : SV_Target2,
  out float2 o3 : SV_Target3)
{
  const float4 icb[] = { { 1.000000, 0, 0, 0},
                              { 0, 1.000000, 0, 0},
                              { 0, 0, 1.000000, 0},
                              { 0, 0, 0, 1.000000} };
  float4 r0,r1,r2,r3;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.x = dot(v3.xyz, clipPlane_g.xyz);
  r0.x = -clipPlane_g.w + r0.x;
  r0.x = cmp(r0.x < 0);
  if (r0.x != 0) discard;
  r0.xy = float2(0.25,0.25) * v0.xy;
  r0.zw = cmp(r0.xy >= -r0.xy);
  r0.xy = frac(abs(r0.xy));
  r0.xy = r0.zw ? r0.xy : -r0.xy;
  r0.xy = float2(4,4) * r0.xy;
  r0.xy = (int2)r0.xy;
  r1.x = dot(ditherMtx_g._m00_m10_m20_m30, icb[r0.x+0].xyzw);
  r1.y = dot(ditherMtx_g._m01_m11_m21_m31, icb[r0.x+0].xyzw);
  r1.z = dot(ditherMtx_g._m02_m12_m22_m32, icb[r0.x+0].xyzw);
  r1.w = dot(ditherMtx_g._m03_m13_m23_m33, icb[r0.x+0].xyzw);
  r0.x = dot(r1.xyzw, icb[r0.y+0].xyzw);
  r0.y = 1 + -r0.x;
  r1.x = instances_g[v2.x].param.x;
  r1.y = instances_g[v2.x].param.y;
  r1.z = instances_g[v2.x].param.z;
  r0.z = cmp(0 < r1.z);
  r0.x = r0.z ? r0.y : r0.x;
  r2.x = viewInv_g._m30 + -v3.x;
  r2.y = viewInv_g._m31 + -v3.y;
  r2.z = viewInv_g._m32 + -v3.z;
  r0.y = dot(r2.xyz, r2.xyz);
  r0.y = sqrt(r0.y);
  r0.y = r0.y + -r1.x;
  r0.y = r0.y * r1.y;
  r0.y = min(1, r0.y);
  r0.y = max(disableMapObjNearFade_g, r0.y);
  r1.x = instances_g[v2.x].color.x;
  r1.y = instances_g[v2.x].color.y;
  r1.z = instances_g[v2.x].color.z;
  r1.w = instances_g[v2.x].color.w;
  r1.w = (opacity_g * sss_injection_data.foliage_opacity_scale) * r1.w;
  r0.y = r1.w * r0.y;
  r0.x = r0.y * mapColor_g.w + -r0.x;
  r0.y = mapColor_g.w * r0.y;
  r0.xy = cmp(r0.xy < float2(0,0.999989986));
  if (r0.x != 0) discard;
  r0.x = 1 & swizzle_flags_g;
  r0.zw = v4.xy * float2(1,-1) + float2(0,1);
  r2.xyzw = Tex0.Sample(Smpl0_s, r0.zw).xyzw;
  r3.x = r2.x;
  r3.w = 1;
  r2.xyzw = r0.xxxx ? r3.xxxw : r2.xyzw;
  r0.x = -alphaTestThreshold_g + r2.w;
  r1.xyzw = r2.xyzw * r1.xyzw;
  r0.x = cmp(r0.x < 0);
  if (r0.x != 0) discard;
  r0.x = r0.y ? 0 : 1;
  uint foliage_class = r0.y ? 3327u : 2303u;
  foliage_class |= 0x80000000u;
  o1.z = foliage_class;
  r0.y = (ssaoIntensity_g * sss_injection_data.foliage_ssao_scale) * r1.w;
  o0.xyz = r1.xyz;
  o0.w = r0.y * r0.x;
  r0.xyz = v3.xyz;
  r0.w = 1;
  r1.x = dot(r0.xyzw, view_g._m00_m10_m20_m30);
  r1.z = ddy_coarse(r1.x);
  r2.w = ddx_coarse(r1.x);
  r1.x = dot(r0.xyzw, view_g._m01_m11_m21_m31);
  r0.x = dot(r0.xyzw, view_g._m02_m12_m22_m32);
  r1.w = ddy_coarse(r1.x);
  r2.y = ddx_coarse(r1.x);
  r1.y = ddy_coarse(r0.x);
  r2.z = ddx_coarse(r0.x);
  r0.xyz = r2.yzw * r1.yzw;
  r0.xyz = r1.wyz * r2.zwy + -r0.xyz;
  r0.w = dot(r0.xyz, r0.xyz);
  r0.w = rsqrt(r0.w);
  r0.yzw = r0.xyz * r0.www;
  r1.x = max(abs(r0.z), abs(r0.y));
  r1.x = 1 / r1.x;
  r1.y = min(abs(r0.z), abs(r0.y));
  r1.x = r1.y * r1.x;
  r1.y = r1.x * r1.x;
  r1.z = r1.y * 0.0208350997 + -0.0851330012;
  r1.z = r1.y * r1.z + 0.180141002;
  r1.z = r1.y * r1.z + -0.330299497;
  r1.y = r1.y * r1.z + 0.999866009;
  r1.z = r1.x * r1.y;
  r1.z = r1.z * -2 + 1.57079637;
  r1.w = cmp(abs(r0.y) < abs(r0.z));
  r1.z = r1.w ? r1.z : 0;
  r1.x = r1.x * r1.y + r1.z;
  r1.y = cmp(r0.y < -r0.y);
  r1.y = r1.y ? -3.141593 : 0;
  r1.x = r1.x + r1.y;
  r1.y = min(r0.z, r0.y);
  r1.y = cmp(r1.y < -r1.y);
  r0.y = max(r0.z, r0.y);
  r0.y = cmp(r0.y >= -r0.y);
  r0.y = r0.y ? r1.y : 0;
  r0.y = r0.y ? -r1.x : r1.x;
  r0.x = 0.318309873 * r0.y;
  r0.xy = float2(1,1) + r0.xw;
  r0.xy = float2(127.5,127.5) * r0.xy;
  r0.xy = (uint2)r0.xy;
  r0.xy = min(uint2(255,255), (uint2)r0.xy);
  o1.w = mad((int)r0.y, 256, (int)r0.x);
  r0.x = dot(v1.xyz, v1.xyz);
  r0.x = rsqrt(r0.x);
  r0.xyz = v1.xyz * r0.xxx;
  r0.w = v6.x ? -1 : 1;
  r1.yzw = r0.xyz * r0.www;
  r0.x = dot(r0.xyz, -lightDirection_g.xyz);
  r0.x = r0.x * r0.w;
  r0.xy = r0.xx * float2(0.5,-0.5) + float2(0.5,0.5);
  r0.z = max(abs(r1.z), abs(r1.y));
  r0.z = 1 / r0.z;
  r0.w = min(abs(r1.z), abs(r1.y));
  r0.z = r0.w * r0.z;
  r0.w = r0.z * r0.z;
  r2.x = r0.w * 0.0208350997 + -0.0851330012;
  r2.x = r0.w * r2.x + 0.180141002;
  r2.x = r0.w * r2.x + -0.330299497;
  r0.w = r0.w * r2.x + 0.999866009;
  r2.x = r0.z * r0.w;
  r2.x = r2.x * -2 + 1.57079637;
  r2.y = cmp(abs(r1.y) < abs(r1.z));
  r2.x = r2.y ? r2.x : 0;
  r0.z = r0.z * r0.w + r2.x;
  r0.w = cmp(r1.y < -r1.y);
  r0.w = r0.w ? -3.141593 : 0;
  r0.z = r0.z + r0.w;
  r0.w = min(r1.z, r1.y);
  r0.w = cmp(r0.w < -r0.w);
  r1.y = max(r1.z, r1.y);
  r1.y = cmp(r1.y >= -r1.y);
  r0.w = r0.w ? r1.y : 0;
  r0.z = r0.w ? -r0.z : r0.z;
  r1.x = 0.318309873 * r0.z;
  r0.zw = float2(1,1) + r1.xw;
  r0.zw = float2(32767.5,32767.5) * r0.zw;
  r0.zw = (uint2)r0.zw;
  o1.xy = min(uint2(65535,65535), (uint2)r0.zw);
  r0.y = max(r0.x, r0.y);
  r0.y = r0.y + -r0.x;
  r0.x = (translucency_g * sss_injection_data.foliage_translucency_scale) * r0.y + r0.x;
  r0.x = 255 * r0.x;
  r0.x = (uint)r0.x;
  r0.x = min(255, (uint)r0.x);
  o2.z = mad((int)r0.x, 256, 255);
  o2.x = materialID_g;
  o2.yw = float2(9.18340949e-41,0);
  r0.xy = v5.xy / v5.ww;
  r0.xy = r0.xy * float2(0.5,-0.5) + float2(0.5,0.5);
  r0.xy = r0.xy * vpSize_g.xy + -v0.xy;
  o3.xy = -motionJitterOffset_g.xy + r0.xy;
  return;
}
