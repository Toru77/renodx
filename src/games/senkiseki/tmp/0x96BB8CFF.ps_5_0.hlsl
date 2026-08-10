// ---- Created with 3Dmigoto v1.3.16 on Wed Sep 03 17:47:27 2025
#include "../cs4/common.hlsl"
SamplerState LinearClampSampler_s : register(s0);
Texture2D<float4> ColorBuffer : register(t0);


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_POSITION0,
  float2 v1 : TEXCOORD0,
  out float4 o0 : SV_TARGET0)
{
  float4 r0,r1,r2,r3,r4,r5;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.xyzw = ColorBuffer.SampleLevel(LinearClampSampler_s, v1.xy, 0).xyzw;
  r0.rgb = decodeColor(r0.rgb);
  r1.xyz = ColorBuffer.Gather(LinearClampSampler_s, v1.xy).xyz;
  r1.rgb = decodeColor(r1.rgb);
  r2.xyz = ColorBuffer.Gather(LinearClampSampler_s, v1.xy, int2(-1, -1)).xzw;
  // r2.rgb = decodeColor(r2.rgb);
  r1.w = max(r1.x, r0.w);
  r2.w = min(r1.x, r0.w);
  r1.w = max(r1.z, r1.w);
  r2.w = min(r2.w, r1.z);
  r3.x = max(r2.y, r2.x);
  r3.y = min(r2.y, r2.x);
  r1.w = max(r3.x, r1.w);
  r2.w = min(r3.y, r2.w);
  r3.x = 0.100000001 * r1.w;
  r1.w = -r2.w + r1.w;
  r2.w = max(0.0833000019, r3.x);
  r2.w = cmp(r1.w >= r2.w);
  if (r2.w != 0) {
    ColorBuffer.GetDimensions(0, fDest.x, fDest.y, fDest.z);
    r3.xy = fDest.xy;
    r3.xy = float2(1,1) / r3.xy;
    r2.w = ColorBuffer.SampleLevel(LinearClampSampler_s, v1.xy, 0, int2(1, -1)).w;
    r3.z = ColorBuffer.SampleLevel(LinearClampSampler_s, v1.xy, 0, int2(-1, 1)).w;
    r4.xy = r2.yx + r1.xz;
    r1.w = 1 / r1.w;
    r3.w = r4.x + r4.y;
    r4.xy = r0.ww * float2(-2,-2) + r4.xy;
    r4.z = r2.w + r1.y;
    r2.w = r2.z + r2.w;
    r4.w = r1.z * -2 + r4.z;
    r2.w = r2.y * -2 + r2.w;
    r2.z = r3.z + r2.z;
    r1.y = r3.z + r1.y;
    r3.z = abs(r4.x) * 2 + abs(r4.w);
    r2.w = abs(r4.y) * 2 + abs(r2.w);
    r4.x = r2.x * -2 + r2.z;
    r1.y = r1.x * -2 + r1.y;
    r3.z = abs(r4.x) + r3.z;
    r1.y = abs(r1.y) + r2.w;
    r2.z = r2.z + r4.z;
    r1.y = cmp(r3.z >= r1.y);
    r2.z = r3.w * 2 + r2.z;
    r2.x = r1.y ? r2.y : r2.x;
    r1.x = r1.y ? r1.x : r1.z;
    r1.z = r1.y ? r3.y : r3.x;
    r2.y = r2.z * 0.0833333358 + -r0.w;
    r2.z = r2.x + -r0.w;
    r2.w = r1.x + -r0.w;
    r2.x = r2.x + r0.w;
    r1.x = r1.x + r0.w;
    r3.z = cmp(abs(r2.z) >= abs(r2.w));
    r2.z = max(abs(r2.z), abs(r2.w));
    r1.z = r3.z ? -r1.z : r1.z;
    r1.w = saturate(abs(r2.y) * r1.w);
    r2.y = r1.y ? r3.x : 0;
    r2.w = r1.y ? 0 : r3.y;
    r3.xy = r1.zz * float2(0.5,0.5) + v1.xy;
    r3.x = r1.y ? v1.x : r3.x;
    r3.y = r1.y ? r3.y : v1.y;
    r4.xy = r3.xy + -r2.yw;
    r5.xy = r3.xy + r2.yw;
    r3.x = r1.w * -2 + 3;
    r3.y = ColorBuffer.SampleLevel(LinearClampSampler_s, r4.xy, 0).w;
    r1.w = r1.w * r1.w;
    r3.w = ColorBuffer.SampleLevel(LinearClampSampler_s, r5.xy, 0).w;
    r1.x = r3.z ? r2.x : r1.x;
    r2.x = 0.25 * r2.z;
    r0.w = -r1.x * 0.5 + r0.w;
    r1.w = r3.x * r1.w;
    r0.w = cmp(r0.w < 0);
    r3.x = -r1.x * 0.5 + r3.y;
    r3.y = -r1.x * 0.5 + r3.w;
    r3.zw = cmp(abs(r3.xy) >= r2.xx);
    r2.z = -r2.y * 1.5 + r4.x;
    r4.x = r3.z ? r4.x : r2.z;
    r2.z = -r2.w * 1.5 + r4.y;
    r4.z = r3.z ? r4.y : r2.z;
    r4.yw = ~(int2)r3.zw;
    r2.z = (int)r4.w | (int)r4.y;
    r4.y = r2.y * 1.5 + r5.x;
    r4.y = r3.w ? r5.x : r4.y;
    r5.x = r2.w * 1.5 + r5.y;
    r4.w = r3.w ? r5.y : r5.x;
    if (r2.z != 0) {
      if (r3.z == 0) {
        r3.x = ColorBuffer.SampleLevel(LinearClampSampler_s, r4.xz, 0).w;
      }
      if (r3.w == 0) {
        r3.y = ColorBuffer.SampleLevel(LinearClampSampler_s, r4.yw, 0).w;
      }
      r2.z = -r1.x * 0.5 + r3.x;
      r3.x = r3.z ? r3.x : r2.z;
      r2.z = -r1.x * 0.5 + r3.y;
      r3.y = r3.w ? r3.y : r2.z;
      r3.zw = cmp(abs(r3.xy) >= r2.xx);
      r2.z = -r2.y * 2 + r4.x;
      r4.x = r3.z ? r4.x : r2.z;
      r2.z = -r2.w * 2 + r4.z;
      r4.z = r3.z ? r4.z : r2.z;
      r5.xy = ~(int2)r3.zw;
      r2.z = (int)r5.y | (int)r5.x;
      r5.x = r2.y * 2 + r4.y;
      r4.y = r3.w ? r4.y : r5.x;
      r5.x = r2.w * 2 + r4.w;
      r4.w = r3.w ? r4.w : r5.x;
      if (r2.z != 0) {
        if (r3.z == 0) {
          r3.x = ColorBuffer.SampleLevel(LinearClampSampler_s, r4.xz, 0).w;
        }
        if (r3.w == 0) {
          r3.y = ColorBuffer.SampleLevel(LinearClampSampler_s, r4.yw, 0).w;
        }
        r2.z = -r1.x * 0.5 + r3.x;
        r3.x = r3.z ? r3.x : r2.z;
        r2.z = -r1.x * 0.5 + r3.y;
        r3.y = r3.w ? r3.y : r2.z;
        r3.zw = cmp(abs(r3.xy) >= r2.xx);
        r2.z = -r2.y * 2 + r4.x;
        r4.x = r3.z ? r4.x : r2.z;
        r2.z = -r2.w * 2 + r4.z;
        r4.z = r3.z ? r4.z : r2.z;
        r5.xy = ~(int2)r3.zw;
        r2.z = (int)r5.y | (int)r5.x;
        r5.x = r2.y * 2 + r4.y;
        r4.y = r3.w ? r4.y : r5.x;
        r5.x = r2.w * 2 + r4.w;
        r4.w = r3.w ? r4.w : r5.x;
        if (r2.z != 0) {
          if (r3.z == 0) {
            r3.x = ColorBuffer.SampleLevel(LinearClampSampler_s, r4.xz, 0).w;
          }
          if (r3.w == 0) {
            r3.y = ColorBuffer.SampleLevel(LinearClampSampler_s, r4.yw, 0).w;
          }
          r2.z = -r1.x * 0.5 + r3.x;
          r3.x = r3.z ? r3.x : r2.z;
          r2.z = -r1.x * 0.5 + r3.y;
          r3.y = r3.w ? r3.y : r2.z;
          r3.zw = cmp(abs(r3.xy) >= r2.xx);
          r2.z = -r2.y * 2 + r4.x;
          r4.x = r3.z ? r4.x : r2.z;
          r2.z = -r2.w * 2 + r4.z;
          r4.z = r3.z ? r4.z : r2.z;
          r5.xy = ~(int2)r3.zw;
          r2.z = (int)r5.y | (int)r5.x;
          r5.x = r2.y * 2 + r4.y;
          r4.y = r3.w ? r4.y : r5.x;
          r5.x = r2.w * 2 + r4.w;
          r4.w = r3.w ? r4.w : r5.x;
          if (r2.z != 0) {
            if (r3.z == 0) {
              r3.x = ColorBuffer.SampleLevel(LinearClampSampler_s, r4.xz, 0).w;
            }
            if (r3.w == 0) {
              r3.y = ColorBuffer.SampleLevel(LinearClampSampler_s, r4.yw, 0).w;
            }
            r2.z = -r1.x * 0.5 + r3.x;
            r3.x = r3.z ? r3.x : r2.z;
            r2.z = -r1.x * 0.5 + r3.y;
            r3.y = r3.w ? r3.y : r2.z;
            r3.zw = cmp(abs(r3.xy) >= r2.xx);
            r2.z = -r2.y * 2 + r4.x;
            r4.x = r3.z ? r4.x : r2.z;
            r2.z = -r2.w * 2 + r4.z;
            r4.z = r3.z ? r4.z : r2.z;
            r5.xy = ~(int2)r3.zw;
            r2.z = (int)r5.y | (int)r5.x;
            r5.x = r2.y * 2 + r4.y;
            r4.y = r3.w ? r4.y : r5.x;
            r5.x = r2.w * 2 + r4.w;
            r4.w = r3.w ? r4.w : r5.x;
            if (r2.z != 0) {
              if (r3.z == 0) {
                r3.x = ColorBuffer.SampleLevel(LinearClampSampler_s, r4.xz, 0).w;
              }
              if (r3.w == 0) {
                r3.y = ColorBuffer.SampleLevel(LinearClampSampler_s, r4.yw, 0).w;
              }
              r2.z = -r1.x * 0.5 + r3.x;
              r3.x = r3.z ? r3.x : r2.z;
              r2.z = -r1.x * 0.5 + r3.y;
              r3.y = r3.w ? r3.y : r2.z;
              r3.zw = cmp(abs(r3.xy) >= r2.xx);
              r2.z = -r2.y * 2 + r4.x;
              r4.x = r3.z ? r4.x : r2.z;
              r2.z = -r2.w * 2 + r4.z;
              r4.z = r3.z ? r4.z : r2.z;
              r5.xy = ~(int2)r3.zw;
              r2.z = (int)r5.y | (int)r5.x;
              r5.x = r2.y * 2 + r4.y;
              r4.y = r3.w ? r4.y : r5.x;
              r5.x = r2.w * 2 + r4.w;
              r4.w = r3.w ? r4.w : r5.x;
              if (r2.z != 0) {
                if (r3.z == 0) {
                  r3.x = ColorBuffer.SampleLevel(LinearClampSampler_s, r4.xz, 0).w;
                }
                if (r3.w == 0) {
                  r3.y = ColorBuffer.SampleLevel(LinearClampSampler_s, r4.yw, 0).w;
                }
                r2.z = -r1.x * 0.5 + r3.x;
                r3.x = r3.z ? r3.x : r2.z;
                r2.z = -r1.x * 0.5 + r3.y;
                r3.y = r3.w ? r3.y : r2.z;
                r3.zw = cmp(abs(r3.xy) >= r2.xx);
                r2.z = -r2.y * 2 + r4.x;
                r4.x = r3.z ? r4.x : r2.z;
                r2.z = -r2.w * 2 + r4.z;
                r4.z = r3.z ? r4.z : r2.z;
                r5.xy = ~(int2)r3.zw;
                r2.z = (int)r5.y | (int)r5.x;
                r5.x = r2.y * 2 + r4.y;
                r4.y = r3.w ? r4.y : r5.x;
                r5.x = r2.w * 2 + r4.w;
                r4.w = r3.w ? r4.w : r5.x;
                if (r2.z != 0) {
                  if (r3.z == 0) {
                    r3.x = ColorBuffer.SampleLevel(LinearClampSampler_s, r4.xz, 0).w;
                  }
                  if (r3.w == 0) {
                    r3.y = ColorBuffer.SampleLevel(LinearClampSampler_s, r4.yw, 0).w;
                  }
                  r2.z = -r1.x * 0.5 + r3.x;
                  r3.x = r3.z ? r3.x : r2.z;
                  r2.z = -r1.x * 0.5 + r3.y;
                  r3.y = r3.w ? r3.y : r2.z;
                  r3.zw = cmp(abs(r3.xy) >= r2.xx);
                  r2.z = -r2.y * 2 + r4.x;
                  r4.x = r3.z ? r4.x : r2.z;
                  r2.z = -r2.w * 2 + r4.z;
                  r4.z = r3.z ? r4.z : r2.z;
                  r5.xy = ~(int2)r3.zw;
                  r2.z = (int)r5.y | (int)r5.x;
                  r5.x = r2.y * 2 + r4.y;
                  r4.y = r3.w ? r4.y : r5.x;
                  r5.x = r2.w * 2 + r4.w;
                  r4.w = r3.w ? r4.w : r5.x;
                  if (r2.z != 0) {
                    if (r3.z == 0) {
                      r3.x = ColorBuffer.SampleLevel(LinearClampSampler_s, r4.xz, 0).w;
                    }
                    if (r3.w == 0) {
                      r3.y = ColorBuffer.SampleLevel(LinearClampSampler_s, r4.yw, 0).w;
                    }
                    r2.z = -r1.x * 0.5 + r3.x;
                    r3.x = r3.z ? r3.x : r2.z;
                    r2.z = -r1.x * 0.5 + r3.y;
                    r3.y = r3.w ? r3.y : r2.z;
                    r3.zw = cmp(abs(r3.xy) >= r2.xx);
                    r2.z = -r2.y * 2 + r4.x;
                    r4.x = r3.z ? r4.x : r2.z;
                    r2.z = -r2.w * 2 + r4.z;
                    r4.z = r3.z ? r4.z : r2.z;
                    r5.xy = ~(int2)r3.zw;
                    r2.z = (int)r5.y | (int)r5.x;
                    r5.x = r2.y * 2 + r4.y;
                    r4.y = r3.w ? r4.y : r5.x;
                    r5.x = r2.w * 2 + r4.w;
                    r4.w = r3.w ? r4.w : r5.x;
                    if (r2.z != 0) {
                      if (r3.z == 0) {
                        r3.x = ColorBuffer.SampleLevel(LinearClampSampler_s, r4.xz, 0).w;
                      }
                      if (r3.w == 0) {
                        r3.y = ColorBuffer.SampleLevel(LinearClampSampler_s, r4.yw, 0).w;
                      }
                      r2.z = -r1.x * 0.5 + r3.x;
                      r3.x = r3.z ? r3.x : r2.z;
                      r2.z = -r1.x * 0.5 + r3.y;
                      r3.y = r3.w ? r3.y : r2.z;
                      r3.zw = cmp(abs(r3.xy) >= r2.xx);
                      r2.z = -r2.y * 4 + r4.x;
                      r4.x = r3.z ? r4.x : r2.z;
                      r2.z = -r2.w * 4 + r4.z;
                      r4.z = r3.z ? r4.z : r2.z;
                      r5.xy = ~(int2)r3.zw;
                      r2.z = (int)r5.y | (int)r5.x;
                      r5.x = r2.y * 4 + r4.y;
                      r4.y = r3.w ? r4.y : r5.x;
                      r5.x = r2.w * 4 + r4.w;
                      r4.w = r3.w ? r4.w : r5.x;
                      if (r2.z != 0) {
                        if (r3.z == 0) {
                          r3.x = ColorBuffer.SampleLevel(LinearClampSampler_s, r4.xz, 0).w;
                        }
                        if (r3.w == 0) {
                          r3.y = ColorBuffer.SampleLevel(LinearClampSampler_s, r4.yw, 0).w;
                        }
                        r2.z = -r1.x * 0.5 + r3.x;
                        r3.x = r3.z ? r3.x : r2.z;
                        r1.x = -r1.x * 0.5 + r3.y;
                        r3.y = r3.w ? r3.y : r1.x;
                        r2.xz = cmp(abs(r3.xy) >= r2.xx);
                        r1.x = -r2.y * 8 + r4.x;
                        r4.x = r2.x ? r4.x : r1.x;
                        r1.x = -r2.w * 8 + r4.z;
                        r4.z = r2.x ? r4.z : r1.x;
                        r1.x = r2.y * 8 + r4.y;
                        r4.y = r2.z ? r4.y : r1.x;
                        r1.x = r2.w * 8 + r4.w;
                        r4.w = r2.z ? r4.w : r1.x;
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
    r1.x = v1.x + -r4.x;
    r2.y = v1.y + -r4.z;
    r1.x = r1.y ? r1.x : r2.y;
    r2.xy = -v1.xy + r4.yw;
    r2.x = r1.y ? r2.x : r2.y;
    r2.yz = cmp(r3.xy < float2(0,0));
    r2.w = r2.x + r1.x;
    r2.yz = cmp((int2)r0.ww != (int2)r2.yz);
    r0.w = 1 / r2.w;
    r2.w = cmp(r1.x < r2.x);
    r1.x = min(r2.x, r1.x);
    r2.x = r2.w ? r2.y : r2.z;
    r1.w = r1.w * r1.w;
    r0.w = r1.x * -r0.w + 0.5;
    r1.x = 0.25 * r1.w;
    r0.w = (int)r0.w & (int)r2.x;
    r0.w = max(r0.w, r1.x);
    r1.xz = r0.ww * r1.zz + v1.xy;
    r2.x = r1.y ? v1.x : r1.x;
    r2.y = r1.y ? r1.z : v1.y;
    r0.xyz = ColorBuffer.SampleLevel(LinearClampSampler_s, r2.xy, 0).xyz;
    r0.rgb = decodeColor(r0.rgb);
  }
  o0.xyz = r0.xyz;

  o0.rgb = encodeColor(o0.rgb);
  o0.w = 1;
  return;
}