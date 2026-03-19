SamplerState samPoint_s : register(s0);
Texture2D<uint4> mrtTexture0 : register(t0);

uint2 ComputePixelCoord(float2 uv, uint width, uint height) {
  width = max(width, 1u);
  height = max(height, 1u);
  float2 pixel_f = saturate(uv) * float2((float)width, (float)height);
  return min((uint2)pixel_f, uint2(width - 1u, height - 1u));
}

float4 main(float4 position : SV_Position, float2 uv : TEXCOORD0) : SV_Target {
  uint mrt_width, mrt_height;
  mrtTexture0.GetDimensions(mrt_width, mrt_height);
  uint2 mrt_pixel = ComputePixelCoord(uv, mrt_width, mrt_height);
  uint4 mrt = mrtTexture0.Load(int3(mrt_pixel, 0));

  const uint marker_mask = 0x7FFFFFFFu;
  const uint variation_bit = 0x0200u;
  const uint canonical_a = 0x08FFu;
  const uint canonical_b = 0x0CFFu;

  uint class_id = (mrt.z & marker_mask) & ~variation_bit;
  bool is_foliage = class_id == canonical_a || class_id == canonical_b;

  float mask = is_foliage ? 1.0 : 0.0;
  return float4(mask, mask, mask, 1.0);
}

