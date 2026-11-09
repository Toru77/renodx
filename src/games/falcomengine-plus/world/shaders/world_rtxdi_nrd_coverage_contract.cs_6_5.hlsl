#include "world_rtxdi_nrd_hitdistance_coverage.hlsli"

RWStructuredBuffer<float2> g_results : register(u0);

cbuffer CoverageContractParams : register(b0) {
  uint g_case_count;
};

[numthreads(4, 1, 1)]
void main(uint3 dispatch_id : SV_DispatchThreadID) {
  if (dispatch_id.x >= g_case_count) return;
  float coverage_end = 0.0;
  bool valid = false;
  if (dispatch_id.x == 0u) {
    // A tiny outward component near the minimum face used to be skipped and
    // could falsely certify the entire trace range as covered.
    valid = NrdHitDistanceCoverageEnd(float3(1.0e-7, 0.5, 0.5), float3(-1.0e-9, 0.0, 0.0), 65504.0,
                                      float3(0.0, 0.0, 0.0), 1.0, coverage_end);
  } else if (dispatch_id.x == 1u) {
    // A small but unambiguous component must limit coverage to its face exit.
    valid = NrdHitDistanceCoverageEnd(float3(1.0e-7, 0.5, 0.5), float3(-2.0e-8, 0.0, 0.0), 65504.0,
                                      float3(0.0, 0.0, 0.0), 1.0, coverage_end);
  } else if (dispatch_id.x == 2u) {
    coverage_end = 65503.5;
    valid = NrdHitDistanceFullyCovered(coverage_end, 65504.0);
  } else {
    coverage_end = 65504.0;
    valid = NrdHitDistanceFullyCovered(coverage_end, 65504.0);
  }
  g_results[dispatch_id.x] = float2(coverage_end, valid ? 1.0 : 0.0);
}
