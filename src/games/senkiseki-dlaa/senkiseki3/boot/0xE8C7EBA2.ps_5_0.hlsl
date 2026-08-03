// ── Modified 0xE8C7EBA2 (pre-FXAA composite) for DLAA ──
// Applies the camera jitter as an image-level UV shift (sampling the scene at a
// jittered UV) instead of jittering the geometry in the VS. This produces the
// jittered pre-FXAA color buffer that RunDLAA feeds to DLSS, without perturbing
// per-vertex texture sampling (avoids mouth/eye atlas seam shimmer).
//
// The sub-pixel shift MUST be a true fractional resample. The game binds a
// POINT/nearest sampler at s0, which would quantize a 0.5px jitter to a whole
// pixel (breaking DLSS alignment entirely — jitter on/off and sign become
// meaningless). So the jittered sample is done with manual bilinear
// interpolation (4 texel Loads) instead of SampleLevel. With jitter off, uv is
// exactly at texel centers, so this yields the exact texel (== point sampling).
//
// SPDX-License-Identifier: MIT

#include "../../shared.h"

SamplerState PointClampSampler_s : register(s0);
Texture2D<float4> ColorBuffer : register(t0);

void main(
  float4 v0 : SV_POSITION0,
  float2 v1 : TEXCOORD0,
  out float4 o0 : SV_TARGET0)
{
  float4 r0;

  // Pure passthrough. Sample at the PIXEL CENTER (SV_Position) instead of the
  // interpolated UV (v1): at the jittered viewport, v1 is shifted by the jitter,
  // so sampling with it would make the manual bilinear below FILTER the jittered
  // scene — blurring it and destroying the subpixel sample pattern DLSS needs.
  // SV_Position is the pixel itself (viewport-independent), so v0.xy/texsize
  // samples the exact texel (no filtering) and the geometry's rasterization
  // jitter passes through unchanged.
  uint texW, texH;
  ColorBuffer.GetDimensions(texW, texH);
  float2 uv = float2(v0.x, v0.y) / float2(texW, texH);

  // Manual bilinear (frac == 0 at texel centers => exact texel, pure passthrough).
  float2 texCoord = uv * float2(texW, texH) - 0.5f;
  float2 base = floor(texCoord);
  float2 frac = saturate(texCoord - base);
  int2 i00 = int2(clamp(base, float2(0, 0), float2(texW - 1, texH - 1)));
  int2 i10 = int2(clamp(base + float2(1, 0), float2(0, 0), float2(texW - 1, texH - 1)));
  int2 i01 = int2(clamp(base + float2(0, 1), float2(0, 0), float2(texW - 1, texH - 1)));
  int2 i11 = int2(clamp(base + float2(1, 1), float2(0, 0), float2(texW - 1, texH - 1)));
  float4 c00 = ColorBuffer.Load(int3(i00, 0));
  float4 c10 = ColorBuffer.Load(int3(i10, 0));
  float4 c01 = ColorBuffer.Load(int3(i01, 0));
  float4 c11 = ColorBuffer.Load(int3(i11, 0));
  float4 top = lerp(c00, c10, frac.x);
  float4 bot = lerp(c01, c11, frac.x);
  r0 = lerp(top, bot, frac.y);

  o0.w = dot(r0.xyz, float3(0.298999995,0.587000012,0.114));
  o0.xyz = r0.xyz;
  return;
}
