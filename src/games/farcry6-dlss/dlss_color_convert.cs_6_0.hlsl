#include "../../shaders/renodx.hlsl"

Texture2D<float4> source : register(t0);
RWTexture2D<float4> target : register(u0);
cbuffer Parameters : register(b0) {
  uint2 extent;
  uint to_linear;
};

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID) {
  if (any(id.xy >= extent)) return;
  float4 color = source.Load(int3(id.xy, 0));
  if (to_linear != 0u) {
    color.rgb = renodx::color::bt2020::from::BT709(renodx::color::srgb::DecodeSafe(color.rgb));
  } else {
    color.rgb = renodx::color::srgb::EncodeSafe(renodx::color::bt709::from::BT2020(color.rgb));
  }
  target[id.xy] = color;
}
