// ---- Created with 3Dmigoto v1.4.1 on Mon Mar 23 01:12:20 2026

SamplerState PointClampSampler_s : register(s0);
Texture2D<float4> DoFCompositeNearInput : register(t0);
Texture2D<float4> DoFCompositeBlurInput : register(t1);
Texture2D<float4> DoFCompositeColorInput : register(t2);


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_POSITION0,
  float2 v1 : TEXCOORD0,
  out float4 o0 : SV_TARGET0)
{
  float4 r0,r1;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.xyzw = DoFCompositeBlurInput.SampleLevel(PointClampSampler_s, v1.xy, 0).xyzw;
  r0.w = r0.w * 2 + -1;
  r1.x = 1.5 * r0.w;
  r1.x = min(1, r1.x);
  r1.y = cmp(0.100000001 < r0.w);
  r0.w = r1.y ? r1.x : r0.w;
  r1.xyz = DoFCompositeColorInput.SampleLevel(PointClampSampler_s, v1.xy, 0).xyz;
  r0.xyz = -r1.xyz + r0.xyz;
  r0.xyz = abs(r0.www) * r0.xyz + r1.xyz;
  r1.xyzw = DoFCompositeNearInput.SampleLevel(PointClampSampler_s, v1.xy, 0).xyzw;
  r0.w = 1 + -r1.w;
  o0.xyz = r0.xyz * r0.www + r1.xyz;
  o0.w = 1;
  return;
}