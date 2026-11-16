// FXAALuma.cs_5_0.hlsl — FXAA 3.11 luma prepass (FXAA_LUMA_SEPARATE_R8 path).
//
// The reference algorithm needs perceptual luma, but the TAA output is HDR
// linear float with no luma in alpha. Instead of FXAA_GREEN_AS_LUMA (blind
// on non-green content), this pass precomputes luma into an owned R8_UNORM
// texture — the exact integration the reference header's SEPARATE_R8 path
// was added for (see reference/taa/Fxaa3_11.h header notes).
//
// Luma encoding (deliberate, documented; never clips HDR):
// - LDR perceptual input (sRGB-encoded target): luma = dot(rgb, weights)
//   directly. sRGB encoding is already perceptual, matching the reference
//   gamma-2.0 luma assumption closely.
// - Linear HDR input (float target): luma = sqrt(Y / (1 + Y)) with
//   Y = dot(max(rgb, 0), weights). Reinhard-style compression bounds
//   highlights asymptotically toward 1.0 without clipping, and mid-tones
//   land near the reference calibration (sqrt(0.18) = 0.42 vs
//   sqrt(0.152) = 0.39), so stock thresholds are a sane starting point.
// The mode is selected CPU-side from the target format. Color data is only
// read here, never modified, so HDR values pass through the stage unclipped.

cbuffer FXAACB : register(b13) {
  float g_width;              // target width in pixels (float; converted to int)
  float g_height;             // target height in pixels
  float g_subpix;             // unused by the luma pass (shared push layout)
  float g_edgeThreshold;      // unused by the luma pass (shared push layout)
  float g_edgeThresholdMin;   // unused by the luma pass (shared push layout)
  float g_lumaMode;           // 0 = LDR perceptual input, 1 = linear HDR input
};

Texture2D<float4> g_src : register(t0);     // un-FXAA'd TAA output (linear-variant SRV: no sRGB decode)
RWTexture2D<float> g_lumaOut : register(u0); // owned R8_UNORM luma target

[numthreads(8, 8, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  int2 dims = int2((int)(g_width + 0.5f), (int)(g_height + 0.5f));
  if (tid.x >= (uint)dims.x || tid.y >= (uint)dims.y) return;
  int2 sp = int2(tid.xy);
  float4 c = g_src.Load(int3(sp, 0));
  float y = dot(max(c.rgb, float3(0.0f, 0.0f, 0.0f)), float3(0.299f, 0.587f, 0.114f));
  float l = (g_lumaMode > 0.5f) ? sqrt(y / (1.0f + y)) : y;
  g_lumaOut[sp] = l;
}
