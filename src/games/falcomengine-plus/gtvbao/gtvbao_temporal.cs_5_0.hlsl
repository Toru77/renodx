// gtvbao_temporal.cs_5_0.hlsl - GTVBAO temporal accumulation (RTAO pass B, RTGI temporal structure).
//
// One thread per AO texel (working domain: Full or Half). Reprojects with CustomTAA_Reproject at the representative
// full-res pixel min(tp * d + d / 2, full - 1), blends 2x2 bilinear history taps (depth and soft normal accept),
// clamps to the 3x3 raw neighbourhood and blends by History Weight times the valid fraction. Reads the raw AO (t0),
// the game depth (t1), the MRT normal (t2), the motion (t3, texel units) and the history (t4: R accumulated AO,
// G distance, BA octahedral normal). Writes the history (u0), the output AO byte (u1) and debug views 30/31 (u2).

#include "gtvbao_common.hlsl"
#include "../taa/taa_reproject.hlsli"
#include "../world/shaders/world_rt_denoise.hlsli"

Texture2D<uint> g_raw_ao : register(t0);
Texture2D<float4> g_game_depth : register(t1);
Texture2D<uint4> g_mrt_normal : register(t2);
Texture2D<float4> g_motion : register(t3);
Texture2D<float4> g_history_read : register(t4);

RWTexture2D<float4> g_history_write : register(u0);
RWTexture2D<uint> g_ao_out : register(u1);
RWTexture2D<float4> g_debug : register(u2);

SamplerState g_samp_point : register(s0);
SamplerState g_samp_linear : register(s1);

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
  uint aw, ah;
  g_ao_out.GetDimensions(aw, ah);
  if (id.x >= aw || id.y >= ah) return;

  const uint2 tp = id.xy;
  const int2 size = int2(aw, ah);
  uint fw, fh;
  g_game_depth.GetDimensions(fw, fh);
  const uint2 full_size = uint2(fw, fh);
  const uint d = (uint)GTVBAO_Divisor();
  const uint2 px = min(tp * d + d / 2u, full_size - 1u);
  const bool final_pass = GTVBAO_final_pass > 0.5f;
  const float output_scale = final_pass ? GT_VBAO_OCCLUSION_TERM_SCALE : 1.0f;
  const float raw = (float)g_raw_ao[tp] * (1.0f / 255.0f);

  const float depth = g_game_depth.Load(int3(px, 0)).x;
  const uint4 mrt = g_mrt_normal.Load(int3(px, 0));
  if (!(depth > 0.0 && any(mrt.xy != 0u)))
  {
    g_history_write[tp] = float4(0.0, 0.0, 0.0, 0.0);
    g_ao_out[tp] = (uint)(saturate(raw * output_scale) * 255.0 + 0.5);
    return;
  }

  const float3 N = DecodeFalcomMrtNormalAsGame(mrt.xy);
  const float2 uv = (float2(px) + 0.5) / float2(full_size);
  const float4 world_h = mul(float4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, depth, 1.0), viewProjInv_g);
  const float3 P = world_h.xyz / world_h.w;
  const float dist_cur = length(P - float3(viewInv_g._m30, viewInv_g._m31, viewInv_g._m32));
  float motion_px = 0.0;
  float depth_spread = 0.0;
  const float2 hist_uv = CustomTAA_Reproject(
      uv, g_game_depth, g_samp_point, g_motion, g_samp_linear,
      1.0 / float2(full_size), float2(1.0, 1.0), motion_px, depth_spread);
  const bool in_bounds = all(hist_uv >= 0.0) && all(hist_uv <= 1.0);

  // 2x2 bilinear history taps at AO size, each weighted by the depth test and the soft normal weight.
  const float2 hp = hist_uv * float2(size) - 0.5;
  const int2 base = (int2)floor(hp);
  const float2 f = hp - floor(hp);
  float weight_total = 0.0;
  float weight_accepted = 0.0;
  float history_sum = 0.0;
  [unroll]
  for (int t = 0; t < 4; ++t)
  {
    const int2 offset = int2(t & 1, t >> 1);
    const float w = (offset.x == 1 ? f.x : 1.0 - f.x) * (offset.y == 1 ? f.y : 1.0 - f.y);
    const int2 tap = clamp(base + offset, int2(0, 0), size - 1);
    weight_total += w;
    if (!in_bounds) continue;
    const float4 h = g_history_read.Load(int3(tap, 0));
    if (h.g <= 0.0) continue;
    if (!TemporalDepthAccept(h.g, dist_cur, GTVBAO_temporal_depth_rejection)) continue;
    const float wn = TemporalNormalWeight(dot(OctDecode(h.ba), N), GTVBAO_temporal_normal_rejection);
    weight_accepted += w * wn;
    history_sum += w * wn * h.r;
  }
  const bool history_valid = GTVBAO_temporal_history_valid > 0.5f;
  const float valid_fraction = (history_valid && weight_total > 0.0 && weight_accepted > 1e-4)
      ? saturate(weight_accepted / weight_total) : 0.0;
  float history_ao = valid_fraction > 0.0 ? history_sum / weight_accepted : raw;

  // History clamp: 3x3 neighbourhood of the raw AO, mean +/- k * sigma. k = 0 = off.
  if (GTVBAO_temporal_history_clamp > 0.0 && valid_fraction > 0.0)
  {
    float s1 = 0.0;
    float s2 = 0.0;
    float n = 0.0;
    [unroll]
    for (int dy = -1; dy <= 1; ++dy)
    {
      [unroll]
      for (int dx = -1; dx <= 1; ++dx)
      {
        const int2 q = clamp(int2(tp) + int2(dx, dy), int2(0, 0), size - 1);
        const float v = (float)g_raw_ao[q] * (1.0f / 255.0f);
        s1 += v;
        s2 += v * v;
        n += 1.0;
      }
    }
    history_ao = ClampToNeighbourhood(history_ao, s1, s2, n, GTVBAO_temporal_history_clamp);
  }

  const float ao = lerp(raw, history_ao, GTVBAO_temporal_history_weight * valid_fraction);
  g_history_write[tp] = float4(ao, dist_cur, OctEncode(N));
  g_ao_out[tp] = (uint)(saturate(ao * output_scale) * 255.0 + 0.5);

  const int debug_mode = (int)GTVBAO_debug_mode;
  if (debug_mode == 30 || debug_mode == 31)
  {
    const float v = debug_mode == 30 ? valid_fraction : ao;
    for (uint by = 0u; by < d; ++by)
    {
      for (uint bx = 0u; bx < d; ++bx)
      {
        const uint2 block_px = tp * d + uint2(bx, by);
        if (all(block_px < full_size)) g_debug[block_px] = float4(v, v, v, 1.0);
      }
    }
  }
}
