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

cbuffer cb_deferred_shadow_blur : register(b2)
{
  float4 metrics : packoffset(c0);
  float4 uvClamp : packoffset(c1);
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
  out float o0 : SV_Target0)
{
  const float4 icb[] = { { 0.545455, 0, 0, 0},
                              { 0.168555, 0.518759, 0, 0},
                              { -0.441282, 0.320610, 0, 0},
                              { -0.441282, -0.320610, 0, 0},
                              { 0.168555, -0.518758, 0, 0},
                              { 1.000000, 0, 0, 0},
                              { 0.809017, 0.587785, 0, 0},
                              { 0.309017, 0.951057, 0, 0},
                              { -0.309017, 0.951056, 0, 0},
                              { -0.809017, 0.587785, 0, 0},
                              { -1.000000, 0, 0, 0},
                              { -0.809017, -0.587785, 0, 0},
                              { -0.309017, -0.951057, 0, 0},
                              { 0.309017, -0.951056, 0, 0},
                              { 0.809017, -0.587785, 0, 0} };
  float4 r0,r1,r2,r3,r4;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.z = depthTexture.SampleLevel(samPoint_s, v1.xy, 0).x;
  r1.x = colorTexture.SampleLevel(samPoint_s, v1.xy, 0).x;
  r0.xy = v1.zw * float2(2,-2) + float2(-1,1);
  r0.w = 1;
  r2.z = dot(r0.xyzw, projInv_g._m00_m10_m20_m30);
  r2.x = dot(r0.xyzw, projInv_g._m01_m11_m21_m31);
  r2.y = dot(r0.xyzw, projInv_g._m02_m12_m22_m32);
  r0.x = dot(r0.xyzw, projInv_g._m03_m13_m23_m33);
  r0.xyz = r2.xyz / r0.xxx;
  r1.yzw = ddy_coarse(r0.yzx);
  r0.xzw = ddx_coarse(r0.xyz);
  r2.xyz = r1.yzw * r0.xzw;
  r0.xzw = r1.wyz * r0.zwx + -r2.xyz;
  r0.x = dot(r0.xzw, r0.xzw);
  r0.x = rsqrt(r0.x);
  r0.x = saturate(r0.w * r0.x);
  r0.z = dot(v1.xy, float2(12.9898005,78.2330017));
  r0.z = sin(r0.z);
  r0.z = 43758.5469 * r0.z;
  r0.z = frac(r0.z);
  sincos(r0.z, r2.x, r3.x);
  r4.x = -r2.x;
  r0.zw = float2(-10,-70) + -r0.yy;
  r0.zw = saturate(float2(0.0500000007,0.0500000007) * r0.zw);
  r0.z = r0.z * -8 + 10;
  r1.y = 5 + -r0.z;
  r0.z = r0.w * r1.y + r0.z;
  r0.x = r0.x * 0.800000012 + 0.200000003;
  r0.x = r0.z * r0.x;
  r0.xz = metrics.xy * r0.xx;
  r4.y = r3.x;
  r4.z = r2.x;
  r2.y = 1;
  r1.yz = float2(0,0);
  r0.w = 0;
  while (true) {
    r1.w = cmp((uint)r0.w >= 15);
    if (r1.w != 0) break;
    r3.x = dot(icb[r0.w+0].yx, r4.xy);
    r3.y = dot(icb[r0.w+0].yx, r4.yz);
    r2.zw = r3.xy * r0.xz + v1.xy;
    r2.zw = min(uvClamp.xy, r2.zw);
    r2.x = depthTexture.SampleLevel(samLinear_s, r2.zw, 0).x;
    r1.w = dot(projInv_g._m22_m32, r2.xy);
    r2.x = dot(projInv_g._m23_m33, r2.xy);
    r1.w = r1.w / r2.x;
    r1.w = r1.w + -r0.y;
    r1.w = -abs(r1.w) * 4 + 1;
    r1.w = max(0, r1.w);
    r1.w = r1.w * r1.w;
    r1.w = r1.w * r1.w;
    r3.xy = v1.xy + -r2.zw;
    r2.x = dot(r3.xy, r3.xy);
    r2.x = sqrt(r2.x);
    r3.x = r2.x * r1.w;
    r2.z = colorTexture.SampleLevel(samLinear_s, r2.zw, 0).x;
    r1.z = r1.w * r2.x + r1.z;
    r1.y = r2.z * r3.x + r1.y;
    r0.w = (int)r0.w + 1;
  }
  r0.x = cmp(r1.z == 0.000000);
  r0.y = r0.x ? 1.000000 : 0;
  r0.y = r1.z + r0.y;
  r0.y = r1.y / r0.y;
  o0.x = r0.x ? r1.x : r0.y;
  return;
}