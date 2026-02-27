// ---- Created with 3Dmigoto v1.4.1 on Fri Feb 27 14:56:55 2026

#include "./shared.h"

cbuffer cb_ui_hdr_cbuffer : register(b4)
{
  float hdr_ui_brightness_g : packoffset(c0);
  float3 unused : packoffset(c0.y);
}

SamplerState samLinear_s : register(s0);
Texture2D<float4> colorTexture : register(t0);


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_Position0,
  float3 v1 : TEXCOORD0,
  float w1 : TEXCOORD4,
  float4 v2 : TEXCOORD1,
  float4 v3 : TEXCOORD2,
  nointerpolation int v4 : TEXCOORD3,
  out float4 o0 : SV_Target0)
{
  const float4 icb[] = { { 1.000000, 0, 0, 0},
                              { 0, 1.000000, 0, 0},
                              { 0, 0, 1.000000, 0},
                              { 0, 0, 0, 1.000000} };
  float4 r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.x = cmp((int)v4.x >= 0);
  r0.y = colorTexture.CalculateLevelOfDetail(samLinear_s, v1.xy);
  r0.y = trunc(r0.y);
  r0.z = (uint)r0.y;
  
  // --- 1. RESINFO FIX ---
  // Replaces the broken resinfo_indexable with valid HLSL GetDimensions
  uint resWidth, resHeight, resMips;
  colorTexture.GetDimensions((uint)r0.z, resWidth, resHeight, resMips);
  r0.z = (float)resWidth;
  r0.w = (float)resHeight;
  // ----------------------

  r1.xyzw = float4(1,1,1,1) / r0.zwzw;
  if (r0.x != 0) {
    r0.z = ddx_coarse(v1.x);
    r0.w = cmp(0.00079999998 < r0.z);
    if (r0.w != 0) {
      r2.xyzw = float4(0,0,0,0);
      r0.w = 0;
      r3.x = -2;
      while (true) {
        r3.y = cmp(2 < (int)r3.x);
        if (r3.y != 0) break;
        r4.xz = (int2)r3.xx;
        r5.xyzw = r2.xyzw;
        r3.y = r0.w;
        r3.z = -2;
        while (true) {
          r3.w = cmp(2 < (int)r3.z);
          if (r3.w != 0) break;
          r3.w = (int)r3.z | (int)r3.x;
          r4.yw = (int2)r3.zz;
          r6.x = dot(r4.zw, r4.zw);
          r6.x = sqrt(r6.x);
          r6.x = 1 / r6.x;
          r3.w = r3.w ? r6.x : 1;
          r6.xyzw = r4.xyzw * r1.xyzw + v1.xyxy;
          r7.xyzw = r1.zwzw * float4(-0.757771552,-0.757771552,0.757771552,-0.757771552) + r6.zwzw;
          r8.xyzw = colorTexture.SampleLevel(samLinear_s, r7.xy, r0.y).xyzw;
          r7.xyzw = colorTexture.SampleLevel(samLinear_s, r7.zw, r0.y).xyzw;
          r7.xyzw = float4(0.374875665,0.374875665,0.374875665,0.374875665) * r7.xyzw;
          r7.xyzw = r8.xyzw * float4(0.374875665,0.374875665,0.374875665,0.374875665) + r7.xyzw;
          r8.xyzw = r1.zwzw * float4(0.757771552,0.757771552,-0.757771552,0.757771552) + r6.zwzw;
          r9.xyzw = colorTexture.SampleLevel(samLinear_s, r8.xy, r0.y).xyzw;
          r7.xyzw = r9.xyzw * float4(0.374875665,0.374875665,0.374875665,0.374875665) + r7.xyzw;
          r8.xyzw = colorTexture.SampleLevel(samLinear_s, r8.zw, r0.y).xyzw;
          r7.xyzw = r8.xyzw * float4(0.374875665,0.374875665,0.374875665,0.374875665) + r7.xyzw;
          r8.xyzw = r1.zwzw * float4(-2.90709925,0,2.90709925,0) + r6.zwzw;
          r9.xyzw = colorTexture.SampleLevel(samLinear_s, r8.xy, r0.y).xyzw;
          r7.xyzw = r9.xyzw * float4(-0.124875657,-0.124875657,-0.124875657,-0.124875657) + r7.xyzw;
          r8.xyzw = colorTexture.SampleLevel(samLinear_s, r8.zw, r0.y).xyzw;
          r7.xyzw = r8.xyzw * float4(-0.124875657,-0.124875657,-0.124875657,-0.124875657) + r7.xyzw;
          r6.xyzw = r1.zwzw * float4(0,-2.90709925,0,2.90709925) + r6.xyzw;
          r8.xyzw = colorTexture.SampleLevel(samLinear_s, r6.xy, r0.y).xyzw;
          r7.xyzw = r8.xyzw * float4(-0.124875657,-0.124875657,-0.124875657,-0.124875657) + r7.xyzw;
          r6.xyzw = colorTexture.SampleLevel(samLinear_s, r6.zw, r0.y).xyzw;
          r6.xyzw = r6.xyzw * float4(-0.124875657,-0.124875657,-0.124875657,-0.124875657) + r7.xyzw;
          r6.xyzw = max(float4(0,0,0,0), r6.xyzw);
          r5.xyzw = r6.xyzw * r3.wwww + r5.xyzw;
          r3.y = r3.y + r3.w;
          r3.z = (int)r3.z + 1;
        }
        r2.xyzw = r5.xyzw;
        r0.w = r3.y;
        r3.x = (int)r3.x + 1;
      }
      r2.xyzw = r2.xyzw / r0.wwww;
    } else {
      r0.z = cmp(0.000600000028 < r0.z);
      if (r0.z != 0) {
        r3.xyzw = float4(0,0,0,0);
        
        // --- 2. -NAN FIX ---
        // 3Dmigoto misreads the assembly "l(0,0,0,-1)" as a -nan float. 
        r0.zw = float2(0, -1);
        // -------------------

        while (true) {
          r4.x = cmp(1 < (int)r0.w);
          if (r4.x != 0) break;
          r4.xz = (int2)r0.ww;
          r5.xyzw = r3.xyzw;
          r6.x = r0.z;
          r6.y = -1;
          while (true) {
            r6.z = cmp(1 < (int)r6.y);
            if (r6.z != 0) break;
            r6.z = (int)r0.w | (int)r6.y;
            r4.yw = (int2)r6.yy;
            r6.w = dot(r4.zw, r4.zw);
            r6.w = sqrt(r6.w);
            r6.w = 1 / r6.w;
            r6.z = r6.z ? r6.w : 1;
            r7.xyzw = r4.xyzw * r1.xyzw + v1.xyxy;
            r8.xyzw = r1.zwzw * float4(-0.757771552,-0.757771552,0.757771552,-0.757771552) + r7.zwzw;
            r9.xyzw = colorTexture.SampleLevel(samLinear_s, r8.xy, r0.y).xyzw;
            r8.xyzw = colorTexture.SampleLevel(samLinear_s, r8.zw, r0.y).xyzw;
            r8.xyzw = float4(0.374875665,0.374875665,0.374875665,0.374875665) * r8.xyzw;
            r8.xyzw = r9.xyzw * float4(0.374875665,0.374875665,0.374875665,0.374875665) + r8.xyzw;
            r9.xyzw = r1.zwzw * float4(0.757771552,0.757771552,-0.757771552,0.757771552) + r7.zwzw;
            r10.xyzw = colorTexture.SampleLevel(samLinear_s, r9.xy, r0.y).xyzw;
            r8.xyzw = r10.xyzw * float4(0.374875665,0.374875665,0.374875665,0.374875665) + r8.xyzw;
            r9.xyzw = colorTexture.SampleLevel(samLinear_s, r9.zw, r0.y).xyzw;
            r8.xyzw = r9.xyzw * float4(0.374875665,0.374875665,0.374875665,0.374875665) + r8.xyzw;
            r9.xyzw = r1.zwzw * float4(-2.90709925,0,2.90709925,0) + r7.zwzw;
            r10.xyzw = colorTexture.SampleLevel(samLinear_s, r9.xy, r0.y).xyzw;
            r8.xyzw = r10.xyzw * float4(-0.124875657,-0.124875657,-0.124875657,-0.124875657) + r8.xyzw;
            r9.xyzw = colorTexture.SampleLevel(samLinear_s, r9.zw, r0.y).xyzw;
            r8.xyzw = r9.xyzw * float4(-0.124875657,-0.124875657,-0.124875657,-0.124875657) + r8.xyzw;
            r7.xyzw = r1.zwzw * float4(0,-2.90709925,0,2.90709925) + r7.xyzw;
            r9.xyzw = colorTexture.SampleLevel(samLinear_s, r7.xy, r0.y).xyzw;
            r8.xyzw = r9.xyzw * float4(-0.124875657,-0.124875657,-0.124875657,-0.124875657) + r8.xyzw;
            r7.xyzw = colorTexture.SampleLevel(samLinear_s, r7.zw, r0.y).xyzw;
            r7.xyzw = r7.xyzw * float4(-0.124875657,-0.124875657,-0.124875657,-0.124875657) + r8.xyzw;
            r7.xyzw = max(float4(0,0,0,0), r7.xyzw);
            r5.xyzw = r7.xyzw * r6.zzzz + r5.xyzw;
            r6.x = r6.x + r6.z;
            r6.y = (int)r6.y + 1;
          }
          r3.xyzw = r5.xyzw;
          r0.z = r6.x;
          r0.w = (int)r0.w + 1;
        }
        r2.xyzw = r3.xyzw / r0.zzzz;
      } else {
        r3.xyzw = r1.zwzw * float4(-0.757771552,-0.757771552,0.757771552,-0.757771552) + v1.xyxy;
        r4.xyzw = colorTexture.SampleLevel(samLinear_s, r3.xy, r0.y).xyzw;
        r3.xyzw = colorTexture.SampleLevel(samLinear_s, r3.zw, r0.y).xyzw;
        r3.xyzw = float4(0.374875665,0.374875665,0.374875665,0.374875665) * r3.xyzw;
        r3.xyzw = r4.xyzw * float4(0.374875665,0.374875665,0.374875665,0.374875665) + r3.xyzw;
        r4.xyzw = r1.zwzw * float4(0.757771552,0.757771552,-0.757771552,0.757771552) + v1.xyxy;
        r5.xyzw = colorTexture.SampleLevel(samLinear_s, r4.xy, r0.y).xyzw;
        r3.xyzw = r5.xyzw * float4(0.374875665,0.374875665,0.374875665,0.374875665) + r3.xyzw;
        r4.xyzw = colorTexture.SampleLevel(samLinear_s, r4.zw, r0.y).xyzw;
        r3.xyzw = r4.xyzw * float4(0.374875665,0.374875665,0.374875665,0.374875665) + r3.xyzw;
        r4.xyzw = r1.zwzw * float4(-2.90709925,0,2.90709925,0) + v1.xyxy;
        r5.xyzw = colorTexture.SampleLevel(samLinear_s, r4.xy, r0.y).xyzw;
        r3.xyzw = r5.xyzw * float4(-0.124875657,-0.124875657,-0.124875657,-0.124875657) + r3.xyzw;
        r4.xyzw = colorTexture.SampleLevel(samLinear_s, r4.zw, r0.y).xyzw;
        r3.xyzw = r4.xyzw * float4(-0.124875657,-0.124875657,-0.124875657,-0.124875657) + r3.xyzw;
        r4.xyzw = r1.zwzw * float4(0,-2.90709925,0,2.90709925) + v1.xyxy;
        r5.xyzw = colorTexture.SampleLevel(samLinear_s, r4.xy, r0.y).xyzw;
        r3.xyzw = r5.xyzw * float4(-0.124875657,-0.124875657,-0.124875657,-0.124875657) + r3.xyzw;
        r4.xyzw = colorTexture.SampleLevel(samLinear_s, r4.zw, r0.y).xyzw;
        r3.xyzw = r4.xyzw * float4(-0.124875657,-0.124875657,-0.124875657,-0.124875657) + r3.xyzw;
        r2.xyzw = max(float4(0,0,0,0), r3.xyzw);
      }
    }
  } else {
    r3.xyzw = r1.zwzw * float4(-0.378885776,-0.378885776,0.378885776,-0.378885776) + v1.xyxy;
    r4.xyzw = colorTexture.SampleLevel(samLinear_s, r3.xy, r0.y).xyzw;
    r3.xyzw = colorTexture.SampleLevel(samLinear_s, r3.zw, r0.y).xyzw;
    r3.xyzw = float4(0.374875665,0.374875665,0.374875665,0.374875665) * r3.xyzw;
    r3.xyzw = r4.xyzw * float4(0.374875665,0.374875665,0.374875665,0.374875665) + r3.xyzw;
    r4.xyzw = r1.zwzw * float4(0.378885776,0.378885776,-0.378885776,0.378885776) + v1.xyxy;
    r5.xyzw = colorTexture.SampleLevel(samLinear_s, r4.xy, r0.y).xyzw;
    r3.xyzw = r5.xyzw * float4(0.374875665,0.374875665,0.374875665,0.374875665) + r3.xyzw;
    r4.xyzw = colorTexture.SampleLevel(samLinear_s, r4.zw, r0.y).xyzw;
    r3.xyzw = r4.xyzw * float4(0.374875665,0.374875665,0.374875665,0.374875665) + r3.xyzw;
    r4.xyzw = r1.zwzw * float4(-1.45354962,0,1.45354962,0) + v1.xyxy;
    r5.xyzw = colorTexture.SampleLevel(samLinear_s, r4.xy, r0.y).xyzw;
    r3.xyzw = r5.xyzw * float4(-0.124875657,-0.124875657,-0.124875657,-0.124875657) + r3.xyzw;
    r4.xyzw = colorTexture.SampleLevel(samLinear_s, r4.zw, r0.y).xyzw;
    r3.xyzw = r4.xyzw * float4(-0.124875657,-0.124875657,-0.124875657,-0.124875657) + r3.xyzw;
    r1.xyzw = r1.xyzw * float4(0,-1.45354962,0,1.45354962) + v1.xyxy;
    r4.xyzw = colorTexture.SampleLevel(samLinear_s, r1.xy, r0.y).xyzw;
    r3.xyzw = r4.xyzw * float4(-0.124875657,-0.124875657,-0.124875657,-0.124875657) + r3.xyzw;
    r1.xyzw = colorTexture.SampleLevel(samLinear_s, r1.zw, r0.y).xyzw;
    r1.xyzw = r1.xyzw * float4(-0.124875657,-0.124875657,-0.124875657,-0.124875657) + r3.xyzw;
    r2.xyzw = max(float4(0,0,0,0), r1.xyzw);
  }
  r1.xyzw = max(float4(0,0,0,0), r2.xyzw);
  if (r0.x != 0) {
    r0.x = v4.x;
    r1.w = dot(r1.xyzw, icb[r0.x+0].xyzw);
    r1.xyz = float3(1,1,1);
  }
  r0.x = dot(r1.xyz, float3(0.298999995,0.587000012,0.114));
  r0.xyz = r0.xxx + -r1.xyz;
  r1.xyz = w1.xxx * r0.xyz + r1.xyz;
  r0.xyzw = v2.xyzw * r1.xyzw;

  // We completely bypass the game's hdr_ui_brightness_g variable,
  float reno_ui_brightness = shader_injection.graphics_white_nits / 170.f;
  r0.xyz = r0.xyz * reno_ui_brightness + v3.xyz;
  // ---------------------------------

  r1.x = r1.w * v2.w + -1;
  r1.x = v1.z * r1.x + 1;
  o0.xyz = r1.xxx * r0.xyz;
  o0.w = r0.w;
  return;
}