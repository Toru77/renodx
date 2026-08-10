// ---- Created with 3Dmigoto v1.3.16 on Tue Aug 26 19:07:22 2025
#include "../cs4/common.hlsl"
cbuffer _Globals : register(b0)
{
  uint4 DuranteSettings : packoffset(c0);

  struct
  {
    float3 EyePosition;
    float4x4 View;
    float4x4 Projection;
    float4x4 ViewProjection;
    float4x4 ViewInverse;
    float4x4 ProjectionInverse;
    float2 cameraNearFar;
    float cameraNearTimesFar;
    float cameraFarMinusNear;
    float cameraFarMinusNearInv;
    float2 ViewportWidthHeight;
    float2 screenWidthHeightInv;
    float3 GlobalAmbientColor;
    float Time;
    float3 FakeRimLightDir;
    float3 FogColor;
    float4 FogRangeParameters;
    float3 MiscParameters1;
    float4 MiscParameters2;
    float3 MonotoneMul;
    float3 MonotoneAdd;
    float3 UserClipPlane2;
    float4 UserClipPlane;
    float4 MiscParameters3;
    float AdditionalShadowOffset;
    float AlphaTestDirection;
    float4 MiscParameters4;
    float3 MiscParameters5;
    float4 light1_attenuation;
    float3 light2_position;
    float3 light2_colorIntensity;
    float4 light2_attenuation;
  } scene : packoffset(c1);

  bool PhyreContextSwitches : packoffset(c43);
  float4 FilterColor : packoffset(c44) = {1,1,1,1};
  float4 FadingColor : packoffset(c45) = {1,1,1,1};
  float4 _MonotoneMul : packoffset(c46) = {1,1,1,1};
  float4 _MonotoneAdd : packoffset(c47) = {0,0,0,0};
  float4 GlowIntensity : packoffset(c48) = {1,1,1,1};
  float4 GodrayParams : packoffset(c49) = {0,0,0,0};
  float4 GodrayParams2 : packoffset(c50) = {0,0,0,0};
  float4 GodrayColor : packoffset(c51) = {0,0,0,1};
  float4 SSRParams : packoffset(c52) = {5,0.300000012,15,1024};
  float4 ToneFactor : packoffset(c53) = {1,1,1,1};
  float4 UvScaleBias : packoffset(c54) = {1,1,0,0};
  float4 GaussianBlurParams : packoffset(c55) = {0,0,0,0};
  float4 DofParams : packoffset(c56) = {0,0,0,0};
  float4 DofParams2 : packoffset(c57) = {0,0,0,0};
  float4 GammaParameters : packoffset(c58) = {1,1,1,0};
  float4 NoiseParams : packoffset(c59) = {0,0,0,0};
  float4 WhirlPinchParams : packoffset(c60) = {0,0,0,0};
  float4 UVWarpParams : packoffset(c61) = {0,0,0,0};
  float4 MotionBlurParams : packoffset(c62) = {0,0,0,0};
  float GlobalTexcoordFactor : packoffset(c63);
}

SamplerState LinearClampSamplerState_s : register(s0);
Texture2DMS<float> DepthBuffer : register(t0);
Texture2D<float4> AOBuffer : register(t1);
Texture2D<float4> AOColorBuffer : register(t2);


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_POSITION0,
  float2 v1 : TEXCOORD0,
  out float4 o0 : SV_TARGET0)
{
  float4 r0,r1,r2;
  uint4 bitmask, uiDest;
  float4 fDest;

  // --- Resolve integer pixel coords from normalized UV ---
  DepthBuffer.GetDimensions(uiDest.x, uiDest.y, uiDest.z);
  r0.xy = uiDest.xy;  // (w,h)
  r0.xy = (uint2)r0.xy;
  r0.xy = v1.xy * r0.xy;  // float pixel coords
  r0.xy = (int2)r0.xy;    // int2 for .Load
  r0.zw = float2(0,0);

   // --- Read depth and linearize using near/far ---
  r0.x = DepthBuffer.Load(r0.xy, 0).x;
  r0.x = r0.x * 2 + -1;
  r0.y = scene.cameraNearFar.y + scene.cameraNearFar.x;
  r0.z = scene.cameraNearFar.y + -scene.cameraNearFar.x;
  r0.x = -r0.x * r0.z + r0.y;
  r0.y = dot(scene.cameraNearFar.yy, scene.cameraNearFar.xx);
  r0.x = r0.y / r0.x;

    // --- AO radius windowing vs fog range (start building a mix mask) ---
  r0.y = min(400, scene.FogRangeParameters.y);
  r0.z = -r0.y * 0.5 + r0.x;
  r0.y = 0.5 * r0.y;
  r0.z = saturate(r0.z / r0.y);

  // --- Sample AO mask (x) ---
  r0.y = cmp(r0.y < r0.x);
  r0.w = AOBuffer.SampleLevel(LinearClampSamplerState_s, v1.xy, 0).x;

  // NOTE: AO is a scalar mask. If it were stored gamma-encoded, we'd decode;
  // but AO is usually linear. Leave as-is.

  // r0.w = renodx::color::srgb::DecodeSafe(r0.w);

   // --- Gentle AO/edge blend around mid-fog ---
  r1.x = 1 + -r0.w;
  r0.z = r0.z * r1.x + r0.w;
  r0.y = r0.y ? r0.z : r0.w;

   // --- Clamp AO for very near depths (avoid AO popping close to camera) ---
  r0.z = -1 + r0.w;
  r0.w = cmp(r0.x < 2);
  r1.x = saturate(-1 + r0.x);
  r0.z = r1.x * r0.z + 1;
  r0.y = r0.w ? r0.z : r0.y;
  r0.y = 1 + -r0.y;

   // --- Sample scene color that AO will modulate ---
  r1.xyzw = AOColorBuffer.SampleLevel(LinearClampSamplerState_s, v1.xy, 0).xyzw;
  r1.rgb = decodeColor(r1.rgb);

    // --- Color-aware falloff (original code compares to a beige-ish color in sRGB) ---
  // This detects skin/bright regions and reduces AO there by building r0.z in [0..1].
  r2.xyz = float3(-0.929411769,-0.854901969,-0.709803939) + r1.xyz;
  r0.z = (dot(r2.xyz, r2.xyz));
  r0.z = sqrt(r0.z);
  r0.z = r0.z + r0.z;
  r0.z = min(1, r0.z);

  // r0.y currently = (1 - AO after near/far eases)
  // Next lines: attenuate that darkness by color distance factor r0.z
  r0.y = -r0.y * r0.z + 1;
  r0.y = 1 + -r0.y;

   // --- Fog-space shaping to fade AO with distance (softens far AO) ---
  r0.z = saturate(-0.200000003 + scene.FogRangeParameters.w);
  r0.w = scene.FogRangeParameters.y + -scene.FogRangeParameters.x;
  r0.z = -r0.w * r0.z + scene.FogRangeParameters.y;
  r0.xz = -scene.FogRangeParameters.xx + r0.xz;
  r0.z = 1 / r0.z;
  r0.x = saturate(r0.x * r0.z);
  r0.z = r0.x * -2 + 3;
  r0.x = r0.x * r0.x;
  r0.x = -r0.z * r0.x + 1;
  r0.x = -r0.y * r0.x + 1;

  // --- ORIGINAL OUTPUT (sRGB-space multiplication) ---
  o0.xyz = r0.xxx * r1.xyz;  // BAD: multiplies sRGB by AO (non-linear darkening)

  o0.rgb = renodx::color::srgb::EncodeSafe(o0.rgb);
  o0.w = r1.w;   // preserve alpha
  return;
}