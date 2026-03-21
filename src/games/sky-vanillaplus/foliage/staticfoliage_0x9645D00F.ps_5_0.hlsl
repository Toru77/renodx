// ---- Created with 3Dmigoto v1.4.1 on Sun Mar 22 00:32:12 2026

struct InstanceParam
{
    float4x4 world;                // Offset:    0
    float4x4 prevWorld;            // Offset:   64
    float4 color;                  // Offset:  128
    float4 uv;                     // Offset:  144
    float4 param;                  // Offset:  160
    uint boneAddress;              // Offset:  176
    float3 param2;                 // Offset:  180
};

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

cbuffer cb_local : register(b5)
{
  float2 uvScroll0_g : packoffset(c0);
  float2 uvScroll1_g : packoffset(c0.z);
  float2 uvScroll2_g : packoffset(c1);
  float emissive_g : packoffset(c1.z);
  float materialFogIntensity_g : packoffset(c1.w);
  float opacity_g : packoffset(c2);
  float translucency_g : packoffset(c2.y);
  float ssaoIntensity_g : packoffset(c2.z);
  uint materialID_g : packoffset(c2.w);
  float3 shadowColor_g : packoffset(c3);
  float glowShadowFadeRatio_g : packoffset(c3.w);
  float3 rimLightColor_g : packoffset(c4);
  float rimLightPower_g : packoffset(c4.w);
  float3 specularColor_g : packoffset(c5);
  float specularShadowFadeRatio_g : packoffset(c5.w);
  float rimIntensity_g : packoffset(c6);
  float dynamicLightIntensity_g : packoffset(c6.y);
  float shadowCastOffset_g : packoffset(c6.z);
  float volumeFogInvalidity_g : packoffset(c6.w);
}

StructuredBuffer<InstanceParam> instances_g : register(t15);


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_Position0,
  float4 v1 : NORMAL0,
  float4 v2 : TEXCOORD0,
  float4 v3 : TEXCOORD4,
  nointerpolation uint4 v4 : TEXCOORD6,
  float4 v5 : TEXCOORD7,
  float4 v6 : TEXCOORD8,
  uint v7 : SV_IsFrontFace0,
  out float4 o0 : SV_Target0,
  out uint4 o1 : SV_Target1,
  out uint4 o2 : SV_Target2,
  out uint2 o3 : SV_Target3,
  out float2 o4 : SV_Target4)
{
  const float4 icb[] = { { 1.000000, 0, 0, 0},
                              { 0, 1.000000, 0, 0},
                              { 0, 0, 1.000000, 0},
                              { 0, 0, 0, 1.000000} };
  float4 r0,r1,r2,r3;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.x = dot(v2.xyz, clipPlane_g.xyz);
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
  r1.x = instances_g[v4.x].param.x;
  r1.y = instances_g[v4.x].param.y;
  r1.z = instances_g[v4.x].param.z;
  r0.z = cmp(0 < r1.z);
  r0.x = r0.z ? r0.y : r0.x;
  r2.x = viewInv_g._m30 + -v2.x;
  r2.y = viewInv_g._m31 + -v2.y;
  r2.z = viewInv_g._m32 + -v2.z;
  r0.y = dot(r2.xyz, r2.xyz);
  r0.y = sqrt(r0.y);
  r0.y = r0.y + -r1.x;
  r0.y = r0.y * r1.y;
  r0.y = min(1, r0.y);
  r0.y = max(disableMapObjNearFade_g, r0.y);
  r0.y = mapColor_g.w * r0.y;
  r0.x = r0.y * v3.w + -r0.x;
  r0.y = v3.w * r0.y;
  r0.x = cmp(r0.x < 0);
  if (r0.x != 0) discard;
  o0.xyzw = v3.xyzw;
  r1.xyz = v2.xyz;
  r1.w = 1;
  r0.x = dot(r1.xyzw, view_g._m00_m10_m20_m30);
  r2.z = ddy_coarse(r0.x);
  r3.w = ddx_coarse(r0.x);
  r0.x = dot(r1.xyzw, view_g._m01_m11_m21_m31);
  r0.z = dot(r1.xyzw, view_g._m02_m12_m22_m32);
  r2.yw = ddy_coarse(r0.zx);
  r3.yz = ddx_coarse(r0.xz);
  r0.xzw = r3.yzw * r2.yzw;
  r0.xzw = r2.wyz * r3.zwy + -r0.xzw;
  r1.x = dot(r0.xzw, r0.xzw);
  r1.x = rsqrt(r1.x);
  r1.yzw = r1.xxx * r0.xzw;
  r0.x = max(abs(r1.z), abs(r1.y));
  r0.x = 1 / r0.x;
  r0.z = min(abs(r1.z), abs(r1.y));
  r0.x = r0.z * r0.x;
  r0.z = r0.x * r0.x;
  r0.w = r0.z * 0.0208350997 + -0.0851330012;
  r0.w = r0.z * r0.w + 0.180141002;
  r0.w = r0.z * r0.w + -0.330299497;
  r0.z = r0.z * r0.w + 0.999866009;
  r0.w = r0.x * r0.z;
  r0.w = r0.w * -2 + 1.57079637;
  r2.x = cmp(abs(r1.y) < abs(r1.z));
  r0.w = r2.x ? r0.w : 0;
  r0.x = r0.x * r0.z + r0.w;
  r0.z = cmp(r1.y < -r1.y);
  r0.z = r0.z ? -3.141593 : 0;
  r0.x = r0.x + r0.z;
  r0.z = min(r1.z, r1.y);
  r0.z = cmp(r0.z < -r0.z);
  r0.w = max(r1.z, r1.y);
  r0.w = cmp(r0.w >= -r0.w);
  r0.z = r0.w ? r0.z : 0;
  r0.x = r0.z ? -r0.x : r0.x;
  r1.x = 0.318309873 * r0.x;
  r0.xz = float2(1,1) + r1.xw;
  r0.xz = float2(127.5,127.5) * r0.xz;
  r0.xz = (uint2)r0.xz;
  r0.xz = min(uint2(255,255), (uint2)r0.xz);
  o1.z = mad((int)r0.z, 256, (int)r0.x);
  r0.x = dot(v1.xyz, v1.xyz);
  r0.x = rsqrt(r0.x);
  r1.yzw = v1.xyz * r0.xxx;
  r0.x = max(abs(r1.z), abs(r1.y));
  r0.x = 1 / r0.x;
  r0.z = min(abs(r1.z), abs(r1.y));
  r0.x = r0.z * r0.x;
  r0.z = r0.x * r0.x;
  r0.w = r0.z * 0.0208350997 + -0.0851330012;
  r0.w = r0.z * r0.w + 0.180141002;
  r0.w = r0.z * r0.w + -0.330299497;
  r0.z = r0.z * r0.w + 0.999866009;
  r0.w = r0.x * r0.z;
  r0.w = r0.w * -2 + 1.57079637;
  r2.x = cmp(abs(r1.y) < abs(r1.z));
  r0.w = r2.x ? r0.w : 0;
  r0.x = r0.x * r0.z + r0.w;
  r0.z = cmp(r1.y < -r1.y);
  r0.z = r0.z ? -3.141593 : 0;
  r0.x = r0.x + r0.z;
  r0.z = min(r1.z, r1.y);
  r0.z = cmp(r0.z < -r0.z);
  r0.w = max(r1.z, r1.y);
  r0.w = cmp(r0.w >= -r0.w);
  r0.z = r0.w ? r0.z : 0;
  r0.x = r0.z ? -r0.x : r0.x;
  r1.x = 0.318309873 * r0.x;
  r0.xz = float2(1,1) + r1.xw;
  r0.w = dot(r1.yzw, -lightDirection_g.xyz);
  r0.xz = float2(32767.5,32767.5) * r0.xz;
  r0.xz = (uint2)r0.xz;
  o1.xy = min(uint2(65535,65535), (uint2)r0.xz);
  r0.x = cmp(r0.y < 0.999989986);
  r0.y = saturate(ssaoIntensity_g * r0.y);
  r0.y = 30.9990005 * r0.y;
  r0.y = (uint)r0.y;
  o3.y = (uint)r0.y << 10;
  o1.w = r0.x ? 44 : 40;
  r0.x = v7.x ? -1 : 1;
  r0.x = r0.w * r0.x;
  r0.xy = r0.xx * float2(0.5,-0.5) + float2(0.5,0.5);
  r0.y = max(r0.x, r0.y);
  r0.y = r0.y + -r0.x;
  r0.w = saturate(translucency_g * r0.y + r0.x);
  r0.xyz = float3(255,255,255);
  r0.xyzw = float4(1,1,1,255) * r0.xyzw;
  o2.xyzw = (uint4)r0.xyzw;
  o3.x = materialID_g;
  r0.xy = v5.xy / v5.ww;
  r0.xy = r0.xy * float2(0.5,-0.5) + float2(0.5,0.5);
  r0.xy = vpSize_g.xy * r0.xy;
  r0.zw = v6.xy / v6.ww;
  r0.zw = r0.zw * float2(0.5,-0.5) + float2(0.5,0.5);
  r0.xy = r0.zw * vpSize_g.xy + -r0.xy;
  o4.xy = jitterDiff_g.xy + r0.xy;
  return;
}