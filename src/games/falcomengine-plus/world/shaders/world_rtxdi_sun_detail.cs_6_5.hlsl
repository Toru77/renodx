struct RtxdiSunResult {
  uint status;
  float distance;
  uint reason;
  uint flags;
};

cbuffer SunDetailParams : register(b0) {
  uint2 g_dimensions;
  uint g_pixel_count;
};

StructuredBuffer<RtxdiSunResult> g_sun_results : register(t0);
RWTexture2D<uint4> g_sun_detail : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 dispatch_thread_id : SV_DispatchThreadID) {
  const uint2 pixel = dispatch_thread_id.xy;
  if (any(pixel >= g_dimensions)) return;
  const uint index = pixel.y * g_dimensions.x + pixel.x;
  if (index >= g_pixel_count) {
    g_sun_detail[pixel] = uint4(0u, 0u, 0u, 0u);
    return;
  }

  const RtxdiSunResult source = g_sun_results[index];
  g_sun_detail[pixel] = uint4(source.status, source.reason, asuint(source.distance), source.flags);
}
