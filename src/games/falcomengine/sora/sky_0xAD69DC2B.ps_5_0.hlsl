// ---- Created with 3Dmigoto v1.3.16 on Sun Aug 24 18:02:51 2025
#include "../common.hlsl"
struct InstanceParam
{
    float4x4 world;                // Offset:    0
    float4 color;                  // Offset:   64
    float4 uv;                     // Offset:   80
    float4 param;                  // Offset:   96
    uint boneAddress;              // Offset:  112
    float3 param2;                 // Offset:  116
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
  float4x4 prevViewProj_g : packoffset(c6);
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
  float2 dudvScrollSpeed_g : packoffset(c3);
  float dudvScale_g : packoffset(c3.z);
  float _pad3 : packoffset(c3.w);
  float depthFadeWidth_g : packoffset(c4);
  float shadowCastOffset_g : packoffset(c4.y);
  float volumeFogInvalidity_g : packoffset(c4.z);
}

SamplerState Smpl0_s : register(s0);
SamplerState Smpl10_s : register(s10);
SamplerState SmplMirror_s : register(s12);
SamplerState SmplLinearClamp_s : register(s15);
Texture2D<float4> Tex0 : register(t0);
Texture2D<float4> Tex10 : register(t10);
StructuredBuffer<InstanceParam> instances_g : register(t15);
Texture2D<float4> texRefractionDepth : register(t23);
Texture3D<float4> volumeFogTexture_g : register(t26);


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_Position0,
  float4 v1 : NORMAL0,
  float4 v2 : TEXCOORD0,
  float4 v3 : TEXCOORD1,
  nointerpolation uint v4 : TEXCOORD6,
  uint v5 : SV_IsFrontFace0,
  out float4 o0 : SV_Target0,
  out uint4 o1 : SV_Target1,
  out uint4 o2 : SV_Target2,
  out uint2 o3 : SV_Target3)
{
  float4 r0,r1,r2,r3,r4,r5;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.x = dot(v2.xyz, clipPlane_g.xyz);
  r0.x = -clipPlane_g.w + r0.x;
  r0.x = cmp(r0.x < 0);
  if (r0.x != 0) discard;
  r0.xy = dudvScrollSpeed_g.xy * sceneTime_g + v3.xy;
  r0.z = 1 + -r0.y;
  r0.xy = Tex10.Sample(Smpl10_s, r0.xz).xy;
  r0.zw = int2(1024,1) & swizzle_flags_g;
  r0.xy = r0.zz ? r0.xx : r0.xy;
  r0.xy = r0.xy * float2(2,2) + float2(-1,-1);
  r0.xy = r0.xy * dudvScale_g + v3.xy;
  r0.z = 1 + -r0.y;
  r1.xyzw = Tex0.Sample(Smpl0_s, r0.xz).xyzw;
  // r1.rgb = srgbDecode(r1.rgb);
  r2.x = r1.x;
  r2.w = 1;
  r0.xyzw = r0.wwww ? r2.xxxw : r1.xyzw;
  r1.x = instances_g[v4.x].color.x;
  r1.y = instances_g[v4.x].color.y;
  r1.z = instances_g[v4.x].color.z;
  r1.w = instances_g[v4.x].color.w;
  r1.w = opacity_g * r1.w;
  r0.xyzw = r1.xyzw * r0.xyzw;
  r1.x = max(1, emissive_g);
  r1.yzw = r1.xxx * r0.xyz;
  r0.xyz = -r0.xyz * r1.xxx + fogColor_g.xyz;
  r1.x = -fogHeight_g + v2.y;
  r1.x = saturate(fogHeightRangeInv_g * r1.x);
  r2.xyz = v2.xyz;
  r2.w = 1;
  r3.x = dot(r2.xyzw, view_g._m02_m12_m22_m32);
  r3.y = -fogNearDistance_g + -r3.x;
  r3.y = saturate(fogFadeRangeInv_g * r3.y);
  r1.x = r3.y * r1.x;
  r3.y = fogIntensity_g * r1.x;
  r1.x = -r1.x * fogIntensity_g + 1;
  r1.x = ssaoIntensity_g * r1.x;
  r3.y = materialFogIntensity_g * r3.y;
  r0.xyz = r3.yyy * r0.xyz + r1.yzw;
  r1.yz = invVPSize_g.xy * v0.xy;
  r4.xy = resolutionScaling_g.xy * r1.yz;
  r1.y = -volumeNearOverFarClip_g + 1;
  r1.z = -r3.x / volumeCameraFarClip_g;
  r1.z = r1.z * volumeCameraFarOverMaxFar_g + -volumeNearOverFarClip_g;
  r4.z = r1.z / r1.y;
  r5.xyzw = volumeFogTexture_g.SampleLevel(SmplLinearClamp_s, r4.xyz, 0).xyzw;
  // r5.rgb = srgbDecode(r5.rgb);
  r1.y = texRefractionDepth.SampleLevel(SmplMirror_s, r4.xy, 0).x;
  r3.yzw = r0.xyz * r5.www + r5.xyz;
  r3.yzw = r3.yzw + -r0.xyz;
  r3.yzw = combineAlpha_g * r3.yzw + r0.xyz;
  r0.xyz = -r3.yzw + r0.xyz;
  r0.xyz = volumeFogInvalidity_g * r0.xyz + r3.yzw;
  o0.xyz = mapColor_g.xyz * r0.xyz;
  // o0.rgb = srgbEncode(o0.rgb);
  r0.x = viewInv_g._m30 + -v2.x;
  r0.y = viewInv_g._m31 + -v2.y;
  r0.z = viewInv_g._m32 + -v2.z;
  r0.x = dot(r0.xyz, r0.xyz);
  r0.x = sqrt(r0.x);
  r0.y = instances_g[v4.x].param.x;
  r0.z = instances_g[v4.x].param.y;
  r0.x = r0.x + -r0.y;
  r0.x = r0.x * r0.z;
  r0.x = min(1, r0.x);
  r0.x = max(disableMapObjNearFade_g, r0.x);
  r0.x = r0.w * r0.x;
  r1.z = 1;
  r0.y = dot(projInv_g._m22_m32, r1.yz);
  r0.z = dot(projInv_g._m23_m33, r1.yz);
  r0.y = r0.y / r0.z;
  r0.y = -r0.y + r3.x;
  r0.y = saturate(r0.y / depthFadeWidth_g);
  r0.x = r0.x * r0.y;
  o0.w = r0.x;
  r0.x = saturate(r1.x * r0.x);
  r0.x = 30.9990005 * r0.x;
  r0.y = dot(r2.xyzw, view_g._m00_m10_m20_m30);
  r0.z = dot(r2.xyzw, view_g._m01_m11_m21_m31);
  r1.zw = ddy_coarse(r0.yz);
  r2.yw = ddx_coarse(r0.zy);
  r1.y = ddy_coarse(r3.x);
  r2.z = ddx_coarse(r3.x);
  r0.yzw = r2.yzw * r1.yzw;
  r0.yzw = r1.wyz * r2.zwy + -r0.yzw;
  r1.x = dot(r0.yzw, r0.yzw);
  r1.x = rsqrt(r1.x);
  r1.yzw = r1.xxx * r0.yzw;
  r0.y = cmp(abs(r1.y) < abs(r1.z));
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
  r0.y = r0.y ? r2.x : 0;
  r0.y = r0.z * r0.w + r0.y;
  r0.z = cmp(r1.y < -r1.y);
  r0.z = r0.z ? -3.141593 : 0;
  r0.y = r0.y + r0.z;
  r0.z = min(r1.z, r1.y);
  r0.z = cmp(r0.z < -r0.z);
  r0.w = max(r1.z, r1.y);
  r0.w = cmp(r0.w >= -r0.w);
  r0.z = r0.w ? r0.z : 0;
  r0.y = r0.z ? -r0.y : r0.y;
  r1.x = 0.318309873 * r0.y;
  r0.yz = float2(1,1) + r1.xw;
  r0.yz = float2(127.5,127.5) * r0.yz;
  r0.xyz = (uint3)r0.xyz;
  r0.yz = min(uint2(0,0), (uint2)r0.yz);
  o1.z = mad((int)r0.z, 256, (int)r0.y);
  r0.y = dot(v1.xyz, v1.xyz);
  r0.y = rsqrt(r0.y);
  r1.yzw = v1.xyz * r0.yyy;
  r0.y = max(abs(r1.z), abs(r1.y));
  r0.y = 1 / r0.y;
  r0.z = min(abs(r1.z), abs(r1.y));
  r0.y = r0.z * r0.y;
  r0.z = r0.y * r0.y;
  r0.w = r0.z * 0.0208350997 + -0.0851330012;
  r0.w = r0.z * r0.w + 0.180141002;
  r0.w = r0.z * r0.w + -0.330299497;
  r0.z = r0.z * r0.w + 0.999866009;
  r0.w = r0.y * r0.z;
  r0.w = r0.w * -2 + 1.57079637;
  r2.x = cmp(abs(r1.y) < abs(r1.z));
  r0.w = r2.x ? r0.w : 0;
  r0.y = r0.y * r0.z + r0.w;
  r0.z = cmp(r1.y < -r1.y);
  r0.z = r0.z ? -3.141593 : 0;
  r0.y = r0.y + r0.z;
  r0.z = min(r1.z, r1.y);
  r0.z = cmp(r0.z < -r0.z);
  r0.w = max(r1.z, r1.y);
  r0.w = cmp(r0.w >= -r0.w);
  r0.z = r0.w ? r0.z : 0;
  r0.y = r0.z ? -r0.y : r0.y;
  r1.x = 0.318309873 * r0.y;
  r0.yz = float2(1,1) + r1.xw;
  r0.yz = float2(32767.5,32767.5) * r0.yz;
  r0.yz = (uint2)r0.yz;
  o1.xy = min(uint2(0,0), (uint2)r0.yz);
  o1.w = 0;
  o2.xyzw = float4(0,0,0,0);
  r0.y = mad((int)r0.x, 32, (int)r0.x);
  o3.x = mad((int)r0.x, 1024, (int)r0.y);
  o3.y = 0;
  return;
}