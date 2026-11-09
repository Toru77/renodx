#include "../rtxdi/rtxdi_di_layout.h"

Texture2D<float4> g_relax_diffuse : register(t0);
Texture2D<float> g_prepared_viewz : register(t1);
Texture2D<uint> g_n1_status : register(t2);
StructuredBuffer<uint> g_n3b_hit_validity : register(t3);
RWTexture2D<float4> g_publication : register(u0);

cbuffer PublicationParams : register(b0) {
  uint2 g_dimensions;
  float g_max_viewz;
};

[numthreads(8, 8, 1)]
void main(uint3 dispatch_id : SV_DispatchThreadID) {
  if (any(dispatch_id.xy >= g_dimensions)) return;
  const uint2 pixel = dispatch_id.xy;
  const uint pixel_index = pixel.y * g_dimensions.x + pixel.x;
  const float viewz = g_prepared_viewz.Load(int3(pixel, 0));
  const uint status = g_n1_status.Load(int3(pixel, 0));
  const uint hit_validity = g_n3b_hit_validity[pixel_index];
  const float3 radiance = g_relax_diffuse.Load(int3(pixel, 0)).rgb;
  const uint sample_status = status & 0x07u;
  const bool known_sample = (status & RTXDI_N1_STATUS_KNOWN_BIT) != 0u
                            && (sample_status == RTXDI_N1_STATUS_VISIBLE
                                || sample_status == RTXDI_N1_STATUS_BLOCKED);
  const bool valid = isfinite(viewz) && viewz > 0.0 && viewz <= g_max_viewz && known_sample
                     && (hit_validity & 1u) != 0u && all(isfinite(radiance))
                     && all(radiance >= 0.0) && all(radiance <= 65504.0);
  const float receiver_class = (status & RTXDI_N1_STATUS_CHARACTER_BIT) != 0u ? 2.0 : 1.0;
  g_publication[pixel] = valid ? float4(radiance, receiver_class) : 0.0;
}
