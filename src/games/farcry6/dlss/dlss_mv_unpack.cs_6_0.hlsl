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
// The game renders its motion vectors at HALF the input resolution (640x360
// for a 1280x720 input), so the packed buffer is indexed at its own size. A
// Load() at input-resolution coordinates runs off the end of the texture, D3D
// returns 0 there, and decode(0) decodes to roughly -0.99 - a ~126px motion
// vector across three quarters of the frame. The output stays at input
// resolution so the motion vectors match the color buffer NGX is handed.
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
  float g_width;      // DLSS input size in pixels: MV scale and output size
  float g_height;
  float g_scale;      // calibration multiplier (1.0 = nominal)
  float g_unused;
  float g_src_width;  // packed MV buffer size (half the input resolution)
  float g_src_height;
  float g_pad0;
  float g_pad1;
};

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
  const int2 dst = int2(dispatch_id.xy);
  if (dst.x >= (int)g_width || dst.y >= (int)g_height) return;

  const int2 out_dim = max(int2(1, 1), int2(int(g_width), int(g_height)));
  const int2 src_dim = max(int2(1, 1), int2(int(g_src_width), int(g_src_height)));

  // The game's own TAA samples this packed buffer with a linear filter. A nearest
  // mapping quantises motion to 2x2 blocks at input resolution, and DLSS reprojects
  // from it - at a 2x upscale that is whole output pixels of error, which reads as
  // softness with edges in the wrong place. Four loads and a lerp over the packed
  // values reproduce what the game's TAA sees; decoding after the blend matches
  // filtering the encoded buffer, which is what the game does.
  const float2 src_pos = (float2(dst) + 0.5f) * float2(src_dim) / float2(out_dim) - 0.5f;
  const int2 base = int2(floor(src_pos));
  const float2 frac = src_pos - float2(base);
  float2 packed = float2(0.0f, 0.0f);
  [unroll] for (int j = 0; j < 2; ++j) {
    [unroll] for (int i = 0; i < 2; ++i) {
      const int2 s = clamp(base + int2(i, j), int2(0, 0), src_dim - 1);
      const float w = ((i == 0) ? (1.0f - frac.x) : frac.x)
                    * ((j == 0) ? (1.0f - frac.y) : frac.y);
      packed += t_packed_mv.Load(int3(s, 0)) * w;
    }
  }
  float2 decoded = float2(DecodeChannel(packed.x), DecodeChannel(packed.y));
  u_out_mv[dst] = decoded * 0.1f * float2(g_width, g_height) * g_scale;
}
