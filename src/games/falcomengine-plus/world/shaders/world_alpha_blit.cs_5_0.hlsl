// world_alpha_blit.cs_5_0.hlsl - resamples the alpha of one alpha-tested texture into one
// slice of the alpha atlas. A slice holds 1024 x 256 alpha texels, four per uint (texel x
// in byte x & 3), so the slice is a 256 x 256 uint array. Thread (x, y) packs texels
// 4x .. 4x+3 of row y from the source, sampled at the texel centers.
cbuffer cb_alpha_blit : register(b0)
{
  uint slice_g : packoffset(c0.x);
};

Texture2D<float4> source_g : register(t0);
SamplerState linear_s : register(s0);
RWTexture2DArray<uint> atlas_g : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
  if (id.x >= 256u || id.y >= 256u) return;
  uint packed = 0u;
  [unroll]
  for (uint k = 0u; k < 4u; ++k)
  {
    const float2 uv = (float2(id.x * 4u + k, id.y) + 0.5f) / float2(1024.f, 256.f);
    const float alpha = source_g.SampleLevel(linear_s, uv, 0).a;
    packed |= (uint)(saturate(alpha) * 255.f + 0.5f) << (k * 8u);
  }
  atlas_g[uint3(id.x, id.y, slice_g)] = packed;
}
