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

SamplerState samPoint_s : register(s1);
Texture2D<float4> colorTexture : register(t0);
Texture2D<float4> depthTexture : register(t1);
Texture2D<uint4> mrtTexture0 : register(t2);
Texture2D<uint2> mrtTexture2 : register(t3);


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_Position0,
  float4 v1 : TEXCOORD0,
  out float4 o0 : SV_Target0)
{
  float4 r0,r1,r2,r3,r4,r5,r6,r7,r8,r9;
  uint4 bitmask, uiDest;
  float4 fDest;

  mrtTexture0.GetDimensions(0, fDest.x, fDest.y, fDest.z);
  r0.xy = fDest.xy;
  r0.zw = v1.xy * r0.xy;
  r1.xy = (int2)r0.zw;
  r1.zw = float2(0,0);
  r1.xyz = mrtTexture0.Load(r1.xyz).xyw;
  r0.z = (int)r1.z & 2;
  if (r0.z == 0) {
    r2.xyz = colorTexture.SampleLevel(samPoint_s, v1.xy, 0).xyz;
    o0.xyz = r2.xyz;
    o0.w = 0;
    return;
  }
  mrtTexture2.GetDimensions(0, fDest.x, fDest.y, fDest.z);
  r0.zw = fDest.xy;
  r0.zw = v1.xy * r0.zw;
  r2.xy = (int2)r0.zw;
  r2.zw = float2(0,0);
  r0.z = mrtTexture2.Load(r2.xyz).y;
  r2.z = depthTexture.SampleLevel(samPoint_s, v1.xy, 0).x;
  r2.xy = v1.zw * float2(2,-2) + float2(-1,1);
  r2.w = 1;
  r3.x = dot(r2.xyzw, projInv_g._m00_m10_m20_m30);
  r3.y = dot(r2.xyzw, projInv_g._m01_m11_m21_m31);
  r3.z = dot(r2.xyzw, projInv_g._m02_m12_m22_m32);
  r0.w = dot(r2.xyzw, projInv_g._m03_m13_m23_m33);
  r2.xyz = r3.xyz / r0.www;
  r1.xy = (uint2)r1.xy;
  r1.zw = r1.xy * float2(3.05180438e-05,3.05180438e-05) + float2(-1,-1);
  r0.w = 3.14159274 * r1.z;
  sincos(r0.w, r3.x, r4.x);
  r0.w = -r1.w * r1.w + 1;
  r0.w = sqrt(r0.w);
  r1.x = r4.x * r0.w;
  r1.y = r3.x * r0.w;
  r0.w = dot(r1.xyw, r1.xyw);
  r0.w = rsqrt(r0.w);
  r1.xyz = r1.xyw * r0.www;
  r3.x = dot(r1.xyz, view_g._m00_m10_m20);
  r3.y = dot(r1.xyz, view_g._m01_m11_m21);
  r3.z = dot(r1.xyz, view_g._m02_m12_m22);
  r0.w = dot(r2.xyz, r2.xyz);
  r0.w = rsqrt(r0.w);
  r1.xyz = r2.xyz * r0.www;
  r0.w = dot(r1.xyz, r3.xyz);
  r0.w = r0.w + r0.w;
  r1.xyz = r3.xyz * -r0.www + r1.xyz;
  r0.w = dot(-r2.xyz, -r2.xyz);
  r0.w = rsqrt(r0.w);
  r4.xyz = -r2.xyz * r0.www;
  r0.w = dot(r4.xyz, r3.xyz);
  r0.z = (uint)r0.z;
  r0.z = 0.0152590219 * r0.z;
  r1.w = maxRayCount_g;
  r0.z = r0.z / r1.w;
  r3.xyz = sceneTime_g + r2.xyz;
  r1.w = dot(r3.xyz, float3(12.9898005,78.2330017,56.7869987));
  r1.w = sin(r1.w);
  r1.w = 43758.5469 * r1.w;
  r1.w = frac(r1.w);
  r3.xy = float2(0.899999976,0.200000048) * r0.zz;
  r0.z = r1.w * r3.y + r3.x;
  r4.xyz = r1.xyz * r0.zzz;
  r2.xyw = r1.xyz * r0.zzz + r2.xyz;
  r0.z = 1 + -abs(r0.w);
  r0.z = r0.z * r0.z;
  r0.z = r0.z * r0.z;
  r0.z = r0.z * r0.z;
  r0.z = -r2.z * r0.z;
  r0.z = 0.0199999996 * r0.z;
  r2.xyz = r1.xyz * r0.zzz + r2.xyw;
  r5.w = 1;
  r6.y = 1;
  r7.xyz = r4.xyz;
  r0.zw = float2(0,0);
  r1.w = 0;
  r8.xyz = r2.xyz;
  r2.w = 0;
  while (true) {
    r3.z = cmp((uint)r2.w >= maxRayCount_g);
    if (r3.z != 0) break;
    r5.xyz = r8.xyz;
    r9.x = dot(r5.xyzw, proj_g._m00_m10_m20_m30);
    r9.y = dot(r5.xyzw, proj_g._m01_m11_m21_m31);
    r3.z = dot(r5.xyzw, proj_g._m03_m13_m23_m33);
    r3.zw = r9.xy / r3.zz;
    r6.zw = float2(0.5,0.5) * r3.zw;
    r9.xy = r3.zw * float2(0.5,0.5) + float2(0.5,0.5);
    r3.z = max(abs(r6.z), abs(r6.w));
    r3.z = cmp(0.5 < r3.z);
    if (r3.z != 0) {
      r0.zw = r9.xy;
      break;
    }
    r9.w = 1 + -r9.y;
    r9.z = 1 + -r9.y;
    r3.zw = resolutionScaling_g.xy * r9.xz;
    r6.x = depthTexture.SampleLevel(samPoint_s, r3.zw, 0).x;
    r3.z = dot(projInv_g._m22_m32, r6.xy);
    r3.w = dot(projInv_g._m23_m33, r6.xy);
    r3.z = r3.z / r3.w;
    r3.z = -r8.z + r3.z;
    r3.w = cmp(0 < r3.z);
    r3.z = cmp(r3.z < 10);
    r3.z = r3.z ? r3.w : 0;
    if (r3.z != 0) {
      r0.zw = r9.xz;
      r1.w = -1;
      break;
    }
    r6.xzw = sceneTime_g * r5.xyz;
    r3.z = dot(r6.xzw, float3(12.9898005,78.2330017,56.7869987));
    r3.z = sin(r3.z);
    r3.z = 43758.5469 * r3.z;
    r3.z = frac(r3.z);
    r3.z = r3.z * r3.y + r3.x;
    r7.xyz = r3.zzz * r1.xyz;
    r8.xyz = r1.xyz * r3.zzz + r5.xyz;
    r2.w = (int)r2.w + 1;
    r0.zw = r9.xw;
    r1.w = 0;
  }
  if (r1.w != 0) {
    r1.xyz = r8.xyz + -r7.xyz;
    r2.xyz = float3(0.25,0.25,0.25) * r7.xyz;
    r3.w = 1;
    r4.y = 1;
    r3.xyz = r1.xyz;
    r5.xy = r0.zw;
    r1.w = 2;
    r2.w = 2;
    r4.z = 0;
    while (true) {
      r4.w = cmp((int)r4.z >= 4);
      if (r4.w != 0) break;
      r6.xyz = r2.xyz * r2.www;
      r7.xyz = sceneTime_g * r3.xyz;
      r4.w = dot(r7.xyz, float3(12.9898005,78.2330017,56.7869987));
      r4.w = sin(r4.w);
      r4.w = 43758.5469 * r4.w;
      r4.w = frac(r4.w);
      r4.w = r4.w * 0.200000048 + 0.899999976;
      r3.xyz = r6.xyz * r4.www + r3.xyz;
      r6.x = dot(r3.xyzw, proj_g._m00_m10_m20_m30);
      r6.y = dot(r3.xyzw, proj_g._m01_m11_m21_m31);
      r4.w = dot(r3.xyzw, proj_g._m03_m13_m23_m33);
      r6.xy = r6.xy / r4.ww;
      r5.xz = r6.xy * float2(0.5,0.5) + float2(0.5,0.5);
      r1.w = 0.5 * r1.w;
      r5.y = 1 + -r5.z;
      r5.zw = resolutionScaling_g.xy * r5.xy;
      r4.x = depthTexture.SampleLevel(samPoint_s, r5.zw, 0).x;
      r4.w = dot(projInv_g._m22_m32, r4.xy);
      r4.x = dot(projInv_g._m23_m33, r4.xy);
      r4.x = r4.w / r4.x;
      r4.x = r4.x + -r3.z;
      r4.w = cmp(0 < r4.x);
      r4.x = cmp(r4.x < 0);
      r4.x = r4.x ? r4.w : 0;
      r2.w = r4.x ? -r1.w : r1.w;
      r4.z = (int)r4.z + 1;
    }
    r0.zw = r5.xy;
    r1.xy = float2(-0.5,-0.5) + r0.zw;
    r1.x = dot(r1.xy, r1.xy);
    r1.x = sqrt(r1.x);
    r1.x = r1.x + r1.x;
    r1.y = r1.x * r1.x;
    r1.x = -r1.x * r1.y + 1;
    r1.yz = resolutionScaling_g.xy * r0.zw;
    r0.xy = r1.yz * r0.xy;
    r2.xy = (int2)r0.xy;
    r2.zw = float2(0,0);
    r0.x = mrtTexture0.Load(r2.xyz).w;
    r0.x = (int)r0.x & 2;
    r0.x = r0.x ? 0 : r1.x;
  } else {
    r1.xy = float2(-0.5,-0.5) + r0.zw;
    r0.y = dot(r1.xy, r1.xy);
    r0.y = sqrt(r0.y);
    r0.y = r0.y + r0.y;
    r1.x = r0.y * r0.y;
    r0.x = -r0.y * r1.x + 1;
  }
  r0.yz = resolutionScaling_g.xy * r0.zw;
  r0.yz = min(uvClamp_g.xy, r0.yz);
  r0.yzw = colorTexture.SampleLevel(samPoint_s, r0.yz, 0).xyz;
  o0.w = max(0, r0.x);
  o0.xyz = r0.yzw;
  return;
}