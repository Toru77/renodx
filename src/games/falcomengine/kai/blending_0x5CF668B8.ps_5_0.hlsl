// ---- Created with 3Dmigoto v1.3.16 on Sun May 10 19:11:10 2026

SamplerState sam0_s : register(s0);
Texture2D<float4> colorTexture : register(t0);
Texture2D<float4> preTransparentPassTexture : register(t1);


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_Position0,
  float4 v1 : TEXCOORD0,
  out float4 o0 : SV_Target0)
{
  float4 r0,r1,r2,r3,r4,r5;
  uint4 bitmask, uiDest;
  float4 fDest;

  colorTexture.GetDimensions(0, uiDest.x, uiDest.y, uiDest.z);
  r0.xy = uiDest.xy;
  r0.xy = (uint2)r0.xy;
  r0.xy = float2(1,1) / r0.xy;
  r0.zw = v1.xy + -r0.xy;
  r1.xyz = preTransparentPassTexture.Sample(sam0_s, r0.zw).xyz;
  r2.xyz = colorTexture.Sample(sam0_s, r0.zw).xyz;
  r1.xyz = -r2.xyz + r1.xyz;
  r1.xyz = cmp(float3(0.0500000007,0.0500000007,0.0500000007) < abs(r1.xyz));
  r0.z = (int)r1.y | (int)r1.x;
  r0.z = (int)r1.z | (int)r0.z;
  r1.xyzw = r0.xyxy * float4(0,-1,1,-1) + v1.xyxy;
  r2.xyz = preTransparentPassTexture.Sample(sam0_s, r1.xy).xyz;
  r3.xyz = colorTexture.Sample(sam0_s, r1.xy).xyz;
  r2.xyz = -r3.xyz + r2.xyz;
  r2.xyz = cmp(float3(0.0500000007,0.0500000007,0.0500000007) < abs(r2.xyz));
  r0.w = (int)r2.y | (int)r2.x;
  r0.w = (int)r2.z | (int)r0.w;
  r0.z = (int)r0.z | (int)r0.w;
  r2.xyz = preTransparentPassTexture.Sample(sam0_s, r1.zw).xyz;
  r1.xyz = colorTexture.Sample(sam0_s, r1.zw).xyz;
  r1.xyz = r2.xyz + -r1.xyz;
  r1.xyz = cmp(float3(0.0500000007,0.0500000007,0.0500000007) < abs(r1.xyz));
  r0.w = (int)r1.y | (int)r1.x;
  r0.w = (int)r1.z | (int)r0.w;
  r0.z = (int)r0.z | (int)r0.w;
  r1.xyzw = r0.xyxy * float4(-1,0,1,0) + v1.xyxy;
  r2.xyz = preTransparentPassTexture.Sample(sam0_s, r1.xy).xyz;
  r3.xyz = colorTexture.Sample(sam0_s, r1.xy).xyz;
  r2.xyz = -r3.xyz + r2.xyz;
  r2.xyz = cmp(float3(0.0500000007,0.0500000007,0.0500000007) < abs(r2.xyz));
  r0.w = (int)r2.y | (int)r2.x;
  r0.w = (int)r2.z | (int)r0.w;
  r0.z = (int)r0.z | (int)r0.w;
  r2.xyz = preTransparentPassTexture.Sample(sam0_s, v1.xy).xyz;
  r3.xyz = colorTexture.Sample(sam0_s, v1.xy).xyz;
  r2.xyz = -r3.xyz + r2.xyz;
  r2.xyz = cmp(float3(0.0500000007,0.0500000007,0.0500000007) < abs(r2.xyz));
  r0.w = (int)r2.y | (int)r2.x;
  r0.w = (int)r2.z | (int)r0.w;
  r0.z = (int)r0.z | (int)r0.w;
  r2.xyz = preTransparentPassTexture.Sample(sam0_s, r1.zw).xyz;
  r1.xyz = colorTexture.Sample(sam0_s, r1.zw).xyz;
  r1.xyz = r2.xyz + -r1.xyz;
  r1.xyz = cmp(float3(0.0500000007,0.0500000007,0.0500000007) < abs(r1.xyz));
  r0.w = (int)r1.y | (int)r1.x;
  r0.w = (int)r1.z | (int)r0.w;
  r0.z = (int)r0.z | (int)r0.w;
  r1.xyzw = r0.xyxy * float4(-1,1,0,1) + v1.xyxy;
  r2.xyz = preTransparentPassTexture.Sample(sam0_s, r1.xy).xyz;
  r3.xyz = colorTexture.Sample(sam0_s, r1.xy).xyz;
  r2.xyz = -r3.xyz + r2.xyz;
  r2.xyz = cmp(float3(0.0500000007,0.0500000007,0.0500000007) < abs(r2.xyz));
  r0.w = (int)r2.y | (int)r2.x;
  r0.w = (int)r2.z | (int)r0.w;
  r0.z = (int)r0.z | (int)r0.w;
  r2.xyz = preTransparentPassTexture.Sample(sam0_s, r1.zw).xyz;
  r1.xyz = colorTexture.Sample(sam0_s, r1.zw).xyz;
  r1.xyz = r2.xyz + -r1.xyz;
  r1.xyz = cmp(float3(0.0500000007,0.0500000007,0.0500000007) < abs(r1.xyz));
  r0.w = (int)r1.y | (int)r1.x;
  r0.w = (int)r1.z | (int)r0.w;
  r0.z = (int)r0.z | (int)r0.w;
  r1.xy = v1.xy + r0.xy;
  r2.xyz = preTransparentPassTexture.Sample(sam0_s, r1.xy).xyz;
  r1.xyz = colorTexture.Sample(sam0_s, r1.xy).xyz;
  r1.xyz = r2.xyz + -r1.xyz;
  r1.xyz = cmp(float3(0.0500000007,0.0500000007,0.0500000007) < abs(r1.xyz));
  r0.w = (int)r1.y | (int)r1.x;
  r0.w = (int)r1.z | (int)r0.w;
  r0.z = (int)r0.z | (int)r0.w;
  if (r0.z == 0) {
    if (-1 != 0) discard;
  }
  r1.xyzw = colorTexture.SampleLevel(sam0_s, v1.xy, 0).xyzw;
  r2.xyz = colorTexture.Gather(sam0_s, v1.xy).xyz;
  r3.xyz = colorTexture.Gather(sam0_s, v1.xy, int2(-1, -1)).xzw;
  r0.z = max(r2.x, r1.w);
  r0.w = min(r2.x, r1.w);
  r0.z = max(r2.z, r0.z);
  r0.w = min(r2.z, r0.w);
  r2.w = max(r3.y, r3.x);
  r3.w = min(r3.y, r3.x);
  r0.z = max(r2.w, r0.z);
  r0.w = min(r3.w, r0.w);
  r2.w = 0.125 * r0.z;
  r0.z = r0.z + -r0.w;
  r0.w = max(0.0311999992, r2.w);
  r0.w = cmp(r0.z >= r0.w);
  if (r0.w != 0) {
    r0.w = colorTexture.SampleLevel(sam0_s, v1.xy, 0, int2(1, -1)).w;
    r2.w = colorTexture.SampleLevel(sam0_s, v1.xy, 0, int2(-1, 1)).w;
    r4.xy = r3.yx + r2.xz;
    r0.z = 1 / r0.z;
    r3.w = r4.x + r4.y;
    r4.xy = r1.ww * float2(-2,-2) + r4.xy;
    r4.z = r0.w + r2.y;
    r0.w = r3.z + r0.w;
    r4.w = r2.z * -2 + r4.z;
    r0.w = r3.y * -2 + r0.w;
    r3.z = r3.z + r2.w;
    r2.y = r2.w + r2.y;
    r2.w = abs(r4.x) * 2 + abs(r4.w);
    r0.w = abs(r4.y) * 2 + abs(r0.w);
    r4.x = r3.x * -2 + r3.z;
    r2.y = r2.x * -2 + r2.y;
    r2.w = abs(r4.x) + r2.w;
    r0.w = abs(r2.y) + r0.w;
    r2.y = r3.z + r4.z;
    r0.w = cmp(r2.w >= r0.w);
    r2.y = r3.w * 2 + r2.y;
    r2.w = r0.w ? r3.y : r3.x;
    r2.x = r0.w ? r2.x : r2.z;
    r2.z = r0.w ? r0.y : r0.x;
    r2.y = r2.y * 0.0833333358 + -r1.w;
    r3.xy = r2.wx + -r1.ww;
    r2.xw = r2.xw + r1.ww;
    r3.z = cmp(abs(r3.x) >= abs(r3.y));
    r3.x = max(abs(r3.x), abs(r3.y));
    r2.z = r3.z ? -r2.z : r2.z;
    r0.z = saturate(abs(r2.y) * r0.z);
    r0.x = r0.w ? r0.x : 0;
    r0.y = r0.w ? 0 : r0.y;
    r3.yw = r2.zz * float2(0.5,0.5) + v1.xy;
    r2.y = r0.w ? v1.x : r3.y;
    r3.y = r0.w ? r3.w : v1.y;
    r4.x = r2.y + -r0.x;
    r4.y = r3.y + -r0.y;
    r5.x = r2.y + r0.x;
    r5.y = r3.y + r0.y;
    r2.y = r0.z * -2 + 3;
    r3.y = colorTexture.SampleLevel(sam0_s, r4.xy, 0).w;
    r0.z = r0.z * r0.z;
    r3.w = colorTexture.SampleLevel(sam0_s, r5.xy, 0).w;
    r2.x = r3.z ? r2.w : r2.x;
    r2.w = 0.25 * r3.x;
    r3.x = -r2.x * 0.5 + r1.w;
    r0.z = r2.y * r0.z;
    r2.y = cmp(r3.x < 0);
    r3.x = -r2.x * 0.5 + r3.y;
    r3.y = -r2.x * 0.5 + r3.w;
    r3.zw = cmp(abs(r3.xy) >= r2.ww);
    r4.z = -r0.x * 1.5 + r4.x;
    r4.x = r3.z ? r4.x : r4.z;
    r4.w = -r0.y * 1.5 + r4.y;
    r4.z = r3.z ? r4.y : r4.w;
    r4.yw = ~(int2)r3.zw;
    r4.y = (int)r4.w | (int)r4.y;
    r4.w = r0.x * 1.5 + r5.x;
    r5.x = r3.w ? r5.x : r4.w;
    r4.w = r0.y * 1.5 + r5.y;
    r5.z = r3.w ? r5.y : r4.w;
    if (r4.y != 0) {
      if (r3.z == 0) {
        r3.x = colorTexture.SampleLevel(sam0_s, r4.xz, 0).w;
      }
      if (r3.w == 0) {
        r3.y = colorTexture.SampleLevel(sam0_s, r5.xz, 0).w;
      }
      r4.y = -r2.x * 0.5 + r3.x;
      r3.x = r3.z ? r3.x : r4.y;
      r3.z = -r2.x * 0.5 + r3.y;
      r3.y = r3.w ? r3.y : r3.z;
      r3.zw = cmp(abs(r3.xy) >= r2.ww);
      r4.y = -r0.x * 2 + r4.x;
      r4.x = r3.z ? r4.x : r4.y;
      r4.y = -r0.y * 2 + r4.z;
      r4.z = r3.z ? r4.z : r4.y;
      r4.yw = ~(int2)r3.zw;
      r4.y = (int)r4.w | (int)r4.y;
      r4.w = r0.x * 2 + r5.x;
      r5.x = r3.w ? r5.x : r4.w;
      r4.w = r0.y * 2 + r5.z;
      r5.z = r3.w ? r5.z : r4.w;
      if (r4.y != 0) {
        if (r3.z == 0) {
          r3.x = colorTexture.SampleLevel(sam0_s, r4.xz, 0).w;
        }
        if (r3.w == 0) {
          r3.y = colorTexture.SampleLevel(sam0_s, r5.xz, 0).w;
        }
        r4.y = -r2.x * 0.5 + r3.x;
        r3.x = r3.z ? r3.x : r4.y;
        r3.z = -r2.x * 0.5 + r3.y;
        r3.y = r3.w ? r3.y : r3.z;
        r3.zw = cmp(abs(r3.xy) >= r2.ww);
        r4.y = -r0.x * 2 + r4.x;
        r4.x = r3.z ? r4.x : r4.y;
        r4.y = -r0.y * 2 + r4.z;
        r4.z = r3.z ? r4.z : r4.y;
        r4.yw = ~(int2)r3.zw;
        r4.y = (int)r4.w | (int)r4.y;
        r4.w = r0.x * 2 + r5.x;
        r5.x = r3.w ? r5.x : r4.w;
        r4.w = r0.y * 2 + r5.z;
        r5.z = r3.w ? r5.z : r4.w;
        if (r4.y != 0) {
          if (r3.z == 0) {
            r3.x = colorTexture.SampleLevel(sam0_s, r4.xz, 0).w;
          }
          if (r3.w == 0) {
            r3.y = colorTexture.SampleLevel(sam0_s, r5.xz, 0).w;
          }
          r4.y = -r2.x * 0.5 + r3.x;
          r3.x = r3.z ? r3.x : r4.y;
          r3.z = -r2.x * 0.5 + r3.y;
          r3.y = r3.w ? r3.y : r3.z;
          r3.zw = cmp(abs(r3.xy) >= r2.ww);
          r4.y = -r0.x * 2 + r4.x;
          r4.x = r3.z ? r4.x : r4.y;
          r4.y = -r0.y * 2 + r4.z;
          r4.z = r3.z ? r4.z : r4.y;
          r4.yw = ~(int2)r3.zw;
          r4.y = (int)r4.w | (int)r4.y;
          r4.w = r0.x * 2 + r5.x;
          r5.x = r3.w ? r5.x : r4.w;
          r4.w = r0.y * 2 + r5.z;
          r5.z = r3.w ? r5.z : r4.w;
          if (r4.y != 0) {
            if (r3.z == 0) {
              r3.x = colorTexture.SampleLevel(sam0_s, r4.xz, 0).w;
            }
            if (r3.w == 0) {
              r3.y = colorTexture.SampleLevel(sam0_s, r5.xz, 0).w;
            }
            r4.y = -r2.x * 0.5 + r3.x;
            r3.x = r3.z ? r3.x : r4.y;
            r3.z = -r2.x * 0.5 + r3.y;
            r3.y = r3.w ? r3.y : r3.z;
            r3.zw = cmp(abs(r3.xy) >= r2.ww);
            r4.y = -r0.x * 2 + r4.x;
            r4.x = r3.z ? r4.x : r4.y;
            r4.y = -r0.y * 2 + r4.z;
            r4.z = r3.z ? r4.z : r4.y;
            r4.yw = ~(int2)r3.zw;
            r4.y = (int)r4.w | (int)r4.y;
            r4.w = r0.x * 2 + r5.x;
            r5.x = r3.w ? r5.x : r4.w;
            r4.w = r0.y * 2 + r5.z;
            r5.z = r3.w ? r5.z : r4.w;
            if (r4.y != 0) {
              if (r3.z == 0) {
                r3.x = colorTexture.SampleLevel(sam0_s, r4.xz, 0).w;
              }
              if (r3.w == 0) {
                r3.y = colorTexture.SampleLevel(sam0_s, r5.xz, 0).w;
              }
              r4.y = -r2.x * 0.5 + r3.x;
              r3.x = r3.z ? r3.x : r4.y;
              r3.z = -r2.x * 0.5 + r3.y;
              r3.y = r3.w ? r3.y : r3.z;
              r3.zw = cmp(abs(r3.xy) >= r2.ww);
              r4.y = -r0.x * 2 + r4.x;
              r4.x = r3.z ? r4.x : r4.y;
              r4.y = -r0.y * 2 + r4.z;
              r4.z = r3.z ? r4.z : r4.y;
              r4.yw = ~(int2)r3.zw;
              r4.y = (int)r4.w | (int)r4.y;
              r4.w = r0.x * 2 + r5.x;
              r5.x = r3.w ? r5.x : r4.w;
              r4.w = r0.y * 2 + r5.z;
              r5.z = r3.w ? r5.z : r4.w;
              if (r4.y != 0) {
                if (r3.z == 0) {
                  r3.x = colorTexture.SampleLevel(sam0_s, r4.xz, 0).w;
                }
                if (r3.w == 0) {
                  r3.y = colorTexture.SampleLevel(sam0_s, r5.xz, 0).w;
                }
                r4.y = -r2.x * 0.5 + r3.x;
                r3.x = r3.z ? r3.x : r4.y;
                r3.z = -r2.x * 0.5 + r3.y;
                r3.y = r3.w ? r3.y : r3.z;
                r3.zw = cmp(abs(r3.xy) >= r2.ww);
                r4.y = -r0.x * 2 + r4.x;
                r4.x = r3.z ? r4.x : r4.y;
                r4.y = -r0.y * 2 + r4.z;
                r4.z = r3.z ? r4.z : r4.y;
                r4.yw = ~(int2)r3.zw;
                r4.y = (int)r4.w | (int)r4.y;
                r4.w = r0.x * 2 + r5.x;
                r5.x = r3.w ? r5.x : r4.w;
                r4.w = r0.y * 2 + r5.z;
                r5.z = r3.w ? r5.z : r4.w;
                if (r4.y != 0) {
                  if (r3.z == 0) {
                    r3.x = colorTexture.SampleLevel(sam0_s, r4.xz, 0).w;
                  }
                  if (r3.w == 0) {
                    r3.y = colorTexture.SampleLevel(sam0_s, r5.xz, 0).w;
                  }
                  r4.y = -r2.x * 0.5 + r3.x;
                  r3.x = r3.z ? r3.x : r4.y;
                  r3.z = -r2.x * 0.5 + r3.y;
                  r3.y = r3.w ? r3.y : r3.z;
                  r3.zw = cmp(abs(r3.xy) >= r2.ww);
                  r4.y = -r0.x * 2 + r4.x;
                  r4.x = r3.z ? r4.x : r4.y;
                  r4.y = -r0.y * 2 + r4.z;
                  r4.z = r3.z ? r4.z : r4.y;
                  r4.yw = ~(int2)r3.zw;
                  r4.y = (int)r4.w | (int)r4.y;
                  r4.w = r0.x * 2 + r5.x;
                  r5.x = r3.w ? r5.x : r4.w;
                  r4.w = r0.y * 2 + r5.z;
                  r5.z = r3.w ? r5.z : r4.w;
                  if (r4.y != 0) {
                    if (r3.z == 0) {
                      r3.x = colorTexture.SampleLevel(sam0_s, r4.xz, 0).w;
                    }
                    if (r3.w == 0) {
                      r3.y = colorTexture.SampleLevel(sam0_s, r5.xz, 0).w;
                    }
                    r4.y = -r2.x * 0.5 + r3.x;
                    r3.x = r3.z ? r3.x : r4.y;
                    r3.z = -r2.x * 0.5 + r3.y;
                    r3.y = r3.w ? r3.y : r3.z;
                    r3.zw = cmp(abs(r3.xy) >= r2.ww);
                    r4.y = -r0.x * 2 + r4.x;
                    r4.x = r3.z ? r4.x : r4.y;
                    r4.y = -r0.y * 2 + r4.z;
                    r4.z = r3.z ? r4.z : r4.y;
                    r4.yw = ~(int2)r3.zw;
                    r4.y = (int)r4.w | (int)r4.y;
                    r4.w = r0.x * 2 + r5.x;
                    r5.x = r3.w ? r5.x : r4.w;
                    r4.w = r0.y * 2 + r5.z;
                    r5.z = r3.w ? r5.z : r4.w;
                    if (r4.y != 0) {
                      if (r3.z == 0) {
                        r3.x = colorTexture.SampleLevel(sam0_s, r4.xz, 0).w;
                      }
                      if (r3.w == 0) {
                        r3.y = colorTexture.SampleLevel(sam0_s, r5.xz, 0).w;
                      }
                      r4.y = -r2.x * 0.5 + r3.x;
                      r3.x = r3.z ? r3.x : r4.y;
                      r3.z = -r2.x * 0.5 + r3.y;
                      r3.y = r3.w ? r3.y : r3.z;
                      r3.zw = cmp(abs(r3.xy) >= r2.ww);
                      r4.y = -r0.x * 4 + r4.x;
                      r4.x = r3.z ? r4.x : r4.y;
                      r4.y = -r0.y * 4 + r4.z;
                      r4.z = r3.z ? r4.z : r4.y;
                      r4.yw = ~(int2)r3.zw;
                      r4.y = (int)r4.w | (int)r4.y;
                      r4.w = r0.x * 4 + r5.x;
                      r5.x = r3.w ? r5.x : r4.w;
                      r4.w = r0.y * 4 + r5.z;
                      r5.z = r3.w ? r5.z : r4.w;
                      if (r4.y != 0) {
                        if (r3.z == 0) {
                          r3.x = colorTexture.SampleLevel(sam0_s, r4.xz, 0).w;
                        }
                        if (r3.w == 0) {
                          r3.y = colorTexture.SampleLevel(sam0_s, r5.xz, 0).w;
                        }
                        r4.y = -r2.x * 0.5 + r3.x;
                        r3.x = r3.z ? r3.x : r4.y;
                        r2.x = -r2.x * 0.5 + r3.y;
                        r3.y = r3.w ? r3.y : r2.x;
                        r2.xw = cmp(abs(r3.xy) >= r2.ww);
                        r3.z = -r0.x * 8 + r4.x;
                        r4.x = r2.x ? r4.x : r3.z;
                        r3.z = -r0.y * 8 + r4.z;
                        r4.z = r2.x ? r4.z : r3.z;
                        r0.x = r0.x * 8 + r5.x;
                        r5.x = r2.w ? r5.x : r0.x;
                        r0.x = r0.y * 8 + r5.z;
                        r5.z = r2.w ? r5.z : r0.x;
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
    r0.x = v1.x + -r4.x;
    r0.y = -v1.x + r5.x;
    r2.x = v1.y + -r4.z;
    r0.x = r0.w ? r0.x : r2.x;
    r2.x = -v1.y + r5.z;
    r0.y = r0.w ? r0.y : r2.x;
    r2.xw = cmp(r3.xy < float2(0,0));
    r3.x = r0.y + r0.x;
    r2.xy = cmp((int2)r2.yy != (int2)r2.xw);
    r2.w = 1 / r3.x;
    r3.x = cmp(r0.x < r0.y);
    r0.x = min(r0.x, r0.y);
    r0.y = r3.x ? r2.x : r2.y;
    r0.z = r0.z * r0.z;
    r0.x = r0.x * -r2.w + 0.5;
    r0.z = 0.75 * r0.z;
    r0.x = (int)r0.x & (int)r0.y;
    r0.x = max(r0.x, r0.z);
    r0.xy = r0.xx * r2.zz + v1.xy;
    r2.x = r0.w ? v1.x : r0.x;
    r2.y = r0.w ? r0.y : v1.y;
    r1.xyz = colorTexture.SampleLevel(sam0_s, r2.xy, 0).xyz;
  }
  o0.xyzw = r1.xyzw;
  return;
}