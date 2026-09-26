// Motion Blur P0 - optional half-resolution colour reduction.
//
// Runs only when MotionBlurHalfRes is on, and only on frames that actually gather.
// It exists so the gather can run at half resolution without aliasing its colour
// input: a half-res gather samples the colour on a 2x sparser grid, and point
// fetching a full-res texture on that grid would silently drop three quarters of
// the image and crawl on fine detail.
//
// A 4-tap box is the right filter and the right place for it. Doing the reduction
// here costs one read per source texel; folding the same 4 taps into every gather
// tap instead would cost four reads per tap, which at 16 taps is roughly an order
// of magnitude more colour traffic.
//
// Load-only, no sampler. That is deliberate and applies to every shader in this
// chain: the filter must not depend on a sampler descriptor being valid, because a
// null one makes D3D11 sampling return zero and silently turns the whole gather
// into a passthrough. It happened once, and a shared GTVBAO-owned sampler handle
// was the cause.
//
// Dimensions come from the push block, so the same shader serves any resolution
// and no extra push fields are needed. mb_working_w/h is FULL resolution here;
// the gather is the pass that sees the halved values.

#include "../shared.h"

Texture2D<float4>   g_srcColor : register(t0);
RWTexture2D<float4> g_outColor : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID) {
  // int2(...), NOT (int2)(...): the parenthesised form is a comma expression, so
  // it evaluates to its second operand and broadcasts, silently producing
  // (height, height) instead of (width, height).
  int2 srcSize = int2(shader_injection_data.mb_working_w, shader_injection_data.mb_working_h);
  if (srcSize.x <= 0 || srcSize.y <= 0) return;
  int2 dstSize = (srcSize + 1) >> 1;

  int2 p = int2(dispatchThreadID.xy);
  if (p.x >= dstSize.x || p.y >= dstSize.y) return;

  // Ceil division means the last row/column can map 1:1, and an odd source width
  // makes the final column reach one texel past the edge. Clamping the indices is
  // what keeps that case well defined; the odd texel is simply counted twice,
  // which is a 1-in-256 boundary artifact on a signal that is about to be blurred
  // and bilinearly upsampled anyway.
  int2 base = p * 2;
  int2 hi = srcSize - 1;
  float4 sum = g_srcColor[clamp(base,                    int2(0, 0), hi)]
             + g_srcColor[clamp(base + int2(1, 0),      int2(0, 0), hi)]
             + g_srcColor[clamp(base + int2(0, 1),      int2(0, 0), hi)]
             + g_srcColor[clamp(base + int2(1, 1),      int2(0, 0), hi)];

  // Alpha is averaged with the rest. The gather forwards the centre sample's
  // alpha straight to the blit, and the game's tonemap consumes RGB from t0, so
  // there is nothing to preserve here.
  g_outColor[p] = sum * 0.25;
}
