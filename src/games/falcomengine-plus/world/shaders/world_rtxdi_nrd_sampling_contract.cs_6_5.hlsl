#include "world_rtxdi_nrd_hitdistance_sampling.hlsli"

RWStructuredBuffer<float4> g_samples : register(u0);

cbuffer SamplingParams : register(b0) {
  uint g_sample_count;
  uint g_frame;
};

[numthreads(64, 1, 1)]
void main(uint3 dispatch_id : SV_DispatchThreadID) {
  if (dispatch_id.x >= g_sample_count) return;
  uint state = NrdHitDistanceHash(dispatch_id.x ^ NrdHitDistanceHash(g_frame + 0xA511E9B3u));
  const float3 direction = NrdCosineHemisphere(float3(0.0, 0.0, -1.0), state);
  g_samples[dispatch_id.x] = float4(direction, 1.0);
}
