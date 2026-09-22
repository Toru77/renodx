// rcas_common.hlsli — shared RCAS core (single implementation, two call sites).
//
// RCAS mathematics ported from AMD FidelityFX Super Resolution 1 (FSR 1)
// ffx_fsr1.h, function FsrRcasF (32-bit float path):
//   Copyright (c) 2021 Advanced Micro Devices, Inc. (MIT License).
// Only FSR_RCAS_F + PASSTHROUGH_ALPHA behavior is ported; EASU, CAS,
// FSR_RCAS_DENOISE-as-default and the 16-bit packed path are not used.
//
// Deviations from the reference (deliberate, documented):
// 1. Exact 1.0/x replaces the APrxMedRcp/ARcp approximations.
// 2. HDR peak adaptation: reference solves the lobe limiter against a {0,1}
//    range (peakC = 1.0). Here peak = max(1.0, localMax) per channel, so LDR
//    input reproduces the reference exactly while HDR highlights keep a
//    meaningful anti-clip limiter instead of a miscalibrated one.
// 3. No output clamp exists in the reference and none is added here: HDR
//    values pass through unclipped.
// 4. Caller supplies the 5 taps and the (possibly motion-interpolated) lobe
//    multiplier; no textures, samplers, or UAVs inside (usable from any
//    shader stage: compute passes and pixel-shader replacements alike).
//
// Callers:
// - rcas/RCASSharpenCS.cs_5_0.hlsl (Sora1st legacy compute pass + Sora2nd
//   fallback while the upscaler is off): thin wrapper, identical behavior.
// - sora2nd/taa/blit_0xC9FA40B7.ps_5_0.hlsl (temporal upscaler): fused RCAS
//   after output-resolution accumulation; ring taps are center-motion-
//   reprojected bilinear history (see blit file for the cost rationale).

#ifndef FALCOMENGINE_PLUS_RCAS_COMMON_HLSLI_
#define FALCOMENGINE_PLUS_RCAS_COMMON_HLSLI_

#define RCAS_LIMIT (0.25 - (1.0 / 16.0))

// b/d/e/f/h: cross taps (b=north, d=west, e=center, f=east, h=south).
// con: lobe multiplier 0..1 (linear response; 0 = exact passthrough since
//   output reduces to e for any lobe when the ring equals the center).
// denoiseOn: >0.5 applies the reference nz grain reduction (lobe *= nz,
//   0.5..1.0 range); reference default off.
float3 RCASSharpen(float3 b, float3 d, float3 e, float3 f, float3 h,
                   float con, float denoiseOn) {
  // Luma times 2 (reference weights, verbatim). Computed only when denoise
  // is on: zero output change when off, ~10 ALU saved per pixel.
  float nz = 1.0;
  if (denoiseOn > 0.5) {
    float bL = b.b * 0.5 + (b.r * 0.5 + b.g);
    float dL = d.b * 0.5 + (d.r * 0.5 + d.g);
    float eL = e.b * 0.5 + (e.r * 0.5 + e.g);
    float fL = f.b * 0.5 + (f.r * 0.5 + f.g);
    float hL = h.b * 0.5 + (h.r * 0.5 + h.g);
    nz = 0.25 * bL + 0.25 * dL + 0.25 * fL + 0.25 * hL - eL;
    nz = saturate(abs(nz) / max(max(max(bL, dL), max(eL, max(fL, hL)))
        - min(min(bL, dL), min(eL, min(fL, hL))), 1e-6));
    nz = -0.5 * nz + 1.0;
  }

  // Min and max of ring (per channel).
  float3 mn4 = min(min(b, d), min(f, h));
  float3 mx4 = max(max(b, d), max(f, h));

  // Limiter peaks: reference peakC.x = 1.0. Generalized to
  // peak = max(1.0, localMax) so LDR matches the reference exactly
  // while HDR keeps headroom-aware limiting. peakC.y = -4*peak.
  float3 peak = max(float3(1.0, 1.0, 1.0), max(mx4, e));
  float3 hitMin = min(mn4, e) / (4.0 * mx4);
  float3 hitMax = (peak - max(mx4, e)) / (4.0 * mn4 - 4.0 * peak);
  float3 lobe3 = max(-hitMin, hitMax);
  // HLSL max() follows fmax semantics (non-NaN wins), matching the
  // reference behavior on degenerate flat fields (e.g. mn == peak).
  float lobe = max(-RCAS_LIMIT, min(max(lobe3.x, max(lobe3.y, lobe3.z)), 0.0)) * con;
  if (denoiseOn > 0.5) {
    lobe *= nz;
  }

  // Resolve (reference equation, verbatim).
  float rcpL = 1.0 / (4.0 * lobe + 1.0);
  return (lobe * (b + d + f + h) + e) * rcpL;
}

#endif  // FALCOMENGINE_PLUS_RCAS_COMMON_HLSLI_
