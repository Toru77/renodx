// Far Cry 6 DLSS: packed motion-vector unpack (compute, cs_6_0).
//
// The TAA pass (0xFF0AF25B / 0x4AD8172A) reads its per-pixel reprojection
// offset from t0 (Texture2D<float2>) through a piecewise decode:
//
//   m = v - 0.49803921580314636
//   x = m * 2.0
//   outer (|x| >= 0.6687403321266174): o = m * 4.830047130584717 +/- 1.415023684501648
//   inner (|x| <  ...):                o = x^4 * sign(m)
//   offset_uv = o * 0.1                        (reprojectUV = uv + offset_uv)
//
// NGX cannot consume that packing, so this pass decodes it on the GPU and
// writes standard pixel-unit motion vectors (RG16F) for DLSS:
//   mv_px = decode(t0) * 0.1 * resolution * scale.
//
// Sign convention matches NGX: offset from the current pixel to the previous
// frame position (the game adds it to the current UV to fetch history).
//
// NOTE on exact-zero motion: the game evaluates sign() via an INF trick
// (m * +INF, saturate) which is NaN when m == 0. Here sign() yields a clean
// 0 instead, so static pixels produce exactly-zero MVs rather than NaN.

Texture2D<float2> t_packed_mv : register(t0);
RWTexture2D<float2> u_out_mv : register(u0);

cbuffer UnpackParams : register(b0) {
  float g_width;   // input width in pixels
  float g_height;  // input height in pixels
  float g_scale;   // calibration multiplier (1.0 = nominal)
  float g_unused;
}

float DecodeChannel(float v) {
  float m = v - 0.49803921580314636f;
  float x = m * 2.0f;
  float o;
  if (x < -0.6687403321266174f) {
    o = m * 4.830047130584717f + 1.415023684501648f;
  } else if (x < 0.6687403321266174f) {
    float s = (m > 0.0f) ? 1.0f : ((m < 0.0f) ? -1.0f : 0.0f);
    float x2 = x * x;
    o = x2 * x2 * s;
  } else {
    o = m * 4.830047130584717f - 1.415023684501648f;
  }
  return o;
}

[numthreads(8, 8, 1)]
void main(uint3 dispatch_id : SV_DispatchThreadID) {
  if (dispatch_id.x >= (uint)g_width || dispatch_id.y >= (uint)g_height) return;
  float2 packed = t_packed_mv.Load(int3(int2(dispatch_id.xy), 0));
  float2 decoded = float2(DecodeChannel(packed.x), DecodeChannel(packed.y));
  u_out_mv[dispatch_id.xy] = decoded * 0.1f * float2(g_width, g_height) * g_scale;
}
