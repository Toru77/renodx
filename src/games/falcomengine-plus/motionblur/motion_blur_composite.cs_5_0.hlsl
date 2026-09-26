// Motion Blur P6 - full-resolution overlay for the split-resolution path.
//
// Runs only when MotionBlurHalfRes is on. It is a WRITE-ONLY overlay, which is
// what lets the whole design work without a second full-resolution buffer:
//
//   * the full-res gather (side 0) already wrote every pixel -- blurred for the
//     short-motion tiles, and the untouched source for the long-motion tiles it
//     does not own;
//   * this pass overwrites ONLY the long-motion tiles, with the half-res gather's
//     result upsampled.
//
// Because it never READS the full-resolution target, there is no
// read-modify-write hazard and no aliasing: a UAV barrier between the two writers
// is sufficient.
//
// WHY A SPLIT, AND WHY SELECTED RATHER THAN BLENDED. Reducing the whole gather to
// half resolution and reconstructing is not a quality trade, it breaks the filter.
// The gather's early-out writes its centre sample unchanged, so a static frame
// routed through the reduction comes out box-downsampled and magnified -- the
// whole image softened with no motion blur anywhere. More generally, any pixel
// crossing the early-out threshold was being given a half-resolution pixel even
// when its streak was only a couple of pixels, which is precisely the regime where
// half resolution has nothing to hide behind. No reconstruction filter fixes that;
// the detail is gone before the gather runs.
//
// So the resolution is chosen per tile by how much smear is happening there: long
// streaks are dominated by the blur and tolerate half resolution, short ones are
// dominated by detail and do not. This is the same idea as the adaptive sample
// ladder -- spend resolution where the filter is working hard -- and the two line
// up deliberately, so the expensive 12-16 tap rungs are the ones that move to the
// cheap resolution.
//
// Selection, not blending. The composite makes the same decision the two gathers
// made, using the same jitter-free per-tile lookup (MBRoutingTile), so a pixel is
// either the full-res result or the half-res one and never a fractional mix. An
// earlier version interpolated a mask instead, which reintroduced exactly the
// "70% original, 30% half-res blur" blend this design exists to avoid.
//
// GTVBAO does the analogous thing for its half-res AO: evaluate the low-frequency
// term on a half-res grid, keep inputs and the final image full-res, and map
// between the two by block centre (gtvbao_atrous.cs_5_0.hlsl:52,
// gtvbao_common.hlsl:139). Here the blur accumulation is the low-frequency term
// and the base colour is the full-res detail.
//
// Load-only, no sampler, and it derives both resolutions from the push block:
// mb_working_w/h is FULL here and the half resolution is derived from it, so the
// half-res gather can run with halved values without this pass disagreeing.

// Pulls in shared.h (for shader_injection_data) plus MBRoutingTile, which MUST
// be the same lookup both gathers use or the three passes would disagree about
// which resolution owns a pixel.
#include "motion_blur_common.hlsli"

Texture2D<float2>   g_srcNeighborMax : register(t0);  // tile grid, routing only
Texture2D<float4>   g_srcHalf        : register(t1);  // half-res gather result
RWTexture2D<float4> g_outFull        : register(u0);  // SHARED with the full-res gather

[numthreads(8, 8, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID) {
  // int2(...), NOT (int2)(...): the parenthesised form is a comma expression, so
  // it evaluates to its second operand and broadcasts, silently producing
  // (height, height) instead of (width, height).
  int2 fullSize = int2(shader_injection_data.mb_working_w, shader_injection_data.mb_working_h);
  if (fullSize.x <= 0 || fullSize.y <= 0) return;
  // Ceil, to match how the half-res surfaces are allocated AND how the downsample
  // pass derives its own destination. A plain >>1 would floor, and on an odd
  // resolution the top row and column of the half-res texture would then never be
  // sampled and the clamp would cut a texel early.
  int2 halfSize = (fullSize + 1) >> 1;

  int2 p = int2(dispatchThreadID.xy);
  if (p.x >= fullSize.x || p.y >= fullSize.y) return;

  float2 uv = (float2(p) + 0.5) / float2(fullSize);

  // Same decision, same lookup, same threshold as both gathers. Short motion means
  // the full-res gather already wrote the correct pixel here, so leave it alone.
  float2 routingVMax = g_srcNeighborMax[MBRoutingTile(uv)];
  float halfResUV = max(shader_injection_data.mb_halfres_px, 0.0) / MB_REF_H;
  if (length(routingVMax) < halfResUV) return;

  // Texel centres must line up: full-res texel p covers source pixels [p, p+1],
  // whose centre in half-res texel units is (p + 0.5) * 0.5 - 0.5. Using
  // (p + 0.5) * 0.5 directly would shift the image by a quarter texel.
  float2 c = (float2(p) + 0.5) * 0.5 - 0.5;
  float2 f = frac(c);
  int2 b = (int2)floor(c);
  int2 hi = halfSize - 1;

  float4 s00 = g_srcHalf[clamp(b,                int2(0, 0), hi)];
  float4 s10 = g_srcHalf[clamp(b + int2(1, 0),   int2(0, 0), hi)];
  float4 s01 = g_srcHalf[clamp(b + int2(0, 1),   int2(0, 0), hi)];
  float4 s11 = g_srcHalf[clamp(b + int2(1, 1),   int2(0, 0), hi)];
  g_outFull[p] = lerp(lerp(s00, s10, f.x), lerp(s01, s11, f.x), f.y);
}
