#ifndef SRC_GAMES_FALCOMENGINE_PLUS_INCLUDE_MRT_NORMAL_HLSI_
#define SRC_GAMES_FALCOMENGINE_PLUS_INCLUDE_MRT_NORMAL_HLSI_

// ── Falcom G-buffer (mrtTexture0) world normal decode ──
//
// Falcom packs the world-space normal as a cylindrical (azimuth, cos-polar)
// pair: .x = atan2(N.y, N.x)/PI + 1, .y = N.z + 1, each scaled by 32767.5.
// Matches the vanilla lighting decode (see games/sora2nd/lighting/lighting.asm:
// `mad r8.zw` -> `sincos` -> `dp3 r3.y, r8.xywx`).
//
// DecodeFalcomMrtNormalAsGame matches the game's decode with no zero case. A packed
// (0,0) can be a valid -Z normal or cleared data; callers need an independent surface
// test to distinguish those cases. The legacy decoder keeps the cleared-data fallback.

// Normalize with an explicit fallback instead of a NaN. A zero-length input
// (a cleared G-buffer texel) is a normal condition here, not an exceptional
// one, and rsqrt(0) would poison every downstream dot product.
float3 FalcomSafeNormalize3(float3 v, float3 fallback)
{
  float len2 = dot(v, v);
  return (len2 < 1e-5) ? fallback : v * rsqrt(len2);
}

// True when a normal is usable geometry rather than a placeholder.
bool FalcomNormalValid(float3 n) { return dot(n, n) > 1e-5; }

// Legacy fallback: treats a packed (0,0) as missing data. Cleared texels and a valid
// world-straight-down normal share this pair, so use only when a surface is not proven.
float3 DecodeFalcomMrtNormal(uint2 packed)
{
  if (packed.x == 0u && packed.y == 0u) return float3(0.0, 0.0, 0.0);

  float2 enc = float2((float)packed.x, (float)packed.y) * (1.0 / 32767.5) + float2(-1.0, -1.0);
  float sin_a, cos_a;
  sincos(3.14159274 * enc.x, sin_a, cos_a);
  float ring = sqrt(saturate(1.0 - enc.y * enc.y));
  return FalcomSafeNormalize3(float3(cos_a * ring, sin_a * ring, enc.y), float3(0.0, 0.0, 0.0));
}

// Game lighting decode (lighting_0xCA3D8596.ps_5_0.hlsl:774-782). The world is y-up and (0,0) means -Z.
float3 DecodeFalcomMrtNormalAsGame(uint2 packed)
{
  float2 enc = float2((float)packed.x, (float)packed.y) * 3.05180438e-05 + float2(-1.0, -1.0);
  float sin_a, cos_a;
  sincos(3.14159274 * enc.x, sin_a, cos_a);
  float ring = sqrt(1.0 - enc.y * enc.y);
  return float3(cos_a * ring, sin_a * ring, enc.y);
}

// Texel address for a screen UV in a source whose resolution differs from the
// AO working domain. Every MRT normal reader must land on
// the SAME texel for the same UV, or the two read different normals for one
// pixel.
//
// The caller passes the already-scaled texel-space coordinate (i.e.
// `uv * dims`), NOT a normalized UV, and that scaling is done ONCE by the
// caller. Computing `(pix + 0.5) / work * dims` here instead would round twice
// and can land just under an integer boundary, e.g. half-res 960 -> full 1920
// at pix 252 gives 504.99999999999994 and floors to 504 where the old
// `floor((252 + 0.5) * 2)` correctly gave 505. Keep the arithmetic in the
// form the main pass used to keep the existing mapping bit-identical.
int2 GTVBAO_MrtTexel(float2 uvTexelSpace, float2 mrtDims)
{
  int2 d = max(int2(mrtDims), int2(1, 1));
  return clamp(int2(floor(uvTexelSpace)), int2(0, 0), d - 1);
}

#endif  // SRC_GAMES_FALCOMENGINE_PLUS_INCLUDE_MRT_NORMAL_HLSI_
