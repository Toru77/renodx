// Experimental masked unpack. The game mixes camera and object motion in the
// packed texture. Only object motion is expected to contain the Halton phase.
// The correction is provisional until the mask and sign are verified in game.
Texture2D<float2> t_packed_mv : register(t0);
Texture2D<float> t_object_mask : register(t1);
RWTexture2D<float2> u_out_mv : register(u0);

cbuffer UnpackParams : register(b0) {
  float2 g_output_size;
  float g_motion_scale;
  float g_correction_scale;
  float2 g_source_size;
  float2 g_jitter_delta;
};

float DecodeChannel(float value) {
  const float m = value - 0.49803921580314636f;
  const float x = m * 2.0f;
  if (x < -0.6687403321266174f) return m * 4.830047130584717f + 1.415023684501648f;
  if (x >= 0.6687403321266174f) return m * 4.830047130584717f - 1.415023684501648f;
  const float x2 = x * x;
  return x2 * x2 * sign(m);
}

[numthreads(8, 8, 1)]
void main(uint3 dispatch_id : SV_DispatchThreadID) {
  const int2 dst = int2(dispatch_id.xy);
  const int2 output_size = int2(g_output_size);
  if (any(dst >= output_size)) return;
  const int2 source_size = max(int2(1, 1), int2(g_source_size));
  const float2 source_position = (float2(dst) + 0.5f) * float2(source_size) / float2(output_size) - 0.5f;
  const int2 base = int2(floor(source_position));
  const float2 fraction = source_position - float2(base);
  float2 packed = 0.0f;
  float object_weight = 0.0f;
  [unroll] for (int y = 0; y < 2; ++y) {
    [unroll] for (int x = 0; x < 2; ++x) {
      const int2 position = clamp(base + int2(x, y), int2(0, 0), source_size - 1);
      const float weight = (x == 0 ? 1.0f - fraction.x : fraction.x)
                         * (y == 0 ? 1.0f - fraction.y : fraction.y);
      packed += t_packed_mv.Load(int3(position, 0)) * weight;
      object_weight += t_object_mask.Load(int3(position, 0)) * weight;
    }
  }
  const float2 decoded = float2(DecodeChannel(packed.x), DecodeChannel(packed.y));
  const float2 motion = decoded * 0.1f * g_output_size * g_motion_scale;
  u_out_mv[dst] = motion + g_jitter_delta * object_weight * g_correction_scale;
}
