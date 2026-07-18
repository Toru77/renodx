// ---- Created with 3Dmigoto v1.3.16 on Wed May 06 00:36:36 2026
#include "../common.hlsl"
struct SceneParam
{
    float4x4 view_g;               // Offset:    0
    float4x4 view_inv_g;           // Offset:   64
    float4x4 proj_g;               // Offset:  128
    float4x4 proj_inv_g;           // Offset:  192
    float4x4 view_proj_g;          // Offset:  256
    float4x4 view_proj_inv_g;      // Offset:  320
    float4 look_at_g;              // Offset:  384
};

struct InstanceParam
{
    float4x4 world;                // Offset:    0
    float4 color;                  // Offset:   64
    float4 uv;                     // Offset:   80
    float4 param;                  // Offset:   96
    uint boneAddress;              // Offset:  112
    float3 param2;                 // Offset:  116
};

cbuffer cb_ysx_scene : register(b0)
{
  float3 lightColor_g : packoffset(c0);
  float deltaTime_g : packoffset(c0.w);
  float3 lightDirection_g : packoffset(c1);
  uint isManaSensing_g : packoffset(c1.w);
  float4x4 farShadowMtx_g : packoffset(c2);
  float2 invFarShadowSize_g : packoffset(c6);
  float2 invShadowSize_g : packoffset(c6.z);
  float3 sceneShadowColor_g : packoffset(c7);
  float shadowFadeNear_g : packoffset(c7.w);
  float3 chrLightDir_g : packoffset(c8);
  float shadowFadeRangeInv_g : packoffset(c8.w);
  float shadowDistance_g : packoffset(c9);
  float farShadowStartDistance_g : packoffset(c9.y);
  float farShadowEndDistance_g : packoffset(c9.z);
  uint shadowSamplingMode_g : packoffset(c9.w);
  float4x4 ditherMtx_g : packoffset(c10);
  float sceneTime_g : packoffset(c14.y);
  float lightSpecularGlossiness_g : packoffset(c14.z);
  float lightSpecularIntensity_g : packoffset(c14.w);
  float2 resolutionScaling_g : packoffset(c15);
  float disableNearCameraAlpha_g : packoffset(c15.z);
  float sceneDeltaTime_g : packoffset(c15.w);
  float3 twoLayeredFogColorLowerNear_g : packoffset(c16);
  uint twoLayeredFogMode_g : packoffset(c16.w);
  float3 twoLayeredFogColorLowerFar_g : packoffset(c17);
  float twoLayeredFogStartDistance_g : packoffset(c17.w);
  float3 twoLayeredFogColorUpperNear_g : packoffset(c18);
  float twoLayeredFogDistanceRangeInv_g : packoffset(c18.w);
  float3 twoLayeredFogColorUpperFar_g : packoffset(c19);
  float2 twoLayeredFogHeightNear_g : packoffset(c20);
  float2 twoLayeredFogHeightFar_g : packoffset(c20.z);
  float2 twoLayeredFogMinIntensity_g : packoffset(c21);
  float2 twoLayeredFogMaxIntensity_g : packoffset(c21.z);
  float2 twoLayeredFogBlend_g : packoffset(c22);
  float2 twoLayeredFogDistanceCoefInv_g : packoffset(c22.z);
  float3 windDirection_g : packoffset(c23);
  float windWaveTime_g : packoffset(c23.w);
  float windWaveFrequency_g : packoffset(c24);
  float windForce_g : packoffset(c24.y);
  float seaWaveLengthScale_g : packoffset(c24.z);
  float seaWaveHeight_g : packoffset(c24.w);
  float3 seaWaveDirection_g : packoffset(c25);
  float seaWaveSpeed_g : packoffset(c25.w);
  float disableFarCameraAlpha_g : packoffset(c26);
  uint localLightProbeCount_g : packoffset(c26.y);
  float2 invVPSize_g : packoffset(c26.z);
  float4 lightProbe_g[9] : packoffset(c27);
  float3 lightTileSizeInv_g : packoffset(c36);
  float4x4 waterCausticsProj_g : packoffset(c37);
  float2 invResolutionScaling_g : packoffset(c41);
  float2 resolutionUVClamp_g : packoffset(c41.z);
  float3 chara_shadow_mul_color_g : packoffset(c42);
  float4x4 shadow_matrices_g[3] : packoffset(c43);
  float3 shadow_split_distance_g : packoffset(c55);
  float4 char_positions_g[2] : packoffset(c56);
  float4 char_world_positions_g[2] : packoffset(c58);
  uint dbgWaterCausticsOff_g : packoffset(c61.y);
}

cbuffer cb_ysx_scene_slot : register(b6)
{
  uint scene_slot_index_g : packoffset(c0);
}

cbuffer cb_map_material : register(b5)
{
  float2 uvScroll0_g : packoffset(c0);
  float2 uvScroll1_g : packoffset(c0.z);
  float2 uvScroll2_g : packoffset(c1);
  float opacity_g : packoffset(c1.z);
  float translucency_g : packoffset(c1.w);
  float materialFogIntensity_g : packoffset(c2);
  float ssaoIntensity_g : packoffset(c2.y);
  float emissive_g : packoffset(c2.z);
  float fresnel0_g : packoffset(c2.w);
  float specularGlossiness0_g : packoffset(c3);
  float4 _align_16_a : packoffset(c4);
  float3 diffuseMapColor0_g : packoffset(c5);
  float glowIntensity_g : packoffset(c5.w);
  float glowLumThreshold_g : packoffset(c6);
  float lightProbeExpCorrect_g : packoffset(c6.y);
  uint materialID_g : packoffset(c6.z);
}

SamplerState Smpl0_s : register(s0);
Texture2D<float4> Tex0 : register(t0);
StructuredBuffer<SceneParam> scene_param_g : register(t12);
StructuredBuffer<InstanceParam> instances_g : register(t23);


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_Position0,
  linear sample float3 v1 : NORMAL0,
  linear sample float4 v2 : TEXCOORD0,
  float4 v3 : TEXCOORD1,
  linear sample float4 v4 : TEXCOORD3,
  linear sample float4 v5 : TEXCOORD4,
  nointerpolation uint v6 : TEXCOORD5,
  uint v7 : SV_IsFrontFace0,
  out float4 o0 : SV_Target0,
  out uint2 o1 : SV_Target1,
  out uint2 o2 : SV_Target2)
{
  const float4 icb[] = { { 1.000000, 0, 0, 0},
                              { 0, 1.000000, 0, 0},
                              { 0, 0, 1.000000, 0},
                              { 0, 0, 0, 1.000000} };
  float4 r0,r1,r2,r3,r4,r5,r6;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.x = cmp(0 < char_positions_g[0].w);
  if (r0.x != 0) {
    r0.x = v0.z * v0.w;
    r0.y = 0;
    while (true) {
      r0.z = cmp((int)r0.y >= 2);
      if (r0.z != 0) break;
      r1.xyz = char_positions_g[r0.y].xyw / invVPSize_g.xyy;
      r0.z = cmp(char_positions_g[r0.y].z < r0.x);
      r1.xy = v0.xy + -r1.xy;
      r0.w = dot(r1.xy, r1.xy);
      r0.w = sqrt(r0.w);
      r0.w = cmp(r0.w < r1.z);
      r0.z = r0.w ? r0.z : 0;
      r0.w = cmp(char_world_positions_g[r0.y].y < v5.y);
      r0.z = r0.w ? r0.z : 0;
      if (r0.z != 0) discard;
      r0.y = (int)r0.y + 1;
    }
  }
  r0.x = instances_g[v6.x].param.x;
  r0.y = instances_g[v6.x].param.y;
  r0.z = instances_g[v6.x].param.z;
  r0.w = instances_g[v6.x].param.w;
  r1.x = instances_g[v6.x].param2.x;
  r2.x = scene_param_g[scene_slot_index_g].view_inv_g._m30;
  r2.y = scene_param_g[scene_slot_index_g].view_inv_g._m31;
  r2.z = scene_param_g[scene_slot_index_g].view_inv_g._m32;
  r1.yzw = -v5.xyz + r2.xyz;
  r1.y = dot(r1.yzw, r1.yzw);
  r1.y = sqrt(r1.y);
  r0.x = r1.y + -r0.x;
  r0.x = saturate(r0.x * r0.y);
  r1.zw = cmp(float2(0,0) >= r0.yw);
  r1.zw = r1.zw ? float2(1,1) : 0;
  r0.x = r1.z + r0.x;
  r0.y = cmp(disableNearCameraAlpha_g >= 1);
  r0.y = r0.y ? 1.000000 : 0;
  r0.x = r0.x + r0.y;
  r0.y = -r1.y + r0.z;
  r0.y = saturate(r0.y * r0.w);
  r0.y = r0.y + r1.w;
  r0.z = cmp(disableFarCameraAlpha_g >= 1);
  r0.z = r0.z ? 1.000000 : 0;
  r0.y = r0.y + r0.z;
  r0.xy = min(float2(1,1), r0.xy);
  r0.x = r0.x * r0.y;
  r0.y = v4.w * r0.x;
  r0.zw = float2(0.25,0.25) * v0.xy;
  r1.yz = cmp(r0.zw >= -r0.zw);
  r0.zw = frac(abs(r0.zw));
  r0.zw = r1.yz ? r0.zw : -r0.zw;
  r0.zw = float2(4,4) * r0.zw;
  r0.zw = (int2)r0.zw;
  r2.x = dot(ditherMtx_g._m00_m10_m20_m30, icb[r0.z+0].xyzw);
  r2.y = dot(ditherMtx_g._m01_m11_m21_m31, icb[r0.z+0].xyzw);
  r2.z = dot(ditherMtx_g._m02_m12_m22_m32, icb[r0.z+0].xyzw);
  r2.w = dot(ditherMtx_g._m03_m13_m23_m33, icb[r0.z+0].xyzw);
  r0.z = dot(r2.xyzw, icb[r0.w+0].xyzw);
  r0.w = (int)r1.x & 64;
  r1.y = 1 + -r0.z;
  r0.z = r0.w ? r1.y : r0.z;
  r0.z = v4.w * r0.x + -r0.z;
  r0.xz = cmp(r0.xz < float2(1,0));
  if (r0.z != 0) discard;
  r0.y = cmp(0.99609375 >= r0.y);
  r0.z = r0.y ? 0.000000 : 0;
  bitmask.y = ((~(-1 << 1)) << 0) & 0xffffffff;  r0.y = (((uint)r0.y << 0) & bitmask.y) | ((uint)2 & ~bitmask.y);
  r0.x = r0.x ? r0.y : r0.z;
  r0.y = dot(v1.xyz, v1.xyz);
  r0.y = rsqrt(r0.y);
  r0.yzw = v1.xyz * r0.yyy;
  r1.yz = v2.xy * float2(1,-1) + float2(0,1);
  r2.xyzw = Tex0.Sample(Smpl0_s, r1.yz).xyzw;
  r2.xyz = diffuseMapColor0_g.xyz * r2.xyz;
  r2.xyzw = v4.xyzw * r2.xyzw;
  r1.y = scene_param_g[scene_slot_index_g].view_g._m00;
  r1.z = scene_param_g[scene_slot_index_g].view_g._m10;
  r1.w = scene_param_g[scene_slot_index_g].view_g._m20;
  r3.x = scene_param_g[scene_slot_index_g].view_g._m01;
  r3.y = scene_param_g[scene_slot_index_g].view_g._m11;
  r3.z = scene_param_g[scene_slot_index_g].view_g._m21;
  r4.x = scene_param_g[scene_slot_index_g].view_g._m02;
  r4.y = scene_param_g[scene_slot_index_g].view_g._m12;
  r4.z = scene_param_g[scene_slot_index_g].view_g._m22;
  r5.x = scene_param_g[scene_slot_index_g].view_g._m03;
  r5.y = scene_param_g[scene_slot_index_g].view_g._m13;
  r5.z = scene_param_g[scene_slot_index_g].view_g._m23;
  r6.x = dot(r0.yzw, r1.yzw);
  r6.y = dot(r0.yzw, r3.xyz);
  r6.z = dot(r0.yzw, r4.xyz);
  r6.w = dot(r0.yzw, r5.xyz);
  r0.y = dot(r6.xyzw, r6.xyzw);
  r0.y = rsqrt(r0.y);
  r0.yzw = r6.xyz * r0.yyy;
  r3.x = scene_param_g[scene_slot_index_g].view_g._m00;
  r3.y = scene_param_g[scene_slot_index_g].view_g._m10;
  r3.z = scene_param_g[scene_slot_index_g].view_g._m20;
  r3.w = scene_param_g[scene_slot_index_g].view_g._m30;
  r4.x = scene_param_g[scene_slot_index_g].view_g._m01;
  r4.y = scene_param_g[scene_slot_index_g].view_g._m11;
  r4.z = scene_param_g[scene_slot_index_g].view_g._m21;
  r4.w = scene_param_g[scene_slot_index_g].view_g._m31;
  r5.x = scene_param_g[scene_slot_index_g].view_g._m02;
  r5.y = scene_param_g[scene_slot_index_g].view_g._m12;
  r5.z = scene_param_g[scene_slot_index_g].view_g._m22;
  r5.w = scene_param_g[scene_slot_index_g].view_g._m32;
  r3.x = dot(v5.xyzw, r3.xyzw);
  r3.y = dot(v5.xyzw, r4.xyzw);
  r3.z = dot(v5.xyzw, r5.xyzw);
  r1.y = max(1, emissive_g);
  r4.xyz = r2.xyz * r1.yyy;
  r1.z = cmp(0 < glowLumThreshold_g);
  // r1.w = dot(r2.xyz, float3(0.298999995,0.587000012,0.114));
  r1.w = calculateLuminanceSRGB(r2.rgb);
  r1.w = -glowLumThreshold_g + r1.w;
  r1.w = max(0, r1.w);
  r1.w = r1.w / glowLumThreshold_g;
  r1.w = min(1, r1.w);
  r1.z = r1.z ? r1.w : 1;
  r1.w = glowIntensity_g * r2.w;
  r1.z = r1.w * r1.z;
  r1.w = dot(r3.xyz, r3.xyz);
  r1.w = sqrt(r1.w);
  r1.w = -twoLayeredFogStartDistance_g + r1.w;
  r1.w = saturate(twoLayeredFogDistanceRangeInv_g * r1.w);
  r3.xy = twoLayeredFogHeightFar_g.xy + -twoLayeredFogHeightNear_g.xy;
  r3.xy = r1.ww * r3.xy + twoLayeredFogHeightNear_g.xy;
  if (twoLayeredFogMode_g != 0) {
    r2.w = scene_param_g[scene_slot_index_g].look_at_g.y;
    r2.w = v5.y + -r2.w;
  } else {
    r2.w = v5.y;
  }
  r2.w = r2.w + -r3.x;
  r3.x = r3.y + -r3.x;
  r2.w = saturate(r2.w / r3.x);
  r3.xy = twoLayeredFogDistanceCoefInv_g.y + -twoLayeredFogDistanceCoefInv_g.x;
  r3.xy = r2.ww * r3.xy + twoLayeredFogDistanceCoefInv_g.x;
  r1.w = log2(r1.w);
  r1.w = r3.x * r1.w;
  r1.w = exp2(r1.w);
  r1.w = min(1, r1.w);
  r3.xzw = twoLayeredFogColorLowerFar_g.xyz + -twoLayeredFogColorLowerNear_g.xyz;
  r3.xzw = r1.www * r3.xzw + twoLayeredFogColorLowerNear_g.xyz;
  r5.xyz = twoLayeredFogColorUpperFar_g.xyz + -twoLayeredFogColorUpperNear_g.xyz;
  r5.xyz = r1.www * r5.xyz + twoLayeredFogColorUpperNear_g.xyz;
  r5.xyz = r5.xyz + -r3.xzw;
  r3.xzw = r2.www * r5.xyz + r3.xzw;
  r5.xy = twoLayeredFogMaxIntensity_g.xy + -twoLayeredFogMinIntensity_g.xy;
  r5.xy = r1.ww * r5.xy + twoLayeredFogMinIntensity_g.xy;
  r1.w = r5.y + -r5.x;
  r1.w = r2.w * r1.w + r5.x;
  // r2.w = dot(r4.xyz, float3(0.298999995,0.587000012,0.114));
  r2.w = calculateLuminanceSRGB(r4.rgb);
  r5.xyz = r3.xzw * r1.www;
  // r4.w = dot(r5.xyz, float3(0.298999995,0.587000012,0.114));
  r4.w = calculateLuminanceSRGB(r5.rgb);
  r2.w = -r4.w * 0.5 + r2.w;
  r2.w = max(0, r2.w);
  r5.xyz = r4.xyz * r2.www;
  r6.xyz = r5.xyz * r3.yyy;
  r5.xyz = r5.xyz * r3.yyy + r3.xzw;
  r3.xyz = r6.xyz * r3.xzw;
  r3.xyz = min(float3(1,1,1), r3.xyz);
  r3.xyz = r5.xyz + -r3.xyz;
  r1.w = materialFogIntensity_g * r1.w;
  r2.xyz = -r2.xyz * r1.yyy + r3.xyz;
  o0.xyz = r1.www * r2.xyz + r4.xyz;
  if (5 == 0) r1.x = 0; else if (5+7 < 32) {   r1.x = (uint)r1.x << (32-(5 + 7)); r1.x = (uint)r1.x >> (32-5);  } else r1.x = (uint)r1.x >> 7;
  r1.y = saturate(0.100000001 * r1.z);
  r1.y = 255 * r1.y;
  r1.y = (uint)r1.y;
  bitmask.z = ((~(-1 << 12)) << 8) & 0xffffffff;  r1.z = (((uint)materialID_g << 8) & bitmask.z) | ((uint)0 & ~bitmask.z);
  r0.x = mad((int)r0.x, 0x00100000, 0);
  r1.x = mad((int)r1.x, 0x08000000, 0);
  r1.y = (int)r1.z + (int)r1.y;
  r0.x = (int)r0.x + (int)r1.y;
  o1.x = (int)r1.x + (int)r0.x;
  r1.w = saturate(r1.w);
  r0.x = 255 * r1.w;
  r0.x = (uint)r0.x;
  o1.y = (int)r0.x + 0x00ffff00;
  r0.xyz = saturate(r0.yzw * float3(0.5,0.5,0.5) + float3(0.5,0.5,0.5));
  r0.xyz = float3(255,255,255) * r0.xyz;
  r0.xyz = (uint3)r0.xyz;
  r0.x = mad((int)r0.y, 256, (int)r0.x);
  o2.x = mad((int)r0.z, 0x00010000, (int)r0.x);
  o0.w = 1;
  o2.y = 0;
  
  return;
}