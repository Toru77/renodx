// ---- Created with 3Dmigoto v1.4.1 on Thu Aug 20 07:37:47 2026

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
  float fresnel0_g : packoffset(c6.z);
  float specularGlossiness0_g : packoffset(c6.w);
  float shakeScale_g : packoffset(c7);
  float shakeSpeed_g : packoffset(c7.y);
  float shakeFlexibility_g : packoffset(c7.z);
  float shakeFreq_g : packoffset(c7.w);
  float shakeWindScale_g : packoffset(c8);
  float shadowCastOffset_g : packoffset(c8.y);
  float volumeFogInvalidity_g : packoffset(c8.z);
}

SamplerState Smpl0_s : register(s0);
Texture2D<float4> Tex0 : register(t0);


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_Position0,
  float4 v1 : NORMAL0,
  float4 v2 : TEXCOORD0,
  float4 v3 : TEXCOORD1,
  float4 v4 : TEXCOORD4,
  uint4 v5 : TEXCOORD6,
  float4 v6 : TEXCOORD7,
  float4 v7 : TEXCOORD8,
  uint v8 : SV_IsFrontFace0,
  out float4 o0 : SV_Target0,
  out uint4 o1 : SV_Target1,
  out uint4 o2 : SV_Target2,
  out uint2 o3 : SV_Target3,
  out float2 o4 : SV_Target4)
{
  float4 r0,r1,r2;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.x = 1 & swizzle_flags_g;
  r0.yz = v3.xy * float2(1,-1) + float2(0,1);
  r1.xyzw = Tex0.Sample(Smpl0_s, r0.yz).xyzw;
  r2.x = r1.x;
  r2.w = 1;
  r0.xyzw = r0.xxxx ? r2.xxxw : r1.xyzw;
  o0.xyzw = v4.xyzw * r0.xyzw;
  r0.xyz = v2.xyz;
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
  o1.z = mad((int)r0.y, 256, (int)r0.x);
  r0.x = dot(v1.xyz, v1.xyz);
  r0.x = max(0.00100000005, r0.x);
  r0.x = rsqrt(r0.x);
  r0.yzw = v1.xyz * r0.xxx;
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
  r1.z = max(r0.z, r0.y);
  r1.z = cmp(r1.z >= -r1.z);
  r1.y = r1.z ? r1.y : 0;
  r1.x = r1.y ? -r1.x : r1.x;
  r0.x = 0.318309873 * r1.x;
  r1.xy = float2(1,1) + r0.xw;
  r0.x = dot(r0.yzw, -lightDirection_g.xyz);
  r0.yz = float2(32767.5,32767.5) * r1.xy;
  r0.yz = (uint2)r0.yz;
  o1.xy = min(uint2(65535,65535), (uint2)r0.yz);
  o1.w = 40;
  r0.y = v8.x ? -1 : 1;
  r0.x = r0.x * r0.y;
  r0.xy = r0.xx * float2(0.5,-0.5) + float2(0.5,0.5);
  r0.y = max(r0.x, r0.y);
  r0.y = r0.y + -r0.x;
  r0.w = saturate(translucency_g * r0.y + r0.x);
  r0.xyz = float3(255,255,255);
  r0.xyzw = float4(1,1,1,255) * r0.xyzw;
  o2.xyzw = (uint4)r0.xyzw;
  r0.x = saturate(ssaoIntensity_g);
  r0.x = 30.9990005 * r0.x;
  r0.x = (uint)r0.x;
  o3.y = (uint)r0.x << 10;
  o3.x = materialID_g;
  r0.xy = v6.xy / v6.ww;
  r0.xy = r0.xy * float2(0.5,-0.5) + float2(0.5,0.5);
  r0.xy = vpSize_g.xy * r0.xy;
  r0.zw = v7.xy / v7.ww;
  r0.zw = r0.zw * float2(0.5,-0.5) + float2(0.5,0.5);
  r0.xy = r0.zw * vpSize_g.xy + -r0.xy;
  o4.xy = jitterDiff_g.xy + r0.xy;
  return;
}