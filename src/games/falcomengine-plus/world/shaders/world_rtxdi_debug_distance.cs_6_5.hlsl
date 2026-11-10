#include "world_rtxdi_debug.hlsli"

cbuffer DebugDistanceConstants : register(b0)
{
  uint g_width;
  uint g_height;
  uint g_pixel_count;
};

Texture2D<uint> g_n1_status : register(t0);
StructuredBuffer<float> g_hit_distance : register(t1);
StructuredBuffer<uint> g_hit_flags : register(t2);
RWTexture2D<float4> g_debug_distance : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 dispatch_id : SV_DispatchThreadID)
{
  const uint2 pixel = dispatch_id.xy;
  if (pixel.x >= g_width || pixel.y >= g_height) return;

  const uint linear_index = pixel.y * g_width + pixel.x;
  float4 carrier = float4(0.0, 0.0, 0.0, 1.0);
  if (linear_index < g_pixel_count) {
    const uint status_word = g_n1_status.Load(int3(pixel, 0));
    const uint flags = g_hit_flags[linear_index];
    const float distance = g_hit_distance[linear_index];
    carrier = RtxdiDebugPackDistance(status_word, distance, flags);
  }
  g_debug_distance[pixel] = carrier;
}
