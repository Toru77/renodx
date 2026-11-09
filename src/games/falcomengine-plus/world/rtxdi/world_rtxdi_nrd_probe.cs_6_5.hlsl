cbuffer NrdProbeConstants : register(b0) {
  uint2 resource_size;
  float2 current_jitter_pixels;
  float2 previous_jitter_pixels;
  float2 motion_correction0_pixels;
  float2 motion_correction1_pixels;
  float2 padding;
  row_major float4x4 current_inverse_view_projection;
  row_major float4x4 previous_inverse_view_projection;
  row_major float4x4 current_view_projection_non_jittered;
  row_major float4x4 previous_view_projection_non_jittered;
  row_major float4x4 previous_view;
};

Texture2D<float> current_depth : register(t0);
Texture2D<float> previous_depth : register(t1);
Texture2D<float4> current_motion : register(t2);
Texture2D<uint4> current_normal : register(t3);
Texture2D<uint4> previous_normal : register(t4);
RWStructuredBuffer<uint> residual_bits : register(u0);

static const uint2 probe_grid = uint2(32u, 18u);
static const uint kInvalidResidual = 0x7fc00000u;

float2 ToUv(float4 clip) {
  return clip.xy / clip.w * float2(0.5, -0.5) + 0.5;
}

void RejectSample(uint index, uint reason) {
  residual_bits[index * 2u + 0u] = kInvalidResidual | reason;
  residual_bits[index * 2u + 1u] = kInvalidResidual | reason;
}

[numthreads(8, 8, 1)]
void main(uint3 dispatch_id : SV_DispatchThreadID) {
  if (any(dispatch_id.xy >= probe_grid)) return;
  const uint sample_index = dispatch_id.y * probe_grid.x + dispatch_id.x;
  const uint2 pixel = min(uint2((float2(dispatch_id.xy) + 0.5) * float2(resource_size) / float2(probe_grid)), resource_size - 1u);
  const uint4 current_mrt = current_normal.Load(int3(pixel, 0));
  if ((current_mrt.w & 8u) == 0u) {
    RejectSample(sample_index, 1u);
    return;
  }

  const float raw_depth = current_depth.Load(int3(pixel, 0));
  if (!isfinite(raw_depth) || raw_depth <= 0.0 || raw_depth > 1.0) {
    RejectSample(sample_index, 2u);
    return;
  }
  const float2 pixel_uv = (float2(pixel) + 0.5) / float2(resource_size);
  const float4 current_clip = float4(pixel_uv.x * 2.0 - 1.0, 1.0 - pixel_uv.y * 2.0, raw_depth, 1.0);
  const float4 current_world_h = mul(current_clip, current_inverse_view_projection);
  if (!isfinite(current_world_h.w) || abs(current_world_h.w) <= 1e-8 || !all(isfinite(current_world_h.xyz))) {
    RejectSample(sample_index, 3u);
    return;
  }
  const float3 current_world = current_world_h.xyz / current_world_h.w;
  if (!all(isfinite(current_world))) {
    RejectSample(sample_index, 4u);
    return;
  }

  const float4 current_non_jittered_clip = mul(float4(current_world, 1.0), current_view_projection_non_jittered);
  const float4 previous_non_jittered_clip = mul(float4(current_world, 1.0), previous_view_projection_non_jittered);
  if (!isfinite(current_non_jittered_clip.w) || !isfinite(previous_non_jittered_clip.w)
      || abs(current_non_jittered_clip.w) <= 1e-8 || abs(previous_non_jittered_clip.w) <= 1e-8) {
    RejectSample(sample_index, 5u);
    return;
  }
  const float2 current_non_jittered_uv = ToUv(current_non_jittered_clip);
  const float2 previous_non_jittered_uv = ToUv(previous_non_jittered_clip);
  const float2 expected_current_uv = current_non_jittered_uv + current_jitter_pixels / float2(resource_size);
  const float current_reprojection_error = length((expected_current_uv - pixel_uv) * float2(resource_size));
  if (!all(isfinite(expected_current_uv)) || current_reprojection_error > 0.75) {
    residual_bits[sample_index * 2u] = isfinite(current_reprojection_error) ? asuint(current_reprojection_error)
                                                                            : kInvalidResidual | 6u;
    residual_bits[sample_index * 2u + 1u] = kInvalidResidual | 6u;
    return;
  }
  const float2 previous_uv = previous_non_jittered_uv + previous_jitter_pixels / float2(resource_size);
  if (!all(isfinite(previous_uv)) || any(previous_uv <= 0.0) || any(previous_uv >= 1.0)) {
    RejectSample(sample_index, 7u);
    return;
  }
  const uint2 previous_pixel = min(uint2(previous_uv * float2(resource_size)), resource_size - 1u);
  const uint4 previous_mrt = previous_normal.Load(int3(previous_pixel, 0));
  if ((previous_mrt.w & 8u) == 0u || current_mrt.w != previous_mrt.w
      || any(current_mrt.xy != previous_mrt.xy)) {
    RejectSample(sample_index, 8u);
    return;
  }

  const float previous_raw_depth = previous_depth.Load(int3(previous_pixel, 0));
  if (!isfinite(previous_raw_depth) || previous_raw_depth <= 0.0 || previous_raw_depth > 1.0) {
    RejectSample(sample_index, 9u);
    return;
  }
  const float2 previous_pixel_uv = (float2(previous_pixel) + 0.5) / float2(resource_size);
  const float4 previous_clip = float4(previous_pixel_uv.x * 2.0 - 1.0, 1.0 - previous_pixel_uv.y * 2.0,
                                      previous_raw_depth, 1.0);
  const float4 previous_world_h = mul(previous_clip, previous_inverse_view_projection);
  if (!isfinite(previous_world_h.w) || abs(previous_world_h.w) <= 1e-8 || !all(isfinite(previous_world_h.xyz))) {
    RejectSample(sample_index, 10u);
    return;
  }
  const float3 previous_world = previous_world_h.xyz / previous_world_h.w;
  const float previous_view_z = -mul(float4(previous_world, 1.0), previous_view).z;
  const float expected_previous_view_z = -mul(float4(current_world, 1.0), previous_view).z;
  if (!all(isfinite(previous_world)) || !isfinite(previous_view_z) || !isfinite(expected_previous_view_z)
      || previous_view_z <= 0.0 || expected_previous_view_z <= 0.0
      || abs(previous_view_z - expected_previous_view_z) > max(0.02, 0.003 * expected_previous_view_z)) {
    RejectSample(sample_index, 11u);
    return;
  }

  const float2 expected_motion = (previous_non_jittered_uv - current_non_jittered_uv) * float2(resource_size);
  const float2 game_motion = current_motion.Load(int3(pixel, 0)).xy;
  if (!all(isfinite(expected_motion)) || !all(isfinite(game_motion))) {
    RejectSample(sample_index, 12u);
    return;
  }
  [unroll]
  for (uint candidate = 0u; candidate < 2u; ++candidate) {
    const float2 correction = candidate == 0u ? motion_correction0_pixels : motion_correction1_pixels;
    const float error_pixels = length(game_motion - correction - expected_motion);
    residual_bits[sample_index * 2u + candidate] = isfinite(error_pixels) ? asuint(error_pixels) : kInvalidResidual;
  }
}
