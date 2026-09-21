// RCASSharpenCS.cs_5_0.hlsl — post-TAA Robust Contrast Adaptive Sharpening.
//
// RCAS mathematics ported from AMD FidelityFX Super Resolution 1 (FSR 1)
// ffx_fsr1.h, function FsrRcasF (32-bit float path) with FsrRcasCon sharpness
// setup done on the CPU side (see addon.cpp RunRCASAfterTAA):
//   Copyright (c) 2021 Advanced Micro Devices, Inc. (MIT License).
// Only FSR_RCAS_F + PASSTHROUGH_ALPHA behavior is ported; EASU, CAS,
// FSR_RCAS_DENOISE and the 16-bit packed path are intentionally not used.
//
// Deviations from the reference (deliberate, documented):
// 1. Loads are integer texel loads (compute) instead of sampler fetches.
// 2. Exact 1.0/x replaces the APrxMedRcp/ARcp approximations.
// 3. The noise term (nz) is computed but only applied when g_denoise is on
//    (reference FSR_RCAS_DENOISE behavior, default off; film grain, if any,
//    belongs after sharpening per AMD guidance).
// 4. HDR peak adaptation: reference solves the lobe limiter against a {0,1}
//    range (peakC = 1.0). Here peak = max(1.0, localMax) per channel, so LDR
//    input reproduces the reference exactly while HDR highlights keep a
//    meaningful anti-clip limiter instead of a miscalibrated one.
// 5. No output clamp exists in the reference and none is added here: HDR
//    values pass through unclipped. Alpha is passed through untouched.
//
// This pass NEVER feeds TAA history: it reads the finished TAA output and
// writes an owned temp resource. History isolation is enforced CPU-side
// (addon.cpp): the unsharpened TAA output is copied to a separate history
// buffer before sharpening, and only that copy is ever bound as t2.

cbuffer RCASCB : register(b13) {
  float g_con;     // base lobe multiplier 0..1 from UI Sharpening (linear).
                   // S=0 never dispatches (CPU passthrough).
  float g_width;   // target width in pixels (float; converted to int)
  float g_height;  // target height in pixels (float; converted to int)
  float g_denoise; // 0 = reference default (full sharpening); 1 = apply the
                   // noise term (lobe *= nz), reducing sharpening where local
                   // variation looks grain-like (0.5..1.0 scale range).
  float g_motionOn;       // 1 = Motion Sharpening UI on AND motion view live.
                          // 0 = base path exactly (motion lookup skipped).
  float g_motionThreshold;// px: motion below this adds no sharpening (jitter floor).
  float g_motionRange;    // px of additional motion to reach the motion target.
  float g_motionResponse; // pow curvature of the threshold->target transition.
  float g_conMotion;      // ABSOLUTE motion target (NOT base+target):
                          // max motion interpolates base -> motion, never sums.
  float g_debug;          // 0 = normal sharpen; 1 = motion-strength heatmap
                          // (green = no boost, yellow = partial, red = full).
};

Texture2D<float4> g_src : register(t0);     // unsharpened TAA output
Texture2D<float4> g_motion : register(t1);  // game motion buffer (texel units, jitter-inclusive)
RWTexture2D<float4> g_dst : register(u0);   // owned sharpened temp target

#define RCAS_LIMIT (0.25 - (1.0 / 16.0))

float4 RCASLoad(int2 p, int2 dims) {
  p = clamp(p, int2(0, 0), dims - int2(1, 1));
  return g_src.Load(int3(p, 0));
}

[numthreads(8, 8, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  int2 dims = int2((int)(g_width + 0.5), (int)(g_height + 0.5));
  if (tid.x >= (uint)dims.x || tid.y >= (uint)dims.y) return;
  int2 sp = int2(tid.xy);

  // Motion-adaptive strength (Stage 2): absolute interpolation between the
  // base and motion targets -- never summed. Jitter-level motion stays under
  // the threshold, so static pixels keep exactly the base strength.
  // Motion buffer shares output UV space (same assumption the TAA shader
  // itself makes), so integer texel loads apply. Skipped entirely when off.
  float con = g_con;
  float motionT = 0.0;
  if (g_motionOn > 0.5) {
    float motionPixels = length(g_motion.Load(int3(sp, 0)).xy);
    motionT = saturate((motionPixels - g_motionThreshold) / max(g_motionRange, 1e-4));
    motionT = pow(motionT, g_motionResponse);
    con = lerp(g_con, g_conMotion, motionT);
  }

  // Motion-strength heatmap (diagnostic): visualizes the same mask the real
  // path uses (green = no boost, yellow = partial, red = full motion target).
  // Skips color work; normal path below is untouched when debug is off.
  if (g_debug > 0.5) {
    float3 heat = lerp(float3(0.0, 1.0, 0.0), float3(1.0, 1.0, 0.0), saturate(motionT * 2.0));
    heat = lerp(heat, float3(1.0, 0.0, 0.0), saturate(motionT * 2.0 - 1.0));
    g_dst[sp] = float4(heat, 1.0);
    return;
  }

  // 5-tap cross (reference b/d/e/f/h).
  float3 b = RCASLoad(sp + int2(0, -1), dims).rgb;
  float3 d = RCASLoad(sp + int2(-1, 0), dims).rgb;
  float3 e = RCASLoad(sp, dims).rgb;
  float alpha = RCASLoad(sp, dims).a;
  float3 f = RCASLoad(sp + int2(1, 0), dims).rgb;
  float3 h = RCASLoad(sp + int2(0, 1), dims).rgb;

  // Luma times 2 (reference weights, verbatim). Needed for the optional
  // noise term below; cheap since the taps are already loaded.
  float bL = b.b * 0.5 + (b.r * 0.5 + b.g);
  float dL = d.b * 0.5 + (d.r * 0.5 + d.g);
  float eL = e.b * 0.5 + (e.r * 0.5 + e.g);
  float fL = f.b * 0.5 + (f.r * 0.5 + f.g);
  float hL = h.b * 0.5 + (h.r * 0.5 + h.g);

  // Noise detection (reference equations, exact rcp instead of the
  // approximation). Normalized high-pass in [0,1], shaped to nz in
  // [0.5,1.0]: structured edges keep ~1.0, grain-like variation drops
  // toward 0.5. Only applied when g_denoise is on (reference default off).
  float nz = 0.25 * bL + 0.25 * dL + 0.25 * fL + 0.25 * hL - eL;
  nz = saturate(abs(nz) / max(max(max(bL, dL), max(eL, max(fL, hL)))
      - min(min(bL, dL), min(eL, min(fL, hL))), 1e-6));
  nz = -0.5 * nz + 1.0;

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
  if (g_denoise > 0.5) {
    lobe *= nz;
  }

  // Resolve (reference equation, verbatim).
  float rcpL = 1.0 / (4.0 * lobe + 1.0);
  float3 outColor = (lobe * (b + d + f + h) + e) * rcpL;
  g_dst[sp] = float4(outColor, alpha);
}
