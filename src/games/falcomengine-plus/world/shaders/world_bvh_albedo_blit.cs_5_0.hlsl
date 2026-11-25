// world_bvh_albedo_blit.cs_5_0.hlsl - resamples the colour of one static mesh texture into one
// slice of the albedo atlas. A slice holds 128 x 128 RGBA8 texels. Thread (x, y) samples
// the source at the destination texel centre and writes it to the slice.
cbuffer cb_albedo_blit : register(b0)
{
  uint slice_g : packoffset(c0.x);
};

Texture2D<float4> source_g : register(t0);
SamplerState linear_s : register(s0);
RWTexture2DArray<float4> atlas_g : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
  if (id.x >= 128u || id.y >= 128u) return;
  const float2 uv = (id.xy + 0.5f) / 128.f;
  atlas_g[uint3(id.xy, slice_g)] = source_g.SampleLevel(linear_s, uv, 0);
}
