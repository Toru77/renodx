///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// falcomengine-plus GTVBAO — À-trous wavelet spatial filter (R3 + perf pass)
//
// SVGF-lite: edge-preserving wavelet blur with doubling stride. Replaces the
// XeGTAO bilateral chain when GTVBAO_atrous_enabled is on. Each dispatch is one
// iteration; the addon runs 3 iterations with strides 1, 2, 4.
//
// Perf optimizations:
//   • Normals come pre-decoded from gtvbao_normal_prep (no per-tap sincos/sqrt)
//   • Normal edge-stop uses power-of-two exponent via repeated squaring
//   • Flat-region early-out: when the ±stride AO neighbors match the center,
//     the pixel passes through without running the kernel
//   • Final iteration folds the ×OCCLUSION_TERM_SCALE multiply-back
//     (GTVBAO_denoise_is_last_pass), removing the separate scale-back dispatch
//
// Bindings (atrous_layout): t0 = AO src (raw /1.5 domain, uint 8-bit packed),
// t1 = depth MIP0, t2 = pre-decoded normals (RGBA16F) → u0 = AO dst.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "gtvbao_common.hlsl"

Texture2D<uint>    g_srcAtrousAO      : register(t0);
Texture2D<float>   g_srcAtrousDepth   : register(t1);
Texture2D<float4>  g_srcAtrousNormal  : register(t2);   // pre-decoded by gtvbao_normal_prep
SamplerState       g_samplerPointClamp : register(s0);
RWTexture2D<uint>  g_outAtrousAO      : register(u0);

// AO difference below which a neighbor counts as "flat" for the early-out probe.
static const float kAtrousFlatEps = 0.02f;

[numthreads(GT_VBAO_NUMTHREADS_X, GT_VBAO_NUMTHREADS_Y, 1)]
void main(uint2 dt : SV_DispatchThreadID)
{
  uint width, height;
  g_srcAtrousAO.GetDimensions(width, height);
  if (dt.x >= width || dt.y >= height) return;

  uint nw, nh;
  g_srcAtrousNormal.GetDimensions(nw, nh);

  GTAOConstants consts = BuildGTAOConstants(uint2(width, height));

  const float step = max(1.0, GTVBAO_atrous_step);
  const float sigmaZ = max(0.01, GTVBAO_atrous_depth_sigma);
  // Quantize normal edge-stop exponent to a power of two: x^(2^sqSteps),
  // implemented with repeated squaring instead of pow().
  const int sqSteps = (int)round(clamp(log2(max(GTVBAO_atrous_normal_sigma, 1.0)), 0.0, 6.0));
  const bool isLastPass = GTVBAO_denoise_is_last_pass > 0.5f;

  float2 uv = (float2(dt) + 0.5) * consts.ViewportPixelSize;
  float centerZ = g_srcAtrousDepth.SampleLevel(g_samplerPointClamp, uv, 0);
  int2 centerTc = min(int2(floor(uv * float2(nw, nh))), int2(nw - 1, nh - 1));
  float3 centerN = g_srcAtrousNormal.Load(int3(centerTc, 0)).xyz;

  float centerAO = (float)g_srcAtrousAO.Load(int3(dt, 0)) * (1.0f / 255.0f);

  // ── Flat-region early-out ──
  // If the four ±stride AO neighbors match the center within kAtrousFlatEps,
  // the filtered result is numerically ≈ center — skip the kernel entirely.
  bool flat = true;
  {
    int2 probes[4] = { int2(-step, 0), int2(step, 0), int2(0, -step), int2(0, step) };
    [unroll]
    for (int p = 0; p < 4; ++p) {
      int2 pc = clamp(int2(dt) + probes[p], int2(0, 0), int2(width - 1, height - 1));
      float v = (float)g_srcAtrousAO.Load(int3(pc, 0)) * (1.0f / 255.0f);
      if (abs(v - centerAO) > kAtrousFlatEps) { flat = false; break; }
    }
    if (flat) {
      // Last iteration folds the ×OCCLUSION_TERM_SCALE multiply-back — this
      // MUST match the kernel exit below, otherwise flat regions stay in the
      // raw (÷1.5) domain and render falsely darkened ("inverted" AO).
      float outFlat = centerAO;
      if (isLastPass) outFlat = saturate(outFlat * GT_VBAO_OCCLUSION_TERM_SCALE);
      g_outAtrousAO[dt] = (uint)(saturate(outFlat) * 255.0f + 0.5f);
      return;
    }
  }

  // B3-spline weights [1,4,6,4,1]/16 — outer product gives the 5×5 kernel.
  static const float kw[5] = { 1.0 / 16.0, 4.0 / 16.0, 6.0 / 16.0, 4.0 / 16.0, 1.0 / 16.0 };

  const float centerW = kw[2] * kw[2];   // (6/16)²
  float sum = centerAO * centerW;
  float wsum = centerW;

  [unroll]
  for (int dy = -2; dy <= 2; ++dy) {
    [unroll]
    for (int dx = -2; dx <= 2; ++dx) {
      if (dx == 0 && dy == 0) continue;

      int2 npc = clamp(int2(dt) + int2(dx, dy) * int2(step, step), int2(0, 0), int2(width - 1, height - 1));

      float sampleAO = (float)g_srcAtrousAO.Load(int3(npc, 0)) * (1.0f / 255.0f);

      float2 nuv = (float2(npc) + 0.5) * consts.ViewportPixelSize;
      float nz = g_srcAtrousDepth.SampleLevel(g_samplerPointClamp, nuv, 0);

      // Depth edge-stop: relative test scaled by world-space pixel footprint so
      // distant geometry isn't over-rejected.
      float worldPx = abs(centerZ) * abs(consts.NDCToViewMul_x_PixelSize.y) * step;
      float dzW = exp(-abs(centerZ - nz) / max(sigmaZ * max(worldPx, 1e-4), 1e-4));

      // Normal edge-stop — repeated squaring (x^(2^sqSteps)), no pow()
      int2 snTc = min(int2(floor(nuv * float2(nw, nh))), int2(nw - 1, nh - 1));
      float nd = saturate(dot(centerN, g_srcAtrousNormal.Load(int3(snTc, 0)).xyz));
      float nW = nd;
      [unroll]
      for (int q = 0; q < 6; ++q) {
        if (q < sqSteps) nW *= nW;
      }

      float weight = kw[dx + 2] * kw[dy + 2] * dzW * nW;
      sum += sampleAO * weight;
      wsum += weight;
    }
  }

  float outAO = sum / max(wsum, 1e-5f);
  // Final iteration folds the ×OCCLUSION_TERM_SCALE multiply-back that used to
  // live in a dedicated passthrough dispatch.
  if (isLastPass) outAO = saturate(outAO * GT_VBAO_OCCLUSION_TERM_SCALE);
  g_outAtrousAO[dt] = (uint)(saturate(outAO) * 255.0f + 0.5f);
}
