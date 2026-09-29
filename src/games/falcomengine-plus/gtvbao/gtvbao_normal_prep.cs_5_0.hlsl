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

  // Same shared decode + transform as the main pass, so the two can never
  // disagree. Unwritten texels stay a ZERO vector, which the à-trous and
  // upscale consumers read as "no g-buffer normal here" and fall back to
  // depth-only weighting for.
  // This texture feeds the AO-side filters (à-trous, upscale), so it uses the
  // AO transform mode, not the independent VBGI one.
  g_outPrepNormal[dt] = float4(
      TransformNormalToView(
          DecodeFalcomMrtNormal(g_srcPrepMrtNormal.Load(int3(dt, 0)).xy),
          GTVBAO_normal_transform_mode),
      0.0);
}
