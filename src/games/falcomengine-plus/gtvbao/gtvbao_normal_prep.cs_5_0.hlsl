///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// falcomengine-plus GTVBAO — MRT Normal Pre-Decode (à-trous perf optimization)
//
// Decodes the packed MRT g-buffer normals ONCE per frame into an fp16 texture,
// so the à-trous wavelet filter can fetch ready-to-use normals with plain
// samples instead of re-running the sincos/sqrt decode on every kernel tap.
//
// Unwritten MRT texels (.xy == 0) with geometry get the depth-derived normal;
// sky (depth <= 0 or >= 65000) stays a zero vector.
//
// Bindings (normal_prep_layout): t0 = MRT normal (uint4), t1 = linear view depth mip 0 (float) → u0 = RGBA16F normal
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "gtvbao_common.hlsl"

Texture2D<uint4>   g_srcPrepMrtNormal : register(t0);
Texture2D<lpfloat> g_depth            : register(t1);
RWTexture2D<float4> g_outPrepNormal    : register(u0);

[numthreads(GT_VBAO_NUMTHREADS_X, GT_VBAO_NUMTHREADS_Y, 1)]
void main(uint2 dt : SV_DispatchThreadID)
{
  uint width, height;
  g_outPrepNormal.GetDimensions(width, height);
  if (dt.x >= width || dt.y >= height) return;

  // Same shared decode + transform as the main pass, so the two can never
  // disagree. Texels with no MRT normal get the depth-derived normal when they
  // have geometry; sky stays a ZERO vector.
  const uint2 mrt = g_srcPrepMrtNormal.Load(int3(dt, 0)).xy;
  float3 n = float3(0.0, 0.0, 0.0);
  if (any(mrt != 0u)) {
    n = TransformNormalToView(DecodeFalcomMrtNormal(mrt));
  } else {
    const float z = g_depth.Load(int3(dt, 0));
    const bool has_geometry = (z > 0.0) && (z < 65000.0);
    if (has_geometry) {
      const GTAOConstants consts = BuildGTAOConstants(uint2(width, height));
      n = GTVBAO_DepthNormalAt(g_depth, int2(dt), uint2(width, height), consts);
    }
  }
  g_outPrepNormal[dt] = float4(n, 0.0);
}
