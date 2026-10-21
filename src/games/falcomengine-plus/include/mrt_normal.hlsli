#ifndef SRC_GAMES_FALCOMENGINE_PLUS_INCLUDE_MRT_NORMAL_HLSI_
#define SRC_GAMES_FALCOMENGINE_PLUS_INCLUDE_MRT_NORMAL_HLSI_

// ── Falcom G-buffer (mrtTexture0) world normal decode ──
//
// Falcom packs the world-space normal as a cylindrical (azimuth, cos-polar)
// pair: .x = atan2(N.y, N.x)/PI + 1, .y = N.z + 1, each scaled by 32767.5.
// Matches the vanilla lighting decode (see sora2nd/lighting/lighting.asm:
// `mad r8.zw` -> `sincos` -> `dp3 r3.y, r8.xywx`).
//
// GTVBAO (gtvbao_common.hlsl) and the shadow passes (shadows/shadows_common.hlsli)
// use DecodeFalcomMrtNormal. The RT passes use DecodeFalcomMrtNormalAsGame, the
// game's own decode with no zero case. Both decode the same encoding, so both live
// here: a second copy elsewhere would be free to drift.

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

// Returns a ZERO vector for a texel the G-buffer never wrote (GTVBAO and shadows
// only; DecodeFalcomMrtNormalAsGame has no zero case). A cleared texel is all-zero,
// which decodes to enc = (-1,-1) and the world-straight-down normal (0,0,-1): unit
// length, so it would survive a naive validity test. A written texel has
// raw.y ~= 32767 when N.z == 0, so an exact (0,0) pair can only mean "no g-buffer
// data here". Callers fall back to the depth-derived normal on a zero result.
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

#endif  // SRC_GAMES_FALCOMENGINE_PLUS_INCLUDE_MRT_NORMAL_HLSI_
