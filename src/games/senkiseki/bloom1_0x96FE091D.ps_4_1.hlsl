// ---- Created with 3Dmigoto v1.3.16 on Fri Jun 06 17:04:53 2025
#include "shared.h"
#include "cs4/common.hlsl"
SamplerState LinearClampSamplerState_s : register(s0);
Texture2D<float4> ColorBuffer : register(t0);


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_POSITION0,
  float4 v1 : TEXCOORD0,
  float4 v2 : TEXCOORD1,
  float4 v3 : TEXCOORD2,
  float4 v4 : TEXCOORD3,
  float4 v5 : TEXCOORD4,
  float4 v6 : TEXCOORD5,
  out float4 o0 : SV_TARGET0)
{
  float4 r0,r1;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.xyzw = ColorBuffer.SampleLevel(LinearClampSamplerState_s, v2.zw, 0).xyzw;
  r0.rgb = processBloomBuffer(r0.rgb);
  r0.xyzw = float4(0.170000002, 0.170000002, 0.170000002, 0.170000002) * r0.xyzw;
  r1.xyzw = ColorBuffer.SampleLevel(LinearClampSamplerState_s, v2.xy, 0).xyzw;
  r1 = processBloomBuffer(r1);
  r0.xyzw = r1.xyzw * float4(0.189999998, 0.189999998, 0.189999998, 0.189999998) + r0.xyzw;
  r1.xyzw = ColorBuffer.SampleLevel(LinearClampSamplerState_s, v3.xy, 0).xyzw;
  r1 = processBloomBuffer(r1);
  r0.xyzw = r1.xyzw * float4(0.150000006, 0.150000006, 0.150000006, 0.150000006) + r0.xyzw;
  r1.xyzw = ColorBuffer.SampleLevel(LinearClampSamplerState_s, v3.zw, 0).xyzw;
  r1 = processBloomBuffer(r1);
  r0.xyzw = r1.xyzw * float4(0.129999995, 0.129999995, 0.129999995, 0.129999995) + r0.xyzw;
  r1.xyzw = ColorBuffer.SampleLevel(LinearClampSamplerState_s, v4.xy, 0).xyzw;
  r1 = processBloomBuffer(r1);
  r0.xyzw = r1.xyzw * float4(0.109999999, 0.109999999, 0.109999999, 0.109999999) + r0.xyzw;
  r1.xyzw = ColorBuffer.SampleLevel(LinearClampSamplerState_s, v4.zw, 0).xyzw;
  r1 = processBloomBuffer(r1);
  r0.xyzw = r1.xyzw * float4(0.0900000036, 0.0900000036, 0.0900000036, 0.0900000036) + r0.xyzw;
  r1.xyzw = ColorBuffer.SampleLevel(LinearClampSamplerState_s, v5.xy, 0).xyzw;
  r1 = processBloomBuffer(r1);
  r0.xyzw = r1.xyzw * float4(0.0700000003, 0.0700000003, 0.0700000003, 0.0700000003) + r0.xyzw;
  r1.xyzw = ColorBuffer.SampleLevel(LinearClampSamplerState_s, v5.zw, 0).xyzw;
  r1 = processBloomBuffer(r1);
  r0.xyzw = r1.xyzw * float4(0.0500000007, 0.0500000007, 0.0500000007, 0.0500000007) + r0.xyzw;
  r1.xyzw = ColorBuffer.SampleLevel(LinearClampSamplerState_s, v6.xy, 0).xyzw;
  r1 = processBloomBuffer(r1);
  r0.xyzw = r1.xyzw * float4(0.0299999993, 0.0299999993, 0.0299999993, 0.0299999993) + r0.xyzw;
  r1.xyzw = ColorBuffer.SampleLevel(LinearClampSamplerState_s, v6.zw, 0).xyzw;
  r1 = processBloomBuffer(r1);
  o0.xyzw = r1.xyzw * float4(0.00999999978, 0.00999999978, 0.00999999978, 0.00999999978) + r0.xyzw;
  o0.rgb = max(o0.rgb, 0.f);
  // o0.rgb = clamp(o0.rgb, 0.f, shader_injection.safe_clamp);

  return;
}