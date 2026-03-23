// ---- Created with 3Dmigoto v1.4.1 on Mon Mar 23 01:12:20 2026
#include "./macleod_boynton.hlsli"
#include "./shared.h"


cbuffer _Globals : register(b0)
{
  float4 AmbColParam : packoffset(c0);
  float4 LuminasParam : packoffset(c1);
  float4 ToneMapParam : packoffset(c2);
  float4 ToneScalerParam : packoffset(c3);
  float4 ToneSaoParam : packoffset(c4);
  float4 GradingParam : packoffset(c5);
  float4x4 ScrnViewProjection : packoffset(c6);
  float4x4 invScrnViewProjection : packoffset(c10);
}

SamplerState PointClampSampler_s : register(s0);
SamplerState LinearClampSampler_s : register(s1);
Texture2D<float4> ColorBuffer : register(t0);
Texture2D<float4> EmissiveBuffer : register(t1);
Texture2D<float4> ColorGradingLUT : register(t2);


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

  r0.xy = (int2)v0.xy;
  r0.zw = float2(0,0);
  r0.xyzw = ColorBuffer.Load(r0.xyz).xyzw;
  

  // Raw hdr color
  float3 colorHDR = r0.xyz;
  //float3 referenceHDR = renodx::tonemap::uncharted2::BT709(r0.xyz);

  r1.x = dot(float3(0.412109375,0.523925781,0.0639648438), r0.xyz);
  r1.y = dot(float3(0.166748047,0.720458984,0.112792969), r0.xyz);
  r1.z = dot(float3(0.0241699219,0.0754394531,0.900390625), r0.xyz);
  o0.w = r0.w;
  r0.xyz = max(float3(0,0,0), r1.xyz);
  r0.xyz = float3(0.00999999978,0.00999999978,0.00999999978) * r0.xyz;
  r0.xyz = log2(r0.xyz);
  r0.xyz = float3(0.159301758,0.159301758,0.159301758) * r0.xyz;
  r0.xyz = exp2(r0.xyz);
  r1.xyz = r0.xyz * float3(18.8515625,18.8515625,18.8515625) + float3(0.8359375,0.8359375,0.8359375);
  r0.xyz = r0.xyz * float3(18.6875,18.6875,18.6875) + float3(1,1,1);
  r0.xyz = r1.xyz / r0.xyz;
  r0.xyz = log2(r0.xyz);
  r0.xyz = float3(78.84375,78.84375,78.84375) * r0.xyz;
  r0.xyz = exp2(r0.xyz);
  r0.w = dot(float2(0.5,0.5), r0.xy);
  r0.w = log2(r0.w);
  r0.w = 0.0126833133 * r0.w;
  r0.w = exp2(r0.w);
  r1.x = -0.8359375 + r0.w;
  r0.w = -r0.w * 18.6875 + 18.8515625;
  r1.x = max(0, r1.x);
  r0.w = r1.x / r0.w;
  r0.w = log2(r0.w);
  r0.w = 6.27739477 * r0.w;
  r0.w = exp2(r0.w);
  r1.x = cmp(r0.w < 0.00999999978);
  r1.y = r0.w * 100 + ToneSaoParam.w;
  r1.z = 100 * r0.w;
  r0.w = r0.w * 100 + -ToneSaoParam.z;
  r1.w = 1 + ToneSaoParam.w;
  r1.w = ToneSaoParam.y * r1.w;
  r1.w = r1.z * r1.w;
  r1.y = r1.w / r1.y;
  r1.w = ToneSaoParam.y * r1.z;
  r1.x = r1.x ? r1.w : r1.y;
  r1.y = ToneSaoParam.x * ToneSaoParam.y;
  r1.w = ToneSaoParam.x + -ToneSaoParam.z;
  r1.y = r1.y * r1.w;
  r1.y = r1.y / -ToneSaoParam.x;
  r1.y = r1.z * r1.y;
  r1.z = cmp(r1.z < ToneSaoParam.x);
  r0.w = -r1.y / r0.w;
  r0.w = r1.z ? r0.w : r1.x;
  r0.w = 0.00999999978 * r0.w;
  r0.w = log2(r0.w);
  r0.w = 0.159301758 * r0.w;
  r0.w = exp2(r0.w);
  r1.xy = r0.ww * float2(18.8515625,18.6875) + float2(0.8359375,1);
  r0.w = r1.x / r1.y;
  r0.w = log2(r0.w);
  r0.w = 78.84375 * r0.w;
  r1.x = exp2(r0.w);
  r1.y = dot(float3(1.61376953,-3.32348633,1.7097168), r0.xyz);
  r1.z = dot(float3(4.37817383,-4.24560547,-0.132568359), r0.xyz);
  r0.x = dot(float3(1,0.00860514585,0.111035608), r1.xyz);
  r0.x = log2(r0.x);
  r0.w = dot(float3(1,-0.00860514585,-0.111035608), r1.xyz);
  r1.x = dot(float3(1,0.560048878,-0.320637465), r1.xyz);
  r0.z = log2(r1.x);
  r0.y = log2(r0.w);
  r0.xyz = float3(0.0126833133,0.0126833133,0.0126833133) * r0.xyz;
  r0.xyz = exp2(r0.xyz);
  r1.xyz = float3(-0.8359375,-0.8359375,-0.8359375) + r0.xyz;
  r0.xyz = -r0.xyz * float3(18.6875,18.6875,18.6875) + float3(18.8515625,18.8515625,18.8515625);
  r1.xyz = max(float3(0,0,0), r1.xyz);
  r0.xyz = r1.xyz / r0.xyz;
  r0.xyz = log2(r0.xyz);
  r0.xyz = float3(6.27739477,6.27739477,6.27739477) * r0.xyz;
  r0.xyz = exp2(r0.xyz);
  r0.xyz = float3(100,100,100) * r0.xyz;
  r1.x = dot(float3(2.07017994,-1.3264569,0.206616014), r0.xyz);
  r1.y = dot(float3(0.364988238,0.680467367,-0.0454217531), r0.xyz);
  r1.z = dot(float3(-0.0495955423,-0.0494211614,1.18799591), r0.xyz);
  r0.x = dot(float3(1.71235168,-0.354878962,-0.250341356), r1.xyz);
  r0.y = dot(float3(-0.667286217,1.61794055,0.0149537995), r1.xyz);
  r0.z = dot(float3(0.0176398493,-0.0427706018,0.942103207), r1.xyz);
  r1.xyz = log2(abs(r0.xyz));
  r1.xyz = float3(0.416666657,0.416666657,0.416666657) * r1.xyz;
  r1.xyz = exp2(r1.xyz);
  r1.xyz = r1.xyz * float3(1.05499995,1.05499995,1.05499995) + float3(-0.0549999997,-0.0549999997,-0.0549999997);
  
  //  SDR color before graded
  float3 colorSDRNeutral = r0.xyz;
  float3 referenceSDR = renodx::tonemap::Reinhard(r0.xyz);

  r2.xyz = float3(12.9200001,12.9200001,12.9200001) * r0.xyz;
  r0.xyz = cmp(float3(0.00313080009,0.00313080009,0.00313080009) >= r0.xyz);
  r0.xyz = r0.xyz ? r2.xyz : r1.xyz;
  r1.xyz = saturate(r0.xyz); 
  r1.xyz = r1.xyz * float3(0.96875,0.96875,0.96875) + float3(0.015625,0.015625,0.015625);
  r2.y = 1 + -r1.y;
  r0.w = r1.z * 32 + -0.5;
  r1.y = floor(r0.w);
  r0.w = saturate(-r1.y + r0.w);
  r1.x = r1.x + r1.y;
  r2.z = r1.x * 0.03125 + 0.03125;
  r2.x = 0.03125 * r1.x;
  r1.xyzw = ColorGradingLUT.Sample(LinearClampSampler_s, r2.xy).xyzw;
  r2.xyzw = ColorGradingLUT.Sample(LinearClampSampler_s, r2.zy).xyzw;
  r2.xyz = r2.xyz + -r1.xyz;
  r1.xyz = r0.www * r2.xyz + r1.xyz;
  r1.xyz = r1.xyz + -r0.xyz;
  r2.xyzw = EmissiveBuffer.SampleLevel(PointClampSampler_s, v1.xy, 0).xyzw;
  r0.w = 1 + -r2.y;
  r0.w = saturate(GradingParam.z * r0.w);
  r0.xyz = r0.www * r1.xyz + r0.xyz;
  r0.xyz = log2(abs(r0.xyz));
  r0.xyz = ToneMapParam.www * r0.xyz;
  r0.xyz = exp2(r0.xyz);


  // gamma space back to linear space
  float3 colorSDRGraded = renodx::color::srgb::Decode(r0.xyz); 
  

  float3 upgradedColor = renodx::tonemap::UpgradeToneMap(colorHDR, colorSDRNeutral, colorSDRGraded, 0);

  float3 finalHDRColor = renodx::draw::ToneMapPass(upgradedColor);

  if (shader_injection.tone_map_hue_processor == 3.f) {
    float strength = shader_injection.tone_map_hue_correction; 
    if (strength > 0.f) {
      // Use the original HDR color as the hue/purity reference
      finalHDRColor = CorrectHueAndPurityMBGated(
          finalHDRColor, referenceSDR, 1.f, 0.f, 0.f, 1.0f, 1.0f, float2(-1.f, -1.f), 1e-6f);
    }
  }


  r0.xyz = finalHDRColor;


  r1.xy = v1.xy * float2(2,2) + float2(-1,-1);
  r0.w = dot(r1.xy, r1.xy);
  r0.w = sqrt(r0.w);
  r0.w = -GradingParam.y + r0.w;
  r0.w = max(0, r0.w);
  r0.w = saturate(-r0.w * GradingParam.x + 1);
  
  o0.xyz = r0.xyz * r0.www;
  o0.xyz = renodx::color::srgb::Decode(o0.xyz);
  o0.xyz = renodx::draw::RenderIntermediatePass(o0.xyz);
  return;
}