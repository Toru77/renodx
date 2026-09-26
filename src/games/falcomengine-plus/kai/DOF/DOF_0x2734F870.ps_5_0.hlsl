// ---- Created with 3Dmigoto v1.4.1 on Fri Mar  6 12:50:53 2026

#include "../../shared.h"

cbuffer cb_dof : register(b2)
{
  float2 uv_clamp : packoffset(c0);
  float cocMaxRadius : packoffset(c0.z);
  float nearZ : packoffset(c0.w);
  float farZ : packoffset(c1);
  float invNearFade : packoffset(c1.y);
  float invFarFade : packoffset(c1.z);
  float nearFadeExp : packoffset(c1.w);
  float farFadeExp : packoffset(c2);
}

SamplerState samPoint_s : register(s0);
SamplerState samLinear_s : register(s1);
Texture2D<float4> colorTexture : register(t0);
// ── IS-FAST volume for the per-pixel rotated gather (pushed at t5 when usable) ──
Texture3D<float2> dofIsfastNoiseTex : register(t5);
// Static point-clamp sampler (mirrors the Vanilla-SSR IS-FAST sampling).
SamplerState dofIsfastNoiseSamp
{
  Filter = MIN_MAG_MIP_POINT;
  AddressU = Clamp;
  AddressV = Clamp;
  AddressW = Clamp;
};


// 3Dmigoto declarations
#define cmp -

float3 GatherDOFImproved(float2 uv, float2 texel_size, float4 center_sample) {
  const float center_coc = center_sample.w;
  const float abs_center_coc = abs(center_coc);
  const float radius_px = abs_center_coc * cocMaxRadius * max(shader_injection_data.dof_radius_scale, 0.001f);
  if (radius_px <= 1e-4f) return center_sample.rgb;

  const int sample_count = clamp((int)round(shader_injection_data.dof_sample_count), 4, 64);
  const float edge_threshold = max(shader_injection_data.dof_edge_threshold, 1e-4f);
  // Option A: opposite-layer taps are attenuated by sign_softness instead of hard-rejected.
  const float sign_softness = saturate(shader_injection_data.dof_sign_softness);
  const bool coverage_fix = shader_injection_data.dof_coverage_enabled > 0.5f;
  const float layer_scale = (center_coc < 0.0f)
                                ? max(shader_injection_data.dof_near_scale, 0.0f)
                                : max(shader_injection_data.dof_far_scale, 0.0f);

  const float golden_angle = 2.39996322973f;

  // ── IS-FAST per-pixel spiral rotation ──
  // The golden-angle spiral is deterministic, so every pixel undersamples in
  // the same directions and the residual error reads as structured spiral/ring
  // ghosting rather than noise. Rotating per pixel decorrelates neighbouring
  // pixels, turning that structure into high-frequency noise that TAA or an
  // upscaler can resolve. Tap weights are derived from each tap's sampled CoC,
  // never from the tap direction, so rotation cannot alter the layering, sign
  // or coverage logic. Gated on dof_isfast_noise_frame < 0 (set by the addon
  // when the master toggle, this toggle, Improved mode or the volume is
  // unavailable) so the gather stays deterministic when disabled.
  float gather_rot = 0.0f;
  {
    float isfast_frame = shader_injection_data.dof_isfast_noise_frame;
    if (shader_injection_data.dof_isfast_enabled > 0.5f && isfast_frame >= 0.0f) {
      float2 isf_nxy = frac((uv / texel_size + 0.5) / 128.0);
      float isf_nz = frac((fmod(isfast_frame, 32.0) + 0.5) / 32.0);
      gather_rot = 6.28318548 * dofIsfastNoiseTex.SampleLevel(dofIsfastNoiseSamp, float3(isf_nxy, isf_nz), 0).x;
    }
  }

  // Option A: de-biased anchor — the sharp center participates with a finite
  // weight so starved thin features converge toward surviving taps instead of
  // snapping back to the unblurred pixel.
  const float anchor_weight = 0.25f;
  float3 accum = center_sample.rgb * anchor_weight;
  float weight_sum = anchor_weight;

  // Option B accumulators (taps only; center excluded from coverage stats).
  float3 plain_accum = float3(0.0f, 0.0f, 0.0f);
  float plain_wsum = 0.0f;
  float layer_tap_wsum = 0.0f;
  float all_tap_wsum = 0.0f;

  [loop]
  for (int tap = 0; tap < 64; ++tap) {
    if (tap >= sample_count) break;

    float t = ((float)tap + 0.5f) / (float)sample_count;
    float ring = sqrt(t);
    float angle = golden_angle * (float)tap + gather_rot;
    float2 dir = float2(cos(angle), sin(angle));

    float2 offset_px = (dir * ring) * radius_px;
    float2 sample_uv = clamp(uv + offset_px * texel_size, float2(0.0f, 0.0f), uv_clamp.xy);

    float4 sample_value = colorTexture.SampleLevel(samLinear_s, sample_uv, 0);
    float sample_coc = sample_value.w;

    float tap_weight = exp2(-2.0f * ring * ring);

    if (coverage_fix) {
      plain_accum += sample_value.rgb * tap_weight;
      plain_wsum += tap_weight;
      all_tap_wsum += tap_weight;
    }

    float sign_compat = (sample_coc * center_coc >= 0.0f)
                            ? 1.0f
                            : sign_softness;
    float coc_delta = abs(abs(sample_coc) - abs_center_coc);
    float coc_compat = 1.0f - smoothstep(edge_threshold, edge_threshold * 2.0f, coc_delta);

    float coc_weight = saturate(abs(sample_coc) * layer_scale * 2.0f + 0.1f);
    float weight = tap_weight * sign_compat * coc_compat * coc_weight;

    accum += sample_value.rgb * weight;
    weight_sum += weight;
    if (coverage_fix) {
      layer_tap_wsum += weight;
    }
  }

  float3 layered = accum / max(weight_sum, 1e-5f);
  if (!coverage_fix) return layered;

  // Option B: blend same-layer bokeh toward the full-disc radial average by the
  // fraction of the disc actually covered by this depth layer, so thin features
  // soften instead of staying sharp.
  float coverage = saturate(layer_tap_wsum / max(all_tap_wsum, 1e-4f));
  float3 plain = plain_accum / max(plain_wsum, 1e-5f);
  return lerp(plain, layered, coverage);
}


void main(
  float4 v0 : SV_Position0,
  float4 v1 : TEXCOORD0,
  out float4 o0 : SV_Target0)
{
  float4 r0,r1,r2,r3,r4,r5,r6,r7,r8;
  uint4 bitmask, uiDest;
  float4 fDest;

  if (shader_injection_data.dof_mode > 0.5f) {
    float4 center_sample = colorTexture.SampleLevel(samPoint_s, v1.xy, 0);
    float coc_abs = abs(center_sample.w);
    float layer_scale = (center_sample.w < 0.0f)
                            ? max(shader_injection_data.dof_near_scale, 0.0f)
                            : max(shader_injection_data.dof_far_scale, 0.0f);
    float blend = saturate(coc_abs * layer_scale * max(shader_injection_data.dof_strength, 0.0f));
    if (blend <= 1e-4f) {
      o0 = center_sample;
      return;
    }

    colorTexture.GetDimensions(0, fDest.x, fDest.y, fDest.z);
    float2 texel_size = rcp(fDest.xy);
    float3 blurred_color = GatherDOFImproved(v1.xy, texel_size, center_sample);

    o0.rgb = lerp(center_sample.rgb, blurred_color, blend);
    o0.a = center_sample.a;
    return;
  }

  r0.x = dot(v1.xy, float2(12.9898005,78.2330017));
  r0.x = sin(r0.x);
  r0.x = 43758.5469 * r0.x;
  r0.x = frac(r0.x);
  sincos(r0.x, r0.x, r1.x);
  r2.xyz = float3(0.363635987,-0.727272987,0.727272987) * r0.xxx;
  r3.w = r2.x;
  r3.xyz = float3(0.363635987,0.727272987,-0.727272987) * r1.xxx;
  r0.yz = cocMaxRadius * r3.xw;
  colorTexture.GetDimensions(0, fDest.x, fDest.y, fDest.z);
  r1.yz = fDest.xy;
  r1.yz = rcp(r1.yz);
  r0.yz = r0.yz * r1.yz + v1.xy;
  r0.yz = min(uv_clamp.xy, r0.yz);
  r0.y = colorTexture.SampleLevel(samLinear_s, r0.yz, 0).w;
  r4.xyzw = colorTexture.SampleLevel(samPoint_s, v1.xy, 0).xyzw;
  r0.z = min(r4.w, r0.y);
  r0.y = max(0, -r0.y);
  r0.y = saturate(r0.y + -r4.w);
  r5.w = abs(r0.z);
  r0.zw = r5.ww * r3.xw;
  r0.zw = r0.zw * r1.yz;
  r0.zw = r0.zw * cocMaxRadius + v1.xy;
  r0.zw = min(uv_clamp.xy, r0.zw);
  r5.xyz = colorTexture.SampleLevel(samLinear_s, r0.zw, 0).xyz;
  r6.x = -r0.x;
  r6.y = r1.x;
  r6.z = r0.x;
  r7.x = dot(float2(0.34583801,0.112369999), r6.xy);
  r7.y = dot(float2(0.34583801,0.112369999), r6.yz);
  r0.xz = cocMaxRadius * r7.xy;
  r0.xz = r0.xz * r1.yz + v1.xy;
  r0.xz = min(uv_clamp.xy, r0.xz);
  r0.x = colorTexture.SampleLevel(samLinear_s, r0.xz, 0).w;
  r0.z = min(r0.x, r4.w);
  r0.x = max(0, -r0.x);
  r0.x = saturate(r0.x + -r4.w);
  r0.x = max(r0.y, r0.x);
  r8.w = abs(r0.z);
  r0.yz = r8.ww * r7.xy;
  r0.yz = r0.yz * r1.yz;
  r0.yz = r0.yz * cocMaxRadius + v1.xy;
  r0.yz = min(uv_clamp.xy, r0.yz);
  r8.xyz = colorTexture.SampleLevel(samLinear_s, r0.yz, 0).xyz;
  r0.yzw = r8.xyz + r5.xyz;
  r7.xyzw = r8.xyzw * r8.wwww;
  r1.x = r8.w + r5.w;
  r5.xyzw = r5.xyzw * r5.wwww + r7.xyzw;
  r7.x = dot(float2(0.213740006,-0.294187993), r6.xy);
  r7.y = dot(float2(0.213740006,-0.294187993), r6.yz);
  r3.xw = cocMaxRadius * r7.xy;
  r3.xw = r3.xw * r1.yz + v1.xy;
  r3.xw = min(uv_clamp.xy, r3.xw);
  r1.w = colorTexture.SampleLevel(samLinear_s, r3.xw, 0).w;
  r3.x = min(r1.w, r4.w);
  r1.w = max(0, -r1.w);
  r1.w = saturate(r1.w + -r4.w);
  r0.x = max(r1.w, r0.x);
  r8.w = abs(r3.x);
  r3.xw = r8.ww * r7.xy;
  r3.xw = r3.xw * r1.yz;
  r3.xw = r3.xw * cocMaxRadius + v1.xy;
  r3.xw = min(uv_clamp.xy, r3.xw);
  r8.xyz = colorTexture.SampleLevel(samLinear_s, r3.xw, 0).xyz;
  r0.yzw = r8.xyz + r0.yzw;
  r5.xyzw = r8.xyzw * r8.wwww + r5.xyzw;
  r1.x = r8.w + r1.x;
  r7.x = dot(float2(-0.213740006,-0.294187993), r6.xy);
  r7.y = dot(float2(-0.213740006,-0.294187993), r6.yz);
  r3.xw = cocMaxRadius * r7.xy;
  r3.xw = r3.xw * r1.yz + v1.xy;
  r3.xw = min(uv_clamp.xy, r3.xw);
  r1.w = colorTexture.SampleLevel(samLinear_s, r3.xw, 0).w;
  r3.x = min(r1.w, r4.w);
  r1.w = max(0, -r1.w);
  r1.w = saturate(r1.w + -r4.w);
  r0.x = max(r1.w, r0.x);
  r8.w = abs(r3.x);
  r3.xw = r8.ww * r7.xy;
  r3.xw = r3.xw * r1.yz;
  r3.xw = r3.xw * cocMaxRadius + v1.xy;
  r3.xw = min(uv_clamp.xy, r3.xw);
  r8.xyz = colorTexture.SampleLevel(samLinear_s, r3.xw, 0).xyz;
  r0.yzw = r8.xyz + r0.yzw;
  r5.xyzw = r8.xyzw * r8.wwww + r5.xyzw;
  r1.x = r8.w + r1.x;
  r7.x = dot(float2(-0.34583801,0.112369999), r6.xy);
  r7.y = dot(float2(-0.34583801,0.112369999), r6.yz);
  r3.xw = cocMaxRadius * r7.xy;
  r3.xw = r3.xw * r1.yz + v1.xy;
  r3.xw = min(uv_clamp.xy, r3.xw);
  r1.w = colorTexture.SampleLevel(samLinear_s, r3.xw, 0).w;
  r3.x = min(r1.w, r4.w);
  r1.w = max(0, -r1.w);
  r1.w = saturate(r1.w + -r4.w);
  r0.x = max(r1.w, r0.x);
  r8.w = abs(r3.x);
  r3.xw = r8.ww * r7.xy;
  r3.xw = r3.xw * r1.yz;
  r3.xw = r3.xw * cocMaxRadius + v1.xy;
  r3.xw = min(uv_clamp.xy, r3.xw);
  r8.xyz = colorTexture.SampleLevel(samLinear_s, r3.xw, 0).xyz;
  r0.yzw = r8.xyz + r0.yzw;
  r5.xyzw = r8.xyzw * r8.wwww + r5.xyzw;
  r1.x = r8.w + r1.x;
  r2.xw = r3.zy;
  r3.xy = cocMaxRadius * r2.wz;
  r3.xy = r3.xy * r1.yz + v1.xy;
  r3.xy = min(uv_clamp.xy, r3.xy);
  r1.w = colorTexture.SampleLevel(samLinear_s, r3.xy, 0).w;
  r3.x = min(r1.w, r4.w);
  r1.w = max(0, -r1.w);
  r1.w = saturate(r1.w + -r4.w);
  r0.x = max(r1.w, r0.x);
  r3.w = abs(r3.x);
  r2.zw = r3.ww * r2.wz;
  r2.zw = r2.zw * r1.yz;
  r2.zw = r2.zw * cocMaxRadius + v1.xy;
  r2.zw = min(uv_clamp.xy, r2.zw);
  r3.xyz = colorTexture.SampleLevel(samLinear_s, r2.zw, 0).xyz;
  r0.yzw = r3.xyz + r0.yzw;
  r5.xyzw = r3.xyzw * r3.wwww + r5.xyzw;
  r1.x = r3.w + r1.x;
  r3.x = dot(float2(0.427480012,0.588375986), r6.xy);
  r3.y = dot(float2(0.427480012,0.588375986), r6.yz);
  r2.zw = cocMaxRadius * r3.xy;
  r2.zw = r2.zw * r1.yz + v1.xy;
  r2.zw = min(uv_clamp.xy, r2.zw);
  r1.w = colorTexture.SampleLevel(samLinear_s, r2.zw, 0).w;
  r2.z = min(r1.w, r4.w);
  r1.w = max(0, -r1.w);
  r1.w = saturate(r1.w + -r4.w);
  r0.x = max(r1.w, r0.x);
  r7.w = abs(r2.z);
  r2.zw = r7.ww * r3.xy;
  r2.zw = r2.zw * r1.yz;
  r2.zw = r2.zw * cocMaxRadius + v1.xy;
  r2.zw = min(uv_clamp.xy, r2.zw);
  r7.xyz = colorTexture.SampleLevel(samLinear_s, r2.zw, 0).xyz;
  r0.yzw = r7.xyz + r0.yzw;
  r3.xyzw = r7.xyzw * r7.wwww + r5.xyzw;
  r1.x = r7.w + r1.x;
  r5.x = dot(float2(0.691677988,0.224739999), r6.xy);
  r5.y = dot(float2(0.691677988,0.224739999), r6.yz);
  r2.zw = cocMaxRadius * r5.xy;
  r2.zw = r2.zw * r1.yz + v1.xy;
  r2.zw = min(uv_clamp.xy, r2.zw);
  r1.w = colorTexture.SampleLevel(samLinear_s, r2.zw, 0).w;
  r2.z = min(r1.w, r4.w);
  r1.w = max(0, -r1.w);
  r1.w = saturate(r1.w + -r4.w);
  r0.x = max(r1.w, r0.x);
  r7.w = abs(r2.z);
  r2.zw = r7.ww * r5.xy;
  r2.zw = r2.zw * r1.yz;
  r2.zw = r2.zw * cocMaxRadius + v1.xy;
  r2.zw = min(uv_clamp.xy, r2.zw);
  r7.xyz = colorTexture.SampleLevel(samLinear_s, r2.zw, 0).xyz;
  r0.yzw = r7.xyz + r0.yzw;
  r3.xyzw = r7.xyzw * r7.wwww + r3.xyzw;
  r1.x = r7.w + r1.x;
  r5.x = dot(float2(0.691677988,-0.224739999), r6.xy);
  r5.y = dot(float2(0.691677988,-0.224739999), r6.yz);
  r2.zw = cocMaxRadius * r5.xy;
  r2.zw = r2.zw * r1.yz + v1.xy;
  r2.zw = min(uv_clamp.xy, r2.zw);
  r1.w = colorTexture.SampleLevel(samLinear_s, r2.zw, 0).w;
  r2.z = min(r1.w, r4.w);
  r1.w = max(0, -r1.w);
  r1.w = saturate(r1.w + -r4.w);
  r0.x = max(r1.w, r0.x);
  r7.w = abs(r2.z);
  r2.zw = r7.ww * r5.xy;
  r2.zw = r2.zw * r1.yz;
  r2.zw = r2.zw * cocMaxRadius + v1.xy;
  r2.zw = min(uv_clamp.xy, r2.zw);
  r7.xyz = colorTexture.SampleLevel(samLinear_s, r2.zw, 0).xyz;
  r0.yzw = r7.xyz + r0.yzw;
  r3.xyzw = r7.xyzw * r7.wwww + r3.xyzw;
  r1.x = r7.w + r1.x;
  r5.x = dot(float2(0.427480012,-0.588375986), r6.xy);
  r5.y = dot(float2(0.427480012,-0.588375986), r6.yz);
  r2.zw = cocMaxRadius * r5.xy;
  r2.zw = r2.zw * r1.yz + v1.xy;
  r2.zw = min(uv_clamp.xy, r2.zw);
  r1.w = colorTexture.SampleLevel(samLinear_s, r2.zw, 0).w;
  r2.z = min(r1.w, r4.w);
  r1.w = max(0, -r1.w);
  r1.w = saturate(r1.w + -r4.w);
  r0.x = max(r1.w, r0.x);
  r7.w = abs(r2.z);
  r2.zw = r7.ww * r5.xy;
  r2.zw = r2.zw * r1.yz;
  r2.zw = r2.zw * cocMaxRadius + v1.xy;
  r2.zw = min(uv_clamp.xy, r2.zw);
  r7.xyz = colorTexture.SampleLevel(samLinear_s, r2.zw, 0).xyz;
  r0.yzw = r7.xyz + r0.yzw;
  r3.xyzw = r7.xyzw * r7.wwww + r3.xyzw;
  r1.x = r7.w + r1.x;
  r2.zw = cocMaxRadius * r2.xy;
  r2.zw = r2.zw * r1.yz + v1.xy;
  r2.zw = min(uv_clamp.xy, r2.zw);
  r1.w = colorTexture.SampleLevel(samLinear_s, r2.zw, 0).w;
  r2.z = min(r1.w, r4.w);
  r1.w = max(0, -r1.w);
  r1.w = saturate(r1.w + -r4.w);
  r0.x = max(r1.w, r0.x);
  r5.w = abs(r2.z);
  r2.xy = r5.ww * r2.xy;
  r2.xy = r2.xy * r1.yz;
  r2.xy = r2.xy * cocMaxRadius + v1.xy;
  r2.xy = min(uv_clamp.xy, r2.xy);
  r5.xyz = colorTexture.SampleLevel(samLinear_s, r2.xy, 0).xyz;
  r0.yzw = r5.xyz + r0.yzw;
  r2.xyzw = r5.xyzw * r5.wwww + r3.xyzw;
  r1.x = r5.w + r1.x;
  r3.x = dot(float2(-0.427480012,-0.588375986), r6.xy);
  r3.y = dot(float2(-0.427480012,-0.588375986), r6.yz);
  r3.zw = cocMaxRadius * r3.xy;
  r3.zw = r3.zw * r1.yz + v1.xy;
  r3.zw = min(uv_clamp.xy, r3.zw);
  r1.w = colorTexture.SampleLevel(samLinear_s, r3.zw, 0).w;
  r3.z = min(r1.w, r4.w);
  r1.w = max(0, -r1.w);
  r1.w = saturate(r1.w + -r4.w);
  r0.x = max(r1.w, r0.x);
  r5.w = abs(r3.z);
  r3.xy = r5.ww * r3.xy;
  r3.xy = r3.xy * r1.yz;
  r3.xy = r3.xy * cocMaxRadius + v1.xy;
  r3.xy = min(uv_clamp.xy, r3.xy);
  r5.xyz = colorTexture.SampleLevel(samLinear_s, r3.xy, 0).xyz;
  r0.yzw = r5.xyz + r0.yzw;
  r2.xyzw = r5.xyzw * r5.wwww + r2.xyzw;
  r1.x = r5.w + r1.x;
  r3.x = dot(float2(-0.691677988,-0.224739), r6.xy);
  r3.y = dot(float2(-0.691677988,-0.224739), r6.yz);
  r3.zw = cocMaxRadius * r3.xy;
  r3.zw = r3.zw * r1.yz + v1.xy;
  r3.zw = min(uv_clamp.xy, r3.zw);
  r1.w = colorTexture.SampleLevel(samLinear_s, r3.zw, 0).w;
  r3.z = min(r1.w, r4.w);
  r1.w = max(0, -r1.w);
  r1.w = saturate(r1.w + -r4.w);
  r0.x = max(r1.w, r0.x);
  r5.w = abs(r3.z);
  r3.xy = r5.ww * r3.xy;
  r3.xy = r3.xy * r1.yz;
  r3.xy = r3.xy * cocMaxRadius + v1.xy;
  r3.xy = min(uv_clamp.xy, r3.xy);
  r5.xyz = colorTexture.SampleLevel(samLinear_s, r3.xy, 0).xyz;
  r0.yzw = r5.xyz + r0.yzw;
  r2.xyzw = r5.xyzw * r5.wwww + r2.xyzw;
  r1.x = r5.w + r1.x;
  r3.x = dot(float2(-0.691677988,0.224739999), r6.xy);
  r3.y = dot(float2(-0.691677988,0.224739999), r6.yz);
  r3.zw = cocMaxRadius * r3.xy;
  r3.zw = r3.zw * r1.yz + v1.xy;
  r3.zw = min(uv_clamp.xy, r3.zw);
  r1.w = colorTexture.SampleLevel(samLinear_s, r3.zw, 0).w;
  r3.z = min(r1.w, r4.w);
  r1.w = max(0, -r1.w);
  r1.w = saturate(r1.w + -r4.w);
  r0.x = max(r1.w, r0.x);
  r5.w = abs(r3.z);
  r3.xy = r5.ww * r3.xy;
  r3.xy = r3.xy * r1.yz;
  r3.xy = r3.xy * cocMaxRadius + v1.xy;
  r3.xy = min(uv_clamp.xy, r3.xy);
  r5.xyz = colorTexture.SampleLevel(samLinear_s, r3.xy, 0).xyz;
  r0.yzw = r5.xyz + r0.yzw;
  r2.xyzw = r5.xyzw * r5.wwww + r2.xyzw;
  r1.x = r5.w + r1.x;
  r3.x = dot(float2(-0.427480012,0.588375986), r6.xy);
  r3.y = dot(float2(-0.427480012,0.588375986), r6.yz);
  r3.zw = cocMaxRadius * r3.xy;
  r3.zw = r3.zw * r1.yz + v1.xy;
  r3.zw = min(uv_clamp.xy, r3.zw);
  r1.w = colorTexture.SampleLevel(samLinear_s, r3.zw, 0).w;
  r3.z = min(r1.w, r4.w);
  r1.w = max(0, -r1.w);
  r1.w = saturate(r1.w + -r4.w);
  r0.x = max(r1.w, r0.x);
  r5.w = abs(r3.z);
  r3.xy = r5.ww * r3.xy;
  r3.xy = r3.xy * r1.yz;
  r3.xy = r3.xy * cocMaxRadius + v1.xy;
  r3.xy = min(uv_clamp.xy, r3.xy);
  r5.xyz = colorTexture.SampleLevel(samLinear_s, r3.xy, 0).xyz;
  r0.yzw = r5.xyz + r0.yzw;
  r2.xyzw = r5.xyzw * r5.wwww + r2.xyzw;
  r1.x = r5.w + r1.x;
  r3.xy = cocMaxRadius * r6.yz;
  r3.xy = r3.xy * r1.yz + v1.xy;
  r3.xy = min(uv_clamp.xy, r3.xy);
  r1.w = colorTexture.SampleLevel(samLinear_s, r3.xy, 0).w;
  r3.x = min(r1.w, r4.w);
  r1.w = max(0, -r1.w);
  r1.w = saturate(r1.w + -r4.w);
  r0.x = max(r1.w, r0.x);
  r3.w = abs(r3.x);
  r5.xy = r6.yz * r3.ww;
  r5.xy = r5.xy * r1.yz;
  r5.xy = r5.xy * cocMaxRadius + v1.xy;
  r5.xy = min(uv_clamp.xy, r5.xy);
  r3.xyz = colorTexture.SampleLevel(samLinear_s, r5.xy, 0).xyz;
  r0.yzw = r3.xyz + r0.yzw;
  r2.xyzw = r3.xyzw * r3.wwww + r2.xyzw;
  r1.x = r3.w + r1.x;
  r3.x = dot(float2(0.406737,0.913545012), r6.xy);
  r3.y = dot(float2(0.406737,0.913545012), r6.yz);
  r3.zw = cocMaxRadius * r3.xy;
  r3.zw = r3.zw * r1.yz + v1.xy;
  r3.zw = min(uv_clamp.xy, r3.zw);
  r1.w = colorTexture.SampleLevel(samLinear_s, r3.zw, 0).w;
  r3.z = min(r1.w, r4.w);
  r1.w = max(0, -r1.w);
  r1.w = saturate(r1.w + -r4.w);
  r0.x = max(r1.w, r0.x);
  r5.w = abs(r3.z);
  r3.xy = r5.ww * r3.xy;
  r3.xy = r3.xy * r1.yz;
  r3.xy = r3.xy * cocMaxRadius + v1.xy;
  r3.xy = min(uv_clamp.xy, r3.xy);
  r5.xyz = colorTexture.SampleLevel(samLinear_s, r3.xy, 0).xyz;
  r0.yzw = r5.xyz + r0.yzw;
  r2.xyzw = r5.xyzw * r5.wwww + r2.xyzw;
  r1.x = r5.w + r1.x;
  r3.x = dot(float2(0.743144989,0.669130981), r6.xy);
  r3.y = dot(float2(0.743144989,0.669130981), r6.yz);
  r3.zw = cocMaxRadius * r3.xy;
  r3.zw = r3.zw * r1.yz + v1.xy;
  r3.zw = min(uv_clamp.xy, r3.zw);
  r1.w = colorTexture.SampleLevel(samLinear_s, r3.zw, 0).w;
  r3.z = min(r1.w, r4.w);
  r1.w = max(0, -r1.w);
  r1.w = saturate(r1.w + -r4.w);
  r0.x = max(r1.w, r0.x);
  r5.w = abs(r3.z);
  r3.xy = r5.ww * r3.xy;
  r3.xy = r3.xy * r1.yz;
  r3.xy = r3.xy * cocMaxRadius + v1.xy;
  r3.xy = min(uv_clamp.xy, r3.xy);
  r5.xyz = colorTexture.SampleLevel(samLinear_s, r3.xy, 0).xyz;
  r0.yzw = r5.xyz + r0.yzw;
  r2.xyzw = r5.xyzw * r5.wwww + r2.xyzw;
  r1.x = r5.w + r1.x;
  r3.x = dot(float2(0.951057017,0.309017003), r6.xy);
  r3.y = dot(float2(0.951057017,0.309017003), r6.yz);
  r3.zw = cocMaxRadius * r3.xy;
  r3.zw = r3.zw * r1.yz + v1.xy;
  r3.zw = min(uv_clamp.xy, r3.zw);
  r1.w = colorTexture.SampleLevel(samLinear_s, r3.zw, 0).w;
  r3.z = min(r1.w, r4.w);
  r1.w = max(0, -r1.w);
  r1.w = saturate(r1.w + -r4.w);
  r0.x = max(r1.w, r0.x);
  r5.w = abs(r3.z);
  r3.xy = r5.ww * r3.xy;
  r3.xy = r3.xy * r1.yz;
  r3.xy = r3.xy * cocMaxRadius + v1.xy;
  r3.xy = min(uv_clamp.xy, r3.xy);
  r5.xyz = colorTexture.SampleLevel(samLinear_s, r3.xy, 0).xyz;
  r0.yzw = r5.xyz + r0.yzw;
  r2.xyzw = r5.xyzw * r5.wwww + r2.xyzw;
  r1.x = r5.w + r1.x;
  r3.x = dot(float2(0.994521976,-0.104529001), r6.xy);
  r3.y = dot(float2(0.994521976,-0.104529001), r6.yz);
  r3.zw = cocMaxRadius * r3.xy;
  r3.zw = r3.zw * r1.yz + v1.xy;
  r3.zw = min(uv_clamp.xy, r3.zw);
  r1.w = colorTexture.SampleLevel(samLinear_s, r3.zw, 0).w;
  r3.z = min(r1.w, r4.w);
  r1.w = max(0, -r1.w);
  r1.w = saturate(r1.w + -r4.w);
  r0.x = max(r1.w, r0.x);
  r5.w = abs(r3.z);
  r3.xy = r5.ww * r3.xy;
  r3.xy = r3.xy * r1.yz;
  r3.xy = r3.xy * cocMaxRadius + v1.xy;
  r3.xy = min(uv_clamp.xy, r3.xy);
  r5.xyz = colorTexture.SampleLevel(samLinear_s, r3.xy, 0).xyz;
  r0.yzw = r5.xyz + r0.yzw;
  r2.xyzw = r5.xyzw * r5.wwww + r2.xyzw;
  r1.x = r5.w + r1.x;
  r3.x = dot(float2(0.866024971,-0.5), r6.xy);
  r3.y = dot(float2(0.866024971,-0.5), r6.yz);
  r3.zw = cocMaxRadius * r3.xy;
  r3.zw = r3.zw * r1.yz + v1.xy;
  r3.zw = min(uv_clamp.xy, r3.zw);
  r1.w = colorTexture.SampleLevel(samLinear_s, r3.zw, 0).w;
  r3.z = min(r1.w, r4.w);
  r1.w = max(0, -r1.w);
  r1.w = saturate(r1.w + -r4.w);
  r0.x = max(r1.w, r0.x);
  r5.w = abs(r3.z);
  r3.xy = r5.ww * r3.xy;
  r3.xy = r3.xy * r1.yz;
  r3.xy = r3.xy * cocMaxRadius + v1.xy;
  r3.xy = min(uv_clamp.xy, r3.xy);
  r5.xyz = colorTexture.SampleLevel(samLinear_s, r3.xy, 0).xyz;
  r0.yzw = r5.xyz + r0.yzw;
  r2.xyzw = r5.xyzw * r5.wwww + r2.xyzw;
  r1.x = r5.w + r1.x;
  r3.x = dot(float2(0.587785006,-0.809017003), r6.xy);
  r3.y = dot(float2(0.587785006,-0.809017003), r6.yz);
  r3.zw = cocMaxRadius * r3.xy;
  r3.zw = r3.zw * r1.yz + v1.xy;
  r3.zw = min(uv_clamp.xy, r3.zw);
  r1.w = colorTexture.SampleLevel(samLinear_s, r3.zw, 0).w;
  r3.z = min(r1.w, r4.w);
  r1.w = max(0, -r1.w);
  r1.w = saturate(r1.w + -r4.w);
  r0.x = max(r1.w, r0.x);
  r5.w = abs(r3.z);
  r3.xy = r5.ww * r3.xy;
  r3.xy = r3.xy * r1.yz;
  r3.xy = r3.xy * cocMaxRadius + v1.xy;
  r3.xy = min(uv_clamp.xy, r3.xy);
  r5.xyz = colorTexture.SampleLevel(samLinear_s, r3.xy, 0).xyz;
  r0.yzw = r5.xyz + r0.yzw;
  r2.xyzw = r5.xyzw * r5.wwww + r2.xyzw;
  r1.x = r5.w + r1.x;
  r3.x = dot(float2(0.207911998,-0.978147984), r6.xy);
  r3.y = dot(float2(0.207911998,-0.978147984), r6.yz);
  r3.zw = cocMaxRadius * r3.xy;
  r3.zw = r3.zw * r1.yz + v1.xy;
  r3.zw = min(uv_clamp.xy, r3.zw);
  r1.w = colorTexture.SampleLevel(samLinear_s, r3.zw, 0).w;
  r3.z = min(r1.w, r4.w);
  r1.w = max(0, -r1.w);
  r1.w = saturate(r1.w + -r4.w);
  r0.x = max(r1.w, r0.x);
  r5.w = abs(r3.z);
  r3.xy = r5.ww * r3.xy;
  r3.xy = r3.xy * r1.yz;
  r3.xy = r3.xy * cocMaxRadius + v1.xy;
  r3.xy = min(uv_clamp.xy, r3.xy);
  r5.xyz = colorTexture.SampleLevel(samLinear_s, r3.xy, 0).xyz;
  r0.yzw = r5.xyz + r0.yzw;
  r2.xyzw = r5.xyzw * r5.wwww + r2.xyzw;
  r1.x = r5.w + r1.x;
  r3.x = dot(float2(-0.207911998,-0.978147984), r6.xy);
  r3.y = dot(float2(-0.207911998,-0.978147984), r6.yz);
  r3.zw = cocMaxRadius * r3.xy;
  r3.zw = r3.zw * r1.yz + v1.xy;
  r3.zw = min(uv_clamp.xy, r3.zw);
  r1.w = colorTexture.SampleLevel(samLinear_s, r3.zw, 0).w;
  r3.z = min(r1.w, r4.w);
  r1.w = max(0, -r1.w);
  r1.w = saturate(r1.w + -r4.w);
  r0.x = max(r1.w, r0.x);
  r5.w = abs(r3.z);
  r3.xy = r5.ww * r3.xy;
  r3.xy = r3.xy * r1.yz;
  r3.xy = r3.xy * cocMaxRadius + v1.xy;
  r3.xy = min(uv_clamp.xy, r3.xy);
  r5.xyz = colorTexture.SampleLevel(samLinear_s, r3.xy, 0).xyz;
  r0.yzw = r5.xyz + r0.yzw;
  r2.xyzw = r5.xyzw * r5.wwww + r2.xyzw;
  r1.x = r5.w + r1.x;
  r3.x = dot(float2(-0.587785006,-0.809017003), r6.xy);
  r3.y = dot(float2(-0.587785006,-0.809017003), r6.yz);
  r3.zw = cocMaxRadius * r3.xy;
  r3.zw = r3.zw * r1.yz + v1.xy;
  r3.zw = min(uv_clamp.xy, r3.zw);
  r1.w = colorTexture.SampleLevel(samLinear_s, r3.zw, 0).w;
  r3.z = min(r1.w, r4.w);
  r1.w = max(0, -r1.w);
  r1.w = saturate(r1.w + -r4.w);
  r0.x = max(r1.w, r0.x);
  r5.w = abs(r3.z);
  r3.xy = r5.ww * r3.xy;
  r3.xy = r3.xy * r1.yz;
  r3.xy = r3.xy * cocMaxRadius + v1.xy;
  r3.xy = min(uv_clamp.xy, r3.xy);
  r5.xyz = colorTexture.SampleLevel(samLinear_s, r3.xy, 0).xyz;
  r0.yzw = r5.xyz + r0.yzw;
  r2.xyzw = r5.xyzw * r5.wwww + r2.xyzw;
  r1.x = r5.w + r1.x;
  r3.x = dot(float2(-0.866024971,-0.5), r6.xy);
  r3.y = dot(float2(-0.866024971,-0.5), r6.yz);
  r3.zw = cocMaxRadius * r3.xy;
  r3.zw = r3.zw * r1.yz + v1.xy;
  r3.zw = min(uv_clamp.xy, r3.zw);
  r1.w = colorTexture.SampleLevel(samLinear_s, r3.zw, 0).w;
  r3.z = min(r1.w, r4.w);
  r1.w = max(0, -r1.w);
  r1.w = saturate(r1.w + -r4.w);
  r0.x = max(r1.w, r0.x);
  r5.w = abs(r3.z);
  r3.xy = r5.ww * r3.xy;
  r3.xy = r3.xy * r1.yz;
  r3.xy = r3.xy * cocMaxRadius + v1.xy;
  r3.xy = min(uv_clamp.xy, r3.xy);
  r5.xyz = colorTexture.SampleLevel(samLinear_s, r3.xy, 0).xyz;
  r0.yzw = r5.xyz + r0.yzw;
  r2.xyzw = r5.xyzw * r5.wwww + r2.xyzw;
  r1.x = r5.w + r1.x;
  r3.x = dot(float2(-0.994521976,-0.104528002), r6.xy);
  r3.y = dot(float2(-0.994521976,-0.104528002), r6.yz);
  r3.zw = cocMaxRadius * r3.xy;
  r3.zw = r3.zw * r1.yz + v1.xy;
  r3.zw = min(uv_clamp.xy, r3.zw);
  r1.w = colorTexture.SampleLevel(samLinear_s, r3.zw, 0).w;
  r3.z = min(r1.w, r4.w);
  r1.w = max(0, -r1.w);
  r1.w = saturate(r1.w + -r4.w);
  r0.x = max(r1.w, r0.x);
  r5.w = abs(r3.z);
  r3.xy = r5.ww * r3.xy;
  r3.xy = r3.xy * r1.yz;
  r3.xy = r3.xy * cocMaxRadius + v1.xy;
  r3.xy = min(uv_clamp.xy, r3.xy);
  r5.xyz = colorTexture.SampleLevel(samLinear_s, r3.xy, 0).xyz;
  r0.yzw = r5.xyz + r0.yzw;
  r2.xyzw = r5.xyzw * r5.wwww + r2.xyzw;
  r1.x = r5.w + r1.x;
  r3.x = dot(float2(-0.951056004,0.309017003), r6.xy);
  r3.y = dot(float2(-0.951056004,0.309017003), r6.yz);
  r3.zw = cocMaxRadius * r3.xy;
  r3.zw = r3.zw * r1.yz + v1.xy;
  r3.zw = min(uv_clamp.xy, r3.zw);
  r1.w = colorTexture.SampleLevel(samLinear_s, r3.zw, 0).w;
  r3.z = min(r1.w, r4.w);
  r1.w = max(0, -r1.w);
  r1.w = saturate(r1.w + -r4.w);
  r0.x = max(r1.w, r0.x);
  r5.w = abs(r3.z);
  r3.xy = r5.ww * r3.xy;
  r3.xy = r3.xy * r1.yz;
  r3.xy = r3.xy * cocMaxRadius + v1.xy;
  r3.xy = min(uv_clamp.xy, r3.xy);
  r5.xyz = colorTexture.SampleLevel(samLinear_s, r3.xy, 0).xyz;
  r0.yzw = r5.xyz + r0.yzw;
  r2.xyzw = r5.xyzw * r5.wwww + r2.xyzw;
  r1.x = r5.w + r1.x;
  r3.x = dot(float2(-0.743144989,0.669130981), r6.xy);
  r5.x = dot(float2(-0.406735986,0.913546026), r6.xy);
  r3.y = dot(float2(-0.743144989,0.669130981), r6.yz);
  r5.y = dot(float2(-0.406735986,0.913546026), r6.yz);
  r3.zw = cocMaxRadius * r3.xy;
  r3.zw = r3.zw * r1.yz + v1.xy;
  r3.zw = min(uv_clamp.xy, r3.zw);
  r1.w = colorTexture.SampleLevel(samLinear_s, r3.zw, 0).w;
  r3.z = min(r1.w, r4.w);
  r1.w = max(0, -r1.w);
  r1.w = saturate(r1.w + -r4.w);
  r0.x = max(r1.w, r0.x);
  r6.w = abs(r3.z);
  r3.xy = r6.ww * r3.xy;
  r3.xy = r3.xy * r1.yz;
  r3.xy = r3.xy * cocMaxRadius + v1.xy;
  r3.xy = min(uv_clamp.xy, r3.xy);
  r6.xyz = colorTexture.SampleLevel(samLinear_s, r3.xy, 0).xyz;
  r0.yzw = r6.xyz + r0.yzw;
  r2.xyzw = r6.xyzw * r6.wwww + r2.xyzw;
  r1.x = r6.w + r1.x;
  r3.xy = cocMaxRadius * r5.xy;
  r3.xy = r3.xy * r1.yz + v1.xy;
  r3.xy = min(uv_clamp.xy, r3.xy);
  r1.w = colorTexture.SampleLevel(samLinear_s, r3.xy, 0).w;
  r3.x = min(r1.w, r4.w);
  r1.w = max(0, -r1.w);
  r1.w = saturate(r1.w + -r4.w);
  r6.w = max(r1.w, r0.x);
  r3.w = abs(r3.x);
  r5.xy = r5.xy * r3.ww;
  r1.yz = r5.xy * r1.yz;
  r1.yz = r1.yz * cocMaxRadius + v1.xy;
  r1.yz = min(uv_clamp.xy, r1.yz);
  r3.xyz = colorTexture.SampleLevel(samLinear_s, r1.yz, 0).xyz;
  r0.xyz = r3.xyz + r0.yzw;
  r6.xyz = float3(0.0333333351,0.0333333351,0.0333333351) * r0.xyz;
  r0.xyzw = r3.xyzw * r3.wwww + r2.xyzw;
  r1.x = r3.w + r1.x;
  r1.y = cmp(r1.x == 0.000000);
  r1.z = r1.y ? 1.000000 : 0;
  r1.x = r1.x + r1.z;
  r0.xyzw = r0.xyzw / r1.xxxx;
  r0.xyzw = r1.yyyy ? r4.xyzw : r0.xyzw;
  r1.xyzw = r6.xyzw + -r0.xyzw;
  o0.xyzw = r6.wwww * r1.xyzw + r0.xyzw;
  return;
}
