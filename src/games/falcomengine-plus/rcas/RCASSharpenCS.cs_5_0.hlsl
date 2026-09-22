// RCASSharpenCS.cs_5_0.hlsl — post-TAA Robust Contrast Adaptive Sharpening.
//
// Thin wrapper over rcas_common.hlsli (single shared RCAS implementation,
// also consumed by the Sora2nd temporal-upscaling blit). Behavior is
// bit-identical to the former inline implementation: same taps, same order,
// same equations. See rcas_common.hlsli for the math and its deliberate
// deviations from AMD FidelityFX FSR 1 ffx_fsr1.h (MIT, AMD).
//
// This pass NEVER feeds TAA history: it reads the finished TAA output and
// writes an owned temp resource. History isolation is enforced CPU-side
// (addon.cpp): the unsharpened TAA output is copied to a separate history
// buffer before sharpening, and only that copy is ever bound as t2.
//
// NOTE (temporal upscaler era): this compute pass now serves Sora1st only,
// plus Sora2nd while its blit upscaler is off. Sora2nd with the upscaler on
// sharpens inside the blit via the same shared core (no double sharpening).

#include "rcas_common.hlsli"

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

  float3 outColor = RCASSharpen(b, d, e, f, h, con, (g_denoise > 0.5f) ? 1.0f : 0.0f);
  g_dst[sp] = float4(outColor, alpha);
}
