// ---- Created with 3Dmigoto v1.3.16 on Sun Aug 31 21:45:36 2025
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
SamplerState PointClampSamplerState_s : register(s1);
Texture2D<float4> ColorBuffer : register(t0);
Texture2D<float4> DepthBuffer : register(t1);
Texture2D<float4> GlareBuffer : register(t2);
Texture2D<float4> FilterTexture : register(t3);


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_POSITION0,
  float2 v1 : TEXCOORD0,
  float2 w1 : TEXCOORD1,
  out float4 o0 : SV_TARGET0)
{
  float4 r0,r1,r2,r3;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.xy = float2(-0.5,-0.5) + v1.yx;
  r0.xy = float2(1.25,1.77777779) * r0.xy;
  r0.z = dot(r0.xy, r0.xy);
  r0.w = r0.z / WhirlPinchParams.z;
  r0.w = sqrt(r0.w);
  r1.x = 1.57079637 * r0.w;
  r0.w = 1 + -r0.w;
  r1.x = sin(r1.x);
  r1.x = log2(abs(r1.x));
  r1.x = -WhirlPinchParams.y * r1.x;
  r1.x = exp2(r1.x);
  r0.xy = r1.xx * r0.xy;
  r1.x = WhirlPinchParams.x * r0.w;
  r0.w = r1.x * r0.w;
  sincos(r0.w, r1.x, r2.x);
  r1.xy = r1.xx * r0.xy;
  r3.x = r2.x * r0.y + -r1.x;
  r3.y = r2.x * r0.x + r1.y;
  r1.xy = r3.xy * float2(0.5625,0.800000012) + float2(0.5,0.5);
  r1.zw = r1.xy * UvScaleBias.xy + UvScaleBias.zw;
  r0.x = cmp(r0.z < WhirlPinchParams.z);
  r0.y = cmp(0 < r0.z);
  r0.x = r0.y ? r0.x : 0;

  float4 coord = float4(v1.x, v1.y, w1.x, w1.y);
  r0.xyzw = r0.xxxx ? r1.xyzw : coord.xyzw;
  r0.xy = r0.xy * float2(1,-1) + float2(0,1);
  r1.xyz = GlareBuffer.SampleLevel(LinearClampSamplerState_s, r0.xy, 0).xyz;
  r1.xyz = GlowIntensity.www * r1.xyz;
  float3 bloom = r1.rgb;
  r2.xyz = ColorBuffer.SampleLevel(LinearClampSamplerState_s, r0.zw, 0).xyz;
  r0.x = DepthBuffer.SampleLevel(PointClampSamplerState_s, r0.zw, 0).x;
  r0.x = DofParams.x + -r0.x;
  r0.x = saturate(DofParams.y / r0.x);
  r0.x = ToneFactor.y + r0.x;
  r0.x = min(1, r0.x);
  r0.yzw = ToneFactor.xxx * r2.xyz;
  r2.xyz = -r2.xyz * ToneFactor.xxx + float3(1,1,1);
  // r0.yzw = r1.xyz * r2.xyz + r0.yzw;
  r0.yzw = r0.yzw;
  r1.xyz = float3(1,1,1) + -r0.yzw;
  r2.xy = v1.xy * float2(1,-1) + float2(0,1);
  r2.xyzw = FilterTexture.SampleLevel(LinearClampSamplerState_s, r2.xy, 0).xyzw;
  r2.xyzw = FilterColor.xyzw * r2.xyzw;
  r2.xyz = r2.xyz * r2.www;
  r3.xyz = r2.xyz * r0.xxx;
  r2.xyz = r2.xyz * r0.xxx + r0.yzw;
  r0.xyz = r3.xyz * r1.xyz + r0.yzw;
  r0.xyz = r0.xyz + -r2.xyz;
  o0.xyz = r0.xyz * float3(0.5,0.5,0.5) + r2.xyz;

  o0.rgb = decodeColor(o0.rgb);
  bloom = decodeColor(bloom);
  o0.rgb = hdrScreenBlend(o0.rgb, bloom);
  o0.rgb = processAndToneMap(o0.rgb);
  o0.w = 1;
  return;
}