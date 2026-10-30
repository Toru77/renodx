/*
 * RCAS and SRTM, adapted from AMD FidelityFX.
 * Copyright (c) 2021 Advanced Micro Devices, Inc. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
Texture2D<float4> source : register(t0);
RWTexture2D<float4> target : register(u0);
cbuffer Parameters : register(b0) {
  uint2 extent;
  float strength;
};

float3 ToRcasRange(float3 color) {
  return color / (1.0f + max(color.r, max(color.g, color.b)));
}

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID) {
  if (any(id.xy >= extent)) return;
  int2 p = int2(id.xy);
  int2 last = int2(extent) - 1;
  float4 center = source.Load(int3(p, 0));
  float3 b = source.Load(int3(clamp(p + int2(0, -1), int2(0, 0), last), 0)).rgb;
  float3 d = source.Load(int3(clamp(p + int2(-1, 0), int2(0, 0), last), 0)).rgb;
  float3 f = source.Load(int3(clamp(p + int2(1, 0), int2(0, 0), last), 0)).rgb;
  float3 h = source.Load(int3(clamp(p + int2(0, 1), int2(0, 0), last), 0)).rgb;
  // SRTM is defined for non-negative HDR. Preserve pixels in signed regions.
  if (any(min(min(b, d), min(min(f, h), center.rgb)) < 0.0f)
      || any(max(max(b, d), max(max(f, h), center.rgb)) >= 32767.0f)) {
    target[p] = center;
    return;
  }
  b = ToRcasRange(b);
  d = ToRcasRange(d);
  float3 e = ToRcasRange(center.rgb);
  f = ToRcasRange(f);
  h = ToRcasRange(h);
  float bL = b.b * 0.5f + b.r * 0.5f + b.g;
  float dL = d.b * 0.5f + d.r * 0.5f + d.g;
  float eL = e.b * 0.5f + e.r * 0.5f + e.g;
  float fL = f.b * 0.5f + f.r * 0.5f + f.g;
  float hL = h.b * 0.5f + h.r * 0.5f + h.g;
  float noise = 0.25f * (bL + dL + fL + hL) - eL;
  float contrast = max(max(max(bL, dL), eL), max(fL, hL)) - min(min(min(bL, dL), eL), min(fL, hL));
  float noise_limit = 1.0f - 0.5f * saturate(abs(noise) / max(contrast, 1e-8f));
  float3 min4 = min(min(b, d), min(f, h));
  float3 max4 = max(max(b, d), max(f, h));
  float3 hit_min = min(min4, e) / max(4.0f * max4, 1e-8f);
  float3 hit_max = (1.0f - max(max4, e)) / min(4.0f * min4 - 4.0f, -1e-8f);
  float3 lobe_rgb = max(-hit_min, hit_max);
  float lobe = max(-0.1875f, min(max(lobe_rgb.r, max(lobe_rgb.g, lobe_rgb.b)), 0.0f));
  lobe *= strength * noise_limit;
  float3 result = (lobe * (b + d + f + h) + e) / (4.0f * lobe + 1.0f);
  // Avoid the inverse's singularity without compressing bright HDR values.
  float inverse_denominator = 1.0f - max(result.r, max(result.g, result.b));
  if (inverse_denominator <= 1.0f / 32768.0f) {
    target[p] = center;
    return;
  }
  result /= inverse_denominator;
  target[p] = float4(result, center.a);
}
