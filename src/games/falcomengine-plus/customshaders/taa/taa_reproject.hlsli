#pragma once

// ---------------------------------------------------------------------------
// Verified external interface (coordinate convention, not algorithm).
//
// The game exposes: t0 color, t1 depth, t2 history, t3 motion (texel units),
// b2 texelSize/prevResolutionScale. TAA-0 reproduces the motion-lookup
// convention (max-depth tap among center + 4 diagonal neighbors selects the
// motion UV) as the initial reference. This convention is owned by the game
// interface; once TAA-0 proves accumulation, it can be inspected/replaced
// independently of the temporal algorithm.
// ---------------------------------------------------------------------------
float2 CustomTAA_Reproject(
    float2 uv,
    Texture2D<float4> depthTexture, SamplerState samPoint,
    Texture2D<float4> motionTexture, SamplerState samLinear,
    float2 texelSize, float2 prevResolutionScale,
    out float motionPixels, out float depthSpread) {
  float centerDepth = depthTexture.SampleLevel(samPoint, uv, 0).x;
  float2 bestOffset = float2(0.0, 0.0);
  float bestDepth = centerDepth;
  float minDepth = centerDepth;
  // Diagonal taps, matching the game's 5-tap footprint.
  const float2 kOffsets[4] = {
    float2(-1.0, -1.0), float2(1.0, -1.0),
    float2(-1.0, 1.0), float2(1.0, 1.0),
  };
  [unroll]
  for (int i = 0; i < 4; ++i) {
    float2 tapUV = saturate(uv + kOffsets[i] * texelSize);
    float tapDepth = depthTexture.SampleLevel(samPoint, tapUV, 0).x;
    if (tapDepth >= bestDepth) {
      bestDepth = tapDepth;
      bestOffset = kOffsets[i];
    }
    minDepth = min(minDepth, tapDepth);
  }
  // Relative depth discontinuity from the same 5 taps (no new samples):
  // ~0 = locally flat, larger = silhouette/edge. Relative form only assumes
  // monotonic depth (larger = nearer, per the max-wins selection above).
  depthSpread = (bestDepth - minDepth) / max(bestDepth, 1e-4);
  float2 motionUV = saturate(uv + bestOffset * texelSize);
  float2 motionTexels = motionTexture.SampleLevel(samLinear, motionUV, 0).xy;
  // No explicit jitter-delta correction, by evidence (not assumption):
  // - Game motion vectors already carry the frame-to-frame jitter delta.
  //   E.g. sora1st foliage (staticfoliage_0x39F91AE8:246-252) computes
  //   (prevClip - curClip) in pixels and adds jitterDiff_g before writing
  //   the motion target; the same jitterDiff_g + motion pattern appears in
  //   the sky and all foliage shaders. The TAA cbuffer's jitter_g field is
  //   therefore vestigial: the historyUV formula below already reprojects
  //   through jittered motion, and an extra +/-jitter term here would
  //   double-compensate.
  // - Runtime corroboration: feedback=0 shows raw upstream jitter while
  //   feedback=0.9 converges stably, which a missing jitter delta would not.
  // Motion magnitude in pixels/frame (texture values are texel units).
  // Jitter-inclusive: a static camera still reports roughly 0-1px from the
  // frame-to-frame jitter delta alone. TAA-5 feedback scaling must therefore
  // stay small enough not to punish subpixel jitter as "motion".
  motionPixels = length(motionTexels);
  float2 historyUV = uv * prevResolutionScale + motionTexels * texelSize;
  return historyUV;
}
