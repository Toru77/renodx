///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// falcomengine-plus GTVBAO — MRT Normal Pre-Decode (à-trous perf optimization)
//
// Decodes the packed MRT g-buffer normals ONCE per frame into an fp16 texture,
// so the à-trous wavelet filter can fetch ready-to-use normals with plain
// samples instead of re-running the sincos/sqrt decode on every kernel tap.
//
// Bindings (normal_prep_layout): t0 = MRT normal (uint4) → u0 = RGBA16F normal
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "gtvbao_common.hlsl"

Texture2D<uint4>   g_srcPrepMrtNormal : register(t0);
RWTexture2D<float4> g_outPrepNormal    : register(u0);

[numthreads(GT_VBAO_NUMTHREADS_X, GT_VBAO_NUMTHREADS_Y, 1)]
void main(uint2 dt : SV_DispatchThreadID)
{
  uint width, height;
  g_outPrepNormal.GetDimensions(width, height);
  if (dt.x >= width || dt.y >= height) return;

  uint4 s = g_srcPrepMrtNormal.Load(int3(dt, 0));
  float2 enc = float2((float)s.x, (float)s.y) * (1.0 / 32767.5) + float2(-1.0, -1.0);
  float azimuth = 3.14159274 * enc.x;
  float sin_a, cos_a;
  sincos(azimuth, sin_a, cos_a);
  float ring = sqrt(saturate(1.0 - enc.y * enc.y));
  float3 n = float3(cos_a * ring, sin_a * ring, enc.y);
  float len = length(n);
  if (len > 1e-4) n /= len; else n = float3(0, 0, 1);

  g_outPrepNormal[dt] = float4(n, 0.0);
}
