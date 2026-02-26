// ---- Created with 3Dmigoto v1.4.1 on Thu Feb 19 19:46:46 2026

struct LightParam
{
    float3 pos;                    // Offset:    0
    float radius;                  // Offset:   12
    float3 color;                  // Offset:   16
    float radiusInv;               // Offset:   28
    float3 charaColor;             // Offset:   32
    float attenuation;             // Offset:   44
    float3 vec;                    // Offset:   48
    float spotAngleInv;            // Offset:   60
    float attenuationAngle;        // Offset:   64
    float specularIntensity;       // Offset:   68
    float specularGlossiness;      // Offset:   72
    float scatterAnisotropy;       // Offset:   76
    float3 scatterColor;           // Offset:   80
    float scatterDensity;          // Offset:   92
    float translucency;            // Offset:   96
    int shadowmapIndex;            // Offset:  100
    float userParams[2];           // Offset:  104
};

struct LightIndexData
{
    int pointLightIndices[63];     // Offset:    0
    uint pointLightCount;          // Offset:  252
    int spotLightIndices[63];      // Offset:  256
    uint spotLightCount;           // Offset:  508
    int lightProbeIndices[14];     // Offset:  512
    uint lightProbeCount;          // Offset:  568
    float tileDepthInv;            // Offset:  572
};

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

cbuffer cb_post_sky : register(b6)
{
  float3 incomingLight_g : packoffset(c0);
  uint isEnableSky_g : packoffset(c0.w);
  float3 scatteringR_g : packoffset(c1);
  float3 scatteringM_g : packoffset(c2);
  float3 extinctionR_g : packoffset(c3);
  float3 extinctionM_g : packoffset(c4);
  float3 densityScaleHeight_g : packoffset(c5);
  float skyHorizonBottomLimit_g : packoffset(c5.w);
  float3 sunDirection_g : packoffset(c6);
  float skyHorizonTopLimit_g : packoffset(c6.w);
  float mieG_g : packoffset(c7);
  float distanceScale_g : packoffset(c7.y);
  float planetRadius_g : packoffset(c7.z);
  float atmosphereHeight_g : packoffset(c7.w);
  float sunIntensity_g : packoffset(c8);
  float skyLutNearOverFarClip_g : packoffset(c8.y);
  float skyLutCameraFarClip_g : packoffset(c8.z);
  float skyBrightness_g : packoffset(c8.w);
  float cloudCoverage_g : packoffset(c9);
  float cloudThickness_g : packoffset(c9.y);
  uint cloudRaySteps_g : packoffset(c9.z);
  float cloudLightIntensity_g : packoffset(c9.w);
  float cloudDistance_g : packoffset(c10);
  float cloudFadeRangeInv_g : packoffset(c10.y);
  float cloudHeight_g : packoffset(c10.z);
  float cloudScale_g : packoffset(c10.w);
  float3 cloudColor_g : packoffset(c11);
}

cbuffer cb_volume_fog : register(b7)
{
  float volumeIntensity_g : packoffset(c0);
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
  float densityScale_g : packoffset(c5.y);
}

cbuffer cb_local : register(b5)
{
  float2 uvScroll0_g : packoffset(c0);
  float2 uvScroll1_g : packoffset(c0.z);
  float2 uvScroll2_g : packoffset(c1);
  float emissive_g : packoffset(c1.z);
  float materialFogIntensity_g : packoffset(c1.w);
  float3 diffuseMapColor0_g : packoffset(c2);
  float opacity_g : packoffset(c2.w);
  float translucency_g : packoffset(c3);
  float ssaoIntensity_g : packoffset(c3.y);
  float3 shadowColor_g : packoffset(c4);
  float glowShadowFadeRatio_g : packoffset(c4.w);
  float3 rimLightColor_g : packoffset(c5);
  float rimLightPower_g : packoffset(c5.w);
  float3 specularColor_g : packoffset(c6);
  float specularShadowFadeRatio_g : packoffset(c6.w);
  float rimIntensity_g : packoffset(c7);
  float dynamicLightIntensity_g : packoffset(c7.y);
  float fresnel0_g : packoffset(c7.z);
  float specularGlossiness0_g : packoffset(c7.w);
  float metalness_g : packoffset(c8);
  float roughness_g : packoffset(c8.y);
  float shadowCastOffset_g : packoffset(c8.z);
  float volumeFogInvalidity_g : packoffset(c8.w);
  uint materialID_g : packoffset(c9);
}

SamplerState Smpl0_s : register(s0);
SamplerState Smpl6_s : register(s6);
SamplerState SmplCube_s : register(s14);
SamplerState SmplLinearClamp_s : register(s15);
SamplerComparisonState SmplShadow_s : register(s13);
Texture2D<float4> Tex0 : register(t0);
Texture2D<float4> Tex6 : register(t6);
StructuredBuffer<LightParam> dynamicLights_g : register(t11);
StructuredBuffer<LightIndexData> lightIndices_g : register(t12);
StructuredBuffer<float4x4> spotShadowMatrices_g : register(t14);
StructuredBuffer<InstanceParam> instances_g : register(t15);
TextureCube<float4> texEnvMap_g : register(t17);
Texture2DArray<float4> spotShadowMaps : register(t18);
Texture3D<float4> atmosphereInscatterLUT : register(t19);
Texture3D<float4> atmosphereExtinctionLUT : register(t20);
Texture3D<float4> volumeFogTexture_g : register(t26);

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
  float4 r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12,r13,r14,r15;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.xyz = v3.xyz;
  r0.w = 1;
  r1.x = dot(r0.xyzw, view_g._m00_m10_m20_m30);
  r1.y = dot(r0.xyzw, view_g._m01_m11_m21_m31);
  r0.x = dot(r0.xyzw, view_g._m02_m12_m22_m32);
  r0.zw = ddy_coarse(r1.xy);
  r0.y = ddy_coarse(r0.x);
  r2.yw = ddx_coarse(r1.yx);
  r2.z = ddx_coarse(r0.x);
  r1.xyz = r2.yzw * r0.yzw;
  r0.xyz = r0.wyz * r2.zwy + -r1.xyz;
  r0.w = dot(r0.xyz, r0.xyz);
  r0.w = rsqrt(r0.w);
  r0.yzw = r0.xyz * r0.www;
  r1.x = instances_g[v2.x].color.x;
  r1.y = instances_g[v2.x].color.y;
  r1.z = instances_g[v2.x].color.z;
  r1.w = instances_g[v2.x].color.w;
  r1.w = opacity_g * r1.w;
  r2.xy = v5.xy / v5.ww;
  r2.xy = r2.xy * float2(0.5,-0.5) + float2(0.5,0.5);
  r2.xy = r2.xy * vpSize_g.xy + -v0.xy;
  o3.xy = -motionJitterOffset_g.xy + r2.xy;
  r2.x = viewInv_g._m30 + -v3.x;
  r2.y = viewInv_g._m31 + -v3.y;
  r2.z = viewInv_g._m32 + -v3.z;
  r2.w = dot(r2.xyz, r2.xyz);
  r3.x = sqrt(r2.w);
  r2.w = rsqrt(r2.w);
  r3.yzw = r2.xyz * r2.www;
  r4.x = instances_g[v2.x].param.x;
  r4.y = instances_g[v2.x].param.y;
  r3.x = -r4.x + r3.x;
  r3.x = r3.x * r4.y;
  r3.x = min(1, r3.x);
  r3.x = max(disableMapObjNearFade_g, r3.x);
  r4.x = dot(v1.xyz, v1.xyz);
  r4.x = rsqrt(r4.x);
  r4.yzw = v1.xyz * r4.xxx;
  r5.xy = v4.xy * float2(1,-1) + float2(0,1);
  r6.xyzw = Tex0.Sample(Smpl0_s, r5.xy).xyzw;
  r5.zw = int2(1,64) & swizzle_flags_g;
  r7.x = r6.x;
  r7.w = 1;
  r6.xyzw = r5.zzzz ? r7.xxxw : r6.xyzw;
  r6.xyz = diffuseMapColor0_g.xyz * r6.xyz;
  r7.xyzw = Tex6.Sample(Smpl6_s, r5.xy).xyzw;
  r8.x = r7.x;
  r8.w = 1;
  r5.xyzw = r5.wwww ? r8.xxxw : r7.xyzw;
  r1.xyzw = r6.xyzw * r1.xyzw;
  r5.w = emissive_g * r5.w;
  r6.x = dot(view_g._m02_m12_m22_m32, v3.xyzw);
  r6.yzw = lightProbe_g[1].xyz * r4.yyy + lightProbe_g[0].xyz;
  r6.yzw = lightProbe_g[2].xyz * r4.zzz + r6.yzw;
  r6.yzw = lightProbe_g[3].xyz * r4.www + r6.yzw;
  r7.xyz = lightProbe_g[4].xyz * r4.www;
  r6.yzw = r7.xyz * r4.yyy + r6.yzw;
  r7.xyz = lightProbe_g[5].xyz * r4.zzz;
  r6.yzw = r7.xyz * r4.www + r6.yzw;
  r7.xyz = lightProbe_g[6].xyz * r4.zzz;
  r6.yzw = r7.xyz * r4.yyy + r6.yzw;
  r7.xy = r4.wz * r4.wz;
  r7.x = r7.x * 3 + -1;
  r6.yzw = lightProbe_g[7].xyz * r7.xxx + r6.yzw;
  r7.x = r4.y * r4.y + -r7.y;
  r6.yzw = lightProbe_g[8].xyz * r7.xxx + r6.yzw;
  r7.xyz = sceneShadowColor_g.xyz + shadowColor_g.xyz;
  r7.xyz = min(float3(1,1,1), r7.xyz);
  r7.w = dot(r4.yzw, -lightDirection_g.xyz);
  r8.x = dot(r4.yzw, r3.yzw);
  r8.yzw = r2.xyz * r2.www + -lightDirection_g.xyz;
  r9.x = dot(r8.yzw, r8.yzw);
  r9.x = rsqrt(r9.x);
  r8.yzw = r9.xxx * r8.yzw;
  r9.x = lightSpecularGlossiness_g * specularGlossiness0_g;
  r8.y = saturate(dot(r8.yzw, r4.yzw));
  r8.z = max(0.00100000005, r9.x);
  r8.y = log2(r8.y);
  r8.y = r8.z * r8.y;
  r8.y = exp2(r8.y);
  r8.y = lightSpecularIntensity_g * r8.y;
  r8.yzw = specularColor_g.xyz * r8.yyy;
  r9.xy = r7.ww * float2(0.5,-0.5) + float2(0.5,0.5);
  r7.w = max(r9.x, r9.y);
  r7.w = r7.w + -r9.x;
  r7.w = translucency_g * r7.w + r9.x;
  r9.xy = metalness_g * r5.xy;
  r5.x = r8.x + r8.x;
  r3.yzw = r4.yzw * -r5.xxx + r3.yzw;

  // -- FIXED GetDimensions & APPLIED CUBEMAP MODS -- 
  texEnvMap_g.GetDimensions(0, uiDest.x, uiDest.y, uiDest.w);
  r5.x = uiDest.w;
  r3.yzw = float3(1,-1,-1) * r3.yzw;
  r5.x = (int)r5.x + -1;
  r5.x = (uint)r5.x;
  r5.x = r9.y * r5.x;
  
  // Modify glass cubemap
  r5.x += 0.0; // Adds X mip levels of blur
  r3.yzw = texEnvMap_g.SampleLevel(SmplCube_s, r3.yzw, r5.x).xyz;
  r3.yzw *= lerp(1.0, 0.25, saturate(sss_injection_data.cubemap_improvements_enabled)); // Reduces the reflection intensity by X
  r5.x = cmp(0 < fresnel0_g);
  r8.x = 1 + -abs(r8.x);
  r8.x = max(0, r8.x);
  r9.z = log2(r8.x);
  r9.z = fresnel0_g * r9.z;
  r9.z = exp2(r9.z);
  r5.x = r5.x ? r9.z : 1;
  r9.z = r9.x * r5.x;
  r10.xyz = r1.xyz * r3.yzw + -r1.xyz;
  r1.xyz = r9.zzz * r10.xyz + r1.xyz;
  r9.z = dot(r3.yzw, float3(0.298999995,0.587000012,0.114));
  r9.y = r9.y * -9 + 10;
  r9.z = log2(r9.z);
  r9.y = r9.y * r9.z;
  r9.y = exp2(r9.y);
  r9.z = 1 + -r9.y;
  r9.x = r9.x * r9.z + r9.y;
  r3.yzw = r9.xxx * r3.yzw;
  r3.yzw = r3.yzw * r5.xxx;
  r5.y = -r5.y * roughness_g + 1;
  r3.yzw = r5.yyy * r3.yzw;
  r3.yzw = r8.yzw * lightColor_g.xyz + r3.yzw;
  r8.yzw = float3(1,1,1) + -r7.xyz;
  r7.xyz = r7.www * r8.yzw + r7.xyz;
  r6.yzw = r7.xyz * lightColor_g.xyz + r6.yzw;
  r5.y = min(1, r5.w);
  r7.xyz = float3(1,1,1) + -r6.yzw;
  r6.yzw = r5.yyy * r7.xyz + r6.yzw;
  r5.y = rimIntensity_g * r8.x;
  r5.y = log2(r5.y);
  r5.y = rimLightPower_g * r5.y;
  r5.y = exp2(r5.y);
  r5.y = min(1, r5.y);
  r3.yzw = rimLightColor_g.xyz * r5.yyy + r3.yzw;
  r7.xy = lightTileSizeInv_g.xy * v0.xy;
  r7.xy = (uint2)r7.xy;
  r5.y = (uint)r7.y << 5;
  r7.y = (int)r7.x + (int)r5.y;
  r7.y = lightIndices_g[r7.y].tileDepthInv;
  r7.y = r7.y * -r6.x;
  r7.y = min(7, r7.y);
  r7.y = max(0, r7.y);
  r7.y = (uint)r7.y;
  r5.y = mad((int)r7.y, 576, (int)r5.y);
  r5.y = (int)r7.x + (int)r5.y;
  r5.y = min(4607, (uint)r5.y);
  r7.x = lightIndices_g[r5.y].pointLightCount;
  r7.x = min(63, (uint)r7.x);
  r7.yzw = float3(0,0,0);
  r8.xyzw = float4(0,0,0,0);
  while (true) {
    r9.x = cmp((uint)r8.w >= (uint)r7.x);
    if (r9.x != 0) break;
    
    // -- FIXED DYNAMIC STRUCTURED BUFFER READ (POINT LIGHTS) --
    r9.x = lightIndices_g[r5.y].pointLightIndices[r8.w];
    
    r9.y = dynamicLights_g[r9.x].pos.x;
    r9.z = dynamicLights_g[r9.x].pos.y;
    r9.w = dynamicLights_g[r9.x].pos.z;
    r9.yzw = -v3.xyz + r9.yzw;
    r10.x = dot(r9.yzw, r9.yzw);
    r10.y = sqrt(r10.x);
    r10.z = dynamicLights_g[r9.x].radiusInv;
    r10.y = r10.y * r10.z;
    r10.z = dynamicLights_g[r9.x].attenuation;
    r10.y = log2(abs(r10.y));
    r10.y = r10.z * r10.y;
    r10.y = exp2(r10.y);
    r10.y = 1 + -r10.y;
    r10.y = max(0, r10.y);
    r10.z = cmp(0 < r10.y);
    if (r10.z != 0) {
      r10.x = rsqrt(r10.x);
      r9.yzw = r10.xxx * r9.yzw;
      r10.x = dynamicLights_g[r9.x].translucency;
      r10.z = dot(r9.yzw, r4.yzw);
      r10.x = max(r10.x, r10.z);
      r10.x = r10.y * r10.x;
      r10.y = dynamicLights_g[r9.x].color.x;
      r10.z = dynamicLights_g[r9.x].color.y;
      r10.w = dynamicLights_g[r9.x].color.z;
      r8.xyz = r10.yzw * r10.xxx + r8.xyz;
      r9.yzw = r2.xyz * r2.www + r9.yzw;
      r11.x = dot(r9.yzw, r9.yzw);
      r11.x = rsqrt(r11.x);
      r9.yzw = r11.xxx * r9.yzw;
      r11.x = dynamicLights_g[r9.x].specularIntensity;
      r11.y = dynamicLights_g[r9.x].specularGlossiness;
      r9.x = specularGlossiness0_g * r11.y;
      r9.y = saturate(dot(r9.yzw, r4.yzw));
      r9.x = max(0.00100000005, r9.x);
      r9.y = log2(r9.y);
      r9.x = r9.x * r9.y;
      r9.x = exp2(r9.x);
      r9.xyz = r10.yzw * r9.xxx;
      r9.xyz = r9.xyz * r10.xxx;
      r7.yzw = r9.xyz * r11.xxx + r7.yzw;
    }
    r8.w = (int)r8.w + 1;
  }
  r6.yzw = r8.xyz * dynamicLightIntensity_g + r6.yzw;
  r7.x = lightIndices_g[r5.y].spotLightCount;
  r7.x = min(63, (uint)r7.x);
  r8.xyz = r7.yzw;
  r9.xyz = float3(0,0,0);
  r8.w = 0;
  while (true) {
    r9.w = cmp((uint)r8.w >= (uint)r7.x);
    if (r9.w != 0) break;
    
    // -- FIXED DYNAMIC STRUCTURED BUFFER READ (SPOT LIGHTS) --
    r9.w = lightIndices_g[r5.y].spotLightIndices[r8.w];
    
    r10.x = dynamicLights_g[r9.w].pos.x;
    r10.y = dynamicLights_g[r9.w].pos.y;
    r10.z = dynamicLights_g[r9.w].pos.z;
    r10.xyz = -v3.xyz + r10.xyz;
    r10.w = dot(r10.xyz, r10.xyz);
    r11.x = rsqrt(r10.w);
    r10.xyz = r11.xxx * r10.xyz;
    r11.x = dynamicLights_g[r9.w].vec.x;
    r11.y = dynamicLights_g[r9.w].vec.y;
    r11.z = dynamicLights_g[r9.w].vec.z;
    r11.w = dynamicLights_g[r9.w].spotAngleInv;
    r11.x = dot(r10.xyz, r11.xyz);
    r11.x = max(0, r11.x);
    r11.x = 1 + -r11.x;
    r11.x = r11.x * r11.w;
    r11.y = dynamicLights_g[r9.w].attenuationAngle;
    r11.x = log2(r11.x);
    r11.x = r11.y * r11.x;
    r11.x = exp2(r11.x);
    r11.x = 1 + -r11.x;
    r11.x = max(0, r11.x);
    r11.y = cmp(0 < r11.x);
    if (r11.y != 0) {
      r10.w = sqrt(r10.w);
      r11.y = dynamicLights_g[r9.w].radiusInv;
      r10.w = r11.y * r10.w;
      r11.y = dynamicLights_g[r9.w].attenuation;
      r10.w = log2(abs(r10.w));
      r10.w = r11.y * r10.w;
      r10.w = exp2(r10.w);
      r10.w = 1 + -r10.w;
      r10.w = max(0, r10.w);
      r10.w = r11.x * r10.w;
      r11.x = cmp(0 < r10.w);
      if (r11.x != 0) {
        r11.x = dynamicLights_g[r9.w].translucency;
        r11.y = dynamicLights_g[r9.w].shadowmapIndex;
        r11.z = cmp((int)r11.y != -1);
        if (r11.z != 0) {
          r12.xyzw = spotShadowMatrices_g[r11.y]._m00_m10_m20_m30;
          r13.xyzw = spotShadowMatrices_g[r11.y]._m01_m11_m21_m31;
          r14.xyzw = spotShadowMatrices_g[r11.y]._m02_m12_m22_m32;
          r15.xyzw = spotShadowMatrices_g[r11.y]._m03_m13_m23_m33;
          r12.x = dot(v3.xyzw, r12.xyzw);
          r12.y = dot(v3.xyzw, r13.xyzw);
          r12.z = dot(v3.xyzw, r14.xyzw);
          r11.z = dot(v3.xyzw, r15.xyzw);
          r12.xyz = r12.xyz / r11.zzz;
          r12.w = (uint)r11.y;
          r11.y = spotShadowMaps.SampleCmpLevelZero(SmplShadow_s, r12.xyw, r12.z).x;
          r13.xyz = float3(0.00244140625,0,0) + r12.xyw;
          r11.z = spotShadowMaps.SampleCmpLevelZero(SmplShadow_s, r13.xyz, r12.z).x;
          r11.z = 0.200000003 * r11.z;
          r11.y = r11.y * 0.200000003 + r11.z;
          r13.xyz = float3(-0.00244140625,0,0) + r12.xyw;
          r11.z = spotShadowMaps.SampleCmpLevelZero(SmplShadow_s, r13.xyz, r12.z).x;
          r11.y = r11.z * 0.200000003 + r11.y;
          r13.xyz = float3(0,0.00244140625,0) + r12.xyw;
          r11.z = spotShadowMaps.SampleCmpLevelZero(SmplShadow_s, r13.xyz, r12.z).x;
          r11.y = r11.z * 0.200000003 + r11.y;
          r12.xyw = float3(0,-0.00244140625,0) + r12.xyw;
          r11.z = spotShadowMaps.SampleCmpLevelZero(SmplShadow_s, r12.xyw, r12.z).x;
          r11.y = r11.z * 0.200000003 + r11.y;
          r10.w = r11.y * r10.w;
        }
        r11.y = dot(r10.xyz, r4.yzw);
        r11.x = max(r11.x, r11.y);
        r10.w = r11.x * r10.w;
        r11.x = dynamicLights_g[r9.w].color.x;
        r11.y = dynamicLights_g[r9.w].color.y;
        r11.z = dynamicLights_g[r9.w].color.z;
        r9.xyz = r11.xyz * r10.www + r9.xyz;
        r10.xyz = r2.xyz * r2.www + r10.xyz;
        r11.w = dot(r10.xyz, r10.xyz);
        r11.w = rsqrt(r11.w);
        r10.xyz = r11.www * r10.xyz;
        r12.x = dynamicLights_g[r9.w].specularIntensity;
        r12.y = dynamicLights_g[r9.w].specularGlossiness;
        r9.w = specularGlossiness0_g * r12.y;
        r10.x = saturate(dot(r10.xyz, r4.yzw));
        r9.w = max(0.00100000005, r9.w);
        r10.x = log2(r10.x);
        r9.w = r10.x * r9.w;
        r9.w = exp2(r9.w);
        r10.xyz = r11.xyz * r9.www;
        r10.xyz = r10.xyz * r10.www;
        r8.xyz = r10.xyz * r12.xxx + r8.xyz;
      }
    }
    r8.w = (int)r8.w + 1;
  }
  r2.xyz = r9.xyz * dynamicLightIntensity_g + r6.yzw;
  r3.yzw = r8.xyz * dynamicLightIntensity_g + r3.yzw;
  r6.yzw = r3.yzw * r5.zzz;
  r1.xyz = r1.xyz * r2.xyz + r6.yzw;
  r2.x = dot(r3.yzw, float3(0.298999995,0.587000012,0.114));
  r2.x = min(1, r2.x);
  r2.y = r5.x * r1.w;
  r2.z = instances_g[v2.x].color.w;
  r1.w = -r1.w * r5.x + r2.z;
  r1.w = r2.x * r1.w + r2.y;
  r2.x = max(1, r5.w);
  r2.yzw = r2.xxx * r1.xyz;
  r1.w = r1.w * r3.x;
  r3.x = -fogNearDistance_g + -r6.x;
  r3.x = saturate(fogFadeRangeInv_g * r3.x);
  r3.y = -fogHeight_g + v3.y;
  r3.y = saturate(fogHeightRangeInv_g * r3.y);
  r3.x = r3.x * r3.y;
  r3.y = fogIntensity_g * r3.x;
  r3.y = materialFogIntensity_g * r3.y;
  r1.xyz = -r1.xyz * r2.xxx + fogColor_g.xyz;
  r1.xyz = r3.yyy * r1.xyz + r2.yzw;
  r2.x = -r6.x / skyLutCameraFarClip_g;
  r5.xy = invVPSize_g.xy * v0.xy;
  r2.x = -skyLutNearOverFarClip_g + r2.x;
  r2.y = -skyLutNearOverFarClip_g + 1;
  r5.z = r2.x / r2.y;
  r2.xyz = atmosphereInscatterLUT.SampleLevel(SmplLinearClamp_s, r5.xyz, 0).xyz;
  r3.yzw = atmosphereExtinctionLUT.SampleLevel(SmplLinearClamp_s, r5.xyz, 0).xyz;
  r1.xyz = r1.xyz * r3.yzw + r2.xyz;
  r2.x = -r6.x / volumeCameraFarClip_g;
  r5.xy = resolutionScaling_g.xy * r5.xy;
  r2.x = r2.x * volumeCameraFarOverMaxFar_g + -volumeNearOverFarClip_g;
  r2.y = -volumeNearOverFarClip_g + 1;
  r5.z = r2.x / r2.y;
  r2.xyzw = volumeFogTexture_g.SampleLevel(SmplLinearClamp_s, r5.xyz, 0).xyzw;
  r2.xyz = r1.xyz * r2.www + r2.xyz;
  r2.xyz = r2.xyz + -r1.xyz;
  r2.xyz = combineAlpha_g * r2.xyz + r1.xyz;
  r1.xyz = -r2.xyz + r1.xyz;
  r1.xyz = volumeFogInvalidity_g * r1.xyz + r2.xyz;
  o0.xyz = mapColor_g.xyz * r1.xyz;
  r1.x = -r3.x * fogIntensity_g + 1;
  r1.x = ssaoIntensity_g * r1.x;
  r1.y = min(abs(r4.z), abs(r4.y));
  r1.z = max(abs(r4.z), abs(r4.y));
  r1.z = 1 / r1.z;
  r1.xy = r1.xy * r1.wz;
  r1.z = r1.y * r1.y;
  r2.x = r1.z * 0.0208350997 + -0.0851330012;
  r2.x = r1.z * r2.x + 0.180141002;
  r2.x = r1.z * r2.x + -0.330299497;
  r1.z = r1.z * r2.x + 0.999866009;
  r2.x = r1.y * r1.z;
  r2.y = cmp(abs(r4.y) < abs(r4.z));
  r2.x = r2.x * -2 + 1.57079637;
  r2.x = r2.y ? r2.x : 0;
  r1.y = r1.y * r1.z + r2.x;
  r1.z = cmp(r4.y < -r4.y);
  r1.z = r1.z ? -3.141593 : 0;
  r1.y = r1.y + r1.z;
  r1.z = min(r4.z, r4.y);
  r2.x = max(r4.z, r4.y);
  r1.z = cmp(r1.z < -r1.z);
  r2.x = cmp(r2.x >= -r2.x);
  r1.z = r1.z ? r2.x : 0;
  r1.y = r1.z ? -r1.y : r1.y;
  r4.x = 0.318309873 * r1.y;
  r1.yz = float2(1,1) + r4.xw;
  r2.x = min(abs(r0.z), abs(r0.y));
  r2.y = max(abs(r0.z), abs(r0.y));
  r2.y = 1 / r2.y;
  r2.x = r2.x * r2.y;
  r2.y = r2.x * r2.x;
  r2.z = r2.y * 0.0208350997 + -0.0851330012;
  r2.z = r2.y * r2.z + 0.180141002;
  r2.z = r2.y * r2.z + -0.330299497;
  r2.y = r2.y * r2.z + 0.999866009;
  r2.z = r2.x * r2.y;
  r2.w = cmp(abs(r0.y) < abs(r0.z));
  r2.z = r2.z * -2 + 1.57079637;
  r2.z = r2.w ? r2.z : 0;
  r2.x = r2.x * r2.y + r2.z;
  r2.y = cmp(r0.y < -r0.y);
  r2.y = r2.y ? -3.141593 : 0;
  r2.x = r2.x + r2.y;
  r2.y = min(r0.z, r0.y);
  r0.y = max(r0.z, r0.y);
  r0.z = cmp(r2.y < -r2.y);
  r0.y = cmp(r0.y >= -r0.y);
  r0.y = r0.y ? r0.z : 0;
  r0.y = r0.y ? -r2.x : r2.x;
  r0.x = 0.318309873 * r0.y;
  r0.xy = float2(1,1) + r0.xw;
  r0.zw = float2(32767.5,32767.5) * r1.yz;
  r0.zw = (uint2)r0.zw;
  o1.xy = min(uint2(65535,65535), (uint2)r0.zw);
  r0.xy = float2(127.5,127.5) * r0.xy;
  r0.xy = (uint2)r0.xy;
  r0.xy = min(uint2(255,255), (uint2)r0.xy);
  o1.w = mad((int)r0.y, 256, (int)r0.x);
  r0.x = 255 * r1.x;
  r0.x = (uint)r0.x;
  r0.x = min(255, (uint)r0.x);
  o2.y = mad((int)r0.x, 256, (int)r0.x);
  o0.w = r1.w;
  o1.z = 0;
  o2.xw = float2(0,0);
  o2.z = r0.x;
  return;
}
