// ── HDR color conversion for DLSS input/output (HDR-mod compatibility) ──
// Modes (params0.x):
//   0 = sRGB DECODE (input pass): t0 sRGB r8g8b8a8   -> u0 linear r16g16b16a16
//   1 = sRGB ENCODE (output pass): t0 linear r16g16b16a16 -> u0 sRGB r8g8b8a8
//   2 = PQ     DECODE (input pass): t0 PQ r8g8b8a8    -> u0 linear r16g16b16a16
//   3 = PQ     ENCODE (output pass): t0 linear r16g16b16a16 -> u0 PQ r8g8b8a8
// DLSS always expects LINEAR input regardless of the IsHDR flag, so when the
// senkiseki HDR mod feeds an sRGB-encoded composite the DLSS output is dark/
// black. These passes linearize the DLSS input and re-encode the output to the
// encoding the HDR chain (0x9DB02646 SignPow + swapchain proxy) expects.
// SPDX-License-Identifier: MIT

Texture2D<float4> g_src : register(t0);
RWTexture2D<float4> g_dst : register(u0);

cbuffer cb_push : register(b13) {
  float4 params0;  // x = mode
};

// IEC 61966-2-1 sRGB transfer functions
float SrgbToLinear(float c) {
  return (c <= 0.04045f) ? c / 12.92f : pow((c + 0.055f) / 1.055f, 2.4f);
}
float LinearToSrgb(float c) {
  return (c <= 0.0031308f) ? c * 12.92f : 1.055f * pow(c, 1.0f / 2.4f) - 0.055f;
}

// SMPTE ST 2084 (PQ) transfer functions (normalized: 1.0 = 10000 nits)
float PqToLinear(float c) {
  const float m1 = 2610.0f / 16384.0f;
  const float m2 = 2523.0f / 4096.0f * 128.0f;
  const float c1 = 3424.0f / 4096.0f;
  const float c2 = 2413.0f / 4096.0f * 32.0f;
  const float c3 = 2392.0f / 4096.0f * 32.0f;
  float cp = pow(c, 1.0f / m2);
  float num = max(cp - c1, 0.0f);
  float den = c2 - c3 * cp;
  return pow(num / den, 1.0f / m1);
}
float LinearToPq(float c) {
  const float m1 = 2610.0f / 16384.0f;
  const float m2 = 2523.0f / 4096.0f * 128.0f;
  const float c1 = 3424.0f / 4096.0f;
  const float c2 = 2413.0f / 4096.0f * 32.0f;
  const float c3 = 2392.0f / 4096.0f * 32.0f;
  float cp = pow(c, m1);
  float num = c1 + c2 * cp;
  float den = 1.0f + c3 * cp;
  return pow(num / den, m2);
}

[numthreads(8, 8, 1)]
void main(uint2 pix : SV_DispatchThreadID)
{
  int mode = (int)params0.x;
  float4 src = g_src[pix];
  float4 dst = src;
  if (mode == 0) {          // sRGB -> linear (DLSS input)
    dst.rgb = float3(SrgbToLinear(src.r), SrgbToLinear(src.g), SrgbToLinear(src.b));
    dst.a = 1.0f;
  } else if (mode == 1) {   // linear -> sRGB (DLSS output for the HDR chain)
    dst.rgb = float3(LinearToSrgb(src.r), LinearToSrgb(src.g), LinearToSrgb(src.b));
    dst.a = 1.0f;
  } else if (mode == 2) {   // PQ -> linear (DLSS input)
    dst.rgb = float3(PqToLinear(src.r), PqToLinear(src.g), PqToLinear(src.b));
    dst.a = 1.0f;
  } else if (mode == 3) {   // linear -> PQ (DLSS output for the HDR chain)
    dst.rgb = float3(LinearToPq(src.r), LinearToPq(src.g), LinearToPq(src.b));
    dst.a = 1.0f;
  }
  g_dst[pix] = dst;
}
