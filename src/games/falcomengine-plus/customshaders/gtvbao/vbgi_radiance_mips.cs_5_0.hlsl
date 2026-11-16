// falcomengine-plus VBGI - Radiance mip chain (M7.4 step 3).
// One level down: each texel is the 2x2 box average of the level above (clamped at the edge).

Texture2D<float4>   g_src : register(t0);
RWTexture2D<float4> g_dst : register(u0);

[numthreads(8, 8, 1)]
void main(uint2 p : SV_DispatchThreadID)
{
  uint dw, dh;
  g_dst.GetDimensions(dw, dh);
  if (p.x >= dw || p.y >= dh) return;

  uint sw, sh;
  g_src.GetDimensions(sw, sh);
  const uint2 base = p * 2u;
  float4 sum = float4(0.0f, 0.0f, 0.0f, 0.0f);
  for (uint y = 0u; y < 2u; ++y) {
    for (uint x = 0u; x < 2u; ++x) {
      sum += g_src.Load(int3(min(base + uint2(x, y), uint2(sw, sh) - 1u), 0));
    }
  }
  g_dst[p] = sum * 0.25f;
}
