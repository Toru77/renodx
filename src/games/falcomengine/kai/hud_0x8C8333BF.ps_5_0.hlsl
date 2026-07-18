// ---- Created with 3Dmigoto v1.3.16 on Sat Mar 14 15:19:14 2026
#include "../common.hlsl"
cbuffer cb_ui_hdr_cbuffer : register(b9)
{
  float hdr_ui_brightness_g : packoffset(c0);
  float3 padding : packoffset(c0.y);
}

SamplerState samLinear_s : register(s0);
Texture2D<float4> colorTexture : register(t0);


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_Position0,
  float4 v1 : TEXCOORD0,
  float4 v2 : TEXCOORD1,
  float4 v3 : TEXCOORD2,
  nointerpolation int v4 : TEXCOORD3,
  out float4 o0 : SV_Target0)
{
  const float4 icb[] = { { 1.000000, 0, 0, 0},
                              { 0, 1.000000, 0, 0},
                              { 0, 0, 1.000000, 0},
                              { 0, 0, 0, 1.000000} };
  float4 r0,r1,r2,r3,r4,r5;
  uint4 bitmask, uiDest;
  float4 fDest;

  colorTexture.GetDimensions(0, fDest.x, fDest.y, fDest.z);
  r0.xy = fDest.xy;
  r0.xyzw = float4(1,1,1,1) / r0.xyxy;
  r1.x = cmp((int)v4.x >= 0);
  if (r1.x != 0) {
    r1.y = ddx_coarse(v1.x);
    r1.y = cmp(0.000600000028 < r1.y);
    if (r1.y != 0) {
      r1.yzw = float3(0,0,-1);
      while (true) {
        r2.x = cmp(2 < (int)r1.w);
        if (r2.x != 0) break;
        r2.x = (int)r1.w;
        r2.zw = r1.yz;
        r3.x = -2;
        while (true) {
          r3.y = cmp(2 < (int)r3.x);
          if (r3.y != 0) break;
          r2.y = (int)r3.x;
          r3.y = dot(r2.xy, r2.xy);
          r3.y = 6.65999985 + -r3.y;
          r3.zw = r2.xy * r0.xy + v1.xy;
          r4.xyzw = colorTexture.SampleLevel(samLinear_s, r3.zw, 0).xyzw;
          r2.y = v4.x;
          r2.y = dot(r4.xyzw, icb[r2.y+0].xyzw);
          r2.z = r2.y * r3.y + r2.z;
          r2.w = r3.y + r2.w;
          r3.x = (int)r3.x + 1;
        }
        r1.yz = r2.zw;
        r1.w = (int)r1.w + 1;
      }
      r2.xyzw = r1.yyyy / r1.zzzz;
    } else {
      r3.xyzw = r0.zwzw * float4(-0.757771552,-0.757771552,0.757771552,-0.757771552) + v1.xyxy;
      r4.xyzw = colorTexture.SampleLevel(samLinear_s, r3.xy, 0).xyzw;
      r3.xyzw = colorTexture.SampleLevel(samLinear_s, r3.zw, 0).xyzw;
      r3.xyzw = float4(0.374875665,0.374875665,0.374875665,0.374875665) * r3.xyzw;
      r3.xyzw = r4.xyzw * float4(0.374875665,0.374875665,0.374875665,0.374875665) + r3.xyzw;
      r4.xyzw = r0.zwzw * float4(0.757771552,0.757771552,-0.757771552,0.757771552) + v1.xyxy;
      r5.xyzw = colorTexture.SampleLevel(samLinear_s, r4.xy, 0).xyzw;
      r3.xyzw = r5.xyzw * float4(0.374875665,0.374875665,0.374875665,0.374875665) + r3.xyzw;
      r4.xyzw = colorTexture.SampleLevel(samLinear_s, r4.zw, 0).xyzw;
      r3.xyzw = r4.xyzw * float4(0.374875665,0.374875665,0.374875665,0.374875665) + r3.xyzw;
      r4.xyzw = r0.zwzw * float4(-2.90709925,0,2.90709925,0) + v1.xyxy;
      r5.xyzw = colorTexture.SampleLevel(samLinear_s, r4.xy, 0).xyzw;
      r3.xyzw = r5.xyzw * float4(-0.124875657,-0.124875657,-0.124875657,-0.124875657) + r3.xyzw;
      r4.xyzw = colorTexture.SampleLevel(samLinear_s, r4.zw, 0).xyzw;
      r3.xyzw = r4.xyzw * float4(-0.124875657,-0.124875657,-0.124875657,-0.124875657) + r3.xyzw;
      r4.xyzw = r0.zwzw * float4(0,-2.90709925,0,2.90709925) + v1.xyxy;
      r5.xyzw = colorTexture.SampleLevel(samLinear_s, r4.xy, 0).xyzw;
      r3.xyzw = r5.xyzw * float4(-0.124875657,-0.124875657,-0.124875657,-0.124875657) + r3.xyzw;
      r4.xyzw = colorTexture.SampleLevel(samLinear_s, r4.zw, 0).xyzw;
      r3.xyzw = r4.xyzw * float4(-0.124875657,-0.124875657,-0.124875657,-0.124875657) + r3.xyzw;
      r2.xyzw = max(float4(0,0,0,0), r3.xyzw);

    }
  } else {
    r3.xyzw = r0.zwzw * float4(-0.378885776,-0.378885776,0.378885776,-0.378885776) + v1.xyxy;
    r4.xyzw = colorTexture.SampleLevel(samLinear_s, r3.xy, 0).xyzw;
    r3.xyzw = colorTexture.SampleLevel(samLinear_s, r3.zw, 0).xyzw;
    r3.xyzw = float4(0.374875665,0.374875665,0.374875665,0.374875665) * r3.xyzw;
    r3.xyzw = r4.xyzw * float4(0.374875665,0.374875665,0.374875665,0.374875665) + r3.xyzw;
    r4.xyzw = r0.zwzw * float4(0.378885776,0.378885776,-0.378885776,0.378885776) + v1.xyxy;
    r5.xyzw = colorTexture.SampleLevel(samLinear_s, r4.xy, 0).xyzw;
    r3.xyzw = r5.xyzw * float4(0.374875665,0.374875665,0.374875665,0.374875665) + r3.xyzw;
    r4.xyzw = colorTexture.SampleLevel(samLinear_s, r4.zw, 0).xyzw;
    r3.xyzw = r4.xyzw * float4(0.374875665,0.374875665,0.374875665,0.374875665) + r3.xyzw;
    r4.xyzw = r0.zwzw * float4(-1.45354962,0,1.45354962,0) + v1.xyxy;
    r5.xyzw = colorTexture.SampleLevel(samLinear_s, r4.xy, 0).xyzw;
    r3.xyzw = r5.xyzw * float4(-0.124875657,-0.124875657,-0.124875657,-0.124875657) + r3.xyzw;
    r4.xyzw = colorTexture.SampleLevel(samLinear_s, r4.zw, 0).xyzw;
    r3.xyzw = r4.xyzw * float4(-0.124875657,-0.124875657,-0.124875657,-0.124875657) + r3.xyzw;
    r0.xyzw = r0.xyzw * float4(0,-1.45354962,0,1.45354962) + v1.xyxy;
    r4.xyzw = colorTexture.SampleLevel(samLinear_s, r0.xy, 0).xyzw;
    r3.xyzw = r4.xyzw * float4(-0.124875657,-0.124875657,-0.124875657,-0.124875657) + r3.xyzw;
    r0.xyzw = colorTexture.SampleLevel(samLinear_s, r0.zw, 0).xyzw;
    r0.xyzw = r0.xyzw * float4(-0.124875657,-0.124875657,-0.124875657,-0.124875657) + r3.xyzw;
    r2.xyzw = max(float4(0,0,0,0), r0.xyzw);
  }
  r0.xyzw = max(float4(0,0,0,0), r2.xyzw);
  if (r1.x != 0) {
    r1.x = v4.x;
    r0.w = dot(r0.xyzw, icb[r1.x+0].xyzw);
    r0.xyz = float3(1,1,1);
  }

  r0.rgb = renodx::color::srgb::DecodeSafe(r0.rgb);

  r0.xyz = hdr_ui_brightness_g * r0.xyz;

  r0.rgb = renodx::color::srgb::EncodeSafe(r0.rgb);
  r1.x = v2.w * r0.w;
  r0.xyz = r0.xyz * v2.xyz + v3.xyz;

  if (RENODX_TONE_MAP_TYPE > 0.f) {
  } else {
    r0.xyz = min(float3(1, 1, 1), r0.xyz);
  }

  r0.w = r0.w * v2.w + -1;
  r0.w = v1.z * r0.w + 1;
  o0.xyz = r0.xyz * r0.www;

  if (RENODX_SCENE_ALREADY_TONEMAPPED == 0.f) 
    o0.rgb = processUI(o0.rgb, true);
  
  o0.w = r1.x;
  return;
}