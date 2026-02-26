// ---- Created with 3Dmigoto v1.4.1 on Thu Feb 19 20:15:25 2026

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

cbuffer cb_local : register(b2)
{
  uint maxRayCount_g : packoffset(c0);
  float rayLength_g : packoffset(c0.y);
  float2 prevResolutionScaling_g : packoffset(c0.z);
}

SamplerState samLinear_s : register(s0);
SamplerState samPoint_s : register(s1);
Texture2D<float4> colorTexture : register(t0);
Texture2D<float4> depthTexture : register(t1);
Texture2D<uint4> mrtTexture0 : register(t2);
Texture2D<uint4> mrtTexture1 : register(t3);
Texture2D<float4> prevSSRTexture : register(t4);

#include "./kai-vanillaplus.h"


// 3Dmigoto declarations
#define cmp -

static const float kPi = 3.14159274;
static const float kInvU16 = 3.05180438e-05;
static const float kInvU8 = 0.00392156886;

float ViewZFromDepth(const float depth_value) {
  float view_z_num = dot(projInv_g._m22_m32, float2(depth_value, 1.0));
  float view_z_den = dot(projInv_g._m23_m33, float2(depth_value, 1.0));
  return view_z_num / view_z_den;
}

float3 DecodeMrt0NormalView(const uint4 mrt_sample) {
  float2 enc = float2((float)mrt_sample.x, (float)mrt_sample.y) * kInvU16 + float2(-1.0, -1.0);
  float sn, cs;
  sincos(kPi * enc.x, sn, cs);
  float xy = sqrt(saturate(1.0 - enc.y * enc.y));
  float3 normal_world = normalize(float3(cs * xy, sn * xy, enc.y));

  float3 normal_view;
  normal_view.x = dot(normal_world, view_g._m00_m10_m20);
  normal_view.y = dot(normal_world, view_g._m01_m11_m21);
  normal_view.z = dot(normal_world, view_g._m02_m12_m22);
  return normalize(normal_view);
}


void main(
  float4 v0 : SV_Position0,
  float4 v1 : TEXCOORD0,
  out float4 o0 : SV_Target0)
{
  float4 r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10;
  uint4 bitmask, uiDest;
  float4 fDest;
  uint4 ssr_mrt0_center;
  uint4 ssr_mrt1_center;
  float2 ssr_mrt0_dims;
  float2 ssr_mrt0_max;
  float2 ssr_mrt1_dims;
  float2 ssr_mrt1_max;
  float ssr_origin_material = 0;
  float3 ssr_origin_view_pos = float3(0, 0, 0);
  float3 ssr_origin_normal_vs = float3(0, 0, 1);
  float ssr_current_view_z = 0;
  float2 ssr_sample_uv = float2(0, 0);
  float2 ssr_sample_uv_scaled = float2(0, 0);
  float2 ssr_reproj_uv = float2(0, 0);
  bool ssr_improved_mode = (sss_injection_data.ssr_mode >= 0.5);

  mrtTexture0.GetDimensions(0, fDest.x, fDest.y, fDest.z);
  r0.xy = fDest.xy;
  ssr_mrt0_dims = r0.xy;
  ssr_mrt0_max = max(ssr_mrt0_dims + float2(-1, -1), float2(0, 0));
  r0.zw = v1.xy * r0.xy;
  r1.xy = (int2)r0.zw;
  r1.zw = float2(0,0);
  ssr_mrt0_center = mrtTexture0.Load(r1.xyz);
  r1.xyz = ssr_mrt0_center.xyz;
  r0.z = (float)((ssr_mrt0_center.z >> 8) & 2u);
  if (r0.z == 0) {
    r2.xyz = colorTexture.SampleLevel(samPoint_s, v1.xy, 0).xyz;
    o0.xyz = r2.xyz;
    o0.w = 0;
    return;
  }
  mrtTexture1.GetDimensions(0, fDest.x, fDest.y, fDest.z);
  ssr_mrt1_dims = fDest.xy;
  ssr_mrt1_max = max(ssr_mrt1_dims + float2(-1, -1), float2(0, 0));
  r0.zw = fDest.xy;
  r0.zw = v1.xy * r0.zw;
  r2.xy = (int2)r0.zw;
  r2.zw = float2(0,0);
  ssr_mrt1_center = mrtTexture1.Load(r2.xyz);
  r0.z = ssr_mrt1_center.x;
  ssr_origin_material = (float)(ssr_mrt1_center.y & 255u) * kInvU8;
  r2.z = depthTexture.SampleLevel(samPoint_s, v1.xy, 0).x;
  ssr_current_view_z = ViewZFromDepth(r2.z);
  r2.xy = v1.zw * float2(2,-2) + float2(-1,1);
  r2.w = 1;
  r3.x = dot(r2.xyzw, projInv_g._m00_m10_m20_m30);
  r3.y = dot(r2.xyzw, projInv_g._m01_m11_m21_m31);
  r3.z = dot(r2.xyzw, projInv_g._m02_m12_m22_m32);
  r0.w = dot(r2.xyzw, projInv_g._m03_m13_m23_m33);
  r3.xyz = r3.xyz / r0.www;
  ssr_origin_view_pos = r3.xyz;
  r1.xy = (uint2)r1.xy;
  r1.zw = r1.xy * float2(3.05180438e-05,3.05180438e-05) + float2(-1,-1);
  r0.w = 3.14159274 * r1.z;
  sincos(r0.w, r4.x, r5.x);
  r0.w = -r1.w * r1.w + 1;
  r0.w = sqrt(r0.w);
  r1.x = r5.x * r0.w;
  r1.y = r4.x * r0.w;
  r0.w = dot(r1.xyw, r1.xyw);
  r0.w = rsqrt(r0.w);
  r1.xyz = r1.xyw * r0.www;
  r4.x = dot(r1.xyz, view_g._m00_m10_m20);
  r4.y = dot(r1.xyz, view_g._m01_m11_m21);
  r4.z = dot(r1.xyz, view_g._m02_m12_m22);
  ssr_origin_normal_vs = normalize(r4.xyz);
  r0.w = dot(r3.xyz, r3.xyz);
  r0.w = rsqrt(r0.w);
  r1.xyz = r3.xyz * r0.www;
  r0.w = dot(r1.xyz, r4.xyz);
  r0.w = r0.w + r0.w;
  r1.xyz = r4.xyz * -r0.www + r1.xyz;
  r0.w = dot(-r3.xyz, -r3.xyz);
  r0.w = rsqrt(r0.w);
  r5.xyz = -r3.xyz * r0.www;
  r0.w = dot(r5.xyz, r4.xyz);
  r0.z = (uint)r0.z;
  r0.z = 0.0152590219 * r0.z;
  float ssr_ray_count_scale = ssr_improved_mode ? clamp(sss_injection_data.ssr_ray_count_scale, 0.5, 8.0) : 1.0;
  uint ssr_max_ray_count = max(1u, (uint)round((float)maxRayCount_g * ssr_ray_count_scale));
  r1.w = (float)ssr_max_ray_count;
  r0.z = r0.z / r1.w;
  r4.xyz = sceneTime_g * r3.xyz;
  r1.w = dot(r4.xyz, float3(12.9898005,78.2330017,56.7869987));
  r1.w = sin(r1.w);
  r1.w = 43758.5469 * r1.w;
  r1.w = frac(r1.w);
  r4.xy = float2(0.899999976,0.200000048) * r0.zz;
  r0.z = r1.w * r4.y + r4.x;
  r5.xyz = r1.xyz * r0.zzz;
  r3.xyw = r1.xyz * r0.zzz + r3.xyz;
  r0.z = 1 + -abs(r0.w);
  r0.z = r0.z * r0.z;
  r0.z = r0.z * r0.z;
  r0.z = r0.z * r0.z;
  r0.z = -r3.z * r0.z;
  r0.z = 0.0199999996 * r0.z;
  r3.xyz = r1.xyz * r0.zzz + r3.xyw;
  r6.w = 1;
  r7.y = 1;
  r8.xyz = r5.xyz;
  r0.zw = float2(0,0);
  r1.w = 0;
  r9.xyz = r3.xyz;
  r3.w = 0;
  while (true) {
    r4.z = cmp((uint)r3.w >= ssr_max_ray_count);
    if (r4.z != 0) break;
    r6.xyz = r9.xyz;
    r10.x = dot(r6.xyzw, proj_g._m00_m10_m20_m30);
    r10.y = dot(r6.xyzw, proj_g._m01_m11_m21_m31);
    r4.z = dot(r6.xyzw, proj_g._m03_m13_m23_m33);
    r4.zw = r10.xy / r4.zz;
    r7.zw = float2(0.5,0.5) * r4.zw;
    r10.xy = r4.zw * float2(0.5,0.5) + float2(0.5,0.5);
    r4.z = max(abs(r7.z), abs(r7.w));
    r4.z = cmp(0.5 < r4.z);
    if (r4.z != 0) {
      r0.zw = r10.xy;
      break;
    }
    r10.w = 1 + -r10.y;
    r10.z = 1 + -r10.y;
    r4.zw = resolutionScaling_g.xy * r10.xz;
    r7.x = depthTexture.SampleLevel(samPoint_s, r4.zw, 0).x;
    r4.z = dot(projInv_g._m22_m32, r7.xy);
    r4.w = dot(projInv_g._m23_m33, r7.xy);
    r4.z = r4.z / r4.w;
    float ray_depth_delta = -r9.z + r4.z;
    if (ray_depth_delta > 0.0) {
      r0.zw = r10.xz;
      r1.w = -1;
      break;
    }
    r7.xzw = sceneTime_g * r6.xyz;
    r4.z = dot(r7.xzw, float3(12.9898005,78.2330017,56.7869987));
    r4.z = sin(r4.z);
    r4.z = 43758.5469 * r4.z;
    r4.z = frac(r4.z);
    r4.z = r4.z * r4.y + r4.x;
    r8.xyz = r4.zzz * r1.xyz;
    r9.xyz = r1.xyz * r4.zzz + r6.xyz;
    r3.w = (int)r3.w + 1;
    r0.zw = r10.xw;
    r1.w = 0;
  }
  if (r1.w != 0) {
    r1.xyz = r9.xyz + -r8.xyz;
    r3.xyz = float3(0.03125,0.03125,0.03125) * r8.xyz;
    r4.w = 1;
    r5.y = 1;
    r4.xyz = r1.xyz;
    r6.xy = r0.zw;
    r1.w = 16;
    r3.w = 16;
    r5.z = 0;
    while (true) {
      r5.w = cmp((int)r5.z >= 32);
      if (r5.w != 0) break;
      r7.xyz = r3.xyz * r3.www;
      r8.xyz = sceneTime_g * r4.xyz;
      r5.w = dot(r8.xyz, float3(12.9898005,78.2330017,56.7869987));
      r5.w = sin(r5.w);
      r5.w = 43758.5469 * r5.w;
      r5.w = frac(r5.w);
      r5.w = r5.w * 0.200000048 + 0.899999976;
      r4.xyz = r7.xyz * r5.www + r4.xyz;
      r7.x = dot(r4.xyzw, proj_g._m00_m10_m20_m30);
      r7.y = dot(r4.xyzw, proj_g._m01_m11_m21_m31);
      r5.w = dot(r4.xyzw, proj_g._m03_m13_m23_m33);
      r7.xy = r7.xy / r5.ww;
      r6.xz = r7.xy * float2(0.5,0.5) + float2(0.5,0.5);
      r1.w = 0.5 * r1.w;
      r6.y = 1 + -r6.z;
      r6.zw = resolutionScaling_g.xy * r6.xy;
      r5.x = depthTexture.SampleLevel(samPoint_s, r6.zw, 0).x;
      r5.w = dot(projInv_g._m22_m32, r5.xy);
      r5.x = dot(projInv_g._m23_m33, r5.xy);
      r5.x = r5.w / r5.x;
      float refine_depth_delta = r5.x + -r4.z;
      r3.w = (refine_depth_delta > 0.0) ? -r1.w : r1.w;
      r5.z = (int)r5.z + 1;
    }
    r0.zw = r6.xy;
    r1.xy = float2(-0.5,-0.5) + r0.zw;
    r1.x = dot(r1.xy, r1.xy);
    r1.x = sqrt(r1.x);
    r1.x = r1.x + r1.x;
    r1.y = r1.x * r1.x;
    r1.x = -r1.x * r1.y + 1;
    r1.yz = resolutionScaling_g.xy * r0.zw;
    r0.xy = r1.yz * r0.xy;
    r3.xy = (int2)r0.xy;
    r3.zw = float2(0,0);
    uint recursive_ssr_flags = mrtTexture0.Load(r3.xyz).z;
    uint recursive_ssr_mask = (recursive_ssr_flags >> 8) & 2u;
    r0.x = (recursive_ssr_mask != 0u) ? 0.0 : r1.x;
  } else {
    r1.xy = float2(-0.5,-0.5) + r0.zw;
    r0.y = dot(r1.xy, r1.xy);
    r0.y = sqrt(r0.y);
    r0.y = r0.y + r0.y;
    r1.x = r0.y * r0.y;
    r0.x = -r0.y * r1.x + 1;
  }

  ssr_sample_uv = r0.zw;

  ssr_sample_uv_scaled = resolutionScaling_g.xy * ssr_sample_uv;
  r0.yz = ssr_sample_uv_scaled;
  r1.xyz = colorTexture.SampleLevel(samPoint_s, r0.yz, 0).xyz;
  r1.w = max(0, r0.x);
  r0.x = dot(r2.xyzw, viewProjInv_g._m00_m10_m20_m30);
  r0.y = dot(r2.xyzw, viewProjInv_g._m01_m11_m21_m31);
  r0.z = dot(r2.xyzw, viewProjInv_g._m02_m12_m22_m32);
  r0.w = dot(r2.xyzw, viewProjInv_g._m03_m13_m23_m33);
  r0.xyzw = r0.xyzw / r0.wwww;
  r2.x = dot(r0.xyzw, prevViewProj_g._m00_m10_m20_m30);
  r2.y = dot(r0.xyzw, prevViewProj_g._m01_m11_m21_m31);
  r0.x = dot(r0.xyzw, prevViewProj_g._m03_m13_m23_m33);
  r0.xy = r2.xy / r0.xx;
  r0.xy = r0.xy * float2(0.5,0.5) + float2(0.5,0.5);
  r0.z = 1 + -r0.y;
  ssr_reproj_uv = float2(r0.x, r0.z);
  r0.yw = -v1.zw + r0.xz;
  r0.y = dot(r0.yw, r0.yw);
  r0.y = sqrt(r0.y);
  r0.y = -1442.69507 * r0.y;
  r0.y = exp2(r0.y);
  r0.y = r0.y * -0.300000012 + 0.400000006;
  r0.xz = resolutionScaling_g.xy * r0.xz;
  r0.xz = prevResolutionScaling_g.xy * r0.xz;
  r2.xyzw = prevSSRTexture.SampleLevel(samLinear_s, r0.xz, 0).xyzw;

  r1.xyzw = -r2.xyzw + r1.xyzw;
  o0.xyzw = r0.yyyy * r1.xyzw + r2.xyzw;
  return;
}
