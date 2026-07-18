// ---- Created with 3Dmigoto v1.3.16 on Mon Sep 08 19:37:02 2025

cbuffer cb_scene : register(b0)
{
  float4x4 view_g : packoffset(c0);
  float4x4 viewInv_g : packoffset(c4);
  float4x4 proj_g : packoffset(c8);
  float4x4 projInv_g : packoffset(c12);
  float4x4 viewProj_g : packoffset(c16);
  float4x4 viewProjInv_g : packoffset(c20);
  float2 vpSize_g : packoffset(c24);
  float2 invVPSize_g : packoffset(c24.z);
  float3 lightColor_g : packoffset(c25);
  float ldotvXZ_g : packoffset(c25.w);
  float3 lightDirection_g : packoffset(c26);
  float shadowSplitDistance_g : packoffset(c26.w);
  float4x4 shadowMtx_g[2] : packoffset(c27);
  float2 invShadowSize_g : packoffset(c35);
  float shadowFadeNear_g : packoffset(c35.z);
  float shadowFadeRangeInv_g : packoffset(c35.w);
  float3 sceneShadowColor_g : packoffset(c36);
  float gameTime_g : packoffset(c36.w);
  float3 windDirection_g : packoffset(c37);
  uint collisionCount_g : packoffset(c37.w);
  float lightTileWidthInv_g : packoffset(c38);
  float lightTileHeightInv_g : packoffset(c38.y);
  float fogNearDistance_g : packoffset(c38.z);
  float fogFadeRangeInv_g : packoffset(c38.w);
  float3 fogColor_g : packoffset(c39);
  float fogIntensity_g : packoffset(c39.w);
  float fogHeight_g : packoffset(c40);
  float fogHeightRangeInv_g : packoffset(c40.y);
  float windWaveTime_g : packoffset(c40.z);
  float windWaveFrequency_g : packoffset(c40.w);
  uint localLightProbeCount_g : packoffset(c41);
  float lightSpecularGlossiness_g : packoffset(c41.y);
  float lightSpecularIntensity_g : packoffset(c41.z);
  uint pointLightCount_g : packoffset(c41.w);
  float4x4 ditherMtx_g : packoffset(c42);
  float4 lightProbe_g[9] : packoffset(c46);
  float4x4 farShadowMtx_g : packoffset(c55);
  float3 chrLightDir_g : packoffset(c59);
  float shadowDistance_g : packoffset(c59.w);
  float resolutionScaling_g : packoffset(c60);
  float sceneTime_g : packoffset(c60.y);
  float windForce_g : packoffset(c60.z);
  float disableMapObjNearFade_g : packoffset(c60.w);
  float4 mapColor_g : packoffset(c61);
  float4 clipPlane_g : packoffset(c62);
  float shadowZeroCascadeUVMult_g : packoffset(c63);
}

SamplerState sam0_s : register(s0);
Texture2D<float4> colorTexture : register(t0);


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

  r0.xyzw = colorTexture.SampleLevel(sam0_s, v1.xy, 0).xyzw;
  r1.xyz = colorTexture.Gather(sam0_s, v1.xy).xyz;
  r2.xyz = colorTexture.Gather(sam0_s, v1.xy, int2(-1, -1)).xzw;
  r1.w = max(r1.x, r0.w);
  r2.w = min(r1.x, r0.w);
  r1.w = max(r1.z, r1.w);
  r2.w = min(r2.w, r1.z);
  r3.x = max(r2.y, r2.x);
  r3.y = min(r2.y, r2.x);
  r1.w = max(r3.x, r1.w);
  r2.w = min(r3.y, r2.w);
  r3.x = 0.125 * r1.w;
  r1.w = -r2.w + r1.w;
  r2.w = max(0.0311999992, r3.x);
  r2.w = cmp(r1.w >= r2.w);
  if (r2.w != 0) {
    r2.w = colorTexture.SampleLevel(sam0_s, v1.xy, 0, int2(1, -1)).w;
    r3.x = colorTexture.SampleLevel(sam0_s, v1.xy, 0, int2(-1, 1)).w;
    r3.yz = r2.yx + r1.xz;
    r1.w = 1 / r1.w;
    r3.w = r3.y + r3.z;
    r3.yz = r0.ww * float2(-2,-2) + r3.yz;
    r4.x = r2.w + r1.y;
    r2.w = r2.z + r2.w;
    r4.y = r1.z * -2 + r4.x;
    r2.w = r2.y * -2 + r2.w;
    r2.z = r3.x + r2.z;
    r1.y = r3.x + r1.y;
    r3.x = abs(r3.y) * 2 + abs(r4.y);
    r2.w = abs(r3.z) * 2 + abs(r2.w);
    r3.y = r2.x * -2 + r2.z;
    r1.y = r1.x * -2 + r1.y;
    r3.x = abs(r3.y) + r3.x;
    r1.y = abs(r1.y) + r2.w;
    r2.z = r2.z + r4.x;
    r1.y = cmp(r3.x >= r1.y);
    r2.z = r3.w * 2 + r2.z;
    r2.x = r1.y ? r2.y : r2.x;
    r1.x = r1.y ? r1.x : r1.z;
    r1.z = r1.y ? invVPSize_g.y : invVPSize_g.x;
    r2.y = r2.z * 0.0833333358 + -r0.w;
    r2.z = r2.x + -r0.w;
    r2.w = r1.x + -r0.w;
    r2.x = r2.x + r0.w;
    r1.x = r1.x + r0.w;
    r3.x = cmp(abs(r2.z) >= abs(r2.w));
    r2.z = max(abs(r2.z), abs(r2.w));
    r1.z = r3.x ? -r1.z : r1.z;
    r1.w = saturate(abs(r2.y) * r1.w);
    r2.y = r1.y ? invVPSize_g.x : 0;
    r2.w = r1.y ? 0 : invVPSize_g.y;
    r3.yz = r1.zz * float2(0.5,0.5) + v1.xy;
    r3.y = r1.y ? v1.x : r3.y;
    r3.z = r1.y ? r3.z : v1.y;
    r4.xy = r3.yz + -r2.yw;
    r5.xy = r3.yz + r2.yw;
    r3.y = r1.w * -2 + 3;
    r3.z = colorTexture.SampleLevel(sam0_s, r4.xy, 0).w;
    r1.w = r1.w * r1.w;
    r3.w = colorTexture.SampleLevel(sam0_s, r5.xy, 0).w;
    r1.x = r3.x ? r2.x : r1.x;
    r2.x = 0.25 * r2.z;
    r2.z = -r1.x * 0.5 + r0.w;
    r1.w = r3.y * r1.w;
    r2.z = cmp(r2.z < 0);
    r3.x = -r1.x * 0.5 + r3.z;
    r3.y = -r1.x * 0.5 + r3.w;
    r3.zw = cmp(abs(r3.xy) >= r2.xx);
    r4.z = -r2.y * 1.5 + r4.x;
    r4.x = r3.z ? r4.x : r4.z;
    r4.w = -r2.w * 1.5 + r4.y;
    r4.z = r3.z ? r4.y : r4.w;
    r4.yw = ~(int2)r3.zw;
    r4.y = (int)r4.w | (int)r4.y;
    r4.w = r2.y * 1.5 + r5.x;
    r5.x = r3.w ? r5.x : r4.w;
    r4.w = r2.w * 1.5 + r5.y;
    r5.z = r3.w ? r5.y : r4.w;
    if (r4.y != 0) {
      if (r3.z == 0) {
        r3.x = colorTexture.SampleLevel(sam0_s, r4.xz, 0).w;
      }
      if (r3.w == 0) {
        r3.y = colorTexture.SampleLevel(sam0_s, r5.xz, 0).w;
      }
      r4.y = -r1.x * 0.5 + r3.x;
      r3.x = r3.z ? r3.x : r4.y;
      r3.z = -r1.x * 0.5 + r3.y;
      r3.y = r3.w ? r3.y : r3.z;
      r3.zw = cmp(abs(r3.xy) >= r2.xx);
      r4.y = -r2.y * 2 + r4.x;
      r4.x = r3.z ? r4.x : r4.y;
      r4.y = -r2.w * 2 + r4.z;
      r4.z = r3.z ? r4.z : r4.y;
      r4.yw = ~(int2)r3.zw;
      r4.y = (int)r4.w | (int)r4.y;
      r4.w = r2.y * 2 + r5.x;
      r5.x = r3.w ? r5.x : r4.w;
      r4.w = r2.w * 2 + r5.z;
      r5.z = r3.w ? r5.z : r4.w;
      if (r4.y != 0) {
        if (r3.z == 0) {
          r3.x = colorTexture.SampleLevel(sam0_s, r4.xz, 0).w;
        }
        if (r3.w == 0) {
          r3.y = colorTexture.SampleLevel(sam0_s, r5.xz, 0).w;
        }
        r4.y = -r1.x * 0.5 + r3.x;
        r3.x = r3.z ? r3.x : r4.y;
        r3.z = -r1.x * 0.5 + r3.y;
        r3.y = r3.w ? r3.y : r3.z;
        r3.zw = cmp(abs(r3.xy) >= r2.xx);
        r4.y = -r2.y * 2 + r4.x;
        r4.x = r3.z ? r4.x : r4.y;
        r4.y = -r2.w * 2 + r4.z;
        r4.z = r3.z ? r4.z : r4.y;
        r4.yw = ~(int2)r3.zw;
        r4.y = (int)r4.w | (int)r4.y;
        r4.w = r2.y * 2 + r5.x;
        r5.x = r3.w ? r5.x : r4.w;
        r4.w = r2.w * 2 + r5.z;
        r5.z = r3.w ? r5.z : r4.w;
        if (r4.y != 0) {
          if (r3.z == 0) {
            r3.x = colorTexture.SampleLevel(sam0_s, r4.xz, 0).w;
          }
          if (r3.w == 0) {
            r3.y = colorTexture.SampleLevel(sam0_s, r5.xz, 0).w;
          }
          r4.y = -r1.x * 0.5 + r3.x;
          r3.x = r3.z ? r3.x : r4.y;
          r3.z = -r1.x * 0.5 + r3.y;
          r3.y = r3.w ? r3.y : r3.z;
          r3.zw = cmp(abs(r3.xy) >= r2.xx);
          r4.y = -r2.y * 2 + r4.x;
          r4.x = r3.z ? r4.x : r4.y;
          r4.y = -r2.w * 2 + r4.z;
          r4.z = r3.z ? r4.z : r4.y;
          r4.yw = ~(int2)r3.zw;
          r4.y = (int)r4.w | (int)r4.y;
          r4.w = r2.y * 2 + r5.x;
          r5.x = r3.w ? r5.x : r4.w;
          r4.w = r2.w * 2 + r5.z;
          r5.z = r3.w ? r5.z : r4.w;
          if (r4.y != 0) {
            if (r3.z == 0) {
              r3.x = colorTexture.SampleLevel(sam0_s, r4.xz, 0).w;
            }
            if (r3.w == 0) {
              r3.y = colorTexture.SampleLevel(sam0_s, r5.xz, 0).w;
            }
            r4.y = -r1.x * 0.5 + r3.x;
            r3.x = r3.z ? r3.x : r4.y;
            r3.z = -r1.x * 0.5 + r3.y;
            r3.y = r3.w ? r3.y : r3.z;
            r3.zw = cmp(abs(r3.xy) >= r2.xx);
            r4.y = -r2.y * 2 + r4.x;
            r4.x = r3.z ? r4.x : r4.y;
            r4.y = -r2.w * 2 + r4.z;
            r4.z = r3.z ? r4.z : r4.y;
            r4.yw = ~(int2)r3.zw;
            r4.y = (int)r4.w | (int)r4.y;
            r4.w = r2.y * 2 + r5.x;
            r5.x = r3.w ? r5.x : r4.w;
            r4.w = r2.w * 2 + r5.z;
            r5.z = r3.w ? r5.z : r4.w;
            if (r4.y != 0) {
              if (r3.z == 0) {
                r3.x = colorTexture.SampleLevel(sam0_s, r4.xz, 0).w;
              }
              if (r3.w == 0) {
                r3.y = colorTexture.SampleLevel(sam0_s, r5.xz, 0).w;
              }
              r4.y = -r1.x * 0.5 + r3.x;
              r3.x = r3.z ? r3.x : r4.y;
              r3.z = -r1.x * 0.5 + r3.y;
              r3.y = r3.w ? r3.y : r3.z;
              r3.zw = cmp(abs(r3.xy) >= r2.xx);
              r4.y = -r2.y * 2 + r4.x;
              r4.x = r3.z ? r4.x : r4.y;
              r4.y = -r2.w * 2 + r4.z;
              r4.z = r3.z ? r4.z : r4.y;
              r4.yw = ~(int2)r3.zw;
              r4.y = (int)r4.w | (int)r4.y;
              r4.w = r2.y * 2 + r5.x;
              r5.x = r3.w ? r5.x : r4.w;
              r4.w = r2.w * 2 + r5.z;
              r5.z = r3.w ? r5.z : r4.w;
              if (r4.y != 0) {
                if (r3.z == 0) {
                  r3.x = colorTexture.SampleLevel(sam0_s, r4.xz, 0).w;
                }
                if (r3.w == 0) {
                  r3.y = colorTexture.SampleLevel(sam0_s, r5.xz, 0).w;
                }
                r4.y = -r1.x * 0.5 + r3.x;
                r3.x = r3.z ? r3.x : r4.y;
                r3.z = -r1.x * 0.5 + r3.y;
                r3.y = r3.w ? r3.y : r3.z;
                r3.zw = cmp(abs(r3.xy) >= r2.xx);
                r4.y = -r2.y * 2 + r4.x;
                r4.x = r3.z ? r4.x : r4.y;
                r4.y = -r2.w * 2 + r4.z;
                r4.z = r3.z ? r4.z : r4.y;
                r4.yw = ~(int2)r3.zw;
                r4.y = (int)r4.w | (int)r4.y;
                r4.w = r2.y * 2 + r5.x;
                r5.x = r3.w ? r5.x : r4.w;
                r4.w = r2.w * 2 + r5.z;
                r5.z = r3.w ? r5.z : r4.w;
                if (r4.y != 0) {
                  if (r3.z == 0) {
                    r3.x = colorTexture.SampleLevel(sam0_s, r4.xz, 0).w;
                  }
                  if (r3.w == 0) {
                    r3.y = colorTexture.SampleLevel(sam0_s, r5.xz, 0).w;
                  }
                  r4.y = -r1.x * 0.5 + r3.x;
                  r3.x = r3.z ? r3.x : r4.y;
                  r3.z = -r1.x * 0.5 + r3.y;
                  r3.y = r3.w ? r3.y : r3.z;
                  r3.zw = cmp(abs(r3.xy) >= r2.xx);
                  r4.y = -r2.y * 2 + r4.x;
                  r4.x = r3.z ? r4.x : r4.y;
                  r4.y = -r2.w * 2 + r4.z;
                  r4.z = r3.z ? r4.z : r4.y;
                  r4.yw = ~(int2)r3.zw;
                  r4.y = (int)r4.w | (int)r4.y;
                  r4.w = r2.y * 2 + r5.x;
                  r5.x = r3.w ? r5.x : r4.w;
                  r4.w = r2.w * 2 + r5.z;
                  r5.z = r3.w ? r5.z : r4.w;
                  if (r4.y != 0) {
                    if (r3.z == 0) {
                      r3.x = colorTexture.SampleLevel(sam0_s, r4.xz, 0).w;
                    }
                    if (r3.w == 0) {
                      r3.y = colorTexture.SampleLevel(sam0_s, r5.xz, 0).w;
                    }
                    r4.y = -r1.x * 0.5 + r3.x;
                    r3.x = r3.z ? r3.x : r4.y;
                    r3.z = -r1.x * 0.5 + r3.y;
                    r3.y = r3.w ? r3.y : r3.z;
                    r3.zw = cmp(abs(r3.xy) >= r2.xx);
                    r4.y = -r2.y * 2 + r4.x;
                    r4.x = r3.z ? r4.x : r4.y;
                    r4.y = -r2.w * 2 + r4.z;
                    r4.z = r3.z ? r4.z : r4.y;
                    r4.yw = ~(int2)r3.zw;
                    r4.y = (int)r4.w | (int)r4.y;
                    r4.w = r2.y * 2 + r5.x;
                    r5.x = r3.w ? r5.x : r4.w;
                    r4.w = r2.w * 2 + r5.z;
                    r5.z = r3.w ? r5.z : r4.w;
                    if (r4.y != 0) {
                      if (r3.z == 0) {
                        r3.x = colorTexture.SampleLevel(sam0_s, r4.xz, 0).w;
                      }
                      if (r3.w == 0) {
                        r3.y = colorTexture.SampleLevel(sam0_s, r5.xz, 0).w;
                      }
                      r4.y = -r1.x * 0.5 + r3.x;
                      r3.x = r3.z ? r3.x : r4.y;
                      r3.z = -r1.x * 0.5 + r3.y;
                      r3.y = r3.w ? r3.y : r3.z;
                      r3.zw = cmp(abs(r3.xy) >= r2.xx);
                      r4.y = -r2.y * 4 + r4.x;
                      r4.x = r3.z ? r4.x : r4.y;
                      r4.y = -r2.w * 4 + r4.z;
                      r4.z = r3.z ? r4.z : r4.y;
                      r4.yw = ~(int2)r3.zw;
                      r4.y = (int)r4.w | (int)r4.y;
                      r4.w = r2.y * 4 + r5.x;
                      r5.x = r3.w ? r5.x : r4.w;
                      r4.w = r2.w * 4 + r5.z;
                      r5.z = r3.w ? r5.z : r4.w;
                      if (r4.y != 0) {
                        if (r3.z == 0) {
                          r3.x = colorTexture.SampleLevel(sam0_s, r4.xz, 0).w;
                        }
                        if (r3.w == 0) {
                          r3.y = colorTexture.SampleLevel(sam0_s, r5.xz, 0).w;
                        }
                        r4.y = -r1.x * 0.5 + r3.x;
                        r3.x = r3.z ? r3.x : r4.y;
                        r1.x = -r1.x * 0.5 + r3.y;
                        r3.y = r3.w ? r3.y : r1.x;
                        r3.zw = cmp(abs(r3.xy) >= r2.xx);
                        r1.x = -r2.y * 8 + r4.x;
                        r4.x = r3.z ? r4.x : r1.x;
                        r1.x = -r2.w * 8 + r4.z;
                        r4.z = r3.z ? r4.z : r1.x;
                        r1.x = r2.y * 8 + r5.x;
                        r5.x = r3.w ? r5.x : r1.x;
                        r1.x = r2.w * 8 + r5.z;
                        r5.z = r3.w ? r5.z : r1.x;
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
    r2.xy = -v1.xy + r5.xz;
    r2.x = r1.y ? r2.x : r2.y;
    r2.yw = cmp(r3.xy < float2(0,0));
    r3.x = r2.x + r1.x;
    r2.yz = cmp((int2)r2.zz != (int2)r2.yw);
    r2.w = 1 / r3.x;
    r3.x = cmp(r1.x < r2.x);
    r1.x = min(r2.x, r1.x);
    r2.x = r3.x ? r2.y : r2.z;
    r1.w = r1.w * r1.w;
    r1.x = r1.x * -r2.w + 0.5;
    r1.w = 0.75 * r1.w;
    r1.x = (int)r1.x & (int)r2.x;
    r1.x = max(r1.x, r1.w);
    r1.xz = r1.xx * r1.zz + v1.xy;
    r2.x = r1.y ? v1.x : r1.x;
    r2.y = r1.y ? r1.z : v1.y;
    r0.xyz = colorTexture.SampleLevel(sam0_s, r2.xy, 0).xyz;
  }
  o0.xyzw = r0.xyzw;
  return;
}