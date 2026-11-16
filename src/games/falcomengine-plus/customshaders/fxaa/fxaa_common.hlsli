// fxaa_common.hlsli — shared wrapper for the FXAA 3.11 quality passes.
//
// Included AFTER the variant defines + the reference header, e.g.:
//   #define FXAA_PC 1
//   #define FXAA_HLSL_5 1
//   #define FXAA_QUALITY__PRESET 12
//   #define FXAA_LUMA_SEPARATE_R8 1
//   #define FXAA_GATHER4_ALPHA 0
//   #include "../reference/taa/Fxaa3_11.h"
//   #include "fxaa_common.hlsli"
//
// The reference file (reference/taa/Fxaa3_11.h) is never modified;
// FxaaPixelShader() runs verbatim. Deliberate deviations at this call
// site, documented:
// 1. Compute dispatch instead of a fullscreen pixel shader: pos is derived
//    from SV_DispatchThreadID; bilinear fetches go through an owned static
//    linear clamp sampler (reference checklist step 7).
// 2. Console-only inputs (fxaaConsolePosPos, 360 bias taps, console
//    uniforms) are zero: the FXAA_PC quality path never reads them.
// 3. Source alpha passes through untouched. The reference returns lumaM in
//    .a on both exit paths; downstream (RCAS + game) needs alpha intact.
// 4. GATHER4_ALPHA is forced 0: the SampleLevel 4-tap path runs everywhere
//    instead of GatherRed on the R8 luma texture (single-channel gather is
//    not guaranteed on D3D11). Same math, more fetches.
// 5. No HDR clipping anywhere: color is never saturated; luma compression
//    (see FXAALuma.cs_5_0.hlsl) only affects edge-detection weights.
//
// This pass NEVER feeds TAA history: it reads the finished TAA output and
// writes an owned temp resource. History isolation is enforced CPU-side
// (addon.cpp): the un-FXAA'd TAA output is copied to the history buffer
// before this runs, and only that copy is ever bound as t2.

cbuffer FXAACB : register(b13) {
  float g_width;             // target width in pixels (float; converted to int)
  float g_height;            // target height in pixels
  float g_subpix;            // [0..1] sub-pixel aliasing removal (reference default 0.75)
  float g_edgeThreshold;     // local-contrast gate (reference default 0.166)
  float g_edgeThresholdMin;  // dark trim (reference high quality 0.0625)
  float g_lumaMode;          // luma prepass only; ignored by the quality pass
};

Texture2D<float4> g_src : register(t0);   // un-FXAA'd TAA output, raw encoding (linear-variant SRV: no sRGB decode)
Texture2D<float4> g_luma : register(t1);  // precomputed perceptual luma in .x (R8_UNORM resource)
RWTexture2D<float4> g_dst : register(u0); // owned FXAA temp target

SamplerState g_fxaa_sampler : register(s0) {
  Filter = MIN_MAG_MIP_LINEAR;
  AddressU = Clamp;
  AddressV = Clamp;
};

[numthreads(8, 8, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  int2 dims = int2((int)(g_width + 0.5f), (int)(g_height + 0.5f));
  if (tid.x >= (uint)dims.x || tid.y >= (uint)dims.y) return;
  int2 sp = int2(tid.xy);
  float2 rcpFrame = float2(1.0f / max(g_width, 1e-6f), 1.0f / max(g_height, 1e-6f));
  float2 pos = (float2(sp) + 0.5f) * rcpFrame;
  FxaaTex fxaaTex;
  fxaaTex.smpl = g_fxaa_sampler;
  fxaaTex.tex = g_src;
  FxaaTex fxaaLumaTex;
  fxaaLumaTex.smpl = g_fxaa_sampler;
  fxaaLumaTex.tex = g_luma;
  FxaaFloat4 result = FxaaPixelShader(
      pos,
      FxaaFloat4(0.0f, 0.0f, 0.0f, 0.0f),
      fxaaTex,
      fxaaLumaTex,
      fxaaTex,  // 360-only exponent-bias aliases; reference docs: same input when not 360
      fxaaTex,
      rcpFrame,
      FxaaFloat4(0.0f, 0.0f, 0.0f, 0.0f),
      FxaaFloat4(0.0f, 0.0f, 0.0f, 0.0f),
      FxaaFloat4(0.0f, 0.0f, 0.0f, 0.0f),
      g_subpix,
      g_edgeThreshold,
      g_edgeThresholdMin,
      0.0f,
      0.0f,
      0.0f,
      FxaaFloat4(0.0f, 0.0f, 0.0f, 0.0f));
  g_dst[sp] = float4(result.xyz, g_src.Load(int3(sp, 0)).a);
}
